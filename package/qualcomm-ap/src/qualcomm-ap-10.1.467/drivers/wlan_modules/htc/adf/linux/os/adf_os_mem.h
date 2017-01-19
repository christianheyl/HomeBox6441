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

#ifndef ADF_OS_MEM_PVT_H
#define ADF_OS_MEM_PVT_H

#include <linux/slab.h>
#include <linux/hardirq.h>

#include <adf_os_types.h>
#include <adf_os_util.h>

extern unsigned int g_mem; 

static inline void*
adf_os_mem_alloc(adf_os_handle_t handle, size_t size)
{
    int flags = GFP_KERNEL;

    if(in_interrupt())
        flags = GFP_ATOMIC;

    return kzalloc(size, flags);
}

static inline void*
adf_os_mem_alloc_outline(void* osdev, size_t size)
{
    int flags = GFP_KERNEL;

    if(in_interrupt() || irqs_disabled())
        flags = GFP_ATOMIC;

    return kzalloc(size, flags);
}

static inline void
adf_os_mem_free(void *buf)
{
    kfree(buf);
}

static inline void
adf_os_mem_free_outline(void *buf)
{
    kfree(buf);
}

/* move a memory buffer */
static inline __ahdecl void
adf_os_mem_copy(void *dst, const void *src, size_t size)
{
    memcpy(dst, src, size);
}

/* set a memory buffer */
static inline void
adf_os_mem_set(void *buf, uint8_t b, size_t size)
{
    memset(buf, b, size);
}

/* zero a memory buffer */
static inline __ahdecl void
adf_os_mem_zero(void *buf, size_t size)
{
    memset(buf, 0, size);
}

/* compare two memory buffers */
static inline int
adf_os_mem_cmp(void *buf1, void *buf2, size_t size)
{
    return (memcmp(buf1, buf2, size) == 0) ? 0 : 1;
}

/**
 * @brief  Unlike memcpy(), memmove() copes with overlapping
 *         areas.
 * @param src
 * @param dst
 * @param size
 */
static inline void
adf_os_mem_move(void *dst, void *src, size_t size)
{
    memmove(dst, src, size);
}


#endif /*ADF_OS_MEM_PVT_H*/
