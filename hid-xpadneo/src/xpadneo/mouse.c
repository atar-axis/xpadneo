// SPDX-License-Identifier: GPL-2.0-only

/*
 * xpadneo mouse driver
 *
 * Copyright (c) 2021 Kai Krakow <kai@kaishome.de>
 */

#include <linux/module.h>

#include "xpadneo.h"

/* always include last */
#include "compat.h"

static bool param_disable_mouse;
module_param_named(disable_mouse, param_disable_mouse, bool, 0444);
MODULE_PARM_DESC(disable_mouse,
		 "(bool) Disable mouse device permanently. 0: allow mouse, 1: disallow mouse.");

/* Mouse movement deadzone and trigger thresholds (raw values) */
#define XPADNEO_MOUSE_MOVEMENT_DEADZONE   3072
#define XPADNEO_TRIGGER_RELEASE_THRESHOLD 384
#define XPADNEO_TRIGGER_PRESS_THRESHOLD   640

bool xpadneo_mouse_toggle(struct xpadneo_devdata *xdata)
{
	if (!xdata->mouse.idev) {
		xdata->mouse_mode = false;
		hid_info(xdata->hdev, "mouse not available\n");
		return false;
	} else if (xdata->mouse_mode) {
		xdata->mouse_mode = false;
		hid_info(xdata->hdev, "mouse mode disabled\n");
	} else {
		xdata->mouse_mode = true;
		hid_info(xdata->hdev, "mouse mode enabled\n");
	}

	/* Indicate that a request was made */
	return true;
}

#define mouse_report_rel(a,v) if((v)!=0)input_report_rel(mouse,(a),(v))
void xpadneo_mouse_report(struct timer_list *t)
{
	__s32 value;

	struct xpadneo_devdata *xdata = timer_container_of(xdata, t, mouse_timer);
	struct input_dev *mouse = xdata->mouse.idev;

	mod_timer(&xdata->mouse_timer, jiffies + msecs_to_jiffies(10));

	if (xdata->mouse_mode) {
		value = xdata->mouse_state.rel_x + xdata->mouse_state.rel_x_err;
		xdata->mouse_state.rel_x_err = value % 1024;
		mouse_report_rel(REL_X, value / 1024);

		value = xdata->mouse_state.rel_y + xdata->mouse_state.rel_y_err;
		xdata->mouse_state.rel_y_err = value % 1024;
		mouse_report_rel(REL_Y, value / 1024);

		value = xdata->mouse_state.wheel_x + xdata->mouse_state.wheel_x_err;
		xdata->mouse_state.wheel_x_err = value % 16384;
		mouse_report_rel(REL_HWHEEL, value / 16384);

		value = xdata->mouse_state.wheel_y + xdata->mouse_state.wheel_y_err;
		xdata->mouse_state.wheel_y_err = value % 16384;
		mouse_report_rel(REL_WHEEL, value / 16384);

		input_sync(xdata->mouse.idev);
	}

}

int xpadneo_mouse_raw_event(struct xpadneo_devdata *xdata, struct hid_report *report,
			    u8 *data, int reportsize)
{
	if (!xdata->mouse_mode)
		return 0;

	/* do nothing for now */
	return 0;
}

static inline void report_key_and_sync(struct xpadneo_subdevice *subdev, unsigned int code,
				       int value)
{
	if (subdev->idev) {
		input_report_key(subdev->idev, code, value);
		subdev->sync = true;
	}
}

#define rescale_axis(v,d) (((v)<(d)&&(v)>-(d))?0:(32768*((v)>0?(v)-(d):(v)+(d))/(32768-(d))))
#define digipad(v,v1,v2,v3) (((v==(v1))||(v==(v2))||(v==(v3)))?1:0)
int xpadneo_mouse_event(struct xpadneo_devdata *xdata, struct hid_usage *usage, __s32 value)
{
	struct input_dev *consumer = xdata->consumer.idev;
	struct input_dev *keyboard = xdata->keyboard.idev;

	if (!xdata->mouse_mode || !xdata->mouse.idev)
		return 0;

	if (usage->type == EV_ABS) {
		switch (usage->code) {
		case ABS_X:
			xdata->mouse_state.rel_x =
			    rescale_axis(value - 32768, XPADNEO_MOUSE_MOVEMENT_DEADZONE);
			return 1;
		case ABS_Y:
			xdata->mouse_state.rel_y =
			    rescale_axis(value - 32768, XPADNEO_MOUSE_MOVEMENT_DEADZONE);
			return 1;
		case ABS_RX:
			xdata->mouse_state.wheel_x =
			    rescale_axis(value - 32768, XPADNEO_MOUSE_MOVEMENT_DEADZONE);
			return 1;
		case ABS_RY:
			xdata->mouse_state.wheel_y =
			    rescale_axis(value - 32768, XPADNEO_MOUSE_MOVEMENT_DEADZONE);
			return 1;
		case ABS_RZ:
			/* TODO: Implement haptic feedback */
			if (xdata->mouse_state.analog_button.left
			    && value < XPADNEO_TRIGGER_RELEASE_THRESHOLD) {
				xdata->mouse_state.analog_button.left = false;
				report_key_and_sync(&xdata->mouse, BTN_LEFT, 0);
			} else if (!xdata->mouse_state.analog_button.left
				   && value > XPADNEO_TRIGGER_PRESS_THRESHOLD) {
				xdata->mouse_state.analog_button.left = true;
				report_key_and_sync(&xdata->mouse, BTN_LEFT, 1);
			}
			return 1;
		case ABS_Z:
			/* TODO: Implement haptic feedback */
			if (xdata->mouse_state.analog_button.right
			    && value < XPADNEO_TRIGGER_RELEASE_THRESHOLD) {
				xdata->mouse_state.analog_button.right = false;
				report_key_and_sync(&xdata->mouse, BTN_RIGHT, 0);
			} else if (!xdata->mouse_state.analog_button.right
				   && value > XPADNEO_TRIGGER_PRESS_THRESHOLD) {
				xdata->mouse_state.analog_button.right = true;
				report_key_and_sync(&xdata->mouse, BTN_RIGHT, 1);
			}
			return 1;
		case ABS_HAT0X:
			/* reports 8 directions: 1 = up, 3 = right, down = 5, left = 7 */
			report_key_and_sync(&xdata->keyboard, KEY_UP, digipad(value, 8, 1, 2));
			report_key_and_sync(&xdata->keyboard, KEY_RIGHT, digipad(value, 2, 3, 4));
			report_key_and_sync(&xdata->keyboard, KEY_DOWN, digipad(value, 4, 5, 6));
			report_key_and_sync(&xdata->keyboard, KEY_LEFT, digipad(value, 6, 7, 8));
			return 1;
		}
	} else if (usage->type == EV_KEY) {
		switch (usage->code) {
		case BTN_A:
			if (!keyboard)
				goto keyboard_missing;
			report_key_and_sync(&xdata->keyboard, KEY_ENTER, value);
			return 1;
		case BTN_B:
			if (!keyboard)
				goto keyboard_missing;
			report_key_and_sync(&xdata->keyboard, KEY_ESC, value);
			return 1;
		case BTN_X:
			if (!consumer)
				goto consumer_missing;
			report_key_and_sync(&xdata->consumer, KEY_ONSCREEN_KEYBOARD, value);
			return 1;
		case BTN_Y:
			report_key_and_sync(&xdata->mouse, BTN_MIDDLE, value);
			return 1;
		case BTN_TL:
			report_key_and_sync(&xdata->mouse, BTN_SIDE, value);
			return 1;
		case BTN_TR:
			report_key_and_sync(&xdata->mouse, BTN_EXTRA, value);
			return 1;
		case BTN_SELECT:
			/*
			 * if the Xbox button is pressed, ignore this event to allow turning
			 * mouse mode off
			 */
			if (xdata->shift_mode)
				break;
			report_key_and_sync(&xdata->mouse, BTN_BACK, value);
			return 1;
		case BTN_START:
			report_key_and_sync(&xdata->mouse, BTN_FORWARD, value);
			return 1;
		case BTN_SHARE:
			report_key_and_sync(&xdata->mouse, BTN_TASK, value);
			return 1;
		}
	}

	return 0;

keyboard_missing:
	xpadneo_device_missing(xdata, XPADNEO_MISSING_KEYBOARD);
	return 1;

consumer_missing:
	xpadneo_device_missing(xdata, XPADNEO_MISSING_CONSUMER);
	return 1;
}

int xpadneo_mouse_init(struct xpadneo_devdata *xdata)
{
	int ret;

	if (param_disable_mouse) {
		xdata->mouse.idev = NULL;
		hid_info(xdata->hdev, "Mouse device disabled permanently.\n");
		return 0;
	}

	if (!xdata->mouse.idev) {
		ret = xpadneo_synthetic_init(xdata, "Mouse", &xdata->mouse);
		if (ret || !xdata->mouse.idev)
			return ret;
	}

	do {
		struct input_dev *mouse = xdata->mouse.idev;

		/* enable relative events for mouse emulation */
		__set_bit(EV_REL, mouse->evbit);
		__set_bit(REL_X, mouse->relbit);
		__set_bit(REL_Y, mouse->relbit);
		__set_bit(REL_HWHEEL, mouse->relbit);
		__set_bit(REL_WHEEL, mouse->relbit);

		/* enable button events for mouse emulation */
		__set_bit(EV_KEY, mouse->evbit);
		__set_bit(BTN_LEFT, mouse->keybit);
		__set_bit(BTN_RIGHT, mouse->keybit);
		__set_bit(BTN_MIDDLE, mouse->keybit);
		__set_bit(BTN_SIDE, mouse->keybit);
		__set_bit(BTN_EXTRA, mouse->keybit);
		__set_bit(BTN_FORWARD, mouse->keybit);
		__set_bit(BTN_BACK, mouse->keybit);
		__set_bit(BTN_TASK, mouse->keybit);
	} while (0);

	ret = xpadneo_synthetic_register(xdata, "mouse", &xdata->mouse);
	if (ret)
		return ret;

	return 0;
}

void xpadneo_mouse_init_timer(struct xpadneo_devdata *xdata)
{
	if (param_disable_mouse)
		return;

	timer_setup(&xdata->mouse_timer, xpadneo_mouse_report, 0);
	mod_timer(&xdata->mouse_timer, jiffies);
}

void xpadneo_mouse_remove_timer(struct xpadneo_devdata *xdata)
{
	if (param_disable_mouse)
		return;

	timer_delete_sync(&xdata->mouse_timer);
}

void xpadneo_mouse_remove(struct xpadneo_devdata *xdata)
{
	if (param_disable_mouse)
		return;

	xpadneo_synthetic_remove(xdata, "mouse", &xdata->mouse);
}
