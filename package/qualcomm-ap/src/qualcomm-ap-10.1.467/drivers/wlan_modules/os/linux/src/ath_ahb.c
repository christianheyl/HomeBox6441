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

#include <osdep.h>

/* set bus cachesize in 4B word units */
void ahb_read_cachesize(osdev_t osdev, int *csz)
{
    /* XXX: get the appropriate value! PCI case reads from config space,
     *   and I think this is the data cache line-size. 
     */
    *csz = L1_CACHE_BYTES / sizeof(u_int32_t);
}

/* NOTE: returns uncached (kseg1) address. */
void *ahb_alloc_consistent(void *hwdev, size_t size,
                            dma_addr_t * dma_handle)
{
     void *ret;
     int gfp = GFP_ATOMIC;

     ret = (void *) __get_free_pages(gfp, get_order(size));

     if (ret != NULL) {
         memset(ret, 0, size);
         *dma_handle = __pa(ret);
         dma_cache_wback_inv((unsigned long) ret, size);
         ret = UNCAC_ADDR(ret);
     }

     return ret;
}

void ahb_free_consistent(void *hwdev, size_t size,
                         void *vaddr, dma_addr_t dma_handle)
{
    unsigned long addr = (unsigned long) vaddr;

    addr = CAC_ADDR(addr);
    free_pages(addr, get_order(size));
}
