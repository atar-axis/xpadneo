// SPDX-License-Identifier: GPL-2.0-only

/*
 * xpadneo rumble driver
 *
 * Copyright (c) 2017 Florian Dollinger <dollinger.florian@gmx.de>
 * Copyright (c) 2026 Kai Krakow <kai@kaishome.de>
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/smp.h>

#include "xpadneo.h"
#include "helpers.h"

/* timing of rumble commands to work around firmware crashes */
#define RUMBLE_THROTTLE_DELAY   (msecs_to_jiffies(10)+1)
#define RUMBLE_THROTTLE_JIFFIES (jiffies + RUMBLE_THROTTLE_DELAY)

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

static inline unsigned long calculate_throttling_delay(struct xpadneo_devdata *xdata)
{
	unsigned long rumble_run_at = jiffies, rumble_throttle_until = xdata->rumble.throttle_until;

	if (time_before(rumble_run_at, rumble_throttle_until)) {
		/* last rumble was recently executed */
		long delay_work = (long)(rumble_throttle_until - rumble_run_at);

		return clamp(delay_work, 0L, (long)RUMBLE_THROTTLE_DELAY);
	}

	/* the firmware is ready */
	return 0;
}

static void rumble_worker(struct work_struct *work)
{
	struct xpadneo_devdata *xdata =
	    container_of(to_delayed_work(work), struct xpadneo_devdata, rumble.worker);
	struct hid_device *hdev = xdata->hdev;
	struct xpadneo_rumble_report *r = xdata->rumble.output_report_dmabuf;
	int ret;

	memset(r, 0, sizeof(*r));
	r->report_id = XPADNEO_XBOX_RUMBLE_REPORT;
	r->data.enable = XBOX_RUMBLE_ALL;

	/*
	 * if pulse is not supported, we do not have to care about explicitly
	 * stopping the effect, the kernel will do this for us as part of its
	 * ff-memless emulation
	 */
	if (likely((xdata->quirks & XPADNEO_QUIRK_NO_PULSE) == 0)) {
		/*
		 * ff-memless has a time resolution of 50ms but we pulse the
		 * motors for 60 minutes as the Windows driver does. To work
		 * around a potential firmware crash, we filter out repeated
		 * motor programming further below.
		 */
		r->data.pulse_sustain_10ms = 0xFF;
		r->data.loop_count = 0xEB;
	}

	scoped_guard(spinlock_irq, &xdata->rumble.lock) {

		/* let our scheduler know we've been called */
		xdata->rumble.scheduled = false;

		/* only proceed once initialization data is globally visible */
		if (unlikely(!smp_load_acquire(&xdata->rumble.enabled)))
			return;

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
			return;

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

	ret = xpadneo_device_output_report(hdev, (__u8 *) r, sizeof(*r));
	if (ret < 0)
		hid_warn(hdev, "failed to send FF report: %d\n", ret);

	/*
	 * throttle next command submission, the firmware doesn't like us to
	 * send rumble data any faster
	 */
	scoped_guard(spinlock_irq, &xdata->rumble.lock) {
		xdata->rumble.throttle_until = RUMBLE_THROTTLE_JIFFIES;
		if (xdata->rumble.scheduled) {
			unsigned long delay_work = calculate_throttling_delay(xdata);

			mod_delayed_work(rumble_wq, &xdata->rumble.worker, delay_work);
		}
	}
}

static inline u8 calculate_magnitude(s32 magnitude, int fraction)
{
	return (u8)((magnitude * fraction + S16_MAX) / U16_MAX);
}

static int rumble_play(struct input_dev *dev, void *data, struct ff_effect *effect)
{
	int fraction_TL, fraction_TR, fraction_MAIN, percent_TRIGGERS, percent_MAIN;
	s32 weak, strong, max_main;

	struct hid_device *hdev = input_get_drvdata(dev);
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	/* do not let FF clients run before rumble state is ready */
	if (unlikely(!smp_load_acquire(&xdata->rumble.enabled)))
		return 0;

	if (unlikely(effect->type != FF_RUMBLE))
		return 0;

	/* copy data from effect structure at the very beginning */
	weak = effect->u.rumble.weak_magnitude;
	strong = effect->u.rumble.strong_magnitude;

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

	scoped_guard(spinlock_irq, &xdata->rumble.lock) {
		unsigned long delay_work;

		/* calculate the physical magnitudes, scale from 16 bit to 0..100 */
		xdata->rumble.data.magnitude_strong = calculate_magnitude(strong, fraction_MAIN);
		xdata->rumble.data.magnitude_weak = calculate_magnitude(weak, fraction_MAIN);

		/* calculate the physical magnitudes, scale from 16 bit to 0..100 */
		xdata->rumble.data.magnitude_left = calculate_magnitude(max_main, fraction_TL);
		xdata->rumble.data.magnitude_right = calculate_magnitude(max_main, fraction_TR);

		/* synchronize: is our worker still scheduled? */
		if (xdata->rumble.scheduled) {
			/* the worker is still guarding rumble programming */
			hid_notice_once(hdev, "throttling rumble reprogramming\n");
			break;
		}

		/* we want to run now but may be throttled */
		delay_work = calculate_throttling_delay(xdata);

		/* schedule writing a rumble report to the controller */
		if (mod_delayed_work(rumble_wq, &xdata->rumble.worker, delay_work)) {
			/* this should never happen */
			hid_err(hdev, "rumble_playback: unexpected scheduling state\n");
		}
		xdata->rumble.scheduled = true;
	}

	return 0;
}

static void rumble_test(char *which, struct xpadneo_devdata *xdata,
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

	xpadneo_device_output_report(xdata->hdev, (u8 *)&pck, sizeof(pck));
	mdelay(300);

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
		xpadneo_device_output_report(xdata->hdev, (u8 *)&pck, sizeof(pck));
	}
	mdelay(30);
}

static void rumble_welcome(struct hid_device *hdev)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
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

int xpadneo_rumble_init(struct hid_device *hdev)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct input_dev *gamepad = xdata->gamepad.idev;
	int ret;

	/* publish that rumble is not ready until init finishes */
	smp_store_release(&xdata->rumble.enabled, false);

	spin_lock_init(&xdata->rumble.lock);
	INIT_DELAYED_WORK(&xdata->rumble.worker, rumble_worker);
	xdata->rumble.output_report_dmabuf = devm_kzalloc(&hdev->dev,
							  sizeof(struct xpadneo_rumble_report),
							  GFP_KERNEL);
	if (xdata->rumble.output_report_dmabuf == NULL)
		return -ENOMEM;

	if (param_trigger_rumble_mode == PARAM_TRIGGER_RUMBLE_DISABLE)
		xdata->quirks |= XPADNEO_QUIRK_NO_TRIGGER_RUMBLE;

	/* initialize our rumble command throttle */
	xdata->rumble.throttle_until = RUMBLE_THROTTLE_JIFFIES;

	/* set capabilities */
	input_set_capability(gamepad, EV_FF, FF_RUMBLE);
	ret = input_ff_create_memless(gamepad, NULL, rumble_play);
	if (ret)
		return ret;

	if (param_ff_connect_notify)
		xpadneo_benchmark(rumble_welcome, hdev);

	/* publish readiness once all rumble state is initialized */
	smp_store_release(&xdata->rumble.enabled, true);

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
	smp_store_release(&xdata->rumble.enabled, false);

	cancel_delayed_work_sync(&xdata->rumble.worker);
}
