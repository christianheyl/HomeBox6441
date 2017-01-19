/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#include <adf_nbuf.h>          /* adf_nbuf_t, etc. */
#include <adf_os_io.h>         /* adf_os_cpu_to_le64 */
#include <ieee80211.h>         /* ieee80211_frame */
#include <ieee80211_var.h>     /* IEEE80211_ADDR_COPY */ 
#include <net.h>               /* ETHERTYPE_IPV4, etc. */

/* external API header files */
#include <ol_ctrl_txrx_api.h>  /* ol_rx_notify */
#include <ol_htt_api.h>        /* htt_pdev_handle */
#include <ol_txrx_api.h>       /* ol_txrx_pdev_handle */
#include <ol_txrx_htt_api.h>   /* ol_rx_indication_handler */
#include <ol_htt_rx_api.h>     /* htt_rx_peer_id, etc. */

/* internal API header files */
#include <ol_txrx_types.h>     /* ol_txrx_vdev_t, etc. */
#include <ol_txrx_peer_find.h> /* ol_txrx_peer_find_by_id */
#include <ol_rx_reorder.h>     /* ol_rx_reorder_store, etc. */
#include <ol_rx_defrag.h>      /* ol_rx_defrag_waitlist_flush */
#include <ol_txrx_internal.h>
#include <wdi_event.h>

#include <htt_types.h>
#include <ol_if_athvar.h>
#include <net.h>
#include <ol_vowext_dbg_defs.h>

static int ol_rx_monitor_deliver(
    ol_txrx_pdev_handle pdev,
    adf_nbuf_t head_msdu,
    u_int32_t no_clone_reqd)
{
    adf_nbuf_t mon_mpdu = NULL;
    htt_pdev_handle htt_pdev = pdev->htt_pdev;

    if (pdev->monitor_vdev == NULL) {
        return -1;
    }

    /* Cloned mon MPDU for delivery via monitor intf */
    mon_mpdu = htt_rx_restitch_mpdu_from_msdus(
        htt_pdev, head_msdu, pdev->rx_mon_recv_status, no_clone_reqd);

    if (mon_mpdu) {
        pdev->monitor_vdev->osif_rx_mon(
            pdev->monitor_vdev->osif_vdev, mon_mpdu, pdev->rx_mon_recv_status);
    }
    return 0;
}

static void ol_rx_process_inv_peer(
    ol_txrx_pdev_handle pdev,
    void *rx_mpdu_desc,
    adf_nbuf_t msdu
    )
{
    a_uint8_t a1[IEEE80211_ADDR_LEN];
    htt_pdev_handle htt_pdev = pdev->htt_pdev;
    struct ol_txrx_vdev_t *vdev = NULL;
    struct ieee80211_frame *wh;
    struct wdi_event_rx_peer_invalid_msg msg;

    wh = (struct ieee80211_frame *)htt_rx_mpdu_wifi_hdr_retrieve(htt_pdev, rx_mpdu_desc);
    if (!IEEE80211_IS_DATA(wh))
        return;

    /* ignore frames for non-existent bssids */
    IEEE80211_ADDR_COPY(a1, wh->i_addr1);
    TAILQ_FOREACH(vdev, &pdev->vdev_list, vdev_list_elem) {
        if (adf_os_mem_cmp(a1, vdev->mac_addr.raw, IEEE80211_ADDR_LEN) == 0) {
            break;
        }
    }
    if (!vdev) 
        return;

    msg.wh = wh;
    msg.msdu = msdu;
    msg.vdev_id = vdev->vdev_id;
    wdi_event_handler(WDI_EVENT_RX_PEER_INVALID, pdev, &msg, NULL, WDI_NO_VAL);
}

#if WDS_VENDOR_EXTENSION
static inline int wds_rx_policy_check(
    htt_pdev_handle htt_pdev, 
    struct ol_txrx_vdev_t *vdev, 
    struct ol_txrx_peer_t *peer, 
    void *rx_mpdu_desc,
    int rx_mcast
    )
    
{
    struct ol_txrx_peer_t *bss_peer;
    int fr_ds, to_ds, rx_3addr, rx_4addr;
    int rx_policy_ucast, rx_policy_mcast;


    if (vdev->opmode == wlan_op_mode_ap) {
        TAILQ_FOREACH(bss_peer, &vdev->peer_list, peer_list_elem) {
            if (bss_peer->bss_peer) {
                /* if wds policy check is not enabled on this vdev, accept all frames */
                if (!bss_peer->wds_rx_filter) {
                    return 1;
                }
                break;
            }
        }
        rx_policy_ucast = bss_peer->wds_rx_ucast_4addr;
        rx_policy_mcast = bss_peer->wds_rx_mcast_4addr;
    }
    else {             /* sta mode */
        if (!peer->wds_rx_filter) {
            return 1;
        }
        rx_policy_ucast = peer->wds_rx_ucast_4addr;
        rx_policy_mcast = peer->wds_rx_mcast_4addr;
    }

/* ------------------------------------------------
 *                       self
 * peer-             rx  rx-
 * wds  ucast mcast dir policy accept note
 * ------------------------------------------------
 * 1     1     0     11  x1     1      AP configured to accept ds-to-ds Rx ucast from wds peers, constraint met; so, accept
 * 1     1     0     01  x1     0      AP configured to accept ds-to-ds Rx ucast from wds peers, constraint not met; so, drop
 * 1     1     0     10  x1     0      AP configured to accept ds-to-ds Rx ucast from wds peers, constraint not met; so, drop
 * 1     1     0     00  x1     0      bad frame, won't see it
 * 1     0     1     11  1x     1      AP configured to accept ds-to-ds Rx mcast from wds peers, constraint met; so, accept
 * 1     0     1     01  1x     0      AP configured to accept ds-to-ds Rx mcast from wds peers, constraint not met; so, drop
 * 1     0     1     10  1x     0      AP configured to accept ds-to-ds Rx mcast from wds peers, constraint not met; so, drop
 * 1     0     1     00  1x     0      bad frame, won't see it
 * 1     1     0     11  x0     0      AP configured to accept from-ds Rx ucast from wds peers, constraint not met; so, drop
 * 1     1     0     01  x0     0      AP configured to accept from-ds Rx ucast from wds peers, constraint not met; so, drop
 * 1     1     0     10  x0     1      AP configured to accept from-ds Rx ucast from wds peers, constraint met; so, accept
 * 1     1     0     00  x0     0      bad frame, won't see it
 * 1     0     1     11  0x     0      AP configured to accept from-ds Rx mcast from wds peers, constraint not met; so, drop
 * 1     0     1     01  0x     0      AP configured to accept from-ds Rx mcast from wds peers, constraint not met; so, drop
 * 1     0     1     10  0x     1      AP configured to accept from-ds Rx mcast from wds peers, constraint met; so, accept
 * 1     0     1     00  0x     0      bad frame, won't see it
 *
 * 0     x     x     11  xx     0      we only accept td-ds Rx frames from non-wds peers in mode.
 * 0     x     x     01  xx     1      
 * 0     x     x     10  xx     0      
 * 0     x     x     00  xx     0      bad frame, won't see it
 * ------------------------------------------------
 */


    fr_ds = htt_rx_mpdu_desc_frds(htt_pdev, rx_mpdu_desc);
    to_ds = htt_rx_mpdu_desc_tods(htt_pdev, rx_mpdu_desc);
    rx_3addr = fr_ds ^ to_ds;
    rx_4addr = fr_ds & to_ds;


    if (vdev->opmode == wlan_op_mode_ap) {
        /* printk("AP m %d 3A %d 4A %d UP %d MP %d ", rx_mcast, rx_3addr, rx_4addr, rx_policy_ucast, rx_policy_mcast); */
        if ((!peer->wds_enabled && rx_3addr && to_ds) ||
            (peer->wds_enabled && !rx_mcast && (rx_4addr == rx_policy_ucast)) ||
            (peer->wds_enabled && rx_mcast && (rx_4addr == rx_policy_mcast))) {
            /* printk("A\n"); */
            return 1;
        }
    }
    else {           /* sta mode */
        /* printk("STA m %d 3A %d 4A %d UP %d MP %d ", rx_mcast, rx_3addr, rx_4addr, rx_policy_ucast, rx_policy_mcast); */
        if ((!rx_mcast && (rx_4addr == rx_policy_ucast)) ||
            (rx_mcast && (rx_4addr == rx_policy_mcast))) {
            /* printk("A\n"); */
            return 1;
        }
    }

    /* printk("D\n"); */
    return 0;
}
#endif

void
ol_rx_indication_handler(
    ol_txrx_pdev_handle pdev,
    adf_nbuf_t rx_ind_msg,
    u_int16_t peer_id,
    u_int8_t tid,
    int num_mpdu_ranges)
{
    int mpdu_range;
    int seq_num_start = 0, seq_num_end = 0, rx_ind_release = 0;
    struct ol_txrx_vdev_t *vdev = NULL;
    struct ol_txrx_peer_t *peer;
    htt_pdev_handle htt_pdev;

    htt_pdev = pdev->htt_pdev;
    peer = ol_txrx_peer_find_by_id(pdev, peer_id);
    if (peer) {
        vdev = peer->vdev;
    }

    TXRX_STATS_INCR(pdev, priv.rx.normal.ppdus);

    if (htt_rx_ind_flush(rx_ind_msg) && peer) {

        htt_rx_ind_flush_seq_num_range(
            rx_ind_msg, &seq_num_start, &seq_num_end);

        if (tid == HTT_INVALID_TID) {
            /* 
             * host/FW reorder state went out-of sync
             * for a while because FW ran out of Rx indication
             * buffer. We have to discard all the buffers in
             * reorder queue.
             */
            ol_rx_reorder_peer_cleanup(vdev, peer); 
        } else {
            ol_rx_reorder_flush(
                vdev, peer, tid, seq_num_start, 
                seq_num_end, htt_rx_flush_release);
        }
    }

    if (htt_rx_ind_release(rx_ind_msg)) {
        /* the ind info of release is saved here and do release at the end.
         * This is for the reason of in HL case, the adf_nbuf_t for msg and
         * payload are the same buf. And the buf will be changed during 
         * processing */
        rx_ind_release = 1;
        htt_rx_ind_release_seq_num_range(
            rx_ind_msg, &seq_num_start, &seq_num_end);
    }
 
    for (mpdu_range = 0; mpdu_range < num_mpdu_ranges; mpdu_range++) {
        enum htt_rx_status status;
        int i, num_mpdus;
        adf_nbuf_t head_msdu, tail_msdu, msdu;
        void *rx_mpdu_desc;

        htt_rx_ind_mpdu_range_info(
            pdev->htt_pdev, rx_ind_msg, mpdu_range, &status, &num_mpdus);
        if ((status == htt_rx_status_ok) && peer) {
            TXRX_STATS_ADD(pdev, priv.rx.normal.mpdus, num_mpdus);
            /* valid frame - deposit it into the rx reordering buffer */
            for (i = 0; i < num_mpdus; i++) {
                int seq_num, msdu_chaining;
                /*
                 * Get a linked list of the MSDUs that comprise this MPDU.
                 * This also attaches each rx MSDU descriptor to the
                 * corresponding rx MSDU network buffer.
                 * (In some systems, the rx MSDU desc is already in the
                 * same buffer as the MSDU payload; in other systems they
                 * are separate, so a pointer needs to be set in the netbuf
                 * to locate the corresponding rx descriptor.)
                 *
                 * It is neccessary to call htt_rx_amsdu_pop before
                 * htt_rx_mpdu_desc_list_next, because the (MPDU) rx
                 * descriptor has the DMA unmapping done during the
                 * htt_rx_amsdu_pop call.  The rx desc should not be
                 * accessed until this DMA unmapping has been done,
                 * since the DMA unmapping involves making sure the
                 * cache area for the mapped buffer is flushed, so the
                 * data written by the MAC DMA into memory will be
                 * fetched, rather than garbage from the cache.
                 */
                msdu_chaining = htt_rx_amsdu_pop(
                    htt_pdev, rx_ind_msg, &head_msdu, &tail_msdu);
                rx_mpdu_desc = 
                    htt_rx_mpdu_desc_list_next(htt_pdev, rx_ind_msg);

                /* Pktlog */
                wdi_event_handler(WDI_EVENT_RX_DESC_REMOTE, pdev, head_msdu, 
                                    peer_id, status);
                ol_rx_monitor_deliver(pdev, head_msdu, 0);
                if (msdu_chaining) {
                    /*
                     * TBDXXX - to deliver SDU with chaining, we need to
                     * stitch those scattered buffers into one single buffer.
                     * Just discard it now.
                     */
		           while (1) {
                        adf_nbuf_t next;
                        next = adf_nbuf_next(head_msdu);
                        htt_rx_desc_frame_free(htt_pdev, head_msdu);
                        if (head_msdu == tail_msdu) {
                            break;
                        }
                        head_msdu = next;
                    }
                } else {
#if WDS_VENDOR_EXTENSION
                    int accept = 1, rx_mcast;
                    void *msdu_desc;
#endif

                    seq_num = htt_rx_mpdu_desc_seq_num(htt_pdev, rx_mpdu_desc);
                    OL_RX_REORDER_TRACE_ADD(pdev, tid, seq_num, 1);
#if HTT_DEBUG_DATA
                    HTT_PRINT("t%ds%d ",tid, seq_num);
#endif
#if WDS_VENDOR_EXTENSION
                    msdu_desc = htt_rx_msdu_desc_retrieve(htt_pdev, head_msdu);
                    if (vdev->opmode == wlan_op_mode_ap ||
                        vdev->opmode == wlan_op_mode_sta) {
                        if (vdev->opmode == wlan_op_mode_ap) {
                            rx_mcast = (htt_rx_msdu_forward(htt_pdev, msdu_desc) && !htt_rx_msdu_discard(htt_pdev, msdu_desc));
                        }
                        else {
                            rx_mcast = htt_rx_msdu_is_wlan_mcast(htt_pdev, msdu_desc);
                        }
                        /* wds rx policy check only applies to vdevs in ap/sta mode */
                        accept = wds_rx_policy_check(htt_pdev, vdev, peer, rx_mpdu_desc, rx_mcast);
                    }
#endif

                    if (
#if WDS_VENDOR_EXTENSION
                        !accept ||
#endif
                        ol_rx_reorder_store(pdev, peer, tid, seq_num,
                                head_msdu, tail_msdu) 
                                ) {

                        /* free the mpdu if it could not be stored, e.g.
                         * duplicate non-aggregate mpdu detected */
                        while (1) {
                            adf_nbuf_t next;
                            next = adf_nbuf_next(head_msdu);
                            htt_rx_desc_frame_free(htt_pdev, head_msdu);
                            if (head_msdu == tail_msdu) {
                                break;
                            }
                            head_msdu = next;
                        }

                        /*
                         * ol_rx_reorder_store should only fail for the case
                         * of (duplicate) non-aggregates.
                         *
                         * Confirm this is a non-aggregate
                         */
                        TXRX_ASSERT2(num_mpdu_ranges == 1 && num_mpdus == 1);

                        /* the MPDU was not stored in the rx reorder array,
                         * so there's nothing to release */
                        rx_ind_release = 0;
                    }
                }
            }
        } else {
            /* invalid frames - discard them */
            OL_RX_REORDER_TRACE_ADD(
                pdev, tid, TXRX_SEQ_NUM_ERR(status), num_mpdus);
            TXRX_STATS_ADD(pdev, priv.rx.err.mpdu_bad, num_mpdus);
            for (i = 0; i < num_mpdus; i++) {
                /* pull the MPDU's MSDUs off the buffer queue */
                htt_rx_amsdu_pop(htt_pdev, rx_ind_msg, &msdu, &tail_msdu);
                /* pull the MPDU desc off the desc queue */
                rx_mpdu_desc = 
                    htt_rx_mpdu_desc_list_next(htt_pdev, rx_ind_msg);
                
		        if (status == htt_rx_status_tkip_mic_err && vdev != NULL &&  peer != NULL) {
                    ol_rx_err(pdev->ctrl_pdev, vdev->vdev_id, peer->mac_addr.raw,
                        tid,0, OL_RX_ERR_TKIP_MIC, msdu);
                }

                wdi_event_handler(WDI_EVENT_RX_DESC_REMOTE,
                                        pdev, msdu, peer_id, status);

                if (status == htt_rx_status_err_inv_peer) {
                    /* once per mpdu */
                    ol_rx_process_inv_peer(pdev, rx_mpdu_desc, msdu);
                }
                if (ol_rx_monitor_deliver(pdev, msdu, 1)) {
                    /* Free the nbuf iff monitor layer did not absorb */
                    while (1) {
                        adf_nbuf_t next;
                        next = adf_nbuf_next(msdu);
                        htt_rx_desc_frame_free(htt_pdev, msdu);
                        if (msdu == tail_msdu) {
                            break;
                        }
                        msdu = next;
                    }
                }
            }
        }
    }
    /*
     * Now that a whole batch of MSDUs have been pulled out of HTT
     * and put into the rx reorder array, it is an appropriate time
     * to request HTT to provide new rx MSDU buffers for the target
     * to fill.
     * This could be done after the end of this function, but it's
     * better to do it now, rather than waiting until after the driver
     * and OS finish processing the batch of rx MSDUs.
     */
    htt_rx_msdu_buff_replenish(htt_pdev);

    if (rx_ind_release && peer) {
#if HTT_DEBUG_DATA
        HTT_PRINT("t%dr[%d,%d)\n",tid, seq_num_start, seq_num_end);
#endif
        ol_rx_reorder_release(vdev, peer, tid, seq_num_start, seq_num_end);
    }

    if (pdev->rx.flags.defrag_timeout_check) {
        ol_rx_defrag_waitlist_flush(pdev);
    }
}

void
ol_rx_sec_ind_handler(
    ol_txrx_pdev_handle pdev,
    u_int16_t peer_id,
    enum htt_sec_type sec_type,
    int is_unicast,
    u_int32_t *michael_key,
    u_int32_t *rx_pn)
{
    struct ol_txrx_peer_t *peer;
    int sec_index, i;

    peer = ol_txrx_peer_find_by_id(pdev, peer_id);
    if (! peer) {
        TXRX_PRINT(TXRX_PRINT_LEVEL_ERR,
            "Couldn't find peer from ID %d - skipping security inits\n",
            peer_id);
        return;
    }
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1,
        "sec spec for peer %p (%02x:%02x:%02x:%02x:%02x:%02x): "
        "%s key of type %d\n",
        peer,
        peer->mac_addr.raw[0], peer->mac_addr.raw[1], peer->mac_addr.raw[2],
        peer->mac_addr.raw[3], peer->mac_addr.raw[4], peer->mac_addr.raw[5],
        is_unicast ? "ucast" : "mcast",
        sec_type);
    sec_index = is_unicast ? txrx_sec_ucast : txrx_sec_mcast;
    peer->security[sec_index].sec_type = sec_type;
    /* michael key only valid for TKIP, but for simplicity, copy it anyway */
    adf_os_mem_copy(
        &peer->security[sec_index].michael_key[0],
        michael_key,
        sizeof(peer->security[sec_index].michael_key));
 
    if (sec_type != htt_sec_type_wapi) {
        adf_os_mem_set(peer->tids_last_pn_valid, 0x00, OL_TXRX_NUM_EXT_TIDS);
    } else {
        for (i = 0; i < OL_TXRX_NUM_EXT_TIDS; i++) {
            /*
             * Setting PN valid bit for WAPI sec_type,
             * since WAPI PN has to be started with predefined value
             */
            peer->tids_last_pn_valid[i] = 1;
            adf_os_mem_copy(
                (u_int8_t *) &peer->tids_last_pn[i],
                (u_int8_t *) rx_pn, sizeof(union htt_rx_pn_t));
            peer->tids_last_pn[i].pn128[1] =
                adf_os_cpu_to_le64(peer->tids_last_pn[i].pn128[1]);
            peer->tids_last_pn[i].pn128[0] =
                adf_os_cpu_to_le64(peer->tids_last_pn[i].pn128[0]);
        }
    }
}

#if defined(PERE_IP_HDR_ALIGNMENT_WAR)

#include <ieee80211.h>

static void
transcap_nwifi_to_8023(adf_nbuf_t msdu)
{
    struct ieee80211_frame *wh;
    a_uint32_t hdrsize;
    struct llc *llchdr;
    struct ether_header *eth_hdr;
    a_uint16_t ether_type = 0;
    a_uint8_t a1[IEEE80211_ADDR_LEN], a2[IEEE80211_ADDR_LEN], a3[IEEE80211_ADDR_LEN];
    a_uint8_t fc1;

    wh = (struct ieee80211_frame *)adf_nbuf_data(msdu);
    IEEE80211_ADDR_COPY(a1, wh->i_addr1);
    IEEE80211_ADDR_COPY(a2, wh->i_addr2);
    IEEE80211_ADDR_COPY(a3, wh->i_addr3);
    fc1 = wh->i_fc[1] & IEEE80211_FC1_DIR_MASK;
    /* Native Wifi header is 80211 non-QoS header */
    hdrsize = sizeof(struct ieee80211_frame);

    llchdr = (struct llc *)(((a_uint8_t *)adf_nbuf_data(msdu)) + hdrsize);
    ether_type = llchdr->llc_un.type_snap.ether_type;

    /* 
     * Now move the data pointer to the beginning of the mac header : 
     * new-header = old-hdr + (wifhdrsize + llchdrsize - ethhdrsize) 
     */
    adf_nbuf_pull_head(
        msdu, (hdrsize + sizeof(struct llc) - sizeof(struct ether_header)));
    eth_hdr = (struct ether_header *)(adf_nbuf_data(msdu));
    switch (fc1) {
    case IEEE80211_FC1_DIR_NODS:
        IEEE80211_ADDR_COPY(eth_hdr->ether_dhost, a1);
        IEEE80211_ADDR_COPY(eth_hdr->ether_shost, a2);
        break;
    case IEEE80211_FC1_DIR_TODS:
        IEEE80211_ADDR_COPY(eth_hdr->ether_dhost, a3);
        IEEE80211_ADDR_COPY(eth_hdr->ether_shost, a2);
        break;
    case IEEE80211_FC1_DIR_FROMDS:
        IEEE80211_ADDR_COPY(eth_hdr->ether_dhost, a1);
        IEEE80211_ADDR_COPY(eth_hdr->ether_shost, a3);
        break;
    case IEEE80211_FC1_DIR_DSTODS:
        break;
    }
    eth_hdr->ether_type = ether_type;
}
#endif

/**
 * @brief Look into a rx MSDU to see what kind of special handling it requires
 * @details
 *      This function is called when the host rx SW sees that the target
 *      rx FW has marked a rx MSDU as needing inspection.
 *      Based on the results of the inspection, the host rx SW will infer
 *      what special handling to perform on the rx frame.
 *      Currently, the only type of frames that require special handling
 *      are IGMP frames.  The rx data-path SW checks if the frame is IGMP
 *      (it should be, since the target would not have set the inspect flag
 *      otherwise), and then calls the ol_rx_notify function so the
 *      control-path SW can perform multicast group membership learning
 *      by sniffing the IGMP frame.
 */
#define SIZEOF_80211_HDR (sizeof(struct ieee80211_frame))
void
ol_rx_inspect(
    struct ol_txrx_vdev_t *vdev,
    struct ol_txrx_peer_t *peer,
    unsigned tid,
    adf_nbuf_t msdu,
    void *rx_desc)
{
    ol_txrx_pdev_handle pdev = vdev->pdev;
    u_int8_t *data, *l3_hdr;
    u_int16_t ethertype;
    int offset;

    data = adf_nbuf_data(msdu);
    if (pdev->frame_format == wlan_frm_fmt_native_wifi) {
        offset = SIZEOF_80211_HDR + LLC_SNAP_HDR_OFFSET_ETHERTYPE;
        l3_hdr = data + SIZEOF_80211_HDR + LLC_SNAP_HDR_LEN;
    } else {
        offset = ETHERNET_ADDR_LEN * 2;
        l3_hdr = data + ETHERNET_HDR_LEN;
    }
    ethertype = (data[offset] << 8) | data[offset+1];
    if (ethertype == ETHERTYPE_IPV4) {
        offset = IPV4_HDR_OFFSET_PROTOCOL;
        if (l3_hdr[offset] == IP_PROTOCOL_IGMP) {
            ol_rx_notify(
                pdev->ctrl_pdev,
                vdev->vdev_id, 
                peer->mac_addr.raw, 
                tid,
                htt_rx_mpdu_desc_tsf32(pdev->htt_pdev, rx_desc),
                OL_RX_NOTIFY_IPV4_IGMP, 
                msdu);
        }
    }
}

void
ol_rx_deliver(
    struct ol_txrx_vdev_t *vdev,
    struct ol_txrx_peer_t *peer,
    unsigned tid,
    adf_nbuf_t msdu_list)
{
    ol_txrx_pdev_handle pdev = vdev->pdev;
    htt_pdev_handle htt_pdev = pdev->htt_pdev;
    adf_nbuf_t deliver_list_head = NULL;
    adf_nbuf_t deliver_list_tail = NULL;
    adf_nbuf_t msdu;

    msdu = msdu_list;
    /*
     * Check each MSDU to see whether it requires special handling,
     * and free each MSDU's rx descriptor
     */
    while (msdu) {
        void *rx_desc;
        int discard, inspect, dummy_fwd;
        adf_nbuf_t next = adf_nbuf_next(msdu);

        rx_desc = htt_rx_msdu_desc_retrieve(pdev->htt_pdev, msdu);
        // for HL, point to payload right now
        if (pdev->cfg.is_high_latency) {
            adf_nbuf_pull_head(msdu,
                    htt_rx_msdu_rx_desc_size_hl(htt_pdev,
                        rx_desc));
        }

        htt_rx_msdu_actions(
            pdev->htt_pdev, rx_desc, &discard, &dummy_fwd, &inspect);
        if (inspect) {
            ol_rx_inspect(vdev, peer, tid, msdu, rx_desc);
        }

        htt_rx_msdu_desc_free(htt_pdev, msdu);
        if (discard) {
#if HTT_DEBUG_DATA
            HTT_PRINT("-\n");
            if (tid == 0) {
                if (adf_nbuf_data(msdu)[0x17] == 0x6) {
                    // TCP, dump seq
                    HTT_PRINT("0x%02x 0x%02x 0x%02x 0x%02x\n",
                            adf_nbuf_data(msdu)[0x26],
                            adf_nbuf_data(msdu)[0x27],
                            adf_nbuf_data(msdu)[0x28],
                            adf_nbuf_data(msdu)[0x29]);
                } else {
                    HTT_PRINT_BUF(printk, adf_nbuf_data(msdu), 
                            0x41,__func__); 
                }
            }

#endif
            adf_nbuf_free(msdu);
        } else {
            TXRX_STATS_MSDU_INCR(vdev->pdev, rx.delivered, msdu);
            OL_TXRX_PROT_AN_LOG(vdev->pdev->prot_an_rx_sent, msdu);
            OL_TXRX_LIST_APPEND(deliver_list_head, deliver_list_tail, msdu);
        }
        msdu = next;
    }
    /* sanity check - are there any frames left to give to the OS shim? */
    if (!deliver_list_head) {
        return;
    }

#if defined(PERE_IP_HDR_ALIGNMENT_WAR)
    if (pdev->host_80211_enable) {
        for (msdu = deliver_list_head; msdu; msdu = adf_nbuf_next(msdu)) {
            transcap_nwifi_to_8023(msdu);
        }
    }
#endif

#if HTT_DEBUG_DATA
    /*
     * we need a place to dump rx data buf
     * to host stack, here's a good place
     */
    for (msdu = deliver_list_head; msdu; msdu = adf_nbuf_next(msdu)) {
        if (tid == 0) {
            if (adf_nbuf_data(msdu)[0x17] == 0x6) {
                // TCP, dump seq
                HTT_PRINT("0x%02x 0x%02x 0x%02x 0x%02x\n",
                        adf_nbuf_data(msdu)[0x26],
                        adf_nbuf_data(msdu)[0x27],
                        adf_nbuf_data(msdu)[0x28],
                        adf_nbuf_data(msdu)[0x29]);
            } else {
                HTT_PRINT_BUF(printk, adf_nbuf_data(msdu), 
                        0x41,__func__);
            }
        }
    }
#endif
    vdev->osif_rx(vdev->osif_vdev, deliver_list_head);
}

void
ol_rx_discard(
    struct ol_txrx_vdev_t *vdev,
    struct ol_txrx_peer_t *peer,
    unsigned tid,
    adf_nbuf_t msdu_list)
{
    ol_txrx_pdev_handle pdev = vdev->pdev;
    htt_pdev_handle htt_pdev = pdev->htt_pdev;

    while (msdu_list) {
        adf_nbuf_t msdu = msdu_list;

        msdu_list = adf_nbuf_next(msdu_list);
        TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1,
            "discard rx %p from partly-deleted peer %p " 
            "(%02x:%02x:%02x:%02x:%02x:%02x)\n",
            msdu, peer,
            peer->mac_addr.raw[0], peer->mac_addr.raw[1],
            peer->mac_addr.raw[2], peer->mac_addr.raw[3],
            peer->mac_addr.raw[4], peer->mac_addr.raw[5]);
        htt_rx_desc_frame_free(htt_pdev, msdu);
    }
}

void
ol_rx_peer_init(struct ol_txrx_pdev_t *pdev, struct ol_txrx_peer_t *peer)
{
    int tid;
    for (tid = 0; tid < OL_TXRX_NUM_EXT_TIDS; tid++) {
        ol_rx_reorder_init(&peer->tids_rx_reorder[tid], tid);

        /* invalid sequence number */
        peer->tids_last_seq[tid] = 0xffff;
    }
    /*
     * Set security defaults: no PN check, no security.
     * The target may send a HTT SEC_IND message to overwrite these defaults.
     */
    peer->security[txrx_sec_ucast].sec_type =
        peer->security[txrx_sec_mcast].sec_type = htt_sec_type_none;
}

void
ol_rx_peer_cleanup(struct ol_txrx_vdev_t *vdev, struct ol_txrx_peer_t *peer)
{
    ol_rx_reorder_peer_cleanup(vdev, peer);
}

/*
 * Free frames including both rx descriptors and buffers
 */
void 
ol_rx_frames_free(
    htt_pdev_handle htt_pdev, 
    adf_nbuf_t frames)
{
    adf_nbuf_t next, frag = frames;

    while (frag) {
        next = adf_nbuf_next(frag);
        htt_rx_desc_frame_free(htt_pdev, frag);
        frag = next;
    }
}

/**
 * @brief populates vow ext stats in given network buffer.
 * @param msdu - network buffer handle 
 * @param pdev - handle to htt dev.
 */
void ol_ath_add_vow_extstats(htt_pdev_handle pdev, adf_nbuf_t msdu)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)pdev->ctrl_pdev;

    if (scn->vow_extstats == 0) {
        return;
    } else {
        u_int8_t *data, *l3_hdr, *bp;
        u_int16_t ethertype;
        int offset;
        struct vow_extstats vowstats; 

        data = adf_nbuf_data(msdu);

        offset = ETHERNET_ADDR_LEN * 2;
        l3_hdr = data + ETHERNET_HDR_LEN;
        ethertype = (data[offset] << 8) | data[offset+1];
        if (ethertype == ETHERTYPE_IPV4) {
            offset = IPV4_HDR_OFFSET_PROTOCOL;
            if ((l3_hdr[offset] == IP_PROTOCOL_UDP) && (l3_hdr[0] == IP_VER4_N_NO_EXTRA_HEADERS)) {
                bp = data+EXT_HDR_OFFSET;

                if ( (data[RTP_HDR_OFFSET] == UDP_PDU_RTP_EXT) && 
                        (bp[0] == 0x12) && 
                        (bp[1] == 0x34) && 
                        (bp[2] == 0x00) && 
                        (bp[3] == 0x08)) {
                    /* clear udp checksum so we do not have to recalculate it after
                     * filling in status fields */
                    data[UDP_CKSUM_OFFSET] = 0;
                    data[(UDP_CKSUM_OFFSET+1)] = 0;

                    bp += IPERF3_DATA_OFFSET;

                    htt_rx_get_vowext_stats(msdu, &vowstats);

                    /* control channel RSSI */
                    *bp++ = vowstats.rx_rssi_ctl0;
                    *bp++ = vowstats.rx_rssi_ctl1;
                    *bp++ = vowstats.rx_rssi_ctl2;

                    /* rx rate info */
                    *bp++ = vowstats.rx_bw; 
                    *bp++ = vowstats.rx_sgi; 
                    *bp++ = vowstats.rx_nss;

                    *bp++ = vowstats.rx_rssi_comb;
                    *bp++ = vowstats.rx_rs_flags; /* rsflags */

                    /* Time stamp Lo*/   
                    *bp++ = (u_int8_t)((vowstats.rx_macTs & 0x0000ff00) >> 8);                    
                    *bp++ = (u_int8_t)(vowstats.rx_macTs & 0x000000ff);
                    /* rx phy errors */
                    *bp++ = (u_int8_t)((scn->chan_stats.phy_err_cnt >> 8) & 0xff);
                    *bp++ = (u_int8_t)(scn->chan_stats.phy_err_cnt & 0xff);
                    /* rx clear count */
                    *bp++ = (u_int8_t)((scn->mib_cycle_cnts.rx_clear_count >> 24) & 0xff);
                    *bp++ = (u_int8_t)((scn->mib_cycle_cnts.rx_clear_count >> 16) & 0xff);
                    *bp++ = (u_int8_t)((scn->mib_cycle_cnts.rx_clear_count >>  8) & 0xff);
                    *bp++ = (u_int8_t)(scn->mib_cycle_cnts.rx_clear_count & 0xff);
                    /* rx cycle count */
                    *bp++ = (u_int8_t)((scn->mib_cycle_cnts.cycle_count >> 24) & 0xff);
                    *bp++ = (u_int8_t)((scn->mib_cycle_cnts.cycle_count >> 16) & 0xff);
                    *bp++ = (u_int8_t)((scn->mib_cycle_cnts.cycle_count >>  8) & 0xff);
                    *bp++ = (u_int8_t)(scn->mib_cycle_cnts.cycle_count & 0xff);

                    *bp++ = vowstats.rx_ratecode;
                    *bp++ = vowstats.rx_moreaggr;

                    /* sequence number */
                    *bp++ = (u_int8_t)((vowstats.rx_seqno >> 8) & 0xff);
                    *bp++ = (u_int8_t)(vowstats.rx_seqno & 0xff);
                }
            }
        }
    }
}


