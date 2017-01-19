/*
 *  Copyright (c) 2008 Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */

#include <ieee80211_api.h>
#include <ieee80211_var.h>
#include <ieee80211_config.h>
#include <ieee80211_rateset.h>
#include <ieee80211_channel.h>
#include <ieee80211_resmgr.h>
#include <ieee80211_target.h>

 
#if UMAC_SUPPORT_WNM
static OS_TIMER_FUNC(ieee80211_bssload_timeout)
{
    struct ieee80211com *ic;
    OS_GET_TIMER_ARG(ic, struct ieee80211com *);
    ieee80211_wnm_bss_validate_inactivity(ic);
    OS_SET_TIMER(&ic->ic_bssload_timer, IEEE80211_BSSLOAD_WAIT  * HZ);
}
#endif

int
isvalid_vht_mcsmap(u_int16_t mcsmap)
{
    /* Valid VHT MCS MAP
      * 0xfffc: NSS=1 MCS 0-7
      * 0xfff0: NSS=2 MCS 0-7
      * 0xffc0: NSS=3 MCS 0-7
      * 0xfffd: NSS=1 MCS 0-8
      * 0xfff5: NSS=2 MCS 0-8
      * 0xffd5: NSS=3 MCS 0-8
      * 0xfffe: NSS=1 MCS 0-9
      * 0xfffa: NSS=2 MCS 0-9
      * 0xffea: NSS=3 MCS 0-9
      */

    if ((mcsmap == VHT_MCSMAP_NSS1_MCS0_7) || (mcsmap == VHT_MCSMAP_NSS2_MCS0_7) ||
        (mcsmap == VHT_MCSMAP_NSS3_MCS0_7) || (mcsmap == VHT_MCSMAP_NSS1_MCS0_8) ||
        (mcsmap == VHT_MCSMAP_NSS2_MCS0_8) || (mcsmap == VHT_MCSMAP_NSS3_MCS0_8) ||
        (mcsmap == VHT_MCSMAP_NSS1_MCS0_9) || (mcsmap == VHT_MCSMAP_NSS2_MCS0_9) ||
        (mcsmap == VHT_MCSMAP_NSS3_MCS0_9)) {
        return 1;
    }

    return 0;
}

int
wlan_set_param(wlan_if_t vaphandle, ieee80211_param param, u_int32_t val)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
	int is2GHz = IEEE80211_IS_CHAN_2GHZ(ic->ic_curchan);
    int retv = 0;

    switch (param) {
	case IEEE80211_SET_TXPWRADJUST:
        if(ic->ic_set_txPowerAdjust)
            ic->ic_set_txPowerAdjust(ic, 2*val, is2GHz);
        break;
    case IEEE80211_AUTO_ASSOC:
       if (val)
            IEEE80211_VAP_AUTOASSOC_ENABLE(vap);
        else
            IEEE80211_VAP_AUTOASSOC_DISABLE(vap);
        break;

    case IEEE80211_SAFE_MODE:
        if (val)
            IEEE80211_VAP_SAFEMODE_ENABLE(vap);
        else
            IEEE80211_VAP_SAFEMODE_DISABLE(vap);
        if (ic->ic_set_safemode) {
            ic->ic_set_safemode(vap, val);
        }
        break;

    case IEEE80211_SEND_80211:
        if (val)
            IEEE80211_VAP_SEND_80211_ENABLE(vap);
        else
            IEEE80211_VAP_SEND_80211_DISABLE(vap);
        break;

    case IEEE80211_RECEIVE_80211:
        if (val)
            IEEE80211_VAP_DELIVER_80211_ENABLE(vap);
        else
            IEEE80211_VAP_DELIVER_80211_DISABLE(vap);
        break;

    case IEEE80211_FEATURE_DROP_UNENC:
        if (val)
            IEEE80211_VAP_DROP_UNENC_ENABLE(vap);
        else
            IEEE80211_VAP_DROP_UNENC_DISABLE(vap);
        break;

#ifdef ATH_COALESCING
    case IEEE80211_FEATURE_TX_COALESCING:
        ic->ic_tx_coalescing = val;
        break;
#endif
    case IEEE80211_SHORT_PREAMBLE:
        if (val)
           IEEE80211_ENABLE_CAP_SHPREAMBLE(ic);
        else
           IEEE80211_DISABLE_CAP_SHPREAMBLE(ic);
         retv = EOK;
        break;    
    
    case IEEE80211_SHORT_SLOT:
        if (val)
            ieee80211_set_shortslottime(ic, 1);
        else
            ieee80211_set_shortslottime(ic, 0);
        break;

    case IEEE80211_RTS_THRESHOLD:
        /* XXX This may force us to flush any packets for which we are 
           might have already calculated the RTS */
        if (val > IEEE80211_RTS_MAX)
            vap->iv_rtsthreshold = IEEE80211_RTS_MAX;
        else
            vap->iv_rtsthreshold = (u_int16_t)val;
        break;

    case IEEE80211_FRAG_THRESHOLD:
        /* XXX We probably should flush our tx path when changing fragthresh */
        if (val > 2346)
            vap->iv_fragthreshold = 2346;
        else if (val < 256)
            vap->iv_fragthreshold = 256;
        else
            vap->iv_fragthreshold = (u_int16_t)val;
        break;

    case IEEE80211_BEACON_INTVAL:
        ic->ic_intval = (u_int16_t)val;
        LIMIT_BEACON_PERIOD(ic->ic_intval);
        break;

#if ATH_SUPPORT_AP_WDS_COMBO
    case IEEE80211_NO_BEACON:
        vap->iv_no_beacon = (u_int8_t) val;
        ic->ic_set_config(vap);
        break;
#endif
    case IEEE80211_LISTEN_INTVAL:
        LIMIT_LISTEN_INTERVAL(val);
        ic->ic_lintval = val;
        vap->iv_bss->ni_lintval = ic->ic_lintval;
        break;

    case IEEE80211_ATIM_WINDOW:
        LIMIT_BEACON_PERIOD(val);
        vap->iv_atim_window = (u_int8_t)val;
        break;

    case IEEE80211_DTIM_INTVAL:
        LIMIT_DTIM_PERIOD(val);
        vap->iv_dtim_period = (u_int8_t)val;
        break;

    case IEEE80211_BMISS_COUNT_RESET:
        vap->iv_bmiss_count_for_reset = (u_int8_t)val;
        break;

    case IEEE80211_BMISS_COUNT_MAX:
        vap->iv_bmiss_count_max = (u_int8_t)val;
        break;

    case IEEE80211_TXPOWER:
        break;

    case IEEE80211_MULTI_DOMAIN:
        if(!ic->ic_country.isMultidomain)
            return -EINVAL;
        
        if (val)
            ic->ic_multiDomainEnabled = 1;
        else
            ic->ic_multiDomainEnabled = 0;
        break;

    case IEEE80211_FEATURE_WMM:
        if (!(ic->ic_caps & IEEE80211_C_WME))
            return -EINVAL;
        
        if (val)
            ieee80211_vap_wme_set(vap);
        else
            ieee80211_vap_wme_clear(vap);
        break;

    case IEEE80211_FEATURE_PRIVACY:
        if (val)
            IEEE80211_VAP_PRIVACY_ENABLE(vap);
        else
            IEEE80211_VAP_PRIVACY_DISABLE(vap);
        break;

    case IEEE80211_FEATURE_WMM_PWRSAVE:
        /*
         * NB: AP WMM power save is a compilation option,
         * and can not be turned on/off at run time.
         */
        if (vap->iv_opmode != IEEE80211_M_STA)
            return -EINVAL;

        if (val)
            ieee80211_set_wmm_power_save(vap, 1);
        else
            ieee80211_set_wmm_power_save(vap, 0);
        break;

    case IEEE80211_FEATURE_UAPSD:
        if (vap->iv_opmode == IEEE80211_M_STA) {
            ieee80211_set_uapsd_flags(vap, (u_int8_t)(val & WME_CAPINFO_UAPSD_ALL) );
            return ieee80211_pwrsave_uapsd_set_max_sp_length(vap, ((val >> WME_CAPINFO_UAPSD_MAXSP_SHIFT) & WME_CAPINFO_UAPSD_MAXSP_MASK));
        }
        else {
            if (IEEE80211_IS_UAPSD_ENABLED(ic)) {
                if (val)
                    IEEE80211_VAP_UAPSD_ENABLE(vap);
                else
                    IEEE80211_VAP_UAPSD_DISABLE(vap);
            }
            else {
                return -EINVAL;
            }
        }
        break;

    case IEEE80211_WPS_MODE:
        vap->iv_wps_mode = (u_int8_t)val;
        break;

    case IEEE80211_NOBRIDGE_MODE:
        if (val)
            IEEE80211_VAP_NOBRIDGE_ENABLE(vap);
        else
            IEEE80211_VAP_NOBRIDGE_DISABLE(vap);
        break;

    case IEEE80211_MIN_BEACON_COUNT:
    case IEEE80211_IDLE_TIME:
        ieee80211_vap_pause_set_param(vaphandle,param,val);
        break;

    case IEEE80211_FEATURE_COUNTER_MEASURES:
        if (val)
            IEEE80211_VAP_COUNTERM_ENABLE(vap);
        else
            IEEE80211_VAP_COUNTERM_DISABLE(vap);
        break;

    case IEEE80211_FEATURE_WDS:
        if (val)
            IEEE80211_VAP_WDS_ENABLE(vap);
        else
            IEEE80211_VAP_WDS_DISABLE(vap);
#ifdef HOST_OFFLOAD
        if(ic->ic_rx_intr_mitigation != NULL)
            ic->ic_rx_intr_mitigation(ic, val); 
#else
        if (vap->iv_opmode == IEEE80211_M_STA)
        {
            if(ic->ic_rx_intr_mitigation != NULL)
                ic->ic_rx_intr_mitigation(ic, val); 
        }
#endif
        ieee80211_update_vap_target(vap);
        break;

#if WDS_VENDOR_EXTENSION
    case IEEE80211_WDS_RX_POLICY:
        vap->iv_wds_rx_policy = val & WDS_POLICY_RX_MASK;
        break;
#endif

    case IEEE80211_FEATURE_VAP_IND:
        if (val)
            ieee80211_vap_vap_ind_set(vap);
        else
            ieee80211_vap_vap_ind_clear(vap);
        break;
    case IEEE80211_FEATURE_HIDE_SSID:
        if (val)
            IEEE80211_VAP_HIDESSID_ENABLE(vap);
        else
            IEEE80211_VAP_HIDESSID_DISABLE(vap);
        break;
    case IEEE80211_FEATURE_PUREG:
        if (val)
            IEEE80211_VAP_PUREG_ENABLE(vap);
        else
            IEEE80211_VAP_PUREG_DISABLE(vap);
        break;
    case IEEE80211_FEATURE_PURE11N:
        if (val)
            IEEE80211_VAP_PURE11N_ENABLE(vap);
        else
            IEEE80211_VAP_PURE11N_DISABLE(vap);
        break;
    case IEEE80211_FEATURE_APBRIDGE:
        if (val == 0)
            IEEE80211_VAP_NOBRIDGE_ENABLE(vap);
        else
            IEEE80211_VAP_NOBRIDGE_DISABLE(vap);
        break;
    case IEEE80211_FEATURE_COPY_BEACON:
        if (val == 0)
            ieee80211_vap_copy_beacon_clear(vap);
        else
            ieee80211_vap_copy_beacon_set(vap);
        break;
    case IEEE80211_FIXED_RATE:
        if (val == IEEE80211_FIXED_RATE_NONE) {
             vap->iv_fixed_rate.mode = IEEE80211_FIXED_RATE_NONE;
             vap->iv_fixed_rateset = IEEE80211_FIXED_RATE_NONE;
             vap->iv_fixed_rate.series = IEEE80211_FIXED_RATE_NONE;
        } else {
             if (val & 0x80) {
             vap->iv_fixed_rate.mode   = IEEE80211_FIXED_RATE_MCS;
             } else {
                 vap->iv_fixed_rate.mode   = IEEE80211_FIXED_RATE_LEGACY;
             }
             vap->iv_fixed_rateset = val;
             vap->iv_fixed_rate.series = val;
        }
        ic->ic_set_config(vap);
        break;
    case IEEE80211_FIXED_RETRIES:
        vap->iv_fixed_retryset = val;
        vap->iv_fixed_rate.retries = val;
        ic->ic_set_config(vap);
        break;
    case IEEE80211_MCAST_RATE:
        vap->iv_mcast_fixedrate = val;
        ieee80211_set_mcast_rate(vap);
        break;
    case IEEE80211_BCAST_RATE:
        vap->iv_bcast_fixedrate = val;
        break;
    case IEEE80211_SHORT_GI:
        if (val) {
            /* Note: Leaving this logic intact for backward compatibility */
            if (ieee80211com_has_htcap(ic, IEEE80211_HTCAP_C_SHORTGI40 | IEEE80211_HTCAP_C_SHORTGI20)) {
                if (ieee80211com_has_htcap(ic, IEEE80211_HTCAP_C_SHORTGI40))
                    ieee80211com_set_htflags(ic, IEEE80211_HTF_SHORTGI40);
                if (ieee80211com_has_htcap(ic, IEEE80211_HTCAP_C_SHORTGI20))
                    ieee80211com_set_htflags(ic, IEEE80211_HTF_SHORTGI20);
                /* With VHT it suffices if we just examine HT */
                vap->iv_sgi = val;
            } else  { 
                return -EINVAL;
            }
        }
        else {
            ieee80211com_clear_htflags(ic, IEEE80211_HTF_SHORTGI40 | IEEE80211_HTF_SHORTGI20);
            vap->iv_sgi = val;
        }
        ic->ic_set_config(vap);
        break;

     case IEEE80211_FEATURE_STAFWD:
     	if (vap->iv_opmode == IEEE80211_M_STA) 
     	{
          if (val == 0)
              ieee80211_vap_sta_fwd_clear(vap);
          else
              ieee80211_vap_sta_fwd_set(vap);
     	}
         else 
         {
             return -EINVAL;
         }
         break;
    case IEEE80211_HT40_INTOLERANT:
        vap->iv_ht40_intolerant = val;
        break;

    case IEEE80211_CHWIDTH:
        if ( val > 3 )
        {
            return -EINVAL;
        }
        vap->iv_chwidth = val;
        break;

    case IEEE80211_CHEXTOFFSET:
	vap->iv_chextoffset = val;
        break;

    case IEEE80211_DISABLE_2040COEXIST:
        if (val) {
            ic->ic_flags |= IEEE80211_F_COEXT_DISABLE;
        }
        else{
            //Resume to the state kept in registry key
            if (ic->ic_reg_parm.disable2040Coexist) {
                ic->ic_flags |= IEEE80211_F_COEXT_DISABLE;
            } else {
                ic->ic_flags &= ~IEEE80211_F_COEXT_DISABLE;
            }
        }
        break;
    case IEEE80211_DISABLE_HTPROTECTION:
        vap->iv_disable_HTProtection = val;
        break;
#ifdef ATH_SUPPORT_QUICK_KICKOUT        
    case IEEE80211_STA_QUICKKICKOUT:
        if(vap->iv_wnm != 1)
           vap->iv_sko_th = val;        
        break;
#endif        
    case IEEE80211_CHSCANINIT:
        vap->iv_chscaninit = val;
        break;
    case IEEE80211_DRIVER_CAPS:
        vap->iv_caps = val;
        break;
    case IEEE80211_FEATURE_COUNTRY_IE:
        if (val)
            /* Enable the Country IE during tx of beacon and ProbeResp. */
            ieee80211_vap_country_ie_set(vap);
        else
            /* Disable the Country IE during tx of beacon and ProbeResp. */
            ieee80211_vap_country_ie_clear(vap);
        break;
    case IEEE80211_FEATURE_IC_COUNTRY_IE:
        if (val) {
            IEEE80211_ENABLE_COUNTRYIE(ic);
        } else {
            IEEE80211_DISABLE_COUNTRYIE(ic);
        }
        break;
    case IEEE80211_FEATURE_DOTH:
        if (val)
            /* Enable the dot h IE's for this VAP. */
            ieee80211_vap_doth_set(vap);
        else
            /* Disable the dot h IE's for this VAP. */
            ieee80211_vap_doth_clear(vap);
        break;

     case  IEEE80211_FEATURE_PSPOLL:       
         retv = wlan_sta_power_set_pspoll(vap, val);
         break;

    case IEEE80211_FEATURE_CONTINUE_PSPOLL_FOR_MOREDATA:
         retv = wlan_sta_power_set_pspoll_moredata_handling(vap, val ? 
										IEEE80211_CONTINUE_PSPOLL_FOR_MORE_DATA :
                                        IEEE80211_WAKEUP_FOR_MORE_DATA);
         break;

#if ATH_SUPPORT_IQUE
	case IEEE80211_ME:
        if (vap->iv_me) {
            if(val == MC_SNOOP_DISABLE_CMD)
            {
                vap->iv_me->mc_snoop_enable = 0;
            }
            else
           {
               vap->iv_me->mc_snoop_enable = 1;
           }
           /*  when snoop is diabled, multicast enhancement will also be diabled*/
            vap->iv_me->mc_mcast_enable = val;
        }
		break;
	case IEEE80211_MEDEBUG:
        if (vap->iv_me) {
		    vap->iv_me->me_debug = val;
        }
		break;
	case IEEE80211_ME_SNOOPLENGTH:
        if (vap->iv_me) {
            vap->iv_me->ieee80211_me_snooplist.msl_max_length = 
                    (val > MAX_SNOOP_ENTRIES)? MAX_SNOOP_ENTRIES : val;
        }
		break;
	case IEEE80211_ME_TIMER:
        if (vap->iv_me) {
		    vap->iv_me->me_timer = val;
        }
		break;
	case IEEE80211_ME_TIMEOUT:
        if (vap->iv_me) {
		    vap->iv_me->me_timeout = val;
        }
		break;
	case IEEE80211_ME_DROPMCAST:
        if (vap->iv_me) {
		    vap->iv_me->mc_discard_mcast = val;
        }
		break;
    case  IEEE80211_ME_CLEARDENY:
        if (vap->iv_ique_ops.me_cleardeny) {
            vap->iv_ique_ops.me_cleardeny(vap);
        }    
        break;
    case IEEE80211_HBR_TIMER:
        if (vap->iv_hbr_list && vap->iv_ique_ops.hbr_settimer) {
            vap->iv_ique_ops.hbr_settimer(vap, val);
        } 
        break;   
#endif /*ATH_SUPPORT_IQUE*/
    case IEEE80211_WEP_MBSSID:
        vap->iv_wep_mbssid = (u_int8_t)val;
        break;
    case IEEE80211_MGMT_RATE:
        vap->iv_mgt_rate = (u_int16_t)val;
        break;
    case IEEE80211_FEATURE_AMPDU:
        if (!val) {
           IEEE80211_DISABLE_AMPDU(ic);
        } else {
           IEEE80211_ENABLE_AMPDU(ic);
        }
        break;
    case IEEE80211_MAX_AMPDU:
        ic->ic_maxampdu = val;
        break;
    case IEEE80211_VHT_MAX_AMPDU:
        ic->ic_vhtcap &= ~IEEE80211_VHTCAP_MAX_AMPDU_LEN_EXP;
        ic->ic_vhtcap |= (val << IEEE80211_VHTCAP_MAX_AMPDU_LEN_EXP_S);
        break;
    case IEEE80211_MIN_FRAMESIZE:
        ic->ic_minframesize = val;
	break;
#if UMAC_SUPPORT_TDLS
    case IEEE80211_TDLS_MACADDR1:
        vap->iv_tdls_macaddr1 = val;
        break;

    case IEEE80211_TDLS_MACADDR2:
        vap->iv_tdls_macaddr2 = val;
        break;

    case IEEE80211_RESMGR_VAP_AIR_TIME_LIMIT:
        retv = ieee80211_resmgr_off_chan_sched_set_air_time_limit(ic->ic_resmgr, vap, val);
        break;

    case IEEE80211_TDLS_ACTION:
        vap->iv_tdls_action = val;
        break;
#endif
    case IEEE80211_UAPSD_MAXSP:
        retv = ieee80211_pwrsave_uapsd_set_max_sp_length(vap,val);
	break;

    case IEEE80211_PROTECTION_MODE:
        vap->iv_protmode = val;
        break;

    case IEEE80211_AUTH_INACT_TIMEOUT:
        vap->iv_inact_auth = (val + IEEE80211_INACT_WAIT-1)/IEEE80211_INACT_WAIT;
        break;

    case IEEE80211_INIT_INACT_TIMEOUT:
        vap->iv_inact_init = (val + IEEE80211_INACT_WAIT-1)/IEEE80211_INACT_WAIT;
        break;

    case IEEE80211_RUN_INACT_TIMEOUT:
        /* Checking if vap is on offload radio or not */
        if (!wlan_get_HWcapabilities(ic,IEEE80211_CAP_PERF_PWR_OFLD)) 
        {
            if (val <= IEEE80211_RUN_INACT_TIMEOUT_THRESHOLD) {
                vap->iv_inact_run = (val + IEEE80211_INACT_WAIT-1)/IEEE80211_INACT_WAIT;
            }
            else {
                printk("\nMaximum value allowed is : %d", IEEE80211_RUN_INACT_TIMEOUT_THRESHOLD);
            }
        }
        break;

    case IEEE80211_PROBE_INACT_TIMEOUT:
        vap->iv_inact_probe = (val + IEEE80211_INACT_WAIT-1)/IEEE80211_INACT_WAIT;
        break;

    case IEEE80211_QBSS_LOAD:
         if (val == 0) {
            ieee80211_vap_bssload_clear(vap);
         } else {
            ieee80211_vap_bssload_set(vap);
         }
         break;

#if UMAC_SUPPORT_CHANUTIL_MEASUREMENT
    case IEEE80211_CHAN_UTIL_ENAB:
         vap->iv_chanutil_enab = val;
         break;
#endif /* UMAC_SUPPORT_CHANUTIL_MEASUREMENT */
    case IEEE80211_RRM_CAP:
         if (val == 0) {
            ieee80211_vap_rrm_clear(vap);
         } else {
            ieee80211_vap_rrm_set(vap);
         }
         break;
    case IEEE80211_RRM_DEBUG:
         ieee80211_rrmdbg_set(vap, val);
         break;
    case IEEE80211_RRM_STATS:
         ieee80211_set_rrmstats(vap,val);
         break;
    case IEEE80211_RRM_SLWINDOW:
         ieee80211_rrm_set_slwindow(vap,val);
         break; 
#if UMAC_SUPPORT_WNM
    case IEEE80211_WNM_CAP:
         if((val == 1) && (ieee80211_ic_wnm_is_set(ic) == 0)) {
              return -EINVAL;
         }
         if (val == 0) {
            ieee80211_vap_wnm_clear(vap);
         } else {
            ieee80211_vap_wnm_set(vap);
         }
         break;
    case IEEE80211_WNM_BSS_CAP:
         if(val == 1 && (ieee80211_vap_wnm_is_set(vap) == 0)) {
              return -EINVAL;
         }
         if(ieee80211_wnm_bss_is_set(vap->wnm) == val) {
              return -EINVAL;
         }
         if (val == 0) {
            ic->ic_wnm_bss_count--;
            ieee80211_wnm_bss_clear(vap->wnm);
            if(ic->ic_wnm_bss_count == 0 && ic->ic_wnm_bss_active == 1) {
                if(ic->ic_opmode != IEEE80211_M_STA) {
                    OS_CANCEL_TIMER(&ic->ic_bssload_timer);
                }
                ic->ic_wnm_bss_active = 0;
            }
         } else {
            ieee80211_wnm_bss_set(vap->wnm);
            ic->ic_wnm_bss_count++;
            if(ic->ic_wnm_bss_active == 0) {
                if(ic->ic_opmode != IEEE80211_M_STA) {
                    OS_SET_TIMER(&ic->ic_bssload_timer, IEEE80211_BSSLOAD_WAIT * 1000);
                }   
                ic->ic_wnm_bss_active = 1;
            }
         }
         break;
    case IEEE80211_WNM_TFS_CAP:
         if(val == 1 && (ieee80211_vap_wnm_is_set(vap) == 0)) {
              return -EINVAL;
         }
         if (val == 0) {
            ieee80211_wnm_tfs_clear(vap->wnm);
         } else {
            ieee80211_wnm_tfs_set(vap->wnm);
         }
         break;
    case IEEE80211_WNM_TIM_CAP:
         if(val == 1 && (ieee80211_vap_wnm_is_set(vap) == 0)) {
              return -EINVAL;
         }
         if (val == 0) {
            ieee80211_wnm_tim_clear(vap->wnm);
         } else {
            ieee80211_wnm_tim_set(vap->wnm);
         }
         break;
    case IEEE80211_WNM_SLEEP_CAP:
         if(val == 1 && (ieee80211_vap_wnm_is_set(vap) == 0)) {
              return -EINVAL;
         }
         if (val == 0) {
            ieee80211_wnm_sleep_clear(vap->wnm);
         } else {
            ieee80211_wnm_sleep_set(vap->wnm);
         }
         break;
    case IEEE80211_WNM_FMS_CAP:
         if(val == 1 && (ieee80211_vap_wnm_is_set(vap) == 0)) {
             return -EINVAL;
         }
         if(ieee80211_wnm_fms_is_set(vap->wnm) == val) {
             return -EINVAL;
         }
         if (val == 0) {
             ieee80211_wnm_fms_clear(vap->wnm);
         } else {
             ieee80211_wnm_fms_set(vap->wnm);
         }
         break;
#endif
    case IEEE80211_AP_REJECT_DFS_CHAN:
        if (val == 0)
            ieee80211_vap_ap_reject_dfs_chan_clear(vap);
        else
            ieee80211_vap_ap_reject_dfs_chan_set(vap);
        break;
	case IEEE80211_WDS_AUTODETECT:
        if (!val) {
           IEEE80211_VAP_WDS_AUTODETECT_DISABLE(vap);
        } else {
           IEEE80211_VAP_WDS_AUTODETECT_ENABLE(vap);
        }
        break;
	case IEEE80211_WEP_TKIP_HT:
		if (!val) {
           ieee80211_ic_wep_tkip_htrate_clear(ic);
        } else {
           ieee80211_ic_wep_tkip_htrate_set(ic);
        }
        break;
	case IEEE80211_IGNORE_11DBEACON:
        if (!val) {
           IEEE80211_DISABLE_IGNORE_11D_BEACON(ic);
        } else {
           IEEE80211_ENABLE_IGNORE_11D_BEACON(ic);
        }
        break;

    case IEEE80211_FEATURE_MFP_TEST:
        if (!val) {
            ieee80211_vap_mfp_test_clear(vap);
        } else {
            ieee80211_vap_mfp_test_set(vap);
        }
        break;

    case IEEE80211_TRIGGER_MLME_RESP:
         if (val == 0) {
            ieee80211_vap_trigger_mlme_resp_clear(vap);
         } else {
            ieee80211_vap_trigger_mlme_resp_set(vap);
         }
         break;

#ifdef ATH_SUPPORT_TxBF
    case IEEE80211_TXBF_AUTO_CVUPDATE:
        vap->iv_autocvupdate = val;
        break;
    case IEEE80211_TXBF_CVUPDATE_PER:
        vap->iv_cvupdateper = val;
        break;
#endif
    case IEEE80211_SMARTNET:
        if (val) {
            ieee80211_vap_smartnet_enable_set(vap);
        }else {
            ieee80211_vap_smartnet_enable_clear(vap);
        }
        break;
    case IEEE80211_WEATHER_RADAR:
            ic->ic_no_weather_radar_chan = !!val;
        break;
    case IEEE80211_WEP_KEYCACHE:
            vap->iv_wep_keycache = !!val;
            break;
#if ATH_SUPPORT_WPA_SUPPLICANT_CHECK_TIME           
    case IEEE80211_REJOINT_ATTEMP_TIME:
            vap->iv_rejoint_attemp_time = val;
            break;
#endif
    case IEEE80211_SEND_DEAUTH:
            vap->iv_send_deauth = val; 
            break;
#if UMAC_SUPPORT_PROXY_ARP
    case IEEE80211_PROXYARP_CAP:
         if (val == 0) {
            ieee80211_vap_proxyarp_clear(vap);
         } else {
            ieee80211_vap_proxyarp_set(vap);
         }
         break;
#if UMAC_SUPPORT_DGAF_DISABLE
    case IEEE80211_DGAF_DISABLE:
         if (val == 0) {
             ieee80211_vap_dgaf_disable_clear(vap);
         } else {
             ieee80211_vap_dgaf_disable_set(vap);
         }
         break;
#endif
#endif
    case IEEE80211_FEATURE_OFF_CHANNEL_SUPPORT:   
         if (val == 0) {
             /* Disable the off-channel IE's for this VAP. */
             ieee80211_vap_off_channel_support_clear(vap);
         }
         else {
             if (ieee80211_ic_off_channel_support_is_set(ic)) {
                 /* Enable the off-channel IE's for this VAP. */
                 ieee80211_vap_off_channel_support_set(vap);
             }
             else {
                  printk("%s: Cannot enable off-channel support IC support=0\n",
                                  __func__);
             }
         }
         break;
#if ATH_SUPPORT_SIMPLE_CONFIG_EXT
    case IEEE80211_NOPBN:
         if (val == 0) {
             ieee80211_vap_nopbn_clear(vap);
         } else {
             ieee80211_vap_nopbn_set(vap);
         }
        break;
#endif

    case IEEE80211_FIXED_VHT_MCS:
         vap->iv_fixed_rate.mode   = IEEE80211_FIXED_RATE_NONE;
         if (val > 9) {
            /* Treat this as disabling fixed rate */
            return EOK;
         }
         vap->iv_fixed_rate.mode   = IEEE80211_FIXED_RATE_VHT;
         vap->iv_vht_fixed_mcs = val;
    break;

    case IEEE80211_FIXED_NSS:
         if (val > 3) {
            return -EINVAL;
         }
         vap->iv_nss = val;
    break;


    case IEEE80211_SUPPORT_LDPC:
        switch (val) {
            case 0: 
                vap->iv_ldpc = val;
            break;

            case 1:
                /* With VHT it suffices if we just examine HT */
                if (ieee80211com_has_htcap(ic, IEEE80211_HTCAP_C_ADVCODING)) {
                    vap->iv_ldpc = val;
                } else {
                    return -EINVAL;
                }
            break;

            default:
                return -EINVAL;
            break;
        }
    break;


    case IEEE80211_SUPPORT_TX_STBC:
        switch (val) {
            case 0: 
                vap->iv_tx_stbc = val;
            break;

            case 1:
                /* With VHT it suffices if we just examine HT */
                if (ieee80211com_has_htcap(ic, IEEE80211_HTCAP_C_TXSTBC)) {
                    vap->iv_tx_stbc = val;
                } else { 
                    return -EINVAL;
                }
            break;

            default:
                return -EINVAL;
            break;
        }
    break;

    case IEEE80211_SUPPORT_RX_STBC:
        switch (val) {
            case 0: 
                vap->iv_rx_stbc = val;
            break;

            case 1:
            case 2:
            case 3:
                if ((ic->ic_htcap & IEEE80211_HTCAP_C_RXSTBC)  ||
                    (ic->ic_vhtcap & IEEE80211_VHTCAP_RX_STBC)){
                    vap->iv_rx_stbc = val;
                } else { 
                    return -EINVAL;
                }
            break;

            default:
                return -EINVAL;
            break;
        }
    break;

    case IEEE80211_OPMODE_NOTIFY_ENABLE:
        /* TODO: This Op Mode Tx feature is not mandatory. 
         * For now we will use this ioctl to trigger op mode notification 
         * At a later date we will use the notify varibale to enable/disable
         * opmode notification dynamically (@runtime)
         */
        if (val) {
            struct ieee80211_node   *ni = NULL;
            struct ieee80211_action_mgt_args actionargs;
            if (ieee80211vap_get_opmode(vap) == IEEE80211_M_HOSTAP) {
                /* create temporary node for broadcast */
                ni = ieee80211_tmp_node(vap, IEEE80211_GET_BCAST_ADDR(vap->iv_ic));
            } else {
                ni = vap->iv_bss;
            }

            if (ni != NULL) {
                actionargs.category = IEEE80211_ACTION_CAT_VHT;
                actionargs.action   = IEEE80211_ACTION_VHT_OPMODE;
                actionargs.arg1     = 0;
                actionargs.arg2     = 0;
                actionargs.arg3     = 0;
                ieee80211_send_action(ni, &actionargs, NULL);

                if (ieee80211vap_get_opmode(vap) == IEEE80211_M_HOSTAP) {
                    /* temporary node - decrement reference count so that the node will be 
                     * automatically freed upon completion */
                    ieee80211_free_node(ni);
                }
            }
        }
        vap->iv_opmode_notify = val;
    break;

    case IEEE80211_ENABLE_RTSCTS:
         if (((val & (~0xff)) != 0) ||
             (((val & 0x0f) == 0) && (((val & 0xf0) >> 4) != 0)) ||
             (((val & 0x0f) != 0) && (((val & 0xf0) >> 4) == 0)) ) {
             printk("%s: Invalid value for RTS-CTS: %x\n", __func__, val);
             return -EINVAL;
         }

         if (((val & 0x0f) > 2) || (((val & 0xf0) >> 4) > 2)) {
             printk("%s: Not yet supported value for RTS-CTS: %x\n",
                     __func__, val);
             return -EINVAL;
         }

         vap->iv_rtscts_enabled = val;
    break;

    case IEEE80211_RC_NUM_RETRIES:
        if (val & 0x1 == 1) { /* do not support odd numbers */
            return -EINVAL;
        }

        if (val == 0) {
            val = 2; /* Default case: use 2 retries - one for each rate-series. */
        }

        vap->iv_rc_num_retries = val;
    break;

    case IEEE80211_VHT_MCSMAP:
         if (isvalid_vht_mcsmap(val)) {
             vap->iv_vht_mcsmap = val;
         } else {
             return -EINVAL;
         }
    break;

    case IEEE80211_MAX_SCANENTRY:
        ieee80211_scan_set_maxentry(ieee80211_vap_get_scan_table(vap), val);
        break;
    case IEEE80211_SCANENTRY_TIMEOUT:
        ieee80211_scan_set_timeout(ieee80211_vap_get_scan_table(vap), val);
        break;
    default:
        break;
    }

    if (ic->ic_vap_set_param) {
        ic->ic_vap_set_param(vap,param, val);
    }
    return retv;
}

u_int32_t
wlan_get_param(wlan_if_t vaphandle, ieee80211_param param)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    u_int32_t val = 0;
    
    switch (param) {
    case IEEE80211_AUTO_ASSOC:
        val = IEEE80211_VAP_IS_AUTOASSOC_ENABLED(vap) ? 1 : 0;
        break;
    case IEEE80211_SAFE_MODE:
        val = IEEE80211_VAP_IS_SAFEMODE_ENABLED(vap) ? 1 : 0;
        break;
        
    case IEEE80211_SEND_80211:
        val = IEEE80211_VAP_IS_SEND_80211_ENABLED(vap) ? 1 : 0;
        break;

    case IEEE80211_RECEIVE_80211:
        val = IEEE80211_VAP_IS_DELIVER_80211_ENABLED(vap) ? 1 : 0;
        break;

    case IEEE80211_FEATURE_DROP_UNENC:
        val = IEEE80211_VAP_IS_DROP_UNENC(vap) ? 1 : 0;
        break;
    
#ifdef ATH_COALESCING
    case IEEE80211_FEATURE_TX_COALESCING:
        val = ic->ic_tx_coalescing;
        break;
#endif
    case IEEE80211_SHORT_PREAMBLE:
        val = IEEE80211_IS_CAP_SHPREAMBLE_ENABLED(ic) ? 1 : 0;
        break;
    
    case IEEE80211_SHORT_SLOT:
        val = IEEE80211_IS_SHSLOT_ENABLED(ic) ? 1 : 0;
        break;
    
    case IEEE80211_RTS_THRESHOLD:
        val = vap->iv_rtsthreshold;
        break;

    case IEEE80211_FRAG_THRESHOLD:
        val = vap->iv_fragthreshold;
        break;

    case IEEE80211_BEACON_INTVAL:
        if (vap->iv_opmode == IEEE80211_M_STA) {
            val = vap->iv_bss->ni_intval;
        } else {
            val = ic->ic_intval;
        }
        break;

#if ATH_SUPPORT_AP_WDS_COMBO
    case IEEE80211_NO_BEACON:
        val = vap->iv_no_beacon;
        break;
#endif

    case IEEE80211_LISTEN_INTVAL:
        val = ic->ic_lintval;
        break;

    case IEEE80211_DTIM_INTVAL:
        val = vap->iv_dtim_period;
        break;

    case IEEE80211_BMISS_COUNT_RESET:
        val = vap->iv_bmiss_count_for_reset ;
        break;

    case IEEE80211_BMISS_COUNT_MAX:
        val = vap->iv_bmiss_count_max;
        break;

    case IEEE80211_ATIM_WINDOW:
        val = vap->iv_atim_window;
        break;

    case IEEE80211_TXPOWER:
        /* 
         * here we'd better return ni_txpower for it's more accurate to
         * current txpower and it must be less than or equal to
         * ic_txpowlimit/ic_curchanmaxpwr, and it's in 0.5 dbm.
         * This value is updated when channel is changed or setTxPowerLimit
         * is called.
         */
        val = vap->iv_bss->ni_txpower;
        break;

    case IEEE80211_MULTI_DOMAIN:
        val = ic->ic_multiDomainEnabled;
        break;

    case IEEE80211_FEATURE_WMM:
        val = ieee80211_vap_wme_is_set(vap) ? 1 : 0;
        break;

    case IEEE80211_FEATURE_PRIVACY:
        val =  IEEE80211_VAP_IS_PRIVACY_ENABLED(vap) ? 1 : 0;
        break;

    case IEEE80211_FEATURE_WMM_PWRSAVE:
        val = ieee80211_get_wmm_power_save(vap) ? 1 : 0;
        break;

    case IEEE80211_FEATURE_UAPSD:
        if (vap->iv_opmode == IEEE80211_M_STA) {
            val = ieee80211_get_uapsd_flags(vap);
        }
        else {
            val = IEEE80211_VAP_IS_UAPSD_ENABLED(vap) ? 1 : 0;
        }
        break;
    case IEEE80211_FEATURE_IC_COUNTRY_IE:
	val = (IEEE80211_IS_COUNTRYIE_ENABLED(ic) != 0);
        break;
    case IEEE80211_PERSTA_KEYTABLE_SIZE:
        /*
         * XXX: We should return the number of key tables (each table has 4 key slots),
         * not the actual number of key slots. Use the node hash table size as an estimation
         * of max supported ad-hoc stations.
         */
        val = IEEE80211_NODE_HASHSIZE;
        break;

    case IEEE80211_WPS_MODE:
        val = vap->iv_wps_mode;
        break;

    case IEEE80211_MIN_BEACON_COUNT:
    case IEEE80211_IDLE_TIME:
        val = ieee80211_vap_pause_get_param(vaphandle,param);
        break;
    case IEEE80211_FEATURE_COUNTER_MEASURES:
        val = IEEE80211_VAP_IS_COUNTERM_ENABLED(vap) ? 1 : 0;
        break;
    case IEEE80211_FEATURE_WDS:
        val = IEEE80211_VAP_IS_WDS_ENABLED(vap) ? 1 : 0;
#ifdef ATH_EXT_AP
        if (val == 0) {
	    val = IEEE80211_VAP_IS_EXT_AP_ENABLED(vap) ? 1 : 0; 
        }
#endif
        break;
#if WDS_VENDOR_EXTENSION
    case IEEE80211_WDS_RX_POLICY:
        val = vap->iv_wds_rx_policy;
        break;
#endif
    case IEEE80211_FEATURE_VAP_IND:
        val = ieee80211_vap_vap_ind_is_set(vap) ? 1 : 0;
        break;
    case IEEE80211_FEATURE_HIDE_SSID:
        val = IEEE80211_VAP_IS_HIDESSID_ENABLED(vap) ? 1 : 0;
        break;
    case IEEE80211_FEATURE_PUREG:
        val = IEEE80211_VAP_IS_PUREG_ENABLED(vap) ? 1 : 0;
        break;
    case IEEE80211_FEATURE_PURE11N:
        val = IEEE80211_VAP_IS_PURE11N_ENABLED(vap) ? 1 : 0;
        break;
    case IEEE80211_FEATURE_APBRIDGE:
        val = IEEE80211_VAP_IS_NOBRIDGE_ENABLED(vap) ? 0 : 1;
        break;
     case  IEEE80211_FEATURE_PSPOLL:       
         val = wlan_sta_power_get_pspoll(vap);
         break;

    case IEEE80211_FEATURE_CONTINUE_PSPOLL_FOR_MOREDATA:
         if (wlan_sta_power_get_pspoll_moredata_handling(vap) == 
             IEEE80211_CONTINUE_PSPOLL_FOR_MORE_DATA ) {
             val = true;
         } else {
             val = false;
         }
         break;
    case IEEE80211_MCAST_RATE:
        val = vap->iv_mcast_fixedrate;
        break;
    case IEEE80211_BCAST_RATE:
        val = vap->iv_bcast_fixedrate;
        break;
    case IEEE80211_HT40_INTOLERANT:
        val = vap->iv_ht40_intolerant;
        break;
    case IEEE80211_MAX_AMPDU:
        val = ic->ic_maxampdu;
        break;
    case IEEE80211_VHT_MAX_AMPDU:
        val = ((ic->ic_vhtcap & IEEE80211_VHTCAP_MAX_AMPDU_LEN_EXP) >> 
                IEEE80211_VHTCAP_MAX_AMPDU_LEN_EXP_S);
        break;
    case IEEE80211_CHWIDTH:
        /*
        ** If the VAP parameter for chwidth is set, use that value, else 
        ** return the chwidth based on current channel characteristics.
        */
        if (vap->iv_chwidth != IEEE80211_CWM_WIDTHINVALID) {
            val = vap->iv_chwidth;
        } else {
            val = ic->ic_cwm_get_width(ic);
        }
        break;
    case IEEE80211_CHEXTOFFSET:
        /*
        ** Extension channel is set through the channel mode selected by AP.  When configured
        ** through this interface, it's stored in the ic.
        */
        val = ic->ic_cwm_get_extoffset(ic);
        break;
    case IEEE80211_DISABLE_HTPROTECTION:
        val = vap->iv_disable_HTProtection;
        break;
#ifdef ATH_SUPPORT_QUICK_KICKOUT        
    case IEEE80211_STA_QUICKKICKOUT:
        val = vap->iv_sko_th;
        break;
#endif        
    case IEEE80211_CHSCANINIT:
        val = vap->iv_chscaninit;
        break;
    case IEEE80211_FEATURE_STAFWD:
        val = ieee80211_vap_sta_fwd_is_set(vap) ? 1 : 0;
        break;
    case IEEE80211_DRIVER_CAPS:
        val = vap->iv_caps;
        break;
    case IEEE80211_FEATURE_COUNTRY_IE:
        val = ieee80211_vap_country_ie_is_set(vap) ? 1 : 0;
        break;
    case IEEE80211_FEATURE_DOTH:
        val = ieee80211_vap_doth_is_set(vap) ? 1 : 0;
        break;
#if ATH_SUPPORT_IQUE
    case IEEE80211_IQUE_CONFIG:
        ic->ic_get_iqueconfig(ic);
        break;
	case IEEE80211_ME:
        if (vap->iv_me) {
            val = vap->iv_me->mc_mcast_enable;
        }
		break;
	case IEEE80211_MEDUMP:
	    val = 0;
        if (vap->iv_ique_ops.me_dump) {
            vap->iv_ique_ops.me_dump(vap);
        }
		break;
	case IEEE80211_MEDEBUG:
        if (vap->iv_me) {
		    val = vap->iv_me->me_debug;
        }
		break;
	case IEEE80211_ME_SNOOPLENGTH:
        if (vap->iv_me) {
    		val = vap->iv_me->ieee80211_me_snooplist.msl_max_length;
        }
		break;
	case IEEE80211_ME_TIMER:
        if (vap->iv_me) {
		    val = vap->iv_me->me_timer;
        }
		break;
	case IEEE80211_ME_TIMEOUT:
        if (vap->iv_me) {
		    val = vap->iv_me->me_timeout;
        }
		break;
	case IEEE80211_ME_DROPMCAST:
        if (vap->iv_me) {
		    val = vap->iv_me->mc_discard_mcast;
        }
		break;
    case IEEE80211_ME_SHOWDENY:
        if (vap->iv_ique_ops.me_showdeny) {
            vap->iv_ique_ops.me_showdeny(vap);
        }    
        break;
#if 0        
    case IEEE80211_HBR_TIMER:
        if (vap->iv_hbr_list) {
            val = vap->iv_hbr_list->hbr_timeout;
        }    
        break;
#endif
#endif        
    case IEEE80211_QBSS_LOAD:
        val = ieee80211_vap_bssload_is_set(vap);
        break;
#if UMAC_SUPPORT_CHANUTIL_MEASUREMENT
    case IEEE80211_CHAN_UTIL_ENAB:
        val = vap->iv_chanutil_enab;
        break;
    case IEEE80211_CHAN_UTIL:
        val = vap->chanutil_info.value;
        break;
#endif /* UMAC_SUPPORT_CHANUTIL_MEASUREMENT */
    case IEEE80211_RRM_CAP:
        val = ieee80211_vap_rrm_is_set(vap);
        break;
    case IEEE80211_RRM_DEBUG:
        val = ieee80211_rrmdbg_get(vap);
        break;
    case IEEE80211_RRM_SLWINDOW:
        val = ieee80211_rrm_get_slwindow(vap);
        break;
    case IEEE80211_RRM_STATS:
        val = ieee80211_get_rrmstats(vap);
        break;
#if UMAC_SUPPORT_WNM
    case IEEE80211_WNM_CAP:
        val = ieee80211_vap_wnm_is_set(vap);
        break;
    case IEEE80211_WNM_BSS_CAP:
        val = ieee80211_wnm_bss_is_set(vap->wnm);
        break;
    case IEEE80211_WNM_TFS_CAP:
        val = ieee80211_wnm_tfs_is_set(vap->wnm);
        break;
    case IEEE80211_WNM_TIM_CAP:
        val = ieee80211_wnm_tim_is_set(vap->wnm);
        break;
    case IEEE80211_WNM_SLEEP_CAP:
        val = ieee80211_wnm_sleep_is_set(vap->wnm);
        break;
    case IEEE80211_WNM_FMS_CAP:
        val = ieee80211_wnm_fms_is_set(vap->wnm);
        break;
#endif
    case IEEE80211_SHORT_GI:
        val = vap->iv_sgi;
        break;
    case IEEE80211_FIXED_RATE:
        val = vap->iv_fixed_rateset;
        break;
    case IEEE80211_FIXED_RETRIES:
        val = vap->iv_fixed_retryset;
        break;
    case IEEE80211_WEP_MBSSID:
        val = vap->iv_wep_mbssid;
        break;
    case IEEE80211_MGMT_RATE:
        val = vap->iv_mgt_rate;
        break;
        
    case IEEE80211_MIN_FRAMESIZE:
        val = ic->ic_minframesize;
        break;
    case IEEE80211_RESMGR_VAP_AIR_TIME_LIMIT:
        val = ieee80211_resmgr_off_chan_sched_get_air_time_limit(ic->ic_resmgr, vap);
        break;
    case IEEE80211_PROTECTION_MODE:
        val = vap->iv_protmode;
		break;

#if UMAC_SUPPORT_TDLS
    case IEEE80211_TDLS_MACADDR1:
        val = vap->iv_tdls_macaddr1;
        break;

    case IEEE80211_TDLS_MACADDR2:
        val = vap->iv_tdls_macaddr2;
        break;

    case IEEE80211_TDLS_ACTION:
        val = vap->iv_tdls_action;
        break;
#endif
	case IEEE80211_ABOLT:
        /*
        * Map capability bits to abolt settings.
        */
        val = 0;
        if (vap->iv_ath_cap & IEEE80211_ATHC_COMP)
            val |= IEEE80211_ABOLT_COMPRESSION;
        if (vap->iv_ath_cap & IEEE80211_ATHC_FF)
            val |= IEEE80211_ABOLT_FAST_FRAME;
        if (vap->iv_ath_cap & IEEE80211_ATHC_XR)
            val |= IEEE80211_ABOLT_XR;
        if (vap->iv_ath_cap & IEEE80211_ATHC_BURST)
            val |= IEEE80211_ABOLT_BURST;
        if (vap->iv_ath_cap & IEEE80211_ATHC_TURBOP)
            val |= IEEE80211_ABOLT_TURBO_PRIME;
        if (vap->iv_ath_cap & IEEE80211_ATHC_AR)
            val |= IEEE80211_ABOLT_AR;
        if (vap->iv_ath_cap & IEEE80211_ATHC_WME)
            val |= IEEE80211_ABOLT_WME_ELE;
		break;
    case IEEE80211_COMP:
        val = (vap->iv_ath_cap & IEEE80211_ATHC_COMP) != 0;
        break;
    case IEEE80211_FF:
        val = (vap->iv_ath_cap & IEEE80211_ATHC_FF) != 0;
        break;
    case IEEE80211_TURBO:
        val = (vap->iv_ath_cap & IEEE80211_ATHC_TURBOP) != 0;
        break;
    case IEEE80211_BURST:
        val = (vap->iv_ath_cap & IEEE80211_ATHC_BURST) != 0;
        break;
    case IEEE80211_AR:
        val = (vap->iv_ath_cap & IEEE80211_ATHC_AR) != 0;
        break;
	case IEEE80211_SLEEP:
		val = vap->iv_bss->ni_flags & IEEE80211_NODE_PWR_MGT;
		break;
	case IEEE80211_EOSPDROP:
		val = IEEE80211_VAP_IS_EOSPDROP_ENABLED(vap) != 0;
		break;
	case IEEE80211_MARKDFS:
        val = (ic->ic_flags_ext & IEEE80211_FEXT_MARKDFS) != 0;
		break;
	case IEEE80211_WDS_AUTODETECT:
        val = IEEE80211_VAP_IS_WDS_AUTODETECT_ENABLED(vap) != 0;
		break;		
	case IEEE80211_WEP_TKIP_HT:
		val = ieee80211_ic_wep_tkip_htrate_is_set(ic);
		break;
	case IEEE80211_ATH_RADIO:
        /*
        ** Extract the radio name from the ATH device object
        */
        //printk("IC Name: %s\n",ic->ic_osdev->name);
        //val = ic->ic_dev->name[4] - 0x30;
		break;
	case IEEE80211_IGNORE_11DBEACON:
		val = IEEE80211_IS_11D_BEACON_IGNORED(ic) != 0;
		break;
        case IEEE80211_FEATURE_MFP_TEST:
            val = ieee80211_vap_mfp_test_is_set(vap) ? 1 : 0;
            break;
    case IEEE80211_TRIGGER_MLME_RESP:
        val = ieee80211_vap_trigger_mlme_resp_is_set(vap);
        break;
    case IEEE80211_AUTH_INACT_TIMEOUT:
        val = vap->iv_inact_auth * IEEE80211_INACT_WAIT;
        break;

    case IEEE80211_INIT_INACT_TIMEOUT:
        val = vap->iv_inact_init * IEEE80211_INACT_WAIT;
        break;

    case IEEE80211_RUN_INACT_TIMEOUT:
        if(wlan_get_HWcapabilities(ic,IEEE80211_CAP_PERF_PWR_OFLD)) {
                val = vap->iv_inact_run;
        }
        else {
                val = vap->iv_inact_run * IEEE80211_INACT_WAIT;
        }
        break;

    case IEEE80211_PROBE_INACT_TIMEOUT:
        val = vap->iv_inact_probe * IEEE80211_INACT_WAIT;
        break;
#ifdef ATH_SUPPORT_TxBF
    case IEEE80211_TXBF_AUTO_CVUPDATE:
        val = vap->iv_autocvupdate;
        break;
    case IEEE80211_TXBF_CVUPDATE_PER:
        val = vap->iv_cvupdateper;
        break;
#endif
    case IEEE80211_WEATHER_RADAR:
        val = ic->ic_no_weather_radar_chan;
        break;

    case IEEE80211_WEP_KEYCACHE:
        val = vap->iv_wep_keycache;
        break;
    case IEEE80211_SMARTNET:
        val = ieee80211_vap_smartnet_enable_is_set(vap) ? 1 : 0;
        break;
#if ATH_SUPPORT_WPA_SUPPLICANT_CHECK_TIME
   case IEEE80211_REJOINT_ATTEMP_TIME:
        val = vap->iv_rejoint_attemp_time;
        break;
#endif
   case IEEE80211_SEND_DEAUTH:
        val = vap->iv_send_deauth; 
        break;
#if UMAC_SUPPORT_PROXY_ARP
    case IEEE80211_PROXYARP_CAP:
        val = ieee80211_vap_proxyarp_is_set(vap);
        break;
#if UMAC_SUPPORT_DGAF_DISABLE
    case IEEE80211_DGAF_DISABLE:
        val = ieee80211_vap_dgaf_disable_is_set(vap);
        break;
#endif
#endif
#if ATH_SUPPORT_SIMPLE_CONFIG_EXT
    case IEEE80211_NOPBN:
        val = ieee80211_vap_nopbn_is_set(vap);
        break;
#endif
    case IEEE80211_FEATURE_OFF_CHANNEL_SUPPORT:
        val = ieee80211_vap_off_channel_support_is_set(vap) ? 1 : 0;
        break;

    case IEEE80211_FIXED_VHT_MCS:
         if (vap->iv_fixed_rate.mode  == IEEE80211_FIXED_RATE_VHT) {
             val = vap->iv_vht_fixed_mcs;
         }
    break;

    case IEEE80211_FIXED_NSS:
         val = vap->iv_nss;
    break;

    case IEEE80211_SUPPORT_LDPC:
         val = vap->iv_ldpc;
    break;

    case IEEE80211_SUPPORT_TX_STBC:
         val = vap->iv_tx_stbc;
    break;

    case IEEE80211_SUPPORT_RX_STBC:
         val = vap->iv_rx_stbc;
    break;

    case IEEE80211_OPMODE_NOTIFY_ENABLE:
         val = vap->iv_opmode_notify;
    break;

    case IEEE80211_ENABLE_RTSCTS:
         val = vap->iv_rtscts_enabled;
    break;

    case IEEE80211_RC_NUM_RETRIES:
        val = vap->iv_rc_num_retries;
    break;

    case IEEE80211_VHT_MCSMAP:
         val = vap->iv_vht_mcsmap;
    break;

    case IEEE80211_MAX_SCANENTRY:
        val = ieee80211_scan_get_maxentry(ieee80211_vap_get_scan_table(vap));
        break;
    case IEEE80211_SCANENTRY_TIMEOUT:
        val = ieee80211_scan_get_timeout(ieee80211_vap_get_scan_table(vap));
        break;

    case IEEE80211_GET_ACS_STATE:
        val = wlan_autoselect_in_progress(vap);
        break;

    case IEEE80211_GET_CAC_STATE:
        if(vap->iv_state_info.iv_state == IEEE80211_S_DFS_WAIT)
            val = 1;
        else
            val = 0;
        break;

    default:
        break;
    }

    return val;
}

int wlan_set_debug_flags(wlan_if_t vaphandle, u_int64_t val)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    int category, debug_any = 0;

    for (category = 0; category < IEEE80211_MSG_MAX; category++) {
        int category_mask = (val >> category) & 0x1;
        asf_print_mask_set(&vap->iv_print, category, category_mask);
        asf_print_mask_set(&ic->ic_print, category, category_mask);
        debug_any = category_mask ? 1 : debug_any;
    }
    /* Update the IEEE80211_MSG_ANY debug mask bit */
    asf_print_mask_set(&vap->iv_print, IEEE80211_MSG_ANY, debug_any);
    asf_print_mask_set(&ic->ic_print, IEEE80211_MSG_ANY, debug_any);
    return 0;
}

u_int64_t wlan_get_debug_flags(wlan_if_t vaphandle)
{
    struct ieee80211vap *vap = vaphandle;
    u_int64_t res = 0;
    int byte_s, total_bytes = sizeof(res);
    u_int8_t *iv_print_ptr = &vap->iv_print.category_mask[0];

    for (byte_s = 0; byte_s < total_bytes; byte_s++) {
        res |= ((u_int64_t)iv_print_ptr[byte_s]) << (byte_s * 8);
    }
    return res;
}

int wlan_get_chanlist(wlan_if_t vaphandle, u_int8_t *chanlist)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;

    memcpy(chanlist, ic->ic_chan_active, sizeof(ic->ic_chan_active));
    return 0;
}


int wlan_get_chaninfo(wlan_if_t vaphandle, struct ieee80211_channel *chan, int *nchan)
{
#define IS_NEW_CHANNEL(c)		\
	((IEEE80211_IS_CHAN_5GHZ((c)) && ((c)->ic_freq > 5000) && isclr(reported_a, (c)->ic_ieee)) || \
	 (IEEE80211_IS_CHAN_5GHZ((c)) && ((c)->ic_freq < 5000) && isclr(reported_49Ghz, (c)->ic_ieee)) || \
	 (IEEE80211_IS_CHAN_2GHZ((c)) && isclr(reported_bg, (c)->ic_ieee)))

    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    u_int8_t reported_a[IEEE80211_CHAN_BYTES];	
    u_int8_t reported_bg[IEEE80211_CHAN_BYTES];	
    u_int8_t reported_49Ghz[IEEE80211_CHAN_BYTES];
    int i, j;

    memset(chan, 0, sizeof(struct ieee80211_channel)*IEEE80211_CHAN_MAX);
    *nchan = 0;
    memset(reported_a, 0, sizeof(reported_a));
    memset(reported_bg, 0, sizeof(reported_bg));
    memset(reported_49Ghz, 0, sizeof(reported_49Ghz));

    for (i = 0; i < ic->ic_nchans; i++)
    {
        const struct ieee80211_channel *c = &ic->ic_channels[i];

        if (IS_NEW_CHANNEL(c) || IEEE80211_IS_CHAN_HALF(c) ||
            IEEE80211_IS_CHAN_QUARTER(c))
        {
            if ((IEEE80211_IS_CHAN_5GHZ(c)) && (c->ic_freq > 5000))
                setbit(reported_a, c->ic_ieee);
            else if ((IEEE80211_IS_CHAN_5GHZ(c)) && (c->ic_freq < 5000))
                setbit(reported_49Ghz, c->ic_ieee);
            else
                setbit(reported_bg, c->ic_ieee);

            chan[*nchan].ic_ieee = c->ic_ieee;
            chan[*nchan].ic_freq = c->ic_freq;
            chan[*nchan].ic_flags = c->ic_flags;
            chan[*nchan].ic_regClassId = c->ic_regClassId;
            if(IEEE80211_IS_CHAN_11AC_VHT80(c)) {
                chan[*nchan].ic_vhtop_ch_freq_seg1 = c->ic_vhtop_ch_freq_seg1;
                chan[*nchan].ic_vhtop_ch_freq_seg2 = c->ic_vhtop_ch_freq_seg2;
            }

            /*
             * Copy HAL extention channel flags to IEEE channel.This is needed 
             * by the wlanconfig to report the DFS channels. 
             */
            chan[*nchan].ic_flagext = c->ic_flagext;  
            if (++(*nchan) >= IEEE80211_CHAN_MAX)
                break;
        }
        else if (!IS_NEW_CHANNEL(c)){
            for (j = 0; j < *nchan; j++){
                if (chan[j].ic_freq == c->ic_freq){
                    chan[j].ic_flags |= c->ic_flags;
                    if(IEEE80211_IS_CHAN_11AC_VHT80(c)) {
                        chan[j].ic_vhtop_ch_freq_seg1 = c->ic_vhtop_ch_freq_seg1;
                        chan[j].ic_vhtop_ch_freq_seg2 = c->ic_vhtop_ch_freq_seg2;
                    }
                    continue;
                }
            }
        }
    }
    return 0;
#undef IS_NEW_CHANNEL
}

int wlan_get_txpow(wlan_if_t vaphandle, int *txpow, int *fixed)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;

    *txpow = vap->iv_bss->ni_txpower/2;
    *fixed = (ic->ic_flags & IEEE80211_F_TXPOW_FIXED) != 0;
    return 0;
}

int wlan_set_txpow(wlan_if_t vaphandle, int txpow)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    int is2GHz = IEEE80211_IS_CHAN_2GHZ(ic->ic_curchan);
    int fixed = (ic->ic_flags & IEEE80211_F_TXPOW_FIXED) != 0;

    if (txpow > 0) {
        if ((ic->ic_caps & IEEE80211_C_TXPMGT) == 0)
            return -EINVAL;
        /*
         * txpow is in dBm while we store in 0.5dBm units
         */
        if(ic->ic_set_txPowerLimit)
            ic->ic_set_txPowerLimit(ic, 2*txpow, 2*txpow, is2GHz);
        ic->ic_flags |= IEEE80211_F_TXPOW_FIXED;
    }
    else {
        if (!fixed) return EOK;

        if(ic->ic_set_txPowerLimit)
            ic->ic_set_txPowerLimit(ic,IEEE80211_TXPOWER_MAX,
                    IEEE80211_TXPOWER_MAX, is2GHz);
        ic->ic_flags &= ~IEEE80211_F_TXPOW_FIXED;
    }
    return EOK;
}

u_int32_t
wlan_get_HWcapabilities(wlan_dev_t devhandle, ieee80211_cap cap)
{
    struct ieee80211com *ic = devhandle;
    u_int32_t val = 0;

    switch (cap) {
    case IEEE80211_CAP_SHSLOT:
        val = (ic->ic_caps & IEEE80211_C_SHSLOT) ? 1 : 0;
        break;

    case IEEE80211_CAP_SHPREAMBLE:
        val = (ic->ic_caps & IEEE80211_F_SHPREAMBLE) ? 1 : 0;
        break;

    case IEEE80211_CAP_MULTI_DOMAIN:
        val = (ic->ic_country.isMultidomain) ? 1 : 0;
        break;

    case IEEE80211_CAP_WMM:
        val = (ic->ic_caps & IEEE80211_C_WME) ? 1 : 0;
        break;

    case IEEE80211_CAP_HT:
        val = (ic->ic_caps & IEEE80211_C_HT) ? 1 : 0;
        break;

    case IEEE80211_CAP_PERF_PWR_OFLD:
        val = (ic->ic_caps_ext & IEEE80211_CEXT_PERF_PWR_OFLD) ? 1 : 0;
        break;

    case IEEE80211_CAP_11AC:
        val = (ic->ic_caps_ext & IEEE80211_CEXT_11AC) ? 1 : 0;
        break;

    default:
        break;
    }

    return val;
}

int wlan_get_maxphyrate(wlan_if_t vaphandle)
{
 struct ieee80211vap *vap = vaphandle;
 struct ieee80211com *ic = vap->iv_ic; 
 /* Rate should show 0 if VAP is not UP */
 return(!ieee80211_vap_ready_is_set(vap) ? 0:ic->ic_get_maxphyrate(ic, vap->iv_bss) * 1000);
} 

int wlan_get_current_phytype(struct ieee80211com *ic)
{
  return(ic->ic_phytype);
}

int
ieee80211_get_desired_ssid(struct ieee80211vap *vap, int index, ieee80211_ssid **ssid)
{
    if (index > vap->iv_des_nssid) {
        return -EOVERFLOW;
    }

    *ssid = &(vap->iv_des_ssid[index]);
    return 0;
}

int
ieee80211_get_desired_ssidlist(struct ieee80211vap *vap, 
                               ieee80211_ssid *ssidlist,
                               int nssid)
{
    int i;

    if (nssid < vap->iv_des_nssid)
        return -EOVERFLOW;
    
    for (i = 0; i < vap->iv_des_nssid; i++) {
        ssidlist[i].len = vap->iv_des_ssid[i].len;
        OS_MEMCPY(ssidlist[i].ssid,
                  vap->iv_des_ssid[i].ssid,
                  ssidlist[i].len);
    }

    return vap->iv_des_nssid;
}

int
wlan_get_desired_ssidlist(wlan_if_t vaphandle, ieee80211_ssid *ssidlist, int nssid)
{
    return ieee80211_get_desired_ssidlist(vaphandle, ssidlist, nssid);
}

int
wlan_set_desired_ssidlist(wlan_if_t vaphandle,
                          u_int16_t nssid, 
                          ieee80211_ssid *ssidlist)
{
    struct ieee80211vap *vap = vaphandle;
    int i;

    if (nssid > IEEE80211_SCAN_MAX_SSID) {
        return -EOVERFLOW;
    }

    for (i = 0; i < nssid; i++) {
        vap->iv_des_ssid[i].len = ssidlist[i].len;
        if (vap->iv_des_ssid[i].len) {
            OS_MEMCPY(vap->iv_des_ssid[i].ssid,
                      ssidlist[i].ssid,
                      ssidlist[i].len);
        }
    }

    vap->iv_des_nssid = nssid;    
    return 0; 
}

void
wlan_get_bss_essid(wlan_if_t vaphandle, ieee80211_ssid *essid)
{
    struct ieee80211vap *vap = vaphandle;
    essid->len = vap->iv_bss->ni_esslen;
    OS_MEMCPY(essid->ssid,vap->iv_bss->ni_essid, vap->iv_bss->ni_esslen);
}

int wlan_set_wmm_param(wlan_if_t vaphandle, wlan_wme_param param, u_int8_t isbss, u_int8_t ac, u_int32_t val)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    int retval=EOK;
    struct ieee80211_wme_state *wme = &ic->ic_wme;

    if(isbss) 
    {
        wme->wme_flags |= WME_F_BSSPARAM_UPDATED;
    }
    switch (param)
    {
    case WLAN_WME_CWMIN:
        if (isbss)
        {
            wme->wme_wmeBssChanParams.cap_wmeParams[ac].wmep_logcwmin = val;
            if ((wme->wme_flags & WME_F_AGGRMODE) == 0)
            {
                wme->wme_bssChanParams.cap_wmeParams[ac].wmep_logcwmin = val;
            }
        }
        else
        {
            wme->wme_wmeChanParams.cap_wmeParams[ac].wmep_logcwmin = val;
            wme->wme_chanParams.cap_wmeParams[ac].wmep_logcwmin = val;
        }
        ieee80211_wme_updateparams(vap);
        break;
    case WLAN_WME_CWMAX:
        if (isbss)
        {
            wme->wme_wmeBssChanParams.cap_wmeParams[ac].wmep_logcwmax = val;
            if ((wme->wme_flags & WME_F_AGGRMODE) == 0)
            {
                wme->wme_bssChanParams.cap_wmeParams[ac].wmep_logcwmax = val;
            }
        }
        else
        {
            wme->wme_wmeChanParams.cap_wmeParams[ac].wmep_logcwmax = val;
            wme->wme_chanParams.cap_wmeParams[ac].wmep_logcwmax = val;
        }
        ieee80211_wme_updateparams(vap);
        break;
    case WLAN_WME_AIFS:
        if (isbss)
        {
            wme->wme_wmeBssChanParams.cap_wmeParams[ac].wmep_aifsn = val;
            if ((wme->wme_flags & WME_F_AGGRMODE) == 0)
            {
                wme->wme_bssChanParams.cap_wmeParams[ac].wmep_aifsn = val;
            }
        }
        else
        {
            wme->wme_wmeChanParams.cap_wmeParams[ac].wmep_aifsn = val;
            wme->wme_chanParams.cap_wmeParams[ac].wmep_aifsn = val;
        }
        ieee80211_wme_updateparams(vap);
        break;
    case WLAN_WME_TXOPLIMIT:
        if (isbss)
        {
            wme->wme_wmeBssChanParams.cap_wmeParams[ac].wmep_txopLimit
                = IEEE80211_US_TO_TXOP(val);
            if ((wme->wme_flags & WME_F_AGGRMODE) == 0)
            {
                wme->wme_bssChanParams.cap_wmeParams[ac].wmep_txopLimit
                    = IEEE80211_US_TO_TXOP(val);
            }
        }
        else
        {
            wme->wme_wmeChanParams.cap_wmeParams[ac].wmep_txopLimit
                = IEEE80211_US_TO_TXOP(val);
            wme->wme_chanParams.cap_wmeParams[ac].wmep_txopLimit
                = IEEE80211_US_TO_TXOP(val);
        }
        ieee80211_wme_updateparams(vap);
        break;
    case WLAN_WME_ACM:
        if (!isbss)
            return -EINVAL;
        /* ACM bit applies to BSS case only */
        wme->wme_wmeBssChanParams.cap_wmeParams[ac].wmep_acm = val;
        if ((wme->wme_flags & WME_F_AGGRMODE) == 0)
            wme->wme_bssChanParams.cap_wmeParams[ac].wmep_acm = val;
        ieee80211_wme_updateparams(vap);
        break;
    case WLAN_WME_ACKPOLICY:
        if (isbss)
            return -EINVAL;
        /* ack policy applies to non-BSS case only */
        wme->wme_wmeChanParams.cap_wmeParams[ac].wmep_noackPolicy = val;
        wme->wme_chanParams.cap_wmeParams[ac].wmep_noackPolicy = val;
        ieee80211_wme_updateparams(vap);
        break;
    default:
        break;
    }
    return retval;
}

u_int32_t wlan_get_wmm_param(wlan_if_t vaphandle, wlan_wme_param param, u_int8_t isbss, u_int8_t ac)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_wme_state *wme = &ic->ic_wme;
    struct chanAccParams *chanParams = (isbss == 0) ?
            &(wme->wme_chanParams)
            : &(wme->wme_bssChanParams);

    switch (param)
    {
    case WLAN_WME_CWMIN:
        return chanParams->cap_wmeParams[ac].wmep_logcwmin;
        break;
    case WLAN_WME_CWMAX:
        return chanParams->cap_wmeParams[ac].wmep_logcwmax;
        break;
    case WLAN_WME_AIFS:
        return chanParams->cap_wmeParams[ac].wmep_aifsn;
        break;
    case WLAN_WME_TXOPLIMIT:
        return IEEE80211_TXOP_TO_US(chanParams->cap_wmeParams[ac].wmep_txopLimit);
        break;
    case WLAN_WME_ACM:
        return wme->wme_wmeBssChanParams.cap_wmeParams[ac].wmep_acm;
        break;
    case WLAN_WME_ACKPOLICY:
        return wme->wme_wmeChanParams.cap_wmeParams[ac].wmep_noackPolicy;
        break;
    default:
        break;
    }
    return 0;
}

int wlan_set_chanswitch(wlan_if_t vaphandle, u_int8_t chan, u_int8_t tbtt, u_int16_t ch_width)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    int freq;
    u_int32_t flags = 0;
#ifdef MAGPIE_HIF_GMAC
    struct ieee80211vap *tmp_vap = NULL;
#endif   
    freq = ieee80211_ieee2mhz(chan, 0);
    ic->ic_chanchange_channel = NULL;

    if ((ch_width == 0) &&(ieee80211_find_channel(ic, freq, ic->ic_curchan->ic_flags) == NULL)) {
        /* Switching between different modes is not allowed, print ERROR */
        printk("%s(): Channel capabilities do not match, chan flags 0x%x\n",
            __func__, ic->ic_curchan->ic_flags);
        return 0;
    }
    else {
         /* deriving the mode from ch_width value */
        if (ch_width == CHWIDTH_VHT20 || ch_width == CHWIDTH_VHT40) {
            if(ch_width == CHWIDTH_VHT20)
               flags = IEEE80211_CHAN_11AC_VHT20;
            else
               flags = IEEE80211_CHAN_11AC_VHT40PLUS;
        }
        else if (ch_width == CHWIDTH_VHT80) {
            flags = IEEE80211_CHAN_11AC_VHT80;
        }
        
        if (flags) {
            if ((ic->ic_chanchange_channel = ieee80211_find_channel(ic, freq, flags)) == NULL) {
                 /* check VHT40MINUS also if channel width is 40, otherwise return */
                if (flags == IEEE80211_CHAN_11AC_VHT40PLUS) {
                    if ((ic->ic_chanchange_channel = ieee80211_find_channel(ic, freq, IEEE80211_CHAN_11AC_VHT40MINUS)) == NULL) {
                        /* Channel is not available for the ch_width */
			IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,
			    "%s(): Channel capabilities do not match, chan flags 0x%x\n",
                            __func__, flags);
                        return 0;
                    }
                }
                else {
                    /* Channel is not available for the ch_width */
		    IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,
		        "%s(): Channel capabilities do not match, chan flags 0x%x\n",
                        __func__, flags);
                    return 0;
                }
            }
            ic->ic_chanchange_chwidth = ch_width;
        }
    }
    /*  flag the beacon update to include the channel switch IE */
    ic->ic_chanchange_chan = chan;
    ic->ic_chanchange_tbtt = tbtt;
#ifdef MAGPIE_HIF_GMAC
    TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
        ic->ic_chanchange_cnt += ic->ic_chanchange_tbtt;    
    }
#endif    
    ic->ic_flags |= IEEE80211_F_CHANSWITCH;
    return 0;
}

int wlan_set_clr_appopt_ie(wlan_if_t vaphandle)
{
    int ftype;

    IEEE80211_VAP_LOCK(vaphandle);
    /* Free app ie buffers */
    for (ftype = 0; ftype < IEEE80211_FRAME_TYPE_MAX; ftype++) {
        vaphandle->iv_app_ie_maxlen[ftype] = 0;
        if (vaphandle->iv_app_ie[ftype].ie) {
            OS_FREE(vaphandle->iv_app_ie[ftype].ie);
            vaphandle->iv_app_ie[ftype].ie = NULL;
            vaphandle->iv_app_ie[ftype].length = 0;
        }
    }
    
    /* Free opt ie buffer */
    vaphandle->iv_opt_ie_maxlen = 0;
    if (vaphandle->iv_opt_ie.ie) {
        OS_FREE(vaphandle->iv_opt_ie.ie);
        vaphandle->iv_opt_ie.ie = NULL;
        vaphandle->iv_opt_ie.length = 0;
    }

    /* Free beacon copy buffer */
    if (vaphandle->iv_beacon_copy_buf) {
        OS_FREE(vaphandle->iv_beacon_copy_buf);
        vaphandle->iv_beacon_copy_buf = NULL;
        vaphandle->iv_beacon_copy_len = 0;
    }        
    IEEE80211_VAP_UNLOCK(vaphandle);

    return 0;
}

int wlan_is_hwbeaconproc_active(wlan_if_t vaphandle)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;

    return ic->ic_is_hwbeaconproc_active(ic);
}

void
wlan_get_vap_addr(wlan_if_t vaphandle, u_int8_t *mac)
{
    struct ieee80211vap *vap = vaphandle;

    memcpy(mac, vap->iv_myaddr, IEEE80211_ADDR_LEN);
}

/* set/get IQUE parameters */
#if ATH_SUPPORT_IQUE
int wlan_set_rtparams(wlan_if_t vaphandle, u_int8_t rt_index, u_int8_t per, u_int8_t probe_intvl)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    if(ic->ic_set_rtparams)
        ic->ic_set_rtparams(ic, rt_index, per, probe_intvl);
    return 0;
}

int wlan_set_acparams(wlan_if_t vaphandle, u_int8_t ac, u_int8_t use_rts, u_int8_t aggrsize_scaling, u_int32_t min_kbps)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    if(ic->ic_set_acparams)
        ic->ic_set_acparams(ic, ac, use_rts, aggrsize_scaling, min_kbps);
    return 0;
}

int wlan_set_hbrparams(wlan_if_t vaphandle, u_int8_t ac, u_int8_t enable, u_int8_t per_low)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    if(ic->ic_set_hbrparams)
        ic->ic_set_hbrparams(vap, ac, enable, per_low);
    return 0;
}

void wlan_get_hbrstate(wlan_if_t vaphandle)
{
    struct ieee80211vap *vap = vaphandle;
    if (vap->iv_ique_ops.hbr_dump) {
        vap->iv_ique_ops.hbr_dump(vap);
    }
}

int wlan_set_me_denyentry(wlan_if_t vaphandle, int *denyaddr)
{
    struct ieee80211vap *vap = vaphandle;
    if (vap->iv_ique_ops.me_adddeny) {
        vap->iv_ique_ops.me_adddeny(vap, denyaddr);
    }
    return 0;
}
#endif /* ATH_SUPPORT_IQUE */


#ifdef  ATH_SUPPORT_AOW

int
wlan_set_aow_param(wlan_if_t vaphandle, ieee80211_param param, u_int32_t value)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    int retv = 0;

    switch (param) {
        case IEEE80211_AOW_SWRETRIES:
            if(ic->ic_set_swretries)
                ic->ic_set_swretries(ic, value);
            break;
        case IEEE80211_AOW_RTSRETRIES:
            if(ic->ic_set_aow_rtsretries)
                ic->ic_set_aow_rtsretries(ic, value);
            break;
        case IEEE80211_AOW_LATENCY:
            if(ic->ic_set_aow_latency)
                ic->ic_set_aow_latency(ic, value);
            break;
        case IEEE80211_AOW_PLAY_LOCAL:
            if(ic->ic_set_aow_playlocal)
                ic->ic_set_aow_playlocal(ic, value);
            break;
        case IEEE80211_AOW_CLEAR_AUDIO_CHANNELS:
            if(ic->ic_aow_clear_audio_channels)
                ic->ic_aow_clear_audio_channels(ic, value);
            break;
        case IEEE80211_AOW_STATS:
            ieee80211_audio_stat_clear(ic);
            break;
        case IEEE80211_AOW_ESTATS:
            if(ic->ic_aow_clear_estats)
                ic->ic_aow_clear_estats(ic);
            break;
        case IEEE80211_AOW_INTERLEAVE:
            ic->ic_aow.interleave = value ? 1:0;    
            break;
        case IEEE80211_AOW_ER:
            if(ic->ic_set_aow_er)
                ic->ic_set_aow_er(ic, value);
            break;
        case IEEE80211_AOW_EC:
            if(ic->ic_set_aow_ec)
                ic->ic_set_aow_ec(ic, value);
            break;
        case IEEE80211_AOW_EC_RAMP:
            if(ic->ic_set_aow_ec_ramp)
                ic->ic_set_aow_ec_ramp(ic, value);
            break;
        case IEEE80211_AOW_EC_FMAP:
            if(ic->ic_set_aow_ec_fmap)
                ic->ic_set_aow_ec_fmap(ic, value);
            break;
        case IEEE80211_AOW_ES:
            if(ic->ic_set_aow_es)
                ic->ic_set_aow_es(ic, value);
            break;
        case IEEE80211_AOW_ESS:
            if(ic->ic_set_aow_ess)
                ic->ic_set_aow_ess(ic, value);
            break;
        case IEEE80211_AOW_ESS_COUNT:
            if(ic->ic_set_aow_ess_count)
                ic->ic_set_aow_ess_count(ic, value);
            break;
        case IEEE80211_AOW_ENABLE_CAPTURE:
            ieee80211_set_audio_data_capture(ic);
            break;
        case IEEE80211_AOW_FORCE_INPUT:
            ieee80211_set_force_aow_data(ic, value);
            break;
        case IEEE80211_AOW_PRINT_CAPTURE:
            ieee80211_audio_print_capture_data(ic);
            break;
        case IEEE80211_AOW_AS:
            if(ic->ic_set_aow_as)
                ic->ic_set_aow_as(ic, value);
            break;
        case IEEE80211_AOW_PLAY_RX_CHANNEL:
            if(ic->ic_set_aow_audio_ch)
                ic->ic_set_aow_audio_ch(ic, value);
            break;
        case IEEE80211_AOW_SIM_CTRL_CMD:
            ieee80211_aow_sim_ctrl_msg(ic, value);
            break;
        case IEEE80211_AOW_FRAME_SIZE:
            if(ic->ic_set_aow_frame_size)
                ic->ic_set_aow_frame_size(ic, value);
            break;
        case IEEE80211_AOW_ALT_SETTING:
            if( ic->ic_set_aow_alt_setting)
                ic->ic_set_aow_alt_setting(ic, value);
            break;
        case IEEE80211_AOW_ASSOC_ONLY:
            if(ic->ic_set_aow_assoc_policy)
                ic->ic_set_aow_assoc_policy(ic, value);
            break;
        case IEEE80211_AOW_DISCONNECT_DEVICE:
            ieee80211_aow_disconnect_device(ic, value);
            break;
        default:
            break;
    }

    return retv;
}    

u_int32_t
wlan_get_aow_param(wlan_if_t vaphandle, ieee80211_param param)
{

    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    u_int32_t val = 0;

    switch (param) {
        case IEEE80211_AOW_SWRETRIES:
            if(ic->ic_get_swretries)
                val = ic->ic_get_swretries(ic);
            break;
        case IEEE80211_AOW_RTSRETRIES:
            if( ic->ic_get_aow_rtsretries)
                val = ic->ic_get_aow_rtsretries(ic);
            break;
        case IEEE80211_AOW_LATENCY:
            if(ic->ic_get_aow_latency)
                val = ic->ic_get_aow_latency(ic);
            break;
        case IEEE80211_AOW_PLAY_LOCAL:
            if( ic->ic_get_aow_playlocal)
                val = ic->ic_get_aow_playlocal(ic);
            break;
        case IEEE80211_AOW_STATS:

            val = ieee80211_aow_get_stats(ic);
            break;
        case IEEE80211_AOW_ESTATS:
            if(ic->ic_aow_get_estats)
                val = ic->ic_aow_get_estats(ic);
            break;
        case IEEE80211_AOW_INTERLEAVE:
            val = ic->ic_aow.interleave;
            break;
        case IEEE80211_AOW_ER:
            if(ic->ic_get_aow_er)
                val = ic->ic_get_aow_er(ic);
            break;
        case IEEE80211_AOW_EC:
            if( ic->ic_get_aow_ec)
                val = ic->ic_get_aow_ec(ic);
            break;
        case IEEE80211_AOW_EC_RAMP:
            if( ic->ic_get_aow_ec_ramp)
                val = ic->ic_get_aow_ec_ramp(ic);
            break;
        case IEEE80211_AOW_EC_FMAP:
            if( ic->ic_get_aow_ec_fmap)
                val = ic->ic_get_aow_ec_fmap(ic);
            break;
        case IEEE80211_AOW_ES:
            if( ic->ic_get_aow_es)
                val = ic->ic_get_aow_es(ic);
            break;
        case IEEE80211_AOW_ESS:
            if( ic->ic_get_aow_ess)
                val = ic->ic_get_aow_ess(ic);
            break;
        case IEEE80211_AOW_LIST_AUDIO_CHANNELS:
            if(ic->ic_list_audio_channel)
                ic->ic_list_audio_channel(ic);
            val = 0;
            break;
        case IEEE80211_AOW_AS:
            if( ic->ic_get_aow_as)
                val = ic->ic_get_aow_as(ic);
            break;
        case IEEE80211_AOW_PLAY_RX_CHANNEL:
            if( ic->ic_get_aow_audio_ch)
                val = ic->ic_get_aow_audio_ch(ic);
            break;
        case IEEE80211_AOW_SIM_CTRL_CMD:
            val = ieee80211_aow_get_ctrl_cmd(ic);
            break;
        case IEEE80211_AOW_FRAME_SIZE:
            if( ic->ic_get_aow_frame_size)
                val = ic->ic_get_aow_frame_size(ic);
            break;
        case IEEE80211_AOW_ALT_SETTING:
            if(ic->ic_get_aow_alt_setting)
                val = ic->ic_get_aow_alt_setting(ic);
            break;
        case IEEE80211_AOW_ASSOC_ONLY:
            if(ic->ic_get_aow_assoc_policy)
                val = ic->ic_get_aow_assoc_policy(ic);
            break;
        case IEEE80211_AOW_PRINT_CAPTURE:
            ieee80211_audio_print_capture_data(ic);
            val = 0;
        default:
            break;

    }
    return val;

}

#endif  /* ATH_SUPPORT_AOW */
