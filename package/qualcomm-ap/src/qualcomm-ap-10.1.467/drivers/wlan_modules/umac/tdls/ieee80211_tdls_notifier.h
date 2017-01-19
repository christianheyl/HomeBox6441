/*
 *  Copyright (c) 2005 Atheros Communications Inc.  All rights reserved.
 */

#ifndef _IEEE80211_TDLS_NOTIFIER_H_
#define _IEEE80211_TDLS_NOTIFIER_H_

#if UMAC_SUPPORT_TDLS

#define IEEE80211_MAX_TDLS_NOTIFICATION_HANDLERS        5

typedef void (*ieee80211_tdls_notification_handler) (
    struct ieee80211_node *ni,
    u_int16_t             notification_type,
    u_int16_t             notification_data_len,
    void                  *notification_data,
    void                  *arg);

typedef struct _ieee80211_tdls_notifier {
    struct ieee80211_node                  *ni;
    spinlock_t                             lock;
    ieee80211_tdls_notification_handler    handlers[IEEE80211_MAX_TDLS_NOTIFICATION_HANDLERS];
    void                                   *arg[IEEE80211_MAX_TDLS_NOTIFICATION_HANDLERS];
    int                                    num_handlers;
} ieee80211_tdls_notifier;

int
ieee80211_tdls_register_notification_handler(
    ieee80211_tdls_notifier             *notifier,
    ieee80211_tdls_notification_handler handler,
    void                                *arg);

int
ieee80211_tdls_unregister_notification_handler(
    ieee80211_tdls_notifier             *notifier,
    ieee80211_tdls_notification_handler handler,
    void                                *arg);

void
notifier_post_event(
    struct ieee80211_node   *ni,
    ieee80211_tdls_notifier *source_notifier,
    u_int16_t               notification_type,
    u_int16_t               notification_data_len,
    void                    *notification_data);



#endif /* UMAC_SUPPORT_TDLS */
#endif /* _IEEE80211_TDLS_NOTIFIER_H_ */


