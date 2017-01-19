/*
 * Copyright (c) 2011, Atheros Communications Inc.
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
 * LMAC CWM specific offload interface functions for UMAC - for power and performance offload model
 */
#include "ol_if_athvar.h"
#include "wmi_unified_api.h"
#include "_ieee80211.h"
#include "ol_if_athpriv.h"

#if ATH_PERF_PWR_OFFLOAD

#define OL_CWM_IEEE80211_EXTPROTMODE enum ieee80211_cwm_extprotmode
#define OL_CWM_IEEE80211_EXTPROTSPACING enum ieee80211_cwm_extprotspacing
#define OL_CWM_IEEE80211_MODE enum ieee80211_cwm_mode
#define OL_CWM_IEEE80211_WIDTH enum ieee80211_cwm_width




OL_CWM_IEEE80211_EXTPROTMODE ol_cwm_get_extprotmode(struct ieee80211com *ic)
{
    /* TBD */
    return IEEE80211_CWM_EXTPROTNONE;
}

int8_t ol_cwm_get_extoffset(struct ieee80211com *ic)
{
    struct ieee80211_channel *curchan;
 
    curchan = ic->ic_curchan;


    if (IEEE80211_IS_CHAN_11AC_VHT40PLUS(curchan) || IEEE80211_IS_CHAN_11NG_HT40PLUS(curchan)
            || IEEE80211_IS_CHAN_11NA_HT40PLUS(curchan)) {
            return 1;
    }

    if (IEEE80211_IS_CHAN_11AC_VHT40MINUS(curchan) || IEEE80211_IS_CHAN_11NG_HT40MINUS(curchan)
            || IEEE80211_IS_CHAN_11NA_HT40MINUS(curchan)) {
            return -1;
    } 
    
    if (IEEE80211_IS_CHAN_11AC_VHT80(curchan)) {
        /* The following logic generates the extension channel offset from the primary channel(ic_ieee) and 80M channel
           central frequency. The channelization for 80M is as following: 
	   | 20M  20M  20M  20M | with the following example | 36 40 44 48 | 
	   |         80M        |			     |     80M     |
           The central frequency is 42 in the example. If the primary channel is 36 and 44, the extension channel is 40PLUS.
	   If the primary channel is 40 and 48 the extension channel is 40MINUS. */ 	
	              	
        if(curchan->ic_ieee < curchan->ic_vhtop_ch_freq_seg1) {
            if((curchan->ic_vhtop_ch_freq_seg1 - curchan->ic_ieee) > 4)
                return 1;
            else
                return -1;
        } else {
            if((curchan->ic_ieee - curchan->ic_vhtop_ch_freq_seg1) > 4)
                return -1;
            else
                return 1;
        }            
    }
        
    return 0;
}

void ol_cwm_set_extprotmode(struct ieee80211com *ic, OL_CWM_IEEE80211_EXTPROTMODE val)
{
    /* TBD */
    return;
}
void ol_cwm_set_extprotspacing(struct ieee80211com *ic, OL_CWM_IEEE80211_EXTPROTSPACING val)
{
    /* TBD */
    return;
}

void ol_cwm_set_enable(struct ieee80211com *ic, u_int32_t val)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    if(wmi_unified_pdev_set_param(scn->wmi_handle,
        WMI_PDEV_PARAM_DYNAMIC_BW, val)==0){
        scn->scn_cwmenable = val;
    }

    return;
}

void ol_cwm_set_extbusythreshold(struct ieee80211com *ic, u_int32_t val)
{
    /* TBD */
    return;
}
void ol_cwm_set_width(struct ieee80211com *ic, OL_CWM_IEEE80211_WIDTH val)
{
    /* TBD */
    return;
}

void ol_cwm_set_extoffset(struct ieee80211com *ic, int8_t val)
{
    /* TBD */
    return;
}

void ol_cwm_set_mode(struct ieee80211com *ic, OL_CWM_IEEE80211_MODE ieee_mode)
{
    /* TBD */
    return;
}

OL_CWM_IEEE80211_MODE ol_cwm_get_mode(struct ieee80211com *ic)
{
    /* TBD */
    return IEEE80211_CWM_MODE20;
}

void ol_ath_cwm_switch_to40(struct ieee80211com *ic)
{
    /* TBD */
    return;
}

void
ol_cwm_switch_to20(struct ieee80211com *ic)
{
    /* TBD */
    return;
}

OL_CWM_IEEE80211_EXTPROTSPACING 
ol_cwm_get_extprotspacing(struct ieee80211com *ic)
{
    /* TBD */
    return IEEE80211_CWM_EXTPROTSPACING20;
}

u_int32_t ol_cwm_get_enable(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    return (scn->scn_cwmenable);
}

u_int32_t ol_cwm_get_extbusythreshold(struct ieee80211com *ic)
{
    /* TBD */
    return 0;
}

OL_CWM_IEEE80211_WIDTH ol_cwm_get_width(struct ieee80211com *ic)
{
    struct ieee80211_channel *curchan;
    
    curchan = ic->ic_curchan;
           
    if (IEEE80211_IS_CHAN_11AC(curchan)) {
        if (IEEE80211_IS_CHAN_11AC_VHT80(curchan)) {
            return IEEE80211_CWM_WIDTH80;
        }
        if (IEEE80211_IS_CHAN_11AC_VHT40(curchan)) {
            return IEEE80211_CWM_WIDTH40;
        }
        if (IEEE80211_IS_CHAN_11AC_VHT20(curchan)) {
            return IEEE80211_CWM_WIDTH20;
        }
    }

    if ( IEEE80211_IS_CHAN_11N(curchan)) {
        if (IEEE80211_IS_CHAN_11NA_HT20(curchan) || IEEE80211_IS_CHAN_11NG_HT20(curchan)) {
            return IEEE80211_CWM_WIDTH20;
        } else {
            return IEEE80211_CWM_WIDTH40;
        }
    }

    return IEEE80211_CWM_WIDTH20;
}





/* Intialization functions */
int
ol_ath_cwm_attach(struct ol_ath_softc_net80211 *scn)
{
    struct ieee80211com     *ic     = &scn->sc_ic;
    /* Install CWM APIs */
    ic->ic_cwm_get_extprotmode = ol_cwm_get_extprotmode;
    ic->ic_cwm_get_extoffset = ol_cwm_get_extoffset;
    ic->ic_cwm_get_extprotspacing = ol_cwm_get_extprotspacing;
    ic->ic_cwm_get_enable = ol_cwm_get_enable;
    ic->ic_cwm_get_extbusythreshold = ol_cwm_get_extbusythreshold;
    ic->ic_cwm_get_mode = ol_cwm_get_mode;
    ic->ic_cwm_get_width = ol_cwm_get_width;
    ic->ic_cwm_set_extprotmode = ol_cwm_set_extprotmode;
    ic->ic_cwm_set_extprotspacing = ol_cwm_set_extprotspacing;
    ic->ic_cwm_set_enable = ol_cwm_set_enable;
    ic->ic_cwm_set_extbusythreshold = ol_cwm_set_extbusythreshold;
    ic->ic_cwm_set_mode = ol_cwm_set_mode;
    ic->ic_cwm_set_width = ol_cwm_set_width;
    ic->ic_cwm_set_extoffset = ol_cwm_set_extoffset;
    ic->ic_bss_to40 = ol_ath_cwm_switch_to40;
    ic->ic_bss_to20 = ol_cwm_switch_to20;
    return 0;
}

#endif
