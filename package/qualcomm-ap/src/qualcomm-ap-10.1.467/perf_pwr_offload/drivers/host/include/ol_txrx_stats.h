/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
/**
 * @file ol_txrx_status.h
 * @brief Functions provided for visibility and debugging.
 * NOTE: This file is used by both kernel driver SW and userspace SW.
 * Thus, do not reference use any kernel header files or defs in this file!
 */
#ifndef _OL_TXRX_STATS__H_
#define _OL_TXRX_STATS__H_

#include <athdefs.h>       /* u_int64_t */

#define TXRX_STATS_LEVEL_OFF   0
#define TXRX_STATS_LEVEL_BASIC 1
#define TXRX_STATS_LEVEL_FULL  2

#ifndef TXRX_STATS_LEVEL
#define TXRX_STATS_LEVEL TXRX_STATS_LEVEL_BASIC
#endif

/*
 * TODO: Following needs a better #define
 *       Currently this works, since x86-64 can handle
 *       64 bit and db12x / ap135 needs only 32 bits
 */
#ifndef BIG_ENDIAN_HOST 
typedef struct {
   u_int64_t pkts;
   u_int64_t bytes;
} ol_txrx_stats_elem;
#else
struct ol_txrx_elem_t {
   u_int32_t pkts;
   u_int32_t bytes;
};
typedef struct ol_txrx_elem_t ol_txrx_stats_elem;
#endif

/**
 * @brief data stats published by the host txrx layer
 */
struct ol_txrx_stats {
    struct {
        /* MSDUs received from the stack */ 
        ol_txrx_stats_elem from_stack;
        /* MSDUs successfully sent across the WLAN */
        ol_txrx_stats_elem delivered;
        struct {
            /* MSDUs that the host did not accept */
            ol_txrx_stats_elem host_reject;
            /* MSDUs which could not be downloaded to the target */
            ol_txrx_stats_elem download_fail;
            /* MSDUs which the target discarded (lack of mem or old age) */
            ol_txrx_stats_elem target_discard;
            /* MSDUs which the target sent but couldn't get an ack for */
            ol_txrx_stats_elem no_ack;
        } dropped;
        u_int32_t desc_in_use;
        u_int32_t desc_alloc_fails;
        u_int32_t ce_ring_full;
        u_int32_t dma_map_error;
        /* MSDUs given to the txrx layer by the management stack */
        ol_txrx_stats_elem mgmt;
    } tx;
    struct {
        /* MSDUs given to the OS shim */
        ol_txrx_stats_elem delivered;
        /* MSDUs forwarded from the rx path to the tx path */
        ol_txrx_stats_elem forwarded;
    } rx;
};

/*
 * Structure to consolidate host stats
 */
struct ieee80211req_ol_ath_host_stats {
    struct ol_txrx_stats txrx_stats;
    struct {
        int pkt_q_fail_count;
        int pkt_q_empty_count;
        int send_q_empty_count;
    } htc;
    struct {
        int pipe_no_resrc_count;
        int ce_ring_delta_fail_count;
    } hif;
};

#endif /* _OL_TXRX_STATS__H_ */
