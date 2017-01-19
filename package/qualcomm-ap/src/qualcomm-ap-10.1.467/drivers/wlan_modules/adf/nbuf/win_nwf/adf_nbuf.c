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
#ifdef CONVERGED_SW

#include <adf_os_types.h>
#include <adf_nbuf.h>
#include <wbuf.h>
#include "Mp_Defines.h"
#include "Mp_Send.h"

extern VOID Mp11IndicateCompletionQueue(PADAPTER pAdapter, BOOLEAN DispatchLevel);
extern VOID MpSendQueuedPackets(PADAPTER pAdapter);

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
 * @param prio
 * 
 * @return nbuf or NULL if no memory
 */
__adf_nbuf_t
__adf_nbuf_alloc(adf_os_device_t osdev, size_t size, int reserve, int align, int  prio)
{
    PLIST_ENTRY listEntry;
    __adf_nbuf_t pBuf = NULL;
    unsigned long offset = 0;
    char *data = NULL;

    ASSERT(size <= osdev->MaxBufSize);
    size = osdev->MaxBufSize;
    /* get pre-allocated __adf_nbuf_t structure */
    listEntry = NdisInterlockedRemoveHeadList((PLIST_ENTRY)&osdev->PreAllocatedBufQueue.Head,
                                              &osdev->PreAllocatedBufQueue.Lock);
    if (listEntry) {
        pBuf = CONTAINING_RECORD(listEntry, struct wbuf_vista, wb_entry);
        /* get pre-allocated data buffer */
        data = (char *)pBuf->wb_data;
        NdisZeroMemory(pBuf, sizeof(*pBuf));
        pBuf->wb_data = (PUCHAR)data;
        /* get pre-allocated network list and network buffer structure */
        pBuf->wb_os_buf = (PNET_BUFFER_LIST)NdisAllocateNetBufferAndNetBufferList(osdev->NetWorkListPoolHandle,
                                                                                  0, 0, NULL, 0, 0);
        if (!pBuf->wb_os_buf) {
            goto ERROR;
        } else {
            PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)pBuf->wb_os_buf);
            NdisZeroMemory(data, osdev->MaxBufSize);
            {
                PMDL pmdl = NdisAllocateMdl(osdev->MiniportAdapterHandle, data, (UINT)size);
                if (!pmdl)
                    goto ERROR;
                NET_BUFFER_FIRST_MDL(os_buf) = NET_BUFFER_CURRENT_MDL(os_buf) = pmdl;
            }
            /* Align & make sure that the tail & data are adjusted properly */
            data += reserve;
            if(align)
                offset = ((ULONG)(size_t)data) % align;
            if (offset)
                offset = align - offset;
            NET_BUFFER_DATA_OFFSET(os_buf) = NET_BUFFER_CURRENT_MDL_OFFSET(os_buf) = reserve + offset;
            NET_BUFFER_DATA_LENGTH(os_buf) = 0;
            NET_BUFFER_NEXT_NB(os_buf) = NULL;
            pBuf->wb_dev = (void*)osdev;
            atomic_set(&pBuf->wb_ref, 1);
        }
    }
    else
    {
        ASSERT(0);
    }

    return pBuf;

ERROR:
    if (pBuf->wb_os_buf)
        NdisFreeNetBufferList((PNET_BUFFER_LIST)pBuf->wb_os_buf);
    NdisInterlockedInsertTailList((PLIST_ENTRY)&osdev->PreAllocatedBufQueue.Head,
                                &pBuf->wb_entry,
                                &osdev->PreAllocatedBufQueue.Lock);
    return NULL;
}

/*
 * @brief free the nbuf its interrupt safe
 * @param skb
 */
void
__adf_nbuf_free(__adf_nbuf_t skb)
{
    if (skb->wb_dev) {
        if (atomic_dec_and_test(&skb->wb_ref)) {
            void *data = NULL;
            ULONG length = 0;
            adf_nbuf_t frag_list = skb->wb_frag_list;
            adf_os_device_t osdev = (adf_os_device_t)skb->wb_dev;
            PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)skb->wb_os_buf);
            PMDL next, pmdl = NET_BUFFER_FIRST_MDL(os_buf);
            while (pmdl) {
                next = pmdl->Next;
                //From MSDN: The caller of NdisAdjustMdlLength must restore the length to its original value before it frees the MDL descriptor with NdisFreeMdl.
                NdisAdjustMdlLength(pmdl, osdev->MaxBufSize);
                NdisFreeMdl(pmdl);
                pmdl = next;
            }
            NET_BUFFER_FIRST_MDL(os_buf) = NET_BUFFER_CURRENT_MDL(os_buf) = NULL;
            NET_BUFFER_DATA_OFFSET(os_buf) = 0;
            NET_BUFFER_CURRENT_MDL_OFFSET(os_buf) = 0;
            NET_BUFFER_DATA_LENGTH(os_buf) = 0;
            /* free network list and network buffer structure */
            NdisFreeNetBufferList((PNET_BUFFER_LIST)skb->wb_os_buf);
            /* free frag list if any */
            while (frag_list) {
                adf_nbuf_t next = adf_nbuf_next_ext(frag_list);
                adf_nbuf_set_next_ext(frag_list, NULL);
                adf_nbuf_free(frag_list);
                frag_list = next;
            }
            /* free __adf_nbuf_t structure */
            NdisInterlockedInsertTailList((PLIST_ENTRY)&osdev->PreAllocatedBufQueue.Head,
                                          &skb->wb_entry,
                                          &osdev->PreAllocatedBufQueue.Lock);
        }
    } else {
        /* this buffer is allocated by wbuf_alloc. generally WMI command buffer or TX data from OS */
        switch (skb->wb_type) {
        case WBUF_TX_MGMT:
            /* WMI command is freed by wbuf_free */
            /* Some WMI command bufs are not freed when stopping HTC during surprise removal. Free it to avoid memory leak*/
            wbuf_complete(skb);
            break;
        case WBUF_TX_DATA:
            wbuf_complete(skb);
            break;
        default:
            ASSERT(0);
            break;
        }
    }
}

int
__adf_nbuf_get_frag_is_wordstream(__adf_nbuf_t skb, int frag_num)
{
    if (frag_num < skb->wb_extra_frags.num)
        return (skb->wb_extra_frags.wordstream_flags >> frag_num) & 0x01;

    return (skb->wb_extra_frags.wordstream_flags >> CVG_NBUF_MAX_EXTRA_FRAGS) & 0x01;
}

void
__adf_nbuf_set_frag_is_wordstream(__adf_nbuf_t skb, int frag_num, int is_wordstream)
{
    if (frag_num >= skb->wb_extra_frags.num)
        frag_num = CVG_NBUF_MAX_EXTRA_FRAGS;
    skb->wb_extra_frags.wordstream_flags &= ~(1 << frag_num);
    skb->wb_extra_frags.wordstream_flags |= (!!is_wordstream) << frag_num;
}

/*
 * @brief Reference the nbuf so it can get held until the last free.
 * @param skb
 */

void
__adf_nbuf_ref(__adf_nbuf_t skb)
{
    if (skb->wb_dev)
        atomic_inc(&skb->wb_ref);
    else
        ASSERT(0);
}

/*
 * @brief Check whether the buffer is shared 
 * @param skb
 */
int 
__adf_nbuf_shared(__adf_nbuf_t skb)
{
    /* TODO: implement __adf_nbuf_shared */
    ASSERT(0);
    return 0;
}

/**
 * @brief create a nbuf map
 * @param osdev
 * @param dmap
 * 
 * @return a_status_t
 */
a_status_t
__adf_nbuf_dmamap_create(adf_os_device_t osdev, __adf_os_dma_map_t *dmap)
{
    /* This function should not be called for peregrine */
    ASSERT(0);
    return A_STATUS_ENOTSUPP;
}

/**
 * @brief free the nbuf map
 * 
 * @param osdev
 * @param dmap
 */
void
__adf_nbuf_dmamap_destroy(adf_os_device_t osdev, __adf_os_dma_map_t dmap)
{
    /* This function should not be called for peregrine */
    ASSERT(0);
}

/**
 * @brief get the dma map of the nbuf
 * 
 * @param osdev
 * @param skb
 * @param dir
 * 
 * @return a_status_t
 */
a_status_t
__adf_nbuf_map(adf_os_device_t osdev, __adf_nbuf_t skb, adf_os_dma_dir_t dir)
{
    adf_os_assert(
        (dir == ADF_OS_DMA_TO_DEVICE) || (dir == ADF_OS_DMA_FROM_DEVICE));
    return __adf_nbuf_map_single(osdev, skb, dir);
}

/**
 * @brief adf_nbuf_unmap() - to unmap a previously mapped buf
 */
void
__adf_nbuf_unmap(adf_os_device_t osdev, __adf_nbuf_t skb, adf_os_dma_dir_t dir)
{
    adf_os_assert(
        (dir == ADF_OS_DMA_TO_DEVICE) || (dir == ADF_OS_DMA_FROM_DEVICE));
    __adf_nbuf_unmap_single(osdev, skb, dir);
}

a_status_t
__adf_nbuf_map_single(adf_os_device_t osdev, adf_nbuf_t skb, adf_os_dma_dir_t dir)
{
    if (skb->wb_dev) {
        /* map for NIC to access */
        adf_os_device_t osdev = (adf_os_device_t)skb->wb_dev;
        PNET_BUFFER os_buf = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)skb->wb_os_buf);
        PMDL pmdl = NET_BUFFER_FIRST_MDL(os_buf);
        int8_t *data = NULL;
        ULONG length = 0;
        PHYSICAL_ADDRESS addr;
        /* this should not fail since we are using non-pageable memory */
        NdisQueryMdl(pmdl, &data, &length, NormalPagePriority);
        addr = MmGetPhysicalAddress(data);
        ASSERT(addr.HighPart == 0);
        ASSERT(addr.LowPart != 0);
        /* we are 32bit PCIE device, so just care about the lower part address */
        skb->wb_mapped_paddr_lo[0] = addr.LowPart + NET_BUFFER_DATA_OFFSET(os_buf);
        /* flush cache line to maintain coherence */
        KeFlushIoBuffers(pmdl, dir == ADF_OS_DMA_FROM_DEVICE, TRUE);
    } else {
        /* this buffer is allocated by wbuf_alloc. generally WMI command buffer or TX data from OS */
        switch (skb->wb_type) {
        case WBUF_TX_MGMT:
            skb->wb_mapped_paddr_lo[0] = wbuf_map_single(NULL, skb, BUS_DMA_TODEVICE, NULL);
            break;
        case WBUF_TX_DATA:
            /* TX data from OS should be mapped before given to HTT by OS shim */
            ASSERT(0);
            break;
        default:
            ASSERT(0);
            break;
        }
    }
    return A_STATUS_OK;
}

void
__adf_nbuf_unmap_single(adf_os_device_t osdev, adf_nbuf_t skb, adf_os_dma_dir_t dir)
{
    if (skb->wb_dev) {
        skb->wb_mapped_paddr_lo[0] = 0;
    } else {
        /* this buffer is allocated by wbuf_alloc. generally WMI command buffer or TX data from OS */
        switch (skb->wb_type) {
        case WBUF_TX_MGMT:
            skb->wb_mapped_paddr_lo[0] = 0;
            break;
        case WBUF_TX_DATA:
            {
                int i = 0;
                while (i < skb->wb_mapped_num) {
                    skb->wb_mapped_vaddr[i] = 0;
                    skb->wb_mapped_len[i] = 0;
                    skb->wb_mapped_paddr_lo[i++] = 0;
                }
                skb->wb_mapped_num = 0;
            }
            break;
        default:
            ASSERT(0);
            break;
        }
    }
}

/**
 * @brief return the dma map info 
 * 
 * @param[in]  bmap
 * @param[out] sg (map_info ptr)
 */
void 
__adf_nbuf_dmamap_info(__adf_os_dma_map_t bmap, adf_os_dmamap_info_t *sg)
{
    /* This function should not be called for peregrine */
    ASSERT(0);
}
/**
 * @brief return the frag data & len, where frag no. is
 *        specified by the index
 * 
 * @param[in] buf
 * @param[out] sg (scatter/gather list of all the frags)
 * 
 */
void  
__adf_nbuf_frag_info(__adf_nbuf_t skb, adf_os_sglist_t  *sg)
{
    /* This function should not be called for peregrine */
    ASSERT(0);
}

a_status_t 
__adf_nbuf_set_rx_cksum(__adf_nbuf_t skb, adf_nbuf_rx_cksum_t *cksum)
{
    return A_STATUS_OK;
}

adf_nbuf_tx_cksum_t
__adf_nbuf_get_tx_cksum(__adf_nbuf_t skb)
{
    return ADF_NBUF_TX_CKSUM_NONE;
}

a_status_t      
__adf_nbuf_get_vlan_info(adf_net_handle_t hdl, __adf_nbuf_t skb, 
                         adf_net_vlanhdr_t *vlan)
{
    /* This function should not be called for peregrine */
    ASSERT(0);
    return A_STATUS_ENOTSUPP;
}

a_uint8_t
__adf_nbuf_get_tid(__adf_nbuf_t skb)
{
    return ADF_NBUF_TX_EXT_TID_INVALID;
}

void
__adf_nbuf_dmamap_set_cb(__adf_os_dma_map_t dmap,
                         void cb(void *, __adf_nbuf_t, adf_os_dma_map_t), void *arg)
{
    /* This function should not be called for peregrine */
    ASSERT(0);
}

void
__adf_nbuf_tx_free(__adf_nbuf_t bufs, int tx_err)
{
    PMP_TX_MSDU         pMpTxd = NULL;
    PADAPTER            pAdapter = NULL;
    u_int32_t           status = tx_err ? WB_STATUS_TX_ERROR : WB_STATUS_OK;

    if (!bufs)
        return;

    if(bufs->wb_dev)
    {
        // Internal TX Frame
        __adf_nbuf_free(bufs);
    }
    else
    {
        // TX Frame from OS
        ASSERT(bufs->wb_os_buf != NULL);
        ASSERT(bufs->wb_type == WBUF_TX_DATA);
        pMpTxd = (PMP_TX_MSDU)bufs->wb_os_buf;
        pAdapter = pMpTxd->Adapter;

        while (bufs) {
            __adf_nbuf_t next = __adf_nbuf_next(bufs);
            if (bufs->wb_frag_list)
            {
                /* free coalescing TX buffer for too fragmented TX MSDU */
                __adf_nbuf_free(bufs->wb_frag_list);
                bufs->wb_frag_list = NULL;
            }
            wbuf_set_status(bufs, status);
            __adf_nbuf_free(bufs);
            bufs = next;
        }
        /* Indicate packets completed */
        Mp11IndicateCompletionQueue(pAdapter, FALSE);

        /* Try to send any queued frames in all the queues */
        MpSendQueuedPackets(pAdapter);
    }        
    
}

#endif
