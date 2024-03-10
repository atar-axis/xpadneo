/*
 * xpadneo keyboard driver
 *
 * Copyright (c) 2021 Kai Krakow <kai@kaishome.de>
 */

#include "../xpadneo.h"

extern int xpadneo_init_keyboard(struct xpadneo_devdata *xdata)
{
	struct hid_device *hdev = xdata->hdev;
	int ret, synth = 0;

	if (!xdata->keyboard) {
		synth = 1;
		ret = xpadneo_init_synthetic(xdata, "Keyboard", &xdata->keyboard);
		if (ret || !xdata->keyboard)
			return ret;
	}

	/* enable key events for keyboard */
	input_set_capability(xdata->keyboard, EV_KEY, BTN_SHARE);

	/* enable key events for mouse mode */
	input_set_capability(xdata->keyboard, EV_KEY, KEY_ESC);
	input_set_capability(xdata->keyboard, EV_KEY, KEY_ENTER);
	input_set_capability(xdata->keyboard, EV_KEY, KEY_UP);
	input_set_capability(xdata->keyboard, EV_KEY, KEY_LEFT);
	input_set_capability(xdata->keyboard, EV_KEY, KEY_RIGHT);
	input_set_capability(xdata->keyboard, EV_KEY, KEY_DOWN);

	if (synth) {
		ret = input_register_device(xdata->keyboard);
		if (ret) {
			hid_err(hdev, "failed to register keyboard\n");
			return ret;
		}

		hid_info(hdev, "keyboard added\n");
	}

	return 0;
}
