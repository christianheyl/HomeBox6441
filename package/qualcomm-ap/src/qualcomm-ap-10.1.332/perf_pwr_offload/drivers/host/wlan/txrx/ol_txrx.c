/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

/*=== includes ===*/
/* header files for OS primitives */
#include <osdep.h>         /* u_int32_t, etc. */
#include <adf_os_mem.h>    /* adf_os_mem_alloc,free */
#include <adf_os_types.h>  /* adf_os_device_t, adf_os_print */
#include <adf_os_lock.h>   /* adf_os_spinlock */
#include <adf_os_atomic.h> /* adf_os_atomic_read */

#if QCA_OL_11AC_FAST_PATH
#include <ath_pci.h> /* struct ath_hif_pci_softc */
#endif 

/* header files for utilities */
#include <queue.h>         /* TAILQ */

/* header files for configuration API */
#include <ol_cfg.h>        /* ol_cfg_is_high_latency */
#include <ol_if_athvar.h>

/* header files for HTT API */
#include <ol_htt_api.h>
#include <ol_htt_tx_api.h>

/* header files for OS shim API */
#include <ol_osif_api.h>

/* header files for our own APIs */
#include <ol_txrx_api.h>
#include <ol_txrx_dbg.h>
#include <ol_txrx_ctrl_api.h>
#include <ol_txrx_osif_api.h>

/* header files for our internal definitions */
#include <ol_txrx_internal.h>  /* TXRX_ASSERT, etc. */
#include <ol_txrx_types.h>     /* ol_txrx_pdev_t, etc. */
#include <ol_tx.h>             /* ol_tx_hl, ol_tx_ll */
#include <ol_rx.h>             /* ol_rx_deliver */
#include <ol_txrx_peer_find.h> /* ol_txrx_peer_find_attach, etc. */
#include <ol_rx_pn.h>          /* ol_rx_pn_check, etc. */
#include <ol_rx_fwd.h>         /* ol_rx_fwd_check, etc. */
#include <ol_tx_desc.h>        /* ol_tx_desc_frame_free */
#include <wdi_event.h>         /* WDI events */
#include <ol_ratectrl_11ac_if.h>    /* attaching/freeing rate-control contexts */


#if QCA_OL_11AC_FAST_PATH
#include <copy_engine_api.h> /* struct ath_hif_pci_softc */
#endif 
/*=== local definitions ===*/
#ifndef OL_TX_AVG_FRM_BYTES
#define OL_TX_AVG_FRM_BYTES 1000
#endif

#ifndef OL_TX_DESC_POOL_SIZE_MIN
#define OL_TX_DESC_POOL_SIZE_MIN 500
#endif

#ifndef OL_TX_DESC_POOL_SIZE_MAX
#define OL_TX_DESC_POOL_SIZE_MAX 5000
#endif

/*=== function definitions ===*/

static int 
ol_tx_desc_pool_size(ol_pdev_handle ctrl_pdev)
{
    int desc_pool_size;
    int steady_state_tx_lifetime_ms;
    int safety_factor;

    /*
     * Steady-state tx latency:
     *     roughly 1-2 ms flight time
     *   + roughly 1-2 ms prep time,
     *   + roughly 1-2 ms target->host notification time.
     * = roughly 6 ms total
     * Thus, steady state number of frames =
     * steady state max throughput / frame size * tx latency, e.g.
     * 1 Gbps / 1500 bytes * 6 ms = 500
     *
     */
    steady_state_tx_lifetime_ms = 6;

    safety_factor = 8;

    desc_pool_size =
        ol_cfg_max_thruput_mbps(ctrl_pdev) *
        1000 /* 1e6 bps/mbps / 1e3 ms per sec = 1000 */ /
        (8 * OL_TX_AVG_FRM_BYTES) *
        steady_state_tx_lifetime_ms *
        safety_factor;

    /* minimum */
    if (desc_pool_size < OL_TX_DESC_POOL_SIZE_MIN) {
        desc_pool_size = OL_TX_DESC_POOL_SIZE_MIN;
    }
    /* maximum */
    if (desc_pool_size > OL_TX_DESC_POOL_SIZE_MAX) {
        desc_pool_size = OL_TX_DESC_POOL_SIZE_MAX;
    }
    return desc_pool_size;
}

static ol_txrx_prot_an_handle
ol_txrx_prot_an_attach(struct ol_txrx_pdev_t *pdev, const char *name)
{
    ol_txrx_prot_an_handle base;

    base = OL_TXRX_PROT_AN_CREATE_802_3(pdev, name);
    if (base) {
        ol_txrx_prot_an_handle ipv4;
        ol_txrx_prot_an_handle ipv6;
        ol_txrx_prot_an_handle arp;

        arp = OL_TXRX_PROT_AN_ADD_ARP(pdev, base);
        ipv6 = OL_TXRX_PROT_AN_ADD_IPV6(pdev, base);
        ipv4 = OL_TXRX_PROT_AN_ADD_IPV4(pdev, base);

        if (ipv4) {
            /* limit TCP printouts to once per 5 sec */
            OL_TXRX_PROT_AN_ADD_TCP(
                pdev, ipv4, TXRX_PROT_ANALYZE_PERIOD_TIME, 0x0, 5000);
            /* limit UDP printouts to once per 5 sec */
            OL_TXRX_PROT_AN_ADD_UDP(
                pdev, ipv4, TXRX_PROT_ANALYZE_PERIOD_TIME, 0x3, 5000);
            /* limit ICMP printouts to two per sec */
            OL_TXRX_PROT_AN_ADD_ICMP(
                pdev, ipv4, TXRX_PROT_ANALYZE_PERIOD_TIME, 0x0, 500);
        }
        /* could add TCP, UDP, and ICMP for IPv6 too */
    }
    return base;
}

ol_txrx_pdev_handle
ol_txrx_pdev_attach(
    ol_pdev_handle ctrl_pdev,
    HTC_HANDLE htc_pdev,
    adf_os_device_t osdev)
{
    int i, desc_pool_size;
    struct ol_txrx_pdev_t *pdev;
    A_STATUS ret;
#if QCA_OL_11AC_FAST_PATH
    struct ol_ath_softc_net80211 *scn =
                (struct ol_ath_softc_net80211 *)ctrl_pdev;
    struct ath_hif_pci_softc *sc =
        (struct ath_hif_pci_softc *)(scn->hif_sc);
#endif

    pdev = adf_os_mem_alloc(osdev, sizeof(*pdev));
    if (!pdev) {
        goto fail0;
    }

    /* init LL/HL cfg here */
    pdev->cfg.is_high_latency = ol_cfg_is_high_latency(ctrl_pdev);

    /* store provided params */
    pdev->ctrl_pdev = ctrl_pdev;
    pdev->osdev = osdev;

    TXRX_STATS_INIT(pdev);

    TAILQ_INIT(&pdev->vdev_list);

    /* do initial set up of the peer ID -> peer object lookup map */
    if (ol_txrx_peer_find_attach(pdev)) {
        goto fail1;
    }

    if (ol_cfg_is_high_latency(ctrl_pdev)) {
        desc_pool_size = ol_tx_desc_pool_size(ctrl_pdev);
    } else {
        /* In LL data path having more descriptors than target raises a
         * race condition with the current target credit implememntation.
         */
        desc_pool_size = ol_cfg_target_tx_credit(ctrl_pdev);
    }

#if QCA_OL_11AC_FAST_PATH
    /*
     * Before the HTT attach, set up the CE handles
     * CE handles are (struct CE_state *)
     * This is only required in the fast path
     */
    pdev->ce_tx_hdl = (struct CE_handle *)
                    sc->CE_id_to_state[CE_HTT_TX_CE];
    pdev->ce_htt_msg_hdl = (struct CE_handle *)
                    sc->CE_id_to_state[CE_HTT_MSG_CE];
#endif /* QCA_OL_11AC_FAST_PATH */

    pdev->htt_pdev = htt_attach(
        pdev, ctrl_pdev, htc_pdev, osdev, desc_pool_size);
    if (!pdev->htt_pdev) {
        goto fail2;
    }

#if QCA_OL_11AC_FAST_PATH
    scn->htt_pdev = pdev->htt_pdev;
#endif /* QCA_OL_11AC_FAST_PATH */

    pdev->tx_desc.array = adf_os_mem_alloc(
        osdev, desc_pool_size * sizeof(union ol_tx_desc_list_elem_t));
    if (!pdev->tx_desc.array) {
        goto fail3;
    }
    adf_os_mem_set(
        pdev->tx_desc.array, 0,
        desc_pool_size * sizeof(union ol_tx_desc_list_elem_t));

    /*
     * Each SW tx desc (used only within the tx datapath SW) has a
     * matching HTT tx desc (used for downloading tx meta-data to FW/HW).
     * Go ahead and allocate the HTT tx desc and link it with the SW tx
     * desc now, to avoid doing it during time-critical transmit.
     */
    pdev->tx_desc.pool_size = desc_pool_size;
    for (i = 0; i < desc_pool_size; i++) {
        void *htt_tx_desc;
        htt_tx_desc = htt_tx_desc_alloc(pdev->htt_pdev);
        if (! htt_tx_desc) {
            adf_os_print("%s: failed to alloc HTT tx desc (%d of %d)\n",
                __func__, i, desc_pool_size);
            while (--i >= 0) {
                htt_tx_desc_free(
                    pdev->htt_pdev,
                    pdev->tx_desc.array[i].tx_desc.htt_tx_desc);
            }
            goto fail4;
        }
        pdev->tx_desc.array[i].tx_desc.htt_tx_desc = htt_tx_desc;
#if QCA_OL_11AC_FAST_PATH
        /* Initialize ID once for all */
        pdev->tx_desc.array[i].tx_desc.id = i;
        adf_os_atomic_init(&pdev->tx_desc.array[i].tx_desc.ref_cnt);
#endif /* QCA_OL_11AC_FAST_PATH */
    }

    /* link SW tx descs into a freelist */
    pdev->tx_desc.freelist = &pdev->tx_desc.array[0];
    for (i = 0; i < desc_pool_size-1; i++) {
        pdev->tx_desc.array[i].next = &pdev->tx_desc.array[i+1];
    }
    pdev->tx_desc.array[i].next = NULL;

    /* initialize the counter of the target's tx buffer availability */
    adf_os_atomic_init(&pdev->target_tx_credit);
    adf_os_atomic_add(
        ol_cfg_target_tx_credit(pdev->ctrl_pdev), &pdev->target_tx_credit);

    /* check what format of frames are expected to be delivered by the OS */
    pdev->frame_format = ol_cfg_frame_type(pdev->ctrl_pdev);
    if (pdev->frame_format == wlan_frm_fmt_native_wifi) {
        pdev->htt_pkt_type = htt_pkt_type_native_wifi;
    } else if (pdev->frame_format == wlan_frm_fmt_802_3) {
        pdev->htt_pkt_type = htt_pkt_type_ethernet;
    } else {
        adf_os_print("Invalid standard frame type: %d\n", pdev->frame_format);
        goto fail5;
    }

    /* setup the global rx defrag waitlist */
    TAILQ_INIT(&pdev->rx.defrag.waitlist);

    /* configure where defrag timeout and duplicate detection is handled */
    pdev->rx.flags.defrag_timeout_check = ol_cfg_rx_host_defrag_timeout_duplicate_check(ctrl_pdev);
    pdev->rx.flags.dup_check = ol_cfg_rx_host_defrag_timeout_duplicate_check(ctrl_pdev);

    /*
     * Determine what rx processing steps are done within the host.
     * Possibilities:
     * 1.  Nothing - rx->tx forwarding and rx PN entirely within target.
     *     (This is unlikely; even if the target is doing rx->tx forwarding,
     *     the host should be doing rx->tx forwarding too, as a back up for
     *     the target's rx->tx forwarding, in case the target runs short on
     *     memory, and can't store rx->tx frames that are waiting for missing
     *     prior rx frames to arrive.)
     * 2.  Just rx -> tx forwarding.
     *     This is the typical configuration for HL, and a likely
     *     configuration for LL STA or small APs (e.g. retail APs).
     * 3.  Both PN check and rx -> tx forwarding.
     *     This is the typical configuration for large LL APs.
     * Host-side PN check without rx->tx forwarding is not a valid
     * configuration, since the PN check needs to be done prior to
     * the rx->tx forwarding.
     */
    if (ol_cfg_rx_pn_check(pdev->ctrl_pdev)) {
        if (ol_cfg_rx_fwd_check(pdev->ctrl_pdev)) {
            /* 
             * Both PN check and rx->tx forwarding done on host.
             */
            pdev->rx_opt_proc = ol_rx_pn_check;
        } else {
            adf_os_print(
                "%s: invalid config: if rx PN check is on the host,"
                "rx->tx forwarding check needs to also be on the host.\n",
                __func__);
            goto fail5;
        }
    } else {
        /* PN check done on target */
        if (ol_cfg_rx_fwd_check(pdev->ctrl_pdev)) {
            /*
             * rx->tx forwarding done on host (possibly as
             * back-up for target-side primary rx->tx forwarding)
             */
            pdev->rx_opt_proc = ol_rx_fwd_check;
        } else {
            pdev->rx_opt_proc = ol_rx_deliver;
        }
    }

    /* Allocate space for holding monitor mode status for RX packets */
    pdev->monitor_vdev = NULL;
    pdev->rx_mon_recv_status = adf_os_mem_alloc(
        osdev, sizeof(struct ieee80211_rx_status));
    if (pdev->rx_mon_recv_status == NULL) {
        goto fail5;
    }

    /* initialize mutexes for tx desc alloc and peer lookup */
    adf_os_spinlock_init(&pdev->tx_mutex);
    adf_os_spinlock_init(&pdev->peer_ref_mutex);
    
    pdev->prot_an_tx_sent = ol_txrx_prot_an_attach(pdev, "xmit 802.3");
    pdev->prot_an_rx_sent = ol_txrx_prot_an_attach(pdev, "recv 802.3");

    if (OL_RX_REORDER_TRACE_ATTACH(pdev) != A_OK) {
        goto fail6;
    }

    if (OL_RX_PN_TRACE_ATTACH(pdev) != A_OK) {
        goto fail7;
    }

#if PERE_IP_HDR_ALIGNMENT_WAR 
    pdev->host_80211_enable = ol_scn_host_80211_enable_get(pdev->ctrl_pdev);
#endif

    /*
     * WDI event attach
     */
    if ((ret = wdi_event_attach(pdev)) == A_ERROR) {
        adf_os_print("WDI event attach unsuccessful\n");
    }

    /*
     * pktlog pdev initialization
     */
    ol_pl_sethandle(&(pdev->pl_dev), pdev->ctrl_pdev);

    /*
     * Initialize rx PN check characteristics for different security types.
     */
    adf_os_mem_set(&pdev->rx_pn[0], 0, sizeof(pdev->rx_pn));

    /* TKIP: 48-bit TSC, CCMP: 48-bit PN */
    pdev->rx_pn[htt_sec_type_tkip].len =
        pdev->rx_pn[htt_sec_type_tkip_nomic].len =
        pdev->rx_pn[htt_sec_type_aes_ccmp].len = 48;
    pdev->rx_pn[htt_sec_type_tkip].cmp =
        pdev->rx_pn[htt_sec_type_tkip_nomic].cmp =
        pdev->rx_pn[htt_sec_type_aes_ccmp].cmp = ol_rx_pn_cmp48;

    /* WAPI: 128-bit PN */
    pdev->rx_pn[htt_sec_type_wapi].len = 128;
    pdev->rx_pn[htt_sec_type_wapi].cmp = ol_rx_pn_wapi_cmp;

    /* Default value unless changed by the WMI-service-ready event. */
    pdev->ratectrl.is_ratectrl_on_host = 0;

    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "Created pdev %p\n", pdev);

    return pdev; /* success */

fail7:
    OL_RX_REORDER_TRACE_DETACH(pdev);

fail6:
    adf_os_spinlock_destroy(&pdev->tx_mutex);
    adf_os_spinlock_destroy(&pdev->peer_ref_mutex);

    adf_os_mem_free(pdev->rx_mon_recv_status);

fail5:
    for (i = 0; i < desc_pool_size; i++) {
        htt_tx_desc_free(
            pdev->htt_pdev, pdev->tx_desc.array[i].tx_desc.htt_tx_desc);
    }

fail4:
    adf_os_mem_free(pdev->tx_desc.array);

fail3:
    htt_detach(pdev->htt_pdev);

fail2:
    ol_txrx_peer_find_detach(pdev);

fail1:
    adf_os_mem_free(pdev);

fail0:
    return NULL; /* fail */
}

A_STATUS
ol_txrx_pdev_attach_target(ol_txrx_pdev_handle pdev)
{
    return htt_attach_target(pdev->htt_pdev);
}

void 
ol_txrx_enable_host_ratectrl(ol_txrx_pdev_handle pdev, u_int32_t enable)
{
    ol_ratectrl_enable_host_ratectrl(pdev, enable);

    return;
}

void
ol_txrx_pdev_detach(ol_txrx_pdev_handle pdev, int force)
{
    int i;
    A_STATUS ret;
    /* preconditions */
    TXRX_ASSERT2(pdev);

    /* check that the pdev has no vdevs allocated */
    TXRX_ASSERT1(TAILQ_EMPTY(&pdev->vdev_list));

    if (force) {
        /*
         * The assertion above confirms that all vdevs within this pdev
         * were detached.  However, they may not have actually been deleted.
         * If the vdev had peers which never received a PEER_UNMAP message
         * from the target, then there are still zombie peer objects, and
         * the vdev parents of the zombie peers are also zombies, hanging
         * around until their final peer gets deleted.
         * Go through the peer hash table and delete any peers left in it.
         * As a side effect, this will complete the deletion of any vdevs
         * that are waiting for their peers to finish deletion.
         */
        TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "Force delete for pdev %p\n", pdev);
        ol_txrx_peer_find_hash_erase(pdev);
    }

    for (i = 0; i < pdev->tx_desc.pool_size; i++) {
        void *htt_tx_desc;

        /*
         * Confirm that each tx descriptor is "empty", i.e. it has
         * no tx frame attached.
         * In particular, check that there are no frames that have
         * been given to the target to transmit, for which the
         * target has never provided a response.
         */
        if (adf_os_atomic_read(&pdev->tx_desc.array[i].tx_desc.ref_cnt)) {
            TXRX_PRINT(TXRX_PRINT_LEVEL_WARN,
                "Warning: freeing tx frame "
                "(no tx completion from the target)\n");
            ol_tx_desc_frame_free_nonstd(
                pdev, &pdev->tx_desc.array[i].tx_desc, 1);
        }
        htt_tx_desc = pdev->tx_desc.array[i].tx_desc.htt_tx_desc;
        htt_tx_desc_free(pdev->htt_pdev, htt_tx_desc);
    }

    adf_os_mem_free(pdev->tx_desc.array);

    htt_detach(pdev->htt_pdev);

    ol_txrx_peer_find_detach(pdev);

    adf_os_spinlock_destroy(&pdev->tx_mutex);
    adf_os_spinlock_destroy(&pdev->peer_ref_mutex);

    adf_os_mem_free(pdev->rx_mon_recv_status);

    OL_TXRX_PROT_AN_FREE(pdev->prot_an_tx_sent);
    OL_TXRX_PROT_AN_FREE(pdev->prot_an_rx_sent);

    OL_RX_REORDER_TRACE_DETACH(pdev);
    OL_RX_PN_TRACE_DETACH(pdev);
    /*
     * WDI event detach
     */
    if ((ret = wdi_event_detach(pdev)) == A_ERROR) {
        adf_os_print("WDI detach unsuccessful\n");
    }
    adf_os_mem_free(pdev);
}

ol_txrx_vdev_handle
ol_txrx_vdev_attach(
    ol_txrx_pdev_handle pdev,
    u_int8_t *vdev_mac_addr,
    u_int8_t vdev_id,
    enum wlan_op_mode op_mode)
{
    struct ol_txrx_vdev_t *vdev;

    /* preconditions */
    TXRX_ASSERT2(pdev);
    TXRX_ASSERT2(vdev_mac_addr);

    vdev = adf_os_mem_alloc(pdev->osdev, sizeof(*vdev));
    if (!vdev) {
        return NULL; /* failure */
    }

    /* store provided params */
    vdev->pdev = pdev;
    vdev->vdev_id = vdev_id;
    vdev->opmode = op_mode;

    vdev->osif_rx =     NULL;
    vdev->osif_rx_mon = NULL;
    vdev->osif_vdev =   NULL;

    vdev->delete.pending = 0;
    vdev->safemode = 0;

    adf_os_mem_copy(
        &vdev->mac_addr.raw[0], vdev_mac_addr, sizeof(vdev->mac_addr));

    if(vdev->opmode != wlan_op_mode_monitor) {
        vdev->pRateCtrl = NULL;

        if(pdev->ratectrl.is_ratectrl_on_host) {
            /* Attach the context for rate-control. */
            vdev->pRateCtrl = ol_ratectrl_vdev_ctxt_attach(pdev, vdev);

            if (!(vdev->pRateCtrl)) {
                /* failure case */
                adf_os_mem_free(vdev);
                vdev = NULL;
                return NULL;
            }
        }

    }

    TAILQ_INIT(&vdev->peer_list);

    /* add this vdev into the pdev's list */
    TAILQ_INSERT_TAIL(&pdev->vdev_list, vdev, vdev_list_elem);

    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1,
        "Created vdev %p (%02x:%02x:%02x:%02x:%02x:%02x)\n",
        vdev,
        vdev->mac_addr.raw[0], vdev->mac_addr.raw[1], vdev->mac_addr.raw[2],
        vdev->mac_addr.raw[3], vdev->mac_addr.raw[4], vdev->mac_addr.raw[5]);

    return vdev;
}

void
ol_txrx_osif_vdev_register(
    ol_txrx_vdev_handle vdev,
    ol_osif_vdev_handle osif_vdev,
    struct ol_txrx_osif_ops *txrx_ops)
{
    vdev->osif_vdev = osif_vdev;
    vdev->osif_rx = txrx_ops->rx.std;
    vdev->osif_rx_mon = txrx_ops->rx.mon;
#if UMAC_SUPPORT_PROXY_ARP
    vdev->osif_proxy_arp = txrx_ops->proxy_arp;
#endif
    if (ol_cfg_is_high_latency(vdev->pdev->ctrl_pdev)) {
        txrx_ops->tx.std = vdev->tx = ol_tx_hl;
        txrx_ops->tx.non_std = ol_tx_non_std_hl;
    } else {
        txrx_ops->tx.std = vdev->tx = ol_tx_ll;
        txrx_ops->tx.non_std = ol_tx_non_std_ll;
    }
}

int
ol_txrx_set_monitor_mode_vap(
    ol_txrx_pdev_handle pdev,
    ol_txrx_vdev_handle vdev)
{
    /* Many monitor VAPs can exists in a system but only one can be up at
     * anytime
     */
    if (vdev && pdev->monitor_vdev) {
        adf_os_print("Monitor mode VAP already up\n");
        return -1;        
    }

    pdev->monitor_vdev = vdev;
    return 0;
}

void
ol_txrx_set_curchan(
    ol_txrx_pdev_handle pdev,
    u_int32_t chan_mhz)
{
    pdev->rx_mon_recv_status->rs_freq = chan_mhz;
    return;
}

void 
ol_txrx_set_safemode(
    ol_txrx_vdev_handle vdev,
    u_int32_t val)
{
    vdev->safemode = val;
}

#if WDS_VENDOR_EXTENSION
void 
ol_txrx_set_wds_rx_policy(
    ol_txrx_vdev_handle vdev,
    u_int32_t val)
{
    struct ol_txrx_peer_t *peer;
    if (vdev->opmode == wlan_op_mode_ap) {
        /* for ap, set it on bss_peer */
        TAILQ_FOREACH(peer, &vdev->peer_list, peer_list_elem) {
            if (peer->bss_peer) {
                peer->wds_rx_filter = 1;
                peer->wds_rx_ucast_4addr = (val & WDS_POLICY_RX_UCAST_4ADDR) ? 1:0;
                peer->wds_rx_mcast_4addr = (val & WDS_POLICY_RX_MCAST_4ADDR) ? 1:0;
                break;
            }
        }
    }
    else if (vdev->opmode == wlan_op_mode_sta) {
        peer = TAILQ_FIRST(&vdev->peer_list);
        peer->wds_rx_filter = 1;
        peer->wds_rx_ucast_4addr = (val & WDS_POLICY_RX_UCAST_4ADDR) ? 1:0;
        peer->wds_rx_mcast_4addr = (val & WDS_POLICY_RX_MCAST_4ADDR) ? 1:0;
    }
}
#endif

void
ol_txrx_vdev_detach(
    ol_txrx_vdev_handle vdev,
    ol_txrx_vdev_delete_cb callback,
    void *context)
{
    struct ol_txrx_pdev_t *pdev = vdev->pdev;

    /* preconditions */
    TXRX_ASSERT2(vdev);

    /* remove the vdev from its parent pdev's list */
    TAILQ_REMOVE(&pdev->vdev_list, vdev, vdev_list_elem);

    /*
     * Use peer_ref_mutex while accessing peer_list, in case
     * a peer is in the process of being removed from the list.
     */
    adf_os_spin_lock_bh(&pdev->peer_ref_mutex);
    /* check that the vdev has no peers allocated */
    if (!TAILQ_EMPTY(&vdev->peer_list)) {
        /* debug print - will be removed later */
        TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1,
            "%s: not deleting vdev object %p (%02x:%02x:%02x:%02x:%02x:%02x)"
            "until deletion finishes for all its peers\n",
            __func__, vdev,
            vdev->mac_addr.raw[0], vdev->mac_addr.raw[1],
            vdev->mac_addr.raw[2], vdev->mac_addr.raw[3],
            vdev->mac_addr.raw[4], vdev->mac_addr.raw[5]);
        /* indicate that the vdev needs to be deleted */
        vdev->delete.pending = 1;
        vdev->delete.callback = callback;
        vdev->delete.context = context;
        adf_os_spin_unlock_bh(&pdev->peer_ref_mutex);
        return;
    }
    adf_os_spin_unlock_bh(&pdev->peer_ref_mutex);

    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1,
        "%s: deleting vdev object %p (%02x:%02x:%02x:%02x:%02x:%02x)\n",
        __func__, vdev,
        vdev->mac_addr.raw[0], vdev->mac_addr.raw[1], vdev->mac_addr.raw[2],
        vdev->mac_addr.raw[3], vdev->mac_addr.raw[4], vdev->mac_addr.raw[5]);

    /* Free the rate-control context. */
    ol_ratectrl_vdev_ctxt_detach(vdev->pRateCtrl);

    /*
     * Doesn't matter if there are outstanding tx frames -
     * they will be freed once the target sends a tx completion
     * message for them.
     */
    adf_os_mem_free(vdev);
    if (callback) {
        callback(context);
    }
}

ol_txrx_peer_handle
ol_txrx_peer_attach(
    ol_txrx_pdev_handle pdev,
    ol_txrx_vdev_handle vdev,
    u_int8_t *peer_mac_addr)
{
    struct ol_txrx_peer_t *peer;
    int i;

    /* preconditions */
    TXRX_ASSERT2(pdev);
    TXRX_ASSERT2(vdev);
    TXRX_ASSERT2(peer_mac_addr);

/* CHECK CFG TO DETERMINE WHETHER TO ALLOCATE BASE OR EXTENDED PEER STRUCT */
/* USE vdev->pdev->osdev, AND REMOVE PDEV FUNCTION ARG? */
    peer = adf_os_mem_alloc(pdev->osdev, sizeof(*peer));
    if (!peer) {
        return NULL; /* failure */
    }

    /* store provided params */
    peer->vdev = vdev;
    adf_os_mem_copy(
        &peer->mac_addr.raw[0], peer_mac_addr, sizeof(peer->mac_addr));

    if(vdev->opmode != wlan_op_mode_monitor) {
        peer->rc_node = NULL;

        if(pdev->ratectrl.is_ratectrl_on_host) {
            /* Attach the context for rate-control. */
            peer->rc_node = ol_ratectrl_peer_ctxt_attach(pdev, vdev, peer);

            if (!(peer->rc_node)) {
                /* failure case */
                adf_os_mem_free(peer);
                peer = NULL;
                return NULL;
            }
        }

    }

    /* add this peer into the vdev's list */
    TAILQ_INSERT_TAIL(&vdev->peer_list, peer, peer_list_elem);

    peer->rx_opt_proc = pdev->rx_opt_proc;

    ol_rx_peer_init(pdev, peer);

    //ol_tx_peer_init(pdev, peer);

    /* initialize the peer_id */
    for (i = 0; i < MAX_NUM_PEER_ID_PER_PEER; i++) {
        peer->peer_ids[i] = HTT_INVALID_PEER;
    }

    adf_os_atomic_init(&peer->ref_cnt);

    /* keep one reference for attach */
    adf_os_atomic_inc(&peer->ref_cnt);

    ol_txrx_peer_find_hash_add(pdev, peer);

    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO2,
        "vdev %p created peer %p (%02x:%02x:%02x:%02x:%02x:%02x)\n",
        vdev, peer,
        peer->mac_addr.raw[0], peer->mac_addr.raw[1], peer->mac_addr.raw[2],
        peer->mac_addr.raw[3], peer->mac_addr.raw[4], peer->mac_addr.raw[5]);
    /*
     * For every peer MAp message search and set if bss_peer
     */
    if (memcmp(peer->mac_addr.raw, vdev->mac_addr.raw, 6) == 0){
        peer->bss_peer = 1;
    }

    return peer;
}

void
ol_txrx_peer_update(struct ol_txrx_peer_t *peer,
        struct peer_ratectrl_params_t *peer_ratectrl_params)
{
    /* Update the peer-params in the context for rate-control. */
    ol_ratectrl_peer_ctxt_update(peer, peer_ratectrl_params);

    return;
}

#if WDS_VENDOR_EXTENSION
void
ol_txrx_peer_wds_tx_policy_update(struct ol_txrx_peer_t *peer,
        int wds_tx_ucast, int wds_tx_mcast)
{
    if (wds_tx_ucast || wds_tx_mcast) {
        peer->wds_enabled = 1;
        peer->wds_tx_ucast_4addr = wds_tx_ucast;
        peer->wds_tx_mcast_4addr = wds_tx_mcast;
    }
    else {
        peer->wds_enabled = 0;
        peer->wds_tx_ucast_4addr = 0;
        peer->wds_tx_mcast_4addr = 0;
    }
    return;
}
#endif

void
ol_txrx_peer_unref_delete(ol_txrx_peer_handle peer)
{
    struct ol_txrx_vdev_t *vdev;
    struct ol_txrx_pdev_t *pdev;

    /* preconditions */
    TXRX_ASSERT2(peer);

    vdev = peer->vdev;
    pdev = vdev->pdev;

    /*
     * Hold the lock all the way from checking if the peer ref count
     * is zero until the peer references are removed from the hash
     * table and vdev list (if the peer ref count is zero).
     * This protects against a new HL tx operation starting to use the
     * peer object just after this function concludes it's done being used.
     * Furthermore, the lock needs to be held while checking whether the
     * vdev's list of peers is empty, to make sure that list is not modified
     * concurrently with the empty check.
     */
    adf_os_spin_lock_bh(&pdev->peer_ref_mutex);
    if (adf_os_atomic_dec_and_test(&peer->ref_cnt)) {
        TXRX_PRINT(TXRX_PRINT_LEVEL_INFO2,
            "Deleting peer %p (%02x:%02x:%02x:%02x:%02x:%02x)\n",
            peer,
            peer->mac_addr.raw[0], peer->mac_addr.raw[1],
            peer->mac_addr.raw[2], peer->mac_addr.raw[3],
            peer->mac_addr.raw[4], peer->mac_addr.raw[5]);

        /* remove the reference to the peer from the hash table */
        ol_txrx_peer_find_hash_remove(pdev, peer);

        /* remove the peer from its parent vdev's list */
        TAILQ_REMOVE(&peer->vdev->peer_list, peer, peer_list_elem);

        /* cleanup the Rx reorder queues for this peer */
        ol_rx_peer_cleanup(vdev, peer);

        /* check whether the parent vdev has no peers left */
        if (TAILQ_EMPTY(&vdev->peer_list)) {
            /*
             * Now that there are no references to the peer, we can
             * release the peer reference lock.
             */
            adf_os_spin_unlock_bh(&pdev->peer_ref_mutex);
            /*
             * Check if the parent vdev was waiting for its peers to be 
             * deleted, in order for it to be deleted too.
             */
            if (vdev->delete.pending) {
                ol_txrx_vdev_delete_cb vdev_delete_cb = vdev->delete.callback;
                void *vdev_delete_context = vdev->delete.context;

                TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1,
                    "%s: deleting vdev object %p "
                    "(%02x:%02x:%02x:%02x:%02x:%02x)"
                    " - its last peer is done\n",
                    __func__, vdev,
                    vdev->mac_addr.raw[0], vdev->mac_addr.raw[1],
                    vdev->mac_addr.raw[2], vdev->mac_addr.raw[3],
                    vdev->mac_addr.raw[4], vdev->mac_addr.raw[5]);
                /* all peers are gone, go ahead and delete it */
                adf_os_mem_free(vdev);
                if (vdev_delete_cb) {
                    vdev_delete_cb(vdev_delete_context);
                }
            }
        } else {
            adf_os_spin_unlock_bh(&pdev->peer_ref_mutex);
        }

        adf_os_mem_free(peer);
    } else {
        adf_os_spin_unlock_bh(&pdev->peer_ref_mutex);
    }
}

void
ol_txrx_peer_detach(ol_txrx_peer_handle peer)
{

    /* redirect the peer's rx delivery function to point to a discard func */
    peer->rx_opt_proc = ol_rx_discard;

    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO2,
        "%s:peer %p (%02x:%02x:%02x:%02x:%02x:%02x)\n",
          __func__, peer,
          peer->mac_addr.raw[0], peer->mac_addr.raw[1],
          peer->mac_addr.raw[2], peer->mac_addr.raw[3],
          peer->mac_addr.raw[4], peer->mac_addr.raw[5]);

    /* Free the rate-control context. */
    ol_ratectrl_peer_ctxt_detach(peer->rc_node);

    /*
     * Remove the reference added during peer_attach.
     * The peer will still be left allocated until the
     * PEER_UNMAP message arrives to remove the other
     * reference, added by the PEER_MAP message.
     */
    ol_txrx_peer_unref_delete(peer);
}

int
ol_txrx_get_tx_pending(ol_txrx_pdev_handle pdev_handle)
{
    struct ol_txrx_pdev_t *pdev = (ol_txrx_pdev_handle)pdev_handle;
    int credit, total;

    total = ol_cfg_target_tx_credit(pdev->ctrl_pdev);
    credit = adf_os_atomic_read(&pdev->target_tx_credit);

    return (total - credit);
}
/*--- debug features --------------------------------------------------------*/

unsigned g_txrx_print_level = TXRX_PRINT_LEVEL_INFO1; /* default */

void ol_txrx_print_level_set(unsigned level)
{
#ifndef TXRX_PRINT_ENABLE
    adf_os_print(
        "The driver is compiled without TXRX prints enabled.\n"
        "To enable them, recompile with TXRX_PRINT_ENABLE defined.\n");
#else
    adf_os_print("TXRX printout level changed from %d to %d\n",
        g_txrx_print_level, level);
    g_txrx_print_level = level;
#endif
}

struct ol_txrx_stats_req_internal {
    struct ol_txrx_stats_req base;
    int serviced; /* state of this request */
    int offset;
};

static inline
u_int64_t OL_TXRX_STATS_PTR_TO_U64(struct ol_txrx_stats_req_internal *req)
{
    return (u_int64_t) ((size_t) req);
}

static inline
struct ol_txrx_stats_req_internal * OL_TXRX_U64_TO_STATS_PTR(u_int64_t cookie)
{
    return (struct ol_txrx_stats_req_internal *) ((size_t) cookie);
}

void
ol_txrx_fw_stats_cfg(
    ol_txrx_vdev_handle vdev,
    u_int8_t cfg_stats_type,
    u_int32_t cfg_val)
{
    u_int64_t dummy_cookie = 0;
    htt_h2t_dbg_stats_get(
        vdev->pdev->htt_pdev,
        0 /* upload mask */,
        0 /* reset mask */,
        cfg_stats_type,
        cfg_val,
        dummy_cookie);
}

A_STATUS
ol_txrx_fw_stats_get(
    ol_txrx_vdev_handle vdev,
    struct ol_txrx_stats_req *req)
{
    struct ol_txrx_pdev_t *pdev = vdev->pdev;
    u_int64_t cookie;
    struct ol_txrx_stats_req_internal *non_volatile_req;

    if (!pdev ||
        req->stats_type_upload_mask >= 1 << HTT_DBG_NUM_STATS ||
        req->stats_type_reset_mask >= 1 << HTT_DBG_NUM_STATS )
    {
        return A_ERROR;
    }

    /*
     * Allocate a non-transient stats request object.
     * (The one provided as an argument is likely allocated on the stack.)
     */
    non_volatile_req = adf_os_mem_alloc(pdev->osdev, sizeof(*non_volatile_req));
    if (! non_volatile_req) {
        return A_NO_MEMORY;
    }
    /* copy the caller's specifications */
    non_volatile_req->base = *req;
    non_volatile_req->serviced = 0;
    non_volatile_req->offset = 0;

    /* use the non-volatile request object's address as the cookie */
    cookie = OL_TXRX_STATS_PTR_TO_U64(non_volatile_req);

    if (htt_h2t_dbg_stats_get(
            pdev->htt_pdev,
            req->stats_type_upload_mask,
            req->stats_type_reset_mask,
            HTT_H2T_STATS_REQ_CFG_STAT_TYPE_INVALID, 0,
            cookie))
    {
        adf_os_mem_free(non_volatile_req);
        return A_ERROR;
    }

    if (req->wait.blocking) {
        while (adf_os_mutex_acquire(pdev->osdev, req->wait.sem_ptr)) {}
    }

    return A_OK;
}

#if QCA_OL_11AC_FAST_PATH
A_STATUS
ol_txrx_host_stats_get(
    ol_txrx_vdev_handle vdev,
    struct ol_txrx_stats_req *req)
{
    struct ol_txrx_pdev_t *pdev = vdev->pdev;
    struct ol_txrx_stats *stats = &pdev->stats.pub; 

    /* HACK */
    adf_os_print("++++++++++ HOST TX STATISTICS +++++++++++\n");
    adf_os_print("Ol Tx Desc In Use\t:  %d\n", stats->tx.desc_in_use);
    adf_os_print("Ol Tx Desc Failed\t:  %d\n", stats->tx.desc_alloc_fails);
    adf_os_print("CE Ring (4) Full \t:  %d\n", stats->tx.ce_ring_full);
    adf_os_print("DMA Map Error    \t:  %d\n", stats->tx.dma_map_error);
    adf_os_print("Tx pkts completed\t:  %lu\n", stats->tx.delivered.pkts);
    adf_os_print("Tx bytes completed\t:  %lu\n", stats->tx.delivered.bytes);
    adf_os_print("Tx pkts from stack\t:  %lu\n", stats->tx.from_stack.pkts);

    adf_os_print("\n");
    adf_os_print("++++++++++ HOST RX STATISTICS +++++++++++\n");
    adf_os_print("Rx pkts completed\t:  %lu\n", stats->rx.delivered.pkts);
    adf_os_print("Rx bytes completed\t:  %lu\n", stats->rx.delivered.bytes);
    
    return 0;
}
void
ol_txrx_host_stats_clr(ol_txrx_vdev_handle vdev)
{
    struct ol_txrx_pdev_t *pdev = vdev->pdev;
    struct ol_txrx_stats *stats = &pdev->stats.pub; 

    /* Tx */
    stats->tx.desc_in_use = 0;
    stats->tx.desc_alloc_fails = 0;
    stats->tx.ce_ring_full = 0;
    stats->tx.dma_map_error = 0;
    stats->tx.delivered.pkts = 0;
    stats->tx.delivered.bytes = 0;
    stats->tx.from_stack.pkts = 0;
    stats->tx.from_stack.bytes = 0;

    /* Rx */
    stats->rx.delivered.pkts = 0;
    stats->rx.delivered.bytes = 0;
}
#endif

void
ol_txrx_fw_stats_handler(
    ol_txrx_pdev_handle pdev,
    u_int64_t cookie,
    u_int8_t *stats_info_list)
{
    enum htt_dbg_stats_type type;
    enum htt_dbg_stats_status status;
    int length;
    u_int8_t *stats_data;
    struct ol_txrx_stats_req_internal *req;
    int more = 0;

    req = OL_TXRX_U64_TO_STATS_PTR(cookie);

    do {
        htt_t2h_dbg_stats_hdr_parse(
            stats_info_list, &type, &status, &length, &stats_data);
        if (status == HTT_DBG_STATS_STATUS_SERIES_DONE) {
            break;
        }
        if (status == HTT_DBG_STATS_STATUS_PRESENT ||
            status == HTT_DBG_STATS_STATUS_PARTIAL)
        {
            u_int8_t *buf;
            int bytes = 0;

            if (status == HTT_DBG_STATS_STATUS_PARTIAL) {
                more = 1;
            }
            if (req->base.print.verbose || req->base.print.concise) {
                /* provide the header along with the data */
                htt_t2h_stats_print(stats_info_list, req->base.print.concise);
            }

            switch (type) {
            case HTT_DBG_STATS_WAL_PDEV_TXRX:
                bytes = sizeof(struct wal_dbg_stats);
                if (req->base.copy.buf) {
                    int limit;

                    limit = sizeof(struct wal_dbg_stats);
                    if (req->base.copy.byte_limit < limit) {
                        limit = req->base.copy.byte_limit;
                    }
                    buf = req->base.copy.buf + req->offset;
                    adf_os_mem_copy(buf, stats_data, limit);
                }
                break;
            case HTT_DBG_STATS_RX_REORDER:
                bytes = sizeof(struct rx_reorder_stats);
                if (req->base.copy.buf) {
                    int limit;

                    limit = sizeof(struct rx_reorder_stats);
                    if (req->base.copy.byte_limit < limit) {
                        limit = req->base.copy.byte_limit;
                    }
                    buf = req->base.copy.buf + req->offset;
                    adf_os_mem_copy(buf, stats_data, limit);
                }
                break;
            case HTT_DBG_STATS_RX_RATE_INFO:
                bytes = sizeof(wal_dbg_rx_rate_info_t);
                if (req->base.copy.buf) {
                    int limit;

                    limit = sizeof(wal_dbg_rx_rate_info_t);
                    if (req->base.copy.byte_limit < limit) {
                        limit = req->base.copy.byte_limit;
                    }
                    buf = req->base.copy.buf + req->offset;
                    adf_os_mem_copy(buf, stats_data, limit);
                }
                break;
 
            case HTT_DBG_STATS_TX_RATE_INFO:
                bytes = sizeof(wal_dbg_tx_rate_info_t);
                if (req->base.copy.buf) {
                    int limit;

                    limit = sizeof(wal_dbg_tx_rate_info_t);
                    if (req->base.copy.byte_limit < limit) {
                        limit = req->base.copy.byte_limit;
                    }
                    buf = req->base.copy.buf + req->offset;
                    adf_os_mem_copy(buf, stats_data, limit);
                }
                break;

            case HTT_DBG_STATS_TX_PPDU_LOG:
                bytes = 0; /* TO DO: specify how many bytes are present */
                /* TO DO: add copying to the requestor's buffer */

            default:
                break;
            }
            buf = req->base.copy.buf ? req->base.copy.buf : stats_data;
            if (req->base.callback.fp) {
                req->base.callback.fp(
                    req->base.callback.ctxt, type, buf, bytes);
            }
        }
        stats_info_list += length;
    } while (1);

    if (! more) {
        if (req->base.wait.blocking) {
            adf_os_mutex_release(pdev->osdev, req->base.wait.sem_ptr);
        }
        adf_os_mem_free(req);
    }
}

int ol_txrx_debug(ol_txrx_vdev_handle vdev, int debug_specs)
{
    if (debug_specs & TXRX_DBG_MASK_OBJS) {
        #if TXRX_DEBUG_LEVEL > 5
            ol_txrx_pdev_display(vdev->pdev, 0);
        #else
            adf_os_print(
                "The pdev,vdev,peer display functions are disabled.\n"
                "To enable them, recompile with TXRX_DEBUG_LEVEL > 5.\n");
        #endif
    }
    if (debug_specs & TXRX_DBG_MASK_STATS) {
        #if TXRX_STATS_LEVEL != TXRX_STATS_LEVEL_OFF
            ol_txrx_stats_display(vdev->pdev);
        #else
            adf_os_print(
                "txrx stats collection is disabled.\n"
                "To enable it, recompile with TXRX_STATS_LEVEL on.\n");
        #endif
    }
    if (debug_specs & TXRX_DBG_MASK_PROT_ANALYZE) {
        #if defined(ENABLE_TXRX_PROT_ANALYZE)
            ol_txrx_prot_ans_display(vdev->pdev);
        #else
            adf_os_print(
                "txrx protocol analysis is disabled.\n"
                "To enable it, recompile with " 
                "ENABLE_TXRX_PROT_ANALYZE defined.\n");
        #endif
    }
    if (debug_specs & TXRX_DBG_MASK_RX_REORDER_TRACE) {
        #if defined(ENABLE_RX_REORDER_TRACE)
            ol_rx_reorder_trace_display(vdev->pdev, 0, 0);
        #else
            adf_os_print(
                "rx reorder seq num trace is disabled.\n"
                "To enable it, recompile with " 
                "ENABLE_RX_REORDER_TRACE defined.\n");
        #endif

    }
    return 0;
}

#if defined(TEMP_AGGR_CFG)
int ol_txrx_aggr_cfg(ol_txrx_vdev_handle vdev, 
                     int max_subfrms_ampdu, 
                     int max_subfrms_amsdu)
{
    return htt_h2t_aggr_cfg_msg(vdev->pdev->htt_pdev, 
                                max_subfrms_ampdu, 
                                max_subfrms_amsdu);
}
#endif

#if TXRX_DEBUG_LEVEL > 5
void
ol_txrx_pdev_display(ol_txrx_pdev_handle pdev, int indent)
{
    struct ol_txrx_vdev_t *vdev;

    adf_os_print("%*s%s:\n", indent, " ", "txrx pdev");
    adf_os_print("%*spdev object: %p\n", indent+4, " ", pdev);
    adf_os_print("%*svdev list:\n", indent+4, " ");
    TAILQ_FOREACH(vdev, &pdev->vdev_list, vdev_list_elem) {
        ol_txrx_vdev_display(vdev, indent+8);
    }    
    ol_txrx_peer_find_display(pdev, indent+4);
    adf_os_print("%*stx desc pool: %d elems @ %p\n", indent+4, " ",
        pdev->tx_desc.pool_size, pdev->tx_desc.array);
    adf_os_print("\n");
    htt_display(pdev->htt_pdev, indent);
}

void
ol_txrx_vdev_display(ol_txrx_vdev_handle vdev, int indent)
{
    struct ol_txrx_peer_t *peer;

    adf_os_print("%*stxrx vdev: %p\n", indent, " ", vdev);
    adf_os_print("%*sID: %d\n", indent+4, " ", vdev->vdev_id);
    adf_os_print("%*sMAC addr: %d:%d:%d:%d:%d:%d\n",
        indent+4, " ",
        vdev->mac_addr.raw[0], vdev->mac_addr.raw[1], vdev->mac_addr.raw[2],
        vdev->mac_addr.raw[3], vdev->mac_addr.raw[4], vdev->mac_addr.raw[5]);
    adf_os_print("%*speer list:\n", indent+4, " ");
    TAILQ_FOREACH(peer, &vdev->peer_list, peer_list_elem) {
        ol_txrx_peer_display(peer, indent+8);
    }    
}

void
ol_txrx_peer_display(ol_txrx_peer_handle peer, int indent)
{
    int i;

    adf_os_print("%*stxrx peer: %p\n", indent, " ", peer);
    for (i = 0; i < MAX_NUM_PEER_ID_PER_PEER; i++) {
        if (peer->peer_ids[i] != HTT_INVALID_PEER) {
            adf_os_print("%*sID: %d\n", indent+4, " ", peer->peer_ids[i]);
        }
    }
}

#endif /* TXRX_DEBUG_LEVEL */

#if TXRX_STATS_LEVEL != TXRX_STATS_LEVEL_OFF
void
ol_txrx_stats_display(ol_txrx_pdev_handle pdev)
{
    adf_os_print("txrx stats:\n");
    if (TXRX_STATS_LEVEL == TXRX_STATS_LEVEL_BASIC) {
        adf_os_print("  tx: %lld msdus (%lld B)\n",
            pdev->stats.pub.tx.delivered.pkts,
            pdev->stats.pub.tx.delivered.bytes);
    } else { /* full */
        adf_os_print(
            "  tx: sent %lld msdus (%lld B), "
            "rejected %lld (%lld B), dropped %lld (%lld B)\n",
            pdev->stats.pub.tx.delivered.pkts,
            pdev->stats.pub.tx.delivered.bytes,
            pdev->stats.pub.tx.dropped.host_reject.pkts,
            pdev->stats.pub.tx.dropped.host_reject.bytes,
            pdev->stats.pub.tx.dropped.download_fail.pkts
              + pdev->stats.pub.tx.dropped.target_discard.pkts
              + pdev->stats.pub.tx.dropped.no_ack.pkts,
            pdev->stats.pub.tx.dropped.download_fail.bytes
              + pdev->stats.pub.tx.dropped.target_discard.bytes
              + pdev->stats.pub.tx.dropped.no_ack.bytes);
        adf_os_print(
            "    download fail: %lld (%lld B), "
            "target discard: %lld (%lld B), "
            "no ack: %lld (%lld B)\n",
            pdev->stats.pub.tx.dropped.download_fail.pkts,
            pdev->stats.pub.tx.dropped.download_fail.bytes,
            pdev->stats.pub.tx.dropped.target_discard.pkts,
            pdev->stats.pub.tx.dropped.target_discard.bytes,
            pdev->stats.pub.tx.dropped.no_ack.pkts,
            pdev->stats.pub.tx.dropped.no_ack.bytes);
    }
    adf_os_print(
        "  rx: %lld ppdus, %lld mpdus, %lld msdus, %lld bytes, %lld errs\n",
        pdev->stats.priv.rx.normal.ppdus,
        pdev->stats.priv.rx.normal.mpdus,
        pdev->stats.pub.rx.delivered.pkts,
        pdev->stats.pub.rx.delivered.bytes,
        pdev->stats.priv.rx.err.mpdu_bad);
    if (TXRX_STATS_LEVEL == TXRX_STATS_LEVEL_FULL) {
        adf_os_print(
            "    forwarded %lld msdus, %lld bytes\n",
            pdev->stats.pub.rx.forwarded.pkts,
            pdev->stats.pub.rx.forwarded.bytes);
    }
}

int
ol_txrx_stats_publish(ol_txrx_pdev_handle pdev, struct ol_txrx_stats *buf)
{
    adf_os_assert(buf);
    adf_os_assert(pdev);
    adf_os_mem_copy(buf, &pdev->stats.pub, sizeof(pdev->stats.pub));
    return TXRX_STATS_LEVEL;
}

#endif /* TXRX_STATS_LEVEL */

#if defined(ENABLE_TXRX_PROT_ANALYZE)

void
ol_txrx_prot_ans_display(ol_txrx_pdev_handle pdev)
{
    ol_txrx_prot_an_display(pdev->prot_an_tx_sent);
    ol_txrx_prot_an_display(pdev->prot_an_rx_sent);
}

#endif /* ENABLE_TXRX_PROT_ANALYZE */
