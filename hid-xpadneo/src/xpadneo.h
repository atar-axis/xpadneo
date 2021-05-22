/*
 * Force feedback support for XBOX ONE S and X gamepads via Bluetooth
 *
 * This driver was developed for a student project at fortiss GmbH in Munich.
 * Copyright (c) 2017 Florian Dollinger <dollinger.florian@gmx.de>
 *
 * Additional features and code redesign
 * Copyright (c) 2020 Kai Krakow <kai@kaishome.de>
 */

#ifndef XPADNEO_H_FILE
#define XPADNEO_H_FILE

#include <linux/hid.h>
#include <linux/version.h>

#include "hid-ids.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,18,0)
#error "kernel version 4.18.0+ required for HID_QUIRK_INPUT_PER_APP"
#endif

#ifndef VERSION
#error "xpadneo version not defined"
#endif

#define XPADNEO_VERSION __stringify(VERSION)

/* helper for printing a notice only once */
#ifndef hid_notice_once
#define hid_notice_once(hid, fmt, ...)					\
do {									\
	static bool __print_once __read_mostly;				\
	if (!__print_once) {						\
		__print_once = true;					\
		hid_notice(hid, fmt, ##__VA_ARGS__);			\
	}								\
} while (0)
#endif

/* benchmark helper */
#define xpadneo_benchmark_start(name) \
do { \
	unsigned long __##name_jiffies = jiffies; \
	pr_info("xpadneo " #name " start\n")
#define xpadneo_benchmark_stop(name) \
	pr_info("xpadneo " #name " took %ums\n", jiffies_to_msecs(jiffies - __##name_jiffies)); \
} while (0)

/* button aliases */
#define BTN_SHARE KEY_RECORD
#define BTN_XBOX  KEY_MODE

/* module parameter "trigger_rumble_mode" */
#define PARAM_TRIGGER_RUMBLE_PRESSURE    0
#define PARAM_TRIGGER_RUMBLE_DIRECTIONAL 1
#define PARAM_TRIGGER_RUMBLE_DISABLE     2

/* module parameter "quirks" */
#define XPADNEO_QUIRK_NO_PULSE          1
#define XPADNEO_QUIRK_NO_TRIGGER_RUMBLE 2
#define XPADNEO_QUIRK_NO_MOTOR_MASK     4
#define XPADNEO_QUIRK_USE_HW_PROFILES   8
#define XPADNEO_QUIRK_LINUX_BUTTONS     16
#define XPADNEO_QUIRK_NINTENDO          32
#define XPADNEO_QUIRK_SHARE_BUTTON      64

/* timing of rumble commands to work around firmware crashes */
#define XPADNEO_RUMBLE_THROTTLE_DELAY   msecs_to_jiffies(10)
#define XPADNEO_RUMBLE_THROTTLE_JIFFIES (jiffies + XPADNEO_RUMBLE_THROTTLE_DELAY)

/* rumble motors enable bits */
enum xpadneo_rumble_motors {
	FF_RUMBLE_NONE = 0x00,
	FF_RUMBLE_WEAK = 0x01,
	FF_RUMBLE_STRONG = 0x02,
	FF_RUMBLE_MAIN = FF_RUMBLE_WEAK | FF_RUMBLE_STRONG,
	FF_RUMBLE_RIGHT = 0x04,
	FF_RUMBLE_LEFT = 0x08,
	FF_RUMBLE_TRIGGERS = FF_RUMBLE_LEFT | FF_RUMBLE_RIGHT,
	FF_RUMBLE_ALL = 0x0F
} __packed;

/* rumble packet structure */
struct ff_data {
	enum xpadneo_rumble_motors enable;
	u8 magnitude_left;
	u8 magnitude_right;
	u8 magnitude_strong;
	u8 magnitude_weak;
	u8 pulse_sustain_10ms;
	u8 pulse_release_10ms;
	u8 loop_count;
} __packed;
#ifdef static_assert
static_assert(sizeof(struct ff_data) == 8);
#endif

/* report number for rumble commands */
#define XPADNEO_XB1S_FF_REPORT 0x03

/* maximum length of report 0x01 for duplicate packet filtering */
#define XPADNEO_REPORT_0x01_LENGTH (55+1)

/* HID packet for rumble commands */
struct ff_report {
	u8 report_id;
	struct ff_data ff;
} __packed;
#ifdef static_assert
static_assert(sizeof(struct ff_report) == 9);
#endif

/* trigger range limits implemented in XBE2 controllers */
enum xpadneo_trigger_scale {
	XBOX_TRIGGER_SCALE_FULL,
	XBOX_TRIGGER_SCALE_HALF,
	XBOX_TRIGGER_SCALE_DIGITAL,
	XBOX_TRIGGER_SCALE_NUM
} __packed;

#define XPADNEO_MISSING_CONSUMER 1
#define XPADNEO_MISSING_GAMEPAD  2
#define XPADNEO_MISSING_KEYBOARD 4

/* private driver instance data */
struct xpadneo_devdata {
	/* unique physical device id (randomly assigned) */
	int id;

	/* logical device interfaces */
	struct hid_device *hdev;
	struct input_dev *consumer, *gamepad, *keyboard;
	short int missing_reported;

	/* quirk flags */
	unsigned int original_rsize;
	u32 quirks;

	/* profile switching */
	bool xbox_button_down, profile_switched;
	u8 profile;

	/* mouse mode */
	bool mouse_mode;

	/* trigger scale */
	struct {
		u8 left, right;
	} trigger_scale;

	/* battery information */
	struct {
		bool initialized;
		struct power_supply *psy;
		struct power_supply_desc desc;
		char *name;
		char *name_pnc;
		u8 report_id;
		u8 flags;
	} battery;

	/* duplicate report buffers */
	u8 input_report_0x01[XPADNEO_REPORT_0x01_LENGTH];

	/* axis states */
	u8 count_abs_z_rz;
	s32 last_abs_z;
	s32 last_abs_rz;

	/* buffer for ff_worker */
	spinlock_t ff_lock;
	struct delayed_work ff_worker;
	unsigned long ff_throttle_until;
	bool ff_scheduled;
	struct ff_data ff;
	struct ff_data ff_shadow;
	void *output_report_dmabuf;
};

extern int xpadneo_init_consumer(struct xpadneo_devdata *);
extern int xpadneo_init_synthetic(struct xpadneo_devdata *, char *, struct input_dev **);

#endif
