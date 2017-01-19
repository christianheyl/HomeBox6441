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

/*
 * Linux implemenation of skbuf
 */
#ifndef _ADF_CMN_NET_PVT_BUF_H
#define _ADF_CMN_NET_PVT_BUF_H

#include <adf_os_types.h>
#include <wbuf.h>

#define __ADF_NBUF_NULL   NULL

typedef struct wbuf_vista *     __adf_nbuf_t;

#define __adf_nbuf_get_num_frags(skb)   (skb->wb_extra_frags.num + skb->wb_mapped_num)

#define __adf_nbuf_frag_push_head(skb, frag_len, frag_vaddr, frag_paddr_lo, frag_paddr_hi) \
    do { \
        if (skb->wb_extra_frags.num < CVG_NBUF_MAX_EXTRA_FRAGS) { \
            skb->wb_extra_frags.vaddr[skb->wb_extra_frags.num] = frag_vaddr; \
            skb->wb_extra_frags.paddr_lo[skb->wb_extra_frags.num] = frag_paddr_lo; \
            skb->wb_extra_frags.len[skb->wb_extra_frags.num] = frag_len; \
            skb->wb_extra_frags.num++; \
        } \
    } while (0)

static inline int __adf_nbuf_get_frag_len(__adf_nbuf_t skb, int frag_num)
{
    if (frag_num < skb->wb_extra_frags.num)
        return skb->wb_extra_frags.len[frag_num];

    frag_num -= skb->wb_extra_frags.num;
    if (skb->wb_dev)
    {
        adf_os_device_t osdev = (adf_os_device_t)skb->wb_dev;
        PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)skb->wb_os_buf);
        ASSERT(frag_num == 0);
        return NET_BUFFER_DATA_LENGTH(os_buf);
    }
    else
    {
        /* this buffer is allocated by wbuf_alloc. generally WMI command buffer or TX data from OS */
        switch (skb->wb_type) {
        case WBUF_TX_MGMT:
            ASSERT(frag_num == 0);
            return wbuf_get_pktlen(skb);
        case WBUF_TX_DATA:
            if (frag_num < skb->wb_mapped_num)
                return skb->wb_mapped_len[frag_num];
            else
                ASSERT(0);
            break;
        default:
            ASSERT(0);
            break;
        }
    }

    return 0;
}

static inline unsigned char * __adf_nbuf_get_frag_vaddr(__adf_nbuf_t skb, int frag_num)
{
    if (frag_num < skb->wb_extra_frags.num)
        return (unsigned char*)skb->wb_extra_frags.vaddr[frag_num];

    frag_num -= skb->wb_extra_frags.num;
    if (skb->wb_dev)
    {
        adf_os_device_t osdev = (adf_os_device_t)skb->wb_dev;
        PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)skb->wb_os_buf);
        PMDL pmdl = NET_BUFFER_FIRST_MDL(os_buf);
        unsigned char *data = NULL;
        ULONG length = 0;
        ASSERT(frag_num == 0);
        /* this should not fail since we are using non-pageable memory */
        NdisQueryMdl(pmdl, &data, &length, NormalPagePriority);
        return data+NET_BUFFER_DATA_OFFSET(os_buf);
    }
    else
    {
        /* this buffer is allocated by wbuf_alloc. generally WMI command buffer or TX data from OS */
        switch (skb->wb_type) {
        case WBUF_TX_MGMT:
            ASSERT(frag_num == 0);
            return (unsigned char *)(skb->wb_header);
        case WBUF_TX_DATA:
            if (frag_num < skb->wb_mapped_num)
                return (unsigned char *)skb->wb_mapped_vaddr[frag_num];
            else
                ASSERT(0);
            break;
        default:
            ASSERT(0);
            break;
        }
    }
    if (frag_num < skb->wb_mapped_num)
        return (unsigned char *)skb->wb_mapped_vaddr[frag_num];

    return NULL;
}

static inline a_uint32_t __adf_nbuf_get_frag_paddr_lo(__adf_nbuf_t skb, int frag_num)
{
    if (frag_num < skb->wb_extra_frags.num)
        return skb->wb_extra_frags.paddr_lo[frag_num];

    frag_num -= skb->wb_extra_frags.num;
    if (skb->wb_dev)
    {
        ASSERT(frag_num == 0);
        return skb->wb_mapped_paddr_lo[frag_num];
    }
    else
    {
        /* this buffer is allocated by wbuf_alloc. generally WMI command buffer or TX data from OS */
        switch (skb->wb_type) {
        case WBUF_TX_MGMT:
            ASSERT(frag_num == 0);
            return skb->wb_mapped_paddr_lo[frag_num];
        case WBUF_TX_DATA:
            if (frag_num < skb->wb_mapped_num)
                return skb->wb_mapped_paddr_lo[frag_num];
            else
                ASSERT(0);
            break;
        default:
            ASSERT(0);
            break;
        }
    }

    return 0;
}

/*
 * nbufs on the power save queue are tagged with an age and
 * timed out.  We reuse the hardware checksum field in the
 * nbuf packet header to store this data.
 */

typedef struct __adf_nbuf_qhead {
    __adf_nbuf_t    head;
    __adf_nbuf_t    tail;
    __a_uint32_t    qlen;
}__adf_nbuf_queue_t;

/*
 * prototypes. Implemented in adf_nbuf_pvt.c
 */
__adf_nbuf_t    __adf_nbuf_alloc(__adf_os_device_t osdev, size_t size, int reserve, int align, int prio);
void            __adf_nbuf_free(__adf_nbuf_t skb);
void            __adf_nbuf_ref(__adf_nbuf_t skb);
int             __adf_nbuf_shared(__adf_nbuf_t skb);
int             __adf_nbuf_get_frag_is_wordstream(__adf_nbuf_t skb, int frag_num);
void            __adf_nbuf_set_frag_is_wordstream(__adf_nbuf_t skb, int frag_num, int is_wordstream);
a_status_t      __adf_nbuf_dmamap_create(__adf_os_device_t osdev, 
                                         __adf_os_dma_map_t *dmap);
void            __adf_nbuf_dmamap_destroy(__adf_os_device_t osdev, 
                                          __adf_os_dma_map_t dmap);
a_status_t      __adf_nbuf_map(__adf_os_device_t osdev,
                    __adf_nbuf_t skb, adf_os_dma_dir_t dir);
void            __adf_nbuf_unmap(__adf_os_device_t osdev,
                    __adf_nbuf_t skb, adf_os_dma_dir_t dir);
a_status_t      __adf_nbuf_map_single(__adf_os_device_t osdev,
                    __adf_nbuf_t skb, adf_os_dma_dir_t dir);
void            __adf_nbuf_unmap_single(__adf_os_device_t osdev,
                    __adf_nbuf_t skb, adf_os_dma_dir_t dir);
void            __adf_nbuf_dmamap_info(__adf_os_dma_map_t bmap, adf_os_dmamap_info_t *sg);
void            __adf_nbuf_frag_info(__adf_nbuf_t skb, adf_os_sglist_t  *sg);
void            __adf_nbuf_dmamap_set_cb(__adf_os_dma_map_t dmap,
                    void cb(void *, __adf_nbuf_t, adf_os_dma_map_t), void *arg);
void            __adf_nbuf_tx_free(__adf_nbuf_t bufs, int tx_err);

#define __adf_nbuf_reserve(buf, size)   ASSERT(0)

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
static inline __adf_nbuf_t
__adf_nbuf_realloc_headroom(__adf_nbuf_t skb, __a_uint32_t headroom) 
{
    /* should not be called in peregrine project */
    ASSERT(0);
    return NULL;
}

/**
 * @brief return the length of the copy bits for skb
 * 
 * @param skb, offset, len, to
 * 
 * @return int32_t
 */
static inline int32_t
__adf_nbuf_copy_bits(__adf_nbuf_t m, int32_t offset, int32_t len, void *to)
{
    //return skb_copy_bits(skb, offset, to, len);
    ASSERT(0);
    return 0;
}

/**
 * @brief This keeps the skb shell intact exapnds the tailroom
 *        in data region. In case of failure it releases the
 *        skb.
 * 
 * @param skb
 * @param tailroom
 * 
 * @return skb or NULL
 */
static inline __adf_nbuf_t
__adf_nbuf_realloc_tailroom(__adf_nbuf_t skb, __a_uint32_t tailroom)
{
    /* should not be called in peregrine project */
    ASSERT(0);
    return NULL;
}

/**
 * @brief return the amount of valid data in the skb, If there
 *        are frags then it returns total length.
 * 
 * @param skb
 * 
 * @return size_t
 */
static inline size_t
__adf_nbuf_len(__adf_nbuf_t skb)
{
    int i, extra_frag_len = 0;

    i = skb->wb_extra_frags.num;
    while (i-- > 0)
        extra_frag_len += skb->wb_extra_frags.len[i];

    if (skb->wb_dev)
    {
        adf_os_device_t osdev = (adf_os_device_t)skb->wb_dev;
        PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)skb->wb_os_buf);
        extra_frag_len += NET_BUFFER_DATA_LENGTH(os_buf);
    }
    else
    {
        /* this buffer is allocated by wbuf_alloc. generally WMI command buffer or TX data from OS */
        switch (skb->wb_type) {
        case WBUF_TX_MGMT:
            extra_frag_len += wbuf_get_pktlen(skb);
            break;
        case WBUF_TX_DATA:
            i = skb->wb_mapped_num;
            while (i-- > 0)
                extra_frag_len += skb->wb_mapped_len[i];
            break;
        default:
            ASSERT(0);
            break;
        }
    }

    return extra_frag_len;
}

/**
 * @brief  create a version of the specified nbuf whose contents
 *         can be safely modified without affecting other
 *         users.If the nbuf is a clone then this function
 *         creates a new copy of the data. If the buffer is not
 *         a clone the original buffer is returned.
 *
 * @param skb (source nbuf to create a writable copy from)
 *
 * @return skb or NULL
 */
static inline __adf_nbuf_t
__adf_nbuf_unshare(__adf_nbuf_t skb)
{
    /* should not be called in peregrine project */
    ASSERT(0);
    return NULL;
}

/**************************nbuf manipulation routines*****************/

static inline a_uint32_t
__adf_nbuf_headroom(__adf_nbuf_t skb)
{
    if (skb->wb_dev)
    {
        /* single frame only */
        adf_os_device_t osdev = (adf_os_device_t)skb->wb_dev;
        PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)skb->wb_os_buf);
        return NET_BUFFER_DATA_OFFSET(os_buf);
    }
    else
        ASSERT(0);
    return 0;
}

/**
 * @brief return the amount of tail space available
 *
 * @param buf
 *
 * @return amount of tail room
 */
static inline a_uint32_t
__adf_nbuf_tailroom(__adf_nbuf_t skb)
{
    if (skb->wb_dev)
    {
        /* single frame only */
        adf_os_device_t osdev = (adf_os_device_t)skb->wb_dev;
        PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)skb->wb_os_buf);
        return (osdev->MaxBufSize - NET_BUFFER_DATA_OFFSET(os_buf) - NET_BUFFER_DATA_LENGTH(os_buf));
    }
    else
        ASSERT(0);
    return 0;
}

/**
 * @brief Push data in the front
 *
 * @param[in] buf      buf instance
 * @param[in] size     size to be pushed
 *
 * @return New data pointer of this buf after data has been pushed,
 *         or NULL if there is not enough room in this buf.
 */
static inline __a_uint8_t *
__adf_nbuf_push_head(__adf_nbuf_t skb, size_t size)
{
    if (skb->wb_dev)
    {
        /* single frame only */
        PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)skb->wb_os_buf);
        PMDL pmdl = NET_BUFFER_FIRST_MDL(os_buf);
        __a_uint8_t *data = NULL;
        ULONG length = 0;
        /* this should not fail since we are using non-pageable memory */
        NdisQueryMdl(pmdl, &data, &length, NormalPagePriority);
        ASSERT(size <= NET_BUFFER_DATA_OFFSET(os_buf));
        NET_BUFFER_DATA_OFFSET(os_buf) -= (ULONG)size;
        NET_BUFFER_CURRENT_MDL_OFFSET(os_buf) = NET_BUFFER_DATA_OFFSET(os_buf);
        NET_BUFFER_DATA_LENGTH(os_buf) += (ULONG)size;
        data += NET_BUFFER_DATA_OFFSET(os_buf);
        return data;
    }
    else
    {
        /* this buffer is allocated by wbuf_alloc. generally WMI command buffer or TX data from OS */
        switch (skb->wb_type) {
        case WBUF_TX_MGMT:
            return wbuf_push(skb, (u_int16_t)size);
        case WBUF_TX_DATA:
            ASSERT(0);
            break;
        default:
            ASSERT(0);
            break;
        }
    }
    return NULL;
}

/**
 * @brief Puts data in the end
 *
 * @param[in] buf      buf instance
 * @param[in] size     size to be pushed
 *
 * @return data pointer of this buf where new data has to be
 *         put, or NULL if there is not enough room in this buf.
 */
static inline __a_uint8_t *
__adf_nbuf_put_tail(__adf_nbuf_t skb, size_t size)
{
    __a_uint8_t *data = NULL;
    if (skb->wb_dev)
    {
        /* single frame only */
        PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)skb->wb_os_buf);
        PMDL pmdl = NET_BUFFER_FIRST_MDL(os_buf);
        ULONG length = 0;
        /* this should not fail since we are using non-pageable memory */
        NdisQueryMdl(pmdl, &data, &length, NormalPagePriority);
        data += NET_BUFFER_DATA_OFFSET(os_buf) + NET_BUFFER_DATA_LENGTH(os_buf);
        ASSERT((size + NET_BUFFER_DATA_OFFSET(os_buf) + NET_BUFFER_DATA_LENGTH(os_buf)) <= length);
        NET_BUFFER_DATA_LENGTH(os_buf) += (ULONG)size;
        return data;
    }
    else
    {
        /* this buffer is allocated by wbuf_alloc. generally WMI command buffer or TX data from OS */
        switch (skb->wb_type) {
        case WBUF_TX_MGMT:
            data = skb->wb_header;
            data += skb->wb_pktlen;
            wbuf_append(skb, (u_int16_t)size);
            return data;
        case WBUF_TX_DATA:
            ASSERT(0);
            break;
        default:
            ASSERT(0);
            break;
        }
    }
    return NULL;
}

/**
 * @brief pull data out from the front
 *
 * @param[in] buf   buf instance
 * @param[in] size  size to be popped
 *
 * @return New data pointer of this buf after data has been popped,
 *         or NULL if there is not sufficient data to pull.
 */
static inline __a_uint8_t *
__adf_nbuf_pull_head(__adf_nbuf_t skb, size_t size)
{
    if (skb->wb_dev && skb->wb_os_buf)
    {
        /* single frame only */
        PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)skb->wb_os_buf);
        PMDL pmdl = NET_BUFFER_FIRST_MDL(os_buf);
        __a_uint8_t *data = NULL;
        ULONG length = 0;
        /* this should not fail since we are using non-pageable memory */
        NdisQueryMdl(pmdl, &data, &length, NormalPagePriority);
        data += NET_BUFFER_DATA_OFFSET(os_buf) + (ULONG)size;
        ASSERT((size + NET_BUFFER_DATA_OFFSET(os_buf)) <= length);
        if (NET_BUFFER_DATA_LENGTH(os_buf) <= size)
            NET_BUFFER_DATA_LENGTH(os_buf) = 0;
        else
            NET_BUFFER_DATA_LENGTH(os_buf) -= (ULONG)size;
        NET_BUFFER_DATA_OFFSET(os_buf) += (ULONG)size;
        NET_BUFFER_CURRENT_MDL_OFFSET(os_buf) = NET_BUFFER_DATA_OFFSET(os_buf);
        return data;
    }
    else
    {
        /* this buffer is allocated by wbuf_alloc. generally WMI command buffer or TX data from OS */
        switch (skb->wb_type) {
        case WBUF_TX_MGMT:
            CHECK_VALID_WBUF(skb);
            ASSERT(skb->wb_pktlen >= size);
            skb->wb_header += size;
            skb->wb_pktlen -= size;
            return skb->wb_header;
        case WBUF_TX_DATA:
            ASSERT(0);
            break;
        default:
            ASSERT(0);
            break;
        }
    }
    return NULL;
}

/**
 *
 * @brief trim data out from the end
 *
 * @param[in] buf   buf instance
 * @param[in] size     size to be popped
 *
 * @return none
 */
static inline void
__adf_nbuf_trim_tail(__adf_nbuf_t skb, size_t size)
{
    if (skb->wb_dev)
    {
        /* single frame only */
        PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)skb->wb_os_buf);
        PMDL pmdl = NET_BUFFER_FIRST_MDL(os_buf);
        __a_uint8_t *data = NULL;
        ULONG length = 0;
        /* this should not fail since we are using non-pageable memory */
        NdisQueryMdl(pmdl, &data, &length, NormalPagePriority);
        ASSERT(size <= NET_BUFFER_DATA_LENGTH(os_buf));
        NET_BUFFER_DATA_LENGTH(os_buf) -= (ULONG)size;
    }
    else
    {
        /* this buffer is allocated by wbuf_alloc. generally WMI command buffer or TX data from OS */
        switch (skb->wb_type) {
        case WBUF_TX_MGMT:
            wbuf_trim(skb, (u_int16_t)size);
            break;
        case WBUF_TX_DATA:
            ASSERT(0);
            break;
        default:
            ASSERT(0);
            break;
        }
    }
}

/**
 * @brief sets the length of the nbuf
 * 
 * @param nbuf, length
 * 
 * @return void
 */
static inline void
__adf_nbuf_set_pktlen(__adf_nbuf_t nbuf, uint32_t len)
{
    wbuf_set_pktlen(nbuf, len);
}

/**
 * @brief test whether the nbuf is cloned or not
 *
 * @param buf
 *
 * @return a_bool_t (TRUE if it is cloned or else FALSE)
 */
static inline a_bool_t
__adf_nbuf_is_cloned(__adf_nbuf_t skb)
{
    /* should not be called in peregrine project */
    ASSERT(0);
    return 0;
}

/*********************nbuf private buffer routines*************/

/**
 * @brief get the priv pointer from the nbuf'f private space
 *
 * @param buf
 *
 * @return data pointer to typecast into your priv structure
 */
static inline __a_uint8_t *
__adf_nbuf_get_priv(__adf_nbuf_t skb)
{
    /* should not be called in peregrine project */
    ASSERT(0);
    return NULL;
}

/**
 * @brief This will return the header's addr & m_len
 */
static inline void
__adf_nbuf_peek_header(__adf_nbuf_t skb, __a_uint8_t **addr, __a_uint32_t *len)
{
    *addr = NULL;
    *len  = 0;

    if (skb->wb_dev)
    {
        /* single frame only */
        PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)skb->wb_os_buf);
        PMDL pmdl = NET_BUFFER_FIRST_MDL(os_buf);
        __a_uint8_t *data = NULL;
        ULONG length = 0;
        /* this should not fail since we are using non-pageable memory */
        NdisQueryMdl(pmdl, &data, &length, NormalPagePriority);
        *addr = data + NET_BUFFER_DATA_OFFSET(os_buf);
        *len = NET_BUFFER_DATA_LENGTH(os_buf);
    }
    else
    {
        /* this buffer is allocated by wbuf_alloc. generally WMI command buffer or TX data from OS */
        switch (skb->wb_type) {
        case WBUF_TX_MGMT:
            *addr = skb->wb_header;
            *len = skb->wb_pktlen;
            break;
        case WBUF_TX_DATA:
            ASSERT(0);
            break;
        default:
            ASSERT(0);
            break;
        }
    }
}

/**
 * @brief initiallize the queue head
 * 
 * @param qhead
 */
static inline a_status_t
__adf_nbuf_queue_init(__adf_nbuf_queue_t *qhead)
{
    OS_MEMZERO(qhead, sizeof(__adf_nbuf_queue_t));
    return A_STATUS_OK;
}

/**
 * @brief add an skb in the tail of the queue. This is a
 *        lockless version, driver must acquire locks if it
 *        needs to synchronize
 * 
 * @param qhead
 * @param skb
 */
static inline void
__adf_nbuf_queue_add(__adf_nbuf_queue_t *qhead, 
                     __adf_nbuf_t skb)
{
    skb->wb_cv_next = NULL;/*Nullify the next ptr*/

    if(!qhead->head)
        qhead->head = skb;
    else
        qhead->tail->wb_cv_next = skb;
    
    qhead->tail = skb;
    qhead->qlen++;
}

/**
 * @brief add an skb at  the head  of the queue. This is a
 * lockless version, driver must acquire locks if it
 * needs to synchronize
 * 
 * @param qhead
 * @param skb
 */
static inline void
__adf_nbuf_queue_insert_head(__adf_nbuf_queue_t *qhead, 
                     __adf_nbuf_t skb)
{
    if(!qhead->head){
        /*Empty queue Tail pointer Must be updated */
        qhead->tail = skb;
    }
    skb->wb_cv_next = qhead->head;
    qhead->head = skb;
    qhead->qlen++;
}

/**
 * @brief remove a skb from the head of the queue, this is a
 * lockless version. Driver should take care of the locks
 * 
 * @param qhead
 * 
 * @return skb or NULL
 */
static inline __adf_nbuf_t
__adf_nbuf_queue_remove(__adf_nbuf_queue_t * qhead)
{
    __adf_nbuf_t  tmp = NULL;

    if (qhead->head) {
        qhead->qlen--;
        tmp = qhead->head;
        if ( qhead->head == qhead->tail ) {
            qhead->head = NULL;
            qhead->tail = NULL;
        } else {
            qhead->head = tmp->wb_cv_next;
        }
        tmp->wb_cv_next = NULL;
    }
    return tmp;
}

/**
 * @brief return the queue length
 * 
 * @param qhead
 * 
 * @return __a_uint32_t
 */
static inline __a_uint32_t
__adf_nbuf_queue_len(__adf_nbuf_queue_t * qhead)
{
    return qhead->qlen;
}
/**
 * @brief returns the first skb in the queue
 * 
 * @param qhead
 * 
 * @return (NULL if the Q is empty)
 */
static inline __adf_nbuf_t 
__adf_nbuf_queue_first(__adf_nbuf_queue_t *qhead)
{
    return qhead->head;
}
/**
 * @brief return the next skb from packet chain, remember the
 *        skb is still in the queue
 * 
 * @param buf (packet)
 * 
 * @return (NULL if no packets are there)
 */
static inline __adf_nbuf_t 
__adf_nbuf_queue_next(__adf_nbuf_t skb)
{
    return skb->wb_cv_next;
}
/**
 * @brief check if the queue is empty or not
 * 
 * @param qhead
 * 
 * @return a_bool_t
 */
static inline a_bool_t
__adf_nbuf_is_queue_empty(__adf_nbuf_queue_t *qhead)
{
    return (qhead->qlen == 0);
}

/*
 * Use sk_buff_head as the implementation of adf_nbuf_queue_t.
 * Because the queue head will most likely put in some structure,
 * we don't use pointer type as the definition.
 */

/*
 * prototypes. Implemented in adf_nbuf_pvt.c
 */
adf_nbuf_tx_cksum_t __adf_nbuf_get_tx_cksum(__adf_nbuf_t skb);
a_status_t          __adf_nbuf_set_rx_cksum(__adf_nbuf_t skb, 
                                        adf_nbuf_rx_cksum_t *cksum);
a_status_t          __adf_nbuf_get_vlan_info(adf_net_handle_t hdl, 
                                         __adf_nbuf_t skb, 
                                         adf_net_vlanhdr_t *vlan);
a_uint8_t __adf_nbuf_get_tid(__adf_nbuf_t skb);

/**
 * @brief Expand both tailroom & headroom. In case of failure
 *        release the skb.
 * 
 * @param skb
 * @param headroom
 * @param tailroom
 * 
 * @return skb or NULL
 */

static inline __adf_nbuf_t
__adf_nbuf_expand(__adf_nbuf_t skb, __a_uint32_t headroom, __a_uint32_t tailroom)
{
    /* should not be called in peregrine project */
    ASSERT(0);
    return NULL;
}

/**
 * @brief clone the nbuf (copy is readonly)
 *
 * @param src_nbuf (nbuf to clone from)
 * @param dst_nbuf (address of the cloned nbuf)
 *
 * @return status
 * 
 * @note if GFP_ATOMIC is overkill then we can check whether its
 *       called from interrupt context and then do it or else in
 *       normal case use GFP_KERNEL
 * @example     use "in_irq() || irqs_disabled()"
 * 
 * 
 */
static inline __adf_nbuf_t
__adf_nbuf_clone(__adf_nbuf_t skb)
{
    /* should not be called in peregrine project */
    ASSERT(0);
    return NULL;
}
/**
 * @brief returns a private copy of the skb, the skb returned is
 *        completely modifiable
 * 
 * @param skb
 * 
 * @return skb or NULL
 */
static inline __adf_nbuf_t
__adf_nbuf_copy(__adf_nbuf_t skb)
{
    if (skb->wb_dev)
    {
        /* single frame only */
        PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)skb->wb_os_buf);
        PMDL pmdl = NET_BUFFER_FIRST_MDL(os_buf);
        __a_uint8_t *data = NULL, *p = NULL;
        ULONG length = 0;
        adf_os_device_t osdev = (adf_os_device_t)skb->wb_dev;
        __adf_nbuf_t copy = NULL;
        /* this should not fail since we are using non-pageable memory */
        NdisQueryMdl(pmdl, &data, &length, NormalPagePriority);
        data += NET_BUFFER_DATA_OFFSET(os_buf);
        copy = __adf_nbuf_alloc(osdev, length, NET_BUFFER_DATA_OFFSET(os_buf), 4, 0);
        if (copy)
        {
            p = __adf_nbuf_put_tail(copy, NET_BUFFER_DATA_LENGTH(os_buf));
            OS_MEMCPY(p, data, NET_BUFFER_DATA_LENGTH(os_buf));
            return copy;
        }
    }
    else
    {
        /* this buffer is allocated by wbuf_alloc. generally WMI command buffer or TX data from OS */
        switch (skb->wb_type) {
        case WBUF_TX_MGMT:
            ASSERT(0);
            break;
        case WBUF_TX_DATA:
            ASSERT(0);
            break;
        default:
            ASSERT(0);
            break;
        }
    }
    return NULL;
}

/***********************XXX: misc api's************************/
static inline a_bool_t
__adf_nbuf_tx_cksum_info(__adf_nbuf_t skb, __a_uint8_t **hdr_off, __a_uint8_t **where)
{
    /* should not be called in peregrine project */
    ASSERT(0);
    return A_FALSE;
}

/*
 * XXX What about other unions in skb? Windows might not have it, but we
 * should penalize linux drivers for it.
 * Besides this function is not likely doint the correct thing.
 */
static inline a_status_t
__adf_nbuf_get_tso_info(__adf_nbuf_t skb, adf_nbuf_tso_t *tso)
{
    /* should not be called in peregrine project */
    ASSERT(0);
    return A_STATUS_ENOTSUPP;
}

/**
 * @brief return the pointer to data header in the skb
 * 
 * @param skb
 * 
 * @return __a_uint8_t *
 */
static inline __a_uint8_t *
__adf_nbuf_data(__adf_nbuf_t skb)
{
    if (skb->wb_dev)
    {
        /* single frame only */
        PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)skb->wb_os_buf);
        PMDL pmdl = NET_BUFFER_FIRST_MDL(os_buf);
        __a_uint8_t *data = NULL;
        ULONG length = 0;
        /* this should not fail since we are using non-pageable memory */
        NdisQueryMdl(pmdl, &data, &length, NormalPagePriority);
        return data + NET_BUFFER_DATA_OFFSET(os_buf);
    }
    else
    {
        /* this buffer is allocated by wbuf_alloc. generally WMI command buffer or TX data from OS */
        switch (skb->wb_type) {
        case WBUF_TX_MGMT:
        case WBUF_TX_DATA:
            return skb->wb_header;
        default:
            ASSERT(0);
            break;
        }
    }
    return NULL;
}

/**
 * @brief return the pointer to data header in the skb
 * 
 * @param skb
 * 
 * @return __a_uint8_t *
 */
static inline __a_uint8_t *
__adf_nbuf_head(__adf_nbuf_t skb)
{
    return ((__a_uint8_t *) __adf_nbuf_data(skb)) - __adf_nbuf_headroom(skb);
}

/**
 * @brief sets the next skb pointer of the current skb
 * 
 * @param skb and next_skb
 * 
 * @return void
 */
static inline void
__adf_nbuf_set_next(__adf_nbuf_t skb, __adf_nbuf_t skb_next)
{
    skb->wb_cv_next = skb_next;
}

/**
 * @brief return the next skb pointer of the current skb
 * 
 * @param skb - the current skb
 * 
 * @return the next skb pointed to by the current skb
 */
static inline __adf_nbuf_t
__adf_nbuf_next(__adf_nbuf_t skb)
{
    return skb->wb_cv_next;
}


/**
 * @brief sets the next skb pointer of the current skb. This fn is used to
 * link up extensions to the head skb. Does not handle linking to the head
 * 
 * @param skb and next_skb
 * 
 * @return void
 */
static inline void
__adf_nbuf_set_next_ext(__adf_nbuf_t m, __adf_nbuf_t m_next)
{
    m->wb_ext_next = m_next;
}

/**
 * @brief return the next skb pointer of the current skb
 * 
 * @param skb - the current skb
 * 
 * @return the next skb pointed to by the current skb
 */
static inline __adf_nbuf_t
__adf_nbuf_next_ext(__adf_nbuf_t m)
{
	return m->wb_ext_next;
}

/**
 * @brief link list of packet extensions to the head segment
 * @details
 *  This function is used to link up a list of packet extensions (seg1, 2,
 *  ...) to the nbuf holding the head segment (seg0)
 * @param[in] head_buf nbuf holding head segment (single)
 * @param[in] ext_list nbuf list holding linked extensions to the head
 * @param[in] ext_len Total length of all buffers in the extension list
 */
static inline void
__adf_nbuf_append_ext_list(
        __adf_nbuf_t head, 
        __adf_nbuf_t ext_list, 
        size_t ext_len)
{
        head->wb_frag_list = ext_list;
        head->wb_frag_data_len = ext_len;
}

/**
 * @brief link two nbufs, the new buf is piggybacked into the
 *        older one. The older (src) skb is released.
 *
 * @param dst( buffer to piggyback into)
 * @param src (buffer to put)
 *
 * @return a_status_t (status of the call) if failed the src skb
 *         is released
 */
static inline a_status_t
__adf_nbuf_cat(__adf_nbuf_t dst, __adf_nbuf_t src)
{
    ULONG size, total = __adf_nbuf_len(src);
    UCHAR *dp, *sp;
    __adf_nbuf_t prev = dst, tmp = dst->wb_frag_list;
    while (tmp)
    {
        prev = tmp;
        tmp = __adf_nbuf_next_ext(tmp);
    }

    size = __adf_nbuf_tailroom(prev);
    if (size > total)
        size = total;
    total -= size;
    if (size)
    {
        dp = __adf_nbuf_put_tail(prev, size);
        sp = __adf_nbuf_data(src);
        OS_MEMCPY(dp, sp, size);
    }
    if (total)
    {
        PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)src->wb_os_buf);
        sp = __adf_nbuf_data(src) + size;
        OS_MEMCPY(src->wb_data, sp, total);
        NET_BUFFER_DATA_OFFSET(os_buf) = 0;
        NET_BUFFER_CURRENT_MDL_OFFSET(os_buf) = NET_BUFFER_DATA_OFFSET(os_buf) = 0;
        NET_BUFFER_DATA_LENGTH(os_buf) = total;
        if (prev == dst)
            prev->wb_frag_list = src;
        else
        {
            dst->wb_frag_data_len += size;
            __adf_nbuf_set_next_ext(prev, src);
        }
        dst->wb_frag_data_len += total;
    }
    else
    {
        __adf_nbuf_free(src);
    }

    return A_STATUS_OK;
}

/**
 * @brief should not be called
 *
 * @param: nbuf 
 *
 * @return: void
 */
static inline void
__adf_nbuf_reset_ctxt(__adf_nbuf_t nbuf)
{
    adf_os_assert_always(0);
}

#endif /*_adf_nbuf_PVT_H */
