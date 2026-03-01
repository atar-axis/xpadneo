// SPDX-License-Identifier: GPL-3.0-only

/*
 * xpadneo helpers for synthetic drivers
 *
 * Copyright (c) 2021 Kai Krakow <kai@kaishome.de>
 */

#include "xpadneo.h"

int xpadneo_synthetic_init(struct xpadneo_devdata *xdata, const char *suffix,
			   struct input_dev **devp)
{
	struct hid_device *hdev = xdata->hdev;
	struct input_dev *input_dev = devm_input_allocate_device(&hdev->dev);
	size_t suffix_len, name_len;

	if (!input_dev)
		return -ENOMEM;

	name_len = strlen(hdev->name);
	suffix_len = strlen(suffix);
	if ((name_len < suffix_len) || strcmp(hdev->name + name_len - suffix_len, suffix)) {
		char *name = devm_kasprintf(&hdev->dev, GFP_KERNEL, "%s %s", hdev->name, suffix);

		if (!name)
			return -ENOMEM;
		input_dev->name = name;
	} else {
		input_dev->name = hdev->name;
	}

	dev_set_drvdata(&input_dev->dev, xdata);
	input_dev->phys = hdev->phys;
	input_dev->uniq = hdev->uniq;
	input_dev->id.bustype = hdev->bus;
	input_dev->id.vendor = hdev->vendor;
	input_dev->id.product = hdev->product;
	input_dev->id.version = hdev->version;

	*devp = input_dev;
	return 0;
}
