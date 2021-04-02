/*
 * xpadneo mouse driver
 *
 * Copyright (c) 2021 Kai Krakow <kai@kaishome.de>
 */

#include "../xpadneo.h"

extern void xpadneo_toggle_mouse(struct xpadneo_devdata *xdata)
{
	if (!xdata->mouse) {
		xdata->mouse_mode = false;
		hid_info(xdata->hdev, "mouse not available\n");
	} else if (xdata->mouse_mode) {
		xdata->mouse_mode = false;
		hid_info(xdata->hdev, "mouse mode disabled\n");
	} else {
		xdata->mouse_mode = true;
		hid_info(xdata->hdev, "mouse mode enabled\n");
	}

	/* Indicate that a request was made */
	xdata->profile_switched = true;
}

#define mouse_report_rel(a,v) if((v)!=0)input_report_rel(mouse,(a),(v))
extern void xpadneo_mouse_report(struct timer_list *t)
{
	__s32 value;

	struct xpadneo_devdata *xdata = timer_container_of(xdata, t, mouse_timer);
	struct input_dev *mouse = xdata->mouse;

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

		input_sync(xdata->mouse);
	}

}

extern int xpadneo_mouse_raw_event(struct xpadneo_devdata *xdata, struct hid_report *report,
				   u8 *data, int reportsize)
{
	if (!xdata->mouse_mode)
		return 0;

	/* do nothing for now */
	return 0;
}

#define rescale_axis(v,d) (((v)<(d)&&(v)>-(d))?0:(32768*((v)>0?(v)-(d):(v)+(d))/(32768-(d))))
#define digipad(v,v1,v2,v3) (((v==(v1))||(v==(v2))||(v==(v3)))?1:0)
extern int xpadneo_mouse_event(struct xpadneo_devdata *xdata, struct hid_usage *usage, __s32 value)
{
	struct input_dev *consumer = xdata->consumer;
	struct input_dev *keyboard = xdata->keyboard;
	struct input_dev *mouse = xdata->mouse;

	if (!xdata->mouse_mode)
		return 0;

	if (usage->type == EV_ABS) {
		switch (usage->code) {
		case ABS_X:
			xdata->mouse_state.rel_x = rescale_axis(value - 32768, 3072);
			return 1;
		case ABS_Y:
			xdata->mouse_state.rel_y = rescale_axis(value - 32768, 3072);
			return 1;
		case ABS_RX:
			xdata->mouse_state.wheel_x = rescale_axis(value - 32768, 3072);
			return 1;
		case ABS_RY:
			xdata->mouse_state.wheel_y = rescale_axis(value - 32768, 3072);
			return 1;
		case ABS_RZ:
			if (xdata->mouse_state.analog_button.left && value < 384) {
				xdata->mouse_state.analog_button.left = false;
				input_report_key(mouse, BTN_LEFT, 0);
				xdata->mouse_sync = true;
			} else if (!xdata->mouse_state.analog_button.left && value > 640) {
				xdata->mouse_state.analog_button.left = true;
				input_report_key(mouse, BTN_LEFT, 1);
				xdata->mouse_sync = true;
			}
			return 1;
		case ABS_Z:
			if (xdata->mouse_state.analog_button.right && value < 384) {
				xdata->mouse_state.analog_button.right = false;
				input_report_key(mouse, BTN_RIGHT, 0);
				xdata->mouse_sync = true;
			} else if (!xdata->mouse_state.analog_button.right && value > 640) {
				xdata->mouse_state.analog_button.right = true;
				input_report_key(mouse, BTN_RIGHT, 1);
				xdata->mouse_sync = true;
			}
			return 1;
		case ABS_HAT0X:
			/* reports 8 directions: 1 = up, 3 = right, down = 5, left = 7 */
			if (xdata->keyboard) {
				input_report_key(keyboard, KEY_UP, digipad(value, 8, 1, 2));
				input_report_key(keyboard, KEY_RIGHT, digipad(value, 2, 3, 4));
				input_report_key(keyboard, KEY_DOWN, digipad(value, 4, 5, 6));
				input_report_key(keyboard, KEY_LEFT, digipad(value, 6, 7, 8));
				xdata->keyboard_sync = true;
			}
			return 1;
		}
	} else if (usage->type == EV_KEY) {
		switch (usage->code) {
		case BTN_A:
			if (!keyboard)
				goto keyboard_missing;
			input_report_key(keyboard, KEY_ENTER, value);
			xdata->keyboard_sync = true;
			return 1;
		case BTN_B:
			if (!keyboard)
				goto keyboard_missing;
			input_report_key(keyboard, KEY_ESC, value);
			xdata->keyboard_sync = true;
			return 1;
		case BTN_X:
			if (!consumer)
				goto consumer_missing;
			input_report_key(consumer, KEY_ONSCREEN_KEYBOARD, value);
			xdata->consumer_sync = true;
			return 1;
		case BTN_Y:
			input_report_key(mouse, BTN_MIDDLE, value);
			xdata->mouse_sync = true;
			return 1;
		case BTN_TL:
			input_report_key(mouse, BTN_SIDE, value);
			xdata->mouse_sync = true;
			return 1;
		case BTN_TR:
			input_report_key(mouse, BTN_EXTRA, value);
			xdata->mouse_sync = true;
			return 1;
		case BTN_SELECT:
			input_report_key(mouse, BTN_BACK, value);
			xdata->mouse_sync = true;
			return 1;
		case BTN_START:
			input_report_key(mouse, BTN_FORWARD, value);
			xdata->mouse_sync = true;
			return 1;
		case BTN_SHARE:
			input_report_key(mouse, BTN_TASK, value);
			xdata->mouse_sync = true;
			return 1;
		}
	}

	return 0;

keyboard_missing:
	xpadneo_core_missing(xdata, XPADNEO_MISSING_KEYBOARD);
	return 1;

consumer_missing:
	xpadneo_core_missing(xdata, XPADNEO_MISSING_CONSUMER);
	return 1;
}

extern int xpadneo_init_mouse(struct xpadneo_devdata *xdata)
{
	struct hid_device *hdev = xdata->hdev;
	int ret, synth = 0;

	if (!xdata->mouse) {
		synth = 1;
		ret = xpadneo_init_synthetic(xdata, "Mouse", &xdata->mouse);
		if (ret || !xdata->mouse)
			return ret;
	}

	/* enable relative events for mouse emulation */
	__set_bit(EV_REL, xdata->mouse->evbit);
	__set_bit(REL_X, xdata->mouse->relbit);
	__set_bit(REL_Y, xdata->mouse->relbit);
	__set_bit(REL_HWHEEL, xdata->mouse->relbit);
	__set_bit(REL_WHEEL, xdata->mouse->relbit);

	/* enable button events for mouse emulation */
	__set_bit(EV_KEY, xdata->mouse->evbit);
	__set_bit(BTN_LEFT, xdata->mouse->keybit);
	__set_bit(BTN_RIGHT, xdata->mouse->keybit);
	__set_bit(BTN_MIDDLE, xdata->mouse->keybit);
	__set_bit(BTN_SIDE, xdata->mouse->keybit);
	__set_bit(BTN_EXTRA, xdata->mouse->keybit);
	__set_bit(BTN_FORWARD, xdata->mouse->keybit);
	__set_bit(BTN_BACK, xdata->mouse->keybit);
	__set_bit(BTN_TASK, xdata->mouse->keybit);

	if (synth) {
		ret = input_register_device(xdata->mouse);
		if (ret) {
			hid_err(hdev, "failed to register mouse\n");
			return ret;
		}

		hid_info(hdev, "mouse added\n");
	}

	return 0;
}
