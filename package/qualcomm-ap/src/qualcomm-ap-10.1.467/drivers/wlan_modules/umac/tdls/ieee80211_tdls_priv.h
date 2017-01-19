/*
 *  Copyright (c) 2005 Atheros Communications Inc.  All rights reserved.
 */

#ifndef _IEEE80211_TDLS_H_
#define _IEEE80211_TDLS_H_

#if UMAC_SUPPORT_TDLS
#include "ieee80211_tdls_notifier.h"
#define IEEE80211_TDLS_USE_TSF_TIMER                             1
#define IEEE80211_TDLS_MAX_CLIENTS      256
#define IEEE80211_TDLS_DIALOG_TKN_SIZE  1
#define IEEE80211_TDLS_CHANNEL_SX_REQ_SIZE  34
#define IEEE80211_TDLS_CHANNEL_SX_RESP_SIZE 30
#define IEEE80211_TDLS_LEARNED_ARP_SIZE 6

#define IEEE80211_TDLS_ENABLE_LINK 251
#define IEEE80211_TDLS_ERROR 252
#define	IEEE80211_TDLS_TEARDOWN_DELETE_NODE 253
/* TDLS direct link teardown due to TDLS peer STA unreachable via the TDLS direct link */
#define IEEE80211_TDLS_TEARDOWN_UNREACHABLE_REASON 25
/* TDLS direct link teardown for unspecified reason */
#define IEEE80211_TDLS_TEARDOWN_UNSPECIFIED_REASON 26
/* Deauthenticated because sending STA is leaving IBSS or ESS */
#define IEEE80211_TDLS_TEARDOWN_LEAVING_BSS_REASON 3

#define IEEE80211_TDLS_PU_BUF_STA_PTI_RETRIES 3

/* temporary to be removed later */
#define dump_pkt(buf, len) { \
int i=0; \
for (i=0; i<len;i++) { \
    printf("%x ", ((unsigned char*)buf)[i]); \
    if(i && i%50 ==0) printf("\n"); } \
printf("\n"); }

/* XXX Move out */
/*  Fix for 
    EV:57703	Carrier 1.4.0.86 TDLS with WPA-PSK not work
    EV:58041	TDLS-WPA link not stable and doesn't work at 5Ghz band.
    Cause:
    DH-Peer Key is not transmitted by the Peer properly with skb_buf=500,
    as sizeof(SMK_M2) msg is 371+hdr, Using 500 size we see last 17 bytes
    of DH-Peer Key is corrupted abd hence increasing this size to 700.
    It works and fixes the above 2 issues.
*/
#define IEEE80211_TDLS_FRAME_MAXLEN   700

#define IEEE80211_TDLS_TID(x)             WME_AC_TO_TID(x)
#define IEEE80211_TDLS_RFTYPE      2
#define IEEE80211_ELEMID_TDLSLNK            101
#define IEEE80211_ELEMID_TDLS_WAKEUP_SCHED  102
#define IEEE80211_ELEMID_TDLS_CHAN_SX       104
#define IEEE80211_ELEMID_TDLS_PTI_CNTRL     105
#define IEEE80211_ELEMID_TDLS_PU_BUF_STATUS 106
#define IEEE80211_TDLS_LNKID_LEN   18
#define IEEE80211_TDLS_CATEGORY    12
#define IEEE80211_TDLS_TOKEN       1
#define IEEE80211_TDLS_PU_BUF_STATUS_LEN  1

/* Used by ioctls to add and delete nodes from upnp */

#define IEEE80211_TDLS_ADD_NODE    0
#define IEEE80211_TDLS_DEL_NODE    1

#define IEEE80211_TDLS_WHDR_SET(skb, vap, da) { \
    struct ether_header *eh = (struct ether_header *) \
                wbuf_push(skb, sizeof(struct ether_header)); \
        IEEE80211_ADDR_COPY(eh->ether_shost, vap->iv_myaddr); \
        IEEE80211_ADDR_COPY(eh->ether_dhost, da);\
        eh->ether_type = htons(IEEE80211_TDLS_ETHERTYPE); \
    }

/* Remote Frame Type = 2, TDLS Frame Type, Category && Action */
#define IEEE80211_TDLS_HDR_SET(frm, _type_, _len_, vap, arg) { \
        struct ieee80211_tdls_frame  *th = \
                    (struct ieee80211_tdls_frame *) frm; \
        th->payloadtype =IEEE80211_TDLS_RFTYPE; \
        th->category = IEEE80211_TDLS_CATEGORY; \
        th->action = _type_; \
        frm += sizeof(struct ieee80211_tdls_frame); \
        }

/* Status Code */
#define IEEE80211_TDLS_SET_STATUS(frm, vap, arg, _status_) { \
        struct ieee80211_tdls_status  *th = \
                    (struct ieee80211_tdls_status *) frm; \
        th->status = htole16(_status_); \
        frm += sizeof(struct ieee80211_tdls_status); \
        }

/* Dialogue Token */
#define IEEE80211_TDLS_SET_TOKEN(frm, vap, arg, _token_) { \
        struct ieee80211_tdls_token  *th = \
                    (struct ieee80211_tdls_token *) frm; \
        th->token = _token_; \
        frm += sizeof(struct ieee80211_tdls_token); \
        }

#define IEEE80211_TDLS_SET_LINKID(frm, vap, arg) { \
        struct ieee80211_tdls_lnk_id  *th = \
                    (struct ieee80211_tdls_lnk_id *) frm; \
        th->elem_id = IEEE80211_ELEMID_TDLSLNK; \
        th->len = IEEE80211_TDLS_LNKID_LEN; \
        IEEE80211_ADDR_COPY(th->bssid, vap->iv_bss->ni_bssid); \
        IEEE80211_ADDR_COPY(th->sa, vap->iv_myaddr); \
        IEEE80211_ADDR_COPY(th->da, (u_int8_t *)arg); \
        frm += sizeof(struct ieee80211_tdls_lnk_id); \
        }

#define IEEE80211_TDLS_SET_LINKID_RESP(frm, vap, arg) { \
        struct ieee80211_tdls_lnk_id  *th = \
                    (struct ieee80211_tdls_lnk_id *) frm; \
        th->elem_id = IEEE80211_ELEMID_TDLSLNK; \
        th->len = IEEE80211_TDLS_LNKID_LEN; \
        IEEE80211_ADDR_COPY(th->bssid, vap->iv_bss->ni_bssid); \
        IEEE80211_ADDR_COPY(th->sa, (u_int8_t *)arg); \
        IEEE80211_ADDR_COPY(th->da, vap->iv_myaddr); \
        frm += sizeof(struct ieee80211_tdls_lnk_id); \
        }

#define IEEE80211_TDLS_SET_PU_BUFFER_STATUS(frm, arg) { \
        struct ieee80211_tdls_pu_buffer_status_ie *th = \
                    (struct ieee80211_tdls_pu_buffer_status_ie *) frm; \
        th->elem_id = IEEE80211_ELEMID_TDLS_PU_BUF_STATUS; \
        th->len = IEEE80211_TDLS_PU_BUF_STATUS_LEN; \
        th->ac_traffic_avail = arg; \
        frm += sizeof(struct ieee80211_tdls_pu_buffer_status_ie); \
        }

#define IEEE80211_TDLS_SET_CHAN_SX_TIMING(frm, _switch_time, _switch_timeout) { \
         struct ieee80211_tdls_chan_switch_timing_ie *th = \
                    (struct ieee80211_tdls_chan_switch_timing_ie *) frm; \
         th->elem_id = IEEE80211_ELEMID_TDLS_CHAN_SX; \
         th->len = sizeof(struct ieee80211_tdls_chan_switch_timing_ie) - 2; \
         th->switch_time = (_switch_time); \
         th->switch_timeout = (_switch_timeout); \
         frm += sizeof(struct ieee80211_tdls_chan_switch_timing_ie); \
         }

#define IEEE80211_TDLS_SET_CHAN_SX_CHANNEL(frm, arg1, arg2) { \
         struct ieee80211_tdls_chan_switch_frame *th = \
                    (struct ieee80211_tdls_chan_switch_frame *) frm; \
         th->tgt_channel = arg1; \
         th->reg_class = arg2; \
         frm += sizeof(struct ieee80211_tdls_chan_switch_frame); \
         }

#define IEEE80211_TDLS_SET_CHAN_SX_SEC_CHANNEL_OFFSET(frm, arg) { \
         struct ieee80211_tdls_chan_switch_sec_chan_offset *th = \
                    (struct ieee80211_tdls_chan_switch_sec_chan_offset*) frm; \
         th->elem_id = IEEE80211_ELEMID_SECCHANOFFSET; \
         th->len = sizeof(struct ieee80211_tdls_chan_switch_sec_chan_offset) - 2; \
         th->sec_chan_offset = arg; \
         frm += sizeof(struct ieee80211_tdls_chan_switch_sec_chan_offset); \
         }

#define IEEE80211_TDLS_LLC_HDR_SET(frm) 

#define IEEE80211_TDLS_IS_INITIATOR(ni)    (ni->ni_tdls->is_initiator)

struct ieee80211_tdls_frame {
    int8_t       payloadtype;   /* IEEE80211_TDLS_RFTYPE */
    int8_t       category;      /* Category */
    int8_t       action;        /* Action (enum ieee80211_tdls_pkt_type) */
} __packed;

struct ieee80211_tdls_status {
    u_int16_t       status; /* Status Code */
} __packed;

struct ieee80211_tdls_token {
    int8_t       token; /* Dialogue Token */
} __packed;

struct ieee80211_tdls_lnk_id {
    u_int8_t    elem_id;
    u_int8_t    len;
    u_int8_t    bssid[6];
    u_int8_t    sa[6]; /* Intiator Mac */
    u_int8_t    da[6]; /* Responder Mac */
} __packed;

struct ieee80211_tdls_chan_switch_timing_ie {
    u_int8_t    elem_id;
    u_int8_t    len;
    u_int16_t   switch_time;
    u_int16_t   switch_timeout;
} __packed; 

struct ieee80211_tdls_pti_ie {
    u_int8_t    elem_id;
    u_int8_t    len;
    u_int8_t    tid;
    u_int16_t   seq_no;
} __packed;

struct ieee80211_tdls_pu_buffer_status_ie {
    u_int8_t    elem_id;
    u_int8_t    len;
    u_int8_t    ac_traffic_avail;
} __packed;

typedef enum {
    IEEE80211_TDLS_STATE_SETUPREQ,
    IEEE80211_TDLS_STATE_SETUPRESP,
    IEEE80211_TDLS_STATE_CONFIRM,
    IEEE80211_TDLS_STATE_TEARDOWN,
    IEEE80211_TDLS_STATE_LEARNEDARP,
}ieee80211_tdls_state;
    
typedef enum {
    IEEE80211_TDLS_PEER_UAPSD_STATE_INVALID = 0,
    IEEE80211_TDLS_PEER_UAPSD_STATE_READY,
    IEEE80211_TDLS_PEER_UAPSD_STATE_NOT_READY,
} ieee80211_tdls_peer_uapsd_state_t;

typedef enum {
    IEEE80211_TDLS_PU_BUF_STA_EVENT_NONE,                   /* None event */
    IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_ON,              /* start the sleep SM*/
    IEEE80211_TDLS_PU_BUF_STA_EVENT_PWRMGT_OFF,             /* stop the SM */
    IEEE80211_TDLS_PU_BUF_STA_EVENT_FORCE_SLEEP,            /* start sleep (do not wait for activity on vap) */
    IEEE80211_TDLS_PU_BUF_STA_EVENT_FORCE_AWAKE,            /* exit force sleep */
    IEEE80211_TDLS_PU_BUF_STA_EVENT_NO_ACTIVITY,            /* no tx/rx activity for vap */
    IEEE80211_TDLS_PU_BUF_STA_EVENT_NO_PENDING_TX,          /* no pending tx for vap */
    IEEE80211_TDLS_PU_BUF_STA_EVENT_PENDING_TX,             /* pending tx for vap */
    IEEE80211_TDLS_PU_BUF_STA_EVENT_SENT_PTI,               /* sent Peer Traffic Indication to remote peer */
    IEEE80211_TDLS_PU_BUF_STA_EVENT_PTI_TIMEOUT,            /* Timeout waiting for Peer Traffic Response */
    IEEE80211_TDLS_PU_BUF_STA_EVENT_RCVD_PTR,               /* Received Peer Traffic Response from remote peer */
    IEEE80211_TDLS_PU_BUF_STA_EVENT_INDICATION_WINDOW_TMO,  /* Peer Traffic Indication Window timeout */
    IEEE80211_TDLS_PU_BUF_STA_EVENT_SEND_QOSNULL_SUCCESS,   /* Send QosNull is successful */
    IEEE80211_TDLS_PU_BUF_STA_EVENT_SEND_QOSNULL_FAIL,      /* Send QosNull failed */

} ieee80211tdls_pu_buf_sta_event;

typedef enum {
    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_NONE,                /* None event */
    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_START,               /* start the sleep SM*/
    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_STOP,                /* stop the SM */
    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_FORCE_SLEEP,         /* start sleep (do not wait for activity on vap) */
    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_FORCE_AWAKE,         /* exit force sleep */
    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_NO_ACTIVITY,         /* no tx/rx activity for vap */
    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_NO_PENDING_TX,       /* no pending tx for vap */
    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_SEND_NULL_SUCCESS,   /* send null is succesful */
    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_SEND_NULL_FAILED,    /* send null failed */
    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_PTI,            /* recieved Peer Traffic Indication from remote peer */
    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RCVD_EOSP,           /* received End-Of-Service-Period */
    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RECV_UCAST,          /* received unicast frame (used when in PSPOLL state) */
    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_TX,                  /* tx path received  a frame for transmit */
    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_LAST_MCAST,          /* received last multicast */
    IEEE80211_TDLS_PU_SLEEP_STA_EVENT_RECV_UCAST_MOREDATA, /* received unicast with MoreData */
} ieee80211tdls_pu_sleep_sta_event;

struct ieee80211tdls_pu_buf_sta_sm_event {
    ieee80211_vap_t          vap;
    struct ieee80211_node    *ni_tdls;
    struct ieee80211_node    *ni_bss_node;
    u_int8_t                 wme_info_uapsd;
};
typedef struct ieee80211tdls_pu_buf_sta_sm_event ieee80211tdls_pu_buf_sta_sm_event_t;

struct ieee80211tdls_pu_sleep_sta_sm_event {
    ieee80211_vap_t          vap;
    struct ieee80211_node    *ni_tdls;
    struct ieee80211_node    *ni_bss_node;
    u_int8_t                 wme_info_uapsd;
};
typedef struct ieee80211tdls_pu_sleep_sta_sm_event ieee80211tdls_pu_sleep_sta_sm_event_t;

/* Peer U-APSD per node information */
struct peer_uapsd_buf_sta {
    ieee80211_tdls_peer_uapsd_state_t       state;
    ieee80211_hsm_t                         hsm_handle;     /* HSM Handle */
    wlan_tdls_sm_t                          w_tdls;        
    ieee80211tdls_pu_buf_sta_sm_event_t     deferred_event;
    ieee80211tdls_pu_buf_sta_event          deferred_event_type;
    os_timer_t                              buf_sta_sm_timer;
    os_timer_t                              action_response_timer;
    bool                                    pending_tx;
    bool                                    indication_window_tmo;
    bool                                    enable_pending_tx_notification;
    bool                                    process_sleep_transition;
    u_int8_t                                dialog_token;
    os_timer_t                              async_qosnull_event_timer;
    u_int16_t                               qosnull_attempt;
    u_int16_t                               pti_retry_count;
};
typedef struct peer_uapsd_buf_sta peer_uapsd_buf_sta_t;

struct peer_uapsd_sleep_sta {
    ieee80211_tdls_peer_uapsd_state_t       state;
    ieee80211_hsm_t                         hsm_handle;     /* HSM Handle */
    wlan_tdls_sm_t                          w_tdls;
    ieee80211tdls_pu_sleep_sta_sm_event_t   deferred_event;
    ieee80211tdls_pu_sleep_sta_event        deferred_event_type;
    u_int16_t                               sleep_attempt;
    os_timer_t                              async_null_event_timer;
    u_int8_t                                rcvd_peer_traffic_indication;
    u_int8_t                                rcvd_eosp;
    u_int8_t                                dialog_token;
};
typedef struct peer_uapsd_sleep_sta peer_uapsd_sleep_sta_t;

struct peer_uapsd {
    peer_uapsd_buf_sta_t    buf_sta;
    peer_uapsd_sleep_sta_t  sleep_sta;
};
typedef struct peer_uapsd peer_uapsd_t;

typedef enum {
    IEEE80211_TDLS_LINK_MONITOR_EVENT_NONE,         /* None event */
    IEEE80211_TDLS_LINK_MONITOR_EVENT_TMO,          /* Link Monitor timeout */
    IEEE80211_TDLS_LINK_MONITOR_EVENT_TX_OK,        /* Frame transmission successful */
    IEEE80211_TDLS_LINK_MONITOR_EVENT_TX_FAILED,    /* Frame transmission failed */
    IEEE80211_TDLS_LINK_MONITOR_EVENT_RX_OK,        /* Received unicast frame */
    IEEE80211_TDLS_LINK_MONITOR_EVENT_RESET,      /* Reset error monitoring */
    IEEE80211_TDLS_LINK_MONITOR_EVENT_START,        /* Start monitoring */
    IEEE80211_TDLS_LINK_MONITOR_EVENT_STOP,         /* Stop monitoring */
} ieee80211tdls_link_monitor_event;

struct ieee80211tdls_link_monitor_sm_event {
    ieee80211_vap_t          vap;
    struct ieee80211_node    *ni_tdls;
    struct ieee80211_node    *ni_bss_node;
};
typedef struct ieee80211tdls_link_monitor_sm_event ieee80211tdls_link_monitor_sm_event_t;

typedef enum ieee80211_tdls_link_monitor_notification_reason {
IEEE80211_TDLS_LINK_MONITOR_REASON_NONE,
IEEE80211_TDLS_LINK_MONITOR_REASON_START,
IEEE80211_TDLS_LINK_MONITOR_REASON_STOP,
IEEE80211_TDLS_LINK_MONITOR_REASON_TX_OK,
IEEE80211_TDLS_LINK_MONITOR_REASON_TX_ERROR,
IEEE80211_TDLS_LINK_MONITOR_REASON_RX_OK,
IEEE80211_TDLS_LINK_MONITOR_REASON_ERROR_COUNT,
IEEE80211_TDLS_LINK_MONITOR_REASON_TIMEOUT,
IEEE80211_TDLS_LINK_MONITOR_REASON_BASE_CHANNEL,
IEEE80211_TDLS_LINK_MONITOR_REASON_OFFCHANNEL,

IEEE80211_TDLS_LINK_MONITOR_NOTIFICATION_REASON_COUNT
} ieee80211_tdls_link_monitor_notification_reason;
struct link_monitor {
    ieee80211_hsm_t                         hsm_handle;     /* HSM Handle */
    u_int16_t                               tx_error_threshold;
    u_int16_t                               tx_error_count;
    os_timer_t                              link_monitor_timer;
    wlan_tdls_sm_t                          w_tdls;
    ieee80211tdls_link_monitor_sm_event_t   deferred_event;
    ieee80211tdls_link_monitor_event        deferred_event_type;
    ieee80211_tdls_link_monitor_notification_reason    state_change_reason;
    ieee80211_tdls_notifier                 notifier;
    u_int32_t                               error_detected_timeout;
    bool                                    is_offchannel;
    u_int32_t                               error_recovery_schedule;
    u_int32_t                               error_recovery_timeout;
    systime_t                               error_recovery_start_time;
};
typedef struct link_monitor link_monitor_t;

int tdls_send_qosnulldata(struct ieee80211_node               *ni,
int                                 ac,
int                                 pwr_save,
wlan_action_frame_complete_handler  completion_handler,
void                                *context);

int
tdls_send_chan_switch_req(struct ieee80211com                *ic,
struct ieee80211vap                *vap,
uint8_t                            *dest_addr,
struct ieee80211_channel           *chan,
u_int32_t                          switch_time,
u_int32_t                          switch_timeout,
wlan_action_frame_complete_handler completion_handler,
void                               *context);

int
tdls_send_chan_switch_resp(struct ieee80211com                *ic,
struct ieee80211vap                *vap,
uint8_t                            *dest_addr,
u_int16_t                          status,
u_int32_t                          switch_time,
u_int32_t                          switch_timeout,
wlan_action_frame_complete_handler completion_handler,
void                               *context);

typedef enum _ieee80211_tdls_link_monitor_notification_type {
IEEE80211_TDLS_LINK_MONITOR_NONE,
IEEE80211_TDLS_LINK_MONITOR_STARTED,
IEEE80211_TDLS_LINK_MONITOR_STOPPED,
IEEE80211_TDLS_LINK_MONITOR_INIT,
IEEE80211_TDLS_LINK_MONITOR_NORMAL,
IEEE80211_TDLS_LINK_MONITOR_ERROR_DETECTED,
IEEE80211_TDLS_LINK_MONITOR_ERROR_RECOVERY,
IEEE80211_TDLS_LINK_MONITOR_UNREACHABLE,

IEEE80211_TDLS_LINK_MONITOR_NOTIFICATION_TYPE_COUNT
} ieee80211_tdls_link_monitor_notification_type;

typedef enum _ieee80211_tdls_chan_switch_notification_reason {
     IEEE80211_TDLS_CHAN_SWITCH_REASON_NONE,
     IEEE80211_TDLS_CHAN_SWITCH_REASON_SWITCH_TIMEOUT,
     IEEE80211_TDLS_CHAN_SWITCH_REASON_OFFCHANNEL_TIMEOUT,
     IEEE80211_TDLS_CHAN_SWITCH_REASON_TRANSMIT_REQUEST_ERROR,
     IEEE80211_TDLS_CHAN_SWITCH_REASON_WAIT_RESPONSE_TIMEOUT,
     IEEE80211_TDLS_CHAN_SWITCH_REASON_TX_RESPONSE_FAILED,
     IEEE80211_TDLS_CHAN_SWITCH_REASON_UNSOLICITED_CHANNEL_SWITCH_RESPONSE,
     IEEE80211_TDLS_CHAN_SWITCH_REASON_DTIM_TIMER,
     IEEE80211_TDLS_CHAN_SWITCH_REASON_LINK_MONITOR,
     IEEE80211_TDLS_CHAN_SWITCH_REASON_CANCELED,
     IEEE80211_TDLS_CHAN_SWITCH_REASON_POWER_SAVE_ERROR,
     IEEE80211_TDLS_CHAN_SWITCH_REASON_INTERNAL_ERROR,
     IEEE80211_TDLS_CHAN_SWITCH_REASON_POOR_LINK_QUALITY,

     IEEE80211_TDLS_CHAN_SWITCH_NOTIFICATION_REASON_COUNT
} ieee80211_tdls_chan_switch_notification_reason_t;
typedef struct _ieee80211_tdls_chan_switch_notification_data {
ieee80211_tdls_chan_switch_notification_reason_t    reason;
} ieee80211_tdls_chan_switch_notification_data_t;

typedef struct _ieee80211_tdls_link_monitor_notification_data {
ieee80211_tdls_link_monitor_notification_reason    reason;
}ieee80211_tdls_link_monitor_notification_data; 
typedef enum _ieee80211_tdls_chan_switch_result {
IEEE80211_TDLS_CHAN_SWITCH_RESULT_NONE,
IEEE80211_TDLS_CHAN_SWITCH_RESULT_RESET,
IEEE80211_TDLS_CHAN_SWITCH_RESULT_FAKESLEEP_SUCCESS,
IEEE80211_TDLS_CHAN_SWITCH_RESULT_FAKESLEEP_ERROR,
IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_REQUEST_SUCCESS,
IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_REQUEST_ERROR,
IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_RESPONSE_SUCCESS,
IEEE80211_TDLS_CHAN_SWITCH_RESULT_SEND_RESPONSE_ERROR,
IEEE80211_TDLS_CHAN_SWITCH_RESULT_PROBE_SUCCESS,
IEEE80211_TDLS_CHAN_SWITCH_RESULT_PROBE_ERROR,

IEEE80211_TDLS_CHAN_SWITCH_RESULT_COUNT
} ieee80211_tdls_chan_switch_result_t;

#if IEEE80211_TDLS_USE_TSF_TIMER
#define tdls_high_definition_timer_t    tsftimer_handle_t
#else
#define tdls_high_definition_timer_t    os_timer_t
#endif
typedef struct chan_switch_stats {
    int    fakesleep_failure_count;
    int    fakesleep_failure_threshold;
    int    fakesleep_retry_threshold;
    
    int    request_failure_count;
    int    request_failure_threshold;
    int    request_retry_threshold;
    
    int    response_failure_count;
    int    response_failure_threshold;
    int    response_retry_threshold;
    
    int    offchannel_failure_count;
    int    offchannel_failure_threshold;
} chan_switch_stats_t;

typedef struct _chan_switch_policy {
int                    reject_request;
int                    interrupt_request;    // WFA test mode - Send ChanSwitchRequest then send unsolicited response

chan_switch_stats_t    stats;
} chan_switch_policy_t;

typedef enum _tdls_policy_t {
TDLS_POLICY_DEFAULT,
TDLS_POLICY_WFA_CERT
} tdls_policy_t;


typedef enum _tdls_policy_value_t {
     TDLS_POLICY_VALUE_NONE,
     TDLS_POLICY_VALUE_IGNORE_CHAN_SWITCH_REQUEST,
     TDLS_POLICY_VALUE_REJECT_CHAN_SWITCH_REQUEST,
     TDLS_POLICY_VALUE_INTERRUPT_CHAN_SWITCH_REQUEST,

     TDLS_POLICY_VALUE_COUNT
} tdls_policy_value_t;
typedef struct chan_switch chan_switch_t;
typedef int (*policy_query_function_t) (chan_switch_t *cs, tdls_policy_value_t value_type , void *query_data);
typedef int (*policy_set_function_t)   (chan_switch_t *cs, tdls_policy_value_t value_type, int value, void *query_data);

struct chan_switch {
    ieee80211_hsm_t                         hsm_handle;
    wlan_tdls_sm_t                          w_tdls;
    wlan_dev_t devhandle;
    wlan_if_t chsx_vap;
     /* Notification of channel switch events */
    ieee80211_tdls_notifier                 notifier;
    /* Parameters used during channel switch operation */
    struct ieee80211_channel                *offchannel;
    os_timer_t                              standard_event_timer;
    tdls_high_definition_timer_t            high_definition_timer;
    int                                     pending_standard_event;
    int                                     pending_high_definition_event;
     /* State Flags */
    int                                     in_progress;
    int                                     is_initiator;
    int                                     monitor_data;
    int                                     monitor_beacon;
    int                                     retry_count;
    int                                     cancelation_requested;
    int                                     stop_channel_switch; /* due to cancelation or unrecoverable errors */
    int                                     is_paused;
    int                                     is_offchannel;
    int                                     is_protocol_error;
  
       /* Policy controlling call switching */
    chan_switch_policy_t                    policy;
    policy_query_function_t                 policy_query_function;
    policy_set_function_t                   policy_set_function;
  
    ieee80211_tdls_chan_switch_notification_reason_t    chan_switch_reason;
  
    /* Timeout parameters */
    u_int32_t                               fakesleep_timeout;
    u_int32_t                               hw_switch_time;        /* how long our HW needs to switch channel */
    u_int32_t                               hw_switch_timeout;     /* how long our HW needs to switch channel */
    u_int32_t                               agreed_switch_time;    /* value agreed upon with the TDLS peer */
    u_int32_t                               agreed_switch_timeout; /* value agreed upon with the TDLS peer */
};

struct ieee80211_tdls_chan_switch_frame {
     u_int8_t    tgt_channel;
     u_int8_t    reg_class;
} __packed;

struct ieee80211_tdls_chan_switch_sec_chan_offset {
     u_int8_t    elem_id;
     u_int8_t    len;
     u_int8_t    sec_chan_offset;
} __packed;

typedef enum _ieee80211_tdls_chan_switch_notification_type {
     IEEE80211_TDLS_CHAN_SWITCH_STARTED,
     IEEE80211_TDLS_CHAN_SWITCH_COMPLETED,
     IEEE80211_TDLS_CHAN_SWITCH_OFFCHANNEL,
     IEEE80211_TDLS_CHAN_SWITCH_BASE_CHANNEL,
     IEEE80211_TDLS_CHAN_SWITCH_DELAYED_TX_ERROR,

     IEEE80211_TDLS_CHAN_SWITCH_NOTIFICATION_TYPE_COUNT
} ieee80211_tdls_chan_switch_notification_type_t;

/* Entry for each node and state machine (per MAC address) */

struct _wlan_tdls_sm {
    struct ieee80211vap    *tdls_vap;   /* back pointer to the vap */
    struct ieee80211_node  *tdls_ni;
        u_int8_t            tdls_addr[IEEE80211_ADDR_LEN];   /* node's mac addr */
    peer_uapsd_t            peer_uapsd;     /* Peer U-APSD Power Save information */
    link_monitor_t          link_monitor;   /* Link status information */
    TAILQ_ENTRY(_wlan_tdls_sm)   tdlslist;
    u_int32_t   pwr_arbiter_id;
    chan_switch_t           chan_switch;    /* Channel Switch information */
};

#if CONFIG_RCPI
/* TODO:RCPI: This is a temporary definition for RCPI Header, 
   refer 802.11k hdr */
typedef struct ieee80211_tdls_frame IEEE80211_TDLS_RCPI_HDR;

/* TODO:RCPI: Path Swicth Element ID */
#define TDLS_RCPI_PATH_ELEMENT_ID 0x1

#define AP_PATH         0
#define DIRECT_PATH     1

#define RCPI_TIMER           3000
#define RCPI_HIGH_THRESHOLD  35 
#define RCPI_LOW_THRESHOLD   10 
#define RCPI_MARGIN          20 

/* Reason for the Path Switch Request frame */
#define REASON_UNSPECIFIED                  0
#define REASON_CHANGE_IN_POWERSAVE_MODE     1
#define REASON_CHANGE_IN_LINK_STATE         2

/* Result for Tx Path Switch Response frame */
#define ACCEPT                                          0
#define REJECT_ENTERING_POWER_SAVE  1
#define REJECT_LINK_STATUS          2
#define REJECT_UNSPECIFIED_REASON   3

struct ieee80211_tdls_rcpi_path {
    u_int8_t     elem_id;    /* Element ID */
    u_int8_t     len;        /* Length of Element */
    u_int8_t     path;       /* Path: 0-AP, 1-Direct Link path */
} __packed;

struct ieee80211_tdls_rcpi_switch {
        struct ieee80211_tdls_rcpi_path path;
        u_int8_t reason;
} __packed;

struct ieee80211_tdls_rcpi_link_req {
        char bssid[ETH_ALEN];           /* BSSID: */
        char sta_addr[ETH_ALEN];        /* STA ADDR: */
} __packed;

struct ieee80211_tdls_rcpi_link_report {
        char bssid[ETH_ALEN];           /* BSSID: */
    u_int8_t     rcpi1;                 /* RCPI value for AP path */
        char sta_addr[ETH_ALEN];        /* STA ADDR: */
    u_int8_t     rcpi2;                 /* RCPI value for Direct Link path */
} __packed;


#define IEEE80211_TDLS_RCPI_SWITCH_SET(req, path, reason)       \
        req.path.elem_id = TDLS_RCPI_PATH_ELEMENT_ID;   \
        req.path.len     = 1;   \
        req.path.path    = path;        \
        req.reason       = reason;

#define IEEE80211_TDLS_RCPI_LINK_ADDR(req, bssid, mac)  \
    IEEE80211_ADDR_COPY(req.bssid, bssid);      \
    IEEE80211_ADDR_COPY(req.sta_addr, mac);

/* used in ieee80211_input() to calculate Avg.RSSI for each node */
#define IEEE80211_TDLS_UPDATE_RCPI ieee80211_tdls_update_rcpi

#define IEEE80211_TDLSRCPI_PATH_SET(ni)    (ni->ni_tdls->link_st = DIRECT_PATH)
#define IEEE80211_TDLSRCPI_PATH_RESET(ni)  (ni->ni_tdls->link_st = AP_PATH)
#define IEEE80211_IS_TDLSRCPI_PATH_SET(ni) (ni->ni_tdls->link_st)

#endif /* CONFIG_RCPI */

#endif /* UMAC_SUPPORT_TDLS */
#endif /* _IEEE80211_TDLS_H_ */
