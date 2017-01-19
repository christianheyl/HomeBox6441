/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#include <adf_nbuf.h>         /* adf_nbuf_t, etc. */


#if QCA_OL_11AC_FAST_PATH
#include <hif.h>              /* HIF_DEVICE */
#endif 
#include <htt.h>              /* HTT_TX_EXT_TID_MGMT */
#if QCA_OL_11AC_FAST_PATH
#include <htt_internal.h>     /* */
#include <htt_types.h>        /* htc_endpoint */ 
#endif
#include <ol_htt_tx_api.h>    /* htt_tx_desc_tid */
#include <ol_txrx_api.h>      /* ol_txrx_vdev_handle */
#include <ol_txrx_ctrl_api.h> /* ol_txrx_sync */

#include <ol_txrx_internal.h> /* TXRX_ASSERT1 */
#include <ol_txrx_types.h>    /* pdev stats */
#include <ol_tx_desc.h>       /* ol_tx_desc */
#include <ol_tx_send.h>       /* ol_tx_send */



#if QCA_OL_11AC_FAST_PATH
#include <htc_api.h>         /* Layering violation, but required for fast path */
#include <copy_engine_api.h>
#endif  /* QCA_OL_11AC_FAST_PATH */

#define ol_tx_prepare_ll(tx_desc, vdev, msdu) \
    do {                                                                      \
        /* 
         * The TXRX module doesn't accept tx frames unless the target has 
         * enough descriptors for them.
         */                                                                   \
        if (adf_os_atomic_read(&vdev->pdev->target_tx_credit) <= 0) {         \
            TXRX_STATS_MSDU_LIST_INCR(                                        \
                vdev->pdev, tx.dropped.host_reject, msdu);                    \
            return msdu;                                                      \
        }                                                                     \
                                                                              \
        tx_desc = ol_tx_desc_ll(vdev->pdev, vdev, msdu);                      \
        if (! tx_desc) {                                                      \
            TXRX_STATS_MSDU_LIST_INCR(                                        \
                vdev->pdev, tx.dropped.host_reject, msdu);                    \
            return msdu; /* the list of unaccepted MSDUs */                   \
        }                                                                     \
        OL_TXRX_PROT_AN_LOG(vdev->pdev->prot_an_tx_sent, msdu);               \
    } while (0)

adf_nbuf_t
ol_tx_ll(ol_txrx_vdev_handle vdev, adf_nbuf_t msdu_list)
{
    adf_nbuf_t msdu = msdu_list;
    /*
     * The msdu_list variable could be used instead of the msdu var, 
     * but just to clarify which operations are done on a single MSDU
     * vs. a list of MSDUs, use a distinct variable for single MSDUs
     * within the list.
     */
    while (msdu) {
        adf_nbuf_t next;
        struct ol_tx_desc_t *tx_desc;

        ol_tx_prepare_ll(tx_desc, vdev, msdu);
        /*
         * If debug display is enabled, show the meta-data being
         * downloaded to the target via the HTT tx descriptor.
         */
        htt_tx_desc_display(tx_desc->htt_tx_desc);
        /*
         * The netbuf may get linked into a different list inside the
         * ol_tx_send function, so store the next pointer before the
         * tx_send call.
         */
        next = adf_nbuf_next(msdu);
        ol_tx_send(vdev, tx_desc, msdu);
        msdu = next;
    }
    return NULL; /* all MSDUs were accepted */
}

#if QCA_OL_11AC_FAST_PATH

#define OL_HTT_TX_DESC_FILL(_htt_pdev, _htt_tx_desc, _tx_desc_id,           \
            _vdev_id, _msdu_paddr, _msdu_len, _pkt_cksum)                   \
do  {                                                                       \
    uint32_t *word;                                                         \
    htt_tx_desc_init((_htt_pdev), (_htt_tx_desc), (_tx_desc_id),            \
         (_msdu_len), (_vdev_id), htt_pkt_type_ethernet, (_pkt_cksum),      \
         ADF_NBUF_TX_EXT_TID_INVALID);                                      \
                                                                            \
    word = (u_int32_t *) (((char *)(_htt_tx_desc)) + HTT_TX_DESC_LEN);      \
    *word = (_msdu_paddr);                                                  \
    word++;                                                                 \
    *word = (_msdu_len);                                                    \
    word++;                                                                 \
    *word = 0;                                                              \
} while (0)


/*
 * ol_tx_ll_fast(): Fast path OL layer send function
 * Function:
 * 1) Get OL Tx desc
 * 2) Fill out HTT + HTC headers
 * 3) Fill out SG list
 * 4) Store meta-data (implicit, because we use pre-allocated pool)
 * 5) Call CE send function
 * Returns:
 *  No. of nbufs that could not be sent.
 */
uint32_t
ol_tx_ll_fast(ol_txrx_vdev_handle vdev,
              adf_nbuf_t *nbuf_arr,
              uint32_t num_msdus)
{
    struct ol_txrx_pdev_t *pdev = vdev->pdev;
    struct ol_tx_desc_t *tx_desc = NULL;
    uint32_t *htt_tx_desc;
    adf_nbuf_t nbuf;
    uint32_t pkt_download_len, num_sent = 0;
    uint32_t ep_id;
    void *htc_hdr_vaddr;
    uint32_t htc_hdr_paddr_lo; /* LSB of physical address */
    int i = 0;

    ASSERT(num_msdus);
    pkt_download_len = ((struct htt_pdev_t *)(pdev->htt_pdev))->download_len;

    /* This can be statically inited once, during allocation */
    /* Call this HTC_HTTEPID_GET */
    ep_id = HTT_EPID_GET(pdev->htt_pdev); 

    /* TODO : Can we batch alloc OL tx descriptors? */

    for (i = 0; i < num_msdus; i++) {
        /* Get Tx desc from pre-allocated pool */
        /* TODO : ol_tx_desc_alloc() uses a lock
         * Check if this lock is required
         */
        tx_desc = ol_tx_desc_alloc(pdev); 
        if (adf_os_unlikely(!tx_desc)) {
            TXRX_STATS_ADD(pdev, pub.tx.desc_alloc_fails, 1);
            break;
        }

        nbuf = nbuf_arr[i];

        /* Fill out ol_tx_desc */
        tx_desc->netbuf = nbuf;
        tx_desc->pkt_type = ol_tx_frm_std; /* this can be prefilled? */

        htt_tx_desc = tx_desc->htt_tx_desc;

        /* HTT Header */
        /* 0. TODO: Prefill as much as possible into HTT header at allocation */
        /* 1. Fill per packet info in HTT descriptor */
        /* 2. Fill the scatter gather list */
        /* 3. What about OS'es providing multiple frags? */
        OL_HTT_TX_DESC_FILL(pdev->htt_pdev, htt_tx_desc, tx_desc->id, vdev->vdev_id,
            adf_nbuf_get_frag_paddr_lo(nbuf, 1), adf_nbuf_len(nbuf),
            adf_nbuf_get_tx_cksum(nbuf));

        /* Get the virtual address of the fragment added by driver */
        htc_hdr_vaddr = (char *)htt_tx_desc - HTC_HEADER_LEN;

        /* TODO: Precompute and store paddr in ol_tx_desc_t */
        htc_hdr_paddr_lo = (u_int32_t) 
                        HTT_TX_DESC_PADDR(pdev->htt_pdev, htc_hdr_vaddr);

        /* Add meta-data for the HTT/HTC header fragment to nbuf->cb */ 
        /* After this point num_extra_frags = 1 */
        /* Can this be passed as parameters to CE to program?
         * Why should we store this at all ?
         */
        adf_nbuf_frag_push_head(
                                nbuf,
                                HTC_HEADER_LEN + HTT_TX_DESC_LEN, 
                                htc_hdr_vaddr, htc_hdr_paddr_lo /*phy addr LSB*/,
                                0 /* phy addr MSB */);

        /*
         *  Do we want to turn on word_stream bit-map here ? For linux, non-TSO this is
         *  not required.
         *  We still have to mark the swap bit correctly, when posting to the ring
         */
        /* Check to make sure, data download length is correct */

        /* TODO : Can we remove this check and always download a fixed length ? */
        if (adf_os_unlikely(adf_nbuf_len(nbuf) < pkt_download_len)) {
            pkt_download_len = adf_nbuf_len(nbuf);
        }

        /* Fill the HTC header information */
        /* 
         * Passing 0 as the seq_no field, we can probably get away
         * with it for the time being, since this is not checked in f/w
         */
        /* Should prefill this also */
        /* Should look at the multi-fragment case */
        HTC_TX_DESC_FILL(htc_hdr_vaddr, pkt_download_len, ep_id, 0);
    }

    /*
     * If we could get descriptor for i packets, just send them one shot
     * to the CE ring
     * Assumption: if there is enough descriptors i should be equal to num_msdus
     */
    if (i) {
        num_sent = CE_send_fast(pdev->ce_tx_hdl, nbuf_arr, i, ep_id);
    }

    ASSERT((num_msdus - num_sent) == 0);

    /* Assume num_msdus == 1 */
    if ((num_sent == 0) && tx_desc) {
        ol_tx_desc_free(pdev, tx_desc);
    }

    /*
     * If there was only a partial send,
     * send the part of the nbuf_arr that could not be
     * sent back to the caller.
     */
    return (num_msdus - num_sent);
}

inline void
ol_tx_stats_inc_pkt_cnt(ol_txrx_vdev_handle vdev) 
{
    struct ol_txrx_pdev_t *pdev = vdev->pdev;

    TXRX_STATS_ADD(pdev, pub.tx.from_stack.pkts, 1);
}

void
ol_tx_stats_inc_map_error(ol_txrx_vdev_handle vdev,
                             uint32_t num_map_error)
{
    struct ol_txrx_pdev_t *pdev = vdev->pdev;

    TXRX_STATS_ADD(pdev, pub.tx.dma_map_error, 1);
}

void
ol_tx_stats_inc_ring_error(ol_txrx_pdev_handle pdev,
                             uint32_t num_ring_error)
{
    TXRX_STATS_ADD(pdev, pub.tx.ce_ring_full, 1);
}

#endif /* QCA_OL_11AC_FAST_PATH */

static inline int
OL_TXRX_TX_IS_RAW(enum ol_txrx_osif_tx_spec tx_spec)
{
    return
        tx_spec &
        (ol_txrx_osif_tx_spec_raw |
         ol_txrx_osif_tx_spec_no_aggr |
         ol_txrx_osif_tx_spec_no_encrypt);
}

static inline u_int8_t
OL_TXRX_TX_RAW_SUBTYPE(enum ol_txrx_osif_tx_spec tx_spec)
{
    u_int8_t sub_type = 0x1; /* 802.11 MAC header present */

    if (tx_spec & ol_txrx_osif_tx_spec_no_aggr) {
        sub_type |= 0x1 << HTT_TX_MSDU_DESC_RAW_SUBTYPE_NO_AGGR_S;
    }
    if (tx_spec & ol_txrx_osif_tx_spec_no_encrypt) {
        sub_type |= 0x1 << HTT_TX_MSDU_DESC_RAW_SUBTYPE_NO_ENCRYPT_S;
    }
    if (tx_spec & ol_txrx_osif_tx_spect_nwifi_no_encrypt) {
        sub_type |= 0x1 << HTT_TX_MSDU_DESC_RAW_SUBTYPE_NO_ENCRYPT_S;
    }
    return sub_type;
}

adf_nbuf_t
ol_tx_non_std_ll(
    ol_txrx_vdev_handle vdev,
    u_int8_t ext_tid,
    enum ol_txrx_osif_tx_spec tx_spec,
    adf_nbuf_t msdu_list)
{
    adf_nbuf_t msdu = msdu_list;
    htt_pdev_handle htt_pdev = vdev->pdev->htt_pdev;
    /*
     * The msdu_list variable could be used instead of the msdu var, 
     * but just to clarify which operations are done on a single MSDU
     * vs. a list of MSDUs, use a distinct variable for single MSDUs
     * within the list.
     */
    while (msdu) {
        adf_nbuf_t next;
        struct ol_tx_desc_t *tx_desc;

        ol_tx_prepare_ll(tx_desc, vdev, msdu);

        /*
         * The netbuf may get linked into a different list inside the
         * ol_tx_send function, so store the next pointer before the
         * tx_send call.
         */
        next = adf_nbuf_next(msdu);

        if (tx_spec != ol_txrx_osif_tx_spec_std) {
            if (tx_spec & ol_txrx_osif_tx_spec_tso) {
                tx_desc->pkt_type = ol_tx_frm_tso;
            } else if (tx_spec & ol_txrx_osif_tx_spect_nwifi_no_encrypt) {
                u_int8_t sub_type = OL_TXRX_TX_RAW_SUBTYPE(tx_spec);
                htt_tx_desc_type(
                    htt_pdev, tx_desc->htt_tx_desc,
                    htt_pkt_type_native_wifi, sub_type);
            } else if (OL_TXRX_TX_IS_RAW(tx_spec)) {
                /* different types of raw frames */
                u_int8_t sub_type = OL_TXRX_TX_RAW_SUBTYPE(tx_spec);
                htt_tx_desc_type(
                    htt_pdev, tx_desc->htt_tx_desc,
                    htt_pkt_type_raw, sub_type);
            }
        }
        /* explicitly specify the TID and the limit 
         * it to the 0-15 value of the QoS TID.
         */

        if (ext_tid >= HTT_TX_EXT_TID_NON_QOS_MCAST_BCAST) {
            ext_tid = HTT_TX_EXT_TID_DEFAULT;           
        }
        htt_tx_desc_tid(htt_pdev, tx_desc->htt_tx_desc, ext_tid);

        /*
         * If debug display is enabled, show the meta-data being
         * downloaded to the target via the HTT tx descriptor.
         */
        htt_tx_desc_display(tx_desc->htt_tx_desc);
        ol_tx_send(vdev, tx_desc, msdu);
        msdu = next;
    }
    return NULL; /* all MSDUs were accepted */
}

adf_nbuf_t
ol_tx_hl(ol_txrx_vdev_handle vdev, adf_nbuf_t msdu_list)
{
    adf_nbuf_t msdu;

    /*
     * TBDXXX - TXRX module for now only downloads number
     * of descriptors that target could accept.
     */
    if (adf_os_atomic_read(&vdev->pdev->target_tx_credit) <= 0) {
        return msdu_list;
    }

    /*
     * The msdu_list variable could be used instead of the msdu var, 
     * but just to clarify which operations are done on a single MSDU
     * vs. a list of MSDUs, use a distinct variable for single MSDUs
     * within the list.
     */
    msdu = msdu_list;
    while (msdu) {
        adf_nbuf_t next;
        struct ol_tx_desc_t *tx_desc;
        tx_desc = ol_tx_desc_hl(vdev->pdev, vdev, msdu);
        if (! tx_desc) {
            TXRX_STATS_MSDU_LIST_INCR(vdev->pdev, tx.dropped.host_reject, msdu);
            return msdu; /* the list of unaccepted MSDUs */
        }
        OL_TXRX_PROT_AN_LOG(vdev->pdev->prot_an_tx_sent, msdu);
        /*
         * If debug display is enabled, show the meta-data being
         * downloaded to the target via the HTT tx descriptor.
         */
        htt_tx_desc_display(tx_desc->htt_tx_desc);
        /*
         * The netbuf will get stored into a (peer-TID) tx queue list
         * inside the ol_tx_classify_store function, so store the next
         * pointer before the tx_classify_store call.
         */
        next = adf_nbuf_next(msdu);
/*
 * FIX THIS:
 * 2.  Call tx classify to determine peer and TID.
 * 3.  Store the frame in a peer-TID queue.
 */
//        ol_tx_classify_store(vdev, tx_desc, msdu);
        /*
         * FIX THIS
         * temp FIFO for bringup
         */
        ol_tx_send(vdev, tx_desc, msdu);
        msdu = next;
    }
/*
 * FIX THIS:
 * 4.  Invoke the download scheduler.
 */
//    ol_tx_dl_sched(vdev->pdev);

    return NULL; /* all MSDUs were accepted */
}

adf_nbuf_t
ol_tx_non_std_hl(
    ol_txrx_vdev_handle data_vdev,
    u_int8_t ext_tid,
    enum ol_txrx_osif_tx_spec tx_spec,
    adf_nbuf_t msdu_list)
{
    /* FILL IN HERE */
    adf_os_assert(0);
    return NULL;
}

void
ol_txrx_mgmt_tx_cb_set(
    ol_txrx_pdev_handle pdev,
    u_int8_t type,
    ol_txrx_mgmt_tx_cb cb,
    void *ctxt)
{
    TXRX_ASSERT1(type < OL_TXRX_MGMT_NUM_TYPES);
    pdev->tx_mgmt.callbacks[type].cb = cb;
    pdev->tx_mgmt.callbacks[type].ctxt = ctxt;
}

int
ol_txrx_mgmt_send(
    ol_txrx_vdev_handle vdev,
    adf_nbuf_t tx_mgmt_frm,
    u_int8_t type)
{
    struct ol_txrx_pdev_t *pdev = vdev->pdev;
    struct ol_tx_desc_t *tx_desc;

    if (pdev->cfg.is_high_latency) {
        tx_desc = ol_tx_desc_hl(pdev, vdev, tx_mgmt_frm);
    } else {
        tx_desc = ol_tx_desc_ll(pdev, vdev, tx_mgmt_frm);
    }
    if (! tx_desc) {
        return 1; /* can't accept the tx mgmt frame */
    }
    TXRX_STATS_MSDU_INCR(vdev->pdev, tx.mgmt, tx_mgmt_frm);
    adf_nbuf_map_single(pdev->osdev, tx_mgmt_frm, ADF_OS_DMA_TO_DEVICE);

    TXRX_ASSERT1(type < OL_TXRX_MGMT_NUM_TYPES);
    tx_desc->pkt_type = type + OL_TXRX_MGMT_TYPE_BASE;
    htt_tx_desc_tid(pdev->htt_pdev, tx_desc->htt_tx_desc, HTT_TX_EXT_TID_MGMT);
    
    if (pdev->cfg.is_high_latency) {
/*
 * FIX THIS:
 * 1.  Look up the peer and queue the frame in the peer's mgmt queue.
 * 2.  Invoke the download scheduler.
 */
//        ol_tx_mgmt_classify_store(vdev, tx_desc, tx_mgmt_frm);
//        ol_tx_dl_sched(vdev->pdev);
    } else {
        ol_tx_send(vdev, tx_desc, tx_mgmt_frm);
    }

    return 0; /* accepted the tx mgmt frame */
}

void
ol_txrx_sync(ol_txrx_pdev_handle pdev, u_int8_t sync_cnt)
{
    htt_h2t_sync_msg(pdev->htt_pdev, sync_cnt);
}

adf_nbuf_t ol_tx_reinject(
    struct ol_txrx_vdev_t *vdev,
    adf_nbuf_t msdu, uint32_t peer_id)
{
	struct ol_tx_desc_t *tx_desc;

    ol_tx_prepare_ll(tx_desc, vdev, msdu);
    HTT_TX_DESC_POSTPONED_SET(*((u_int32_t *)(tx_desc->htt_tx_desc)), TRUE);

    htt_tx_desc_set_peer_id((u_int32_t *)(tx_desc->htt_tx_desc), peer_id);

    ol_tx_send(vdev, tx_desc, msdu);

    return NULL;
}

