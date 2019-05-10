#define DRV_VER "@DO_NOT_CHANGE@"

#include <linux/hid.h>
#include <linux/power_supply.h>
#include <linux/input.h>	/* ff_memless(), ... */
#include <linux/module.h>	/* MODULE_*, module_*, ... */
#include <linux/slab.h>		/* kzalloc(), kfree(), ... */
#include <linux/delay.h>	/* mdelay(), ... */
#include <linux/timer.h>	/* Interrupt Ticks (Timer) */

#include "hid-ids.h"		/* VENDOR_ID... */

#define DEBUG


/* Module Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Florian Dollinger <dollinger.florian@gmx.de>");
MODULE_DESCRIPTION("Linux kernel driver for Xbox ONE S+ gamepads (BT), incl. FF");
MODULE_VERSION(DRV_VER);


extern void vmouse_movement(int, int);
extern void vmouse_leftclick(int);


/* Module Parameters, located at /sys/module/hid_xpadneo/parameters */

/* NOTE:
 * In general it is not guaranteed that a short variable is no more than
 * 16 bit long in C, it depends on the computer architecure. But as a kernel
 * module parameter it is, since <params.c> does use kstrtou16 for shorts
 * since version 3.14
 */

#ifdef DEBUG
static u8 param_debug_level;
module_param_named(debug_level, param_debug_level, byte, 0644);
MODULE_PARM_DESC(debug_level, "(u8) Debug information level: 0 (none) to 3+ (most verbose).");
#endif

static u8 param_disable_ff;
module_param_named(disable_ff, param_disable_ff, byte, 0644);
MODULE_PARM_DESC(disable_ff, "(u8) Disable FF: 0 (all enabled), 1 (disable main), 2 (disable triggers), 3 (disable all).");

#define PARAM_DISABLE_FF_NONE    0
#define PARAM_DISABLE_FF_MAIN    1
#define PARAM_DISABLE_FF_TRIGGER 2
#define PARAM_DISABLE_FF_ALL     3

static bool param_combined_z_axis;
module_param_named(combined_z_axis, param_combined_z_axis, bool, 0644);
MODULE_PARM_DESC(combined_z_axis, "(bool) Combine the triggers to form a single axis. 1: combine, 0: do not combine");

static u8 param_trigger_rumble_damping = 4;
module_param_named(trigger_rumble_damping, param_trigger_rumble_damping, byte, 0644);
MODULE_PARM_DESC(trigger_rumble_damping, "(u8) Damp the trigger: 1 (none) to 2^8+ (max)");

static u16 param_fake_dev_version = 0x1130;
module_param_named(fake_dev_version, param_fake_dev_version, ushort, 0644);
MODULE_PARM_DESC(fake_dev_version, "(u16) Fake device version # to hide from SDL's mappings. 0x0001-0xFFFF: fake version, others: keep original");


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
			hid_printk(KERN_DEBUG, pr_fmt(fmt_hdev), \
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
	FF_ENABLE_NONE          = 0x00,
	FF_ENABLE_RIGHT         = 0x01,
	FF_ENABLE_LEFT          = 0x02,
	FF_ENABLE_RIGHT_TRIGGER = 0x04,
	FF_ENABLE_LEFT_TRIGGER  = 0x08,
	FF_ENABLE_ALL           = 0x0F
};

struct ff_data {
	u8 enable_actuators;
	u8 magnitude_left_trigger;
	u8 magnitude_right_trigger;
	u8 magnitude_left;
	u8 magnitude_right;
	u8 duration;
	u8 start_delay;
	u8 loop_count;
} __packed;

struct ff_report {
	u8 report_id;
	struct ff_data ff;
} __packed;

/* static variables are zeroed => empty initialization struct */
static const struct ff_data ff_clear;


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

	/* pointer / gamepad mode */
	bool mode_gp;

	/* timers */
	struct timer_list timer;

	/* battery information */
	struct power_supply_desc batt_desc;
	u8 ps_online;
	u8 ps_present;
	u8 ps_capacity_level;
	u8 ps_status;

	/* axis states */
	s32 last_abs_z;
	s32 last_abs_rz;
};