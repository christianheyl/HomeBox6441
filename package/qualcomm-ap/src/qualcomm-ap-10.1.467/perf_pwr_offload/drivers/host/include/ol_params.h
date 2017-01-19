/*
 * Copyright (c) 2010, Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Defintions for the Atheros Wireless LAN controller driver.
 */
#ifndef _DEV_OL_PARAMS_H
#define _DEV_OL_PARAMS_H
#include "wal_dbg_stats.h"
#include "ol_txrx_stats.h"
/*
** Enumeration of PDEV Configuration parameter 
*/

typedef enum _ol_ath_param_t {
    OL_ATH_PARAM_TXCHAINMASK               = 1,
    OL_ATH_PARAM_RXCHAINMASK               = 2,
    OL_ATH_PARAM_AMPDU                     = 6,
    OL_ATH_PARAM_AMPDU_LIMIT               = 7,
    OL_ATH_PARAM_AMPDU_SUBFRAMES           = 8,
    OL_ATH_PARAM_TXPOWER_LIMIT2G           = 12,
    OL_ATH_PARAM_TXPOWER_LIMIT5G           = 13,
    OL_ATH_PARAM_LDPC                      = 32,
    OL_ATH_PARAM_VOW_EXT_STATS             = 45,
    OL_ATH_PARAM_DYN_TX_CHAINMASK          = 73,
    OL_ATH_PARAM_BURST_ENABLE              = 77,
    OL_ATH_PARAM_BURST_DUR                 = 78,
    OL_ATH_PARAM_BCN_BURST                 = 80,
    OL_ATH_PARAM_DCS                       = 82,
#if UMAC_SUPPORT_PERIODIC_PERFSTATS
    OL_ATH_PARAM_PRDPERFSTAT_THRPUT_ENAB   = 83,
    OL_ATH_PARAM_PRDPERFSTAT_THRPUT_WIN    = 84,
    OL_ATH_PARAM_PRDPERFSTAT_THRPUT        = 85,
    OL_ATH_PARAM_PRDPERFSTAT_PER_ENAB      = 86,
    OL_ATH_PARAM_PRDPERFSTAT_PER_WIN       = 87,
    OL_ATH_PARAM_PRDPERFSTAT_PER           = 88,
#endif                              /* UMAC_SUPPORT_PERIODIC_PERFSTATS */
    OL_ATH_PARAM_TOTAL_PER                 = 89,
    OL_ATH_PARAM_DCS_COCH_THR              = 93, /** co channel interference threshold level */
    OL_ATH_PARAM_DCS_TXERR_THR             = 94, /** transmit error threshold */
    OL_ATH_PARAM_DCS_PHYERR_THR            = 95, /** phy error threshold */
    /*  The IOCTL number is 114, it is made 114, inorder to make the IOCTL 
        number same as Direct-attach IOCTL.
        Please, don't change number. This IOCTL gets the Interface code path,
        it should be either DIRECT-ATTACH or OFF-LOAD.
    */
    OL_ATH_PARAM_GET_IF_ID      = 114,
    OL_ATH_PARAM_ARPDHCP_AC_OVERRIDE,
    OL_ATH_PARAM_NON_AGG_SW_RETRY_TH,
    OL_ATH_PARAM_AGG_SW_RETRY_TH,
    OL_ATH_PARAM_STA_KICKOUT_TH,
    OL_ATH_PARAM_WLAN_PROF_ENABLE,
    OL_ATH_PARAM_LTR_ENABLE,
    OL_ATH_PARAM_LTR_AC_LATENCY_BE,
    OL_ATH_PARAM_LTR_AC_LATENCY_BK,
    OL_ATH_PARAM_LTR_AC_LATENCY_VI,
    OL_ATH_PARAM_LTR_AC_LATENCY_VO,
    OL_ATH_PARAM_LTR_AC_LATENCY_TIMEOUT,
    OL_ATH_PARAM_LTR_TX_ACTIVITY_TIMEOUT,
    OL_ATH_PARAM_LTR_SLEEP_OVERRIDE,
    OL_ATH_PARAM_LTR_RX_OVERRIDE,
    OL_ATH_PARAM_L1SS_ENABLE,
    OL_ATH_PARAM_DSLEEP_ENABLE,
    OL_ATH_PARAM_DCS_RADAR_ERR_THR ,    /** radar error threshold */
    OL_ATH_PARAM_DCS_USERMAX_CU_THR,    /** Tx channel utilization due to AP's tx and rx */
    OL_ATH_PARAM_DCS_INTR_DETECT_THR,   /** interference detection threshold */
    OL_ATH_PARAM_DCS_SAMPLE_WINDOW,     /** sampling window, default 10secs */
    OL_ATH_PARAM_DCS_DEBUG,             /** debug logs enable/disable */
    OL_ATH_PARAM_ANI_ENABLE,
    OL_ATH_PARAM_ANI_POLL_PERIOD,
    OL_ATH_PARAM_ANI_LISTEN_PERIOD,  
    OL_ATH_PARAM_ANI_OFDM_LEVEL,
    OL_ATH_PARAM_ANI_CCK_LEVEL,
    OL_ATH_PARAM_DSCP_TID_MAP,
    OL_ATH_PARAM_TXPOWER_SCALE,
    OL_ATH_PARAM_DCS_PHYERR_PENALTY,   /** Phy error penalty */
} ol_ath_param_t;

/*
** Enumeration of PDEV Configuration parameter 
*/

typedef enum _ol_hal_param_t {
    OL_HAL_CONFIG_DMA_BEACON_RESPONSE_TIME         = 0
} ol_hal_param_t;


/*
** structure to hold all stats information 
** for offload device interface
*/
struct ol_stats {
    int txrx_stats_level;
    struct ol_txrx_stats txrx_stats;
    struct wal_dbg_stats stats;
    struct ol_ath_radiostats interface_stats;
};

#endif /* _DEV_OL_PARAMS_H  */
