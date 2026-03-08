// SPDX-License-Identifier: GPL-2.0-only

/*
 * xpadneo helpers for synthetic drivers
 *
 * Copyright (c) 2021 Kai Krakow <kai@kaishome.de>
 */

#include "xpadneo.h"

int xpadneo_synthetic_init(struct xpadneo_devdata *xdata, const char *suffix,
			   struct xpadneo_subdevice *subdev)
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

	subdev->idev = input_dev;
	subdev->is_synthetic = true;
	subdev->sync = false;

	return 0;
}

int xpadneo_synthetic_register(struct xpadneo_devdata *xdata, const char *name,
			       struct xpadneo_subdevice *subdev)
{
	struct hid_device *hdev = xdata->hdev;

	/* register the device on our behalf if synthetic */
	if (subdev->is_synthetic) {
		int ret = input_register_device(subdev->idev);

		if (ret) {
			hid_err(hdev, "failed to register %s\n", name);
			subdev->idev = NULL;
			subdev->is_synthetic = false;
			return ret;
		}

		hid_info(hdev, "%s added\n", name);
	}
	return 0;
}

void xpadneo_synthetic_remove(struct xpadneo_devdata *xdata, const char *name,
			      struct xpadneo_subdevice *subdev)
{
	struct hid_device *hdev = xdata->hdev;

	/* unregister the device on our behalf if synthetic */
	if (subdev->idev && subdev->is_synthetic) {
		input_unregister_device(subdev->idev);
		subdev->idev = NULL;
		subdev->is_synthetic = false;
		hid_info(hdev, "%s removed\n", name);
	}
}
