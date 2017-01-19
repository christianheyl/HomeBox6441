/*
 * Copyright (c) 2011-2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_TARGET_C$
 */

#ifndef _RATECTRL_H_
#define	_RATECTRL_H_

#if defined(ATH_TARGET)
#include <osapi.h>
#include <whal_api.h>
#else
#include "ratectrl_11ac_types.h"
#include "ol_ratetable.h"
//#include "ieee80211_defs.h"
#include "_ieee80211.h"
#endif
#include "wlan_defs.h"

#if !defined(ATH_TARGET)
/*TODO: A better way to include this info? */
#define WMM_NUM_AC      4   /* 4 AC categories */

#endif

typedef A_UINT32 A_RSSI32;

#define RATE_CODE_2MB_SHORT    0x1e
#define RATE_CODE_55MB_SHORT   0x1d
#define RATE_CODE_11MB_SHORT   0x1c
#define RATE_CODE_INVALID      0x0

#define MAX_RETRY_ENTRIES (NUM_SCHED_ENTRIES)


typedef struct {	
    RC_TX_RATE_SCHEDULE hwSched;
}tRetrySchedule;

#define RATEMASK_CLR_IDX(dstmask, mask, idx)  ((dstmask) = (mask) & ~((A_RATEMASK) 1 << (idx)))
#define RATEMASK_SET_IDX(dstmask, idx)        ((dstmask) = (A_RATEMASK) 1 << (idx))
#define RATEMASK_ADD_IDX(dstmask, mask, idx)  ((dstmask) = (mask) | (A_RATEMASK) 1 << (idx))
#define RATEMASK_CLR_ALL(dstmask, idx)        ((dstmask) = 0)
#define RATEMASK_IS_VALID(mask, idx)          ((mask) & ((A_RATEMASK) 1 << (idx)))
#define RATEMASK_LT_IDX(mask, idx)            ((mask) < ((A_RATEMASK) 1 << (idx)))
#define RATEMASK_GT_IDX(mask, idx)            ((mask) > ((A_RATEMASK) 1 << (idx)))

#define RT_INVAL_IDX                        0xff

#define RATEMASK_GET_VHT_BITS(mask) ( ((mask) >> 61) & 0x7ULL)
#define RATEMASK_CLR_VHT_BITS(mask) ((mask) = (mask) & ~(0x7ULL << 61))
#define RATEMASK_SKIP_VHT_BITS(mask) ((mask) & ~(0x7ULL << 61)) 

#define RT_ARRAY_TO_RATEMASK(array)         ((A_RATEMASK)((array)[0]) | (A_RATEMASK)((array)[1])<<32);
#define RT_RATEMASK_TO_ARRAY(mask, array)   do { \
                                               (array)[0] = (A_UINT32)(mask); \
                                               (array)[1] = (A_UINT32)((mask) >>32); \
                                            } while(0)


#define A_RATEMASK_FULL  ((A_RATEMASK)~0)  
#define A_RATEMASK_CLEAR ((A_RATEMASK)0)
#define RT_HT_AMPDU_TH1     15             /* MCS 3 */
#define RT_HT_AMPDU_TH2     23             /* MCS 11 */
#define RT_HT_ONLY_TH       13             /* MCS 1 */

#define MAX_TX_RATE_TBL RATE_TABLE_SIZE


#define A_RATEMASK_ILLEGAL_VHT_20 ((1 << 9) | (1<< 19)) /* 20M 1x1, 2x2 MCS9 */
#define A_RATEMASK_ILLEGAL_VHT_80 (1 << 26) /* 80M 3x3 MCS6 */
#define A_RATEMASK_HT_ALL ((A_RATEMASK)((1<<(8*(NUM_SPATIAL_STREAM)))-1))
#define A_RATEMASK_VHT_ALL ((A_RATEMASK)((1<<(10*(NUM_SPATIAL_STREAM)))-1))

#define A_RATEMASK_OFDM_CCK ((A_RATEMASK)((1<<12) -1))
#define A_RATEMASK_HT_20 A_RATEMASK_HT_ALL
#define A_RATEMASK_HT_40 A_RATEMASK_HT_ALL
#define A_RATEMASK_VHT_20 (A_RATEMASK_VHT_ALL & ~A_RATEMASK_ILLEGAL_VHT_20)
#define A_RATEMASK_VHT_40 (A_RATEMASK_VHT_ALL)
#define A_RATEMASK_VHT_80 (A_RATEMASK_VHT_ALL & ~A_RATEMASK_ILLEGAL_VHT_80)

#define RC_GET_HT_RATEMASK_4_NSS(nss) ((A_RATEMASK)((1<<(8*(nss)))-1))
#define RC_GET_VHT_RATEMASK_4_NSS(nss) ((A_RATEMASK)((1<<(10*(nss)))-1))

/*
 * State structures for new rate adaptation code
 *
 * NOTE: Modifying these structures will impact
 * the Perl script that parses packet logging data.
 * See the packet logging module for more information.
 */
typedef struct TxRateCrtlState_s {
#ifdef ENABLE_RSSI_BASED_RATECTRL
    A_RSSI rssiThres;           /* required rssi for this rate (dB) */
#endif /* ENABLE_RSSI_BASED_RATECTRL */
    A_UINT8 per;                /* recent estimate of packet error rate (%) */
} TxRateCtrlState;


typedef struct TxRateCtrl_s {
    A_RSSI rssiLast;            /* last ack rssi */
#ifdef ENABLE_RSSI_BASED_RATECTRL
    A_RSSI rssiLastLkup;        /* last ack rssi used for lookup */
    A_RSSI rssiLastPrev;        /* previous last ack rssi */
    A_RSSI rssiLastPrev2;       /* 2nd previous last ack rssi */
    A_RSSI rssiSumCnt;          /* count of rssiSum for averaging */
    A_RSSI rssiSumRate;         /* rate that we are averaging */
    A_RSSI32 rssiSum;           /* running sum of rssi for averaging */
#endif /* ENABLE_RSSI_BASED_RATECTRL */
	/* this value is A_UINT32 for both Mercury and Venus because of original 
	 * mercury code base. */
    A_UINT8 rc_mask_idx; /* idx into validTxRateMask set */
    A_UINT8 rateTableSize;      /* rate table size */
#ifdef ENABLE_RSSI_BASED_RATECTRL
    A_UINT32 rssiTime;          /* msec timestamp for last ack rssi */
    A_UINT32 rssiDownTime;      /* msec timestamp for last down step */
#endif /* ENABLE_RSSI_BASED_RATECTRL */
    A_UINT8 bw_probe_pending;     /* hgher bw probe pending */
    A_UINT8 htSgiValid;			/* b'0 valid at 20Mhz, b'1 valid at 40Mhz, 
                            	b'2 valid at 80MHz, b'3 valid at 160MHz */
    A_UINT8 excessRetries; /* Count of PPDUs that have seen excess retries
                              consecutively or "frequently" */
    A_UINT8 rateMax[NUM_DYN_BW]; /* max rates that has recently worked */
    A_UINT8 probeRate[NUM_DYN_BW];          /* rate we are probing at */
    A_RATEMASK validTxRateMask[NUM_VALID_RC_MASK]; /* mask of valid rates/mask */
    TxRateCtrlState state[MAX_TX_RATE_TBL];    /* state for each rate */
    A_UINT32 probeTime[NUM_DYN_BW];         /* msec timestamp for last probe */
    A_UINT8 hwMaxRetryPktCnt[NUM_DYN_BW]; /* num packets since we got HW max retry error */
    /* we dont want to mix legacy OFDM, HT, VHT rate in one place to make sure
            all iterator functions are optimal --only look for corresponding 
            phy /bw type. However, if we have just one dimension array for 
            this, a sorted sequence of rate would be having all the rates 
            mixed and the next higher functions would take lots of time to 
            search.
       */
    A_UINT8 maxValidRate[NUM_VALID_RC_MASK]; /* maximum number of valid rate*/
    /* valid rate index */
    A_UINT8 validRateIndex[NUM_VALID_RC_MASK][30]; /* Assuming 3x3 VHT */ 
    A_UINT32 perDownTime[NUM_DYN_BW];       /* msec timstamp for last PER down step */
#define TX_RATE_SGI_20 (0x01)
#define TX_RATE_SGI_40 (0x02)
#define TX_RATE_SGI_80 (0x04)
#define TX_RATE_SGI_160 (0x08)
} TX_RATE_CTRL;

typedef struct tx_peer_params_t {
    A_UINT32                   ni_flags;
    A_UINT8                    valid_tx_chainmask;
    WLAN_PHY_MODE              phymode;
    A_UINT8                    ni_ht_mcs_set[MAX_SPATIAL_STREAM];   /* Negotiated HT MCS map */
    A_UINT16                   ni_vht_mcs_set;   /* Negotiated VHT MCS map */
    A_UINT32                   ht_caps;        /* negotiated HT capabilities */
    A_UINT32                   vht_caps;       /* Negotiated VHT capabilities */
} TX_PEER_PARAMS;

/* per-node rate information */
struct rate_node {
    TX_RATE_CTRL   *txRateCtrl;	/* rate control state proper */
    void           *peer;
    TX_PEER_PARAMS peer_params;
    /*
     * id to identify cache object. 
     * used whne target rate control paging functionality.
     */ 
    A_UINT16       cachmgr_id; 
#define RC_FLAGA_CACHE_OBJ_NOT_INITED  0x1   /* ratectrl cache object is not initialized */
#define RC_FLAGA_CACHE_OBJ_NEEDS_UPDATE 0x2  /* ratectrl cache object needs to be updated with rate parameters */
    A_UINT16       rc_flags; 
};

/* Vdev Context */
#define NUM_RATE_POLICIES 5

typedef struct RATE_POLICY {
    A_RATEMASK rateMask;
    A_UINT8 shRetries;
    A_UINT8 lgRetries;
    A_UINT8 rc_mask_idx; /* Bitmap of RC MASKs that are valid for this rateMask */
    A_UINT8 pad;
} RATE_POLICY;

typedef struct tx_vdev_params_t {
    A_UINT8                     ic_subopmode;
    void                        *bss; /* wlan_bss_t */
    A_UINT8                     ic_opmode;
    WLAN_PHY_MODE               ic_curmode;
} TX_VDEV_PARAMS;

typedef struct RATE_CONTEXT {
    const WHAL_RATE_TABLE   *sc_currates;
    A_RATEMASK              sc_fixRateSet[NUM_VALID_RC_MASK] ; // rate mask identifying rates restricted by host
    A_RATEMASK              wmiFixRateSetMask[NUM_VALID_RC_MASK]; // rate mask from wmi (host)
    A_RATEMASK              sc_frameRateMask; // rate mask identifying rates restricted by host for sc_frameType
    A_UINT32                sc_phyMask; // used to limit rate selection by phy/modulation type.
    A_RATEMASK              txSelectRate[MODE_MAX];
    A_RATEMASK              sgiMask[2]; /* for HT and VHT rate masks */
    A_RATEMASK              userSgiMask; /* host request for SGI mask */
    A_RATEMASK              mcs_mask_4_nss[2*NUM_SPATIAL_STREAM]; /*rate table sequence per sim*/
    void                    *dev_context;
    RATE_POLICY             policies[NUM_RATE_POLICIES];
    A_INT16                 sc_userRix;
    A_INT16                 sc_fixedrix;
    A_UINT8                 sgiPERThreshold; /* Do we ever want different for HT/VHT ? */
    A_UINT8                 curr_nss; /* Current num spatial streams used */
    A_UINT8                 sc_defmgtrix;
    A_UINT8                 sc_protrix;
    A_UINT8                 sc_frameTriesMgmt;
    A_UINT8                 sc_frameTriesCtl;
    A_UINT8                 sc_frameTriesData[WMM_NUM_AC];
    A_UINT8                 sc_frameTriesEnable;
    A_UINT8                 tx_rix;
    A_UINT8                 sc_ifmgmtfixed;
    A_UINT8                 sc_bEnableFrameMask;
    A_UINT8                 sc_bFramematch;
    A_UINT8                 sc_frameType;
    //A_BOOL                  sc_bPspoll;
    A_UINT8                 ht_rate_ampdu_th1; /* Lowest MCS rate that support 4msec rule for 8frame aggr.*/
    A_UINT8                 ht_rate_ampdu_th2; /* Next MCS rate that support 4msec rule for 16frame aggr.*/
    A_UINT8                 ht_only_th;        /* Rates below this need to follow the HT only rates rule.*/
#define INVALID_SGI_MASK            (A_RATEMASK_FULL)
#define DEFAULT_SGI_PER_THRESHOLD   (10) /* PER lower than this will permit SGI */
    A_UINT8                 apRateSet;
    A_UINT8                 tx_sgi;

    void                    *pExt;
    TX_VDEV_PARAMS          vdev_params;
} RATE_CONTEXT;


#endif /* _RATECTRL_H_ */
