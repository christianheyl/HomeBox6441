/*
 * Copyright (c) 2010, Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _ADF_OS_TIME_PVT_H
#define _ADF_OS_TIME_PVT_H

#include <linux/jiffies.h>

static inline unsigned long
adf_os_ticks(void)
{
    return (jiffies);
}
static inline uint32_t
adf_os_ticks_to_msecs(unsigned long ticks)
{
    return (jiffies_to_msecs(ticks));
}
static inline unsigned long
adf_os_msecs_to_ticks(a_uint32_t msecs)
{
    return (msecs_to_jiffies(msecs));
}
static inline unsigned long
adf_os_getuptime(void)
{
    return jiffies;
}

static inline void
adf_os_udelay(int usecs)
{
    udelay(usecs);
}

static inline void
adf_os_mdelay(int msecs)
{
    mdelay(msecs);
}

#define adf_os_time_after(a,b) ((long)(b) - (long)(a) < 0)
#define adf_os_time_before(a,b) __adf_os_time_after(b,a)
#define adf_os_time_after_eq(a,b) ((long)(a) - (long)(b) >= 0)

#endif
