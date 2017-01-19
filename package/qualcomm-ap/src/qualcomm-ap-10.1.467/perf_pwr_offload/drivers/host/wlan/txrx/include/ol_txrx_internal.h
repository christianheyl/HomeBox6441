/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#ifndef _OL_TXRX_INTERNAL__H_
#define _OL_TXRX_INTERNAL__H_

#include <adf_os_util.h>   /* adf_os_assert */
#include <adf_nbuf.h>      /* adf_nbuf_t */
#include <adf_os_mem.h>    /* adf_os_mem_set */

#include <ol_htt_rx_api.h> /* htt_rx_msdu_desc_completes_mpdu, etc. */

#include <ol_txrx_types.h>

#include <ol_txrx_dbg.h>

#ifndef TXRX_ASSERT_LEVEL
#define TXRX_ASSERT_LEVEL 3
#endif

#if TXRX_ASSERT_LEVEL > 0
#define TXRX_ASSERT1(condition) adf_os_assert((condition))
#else
#define TXRX_ASSERT1(condition)
#endif

#if TXRX_ASSERT_LEVEL > 1
#define TXRX_ASSERT2(condition) adf_os_assert((condition))
#else
#define TXRX_ASSERT2(condition)
#endif

enum {
    /* FATAL_ERR - print only irrecoverable error messages */
    TXRX_PRINT_LEVEL_FATAL_ERR,

    /* ERR - include non-fatal err messages */
    TXRX_PRINT_LEVEL_ERR,

    /* WARN - include warnings */
    TXRX_PRINT_LEVEL_WARN,

    /* INFO1 - include fundamental, infrequent events */
    TXRX_PRINT_LEVEL_INFO1,

    /* INFO2 - include non-fundamental but infrequent events */
    TXRX_PRINT_LEVEL_INFO2,

    /* INFO3 - include frequent events */
    /* to avoid performance impact, don't use INFO3 unless explicitly enabled */
    #ifdef TXRX_PRINT_VERBOSE_ENABLE
    TXRX_PRINT_LEVEL_INFO3,
    #endif /* TXRX_PRINT_VERBOSE_ENABLE */
};

extern unsigned g_txrx_print_level;

#ifdef TXRX_PRINT_ENABLE

#include <stdarg.h>       /* va_list */
#include <adf_os_types.h> /* adf_os_vprint */

#define ol_txrx_print(level, fmt, ...) \
    if(level <= g_txrx_print_level) adf_os_print(fmt, ## __VA_ARGS__)
#define TXRX_PRINT(level, fmt, ...) \
    ol_txrx_print(level, "TXRX: " fmt, ## __VA_ARGS__)

#ifdef TXRX_PRINT_VERBOSE_ENABLE

#define ol_txrx_print_verbose(fmt, ...) \
    if(TXRX_PRINT_LEVEL_INFO3 <= g_txrx_print_level) adf_os_print(fmt, ## __VA_ARGS__)
#define TXRX_PRINT_VERBOSE(fmt, ...) \
    ol_txrx_print_verbose("TXRX: " fmt, ## __VA_ARGS__)
#else
#define TXRX_PRINT_VERBOSE(fmt, ...)
#endif /* TXRX_PRINT_VERBOSE_ENABLE */

#else
#define TXRX_PRINT(level, fmt, ...)
#define TXRX_PRINT_VERBOSE(fmt, ...)
#endif /* TXRX_PRINT_ENABLE */


#define OL_TXRX_LIST_APPEND(head, tail, elem) \
do {                                            \
    if (!(head)) {                              \
        (head) = (elem);                        \
    } else {                                    \
        adf_nbuf_set_next((tail), (elem));      \
    }                                           \
    (tail) = (elem);                            \
} while (0)

static inline void
ol_rx_mpdu_list_next(
    struct ol_txrx_pdev_t *pdev,
    void *mpdu_list,
    adf_nbuf_t *mpdu_tail,
    adf_nbuf_t *next_mpdu)
{
    htt_pdev_handle htt_pdev = pdev->htt_pdev;
    adf_nbuf_t msdu;

    /*
     * For now, we use a simply flat list of MSDUs.
     * So, traverse the list until we reach the last MSDU within the MPDU.
     */
    TXRX_ASSERT2(mpdu_list);
    msdu = mpdu_list;
    while (!htt_rx_msdu_desc_completes_mpdu(
                htt_pdev, htt_rx_msdu_desc_retrieve(htt_pdev, msdu)))
    {
        msdu = adf_nbuf_next(msdu);
        TXRX_ASSERT2(msdu);
    }
    /* msdu now points to the last MSDU within the first MPDU */
    *mpdu_tail = msdu;
    *next_mpdu = adf_nbuf_next(msdu);
}


/*--- txrx stats macros ---*/


/* unconditional defs */
#define TXRX_STATS_INCR(pdev, field) TXRX_STATS_ADD(pdev, field, 1)

/* default conditional defs (may be undefed below) */

#define TXRX_STATS_INIT(_pdev) \
    adf_os_mem_set(&((_pdev)->stats), 0x0, sizeof((_pdev)->stats))
#define TXRX_STATS_ADD(_pdev, _field, _delta) \
    _pdev->stats._field += _delta
#define TXRX_STATS_SUB(_pdev, _field, _delta) \
    _pdev->stats._field -= _delta
#define TXRX_STATS_MSDU_INCR(pdev, field, netbuf) \
    do { \
        TXRX_STATS_INCR((pdev), pub.field.pkts); \
        TXRX_STATS_ADD((pdev), pub.field.bytes, adf_nbuf_len(netbuf)); \
    } while (0)

/* conditional defs based on verbosity level */

#if /*---*/ TXRX_STATS_LEVEL == TXRX_STATS_LEVEL_FULL

#define TXRX_STATS_MSDU_LIST_INCR(pdev, field, netbuf_list) \
    do { \
        adf_nbuf_t tmp_list = netbuf_list; \
        while (tmp_list) { \
            TXRX_STATS_MSDU_INCR(pdev, field, tmp_list); \
            tmp_list = adf_nbuf_next(tmp_list); \
        } \
    } while (0)

#define TXRX_STATS_MSDU_INCR_TX_STATUS(status, pdev, netbuf) \
    switch (status) { \
    case htt_tx_status_ok: \
        TXRX_STATS_MSDU_INCR(pdev, tx.delivered, netbuf); \
        break; \
    case htt_tx_status_discard: \
        TXRX_STATS_MSDU_INCR(pdev, tx.dropped.target_discard, netbuf); \
        break; \
    case htt_tx_status_no_ack: \
        TXRX_STATS_MSDU_INCR(pdev, tx.dropped.no_ack, netbuf); \
        break; \
    case htt_tx_status_download_fail: \
        TXRX_STATS_MSDU_INCR(pdev, tx.dropped.download_fail, netbuf); \
        break; \
    default: \
        break; \
    }

#define TXRX_STATS_UPDATE_TX_STATS(_pdev, _status, _p_cntrs, _b_cntrs)          \
do {                                                                            \
    switch (status) {                                                           \
    case htt_tx_status_ok:                                                      \
       TXRX_STATS_ADD(_pdev, pub.tx.delivered.pkts, _p_cntrs);                  \
       TXRX_STATS_ADD(_pdev, pub.tx.delivered.bytes, _b_cntrs);                 \
        break;                                                                  \
    case htt_tx_status_discard:                                                 \
       TXRX_STATS_ADD(_pdev, pub.tx.dropped.target_discard.pkts, _p_cntrs);     \
       TXRX_STATS_ADD(_pdev, pub.tx.dropped.target_discard.bytes, _b_cntrs);    \
        break;                                                                  \
    case htt_tx_status_no_ack:                                                  \
       TXRX_STATS_ADD(_pdev, pub.tx.dropped.no_ack.pkts, _p_cntrs);             \
       TXRX_STATS_ADD(_pdev, pub.tx.dropped.no_ack.bytes, _b_cntrs);            \
        break;                                                                  \
    case htt_tx_status_download_fail:                                           \
       TXRX_STATS_ADD(_pdev, pub.tx.dropped.download_fail.pkts, _p_cntrs);      \
       TXRX_STATS_ADD(_pdev, pub.tx.dropped.download_fail.bytes, _b_cntrs);     \
        break;                                                                  \
    default:                                                                    \
        break;                                                                  \
    }                                                                           \
} while (0)

#elif /*---*/ TXRX_STATS_LEVEL == TXRX_STATS_LEVEL_BASIC

#define TXRX_STATS_MSDU_LIST_INCR(pdev, field, netbuf_list)

#define TXRX_STATS_MSDU_INCR_TX_STATUS(status, pdev, netbuf) \
    do { \
        if (status == htt_tx_status_ok) { \
            TXRX_STATS_MSDU_INCR(pdev, tx.delivered, netbuf); \
        } \
    } while (0)

#define TXRX_STATS_INIT(_pdev) \
    adf_os_mem_set(&((_pdev)->stats), 0x0, sizeof((_pdev)->stats))

#define TXRX_STATS_UPDATE_TX_STATS(_pdev, _status, _p_cntrs, _b_cntrs)          \
do {                                                                            \
    if (adf_os_likely(_status == htt_tx_status_ok)) {                           \
       TXRX_STATS_ADD(_pdev, pub.tx.delivered.pkts, _p_cntrs);                  \
       TXRX_STATS_ADD(_pdev, pub.tx.delivered.bytes, _b_cntrs);                 \
    }                                                                           \
} while (0)

#else /*---*/ /* stats off */

#undef  TXRX_STATS_INIT
#define TXRX_STATS_INIT(_pdev)

#undef  TXRX_STATS_ADD
#define TXRX_STATS_ADD(_pdev, _field, _delta)
#define TXRX_STATS_SUB(_pdev, _field, _delta)

#undef  TXRX_STATS_MSDU_INCR
#define TXRX_STATS_MSDU_INCR(pdev, field, netbuf)

#define TXRX_STATS_MSDU_LIST_INCR(pdev, field, netbuf_list)

#define TXRX_STATS_MSDU_INCR_TX_STATUS(status, pdev, netbuf)

#define TXRX_STATS_UPDATE_TX_STATS(_pdev, _status, _p_cntrs, _b_cntrs)

#endif /*---*/ /* TXRX_STATS_LEVEL */


/*--- txrx sequence number trace macros ---*/


#define TXRX_SEQ_NUM_ERR(_status) (0xffff - _status)

#if defined(ENABLE_RX_REORDER_TRACE)

A_STATUS ol_rx_reorder_trace_attach(ol_txrx_pdev_handle pdev);
void ol_rx_reorder_trace_detach(ol_txrx_pdev_handle pdev);
void ol_rx_reorder_trace_add(
    ol_txrx_pdev_handle pdev,
    u_int8_t tid,
    u_int16_t seq_num,
    int num_mpdus);

#define OL_RX_REORDER_TRACE_ATTACH ol_rx_reorder_trace_attach
#define OL_RX_REORDER_TRACE_DETACH ol_rx_reorder_trace_detach
#define OL_RX_REORDER_TRACE_ADD    ol_rx_reorder_trace_add

#else

#define OL_RX_REORDER_TRACE_ATTACH(_pdev) A_OK
#define OL_RX_REORDER_TRACE_DETACH(_pdev)
#define OL_RX_REORDER_TRACE_ADD(pdev, tid, seq_num, num_mpdus)

#endif /* ENABLE_RX_REORDER_TRACE */


/*--- txrx packet number trace macros ---*/


#if defined(ENABLE_RX_PN_TRACE)

A_STATUS ol_rx_pn_trace_attach(ol_txrx_pdev_handle pdev);
void ol_rx_pn_trace_detach(ol_txrx_pdev_handle pdev);
void ol_rx_pn_trace_add(
    struct ol_txrx_pdev_t *pdev,
    struct ol_txrx_peer_t *peer,
    u_int16_t tid,
    void *rx_desc);

#define OL_RX_PN_TRACE_ATTACH ol_rx_pn_trace_attach
#define OL_RX_PN_TRACE_DETACH ol_rx_pn_trace_detach
#define OL_RX_PN_TRACE_ADD    ol_rx_pn_trace_add

#else

#define OL_RX_PN_TRACE_ATTACH(_pdev) A_OK
#define OL_RX_PN_TRACE_DETACH(_pdev)
#define OL_RX_PN_TRACE_ADD(pdev, peer, tid, rx_desc)

#endif /* ENABLE_RX_PN_TRACE */


#endif /* _OL_TXRX_INTERNAL__H_ */
