/* 
 * Copyright (c) 2012 Qualcomm Atheros, Inc. 
 * All Rights Reserved. 
 * Qualcomm Atheros Confidential and Proprietary. 
 * $ATH_LICENSE_TARGET_C$
 */

#ifndef _AR_WAL_DBG_STATS_
#define _AR_WAL_DBG_STATS_

/*
 * NB: it is important to keep all the fields in the structure dword long
 * so that it is easy to handle the statistics in BE host.
 */

struct wal_dbg_tx_stats {
    /* Num HTT cookies queued to dispatch list */
    A_INT32 comp_queued;
    /* Num HTT cookies dispatched */
    A_INT32 comp_delivered;
    /* Num MSDU queued to WAL */
    A_INT32 msdu_enqued;
    /* Num MPDU queue to WAL */
    A_INT32 mpdu_enqued;
    /* Num MSDUs dropped by WMM limit */
    A_INT32 wmm_drop;
    /* Num Local frames queued */
    A_INT32 local_enqued;
    /* Num Local frames done */
    A_INT32 local_freed;
    /* Num queued to HW */
    A_INT32 hw_queued;
    /* Num PPDU reaped from HW */
    A_INT32 hw_reaped;
    /* Num underruns */
    A_INT32 underrun;
    /* Num PPDUs cleaned up in TX abort */
    A_INT32 tx_abort;
    /* Num MPDUs requed by SW */
    A_INT32 mpdus_requed;
    /* excessive retries */
    A_UINT32 tx_ko; 
    /* data hw rate code */
    A_UINT32 data_rc;
    /* Scheduler self triggers */
    A_UINT32 self_triggers;
    /* frames dropped due to excessive sw retries */
    A_UINT32 sw_retry_failure;
    /* illegal rate phy errors  */
    A_UINT32 illgl_rate_phy_err;
    /* wal pdev continous xretry */
    A_UINT32 pdev_cont_xretry;
    /* wal pdev continous xretry */
    A_UINT32 pdev_tx_timeout;
    /* wal pdev resets  */
    A_UINT32 pdev_resets;
    /* frames dropped due to non-availability of stateless TIDs */
    A_UINT32 stateless_tid_alloc_failure;

    A_UINT32 phy_underrun;
    /* MPDU is more than txop limit */
    A_UINT32 txop_ovf;
};

struct wal_dbg_rx_stats {
    /* Cnts any change in ring routing mid-ppdu */
    A_INT32 mid_ppdu_route_change;
    /* Total number of statuses processed */
    A_INT32 status_rcvd;
    /* Extra frags on rings 0-3 */
    A_INT32 r0_frags;
    A_INT32 r1_frags;
    A_INT32 r2_frags;
    A_INT32 r3_frags;
    /* MSDUs / MPDUs delivered to HTT */
    A_INT32 htt_msdus;
    A_INT32 htt_mpdus;
    /* MSDUs / MPDUs delivered to local stack */
    A_INT32 loc_msdus;
    A_INT32 loc_mpdus;
    /* AMSDUs that have more MSDUs than the status ring size */
    A_INT32 oversize_amsdu;
    /* Number of PHY errors */
    A_INT32 phy_errs;
    /* Number of PHY errors drops */
    A_INT32 phy_err_drop;
    /* Number of mpdu errors - FCS, MIC, ENC etc. */
    A_INT32 mpdu_errs;
};

struct wal_dbg_peer_stats {

	A_INT32 dummy; /* REMOVE THIS ONCE REAL PEER STAT COUNTERS ARE ADDED */
};

typedef struct {
    A_UINT32 mcs[10];
    A_UINT32 sgi[10];
    A_UINT32 nss[4];
    A_UINT32 nsts;
    A_UINT32 stbc[10];
    A_UINT32 bw[3];
    A_UINT32 pream[6];
    A_UINT32 ldpc;
    A_UINT32 txbf;
    A_UINT32 mgmt_rssi;
    A_UINT32 data_rssi;
    A_UINT32 rssi_chain0;
    A_UINT32 rssi_chain1;
    A_UINT32 rssi_chain2;

} wal_dbg_rx_rate_info_t ;

typedef struct {
    A_UINT32 mcs[10];
    A_UINT32 sgi[10];
    A_UINT32 nss[3];
    A_UINT32 stbc[10];
    A_UINT32 bw[3];
    A_UINT32 pream[4];
    A_UINT32 ldpc;
    A_UINT32 rts_cnt;
    A_UINT32 ack_rssi;
} wal_dbg_tx_rate_info_t ;

typedef struct {
    wal_dbg_rx_rate_info_t rx_phy_info;
    wal_dbg_tx_rate_info_t tx_rate_info;
} wal_dbg_rate_info_t;

/* Add functional stats in groups */

struct wal_dbg_stats {
    struct wal_dbg_tx_stats tx;
    struct wal_dbg_rx_stats rx;
    struct wal_dbg_peer_stats peer;
};

struct ol_ath_radiostats {
    A_UINT64 tx_beacon;
    A_INT32 tx_mgmt;
    A_INT32 rx_mgmt;
    A_UINT32 rx_crcerr;
    A_UINT32 rx_rssi;
    A_UINT32 rx_phyerr;
    A_UINT8 ap_stats_tx_cal_enable;
};

#endif
