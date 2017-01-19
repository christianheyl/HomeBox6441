/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc..
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#ifndef _IEEE80211_WIFIPOS_H
#define _IEEE80211_WIFIPOS_H
int ieee80211_wifipos_vattach(struct ieee80211vap *vap);
int ieee80211_wifipos_vdetach(struct ieee80211vap *vap);
int ieee80211_wifipos_set_txcorrection(wlan_if_t vaphandle, unsigned int corr);
int ieee80211_wifipos_set_rxcorrection(wlan_if_t vaphandle, unsigned int corr);
unsigned int ieee80211_wifipos_get_txcorrection(void);
unsigned int ieee80211_wifipos_get_rxcorrection(void);
void ieee80211_wifipos_nlsend_tsf_update(u_int8_t *mac);
#endif
