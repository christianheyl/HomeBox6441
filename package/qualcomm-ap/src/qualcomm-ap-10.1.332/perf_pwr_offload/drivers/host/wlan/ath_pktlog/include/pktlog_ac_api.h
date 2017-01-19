/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

/* 
 *  The file is used to define structures that are shared between
 *  kernel space and user space pktlog application. 
 */

#ifndef _PKTLOG_AC_API_
#define _PKTLOG_AC_API_
#ifndef REMOVE_PKT_LOG

/**
 * @typedef ol_pktlog_dev_handle
 * @brief opaque handle for pktlog device object
 */
struct ol_pktlog_dev_t;
typedef struct ol_pktlog_dev_t* ol_pktlog_dev_handle;

/**
 * @typedef ol_ath_softc_net80211_handle
 * @brief opaque handle for ol_ath_softc_net80211
 */
struct ol_ath_softc_net80211;
typedef struct ol_ath_softc_net80211* ol_ath_softc_net80211_handle; 

/**
 * @typedef net_device_handle
 * @brief opaque handle linux phy device object 
 */
struct net_device;
typedef struct net_device* net_device_handle;

void ol_pl_set_name(ol_ath_softc_net80211_handle scn, net_device_handle dev);

void ol_pl_sethandle(ol_pktlog_dev_handle *pl_handle,
                    ol_ath_softc_net80211_handle scn);

/* Packet log state information */
#ifndef _PKTLOG_INFO
#define _PKTLOG_INFO
#if !_MAVERICK_STA_
struct ath_pktlog_info {
    struct ath_pktlog_buf *buf;
    u_int32_t log_state;
    u_int32_t saved_state;
    u_int32_t options;
    int32_t buf_size;           /* Size of buffer in bytes */
    spinlock_t log_lock;
    //struct ath_softc *pl_sc;    /*Needed to call 'AirPort_AthrFusion__allocatePktLog' or similar functions */
    int sack_thr;               /* Threshold of TCP SACK packets for triggered stop */
    int tail_length;            /* # of tail packets to log after triggered stop */
    u_int32_t thruput_thresh;           /* throuput threshold in bytes for triggered stop */
	u_int32_t pktlen;          /* (aggregated or single) packet size in bytes */ 
    /* a temporary variable for counting TX throughput only */
    u_int32_t per_thresh;               /* PER threshold for triggered stop, 10 for 10%, range [1, 99] */
    u_int32_t phyerr_thresh;          /* Phyerr threshold for triggered stop */
    u_int32_t trigger_interval;       /* time period for counting trigger parameters, in milisecond */
    u_int32_t start_time_thruput;
    u_int32_t start_time_per;
};
#else
struct ath_pktlog_info {
    struct ath_pktlog_buf *buf;                                                             /* 64-bits */
    u_int32_t log_state __attribute__((aligned(8))); /* keep 64-bit alignment */            /* 32-bits */
    u_int32_t saved_state;                                                                  /* 32-bits */
    u_int32_t options;                                                                      /* 32-bits */
    /* Size of buffer in bytes */
    int32_t buf_size;                                                                       /* 32-bits */
    spinlock_t log_lock __attribute__((aligned(8)));
    //struct ath_softc *pl_sc __attribute__((aligned(8)));                                    /* 64-bits */
    /* Threshold of TCP SACK packets for triggered stop */
    int32_t sack_thr __attribute__((aligned(8)));                                           /* 32-bits */
    /* # of tail packets to log after triggered stop */
    int32_t tail_length;                                                                    /* 32-bits */
    u_int32_t thruput_thresh;           /* throuput threshold in bytes for triggered stop */
	u_int32_t pktlen;          /* (aggregated or single) packet size in bytes */ 
    /* a temporary variable for counting TX throughput only */
    u_int32_t per_thresh;               /* PER threshold for triggered stop, 10 for 10%, range [1, 99] */
    u_int32_t phyerr_thresh;          /* Phyerr threshold for triggered stop */
    u_int32_t trigger_interval;       /* time period for counting trigger parameters, in milisecond */
    u_int32_t start_time_thruput;
    u_int32_t start_time_per;
    /* might be 64-bit client addr with 32-bit kernel */
    u_int32_t rxfilterVal;        /*Store value of AR_RX_FILTER before enabling phyerr and restore after stopping pktlog */  /* 32-bits */
    u_int32_t rxcfgVal;        /*Store val of AR_RXCFG  before enabling phyerr and restore after stopping pktlog */  /* 32-bits */
    u_int32_t phyErrMaskVal;        /*Store val of AR_PHY_ERR before enabling phyerr and restore after stopping pktlog */  /* 32-bits */
    u_int32_t macPcuPhyErrRegval;        /*Store val of 0x8338 before enabling phyerr and restore after stopping pktlog */  /* 32-bits */
    user_addr_t  buf_clientAddr __attribute__((aligned(8)));                                /* 64-bits */
};
#endif /* _MAVERICK_STA_ */
#endif /* _PKTLOG_INFO */

extern char dbglog_print_buffer[1024];

#else  /* REMOVE_PKT_LOG */
typedef void* ol_pktlog_dev_handle;
#define ol_pl_sethandle(pl_handle, scn)
#define ol_pl_set_name(scn, dev)
#endif /* REMOVE_PKT_LOG */
#endif  /* _PKTLOG_AC_API_ */
