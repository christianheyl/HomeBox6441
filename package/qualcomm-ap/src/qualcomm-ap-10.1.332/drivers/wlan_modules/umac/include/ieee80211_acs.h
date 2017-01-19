/*
 *  Copyright (c) 2008 Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */

#ifndef _IEEE80211_ACS_H
#define _IEEE80211_ACS_H

typedef struct ieee80211_acs    *ieee80211_acs_t;

#if UMAC_SUPPORT_ACS
#define ACS_CHAN_STATS 0 
#define ACS_CHAN_STATS_NF 1
#define IEEE80211_HAL_OFFLOAD_ARCH 0
#define IEEE80211_HAL_DIRECT_ARCH 1
int ieee80211_acs_attach(ieee80211_acs_t *acs, 
                          struct ieee80211com *ic, 
                          osdev_t             osdev);

int ieee80211_acs_detach(ieee80211_acs_t *acs);

void ieee80211_acs_stats_update(ieee80211_acs_t acs,
                                u_int8_t flags,
                                u_int ieee_chan,
                                int16_t chan_nf,
                                struct ieee80211_chan_stats *chan_stats);

#else /* UMAC_SUPPORT_ACS */

static INLINE int ieee80211_acs_attach(ieee80211_acs_t *acs, 
                                        struct ieee80211com *ic, 
                                        osdev_t             osdev) 
{
    return 0;
}
#define ieee80211_acs_detach(acs) /**/

#define ieee80211_acs_stats_update(acs, \
                                   flags,       \
                                   ieee_chan,   \
                                   chan_nf,      \
                                   chan_stats) 

#endif /* UMAC_SUPPORT_ACS */

#endif
