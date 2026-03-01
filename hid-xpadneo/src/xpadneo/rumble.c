// SPDX-License-Identifier: GPL-3.0-only

/*
 * xpadneo rumble driver
 *
 * Copyright (c) 2017 Florian Dollinger <dollinger.florian@gmx.de>
 * Copyright (c) 2026 Kai Krakow <kai@kaishome.de>
 */

#include <linux/delay.h>
#include <linux/module.h>

#include "../xpadneo.h"

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

static void xpadneo_rumble_worker(struct work_struct *work)
{
	struct xpadneo_devdata *xdata =
	    container_of(to_delayed_work(work), struct xpadneo_devdata, rumble.worker);
	struct hid_device *hdev = xdata->hdev;
	struct rumble_report *r = xdata->rumble.output_report_dmabuf;
	int ret;
	unsigned long flags;

	memset(r, 0, sizeof(*r));
	r->report_id = XPADNEO_XB1S_FF_REPORT;
	r->data.enable = FF_RUMBLE_ALL;

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

	spin_lock_irqsave(&xdata->rumble.lock, flags);

	/* let our scheduler know we've been called */
	xdata->rumble.scheduled = false;

	if (unlikely(xdata->quirks & XPADNEO_QUIRK_NO_TRIGGER_RUMBLE)) {
		/* do not send these bits if not supported */
		r->data.enable &= ~FF_RUMBLE_TRIGGERS;
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
		r->data.enable &= ~FF_RUMBLE_STRONG;
	if (unlikely(xdata->rumble.shadow.magnitude_weak == r->data.magnitude_weak))
		r->data.enable &= ~FF_RUMBLE_WEAK;
	if (likely(xdata->rumble.shadow.magnitude_left == r->data.magnitude_left))
		r->data.enable &= ~FF_RUMBLE_LEFT;
	if (likely(xdata->rumble.shadow.magnitude_right == r->data.magnitude_right))
		r->data.enable &= ~FF_RUMBLE_RIGHT;

	/* do not send a report if nothing changed */
	if (unlikely(r->data.enable == FF_RUMBLE_NONE)) {
		spin_unlock_irqrestore(&xdata->rumble.lock, flags);
		return;
	}

	/* shadow our current rumble values for the next cycle */
	memcpy(&xdata->rumble.shadow, &xdata->rumble.data, sizeof(xdata->rumble.data));

	/* clear the magnitudes to properly accumulate the maximum values */
	xdata->rumble.data.magnitude_left = 0;
	xdata->rumble.data.magnitude_right = 0;
	xdata->rumble.data.magnitude_weak = 0;
	xdata->rumble.data.magnitude_strong = 0;

	/*
	 * throttle next command submission, the firmware doesn't like us to
	 * send rumble data any faster
	 */
	xdata->rumble.throttle_until = XPADNEO_RUMBLE_THROTTLE_JIFFIES;

	spin_unlock_irqrestore(&xdata->rumble.lock, flags);

	/* set all bits if not supported (some clones require these set) */
	if (unlikely(xdata->quirks & XPADNEO_QUIRK_NO_MOTOR_MASK))
		r->data.enable = FF_RUMBLE_ALL;

	/* reverse the bits for trigger and main motors */
	if (unlikely(xdata->quirks & XPADNEO_QUIRK_REVERSE_MASK))
		r->data.enable = SWAP_BITS(SWAP_BITS(r->data.enable, 1, 2), 0, 3);

	/* swap the bits of trigger and main motors */
	if (unlikely(xdata->quirks & XPADNEO_QUIRK_SWAPPED_MASK))
		r->data.enable = SWAP_BITS(SWAP_BITS(r->data.enable, 0, 2), 1, 3);

	ret = xpadneo_output_report(hdev, (__u8 *) r, sizeof(*r));
	if (ret < 0)
		hid_warn(hdev, "failed to send FF report: %d\n", ret);
}

static inline void update_magnitude(u8 *m, u8 v)
{
	*m = v > 0 ? max(*m, v) : 0;
}

static int xpadneo_rumble_play(struct input_dev *dev, void *data, struct ff_effect *effect)
{
	unsigned long flags, rumble_run_at, rumble_throttle_until;
	long delay_work;
	int fraction_TL, fraction_TR, fraction_MAIN, percent_TRIGGERS, percent_MAIN;
	s32 weak, strong, max_main;

	struct hid_device *hdev = input_get_drvdata(dev);
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	if (effect->type != FF_RUMBLE)
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

	spin_lock_irqsave(&xdata->rumble.lock, flags);

	/* calculate the physical magnitudes, scale from 16 bit to 0..100 */
	update_magnitude(&xdata->rumble.data.magnitude_strong,
			 (u8)((strong * fraction_MAIN + S16_MAX) / U16_MAX));
	update_magnitude(&xdata->rumble.data.magnitude_weak,
			 (u8)((weak * fraction_MAIN + S16_MAX) / U16_MAX));

	/* calculate the physical magnitudes, scale from 16 bit to 0..100 */
	update_magnitude(&xdata->rumble.data.magnitude_left,
			 (u8)((max_main * fraction_TL + S16_MAX) / U16_MAX));
	update_magnitude(&xdata->rumble.data.magnitude_right,
			 (u8)((max_main * fraction_TR + S16_MAX) / U16_MAX));

	/* synchronize: is our worker still scheduled? */
	if (xdata->rumble.scheduled) {
		/* the worker is still guarding rumble programming */
		hid_notice_once(hdev, "throttling rumble reprogramming\n");
		goto unlock_and_return;
	}

	/* we want to run now but may be throttled */
	rumble_run_at = jiffies;
	rumble_throttle_until = xdata->rumble.throttle_until;
	if (time_before(rumble_run_at, rumble_throttle_until)) {
		/* last rumble was recently executed */
		delay_work = (long)rumble_throttle_until - (long)rumble_run_at;
		delay_work = clamp(delay_work, 0L, (long)HZ);
	} else {
		/* the firmware is ready */
		delay_work = 0;
	}

	/* schedule writing a rumble report to the controller */
	if (queue_delayed_work(rumble_wq, &xdata->rumble.worker, delay_work))
		xdata->rumble.scheduled = true;
	else
		hid_err(hdev, "lost rumble packet\n");

unlock_and_return:
	spin_unlock_irqrestore(&xdata->rumble.lock, flags);
	return 0;
}

static void xpadneo_rumble_test(char *which, struct xpadneo_devdata *xdata,
				struct rumble_report pck)
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
		pck.data.enable = FF_RUMBLE_ALL;
		if (!(enabled & FF_RUMBLE_WEAK))
			pck.data.magnitude_weak = 0;
		if (!(enabled & FF_RUMBLE_STRONG))
			pck.data.magnitude_strong = 0;
		if (!(enabled & FF_RUMBLE_RIGHT))
			pck.data.magnitude_right = 0;
		if (!(enabled & FF_RUMBLE_LEFT))
			pck.data.magnitude_left = 0;
	}

	/*
	 * XPADNEO_QUIRK_NO_TRIGGER_RUMBLE:
	 * The controller does not support trigger rumble, so filter for the main motors
	 * only if we enabled all before.
	 */
	if (xdata->quirks & XPADNEO_QUIRK_NO_TRIGGER_RUMBLE)
		pck.data.enable &= FF_RUMBLE_MAIN;

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

	xpadneo_output_report(xdata->hdev, (u8 *)&pck, sizeof(pck));
	mdelay(300);

	/*
	 * XPADNEO_QUIRK_NO_PULSE:
	 * The controller doesn't support timing parameters of the rumble command, so we manually
	 * stop the motors.
	 */
	if (xdata->quirks & XPADNEO_QUIRK_NO_PULSE) {
		if (enabled & FF_RUMBLE_WEAK)
			pck.data.magnitude_weak = 0;
		if (enabled & FF_RUMBLE_STRONG)
			pck.data.magnitude_strong = 0;
		if (enabled & FF_RUMBLE_RIGHT)
			pck.data.magnitude_right = 0;
		if (enabled & FF_RUMBLE_LEFT)
			pck.data.magnitude_left = 0;
		xpadneo_output_report(xdata->hdev, (u8 *)&pck, sizeof(pck));
	}
	mdelay(30);
}

static void xpadneo_rumble_welcome(struct hid_device *hdev)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct rumble_report pck = { };

	pck.report_id = XPADNEO_XB1S_FF_REPORT;

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

	pck.data.enable = FF_RUMBLE_WEAK;
	xpadneo_rumble_test("weak motor", xdata, pck);

	pck.data.enable = FF_RUMBLE_STRONG;
	xpadneo_rumble_test("strong motor", xdata, pck);

	if (!(xdata->quirks & XPADNEO_QUIRK_NO_TRIGGER_RUMBLE)) {
		pck.data.enable = FF_RUMBLE_TRIGGERS;
		xpadneo_rumble_test("trigger motors", xdata, pck);
	}
}

int xpadneo_rumble_init(struct hid_device *hdev)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct input_dev *gamepad = xdata->gamepad;

	spin_lock_init(&xdata->rumble.lock);
	INIT_DELAYED_WORK(&xdata->rumble.worker, xpadneo_rumble_worker);
	xdata->rumble.output_report_dmabuf = devm_kzalloc(&hdev->dev,
							  sizeof(struct rumble_report), GFP_KERNEL);
	if (xdata->rumble.output_report_dmabuf == NULL)
		return -ENOMEM;

	if (param_trigger_rumble_mode == PARAM_TRIGGER_RUMBLE_DISABLE)
		xdata->quirks |= XPADNEO_QUIRK_NO_TRIGGER_RUMBLE;

	if (param_ff_connect_notify) {
		xpadneo_benchmark_start(xpadneo_welcome_rumble);
		xpadneo_rumble_welcome(hdev);
		xpadneo_benchmark_stop(xpadneo_welcome_rumble);
	}

	/* initialize our rumble command throttle */
	xdata->rumble.throttle_until = XPADNEO_RUMBLE_THROTTLE_JIFFIES;

	input_set_capability(gamepad, EV_FF, FF_RUMBLE);
	return input_ff_create_memless(gamepad, NULL, xpadneo_rumble_play);
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
	cancel_delayed_work_sync(&xdata->rumble.worker);
}
