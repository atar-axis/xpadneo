// SPDX-License-Identifier: GPL-2.0-only

/*
 * xpadneo HID descriptor debug helpers
 *
 * Logs descriptor size and CRC-16 checksum unconditionally, and dumps the
 * full unpatched HID descriptor (including OUI flags) automatically for
 * locally-administered (LAA) addresses or when the debug_descriptor module
 * parameter is set.
 *
 * Copyright (c) 2026 Kai Krakow <kai@kaishome.de>
 */

#include <linux/crc16.h>
#include <linux/hid.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/types.h>

#include "xpadneo.h"

static bool param_debug_descriptor;
module_param_named(debug_descriptor, param_debug_descriptor, bool, 0644);
MODULE_PARM_DESC(debug_descriptor,
		 "(bool) Dump unpatched HID descriptor to dmesg. 0: auto, 1: always.");

static bool param_debug_hid;
module_param_named(debug_hid, param_debug_hid, bool, 0644);
MODULE_PARM_DESC(debug_hid, "(bool) Debug HID reports. 0: disable, 1: enable.");

struct crc_name {
	u16 crc16;
	char *name;
};

const struct crc_name known_checksums[] = {
	{ .crc16 = 0x534B, .name = "GuliKit ES PRO E-Sports Controller" },
	{ .crc16 = 0x6500, .name = "Xbox One Elite Series 2" },
	{ .crc16 = 0x8BC5, .name = "Xbox Wireless Controller (legacy)" },
	{ .crc16 = 0x931D, .name = "Xbox Wireless Controller (modern)" },
};

/*
 * Parse the first three OUI bytes from a Bluetooth MAC address string of
 * the form "aa:bb:cc:dd:ee:ff".  Returns 0 on success, -EINVAL otherwise.
 */
static int parse_oui(const char *uniq, u8 *b0, u8 *b1, u8 *b2)
{
	char tmp[3] = { };

	if (!uniq)
		return -EINVAL;

	/* need at least "XX:XX:XX" (8 chars) with ':' separators */
	if (strnlen(uniq, 9) < 8 || uniq[2] != ':' || uniq[5] != ':')
		return -EINVAL;

	memcpy(tmp, uniq + 0, 2);
	if (kstrtou8(tmp, 16, b0))
		return -EINVAL;

	memcpy(tmp, uniq + 3, 2);
	if (kstrtou8(tmp, 16, b1))
		return -EINVAL;

	memcpy(tmp, uniq + 6, 2);
	if (kstrtou8(tmp, 16, b2))
		return -EINVAL;

	return 0;
}

void xpadneo_debug_hid_report(const struct hid_device *hdev, const void *buf, const size_t len)
{
	u8 report_id;

	if (likely(!param_debug_hid))
		return;

	if (unlikely(len == 0)) {
		hid_err(hdev, "HID debug: invalid length %zu\n", len);
		return;
	}

	report_id = ((const u8 *)buf)[0];
	switch (report_id) {
	case 0x03:
		if (unlikely(len != sizeof(struct xpadneo_rumble_report))) {
			hid_info(hdev, "HID debug: len %zu malformed cmd 0x%02x\n", len, report_id);
		} else {
			const struct xpadneo_rumble_report *r = buf;
			hid_info(hdev,
				 "HID debug: rumble cmd 0x%02x "
				 "motors left %d right %d strong %d weak %d "
				 "magnitude left %d right %d strong %d weak %d "
				 "pulse sustain %dms release %dms loop %d\n",
				 r->report_id,
				 !!(r->data.enable & XBOX_RUMBLE_LEFT),
				 !!(r->data.enable & XBOX_RUMBLE_RIGHT),
				 !!(r->data.enable & XBOX_RUMBLE_STRONG),
				 !!(r->data.enable & XBOX_RUMBLE_WEAK),
				 r->data.magnitude_left, r->data.magnitude_right,
				 r->data.magnitude_strong, r->data.magnitude_weak,
				 r->data.pulse_sustain_10ms * 10,
				 r->data.pulse_release_10ms * 10, r->data.loop_count);
			return;
		}
		break;
	default:
		hid_info(hdev, "HID debug: unhandled cmd 0x%02x\n", report_id);
	}

	print_hex_dump(KERN_INFO, "xpadneo hid-report: ", DUMP_PREFIX_OFFSET, 32, 1, buf, len,
		       false);
}

/*
 * xpadneo_debug_descriptor - log descriptor metadata and optionally hex-dump
 * @hdev:   HID device whose descriptor is being examined
 * @rdesc:  pointer to the *unpatched* report descriptor bytes
 * @rsize:  length of the descriptor in bytes
 *
 * Must be called at the very start of report_fixup, before any in-place
 * modifications, so that the logged data reflects the device's original
 * descriptor.
 *
 * Unconditionally emits one line containing the descriptor byte-length and
 * CRC-16 checksum (CRC-16/IBM, poly 0x8005, init 0), plus one line
 * describing the OUI flags.
 *
 * Additionally performs a full hex-dump when:
 *   - the module parameter debug_descriptor=1 is set, OR
 *   - the device's Bluetooth OUI is locally administered (LAA).
 */
void xpadneo_debug_descriptor(const struct hid_device *hdev, const __u8 *rdesc, unsigned int rsize)
{
	u8 oui0 = 0, oui1 = 0, oui2 = 0;
	u16 crc = crc16(0, rdesc, rsize);
	bool oui_valid = (parse_oui(hdev->uniq, &oui0, &oui1, &oui2) == 0);
	bool is_laa = !!(oui0 & XPADNEO_OUI_IS_LAA);
	bool is_multicast = !!(oui0 & XPADNEO_OUI_IS_MULTICAST);
	bool do_dump;

	hid_info(hdev, "report descriptor: length %u crc16 0x%04x\n", rsize, crc);

	if (oui_valid) {
		hid_info(hdev,
			 "report descriptor: OUI %02X:%02X:%02X (0x%02X) - %s, %s\n",
			 oui0, oui1, oui2, oui0,
			 is_laa ? "locally-administered (LAA)" : "globally-assigned",
			 is_multicast ? "multicast" : "unicast");
	} else {
		hid_info(hdev, "report descriptor: OUI unavailable (uniq='%.17s')\n", hdev->uniq);
	}

	do_dump = param_debug_descriptor;
	if (!do_dump) {
		for (int i = 0; i < ARRAY_SIZE(known_checksums); i++) {
			if (crc == known_checksums[i].crc16) {
				hid_info(hdev,
					 "report descriptor: known checksum crc16 0x%04x name '%s'\n",
					 crc, known_checksums[i].name);
				break;
			}
		}
	}

	do_dump = param_debug_descriptor || (oui_valid && is_laa);
	if (do_dump) {
		hid_info(hdev,
			 "report descriptor: dumping %u unpatched bytes crc16 0x%04x%s\n", rsize,
			 crc, param_debug_descriptor ? " [forced]" : " [LAA OUI]");
		print_hex_dump(KERN_INFO, "xpadneo hid-desc: ", DUMP_PREFIX_OFFSET, 32, 1, rdesc,
			       rsize, false);
	}
}
