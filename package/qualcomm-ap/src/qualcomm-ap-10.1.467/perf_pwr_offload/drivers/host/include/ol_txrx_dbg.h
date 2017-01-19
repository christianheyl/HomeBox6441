/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
/**
 * @file ol_txrx_dbg.h
 * @brief Functions provided for visibility and debugging.
 */
#ifndef _OL_TXRX_DBG__H_
#define _OL_TXRX_DBG__H_

#include <athdefs.h>       /* A_STATUS, u_int64_t */
#include <adf_os_lock.h>   /* adf_os_mutex_t */
#include <htt.h>           /* htt_dbg_stats_type */
#include <ol_txrx_stats.h> /* ol_txrx_stats */

typedef void (*ol_txrx_stats_callback)(
    void *ctxt,
    enum htt_dbg_stats_type type,
    u_int8_t *buf,
    int bytes);

struct ol_txrx_stats_req {
    u_int32_t stats_type_upload_mask; /* which stats to upload */
    u_int32_t stats_type_reset_mask;  /* which stats to reset */

    /* stats will be printed if either print element is set */
    struct {
        int verbose; /* verbose stats printout */
        int concise; /* concise stats printout (takes precedence) */
    } print; /* print uploaded stats */

    /* stats notify callback will be invoked if fp is non-NULL */
    struct {
        ol_txrx_stats_callback fp;
        void *ctxt;
    } callback;

    /* stats will be copied into the specified buffer if buf is non-NULL */
    struct {
        u_int8_t *buf;
        int byte_limit; /* don't copy more than this */
    } copy;

    /*
     * If blocking is true, the caller will take the specified semaphore
     * to wait for the stats to be uploaded, and the driver will release
     * the semaphore when the stats are done being uploaded.
     */
    struct {
        int blocking;
        adf_os_mutex_t *sem_ptr;
    } wait;
};


#if ATH_PERF_PWR_OFFLOAD == 0 /*---------------------------------------------*/

#define ol_txrx_debug(vdev, debug_specs) 0
#define ol_txrx_fw_stats_cfg(vdev, type, val) 0
#define ol_txrx_fw_stats_get(vdev, req) 0
#define ol_txrx_aggr_cfg(vdev, max_subfrms_ampdu, max_subfrms_amsdu) 0

#else /*---------------------------------------------------------------------*/


#include <ol_txrx_api.h> /* ol_txrx_pdev_handle, etc. */

int ol_txrx_debug(ol_txrx_vdev_handle vdev, int debug_specs);

void ol_txrx_fw_stats_cfg(
    ol_txrx_vdev_handle vdev,
    u_int8_t cfg_stats_type,
    u_int32_t cfg_val);

int ol_txrx_fw_stats_get(
    ol_txrx_vdev_handle vdev,
    struct ol_txrx_stats_req *req);

#if QCA_OL_11AC_FAST_PATH
int ol_txrx_host_stats_get(
    ol_txrx_vdev_handle vdev,
    struct ol_txrx_stats_req *req);
void
ol_txrx_host_stats_clr(ol_txrx_vdev_handle vdev);
#endif /* QCA_OL_11AC_FAST_PATH */



#if defined(TEMP_AGGR_CFG)
int ol_txrx_aggr_cfg(ol_txrx_vdev_handle vdev, 
                     int max_subfrms_ampdu, 
                     int max_subfrms_amsdu);
#else
#define ol_txrx_aggr_cfg(vdev, max_subfrms_ampdu, max_subfrms_amsdu) 0
#endif

enum {
    TXRX_DBG_MASK_OBJS             = 0x01,
    TXRX_DBG_MASK_STATS            = 0x02,
    TXRX_DBG_MASK_PROT_ANALYZE     = 0x04,
    TXRX_DBG_MASK_RX_REORDER_TRACE = 0x08,
    TXRX_DBG_MASK_RX_PN_TRACE      = 0x10
};

/*--- txrx printouts ---*/

/*
 * Uncomment this to enable txrx printouts with dynamically adjustable
 * verbosity.  These printouts should not impact performance.
 */
#define TXRX_PRINT_ENABLE 1
/* uncomment this for verbose txrx printouts (may impact performance) */
//#define TXRX_PRINT_VERBOSE_ENABLE 1

void ol_txrx_print_level_set(unsigned level);

/*--- txrx object (pdev, vdev, peer) display debug functions ---*/

#ifndef TXRX_DEBUG_LEVEL
#define TXRX_DEBUG_LEVEL 0 /* no debug info */
#endif

#if TXRX_DEBUG_LEVEL > 5
void ol_txrx_pdev_display(ol_txrx_pdev_handle pdev, int indent);
void ol_txrx_vdev_display(ol_txrx_vdev_handle vdev, int indent);
void ol_txrx_peer_display(ol_txrx_peer_handle peer, int indent);
#else
#define ol_txrx_pdev_display(pdev, indent)
#define ol_txrx_vdev_display(vdev, indent)
#define ol_txrx_peer_display(peer, indent)
#endif

/*--- txrx stats display debug functions ---*/

#if TXRX_STATS_LEVEL != TXRX_STATS_LEVEL_OFF

void ol_txrx_stats_display(ol_txrx_pdev_handle pdev);

int
ol_txrx_stats_publish(ol_txrx_pdev_handle pdev, struct ol_txrx_stats *buf);

#else
#define ol_txrx_stats_display(pdev)
#define ol_txrx_stats_publish(pdev, buf) TXRX_STATS_LEVEL_OFF
#endif /* TXRX_STATS_LEVEL */

/*--- txrx protocol analyzer debug feature ---*/

/* uncomment this to enable the protocol analzyer feature */
//#define ENABLE_TXRX_PROT_ANALYZE 1

#if defined(ENABLE_TXRX_PROT_ANALYZE)

void ol_txrx_prot_ans_display(ol_txrx_pdev_handle pdev);

#else

#define ol_txrx_prot_ans_display(pdev)

#endif /* ENABLE_TXRX_PROT_ANALYZE */

/*--- txrx sequence number trace debug feature ---*/

/* uncomment this to enable the rx reorder trace feature */
//#define ENABLE_RX_REORDER_TRACE 1

#define ol_txrx_seq_num_trace_display(pdev) \
    ol_rx_reorder_trace_display(pdev, 0, 0)

#if defined(ENABLE_RX_REORDER_TRACE)

void
ol_rx_reorder_trace_display(ol_txrx_pdev_handle pdev, int just_once, int limit);

#else

#define ol_rx_reorder_trace_display(pdev, just_once, limit)

#endif /* ENABLE_RX_REORDER_TRACE */

/*--- txrx packet number trace debug feature ---*/

/* uncomment this to enable the rx PN trace feature */
//#define ENABLE_RX_PN_TRACE 1

#define ol_txrx_pn_trace_display(pdev) ol_rx_pn_trace_display(pdev, 0)

#if defined(ENABLE_RX_PN_TRACE)

void
ol_rx_pn_trace_display(ol_txrx_pdev_handle pdev, int just_once);

#else

#define ol_rx_pn_trace_display(pdev, just_once)

#endif /* ENABLE_RX_PN_TRACE */

#endif /* ATH_PERF_PWR_OFFLOAD  */ /*----------------------------------------*/

#endif /* _OL_TXRX_DBG__H_ */
