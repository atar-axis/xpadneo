// SPDX-License-Identifier: GPL-2.0-only

/*
 * xpadneo consumer control driver
 *
 * Copyright (c) 2021 Kai Krakow <kai@kaishome.de>
 */

#include "../xpadneo.h"

extern int xpadneo_init_consumer(struct xpadneo_devdata *xdata)
{
	struct hid_device *hdev = xdata->hdev;
	int ret, synth = 0;

	if (!xdata->consumer) {
		synth = 1;
		ret = xpadneo_init_synthetic(xdata, "Consumer Control", &xdata->consumer);
		if (ret || !xdata->consumer)
			return ret;
	}

	if (synth) {
		ret = input_register_device(xdata->consumer);
		if (ret) {
			hid_err(hdev, "failed to register consumer control\n");
			return ret;
		}

		hid_info(hdev, "consumer control added\n");
	}

	return 0;
}
