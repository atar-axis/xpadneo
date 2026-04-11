// SPDX-License-Identifier: GPL-2.0-only

/*
 * xpadneo driver quirks handling
 *
 * Copyright (c) 2026 Kai Krakow <kai@kaishome.de>
 */

#include <linux/module.h>

#include "xpadneo.h"

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

/* MAC OUI masks */
#define XPADNEO_OUI_MASK_LAA_MULTICAST (XPADNEO_OUI_IS_MULTICAST | XPADNEO_OUI_IS_LAA)

static const struct quirk quirks[] = {
	DEVICE_OUI_QUIRK("98:B6:EA",
			 XPADNEO_QUIRK_NO_PULSE | XPADNEO_QUIRK_NO_TRIGGER_RUMBLE |
			 XPADNEO_QUIRK_REVERSE_MASK),
	DEVICE_OUI_QUIRK("98:B6:EC",
			 XPADNEO_QUIRK_SIMPLE_CLONE | XPADNEO_QUIRK_SWAPPED_MASK),
	DEVICE_OUI_QUIRK("A0:5A:5D", XPADNEO_QUIRK_NO_HAPTICS),
	DEVICE_OUI_QUIRK("E4:17:D8", XPADNEO_QUIRK_SIMPLE_CLONE),
};

int xpadneo_quirks_init(struct xpadneo_devdata *xdata)
{
	struct hid_device *hdev = xdata->hdev;
	struct input_dev *gamepad = xdata->gamepad.idev;
	u32 quirks_set = 0, quirks_unset = 0, quirks_override = U32_MAX;
	u8 oui_byte = 0;
	char oui[3] = { };

	for (int i = 0; i < ARRAY_SIZE(quirks); i++) {
		const struct quirk *q = &quirks[i];

		if (q->name_match && gamepad->name
		    && (strncmp(q->name_match, gamepad->name, q->name_len) == 0))
			xdata->quirks |= q->flags;

		if (q->oui_match && gamepad->uniq
		    && (strncasecmp(q->oui_match, gamepad->uniq, 8) == 0))
			xdata->quirks |= q->flags;
	}

	kernel_param_lock(THIS_MODULE);
	for (int i = 0; gamepad->uniq && (i < param_quirks.nargs); i++) {
		const char *arg = param_quirks.args[i];
		size_t uniq_len = strnlen(gamepad->uniq, 18);
		size_t arg_len = strnlen(arg, 128);

		/* check if the argument is long enough to fetch uniq + a suffix */
		if (arg_len <= uniq_len) {
			hid_warn(hdev,
				 "quirks parameter '%s' ignored: invalid MAC or missing modifier\n",
				 arg);
			continue;
		}

		if (strncasecmp(gamepad->uniq, param_quirks.args[i], uniq_len) != 0)
			continue;

		if ((arg[uniq_len] != ':') && (arg[uniq_len] != '+') && (arg[uniq_len] != '-')) {
			hid_warn(hdev,
				 "quirks parameter '%s' ignored: invalid modifier '%c'\n",
				 arg, arg[uniq_len]);
			continue;
		}

		do {
			const char *quirks_arg = arg + uniq_len + 1;
			u32 quirks = 0;
			int ret = kstrtou32(quirks_arg, 0, &quirks);

			if (ret) {
				hid_err(hdev, "quirks override invalid: %s\n", quirks_arg);
				kernel_param_unlock(THIS_MODULE);
				return -EINVAL;
			} else if (arg[uniq_len] == ':') {
				quirks_override = quirks;
			} else if (arg[uniq_len] == '-') {
				quirks_unset = quirks;
			} else {
				quirks_set = quirks;
			}
		} while (0);

		break;
	}
	kernel_param_unlock(THIS_MODULE);

	/* handle quirk flags which override a behavior before heuristics */
	if (quirks_override != U32_MAX) {
		hid_info(hdev, "quirks override: %s\n", gamepad->uniq);
		xdata->quirks = quirks_override;
	}

	/* handle quirk flags which add a behavior before heuristics */
	if (quirks_set > 0) {
		hid_info(hdev, "quirks added: %s flags 0x%08X\n", gamepad->uniq, quirks_set);
		xdata->quirks |= quirks_set;
	}

	/*
	 * Check whether we should enable heuristics checks at all, and then
	 * copy the first two characters from the uniq ID (MAC address) and
	 * expect it being too big to copy, then `kstrtou8()` converts the
	 * uniq ID "aa:bb:cc:dd:ee:ff" to u8, so we get the first OUI byte.
	 */
	if (((xdata->quirks & XPADNEO_QUIRK_NO_HEURISTICS) == 0)
	    && ((xdata->quirks & XPADNEO_QUIRK_SIMPLE_CLONE) == 0)
	    && (strscpy(oui, gamepad->uniq, sizeof(oui)) == -E2BIG)
	    && (kstrtou8(oui, 16, &oui_byte) == 0)) {
		/*
		 * All known GameSir devices at least one of the LAA or
		 * multicast bits set, and a descriptor length of 283 or 306
		 * bytes.
		 */
		if (((xdata->original_rsize == 283) || (xdata->original_rsize == 306))
		    && ((oui_byte & XPADNEO_OUI_MASK_LAA_MULTICAST) > 0)) {
			hid_info(hdev, "enabling heuristic GameSir quirks\n");
			xdata->quirks |= XPADNEO_QUIRK_SIMPLE_CLONE;
		}
	}

	/* handle quirk flags which remove a behavior after heuristics */
	if (quirks_unset > 0) {
		hid_info(hdev, "quirks removed: %s flag 0x%08X\n", gamepad->uniq, quirks_unset);
		xdata->quirks &= ~quirks_unset;
	}

	if (xdata->quirks > 0)
		hid_info(hdev, "controller quirks: 0x%08x\n", xdata->quirks);

	return 0;
}

void xpadneo_quirks_remove(struct xpadneo_devdata *xdata)
{
	struct hid_device *hdev = xdata->hdev;

	if (xdata->quirks > 0)
		hid_info(hdev, "removing controller quirks: 0x%08x\n", xdata->quirks);
}
