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

static bool param_gamepad_compliance = 1;
module_param_named(gamepad_compliance, param_gamepad_compliance, bool, 0444);
MODULE_PARM_DESC(gamepad_compliance,
		 "(bool) Adhere to Linux Gamepad Specification by using signed axis values. "
		 "1: enable, 0: disable.");

static bool param_disable_deadzones = 0;
module_param_named(disable_deadzones, param_disable_deadzones, bool, 0444);
MODULE_PARM_DESC(disable_deadzones,
		 "(bool) Disable dead zone handling for raw processing by Wine/Proton, confuses joydev. "
		 "0: disable, 1: enable.");

static bool param_enable_rolling_axis = 0;
module_param_named(enable_rolling_axis, param_enable_rolling_axis, bool, 0444);
MODULE_PARM_DESC(enable_rolling_axis,
		 "(bool) Enable rolling axis by combining both triggers, out of spec for many games. (deprecated) "
		 "0: disable, 1: enable.");

static bool param_disable_shift_mode = 0;
module_param_named(disable_shift_mode, param_disable_shift_mode, bool, 0644);
MODULE_PARM_DESC(disable_shift_mode,
		 "(bool) Disable use Xbox logo button as shift. Will prohibit profile switching when enabled. "
		 "0: disable, 1: enable.");

static bool param_debug_hid = 0;
module_param_named(debug_hid, param_debug_hid, bool, 0644);
MODULE_PARM_DESC(debug_hid, "(u8) Debug HID reports. 0: disable, 1: enable.");

static struct {
	char *args[17];
	unsigned int nargs;
} param_quirks;
module_param_array_named(quirks, param_quirks.args, charp, &param_quirks.nargs, 0644);
MODULE_PARM_DESC(quirks,
		 "(string) Override or change device quirks, specify as: \"MAC1{:,+,-}quirks1[,...16]\""
		 ", MAC format = 11:22:33:44:55:66"
		 ", no pulse parameters = " __stringify(XPADNEO_QUIRK_NO_PULSE)
		 ", no trigger rumble = " __stringify(XPADNEO_QUIRK_NO_TRIGGER_RUMBLE)
		 ", no motor masking = " __stringify(XPADNEO_QUIRK_NO_MOTOR_MASK)
		 ", use Linux button mappings = " __stringify(XPADNEO_QUIRK_LINUX_BUTTONS)
		 ", use Nintendo mappings = " __stringify(XPADNEO_QUIRK_NINTENDO)
		 ", use Share button mappings = " __stringify(XPADNEO_QUIRK_SHARE_BUTTON)
		 ", reversed motor masking = " __stringify(XPADNEO_QUIRK_REVERSE_MASK)
		 ", swapped motor masking = " __stringify(XPADNEO_QUIRK_SWAPPED_MASK)
		 ", apply no heuristics = " __stringify(XPADNEO_QUIRK_NO_HEURISTICS));

static DEFINE_IDA(xpadneo_device_id_allocator);

static enum power_supply_property xpadneo_battery_props[] = {
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_SCOPE,
	POWER_SUPPLY_PROP_STATUS,
};

#define DEVICE_NAME_QUIRK(n, f) \
	{ .name_match = (n), .name_len = sizeof(n) - 1, .flags = (f) }
#define DEVICE_OUI_QUIRK(o, f) \
	{ .oui_match = (o), .flags = (f) }

struct quirk {
	char *name_match;
	char *oui_match;
	u16 name_len;
	u32 flags;
};

static const struct quirk xpadneo_quirks[] = {
	DEVICE_OUI_QUIRK("28:EA:0B", XPADNEO_QUIRK_NO_HEURISTICS),
	DEVICE_OUI_QUIRK("3C:FA:06", XPADNEO_QUIRK_NO_HEURISTICS),
	DEVICE_OUI_QUIRK("68:6C:E6", XPADNEO_QUIRK_NO_HEURISTICS),
	DEVICE_OUI_QUIRK("98:B6:EA",
			 XPADNEO_QUIRK_NO_PULSE | XPADNEO_QUIRK_NO_TRIGGER_RUMBLE |
			 XPADNEO_QUIRK_REVERSE_MASK),
	DEVICE_OUI_QUIRK("98:B6:EC",
			 XPADNEO_QUIRK_SIMPLE_CLONE | XPADNEO_QUIRK_SWAPPED_MASK),
	DEVICE_OUI_QUIRK("A0:5A:5D", XPADNEO_QUIRK_NO_HAPTICS),
	DEVICE_OUI_QUIRK("A8:8C:3E", XPADNEO_QUIRK_NO_HEURISTICS),
	DEVICE_OUI_QUIRK("AC:8E:BD", XPADNEO_QUIRK_NO_HEURISTICS),
	DEVICE_OUI_QUIRK("E4:17:D8", XPADNEO_QUIRK_SIMPLE_CLONE),
	DEVICE_OUI_QUIRK("EC:83:50", XPADNEO_QUIRK_NO_HEURISTICS),
};

struct usage_map {
	u32 usage;
	enum {
		MAP_IGNORE = -1,	/* Completely ignore this field */
		MAP_AUTO,	/* Do not really map it, let hid-core decide */
		MAP_STATIC	/* Map to the values given */
	} behaviour;
	struct {
		u8 event_type;	/* input event (EV_KEY, EV_ABS, ...) */
		u16 input_code;	/* input code (BTN_A, ABS_X, ...) */
	} ev;
};

#define USAGE_MAP(u, b, e, i) \
	{ .usage = (u), .behaviour = (b), .ev = { .event_type = (e), .input_code = (i) } }
#define USAGE_IGN(u) USAGE_MAP(u, MAP_IGNORE, 0, 0)

static const struct usage_map xpadneo_usage_maps[] = {
	/* fixup buttons to Linux codes */
	USAGE_MAP(0x90001, MAP_STATIC, EV_KEY, BTN_A),	/* A */
	USAGE_MAP(0x90002, MAP_STATIC, EV_KEY, BTN_B),	/* B */
	USAGE_MAP(0x90003, MAP_STATIC, EV_KEY, BTN_X),	/* X */
	USAGE_MAP(0x90004, MAP_STATIC, EV_KEY, BTN_Y),	/* Y */
	USAGE_MAP(0x90005, MAP_STATIC, EV_KEY, BTN_TL),	/* LB */
	USAGE_MAP(0x90006, MAP_STATIC, EV_KEY, BTN_TR),	/* RB */
	USAGE_MAP(0x90007, MAP_STATIC, EV_KEY, BTN_SELECT),	/* Back */
	USAGE_MAP(0x90008, MAP_STATIC, EV_KEY, BTN_START),	/* Menu */
	USAGE_MAP(0x90009, MAP_STATIC, EV_KEY, BTN_THUMBL),	/* LS */
	USAGE_MAP(0x9000A, MAP_STATIC, EV_KEY, BTN_THUMBR),	/* RS */

	/* fixup the Xbox logo button */
	USAGE_MAP(0x9000B, MAP_STATIC, EV_KEY, BTN_XBOX),	/* Xbox */

	/* fixup the Share button */
	USAGE_MAP(0x9000C, MAP_STATIC, EV_KEY, BTN_SHARE),	/* Share */

	/* fixup code "Sys Main Menu" from Windows report descriptor */
	USAGE_MAP(0x10085, MAP_STATIC, EV_KEY, BTN_XBOX),

	/* fixup code "AC Home" from Linux report descriptor */
	USAGE_MAP(0xC0223, MAP_STATIC, EV_KEY, BTN_XBOX),

	/* fixup code "AC Back" from Linux report descriptor */
	USAGE_MAP(0xC0224, MAP_STATIC, EV_KEY, BTN_SELECT),

	/* map special buttons without HID bitmaps, corrected in event handler */
	USAGE_MAP(0xC0081, MAP_STATIC, EV_KEY, BTN_PADDLES(0)),	/* Four paddles */

	/* hardware features handled at the raw report level */
	USAGE_IGN(0xC0085),	/* Profile switcher */
	USAGE_IGN(0xC0099),	/* Trigger scale switches */

	/* XBE2: Disable "dial", which is a redundant representation of the D-Pad */
	USAGE_IGN(0x10037),

	/* XBE2: Disable duplicate report fields of broken v1 packet format */
	USAGE_IGN(0x10040),	/* Vx, copy of X axis */
	USAGE_IGN(0x10041),	/* Vy, copy of Y axis */
	USAGE_IGN(0x10042),	/* Vz, copy of Z axis */
	USAGE_IGN(0x10043),	/* Vbrx, copy of Rx */
	USAGE_IGN(0x10044),	/* Vbry, copy of Ry */
	USAGE_IGN(0x10045),	/* Vbrz, copy of Rz */
	USAGE_IGN(0x90010),	/* copy of A */
	USAGE_IGN(0x90011),	/* copy of B */
	USAGE_IGN(0x90013),	/* copy of X */
	USAGE_IGN(0x90014),	/* copy of Y */
	USAGE_IGN(0x90016),	/* copy of LB */
	USAGE_IGN(0x90017),	/* copy of RB */
	USAGE_IGN(0x9001B),	/* copy of Start */
	USAGE_IGN(0x9001D),	/* copy of LS */
	USAGE_IGN(0x9001E),	/* copy of RS */
	USAGE_IGN(0xC0082),	/* copy of Select button */

	/* XBE2: Disable unused buttons */
	USAGE_IGN(0x90012),	/* 6 "TRIGGER_HAPPY" buttons */
	USAGE_IGN(0x90015),
	USAGE_IGN(0x90018),
	USAGE_IGN(0x90019),
	USAGE_IGN(0x9001A),
	USAGE_IGN(0x9001C),
	USAGE_IGN(0xC00B9),	/* KEY_SHUFFLE button */
};

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

#define XPADNEO_PSY_ONLINE(data)     ((data&0x80)>0)
#define XPADNEO_PSY_MODE(data)       ((data&0x0C)>>2)

#define XPADNEO_POWER_USB(data)      (XPADNEO_PSY_MODE(data)==0)
#define XPADNEO_POWER_BATTERY(data)  (XPADNEO_PSY_MODE(data)==1)
#define XPADNEO_POWER_PNC(data)      (XPADNEO_PSY_MODE(data)==2)

#define XPADNEO_BATTERY_ONLINE(data)         ((data&0x0C)>0)
#define XPADNEO_BATTERY_CHARGING(data)       ((data&0x10)>0)
#define XPADNEO_BATTERY_CAPACITY_LEVEL(data) (data&0x03)

static int xpadneo_get_psy_property(struct power_supply *psy,
				    enum power_supply_property property,
				    union power_supply_propval *val)
{
	struct xpadneo_devdata *xdata = power_supply_get_drvdata(psy);
	int ret = 0;

	static int capacity_level_map[] = {
		[0] = POWER_SUPPLY_CAPACITY_LEVEL_LOW,
		[1] = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL,
		[2] = POWER_SUPPLY_CAPACITY_LEVEL_HIGH,
		[3] = POWER_SUPPLY_CAPACITY_LEVEL_FULL,
	};

	u8 flags = xdata->battery.flags;
	u8 level = min(3, XPADNEO_BATTERY_CAPACITY_LEVEL(flags));
	bool online = XPADNEO_PSY_ONLINE(flags);
	bool charging = XPADNEO_BATTERY_CHARGING(flags);

	switch (property) {
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		if (online && XPADNEO_BATTERY_ONLINE(flags))
			val->intval = capacity_level_map[level];
		else
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN;
		break;

	case POWER_SUPPLY_PROP_MODEL_NAME:
		if (online && XPADNEO_POWER_PNC(flags))
			val->strval = xdata->battery.name_pnc;
		else
			val->strval = xdata->battery.name;
		break;

	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = online && XPADNEO_BATTERY_ONLINE(flags);
		break;

	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = online && (XPADNEO_BATTERY_ONLINE(flags) || charging);
		break;

	case POWER_SUPPLY_PROP_SCOPE:
		val->intval = POWER_SUPPLY_SCOPE_DEVICE;
		break;

	case POWER_SUPPLY_PROP_STATUS:
		if (online && charging)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else if (online && !charging && XPADNEO_POWER_USB(flags))
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else if (online && XPADNEO_BATTERY_ONLINE(flags))
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int xpadneo_setup_psy(struct hid_device *hdev)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct power_supply_config ps_config = {
		.drv_data = xdata
	};
	int ret;

	/* already registered */
	if (xdata->battery.psy)
		return 0;

	xdata->battery.desc.name = kasprintf(GFP_KERNEL, "xpadneo_battery_%i", xdata->id);
	if (!xdata->battery.desc.name)
		return -ENOMEM;

	xdata->battery.desc.properties = xpadneo_battery_props;
	xdata->battery.desc.num_properties = ARRAY_SIZE(xpadneo_battery_props);
	xdata->battery.desc.use_for_apm = 0;
	xdata->battery.desc.get_property = xpadneo_get_psy_property;
	xdata->battery.desc.type = POWER_SUPPLY_TYPE_BATTERY;

	/* register battery via device manager */
	xdata->battery.psy =
	    devm_power_supply_register(&hdev->dev, &xdata->battery.desc, &ps_config);
	if (IS_ERR(xdata->battery.psy)) {
		ret = PTR_ERR(xdata->battery.psy);
		hid_err(hdev, "battery registration failed\n");
		goto err_free_name;
	} else {
		hid_info(hdev, "battery registered\n");
	}

	power_supply_powers(xdata->battery.psy, &xdata->hdev->dev);

	return 0;

err_free_name:
	kfree(xdata->battery.desc.name);
	xdata->battery.desc.name = NULL;

	return ret;
}

static void xpadneo_update_psy(struct xpadneo_devdata *xdata, u8 value)
{
	int old_value = xdata->battery.flags;

	if (!xdata->battery.initialized && XPADNEO_PSY_ONLINE(value)) {
		xdata->battery.initialized = true;
		xpadneo_setup_psy(xdata->hdev);
	}

	if (!xdata->battery.psy)
		return;

	xdata->battery.flags = value;
	if (old_value != value) {
		if (!XPADNEO_PSY_ONLINE(value))
			hid_info(xdata->hdev, "shutting down\n");
		power_supply_changed(xdata->battery.psy);
	}
}

#define xpadneo_map_usage_clear(ev) hid_map_usage_clear(hi, usage, bit, max, (ev).event_type, (ev).input_code)
static int xpadneo_input_mapping(struct hid_device *hdev, struct hid_input *hi,
				 struct hid_field *field,
				 struct hid_usage *usage, unsigned long **bit, int *max)
{
	int i = 0;

	if (usage->hid == HID_DC_BATTERYSTRENGTH) {
		struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

		xdata->battery.report_id = field->report->id;
		hid_info(hdev, "battery detected\n");

		return MAP_IGNORE;
	}

	for (i = 0; i < ARRAY_SIZE(xpadneo_usage_maps); i++) {
		const struct usage_map *entry = &xpadneo_usage_maps[i];

		if (entry->usage == usage->hid) {
			if (entry->behaviour == MAP_STATIC)
				xpadneo_map_usage_clear(entry->ev);
			return entry->behaviour;
		}
	}

	/* let HID handle this */
	return MAP_AUTO;
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

static void xpadneo_toggle_mouse(struct xpadneo_devdata *xdata)
{
	if (xdata->mouse_mode) {
		xdata->mouse_mode = false;
		hid_info(xdata->hdev, "mouse mode disabled\n");
	} else {
		xdata->mouse_mode = true;
		hid_info(xdata->hdev, "mouse mode enabled\n");
	}

	/* Indicate that a request was made */
	xdata->profile_switched = true;
}

static void xpadneo_switch_profile(struct xpadneo_devdata *xdata, const u8 profile,
				   const bool emulated)
{
	if (xdata->profile != profile) {
		hid_info(xdata->hdev, "switching profile to %d\n", profile);
		xdata->profile = profile;
	}

	/* Indicate to profile emulation that a request was made */
	xdata->profile_switched = emulated;
}

static void xpadneo_switch_triggers(struct xpadneo_devdata *xdata, const u8 mode)
{
	char *name[XBOX_TRIGGER_SCALE_NUM] = {
		[XBOX_TRIGGER_SCALE_FULL] = "full range",
		[XBOX_TRIGGER_SCALE_HALF] = "half range",
		[XBOX_TRIGGER_SCALE_DIGITAL] = "digital",
	};

	enum xpadneo_trigger_scale left = (mode >> 0) & 0x03;
	enum xpadneo_trigger_scale right = (mode >> 2) & 0x03;

	if ((xdata->trigger_scale.left != left) && (left < XBOX_TRIGGER_SCALE_NUM)) {
		hid_info(xdata->hdev, "switching left trigger to %s mode\n", name[left]);
		xdata->trigger_scale.left = left;
	}

	if ((xdata->trigger_scale.right != right) && (right < XBOX_TRIGGER_SCALE_NUM)) {
		hid_info(xdata->hdev, "switching right trigger to %s mode\n", name[right]);
		xdata->trigger_scale.right = right;
	}
}

static int xpadneo_raw_event(struct hid_device *hdev, struct hid_report *report,
			     u8 *data, int reportsize)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	/* the controller spams reports multiple times */
	if (likely(report->id == 0x01)) {
		int size = min(reportsize, (int)XPADNEO_REPORT_0x01_LENGTH);
		if (likely(memcmp(&xdata->input_report_0x01, data, size) == 0))
			return -1;
		memcpy(&xdata->input_report_0x01, data, size);
	}

	/* reset the count at the beginning of the frame */
	xdata->count_abs_z_rz = 0;

	/* we are taking care of the battery report ourselves */
	if (xdata->battery.report_id && report->id == xdata->battery.report_id && reportsize == 2) {
		xpadneo_update_psy(xdata, data[1]);
		return -1;
	}

	/* correct button mapping of Xbox controllers in Linux mode */
	if ((xdata->quirks & XPADNEO_QUIRK_LINUX_BUTTONS) && report->id == 1 && reportsize >= 17) {
		u16 bits = 0;

		bits |= (data[14] & (BIT(0) | BIT(1))) >> 0;	/* A, B */
		bits |= (data[14] & (BIT(3) | BIT(4))) >> 1;	/* X, Y */
		bits |= (data[14] & (BIT(6) | BIT(7))) >> 2;	/* LB, RB */
		if (xdata->quirks & XPADNEO_QUIRK_SHARE_BUTTON)
			bits |= (data[15] & BIT(2)) << 4;	/* Back */
		else
			bits |= (data[16] & BIT(0)) << 6;	/* Back */
		bits |= (data[15] & BIT(3)) << 4;	/* Menu */
		bits |= (data[15] & BIT(5)) << 3;	/* LS */
		bits |= (data[15] & BIT(6)) << 3;	/* RS */
		bits |= (data[15] & BIT(4)) << 6;	/* Xbox */
		if (xdata->quirks & XPADNEO_QUIRK_SHARE_BUTTON)
			bits |= (data[16] & BIT(0)) << 11;	/* Share */
		data[14] = (u8)((bits >> 0) & 0xFF);
		data[15] = (u8)((bits >> 8) & 0xFF);
		data[16] = 0;
	}

	/* swap button A with B and X with Y for Nintendo style controllers */
	if ((xdata->quirks & XPADNEO_QUIRK_NINTENDO) && report->id == 1 && reportsize >= 15) {
		data[14] = SWAP_BITS(data[14], 0, 1);
		data[14] = SWAP_BITS(data[14], 2, 3);
	}

	/* XBE2: track the current controller settings */
	if (report->id == 1 && reportsize >= 20) {
		if (!(xdata->quirks & XPADNEO_QUIRK_USE_HW_PROFILES)) {
			hid_info(hdev, "mapping profiles detected\n");
			xdata->quirks |= XPADNEO_QUIRK_USE_HW_PROFILES;
		}
		if (reportsize == 55) {
			hid_notice_once(hdev,
					"detected broken XBE2 v1 packet format, please update the firmware");
			xpadneo_switch_profile(xdata, data[35] & 0x03, false);
			xpadneo_switch_triggers(xdata, data[36] & 0x0F);
		} else if (reportsize >= 21) {
			/* firmware 4.x style packet */
			xpadneo_switch_profile(xdata, data[19] & 0x03, false);
			xpadneo_switch_triggers(xdata, data[20] & 0x0F);
		} else {
			/* firmware 5.x style packet */
			xpadneo_switch_profile(xdata, data[17] & 0x03, false);
			xpadneo_switch_triggers(xdata, data[18] & 0x0F);
		}
	}

	return 0;
}

static int xpadneo_input_configured(struct hid_device *hdev, struct hid_input *hi)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	int deadzone = 3072, abs_min = 0, abs_max = 65535;

	switch (hi->application) {
	case HID_GD_GAMEPAD:
		hid_info(hdev, "gamepad detected\n");
		xdata->gamepad = hi->input;
		break;
	case HID_GD_KEYBOARD:
		hid_info(hdev, "keyboard detected\n");
		xdata->keyboard = hi->input;

		/* do not report bogus keys as part of the keyboard */
		__clear_bit(KEY_UNKNOWN, xdata->keyboard->keybit);

		return 0;
	case HID_CP_CONSUMER_CONTROL:
		hid_info(hdev, "consumer control detected\n");
		xdata->consumer = hi->input;
		return 0;
	case 0xFF000005:
		/* FIXME: this is no longer in the current firmware */
		hid_info(hdev, "mapping profiles detected\n");
		xdata->quirks |= XPADNEO_QUIRK_USE_HW_PROFILES;
		return 0;
	default:
		hid_warn(hdev, "unhandled input application 0x%x\n", hi->application);
	}

	if (param_disable_deadzones) {
		hid_warn(hdev, "disabling dead zones\n");
		deadzone = 0;
	}

	if (param_gamepad_compliance) {
		hid_info(hdev, "enabling compliance with Linux Gamepad Specification\n");
		abs_min = -32768;
		abs_max = 32767;
	}

	input_set_abs_params(xdata->gamepad, ABS_X, abs_min, abs_max, 32, deadzone);
	input_set_abs_params(xdata->gamepad, ABS_Y, abs_min, abs_max, 32, deadzone);
	input_set_abs_params(xdata->gamepad, ABS_RX, abs_min, abs_max, 32, deadzone);
	input_set_abs_params(xdata->gamepad, ABS_RY, abs_min, abs_max, 32, deadzone);

	input_set_abs_params(xdata->gamepad, ABS_Z, 0, 1023, 4, 0);
	input_set_abs_params(xdata->gamepad, ABS_RZ, 0, 1023, 4, 0);

	/* combine triggers to form a rudder, use ABS_MISC to order after dpad */
	if (param_enable_rolling_axis) {
		hid_info(hdev, "enabling rolling axis is deprecated\n");
		input_set_abs_params(xdata->gamepad, ABS_MISC, -1023, 1023, 3, 63);
	}

	/* do not report the keyboard buttons as part of the gamepad */
	__clear_bit(BTN_SHARE, xdata->gamepad->keybit);
	__clear_bit(KEY_RECORD, xdata->gamepad->keybit);
	__clear_bit(KEY_UNKNOWN, xdata->gamepad->keybit);

	/* ensure all four paddles exist as part of the gamepad */
	if (test_bit(BTN_PADDLES(0), xdata->gamepad->keybit)) {
		__set_bit(BTN_PADDLES(1), xdata->gamepad->keybit);
		__set_bit(BTN_PADDLES(2), xdata->gamepad->keybit);
		__set_bit(BTN_PADDLES(3), xdata->gamepad->keybit);
	}

	return 0;
}

static int xpadneo_event(struct hid_device *hdev, struct hid_field *field,
			 struct hid_usage *usage, __s32 value)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct input_dev *gamepad = xdata->gamepad;
	struct input_dev *keyboard = xdata->keyboard;

	if ((usage->type == EV_KEY) && (usage->code == BTN_PADDLES(0))) {
		if (gamepad && xdata->profile == 0) {
			/* report the paddles individually */
			input_report_key(gamepad, BTN_PADDLES(0), (value & 1) ? 1 : 0);
			input_report_key(gamepad, BTN_PADDLES(1), (value & 2) ? 1 : 0);
			input_report_key(gamepad, BTN_PADDLES(2), (value & 4) ? 1 : 0);
			input_report_key(gamepad, BTN_PADDLES(3), (value & 8) ? 1 : 0);
			xdata->gamepad_sync = true;
		}
		goto stop_processing;
	} else if (usage->type == EV_ABS) {
		switch (usage->code) {
		case ABS_X:
		case ABS_Y:
		case ABS_RX:
		case ABS_RY:
			/* Linux Gamepad Specification */
			if (param_gamepad_compliance) {
				input_report_abs(gamepad, usage->code, value - 32768);
				xdata->gamepad_sync = true;
				goto stop_processing;
			}
			break;
		case ABS_Z:
			xdata->last_abs_z = value;
			goto combine_z_axes;
		case ABS_RZ:
			xdata->last_abs_rz = value;
			goto combine_z_axes;
		}
	} else if (!param_disable_shift_mode && (usage->type == EV_KEY)
		   && (usage->code == BTN_XBOX)) {
		/*
		 * Handle the Xbox logo button: We want to cache the button
		 * down event to allow for profile switching. The button will
		 * act as a shift key and only send the input events when
		 * released without pressing an additional button.
		 */
		if (!xdata->xbox_button_down && (value == 1)) {
			/* cache this event */
			xdata->xbox_button_down = true;
		} else if (xdata->xbox_button_down && (value == 0)) {
			xdata->xbox_button_down = false;
			if (xdata->profile_switched) {
				xdata->profile_switched = false;
			} else {
				/* replay cached event */
				input_report_key(gamepad, BTN_XBOX, 1);
				input_sync(gamepad);
				/* synthesize the release to remove the scan code */
				input_report_key(gamepad, BTN_XBOX, 0);
				input_sync(gamepad);
			}
		}
		goto stop_processing;
	} else if ((usage->type == EV_KEY) && (usage->code == BTN_SHARE)) {
		/* move the Share button to the keyboard device */
		if (!keyboard)
			goto keyboard_missing;
		input_report_key(keyboard, BTN_SHARE, value);
		xdata->keyboard_sync = true;
		goto stop_processing;
	} else if (xdata->xbox_button_down && (usage->type == EV_KEY)) {
		if (!(xdata->quirks & XPADNEO_QUIRK_USE_HW_PROFILES)) {
			switch (usage->code) {
			case BTN_A:
				if (value == 1)
					xpadneo_switch_profile(xdata, 0, true);
				goto stop_processing;
			case BTN_B:
				if (value == 1)
					xpadneo_switch_profile(xdata, 1, true);
				goto stop_processing;
			case BTN_X:
				if (value == 1)
					xpadneo_switch_profile(xdata, 2, true);
				goto stop_processing;
			case BTN_Y:
				if (value == 1)
					xpadneo_switch_profile(xdata, 3, true);
				goto stop_processing;
			case BTN_SELECT:
				if (value == 1)
					xpadneo_toggle_mouse(xdata);
				goto stop_processing;
			}
		}
	}

	/* Let hid-core handle the event */
	xdata->gamepad_sync = true;
	return 0;

combine_z_axes:
	if (++xdata->count_abs_z_rz == 2) {
		xdata->count_abs_z_rz = 0;
		if (param_enable_rolling_axis) {
			input_report_abs(gamepad, ABS_MISC, xdata->last_abs_rz - xdata->last_abs_z);
			xdata->gamepad_sync = true;
		}
	}
	return 0;

keyboard_missing:
	xpadneo_core_missing(xdata, XPADNEO_MISSING_KEYBOARD);

stop_processing:
	return 1;
}

static int xpadneo_init_hw(struct hid_device *hdev)
{
	int i, ret;
	u8 oui_byte;
	u32 quirks_set = 0, quirks_unset = 0, quirks_override = U32_MAX;
	char oui[3] = { };
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	if (!xdata->gamepad) {
		xpadneo_core_missing(xdata, XPADNEO_MISSING_GAMEPAD);
		return -EINVAL;
	}

	xdata->battery.name =
	    kasprintf(GFP_KERNEL, "%s [%s]", xdata->gamepad->name, xdata->gamepad->uniq);
	if (!xdata->battery.name) {
		ret = -ENOMEM;
		goto err_free_name;
	}

	xdata->battery.name_pnc =
	    kasprintf(GFP_KERNEL, "%s [%s] Play'n Charge Kit", xdata->gamepad->name,
		      xdata->gamepad->uniq);
	if (!xdata->battery.name_pnc) {
		ret = -ENOMEM;
		goto err_free_name;
	}

	for (i = 0; i < ARRAY_SIZE(xpadneo_quirks); i++) {
		const struct quirk *q = &xpadneo_quirks[i];

		if (q->name_match
		    && (strncmp(q->name_match, xdata->gamepad->name, q->name_len) == 0))
			xdata->quirks |= q->flags;

		if (q->oui_match && (strncasecmp(q->oui_match, xdata->gamepad->uniq, 8) == 0))
			xdata->quirks |= q->flags;
	}

	kernel_param_lock(THIS_MODULE);
	for (i = 0; i < param_quirks.nargs; i++) {
		int offset = strnlen(xdata->gamepad->uniq, 18);
		if ((strncasecmp(xdata->gamepad->uniq, param_quirks.args[i], offset) == 0)
		    && ((param_quirks.args[i][offset] == ':')
			|| (param_quirks.args[i][offset] == '+')
			|| (param_quirks.args[i][offset] == '-'))) {
			char *quirks_arg = &param_quirks.args[i][offset + 1];
			u32 quirks = 0;
			ret = kstrtou32(quirks_arg, 0, &quirks);
			if (ret) {
				hid_err(hdev, "quirks override invalid: %s\n", quirks_arg);
				goto err_free_name;
			} else if (param_quirks.args[i][offset] == ':') {
				quirks_override = quirks;
			} else if (param_quirks.args[i][offset] == '-') {
				quirks_unset = quirks;
			} else {
				quirks_set = quirks;
			}
			break;
		}
	}
	kernel_param_unlock(THIS_MODULE);

	/* handle quirk flags which override a behavior before heuristics */
	if (quirks_override != U32_MAX) {
		hid_info(hdev, "quirks override: %s\n", xdata->gamepad->uniq);
		xdata->quirks = quirks_override;
	}

	/* handle quirk flags which add a behavior before heuristics */
	if (quirks_set > 0) {
		hid_info(hdev, "quirks added: %s flags 0x%08X\n", xdata->gamepad->uniq, quirks_set);
		xdata->quirks |= quirks_set;
	}

	/*
	 * copy the first two characters from the uniq ID (MAC address) and
	 * expect it being too big to copy, then `kstrtou8()` converts the
	 * uniq ID "aa:bb:cc:dd:ee:ff" to u8, so we get the first OUI byte
	 */
	if ((xdata->original_rsize == 283)
	    && ((xdata->quirks & XPADNEO_QUIRK_NO_HEURISTICS) == 0)
	    && ((xdata->quirks & XPADNEO_QUIRK_SIMPLE_CLONE) == 0)
	    && (strscpy(oui, xdata->gamepad->uniq, sizeof(oui)) == -E2BIG)
	    && (kstrtou8(oui, 16, &oui_byte) == 0)
	    && XPADNEO_OUI_MASK(oui_byte, XPADNEO_OUI_MASK_GAMESIR_NOVA)) {
		hid_info(hdev, "enabling heuristic GameSir Nova quirks\n");
		xdata->quirks |= XPADNEO_QUIRK_SIMPLE_CLONE;
	}

	/* handle quirk flags which remove a behavior after heuristics */
	if (quirks_unset > 0) {
		hid_info(hdev, "quirks removed: %s flag 0x%08X\n", xdata->gamepad->uniq,
			 quirks_unset);
		xdata->quirks &= ~quirks_unset;
	}

	if (xdata->quirks > 0)
		hid_info(hdev, "controller quirks: 0x%08x\n", xdata->quirks);

	return 0;

err_free_name:
	kfree(xdata->battery.name);
	xdata->battery.name = NULL;

	kfree(xdata->battery.name_pnc);
	xdata->battery.name_pnc = NULL;

	return ret;
}

static int xpadneo_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	int ret;
	struct xpadneo_devdata *xdata;

	xdata = devm_kzalloc(&hdev->dev, sizeof(*xdata), GFP_KERNEL);
	if (xdata == NULL)
		return -ENOMEM;

	xdata->id = ida_simple_get(&xpadneo_device_id_allocator, 0, 0, GFP_KERNEL);
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

	ret = xpadneo_init_hw(hdev);
	if (ret) {
		hid_err(hdev, "hw init failed: %d\n", ret);
		hid_hw_stop(hdev);
		return ret;
	}

	ret = xpadneo_init_ff(hdev);
	if (ret)
		hid_err(hdev, "could not initialize ff, continuing anyway\n");

	hid_info(hdev, "%s connected\n", xdata->battery.name);

	return 0;
}

static void xpadneo_release_device_id(struct xpadneo_devdata *xdata)
{
	if (xdata->id >= 0) {
		ida_simple_remove(&xpadneo_device_id_allocator, xdata->id);
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

	cancel_delayed_work_sync(&xdata->ff_worker);

	kfree(xdata->battery.name);
	xdata->battery.name = NULL;

	kfree(xdata->battery.name_pnc);
	xdata->battery.name_pnc = NULL;

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
	.input_mapping = xpadneo_input_mapping,
	.input_configured = xpadneo_input_configured,
	.probe = xpadneo_probe,
	.remove = xpadneo_remove,
	.report = xpadneo_report,
	.report_fixup = xpadneo_report_fixup,
	.raw_event = xpadneo_raw_event,
	.event = xpadneo_event,
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
