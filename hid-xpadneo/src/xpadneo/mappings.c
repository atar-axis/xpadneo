// SPDX-License-Identifier: GPL-2.0-only

/*
 * xpadneo driver usage mappings
 *
 * Copyright (c) 2017 Florian Dollinger <dollinger.florian@gmx.de>
 * Copyright (c) 2026 Kai Krakow <kai@kaishome.de>
 */

#include "xpadneo.h"

/* always include last */
#include "compat.h"

struct usage_map {
	u32 usage;
	enum {
		MAP_IGNORE = -1,	/* Completely ignore this field */
		MAP_AUTO,	/* Do not really map it, let hid-core decide */
		MAP_STATIC	/* Map to the values given */
	} behaviour;
	struct {
		u8 event_type;	/* input event (EV_KEY, EV_ABS, ...) */
		u16 input_code;	/* input code (BTN_A, ABS_X, ...) */
	} ev;
};

#define USAGE_MAP(u, b, e, i) \
	{ .usage = (u), .behaviour = (b), .ev = { .event_type = (e), .input_code = (i) } }
#define USAGE_IGN(u) USAGE_MAP(u, MAP_IGNORE, 0, 0)

static const struct usage_map usage_maps[] = {
	/* fixup buttons to Linux codes */
	USAGE_MAP(0x90001, MAP_STATIC, EV_KEY, BTN_A),	/* A */
	USAGE_MAP(0x90002, MAP_STATIC, EV_KEY, BTN_B),	/* B */
	USAGE_MAP(0x90003, MAP_STATIC, EV_KEY, BTN_X),	/* X */
	USAGE_MAP(0x90004, MAP_STATIC, EV_KEY, BTN_Y),	/* Y */
	USAGE_MAP(0x90005, MAP_STATIC, EV_KEY, BTN_TL),	/* LB */
	USAGE_MAP(0x90006, MAP_STATIC, EV_KEY, BTN_TR),	/* RB */
	USAGE_MAP(0x90007, MAP_STATIC, EV_KEY, BTN_SELECT),	/* Back */
	USAGE_MAP(0x90008, MAP_STATIC, EV_KEY, BTN_START),	/* Menu */
	USAGE_MAP(0x90009, MAP_STATIC, EV_KEY, BTN_THUMBL),	/* LS */
	USAGE_MAP(0x9000A, MAP_STATIC, EV_KEY, BTN_THUMBR),	/* RS */

	/* fixup the Xbox logo button */
	USAGE_MAP(0x9000B, MAP_STATIC, EV_KEY, BTN_XBOX),	/* Xbox */

	/* fixup the Share button */
	USAGE_MAP(0x9000C, MAP_STATIC, EV_KEY, BTN_SHARE),	/* Share */

	/* fixup code "Sys Main Menu" from Windows report descriptor */
	USAGE_MAP(0x10085, MAP_STATIC, EV_KEY, BTN_XBOX),

	/* fixup code "AC Home" from Linux report descriptor */
	USAGE_MAP(0xC0223, MAP_STATIC, EV_KEY, BTN_XBOX),

	/* fixup code "AC Back" from Linux report descriptor */
	USAGE_MAP(0xC0224, MAP_STATIC, EV_KEY, BTN_SELECT),

	/* map special buttons without HID bitmaps, corrected in event handler */
	USAGE_MAP(0xC0081, MAP_STATIC, EV_KEY, BTN_GRIPL),	/* Four paddles */

	/* hardware features handled at the raw report level */
	USAGE_IGN(0xC0085),	/* Profile switcher */
	USAGE_IGN(0xC0099),	/* Trigger scale switches */

	/* XBE2: Disable "dial", which is a redundant representation of the D-Pad */
	USAGE_IGN(0x10037),

	/* XBE2: Disable duplicate report fields of broken v1 packet format */
	USAGE_IGN(0x10040),	/* Vx, copy of X axis */
	USAGE_IGN(0x10041),	/* Vy, copy of Y axis */
	USAGE_IGN(0x10042),	/* Vz, copy of Z axis */
	USAGE_IGN(0x10043),	/* Vbrx, copy of Rx */
	USAGE_IGN(0x10044),	/* Vbry, copy of Ry */
	USAGE_IGN(0x10045),	/* Vbrz, copy of Rz */
	USAGE_IGN(0x90010),	/* copy of A */
	USAGE_IGN(0x90011),	/* copy of B */
	USAGE_IGN(0x90013),	/* copy of X */
	USAGE_IGN(0x90014),	/* copy of Y */
	USAGE_IGN(0x90016),	/* copy of LB */
	USAGE_IGN(0x90017),	/* copy of RB */
	USAGE_IGN(0x9001B),	/* copy of Start */
	USAGE_IGN(0x9001D),	/* copy of LS */
	USAGE_IGN(0x9001E),	/* copy of RS */
	USAGE_IGN(0xC0082),	/* copy of Select button */

	/* XBE2: Disable unused buttons */
	USAGE_IGN(0x90012),	/* 6 "TRIGGER_HAPPY" buttons */
	USAGE_IGN(0x90015),
	USAGE_IGN(0x90018),
	USAGE_IGN(0x90019),
	USAGE_IGN(0x9001A),
	USAGE_IGN(0x9001C),
	USAGE_IGN(0xC00B9),	/* KEY_SHUFFLE button */
};

#define map_usage_clear(ev) \
	hid_map_usage_clear(hi, usage, bit, max, (ev).event_type, (ev).input_code)
int xpadneo_mappings_input(struct hid_device *hdev, struct hid_input *hi,
			   struct hid_field *field,
			   struct hid_usage *usage, unsigned long **bit, int *max)
{
	int i = 0;

	if (usage->hid == HID_DC_BATTERYSTRENGTH) {
		struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

		xdata->battery.report_id = field->report->id;
		hid_info(hdev, "battery detected\n");

		return MAP_IGNORE;
	}

	for (i = 0; i < ARRAY_SIZE(usage_maps); i++) {
		const struct usage_map *entry = &usage_maps[i];

		if (entry->usage == usage->hid) {
			if (entry->behaviour == MAP_STATIC)
				map_usage_clear(entry->ev);
			return entry->behaviour;
		}
	}

	/* let HID handle this */
	return MAP_AUTO;
}
