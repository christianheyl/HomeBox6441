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


#ifndef IEEE80211_TDLS_CHAN_SWITCH_H_
#define IEEE80211_TDLS_CHAN_SWITCH_H_
#include "ieee80211_tdls_notifier.h"
typedef enum {
    IEEE80211_TDLS_CHAN_SWITCH_CONTROL_NONE              = 0x0000,
    IEEE80211_TDLS_CHAN_SWITCH_CONTROL_REJECT_REQUEST    = 0x0001,
    IEEE80211_TDLS_CHAN_SWITCH_CONTROL_INTERRUPT_REQUEST = 0x0002,
} ieee80211_tdls_chan_switch_control_t;

#if UMAC_SUPPORT_TDLS_CHAN_SWITCH

static INLINE struct ieee80211vap *ieee80211tdls_chan_sx_select_vap(chan_switch_t *cs)
{
#if TDLS_USE_CHSX_VAP
    return(cs->chsx_vap);
#else
    return(cs->w_tdls->tdls_vap);
#endif
}

int
ieee80211tdls_chan_switch_sm_create(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls);

int
ieee80211tdls_chan_switch_sm_delete(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls);

int
ieee80211tdls_chan_switch_sm_start(
    struct ieee80211_node    *ni,
    wlan_tdls_sm_t           w_tdls,
    struct ieee80211_channel *offchannel
    );

int
ieee80211tdls_chan_switch_sm_stop(
    struct ieee80211_node    *ni,
    wlan_tdls_sm_t           tdls
    );

int
ieee80211tdls_chan_switch_sm_data_recv(
    struct ieee80211_node *ni,
    wlan_tdls_sm_t        w_tdls
    );

int
ieee80211tdls_chan_switch_sm_req_recv(
    struct ieee80211_node                             *ni,
    wlan_tdls_sm_t                                    w_tdls,
    struct ieee80211_tdls_chan_switch_frame           *channel_switch_frame,
    struct ieee80211_tdls_chan_switch_timing_ie       *channel_switch_timing,
    struct ieee80211_tdls_chan_switch_sec_chan_offset *sec_chan_offset
    );

int
ieee80211tdls_chan_switch_sm_resp_recv(
    struct ieee80211_node                       *ni,
    wlan_tdls_sm_t                              w_tdls,
    u_int16_t                                   status,
    struct ieee80211_tdls_chan_switch_timing_ie *channel_switch_timing
    );

void
ieee80211tdls_sta_power_event_handler(
    ieee80211_sta_power_t     powersta,
    ieee80211_sta_power_event *event,
    void                      *arg);

const char*
ieee80211_tdls_chan_switch_notification_type_name(
    ieee80211_tdls_chan_switch_notification_type_t notification_type);

const char*
ieee80211_tdls_chan_switch_notification_reason_name(
    ieee80211_tdls_chan_switch_notification_reason_t reason);

int
ieee80211_tdls_chan_switch_register_notification_handler(
    chan_switch_t                       *cs,
    ieee80211_tdls_notification_handler handler,
    void                                *arg);

int
ieee80211_tdls_chan_switch_unregister_notification_handler(
    chan_switch_t                       *cs,
    ieee80211_tdls_notification_handler handler,
    void                                *arg);

void
ieee80211tdls_chan_switch_handle_link_monitor_notification(
    struct ieee80211_node *ni,
    u_int16_t             notification_type,
    u_int16_t             notification_data_len,
    void                  *notification_data,
    void                  *arg);

int
ieee80211_tdls_set_channel_switch_control(
    chan_switch_t *cs,
    int           control_flags);

int
tdls_policy_query(chan_switch_t        *cs,
                  tdls_policy_value_t  value_type,
                  void                 *value_data);

int
tdls_policy_set(chan_switch_t        *cs,
                tdls_policy_value_t  value_type,
                int                  value,
                void                 *value_data);

#else /* ! UMAC_SUPPORT_TDLS_CHAN_SWITCH */

static INLINE int  ieee80211tdls_chan_switch_sm_create(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls) { return 0; }
static INLINE int  ieee80211tdls_chan_switch_sm_delete(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls) { return 0; }
static INLINE int  ieee80211tdls_chan_switch_sm_start(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls, struct ieee80211_channel *channel) { return 0; }
static INLINE int  ieee80211tdls_chan_switch_sm_stop(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls) { return 0; }
static INLINE ieee80211_vap_txrx_event_type ieee80211_tdls_chan_switch_get_txrx_filter(chan_switch_t *cs) { return 0; }
static INLINE int  ieee80211tdls_chan_switch_sm_data_recv(struct ieee80211_node *ni,wlan_tdls_sm_t w_tdls) { return 0; }
static INLINE void ieee80211_tdls_chan_switch_txrx_event_handler(ieee80211_vap_t vap, ieee80211_vap_txrx_event *event, void *arg) { return; }
static INLINE int  ieee80211tdls_chan_switch_sm_req_recv(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls, struct ieee80211_tdls_chan_switch_frame *channel_switch_frame, struct ieee80211_tdls_chan_switch_timing_ie *channel_switch_timing, struct ieee80211_tdls_chan_switch_sec_chan_offset *sec_chan_offset) { return 0; }
static INLINE int  ieee80211tdls_chan_switch_sm_resp_recv(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls, u_int16_t status, struct ieee80211_tdls_chan_switch_timing_ie *channel_switch_timing) { return 0; }
static INLINE void ieee80211tdls_sta_power_event_handler(ieee80211_sta_power_t powersta, ieee80211_sta_power_event *event, void *arg) { return; }
#define ieee80211_tdls_chan_switch_notification_type_name(_notification_type) "UNKNOWN"
#define ieee80211_tdls_chan_switch_notification_reason_name(_reason)  "UNKNOWN"
static INLINE int  ieee80211_tdls_chan_switch_register_notification_handler(chan_switch_t *cs, ieee80211_tdls_notification_handler handler, void *arg)  { return 0; }
static INLINE int  ieee80211_tdls_chan_switch_unregister_notification_handler(chan_switch_t *cs, ieee80211_tdls_notification_handler handler, void *arg)  { return 0; }
static INLINE void ieee80211tdls_chan_switch_handle_link_monitor_notification(struct ieee80211_node *ni, u_int16_t event, u_int16_t event_data_len, void *event_data, void *arg) { return; }
static INLINE int  ieee80211_tdls_set_channel_switch_control(chan_switch_t *cs, int control_flags) { return 0; }
static INLINE int  tdls_policy_query(chan_switch_t *cs, tdls_policy_value_t value_type, void *value_data) { return 0; }
static INLINE int  tdls_policy_set(chan_switch_t *cs, tdls_policy_value_t value_type, int value, void *value_data) { return 0; }

#endif /* ! UMAC_SUPPORT_TDLS_CHAN_SWITCH */
#endif /* IEEE80211_TDLS_CHAN_SWITCH_H_ */
