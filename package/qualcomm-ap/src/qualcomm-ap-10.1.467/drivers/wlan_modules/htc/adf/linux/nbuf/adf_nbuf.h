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

#ifndef ADF_NBUF_H
#define ADF_NBUF_H

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/skbuff.h>
#include <adf_os_types.h>
#include <adf_os_util.h>

#define ADF_NBUF_NULL   NULL

typedef struct sk_buff*        adf_nbuf_t;

extern void dev_kfree_skb_any(struct sk_buff *skb);

/*
 * @brief This allocates an nbuf aligns if needed and reserves
 *        some space in the front, since the reserve is done
 *        after alignment the reserve value if being unaligned
 *        will result in an unaligned address.
 * 
 * @param hdl
 * @param size
 * @param reserve
 * @param align
 * 
 * @return nbuf or NULL if no memory
 */
static inline struct sk_buff*
adf_nbuf_alloc(adf_os_handle_t hdl, size_t size, int reserve, int align)
{
    struct sk_buff *skb;
    unsigned long offset;

    if(align)
        size += (align - 1);

    skb = dev_alloc_skb(size);

    if (skb == NULL) {
        printk("skg alloc failed\n");
        return NULL;
    }

    /**
     * XXX:how about we reserve first then align
     */

    /**
     * Align & make sure that the tail & data are adjusted properly
     */
    if(align){
        offset = ((unsigned long) skb->data) % align;
        if(offset)
            skb_reserve(skb, align - offset);
    }

    /**
     * Shim doesn't take responsibility if reserve unaligned the data
     * pointer
     */
    skb_reserve(skb, reserve);

    return skb;
}

/*
 * @brief free the nbuf its interrupt safe
 * @param skb
 */
static inline void
adf_nbuf_free(struct sk_buff *skb)
{
    dev_kfree_skb_any(skb);
}

static inline void
adf_nbuf_peek_header(struct sk_buff *skb, a_uint8_t **addr, 
                     a_uint32_t *len)
{
    *addr = skb->data;
    *len  = skb->len;
}

static inline a_uint8_t*
adf_nbuf_put_tail(struct sk_buff *skb, size_t size)
{
   return skb_put(skb, size);
}

static inline a_uint8_t*
adf_nbuf_pull_head(struct sk_buff *skb, size_t size)
{
    return skb_pull(skb, size);
}

static inline a_uint8_t*
adf_nbuf_push_head(struct sk_buff *skb, size_t size)
{
    return skb_push(skb, size);
}

static inline void
adf_nbuf_trim_tail(struct sk_buff *skb, size_t size)
{
    return skb_trim(skb, skb->len - size);
}

static inline size_t
adf_nbuf_len(struct sk_buff * skb)
{
    return skb->len;
}

static inline a_status_t
adf_os_to_status(signed int error)
{
    switch(error){
    case 0:
        return A_STATUS_OK;
    case ENOMEM:
    case -ENOMEM:
        return A_STATUS_ENOMEM;
    default:
        return A_STATUS_ENOTSUPP;
    }
}

static inline a_status_t
adf_nbuf_cat(struct sk_buff *dst, struct sk_buff *src)
{
    a_status_t error = 0;

    adf_os_assert(dst && src);

    if(!(error = pskb_expand_head(dst, 0, src->len, GFP_ATOMIC)))
        memcpy(skb_put(dst, src->len), src->data, src->len);

    dev_kfree_skb_any(src);

    return adf_os_to_status(error);
}

/**
 * @brief create a nbuf map
 * @param osdev
 * @param dmap
 * 
 * @return a_status_t
 */
static inline a_status_t
adf_nbuf_dmamap_create(adf_os_device_t osdev, adf_os_dma_map_t *dmap)
{
    a_status_t error = A_STATUS_OK;
    /**
     * XXX: driver can tell its SG capablity, it must be handled.
     * XXX: Bounce buffers if they are there
     */
    (*dmap) = kzalloc(sizeof(struct adf_os_dma_map), GFP_KERNEL);
    if(!(*dmap))
        error = A_STATUS_ENOMEM;

    return error;
}

/**
 * @brief free the nbuf map
 * 
 * @param osdev
 * @param dmap
 */
static inline void
adf_nbuf_dmamap_destroy(adf_os_device_t osdev, adf_os_dma_map_t dmap)
{
    kfree(dmap);
}

/**
 * @brief get the dma map of the nbuf
 * 
 * @param osdev
 * @param bmap
 * @param skb
 * @param dir
 * 
 * @return a_status_t
 */
static inline a_status_t
adf_nbuf_map(adf_os_device_t osdev, adf_os_dma_map_t bmap, 
               struct sk_buff *skb, adf_os_dma_dir_t dir)
{
    int len0 = 0;
    struct skb_shared_info  *sh = skb_shinfo(skb);

    adf_os_assert((dir == DMA_TO_DEVICE) || (dir == DMA_FROM_DEVICE));

    /**
     * if len != 0 then it's Tx or else Rx (data = tail)
     */
    len0                = (skb->len ? skb->len : skb_tailroom(skb));
    bmap->seg[0].daddr  = dma_map_single(osdev->bdev, skb->data, len0, dir);
    bmap->seg[0].len    = len0;
    bmap->nsegs         = 1;
    bmap->mapped        = 1;

#ifndef __ADF_SUPPORT_FRAG_MEM
    adf_os_assert(sh->nr_frags == 0);
#else
    adf_os_assert(sh->nr_frags <= ADF_OS_MAX_SCATTER);

    for (int i = 1; i <= sh->nr_frags; i++) {
        skb_frag_t *f           = &sh->frags[i-1]
        bmap->seg[i].daddr    = dma_map_page(osdev->bdev, 
                                             f->page, 
                                             f->page_offset,
                                             f->size, 
                                             dir);
        bmap->seg[i].len      = f->size;
    }
    /**
     * XXX: IP fragments, generally linux api's recurse, but how
     * will our map look like
     */
    bmap->nsegs += i;
    adf_os_assert(sh->frag_list == NULL);
#endif
    return A_STATUS_OK;
}

/**
 * @brief adf_nbuf_unmap() - to unmap a previously mapped buf
 */
static inline void
adf_nbuf_unmap(adf_os_device_t osdev, adf_os_dma_map_t bmap, 
                 adf_os_dma_dir_t dir)
{

    adf_os_assert(((dir == DMA_TO_DEVICE) || (dir == DMA_FROM_DEVICE)));

    dma_unmap_single(osdev->bdev, bmap->seg[0].daddr, bmap->seg[0].len, dir);

    bmap->mapped = 0;

    adf_os_assert(bmap->nsegs == 1);
    
}
/**
 * @brief return the dma map info 
 * 
 * @param[in]  bmap
 * @param[out] sg (map_info ptr)
 */
static inline void 
adf_nbuf_dmamap_info(adf_os_dma_map_t bmap, adf_os_dmamap_info_t *sg)
{
    adf_os_assert(bmap->mapped);
    adf_os_assert(bmap->nsegs <= ADF_OS_MAX_SCATTER);
    
    memcpy(sg->dma_segs, bmap->seg, bmap->nsegs * 
           sizeof(struct adf_os_segment));
    sg->nsegs = bmap->nsegs;
}

/**
 * @brief return the frag data & len, where frag no. is
 *        specified by the index
 * 
 * @param[in] buf
 * @param[out] sg (scatter/gather list of all the frags)
 * 
 */
static inline void  
adf_nbuf_frag_info(struct sk_buff *skb, adf_os_sglist_t *sg)
{
    struct skb_shared_info  *sh = skb_shinfo(skb);

    adf_os_assert(skb != NULL);
    sg->sg_segs[0].vaddr = skb->data;
    sg->sg_segs[0].len   = skb->len;
    sg->nsegs            = 1;

#ifndef __ADF_SUPPORT_FRAG_MEM
    adf_os_assert(sh->nr_frags == 0);
#else
    for(int i = 1; i <= sh->nr_frags; i++){
        skb_frag_t    *f        = &sh->frags[i - 1];
        sg->sg_segs[i].vaddr    = (uint8_t *)(page_address(f->page) + 
                                  f->page_offset);
        sg->sg_segs[i].len      = f->size;

        adf_os_assert(i < ADF_OS_MAX_SGLIST);
    }
    sg->nsegs += i;
#endif
}
/**
* @brief This keeps the skb shell intact expands the headroom
*        in the data region. In case of failure the skb is
*        released.
* 
* @param skb
* @param headroom
* 
* @return skb or NULL
 */
static inline struct sk_buff *
adf_nbuf_realloc_headroom(struct sk_buff *skb, uint32_t headroom)
{
    if(pskb_expand_head(skb, headroom, 0, GFP_ATOMIC)){
        dev_kfree_skb_any(skb);
        skb = NULL;
    }
    return skb;
}

static inline int
adf_nbuf_headroom(struct sk_buff *skb)
{
    return skb_headroom(skb);
}

#endif
