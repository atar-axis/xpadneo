#define DRV_VER "@DO_NOT_CHANGE@"

/*
 * Force feedback support for XBOX ONE S and X gamepads via Bluetooth
 *
 * This driver was developed for a student project at fortiss GmbH in Munich.
 * Copyright (c) 2017 Florian Dollinger <dollinger.florian@gmx.de>
 */

#include <linux/hid.h>
#include <linux/power_supply.h>
#include <linux/input.h>	/* ff_memless(), ... */
#include <linux/module.h>	/* MODULE_*, module_*, ... */
#include <linux/slab.h>		/* kzalloc(), kfree(), ... */
#include <linux/delay.h>	/* mdelay(), ... */
#include "hid-ids.h"		/* VENDOR_ID... */

#define DEBUG

/* Module Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Florian Dollinger <dollinger.florian@gmx.de>");
MODULE_DESCRIPTION
    ("Linux kernel driver for Xbox ONE S+ gamepads (BT), incl. FF");
MODULE_VERSION(DRV_VER);

/* Module Parameters, located at /sys/module/hid_xpadneo/parameters */

/*
 * NOTE:
 * In general it is not guaranteed that a short variable is no more than
 * 16 bit long in C, it depends on the computer architecure. But as a kernel
 * module parameter it is, since <params.c> does use kstrtou16 for shorts
 * since version 3.14
 */

#ifdef DEBUG
static u8 param_debug_level;

module_param_named(debug_level, param_debug_level, byte, 0644);
MODULE_PARM_DESC(debug_level,
		 "(u8) Debug information level: 0 (none) to 3+ (most verbose).");
#endif

static u8 param_disable_ff;

module_param_named(disable_ff, param_disable_ff, byte, 0644);
MODULE_PARM_DESC(disable_ff,
		 "(u8) Disable FF: 0 (all enabled), 1 (disable main), 2 (disable triggers), 3 (disable all).");

#define PARAM_DISABLE_FF_NONE    0
#define PARAM_DISABLE_FF_MAIN    1
#define PARAM_DISABLE_FF_TRIGGER 2
#define PARAM_DISABLE_FF_ALL     3

static bool param_combined_z_axis;

module_param_named(combined_z_axis, param_combined_z_axis, bool, 0644);
MODULE_PARM_DESC(combined_z_axis,
		 "(bool) Combine the triggers to form a single axis. 1: combine, 0: do not combine.");

static u8 param_trigger_rumble_damping = 0;

module_param_named(trigger_rumble_damping, param_trigger_rumble_damping, byte,
		   0644);
MODULE_PARM_DESC(trigger_rumble_damping,
		 "(u8) Damp the trigger: 1 (none) to 2^8+ (max).");

static bool param_ff_connect_notify = 1;

module_param_named(ff_connect_notify, param_ff_connect_notify, bool, 0644);
MODULE_PARM_DESC(ff_connect_notify,
		 "(bool) Connection notification using force feedback. 1: enable, 0: disable.");

static DEFINE_IDA(xpadneo_device_id_allocator);

enum {
	FF_RUMBLE_NONE = 0x00,
	FF_RUMBLE_WEAK = 0x01,
	FF_RUMBLE_STRONG = 0x02,
	FF_RUMBLE_MAIN = FF_RUMBLE_WEAK | FF_RUMBLE_STRONG,
	FF_RUMBLE_RIGHT = 0x04,
	FF_RUMBLE_LEFT = 0x08,
	FF_RUMBLE_TRIGGERS = FF_RUMBLE_LEFT | FF_RUMBLE_RIGHT,
	FF_RUMBLE_ALL = 0x0F
};

struct ff_data {
	u8 enable;
	u8 magnitude_left;
	u8 magnitude_right;
	u8 magnitude_strong;
	u8 magnitude_weak;
	u8 pulse_sustain_10ms;
	u8 pulse_release_10ms;
	u8 loop_count;
} __packed;

#define XPADNEO_XB1S_FF_REPORT 0x03

struct ff_report {
	u8 report_id;
	struct ff_data ff;
} __packed;

struct xpadneo_devdata {
	/* unique physical device id (randomly assigned) */
	int id;

	/* logical device interfaces */
	struct hid_device *hdev;
	struct input_dev *idev;
	struct power_supply *battery;

	/* battery information */
	struct power_supply_desc psy_desc;
	u8 battery_report_id;
	u8 battery_flags;
	u8 battery_capacity;

	/* axis states */
	s32 last_abs_z;
	s32 last_abs_rz;

	/* buffer for ff_worker */
	struct work_struct ff_worker;
	struct ff_data ff;
	struct ff_data ff_shadow;
	void *output_report_dmabuf;
};

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

static const struct usage_map xpadneo_usage_maps[] = {
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
	USAGE_MAP(0x9000B, MAP_STATIC, EV_KEY, KEY_HOMEPAGE),	/* Xbox */

	/* fixup code "Sys Main Menu" from Windows report descriptor */
	USAGE_MAP(0x10085, MAP_STATIC, EV_KEY, KEY_HOMEPAGE),

	/* fixup code "AC Home" from Linux report descriptor */
	USAGE_MAP(0xC0223, MAP_STATIC, EV_KEY, KEY_HOMEPAGE),

	/* disable duplicate button */
	USAGE_IGN(0xC0224),
};

static void xpadneo_ff_worker(struct work_struct *work)
{
	struct xpadneo_devdata *xdata =
	    container_of(work, struct xpadneo_devdata, ff_worker);
	struct hid_device *hdev = xdata->hdev;
	struct ff_report *r = xdata->output_report_dmabuf;
	int ret;

	/* generate no report when magnitudes are still the same */
	if (memcmp(&xdata->ff_shadow, &xdata->ff, sizeof(xdata->ff)) == 0)
		return;
	else
		memcpy(&xdata->ff_shadow, &xdata->ff, sizeof(xdata->ff));

	memset(r, 0, sizeof(*r));

	r->report_id = XPADNEO_XB1S_FF_REPORT;

	/* the user can disable some rumble motors */
	r->ff.enable = FF_RUMBLE_ALL;
	if (param_disable_ff & PARAM_DISABLE_FF_TRIGGER)
		r->ff.enable &= ~FF_RUMBLE_TRIGGERS;
	if (param_disable_ff & PARAM_DISABLE_FF_MAIN)
		r->ff.enable &= ~FF_RUMBLE_MAIN;

	/*
	 * ff-memless has a time resolution of 50ms but we pulse the motors
	 * as long as possible
	 */
	r->ff.pulse_sustain_10ms = U8_MAX;
	r->ff.loop_count = U8_MAX;

	/* trigger motors */
	r->ff.magnitude_left = xdata->ff.magnitude_left;
	r->ff.magnitude_right = xdata->ff.magnitude_right;

	/* main motors */
	r->ff.magnitude_strong = xdata->ff.magnitude_strong;
	r->ff.magnitude_weak = xdata->ff.magnitude_weak;

	ret = hid_hw_output_report(hdev, (__u8 *) r, sizeof(*r));
	if (ret < 0)
		hid_warn(hdev, "failed to send FF report: %d\n", ret);
}

static int xpadneo_ff_play(struct input_dev *dev, void *data,
			   struct ff_effect *effect)
{
	enum {
		DIRECTION_DOWN = 0x0000UL,
		DIRECTION_LEFT = 0x4000UL,
		DIRECTION_UP = 0x8000UL,
		DIRECTION_RIGHT = 0xC000UL,
		QUARTER = DIRECTION_LEFT,
	};

	int fraction_TL, fraction_TR, fraction_MAIN;
	s32 weak, strong, direction, max_damped, max_unscaled;

	struct hid_device *hdev = input_get_drvdata(dev);
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	if (effect->type != FF_RUMBLE)
		return 0;

	/* copy data from effect structure at the very beginning */
	weak = effect->u.rumble.weak_magnitude;
	strong = effect->u.rumble.strong_magnitude;
	direction = effect->direction;

	/*
	 * scale the main rumble lineary within each half of the cirlce,
	 * so we can completely turn off the main rumble while still doing
	 * trigger rumble alone
	 */
	if (direction <= DIRECTION_UP) {
		/* scale the main rumbling between 0x0000..0x8000 (100%..0%) */
		fraction_MAIN =
		    ((DIRECTION_UP - direction) * 100) / DIRECTION_UP;
	} else {
		/* scale the main rumbling between 0x8000..0xffff (0%..100%) */
		fraction_MAIN =
		    ((direction - DIRECTION_UP) * 100) / DIRECTION_UP;
	}

	/* calculate the physical magnitudes, scale from 16 bit to 0..100 */
	xdata->ff.magnitude_strong = (u8)((strong * fraction_MAIN) / U16_MAX);
	xdata->ff.magnitude_weak = (u8)((weak * fraction_MAIN) / U16_MAX);

	/*
	 * scale the trigger rumble lineary within each quarter:
	 *        _ _
	 * LT = /     \
	 * RT = _ / \ _
	 *      1 2 3 4
	 *
	 * This gives us 4 different modes of operation (with smooth transitions)
	 * to get a mostly somewhat independent control over each motor:
	 *
	 *                DOWN .. LEFT ..  UP  .. RGHT .. DOWN
	 * left rumble  =   0% .. 100% .. 100% ..   0% ..   0%
	 * right rumble =   0% ..   0% .. 100% .. 100% ..   0%
	 * main rumble  = 100% ..  50% ..   0% ..  50% .. 100%
	 *
	 * For completely independent control, we'd need a sphere instead of a
	 * circle but we only have one direction. We could decouple the
	 * direction from the main rumble but that seems to be outside the spec
	 * of the rumble protocol (direction without any magnitude should do
	 * nothing).
	 */
	if (direction <= DIRECTION_LEFT) {
		/* scale the left trigger between 0x0000..0x4000 (0%..100%) */
		fraction_TL = (direction * 100) / QUARTER;
		fraction_TR = 0;
	} else if (direction <= DIRECTION_UP) {
		/* scale the right trigger between 0x4000..0x8000 (0%..100%) */
		fraction_TL = 100;
		fraction_TR = ((direction - DIRECTION_LEFT) * 100) / QUARTER;
	} else if (direction <= DIRECTION_RIGHT) {
		/* scale the right trigger between 0x8000..0xC000 (100%..0%) */
		fraction_TL = 100;
		fraction_TR = ((DIRECTION_RIGHT - direction) * 100) / QUARTER;
	} else {
		/* scale the left trigger between 0xC000...0xFFFF (0..100%) */
		fraction_TL =
		    100 - ((direction - DIRECTION_RIGHT) * 100) / QUARTER;
		fraction_TR = 0;
	}

	/*
	 * we want to keep the rumbling at the triggers below the maximum
	 * of the weak and strong main rumble
	 */
	max_unscaled = weak > strong ? weak : strong;

	/*
	 * the user can change the damping at runtime, hence check the
	 * range
	 */
	if (param_trigger_rumble_damping > 0)
		max_damped = max_unscaled / param_trigger_rumble_damping;
	else
		max_damped = max_unscaled;

	/* calculate the physical magnitudes, scale from 16 bit to 0..100 */
	xdata->ff.magnitude_left = (u8)((max_damped * fraction_TL) / U16_MAX);
	xdata->ff.magnitude_right = (u8)((max_damped * fraction_TR) / U16_MAX);

	/* schedule writing a rumble report to the controller */
	schedule_work(&xdata->ff_worker);
	return 0;
}

static void xpadneo_welcome_rumble(struct hid_device *hdev)
{
	struct ff_report ff_pck;

	memset(&ff_pck.ff, 0, sizeof(ff_pck.ff));

	ff_pck.report_id = XPADNEO_XB1S_FF_REPORT;
	ff_pck.ff.magnitude_weak = 40;
	ff_pck.ff.magnitude_strong = 20;
	ff_pck.ff.magnitude_right = 10;
	ff_pck.ff.magnitude_left = 10;
	ff_pck.ff.pulse_sustain_10ms = 5;
	ff_pck.ff.pulse_release_10ms = 5;
	ff_pck.ff.loop_count = 3;

	ff_pck.ff.enable = FF_RUMBLE_WEAK;
	hid_hw_output_report(hdev, (u8 *)&ff_pck, sizeof(ff_pck));
	mdelay(330);

	ff_pck.ff.enable = FF_RUMBLE_STRONG;
	hid_hw_output_report(hdev, (u8 *)&ff_pck, sizeof(ff_pck));
	mdelay(330);

	ff_pck.ff.enable = FF_RUMBLE_TRIGGERS;
	hid_hw_output_report(hdev, (u8 *)&ff_pck, sizeof(ff_pck));
	mdelay(330);
}

static int xpadneo_init_ff(struct hid_device *hdev)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct input_dev *idev = xdata->idev;

	INIT_WORK(&xdata->ff_worker, xpadneo_ff_worker);
	xdata->output_report_dmabuf = devm_kzalloc(&hdev->dev,
						   sizeof(struct
							  ff_report),
						   GFP_KERNEL);
	if (xdata->output_report_dmabuf == NULL)
		return -ENOMEM;

	if (!param_disable_ff && param_ff_connect_notify)
		xpadneo_welcome_rumble(hdev);

	input_set_capability(idev, EV_FF, FF_RUMBLE);
	return input_ff_create_memless(idev, NULL, xpadneo_ff_play);
}

#define XPADNEO_BATTERY_ONLINE(data)         ((data&0x80)>>7)
#define XPADNEO_BATTERY_MODE(data)           ((data&0x0C)>>2)
#define XPADNEO_BATTERY_PRESENT(data)        ((data&0x0C)>0)
#define XPADNEO_BATTERY_CHARGING(data)       ((data&0x10)>>4)
#define XPADNEO_BATTERY_CAPACITY_LEVEL(data) (data&0x03)

static int xpadneo_get_battery_property(struct power_supply *psy,
					enum power_supply_property property,
					union power_supply_propval *val)
{
	struct xpadneo_devdata *xdata = power_supply_get_drvdata(psy);

	u8 flags = xdata->battery_flags;
	u8 level = XPADNEO_BATTERY_CAPACITY_LEVEL(flags);
	u8 charging = XPADNEO_BATTERY_CHARGING(flags);

	static int capacity_level_map[] = {
		[0] = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL,
		[1] = POWER_SUPPLY_CAPACITY_LEVEL_LOW,
		[2] = POWER_SUPPLY_CAPACITY_LEVEL_HIGH,
		[3] = POWER_SUPPLY_CAPACITY_LEVEL_FULL,
	};

	if (!xdata->battery)
		return -EINVAL;

	switch (property) {
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = xdata->battery_capacity;
		break;

	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		if (level >= ARRAY_SIZE(capacity_level_map)
		    || xdata->psy_desc.type == POWER_SUPPLY_TYPE_UNKNOWN)
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN;
		else if (XPADNEO_BATTERY_MODE(flags)
			 || XPADNEO_BATTERY_CHARGING(flags)) {
			val->intval = capacity_level_map[level];
		} else
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN;
		break;

	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = xdata->hdev->name;
		break;

	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = XPADNEO_BATTERY_ONLINE(flags);
		break;

	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = XPADNEO_BATTERY_PRESENT(flags);
		break;

	case POWER_SUPPLY_PROP_SCOPE:
		val->intval = POWER_SUPPLY_SCOPE_DEVICE;
		break;

	case POWER_SUPPLY_PROP_STATUS:
		if (xdata->psy_desc.type == POWER_SUPPLY_TYPE_UNKNOWN)
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		else if (!charging
			 && XPADNEO_BATTERY_CAPACITY_LEVEL(flags) == 3)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else if (charging)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else if (XPADNEO_BATTERY_PRESENT(flags))
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int xpadneo_setup_battery(struct hid_device *hdev,
				 struct hid_field *field)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct power_supply_config ps_config = {
		.drv_data = xdata
	};

	int ret;

	/* What properties can be queried? */
	static enum power_supply_property battery_props[] = {
		POWER_SUPPLY_PROP_CAPACITY,
		POWER_SUPPLY_PROP_CAPACITY_LEVEL,
		POWER_SUPPLY_PROP_MODEL_NAME,
		POWER_SUPPLY_PROP_ONLINE,
		POWER_SUPPLY_PROP_PRESENT,
		POWER_SUPPLY_PROP_SCOPE,
		POWER_SUPPLY_PROP_STATUS,
	};

	/* already registered */
	if (xdata->battery)
		return 0;

	xdata->psy_desc.name =
	    kasprintf(GFP_KERNEL, "xpadneo-%s-battery",
		      strnlen(hdev->uniq,
			      1) ? hdev->uniq : dev_name(&hdev->dev));
	if (!xdata->psy_desc.name)
		return -ENOMEM;

	xdata->psy_desc.properties = battery_props;
	xdata->psy_desc.num_properties = ARRAY_SIZE(battery_props);
	xdata->psy_desc.use_for_apm = 0;
	xdata->psy_desc.get_property = xpadneo_get_battery_property;

	/*
	 * This is a work-around to prevent undefined capacity indication
	 * and make the desktops actually report the battery capacity. In
	 * tests, changing the power supply type dynamically wasn't picked
	 * up by the desktop notification.
	 */
	xdata->psy_desc.type = POWER_SUPPLY_TYPE_BATTERY;
	xdata->battery_capacity = 50;

	/*
	 * register power supply via device manager (thus it will
	 * auto-release on detach)
	 */
	xdata->battery =
	    devm_power_supply_register(&hdev->dev, &xdata->psy_desc,
				       &ps_config);
	if (IS_ERR(xdata->battery)) {
		ret = PTR_ERR(xdata->battery);
		hid_err(hdev, "battery registration failed\n");
		goto err_free_name;
	} else {
		hid_info(hdev, "battery registered\n");
	}

	xdata->battery_report_id = field->report->id;
	power_supply_powers(xdata->battery, &hdev->dev);
	return 0;

err_free_name:
	kfree(xdata->psy_desc.name);
	xdata->psy_desc.name = NULL;

	return ret;
}

static int xpadneo_mapping(struct hid_device *hdev, struct hid_input *hi,
			   struct hid_field *field, struct hid_usage *usage,
			   unsigned long **bit, int *max)
{
	int i = 0;

	if (usage->hid == HID_DC_BATTERYSTRENGTH) {
		xpadneo_setup_battery(hdev, field);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(xpadneo_usage_maps); i++) {
		const struct usage_map *entry = &xpadneo_usage_maps[i];

		if (entry->usage == usage->hid) {
			if (entry->behaviour == 1) {
				hid_map_usage_clear(hi, usage, bit, max,
						    entry->ev.event_type,
						    entry->ev.input_code);
			}
			return entry->behaviour;
		}
	}

	/* let HID handle this */
	return 0;
}

static u8 *xpadneo_report_fixup(struct hid_device *hdev, u8 *rdesc,
				unsigned int *rsize)
{
	/* fixup trailing NUL byte */
	if (rdesc[*rsize - 2] == 0xC0 && rdesc[*rsize - 1] == 0x00) {
		hid_notice(hdev, "fixing up report size\n");
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
		/* 11 buttons instead of 10: properly remap the Xbox button */
		if (rdesc[140] == 0x05 && rdesc[141] == 0x09 &&
		    rdesc[144] == 0x29 && rdesc[145] == 0x0F &&
		    rdesc[152] == 0x95 && rdesc[153] == 0x0F &&
		    rdesc[162] == 0x95 && rdesc[163] == 0x01) {
			hid_notice(hdev, "fixing up button mapping\n");
			rdesc[145] = 0x0B;	/* 15 buttons -> 11 buttons */
			rdesc[153] = 0x0B;	/* 15 bits -> 11 bits buttons */
			rdesc[163] = 0x05;	/* 1 bit -> 5 bits constants */
		}
	}

	return rdesc;
}

static void xpadneo_update_battery(struct xpadneo_devdata *xdata, u8 value)
{
	xdata->battery_flags = value;
	switch (XPADNEO_BATTERY_MODE(value)) {
	case 0:
		xdata->psy_desc.type = POWER_SUPPLY_TYPE_USB;
		break;
	case 1:
	case 2:
		xdata->psy_desc.type = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		xdata->psy_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
	}

	if (XPADNEO_BATTERY_CHARGING(value) || XPADNEO_BATTERY_PRESENT(value))
		switch (XPADNEO_BATTERY_CAPACITY_LEVEL(value)) {
		case 0:
			xdata->battery_capacity = 5;
			break;
		case 1:
			xdata->battery_capacity = 33;
			break;
		case 2:
			xdata->battery_capacity = 66;
			break;
		case 3:
			xdata->battery_capacity = 99;
			break;
		}

	power_supply_changed(xdata->battery);
}

static int xpadneo_raw_event(struct hid_device *hdev, struct hid_report *report,
			     u8 *data, int reportsize)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	/* we are taking care of the battery report ourselves */
	if (report->id == xdata->battery_report_id && reportsize == 2) {
		xpadneo_update_battery(xdata, data[1]);
		return -1;
	}

	/* correct button mapping of Xbox controllers in Linux mode */
	if (report->id == 1 && (reportsize == 17 || reportsize == 39)) {
		u16 bits = 0;

		bits |= (data[14] & (BIT(0) | BIT(1))) >> 0;	/* A, B */
		bits |= (data[14] & (BIT(3) | BIT(4))) >> 1;	/* X, Y */
		bits |= (data[14] & (BIT(6) | BIT(7))) >> 2;	/* LB, RB */
		bits |= (data[16] & BIT(0)) << 6;	/* Back */
		bits |= (data[15] & BIT(3)) << 4;	/* Menu */
		bits |= (data[15] & BIT(5)) << 3;	/* LS */
		bits |= (data[15] & BIT(6)) << 3;	/* RS */
		bits |= (data[15] & BIT(4)) << 6;	/* Xbox */
		data[14] = (u8)((bits >> 0) & 0xFF);
		data[15] = (u8)((bits >> 8) & 0xFF);
		data[16] = 0;
	}

	return 0;
}

static int xpadneo_input_configured(struct hid_device *hdev,
				    struct hid_input *hi)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	xdata->idev = hi->input;

	/*
	 * Pretend that we are in Windows pairing mode as we are actually
	 * exposing the Windows mapping. This prevents SDL and other layers
	 * (probably browser game controller APIs) from treating our driver
	 * unnecessarily with button and axis mapping fixups, and it seems
	 * this is actually a firmware mode meant to Android usage only:
	 *
	 * Xbox One S:
	 * 0x2E0 wireless Windows mode (non-Android mode)
	 * 0x2EA USB Windows and Linux mode
	 * 0x2FD wireless Linux mode (Android mode)
	 *
	 * Xbox Elite 2:
	 * 0xB00 USB Windows and Linux mode
	 * 0xB05 wireless Linux mode (Android mode)
	 */
	switch (xdata->idev->id.product) {
#if 0
	case 0x0B05:
		/* FIXME What's the Windows ID of this controller? */
		hid_info(hdev,
			 "pretending XBE2 Windows wireless mode (changed PID from 0x%04X to 0x???)\n",
			 (u16)xdata->idev->id.product);
		xdata->idev->id.product = 0x02E0;
		break;

#endif
	case 0x02E0:
		hid_info(hdev,
			 "pretending XB1S Linux firmware version "
			 "(changed version from 0x%08X to 0x00000903)\n",
			 xdata->idev->id.version);
		xdata->idev->id.version = 0x00000903;
		break;
	case 0x02FD:
		hid_info(hdev,
			 "pretending XB1S Windows wireless mode "
			 "(changed PID from 0x%04X to 0x02E0)\n",
			 (u16)xdata->idev->id.product);
		xdata->idev->id.product = 0x02E0;
		break;

	}

	if (param_combined_z_axis) {
		/*
		 * We also need to translate the incoming events to fit within
		 * the new range, we will do that in the xpadneo_event() hook.
		 */
		input_set_abs_params(xdata->idev, ABS_Z, -1023, 1023, 3, 63);
		__clear_bit(ABS_RZ, xdata->idev->absbit);
	}

	return 0;
}

static int xpadneo_event(struct hid_device *hdev, struct hid_field *field,
			 struct hid_usage *usage, __s32 value)
{
	enum {
		EV_CONT_PROCESSING,	/* Let the hid-core autodetect the event */
		EV_STOP_PROCESSING	/* Stop further processing */
	};

	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct input_dev *idev = xdata->idev;

	/*
	 * We need to combine ABS_Z and ABS_RZ if param_combined_z_axis
	 * is set, so remember the current value
	 */
	if (param_combined_z_axis && (usage->type == EV_ABS)) {
		switch (usage->code) {
		case ABS_Z:
			xdata->last_abs_z = value;
			goto combine_z_axes;
		case ABS_RZ:
			xdata->last_abs_rz = value;
			goto combine_z_axes;
		}
	}

	return EV_CONT_PROCESSING;

combine_z_axes:
	input_report_abs(idev, ABS_Z, xdata->last_abs_rz - xdata->last_abs_z);
	input_sync(idev);
	return EV_STOP_PROCESSING;
}

static int xpadneo_probe(struct hid_device *hdev,
			 const struct hid_device_id *id)
{
	int ret;
	struct xpadneo_devdata *xdata;

	xdata = devm_kzalloc(&hdev->dev, sizeof(*xdata), GFP_KERNEL);
	if (xdata == NULL)
		return -ENOMEM;

	xdata->id = ida_simple_get(&xpadneo_device_id_allocator,
				   0, 0, GFP_KERNEL);

	xdata->hdev = hdev;
	hid_set_drvdata(hdev, xdata);

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "parse failed");
		goto return_error;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT & ~HID_CONNECT_FF);
	if (ret) {
		hid_err(hdev, "hw start failed\n");
		goto return_error;
	}

	ret = xpadneo_init_ff(hdev);
	if (ret)
		hid_err(hdev, "could not initialize ff, continuing anyway\n");

	return 0;

return_error:
	return ret;
}

static void xpadneo_release_device_id(struct xpadneo_devdata *xdata)
{
	if (xdata->id >= 0) {
		ida_simple_remove(&xpadneo_device_id_allocator, xdata->id);
		xdata->id = -1;
	}
}

static void xpadneo_remove(struct hid_device *hdev)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	hid_hw_close(hdev);

	cancel_work_sync(&xdata->ff_worker);

	xpadneo_release_device_id(xdata);
	hid_hw_stop(hdev);
}

static const struct hid_device_id xpadneo_devices[] = {
	/* XBOX ONE S / X */
	{HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x02FD)},
	{HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x02E0)},

	/* XBOX ONE Elite Series 2 */
	{HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x0B05)},

	/* SENTINEL VALUE, indicates the end */
	{}
};

MODULE_DEVICE_TABLE(hid, xpadneo_devices);

static struct hid_driver xpadneo_driver = {
	.name = "xpadneo",
	.id_table = xpadneo_devices,
	.input_mapping = xpadneo_mapping,
	.input_configured = xpadneo_input_configured,
	.probe = xpadneo_probe,
	.remove = xpadneo_remove,
	.report_fixup = xpadneo_report_fixup,
	.raw_event = xpadneo_raw_event,
	.event = xpadneo_event,
};

static int __init xpadneo_init(void)
{
	dbg_hid("xpadneo:%s\n", __func__);
	return hid_register_driver(&xpadneo_driver);
}

static void __exit xpadneo_exit(void)
{
	dbg_hid("xpadneo:%s\n", __func__);
	hid_unregister_driver(&xpadneo_driver);
	ida_destroy(&xpadneo_device_id_allocator);
}

module_init(xpadneo_init);
module_exit(xpadneo_exit);
