// SPDX-License-Identifier: GPL-3.0-only

/*
 * xpadneo consumer control driver
 *
 * Copyright (c) 2021 Kai Krakow <kai@kaishome.de>
 */

#include "xpadneo.h"

extern int xpadneo_consumer_init(struct xpadneo_devdata *xdata)
{
	struct hid_device *hdev = xdata->hdev;

	if (!xdata->consumer) {
		int ret = xpadneo_synthetic_init(xdata, "Consumer Control", &xdata->consumer);

		if (ret || !xdata->consumer)
			return ret;

		xdata->consumer_is_synthetic = true;
	}

	/* enable consumer events for mouse mode */
	input_set_capability(xdata->consumer, EV_KEY, KEY_ONSCREEN_KEYBOARD);

	if (xdata->consumer_is_synthetic) {
		int ret = input_register_device(xdata->consumer);

		if (ret) {
			hid_err(hdev, "failed to register consumer control\n");
			xdata->consumer = NULL;
			xdata->consumer_is_synthetic = false;
			return ret;
		}

		hid_info(hdev, "consumer control added\n");
	}

	return 0;
}
