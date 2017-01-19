/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

/*=== header file includes ===*/
/* generic utilities */
#include <adf_nbuf.h>          /* adf_nbuf_t, etc. */
#include <adf_os_mem.h>        /* adf_os_mem_alloc */

/* external interfaces */
#include <ol_txrx_api.h>       /* ol_txrx_pdev_handle */
#include <ol_txrx_htt_api.h>   /* ol_rx_addba_handler, etc. */
#include <ol_htt_rx_api.h>     /* htt_rx_desc_frame_free */

/* datapath internal interfaces */
#include <ol_txrx_peer_find.h> /* ol_txrx_peer_find_by_id */
#include <ol_txrx_internal.h>  /* TXRX_ASSERT */
#include <ol_rx_reorder.h>
#include <ol_rx_defrag.h>


/*=== data types and defines ===*/
#define OL_RX_REORDER_ROUND_PWR2(value) g_log2ceil[value]

/*=== global variables ===*/

static char g_log2ceil[] = {
    1, // 0 -> 1
    1, // 1 -> 1
    2, // 2 -> 2
    4, 4, // 3-4 -> 4
    8, 8, 8, 8, // 5-8 -> 8
    16, 16, 16, 16, 16, 16, 16, 16, // 9-16 -> 16
    32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, // 17-32 -> 32
    64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, // 33-64 -> 64
};


/*=== function definitions ===*/

/* functions called by txrx components */

void ol_rx_reorder_init(struct ol_rx_reorder_t *rx_reorder, int tid)
{
    rx_reorder->win_sz_mask = 0;
    rx_reorder->array = &rx_reorder->base;
    rx_reorder->base.head = rx_reorder->base.tail = NULL;
    rx_reorder->tid = tid;
    rx_reorder->defrag_timeout_ms = 0;

    rx_reorder->defrag_waitlist_elem.tqe_next = NULL;
    rx_reorder->defrag_waitlist_elem.tqe_prev = NULL;
}

int
ol_rx_reorder_store(
    struct ol_txrx_pdev_t *pdev,
    struct ol_txrx_peer_t *peer,
    unsigned tid,
    unsigned seq_num,
    adf_nbuf_t head_msdu,
    adf_nbuf_t tail_msdu)
{
    struct ol_rx_reorder_array_elem_t *rx_reorder_array_elem;
    TXRX_ASSERT2(tid < OL_TXRX_NUM_EXT_TIDS);

    /*
     * if host duplicate detection is enabled and aggregation is not
     * on, check if the incoming sequence number is a duplication of
     * the one just received.
     */
    if (pdev->rx.flags.dup_check && peer->tids_rx_reorder[tid].win_sz_mask == 0) {
        /*
         * TBDXXX: should we check the retry bit is set or not? A strict
         * duplicate packet should be the one with retry bit set;
         * however, since many implementations do not set the retry bit,
         * ignore the check for now.
         */
        if (seq_num == peer->tids_last_seq[tid]) {
            return -1;
        } 

    }

    peer->tids_last_seq[tid] = seq_num;

    seq_num &= peer->tids_rx_reorder[tid].win_sz_mask;
    rx_reorder_array_elem = &peer->tids_rx_reorder[tid].array[seq_num];
    if (rx_reorder_array_elem->head) {
        adf_nbuf_set_next(rx_reorder_array_elem->tail, head_msdu);
    } else {
        rx_reorder_array_elem->head = head_msdu;
    }
    rx_reorder_array_elem->tail = tail_msdu;

    return 0;
}

void
ol_rx_reorder_release(
    struct ol_txrx_vdev_t *vdev,
    struct ol_txrx_peer_t *peer,
    unsigned tid,
    unsigned seq_num_start,
    unsigned seq_num_end)
{
    unsigned seq_num;
    unsigned win_sz_mask;
    struct ol_rx_reorder_array_elem_t *rx_reorder_array_elem;
    adf_nbuf_t head_msdu;
    adf_nbuf_t tail_msdu;

    win_sz_mask = peer->tids_rx_reorder[tid].win_sz_mask;
    seq_num_start &= win_sz_mask;
    seq_num_end   &= win_sz_mask;
    rx_reorder_array_elem = &peer->tids_rx_reorder[tid].array[seq_num_start];

    head_msdu = rx_reorder_array_elem->head;
    tail_msdu = rx_reorder_array_elem->tail;
    if ((head_msdu == NULL) || ( tail_msdu == NULL)) {        
        printk("%s : head/tail msdu is null. peer %p (%s) tid %d seq_start %d seq_end %d win_sz_mask %d seq_num %d head %p tail %p  \n",
                __func__, peer, ether_sprintf(peer->mac_addr.raw), tid, seq_num_start, seq_num_end, win_sz_mask,seq_num, 
                rx_reorder_array_elem->head, rx_reorder_array_elem->tail);
        return ;
    }
    rx_reorder_array_elem->head = rx_reorder_array_elem->tail = NULL;
    seq_num = (seq_num_start + 1) & win_sz_mask;
    while (seq_num != seq_num_end) {
        rx_reorder_array_elem = &peer->tids_rx_reorder[tid].array[seq_num];
        /* Check if reorder array element is NULL */
        if (rx_reorder_array_elem->head) {
            adf_nbuf_set_next(tail_msdu, rx_reorder_array_elem->head);
            tail_msdu = rx_reorder_array_elem->tail;
            rx_reorder_array_elem->head = rx_reorder_array_elem->tail = NULL;
        } else {
            printk("%s : reorder_array_element is null. peer %p (%s) tid %d seq_start %d seq_end %d win_sz_mask %d seq_num %d head %p tail %p  \n",
                    __func__, peer, ether_sprintf(peer->mac_addr.raw), tid, seq_num_start, seq_num_end, win_sz_mask,seq_num, 
                    rx_reorder_array_elem->head, rx_reorder_array_elem->tail);
        }
        seq_num++;
        seq_num &= win_sz_mask;
    }
    if(tail_msdu == NULL) {
        printk("%s : tail msdu is null. peer %p (%s) tid %d seq_start %d seq_end %d win_sz_mask %d seq_num %d head %p tail %p  \n",
                __func__, peer, ether_sprintf(peer->mac_addr.raw), tid, seq_num_start, seq_num_end, win_sz_mask,seq_num, 
                rx_reorder_array_elem->head, rx_reorder_array_elem->tail);
        return;
    }

    /* rx_opt_proc takes a NULL-terminated list of msdu netbufs */
    adf_nbuf_set_next(tail_msdu, NULL);
    peer->rx_opt_proc(vdev, peer, tid, head_msdu);
}

void
ol_rx_reorder_flush(
    struct ol_txrx_vdev_t *vdev,
    struct ol_txrx_peer_t *peer,
    unsigned tid,
    unsigned seq_num_start,
    unsigned seq_num_end,
    enum htt_rx_flush_action action)
{
    struct ol_txrx_pdev_t *pdev;
    unsigned win_sz_mask;
    struct ol_rx_reorder_array_elem_t *rx_reorder_array_elem;
    adf_nbuf_t head_msdu = NULL;
    adf_nbuf_t tail_msdu = NULL;

    pdev = vdev->pdev;
    win_sz_mask = peer->tids_rx_reorder[tid].win_sz_mask;
    seq_num_start &= win_sz_mask;
    seq_num_end   &= win_sz_mask;

    do {
        rx_reorder_array_elem = 
            &peer->tids_rx_reorder[tid].array[seq_num_start];
        seq_num_start = (seq_num_start + 1) & win_sz_mask;

        if (rx_reorder_array_elem->head) {
            if (head_msdu == NULL) {
                head_msdu = rx_reorder_array_elem->head;
                tail_msdu = rx_reorder_array_elem->tail;
                rx_reorder_array_elem->head = NULL;
                rx_reorder_array_elem->tail = NULL;
                continue;
            }
            adf_nbuf_set_next(tail_msdu, rx_reorder_array_elem->head);
            tail_msdu = rx_reorder_array_elem->tail;
            rx_reorder_array_elem->head = rx_reorder_array_elem->tail = NULL;
        }
    } while (seq_num_start != seq_num_end);

    ol_rx_defrag_waitlist_remove(peer, tid);

    if (head_msdu) {
        /* rx_opt_proc takes a NULL-terminated list of msdu netbufs */
        adf_nbuf_set_next(tail_msdu, NULL);
        if (action == htt_rx_flush_release) {
            peer->rx_opt_proc(vdev, peer, tid, head_msdu);
        } else {
            do {
                adf_nbuf_t next;
                next = adf_nbuf_next(head_msdu);
                htt_rx_desc_frame_free(pdev->htt_pdev, head_msdu);
                head_msdu = next;
            } while (head_msdu);
        }
    }
}

void
ol_rx_reorder_peer_cleanup(
    struct ol_txrx_vdev_t *vdev, struct ol_txrx_peer_t *peer)
{
    int tid;
    for (tid = 0; tid < OL_TXRX_NUM_EXT_TIDS; tid++) {
        ol_rx_reorder_flush(vdev, peer, tid, 0, 0, htt_rx_flush_discard);
    }
}

/* functions called by HTT */

void
ol_rx_addba_handler(
    ol_txrx_pdev_handle pdev,
    u_int16_t peer_id,
    u_int8_t tid,
    u_int8_t win_sz)
{
    unsigned round_pwr2_win_sz, array_size;
    struct ol_txrx_peer_t *peer;
    struct ol_rx_reorder_t *rx_reorder;

    peer = ol_txrx_peer_find_by_id(pdev, peer_id);
    if (peer == NULL) {
        return;
    }
    rx_reorder = &peer->tids_rx_reorder[tid];

    TXRX_ASSERT2(win_sz <= 64);
    round_pwr2_win_sz = OL_RX_REORDER_ROUND_PWR2(win_sz);
    array_size = round_pwr2_win_sz * sizeof(struct ol_rx_reorder_array_elem_t);
    rx_reorder->array = adf_os_mem_alloc(pdev->osdev, array_size);
    TXRX_ASSERT1(rx_reorder->array);
    adf_os_mem_set(rx_reorder->array, 0x0, array_size);

    rx_reorder->win_sz_mask = round_pwr2_win_sz - 1;
}

void
ol_rx_delba_handler(
    ol_txrx_pdev_handle pdev,
    u_int16_t peer_id,
    u_int8_t tid)
{
    struct ol_txrx_peer_t *peer;
    struct ol_rx_reorder_t *rx_reorder;

    peer = ol_txrx_peer_find_by_id(pdev, peer_id);
    if (peer == NULL) {
        return;
    }
    rx_reorder = &peer->tids_rx_reorder[tid];

    /* deallocate the old rx reorder array */
    adf_os_mem_free(rx_reorder->array);

    /* set up the TID with default parameters (ARQ window size = 1) */
    ol_rx_reorder_init(rx_reorder, tid);
}

void
ol_rx_flush_handler(
    ol_txrx_pdev_handle pdev,
    u_int16_t peer_id,
    u_int8_t tid,
    int seq_num_start,
    int seq_num_end,
    enum htt_rx_flush_action action)
{
    struct ol_txrx_vdev_t *vdev = NULL;
    void *rx_desc;
    struct ol_txrx_peer_t *peer;
    int seq_num;
    struct ol_rx_reorder_array_elem_t *rx_reorder_array_elem;
    htt_pdev_handle htt_pdev = pdev->htt_pdev;

    peer = ol_txrx_peer_find_by_id(pdev, peer_id);
    if (peer) {
        vdev = peer->vdev;
    } else {
        return;
    } 

    seq_num = seq_num_start & peer->tids_rx_reorder[tid].win_sz_mask;
    rx_reorder_array_elem = &peer->tids_rx_reorder[tid].array[seq_num];
    if (rx_reorder_array_elem->head) {    
        rx_desc =
            htt_rx_msdu_desc_retrieve(htt_pdev, rx_reorder_array_elem->head);
        if (htt_rx_msdu_is_frag(htt_pdev, rx_desc)) {
            ol_rx_reorder_flush_frag(htt_pdev, peer, tid, seq_num_start);
            /*
             * Assuming flush message sent seperately for frags
             * and for normal frames
             */ 
            return;
        }
    }
    ol_rx_reorder_flush(
        vdev, peer, tid, seq_num_start, seq_num_end, action);    
}

#if defined(ENABLE_RX_REORDER_TRACE)

A_STATUS
ol_rx_reorder_trace_attach(ol_txrx_pdev_handle pdev)
{
    int num_elems;

    num_elems = 1 << TXRX_RX_REORDER_TRACE_SIZE_LOG2;
    pdev->rx_reorder_trace.idx = 0;
    pdev->rx_reorder_trace.cnt = 0;
    pdev->rx_reorder_trace.mask = num_elems - 1;
    pdev->rx_reorder_trace.data = adf_os_mem_alloc(
        pdev->osdev, sizeof(*pdev->rx_reorder_trace.data) * num_elems);
    if (! pdev->rx_reorder_trace.data) {
        return A_ERROR;
    }
    while (--num_elems >= 0) {
        pdev->rx_reorder_trace.data[num_elems].seq_num = 0xffff;
    }

    return A_OK;
}

void
ol_rx_reorder_trace_detach(ol_txrx_pdev_handle pdev)
{
    adf_os_mem_free(pdev->rx_reorder_trace.data);
}

void
ol_rx_reorder_trace_add(
    ol_txrx_pdev_handle pdev, u_int8_t tid, u_int16_t seq_num, int num_mpdus)
{
    u_int32_t idx = pdev->rx_reorder_trace.idx;

    pdev->rx_reorder_trace.data[idx].tid = tid;
    pdev->rx_reorder_trace.data[idx].seq_num = seq_num;
    pdev->rx_reorder_trace.data[idx].num_mpdus = num_mpdus;
    pdev->rx_reorder_trace.cnt++;
    idx++;
    pdev->rx_reorder_trace.idx = idx & pdev->rx_reorder_trace.mask;
}

void
ol_rx_reorder_trace_display(ol_txrx_pdev_handle pdev, int just_once, int limit)
{
    static int print_count = 0;
    u_int32_t i, start, end;
    u_int64_t cnt;
    int elems;

    if (print_count != 0 && just_once) {
        return;
    }
    print_count++;

    end = pdev->rx_reorder_trace.idx;
    if (pdev->rx_reorder_trace.data[end].seq_num == 0xffff) {
        /* trace log has not yet wrapped around - start at the top */
        start = 0;
        cnt = 0;
    } else {
        start = end;
        cnt = pdev->rx_reorder_trace.cnt - (pdev->rx_reorder_trace.mask + 1);
    }
    elems = (end - 1 - start) & pdev->rx_reorder_trace.mask;
    if (limit > 0 && elems > limit) {
        int delta;
        delta = elems - limit;
        start += delta;
        start &= pdev->rx_reorder_trace.mask;
        cnt += delta;
    }

    i = start;
    adf_os_print("                     seq\n");
    adf_os_print("   count   idx  tid  num (LSBs)\n");
    do {
        u_int16_t seq_num;
        seq_num = pdev->rx_reorder_trace.data[i].seq_num;
        if (seq_num < (1 << 14)) {
            adf_os_print("  %6lld  %4d  %3d %4d (%d)\n",
                cnt, i, pdev->rx_reorder_trace.data[i].tid,
                seq_num, seq_num & 63);
        } else {
            int err = TXRX_SEQ_NUM_ERR(seq_num);
            adf_os_print("  %6lld  %4d err %d (%d MPDUs)\n",
                cnt, i, err, pdev->rx_reorder_trace.data[i].num_mpdus);
        }
        cnt++;
        i++;
        i &= pdev->rx_reorder_trace.mask;
    } while (i != end);
}

#endif /* ENABLE_RX_REORDER_TRACE */
