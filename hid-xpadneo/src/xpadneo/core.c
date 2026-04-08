// SPDX-License-Identifier: GPL-2.0-only

/*
 * xpadneo driver core
 *
 * This driver was developed for a student project at fortiss GmbH in Munich by
 * Florian Dollinger.
 *
 * Copyright (c) 2021 Kai Krakow <kai@kaishome.de>
 * Copyright (c) 2017 Florian Dollinger <dollinger.florian@gmx.de>
 */

#include <linux/hid.h>
#include <linux/idr.h>
#include <linux/input.h>
#include <linux/module.h>

#include "xpadneo.h"

/* always include last */
#include "compat.h"

#ifndef VERSION
#error "xpadneo version not defined"
#endif

#define XPADNEO_VERSION __stringify(VERSION)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Florian Dollinger <dollinger.florian@gmx.de>");
MODULE_AUTHOR("Kai Krakow <kai@kaishome.de>");
MODULE_DESCRIPTION("Linux kernel driver for Xbox ONE S+ gamepads (Bluetooth), including rumble");
MODULE_VERSION(XPADNEO_VERSION);
MODULE_SOFTDEP("pre: uhid");

static DEFINE_IDA(xpadneo_core_device_id_allocator);

#ifndef USB_VENDOR_ID_MICROSOFT
#define USB_VENDOR_ID_MICROSOFT 0x045e
#endif

static const struct hid_device_id core_devices[] = {
	/* XBOX ONE S / X */
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x02E0) },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x02FD) },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x0B20),
	 .driver_data = XPADNEO_QUIRK_SHARE_BUTTON },

	/* XBOX ONE Elite Series 2 */
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x0B05) },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x0B22),
	 .driver_data = XPADNEO_QUIRK_SHARE_BUTTON },

	/* XBOX Series X|S / Xbox Wireless Controller (BLE) */
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x0B13),
	 .driver_data = XPADNEO_QUIRK_SHARE_BUTTON },

	/* SENTINEL VALUE, indicates the end */
	{ }
};

MODULE_DEVICE_TABLE(hid, core_devices);

static int core_init_base_device(struct hid_device *hdev)
{
	int ret;
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	if (!xdata->gamepad.idev) {
		xpadneo_device_missing(xdata, XPADNEO_MISSING_GAMEPAD);
		return -EINVAL;
	}

	ret = xpadneo_power_init(xdata);
	if (ret)
		goto err_free_name;

	ret = xpadneo_quirks_init(xdata);
	if (ret)
		goto err_uninit_power;

	return 0;

err_uninit_power:
	xpadneo_power_remove(xdata);

err_free_name:
	return ret;
}

static void core_release_device_id(struct xpadneo_devdata *xdata)
{
	if (xdata->id >= 0) {
		ida_free(&xpadneo_core_device_id_allocator, xdata->id);
		xdata->id = -1;
	}
}

static void core_remove(struct hid_device *hdev)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	hid_hw_close(hdev);

	if (hdev->version != xdata->original_version) {
		hid_info(hdev,
			 "reverting to original version "
			 "(changed version from 0x%08X to 0x%08X)\n",
			 hdev->version, xdata->original_version);
		hdev->version = xdata->original_version;
	}

	if (hdev->product != xdata->original_product) {
		hid_info(hdev,
			 "reverting to original product "
			 "(changed PID from 0x%04X to 0x%04X)\n",
			 hdev->product, xdata->original_product);
		hdev->product = xdata->original_product;
	}

	xpadneo_mouse_remove_timer(xdata);
	xpadneo_rumble_remove(xdata);
	xpadneo_quirks_remove(xdata);
	xpadneo_power_remove(xdata);
	xpadneo_mouse_remove(xdata);
	xpadneo_keyboard_remove(xdata);
	xpadneo_consumer_remove(xdata);

	core_release_device_id(xdata);
	hid_hw_stop(hdev);
}

#if KERNEL_VERSION(4, 18, 0) > LINUX_VERSION_CODE
#error "kernel version 4.18.0+ required for HID_QUIRK_INPUT_PER_APP"
#endif
static int core_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	int ret, index;
	struct xpadneo_devdata *xdata;

	xdata = devm_kzalloc(&hdev->dev, sizeof(*xdata), GFP_KERNEL);
	if (xdata == NULL)
		return -ENOMEM;

	index = ida_alloc(&xpadneo_core_device_id_allocator, GFP_KERNEL);
	if (index < 0)
		return index;
	xdata->id = index;

	xdata->quirks = id->driver_data;

	xdata->hdev = hdev;
	hdev->quirks |= HID_QUIRK_INPUT_PER_APP;
	hdev->quirks |= HID_QUIRK_NO_INPUT_SYNC;
	hid_set_drvdata(hdev, xdata);

	xdata->uses_hogp = false;
	if (hdev->version == 0x00000903)
		hid_warn(hdev, "buggy firmware detected, please upgrade to the latest version\n");
	else if (hdev->version < 0x00000500)
		hid_warn(hdev,
			 "classic Bluetooth firmware version %x.%02x, please upgrade for better stability\n",
			 hdev->version >> 8, (u8)hdev->version);
	else if (hdev->version < 0x00000512) {
		xdata->uses_hogp = (hdev->bus == BUS_BLUETOOTH);
		hid_warn(hdev,
			 "BLE firmware version %x.%02x, please upgrade for better stability\n",
			 hdev->version >> 8, (u8)hdev->version);
	} else {
		xdata->uses_hogp = (hdev->bus == BUS_BLUETOOTH);
		hid_info(hdev, "BLE firmware version %x.%02x\n",
			 hdev->version >> 8, (u8)hdev->version);
	}

	/* log if we are using the HOGP protocol */
	if (xdata->uses_hogp) {
		hid_info(hdev,
			 "expecting HOGP protocol, this will cause rumble issues if the controller does not use BLE\n");
	}

	/*
	 * Pretend that we are in Windows pairing mode as we are actually
	 * exposing the Windows mapping. This prevents SDL and other layers
	 * (probably browser game controller APIs) from treating our driver
	 * unnecessarily with button and axis mapping fixups, and it seems
	 * this is actually a firmware mode meant for Android usage only:
	 *
	 * Xbox One S:
	 * 0x2E0 wireless Windows mode (non-Android mode)
	 * 0x2EA USB Windows and Linux mode
	 * 0x2FD wireless Linux mode (Android mode)
	 *
	 * Xbox Elite 2:
	 * 0xB00 USB Windows and Linux mode
	 * 0xB05 wireless Linux mode (Android mode)
	 *
	 * Xbox Series X|S:
	 * 0xB12 Dongle, USB Windows and USB Linux mode
	 * 0xB13 wireless Linux mode (Android mode)
	 *
	 * Xbox Controller BLE mode:
	 * 0xB20 wireless BLE mode
	 *
	 * TODO: We should find a better way of doing this so SDL2 could
	 * still detect our driver as the correct model. Currently this
	 * maps all controllers to the same model.
	 */
	xdata->original_product = hdev->product;
	xdata->original_version = hdev->version;
	hdev->product = 0x028E;
	hdev->version = 0x00001130;

	if (hdev->product != xdata->original_product)
		hid_info(hdev,
			 "pretending XB1S Windows wireless mode "
			 "(changed PID from 0x%04X to 0x%04X)\n", xdata->original_product,
			 hdev->product);

	if (hdev->version != xdata->original_version)
		hid_info(hdev,
			 "working around wrong SDL2 mappings "
			 "(changed version from 0x%08X to 0x%08X)\n", xdata->original_version,
			 hdev->version);

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "parse failed\n");
		goto err_release_id;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret) {
		hid_err(hdev, "hw start failed\n");
		goto err_release_id;
	}

	ret = xpadneo_consumer_init(xdata);
	if (ret)
		goto err_stop_hw;

	ret = xpadneo_keyboard_init(xdata);
	if (ret)
		goto err_uninit_consumer;

	ret = xpadneo_mouse_init(xdata);
	if (ret)
		goto err_uninit_keyboard;

	ret = core_init_base_device(hdev);
	if (ret) {
		hid_err(hdev, "hw init failed: %d\n", ret);
		goto err_uninit_mouse;
	}

	ret = xpadneo_rumble_init(hdev);
	if (ret)
		hid_err(hdev, "could not initialize rumble, continuing anyway\n");

	xpadneo_mouse_init_timer(xdata);

	hid_info(hdev, "%s connected\n", xdata->battery.name);

	return 0;

	/* error paths in reverse initialization order */
err_uninit_mouse:
	xpadneo_mouse_remove(xdata);

err_uninit_keyboard:
	xpadneo_keyboard_remove(xdata);

err_uninit_consumer:
	xpadneo_consumer_remove(xdata);

err_stop_hw:
	hid_hw_stop(hdev);

err_release_id:
	/* restore the original device IDs first */
	hdev->product = xdata->original_product;
	hdev->version = xdata->original_version;

	core_release_device_id(xdata);
	return ret;
}

static struct hid_driver core_driver = {
	.name = "xpadneo",
	.event = xpadneo_events_event,
	.id_table = core_devices,
	.input_configured = xpadneo_events_input_configured,
	.input_mapping = xpadneo_mappings_input,
	.probe = core_probe,
	.remove = core_remove,
	.report = xpadneo_device_report,
	.report_fixup = xpadneo_device_report_fixup_compat,
	.raw_event = xpadneo_events_raw_event,
};

static int __init core_init(void)
{
	int ret;

	pr_info("loaded hid-xpadneo %s\n", XPADNEO_VERSION);
	dbg_hid("xpadneo:%s\n", __func__);

	ret = xpadneo_rumble_init_workqueue();
	if (!ret) {
		ret = hid_register_driver(&core_driver);
		if (ret)
			xpadneo_rumble_destroy_workqueue();
	}

	return ret;
}

static void __exit core_exit(void)
{
	dbg_hid("xpadneo:%s\n", __func__);
	hid_unregister_driver(&core_driver);
	ida_destroy(&xpadneo_core_device_id_allocator);
	xpadneo_rumble_destroy_workqueue();
}

module_init(core_init);
module_exit(core_exit);
