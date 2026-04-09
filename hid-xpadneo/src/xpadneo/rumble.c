// SPDX-License-Identifier: GPL-2.0-only

/*
 * xpadneo rumble driver
 *
 * Copyright (c) 2017 Florian Dollinger <dollinger.florian@gmx.de>
 * Copyright (c) 2026 Kai Krakow <kai@kaishome.de>
 */

#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/smp.h>

#include "xpadneo.h"
#include "helpers.h"

/* module parameter "trigger_rumble_mode" */
#define PARAM_TRIGGER_RUMBLE_PRESSURE 0
#define PARAM_TRIGGER_RUMBLE_RESERVED 1
#define PARAM_TRIGGER_RUMBLE_DISABLE  2

static u8 param_trigger_rumble_mode;
module_param_named(trigger_rumble_mode, param_trigger_rumble_mode, byte, 0644);
MODULE_PARM_DESC(trigger_rumble_mode, "(u8) Trigger rumble mode. 0: pressure, 2: disable.");

static u8 param_rumble_attenuation[2];
module_param_array_named(rumble_attenuation, param_rumble_attenuation, byte, NULL, 0644);
MODULE_PARM_DESC(rumble_attenuation,
		 "(u8) Attenuate the rumble strength: all[,triggers] "
		 "0 (none, full rumble) to 100 (max, no rumble).");

static bool param_ff_connect_notify = 1;
module_param_named(ff_connect_notify, param_ff_connect_notify, bool, 0644);
MODULE_PARM_DESC(ff_connect_notify,
		 "(bool) Connection notification using force feedback. 1: enable, 0: disable.");

static struct workqueue_struct *rumble_wq;

inline void xpadneo_rumble_streaming_set(struct xpadneo_devdata *xdata, const bool enabled)
{
	if (likely(cmpxchg(&xdata->rumble.enabled, !enabled, enabled) == !enabled))
		hid_info(xdata->hdev, "rumble streaming %s\n", enabled ? "enabled" : "disabled");
}

inline bool xpadneo_rumble_streaming_get(const struct xpadneo_devdata *xdata)
{
	/* get globally visible state if rumble streaming is enabled */
	return smp_load_acquire(&xdata->rumble.enabled);
}

static void rumble_worker(struct work_struct *work)
{
	struct xpadneo_devdata *xdata = container_of(work, struct xpadneo_devdata, rumble.worker);
	struct hid_device *hdev = xdata->hdev;
	struct xpadneo_rumble_report *r = xdata->rumble.output_report_dmabuf;
	int ret;

	memset(r, 0, sizeof(*r));
	r->report_id = XPADNEO_XBOX_RUMBLE_REPORT;

	/*
	 * if pulse is not supported, we do not care about explicitly stopping
	 * the effect, the game (or another client) is expected to do this
	 */
	if (likely((xdata->quirks & XPADNEO_QUIRK_NO_PULSE) == 0)) {
		/*
		 * We pulse the motors for 60 minutes as the Windows driver
		 * does. To work around a potential firmware crash, we filter
		 * out repeated motor programming further below.
		 */
		r->data.pulse_sustain_10ms = 0xFF;
		r->data.loop_count = 0xEB;
	}

reschedule:
	r->data.enable = XBOX_RUMBLE_ALL;

	scoped_guard(spinlock_irqsave, &xdata->rumble.lock) {
		/* only proceed once initialization data is globally visible */
		if (unlikely(!xpadneo_rumble_streaming_get(xdata)))
			goto check_pending;

		if (unlikely(xdata->quirks & XPADNEO_QUIRK_NO_TRIGGER_RUMBLE)) {
			/* do not send these bits if not supported */
			r->data.enable &= ~XBOX_RUMBLE_TRIGGERS;
		} else {
			/* trigger motors */
			r->data.magnitude_left = xdata->rumble.data.magnitude_left;
			r->data.magnitude_right = xdata->rumble.data.magnitude_right;
		}

		/* main motors */
		r->data.magnitude_strong = xdata->rumble.data.magnitude_strong;
		r->data.magnitude_weak = xdata->rumble.data.magnitude_weak;

		/* do not reprogram motors that have not changed */
		if (unlikely(xdata->rumble.shadow.magnitude_strong == r->data.magnitude_strong))
			r->data.enable &= ~XBOX_RUMBLE_STRONG;
		if (unlikely(xdata->rumble.shadow.magnitude_weak == r->data.magnitude_weak))
			r->data.enable &= ~XBOX_RUMBLE_WEAK;
		if (likely(xdata->rumble.shadow.magnitude_left == r->data.magnitude_left))
			r->data.enable &= ~XBOX_RUMBLE_LEFT;
		if (likely(xdata->rumble.shadow.magnitude_right == r->data.magnitude_right))
			r->data.enable &= ~XBOX_RUMBLE_RIGHT;

		/* do not send a report if nothing changed */
		if (unlikely(r->data.enable == XBOX_RUMBLE_NONE))
			goto check_pending;

		/* shadow our current rumble values for the next cycle */
		memcpy(&xdata->rumble.shadow, &xdata->rumble.data, sizeof(xdata->rumble.data));

		/* set all bits if not supported (some clones require these set) */
		if (unlikely(xdata->quirks & XPADNEO_QUIRK_NO_MOTOR_MASK))
			r->data.enable = XBOX_RUMBLE_ALL;

		/* reverse the bits for trigger and main motors */
		if (unlikely(xdata->quirks & XPADNEO_QUIRK_REVERSE_MASK))
			r->data.enable = SWAP_BITS(SWAP_BITS(r->data.enable, 1, 2), 0, 3);

		/* swap the bits of trigger and main motors */
		if (unlikely(xdata->quirks & XPADNEO_QUIRK_SWAPPED_MASK))
			r->data.enable = SWAP_BITS(SWAP_BITS(r->data.enable, 0, 2), 1, 3);
	}

	ret = xpadneo_device_output_report(hdev, (__u8 *) r, sizeof(*r), xdata->uses_hogp);
	if (ret < 0)
		hid_warn(hdev, "failed to send rumble report: %d\n", ret);

check_pending:
	/* pairs with smp_store_release() in rumble_playback() so we see fresh pending state */
	if (unlikely(cmpxchg(&xdata->rumble.pending, true, false))) {
		/* rumble data was still pending */
		goto reschedule;
	}
}

static inline u8 calculate_magnitude(s32 magnitude, int fraction)
{
	return (u8)((magnitude * fraction + S16_MAX) / U16_MAX);
}

static int rumble_playback(struct input_dev *dev, int effect_id, int value)
{
	int fraction_TL, fraction_TR, fraction_MAIN, percent_TRIGGERS, percent_MAIN;
	s32 weak, strong, max_main;

	struct hid_device *hdev = input_get_drvdata(dev);
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct ff_effect *effect = &xdata->gamepad.idev->ff->effects[effect_id];

	/* do not let FF clients run before rumble state is ready */
	if (unlikely(!smp_load_acquire(&xdata->rumble.enabled)))
		return 0;

	if (unlikely(effect->type != FF_RUMBLE)) {
		hid_info(hdev, "%s unexpected effect_id %d value %d\n", __func__, effect_id, value);
		return 0;
	}

	if (value == 0) {
		/* explicitly stop the effect */
		weak = 0;
		strong = 0;
	} else {
		if (value > 1)
			hid_warn_once(hdev, "%s unexpected value %d > 1\n", __func__, value);

		/* copy data from effect structure at the very beginning */
		weak = effect->u.rumble.weak_magnitude;
		strong = effect->u.rumble.strong_magnitude;
	}

	/* calculate the rumble attenuation */
	percent_MAIN = 100 - param_rumble_attenuation[0];
	percent_MAIN = clamp(percent_MAIN, 0, 100);

	percent_TRIGGERS = 100 - param_rumble_attenuation[1];
	percent_TRIGGERS = clamp(percent_TRIGGERS, 0, 100);
	percent_TRIGGERS = percent_TRIGGERS * percent_MAIN / 100;

	switch (param_trigger_rumble_mode) {
	case PARAM_TRIGGER_RUMBLE_DISABLE:
		fraction_MAIN = percent_MAIN;
		fraction_TL = 0;
		fraction_TR = 0;
		break;
	case PARAM_TRIGGER_RUMBLE_PRESSURE:
	default:
		fraction_MAIN = percent_MAIN;
		fraction_TL = (xdata->last_abs_z * percent_TRIGGERS + 511) / 1023;
		fraction_TR = (xdata->last_abs_rz * percent_TRIGGERS + 511) / 1023;
		break;
	}

	/*
	 * we want to keep the rumbling at the triggers at the maximum
	 * of the weak and strong main rumble
	 */
	max_main = max(weak, strong);

	scoped_guard(spinlock_irqsave, &xdata->rumble.lock) {
		/* calculate the physical magnitudes, scale from 16 bit to 0..100 */
		xdata->rumble.data.magnitude_strong = calculate_magnitude(strong, fraction_MAIN);
		xdata->rumble.data.magnitude_weak = calculate_magnitude(weak, fraction_MAIN);

		/* calculate the physical magnitudes, scale from 16 bit to 0..100 */
		xdata->rumble.data.magnitude_left = calculate_magnitude(max_main, fraction_TL);
		xdata->rumble.data.magnitude_right = calculate_magnitude(max_main, fraction_TR);

		/* schedule writing a rumble report to the controller */
		if (!queue_work(rumble_wq, &xdata->rumble.worker)) {
			/* the worker is still waiting on the hardware */
			smp_store_release(&xdata->rumble.pending, true);
			hid_notice_once(hdev, "throttled rumble reprogramming\n");
		}
	}

	return 0;
}

static void rumble_test(char *which, const struct xpadneo_devdata *xdata,
			struct xpadneo_rumble_report pck)
{
	enum xpadneo_rumble_motors enabled = pck.data.enable;

	hid_info(xdata->hdev, "testing %s: sustain %dms release %dms loop %d wait 30ms\n", which,
		 pck.data.pulse_sustain_10ms * 10, pck.data.pulse_release_10ms * 10,
		 pck.data.loop_count);

	/*
	 * XPADNEO_QUIRK_NO_MOTOR_MASK:
	 * The controller does not support motor masking so we set all bits, and set
	 * the magnitude to 0 instead.
	 */
	if (xdata->quirks & XPADNEO_QUIRK_NO_MOTOR_MASK) {
		pck.data.enable = XBOX_RUMBLE_ALL;
		if (!(enabled & XBOX_RUMBLE_WEAK))
			pck.data.magnitude_weak = 0;
		if (!(enabled & XBOX_RUMBLE_STRONG))
			pck.data.magnitude_strong = 0;
		if (!(enabled & XBOX_RUMBLE_RIGHT))
			pck.data.magnitude_right = 0;
		if (!(enabled & XBOX_RUMBLE_LEFT))
			pck.data.magnitude_left = 0;
	}

	/*
	 * XPADNEO_QUIRK_NO_TRIGGER_RUMBLE:
	 * The controller does not support trigger rumble, so filter for the main motors
	 * only if we enabled all before.
	 */
	if (xdata->quirks & XPADNEO_QUIRK_NO_TRIGGER_RUMBLE)
		pck.data.enable &= XBOX_RUMBLE_MAIN;

	/*
	 * XPADNEO_QUIRK_REVERSE_MASK:
	 * The controller firmware reverses the order of the motor masking bits, so we swap
	 * the bits to reverse it.
	 */
	if (xdata->quirks & XPADNEO_QUIRK_REVERSE_MASK)
		pck.data.enable = SWAP_BITS(SWAP_BITS(pck.data.enable, 1, 2), 0, 3);

	/*
	 * XPADNEO_QUIRK_SWAPPED_MASK:
	 * The controller firmware swaps the bit masks for the trigger motors with those for
	 * the main motors.
	 */
	if (xdata->quirks & XPADNEO_QUIRK_SWAPPED_MASK)
		pck.data.enable = SWAP_BITS(SWAP_BITS(pck.data.enable, 0, 2), 1, 3);

	xpadneo_device_output_report(xdata->hdev, (u8 *)&pck, sizeof(pck), xdata->uses_hogp);
	msleep(300);

	/*
	 * XPADNEO_QUIRK_NO_PULSE:
	 * The controller doesn't support timing parameters of the rumble command, so we manually
	 * stop the motors.
	 */
	if (xdata->quirks & XPADNEO_QUIRK_NO_PULSE) {
		if (enabled & XBOX_RUMBLE_WEAK)
			pck.data.magnitude_weak = 0;
		if (enabled & XBOX_RUMBLE_STRONG)
			pck.data.magnitude_strong = 0;
		if (enabled & XBOX_RUMBLE_RIGHT)
			pck.data.magnitude_right = 0;
		if (enabled & XBOX_RUMBLE_LEFT)
			pck.data.magnitude_left = 0;
		xpadneo_device_output_report(xdata->hdev, (u8 *)&pck, sizeof(pck),
					     xdata->uses_hogp);
	}
	msleep(30);
}

static void rumble_welcome(const struct xpadneo_devdata *xdata)
{
	struct xpadneo_rumble_report pck = { };

	pck.report_id = XPADNEO_XBOX_RUMBLE_REPORT;

	/*
	 * Initialize the motor magnitudes here, the test command will individually zero masked
	 * magnitudes if needed.
	 */
	pck.data.magnitude_weak = 40;
	pck.data.magnitude_strong = 20;
	pck.data.magnitude_right = 10;
	pck.data.magnitude_left = 10;

	/*
	 * XPADNEO_QUIRK_NO_PULSE:
	 * If the controller doesn't support timing parameters of the rumble command, don't set
	 * them. Otherwise we may miss controllers that actually do support the parameters.
	 */
	if (!(xdata->quirks & XPADNEO_QUIRK_NO_PULSE)) {
		pck.data.pulse_sustain_10ms = 5;
		pck.data.pulse_release_10ms = 5;
		pck.data.loop_count = 2;
	}

	pck.data.enable = XBOX_RUMBLE_WEAK;
	rumble_test("weak motor", xdata, pck);

	pck.data.enable = XBOX_RUMBLE_STRONG;
	rumble_test("strong motor", xdata, pck);

	if (!(xdata->quirks & XPADNEO_QUIRK_NO_TRIGGER_RUMBLE)) {
		pck.data.enable = XBOX_RUMBLE_TRIGGERS;
		rumble_test("trigger motors", xdata, pck);
	}
}

static void rumble_welcome_worker(struct work_struct *work)
{
	struct xpadneo_devdata *xdata =
	    container_of(work, struct xpadneo_devdata, rumble.init_worker);

	xpadneo_benchmark(rumble_welcome, xdata);

	hid_info(xdata->hdev,
		 "please report a bug if your controller did not rumble: uses_hogp %d\n",
		 xdata->uses_hogp);

	xpadneo_rumble_streaming_set(xdata, true);
}

static int ff_upload_effect(struct input_dev *dev, struct ff_effect *effect, struct ff_effect *old)
{
	struct hid_device *hdev = input_get_drvdata(dev);
	const char *type_name = "UNKNOWN";

	/* only log what effect type is being uploaded */

	switch (effect->type) {
	case FF_PERIODIC:
		type_name = "FF_PERIODIC";
		break;
	case FF_CONSTANT:
		type_name = "FF_CONSTANT";
		break;
	case FF_SPRING:
		type_name = "FF_SPRING";
		break;
	case FF_DAMPER:
		type_name = "FF_DAMPER";
		break;
	case FF_RUMBLE:
		/* only force effects should have a direction */
		if (unlikely(effect->direction))
			hid_warn_once(hdev, "%s unexpected direction %d for non-force effect\n",
				      __func__, effect->direction);

		/* games are expected to trigger rumble effects autonomously */
		if (unlikely(effect->trigger.button || effect->trigger.interval))
			hid_warn_once(hdev,
				      "%s unexpected triggering condition %d:%d for rumble effect\n",
				      __func__, effect->trigger.button, effect->trigger.interval);

		/* replay should not be used when streaming rumble effects */
		if (unlikely(((effect->replay.length != U16_MAX) && (effect->replay.length != 0))
			     || effect->replay.delay))
			hid_warn_once(hdev,
				      "%s unexpected replay parameters %u:%u while streaming rumble\n",
				      __func__, effect->replay.length, effect->replay.delay);

		/* do not log, effect is known */
		return 0;
	}

	hid_info(hdev, "ff: effect upload requested (id %d, type %s), rejecting.\n",
		 effect->id, type_name);

	return -EOPNOTSUPP;
}

static int ff_erase_effect(struct input_dev *dev, int effect_id)
{
	struct hid_device *hdev = input_get_drvdata(dev);

	hid_info(hdev, "ff: effect %d erased\n", effect_id);
	return 0;
}

int xpadneo_rumble_init(struct hid_device *hdev)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct input_dev *gamepad = xdata->gamepad.idev;
	int ret;

	/* publish that rumble is not ready until init finishes */
	smp_store_release(&xdata->rumble.enabled, false);

	/* clear any pending rumble data */
	smp_store_release(&xdata->rumble.pending, false);

	spin_lock_init(&xdata->rumble.lock);
	INIT_WORK(&xdata->rumble.worker, rumble_worker);
	INIT_WORK(&xdata->rumble.init_worker, rumble_welcome_worker);
	xdata->rumble.output_report_dmabuf = devm_kzalloc(&hdev->dev,
							  sizeof(struct xpadneo_rumble_report),
							  GFP_KERNEL);
	if (xdata->rumble.output_report_dmabuf == NULL)
		return -ENOMEM;

	if (param_trigger_rumble_mode == PARAM_TRIGGER_RUMBLE_DISABLE)
		xdata->quirks |= XPADNEO_QUIRK_NO_TRIGGER_RUMBLE;

	/* set capabilities */
	input_set_capability(gamepad, EV_FF, FF_RUMBLE);
	ret = input_ff_create(gamepad, FF_MAX_EFFECTS);
	if (ret)
		return ret;

	/* initialize rumble callbacks */
	gamepad->ff->upload = ff_upload_effect;
	gamepad->ff->erase = ff_erase_effect;
	gamepad->ff->playback = rumble_playback;

	if (param_ff_connect_notify)
		queue_work(rumble_wq, &xdata->rumble.init_worker);
	else
		/* publish readiness once all rumble state is initialized */
		xpadneo_rumble_streaming_set(xdata, true);

	return 0;
}

int xpadneo_rumble_init_workqueue(void)
{
	if (param_trigger_rumble_mode == 1)
		pr_warn("hid-xpadneo trigger_rumble_mode=1 is unknown, defaulting to 0\n");

	rumble_wq = alloc_ordered_workqueue("xpadneo/rumbled", WQ_HIGHPRI);
	if (rumble_wq)
		return 0;

	return -ENOMEM;
}

void xpadneo_rumble_destroy_workqueue(void)
{
	destroy_workqueue(rumble_wq);
	rumble_wq = NULL;
}

void xpadneo_rumble_remove(struct xpadneo_devdata *xdata)
{
	/* disable rumble before removable to prevent queueing new data */
	xpadneo_rumble_streaming_set(xdata, false);

	cancel_work_sync(&xdata->rumble.init_worker);
	cancel_work_sync(&xdata->rumble.worker);
}
