// SPDX-License-Identifier: GPL-2.0-only

/*
 * xpadneo core device helpers
 *
 * Copyright (c) 2021 Kai Krakow <kai@kaishome.de>
 */

#include <linux/delay.h>
#include <linux/module.h>

#include "xpadneo.h"

int xpadneo_device_output_report(struct hid_device *hdev, __u8 *buf, size_t len, bool uses_hogp)
{
	struct xpadneo_rumble_report *r = (struct xpadneo_rumble_report *)buf;

	xpadneo_debug_hid_report(hdev, buf, len);

	/*
	 * Some BLE controllers (e.g. XBE2 0x0B22) require a GATT Write Request
	 * (acknowledged) rather than the unacknowledged GATT Write Command that
	 * hid_hw_output_report sends. Use hid_hw_raw_request with HID_OUTPUT_REPORT
	 * and HID_REQ_SET_REPORT to force the acknowledged path for these devices.
	 *
	 * For non-HOGP devices, add a 20ms delay after the report is sent to
	 * give the device time to process the report before the next one is sent.
	 */
	if (uses_hogp) {
		return hid_hw_raw_request(hdev, r->report_id, buf, len,
					  HID_OUTPUT_REPORT, HID_REQ_SET_REPORT);
	} else {
		int ret = hid_hw_output_report(hdev, buf, len);

		msleep(20);
		return ret;
	}
}

void xpadneo_device_missing(struct xpadneo_devdata *xdata, u32 flag)
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

static inline void sync_device(struct xpadneo_subdevice *subdev)
{
	if (subdev->idev && subdev->sync) {
		subdev->sync = false;
		input_sync(subdev->idev);
	}
}

void xpadneo_device_report(struct hid_device *hdev, struct hid_report *report)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	sync_device(&xdata->consumer);
	sync_device(&xdata->gamepad);
	sync_device(&xdata->keyboard);
	sync_device(&xdata->mouse);
}

const __u8 *xpadneo_device_report_fixup(struct hid_device *hdev, __u8 *rdesc, unsigned int *rsize)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	/* preserve the original descriptor size for post-parse quirk heuristics */
	xdata->original_rsize = *rsize;

	/* log size/CRC and optionally hex-dump before any in-place patches */
	xpadneo_debug_descriptor(hdev, rdesc, *rsize);

	/* fixup trailing NUL byte */
	if (*rsize >= 2 && rdesc[*rsize - 2] == 0xC0 && rdesc[*rsize - 1] == 0x00) {
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
