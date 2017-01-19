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

#ifndef IEEE80211_TDLS_POWER_H_
#define IEEE80211_TDLS_POWER_H_

#if UMAC_SUPPORT_TDLS

#if UMAC_SUPPORT_TDLS_PEER_UAPSD

/*
 * State machine states
 */
typedef enum {
    IEEE80211_TDLS_PU_BUF_STA_STATE_IDLE = 0,
    IEEE80211_TDLS_PU_BUF_STA_STATE_NOTIFY,
    IEEE80211_TDLS_PU_BUF_STA_STATE_WAIT,
    IEEE80211_TDLS_PU_BUF_STA_STATE_TEARDOWN,
} ieee80211_tdls_pu_buf_sta_state;

/*
 * State machine states
 */
typedef enum {
    IEEE80211_TDLS_PU_SLEEP_STA_STATE_IDLE = 0,
    IEEE80211_TDLS_PU_SLEEP_STA_STATE_ACTIVE,
    IEEE80211_TDLS_PU_SLEEP_STA_STATE_NULL_SENT,
    IEEE80211_TDLS_PU_SLEEP_STA_STATE_SLEEP,
} ieee80211_tdls_pu_sleep_sta_state;

static INLINE int ieee80211tdls_pu_buf_sta_sm_dispatch(peer_uapsd_buf_sta_t *buf_sta, u_int16_t event, 
                                                                   ieee80211tdls_pu_buf_sta_sm_event_t *event_data) 
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
    ieee80211_sm_dispatch(buf_sta->hsm_handle, event, sizeof(struct ieee80211tdls_pu_buf_sta_sm_event), event_data);
    return EOK;
}

static INLINE int ieee80211tdls_pu_sleep_sta_sm_dispatch(peer_uapsd_sleep_sta_t *sleep_sta, u_int16_t event, 
                                                                   ieee80211tdls_pu_sleep_sta_sm_event_t *event_data) 
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
    ieee80211_sm_dispatch(sleep_sta->hsm_handle, event, sizeof(struct ieee80211tdls_pu_sleep_sta_sm_event), event_data);
    return EOK;
}

static INLINE int ieee80211tdls_pu_buf_sta_set_dialog_token(peer_uapsd_buf_sta_t *buf_sta, u_int8_t dialog_token)
{
    buf_sta->dialog_token = dialog_token;
    return(EOK);
}

static INLINE u_int8_t ieee80211tdls_pu_buf_sta_get_dialog_token(peer_uapsd_buf_sta_t *buf_sta)
{
    return(buf_sta->dialog_token);
}

static INLINE int ieee80211tdls_pu_sleep_sta_set_dialog_token(peer_uapsd_sleep_sta_t *sleep_sta, u_int8_t dialog_token)
{
    sleep_sta->dialog_token = dialog_token;
    return(EOK);
}

static INLINE u_int8_t ieee80211tdls_pu_sleep_sta_get_dialog_token(peer_uapsd_sleep_sta_t *sleep_sta)
{
    return(sleep_sta->dialog_token);
}

void
ieee80211tdls_peer_uapsd_sm_create(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls);

void
ieee80211tdls_peer_uapsd_sm_delete(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls);

/* Initialize the PU Buffer STA */
static void
tdls_pu_init_buf_sta(struct ieee80211_node *ni, peer_uapsd_buf_sta_t *buf_sta);

/* Initialize the PU Sleep STA */
static void
tdls_pu_init_sleep_sta(struct ieee80211_node *ni, peer_uapsd_sleep_sta_t *sleep_sta);

/* Cleanup the PU Buffer STA */
static void
tdls_pu_cleanup_buf_sta(struct ieee80211_node *ni, peer_uapsd_buf_sta_t *buf_sta);

/* Cleanup the PU Sleep STA */
static void
tdls_pu_cleanup_sleep_sta(struct ieee80211_node *ni, peer_uapsd_sleep_sta_t *sleep_sta);

#else

static INLINE void
ieee80211tdls_peer_uapsd_sm_create(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls) { return; }

static INLINE void
ieee80211tdls_peer_uapsd_sm_delete(struct ieee80211_node *ni, wlan_tdls_sm_t w_tdls) { return; }

static INLINE int
ieee80211tdls_pu_buf_sta_sm_dispatch(peer_uapsd_buf_sta_t *buf_sta, u_int16_t event,
                                         ieee80211tdls_pu_buf_sta_sm_event_t *event_data) { return EOK; }

static INLINE int
ieee80211tdls_pu_sleep_sta_sm_dispatch(peer_uapsd_sleep_sta_t *sleep_sta, u_int16_t event,
                                         ieee80211tdls_pu_sleep_sta_sm_event_t *event_data) { return EOK; }
static INLINE int
ieee80211tdls_pu_buf_sta_set_dialog_token(peer_uapsd_buf_sta_t *buf_sta, u_int8_t dialog_token)  { return EOK; }

static INLINE u_int8_t 
ieee80211tdls_pu_buf_sta_get_dialog_token(peer_uapsd_buf_sta_t *buf_sta)  { return EOK; }

static INLINE int 
ieee80211tdls_pu_sleep_sta_set_dialog_token(peer_uapsd_sleep_sta_t *sleep_sta, u_int8_t dialog_token)  { return EOK; }

static INLINE u_int8_t ieee80211tdls_pu_sleep_sta_get_dialog_token(peer_uapsd_sleep_sta_t *sleep_sta)  { return EOK; }

#endif /* UMAC_SUPPORT_TDLS_PEER_UAPSD */

#endif /* UMAC_SUPPORT_TDLS */

#endif /*IEEE80211_TDLS_POWER_H_*/
