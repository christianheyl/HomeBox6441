/*
 *  Copyright (c) 2008 Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */

#ifndef _ATH_MLME_APP_IE_H
#define _ATH_MLME_APP_IE_H

struct app_ie_entry {

    bool                        inserted;       /* whether this entry is inserted into IE list */
    LIST_ENTRY(app_ie_entry)    link_entry;     /* link entry in IE list */

    struct ieee80211_app_ie_t   app_ie;
    u_int16_t                   ie_buf_maxlen;
};

bool ieee80211_mlme_app_ie_check_wpaie(struct ieee80211vap *vap);

u_int8_t *ieee80211_mlme_app_ie_append(
    struct ieee80211vap     *vap, 
    ieee80211_frame_type    ftype, 
    u_int8_t                *frm);

#endif /* end of _ATH_MLME_APP_IE_H */
