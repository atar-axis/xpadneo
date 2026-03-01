// SPDX-License-Identifier: GPL-3.0-only

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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Florian Dollinger <dollinger.florian@gmx.de>");
MODULE_AUTHOR("Kai Krakow <kai@kaishome.de>");
MODULE_DESCRIPTION("Linux kernel driver for Xbox ONE S+ gamepads (Bluetooth), including rumble");
MODULE_VERSION(XPADNEO_VERSION);

static DEFINE_IDA(xpadneo_core_device_id_allocator);

static bool param_debug_hid;
module_param_named(debug_hid, param_debug_hid, bool, 0644);
MODULE_PARM_DESC(debug_hid, "(u8) Debug HID reports. 0: disable, 1: enable.");

static const struct hid_device_id xpadneo_devices[] = {
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

MODULE_DEVICE_TABLE(hid, xpadneo_devices);

int xpadneo_core_output_report(struct hid_device *hdev, __u8 *buf, size_t len)
{
	struct rumble_report *r = (struct rumble_report *)buf;

	if (unlikely(param_debug_hid && (len > 0))) {
		switch (buf[0]) {
		case 0x03:
			if (len >= sizeof(*r)) {
				hid_info(hdev,
					 "HID debug: len %ld rumble cmd 0x%02x "
					 "motors left %d right %d strong %d weak %d "
					 "magnitude left %d right %d strong %d weak %d "
					 "pulse sustain %dms release %dms loop %d\n",
					 len, r->report_id,
					 !!(r->data.enable & FF_RUMBLE_LEFT),
					 !!(r->data.enable & FF_RUMBLE_RIGHT),
					 !!(r->data.enable & FF_RUMBLE_STRONG),
					 !!(r->data.enable & FF_RUMBLE_WEAK),
					 r->data.magnitude_left, r->data.magnitude_right,
					 r->data.magnitude_strong, r->data.magnitude_weak,
					 r->data.pulse_sustain_10ms * 10,
					 r->data.pulse_release_10ms * 10, r->data.loop_count);
			} else {
				hid_info(hdev, "HID debug: len %ld malformed cmd 0x%02x\n", len,
					 buf[0]);
			}
			break;
		default:
			hid_info(hdev, "HID debug: len %ld unhandled cmd 0x%02x\n", len, buf[0]);
		}
	}
	return hid_hw_output_report(hdev, buf, len);
}

void xpadneo_core_missing(struct xpadneo_devdata *xdata, u32 flag)
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

static int xpadneo_core_init_hw(struct hid_device *hdev)
{
	int ret;
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	if (!xdata->gamepad) {
		xpadneo_core_missing(xdata, XPADNEO_MISSING_GAMEPAD);
		return -EINVAL;
	}

	ret = xpadneo_power_init(xdata);
	if (ret)
		goto err_free_name;

	ret = xpadneo_quirks_init(xdata);
	if (ret)
		goto err_free_name;

	return 0;

err_free_name:
	xpadneo_power_remove(xdata);
	return ret;
}

static void xpadneo_core_release_device_id(struct xpadneo_devdata *xdata)
{
	if (xdata->id >= 0) {
		ida_free(&xpadneo_core_device_id_allocator, xdata->id);
		xdata->id = -1;
	}
}

static void xpadneo_core_remove(struct hid_device *hdev)
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
	xpadneo_power_remove(xdata);

	xpadneo_core_release_device_id(xdata);
	hid_hw_stop(hdev);
}

static int xpadneo_core_probe(struct hid_device *hdev, const struct hid_device_id *id)
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

	if (hdev->version == 0x00000903)
		hid_warn(hdev, "buggy firmware detected, please upgrade to the latest version\n");
	else if (hdev->version < 0x00000500)
		hid_warn(hdev,
			 "classic Bluetooth firmware version %x.%02x, please upgrade for better stability\n",
			 hdev->version >> 8, (u8)hdev->version);
	else if (hdev->version < 0x00000512)
		hid_warn(hdev,
			 "BLE firmware version %x.%02x, please upgrade for better stability\n",
			 hdev->version >> 8, (u8)hdev->version);
	else
		hid_info(hdev, "BLE firmware version %x.%02x\n",
			 hdev->version >> 8, (u8)hdev->version);

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
		return ret;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret) {
		hid_err(hdev, "hw start failed\n");
		return ret;
	}

	ret = xpadneo_init_consumer(xdata);
	if (ret)
		return ret;

	ret = xpadneo_init_keyboard(xdata);
	if (ret)
		return ret;

	ret = xpadneo_mouse_init(xdata);
	if (ret)
		return ret;

	ret = xpadneo_core_init_hw(hdev);
	if (ret) {
		hid_err(hdev, "hw init failed: %d\n", ret);
		hid_hw_stop(hdev);
		return ret;
	}

	ret = xpadneo_rumble_init(hdev);
	if (ret)
		hid_err(hdev, "could not initialize ff, continuing anyway\n");

	xpadneo_mouse_init_timer(xdata);

	hid_info(hdev, "%s connected\n", xdata->battery.name);

	return 0;
}

void xpadneo_core_report(struct hid_device *hdev, struct hid_report *report)
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

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
static u8 *xpadneo_core_report_fixup(struct hid_device *hdev, u8 *rdesc, unsigned int *rsize)
#else
static const __u8 *xpadneo_core_report_fixup(struct hid_device *hdev, __u8 *rdesc,
					     unsigned int *rsize)
#endif
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	xdata->original_rsize = *rsize;
	hid_info(hdev, "report descriptor size: %d bytes\n", *rsize);

	/* fixup trailing NUL byte */
	if (rdesc[*rsize - 2] == 0xC0 && rdesc[*rsize - 1] == 0x00) {
		hid_notice(hdev, "fixing up report descriptor size\n");
		*rsize -= 1;
	}

	/* fixup reported axes for Xbox One S */
	if (*rsize >= 81) {
		if (rdesc[34] == 0x09 && rdesc[35] == 0x32) {
			hid_notice(hdev, "fixing up Rx axis\n");
			rdesc[35] = 0x33;	/* Z --> Rx */
		}
		if (rdesc[36] == 0x09 && rdesc[37] == 0x35) {
			hid_notice(hdev, "fixing up Ry axis\n");
			rdesc[37] = 0x34;	/* Rz --> Ry */
		}
		if (rdesc[52] == 0x05 && rdesc[53] == 0x02 &&
		    rdesc[54] == 0x09 && rdesc[55] == 0xC5) {
			hid_notice(hdev, "fixing up Z axis\n");
			rdesc[53] = 0x01;	/* Simulation -> Gendesk */
			rdesc[55] = 0x32;	/* Brake -> Z */
		}
		if (rdesc[77] == 0x05 && rdesc[78] == 0x02 &&
		    rdesc[79] == 0x09 && rdesc[80] == 0xC4) {
			hid_notice(hdev, "fixing up Rz axis\n");
			rdesc[78] = 0x01;	/* Simulation -> Gendesk */
			rdesc[80] = 0x35;	/* Accelerator -> Rz */
		}
	}

	/* fixup reported button count for Xbox controllers in Linux mode */
	if (*rsize >= 164) {
		/*
		 * 12 buttons instead of 10: properly remap the
		 * Xbox button (button 11)
		 * Share button (button 12)
		 */
		if (rdesc[140] == 0x05 && rdesc[141] == 0x09 &&
		    rdesc[144] == 0x29 && rdesc[145] == 0x0F &&
		    rdesc[152] == 0x95 && rdesc[153] == 0x0F &&
		    rdesc[162] == 0x95 && rdesc[163] == 0x01) {
			hid_notice(hdev, "fixing up button mapping\n");
			xdata->quirks |= XPADNEO_QUIRK_LINUX_BUTTONS;
			rdesc[145] = 0x0C;	/* 15 buttons -> 12 buttons */
			rdesc[153] = 0x0C;	/* 15 bits -> 12 bits buttons */
			rdesc[163] = 0x04;	/* 1 bit -> 4 bits constants */
		}
	}

	return rdesc;
}

static struct hid_driver xpadneo_driver = {
	.name = "xpadneo",
	.event = xpadneo_events_event,
	.id_table = xpadneo_devices,
	.input_configured = xpadneo_events_input_configured,
	.input_mapping = xpadneo_mapping_input,
	.probe = xpadneo_core_probe,
	.remove = xpadneo_core_remove,
	.report = xpadneo_core_report,
	.report_fixup = xpadneo_core_report_fixup,
	.raw_event = xpadneo_events_raw_event,
};

static int __init xpadneo_core_init(void)
{
	int ret;

	pr_info("loaded hid-xpadneo %s\n", XPADNEO_VERSION);
	dbg_hid("xpadneo:%s\n", __func__);

	ret = xpadneo_rumble_init_workqueue();
	if (!ret) {
		ret = hid_register_driver(&xpadneo_driver);
		if (ret)
			xpadneo_rumble_destroy_workqueue();
	}

	return ret;
}

static void __exit xpadneo_core_exit(void)
{
	dbg_hid("xpadneo:%s\n", __func__);
	hid_unregister_driver(&xpadneo_driver);
	ida_destroy(&xpadneo_core_device_id_allocator);
	xpadneo_rumble_destroy_workqueue();
}

module_init(xpadneo_core_init);
module_exit(xpadneo_core_exit);
