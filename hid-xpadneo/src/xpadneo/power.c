// SPDX-License-Identifier: GPL-2.0-only

/*
 * xpadneo power driver
 *
 * Copyright (c) 2017 Florian Dollinger <dollinger.florian@gmx.de>
 * Copyright (c) 2026 Kai Krakow <kai@kaishome.de>
 */

#include <linux/power_supply.h>

#include "xpadneo.h"

static enum power_supply_property xpadneo_battery_props[] = {
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_SCOPE,
	POWER_SUPPLY_PROP_STATUS,
};

#define XPADNEO_PSY_ONLINE(data)     ((data&0x80) > 0)
#define XPADNEO_PSY_MODE(data)       ((data&0x0C)>>2)

#define XPADNEO_POWER_USB(data)      (XPADNEO_PSY_MODE(data) == 0)
#define XPADNEO_POWER_BATTERY(data)  (XPADNEO_PSY_MODE(data) == 1)
#define XPADNEO_POWER_PNC(data)      (XPADNEO_PSY_MODE(data) == 2)

#define XPADNEO_BATTERY_ONLINE(data)         ((data&0x0C) > 0)
#define XPADNEO_BATTERY_CHARGING(data)       ((data&0x10) > 0)
#define XPADNEO_BATTERY_CAPACITY_LEVEL(data) (data&0x03)

static int get_psy_property(struct power_supply *psy,
			    enum power_supply_property property, union power_supply_propval *val)
{
	struct xpadneo_devdata *xdata = power_supply_get_drvdata(psy);
	int ret = 0;

	static int capacity_level_map[] = {
		[0] = POWER_SUPPLY_CAPACITY_LEVEL_LOW,
		[1] = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL,
		[2] = POWER_SUPPLY_CAPACITY_LEVEL_HIGH,
		[3] = POWER_SUPPLY_CAPACITY_LEVEL_FULL,
	};

	u8 flags = xdata->battery.flags;

	/*
	 * Clamp to the known power_supply range until higher-capacity states are modeled
	 * explicitly.
	 */
	u8 level = min(3, XPADNEO_BATTERY_CAPACITY_LEVEL(flags));
	bool online = XPADNEO_PSY_ONLINE(flags);
	bool charging = XPADNEO_BATTERY_CHARGING(flags);

	switch (property) {
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		if (online && XPADNEO_BATTERY_ONLINE(flags))
			val->intval = capacity_level_map[level];
		else
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN;
		break;

	case POWER_SUPPLY_PROP_MODEL_NAME:
		if (online && XPADNEO_POWER_PNC(flags))
			val->strval = xdata->battery.name_pnc;
		else
			val->strval = xdata->battery.name;
		break;

	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = online && XPADNEO_BATTERY_ONLINE(flags);
		break;

	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = online && (XPADNEO_BATTERY_ONLINE(flags) || charging);
		break;

	case POWER_SUPPLY_PROP_SCOPE:
		val->intval = POWER_SUPPLY_SCOPE_DEVICE;
		break;

	case POWER_SUPPLY_PROP_STATUS:
		if (online && charging)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else if (online && !charging && XPADNEO_POWER_USB(flags))
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else if (online && XPADNEO_BATTERY_ONLINE(flags))
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int setup_psy(struct hid_device *hdev)
{
	struct xpadneo_devdata *xdata = hid_get_drvdata(hdev);
	struct power_supply *psy;
	struct power_supply_config ps_config = {
		.drv_data = xdata
	};
	char *name;

	/* already registered */
	if (xdata->battery.psy)
		return 0;

	name = devm_kasprintf(&hdev->dev, GFP_KERNEL, "xpadneo_battery_%i", xdata->id);
	if (!name)
		return -ENOMEM;

	xdata->battery.desc.name = name;
	xdata->battery.desc.properties = xpadneo_battery_props;
	xdata->battery.desc.num_properties = ARRAY_SIZE(xpadneo_battery_props);
	xdata->battery.desc.use_for_apm = 0;
	xdata->battery.desc.get_property = get_psy_property;
	xdata->battery.desc.type = POWER_SUPPLY_TYPE_BATTERY;

	/* register battery via device manager */
	psy = devm_power_supply_register(&hdev->dev, &xdata->battery.desc, &ps_config);
	if (IS_ERR(psy)) {
		hid_err(hdev, "battery registration failed\n");
		return PTR_ERR(psy);
	}

	xdata->battery.psy = psy;
	hid_info(hdev, "battery registered\n");

	power_supply_powers(xdata->battery.psy, &xdata->hdev->dev);

	return 0;
}

void xpadneo_power_update(struct xpadneo_devdata *xdata, u8 value)
{
	int old_value = xdata->battery.flags;

	if (!xdata->battery.initialized && XPADNEO_PSY_ONLINE(value)) {
		xdata->battery.initialized = true;
		setup_psy(xdata->hdev);
	}

	if (!xdata->battery.psy)
		return;

	xdata->battery.flags = value;
	if (old_value != value) {
		if (!XPADNEO_PSY_ONLINE(value))
			hid_info(xdata->hdev, "shutting down\n");
		power_supply_changed(xdata->battery.psy);
	}
}

int xpadneo_power_init(struct xpadneo_devdata *xdata)
{
	struct device *dev = &xdata->hdev->dev;

	xdata->battery.name =
	    devm_kasprintf(dev, GFP_KERNEL, "%s [%s]", xdata->gamepad.idev->name,
			   xdata->gamepad.idev->uniq);
	if (!xdata->battery.name)
		return -ENOMEM;

	xdata->battery.name_pnc =
	    devm_kasprintf(dev, GFP_KERNEL, "%s [%s] Play'n Charge Kit", xdata->gamepad.idev->name,
			   xdata->gamepad.idev->uniq);
	if (!xdata->battery.name_pnc)
		return -ENOMEM;

	return 0;
}

void xpadneo_power_remove(struct xpadneo_devdata *xdata)
{
	struct hid_device *hdev = xdata->hdev;

	xdata->battery.name = NULL;
	xdata->battery.name_pnc = NULL;

	hid_info(hdev, "battery removed\n");
}
