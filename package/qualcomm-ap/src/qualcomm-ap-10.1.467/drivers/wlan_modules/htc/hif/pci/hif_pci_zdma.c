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

#include <adf_nbuf.h>
#include <adf_os_mem.h>
#include <adf_os_dma.h>
#include "hif_pci_zdma.h"

/**************************** Globals ****************************/


a_status_t
pci_dma_recv_refill(adf_os_device_t osdev, zdma_swdesc_t *swdesc,
                    a_uint32_t size)
{
    adf_nbuf_t          buf;

    buf = adf_nbuf_alloc(osdev, size, 0, PCI_NBUF_ALIGNMENT);
    if (!buf)
        adf_os_assert(0);

    pci_dma_link_buf(osdev, swdesc, buf);
    
    pci_zdma_mark_rdy(swdesc, (ZM_FS_BIT | ZM_LS_BIT));
    
    return A_STATUS_OK;
}

a_status_t
pci_dma_recv_init(adf_os_device_t osdev, pci_dma_softc_t *dma_q,
                  adf_nbuf_t  buf, a_uint32_t  pos)
{
    zdma_swdesc_t  *swdesc;

    swdesc = &dma_q->sw_ring[pos];
    
    pci_dma_link_buf(osdev, swdesc, buf);

    pci_zdma_mark_rdy(swdesc, (ZM_FS_BIT | ZM_LS_BIT));

    return A_STATUS_OK;
}

/**
 * @brief Initialize the H/W ring
 * Note: This joins all the H/W descriptor together
 * 
 * @param swdesc
 * @param num_desc
 */
static void
pci_dma_init_ring(zdma_swdesc_t  *swdesc , a_uint32_t  num_desc)
{
    a_uint32_t  i = 0, j = 0;

    for (i = 0; i < num_desc; i++) {
        j = (i + 1) % num_desc;
        swdesc[i].descp->nextAddr = (struct zsDmaDesc *)swdesc[j].hwaddr;
    }
}


/**
 * @brief Allocate & initialize the S/W descriptor & H/W
 *        Descriptor
 * 
 * @param osdev
 * @param sc
 * @param num_desc
 */
static void
pci_dma_alloc_swdesc(adf_os_device_t osdev, pci_dma_softc_t *sc,
                     a_uint32_t  num_desc)
{
    a_uint32_t  size_sw, size_hw, i = 0;
    adf_os_dma_addr_t  paddr;
    zdma_swdesc_t  *swdesc;
    struct zsDmaDesc  *hwdesc;


    size_sw = sizeof(struct zdma_swdesc) * num_desc;
    size_hw = sizeof(struct zsDmaDesc) * num_desc;

    sc->sw_ring = adf_os_mem_alloc(osdev, size_sw);
    adf_os_assert(sc->sw_ring);

    sc->hw_ring = adf_os_dmamem_alloc(osdev, size_hw, PCI_DMA_MAPPING, 
                                      &sc->dmap);
    adf_os_assert(sc->hw_ring);

    swdesc = sc->sw_ring;
    hwdesc = sc->hw_ring;
    paddr  = adf_os_dmamem_map2addr(sc->dmap);
    
    for (i = 0; i < num_desc; i++) {
        swdesc[i].descp = &hwdesc[i];
        swdesc[i].hwaddr = paddr;
        paddr = (adf_os_dma_addr_t)((struct zsDmaDesc *)paddr + 1);
    }
    sc->num_desc = num_desc;

    pci_dma_init_ring(swdesc, num_desc);

}

/**
 * @brief Free the S/W & H/W descriptor ring
 * 
 * @param osdev
 * @param dma_q
 * @param num_desc
 */
static void
pci_dma_free_swdesc(adf_os_device_t  osdev, pci_dma_softc_t  *dma_q,
                    a_uint32_t num_desc)
{
    a_uint32_t  size_hw;

    size_hw = sizeof(struct zsDmaDesc) * num_desc;
    adf_os_dmamem_free(osdev, size_hw, PCI_DMA_MAPPING, dma_q->hw_ring, 
                       dma_q->dmap);

    adf_os_mem_free(dma_q->sw_ring);
}

/**
 * @brief Mark the H/W descriptor not ready
 * 
 * @param swdesc
 * @param ctrl
 */
static inline void
pci_zdma_mark_notrdy(zdma_swdesc_t  *swdesc)
{
    struct zsDmaDesc  *hwdesc = swdesc->descp;

    hwdesc->status   = ZM_OWN_BITS_SW;
}


/**
 * @brief Initialize the RX ring
 * 
 * @param osdev
 * @param dma_q
 * @param num_desc
 * @param buf_size
 */
void    
pci_dma_init_rx(adf_os_device_t osdev, pci_dma_softc_t *dma_q,
                a_uint32_t num_desc, adf_os_size_t buf_size)
{
    a_uint32_t  i;
    zdma_swdesc_t  *swdesc;
    adf_nbuf_t  buf;

    pci_dma_alloc_swdesc(osdev, dma_q, num_desc);

    swdesc = dma_q->sw_ring;

    for (i = 0; i < num_desc ; i++) {
        adf_nbuf_dmamap_create(osdev, &swdesc[i].nbuf_map);

        buf = adf_nbuf_alloc(osdev, buf_size, 0, PCI_NBUF_ALIGNMENT);
        adf_os_assert(buf);
        
        pci_dma_recv_init(osdev, dma_q, buf, i);
        
    }
}   

/**
 * @brief Free the RX Ring ( S/W & H/W), dequeue all the SKB's
 *        and free them starting from the head
 * NOTE: The NULL terminator doesn't have a SKB
 * 
 * @param osdev
 * @param dma_q
 */
void    
pci_dma_deinit_rx(adf_os_device_t osdev, pci_dma_softc_t *dma_q) 
{
    a_uint32_t      i, num_desc;
    zdma_swdesc_t  *swdesc;
    adf_nbuf_t      buf;

    num_desc = dma_q->num_desc;
    swdesc = dma_q->sw_ring;

    for (i = 0; i < num_desc; i++, swdesc++) {
        
        pci_zdma_mark_notrdy(swdesc);

        adf_nbuf_unmap(osdev, swdesc->nbuf_map, ADF_OS_DMA_TO_DEVICE);
        
        buf = pci_dma_unlink_buf(osdev, swdesc);
        
        adf_os_assert(buf);
        
        adf_nbuf_free(buf);

        adf_nbuf_dmamap_destroy(osdev, swdesc->nbuf_map);
    }

    pci_dma_free_swdesc(osdev, dma_q, num_desc);
}

/**
 * @brief Initialize the TX ring
 * 
 * @param osdev
 * @param dma_q
 * @param num_desc
 */
void    
pci_dma_init_tx(adf_os_device_t osdev, pci_dma_softc_t *dma_q,
                a_uint32_t num_desc)
{
    zdma_swdesc_t  *swdesc;
    a_uint32_t    i = 0;

    pci_dma_alloc_swdesc(osdev, dma_q, num_desc);

    swdesc = dma_q->sw_ring;

    for (i = 0; i < num_desc; i++) 
        adf_nbuf_dmamap_create(osdev, &swdesc[i].nbuf_map);
}

/**
 * @brief Free the TX Ring
 * 
 * @param osdev
 * @param dma_q
 */
void    
pci_dma_deinit_tx(adf_os_device_t osdev, pci_dma_softc_t *dma_q) 
{
    a_uint32_t  i, num_desc;
    zdma_swdesc_t  *swdesc;

    swdesc = dma_q->sw_ring;
    num_desc = dma_q->num_desc;
    
    for (i = 0; i < num_desc; i++, swdesc++) 
        adf_nbuf_dmamap_destroy(osdev, swdesc->nbuf_map);

    pci_dma_free_swdesc(osdev, dma_q, num_desc);
}




/* pci_dma_softc_t  *dma_q0 = NULL; */

/* void */
/* pci_prn_uncached(zdma_swdesc_t  *swdesc) */
/* { */
/*     adf_os_dma_addr_t        hwaddr = swdesc->hwaddr; */
/*  */
/*     printk("hwu = %#x ",hwaddr); */
/*     printk("swu = %#x ",swdesc->descp); */
/*  */
/*     hwaddr &= ~0x80000000; */
/*     hwaddr |=  0xa0000000; */
/*  */
/*     printk("hwc = %#x ",hwaddr); */
/*     printk("s_addr = %#x ",swdesc->buf_addr); */
/*     printk("h_addr = %#x ", ((struct zsDmaDesc *)hwaddr)->dataAddr); */
/*     printk("n_addr = %#x ", ((struct zsDmaDesc *)hwaddr)->nextAddr); */
/*     printk("l_addr = %#x\n", ((struct zsDmaDesc *)hwaddr)->lastAddr); */
/* } */
/*  */
/* void */
/* pci_prn_uncached0(pci_dma_softc_t  *dma_q) */
/* { */
/*     adf_os_dma_addr_t        hwaddr = dma_q->sw_ring[0].hwaddr; */
/*  */
/*     printk("hwu = %#x ",hwaddr); */
/*  */
/*     hwaddr &= ~0x80000000; */
/*     hwaddr |=  0xa0000000; */
/*  */
/*     printk("hwc = %#x ",hwaddr); */
/*     printk("s_addr = %#x ",dma_q->sw_ring[0].buf_addr); */
/*     printk("h_addr = %#x ", ((struct zsDmaDesc *)hwaddr)->dataAddr); */
/*     printk("n_addr = %#x ", ((struct zsDmaDesc *)hwaddr)->nextAddr); */
/*     printk("l_addr = %#x\n", ((struct zsDmaDesc *)hwaddr)->lastAddr); */
/* } */
/*  */




