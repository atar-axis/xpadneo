/*
 * Force feedback support for XBOX ONE S and X gamepads via Bluetooth
 *
 * This driver was developed for a student project at fortiss GmbH in Munich.
 * Copyright (c) 2017 Florian Dollinger <dollinger.florian@gmx.de>
 */

#include "hid-xpadneo.h"


static void create_ff_pck (struct ff_report *pck, u8 id, u8 en_act,
	u8 mag_lt, u8 mag_rt, u8 mag_l, u8 mag_r,
	u8 start_delay) {

	pck->report_id = id;

	pck->ff.enable_actuators = en_act;
	pck->ff.magnitude_left_trigger = mag_lt;
	pck->ff.magnitude_right_trigger = mag_rt;
	pck->ff.magnitude_left = mag_l;
	pck->ff.magnitude_right = mag_r;
	pck->ff.duration = 0xFF;
	pck->ff.start_delay = start_delay;
	pck->ff.loop_count = 0xFF;

	/* It is up to the Input-Subsystem to start and stop effects as needed.
	 * All WE need to do is to play the effect at least 32767 ms long.
	 * Take a look here:
	 * https://stackoverflow.com/questions/48034091/
	 * We therefore simply play the effect as long as possible, which is
	 * 2,55s * 255 = 650,25s ~ = 10min
	 */
}

/*
 * Force Feedback Callback
 *
 * This function is called by the Input Subsystem.
 * The effect data is set in userspace and sent to the driver via ioctl.
 */

static int xpadneo_ff_play(struct input_dev *dev, void *data,
	struct ff_effect *effect)
{
	/* Q: where is drvdata set to hid_device?
	 * A: hid_hw_start (called in probe)
	 *    -> hid_connect -> hidinput_connect
	 *    -> hidinput_allocate (sets drvdata to hid_device)
	 */

	struct ff_report ff_pck;
	u16 weak, strong, direction, max, max_damped;
	u8 mag_main_right, mag_main_left, mag_trigger_right, mag_trigger_left;
	u8 ff_active;

	const int fractions_milli[]
		= {1000, 962, 854, 691, 500, 309, 146, 38, 0};
	const int proportions_idx_max = 8;
	u8 index_left, index_right;
	int fraction_TL, fraction_TR;
	u8 trigger_rumble_damping_nonzero;

	enum {
		DIRECTION_DOWN  = 0x0000,
		DIRECTION_LEFT  = 0x4000,
		DIRECTION_UP    = 0x8000,
		DIRECTION_RIGHT = 0xC000,
	};


	struct hid_device *hdev = input_get_drvdata(dev);

	if (param_disable_ff == PARAM_DISABLE_FF_ALL)
		return 0;

	if (effect->type != FF_RUMBLE)
		return 0;

	/* copy data from effect structure at the very beginning */
	weak      = effect->u.rumble.weak_magnitude;
	strong    = effect->u.rumble.strong_magnitude;
	direction = effect->direction;

	hid_dbg_lvl(DBG_LVL_FEW, hdev, "playing effect: strong: %#04x, weak: %#04x, direction: %#04x\n",
		strong, weak, direction);

	/* calculate the physical magnitudes */
	mag_main_right = (u8)((weak & 0xFF00) >> 8);   /* u16 to u8 */
	mag_main_left  = (u8)((strong & 0xFF00) >> 8); /* u16 to u8 */


	/* get the proportions from a precalculated cosine table
	 * calculation goes like:
	 * cosine(a) * 1000 =  {1000, 924, 707, 383, 0, -383, -707, -924, -1000}
	 * fractions_milli(a) = (1000 + (cosine * 1000)) / 2
	 */

	fraction_TL = 0;
	fraction_TR = 0;

	if (direction >= DIRECTION_LEFT && direction <= DIRECTION_RIGHT) {
		index_left = (direction - DIRECTION_LEFT) >> 12;
		index_right = proportions_idx_max - index_left;

		fraction_TL = fractions_milli[index_left];
		fraction_TR = fractions_milli[index_right];
	}

	/* we want to keep the rumbling at the triggers below the maximum
	* of the weak and strong main rumble
	*/
	max = mag_main_right > mag_main_left ? mag_main_right : mag_main_left;

	/* the user can change the damping at runtime, hence check the range */
	trigger_rumble_damping_nonzero
		= param_trigger_rumble_damping == 0 ? 1 : param_trigger_rumble_damping;

	max_damped = max / trigger_rumble_damping_nonzero;

	mag_trigger_left = (u8)((max_damped * fraction_TL) / 1000);
	mag_trigger_right = (u8)((max_damped * fraction_TR) / 1000);


	ff_active = FF_ENABLE_ALL;

	if (param_disable_ff & PARAM_DISABLE_FF_TRIGGER)
		ff_active &= ~(FF_ENABLE_LEFT_TRIGGER | FF_ENABLE_RIGHT_TRIGGER);

	if (param_disable_ff & PARAM_DISABLE_FF_MAIN)
		ff_active &= ~(FF_ENABLE_LEFT | FF_ENABLE_RIGHT);


	create_ff_pck(
		&ff_pck, 0x03,
		ff_active,
		mag_trigger_left, mag_trigger_right,
		mag_main_left, mag_main_right,
		0);


	hid_dbg_lvl(DBG_LVL_FEW, hdev,
		"active: %#04x, max: %#04x, prop_left: %#04x, prop_right: %#04x, left trigger: %#04x, right: %#04x\n",
		ff_active,
		max, fraction_TL, fraction_TR,
		ff_pck.ff.magnitude_left_trigger,
		ff_pck.ff.magnitude_right_trigger);

	hid_hw_output_report(hdev, (u8 *)&ff_pck, sizeof(ff_pck));

	return 0;
}


/*
 * Device (Gamepad) Initialization
 */

static int xpadneo_initDevice(struct hid_device *hdev)
{
	int error;

	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct input_dev *idev = xdata->idev;


	struct ff_report ff_pck;

	/* TODO: outsource that */

	ff_pck.ff = ff_clear;

	/* 'HELLO' FROM THE OTHER SIDE */
	if (!param_disable_ff) {
		ff_pck.report_id = 0x03;
		ff_pck.ff.magnitude_right = 0x80;
		ff_pck.ff.magnitude_left  = 0x40;
		ff_pck.ff.magnitude_right_trigger = 0x20;
		ff_pck.ff.magnitude_left_trigger  = 0x20;
		ff_pck.ff.duration = 33;

		ff_pck.ff.enable_actuators = FF_ENABLE_RIGHT;
		hid_hw_output_report(hdev, (u8 *)&ff_pck, sizeof(ff_pck));
		mdelay(330);

		ff_pck.ff.enable_actuators = FF_ENABLE_LEFT;
		hid_hw_output_report(hdev, (u8 *)&ff_pck, sizeof(ff_pck));
		mdelay(330);

		ff_pck.ff.enable_actuators
			= FF_ENABLE_RIGHT_TRIGGER | FF_ENABLE_LEFT_TRIGGER;
		hid_hw_output_report(hdev, (u8 *)&ff_pck, sizeof(ff_pck));
		mdelay(330);
	}


	/* Init Input System for Force Feedback (FF) */
	input_set_capability(idev, EV_FF, FF_RUMBLE);
	error = input_ff_create_memless(idev, NULL, xpadneo_ff_play);
	if (error)
		return error;


	/*
	 * Set default values, otherwise tools which depend on the joystick
	 * subsystem, report arbitrary values until the first real event
	 * TODO: Is this really necessary?
	 */
	input_report_abs(idev, ABS_X,      0);
	input_report_abs(idev, ABS_Y,      0);
	input_report_abs(idev, ABS_Z,      0);
	input_report_abs(idev, ABS_RX,     0);
	input_report_abs(idev, ABS_RY,     0);
	input_report_abs(idev, ABS_RZ,     0);
	input_report_key(idev, BTN_A,      0);
	input_report_key(idev, BTN_B,      0);
	input_report_key(idev, BTN_X,      0);
	input_report_key(idev, BTN_Y,      0);
	input_report_key(idev, BTN_TR,     0);
	input_report_key(idev, BTN_TL,     0);
	input_report_key(idev, BTN_THUMBL, 0);
	input_report_key(idev, BTN_THUMBR, 0);
	input_report_key(idev, BTN_START,  0);
	input_report_key(idev, BTN_MODE,   0);
	input_report_key(idev, ABS_HAT0X,  0);
	input_report_key(idev, ABS_HAT0Y,  0);
	input_sync(idev);

	/* TODO: - do not hardcode codes and values but
	 *         keep them in the mapping structures
	 *       - maybe initDevice isn't the right place
	 */

	return 0;
}


/* Callback function which return the available properties to userspace */
static int battery_get_property(struct power_supply *ps,
	enum power_supply_property property, union power_supply_propval *val)
{
	struct xpadneo_devdata *xdata = power_supply_get_drvdata(ps);
	unsigned long flags;
	u8 capacity_level, present, online, status;

	spin_lock_irqsave(&xdata->lock, flags);
	capacity_level = xdata->ps_capacity_level;
	present        = xdata->ps_present;
	online         = xdata->ps_online;
	status         = xdata->ps_status;
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
		/* pass the driver instance xpadneo_data to the get_property function */
		.drv_data = xdata
	};


	/* Set up power supply */

	/* Set the battery capacity to 'full' until we get our first real
	 * battery event. Prevents false "critical low battery" notifications
	 */
	xdata->ps_capacity_level = POWER_SUPPLY_CAPACITY_LEVEL_FULL;

	/* NOTE: hdev->uniq is meant to be the MAC address and hence
	 *       it should be unique. Unfortunately, here it is not unique
	 *       neither is it the bluetooth MAC address.
	 *       As a solution we add an unique id for every gamepad.
	 */

	xdata->batt_desc.name = kasprintf(GFP_KERNEL, "xpadneo_batt_%pMR_%i",
				hdev->uniq, xdata->id);
	if (!xdata->batt_desc.name)
		return -ENOMEM;

	xdata->batt_desc.type = POWER_SUPPLY_TYPE_BATTERY;

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

u8 map_hid_to_input_windows(struct hid_usage *usage, struct input_ev *map_to)
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
			*map_to = (struct input_ev){EV_KEY, BTN_A};
			return MAP_STATIC;
		case 0x02:
			*map_to = (struct input_ev){EV_KEY, BTN_B};
			return MAP_STATIC;
		case 0x03:
			*map_to = (struct input_ev){EV_KEY, BTN_X};
			return MAP_STATIC;
		case 0x04:
			*map_to = (struct input_ev){EV_KEY, BTN_Y};
			return MAP_STATIC;
		case 0x05:
			*map_to = (struct input_ev){EV_KEY, BTN_TL};
			return MAP_STATIC;
		case 0x06:
			*map_to = (struct input_ev){EV_KEY, BTN_TR};
			return MAP_STATIC;
		case 0x07:
			*map_to = (struct input_ev){EV_KEY, BTN_SELECT};
			return MAP_STATIC;
		case 0x08:
			*map_to = (struct input_ev){EV_KEY, BTN_START};
			return MAP_STATIC;
		case 0x09:
			*map_to = (struct input_ev){EV_KEY, BTN_THUMBL};
			return MAP_STATIC;
		case 0x0A:
			*map_to = (struct input_ev){EV_KEY, BTN_THUMBR};
			return MAP_STATIC;
		}
	case HID_UP_GENDESK:
		switch (hid_usage) {
		case 0x30:
			*map_to = (struct input_ev){EV_ABS, ABS_X};
			return MAP_STATIC;
		case 0x31:
			*map_to = (struct input_ev){EV_ABS, ABS_Y};
			return MAP_STATIC;
		case 0x32:
			*map_to = (struct input_ev){EV_ABS, ABS_Z};
			return MAP_STATIC;
		case 0x33:
			*map_to = (struct input_ev){EV_ABS, ABS_RX};
			return MAP_STATIC;
		case 0x34:
			*map_to = (struct input_ev){EV_ABS, ABS_RY};
			return MAP_STATIC;
		case 0x35:
			*map_to = (struct input_ev){EV_ABS, ABS_RZ};
			return MAP_STATIC;
		case 0x39:
			*map_to = (struct input_ev){0, 0};
			return MAP_AUTO;
		case 0x85:
			*map_to = (struct input_ev){EV_KEY, BTN_MODE};
			return MAP_STATIC;
		}
	}

	return MAP_IGNORE;
}

u8 map_hid_to_input_linux(struct hid_usage *usage, struct input_ev *map_to)
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
			*map_to = (struct input_ev){EV_KEY, BTN_A};
			return MAP_STATIC;
		case 0x02:
			*map_to = (struct input_ev){EV_KEY, BTN_B};
			return MAP_STATIC;
		case 0x04:
			*map_to = (struct input_ev){EV_KEY, BTN_X};
			return MAP_STATIC;
		case 0x05:
			*map_to = (struct input_ev){EV_KEY, BTN_Y};
			return MAP_STATIC;
		case 0x07:
			*map_to = (struct input_ev){EV_KEY, BTN_TL};
			return MAP_STATIC;
		case 0x08:
			*map_to = (struct input_ev){EV_KEY, BTN_TR};
			return MAP_STATIC;
		case 0x0C:
			*map_to = (struct input_ev){EV_KEY, BTN_START};
			return MAP_STATIC;
		case 0x0E:
			*map_to = (struct input_ev){EV_KEY, BTN_THUMBL};
			return MAP_STATIC;
		case 0x0F:
			*map_to = (struct input_ev){EV_KEY, BTN_THUMBR};
			return MAP_STATIC;
		}
	case HID_UP_CONSUMER:
		switch (hid_usage) {
		case 0x223:
			*map_to = (struct input_ev){EV_KEY, BTN_MODE};
			return MAP_STATIC;
		case 0x224:
			*map_to = (struct input_ev){EV_KEY, BTN_SELECT};
			return MAP_STATIC;
		}
	case HID_UP_GENDESK:
		switch (hid_usage) {
		case 0x30:
			*map_to = (struct input_ev){EV_ABS, ABS_X};
			return MAP_STATIC;
		case 0x31:
			*map_to = (struct input_ev){EV_ABS, ABS_Y};
			return MAP_STATIC;
		case 0x32:
			*map_to = (struct input_ev){EV_ABS, ABS_RX};
			return MAP_STATIC;
		case 0x35:
			*map_to = (struct input_ev){EV_ABS, ABS_RY};
			return MAP_STATIC;
		case 0x39:
			*map_to = (struct input_ev){0, 0};
			return MAP_AUTO;
		}
	case HID_UP_SIMULATION:
		switch (hid_usage) {
		case 0xC4:
			*map_to = (struct input_ev){EV_ABS, ABS_RZ};
			return MAP_STATIC;
		case 0xC5:
			*map_to = (struct input_ev){EV_ABS, ABS_Z};
			return MAP_STATIC;
		}
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
		RET_MAP_IGNORE = -1,   /* completely ignore this input */
		RET_MAP_AUTO,        /* let hid-core autodetect the mapping */
		RET_MAP_STATIC       /* mapped by hand, no further processing */
	};

	struct input_ev map_to;
	u8 (*perform_mapping)(struct hid_usage *usage, struct input_ev *map_to);
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);


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
		usage->hid & HID_USAGE_PAGE, usage->hid & HID_USAGE);

		return RET_MAP_AUTO;

	case MAP_IGNORE:
		hid_dbg_lvl(DBG_LVL_FEW, hdev,
		"UP: 0x%04X, USG: 0x%04X -> ignored\n",
		usage->hid & HID_USAGE_PAGE, usage->hid & HID_USAGE);

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
 * one and return a pointer to it. In the latter, you may have to
 * modify the rsize value too.
 */

static u8 *xpadneo_report_fixup(struct hid_device *hdev, u8 *rdesc,
				unsigned int *rsize)
{
	hid_dbg_lvl(DBG_LVL_SOME, hdev,	"REPORT (DESCRIPTOR) FIXUP HOOK\n");
	dbg_hex_dump_lvl(DBG_LVL_FEW, "xpadneo: report-descr: ", rdesc, *rsize);

	return rdesc;
}


static void parse_raw_event_battery(struct hid_device *hdev, u8 *data,
				    int reportsize)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	unsigned long flags;
	u8 capacity_level, present, online, status;


	/*  msb          ID 04          lsb
	 * +---+---+---+---+---+---+---+---+
	 * | O | R | E | C | M | M | L | L |
	 * +---+---+---+---+---+---+---+---+
	 *
	 * O: Online
	 * R: Reserved / Unused
	 * E: Error (?) / Unknown
	 * C: Charging, I mean really charging the battery (P 'n C)
	 *              not (only) the power cord powering the controller
	 * M M: Mode
	 *   00: Powered by USB
	 *   01: Powered by (disposable) batteries
	 *   10: Powered by Play 'n Charge battery pack (only, no cable)
	 * L L: Capacity Level
	 *   00: (Super) Critical
	 *   01: Low
	 *   10: Medium
	 *   11: Full
	 */


	/* I think "online" means whether the dev is online or shutting down */
	online = (data[1] & 0x80) >> 7;

	/* The _battery_ is only present if not powered by USB */
	present = ((data[1] & 0x0C) != 0x00);

	/* Capacity level, only valid as long as the battery is present */
	switch (data[1] & 0x03) {
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

	/* Is the (Play 'n Charge) battery charging right now? */
	switch ((data[1] & 0x10) >> 4) {
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

static void check_report_behaviour(struct hid_device *hdev, u8 *data,
				   int reportsize)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	/*
	 * The length of the first input report with an ID of 0x01
	 * reveals which report-type the controller is actually
	 * sending (windows: 16, or linux: 17).
	 */
	if (xdata->report_behaviour == UNKNOWN) {
		switch (reportsize) {
		case 16:
			xdata->report_behaviour = WINDOWS;
			break;
		case 17:
			xdata->report_behaviour = LINUX;
			break;
		default:
			xdata->report_behaviour = UNKNOWN;
			break;
		}
	}

	hid_dbg_lvl(DBG_LVL_SOME, hdev, "desc: %s, beh: %s\n",
		report_type_text[xdata->report_descriptor],
		report_type_text[xdata->report_behaviour]);

	/* TODO:
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

/*
 * HID Raw Event Hook
 */

int xpadneo_raw_event(struct hid_device *hdev, struct hid_report *report,
		      u8 *data, int reportsize)
{
	/* Return Codes */
	enum {
		RAWEV_CONT_PROCESSING, /* Let the hid-core autodetect the event */
		RAWEV_STOP_PROCESSING  /* Stop further processing */
	};

	dbg_hex_dump_lvl(DBG_LVL_SOME, "xpadneo: raw_event: ", data, reportsize);

	switch (report->id) {
	case 01:
		check_report_behaviour(hdev, data, reportsize);
		break;
	case 04:
		parse_raw_event_battery(hdev, data, reportsize);
		return RAWEV_STOP_PROCESSING;
	}

	/* Continue processing */
	return RAWEV_CONT_PROCESSING;
}


void xpadneo_report(struct hid_device *hdev, struct hid_report *report)
{
	hid_dbg_lvl(DBG_LVL_SOME, hdev, "REPORT HOOK\n");
}


/*
 * Input Configured Hook
 *
 * Called as soon as the Input Device is able to get registered.
 *
 */

static int xpadneo_input_configured(struct hid_device *hdev,
				    struct hid_input *hi)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	/* set a pointer to the logical input device at the device structure */
	xdata->idev = hi->input;

	hid_dbg_lvl(DBG_LVL_SOME, hdev, "INPUT CONFIGURED HOOK\n");

	if (param_fake_dev_version) {
		xdata->idev->id.version = (u16) param_fake_dev_version;
		hid_dbg_lvl(DBG_LVL_FEW, hdev, "Fake device version: 0x%04X\n",
			param_fake_dev_version);
	}


	// The HID device descriptor defines a range from 0 to 65535 for all
	// absolute axis (like ABS_X), this is in contrary to what the linux
	// gamepad specification demands [–32.768; 32.767].
	// Therefore, we have to set the min, max, fuzz and flat values by hand:

	input_set_abs_params(xdata->idev, ABS_X, -32768, 32767, 255, 4095);
	input_set_abs_params(xdata->idev, ABS_Y, -32768, 32767, 255, 4095);

	input_set_abs_params(xdata->idev, ABS_RX, -32768, 32767, 255, 4095);
	input_set_abs_params(xdata->idev, ABS_RY, -32768, 32767, 255, 4095);

	if (param_combined_z_axis)
		input_set_abs_params(xdata->idev, ABS_Z, -1024, 1023, 3, 63);

	// furthermore, we need to translate the incoming events to fit within
	// the new range, we will do that in the xpadneo_event() hook.

	// Remove the ABS_RZ event if param_combined_z_axis is enabled
	if (param_combined_z_axis) {
		__clear_bit(ABS_RZ, xdata->idev->absbit);
	}

	return 0;
}





#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
static void switch_mode(struct timer_list *t) {

	struct xpadneo_devdata *xdata = from_timer(xdata, t, timer);
	struct hid_device *hdev = xdata->hdev;
	struct ff_report ff_pck;

	xdata->mode_gp = !xdata->mode_gp;

	/* TODO: outsource that */

	ff_pck.ff = ff_clear;

	if (!param_disable_ff) {
		ff_pck.report_id = 0x03;
		ff_pck.ff.magnitude_right = 0x80;
		ff_pck.ff.magnitude_left  = 0x80;
		ff_pck.ff.duration = 10;
		ff_pck.ff.enable_actuators = FF_ENABLE_RIGHT | FF_ENABLE_LEFT;

		hid_hw_output_report(hdev, (u8 *)&ff_pck, sizeof(ff_pck));
		mdelay(ff_pck.ff.duration * 10); // TODO: busy waiting?!
	}

	if (xdata->mode_gp) {
		hid_dbg_lvl(DBG_LVL_ALL, hdev, "mode switched to: Gamepad\n");
	} else {
		hid_dbg_lvl(DBG_LVL_ALL, hdev, "mode switched to: Mouse\n");
	}

}
#endif


/*
 * Event Hook
 *
 * This hook is called whenever an event occurs that is listed on
 * xpadneo_driver.usage_table (which is NULL in our case, therefore it is
 * invoked on every event).
 *
 * Before we can send additional input events, we have to enable
 * the corresponding keys in xpadneo_input_configured.
 */

int xpadneo_event(struct hid_device *hdev, struct hid_field *field,
		  struct hid_usage *usage, __s32 value)
{
	/* Return Codes */
	enum {
		EV_CONT_PROCESSING, /* Let the hid-core autodetect the event */
		EV_STOP_PROCESSING  /* Stop further processing */
	};

	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct input_dev *idev = xdata->idev;

	u16 usg_type = usage->type;
	u16 usg_code = usage->code;



#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)

	// MODE SWITCH DETECTION

	if (usg_type == EV_KEY && usg_code == BTN_MODE) {
		if (value == 1) {
			mod_timer(&xdata->timer, jiffies + msecs_to_jiffies(2000));
		} else {
			del_timer_sync(&xdata->timer);
		}
	}

	// MOUSE MODE HANDLING

	if (!(xdata->mode_gp) && use_vmouse) {

		if (usg_type == EV_ABS) {

			int centered_value = value - 32768;
			int mouse_value = 0;

			switch (usg_code) {
			case ABS_X:
				mouse_value = centered_value / 3000;
				__vmouse_movement(1, mouse_value);
				break;
			case ABS_Y:
				mouse_value = centered_value / 3000;
				__vmouse_movement(0, mouse_value);
				break;
			case ABS_RY:
				mouse_value = -(centered_value / 16384);
				__vmouse_wheel(mouse_value);
				break;
			}

		} else if (usg_type == EV_KEY) {

			switch (usg_code) {
			case BTN_A:
				__vmouse_leftclick(value);
				break;
			case BTN_B:
				__vmouse_rightclick(value);
				break;
			}

		}

		// do not report GP events in mouse mode
		return EV_STOP_PROCESSING;
	}
#endif


	hid_dbg_lvl(DBG_LVL_ALL, hdev,
		"hid-up: %02x, hid-usg: %02x, input-code: %02x, value: %02x\n",
		(usage->hid & HID_USAGE_PAGE), (usage->hid & HID_USAGE),
		usage->code, value);


	// we have to shift the range of the analogues sticks (ABS_X/Y/RX/RY)
	// as already explained in xpadneo_input_configured() above
	// furthermore we need to combine ABS_Z and ABS_RZ if param_combined_z_axis
	// is set

	if (usg_type == EV_ABS) {
		if (usg_code == ABS_X || usg_code == ABS_Y
				|| usg_code == ABS_RX || usg_code == ABS_RY) {
			hid_dbg_lvl(DBG_LVL_ALL, hdev, "shifted axis %02x, old value: %i, new value: %i\n", usg_code, value, value - 32768);
			input_report_abs(idev, usg_code, value - 32768);
			goto sync_and_stop_processing;
		}

		if (param_combined_z_axis) {
			if (usg_code == ABS_Z || usg_code == ABS_RZ) {
				if (usg_code == ABS_Z)
					xdata->last_abs_z = value;
				if (usg_code == ABS_RZ)
					xdata->last_abs_rz = value;

				input_report_abs(idev, ABS_Z, 0 - xdata->last_abs_z + xdata->last_abs_rz);
				goto sync_and_stop_processing;
			}
		}
	}


	/* TODO:
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
	xdata->mode_gp = true;
	/* Unknown until first report with ID 01 arrives (see raw_event) */
	xdata->report_behaviour = UNKNOWN;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
	timer_setup(&xdata->timer, switch_mode, 0);
#endif

	switch (hdev->dev_rsize) {
	case 307:
		xdata->report_descriptor = WINDOWS;
		break;
	case 335:
		xdata->report_descriptor = LINUX;
		break;
	default:
		xdata->report_descriptor = UNKNOWN;
		break;
	}

	hid_set_drvdata(hdev, xdata);


	/* Parse the raw report (includes a call to report_fixup) */
	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "parse failed\n");
		return ret;
	}

	/* Debug Output*/
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
	hid_dbg_lvl(DBG_LVL_FEW, hdev, "* physical location: %pMR\n", hdev->phys);

	/* We start our hardware without FF, we will add it afterwards by hand
	 * HID_CONNECT_DEFAULT = (HID_CONNECT_HIDINPUT | HID_CONNECT_HIDRAW
	 *                        | HID_CONNECT_HIDDEV | HID_CONNECT_FF)
	 * Our Input Device is created automatically since we defined
	 * HID_CONNECT_HIDINPUT as one of the flags.
	 */
	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT & ~HID_CONNECT_FF);
	if (ret) {
		hid_err(hdev, "hw start failed\n");
		return ret;
	}

	/* Call the device initialization routines */
	ret = xpadneo_initDevice(hdev);
	if (ret) {
		hid_err(hdev, "device initialization failed\n");
		return ret;
	}

	ret = xpadneo_initBatt(hdev);
	if (ret) {
		hid_err(hdev, "battery initialization failed\n");
		return ret;
	}

	/* Everything is fine */
	return 0;
}


static void xpadneo_remove_device(struct hid_device *hdev)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);

	hid_hw_close(hdev);

	/* Cleaning up here */
	ida_simple_remove(&xpadneo_device_id_allocator, xdata->id);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
	del_timer_sync(&xdata->timer);
#endif

	hid_hw_stop(hdev);

	hid_dbg_lvl(DBG_LVL_FEW, hdev, "Goodbye %s!\n", hdev->name);
}



/*
 * Device ID Structure, define all supported devices here
 */

static const struct hid_device_id xpadneo_devices[] = {

	/*
	 * The ProductID is somehow related to the Firmware Version,
	 * but it also changed back from 0x02FD (newer fw) to 0x02E0 (older one)
	 * and vice versa on one controller here.
	 *
	 * Unfortunately you cannot tell from product id how the gamepad really
	 * behaves on reports, since the newer firmware supports both mappings
	 * (the one which is standard in linux and the old one, which is still
	 * used in windows).
	 */

	/* XBOX ONE S / X */
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x02FD) },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_MICROSOFT, 0x02E0) },
	/* SENTINEL VALUE, indicates the end*/
	{ }
};

static struct hid_driver xpadneo_driver = {
	/* The name of the driver */
	.name = "xpadneo",

	/* Which devices is this driver for */
	.id_table = xpadneo_devices,

	/* Hooked as the input device is configured (before it is registered)
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
	pr_info("%s: hello there (kernel %06X)!\n", xpadneo_driver.name, LINUX_VERSION_CODE);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
	pr_info("%s: kernel version < 4.14.0, no mouse support!\n", xpadneo_driver.name);
#endif

	/* Init pointers to vmouse */
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
	__vmouse_movement = symbol_get(vmouse_movement);
	if (!__vmouse_movement)
		use_vmouse = 0;

	__vmouse_leftclick = symbol_get(vmouse_leftclick);
	if (!__vmouse_leftclick)
		use_vmouse = 0;

	__vmouse_rightclick = symbol_get(vmouse_rightclick);
	if (!__vmouse_rightclick)
		use_vmouse = 0;

	__vmouse_wheel = symbol_get(vmouse_wheel);
	if (!vmouse_wheel)
		use_vmouse = 0;

	if (use_vmouse) {
		pr_info("%s: vmouse support enabled!\n", xpadneo_driver.name);
	} else {
		pr_info("%s: vmouse support disabled!\n", xpadneo_driver.name);
	}

	#endif

	return hid_register_driver(&xpadneo_driver);
}

static void __exit xpadneo_exitModule(void)
{
	/* we will not use vmouse anymore */
	if (__vmouse_movement) symbol_put(vmouse_movement);
	if (__vmouse_leftclick) symbol_put(vmouse_leftclick);
	if (__vmouse_rightclick) symbol_put(vmouse_rightclick);
	if (__vmouse_wheel) symbol_put(vmouse_wheel);



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

