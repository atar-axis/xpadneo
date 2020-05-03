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
		 "(bool) Combine the triggers to form a single axis. 1: combine, 0: do not combine");

static u8 param_trigger_rumble_damping = 0;

module_param_named(trigger_rumble_damping,
		   param_trigger_rumble_damping, byte, 0644);
MODULE_PARM_DESC(trigger_rumble_damping,
		 "(u8) Damp the trigger: 1 (none) to 2^8+ (max)");

/*
 * Debug Printk
 *
 * Prints a debug message to kernel (dmesg)
 * only if both is true, this is a DEBUG version and the
 * param_debug_level-parameter is equal or higher than the level
 * specified in hid_dbg_lvl
 */

#define DBG_LVL_NONE 0
#define DBG_LVL_FEW  1
#define DBG_LVL_SOME 2
#define DBG_LVL_ALL  3

#ifdef DEBUG
#define hid_dbg_lvl(lvl, fmt_hdev, fmt_str, ...) \
	do { \
		if (param_debug_level >= lvl) \
			hid_dbg(pr_fmt(fmt_hdev), \
				pr_fmt(fmt_str), ##__VA_ARGS__); \
	} while (0)
#define dbg_hex_dump_lvl(lvl, fmt_prefix, data, size) \
	do { \
		if (param_debug_level >= lvl) \
			print_hex_dump(KERN_DEBUG, pr_fmt(fmt_prefix), \
				DUMP_PREFIX_NONE, 32, 1, data, size, false); \
	} while (0)
#else
#define hid_dbg_lvl(lvl, fmt_hdev, fmt_str, ...) \
		no_printk(KERN_DEBUG pr_fmt(fmt_str), ##__VA_ARGS__)
#define dbg_hex_dump_lvl(lvl, fmt_prefix, data, size) \
		no_printk(KERN_DEBUG pr_fmt(fmt_prefix))
#endif

static DEFINE_IDA(xpadneo_device_id_allocator);

/*
 * FF Output Report
 *
 * This is the structure for the rumble output report. For more information
 * about this structure please take a look in the hid-report description.
 * Please notice that the structs are __packed, therefore there is no "padding"
 * between the elements (they behave more like an array).
 *
 */

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

/*
 * Device Data
 *
 * We attach information to hdev, which is therefore nearly globally accessible
 * via hid_get_drvdata(hdev). It is attached to the hid_device via
 * hid_set_drvdata(hdev) at the probing function.
 */

enum report_type {
	UNKNOWN,
	LINUX,
	WINDOWS
};

// TODO: avoid data duplication

const char *report_type_text[] = {
	"unknown",
	"linux/android",
	"windows"
};

struct xpadneo_devdata {
	/* mutual exclusion */
	spinlock_t lock;

	/* unique physical device id (randomly assigned) */
	int id;

	/* logical device interfaces */
	struct hid_device *hdev;
	struct input_dev *idev;
	struct power_supply *batt;

	/* report types */
	enum report_type report_descriptor;
	enum report_type report_behaviour;

	/* battery information */
	struct power_supply_desc batt_desc;
	u8 ps_online;
	u8 ps_present;
	u8 ps_capacity_level;
	u8 ps_status;

	/* axis states */
	s32 last_abs_z;
	s32 last_abs_rz;

	/* buffer for ff_worker */
	struct work_struct ff_worker;
	struct ff_data ff;
	void *output_report_dmabuf;

	/* buffer for batt_worker */
	struct work_struct batt_worker;
	u8 batt_status;
};

static void xpadneo_ff_worker(struct work_struct *work)
{
	struct xpadneo_devdata *xdata =
	    container_of(work, struct xpadneo_devdata, ff_worker);
	struct hid_device *hdev = xdata->hdev;
	struct ff_report *r = xdata->output_report_dmabuf;
	int ret;

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

/*
 * Force Feedback Callback
 *
 * This function is called by the Input Subsystem.
 * The effect data is set in userspace and sent to the driver via ioctl.
 *
 * Q: where is drvdata set to hid_device?
 * A: hid_hw_start (called in probe)
 *    -> hid_connect -> hidinput_connect
 *    -> hidinput_allocate (sets drvdata to hid_device)
 */
static int xpadneo_ff_play(struct input_dev *dev, void *data,
			   struct ff_effect *effect)
{
	const int fractions_percent[] = {
		100, 96, 85, 69, 50, 31, 15, 4, 0, 4, 15, 31, 50, 69, 85, 96,
		100
	};
	const int proportions_idx_max = 16;

	enum {
		DIRECTION_DOWN = 0x0000,
		DIRECTION_LEFT = 0x4000,
		DIRECTION_UP = 0x8000,
		DIRECTION_RIGHT = 0xC000,
	};

	int fraction_TL, fraction_TR;
	u32 weak, strong, direction, max_damped, max_unscaled;

	struct hid_device *hdev = input_get_drvdata(dev);
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	if (effect->type != FF_RUMBLE)
		return 0;

	/* copy data from effect structure at the very beginning */
	weak = effect->u.rumble.weak_magnitude;
	strong = effect->u.rumble.strong_magnitude;
	direction = effect->direction;

	hid_dbg_lvl(DBG_LVL_FEW, hdev,
		    "playing effect: strong: %#04x, weak: %#04x, direction: %#04x\n",
		    strong, weak, direction);

	/* calculate the physical magnitudes, scale from 16 bit to 0..100 */
	xdata->ff.magnitude_strong = (u8)((strong * 100) / U16_MAX);
	xdata->ff.magnitude_weak = (u8)((weak * 100) / U16_MAX);

	/*
	 * we want to keep the rumbling at the triggers below the maximum
	 * of the weak and strong main rumble
	 */
	max_unscaled = weak > strong ? weak : strong;

	/*
	 * get the proportions from a precalculated cosine table
	 * calculation goes like:
	 * cosine(a) * 100 =  {100, 96, 85, 69, 50, 31, 15, 4, 0, 4, 15, 31, 50, 69, 85, 96, 100}
	 * fractions_percent(a) = round(50 + (cosine * 50))
	 */
	fraction_TL = fraction_TR = 0;
	if (direction >= DIRECTION_LEFT && direction <= DIRECTION_RIGHT) {
		u8 index_left = (direction - DIRECTION_LEFT) >> 11;
		u8 index_right = proportions_idx_max - index_left;
		fraction_TL = fractions_percent[index_left];
		fraction_TR = fractions_percent[index_right];
	}

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

/* Device (Gamepad) Initialization */
static int xpadneo_initDevice(struct hid_device *hdev)
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

	/* 'HELLO' FROM THE OTHER SIDE */
	if (!param_disable_ff)
		xpadneo_welcome_rumble(hdev);

	/* Init Input System for Force Feedback (FF) */
	input_set_capability(idev, EV_FF, FF_RUMBLE);
	return input_ff_create_memless(idev, NULL, xpadneo_ff_play);
}

/* Callback function which return the available properties to userspace */
static int battery_get_property(struct power_supply *ps,
				enum power_supply_property property,
				union power_supply_propval *val)
{
	struct xpadneo_devdata *xdata = power_supply_get_drvdata(ps);
	unsigned long flags;
	u8 capacity_level, present, online, status;

	spin_lock_irqsave(&xdata->lock, flags);
	capacity_level = xdata->ps_capacity_level;
	present = xdata->ps_present;
	online = xdata->ps_online;
	status = xdata->ps_status;
	spin_unlock_irqrestore(&xdata->lock, flags);

	switch (property) {
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = "Microsoft";
		break;
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = "Xbox Wireless Controller";
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = present;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = online;
		break;
	case POWER_SUPPLY_PROP_SCOPE:
		val->intval = POWER_SUPPLY_SCOPE_DEVICE;
		break;
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		val->intval = capacity_level;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = status;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#define XPADNEO_BATTERY_ONLINE(data)   ((data&0x80)>>7)
#define XPADNEO_BATTERY_PRESENT(data)  ((data&0x0C)!=0x00)
#define XPADNEO_BATTERY_CHARGING(data) ((data&0x10)>>4)
#define XPADNEO_BATTERY_CAPACITY(data) (data&0x03)

/*
 * Battery Worker
 *
 * This function is called by the kernel worker thread.
 */
static void xpadneo_batt_worker(struct work_struct *work)
{
	struct xpadneo_devdata *xdata =
	    container_of(work, struct xpadneo_devdata, batt_worker);

	unsigned long flags;
	u8 capacity_level, present, online, status;
	u8 data = xdata->batt_status;

	online = XPADNEO_BATTERY_ONLINE(data);
	present = XPADNEO_BATTERY_PRESENT(data);

	switch (XPADNEO_BATTERY_CAPACITY(data)) {
	case 0x00:
		capacity_level = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
		break;
	case 0x01:
		capacity_level = POWER_SUPPLY_CAPACITY_LEVEL_LOW;
		break;
	case 0x02:
		capacity_level = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;
		break;
	case 0x03:
		capacity_level = POWER_SUPPLY_CAPACITY_LEVEL_HIGH;
		break;
	}

	switch (XPADNEO_BATTERY_CHARGING(data)) {
	case 0:
		status = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	case 1:
		status = POWER_SUPPLY_STATUS_CHARGING;
		break;
	}

	spin_lock_irqsave(&xdata->lock, flags);
	xdata->ps_status = status;
	xdata->ps_capacity_level = capacity_level;
	xdata->ps_online = online;
	xdata->ps_present = present;
	spin_unlock_irqrestore(&xdata->lock, flags);

	power_supply_changed(xdata->batt);
}

static int xpadneo_initBatt(struct hid_device *hdev)
{
	int ret = 0;
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	static enum power_supply_property battery_props[] = {
		/* is a power supply available? always true */
		POWER_SUPPLY_PROP_PRESENT,
		/* critical, low, normal, high, full */
		POWER_SUPPLY_PROP_CAPACITY_LEVEL,
		/* powers a specific device */
		POWER_SUPPLY_PROP_SCOPE,
		/* charging (full, plugged), not_charging */
		POWER_SUPPLY_PROP_STATUS,
		/* cstring - manufacturer name */
		POWER_SUPPLY_PROP_MANUFACTURER,
		/* cstring - model name */
		POWER_SUPPLY_PROP_MODEL_NAME,
		POWER_SUPPLY_PROP_ONLINE
	};

	struct power_supply_config ps_config = {
		/* pass the xpadneo_data to the get_property function */
		.drv_data = xdata
	};

	INIT_WORK(&xdata->batt_worker, xpadneo_batt_worker);

	/*
	 * NOTE: hdev->uniq is meant to be the MAC address and hence
	 *       it should be unique. Unfortunately, here it is not unique
	 *       neither is it the bluetooth MAC address.
	 *       As a solution we add an unique id for every gamepad.
	 */
	xdata->batt_desc.name = kasprintf(GFP_KERNEL, "xpadneo_batt_%pMR_%i",
					  hdev->uniq, xdata->id);
	if (!xdata->batt_desc.name)
		return -ENOMEM;

	/* Set up power supply */
	xdata->batt_desc.type = POWER_SUPPLY_TYPE_BATTERY;

	/*
	 * Set the battery capacity to 'full' until we get our first real
	 * battery event. Prevents false "critical low battery" notifications
	 */
	xdata->ps_capacity_level = POWER_SUPPLY_CAPACITY_LEVEL_FULL;

	/* Which properties of the battery are accessible? */
	xdata->batt_desc.properties = battery_props;
	xdata->batt_desc.num_properties = ARRAY_SIZE(battery_props);

	/*
	 * We have to offer a function which returns the current
	 * property values we defined above. Make sure that
	 * the get_property functions covers all properties above.
	 */
	xdata->batt_desc.get_property = battery_get_property;

	/* Advanced power management emulation */
	xdata->batt_desc.use_for_apm = 0;

	/* Register power supply for our gamepad device */
	xdata->batt = devm_power_supply_register(&hdev->dev,
						 &xdata->batt_desc, &ps_config);
	if (IS_ERR(xdata->batt)) {
		ret = PTR_ERR(xdata->batt);
		hid_err(hdev, "Unable to register battery device\n");
		goto err_free;
	} else {
		hid_dbg_lvl(DBG_LVL_SOME, hdev, "battery registered\n");
	}

	power_supply_powers(xdata->batt, &hdev->dev);

err_free:
	kfree(xdata->batt_desc.name);
	xdata->batt_desc.name = NULL;

	return ret;
}

enum mapping_behaviour {
	MAP_IGNORE,		/* Completely ignore this field */
	MAP_AUTO,		/* Do not really map it, let hid-core decide */
	MAP_STATIC		/* Map to the values given */
};

struct input_ev {
	/* Map to which input event (EV_KEY, EV_ABS, ...)? */
	u8 event_type;
	/* Map to which input code (BTN_A, ABS_X, ...)? */
	u16 input_code;
};

static u8 map_hid_to_input_windows(struct hid_usage *usage,
				   struct input_ev *map_to)
{
	/*
	 * Windows report-descriptor (307 byte):
	 *
	 * 05 01 09 05 a1 01 85 01 09 01 a1 00 09 30 09 31 15 00 27 ff
	 * ff 00 00 95 02 75 10 81 02 c0 09 01 a1 00 09 33 09 34 15 00
	 * 27 ff ff 00 00 95 02 75 10 81 02 c0 05 01 09 32 15 00 26 ff
	 * 03 95 01 75 0a 81 02 15 00 25 00 75 06 95 01 81 03 05 01 09
	 * 35 15 00 26 ff 03 95 01 75 0a 81 02 15 00 25 00 75 06 95 01
	 * 81 03 05 01 09 39 15 01 25 08 35 00 46 3b 01 66 14 00 75 04
	 * 95 01 81 42 75 04 95 01 15 00 25 00 35 00 45 00 65 00 81 03
	 * 05 09 19 01 29 0a 15 00 25 01 75 01 95 0a 81 02 15 00 25 00
	 * 75 06 95 01 81 03 05 01 09 80 85 02 a1 00 09 85 15 00 25 01
	 * 95 01 75 01 81 02 15 00 25 00 75 07 95 01 81 03 c0 05 0f 09
	 * 21 85 03 a1 02 09 97 15 00 25 01 75 04 95 01 91 02 15 00 25
	 * 00 75 04 95 01 91 03 09 70 15 00 25 64 75 08 95 04 91 02 09
	 * 50 66 01 10 55 0e 15 00 26 ff 00 75 08 95 01 91 02 09 a7 15
	 * 00 26 ff 00 75 08 95 01 91 02 65 00 55 00 09 7c 15 00 26 ff
	 * 00 75 08 95 01 91 02 c0 85 04 05 06 09 20 15 00 26 ff 00 75
	 * 08 95 01 81 02 c0 00
	 */

	unsigned int hid_usage = usage->hid & HID_USAGE;
	unsigned int hid_usage_page = usage->hid & HID_USAGE_PAGE;

	switch (hid_usage_page) {
	case HID_UP_BUTTON:
		switch (hid_usage) {
		case 0x01:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_A};
			return MAP_STATIC;
		case 0x02:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_B};
			return MAP_STATIC;
		case 0x03:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_X};
			return MAP_STATIC;
		case 0x04:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_Y};
			return MAP_STATIC;
		case 0x05:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_TL};
			return MAP_STATIC;
		case 0x06:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_TR};
			return MAP_STATIC;
		case 0x07:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_SELECT};
			return MAP_STATIC;
		case 0x08:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_START};
			return MAP_STATIC;
		case 0x09:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_THUMBL};
			return MAP_STATIC;
		case 0x0A:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_THUMBR};
			return MAP_STATIC;
		}
		return MAP_IGNORE;
	case HID_UP_GENDESK:
		switch (hid_usage) {
		case 0x30:
			*map_to = (struct input_ev) {
			EV_ABS, ABS_X};
			return MAP_STATIC;
		case 0x31:
			*map_to = (struct input_ev) {
			EV_ABS, ABS_Y};
			return MAP_STATIC;
		case 0x32:
			*map_to = (struct input_ev) {
			EV_ABS, ABS_Z};
			return MAP_STATIC;
		case 0x33:
			*map_to = (struct input_ev) {
			EV_ABS, ABS_RX};
			return MAP_STATIC;
		case 0x34:
			*map_to = (struct input_ev) {
			EV_ABS, ABS_RY};
			return MAP_STATIC;
		case 0x35:
			*map_to = (struct input_ev) {
			EV_ABS, ABS_RZ};
			return MAP_STATIC;
		case 0x39:
			*map_to = (struct input_ev) {
			0, 0};
			return MAP_AUTO;
		case 0x85:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_MODE};
			return MAP_STATIC;
		}
		return MAP_IGNORE;
	}

	return MAP_IGNORE;
}

static u8 map_hid_to_input_linux(struct hid_usage *usage,
				 struct input_ev *map_to)
{
	/*
	 * Linux report-descriptor (335 byte):
	 *
	 * 05 01 09 05 a1 01 85 01 09 01 a1 00 09 30 09 31 15 00 27 ff
	 * ff 00 00 95 02 75 10 81 02 c0 09 01 a1 00 09 32 09 35 15 00
	 * 27 ff ff 00 00 95 02 75 10 81 02 c0 05 02 09 c5 15 00 26 ff
	 * 03 95 01 75 0a 81 02 15 00 25 00 75 06 95 01 81 03 05 02 09
	 * c4 15 00 26 ff 03 95 01 75 0a 81 02 15 00 25 00 75 06 95 01
	 * 81 03 05 01 09 39 15 01 25 08 35 00 46 3b 01 66 14 00 75 04
	 * 95 01 81 42 75 04 95 01 15 00 25 00 35 00 45 00 65 00 81 03
	 * 05 09 19 01 29 0f 15 00 25 01 75 01 95 0f 81 02 15 00 25 00
	 * 75 01 95 01 81 03 05 0c 0a 24 02 15 00 25 01 95 01 75 01 81
	 * 02 15 00 25 00 75 07 95 01 81 03 05 0c 09 01 85 02 a1 01 05
	 * 0c 0a 23 02 15 00 25 01 95 01 75 01 81 02 15 00 25 00 75 07
	 * 95 01 81 03 c0 05 0f 09 21 85 03 a1 02 09 97 15 00 25 01 75
	 * 04 95 01 91 02 15 00 25 00 75 04 95 01 91 03 09 70 15 00 25
	 * 64 75 08 95 04 91 02 09 50 66 01 10 55 0e 15 00 26 ff 00 75
	 * 08 95 01 91 02 09 a7 15 00 26 ff 00 75 08 95 01 91 02 65 00
	 * 55 00 09 7c 15 00 26 ff 00 75 08 95 01 91 02 c0 85 04 05 06
	 * 09 20 15 00 26 ff 00 75 08 95 01 81 02 c0 00
	 */

	unsigned int hid_usage = usage->hid & HID_USAGE;
	unsigned int hid_usage_page = usage->hid & HID_USAGE_PAGE;

	switch (hid_usage_page) {
	case HID_UP_BUTTON:
		switch (hid_usage) {
		case 0x01:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_A};
			return MAP_STATIC;
		case 0x02:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_B};
			return MAP_STATIC;
		case 0x04:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_X};
			return MAP_STATIC;
		case 0x05:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_Y};
			return MAP_STATIC;
		case 0x07:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_TL};
			return MAP_STATIC;
		case 0x08:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_TR};
			return MAP_STATIC;
		case 0x0C:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_START};
			return MAP_STATIC;
		case 0x0E:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_THUMBL};
			return MAP_STATIC;
		case 0x0F:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_THUMBR};
			return MAP_STATIC;
		}
		return MAP_IGNORE;
	case HID_UP_CONSUMER:
		switch (hid_usage) {
		case 0x223:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_MODE};
			return MAP_STATIC;
		case 0x224:
			*map_to = (struct input_ev) {
			EV_KEY, BTN_SELECT};
			return MAP_STATIC;
		}
		return MAP_IGNORE;
	case HID_UP_GENDESK:
		switch (hid_usage) {
		case 0x30:
			*map_to = (struct input_ev) {
			EV_ABS, ABS_X};
			return MAP_STATIC;
		case 0x31:
			*map_to = (struct input_ev) {
			EV_ABS, ABS_Y};
			return MAP_STATIC;
		case 0x32:
			*map_to = (struct input_ev) {
			EV_ABS, ABS_RX};
			return MAP_STATIC;
		case 0x35:
			*map_to = (struct input_ev) {
			EV_ABS, ABS_RY};
			return MAP_STATIC;
		case 0x39:
			*map_to = (struct input_ev) {
			0, 0};
			return MAP_AUTO;
		}
		return MAP_IGNORE;
	case HID_UP_SIMULATION:
		switch (hid_usage) {
		case 0xC4:
			*map_to = (struct input_ev) {
			EV_ABS, ABS_RZ};
			return MAP_STATIC;
		case 0xC5:
			*map_to = (struct input_ev) {
			EV_ABS, ABS_Z};
			return MAP_STATIC;
		}
		return MAP_IGNORE;
	}

	return MAP_IGNORE;
}

/*
 * Input Mapping Hook
 *
 * Invoked at input registering before mapping an usage
 * (called once for every hid-usage).
 */
static int xpadneo_mapping(struct hid_device *hdev, struct hid_input *hi,
			   struct hid_field *field, struct hid_usage *usage,
			   unsigned long **bit, int *max)
{
	/* Return values */
	enum {
		RET_MAP_IGNORE = -1,	/* completely ignore this input */
		RET_MAP_AUTO,	/* let hid-core autodetect the mapping */
		RET_MAP_STATIC	/* mapped by hand, no further processing */
	};

	struct input_ev map_to;
	u8 (*perform_mapping) (struct hid_usage * usage,
			       struct input_ev * map_to);
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	switch (usage->hid) {
	case HID_DC_BATTERYSTRENGTH:
		hid_dbg_lvl(DBG_LVL_FEW, hdev,
			    "USG: 0x%05X -> battery report\n", usage->hid);
		return RET_MAP_AUTO;
	}

	switch (xdata->report_descriptor) {
	case LINUX:
		perform_mapping = map_hid_to_input_linux;
		break;
	case WINDOWS:
		perform_mapping = map_hid_to_input_windows;
		break;
	default:
		return RET_MAP_AUTO;
	}

	switch (perform_mapping(usage, &map_to)) {
	case MAP_AUTO:
		hid_dbg_lvl(DBG_LVL_FEW, hdev,
			    "UP: 0x%04X, USG: 0x%04X -> automatically\n",
			    usage->hid & HID_USAGE_PAGE,
			    usage->hid & HID_USAGE);

		return RET_MAP_AUTO;

	case MAP_IGNORE:
		hid_dbg_lvl(DBG_LVL_FEW, hdev,
			    "UP: 0x%04X, USG: 0x%04X -> ignored\n",
			    usage->hid & HID_USAGE_PAGE,
			    usage->hid & HID_USAGE);

		return RET_MAP_IGNORE;

	case MAP_STATIC:
		hid_dbg_lvl(DBG_LVL_FEW, hdev,
			    "UP: 0x%04X, USG: 0x%04X -> EV: 0x%03X, INP: 0x%03X\n",
			    usage->hid & HID_USAGE_PAGE, usage->hid & HID_USAGE,
			    map_to.event_type, map_to.input_code);

		hid_map_usage_clear(hi, usage, bit, max,
				    map_to.event_type, map_to.input_code);
		return RET_MAP_STATIC;

	}

	/* Something went wrong, ignore this field */
	return RET_MAP_IGNORE;
}

/*
 * Report Descriptor Fixup Hook
 *
 * You can either modify the original report in place and just
 * return the original start address (rdesc) or you reserve a new
 * one and return a pointer to it. In the latter, you mostly have to
 * modify the rsize value too.
 */
static u8 *xpadneo_report_fixup(struct hid_device *hdev, u8 *rdesc,
				unsigned int *rsize)
{
	hid_dbg_lvl(DBG_LVL_SOME, hdev, "REPORT (DESCRIPTOR) FIXUP HOOK\n");
	dbg_hex_dump_lvl(DBG_LVL_FEW, "xpadneo: report-descr: ", rdesc, *rsize);

	return rdesc;
}

static void process_battery_event(struct hid_device *hdev, __s32 value)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	xdata->batt_status = (u8)value;
	schedule_work(&xdata->batt_worker);
}

static void
check_report_behaviour(struct hid_device *hdev, u8 *data, int reportsize)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	if (xdata->report_behaviour == UNKNOWN) {
		if (data[0] == 0x01) {
			/*
			 * The length of the first input report with an ID of 0x01
			 * reveals which report-type the controller is actually
			 * sending (windows: 16, or linux: 17).
			 */
			switch (reportsize) {
			case 16:{
					xdata->report_behaviour = WINDOWS;
					break;
				}
			case 17:{
					xdata->report_behaviour = LINUX;
					break;
				}
			}
		} else if (data[0] == 0x02) {
			/* According to the descriptor, we can also assume Linux
			 * behaviour if we see report ID 0x02
			 */
			xdata->report_behaviour = LINUX;
		}
	}

	hid_dbg_lvl(DBG_LVL_SOME, hdev, "desc: %s, beh: %s\n",
		    report_type_text[xdata->report_descriptor],
		    report_type_text[xdata->report_behaviour]);

	/*
	 * TODO:
	 * Maybe the best solution would be to replace the report descriptor
	 * in case that the wrong reports are sent. Unfortunately we do not
	 * know if the report descriptor is the right one until the first
	 * report is sent to us. At this time, the report_fixup hook is
	 * already over and the original descriptor is parsed into hdev
	 * i.e. report_enum and collection.
	 *
	 * The next best solution would be to replace the report with
	 * ID 0x01 with the right one in report_enum (and collection?).
	 * I don't know yet how this would works, perhaps like this:
	 * - create a new report struct
	 * - fill it by hand
	 * - add all neccessary fields (automatic way?)
	 *
	 * Another way to fix it is:
	 * - Register another report with a _new_ ID by hand
	 *   (unfortunately we cannot use the same id again)
	 * - in raw_event: change the ID from 0x01 to the new one if
	 *   necessary. leave it if not.
	 *
	 * What we currently do is:
	 * We examine every report and fire the input events by hand.
	 * That's not very generic.
	 *
	 */

	// TODO:
	// * remove old report using list operations
	// * create new one like they do in hid_register_report
	// * add it to output_reports->report_list and array
}

static int xpadneo_raw_event(struct hid_device *hdev, struct hid_report *report,
			     u8 *data, int reportsize)
{
	/* Return Codes */
	enum {
		RAWEV_CONT_PROCESSING,	/* Let the hid-core autodetect the event */
		RAWEV_STOP_PROCESSING	/* Stop further processing */
	};

	//hid_dbg_lvl(DBG_LVL_SOME, hdev, "RAW EVENT HOOK\n");

	dbg_hex_dump_lvl(DBG_LVL_SOME, "xpadneo: raw_event: ", data,
			 reportsize);
	//hid_dbg_lvl(DBG_LVL_ALL, hdev, "report->size: %d\n", (report->size)/8);
	//hid_dbg_lvl(DBG_LVL_ALL, hdev, "data size (wo id): %d\n", reportsize-1);

	switch (report->id) {
	case 0x01:
	case 0x02:
		check_report_behaviour(hdev, data, reportsize);
		break;
	}

	/* Continue processing */
	return RAWEV_CONT_PROCESSING;
}

static void xpadneo_report(struct hid_device *hdev, struct hid_report *report)
{
	hid_dbg_lvl(DBG_LVL_SOME, hdev, "REPORT HOOK\n");
}

/*
 * Input Configured Hook
 *
 * We have to fix up the key-bitmap, because there is
 * no DPAD_UP, _RIGHT, _DOWN, _LEFT on the device by default
 *
 */
static int xpadneo_input_configured(struct hid_device *hdev,
				    struct hid_input *hi)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	xdata->idev = hi->input;

	hid_dbg_lvl(DBG_LVL_SOME, hdev, "INPUT CONFIGURED HOOK\n");

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
	case 0x02FD:
		hid_info(hdev,
			 "pretending XB1S Windows wireless mode (changed PID from 0x%04X to 0x02E0)\n",
			 (u16)xdata->idev->id.product);
		xdata->idev->id.product = 0x02E0;
		break;

	}

	if (param_combined_z_axis) {
		/*
		 * furthermore, we need to translate the incoming events to fit within
		 * the new range, we will do that in the xpadneo_event() hook.
		 */
		input_set_abs_params(xdata->idev, ABS_Z, -1024, 1023, 3, 63);

		/* We remove the ABS_RZ event if param_combined_z_axis is enabled */
		__clear_bit(ABS_RZ, xdata->idev->absbit);
	}

	return 0;
}

static int xpadneo_event(struct hid_device *hdev, struct hid_field *field,
			 struct hid_usage *usage, __s32 value)
{
	/* Return Codes */
	enum {
		EV_CONT_PROCESSING,	/* Let the hid-core autodetect the event */
		EV_STOP_PROCESSING	/* Stop further processing */
	};

	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct input_dev *idev = xdata->idev;

	switch (usage->hid) {
	case HID_DC_BATTERYSTRENGTH:
		process_battery_event(hdev, value);
		goto sync_and_stop_processing;
	}

	hid_dbg_lvl(DBG_LVL_ALL, hdev,
		    "hid-up: %02x, hid-usg: %02x, input-code: %02x, value: %02x\n",
		    (usage->hid & HID_USAGE_PAGE), (usage->hid & HID_USAGE),
		    usage->code, value);

	if (usage->type == EV_ABS) {
		switch (usage->code) {
		case ABS_Z:
			/*
			 * We need to combine ABS_Z and ABS_RZ if param_combined_z_axis
			 * is set, so remember the current value
			 */
			if (param_combined_z_axis) {
				xdata->last_abs_z = value;
				goto combine_z_axes;
			}
			break;
		case ABS_RZ:
			/*
			 * We need to combine ABS_Z and ABS_RZ if param_combined_z_axis
			 * is set, so remember the current value
			 */
			if (param_combined_z_axis) {
				xdata->last_abs_rz = value;
				goto combine_z_axes;
			}
			break;
		}
	}

	/*
	 * TODO:
	 * This is a workaround for the wrong report (Windows report but
	 * Linux descriptor). We would prefer to fixup the descriptor, but we
	 * cannot fix it anymore at the time we recognize the wrong behaviour,
	 * hence we will fire the input events by hand.
	 */
	if (xdata->report_behaviour == WINDOWS
	    && xdata->report_descriptor == LINUX) {
		/*
		 * we fix all buttons by hand. You may think that we
		 * could do that by using the windows_map too, but it is more
		 * like an coincidence that this would work in this case:
		 * It would only, because HID_UP_BUTTONS has no special names
		 * for the HID_USAGE's, therefore the first button stays 0x01
		 * on both reports (windows and linux) - it is a 1: 1 mapping.
		 * But this is not true in general (i.e. for other USAGE_PAGES)
		 */
		if ((usage->hid & HID_USAGE_PAGE) == HID_UP_BUTTON) {
			switch (usage->hid & HID_USAGE) {
			case 0x01:
				input_report_key(idev, BTN_A, value);
				break;
			case 0x02:
				input_report_key(idev, BTN_B, value);
				break;
			case 0x03:
				input_report_key(idev, BTN_X, value);
				break;
			case 0x04:
				input_report_key(idev, BTN_Y, value);
				break;
			case 0x05:
				input_report_key(idev, BTN_TL, value);
				break;
			case 0x06:
				input_report_key(idev, BTN_TR, value);
				break;
			case 0x07:
				input_report_key(idev, BTN_SELECT, value);
				break;
			case 0x08:
				input_report_key(idev, BTN_START, value);
				break;
			case 0x09:
				input_report_key(idev, BTN_THUMBL, value);
				break;
			case 0x0A:
				input_report_key(idev, BTN_THUMBR, value);
				break;
			}

			hid_dbg_lvl(DBG_LVL_ALL, hdev,
				    "hid-upage: %02x, hid-usage: %02x fixed\n",
				    (usage->hid & HID_USAGE_PAGE),
				    (usage->hid & HID_USAGE));

			goto sync_and_stop_processing;
		}
	}

	return EV_CONT_PROCESSING;

combine_z_axes:
	input_report_abs(idev, ABS_Z,
			 0 - xdata->last_abs_z + xdata->last_abs_rz);

sync_and_stop_processing:
	input_sync(idev);
	return EV_STOP_PROCESSING;
}

/* Device Probe and Remove Hook */
static int xpadneo_probe_device(struct hid_device *hdev,
				const struct hid_device_id *id)
{
	int ret;
	struct xpadneo_devdata *xdata;

	hid_dbg_lvl(DBG_LVL_FEW, hdev, "probing device: %s\n", hdev->name);

	/*
	 * Create a per-device data structure which is "nearly globally" accessible
	 * through hid_get_drvdata. The structure is freed automatically
	 * as soon as hdev->dev (the device) is removed, since we use the devm_
	 * derivate.
	 */
	xdata = devm_kzalloc(&hdev->dev, sizeof(*xdata), GFP_KERNEL);
	if (xdata == NULL)
		return -ENOMEM;

	xdata->id = ida_simple_get(&xpadneo_device_id_allocator,
				   0, 0, GFP_KERNEL);

	xdata->hdev = hdev;

	/* Unknown until first report with ID 01 arrives (see raw_event) */
	xdata->report_behaviour = UNKNOWN;
	switch (hdev->dev_rsize) {
	case 307:
		xdata->report_descriptor = WINDOWS;
		break;
	case 335:
		xdata->report_descriptor = LINUX;
		break;
	}

	hid_set_drvdata(hdev, xdata);

	/* Parse the raw report (includes a call to report_fixup) */
	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "parse failed\n");
		goto return_error;
	}

	/* Debug Output */
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "driver:\n");
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "* version: %s\n", DRV_VER);
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "hdev:\n");
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "* raw rdesc: (unfixed, see above)\n");
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "* raw rsize: %u\n", hdev->dev_rsize);
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "* bus: 0x%04X\n", hdev->bus);
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "* report group: %u\n", hdev->group);
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "* vendor: 0x%08X\n", hdev->vendor);
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "* version: 0x%08X\n", hdev->version);
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "* product: 0x%08X\n", hdev->product);
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "* country: %u\n", hdev->country);
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "* driverdata: %lu\n", id->driver_data);
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "* serial: %pMR\n", hdev->uniq);
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "* physical location: %pMR\n",
		    hdev->phys);

	/*
	 * We start our hardware without FF, we will add it afterwards by hand
	 * HID_CONNECT_DEFAULT = (HID_CONNECT_HIDINPUT | HID_CONNECT_HIDRAW
	 *                        | HID_CONNECT_HIDDEV | HID_CONNECT_FF)
	 * Our Input Device is created automatically since we defined
	 * HID_CONNECT_HIDINPUT as one of the flags.
	 */
	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT & ~HID_CONNECT_FF);
	if (ret) {
		hid_err(hdev, "hw start failed\n");
		goto return_error;
	}

	/* Call the device initialization routines */
	ret = xpadneo_initDevice(hdev);
	if (ret) {
		hid_err(hdev, "device initialization failed\n");
		goto return_error;
	}

	ret = xpadneo_initBatt(hdev);
	if (ret) {
		hid_err(hdev, "battery initialization failed\n");
		goto return_error;
	}

	/* Everything is fine */
	return 0;

return_error:
	return ret;
}

static void xpadneo_remove_device(struct hid_device *hdev)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	hid_hw_close(hdev);

	cancel_work_sync(&xdata->batt_worker);
	cancel_work_sync(&xdata->ff_worker);

	/* Cleaning up here */
	ida_simple_remove(&xpadneo_device_id_allocator, xdata->id);
	hid_hw_stop(hdev);
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "Goodbye %s!\n", hdev->name);
}

/* Device ID Structure, define all supported devices here */
static const struct hid_device_id xpadneo_devices[] = {
	/*
	 * The ProductID is somehow related to the Firmware Version,
	 * but it somehow changed back from 0x02FD (newer fw) to 0x02E0 (older)
	 * and vice versa on one controller here.
	 *
	 * Unfortunately you cannot tell from product id how the gamepad really
	 * behaves on reports, since the newer firmware supports both mappings
	 * (the one which is standard in linux and the old one, which is still
	 * used in windows).
	 */

	/* XBOX ONE S / X */
	{HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x02FD)},
	{HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x02E0)},

	/* XBOX ONE Elite Series 2 */
	{HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x0B05)},

	/* SENTINEL VALUE, indicates the end */
	{}
};

static struct hid_driver xpadneo_driver = {
	/* The name of the driver */
	.name = "xpadneo",

	/* Which devices is this driver for */
	.id_table = xpadneo_devices,

	/*
	 * Hooked as the input device is configured (before it is registered)
	 * we need that because we do not configure the input-device ourself
	 * but leave it up to hid_hw_start()
	 */
	.input_configured = xpadneo_input_configured,

	/* Invoked on input registering before mapping an usage */
	.input_mapping = xpadneo_mapping,

	/* If usage in usage_table, this hook is called */
	.event = xpadneo_event,

	/* Called before report descriptor parsing (NULL means nop) */
	.report_fixup = xpadneo_report_fixup,

	/* Called when a new device is inserted */
	.probe = xpadneo_probe_device,

	/* Called when a device is removed */
	.remove = xpadneo_remove_device,

	/* If report in report_table, this hook is called */
	.raw_event = xpadneo_raw_event,

	.report = xpadneo_report
};

MODULE_DEVICE_TABLE(hid, xpadneo_devices);

/*
 * Module Init and Exit
 *
 * We may replace init and remove by module_hid_driver(xpadneo_driver)
 * in future versions, as long as there is nothing special in these two
 * functions (but registering and unregistering the driver). Up to now it is
 * more useful for us to not "oversimplify" the whole driver-registering thing.
 *
 * Caution: do not use both! (module_hid_driver and hid_(un)register_driver)
 */
static int __init xpadneo_initModule(void)
{
	pr_info("%s: hello there!\n", xpadneo_driver.name);

	return hid_register_driver(&xpadneo_driver);
}

static void __exit xpadneo_exitModule(void)
{
	hid_unregister_driver(&xpadneo_driver);

	ida_destroy(&xpadneo_device_id_allocator);

	pr_info("%s: goodbye!\n", xpadneo_driver.name);
}

/*
 * Tell the driver system which functions to call at initialization and
 * removal of the module
 */
module_init(xpadneo_initModule);
module_exit(xpadneo_exitModule);
