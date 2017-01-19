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

#ifndef _ADF_OS_UTIL_PVT_H
#define _ADF_OS_UTIL_PVT_H

#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/types.h>

#include <linux/random.h>

#include <asm/system.h>
#include <adf_os_types.h>
/*
 * Generic compiler-dependent macros if defined by the OS
 */
#define adf_os_unlikely(_expr)   unlikely(_expr)
#define adf_os_likely(_expr)     likely(_expr)

/**
 * @brief memory barriers. 
 */
#define adf_os_wmb()                wmb()
#define adf_os_rmb()                rmb()
#define adf_os_mb()                 mb()

#define adf_os_min(_a, _b)         min(_a, _b)
#define adf_os_max(_a, _b)         max(_a, _b)

#define adf_os_packed          __attribute__ ((packed))

#define adf_os_assert(expr)  do {\
    if(unlikely(!(expr))) {                                   \
        printk(KERN_ERR "Assertion failed! %s:%s %s:%d\n", \
              #expr, __FUNCTION__, __FILE__, __LINE__);          \
        panic("Take care of the assert first\n");           \
    }                                                           \
}while(0);


static inline void 
adf_os_get_rand(adf_os_handle_t  hdl, uint8_t *ptr, uint32_t  len)
{
    get_random_bytes(ptr, len);
}
#endif /*_ADF_OS_UTIL_PVT_H*/
