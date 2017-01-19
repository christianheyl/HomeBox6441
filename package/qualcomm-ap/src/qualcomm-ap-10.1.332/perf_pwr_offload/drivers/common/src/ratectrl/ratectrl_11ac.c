/*
 * Copyright (c) 2011-2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_TARGET_C$
 */

//-
// Copyright tc) 2004 Video54 Technologies, Inc.
// Copyright (c) 2004 Atheros Communications Inc.
// All rights reserved.
// $ATH_LICENSE_TARGET_C$
//
// $Id: //depot/sw/qca_main/components/wlan/qca-wifi-fw/1.1/drivers/common/src/ratectrl/ratectrl_11ac.c#11 $
//

#if defined(ATH_TARGET)
#include <osapi.h>
#include <athdefs.h>
#include <wmi.h>
#include <wmi_api.h>
#include <wmi_svc_api.h>
#include <wlan_common.h>
#include <wlan_bss.h>
#include <wlan_dev.h>
#include <wmi_api.h>
#include "whal_api.h"
#include "if_athvar.h"
#include "wlan_buf.h"
#include "wlan_peer.h"
#include "meta.h"
#include "umac_api.h"
#include "ieee80211_proto.h"
#include "wlan_framegen.h"
#include <wal.h>
#include <ar_wal.h>  /* TODO remove this */

#else

#include "ieee80211.h"
#include "ol_ratetable.h"
#include "ol_txrx_types.h"
#include "ol_txrx_internal.h" //for debug prints. TODO: remove

#endif


#include "wlan_defs.h"
#include "pool_mgr_api.h"

#include "ratectrl_11ac.h"
#include "ratectrl_11ac_api.h"
#include "ratectrl_11ac_internal.h"
#include "ratectrl_11ac_common.h"

#define RC_WAR_INCORRECT_PEER_PHYMODE 1

A_UINT32 g_trace_probe=0; //todo: hbrc

/* Default rate table defining candidate MCS set to be used */

/* 2x2 11n rate table does not include MCS 5, 6, 7, 8, 9, 10 */ 
#define DEFAULT_HT_RATE_TABLE_2SS  ((A_RATEMASK)0xf81f)
/* 3x3 11n rate table does not include MCS 4, 5, 6, 7, 8, 9, 14, 15,
   16, 17, 18, 19 
*/ 
#define DEFAULT_HT_RATE_TABLE_3SS  ((A_RATEMASK)0xf03c0f)

/* 2x2 11ac rate table does not include MCS 1x1:5, 6, 7, 8, 9
       2x2: 0, 1, 2 
*/ 
#define DEFAULT_VHT_RATE_TABLE_2SS ((A_RATEMASK)0xfe01f)

/* 3x3 11ac rate table does not include MCSs 1x1: 3,4,5,6,7,8,9
       2x2: 0, 1, 5, 6, 7,8,9
       3x3: 0, 1, 2, 3  
*/ 
#define DEFAULT_VHT_RATE_TABLE_3SS ((A_RATEMASK)0x3f007007)

#define PERE_HW_WAR_NO_MLME_SUPPORT 0

typedef enum {
    RC_BW_20 = 0,
    RC_BW_40 = 1,
    RC_BW_80 = 2
} RC_BW_TYPE_e;


typedef A_UINT16 RC_PHYMASK;

#define RC_PHYMASK_CCK (1<<WHAL_MOD_IEEE80211_T_CCK)
#define RC_PHYMASK_OFDM (1<<WHAL_MOD_IEEE80211_T_OFDM)
#define RC_PHYMASK_HT20 (1<<WHAL_MOD_IEEE80211_T_HT_20)
#define RC_PHYMASK_HT40 (1<<WHAL_MOD_IEEE80211_T_HT_40)

#define RC_PHYMASK_VHT20 (1<<WHAL_MOD_IEEE80211_T_VHT_20)
#define RC_PHYMASK_VHT40 (1<<WHAL_MOD_IEEE80211_T_VHT_40)
#define RC_PHYMASK_VHT80 (1<<WHAL_MOD_IEEE80211_T_VHT_80)
#define RC_PHYMASK_VHT160 (1<<WHAL_MOD_IEEE80211_T_VHT_160)

#define RC_PHYMASK_VHT (RC_PHYMASK_VHT20 | RC_PHYMASK_VHT40 | \
        RC_PHYMASK_VHT80 | RC_PHYMASK_VHT160)

#define RC_PHYMASK_ALL (RC_PHYMASK_CCK | RC_PHYMASK_OFDM | RC_PHYMASK_HT20 |\
    RC_PHYMASK_HT40 | RC_PHYMASK_VHT)


/* default rate table for different nss configurations */

const A_RATEMASK default_ht_rt_masks[] = {
      0xff ,                    /* 1x1 11n MCS 0..7 */
      DEFAULT_HT_RATE_TABLE_2SS,/* 2x2 11n  */
      DEFAULT_HT_RATE_TABLE_3SS /* 3x3 11n */
};

const A_RATEMASK default_vht_rt_masks[] = {
      0x3ff ,                    /* 1x1 11ac MCS 0..9 */
      DEFAULT_VHT_RATE_TABLE_2SS,/* 2x2 11ac  */
      DEFAULT_VHT_RATE_TABLE_3SS /* 3x3 11ac */
};


/* defaultTxSelectRate is a set of mode indexed rate masks that
 * are used to fine tune the tx rate selection.  For example
 * when HT rates are enabled for a mode this mask array will
 * prevent use of the OFDM rates as they are not as
 * efficient as the equivalent HT rates. The host can
 * override the default settings by using WMI_SET_TX_SELECT_RATES */
/* The following defines rate drop sequence for all the peers.
   Based on simulation results certain MCSs are excluded from
   the candidate MCSs to be used by rate control
*/
A_RATEMASK defaultTxSelectRate[] =
{
    /*[MODE_11A]        */ (A_RATEMASK)0xff0, // ofdm only
    /*[MODE_11G]        */ (A_RATEMASK)0xfff, // ofdm + cck
    /*[MODE_11B]        */ (A_RATEMASK)0xf, // cck only
    /*[MODE_11GONLY]    */ (A_RATEMASK)0xff0, // ofdm only
    /*[MODE_11NA_HT20]  */ A_RATEMASK_HT_20 & DEFAULT_HT_RATE_TABLE_3SS, // 11n
    /*[MODE_11NG_HT20]  */ A_RATEMASK_HT_20 & DEFAULT_HT_RATE_TABLE_3SS, // 11n
    /*[MODE_11NA_HT40]  */ A_RATEMASK_HT_40 & DEFAULT_HT_RATE_TABLE_3SS, // 11n
    /*[MODE_11NG_HT40]  */ A_RATEMASK_HT_40 & DEFAULT_HT_RATE_TABLE_3SS, // 11n
    /*[MODE_11AC_VHT20] */ A_RATEMASK_VHT_20 & DEFAULT_VHT_RATE_TABLE_3SS, //11AC
    /*[MODE_11AC_VHT40] */ A_RATEMASK_VHT_40 & DEFAULT_VHT_RATE_TABLE_3SS,
    /*[MODE_11AC_VHT80] */ A_RATEMASK_VHT_80 & DEFAULT_VHT_RATE_TABLE_3SS,
};

/* Certain devices do not like Cyclic Delay Diversity */
static A_UINT32 g_rc_no_cdd = 0;
#define RATECTRL_DO_NOT_USE_CDD() (g_rc_no_cdd)

/* 1x1 devices do not like Cyclic Delay Diversity for 64-QAM MCSs */
static A_UINT32 g_rc_1x1_mcs_no_cdd = 0x5;
#define RATECTRL_NO_CDD_1x1_STA(vdev) (wal_rc_is_dyntxchain(vdev))
#define RATECTRL_CDD_NOT_ALLOWED_FOR_MCS(m) ((m) >= g_rc_1x1_mcs_no_cdd)
#define RATECTRL_PEER_1x1(an) ((an)->peer_params.valid_tx_chainmask == 0x1)

/* g_rc_bw_probe_shift_factor controls how often should we probe for
   higher bw, when dynamic bw results too much of per on higher bw
   as of now it uses rate specific 50ms << g_rc_bw_probe_shift_factor
   ms of time before it gives  a next try
*/
static A_UINT32 g_rc_bw_probe_shift_factor =1;
/*
 * Get number of supported spatial stream based on chainmask
*/
static inline A_UINT8 rc_get_nss_from_chainmask(A_UINT8 chain_mask)
{
    A_UINT8 nss = 0;

    switch (chain_mask) {

       case 7:
       {
          nss = 3;
          break;
       }
       case 3:
       case 5:
       case 6:
       {
          nss = 2;
          break;
       }
       case 1:
       case 2:
       case 4:
       {
           nss = 1;
           break;
       }
        default:
            A_ASSERT(0);
            break;
    }

    return nss;
}

static inline A_UINT8 rc_get_rate_bw_idx(A_UINT32 pos)
{
    static const A_UINT8 _rate_bw_idx[WHAL_RC_MASK_MAX_IDX + 1] =
        {   /*[WHAL_RC_MASK_IDX_NON_HT]  */ 0,
            /*[WHAL_RC_MASK_IDX_HT_20]   */ 0,
            /*[WHAL_RC_MASK_IDX_HT_40]   */ 1,
            /*[WHAL_RC_MASK_IDX_VHT_20]  */ 0,
            /*[WHAL_RC_MASK_IDX_VHT_40]  */ 1,
            /*[WHAL_RC_MASK_IDX_VHT_80]  */ 2,
        };

    return _rate_bw_idx[pos];

}

#ifdef RC_WAR_INCORRECT_PEER_PHYMODE
/*
 * The idea of having separate phy_mode and ni_flag bw indication was
 * to maintain max capability receied in Assoc Req and rather dynamic
 * VHT Operating Mode field changing ni_flags bw dynamically to avoid
 * overhead of doing rcSibUpdate again.
 * However, in past, umac and firmware went out of sync while handling
 * VHT Operating Mode element and for the sake of quick fix this 
 * workaround was proposed, which:
 *     - overwrites phy_mode to allow run-time changes into ni_flags bw
         by using WMI_PEER_CHWIDTH.
 */
static void rc_war_correct_phymode(struct rate_node *pSib)
{
     if (pSib->peer_params.ni_flags & RC_PEER_VHT) {
         pSib->peer_params.phymode = MODE_11AC_VHT80;

#if 0
        if (pSib->peer_params.ni_flags & RC_PEER_80MHZ)
            pSib->peer_params.phymode = MODE_11AC_VHT80;
        else if (pSib->peer_params.ni_flags & RC_PEER_40MHZ)
            pSib->peer_params.phymode = MODE_11AC_VHT40;
        else
            pSib->peer_params.phymode = MODE_11AC_VHT20;
#endif
    }

}
#endif

A_UINT32 defaultSpatialMuxMCSMask[] =
{
    /*[MODE_STBC]    */ 0x0000f07f, // Enable MCS 0-6, 12-15 when STBC/TXBF is supported.
    /*[MODE_NO_STBC] */ 0x0000f81f, // Enable MCS 0-4, 11-15 when STBC/TXBF is not supported.
};

/* ieee80211_basic_rateset a set of basic rates for each mode. these
 * values represent the dot11 protocol representation for each rate
 * (eg 2 = 1mbps).  */
static struct ieee80211_rateset ieee80211_basic_rateset[] = {
        { 3, { 12, 24, 48 } },      /* MODE_11A */
        { 4, { 2, 4, 11, 22 } },    /* MODE_11G (mixed b/g) */
        { 4, { 2, 4, 11, 22 } },    /* MODE_11B */
        { 3, { 12, 24, 48 } },      /* MODE_11GONLY */
        { 3, { 12, 24, 48 } },      /* MODE_11NA_HT20 */
        { 4, { 2, 4, 11, 22 } },    /* MODE_11NG_HT20 (mixed b/g) */
        { 3, { 12, 24, 48 } },      /* MODE_11NA_HT40 */
        { 4, { 2, 4, 11, 22 } },    /* MODE_11NG_HT40 (mixed b/g) */
        { 3, { 12, 24, 48 } },      /* MODE_11AC_VHT20 */
        { 3, { 12, 24, 48 } },      /* MODE_11AC_VHT40 */
        { 3, { 12, 24, 48 } },      /* MODE_11AC_VHT80 */
};

/* FIXME */
struct ieee80211_rateset *pBasicRateSet = ieee80211_basic_rateset;

#ifdef ENABLE_RSSI_BASED_RATECTRL
static A_UINT32 ratectrl_mode = 0;
/* Based on passed mode determine if RSSI should be taken into account as well.
    REVISIT
 */
#define not_per_only()    ((ratectrl_mode) != 0)
#endif /* ENABLE_RSSI_BASED_RATECTRL */

/*
 * INTERNAL FUNCTIONS START
 */

/*
    Returns 0: if peer STA is legacy, 1 if peer STA is HT and not VHT, 2 if peer STA is VHT
*/
A_UINT32 _RATE_GetPeerPhy(struct rate_node *an)
{
    /* Get rate control mask that is used to derive best rate based on whether
            peer STA is legacy (0), HT (1) or VHT (3) capable.
     */
    if (IS_CONN_ACTIVE((an->peer))) {
        if (an->peer_params.ni_flags & RC_PEER_VHT)
            return 2;
        if (an->peer_params.ni_flags & RC_PEER_HT)
            return 1;
    }

    return 0;
}

A_UINT32 _RATE_GetTxChainMask(
        struct rate_node *an,
        const WHAL_RATE_TABLE *pRateTable,
        A_UINT8 rix)
{
    A_UINT32 tx_chain_mask;

    if (IS_CONN_ACTIVE((an->peer))) {
        if (RATECTRL_DO_NOT_USE_CDD()) {
            tx_chain_mask = an->peer_params.valid_tx_chainmask;
        }
        else {
#if ATH_TARGET
            wlan_vdev_t *dev =
                    (wlan_vdev_t *)(((wlan_peer_t *)(an->peer))->pDev);
            A_UINT8 rc = RT_GET_HW_RATECODE(pRateTable,  rix);
            A_UINT8 mcs = RT_GET_DOT11RATE(pRateTable,  rix);

            tx_chain_mask =
                wal_rc_get_vdev_tx_chainmask_4_rc(DEV_GET_WAL_VDEV(dev), rc);

            /* Check if CDD disabled for selected rates if so modify tx chain
               mask accordingly */
            if (RATECTRL_PEER_1x1(an) && 
                RATECTRL_NO_CDD_1x1_STA(DEV_GET_WAL_VDEV(dev)) && 
                RT_IS_HT_INDEX(rix) &&
                RATECTRL_CDD_NOT_ALLOWED_FOR_MCS(mcs)) {
                /* Get chaninmask where only lowest chain is enabled */
                tx_chain_mask = tx_chain_mask & ~(tx_chain_mask - 1);
            }
#else
            tx_chain_mask = an->peer_params.valid_tx_chainmask;
#endif
        }
    }
    else {
        /* Why are we here? */
        A_ASSERT(0);
    }

    A_ASSERT(tx_chain_mask && (tx_chain_mask < (1 << MAX_SPATIAL_STREAM)));

    return tx_chain_mask;
}

/*
 * Returns HW rate flags when rate supports STBC, LDPC
*/
A_UINT32 _RATE_GetRCFlags(struct rate_node *an,
        RATE_CONTEXT * g_pRATE,
        const WHAL_RATE_TABLE *pRateTable,
        A_UINT8 rix)
{
    A_UINT32 flags = 0;

    if (RT_IS_HT_INDEX(rix) && IS_CONN_ACTIVE((an->peer))) {

        if(an->peer_params.ni_flags & RC_PEER_VHT) { /* VHT mode */
            if (RT_IS_STBC(pRateTable, rix) &&
                (an->peer_params.vht_caps & IEEE80211_VHTCAP_RX_STBC)) {
                flags |= WHAL_RC_FLAG_STBC;
            }

            if (RT_IS_LDPC(pRateTable, rix) &&
                (an->peer_params.vht_caps & IEEE80211_VHTCAP_RX_LDPC)) {
                flags |= WHAL_RC_FLAG_LDPC;
            }
        } else { /* HT mode */
            if (RT_IS_STBC(pRateTable, rix) &&
                (an->peer_params.ht_caps & IEEE80211_HTCAP_C_RXSTBC)) {
                flags |= WHAL_RC_FLAG_STBC;
            }

            if (RT_IS_LDPC(pRateTable, rix) &&
                (an->peer_params.ht_caps & IEEE80211_HTCAP_C_ADVCODING)) {
                flags |= WHAL_RC_FLAG_LDPC;
            }
        }
    }
    
    return flags;
}

A_UINT32 _RATE_GetSGIFlag(struct rate_node *an,
        RATE_CONTEXT * g_pRATE,
        const WHAL_RATE_TABLE *pRateTable,
        A_UINT8 rix)
{
    if (an && an->txRateCtrl->htSgiValid) {
        A_RATEMASK sgi_mask;
        A_UINT32 m_idx, bit_pos;

        RT_GET_RC_MASK_4_RIX((void *)pRateTable, rix, &m_idx,&bit_pos);

        if (m_idx && RT_IS_VHT_INDEX(rix)) { /* VHT */
            sgi_mask = g_pRATE->sgiMask[1];

            if (RATEMASK_IS_VALID(sgi_mask, bit_pos) &&
                (an->txRateCtrl->state[rix].per <=
                g_pRATE->sgiPERThreshold)) {
                if ((m_idx == WHAL_RC_MASK_IDX_VHT_80 && 
                    (an->txRateCtrl->htSgiValid & TX_RATE_SGI_80)) ||
                    (m_idx == WHAL_RC_MASK_IDX_VHT_40 && 
                    (an->txRateCtrl->htSgiValid & TX_RATE_SGI_40)) ||
                    (m_idx == WHAL_RC_MASK_IDX_VHT_20 && 
                    (an->txRateCtrl->htSgiValid & TX_RATE_SGI_20))) {

                    return WHAL_RC_FLAG_SGI;                    
                }
            }
            
        }
        else if (m_idx) { /* HT case */
            sgi_mask = g_pRATE->sgiMask[0];
            if (RATEMASK_IS_VALID(sgi_mask, bit_pos) && 
                (an->txRateCtrl->state[rix].per <=
                g_pRATE->sgiPERThreshold)) {
                if ((m_idx == WHAL_RC_MASK_IDX_HT_40 && 
                    (an->txRateCtrl->htSgiValid & TX_RATE_SGI_40)) ||
                    (m_idx == WHAL_RC_MASK_IDX_HT_20 && 
                    (an->txRateCtrl->htSgiValid & TX_RATE_SGI_20))) {

                    return WHAL_RC_FLAG_SGI;                    
                }
            }
            
        }

    }
    return 0;
}

/* Return value
    0: NO RTS/CTS
    1: RTS-CTS
    2: CTS2Self
    3: RTS-CTS with signalling TA
*/
//A_UINT32 g_rc_rts = 0;

A_UINT32 _RATE_IsRTS_CTS_Enabled(struct rate_node *an)
{
#if ATH_TARGET
    wlan_peer_t *peer = (wlan_peer_t *)(an->peer);
    wlan_vdev_t *dev  = (wlan_vdev_t *)(peer->pDev);
    wal_vdev_t *wal_vdev = DEV_GET_WAL_VDEV(dev);
    return wal_rc_get_vdev_rts_cts(wal_vdev);
#else
    /* WAR for host-based. Needs to be modified. */
    return 0;
#endif
}

/* returns index of least sig bit */
A_UINT8 _GetLSB(A_RATEMASK mask)
{
    A_UINT8 res = 0/*?*/;
    
    if(!mask) {
        return 0xff;
    }
    
    do {
        if(mask & 0x000000ff) {
            res = 0;
        } else if(mask & 0x0000ff00) {
            res = 8;
        } else if(mask & 0x00ff0000) {
            res = 16;
        } else if(mask & 0xff000000) {
            res = 24;
        } else if(mask & 0x000000ff00000000ULL) {
            res = 32;
        } else if(mask & 0x00000f0000000000ULL) {
            res = 40;
        }
        
        
        mask >>= res;
        
        if((mask & 0x0f) == 0) {
            mask >>= 4;
            res += 4;
        }
        
        if(mask & 0x01) {
            break;
        }
        
        switch (mask & 0x0f) {
        case 2:
        case 6:
        case 10:
        case 14:
            res += 1;
            break;
        case 4:
        case 12:
            res += 2;
            break;
        case 8:
            res += 3;
            break;        
        }
    }while(0);
    
    return res;
}

/* returns index of most sig bit */
A_UINT8 _GetMSB(A_RATEMASK mask)
{
    A_UINT8 res;
    
    if(!mask) {
        return 0xff;
    }
    
    do {
        if(mask & 0x00000f0000000000ULL) {
            res = 40;
        } else if(mask & 0x000000ff00000000ULL) {
            res = 32;
        }
        if (0) {
        }
        else if(mask & 0xff000000) {
            res = 24;
        } else if(mask & 0x00ff0000) {
            res = 16;
        } else if(mask & 0x0000ff00) {
            res = 8;
        } else {
            res = 0;
        }
        
        mask >>= res;
    
        if(mask & 0xf0) {
            mask >>= 4;
            res += 4;
        }                
        
        switch (mask & 0x0f) {
        case 1:
            break;
        case 2:
        case 3:
            res += 1;
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            res += 2;
            break;
        default:
            res += 3;
            break;            
        }
    }while(0);
    
    return res;
}
 
void _rate_setup(RATE_CONTEXT *g_pRATE, WLAN_PHY_MODE mode)
{
#if 0
    const WHAL_RATE_TABLE *rt;
    struct ieee80211_rateset *rs;
    A_UINT32 i;
    A_UINT8 count;
    
    rt = whalGetRateTable(mode);

#ifdef WLAN_DEBUG
    if (rt == NULL) {
        A_PRINTF("%s:No Rate Table Setup for mode %d\n", __func__, mode);
        return ;
    }
#endif    
    
//    rs = &dev->ic_sup_rates[mode];
//    rs = &g_pRATE->vdev_params.ic_sup_rates[mode];

    count = 0;

/*
     REVISIT
     This function can be simplified a lot based on mandatory rates
     also RT_GET_RATE_COUNT should be removed with only OFDM/CCK? but there 
     a break below.
*/
    for (i = 0; i < RT_GET_RATE_COUNT(rt) ; i++)
    {
        if( RT_IS_VALID_RATE(rt, i, mode) && 
            !RT_IS_HT(rt,i) && !RT_IS_VHT(rt,i))
        {
#ifdef P2P
            /*Disable CCK rates for P2P device rate set*/
            if((g_pRATE->vdev_params.ic_subopmode !=SUB_OPMODE_NONE) &&(mode == MODE_11G ||
                mode == MODE_11NG_HT20 || mode == MODE_11NG_HT40)) {
                if(RT_GET_PHY(rt, i) == WHAL_MOD_IEEE80211_T_CCK)
                  continue;
            }
#endif
            rs->rs_rates[count++] = RT_GET_DOT11RATE(rt, i);

            if(count >= IEEE80211_RATE_MAXSIZE)
                break;
        }
    }
    rs->rs_nrates = count;
#endif
}

/* Iterators for validTxRateMask */
/* rcGetNextValidTxRate - used when probing for next higher rate */
/* Iterators for validTxRateMask */
A_BOOL _rcGetNextValidTxRate(const WHAL_RATE_TABLE *pRateTable,
        struct TxRateCtrl_s *pRc, RC_MASK_INFO_t *rc_mask_info, 
        A_UINT8 curValidTxRate, A_UINT8 *pNextIndex)
{
    A_UINT8     i,j;
    A_BOOL      found = FALSE; 
    A_UINT8 num_rix = pRc->maxValidRate[rc_mask_info->idx];

    /* Don't do anything if already at highest rate */
    if (pRc->validRateIndex[rc_mask_info->idx][num_rix -1] 
        == curValidTxRate) {
        goto done;
    }

    for (i = 0; i < num_rix; i++) {
        if (found == TRUE) {
            A_UINT8 upper, lower;

            RT_GET_RIX_LIMITS_4_MASK(&upper, &lower, rc_mask_info->idx);

            j = pRc->validRateIndex[rc_mask_info->idx][i];

            if (RATEMASK_IS_VALID(rc_mask_info->rc_mask, (j - lower))){
                *pNextIndex = pRc->validRateIndex[rc_mask_info->idx][i];
                return TRUE;
            }
        }
        else {
            if (pRc->validRateIndex[rc_mask_info->idx][i] == curValidTxRate) {
                found = TRUE;
            }
        }
    }

done:
    /* No more valid rates */
    *pNextIndex = 0;
    return FALSE;
}

A_BOOL _rcGetLowerValid(const WHAL_RATE_TABLE *pRateTable, A_UINT8 index,
    struct TxRateCtrl_s  *pRc, RC_MASK_INFO_t *rc_mask_info,
    A_UINT8 *pNextIndex)
{
    A_UINT8 j;
    A_RATEMASK currMask = rc_mask_info->rc_mask;
    A_UINT8 upper, lower;

    RT_GET_RIX_LIMITS_4_MASK(&upper, &lower, rc_mask_info->idx);
    
    while(index) {
        /* Move to the next valid rate   */
        index--;
        j = pRc->validRateIndex[rc_mask_info->idx][index];
        
        if (RATEMASK_IS_VALID(currMask, (j - lower))) {
            *pNextIndex = index;
            return TRUE;
        }
    }

    *pNextIndex = index;
    return FALSE;
}

A_UINT8 _rcGetNextLowerValidTxRate(const WHAL_RATE_TABLE *pRateTable,
        RC_MASK_INFO_t *rc_mask_info, struct TxRateCtrl_s  *pRc,
        A_UINT8 curValidTxRate, A_UINT8 *pNextRateIndex)
{
    A_UINT8 i;

    for (i = 0; i < pRc->maxValidRate[rc_mask_info->idx] ; i++) {
        if (pRc->validRateIndex[rc_mask_info->idx][i] == curValidTxRate) {
            break;
        }
    }
    if (i == pRc->maxValidRate[rc_mask_info->idx]) {
        *pNextRateIndex = curValidTxRate;
        return RT_INVAL_IDX;
    }
    /* Ratemask will have the latest valid rates so double check with the rate mask.
     * to see if the next lower index is indeed valid.
     */
    if (rcGetLowerValid(pRateTable, i, pRc, rc_mask_info, &i)) {
        *pNextRateIndex = pRc->validRateIndex[rc_mask_info->idx][i];
    } else {
        *pNextRateIndex = curValidTxRate;
        return RT_INVAL_IDX;
    }

    return i;
}


/*
 * This API returns rate tbl, txrate and rate Index based on
 * pkt type(MGMT, CTRL, DATA or NULL DATA) for non-VHT mode
 * for VHT it has different meaning and is only used for data frames
 
 */
void
_GetRateTblAndIndex(RATE_CONTEXT *g_pRATE, A_UINT8 *probe_rix,
                        A_BOOL shortPreamble, struct rate_node *an,
                        A_UINT8 * txrate, const WHAL_RATE_TABLE ** rt,
                        A_UINT8 * rix, A_BOOL isLowest, 
                        RC_MASK_INFO_t *rc_mask_info, A_UINT32 nowMsec)
{
    const WHAL_RATE_TABLE * pRateTable = NULL;

    pRateTable = g_pRATE->sc_currates;
    
    /* a non-zero ratemask passed in trumps all other rules. It
     * implies that the host has specified through a rate policy
     * what rates should be considered for this frame. */
    if(rc_mask_info->rc_mask) {    
    
        *rix = (A_UINT8)rcRateFind(g_pRATE, an, 1500, FALSE, rc_mask_info,
                                   probe_rix, nowMsec);
        *txrate = RT_GET_HW_RATECODE(pRateTable, (*rix)) |
                 (shortPreamble ? RT_GET_SHORTPREAM_CODE(pRateTable, (*rix)) :0);
    
    } else {
        /* Execution path if a rateMask has not been specified by the caller */

        g_pRATE->sc_phyMask = RC_PHYMASK_ALL;

#ifdef rcclean
#ifndef SUPPORT_VHT
        if(g_pRATE->sc_bEnableFrameMask &&  
            (g_pRATE->sc_frameType == 
            (wh->i_fc[0] & (IEEE80211_FC0_TYPE_MASK | IEEE80211_FC0_SUBTYPE_MASK)))){

            g_pRATE->sc_bFramematch = 1;
            
        }


        if (bss && IS_PROT_EN(bss) && (g_pRATE->vdev_params.ic_curmode == MODE_11G) && \
                (cb->flags & (TXRX_PSPOLL_FRAME | TXRX_NULL_DATA_FRAME))) {
            g_pRATE->sc_phyMask = RC_PHYMASK_CCK;
        }

        if ((cb->flags & TXRX_DATA_FRAME) ||
            (cb->flags & TXRX_SEND_AT_DATA_RATE) ||
            (g_pRATE->sc_bFramematch && an != NULL))
        {
            A_ASSERT(an != NULL);

#endif
#endif //rcclean
            // If a valid fixed rate is specified, use it
            if (g_pRATE->sc_fixedrix != -1){
                A_UINT32 m_idx, b_idx;
                
                RT_GET_RC_MASK_4_RIX((void *)pRateTable,g_pRATE->sc_fixedrix, 
                    &m_idx,&b_idx);

                if (RATEMASK_IS_VALID(an->txRateCtrl->validTxRateMask[m_idx],
                        b_idx)) {
                    *rix = (A_UINT8)g_pRATE->sc_fixedrix;
                    *txrate = RT_GET_HW_RATECODE(pRateTable, (*rix));
                    RATEMASK_SET_IDX(rc_mask_info->rc_mask, b_idx);
                }
                else {
                    A_ASSERT(0);
                }
            } 
            else { /* Actual rate control find, no Fix rate */
                *rix = (A_UINT8)rcRateFind(g_pRATE, an, 1500, isLowest,
                    rc_mask_info, probe_rix, nowMsec);
                
                *txrate = RT_GET_HW_RATECODE(pRateTable, (*rix)) |
                           (shortPreamble ? RT_GET_SHORTPREAM_CODE(pRateTable, (*rix)) :0);
                #if 0
                rc_mask_info->rc_mask = 
                          g_pRATE->sc_fixRateSet[rc_mask_info->idx] & 
                          an->txRateCtrl.validTxRateMask[rc_mask_info->idx];

                if(g_pRATE->sc_bFramematch){
                    rc_mask_info->rc_mask &= 
                            g_pRATE->sc_frameRateMask[rc_mask_info->idx];
                }
                #endif
                
            }
#ifdef rcclean
#ifndef SUPPORT_VHT
        } 
        else { /* management frames */
            if (an && (!g_pRATE->sc_ifmgmtfixed)) {
                *rix = an->an_min_basic;
                *txrate = RT_GET_HW_RATECODE(pRateTable, *rix);

                if(shortPreamble)
                    *txrate |= RT_GET_SHORTPREAM_CODE(pRateTable, *rix);
                
                rc_mask_info->rc_mask =  an->txRateCtrl->validTxRateMask[0];
                //ensure that mgmt rate is included in returned ratemask
                RATEMASK_SET_IDX(rc_mask_info->rc_mask, *rix);
            } else {
                *rix = g_pRATE->sc_defmgtrix;
#ifdef P2P
              if((g_pRATE->vdev_params.ic_subopmode != SUB_OPMODE_NONE) &&
                      (*rix < RIX_11A_TO_RIX_11G_DELTA)) {
                 *rix = RIX_11A_TO_RIX_11G_DELTA;
              }
#endif
                *txrate = RT_GET_HW_RATECODE(pRateTable, *rix);

                if(shortPreamble)
                    *txrate |= RT_GET_SHORTPREAM_CODE(pRateTable, *rix);

                rc_mask_info->rc_mask = A_RATEMASK_OFDM_CCK;
            }
        }
#endif /* SUPPORT_VHT */
#endif //rcclean
    }

    *rt = pRateTable;
    g_pRATE->sc_bFramematch = 0;
    return ;
}

A_BOOL _RateCheckDefaultMCSMask(struct rate_node *rc_node, A_UINT8 mcs_idx)
{
    /* TODO 
            add VHT specific STBC/TxBF testing here
       */
            
    if (rc_node->peer_params.ni_flags & RC_PEER_VHT) {
        //need to fix this
        return TRUE;
    }


    /* N.B. STBC capabilities are derived from the ht_caps/vht_caps now. Not
     * updated here since this function is not being used anymore. */
    if (rc_node->peer_params.ni_flags & RC_PEER_SPATIAL_MUX) {
        if (rc_node->peer_params.ni_flags &
                (RC_PEER_STBC|RC_PEER_TXBF)) {
            if (defaultSpatialMuxMCSMask[MODE_STBC] & (1<<mcs_idx)) {
                return TRUE;
            }
        } else {
            if (defaultSpatialMuxMCSMask[MODE_NO_STBC] & (1<<mcs_idx)) {
                return TRUE;
            }
        }

        return FALSE;
   }

   return TRUE;
}
/* Check the rate index is valid for the current node flags.
 * This routine can be extended to support other features that needs to enable or disable
 * rates per node basis.
 *  - Check if HT40 is enabled in the ni_flags before enabling HT40 rates.
 *  - Check if spatial multiplexing can be enabled.
 */
A_BOOL _RateValidCheckNodeFlags(struct rate_node *rc_node,
        const WHAL_RATE_TABLE *pRateTable, A_UINT16 rix)
{
    if (rc_node->peer_params.ni_flags & RC_PEER_VHT) {
        
        if (RT_IS_VHT_80MHZ(pRateTable, rix) && 
            (rc_node->peer_params.ni_flags & RC_PEER_80MHZ))
            goto check_nss;

        else if (RT_IS_VHT_40MHZ(pRateTable, rix) && 
            (rc_node->peer_params.ni_flags & RC_PEER_40MHZ))
            goto check_nss;

        else if (RT_IS_VHT_20MHZ(pRateTable, rix))
            goto check_nss;
        
    }

    if (!RT_IS_40MHZ(pRateTable, rix) || 
        (rc_node->peer_params.ni_flags & RC_PEER_40MHZ)) {
        goto check_nss;
    }

check_nss:
    if (RT_IS_1SS(pRateTable, rix) || 
        (rc_node->peer_params.ni_flags & RC_PEER_SPATIAL_MUX))
        return TRUE;

    return FALSE;
}

A_BOOL _RateValidCheck(const WHAL_RATE_TABLE *pRateTable,
        A_UINT16 rix, struct rate_node *rc_node)
{
    WLAN_PHY_MODE mode = rc_node->peer_params.phymode;

    if(!RT_IS_VALID_RATE(pRateTable, rix, mode))
        return FALSE;

    return TRUE;
}

void _rcSortValidRates(const WHAL_RATE_TABLE *pRateTable, TX_RATE_CTRL *pRc,
        WHAL_RC_MASK_IDX mask_idx)
{
    A_UINT8 i,j;

    if (!pRc->maxValidRate[mask_idx]) {
        return;
    }

    for (i=pRc->maxValidRate[mask_idx]-1; i > 0; i--) {
        for (j=0; j <= i-1; j++) {
            if (RT_GET_RAW_KBPS(pRateTable, pRc->validRateIndex[mask_idx][j])>
                    RT_GET_RAW_KBPS(pRateTable, 
                        pRc->validRateIndex[mask_idx][j+1])) {
                A_UINT8 tmp=0;
                tmp = pRc->validRateIndex[mask_idx][j];
                pRc->validRateIndex[mask_idx][j] = 
                    pRc->validRateIndex[mask_idx][j+1];
                pRc->validRateIndex[mask_idx][j+1] = tmp;
            }
        }
    }

    return;
}

/*
 *  Check whether the MCS 8 and 9 are supported by the peer.
 *  This check is done based on the VHT Supported MCS Set
 *  information as per 11n and 11ac specs.
 */
#define VHT_MCS_SET_FOR_NSS(x, ss)   ( ((x) & (3 << ((ss)<<1))) >> ((ss)<<1) )
#define NUM_VHT_MCS_PER_NSS          10
#define NUM_HT_MCS_PER_NSS            8

void
_RateInMCSSetCheck(struct rate_node *pSib)
{
    struct TxRateCtrl_s *pRc = pSib->txRateCtrl;

    A_UINT8 nss_idx = rc_get_nss_from_chainmask(pSib->peer_params.valid_tx_chainmask);
    A_UINT8 i = 0, idx = 0;
    A_UINT8 mcs_set = 0;
    A_UINT32 mcsMask = (A_UINT32)0xffffffff, mcsSubMask = 0;

    if( pSib->peer_params.ni_flags & RC_PEER_HT ) {
        for( i=nss_idx; i>0; i-- ) {
            mcsMask = mcsMask << NUM_HT_MCS_PER_NSS;
            mcsMask = mcsMask | (pSib->peer_params.ni_ht_mcs_set[i-1]);
        }

        pRc->validTxRateMask[WHAL_RC_MASK_IDX_HT_20] &= mcsMask;
        pRc->validTxRateMask[WHAL_RC_MASK_IDX_HT_40] &= mcsMask;
    }

    mcsMask = (A_UINT32)0xffffffff;

    if( pSib->peer_params.ni_flags & RC_PEER_VHT ) {
        for( i=nss_idx; i > 0; i-- ) {
            mcs_set = VHT_MCS_SET_FOR_NSS(pSib->peer_params.ni_vht_mcs_set, (i-1));
            mcsMask = mcsMask << NUM_VHT_MCS_PER_NSS;

            switch(mcs_set) {
            case 0:
                mcsSubMask = 0xff; /* mask MCS 8 and 9 */
                break;
            case 1:
                mcsSubMask = 0x1ff; /* mask MCS 9 */
                break;
            case 2:
                mcsSubMask = 0x3ff; /* mask nothing */
                break;
            case 3:
                mcsSubMask = 0x00; /* mask all MCS */
                break;
            default:
                A_ASSERT(0);
                break;
            }

            mcsMask = mcsMask | mcsSubMask;
        }

        for( idx = WHAL_RC_MASK_IDX_VHT_20; idx <= WHAL_RC_MASK_IDX_VHT_80; idx++ ) {
            pRc->validTxRateMask[idx] &= mcsMask;
        }
    }

    return;
}

/*
 *  Update the SIB's rate control information
 *
 *  This should be called when the supported rates change
 *  (e.g. SME operation, wireless mode change)
 *
 *  It will determine which rates are valid for use.
 */

/* Used to change the PER-threshold at debug-time.
 * Will still need to re-assoc though, for this to take effect.
 */
A_UINT8 g_default_sgi_per_thresh = 10;

/*TODO: hbrc- change rcSibUpdate's signature to include only peer rate-ctxt*/

void _rcSibUpdate(RATE_CONTEXT * g_pRATE, struct rate_node *pSib, void *peer)
{
    const WHAL_RATE_TABLE    *pRateTable = g_pRATE->sc_currates;
    struct TxRateCtrl_s *pRc        = pSib->txRateCtrl;
    A_UINT8             i, j, hi = 0, count, nss_idx;
    int                 rateCount;
    A_RATEMASK mask;
    A_UINT32 mask_idx, rc_bit;
    A_UINT8 rc_mask_cnt[NUM_VALID_RC_MASK] = {0};
    /* Initial rate table size. Will change depending on the working rate set */
    pRc->rateTableSize = MAX_TX_RATE_TBL;
    rateCount = RT_GET_RATE_COUNT(pRateTable);
    /* Pointer to the peer structure */
    pSib->peer = peer;

    nss_idx = rc_get_nss_from_chainmask(pSib->peer_params.valid_tx_chainmask);

    /* Initialize thresholds according to the global rate table */
    for (i = 0 ; i < pRc->rateTableSize; i++) {
#ifdef ENABLE_RSSI_BASED_RATECTRL
        pRc->state[i].rssiThres = (i<rateCount)? RT_GET_RSSI_MIN(pRateTable, i) : 0;
#endif /* ENABLE_RSSI_BASED_RATECTRL */
        pRc->state[i].per       = 0;
    }
    
#ifdef ENABLE_RSSI_BASED_RATECTRL
    pRc->rssiDownTime = A_MILLISECONDS();
#endif /* ENABLE_RSSI_BASED_RATECTRL */

    if (g_pRATE->vdev_params.ic_opmode != RC_VDEV_M_AP) {
        A_MEMSET(&g_pRATE->sc_fixRateSet, 0, sizeof(g_pRATE->sc_fixRateSet));
    }

    /* Determine the valid rates */
    memset(&pRc->validTxRateMask, 0, sizeof(pRc->validTxRateMask));

    count = 0;

    if (1) {
        A_UINT32 ht_mcs_set = 0;
        ht_mcs_set = pSib->peer_params.ni_ht_mcs_set[0];
        if (MAX_SPATIAL_STREAM > 1) {
            ht_mcs_set |= (pSib->peer_params.ni_ht_mcs_set[1] << 8);
            if (MAX_SPATIAL_STREAM > 2) {
                ht_mcs_set |= (pSib->peer_params.ni_ht_mcs_set[2] << 16);
            }
        }

#if ATH_TARGET
        DBGLOG_RECORD_LOG(WLAN_MODULE_RATECTRL, -1,
            RATECTRL_DBGID_ASSOC, DBGLOG_ERR, 5,
            pSib->peer_params.valid_tx_chainmask, pSib->peer_params.phymode, pSib->peer_params.ni_flags,
            pSib->peer_params.ni_vht_mcs_set, ht_mcs_set);
#endif
    }
    A_ASSERT(pSib->peer_params.phymode <= MODE_11AC_VHT80);

#ifdef RC_WAR_INCORRECT_PEER_PHYMODE
   rc_war_correct_phymode(pSib);
#endif

    if (1) {
        /* No working rate, use valid rates */
        for (i = 0; i < rateCount; i++) {

            if (RateValidCheck(pRateTable, i, pSib))
            {
                RT_GET_RC_MASK_4_RIX((void *)pRateTable, i, &mask_idx,&rc_bit);
                pRc->validRateIndex[mask_idx][rc_mask_cnt[mask_idx]++] = i;
                count ++;
                RATEMASK_ADD_IDX(pRc->validTxRateMask[mask_idx], 
                    pRc->validTxRateMask[mask_idx], rc_bit);

                hi = A_MAX(hi, i);

            }  
        }
        A_MEMCPY(&pRc->maxValidRate,&rc_mask_cnt,sizeof (rc_mask_cnt));
        RateInMCSSetCheck(pSib);
    }

        /* Get actually valid rate index, previous we get it from rate table,
         * now get rate table which include all working rate, so we need make
         * sure our valid rate table align with working rate */

        for (j = WHAL_RC_MASK_IDX_NON_HT; j <= WHAL_RC_MASK_MAX_IDX; j++) {
            A_UINT8 lower_rix, upper_rix;
            count = 0;
            mask = pRc->validTxRateMask[j];
            RT_GET_RIX_LIMITS_4_MASK(&upper_rix, &lower_rix, j);
            for (i = lower_rix; i <= upper_rix; i++) {
                if (RATEMASK_IS_VALID(mask, (i-lower_rix))) {
                    pRc->validRateIndex[j][count++] = i;
                }
            }
            pRc->maxValidRate[j] = count;
            /* Sort the rate in the current mask */
            rcSortValidRates(pRateTable, pRc, j);
        }
    
    pRc->rateTableSize = hi + 1;

    switch (pSib->peer_params.phymode) {

        /* REVISIT 
           Make sure all the init max rates are valid given dynamic masks*/
        case MODE_11NA_HT20:
        case MODE_11NA_HT40:
            pRc->rateMax[0] = RT_GET_INIT_MAXRATE(pRateTable, MODE_11NA_HT20);
            pRc->rateMax[1] = RT_GET_INIT_MAXRATE(pRateTable, MODE_11NA_HT40);
            break;

        case MODE_11NG_HT20:
        case MODE_11NG_HT40:
            pRc->rateMax[0] = RT_GET_INIT_MAXRATE(pRateTable, MODE_11NG_HT20);
            pRc->rateMax[1] = RT_GET_INIT_MAXRATE(pRateTable, MODE_11NG_HT40);
            break;

        case MODE_11AC_VHT20:
        case MODE_11AC_VHT40:
        case MODE_11AC_VHT80:
            pRc->rateMax[0] = RT_GET_INIT_MAXRATE(pRateTable, MODE_11AC_VHT20);
            pRc->rateMax[1] = RT_GET_INIT_MAXRATE(pRateTable, MODE_11AC_VHT40);
            pRc->rateMax[2] = RT_GET_INIT_MAXRATE(pRateTable, MODE_11AC_VHT80);
            break;
        default:
            pRc->rateMax[0] =
                    RT_GET_INIT_MAXRATE(pRateTable, pSib->peer_params.phymode);
            break;
            
    }

    /* Set htSgiValid so that ratectrl can decide whether to use SGI */
    pRc->htSgiValid = 0;

    /* Update the sgiMask. For now, SGI is enabled only for MCS (8*n - 1) in HT mode and
     * MCS (8*n - 1), (9*n - 1) and (10*n - 1) in VHT mode, where n is the nss. */
    g_pRATE->sgiMask[0] = 0x0;
    g_pRATE->sgiMask[1] = 0x0;
    g_pRATE->sgiPERThreshold = g_default_sgi_per_thresh;
    g_pRATE->userSgiMask = INVALID_SGI_MASK;

    if( pSib->peer_params.ni_flags & RC_PEER_HT ) {
        RATE_SetHTConnCap( pSib, pSib->peer_params.ht_caps );
        g_pRATE->sgiMask[0] = (0x80) << ((nss_idx - 1)<<3);
    }

    if( pSib->peer_params.ni_flags & RC_PEER_VHT ) {
        RATE_SetVHTConnCap( pSib, pSib->peer_params.vht_caps );
        g_pRATE->sgiMask[1] = (0x380) << (((nss_idx - 1)<<3) + ((nss_idx - 1)<<1));
    }


    A_ASSERT(pRc->rateTableSize <= MAX_TX_RATE_TBL);

    A_MEMCPY(&g_pRATE->sc_fixRateSet, &g_pRATE->wmiFixRateSetMask,
        sizeof(g_pRATE->sc_fixRateSet));


    // If fixedRateMask and validTxRateMask for the connection doesnt have anything common
    // use all validTxRates 
    /* Index 0 is for OFDM/CCK non-HT rates */
    if ((g_pRATE->sc_fixRateSet[0] & pRc->validTxRateMask[0]) == 0) {
        g_pRATE->sc_fixRateSet[0] = A_RATEMASK_OFDM_CCK;
    }

    /* REVISIT
       TODO : mask out all the rates not desried to be used as candidate
       MCSs for rate control.
       Even better defaultTxRateMask should be modified before for this
       vdev if this is running anything other than 3x3.
    */

//A_PRINTF_ALWAYS("rmasks: v=%08x f=%08x\n", pRc->validTxRateMask, g_pRATE->sc_fixRateSet);
}

#ifdef ENABLE_RSSI_BASED_RATECTRL
/*
 * Return the median of three numbers
 */
static INLINE A_RSSI
median(A_RSSI a, A_RSSI b, A_RSSI c)
{
    if (a >= b) {
        if (b >= c) {
            return b;
        } else if (a > c) {
            return c;
        } else {
            return a;
        }
    } else {
        if (a >= c) {
            return a;
        } else if (b >= c) {
            return c;
        } else {
            return b;
        }
    }
}
#endif /* ENABLE_RSSI_BASED_RATECTRL */

#if 0
/* reads the ccount register (cpu ticks) and returns value */
static __inline__ int read_ccount()
{
    unsigned int ccount;
    __asm__ __volatile__ (
    "rsr %0, ccount"
    : "=a" (ccount) : : "memory"
    );
    return ccount;
}
#endif

/*
    _GetBestRate currently only works for passed vMask which would be either 
    for VHT or non-VHT (11n/erp ) rates
    make it more clean
    TODO

*/

/* finds the rate with the best thru-put given an allowed rate mask a rate table 
 * and a RateCtrl object for the receiver. */
//LOCAL
A_UINT8 _GetBestRate(RATE_CONTEXT *g_pRATE, RC_MASK_INFO_t *rc_mask_info,
            const WHAL_RATE_TABLE *pRateTable, 
            struct TxRateCtrl_s  *pRc, A_RSSI rssiLast,
            A_UINT8 forced_max_rix)
{
        A_INT16 index;
        A_UINT32 bestThruput,thisThruput;
        A_UINT8 bestRate, fixRateMin, minRate, maxRate;
        
        bestThruput = 0;
        bestRate    = 0;
        fixRateMin =  RT_INVALID_INDEX;   

        RT_GET_RIX_LIMITS_4_MASK(&maxRate,&minRate,rc_mask_info->idx);

        for (index = maxRate ; index >= minRate ; index--) {                
            
            /* Skip invalid rates */
            if (!RATEMASK_IS_VALID(rc_mask_info->rc_mask, (index - minRate))){
                continue;
            }                                        
    
            /* this check unfortunately is only needed for legacy
               wouldn't it be nice to move out of for (index...) loop
               REVISIT
            */
            if(g_pRATE->sc_phyMask != RC_PHYMASK_ALL) {
                if(((1<<(RT_GET_PHY(pRateTable, index))) & 
                    g_pRATE->sc_phyMask) == 0){
                    continue;
                }
            }
    
            if (forced_max_rix && 
                   RT_GET_RAW_KBPS(pRateTable, index) >
                   RT_GET_RAW_KBPS(pRateTable, forced_max_rix)) {
                continue;
            }
            if (fixRateMin > index) {
                fixRateMin = index;
            }
            
#ifdef ENABLE_RSSI_BASED_RATECTRL
            if (not_per_only() &&
                rssiLast < pRc->state[index].rssiThres) {
                continue;
            }
#endif /* ENABLE_RSSI_BASED_RATECTRL */
    
            thisThruput = (100-pRc->state[index].per) * 
                    RT_GET_USER_KBPS(pRateTable, index);                
            
            if (bestThruput <= thisThruput) {
                bestThruput = thisThruput;
                bestRate    = index;            
            }
            
        }

        A_ASSERT(fixRateMin != RT_INVALID_INDEX);
        
        /*   bestRate is initialized to 0 but 0 may not be valid
                     therefore set bestRate to fixRateMin 
               */
        if ((fixRateMin != RT_INVALID_INDEX) && 
            (fixRateMin > bestRate)) {
            bestRate = fixRateMin;
        }
    
        return bestRate;
        
}

/* Get Valid Rate Mask depending on vdev, peer, and hw rate table */

static void _rc_get_peer_valid_rc_mask(RATE_CONTEXT *g_pRATE,
     struct rate_node *pSib,
     RC_MASK_INFO_t *rc_mask_info)
{
   struct TxRateCtrl_s  *pRc;
   pRc = pSib->txRateCtrl;

   /* Update the rate mask when it's not specified */
   rc_mask_info->rc_mask = g_pRATE->sc_fixRateSet[rc_mask_info->idx] & 
                pRc->validTxRateMask[rc_mask_info->idx];
    
   if(g_pRATE->sc_bFramematch)
       rc_mask_info->rc_mask &= g_pRATE->sc_frameRateMask;                    
   if (rc_mask_info->idx) { /* HT/VHT rate apply MCS mask */
        /* The HW rate table MCS mask is based on minimum of
           wlan vdev current supported nss and peer's supported
           NSS.
        */
       A_UINT8 mcs_mask_idx;
       A_UINT8 nss_idx = 
           rc_get_nss_from_chainmask(pSib->peer_params.valid_tx_chainmask);
       if (nss_idx > g_pRATE->curr_nss)
           nss_idx = g_pRATE->curr_nss;
       nss_idx--; /* Actual NSS index starts from 0 */

       mcs_mask_idx = (rc_mask_info->idx >= WHAL_RC_MASK_IDX_VHT_20)?
                      (NUM_SPATIAL_STREAM + nss_idx) : nss_idx; 

       A_ASSERT(mcs_mask_idx < (2*NUM_SPATIAL_STREAM));

       rc_mask_info->rc_mask &= g_pRATE->mcs_mask_4_nss[mcs_mask_idx];
   }
}

/*
 * Determines and returns the new Tx rate index.
 */
A_UINT16
_rcRateFind(RATE_CONTEXT * g_pRATE, struct rate_node *pSib, A_UINT32 frameLen,
           A_BOOL isLowest, RC_MASK_INFO_t *rc_mask_info, A_UINT8 *probe_rix, A_UINT32 nowMsec)
{
    const WHAL_RATE_TABLE     *pRateTable  = g_pRATE->sc_currates;
    struct TxRateCtrl_s  *pRc;
#ifdef ENABLE_RSSI_BASED_RATECTRL
    A_UINT32             dt;
#endif /* ENABLE_RSSI_BASED_RATECTRL */
    A_UINT8              rate, nextRate;
    A_RSSI               rssiLast=0;
#ifdef ENABLE_RSSI_BASED_RATECTRL
    A_RSSI               rssiReduce;
#endif /* ENABLE_RSSI_BASED_RATECTRL */
    A_INT16              index;

    A_UINT32             minRateKbps = (A_UINT32)0xffffffff;
    A_RATEMASK           vMask;
    A_ASSERT(pSib);    

    /* have the real rate control logic kick in */
    pRc = pSib->txRateCtrl;
    
    if (isLowest) {
        /* Find the lowest rate */
        nextRate = 0;
        vMask = g_pRATE->sc_fixRateSet[WHAL_RC_MASK_IDX_NON_HT];
        vMask &= pRc->validTxRateMask[WHAL_RC_MASK_IDX_NON_HT];            

        /* There should be no case, ofcourse except some craziness, where
                   there is no OFDM/CCK rate enabled
               */
        A_ASSERT(vMask);
        
        for (index = 0; RT_IS_OFDM_CCK_INDEX(index); index++) {

            /* skip unsupported rates */            
            if (!RATEMASK_IS_VALID(vMask, index)) {
                continue;
            }

            rate = index;
            if (minRateKbps > RT_GET_RAW_KBPS(pRateTable, rate)) {
                nextRate = rate;
                minRateKbps = RT_GET_RAW_KBPS(pRateTable, rate);
            }       
        }
        return nextRate;
    }

#ifdef ENABLE_RSSI_BASED_RATECTRL
    if (pRc->rssiLastPrev || pRc->rssiLastPrev2) {
        /* Find the best rate */
        rssiLast   = median(pRc->rssiLast, pRc->rssiLastPrev, pRc->rssiLastPrev2);
    } else {
        rssiLast = pRc->rssiLast;
    }
    rssiReduce = 0;

    /*
     * Age (reduce) last ack rssi based on how old it is.
     * The bizarre numbers are so the delta is 160msec,
     * meaning we divide by 16.
     *   0msec   <= dt <= 25msec:   don't derate
     *   25msec  <= dt <= 185msec:  derate linearly from 0 to 10dB
     *   185msec <= dt:             derate by 10dB
     */

    /*
            These numbers and associated computations sound interesting.
            Do they need some tuning for 11ac?
            REVISIT
        */
//    nowMsec = A_MILLISECONDS();
    dt = nowMsec - pRc->rssiTime;

    if (dt >= 185) {
        rssiReduce = 10;
    } else if (dt >= 25) {
        rssiReduce = (A_UINT8)((dt - 25) >> 4);
    }
 
    /* Reduce rssi for long packets */
    if (frameLen > 800) {
        rssiReduce += 1;    /* need 1 more dB for packets > 800 bytes */
    }

    /* Now reduce rssiLast by rssiReduce */
    if (rssiLast < rssiReduce) {
        rssiLast = 0;
    } else {
        rssiLast -= rssiReduce;
    }
#endif /* ENABLE_RSSI_BASED_RATECTRL */

    /*
     * Now look up the rate in the rssi table and return it.
     * If no rates match then we return 0 (lowest rate)
     */

    if(rc_mask_info->rc_mask == (A_RATEMASK)0) {
        /* Update the rate mask when it's not specified */
        _rc_get_peer_valid_rc_mask(g_pRATE, pSib, rc_mask_info);
    }
  
    A_ASSERT(rc_mask_info->rc_mask);

    index = rc_get_rate_bw_idx(rc_mask_info->idx);

    rate = GetBestRate(g_pRATE, rc_mask_info, pRateTable, pRc, rssiLast,
              pRc->rateMax[index]);   
 
#ifdef ENABLE_RSSI_BASED_RATECTRL
    pRc->rssiLastLkup = rssiLast;
#endif /* ENABLE_RSSI_BASED_RATECTRL */

    /*
     * Must check the actual rate (rateKbps) to account for non-monoticity of
     * 11g's rate table
     */
    {
        /*
         * Always probe the next rate in the rate Table (ignoring monotonicity).
         * Reason:  If OFDM is broken, when rateMax = 5.5, it will probe
         *          11 Mbps first.
         */
        if (((pRc->hwMaxRetryPktCnt[index] >= 4) 
              || (rate == pRc->validRateIndex[rc_mask_info->idx][0])) &&
            (nowMsec - pRc->probeTime[index] >
                RT_GET_PROBE_PERIOD(pRateTable, rate)) &&
            rcGetNextValidTxRate(pRateTable, pRc, rc_mask_info, rate, &nextRate))
        {
//            rate                  = nextRate;
            pRc->probeRate[index] = (A_UINT8)nextRate;
            pRc->probeTime[index] = nowMsec;
            pRc->hwMaxRetryPktCnt[index] = 0;
        
//            *is_probe = TRUE;
            *probe_rix = nextRate;

#ifdef debugratectrl
            TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "probe: is probing\n");
#endif
        }
    }

#ifdef rcclean
#ifdef RATECTRL_DEBUG
    /*
     * Make sure rate is not higher than the allowed maximum.
     * We should also enforce the min, but I suspect the min is
     * normally 1 rather than 0 because of the rate 9 vs 6 issue
     * in the old code.
     */
    if (rate > (pSib->txRateCtrl->rateTableSize - 1)) {
        rate = pSib->txRateCtrl->rateTableSize - 1;
    }
#endif
#endif //rcclean

#ifdef debugratectrl
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "probe_ratefind: nowMsec = 0x%x %d\n",
            nowMsec, nowMsec);
#endif

    return rate;
}


A_UINT32 g_dbg_probe;
A_UINT32 g_dbg_probe_idx;
A_UINT32 g_dbg_tx_rate;
RC_TX_RATE_SCHEDULE g_dbg_rc_tx_rate_sched;
/*
 * _g_dbg_rc_weight is currently used to cap the maximum weight in case of
   excessive retries. The idea is to remove this after some fine tuning further
*/

A_UINT32 _g_dbg_rc_weight = 80;
RC_TX_DONE_PARAMS *_g_test_args;

//#define RATECONTROL_DYNAMIC_BW_DEBUG 1

#ifdef RATECONTROL_DYNAMIC_BW_DEBUG
static A_UINT32 g_dbg_rc_err_inject = 0;
static void rc_dbg_dyn_bw_err_inj(A_UINT32 bw, A_UINT8 *mpdu_failed,
     A_UINT8 *mpdu_tried, A_UINT32 *flags)
{
    A_UINT32 err = (g_dbg_rc_err_inject >> (bw << 3)) & 0xff;
    if (!err)
        return;

    if (err == 100 )
       *flags |= WHAL_RC_TXDONE_X_RETRIES;
    else 
       *mpdu_failed = (*mpdu_tried * err ) >> 7; /* /128) */
}
#endif

void _rcUpdate_HT (RATE_CONTEXT *g_pRATE, struct rate_node *pSib,
        RC_TX_RATE_SCHEDULE *rate_sched, RC_TX_DONE_PARAMS *args,
        A_UINT8 probe_aborted)
{
    static int __count;

    const WHAL_RATE_TABLE *pRateTable;
    struct TxRateCtrl_s *pRc;
    A_UINT32            nowMsec     = 0; //A_MILLISECONDS();
#ifdef ENABLE_RSSI_BASED_RATECTRL
    A_BOOL              stateChange = FALSE;
#endif /* ENABLE_RSSI_BASED_RATECTRL */
    A_UINT8             lastPer;
    int                 rate;
    A_UINT8             txRate;
    A_RSSI rssiAck;
    A_UINT8 nFrames, retries, nBad;
    A_RATEMASK rMask;
    A_UINT32 rMaskPos, rateBit;
    A_UINT8 idx;

   pRateTable = g_pRATE->sc_currates;
   pRc        = pSib->txRateCtrl;

   if(probe_aborted == 1) { /* args passed in would be null if this is true. */
       A_MEMSET(&pRc->probeRate, 0, sizeof (pRc->probeRate));
       return;
   }

    _g_test_args = args;

#ifdef debugratectrl
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "Enter %s | pSib: 0x%x | g_pRATE: 0x%x\n",
            __func__, (A_UINT32)pSib, (A_UINT32)g_pRATE);
#endif

    if( g_trace_probe && ((__count++ % 1) == 0) ) {
//    if( g_trace_probe ) {
#ifdef debugratectrl
        TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "probe1 0x%x\n", g_trace_probe);
#endif
    }

    /*
        Don't let the function change statistics if the rate schedule
        of this ppdu was not determined through rate control 
    */
    rssiAck = args->ack_rssi;
    nFrames = args->num_enqued;
    retries = args->num_retries;
    nBad    = args->num_failed;

    nowMsec = args->time_stamp;

#ifdef debugratectrl
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "probe_rcupdate: nowMsec = 0x%x %d\n",
            nowMsec, nowMsec);
#endif

    /* Silently return if we reach here in case of a filtered frame. */
    if( retries == 0 ) {
        return;
    }

    A_ASSERT(pRateTable);
    txRate     = RT_WHALCODE_TO_INDEX(pRateTable, args->ptx_rc);

#if 0 //ATH_TARGET
    if (!(pSib->peer_params.ni_flags & RC_PEER_AUTH) ||
        (RT_IS_OFDM_CCK_INDEX(txRate) && 
        (pSib->peer_params.ni_flags & (RC_PEER_HT | RC_PEER_VHT)))) {
        /* called when association is not present or legacy frame but HT 
           /VHT association Ideally we should assert here but given too many
           aserts just send a warning message for now
        */
        DBGLOG_RECORD_LOG(WLAN_MODULE_RATECTRL, -1,
            RATECTRL_DBGID_UNEXPECTED_FRAME, DBGLOG_ERR, 2,
            txRate, pSib->peer_params.ni_flags);
        return; 
    }
#endif

    A_ASSERT(txRate < pRc->rateTableSize);
    
    lastPer = pRc->state[txRate].per;


    /*  get apropriate rate mask to validate index */

    RT_GET_RC_MASK_4_RIX((void *)pRateTable, txRate, &rMaskPos,&rateBit);
    rMask = pRc->validTxRateMask[rMaskPos];
  
    /* If txRate is not in valid rate range, don't let it go into our update module.
       This is becuase NUll data packet might use basic rate but its not included
       in TxRateMask. It will cause we stuck in the basic rate */
    if(!RATEMASK_IS_VALID(rMask, rateBit))
    {
        return;
    }
  
    idx = rc_get_rate_bw_idx(rMaskPos);

#ifdef RATECONTROL_DYNAMIC_BW_DEBUG
    rc_dbg_dyn_bw_err_inj(idx, &nBad, &nFrames, &args->flags); 
#endif

    /* 40Mhz rssi affect borrowed from Fusion code base */

/* TODO
    Don't know history --need to check-- however, this does not sound 
    practical  when ACK was received with diffferent rate/tx power 
    For now only add for 80MHz bandwidth since 160 is still not there
 */
    
    if (args->ptx_rc.flags & WHAL_RC_FLAG_80MHZ) {
        rssiAck = rssiAck < 6? 0: rssiAck - 6;
    } else if (args->ptx_rc.flags & WHAL_RC_FLAG_40MHZ) {
        rssiAck = rssiAck < 3? 0: rssiAck - 3;
    }
    
    if (args->flags & WHAL_RC_TXDONE_X_RETRIES) {
        /* Update the PER. */

        /* first check if this was bw probe which failed */
        if (idx && (pRc->bw_probe_pending == idx) &&
                    (txRate == pRc->probeRate[idx])) {
            pRc->probeRate[idx] = 0;
        }
        else {
            /* We shouldn't penalize severely based on number of tx attempts. */
            pRc->excessRetries++;
            if( pRc->excessRetries > (3>>(retries-1)) ) {
                pRc->state[txRate].per += 30;
                pRc->state[txRate].per =
                   A_MIN(pRc->state[txRate].per, _g_dbg_rc_weight);

                pRc->excessRetries = 0;

                /* adjust PER for all rates above the current to at least equal current PER */
                // FIXME
                //TODO:
                // when you are really using lower index this would just kill performance

                for (rate = txRate + 1; rate < pRc->rateTableSize; rate++) {
                    if (RT_GET_PHY(pRateTable, rate) != RT_GET_PHY(pRateTable, txRate)) {
                        break;
                    }

                    if (pRc->state[rate].per < pRc->state[txRate].per) {
                        pRc->state[rate].per = pRc->state[txRate].per;
                    }
                }
            }

            /* Update the RSSI threshold */
            /*
             * Oops, it didn't work at all.  Set the required rssi
             * to the rssiAck we used to lookup the rate, plus 4dB.
             * The immediate effect is we won't try this rate again
             * until we get an rssi at least 4dB higher.
             */
#ifdef ENABLE_RSSI_BASED_RATECTRL
            /* It is found that increasing rssi for failed frames can, in
             *  certain scenarios, cause the rate logic to lock out higher
             *  rates.  Specifically this problem is seen to occur when
             *  the link is strong (high rssi) and aggregation is used.
             *  Collisions can occur in bursts causing the rssi threshold
             *  to be raised and forcing otherwise valid rates to be discarded
             *  in favor of lower rates.  It is noted that the Fusion
             *  code base has also removed this logic for AR5416 perhaps
             *  because of the same behavior.  */
            if (not_per_only() && (txRate > 0)) {
                A_RSSI rssi = A_MAX(pRc->rssiLastLkup, pRc->rssiLast - 2);
                A_RSSI cap = RT_GET_RSSI_MIN(pRateTable, txRate) + (RT_GET_RSSI_DELTA(pRateTable, txRate)<<1);

                    if(pRc->state[txRate].rssiThres + 2 < cap) {
                        if (pRc->state[txRate].rssiThres + 2 < rssi) {
                            pRc->state[txRate].rssiThres += 2;
                        } else {
                            pRc->state[txRate].rssiThres += 1;
                        }
                    }

                    stateChange         = TRUE;
            }

            /*
             * Also, if we are not doing a probe, we force a significant
             * backoff by reducing rssiLast.  This doesn't have a big
             * impact since we will bump the rate back up as soon as we
             * get any good ACK.  RateMax will also be set to the current
             * txRate if the failed frame is not a probe.
             */
            if (pRc->probeRate[idx] == 0 || pRc->probeRate[idx] != txRate) {

                if (retries > 1) {
                    pRc->rssiLast      = 10 * pRc->state[txRate].rssiThres / 16;
                    pRc->rssiLastPrev  = pRc->rssiLast;
                    pRc->rssiLastPrev2 = pRc->rssiLast;
                }
            }

#endif /* ENABLE_RSSI_BASED_RATECTRL */

            /* only 0 probeRate if it matches current as there may be multiple
             * tx requests submitted */
            if (pRc->probeRate[idx] == txRate) {
                pRc->probeRate[idx] = 0;
                g_dbg_probe_idx = 0;
            	g_trace_probe |= 2;
            }
        }
    } else {
        /* Update the PER.  Tries below is actual hw retries */
        A_UINT32 tries = (retries)? ( retries -1 ) : retries;

        if (nBad) 
        {
            /* new_PER = 7/8*old_PER + 1/8*(currentPER)  */
            /*
             * Assuming that nFrames is not 0.  The current PER
             * from the retries is 100 * retries / (retries+1),
             * since the first retries attempts failed, and the
             * next one worked.  For the one that worked, nBad
             * subframes out of nFrames wored, so the PER for
             * that part is 100 * nBad / nFrames, and it contributes
             * 100 * nBad / (nFrames * (retries+1)) to the above
             * PER.  The expression below is a simplified version
             * of the sum of these two terms.
             */
            if (nFrames > 0)
            {
                pRc->state[txRate].per = (A_UINT8)(pRc->state[txRate].per -
                                     (pRc->state[txRate].per >> 3) +
                ((100*(tries*nFrames + nBad)/(nFrames*(tries+1))) >> 3));
            }
        } 
        else {
            /* new_PER = 7/8*old_PER + 1/8*(currentPER) */

            pRc->state[txRate].per = (A_UINT8)(pRc->state[txRate].per -
            (pRc->state[txRate].per >> 3) + ((100 * tries >> 3) >> 3));
        }

        pRc->excessRetries = (pRc->excessRetries > 0) ? (pRc->excessRetries - 1) : 0;

#ifdef ENABLE_RSSI_BASED_RATECTRL
        pRc->rssiLastPrev2 = pRc->rssiLastPrev;
        pRc->rssiLastPrev  = pRc->rssiLast;
        pRc->rssiTime      = nowMsec;
#endif /* ENABLE_RSSI_BASED_RATECTRL */

        pRc->rssiLast      = rssiAck;

        /*
         * If we got at most one retry then increase the max rate if
         * this was a probe.  Otherwise, ignore the probe.
         */

        if (pRc->probeRate[idx]) {
        	g_dbg_probe++;
        	g_dbg_probe_idx = pRc->probeRate[idx];
        	g_dbg_tx_rate = txRate;
        	g_trace_probe |= 4;
        }

        if (pRc->probeRate[idx] && pRc->probeRate[idx] == txRate) {
        	g_dbg_probe_idx = 0;
//        	rate_sched->is_probe = 0;
//        	rate_sched->probe_rix = 0xff;
//        	args->is_probe = 0;

            if ( retries > 1 || 2 * nBad > nFrames) { 
                /*
                 * if the attempt worked, but more than half
                 * the subframes were bad then consider
                 * the probe a failure.
                 */
                pRc->probeRate[idx] = 0;
                g_trace_probe |= 8;
            } else {
                pRc->rateMax[idx] = pRc->probeRate[idx];
#if 0 /* Mercury logic */
                if (pRc->state[pRc->probeRate].per > 45) {
                    pRc->state[pRc->probeRate].per = 20;
                }
#else /* fusion logic */
                if (pRc->state[pRc->probeRate[idx]].per > 30) {
                    pRc->state[pRc->probeRate[idx]].per = 20;
                }
#endif

                pRc->probeRate[idx] = 0;
                g_trace_probe |= 0x10;
#ifdef debugratectrl
        TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "probe2 0x%x | rateMax %d | idx %d | txRate %d\n",
                g_trace_probe, pRc->rateMax[idx], idx, txRate);
#endif
                /*
                 * Since this probe succeeded, we allow the next probe
                 * twice as soon.  This allows the maxRate to move up
                 * faster if the probes are succesful.
                 */
                pRc->probeTime[idx] = nowMsec - 
                   (RT_GET_PROBE_PERIOD(pRateTable, pRc->rateMax[idx])>>1);
                /* if this was bw probe it's sucessful */
                if (idx && (pRc->bw_probe_pending == idx)) {
                    /* wait for higher, if not already */
                    /* set to 80M if currently on 40MHz 
                      (pSib->peer_params.ni_flags & RC_PEER_80MHZ) */
                    pRc->bw_probe_pending = (idx == 2)?0: idx +1;
                }
            }
        }

        if (retries > 1 || (2 * nBad >= nFrames)) {
            /*
             * Don't update anything.  We don't know if this was because
             * of collisions or poor signal.
             *
             * Later: if rssiAck is close to pRc->state[txRate].rssiThres
             * and we see lots of retries, then we could increase
             * pRc->state[txRate].rssiThres.
             */
            pRc->hwMaxRetryPktCnt[idx] = 0;
        } else {
            /*
             * It worked with one tries.  First ignore bogus (small)
             * rssiAck values.
             */       
            /* < gives fast convergence to higher rate when PER improves*/
            if (RT_GET_RAW_KBPS(pRateTable, txRate) <= 
                RT_GET_RAW_KBPS(pRateTable, pRc->rateMax[idx]) && 
                pRc->hwMaxRetryPktCnt[idx] < 8) {
                pRc->hwMaxRetryPktCnt[idx]++;
            }

#ifdef ENABLE_RSSI_BASED_RATECTRL
            if (not_per_only() && 
                 (rssiAck >= RT_GET_RSSI_MIN(pRateTable, txRate))) {
                /* Average the rssi */
                if (txRate != pRc->rssiSumRate) {
                    pRc->rssiSumRate = txRate;
                    pRc->rssiSum     = pRc->rssiSumCnt = 0;
                }

                pRc->rssiSum += rssiAck;
                pRc->rssiSumCnt++;

                if (pRc->rssiSumCnt >= 4) {
                    A_RSSI32 rssiAckAvg = (pRc->rssiSum + 2) / 4;

                    pRc->rssiSum = pRc->rssiSumCnt = 0;

                    /* Now reduce the current rssi threshold. */
                    if ((rssiAckAvg < pRc->state[txRate].rssiThres + 2) &&
                        (pRc->state[txRate].rssiThres > RT_GET_RSSI_MIN(pRateTable, txRate)))
                    {
                        pRc->state[txRate].rssiThres--;
                    }

                    stateChange = TRUE;
                }
            }
#endif /* ENABLE_RSSI_BASED_RATECTRL */
        }
    }
    
    /* For all cases */

    /*
     * If this rate looks bad (high PER, historically set to 60 but that seems
       too much now so we have 50%) then stop using it for
     * a while (except if we are probing).
     */
    if (pRc->state[txRate].per >= 50 && txRate > 0 &&
        RT_GET_RAW_KBPS(pRateTable, txRate) <= 
            RT_GET_RAW_KBPS(pRateTable, pRc->rateMax[idx])) {

        RC_MASK_INFO_t tmp;
        A_UINT8 ret_val;

        tmp.rc_mask = rMask & g_pRATE->sc_fixRateSet[rMaskPos];
        tmp.idx = rMaskPos;
        
        /* Make sure rateMax mask is valid */
        _rc_get_peer_valid_rc_mask(g_pRATE, pSib, &tmp);

        ret_val = rcGetNextLowerValidTxRate(pRateTable, &tmp,
                        pRc, (A_UINT8) txRate, &pRc->rateMax[idx]);

        if (ret_val != RT_INVAL_IDX) { /* keep probing if at lowest rate */
            /* Don't probe for a little while. */
            pRc->probeTime[idx] = nowMsec;
#ifdef debugratectrl
        TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "probe3 0x%x | rateMax %d | idx %d | txRate %d\n",
                g_trace_probe, pRc->rateMax[idx], idx, txRate);
#endif
        }
    }

#ifdef ENABLE_RSSI_BASED_RATECTRL
    if (stateChange) {
        /*
         * Make sure the rates above this have higher rssi thresholds.
         * (Note:  Monotonicity is kept within the OFDM rates and within the CCK rates.
         *         However, no adjustment is made to keep the rssi thresholds monotonically
         *         increasing between the CCK and OFDM rates.)
         */
        for (rate = txRate; rate < pRc->rateTableSize - 1; rate++) {
            if (RT_GET_PHY(pRateTable, rate+1) != RT_GET_PHY(pRateTable, txRate)) {
                break;
            }

            if (pRc->state[rate].rssiThres + RT_GET_RSSI_DELTA(pRateTable, rate) >
                pRc->state[rate+1].rssiThres)
            {
                pRc->state[rate+1].rssiThres =
                    pRc->state[rate].rssiThres + RT_GET_RSSI_DELTA(pRateTable, rate);
            }
        }

        /* Make sure the rates below this have lower rssi thresholds. */
        for (rate = txRate - 1; rate >= 0; rate--) {
            if (RT_GET_PHY(pRateTable, rate) != RT_GET_PHY(pRateTable, txRate)) {
                break;
            }

            if (pRc->state[rate].rssiThres + RT_GET_RSSI_DELTA(pRateTable, rate) >
                pRc->state[rate+1].rssiThres)
            {
                if (pRc->state[rate+1].rssiThres < RT_GET_RSSI_DELTA(pRateTable, rate)) {
                    pRc->state[rate].rssiThres = 0;
                } else {
                    pRc->state[rate].rssiThres =
                        pRc->state[rate+1].rssiThres - RT_GET_RSSI_DELTA(pRateTable, rate);
                }

                if (pRc->state[rate].rssiThres < RT_GET_RSSI_MIN(pRateTable, rate)) {
                    pRc->state[rate].rssiThres = RT_GET_RSSI_MIN(pRateTable, rate);
                }
            }
        }
    }
#endif /* ENABLE_RSSI_BASED_RATECTRL */

    /* Make sure the rates below this have lower PER */
    /* Monotonicity is kept only for rates below the current rate. */
    if (pRc->state[txRate].per < lastPer) {
        for (rate = txRate - 1; rate >= 0; rate--) {
            if (RT_GET_PHY(pRateTable, rate) != RT_GET_PHY(pRateTable, txRate)) {
                break;
            }

            if (pRc->state[rate].per > pRc->state[rate+1].per) {
                pRc->state[rate].per = pRc->state[rate+1].per;
            }
        }
    }

#ifdef ENABLE_RSSI_BASED_RATECTRL
    /* Every so often, we reduce the thresholds (different for CCK and OFDM). */
    if (not_per_only() && 
        (nowMsec - pRc->rssiDownTime >=
           RT_GET_RSSI_REDUCE(pRateTable, txRate))) {

        A_UINT8 lower_rix, upper_rix;
    
        RT_GET_RIX_LIMITS_4_MASK(&upper_rix, &lower_rix, rMaskPos);

        for (rate = lower_rix; rate <= upper_rix; rate++) {
            if (pRc->state[rate].rssiThres > RT_GET_RSSI_MIN(pRateTable, rate)) {
                pRc->state[rate].rssiThres -= 1;
            }
        }

        pRc->rssiDownTime = nowMsec;
    }
#endif /* ENABLE_RSSI_BASED_RATECTRL */

    /* Every so often, we reduce PER (different for CCK and OFDM). */
    if (nowMsec - pRc->perDownTime[idx] >= 
        (RT_GET_RSSI_REDUCE(pRateTable, txRate)/2)) {
        A_UINT8 lower_rix, upper_rix;
    
        RT_GET_RIX_LIMITS_4_MASK(&upper_rix, &lower_rix, rMaskPos);

        for (rate = lower_rix; rate <= upper_rix; rate++) {
            if (pRc->state[rate].per)
                pRc->state[rate].per = 7*pRc->state[rate].per/8;
        }

        pRc->perDownTime[idx] = nowMsec;
    }


    if (args->is_probe) {
        A_MEMSET(&pRc->probeRate, 0, sizeof (pRc->probeRate));
        g_trace_probe |= 0x20;
    }

#ifdef debugratectrl
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "Exit %s\n", __func__);
#endif

    return;
}


/*
  * API FUNCTIONS START
  */


/*todo: hbrc: used in vdevinit(). should be moved along with it to ratectrl_if.c*/
//#ifdef rcclean
//LOCAL
void _RATE_SetCurmode(RATE_CONTEXT * g_pRATE, WLAN_PHY_MODE mode)
{
    const WHAL_RATE_TABLE *rt;
    A_UINT16 i;

    rt = whalGetRateTable(mode);
    A_ASSERT(rt != NULL);

    g_pRATE->sc_currates = rt;

    /*todo: very important: hbrc: ic_curmode is set in ratectrl but is also
     * referred to in wlan_beacon.c and wlan_umlme.c */
//    dev->ic_curmode = mode;
    
    g_pRATE->sgiMask[0] = 0;
    g_pRATE->sgiMask[1] = 0;

    /*
            TODO
            no support for SGI yet for VHT PHT
            REVISIT
        */
    if (!IS_MODE_VHT(mode)) {

    if(g_pRATE->userSgiMask != INVALID_SGI_MASK)
    {
        g_pRATE->sgiMask[0] = g_pRATE->userSgiMask;
        /* preventive measure; don't allow sgi bit for
         * non HT rates */
	/* The following loop should be only run for HT case
           optimize later on
           REVISIT
        */
        for(i=0 ; i<RT_GET_RATE_COUNT(rt) ; i++)
        {
            if(RT_IS_HT(rt, i) == FALSE)
            {
                RATEMASK_CLR_IDX(g_pRATE->sgiMask[0], g_pRATE->sgiMask[0], i);
            }
        }
    }
    else
    {
        /* default behavior for sgi is to allow sgi only for the MAX mcs rate 
         * for the mode */
        if(RT_IS_HT(rt, RT_GET_INIT_MAXRATE(rt, mode)))
        {
                RATEMASK_SET_IDX(g_pRATE->sgiMask[0], 
                    RT_GET_INIT_MAXRATE(rt, mode));
            }
        }
    }
//A_PRINTF_ALWAYS("new mode= %d\n", mode);
    // Update the fixedRate based on the mode
    g_pRATE->sc_fixedrix = g_pRATE->sc_userRix;
    
    if( g_pRATE->sc_ifmgmtfixed == 0 || 
        RT_IS_VALID_RATE(rt, g_pRATE->sc_defmgtrix, g_pRATE->vdev_params.ic_curmode) == FALSE)
    {
        for(i=0 ; i<RT_GET_RATE_COUNT(rt) ; i++)
        {
            if(RT_IS_VALID_RATE(rt, i, g_pRATE->vdev_params.ic_curmode))
                break;
        }
            
        g_pRATE->sc_defmgtrix = i;
    }            
      
    return;
}
//#endif //rcclean


/* when adding 11N support it became necessary to return
 * the rate index instead of the rate code because for
 * 11N rates the ratecode is not sufficient to describe
 * the rate to the HW. */
A_UINT8 _RATE_GetRtsCtsRate(RATE_CONTEXT *g_pRATE, const WHAL_RATE_TABLE *pRateTable)
{
    A_UINT8 rts_rix   = RT_CODE_TO_INDEX(pRateTable, 0x03, 0);

    return rts_rix;
}

void rate_init_peer_ratectxt(struct rate_node *pSib, void *peer)
{
    struct TxRateCtrl_s *pRc  = NULL;

#ifdef debugratectrl
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "Enter %s | pSib: 0x%x | peer: 0x%x\n",
            __func__, (A_UINT32)pSib, (A_UINT32)peer);
#endif

    pRc  = pSib->txRateCtrl;

    pSib->peer = peer;

    pRc->rssiLast = 0;            /* last ack rssi */
#ifdef ENABLE_RSSI_BASED_RATECTRL
    pRc->rssiLastLkup = 0;        /* last ack rssi used for lookup */
    pRc->rssiLastPrev = 0;        /* previous last ack rssi */
    pRc->rssiLastPrev2 = 0;       /* 2nd previous last ack rssi */
    pRc->rssiSumCnt = 0;          /* count of rssiSum for averaging */
    pRc->rssiSumRate = 0;         /* rate that we are averaging */
    pRc->rssiSum = 0;           /* running sum of rssi for averaging */
#endif /* ENABLE_RSSI_BASED_RATECTRL */
    /* mask of valid rates */
    memset(pRc->validTxRateMask, 0, sizeof(pRc->validTxRateMask));
    pRc->rateTableSize = 0;      /* rate table size */
    pRc->rateMax[0] = 0;         /* max rate that has recently worked */
    A_MEMSET(&pRc->probeRate, 0, sizeof (pRc->probeRate)); /* rate we are probing at */
#ifdef ENABLE_RSSI_BASED_RATECTRL
    pRc->rssiTime = 0;          /* msec timestamp for last ack rssi */
    pRc->rssiDownTime = 0;      /* msec timestamp for last down step */
#endif /* ENABLE_RSSI_BASED_RATECTRL */
    A_MEMSET(&pRc->probeTime, 0, sizeof (pRc->probeTime)); /* msec timestamp for last probe */
    pRc->bw_probe_pending = 0; /* Waiting for higher bandwith probe */
    A_MEMSET(&pRc->hwMaxRetryPktCnt, 0 ,
         sizeof(pRc->hwMaxRetryPktCnt));  /* num packets since we got HW max retry error */
    /* maximum number of valid rate */
    A_MEMSET(pRc->maxValidRate, 0, sizeof (pRc->maxValidRate));
    A_MEMSET(&pRc->perDownTime, 0, sizeof (pRc->perDownTime)); /* msec timstamp for last PER down step */
    pRc->htSgiValid = 0;        /* Flags to allow Short Guard Interval in HT 20MHz or HT 40MHz */
    pRc->excessRetries = 0;

#ifdef debugratectrl
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "Exit %s\n", __func__);
#endif

    return;
}


void rate_init_vdev_ratectxt(RATE_CONTEXT *g_pRATE, void *vdev)
{
#ifdef debugratectrl
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "Enter: %s | g_pRATE: 0x%x | vdev: 0x%x\n",
            __func__, (A_UINT32)g_pRATE, (A_UINT32)vdev);
#endif

    g_pRATE->dev_context = vdev;

    g_pRATE->sc_userRix = -1;
    g_pRATE->sc_fixedrix = -1;

    WHAL_RC_INIT_RC_MASKS(g_pRATE->wmiFixRateSetMask);

    WHAL_RC_INIT_RC_MASKS(g_pRATE->sc_fixRateSet);

    /* WAR mask out MCS 8 and 9 for VHT due to the underrun issue
     * identified with them.
     * TODO REVISIT REMOVE
     */
#if 0
    g_pRATE->wmiFixRateSetMask[3] &= 0x0ff3fcff;
    g_pRATE->sc_fixRateSet[3]     &= 0x0ff3fcff;
    g_pRATE->wmiFixRateSetMask[4] &= 0x0ff3fcff;
    g_pRATE->wmiFixRateSetMask[5] &= 0x0ff3fcff;
    g_pRATE->sc_fixRateSet[4]     &= 0x0ff3fcff;
    g_pRATE->sc_fixRateSet[5]     &= 0x0ff3fcff;
#endif

    /* Set Default HW rate table VHT MCSs Masks anc current NSS */
    g_pRATE->curr_nss = NUM_SPATIAL_STREAM;
    A_MEMCPY(&g_pRATE->mcs_mask_4_nss[0], default_ht_rt_masks,
             sizeof (A_RATEMASK) * NUM_SPATIAL_STREAM);
    A_MEMCPY(&g_pRATE->mcs_mask_4_nss[NUM_SPATIAL_STREAM],
             default_vht_rt_masks, sizeof (A_RATEMASK) * NUM_SPATIAL_STREAM);

    g_pRATE->sc_ifmgmtfixed = 0;
    g_pRATE->sc_defmgtrix = 0;
    g_pRATE->sc_bEnableFrameMask = 0;
    g_pRATE->sc_bFramematch = 0;
    g_pRATE->sc_frameType = 0;
    g_pRATE->sc_frameRateMask = A_RATEMASK_FULL;

    RATE_SetCurmode(g_pRATE, MODE_11G);

    rate_init_vdev_params(g_pRATE);

    A_MEMCPY(g_pRATE->txSelectRate, defaultTxSelectRate, sizeof(defaultTxSelectRate));

    g_pRATE->ht_rate_ampdu_th1 = RT_HT_AMPDU_TH1;
    g_pRATE->ht_rate_ampdu_th2 = RT_HT_AMPDU_TH2;
    g_pRATE->ht_only_th = RT_HT_ONLY_TH;

#ifdef debugratectrl
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "Exit %s\n", __func__);
#endif
    return;
}


/* will be cleaned after test and before check-in*/
#if ATH_TARGET
void wlan_rc_set_max_nss(wlan_vdev_t *dev, A_UINT8 nss)
{
    RATE_CONTEXT * g_pRATE = (RATE_CONTEXT *)dev->pRateCtrl;
    static A_UINT8 orig_nss = 0;
    if (nss && nss <= NUM_SPATIAL_STREAM) {

        DBGLOG_RECORD_LOG(WLAN_MODULE_RATECTRL, -1,
            RATECTRL_DBGID_NSS_CHANGE, DBGLOG_ERR, 1, nss);

        if (!orig_nss) {
            orig_nss = g_pRATE->curr_nss;
        }

        if (nss == NUM_SPATIAL_STREAM) {
           g_pRATE->curr_nss = orig_nss;
           orig_nss = 0; 
        }
        else { /* reduced nss */
 
           g_pRATE->curr_nss = nss;
           /* set corresponding wal level nss */

        }
        wal_rc_set_vdev_max_nss(DEV_GET_WAL_VDEV(dev), nss);

        /* Should we also turn off HW rf chain? */
        /* TODO */
    }
}
#endif



/*****************************************************************************/
/* FUNCTION: RATE_GetTxRetrySchedule
 * 
 * PARAMS:  [IN] void *pdev - pointer to wal device structure.
 *          [IN] RC_TX_RATE_SCHEDULE *rc_sched - pointer to rate schedule.
 *          [IN/OUT] WHAL_RATE_TABLE **rt - HW rate table.
 *          [IN] void *ptid - Pointer to wal tid
 *          [IN] void *arg - The list of arguments 
 *
 * RETURNS: N/A
 *
 * DESCRIPTION: Called by transmit authority to calculate the transmit 
 *      retry schedule for a frame.  The function accounts for frame
 *      attributes, network capabilities and recent tx/rx results.
 *      The returned retry schedule is intended to be used by the
 *      caller to transmit the frame to the intended target.
 */
/*****************************************************************************/

volatile A_UINT32 __my_assert = 1;
volatile void *dbg_peer = NULL;

A_BOOL _RATE_GetTxRetrySchedule(void *pdev,
                  const WHAL_RATE_TABLE ** rt,
                  void *ppeer,
                  void *args,
                  void *pkt_info_rcf)

{
#if ATH_TARGET
    wlan_peer_t *wlan_peer = (wlan_peer_t *)ppeer;
//    wal_peer_t  *peer = (wlan_peer) ? (wal_peer_t *)(wlan_peer->wal_peer) : NULL;
    wlan_vdev_t *dev = (wlan_peer)? (wlan_vdev_t *)wlan_peer->pDev: NULL;
#else
    struct ol_txrx_peer_t *wlan_peer = (struct ol_txrx_peer_t *)ppeer;
    struct ol_txrx_vdev_t *dev = (wlan_peer)?wlan_peer->vdev: NULL;
#endif
//    A_UINT32 flags = rc_args->flags;
    A_BOOL shortPreamble =1; 
    struct rate_node *an = RATE_NODE(wlan_peer);

    RATE_CONTEXT * g_pRATE = (dev) ? (RATE_CONTEXT *)dev->pRateCtrl: NULL;
    const WHAL_RATE_TABLE * pRateTable = NULL;
//    A_BOOL no_ack;
    A_UINT8 tries = 0;
    A_UINT8 txrate = 0;
    A_RATEMASK rateMask;
    A_UINT8 rix;
#ifdef rcclean
#ifndef SUPPORT_VHT
    A_UINT8 policyID = 0; 
    HOST_FRAME_TEMPLATE* pTemplate = NULL;
#endif
#endif //rcclean

    RC_BW_TYPE_e bw, bw_idx;
    RC_MASK_INFO_t rc_mask_info;
    WHAL_RC_MASK_IDX m_idx;
    A_UINT32 rts_cts = _RATE_IsRTS_CTS_Enabled(an);
    A_UINT32 is_rts_cts = (rts_cts >0);

    RC_TX_RATE_SCHEDULE rc_sched;
    A_RATE probe_rix = 0;
    A_RATE prev_bw_rix = 0;
    struct TxRateCtrl_s *pRc        = (an) ? an->txRateCtrl: NULL;

    A_UINT32 nowMsec = (A_UINT32)(size_t)args;

#ifdef debugratectrl
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "probe_retrysched: nowMsec = 0x%x %d\n",
            nowMsec, nowMsec);
#endif

    A_MEMSET(&rc_sched, 0, sizeof (RC_TX_RATE_SCHEDULE));

#ifdef debugratectrl
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "Enter %s | pSib:  | g_pRATE: \n",
            __func__);
#endif

    if( dev == NULL || !g_pRATE || rt == NULL) 
    {
        dbg_peer = ppeer;
        while (__my_assert);
        return FALSE;
    }

//    no_ack = (flags & WAL_RC_FLAG_NO_ACK)?1:0;


//    tries = wal_rc_get_vdev_tx_tries(peer->vdev);
    RC_GET_TX_RETRIES(tries, wlan_peer);
    /* Get initial rate mask and mask index */
    rateMask = 0;
    m_idx = _RATE_GetPeerPhy(an);
    /* Get default rate mask index based on peer capability */
    m_idx = (m_idx >=2)?WHAL_RC_MASK_IDX_VHT_20 :
                (m_idx) ?WHAL_RC_MASK_IDX_HT_20:0;

    /* Non QoS Frame should never be sent HT/VHT rates */
    /* Removed since now this should be taken care of in WAL */
//    m_idx = !(rc_args->frame_ctrl & IEEE80211_FC0_SUBTYPE_QOS)?0:m_idx;

    
    /*
            Populating series/bw rate retry series.
            a) determine whether static/dynamic bw configuration
            b) get apropriate rate mask indices to limit the search
            c) Optionally, when RTS/CTS2Self enabled, program two series
                c.1) both first and second series have try count set to 1
                c.2) second series has same rate setting except RTS/CTS enabled.

            When already running with lowest HT/VHT rates shall we fall down to OFDM/CCK?
            What is the advantage? in Enterprise case it would just kill everyone else.
            In fact most of the time not even lower HT/VHT rates should be used.
            REVISIT
            based on review/comments.
            Remmber going forward at one point of time we do have to leave legacy rates only
            for association to be fair with other smart devices.
        */

    /* Find out connection bw 

        */

    A_ASSERT((an->peer));

    /* Get the Conn bw which is min of (vdev_bw, peer_bw) */
    RC_GET_PEER_BW(bw, wlan_peer);

    /* Find out whether dynamic BW or static BW configuration */
    /* for legacy non-ht as well? */

//    if (wal_rc_is_dyn_bw(pdev)) {
    if (RC_IS_DYN_BW(pdev)) {

       A_UINT32 rt_idx = 0;
        /*  Populate multiple rate series for each supported bw.
            When RTS/CTS is not enabled we only program one rate 
            series. However, when RTS/CTS is enabled we have two
            series. First, original transmission just one try, and 
            second series same original transmission however 
            preceded by RTS/CTS exchange.
            REVIST
            Due to curent nature of ppdu formation RTS/CTS 
            enablement may incur additional overhead, therefore
            verify the side effect in terms of performance.
        */
        
        rc_sched.tries[0] = (is_rts_cts) ? 1: tries;
        
        for (bw_idx = 0; bw_idx <= bw; bw_idx++) {

            rc_mask_info.idx = m_idx + bw_idx;
            rc_mask_info.rc_mask = 0;

           /* Check if higher bandwiths are not waiting to be probed before
              getting those higher bandwith rates
           */
           if (bw_idx && (pRc->bw_probe_pending == bw_idx)) { /* 40/80MHz probe waiting*/
               A_RATE lower_rix, upper_rix, i;

               if (pRc->probeTime[bw_idx] && 
                  (pRc->probeTime[bw_idx] > nowMsec)) {
                  /* Probe interval not expired still waiting */
                  break;
               }
               
               /* If the previous rate (previous lower bw) is already lowest,
                  and we have not transmitted successfully enough frames on
                  that rate, don't probe higher bw yet but wait for channel
                  to improve rather
               */
               if ((pRc->validRateIndex[rc_mask_info.idx - 1][0] == 
                       prev_bw_rix) ||
                   (pRc->hwMaxRetryPktCnt[bw_idx - 1] < 4)
                   /* || (pRc->state[prev_bw_rix].per > 30) */) {
                   break;
               }
               /* Time to turn on higher bandwiths again:
                      1. Get valid rate >= prev_bw_rix
                      2. Limit the aggreate, similar to probe, 
                      3. Set RateMax to valid rate
                      4. ageout per for this bw so it can rate adapt
               */

               RT_GET_RIX_LIMITS_4_MASK(&upper_rix, &lower_rix,  
                  rc_mask_info.idx); 
               
               rix = RT_INVALID_INDEX;
               /* Get valid rate mask */
               _rc_get_peer_valid_rc_mask(g_pRATE, an, &rc_mask_info);

               for (i = lower_rix; i <= upper_rix; i++) {
                   /* Reset PER since rate is just higher than lower bw */
                   pRc->state[i].per = 0;
#ifdef ENABLE_RSSI_BASED_RATECTRL
                   pRc->state[i].rssiThres = 0;
#endif /* ENABLE_RSSI_BASED_RATECTRL */

                   /* Skip invalid rates */
                   if (!RATEMASK_IS_VALID(rc_mask_info.rc_mask, 
                        (i - lower_rix))){
                       continue;
                   }

                   /* pRateTable will be populated here by 20MHz */
                   if (rix == RT_INVALID_INDEX && 
                       (RT_GET_RAW_KBPS(pRateTable, prev_bw_rix) <
                         RT_GET_RAW_KBPS(pRateTable, i))) {
                         rix = i; /* Save this rate to be used */
                   }
               }
               pRc->perDownTime[bw_idx] = nowMsec;
#ifdef ENABLE_RSSI_BASED_RATECTRL
               pRc->rssiDownTime = nowMsec;
#endif /* ENABLE_RSSI_BASED_RATECTRL */

               A_ASSERT(rix != RT_INVALID_INDEX);
 
               /* Set current rate since we have good rate */
               pRc->probeRate[bw_idx] = probe_rix = rix;
               /* In case, both 40/80 MHz are disabled, keep higher bw still 
                  disabled
               */
               /* Disable all higher bw for now */
               while (bw_idx <= bw) {
                   /* what about time wrap */
                   pRc->probeTime[bw--] = /* use rate probe time?*/
                       nowMsec + (RT_GET_PROBE_PERIOD(pRateTable, rix) <<
                                    g_rc_bw_probe_shift_factor);
               }

               /* Skip getRateTbleAndIndex now */
           }
           else {
            
                GetRateTblAndIndex(g_pRATE, &probe_rix, shortPreamble, an,
                    &txrate, &pRateTable, &rix, 0, &rc_mask_info, nowMsec);
             /* It may be possible that, in dynamic bw scenario, that rate
                returned for higher bw goodput is less than current rate for
                lower bw, in that case disable higher bw for a while.
                Eventually, we would probe for higher bw to see if that is
                working and if yes, enable using that bw from thereon.
              */
               if (bw_idx && /* Lower BW rate is better */
                  ((RT_GET_RAW_KBPS(pRateTable, prev_bw_rix) >=
                         RT_GET_RAW_KBPS(pRateTable, rix)) ||
                   /* or, running at lowest MCS and PER is high */
                  ((rix == pRc->validRateIndex[rc_mask_info.idx][0]) &&
                         (pRc->state[rix].per > 30)))) {
                   /* Disable all higher bw for now */
                   pRc->bw_probe_pending = bw_idx;

                   while (bw_idx <= bw) {
                       /* what about time wrap? */
                       pRc->probeTime[bw--] = /* use rate probe time?*/
                           nowMsec + (RT_GET_PROBE_PERIOD(pRateTable, rix) <<
                                  g_rc_bw_probe_shift_factor);
                   }
               }

           }

           prev_bw_rix = rix;
           /* Set Primary rate */
           rc_sched.rix[rt_idx] = rix;
            
           /*
             With dynamic BW we have a limitation here where we cannot
             just send a probe frame on fixed bw. Therefore, when we 
             are probing for any bw, dont program higher bw anymore
             There is still a chance that it may use lower bw, which is
             not a probe, e.g. CCA on secondary got busy, however, currently
             that is handled as skipped probe and we try again later on.
            */
            if (probe_rix) {
                /* Terminate rate schedule here */
                bw = bw_idx;
                /* Limit the HW tries to 1 */
                rc_sched.tries[0] = 1;
                /* disable second tries if enabled by previous rates */
                rc_sched.tries[1] = 0;
                /* 
                   In general, probe and primary rate may have different
                   capability, e.g., STBC, SGI. Ideally, it would be nice
                   to store probe flags similar to rateflags, but due to 
                   memory constraint for now lets use probe rate's properties
                   for primary rate as well, since this is going to be 
                   thrown after 1/2 ppdus transmissions.
                */
                /* TODO: Add probe flag separately or change design to always
                   have probe rix == primary rix
                */
                rix = probe_rix;

#ifdef debugratectrl
        TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "probe4 0x%x | proberix %d\n", g_trace_probe, probe_rix);
#endif
                g_trace_probe = 1;
                /* there might be some entries programmed because of RTS/CTS
                   for previous bw but that should be fine since 
                   num_valid_rates stump out 
                */
                /* set the probe flag, to mark this rate special */
//                rc_args->flags |= WAL_RC_FLAG_PROBE_RATE;
//                *(rc_args->subfrm_limit_ptr) =
//                   A_MIN(g_rc_probe_msdu_limit, *(rc_args->subfrm_limit_ptr));
            }

            /* Set apropriate BW flag */
            RT_GET_BW_FLAG(rc_sched.flags[rt_idx],pRateTable,rix);
            
            /* Set SGI flag */
            rc_sched.flags[rt_idx] |=
                _RATE_GetSGIFlag(an, g_pRATE, pRateTable, rix);
            
            /* STBC, LDPC */
            rc_sched.flags[rt_idx] |=
                _RATE_GetRCFlags(an, g_pRATE, pRateTable, rix);

            /* Chain masks */
            rc_sched.flags[rt_idx] |=
                _RATE_GetTxChainMask(an, pRateTable, rix);
                
            /* set default tpc */
//            rc_sched.tpc[rt_idx] = wal_rc_get_vdev_tpc(peer->vdev);
            RC_GET_TPC(rc_sched.tpc[rt_idx], wlan_peer);
                     
            /*
                     enable RTS/CTS if needed
                      */
            if (is_rts_cts && !(probe_rix) && (tries > 1)) {
                rt_idx++;
                /* Use the same rate series as original transmission except
                                with RTS/CTS enabled
                            */
                rc_sched.tries[1] = tries -1;
                rc_sched.rix[rt_idx] = rix;

                /* set default tpc */
//                rc_sched.tpc[rt_idx + bw + 1] =
//                      wal_rc_get_vdev_tpc(peer->vdev);
                RC_GET_TPC(rc_sched.tpc[rt_idx], wlan_peer);

                /* Set apropriate flags (original + RTS/CTS + series 1 flag) */
                rc_sched.flags[rt_idx ] =
                    rc_sched.flags[rt_idx-1] | WHAL_RC_FLAG_SERIES1;

                if (rts_cts & 1) {
                    rc_sched.flags[rt_idx] |=
                        WHAL_RC_FLAG_RTSENA;
                    rc_sched.bw_in_service = RT_IS_VHT_INDEX(rix)?2:0;
                }
                else {
                    rc_sched.flags[rt_idx] |=
                        WHAL_RC_FLAG_CTSENA;
                }
                rc_sched.rts_rix = RATE_GetRtsCtsRate(g_pRATE, pRateTable);
            }
            rt_idx++;
        }
        
        rc_sched.num_valid_rates = rt_idx;

    }
    else { /* Static BW mode */
        /* only one rate series */
        rc_sched.num_valid_rates = 1;
        rc_mask_info.idx = m_idx + bw;
        rc_mask_info.rc_mask = rateMask;

        GetRateTblAndIndex(g_pRATE, &probe_rix, shortPreamble, an,
            &txrate, &pRateTable, &rix, 0, &rc_mask_info, nowMsec);

        /* Set Primary rate */
        rc_sched.rix[0] = rix;

        if (probe_rix) {
                /* Limit the HW tries to 1 */
                rc_sched.tries[0] = 1;
                /* disable second tries if enabled by previous rates */
                rc_sched.tries[1] = 0;
                /*
                   In general, probe and primary rate may have different
                   capability, e.g., STBC, SGI. Ideally, it would be nice
                   to store probe flags similar to rateflags, but due to
                   memory constraint for now lets use probe rate's properties
                   for primary rate as well, since this is going to be
                   thrown after 1/2 ppdus transmissions.
                */
                /* TODO: Add probe flag separately or change design to always
                   have probe rix == primary rix
                */
                rix = probe_rix;


                /* set the probe flag, to mark this rate special */
//                rc_args->flags |= WAL_RC_FLAG_PROBE_RATE;
//                *(rc_args->subfrm_limit_ptr) =
//                   A_MIN(g_rc_probe_msdu_limit, *(rc_args->subfrm_limit_ptr));

        }
        else {
             rc_sched.tries[0] = (is_rts_cts) ? 1: tries;
        }

        /* Set apropriate BW flag */
        RT_GET_BW_FLAG(rc_sched.flags[0],pRateTable,rix);

        /* Set SGI flag */
        rc_sched.flags[0] |=
            _RATE_GetSGIFlag(an, g_pRATE, pRateTable, rix);

        /* STBC, LDPC */
        rc_sched.flags[0] |=
            _RATE_GetRCFlags(an, g_pRATE, pRateTable, rix);

        /* Chain masks */
        rc_sched.flags[0] |=
            _RATE_GetTxChainMask(an, pRateTable, rix);

         /* set default tpc */
//        rc_sched.tpc[0] = wal_rc_get_vdev_tpc(peer->vdev);
        RC_GET_TPC(rc_sched.tpc[0], wlan_peer);

                 
        /* enable RTS/CTS if needed */
        if (is_rts_cts && !(probe_rix) && (tries > 1)) {
            /* Use the same rate series as original transmission except
                            with RTS/CTS enabled
                        */
            rc_sched.num_valid_rates++;
            rc_sched.tries[1] = tries -1;
            rc_sched.rix[1] = rix;

            /* Set apropriate flags (original + RTS/CTS + series 1 flag) */
            rc_sched.flags[1] =
                rc_sched.flags[0] | WHAL_RC_FLAG_SERIES1;

            if (rts_cts & 1) {
                rc_sched.flags[1] |=
                    WHAL_RC_FLAG_RTSENA;
                rc_sched.bw_in_service = RT_IS_VHT_INDEX(rix)?2:0;
            }
            else {
                rc_sched.flags[1] |=
                    WHAL_RC_FLAG_CTSENA;
            }
            rc_sched.rts_rix = RATE_GetRtsCtsRate(g_pRATE, pRateTable);
        }
    }


    *rt = pRateTable;

//    rc_sched.is_probe = is_probe;
    rc_sched.probe_rix = probe_rix;

#ifdef debugratectrl
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "psdu_len: %d %d %d %d %d %d\n",
            rc_sched.psdu_len[0],rc_sched.psdu_len[1],rc_sched.psdu_len[2],
            rc_sched.psdu_len[3],rc_sched.psdu_len[4],rc_sched.psdu_len[5]);
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "flags: %d %d %d %d %d %d\n",
            rc_sched.flags[0],rc_sched.flags[1],rc_sched.flags[2],
            rc_sched.flags[3],rc_sched.flags[4],rc_sched.flags[5]);
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "rix: %d %d %d %d %d %d\n",
            rc_sched.rix[0],rc_sched.rix[1],rc_sched.rix[2],
            rc_sched.rix[3],rc_sched.rix[4],rc_sched.rix[5]);
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "tpc: %d %d %d %d %d %d\n",
            rc_sched.tpc[0],rc_sched.tpc[1],rc_sched.tpc[2],
            rc_sched.tpc[3],rc_sched.tpc[4],rc_sched.tpc[5]);
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "num_mpdus: %d %d %d %d %d %d\n",
            rc_sched.num_mpdus[0],rc_sched.num_mpdus[1],rc_sched.num_mpdus[2],
            rc_sched.num_mpdus[3],rc_sched.num_mpdus[4],rc_sched.num_mpdus[5]);
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "antmask: %d %d \n",
            rc_sched.antmask[0],rc_sched.antmask[1]);
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "tries: %d %d \n",
            rc_sched.tries[0],rc_sched.tries[1]);
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "txbf_cv_ptr: %d | txbf_cv_len: %d \n",
            rc_sched.txbf_cv_ptr,rc_sched.txbf_cv_len);
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "num_valid_rates: %d | paprd_mask: %d \n",
            rc_sched.num_valid_rates,rc_sched.paprd_mask);
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "rts_rix: %d | sh_pream: %d \n",
            rc_sched.rts_rix,rc_sched.sh_pream);
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "min_spacing_1_4_us: %d | fixed_delims: %d \n",
            rc_sched.min_spacing_1_4_us,rc_sched.fixed_delims);
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "bw_in_service: %d | probe_rix: %d \n",
            rc_sched.bw_in_service,rc_sched.probe_rix);
#endif

    ratectrl_update_rate_sched(an, &rc_sched, pkt_info_rcf);

#ifdef debugratectrl
    TXRX_PRINT(TXRX_PRINT_LEVEL_INFO1, "Exit %s\n", __func__);
#endif

    return TRUE;
}

void _RATE_SetHTConnCap(struct rate_node *pSib, A_UINT32 htCap)
{
    struct TxRateCtrl_s *pRc        = pSib->txRateCtrl;
#if 0
    /* Not sure why this was done here - the host actually passes down
     * an intersection of the SGI capabilities of the peer and ours.
     * Commenting out for now. */

    A_UINT8 band = (dev->ic_curmode == MODE_11A || 
                    dev->ic_curmode == MODE_11NA_HT20 || 
                    dev->ic_curmode == MODE_11NA_HT40 ||
                    IS_MODE_VHT(dev->ic_curmode))? A_BAND_5GHZ : A_BAND_24GHZ;
    A_UINT16 dev_hc_cap = A_LE_READ_2((&dev->ht_cap_ie_info[band].hc_ie.hc_cap));

    /* bit 5 of ht_cap indicates allowed use of SGI 
     * in 20MHz mcs rates. While bit 6 of ht_cap indicates
     * allowed use of SGI in 40Mhz mcs rates. */
    if((htCap & IEEE80211_HTCAP_C_SHORTGI20) && 
        (dev_hc_cap & IEEE80211_HTCAP_C_SHORTGI20))
    {
        pRc->htSgiValid |= TX_RATE_SGI_20;
    }

    if((htCap & IEEE80211_HTCAP_C_SHORTGI40) && 
        (dev_hc_cap & IEEE80211_HTCAP_C_SHORTGI40))
    {
        pRc->htSgiValid |= TX_RATE_SGI_40; 
    }
#endif // 0

    if( htCap & IEEE80211_HTCAP_C_SHORTGI20 ) {
        pRc->htSgiValid |= TX_RATE_SGI_20;
    }

    if( htCap & IEEE80211_HTCAP_C_SHORTGI40 ) {
        pRc->htSgiValid |= TX_RATE_SGI_40;
    }

    return;
}

void _RATE_SetVHTConnCap(struct rate_node *pSib, A_UINT32 vhtCap)
{
    struct TxRateCtrl_s *pRc        = pSib->txRateCtrl;

    if( vhtCap & IEEE80211_VHTCAP_SHORTGI_80 ) {
        pRc->htSgiValid |= TX_RATE_SGI_80;
    }

    if( vhtCap & IEEE80211_VHTCAP_SHORTGI_160 ) {
        pRc->htSgiValid |= TX_RATE_SGI_160;
    }

    return;
}

#ifdef NUM_SYMBOLS_PER_USEC
#undef NUM_SYMBOLS_PER_USEC
#endif
#define NUM_SYMBOLS_PER_USEC(_u) (((_u)+ 3) >> 2)

#ifdef NUM_SYMBOLS_PER_USEC_HALFGI
#undef NUM_SYMBOLS_PER_USEC_HALFGI
#endif
#define NUM_SYMBOLS_PER_USEC_HALFGI(_u) (((_u)*5 + 17)/18)

#ifdef ATH_AGGR_MINPLEN
#undef ATH_AGGR_MINPLEN
#endif
#define ATH_AGGR_MINPLEN 256


#define BITS_PER_BYTE 8
#define BITS_PER_BYTE_SHIFT 3
#define ATH_AGGR_DELIM_SZ 4
#define ATH_AGGR_DELIM_SZ_SHIFT 2
#define ATH_AGGR_GET_NDELIM(_len)                                   \
    (((((_len) + ATH_AGGR_DELIM_SZ) < ATH_AGGR_MINPLEN) ?           \
      (ATH_AGGR_MINPLEN - (_len) - ATH_AGGR_DELIM_SZ) : 0) >> 2)
/* The calculation for this value assumes the following: 
 * mpdudensity = 16 usec (worst case value)
 * raw rate = MCS 7 @ 40 MHz using SGI
 * SGI has 3.6 usec per symbol which translates to 5 symbols in 16 usec
 * MCS 7 @ 40 MHz can produce 540 bits per symbol
 * round up to the nearest byte and done. */
#define MPDU_DENSITY_MAX_FRAME_LEN 338 // (5 symbols * 540 bitspersymbol + 7) / 8   

#define MPDU_DENSITY_MAX_FOR_1   (9)
#define MPDU_DENSITY_MAX_FOR_2   (9)
#define MPDU_DENSITY_MAX_FOR_4  (26)
#define MPDU_DENSITY_MAX_FOR_8  (43)
#define MPDU_DENSITY_MAX_FOR_16 (77)

static const A_UINT8 MPDUDensityMax[17] = 
{
    0,
    MPDU_DENSITY_MAX_FOR_1,
    MPDU_DENSITY_MAX_FOR_2,
    MPDU_DENSITY_MAX_FOR_4,
    MPDU_DENSITY_MAX_FOR_4,
    MPDU_DENSITY_MAX_FOR_8,
    MPDU_DENSITY_MAX_FOR_8,
    MPDU_DENSITY_MAX_FOR_8,
    MPDU_DENSITY_MAX_FOR_8,
    MPDU_DENSITY_MAX_FOR_16,
    MPDU_DENSITY_MAX_FOR_16,
    MPDU_DENSITY_MAX_FOR_16,
    MPDU_DENSITY_MAX_FOR_16,
    MPDU_DENSITY_MAX_FOR_16,
    MPDU_DENSITY_MAX_FOR_16,
    MPDU_DENSITY_MAX_FOR_16,
    MPDU_DENSITY_MAX_FOR_16,       
};


#ifndef DISABLE_RATE_INDIRECTION_TABLE
RATE_API_INDIRECTION_TABLE rateApiIndirectionTable = {
    _RATE_GetTxRetrySchedule,
    _RATE_SetCurmode,
    _RATE_GetRtsCtsRate,
    _RATE_SetHTConnCap,
    _RATE_SetVHTConnCap,
};

RATE_INTERNAL_INDIRECTION_TABLE rateInternalIndirectionTable = {
    _rcRateFind,
    _rcUpdate_HT,
    _rcSibUpdate,
    _GetRateTblAndIndex,
    _RateValidCheck,
    _rcGetNextValidTxRate,
    _rcGetNextLowerValidTxRate,
    _rcGetLowerValid,
    _RateValidCheckNodeFlags,
    _rcSortValidRates,
    _GetLSB,
    _GetMSB,
    _rate_setup,
    _GetBestRate,
    _RateCheckDefaultMCSMask,
    _RateInMCSSetCheck,
};

#endif /* DISABLE_RATE_INDIRECTION_TABLE */
