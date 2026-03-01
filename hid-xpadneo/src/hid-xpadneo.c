/*
 * Force feedback support for XBOX ONE S and X gamepads via Bluetooth
 *
 * This driver was developed for a student project at fortiss GmbH in Munich.
 * Copyright (c) 2017 Florian Dollinger <dollinger.florian@gmx.de>
 *
 * Additional features and code redesign
 * Copyright (c) 2020 Kai Krakow <kai@kaishome.de>
 */

#include <linux/delay.h>
#include <linux/module.h>

#include "xpadneo.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Florian Dollinger <dollinger.florian@gmx.de>");
MODULE_AUTHOR("Kai Krakow <kai@kaishome.de>");
MODULE_DESCRIPTION("Linux kernel driver for Xbox ONE S+ gamepads (BT), incl. FF");
MODULE_VERSION(XPADNEO_VERSION);

static u8 param_trigger_rumble_mode = 0;
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

static bool param_debug_hid = 0;
module_param_named(debug_hid, param_debug_hid, bool, 0644);
MODULE_PARM_DESC(debug_hid, "(u8) Debug HID reports. 0: disable, 1: enable.");

static DEFINE_IDA(xpadneo_device_id_allocator);

static struct workqueue_struct *xpadneo_rumble_wq;

static int xpadneo_output_report(struct hid_device *hdev, __u8 *buf, size_t len)
{
	struct ff_report *r = (struct ff_report *)buf;

	if (unlikely(param_debug_hid && (len > 0))) {
		switch (buf[0]) {
		case 0x03:
			if (len >= sizeof(*r)) {
				hid_info(hdev,
					 "HID debug: len %ld rumble cmd 0x%02x "
					 "motors left %d right %d strong %d weak %d "
					 "magnitude left %d right %d strong %d weak %d "
					 "pulse sustain %dms release %dms loop %d\n",
					 len, r->report_id,
					 !!(r->ff.enable & FF_RUMBLE_LEFT),
					 !!(r->ff.enable & FF_RUMBLE_RIGHT),
					 !!(r->ff.enable & FF_RUMBLE_STRONG),
					 !!(r->ff.enable & FF_RUMBLE_WEAK),
					 r->ff.magnitude_left, r->ff.magnitude_right,
					 r->ff.magnitude_strong, r->ff.magnitude_weak,
					 r->ff.pulse_sustain_10ms * 10,
					 r->ff.pulse_release_10ms * 10, r->ff.loop_count);
			} else {
				hid_info(hdev, "HID debug: len %ld malformed cmd 0x%02x\n", len,
					 buf[0]);
			}
			break;
		default:
			hid_info(hdev, "HID debug: len %ld unhandled cmd 0x%02x\n", len, buf[0]);
		}
	}
	return hid_hw_output_report(hdev, buf, len);
}

static void xpadneo_ff_worker(struct work_struct *work)
{
	struct xpadneo_devdata *xdata =
	    container_of(to_delayed_work(work), struct xpadneo_devdata, ff_worker);
	struct hid_device *hdev = xdata->hdev;
	struct ff_report *r = xdata->output_report_dmabuf;
	int ret;
	unsigned long flags;

	memset(r, 0, sizeof(*r));
	r->report_id = XPADNEO_XB1S_FF_REPORT;
	r->ff.enable = FF_RUMBLE_ALL;

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
		r->ff.pulse_sustain_10ms = 0xFF;
		r->ff.loop_count = 0xEB;
	}

	spin_lock_irqsave(&xdata->ff_lock, flags);

	/* let our scheduler know we've been called */
	xdata->ff_scheduled = false;

	if (unlikely(xdata->quirks & XPADNEO_QUIRK_NO_TRIGGER_RUMBLE)) {
		/* do not send these bits if not supported */
		r->ff.enable &= ~FF_RUMBLE_TRIGGERS;
	} else {
		/* trigger motors */
		r->ff.magnitude_left = xdata->ff.magnitude_left;
		r->ff.magnitude_right = xdata->ff.magnitude_right;
	}

	/* main motors */
	r->ff.magnitude_strong = xdata->ff.magnitude_strong;
	r->ff.magnitude_weak = xdata->ff.magnitude_weak;

	/* do not reprogram motors that have not changed */
	if (unlikely(xdata->ff_shadow.magnitude_strong == r->ff.magnitude_strong))
		r->ff.enable &= ~FF_RUMBLE_STRONG;
	if (unlikely(xdata->ff_shadow.magnitude_weak == r->ff.magnitude_weak))
		r->ff.enable &= ~FF_RUMBLE_WEAK;
	if (likely(xdata->ff_shadow.magnitude_left == r->ff.magnitude_left))
		r->ff.enable &= ~FF_RUMBLE_LEFT;
	if (likely(xdata->ff_shadow.magnitude_right == r->ff.magnitude_right))
		r->ff.enable &= ~FF_RUMBLE_RIGHT;

	/* do not send a report if nothing changed */
	if (unlikely(r->ff.enable == FF_RUMBLE_NONE)) {
		spin_unlock_irqrestore(&xdata->ff_lock, flags);
		return;
	}

	/* shadow our current rumble values for the next cycle */
	memcpy(&xdata->ff_shadow, &xdata->ff, sizeof(xdata->ff));

	/* clear the magnitudes to properly accumulate the maximum values */
	xdata->ff.magnitude_left = 0;
	xdata->ff.magnitude_right = 0;
	xdata->ff.magnitude_weak = 0;
	xdata->ff.magnitude_strong = 0;

	/*
	 * throttle next command submission, the firmware doesn't like us to
	 * send rumble data any faster
	 */
	xdata->ff_throttle_until = XPADNEO_RUMBLE_THROTTLE_JIFFIES;

	spin_unlock_irqrestore(&xdata->ff_lock, flags);

	/* set all bits if not supported (some clones require these set) */
	if (unlikely(xdata->quirks & XPADNEO_QUIRK_NO_MOTOR_MASK))
		r->ff.enable = FF_RUMBLE_ALL;

	/* reverse the bits for trigger and main motors */
	if (unlikely(xdata->quirks & XPADNEO_QUIRK_REVERSE_MASK))
		r->ff.enable = SWAP_BITS(SWAP_BITS(r->ff.enable, 1, 2), 0, 3);

	/* swap the bits of trigger and main motors */
	if (unlikely(xdata->quirks & XPADNEO_QUIRK_SWAPPED_MASK))
		r->ff.enable = SWAP_BITS(SWAP_BITS(r->ff.enable, 0, 2), 1, 3);

	ret = xpadneo_output_report(hdev, (__u8 *) r, sizeof(*r));
	if (ret < 0)
		hid_warn(hdev, "failed to send FF report: %d\n", ret);
}

#define update_magnitude(m, v) m = (v) > 0 ? max(m, v) : 0
static int xpadneo_ff_play(struct input_dev *dev, void *data, struct ff_effect *effect)
{
	unsigned long flags, ff_run_at, ff_throttle_until;
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

	spin_lock_irqsave(&xdata->ff_lock, flags);

	/* calculate the physical magnitudes, scale from 16 bit to 0..100 */
	update_magnitude(xdata->ff.magnitude_strong,
			 (u8)((strong * fraction_MAIN + S16_MAX) / U16_MAX));
	update_magnitude(xdata->ff.magnitude_weak,
			 (u8)((weak * fraction_MAIN + S16_MAX) / U16_MAX));

	/* calculate the physical magnitudes, scale from 16 bit to 0..100 */
	update_magnitude(xdata->ff.magnitude_left,
			 (u8)((max_main * fraction_TL + S16_MAX) / U16_MAX));
	update_magnitude(xdata->ff.magnitude_right,
			 (u8)((max_main * fraction_TR + S16_MAX) / U16_MAX));

	/* synchronize: is our worker still scheduled? */
	if (xdata->ff_scheduled) {
		/* the worker is still guarding rumble programming */
		hid_notice_once(hdev, "throttling rumble reprogramming\n");
		goto unlock_and_return;
	}

	/* we want to run now but may be throttled */
	ff_run_at = jiffies;
	ff_throttle_until = xdata->ff_throttle_until;
	if (time_before(ff_run_at, ff_throttle_until)) {
		/* last rumble was recently executed */
		delay_work = (long)ff_throttle_until - (long)ff_run_at;
		delay_work = clamp(delay_work, 0L, (long)HZ);
	} else {
		/* the firmware is ready */
		delay_work = 0;
	}

	/* schedule writing a rumble report to the controller */
	if (queue_delayed_work(xpadneo_rumble_wq, &xdata->ff_worker, delay_work))
		xdata->ff_scheduled = true;
	else
		hid_err(hdev, "lost rumble packet\n");

unlock_and_return:
	spin_unlock_irqrestore(&xdata->ff_lock, flags);
	return 0;
}

static void xpadneo_test_rumble(char *which, struct xpadneo_devdata *xdata, struct ff_report pck)
{
	enum xpadneo_rumble_motors enabled = pck.ff.enable;

	hid_info(xdata->hdev, "testing %s: sustain %dms release %dms loop %d wait 30ms\n", which,
		 pck.ff.pulse_sustain_10ms * 10, pck.ff.pulse_release_10ms * 10, pck.ff.loop_count);

	/*
	 * XPADNEO_QUIRK_NO_MOTOR_MASK:
	 * The controller does not support motor masking so we set all bits, and set the magnitude to 0 instead.
	 */
	if (xdata->quirks & XPADNEO_QUIRK_NO_MOTOR_MASK) {
		pck.ff.enable = FF_RUMBLE_ALL;
		if (!(enabled & FF_RUMBLE_WEAK))
			pck.ff.magnitude_weak = 0;
		if (!(enabled & FF_RUMBLE_STRONG))
			pck.ff.magnitude_strong = 0;
		if (!(enabled & FF_RUMBLE_RIGHT))
			pck.ff.magnitude_right = 0;
		if (!(enabled & FF_RUMBLE_LEFT))
			pck.ff.magnitude_left = 0;
	}

	/*
	 * XPADNEO_QUIRK_NO_TRIGGER_RUMBLE:
	 * The controller does not support trigger rumble, so filter for the main motors only if we enabled all
	 * before.
	 */
	if (xdata->quirks & XPADNEO_QUIRK_NO_TRIGGER_RUMBLE)
		pck.ff.enable &= FF_RUMBLE_MAIN;

	/*
	 * XPADNEO_QUIRK_REVERSE_MASK:
	 * The controller firmware reverses the order of the motor masking bits, so we swap the bits to reverse it.
	 */
	if (xdata->quirks & XPADNEO_QUIRK_REVERSE_MASK)
		pck.ff.enable = SWAP_BITS(SWAP_BITS(pck.ff.enable, 1, 2), 0, 3);

	/*
	 * XPADNEO_QUIRK_SWAPPED_MASK:
	 * The controller firmware swaps the bit masks for the trigger motors with those for the main motors.
	 */
	if (xdata->quirks & XPADNEO_QUIRK_SWAPPED_MASK)
		pck.ff.enable = SWAP_BITS(SWAP_BITS(pck.ff.enable, 0, 2), 1, 3);

	xpadneo_output_report(xdata->hdev, (u8 *)&pck, sizeof(pck));
	mdelay(300);

	/*
	 * XPADNEO_QUIRK_NO_PULSE:
	 * The controller doesn't support timing parameters of the rumble command, so we manually stop the motors.
	 */
	if (xdata->quirks & XPADNEO_QUIRK_NO_PULSE) {
		if (enabled & FF_RUMBLE_WEAK)
			pck.ff.magnitude_weak = 0;
		if (enabled & FF_RUMBLE_STRONG)
			pck.ff.magnitude_strong = 0;
		if (enabled & FF_RUMBLE_RIGHT)
			pck.ff.magnitude_right = 0;
		if (enabled & FF_RUMBLE_LEFT)
			pck.ff.magnitude_left = 0;
		xpadneo_output_report(xdata->hdev, (u8 *)&pck, sizeof(pck));
	}
	mdelay(30);
}

static void xpadneo_welcome_rumble(struct hid_device *hdev)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct ff_report ff_pck = { };

	ff_pck.report_id = XPADNEO_XB1S_FF_REPORT;

	/*
	 * Initialize the motor magnitudes here, the test command will individually zero masked magnitudes if
	 * needed.
	 */
	ff_pck.ff.magnitude_weak = 40;
	ff_pck.ff.magnitude_strong = 20;
	ff_pck.ff.magnitude_right = 10;
	ff_pck.ff.magnitude_left = 10;

	/*
	 * XPADNEO_QUIRK_NO_PULSE:
	 * If the controller doesn't support timing parameters of the rumble command, don't set them. Otherwise we
	 * may miss controllers that actually do support the parameters.
	 */
	if (!(xdata->quirks & XPADNEO_QUIRK_NO_PULSE)) {
		ff_pck.ff.pulse_sustain_10ms = 5;
		ff_pck.ff.pulse_release_10ms = 5;
		ff_pck.ff.loop_count = 2;
	}

	ff_pck.ff.enable = FF_RUMBLE_WEAK;
	xpadneo_test_rumble("weak motor", xdata, ff_pck);

	ff_pck.ff.enable = FF_RUMBLE_STRONG;
	xpadneo_test_rumble("strong motor", xdata, ff_pck);

	if (!(xdata->quirks & XPADNEO_QUIRK_NO_TRIGGER_RUMBLE)) {
		ff_pck.ff.enable = FF_RUMBLE_TRIGGERS;
		xpadneo_test_rumble("trigger motors", xdata, ff_pck);
	}
}

static int xpadneo_init_ff(struct hid_device *hdev)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct input_dev *gamepad = xdata->gamepad;

	INIT_DELAYED_WORK(&xdata->ff_worker, xpadneo_ff_worker);
	xdata->output_report_dmabuf = devm_kzalloc(&hdev->dev,
						   sizeof(struct ff_report), GFP_KERNEL);
	if (xdata->output_report_dmabuf == NULL)
		return -ENOMEM;

	if (param_trigger_rumble_mode == PARAM_TRIGGER_RUMBLE_DISABLE)
		xdata->quirks |= XPADNEO_QUIRK_NO_TRIGGER_RUMBLE;

	if (param_ff_connect_notify) {
		xpadneo_benchmark_start(xpadneo_welcome_rumble);
		xpadneo_welcome_rumble(hdev);
		xpadneo_benchmark_stop(xpadneo_welcome_rumble);
	}

	/* initialize our rumble command throttle */
	xdata->ff_throttle_until = XPADNEO_RUMBLE_THROTTLE_JIFFIES;

	input_set_capability(gamepad, EV_FF, FF_RUMBLE);
	return input_ff_create_memless(gamepad, NULL, xpadneo_ff_play);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,12,0)
static u8 *xpadneo_report_fixup(struct hid_device *hdev, u8 *rdesc, unsigned int *rsize)
#else
static const __u8 *xpadneo_report_fixup(struct hid_device *hdev, __u8 *rdesc, unsigned int *rsize)
#endif
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	xdata->original_rsize = *rsize;
	hid_info(hdev, "report descriptor size: %d bytes\n", *rsize);

	/* fixup trailing NUL byte */
	if (rdesc[*rsize - 2] == 0xC0 && rdesc[*rsize - 1] == 0x00) {
		hid_notice(hdev, "fixing up report descriptor size\n");
		*rsize -= 1;
	}

	/* fixup reported axes for Xbox One S */
	if (*rsize >= 81) {
		if (rdesc[34] == 0x09 && rdesc[35] == 0x32) {
			hid_notice(hdev, "fixing up Rx axis\n");
			rdesc[35] = 0x33;	/* Z --> Rx */
		}
		if (rdesc[36] == 0x09 && rdesc[37] == 0x35) {
			hid_notice(hdev, "fixing up Ry axis\n");
			rdesc[37] = 0x34;	/* Rz --> Ry */
		}
		if (rdesc[52] == 0x05 && rdesc[53] == 0x02 &&
		    rdesc[54] == 0x09 && rdesc[55] == 0xC5) {
			hid_notice(hdev, "fixing up Z axis\n");
			rdesc[53] = 0x01;	/* Simulation -> Gendesk */
			rdesc[55] = 0x32;	/* Brake -> Z */
		}
		if (rdesc[77] == 0x05 && rdesc[78] == 0x02 &&
		    rdesc[79] == 0x09 && rdesc[80] == 0xC4) {
			hid_notice(hdev, "fixing up Rz axis\n");
			rdesc[78] = 0x01;	/* Simulation -> Gendesk */
			rdesc[80] = 0x35;	/* Accelerator -> Rz */
		}
	}

	/* fixup reported button count for Xbox controllers in Linux mode */
	if (*rsize >= 164) {
		/*
		 * 12 buttons instead of 10: properly remap the
		 * Xbox button (button 11)
		 * Share button (button 12)
		 */
		if (rdesc[140] == 0x05 && rdesc[141] == 0x09 &&
		    rdesc[144] == 0x29 && rdesc[145] == 0x0F &&
		    rdesc[152] == 0x95 && rdesc[153] == 0x0F &&
		    rdesc[162] == 0x95 && rdesc[163] == 0x01) {
			hid_notice(hdev, "fixing up button mapping\n");
			xdata->quirks |= XPADNEO_QUIRK_LINUX_BUTTONS;
			rdesc[145] = 0x0C;	/* 15 buttons -> 12 buttons */
			rdesc[153] = 0x0C;	/* 15 bits -> 12 bits buttons */
			rdesc[163] = 0x04;	/* 1 bit -> 4 bits constants */
		}
	}

	return rdesc;
}

static int xpadneo_init_hw(struct hid_device *hdev)
{
	int ret;
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	if (!xdata->gamepad) {
		xpadneo_core_missing(xdata, XPADNEO_MISSING_GAMEPAD);
		return -EINVAL;
	}

	ret = xpadneo_power_init(xdata);
	if (ret)
		goto err_free_name;

	ret = xpadneo_quirks_init(xdata);
	if (ret)
		goto err_free_name;

	return 0;

err_free_name:
	xpadneo_power_remove(xdata);
	return ret;
}

static int xpadneo_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	int ret, index;
	struct xpadneo_devdata *xdata;

	xdata = devm_kzalloc(&hdev->dev, sizeof(*xdata), GFP_KERNEL);
	if (xdata == NULL)
		return -ENOMEM;

	index = ida_alloc(&xpadneo_device_id_allocator, GFP_KERNEL);
	if (index < 0)
		return index;
	xdata->id = index;

	xdata->quirks = id->driver_data;

	xdata->hdev = hdev;
	hdev->quirks |= HID_QUIRK_INPUT_PER_APP;
	hdev->quirks |= HID_QUIRK_NO_INPUT_SYNC;
	hid_set_drvdata(hdev, xdata);

	if (hdev->version == 0x00000903)
		hid_warn(hdev, "buggy firmware detected, please upgrade to the latest version\n");
	else if (hdev->version < 0x00000500)
		hid_warn(hdev,
			 "classic Bluetooth firmware version %x.%02x, please upgrade for better stability\n",
			 hdev->version >> 8, (u8)hdev->version);
	else if (hdev->version < 0x00000512)
		hid_warn(hdev,
			 "BLE firmware version %x.%02x, please upgrade for better stability\n",
			 hdev->version >> 8, (u8)hdev->version);
	else
		hid_info(hdev, "BLE firmware version %x.%02x\n",
			 hdev->version >> 8, (u8)hdev->version);

	/*
	 * Pretend that we are in Windows pairing mode as we are actually
	 * exposing the Windows mapping. This prevents SDL and other layers
	 * (probably browser game controller APIs) from treating our driver
	 * unnecessarily with button and axis mapping fixups, and it seems
	 * this is actually a firmware mode meant for Android usage only:
	 *
	 * Xbox One S:
	 * 0x2E0 wireless Windows mode (non-Android mode)
	 * 0x2EA USB Windows and Linux mode
	 * 0x2FD wireless Linux mode (Android mode)
	 *
	 * Xbox Elite 2:
	 * 0xB00 USB Windows and Linux mode
	 * 0xB05 wireless Linux mode (Android mode)
	 *
	 * Xbox Series X|S:
	 * 0xB12 Dongle, USB Windows and USB Linux mode
	 * 0xB13 wireless Linux mode (Android mode)
	 *
	 * Xbox Controller BLE mode:
	 * 0xB20 wireless BLE mode
	 *
	 * TODO: We should find a better way of doing this so SDL2 could
	 * still detect our driver as the correct model. Currently this
	 * maps all controllers to the same model.
	 */
	xdata->original_product = hdev->product;
	xdata->original_version = hdev->version;
	hdev->product = 0x028E;
	hdev->version = 0x00001130;

	if (hdev->product != xdata->original_product)
		hid_info(hdev,
			 "pretending XB1S Windows wireless mode "
			 "(changed PID from 0x%04X to 0x%04X)\n", xdata->original_product,
			 hdev->product);

	if (hdev->version != xdata->original_version)
		hid_info(hdev,
			 "working around wrong SDL2 mappings "
			 "(changed version from 0x%08X to 0x%08X)\n", xdata->original_version,
			 hdev->version);

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "parse failed\n");
		return ret;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret) {
		hid_err(hdev, "hw start failed\n");
		return ret;
	}

	ret = xpadneo_init_consumer(xdata);
	if (ret)
		return ret;

	ret = xpadneo_init_keyboard(xdata);
	if (ret)
		return ret;

	ret = xpadneo_mouse_init(xdata);
	if (ret)
		return ret;

	ret = xpadneo_init_hw(hdev);
	if (ret) {
		hid_err(hdev, "hw init failed: %d\n", ret);
		hid_hw_stop(hdev);
		return ret;
	}

	ret = xpadneo_init_ff(hdev);
	if (ret)
		hid_err(hdev, "could not initialize ff, continuing anyway\n");

	xpadneo_mouse_init_timer(xdata);

	hid_info(hdev, "%s connected\n", xdata->battery.name);

	return 0;
}

static void xpadneo_release_device_id(struct xpadneo_devdata *xdata)
{
	if (xdata->id >= 0) {
		ida_free(&xpadneo_device_id_allocator, xdata->id);
		xdata->id = -1;
	}
}

static void xpadneo_remove(struct hid_device *hdev)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	hid_hw_close(hdev);

	if (hdev->version != xdata->original_version) {
		hid_info(hdev,
			 "reverting to original version "
			 "(changed version from 0x%08X to 0x%08X)\n",
			 hdev->version, xdata->original_version);
		hdev->version = xdata->original_version;
	}

	if (hdev->product != xdata->original_product) {
		hid_info(hdev,
			 "reverting to original product "
			 "(changed PID from 0x%04X to 0x%04X)\n",
			 hdev->product, xdata->original_product);
		hdev->product = xdata->original_product;
	}

	xpadneo_mouse_remove_timer(xdata);

	cancel_delayed_work_sync(&xdata->ff_worker);

	xpadneo_power_remove(xdata);

	xpadneo_release_device_id(xdata);
	hid_hw_stop(hdev);
}

static const struct hid_device_id xpadneo_devices[] = {
	/* XBOX ONE S / X */
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x02E0) },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x02FD) },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x0B20),
	 .driver_data = XPADNEO_QUIRK_SHARE_BUTTON },

	/* XBOX ONE Elite Series 2 */
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x0B05) },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x0B22),
	 .driver_data = XPADNEO_QUIRK_SHARE_BUTTON },

	/* XBOX Series X|S / Xbox Wireless Controller (BLE) */
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x0B13),
	 .driver_data = XPADNEO_QUIRK_SHARE_BUTTON },

	/* SENTINEL VALUE, indicates the end */
	{ }
};

MODULE_DEVICE_TABLE(hid, xpadneo_devices);

static struct hid_driver xpadneo_driver = {
	.name = "xpadneo",
	.id_table = xpadneo_devices,
	.input_mapping = xpadneo_mapping_input,
	.input_configured = xpadneo_events_input_configured,
	.probe = xpadneo_probe,
	.remove = xpadneo_remove,
	.report = xpadneo_report,
	.report_fixup = xpadneo_report_fixup,
	.raw_event = xpadneo_events_raw_event,
	.event = xpadneo_events_event,
};

static int __init xpadneo_init(void)
{
	pr_info("loaded hid-xpadneo %s\n", XPADNEO_VERSION);
	dbg_hid("xpadneo:%s\n", __func__);

	if (param_trigger_rumble_mode == 1)
		pr_warn("hid-xpadneo trigger_rumble_mode=1 is unknown, defaulting to 0\n");

	xpadneo_rumble_wq = alloc_ordered_workqueue("xpadneo/rumbled", WQ_HIGHPRI);
	if (xpadneo_rumble_wq) {
		int ret = hid_register_driver(&xpadneo_driver);
		if (ret != 0) {
			destroy_workqueue(xpadneo_rumble_wq);
			xpadneo_rumble_wq = NULL;
		}
		return ret;
	}
	return -EINVAL;
}

static void __exit xpadneo_exit(void)
{
	dbg_hid("xpadneo:%s\n", __func__);
	hid_unregister_driver(&xpadneo_driver);
	ida_destroy(&xpadneo_device_id_allocator);
	destroy_workqueue(xpadneo_rumble_wq);
}

module_init(xpadneo_init);
module_exit(xpadneo_exit);
