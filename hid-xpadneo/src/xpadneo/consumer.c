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

	/* enable key events for consumer control */
	input_set_capability(xdata->consumer, EV_KEY, BTN_XBOX);
	input_set_capability(xdata->consumer, EV_KEY, BTN_SHARE);

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
