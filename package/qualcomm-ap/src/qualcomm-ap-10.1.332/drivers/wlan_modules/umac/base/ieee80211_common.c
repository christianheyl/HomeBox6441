/*
 *  Copyright (c) 2008 Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */

#include <ieee80211_var.h>
#include <ieee80211_channel.h>
#include <ieee80211_rateset.h>
#include <ieee80211_config.h>
#include <ieee80211_scan.h>
#include <ieee80211_tsftimer.h>
#include <ieee80211_notify_tx_bcn.h>
#include <ieee80211_aow.h>
#include <ieee80211P2P_api.h>
#include <ieee80211_prdperfstats.h>
#include <ieee80211_wnm_proto.h>
#include "ieee80211_vi_dbg.h"

#ifdef ATHR_RNWF

extern BOOLEAN
Mp11HasPendingSends(
    IN  PADAPTER    pAdapter
    );

#endif

int module_init_wlan(void);
void module_exit_wlan(void);

static bool ieee80211_is_sw_txq_empty(struct ieee80211com *ic)
{
#ifdef ATHR_RNWF
    /* 
     * Vista has a **temporary** function to determine whether upper layer has
     * frames waiting to be transmitted.
     * Use it for now, and REMOVE this function once we move the buffering of 
     * frames sent by the OS to the UMAC.
     */
    return ! Mp11HasPendingSends(ic->ic_osdev->pAdapter);
#else
    return true;
#endif
}

static void ieee80211_vap_iter_mlme_inact_timeout(void *arg, struct ieee80211vap *vap) 
{
    mlme_inact_timeout(vap);
}

/*
 * Per-ieee80211com inactivity timer callback.
 * used for checking any kind of inactivity in the
 * COM device.
 */
static OS_TIMER_FUNC(ieee80211_inact_timeout)
{
	struct ieee80211com *ic;
    OS_GET_TIMER_ARG(ic, struct ieee80211com *);
    ieee80211_timeout_stations(&ic->ic_sta);
    ieee80211_timeout_fragments(ic, IEEE80211_FRAG_TIMEOUT*1000);
    wlan_iterate_vap_list(ic,ieee80211_vap_iter_mlme_inact_timeout,NULL);
    OS_SET_TIMER(&ic->ic_inact_timer, IEEE80211_INACT_WAIT*1000);
}

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
ieee80211_ifattach(struct ieee80211com *ic, IEEE80211_REG_PARAMETERS *ieee80211_reg_parm)
{
    u_int8_t bcast[IEEE80211_ADDR_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff};

    ic->ic_reg_parm = *ieee80211_reg_parm;
    /* set up broadcast address */
    IEEE80211_ADDR_COPY(ic->ic_broadcast, bcast);
    
    /* initialize channel list */
    ieee80211_update_channellist(ic, 0);

    /* initialize rate set */
    ieee80211_init_rateset(ic);
    
    /* validate ic->ic_curmode */
    if (!IEEE80211_SUPPORT_PHY_MODE(ic, ic->ic_curmode))
        ic->ic_curmode = IEEE80211_MODE_AUTO;

    /* setup initial channel settings */
    ic->ic_curchan = ieee80211_get_channel(ic, 0); /* arbitrarily pick the first channel */

    /* Enable marking of dfs by default */
    ic->ic_flags_ext |= IEEE80211_FEXT_MARKDFS;

    if (ic->ic_reg_parm.htEnableWepTkip) {
        ieee80211_ic_wep_tkip_htrate_set(ic);
    } else {
        ieee80211_ic_wep_tkip_htrate_clear(ic);
    }

    if (ic->ic_reg_parm.htVendorIeEnable)
        IEEE80211_ENABLE_HTVIE(ic);

    /* whether to ignore 11d beacon */
    if (ic->ic_reg_parm.ignore11dBeacon)
        IEEE80211_ENABLE_IGNORE_11D_BEACON(ic);
    
    if (ic->ic_reg_parm.disallowAutoCCchange) {
        ieee80211_ic_disallowAutoCCchange_set(ic);
    }
    else {
        ieee80211_ic_disallowAutoCCchange_clear(ic);
    }
    
    (void) ieee80211_setmode(ic, ic->ic_curmode, ic->ic_opmode);

    ic->ic_intval = IEEE80211_BINTVAL_DEFAULT; /* beacon interval */
    ic->ic_lintval = 1;         /* listen interval */
    ic->ic_lintval_assoc = IEEE80211_LINTVAL_MAX; /* listen interval to use in association */
    ic->ic_bmisstimeout = IEEE80211_BMISS_LIMIT * ic->ic_intval;
    TAILQ_INIT(&ic->ic_vaps);

    ic->ic_txpowlimit = IEEE80211_TXPOWER_MAX;

    /* Intialize WDS Auto Detect mode */
    ic->ic_flags_ext |= IEEE80211_FEXT_WDS_AUTODETECT;

	/*
	** Enable the 11d country code IE by default
	*/

	ic->ic_flags_ext |= IEEE80211_FEXT_COUNTRYIE;

    /* setup CWM configuration */
    ic->ic_cwm_set_mode(ic, ic->ic_reg_parm.cwmMode);
    ic->ic_cwm_set_extoffset(ic, ic->ic_reg_parm.cwmExtOffset);
    ic->ic_cwm_set_extprotmode(ic, ic->ic_reg_parm.cwmExtProtMode);
    ic->ic_cwm_set_extprotspacing(ic, ic->ic_reg_parm.cwmExtProtSpacing);

#if tbd
    /* XXX - TODO - move these into ath layer */
#else
    ic->ic_cwm_set_enable(ic, ic->ic_reg_parm.cwmEnable);
    ic->ic_cwm_set_extbusythreshold(ic, ic->ic_reg_parm.cwmExtBusyThreshold);
#endif

    ic->ic_enable2GHzHt40Cap = ic->ic_reg_parm.enable2GHzHt40Cap;

#ifdef ATH_COALESCING    
    ic->ic_tx_coalescing     = ic->ic_reg_parm.txCoalescingEnable;
#endif
    ic->ic_ignoreDynamicHalt = ic->ic_reg_parm.ignoreDynamicHalt;

    /* default to auto ADDBA mode */
    ic->ic_addba_mode = ADDBA_MODE_AUTO;

    if (ic->ic_reg_parm.ht20AdhocEnable) {
        /*
         * Support HT rates in Ad hoc connections.
         */
        if (IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11NA_HT20) ||
            IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11NG_HT20)) {
            ieee80211_ic_ht20Adhoc_set(ic);
        
            if (ic->ic_reg_parm.htAdhocAggrEnable) {
                ieee80211_ic_htAdhocAggr_set(ic);
            }
        }
    }

    if (ic->ic_reg_parm.ht40AdhocEnable) {
        /*
         * Support HT rates in Ad hoc connections.
         */
        if (IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11NA_HT40PLUS) ||
            IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11NA_HT40MINUS) ||
            IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11NG_HT40PLUS) ||
            IEEE80211_SUPPORT_PHY_MODE(ic, IEEE80211_MODE_11NG_HT40MINUS)) {
            ieee80211_ic_ht40Adhoc_set(ic);
        
            if (ic->ic_reg_parm.htAdhocAggrEnable) {
                ieee80211_ic_htAdhocAggr_set(ic);
            }
        }
    } 

    OS_INIT_TIMER(ic->ic_osdev, &(ic->ic_inact_timer), ieee80211_inact_timeout, (void *) (ic));
#if UMAC_SUPPORT_WNM
    OS_INIT_TIMER(ic->ic_osdev, &(ic->ic_bssload_timer), ieee80211_bssload_timeout, (void *) (ic));
#endif
        
    if (ic->ic_reg_parm.disable2040Coexist) {
        ic->ic_flags |= IEEE80211_F_COEXT_DISABLE;
    } else {
        ic->ic_flags &= ~IEEE80211_F_COEXT_DISABLE;
    }

    /* setup other modules */

    /* The TSF Timer module is required when P2P or Off-channel support are required */
    ic->ic_tsf_timer = ieee80211_tsf_timer_attach(ic);
#if UMAC_SUPPORT_TDLS_CHAN_SWITCH
     /* TDLS off-channel support requires TSF timer */
    if (ic->ic_tsf_timer) {
        ieee80211_ic_off_channel_support_set(ic);
    }
    else {
        ieee80211_ic_off_channel_support_clear(ic);
    }
#else
    ieee80211_ic_off_channel_support_clear(ic);
#endif

    ieee80211_p2p_attach(ic);
    ieee80211_crypto_attach(ic);
    ieee80211_node_attach(ic);
    ieee80211_proto_attach(ic);
    ieee80211_power_attach(ic);
    ieee80211_mlme_attach(ic);
#if ATH_SUPPORT_DFS
    ieee80211_dfs_attach(ic);
#endif /* ATH_SUPPORT_DFS */

    if (IEEE80211_ENAB_AOW(ic))
        ieee80211_aow_attach(ic);

    ieee80211_scan_table_attach(ic, &(ic->ic_scan_table), ic->ic_osdev);
    /*
     * By default overwrite probe response with beacon IE in scan entry.
     */
    ieee80211_ic_override_proberesp_ie_set(ic); 
    ieee80211_scan_attach(&(ic->ic_scanner), 
                          ic, 
                          ic->ic_osdev, 
                          ieee80211_is_connected,
                          ieee80211_is_txq_empty, 
                          ieee80211_is_sw_txq_empty);
    ic->ic_resmgr = ieee80211_resmgr_create(ic, IEEE80211_RESMGR_MODE_SINGLE_CHANNEL);

    ieee80211_acs_attach(&(ic->ic_acs), 
                          ic, 
                          ic->ic_osdev);

    ic->ic_notify_tx_bcn_mgr = ieee80211_notify_tx_bcn_attach(ic);
    ieee80211_rptplacement_attach(ic);
    IEEE80211_TDLS_ATTACH(ic);
#if UMAC_SUPPORT_VI_DBG
    ieee80211_vi_dbg_attach(ic);
#endif
    ieee80211_quiet_attach(ic);
	ieee80211_admctl_attach(ic);

    /* 
     * Perform steps that require multiple objects to be initialized. 
     * For example, cross references between objects such as ResMgr and Scanner.
     */
    ieee80211_scan_attach_complete(ic->ic_scanner);
    ieee80211_resmgr_create_complete(ic->ic_resmgr);
    ieee80211_smartantenna_attach(ic);

    ieee80211_prdperfstats_attach(ic);

    /* initialization complete */
    ic->ic_initialized = 1;

    return 0;
}

void
ieee80211_ifdetach(struct ieee80211com *ic)
{
    if (!ic->ic_initialized) {
        return;
    }

    /* 
     * Preparation for detaching objects.
     * For example, remove and cross references between objects such as those 
     * between ResMgr and Scanner.
     */
    ieee80211_scan_detach_prepare(ic->ic_scanner);
    ieee80211_resmgr_delete_prepare(ic->ic_resmgr);

    OS_FREE_TIMER(&ic->ic_inact_timer);
#if UMAC_SUPPORT_WNM
    OS_FREE_TIMER(&ic->ic_bssload_timer);
#endif

    /* all the vaps should have been deleted now */
    ASSERT(TAILQ_FIRST(&ic->ic_vaps) == NULL);

    ieee80211_scan_table_detach(&(ic->ic_scan_table));
    ieee80211_node_detach(ic);
    ieee80211_quiet_detach(ic);
	ieee80211_admctl_detach(ic);

    if (IEEE80211_ENAB_AOW(ic))
        ieee80211_aow_detach(ic);

#if ATH_SUPPORT_DFS
    ieee80211_dfs_detach(ic);
#endif /* ATH_SUPPORT_DFS */
    ieee80211_proto_detach(ic);
    ieee80211_crypto_detach(ic);
    ieee80211_power_detach(ic);
    ieee80211_mlme_detach(ic);
    ieee80211_notify_tx_bcn_detach(ic->ic_notify_tx_bcn_mgr);
    ieee80211_resmgr_delete(ic->ic_resmgr);
    ieee80211_scan_detach(&(ic->ic_scanner));
    ieee80211_p2p_detach(ic);
    ieee80211_acs_detach(&(ic->ic_acs));
    ieee80211_rptplacement_detach(ic);  
    IEEE80211_TDLS_DETACH(ic);
#if UMAC_SUPPORT_VI_DBG
    ieee80211_vi_dbg_detach(ic);
#endif
    ieee80211_smartantenna_detach(ic);

    ieee80211_prdperfstats_detach(ic);
    /* Detach TSF timer at the end to avoid assertion */
    if (ic->ic_tsf_timer) {
        ieee80211_tsf_timer_detach(ic->ic_tsf_timer);
        ic->ic_tsf_timer = NULL;
    }

    spin_lock_destroy(&ic->ic_lock);
    IEEE80211_STATE_LOCK_DESTROY(ic);
}

/*
 * Start this IC
 */
void ieee80211_start_running(struct ieee80211com *ic)
{
    OS_SET_TIMER(&ic->ic_inact_timer, IEEE80211_INACT_WAIT*1000);
}

/*
 * Stop this IC
 */
void ieee80211_stop_running(struct ieee80211com *ic)
{
    OS_CANCEL_TIMER(&ic->ic_inact_timer);
}

int ieee80211com_register_event_handlers(struct ieee80211com *ic, 
                                     void *event_arg,
                                     wlan_dev_event_handler_table *evtable)
{
    int i;
    /* unregister if there exists one already */
    ieee80211com_unregister_event_handlers(ic,event_arg,evtable);
    IEEE80211_COMM_LOCK(ic);
    for (i=0;i<IEEE80211_MAX_DEVICE_EVENT_HANDLERS; ++i) {
        if ( ic->ic_evtable[i] == NULL) {
            ic->ic_evtable[i] = evtable;
            ic->ic_event_arg[i] = event_arg;
            IEEE80211_COMM_UNLOCK(ic);
            return 0;
        }
    }
    IEEE80211_COMM_UNLOCK(ic);
    return -ENOMEM;


}

int ieee80211com_unregister_event_handlers(struct ieee80211com *ic, 
                                     void *event_arg,
                                     wlan_dev_event_handler_table *evtable)
{
    int i;
    IEEE80211_COMM_LOCK(ic);
    for (i=0;i<IEEE80211_MAX_DEVICE_EVENT_HANDLERS; ++i) {
        if ( ic->ic_evtable[i] == evtable &&  ic->ic_event_arg[i] == event_arg) {
            ic->ic_evtable[i] = NULL;
            ic->ic_event_arg[i] = NULL;
            IEEE80211_COMM_UNLOCK(ic);
            return 0;
        }
    }
    IEEE80211_COMM_UNLOCK(ic);
    return -EEXIST;
}

/* Clear user defined ADDBA response codes for all nodes. */
static void 
ieee80211_addba_clearresponse(void *arg, struct ieee80211_node *ni)
{
    struct ieee80211com *ic = (struct ieee80211com *) arg;
    ic->ic_addba_clearresponse(ni);
}

int wlan_device_register_event_handlers(wlan_dev_t devhandle, 
                                     void *event_arg,
                                     wlan_dev_event_handler_table *evtable)
{

    return ieee80211com_register_event_handlers(devhandle,event_arg,evtable);
}


int wlan_device_unregister_event_handlers(wlan_dev_t devhandle, 
                                     void *event_arg,
                                     wlan_dev_event_handler_table *evtable)
{
    return ieee80211com_unregister_event_handlers(devhandle,event_arg,evtable);
}


int wlan_set_device_param(wlan_dev_t ic, ieee80211_device_param param, u_int32_t val)
{
    int retval=EOK;
    switch(param) {
    case IEEE80211_DEVICE_TX_CHAIN_MASK:
    case IEEE80211_DEVICE_TX_CHAIN_MASK_LEGACY:
	if(ic->ic_set_chain_mask(ic,param,val) == 0) {
            ic->ic_tx_chainmask = val;
        } else { 
            retval=EINVAL;
        }
        break;
    case IEEE80211_DEVICE_RX_CHAIN_MASK:
    case IEEE80211_DEVICE_RX_CHAIN_MASK_LEGACY:
	if(ic->ic_set_chain_mask(ic,param,val) == 0) {
            ic->ic_rx_chainmask = val;
        } else { 
            retval=EINVAL;
        }
        break;

    case IEEE80211_DEVICE_PROTECTION_MODE:
        if (val > IEEE80211_PROT_RTSCTS) {
	    retval=EINVAL;
        } else {
	   ic->ic_protmode = val;
        }
        break;
    case IEEE80211_DEVICE_NUM_TX_CHAIN:
    case IEEE80211_DEVICE_NUM_RX_CHAIN:
    case IEEE80211_DEVICE_COUNTRYCODE:
       /* read only */
	retval=EINVAL;
        break;
    case IEEE80211_DEVICE_BMISS_LIMIT:
    	ic->ic_bmisstimeout = val * ic->ic_intval;
        break;
    case IEEE80211_DEVICE_BLKDFSCHAN:
        if (val == 0) {
            ieee80211_ic_block_dfschan_clear(ic);
        } else {
            ieee80211_ic_block_dfschan_set(ic);
        }
        break;
    case IEEE80211_DEVICE_GREEN_AP_PS_ENABLE:
        ic->ic_green_ap_set_enable(ic, val);
        break;
    case IEEE80211_DEVICE_GREEN_AP_PS_TIMEOUT:
        ic->ic_green_ap_set_transition_time(ic, val);
        break;
    case IEEE80211_DEVICE_GREEN_AP_PS_ON_TIME:
        ic->ic_green_ap_set_on_time(ic, val);
        break;
    case IEEE80211_DEVICE_GREEN_AP_ENABLE_PRINT:
        ic->ic_green_ap_set_print_level(ic, val);
        break;
    case IEEE80211_DEVICE_CWM_EXTPROTMODE:
        if (val < IEEE80211_CWM_EXTPROTMAX) {
            ic->ic_cwm_set_extprotmode(ic, val);
        } else {
            retval = EINVAL;
        }
        break;
    case IEEE80211_DEVICE_CWM_EXTPROTSPACING:
        if (val < IEEE80211_CWM_EXTPROTSPACINGMAX) {
            ic->ic_cwm_set_extprotspacing(ic, val);
        } else {
            retval = EINVAL;
        }
        break;
    case IEEE80211_DEVICE_CWM_ENABLE:
        ic->ic_cwm_set_enable(ic, val);
        break;
    case IEEE80211_DEVICE_CWM_EXTBUSYTHRESHOLD:
        ic->ic_cwm_set_extbusythreshold(ic, val);
        break;
    case IEEE80211_DEVICE_DOTH:
        if (val == 0) {
            ieee80211_ic_doth_clear(ic);
        } else {
            ieee80211_ic_doth_set(ic);
        }
        break;
#if UMAC_SUPPORT_TDLS_CHAN_SWITCH
    case IEEE80211_DEVICE_OFF_CHANNEL_SUPPORT:
         if (val == 0) {
             ieee80211_ic_off_channel_support_clear(ic);
         } else {
             if (ic->ic_tsf_timer) {
                 ieee80211_ic_off_channel_support_set(ic);
             }
             else {
                 printk("%s: Cannot enable off-channel support ic_tsf_timer=%p",
                     __func__, ic->ic_tsf_timer);
             }
         }
         break;
#endif
    case IEEE80211_DEVICE_ADDBA_MODE:
        ic->ic_addba_mode = val;
        /*
        * Clear any user defined ADDBA response codes before switching modes.
        */
        ieee80211_iterate_node(ic, ieee80211_addba_clearresponse, ic);
        break;
    case IEEE80211_DEVICE_MULTI_CHANNEL:
        if (!val) {
            /* Disable Multi-Channel */
            retval = ieee80211_resmgr_setmode(ic->ic_resmgr, IEEE80211_RESMGR_MODE_SINGLE_CHANNEL);
        } 
        else if (ic->ic_caps_ext & IEEE80211_CEXT_MULTICHAN) {
            retval = ieee80211_resmgr_setmode(ic->ic_resmgr, IEEE80211_RESMGR_MODE_MULTI_CHANNEL);
        }
        else {
            printk("%s: Unable to enable Multi-Channel Scheduling since device/driver don't support it.\n", __func__);
            retval = EINVAL;
        }
        break;
    case IEEE80211_DEVICE_MAX_AMSDU_SIZE:
        ic->ic_amsdu_max_size = val;
        break; 
#if ATH_SUPPORT_IBSS_HT        
    case IEEE80211_DEVICE_HT20ADHOC:
        if (val == 0) {
            ieee80211_ic_ht20Adhoc_clear(ic);
        } else {
            ieee80211_ic_ht20Adhoc_set(ic);
        }
        break;
    case IEEE80211_DEVICE_HT40ADHOC:
        if (val == 0) {
            ieee80211_ic_ht40Adhoc_clear(ic);
        } else {
            ieee80211_ic_ht40Adhoc_set(ic);
        }
        break;
    case IEEE80211_DEVICE_HTADHOCAGGR:
        if (val == 0) {
            ieee80211_ic_htAdhocAggr_clear(ic);
        } else {
            ieee80211_ic_htAdhocAggr_set(ic);
        }
        break;
#endif /* end of #if ATH_SUPPORT_IBSS_HT */        
    case IEEE80211_DEVICE_PWRTARGET:
        ieee80211com_set_curchanmaxpwr(ic, val);
        break;
    case IEEE80211_DEVICE_P2P:
        if (val == 0) {
            ieee80211_ic_p2pDevEnable_clear(ic);
        }
        else if (ic->ic_caps_ext & IEEE80211_CEXT_P2P) {
            ieee80211_ic_p2pDevEnable_set(ic);
        }
        else {
            printk("%s: Unable to enable P2P since device/driver don't support it.\n", __func__);
            retval = EINVAL;
        }
        break;

    case IEEE80211_DEVICE_OVERRIDE_SCAN_PROBERESPONSE_IE:
      if (val) {
          ieee80211_ic_override_proberesp_ie_set(ic); 
      } else {
          ieee80211_ic_override_proberesp_ie_clear(ic);
      }
      break;
    case IEEE80211_DEVICE_2G_CSA:
        if (val == 0) {
            ieee80211_ic_2g_csa_clear(ic);
        } else {
            ieee80211_ic_2g_csa_set(ic);
        }
        break;

    default:
        printk("%s: Error: invalid param=%d.\n", __func__, param);
    }
    return retval;

}

u_int32_t wlan_get_device_param(wlan_dev_t ic, ieee80211_device_param param)
{
    IEEE80211_COUNTRY_ENTRY c;

    switch(param) {
    case IEEE80211_DEVICE_NUM_TX_CHAIN:
        return (ic->ic_num_tx_chain);
        break;
    case IEEE80211_DEVICE_NUM_RX_CHAIN:
        return (ic->ic_num_rx_chain);
        break;
    case IEEE80211_DEVICE_TX_CHAIN_MASK:
        return (ic->ic_tx_chainmask);
        break;
    case IEEE80211_DEVICE_RX_CHAIN_MASK:
        return (ic->ic_rx_chainmask);
        break;
    case IEEE80211_DEVICE_PROTECTION_MODE:
	return (ic->ic_protmode );
        break;
    case IEEE80211_DEVICE_BMISS_LIMIT:
    	return (ic->ic_bmisstimeout / ic->ic_intval);
        break;
    case IEEE80211_DEVICE_BLKDFSCHAN:
        return (ieee80211_ic_block_dfschan_is_set(ic));
        break;
    case IEEE80211_DEVICE_GREEN_AP_PS_ENABLE:
        return ic->ic_green_ap_get_enable(ic);
        break;
    case IEEE80211_DEVICE_GREEN_AP_PS_TIMEOUT:
        return ic->ic_green_ap_get_transition_time(ic);
        break;
    case IEEE80211_DEVICE_GREEN_AP_PS_ON_TIME:
        return ic->ic_green_ap_get_on_time(ic);
        break;
    case IEEE80211_DEVICE_GREEN_AP_ENABLE_PRINT:
        return ic->ic_green_ap_get_print_level(ic);
        break;
    case IEEE80211_DEVICE_CWM_EXTPROTMODE:
        return ic->ic_cwm_get_extprotmode(ic);
        break;
    case IEEE80211_DEVICE_CWM_EXTPROTSPACING:
        return ic->ic_cwm_get_extprotspacing(ic);
        break;
    case IEEE80211_DEVICE_CWM_ENABLE:
        return ic->ic_cwm_get_enable(ic);
        break;
    case IEEE80211_DEVICE_CWM_EXTBUSYTHRESHOLD:
        return ic->ic_cwm_get_extbusythreshold(ic);
        break;
    case IEEE80211_DEVICE_DOTH:
        return (ieee80211_ic_doth_is_set(ic));
        break;
#if UMAC_SUPPORT_TDLS_CHAN_SWITCH
    case IEEE80211_DEVICE_OFF_CHANNEL_SUPPORT:
        return (ieee80211_ic_off_channel_support_is_set(ic));
        break;
#endif
    case IEEE80211_DEVICE_ADDBA_MODE:
        return ic->ic_addba_mode;
        break;
    case IEEE80211_DEVICE_COUNTRYCODE:
        ic->ic_get_currentCountry(ic, &c);
        return c.countryCode;
        break;
    case IEEE80211_DEVICE_MULTI_CHANNEL:
        return (ieee80211_resmgr_getmode(ic->ic_resmgr) 
                == IEEE80211_RESMGR_MODE_MULTI_CHANNEL);
        break;    
    case IEEE80211_DEVICE_MAX_AMSDU_SIZE:
        return(ic->ic_amsdu_max_size);
        break;
#if ATH_SUPPORT_IBSS_HT
    case IEEE80211_DEVICE_HT20ADHOC:
        return (ieee80211_ic_ht20Adhoc_is_set(ic));
        break;
    case IEEE80211_DEVICE_HT40ADHOC:
        return (ieee80211_ic_ht40Adhoc_is_set(ic));
        break;
    case IEEE80211_DEVICE_HTADHOCAGGR:
        return (ieee80211_ic_htAdhocAggr_is_set(ic));
        break;
#endif /* end of #if ATH_SUPPORT_IBSS_HT */
    case IEEE80211_DEVICE_PWRTARGET:
        return (ieee80211com_get_curchanmaxpwr(ic));
        break;
    case IEEE80211_DEVICE_P2P:
        return (ieee80211_ic_p2pDevEnable_is_set(ic));
        break;
    case IEEE80211_DEVICE_OVERRIDE_SCAN_PROBERESPONSE_IE:
        return  ieee80211_ic_override_proberesp_ie_is_set(ic); 
        break;
    case IEEE80211_DEVICE_2G_CSA:
        return (ieee80211_ic_2g_csa_is_set(ic));
        break;
    default:
        return 0;
    }
}

int wlan_get_device_mac_addr(wlan_dev_t ic, u_int8_t *mac_addr)
{
   IEEE80211_ADDR_COPY(mac_addr, ic->ic_myaddr);
   return EOK;
}

struct ieee80211_stats *
wlan_get_stats(wlan_if_t vaphandle)
{
    struct ieee80211vap *vap = vaphandle;
    return &vap->iv_stats;
}

void wlan_device_note(struct ieee80211com *ic, const char *fmt, ...) 
{
     char                   tmp_buf[OS_TEMP_BUF_SIZE];  
     va_list                ap;             
     va_start(ap, fmt);                                         
     vsnprintf (tmp_buf,OS_TEMP_BUF_SIZE, fmt, ap);            
     va_end(ap);                                               
     printk("%s",tmp_buf);                                       
     ic->ic_log_text(ic,tmp_buf);             
}

void wlan_get_vap_opmode_count(wlan_dev_t ic, 
                               struct ieee80211_vap_opmode_count *vap_opmode_count)
{
    ieee80211_get_vap_opmode_count(ic, vap_opmode_count);
}

static void ieee80211_vap_iter_active_vaps(void *arg, struct ieee80211vap *vap) 
{
    u_int16_t *pnactive = (u_int16_t *)arg;
    if (ieee80211_vap_active_is_set(vap))
        ++(*pnactive);

}

/*
 * returns number of vaps active.
 */
u_int16_t
ieee80211_vaps_active(struct ieee80211com *ic)
{
    u_int16_t nactive=0;
    wlan_iterate_vap_list(ic,ieee80211_vap_iter_active_vaps,(void *) &nactive);
    return nactive;
}

static void
ieee80211_iter_vap_opmode(void *arg, struct ieee80211vap *vaphandle)
{
    struct ieee80211_vap_opmode_count    *vap_opmode_count = arg;
    enum ieee80211_opmode                opmode = ieee80211vap_get_opmode(vaphandle);

    vap_opmode_count->total_vaps++;

    switch (opmode) {
    case IEEE80211_M_IBSS:
        vap_opmode_count->ibss_count++;
        break;

    case IEEE80211_M_STA:
        vap_opmode_count->sta_count++;
        break;

    case IEEE80211_M_WDS:
        vap_opmode_count->wds_count++;
        break;

    case IEEE80211_M_AHDEMO:
        vap_opmode_count->ahdemo_count++;
        break;

    case IEEE80211_M_HOSTAP:
        vap_opmode_count->ap_count++;
        break;

    case IEEE80211_M_MONITOR:
        vap_opmode_count->monitor_count++;
        break;

    case IEEE80211_M_BTAMP:
        vap_opmode_count->btamp_count++;
        break;

    default:
        vap_opmode_count->unknown_count++;

        printk("%s vap=%p unknown opmode=%d\n",
            __func__, vaphandle, opmode);
        break;
    }
}

void
ieee80211_get_vap_opmode_count(struct ieee80211com *ic, 
                               struct ieee80211_vap_opmode_count *vap_opmode_count)
{
    wlan_iterate_vap_list(ic, ieee80211_iter_vap_opmode, (void *) vap_opmode_count);
}

static void
ieee80211_vap_iter_last_traffic_timestamp(void *arg, struct ieee80211vap *vap)
{
    systime_t    *p_last_traffic_timestamp = arg;
    systime_t    current_traffic_timestamp = ieee80211_get_traffic_indication_timestamp(vap);

    if (current_traffic_timestamp > *p_last_traffic_timestamp) {
        *p_last_traffic_timestamp = current_traffic_timestamp;
    }
}

systime_t 
ieee80211com_get_traffic_indication_timestamp(struct ieee80211com *ic)
{
    systime_t    traffic_timestamp = 0;

    wlan_iterate_vap_list(ic, ieee80211_vap_iter_last_traffic_timestamp,(void *) &traffic_timestamp);

    return traffic_timestamp;
}

struct ieee80211_iter_vaps_ready_arg {
    u_int8_t num_sta_vaps_ready;
    u_int8_t num_ibss_vaps_ready; 
    u_int8_t num_ap_vaps_ready;
};

static void ieee80211_vap_iter_ready_vaps(void *arg, wlan_if_t vap) 
{
    struct ieee80211_iter_vaps_ready_arg *params = (struct ieee80211_iter_vaps_ready_arg *) arg;  
    if (ieee80211_vap_ready_is_set(vap)) {
        switch(ieee80211vap_get_opmode(vap)) {
        case IEEE80211_M_HOSTAP:
        case IEEE80211_M_BTAMP:
            params->num_ap_vaps_ready++;
            break;

        case IEEE80211_M_IBSS:
            params->num_ibss_vaps_ready++;
            break;

        case IEEE80211_M_STA:
            params->num_sta_vaps_ready++;
            break;

        default:
            break;

        }
    }
}

/*
 * returns number of vaps ready.
 */
u_int16_t
ieee80211_vaps_ready(struct ieee80211com *ic, enum ieee80211_opmode opmode)
{
    struct ieee80211_iter_vaps_ready_arg params; 
    u_int16_t nready = 0;
    OS_MEMZERO(&params, sizeof(params));
    wlan_iterate_vap_list(ic,ieee80211_vap_iter_ready_vaps,(void *) &params);
    switch(opmode) {
        case IEEE80211_M_HOSTAP:
        case IEEE80211_M_BTAMP:
            nready = params.num_ap_vaps_ready;
            break;

        case IEEE80211_M_IBSS:
            nready = params.num_ibss_vaps_ready;
            break;

        case IEEE80211_M_STA:
            nready = params.num_sta_vaps_ready;
            break;

        default:
            break;
    }
    return nready;
}

int
module_init_wlan(void)
{
    return 0;
}

void
module_exit_wlan(void)
{
}
