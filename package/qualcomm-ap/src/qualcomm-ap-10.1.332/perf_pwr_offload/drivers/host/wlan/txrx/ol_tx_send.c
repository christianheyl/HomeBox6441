/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#include <adf_os_atomic.h>   /* adf_os_atomic_inc, etc. */
#include <adf_os_lock.h>     /* adf_os_spinlock */
#include <adf_nbuf.h>        /* adf_nbuf_t */
#include <net.h>
#include <queue.h>           /* TAILQ */

#include <ol_txrx_api.h>     /* ol_txrx_vdev_handle, etc. */
#include <ol_htt_tx_api.h>   /* htt_tx_compl_desc_id */
#include <ol_txrx_htt_api.h> /* htt_tx_status */

#include <ol_txrx_types.h>   /* ol_txrx_vdev_t, etc */
#include <ol_tx_desc.h>      /* ol_tx_desc_find, ol_tx_desc_frame_free */
#include <ol_txrx_internal.h>
#include <ol_if_athvar.h>

#include <ol_cfg.h>          /* ol_cfg_is_high_latency */

void
ol_tx_send(
    struct ol_txrx_vdev_t *vdev,
    struct ol_tx_desc_t *tx_desc,
    adf_nbuf_t msdu)
{
    struct ol_txrx_pdev_t *pdev = vdev->pdev;
    u_int16_t id;

    adf_os_atomic_dec(&pdev->target_tx_credit);

    /*
     * When the tx frame is downloaded to the target, there are two
     * outstanding references:
     * 1.  The host download SW (HTT, HTC, HIF)
     *     This reference is cleared by the ol_tx_send_done callback
     *     functions.
     * 2.  The target FW
     *     This reference is cleared by the ol_tx_completion_handler
     *     function.
     * It is extremely probable that the download completion is processed
     * before the tx completion message.  However, under exceptional
     * conditions the tx completion may be processed first.  Thus, rather
     * that assuming that reference (1) is done before reference (2),
     * explicit reference tracking is needed.
     * Double-increment the ref count to account for both references
     * described above.
     */

#if !ATH_11AC_TXCOMPACT
    adf_os_atomic_init(&tx_desc->ref_cnt);
    adf_os_atomic_inc(&tx_desc->ref_cnt);
    adf_os_atomic_inc(&tx_desc->ref_cnt); 
#endif

    id = ol_tx_desc_id(pdev, tx_desc);
    if (htt_tx_send_std(pdev->htt_pdev, tx_desc->htt_tx_desc, msdu, id)) {
/* TBD:
 * Would it be better to just free the descriptor, and return the netbuf
 * to the OS shim, rather than freeing both the descriptor and the
 * netbuf here?
 */
        adf_os_atomic_inc(&pdev->target_tx_credit);
        ol_tx_desc_frame_free_nonstd(pdev, tx_desc, 1 /* had error */);
    }
}

static inline void
ol_tx_download_done_base(
    struct ol_txrx_pdev_t *pdev,
    A_STATUS status,
    adf_nbuf_t msdu,
    u_int16_t msdu_id)
{
    struct ol_tx_desc_t *tx_desc;

    tx_desc = ol_tx_desc_find(pdev, msdu_id);
    adf_os_assert(tx_desc);
    if (status != A_OK) {
        ol_tx_desc_frame_free_nonstd(pdev, tx_desc, 1 /* download err */);
    } else {
#if !ATH_11AC_TXCOMPACT        
        if (adf_os_atomic_dec_and_test(&tx_desc->ref_cnt)) 
#endif            
        {
            /*
             * The decremented value was zero - free the frame.
             * Use the tx status recorded previously during
             * tx completion handling.
             */
            ol_tx_desc_frame_free_nonstd(
                pdev, tx_desc, tx_desc->status != htt_tx_status_ok);
        }
    }
}

void
ol_tx_download_done_ll(
    void *pdev,
    A_STATUS status,
    adf_nbuf_t msdu,
    u_int16_t msdu_id)
{
    ol_tx_download_done_base(
        (struct ol_txrx_pdev_t *) pdev, status, msdu, msdu_id);
}

void
ol_tx_download_done_hl_retain(
    void *txrx_pdev,
    A_STATUS status,
    adf_nbuf_t msdu,
    u_int16_t msdu_id)
{
    struct ol_txrx_pdev_t *pdev = txrx_pdev;
    ol_tx_download_done_base(pdev, status, msdu, msdu_id);
#if 0 /* TODO: Advanced feature */
    //ol_tx_dwl_sched(pdev, OL_TX_HL_SCHED_DOWNLOAD_DONE);
adf_os_assert(0);
#endif
}

void
ol_tx_download_done_hl_free(
    void *txrx_pdev,
    A_STATUS status,
    adf_nbuf_t msdu,
    u_int16_t msdu_id)
{
    struct ol_txrx_pdev_t *pdev = txrx_pdev;
    struct ol_tx_desc_t *tx_desc;

    tx_desc = ol_tx_desc_find(pdev, msdu_id);
    adf_os_assert(tx_desc);
    ol_tx_desc_frame_free_nonstd(pdev, tx_desc, status != A_OK);
#if 0 /* TODO: Advanced feature */
    //ol_tx_dwl_sched(pdev, OL_TX_HL_SCHED_DOWNLOAD_DONE);
adf_os_assert(0);
#endif
}

/*
 * The following macros could have been inline functions too.
 * The only rationale for choosing macros, is to force the compiler to inline
 * the implementation, which cannot be controlled for actual "inline" functions,
 * since "inline" is only a hint to the compiler.
 * In the performance path, we choose to force the inlining, in preference to
 * type-checking offered by the actual inlined functions.
 */
#define ol_tx_msdu_complete_batch(_pdev, _tx_desc, _tx_descs, _status)                          \
        do {                                                                                    \
                TAILQ_INSERT_TAIL(&(_tx_descs), (_tx_desc), tx_desc_list_elem);                 \
        } while (0)
#if !ATH_11AC_TXCOMPACT
#define ol_tx_msdu_complete_single(_pdev, _tx_desc, _netbuf, _lcl_freelist, _tx_desc_last)      \
        do {                                                                                    \
                adf_os_atomic_init(&(_tx_desc)->ref_cnt); /* clear the ref cnt */               \
                adf_nbuf_unmap((_pdev)->osdev, (_netbuf), ADF_OS_DMA_TO_DEVICE);                \
                adf_nbuf_free((_netbuf));                                                       \
                ((union ol_tx_desc_list_elem_t *)(_tx_desc))->next = (_lcl_freelist);           \
                if (adf_os_unlikely(!lcl_freelist)) {                                           \
                    (_tx_desc_last) = (union ol_tx_desc_list_elem_t *)(_tx_desc);               \
                }                                                                               \
                (_lcl_freelist) = (union ol_tx_desc_list_elem_t *)(_tx_desc);                   \
        } while (0)
#else  /*!ATH_11AC_TXCOMPACT*/

#define ol_tx_msdu_complete_single(_pdev, _tx_desc, _netbuf, _lcl_freelist, _tx_desc_last)      \
        do {                                                                                    \
                adf_nbuf_unmap((_pdev)->osdev, (_netbuf), ADF_OS_DMA_TO_DEVICE);                \
                adf_nbuf_free((_netbuf));                                                       \
                ((union ol_tx_desc_list_elem_t *)(_tx_desc))->next = (_lcl_freelist);           \
                if (adf_os_unlikely(!lcl_freelist)) {                                           \
                    (_tx_desc_last) = (union ol_tx_desc_list_elem_t *)(_tx_desc);               \
                }                                                                               \
                (_lcl_freelist) = (union ol_tx_desc_list_elem_t *)(_tx_desc);                   \
        } while (0)


#endif /*!ATH_11AC_TXCOMPACT*/

#if QCA_TX_SINGLE_COMPLETIONS 
    #if QCA_TX_STD_PATH_ONLY
        #define ol_tx_msdu_complete(_pdev, _tx_desc, _tx_descs, _netbuf, _lcl_freelist,         \
                                        _tx_desc_last, _status)                                 \
            ol_tx_msdu_complete_single((_pdev), (_tx_desc), (_netbuf), (_lcl_freelist),         \
                                             _tx_desc_last)
    #else   /* !QCA_TX_STD_PATH_ONLY */
        #define ol_tx_msdu_complete(_pdev, _tx_desc, _tx_descs, _netbuf, _lcl_freelist,         \
                                        _tx_desc_last, _status)                                 \
        do {                                                                                    \
            if (adf_os_likely((_tx_desc)->pkt_type == ol_tx_frm_std)) {                         \
                ol_tx_msdu_complete_single((_pdev), (_tx_desc), (_netbuf), (_lcl_freelist),     \
                                             (_tx_desc_last));                                  \
            } else {                                                                            \
                ol_tx_desc_frame_free_nonstd(                                                   \
                    (_pdev), (_tx_desc), (_status) != htt_tx_status_ok);                        \
            }                                                                                   \
        } while (0)
    #endif  /* !QCA_TX_STD_PATH_ONLY */
#else  /* !QCA_TX_SINGLE_COMPLETIONS */
    #if (QCA_TX_STD_PATH_ONLY)
        #define ol_tx_msdu_complete(_pdev, _tx_desc, _tx_descs, _netbuf, _lcl_freelist,         \
                                        _tx_desc_last, _status)                                 \
            ol_tx_msdus_complete_batch((_pdev), (_tx_desc), (_tx_descs), (_status))
    #else   /* !QCA_TX_STD_PATH_ONLY */
        #define ol_tx_msdu_complete(_pdev, _tx_desc, _tx_descs, _netbuf, _lcl_freelist,         \
                                        _tx_desc_last, _status)                                 \
        do {                                                                                    \
            if (adf_os_likely((_tx_desc)->pkt_type == ol_tx_frm_std)) {                         \
                ol_tx_msdu_complete_batch((_pdev), (_tx_desc), (_tx_descs), (_status));         \
            } else {                                                                            \
                ol_tx_desc_frame_free_nonstd(                                                   \
                    (_pdev), (_tx_desc), (_status) != htt_tx_status_ok);                        \
            }                                                                                   \
        } while (0)
    #endif  /* !QCA_TX_STD_PATH_ONLY */
#endif /* QCA_TX_SINGLE_COMPLETIONS */

/* WARNING: ol_tx_inspect_handler()'s bahavior is similar to that of ol_tx_completion_handler().
 * any change in ol_tx_completion_handler() must be mirrored in ol_tx_inspect_handler().
 */
void
ol_tx_completion_handler(
    ol_txrx_pdev_handle pdev,
    int num_msdus,
    enum htt_tx_status status,
    void *tx_desc_id_iterator)
{
    int i;
    u_int16_t *desc_ids = (u_int16_t *)tx_desc_id_iterator;
    u_int16_t tx_desc_id;
    struct ol_tx_desc_t *tx_desc;
    struct ol_ath_softc_net80211 *scn =
                            (struct ol_ath_softc_net80211 *)pdev->ctrl_pdev;
    uint32_t   byte_cnt = 0;
    union ol_tx_desc_list_elem_t *td_array = pdev->tx_desc.array;
    adf_nbuf_t  netbuf;

    union ol_tx_desc_list_elem_t *lcl_freelist = NULL;
    union ol_tx_desc_list_elem_t *tx_desc_last = NULL;
    ol_tx_desc_list tx_descs;
    TAILQ_INIT(&tx_descs);
    for (i = 0; i < num_msdus; i++) {
        tx_desc_id = desc_ids[i];
        tx_desc = &td_array[tx_desc_id].tx_desc;
        tx_desc->status = status;
        netbuf = tx_desc->netbuf;

        /* Per SDU update of byte count */
        byte_cnt += adf_nbuf_len(netbuf);

#if !QCA_OL_11AC_FAST_PATH && !ATH_11AC_TXCOMPACT
        if (adf_os_atomic_dec_and_test(&tx_desc->ref_cnt))
#endif /* QCA_OL_11AC_FAST_PATH */
        {
            ol_tx_msdu_complete(pdev, tx_desc, tx_descs, netbuf, lcl_freelist,
                                    tx_desc_last, status);
        }
    }
    if (scn->scn_stats.ap_stats_tx_cal_enable) {
        struct ol_txrx_peer_t *peer = NULL;
        struct ieee80211vap *vap = NULL;
        struct ieee80211_node *ni = NULL;
        struct ieee80211_mac_stats *mac_stats = NULL;
        struct ieee80211com *ic = &scn->sc_ic;
        switch (status) {
        case htt_tx_status_ok:
            peer = (pdev->tx_stats.peer_id == HTT_INVALID_PEER) ?
                        NULL : pdev->peer_id_to_obj_map[pdev->tx_stats.peer_id];
            if (peer) {
                vap = ol_ath_vap_get(scn,
                HTT_TX_DESC_VDEV_ID_GET(*((u_int32_t *)(tx_desc->htt_tx_desc))));
                mac_stats = peer->bss_peer ? &vap->iv_multicast_stats :
                                             &vap->iv_unicast_stats;
                mac_stats->ims_tx_data_packets += num_msdus;
                mac_stats->ims_tx_data_bytes += byte_cnt;
                mac_stats->ims_tx_datapyld_bytes += byte_cnt -
                                                (num_msdus * ETHERNET_HDR_LEN);
                ni = ieee80211_find_node(&ic->ic_sta, peer->mac_addr.raw);
                if (!ni) {
                   adf_os_print("\n Could not find the peer \n");
                   return;
                }

                if (peer->bss_peer) {
                    ni->ni_stats.ns_tx_mcast += num_msdus;
                } else {
                    ni->ni_stats.ns_tx_ucast += num_msdus;
                }
                ni->ni_stats.ns_tx_data_success += num_msdus;
                ni->ni_stats.ns_tx_bytes_success += byte_cnt;
                ieee80211_free_node(ni);
            }
            pdev->tx_stats.peer_id = HTT_INVALID_PEER;
        break;
        case htt_tx_status_discard:
            peer = (pdev->tx_stats.peer_id == HTT_INVALID_PEER) ?
                        NULL : pdev->peer_id_to_obj_map[pdev->tx_stats.peer_id];
            if (peer) {
                vap = ol_ath_vap_get(scn,
                    HTT_TX_DESC_VDEV_ID_GET(*((u_int32_t *)(tx_desc->htt_tx_desc))));
                vap->iv_unicast_stats.ims_tx_discard += num_msdus;
            }
        break;
        default:
        break;
        }
    }
    /* One shot protected access to pdev freelist, when setup */
    if (lcl_freelist) {
        adf_os_spin_lock(&pdev->tx_mutex);
        tx_desc_last->next = pdev->tx_desc.freelist;
        pdev->tx_desc.freelist = lcl_freelist; 
        adf_os_spin_unlock(&pdev->tx_mutex);
#if QCA_OL_11AC_FAST_PATH
        TXRX_STATS_SUB(pdev, pub.tx.desc_in_use, num_msdus);
#endif /* QCA_OL_11AC_FAST_PATH */
    } else {
        ol_tx_desc_frame_list_free(pdev, &tx_descs, status != htt_tx_status_ok);
    }

    adf_os_atomic_add(num_msdus, &pdev->target_tx_credit);

    /* Do one shot statistics */
    TXRX_STATS_UPDATE_TX_STATS(pdev, status, num_msdus, byte_cnt);
}

/* WARNING: ol_tx_inspect_handler()'s bahavior is similar to that of 
 * ol_tx_completion_handler(). any change in ol_tx_completion_handler() must be
 * mirrored here.
 */
void
ol_tx_inspect_handler(
    ol_txrx_pdev_handle pdev,
    int num_msdus,
    void *tx_desc_id_iterator)
{
    u_int16_t vdev_id, i;
    struct ol_txrx_vdev_t *vdev;
    u_int16_t *desc_ids = (u_int16_t *)tx_desc_id_iterator;
    u_int16_t tx_desc_id;
    struct ol_tx_desc_t *tx_desc;
    union ol_tx_desc_list_elem_t *td_array = pdev->tx_desc.array;
    union ol_tx_desc_list_elem_t *lcl_freelist = NULL;
    union ol_tx_desc_list_elem_t *tx_desc_last = NULL;
    adf_nbuf_t  netbuf;
    ol_tx_desc_list tx_descs;
    TAILQ_INIT(&tx_descs);

    for (i = 0; i < num_msdus; i++) {
        tx_desc_id = desc_ids[i];
        tx_desc = &td_array[tx_desc_id].tx_desc;
        netbuf = tx_desc->netbuf;

        /* find the "vdev" this tx_desc belongs to */
        vdev_id = HTT_TX_DESC_VDEV_ID_GET(*((u_int32_t *)(tx_desc->htt_tx_desc)));
        TAILQ_FOREACH(vdev, &pdev->vdev_list, vdev_list_elem) {
            if (vdev->vdev_id == vdev_id)
                break;
        }

        /* vdev now points to the vdev for this descriptor. */

#if UMAC_SUPPORT_PROXY_ARP || UMAC_SUPPORT_NAWDS || WDS_VENDOR_EXTENSION
        {
            struct ol_txrx_peer_t *peer;
            adf_nbuf_t  netbuf_copy;
#if WDS_VENDOR_EXTENSION
            int num_peers_3addr = 0, is_mcast = 0, is_ucast = 0;
            struct ether_header *eth_hdr = (struct ether_header *)(adf_nbuf_data(netbuf));
#define IS_MULTICAST(_a) (*(_a) & 0x01)
            is_mcast = (IS_MULTICAST(eth_hdr->ether_dhost)) ? 1 : 0;
            is_ucast = !is_mcast;
#undef IS_MULTICAST

            TAILQ_FOREACH(peer, &vdev->peer_list, peer_list_elem) {
                if (peer->bss_peer || (peer->peer_ids[0] == HTT_INVALID_PEER))
                    continue;

                /* count wds peers that use 3-addr framing for mcast.
                 * if there are any, the bss_peer is used to send the 
                 * the mcast frame using 3-addr format. all wds enabled
                 * peers that use 4-addr framing for mcast frames will
                 * be duplicated and sent as 4-addr frames below.
                 */
                if (!peer->wds_enabled || !peer->wds_tx_mcast_4addr)
                    num_peers_3addr ++;
            }
#endif
            TAILQ_FOREACH(peer, &vdev->peer_list, peer_list_elem) {
                if (peer) {
                    /* 
                     * osif_proxy_arp should always return true for NAWDS to work properly. 
                     */
                    if((peer->peer_ids[0] != HTT_INVALID_PEER) && 
#if WDS_VENDOR_EXTENSION
                            ((peer->bss_peer && num_peers_3addr) || 
                             (peer->wds_enabled && 
                              ((is_mcast && peer->wds_tx_mcast_4addr) || 
                               (is_ucast && peer->wds_tx_ucast_4addr))))
#else
                            (peer->bss_peer || peer->nawds_enabled)
#endif
#if UMAC_SUPPORT_PROXY_ARP
                            && !(vdev->osif_proxy_arp(vdev->osif_vdev, netbuf))
#endif
                            ) {
                        /* re-inject multicast packet back to target by copying.
                         * get a new descriptor and send this packet.
                         * TODO: optimize this code to not use skb_copy
                         */
                        netbuf_copy = adf_nbuf_copy(netbuf);
                        if (netbuf_copy) {
                            adf_nbuf_reset_ctxt(netbuf_copy);
                            adf_nbuf_map_single(pdev->osdev, netbuf_copy, ADF_OS_DMA_TO_DEVICE);
                            ol_tx_reinject(vdev, netbuf_copy, peer->peer_ids[0]);
                        }
                    }//proxy arp or nawds_enabled if check
                }//peer NULL check
            }//TAILQ_FOREACH
        }
#endif // PROXY_ARP or NAWDS or WDS_VENDOR_EXTENSION
#if !ATH_11AC_TXCOMPACT        
        /* save this multicast packet to local free list */
        if (adf_os_atomic_dec_and_test(&tx_desc->ref_cnt)) 
#endif            
        {
            /* for this function only, force htt status to be "htt_tx_status_ok" 
             * for graceful freeing of this multicast frame
             */
            ol_tx_msdu_complete(pdev, tx_desc, tx_descs, netbuf, lcl_freelist,
                                    tx_desc_last, htt_tx_status_ok);
        }
    }

    if (lcl_freelist) {
        adf_os_spin_lock(&pdev->tx_mutex);
        tx_desc_last->next = pdev->tx_desc.freelist;
        pdev->tx_desc.freelist = lcl_freelist; 
        adf_os_spin_unlock(&pdev->tx_mutex);
    } else {
        ol_tx_desc_frame_list_free(pdev, &tx_descs, htt_tx_status_discard);
    }
    adf_os_atomic_add(num_msdus, &pdev->target_tx_credit);

    return;
}

