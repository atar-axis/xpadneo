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

#define mouse_report_rel(a,v) if((v)!=0)input_report_rel(mouse,a,v)
extern void xpadneo_mouse_report(struct timer_list *t)
{
	struct xpadneo_devdata *xdata = from_timer(xdata, t, mouse_timer);
	struct input_dev *mouse = xdata->mouse;

	mod_timer(&xdata->mouse_timer, jiffies + msecs_to_jiffies(10));

	if (xdata->mouse_mode) {
		mouse_report_rel(REL_X, xdata->mouse_state.rel_x);
		mouse_report_rel(REL_Y, xdata->mouse_state.rel_y);
		mouse_report_rel(REL_HWHEEL, xdata->mouse_state.wheel_x);
		mouse_report_rel(REL_WHEEL, xdata->mouse_state.wheel_y);
		input_sync(xdata->mouse);
	}

}

extern int xpadneo_mouse_raw_event(struct xpadneo_devdata *xdata, struct hid_report *report,
				   u8 *data, int reportsize)
{
	if (!xdata->mouse_mode)
		return 0;
	return 0; //todo!
}

#define rescale_axis(v,d) (((v)<(d)&&(v)>-(d))?0:(32768*((v)>0?(v)-(d):(v)+(d))/(32768-(d))))
extern int xpadneo_mouse_event(struct xpadneo_devdata *xdata, struct hid_usage *usage, __s32 value)
{
	if (!xdata->mouse_mode)
		return 0;

	if (usage->type == EV_ABS) {
		switch (usage->code) {
		case ABS_X:
			xdata->mouse_state.rel_x = rescale_axis(value - 32768, 3072) / 2048;
			return 1;
		case ABS_Y:
			xdata->mouse_state.rel_y = rescale_axis(value - 32768, 3072) / 2048;
			return 1;
		case ABS_RX:
			xdata->mouse_state.wheel_x = rescale_axis(value - 32768, 3072) / 8192;
			return 1;
		case ABS_RY:
			xdata->mouse_state.wheel_y = rescale_axis(value - 32768, 3072) / 8192;
			return 1;
		case ABS_Z:
			if (xdata->mouse_state.analog_button.left && value < 384) {
				xdata->mouse_state.analog_button.left = false;
				input_report_key(xdata->mouse, BTN_LEFT, 0);
				input_sync(xdata->mouse);
			} else if (!xdata->mouse_state.analog_button.left && value > 640) {
				xdata->mouse_state.analog_button.left = true;
				input_report_key(xdata->mouse, BTN_LEFT, 1);
				input_sync(xdata->mouse);
			}
			return 1;
		case ABS_RZ:
			if (xdata->mouse_state.analog_button.right && value < 384) {
				xdata->mouse_state.analog_button.right = false;
				input_report_key(xdata->mouse, BTN_RIGHT, 0);
				input_sync(xdata->mouse);
			} else if (!xdata->mouse_state.analog_button.right && value > 640) {
				xdata->mouse_state.analog_button.right = true;
				input_report_key(xdata->mouse, BTN_RIGHT, 1);
				input_sync(xdata->mouse);
			}
			return 1;
		}
	} else if (usage->type == EV_KEY) {
		switch (usage->code) {
		case BTN_TL:
			input_report_key(xdata->mouse, BTN_SIDE, value);
			input_sync(xdata->mouse);
			return 1;
		case BTN_TR:
			input_report_key(xdata->mouse, BTN_EXTRA, value);
			input_sync(xdata->mouse);
			return 1;
		case BTN_START:
			if (xdata->consumer) {
				input_report_key(xdata->consumer, KEY_KEYBOARD, value);
				input_sync(xdata->consumer);
			}
			return 1;
		}
	}

	return 0;
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
