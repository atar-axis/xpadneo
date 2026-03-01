// SPDX-License-Identifier: GPL-3.0-only

/*
 * xpadneo driver quirks handling
 *
 * Copyright (c) 2026 Kai Krakow <kai@kaishome.de>
 */

#include <linux/module.h>

#include "../xpadneo.h"

static struct {
	char *args[17];
	unsigned int nargs;
} param_quirks;
module_param_array_named(quirks, param_quirks.args, charp, &param_quirks.nargs, 0644);
MODULE_PARM_DESC(quirks,
		 "(string) Override or change device quirks, specify as: \"MAC1{:,+,-}quirks1[,...16]\""
		 ", MAC format = 11:22:33:44:55:66"
		 ", no pulse parameters = " __stringify(XPADNEO_QUIRK_NO_PULSE)
		 ", no trigger rumble = " __stringify(XPADNEO_QUIRK_NO_TRIGGER_RUMBLE)
		 ", no motor masking = " __stringify(XPADNEO_QUIRK_NO_MOTOR_MASK)
		 ", use Linux button mappings = " __stringify(XPADNEO_QUIRK_LINUX_BUTTONS)
		 ", use Nintendo mappings = " __stringify(XPADNEO_QUIRK_NINTENDO)
		 ", use Share button mappings = " __stringify(XPADNEO_QUIRK_SHARE_BUTTON)
		 ", reversed motor masking = " __stringify(XPADNEO_QUIRK_REVERSE_MASK)
		 ", swapped motor masking = " __stringify(XPADNEO_QUIRK_SWAPPED_MASK)
		 ", apply no heuristics = " __stringify(XPADNEO_QUIRK_NO_HEURISTICS));

#define DEVICE_NAME_QUIRK(n, f) \
	{ .name_match = (n), .name_len = sizeof(n) - 1, .flags = (f) }
#define DEVICE_OUI_QUIRK(o, f) \
	{ .oui_match = (o), .flags = (f) }

struct quirk {
	char *name_match;
	char *oui_match;
	u16 name_len;
	u32 flags;
};

static const struct quirk xpadneo_quirks[] = {
	DEVICE_OUI_QUIRK("28:EA:0B", XPADNEO_QUIRK_NO_HEURISTICS),
	DEVICE_OUI_QUIRK("3C:FA:06", XPADNEO_QUIRK_NO_HEURISTICS),
	DEVICE_OUI_QUIRK("68:6C:E6", XPADNEO_QUIRK_NO_HEURISTICS),
	DEVICE_OUI_QUIRK("78:86:2E", XPADNEO_QUIRK_NO_HEURISTICS),
	DEVICE_OUI_QUIRK("98:B6:EA",
			 XPADNEO_QUIRK_NO_PULSE | XPADNEO_QUIRK_NO_TRIGGER_RUMBLE |
			 XPADNEO_QUIRK_REVERSE_MASK),
	DEVICE_OUI_QUIRK("98:B6:EC",
			 XPADNEO_QUIRK_SIMPLE_CLONE | XPADNEO_QUIRK_SWAPPED_MASK),
	DEVICE_OUI_QUIRK("A0:5A:5D", XPADNEO_QUIRK_NO_HAPTICS),
	DEVICE_OUI_QUIRK("A8:8C:3E", XPADNEO_QUIRK_NO_HEURISTICS),
	DEVICE_OUI_QUIRK("AC:8E:BD", XPADNEO_QUIRK_NO_HEURISTICS),
	DEVICE_OUI_QUIRK("E4:17:D8", XPADNEO_QUIRK_SIMPLE_CLONE),
	DEVICE_OUI_QUIRK("EC:83:50", XPADNEO_QUIRK_NO_HEURISTICS),
};

int xpadneo_quirks_init(struct xpadneo_devdata *xdata)
{
	struct hid_device *hdev = xdata->hdev;
	u32 quirks_set = 0, quirks_unset = 0, quirks_override = U32_MAX;
	u8 oui_byte;
	char oui[3] = { };

	for (int i = 0; i < ARRAY_SIZE(xpadneo_quirks); i++) {
		const struct quirk *q = &xpadneo_quirks[i];

		if (q->name_match
		    && (strncmp(q->name_match, xdata->gamepad->name, q->name_len) == 0))
			xdata->quirks |= q->flags;

		if (q->oui_match && (strncasecmp(q->oui_match, xdata->gamepad->uniq, 8) == 0))
			xdata->quirks |= q->flags;
	}

	kernel_param_lock(THIS_MODULE);
	for (int i = 0; i < param_quirks.nargs; i++) {
		int offset = strnlen(xdata->gamepad->uniq, 18);

		if ((strncasecmp(xdata->gamepad->uniq, param_quirks.args[i], offset) == 0)
		    && ((param_quirks.args[i][offset] == ':')
			|| (param_quirks.args[i][offset] == '+')
			|| (param_quirks.args[i][offset] == '-'))) {
			char *quirks_arg = &param_quirks.args[i][offset + 1];
			u32 quirks = 0;
			int ret = kstrtou32(quirks_arg, 0, &quirks);

			if (ret) {
				hid_err(hdev, "quirks override invalid: %s\n", quirks_arg);
				return -EINVAL;
			} else if (param_quirks.args[i][offset] == ':') {
				quirks_override = quirks;
			} else if (param_quirks.args[i][offset] == '-') {
				quirks_unset = quirks;
			} else {
				quirks_set = quirks;
			}
			break;
		}
	}
	kernel_param_unlock(THIS_MODULE);

	/* handle quirk flags which override a behavior before heuristics */
	if (quirks_override != U32_MAX) {
		hid_info(hdev, "quirks override: %s\n", xdata->gamepad->uniq);
		xdata->quirks = quirks_override;
	}

	/* handle quirk flags which add a behavior before heuristics */
	if (quirks_set > 0) {
		hid_info(hdev, "quirks added: %s flags 0x%08X\n", xdata->gamepad->uniq, quirks_set);
		xdata->quirks |= quirks_set;
	}

	/*
	 * copy the first two characters from the uniq ID (MAC address) and
	 * expect it being too big to copy, then `kstrtou8()` converts the
	 * uniq ID "aa:bb:cc:dd:ee:ff" to u8, so we get the first OUI byte
	 */
	if ((xdata->original_rsize == 283)
	    && ((xdata->quirks & XPADNEO_QUIRK_NO_HEURISTICS) == 0)
	    && ((xdata->quirks & XPADNEO_QUIRK_SIMPLE_CLONE) == 0)
	    && (strscpy(oui, xdata->gamepad->uniq, sizeof(oui)) == -E2BIG)
	    && (kstrtou8(oui, 16, &oui_byte) == 0)
	    && XPADNEO_OUI_MASK(oui_byte, XPADNEO_OUI_MASK_GAMESIR_NOVA)) {
		hid_info(hdev, "enabling heuristic GameSir Nova quirks\n");
		xdata->quirks |= XPADNEO_QUIRK_SIMPLE_CLONE;
	}

	/* handle quirk flags which remove a behavior after heuristics */
	if (quirks_unset > 0) {
		hid_info(hdev, "quirks removed: %s flag 0x%08X\n", xdata->gamepad->uniq,
			 quirks_unset);
		xdata->quirks &= ~quirks_unset;
	}

	if (xdata->quirks > 0)
		hid_info(hdev, "controller quirks: 0x%08x\n", xdata->quirks);

	return 0;
}
