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

/**
 * @ingroup adf_os_public
 * @file adf_os_types.h
 * This file defines types used in the OS abstraction API.
 */

#ifndef _ADF_OS_TYPES_H
#define _ADF_OS_TYPES_H

#include "osdep.h"
#include "adf_os_stdtypes.h"


#ifdef __i386__
#define __ahdecl   __attribute__((regparm(0))) 
#else
#define __ahdecl
#endif

#define DMA_TO_DEVICE   0
#define DMA_FROM_DEVICE 0
#include <asm/byteorder.h>

#if defined(__LITTLE_ENDIAN_BITFIELD)
#define ADF_LITTLE_ENDIAN_MACHINE
#elif defined (__BIG_ENDIAN_BITFIELD)
#define ADF_BIG_ENDIAN_MACHINE
#else
#error
#endif

/**
 * @brief handles opaque to each other
 */
typedef void*      adf_net_handle_t;
typedef void*      adf_drv_handle_t;
typedef osdev_t    adf_os_handle_t;
typedef osdev_t    adf_os_device_t;

typedef size_t            adf_os_size_t;
typedef size_t            adf_os_dma_size_t;
typedef dma_addr_t        adf_os_dma_addr_t;

#define ADF_OS_MAX_SCATTER        1
#define ADF_OS_MAX_SGLIST         4

typedef struct adf_os_segment {
    dma_addr_t  daddr;
    uint32_t    len; 
} adf_os_segment_t;

struct adf_os_dma_map {
    uint32_t                mapped;
    uint32_t                nsegs;
    uint32_t                coherent;
    adf_os_segment_t        seg[ADF_OS_MAX_SCATTER];
};

typedef struct adf_os_dma_map* adf_os_dma_map_t;

typedef struct adf_os_dmamap_info {
    a_uint32_t                  nsegs;   /**< total number mapped segments*/
    struct __dma_segs {
        adf_os_dma_addr_t       paddr;   /**< phy(dma'able) address */
        adf_os_dma_size_t       len;     /**< length of the segment*/
    } dma_segs[ADF_OS_MAX_SCATTER]; 
} adf_os_dmamap_info_t;

/**
 * @brief Representation of a scatter-gather list.
 */ 
typedef struct adf_os_sglist {
    a_uint32_t                  nsegs;      /**< total number of segments*/
    struct __sg_segs{
        a_uint8_t              *vaddr;      /**< Virt address of the segment*/
        a_uint32_t              len;        /**< Length of the segment*/
    } sg_segs[ADF_OS_MAX_SGLIST];

} adf_os_sglist_t;

#define __ADF_OS_DMA_TO_DEVICE      DMA_TO_DEVICE
#define __ADF_OS_DMA_FROM_DEVICE    DMA_FROM_DEVICE

/**
 * @brief Types of buses.
 */ 
typedef enum {
    ADF_OS_BUS_TYPE_PCI = 1,
    ADF_OS_BUS_TYPE_GENERIC,
}adf_os_bus_type_t;

/**
 * @brief IRQ handler response codes.
 */ 
typedef enum {
    ADF_OS_IRQ_NONE,
    ADF_OS_IRQ_HANDLED,
}adf_os_irq_resp_t;

/**
 * @brief DMA mask types.
 */ 
typedef enum {
    ADF_OS_DMA_MASK_32BIT,
    ADF_OS_DMA_MASK_64BIT,
}adf_os_dma_mask_t;


/**
 * @brief DMA directions
 *        ADF_OS_DMA_TO_DEVICE (data going from device to memory)
 *        ADF_OS_DMA_FROM_DEVICE (data going from memory to device)
 */
typedef enum {
    ADF_OS_DMA_TO_DEVICE = __ADF_OS_DMA_TO_DEVICE, 
    ADF_OS_DMA_FROM_DEVICE = __ADF_OS_DMA_FROM_DEVICE, 
} adf_os_dma_dir_t;

typedef enum adf_os_cache_sync {
    ADF_SYNC_PREREAD,
    ADF_SYNC_PREWRITE,
    ADF_SYNC_POSTREAD,
    ADF_SYNC_POSTWRITE
} adf_os_cache_sync_t;

/**
 * @brief Generic status to be used by adf_drv.
 */
typedef enum {
    A_STATUS_OK,
    A_STATUS_FAILED,
    A_STATUS_ENOENT,
    A_STATUS_ENOMEM,
    A_STATUS_EINVAL,
    A_STATUS_EINPROGRESS,
    A_STATUS_ENOTSUPP,
    A_STATUS_EBUSY,
    A_STATUS_E2BIG,
    A_STATUS_EADDRNOTAVAIL,
    A_STATUS_ENXIO,
    A_STATUS_EFAULT,
    A_STATUS_EIO,
} a_status_t;

typedef int (*adf_os_intr)(void *);

/**
 * @brief Prototype of IRQ function.
 */ 
typedef adf_os_irq_resp_t (*adf_os_drv_intr)(adf_drv_handle_t hdl);

/**
 * @brief work queue(kernel thread)/DPC function callback
 */
typedef void (*adf_os_defer_fn_t)(void *);

/**
 * @brief Prototype of the critical region function that is to be
 * executed with spinlock held and interrupt disalbed
 */
typedef a_bool_t (*adf_os_irqlocked_func_t)(void *);

#define adf_os_print(...)                         \
    do {                                          \
        printk(KERN_DEBUG __VA_ARGS__);           \
    } while (0)

#define adf_os_vprint              vprintk

#endif
