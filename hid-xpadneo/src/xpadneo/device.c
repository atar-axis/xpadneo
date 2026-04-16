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
	"8BitDo SN30 Pro+",
	"8Bitdo SN30 Pro",
	"8Bitdo SF30 Pro",
	"8BitDo Zero 2 gamepad",
	"8BitDo M30 gamepad",
	"GuliKit Controller XW",
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
	} else if (rdesc[*rsize - 2] == 0x00 && rdesc[*rsize - 1] == 0xC0) {
		/*
		 * The firmware wrote 0x00 where End Collection (0xC0) was needed,
		 * then followed with a correct 0xC0.  Replace the 0x00 with 0xC0
		 * so the descriptor ends with two End Collections.  Do NOT shrink
		 * the descriptor: both bytes are valid End Collection items and are
		 * needed to close the two open collections.
		 */
		hid_notice(hdev, "fixing up report descriptor NUL before end collection\n");
		rdesc[*rsize - 2] = 0xC0;
	}

	/*
	 * Xbox One S 1708 (PID 0x02FD, fw 0x0903): the firmware sends a
	 * truncated 307-byte descriptor.  The rumble report (ID 3) definition
	 * is cut off mid-item after "Usage(Loop Count) / Logical Minimum(0)";
	 * the last three bytes are 15 00 00 instead of the expected output
	 * item and two End Collections.  The HID parser hits the trailing 0x00,
	 * treats it as an unknown Main item (tag 0x0), aborts, then reports
	 * "unbalanced collection" because the two open collections (Application
	 * and Logical) were never closed.
	 *
	 * Replace the dead "Logical Minimum(0)" item and trailing NUL with two
	 * End Collections and shrink the descriptor by one byte.  The incomplete
	 * Loop Count Output field is dropped, but xpadneo sends the rumble report
	 * as a fixed raw struct so the HID descriptor definition of that field
	 * is not required for correct operation.
	 */
	if (*rsize == 307 &&
	    rdesc[302] == 0x09 && rdesc[303] == 0x7C &&
	    rdesc[304] == 0x15 && rdesc[305] == 0x00 && rdesc[306] == 0x00) {
		hid_notice(hdev, "fixing up truncated report descriptor\n");
		rdesc[304] = 0xC0;	/* End Collection (closes Logical collection) */
		rdesc[305] = 0xC0;	/* End Collection (closes Application collection) */
		*rsize = 306;
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
			 * Xbox controllers in Linux/Android BT mode expose a
			 * non-sequential button layout: HID usage 0x90003 is an
			 * empty hole (never asserted by the firmware); the physical
			 * X button lives at usage 0x90004, Y at 0x90005, and so on.
			 * This shifts every button from X onward by one slot relative
			 * to what software expects from a sequential 1-N mapping.
			 *
			 * Enable LINUX_BUTTONS so raw_event reorders the bits into a
			 * clean sequential layout for both Steam Input (which reads
			 * hidraw directly) and native Linux evdev consumers.
			 *
			 * 0x0B05 also sets LINUX_BUTTONS unconditionally via driver_data
			 * in case its classic-BT descriptor does not hit this byte
			 * pattern (the BT and BLE descriptors have different layouts).
			 *
			 * 0x0B22 (Elite Series 2 BLE) sets LINUX_BUTTONS here when its
			 * 15-button descriptor pattern is detected. It also carries
			 * SHARE_BUTTON in driver_data so raw_event reads Back from the
			 * correct byte position alongside the Share button.
			 *
			 * 0x02FD (Xbox One S 1708) does NOT need LINUX_BUTTONS: its HID
			 * descriptor already describes the correct sequential mapping and
			 * the kernel processes it without raw_event intervention.
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
	 * Xbox Series X|S BLE (0x0B13) uses a 12-button descriptor that does
	 * not match the 15-button byte pattern checked above, so it needs an
	 * unconditional check here.  Like other Xbox Linux-mode controllers it
	 * has the non-sequential button layout with holes at usages 0x90003,
	 * 0x90006, 0x90009, 0x9000A.
	 *
	 * 0x0B22 (Elite Series 2 BLE) is handled above by the 15-button
	 * byte-pattern check.  0x02FD (Xbox One S 1708) does not need
	 * LINUX_BUTTONS: its descriptor already describes a sequential
	 * button layout that the kernel maps correctly without raw_event
	 * intervention.
	 */
	if (hdev->product == 0x0B13) {
		hid_notice(hdev, "fixing up button mapping\n");
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
	 * Detection is name- or OUI-based so the real Xbox One S
	 * ("Xbox Wireless Controller", Microsoft OUI) is never affected.
	 *
	 * This spoof path is architecturally separate from enable_pid_spoof=1:
	 * ┌────────────┬──────────────────────────┬───────────────────────────┐
	 * │            │    enable_pid_spoof=1    │   3rd party 02E0 → 02DD   │
	 * ├────────────┼──────────────────────────┼───────────────────────────┤
	 * │ PID target │ 028E (Xbox 360)          │ 02DD (Xbox One 1697)      │
	 * ├────────────┼──────────────────────────┼───────────────────────────┤
	 * │ Descriptor │ Modified (15→12 buttons) │ Native (untouched)        │
	 * ├────────────┼──────────────────────────┼───────────────────────────┤
	 * │ Goal       │ Legacy SDL2 <2.28 compat │ SDL3 HIDAPI bypass        │
	 * ├────────────┼──────────────────────────┼───────────────────────────┤
	 * │ Scope      │ All controllers          │ Named/OUI 3rd party only  │
	 * └────────────┴──────────────────────────┴───────────────────────────┘
	 */
	if (hdev->product == 0x02E0) {
		/*
		 * GameSir controllers register themselves as "Xbox Wireless Controller"
		 * with PID 02E0, making them indistinguishable from the real Xbox One S
		 * by name. Use OUI to identify them and apply the 02DD spoof.
		 */
		if (strncasecmp("A0:5A:5D", hdev->uniq, 8) == 0) {
			hid_notice(hdev, "GameSir (OUI A0:5A:5D) detected, spoofing as Xbox One 1697 (0x02DD)\n");
			xdata->quirks |= XPADNEO_QUIRK_NO_HAPTICS;
			hdev->product = 0x02DD;
		} else if (strncasecmp("E4:17:D8", hdev->uniq, 8) == 0) {
			hid_notice(hdev, "GameSir (OUI E4:17:D8) detected, spoofing as Xbox One 1697 (0x02DD)\n");
			xdata->quirks |= XPADNEO_QUIRK_SIMPLE_CLONE;
			hdev->product = 0x02DD;
		}

		/* The rest we can identify by name then fine-tune by device mac */
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
			 * GuliKit Controller XW: all share the same device name but
			 * have different hardware revisions requiring different quirks,
			 * identified by MAC address OUI.
			 */
			if (!strcmp(hdev->name, "GuliKit Controller XW")) {
				if (strncasecmp("98:B6:EA", hdev->uniq, 8) == 0)
					xdata->quirks |= XPADNEO_QUIRK_NO_PULSE |
							 XPADNEO_QUIRK_NO_TRIGGER_RUMBLE |
							 XPADNEO_QUIRK_REVERSE_MASK;
				else if (strncasecmp("98:B6:EC", hdev->uniq, 8) == 0)
					xdata->quirks |= XPADNEO_QUIRK_SIMPLE_CLONE |
							 XPADNEO_QUIRK_SWAPPED_MASK;
				else
					/*
					 * Unknown GuliKit hardware revision: apply a safe
					 * default.  Without NO_PULSE the welcome sequence
					 * sends pulse timing params the controller ignores,
					 * leaving all motors running indefinitely.
					 * SIMPLE_CLONE (NO_PULSE + NO_MOTOR_MASK +
					 * NO_TRIGGER_RUMBLE) sends a zero-magnitude stop
					 * packet and avoids trigger-motor commands that
					 * clone hardware typically does not support.
					 */
					xdata->quirks |= XPADNEO_QUIRK_SIMPLE_CLONE;
			}

			/* Force all devices to be recognized as "Xbox Wireless Controller" */
			strscpy(hdev->name, "Xbox Wireless Controller", sizeof(hdev->name));
			hdev->product = 0x02DD;
		}
	}

	return rdesc;
}
