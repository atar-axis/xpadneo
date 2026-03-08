// SPDX-License-Identifier: GPL-2.0-only

/*
 * xpadneo keyboard driver
 *
 * Copyright (c) 2021 Kai Krakow <kai@kaishome.de>
 */

#include "xpadneo.h"

int xpadneo_keyboard_init(struct xpadneo_devdata *xdata)
{
	int ret;

	if (!xdata->keyboard.idev) {
		ret = xpadneo_synthetic_init(xdata, "Keyboard", &xdata->keyboard);
		if (ret || !xdata->keyboard.idev)
			return ret;
	}

	do {
		struct input_dev *keyboard = xdata->keyboard.idev;

		/* enable key events for keyboard */
		input_set_capability(keyboard, EV_KEY, BTN_SHARE);

		/* enable key events for mouse mode */
		input_set_capability(keyboard, EV_KEY, KEY_ESC);
		input_set_capability(keyboard, EV_KEY, KEY_ENTER);
		input_set_capability(keyboard, EV_KEY, KEY_UP);
		input_set_capability(keyboard, EV_KEY, KEY_LEFT);
		input_set_capability(keyboard, EV_KEY, KEY_RIGHT);
		input_set_capability(keyboard, EV_KEY, KEY_DOWN);
	} while (0);

	ret = xpadneo_synthetic_register(xdata, "keyboard", &xdata->keyboard);
	if (ret)
		return ret;

	return 0;
}

void xpadneo_keyboard_remove(struct xpadneo_devdata *xdata)
{
	xpadneo_synthetic_remove(xdata, "keyboard", &xdata->keyboard);
}
