/*
 *  Copyright (c) 2010 Atheros Communications Inc.
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

#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

#include <ieee80211_var.h>
#include <ieee80211.h>

#if UMAC_SUPPORT_TDLS

#include <ieee80211_sm.h>
#include "ieee80211_tdls_priv.h"
#include "ieee80211_tdls_power.h"

#if UMAC_SUPPORT_TDLS_PEER_UAPSD

#define MAX_QUEUED_EVENTS  16
#define IEEE80211TDLS_MAX_SLEEP_ATTEMPTS              3   /* how many times to try sending null  frames before giving up*/

static const char* tdls_pu_buf_sta_event_name[] = {
/* IEEE80211_TDLS_PU_BUF_STA_EVENT_NONE */				    "TDLS_PU_BUF_NONE",
/* IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_ON */			    "TDLS_PU_BUF_PWRMGT_ON",
/* IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_OFF */			"TDLS_PU_BUF_PWRMGT_OFF",
/* IEEE80211_TDLS_PU_BUF_STA_EVENT_FORCE_SLEEP */		    "TDLS_PU_BUF_FORCE_SLEEP",
/* IEEE80211_TDLS_PU_BUF_STA_EVENT_FORCE_AWAKE */		    "TDLS_PU_BUF_FORCE_AWAKE",
/* IEEE80211_TDLS_PU_BUF_STA_EVENT_NO_ACTIVITY */		    "TDLS_PU_BUF_NO_ACTIVITY",
/* IEEE80211_TDLS_PU_BUF_STA_EVENT_NO_PENDING_TX */		    "TDLS_PU_BUF_NO_PENDING_TX",
/* IEEE80211_TDLS_PU_BUF_STA_EVENT_PENDING_TX */            "TDLS_PU_BUF_PENDING_TX",
/* IEEE80211_TDLS_PU_BUF_STA_EVENT_SENT_PTI */			    "TDLS_PU_BUF_SENT_PTI",
/* IEEE80211_TDLS_PU_BUF_STA_EVENT_PTI_TIMEOUT */		    "TDLS_PU_BUF_PTI_TIMEOUT",
/* IEEE80211_TDLS_PU_BUF_STA_EVENT_RCVD_PTR */			    "TDLS_PU_BUF_RCVD_PTR",
/* IEEE80211_TDLS_PU_BUF_STA_EVENT_INDICATION_WINDOW_TMO */ "TDLS_PU_BUF_INDICATION_WINDOW_TMO",
/* IEEE80211_TDLS_PU_BUF_STA_EVENT_SEND_QOSNULL_SUCCESS */  "TDLS_PU_BUF_QOSNULL_SUCCESS",
/* IEEE80211_TDLS_PU_BUF_STA_EVENT_SEND_QOSNULL_FAIL */     "TDLS_PU_BUF_QOSNULL_FAIL",
};

void
ieee80211tdls_peer_uapsd_sm_create(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls)
{
    peer_uapsd_t *pu;
    
    if (ni == NULL || w_tdls == NULL) {
        return;
    }
    
    pu = &w_tdls->peer_uapsd;
    
    OS_MEMZERO(pu, sizeof(peer_uapsd_t));
    
    pu->buf_sta.w_tdls = w_tdls;
    pu->sleep_sta.w_tdls = w_tdls;
    
    /* Initialize the PU Buffer STA */
    tdls_pu_init_buf_sta(ni, &pu->buf_sta);
    
    /* Initialize the PU Sleep STA */
    tdls_pu_init_sleep_sta(ni, &pu->sleep_sta);
    
    return;
}

void
ieee80211tdls_peer_uapsd_sm_delete(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls)
{
    peer_uapsd_t *pu;
    
    if (w_tdls == NULL) {
        return;
    }

    pu = &w_tdls->peer_uapsd;
    
    /* Cleanup the PU Buffer STA */
    tdls_pu_cleanup_buf_sta(ni, &pu->buf_sta);
    
    /* Cleanup the PU Sleep STA */
    tdls_pu_cleanup_sleep_sta(ni, &pu->sleep_sta);
            
    return;
}

int 
ieee80211tdls_send_qosnull(wlan_tdls_sm_t w_tdls, struct ieee80211_node *ni, int uapsd)
{
        
    int ac;
    int rc = EOK;

    ac = WME_AC_BE;
    /* send the trigger on highest trigger enabled ac */
    if (uapsd & WME_CAPINFO_UAPSD_VO) {
         ac = WME_AC_VO;
    } else if (uapsd & WME_CAPINFO_UAPSD_VI) {
         ac = WME_AC_VI;
      } else if (uapsd & WME_CAPINFO_UAPSD_BE) {
             ac = WME_AC_BE;
        } else if (uapsd & WME_CAPINFO_UAPSD_BK) {
            ac = WME_AC_BK;
          }
    if (ni) {
       IEEE80211_DPRINTF(w_tdls->tdls_vap,IEEE80211_MSG_POWER, "%s: UAPSD send qos null \n", __func__);
             rc = ieee80211_send_qosnulldata(ni,ac,true);
    }
    printk(" \n TDLS: %s sending QOS NULL \n",__func__);        
    return(rc);
}

int 
ieee80211tdls_retry_qosnull(peer_uapsd_buf_sta_t *buf_sta)
{
    int rc = EOK;
    struct ieee80211_node *ni;

    ni = buf_sta->w_tdls->tdls_ni;
    ++buf_sta->qosnull_attempt;         /* current attempt for sending out null frame */
    if (buf_sta->qosnull_attempt < IEEE80211TDLS_MAX_SLEEP_ATTEMPTS) {
        rc = ieee80211tdls_send_qosnull(buf_sta->w_tdls, ni, ni->ni_uapsd);
        if (rc != EOK) {
            IEEE80211_DPRINTF(buf_sta->w_tdls->tdls_vap, IEEE80211_MSG_POWER, "%s: ieee80211_send_nulldata failed PM=1 rc=%d (%0x08X)\n",
                              __func__,
                              rc, rc);
        }
    }
    else {
        rc = EIO;
    }

    return(rc);
}

OS_TIMER_FUNC(pu_buf_async_qosnull_sm_timeout)
{
    peer_uapsd_buf_sta_t *buf_sta;
    ieee80211tdls_pu_buf_sta_sm_event_t event;

    OS_GET_TIMER_ARG(buf_sta, peer_uapsd_buf_sta_t *);

    event.vap = buf_sta->w_tdls->tdls_vap;
    event.ni_tdls = buf_sta->w_tdls->tdls_ni;
    event.ni_bss_node = NULL;

    /* Dispatch PU Sleep STA SM */
    ieee80211tdls_pu_buf_sta_sm_dispatch(buf_sta,
            IEEE80211_TDLS_PU_BUF_STA_EVENT_SEND_QOSNULL_FAIL, &event);
}

/*
 * txrx event handler to find out when the client enters in to sleep
 * wrt AP.
 */
static void tdls_pu_buf_sta_txrx_event_handler (ieee80211_vap_t vap,
        ieee80211_vap_txrx_event *event, void *arg)
{
    peer_uapsd_buf_sta_t *buf_sta = (peer_uapsd_buf_sta_t *) arg;
    ieee80211tdls_pu_buf_sta_sm_event_t pu_buf_sta_event;
    struct ieee80211com      *ic;
    if (vap == NULL || event == NULL || arg == NULL) {
        return;
    }

    ic = buf_sta->w_tdls->tdls_ni->ni_ic;

    if (event->ni == NULL || event->ni != buf_sta->w_tdls->tdls_ni) {
        /* Filter this event; It doesn't belong to this handler */
        return;
    }

    pu_buf_sta_event.ni_bss_node = NULL;
    pu_buf_sta_event.ni_tdls = buf_sta->w_tdls->tdls_ni;
    pu_buf_sta_event.vap = buf_sta->w_tdls->tdls_vap;
    pu_buf_sta_event.wme_info_uapsd = 0;

    switch(event->type) {
    case IEEE80211_VAP_OUTPUT_EVENT_COMPLETE_PS_NULL:
        if (event->u.status == 0) {
            ieee80211tdls_pu_buf_sta_sm_dispatch(buf_sta,
                    IEEE80211_TDLS_PU_BUF_STA_EVENT_SEND_QOSNULL_SUCCESS, &pu_buf_sta_event);
        } else {
            /*
             * this could have been failed synchronously from the SM with the ieee80211_send_nulldata() function call
             * do not dispatch the event synchronously instead send it asynchronously.
             */
             OS_SET_TIMER(&buf_sta->async_qosnull_event_timer,0);
        }
        break;
    default:
        break;
    }

    return;
}

/* Event Deferring and Processing */
static bool
ieee80211tdls_pu_buf_sta_defer_event(peer_uapsd_buf_sta_t *buf_sta,
        ieee80211tdls_pu_buf_sta_sm_event_t *event,
        u_int16_t event_type)
{
    if (buf_sta) {
        /* VAP related event */
        if (!buf_sta->deferred_event_type) {
            if (event->vap) {
                event->ni_bss_node=ieee80211_ref_bss_node(event->vap);
            }

            OS_MEMCPY(&buf_sta->deferred_event, event, sizeof(ieee80211tdls_pu_buf_sta_sm_event_t));
            buf_sta->deferred_event_type = event_type;
            IEEE80211_DPRINTF_IC(buf_sta->w_tdls->tdls_vap->iv_ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                                 "%s: defer event %d vap %d \n ",__func__,event_type,event->vap?event->vap->iv_unit:-1 );
            return true;
        }
    }

    return false;
}

static void
ieee80211tdls_pu_buf_sta_clear_event(peer_uapsd_buf_sta_t *buf_sta)
{
    if (buf_sta) {
        /* VAP related event */
       if (buf_sta->deferred_event.ni_bss_node) {
           ieee80211_free_node(buf_sta->deferred_event.ni_bss_node);
       }
       IEEE80211_DPRINTF_IC(buf_sta->w_tdls->tdls_ni->ni_ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                                 "%s: clear deferred event %d \n ",__func__,buf_sta->deferred_event_type);
        OS_MEMZERO(&buf_sta->deferred_event, sizeof(ieee80211tdls_pu_buf_sta_sm_event_t));
        buf_sta->deferred_event_type = IEEE80211_TDLS_PU_BUF_STA_EVENT_NONE;
    }
}

static void
ieee80211tdls_pu_buf_sta_sm_debug_print (void *ctx, const char *fmt,...)
{
    char tmp_buf[256];
    va_list ap;
    peer_uapsd_buf_sta_t *buf_sta = (peer_uapsd_buf_sta_t *) ctx;
    
    va_start(ap, fmt);
    vsnprintf (tmp_buf,256, fmt, ap);
    va_end(ap);
    IEEE80211_DPRINTF(buf_sta->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s", tmp_buf);
}

static void
ieee80211tdls_pu_buf_sta_process_deferred_events(peer_uapsd_buf_sta_t *buf_sta)
{
    ieee80211tdls_pu_buf_sta_sm_event_t *event;
    u_int16_t event_type;
    bool dispatch_event;

    if (! buf_sta) {
        return;
    }

    event_type = buf_sta->deferred_event_type;

    if (event_type != IEEE80211_TDLS_PU_BUF_STA_EVENT_NONE) {

        dispatch_event = false;
        event = &buf_sta->deferred_event;

        switch(event_type) {
        case IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_ON:
        case IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_OFF:
        case IEEE80211_TDLS_PU_BUF_STA_EVENT_PENDING_TX:
        case IEEE80211_TDLS_PU_BUF_STA_EVENT_INDICATION_WINDOW_TMO:
            dispatch_event = true;
            break;

        case IEEE80211_TDLS_PU_BUF_STA_EVENT_NONE:
        case IEEE80211_TDLS_PU_BUF_STA_EVENT_FORCE_SLEEP:
        case IEEE80211_TDLS_PU_BUF_STA_EVENT_FORCE_AWAKE:
        case IEEE80211_TDLS_PU_BUF_STA_EVENT_NO_ACTIVITY:
        case IEEE80211_TDLS_PU_BUF_STA_EVENT_NO_PENDING_TX:
        case IEEE80211_TDLS_PU_BUF_STA_EVENT_SENT_PTI:
        case IEEE80211_TDLS_PU_BUF_STA_EVENT_PTI_TIMEOUT:
        case IEEE80211_TDLS_PU_BUF_STA_EVENT_RCVD_PTR:
        default:
            dispatch_event = false;
            break;
        }

        if (dispatch_event) {
            ieee80211tdls_pu_buf_sta_sm_dispatch(buf_sta, event_type, event);
            dispatch_event = false;
        }

        /* Clear deferred event */
        ieee80211tdls_pu_buf_sta_clear_event(buf_sta);
    }

    return;
}

/* 
 * different state related functions.
 */
/*
 * State IDLE
 */
static void
ieee80211tdls_pu_buf_sta_state_idle_entry(void *ctx)
{
    peer_uapsd_buf_sta_t *buf_sta = (peer_uapsd_buf_sta_t *) ctx;
    
    /* We do not expect events in this state */ 
    ieee80211_vap_txrx_unregister_event_handler(buf_sta->w_tdls->tdls_vap,
            tdls_pu_buf_sta_txrx_event_handler, (void *) buf_sta);
    
    /* This is a stable state, process deferred events if any ...  */
    ieee80211tdls_pu_buf_sta_process_deferred_events(buf_sta);
}

/* 
*Register the event handler to get notification whent station enters into Power save mode with respect to AP
*/
static void
ieee80211tdls_pu_buf_sta_state_idle_exit(void *ctx)
{
    peer_uapsd_buf_sta_t *buf_sta = (peer_uapsd_buf_sta_t *) ctx;
    u_int32_t event_filter;
    u_int32_t error;
    struct ieee80211com      *ic;

    ic = buf_sta->w_tdls->tdls_vap->iv_ic;

    event_filter = IEEE80211_VAP_OUTPUT_EVENT_COMPLETE_PS_NULL;
    error = ieee80211_vap_txrx_register_event_handler(buf_sta->w_tdls->tdls_vap,
            tdls_pu_buf_sta_txrx_event_handler, (void *) buf_sta, event_filter);

    if (error != EOK) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                             "%s : ieee80211_vap_txrx_register_event_handler failure!\n", __FUNCTION__);
    }

    buf_sta->qosnull_attempt = 0;
}

static bool
ieee80211tdls_pu_buf_sta_state_idle_event(void *ctx, u_int16_t event_type, u_int16_t event_data_len, void *event_data)
{
    peer_uapsd_buf_sta_t *buf_sta = (peer_uapsd_buf_sta_t *) ctx;
    bool retVal = true;
 
    if (buf_sta == NULL) {
        return false;
    }

    switch (event_type) {
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_ON:

        /* transition to NOTIFY state */
        ieee80211_sm_transition_to(buf_sta->hsm_handle, IEEE80211_TDLS_PU_BUF_STA_STATE_NOTIFY);
        break;

    case IEEE80211_TDLS_PU_BUF_STA_EVENT_NONE:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_OFF:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PENDING_TX:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_SENT_PTI:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PTI_TIMEOUT:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_RCVD_PTR:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_INDICATION_WINDOW_TMO:
    default:
        break;
    }
    
    return retVal;
}

/*
 * State NOTIFY
 */
static void
ieee80211tdls_pu_buf_sta_state_notify_entry(void *ctx)
{
    peer_uapsd_buf_sta_t *buf_sta = (peer_uapsd_buf_sta_t *) ctx;
    u_int32_t bintval_usec, duration_msec;

    buf_sta->indication_window_tmo = false;
    buf_sta->pending_tx = false;
    buf_sta->enable_pending_tx_notification = false;
    
    /* Start OS timer for indication window timeout */
    bintval_usec = ieee80211_node_get_beacon_interval(buf_sta->w_tdls->tdls_vap->iv_bss);
    duration_msec = (bintval_usec *
            buf_sta->w_tdls->tdls_ni->ni_ic->ic_tdls->peer_uapsd_indication_window) / 1000;

    OS_SET_TIMER(&buf_sta->buf_sta_sm_timer, duration_msec);

    return;
}

static bool
ieee80211tdls_pu_buf_sta_state_notify_event(void *ctx, u_int16_t event_type, u_int16_t event_data_len, void *event_data)
{
    struct ieee80211com      *ic;
    struct ieee80211_node *ni;
    peer_uapsd_buf_sta_t *buf_sta = (peer_uapsd_buf_sta_t *) ctx;
    bool retVal = true;
    int rc; 
#define  ENFORCE_MAX_SP_LIMIT true
#define  NO_MAX_SP_LIMIT false

    if (buf_sta == NULL) {
        return false;
    }

    ni = buf_sta->w_tdls->tdls_ni;
    ic = ni->ni_ic;

    switch (event_type) {
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_INDICATION_WINDOW_TMO:
        OS_CANCEL_TIMER(&buf_sta->buf_sta_sm_timer);

        buf_sta->indication_window_tmo = true;

        if (buf_sta->pending_tx == true) {
            /* transition to WAIT state */
            ieee80211_sm_transition_to(buf_sta->hsm_handle, IEEE80211_TDLS_PU_BUF_STA_STATE_WAIT);
        }
        else {
            buf_sta->enable_pending_tx_notification = true;
        }
        break;

    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_OFF:
        OS_CANCEL_TIMER(&buf_sta->buf_sta_sm_timer);

        buf_sta->enable_pending_tx_notification = false;

        /* transition to IDLE state */
        ieee80211_sm_transition_to(buf_sta->hsm_handle, IEEE80211_TDLS_PU_BUF_STA_STATE_IDLE);
        break;

    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PENDING_TX:
        buf_sta->pending_tx = true;
        buf_sta->enable_pending_tx_notification = false;

        /* Check if Peer U-APSD Indication Window has expired */
        if (buf_sta->indication_window_tmo == true) {
            /* transition to WAIT state */
            ieee80211_sm_transition_to(buf_sta->hsm_handle, IEEE80211_TDLS_PU_BUF_STA_STATE_WAIT);
        }
        break;
    
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_FORCE_SLEEP:
        OS_CANCEL_TIMER(&buf_sta->buf_sta_sm_timer);

        /* Set flag to indicate a sleep transition in progress */
        buf_sta->process_sleep_transition = true;
        /* transition to NOTIFY state */
        ieee80211_sm_transition_to(buf_sta->hsm_handle, IEEE80211_TDLS_PU_BUF_STA_STATE_WAIT);
        break;

    case IEEE80211_TDLS_PU_BUF_STA_EVENT_SEND_QOSNULL_SUCCESS:
        buf_sta->qosnull_attempt = 0;
        break;

    case IEEE80211_TDLS_PU_BUF_STA_EVENT_SEND_QOSNULL_FAIL:
        rc = ieee80211tdls_retry_qosnull(buf_sta);
        if (rc != EOK) {
            OS_CANCEL_TIMER(&buf_sta->buf_sta_sm_timer);
            buf_sta->enable_pending_tx_notification = false;

            /* transition to IDLE state */
            ieee80211_sm_transition_to(buf_sta->hsm_handle, IEEE80211_TDLS_PU_BUF_STA_STATE_IDLE);
        }
        break;
	
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_NONE:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_ON:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_SENT_PTI:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PTI_TIMEOUT:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_RCVD_PTR:
    default:
        break;
    }
    
    return retVal;

#undef  ENFORCE_MAX_SP_LIMIT
#undef  NO_MAX_SP_LIMIT
}

/*
 * State WAIT
 */
static void
ieee80211tdls_pu_buf_sta_state_wait_entry(void *ctx)
{
    peer_uapsd_buf_sta_t *buf_sta = (peer_uapsd_buf_sta_t *) ctx;
    u_int32_t duration_msec;
    struct ieee80211_node *ni = buf_sta->w_tdls->tdls_ni;

    duration_msec = buf_sta->w_tdls->tdls_ni->ni_ic->ic_tdls->response_timeout_msec;

    /* Send Peer Traffic Indication */
    ieee80211_tdls_send_mgmt(buf_sta->w_tdls->tdls_vap,
            IEEE80211_TDLS_PEER_TRAFFIC_INDICATION, buf_sta->w_tdls->tdls_ni->ni_macaddr);
    buf_sta->pti_retry_count = 0;
    ieee80211tdls_send_qosnull(buf_sta->w_tdls, ni, ni->ni_uapsd);
    
    OS_SET_TIMER(&buf_sta->action_response_timer, duration_msec);
}

static bool
ieee80211tdls_pu_buf_sta_state_wait_event(void *ctx, u_int16_t event_type, u_int16_t event_data_len, void *event_data)
{
    struct ieee80211com      *ic;
    struct ieee80211_node *ni;
    char macaddr[18];
    peer_uapsd_buf_sta_t *buf_sta = (peer_uapsd_buf_sta_t *) ctx;
    bool retVal = true;
    u_int32_t duration_msec; 

#define  ENFORCE_MAX_SP_LIMIT true
#define  NO_MAX_SP_LIMIT false

    if (buf_sta == NULL) {
        return false;
    }

    ni = buf_sta->w_tdls->tdls_ni;
    ic = ni->ni_ic;

    switch (event_type) {
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_RCVD_PTR:
        /* Cancel PTI response timeout */
        OS_CANCEL_TIMER(&buf_sta->action_response_timer);
       
        if (buf_sta->process_sleep_transition) {
            ieee80211tdls_send_qosnull(buf_sta->w_tdls, ni, ni->ni_uapsd);
        }

        if (buf_sta->dialog_token >= 0x7f) {
            buf_sta->dialog_token = 1;
        }
        else {
            /* Increment the dialog token */
            buf_sta->dialog_token += 1;
        }
        /* Enter idle state only after PS bit is turned off by the peer */ 
        /* transition to NOTIFY state */
        ieee80211_sm_transition_to(buf_sta->hsm_handle, IEEE80211_TDLS_PU_BUF_STA_STATE_NOTIFY);
        break;

    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PTI_TIMEOUT:
        /* Cancel PTI response timeout */
        OS_CANCEL_TIMER(&buf_sta->action_response_timer);
         
        buf_sta->pti_retry_count += 1;
        if (buf_sta->pti_retry_count > IEEE80211_TDLS_PU_BUF_STA_PTI_RETRIES) {
            buf_sta->process_sleep_transition = false;

            ieee80211_vap_txrx_unregister_event_handler(buf_sta->w_tdls->tdls_vap,
                    tdls_pu_buf_sta_txrx_event_handler, (void *) buf_sta);

            IEEE80211_ADDR_COPY(macaddr, ni->ni_macaddr);

            ieee80211_tdls_send_mgmt(ni->ni_vap, IEEE80211_TDLS_TEARDOWN, macaddr);

            /* transition to TEARDOWN state */
            ieee80211_sm_transition_to(buf_sta->hsm_handle, IEEE80211_TDLS_PU_BUF_STA_STATE_IDLE);
        }
        else {
            duration_msec = buf_sta->w_tdls->tdls_ni->ni_ic->ic_tdls->response_timeout_msec;

            /* Send Peer Traffic Indication */
            ieee80211_tdls_send_mgmt(buf_sta->w_tdls->tdls_vap,
                    IEEE80211_TDLS_PEER_TRAFFIC_INDICATION, buf_sta->w_tdls->tdls_ni->ni_macaddr);

            ieee80211tdls_send_qosnull(buf_sta->w_tdls, ni, ni->ni_uapsd);

            OS_SET_TIMER(&buf_sta->action_response_timer, duration_msec);
        }

        break;

    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_OFF:
        /* Cancel PTI response timeout */
        OS_CANCEL_TIMER(&buf_sta->action_response_timer);
        buf_sta->process_sleep_transition = false;
        /* transition to TEARDOWN state */
        ieee80211_sm_transition_to(buf_sta->hsm_handle, IEEE80211_TDLS_PU_BUF_STA_STATE_IDLE);
        break;
    
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_FORCE_SLEEP:
     
         /* Set flag to indicate a sleep transition in progress */
         buf_sta->process_sleep_transition = true;
         break;

    case IEEE80211_TDLS_PU_BUF_STA_EVENT_SEND_QOSNULL_SUCCESS:
          buf_sta->qosnull_attempt = 0;
          break;
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_SEND_QOSNULL_FAIL:
          (void) ieee80211tdls_retry_qosnull(buf_sta);
           break;


    case IEEE80211_TDLS_PU_BUF_STA_EVENT_NONE:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_ON:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PENDING_TX:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_SENT_PTI:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_INDICATION_WINDOW_TMO:
    default:
        break;
    }
    
    return retVal;

#undef  ENFORCE_MAX_SP_LIMIT
#undef  NO_MAX_SP_LIMIT
}

/*
 * State TEARDOWN
 */
static void
ieee80211tdls_pu_buf_sta_state_teardown_entry(void *ctx)
{
    //peer_uapsd_buf_sta_t *buf_sta = (peer_uapsd_buf_sta_t *) ctx;
    
}

static bool
ieee80211tdls_pu_buf_sta_state_teardown_event(void *ctx, u_int16_t event_type, u_int16_t event_data_len, void *event_data)
{
    peer_uapsd_buf_sta_t *buf_sta = (peer_uapsd_buf_sta_t *) ctx;
    bool retVal = true;
 
    if (buf_sta == NULL) {
        return false;
    }

    switch (event_type) {
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_ON:
        /* Start OS timer for indication window timeout */

        /* transition to NOTIFY state */

        break;
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_NONE:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_OFF:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PENDING_TX:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_SENT_PTI:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_PTI_TIMEOUT:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_RCVD_PTR:
    case IEEE80211_TDLS_PU_BUF_STA_EVENT_INDICATION_WINDOW_TMO:
    default:
        break;
    }
    
    return retVal;
}

ieee80211_state_info ieee80211_tdls_pu_buf_sta_sm_info[] = {
    { 
        (u_int8_t) IEEE80211_TDLS_PU_BUF_STA_STATE_IDLE, 
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "IDLE",
        ieee80211tdls_pu_buf_sta_state_idle_entry,
        ieee80211tdls_pu_buf_sta_state_idle_exit,
        ieee80211tdls_pu_buf_sta_state_idle_event
    },
    { 
        (u_int8_t) IEEE80211_TDLS_PU_BUF_STA_STATE_NOTIFY, 
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "NOTIFY",
        ieee80211tdls_pu_buf_sta_state_notify_entry,
        NULL,
        ieee80211tdls_pu_buf_sta_state_notify_event
    },
    { 
        (u_int8_t) IEEE80211_TDLS_PU_BUF_STA_STATE_WAIT, 
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "WAIT",
        ieee80211tdls_pu_buf_sta_state_wait_entry,
        NULL,
        ieee80211tdls_pu_buf_sta_state_wait_event
    },
    { 
        (u_int8_t) IEEE80211_TDLS_PU_BUF_STA_STATE_TEARDOWN, 
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "TEARDOWN",
        ieee80211tdls_pu_buf_sta_state_teardown_entry,
        NULL,
        ieee80211tdls_pu_buf_sta_state_teardown_event
    },
};

OS_TIMER_FUNC(pu_buf_sta_sm_timeout)
{
    peer_uapsd_buf_sta_t *buf_sta;
    ieee80211tdls_pu_buf_sta_sm_event_t event;

    OS_GET_TIMER_ARG(buf_sta, peer_uapsd_buf_sta_t *);

    event.vap = buf_sta->w_tdls->tdls_vap;
    event.ni_tdls = buf_sta->w_tdls->tdls_ni;
    event.ni_bss_node = NULL;

    /* Dispatch PU Buffer STA SM */
    ieee80211tdls_pu_buf_sta_sm_dispatch(buf_sta,
            IEEE80211_TDLS_PU_BUF_STA_EVENT_INDICATION_WINDOW_TMO,
            &event);
}

OS_TIMER_FUNC(pu_buf_sta_action_response_timeout)
{
    peer_uapsd_buf_sta_t *buf_sta;
    ieee80211tdls_pu_buf_sta_sm_event_t event;

    OS_GET_TIMER_ARG(buf_sta, peer_uapsd_buf_sta_t *);

    event.vap = buf_sta->w_tdls->tdls_vap;
    event.ni_tdls = buf_sta->w_tdls->tdls_ni;
    event.ni_bss_node = NULL;

    /* Dispatch PU Buffer STA SM */
    ieee80211tdls_pu_buf_sta_sm_dispatch(buf_sta,
            IEEE80211_TDLS_PU_BUF_STA_EVENT_PTI_TIMEOUT,
            &event);
}

/* Initialize the PU Buffer STA */
static void
tdls_pu_init_buf_sta(struct ieee80211_node *ni, peer_uapsd_buf_sta_t *buf_sta)
{
    struct ieee80211com      *ic;
    
    ic = buf_sta->w_tdls->tdls_ni->ni_ic;

    buf_sta->state = IEEE80211_TDLS_PEER_UAPSD_STATE_NOT_READY;
    buf_sta->deferred_event_type = IEEE80211_TDLS_PU_BUF_STA_EVENT_NONE;
    OS_MEMZERO(&buf_sta->deferred_event, sizeof(ieee80211tdls_pu_buf_sta_sm_event_t));
    buf_sta->indication_window_tmo = false;
    buf_sta->pending_tx = false;
    buf_sta->enable_pending_tx_notification = false;
    buf_sta->dialog_token = 1;
    buf_sta->process_sleep_transition = false;
    /* Create State Machine and start */
    buf_sta->hsm_handle = ieee80211_sm_create(ic->ic_osdev, 
                         "TDLS_PU_BUF_STA",
                         (void *) buf_sta, 
                         IEEE80211_TDLS_PU_BUF_STA_STATE_IDLE,
                         ieee80211_tdls_pu_buf_sta_sm_info,
                         sizeof(ieee80211_tdls_pu_buf_sta_sm_info)/sizeof(ieee80211_state_info),
                         MAX_QUEUED_EVENTS,
                         sizeof(struct ieee80211tdls_pu_buf_sta_sm_event), /* size of event data */
                         MESGQ_PRIORITY_HIGH,
                                 IEEE80211_HSM_ASYNCHRONOUS, 
                                         ieee80211tdls_pu_buf_sta_sm_debug_print,
                                         tdls_pu_buf_sta_event_name,
                                         IEEE80211_N(tdls_pu_buf_sta_event_name)); 
    
    if (!buf_sta->hsm_handle) {
        printk("%s : ieee80211_sm_create failed \n", __func__);
        goto error1;
    }

    /* Allocate an OS Timers */
    OS_INIT_TIMER(ic->ic_osdev, &buf_sta->buf_sta_sm_timer,
            pu_buf_sta_sm_timeout, buf_sta);
    OS_INIT_TIMER(ic->ic_osdev, &buf_sta->action_response_timer,
            pu_buf_sta_action_response_timeout, buf_sta);
    OS_INIT_TIMER(ic->ic_osdev, &buf_sta->async_qosnull_event_timer,
            pu_buf_async_qosnull_sm_timeout, buf_sta);
    buf_sta->state = IEEE80211_TDLS_PEER_UAPSD_STATE_READY;

error1:

    return;
}

/* Cleanup the PU Buffer STA */
static void
tdls_pu_cleanup_buf_sta(struct ieee80211_node *ni, peer_uapsd_buf_sta_t *buf_sta)
{
    buf_sta->state = IEEE80211_TDLS_PEER_UAPSD_STATE_NOT_READY;
    ieee80211_vap_txrx_unregister_event_handler(buf_sta->w_tdls->tdls_vap,
            tdls_pu_buf_sta_txrx_event_handler, (void *) buf_sta);
    
    /* Delete the HSM */
    if (buf_sta->hsm_handle) {
        ieee80211_sm_delete(buf_sta->hsm_handle);
    }
    
    OS_FREE_TIMER(&buf_sta->buf_sta_sm_timer);
    OS_FREE_TIMER(&buf_sta->action_response_timer);
    OS_FREE_TIMER(&buf_sta->async_qosnull_event_timer);
    return;
}

static const char* tdls_pu_sleep_sta_event_name[] = {
/* IEEE80211_TDLS_PU_SLEEP_STA_EVENT_NONE */				    "TDLS_PU_SLEEP_NONE",
/* IEEE80211_TDLS_PU_SLEEP_STA_EVENT_START */			        "TDLS_PU_SLEEP_START",
/* IEEE80211_TDLS_PU_SLEEP_STA_EVENT_STOP */			        "TDLS_PU_SLEEP_STOP",
/* IEEE80211_TDLS_PU_SLEEP_STA_EVENT_FORCE_SLEEP */		        "TDLS_PU_SLEEP_FORCE_SLEEP",
/* IEEE80211_TDLS_PU_SLEEP_STA_EVENT_FORCE_AWAKE */		        "TDLS_PU_SLEEP_FORCE_AWAKE",
/* IEEE80211_TDLS_PU_SLEEP_STA_EVENT_NO_ACTIVITY */		        "TDLS_PU_SLEEP_NO_ACTIVITY",
/* IEEE80211_TDLS_PU_SLEEP_STA_EVENT_NO_PENDING_TX */		    "TDLS_PU_SLEEP_NO_PENDING_TX",
/* IEEE80211_TDLS_PU_SLEEP_STA_EVENT_SEND_NULL_SUCCESS */       "TDLS_PU_SLEEP_SEND_NULL_SUCCESS",
/* IEEE80211_TDLS_PU_SLEEP_STA_EVENT_SEND_NULL_FAILED */        "TDLS_PU_SLEEP_SEND_NULL_FAILED",
/* IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_PTI */                "TDLS_PU_SLEEP_RCVD_PTI",
/* IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_EOSP */               "TDLS_PU_SLEEP_RCVD_EOSP",
/* IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RECV_UCAST */              "TDLS_PU_SLEEP_RECV_UCAST",
/* IEEE80211_TDLS_PU_SLEEP_STA_EVENT_TX */                      "TDLS_PU_SLEEP_TX",
/* IEEE80211_TDLS_PU_SLEEP_STA_EVENT_LAST_MCAST */              "TDLS_PU_SLEEP_LAST_MCAST",
/* IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RECV_UCAST_MOREDATA */     "TDLS_PU_SLEEP_RECV_UCAST_MORE_DATA",
};

/*
 * txrx event handler to find out when the client enters in to sleep
 * wrt AP.
 */
static void tdls_pu_sleep_sta_txrx_event_handler (ieee80211_vap_t vap,
        ieee80211_vap_txrx_event *event, void *arg)
{
    peer_uapsd_sleep_sta_t *sleep_sta = (peer_uapsd_sleep_sta_t *) arg;
    ieee80211tdls_pu_sleep_sta_sm_event_t pu_sleep_sta_event;
    u_int32_t event_filter;
    u_int32_t error;
    struct ieee80211com      *ic;
    
    if (vap == NULL || event == NULL || arg == NULL) {
        return;
    }

    ic = sleep_sta->w_tdls->tdls_ni->ni_ic;

    if (event->ni == NULL || event->ni != sleep_sta->w_tdls->tdls_ni) {
        /* Filter this event; It doesn't belong to this handler */
        return;
    }

    pu_sleep_sta_event.ni_bss_node = NULL;
    pu_sleep_sta_event.ni_tdls = sleep_sta->w_tdls->tdls_ni;
    pu_sleep_sta_event.vap = sleep_sta->w_tdls->tdls_vap;
    pu_sleep_sta_event.wme_info_uapsd = 0;

    switch(event->type) {
    case IEEE80211_VAP_INPUT_EVENT_EOSP:

        IEEE80211_DPRINTF(sleep_sta->w_tdls->tdls_vap,IEEE80211_MSG_TDLS,
                "%s: EOSP: UAPSD SP end \n", __FUNCTION__);

        event_filter = 0;
        error = ieee80211_vap_txrx_register_event_handler(sleep_sta->w_tdls->tdls_vap,
                tdls_pu_sleep_sta_txrx_event_handler, (void *) sleep_sta, event_filter);

        if (error != EOK) {
            IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_TDLS,
                                 "%s : ieee80211_vap_txrx_register_event_handler failure!\n", __FUNCTION__);
        }
        
        ieee80211tdls_pu_sleep_sta_sm_dispatch(sleep_sta,
                IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_EOSP, &pu_sleep_sta_event);

        break;
    case IEEE80211_VAP_OUTPUT_EVENT_COMPLETE_PS_NULL:
        if (event->u.status == 0) {
            ieee80211tdls_pu_sleep_sta_sm_dispatch(sleep_sta,
                    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_SEND_NULL_SUCCESS, &pu_sleep_sta_event);
        } else {
            /*
             * this could have been failed synchronously from the SM with the ieee80211_send_nulldata() function call
             * do not dispatch the event synchronously instead send it asynchronously.
             */
             OS_SET_TIMER(&sleep_sta->async_null_event_timer,0);
        }
        break;
    case IEEE80211_VAP_INPUT_EVENT_UCAST:
        if (event->u.more_data != 0) {
            ieee80211tdls_pu_sleep_sta_sm_dispatch(sleep_sta,
                    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RECV_UCAST_MOREDATA, &pu_sleep_sta_event);
        }
        break;
    default:
        break;
    }

    return;
}

/* Event Deferring and Processing */
static bool
ieee80211tdls_pu_sleep_sta_defer_event(peer_uapsd_sleep_sta_t *sleep_sta,
        ieee80211tdls_pu_sleep_sta_sm_event_t *event,
        u_int16_t event_type)
{
    struct ieee80211com      *ic;

    if (sleep_sta) {
        ic = sleep_sta->w_tdls->tdls_ni->ni_ic;

        /* VAP related event */
        if (!sleep_sta->deferred_event_type) {
            if (event->vap) {
                event->ni_bss_node=ieee80211_ref_bss_node(event->vap);
            }

            OS_MEMCPY(&sleep_sta->deferred_event, event, sizeof(ieee80211tdls_pu_sleep_sta_sm_event_t));
            sleep_sta->deferred_event_type = event_type;
            IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                                 "%s: defer event %d vap %d \n ",__func__,event_type,event->vap?event->vap->iv_unit:-1 );
            return true;
        }
    }

    return false;
}

static void
ieee80211tdls_pu_sleep_sta_clear_event(peer_uapsd_sleep_sta_t *sleep_sta)
{
    struct ieee80211com      *ic;

    ic = sleep_sta->w_tdls->tdls_ni->ni_ic;

    if (sleep_sta) {
        /* VAP related event */
       if (sleep_sta->deferred_event.ni_bss_node) {
           ieee80211_free_node(sleep_sta->deferred_event.ni_bss_node);
       }
       IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                                 "%s: clear deferred event %d \n ",__func__,sleep_sta->deferred_event_type);
        OS_MEMZERO(&sleep_sta->deferred_event, sizeof(ieee80211tdls_pu_sleep_sta_sm_event_t));
        sleep_sta->deferred_event_type = IEEE80211_TDLS_PU_SLEEP_STA_EVENT_NONE;
    }
}

static void
ieee80211tdls_pu_sleep_sta_sm_debug_print (void *ctx, const char *fmt,...)
{
    char tmp_buf[256];
    va_list ap;
    peer_uapsd_sleep_sta_t *sleep_sta = (peer_uapsd_sleep_sta_t *) ctx;
    struct ieee80211com      *ic;

    ic = sleep_sta->w_tdls->tdls_ni->ni_ic;

    va_start(ap, fmt);
    vsnprintf (tmp_buf,256, fmt, ap);
    va_end(ap);
    IEEE80211_DPRINTF(sleep_sta->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s", tmp_buf);
}

static void
ieee80211tdls_pu_sleep_sta_process_deferred_events(peer_uapsd_sleep_sta_t *sleep_sta)
{
    ieee80211tdls_pu_sleep_sta_sm_event_t *event;
    u_int16_t event_type;
    bool dispatch_event;

    if (! sleep_sta) {
        return;
    }

    event_type = sleep_sta->deferred_event_type;

    if (event_type != IEEE80211_TDLS_PU_SLEEP_STA_EVENT_NONE) {

        dispatch_event = false;
        event = &sleep_sta->deferred_event;

        switch(event_type) {
        case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_PTI:
        case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_EOSP:
            dispatch_event = true;
            break;

        case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_START:
        case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_STOP:
        case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_FORCE_SLEEP:
        case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_FORCE_AWAKE:
        case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_NO_ACTIVITY:
        case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_NO_PENDING_TX:
        case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_SEND_NULL_SUCCESS:
        case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_SEND_NULL_FAILED:
        default:
            dispatch_event = false;
            break;
        }

        if (dispatch_event) {
            ieee80211tdls_pu_sleep_sta_sm_dispatch(sleep_sta, event_type, event);
            dispatch_event = false;
        }

        /* Clear deferred event */
        ieee80211tdls_pu_sleep_sta_clear_event(sleep_sta);
    }

    return;
}

/* 
 * different state related functions.
 */
/*
 * State IDLE
 */
static void
ieee80211tdls_pu_sleep_sta_state_idle_entry(void *ctx)
{
    return;
}

static void
ieee80211tdls_pu_sleep_sta_state_idle_exit(void *ctx)
{
    return;
}

static bool
ieee80211tdls_pu_sleep_sta_state_idle_event(void *ctx, u_int16_t event_type, u_int16_t event_data_len, void *event_data)
{
    peer_uapsd_sleep_sta_t *sleep_sta = (peer_uapsd_sleep_sta_t *) ctx;
    bool retVal = true;

    if (sleep_sta == NULL) {
        retVal = false;
    }

    switch (event_type) {
    case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_FORCE_SLEEP:
	    printk("TDLD: %s : %d\n",__func__,__LINE__);	
        /* The Buf STA SM will send a QosNull to remote peer */
        /* transition to SLEEP state */
        ieee80211_sm_transition_to(sleep_sta->hsm_handle, IEEE80211_TDLS_PU_SLEEP_STA_STATE_NULL_SENT);
		break;
    case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_FORCE_AWAKE:
        /* transition to ACTIVE state */
        ieee80211_sm_transition_to(sleep_sta->hsm_handle, IEEE80211_TDLS_PU_SLEEP_STA_STATE_ACTIVE);
        break;
    case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_PTI:
    case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_EOSP:
    default:
        break;
    }

    return retVal;
}

/*
 * State ACTIVE
 */
static void
ieee80211tdls_pu_sleep_sta_state_active_entry(void *ctx)
{
    peer_uapsd_sleep_sta_t *sleep_sta = (peer_uapsd_sleep_sta_t *) ctx;
    u_int32_t event_filter;
    u_int32_t error;
    struct ieee80211com      *ic;

    ic = sleep_sta->w_tdls->tdls_ni->ni_ic;

    event_filter = IEEE80211_VAP_INPUT_EVENT_EOSP;
    error = ieee80211_vap_txrx_register_event_handler(sleep_sta->w_tdls->tdls_vap,
            tdls_pu_sleep_sta_txrx_event_handler, (void *) sleep_sta, event_filter);

    if (error != EOK) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                             "%s : ieee80211_vap_txrx_register_event_handler failure!\n", __FUNCTION__);
        return;
    }

    return;
}

static void
ieee80211tdls_pu_sleep_sta_state_active_exit(void *ctx)
{
    peer_uapsd_sleep_sta_t *sleep_sta = (peer_uapsd_sleep_sta_t *) ctx;

    (void) ieee80211_vap_txrx_unregister_event_handler(sleep_sta->w_tdls->tdls_vap,
            tdls_pu_sleep_sta_txrx_event_handler, (void *) sleep_sta);

    return;
}

static bool
ieee80211tdls_pu_sleep_sta_state_active_event(void *ctx, u_int16_t event_type, u_int16_t event_data_len, void *event_data)
{
    peer_uapsd_sleep_sta_t *sleep_sta = (peer_uapsd_sleep_sta_t *) ctx;
    bool retVal = true;
    struct ieee80211com      *ic;

    if (sleep_sta == NULL) {
        retVal = false;
    }

    ic = sleep_sta->w_tdls->tdls_ni->ni_ic;

    switch (event_type) {
    case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_EOSP:
        /* transition to ACTIVE state */
        ieee80211_sm_transition_to(sleep_sta->hsm_handle, IEEE80211_TDLS_PU_SLEEP_STA_STATE_SLEEP);
        break;
    case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_FORCE_SLEEP:
        /* transition to NULL_SENT state */
        ieee80211_sm_transition_to(sleep_sta->hsm_handle, IEEE80211_TDLS_PU_SLEEP_STA_STATE_NULL_SENT);
        break;
    case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_PTI:
        /* Send TDLS Peer U-APSD Traffic Response */
        ieee80211_tdls_send_mgmt(sleep_sta->w_tdls->tdls_vap,
                IEEE80211_TDLS_PEER_TRAFFIC_RESPONSE, sleep_sta->w_tdls->tdls_ni->ni_macaddr);
        break;
    default:
        break;
    }

    return retVal;
}

/*
 * State NULLSENT
 */
static void
ieee80211tdls_pu_sleep_sta_state_nullsent_entry(void *ctx)
{
    peer_uapsd_sleep_sta_t *sleep_sta = (peer_uapsd_sleep_sta_t *) ctx;
    wlan_tdls_sm_t sm;
    u_int32_t event_filter;
    u_int32_t error, rc;
    struct ieee80211com      *ic;

    sm = sleep_sta->w_tdls;
    ic = sleep_sta->w_tdls->tdls_vap->iv_ic;

    event_filter = (IEEE80211_VAP_INPUT_EVENT_UCAST |
            IEEE80211_VAP_OUTPUT_EVENT_DATA |
            IEEE80211_VAP_OUTPUT_EVENT_COMPLETE_PS_NULL |
			IEEE80211_VAP_INPUT_EVENT_EOSP);
    error = ieee80211_vap_txrx_register_event_handler(sleep_sta->w_tdls->tdls_vap,
            tdls_pu_sleep_sta_txrx_event_handler, (void *) sleep_sta, event_filter);

    if (error != EOK) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                             "%s : ieee80211_vap_txrx_register_event_handler failure!\n", __FUNCTION__);
        return;
    }

    sleep_sta->sleep_attempt = 0;
    rc = ieee80211tdls_send_qosnull(sm, sm->tdls_ni, sm->tdls_ni->ni_uapsd);
	if (rc != EOK) {
        IEEE80211_DPRINTF(sm->tdls_vap, IEEE80211_MSG_POWER, "%s: ieee80211_send_nulldata failed PM=1 rc=%d (%0x08X)\n",
                          __func__,
                          rc, rc);
    }

    return;
}

static void
ieee80211tdls_pu_sleep_sta_state_nullsent_exit(void *ctx)
{
    peer_uapsd_sleep_sta_t *sleep_sta = (peer_uapsd_sleep_sta_t *) ctx;

    (void) ieee80211_vap_txrx_unregister_event_handler(sleep_sta->w_tdls->tdls_vap,
            tdls_pu_sleep_sta_txrx_event_handler, (void *) sleep_sta);

    return;
}

static bool
ieee80211tdls_pu_sleep_sta_state_nullsent_event(void *ctx, u_int16_t event_type, u_int16_t event_data_len, void *event_data)
{
    peer_uapsd_sleep_sta_t *sleep_sta = (peer_uapsd_sleep_sta_t *) ctx;
    bool retVal = true;
    u_int32_t rc = 0;

    if (sleep_sta == NULL) {
        retVal = false;
    }

    switch (event_type) {
    case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_SEND_NULL_FAILED:
        ++sleep_sta->sleep_attempt;         /* current attempt for sending out null frame */
        if (sleep_sta->sleep_attempt < IEEE80211TDLS_MAX_SLEEP_ATTEMPTS) {
            rc = ieee80211tdls_send_qosnull(sleep_sta->w_tdls, sleep_sta->w_tdls->tdls_ni, sleep_sta->w_tdls->tdls_ni->ni_uapsd);
			if (rc != EOK) {
                IEEE80211_DPRINTF(sleep_sta->w_tdls->tdls_vap, IEEE80211_MSG_POWER, "%s: ieee80211_send_nulldata failed PM=1 rc=%d (%0x08X)\n",
                                  __func__,
                                  rc, rc);
            }
        } else {
            /* max attempts reached */
            ieee80211_sm_transition_to(sleep_sta->hsm_handle,
                    IEEE80211_TDLS_PU_SLEEP_STA_STATE_ACTIVE);
        }
        break;

    case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_SEND_NULL_SUCCESS:
        ieee80211_sm_transition_to(sleep_sta->hsm_handle, IEEE80211_TDLS_PU_SLEEP_STA_STATE_SLEEP);
        break;
    case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_PTI:
    case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_EOSP:
    default:
        break;
    }

    return retVal;
}

/*
 * State SLEEP
 */
static void
ieee80211tdls_pu_sleep_sta_state_sleep_entry(void *ctx)
{
    peer_uapsd_sleep_sta_t *sleep_sta = (peer_uapsd_sleep_sta_t *) ctx;
    wlan_tdls_sm_t sm;
    struct ieee80211com      *ic;
    u_int32_t event_filter;
    u_int32_t error;
    
    sm = sleep_sta->w_tdls;
    ic = sleep_sta->w_tdls->tdls_vap->iv_ic;
    sleep_sta->rcvd_peer_traffic_indication = 0;
    sleep_sta->rcvd_eosp = 0;

#if DISABLE_FOR_NOW
    (void) ieee80211_power_arbiter_enter_nwsleep(sm->tdls_vap, sm->pwr_arbiter_id);
#endif
    /* Send PTR when QOS data received with EOSP and MORE data bits */
    event_filter = IEEE80211_VAP_INPUT_EVENT_EOSP | IEEE80211_VAP_INPUT_EVENT_UCAST;
    error = ieee80211_vap_txrx_register_event_handler(sleep_sta->w_tdls->tdls_vap,
            tdls_pu_sleep_sta_txrx_event_handler, (void *) sleep_sta, event_filter);

    if (error != EOK) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                             "%s : ieee80211_vap_txrx_register_event_handler failure!\n", __FUNCTION__);
        return;
    }
    return;
}

static void
ieee80211tdls_pu_sleep_sta_state_sleep_exit(void *ctx)
{
    peer_uapsd_sleep_sta_t *sleep_sta = (peer_uapsd_sleep_sta_t *) ctx;
    wlan_tdls_sm_t sm;

    sm = sleep_sta->w_tdls;

#if DISABLE_FOR_NOW
    (void) ieee80211_power_arbiter_exit_nwsleep(sm->tdls_vap, sm->pwr_arbiter_id);
#endif
    ieee80211_vap_txrx_unregister_event_handler(sleep_sta->w_tdls->tdls_vap,
            tdls_pu_sleep_sta_txrx_event_handler, (void *) sleep_sta);

    if (1) {
        ieee80211tdls_send_qosnull(sm, sm->tdls_ni, sm->tdls_ni->ni_uapsd);
    }
    
    if (sleep_sta->rcvd_peer_traffic_indication >= 1) {
        /* Send TDLS Peer U-APSD Traffic Response */
        ieee80211_tdls_send_mgmt(sleep_sta->w_tdls->tdls_vap,
                IEEE80211_TDLS_PEER_TRAFFIC_RESPONSE, sleep_sta->w_tdls->tdls_ni->ni_macaddr);
    }

    return;
}

static bool
ieee80211tdls_pu_sleep_sta_state_sleep_event(void *ctx, u_int16_t event_type, u_int16_t event_data_len, void *event_data)
{
    peer_uapsd_sleep_sta_t *sleep_sta = (peer_uapsd_sleep_sta_t *) ctx;
    bool retVal = true;

    if (sleep_sta == NULL) {
        retVal = false;
    }

    switch (event_type) {
    case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_PTI:
        sleep_sta->rcvd_peer_traffic_indication += 1;
        /* INTENTIONALLY FALLING THROUGH */
        /* Send PTR when QOS data received with EOSP and MORE data bits */
    case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RECV_UCAST_MOREDATA:
	case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_FORCE_AWAKE:
        /* transition to ACTIVE state */
        ieee80211_sm_transition_to(sleep_sta->hsm_handle, IEEE80211_TDLS_PU_SLEEP_STA_STATE_ACTIVE);
        break;
    case IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_EOSP:
        sleep_sta->rcvd_eosp += 1;
    default:
        break;
    }

    return retVal;
}

ieee80211_state_info ieee80211tdls_pu_sleep_sta_sm_info[] = {
    {
        (u_int8_t) IEEE80211_TDLS_PU_SLEEP_STA_STATE_IDLE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "IDLE",
        ieee80211tdls_pu_sleep_sta_state_idle_entry,
        ieee80211tdls_pu_sleep_sta_state_idle_exit,
        ieee80211tdls_pu_sleep_sta_state_idle_event
    },
    {
        (u_int8_t) IEEE80211_TDLS_PU_SLEEP_STA_STATE_ACTIVE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "ACTIVE",
        ieee80211tdls_pu_sleep_sta_state_active_entry,
        ieee80211tdls_pu_sleep_sta_state_active_exit,
        ieee80211tdls_pu_sleep_sta_state_active_event
    },
    {
        (u_int8_t) IEEE80211_TDLS_PU_SLEEP_STA_STATE_NULL_SENT,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "NULLSENT",
        ieee80211tdls_pu_sleep_sta_state_nullsent_entry,
        ieee80211tdls_pu_sleep_sta_state_nullsent_exit,
        ieee80211tdls_pu_sleep_sta_state_nullsent_event
    },
    {
        (u_int8_t) IEEE80211_TDLS_PU_SLEEP_STA_STATE_SLEEP,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "SLEEP",
        ieee80211tdls_pu_sleep_sta_state_sleep_entry,
        ieee80211tdls_pu_sleep_sta_state_sleep_exit,
        ieee80211tdls_pu_sleep_sta_state_sleep_event
    },
};

OS_TIMER_FUNC(pu_sta_async_null_sm_timeout)
{
    peer_uapsd_sleep_sta_t *sleep_sta;
    ieee80211tdls_pu_sleep_sta_sm_event_t event;

    OS_GET_TIMER_ARG(sleep_sta, peer_uapsd_sleep_sta_t *);

    event.vap = sleep_sta->w_tdls->tdls_vap;
    event.ni_tdls = sleep_sta->w_tdls->tdls_ni;
    event.ni_bss_node = NULL;

    /* Dispatch PU Sleep STA SM */
    ieee80211tdls_pu_sleep_sta_sm_dispatch(sleep_sta,
            IEEE80211_TDLS_PU_SLEEP_STA_EVENT_SEND_NULL_FAILED, &event);
}

/* Initialize the PU Sleep STA */
static void
tdls_pu_init_sleep_sta(struct ieee80211_node *ni, peer_uapsd_sleep_sta_t *sleep_sta)
{
    struct ieee80211com      *ic;
    u_int32_t event_filter;
    int32_t error = 0;
    
    ic = sleep_sta->w_tdls->tdls_ni->ni_ic;

    sleep_sta->state = IEEE80211_TDLS_PEER_UAPSD_STATE_NOT_READY;
    sleep_sta->deferred_event_type = IEEE80211_TDLS_PU_BUF_STA_EVENT_NONE;
    OS_MEMZERO(&sleep_sta->deferred_event, sizeof(ieee80211tdls_pu_sleep_sta_sm_event_t));
    
    /* Create State Machine and start */
    sleep_sta->hsm_handle = ieee80211_sm_create(ic->ic_osdev, 
                         "TDLS_PU_SLEEP_STA",
                         (void *) sleep_sta, 
                         IEEE80211_TDLS_PU_SLEEP_STA_STATE_IDLE,
                         ieee80211tdls_pu_sleep_sta_sm_info,
                         sizeof(ieee80211tdls_pu_sleep_sta_sm_info)/sizeof(ieee80211_state_info),
                         MAX_QUEUED_EVENTS,
                         sizeof(struct ieee80211tdls_pu_sleep_sta_sm_event), /* size of event data */
                         MESGQ_PRIORITY_HIGH,
                                 IEEE80211_HSM_ASYNCHRONOUS, 
                                         ieee80211tdls_pu_sleep_sta_sm_debug_print,
                                         tdls_pu_sleep_sta_event_name,
                                         IEEE80211_N(tdls_pu_sleep_sta_event_name)); 
    
    if (!sleep_sta->hsm_handle) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                             "%s : ieee80211_sm_create failure!\n", __FUNCTION__);
        return;
    }

    /* Register an event handler with the TXRX module; An event_filter of zero
     * indicates no notification of TXRX events.
     */
    event_filter = 0;
    error = ieee80211_vap_txrx_register_event_handler(sleep_sta->w_tdls->tdls_vap,
            tdls_pu_sleep_sta_txrx_event_handler, (void *) sleep_sta, event_filter);

    if (error != EOK) {
        IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_STATE,
                             "%s : ieee80211_vap_txrx_register_event_handler failure!\n", __FUNCTION__);
        return;
    }

    /* Allocate an OS Timers */
    OS_INIT_TIMER(ic->ic_osdev, &sleep_sta->async_null_event_timer,
            pu_sta_async_null_sm_timeout, sleep_sta);

    sleep_sta->state = IEEE80211_TDLS_PEER_UAPSD_STATE_READY;

    return;
}

/* Cleanup the PU Sleep STA */
static void
tdls_pu_cleanup_sleep_sta(struct ieee80211_node *ni, peer_uapsd_sleep_sta_t *sleep_sta)
{
    sleep_sta->state = IEEE80211_TDLS_PEER_UAPSD_STATE_NOT_READY;
    
    (void) ieee80211_vap_txrx_unregister_event_handler(sleep_sta->w_tdls->tdls_vap,
            tdls_pu_sleep_sta_txrx_event_handler, (void *) sleep_sta);

    /* Delete the HSM */
    if (sleep_sta->hsm_handle) {
        ieee80211_sm_delete(sleep_sta->hsm_handle);
    }
    
    return;
}

#endif /* UMAC_SUPPORT_TDLS_PEER_UAPSD */

#endif /* UMAC_SUPPORT_TDLS */

