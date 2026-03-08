/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * xpadneo kernel compatibility helpers
 *
 * Copyright (c) 2020 Kai Krakow <kai@kaishome.de>
 */

#ifndef COMPAT_H
#define COMPAT_H

#include <linux/version.h>

/* v4.19: ida_simple_{get,remove}() have been replaced */
#if KERNEL_VERSION(4, 19, 0) >= LINUX_VERSION_CODE
#define ida_alloc(a, f) ida_simple_get((a), 0, 0, (f))
#define ida_free(a, f) ida_simple_remove((a), (f))
#endif

/* v6.15: del_timer_sync() renamed to timer_delete_sync() */
#if KERNEL_VERSION(6, 15, 0) > LINUX_VERSION_CODE
#define timer_delete_sync(t) del_timer_sync(t)
#endif

/* v6.16: from_timer() renamed to timer_container_of() */
#ifndef timer_container_of
#define timer_container_of(v, c, t) from_timer(v, c, t)
#endif

/* Profile usage code for kernel < 6.0-rc1 */
#ifndef ABS_PROFILE
#define ABS_PROFILE 0x21
#endif

/* Paddle usage codes for kernel < 6.17 */
#ifndef BTN_GRIPL
#define BTN_GRIPL  0x224
#define BTN_GRIPR  0x225
#define BTN_GRIPL2 0x226
#define BTN_GRIPR2 0x227
#endif

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
static inline u8 *xpadneo_device_report_fixup_compat(struct hid_device *hdev, u8 *rdesc,
						     unsigned int *rsize)
{
	return (u8 *)xpadneo_device_report_fixup(hdev, rdesc, rsize);
}
#else
#define xpadneo_device_report_fixup_compat xpadneo_device_report_fixup
#endif

#endif
