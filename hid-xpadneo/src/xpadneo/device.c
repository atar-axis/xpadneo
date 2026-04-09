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
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct xpadneo_rumble_report *r = (struct xpadneo_rumble_report *)buf;

	xpadneo_debug_hid_report(hdev, r, len);

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

/*
 * Third-party controllers that connect in Xbox BT mode, spoofing as 045E:02E0
 * (Xbox One S). Identified by device name since descriptor layout varies
 * between models and cannot be used as a reliable fingerprint.
 * The real Xbox One S reports "Xbox Wireless Controller" and is not listed.
 */
static const char * const xpadneo_02e0_spoof_names[] = {
	"8BitDo Pro 2",
	"8Bitdo SN30 Pro",
	"8Bitdo SF30 Pro",
	"8BitDo Zero 2 gamepad",
	"8BitDo M30 gamepad",
	"Gulikit Controller XW",
	NULL,
};

const __u8 *xpadneo_device_report_fixup(struct hid_device *hdev, __u8 *rdesc, unsigned int *rsize)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	/* preserve the original descriptor size for post-parse quirk heuristics */
	xdata->original_rsize = *rsize;

	/* log size/CRC and optionally hex-dump before any in-place patches */
	xpadneo_debug_descriptor(hdev, rdesc, *rsize);

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
		if (rdesc[140] == 0x05 && rdesc[141] == 0x09 &&
		    rdesc[144] == 0x29 && rdesc[145] == 0x0F &&
		    rdesc[152] == 0x95 && rdesc[153] == 0x0F &&
		    rdesc[162] == 0x95 && rdesc[163] == 0x01) {
			/*
			 * Xbox Elite Series 2 BLE (0x0B22): Steam Input has no database
			 * entry for this controller's BLE firmware layout, so it falls back
			 * to generic sequential mapping which misreads the non-sequential
			 * firmware button order. Enable LINUX_BUTTONS remapping so raw_event
			 * reorders the button bits sequentially for both Steam Input and
			 * native Linux.
			 *
			 * Do NOT modify the descriptor bytes here. The XBE2 has a larger
			 * descriptor than the Series X|S (extra paddle/profile fields), so
			 * the byte offsets 145/153/163 land in different places and would
			 * corrupt the rumble output report definition. The 15-button
			 * descriptor with 12 real sequential bits + 3 always-zero trailing
			 * bits is harmless.
			 */
			if (hdev->product == 0x0B22) {
				hid_notice(hdev, "fixing up XBE2 button mapping\n");
				xdata->quirks |= XPADNEO_QUIRK_LINUX_BUTTONS;
			}

			/*
			 * All controllers: apply button remapping + descriptor fixup when
			 * PID spoofing is enabled (enable_pid_spoof=1) for legacy SDL2
			 * (<2.28) compatibility via the Xbox 360 protocol.
			 */
			if (xpadneo_param_enable_pid_spoof()) {
				hid_notice(hdev, "fixing up button mapping (PID spoof mode)\n");
				xdata->quirks |= XPADNEO_QUIRK_LINUX_BUTTONS;
				rdesc[145] = 0x0C;	/* 15 buttons -> 12 buttons */
				rdesc[153] = 0x0C;	/* 15 bits -> 12 bits buttons */
				rdesc[163] = 0x04;	/* 1 bit -> 4 bits constants */
			}
		}
	}

	/*
	 * Xbox Series X|S BLE (0x0B13): same button hole at HID usage 0x90003
	 * (bit 2 of the raw report) as XBE2 BLE (0x0B22). The physical X button
	 * sends usage 0x90004 and physical Y sends 0x90005, shifting all buttons
	 * from X onward by one slot. Without LINUX_BUTTONS reordering, evdev
	 * applications see X→BTN_Y, Y→BTN_TL, LB→BTN_TR, etc.
	 *
	 * Steam Input reads hidraw directly (before raw_event processing), so
	 * enabling LINUX_BUTTONS fixes evdev without affecting Steam HIDAPI.
	 * MENU_GHOST handling is automatically bypassed when LINUX_BUTTONS is set.
	 *
	 * This check is outside the descriptor byte-check block above because
	 * Series X|S has 12 buttons (0x0C) and that block matches 15 (0x0F).
	 */
	if (hdev->product == 0x0B13) {
		hid_notice(hdev, "fixing up Xbox Series X|S button mapping\n");
		xdata->quirks |= XPADNEO_QUIRK_LINUX_BUTTONS;
	}

	/*
	 * 8BitDo controllers in Xbox BT mode (PID 0x02E0, shared with the real
	 * Xbox One S): SDL3's HIDAPI Xbox One driver claims any 045E:02E0 device
	 * via Bluetooth and applies a built-in mapping with guide:b10,leftstick:b8,
	 * rightstick:b9 — designed for the real Xbox One S which exposes no Guide
	 * on the gamepad device. These 8BitDo controllers' Guide button (0x10085,
	 * inside the Gamepad Application Collection) maps naturally to BTN_MODE
	 * (316), which sorts before BTN_THUMBL (317) and BTN_THUMBR (318), making
	 * the real layout guide:b8,leftstick:b9,rightstick:b10. HIDAPI cannot be
	 * tricked by descriptor changes since it reads hidraw directly.
	 *
	 * Spoof as Xbox One model 1697 (0x02DD): this was a USB-only controller
	 * with no wireless capability, so SDL3 HIDAPI has no Xbox One BT driver
	 * path that would claim 045E:02DD via Bluetooth. SDL3 falls back to evdev
	 * and auto-detects by key code: BTN_MODE→guide, BTN_THUMBL→leftstick,
	 * BTN_THUMBR→rightstick — correct for our layout. Steam and other apps
	 * still identify it as an Xbox One controller.
	 *
	 * Detection is name-based (see xpadneo_02e0_spoof_names[]) so the real
	 * Xbox One S ("Xbox Wireless Controller") is never affected.
	 */
	if (hdev->product == 0x02E0) {
		int i;

		for (i = 0; xpadneo_02e0_spoof_names[i]; i++) {
			if (!strcmp(hdev->name, xpadneo_02e0_spoof_names[i]))
				break;
		}
		if (xpadneo_02e0_spoof_names[i]) {
			hid_notice(hdev, "%s detected, spoofing as Xbox One 1697 (0x02DD) to bypass SDL3 HIDAPI\n",
				   hdev->name);
			xdata->quirks |= XPADNEO_QUIRK_NO_GUIDE_BTN;

			/*
			 * 8BitDo M30: X and Y face buttons are physically swapped
			 * relative to the standard Xbox layout. Swap bits 2 and 3
			 * of data[14] in raw_event to correct BTN_NORTH ↔ BTN_WEST.
			 */
			if (!strcmp(hdev->name, "8BitDo M30 gamepad"))
				xdata->quirks |= XPADNEO_QUIRK_SWAP_XY;

			/*
			 * Gulikit Controller XW: does not implement the Xbox rumble
			 * protocol correctly — sending rumble commands causes the
			 * motors to latch on indefinitely at connect time. Disable
			 * all haptics to prevent this.
			 */
			if (!strcmp(hdev->name, "Gulikit Controller XW"))
				xdata->quirks |= XPADNEO_QUIRK_SIMPLE_CLONE;

			hdev->product = 0x02DD;
		}
	}

	return rdesc;
}
