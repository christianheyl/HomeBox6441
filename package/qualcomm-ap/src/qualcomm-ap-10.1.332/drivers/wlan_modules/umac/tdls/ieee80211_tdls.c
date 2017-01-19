/*
 *  Copyright (c) 2005 Atheros Communications Inc.  All rights reserved.
 */

#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

#include <ieee80211_var.h>
#include <ieee80211.h>
#include "ieee80211_wds.h"
#include "ieee80211_rateset.h" /* for rateset vars */

#if UMAC_SUPPORT_TDLS 
#include <osif_private.h>   /* for wireless_send_event */
#include <ieee80211_sm.h>
#include <ieee80211_tdls_priv.h>
#include <ieee80211_tdls_power.h>
#include <ieee80211_tdls_chan_switch.h>

#include <if_athvar.h> /* for definition of ATH_NODE_NET80211(ni) */

#define MAX_QUEUED_EVENTS  16
#define LINK_MONITOR_ABNORMAL_TIMEOUT_MSEC  5000
#define LINK_MONITOR_TX_ERROR_THRESHOLD  5

#define WRITE_TDLS_LENGTH_FIELD(th, _len)  \
                        ((struct ieee80211_tdls_frame *)th)->lnk_id.len = _len;
#define IEEE80211_ADDSHORT(frm, v)  do {frm[0] = (v) & 0xff; frm[1] = (v) >> 8; frm += 2;} while (0)
#define IEEE80211_ADDSELECTOR(frm, sel) do {OS_MEMCPY(frm, sel, 4); frm += 4;} while (0)
#define IEEE80211_SM(_v, _f)    (((_v) << _f##_S) & _f)

/* taken as reference from os/linux/src/osif_umac.c */
#define OSIF_TO_NETDEV(_osif) (((osif_dev *)(_osif))->netdev)
#define ieee80211_tdls_get_dev(vap) OSIF_TO_NETDEV((struct net_device *) \
            wlan_vap_get_registered_handle(vap))
            
/* TDLS message type packet length */
int frame_length[] = { 
    /* IEEE80211_TDLS_SETUP_REQ */
    IEEE80211_TDLS_FRAME_MAXLEN + IEEE80211_TDLS_DIALOG_TKN_SIZE,
    /* IEEE80211_TDLS_SETUP_RESP */
    IEEE80211_TDLS_FRAME_MAXLEN + IEEE80211_TDLS_DIALOG_TKN_SIZE + 2,
    /* IEEE80211_TDLS_SETUP_CONFIRM */
    /* XXX: ?? allocate more memory to fill SMK */
    IEEE80211_TDLS_FRAME_MAXLEN + IEEE80211_TDLS_DIALOG_TKN_SIZE,
    /* IEEE80211_TDLS_TEARDOWN */
    IEEE80211_TDLS_FRAME_MAXLEN + IEEE80211_TDLS_DIALOG_TKN_SIZE,
    /* IEEE80211_TDLS_PEER_TRAFFIC_INDICATION */
    IEEE80211_TDLS_FRAME_MAXLEN + IEEE80211_TDLS_DIALOG_TKN_SIZE,
    /* IEEE80211_TDLS_CHANNEL_SX_REQ */ 
    IEEE80211_TDLS_CHANNEL_SX_REQ_SIZE,
    /* IEEE80211_TDLS_CHANNEL_SX_RESP */
    IEEE80211_TDLS_CHANNEL_SX_RESP_SIZE,
    /* IEEE80211_TDLS_PEER_PSM_REQ */
    0,
    /* IEEE80211_TDLS_PEER_PSM_RESP */
    0,
    /* reserved for other frames */
    /* IEEE80211_TDLS_PEER_TRAFFIC_RESPONSE */
    IEEE80211_TDLS_FRAME_MAXLEN + IEEE80211_TDLS_DIALOG_TKN_SIZE,
    /* IEEE80211_TDLS_DISCOVERY_REQ */
    IEEE80211_TDLS_FRAME_MAXLEN + IEEE80211_TDLS_DIALOG_TKN_SIZE,
    0,0,0, /* reserved for other frames */
    /* IEEE80211_TDLS_DISCOVERY_RESP */
    IEEE80211_TDLS_FRAME_MAXLEN + IEEE80211_TDLS_DIALOG_TKN_SIZE,
#if CONFIG_RCPI
    /* IEEE80211_TDLS_TXPATH_SWITCH_REQ */
    IEEE80211_TDLS_FRAME_MAXLEN + 4, /* Path(3-Octets) + Reason(1-Octet) */
    /* IEEE80211_TDLS_TXPATH_SWITCH_RESP */
    IEEE80211_TDLS_FRAME_MAXLEN + 4, /* Path(3-Octets) + Reason(1-Octet) */
    /* IEEE80211_TDLS_RXPATH_SWITCH_REQ */
    IEEE80211_TDLS_FRAME_MAXLEN + 4, /* Path(3-Octets) + Reason(1-Octet) */
    /* IEEE80211_TDLS_RXPATH_SWITCH_RESP */
    IEEE80211_TDLS_FRAME_MAXLEN + 3, /* Path(3-Octets) */
    /* IEEE80211_TDLS_LINKRCPI_REQ */
    IEEE80211_TDLS_FRAME_MAXLEN + 12, /* BSSID + STA Address */
    /* IEEE80211_TDLS_LINKRCPI_REPORT */
    IEEE80211_TDLS_FRAME_MAXLEN + 14, /* BSSID + RCPI + STA Address + RCPI */
#endif
    /* IEEE80211_TDLS_LEARNED_ARP */
    IEEE80211_TDLS_FRAME_MAXLEN + IEEE80211_TDLS_DIALOG_TKN_SIZE
};

/*
 * State machine states
 */
typedef enum {
    IEEE80211_TDLS_LINK_MONITOR_STATE_INIT = 0,
    IEEE80211_TDLS_LINK_MONITOR_STATE_NORMAL,
    IEEE80211_TDLS_LINK_MONITOR_STATE_ERROR_DETECTED,
    IEEE80211_TDLS_LINK_MONITOR_STATE_ERROR_RECOVERY,
    IEEE80211_TDLS_LINK_MONITOR_STATE_UNREACHABLE,
} ieee80211_tdls_link_monitor_state;

static const char* tdls_link_monitor_event_name[] = {
/* IEEE80211_TDLS_LINK_MONITOR_EVENT_NONE */                "TDLS_LINK_MONITOR_NONE",
/* IEEE80211_TDLS_LINK_MONITOR_EVENT_TMO */                 "TDLS_LINK_MONITOR_TIMEOUT",
/* IEEE80211_TDLS_LINK_MONITOR_EVENT_TX_OK */               "TDLS_LINK_MONITOR_TX_OK",
/* IEEE80211_TDLS_LINK_MONITOR_EVENT_TX_FAILED */           "TDLS_LINK_MONITOR_TX_FAILED",
/* IEEE80211_TDLS_LINK_MONITOR_EVENT_RX_OK */               "RX_OK",
/* IEEE80211_TDLS_LINK_MONITOR_EVENT_RESET */               "RESET",
/* IEEE80211_TDLS_LINK_MONITOR_EVENT_START */               "TDLS_LINK_MONITOR_START",
/* IEEE80211_TDLS_LINK_MONITOR_EVENT_STOP */                "TDLS_LINK_MONITOR_STOP",
};

#if CONFIG_RCPI
int ieee80211_tdls_link_rcpi_report(struct ieee80211vap *vap, char * bssid,
                                char * mac, u_int8_t rcpi1,
                                u_int8_t rcpi2);

static void ieee80211_tdlsrcpi_timer_init(struct ieee80211com *ic,
                                void * timerArg);

static void ieee80211_tdlsrcpi_timer_stop(struct ieee80211com *ic,
                                unsigned long timerArg);

static int ieee80211_tdls_link_rcpi_req(struct ieee80211vap *vap, char * bssid,
                                char * mac);
#endif

static void ieee80211tdls_deliver_pu_buf_sta_event(struct ieee80211_node *ni,
        ieee80211tdls_pu_buf_sta_event event_type, ieee80211tdls_pu_buf_sta_sm_event_t *event);
static void ieee80211tdls_deliver_pu_sleep_sta_event(struct ieee80211_node *ni,
        ieee80211tdls_pu_sleep_sta_event event_type, ieee80211tdls_pu_sleep_sta_sm_event_t *event);
void ieee80211_tdls_del_node(struct ieee80211_node *ni);
static int tdls_send_teardown_ap(struct ieee80211com *ic, struct ieee80211vap *vap,
                           void* arg, uint8_t *buf, uint16_t buf_len, u_int16_t status);
void tdls_request_teardown_ftie(struct ieee80211vap *vap, char *mac,
                            enum ieee80211_tdls_action type,
                            u16 reason_code, u8 dtoken);

extern int ieee80211_tdls_setup_ht_rates(struct ieee80211_node *ni,
                         u_int8_t *ie, int flags);
extern void tdls_rate_updatenode(struct ieee80211_node *ni);
static void ieee80211tdls_vap_event_handler(ieee80211_vap_t     vap,
                                            ieee80211_vap_event *event,
                                            void                *arg);

//extern wbuf_t ieee80211_getmgtframe(struct ieee80211_node *ni, int subtype, u_int8_t **frm);

#if 0 & UMAC_SUPPORT_TDLS
extern wbuf_t ieee80211_decap(struct ieee80211vap *vap, wbuf_t wbuf, size_t hdrspace);
#endif /* UMAC_SUPPORT_TDLS */

static int get_wpa_mode(struct ieee80211vap *vap)
{
	/* IEEE80211_FEATURE_PRIVACY, flag is good to say 
	 * if vap is in OPEN or Secure mode(either WEP/WPA/WPA2) 
	 */
	return wlan_get_param(vap, IEEE80211_FEATURE_PRIVACY);
}

static INLINE int ieee80211tdls_link_monitor_sm_dispatch(link_monitor_t *lm, u_int16_t event,
                                                                   ieee80211tdls_link_monitor_sm_event_t *event_data)
{
    if (event_data->vap && event_data->vap->iv_ic->ic_reg_parm.resmgrSyncSm == 0)  {
        /* NOTE: there is potential race condition where the VAP could be deleted. */
        if ((event_data->vap->iv_node_count == 0) || (ieee80211_vap_deleted_is_set(event_data->vap))) {
            /*
             * VAP will be freed right after we return  from here. So do not send the event to  the
             * resource manage if the resmgr is running asynchronously. drop it here.
             * EV 71462 has been filed to find a proper fix .
             * this condition hits on win7 for monitor mode.
             */
             printk("%s: ****  node count is 0 , drop the event %d **** \n",__func__,event );
             return EOK;
        }
        event_data->ni_bss_node = ieee80211_ref_bss_node(event_data->vap);
    } else {
        /* if  SM is running synchronosly , do not need vap/bss node ref */
        event_data->ni_bss_node=NULL;
    }
    ieee80211_sm_dispatch(lm->hsm_handle, event, sizeof(struct ieee80211tdls_link_monitor_sm_event), event_data);
    return EOK;
}

/*
 * txrx event handler to find out if TX errors are persisting.
 */
static void tdls_link_monitor_txrx_event_handler (ieee80211_vap_t vap,
        ieee80211_vap_txrx_event *event, void *arg)
{
    link_monitor_t *lm = (link_monitor_t *) arg;
    ieee80211tdls_link_monitor_sm_event_t link_monitor_event;

    if (vap == NULL || event == NULL || arg == NULL) {
        return;
    }

    if (event->ni == NULL || event->ni != lm->w_tdls->tdls_ni) {
        /* Filter this event; It doesn't belong to this handler */
        return;
    }

    OS_MEMZERO(&link_monitor_event, sizeof(ieee80211tdls_link_monitor_sm_event_t));
    link_monitor_event.vap = vap;
    link_monitor_event.ni_tdls = event->ni;

    switch(event->type) {

    case IEEE80211_VAP_OUTPUT_EVENT_TX_ERROR:
        /* Dispatch Link Monitor SM */
        ieee80211tdls_link_monitor_sm_dispatch(lm,
                IEEE80211_TDLS_LINK_MONITOR_EVENT_TX_FAILED,
                &link_monitor_event);
        printk("%s : Tx failed, ni = 0x%08x\n", __func__, (u_int32_t) event->ni);
        break;

    case IEEE80211_VAP_OUTPUT_EVENT_TX_SUCCESS:
        /* Dispatch Link Monitor SM */
        ieee80211tdls_link_monitor_sm_dispatch(lm,
                IEEE80211_TDLS_LINK_MONITOR_EVENT_TX_OK,
                &link_monitor_event);
        printk("%s : Tx OK, ni = 0x%08x\n", __func__, (u_int32_t) event->ni);
        break;
    case IEEE80211_VAP_INPUT_EVENT_UCAST:
        /* Dispatch Link Monitor SM */
        ieee80211tdls_link_monitor_sm_dispatch(lm,
        IEEE80211_TDLS_LINK_MONITOR_EVENT_RX_OK,
                &link_monitor_event);
        break;

    default:
        break;
    }

    return;
}

static void
ieee80211tdls_link_monitor_start(struct ieee80211_node *ni)
{
    link_monitor_t *lm;
    ieee80211tdls_link_monitor_sm_event_t link_monitor_event;

    if (ni == NULL) {
        return;
    }

    lm = &ni->ni_tdls->w_tdls->link_monitor;

    OS_MEMZERO(&link_monitor_event, sizeof(ieee80211tdls_link_monitor_sm_event_t));
    link_monitor_event.vap = ni->ni_vap;
    link_monitor_event.ni_tdls = ni;

    /* Dispatch Link Monitor SM */
    ieee80211tdls_link_monitor_sm_dispatch(lm,
            IEEE80211_TDLS_LINK_MONITOR_EVENT_START,
            &link_monitor_event);

    return;
}

static void
ieee80211tdls_link_monitor_stop(struct ieee80211_node *ni)
{
    link_monitor_t *lm;
    ieee80211tdls_link_monitor_sm_event_t link_monitor_event;

    if (ni == NULL) {
        return;
    }

    lm = &ni->ni_tdls->w_tdls->link_monitor;

    OS_MEMZERO(&link_monitor_event, sizeof(ieee80211tdls_link_monitor_sm_event_t));
    link_monitor_event.vap = ni->ni_vap;
    link_monitor_event.ni_tdls = ni;

    /* Dispatch Link Monitor SM */
    ieee80211tdls_link_monitor_sm_dispatch(lm,
            IEEE80211_TDLS_LINK_MONITOR_EVENT_STOP,
            &link_monitor_event);

    return;
}

static void
ieee80211tdls_link_monitor_sm_debug_print (void *ctx, const char *fmt,...)
{
    char tmp_buf[256];
    va_list ap;
    link_monitor_t *lm = (link_monitor_t *) ctx;
     
    va_start(ap, fmt);
    vsnprintf (tmp_buf,256, fmt, ap);
    va_end(ap);
    IEEE80211_DPRINTF(lm->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s", tmp_buf);
}

static void
ieee80211tdls_link_monitor_clear_event(link_monitor_t *lm)
{
    if (lm) {
        /* VAP related event */
       if (lm->deferred_event.ni_bss_node) {
           ieee80211_free_node(lm->deferred_event.ni_bss_node);
       }
       IEEE80211_DPRINTF_IC(lm->w_tdls->tdls_ni->ni_ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                                 "%s: clear deferred event %d \n ",__func__,lm->deferred_event_type);
        OS_MEMZERO(&lm->deferred_event, sizeof(ieee80211tdls_link_monitor_sm_event_t));
        lm->deferred_event_type = IEEE80211_TDLS_LINK_MONITOR_EVENT_NONE;
    }
}

static void
ieee80211tdls_link_monitor_process_deferred_events(link_monitor_t *lm)
{
    ieee80211tdls_link_monitor_sm_event_t *event;
    u_int16_t event_type;
    bool dispatch_event;

    if (! lm) {
        return;
    }

    event_type = lm->deferred_event_type;

    if (event_type != IEEE80211_TDLS_LINK_MONITOR_EVENT_NONE) {

        dispatch_event = false;
        event = &lm->deferred_event;

        switch(event_type) {
        case IEEE80211_TDLS_LINK_MONITOR_EVENT_TMO:
        case IEEE80211_TDLS_LINK_MONITOR_EVENT_TX_OK:
        case IEEE80211_TDLS_LINK_MONITOR_EVENT_TX_FAILED:
        case IEEE80211_TDLS_LINK_MONITOR_EVENT_START:
        case IEEE80211_TDLS_LINK_MONITOR_EVENT_STOP:
        case IEEE80211_TDLS_LINK_MONITOR_EVENT_RX_OK:
        case IEEE80211_TDLS_LINK_MONITOR_EVENT_RESET:
            dispatch_event = true;
            break;

        default:
            dispatch_event = false;
            break;
    }

        if (dispatch_event) {
            ieee80211tdls_link_monitor_sm_dispatch(lm, event_type, event);
            dispatch_event = false;
        }

        /* Clear deferred event */
        ieee80211tdls_link_monitor_clear_event(lm);
    }

    return;
}

static void
link_monitor_post_event(link_monitor_t                                 *lm,
                       ieee80211_tdls_link_monitor_notification_type   notification_type,
                       ieee80211_tdls_link_monitor_notification_reason reason)
{
    ieee80211_tdls_link_monitor_notification_data    link_monitor_notification_data;

    IEEE80211_DPRINTF(lm->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: notification_type=%d reason=%d\n",
        __func__, notification_type, reason);

    // Populate event fields in event structure
    OS_MEMZERO(&link_monitor_notification_data, sizeof(link_monitor_notification_data));
    link_monitor_notification_data.reason = reason;
    notifier_post_event(
        lm->w_tdls->tdls_ni,
        &(lm->notifier),
        notification_type,
        sizeof(link_monitor_notification_data),
        &(link_monitor_notification_data));
}

/*
 * different state related functions.
 */
/*
 * State INIT
 */

static void
ieee80211tdls_link_monitor_state_init_entry(void *ctx)
{
    link_monitor_t    *lm = (link_monitor_t *) ctx;

    link_monitor_post_event(lm,
                            IEEE80211_TDLS_LINK_MONITOR_INIT,
                            lm->state_change_reason);
}
static void
ieee80211tdls_link_monitor_state_init_exit(void *ctx)
{
    link_monitor_t *lm = (link_monitor_t *) ctx;
    u_int32_t event_filter;
    u_int32_t error;
    struct ieee80211com      *ic;

    ic = lm->w_tdls->tdls_ni->ni_ic;

    event_filter = (IEEE80211_VAP_OUTPUT_EVENT_TX_ERROR |
                    IEEE80211_VAP_OUTPUT_EVENT_TX_SUCCESS);
    error = ieee80211_vap_txrx_register_event_handler(lm->w_tdls->tdls_vap,
            tdls_link_monitor_txrx_event_handler, (void *) lm, event_filter);

    if (error != EOK) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                             "%s : ieee80211_vap_txrx_register_event_handler failure!\n", __FUNCTION__);
        return;
    }
    
    error = ieee80211_sta_power_register_event_handler(lm->w_tdls->tdls_vap,
                                                       (ieee80211_sta_power_event_handler)ieee80211tdls_sta_power_event_handler,
                                                       &(lm->w_tdls->chan_switch));

    if (error != EOK) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                             "%s: ieee80211_sta_power_register_event_handler failure! error=%lu\n", __func__, error);
        return;
    }
    
    link_monitor_post_event(lm,
                            IEEE80211_TDLS_LINK_MONITOR_STARTED,
                            lm->state_change_reason);

    return;
}

static bool
ieee80211tdls_link_monitor_state_init_event(void *ctx, u_int16_t event_type, u_int16_t event_data_len, void *event_data)
{
    link_monitor_t *lm = (link_monitor_t *) ctx;
    bool retVal = true;

    if (lm == NULL) {
        return false;
    }

    switch (event_type) {
    case IEEE80211_TDLS_LINK_MONITOR_EVENT_START:

        /* transition to NORMAL state */
        ieee80211_sm_transition_to(lm->hsm_handle, IEEE80211_TDLS_LINK_MONITOR_STATE_NORMAL);
        break;

        default:
        break;
    }

    return(retVal);
}

/*
 * State NORMAL
 */
static void
ieee80211tdls_link_monitor_state_normal_entry(void *ctx)
{
    link_monitor_t *lm = (link_monitor_t *) ctx;

    lm->tx_error_count = 0;

    link_monitor_post_event(lm,
                            IEEE80211_TDLS_LINK_MONITOR_NORMAL,
                            lm->state_change_reason);
    /* This is a stable state, process deferred events if any ...  */
    ieee80211tdls_link_monitor_process_deferred_events(lm);
}

static bool
ieee80211tdls_link_monitor_state_normal_event(void *ctx, u_int16_t event_type, u_int16_t event_data_len, void *event_data)
{
    link_monitor_t *lm = (link_monitor_t *) ctx;
    bool retVal = true;
    u_int32_t error, event_filter;
    struct ieee80211com      *ic;


    if (lm == NULL) {
        return false;
    }

    ic = lm->w_tdls->tdls_ni->ni_ic;

    switch (event_type) {
    case IEEE80211_TDLS_LINK_MONITOR_EVENT_TX_FAILED:
        
        /* transition to IEEE80211_TDLS_LINK_MONITOR_STATE_ERROR_DETECTED state */
        lm->state_change_reason = IEEE80211_TDLS_LINK_MONITOR_REASON_TX_ERROR;                                                      ieee80211_sm_transition_to(lm->hsm_handle, IEEE80211_TDLS_LINK_MONITOR_STATE_ERROR_DETECTED);
        break;

    case IEEE80211_TDLS_LINK_MONITOR_EVENT_STOP:

        event_filter = IEEE80211_VAP_INPUT_EVENT_NONE;
        error = ieee80211_vap_txrx_register_event_handler(lm->w_tdls->tdls_vap,
                tdls_link_monitor_txrx_event_handler, (void *) lm, event_filter);

        if (error != EOK) {
            IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                "%s : ieee80211_vap_txrx_register_event_handler failure!\n", __FUNCTION__);
        }
        error = ieee80211_sta_power_register_event_handler(lm->w_tdls->tdls_vap,
                 (ieee80211_sta_power_event_handler)ieee80211tdls_sta_power_event_handler, &(lm->w_tdls->chan_switch));
        
        if (error != EOK) {
            IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                "%s: ieee80211_sta_power_register_event_handler failure! error=%lu\n", __FUNCTION__);
        }

        /* transition to INIT state */
        lm->state_change_reason = IEEE80211_TDLS_LINK_MONITOR_REASON_STOP;
        ieee80211_sm_transition_to(lm->hsm_handle, IEEE80211_TDLS_LINK_MONITOR_STATE_INIT);
        break;

        default:
        break;
    }

    return(retVal);
}

/*
 * State ERROR_DETECTED
 */
static void
ieee80211tdls_link_monitor_state_error_detected_entry(void *ctx)
{
    link_monitor_t    *lm = (link_monitor_t *) ctx;

    lm->tx_error_count = 1;

    if (lm->error_detected_timeout > 0) {
        OS_SET_TIMER(&lm->link_monitor_timer, lm->error_detected_timeout);
    }

    link_monitor_post_event(lm, IEEE80211_TDLS_LINK_MONITOR_ERROR_DETECTED, lm->state_change_reason);
}

static bool
ieee80211tdls_link_monitor_state_error_detected_event(void *ctx, u_int16_t event_type, u_int16_t event_data_len, void *event_data)
{
    link_monitor_t         *lm = (link_monitor_t *) ctx;
    bool                   retVal = true;
    u_int32_t              error;
    struct ieee80211com    *ic;

    if (lm == NULL) {
        return false;
    }

    ic = lm->w_tdls->tdls_ni->ni_ic;

    switch (event_type) {
    case IEEE80211_TDLS_LINK_MONITOR_EVENT_TX_OK:
     
        /* Cancel Link Monitor timeout */
        OS_CANCEL_TIMER(&lm->link_monitor_timer);

        /* transition to NORMAL state */
        lm->state_change_reason = IEEE80211_TDLS_LINK_MONITOR_REASON_TX_OK;
        ieee80211_sm_transition_to(lm->hsm_handle, IEEE80211_TDLS_LINK_MONITOR_STATE_NORMAL);
        break;
    
    case IEEE80211_TDLS_LINK_MONITOR_EVENT_TX_FAILED:

        lm->tx_error_count += 1;

        if ((lm->tx_error_threshold > 0) && (lm->tx_error_count >= lm->tx_error_threshold)) {
            /* Cancel Link Monitor timeout */
            OS_CANCEL_TIMER(&lm->link_monitor_timer);

            /* transition to Error Recovery state */
            lm->state_change_reason = IEEE80211_TDLS_LINK_MONITOR_REASON_ERROR_COUNT;
            ieee80211_sm_transition_to(lm->hsm_handle, IEEE80211_TDLS_LINK_MONITOR_STATE_ERROR_RECOVERY);
        }
        break;
    
    case IEEE80211_TDLS_LINK_MONITOR_EVENT_TMO:
        lm->state_change_reason = IEEE80211_TDLS_LINK_MONITOR_REASON_TIMEOUT;
        ieee80211_sm_transition_to(lm->hsm_handle, IEEE80211_TDLS_LINK_MONITOR_STATE_ERROR_RECOVERY);
        break;

    case IEEE80211_TDLS_LINK_MONITOR_EVENT_STOP:
        /* Cancel Link Monitor timeout */
        OS_CANCEL_TIMER(&lm->link_monitor_timer);

        error = ieee80211_vap_txrx_register_event_handler(lm->w_tdls->tdls_vap,
           tdls_link_monitor_txrx_event_handler,
            (void *) lm, IEEE80211_VAP_INPUT_EVENT_NONE);

        if (error != EOK) {
            IEEE80211_DPRINTF(lm->w_tdls->tdls_vap, IEEE80211_MSG_TDLS,
                "%s: ieee80211_vap_txrx_register_event_handler failure! error=%lu\n", __func__, error);
        }

        error = ieee80211_sta_power_register_event_handler(lm->w_tdls->tdls_vap,
                                                           (ieee80211_sta_power_event_handler)ieee80211tdls_sta_power_event_handler,
                                                           &(lm->w_tdls->chan_switch));
                                                                                                            
        if (error != EOK) {
            IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                "%s: ieee80211_sta_power_register_event_handler failure! error=%lu\n", __func__, error);
        }                                                                 

        /* transition to INIT state */
        lm->state_change_reason = IEEE80211_TDLS_LINK_MONITOR_REASON_STOP;
        ieee80211_sm_transition_to(lm->hsm_handle, IEEE80211_TDLS_LINK_MONITOR_STATE_INIT);
        break;


    case IEEE80211_TDLS_LINK_MONITOR_EVENT_RX_OK:
        /* Cancel Link Monitor timeout */
        OS_CANCEL_TIMER(&lm->link_monitor_timer);

        /* transition to NORMAL state */
        lm->state_change_reason = IEEE80211_TDLS_LINK_MONITOR_REASON_RX_OK;
        ieee80211_sm_transition_to(lm->hsm_handle, IEEE80211_TDLS_LINK_MONITOR_STATE_NORMAL);
        break;


    case IEEE80211_TDLS_LINK_MONITOR_EVENT_RESET:
        /*
         * Reset error counting due to transition off_channel -> base_channel.
         * Flag is_offchannel already set to false.
         */
        lm->state_change_reason = IEEE80211_TDLS_LINK_MONITOR_REASON_BASE_CHANNEL;
        ieee80211_sm_transition_to(lm->hsm_handle, IEEE80211_TDLS_LINK_MONITOR_STATE_NORMAL);
        break;

    default:
        break;
    }

    return (retVal);
}

/*
 * State ERROR_RECOVERY
 */
static void
ieee80211tdls_send_error_recovery_frame(link_monitor_t *lm)
{
    int    rc;

    /*
     * If on base channel, send a Null frame to verify status of Direct Link.
     */
    if (! lm->is_offchannel) {
        rc = tdls_send_qosnulldata(lm->w_tdls->tdls_ni,
                                   WME_AC_VO,
                                   0,
                                   NULL,
                                   NULL);

        if (rc != 0) {
            IEEE80211_DPRINTF(lm->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: failed to send NullData rc=%d\n",
                __func__, rc);
        }
    }

    /*
     * Set timer to error recovery schedule (interval to retry frame exchange
     * with peer). If no recovery schedule defined, set timer to recovery
     * timeout so that we know when to give up.
     */
    if (lm->error_recovery_schedule > 0) {
        OS_SET_TIMER(&lm->link_monitor_timer, lm->error_recovery_schedule);
    }
    else {
        if (lm->error_recovery_timeout > 0) {
            OS_SET_TIMER(&lm->link_monitor_timer, lm->error_recovery_timeout);
        }
    }
}

static void
ieee80211tdls_link_monitor_state_error_recovery_entry(void *ctx)
{
    link_monitor_t    *lm = (link_monitor_t *) ctx;

    lm->error_recovery_start_time = OS_GET_TIMESTAMP();

    link_monitor_post_event(lm,
                            IEEE80211_TDLS_LINK_MONITOR_ERROR_RECOVERY,
                            lm->state_change_reason);

    /*
     * Send a Null frame to verify status of Direct Link.
     */
    ieee80211tdls_send_error_recovery_frame(lm);
}

static bool
ieee80211tdls_link_monitor_state_error_recovery_event(void *ctx, u_int16_t event_type, 
        u_int16_t event_data_len, void *event_data)
{
    link_monitor_t         *lm = (link_monitor_t *) ctx;
    bool                   retVal = true;
    u_int32_t              error;
    struct ieee80211com    *ic;

    if (lm == NULL) {
   
        return false;
    
    }
   

    ic = lm->w_tdls->tdls_ni->ni_ic;

    switch (event_type) {
    case IEEE80211_TDLS_LINK_MONITOR_EVENT_TX_OK:

        // Cancel Link Monitor timeout
        OS_CANCEL_TIMER(&lm->link_monitor_timer);

        // transition to NORMAL state
        lm->state_change_reason = IEEE80211_TDLS_LINK_MONITOR_REASON_TX_OK;
        ieee80211_sm_transition_to(lm->hsm_handle, IEEE80211_TDLS_LINK_MONITOR_STATE_NORMAL);
        break;

    case IEEE80211_TDLS_LINK_MONITOR_EVENT_RX_OK:

        // Cancel Link Monitor timeout
        OS_CANCEL_TIMER(&lm->link_monitor_timer);

        // transition to NORMAL state
        lm->state_change_reason = IEEE80211_TDLS_LINK_MONITOR_REASON_RX_OK;
        ieee80211_sm_transition_to(lm->hsm_handle, IEEE80211_TDLS_LINK_MONITOR_STATE_NORMAL);
        break;

    case IEEE80211_TDLS_LINK_MONITOR_EVENT_TX_FAILED:
        // Count error, but take no further action
        lm->tx_error_count += 1;
        break;

    case IEEE80211_TDLS_LINK_MONITOR_EVENT_TMO:
        {
            u_int32_t    elapsed_time = CONVERT_SYSTEM_TIME_TO_MS(
            OS_GET_TIMESTAMP() - lm->error_recovery_start_time);

            if (elapsed_time < lm->error_recovery_timeout) {
                /*
                 * If maximum allowed time for error recovery hasn't expired,
                 * continue trying.
                 * Send a Null frame to verify status of Direct Link if we are
                 * on the base channel.
                 */
                ieee80211tdls_send_error_recovery_frame(lm);
            }
            else {
                /*
                 * Exceeded maximum time without being able to exchange frames
                 * on the Direct Link. Transition to "unreachable" state, where
                 * link will be torn down.
                 */
                error = ieee80211_vap_txrx_register_event_handler(lm->w_tdls->tdls_vap,
                                tdls_link_monitor_txrx_event_handler, (void *) lm, 
                                IEEE80211_VAP_INPUT_EVENT_NONE);

                if (error != EOK) {
                    IEEE80211_DPRINTF(lm->w_tdls->tdls_vap, IEEE80211_MSG_TDLS,
                        "%s: ieee80211_vap_txrx_register_event_handler failure! error=%lu\n", __func__, error);
                }

                error = ieee80211_sta_power_register_event_handler(lm->w_tdls->tdls_vap,
                                (ieee80211_sta_power_event_handler)ieee80211tdls_sta_power_event_handler, &(lm->w_tdls->chan_switch));

                if (error != EOK) {
                    IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                        "%s: ieee80211_sta_power_register_event_handler failure! error=%lu\n", __func__, error);
                }

                // transition to UNREACHABLE state
                lm->state_change_reason = IEEE80211_TDLS_LINK_MONITOR_REASON_TIMEOUT;
                ieee80211_sm_transition_to(lm->hsm_handle,
                        IEEE80211_TDLS_LINK_MONITOR_STATE_UNREACHABLE);
            }
        }
        break;

    case IEEE80211_TDLS_LINK_MONITOR_EVENT_STOP:
        /* Cancel Link Monitor timeout */
        OS_CANCEL_TIMER(&lm->link_monitor_timer);

        error = ieee80211_vap_txrx_register_event_handler(lm->w_tdls->tdls_vap,
            tdls_link_monitor_txrx_event_handler,
            (void *) lm,
            IEEE80211_VAP_INPUT_EVENT_NONE);

        if (error != EOK) {
            IEEE80211_DPRINTF(lm->w_tdls->tdls_vap, IEEE80211_MSG_TDLS,
                "%s: ieee80211_vap_txrx_register_event_handler failure! error=%lu\n", __func__, error);
        }

        error = ieee80211_sta_power_register_event_handler(lm->w_tdls->tdls_vap,
                                                           (ieee80211_sta_power_event_handler)ieee80211tdls_sta_power_event_handler,
                                                           &(lm->w_tdls->chan_switch));

        if (error != EOK) {
            IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                "%s: ieee80211_sta_power_register_event_handler failure! error=%lu\n", __func__, error);
        }

        /* transition to INIT state */
        lm->state_change_reason = IEEE80211_TDLS_LINK_MONITOR_REASON_STOP;
        ieee80211_sm_transition_to(lm->hsm_handle, IEEE80211_TDLS_LINK_MONITOR_STATE_INIT);
        break;

    case IEEE80211_TDLS_LINK_MONITOR_EVENT_RESET:
        /*
         * Reset error counting due to transition off_channel -> base_channel.
         * Flag is_offchannel already set to false.
         */
        lm->state_change_reason = IEEE80211_TDLS_LINK_MONITOR_REASON_BASE_CHANNEL;
        ieee80211_sm_transition_to(lm->hsm_handle, IEEE80211_TDLS_LINK_MONITOR_STATE_NORMAL);
        break;

    default:
        break;
    }

    return (retVal);
}

/*
 * State UNREACHABLE
 */
static void
ieee80211tdls_link_monitor_state_unreachable_entry(void *ctx)
{
    link_monitor_t *lm = (link_monitor_t *) ctx;
    char macaddr[18];
    struct ieee80211vap     *vap;

    if (! lm) {
        return;
    }
    
    link_monitor_post_event(lm,
                   IEEE80211_TDLS_LINK_MONITOR_UNREACHABLE,
                   lm->state_change_reason);

    lm->tx_error_count = 0;
    vap = lm->w_tdls->tdls_vap;

    IEEE80211_ADDR_COPY(macaddr, lm->w_tdls->tdls_ni->ni_macaddr);

    /*
     * Request wpa_supplicant to prepare additional IEs for TDLS Teardown
     * to tear down a direct link.
 */
    tdls_request_teardown_ftie(vap, macaddr,
                   IEEE80211_TDLS_TEARDOWN,
                   IEEE80211_TDLS_TEARDOWN_UNREACHABLE_REASON,
                   lm->w_tdls->tdls_ni->ni_tdls->token);

    return;
}

static bool
ieee80211tdls_link_monitor_state_unreachable_event(void *ctx, u_int16_t event_type, u_int16_t event_data_len, void *event_data)
{
    link_monitor_t *lm = (link_monitor_t *) ctx;
    bool retVal = true;
    u_int32_t error, event_filter;
    struct ieee80211com      *ic;

    if (lm == NULL) {
        return false;
    }

    ic = lm->w_tdls->tdls_ni->ni_ic;

    switch (event_type) {
    case IEEE80211_TDLS_LINK_MONITOR_EVENT_STOP:

        event_filter = 0;
        error = ieee80211_vap_txrx_register_event_handler(lm->w_tdls->tdls_vap,
                tdls_link_monitor_txrx_event_handler, (void *) lm, event_filter);

        if (error != EOK) {
            IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                "%s : ieee80211_vap_txrx_register_event_handler failure!\n", __FUNCTION__);
        }
        error = ieee80211_sta_power_register_event_handler(lm->w_tdls->tdls_vap,
                (ieee80211_sta_power_event_handler)ieee80211tdls_sta_power_event_handler, &(lm->w_tdls->chan_switch));
          
        if (error != EOK) {
            IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                "%s: ieee80211_sta_power_register_event_handler failure! error=%lu\n", __FUNCTION__);
        }

        /* transition to INIT state */
        lm->state_change_reason = IEEE80211_TDLS_LINK_MONITOR_REASON_STOP;
        ieee80211_sm_transition_to(lm->hsm_handle, IEEE80211_TDLS_LINK_MONITOR_STATE_INIT);
        break;
    default:
        break;

    }

    return(retVal);
}

ieee80211_state_info ieee80211_tdls_link_monitor_sm_info[] = {
    { 
        (u_int8_t) IEEE80211_TDLS_LINK_MONITOR_STATE_INIT,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "INIT",
        ieee80211tdls_link_monitor_state_init_entry,
        ieee80211tdls_link_monitor_state_init_exit,
        ieee80211tdls_link_monitor_state_init_event
    },
    { 
        (u_int8_t) IEEE80211_TDLS_LINK_MONITOR_STATE_NORMAL,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "NORMAL",
        ieee80211tdls_link_monitor_state_normal_entry,
        NULL,
        ieee80211tdls_link_monitor_state_normal_event
    },
    {
         (u_int8_t) IEEE80211_TDLS_LINK_MONITOR_STATE_ERROR_DETECTED,
         (u_int8_t) IEEE80211_HSM_STATE_NONE,
         (u_int8_t) IEEE80211_HSM_STATE_NONE,
         false,
         "ERROR_DETECTED",
         ieee80211tdls_link_monitor_state_error_detected_entry,
         NULL,
         ieee80211tdls_link_monitor_state_error_detected_event
    },
    {
         (u_int8_t) IEEE80211_TDLS_LINK_MONITOR_STATE_ERROR_RECOVERY,
         (u_int8_t) IEEE80211_HSM_STATE_NONE,
         (u_int8_t) IEEE80211_HSM_STATE_NONE,
         false,
         "ERROR_RECOVERY",
         ieee80211tdls_link_monitor_state_error_recovery_entry,
         NULL,
         ieee80211tdls_link_monitor_state_error_recovery_event
    },
    { 
        (u_int8_t) IEEE80211_TDLS_LINK_MONITOR_STATE_UNREACHABLE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "UNREACHABLE",
        ieee80211tdls_link_monitor_state_unreachable_entry,
        NULL,
        ieee80211tdls_link_monitor_state_unreachable_event
    },
};

OS_TIMER_FUNC(link_monitor_sm_timeout)
{
    link_monitor_t *lm;
    ieee80211tdls_link_monitor_sm_event_t event;

    OS_GET_TIMER_ARG(lm, link_monitor_t *);

    OS_MEMZERO(&event, sizeof(ieee80211tdls_link_monitor_sm_event_t));

    event.vap = lm->w_tdls->tdls_vap;
    event.ni_tdls = lm->w_tdls->tdls_ni;

    /* Dispatch Link Monitor SM */
    ieee80211tdls_link_monitor_sm_dispatch(lm,
            IEEE80211_TDLS_LINK_MONITOR_EVENT_TMO,
            &event);
}

static void
ieee80211tdls_link_monitor_sm_delete(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls)
{
    link_monitor_t *lm;
    struct ieee80211com      *ic;
    int error;

    if (ni == NULL || w_tdls == NULL) {
        return;
    }

    ic = w_tdls->tdls_ni->ni_ic;
    lm = &w_tdls->link_monitor;

    error =  ieee80211_vap_txrx_unregister_event_handler(lm->w_tdls->tdls_vap,
            tdls_link_monitor_txrx_event_handler, (void *) lm);
    if (error != EOK) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
            "%s: ieee80211_vap_txrx_unregister_event_handler failure! error=%lu\n", __FUNCTION__);
    }
    error = ieee80211_sta_power_unregister_event_handler(lm->w_tdls->tdls_vap,
                    (ieee80211_sta_power_event_handler)ieee80211tdls_sta_power_event_handler, &(lm->w_tdls->chan_switch));
    
    if (error != EOK) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
            "%s: ieee80211_sta_power_unregister_event_handler failure! error=%lu\n", __func__, error);
    }
    
    // clear all event handlers
    spin_lock(&(lm->notifier.lock));
    lm->notifier.num_handlers = 0;
    spin_unlock(&(lm->notifier.lock));
    spin_lock_destroy(&(lm->notifier.lock));



    /* Cancel Link Monitor timeout */
    OS_CANCEL_TIMER(&lm->link_monitor_timer);

    /* Delete the HSM */
    if (lm->hsm_handle) {
        ieee80211_sm_delete(lm->hsm_handle);
    }

    OS_FREE_TIMER(&lm->link_monitor_timer);

    return;
}
#define LINK_MONITOR_WFA_CERT_ERROR_DETECTED_TIMEOUT       2000
#define LINK_MONITOR_WFA_CERT_ERROR_RECOVERY_SCHEDULE      1000
#define LINK_MONITOR_WFA_CERT_ERROR_RECOVERY_TIMEOUT       15000
#define LINK_MONITOR_WFA_CERT_TX_ERROR_THRESHOLD           0

#define LINK_MONITOR_DEFAULT_ERROR_DETECTED_TIMEOUT        2000
#define LINK_MONITOR_DEFAULT_ERROR_RECOVERY_SCHEDULE       100
#define LINK_MONITOR_DEFAULT_ERROR_RECOVERY_TIMEOUT        500
#define LINK_MONITOR_DEFAULT_TX_ERROR_THRESHOLD            5


void
ieee80211_link_monitor_policy_initialize(link_monitor_t *lm,
                                         int            policy)
{
    lm->error_recovery_start_time = 0;

    switch (policy) {
    case TDLS_POLICY_DEFAULT:
        lm->error_detected_timeout  = LINK_MONITOR_DEFAULT_ERROR_DETECTED_TIMEOUT;
        lm->error_recovery_schedule = LINK_MONITOR_DEFAULT_ERROR_RECOVERY_SCHEDULE;
        lm->error_recovery_timeout  = LINK_MONITOR_DEFAULT_ERROR_RECOVERY_TIMEOUT;
        lm->tx_error_threshold      = LINK_MONITOR_DEFAULT_TX_ERROR_THRESHOLD;
        break;

    case TDLS_POLICY_WFA_CERT:
        lm->error_detected_timeout  = LINK_MONITOR_WFA_CERT_ERROR_DETECTED_TIMEOUT;
        lm->error_recovery_schedule = LINK_MONITOR_WFA_CERT_ERROR_RECOVERY_SCHEDULE;
        lm->error_recovery_timeout  = LINK_MONITOR_WFA_CERT_ERROR_RECOVERY_TIMEOUT;
        lm->tx_error_threshold      = LINK_MONITOR_WFA_CERT_TX_ERROR_THRESHOLD;
        break;

    default:
        IEEE80211_DPRINTF(lm->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ERROR! Invalid policy\n",
            __func__);
        ASSERT(0);
        break;
    }
}
static void
ieee80211tdls_link_monitor_sm_create(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls)
{
    link_monitor_t *lm;
    struct ieee80211com      *ic;

    if (ni == NULL || w_tdls == NULL) {
        return;
    }

    ic = w_tdls->tdls_ni->ni_ic;
    lm = &w_tdls->link_monitor;

    OS_MEMZERO(lm, sizeof(link_monitor_t));

    lm->w_tdls = w_tdls;
    lm->tx_error_threshold = LINK_MONITOR_TX_ERROR_THRESHOLD;
    lm->state_change_reason = IEEE80211_TDLS_LINK_MONITOR_REASON_NONE;
    lm->is_offchannel       = false;
    ieee80211_link_monitor_policy_initialize(lm, TDLS_POLICY_WFA_CERT);
    /* Create State Machine and start */
    lm->hsm_handle = ieee80211_sm_create(ic->ic_osdev,
                         "TDLS_LINK_MONITOR",
                         (void *) lm,
                         IEEE80211_TDLS_LINK_MONITOR_STATE_INIT,
                         ieee80211_tdls_link_monitor_sm_info,
                         sizeof(ieee80211_tdls_link_monitor_sm_info)/sizeof(ieee80211_state_info),
                         MAX_QUEUED_EVENTS,
                         sizeof(struct ieee80211tdls_link_monitor_sm_event), /* size of event data */
                         MESGQ_PRIORITY_HIGH,
                                 IEEE80211_HSM_ASYNCHRONOUS,
                                         ieee80211tdls_link_monitor_sm_debug_print,
                                         tdls_link_monitor_event_name,
                                         IEEE80211_N(tdls_link_monitor_event_name));

    if (!lm->hsm_handle) {
        printk("%s : ieee80211_sm_create failed \n", __func__);
        goto error1;
    }
    // Initialize notifier
    lm->notifier.num_handlers = 0;
    spin_lock_init(&(lm->notifier.lock));

    /* Allocate an OS Timer */
    OS_INIT_TIMER(ic->ic_osdev, &lm->link_monitor_timer,
            link_monitor_sm_timeout, lm);

error1:

    return;
}
#if UMAC_SUPPORT_TDLS_CHAN_SWITCH
void
ieee80211tdls_link_monitor_handle_chan_switch_notification(
    struct ieee80211_node    *ni,
    u_int16_t                notification_type,
    u_int16_t                notification_data_len,
    void                     *notification_data,
    void                     *arg)
{
    link_monitor_t                                    *lm = arg;
    ieee80211_tdls_chan_switch_notification_data_t    *chan_switch_notification_data = notification_data;
    ieee80211tdls_link_monitor_sm_event_t             link_monitor_event;

    if (ni == NULL) {
        printk("%s: Error: ni == NULL\n", __func__);

        return;
    }
    IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_ANY,
        "%s: event=%s (%d) reason=%s (%d)\n",
        __func__,
        ieee80211_tdls_chan_switch_notification_type_name(notification_type),
        notification_type,
        (chan_switch_notification_data != NULL) ? 
        ieee80211_tdls_chan_switch_notification_reason_name(chan_switch_notification_data->reason) : "???",
        (chan_switch_notification_data != NULL) ? chan_switch_notification_data->reason : -1);

    switch (notification_type) {
    case IEEE80211_TDLS_CHAN_SWITCH_STARTED:
        break;

    case IEEE80211_TDLS_CHAN_SWITCH_COMPLETED:
    case IEEE80211_TDLS_CHAN_SWITCH_BASE_CHANNEL:
        if (lm->is_offchannel) {
            lm->is_offchannel = false;

            OS_MEMZERO(&link_monitor_event, sizeof(ieee80211tdls_link_monitor_sm_event_t));
            link_monitor_event.vap     = NULL;
            link_monitor_event.ni_tdls = NULL;

            // Dispatch Link Monitor SM
            ieee80211tdls_link_monitor_sm_dispatch(lm,
                    IEEE80211_TDLS_LINK_MONITOR_EVENT_RESET,
                    &link_monitor_event);
        }
        break;

    case IEEE80211_TDLS_CHAN_SWITCH_DELAYED_TX_ERROR:
        /*
         * Reset error counting due to a corner case in channel switching.
         *     -STA requests that an unsolicited channel switch response or
         *      QOS Null to be transmitted.
         *     -Unsolicited channel switch response is received, indicating
         *      peer is transitioning back to base channel (we do likewise).
         *     -Channel Switch state machine notifies Link Monitor of
         *      transition to base channel, so error counting is reset and
         *      corresponding flags are set.
         *     -Our unsolicited channel switch response or QOS Null fails, as
         *      it was transmitted in the off-channel and the peer transitioned
         *      to the base channel.
         *     -The TX Error indication is received *after* the Link Monitor
         *      has received the notification of return to the base channel,
         *      so it starts the procedure that could lead to a TDLS reset.
         *      A mechanism has been introduced to try to successfully transmit
         *      a frame to our peer before resetting TDLS, but we still use the
         *      delayed error notification to make the connection more robust.
         */
        OS_MEMZERO(&link_monitor_event, sizeof(ieee80211tdls_link_monitor_sm_event_t));
        link_monitor_event.vap     = NULL;
        link_monitor_event.ni_tdls = NULL;

        // Dispatch Link Monitor SM
        ieee80211tdls_link_monitor_sm_dispatch(lm,
                IEEE80211_TDLS_LINK_MONITOR_EVENT_RESET,
                &link_monitor_event);
        break;

    case IEEE80211_TDLS_CHAN_SWITCH_OFFCHANNEL:
        lm->is_offchannel = true;
        break;

    default:
        break;
    }
}
#endif
int ieee80211_tdls_link_monitor_unregister_notification_handler(
    link_monitor_t                      *lm,
    ieee80211_tdls_notification_handler handler,
    void                                *arg)
{
    ASSERT(lm->w_tdls->tdls_vap->iv_ic->ic_tdls != NULL);
    if (lm->w_tdls->tdls_vap->iv_ic->ic_tdls == NULL) {
        IEEE80211_DPRINTF(lm->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ic_tdls not set handler=%p arg=%p\n",
            __func__, handler, arg);

        return -ENOSPC;
    }

    return ieee80211_tdls_unregister_notification_handler(&(lm->notifier), handler, arg);
}
int ieee80211_tdls_link_monitor_register_notification_handler(
    link_monitor_t                      *lm,
    ieee80211_tdls_notification_handler handler,
    void                                *arg)
{
    ASSERT(lm->w_tdls->tdls_vap->iv_ic->ic_tdls != NULL);
    if (lm->w_tdls->tdls_vap->iv_ic->ic_tdls == NULL) {
        IEEE80211_DPRINTF(lm->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ic_tdls not set handler=%p arg=%p\n",
            __func__, handler, arg);

        return -ENOSPC;
    }

    return ieee80211_tdls_register_notification_handler(&(lm->notifier), handler, arg);
}

/* Iterate through the list to find the state machine by the node's address */
/* Call it with lock held */
static wlan_tdls_sm_t ieee80211_tdls_find_byaddr(struct ieee80211com *ic, u_int8_t *addr)
{
    wlan_tdls_sm_t sm;
    wlan_tdls_sm_t smt;
    struct ieee80211_tdls *td;
    rwlock_state_t lock_state;

    if (!addr)
        return NULL;	
    sm = NULL;
    td = ic->ic_tdls;
    TAILQ_FOREACH(smt, &td->tdls_node, tdlslist) {
        if (smt) {
            if(IEEE80211_ADDR_EQ(addr, smt->tdls_addr)) {	
                sm = smt;
                break;
            }
        }	
    }
    return sm;
}

/* Iterate through the list to find sm from node's address and delete it */
/* Call it with lock held */
static void ieee80211_tdls_delentry(struct ieee80211com *ic, struct ieee80211_node *ni)
{
    wlan_tdls_sm_t sm;
    struct ieee80211_tdls *td;
    int rc;

    td = ic->ic_tdls;
    if (ni == NULL) {
        return;      
    }      
    sm = ieee80211_tdls_find_byaddr(ic, ni->ni_macaddr);
    if (sm) {

#if UMAC_SUPPORT_TDLS_CHAN_SWITCH
        rc = ieee80211_tdls_chan_switch_unregister_notification_handler(
                 &(sm->chan_switch),
                 ieee80211tdls_link_monitor_handle_chan_switch_notification,
                 &(sm->link_monitor));
        if (rc != EOK) {
            IEEE80211_DPRINTF(sm->tdls_vap, IEEE80211_MSG_TDLS,
                "%s: Failed to unregister handler from chan_switch_t rc=%d\n",
                __func__, -rc);
        }
#endif

        rc = ieee80211_tdls_link_monitor_unregister_notification_handler(
                     &(sm->link_monitor),
                     ieee80211tdls_chan_switch_handle_link_monitor_notification,
                     &(sm->chan_switch));
        if (rc != EOK) {
            IEEE80211_DPRINTF(sm->tdls_vap, IEEE80211_MSG_TDLS,
                "%s: Failed to unregister handler from link_monitor_t rc=%d\n", __func__, -rc);
        }
        
        ieee80211tdls_link_monitor_sm_delete(ni, sm);
        ieee80211tdls_peer_uapsd_sm_delete(ni, sm);

        /* Release or free the power arbiter requestor ID */
        if (sm->pwr_arbiter_id != 0) {
            ieee80211_power_arbiter_disable(sm->tdls_vap, sm->pwr_arbiter_id);
            ieee80211_power_arbiter_free_id(sm->tdls_vap, sm->pwr_arbiter_id);
            sm->pwr_arbiter_id = 0;
        }
        
        ieee80211tdls_chan_switch_sm_delete(ni, sm);
        
        td->tdls_count --;
        TAILQ_REMOVE(&td->tdls_node, sm, tdlslist);
        OS_FREE(sm); sm=NULL;
    }
   
    
    return;
}

/* Add one entry to the state machine list if no one state machine with 
 * this address is found in the list 
 */
static void ieee80211_tdls_addentry(struct ieee80211vap *vap, struct ieee80211_node *ni)
{
    wlan_tdls_sm_t sm;
    struct ieee80211_tdls *td;
    rwlock_state_t lock_state;
    int rc;

    td = vap->iv_tdlslist;
    if (ni == NULL)
        return;            

    OS_RWLOCK_WRITE_LOCK(&td->tdls_lock, &lock_state);
    sm = ieee80211_tdls_find_byaddr(vap->iv_ic, ni->ni_macaddr);

    /*remove this entry if it exists previously, reset the state machine for this node*/
    if (sm) 
        ieee80211_tdls_delentry(vap->iv_ic, ni);       
    OS_RWLOCK_WRITE_UNLOCK(&td->tdls_lock, &lock_state);

    sm = (wlan_tdls_sm_t)OS_MALLOC(vap-> iv_ic->ic_osdev, sizeof(struct _wlan_tdls_sm), GFP_KERNEL);
    if (sm == NULL) {
	    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, "
			      "unable to allocate sm\n", __FUNCTION__);
	    return;
    }
    OS_MEMZERO(sm, sizeof(struct _wlan_tdls_sm));
    sm->tdls_vap = vap;
    sm->tdls_ni = ni;
    IEEE80211_ADDR_COPY(sm->tdls_addr, ni->ni_macaddr);
    
    ni->ni_tdls->w_tdls = sm;

    /* Create a TDLS Link Monitor state machine */
    ieee80211tdls_link_monitor_sm_create(ni, sm);

    /* Create Peer UAPSD module if enabled */
    ieee80211tdls_peer_uapsd_sm_create(ni, sm);
    
    sm->pwr_arbiter_id = ieee80211_power_arbiter_alloc_id(vap, "TDLS_DIRECT",
            IEEE80211_PWR_ARBITER_TYPE_TDLS);
    if (sm->pwr_arbiter_id != 0) {
        (void) ieee80211_power_arbiter_enable(vap, sm->pwr_arbiter_id);
    }
    else {
        IEEE80211_DPRINTF(vap,IEEE80211_MSG_TDLS,
                "%s : Unable to allocate power arbiter ID\n", __func__);
    }

    /* Create Channel Switch module if enabled */
    ieee80211tdls_chan_switch_sm_create(ni, sm);

    OS_RWLOCK_WRITE_LOCK(&td->tdls_lock, &lock_state);
    td->tdls_count ++;
    TAILQ_INSERT_TAIL(&td->tdls_node, sm, tdlslist);
    OS_RWLOCK_WRITE_UNLOCK(&td->tdls_lock, &lock_state);
#if UMAC_SUPPORT_TDLS_CHAN_SWITCH
    rc = ieee80211_tdls_chan_switch_register_notification_handler(
                 &(sm->chan_switch),
                 ieee80211tdls_link_monitor_handle_chan_switch_notification,
                 &(sm->link_monitor));
     if (rc != EOK) {
         IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                 "%s: Failed to register handle with chan_switch_t rc=%d\n",
                  __func__, -rc);
     }
#endif
     rc = ieee80211_tdls_link_monitor_register_notification_handler(
                  &(sm->link_monitor), ieee80211tdls_chan_switch_handle_link_monitor_notification,
                  &(sm->chan_switch));
     if (rc != EOK) {
         IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                 "%s: Failed to register handle with link_monitor_t rc=%d\n",
                  __func__, -rc);
     }

    return;
}

int 
ieee80211_tdls_attach(struct ieee80211com *ic)
{

    if (!ic) return -EFAULT;

     ic->ic_tdls = (struct ieee80211_tdls *) OS_MALLOC(ic->ic_osdev,
     					sizeof(struct ieee80211_tdls), GFP_KERNEL);
    if (!ic->ic_tdls) {
        return -ENOMEM;
    }

    OS_MEMZERO(ic->ic_tdls, sizeof(struct ieee80211_tdls));

    /* Disabled by default. If you want to enable it use iwpriv command
       to enable it */
    ic->ic_tdls->tdls_enable = 1;
    ic->ic_tdls->peer_uapsd_enable = 1;
    ic->ic_tdls->peer_uapsd_indication_window = 1;
    ic->ic_tdls->response_timeout_msec = 20000;
    ic->ic_tdls->pwr_mgt_state = 0;

    ic->tdls_recv_mgmt = ieee80211_tdls_recv_mgmt;
    ic->ic_tdls->recv_null_data          = ieee80211_tdls_recv_null_data;	
#if CONFIG_RCPI
        ieee80211_tdlsrcpi_timer_init(ic, (void *)ic);
#endif
    return 0;
}

int 
ieee80211_tdls_sm_attach(struct ieee80211com *ic, struct ieee80211vap *vap)
{
    
    if(NULL == vap->iv_tdlslist) {
    vap->iv_tdlslist = ic->ic_tdls;
    vap->iv_tdlslist->tdlslist_vap = vap;  
    vap->iv_tdlslist->tdls_count = 0;
    OS_RWLOCK_INIT(&vap->iv_tdlslist->tdls_lock);
    TAILQ_INIT(&vap->iv_tdlslist->tdls_node);

    /* attach functions */
    vap->iv_tdls_ops.tdls_detach = ieee80211_tdls_detach;
    vap->iv_tdls_ops.tdls_wpaftie = ieee80211_wpa_tdls_ftie;
    ieee80211_vap_register_event_handler(vap, ieee80211tdls_vap_event_handler, NULL);
    }
    
    return 0;
}

int 
ieee80211_tdls_sm_detach(struct ieee80211com *ic, struct ieee80211vap *vap)
{
    wlan_tdls_sm_t sm;
    wlan_tdls_sm_t smt;
    struct ieee80211_tdls *td;
    rwlock_state_t lock_state;

    td = ic->ic_tdls;
	if(!td)
	    return 0;

    sm = smt = NULL;
    OS_RWLOCK_WRITE_LOCK(&td->tdls_lock, &lock_state);
    TAILQ_FOREACH_SAFE(sm,  &td->tdls_node, tdlslist, smt) {
        if (sm) {
            ieee80211_tdls_delentry(ic, sm->tdls_ni);
        }
    }
    OS_RWLOCK_WRITE_UNLOCK(&td->tdls_lock, &lock_state);

    return 0;
}

int 
ieee80211_tdls_detach(struct ieee80211com *ic)
{
    struct ieee80211_tdls *td;

    if (!ic) return -EFAULT;
    
#if CONFIG_RCPI
        ieee80211_tdlsrcpi_timer_stop(ic, (unsigned long)ic->ic_tdls);
#endif
    td = ic->ic_tdls;
    if(td->tdlslist_vap)
        ieee80211_tdls_sm_detach(ic, td->tdlslist_vap);
    
    OS_FREE(ic->ic_tdls); ic->ic_tdls=NULL;
    
    return 0;
}

/* Delete tdls node */
void 
ieee80211_tdls_del_node(struct ieee80211_node *ni)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_tdls *td;
    wlan_tdls_sm_t sm;
    rwlock_state_t lock_state;

    td = vap->iv_ic->ic_tdls;

    /* clear entry from state machine */
    OS_RWLOCK_WRITE_LOCK(&td->tdls_lock, &lock_state);
    sm = ieee80211_tdls_find_byaddr(vap->iv_ic, ni->ni_macaddr);
    if (sm) {
        /*remove this entry if it exists, reset the state machine for this node*/
        ieee80211_tdls_delentry(vap->iv_ic, ni);       
    }
    OS_RWLOCK_WRITE_UNLOCK(&td->tdls_lock, &lock_state);
    
    /* Reomve only TDLS nodes here */
    if (ni->ni_flags & IEEE80211_NODE_TDLS) {
        
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "Deleting the node..."
                          " refcnt:%d\n", ieee80211_node_refcnt(ni));
        IEEE80211_NODE_LEAVE(ni);
    }

}

static u8 * ieee80211_tdls_offset(struct ieee80211_tdls_frame * tf)
{
  	u8 * sbuf = NULL;

    /* Parse thro frame to get position of IEs */
    switch(tf->action) {
    case IEEE80211_TDLS_SETUP_REQ:
        sbuf = (u8 *)( (u8*)(tf+1)
             + sizeof(struct ieee80211_tdls_token)
             + sizeof(u16) /* capinfo */);
    break;
    case IEEE80211_TDLS_SETUP_RESP:
        sbuf = (u8 *)( (u8*)(tf+1) + 
             + sizeof(struct ieee80211_tdls_status)
             + sizeof(struct ieee80211_tdls_token) 
             + sizeof(u16) /* capinfo */);
    break;
    case IEEE80211_TDLS_SETUP_CONFIRM:
        sbuf = (u8 *)( (u8*)(tf+1) + 
             + sizeof(struct ieee80211_tdls_status)
             + sizeof(struct ieee80211_tdls_token));
    break;
    case IEEE80211_TDLS_TEARDOWN:
        sbuf = (u8 *)( (u8*)(tf+1) + 
             + sizeof(struct ieee80211_tdls_status) );
    break;
    case IEEE80211_TDLS_PEER_TRAFFIC_INDICATION:
        sbuf = (u8 *) ( (u8*)(tf+1) +
                sizeof(struct ieee80211_tdls_token) );
        break;
    case IEEE80211_TDLS_PEER_TRAFFIC_RESPONSE:
        sbuf = (u8 *) ( (u8*)(tf+1) +
                sizeof(struct ieee80211_tdls_token) );
        break;
    case IEEE80211_TDLS_CHANNEL_SX_REQ:
        sbuf = (u8 *) ( (u8*)(tf+1) +
                + sizeof(u8) /* Target Channel */
                + sizeof(u8) /* Regulatory Class */ );
        break;
    case IEEE80211_TDLS_CHANNEL_SX_RESP:
        sbuf = (u8 *) ( (u8*)(tf+1) +
                + sizeof(struct ieee80211_tdls_status) );
        break;
    default:
        /* Default: Should not hit */
        sbuf = NULL;
    break;
    }
    
    return sbuf;    
}

static void
ieee80211tdls_vap_event_handler(ieee80211_vap_t     vap,
                                ieee80211_vap_event *event,
                                void                *arg)
{
    struct ieee80211com *ic = vap->iv_ic; 
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: event=%d\n",
        __func__, event->type);
    switch(event->type) {
    case IEEE80211_VAP_DOWN:
        ieee80211_tdls_detach(ic);
        break;

    default:
        break;
    }
}

/*
 * Parse WMM parameter element received in the Setup Confirm action frame.
 */
static int
tdls_parse_wmm_qosinfo(u_int8_t *frm, struct ieee80211_node *ni)
{
    u_int len = frm[1];
    u_int8_t ac;

    if (len < 7) {
        IEEE80211_DISCARD_IE(ni->ni_vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WME,
            "WME IE", "too short, len %u", len);
        return -1;
    }
    ni->ni_uapsd = frm[WME_CAPINFO_IE_OFFSET];

    if (ni->ni_uapsd) {
        ni->ni_flags |= IEEE80211_NODE_UAPSD;
        switch (WME_UAPSD_MAXSP(ni->ni_uapsd)) {
        case 1:
            ni->ni_uapsd_maxsp = 2;
            break;
        case 2:
            ni->ni_uapsd_maxsp = 4;
            break;
        case 3:
            ni->ni_uapsd_maxsp = 6;
            break;
        default:
            ni->ni_uapsd_maxsp = WME_UAPSD_NODE_MAXQDEPTH;
        }

        for (ac = 0; ac < WME_NUM_AC; ac++) {
            ni->ni_uapsd_ac_trigena[ac] = (WME_UAPSD_AC_ENABLED(ac, ni->ni_uapsd)) ? 1:0;
            ni->ni_uapsd_ac_delivena[ac] = (WME_UAPSD_AC_ENABLED(ac, ni->ni_uapsd)) ? 1:0;
        }
    }

    IEEE80211_NOTE(ni->ni_vap, IEEE80211_MSG_TDLS, ni,
        "UAPSD bit settings from STA: %02x", ni->ni_uapsd);

    return 1;
}

u8 
ieee80211_tdls_processie(struct ieee80211_node *ni, 
        struct ieee80211_tdls_frame * tf, u16 len) {

#if ATH_DEBUG
    struct ieee80211vap *vap = ni->ni_vap;
#endif
  	u8 * sbuf = NULL;
  	u8 * ebuf = NULL;
    u_int8_t  *rates, *xrates, *htcap, *extcap, *extcap2, *wme;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
        "Updating IEs for MAC:%s\n", ether_sprintf(ni->ni_macaddr));

    rates = xrates = htcap = extcap = extcap2 = wme = NULL;

    if(!tf) return -EINVAL;

    if (tf) {
	    sbuf = (u8*)(tf);
  	    ebuf = sbuf + len;
        sbuf = ieee80211_tdls_offset(tf);
    }

    while (sbuf < ebuf) {
        switch (*sbuf) {
        case IEEE80211_ELEMID_RATES:
            rates = sbuf;
            break;
        case IEEE80211_ELEMID_XRATES:
            xrates = sbuf;
            break;
        case IEEE80211_ELEMID_HTCAP_ANA:
            if (htcap == NULL) {
                htcap = (u_int8_t *)&((struct ieee80211_ie_htcap *)sbuf)->hc_ie;
            }
            break;
        case IEEE80211_ELEMID_VENDOR:
            if(ishtcap(sbuf)) {
                if (htcap == NULL) {
                    htcap = (u_int8_t *)&((struct vendor_ie_htcap *)sbuf)->hc_ie;
                }
            }
            else if (iswmeinfo(sbuf) || iswmeparam(sbuf)) {
                wme = sbuf;
            }

            break;
        case IEEE80211_ELEMID_XCAPS:
            extcap = (u_int8_t *) &((struct ieee80211_ie_ext_cap *)sbuf)->ext_capflags;
            break;
        }
        sbuf += sbuf[1] + 2;
    }

    if ((wme != NULL) && tdls_parse_wmm_qosinfo(wme, ni) > 0) {
        ieee80211node_set_flag(ni, IEEE80211_NODE_QOS);
    }
    else {
        ieee80211node_clear_flag(ni, IEEE80211_NODE_QOS);
    }

    if (tf->action != IEEE80211_TDLS_SETUP_CONFIRM &&
	(!rates || (rates[1] > IEEE80211_RATE_MAXSIZE))) {
        /* XXX: msg + stats */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
            "%s: Failed to get HTRATE or rate set mismatch for MAC:%s\n", 
            __func__, ether_sprintf(ni->ni_macaddr));
        return -EINVAL;
    }

    if (tf->action != IEEE80211_TDLS_SETUP_CONFIRM &&
	!ieee80211_setup_rates(ni, rates, xrates,
        IEEE80211_F_DOSORT | IEEE80211_F_DOFRATE |
        IEEE80211_F_DOXSECT)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
            "%s: Failed to update HTRATE for MAC:%s\n",
            __func__, ether_sprintf(ni->ni_macaddr));
        return -EINVAL;
    }

    if(htcap) {
        /*
         * Flush any state from a previous handshake.
         */
        ieee80211node_clear_flag(ni, IEEE80211_NODE_HT);
        IEEE80211_NODE_CLEAR_HTCAP(ni);

        /* record capabilities, mark node as capable of HT */
        ieee80211_parse_htcap(ni, htcap);

        if (!ieee80211_tdls_setup_ht_rates(ni, htcap,
            IEEE80211_F_DOFRATE | IEEE80211_F_DOXSECT |
            IEEE80211_F_DOBRS)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
            "%s: Failed to update HTCAP for MAC:%s\n", 
                __func__, ether_sprintf(ni->ni_macaddr));
            return -EINVAL;
        }
    } else if (tf->action != IEEE80211_TDLS_SETUP_CONFIRM) {
        /*
         * Flush any state from a previous handshake.
         */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
            "%s: Failed: flushing HTRATE for MAC:%s\n", 
            __func__, ether_sprintf(ni->ni_macaddr));

        ni->ni_flags &= ~IEEE80211_NODE_HT;
        ni->ni_htcap = 0;
    }

    /* Check Extended Capabilities for TDLS parameters 
     * and add to TDLS node flags
     */
    if (extcap || extcap2) {
        /* Clear any old capabilities */
        /* TODO: use extcap2: check for 37-bit */
        /* ieee80211node_clear_flag(ni, IEEE80211_NODE_TDLS); */
        ieee80211node_clear_flag(ni, IEEE80211_NODE_UAPSD);
        
        ni->ni_tdls->flags = 0;
        
        ieee80211_process_extcap(ni, extcap);
    } else if (tf->action != IEEE80211_TDLS_SETUP_CONFIRM) {
        ieee80211node_clear_flag(ni, IEEE80211_NODE_TDLS);
        ieee80211node_clear_flag(ni, IEEE80211_NODE_UAPSD);
        
        ni->ni_tdls->flags = 0;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
        "niflags:%x, HTCAP:%x, CAPINFO:%x \n",
        ni->ni_flags, ni->ni_htcap, ni->ni_capinfo);

    tdls_rate_updatenode(ni);

    return 0;
}

u8 * 
ieee80211_tdls_get_ie(struct ieee80211_tdls_frame * tf, u8 elementid, u16 len) {

  	u8 * sbuf = NULL;
  	u8 * eof  = NULL;

    if (tf) {
	    sbuf = (u8*)(tf);
  	    eof  = sbuf + len;
        sbuf = ieee80211_tdls_offset(tf);

    	do {
            if(*sbuf == elementid) {
                /* printk(">>> %s : Found Element ID : %d \n",__func__, elementid); */
                break;
            } else {
                sbuf++; /* To access len */
                sbuf += *sbuf;
                sbuf++; /* To offset ie */
            }
        } while((eof-sbuf) > 0);

        if(*sbuf != elementid)
            sbuf = NULL;
    }
    return sbuf;
}

u8 * 
ieee80211_tdls_get_iebuf(u8* buf, u8 elementid, u8 len) {

  	u8 * sbuf = NULL;
  	u8 * eof  = NULL;

    if (buf) {
	    sbuf = buf;
  	    eof  = sbuf + len;

    	do {
            if(*sbuf == elementid) {
                /* printk(">>> %s : Found Element ID : %d \n",__func__, elementid); */
                break;
            } else {
                sbuf++; /* To access len */
                sbuf += *sbuf;
                sbuf++; /* To offset ie */
            }
        } while((eof-sbuf) > 0);

        if(*sbuf != elementid)
            sbuf = NULL;
    }
    return sbuf;
}

static void 
tdls_notify_supplicant(struct ieee80211vap *vap, char *mac, 
		enum ieee80211_tdls_action type, struct ieee80211_tdls_frame * tf, u16 _len)
{
    u16 len = 0;
    u8 * _buf = NULL;
    static const char *tag = "STKSTART.request=";
    struct net_device *dev = ieee80211_tdls_get_dev(vap);
    union iwreq_data wrqu;
    char buf[512], *cur_buf;

	if (type != IEEE80211_TDLS_TEARDOWN &&
	    type != IEEE80211_TDLS_SETUP_REQ)
		return;

	/* Just send the plain frame TDLS_Hdr followed by IE*/
	_buf = (u8*)tf;
	len = _len;
	
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,  "tdls_notify_supplicant: %s _len=%d\n",
                      ether_sprintf(mac), _len);

	cur_buf = buf;

    snprintf(cur_buf, sizeof(buf), "%s", tag);
	cur_buf += strlen(tag);

	memcpy(cur_buf, mac, ETH_ALEN);
	cur_buf += ETH_ALEN;

	*(uint16_t*)cur_buf = type;
	cur_buf += 2;

	*(uint16_t*)cur_buf = (u16)len;
	cur_buf += 2;

	if(_buf && len) {
	    /* len is checked to avoid buffer overflow attack */
		if(len <= sizeof(buf) + buf - cur_buf) {
	        memcpy(cur_buf, _buf, len);
            cur_buf += len;
    	} else {
	        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"Message length "
				"too high, len=%d\n", len);
	        return;
	    }
	}

    memset(&wrqu, 0, sizeof(wrqu));
    wrqu.data.length = cur_buf - buf;

	IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: %s: %s, %x len:%d\n", 
                      __FUNCTION__, ether_sprintf(mac), buf, type, wrqu.data.length);

	/* HACK in driver to get around IWEVCUSTOM size limit */
	/* 
	 * IWEVASSOCREQIE: Linux wireless extension events are not really
	 * going to work very well in some cases. Its assumed, this doesn't apply 
	 * to target systems with 64-bit kernel and 32-bit userspace, if so, 
	 * please note that this will break.
	 *
	 */
	wireless_send_event(dev, IWEVASSOCREQIE, &wrqu, buf);
}


/* Request Supplicant to send FTIE for TDLS TearDown message
   */
void 
tdls_request_teardown_ftie(struct ieee80211vap *vap, char *mac, 
                            enum ieee80211_tdls_action type, 
                            u16 reason_code, u8 dtoken)
{
    static const char *tag = "STKSTART.request=";
    struct net_device *dev = ieee80211_tdls_get_dev(vap);
    union iwreq_data wrqu;
    char buf[512], *cur_buf;

	cur_buf = buf;

	snprintf(cur_buf, sizeof(buf), "%s", tag);
	cur_buf += strlen(tag);

	memcpy(cur_buf, mac, ETH_ALEN);
	cur_buf += ETH_ALEN;

	*(uint16_t*)cur_buf = type;
	cur_buf += 2;

	*(uint16_t*)cur_buf = sizeof(reason_code); 
	cur_buf += 2;

	/* as supplicant reads dialogue token from Setup Req/Resp, 
	 * send only reason_code for Teardown*/
	*(uint16_t*)cur_buf = reason_code;
	cur_buf += 2;

	memset(&wrqu, 0, sizeof(wrqu));
	wrqu.data.length = cur_buf - buf;

	IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: %s: %s, %x len:%d\n", 
                      __FUNCTION__, ether_sprintf(mac), buf, type, wrqu.data.length);

	/* HACK in driver to get around IWEVCUSTOM size limit */
	/* 
	 * IWEVASSOCREQIE: Linux wireless extension events are not really
	 * going to work very well in some cases. Its assumed, this doesn't apply 
	 * to target systems with 64-bit kernel and 32-bit userspace, if so, 
	 * please note that this will break.
	 *
	 */
	wireless_send_event(dev, IWEVASSOCREQIE, &wrqu, buf);
}

#if 0
/* Notification to TdlsDiscovery process on TearDown from Supplicant 
   also used on other occassions like channel change and others
   */
void 
tdls_notify_teardown(struct ieee80211vap *vap, char *mac, 
                            enum ieee80211_tdls_action type, 
                            struct ieee80211_tdls_frame *tf)
{
    uint16_t len = 0;
    static const char *tag = "STKSTART.request=TearDown";
    struct net_device *dev = ieee80211_tdls_get_dev(vap);
    union iwreq_data wrqu;
    char buf[512], *cur_buf;

	cur_buf = buf;

	snprintf(cur_buf, sizeof(buf), "%s", tag);
	cur_buf += strlen(tag);

	memcpy(cur_buf, mac, ETH_ALEN);
	cur_buf += ETH_ALEN;

	*(uint16_t*)cur_buf = type;
	cur_buf += 2;

	*(uint16_t*)cur_buf = len;
	cur_buf += 2;

	memset(&wrqu, 0, sizeof(wrqu));
	wrqu.data.length = cur_buf - buf;

	IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: %s: %s, %x len:%d\n", 
                      __FUNCTION__, ether_sprintf(mac), buf, type, wrqu.data.length);

	/* HACK in driver to get around IWEVCUSTOM size limit */
	/* 
	 * IWEVASSOCREQIE: Linux wireless extension events are not really
	 * going to work very well in some cases. Its assumed, this doesn't apply 
	 * to target systems with 64-bit kernel and 32-bit userspace, if so, 
	 * please note that this will break.
	 *
	 */
	wireless_send_event(dev, IWEVASSOCREQIE, &wrqu, buf);
}
#endif

/** 
 * check wpa/rsn ie is present in the ie buffer passed in. 
 */
static bool ieee80211_check_wpaie(struct ieee80211vap *vap, u_int8_t *iebuf, u_int32_t length)
{
    u_int8_t *iebuf_end = iebuf + length;
    struct ieee80211_rsnparms tmp_rsn;
    bool add_wpa_ie = true;
    while (add_wpa_ie && ((iebuf+1) < iebuf_end )) {
        if (iebuf[0] == IEEE80211_ELEMID_VENDOR) {
            if (iswpaoui(iebuf) &&  (ieee80211_parse_wpa(vap, iebuf, &tmp_rsn) == 0)) {
                if (RSN_CIPHER_IS_CLEAR(&vap->iv_rsn) || vap->iv_rsn.rsn_ucastcipherset == 0) {
                    vap->iv_rsn = tmp_rsn;
                }
                /* found WPA IE */
                add_wpa_ie = false;
            }
        }
        else if(iebuf[0] == IEEE80211_ELEMID_RSN) {
            if (ieee80211_parse_rsn(vap, iebuf, &tmp_rsn) == 0) {
                /* found RSN IE */
                if (RSN_CIPHER_IS_CLEAR(&vap->iv_rsn) || vap->iv_rsn.rsn_ucastcipherset == 0) {
                vap->iv_rsn = tmp_rsn;
                }
                add_wpa_ie = false;
            }
        }
        iebuf += iebuf[1] + 2;
        continue;
    }
    return add_wpa_ie;
}           

void 
ieee80211tdls_add_extcap(struct ieee80211_node *ni, u_int32_t *ext_capflags)
{
    /* Check for Tunneled Direct Link Setup (TDLS) capable node */
    if (IEEE80211_TDLS_ENABLED(ni->ni_vap)) {
        /* todo[GDS]:  Add TDLS support bit into extended capabilities field */
        *ext_capflags |= IEEE80211_EXTCAPIE_TDLSSUPPORT; 
        
        if (IEEE80211_IS_TDLS_PEER_UAPSD_ENABLED(ni)) {
            /* Advertise support for TDLS Peer U-APSD Power Save */
            *ext_capflags |= IEEE80211_EXTCAPIE_PEER_UAPSD_BUF_STA;            
        }
        
        if (ieee80211_vap_off_channel_support_is_set(ni->ni_vap)) {
            /* Advertise support for TDLS channel switch */
            *ext_capflags |= IEEE80211_EXTCAPIE_TDLS_CHAN_SX;
        }
    }
    
    return;
}

void
ieee80211tdls_process_extcap_ie(struct ieee80211_node *ni, u_int32_t extcap)
{
    /* todo[gstanton]: Add if (extcap & IEEE80211_EXTCAPIE_TDLS_SUPPORT) */
  
    if (IEEE80211_IS_TDLS_PEER_UAPSD_ENABLED(ni)) {           
        /* PEER U-APSD bit is ONLY relevant for TDLS at present */
        if (extcap & IEEE80211_EXTCAPIE_PEER_UAPSD_BUF_STA) {               
            ieee80211node_set_flag(ni, IEEE80211_NODE_UAPSD);                
            ieee80211tdls_set_flag(ni, IEEE80211_TDLS_PEER_UAPSD_BUF_STA);

            /* Set Node extended capabilities */
            ni->ni_ext_caps |= IEEE80211_NODE_C_QOS;
            ni->ni_ext_caps |= IEEE80211_NODE_C_UAPSD;
        }
        else {
            ieee80211node_clear_flag(ni, IEEE80211_NODE_UAPSD);
            ieee80211tdls_clear_flag(ni, IEEE80211_TDLS_PEER_UAPSD_BUF_STA);
        }
    }
    else {
        ieee80211node_clear_flag(ni, IEEE80211_NODE_UAPSD);
        ieee80211tdls_clear_flag(ni, IEEE80211_TDLS_PEER_UAPSD_BUF_STA);
        ni->ni_ext_caps &= ~(IEEE80211_NODE_C_QOS | IEEE80211_NODE_C_UAPSD);
    }
        if (ieee80211_vap_off_channel_support_is_set(ni->ni_vap)) {
        /* The channel switch bit is ONLY relevant for TDLS */
        if (extcap & IEEE80211_EXTCAPIE_TDLS_CHAN_SX) {
            ieee80211tdls_set_flag(ni, IEEE80211_TDLS_CHANNEL_SWITCH);
       }
    }
    
    return;
}

/*
 * Add a WME Info element to a frame. It must be customized for TDLS.
 */
static u_int8_t *
tdls_add_wmeinfo(u_int8_t *frm, struct ieee80211_node *ni_tdls,
                      u_int8_t wme_subtype, u_int8_t *wme_info, u_int8_t info_len)
{
    static const u_int8_t oui[4] = { WME_OUI_BYTES, WME_OUI_TYPE };
    struct ieee80211_ie_wme *ie = (struct ieee80211_ie_wme *) frm;

    *frm++ = IEEE80211_ELEMID_VENDOR;
    *frm++ = 0;                             /* length filled in below */
    OS_MEMCPY(frm, oui, sizeof(oui));       /* WME OUI */
    frm += sizeof(oui);
    *frm++ = wme_subtype;          /* OUI subtype */
    switch (wme_subtype) {
    case WME_INFO_OUI_SUBTYPE:
        *frm++ = WME_VERSION;                   /* protocol version */
        /* QoS Info field depends on operating mode */
        ie->wme_info = 0;
        ie->wme_info |= ni_tdls->ni_tdls->wme_info_uapsd;
        
        frm++;
        break;
    case WME_TSPEC_OUI_SUBTYPE:
        *frm++ = WME_TSPEC_OUI_VERSION;        /* protocol version */
        OS_MEMCPY(frm, wme_info, info_len);
        frm += info_len;
        break;
    default:
        break;
    }

    ie->wme_len = (u_int8_t)(frm - &ie->wme_oui[0]);

    return frm;
}

/*
 * Add 802.11h information elements to a frame.
 * Custom made for TDLS to avoid Power Capabilities.
 */
static u_int8_t *
ieee80211_tdls_add_doth(u_int8_t *frm, struct ieee80211vap *vap)
{
    struct ieee80211_channel *c;
    int    i, j, chancnt;
    u_int8_t chanlist[IEEE80211_CHAN_MAX + 1];
    u_int8_t prevchan;
    u_int8_t *frmbeg;
    struct ieee80211com *ic = vap->iv_ic;

        /*
         * Supported Channels IE as per 802.11h-2003.
         */
    frmbeg = frm;
    prevchan = 0;
    chancnt = 0;

    for (i = 0; i < ic->ic_nchans; i++)
    {
        c = &ic->ic_channels[i];

        /* Skip turbo channels */
        if (IEEE80211_IS_CHAN_TURBO(c))
            continue;

        /* Skip half/quarter rate channels */
        if (IEEE80211_IS_CHAN_HALF(c) || IEEE80211_IS_CHAN_QUARTER(c))
            continue;

        /* Skip previously reported channels */
        for (j=0; j < chancnt; j++) {
            if (c->ic_ieee == chanlist[j])
                break;
                }
        if (j != chancnt) /* found a match */
            continue;

        chanlist[chancnt] = c->ic_ieee;
        chancnt++;

        if ((c->ic_ieee == (prevchan + 1)) && prevchan) {
            frm[1] = frm[1] + 1;
        } else {
            frm += 2;
            frm[0] =  c->ic_ieee;
            frm[1] = 1;
        }

        prevchan = c->ic_ieee;
    }

    frm += 2;

    if (chancnt) {
        frmbeg[0] = IEEE80211_ELEMID_SUPPCHAN;
        frmbeg[1] = (u_int8_t)(frm - frmbeg - 2);
    } else {
        frm = frmbeg;
    }

    return frm;
}
static u_int8_t *ieee80211_tdls_add_chanrep(u_int8_t *frm, struct ieee80211com *ic)
{
    int i;
    *frm++ = 221; /* IEEE80211_ELEMID_REGCLASS */
    *frm++ = ic->ic_nregclass; /* for reg class */;
    for (i = 0; i < ic->ic_nregclass; i++) {
        *frm++ = ic->ic_regclassids[i];
    }
    return frm;
}

static u_int8_t *ieee80211_tdls_add_timoutie(u_int8_t *frm)
{
   struct ieee80211_ie_timeout_interval *ie = (struct ieee80211_ie_timeout_interval *) frm;
   memset(ie, 0, sizeof(struct ieee80211_ie_timeout_interval));

   ie->elem_id = IEEE80211_ELEMID_TIMEOUT_INTERVAL;
   ie->elem_len = sizeof (struct ieee80211_ie_timeout_interval)-2;
   ie->interval_type = FTIE_TYPE_LIFETIME_INTERVAL;
   ie->value = 43200; // 12 hours
   return frm + sizeof(struct ieee80211_ie_timeout_interval);
}

/*
 * Setup IEs for TDLS Setup Request/Response frames,
 * and returns the frame length.
 * TDLS: Modification for Draft13.0
 */
static u_int16_t
ieee80211_tdls_setup_cap(
    struct ieee80211_node *ni,
    struct ieee80211_frame *wh,
    u_int8_t *previous_bssid,
    struct ieee80211_node *ni_tdls
    )
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic = ni->ni_ic;
    u_int8_t *frm;
    u_int16_t capinfo = 0;

    frm = (u_int8_t *)wh;

	/*
	 * TDLS fixed parameters for Setup Request/Response()
	 * [2] - Capability
	 * Variable list of IEs in order
	 * [tlv] - Supported Rates
	 * [tlv] - Country IE
	 * [tlv] - Extended supported rates
	 * [tlv] - Supported Channels
	 */
    if (vap->iv_opmode == IEEE80211_M_IBSS)
        capinfo |= IEEE80211_CAPINFO_IBSS;
    else if (vap->iv_opmode == IEEE80211_M_STA || vap->iv_opmode == IEEE80211_M_BTAMP)
        capinfo |= IEEE80211_CAPINFO_ESS;
    else
        ASSERT(0);
    
    if (IEEE80211_VAP_IS_PRIVACY_ENABLED(vap))
        capinfo |= IEEE80211_CAPINFO_PRIVACY;
    /*
     * NB: Some 11a AP's reject the request when
     *     short premable is set.
     */
    if (IEEE80211_IS_CHAN_2GHZ(ic->ic_curchan))
        capinfo |= IEEE80211_CAPINFO_SHORT_PREAMBLE;
    if (ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_SLOTTIME)
        capinfo |= IEEE80211_CAPINFO_SHORT_SLOTTIME;
    if (ieee80211_ic_doth_is_set(ic) && ieee80211_vap_doth_is_set(vap))
        capinfo |= IEEE80211_CAPINFO_SPECTRUM_MGMT;

	/* Add capinfo 16-bit information */
    *(u_int16_t *)frm = htole16(capinfo);
    frm += 2;

    /* Add Supported Rates */
    frm = ieee80211_add_rates(frm, &ni->ni_rates);
    
    /* Add Country IE */
    if (IEEE80211_IS_COUNTRYIE_ENABLED(ic)) {
        frm = ieee80211_add_country(frm, vap);
    }

    /* Add Extended Supported Rates */
    frm = ieee80211_add_xrates(frm, &ni->ni_rates);

    /*
     * DOTH elements
     */
    if (ieee80211_ic_doth_is_set(ic) && ieee80211_vap_doth_is_set(vap)) {
        frm = ieee80211_tdls_add_doth(frm, vap);
    }

    /* Insert ieee80211_ie_ath_extcap IE */
    if ((ni->ni_flags & IEEE80211_NODE_ATH) &&
        ic->ic_ath_extcap) {
        u_int16_t ath_extcap = 0;
        u_int8_t  ath_rxdelim = 0;
        
        if (ieee80211_has_weptkipaggr(ni)) {
                ath_extcap |= IEEE80211_ATHEC_WEPTKIPAGGR;
                ath_rxdelim = ic->ic_weptkipaggr_rxdelim;
        }

        if ((ni->ni_flags & IEEE80211_NODE_OWL_WDSWAR) &&
            (ic->ic_ath_extcap & IEEE80211_ATHEC_OWLWDSWAR)) {
                ath_extcap |= IEEE80211_ATHEC_OWLWDSWAR;
        }

        if (ieee80211com_has_extradelimwar(ic))
            ath_extcap |= IEEE80211_ATHEC_EXTRADELIMWAR;

        frm = ieee80211_add_athextcap(frm, ath_extcap, ath_rxdelim);
    }

#ifdef notyet
    if (ni->ni_ath_ie != NULL)
        frm = ieee80211_add_ath(frm, ni);
#endif

    /*
     * Add WMM Information Element
     */
    if ((ni_tdls->ni_ext_caps & IEEE80211_NODE_C_QOS)) {
        frm = tdls_add_wmeinfo(frm, ni_tdls, WME_INFO_OUI_SUBTYPE, NULL, 0);
    }

    return (frm - (u_int8_t *)wh);
}

/* Make frame hdrs */
static uint8_t * 
tdls_mgmt_frame(struct ieee80211vap *vap, uint8_t type, 
                                  void *arg, wbuf_t *wbuf)
{
    uint8_t *frm=NULL;

    /* *skb = ieee80211_getmgtframe(&frm, sizeof(struct ieee80211_tdls_frame) 
                                       + frame_length[type]+2, 
                                 sizeof(struct ether_header)); */

    *wbuf = wbuf_alloc(vap->iv_ic->ic_osdev, WBUF_TX_MGMT, 
    		sizeof(struct ieee80211_tdls_frame) + frame_length[type]+2);

    if (!*wbuf) {
	return NULL;
    }
	frm = wbuf_raw_data(*wbuf);
	
    /* Fill the LLC/SNAP header and set ethertype to TDLS ether type */
    IEEE80211_TDLS_LLC_HDR_SET(frm); /* nothing happens here, its dummy as of now */

    /* Fill the TDLS header */
    IEEE80211_TDLS_HDR_SET(frm, type, IEEE80211_TDLS_LNKID_LEN, vap, arg);

    return frm;
};

/* Send tdls mgmt frame */
static uint8_t 
tdls_send_mgmt_frame(struct ieee80211com *ic, wbuf_t wbuf)
{
    struct ieee80211_node *ni = wbuf_get_node(wbuf);
    struct ieee80211vap *vap  = ni->ni_vap;
    uint8_t ret=0;

    IEEE80211_NODE_STAT(ni, tx_data);

    wbuf_set_priority(wbuf, WME_AC_BK);
    wbuf_set_tid(wbuf, WME_AC_TO_TID(WME_AC_BK));
    /* ni->ni_flags |= N_TDLS; */ /*TODO: to make ath_dev use this flag */
    ret = vap->iv_evtable->wlan_dev_xmit_queue(vap->iv_ifp, wbuf);
	
    if (ret) ieee80211_free_node(ni);

    return ret; 
}

/* Create the Wireless ARP message for WDS and send it out to all the
 * TDLS nodes */
static int 
tdls_send_arp(struct ieee80211com *ic, struct ieee80211vap *vap, 
                  void *arg, uint8_t *buf, uint16_t buf_len, u_int16_t status)
{
    wbuf_t wbuf;
    uint8_t              *frm;
    struct ieee80211_node *ni = vap->iv_bss;
    uint8_t  bcast_addr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    uint16_t               len=0;

    /* If TDLS not enabled, don't send any ARP messages */
    if (!(vap->iv_ath_cap & IEEE80211_ATHC_TDLS)) {
        printk("TDLS Disabled \n");
        return 0;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, Sending out WDS ARP for %s "
                      "behind me\n", __FUNCTION__, ether_sprintf(arg));
 
    frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_LEARNED_ARP, bcast_addr,
                              &wbuf);
    if (!frm) {
       printk("frm returns NULL\n");
       return -ENOMEM;
    }

	len = sizeof(struct ieee80211_tdls_frame);

    IEEE80211_ADDR_COPY(frm, arg);
    len += 6;

    N_NODE_SET(wbuf, ieee80211_ref_node(ni));

    wbuf_set_pktlen(wbuf, len);

    IEEE80211_TDLS_WHDR_SET(wbuf, vap, bcast_addr);

	/* temporary to be removed later */
	dump_pkt((wbuf->data), wbuf->len);

    tdls_send_mgmt_frame(ic, wbuf);

    return 0;
}

/* TDLS Peer Traffic Indication */
static int
tdls_send_peer_traffic_indication(struct ieee80211com *ic, struct ieee80211vap *vap,
                        void* arg, uint8_t *buf, uint16_t buf_len, u_int16_t status)
{
    struct ieee80211_node *ni = vap->iv_bss;
    struct ieee80211_node *ni_tdls;
    uint8_t               *frm;
    uint8_t                _token = IEEE80211_TDLS_TOKEN;
    uint16_t               len=0;
    wbuf_t                 wbuf;
    peer_uapsd_buf_sta_t  *buf_sta;

    ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, arg);

    if (!ni_tdls) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"%s,Node not present:%s\n",
                          __FUNCTION__, ether_sprintf(arg));
        return 0;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "%s, Send Peer Traffic Indication Message to %s\n", __FUNCTION__,
                      ether_sprintf(arg));
    
    buf_sta = &ni_tdls->ni_tdls->w_tdls->peer_uapsd.buf_sta;
    _token = ieee80211tdls_pu_buf_sta_get_dialog_token(buf_sta);
    ni_tdls->ni_tdls->token  = _token; /* Dialogue Token from Initiator */
    ni_tdls->ni_tdls->stk_st = 0;

    frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_PEER_TRAFFIC_INDICATION, arg, &wbuf);

    if (!frm) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, "
                             "unable to allocate wbuf\n", __FUNCTION__);
	ieee80211_free_node(ni_tdls); /* for above find */
        return -ENOMEM;
    }

    /* Add dialog token */
    IEEE80211_TDLS_SET_TOKEN(frm, vap, arg, _token);

    /* Add Link Identifier IE */
    if (!IEEE80211_TDLS_IS_INITIATOR(ni_tdls)) {
         IEEE80211_TDLS_SET_LINKID(frm, vap, arg);
         IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                       "%s: Peer Traffic Indication Message to Responder: %s\n",
                       __func__, ether_sprintf(arg));
     }
     else  {
         IEEE80211_TDLS_SET_LINKID_RESP(frm, vap, arg);
         IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                       "%s: Peer Traffic Indication Message to Initiator: %s\n",
                       __func__, ether_sprintf(arg));
     }

    /* Add PU Buffer Status IE */
    IEEE80211_TDLS_SET_PU_BUFFER_STATUS(frm, ni_tdls->ni_tdls->wme_info_uapsd);
    ieee80211_free_node(ni_tdls); /* for above find */

    len = frm - (u_int8_t *) wbuf_header(wbuf);

    IEEE80211_NODE_STAT(ni, tx_data);

    N_NODE_SET(wbuf, ieee80211_ref_node(ni));

    wbuf_set_pktlen(wbuf, len);

    IEEE80211_TDLS_WHDR_SET(wbuf, vap, arg);

    tdls_send_mgmt_frame(ic, wbuf);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
        "SEND_TDLS_PEER_TRAFFIC_INDICATION: successfully sent ... \n", len);

    return 0;
}

/* TDLS Peer Traffic Response */
static int
tdls_send_peer_traffic_response(struct ieee80211com *ic, struct ieee80211vap *vap,
                        void* arg, uint8_t *buf, uint16_t buf_len, u_int16_t status)
{
    struct ieee80211_node *ni_tdls;
    uint8_t               *frm;
    uint16_t               len=0;
    wbuf_t                 wbuf;
    peer_uapsd_sleep_sta_t *sleep_sta;
    u_int8_t               token;	
	
    ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, arg);
    sleep_sta = &ni_tdls->ni_tdls->w_tdls->peer_uapsd.sleep_sta;
    if (!ni_tdls) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"%s,Node not present:%s\n",
                          __FUNCTION__, ether_sprintf(arg));
        return 0;
    }
    token = ieee80211tdls_pu_sleep_sta_get_dialog_token(sleep_sta);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "%s, Send Peer Traffic Response Message to %s\n", __FUNCTION__,
                      ether_sprintf(arg));

    frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_PEER_TRAFFIC_RESPONSE, arg, &wbuf);

    if (!frm) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, "
                             "unable to allocate wbuf\n", __FUNCTION__);
	ieee80211_free_node(ni_tdls); /* for above find */
        return -ENOMEM;
    }

    /* Add dialog token */
    IEEE80211_TDLS_SET_TOKEN(frm, vap, arg, token);
    if (!IEEE80211_TDLS_IS_INITIATOR(ni_tdls)) {
        IEEE80211_TDLS_SET_LINKID(frm, vap, arg);
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                       "%s: Peer Traffic Response Message to Responder: %s\n",
                       __func__, ether_sprintf(arg));
     }
     else  {
         IEEE80211_TDLS_SET_LINKID_RESP(frm, vap, arg);
         IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                       "%s: Peer Traffic Response Message to Initiator: %s\n",
                       __func__, ether_sprintf(arg));
     }

    len = (frm - (u_int8_t*)wbuf_header(wbuf));
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "SEND_PEER_TRAFFIC_RESP: hdr_len=%d \n", len);

    wbuf_set_pktlen(wbuf, len);

    N_NODE_SET(wbuf, ieee80211_ref_node(ni_tdls)); /* sending via Direct path */

    IEEE80211_TDLS_WHDR_SET(wbuf, vap, arg);

    /* Notification to TdlsDiscovery process on TearDown */
    /* tdls_notify_teardown(vap, arg, IEEE80211_TDLS_TEARDOWN, NULL); */

    /* temporary to be removed later */
    //dump_pkt((wbuf->data), wbuf->len);

    tdls_send_mgmt_frame(ic, wbuf);

    ieee80211_free_node(ni_tdls); /* for above find */

    return 0;
}

/* TDLS Channel Switch Request */
int
tdls_send_chan_switch_req(struct ieee80211com                *ic,
                           struct ieee80211vap                *vap,
                           uint8_t                            *dest_addr,
                           struct ieee80211_channel           *chan,
                           u_int32_t                          switch_time,
                           u_int32_t                          switch_timeout,
                           wlan_action_frame_complete_handler completion_handler,
                           void                               *context)
{
    struct ieee80211_node *ni_tdls;
    uint8_t               *frm;
    uint16_t               len=0;
    wbuf_t                 wbuf;
    wlan_tdls_sm_t         w_tdls = NULL;
    
    w_tdls = TAILQ_FIRST(&(ic->ic_tdls->tdls_node));
    if (!w_tdls) {
        return  -EINVAL;
    }

    if (!chan) {
        return -EINVAL;
    }
    
    ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, dest_addr);

    if (!ni_tdls) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"%s,Node not present:%s\n",
                          __FUNCTION__, ether_sprintf(dest_addr));
        return 0;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "%s, Send Peer Traffic Response Message to %s\n", __FUNCTION__,
                      ether_sprintf(dest_addr));

    frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_CHANNEL_SX_REQ, dest_addr, &wbuf);

    if (!frm) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, "
                             "unable to allocate wbuf\n", __FUNCTION__);
        ieee80211_free_node(ni_tdls); /* for above find */
        return -ENOMEM;
    }

    /* Set target channel and regulatory */
    IEEE80211_TDLS_SET_CHAN_SX_CHANNEL(frm, chan->ic_ieee, chan->ic_regClassId);

    /* Secondary channel offset */
    if (((chan->ic_flags & IEEE80211_CHAN_11NG_HT40PLUS) == IEEE80211_CHAN_11NG_HT40PLUS) ||
        ((chan->ic_flags & IEEE80211_CHAN_11NA_HT40PLUS) == IEEE80211_CHAN_11NA_HT40PLUS)) {
        IEEE80211_TDLS_SET_CHAN_SX_SEC_CHANNEL_OFFSET(frm, IEEE80211_SECONDARY_CHANNEL_ABOVE);
    }
    else if (((chan->ic_flags & IEEE80211_CHAN_11NA_HT40MINUS) == IEEE80211_CHAN_11NA_HT40MINUS)
             || ((chan->ic_flags & IEEE80211_CHAN_11NG_HT40MINUS) == IEEE80211_CHAN_11NA_HT40MINUS)) {
        IEEE80211_TDLS_SET_CHAN_SX_SEC_CHANNEL_OFFSET(frm, IEEE80211_SECONDARY_CHANNEL_BELOW);
    }
    
    if (!IEEE80211_TDLS_IS_INITIATOR(ni_tdls)) {
        IEEE80211_TDLS_SET_LINKID(frm, vap, dest_addr);
    }
    else  {
        IEEE80211_TDLS_SET_LINKID_RESP(frm, vap, dest_addr);
    }

    /* Add channel switching time element */
    IEEE80211_TDLS_SET_CHAN_SX_TIMING(frm, switch_time, switch_timeout);


    len = (frm - (u_int8_t*)wbuf_header(wbuf));
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "SEND_PEER_TRAFFIC_RESP: hdr_len=%d \n", len);

    wbuf_set_pktlen(wbuf, len);

    N_NODE_SET(wbuf, ieee80211_ref_node(ni_tdls)); /* sending via Direct path */

    IEEE80211_TDLS_WHDR_SET(wbuf, vap, dest_addr);

    //dump_pkt((wbuf->data), wbuf->len);

    wbuf_set_complete_handler(wbuf, completion_handler, context);
    tdls_send_mgmt_frame(ic, wbuf);

    ieee80211_free_node(ni_tdls); /* for above find */

    return 0;
}

/* TDLS Channel Switch Response */
int
tdls_send_chan_switch_resp(struct ieee80211com                *ic,
                            struct ieee80211vap                *vap,
                            uint8_t                            *dest_addr,
                            u_int16_t                          status,
                            u_int32_t                          switch_time,
                            u_int32_t                          switch_timeout,
                            wlan_action_frame_complete_handler completion_handler,
                            void                               *context)
{
    struct ieee80211_node *ni_tdls;
    uint8_t               *frm;
    uint16_t               len=0;
    wbuf_t                 wbuf;

    ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, dest_addr);

    if (!ni_tdls) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"%s,Node not present:%s\n",
                          __FUNCTION__, ether_sprintf(dest_addr));
        return 0;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "%s, Send Channel switch  Response Message to %s\n", __FUNCTION__,
                      ether_sprintf(dest_addr));

    frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_CHANNEL_SX_RESP, dest_addr, &wbuf);

    if (!frm) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, "
                             "unable to allocate wbuf\n", __FUNCTION__);
    ieee80211_free_node(ni_tdls); /* for above find */
        return -ENOMEM;
    }

    /* Add status */
    IEEE80211_TDLS_SET_STATUS(frm, vap, dest_addr, status);

    if (!IEEE80211_TDLS_IS_INITIATOR(ni_tdls)) {
        IEEE80211_TDLS_SET_LINKID(frm, vap, dest_addr);
    }
    else  {
        IEEE80211_TDLS_SET_LINKID_RESP(frm, vap, dest_addr);
    }

    /* Add channel switching time element */
    IEEE80211_TDLS_SET_CHAN_SX_TIMING(frm, switch_time, switch_timeout);

    len = (frm - (u_int8_t*)wbuf_header(wbuf));
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "SEND_PEER_TRAFFIC_RESP: hdr_len=%d \n", len);

    wbuf_set_pktlen(wbuf, len);

    N_NODE_SET(wbuf, ieee80211_ref_node(ni_tdls)); /* sending via Direct path */

    IEEE80211_TDLS_WHDR_SET(wbuf, vap, dest_addr);

    //dump_pkt((wbuf->data), wbuf->len);
    wbuf_set_complete_handler(wbuf, completion_handler, context);

    tdls_send_mgmt_frame(ic, wbuf);

    ieee80211_free_node(ni_tdls); /* for above find */

    return 0;
}

/* TDLS setup request */
static int 
tdls_send_setup_req(struct ieee80211com *ic, struct ieee80211vap *vap,
                        void* arg, uint8_t *buf, uint16_t buf_len, u_int16_t status)
{
    struct ieee80211_node *ni = vap->iv_bss, *ni_tdls;
    uint8_t              *frm;
    uint8_t                _token = IEEE80211_TDLS_TOKEN;
    uint16_t               len=0;

    wbuf_t wbuf;
    u8 *ie;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "%s, SETUP Message to %s\n", __FUNCTION__, 
                      ether_sprintf(arg));

    ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, arg);

    if (!ni_tdls) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"%s,Node not present:%s\n",
                          __FUNCTION__, ether_sprintf(arg));
        return 0;
   }

   ni_tdls->ni_tdls->state  = IEEE80211_TDLS_S_SETUP_REQ_INPROG;
   ni_tdls->ni_tdls->token  = _token; /* Dialogue Token from Initiator */
   ni_tdls->ni_tdls->stk_st = 0;
   ni_tdls->ni_tdls->is_initiator = 0; /* Node is Responder */
   ieee80211_free_node(ni_tdls); /* for above find */

	/*
	 * TDLS fixed parameters for Setup Request()
	 * [1] - Category 
	 * [1] - Action 
	 * [1] - Dialog Token 
	 * [2] - Capability
	 * Variable list of IEs in order
	 * [tlv] - Supported Rates
	 * [tlv] - Country IE
	 * [tlv] - Extended supported rates
	 * [tlv] - Supported Channels
	 * [tlv] - RSNIE (if security enabled)
	 * [tlv] - Extended Capabilities
	 * [tlv] - QoS Capability
	 * [tlv] - FTIE (if security enabled)
	 * [tlv] - Timeout Interval (if security enabled)
	 * [tlv] - Supported Regulatory Classes
	 * [tlv] - HT Capabilities
	 * [tlv] - 20/40 BSS Coexistence
	 * [tlv] - Link Identifier
	 */

   frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_SETUP_REQ, arg, &wbuf);

   if (!frm) {
       IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, "
       						"unable to allocate wbuf\n", __FUNCTION__);
       return -ENOMEM;
   }
   
   IEEE80211_TDLS_SET_TOKEN(frm, vap, arg, _token);
   /* set the default Max Service period (MaxSP ) to 00, meaning deliver all buffered frames */
   ni_tdls->ni_tdls->wme_info_uapsd |= (0x0 << WME_CAPINFO_UAPSD_MAXSP_SHIFT);
   ni_tdls->ni_tdls->wme_info_uapsd |= WME_CAPINFO_UAPSD_NONE;

   /* Fill the TDLS information up to and including Supported Channels IE */
    frm += ieee80211_tdls_setup_cap(ni, (struct ieee80211_frame *)frm,
                            vap->iv_bss->ni_bssid, ni_tdls);

    if (buf && buf_len) {
			u8 *rsnie;

			/* Add RSNIE given by supplicant */
			rsnie = ieee80211_tdls_get_iebuf(buf, IEEE80211_ELEMID_RSN, buf_len);
			if(rsnie) {
				u8 len = rsnie[1]+2;
				OS_MEMCPY(frm, rsnie, len);
				frm += len;
				IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
					"SEND_TDLS_REQ RSNIE added len=%d.\n", len);
			}
    }

    /* 20/40 BSS Coexistence && TDLS Support (bit37) */
    frm = ieee80211_add_extcap(frm, ni);
		    
    /* TODO: to add Qos Capability IE */

    if (buf && buf_len) {
			u8 *ftie, *timeoutie;

			/* Add FTIE given by supplicant */
			ftie = ieee80211_tdls_get_iebuf(buf, IEEE80211_ELEMID_FT, buf_len);
			if(ftie) {
				u8 len = ftie[1]+2;
				OS_MEMCPY(frm, ftie, len);
				frm += len;
				IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
					"SEND_TDLS_REQ FTIE added len=%d.\n", len);
			}

			/* Add Timeout Interval IE given by supplicant */
			timeoutie = ieee80211_tdls_get_iebuf(buf, 
						IEEE80211_ELEMID_TIMEOUT_INTERVAL, buf_len);
			if(timeoutie) {
				u8 len = timeoutie[1]+2;
				OS_MEMCPY(frm, timeoutie, len);
				frm += len;
				IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
					"SEND_TDLS_REQ Timeout Interval added len=%d.\n", len);
			}
    }

	/* TODO: Supported Regulatory Classes */
	
	/* Add HTCAP IE */
    frm = ieee80211_add_htcap(frm, ni_tdls, IEEE80211_FC0_SUBTYPE_ASSOC_REQ);

    /* TODO: 20/40 BSS Coexistence IE */

    /* Add Link Identifier IE */
    if (buf && buf_len &&
	(ie = ieee80211_tdls_get_iebuf(buf, IEEE80211_ELEMID_TDLSLNK,
				       buf_len)) &&
	ie[1] == IEEE80211_TDLS_LNKID_LEN) {
	    /* Allow userspace to override LinkId; this is mainly for testing
	     */
	    OS_MEMCPY(frm, ie, 2 + IEEE80211_TDLS_LNKID_LEN);
	    frm += 2 + IEEE80211_TDLS_LNKID_LEN;
	    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: Replace LinkId\n",
			      __func__);
    } else {
	    IEEE80211_TDLS_SET_LINKID(frm, vap, arg);
    }

    len = frm - (u_int8_t *) wbuf_header(wbuf);
	
    IEEE80211_NODE_STAT(ni, tx_data);

    N_NODE_SET(wbuf, ieee80211_ref_node(ni));

    wbuf_set_pktlen(wbuf, len);

    IEEE80211_TDLS_WHDR_SET(wbuf, vap, arg);

	/* temporary to be removed later */
	dump_pkt((wbuf->data), wbuf->len);

    tdls_send_mgmt_frame(ic, wbuf);
	IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
		"SEND_TDLS_REQ: successfully sent ... \n", len);

    return 0;
}

/* TDLS setup response */
static int 
tdls_send_setup_resp(struct ieee80211com *ic, struct ieee80211vap *vap,
                        void* arg, uint8_t *buf, uint16_t buf_len, u_int16_t status)
{
    struct ieee80211_node *ni = vap->iv_bss, *ni_tdls=NULL;
    wbuf_t wbuf;
    uint8_t              *frm=NULL;
    uint16_t               len=0;
    u8 *ie;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: SETUP RESPONSE Message to %s\n",
                      ether_sprintf(arg));

    ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, arg);

    if (!ni_tdls) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"%s,Node not present:%s\n",
                          __FUNCTION__, ether_sprintf(arg));
		return 0;
    }

    ni_tdls->ni_tdls->state = IEEE80211_TDLS_S_SETUP_RESP_INPROG;

    ieee80211_free_node(ni_tdls); /* for above find */

	/*
	 * TDLS fixed parameters for Setup Response()
	 * [1] - Category 
	 * [1] - Action 
	 * [1] - Status Code 
	 * [1] - Dialog Token 
	 * [2] - Capability
	 * Variable list of IEs in order
	 * [tlv] - Supported Rates
	 * [tlv] - Country IE
	 * [tlv] - Extended supported rates
	 * [tlv] - Supported Channels
	 * [tlv] - RSNIE (if security enabled)
	 * [tlv] - Extended Capabilities
	 * [tlv] - QoS Capability
	 * [tlv] - FTIE (if security enabled)
	 * [tlv] - Timeout Interval (if security enabled)
	 * [tlv] - Supported Regulatory Classes
	 * [tlv] - HT Capabilities
	 * [tlv] - 20/40 BSS Coexistence
	 * [tlv] - Link Identifier
	 */

    frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_SETUP_RESP, arg, &wbuf);

    if (!frm) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, "
     			"unable to allocate wbuf\n", __FUNCTION__);
        return -ENOMEM;
    }
   
   IEEE80211_TDLS_SET_STATUS(frm, vap, arg, status);
   IEEE80211_TDLS_SET_TOKEN(frm, vap, arg, ni_tdls->ni_tdls->token);
   if (status != 0)
       goto send_err;

   /* set the default Max Service period (MaxSP ) to 00, meaning deliver all buffered frames */
   ni_tdls->ni_tdls->wme_info_uapsd |= (0 << WME_CAPINFO_UAPSD_MAXSP_SHIFT);
   ni_tdls->ni_tdls->wme_info_uapsd |= WME_CAPINFO_UAPSD_ALL;
    
   /* Fill the TDLS information up to and including Supported Channels IE */
    frm += ieee80211_tdls_setup_cap(ni, (struct ieee80211_frame *)(frm+len),
                            vap->iv_bss->ni_bssid, ni_tdls);

    if (buf && buf_len) {
			u8 *rsnie;

			/* Add RSNIE given by supplicant */
			rsnie = ieee80211_tdls_get_iebuf(buf, IEEE80211_ELEMID_RSN, buf_len);
			if(rsnie) {
				u8 len = rsnie[1]+2;
				OS_MEMCPY(frm, rsnie, len);
				frm += len;
				IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
					"SEND_TDLS_RESP RSNIE added len=%d.\n", len);
			}
    }

    /* 20/40 BSS Coexistence && TDLS Support (bit37) */
    frm = ieee80211_add_extcap(frm, ni);

    /* TODO: to add Qos Capability IE */

    if (buf && buf_len) {
			u8 *ftie, *timeoutie;

			/* Add FTIE given by supplicant */
			ftie = ieee80211_tdls_get_iebuf(buf, IEEE80211_ELEMID_FT, buf_len);
			if(ftie) {
				u8 len = ftie[1]+2;
				OS_MEMCPY(frm, ftie, len);
				frm += len;
				IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
					"SEND_TDLS_RESP FTIE added len=%d.\n", len);
			}

			/* Add Timeout Interval IE given by supplicant */
			timeoutie = ieee80211_tdls_get_iebuf(buf, 
					IEEE80211_ELEMID_TIMEOUT_INTERVAL, buf_len);
			if(timeoutie) {
				u8 len = timeoutie[1]+2;
				OS_MEMCPY(frm, timeoutie, len);
				frm += len;
				IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
					"SEND_TDLS_RESP Timeout Interval added len=%d.\n", len);
			}
    }

	/* TODO: Supported Regulatory Classes */
	
	/* Add HTCAP IE */
    frm = ieee80211_add_htcap(frm, ni_tdls, IEEE80211_FC0_SUBTYPE_ASSOC_REQ);

    /* TODO: 20/40 BSS Coexistence IE */

    /* Add Link Identifier IE */
    if (buf && buf_len &&
	(ie = ieee80211_tdls_get_iebuf(buf, IEEE80211_ELEMID_TDLSLNK,
				       buf_len)) &&
	ie[1] == IEEE80211_TDLS_LNKID_LEN) {
	    /* Allow userspace to override LinkId; this is mainly for testing
	     */
	    OS_MEMCPY(frm, ie, 2 + IEEE80211_TDLS_LNKID_LEN);
	    frm += 2 + IEEE80211_TDLS_LNKID_LEN;
	    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: Replace LinkId\n",
			      __func__);
    } else {
	    IEEE80211_TDLS_SET_LINKID_RESP(frm, vap, arg);
    }

send_err:
    len = frm - (u_int8_t *) wbuf_header(wbuf);

    IEEE80211_NODE_STAT(ni, tx_data);

    N_NODE_SET(wbuf, ieee80211_ref_node(ni));

    wbuf_set_pktlen(wbuf, len);

    IEEE80211_TDLS_WHDR_SET(wbuf, vap, arg);
	IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
		"SEND_TDLS_RESP: successfully sent ... \n", len);

	/* temporary to be removed later */
	dump_pkt((wbuf->data), wbuf->len);

    tdls_send_mgmt_frame(ic, wbuf);

    return 0;
}
/*
 * Add a WME Parameter element to a frame.
 */
static u_int8_t *
tdls_add_wme_param(struct ieee80211_node *ni, u_int8_t *frm, struct ieee80211_wme_state *wme,
                        int uapsd_enable)
{
    static const u_int8_t oui[4] = { WME_OUI_BYTES, WME_OUI_TYPE };
    struct ieee80211_wme_param *ie = (struct ieee80211_wme_param *) frm;
    int i;

    *frm++ = IEEE80211_ELEMID_VENDOR;
    *frm++ = 0;             /* length filled in below */
    OS_MEMCPY(frm, oui, sizeof(oui));       /* WME OUI */
    frm += sizeof(oui);
    *frm++ = WME_PARAM_OUI_SUBTYPE;     /* OUI subtype */
    *frm++ = WME_VERSION;           /* protocol version */

    ie->param_qosInfo = 0;
    *frm = ni->ni_tdls->wme_info_uapsd;
    if (uapsd_enable) {
        *frm |= WME_CAPINFO_UAPSD_EN;
    }
    frm++;
    *frm++ = 0;                             /* reserved field */
    for (i = 0; i < WME_NUM_AC; i++) {
        const struct wmeParams *ac =
            &wme->wme_bssChanParams.cap_wmeParams[i];
        *frm++ = IEEE80211_SM(i, WME_PARAM_ACI)
            | IEEE80211_SM(ac->wmep_acm, WME_PARAM_ACM)
            | IEEE80211_SM(ac->wmep_aifsn, WME_PARAM_AIFSN)
            ;
        *frm++ = IEEE80211_SM(ac->wmep_logcwmax, WME_PARAM_LOGCWMAX)
            | IEEE80211_SM(ac->wmep_logcwmin, WME_PARAM_LOGCWMIN)
            ;
        IEEE80211_ADDSHORT(frm, ac->wmep_txopLimit);
    }

    ie->param_len = frm - &ie->param_oui[0];

    return frm;
}


/* Send TDLS setup confirm */
static int 
tdls_send_setup_confirm(struct ieee80211com *ic, struct ieee80211vap *vap,
                            void* arg, uint8_t *buf, uint16_t buf_len, u_int16_t status)
{
    struct ieee80211_node *ni = vap->iv_bss, *ni_tdls=NULL;
    wbuf_t wbuf;
    uint8_t              *frm=NULL;
    uint16_t               len=0;
    u8 *ie;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: SETUP CONFIRM Message to %s\n",
                      ether_sprintf(arg));

    ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, arg);

    if (!ni_tdls) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"%s,Node not present:%s\n",
                          __FUNCTION__, ether_sprintf(arg));
		return 0;
    }

    ieee80211_free_node(ni_tdls); /* for above find */

	/*
	 * TDLS fixed parameters for Setup Confirm()
	 * [1] - Category 
	 * [1] - Action 
	 * [1] - Status Code 
	 * [1] - Dialog Token 
	 * Variable list of IEs in order
	 * [tlv] - RSNIE (if security enabled)
	 * [tlv] - EDCA Parameter Set
	 * [tlv] - FTIE (if security enabled)
	 * [tlv] - HT Operation
	 * [tlv] - Link Identifier
	 */

    frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_SETUP_CONFIRM, arg, &wbuf);

    if (!frm) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, "
        					"unable to allocate wbuf\n", __FUNCTION__);
        return -ENOMEM;
    }

    IEEE80211_TDLS_SET_STATUS(frm, vap, arg, status);
    IEEE80211_TDLS_SET_TOKEN(frm, vap, arg, ni_tdls->ni_tdls->token);
    if (status != 0)
	goto send_err;

    if (buf && buf_len) {
			u8 *rsnie;

			/* Add RSNIE given by supplicant */
			rsnie = ieee80211_tdls_get_iebuf(buf, IEEE80211_ELEMID_RSN, buf_len);
			if(rsnie) {
				u8 len = rsnie[1]+2;
				OS_MEMCPY(frm, rsnie, len);
				frm += len;
				IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
					"SEND_TDLS_CONFIRM RSNIE added len=%d.\n", len);
			}
    }
			
    /* TODO: to add EDCA Parameter Set */

    if (buf && buf_len) {
			u8 *ftie, *timeoutie;
			/* Add FTIE given by supplicant */
			ftie = ieee80211_tdls_get_iebuf(buf, IEEE80211_ELEMID_FT, buf_len);
			if(ftie) {
				u8 len = ftie[1]+2;
				OS_MEMCPY(frm, ftie, len);
				frm += len;
				IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
					"SEND_TDLS_CONFIRM FTIE added len=%d.\n", len);
			}

			/* Add Timeout Interval IE given by supplicant */
			timeoutie = ieee80211_tdls_get_iebuf(buf, 
					IEEE80211_ELEMID_TIMEOUT_INTERVAL, buf_len);
			if(timeoutie) {
				u8 len = timeoutie[1]+2;
				OS_MEMCPY(frm, timeoutie, len);
				frm += len;
				IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
					"SEND_TDLS_CONFIRM Timeout Interval added len=%d.\n", len);
			}
    }
    
     
    /* set the default Max Service period (MaxSP ) to 00, meaning deliver all buffered frames */
    ni_tdls->ni_tdls->wme_info_uapsd |= (0x0 << WME_CAPINFO_UAPSD_MAXSP_SHIFT);
    ni_tdls->ni_tdls->wme_info_uapsd |= WME_CAPINFO_UAPSD_ALL;

    frm = tdls_add_wme_param(ni_tdls, frm, &ic->ic_wme, IEEE80211_IS_TDLS_PEER_UAPSD_ENABLED(ni_tdls));

#ifdef TDLS_ADD_HTCAP
    frm = ieee80211_add_htcap(frm, ni_tdls, subtype);

#if 0
    if (!IEEE80211_IS_HTVIE_ENABLED(ic)) {
        frm = ieee80211_add_htcap_pre_ana(frm, ni, IEEE80211_FC0_SUBTYPE_PROBE_RESP);
    } 
    else {
        frm = ieee80211_add_htcap_vendor_specific(frm, ni, subtype);
    }
#endif
#endif /* TDLS_ADD_HTCAP */
    
    /* Add HT Operation set IE */
    frm = ieee80211_add_htinfo(frm, ni);
#if 0
    if(!IEEE80211_IS_HTVIE_ENABLED(ic))
        frm = ieee80211_add_htinfo_pre_ana(frm, ni);
#endif

    /* Add Link Identifier IE */
    if (buf && buf_len &&
	(ie = ieee80211_tdls_get_iebuf(buf, IEEE80211_ELEMID_TDLSLNK,
				       buf_len)) &&
	ie[1] == IEEE80211_TDLS_LNKID_LEN) {
	    /* Allow userspace to override LinkId; this is mainly for testing
	     */
	    OS_MEMCPY(frm, ie, 2 + IEEE80211_TDLS_LNKID_LEN);
	    frm += 2 + IEEE80211_TDLS_LNKID_LEN;
	    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: Replace LinkId\n",
			      __func__);
    } else {
	    IEEE80211_TDLS_SET_LINKID(frm, vap, arg);
    }

send_err:
    len = frm - (u_int8_t *) wbuf_header(wbuf);

    IEEE80211_NODE_STAT(ni, tx_data);

    wbuf_set_pktlen(wbuf, len);

    N_NODE_SET(wbuf, ieee80211_ref_node(ni));

    IEEE80211_TDLS_WHDR_SET(wbuf, vap, arg);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
    	"SEND_TDLS_CONFIRM: successfully sent ... \n", len);

	/* temporary to be removed later */
	dump_pkt((wbuf->data), wbuf->len);

    tdls_send_mgmt_frame(ic, wbuf);

#if CONFIG_RCPI
  if (status == 0) {
    ieee80211_tdls_link_rcpi_req(ni->ni_vap, ni->ni_bssid, arg);
    ni_tdls->ni_tdls->link_st = 1;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, ", Setting the Path to %d(TDLS link)\n",
                      ni_tdls->ni_tdls->link_st);
        //IEEE80211_TDLSRCPI_PATH_SET(ni);
  }
#endif

    return 0;
}

/* TDLS teardown send request */
static int 
tdls_send_teardown(struct ieee80211com *ic, struct ieee80211vap *vap,
                           void* arg, uint8_t *buf, uint16_t buf_len, u_int16_t status)
{
    struct ieee80211_node *ni_tdls=NULL;
    wbuf_t 				  wbuf;
    uint8_t              *frm=NULL;
    uint8_t                _reason = IEEE80211_TDLS_TEARDOWN_UNSPECIFIED_REASON;
    uint16_t               len=0;
    u8 *ie;

#if ATH_TDLS_AUTO_CONNECT
    if (ic->ic_tdls_cleaning) {
         _reason = IEEE80211_TDLS_TEARDOWN_LEAVING_BSS_REASON;
         IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                           "%s Auto-TDLS: A TDLS Teardown frame "
                           "with Reason Code Reason Code 3 Deauthenticated " 
                           "because sending STA is leaving IBSS or ESS \n",
                            __FUNCTION__);
    }
#endif

    ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, arg);

    /* Node is already deleted or teardown in progress just return */
    if (!ni_tdls || (ni_tdls->ni_tdls && 
        (ni_tdls->ni_tdls->state == IEEE80211_TDLS_S_TEARDOWN_INPROG))) {
         if (ni_tdls) ieee80211_free_node(ni_tdls);
        return 0;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: TEARDOWN Message to %s\n",
                      ether_sprintf(arg));

    ieee80211tdls_link_monitor_stop(ni_tdls);

    frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_TEARDOWN, arg, &wbuf);

    if (!frm) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, "
        					"unable to allocate wbuf\n", __FUNCTION__);
	ieee80211_free_node(ni_tdls);
        return -ENOMEM;
    }

    /* Reason Code */
    IEEE80211_TDLS_SET_STATUS(frm, vap, arg, _reason);
    
    if (buf && buf_len) {
			u8 *ftie = NULL;
			/* Add FTIE given by supplicant */
			ftie = ieee80211_tdls_get_iebuf(buf, IEEE80211_ELEMID_FT, buf_len);
			if(ftie) {
				u8 len = ftie[1]+2;
				OS_MEMCPY(frm, ftie, len);
				frm += len;
				IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
					"TDLS: TEARDOWN FTIE added len=%d.\n", len);
			}
    }
    
    /* Add Link Identifier IE */
    if (buf && buf_len &&
	(ie = ieee80211_tdls_get_iebuf(buf, IEEE80211_ELEMID_TDLSLNK,
				       buf_len)) &&
	ie[1] == IEEE80211_TDLS_LNKID_LEN) {
	    /* Allow userspace to override LinkId; this is mainly for testing
	     */
	    OS_MEMCPY(frm, ie, 2 + IEEE80211_TDLS_LNKID_LEN);
	    frm += 2 + IEEE80211_TDLS_LNKID_LEN;
	    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: Replace LinkId\n",
			      __func__);
    } else {
	if(!IEEE80211_TDLS_IS_INITIATOR(ni_tdls)) {
		IEEE80211_TDLS_SET_LINKID(frm, vap, arg);
    	IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: TEARDOWN Message to Responder: %s\n",
                      ether_sprintf(arg));
	}
	else  {
		IEEE80211_TDLS_SET_LINKID_RESP(frm, vap, arg);
    	IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: TEARDOWN Message to Initiator: %s\n",
                      ether_sprintf(arg));
	}
    }

    len = frm - (u_int8_t *) wbuf_header(wbuf);

    wbuf_set_pktlen(wbuf, len);

    N_NODE_SET(wbuf, ieee80211_ref_node(ni_tdls)); /* sending via Direct path */

    IEEE80211_TDLS_WHDR_SET(wbuf, vap, arg);

    /* Notification to TdlsDiscovery process on TearDown */
    /* tdls_notify_teardown(vap, arg, IEEE80211_TDLS_TEARDOWN, NULL); */

	/* temporary to be removed later */
	dump_pkt((wbuf->data), wbuf->len);

    tdls_send_mgmt_frame(ic, wbuf);

    ni_tdls->ni_tdls->state = IEEE80211_TDLS_S_TEARDOWN_INPROG;

#if CONFIG_RCPI
    ni_tdls->ni_tdls->link_st = 0;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, ", Setting the Path to %d(AP link)\n",
                      ni_tdls->ni_tdls->link_st);
#endif

    ieee80211_free_node(ni_tdls);
    //ieee80211_tdls_del_node(ni_tdls);
    return 0;
}

int
tdls_send_qosnulldata(
    struct ieee80211_node              *ni,
    int                                ac,
    int                                pwr_save,
    wlan_action_frame_complete_handler completion_handler,
    void                               *context)
{
    struct ieee80211vap          *vap = ni->ni_vap;
    struct ieee80211com          *ic = ni->ni_ic;
    wbuf_t                       wbuf;
    struct ieee80211_qosframe    *qwh;
    u_int32_t                    hdrsize;

    /*
     * XXX: It's the same as a management frame in the sense that
     * both are self-generated frames.
     */
    wbuf = wbuf_alloc(ic->ic_osdev, WBUF_TX_MGMT, sizeof(struct ieee80211_qosframe));
    if (wbuf == NULL)
        return -ENOMEM;

    ieee80211_prepare_qosnulldata(ni, wbuf, ac);

    qwh = (struct ieee80211_qosframe *)wbuf_header(wbuf);

    hdrsize = sizeof(struct ieee80211_qosframe);

    if (ic->ic_flags & IEEE80211_F_DATAPAD) {
        /* add padding if required and zero out the padding */
        u_int8_t    pad = roundup(hdrsize, sizeof(u_int32_t)) - hdrsize;

        OS_MEMZERO( (u_int8_t *) ((u_int8_t *) qwh + hdrsize), pad);
        hdrsize += pad;
    }

    wbuf_set_pktlen(wbuf, hdrsize);

    vap->iv_lastdata = OS_GET_TIMESTAMP();

    wbuf_set_complete_handler(wbuf, completion_handler, context);

    /* qos null data  can not go out if the vap is in forced pause state */
    if (ieee80211_vap_is_force_paused(vap)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_POWER | IEEE80211_MSG_TDLS,
                          "%s: queue null data frame the vap is in forced pause state\n",
                          __func__);
        ieee80211_node_saveq_queue(ni,wbuf,IEEE80211_FC0_TYPE_MGT);

        return EOK;
    } else {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_POWER | IEEE80211_MSG_TDLS,
                          "%s: send qos null data frame\n", __func__);

       return ieee80211_send_mgmt(vap,ni, wbuf,true);
    }
}
#if CONFIG_RCPI

/* TDLS Tx Switch send request */
static
int tdls_send_txpath_switch_req(struct ieee80211com *ic, struct ieee80211vap *vap,
                           void* arg, u_int8_t *buf, u_int16_t buf_len)
{
    struct ieee80211_node *ni = vap->iv_bss, *ni_tdls=NULL;
    wbuf_t        wbuf;
    u_int8_t     *frm=NULL;

    ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, arg);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: Tx Switch REQ Message to %s\n",
                      ether_sprintf(arg));
    frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_TXPATH_SWITCH_REQ, arg, &wbuf);
    if (!frm) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s, "
                                                "unable to allocate wbuf\n", __FUNCTION__);
	if (ni_tdls)
		ieee80211_free_node(ni_tdls);
        return -ENOMEM;
    }
    /* FIX: need to use IEEE80211_TDLS_SET_LINKID_RESP if the peer is the
     * initiator */
   IEEE80211_TDLS_SET_LINKID(frm, vap, arg);
   /* Status code (2 bytes) */
   *(u_int16_t *)frm = 0;
   frm += 2;
        /* Path  : The Path element contains the requested transmit path 
         *         0 - AP_path
         *         1 - TDLS_path
         * Reason: 0 - Unspecified
         *         1 - Change in power save mode 
         *         2 - Change in link state
         */
    memcpy(frm, buf, buf_len);
    frm += buf_len;
    wbuf_set_pktlen(wbuf, frm - (u_int8_t *)wbuf_raw_data(wbuf));
    N_NODE_SET(wbuf, ieee80211_ref_node(ni));
    IEEE80211_TDLS_WHDR_SET(wbuf, vap, arg);
    tdls_send_mgmt_frame(ic, wbuf);
    ni_tdls->ni_tdls->state = IEEE80211_TDLS_S_TXSWITCH_REQ_INPROG;
    ieee80211_free_node(ni_tdls); /* For above find */
    return 0;
}

/* TDLS Tx Switch send response */
static
int tdls_send_txpath_switch_resp(struct ieee80211com *ic, struct ieee80211vap *vap,
                           void* arg, u_int8_t *buf, u_int16_t buf_len)
{
    struct ieee80211_node *ni = vap->iv_bss, *ni_tdls=NULL;
    wbuf_t        wbuf;
    u_int8_t      *frm=NULL;
    ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, arg);
    if (!ni_tdls) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"%s,Node not present:%s\n",
                          __FUNCTION__, ether_sprintf(arg));
		return 0;
    }
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: Tx Switch RESP Message to %s\n",
                      ether_sprintf(arg));
    frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_TXPATH_SWITCH_RESP, arg, &wbuf);
    if (!frm) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s, "
                                                "unable to allocate wbuf\n", __FUNCTION__);
	ieee80211_free_node(ni_tdls);
        return -ENOMEM;
    }
    /* FIX: need to use IEEE80211_TDLS_SET_LINKID_RESP if the peer is the
     * initiator */
   IEEE80211_TDLS_SET_LINKID(frm, vap, arg);
   if (!frm) {
       IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s, "
       						"unable to allocate wbuf\n", __FUNCTION__);
       ieee80211_free_node(ni_tdls);
       return -ENOMEM;
   }
    /* Status code (2 bytes) */
   *(u_int16_t *)frm = 0;
   frm += 2;
        /* Path  :  The Path element echoes the requested transmit transmit path.
         * Reason:  0 - Accept
         *          1 - Reject because of entering power save mode
         *          2 - Reject because of the link status
         *          3 - Reject because of unspecified reason
         */
    memcpy(frm, buf, buf_len);
    frm += buf_len;
    wbuf_set_pktlen(wbuf, frm - (u_int8_t *)wbuf_raw_data(wbuf));
    N_NODE_SET(wbuf, ieee80211_ref_node(ni));
    IEEE80211_TDLS_WHDR_SET(wbuf, vap, arg);

#if 0    
	/* temporary to be removed later */
	dump_pkt((wbuf->data), wbuf->len);
#endif
    tdls_send_mgmt_frame(ic, wbuf);
    ni_tdls->ni_tdls->state = IEEE80211_TDLS_S_TXSWITCH_RESP_INPROG;
    ieee80211_free_node(ni_tdls);
    return 0;
}

/* TDLS Rx Switch send request */
static
int tdls_send_rxpath_switch_req(struct ieee80211com *ic, struct ieee80211vap *vap,
                           void* arg, u_int8_t *buf, u_int16_t buf_len)
{
   struct ieee80211_node *ni = vap->iv_bss, *ni_tdls=NULL;
   wbuf_t        wbuf;
   u_int8_t      *frm=NULL;

   ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, arg);
    if (!ni_tdls) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"%s,Node not present:%s\n",
                          __FUNCTION__, ether_sprintf(arg));
		return 0;
    }
   IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: Rx Switch REQ Message to %s\n",
                      ether_sprintf(arg));
   frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_RXPATH_SWITCH_REQ, arg, &wbuf);
   if (!frm) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s, "
                         "unable to allocate wbuf\n", __FUNCTION__);
	ieee80211_free_node(ni_tdls);
        return -ENOMEM;
   }
    /* FIX: need to use IEEE80211_TDLS_SET_LINKID_RESP if the peer is the
     * initiator */
   IEEE80211_TDLS_SET_LINKID(frm, vap, arg);
   
   /* Status code (2 bytes) */
   *(u_int16_t *)frm = 0;
   frm += 2;
        /* Path  : The Path element contains the requested receive path 
         *         0 - AP_path
         *         1 - TDLS_path
         * Reason: 0 - Unspecified
         *         1 - Change in power save mode 
         *         2 - Change in link state
         */
    memcpy(frm, buf, buf_len);
    frm += buf_len;
    wbuf_set_pktlen(wbuf, frm - (u_int8_t *)wbuf_raw_data(wbuf));
    N_NODE_SET(wbuf, ieee80211_ref_node(ni));
    IEEE80211_TDLS_WHDR_SET(wbuf, vap, arg);
    
    tdls_send_mgmt_frame(ic, wbuf);
    ni_tdls->ni_tdls->state = IEEE80211_TDLS_S_RXSWITCH_REQ_INPROG;
    ieee80211_free_node(ni_tdls);
    
    return 0;
}

/* TDLS Rx Switch send response */
static
int tdls_send_rxpath_switch_resp(struct ieee80211com *ic, struct ieee80211vap *vap,
                           void* arg, u_int8_t *buf, u_int16_t buf_len)
{
    struct ieee80211_node *ni = vap->iv_bss, *ni_tdls=NULL;
    wbuf_t        wbuf;
    u_int8_t      *frm=NULL;

    ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, arg);
    if (!ni_tdls) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"%s,Node not present:%s\n",
                          __FUNCTION__, ether_sprintf(arg));
		return 0;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: Rx Switch RESP Message to %s\n",
                      ether_sprintf(arg));

    frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_RXPATH_SWITCH_RESP, arg, &wbuf);
    if (!frm) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s, "
                                                "unable to allocate wbuf\n", __FUNCTION__);
	ieee80211_free_node(ni_tdls);
        return -ENOMEM;
    }

    /* FIX: need to use IEEE80211_TDLS_SET_LINKID_RESP if the peer is the
     * initiator */
   IEEE80211_TDLS_SET_LINKID(frm, vap, arg);
   if (!frm) {
       IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s, "
       						"unable to allocate wbuf\n", __FUNCTION__);
       ieee80211_free_node(ni_tdls);
       return -ENOMEM;
   }

    /* Status code (2 bytes) */
    *(u_int16_t *)frm = 0;
    frm += 2;

        /* Path  :  The Path element echoes the requested transmit transmit path.
         */
    memcpy(frm, buf, buf_len);
    frm += buf_len;
    wbuf_set_pktlen(wbuf, frm - (u_int8_t *)wbuf_raw_data(wbuf));
    N_NODE_SET(wbuf, ieee80211_ref_node(ni));
    IEEE80211_TDLS_WHDR_SET(wbuf, vap, arg);
    tdls_send_mgmt_frame(ic, wbuf);
    ni_tdls->ni_tdls->state = IEEE80211_TDLS_S_RXSWITCH_RESP_INPROG;
    ieee80211_free_node(ni_tdls); /* For above find */

    return 0;
}

/* TDLS Link RCPI Request */
static
int tdls_send_link_rcpi_req(struct ieee80211com *ic, struct ieee80211vap *vap,
                           void* arg, u_int8_t *buf, u_int16_t buf_len, u_int8_t link)
{
    struct ieee80211_node *ni = vap->iv_bss, *ni_tdls=NULL;
    wbuf_t        wbuf;
    u_int8_t      *frm=NULL;

    ni_tdls = ieee80211_find_node(&ic->ic_sta, arg);
    if (!ni_tdls) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"%s,Node not present:%s\n",
                          __FUNCTION__, ether_sprintf(arg));
		return 0;
    }
    frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_LINKRCPI_REQ, arg, &wbuf);
    if (!frm) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, "
                                                "unable to allocate wbuf\n", __FUNCTION__);
	ieee80211_free_node(ni_tdls);
        return -ENOMEM;
    }

    /* FIX: need to use IEEE80211_TDLS_SET_LINKID_RESP if the peer is the
     * initiator */
    IEEE80211_TDLS_SET_LINKID(frm, vap, arg);
    /* Status code (2 bytes) */
    *(u_int16_t *)frm = 0;
    frm += 2;
    memcpy(frm, buf, buf_len);
    frm += buf_len;
    wbuf_set_pktlen(wbuf, frm - (u_int8_t *)wbuf_raw_data(wbuf));

    /* BSSID :       To get AP's RSSI value of Peer Station */
    /* STA Address: To get Direct Link RSSI value of Peer Station */
    if(link)
        N_NODE_SET(wbuf, ieee80211_ref_node(ni_tdls));
    else
        N_NODE_SET(wbuf, ieee80211_ref_node(ni));

    IEEE80211_TDLS_WHDR_SET(wbuf, vap, arg);
    tdls_send_mgmt_frame(ic, wbuf);
    ni_tdls->ni_tdls->state = IEEE80211_TDLS_S_LINKRCPI_REQ_INPROG;
    ieee80211_free_node(ni_tdls); /* For above find */

    return 0;
}

/* TDLS Link RCPI report */
static
int tdls_send_link_rcpi_report(struct ieee80211com *ic, struct ieee80211vap *vap,
                           void* arg, u_int8_t *buf, u_int16_t buf_len)
{
    struct ieee80211_node *ni = vap->iv_bss, *ni_tdls=NULL;
    wbuf_t        wbuf;
    u_int8_t      *frm=NULL;

    ni_tdls = ieee80211_find_node(&ic->ic_sta, arg);
    if (!ni_tdls) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"%s,Node not present:%s\n",
                          __FUNCTION__, ether_sprintf(arg));
		return 0;
    }
    
    frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_LINKRCPI_REPORT, arg, &wbuf);
    if (!frm) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, "
                                                "unable to allocate wbuf\n", __FUNCTION__);
	ieee80211_free_node(ni_tdls);
        return -ENOMEM;
    }
    /* FIX: need to use IEEE80211_TDLS_SET_LINKID_RESP if the peer is the
     * initiator */
    IEEE80211_TDLS_SET_LINKID(frm, vap, arg);
    /* Status code (2 bytes) */
    *(u_int16_t *)frm = 0;
    frm += 2;

    /* BSSID:         To get AP's RCPI value */
    /* RCPI_1 value   For frames from AP in dB*/
    /* STA Address:   To get Direct Link RCPI value */
    /* RCPI_2 value   For frames thro Direct Link in dB*/
    memcpy(frm, buf, buf_len);
    frm += buf_len;

    wbuf_set_pktlen(wbuf, frm - (u_int8_t *)wbuf_raw_data(wbuf));
    N_NODE_SET(wbuf, ieee80211_ref_node(ni));
    IEEE80211_TDLS_WHDR_SET(wbuf, vap, arg);
    tdls_send_mgmt_frame(ic, wbuf);
    ni_tdls->ni_tdls->state = IEEE80211_TDLS_S_LINKRCPI_REPORT_INPROG;
    ieee80211_free_node(ni_tdls); /* For above find */

    return 0;
}

/* TDLS Tx Switch request */
static 
int tdls_recv_txpath_switch_req(struct ieee80211com *ic,
                           struct ieee80211_node *ni, void *arg, u16 dummy)
{
    int ret = 0; 
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_tdls_frame *tf = (struct ieee80211_tdls_frame *)arg; 

    struct ieee80211_tdls_lnk_id * linkid = NULL;
    struct ieee80211_tdls_rcpi_switch *txreq = NULL;
    struct ieee80211_tdls_rcpi_switch txresp;
    struct ieee80211_node *ni_ap    = NULL;

	u_int8_t    path;
	u_int8_t    reason;
	u_int8_t    hithreshold;
	u_int8_t    lothreshold;
	u_int8_t    margin;
	u_int8_t    link_st;
	u_int8_t    ap_rssi  = 0;
	u_int8_t    sta_rssi = 0;
	int         diff;

    linkid = (struct ieee80211_tdls_lnk_id *)(tf+1);
    txreq = (struct ieee80211_tdls_rcpi_switch*) (((char*)(linkid+1))+2);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: TXSWITCH REQ Message from %s to switch to \n",
                      ether_sprintf(linkid->sa));

    ni->ni_tdls->state = IEEE80211_TDLS_S_TXSWITCH_REQ_INPROG;
    
    /* process_the_request_suitably_to_send_response */
	/* Path  : The Path element contains the requested transmit path 
	 *  	   0 - AP_path
	 *  	   1 - TDLS_path
	 * Reason: 0 - Unspecified
	 * 	       1 - Change in power save mode 
	 * 	       2 - Change in link state
	 */

	path   = txreq->path.path;
	reason = txreq->reason;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, " new link=%s, reason=%d "
                      " current link=%d\n", path == 0? "AP Path":"TDLS Path",
                      reason, ni->ni_tdls->link_st);

	hithreshold 	= 	ic->ic_tdls->hithreshold;
	lothreshold 	= 	ic->ic_tdls->lothreshold;
	margin 		    = 	ic->ic_tdls->margin;

	ni_ap 			= 	ni->ni_vap->iv_bss;
	ap_rssi 		= 	(ATH_NODE_NET80211(ni_ap)->an_avgrssi);

	link_st			= 	ni->ni_tdls->link_st;
	sta_rssi 		= 	(ATH_NODE_NET80211(ni)->an_avgrssi);
	diff        	= 	ap_rssi - sta_rssi;

	memset(&txresp, 0, sizeof(txresp));
	memcpy(&txresp, txreq, sizeof(txresp));

    if(reason == REASON_CHANGE_IN_LINK_STATE) 
    {
        /* check if reason is OK to switch */
		/* received REQ for switch to Direct Path */
		if(path && (link_st == AP_PATH) && (sta_rssi > hithreshold))  
        {
            IEEE80211_TDLSRCPI_PATH_SET(ni);
			txresp.reason = ACCEPT;
		    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
		                    "TDLS: Switching to Direct Path: %s, TDLS link rssi=%d, "
                            "AP link rssi=%d, hiT=%d\n", ether_sprintf(linkid->sa), 
                            sta_rssi, ap_rssi, hithreshold);
         }
			/* received REQ for switch to AP Path */
	     else if(!path && (link_st == DIRECT_PATH) && (sta_rssi < lothreshold) && (diff > margin))  
         {

			IEEE80211_TDLSRCPI_PATH_RESET(ni);
			IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
			                   "TDLS: Switching to AP Path: %s, TDLS link rssi=%d, "
                                "AP link rssi=%d, diff=%d, margin=%d\n", ether_sprintf(linkid->sa), 
                                 sta_rssi, ap_rssi, diff, margin);
			txresp.reason = ACCEPT;
		  } 
          else 
	  	  /* REJECT request */
          {
			 IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
                			    "TDLS: Rejecting the path switch request: %s TDLS link rssi=%d,"
                                " AP link rssi=%d, diff=%d, margin=%d\n", ether_sprintf(linkid->sa), 
             sta_rssi, ap_rssi, diff, margin);
			 txresp.reason = REJECT_LINK_STATUS;
          }
				
	}
    /* Send response to Peer node suitably */
 	ret = tdls_send_txpath_switch_resp(ic, vap, linkid->sa, (u_int8_t*)&txresp, sizeof(txresp));

    return ret;
}

/* TDLS Tx Switch response */
static 
int tdls_recv_txpath_switch_resp(struct ieee80211com *ic,
                           struct ieee80211_node *ni, void *arg, u16 dummy)
{
    int ret = 0; 
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_tdls_frame *tf = (struct ieee80211_tdls_frame *)arg; 

    struct ieee80211_tdls_lnk_id * linkid = NULL;
	struct ieee80211_tdls_rcpi_switch *txresp = NULL;

    linkid = (struct ieee80211_tdls_lnk_id *)(tf+1);
	txresp = (struct ieee80211_tdls_rcpi_switch*)  (((char*)(linkid+1))+2);;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: TXSWITCH RESP Message from %s, reason=%d\n",
                      ether_sprintf(linkid->sa), txresp->reason);
    ni->ni_tdls->state = IEEE80211_TDLS_S_TXSWITCH_RESP_INPROG;
    
    /* process_the_response */
	/* Path  :  The Path element echoes the requested transmit transmit path.
	 * Reason:  0 - Accept
	 *  		1 - Reject because of entering power save mode
	 *  		2 - Reject because of the link status
	 *  		3 - Reject because of unspecified reason
	 */

	if(txresp->reason == ACCEPT) 
    {
		/* make a Switch */
        if(txresp->path.path == DIRECT_PATH) 
        {
    		ni->ni_tdls->link_st = 1;
	    	IEEE80211_TDLSRCPI_PATH_SET(ni);
	        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
	                	    "TDLS: Switching to Direct Path: %s\n", ether_sprintf(linkid->sa));
		} 
        else 
        {
		    ni->ni_tdls->link_st = 0;
			IEEE80211_TDLSRCPI_PATH_RESET(ni);
		    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
		                    "TDLS: Switching to AP Path: %s\n", ether_sprintf(linkid->sa));
		}
	} 
    else 
    {
		 /* Nothing doing here */
	}
	
    return ret;
}

/* TDLS Rx Switch request */
static 
int tdls_recv_rxpath_switch_req(struct ieee80211com *ic,
                           struct ieee80211_node *ni, void *arg, u16 dummy)
{
    int ret = 0; 
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_tdls_frame *tf = (struct ieee80211_tdls_frame *)arg; 
    struct ieee80211_tdls_lnk_id * linkid = NULL;
    struct ieee80211_tdls_rcpi_switch *rxreq = NULL;
    struct ieee80211_tdls_rcpi_switch rxresp;
    u_int8_t path;
    u_int8_t reason;

    linkid = (struct ieee80211_tdls_lnk_id *)(tf+1);
    rxreq = (struct ieee80211_tdls_rcpi_switch*)  (((char*)(linkid+1))+2);;
    
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: RXSWITCH REQ Message from %s\n",
                      ether_sprintf(linkid->sa));

    ni->ni_tdls->state = IEEE80211_TDLS_S_RXSWITCH_REQ_INPROG;
    
    /* process_the_request_suitably_to_send_response */
	path   = rxreq->path.path;
	reason = rxreq->reason;

	memset(&rxresp, 0, sizeof(rxresp));
	memcpy(&rxresp, rxreq, sizeof(rxresp));
	
	if(reason == REASON_CHANGE_IN_LINK_STATE) 
    {
        if(1) /* TODO:RCPI: check if reason is OK to switch */
            rxresp.reason = ACCEPT;
		else
			rxresp.reason = REJECT_LINK_STATUS;

    }
    else if (reason == REASON_CHANGE_IN_POWERSAVE_MODE) 
    {
		if(1) /* TODO:RCPI: check if reason is OK to switch */
			rxresp.reason = ACCEPT;
		else
			rxresp.reason = REJECT_ENTERING_POWER_SAVE;
				
	} 
    else 
    {   /* REASON_UNSPECIFIED */
		if( 1 ) /* TODO: RCPI: Unspecified reason */
			rxresp.reason = ACCEPT;
		else
			rxresp.reason = REJECT_UNSPECIFIED_REASON;
	}
	/* Send response to Peer node suitably */
    ret = tdls_send_txpath_switch_resp(ic, vap, linkid->sa, 
		    					(u_int8_t*)&rxresp, sizeof(rxresp));
	/* if we are OK to make a switch */
	if(rxresp.reason == ACCEPT) 
    {
	    	if(path)
				; /* TODO:RCPI: switch to Direct Path as requested */
	    	else 
				; /* TODO:RCPI: switch to AP Path as requested */
	}

    return ret;
}

/* TDLS Rx Switch response */
static 
int tdls_recv_rxpath_switch_resp(struct ieee80211com *ic,
                           struct ieee80211_node *ni, void *arg, u16 dummy)
{
    int ret = 0; 
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_tdls_frame *tf = (struct ieee80211_tdls_frame *)arg; 
    struct ieee80211_tdls_lnk_id * linkid = NULL;
    linkid = (struct ieee80211_tdls_lnk_id *)(tf+1);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: RXSWITCH RESP Message from %s\n",
                      ether_sprintf(linkid->sa));

    ni->ni_tdls->state = IEEE80211_TDLS_S_RXSWITCH_RESP_INPROG;
    
	/* TODO:RCPI: make a Switch */
    return ret;
}

/* TDLS Link RCPI Request */
static 
int tdls_recv_link_rcpi_req(struct ieee80211com *ic,
                            struct ieee80211_node *ni, void *arg, u16 dummy)
{
    int ret = 0; 
    IEEE80211_TDLS_RCPI_HDR *tf = (IEEE80211_TDLS_RCPI_HDR *)arg; 
    struct ieee80211_tdls_lnk_id * linkid = NULL;    

	struct ieee80211_tdls_rcpi_link_req *lnk_req = NULL;
	u_int8_t bssid[ETH_ALEN];
	u_int8_t sta_addr[ETH_ALEN];

    linkid = (struct ieee80211_tdls_lnk_id *)(tf+1);
    lnk_req= (struct ieee80211_tdls_rcpi_link_req *) (((char*)(linkid+1))+2);;
    
    ni->ni_tdls->state = IEEE80211_TDLS_S_LINKRCPI_REPORT_INPROG;

    IEEE80211_ADDR_COPY(bssid,    (u_int8_t*)lnk_req->bssid);
    IEEE80211_ADDR_COPY(sta_addr, (u_int8_t*)lnk_req->sta_addr);

#if 0  
    /* No reports etc. right now. send some frame and calculate RSSI 
     * based on recieved frame
     */
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "bssid: %s, ", 
                      ether_sprintf(bssid));
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "sta_addr:%s. \n",
                      ether_sprintf(sta_addr));

	/* TODO:RCPI: Temporarily commented */
	/* Check if BSSID and STA_ADDR are proper and start measurement */
	if( IEEE80211_ADDR_EQ(bssid, ni->ni_bssid) &&
		IEEE80211_ADDR_EQ(sta_addr, vap->iv_myaddr)) {
            int ap_rssi=0, sta_rssi=0;
			/* Send RCPI measurement details */
			ap_rssi = ni->ni_vap->iv_bss->ap_avgrssi;
			sta_rssi = ni->avgrssi;
			IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
				"TDLS: Link RCPI request: RSSI_AP-%d RSSI_STA-%d\n",
					ap_rssi, sta_rssi);
			/* Need to send report now!!! */
			ieee80211_tdls_link_rcpi_report(vap, bssid, ni->ni_macaddr,
					ap_rssi, sta_rssi);
	} else {
		/* Link RCPI request frame not for us. */
	}
#endif
    return ret;
}

/* TDLS Link RCPI Report */
static 
int tdls_recv_link_rcpi_report(struct ieee80211com *ic,
                            struct ieee80211_node *ni, void *arg, u16 dummy)
{
    int ret = 0; 
    struct ieee80211vap *vap = ni->ni_vap;
    IEEE80211_TDLS_RCPI_HDR *tf = (IEEE80211_TDLS_RCPI_HDR *)arg; 
	struct ieee80211_tdls_rcpi_link_report lnk_rep;
	char * cur_ptr;
	char bssid[ETH_ALEN];
	char sta_addr[ETH_ALEN];
    struct ieee80211_tdls_lnk_id * linkid = NULL;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: Link RCPI Report from %s\n",
                      ether_sprintf(linkid->sa));

    ni->ni_tdls->state = IEEE80211_TDLS_S_LINKRCPI_REPORT_INPROG;

    linkid = (struct ieee80211_tdls_lnk_id *)(tf+1);
	cur_ptr = (char*) (((char*)(linkid+1))+2);

	memset(&lnk_rep, 0, sizeof(lnk_rep));
	memcpy(&lnk_rep, cur_ptr, sizeof(lnk_rep));

	IEEE80211_ADDR_COPY(bssid,    &lnk_rep.bssid);
	IEEE80211_ADDR_COPY(sta_addr, &lnk_rep.sta_addr);

	/* Check if BSSID and STA_ADDR are proper and accept measurement */
	if( IEEE80211_ADDR_EQ(bssid, ni->ni_bssid) &&
		IEEE80211_ADDR_EQ(sta_addr, ni->ni_macaddr))
	{
		/* update measurement for STA_ADDR */
		/* TODO: check if RCPI is OK to make switch */

		IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: Link RCPI Report: BSSID: %s, RSSI=%d \n",
                      ether_sprintf(lnk_rep.bssid), lnk_rep.rcpi1);
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: Link RCPI Report: STA_ADDR:%s, RSSI=%d \n",
                      ether_sprintf(lnk_rep.sta_addr), lnk_rep.rcpi2);

#if 0
		ni->ap_avgrssi = lnk_rep.rcpi1;
		ni->avgrssi    = lnk_rep.rcpi2;
#endif
	} 
    else 
    {
		/* Link RCPI request frame not for STA_ADDR in our list. */
	}
	
    return ret;
}

/*****************************************************************************/
/*  Here comes the list of func() calls which is used by the switching logic
 *  to send frames or make request suitably to make switch and reports.
 *	ieee80211_tdls_txpath_switch_req()
 *	ieee80211_tdls_rxpath_switch_req()
 *	ieee80211_tdls_link_rcpi_req()
 *	ieee80211_tdls_link_rcpi_report()
 *
 */

/* Send TDLS Tx Switch Request to Sta-mac address */
static int 
ieee80211_tdls_txpath_switch_req(struct ieee80211vap *vap, char *mac, 
			u_int8_t path, u_int8_t reason)
{
    struct ieee80211_tdls_rcpi_switch txreq;
	memset(&txreq, 0, sizeof(txreq));
	IEEE80211_TDLS_RCPI_SWITCH_SET(txreq, path, reason);
	return tdls_send_txpath_switch_req(vap->iv_ic, vap, mac, 
			   (u_int8_t*)&txreq, 
			   sizeof(txreq));
}

/* Send TDLS Rx Switch Request to Sta-mac address */
int 
ieee80211_tdls_rxpath_switch_req(struct ieee80211vap *vap, char *mac, 
			u_int8_t path, u_int8_t reason)
{
    struct ieee80211_tdls_rcpi_switch rxreq;
	memset(&rxreq, 0, sizeof(rxreq));
	IEEE80211_TDLS_RCPI_SWITCH_SET(rxreq, path, reason);
	return tdls_send_rxpath_switch_req(vap->iv_ic, vap, mac, 
			   (u_int8_t*)&rxreq, 
			   sizeof(rxreq));
}

/* Send TDLS Link RCPI Request for RSSI measurment on AP and Direct path */
static int 
ieee80211_tdls_link_rcpi_req(struct ieee80211vap *vap, char * bssid, 
			char * mac)
{
    struct ieee80211_tdls_rcpi_link_req lnkreq;
	memset(&lnkreq, 0, sizeof(lnkreq));
	IEEE80211_TDLS_RCPI_LINK_ADDR(lnkreq, bssid, mac);
	
	if(tdls_send_link_rcpi_req(vap->iv_ic, vap, mac,
		   (u_int8_t*)&lnkreq, sizeof(lnkreq), 0))
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"Error in sending"
				"TDLS_RCPI_Request to %s thro AP path \n", 
				ether_sprintf(mac));
			
	if(tdls_send_link_rcpi_req(vap->iv_ic, vap, mac, 
		   (u_int8_t*)&lnkreq, sizeof(lnkreq), 1))
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"Error in sending"
				"TDLS_RCPI_Request to %s thro Direct path \n", 
				ether_sprintf(mac));
			
	return 0;
}

/* Send TDLS Link RCPI Report for measured RSSI on AP and Direct path */
int 
ieee80211_tdls_link_rcpi_report(struct ieee80211vap *vap, char * bssid, 
			char * mac, u_int8_t rcpi1, 
			u_int8_t rcpi2)
{
    struct ieee80211_tdls_rcpi_link_report lnkrep;
	memset(&lnkrep, 0, sizeof(lnkrep));
	IEEE80211_TDLS_RCPI_LINK_ADDR(lnkrep, bssid, mac);
	lnkrep.rcpi1 = rcpi1;
	lnkrep.rcpi2 = rcpi2;
	
    return tdls_send_link_rcpi_report(vap->iv_ic, vap, mac, 
		 (u_int8_t*)&lnkrep, sizeof(lnkrep));
									   
}

/*
 * Handler RCPI timer
 *
 */
static OS_TIMER_FUNC(ieee80211_tdlsrcpi_timer)
{
    struct ieee80211com *ic;
    struct ieee80211_node *ni       = NULL;
    struct ieee80211_node *ni_ap    = NULL;
    struct ieee80211_node_table *nt = NULL;

	int      	diff;
	u_int8_t    hithreshold;
	u_int8_t    lothreshold;
	u_int8_t    margin;
	u_int8_t    link_st;
	u_int8_t	ap_rssi;
	u_int8_t	sta_rssi;

    OS_GET_TIMER_ARG(ic, struct ieee80211com *);
    
	/* Here comes the logic to make switching between AP/Direct path */
	
	/* 1. Select each node, Nada_Peer
	 * 2. Get the Average RSSI value for AP & STA path and get the difference.
	 * 3. if (link_st == 1-DIRECT_PATH),
	 * 4. 	if (diff < margin)&&(sta_rssi < loThreshold) switch to AP's path
	 *  	else continue to use existing Direct path.
	 * 5. else if (link_st == 0-AP_PATH),
	 * 6.	if (sta_rssi > hithreshold) switch to Direct Path
	 * 		else continue to use existing AP Path.
	 * 7. Re-Initialize the TIMER = 250ms, for next scheduling 
	 */ 

    diff        = 0;
	ap_rssi		= 0;
	sta_rssi	= 0;
	hithreshold 	= 	ic->ic_tdls->hithreshold;
	lothreshold 	= 	ic->ic_tdls->lothreshold;
	margin 		    = 	ic->ic_tdls->margin;

	nt = &ic->ic_sta;
	TAILQ_FOREACH(ni, &nt->nt_node, ni_list) 
    {	

	    if(!ni_ap) 
        {
		   ni_ap 	= ni->ni_vap->iv_bss;
		   ap_rssi 	= (ATH_NODE_NET80211(ni_ap)->an_avgrssi);
	    }

    	if(!IEEE80211_IS_TDLS_NODE(ni)|| ni == ni->ni_vap->iv_bss)	
        {
    		/* as Node is not TDLS supported, either AP/Non-TDLS Station */
	    	continue; 
	    }

    	/* Continue with RCPI work, as this Node supports TDLS */
    	link_st		= ni->ni_tdls->link_st;
	    sta_rssi 	= (ATH_NODE_NET80211(ni)->an_avgrssi);
    	diff        = ap_rssi - sta_rssi;

    #if 0
    	IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_TDLS, "TDLS:RCPI: "
	                    	"ap_rssi:%ddB, sta_rssi:%ddB, diff:%ddB \n",
                    		 ap_rssi, sta_rssi, diff);
    	IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_TDLS, "TDLS:RCPI: "
	                    	"link_st=%d, hithreshold=%d, lothreshold=%d, margin=%d\n ",
                    		 link_st, hithreshold, lothreshold, margin);
    #endif
				
    	/* Just a check to make sure existing TDLS Link is RUNNING. 
	     * If the link it TEARED_DOWN then no use of switch action.
    	 */				
    	if(ni->ni_tdls->state >= IEEE80211_TDLS_S_RUN) 
        {
    		/* To get RCPI Link Report and update RSSI value */
	    	/* TODO:RCPI: Currently RCPI_Link_Request_frame() is used 
		     * for calculation of RSSI measurement in both AP and Direct Link,
    		 * so send this frame in both AP and Direct Path.
	    	 */
		    ieee80211_tdls_link_rcpi_req(ni->ni_vap, ni->ni_bssid, ni->ni_macaddr);

    		if((link_st == AP_PATH) && (sta_rssi > hithreshold)) 
            {
    			/* Request to re-switch to Direct Link */
	    		IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_TDLS, 
		                    		"Sending DIRECT_PATH_SWITCH frame to %s , "
                                    "direct link rssi=%d, threshold=%d\n", 
                    				ether_sprintf(ni->ni_macaddr), sta_rssi, hithreshold);
						
        		ieee80211_tdls_txpath_switch_req(ni->ni_vap, ni->ni_macaddr,
				DIRECT_PATH, REASON_CHANGE_IN_LINK_STATE);
    			continue;
				
    		} 
            else if( (link_st == DIRECT_PATH) && (sta_rssi < lothreshold) && (diff > margin) )
            {
    
	    		/* Request to switch to AP Path */
		    	IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_TDLS, 
			                       "Sending AP_PATH_SWITCH frame to %s, direct link rssi=%d, "
                                   "lothrehold=%d, diff:%d, margin:%d \n", 
                        			ether_sprintf(ni->ni_macaddr), sta_rssi, lothreshold, diff, margin);
				
    			ieee80211_tdls_txpath_switch_req(ni->ni_vap, ni->ni_macaddr, 
                                                    AP_PATH, REASON_CHANGE_IN_LINK_STATE);
			    continue;
				
    		} 
            else 
            {
			    /* Nothing to do, Select the next Node */
    			continue;
		    }

		} /* if condition */
        else 
        {
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_TDLS,
                  " Link state is not RUN: %s, current state:%d \n", ether_sprintf(ni->ni_macaddr),
                      ni->ni_tdls->state);
        }
	} /* TAILQ_FOREACH */	

    /* re-queue the timer handler */
	OS_SET_TIMER(&ic->ic_tdls->tmrhdlr, ic->ic_tdls->timer);

	return;
}

/*
 * Start RCPI timer
 *
 */
static void
ieee80211_tdlsrcpi_timer_init(struct ieee80211com *ic, void *timerArg)
{

    ic->ic_tdls->timer        = RCPI_TIMER;          /* 3000 */
    ic->ic_tdls->hithreshold  = RCPI_HIGH_THRESHOLD;
    ic->ic_tdls->lothreshold  = RCPI_LOW_THRESHOLD; 
    ic->ic_tdls->margin       = RCPI_MARGIN;         
    OS_INIT_TIMER(ic->ic_osdev, 
                &ic->ic_tdls->tmrhdlr, 
                ieee80211_tdlsrcpi_timer, 
                timerArg);
	OS_SET_TIMER(&ic->ic_tdls->tmrhdlr, ic->ic_tdls->timer);
	
	return;   
}

/*
 * Stop RCPI timer
 *
 */
static void
ieee80211_tdlsrcpi_timer_stop(struct ieee80211com *ic, unsigned long timerArg)
{
    OS_FREE_TIMER(&ic->ic_tdls->tmrhdlr);
    return;
}
#endif


/* TDLS receive Peer Traffic Indication */
static int
tdls_recv_peer_traffic_indication(struct ieee80211com *ic, struct ieee80211_node *ni,
                         void *arg, u16 len)
{
    int ret = 0;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_tdls_frame *tf = (struct ieee80211_tdls_frame *)arg;
    struct ieee80211_tdls_token  * token = NULL;
    struct ieee80211_tdls_lnk_id * lnkid = NULL;
    struct ieee80211_tdls_pu_buffer_status_ie *pu_buf_status = NULL;
    ieee80211tdls_pu_sleep_sta_sm_event_t event;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                  "%s :  Received a TDLS Peer Traffic Indication\n",
                  __FUNCTION__);

    token = (struct ieee80211_tdls_token *)(tf+1);


    lnkid = (struct ieee80211_tdls_lnk_id * )ieee80211_tdls_get_ie(tf,
                IEEE80211_ELEMID_TDLSLNK, len);

    /* Validate Peer Traffic Indication */
    if(!lnkid) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: No Link ID present in Peer Traffic Indication %s\n",
                      __FUNCTION__);
        return -1;
    }

    if (!IEEE80211_ADDR_EQ(lnkid->bssid,
			   ieee80211_node_get_bssid(vap->iv_bss))) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: BSSID mismatch in Peer Traffic Indication from %s\n",
                      ether_sprintf(lnkid->sa));
        return -1;
    }
	
    ret = ieee80211tdls_pu_sleep_sta_set_dialog_token(
            &ni->ni_tdls->w_tdls->peer_uapsd.sleep_sta, token->token);
    if (ret != EOK) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "%s: Unable to store dialog token from Peer Traffic Indication from %s\n",
                      __func__, ether_sprintf(lnkid->sa));
        return -1;
    }
    pu_buf_status = (struct ieee80211_tdls_pu_buffer_status_ie *)
            ieee80211_tdls_get_ie(tf, IEEE80211_ELEMID_TDLS_PU_BUF_STATUS, len);

    if (! pu_buf_status) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: No PU Buffer Status in Peer Traffic Indication from %s\n",
                      ether_sprintf(lnkid->sa));
        return -1;
    }

    OS_MEMZERO(&event, sizeof(ieee80211tdls_pu_sleep_sta_sm_event_t));
    event.wme_info_uapsd = pu_buf_status->ac_traffic_avail;

    /* Deliver received PTI event to TDLS Peer U-APSD SLEEP STA SM */
    ieee80211tdls_deliver_pu_sleep_sta_event(ni, IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_PTI, &event);

    return ret;
}

/* TDLS receive Peer Traffic Response */
static int
tdls_recv_peer_traffic_response(struct ieee80211com *ic, struct ieee80211_node *ni,
                         void *arg, u16 len)
{
    int ret = 0;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_tdls_frame *tf = (struct ieee80211_tdls_frame *)arg;
    struct ieee80211_tdls_token  * token = NULL;
    struct ieee80211_tdls_lnk_id * lnkid = NULL;
    ieee80211tdls_pu_buf_sta_sm_event_t event;
    u_int8_t                               cached_dialog_token;
    peer_uapsd_buf_sta_t                   *buf_sta;    
    token = (struct ieee80211_tdls_token *)(tf+1);

    lnkid = (struct ieee80211_tdls_lnk_id * )ieee80211_tdls_get_ie(tf,
                IEEE80211_ELEMID_TDLSLNK, len);

    /* Validate Peer Traffic Indication */
    if(!lnkid) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: No Link ID present in Peer Traffic Response from %s\n",
                      __FUNCTION__);
        return -1;
    }

    if (!IEEE80211_ADDR_EQ(lnkid->bssid,
			   ieee80211_node_get_bssid(vap->iv_bss))) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: BSSID mismatch in Peer Traffic Indication from %s\n",
                      ether_sprintf(lnkid->sa));
        return -1;
    }
    
    buf_sta = &ni->ni_tdls->w_tdls->peer_uapsd.buf_sta;
    cached_dialog_token = ieee80211tdls_pu_buf_sta_get_dialog_token(buf_sta);
    if (token->token != cached_dialog_token) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                    "%s: Token mismatch in Peer Traffic Response from %s\n",
                     __func__, ether_sprintf(lnkid->sa));
        return -1;
    }

    OS_MEMZERO(&event, sizeof(ieee80211tdls_pu_buf_sta_sm_event_t));

    /* Deliver received PTR event to TDLS Peer U-APSD SLEEP STA SM */
    ieee80211tdls_deliver_pu_buf_sta_event(ni, IEEE80211_TDLS_PU_BUF_STA_EVENT_RCVD_PTR, &event);

    return ret;
}

/* TDLS receive Channel Switch Request */
static int
tdls_recv_chan_switch_req(struct ieee80211com *ic, struct ieee80211_node *ni,
                         void *arg, u16 len)
{
    int ret = 0;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_tdls_frame *tf = (struct ieee80211_tdls_frame *)arg;
    struct ieee80211_tdls_token  * token = NULL;
    struct ieee80211_tdls_lnk_id * lnkid = NULL;
    struct ieee80211_tdls_chan_switch_frame              *chan_switch_frame;
    struct ieee80211_tdls_chan_switch_timing_ie          *chan_switch_timing;
    struct ieee80211_tdls_chan_switch_sec_chan_offset    *sec_chan_offset;
    wlan_tdls_sm_t   w_tdls;
	
    chan_switch_frame = (struct ieee80211_tdls_chan_switch_frame *)(tf+1);


    token = (struct ieee80211_tdls_token *)(tf+1);

    lnkid = (struct ieee80211_tdls_lnk_id * )ieee80211_tdls_get_ie(tf,
                IEEE80211_ELEMID_TDLSLNK, len);

    /* Validate Channel Switch Request */
    if(!lnkid) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: No Link ID present in Channel Switch Request %s\n",
                      __FUNCTION__);
        return -1;
    }
    
    sec_chan_offset = (struct ieee80211_tdls_chan_switch_sec_chan_offset *)ieee80211_tdls_get_ie(tf,
            IEEE80211_ELEMID_SECCHANOFFSET, len);

    chan_switch_timing = (struct ieee80211_tdls_chan_switch_timing_ie *)ieee80211_tdls_get_ie(tf,
            IEEE80211_ELEMID_TDLS_CHAN_SX, len);


    if (!IEEE80211_ADDR_EQ(lnkid->bssid,
               ieee80211_node_get_bssid(vap->iv_bss))) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: BSSID mismatch in Channel Switch Request from %s\n",
                      ether_sprintf(lnkid->sa));
        return -1;
    }
	/* Check channel switch support and control accepting request */
    if (! (ieee80211_ic_off_channel_support_is_set(ic) && ieee80211_vap_off_channel_support_is_set(vap))) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "%s: Channel Switch not supported (ic=%d vap=%d); ignore Channel Switch Request from %s\n",
                      __func__,
                      ieee80211_ic_off_channel_support_is_set(ic), ieee80211_vap_off_channel_support_is_set(vap),
                      ether_sprintf(lnkid->sa));
        return -1;
    }
	if (vap->iv_tdlslist == NULL) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "%s: Node not TDLS; ignore Channel Switch Request from %s\n",
                      __func__, ether_sprintf(lnkid->sa));
        return -1;
    }

    if (vap->iv_tdls_channel_switch_control == IEEE80211_TDLS_CHANNEL_SWITCH_CONTROL_OFF) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "%s: Channel Switch Control OFF; ignore Channel Switch Request from %s\n",
                      __func__, ether_sprintf(lnkid->sa));
        return -1;
    }

    if (!chan_switch_timing) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "%s: ***CANNOT FIND CHANNEL SWITCH TIMING IE\n",
                      __func__);
    }

    w_tdls = ieee80211_tdls_find_byaddr(ic, ni->ni_macaddr);
    ret = ieee80211tdls_chan_switch_sm_req_recv(ni, w_tdls, chan_switch_frame, chan_switch_timing, sec_chan_offset);
    if (ret != 0) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "%s: Failure in function ieee80211tdls_chan_switch_sm_req_recv ret=%d\n",
                      __func__, ret);
    }


    return ret;
}

/* TDLS receive Channel Switch Response */
static int
tdls_recv_chan_switch_resp(struct ieee80211com *ic, struct ieee80211_node *ni,
                         void *arg, u16 len)
{
    int ret = 0;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_tdls_frame *tf = (struct ieee80211_tdls_frame *)arg;
    struct ieee80211_tdls_token  * token = NULL;
    struct ieee80211_tdls_lnk_id * lnkid = NULL;
    struct ieee80211_tdls_chan_switch_timing_ie    *chan_switch_timing;
    wlan_tdls_sm_t                                 w_tdls;
    struct ieee80211_tdls_status                   *status = NULL;
    status = (struct ieee80211_tdls_status *)(tf+1);

    token = (struct ieee80211_tdls_token *)(tf+1);

    lnkid = (struct ieee80211_tdls_lnk_id * )ieee80211_tdls_get_ie(tf,
                IEEE80211_ELEMID_TDLSLNK, len);
    chan_switch_timing = (struct ieee80211_tdls_chan_switch_timing_ie *)ieee80211_tdls_get_ie(tf,
            IEEE80211_ELEMID_TDLS_CHAN_SX, len);

    /* Validate Channel Switch Response */
    if(!lnkid) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: No Link ID present in Channel Switch Response %s\n",
                      __FUNCTION__);
        return -1;
    }

    if (!IEEE80211_ADDR_EQ(lnkid->bssid,
               ieee80211_node_get_bssid(vap->iv_bss))) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: BSSID mismatch in Channel Switch Response from %s\n",
                      ether_sprintf(lnkid->sa));
        return -1;
    }
    if (!chan_switch_timing) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "%s: %s: ***CANNOT FIND CHANNEL SWITCH TIMING IE\n",
                      __func__);
        return -1;
    }
    w_tdls = ieee80211_tdls_find_byaddr(ic, ni->ni_macaddr);
    ret = ieee80211tdls_chan_switch_sm_resp_recv(ni, w_tdls, status->status, chan_switch_timing);
    if (ret != 0) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "%s: Failure in function ieee80211tdls_chan_switch_sm_resp_recv ret=%d\n",
                      __func__, ret);
    }

    return ret;
}

/* TDLS receive setup req frame */
static int 
tdls_recv_setup_req(struct ieee80211com *ic, struct ieee80211_node *ni, 
                         void *arg, u16 len) 
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_tdls_frame *tf = (struct ieee80211_tdls_frame *)arg; 
    struct ieee80211_tdls_token  * token = NULL;
    struct ieee80211_tdls_lnk_id * lnkid = NULL;
    u16 capinfo = 0;

    token = (struct ieee80211_tdls_token *)(tf+1);
    capinfo = le16toh(*(u16*)(token+1));

    lnkid = (struct ieee80211_tdls_lnk_id * )ieee80211_tdls_get_ie(tf, 
    			IEEE80211_ELEMID_TDLSLNK, len);

    if(!lnkid) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: No Link ID present in SETUP REQ %s\n",
                      __FUNCTION__);
        return -1;
    }

    /*  Do not respond to Setup Request Frame if the recieved Management frame (Beacon) from the AP has
     *  "TDLS Prohibited" set.
     */
    if (ni->ni_tdls_caps & IEEE80211_TDLS_PROHIBIT)
    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: TDLS Prohibited Bit is set in current BSSS;discarding setup from: %s\n",
                      ether_sprintf(lnkid->sa));
        return -1;
    }


    if (ni->ni_tdls->state == IEEE80211_TDLS_S_SETUP_REQ_INPROG) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: SETUP REQ, outstanding Setup Req inprogress\n",
                      ether_sprintf(lnkid->sa));
        return -1;
    }

    if (!IEEE80211_ADDR_EQ(lnkid->bssid,
                ieee80211_node_get_bssid(vap->iv_bss))) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: BSSID mismatch in SETUP REQ from %s\n",
                      ether_sprintf(lnkid->sa));
        tdls_send_setup_resp(ic, vap, lnkid->sa, NULL, 0, TDLS_STATUS_BSS_MISMATCH);
        return -1;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: SETUP REQ Message from %s\n",
                      ether_sprintf(lnkid->sa));

    ni->ni_tdls->token = token->token; /* Dialogue Token from Initiator */
    ni->ni_tdls->capinfo = capinfo;
    ni->ni_capinfo = capinfo;

    if(ieee80211_tdls_processie(ni, tf, len)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: SETUP REQ, failed to process IE from %s\n",
                      ether_sprintf(lnkid->sa));
    }

    ni->ni_tdls->state = IEEE80211_TDLS_S_SETUP_RESP_INPROG;
    ni->ni_tdls->stk_st = 0;
    ni->ni_tdls->is_initiator = 1; /* Node is Initiator */

    return 0;
}

/* TDLS receive response for setup  */
static int 
tdls_recv_setup_resp(struct ieee80211com *ic, struct ieee80211_node *ni,
                         void *arg, u16 len) 
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_tdls_frame *tf = (struct ieee80211_tdls_frame *)arg; 
    struct ieee80211_tdls_status * sta   = NULL;
    struct ieee80211_tdls_token  * token = NULL;
    struct ieee80211_tdls_lnk_id * lnkid = NULL;
    u16 capinfo = 0;

    sta = (struct ieee80211_tdls_status *)(tf+1);
    if(0 != le16toh(sta->status)) {
        /* if Status != SUCCESS, drop the frame */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: SETUP RESPONSE Message from %s failed.",
                      ether_sprintf(ni->ni_macaddr));
        return -1;
    }

    token = (struct ieee80211_tdls_token *)(sta+1);
    if(token->token != ni->ni_tdls->token) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
            "TDLS: SETUP RESPONSE Message from %s failed,"
            " token mismatch sent=%d, revd=%d",
               ether_sprintf(ni->ni_macaddr),
               ni->ni_tdls->token, token->token);
        return -1;
    }

    capinfo = le16toh(*(u16*)(token+1));

    lnkid = (struct ieee80211_tdls_lnk_id * )ieee80211_tdls_get_ie(tf, 
               IEEE80211_ELEMID_TDLSLNK, len);

    if(!lnkid) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: No Link ID present in SETUP RESP %s\n",
                      __FUNCTION__);
        return -1;
    }

    if (!IEEE80211_ADDR_EQ(lnkid->bssid,
                ieee80211_node_get_bssid(vap->iv_bss))) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: BSSID mismatch in SETUP RESP from %s\n",
                      ether_sprintf(lnkid->da));
        tdls_send_setup_confirm(ic, vap, lnkid->da, NULL, 0, TDLS_STATUS_BSS_MISMATCH);
        return -1;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: SETUP RESPONSE Message from %s, ",
                      ether_sprintf(lnkid->da)); /* as Reponder sends this Setup_Response() */
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      " TA: %s\n", ether_sprintf(ni->ni_macaddr));

    if(ieee80211_tdls_processie(ni, tf, len)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: SETUP RESP, failed to process IE from %s\n",
                      ether_sprintf(lnkid->sa));
    }

    ni->ni_tdls->capinfo = capinfo;
    ni->ni_capinfo = capinfo;

    ieee80211tdls_link_monitor_start(ni);

    return 0;
}

static int 
tdls_recv_setup_confirm(struct ieee80211com *ic, 
                            struct ieee80211_node *ni, void *arg, u16 len) 
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_tdls_frame *tf = (struct ieee80211_tdls_frame *)arg; 
    struct ieee80211_tdls_status * status   = NULL;
    struct ieee80211_tdls_token  * token = NULL;
    struct ieee80211_tdls_lnk_id * lnkid = NULL;

    status   = (struct ieee80211_tdls_status *)(tf+1);
    token = (struct ieee80211_tdls_token *)(status+1);
    lnkid = (struct ieee80211_tdls_lnk_id * )ieee80211_tdls_get_ie(tf, 
                     IEEE80211_ELEMID_TDLSLNK, len);

    if(!lnkid) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: No Link ID present in SETUP CONFIRM %s\n",
                      __FUNCTION__);
        return -1;
    }

    if(le16toh(status->status) != 0)
    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: Recieved SETUP CONFIRM from %s with Status : %d discarding setup confirm\n",
                      ether_sprintf(lnkid->sa),le16toh(status->status));
        return -1;
    }


    if (!IEEE80211_ADDR_EQ(lnkid->bssid,
        ieee80211_node_get_bssid(vap->iv_bss))) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: BSSID mismatch in SETUP CONFIRM from %s\n",
                      ether_sprintf(lnkid->sa));
        return -1;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: SETUP CONFIRM Message from %s\n",
                      ether_sprintf(lnkid->sa));

    if(ieee80211_tdls_processie(ni, tf, len)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: SETUP CONFIRM, failed to process IE from %s\n",
                      ether_sprintf(lnkid->sa));
    }

#if CONFIG_RCPI
    ieee80211_tdls_link_rcpi_req(ni->ni_vap, ni->ni_bssid, lnkid->sa);
    ni->ni_tdls->link_st = 1;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, ", Setting the Path to %d(TDLS link)\n",
                      ni->ni_tdls->link_st);
        //IEEE80211_TDLSRCPI_PATH_SET(ni);
#endif

    ieee80211tdls_link_monitor_start(ni);

    return 0;
}

/* TDLS teardown request: This frame will be processed in user space. The
 * handler here is only for debug purposes. */
static int 
tdls_recv_teardown(struct ieee80211com *ic,
                           struct ieee80211_node *ni, void *arg, u16 len)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_tdls_frame *tf = (struct ieee80211_tdls_frame *)arg; 
    struct ieee80211_tdls_lnk_id *lnkid = NULL;

    lnkid = (struct ieee80211_tdls_lnk_id * )ieee80211_tdls_get_ie(tf, 
    			IEEE80211_ELEMID_TDLSLNK, len);

    if (!lnkid) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: No Link ID present in TEARDOWN %s\n",
                      __FUNCTION__);
        return -1;
    }

    if (!IEEE80211_ADDR_EQ(lnkid->bssid,
                ieee80211_node_get_bssid(vap->iv_bss))) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: BSSID mismatch in TEARDOWN from %s\n",
                      ether_sprintf(ni->ni_macaddr));
        return -1;
    }

#if ATH_TDLS_AUTO_CONNECT
    if (ic->ic_tdls_auto_enable) {
        struct ieee80211_tdls_status * status   = NULL;
        uint8_t _reason;
        status   = (struct ieee80211_tdls_status *)(tf+1);
        _reason = (uint8_t) le16toh(status->status);
        
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                          "Auto-TDLS: %s TEARDOWN Reason Code = %d \n"
                          ,__FUNCTION__,_reason);
        if ((_reason == IEEE80211_TDLS_TEARDOWN_UNREACHABLE_REASON) ||
            (_reason == IEEE80211_TDLS_TEARDOWN_UNSPECIFIED_REASON)) {
                ath_tdls_teardown_block_check(ic, ni, ni->ni_macaddr);
        }
    }
#endif

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: TEARDOWN Message from %s\n",
                      ether_sprintf(ni->ni_macaddr));

    /* Is this Teardown Request from Initiator? */
    if (IEEE80211_TDLS_IS_INITIATOR(ni)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                "TDLS: TEARDOWN Message from Initiator: %s\n", 
                ether_sprintf(lnkid->sa));
    } else {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                "TDLS: TEARDOWN Message from Responder: %s\n",
                ether_sprintf(lnkid->da));
    }

    /*
     * Let wpa_supplicant verify the frame and wait for its notification to
     * delete
     */
    return 0;
}

/* Receive Wireless ARP and add it to our wds list */
static int 
tdls_recv_arp_mgmt_frame(struct ieee80211com *ic, 
                             struct ieee80211_node *ni, void *arg, u16 len)
{
#if ATH_DEBUG
    struct ieee80211vap   *vap = ni->ni_vap;
#endif
    uint8_t *macaddr;
    struct ieee80211_node *wds_ni=NULL;
    struct ieee80211_tdls_frame *tf = (struct ieee80211_tdls_frame *)arg;

    /* Get the data followed by TDLS header */
    /* XXX: Use skb_pull() instead? */
    macaddr  = (uint8_t *)(tf + 1);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
                      "Received Nada node MAC address:%s\n", 
                      ether_sprintf(macaddr));

    if ((wds_ni=ieee80211_find_wds_node(&ic->ic_sta, macaddr))) {
        ieee80211_free_node(wds_ni);
        return 1;
    }
    if (ieee80211_add_wds_addr(&ic->ic_sta, ni, macaddr , 
                               IEEE80211_NODE_F_WDS_REMOTE )) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "Failed to add %s "
        					"to WDS table", ether_sprintf(macaddr));
        return -EINVAL;
    }

    return 0;
}

 /* setup TDLS discovery response frame*/
static u_int16_t
tdls_setup_discovery_resp(
    struct ieee80211_node *ni,
    struct ieee80211_frame *wh,
    u_int8_t *bssid
    )
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211_rsnparms *rsn = &vap->iv_rsn;
    u_int8_t *frm;
    u_int16_t capinfo = 0;
    bool  add_wpa_ie;
    uint8_t  wpa;
    
    frm = (u_int8_t *)wh;

    /*
     * discresponse frame format
     *[2]   capability information
     *[tlv] supported rates
     *[tlv] Extended supported rates
     *[28]  supported channels if channel switch is true
     *      RSNIE if sec enabled
     *      Extended Capabilities  
     *      FTIE if sec enabled
     *      Time Out intervel if security is enabled
     *      Supported Regulatory Classes if channel switch  is true
     *[tlv] HT Capabilities
            20/40 BSS Coexistense
            Link ID
     */
    if (vap->iv_opmode == IEEE80211_M_IBSS)
        capinfo |= IEEE80211_CAPINFO_IBSS;
    else if (vap->iv_opmode == IEEE80211_M_STA)
        capinfo |= IEEE80211_CAPINFO_ESS;
    else
        ASSERT(0);
    
    if (IEEE80211_VAP_IS_PRIVACY_ENABLED(vap))
        capinfo |= IEEE80211_CAPINFO_PRIVACY;
    /*
     * NB: Some 11a AP's reject the request when
     *     short premable is set.
     */
    if ((ic->ic_flags & IEEE80211_F_SHPREAMBLE) &&
        IEEE80211_IS_CHAN_2GHZ(ic->ic_curchan))
        capinfo |= IEEE80211_CAPINFO_SHORT_PREAMBLE;
    if ((ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_SLOTTIME) &&
        (ic->ic_flags & IEEE80211_F_SHSLOT))
        capinfo |= IEEE80211_CAPINFO_SHORT_SLOTTIME;
    if (ieee80211_ic_doth_is_set(ic) && ieee80211_vap_doth_is_set(vap))
        capinfo |= IEEE80211_CAPINFO_SPECTRUM_MGMT;

    *(u_int16_t *)frm = htole16(capinfo);
    frm += 2;

    frm = ieee80211_add_rates(frm, &ni->ni_rates);

    frm = ieee80211_add_xrates(frm, &ni->ni_rates);

    /* Add Supported Channel List
     */
    if (ieee80211_ic_doth_is_set(ic) && ieee80211_vap_doth_is_set(vap)) {
        frm = ieee80211_tdls_add_doth(frm, vap);
    }
    
    // RSN IE 
    add_wpa_ie=true;
    /*
     * check if os shim has setup RSN IE it self.
     */
    spin_lock(&vap->iv_lock);
    if (vap->iv_app_ie[IEEE80211_FRAME_TYPE_ASSOCREQ].length) {
        add_wpa_ie = ieee80211_check_wpaie(vap, vap->iv_app_ie[IEEE80211_FRAME_TYPE_ASSOCREQ].ie,
                                           vap->iv_app_ie[IEEE80211_FRAME_TYPE_ASSOCREQ].length);
    }
    if (vap->iv_opt_ie.length) {
        add_wpa_ie = ieee80211_check_wpaie(vap, vap->iv_opt_ie.ie,
                                           vap->iv_opt_ie.length);
    }
    spin_unlock(&vap->iv_lock);

    /* FIX: need to add proper TDLS RSN IE */
    if (add_wpa_ie) {
        if (RSN_AUTH_IS_RSNA(rsn)) {
            frm = ieee80211_setup_rsn_ie(vap, frm);
        }
    }

#if ATH_SUPPORT_WAPI
    if (RSN_AUTH_IS_WAI(rsn))
        frm = ieee80211_setup_wapi_ie(vap, frm);
#endif

    /*
     * add wpa/rsn ie if os did not setup one.
     */
    if (add_wpa_ie) {
        if (RSN_AUTH_IS_WPA(rsn))
            frm = ieee80211_setup_wpa_ie(vap, frm);

        if (RSN_AUTH_IS_CCKM(rsn)) {
            ASSERT(!RSN_AUTH_IS_RSNA(rsn) && !RSN_AUTH_IS_WPA(rsn));
        
            /*
             * CCKM AKM can be either added to WPA IE or RSN IE,
             * depending on the AP's configuration
             */
            if (RSN_AUTH_IS_RSNA(&ni->ni_rsn))
                frm = ieee80211_setup_rsn_ie(vap, frm);
            else if (RSN_AUTH_IS_WPA(&ni->ni_rsn))
                frm = ieee80211_setup_wpa_ie(vap, frm);
        }
    }

    spin_lock(&vap->iv_lock);    
    if (vap->iv_opt_ie.length){
        OS_MEMCPY(frm, vap->iv_opt_ie.ie,
                  vap->iv_opt_ie.length);
        frm += vap->iv_opt_ie.length;
    }
    if (vap->iv_app_ie[IEEE80211_FRAME_TYPE_ASSOCREQ].length){
        OS_MEMCPY(frm, vap->iv_app_ie[IEEE80211_FRAME_TYPE_ASSOCREQ].ie,
                  vap->iv_app_ie[IEEE80211_FRAME_TYPE_ASSOCREQ].length);
        frm += vap->iv_app_ie[IEEE80211_FRAME_TYPE_ASSOCREQ].length;
    }
    spin_unlock(&vap->iv_lock);
    /* 
     * Add extended capability IE if necessary. It will always
     * be present if this is a TDLS-capable node.
     */
    frm = ieee80211_add_extcap(frm, ni);  // TDLS & Power save bits added in geralds branch
    
    if ((wpa=get_wpa_mode(vap))) {
      /* TODO: Add FTIE from user space */

     /*TODO : Time Out Intervel;  ideally we should get from supplicant
       temporarly fill  (12*60*60) required for plugfest
       */
    frm = ieee80211_tdls_add_timoutie(frm);
    }
        
    /* TODO:Supported Regulatory if channel switch is true Check for condition before adding
     */
    
    /* HT Capablities */
    if ((ni->ni_flags & IEEE80211_NODE_HT) && 
        IEEE80211_IS_CHAN_11N(ic->ic_curchan) && ieee80211vap_htallowed(vap)) {
        frm = ieee80211_add_htcap(frm, ni, IEEE80211_FC0_SUBTYPE_ASSOC_REQ);

#if 0
        if (!IEEE80211_IS_HTVIE_ENABLED(ic)) {
            frm = ieee80211_add_htcap_pre_ana(frm, ni, IEEE80211_FC0_SUBTYPE_PROBE_RESP);
        }
        else {
            frm = ieee80211_add_htcap_vendor_specific(frm, ni, IEEE80211_FC0_SUBTYPE_ASSOC_REQ);
        }
#endif
    }
     /* TODO: Add 20/40 BSS Coexistence */
   
    return (frm - (u_int8_t *)wh);
}



int 
tdls_send_discovery_req(struct ieee80211com *ic, struct ieee80211vap *vap,
                        void* arg, uint8_t *buf, uint16_t buf_len, u_int16_t dialog_token)
{
    struct ieee80211_node *ni = vap->iv_bss;
    uint8_t              *frm;
    wbuf_t wbuf;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS Discovery request to %s\n",
                      ether_sprintf(arg));

    wbuf = wbuf_alloc(vap->iv_ic->ic_osdev, WBUF_TX_MGMT,
               sizeof(struct ieee80211_tdls_frame) +
               sizeof(struct ieee80211_tdls_token) +
               sizeof(struct ieee80211_tdls_lnk_id));
    if (!wbuf) {
        printk("%s unable to allocate wbuf\n", __FUNCTION__);
	return -ENOMEM;
    }
    frm = wbuf_raw_data(wbuf);

    /* Fill the TDLS header */
    IEEE80211_TDLS_HDR_SET(frm, IEEE80211_TDLS_DISCOVERY_REQ,
                                IEEE80211_TDLS_LNKID_LEN, vap, arg);
    IEEE80211_TDLS_SET_TOKEN(frm, vap, arg, dialog_token);
    IEEE80211_TDLS_SET_LINKID(frm, vap, arg);

    IEEE80211_NODE_STAT(ni, tx_data);
    N_NODE_SET(wbuf, ieee80211_ref_node(ni));
    wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));
    IEEE80211_TDLS_WHDR_SET(wbuf, vap, arg);

    tdls_send_mgmt_frame(ic, wbuf);

    return 0;
}

/* Discovery response is public action frame of type 14
 * So frame format is Management Action Frame with Public Action Type is 4
 */

int 
tdls_send_discovery_resp(struct ieee80211com *ic, struct ieee80211vap *vap,
                        void* arg, uint8_t *buf, uint16_t token, u_int16_t status)
{
    struct ieee80211_node *ni = vap->iv_bss, *ni_tdls=NULL;
    wbuf_t wbuf = NULL;
    struct ieee80211_action *action_header;	
    uint8_t              *frm=NULL;
    uint16_t               len=0;
    int error;
    
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: DISCOVERY RESPONSE Message to %s\n",
                      ether_sprintf(arg));
    
    ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, arg);

    if (!ni_tdls) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,"%s,Node not present:%s\n",
                          __FUNCTION__, ether_sprintf(arg));

        /* If Node is not present create a temp node for
         * Trasmission 
         */
         ni_tdls = ieee80211_tmp_node(vap,(u_int8_t *)arg);
         if(ni_tdls)
            ni_tdls->ni_flags |= IEEE80211_NODE_TDLS; 
         else
            return -EINVAL;
    }

    wbuf = ieee80211_getmgtframe(ni_tdls, IEEE80211_FC0_SUBTYPE_ACTION, &frm, 0);
    if (wbuf == NULL) {
        ieee80211_free_node(ni_tdls);
        return -ENOMEM;
    }
    action_header = (struct ieee80211_action *)frm;
    action_header->ia_category = IEEE80211_ACTION_CAT_PUBLIC;
    action_header->ia_action = IEEE80211_TDLS_DISCOVERY_RESP; // TDLS Discovery Response

    frm += sizeof(struct ieee80211_action);

    IEEE80211_TDLS_SET_TOKEN(frm, vap, arg, token);
    len += tdls_setup_discovery_resp(ni, (struct ieee80211_frame *)frm, vap->iv_bss->ni_bssid);
    frm += len;

    /* TDLS Responder STA Address field in LinkID of the Discovery Response frame be set 
     * to the MAC address of the STA sending Discovery Response Frame */
    IEEE80211_TDLS_SET_LINKID_RESP(frm, vap, arg);
    
    len = (frm - (u_int8_t*)wbuf_header(wbuf));
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "SEND_TDLS_DISC_RESP: hdr_len=%d \n", len);
    /* temporary to be removed later */
    dump_pkt(wbuf->data, len);

    wbuf_set_pktlen(wbuf, len);
    error = ieee80211_send_mgmt(vap,ni_tdls, wbuf,false);
     // for above find 
    ieee80211_free_node(ni_tdls);
    return 0;
}

static int 
tdls_recv_discovery_req(struct ieee80211com *ic, struct ieee80211_node *ni, 
                         void *arg, u16 len) 
{
    // Here ni is AP's Node
    int ret = 0;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_tdls_frame *tf = (struct ieee80211_tdls_frame *)arg; 
    struct ieee80211_tdls_token  * token = NULL;
    struct ieee80211_tdls_lnk_id * lnkid = NULL;

    token = (struct ieee80211_tdls_token *)(tf+1);
    lnkid = (struct ieee80211_tdls_lnk_id *)(token+1);

    if(!lnkid) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: No Link ID present in Dicovery Request %s\n",
                      __FUNCTION__);
        return -1;
    }

    /* BSSID Check */
    if (!IEEE80211_ADDR_EQ(lnkid->bssid,
        ieee80211_node_get_bssid(vap->iv_bss))) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
        "TDLS: Discarding Discovery Request BSSID(%s) "
        "mismatch\n", ether_sprintf(lnkid->bssid));
        return -1;
    }
    /* Ceck for Responder address it shouldbe ours*/
    if (!IEEE80211_ADDR_EQ(lnkid->da,vap->iv_myaddr ))
    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: Discarding Dicovery Request responder mac(%s) Mismatch\n",ether_sprintf(lnkid->da));
        return -1;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: DISC REQ Message from %s\n",
                      ether_sprintf(lnkid->sa));

    ret = tdls_send_discovery_resp(ic, vap, lnkid->sa, NULL, token->token, 0);
    return ret;
}
typedef int (*tdls_recv_fn)(struct ieee80211com *, struct ieee80211_node *, void *, u16);

/* Function pointers for message types received */
tdls_recv_fn tdls_recv[IEEE80211_TDLS_MSG_MAX] = {
    tdls_recv_setup_req,
    tdls_recv_setup_resp,
    tdls_recv_setup_confirm,
    tdls_recv_teardown,
    tdls_recv_peer_traffic_indication,
    tdls_recv_chan_switch_req,
    tdls_recv_chan_switch_resp,
    NULL, NULL,
    tdls_recv_peer_traffic_response,
    tdls_recv_discovery_req,
    NULL, NULL, NULL,
    NULL, /* recieve disvocery response */
#if CONFIG_RCPI
    tdls_recv_txpath_switch_req,
    tdls_recv_txpath_switch_resp,
    tdls_recv_rxpath_switch_req,
    tdls_recv_rxpath_switch_resp,
    tdls_recv_link_rcpi_req,
    tdls_recv_link_rcpi_report,
#endif
    tdls_recv_arp_mgmt_frame /* For WDS clients behind the other side */
};

typedef  int (*tdls_send_fn)(struct ieee80211com *, struct ieee80211vap *,
                             void *, uint8_t *, uint16_t, u_int16_t);

/* TDLS send message type functions */
tdls_send_fn tdls_send[IEEE80211_TDLS_MSG_MAX] = {
    tdls_send_setup_req,
    tdls_send_setup_resp,
    tdls_send_setup_confirm,
    tdls_send_teardown,
    tdls_send_peer_traffic_indication,
    NULL,//tdls_send_chan_switch_req,
    NULL,//tdls_send_chan_switch_resp,
    NULL, NULL,
    tdls_send_peer_traffic_response,
    tdls_send_discovery_req,
    NULL, NULL, NULL,
    tdls_send_discovery_resp,
#if CONFIG_RCPI 
    tdls_send_txpath_switch_req,
    tdls_send_txpath_switch_resp,
    tdls_send_rxpath_switch_req,
    tdls_send_rxpath_switch_resp,
    NULL, /* tdls_send_link_rcpi_req, to avoid Warning */
    tdls_send_link_rcpi_report,
#endif
    tdls_send_arp  /* For WDS clients behind the stations */
};

struct ieee80211_node *
ieee80211_create_tdls_node(struct ieee80211vap *vap, uint8_t *macaddr)
{
    struct ieee80211_node *ni;
    struct ieee80211com *ic = vap->iv_ic;

    ni = ieee80211_dup_bss(vap, macaddr);

    if (ni != NULL) {
        /* free the extra refcount got from dup_bss() */
        ieee80211_free_node(ni);

        ni->ni_esslen = vap->iv_bss->ni_esslen;
        memcpy(ni->ni_essid, vap->iv_bss->ni_essid, vap->iv_bss->ni_esslen);
        IEEE80211_ADDR_COPY(ni->ni_bssid, vap->iv_bss->ni_bssid);
        memcpy(ni->ni_tstamp.data, vap->iv_bss->ni_tstamp.data, 
               sizeof(ni->ni_tstamp));

        if (vap->iv_opmode == IEEE80211_M_IBSS)
            ni->ni_capinfo = IEEE80211_CAPINFO_IBSS;
        else
            ni->ni_capinfo = IEEE80211_CAPINFO_ESS;

        if (vap->iv_flags & IEEE80211_F_PRIVACY)
            ni->ni_capinfo |= IEEE80211_CAPINFO_PRIVACY;

        if ((ic->ic_flags & IEEE80211_F_SHPREAMBLE) &&
             IEEE80211_IS_CHAN_2GHZ(vap->iv_bsschan))
            ni->ni_capinfo |= IEEE80211_CAPINFO_SHORT_PREAMBLE;

        if (1 || ic->ic_flags & IEEE80211_F_SHSLOT)
            ni->ni_capinfo |= IEEE80211_CAPINFO_SHORT_SLOTTIME;

        if (1 || IEEE80211_IS_CHAN_11N(vap->iv_bsschan)){
            /* enable HT capability */
            ni->ni_flags |= IEEE80211_NODE_HT | IEEE80211_NODE_QOS |
                            IEEE80211_NODE_TDLS;/* XXX: | IEEE80211_NODE_HT_DS; */
            ni->ni_htcap = IEEE80211_HTCAP_C_SM_ENABLED;
        } else
        ni->ni_flags |= IEEE80211_NODE_TDLS;

        if (1 || ic->ic_htcap & IEEE80211_HTCAP_C_SHORTGI40)
            ni->ni_htcap |= IEEE80211_HTCAP_C_SHORTGI40;
        if (1 || ic->ic_htcap & IEEE80211_HTCAP_C_CHWIDTH40) {
            ni->ni_htcap |= IEEE80211_HTCAP_C_CHWIDTH40;
            ni->ni_chwidth = IEEE80211_CWM_WIDTH40;
        }

        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "HTCAP:%x, CAPINFO:%x \n",
                          ni->ni_htcap, ni->ni_capinfo);

        if (IEEE80211_IS_TDLS_PEER_UAPSD_ENABLED(ni)) {
            ieee80211node_set_flag(ni, IEEE80211_NODE_UAPSD);
            ni->ni_ext_caps |= IEEE80211_NODE_C_QOS;
        }

        ni->ni_chan = vap->iv_bss->ni_chan;
        ni->ni_intval = vap->iv_bss->ni_intval;
        ni->ni_erp = vap->iv_bss->ni_erp;

        ni->ni_tdls = (struct ieee80211_tdls_node *) OS_MALLOC(ic->ic_osdev,
                               sizeof(struct ieee80211_tdls_node), GFP_KERNEL);
        if (ni->ni_tdls == NULL) { 
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                    "Failed to allocate TDLS node, mac: %s\n", 
            ether_sprintf(ni->ni_macaddr));
            ieee80211_free_node(ni);
            return NULL;
        }
        OS_MEMZERO(ni->ni_tdls, sizeof(struct ieee80211_tdls_node));

        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, " Added TDLS node, mac: %s\n", 
                          ether_sprintf(ni->ni_macaddr));
#if CONFIG_RCPI
        /* Just start with some rssi above the low threshold+extra so that
           we dont switch to AP path */
         ATH_RSSI_LPF(ATH_NODE_NET80211(ni)->an_avgrssi, RCPI_HIGH_THRESHOLD*15);
         IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, " Setting avgrssi to %d dB\n",
                           (ATH_NODE_NET80211(ni)->an_avgrssi));
#endif

        ieee80211_node_join(ni);

        /* newma_specific: as ieee80211_node_join() fails to set tx_rate */
        /* Set up node tx rate */
        if (ic->ic_newassoc) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "Set up node tx rate: %s\n", 
                          ether_sprintf(ni->ni_macaddr));
            ic->ic_newassoc(ni, TRUE);
        }

        /* XXX Should we authorize the node here or after handshake */
        ieee80211_node_authorize(ni);

    }

    ieee80211_tdls_sm_attach(ic, vap);
    /* create entry into state machine */
    ieee80211_tdls_addentry(vap, ni);

#if ATH_TDLS_AUTO_CONNECT
    ni->ni_tdls->tstamp = OS_GET_TIMESTAMP();
#endif

    return ni;
}

/* TDLS Receive handler */
int 
_ieee80211_tdls_recv_mgmt(struct ieee80211com *ic, struct ieee80211_node *ni, 
                             wbuf_t wbuf)
{
#if ATH_DEBUG
    struct ieee80211vap         *vap = ni->ni_vap;
#endif
    struct ieee80211_tdls_frame *tf;
    struct ieee80211_node_table *nt = &ic->ic_sta;
    struct ieee80211_node *ni_tdls=NULL;
    struct ether_header  *eh; 
    u_int32_t ret=0, isnew=0; 
    char * sa = NULL;
    u16 len = 0;
    
     IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "TDLS: Entering %s for node:%s\n",
                       __FUNCTION__, ether_sprintf(ni->ni_macaddr));

    if (!(ni->ni_vap->iv_ath_cap & IEEE80211_ATHC_TDLS)) {
        printk("TDLS not enabled \n");
        return 0;
    }

     eh = (struct ether_header *)wbuf_raw_data(wbuf);
     tf = (struct ieee80211_tdls_frame *)(wbuf_raw_data(wbuf) + sizeof(struct ether_header));
     sa = eh->ether_shost;
     len = wbuf_get_pktlen(wbuf) - sizeof(struct ether_header); /* len of TDLS data */

#if ATH_TDLS_AUTO_CONNECT
     if ((tf->action == IEEE80211_TDLS_SETUP_REQ) || (tf->action == IEEE80211_TDLS_DISCOVERY_REQ)) {

         IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "Get TDLS Request from %s, action:%x \n",
                         ether_sprintf(sa), tf->action);

         if ((ic->ic_tdls->tdls_enable) && (ic->ic_tdls_auto_enable)) {
             ic->ic_recv_tdls_sord_req(ic, sa);
         }
     }
#endif

    if (IEEE80211_ADDR_EQ(sa, ni->ni_vap->iv_myaddr)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, " %s My own echo, "
                          "discarding it. \n", __FUNCTION__);
        return 0;
    }

    /* Check to make sure its a valid setup request */
     if (tf->payloadtype != IEEE80211_TDLS_RFTYPE || 
        tf->action > IEEE80211_TDLS_LEARNED_ARP) {
       IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "TDLS: Invalid ether "
                         "type: %x, payloadtype: %x, action:%x \n",
                         eh->ether_type, tf->payloadtype, 
                         tf->action);
        return -EINVAL;
    }
    
    ni_tdls = ieee80211_find_node(nt, sa);

    /* Create a node, if its a new request */
    if (!ni_tdls && tf->action == IEEE80211_TDLS_SETUP_REQ) {
        ni_tdls = ieee80211_create_tdls_node(ni->ni_vap, sa);
        isnew = 1;
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                          "Creating TDLS node:ni:%x, ni_tdls:%x \n",
                          ni_tdls, ni_tdls->ni_tdls);
    }

    if ((!ni_tdls) && (tf->action != IEEE80211_TDLS_DISCOVERY_REQ)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "Node not present.. %s, "
                          "type:%d\n",
                          ether_sprintf(sa), tf->action);
            ret = -EINVAL;
            goto ex;
    }
    
    if( (IEEE80211_TDLS_SETUP_REQ <= tf->action) &&
        (tf->action <= IEEE80211_TDLS_LEARNED_ARP) )
    {
        if (tdls_recv[tf->action])
        {
            if (tf->action != IEEE80211_TDLS_DISCOVERY_REQ)
                ret = tdls_recv[tf->action](ic, ni_tdls, (void *)tf, len);
            else
                ret = tdls_recv[tf->action](ic, ni, (void *)tf, len);
        } 
    }

ex:
    if (ni_tdls && !isnew) { 
         /* Decrement the ref count increased by ieee80211_find_node(). If
          * ieee80211_create_tdls_node() was used, the refcnf is left at 1. */
        ieee80211_free_node(ni_tdls);
    }

    return ret;
}
static int
_ieee80211_tdls_recv_data(
    struct ieee80211com   *ic,
    struct ieee80211_node *ni,
    wbuf_t                wbuf)
{
#if ATH_DEBUG
    struct ieee80211vap    *vap = ni->ni_vap;
#endif
    struct ether_header    *eh;
    wlan_tdls_sm_t         w_tdls;
    u_int32_t              ret = 0;
    u_int8_t               *sa = NULL;

    if (!(ni->ni_vap->iv_ath_cap & IEEE80211_ATHC_TDLS)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: TDLS not enabled cap=%04X",
            __func__, ni->ni_vap->iv_ath_cap);
        return 0;
    }

    eh = (struct ether_header *)wbuf_raw_data(wbuf);
    sa = eh->ether_shost;

    if (IEEE80211_ADDR_EQ(sa, ni->ni_vap->iv_myaddr)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: My own echo, discarding it.\n", __func__);
        return 0;
    }

    w_tdls = ieee80211_tdls_find_byaddr(ic, ni->ni_macaddr);
    if (w_tdls != NULL) {
        ret = ieee80211tdls_chan_switch_sm_data_recv(ni, w_tdls);
        if (ret != 0) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                          "%s: Failure in function ieee80211tdls_chan_switch_sm_data_recv ret=%d\n",
                          __func__, ret);
        }
    }

    return ret;
}


int 
ieee80211_tdls_recv_mgmt(struct ieee80211com *ic, struct ieee80211_node *ni, 
                             wbuf_t wbuf) {
    struct ieee80211vap    *vap = ni->ni_vap;

    if (vap->iv_opmode == IEEE80211_M_STA &&
        IEEE80211_TDLS_ENABLED(vap) ) {
        struct ether_header * eh;
        eh = (struct ether_header *)(wbuf->data);
            switch (ntohs(eh->ether_type)) {
            
            case IEEE80211_TDLS_ETHERTYPE:
                       _ieee80211_tdls_recv_mgmt(vap->iv_ic, ni, wbuf);
                break;
            case ETHERTYPE_IP:
                _ieee80211_tdls_recv_data(vap->iv_ic, ni, wbuf);
                break;
            default:
                 break;
            }

        }
    return 0;                             
}

/*
 *QOS NULL Data frames and NULL Data frames must be reported to the Channel Switch state machine 
 *since they are used to confirm that TDLS peers can communicate with each other after switching to offchannel
 */
int
ieee80211_tdls_recv_null_data(
    struct ieee80211com   *ic,
    struct ieee80211_node *ni,
    struct sk_buff        *skb)
{
#if ATH_DEBUG
    struct ieee80211vap    *vap = ni->ni_vap;
#endif
    wlan_tdls_sm_t         w_tdls;
    u_int32_t              ret = 0;

    if (!(ni->ni_vap->iv_ath_cap & IEEE80211_ATHC_TDLS)) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: TDLS not enabled cap=%04X\n",
            __func__, ni->ni_vap->iv_ath_cap);

        return 0;
    }

    w_tdls = ieee80211_tdls_find_byaddr(ic, ni->ni_macaddr);
    if (w_tdls != NULL) {
        ret = ieee80211tdls_chan_switch_sm_data_recv(ni, w_tdls);
        if (ret != 0) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                          "%s: Failure in function ieee80211tdls_chan_switch_sm_data_recv ret=%d\n",
                          __func__, ret);
        }
    }

    return ret;
}
int 
ieee80211_tdls_send_mgmt(struct ieee80211vap *vap, u_int32_t type, 
                                   void *arg)
{
    struct ieee80211com   *ic = vap->iv_ic;
    int ret = 0;

    if (!(vap->iv_ath_cap & IEEE80211_ATHC_TDLS)) {
        printk("TDLS Disabled \n");
        return 0;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, type:%d, macaddr:%s\n",
                      __FUNCTION__, type, ether_sprintf(arg));

    ret = tdls_send[type](ic, vap, arg, NULL, 0, 0);

    return ret;
}

/* Add remote mac address */
int 
ieee80211_tdls_ioctl(struct ieee80211vap *vap, int type, char *mac)
{
    struct ieee80211_node *ni_tdls;

    switch (type) {
    case IEEE80211_TDLS_ADD_NODE:
        ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, mac);
        if (ni_tdls) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: Already in "
                    "the setup process or node already exists " 
                    "for %s\n",
                     __FUNCTION__, ether_sprintf(mac));
                    ieee80211_free_node(ni_tdls);
            return 0;
        }

        /* Create a node, if it's a new request */
        ni_tdls = ieee80211_create_tdls_node(vap, mac);
        if (!ni_tdls)
            return -1;

        /*
         * Request wpa_supplicant to prepare additional IEs for TDLS Setup
         * Request to initiate TDLS setup.
         */
        tdls_notify_supplicant(vap, mac, IEEE80211_TDLS_SETUP_REQ, NULL, 0);
        break;
    case IEEE80211_TDLS_DEL_NODE:
        ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, mac);
        if (!ni_tdls) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, Node not "
                    "present for Teardown request %s\n",
                    __FUNCTION__, ether_sprintf(mac));
            return -1;
        }

    /*
     * Request wpa_supplicant to prepare additional IEs for TDLS Teardown
     * to tear down a direct link.
     */
	tdls_request_teardown_ftie(vap, mac, 
				   IEEE80211_TDLS_TEARDOWN,
		        		IEEE80211_TDLS_TEARDOWN_UNSPECIFIED_REASON,
				   ni_tdls->ni_tdls->token);
	ieee80211_free_node(ni_tdls);
        break;
    default:
        return -1;
    }

    return 0;
}

/* Configure and initiate off channel operations */
/*
 * Start off_channel operation.
 * the channel parameter may be NULL depending on the mode being set
 */
int
ieee80211_tdls_set_off_channel_mode(
    struct ieee80211vap      *vap,
    int                      mode,
    struct ieee80211_channel *channel)
{
    struct ieee80211com      *ic = vap->iv_ic;
    struct _wlan_tdls_sm     *w_tdls;
    struct ieee80211_node    *ni = NULL;
    int                      rc;
    
    if (! (ieee80211_ic_off_channel_support_is_set(ic) && ieee80211_vap_off_channel_support_is_set(vap))) {
        if (mode != IEEE80211_TDLS_CHANNEL_SWITCH_CONTROL_OFF) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: Off Chan Support disabled (ic=%d vap=%d) cannot set mode=%d\n",
                __func__,
                ieee80211_ic_off_channel_support_is_set(ic),
                ieee80211_vap_off_channel_support_is_set(vap),
                mode);
        }

        return -EINVAL;
    }

    w_tdls = TAILQ_FIRST(&(ic->ic_tdls->tdls_node));
    if (w_tdls != NULL) {
        ni = w_tdls->tdls_ni;
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: mode=%d channel=%d current_control=%d w_tdls=%p ni=%p iv_tdlslist=%p\n",
        __func__,
        mode,
        (channel != NULL) ? ieee80211_channel_ieee(channel) : -1,
        vap->iv_tdls_channel_switch_control,
        w_tdls,
        ni,
        vap->iv_tdlslist);

    switch (mode) {
    case IEEE80211_TDLS_CHANNEL_SWITCH_CMD_MODE_OFF:
        vap->iv_tdls_channel_switch_control = IEEE80211_TDLS_CHANNEL_SWITCH_CONTROL_OFF;

        if ((w_tdls != NULL) && (ni != NULL) && (vap->iv_tdlslist != NULL)) {
            ieee80211_tdls_set_channel_switch_control(&(w_tdls->chan_switch),
                (IEEE80211_TDLS_CHAN_SWITCH_CONTROL_REJECT_REQUEST    |
                 IEEE80211_TDLS_CHAN_SWITCH_CONTROL_INTERRUPT_REQUEST));

            ieee80211tdls_chan_switch_sm_stop(ni, w_tdls);
        }
        break;

    case IEEE80211_TDLS_CHANNEL_SWITCH_CMD_MODE_INITIATE:
        vap->iv_tdls_channel_switch_control = IEEE80211_TDLS_CHANNEL_SWITCH_CONTROL_ACCEPT;

        if ((w_tdls != NULL) && (ni != NULL) && (vap->iv_tdlslist != NULL)) {
            ieee80211_tdls_set_channel_switch_control(&(w_tdls->chan_switch),
                IEEE80211_TDLS_CHAN_SWITCH_CONTROL_NONE);
            rc = ieee80211tdls_chan_switch_sm_start(ni, w_tdls, channel);
            if (rc != 0) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: Failed to initiate channel switch rc=%d\n",
                    __func__, rc);
            }
        }
        else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                          "%s: Node not TDLS; ignore function\n",
                          __func__);
        }
        break;

    case IEEE80211_TDLS_CHANNEL_SWITCH_CMD_MODE_PASSIVE:
        vap->iv_tdls_channel_switch_control = IEEE80211_TDLS_CHANNEL_SWITCH_CONTROL_ACCEPT;

        if ((w_tdls != NULL) && (ni != NULL) && (vap->iv_tdlslist != NULL)) {
            ieee80211_tdls_set_channel_switch_control(&(w_tdls->chan_switch),
                IEEE80211_TDLS_CHAN_SWITCH_CONTROL_NONE);

            // Terminate any ongoing offchannel session
            ieee80211tdls_chan_switch_sm_stop(ni, w_tdls);
        }
        break;

    case IEEE80211_TDLS_CHANNEL_SWITCH_CMD_MODE_REJECT_REQ:
        vap->iv_tdls_channel_switch_control = IEEE80211_TDLS_CHANNEL_SWITCH_CONTROL_REJECT;

        if ((w_tdls != NULL) && (ni != NULL) && (vap->iv_tdlslist != NULL)) {
            ieee80211_tdls_set_channel_switch_control(&(w_tdls->chan_switch),
                IEEE80211_TDLS_CHAN_SWITCH_CONTROL_REJECT_REQUEST);

            // Terminate any ongoing offchannel session
            ieee80211tdls_chan_switch_sm_stop(ni, w_tdls);
        }
        break;

    case IEEE80211_TDLS_CHANNEL_SWITCH_CMD_MODE_UNSOLICITED:
        vap->iv_tdls_channel_switch_control = IEEE80211_TDLS_CHANNEL_SWITCH_CONTROL_ACCEPT;

        if ((w_tdls != NULL) && (ni != NULL) && (vap->iv_tdlslist != NULL)) {
            ieee80211_tdls_set_channel_switch_control(&(w_tdls->chan_switch),
                IEEE80211_TDLS_CHAN_SWITCH_CONTROL_INTERRUPT_REQUEST);

            rc = ieee80211tdls_chan_switch_sm_start(ni, w_tdls, channel);
            if (rc != 0) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: Failed to initiate channel switch rc=%d\n",
                    __func__, rc);
            }
        }
        else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: Failed to initiate channel switch ni=%p\n",
                __func__, ni);
        }
        break;

    default:
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: Invalid mode=%d\n",
            __func__, mode);
        break;
    }

    return 0;
}

/* Set the off channel switch time */
int
ieee80211_tdls_set_channel_switch_time(
    struct ieee80211vap *vap,
    int                 switchTime)
{
    return 0;
}

/* Configure the max timeout */
int
ieee80211_tdls_set_channel_switch_timeout(
    struct ieee80211vap *vap,
    int                 timeout)
{
    return 0;
}

/* TDLS teardown send request thro AP path */
static int 
tdls_send_teardown_ap(struct ieee80211com *ic, struct ieee80211vap *vap,
                           void* arg, uint8_t *buf, uint16_t buf_len, u_int16_t status)
{
    struct ieee80211_node *ni_tdls=NULL;
    wbuf_t	wbuf;
    uint8_t	*frm=NULL;
    uint8_t	_reason = IEEE80211_TDLS_TEARDOWN_UNREACHABLE_REASON;
    uint16_t	len=0;
    u8 *ie;

#if ATH_TDLS_AUTO_CONNECT
    if (ic->ic_tdls_cleaning) {
         _reason = IEEE80211_TDLS_TEARDOWN_LEAVING_BSS_REASON;
         IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                           "%s Auto-TDLS: A TDLS Teardown frame "
                           "with Reason Code Reason Code 3 Deauthenticated " 
                           "because sending STA is leaving IBSS or ESS \n",
                            __FUNCTION__);
    }
#endif

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: TEARDOWN Message thro AP path to %s\n",
                      ether_sprintf(arg));
    ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, arg);
    if (!ni_tdls) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, "
        		"Node not present \n", __FUNCTION__);
        return -1;
    }

    ieee80211tdls_link_monitor_stop(ni_tdls);

    frm = tdls_mgmt_frame(vap, IEEE80211_TDLS_TEARDOWN, arg, &wbuf);

    if (!frm) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, "
        					"unable to allocate wbuf\n", __FUNCTION__);
	ieee80211_free_node(ni_tdls);
        return -ENOMEM;
    }

    /* Reason Code */
    IEEE80211_TDLS_SET_STATUS(frm, vap, arg, _reason);
    
    if (buf && buf_len) {
        u8 *ftie = NULL;
        /* Add FTIE given by supplicant */
        ftie = ieee80211_tdls_get_iebuf(buf, IEEE80211_ELEMID_FT, buf_len);
        if(ftie) {
            u8 len = ftie[1]+2;
            OS_MEMCPY(frm, ftie, len);
            frm += len;
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, 
                "TDLS: TEARDOWN FTIE added len=%d.\n", len);
        }
    }
    
    /* Add Link Identifier IE */
    if (buf && buf_len &&
	(ie = ieee80211_tdls_get_iebuf(buf, IEEE80211_ELEMID_TDLSLNK,
				       buf_len)) &&
	ie[1] == IEEE80211_TDLS_LNKID_LEN) {
	    /* Allow userspace to override LinkId; this is mainly for testing
	     */
	    OS_MEMCPY(frm, ie, 2 + IEEE80211_TDLS_LNKID_LEN);
	    frm += 2 + IEEE80211_TDLS_LNKID_LEN;
	    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: Replace LinkId\n",
			      __func__);
    } else {
	if(!IEEE80211_TDLS_IS_INITIATOR(ni_tdls)) {
		IEEE80211_TDLS_SET_LINKID(frm, vap, arg);
    	IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: TEARDOWN Message thro AP path to Responder: %s\n",
                      ether_sprintf(arg));
	}
	else  {
		IEEE80211_TDLS_SET_LINKID_RESP(frm, vap, arg);
    	IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
                      "TDLS: TEARDOWN Message thro AP path to Initiator: %s\n",
                      ether_sprintf(arg));
	}
    }

    len = frm - (u_int8_t *) wbuf_header(wbuf);

    wbuf_set_pktlen(wbuf, len);

    N_NODE_SET(wbuf, ieee80211_ref_node(vap->iv_bss)); /* sending via AP path */

    IEEE80211_TDLS_WHDR_SET(wbuf, vap, arg);

    /* Notification to TdlsDiscovery process on TearDown */
    /* tdls_notify_teardown(vap, arg, IEEE80211_TDLS_TEARDOWN, NULL); */

    /* temporary to be removed later */
    dump_pkt((wbuf->data), wbuf->len);

    tdls_send_mgmt_frame(ic, wbuf);

    ieee80211_free_node(ni_tdls);

    return 0;
}

static int ieee80211_wpa_tdls_delete_node(struct ieee80211vap *vap, char *mac)
{
	/* Delete Node as instructed by Supplicant to do so */
	struct ieee80211_node *ni;

	ni = ieee80211_find_node(&vap->iv_ic->ic_sta, mac);
	if (ni) {
		ieee80211_free_node(ni);
		ieee80211_tdls_del_node(ni);
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "WPA_TDLS_CMD: "
				  "PEER-MAC:%s deleted successfully after MIC "
				  "check from supplicant\n",
				  ether_sprintf(mac));
		return 0;
	} else {
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "WPA_TDLS_CMD: "
				  "PEER-MAC:%s Node not present to delete\n",
				  ether_sprintf(mac));
		return -1;
	}
}

static int ieee80211_wpa_tdls_error(struct ieee80211vap *vap, u8 *mac,
				    u8 *buf, size_t len)
{
	u16 type, err_code;
	int err;

	if (len < 4)
		return -EINVAL;

	type = *(u16 *) buf;
	buf += 2;
	err_code = *(u16 *) buf;
	buf += 2;

	switch (type) {
	case IEEE80211_TDLS_SETUP_RESP:
		err = tdls_send_setup_resp(vap->iv_ic, vap, mac, NULL, 0,
					   err_code);
		break;
	case IEEE80211_TDLS_SETUP_CONFIRM:
		err = tdls_send_setup_confirm(vap->iv_ic, vap, mac, NULL, 0,
					      err_code);
		break;
	default:
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
				  "Unsupported IEEE80211_TDLS_ERROR type %u",
				  type);
		err = -EOPNOTSUPP;
		break;
	}

	return err;
}

static int ieee80211_wpa_tdls_enable_link(struct ieee80211vap *vap, char *mac)
{
	struct ieee80211_node *ni;

	ni = ieee80211_find_node(&vap->iv_ic->ic_sta, mac);
	if (!ni)
		return -1;
	if (!ni->ni_tdls) {
		ieee80211_free_node(ni);
		return -1;
	}

	ni->ni_tdls->state = IEEE80211_TDLS_S_RUN;
	IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "WPA_TDLS_CMD: "
			  "PEER-MAC:%s link enabled after check from "
			  "supplicant\n", ether_sprintf(mac));
	ieee80211_free_node(ni);

	return 0;
}

/** ieee80211_wpa_tdls_ftie - to process TDLS command from Supplicant
 *  buf - contains TDLS message Peer-MAC, TDLS_TYPE and
 *  SMK-FTIE if needed
 * 	len - length of the buffer
 */
int 
ieee80211_wpa_tdls_ftie(struct ieee80211vap *vap, u8 *buf, int len)
{
	struct ieee80211_node *ni;
	u16 type, smk_len;
	u8 *mac, *smk;
	int err;
	
    if (len < ETH_ALEN + 4)
		return -EINVAL;

	smk = buf;
	mac = smk;
	smk += ETH_ALEN;
	type = *(u16 *) smk;
	smk += 2;
	smk_len = *(u16 *) smk;
	
	if (type == IEEE80211_TDLS_SETUP_REQ || type == IEEE80211_TDLS_SETUP_RESP || 
    			type == IEEE80211_TDLS_SETUP_CONFIRM || type == IEEE80211_TDLS_TEARDOWN ) {
    
	    smk += 2; //two byte dialog token added for tdls mgmt message 
	    smk_len = *(u16 *) smk;
	}
    
    smk += 2;

    if (len < ETH_ALEN + 4 + smk_len)
        return -EINVAL;
	
    if (smk_len > (IEEE80211_TDLS_FRAME_MAXLEN -
		       sizeof(struct ieee80211_tdls_frame))) {
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "Message length "
				  "too high, len=%d\n", smk_len);
		return -EINVAL;
    }

	IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "WPA_TDLS_CMD: PEER-MAC:%s "
			"for len:%d, type:%d, vap:%p\n", 
			ether_sprintf(mac), smk_len, type, vap);

    switch (type) {
	case IEEE80211_TDLS_SETUP_REQ:
		err = tdls_send_setup_req(vap->iv_ic, vap, mac, smk, smk_len,
					  0);
		break;
	case IEEE80211_TDLS_SETUP_RESP:
		err = tdls_send_setup_resp(vap->iv_ic, vap, mac, smk, smk_len,
					   0);
		break;
	case IEEE80211_TDLS_SETUP_CONFIRM:
		err = tdls_send_setup_confirm(vap->iv_ic, vap, mac, smk,
					      smk_len, 0);
		break;
	case IEEE80211_TDLS_TEARDOWN:
		/* TODO: Reason Code from supplicant */
		err = tdls_send_teardown(vap->iv_ic, vap, mac, smk, smk_len,
					 0);

		/* As a workaround, send the teardown also through the AP path.
		 * TODO: This needs to be handled conditionally based on
		 * whether the transmission via direct path gets acknowledged.
		 */
		tdls_send_teardown_ap(vap->iv_ic, vap, mac, smk, smk_len, 0);

		/* Delete Node as instructed by Supplicant to do so */
		ni = ieee80211_find_node(&vap->iv_ic->ic_sta, mac);
		if (ni) {
			ieee80211_free_node(ni);
			ieee80211_tdls_del_node(ni);
		}
		break;
	case IEEE80211_TDLS_TEARDOWN_DELETE_NODE:
		err = ieee80211_wpa_tdls_delete_node(vap, mac);
		break;
	case IEEE80211_TDLS_ERROR:
		err = ieee80211_wpa_tdls_error(vap, mac, smk, smk_len);
		break;
	case IEEE80211_TDLS_ENABLE_LINK:
		err = ieee80211_wpa_tdls_enable_link(vap, mac);
		break;
	default:
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
				  "Unsupported TDLS FTIE command type %u",
				  type);
		err = -EOPNOTSUPP;
		break;
	}

	if (err) {
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS,
				  "%s: type:%d error:%d\n",
				  __func__, type, err);
	}

	return err;
}

static int ieee80211_convert_mac_to_hex(struct ieee80211vap *vap, char *maddr)
{
    int i, j = 2;

    for(i=2; i<strlen(maddr); i+=3)
    {
        maddr[j++] = maddr[i+1];
        maddr[j++] = maddr[i+2];
    }
        
    for(i=0; i<12; i++)
    {
        /* check 0~9, A~F */
        maddr[i] = ((maddr[i]-48) < 10) ? (maddr[i] - 48) : (maddr[i] - 55);

        /* check a~f */ 
        if ( maddr[i] >= 42 )
            maddr[i] -= 32;

        if ( maddr[i] > 0xf )
            return -EFAULT;
    }
    
    for(i=0; i<6; i++)
    {
        maddr[i] = (maddr[(i<<1)] << 4) + maddr[(i<<1)+1];
    }

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s : %s \n",
                      __FUNCTION__, ether_sprintf(maddr));

    return 0;
}

int
ieee80211_ioctl_set_tdls_rmac(struct net_device *dev,
                              void *info, void *w,
                              char *extra)
{
    wlan_if_t ifvap = ((osif_dev *)ath_netdev_priv(dev))->os_if;
    struct ieee80211vap *vap = ifvap;
    struct iw_point *wri;
    char mac[18];

    if (!IEEE80211_TDLS_ENABLED(vap))
        return -EFAULT;

    wri = (struct iw_point *) w;

    if (wri->length > sizeof(mac))
    {
        printk("len > mac_len \n");
        return -EFAULT;
    }

    memcpy(&mac, extra, 17/*sizeof(mac)*/);
    mac[17]='\0';

    if (ieee80211_convert_mac_to_hex(vap, mac)) {
        printk(KERN_ERR "Invalid mac address \n");
        return -EINVAL;
    }

    IEEE80211_TDLS_IOCTL(vap, IEEE80211_TDLS_ADD_NODE, mac);

    return 0;
}

int
ieee80211_ioctl_clr_tdls_rmac(struct net_device *dev,
                              void *info, void *w,
                              char *extra)
{
    wlan_if_t ifvap = ((osif_dev *)ath_netdev_priv(dev))->os_if;
    struct ieee80211vap *vap = ifvap;
    struct iw_point *wri;
    char mac[18];

    if (!IEEE80211_TDLS_ENABLED(vap))
        return -EFAULT;

    wri = (struct iw_point *) w;

    if (wri->length > sizeof(mac))
    {
        printk("len > mac_len \n");
        return -EFAULT;
    }

    memcpy(&mac, extra, 17/*sizeof(mac)*/);
    mac[17]='\0';

    printk(" clearing the mac address : %s \n", mac);

    if (ieee80211_convert_mac_to_hex(vap, mac)) {
        printk(KERN_ERR "Invalid mac address\n");
        return -EINVAL;
    }

    IEEE80211_TDLS_IOCTL(vap, IEEE80211_TDLS_DEL_NODE, mac);

    return 0;
}

int
ieee80211_ioctl_tdls_qosnull(struct net_device *dev,
                              void *info, void *w,
                              char *extra, int ac)
{
    wlan_if_t ifvap = ((osif_dev *)ath_netdev_priv(dev))->os_if;
    struct ieee80211vap *vap = ifvap;
    struct iw_point *wri;
    char mac[18];
    struct ieee80211_node *ni_tdls;

    if (!IEEE80211_TDLS_ENABLED(vap))
        return -EFAULT;

    wri = (struct iw_point *) w;

    if (wri->length > sizeof(mac))
    {
        printk("len > mac_len \n");
        return -EFAULT;
    }

    memcpy(&mac, extra, 17/*sizeof(mac)*/);
    mac[17]='\0';

    printk(" Send QOSNULL frame to TDLS node : %s [AC=%d]\n", mac, ac);

    if (ieee80211_convert_mac_to_hex(vap, mac)) {
        printk(KERN_ERR "Invalid mac address\n");
        return -EINVAL;
    }

    ni_tdls = ieee80211_find_node(&vap->iv_ic->ic_sta, mac);
    if (ni_tdls) {
        /* Force a QoS Null for testing. */
        ieee80211_send_qosnulldata(ni_tdls, ac, 0);

        /* To decrement the ref count increased by ieee80211_find_node */
        ieee80211_free_node(ni_tdls);
    } else {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s, Node Not "
            " present to send QOSNULL frame .... %s\n",
            __FUNCTION__, ether_sprintf(mac));
        return -EFAULT;
    }

    return 0;
}

/* The function is used  to set the MAC address  of a device set via iwpriv */
void ieee80211_tdls_set_mac_addr(u_int8_t *mac_address, u_int32_t word1, u_int32_t word2)
{
    mac_address[0] =  (u_int8_t) ((word1 & 0xff000000) >> 24);
    mac_address[1] =  (u_int8_t) ((word1 & 0x00ff0000) >> 16);
    mac_address[2] =  (u_int8_t) ((word1 & 0x0000ff00) >> 8);
    mac_address[3] =  (u_int8_t) ((word1 & 0x000000ff));
    mac_address[4] =  (u_int8_t) ((word2 & 0x0000ff00) >> 8);
    mac_address[5] =  (u_int8_t) ((word2 & 0x000000ff));
}

int ieee80211_ioctl_set_tdls_peer_uapsd_enable(struct net_device *dev, enum ieee80211_tdls_peer_uapsd_enable flag)
{
    wlan_if_t ifvap = ((osif_dev *)ath_netdev_priv(dev))->os_if;
    struct ieee80211vap *vap = ifvap;
    
    if (flag == TDLS_PEER_UAPSD_DISABLE) {
        vap->iv_ic->ic_tdls->peer_uapsd_enable = 0;
    }
    else {
        vap->iv_ic->ic_tdls->peer_uapsd_enable = 1;
    }
    return 0;
}

enum ieee80211_tdls_peer_uapsd_enable
ieee80211_ioctl_get_tdls_peer_uapsd_enable(struct net_device *dev) 
{
    wlan_if_t ifvap = ((osif_dev *)ath_netdev_priv(dev))->os_if;
    struct ieee80211vap *vap = ifvap;
    
    if (vap->iv_ic->ic_tdls->peer_uapsd_enable == 1) {
        return TDLS_PEER_UAPSD_ENABLE;
    }
    else {
        return TDLS_PEER_UAPSD_DISABLE;
    }
}

void
ieee80211_tdls_pwrsave_check(struct ieee80211_node *ni, struct ieee80211_frame *wh)
{
    ieee80211tdls_pu_buf_sta_sm_event_t event;
    u_int32_t enable;

    if (! ni || ! wh) {
        return;
    }

    if (IEEE80211_IS_TDLS_NODE(ni)) {
        enable = (wh->i_fc[1] & IEEE80211_FC1_PWR_MGT);

        if  ((ni->ni_flags & IEEE80211_NODE_PWR_MGT) ^ enable) {
            if (enable) {
                ieee80211node_set_flag(ni, IEEE80211_NODE_PWR_MGT);
                ieee80211tdls_deliver_pu_buf_sta_event(ni,
                        IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_ON, &event);
            }
            else {
                ieee80211node_clear_flag(ni, IEEE80211_NODE_PWR_MGT);
                ieee80211tdls_deliver_pu_buf_sta_event(ni,
                        IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_OFF, &event);
            }
        }            
    }
    
    return;
}

static void ieee80211tdls_deliver_pu_sleep_sta_event(struct ieee80211_node *ni,
        ieee80211tdls_pu_sleep_sta_event event_type, ieee80211tdls_pu_sleep_sta_sm_event_t *event)
{
    if (IEEE80211_IS_TDLS_PEER_UAPSD_ENABLED(ni) && event) {

        event->ni_bss_node = NULL;
        event->ni_tdls = ni;
        event->vap = ni->ni_vap;

        ieee80211tdls_pu_sleep_sta_sm_dispatch(&ni->ni_tdls->w_tdls->peer_uapsd.sleep_sta,
                event_type, event);
    }
    else {
        IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_TDLS,
            "TDLS : (%s) Unable to deliver event to Peer U-APSD SM\n", __FUNCTION__);
    }

    return;

}

static void
ieee80211tdls_deliver_pu_buf_sta_event(struct ieee80211_node *ni,
        ieee80211tdls_pu_buf_sta_event event_type, ieee80211tdls_pu_buf_sta_sm_event_t *event)
{
    if (IEEE80211_IS_TDLS_PEER_UAPSD_ENABLED(ni) && event) {

        event->ni_bss_node = NULL;
        event->ni_tdls = ni;
        event->vap = ni->ni_vap;

        (void) ieee80211tdls_pu_buf_sta_sm_dispatch(&ni->ni_tdls->w_tdls->peer_uapsd.buf_sta,
                event_type, event);
    }
    else {
        IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_TDLS,
            "TDLS : (%s) Unable to deliver event to Peer U-APSD SM\n", __FUNCTION__);
    }

    return;
}

void
ieee80211tdls_peer_uapsd_pending_xmit(struct ieee80211_node *ni, wbuf_t wbuf)
{
    if (IEEE80211_IS_TDLS_NODE(ni) &&
            WME_UAPSD_AC_ISDELIVERYENABLED(wbuf_get_priority(wbuf), ni)) {
        if (ni->ni_tdls->w_tdls->peer_uapsd.buf_sta.enable_pending_tx_notification) {
            ieee80211tdls_pu_buf_sta_sm_event_t event;

            OS_MEMZERO(&event, sizeof(ieee80211tdls_pu_buf_sta_sm_event_t));

            /* Deliver pending xmit event to TDLS Peer U-APSD Buf STA SM */
            ieee80211tdls_deliver_pu_buf_sta_event(ni,
                    IEEE80211_TDLS_PU_BUF_STA_EVENT_PENDING_TX, &event);
        }
    }

    return;
}

void
ieee80211_tdls_peer_uapsd_force_sleep(struct ieee80211vap *vap, bool enable)
{
    wlan_tdls_sm_t smt;
    struct ieee80211_tdls *td;
    rwlock_state_t lock_state;
    ieee80211tdls_pu_sleep_sta_sm_event_t event;
    ieee80211tdls_pu_buf_sta_sm_event_t      buf_event;
    /*
     * Walk TDLS linked-list
     */
    td = vap->iv_ic->ic_tdls;
    if (td) {
        if (! TAILQ_EMPTY(&td->tdls_node)) {
            OS_MEMZERO(&event, sizeof(ieee80211tdls_pu_sleep_sta_sm_event_t));

    OS_RWLOCK_WRITE_LOCK(&td->tdls_lock, &lock_state);
    TAILQ_FOREACH(smt, &td->tdls_node, tdlslist) {
        if (smt) {
            /* Check if Peer U-APSD is enabled */
            if (ieee80211node_has_flag(smt->tdls_ni, IEEE80211_NODE_UAPSD)) {
                        event.ni_tdls = buf_event.ni_tdls = smt->tdls_ni;
                        event.vap = buf_event.vap =smt->tdls_vap;
                        td->pwr_mgt_state = enable;

                /* If Peer U-APSD enabled, then notify state machine of force sleep event */
                if (enable) {
                            
                            ieee80211tdls_pu_sleep_sta_sm_dispatch(&smt->peer_uapsd.sleep_sta,
                                    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_FORCE_SLEEP,
                                    &event);
				            ieee80211tdls_pu_buf_sta_sm_dispatch(&smt->peer_uapsd.buf_sta,
                                    IEEE80211_TDLS_PU_BUF_STA_EVENT_FORCE_SLEEP,
                                    &buf_event);
                }
                else {
                            ieee80211tdls_pu_sleep_sta_sm_dispatch(&smt->peer_uapsd.sleep_sta,
                                    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_FORCE_AWAKE,
                                    &event);
							ieee80211tdls_pu_buf_sta_sm_dispatch(&smt->peer_uapsd.buf_sta,
                                    IEEE80211_TDLS_PU_BUF_STA_EVENT_FORCE_AWAKE,
                                    &buf_event);
                }
            }
        }
    }
    OS_RWLOCK_WRITE_UNLOCK(&td->tdls_lock, &lock_state);
        }
    }

    return;
}

#if ATH_TDLS_AUTO_CONNECT
EXPORT_SYMBOL(ieee80211_tdls_ioctl);
EXPORT_SYMBOL(tdls_send_discovery_req);
#endif

#if 0 & UMAC_SUPPORT_TDLS
void 
wlan_tdls_teardown_complete_handler(wlan_if_t vaphandle, wbuf_t wbuf, 
		ieee80211_xmit_status *ts) 
{
	struct ieee80211vap *vap = vaphandle;
	struct ieee80211com *ic = vap->iv_ic;
	struct ieee80211_node *ni = wbuf_get_node(wbuf);
	struct ieee80211_tdls_frame *tf = NULL; 
	u_int8_t *mac = ni->ni_macaddr;
	u_int8_t * ftie = NULL; 
	struct ether_header * eh;
	wbuf_t wbuf1;
	wbuf_t wbuf2;
	u_int16_t hdrspace;
	u_int16_t len = 0;

	if (!vap->iv_opmode == IEEE80211_M_STA ||
		!IEEE80211_TDLS_ENABLED(vap) ) {
		return;  
	}

	hdrspace = ieee80211_hdrspace(ic, wbuf_header(wbuf));
	if (wbuf_get_pktlen(wbuf) < hdrspace) {
		IEEE80211_DPRINTF(vap, 
			IEEE80211_MSG_TDLS, "data too small len %d ", 
			wbuf_get_pktlen(wbuf));
		return;
	}

	if (!(wbuf1 = wbuf_copy(wbuf)))
		return;

	if (!(wbuf2 = ieee80211_decap(vap, wbuf1, hdrspace)))
		goto out;

	eh = (struct ether_header *)(wbuf2->data);
        if (ntohs(eh->ether_type) != IEEE80211_TDLS_ETHERTYPE)
       		goto out;

	/* 
	 * hereon its TDLS packet, just validate if its teardown 
	 * and do suitably if Direct link has failed 
	 */

	tf = (struct ieee80211_tdls_frame *)(wbuf_raw_data(wbuf2) + 
		sizeof(struct ether_header));
	/* len of TDLS data */
	len = wbuf_get_pktlen(wbuf2) - sizeof(struct ether_header); 

	/* Check to make sure its a valid setup request */
     	if (tf->action != IEEE80211_TDLS_TEARDOWN) {
       		IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "TDLS: Not Teardown"
                         "payloadtype: %x, action:%x \n",
                         tf->payloadtype, 
       	                 tf->action);
        	goto out;
    	}

	if (ts->ts_flags == IEEE80211_TX_ERROR) {
		/* Tx failed thro direct path, so try via AP path.
		 * step 1: extract FTIE needed from wbuf (if security enabled);
		 * step 2: call handler to resend Teardown via AP path
		 */
		ftie = ieee80211_tdls_get_ie(tf, IEEE80211_ELEMID_FT, len);
		/* FTIE is not needed for OPEN mode */
		if (NULL == ftie) {
			tdls_send_teardown_ap(vap->iv_ic, vap, mac, NULL, 0, 0);
		} else {
			tdls_send_teardown_ap(vap->iv_ic, vap, mac, ftie, ftie[1], 0);
		}
	} else {
		/* successfull Tx thro Direct Link, so free resources */
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "TDLS: Teardown "
			"successfully sent via Direct Link to %s \n", 
			ether_sprintf(ni->ni_macaddr));
	}

	ieee80211_tdls_del_node(ni);
out:
	wbuf_free(wbuf1);
	return;
}
#endif /* UMAC_SUPPORT_TDLS */

#endif /* UMAC_SUPPORT_TDLS */

