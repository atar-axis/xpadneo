/* SPDX-License-Identifier: GPL-3.0-only */

/*
 * xpadneo kernel compatibility helpers
 *
 * Copyright (c) 2020 Kai Krakow <kai@kaishome.de>
 */

#ifndef COMPAT_H
#define COMPAT_H

#include <linux/version.h>

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
