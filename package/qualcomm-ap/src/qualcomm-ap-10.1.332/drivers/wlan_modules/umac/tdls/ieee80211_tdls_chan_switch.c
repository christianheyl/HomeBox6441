/*
 *  Copyright (c) 2010-2012 Atheros Communications Inc.
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
#include "ieee80211_tdls_notifier.h"
#include "ieee80211_tdls_priv.h"
#include "ieee80211_tdls_chan_switch.h"
//#include "ieee80211_tdls_link_monitor.h"

#if UMAC_SUPPORT_TDLS_CHAN_SWITCH

#include "ieee80211_channel.h"

#define MAX_QUEUED_EVENTS  16

/*
 * Generic timeouts used in Channel Switching.
 * Values in milliseconds unless noted
 */
#define TDLS_FAKESLEEP_TIMEOUT                    200
#define TDLS_TX_REQUEST_TIMEOUT                   200
#define TDLS_TX_RESPONSE_TIMEOUT                  200 
#define TDLS_TX_UNSOLICITED_RESPONSE_TIMEOUT      100
#define TDLS_WAIT_RESPONSE_TIMEOUT                500
#define TDLS_DEFAULT_BASECHANNEL_TIMEOUT          105
#define TDLS_WAKE_UP_TIMEOUT                      300
#define TDLS_SEND_REQUEST_RETRY_TIME              100
#define TDLS_SEND_RESPONSE_RETRY_TIME             100
#define TDLS_FAKESLEEP_RETRY_TIME                 100

/*
 * Higher-precision timers used for actually switching the radio channel.
 * Values in microseconds to keep the same unit specified in the 802.11z
 * specification for the Channel Switch Timing element and avoid run-time
 * multiplications and divisions by 1000
 */
#define TDLS_DEFAULT_HW_SWITCH_TIME_USEC          8000
#define TDLS_DEFAULT_HW_SWITCH_TIMEOUT_USEC       16000
#define TDLS_CHANNEL_SWITCH_LATENCY_USEC          10000
#define TDLS_MAX_OFFCHANNEL_TIME_USEC             1000000
#define TDLS_BASECHANNEL_OFFCHANNEL_DELAY_USEC    3000
#define TDLS_DTIM_TIMER_EVENT_ID                  77

#define SELECT_VAP(x)  ieee80211tdls_chan_sx_select_vap(x)

// Notification event names
static const char* tdls_link_monitor_notification_type_name[] = {
    /* IEEE80211_TDLS_LINK_MONITOR_NONE           */ "NONE",
    /* IEEE80211_TDLS_LINK_MONITOR_STARTED        */ "STARTED",
    /* IEEE80211_TDLS_LINK_MONITOR_STOPPED        */ "STOPPED",
    /* IEEE80211_TDLS_LINK_MONITOR_INIT           */ "INIT",
    /* IEEE80211_TDLS_LINK_MONITOR_NORMAL         */ "NORMAL",
    /* IEEE80211_TDLS_LINK_MONITOR_ERROR_DETECTED */ "ERROR_DETECTED",
    /* IEEE80211_TDLS_LINK_MONITOR_ERROR_RECOVERY */ "ERROR_RECOVERY",
    /* IEEE80211_TDLS_LINK_MONITOR_UNREACHABLE    */ "UNREACHABLE",
};

// Notification reason names
static const char* tdls_link_monitor_notification_reason_name[] = {
    /* IEEE80211_TDLS_LINK_MONITOR_REASON_NONE         */ "NONE",
    /* IEEE80211_TDLS_LINK_MONITOR_REASON_START        */ "START",
    /* IEEE80211_TDLS_LINK_MONITOR_REASON_STOP         */ "STOP",
    /* IEEE80211_TDLS_LINK_MONITOR_REASON_TX_OK        */ "TX_OK",
    /* IEEE80211_TDLS_LINK_MONITOR_REASON_TX_ERROR     */ "TX_ERROR",
    /* IEEE80211_TDLS_LINK_MONITOR_REASON_RX_OK        */ "RX_OK",
    /* IEEE80211_TDLS_LINK_MONITOR_REASON_ERROR_COUNT  */ "ERROR_COUNT",
    /* IEEE80211_TDLS_LINK_MONITOR_REASON_TIMEOUT      */ "TIMEOUT",
    /* IEEE80211_TDLS_LINK_MONITOR_REASON_BASE_CHANNEL */ "BASE_CHANNEL",
    /* IEEE80211_TDLS_LINK_MONITOR_REASON_OFFCHANNEL   */ "OFFCHANNEL",
};

const char*
ieee80211_tdls_link_monitor_notification_type_name(ieee80211_tdls_link_monitor_notification_type notification_type)
{
    ASSERT(notification_type < ARRAY_LENGTH(tdls_link_monitor_notification_type_name));
    ASSERT(ARRAY_LENGTH(tdls_link_monitor_notification_type_name) == IEEE80211_TDLS_LINK_MONITOR_NOTIFICATION_TYPE_COUNT);

    return (tdls_link_monitor_notification_type_name[notification_type]);
}

const char*
ieee80211_tdls_link_monitor_notification_reason_name(ieee80211_tdls_link_monitor_notification_reason reason)
{
    ASSERT(reason < ARRAY_LENGTH(tdls_link_monitor_notification_reason_name));
    ASSERT(ARRAY_LENGTH(tdls_link_monitor_notification_reason_name) == IEEE80211_TDLS_LINK_MONITOR_NOTIFICATION_REASON_COUNT);

    return (tdls_link_monitor_notification_reason_name[reason]);
}


/*
 * Internal event data
 */
struct ieee80211tdls_chan_sx_sm_event {
    ieee80211_vap_t             vap;
    struct ieee80211_node       *ni_tdls;
    struct ieee80211_node       *ni_bss_node;
    u_int8_t                    wme_info_uapsd;
    u_int16_t                   cx_resp_status;
    u_int32_t                   cx_switch_time;
    u_int32_t                   cx_switch_timeout;
    struct ieee80211_channel    *cx_offchannel;
};
typedef struct ieee80211tdls_chan_sx_sm_event ieee80211tdls_chan_sx_sm_event_t;

/*
 * State machine states
 */
typedef enum {
    IEEE80211_TDLS_CHAN_SX_STATE_IDLE = 0,
    IEEE80211_TDLS_CHAN_SX_STATE_INIT_SUSPENDING,
    IEEE80211_TDLS_CHAN_SX_STATE_INIT_SEND_REQUEST,
    IEEE80211_TDLS_CHAN_SX_STATE_INIT_WAIT_RESPONSE,
    IEEE80211_TDLS_CHAN_SX_STATE_RESP_SUSPENDING,
    IEEE80211_TDLS_CHAN_SX_STATE_RESP_SEND_RESPONSE,
    IEEE80211_TDLS_CHAN_SX_STATE_RESP_REJECT_REQUEST,
    IEEE80211_TDLS_CHAN_SX_STATE_SWITCH_TIME,
    IEEE80211_TDLS_CHAN_SX_STATE_PROBING,
    IEEE80211_TDLS_CHAN_SX_STATE_OFFCHANNEL,
    IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE,
    IEEE80211_TDLS_CHAN_SX_STATE_RESUMING,
    IEEE80211_TDLS_CHAN_SX_STATE_BASE_CHANNEL,
} ieee80211_tdls_chan_sx_state;

/*
 * State machine events
 */
typedef enum {
    IEEE80211_TDLS_CHAN_SX_EVENT_NONE = 0,
    IEEE80211_TDLS_CHAN_SX_EVENT_INITIATE,
    IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_COMPLETED,
    IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_FAILED,
    IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_TIMEOUT,
    IEEE80211_TDLS_CHAN_SX_EVENT_WAKEUP_COMPLETED,
    IEEE80211_TDLS_CHAN_SX_EVENT_WAKEUP_FAILED,
    IEEE80211_TDLS_CHAN_SX_EVENT_WAKEUP_TIMEOUT,
    IEEE80211_TDLS_CHAN_SX_EVENT_TX_REQUEST_OK,
    IEEE80211_TDLS_CHAN_SX_EVENT_TX_REQUEST_FAILED,
    IEEE80211_TDLS_CHAN_SX_EVENT_TX_REQUEST_TIMEOUT,
    IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_OK,
    IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_FAILED,
    IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_TIMEOUT,
    IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_OK,
    IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_FAILED,
    IEEE80211_TDLS_CHAN_SX_EVENT_REQUEST_RECEIVED,
    IEEE80211_TDLS_CHAN_SX_EVENT_RESPONSE_RECEIVED,
    IEEE80211_TDLS_CHAN_SX_EVENT_DATA_TRAFFIC_RECEIVED,
    IEEE80211_TDLS_CHAN_SX_EVENT_BEACON_RECEIVED,
    IEEE80211_TDLS_CHAN_SX_EVENT_DTIM_TIMER,
    IEEE80211_TDLS_CHAN_SX_EVENT_CANCEL,
    IEEE80211_TDLS_CHAN_SX_EVENT_SWITCH_TIME,
    IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_SEND_REQUEST,
    IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_SEND_RESPONSE,
    IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_FAKESLEEP,
    IEEE80211_TDLS_CHAN_SX_EVENT_WAIT_RESPONSE_TIMEOUT,
    IEEE80211_TDLS_CHAN_SX_EVENT_SWITCH_TIMEOUT,
    IEEE80211_TDLS_CHAN_SX_EVENT_PROBE_TIMEOUT,
    IEEE80211_TDLS_CHAN_SX_EVENT_BASECHANNEL_TIMEOUT,
    IEEE80211_TDLS_CHAN_SX_EVENT_OFFCHANNEL_DELAY,
    IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR,
    IEEE80211_TDLS_CHAN_SX_EVENT_POOR_LINK_QUALITY,
} ieee80211tdls_chan_sx_event;

static const char* tdls_chan_sx_event_name[] = {
    /* IEEE80211_TDLS_CHAN_SX_EVENT_NONE                      */ "NONE",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_INITIATE                  */ "INITIATE",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_COMPLETED       */ "FAKESLEEP_COMPLETED",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_FAILED          */ "FAKESLEEP_FAILED",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_TIMEOUT         */ "FAKESLEEP_TIMEOUT",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_WAKEUP_COMPLETED          */ "WAKEUP_COMPLETED",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_WAKEUP_FAILED             */ "WAKEUP_FAILED",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_WAKEUP_TIMEOUT            */ "WAKEUP_TIMEOUT",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_TX_REQUEST_OK             */ "TX_REQUEST_OK",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_TX_REQUEST_FAILED         */ "TX_REQUEST_FAILED",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_TX_REQUEST_TIMEOUT        */ "TX_REQUEST_TIMEOUT",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_OK            */ "TX_RESPONSE_OK",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_FAILED        */ "TX_RESPONSE_FAILED",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_TIMEOUT       */ "TX_RESPONSE_TIMEOUT",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_OK         */ "TX_QOSNULLDATA_OK",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_FAILED     */ "TX_QOSNULLDATA_FAILED",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_REQUEST_RECEIVED          */ "REQUEST_RECEIVED",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_RESPONSE_RECEIVED         */ "RESPONSE_RECEIVED",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_DATA_TRAFFIC_RECEIVED     */ "DATA_TRAFFIC_RECEIVED",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_BEACON_RECEIVED           */ "BEACON_RECEIVED",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_DTIM_TIMER                */ "DTIM_TIMER",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_CANCEL                    */ "CANCEL",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_SWITCH_TIME               */ "SWITCH_TIME",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_SEND_REQUEST        */ "RETRY_SEND_REQUEST",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_SEND_RESPONSE       */ "RETRY_SEND_RESPONSE",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_FAKESLEEP           */ "RETRY_FAKESLEEP",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_WAIT_RESPONSE_TIMEOUT     */ "WAIT_RESPONSE_TIMEOUT",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_SWITCH_TIMEOUT            */ "SWITCH_TIMEOUT",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_PROBE_TIMEOUT             */ "PROBE_TIMEOUT",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_BASECHANNEL_TIMEOUT       */ "BASECHANNEL_TIMEOUT",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_OFFCHANNEL_DELAY          */ "OFFCHANNEL_DELAY",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR                 */ "API_ERROR",
    /* IEEE80211_TDLS_CHAN_SX_EVENT_POOR_LINK_QUALITY         */ "POOR_LINK_QUALITY",
};

// Notification event names
static const char* tdls_chan_switch_notification_type_name[] = {
    /* IEEE80211_TDLS_CHAN_SWITCH_STARTED          */ "STARTED",
    /* IEEE80211_TDLS_CHAN_SWITCH_COMPLETED        */ "COMPLETED",
    /* IEEE80211_TDLS_CHAN_SWITCH_OFFCHANNEL       */ "OFFCHANNEL",
    /* IEEE80211_TDLS_CHAN_SWITCH_BASE_CHANNEL     */ "BASE_CHANNEL",
    /* IEEE80211_TDLS_CHAN_SWITCH_DELAYED_TX_ERROR */ "DELAYED_TX_ERROR",
};

// Notification reason names
static const char* tdls_chan_switch_notification_reason_name[] = {
    /* IEEE80211_TDLS_CHAN_SWITCH_REASON_NONE                                */ "NONE",
    /* IEEE80211_TDLS_CHAN_SWITCH_REASON_SWITCH_TIMEOUT                      */ "SWITCH_TIMEOUT",
    /* IEEE80211_TDLS_CHAN_SWITCH_REASON_OFFCHANNEL_TIMEOUT                  */ "OFFCHANNEL_TIMEOUT",
    /* IEEE80211_TDLS_CHAN_SWITCH_REASON_TRANSMIT_REQUEST_ERROR              */ "TRANSMIT_REQUEST_ERROR",
    /* IEEE80211_TDLS_CHAN_SWITCH_REASON_WAIT_RESPONSE_TIMEOUT               */ "WAIT_RESPONSE_TIMEOUT",
    /* IEEE80211_TDLS_CHAN_SWITCH_REASON_TX_RESPONSE_FAILED                  */ "TX_RESPONSE_FAILED",
    /* IEEE80211_TDLS_CHAN_SWITCH_REASON_UNSOLICITED_CHANNEL_SWITCH_RESPONSE */ "UNSOLICITED_CHANNEL_SWITCH_RESPONSE",
    /* IEEE80211_TDLS_CHAN_SWITCH_REASON_DTIM_TIMER                          */ "DTIM_TIMER",
    /* IEEE80211_TDLS_CHAN_SWITCH_REASON_LINK_MONITOR                        */ "LINK_MONITOR",
    /* IEEE80211_TDLS_CHAN_SWITCH_REASON_CANCELED                            */ "CANCELED",
    /* IEEE80211_TDLS_CHAN_SWITCH_REASON_POWER_SAVE_ERROR                    */ "POWER_SAVE_ERROR",
    /* IEEE80211_TDLS_CHAN_SWITCH_REASON_INTERNAL_ERROR                      */ "INTERNAL_ERROR",
    /* IEEE80211_TDLS_CHAN_SWITCH_REASON_POOR_LINK_QUALITY                   */ "POOR_LINK_QUALITY",
};

// Channel Switch result names
static const char* tdls_chan_switch_result_name[] = {
    /* IEEE80211_TDLS_CHAN_SWITCH_RESULT_NONE                  */ "NONE",
    /* IEEE80211_TDLS_CHAN_SWITCH_RESULT_RESET                 */ "RESET",
    /* IEEE80211_TDLS_CHAN_SWITCH_RESULT_FAKESLEEP_SUCCESS     */ "FAKESLEEP_SUCCESS",
    /* IEEE80211_TDLS_CHAN_SWITCH_RESULT_FAKESLEEP_ERROR       */ "FAKESLEEP_ERROR",
    /* IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_REQUEST_SUCCESS  */ "SEND_REQUEST_SUCCESS",
    /* IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_REQUEST_ERROR    */ "SEND_REQUEST_ERROR",
    /* IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_RESPONSE_SUCCESS */ "SEND_RESPONSE_SUCCESS",
    /* IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_RESPONSE_ERROR   */ "SEND_RESPONSE_ERROR",
    /* IEEE80211_TDLS_CHAN_SWITCH_RESULT_PROBE_SUCCESS         */ "SEND_PROBE_SUCCESS",
    /* IEEE80211_TDLS_CHAN_SWITCH_RESULT_PROBE_ERROR           */ "SEND_PROBE_ERROR",
};

#if TDLS_USE_CHSX_VAP

/* WLAN event handlers */
static wlan_event_handler_table wlanEventHandlerTable = {
    NULL,        /* wlan_receive */
    NULL,        /* wlan_receive_filter_80211 */
    NULL,        /* wlan_receive_monitor_80211 */
    NULL,        /* wlan_dev_xmit_queue */
    NULL,        /* wlan_vap_xmit_queue */
    NULL,        /* wlan_xmit_update_status */
};

/* MLME handlers */
static wlan_mlme_event_handler_table wlanMLMEHandlerTable = {
    /* MLME confirmation handler */
    NULL,                      /* mlme_join_complete_infra */
    NULL,                      /* mlme_join_complete_adhoc */
    NULL,                      /* mlme_auth_complete */
    NULL,                      /* mlme_assoc_req */
    NULL,                      /* mlme_assoc_complete */
    NULL,                      /* mlme_reassoc_complete */
    NULL,                      /* mlme_deauth_complete */
    NULL,                      /* mlme_disassoc_complete */

    /* MLME indication handler */
    NULL,                      /* mlme_auth_indication */
    NULL,                      /* mlme_deauth_indication */
    NULL,                      /* mlme_assoc_indication */
    NULL,                      /* mlme_reassoc_indication */
    NULL,                      /* mlme_disassoc_indication */
    NULL,                      /* mlme_ibss_merge_start_indication */
    NULL,                      /* mlme_ibss_merge_completion_indication */
};

static wlan_misc_event_handler_table wlanMiscHandlerTable = {
    NULL,                       /* wlan_channel_change */
    NULL,                       /* wlan_country_changed */
    NULL,                       /* wlan_linkspeed */
    NULL,                       /* wlan_michael_failure_indication */
    NULL,                       /* wlan_replay_failure_indication */
    NULL,                       /* wlan_beacon_miss_indication */
    NULL,                       /* wlan_device_error_indication */
    NULL,                       /* wlan_sta_clonemac_indication */
    NULL,                       /* wlan_sta_scan_entry_update */
    NULL,                       /* wlan_ap_stopped */
#if ATH_SUPPORT_WAPI
    NULL,                       /* wlan_sta_rekey_indication */
#endif
};

static void
TDLSHandleScanEvent(wlan_if_t vaphandle,
                     ieee80211_scan_event *event,
                     void *arg)
{
    return;
}
#endif

/*************************** POLICY FUNCTIONS ***************************/
/* Counters */

/*
 * For WFA's TDLS Certification, don't stop channel switching due to excessive
 * errors
 */
#define TDLS_MAX_WFA_CERT_FAKESLEEP_RETRIES          1
#define TDLS_MAX_WFA_CERT_FAKESLEEP_FAILURES         0
#define TDLS_MAX_WFA_CERT_REQUEST_RETRIES            1
#define TDLS_MAX_WFA_CERT_REQUEST_FAILURES           0
#define TDLS_MAX_WFA_CERT_RESPONSE_RETRIES           1
#define TDLS_MAX_WFA_CERT_RESPONSE_FAILURES          0
#define TDLS_MAX_WFA_CERT_OFFCHANNEL_FAILURES        0

#define TDLS_MAX_DEFAULT_FAKESLEEP_RETRIES           1
#define TDLS_MAX_DEFAULT_FAKESLEEP_FAILURES          5
#define TDLS_MAX_DEFAULT_REQUEST_RETRIES             1
#define TDLS_MAX_DEFAULT_REQUEST_FAILURES            5
#define TDLS_MAX_DEFAULT_RESPONSE_RETRIES            1
#define TDLS_MAX_DEFAULT_RESPONSE_FAILURES           5
#define TDLS_MAX_DEFAULT_OFFCHANNEL_FAILURES         10

static int
tdls_policy_query_wfa_cert(chan_switch_t        *cs,
                           tdls_policy_value_t  value_type,
                           void                 *value_data)
{
    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: value_type=%d tdls_control=%d reject=%d interrupt=%d\n",
        __func__, value_type,
        cs->w_tdls->tdls_vap->iv_tdls_channel_switch_control,
        cs->policy.reject_request,
        cs->policy.interrupt_request);

    switch (value_type) {
    case TDLS_POLICY_VALUE_IGNORE_CHAN_SWITCH_REQUEST:
        return (cs->w_tdls->tdls_vap->iv_tdls_channel_switch_control == IEEE80211_TDLS_CHANNEL_SWITCH_CONTROL_OFF);

    case TDLS_POLICY_VALUE_REJECT_CHAN_SWITCH_REQUEST:
        return cs->policy.reject_request;

    case TDLS_POLICY_VALUE_INTERRUPT_CHAN_SWITCH_REQUEST:
        return cs->policy.interrupt_request;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: invalid query %d\n",
            __func__, value_type);
        break;
    }

    return false;
}

static int
tdls_policy_set_wfa_cert(chan_switch_t        *cs,
                         tdls_policy_value_t  value_type,
                         int                  value,
                         void                 *value_data)
{
    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: value_type=%d tdls_control=%d reject=%d interrupt=%d\n",
        __func__, value_type,
        cs->w_tdls->tdls_vap->iv_tdls_channel_switch_control,
        cs->policy.reject_request,
        cs->policy.interrupt_request);

    switch (value_type) {
    case TDLS_POLICY_VALUE_IGNORE_CHAN_SWITCH_REQUEST:
        cs->w_tdls->tdls_vap->iv_tdls_channel_switch_control = value;
        break;

    case TDLS_POLICY_VALUE_REJECT_CHAN_SWITCH_REQUEST:
        cs->policy.reject_request = value;
        break;

    case TDLS_POLICY_VALUE_INTERRUPT_CHAN_SWITCH_REQUEST:
        cs->policy.interrupt_request = value;
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: invalid query %d\n",
            __func__, value_type);
        break;
    }

    return false;
}

static int
tdls_policy_query_default(chan_switch_t        *cs,
                          tdls_policy_value_t  value_type,
                          void                 *value_data)
{
    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: value_type=%d tdls_control=%d reject=%d interrupt=%d\n",
        __func__, value_type,
        cs->w_tdls->tdls_vap->iv_tdls_channel_switch_control,
        cs->policy.reject_request,
        cs->policy.interrupt_request);

    switch (value_type) {
    case TDLS_POLICY_VALUE_IGNORE_CHAN_SWITCH_REQUEST:
        return (cs->w_tdls->tdls_vap->iv_tdls_channel_switch_control == IEEE80211_TDLS_CHANNEL_SWITCH_CONTROL_OFF);

    case TDLS_POLICY_VALUE_REJECT_CHAN_SWITCH_REQUEST:
        return cs->policy.reject_request;

    case TDLS_POLICY_VALUE_INTERRUPT_CHAN_SWITCH_REQUEST:
        return cs->policy.interrupt_request;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: invalid query %d\n",
            __func__, value_type);
        break;
    }

    return false;
}

static int
tdls_policy_set_default(chan_switch_t        *cs,
                        tdls_policy_value_t  value_type,
                        int                  value,
                        void                 *value_data)
{
    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: value_type=%d tdls_control=%d reject=%d interrupt=%d\n",
        __func__, value_type,
        cs->w_tdls->tdls_vap->iv_tdls_channel_switch_control,
        cs->policy.reject_request,
        cs->policy.interrupt_request);

    switch (value_type) {
    case TDLS_POLICY_VALUE_IGNORE_CHAN_SWITCH_REQUEST:
        cs->w_tdls->tdls_vap->iv_tdls_channel_switch_control = value;
        break;

    case TDLS_POLICY_VALUE_REJECT_CHAN_SWITCH_REQUEST:
        cs->policy.reject_request = value;
        break;

    case TDLS_POLICY_VALUE_INTERRUPT_CHAN_SWITCH_REQUEST:
        cs->policy.interrupt_request = value;
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: invalid query %d\n",
            __func__, value_type);
        break;
    }

    return false;
}

void
ieee80211_channel_switch_policy_initialize(chan_switch_t *cs,
                                           int           policy)
{
    chan_switch_stats_t    *stats = &(cs->policy.stats);

    switch (policy) {
    case TDLS_POLICY_DEFAULT:
        cs->policy_query_function = tdls_policy_query_default;
        cs->policy_set_function   = tdls_policy_set_default;

        stats->fakesleep_failure_count      = 0;
        stats->fakesleep_failure_threshold  = TDLS_MAX_DEFAULT_FAKESLEEP_FAILURES;
        stats->fakesleep_retry_threshold    = TDLS_MAX_DEFAULT_FAKESLEEP_RETRIES;

        stats->request_failure_count        = 0;
        stats->request_failure_threshold    = TDLS_MAX_DEFAULT_REQUEST_FAILURES;
        stats->request_retry_threshold      = TDLS_MAX_DEFAULT_REQUEST_RETRIES;

        stats->response_failure_count       = 0;
        stats->response_failure_threshold   = TDLS_MAX_DEFAULT_RESPONSE_FAILURES;
        stats->response_retry_threshold     = TDLS_MAX_DEFAULT_RESPONSE_RETRIES;

        stats->offchannel_failure_count     = 0;
        stats->offchannel_failure_threshold = TDLS_MAX_DEFAULT_OFFCHANNEL_FAILURES;
        break;

    case TDLS_POLICY_WFA_CERT:
        cs->policy_query_function = tdls_policy_query_wfa_cert;
        cs->policy_set_function   = tdls_policy_set_wfa_cert;

        stats->fakesleep_failure_count      = 0;
        stats->fakesleep_failure_threshold  = TDLS_MAX_WFA_CERT_FAKESLEEP_FAILURES;
        stats->fakesleep_retry_threshold    = TDLS_MAX_WFA_CERT_FAKESLEEP_RETRIES;

        stats->request_failure_count        = 0;
        stats->request_failure_threshold    = TDLS_MAX_WFA_CERT_REQUEST_FAILURES;
        stats->request_retry_threshold      = TDLS_MAX_WFA_CERT_REQUEST_RETRIES;

        stats->response_failure_count       = 0;
        stats->response_failure_threshold   = TDLS_MAX_WFA_CERT_RESPONSE_FAILURES;
        stats->response_retry_threshold     = TDLS_MAX_WFA_CERT_RESPONSE_RETRIES;

        stats->offchannel_failure_count     = 0;
        stats->offchannel_failure_threshold = TDLS_MAX_WFA_CERT_OFFCHANNEL_FAILURES;
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ERROR! Invalid policy\n",
            __func__);
        ASSERT(0);
        break;
    }
}

int
tdls_policy_query(chan_switch_t        *cs,
                  tdls_policy_value_t  value_type,
                  void                 *value_data)
{
    if (cs->policy_query_function != NULL) {
        return cs->policy_query_function(cs, value_type, value_data);
    }

    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ERROR! No query function\n",
        __func__);

    return -EINVAL;
}

int
tdls_policy_set(chan_switch_t        *cs,
                tdls_policy_value_t  value_type,
                int                  value,
                void                 *value_data)
{
    if (cs->policy_set_function != NULL) {
        return cs->policy_set_function(cs, value_type, value, value_data);
    }

    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ERROR! No set function\n",
        __func__);

    return -EINVAL;
}

int
ieee80211_tdls_set_channel_switch_control(chan_switch_t *cs,
                                          int           control_flags)
{
    cs->policy.reject_request    =
        ((control_flags & IEEE80211_TDLS_CHAN_SWITCH_CONTROL_REJECT_REQUEST) != 0);

    cs->policy.interrupt_request =
        ((control_flags & IEEE80211_TDLS_CHAN_SWITCH_CONTROL_INTERRUPT_REQUEST) != 0);

    return 0;
}
/*************************** POLICY FUNCTIONS ***************************/

static int tdls_cs_sm_create(chan_switch_t *cs);
static int tdls_cs_sm_delete(chan_switch_t *cs);

static const char*
ieee80211_tdls_chan_switch_event_name(ieee80211tdls_chan_sx_event event)
{
    ASSERT(event < IEEE80211_N(tdls_chan_sx_event_name));

    return (tdls_chan_sx_event_name[event]);
}

static const char*
ieee80211_tdls_chan_switch_result_name(ieee80211_tdls_chan_switch_result_t result)
{
    ASSERT(result < ARRAY_LENGTH(tdls_chan_switch_result_name));
    ASSERT(ARRAY_LENGTH(tdls_chan_switch_result_name) == IEEE80211_TDLS_CHAN_SWITCH_RESULT_COUNT);

    return (tdls_chan_switch_result_name[result]);
}

static void
ieee80211_tdls_chan_switch_txrx_event_handler(
    ieee80211_vap_t          vap,
    ieee80211_vap_txrx_event *event,
    void                     *arg)
{
    chan_switch_t    *cs = (chan_switch_t *) arg;

    if ((vap == NULL) || (event == NULL) || (arg == NULL)) {
        printf("%s: Skip event=%d vap=%p event=%p arg=%p\n",
            __func__,
            (event != NULL) ? event->type : -999,
            vap,
            event,
            arg);

        return;
    }

    // Beacon notification must be from our AP
    if (event->type == IEEE80211_VAP_INPUT_EVENT_BEACON) {
        if ((cs->monitor_beacon) && (event->ni == cs->w_tdls->tdls_vap->iv_bss)) {
            cs->monitor_beacon = false;

            ieee80211_sm_dispatch(cs->hsm_handle,
                                  IEEE80211_TDLS_CHAN_SX_EVENT_BEACON_RECEIVED,
                                  0,
                                  NULL);
        }
    }

    // These other notifications must be from frames received from our peer
    if ((event->ni == NULL) || (event->ni != cs->w_tdls->tdls_ni)) {
        // Filter this event; It doesn't belong to this handler
        return;
    }

    switch (event->type) {
    case IEEE80211_VAP_INPUT_EVENT_UCAST:
        if (cs->monitor_data) {
            cs->monitor_data = false;

            ieee80211_sm_dispatch(cs->hsm_handle,
                                  IEEE80211_TDLS_CHAN_SX_EVENT_DATA_TRAFFIC_RECEIVED,
                                  0,
                                  NULL);
        }
        break;

    case IEEE80211_VAP_OUTPUT_EVENT_COMPLETE_TX:
        break;

    case IEEE80211_VAP_OUTPUT_EVENT_TX_SUCCESS:
        break;

    case IEEE80211_VAP_OUTPUT_EVENT_TX_ERROR:
        break;

    default:
        break;
    }
}

static int
ieee80211_tdls_report_chan_switch_result(chan_switch_t                       *cs,
                                         ieee80211_tdls_chan_switch_result_t result)
{
    int                    stop_channel_switch = false;
    chan_switch_stats_t    *stats = &(cs->policy.stats);

    switch (result) {
    case IEEE80211_TDLS_CHAN_SWITCH_RESULT_NONE:
        break;

    case IEEE80211_TDLS_CHAN_SWITCH_RESULT_RESET:
        stats->fakesleep_failure_count  = 0;
        stats->request_failure_count    = 0;
        stats->offchannel_failure_count = 0;
        break;

    case IEEE80211_TDLS_CHAN_SWITCH_RESULT_FAKESLEEP_SUCCESS:
        stats->fakesleep_failure_count = 0;
        break;

    case IEEE80211_TDLS_CHAN_SWITCH_RESULT_FAKESLEEP_ERROR:
        /*
         * Keep track of total failures in both the fakesleep step and the
         * overall off-channel operation, so we can stop trying if too many
         * consecutive errors are detected.
         */
        stats->fakesleep_failure_count++;
        stats->offchannel_failure_count++;

        if ((stats->fakesleep_failure_threshold > 0) &&
            (stats->fakesleep_failure_count > stats->fakesleep_failure_threshold)) {
            // stop channel switching due to too many errors
            stop_channel_switch = true;
        }
        if ((stats->offchannel_failure_threshold > 0) &&
            (stats->offchannel_failure_count > stats->offchannel_failure_threshold)) {
            // stop channel switching due to too many errors
            stop_channel_switch = true;
        }
        break;

    case IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_REQUEST_SUCCESS:
        stats->request_failure_count = 0;
        break;

    case IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_REQUEST_ERROR:
        /*
         * Keep track of total failures in both the Channel Switch Request
         * step and the overall off-channel operation, so we can stop
         * trying if too many consecutive errors are detected.
         */
        stats->request_failure_count++;
        stats->offchannel_failure_count++;

        if ((stats->request_failure_threshold > 0) &&
            (stats->request_failure_count > stats->request_failure_threshold)) {
            // stop channel switching due to too many errors
            stop_channel_switch = true;
        }
        if ((stats->offchannel_failure_threshold > 0) &&
            (stats->offchannel_failure_count > stats->offchannel_failure_threshold)) {
            // stop channel switching due to too many errors
            stop_channel_switch = true;
        }
        break;

    case IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_RESPONSE_SUCCESS:
        stats->response_failure_count = 0;
        break;

    case IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_RESPONSE_ERROR:
        /*
         *  Not technically necessary to stop channel switching due to too
         *  many channel switch response errors since channel switch
         *  operation is being driven by peer.
         */
        stats->response_failure_count++;

        if ((stats->response_failure_threshold > 0) &&
            (stats->response_failure_count > stats->response_failure_threshold)) {
            // stop channel switching due to too many errors
            stop_channel_switch = true;
        }
        break;

    case IEEE80211_TDLS_CHAN_SWITCH_RESULT_PROBE_SUCCESS:
        stats->offchannel_failure_count = 0;
        break;

    case IEEE80211_TDLS_CHAN_SWITCH_RESULT_PROBE_ERROR:
        stats->offchannel_failure_count++;
        if ((stats->offchannel_failure_threshold > 0) &&
            (stats->offchannel_failure_count > stats->offchannel_failure_threshold)) {
            // stop channel switching due to too many errors
            stop_channel_switch = true;
        }
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_ANY,
            "%s: invalid result=%s (%d)\n",
            __func__,
            ieee80211_tdls_chan_switch_result_name(result),
            result);
        ASSERT(0);
        break;
    }

    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_ANY,
        "%s: result=%s (%d) failure_counts=%d,%d,%d stop_channel_switch=%d\n",
        __func__,
        ieee80211_tdls_chan_switch_result_name(result),
        result,
        stats->fakesleep_failure_count,
        stats->request_failure_count,
        stats->offchannel_failure_count,
        stop_channel_switch);

    return stop_channel_switch;
}

#if IEEE80211_TDLS_USE_TSF_TIMER

static void
ieee80211tdls_high_definition_timer_handler(tdls_high_definition_timer_t h_tsftimer,
                                            int                          event_id,
                                            void                         *arg,
                                            u_int32_t                    tsf)
{
    chan_switch_t    *cs = arg;
    int              event;

    /*
     * Clear pending event before dispatching timeout event to avoid race conditions.
     * Using a spinlock would be even better.
     */
    event = cs->pending_high_definition_event;
    cs->pending_high_definition_event = IEEE80211_TDLS_CHAN_SX_EVENT_NONE;

    ieee80211_sm_dispatch(cs->hsm_handle,
                          event,
                          0,
                          NULL);
}

static int
ieee80211tdls_initialize_high_definition_timer(struct ieee80211com *ic, chan_switch_t *cs)
{
    cs->pending_high_definition_event = IEEE80211_TDLS_CHAN_SX_EVENT_NONE;

    /*
     * No need to handle changes in the TSF timer.
     * We use the TSF timer only when offchannel, when we cannot receive
     * frames from the AP, so the TSF cannot change.
     */
    cs->high_definition_timer = ieee80211_tsftimer_alloc(ic->ic_tsf_timer,
                                                         0,
                                                         ieee80211tdls_high_definition_timer_handler,
                                                         TDLS_DTIM_TIMER_EVENT_ID,
                                                         cs,
                                                         NULL);

    return (cs->high_definition_timer != NULL);
}

static int
ieee80211tdls_schedule_high_definition_event(chan_switch_t               *cs,
                                             int                         time_usec,
                                             ieee80211tdls_chan_sx_event event)
{
    u_int64_t    cur_tsf64;

    cur_tsf64 = cs->w_tdls->tdls_vap->iv_ic->ic_get_TSF64(cs->w_tdls->tdls_vap->iv_ic);

    /*
     * cs->pending_high_definition_event != IEEE80211_TDLS_CHAN_SX_EVENT_NONE
     * indicates the high-definition timer has been set but has not expired
     * yet. Trying to start the timer again indicates a flaw in the state
     * machine's behavior.
     */
    if (cs->pending_high_definition_event != IEEE80211_TDLS_CHAN_SX_EVENT_NONE) {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: Timer already set. Event: current=%d new=%d\n",
            __func__, cs->pending_high_definition_event, event);
    }

    cs->pending_high_definition_event = event;

    return ieee80211_tsftimer_start(cs->high_definition_timer, cur_tsf64 + time_usec, 0);
}

static void
ieee80211tdls_free_high_definition_timer(chan_switch_t *cs)
{
    if (cs->high_definition_timer != NULL) {
        ieee80211_tsftimer_free(cs->high_definition_timer, true);
        cs->high_definition_timer = NULL;
    }
}

static void
ieee80211tdls_cancel_high_definition_timer(chan_switch_t *cs)
{
    ieee80211_tsftimer_stop(cs->high_definition_timer);
    cs->pending_high_definition_event = IEEE80211_TDLS_CHAN_SX_EVENT_NONE;
}

#else    // IEEE80211_TDLS_USE_TSF_TIMER

static OS_TIMER_FUNC(ieee80211tdls_high_definition_timer_handler)
{
    chan_switch_t    *cs;
    int              event;

    OS_GET_TIMER_ARG(cs, chan_switch_t *);

    /*
     * Clear pending event before dispatching timeout event to avoid race conditions.
     * Using a spinlock would be even better.
     */
    event = cs->pending_high_definition_event;
    cs->pending_high_definition_event = IEEE80211_TDLS_CHAN_SX_EVENT_NONE;

    ieee80211_sm_dispatch(cs->hsm_handle,
                          event,
                          0,
                          NULL);
}

static int
ieee80211tdls_initialize_high_definition_timer(struct ieee80211com *ic, chan_switch_t *cs)
{
    cs->pending_high_definition_event = IEEE80211_TDLS_CHAN_SX_EVENT_NONE;

    OS_INIT_TIMER(ic->ic_osdev, &(cs->high_definition_timer), ieee80211tdls_high_definition_timer_handler, (void *) cs);

    return EOK;
}

static int
ieee80211tdls_schedule_high_definition_event(chan_switch_t               *cs,
                                             int                         time_usec,
                                             ieee80211tdls_chan_sx_event event)
{
    /*
     * cs->pending_high_definition_event != IEEE80211_TDLS_CHAN_SX_EVENT_NONE
     * indicates the high-definition timer has been set but has not expired
     * yet. Trying to start the timer again indicates a flaw in the state
     * machine's behavior.
     */
    if (cs->pending_high_definition_event != IEEE80211_TDLS_CHAN_SX_EVENT_NONE) {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: Timer already set. Event: current=%d new=%d\n",
            __func__, cs->pending_high_definition_event, event);
    }

    cs->pending_high_definition_event = event;

    OS_SET_TIMER(&cs->high_definition_timer, time_usec / 1000);

    /*
     * DO NOT use return code from OS_SET_TIMER
     * Each OS uses a different value to indicate success.
     */
    return EOK;
}

static void
ieee80211tdls_free_high_definition_timer(chan_switch_t *cs)
{
    OS_FREE_TIMER(&(cs->high_definition_timer));
}

static void
ieee80211tdls_cancel_high_definition_timer(chan_switch_t *cs)
{
    OS_CANCEL_TIMER(&(cs->high_definition_timer));
    cs->pending_high_definition_event = IEEE80211_TDLS_CHAN_SX_EVENT_NONE;
}

#endif    // IEEE80211_TDLS_USE_TSF_TIMER

/*
 * All times in this function are in usec to reduce the number of divisions.
 */
static int32_t
ieee80211tdls_calculate_max_offchannel_time_usec(struct ieee80211vap   *vap,
                                                 struct ieee80211_node *bss_node,
                                                 u_int32_t             switch_time)
{
    u_int64_t    cur_tsf64;
    u_int32_t    dtim_countdown;
    u_int32_t    beacon_interval;
    u_int32_t    beacon_offset;
    int32_t      max_offchannel_time;

    cur_tsf64           = bss_node->ni_vap->iv_ic->ic_get_TSF64(bss_node->ni_vap->iv_ic);
    beacon_interval     = IEEE80211_TU_TO_USEC(bss_node->ni_intval);
    dtim_countdown      = (bss_node->ni_dtim_count > 0) ?
                              (beacon_interval * bss_node->ni_dtim_count) :
                              (beacon_interval * bss_node->ni_dtim_period);
    beacon_offset       = OS_MOD64_TBTT_OFFSET(cur_tsf64, beacon_interval);
    max_offchannel_time = ((int32_t) (dtim_countdown - beacon_offset)) - ((int32_t) switch_time + TDLS_CHANNEL_SWITCH_LATENCY_USEC);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: dtim: count=%d period=%d countdown=%u beacon_interval=%u beacon_offset=%u cur_tsf64=%u max_offchannel_time=%d\n",
        __func__,
        bss_node->ni_dtim_count,
        bss_node->ni_dtim_period,
        dtim_countdown,
        beacon_interval,
        beacon_offset,
        (unsigned int) cur_tsf64,
        max_offchannel_time);

    if (max_offchannel_time > TDLS_MAX_OFFCHANNEL_TIME_USEC) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_TDLS, "%s: Adjusting max_offchannel_time:%d=>%d\n",
            __func__, max_offchannel_time, TDLS_MAX_OFFCHANNEL_TIME_USEC);

        max_offchannel_time = TDLS_MAX_OFFCHANNEL_TIME_USEC;
    }

    return max_offchannel_time;
}

static int
ieee80211tdls_channel_transition(chan_switch_t *cs, struct ieee80211_channel *new_channel)
{
    int          rc = EOK;
    u_int64_t    start_time, elapsed_time;

    start_time = cs->w_tdls->tdls_vap->iv_ic->ic_get_TSF64(cs->w_tdls->tdls_vap->iv_ic);

    ieee80211_node_saveq_flush(cs->w_tdls->tdls_ni);
    ieee80211_node_saveq_flush(cs->w_tdls->tdls_vap->iv_bss);

    ieee80211node_pause(cs->w_tdls->tdls_ni);
    ieee80211node_pause(cs->w_tdls->tdls_vap->iv_bss);

    if (ieee80211_set_channel(cs->w_tdls->tdls_ni->ni_ic, new_channel) != EOK) {
        rc = -EINVAL;
    }

    ieee80211node_unpause(cs->w_tdls->tdls_vap->iv_bss);
    ieee80211node_unpause(cs->w_tdls->tdls_ni);

    // Calculate elapsed_time in usec.
    elapsed_time = cs->w_tdls->tdls_vap->iv_ic->ic_get_TSF64(cs->w_tdls->tdls_vap->iv_ic) - start_time;
    /*************** TRACE ONLY ***************/
    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: elapsed_time=%d\n",
        __func__, elapsed_time);
    /*************** TRACE ONLY ***************/

    return rc;
}

int
ieee80211tdls_chan_switch_sm_start(
    struct ieee80211_node    *ni,
    wlan_tdls_sm_t           w_tdls,
    struct ieee80211_channel *offchannel
    )
{
    ieee80211tdls_chan_sx_sm_event_t    chan_sx_event;
    chan_switch_t                       *cs = &w_tdls->chan_switch;

    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: in_progress=%d cancel=%d stop_channel_switch=%d\n",
        __func__, cs->in_progress, cs->cancelation_requested, cs->stop_channel_switch);

    if (cs->in_progress) {
        return -EPERM;
    }

    /* Flush any messages from message queue and reset HSM to IDLE state. */
    ieee80211_sm_reset(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_IDLE, NULL);

    OS_MEMZERO(&chan_sx_event, sizeof(chan_sx_event));

    chan_sx_event.cx_offchannel = offchannel;

    ieee80211_sm_dispatch(cs->hsm_handle,
                          IEEE80211_TDLS_CHAN_SX_EVENT_INITIATE,
                          sizeof(chan_sx_event),
                          &(chan_sx_event));
    return 0;
}

int
ieee80211tdls_chan_switch_sm_stop(
    struct ieee80211_node    *ni,
    wlan_tdls_sm_t           w_tdls
    )
{
    chan_switch_t    *cs = &w_tdls->chan_switch;

    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: in_progress=%d cancel=%d stop_channel_switch=%d\n",
        __func__, cs->in_progress, cs->cancelation_requested, cs->stop_channel_switch);

    if (cs->in_progress) {
        cs->cancelation_requested = true;
        cs->stop_channel_switch   = true;

        ieee80211_sm_dispatch(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_EVENT_CANCEL, 0, NULL);
    }

    return 0;
}

const char*
ieee80211_tdls_chan_switch_notification_type_name(ieee80211_tdls_chan_switch_notification_type_t notification_type)
{
    ASSERT(notification_type < ARRAY_LENGTH(tdls_chan_switch_notification_type_name));
    ASSERT(ARRAY_LENGTH(tdls_chan_switch_notification_type_name) == IEEE80211_TDLS_CHAN_SWITCH_NOTIFICATION_TYPE_COUNT);

    return (tdls_chan_switch_notification_type_name[notification_type]);
}

const char*
ieee80211_tdls_chan_switch_notification_reason_name(ieee80211_tdls_chan_switch_notification_reason_t reason)
{
    ASSERT(reason < ARRAY_LENGTH(tdls_chan_switch_notification_reason_name));
    ASSERT(ARRAY_LENGTH(tdls_chan_switch_notification_reason_name) == IEEE80211_TDLS_CHAN_SWITCH_NOTIFICATION_REASON_COUNT);

    return (tdls_chan_switch_notification_reason_name[reason]);
}

int
ieee80211_tdls_chan_switch_register_notification_handler(
    chan_switch_t                       *cs,
    ieee80211_tdls_notification_handler handler,
    void                                *arg)
{
    ASSERT(cs->w_tdls->tdls_vap->iv_ic->ic_tdls != NULL);
    if (cs->w_tdls->tdls_vap->iv_ic->ic_tdls == NULL) {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ic_tdls not set handler=%p arg=%p\n",
            __func__, handler, arg);

        return -ENOSPC;
    }

    return ieee80211_tdls_register_notification_handler(&(cs->notifier), handler, arg);
}

int
ieee80211_tdls_chan_switch_unregister_notification_handler(
    chan_switch_t                       *cs,
    ieee80211_tdls_notification_handler handler,
    void                                *arg)
{
    ASSERT(cs->w_tdls->tdls_vap->iv_ic->ic_tdls != NULL);
    if (cs->w_tdls->tdls_vap->iv_ic->ic_tdls == NULL) {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ic_tdls not set handler=%p arg=%p\n",
            __func__, handler, arg);

        return -ENOSPC;
    }

    return ieee80211_tdls_unregister_notification_handler(&(cs->notifier), handler, arg);
}

static void
chan_switch_post_event(chan_switch_t                                    *cs,
                       ieee80211_tdls_chan_switch_notification_type_t   notification_type,
                       ieee80211_tdls_chan_switch_notification_reason_t reason)
{
    ieee80211_tdls_chan_switch_notification_data_t    chan_switch_notification_data;

    if (notification_type == IEEE80211_TDLS_CHAN_SWITCH_BASE_CHANNEL) {
        /* When returning to the base channel, a reason must be provided. */
        if (notification_type == IEEE80211_TDLS_CHAN_SWITCH_REASON_NONE) {
            IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: reason not specified\n",
                __func__);
        }
    }

    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: notification_type=%d reason=%d\n",
        __func__, notification_type, reason);

    /* Populate event fields in event structure */
    OS_MEMZERO(&chan_switch_notification_data, sizeof(chan_switch_notification_data));
    chan_switch_notification_data.reason = reason;

    notifier_post_event(
         cs->w_tdls->tdls_ni,
         &(cs->notifier),
        notification_type,
        sizeof(chan_switch_notification_data),
        &(chan_switch_notification_data));
}

int
ieee80211tdls_chan_switch_sm_data_recv(
    struct ieee80211_node *ni,
    wlan_tdls_sm_t        w_tdls
    )
{
    chan_switch_t    *cs = &w_tdls->chan_switch;

    if (cs->monitor_data) {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s:\n",
            __func__);

        ieee80211_sm_dispatch(cs->hsm_handle,
                              IEEE80211_TDLS_CHAN_SX_EVENT_DATA_TRAFFIC_RECEIVED,
                              0,
                              NULL);
    }

    return 0;
}

int
ieee80211tdls_chan_switch_sm_req_recv(
    struct ieee80211_node                             *ni,
    wlan_tdls_sm_t                                    w_tdls,
    struct ieee80211_tdls_chan_switch_frame           *channel_switch_frame,
    struct ieee80211_tdls_chan_switch_timing_ie       *channel_switch_timing_ie,
    struct ieee80211_tdls_chan_switch_sec_chan_offset *sec_chan_offset
    )
{
    ieee80211tdls_chan_sx_sm_event_t    chan_sx_event;
    chan_switch_t                       *cs = &w_tdls->chan_switch;
    enum ieee80211_phymode              mode;
    struct ieee80211_channel            *offchannel;

    /* Checking of channel switch control already done by the caller */

    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: tgt_channel=%d switch_time=%d/%d sec_chan_offset=%d\n",
        __func__,
        channel_switch_frame->tgt_channel,
        channel_switch_timing_ie->switch_time,
        channel_switch_timing_ie->switch_timeout,
        (sec_chan_offset != NULL) ? sec_chan_offset->sec_chan_offset : -1);

    if (channel_switch_frame->tgt_channel < 27) {
        mode = IEEE80211_MODE_11NG_HT20;
        if (sec_chan_offset) {
            if (sec_chan_offset->sec_chan_offset == IEEE80211_SECONDARY_CHANNEL_ABOVE) {
                mode = IEEE80211_MODE_11NG_HT40PLUS;
            }
            else if (sec_chan_offset->sec_chan_offset == IEEE80211_SECONDARY_CHANNEL_BELOW) {
                mode = IEEE80211_MODE_11NG_HT40MINUS;
            }
        }
    }
    else {
        mode = IEEE80211_MODE_11NA_HT20;
        if (sec_chan_offset) {
            if (sec_chan_offset->sec_chan_offset == IEEE80211_SECONDARY_CHANNEL_ABOVE) {
                mode = IEEE80211_MODE_11NA_HT40PLUS;
            }
            else if (sec_chan_offset->sec_chan_offset == IEEE80211_SECONDARY_CHANNEL_BELOW) {
                mode = IEEE80211_MODE_11NA_HT40MINUS;
            }
        }
    }
    offchannel = ieee80211_find_dot11_channel(ni->ni_ic, channel_switch_frame->tgt_channel, mode);
    if (offchannel == NULL) {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: cannot find channel %d mode=%d\n",
            __func__,
            (channel_switch_frame  != NULL) ? channel_switch_frame->tgt_channel     : -1,
            mode);
    }
    if (offchannel == NULL) {
        if (channel_switch_frame->tgt_channel < 27) {
            mode = IEEE80211_MODE_11G;
        }
        else {
            mode = IEEE80211_MODE_11A;
        }
        offchannel = ieee80211_find_dot11_channel(ni->ni_ic, channel_switch_frame->tgt_channel, mode);
        if (offchannel == NULL) {
            IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: cannot find channel %d mode=%d\n",
                __func__,
                (channel_switch_frame  != NULL) ? channel_switch_frame->tgt_channel     : -1,
                mode);
        }
    }

    // Populate event fields with channel switch request information
    OS_MEMZERO(&chan_sx_event, sizeof(chan_sx_event));
    chan_sx_event.cx_offchannel = ieee80211_find_dot11_channel(ni->ni_ic, channel_switch_frame->tgt_channel, mode);
    chan_sx_event.cx_switch_time    = channel_switch_timing_ie->switch_time;
    chan_sx_event.cx_switch_timeout = channel_switch_timing_ie->switch_timeout;
    //XXX: Reject request if IE is missing

    ieee80211_sm_dispatch(cs->hsm_handle,
                          IEEE80211_TDLS_CHAN_SX_EVENT_REQUEST_RECEIVED,
                          sizeof(chan_sx_event),
                          &(chan_sx_event));

    return 0;
}

int
ieee80211tdls_chan_switch_sm_resp_recv(
    struct ieee80211_node                       *ni,
    wlan_tdls_sm_t                              w_tdls,
    u_int16_t                                   status,
    struct ieee80211_tdls_chan_switch_timing_ie *channel_switch_timing_ie
    )
{
    ieee80211tdls_chan_sx_sm_event_t    chan_sx_event;
    chan_switch_t                       *cs = &w_tdls->chan_switch;

    if (! cs->in_progress) {
        return -EPERM;
    }

    OS_MEMZERO(&chan_sx_event, sizeof(chan_sx_event));
    chan_sx_event.cx_resp_status    = status;
    chan_sx_event.cx_switch_time    = channel_switch_timing_ie->switch_time;
    chan_sx_event.cx_switch_timeout = channel_switch_timing_ie->switch_timeout;
    ieee80211_sm_dispatch(cs->hsm_handle,
                          IEEE80211_TDLS_CHAN_SX_EVENT_RESPONSE_RECEIVED,
                          sizeof(chan_sx_event),
                          &(chan_sx_event));

    return 0;
}

static OS_TIMER_FUNC(ieee80211tdls_chan_sx_standard_event_timer_handler)
{
    chan_switch_t    *cs;
    int              event;

    OS_GET_TIMER_ARG(cs, chan_switch_t *);

    /*
     * Clear pending event before dispatching timeout event to avoid race conditions.
     * Using a spinlock would be even better.
     */
    event = cs->pending_standard_event;
    cs->pending_standard_event = IEEE80211_TDLS_CHAN_SX_EVENT_NONE;
    //TODO subrat comemnted below for testing
    ieee80211_sm_dispatch(cs->hsm_handle,
                          event,
                          0,
                          NULL);
}

static void
ieee80211tdls_schedule_timed_event(chan_switch_t               *cs,
                                   int                         msec,
                                   ieee80211tdls_chan_sx_event event)
{
    /*
     * cs->pending_standard_event != IEEE80211_TDLS_CHAN_SX_EVENT_NONE indicates
     * the generic timer has been set but has not expired yet. Trying to start
     * the timer again indicates a flaw in the state machine's behavior.
     */
    if (cs->pending_standard_event != IEEE80211_TDLS_CHAN_SX_EVENT_NONE) {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: Timer already set. Event: current=%d new=%d\n",
            __func__, cs->pending_standard_event, event);
    }

    cs->pending_standard_event = event;
    OS_SET_TIMER(&cs->standard_event_timer, msec);
}

static void ieee80211tdls_clear_timed_event(chan_switch_t *cs)
{
    OS_CANCEL_TIMER(&cs->standard_event_timer);
    cs->pending_standard_event = IEEE80211_TDLS_CHAN_SX_EVENT_NONE;
}

void ieee80211tdls_sta_power_event_handler(ieee80211_sta_power_t powersta, ieee80211_sta_power_event *event, void *arg)
{
    chan_switch_t                  *cs = (chan_switch_t *) arg;
    ieee80211tdls_chan_sx_event    tdls_event = IEEE80211_TDLS_CHAN_SX_EVENT_NONE;

    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s event=%d status=%d\n",
        __func__,
        ieee80211_sm_get_current_state_name(cs->hsm_handle),
        event->type,
        event->status);

    switch (event->type) {
    case IEEE80211_POWER_STA_PAUSE_COMPLETE:
        tdls_event = ((event->status == IEEE80211_POWER_STA_STATUS_SUCCESS) ?
                         IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_COMPLETED :
                         IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_FAILED);

        break;

    case IEEE80211_POWER_STA_UNPAUSE_COMPLETE:
        tdls_event = ((event->status == IEEE80211_POWER_STA_STATUS_SUCCESS) ?
                         IEEE80211_TDLS_CHAN_SX_EVENT_WAKEUP_COMPLETED :
                         IEEE80211_TDLS_CHAN_SX_EVENT_WAKEUP_FAILED);
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s event=%d status=%d not handled\n",
            __func__,
            ieee80211_sm_get_current_state_name(cs->hsm_handle),
            event->type,
            event->status);
        break;
    }

    if (tdls_event != IEEE80211_TDLS_CHAN_SX_EVENT_NONE) {
        ieee80211_sm_dispatch(cs->hsm_handle, tdls_event, 0, NULL);
    }
}

static void
ieee80211tdls_cx_req_completion_handler(wlan_if_t             vap,
                                        wbuf_t                wbuf,
                                        void*                 arg,
                                        u_int8_t              *dst_addr,
                                        u_int8_t              *src_addr,
                                        u_int8_t              *bssid,
                                        ieee80211_xmit_status *ts)
{
    chan_switch_t                  *cs = (chan_switch_t *) arg;
    ieee80211tdls_chan_sx_event    event = IEEE80211_TDLS_CHAN_SX_EVENT_NONE;

    event = (ts->ts_flags == 0) ?
        IEEE80211_TDLS_CHAN_SX_EVENT_TX_REQUEST_OK :
        IEEE80211_TDLS_CHAN_SX_EVENT_TX_REQUEST_FAILED;

    ieee80211_sm_dispatch(cs->hsm_handle, event, 0, NULL);
}

static void
ieee80211tdls_cx_resp_completion_handler(wlan_if_t             vap,
                                         wbuf_t                wbuf,
                                         void*                 arg,
                                         u_int8_t              *dst_addr,
                                         u_int8_t              *src_addr,
                                         u_int8_t              *bssid,
                                         ieee80211_xmit_status *ts)
{
    chan_switch_t                  *cs = (chan_switch_t *) arg;
    ieee80211tdls_chan_sx_event    event = IEEE80211_TDLS_CHAN_SX_EVENT_NONE;
    event = (ts->ts_flags == 0) ?
        IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_OK :
        IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_FAILED;

    ieee80211_sm_dispatch(cs->hsm_handle, event, 0, NULL);
}

static void
ieee80211tdls_cx_qosnulldata_completion_handler(wlan_if_t             vap,
                                                wbuf_t                wbuf,
                                                void*                 arg,
                                                u_int8_t              *dst_addr,
                                                u_int8_t              *src_addr,
                                                u_int8_t              *bssid,
                                                ieee80211_xmit_status *ts)
{
    chan_switch_t                  *cs = (chan_switch_t *) arg;
    ieee80211tdls_chan_sx_event    event = IEEE80211_TDLS_CHAN_SX_EVENT_NONE;

    event = (ts->ts_flags == 0) ?
        IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_OK :
        IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_FAILED;

    ieee80211_sm_dispatch(cs->hsm_handle, event, 0, NULL);
}

void
ieee80211tdls_chan_switch_handle_link_monitor_notification(
    struct ieee80211_node *ni,
    u_int16_t             notification_type,
    u_int16_t             notification_data_len,
    void                  *notification_data,
    void                  *arg)
{
    chan_switch_t                                    *cs = arg;
    //subrat link_monitor
    ieee80211_tdls_link_monitor_notification_data    *link_monitor_notification_data = notification_data;

    if (ni == NULL) {
        printk("%s: Error: ni == NULL\n", __func__);

        return;
    }
#if 1 //subrat link_monitor
    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_ANY,
            "%s: event=%s (%d) reason=%s (%d)\n",
            __func__,
            ieee80211_tdls_link_monitor_notification_type_name(notification_type),
            notification_type,
            (link_monitor_notification_data != NULL) ? ieee80211_tdls_link_monitor_notification_reason_name(link_monitor_notification_data->reason) : "???",
            (link_monitor_notification_data != NULL) ? link_monitor_notification_data->reason : -1);
#endif
    switch (notification_type) {
    case IEEE80211_TDLS_LINK_MONITOR_ERROR_RECOVERY:
        /*
         * Too many errors while we're off-channel; we want to return to base
         * channel to try to re-synch with our peer.
         * There's not much we can do if we're not off-channel, and the Link
         * Monitor will probably reset the Direct Link.
         */
        if (cs->is_offchannel) {
            ieee80211_sm_dispatch(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_EVENT_POOR_LINK_QUALITY, 0, NULL);
        }
        break;

    default:
        /* No need to handle other events yet */
        break;
    }
}

int
ieee80211tdls_chan_switch_sm_create(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls)
{
    struct ieee80211com    *ic;
    chan_switch_t          *cs;
    wlan_dev_t             devhandle;
#if TDLS_USE_CHSX_VAP
    wlan_if_t              vap = NULL;
#endif
    int                    rc = 0;

    if ((ni == NULL) || (w_tdls == NULL)) {
        printk("%s: ERROR ni=%p w_tdls=%p\n",
            __func__, ni, w_tdls);

        return -ENOENT;
    }

    ic = ni->ni_ic;
    if (! (ieee80211_ic_off_channel_support_is_set(ic))) {
        IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_TDLS, "%s: macaddr=%s FAIL: off_channel_support=0\n",
            __func__, ether_sprintf(ni->ni_macaddr));

        return -EPERM;
    }

    IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_TDLS, "%s: macaddr=%s\n",
        __func__, ether_sprintf(ni->ni_macaddr));

    devhandle = ic;
    cs = &w_tdls->chan_switch;

    OS_MEMZERO(cs, sizeof(chan_switch_t));

#if TDLS_USE_CHSX_VAP
    /*
     * create station VAP associated with TDLS connection.
     */
    vap = wlan_vap_create(devhandle,
                          IEEE80211_M_STA,
                          DEF_VAP_SCAN_PRI_MAP_OPMODE_STA_TDLS,
                          IEEE80211_TDLS_VAP | IEEE80211_NO_STABEACONS | IEEE80211_CLONE_BSSID,
                          NULL);
    if (vap == NULL) {
        ieee80211com_note(devhandle, "%s: Failed to create vap for TDLS object\n",__func__);
        return -ENOMEM;
    }
    //subrt TODO
    /* Set VAP and IC debug levels */
    wlan_set_param(vap, 1/*IEEE80211_MSG_FLAGS*/, wlan_get_param(vap,/* IEEE80211_MSG_FLAGS*/1) | IEEE80211_MSG_TDLS);

    /* Register event handler with VAP */
    wlan_vap_set_registered_handle(vap, (os_if_t)cs);
    wlan_vap_register_event_handlers(vap, &wlanEventHandlerTable);

    /* Register MLME handler with VAP */
    wlan_vap_register_mlme_event_handlers(vap,
                                          (os_if_t)cs,
                                          &wlanMLMEHandlerTable);

    /* Register Miscellaneous handler with VAP */
    wlan_vap_register_misc_event_handlers(vap,
                                          (os_if_t)cs,
                                          &wlanMiscHandlerTable);

    /* Register event handler with Scanner */
        printk("\n ** scan handler %s : %d -- %p***\n",__func__,__LINE__,TDLSHandleScanEvent);
    rc = wlan_scan_register_event_handler(vap, TDLSHandleScanEvent, (void *) cs);
    if (rc != EOK) {
        ieee80211com_note(devhandle, "%s: wlan_scan_register_event_handler() failed\n", __func__);
    }

    cs->chsx_vap                      = vap;
#endif

    cs->w_tdls                        = w_tdls;
    cs->devhandle                     = devhandle;
    OS_INIT_TIMER(ic->ic_osdev, &(cs->standard_event_timer), ieee80211tdls_chan_sx_standard_event_timer_handler, (void *) cs);
    cs->in_progress                   = false;
    cs->pending_standard_event        = IEEE80211_TDLS_CHAN_SX_EVENT_NONE;
    cs->pending_high_definition_event = IEEE80211_TDLS_CHAN_SX_EVENT_NONE;
    cs->fakesleep_timeout             = TDLS_FAKESLEEP_TIMEOUT;
    cs->is_initiator                  = false;
    cs->cancelation_requested         = false;
    cs->stop_channel_switch           = false;
    cs->is_paused                     = false;
    cs->is_offchannel                 = false;
    cs->is_protocol_error             = false;
    cs->offchannel                    = NULL;
    cs->hw_switch_time                = TDLS_DEFAULT_HW_SWITCH_TIME_USEC;
    cs->hw_switch_timeout             = TDLS_DEFAULT_HW_SWITCH_TIMEOUT_USEC;
    cs->agreed_switch_time            = cs->hw_switch_time;
    cs->agreed_switch_timeout         = cs->hw_switch_timeout;
    cs->retry_count                   = 0;
    /*
     * Initialize channel switching policy to comply with WiFi Alliance's
     * certification tests.
     */
    ieee80211_channel_switch_policy_initialize(cs, TDLS_POLICY_WFA_CERT);

    rc = ieee80211_vap_txrx_register_event_handler(cs->w_tdls->tdls_vap,
            ieee80211_tdls_chan_switch_txrx_event_handler,
            (void *) cs,
            IEEE80211_VAP_OUTPUT_EVENT_TX_ERROR   |
            IEEE80211_VAP_OUTPUT_EVENT_TX_SUCCESS |
            IEEE80211_VAP_INPUT_EVENT_UCAST       |
            IEEE80211_VAP_OUTPUT_EVENT_COMPLETE_TX |  //subrat
            IEEE80211_VAP_INPUT_EVENT_BEACON);

    if (rc != EOK) {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS,
                             "%s: ieee80211_vap_txrx_register_event_handler failure! rc=%lu\n",
                             __func__, rc);
    }

    ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_RESET);

    ieee80211tdls_initialize_high_definition_timer(ic, cs);

    /* Initialize notifier */
    cs->notifier.num_handlers = 0;
    spin_lock_init(&(cs->notifier.lock));

    rc = tdls_cs_sm_create(cs);

    return EOK;
}

int
ieee80211tdls_chan_switch_sm_delete(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls)
{
    struct ieee80211com    *ic;
    chan_switch_t          *cs;
    int                    rc;

    if ((ni == NULL) || (w_tdls == NULL)) {
        printk("%s: ERROR ni=%p w_tdls=%p\n",
            __func__, ni, w_tdls);

        return -ENOENT;
    }

    IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_TDLS, "%s: macaddr=%s\n",
        __func__, ether_sprintf(ni->ni_macaddr));

    ic = ni->ni_ic;
    cs = &w_tdls->chan_switch;

    rc = ieee80211_vap_txrx_unregister_event_handler(cs->w_tdls->tdls_vap,
            ieee80211_tdls_chan_switch_txrx_event_handler, (void *) cs);

    if (rc != EOK) {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS,
                             "%s: ieee80211_vap_txrx_unregister_event_handler failure! rc=%lu\n",
                             __func__, rc);
    }

    /* clear all event handlers */
    spin_lock(&(cs->notifier.lock));
    cs->notifier.num_handlers = 0;
    spin_unlock(&(cs->notifier.lock));
    spin_lock_destroy(&(cs->notifier.lock));

    OS_CANCEL_TIMER(&(cs->standard_event_timer));
    ieee80211tdls_cancel_high_definition_timer(cs);

    OS_FREE_TIMER(&(cs->standard_event_timer));
    ieee80211tdls_free_high_definition_timer(cs);

    rc = tdls_cs_sm_delete(cs);

#if TDLS_USE_CHSX_VAP
    rc = wlan_scan_unregister_event_handler(cs->chsx_vap, TDLSHandleScanEvent, (void *) cs);
    if (rc != EOK) {
        ieee80211com_note(ic, "%s: wlan_scan_unregister_event_handler() failed\n",__func__);
    }

    wlan_vap_delete(cs->chsx_vap);
#endif

    return EOK;
}

static void
ieee80211tdls_chan_sx_sm_debug_print (void *ctx, const char *fmt,...)
{
    char             tmp_buf[256];
    va_list          ap;
    chan_switch_t    *cs = (chan_switch_t *) ctx;

    va_start(ap, fmt);
    vsnprintf (tmp_buf,256, fmt, ap);
    va_end(ap);
    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s", tmp_buf);
}

static int
ieee80211tdls_pause_vap(chan_switch_t *cs)
{
    int    rc;

    //XXX: Cannot call ieee80211_sta_power_pause otherwise TDLS frames are not transmitted
#if USE_VAP_PAUSE
    rc = ieee80211_sta_power_pause(cs->w_tdls->tdls_vap->iv_pwrsave_sta, cs->fakesleep_timeout);
    if (rc == 0) {
        cs->is_paused = true;
    }
    else {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ieee80211_sta_power_pause failed rc=%d\n",
            __func__, rc);
    }
#else
    rc = ieee80211_send_nulldata(cs->w_tdls->tdls_vap->iv_bss, true);
    if (rc == 0) {
        cs->is_paused = true;
    }
    else {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ieee80211_send_nulldata failed rc=%d\n",
            __func__, rc);
    }
    ieee80211_sm_dispatch(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_COMPLETED, 0, NULL);

    // We're not handling errors yet
    rc = 0;
#endif

    return rc;
}

static int
ieee80211tdls_unpause_vap(chan_switch_t *cs)
{
    int    rc;

    /*
     * XXX: since we did not call ieee80211_sta_power_pause,
     * don't call ieee80211_sta_power_unpause either.
     */
#if USE_VAP_PAUSE
    rc = ieee80211_sta_power_unpause(cs->w_tdls->tdls_vap->iv_pwrsave_sta);
    if (rc == 0) {
        cs->is_paused = false;
    }
    else {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ieee80211_sta_power_pause failed rc=%d\n",
            __func__, rc);
    }

    return rc;
#else
    rc = ieee80211_send_nulldata(cs->w_tdls->tdls_vap->iv_bss, false);
    if (rc == 0) {
        cs->is_paused = false;
    }
    else {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ieee80211_send_nulldata failed rc=%d\n",
            __func__, rc);
    }

    ieee80211_sm_dispatch(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_EVENT_WAKEUP_COMPLETED, 0, NULL);

    rc = 0;
#endif

    return rc;
}

/*
 * different state-related functions.
 */
/*
 * State IDLE
 */
static void
ieee80211tdls_chan_sx_state_idle_entry(void *ctx)
{
    chan_switch_t    *cs = ctx;

    ieee80211tdls_clear_timed_event(cs);

    /* Sanity Check */
    if (cs->is_offchannel || cs->is_paused) {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ERROR: inconsistent flags is_paused=%d is_offchannel=%d\n",
            __func__, cs->is_paused, cs->is_offchannel);
    }

    chan_switch_post_event(cs,
                           IEEE80211_TDLS_CHAN_SWITCH_COMPLETED,
                           IEEE80211_TDLS_CHAN_SWITCH_REASON_NONE);

    cs->in_progress           = false;
    cs->is_initiator          = false;
    cs->cancelation_requested = false;
    cs->stop_channel_switch   = false;
    cs->monitor_data          = false;
    cs->monitor_beacon        = false;
    cs->is_offchannel         = false;
    cs->is_paused             = false;
    cs->is_protocol_error     = false;
    /*
     * Do not reset hw_switch_time/timeout.
     */
    cs->agreed_switch_time    = cs->hw_switch_time;
    cs->agreed_switch_timeout = cs->hw_switch_timeout;
    cs->retry_count           = 0;

    ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_RESET);
}

static bool
ieee80211tdls_chan_sx_state_idle_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data)
{
    chan_switch_t    *cs = ctx;
    int32_t          max_offchannel_time;

    if (cs == NULL) {
        return false;
    }

    switch (event) {
    case IEEE80211_TDLS_CHAN_SX_EVENT_INITIATE:
        {
            ieee80211tdls_chan_sx_sm_event_t    *chan_sx_event = event_data;

            cs->in_progress  = true;
            cs->offchannel   = chan_sx_event->cx_offchannel;
            cs->is_initiator = true;

            /*
             * Calculate time necessary to go offchannel and return to base channel.
             * We don't have our peer's values yet, so use ours as a base.
             */
            max_offchannel_time = ieee80211tdls_calculate_max_offchannel_time_usec(cs->w_tdls->tdls_vap,
                                                                                   cs->w_tdls->tdls_vap->iv_bss,
                                                                                   2 * (u_int32_t) cs->hw_switch_time);

            IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: INITIATE: dtim: count=%d period=%d max_offchannel_time=%d\n",
                __func__,
                cs->w_tdls->tdls_vap->iv_bss->ni_dtim_count,
                cs->w_tdls->tdls_vap->iv_bss->ni_dtim_period,
                max_offchannel_time);

            if (max_offchannel_time < 0) {
                /*
                 * ToDo: Postpone going off-channel until after DTIM.
                 */

                IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: INITIATOR: Not enough time to go offchannel before DTIM; offchannel_time=%d\n",
                    __func__,
                    max_offchannel_time);
            }

            chan_switch_post_event(cs,
                                   IEEE80211_TDLS_CHAN_SWITCH_STARTED,
                                   IEEE80211_TDLS_CHAN_SWITCH_REASON_NONE);

            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_INIT_SUSPENDING);
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_REQUEST_RECEIVED:
        {
            ieee80211tdls_chan_sx_sm_event_t    *chan_sx_event = event_data;

            cs->is_initiator = false;
            cs->in_progress = true;

            cs->offchannel = chan_sx_event->cx_offchannel;

            if (cs->agreed_switch_time < chan_sx_event->cx_switch_time) {
                cs->agreed_switch_time = chan_sx_event->cx_switch_time;
            }
            if (cs->agreed_switch_timeout < chan_sx_event->cx_switch_timeout) {
                cs->agreed_switch_timeout = chan_sx_event->cx_switch_timeout;
            }

            /*
             * Calculate time necessary to go offchannel and return to base channel.
             * The agreed upon switch time indicates how long it takes for both
             * peers to reach the offchannel. We then add the time it takes us
             * to return to the home channel.
             */
            max_offchannel_time = ieee80211tdls_calculate_max_offchannel_time_usec(
                    cs->w_tdls->tdls_vap,
                    cs->w_tdls->tdls_vap->iv_bss,
                    (u_int32_t) cs->hw_switch_time + (u_int32_t) cs->agreed_switch_time);

            IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s received Channel Switch Request offchannel=%d switch_time=%d/%d control=%d tdlscaps=%08X\n",
                __func__,
                ieee80211_sm_get_current_state_name(cs->hsm_handle),
                (chan_sx_event->cx_offchannel != NULL) ? ieee80211_channel_ieee(chan_sx_event->cx_offchannel) : -1,
                chan_sx_event->cx_switch_time,
                chan_sx_event->cx_switch_timeout,
                cs->w_tdls->tdls_vap->iv_tdls_channel_switch_control,
                cs->w_tdls->tdls_vap->iv_bss->ni_tdls_caps);

            if (tdls_policy_query(cs, TDLS_POLICY_VALUE_IGNORE_CHAN_SWITCH_REQUEST, NULL)) {
                // Ignore Channel Switch Request if VAP is configured to do so
                return true;
            }

            if (max_offchannel_time < 0) {
                /*
                 * ToDo: Postpone going off-channel until after DTIM?
                 */

                IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: RESPONDER: Not enough time to go offchannel before DTIM; offchannel_time=%d\n",
                    __func__,
                    max_offchannel_time);
            }

            chan_switch_post_event(cs,
                                   IEEE80211_TDLS_CHAN_SWITCH_STARTED,
                                   IEEE80211_TDLS_CHAN_SWITCH_REASON_NONE);

            if (tdls_policy_query(cs, TDLS_POLICY_VALUE_REJECT_CHAN_SWITCH_REQUEST, NULL)) {
                // Reject Channel Switch Request
                ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESP_REJECT_REQUEST);

                return true;
            }

            ieee80211tdls_clear_timed_event(cs);

            /*
             * Accept Channel Switch Request if:
             *     - Target channel is specified;
             *     - Channel Switch Timing values are consistent;
             *     - AP is not advertising that Channel Switching is prohibited
             */

            if ((cs->offchannel != NULL) &&
                (cs->agreed_switch_timeout > cs->agreed_switch_time) &&
                (! (cs->w_tdls->tdls_vap->iv_bss->ni_tdls_caps & IEEE80211_TDLS_CHAN_SX_PROHIBIT))) {
                ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESP_SUSPENDING);
            }
            else {
                ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESP_REJECT_REQUEST);
            }
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_OK:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_TIMEOUT:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_OK:
    case IEEE80211_TDLS_CHAN_SX_EVENT_DATA_TRAFFIC_RECEIVED:
        /*
         * Ignore these events.
         * If the TDLS peer transmits an unsolicited channel switch response
         * at the same time we do, there's a chance we'll receive the peer's
         * frame before our frame completes. In this case we transition to
         * RESUMING state and the TX_RESPONSE completion event is received in
         * the new state.
         */
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s ignored event=%s - took too long rx/tx frame\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_FAILED:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_FAILED:
        /*
         * Events posted almost simultaneously with reception of unsolicited
         * channel switch response may be handled in the next state.
         * In case of TX error, notify clients (namely the Link Monitor module)
         * that error must be ignored.
         */
        chan_switch_post_event(cs,
                               IEEE80211_TDLS_CHAN_SWITCH_DELAYED_TX_ERROR,
                               IEEE80211_TDLS_CHAN_SWITCH_REASON_NONE);

        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s ignored event=%s - took too long rx/tx frame\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s event=%s not handled\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));

        return false;
    }

    return true;
}

static void
ieee80211tdls_chan_sx_init_suspending_entry(void *ctx)
{
    chan_switch_t    *cs = ctx;
    int              rc;

    // Reset number of retries
    cs->retry_count = 0;

    // Request the PowerManagement module to enter Network Sleep
    rc = ieee80211tdls_pause_vap(cs);

    if (rc == 0) {
        // Set timeout for completion of pause request
        ieee80211tdls_schedule_timed_event(cs, TDLS_FAKESLEEP_TIMEOUT, IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_TIMEOUT);
    }
    else {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ieee80211_sta_power_pause failed rc=%d\n",
            __func__, rc);

        // Indicate error
        ieee80211_sm_dispatch(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR, 0, NULL);
    }
}

static bool
ieee80211tdls_chan_sx_init_suspending_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data)
{
    chan_switch_t    *cs = ctx;

    switch (event) {
    case IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_COMPLETED:
        ieee80211tdls_clear_timed_event(cs);

        // Fakesleep suceeded
        cs->retry_count = 0;
        ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_FAKESLEEP_SUCCESS);

        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_INIT_SEND_REQUEST);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR:
        ieee80211tdls_clear_timed_event(cs);

        /*
         * Increment number of attempts to send fakesleep.
         * If exceeded retry limit, fail this attempt to go off-channel and
         * return to BASE_CHANNEL state to try again later.
         */
        cs->retry_count++;
        if (cs->retry_count > cs->policy.stats.fakesleep_retry_threshold) {
            /*
             * Report result of this channel switch attempt.
             * Policy module decides whether to continue attempting
             * channel switch.
             */
            cs->stop_channel_switch =
                ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_FAKESLEEP_ERROR);

            /*
             * We did not enter network sleep or transmit a channel switch request,
             * so just abort channel switching and return to IDLE.
             */
            cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_INTERNAL_ERROR;
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_BASE_CHANNEL);
        }
        else {
            // Retry after a short delay
            ieee80211tdls_schedule_timed_event(cs, TDLS_FAKESLEEP_RETRY_TIME, IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_FAKESLEEP);
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_FAILED:
    case IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_TIMEOUT:
        ieee80211tdls_clear_timed_event(cs);

        /*
         * Increment number of attempts to send fakesleep.
         * If exceeded retry limit, fail this attempt to go off-channel and
         * return to BASE_CHANNEL state to try again later.
         */
        cs->retry_count++;
        if (cs->retry_count > cs->policy.stats.fakesleep_retry_threshold) {
            /*
             * Report result of this channel switch attempt.
             * Policy module decides whether to continue attempting
             * channel switch.
             */
            cs->stop_channel_switch =
                ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_FAKESLEEP_ERROR);

            /*
             * We did not enter network sleep or transmit a channel switch request,
             * so just abort channel switching and return to IDLE.
             */
            cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_POWER_SAVE_ERROR;
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_BASE_CHANNEL);
        }
        else {
            // Retry after a short delay
            ieee80211tdls_schedule_timed_event(cs, TDLS_FAKESLEEP_RETRY_TIME, IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_FAKESLEEP);
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_FAKESLEEP:
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_INIT_SUSPENDING);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_CANCEL:
        ieee80211tdls_clear_timed_event(cs);

        /*
         * We are currently trying to enter Network Sleep, so we must notify
         * the AP we're returning to normal operation.
         */
        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_CANCELED;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s event=%s not handled\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));

        return false;
    }

    return true;
}

static void
ieee80211tdls_chan_sx_init_send_request_entry(void *ctx)
{
    chan_switch_t    *cs = ctx;
    int              rc;

    rc = tdls_send_chan_switch_req((SELECT_VAP(cs))->iv_ic,
                                   SELECT_VAP(cs),
                                   cs->w_tdls->tdls_ni->ni_macaddr,
                                   cs->w_tdls->chan_switch.offchannel,
                                   cs->agreed_switch_time,
                                   cs->agreed_switch_timeout,
                                   ieee80211tdls_cx_req_completion_handler,
                                   cs);

    if (rc == 0) {
        ieee80211tdls_schedule_timed_event(cs, TDLS_TX_REQUEST_TIMEOUT, IEEE80211_TDLS_CHAN_SX_EVENT_TX_REQUEST_TIMEOUT);
    }
    else {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: failed to send ChanSwitchRequest rc=%d\n",
            __func__, rc);

        ieee80211_sm_dispatch(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR, 0, NULL);
    }
}

static bool
ieee80211tdls_chan_sx_init_send_request_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data)
{
    chan_switch_t    *cs = ctx;

    switch (event) {
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_REQUEST_OK:
        ieee80211tdls_clear_timed_event(cs);

        // Succeeded in transmitting Channel Switch Request
        cs->retry_count = 0;
        ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_REQUEST_SUCCESS);

        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_INIT_WAIT_RESPONSE);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_RESPONSE_RECEIVED:
        {
            /*
             * A ChannelSwitchResponse may be received while we're still
             * transmitting the ChannelSwitchRequest if the ACK is lost.
             * In this case the peer moves on and sends the response before the
             * hardware indicates completion of transmission of the request.
             */
            ieee80211tdls_chan_sx_sm_event_t    *chan_sx_event = event_data;

            ieee80211tdls_clear_timed_event(cs);

            // Succeeded in transmitting Channel Switch Request
            cs->retry_count = 0;
            ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_REQUEST_SUCCESS);

            IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s received Channel Switch Response status=%d agreed_switch_time=%d/%d\n",
                __func__,
                ieee80211_sm_get_current_state_name(cs->hsm_handle),
                chan_sx_event->cx_resp_status,
                chan_sx_event->cx_switch_time,
                chan_sx_event->cx_switch_timeout);

            if (chan_sx_event->cx_resp_status == 0) {
                if (cs->agreed_switch_time < chan_sx_event->cx_switch_time) {
                    cs->agreed_switch_time = chan_sx_event->cx_switch_time;
                }
                if (cs->agreed_switch_timeout < chan_sx_event->cx_switch_timeout) {
                    cs->agreed_switch_timeout = chan_sx_event->cx_switch_timeout;
                }

                ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SWITCH_TIME);
            }
            else {
                /*
                 * No need to send unsolicited channel switch response if peer
                 * rejected channel switch request.
                 */
                ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
            }
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR:
        ieee80211tdls_clear_timed_event(cs);

        /*
         * Increment number of attempts to send Channel Switch Request.
         * If exceeded retry limit, fail this attempt to go off-channel and
         * return to BASE_CHANNEL state to try again later.
         */
        cs->retry_count++;
        if (cs->retry_count > cs->policy.stats.request_retry_threshold) {
            /*
             * Report result of this channel switch attempt.
             * Policy module decides whether to continue attempting
             * channel switch.
             */
            cs->stop_channel_switch =
                ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_REQUEST_ERROR);

            // Leave network sleep
            cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_INTERNAL_ERROR;
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        }
        else {
            // Retry after a short delay
            ieee80211tdls_schedule_timed_event(cs, TDLS_SEND_REQUEST_RETRY_TIME, IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_SEND_REQUEST);
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_REQUEST_TIMEOUT:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_REQUEST_FAILED:
        ieee80211tdls_clear_timed_event(cs);

        /*
         * Increment number of attempts to send Channel Switch Request.
         * If exceeded retry limit, fail this attempt to go off-channel and
         * return to BASE_CHANNEL state to try again later.
         */
        cs->retry_count++;
        if (cs->retry_count > cs->policy.stats.request_retry_threshold) {
            /*
             * Report result of this channel switch attempt.
             * Policy module decides whether to continue attempting
             * channel switch.
             */
            cs->stop_channel_switch =
                ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_REQUEST_ERROR);

            // Leave network sleep
            cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_INTERNAL_ERROR;
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        }
        else {
            // Retry after a short delay
            ieee80211tdls_schedule_timed_event(cs, TDLS_SEND_REQUEST_RETRY_TIME, IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_SEND_REQUEST);
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_SEND_REQUEST:
        // Retry
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_INIT_SEND_REQUEST);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_CANCEL:
        ieee80211tdls_clear_timed_event(cs);

        // Send unsolicited channel switch response and return to base channel
        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_CANCELED;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s event=%s not handled\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));

        return false;
    }

    return true;
}

static void
ieee80211tdls_chan_sx_init_wait_response_entry(void *ctx)
{
    chan_switch_t    *cs = ctx;

    ieee80211tdls_schedule_timed_event(cs, TDLS_WAIT_RESPONSE_TIMEOUT, IEEE80211_TDLS_CHAN_SX_EVENT_WAIT_RESPONSE_TIMEOUT);
}

static bool
ieee80211tdls_chan_sx_init_wait_response_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data)
{
    chan_switch_t    *cs = ctx;

    switch (event) {
    case IEEE80211_TDLS_CHAN_SX_EVENT_RESPONSE_RECEIVED:
        {
            ieee80211tdls_chan_sx_sm_event_t    *chan_sx_event = event_data;

            ieee80211tdls_clear_timed_event(cs);

            IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s received Channel Switch Response status=%d switch_time=%d/%d\n",
                __func__,
                ieee80211_sm_get_current_state_name(cs->hsm_handle),
                chan_sx_event->cx_resp_status,
                chan_sx_event->cx_switch_time,
                chan_sx_event->cx_switch_timeout);

            if (chan_sx_event->cx_resp_status == 0) {
                if (cs->agreed_switch_time < chan_sx_event->cx_switch_time) {
                    cs->agreed_switch_time = chan_sx_event->cx_switch_time;
                }
                if (cs->agreed_switch_timeout < chan_sx_event->cx_switch_timeout) {
                    cs->agreed_switch_timeout = chan_sx_event->cx_switch_timeout;
                }

                ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SWITCH_TIME);
            }
            else {
                /*
                 * No need to send unsolicited channel switch response if peer
                 * rejected channel switch request.
                 */
                ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
            }
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_WAIT_RESPONSE_TIMEOUT:
        ieee80211tdls_clear_timed_event(cs);

        /*
         * Peer may have successfully received the channel switch request, so
         * we must indicate we're returning to the base channel.
         * Of course peer may be already offchannel by now and will miss the
         * unsolicited channel switch response, but we should re-synchronize
         * back in the base channel.
         */
        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_WAIT_RESPONSE_TIMEOUT;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_CANCEL:
        ieee80211tdls_clear_timed_event(cs);

        /*
         * Peer may have successfully received the channel switch request, so
         * we must indicate we're returning to the base channel.
         * Of course peer may be already offchannel by now and will miss the
         * unsolicited channel switch response, but we should re-synchronize
         * back in the base channel.
         */
        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_CANCELED;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_REQUEST_OK:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_REQUEST_TIMEOUT:
        /*
         * Ignore these events.
         * They may occur if we transmit a ChannelSwitchRequest and the ACK
         * is lost. In this case the peer moves on and sends the
         * ChannelSwitchResponse before the hardware indicates completion of
         * transmission of the request. The state machine accepts the response
         * and advances to the next state (this one). The hardware eventually
         * completes transmission of the request (with either success or
         * error), so this indication is received in an unexpected state.
         */
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s event=%s not handled\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));

        return false;
    }

    return true;
}

static void
ieee80211tdls_chan_sx_resp_suspending_entry(void *ctx)
{
    chan_switch_t    *cs = ctx;
    int              rc;

    // Request the PowerManagement module to enter Network Sleep
    rc = ieee80211tdls_pause_vap(cs);

    if (rc == 0) {
        // Set timeout for completion of pause request
        ieee80211tdls_schedule_timed_event(cs, TDLS_FAKESLEEP_TIMEOUT, IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_TIMEOUT);
    }
    else {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ieee80211_sta_power_pause failed rc=%d\n",
            __func__, rc);

        // Indicate error
        ieee80211_sm_dispatch(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR, 0, NULL);
    }
}

static bool
ieee80211tdls_chan_sx_resp_suspending_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data)
{
    chan_switch_t    *cs = ctx;

    switch (event) {
    case IEEE80211_TDLS_CHAN_SX_EVENT_REQUEST_RECEIVED:
        /*
         * Peer is retransmitting channel switch request while we're still
         * trying to enter network sleep. Ignore this request for now, and
         * let's hope we'll be able to enter network sleep before peer gives
         * up.
         */
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_COMPLETED:
        ieee80211tdls_clear_timed_event(cs);
        // Fakesleep suceeded
        cs->retry_count = 0;
        ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_FAKESLEEP_SUCCESS);

        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESP_SEND_RESPONSE);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR:
        ieee80211tdls_clear_timed_event(cs);

        /*
         * Increment number of attempts to send fakesleep.
         * Give up if exceeded retry limit.
         */
        cs->retry_count++;
        if (cs->retry_count > cs->policy.stats.fakesleep_retry_threshold) {
            /*
             * Report result of this channel switch attempt.
             * Policy module decides whether to continue attempting
             * channel switch.
             * Not needed for the responder, but we'll do it for consistency.
             */
            cs->stop_channel_switch =
                ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_FAKESLEEP_ERROR);

            /*
             * Something wrong with the driver. Technically there's no need to
             * leave network sleep since we couldn't even enter it, but we'll do
             * it just in case.
             */
            cs->is_protocol_error = true;
            cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_INTERNAL_ERROR;
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        }
        else {
            // Retry after a short delay
            ieee80211tdls_schedule_timed_event(cs, TDLS_FAKESLEEP_RETRY_TIME, IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_FAKESLEEP);
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_FAILED:
    case IEEE80211_TDLS_CHAN_SX_EVENT_FAKESLEEP_TIMEOUT:
        ieee80211tdls_clear_timed_event(cs);

        /*
         * Increment number of attempts to send fakesleep.
         * Give up if exceeded retry limit.
         */
        cs->retry_count++;
        if (cs->retry_count > cs->policy.stats.fakesleep_retry_threshold) {
            /*
             * Report result of this channel switch attempt.
             * Policy module decides whether to continue attempting
             * channel switch.
             * Not needed for the responder, but we'll do it for consistency.
             */
            cs->stop_channel_switch =
                ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_FAKESLEEP_ERROR);

            /*
             * Error entering network sleep, so ignore the incoming channel switch
             * request. Since there's a chance the NULL data frame with PM=1 was
             * received by the AP, explicitly leave network sleep.
             */
            cs->is_protocol_error = true;
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        }
        else {
            // Retry after a short delay
            ieee80211tdls_schedule_timed_event(cs, TDLS_FAKESLEEP_RETRY_TIME, IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_FAKESLEEP);
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_FAKESLEEP:
        // Retry
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESP_SUSPENDING);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_CANCEL:
        ieee80211tdls_clear_timed_event(cs);

        /*
         * Cancelation received as we're trying to enter network sleep, so we
         * must explicitly leave network sleep.
         */
        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_CANCELED;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s event=%s not handled\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));

        return false;
    }

    return true;
}

static void
ieee80211tdls_chan_sx_resp_send_response_entry(void *ctx)
{
    chan_switch_t    *cs = ctx;
    int              rc;

    // Send CS Response
    rc = tdls_send_chan_switch_resp((SELECT_VAP(cs))->iv_ic,
                                    SELECT_VAP(cs),
                                    cs->w_tdls->tdls_ni->ni_macaddr,
                                    IEEE80211_STATUS_SUCCESS,
                                    cs->agreed_switch_time,
                                    cs->agreed_switch_timeout,
                                    ieee80211tdls_cx_resp_completion_handler,
                                    cs);

    if (rc == 0) {
        ieee80211tdls_schedule_timed_event(cs, TDLS_TX_RESPONSE_TIMEOUT, IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_TIMEOUT);
    }
    else {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: failed to send ChanSwitchRequest rc=%d\n",
            __func__, rc);

        // Indicate error
        ieee80211_sm_dispatch(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR, 0, NULL);
    }
}

static bool
ieee80211tdls_chan_sx_resp_send_response_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data)
{
    chan_switch_t    *cs = ctx;

    switch (event) {
    case IEEE80211_TDLS_CHAN_SX_EVENT_REQUEST_RECEIVED:
        /*
         * Peer retransmitted channel switch request while we were still
         * trying to enter network sleep. No need to retransmit the response.
         * Hopefully peer will accept the channel switch response we're
         * trying to transmit.
         */
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_OK:
        ieee80211tdls_clear_timed_event(cs);

        // Channel Switch Response transmitted successfully
        cs->retry_count = 0;
        ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_RESPONSE_SUCCESS);

        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SWITCH_TIME);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR:
        ieee80211tdls_clear_timed_event(cs);

        cs->retry_count++;
        if (cs->retry_count > cs->policy.stats.response_retry_threshold) {
            /*
             * Report result of this channel switch attempt.
             * Policy module decides whether to continue attempting
             * channel switch.
             * Not needed for the responder, but we'll do it for consistency.
             */
            cs->stop_channel_switch =
                ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_RESPONSE_ERROR);

            /*
             * Something wrong with driver. Ignore channel switch request.
             * Must leave network sleep before returning to IDLE.
             */
            cs->is_protocol_error = true;
            cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_INTERNAL_ERROR;
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        }
        else {
            // Retry after a short delay
            ieee80211tdls_schedule_timed_event(cs, TDLS_SEND_RESPONSE_RETRY_TIME, IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_SEND_RESPONSE);
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_TIMEOUT:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_FAILED:
        ieee80211tdls_clear_timed_event(cs);

        cs->retry_count++;
        if (cs->retry_count > cs->policy.stats.response_retry_threshold) {
            /*
             * Report result of this channel switch attempt.
             * Policy module decides whether to continue attempting
             * channel switch.
             * Not needed for the responder, but we'll do it for consistency.
             */
            cs->stop_channel_switch =
                ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_RESPONSE_ERROR);

            /*
             * Error trying to send channel switch response, so leave network
             * sleep and remain in base channel.
             * No reason to send an unsolicited channel switch response here, as
             * it could be mistaken by the actual response.
             * There's a possibility peer received the response but the ACK was
             * lost. In this case we'll re-synchronize with peer after probing
             * fails and causes it return to base channel.
             */
            cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_INTERNAL_ERROR;
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        }
        else {
            // Retry after a short delay
            ieee80211tdls_schedule_timed_event(cs, TDLS_SEND_RESPONSE_RETRY_TIME, IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_SEND_RESPONSE);
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_RETRY_SEND_RESPONSE:
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESP_SEND_RESPONSE);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_CANCEL:
        ieee80211tdls_clear_timed_event(cs);

        /*
         * Cancelation while transmitting send channel switch response. Leave
         * network sleep and remain in base channel.
         * No reason to send an unsolicited channel switch response here, as
         * it could be mistaken by the actual response.
         * Peer will probably receive the response we were trying to transmit
         * go offchannel. Tough luck. Peer's probing will fail and cause a
         * return to base channel.
         */
        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_CANCELED;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s event=%s not handled\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));

        return false;
    }

    return true;
}

static void
ieee80211tdls_chan_sx_resp_reject_request_entry(void *ctx)
{
    chan_switch_t    *cs = ctx;
    int              rc;

    // Send CS Response with reject status
    rc = tdls_send_chan_switch_resp((SELECT_VAP(cs))->iv_ic,
                                    SELECT_VAP(cs),
                                    cs->w_tdls->tdls_ni->ni_macaddr,
                                    IEEE80211_STATUS_REFUSED,
                                    0,
                                    0,
                                    ieee80211tdls_cx_resp_completion_handler,
                                    cs);
    if (rc == 0) {
        ieee80211tdls_schedule_timed_event(cs, TDLS_TX_RESPONSE_TIMEOUT, IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_TIMEOUT);
    }
    else {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: failed to send ChanSwitchRequest rc=%d\n",
            __func__, rc);

        // Indicate error
        ieee80211_sm_dispatch(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR, 0, NULL);
    }
}

static bool
ieee80211tdls_chan_sx_resp_reject_request_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data)
{
    chan_switch_t    *cs = ctx;

    switch (event) {
    case IEEE80211_TDLS_CHAN_SX_EVENT_REQUEST_RECEIVED:
        /*
         * Peer retransmitted channel switch request while we were trying to
         * reject the initial one - either because it took us too long to
         * enter network sleep or because we lost the ACK to our reject frame
         * and peer is already trying to request channel switch again. Either
         * way, just ignore new request.
         */
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_OK:
        ieee80211tdls_clear_timed_event(cs);

        /*
         * Leave network sleep before returning to IDLE.
         * Sometimes we decide to reject a channel switch request as soon as
         * it is received, so we don't even enter network sleep. In this case
         * we could simply return to IDLE.
         * However, we may also decide to reject a channel switch request
         * after entering or attempting to enter network sleep, in which case
         * we have to leave network sleep.
         */
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_FAILED:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_TIMEOUT:
        ieee80211tdls_clear_timed_event(cs);

        /*
         * Don't feel too bad just because our frame was lost: we're trying to
         * reject the request anyway. It'll appear to the initiator that we
         * ignored the request instead of rejecting it, but the final result
         * is the same.
         */
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_CANCEL:
        /*
         * No need to do anything now. Just wait for transmission of our
         * frame to complete and then proceed.
         */
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s event=%s not handled\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));

        return false;
    }

    return true;
}

static void
ieee80211tdls_chan_sx_switch_time_entry(void *ctx)
{
    chan_switch_t    *cs = ctx;
    int32_t          max_offchannel_time;
    bool             is_timer_set = false;

    // Set switch time (value kept by the driver is in microseconds, so convert to ms)
    ieee80211tdls_schedule_timed_event(cs, (cs->agreed_switch_time / 1000), IEEE80211_TDLS_CHAN_SX_EVENT_SWITCH_TIME);

    // Enter offchannel
    if (ieee80211tdls_channel_transition(cs, cs->offchannel) == EOK) {
        cs->is_offchannel = true;
    }

    // Set flag indicating we're waiting for data on the offchannel.
    cs->monitor_data = true;

    chan_switch_post_event(cs,
                           IEEE80211_TDLS_CHAN_SWITCH_OFFCHANNEL,
                           IEEE80211_TDLS_CHAN_SWITCH_REASON_NONE);

    /*
     * Recalculate time remaining before next DTIM after the channel change
     */
    max_offchannel_time = ieee80211tdls_calculate_max_offchannel_time_usec(cs->w_tdls->tdls_vap,
                                                                           cs->w_tdls->tdls_vap->iv_bss,
                                                                           cs->hw_switch_time);

    if (max_offchannel_time > 0) {
        is_timer_set =
            (ieee80211tdls_schedule_high_definition_event(cs,
                                                          max_offchannel_time,
                                                          IEEE80211_TDLS_CHAN_SX_EVENT_DTIM_TIMER) == EOK);
    }
    else {
        /*
         * No time to spend in the offchannel.
         * Print a message and dispatch an event to force the state machine to
         * return to the base channel.
         */
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_ANY, "%s: Don't go offchannel: max_offchannel_time=%d\n",
            __func__, max_offchannel_time);
    }

    if (! is_timer_set) {
        cs->monitor_data = false;
        ieee80211tdls_clear_timed_event(cs);

        // Error setting timer; dispatch special event
        ieee80211_sm_dispatch(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR, 0, NULL);
    }
}

static bool
ieee80211tdls_chan_sx_switch_time_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data)
{
    chan_switch_t    *cs = ctx;

    switch (event) {
    case IEEE80211_TDLS_CHAN_SX_EVENT_SWITCH_TIME:
        /*
         * Switch time elapsed.
         * Check policy to decide whether to start probing or to send
         * unsolicited channel switch response and abort operation.
         */
        if (tdls_policy_query(cs, TDLS_POLICY_VALUE_INTERRUPT_CHAN_SWITCH_REQUEST, NULL)) {
            cs->stop_channel_switch = true;
            cs->chan_switch_reason  = IEEE80211_TDLS_CHAN_SWITCH_REASON_CANCELED;
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        }
        else {
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_PROBING);
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_DATA_TRAFFIC_RECEIVED:
        cs->monitor_data = false;
        ieee80211tdls_clear_timed_event(cs);

        /*
         * Received data before switch time elapsed.
         * Check policy to decide whether to go offchannel or to send
         * unsolicited channel switch response and abort operation.
         */
        if (tdls_policy_query(cs, TDLS_POLICY_VALUE_INTERRUPT_CHAN_SWITCH_REQUEST, NULL)) {
            cs->stop_channel_switch = true;
            cs->chan_switch_reason  = IEEE80211_TDLS_CHAN_SWITCH_REASON_CANCELED;
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        }
        else {
            // Accept the early traffic indication and go offchannel.
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_OFFCHANNEL);
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_RESPONSE_RECEIVED:
        ieee80211tdls_clear_timed_event(cs);

        // Unsolicited channel switch response; go back to base channel
        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_UNSOLICITED_CHANNEL_SWITCH_RESPONSE;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_DTIM_TIMER:
        // DTIM TBTT approaching; return to base channel.
        ieee80211tdls_clear_timed_event(cs);

        /*
         * DTIM TBTT approaching.
         * Check policy to decide whether to cancel channel switch or return
         * to base channel and continue channel switching after DTIM.
         */
        if (tdls_policy_query(cs, TDLS_POLICY_VALUE_INTERRUPT_CHAN_SWITCH_REQUEST, NULL)) {
            cs->stop_channel_switch = true;
            cs->chan_switch_reason  = IEEE80211_TDLS_CHAN_SWITCH_REASON_CANCELED;
        }
        else {
            cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_DTIM_TIMER;
        }
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_CANCEL:
        ieee80211tdls_clear_timed_event(cs);

        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_CANCELED;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR:
        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_INTERNAL_ERROR;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_POOR_LINK_QUALITY:
        ieee80211tdls_clear_timed_event(cs);

        /*
         * Link Monitor is about to reset link due to too many errors.
         * Check policy to decide whether to cancel channel switch or return
         * to base channel and continue channel switching after DTIM.
         */
        if (tdls_policy_query(cs, TDLS_POLICY_VALUE_INTERRUPT_CHAN_SWITCH_REQUEST, NULL)) {
            cs->stop_channel_switch = true;
            cs->chan_switch_reason  = IEEE80211_TDLS_CHAN_SWITCH_REASON_CANCELED;
        }
        else {
            cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_POOR_LINK_QUALITY;
        }
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s event=%s not handled\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));

        return false;
    }

    return true;
}

static void
ieee80211tdls_chan_sx_probing_entry(void *ctx)
{
    chan_switch_t    *cs = ctx;
    int              rc;

    rc = tdls_send_qosnulldata(cs->w_tdls->tdls_ni,
                               WME_AC_VO,
                               0,
                               ieee80211tdls_cx_qosnulldata_completion_handler,
                               cs);

    /*
     * *** Special error handling here ***
     * Could not send Null Data frame. Bad...
     * Instead of aborting the offchannel request, start probing anyway and
     * hope the other peer will send a data frame.
     */
    if (rc != 0) {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: failed to send NullData rc=%d\n",
            __func__, rc);
    }

    // Set switch time (value kept by the driver is in microseconds, so convert to ms)
    ieee80211tdls_schedule_timed_event(cs, ((cs->agreed_switch_timeout - cs->agreed_switch_time) / 1000), IEEE80211_TDLS_CHAN_SX_EVENT_SWITCH_TIMEOUT);
}


static bool
ieee80211tdls_chan_sx_probing_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data)
{
    chan_switch_t    *cs = ctx;

    switch (event) {
    case IEEE80211_TDLS_CHAN_SX_EVENT_DATA_TRAFFIC_RECEIVED:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_OK:
        cs->monitor_data = false;
        ieee80211tdls_clear_timed_event(cs);

        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: Received data\n",
            __func__);

        // OK to go offchannel
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_OFFCHANNEL);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_SWITCH_TIMEOUT:
        // Did not receive data from peer; return to base channel.
        ieee80211tdls_clear_timed_event(cs);

        /*
         * Report result of this channel switch attempt.
         * Policy module decides whether to continue attempting
         * channel switch.
         * Not needed for the responder, but we'll do it for consistency.
         */
        cs->stop_channel_switch =
            ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_PROBE_ERROR);

        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_SWITCH_TIMEOUT;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_RESPONSE_RECEIVED:
        ieee80211tdls_clear_timed_event(cs);

        // Unsolicited channel switch response; go back to base channel
        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_UNSOLICITED_CHANNEL_SWITCH_RESPONSE;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_DTIM_TIMER:
        // DTIM TBTT approaching; return to base channel.
        ieee80211tdls_clear_timed_event(cs);

        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_DTIM_TIMER;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_CANCEL:
        // Cancel requested; return to base channel.
        ieee80211tdls_clear_timed_event(cs);

        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_CANCELED;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_FAILED:
        /*
         * Transmission of QOS Null Data failed. Maybe the peer has not
         * switched to the offchannel, but maybe it has and the ACK was lost.
         * Continue in the PROBING state, waiting for the first data frame.
         */
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_POOR_LINK_QUALITY:
        /*
         * Link Monitor is about to reset link due to too many errors.
         * Return to base channel
         */
        ieee80211tdls_clear_timed_event(cs);

        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_POOR_LINK_QUALITY;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s event=%s not handled\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));

        return false;
    }

    return true;
}

static void
ieee80211tdls_chan_sx_offchannel_entry(void *ctx)
{
    chan_switch_t    *cs = ctx;

    // Reached off-channel, so probe was successful.
    ieee80211_tdls_report_chan_switch_result(cs, IEEE80211_TDLS_CHAN_SWITCH_RESULT_PROBE_SUCCESS);
}

static bool
ieee80211tdls_chan_sx_offchannel_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data)
{
    chan_switch_t    *cs = ctx;

    switch (event) {
    case IEEE80211_TDLS_CHAN_SX_EVENT_RESPONSE_RECEIVED:
        ieee80211tdls_clear_timed_event(cs);

        // Unsolicited channel switch response; go back to base channel
        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_UNSOLICITED_CHANNEL_SWITCH_RESPONSE;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_DTIM_TIMER:
        // DTIM TBTT approaching; return to base channel.
        ieee80211tdls_clear_timed_event(cs);

        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_DTIM_TIMER;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_CANCEL:
        ieee80211tdls_clear_timed_event(cs);

        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_CANCELED;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_SWITCH_TIME:
        /*
         * Ignore.
         * This scenario can happen if we receive a data frame from the peer
         * about the same time the Switch Time is expiring.
         * The data frame causes a transition to OFFCHANNEL, and the SwitchTime
         * event is received in an unexpected state.
         */
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_OK:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_FAILED:
    case IEEE80211_TDLS_CHAN_SX_EVENT_DATA_TRAFFIC_RECEIVED:
    case IEEE80211_TDLS_CHAN_SX_EVENT_SWITCH_TIMEOUT:
        /*
         * Ignore these events. They may happen in case we receive a frame
         * from the peer at the same time we're transmitting one.
         * The first of these events causes a state transition, and the next
         * event is received in the new state.
         * Or we can complete transmission and/or receive an ACK as the timer
         * is expiring.
         */
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s ignored event=%s - already offchannel\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_POOR_LINK_QUALITY:
        /*
         * Link Monitor is about to reset link due to too many errors.
         * Return to base channel
         */
        ieee80211tdls_clear_timed_event(cs);

        cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_POOR_LINK_QUALITY;
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE);
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s event=%s not handled\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));

        return false;
    }

    return true;
}

static void
ieee80211tdls_chan_sx_send_unsolicited_response_entry(void *ctx)
{
    chan_switch_t    *cs = ctx;
    int              rc;

    /* No longer monitoring traffic */
    cs->monitor_data = false;

    rc = tdls_send_chan_switch_resp((SELECT_VAP(cs))->iv_ic,
                                    SELECT_VAP(cs),
                                    cs->w_tdls->tdls_ni->ni_macaddr,
                                    IEEE80211_STATUS_SUCCESS,
                                    cs->agreed_switch_time,
                                    cs->agreed_switch_timeout,
                                    ieee80211tdls_cx_resp_completion_handler,
                                    cs);
    if (rc == 0) {
        ieee80211tdls_schedule_timed_event(cs, TDLS_TX_UNSOLICITED_RESPONSE_TIMEOUT, IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_TIMEOUT);
    }
    else {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: failed to send ChanSwitchRequest rc=%d\n",
            __func__, rc);

        ieee80211_sm_dispatch(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR, 0, NULL);
    }
}

static bool
ieee80211tdls_chan_sx_send_unsolicited_response_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data)
{
    chan_switch_t    *cs = ctx;

    switch (event) {
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_OK:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_FAILED:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_TIMEOUT:
        /*
         * When sending an unsolicited channel switch response, it doesn't
         * matter whether transmission is successful. Just proceed to the
         * next state.
         */
        ieee80211tdls_clear_timed_event(cs);

        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR:
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_CANCEL:
        /*
         * Try to speed up cancelation process by proceeding immediately
         * to RESUMING state.
         */
        ieee80211tdls_clear_timed_event(cs);

        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_RESPONSE_RECEIVED:
        /*
         * Both peers sending unsolicited response. No need to wait until our
         * frame is transmitted. There's a good chance it will fail since our
         * peer has already indicated it's returning to the base channel, and
         * we want to avoid delays.
         */
        ieee80211tdls_clear_timed_event(cs);

        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_RESUMING);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_OK:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_FAILED:
    case IEEE80211_TDLS_CHAN_SX_EVENT_DATA_TRAFFIC_RECEIVED:
        /*
         * Ignore these events - too late now. Channel switch timeout occurred
         * just as we completed transmission of our frame or received an ACK
         * from peer. Since we are already transmitting the unsolicited
         * channel switch response, proceed to base channel.
         */
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s ignored event=%s - took too long rx/tx frame\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_POOR_LINK_QUALITY:
        /*
         * Link Monitor is about to reset link due to too many errors.
         * Already returning to base channel; nothing else to do.
         */
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s event=%s not handled\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));

        return false;
    }

    return true;
}

static void
ieee80211tdls_chan_sx_resuming_entry(void *ctx)
{
    chan_switch_t    *cs = ctx;
    int              rc;

    /* No longer monitoring traffic */
    cs->monitor_data = false;

    /* DTIM timer no longer needed */
    ieee80211tdls_cancel_high_definition_timer(cs);

    /* Return to base channel */
    if (cs->is_offchannel) {
        if (cs->w_tdls->tdls_vap->iv_bsschan != NULL) {
            if (ieee80211tdls_channel_transition(cs, cs->w_tdls->tdls_vap->iv_bsschan) == EOK) {
                /*
                 * Reason for return to base channel specified by the event that
                 * cause this transition.
                 */
                chan_switch_post_event(cs,
                                       IEEE80211_TDLS_CHAN_SWITCH_BASE_CHANNEL,
                                       cs->chan_switch_reason);
                // clear field containing reason for channel switch
                cs->chan_switch_reason = IEEE80211_TDLS_CHAN_SWITCH_REASON_NONE;

                cs->is_offchannel = false;
            }
        }
        else {
            IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ERROR: no Bsschan\n",
                __func__);
        }
    }

    /*
     * Request the PowerManagement module to leave NetworkSleep,
     * notifying the AP that we're back on the home channel.
     */
    rc = ieee80211tdls_unpause_vap(cs);

    if (rc == 0) {
        // Set timeout for completion of unpause request
        ieee80211tdls_schedule_timed_event(cs, TDLS_WAKE_UP_TIMEOUT, IEEE80211_TDLS_CHAN_SX_EVENT_WAKEUP_TIMEOUT);
    }
    else {
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: ieee80211_sta_power_unpause failed rc=%d\n",
            __func__, rc);

        // Indicate error
        ieee80211_sm_dispatch(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR, 0, NULL);
    }
}

static bool
ieee80211tdls_chan_sx_resuming_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data)
{
    chan_switch_t    *cs = ctx;

    switch (event) {
    case IEEE80211_TDLS_CHAN_SX_EVENT_API_ERROR:
        ieee80211tdls_clear_timed_event(cs);

        // Reason already determined
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_IDLE);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_WAKEUP_COMPLETED:
        ieee80211tdls_clear_timed_event(cs);

        if (cs->is_initiator && (! cs->stop_channel_switch)) {
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_BASE_CHANNEL);
        }
        else {
            // Reason already determined
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_IDLE);
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_WAKEUP_FAILED:
    case IEEE80211_TDLS_CHAN_SX_EVENT_WAKEUP_TIMEOUT:
        ieee80211tdls_clear_timed_event(cs);

        if (cs->is_initiator && (! cs->stop_channel_switch)) {
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_BASE_CHANNEL);
        }
        else {
            // Reason already determined
            ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_IDLE);
        }
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_CANCEL:
    case IEEE80211_TDLS_CHAN_SX_EVENT_RESPONSE_RECEIVED:
        // Already resuming; wait for other events
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_OK:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_TIMEOUT:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_OK:
        /*
         * Ignore these events.
         * If the TDLS peer transmits an unsolicited channel switch response
         * at the same time we do, there's a chance we'll receive the peer's
         * frame before our frame completes. In this case we transition to
         * RESUMING state and the TX_RESPONSE completion event is received in
         * the new state.
         */
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s ignored event=%s - took too long rx/tx frame\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_FAILED:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_FAILED:
        /*
         * Events posted almost simultaneously with reception of unsolicited
         * channel switch response may be handled in the next state.
         * In case of TX error, notify clients (namely the Link Monitor module)
         * that error must be ignored.
         */
        chan_switch_post_event(cs,
                               IEEE80211_TDLS_CHAN_SWITCH_DELAYED_TX_ERROR,
                               IEEE80211_TDLS_CHAN_SWITCH_REASON_NONE);

        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s ignored event=%s - took too long rx/tx frame\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_POOR_LINK_QUALITY:
        /*
         * Link Monitor is about to reset link due to too many errors.
         * Already returning to base channel; nothing else to do.
         */
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s event=%s not handled\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));

        return false;
    }

    return true;
}

static void
ieee80211tdls_chan_sx_base_channel_entry(void *ctx)
{
    chan_switch_t    *cs = ctx;
    u_int32_t        basechannel_timeout = TDLS_DEFAULT_BASECHANNEL_TIMEOUT;

    /* Wait at the base channel for a maximum of 1 beacon interval */
    if (cs->w_tdls->tdls_vap->iv_bss != NULL) {
        basechannel_timeout = IEEE80211_TU_TO_MS(cs->w_tdls->tdls_vap->iv_bss->ni_intval);
    }

    /*
     * End of the previous offchannel period; reset channel switch timing.
     * Do not reset hw_switch_time/timeout.
     */
    cs->agreed_switch_time    = cs->hw_switch_time;
    cs->agreed_switch_timeout = cs->hw_switch_timeout;
    cs->retry_count           = 0;
    cs->monitor_data          = false;
    cs->monitor_beacon        = true;

    ieee80211tdls_schedule_timed_event(cs,
                                       basechannel_timeout,
                                       IEEE80211_TDLS_CHAN_SX_EVENT_BASECHANNEL_TIMEOUT);
}

static void
ieee80211tdls_chan_sx_base_channel_exit(void *ctx)
{
    chan_switch_t    *cs = ctx;

    /* End of the previous basechannel period; reset beacon monitoring */
    cs->monitor_beacon = false;
}

static bool
ieee80211tdls_chan_sx_base_channel_event(void *ctx, u_int16_t event, u_int16_t event_data_len, void *event_data)
{
    chan_switch_t    *cs = ctx;

    switch (event) {
    case IEEE80211_TDLS_CHAN_SX_EVENT_BEACON_RECEIVED:
        ieee80211tdls_clear_timed_event(cs);

        ieee80211tdls_schedule_high_definition_event(cs,
                                                     TDLS_BASECHANNEL_OFFCHANNEL_DELAY_USEC,
                                                     IEEE80211_TDLS_CHAN_SX_EVENT_OFFCHANNEL_DELAY);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_OFFCHANNEL_DELAY:
        // Spent enough time in the base channel after beacon - go offchannel again
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_INIT_SUSPENDING);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_BASECHANNEL_TIMEOUT:
        // Spent enough time in the base channel - go offchannel again
        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_INIT_SUSPENDING);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_CANCEL:
        ieee80211tdls_clear_timed_event(cs);

        ieee80211_sm_transition_to(cs->hsm_handle, IEEE80211_TDLS_CHAN_SX_STATE_IDLE);
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_RESPONSE_RECEIVED:
        // Already in the base channel, so there's nothing to do
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_OK:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_TIMEOUT:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_OK:
        /*
         * Ignore these events.
         * If the TDLS peer transmits an unsolicited channel switch response
         * at the same time we do, there's a chance we'll receive the peer's
         * frame before our frame completes. In this case we transition to
         * RESUMING state and the TX_RESPONSE completion event is received in
         * the new state.
         */
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s ignored event=%s - took too long rx/tx frame\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));
        break;

    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_RESPONSE_FAILED:
    case IEEE80211_TDLS_CHAN_SX_EVENT_TX_QOSNULLDATA_FAILED:
        /*
         * Events posted almost simultaneously with reception of unsolicited
         * channel switch response may be handled in the next state.
         * In case of TX error, notify clients (namely the Link Monitor module)
         * that error must be ignored.
         */
        chan_switch_post_event(cs,
                               IEEE80211_TDLS_CHAN_SWITCH_DELAYED_TX_ERROR,
                               IEEE80211_TDLS_CHAN_SWITCH_REASON_NONE);

        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s ignored event=%s - took too long rx/tx frame\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));
        break;

    default:
        IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s: state=%s event=%s not handled\n",
            __func__, ieee80211_sm_get_current_state_name(cs->hsm_handle), ieee80211_tdls_chan_switch_event_name(event));

        return false;
    }

    return true;
}

ieee80211_state_info ieee80211_tdls_chan_sx_sm_info[] = {
    {
        (u_int8_t) IEEE80211_TDLS_CHAN_SX_STATE_IDLE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "IDLE",
        ieee80211tdls_chan_sx_state_idle_entry,
        NULL,
        ieee80211tdls_chan_sx_state_idle_event
    },
    {
        (u_int8_t) IEEE80211_TDLS_CHAN_SX_STATE_INIT_SUSPENDING,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "INIT_SUSPENDING",
        ieee80211tdls_chan_sx_init_suspending_entry,
        NULL,
        ieee80211tdls_chan_sx_init_suspending_event
    },
    {
        (u_int8_t) IEEE80211_TDLS_CHAN_SX_STATE_INIT_SEND_REQUEST,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "INIT_SEND_REQUEST",
        ieee80211tdls_chan_sx_init_send_request_entry,
        NULL,
        ieee80211tdls_chan_sx_init_send_request_event
    },
    {
        (u_int8_t) IEEE80211_TDLS_CHAN_SX_STATE_INIT_WAIT_RESPONSE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "INIT_WAIT_RESPONSE",
        ieee80211tdls_chan_sx_init_wait_response_entry,
        NULL,
        ieee80211tdls_chan_sx_init_wait_response_event
    },
    {
        (u_int8_t) IEEE80211_TDLS_CHAN_SX_STATE_RESP_SUSPENDING,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "RESP_SUSPENDING",
        ieee80211tdls_chan_sx_resp_suspending_entry,
        NULL,
        ieee80211tdls_chan_sx_resp_suspending_event
    },
    {
        (u_int8_t) IEEE80211_TDLS_CHAN_SX_STATE_RESP_SEND_RESPONSE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "RESP_SEND_RESPONSE",
        ieee80211tdls_chan_sx_resp_send_response_entry,
        NULL,
        ieee80211tdls_chan_sx_resp_send_response_event
    },
    {
        (u_int8_t) IEEE80211_TDLS_CHAN_SX_STATE_RESP_REJECT_REQUEST,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "RESP_REJECT_REQUEST",
        ieee80211tdls_chan_sx_resp_reject_request_entry,
        NULL,
        ieee80211tdls_chan_sx_resp_reject_request_event
    },
    {
        (u_int8_t) IEEE80211_TDLS_CHAN_SX_STATE_SWITCH_TIME,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "SWITCH_TIME",
        ieee80211tdls_chan_sx_switch_time_entry,
        NULL,
        ieee80211tdls_chan_sx_switch_time_event
    },
    {
        (u_int8_t) IEEE80211_TDLS_CHAN_SX_STATE_PROBING,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "PROBING",
        ieee80211tdls_chan_sx_probing_entry,
        NULL,
        ieee80211tdls_chan_sx_probing_event
    },
    {
        (u_int8_t) IEEE80211_TDLS_CHAN_SX_STATE_OFFCHANNEL,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "OFFCHANNEL",
        ieee80211tdls_chan_sx_offchannel_entry,
        NULL,
        ieee80211tdls_chan_sx_offchannel_event
    },
    {
        (u_int8_t) IEEE80211_TDLS_CHAN_SX_STATE_SEND_UNSOLICITED_RESPONSE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "SEND_UNSOLICITED_RESPONSE",
        ieee80211tdls_chan_sx_send_unsolicited_response_entry,
        NULL,
        ieee80211tdls_chan_sx_send_unsolicited_response_event
    },
    {
        (u_int8_t) IEEE80211_TDLS_CHAN_SX_STATE_RESUMING,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "RESUMING",
        ieee80211tdls_chan_sx_resuming_entry,
        NULL,
        ieee80211tdls_chan_sx_resuming_event
    },
    {
        (u_int8_t) IEEE80211_TDLS_CHAN_SX_STATE_BASE_CHANNEL,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        (u_int8_t) IEEE80211_HSM_STATE_NONE,
        false,
        "BASE_CHANNEL",
        ieee80211tdls_chan_sx_base_channel_entry,
        ieee80211tdls_chan_sx_base_channel_exit,
        ieee80211tdls_chan_sx_base_channel_event
    },
};

static int
tdls_cs_sm_create(chan_switch_t *cs)
{
    struct ieee80211com    *ic;

    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s:\n",
        __func__);

    ic = cs->w_tdls->tdls_vap->iv_ic;

    /* Create State Machine and start */
    cs->hsm_handle = ieee80211_sm_create(ic->ic_osdev,
                                         "TDLS_CHAN_SWITCH",
                                         (void *) cs,
                                         IEEE80211_TDLS_CHAN_SX_STATE_IDLE,
                                         ieee80211_tdls_chan_sx_sm_info,
                                         sizeof(ieee80211_tdls_chan_sx_sm_info)/sizeof(ieee80211_state_info),
                                         MAX_QUEUED_EVENTS,
                                         sizeof(struct ieee80211tdls_chan_sx_sm_event), /* size of event data */
                                         MESGQ_PRIORITY_HIGH,
                                         IEEE80211_HSM_ASYNCHRONOUS,
                                         ieee80211tdls_chan_sx_sm_debug_print,
                                         tdls_chan_sx_event_name,
                                         IEEE80211_N(tdls_chan_sx_event_name));

    if (!cs->hsm_handle) {
        printk("%s: : ieee80211_sm_create failed \n", __func__);
        return -ENOMEM;
    }

    return EOK;
}

static int
tdls_cs_sm_delete(chan_switch_t *cs)
{
    IEEE80211_DPRINTF(cs->w_tdls->tdls_vap, IEEE80211_MSG_TDLS, "%s:\n",
        __func__);

    /* Delete the HSM */
    if (cs->hsm_handle) {
        ieee80211_sm_delete(cs->hsm_handle);

        return -EINVAL;
    }

    return EOK;
}

#endif /* UMAC_SUPPORT_TDLS_CHAN_SWITCH */
#endif /* UMAC_SUPPORT_TDLS */
