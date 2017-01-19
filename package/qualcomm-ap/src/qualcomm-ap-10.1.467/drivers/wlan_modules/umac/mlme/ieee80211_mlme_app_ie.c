/* *  Copyright (c) 2010 Atheros Communications Inc.  All rights reserved.
 */
/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#include "ieee80211_mlme_priv.h"    /* Private to MLME module */


struct wlan_mlme_app_ie {
    wlan_if_t                   vaphandle;

    /* app ie entry for each frame type */
    struct app_ie_entry         entry[IEEE80211_APPIE_MAX_FRAMES];
};

static void remove_ie_entry(
    struct ieee80211vap *vap,
    struct app_ie_entry *ie_entry,
    ieee80211_frame_type ftype)
{
    ASSERT(ie_entry->inserted);

    LIST_REMOVE(ie_entry, link_entry);
    ie_entry->inserted = false;

    vap->iv_app_ie_list[ftype].total_ie_len -= ie_entry->app_ie.length;
    ie_entry->app_ie.length = 0;
}

/**
 * check wpa/rsn ie is present in the ie buffer passed in.
 * Returns false if the WPA IE is found.
 * NOTE: Assumed that the vap's lock to protect the vap->iv_app_ie_list[] structures are held.
 */
bool ieee80211_mlme_app_ie_check_wpaie(
    struct ieee80211vap     *vap)
{
    struct app_ie_entry         *ie_entry;
    bool                        add_wpa_ie = true;
    const ieee80211_frame_type  ftype = IEEE80211_FRAME_TYPE_ASSOCREQ;
    
    ASSERT(ftype < IEEE80211_APPIE_MAX_FRAMES);

    if (vap->iv_app_ie_list[ftype].total_ie_len == 0) {
        /* Not found since empty list */
        return true;
    }
    ASSERT(!LIST_EMPTY(&vap->iv_app_ie_list[ftype]));

    LIST_FOREACH(ie_entry, &vap->iv_app_ie_list[ftype], link_entry) {
        ASSERT(ie_entry->app_ie.ie != NULL);
        ASSERT(ie_entry->app_ie.length > 0);

        add_wpa_ie = ieee80211_check_wpaie(vap, ie_entry->app_ie.ie, ie_entry->app_ie.length);
        if (!add_wpa_ie) {
            /* Found WPA IE */
            break;
        }
    }
    return add_wpa_ie;
}

/*
 * Function to append the Application IE for this frame type.
 * Returns the new buffer pointer after the newly added IE and the
 * length of the added IE.
 * NOTE: Assumed that the vap's lock to protect the vap->iv_app_ie_list[] structures are held.
 */
u_int8_t *ieee80211_mlme_app_ie_append(
    struct ieee80211vap     *vap, 
    ieee80211_frame_type    ftype, 
    u_int8_t                *frm
    )
{
    struct app_ie_entry     *ie_entry;
    u_int16_t               ie_len;
    
    ASSERT(ftype < IEEE80211_APPIE_MAX_FRAMES);

    ie_len = 0;

    if (LIST_EMPTY(&vap->iv_app_ie_list[ftype])) {
        /* No change */
        vap->iv_app_ie_list[ftype].changed = false; /* indicates that we have used this ie. */
        return frm;
    }

    LIST_FOREACH(ie_entry, &vap->iv_app_ie_list[ftype], link_entry) {
        ASSERT(ie_entry->app_ie.ie != NULL);
        ASSERT(ie_entry->app_ie.length > 0);

        OS_MEMCPY(frm, ie_entry->app_ie.ie, ie_entry->app_ie.length);

        frm += ie_entry->app_ie.length;
        ie_len += ie_entry->app_ie.length;
    }
    ASSERT(ie_len == vap->iv_app_ie_list[ftype].total_ie_len);

    vap->iv_app_ie_list[ftype].changed = false; /* indicates that we have used this ie. */

    return frm;
}

/*
 * Create an instance to Application IE module to append new IE's for various frames.
 * Returns a handle for this instance to use for subsequent calls.
 */
wlan_mlme_app_ie_t wlan_mlme_app_ie_create(wlan_if_t vaphandle)
{
    struct ieee80211vap     *vap = vaphandle;
    struct ieee80211com     *ic = vap->iv_ic;
    wlan_mlme_app_ie_t      app_ie_handle = NULL;
    int                     error = 0;

    do {

        app_ie_handle = (wlan_mlme_app_ie_t)OS_MALLOC(ic->ic_osdev, sizeof(struct wlan_mlme_app_ie), 0);
        if (app_ie_handle == NULL) {
            error = -ENOMEM;
            break;
        }

        OS_MEMZERO(app_ie_handle, sizeof(struct wlan_mlme_app_ie));

        app_ie_handle->vaphandle = vaphandle;

    } while ( false );

    if (error != 0) {
        /* Some error. */
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: Failed and error=%d\n", __func__, error);

        if (app_ie_handle != NULL) {
            OS_FREE(app_ie_handle);
            app_ie_handle = NULL;
        }
    }

    return app_ie_handle;
}

/*
 * Detach from Application IE module. Any remaining IE's will be removed and freed.
 */
void wlan_mlme_app_ie_delete(wlan_mlme_app_ie_t app_ie_handle)
{
    ieee80211_frame_type    ftype;
    struct ieee80211vap     *vap;

    if (app_ie_handle == NULL) {
        printk("%s: handle is NULL. Do nothing.\n", __func__);
        return;
    }

    vap = app_ie_handle->vaphandle;
    ASSERT(vap);

    IEEE80211_VAP_LOCK(vap);

    /* 
     * Temp: reduce window of race with beacon update in Linux AP.
     * In Linux AP, ieee80211_beacon_update is called in ISR, so
     * iv_lock is not acquired.
     */
    IEEE80211_VAP_APPIE_UPDATE_DISABLE(vap);

    for (ftype = 0; ftype < IEEE80211_APPIE_MAX_FRAMES; ftype++) {
        struct app_ie_entry     *ie_entry;

        ie_entry = &(app_ie_handle->entry[ftype]);

        if (ie_entry->inserted) {
            /* Remove this App IE from vap's IE list */
            remove_ie_entry(vap, ie_entry, ftype);
        }

        if (ie_entry->ie_buf_maxlen > 0) {
            ASSERT(ie_entry->app_ie.ie != NULL);

            OS_FREE(ie_entry->app_ie.ie);
            ie_entry->app_ie.ie = NULL;
            ie_entry->app_ie.length = 0;
        }
    }

    /* Set appropriate flag so that the IE gets updated in the next beacon */
    IEEE80211_VAP_APPIE_UPDATE_ENABLE(vap);

    IEEE80211_VAP_UNLOCK(vap);

    OS_FREE(app_ie_handle);
}

/*
 * To set a new Application IE for a frame type. Any existing IE for this frame and
 * instance of APP IE will be overwritten. However, other IE from other instances
 * will be preserved. When buflen is zero, the existing IE for this instanace and
 * frame type is removed.
 */
int wlan_mlme_app_ie_set(
    wlan_mlme_app_ie_t      app_ie_handle, 
    ieee80211_frame_type    ftype, 
    u_int8_t                *buf, 
    u_int16_t               buflen)
{
    struct ieee80211vap     *vap = app_ie_handle->vaphandle;
    struct ieee80211com     *ic = vap->iv_ic;
    int                     error = 0;
    u_int8_t                *iebuf = NULL;
    bool                    allocated_iebuf = FALSE;
    struct app_ie_entry     *ie_entry;
    bool                    remove_ie = false;
    
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: Adding App IE for fr-type=%d\n",
                      __func__, ftype);

    ASSERT(ftype < IEEE80211_FRAME_TYPE_MAX);
    if (ftype >= IEEE80211_FRAME_TYPE_MAX) {
        error = -EINVAL;
        return error;
    }

    if ((buflen == 0) || (buf == NULL)) {
        /* Remove this App IE */
        remove_ie = true;
    }

    IEEE80211_VAP_LOCK(vap);
    /* 
     * Temp: reduce window of race with beacon update in Linux AP.
     * In Linux AP, ieee80211_beacon_update is called in ISR, so
     * iv_lock is not acquired.
     */
    IEEE80211_VAP_APPIE_UPDATE_DISABLE(vap);

    do {
        ie_entry = &(app_ie_handle->entry[ftype]);
    
        if (remove_ie) {
            /* Remove this App IE from vap's IE list */
            if (ie_entry->inserted) {
                remove_ie_entry(vap, ie_entry, ftype);
            }

            /* NOTE: for performance reasons, we keep the buffer around for reuse. */
            ie_entry->app_ie.length = 0;
        }
        else {
            /* Update this App IE */
            u_int16_t orig_buflen = ie_entry->app_ie.length;

            /* Make sure existing buffer is big enough */
            if (buflen > ie_entry->ie_buf_maxlen) {
                /* Old Buffer not big enough, allocate new ie buffer */
                iebuf = OS_MALLOC(ic->ic_osdev, buflen, 0);
    
                if (iebuf == NULL) {
                    error = -ENOMEM;
                    break;
                }
    
                allocated_iebuf = TRUE;
                ie_entry->ie_buf_maxlen = buflen;
            } else {
                /* Else reused the existing buffer */
                iebuf = ie_entry->app_ie.ie;
            }

            if (allocated_iebuf == TRUE && ie_entry->app_ie.ie) {
                /* Free existing buffer */
                OS_FREE(ie_entry->app_ie.ie);
                ie_entry->app_ie.ie = NULL;
            }

            /* Insert a new App IE into the vap's IE list */
            ie_entry->app_ie.ie = iebuf;
            ie_entry->app_ie.length = buflen;

            /* TODO: in the future, we may need to order (or sort) the IE's. 
               At this moment, I assumed that the order is not important other than
               it is added at the end of beacon frame. */
            if (!ie_entry->inserted) {
                /* This entry is never attached to IE list */
                ASSERT(orig_buflen == 0);
                LIST_INSERT_HEAD(&vap->iv_app_ie_list[ftype], ie_entry, link_entry);
                vap->iv_app_ie_list[ftype].total_ie_len += ie_entry->app_ie.length;
                ie_entry->inserted = true;
            }
            else {
                ASSERT(orig_buflen != 0);

                vap->iv_app_ie_list[ftype].total_ie_len += (ie_entry->app_ie.length - orig_buflen);
            }

            ASSERT(buflen);
            ASSERT(buf);

            /* Copy app ie contents and save pointer/length */
            OS_MEMCPY(iebuf, buf, buflen);
            vap->iv_app_ie_list[ftype].changed = true;
        }

    } while ( false );

    /* Set appropriate flag so that the IE gets updated in the next beacon */
    IEEE80211_VAP_APPIE_UPDATE_ENABLE(vap);

    IEEE80211_VAP_UNLOCK(vap);

    return error;
}

/*
 * To get the Application IE for this frame type and instance.
 */
int wlan_mlme_app_ie_get(
    wlan_mlme_app_ie_t      app_ie_handle, 
    ieee80211_frame_type    ftype, 
    u_int8_t                *buf, 
    u_int32_t               *ielen, 
    u_int32_t               buflen)
{
    int                     error = 0;
    struct ieee80211vap     *vap;

    ASSERT(app_ie_handle);
    vap = app_ie_handle->vaphandle;
    ASSERT(vap);

    ASSERT(ielen);
    *ielen = 0;

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: get IE for ftype=%d\n",
                      __func__, ftype);

    ASSERT(ftype < IEEE80211_FRAME_TYPE_MAX);
    if (ftype >= IEEE80211_FRAME_TYPE_MAX) {
        error = -EINVAL;
        return error;
    }

    IEEE80211_VAP_LOCK(vap);

    do {
        struct app_ie_entry     *ie_entry;

        ie_entry = &(app_ie_handle->entry[ftype]);

        *ielen = ie_entry->app_ie.length;

        /* verify output buffer is large enough */
        if (buflen < ie_entry->app_ie.length) {
            error = -EOVERFLOW;
            break;
        }

        /* copy app ie contents to output buffer */
        if (*ielen) {
            OS_MEMCPY(buf, ie_entry->app_ie.ie, ie_entry->app_ie.length);
        }
    } while ( false );

    IEEE80211_VAP_UNLOCK(vap);

    return error;
}

