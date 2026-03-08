// SPDX-License-Identifier: GPL-2.0-only

/*
 * xpadneo consumer control driver
 *
 * Copyright (c) 2021 Kai Krakow <kai@kaishome.de>
 */

#include "xpadneo.h"

int xpadneo_consumer_init(struct xpadneo_devdata *xdata)
{
	int ret;

	if (!xdata->consumer.idev) {
		ret = xpadneo_synthetic_init(xdata, "Consumer Control", &xdata->consumer);
		if (ret || !xdata->consumer.idev)
			return ret;
	}

	/* enable consumer events for mouse mode */
	input_set_capability(xdata->consumer.idev, EV_KEY, KEY_ONSCREEN_KEYBOARD);

	ret = xpadneo_synthetic_register(xdata, "consumer control", &xdata->consumer);
	if (ret)
		return ret;

	return 0;
}

void xpadneo_consumer_remove(struct xpadneo_devdata *xdata)
{
	xpadneo_synthetic_remove(xdata, "consumer control", &xdata->consumer);
}
