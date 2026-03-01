/*
 * Force feedback support for XBOX ONE S and X gamepads via Bluetooth
 *
 * This driver was developed for a student project at fortiss GmbH in Munich.
 * Copyright (c) 2017 Florian Dollinger <dollinger.florian@gmx.de>
 *
 * Additional features and code redesign
 * Copyright (c) 2020 Kai Krakow <kai@kaishome.de>
 */

#ifndef XPADNEO_H
#define XPADNEO_H

#include <linux/hid.h>
#include <linux/input.h>
#include <linux/power_supply.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <linux/workqueue.h>

/* v4.19: ida_simple_{get,remove}() have been replaced */
#if KERNEL_VERSION(4, 19, 0) >= LINUX_VERSION_CODE
#define ida_alloc(a, f) ida_simple_get((a), 0, 0, (f))
#define ida_free(a, f) ida_simple_remove((a), (f))
#endif

#if KERNEL_VERSION(4, 18, 0) > LINUX_VERSION_CODE
#error "kernel version 4.18.0+ required for HID_QUIRK_INPUT_PER_APP"
#endif

/* v6.15: del_timer_sync() renamed to timer_delete_sync() */
#if KERNEL_VERSION(6, 15, 0) > LINUX_VERSION_CODE
#define timer_delete_sync(t) del_timer_sync(t)
#endif

/* v6.16: from_timer() renamed to timer_container_of() */
#ifndef timer_container_of
#define timer_container_of(v, c, t) from_timer(v, c, t)
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

#ifndef USB_VENDOR_ID_MICROSOFT
#define USB_VENDOR_ID_MICROSOFT 0x045e
#endif

/* button aliases */
#define BTN_SHARE KEY_F12
#define BTN_XBOX  BTN_MODE

/* Paddle usage codes for kernel < 6.17 */
#ifndef BTN_GRIPL
#define BTN_GRIPL  0x224
#define BTN_GRIPR  0x225
#define BTN_GRIPL2 0x226
#define BTN_GRIPR2 0x227
#endif

/* XBE2 controllers support four profiles */
#define XPADNEO_XBE2_PROFILES_MAX 4

/* Profile usage code for kernel < 6.0-rc1 */
#define ABS_PROFILE 0x21

/* module parameter "trigger_rumble_mode" */
#define PARAM_TRIGGER_RUMBLE_PRESSURE 0
#define PARAM_TRIGGER_RUMBLE_RESERVED 1
#define PARAM_TRIGGER_RUMBLE_DISABLE  2

/* module parameter "quirks" */
#define XPADNEO_QUIRK_NO_PULSE          1
#define XPADNEO_QUIRK_NO_TRIGGER_RUMBLE 2
#define XPADNEO_QUIRK_NO_MOTOR_MASK     4
#define XPADNEO_QUIRK_USE_HW_PROFILES   8
#define XPADNEO_QUIRK_LINUX_BUTTONS     16
#define XPADNEO_QUIRK_NINTENDO          32
#define XPADNEO_QUIRK_SHARE_BUTTON      64
#define XPADNEO_QUIRK_REVERSE_MASK      128
#define XPADNEO_QUIRK_SWAPPED_MASK      256
#define XPADNEO_QUIRK_NO_HEURISTICS	512

#define XPADNEO_QUIRK_NO_HAPTICS        (XPADNEO_QUIRK_NO_PULSE|XPADNEO_QUIRK_NO_MOTOR_MASK)
#define XPADNEO_QUIRK_SIMPLE_CLONE      (XPADNEO_QUIRK_NO_HAPTICS|XPADNEO_QUIRK_NO_TRIGGER_RUMBLE)

/* MAC OUI masks */
#define XPADNEO_OUI_MASK(oui,mask)    (((oui)&(mask))==(mask))
#define XPADNEO_OUI_MASK_GAMESIR_NOVA 0x28

/* timing of rumble commands to work around firmware crashes */
#define XPADNEO_RUMBLE_THROTTLE_DELAY   msecs_to_jiffies(50)
#define XPADNEO_RUMBLE_THROTTLE_JIFFIES (jiffies + XPADNEO_RUMBLE_THROTTLE_DELAY)

/* helpers */
#define SWAP_BITS(v,b1,b2) \
	(((v)>>(b1)&1)==((v)>>(b2)&1)?(v):(v^(1ULL<<(b1))^(1ULL<<(b2))))

/* rumble motors enable bits */
enum xpadneo_rumble_motors {
	XBOX_RUMBLE_NONE = 0x00,
	XBOX_RUMBLE_WEAK = 0x01,
	XBOX_RUMBLE_STRONG = 0x02,
	XBOX_RUMBLE_MAIN = XBOX_RUMBLE_WEAK | XBOX_RUMBLE_STRONG,
	XBOX_RUMBLE_RIGHT = 0x04,
	XBOX_RUMBLE_LEFT = 0x08,
	XBOX_RUMBLE_TRIGGERS = XBOX_RUMBLE_LEFT | XBOX_RUMBLE_RIGHT,
	XBOX_RUMBLE_ALL = 0x0F
} __packed;

/* rumble packet structure */
struct rumble_data {
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
static_assert(sizeof(struct rumble_data) == 8);
#endif

/* report number for rumble commands */
#define XPADNEO_XBOX_RUMBLE_REPORT 0x03

/* maximum length of report 0x01 for duplicate packet filtering */
#define XPADNEO_REPORT_0x01_LENGTH (55+1)

/* HID packet for rumble commands */
struct rumble_report {
	u8 report_id;
	struct rumble_data data;
} __packed;
#ifdef static_assert
static_assert(sizeof(struct rumble_report) == 9);
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
	struct input_dev *consumer, *gamepad, *keyboard, *mouse;
	bool consumer_sync, gamepad_sync, keyboard_sync, mouse_sync;
	short int missing_reported;

	/* revert fixups on removal */
	u16 original_product;
	u32 original_version;

	/* quirk flags */
	unsigned int original_rsize;
	u32 quirks;

	/* profile switching */
	bool xbox_button_down, profile_switched;
	u8 last_profile, profile;

	/* mouse mode */
	bool mouse_mode;
	struct timer_list mouse_timer;
	struct {
		s32 rel_x, rel_y, wheel_x, wheel_y;
		s32 rel_x_err, rel_y_err, wheel_x_err, wheel_y_err;
		struct {
			bool left, right;
		} analog_button;
	} mouse_state;

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

	/* buffer for rumble_worker */
	struct {
		spinlock_t lock;
		struct delayed_work worker;
		unsigned long throttle_until;
		bool scheduled;
		struct rumble_data data;
		struct rumble_data shadow;
		void *output_report_dmabuf;
	} rumble;
};

/* xpadneo helpers for synthetic drivers */
extern int xpadneo_synthetic_init(struct xpadneo_devdata *, const char *, struct input_dev **);

/* xpadneo core device functions */
extern void xpadneo_device_report(struct hid_device *, struct hid_report *);
extern void xpadneo_device_missing(struct xpadneo_devdata *, u32);
extern int xpadneo_device_output_report(struct hid_device *, __u8 *, size_t);
#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
extern u8 *xpadneo_device_report_fixup(struct hid_device *hdev, u8 *rdesc, unsigned int *rsize);
#else
extern const __u8 *xpadneo_device_report_fixup(struct hid_device *hdev, __u8 *rdesc,
					       unsigned int *rsize);
#endif

/* xpadneo consumer driver */
extern int xpadneo_consumer_init(struct xpadneo_devdata *);

/* xpadneo keyboard driver */
extern int xpadneo_keyboard_init(struct xpadneo_devdata *);

/* xpadneo rumble driver */
extern int xpadneo_rumble_init(struct hid_device *);
extern int xpadneo_rumble_init_workqueue(void);
extern void xpadneo_rumble_destroy_workqueue(void);
extern void xpadneo_rumble_remove(struct xpadneo_devdata *);

/* xpadneo mouse driver */
extern int xpadneo_mouse_init(struct xpadneo_devdata *);
extern void xpadneo_mouse_init_timer(struct xpadneo_devdata *);
extern void xpadneo_mouse_report(struct timer_list *);
extern void xpadneo_mouse_toggle(struct xpadneo_devdata *);
extern int xpadneo_mouse_event(struct xpadneo_devdata *, struct hid_usage *, __s32);
extern int xpadneo_mouse_raw_event(struct xpadneo_devdata *, struct hid_report *, u8 *, int);
extern void xpadneo_mouse_remove_timer(struct xpadneo_devdata *);

/* battery and power functions */
extern int xpadneo_power_init(struct xpadneo_devdata *);
extern void xpadneo_power_update(struct xpadneo_devdata *, u8);
extern void xpadneo_power_remove(struct xpadneo_devdata *xdata);

/* driver quirks handling */
extern int xpadneo_quirks_init(struct xpadneo_devdata *);

/* driver usage mappings */
extern int xpadneo_mapping_input(struct hid_device *, struct hid_input *,
				 struct hid_field *,
				 struct hid_usage *, unsigned long **, int *);

/* driver events and profiles handling */
extern int xpadneo_events_raw_event(struct hid_device *, struct hid_report *, u8 *, int);
extern int xpadneo_events_event(struct hid_device *, struct hid_field *, struct hid_usage *, __s32);
extern int xpadneo_events_input_configured(struct hid_device *, struct hid_input *);

#endif
