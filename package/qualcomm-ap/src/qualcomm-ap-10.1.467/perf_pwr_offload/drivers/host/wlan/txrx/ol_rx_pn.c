/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#include <adf_nbuf.h>         /* adf_nbuf_t */

#include <ol_htt_rx_api.h>    /* htt_rx_pn_t, etc. */
#include <ol_ctrl_txrx_api.h> /* ol_rx_err */

#include <ol_txrx_internal.h> /* ol_rx_mpdu_list_next */
#include <ol_txrx_types.h>    /* ol_txrx_vdev_t, etc. */
#include <ol_rx_pn.h>  /* our own defs */
#include <ol_rx_fwd.h> /* ol_rx_fwd_check */

int ol_rx_pn_cmp24(
    union htt_rx_pn_t *new_pn,
    union htt_rx_pn_t *old_pn,
    int is_unicast,
    int opmode)
{
    return ((new_pn->pn24 & 0xffffff) <= (old_pn->pn24 & 0xffffff));
}


int ol_rx_pn_cmp48(
    union htt_rx_pn_t *new_pn,
    union htt_rx_pn_t *old_pn,
    int is_unicast,
    int opmode)
{
   return
       ((new_pn->pn48 & 0xffffffffffffULL) <=
        (old_pn->pn48 & 0xffffffffffffULL));
}

int ol_rx_pn_wapi_cmp(
    union htt_rx_pn_t *new_pn,
    union htt_rx_pn_t *old_pn,
    int is_unicast,
    int opmode)
{
    int pn_is_replay = 0;

    if (new_pn->pn128[1] == old_pn->pn128[1]) {
        pn_is_replay =  (new_pn->pn128[0] <= old_pn->pn128[0]);
    } else {
        pn_is_replay =  (new_pn->pn128[1] < old_pn->pn128[1]);
    }

    if (is_unicast) {
        if (opmode == wlan_op_mode_ap) {
            pn_is_replay |= ((new_pn->pn128[0] & 0x1ULL) != 0);
        } else {
            pn_is_replay |= ((new_pn->pn128[0] & 0x1ULL) != 1);
        }
    }
    return pn_is_replay;
}

adf_nbuf_t
ol_rx_pn_check_base(
    struct ol_txrx_vdev_t *vdev,
    struct ol_txrx_peer_t *peer,
    unsigned tid,
    adf_nbuf_t msdu_list)
{
    struct ol_txrx_pdev_t *pdev = vdev->pdev;
    union htt_rx_pn_t *last_pn;
    adf_nbuf_t out_list_head = NULL;
    adf_nbuf_t out_list_tail = NULL;
    adf_nbuf_t mpdu;
    int index; /* unicast vs. multicast */
    int pn_len;
    void *rx_desc;
    int last_pn_valid;

    /* First, check whether the PN check applies */
    rx_desc = htt_rx_msdu_desc_retrieve(pdev->htt_pdev, msdu_list);
    adf_os_assert(htt_rx_msdu_has_wlan_mcast_flag(pdev->htt_pdev, rx_desc));
    index = htt_rx_msdu_is_wlan_mcast(pdev->htt_pdev, rx_desc) ?
        txrx_sec_mcast : txrx_sec_ucast;
    pn_len = pdev->rx_pn[peer->security[index].sec_type].len;
    if (pn_len == 0) {
        return msdu_list;
    }

    last_pn_valid = peer->tids_last_pn_valid[tid];
    last_pn = &peer->tids_last_pn[tid];
    mpdu = msdu_list;
    while (mpdu) {
        adf_nbuf_t mpdu_tail, next_mpdu;
        union htt_rx_pn_t new_pn;
        int pn_is_replay = 0;

        rx_desc = htt_rx_msdu_desc_retrieve(pdev->htt_pdev, mpdu);

        /*
         * Find the last MSDU within this MPDU, and
         * the find the first MSDU within the next MPDU.
         */
        ol_rx_mpdu_list_next(pdev, mpdu, &mpdu_tail, &next_mpdu);

        /* retrieve PN from rx descriptor */
        htt_rx_mpdu_desc_pn(pdev->htt_pdev, rx_desc, &new_pn, pn_len);

        /* if there was no prior PN, there's nothing to check */
        if (last_pn_valid) {
            pn_is_replay = pdev->rx_pn[peer->security[index].sec_type].cmp(
                &new_pn, last_pn, index == txrx_sec_ucast, vdev->opmode);
        } else {
            last_pn_valid = peer->tids_last_pn_valid[tid] = 1;
        }

        if (pn_is_replay) {
            adf_nbuf_t msdu;
            /*
             * This MPDU failed the PN check:
             * 1.  Notify the control SW of the PN failure
             *     (so countermeasures can be taken, if necessary)
             * 2.  Discard all the MSDUs from this MPDU.
             */
            msdu = mpdu;
            TXRX_PRINT(TXRX_PRINT_LEVEL_INFO2,
                "PN check failed - TID %d, peer %p "
                "(%02x:%02x:%02x:%02x:%02x:%02x) %s\n"
                "    old PN (u64 x2)= 0x%08llx %08llx (LSBs = %lld)\n"
                "    new PN (u64 x2)= 0x%08llx %08llx (LSBs = %lld)\n"
                "    new seq num = %d\n",
                tid, peer,
                peer->mac_addr.raw[0], peer->mac_addr.raw[1],
                peer->mac_addr.raw[2], peer->mac_addr.raw[3],
                peer->mac_addr.raw[4], peer->mac_addr.raw[5],
                (index == txrx_sec_ucast) ? "ucast" : "mcast",
                last_pn->pn128[1],
                last_pn->pn128[0],
                last_pn->pn128[0] & 0xffffffffffffULL,
                new_pn.pn128[1],
                new_pn.pn128[0],
                new_pn.pn128[0] & 0xffffffffffffULL,
                htt_rx_mpdu_desc_seq_num(pdev->htt_pdev, rx_desc));
            ol_rx_pn_trace_display(pdev, 1);
            ol_rx_err(
                pdev->ctrl_pdev,
                vdev->vdev_id, peer->mac_addr.raw, tid,
                htt_rx_mpdu_desc_tsf32(pdev->htt_pdev, rx_desc),
                OL_RX_ERR_PN, mpdu);
            /* free all MSDUs within this MPDU */
            do {
                adf_nbuf_t next_msdu;
                next_msdu = adf_nbuf_next(msdu);
                htt_rx_desc_frame_free(pdev->htt_pdev, msdu);
                if (msdu == mpdu_tail) {
                    break;
                } else {
                    msdu = next_msdu;
                }
            } while (1);
        } else {
            /* add the MSDUs from this MPDU to the list of good frames */
            if (!out_list_head) {
                out_list_head = mpdu;
            } else {
                adf_nbuf_set_next(out_list_tail, mpdu);
            }
            out_list_tail = mpdu_tail;
            /*
             * Remember the new PN.
             * For simplicity, just do 2 64-bit word copies to cover the worst
             * case (WAPI), regardless of the length of the PN.
             * This is more efficient than doing a conditional branch to copy
             * only the relevant portion.
             */
            last_pn->pn128[0] = new_pn.pn128[0];
            last_pn->pn128[1] = new_pn.pn128[1];
            OL_RX_PN_TRACE_ADD(pdev, peer, tid, rx_desc);
        }

        mpdu = next_mpdu;
    }
    /* make sure the list is null-terminated */
    if (out_list_tail) {
        adf_nbuf_set_next(out_list_tail, NULL);
    }
    return out_list_head;
}

void
ol_rx_pn_check(
    struct ol_txrx_vdev_t *vdev,
    struct ol_txrx_peer_t *peer,
    unsigned tid,
    adf_nbuf_t msdu_list)
{
    msdu_list = ol_rx_pn_check_base(vdev, peer, tid, msdu_list);
    ol_rx_fwd_check(vdev, peer, tid, msdu_list);
}

#if defined(ENABLE_RX_PN_TRACE)

A_STATUS
ol_rx_pn_trace_attach(ol_txrx_pdev_handle pdev)
{
    int num_elems;

    num_elems = 1 << TXRX_RX_PN_TRACE_SIZE_LOG2;
    pdev->rx_pn_trace.idx = 0;
    pdev->rx_pn_trace.cnt = 0;
    pdev->rx_pn_trace.mask = num_elems - 1;
    pdev->rx_pn_trace.data = adf_os_mem_alloc(
        pdev->osdev, sizeof(*pdev->rx_pn_trace.data) * num_elems);
    if (! pdev->rx_pn_trace.data) {
        return A_ERROR;
    }
    return A_OK;
}

void
ol_rx_pn_trace_detach(ol_txrx_pdev_handle pdev)
{
    adf_os_mem_free(pdev->rx_pn_trace.data);
}

void
ol_rx_pn_trace_add(
    struct ol_txrx_pdev_t *pdev,
    struct ol_txrx_peer_t *peer,
    u_int16_t tid,
    void *rx_desc)
{
    u_int32_t idx = pdev->rx_pn_trace.idx;
    union htt_rx_pn_t pn;
    u_int32_t pn32;
    u_int16_t seq_num;
    u_int8_t  unicast;

    htt_rx_mpdu_desc_pn(pdev->htt_pdev, rx_desc, &pn, 48);
    pn32 = pn.pn48 & 0xffffffff;
    seq_num = htt_rx_mpdu_desc_seq_num(pdev->htt_pdev, rx_desc);
    unicast = ! htt_rx_msdu_is_wlan_mcast(pdev->htt_pdev, rx_desc);

    pdev->rx_pn_trace.data[idx].peer = peer;
    pdev->rx_pn_trace.data[idx].tid = tid;
    pdev->rx_pn_trace.data[idx].seq_num = seq_num;
    pdev->rx_pn_trace.data[idx].unicast = unicast;
    pdev->rx_pn_trace.data[idx].pn32 = pn32;
    pdev->rx_pn_trace.cnt++;
    idx++;
    pdev->rx_pn_trace.idx = idx & pdev->rx_pn_trace.mask;
}

void
ol_rx_pn_trace_display(ol_txrx_pdev_handle pdev, int just_once)
{
    static int print_count = 0;
    u_int32_t i, start, end;
    u_int64_t cnt;
    int elems;
    int limit = 0; /* move this to the arg list? */

    if (print_count != 0 && just_once) {
        return;
    }
    print_count++;

    end = pdev->rx_pn_trace.idx;
    if (pdev->rx_pn_trace.cnt <= pdev->rx_pn_trace.mask) {
        /* trace log has not yet wrapped around - start at the top */
        start = 0;
        cnt = 0;
    } else {
        start = end;
        cnt = pdev->rx_pn_trace.cnt - (pdev->rx_pn_trace.mask + 1);
    }
    elems = (end - 1 - start) & pdev->rx_pn_trace.mask;
    if (limit > 0 && elems > limit) {
        int delta;
        delta = elems - limit;
        start += delta;
        start &= pdev->rx_pn_trace.mask;
        cnt += delta;
    }

    i = start;
    adf_os_print("                                 seq     PN\n");
    adf_os_print("   count  idx    peer   tid uni  num    LSBs\n");
    do {
        adf_os_print("  %6lld %4d  %p %2d   %d %4d %8d\n",
            cnt, i,
            pdev->rx_pn_trace.data[i].peer,
            pdev->rx_pn_trace.data[i].tid,
            pdev->rx_pn_trace.data[i].unicast,
            pdev->rx_pn_trace.data[i].seq_num,
            pdev->rx_pn_trace.data[i].pn32);
        cnt++;
        i++;
        i &= pdev->rx_pn_trace.mask;
    } while (i != end); 
}
#endif /* ENABLE_RX_PN_TRACE */
