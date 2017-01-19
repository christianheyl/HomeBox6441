/*
 *  Copyright (c) 2008 Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */

#include "ieee80211_var.h"
#include "ieee80211_sme_api.h"
#include "ieee80211_sm.h"
#include "ieee80211_assoc_private.h"

/*
 * default max assoc and auth attemtps.
 */
#define MAX_ASSOC_ATTEMPTS 6
#define MAX_AUTH_ATTEMPTS  6
#define MAX_TSFSYNC_TIME   600     /* msec */
#define MAX_MGMT_TIME      200   /* msec */
#define AUTH_RETRY_TIME    30
#define ASSOC_RETRY_TIME   30
#define DISASSOC_WAIT_TIME 10 /* msec */ 
#define MAX_QUEUED_EVENTS  16
#define MLME_OP_CHECK_TIME 10 /* msec */
#define REJOIN_CHECKING_TIME 1000   /* Cisco AP workaround */
#define REJOIN_ATTEMP_TIME      5   /* Cisco AP workaround */
 


static OS_TIMER_FUNC(assoc_sm_timer_handler);

struct _wlan_assoc_sm {
    osdev_t           os_handle;
    wlan_if_t         vap_handle;
    ieee80211_hsm_t   hsm_handle; 
    wlan_scan_entry_t scan_entry;
    wlan_assoc_sm_event_handler sm_evhandler;
    os_if_t           osif;
    os_timer_t        sm_timer; /* generic timer */
    u_int8_t          max_assoc_attempts; /* maxmimum assoc attempts */
    u_int8_t          cur_assoc_attempts; /* current assoc attempt */
    u_int8_t          max_auth_attempts;  /* maxmimum auth attempts */
    u_int8_t          cur_auth_attempts;  /* current auth attempt */
    u_int8_t          max_mgmt_time;      /* maxmimum time to wait for response */
    u_int16_t         max_tsfsync_time;   /* max time to wait for a beacon in msec */
    u_int8_t          last_reason;
    wlan_assoc_sm_event_disconnect_reason   last_failure;
    u_int8_t          prev_bssid[IEEE80211_ADDR_LEN];
    u_int8_t          timeout_event;      /* event to dispacth when timer expires */
    u_int32_t         is_bcn_recvd:1,
                      is_stop_requested:1,
                      sync_stop_requested:1,
                      is_running:1,
                      is_join:1;
    /* Cisco AP workaround */
    u_int32_t         last_connected_time;
    u_int8_t          cur_rejoin_attempts;
};

static void ieee80211_assoc_sm_debug_print (void *ctx,const char *fmt,...) 
{
    char tmp_buf[256];
    va_list ap;
     
    va_start(ap, fmt);
    vsnprintf (tmp_buf,256, fmt, ap);
    va_end(ap);
    IEEE80211_DPRINTF(((wlan_assoc_sm_t) ctx)->vap_handle, IEEE80211_MSG_STATE,
        "%s", tmp_buf);
}
/*
 * 802.11 station association state machine implementation.
 */
static void ieee80211_send_event(wlan_assoc_sm_t sm, wlan_assoc_sm_event_type type,wlan_assoc_sm_event_disconnect_reason reason)
{
    wlan_assoc_sm_event sm_event;
    sm_event.event = type;
    sm_event.reason = reason ;
    sm_event.reasoncode = sm->last_reason;
    (* sm->sm_evhandler) (sm, sm->osif, &sm_event);
}

/* 
 * different state related functions.
 */


/*
 * INIT
 */
static void ieee80211_assoc_state_init_entry(void *ctx) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;

    if (sm->is_join) {
       sm->is_join=0;
       /* cancel any pending mlme operation */
       wlan_mlme_cancel(sm->vap_handle);
       if (sm->sync_stop_requested) {
           wlan_mlme_stop_bss(sm->vap_handle, WLAN_MLME_STOP_BSS_F_FORCE_STOP_RESET);
       } else {
           wlan_mlme_connection_reset(sm->vap_handle);
       }
    }
    if (sm->scan_entry) {
        wlan_scan_entry_remove_reference(sm->scan_entry);
        sm->scan_entry=NULL;
    }
    if (sm->is_stop_requested ) {
    ieee80211_send_event(sm, WLAN_ASSOC_SM_EVENT_DISCONNECT, 0);
    } else {
    ieee80211_send_event(sm, WLAN_ASSOC_SM_EVENT_FAILED, sm->last_failure);
    }
    sm->cur_auth_attempts = 0; 
    sm->cur_assoc_attempts = 0; 
    sm->last_reason = 0; 
    sm->last_failure = 0; 
    sm->is_bcn_recvd = 0; 
    sm->is_stop_requested = 0;
    sm->sync_stop_requested=0;
    sm->is_running = 0;

    /* Cisco AP workaround */
    sm->last_connected_time = 0;
    sm->cur_rejoin_attempts = 0;    
}

static void ieee80211_assoc_state_init_exit(void *ctx) 
{
    /* NONE */
}

static bool ieee80211_assoc_state_init_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    switch(event) {
    case IEEE80211_ASSOC_EVENT_CONNECT_REQUEST:
    case IEEE80211_ASSOC_EVENT_REASSOC_REQUEST:
        ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_JOIN); 
        return true;
        break;
    case IEEE80211_ASSOC_EVENT_DISCONNECT_REQUEST:
    case IEEE80211_ASSOC_EVENT_DISASSOC_REQUEST:
        return true;

    default:
        return false;
    }
}

/*
 *JOIN 
 */
static void ieee80211_assoc_state_join_entry(void *ctx) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    sm->is_join=1;
    if (wlan_mlme_join_infra(sm->vap_handle, sm->scan_entry, sm->max_tsfsync_time) != 0 ) {
        IEEE80211_DPRINTF(sm->vap_handle,IEEE80211_MSG_STATE,"%s: join_infra failed \n",__func__);
        ieee80211_sm_dispatch(sm->hsm_handle,IEEE80211_ASSOC_EVENT_JOIN_FAIL,0,NULL);
        return;
    }
}

static void ieee80211_assoc_state_join_exit(void *ctx) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    wlan_mlme_cancel(sm->vap_handle);/* cancel any pending mlme join req */
}

static bool ieee80211_assoc_state_join_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    switch(event) {
    case IEEE80211_ASSOC_EVENT_JOIN_SUCCESS:
        if (wlan_scan_entry_assoc_state(sm->scan_entry) >= AP_ASSOC_STATE_AUTH) {
            ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_ASSOC); 
        } else {
            ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_AUTH); 
        }
        return true;
        break;

    case IEEE80211_ASSOC_EVENT_BEACON_WAIT_TIMEOUT:
    case IEEE80211_ASSOC_EVENT_BEACON_MISS:
    case IEEE80211_ASSOC_EVENT_JOIN_FAIL:
    case IEEE80211_ASSOC_EVENT_DISCONNECT_REQUEST:
    case IEEE80211_ASSOC_EVENT_DISASSOC_REQUEST:
       /* cancel pending mlme operation */
        wlan_mlme_cancel(sm->vap_handle);
        if (wlan_mlme_operation_in_progress(sm->vap_handle)) {
            ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_MLME_WAIT); 
        } else {
            ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_INIT); 
        }
        return true;
        break;

    default:
        return false;
    }
}

/*
 * AUTH 
 */
static void ieee80211_assoc_state_auth_entry(void *ctx) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    ++sm->cur_auth_attempts; 

    if (wlan_mlme_auth_request(sm->vap_handle,sm->max_mgmt_time) !=0 ) {
        IEEE80211_DPRINTF(sm->vap_handle,IEEE80211_MSG_STATE,"%s: auth_request failed retrying ...\n",__func__);
        sm->timeout_event = IEEE80211_ASSOC_EVENT_TIMEOUT, 
        OS_SET_TIMER(&sm->sm_timer,AUTH_RETRY_TIME);
        return;
    }
}

static void ieee80211_assoc_state_auth_exit(void *ctx) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    wlan_mlme_cancel(sm->vap_handle); /* cancel any pending mlme auth req*/
    OS_CANCEL_TIMER(&sm->sm_timer);
}

static bool ieee80211_assoc_state_auth_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;

    switch(event) {

    case IEEE80211_ASSOC_EVENT_AUTH_SUCCESS:
        ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_ASSOC); 
        return true;
        break;

    case IEEE80211_ASSOC_EVENT_AUTH_FAIL:
    case IEEE80211_ASSOC_EVENT_TIMEOUT:
        sm->last_failure = WLAN_ASSOC_SM_REASON_AUTH_FAILED;
        if (sm->cur_auth_attempts < sm->max_auth_attempts) {
            ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_AUTH); 
            return true;
            break;
        }

        IEEE80211_DPRINTF(sm->vap_handle,IEEE80211_MSG_STATE,"%s: max auth attempts reached \n",__func__);
        if (sm->scan_entry) {
               wlan_scan_entry_set_assoc_state(sm->scan_entry, AP_ASSOC_STATE_NONE);
        }
        /* fall thru */
       
    case IEEE80211_ASSOC_EVENT_DISCONNECT_REQUEST:
    case IEEE80211_ASSOC_EVENT_DISASSOC_REQUEST:
       /* cancel pending mlme operation */
        wlan_mlme_cancel(sm->vap_handle);
        if (wlan_mlme_operation_in_progress(sm->vap_handle)) {
            ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_MLME_WAIT); 
        } else {
            ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_INIT); 
        }
        return true;
        break;

    case IEEE80211_ASSOC_EVENT_DEAUTH:
        ieee80211_send_event(sm, WLAN_ASSOC_SM_EVENT_REJOINING, WLAN_ASSOC_SM_REASON_DEAUTH);
        ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_AUTH); 
        return true;
        break;

    default:
        return false;
        
    }
}

/*
 * ASSOC
 */
static void ieee80211_assoc_state_assoc_entry(void *ctx) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    ++sm->cur_assoc_attempts; 
    if (wlan_scan_entry_assoc_state(sm->scan_entry) == AP_ASSOC_STATE_ASSOC) {
        if (wlan_mlme_reassoc_request(sm->vap_handle,sm->prev_bssid, sm->max_mgmt_time) !=0 ) {
            IEEE80211_DPRINTF(sm->vap_handle,IEEE80211_MSG_STATE,"%s: reassoc request failed retrying ...\n",__func__);
            sm->timeout_event = IEEE80211_ASSOC_EVENT_TIMEOUT, 
            OS_SET_TIMER(&sm->sm_timer,ASSOC_RETRY_TIME);
            return;
        }
    } else {
        if (wlan_mlme_assoc_request(sm->vap_handle, sm->max_mgmt_time) !=0 ) {
            IEEE80211_DPRINTF(sm->vap_handle,IEEE80211_MSG_STATE,"%s: assoc request failed retrying ...\n",__func__);
            sm->timeout_event = IEEE80211_ASSOC_EVENT_TIMEOUT, 
            OS_SET_TIMER(&sm->sm_timer,ASSOC_RETRY_TIME);
            return;
        }
    }
}

static void ieee80211_assoc_state_assoc_exit(void *ctx) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    wlan_mlme_cancel(sm->vap_handle); /* cancel any pending mlme assoc req */
    OS_CANCEL_TIMER(&sm->sm_timer);
}

static bool ieee80211_assoc_state_assoc_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;

    switch(event) {
    case IEEE80211_ASSOC_EVENT_ASSOC_SUCCESS:
        sm->last_connected_time = OS_GET_TIMESTAMP();
        ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_RUN); 
        return true;
        break;

    case IEEE80211_ASSOC_EVENT_ASSOC_FAIL:
    case IEEE80211_ASSOC_EVENT_TIMEOUT:
        if (sm->cur_assoc_attempts >= sm->max_assoc_attempts) {
            IEEE80211_DPRINTF(sm->vap_handle,IEEE80211_MSG_STATE,"%s: max assoc attempts reached \n",__func__);
            if (sm->scan_entry) wlan_scan_entry_set_assoc_state(sm->scan_entry, AP_ASSOC_STATE_NONE);
            ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_INIT); 
            return true;
        }
        sm->last_failure = WLAN_ASSOC_SM_REASON_ASSOC_FAILED;
        ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_ASSOC); 
        return true;
        break;

    case IEEE80211_ASSOC_EVENT_DISCONNECT_REQUEST:
    case IEEE80211_ASSOC_EVENT_DISASSOC_REQUEST:
        if (sm->scan_entry) wlan_scan_entry_set_assoc_state(sm->scan_entry, AP_ASSOC_STATE_NONE);
        /* cancel pending mlme operation */
        wlan_mlme_cancel(sm->vap_handle);
        if (wlan_mlme_operation_in_progress(sm->vap_handle)) {
            ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_MLME_WAIT); 
        } else {
            ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_INIT); 
        }
        break;

    case IEEE80211_ASSOC_EVENT_DEAUTH:
        ieee80211_send_event(sm, WLAN_ASSOC_SM_EVENT_REJOINING, WLAN_ASSOC_SM_REASON_DEAUTH);
        ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_AUTH); 
        return true;
        break;

    default:
        return false;
    }
    return false;
}

/*
 *RUN 
 */
static void ieee80211_assoc_state_run_entry(void *ctx) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    wlan_assoc_sm_event sm_event;

    wlan_mlme_connection_up(sm->vap_handle);
    sm_event.event = WLAN_ASSOC_SM_EVENT_SUCCESS;
    sm_event.reason = WLAN_ASSOC_SM_REASON_NONE;
    sm_event.reasoncode = 0;
    (* sm->sm_evhandler) (sm, sm->osif, &sm_event);
}

static void ieee80211_assoc_state_run_exit(void *ctx) 
{
    u_int8_t  bssid[IEEE80211_ADDR_LEN];
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    wlan_mlme_connection_down(sm->vap_handle);
    sm->cur_auth_attempts = 0; 
    sm->cur_assoc_attempts = 0; 
    sm->last_reason = 0; 
    sm->is_bcn_recvd = 0; 
   /* delete any unicast key installed */
   wlan_vap_get_bssid(sm->vap_handle,bssid);
   wlan_del_key(sm->vap_handle,IEEE80211_KEYIX_NONE,bssid); 

    
}

static bool ieee80211_assoc_state_run_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    wlan_if_t vap = sm->vap_handle;
    u_int32_t time_elapsed;

    switch(event) {
    case IEEE80211_ASSOC_EVENT_BEACON_MISS:
        sm->last_failure = WLAN_ASSOC_SM_REASON_BEACON_MISS;    /* beacon miss */ 
        ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_INIT); 
        return true;
        break;
    case IEEE80211_ASSOC_EVENT_DISASSOC:
        time_elapsed = CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP() - sm->last_connected_time);
        if ((sm->last_reason == IEEE80211_REASON_AUTH_EXPIRE || sm->last_reason == IEEE80211_REASON_ASSOC_EXPIRE) &&
            (vap->auto_assoc)) {
            ieee80211_send_event(sm, WLAN_ASSOC_SM_EVENT_REJOINING, WLAN_ASSOC_SM_REASON_DISASSOC);
            ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_JOIN); 
#if ATH_SUPPORT_WPA_SUPPLICANT_CHECK_TIME
        } else if(sm->cur_rejoin_attempts < vap->iv_rejoint_attemp_time ) {
            sm->cur_rejoin_attempts++;
            ieee80211_send_event(sm, WLAN_ASSOC_SM_EVENT_REJOINING, WLAN_ASSOC_SM_REASON_DISASSOC);
            ieee80211_sm_transition_to(sm->hsm_handle, IEEE80211_ASSOC_STATE_JOIN); 
        } else {
#else
        } else if(sm->cur_rejoin_attempts < REJOIN_ATTEMP_TIME && time_elapsed < REJOIN_CHECKING_TIME) {
            sm->cur_rejoin_attempts++;
            ieee80211_send_event(sm, WLAN_ASSOC_SM_EVENT_REJOINING, WLAN_ASSOC_SM_REASON_DISASSOC);
            ieee80211_sm_transition_to(sm->hsm_handle, IEEE80211_ASSOC_STATE_JOIN); 
        } else {
#endif 
            ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_INIT); 
        }
        return true;
        break;

    case IEEE80211_ASSOC_EVENT_DEAUTH:
        time_elapsed = CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP() - sm->last_connected_time);
        if ((sm->last_reason == IEEE80211_REASON_AUTH_EXPIRE || sm->last_reason == IEEE80211_REASON_ASSOC_EXPIRE) &&
            (vap->auto_assoc)) {
            ieee80211_send_event(sm, WLAN_ASSOC_SM_EVENT_REJOINING, WLAN_ASSOC_SM_REASON_DEAUTH);
           ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_JOIN); 
#if ATH_SUPPORT_WPA_SUPPLICANT_CHECK_TIME
        } else if(sm->cur_rejoin_attempts < vap->iv_rejoint_attemp_time ) {
            sm->cur_rejoin_attempts++;
            ieee80211_send_event(sm, WLAN_ASSOC_SM_EVENT_REJOINING, WLAN_ASSOC_SM_REASON_DEAUTH);
            ieee80211_sm_transition_to(sm->hsm_handle, IEEE80211_ASSOC_STATE_JOIN);
        } else {
#else
        } else if(sm->cur_rejoin_attempts < REJOIN_ATTEMP_TIME && time_elapsed < REJOIN_CHECKING_TIME) {
            sm->cur_rejoin_attempts++;
            ieee80211_send_event(sm, WLAN_ASSOC_SM_EVENT_REJOINING, WLAN_ASSOC_SM_REASON_DEAUTH);
            ieee80211_sm_transition_to(sm->hsm_handle, IEEE80211_ASSOC_STATE_JOIN);
        } else {
#endif
            ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_INIT); 
        }
        return true;
        break;

    case IEEE80211_ASSOC_EVENT_DISCONNECT_REQUEST:
        /* no need to send disassoc usully caaled while romaing */
        ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_INIT); 
        return true;
        break;
    case IEEE80211_ASSOC_EVENT_DISASSOC_REQUEST:
        ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_DISASSOC); 
        return true;
        break;

    case IEEE80211_ASSOC_EVENT_RECONNECT_REQUEST:
        ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_JOIN); 
        return true;
        break;

    default:
        return false;
    }
}

/* Callback function for the Completion of Disassoc request frame. */
static void
tx_disassoc_req_completion(wlan_if_t vaphandle, wbuf_t wbuf, void *arg, 
                           u_int8_t *dst_addr, u_int8_t *src_addr, 
                           u_int8_t *bssid, ieee80211_xmit_status *ts)
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) arg;

    IEEE80211_DPRINTF(sm->vap_handle,IEEE80211_MSG_STATE,"%s: Tx disassoc. status=%d\n",
                      __func__, ts->ts_flags);

    ieee80211_sm_dispatch(sm->hsm_handle, IEEE80211_ASSOC_EVENT_DISASSOC_SENT,0,NULL); 
}

/*
 *DISASSOC 
 */
static void ieee80211_assoc_state_disassoc_entry(void *ctx) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    u_int8_t          cur_bssid[IEEE80211_ADDR_LEN];
    wlan_vap_get_bssid(sm->vap_handle, cur_bssid);

    if (wlan_mlme_disassoc_request_with_callback(sm->vap_handle, cur_bssid, 
                                                 IEEE80211_REASON_ASSOC_LEAVE,
                                                 tx_disassoc_req_completion, sm) !=0 ) {
        IEEE80211_DPRINTF(sm->vap_handle,IEEE80211_MSG_STATE,"%s: ignore send disassoc failure \n",__func__);
    }

    if (sm->scan_entry) wlan_scan_entry_set_assoc_state(sm->scan_entry, AP_ASSOC_STATE_NONE);
    sm->timeout_event = IEEE80211_ASSOC_EVENT_TIMEOUT; 
    if (sm->sync_stop_requested) {
        /* wait for some time for disassoc frame to go out */
        OS_SLEEP(DISASSOC_WAIT_TIME*1000);
        ieee80211_sm_dispatch(sm->hsm_handle, sm->timeout_event,0,NULL); 
    } else {
        OS_SET_TIMER(&sm->sm_timer,sm->max_mgmt_time);
    }
}

static void ieee80211_assoc_state_disassoc_exit(void *ctx) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    OS_CANCEL_TIMER(&sm->sm_timer);
}

static bool ieee80211_assoc_state_disassoc_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    switch(event) {
    case IEEE80211_ASSOC_EVENT_DISASSOC_SENT:
    case IEEE80211_ASSOC_EVENT_TIMEOUT:
        ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_INIT); 
        return true;
        break;

    default:
        return false;
    }
}

/*
 * MLME_WAIT 
 */
static void ieee80211_assoc_state_mlme_wait_entry(void *ctx) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    sm->timeout_event = IEEE80211_ASSOC_EVENT_TIMEOUT; 
    OS_SET_TIMER(&sm->sm_timer,MLME_OP_CHECK_TIME);

}

static bool ieee80211_assoc_state_mlme_wait_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data) 
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) ctx;
    switch(event) {
    case IEEE80211_ASSOC_EVENT_TIMEOUT:
        if (wlan_mlme_operation_in_progress(sm->vap_handle)) {
            printk("%s: waiting for mlme cancel to complete \n",__func__);
            OS_SET_TIMER(&sm->sm_timer,MLME_OP_CHECK_TIME);
        } else {
            ieee80211_sm_transition_to(sm->hsm_handle,IEEE80211_ASSOC_STATE_INIT); 
        }
        return true;
        break;
    default:
        return false;

    }

}

ieee80211_state_info ieee80211_assoc_sm_info[] = {
    { 
        (u_int8_t) IEEE80211_ASSOC_STATE_INIT, 
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "INIT",
        ieee80211_assoc_state_init_entry,
        ieee80211_assoc_state_init_exit,
        ieee80211_assoc_state_init_event
    },
    { 
        (u_int8_t) IEEE80211_ASSOC_STATE_JOIN, 
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "JOIN",
        ieee80211_assoc_state_join_entry,
        ieee80211_assoc_state_join_exit,
        ieee80211_assoc_state_join_event
    },
    { 
        (u_int8_t) IEEE80211_ASSOC_STATE_AUTH, 
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "AUTH",
        ieee80211_assoc_state_auth_entry,
        ieee80211_assoc_state_auth_exit,
        ieee80211_assoc_state_auth_event
    },
    { 
        (u_int8_t) IEEE80211_ASSOC_STATE_ASSOC, 
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "ASSOC",
        ieee80211_assoc_state_assoc_entry,
        ieee80211_assoc_state_assoc_exit,
        ieee80211_assoc_state_assoc_event
    },
    { 
        (u_int8_t) IEEE80211_ASSOC_STATE_RUN, 
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "RUN",
        ieee80211_assoc_state_run_entry,
        ieee80211_assoc_state_run_exit,
        ieee80211_assoc_state_run_event
    },
    { 
        (u_int8_t) IEEE80211_ASSOC_STATE_DISASSOC, 
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "DISASSOC",
        ieee80211_assoc_state_disassoc_entry,
        ieee80211_assoc_state_disassoc_exit,
        ieee80211_assoc_state_disassoc_event
    },
    { 
        (u_int8_t) IEEE80211_ASSOC_STATE_MLME_WAIT, 
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "MLME_WAIT",
        ieee80211_assoc_state_mlme_wait_entry,
        NULL,
        ieee80211_assoc_state_mlme_wait_event,
    },
};

static const char *assoc_event_names[] = {      "CONNECT_REQUEST",
                                                "DISCONNECT_REQUEST",
                                                "DISASSOC_REQUEST",
                                                "REASSOC_REQUEST",
                                                "JOIN_SUCCESS",
                                                "JOIN_FAIL",
                                                "AUTH_SUCCESS",
                                                "AUTH_FAIL",
                                                "ASSOC_FAIL",
                                                "ASSOC_SUCCESS",
                                                "BEACON_WAIT_TIMEOUT",
                                                "BEACON_MISS",
                                                "DISASSOC",
                                                "DEAUTH",
                                                "DISASSOC_SENT",
                                                "TIMEOUT",
                                                "RECONNECT_REQUEST"
                                              };
static OS_TIMER_FUNC(assoc_sm_timer_handler)
{
    wlan_assoc_sm_t sm;

    OS_GET_TIMER_ARG(sm, wlan_assoc_sm_t);
    IEEE80211_DPRINTF(sm->vap_handle,IEEE80211_MSG_STATE,"%s: timed out cur state %s \n",
                      __func__, ieee80211_assoc_sm_info[ieee80211_sm_get_curstate(sm->hsm_handle)].name);
    ieee80211_sm_dispatch(sm->hsm_handle, sm->timeout_event,0,NULL); 

}

/*
 * mlme event handlers.
 */

static void sm_join_complete(os_handle_t osif, IEEE80211_STATUS status)
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) osif;
    sm->last_reason=status;
    if (status == IEEE80211_STATUS_SUCCESS) {
        ieee80211_sm_dispatch(sm->hsm_handle,IEEE80211_ASSOC_EVENT_JOIN_SUCCESS,0,NULL);
    } else {
        ieee80211_sm_dispatch(sm->hsm_handle,IEEE80211_ASSOC_EVENT_JOIN_FAIL,0,NULL);
    }
}

static void sm_auth_complete(os_handle_t osif, IEEE80211_STATUS status)
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) osif;
    sm->last_reason=status;
    if (status == IEEE80211_STATUS_SUCCESS) {
        if (sm->scan_entry) wlan_scan_entry_set_assoc_state(sm->scan_entry, AP_ASSOC_STATE_AUTH);
        ieee80211_sm_dispatch(sm->hsm_handle,IEEE80211_ASSOC_EVENT_AUTH_SUCCESS,0,NULL);
    } else {
        ieee80211_sm_dispatch(sm->hsm_handle,IEEE80211_ASSOC_EVENT_AUTH_FAIL,0,NULL);
    }
}

static void sm_assoc_complete(os_handle_t osif, IEEE80211_STATUS status,u_int16_t aid, wbuf_t wbuf)
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) osif;
    sm->last_reason=status;
    if (status == IEEE80211_STATUS_SUCCESS) {
        if (sm->scan_entry) wlan_scan_entry_set_assoc_state(sm->scan_entry, AP_ASSOC_STATE_ASSOC);
        ieee80211_sm_dispatch(sm->hsm_handle,IEEE80211_ASSOC_EVENT_ASSOC_SUCCESS,0,NULL);
    } else {
        ieee80211_sm_dispatch(sm->hsm_handle,IEEE80211_ASSOC_EVENT_ASSOC_FAIL,0,NULL);
    }

}

static void sm_reassoc_complete(os_handle_t osif, IEEE80211_STATUS status,u_int16_t aid, wbuf_t wbufs)
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) osif;
    sm->last_reason=status;
    if (status == IEEE80211_STATUS_SUCCESS) {
        if (sm->scan_entry) wlan_scan_entry_set_assoc_state(sm->scan_entry, AP_ASSOC_STATE_ASSOC);
        ieee80211_sm_dispatch(sm->hsm_handle,IEEE80211_ASSOC_EVENT_ASSOC_SUCCESS,0,NULL);
    } else {
        ieee80211_sm_dispatch(sm->hsm_handle,IEEE80211_ASSOC_EVENT_ASSOC_FAIL,0,NULL);
    }

}

static void sm_deauth_indication(os_handle_t osif,u_int8_t *macaddr, u_int16_t reason)
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) osif;
    sm->last_reason = reason;
    if (sm->scan_entry) wlan_scan_entry_set_assoc_state(sm->scan_entry, AP_ASSOC_STATE_NONE); // we need to auth
    ieee80211_sm_dispatch(sm->hsm_handle,IEEE80211_ASSOC_EVENT_DEAUTH,0,NULL);
}

static void sm_disassoc_indication(os_handle_t osif,u_int8_t *macaddr, u_int32_t reason)
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) osif;
    sm->last_reason = reason;
    /*
     * In theory we should still be auth'ed
     * But many APs send disassoc when they mean deauth
     * so just start again to save an extra deauth
     */
    if (sm->scan_entry) wlan_scan_entry_set_assoc_state(sm->scan_entry, AP_ASSOC_STATE_NONE);
    ieee80211_sm_dispatch(sm->hsm_handle,IEEE80211_ASSOC_EVENT_DISASSOC,0,NULL);
}

wlan_mlme_event_handler_table sta_mlme_evt_handler = {
    sm_join_complete,
    NULL,
    sm_auth_complete,
    NULL,
    sm_assoc_complete,
    sm_reassoc_complete,
    NULL,
    NULL,
    NULL,
    sm_deauth_indication,
    NULL,
    NULL,
    sm_disassoc_indication,
};

/*
 * misc event handlers
 */
static void sm_beacon_miss(os_handle_t osif)
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) osif;
    ieee80211_sm_dispatch(sm->hsm_handle,IEEE80211_ASSOC_EVENT_BEACON_MISS,0,NULL);
}

static void sm_clonemac(os_handle_t osif)
{
    wlan_assoc_sm_t sm = (wlan_assoc_sm_t) osif;
    ieee80211_sm_dispatch(sm->hsm_handle,IEEE80211_ASSOC_EVENT_RECONNECT_REQUEST,0,NULL);
}


static wlan_misc_event_handler_table sta_misc_evt_handler = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    sm_beacon_miss,
    NULL,                               /* wlan_beacon_rssi_indication */
    NULL,
    sm_clonemac,
    NULL,                               /* wlan_sta_scan_entry_update */
    NULL,                               /* wlan_ap_stopped */
#if ATH_SUPPORT_WAPI
    NULL,                               /* wlan_sta_rekey_indication */
#endif
};

wlan_assoc_sm_t wlan_assoc_sm_create(osdev_t oshandle, wlan_if_t vaphandle)
{
    wlan_assoc_sm_t sm;
    sm = (wlan_assoc_sm_t) OS_MALLOC(oshandle,sizeof(struct _wlan_assoc_sm),0);
    if (!sm) {
        return NULL;
    }
    OS_MEMZERO(sm, sizeof(struct _wlan_assoc_sm));
    sm->os_handle = oshandle;
    sm->vap_handle = vaphandle;
    sm->hsm_handle = ieee80211_sm_create(oshandle, 
                                         "assoc",
                                         (void *) sm, 
                                         IEEE80211_ASSOC_STATE_INIT,
                                         ieee80211_assoc_sm_info, 
                                         sizeof(ieee80211_assoc_sm_info)/sizeof(ieee80211_state_info),
                                         MAX_QUEUED_EVENTS, 
                                         0 /* no event data */, 
                                         MESGQ_PRIORITY_HIGH,
                                         IEEE80211_HSM_ASYNCHRONOUS, /* run the SM asynchronously */
                                         ieee80211_assoc_sm_debug_print,
                                         assoc_event_names,
                                         IEEE80211_N(assoc_event_names)
                                         );
    if (!sm->hsm_handle) {
        OS_FREE(sm);
        IEEE80211_DPRINTF(vaphandle, IEEE80211_MSG_STATE,
            "%s : ieee80211_sm_create failed\n", __func__);
        return NULL;
    }

    sm->max_assoc_attempts = MAX_ASSOC_ATTEMPTS;
    sm->max_auth_attempts =  MAX_AUTH_ATTEMPTS;
    sm->max_tsfsync_time =  MAX_TSFSYNC_TIME;
    sm->max_mgmt_time =  MAX_MGMT_TIME;
    OS_INIT_TIMER(oshandle, &(sm->sm_timer), assoc_sm_timer_handler, (void *)sm);
       
    wlan_vap_register_mlme_event_handlers(vaphandle,(os_if_t) sm, &sta_mlme_evt_handler);
    wlan_vap_register_misc_event_handlers(vaphandle,(os_if_t)sm,&sta_misc_evt_handler);

    return sm;                                   
}



void  wlan_assoc_sm_delete(wlan_assoc_sm_t smhandle)
{
    if (smhandle->is_running) {
        IEEE80211_DPRINTF(smhandle->vap_handle,IEEE80211_MSG_STATE,"%s : can not delete while still runing \n", __func__); 
    }
    OS_CANCEL_TIMER(&(smhandle->sm_timer));
    OS_FREE_TIMER(&(smhandle->sm_timer));
    if (wlan_vap_unregister_misc_event_handlers(smhandle->vap_handle,(os_if_t)smhandle,&sta_misc_evt_handler)) {
        IEEE80211_DPRINTF(smhandle->vap_handle,IEEE80211_MSG_STATE,"%s : unregister nusc evt handler failed \n", __func__); 
    }
    if (wlan_vap_unregister_mlme_event_handlers(smhandle->vap_handle,(os_if_t)smhandle,&sta_mlme_evt_handler)) {
        IEEE80211_DPRINTF(smhandle->vap_handle,IEEE80211_MSG_STATE,"%s : unregister mlme evt handler failed \n", __func__); 
    }
    ieee80211_sm_delete(smhandle->hsm_handle);
    OS_FREE(smhandle);
}

void wlan_assoc_sm_register_event_handlers(wlan_assoc_sm_t smhandle, os_if_t osif,
                                            wlan_assoc_sm_event_handler sm_evhandler)
{
    smhandle->osif = osif;
    smhandle->sm_evhandler = sm_evhandler;
    
}

/*
 * start the state machine and handling the events.
 */
int wlan_assoc_sm_start(wlan_assoc_sm_t smhandle, wlan_scan_entry_t scan_entry, u_int8_t *curbssid)
{
    if (scan_entry == NULL) {
        return -EINVAL;
    }
    if ( smhandle->is_running ) {
        IEEE80211_DPRINTF(smhandle->vap_handle,IEEE80211_MSG_STATE,"%s: association SM is already running!!  \n", __func__);
        return -EINPROGRESS;
    }
  
    /* mark it as running */
    smhandle->is_running = 1;
    if (curbssid) {
           IEEE80211_ADDR_COPY(smhandle->prev_bssid,curbssid);
    }

    /* Reset HSM to INIT state. */
    ieee80211_sm_reset(smhandle->hsm_handle, IEEE80211_ASSOC_STATE_INIT, NULL);

    smhandle->scan_entry = scan_entry;
    wlan_scan_entry_add_reference(scan_entry);
    if (wlan_scan_entry_assoc_state(smhandle->scan_entry) == AP_ASSOC_STATE_ASSOC) {
        ieee80211_sm_dispatch(smhandle->hsm_handle,
                              IEEE80211_ASSOC_EVENT_REASSOC_REQUEST,0,NULL); 
    } else {
        ieee80211_sm_dispatch(smhandle->hsm_handle,
                              IEEE80211_ASSOC_EVENT_CONNECT_REQUEST,0,NULL); 
    }
    return EOK;
}

/*
 * stop handling the events.
 */
int wlan_assoc_sm_stop(wlan_assoc_sm_t smhandle, u_int32_t flags)
{
    /*
     * return an error if it is already stopped (or)
     * there is a stop request is pending.
     */
    if (!smhandle->is_running ) {
        IEEE80211_DPRINTF(smhandle->vap_handle,IEEE80211_MSG_STATE,"%s: association SM is already stopped  !!  \n",__func__);
        return -EALREADY;
    }
    if (smhandle->is_stop_requested) {
        IEEE80211_DPRINTF(smhandle->vap_handle,IEEE80211_MSG_STATE,"%s: association SM is already being stopped !!  \n",__func__);
        return -EALREADY;
    }
    smhandle->is_stop_requested = 1;
    if (flags & IEEE80211_ASSOC_SM_STOP_SYNC) {
        smhandle->sync_stop_requested=1;
    }
    if (flags & IEEE80211_ASSOC_SM_STOP_DISASSOC) {
        if (smhandle->sync_stop_requested) {
            ieee80211_sm_dispatch_sync(smhandle->hsm_handle,
                                       IEEE80211_ASSOC_EVENT_DISASSOC_REQUEST,0,NULL,true); 
        } else {
            ieee80211_sm_dispatch(smhandle->hsm_handle,
                                  IEEE80211_ASSOC_EVENT_DISASSOC_REQUEST,0,NULL); 
        }
        
    } else {
        if (smhandle->sync_stop_requested) {
            ieee80211_sm_dispatch_sync(smhandle->hsm_handle,
                                       IEEE80211_ASSOC_EVENT_DISCONNECT_REQUEST,0,NULL,true); 
        } else {
            ieee80211_sm_dispatch(smhandle->hsm_handle,
                                  IEEE80211_ASSOC_EVENT_DISCONNECT_REQUEST,0,NULL); 
        }
    }
    return EOK;
}

/*
 * set a assoc ame param.
 */
int wlan_assoc_sm_set_param(wlan_assoc_sm_t smhandle, wlan_assoc_sm_param param,u_int32_t val)
{

    switch(param) {
    case PARAM_MAX_ASSOC_ATTEMPTS:
        smhandle->max_assoc_attempts = (u_int8_t)val;
        if (val == 0) return -EINVAL;
        break;
    case PARAM_MAX_AUTH_ATTEMPTS:
        smhandle->max_auth_attempts = (u_int8_t)val;
        if (val == 0) return -EINVAL;
        break;
    case PARAM_MAX_TSFSYNC_TIME:
        smhandle->max_tsfsync_time = (u_int16_t)val;
        if (val == 0) return -EINVAL;
        break;
    case PARAM_MAX_MGMT_TIME:
        smhandle->max_mgmt_time = (u_int16_t)val;
        if (val == 0) return -EINVAL;
        break;
    case PARAM_CURRENT_STATE:
    return -EINVAL;
        break;
    }

    return EOK;
}

/*
 * get an assoc ame param.
 */
u_int32_t wlan_assoc_sm_get_param(wlan_assoc_sm_t smhandle, wlan_assoc_sm_param param)
{
    u_int32_t val=0;

    switch(param) {
    case PARAM_MAX_ASSOC_ATTEMPTS:
        val = smhandle->max_assoc_attempts;
        break;
    case PARAM_MAX_AUTH_ATTEMPTS:
        val = smhandle->max_auth_attempts;
        break;
    case PARAM_MAX_TSFSYNC_TIME:
        val = smhandle->max_tsfsync_time;
        break;
    case PARAM_MAX_MGMT_TIME:
        val = smhandle->max_mgmt_time;
        break;
    case PARAM_CURRENT_STATE:
        switch((ieee80211_connection_state)ieee80211_sm_get_curstate(smhandle->hsm_handle)) {
        case IEEE80211_ASSOC_STATE_INIT: 
            val=WLAN_ASSOC_STATE_INIT;
            break;
        case IEEE80211_ASSOC_STATE_JOIN: 
        case IEEE80211_ASSOC_STATE_AUTH: 
        case IEEE80211_ASSOC_STATE_MLME_WAIT: 
            val=WLAN_ASSOC_STATE_AUTH;
            break;
        case IEEE80211_ASSOC_STATE_ASSOC: 
            val=WLAN_ASSOC_STATE_ASSOC;
            break;
        case IEEE80211_ASSOC_STATE_RUN: 
            val=WLAN_ASSOC_STATE_RUN;
            break;
        case IEEE80211_ASSOC_STATE_DISASSOC: 
            val=WLAN_ASSOC_STATE_RUN;
            break;
        }
        break;
    }

    return val;
}

/*
 * go back to initial state to start new associaiton.
 */
int wlan_assoc_sm_restart(wlan_assoc_sm_t smhandle, u_int32_t flags)
{
    /*
     * return an error if it is already stopped (or)
     * there is a stop request is pending.
     */
    if (!smhandle->is_running ) {
        IEEE80211_DPRINTF(smhandle->vap_handle,IEEE80211_MSG_STATE,"%s: association SM is already stopped  !!  \n",__func__);
        return -EALREADY;
    }
    if (smhandle->is_stop_requested) {
        IEEE80211_DPRINTF(smhandle->vap_handle,IEEE80211_MSG_STATE,"%s: association SM is already being stopped !!  \n",__func__);
        return -EALREADY;
    }
    smhandle->is_stop_requested = 0;
    if (flags & IEEE80211_ASSOC_SM_STOP_SYNC) {
        smhandle->sync_stop_requested=1;
        ieee80211_sm_dispatch_sync(smhandle->hsm_handle,
                                   IEEE80211_ASSOC_EVENT_DISCONNECT_REQUEST,0,NULL,true); 
    } else {
        ieee80211_sm_dispatch(smhandle->hsm_handle,
                              IEEE80211_ASSOC_EVENT_DISCONNECT_REQUEST,0,NULL); 
    }
    return EOK;
}

void wlan_assoc_set_scan_buf(wlan_assoc_sm_t smhandle, wlan_scan_entry_t scan_result)
{
    smhandle->scan_entry = scan_result;
}
