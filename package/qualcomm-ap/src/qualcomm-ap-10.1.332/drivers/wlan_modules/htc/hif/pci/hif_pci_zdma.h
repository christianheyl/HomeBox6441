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

#ifndef __HIF_PCI_ZDMA_H
#define __HIF_PCI_ZDMA_H

#include <adf_os_types.h>
//#include <adf_net.h>

#define PCI_NBUF_ALIGNMENT              4
/**
 * Change this for the mapping 0 is streaming & 1 is coherent
 */
#define PCI_DMA_MAPPING                 1

#define ADF_BIG_ENDIAN_MACHINE          1

struct zsDmaDesc{
#if defined (ADF_BIG_ENDIAN_MACHINE)
    a_uint16_t      ctrl;       // Descriptor control
    a_uint16_t      status;     // Descriptor status
    a_uint16_t      totalLen;   // Total length
    a_uint16_t      dataSize;   // Data size
#elif defined (ADF_LITTLE_ENDIAN_MACHINE)
    a_uint16_t      status;     // Descriptor status
    a_uint16_t      ctrl;       // Descriptor control
    a_uint16_t      dataSize;   // Data size
    a_uint16_t      totalLen;   // Total length
#else
#error "Endianess unknown, Fix me"
#endif
    struct zsDmaDesc*        lastAddr;   // Last address of this chain
    a_uint32_t               dataAddr;   // Data buffer address
    struct zsDmaDesc*        nextAddr;   // Next TD address
    a_uint8_t                pad[12]; /* Pad for 32 byte Cache Alignment*/
};


typedef struct zdma_swdesc {
    a_uint8_t               *buf_addr;
    a_uint32_t               buf_size;
    adf_nbuf_t               nbuf;
    adf_os_dma_map_t         nbuf_map;
    adf_os_dma_addr_t        hwaddr;
    struct zsDmaDesc        *descp; 
}zdma_swdesc_t;


typedef struct pci_dma_softc{
    adf_os_dma_map_t     dmap; 
    zdma_swdesc_t       *sw_ring;
    struct zsDmaDesc    *hw_ring;
    /**
     * For ring mgmt
     */
    a_uint32_t           tail;/* dequeue*/
    a_uint32_t           head;/* enqueue*/
    a_uint32_t           num_desc;
}pci_dma_softc_t;

/* Status bits definitions */
/* Own bits definitions */
#define ZM_OWN_BITS_MASK        0x3
#define ZM_OWN_BITS_SW          0x0
#define ZM_OWN_BITS_HW          0x1
#define ZM_OWN_BITS_SE          0x2
/* Control bits definitions */
/* First segament bit */
#define ZM_LS_BIT               0x100
/* Last segament bit */
#define ZM_FS_BIT               0x200


#define ring_incr(_val, _lim)       ((_val) + 1)&((_lim) - 1)
#define ring_tx_incr(_val)          ring_incr((_val), HIF_PCI_MAX_TX_DESC)
#define ring_rx_incr(_val)          ring_incr((_val), HIF_PCI_MAX_RX_DESC)

#define RING_MAX                    HIF_PCI_MAX_TX_DESC

#define ring_full(head, tail)       (((head + 1) % RING_MAX) == tail ) 

#define ring_empty(head, tail)      (head == tail)

#define ring_free(_h, _t, _num)        \
    ((_h >= _t) ? (_num - _h + _t) : (_t - _h))

#define ring_tx_free(_h, _t)        ring_free(_h, _t, HIF_PCI_MAX_TX_DESC)

#define hw_desc_own(hwdesc_p)       ((hwdesc_p)->status == ZM_OWN_BITS_HW) 
#define hw_desc_len(hwdesc_p)       (hwdesc_p)->totalLen

/**
 * 
 * @param dma_q
 * 
 * @return a_uint32_t
 */
static inline a_uint32_t
pci_dma_tail_addr(pci_dma_softc_t  *dma_q)
{
    a_uint32_t tail = dma_q->tail;
    zdma_swdesc_t  *swdesc = &dma_q->sw_ring[tail];

    return (swdesc->hwaddr);
}

/**
 * 
 * @param dma_q
 * 
 * @return a_uint32_t
 */
static inline zdma_swdesc_t *
pci_dma_tail_vaddr(pci_dma_softc_t  *dma_q)
{
    a_uint32_t  tail = dma_q->tail;
    zdma_swdesc_t  *swdesc = &dma_q->sw_ring[tail];

    return (swdesc);
}

/**
 * 
 * @param dma_q
 * 
 * @return a_uint32_t
 */
static inline zdma_swdesc_t *
pci_dma_head_vaddr(pci_dma_softc_t  *dma_q)
{
    a_uint32_t head = dma_q->head;
    zdma_swdesc_t  *swdesc = &dma_q->sw_ring[head];

    return (swdesc);
}

/**
 * @brief Mark the H/W descriptor ready
 * 
 * @param swdesc
 * @param ctrl
 */
static inline void
pci_zdma_mark_rdy(zdma_swdesc_t  *swdesc, a_uint16_t ctrl)
{
    struct zsDmaDesc  *hwdesc = swdesc->descp;

    hwdesc->dataAddr = (a_uint32_t)swdesc->buf_addr;
    hwdesc->dataSize = swdesc->buf_size;
    hwdesc->lastAddr = (struct zsDmaDesc  *)swdesc->hwaddr;
    hwdesc->status   = ZM_OWN_BITS_HW;
    hwdesc->ctrl     = ctrl;
}

/**
 * @brief Add SKB into the S/W descriptor
 * 
 * @param osdev
 * @param swdesc
 * @param buf
 */
static inline void
pci_dma_link_buf(adf_os_device_t osdev, zdma_swdesc_t *swdesc, 
                 adf_nbuf_t  buf)
{
    adf_os_dmamap_info_t  sg = {0};

    adf_nbuf_map(osdev, swdesc->nbuf_map, buf, ADF_OS_DMA_TO_DEVICE);

    /**
     * XXX: for TX gather we need to use multiple swdesc
     */
    adf_nbuf_dmamap_info(swdesc->nbuf_map, &sg);
    adf_os_assert(sg.nsegs == 1);

    swdesc->nbuf       = buf;
    swdesc->buf_addr   = (a_uint8_t *)sg.dma_segs[0].paddr;
    swdesc->buf_size   = sg.dma_segs[0].len;
}

/**
 * @brief remove the SKB references in S/W descriptor
 * 
 * @param osdev
 * @param swdesc
 * 
 * @return adf_nbuf_t
 */
static inline adf_nbuf_t
pci_dma_unlink_buf(adf_os_device_t osdev, zdma_swdesc_t *swdesc)
{
    adf_nbuf_t  buf = swdesc->nbuf;

    adf_nbuf_unmap(osdev, swdesc->nbuf_map, ADF_OS_DMA_TO_DEVICE);

    swdesc->buf_addr = NULL;
    swdesc->buf_size = 0;
    swdesc->nbuf     = ADF_NBUF_NULL;

    return buf;
}

void            pci_prn_uncached(zdma_swdesc_t  *swdesc);

void            pci_dma_init_rx(adf_os_device_t osdev, pci_dma_softc_t *dma_q,
                                a_uint32_t num_desc, adf_os_size_t buf_size);

void            pci_dma_deinit_rx(adf_os_device_t , pci_dma_softc_t *);
void            pci_dma_init_tx(adf_os_device_t , pci_dma_softc_t *,
                                a_uint32_t num_desc);
void            pci_dma_deinit_tx(adf_os_device_t , pci_dma_softc_t *);

a_status_t      pci_dma_recv_refill(adf_os_device_t , zdma_swdesc_t *,
                                    a_uint32_t );

#endif
