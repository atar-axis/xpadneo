/* SPDX-License-Identifier: GPL-2.0-only */

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
#include <linux/workqueue.h>

/* detect locally administered unicast MAC addresses */
#define XPADNEO_OUI_IS_MULTICAST       (1 << 0)
#define XPADNEO_OUI_IS_LAA             (1 << 1)

/* button aliases */
#define BTN_SHARE KEY_F12
#define BTN_XBOX  BTN_MODE

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
#define XPADNEO_QUIRK_NO_HEURISTICS     512

/* common quirk combinations */
#define XPADNEO_QUIRK_NO_HAPTICS        (XPADNEO_QUIRK_NO_PULSE|XPADNEO_QUIRK_NO_MOTOR_MASK)
#define XPADNEO_QUIRK_SIMPLE_CLONE      (XPADNEO_QUIRK_NO_HAPTICS|XPADNEO_QUIRK_NO_TRIGGER_RUMBLE)

/* report number for rumble commands */
#define XPADNEO_XBOX_RUMBLE_REPORT 0x03

/* maximum length of report 0x01 for duplicate packet filtering */
#define XPADNEO_REPORT_0x01_LENGTH (55+1)

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
struct xpadneo_rumble_data {
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
static_assert(sizeof(struct xpadneo_rumble_data) == 8);
#endif

/* HID packet for rumble commands */
struct xpadneo_rumble_report {
	u8 report_id;
	struct xpadneo_rumble_data data;
} __packed;
#ifdef static_assert
static_assert(sizeof(struct xpadneo_rumble_report) == 9);
#endif

/* sub devices */
struct xpadneo_subdevice {
	struct input_dev *idev;
	bool sync;
	bool is_synthetic;
};

/* private driver instance data */
struct xpadneo_devdata {
	/* unique physical device id (randomly assigned) */
	int id;

	/* logical device interfaces */
	struct hid_device *hdev;

	/* sub devices */
	struct xpadneo_subdevice consumer;
	struct xpadneo_subdevice gamepad;
	struct xpadneo_subdevice keyboard;
	struct xpadneo_subdevice mouse;

	short int missing_reported;

	/* revert fixups on removal */
	u16 original_product;
	u32 original_version;

	/* quirk flags */
	unsigned int original_rsize;
	u32 quirks;

	/* profile switching */
	bool shift_mode, profile_switched;
	u8 last_profile, profile;

	/* HOGP protocol */
	bool uses_hogp;

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
	s32 last_abs_z;
	s32 last_abs_rz;

	/* buffer for rumble_worker */
	struct {
		spinlock_t lock;
		struct work_struct worker;
		struct work_struct init_worker;
		bool enabled, pending;
		struct xpadneo_rumble_data data;
		struct xpadneo_rumble_data shadow;
		void *output_report_dmabuf;
	} rumble;
};

/* xpadneo helpers for synthetic drivers */
extern int xpadneo_synthetic_init(struct xpadneo_devdata *, const char *,
				  struct xpadneo_subdevice *);
extern int xpadneo_synthetic_register(struct xpadneo_devdata *, const char *,
				      struct xpadneo_subdevice *);
extern void xpadneo_synthetic_remove(struct xpadneo_devdata *, const char *,
				     struct xpadneo_subdevice *);

/* xpadneo descriptor debug helpers */
extern void xpadneo_debug_hid_report(const struct hid_device *, const void *, const size_t);
extern void xpadneo_debug_descriptor(const struct hid_device *, const __u8 *, unsigned int);

/* xpadneo core device functions */
extern void xpadneo_device_report(struct hid_device *, struct hid_report *);
extern void xpadneo_device_missing(struct xpadneo_devdata *, u32);
extern int xpadneo_device_output_report(struct hid_device *, __u8 *, size_t, bool);
extern const __u8 *xpadneo_device_report_fixup(struct hid_device *hdev, __u8 *rdesc,
					       unsigned int *rsize);

/* xpadneo consumer driver */
extern int xpadneo_consumer_init(struct xpadneo_devdata *);
extern void xpadneo_consumer_remove(struct xpadneo_devdata *);

/* xpadneo keyboard driver */
extern int xpadneo_keyboard_init(struct xpadneo_devdata *);
extern void xpadneo_keyboard_remove(struct xpadneo_devdata *);

/* xpadneo rumble driver */
extern int xpadneo_rumble_init(struct hid_device *);
extern int xpadneo_rumble_init_workqueue(void);
extern void xpadneo_rumble_destroy_workqueue(void);
extern inline void xpadneo_rumble_streaming_set(struct xpadneo_devdata *, const bool);
extern inline bool xpadneo_rumble_streaming_get(const struct xpadneo_devdata *);
extern void xpadneo_rumble_remove(struct xpadneo_devdata *);

/* xpadneo mouse driver */
extern int xpadneo_mouse_init(struct xpadneo_devdata *);
extern void xpadneo_mouse_init_timer(struct xpadneo_devdata *);
extern void xpadneo_mouse_report(struct timer_list *);
extern bool xpadneo_mouse_toggle(struct xpadneo_devdata *);
extern int xpadneo_mouse_event(struct xpadneo_devdata *, struct hid_usage *, __s32);
extern int xpadneo_mouse_raw_event(struct xpadneo_devdata *, struct hid_report *, u8 *, int);
extern void xpadneo_mouse_remove_timer(struct xpadneo_devdata *);
extern void xpadneo_mouse_remove(struct xpadneo_devdata *);

/* battery and power functions */
extern int xpadneo_power_init(struct xpadneo_devdata *);
extern void xpadneo_power_update(struct xpadneo_devdata *, u8);
extern void xpadneo_power_remove(struct xpadneo_devdata *);

/* driver quirks handling */
extern int xpadneo_quirks_init(struct xpadneo_devdata *);
extern void xpadneo_quirks_remove(struct xpadneo_devdata *);

/* driver usage mappings */
extern int xpadneo_mappings_input(struct hid_device *, struct hid_input *,
				  struct hid_field *, struct hid_usage *, unsigned long **, int *);

/* driver events and profiles handling */
extern int xpadneo_events_raw_event(struct hid_device *, struct hid_report *, u8 *, int);
extern int xpadneo_events_event(struct hid_device *, struct hid_field *, struct hid_usage *, __s32);
extern int xpadneo_events_input_configured(struct hid_device *, struct hid_input *);

#endif
