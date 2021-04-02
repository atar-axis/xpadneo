/*
 * xpadneo driver core
 *
 * Copyright (c) 2021 Kai Krakow <kai@kaishome.de>
 */

#include "../xpadneo.h"

extern int xpadneo_init_synthetic(struct xpadneo_devdata *xdata, char *suffix,
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

extern void xpadneo_report(struct hid_device *hdev, struct hid_report *report)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	if (xdata->consumer && xdata->consumer_sync) {
		xdata->consumer_sync = false;
		input_sync(xdata->consumer);
	}

	if (xdata->gamepad && xdata->gamepad_sync) {
		xdata->gamepad_sync = false;
		input_sync(xdata->gamepad);
	}

	if (xdata->keyboard && xdata->keyboard_sync) {
		xdata->keyboard_sync = false;
		input_sync(xdata->keyboard);
	}

	if (xdata->mouse && xdata->mouse_sync) {
		xdata->mouse_sync = false;
		input_sync(xdata->mouse);
	}
}

extern void xpadneo_core_missing(struct xpadneo_devdata *xdata, u32 flag)
{
	struct hid_device *hdev = xdata->hdev;

	if ((xdata->missing_reported & flag) == 0) {
		xdata->missing_reported |= flag;
		switch (flag) {
		case XPADNEO_MISSING_CONSUMER:
			hid_err(hdev, "consumer control not detected\n");
			break;
		case XPADNEO_MISSING_GAMEPAD:
			hid_err(hdev, "gamepad not detected\n");
			break;
		case XPADNEO_MISSING_KEYBOARD:
			hid_err(hdev, "keyboard not detected\n");
			break;
		default:
			hid_err(hdev, "unexpected subdevice missing: %d\n", flag);
		}
	}

}
