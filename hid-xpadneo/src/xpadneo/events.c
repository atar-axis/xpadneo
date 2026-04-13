// SPDX-License-Identifier: GPL-2.0-only

/*
 * xpadneo driver events and profiles handling
 *
 * Copyright (c) 2017 Florian Dollinger <dollinger.florian@gmx.de>
 * Copyright (c) 2026 Kai Krakow <kai@kaishome.de>
 */

#include <linux/module.h>

#include "xpadneo.h"
#include "helpers.h"

/* always include last */
#include "compat.h"

/* XBE2 controllers support four profiles */
#define XPADNEO_XBE2_PROFILES_MAX 4

static bool param_gamepad_compliance = 1;
module_param_named(gamepad_compliance, param_gamepad_compliance, bool, 0444);
MODULE_PARM_DESC(gamepad_compliance,
		 "(bool) Adhere to Linux Gamepad Specification by using signed axis values. "
		 "1: enable, 0: disable.");

static bool param_disable_deadzones;
module_param_named(disable_deadzones, param_disable_deadzones, bool, 0444);
MODULE_PARM_DESC(disable_deadzones,
		 "(bool) Disable dead zone handling for raw processing by Wine/Proton, confuses joydev. "
		 "0: disable, 1: enable.");

static bool param_disable_shift_mode;
module_param_named(disable_shift_mode, param_disable_shift_mode, bool, 0644);
MODULE_PARM_DESC(disable_shift_mode,
		 "(bool) Disable use Xbox logo button as shift. Will prohibit profile switching when enabled. "
		 "0: disable, 1: enable.");

static void switch_profile(struct xpadneo_devdata *xdata, const u8 profile, const bool emulated)
{
	if (xdata->profile != profile) {
		hid_info(xdata->hdev, "switching profile to %d\n", profile);
		xdata->profile = profile;
	}

	/* Indicate to profile emulation that a request was made */
	if (emulated)
		xdata->profile_switched = true;
}

static void switch_triggers(struct xpadneo_devdata *xdata, const u8 mode)
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

int xpadneo_events_raw_event(struct hid_device *hdev, struct hid_report *report,
			     u8 *data, int reportsize)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	/* the controller spams reports multiple times */
	if (likely(report->id == 0x01)) {
		int size = min_t(int, reportsize, XPADNEO_REPORT_0x01_LENGTH);

		if (likely(memcmp(&xdata->input_report_0x01, data, size) == 0))
			return -1;
		memcpy(&xdata->input_report_0x01, data, size);
	}

	xpadneo_debug_hid_report(hdev, data, reportsize);

	/* we are taking care of the battery report ourselves */
	if (xdata->battery.report_id && report->id == xdata->battery.report_id && reportsize == 2) {
		xpadneo_power_update(xdata, data[1]);
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
					"detected broken XBE2 v1 packet format, please update the firmware\n");
			switch_profile(xdata, data[35] & 0x03, false);
			switch_triggers(xdata, data[36] & 0x0F);
		} else if (reportsize >= 21) {
			/* firmware 4.x style packet */
			switch_profile(xdata, data[19] & 0x03, false);
			switch_triggers(xdata, data[20] & 0x0F);
		} else {
			/* firmware 5.x style packet */
			switch_profile(xdata, data[17] & 0x03, false);
			switch_triggers(xdata, data[18] & 0x0F);
		}
	}

	if (xpadneo_mouse_raw_event(xdata, report, data, reportsize))
		return -1;

	return 0;
}

int xpadneo_events_event(struct hid_device *hdev, struct hid_field *field,
			 struct hid_usage *usage, __s32 value)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct input_dev *gamepad = xdata->gamepad.idev;
	struct input_dev *keyboard = xdata->keyboard.idev;

	if (xpadneo_mouse_event(xdata, usage, value))
		goto stop_processing;

	if ((usage->type == EV_KEY) && (usage->code == BTN_GRIPL)) {
		if (gamepad && xdata->profile == 0) {
			/* report the paddles individually */
			input_report_key(gamepad, BTN_GRIPR, (value & 1) ? 1 : 0);
			input_report_key(gamepad, BTN_GRIPR2, (value & 2) ? 1 : 0);
			input_report_key(gamepad, BTN_GRIPL, (value & 4) ? 1 : 0);
			input_report_key(gamepad, BTN_GRIPL2, (value & 8) ? 1 : 0);
			xdata->gamepad.sync = true;
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
				xdata->gamepad.sync = true;
				goto stop_processing;
			}
			break;
		case ABS_Z:
			xdata->last_abs_z = value;
			break;
		case ABS_RZ:
			xdata->last_abs_rz = value;
			break;
		}
	} else if (!param_disable_shift_mode && (usage->type == EV_KEY)
		   && (usage->code == BTN_XBOX)) {
		/*
		 * Handle the Xbox logo button: We want to cache the button
		 * down event to allow for profile switching. The button will
		 * act as a shift key and only send the input events when
		 * released without pressing an additional button.
		 */

		if (!xdata->shift_mode && (value == 1)) {
			/* cache this event */
			xdata->shift_mode = true;
		} else if (xdata->shift_mode && (value == 0)) {
			xdata->shift_mode = false;
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
		xdata->keyboard.sync = true;
		goto stop_processing;
	} else if (xdata->shift_mode && (usage->type == EV_KEY)) {
		hid_notice_once(hdev,
				"shift mode active: operation of the Xbox button may be limited in Steam Input\n");
		if (!(xdata->quirks & XPADNEO_QUIRK_USE_HW_PROFILES)) {
			switch (usage->code) {
			case BTN_A:
				if (value == 1)
					switch_profile(xdata, 0, true);
				goto stop_processing;
			case BTN_B:
				if (value == 1)
					switch_profile(xdata, 1, true);
				goto stop_processing;
			case BTN_X:
				if (value == 1)
					switch_profile(xdata, 2, true);
				goto stop_processing;
			case BTN_Y:
				if (value == 1)
					switch_profile(xdata, 3, true);
				goto stop_processing;
			}
		}
		switch (usage->code) {
		case BTN_SELECT:
			if ((value == 1) && xpadneo_mouse_toggle(xdata))
				xdata->profile_switched = true;
			goto stop_processing;
		}
	}

	/* Let hid-core handle the event */
	xdata->gamepad.sync = true;
	return 0;

keyboard_missing:
	xpadneo_device_missing(xdata, XPADNEO_MISSING_KEYBOARD);

stop_processing:
	/* report the profile change */
	if (xdata->profile >= XPADNEO_XBE2_PROFILES_MAX) {
		hid_notice_once(hdev, "unexpected profile value %d\n", xdata->profile);
	} else if (xdata->last_profile != xdata->profile) {
		/*
		 * Profile axis mirrors LED state; originating physical event already
		 * forces sync.
		 */
		input_report_abs(gamepad, ABS_PROFILE, xdata->profile);
		xdata->last_profile = xdata->profile;
	}

	return 1;
}

int xpadneo_events_input_configured(struct hid_device *hdev, struct hid_input *hi)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	int deadzone = 3072, abs_min = 0, abs_max = 65535;

	switch (hi->application) {
	case HID_GD_GAMEPAD:
		hid_info(hdev, "gamepad detected\n");
		xdata->gamepad.idev = hi->input;
		break;
	case HID_GD_KEYBOARD:
		hid_info(hdev, "keyboard detected\n");
		xdata->keyboard.idev = hi->input;

		/* do not report bogus keys as part of the keyboard */
		__clear_bit(KEY_UNKNOWN, xdata->keyboard.idev->keybit);

		return 0;
	case HID_CP_CONSUMER_CONTROL:
		hid_info(hdev, "consumer control detected\n");
		xdata->consumer.idev = hi->input;
		return 0;
	case 0xFF000005:
		/* FIXME: this is no longer in the current firmware */
		hid_info(hdev, "mapping profiles detected\n");
		xdata->quirks |= XPADNEO_QUIRK_USE_HW_PROFILES;
		return 0;
	default:
		hid_warn(hdev, "unhandled input application 0x%x\n", hi->application);
		return 0;
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

	/* setup gamepad */
	do {
		struct input_dev *gamepad = xdata->gamepad.idev;

		/* set thumb stick parameters */
		input_set_abs_params(gamepad, ABS_X, abs_min, abs_max, 32, deadzone);
		input_set_abs_params(gamepad, ABS_Y, abs_min, abs_max, 32, deadzone);
		input_set_abs_params(gamepad, ABS_RX, abs_min, abs_max, 32, deadzone);
		input_set_abs_params(gamepad, ABS_RY, abs_min, abs_max, 32, deadzone);

		/* set trigger parameters */
		input_set_abs_params(gamepad, ABS_Z, 0, 1023, 4, 0);
		input_set_abs_params(gamepad, ABS_RZ, 0, 1023, 4, 0);

		/* do not report the keyboard buttons as part of the gamepad */
		__clear_bit(BTN_SHARE, gamepad->keybit);
		__clear_bit(KEY_RECORD, gamepad->keybit);
		__clear_bit(KEY_UNKNOWN, gamepad->keybit);

		/* ensure all four paddles exist as part of the gamepad */
		if (test_bit(BTN_GRIPL, gamepad->keybit)) {
			__set_bit(BTN_GRIPR, gamepad->keybit);
			__set_bit(BTN_GRIPL2, gamepad->keybit);
			__set_bit(BTN_GRIPR2, gamepad->keybit);
		}

		/* expose current profile as axis */
		input_set_abs_params(gamepad, ABS_PROFILE, 0, XPADNEO_XBE2_PROFILES_MAX - 1, 0, 0);
		input_report_abs(gamepad, ABS_PROFILE, 0);
	} while (0);

	return 0;
}
