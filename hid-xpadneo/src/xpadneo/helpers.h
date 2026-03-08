/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * xpadneo helper functions and macros
 *
 * Copyright (c) 2026 Kai Krakow <kai@kaishome.de>
 */

#ifndef HELPERS_H
#define HELPERS_H

/* helper for printing a notice only once */
#ifndef hid_notice_once
#define hid_notice_once(hid, fmt, ...)			\
do {							\
	static bool __print_once __read_mostly;		\
	if (!__print_once) {				\
		__print_once = true;			\
		hid_notice(hid, fmt, ##__VA_ARGS__);	\
	}						\
} while (0)
#endif

/* benchmark helper */
#define xpadneo_benchmark(name, ...)					\
do {									\
	unsigned long __##name##_jiffies = jiffies;			\
	pr_info("xpadneo " #name " start\n");				\
	name(__VA_ARGS__);						\
	pr_info("xpadneo " #name " took %ums\n",			\
		jiffies_to_msecs(jiffies - __##name##_jiffies));	\
} while (0)

/* generic helpers */
#define SWAP_BITS(v, b1, b2) \
	(((v)>>(b1)&1) == ((v)>>(b2)&1)?(v):(v^(1ULL<<(b1))^(1ULL<<(b2))))

#endif
