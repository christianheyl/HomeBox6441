//------------------------------------------------------------------------------
// Copyright (c) 2011 Atheros Communications Inc.
// All rights reserved.
//
// $ATH_LICENSE_HOSTSDK0_C$
//------------------------------------------------------------------------------

#if 0
#include <adf_os_lock.h>
#include <adf_os_stdtypes.h>
#include <adf_nbuf.h>
#endif
#include <osdep.h>
#include "a_types.h"
#include <athdefs.h>
#include "a_osapi.h"
#include "hif_msg_based.h"
#include "ath_pci.h"
#include "copy_engine_api.h"
#include "copy_engine_internal.h"
#include "ol_ath.h"
#include "adf_os_lock.h"
#include "hif_pci.h"

#if QCA_OL_11AC_FAST_PATH
#include <htt_types.h> /* struct htt_pdev_t */
#include <htc.h>
#include <htc_packet.h>
#include <hif.h>
#endif /* QCA_OL_11AC_FAST_PATH */

/*
 * This implementation is tied to HW, so we
 * make no attempt to use target_reg_table.h.
 * Instead, we directly include hardware
 * header files.
 *
 * If a future implementation varies only slightly,
 * we can revisit this decision.
 */
#include "hw/apb_athr_wlan_map.h"
#include "soc_core_reg.h"

/*
 * Support for Copy Engine hardware, which is mainly used for
 * communication between Host and Target over a PCIe interconnect.
 */

/*
 * A single CopyEngine (CE) comprises two "rings":
 *   a source ring
 *   a destination ring
 *
 * Each ring consists of a number of descriptors which specify
 * an address, length, and meta-data.
 *
 * Typically, one side of the PCIe interconnect (Host or Target)
 * controls one ring and the other side controls the other ring.
 * The source side chooses when to initiate a transfer and it
 * chooses what to send (buffer address, length). The destination
 * side keeps a supply of "anonymous receive buffers" available and
 * it handles incoming data as it arrives (when the destination
 * recieves an interrupt).
 *
 * The sender may send a simple buffer (address/length) or it may
 * send a small list of buffers.  When a small list is sent, hardware
 * "gathers" these and they end up in a single destination buffer
 * with a single interrupt.
 *
 * There are several "contexts" managed by this layer -- more, it
 * may seem -- than should be needed. These are provided mainly for
 * maximum flexibility and especially to facilitate a simpler HIF
 * implementation. There are per-CopyEngine recv, send, and watermark
 * contexts. These are supplied by the caller when a recv, send,
 * or watermark handler is established and they are echoed back to
 * the caller when the respective callbacks are invoked. There is
 * also a per-transfer context supplied by the caller when a buffer
 * (or sendlist) is sent and when a buffer is enqueued for recv.
 * These per-transfer contexts are echoed back to the caller when
 * the buffer is sent/received.
 */

/*
 * Guts of CE_send, used by both CE_send and CE_sendlist_send.
 * The caller takes responsibility for any needed locking.
 */
int
CE_completed_send_next_nolock(struct CE_state *CE_state,
                              void **per_CE_contextp,
                              void **per_transfer_contextp,
                              CE_addr_t *bufferp,
                              unsigned int *nbytesp,
                              unsigned int *transfer_idp);

int
CE_send_nolock(struct CE_handle *copyeng,
               void *per_transfer_context,
               CE_addr_t buffer,
               unsigned int nbytes,
               unsigned int transfer_id,
               unsigned int flags)
{
    int status;
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    struct CE_ring_state *src_ring = CE_state->src_ring;
    u_int32_t ctrl_addr = CE_state->ctrl_addr;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    A_target_id_t targid = TARGID(sc);
    unsigned int nentries_mask = src_ring->nentries_mask;
    unsigned int sw_index = src_ring->sw_index;
    unsigned int write_index = src_ring->write_index;
    
	A_ASSERT(nbytes <= CE_state->src_sz_max); /* TBDXXX -- may remove before tapeout */
    
	A_TARGET_ACCESS_BEGIN(targid);
    if (unlikely(CE_RING_DELTA(nentries_mask, write_index, sw_index-1) <= 0)) {
        OL_ATH_CE_PKT_ERROR_COUNT_INCR(sc,CE_RING_DELTA_FAIL);
        status = A_ERROR;
    	A_TARGET_ACCESS_END(targid);
    	return status;
    }
    {
        struct CE_src_desc *src_ring_base = (struct CE_src_desc *)src_ring->base_addr_owner_space;
        struct CE_src_desc *shadow_base = (struct CE_src_desc *)src_ring->shadow_base;
        struct CE_src_desc *src_desc = CE_SRC_RING_TO_DESC(src_ring_base, write_index);
        struct CE_src_desc *shadow_src_desc = CE_SRC_RING_TO_DESC(shadow_base, write_index);

        /* Update source descriptor */
        shadow_src_desc->src_ptr   = buffer;
        shadow_src_desc->meta_data = transfer_id;

        /*
         * Set the swap bit if:
         *   typical sends on this CE are swapped (host is big-endian) and
         *   this send doesn't disable the swapping (data is not bytestream)
         */
        shadow_src_desc->byte_swap =
            (((CE_state->attr_flags & CE_ATTR_BYTE_SWAP_DATA) != 0) &
            ((flags & CE_SEND_FLAG_SWAP_DISABLE) == 0));
        shadow_src_desc->gather    = ((flags & CE_SEND_FLAG_GATHER) != 0);
        shadow_src_desc->nbytes    = nbytes;

        *src_desc = *shadow_src_desc;

        src_ring->per_transfer_context[write_index] = per_transfer_context;

        /* Update Source Ring Write Index */
        write_index = CE_RING_IDX_INCR(nentries_mask, write_index);

        /* WORKAROUND */
        if (!shadow_src_desc->gather) {
            WAR_CE_SRC_RING_WRITE_IDX_SET(targid, ctrl_addr, write_index);
        }

        src_ring->write_index = write_index;
        status = A_OK;
    }
    A_TARGET_ACCESS_END(targid);

    return status;
}

int
CE_send(struct CE_handle *copyeng,
        void *per_transfer_context,
        CE_addr_t buffer,
        unsigned int nbytes,
        unsigned int transfer_id,
        unsigned int flags)
{
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    int status;


    adf_os_spin_lock_bh(&sc->target_lock);
    status = CE_send_nolock(copyeng, per_transfer_context, buffer, nbytes, transfer_id, flags);
    adf_os_spin_unlock_bh(&sc->target_lock);

    return status;
}

A_COMPILE_TIME_ASSERT(sendlist_sz_ok,
        (sizeof(struct CE_sendlist) >= sizeof(struct CE_sendlist_s *)));

unsigned int
CE_sendlist_sizeof(void)
{
   return sizeof(struct CE_sendlist);
}

void
CE_sendlist_init(struct CE_sendlist *sendlist)
{
    struct CE_sendlist_s *sl = (struct CE_sendlist_s *)sendlist;
    sl->num_items=0;
}

void
CE_sendlist_buf_add(struct CE_sendlist *sendlist,
                    CE_addr_t buffer,
                    unsigned int nbytes,
                    u_int32_t flags)
{
    struct CE_sendlist_s *sl = (struct CE_sendlist_s *)sendlist;
    unsigned int num_items = sl->num_items;
    struct CE_sendlist_item *item;

    A_ASSERT(num_items < CE_SENDLIST_ITEMS_MAX);

    item = &sl->item[num_items];
    item->send_type = CE_SIMPLE_BUFFER_TYPE;
    item->data = buffer;
    item->u.nbytes = nbytes;
    item->flags = flags;
    sl->num_items = num_items+1;
}

int
CE_sendlist_send(struct CE_handle *copyeng,
                 void *per_transfer_context,
                 struct CE_sendlist *sendlist,
                 unsigned int transfer_id)
{
    int status = -ENOMEM;
    struct CE_sendlist_s *sl = (struct CE_sendlist_s *)sendlist;
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    struct CE_ring_state *src_ring = CE_state->src_ring;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    unsigned int nentries_mask = src_ring->nentries_mask;
    unsigned int num_items = sl->num_items;
    unsigned int sw_index ;
    unsigned int write_index ;

    A_ASSERT((num_items > 0) && (num_items < src_ring->nentries));

    adf_os_spin_lock_bh(&sc->target_lock);
    sw_index = src_ring->sw_index;
    write_index = src_ring->write_index;

    if (CE_RING_DELTA(nentries_mask, write_index, sw_index-1) >= num_items) {
        struct CE_sendlist_item *item;
        int i;

        /* handle all but the last item uniformly */
        for (i = 0; i < num_items-1; i++) {
            item = &sl->item[i];
            /* TBDXXX: Support extensible sendlist_types? */
            A_ASSERT(item->send_type == CE_SIMPLE_BUFFER_TYPE);
            status = CE_send_nolock(copyeng, CE_SENDLIST_ITEM_CTXT, 
                                    (CE_addr_t)item->data, item->u.nbytes,
                                    transfer_id,
                                    item->flags | CE_SEND_FLAG_GATHER);
            A_ASSERT(status == A_OK);
        }
        /* provide valid context pointer for final item */
        item = &sl->item[i];
        /* TBDXXX: Support extensible sendlist_types? */
        A_ASSERT(item->send_type == CE_SIMPLE_BUFFER_TYPE);
        status = CE_send_nolock(copyeng, per_transfer_context, 
                                (CE_addr_t)item->data, item->u.nbytes,
                                transfer_id, item->flags);
        A_ASSERT(status == A_OK);
    } else {
        /*
         * Probably not worth the additional complexity to support
         * partial sends with continuation or notification.  We expect
         * to use large rings and small sendlists. If we can't handle
         * the entire request at once, punt it back to the caller.
         */
    }
    adf_os_spin_unlock_bh(&sc->target_lock);

    return status;
}

#if QCA_OL_11AC_FAST_PATH
void
ol_tx_stats_inc_ring_error(ol_txrx_pdev_handle pdev,
                             uint32_t num_ring_error);
/*
 * CE layer Tx buffer posting function
 * Assumption : Called with an array of MSDU's 
 *
 * Function:
 * For each msdu in the array
 * 1. Check no. of available entries
 * 2. Create src ring entries (allocated in consistent memory
 * 3. Write index to h/w
 * Returns:
 *  No. of packets that could be sent
 */
int
CE_send_fast(struct CE_handle *ce_tx_hdl,
             adf_nbuf_t *nbuf_arr,
             uint32_t num_msdus,
             uint32_t transfer_id)
{
    struct CE_state *ce_state = (struct CE_state *)ce_tx_hdl;
    struct ath_hif_pci_softc *sc = ce_state->sc;
    struct CE_ring_state *src_ring = ce_state->src_ring;
    u_int32_t ctrl_addr = ce_state->ctrl_addr;
    A_target_id_t targid = TARGID(sc);

    uint32_t nentries_mask = src_ring->nentries_mask;
    uint32_t sw_index, write_index, frag_len;

    struct CE_src_desc *src_desc_base = (struct CE_src_desc *)src_ring->base_addr_owner_space;
    struct CE_src_desc *src_desc;

    adf_nbuf_t msdu;
    int i;

    /*
     * This lock could be more fine-grained, one per CE,
     * TODO : Add this lock now.
     * That is the next step of optimization.
     */
    adf_os_spin_lock_bh(&sc->target_lock);
    sw_index = src_ring->sw_index;
    write_index = src_ring->write_index;

    for (i = 0; i < num_msdus; i++) {
        /*
         * Each msdu needs 2 elements of the ring
         * Check for space
         * TODO: If the ring is deep enough, this check can be an assertion
         */
        if (adf_os_unlikely(CE_RING_DELTA(nentries_mask, write_index, sw_index-1) < 2)) {
            ol_tx_stats_inc_ring_error(sc->scn->pdev_txrx_handle, 1);
            break;
        }

        src_desc = CE_SRC_RING_TO_DESC(src_desc_base, write_index);

        msdu = nbuf_arr[i];
        /*
         * First fill out the ring descriptor for the HTC HTT frame header
         * These are uncached writes. Should we use a local structure instead?
         */
        /* HTT/HTC header can be passed as a argument */
        src_desc->src_ptr   = adf_nbuf_get_frag_paddr_lo(msdu, 0);
        src_desc->meta_data = transfer_id; /* Could be prefilled for HTT CE */
        src_desc->nbytes    = adf_nbuf_get_frag_len(msdu, 0);

        /* HTC HTT header is a word stream, so byte swap if CE byte swap enabled */
        src_desc->byte_swap = ((ce_state->attr_flags & CE_ATTR_BYTE_SWAP_DATA) != 0);
        src_desc->gather    = 1; /* For the first one, it still does not need to write */

        /* By default we could initialize the transfer context to this value */
        src_ring->per_transfer_context[write_index] = CE_SENDLIST_ITEM_CTXT;

        write_index = CE_RING_IDX_INCR(nentries_mask, write_index);

        /* TODO:  This needs to be a loop to cover a real SG list from OS */

        src_desc = CE_SRC_RING_TO_DESC(src_desc_base, write_index);
        /* Now fill out the ring descriptor for the actual data packet */
        src_desc->src_ptr   =  adf_nbuf_get_frag_paddr_lo(msdu, 1);
        src_desc->meta_data = transfer_id; /* Could be prefilled for HTT CE */

        frag_len = adf_nbuf_get_frag_len(msdu, 1); /* get actual packet length */

        /* Extra check, probably not needed */
        src_desc->nbytes = frag_len > ce_state->download_len ?
                                ce_state->download_len : frag_len;
 

        /*  Data packet is a byte stream, so disable byte swap */
        src_desc->byte_swap = 0;
        src_desc->gather    = 0; /* For the last one, gather is not set */

        src_ring->per_transfer_context[write_index] = msdu;

        write_index = CE_RING_IDX_INCR(nentries_mask, write_index);
    }

    /* Write the final index to h/w one-shot */
    if (i) {
        src_ring->write_index = write_index;
        /* Don't call WAR_XXX from here
         * Just call XXX instead, that has the reqd. intel
         */
        WAR_CE_SRC_RING_WRITE_IDX_SET(targid, ctrl_addr, write_index);
    }

    adf_os_spin_unlock_bh(&sc->target_lock);

    /*
     * If all packets in the array are transmitted,
     * i = num_msdus
     * Temporarily add an ASSERT
     */
    ASSERT (i == num_msdus);
    return i;
}
#endif /* QCA_OL_11AC_FAST_PATH */

int
CE_recv_buf_enqueue(struct CE_handle *copyeng,
                    void *per_recv_context,
                    CE_addr_t buffer)
{
    int status;
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    struct CE_ring_state *dest_ring = CE_state->dest_ring;
    u_int32_t ctrl_addr = CE_state->ctrl_addr;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    A_target_id_t targid = TARGID(sc);
    unsigned int nentries_mask = dest_ring->nentries_mask;
    unsigned int write_index ;
    unsigned int sw_index;

#if QCA_OL_11AC_FAST_PATH
    unsigned int old_write_index;
#endif /* QCA_OL_11AC_FAST_PATH */
    
    adf_os_spin_lock_bh(&sc->target_lock);
    write_index = dest_ring->write_index;
    sw_index = dest_ring->sw_index;
#if QCA_OL_11AC_FAST_PATH
    old_write_index = write_index;
#endif  /* QCA_OL_11AC_FAST_PATH */

    A_TARGET_ACCESS_BEGIN(targid);
    if (CE_RING_DELTA(nentries_mask, write_index, sw_index-1) > 0) {
        struct CE_dest_desc *dest_ring_base = (struct CE_dest_desc *)dest_ring->base_addr_owner_space;
        struct CE_dest_desc *dest_desc = CE_DEST_RING_TO_DESC(dest_ring_base, write_index);

        /* Update destination descriptor */
        dest_desc->dest_ptr = buffer;
        dest_desc->info.nbytes = 0; /* NB: Enable CE_completed_recv_next_nolock to
								protect against race between DRRI update and
								desc update */
		
        dest_ring->per_transfer_context[write_index] = per_recv_context;

        /* Update Destination Ring Write Index */
        write_index = CE_RING_IDX_INCR(nentries_mask, write_index);
        CE_DEST_RING_WRITE_IDX_SET(targid, ctrl_addr, write_index);
        dest_ring->write_index = write_index;
        status = A_OK;
    } else {
        status = A_ERROR;
    }
#if QCA_OL_11AC_FAST_PATH
    if (adf_os_unlikely(!sc->hif_started) && (CE_state->id == CE_HTT_MSG_CE)) {
        struct CE_dest_desc *dest_ring_base = (struct CE_dest_desc *)dest_ring->base_addr_owner_space;
        struct CE_dest_desc *dest_desc = CE_DEST_RING_TO_DESC(dest_ring_base, (write_index - 1));

        /*
         * NOTE: This check holds good only if CE_attr->dest_nentries has been
         * set to a power of 2 start with, else this will fail. However the normal
         * case is to set attr->dest_nentries to a power of 2 always.
         */
        if (old_write_index == (dest_ring->nentries - 1)) {
            adf_os_print("%s %d Populate last entry %d for CE %d\n", __func__, __LINE__,
                dest_ring->nentries, CE_state->id);

            dest_desc = CE_DEST_RING_TO_DESC(dest_ring_base, old_write_index);
            dest_desc->dest_ptr = buffer;
            dest_desc->info.nbytes = 0;
            dest_ring->per_transfer_context[old_write_index] = per_recv_context;

            adf_os_print("%s %d CE 5 wi %d dest_ptr 0x%x nbytes %d recv_ctxt 0x%p\n",
                __func__, __LINE__, old_write_index, dest_desc->dest_ptr,
                dest_desc->info.nbytes, dest_ring->per_transfer_context[old_write_index]);
        }
    }
#endif  /* QCA_OL_11AC_FAST_PATH */
    A_TARGET_ACCESS_END(targid);
    adf_os_spin_unlock_bh(&sc->target_lock);

    return status;
}

void
CE_send_watermarks_set(struct CE_handle *copyeng,
                       unsigned int low_alert_nentries,
                       unsigned int high_alert_nentries)
{
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    u_int32_t ctrl_addr = CE_state->ctrl_addr;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    A_target_id_t targid = TARGID(sc);

    adf_os_spin_lock(&sc->target_lock);
    CE_SRC_RING_LOWMARK_SET(targid, ctrl_addr, low_alert_nentries);
    CE_SRC_RING_HIGHMARK_SET(targid, ctrl_addr, high_alert_nentries);
    adf_os_spin_unlock(&sc->target_lock);
}

void
CE_recv_watermarks_set(struct CE_handle *copyeng,
                       unsigned int low_alert_nentries,
                       unsigned int high_alert_nentries)
{
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    u_int32_t ctrl_addr = CE_state->ctrl_addr;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    A_target_id_t targid = TARGID(sc);

    adf_os_spin_lock(&sc->target_lock);
    CE_DEST_RING_LOWMARK_SET(targid, ctrl_addr, low_alert_nentries);
    CE_DEST_RING_HIGHMARK_SET(targid, ctrl_addr, high_alert_nentries);
    adf_os_spin_unlock(&sc->target_lock);
}

unsigned int
CE_send_entries_avail(struct CE_handle *copyeng)
{
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    struct CE_ring_state *src_ring = CE_state->src_ring;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    unsigned int nentries_mask = src_ring->nentries_mask;
    unsigned int sw_index;
    unsigned int write_index;

    adf_os_spin_lock(&sc->target_lock);
    sw_index = src_ring->sw_index;
    write_index = src_ring->write_index;
    adf_os_spin_unlock(&sc->target_lock);

    return CE_RING_DELTA(nentries_mask, write_index, sw_index-1);
}

unsigned int
CE_recv_entries_avail(struct CE_handle *copyeng)
{
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    struct CE_ring_state *dest_ring = CE_state->dest_ring;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    unsigned int nentries_mask = dest_ring->nentries_mask;
    unsigned int sw_index;
    unsigned int write_index;

    adf_os_spin_lock(&sc->target_lock);
    sw_index = dest_ring->sw_index;
    write_index = dest_ring->write_index;
    adf_os_spin_unlock(&sc->target_lock);

    return CE_RING_DELTA(nentries_mask, write_index, sw_index-1);
}

/*
 * Guts of CE_send_entries_done.
 * The caller takes responsibility for any necessary locking.
 */
unsigned int
CE_send_entries_done_nolock(struct ath_hif_pci_softc *sc, struct CE_state *CE_state)
{
    struct CE_ring_state *src_ring = CE_state->src_ring;
    u_int32_t ctrl_addr = CE_state->ctrl_addr;
    A_target_id_t targid = TARGID(sc);
    unsigned int nentries_mask = src_ring->nentries_mask;
    unsigned int sw_index;
    unsigned int read_index;

    sw_index = src_ring->sw_index;
    read_index = CE_SRC_RING_READ_IDX_GET(targid, ctrl_addr);

    return CE_RING_DELTA(nentries_mask, sw_index, read_index);
}

unsigned int
CE_send_entries_done(struct CE_handle *copyeng)
{
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    unsigned int nentries;

    adf_os_spin_lock(&sc->target_lock);
    nentries = CE_send_entries_done_nolock(sc, CE_state);
    adf_os_spin_unlock(&sc->target_lock);

    return nentries;
}

/*
 * Guts of CE_recv_entries_done.
 * The caller takes responsibility for any necessary locking.
 */
unsigned int
CE_recv_entries_done_nolock(struct ath_hif_pci_softc *sc, struct CE_state *CE_state)
{
    struct CE_ring_state *dest_ring = CE_state->dest_ring;
    u_int32_t ctrl_addr = CE_state->ctrl_addr;
    A_target_id_t targid = TARGID(sc);
    unsigned int nentries_mask = dest_ring->nentries_mask;
    unsigned int sw_index;
    unsigned int read_index;

    sw_index = dest_ring->sw_index;
    read_index = CE_DEST_RING_READ_IDX_GET(targid, ctrl_addr);

    return CE_RING_DELTA(nentries_mask, sw_index, read_index);
}

unsigned int
CE_recv_entries_done(struct CE_handle *copyeng)
{
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    unsigned int nentries;

    adf_os_spin_lock(&sc->target_lock);
    nentries = CE_recv_entries_done_nolock(sc, CE_state);
    adf_os_spin_unlock(&sc->target_lock);

    return nentries;
}

/* Debug support */
void *ce_debug_cmplrn_context; /* completed recv next context */
void *ce_debug_cnclsn_context; /* cancel send next context */
void *ce_debug_rvkrn_context;  /* revoke receive next context */
void *ce_debug_cmplsn_context; /* completed send next context */


/*
 * Guts of CE_completed_recv_next.
 * The caller takes responsibility for any necessary locking.
 */
int
CE_completed_recv_next_nolock(struct CE_state *CE_state,
                              void **per_CE_contextp,
                              void **per_transfer_contextp,
                              CE_addr_t *bufferp,
                              unsigned int *nbytesp,
                              unsigned int *transfer_idp,
                              unsigned int *flagsp)
{
    int status;
    struct CE_ring_state *dest_ring = CE_state->dest_ring;
    unsigned int nentries_mask = dest_ring->nentries_mask;
    unsigned int sw_index = dest_ring->sw_index;

    struct CE_dest_desc *dest_ring_base = (struct CE_dest_desc *)dest_ring->base_addr_owner_space;
    struct CE_dest_desc *dest_desc = CE_DEST_RING_TO_DESC(dest_ring_base, sw_index);
    int nbytes;
    struct dest_desc_info dest_desc_info;

    /* 
     * By copying the dest_desc_info element to local memory, we could
     * avoid extra memory read from non-cachable memory.          
     */
    dest_desc_info = dest_desc->info;
    nbytes = dest_desc_info.nbytes;
    if (nbytes == 0) {
        /*
         * This closes a relatively unusual race where the Host
         * sees the updated DRRI before the update to the
         * corresponding descriptor has completed. We treat this
         * as a descriptor that is not yet done.
         */
        status = A_ERROR;
        goto done;
    }

    dest_desc->info.nbytes = 0;
		
    /* Return data from completed destination descriptor */
    *bufferp      = (CE_addr_t)(dest_desc->dest_ptr);
    *nbytesp      = nbytes;
    *transfer_idp = dest_desc_info.meta_data;
    *flagsp       = (dest_desc_info.byte_swap) ?  CE_RECV_FLAG_SWAPPED : 0;

    if (per_CE_contextp) {
        *per_CE_contextp = CE_state->recv_context;
    }

    ce_debug_cmplrn_context = dest_ring->per_transfer_context[sw_index];
    if (per_transfer_contextp) {
        *per_transfer_contextp = ce_debug_cmplrn_context;
    }
    dest_ring->per_transfer_context[sw_index] = 0; /* sanity */

    /* Update sw_index */
    sw_index = CE_RING_IDX_INCR(nentries_mask, sw_index);
    dest_ring->sw_index = sw_index;
    status = A_OK;

done:
    return status;
}

int
CE_completed_recv_next(struct CE_handle *copyeng,
                       void **per_CE_contextp,
                       void **per_transfer_contextp,
                       CE_addr_t *bufferp,
                       unsigned int *nbytesp,
                       unsigned int *transfer_idp,
                       unsigned int *flagsp)
{
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    int status;


    adf_os_spin_lock(&sc->target_lock);
    status = CE_completed_recv_next_nolock(CE_state, per_CE_contextp, per_transfer_contextp,
                                               bufferp, nbytesp, transfer_idp, flagsp);
    adf_os_spin_unlock(&sc->target_lock);

    return status;
}

/* NB: Modeled after CE_completed_recv_next_nolock */
A_STATUS
CE_revoke_recv_next(struct CE_handle *copyeng,
                    void **per_CE_contextp,
                    void **per_transfer_contextp,
                    CE_addr_t *bufferp)
{
    struct CE_state *CE_state;
    struct CE_ring_state *dest_ring;
    unsigned int nentries_mask;
    unsigned int sw_index;
    unsigned int write_index;
    A_STATUS status;
    struct ath_hif_pci_softc *sc;

    CE_state = (struct CE_state *)copyeng;
    dest_ring = CE_state->dest_ring;
    if (!dest_ring) {
        return A_ERROR;
    }

    sc = CE_state->sc;
    adf_os_spin_lock(&sc->target_lock);
    nentries_mask = dest_ring->nentries_mask;
    sw_index = dest_ring->sw_index;
    write_index = dest_ring->write_index;
    if (write_index != sw_index) {
        struct CE_dest_desc *dest_ring_base = (struct CE_dest_desc *)dest_ring->base_addr_owner_space;
        struct CE_dest_desc *dest_desc = CE_DEST_RING_TO_DESC(dest_ring_base, sw_index);

        /* Return data from completed destination descriptor */
        *bufferp     = (CE_addr_t)(dest_desc->dest_ptr);

        if (per_CE_contextp) {
            *per_CE_contextp = CE_state->recv_context;
        }

        ce_debug_rvkrn_context = dest_ring->per_transfer_context[sw_index];
        if (per_transfer_contextp) {
            *per_transfer_contextp = ce_debug_rvkrn_context;
        }
        dest_ring->per_transfer_context[sw_index] = 0; /* sanity */

        /* Update sw_index */
        sw_index = CE_RING_IDX_INCR(nentries_mask, sw_index);
        dest_ring->sw_index = sw_index;
        status = A_OK;
    } else {
        status = A_ERROR;
    }
    adf_os_spin_unlock(&sc->target_lock);

    return status;
}

/*
 * Guts of CE_completed_send_next.
 * The caller takes responsibility for any necessary locking.
 */
int
CE_completed_send_next_nolock(struct CE_state *CE_state,
                              void **per_CE_contextp,
                              void **per_transfer_contextp,
                              CE_addr_t *bufferp,
                              unsigned int *nbytesp,
                              unsigned int *transfer_idp)
{
    int status = A_ERROR;
    struct CE_ring_state *src_ring = CE_state->src_ring;
    u_int32_t ctrl_addr = CE_state->ctrl_addr;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    A_target_id_t targid = TARGID(sc);
    unsigned int nentries_mask = src_ring->nentries_mask;
    unsigned int sw_index = src_ring->sw_index;
    unsigned int read_index;


    if (src_ring->hw_index == sw_index) {
        /*
         * The SW completion index has caught up with the cached
         * version of the HW completion index.
         * Update the cached HW completion index to see whether
         * the SW has really caught up to the HW, or if the cached
         * value of the HW index has become stale.
         */
        A_TARGET_ACCESS_BEGIN(targid);
        src_ring->hw_index = CE_SRC_RING_READ_IDX_GET(targid, ctrl_addr);
        A_TARGET_ACCESS_END(targid);
    }
    read_index = src_ring->hw_index;

    if ((read_index != sw_index) && (read_index != 0xffffffff)) {
        struct CE_src_desc *shadow_base = (struct CE_src_desc *)src_ring->shadow_base;
        struct CE_src_desc *shadow_src_desc = CE_SRC_RING_TO_DESC(shadow_base, sw_index);

        /* Return data from completed source descriptor */
        *bufferp      = (CE_addr_t)(shadow_src_desc->src_ptr);
        *nbytesp      = shadow_src_desc->nbytes;
        *transfer_idp = shadow_src_desc->meta_data;
        
        if (per_CE_contextp) {
            *per_CE_contextp = CE_state->send_context;
        }

        ce_debug_cmplsn_context = src_ring->per_transfer_context[sw_index];
        if (per_transfer_contextp) {
            *per_transfer_contextp = ce_debug_cmplsn_context;
        }
        src_ring->per_transfer_context[sw_index] = 0; /* sanity */

        /* Update sw_index */
        sw_index = CE_RING_IDX_INCR(nentries_mask, sw_index);
        src_ring->sw_index = sw_index;
        status = A_OK;
    }

    return status;
}

/* NB: Modeled after CE_completed_send_next */
A_STATUS
CE_cancel_send_next(struct CE_handle *copyeng,
                    void **per_CE_contextp,
                    void **per_transfer_contextp,
                    CE_addr_t *bufferp,
                    unsigned int *nbytesp,
                    unsigned int *transfer_idp)
{
    struct CE_state *CE_state;
    struct CE_ring_state *src_ring;
    unsigned int nentries_mask;
    unsigned int sw_index;
    unsigned int write_index;
    A_STATUS status;
    struct ath_hif_pci_softc *sc;

    CE_state = (struct CE_state *)copyeng;
    src_ring = CE_state->src_ring;
    if (!src_ring) {
        return A_ERROR;
    }

    sc = CE_state->sc;
    adf_os_spin_lock(&sc->target_lock);
    nentries_mask = src_ring->nentries_mask;
    sw_index = src_ring->sw_index;
    write_index = src_ring->write_index;

    if (write_index != sw_index) {
        struct CE_src_desc *src_ring_base = (struct CE_src_desc *)src_ring->base_addr_owner_space;
        struct CE_src_desc *src_desc = CE_SRC_RING_TO_DESC(src_ring_base, sw_index);

        /* Return data from completed source descriptor */
        *bufferp      = (CE_addr_t)(src_desc->src_ptr);
        *nbytesp      = src_desc->nbytes;
        *transfer_idp = src_desc->meta_data;

        if (per_CE_contextp) {
            *per_CE_contextp = CE_state->send_context;
        }

        ce_debug_cnclsn_context = src_ring->per_transfer_context[sw_index];
        if (per_transfer_contextp) {
            *per_transfer_contextp = ce_debug_cnclsn_context;
        }
        src_ring->per_transfer_context[sw_index] = 0; /* sanity */

        /* Update sw_index */
        sw_index = CE_RING_IDX_INCR(nentries_mask, sw_index);
        src_ring->sw_index = sw_index;
        status = A_OK;
    } else {
        status = A_ERROR;
    }
    adf_os_spin_unlock(&sc->target_lock);

    return status;
}

/* Shift bits to convert IS_*_RING_*_WATERMARK_MASK to CE_WM_FLAG_*_* */
#define CE_WM_SHFT 1
A_COMPILE_TIME_ASSERT(watermarks_ok,
                   (((HOST_IS_SRC_RING_HIGH_WATERMARK_MASK >> CE_WM_SHFT) == CE_WM_FLAG_SEND_HIGH) &&
                    ((HOST_IS_SRC_RING_LOW_WATERMARK_MASK >>  CE_WM_SHFT) == CE_WM_FLAG_SEND_LOW)  &&
                    ((HOST_IS_DST_RING_HIGH_WATERMARK_MASK >> CE_WM_SHFT) == CE_WM_FLAG_RECV_HIGH) &&
                    ((HOST_IS_DST_RING_LOW_WATERMARK_MASK >>  CE_WM_SHFT) == CE_WM_FLAG_RECV_LOW)));


int
CE_completed_send_next(struct CE_handle *copyeng,
                       void **per_CE_contextp,
                       void **per_transfer_contextp,
                       CE_addr_t *bufferp,
                       unsigned int *nbytesp,
                       unsigned int *transfer_idp)
{
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    int status;


    adf_os_spin_lock(&sc->target_lock);
    status = CE_completed_send_next_nolock(CE_state, per_CE_contextp, per_transfer_contextp,
                                               bufferp, nbytesp, transfer_idp);
    adf_os_spin_unlock(&sc->target_lock);

    return status;
}


#if ATH_11AC_TXCOMPACT
/* CE engine descriptor reap
   Similar to CE_per_engine_service , Only difference is CE_per_engine_service
   does recieve and reaping of completed descriptor ,
   This function only handles reaping of Tx complete descriptor.
   The Function is called from threshold reap  poll routine HIFSendCompleteCheck
   So should not countain recieve functionality within it . 
 */

void
CE_per_engine_servicereap(struct ath_hif_pci_softc *sc, unsigned int CE_id)
{
    struct CE_state *CE_state = sc->CE_id_to_state[CE_id];
    A_target_id_t targid = TARGID(sc);
    void *CE_context;
    void *transfer_context;
    CE_addr_t buf;
    unsigned int nbytes;
    unsigned int id;

    A_TARGET_ACCESS_BEGIN(targid);

    adf_os_spin_lock(&sc->target_lock);


    if (CE_state->send_cb) {
       {
            /* Pop completed send buffers and call the registered send callback for each */
            while (CE_completed_send_next_nolock(CE_state, &CE_context, &transfer_context,
                        &buf, &nbytes, &id) == A_OK)
            {
#if QCA_OL_11AC_FAST_PATH
                adf_os_spin_unlock(&sc->target_lock);
                CE_state->send_cb((struct CE_handle *)CE_state, CE_context, transfer_context, buf, nbytes, id);
                adf_os_spin_lock(&sc->target_lock);
#else
                if(CE_id != CE_HTT_H2T_MSG){
                    adf_os_spin_unlock(&sc->target_lock);
                    CE_state->send_cb((struct CE_handle *)CE_state, CE_context, transfer_context, buf, nbytes, id);
                    adf_os_spin_lock(&sc->target_lock);
                }else{
                     struct HIF_CE_pipe_info *pipe_info = (struct HIF_CE_pipe_info *)CE_context;

                     adf_os_spin_lock(&pipe_info->completion_freeq_lock);
                     pipe_info->num_sends_allowed++;
                     adf_os_spin_unlock(&pipe_info->completion_freeq_lock);
                }
#endif
            }
        }
    }

    adf_os_spin_unlock(&sc->target_lock);
    A_TARGET_ACCESS_END(targid);
}

#endif /*ATH_11AC_TXCOMPACT*/
/*
 * Guts of interrupt handler for per-engine interrupts on a particular CE.
 *
 * Invokes registered callbacks for recv_complete,
 * send_complete, and watermarks.
 */
void
CE_per_engine_service(struct ath_hif_pci_softc *sc, unsigned int CE_id)
{
    struct CE_state *CE_state = sc->CE_id_to_state[CE_id];
    u_int32_t ctrl_addr = CE_state->ctrl_addr;
    A_target_id_t targid = TARGID(sc);
    void *CE_context;
    void *transfer_context;
    CE_addr_t buf;
    unsigned int nbytes;
    unsigned int id;
    unsigned int flags;
    u_int32_t CE_int_status;

    A_TARGET_ACCESS_BEGIN(targid);

    adf_os_spin_lock(&sc->target_lock);

more_completions:
    /*
     * Clear the copy-complete interrupts that will be handled here.
     */
    CE_ENGINE_INT_STATUS_CLEAR(targid, ctrl_addr, HOST_IS_COPY_COMPLETE_MASK);
    if (CE_state->recv_cb) {
        /* Clear force_break flag and re-initialize receive_count to 0 */
        sc->receive_count = 0;
        
        /* Pop completed recv buffers and call the registered recv callback for each */
        while (CE_completed_recv_next_nolock(CE_state, &CE_context, &transfer_context,
                    &buf, &nbytes, &id, &flags) == A_OK)
        {
                adf_os_spin_unlock(&sc->target_lock);
                CE_state->recv_cb((struct CE_handle *)CE_state, CE_context, transfer_context,
                                    buf, nbytes, id, flags);
                
                /*
                 * EV #112693 - [Peregrine][ES1][WB342][Win8x86][Performance] BSoD_0x133 occurred in VHT80 UDP_DL      
                 * Break out DPC by force if number of loops in HIF_PCI_CE_recv_data reaches MAX_NUM_OF_RECEIVES to avoid spending too long time in DPC for each interrupt handling.
                 * Schedule another DPC to avoid data loss if we had taken force-break action before
                 * Apply to Windows OS only currently, Linux/MAC os can expand to their platform if necessary
                 */

                /* Break the receive processes by force if force_break set up */
                if (adf_os_unlikely(sc->force_break))
                {
                    A_TARGET_ACCESS_END(targid);
                    return;
                }
                adf_os_spin_lock(&sc->target_lock);
        }
    }

    /*
     * Attention: We may experience potential infinite loop for below While Loop during Sending Stress test
     * Resolve the same way as Receive Case (Refer to EV #112693)
     */
    
    if (CE_state->send_cb) {
        /* Pop completed send buffers and call the registered send callback for each */

#if ATH_11AC_TXCOMPACT
        while (CE_completed_send_next_nolock(CE_state, &CE_context, &transfer_context,
                    &buf, &nbytes, &id) == A_OK){

            if(CE_id != CE_HTT_H2T_MSG){
                adf_os_spin_unlock(&sc->target_lock);
                CE_state->send_cb((struct CE_handle *)CE_state, CE_context, transfer_context, buf, nbytes, id);
                adf_os_spin_lock(&sc->target_lock);
            }else{
                struct HIF_CE_pipe_info *pipe_info = (struct HIF_CE_pipe_info *)CE_context;

                adf_os_spin_lock(&pipe_info->completion_freeq_lock);
                pipe_info->num_sends_allowed++;
                adf_os_spin_unlock(&pipe_info->completion_freeq_lock);
            }
        }
#else  /*ATH_11AC_TXCOMPACT*/ 
        while (CE_completed_send_next_nolock(CE_state, &CE_context, &transfer_context,
                    &buf, &nbytes, &id) == A_OK){
            adf_os_spin_unlock(&sc->target_lock);
            CE_state->send_cb((struct CE_handle *)CE_state, CE_context, transfer_context, buf, nbytes, id);
            adf_os_spin_lock(&sc->target_lock);
        }
#endif /*ATH_11AC_TXCOMPACT*/
    }

    if (CE_state->misc_cbs) {
more_watermarks:
        CE_int_status = CE_ENGINE_INT_STATUS_GET(targid, ctrl_addr);
        if (CE_int_status & CE_WATERMARK_MASK) {
            if (CE_state->watermark_cb) {

                adf_os_spin_unlock(&sc->target_lock);
                /* Convert HW IS bits to software flags */
                flags = (CE_int_status & CE_WATERMARK_MASK) >> CE_WM_SHFT;

                CE_state->watermark_cb((struct CE_handle *)CE_state, CE_state->wm_context, flags);
                adf_os_spin_lock(&sc->target_lock);
            }
        }

        /*
         * Clear the misc interrupts (watermark) that were handled above,
         * and that will be checked again below.
         * Clear and check for copy-complete interrupts again, just in case
         * more copy completions happened while the misc interrupts were being
         * handled.
         */
        CE_ENGINE_INT_STATUS_CLEAR(targid, ctrl_addr, CE_WATERMARK_MASK | HOST_IS_COPY_COMPLETE_MASK);

        /*
         * Now that per-engine interrupts are cleared, verify that
         * we didn't miss anything.  If we did, go back and handle it.
         * If there's nothing to do and interrupts are cleared, we're done.
         */
        if (CE_state->recv_cb && CE_recv_entries_done_nolock(sc, CE_state)) {
            goto more_completions;
        }

        if (CE_state->send_cb && CE_send_entries_done_nolock(sc, CE_state)) {
            goto more_completions;
        }

        if (CE_int_status & CE_WATERMARK_MASK) {
            if (CE_state->watermark_cb) {
                goto more_watermarks;
            }
        }
    } else {
        /*
         * Misc CE interrupts are not being handled, but still need
         * to be cleared.
         */
        CE_ENGINE_INT_STATUS_CLEAR(targid, ctrl_addr, CE_WATERMARK_MASK);
    }

    adf_os_spin_unlock(&sc->target_lock);
    A_TARGET_ACCESS_END(targid);
}

/*
 * Handler for per-engine interrupts on ALL active CEs.
 * This is used in cases where the system is sharing a
 * single interrput for all CEs
 */

void
CE_per_engine_service_any(int irq, void *arg)
{
    struct ath_hif_pci_softc *sc = arg;
    A_target_id_t targid = TARGID(sc);
    int CE_id;
    A_UINT32 intr_summary;
	
    A_TARGET_ACCESS_BEGIN(targid);
    intr_summary = CE_INTERRUPT_SUMMARY(targid);
	
    for (CE_id=0; intr_summary && (CE_id < sc->ce_count); CE_id++) {
        if (intr_summary & (1<<CE_id)) {
            intr_summary &= ~(1<<CE_id);
        } else {
            continue; /* no intr pending on this CE */
        }
		
        CE_per_engine_service(sc, CE_id);
    }

    A_TARGET_ACCESS_END(targid);
}

#if QCA_OL_11AC_FAST_PATH

bool
CE_htt_ce_cmpl_done(struct CE_handle *ce_hdl)
{
    struct CE_state *ce_state = (struct CE_state *)ce_hdl;
    struct CE_ring_state *src_ring = ce_state->src_ring;
    struct ath_hif_pci_softc *sc = ce_state->sc;

    bool ret;

    adf_os_spin_lock_bh(&sc->target_lock);

    ret = (src_ring->sw_index == src_ring->write_index);

    adf_os_spin_unlock_bh(&sc->target_lock);

    return ret;
}

/*
 * API to reap off the HTT source ring when HTT completion happens:
 * 1) For data packets, on packet completion
 * 2) For other other HTT messages, through HTT t->h messages
 */
static void
CE_htt_ce_cmpl(struct CE_state *ce_state, uint32_t num_htt_cmpls)
{
    struct CE_ring_state *src_ring = ce_state->src_ring;
    uint32_t nentries_mask = src_ring->nentries_mask;
    struct ath_hif_pci_softc *sc = ce_state->sc;

    ASSERT (num_htt_cmpls);

    adf_os_spin_lock(&sc->target_lock);

    /*
     * This locks the index manipulation of this CE with those done
     * in ce_send_fast(). Since this is only on a given CE, we use
     * a finer grained per CE lock.
     * src_ring->ce_ring_lock? 
     */

    /*
     * Advance the s/w index:
     * This effectively simulates completing the CE ring descriptors 
     */

    src_ring->sw_index = 
        CE_RING_IDX_ADD(nentries_mask, src_ring->sw_index, num_htt_cmpls);
    adf_os_spin_unlock(&sc->target_lock);
}



extern void
htt_t2h_msg_handler_fast(void *htt_pdev, adf_nbuf_t *nbuf_cmpl_arr,
                         uint32_t num_cmpls, uint32_t *num_tx_cmpls);
#define MSG_FLUSH_NUM 32
/*
 * CE handler routine to service HTT message CE (= 1)
 * Function:
 * 1) Go through the HTT CE ring, and find the completions
 * 2) For valid completions retrieve context (nbuf) for per_transfer_context[]
 * 3) Unmap buffer & accumulate in an array.
 * 4) Call HTT message handler when array is full or when exiting the handler
 */
static inline void
CE_per_engine_service_fast(struct ath_hif_pci_softc *sc, uint32_t ce_id)
{
    struct CE_state *ce_state = sc->CE_id_to_state[ce_id];
    struct CE_state *CE_tx_cmpl_state = sc->CE_id_to_state[CE_HTT_TX_CE];
    struct CE_ring_state *dest_ring = ce_state->dest_ring;
    struct CE_dest_desc *dest_ring_base = (struct CE_dest_desc *)dest_ring->base_addr_owner_space;

    uint32_t nentries_mask = dest_ring->nentries_mask;
    uint32_t sw_index = dest_ring->sw_index;
    uint32_t write_index;
    uint32_t paddr_lo;
    uint32_t nbytes, nbuf_cmpl_idx = 0;
    uint32_t num_htt_cmpls;
    void **transfer_contexts;
    adf_nbuf_t *nbuf_cmpl_arr = NULL;
    adf_nbuf_t nbuf;

    struct CE_dest_desc *dest_desc = CE_DEST_RING_TO_DESC(dest_ring_base, sw_index);
    struct dest_desc_info *dest_desc_info = &dest_desc->info;

    u_int32_t ctrl_addr = ce_state->ctrl_addr;
    A_target_id_t targid = TARGID(sc);

    struct htt_pdev_t *htt_pdev = sc->scn->htt_pdev;

    CE_ENGINE_INT_STATUS_CLEAR(targid, ctrl_addr, HOST_IS_COPY_COMPLETE_MASK);

    nbuf_cmpl_arr = &htt_pdev->nbuf_cmpl_arr[0];
    transfer_contexts = dest_ring->per_transfer_context;

    for (;;) {

        dest_desc = CE_DEST_RING_TO_DESC(dest_ring_base, sw_index);

        dest_desc_info = &dest_desc->info;

        /*
         * The following 2 reads are from non-cached memory
         * TODO : Make this cacheable memory, since we will read
         *        ring in a loop in a given call. That way, we
         *        can avoid quite a few memory reads.
         */
        nbytes = dest_desc_info->nbytes;

        /* If completion is invalid, break */
        if (adf_os_unlikely(nbytes == 0)) {
            break;
        }

        /*
         * CAREFUL : Uncached write, but still less expensive,
         * since most modern caches use "write-combining" to flush
         * multiple cache-writes all at once.
         */
        dest_desc_info->nbytes = 0;

        /*
         * Build the nbuf list from valid completions
         */
        nbuf = dest_ring->per_transfer_context[sw_index];

        paddr_lo = adf_nbuf_get_frag_paddr_lo(nbuf, 0);
        OS_SYNC_SINGLE(sc->scn->sc_osdev, paddr_lo, nbytes, BUS_DMA_FROMDEVICE, &paddr_lo);

        /* Set the length, & put accumulate in the array */
        adf_nbuf_put_tail(nbuf, nbytes);
        nbuf_cmpl_arr[nbuf_cmpl_idx++] = nbuf;

        /*
         * If array is full call the HTT message handler directly here
         * Also post buffers for this CE
         * TODO : Optimize this posting, these postings are not required.
         * Need to incorporate Anand's idea of reusing the buffers
         */
        if (adf_os_unlikely(nbuf_cmpl_idx == MSG_FLUSH_NUM)) {

            htt_t2h_msg_handler_fast(htt_pdev, nbuf_cmpl_arr, MSG_FLUSH_NUM, &num_htt_cmpls);

            /* Update Destination Ring Write Index */
            write_index = dest_ring->write_index;
            write_index = CE_RING_IDX_ADD(nentries_mask, write_index, MSG_FLUSH_NUM);
            CE_DEST_RING_WRITE_IDX_SET(targid, ctrl_addr, write_index);
            dest_ring->write_index = write_index;

            CE_htt_ce_cmpl(CE_tx_cmpl_state, num_htt_cmpls);

            nbuf_cmpl_idx = 0;
        }

        /*
         * No lock is needed here, since this is the only thread
         * that accesses the sw_index
         */
        sw_index = CE_RING_IDX_INCR(nentries_mask, sw_index);
    }

    /*
     * If there are not enough completions to fill the array,
     * just call the HTT message handler here
     */
    if (adf_os_likely(nbuf_cmpl_idx)) {

        htt_t2h_msg_handler_fast(htt_pdev, nbuf_cmpl_arr, nbuf_cmpl_idx, &num_htt_cmpls);

        /* Update Destination Ring Write Index */
        write_index = dest_ring->write_index;
        write_index = CE_RING_IDX_ADD(nentries_mask, write_index, nbuf_cmpl_idx);
        CE_DEST_RING_WRITE_IDX_SET(targid, ctrl_addr, write_index);
        dest_ring->write_index = write_index;

        CE_htt_ce_cmpl(CE_tx_cmpl_state, num_htt_cmpls);

        nbuf_cmpl_idx = 0;
    }
    dest_ring->sw_index = sw_index;
}


/*
 * This is only used in the cleanup. This API is only required,
 * since we don't want to get rid of the "static inline" nature
 * CE_per_engine_service_fast(), which is used in the fast data
 * path. This function is accessible as an exported API from other
 * modules.
 */
void
CE_per_engine_service_one(struct ath_hif_pci_softc *sc, uint32_t ce_id)
{
    CE_per_engine_service_fast(sc, ce_id);
}

#define MAX_RX_CES      8
#define MAX_TX_CES      8
#define MAX_MS_CES      8

int rx_ce_list_fast[MAX_RX_CES] = {1, 2, -1};

int tx_ce_list[MAX_TX_CES] = {3, -1};
int misc_ce_list[MAX_MS_CES] = {0, -1};

void
CE_per_engine_service_each(int irq, struct ath_hif_pci_softc *sc)
{
    A_target_id_t targid = TARGID(sc);
    void **transfer_contexts;
    void *tx_transfer_ctx;
    struct CE_state *CE_state;
    int CE_id;
    struct CE_dest_desc *dest_ring_base;
    struct CE_dest_desc *dest_desc;
    int nbytes;
    CE_addr_t buf;
    unsigned int sw_index;
    unsigned int nentries_mask;
    unsigned int id;
    int i = 0;

    struct HIF_CE_pipe_info *pipe_info;
    struct HIF_CE_state *hif_state;
    struct HIF_CE_completion_state *compl_state;
    struct HIF_CE_completion_state *compl_queue_head, *compl_queue_tail; /* local queue */
    uint32_t num_cmpl_states = 0;

    adf_os_device_t adf_dev = sc->scn->adf_dev;

    /* For CE = 5 */
    CE_per_engine_service_fast(sc, CE_HTT_MSG_CE);

    /* Process Rx CE's */
    for (i = 0; i < MAX_RX_CES; i++) {
        if (rx_ce_list_fast[i] == -1) {
            break;
        }
        CE_id = rx_ce_list_fast[i];

        CE_state = sc->CE_id_to_state[CE_id];
        pipe_info = CE_state->recv_context;
        hif_state = pipe_info->HIF_CE_state;
        compl_queue_head = compl_queue_tail = NULL;

        CE_ENGINE_INT_STATUS_CLEAR(targid, CE_state->ctrl_addr, HOST_IS_COPY_COMPLETE_MASK);
        /*
         * Grab the SW index and ring base. Get the mask to be used in the
         * loop for index increment.
         */
        sw_index = (CE_state->dest_ring->sw_index);
        dest_ring_base = (struct CE_dest_desc *)(CE_state->dest_ring->base_addr_owner_space);
        nentries_mask = (CE_state->dest_ring->nentries_mask);
        transfer_contexts = (CE_state->dest_ring->per_transfer_context);
        while (1) {
            dest_desc = CE_DEST_RING_TO_DESC(dest_ring_base, sw_index);
            nbytes = dest_desc->info.nbytes;
            if (nbytes != 0) {
                adf_nbuf_t netbuf;
                netbuf = transfer_contexts[sw_index];
                dest_desc->info.nbytes = 0;

                sw_index = CE_RING_IDX_INCR(nentries_mask, sw_index);
                CE_state->dest_ring->sw_index = sw_index;
                {
                    adf_os_spin_lock(&pipe_info->completion_freeq_lock);
                    compl_state = pipe_info->completion_freeq_head;
                    ASSERT(compl_state != NULL);
                    pipe_info->completion_freeq_head = compl_state->next;
                    adf_os_spin_unlock(&pipe_info->completion_freeq_lock);
                    compl_state->next = NULL;
                    /* Enqueue at end of local queue */
                    if (compl_queue_tail) {
                        compl_queue_tail->next = compl_state;
                        compl_queue_tail = compl_state;
                    } else {
                        compl_queue_head = compl_queue_tail = compl_state;
                    }

                    compl_state->send_or_recv = HIF_CE_COMPLETE_RECV;
                    compl_state->copyeng = CE_state;
                    compl_state->ce_context = CE_state->recv_context;
                    compl_state->transfer_context = netbuf;
                    compl_state->data = (CE_addr_t)(dest_desc->dest_ptr);
                    compl_state->nbytes = nbytes;
                    compl_state->transfer_id = dest_desc->info.meta_data;
                    compl_state->flags = dest_desc->info.byte_swap ?  CE_RECV_FLAG_SWAPPED : 0;
                    adf_nbuf_unmap_single(adf_dev, (adf_nbuf_t)compl_state->transfer_context,
                                    ADF_OS_DMA_FROM_DEVICE);
                    num_cmpl_states++;
                }
            } else {
                break;
            }
        }
        if (compl_queue_head) {
            num_cmpl_states = 0;

            // Hand off the list to the handler function.
            if (hif_state->completion_pendingq_head) {
                hif_state->completion_pendingq_tail->next = compl_queue_head;
                hif_state->completion_pendingq_tail = compl_queue_tail;
            }
            else {
                hif_state->completion_pendingq_head = compl_queue_head;
                hif_state->completion_pendingq_tail = compl_queue_tail;
            }
            /* Alert the recv completion service thread */
            hif_completion_thread(hif_state);
        }
    }

    /* Process Tx CE's */
    for (i = 0; i < MAX_TX_CES; i++) {
        void *CE_context;
        if (tx_ce_list[i] == -1) {
            break;
        }
        CE_id = tx_ce_list[i];

        CE_state = sc->CE_id_to_state[CE_id];
        CE_ENGINE_INT_STATUS_CLEAR(targid, CE_state->ctrl_addr, HOST_IS_COPY_COMPLETE_MASK);

        while (CE_completed_send_next_nolock(CE_state, &CE_context, &tx_transfer_ctx,
                &buf, &nbytes, &id) == A_OK) {
            CE_state->send_cb((struct CE_handle *)CE_state, CE_context, tx_transfer_ctx, buf, nbytes, id);
        }
    }

    /* Just clear the interrupt source for all other CE's */
    for (i = 0; i < MAX_MS_CES; i++) {
        if (misc_ce_list[i] == -1) {
            break;
        }
        CE_id = misc_ce_list[i];
        CE_state = sc->CE_id_to_state[CE_id];
        CE_ENGINE_INT_STATUS_CLEAR(targid, CE_state->ctrl_addr, HOST_IS_COPY_COMPLETE_MASK);
    }
}

/*
 * Cleanup buffers on the t2h HTT msg queue.
 * These buffers are never allocated on the fly, but
 * are allocated only once during HIF start and freed
 * only once during HIF stop.
 * NOTE:
 * The assumption here is there is no in-flight DMA in progress
 * currently, so that buffers can be freed up safely.
 */
static void
CE_t2h_msg_ce_cleanup(struct CE_handle *ce_hdl)
{
    struct CE_state *ce_state = (struct CE_state *)ce_hdl;
    struct CE_ring_state *dst_ring = ce_state->dest_ring;
    struct ath_hif_pci_softc *sc = ce_state->sc;
    adf_nbuf_t nbuf;
    int i; 

    /*
     * Unlike other CE's, this CE is completely full:
     * does not leave one blank space, to distinguish
     * between empty queue & full queue.
     * So free all the entries.
     */
    for (i = 0; i < dst_ring->nentries; i++) {
        nbuf = dst_ring->per_transfer_context[i];

        /*
         * The reasons for doing this check are:
         * 1) Protect against calling cleanup before allocating buffers
         * 2) In a corner case, hif_started may be set, but we could have
         *    a partially filled ring, because of a memory allocation failure
         *    in the middle of allocating ring. 
         *    This check accounts for that case, checking hif_started flag or
         *    started flag would not have covered that case.
         *    This is not in performance path, so OK to do this.
         */
        if (adf_os_likely(nbuf)) {
            adf_nbuf_free(nbuf);
        }
    }
}


/*
 * Place holder function for H2T CE cleanup.
 * No processing is required inside this function.
 * Using an assert, this function makes sure that,
 * the TX CE has been processed completely. 
 */
static void
CE_h2t_tx_ce_cleanup(struct CE_handle *ce_hdl)
{
    struct CE_state *ce_state = (struct CE_state *)ce_hdl;
    struct CE_ring_state *src_ring = ce_state->src_ring;
    struct ath_hif_pci_softc *sc = ce_state->sc;
    uint32_t sw_index, write_index;

    adf_os_spin_lock_bh(&sc->target_lock);
    sw_index = src_ring->sw_index;
    write_index = src_ring->sw_index;
    adf_os_spin_unlock_bh(&sc->target_lock);

    /* At this point Tx CE should be clean */
    adf_os_assert_always(sw_index == write_index);
}
#endif /* QCA_OL_11AC_FAST_PATH */

/*
 * Adjust interrupts for the copy complete handler.
 * If it's needed for either send or recv, then unmask
 * this interrupt; otherwise, mask it.
 *
 * Called with target_lock held.
 */
static void
CE_per_engine_handler_adjust(struct CE_state *CE_state,
                             int disable_copy_compl_intr)
{
    u_int32_t ctrl_addr = CE_state->ctrl_addr;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    A_target_id_t targid = TARGID(sc);

    A_TARGET_ACCESS_BEGIN(targid);
    if ((!disable_copy_compl_intr) &&
        (CE_state->send_cb || CE_state->recv_cb))
    {
        CE_COPY_COMPLETE_INTR_ENABLE(targid, ctrl_addr);
    } else {
        CE_COPY_COMPLETE_INTR_DISABLE(targid, ctrl_addr);
    }

    if (CE_state->watermark_cb) {
        CE_WATERMARK_INTR_ENABLE(targid, ctrl_addr);
    } else {
        CE_WATERMARK_INTR_DISABLE(targid, ctrl_addr);
    }
    A_TARGET_ACCESS_END(targid);

}

void
CE_send_cb_register(struct CE_handle *copyeng,
                    CE_send_cb fn_ptr,
                    void *CE_send_context,
                    int disable_interrupts)
{
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    struct ath_hif_pci_softc *sc = CE_state->sc;

    adf_os_spin_lock(&sc->target_lock);
    CE_state->send_cb = fn_ptr;
    CE_state->send_context = CE_send_context;
    CE_per_engine_handler_adjust(CE_state, disable_interrupts);
    adf_os_spin_unlock(&sc->target_lock);
}

void
CE_recv_cb_register(struct CE_handle *copyeng,
                    CE_recv_cb fn_ptr,
                    void *CE_recv_context)
{
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    struct ath_hif_pci_softc *sc = CE_state->sc;

    adf_os_spin_lock(&sc->target_lock);
    CE_state->recv_cb = fn_ptr;
    CE_state->recv_context = CE_recv_context;
    CE_per_engine_handler_adjust(CE_state, 0);
    adf_os_spin_unlock(&sc->target_lock);
}

void
CE_watermark_cb_register(struct CE_handle *copyeng,
                         CE_watermark_cb fn_ptr,
                         void *CE_wm_context)
{
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    struct ath_hif_pci_softc *sc = CE_state->sc;

    adf_os_spin_lock(&sc->target_lock);
    CE_state->watermark_cb = fn_ptr;
    CE_state->wm_context = CE_wm_context;
    CE_per_engine_handler_adjust(CE_state, 0);
    if (fn_ptr) {
        CE_state->misc_cbs = 1;
    }
    adf_os_spin_unlock(&sc->target_lock);
}

static unsigned int
roundup_pwr2(unsigned int n)
{
    int i;
    unsigned int test_pwr2;

    if (!(n & (n-1))) {
        return n; /* already a power of 2 */
    }

    test_pwr2 = 4;
    for (i=0; i<29; i++) {
        if (test_pwr2 > n) {
            return test_pwr2;
        }
        test_pwr2 = test_pwr2 << 1;
    }

    A_ASSERT(0); /* n too large */
    return 0;
}

#if QCA_OL_11AC_FAST_PATH
void CE_pkt_dl_len_set(void *hif_sc,
                          u_int32_t pkt_download_len)
{
    struct ath_hif_pci_softc *sc = 
        (struct ath_hif_pci_softc *)(hif_sc);

    struct CE_state *CE_state = 
            sc->CE_id_to_state[CE_HTT_TX_CE];
    
    adf_os_assert_always(CE_state);

    adf_os_spin_lock_bh(&sc->target_lock);
    CE_state->download_len = pkt_download_len; 
    adf_os_spin_unlock_bh(&sc->target_lock);

    adf_os_print("%s CE %d Pkt download length %d\n",
            __func__, CE_state->id, CE_state->download_len);
}
#endif /* QCA_OL_11AC_FAST_PATH */

/*
 * Initialize a Copy Engine based on caller-supplied attributes.
 * This may be called once to initialize both source and destination
 * rings or it may be called twice for separate source and destination
 * initialization. It may be that only one side or the other is
 * initialized by software/firmware.
 */
struct CE_handle *
CE_init(struct ath_hif_pci_softc *sc,
        unsigned int CE_id,
        struct CE_attr *attr)
{
    struct CE_state *CE_state;
    u_int32_t ctrl_addr;
    A_target_id_t targid;
    unsigned int nentries;
    adf_os_dma_addr_t base_addr;
    struct ol_ath_softc_net80211 *scn = sc->scn;

    A_ASSERT(CE_id < sc->ce_count);
    ctrl_addr = CE_BASE_ADDRESS(CE_id);
    adf_os_spin_lock(&sc->target_lock);
    CE_state = sc->CE_id_to_state[CE_id];
    

    if (!CE_state) {
        adf_os_spin_unlock(&sc->target_lock);
        CE_state = (struct CE_state *)A_MALLOC(sizeof(*CE_state));
		A_ASSERT(CE_state); /* TBDXXX */
        A_MEMZERO(CE_state, sizeof(*CE_state));
        adf_os_spin_lock(&sc->target_lock);
        if (!sc->CE_id_to_state[CE_id]) { /* re-check under lock */
            sc->CE_id_to_state[CE_id] = CE_state;

            CE_state->sc = sc;
            CE_state->id = CE_id;
            CE_state->ctrl_addr = ctrl_addr;
            CE_state->state = CE_RUNNING;
            CE_state->attr_flags = attr->flags; /* Save attribute flags */
        } else {
            /*
             * We released target_lock in order to allocate CE state,
             * but someone else beat us to it.  Continue, using that
             * CE_state (and free the one we allocated).
             */
            A_FREE(CE_state);
            CE_state = sc->CE_id_to_state[CE_id];
        }
    }
    adf_os_spin_unlock(&sc->target_lock);

    if (attr == NULL) {
        /* Already initialized; caller wants the handle */
        return (struct CE_handle *)CE_state;
    }

    targid = TARGID(sc);

    if (CE_state->src_sz_max) {
        A_ASSERT(CE_state->src_sz_max == attr->src_sz_max);
    } else {
        CE_state->src_sz_max = attr->src_sz_max;
    }

    /* source ring setup */
    nentries = attr->src_nentries;
    if (nentries) {
        struct CE_ring_state *src_ring;
        unsigned CE_nbytes;
        char *ptr;

        nentries = roundup_pwr2(nentries);
        if (CE_state->src_ring) {
            A_ASSERT(CE_state->src_ring->nentries == nentries);
        } else {
            CE_nbytes = sizeof(struct CE_ring_state)
                      + (nentries * sizeof(void *)); /* per-send context */
            ptr = A_MALLOC(CE_nbytes);
            A_ASSERT(ptr); /* TBDXXX */
            A_MEMZERO(ptr, CE_nbytes);

            src_ring = CE_state->src_ring = (struct CE_ring_state *)ptr;
            ptr += sizeof(struct CE_ring_state);
            src_ring->nentries = nentries;
            src_ring->nentries_mask = nentries-1;
            A_TARGET_ACCESS_BEGIN(targid);
            src_ring->hw_index =
                src_ring->sw_index = CE_SRC_RING_READ_IDX_GET(targid, ctrl_addr);
            src_ring->write_index = CE_SRC_RING_WRITE_IDX_GET(targid, ctrl_addr);
            A_TARGET_ACCESS_END(targid);
            src_ring->low_water_mark_nentries = 0;
            src_ring->high_water_mark_nentries = nentries;
            src_ring->per_transfer_context = (void **)ptr;
            ptr += (nentries * sizeof(void *));

            /* Legacy platforms that do not support cache coherent DMA are unsupported */
            src_ring->base_addr_owner_space_unaligned =
                OS_MALLOC_CONSISTENT(scn->sc_osdev,
                                    (nentries * sizeof(struct CE_src_desc) + CE_DESC_RING_ALIGN),
                                    &base_addr,
                                    OS_GET_DMA_MEM_CONTEXT(src_ring, ce_dmacontext), 0);
            src_ring->base_addr_CE_space_unaligned = base_addr;
		

            if (src_ring->base_addr_CE_space_unaligned & (CE_DESC_RING_ALIGN-1)) {

                src_ring->base_addr_CE_space = (src_ring->base_addr_CE_space_unaligned +
                        CE_DESC_RING_ALIGN-1) & ~(CE_DESC_RING_ALIGN-1);

                src_ring->base_addr_owner_space = (void *)(((size_t)src_ring->base_addr_owner_space_unaligned +
                        CE_DESC_RING_ALIGN-1) & ~(CE_DESC_RING_ALIGN-1));
            } else {
                src_ring->base_addr_CE_space = src_ring->base_addr_CE_space_unaligned;
                src_ring->base_addr_owner_space = src_ring->base_addr_owner_space_unaligned;
            }
            /*
             * Also allocate a shadow src ring in regular mem to use for
             * faster access.
             */
            src_ring->shadow_base_unaligned = A_MALLOC(
                nentries * sizeof(struct CE_src_desc) + CE_DESC_RING_ALIGN);
            src_ring->shadow_base = (struct CE_src_desc *)
                (((size_t) src_ring->shadow_base_unaligned +
                CE_DESC_RING_ALIGN-1) & ~(CE_DESC_RING_ALIGN-1));

#if QCA_OL_11AC_FAST_PATH
            adf_os_spinlock_init(&src_ring->ce_ring_lock);
#endif
            A_TARGET_ACCESS_BEGIN(targid);
            CE_SRC_RING_BASE_ADDR_SET(targid, ctrl_addr, src_ring->base_addr_CE_space);
            CE_SRC_RING_SZ_SET(targid, ctrl_addr, nentries);
            CE_SRC_RING_DMAX_SET(targid, ctrl_addr, attr->src_sz_max);
#ifdef BIG_ENDIAN_HOST
            /* Enable source ring byte swap for big endian host */
            CE_SRC_RING_BYTE_SWAP_SET(targid, ctrl_addr, 1);
#endif
            CE_SRC_RING_LOWMARK_SET(targid, ctrl_addr, 0);
            CE_SRC_RING_HIGHMARK_SET(targid, ctrl_addr, nentries);
            A_TARGET_ACCESS_END(targid);
        }
    }

    /* destination ring setup */
    nentries = attr->dest_nentries;
    if (nentries) {
        struct CE_ring_state *dest_ring;
        unsigned CE_nbytes;
        char *ptr;

        nentries = roundup_pwr2(nentries);
        if (CE_state->dest_ring) {
            A_ASSERT(CE_state->dest_ring->nentries == nentries);
        } else {
            CE_nbytes = sizeof(struct CE_ring_state)
                      + (nentries * sizeof(void *)); /* per-recv context */
            ptr = A_MALLOC(CE_nbytes);
            A_ASSERT(ptr); /* TBDXXX */
            A_MEMZERO(ptr, CE_nbytes);

            dest_ring = CE_state->dest_ring = (struct CE_ring_state *)ptr;
            ptr += sizeof(struct CE_ring_state);
            dest_ring->nentries = nentries;
            dest_ring->nentries_mask = nentries-1;
            A_TARGET_ACCESS_BEGIN(targid);
            dest_ring->sw_index = CE_DEST_RING_READ_IDX_GET(targid, ctrl_addr);
            dest_ring->write_index = CE_DEST_RING_WRITE_IDX_GET(targid, ctrl_addr);
            A_TARGET_ACCESS_END(targid);
            dest_ring->low_water_mark_nentries = 0;
            dest_ring->high_water_mark_nentries = nentries;
            dest_ring->per_transfer_context = (void **)ptr;
            ptr += (nentries * sizeof(void *));

            /* Legacy platforms that do not support cache coherent DMA are unsupported */
            dest_ring->base_addr_owner_space_unaligned =
                OS_MALLOC_CONSISTENT(scn->sc_osdev,
                                    (nentries * sizeof(struct CE_dest_desc) + CE_DESC_RING_ALIGN),
                                    &base_addr,
                                    OS_GET_DMA_MEM_CONTEXT(dest_ring, ce_dmacontext), 0);
            dest_ring->base_addr_CE_space_unaligned = base_addr;

            /* Correctly initialize memory to 0 to prevent garbage data
             * crashing system when download firmware
             */
            A_MEMZERO(dest_ring->base_addr_owner_space_unaligned, nentries * sizeof(struct CE_dest_desc) + CE_DESC_RING_ALIGN);

            if (dest_ring->base_addr_CE_space_unaligned & (CE_DESC_RING_ALIGN-1)) {

                dest_ring->base_addr_CE_space = (dest_ring->base_addr_CE_space_unaligned +
                        CE_DESC_RING_ALIGN-1) & ~(CE_DESC_RING_ALIGN-1);

                dest_ring->base_addr_owner_space = (void *)(((size_t)dest_ring->base_addr_owner_space_unaligned +
                        CE_DESC_RING_ALIGN-1) & ~(CE_DESC_RING_ALIGN-1));
            } else {
                dest_ring->base_addr_CE_space = dest_ring->base_addr_CE_space_unaligned;
                dest_ring->base_addr_owner_space = dest_ring->base_addr_owner_space_unaligned;
            }
#if QCA_OL_11AC_FAST_PATH
            adf_os_spinlock_init(&dest_ring->ce_ring_lock);
#endif
            A_TARGET_ACCESS_BEGIN(targid);
            CE_DEST_RING_BASE_ADDR_SET(targid, ctrl_addr, dest_ring->base_addr_CE_space);
            CE_DEST_RING_SZ_SET(targid, ctrl_addr, nentries);
#ifdef BIG_ENDIAN_HOST
            /* Enable Destination ring byte swap for big endian host */
            CE_DEST_RING_BYTE_SWAP_SET(targid, ctrl_addr, 1);
#endif
            CE_DEST_RING_LOWMARK_SET(targid, ctrl_addr, 0);
            CE_DEST_RING_HIGHMARK_SET(targid, ctrl_addr, nentries);
            A_TARGET_ACCESS_END(targid);
        }
    }

    /* Enable CE error interrupts */
    A_TARGET_ACCESS_BEGIN(targid);
    CE_ERROR_INTR_ENABLE(targid, ctrl_addr);
    A_TARGET_ACCESS_END(targid);

    return (struct CE_handle *)CE_state;
}

void
CE_pause(struct CE_handle *copyeng)
{
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    A_UINT32 ctrl_addr = CE_state->ctrl_addr;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    A_target_id_t targid = TARGID(sc);
    int i = 0;
    A_TARGET_ACCESS_BEGIN(targid);
    CE_HALT(targid, ctrl_addr);
    while (!CE_HALT_STATUS(targid, ctrl_addr)) {
        A_MDELAY(1);
        if (i++ > CE_HALT_TIMEOUT_MS) {
            printk("%s : %d : ERROR failed to HALT Copy Engine id %d \n", 
                     __func__, __LINE__, CE_state->id);
            goto done;
        }
    }
done:
    A_TARGET_ACCESS_END(targid);
}


void
CE_fini(struct CE_handle *copyeng)
{
    struct CE_state *CE_state = (struct CE_state *)copyeng;
    unsigned int CE_id = CE_state->id;
    struct ath_hif_pci_softc *sc = CE_state->sc;
    struct ol_ath_softc_net80211 *scn = sc->scn;

    CE_state->state = CE_UNUSED;
    CE_state->sc->CE_id_to_state[CE_id] = NULL;
    /* temp. workaround to Halt CE to avoid any uplink transfer during SOC reset */
    CE_pause(copyeng);
    if (CE_state->src_ring) {
#if QCA_OL_11AC_FAST_PATH
        /* Cleanup the HTT Tx ring */
        if (CE_state->id == CE_HTT_TX_CE) {
            adf_os_print("%s %d Cleaning up HTT Tx CE\n", __func__, __LINE__);
            CE_h2t_tx_ce_cleanup(copyeng);
        }
#endif
        A_FREE(CE_state->src_ring->shadow_base_unaligned);
		OS_FREE_CONSISTENT(scn->sc_osdev,
                   (CE_state->src_ring->nentries * sizeof(struct CE_dest_desc) + CE_DESC_RING_ALIGN),
                   CE_state->src_ring->base_addr_owner_space, CE_state->src_ring->base_addr_CE_space,
                   OS_GET_DMA_MEM_CONTEXT(CE_state->src_ring, ce_dmacontext));
#if QCA_OL_11AC_FAST_PATH
        adf_os_spinlock_destroy(&CE_state->src_ring->ce_ring_lock);
#endif
        A_FREE(CE_state->src_ring);
    }
    if (CE_state->dest_ring) {
#if QCA_OL_11AC_FAST_PATH
        /* Cleanup the HTT Tx ring */
        if (CE_state->id == CE_HTT_MSG_CE) {
            adf_os_print("%s Cleaning up HTT MSG CE(%d)\n", __func__, CE_HTT_MSG_CE);
            CE_t2h_msg_ce_cleanup(copyeng);
        }
#endif /* QCA_OL_11AC_FAST_PATH */
		OS_FREE_CONSISTENT(scn->sc_osdev,
                   (CE_state->dest_ring->nentries * sizeof(struct CE_dest_desc) + CE_DESC_RING_ALIGN),
                   CE_state->dest_ring->base_addr_owner_space, CE_state->dest_ring->base_addr_CE_space,
                   OS_GET_DMA_MEM_CONTEXT(CE_state->dest_ring, ce_dmacontext));
#if QCA_OL_11AC_FAST_PATH
        adf_os_spinlock_destroy(&CE_state->dest_ring->ce_ring_lock);
#endif
        A_FREE(CE_state->dest_ring);
    }
    A_FREE(CE_state);
}
