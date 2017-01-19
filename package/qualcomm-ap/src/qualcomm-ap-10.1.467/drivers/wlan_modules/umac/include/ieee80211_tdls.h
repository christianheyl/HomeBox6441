/*
 *  Copyright (c) 2005 Atheros Communications Inc.  All rights reserved.
 */
/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */


#ifndef _IEEE80211_TDLS_SHIM_H_
#define _IEEE80211_TDLS_SHIM_H_

/* Used in either case, even if TDLS is not enabled, we check 
 * for this frame type, if it is we drop the frame. No business
 * for this frame if TDLS is disabled. 
 */
#define IEEE80211_TDLS_ETHERTYPE        0x890d

/* To represent MAC in string format xx:xx:xx:xx:xx:xx */
#define MACSTR_LEN 18

#define  IEEE80211_TDLS_PEER_UAPSD_BUF_STA    0x0001       /* Peer U-APSD buffer station flag */
#define  IEEE80211_TDLS_PEER_PSM              0x0002       /* Peer PSM flag */
#define  IEEE80211_TDLS_CHANNEL_SWITCH        0x0004       /* Channel switching flag */

#if UMAC_SUPPORT_TDLS

#define IEEE80211_TDLS_ATTACH(ic)               ieee80211_tdls_attach(ic)
#define IEEE80211_TDLS_DETACH(ic)               ieee80211_tdls_detach(ic)
#define IEEE80211_TDLS_IOCTL(vap, type, mac)    ieee80211_tdls_ioctl(vap, type, mac)
/* channel switch ioctls */
#define IEEE80211_TDLS_OFFCHANNEL_IOCTL(vap, mode, offChannel) ieee80211_tdls_set_off_channel_mode(vap, mode, offChannel)
#define IEEE80211_TDLS_CHN_SWITCH_TIME(vap, time) ieee80211_tdls_set_channel_switch_time(vap, time)
#define IEEE80211_TDLS_TIMEOUT(vap, timeout)       ieee80211_tdls_set_channel_switch_timeout(vap, timeout)

#define IEEE80211_TDLS_RCV_MGMT(ic, ni, skb)    ieee80211_tdls_recv_mgmt(ic, ni, skb)
#define IEEE80211_TDLS_SND_MGMT(vap, type, arg) ieee80211_tdls_send_mgmt(vap, type, arg)
#define IEEE80211_TDLS_MGMT_FRAME(llc)          (llc->llc_snap.ether_type == IEEE80211_TDLS_ETHERTYPE)
#define IEEE80211_TDLS_ENABLED(vap)             (vap->iv_ath_cap & IEEE80211_ATHC_TDLS)
#define IEEE80211_IS_TDLS_NODE(ni)              (ni->ni_flags & IEEE80211_NODE_TDLS)
#define IEEE80211_IS_TDLS_TIMEOUT(ni) \
	((ni->ni_tdls->state == IEEE80211_TDLS_S_TEARDOWN_INPROG))
#define IEEE80211_IS_TDLS_PEER_UAPSD_ENABLED(ni) (ni->ni_ic->ic_tdls->peer_uapsd_enable)
#define IEEE80211_IS_TDLS_CHANNEL_SWITCH_ENABLED(ni) (ni->ni_ic->ic_tdls->tdls_channel_switch_control)

#define TDLS_STATUS_WAKEUP_ALT  2  /* TDLS wakeup schedule rejected but alternative schedule provided */
#define TDLS_STATUS_WAKEUP_REJ  3 /* TDLS wakeup schedule rejected */
#define TDLS_STATUS_SECURITY_DISABLED   5 /* Security disabled */
#define TDLS_STATUS_LIFETIME   6  /* Unacceptable lifetime*/          
#define TDLS_STATUS_BSS_MISMATCH  7 /* Not in same BSS */
#define TDLS_STATUS_INVALID_RSNIE  72 /* Invalid contents of RSNIE */

#define FTIE_TYPE_LIFETIME_INTERVAL 2 /* TPK Keytype Lifetime */
#if CONFIG_RCPI
#define IEEE80211_IS_TDLSRCPI_PATH_SET(ni) (ni->ni_tdls->link_st)
#endif /* CONFIG_RCPI */
#define IEEE80211_IS_TDLS_SETUP_COMPLETE(ni)	(ni->ni_tdls->state >= IEEE80211_TDLS_S_RUN)
#define IEEE80211_IS_TDLS_PWR_MGT(ni)   (ni->ni_ic->ic_tdls->pwr_mgt_state)

/*
  * Mode for WiFi Alliance's ChSwitchMode command
*/
enum ieee80211_tdls_off_channel_cmd_mode {
     IEEE80211_TDLS_CHANNEL_SWITCH_CMD_MODE_OFF = 0,        /* off channel off: ignore requests and cannot initiate */
     IEEE80211_TDLS_CHANNEL_SWITCH_CMD_MODE_INITIATE,       /* STA initiates channel switch */
     IEEE80211_TDLS_CHANNEL_SWITCH_CMD_MODE_PASSIVE,        /* passive state where incoming channel switch requests are accept    ed */
     IEEE80211_TDLS_CHANNEL_SWITCH_CMD_MODE_REJECT_REQ,     /* neither initiate or respond */
     IEEE80211_TDLS_CHANNEL_SWITCH_CMD_MODE_UNSOLICITED,

     IEEE80211_TDLS_CHANNEL_SWITCH_CMD_MODE_COUNT,
};

/*
 * Channel Switch Control: Ignore(Off)/Accept/Reject Channel Switch Requests
*/
enum ieee80211_tdls_channel_channel_control {
     IEEE80211_TDLS_CHANNEL_SWITCH_CONTROL_OFF,          /* channel off: don't initiate channel switch and ignore requests */
     IEEE80211_TDLS_CHANNEL_SWITCH_CONTROL_ACCEPT,       /* accept channel switch requests */
     IEEE80211_TDLS_CHANNEL_SWITCH_CONTROL_REJECT,       /* reject channel switch requests */
};

/* 
 * Time (micro second) taken by the station (sends channel switch timing element) to switch channel.
*/
enum ieee80211_tdls_switch_time_limits {
     IEEE_TDLS_MIN_SWITCH_TIME = 20,
     IEEE_TDLS_MAX_SWITCH_TIME = 500,
};

/*
 * Time (micro second) for which station (sends channel swithc timing element) will wait for the 
 * first data packet exchange  to happen on off-channel before switching to base channel. 
 * This time is  measured from the last symbol on the ACK frame transmitted in response to the 
 * TDLS channel switch response frame.
*/
enum ieee80211_tdls_switch_timeout_limits {
     IEEE_TDLS_SWITCH_TIME_TIMEOUT_MIN = 200,
     IEEE_TDLS_SWITCH_TIME_TIMEOUT_MAX = 800,
};

/* TDLS frame format */
enum ieee80211_tdls_action {
    IEEE80211_TDLS_SETUP_REQ,
    IEEE80211_TDLS_SETUP_RESP,
    IEEE80211_TDLS_SETUP_CONFIRM,
    IEEE80211_TDLS_TEARDOWN,
    IEEE80211_TDLS_PEER_TRAFFIC_INDICATION,
    IEEE80211_TDLS_CHANNEL_SX_REQ,
    IEEE80211_TDLS_CHANNEL_SX_RESP,
    IEEE80211_TDLS_PEER_PSM_REQ,
    IEEE80211_TDLS_PEER_PSM_RESP,
    IEEE80211_TDLS_PEER_TRAFFIC_RESPONSE,
    IEEE80211_TDLS_DISCOVERY_REQ,
    IEEE80211_TDLS_DISCOVERY_RESP = 14,
    IEEE80211_TDLS_LEARNED_ARP,
#if CONFIG_RCPI
    IEEE80211_TDLS_TXPATH_SWITCH_REQ,
    IEEE80211_TDLS_TXPATH_SWITCH_RESP,
    IEEE80211_TDLS_RXPATH_SWITCH_REQ,
    IEEE80211_TDLS_RXPATH_SWITCH_RESP,
    IEEE80211_TDLS_LINKRCPI_REQ,
    IEEE80211_TDLS_LINKRCPI_REPORT,
#endif /* CONFIG_RCPI */
    IEEE80211_TDLS_MSG_MAX
};

struct ieee80211_tdls {
    /* Clients behind this node */
    ATH_LIST_HEAD(, ieee80211_tdlsarp) tdls_arp_hash[IEEE80211_NODE_HASHSIZE];
	int (*recv_mgmt)(struct ieee80211com *ic, 
	        struct ieee80211_node *ni, wbuf_t wbuf);
	int (*recv_null_data)(struct ieee80211com *ic, 
	        struct ieee80211_node *ni, wbuf_t wbuf);
    int tdls_enable;
#if CONFIG_RCPI
        struct timer_list tmrhdlr;
        u_int16_t    timer;
        u_int8_t     hithreshold;
        u_int8_t     lothreshold;
        u_int8_t     margin;
        u_int8_t     hi_tmp;
        u_int8_t     lo_tmp;
        u_int8_t     mar_tmp;
#endif /* CONFIG_RCPI */

    /* A list for all nodes (and state machines) */
    struct ieee80211vap *tdlslist_vap;       /* backpointer to vap */
    u_int16_t       tdls_count;
    rwlock_t        tdls_lock; 
    TAILQ_HEAD(TDLS_HEAD_TYPE, _wlan_tdls_sm) tdls_node;

    /*
     * functions for tdls
     */                               
    int    (*tdls_detach) (struct ieee80211com *);      
    int    (*tdls_wpaftie)(struct ieee80211vap *vap, u8 *buf, int len);
    int    (*tdls_recv)   (struct ieee80211com *ic, u8 *buf, int len);

    /* Peer U-APSD state */
    u_int8_t peer_uapsd_enable;
    /*Accept/Reject/Ignore Channel Switch Requests */
    u_int8_t tdls_channel_switch_control; 
    /* TDLS Peer U-APSD Indication Window */
    u_int8_t peer_uapsd_indication_window;
    u_int16_t response_timeout_msec;
    /* U-APSD TDLS node power management state */
    u_int8_t pwr_mgt_state;
};

#define ieee80211_tdlslist  ieee80211_tdls

typedef struct _wlan_tdls_sm *wlan_tdls_sm_t;

/* Stores node related state for TDLS */
struct ieee80211_tdls_node {
#define IEEE80211_TDLS_S_INIT                   0
#define IEEE80211_TDLS_S_SETUP_REQ_INPROG       1
#define IEEE80211_TDLS_S_SETUP_RESP_INPROG      2
#define IEEE80211_TDLS_S_SETUP_CONF_INPROG      3
#define IEEE80211_TDLS_S_TEARDOWN_INPROG        4
#define IEEE80211_TDLS_S_RUN                    6
#define IEEE80211_TDLS_S_DISCOVERY_REQ_INPROG   7  /* state flag for discovery req progress */
#define IEEE80211_TDLS_S_DISCOVERY_RESP_INPROG      8 /* state flag for discovery resp progress */
#if CONFIG_RCPI
#define IEEE80211_TDLS_S_TXSWITCH_REQ_INPROG    7
#define IEEE80211_TDLS_S_TXSWITCH_RESP_INPROG   8 
#define IEEE80211_TDLS_S_RXSWITCH_REQ_INPROG    9
#define IEEE80211_TDLS_S_RXSWITCH_RESP_INPROG   10 
#define IEEE80211_TDLS_S_LINKRCPI_REQ_INPROG    11 
#define IEEE80211_TDLS_S_LINKRCPI_REPORT_INPROG 12 
    u_int8_t    link_st;
#endif /* CONFIG_RCPI */
#define IEEE80211_TDLS_S_PEER_TRAFFIC_INDICATION_INPROG 13
#define IEEE80211_TDLS_S_PEER_TRAFFIC_RESPONSE_INPROG   14
    u_int8_t    stk_st;
    u_int8_t    token;
    u_int8_t    lnkid;
    u_int8_t    state;
    u_int8_t    is_initiator;
    u_int16_t    capinfo;
    
    /* TDLS-specific flags */
    u_int16_t   flags;
    u_int8_t    wme_info_uapsd;
    wlan_tdls_sm_t w_tdls;
#if ATH_TDLS_AUTO_CONNECT
    u_int8_t    last_disc_resp_rssi;
    u_int8_t    is_last_weak;
    u_int8_t    peer_miss_count;
    systime_t   tstamp;
#endif
};

enum ieee80211_tdls_peer_uapsd_enable {
    TDLS_PEER_UAPSD_DISABLE = 0,
    TDLS_PEER_UAPSD_ENABLE,
};

static INLINE void
ieee80211tdls_set_flag(struct ieee80211_node *ni, u_int16_t flag)
{
    ni->ni_tdls->flags |= flag;
}

static INLINE void
ieee80211tdls_clear_flag(struct ieee80211_node *ni, u_int16_t flag)
{
    ni->ni_tdls->flags &= ~flag;
}

static INLINE int
ieee80211tdls_has_flag(struct ieee80211_node *ni, u_int16_t flag)
{
    return ((ni->ni_tdls->flags & flag) != 0);
}

int ieee80211_tdls_attach(struct ieee80211com *ic);
int ieee80211_tdls_detach(struct ieee80211com *ic);
int ieee80211_tdls_ioctl(struct ieee80211vap *vap, int type, char *mac);

int ieee80211_tdls_set_off_channel_mode(struct ieee80211vap *vap, int mode, struct ieee80211_channel *channel);
int ieee80211_tdls_set_channel_switch_time(struct ieee80211vap *vap, int switchTime);
int ieee80211_tdls_set_channel_switch_timeout(struct ieee80211vap *vap, int timeout);

int ieee80211_tdls_send_mgmt(struct ieee80211vap *vap, u_int32_t type, void *arg);
int ieee80211_tdls_recv_mgmt(struct ieee80211com *ic, struct ieee80211_node *ni,
                              struct sk_buff *skb);
int ieee80211_tdls_recv_null_data(struct ieee80211com *ic, struct ieee80211_node *ni, struct sk_buff *skb);
/* The function is used  to set the MAC address  of a device set via iwpriv */
void ieee80211_tdls_set_mac_addr(u_int8_t *mac_address, 
                              u_int32_t word1, u_int32_t word2);
int ieee80211_ioctl_set_tdls_rmac(struct net_device *dev,
                              void *info, void *w, char *extra);
int ieee80211_ioctl_clr_tdls_rmac(struct net_device *dev,
                              void *info, void *w, char *extra);
int ieee80211_ioctl_tdls_qosnull(struct net_device *dev,
                              void *info, void *w, char *extra, int ac);
int ieee80211_wpa_tdls_ftie(struct ieee80211vap *vap, u8 *buf, int len);

struct ieee80211_node *
ieee80211_find_tdlsnode(struct ieee80211vap *vap, const u_int8_t *macaddr);

int ieee80211_ioctl_set_tdls_peer_uapsd_enable(struct net_device *dev, enum ieee80211_tdls_peer_uapsd_enable flag);

enum ieee80211_tdls_peer_uapsd_enable
ieee80211_ioctl_get_tdls_peer_uapsd_enable(struct net_device *dev);

void 
ieee80211tdls_add_extcap(struct ieee80211_node *ni, u_int32_t *ext_capflags);

void
ieee80211tdls_process_extcap_ie(struct ieee80211_node *ni, u_int32_t extcap);

void
ieee80211_tdls_pwrsave_check(struct ieee80211_node *ni, struct ieee80211_frame *wh);

int 
tdls_send_discovery_req(struct ieee80211com *ic, struct ieee80211vap *vap,
                                void* arg, uint8_t *buf, uint16_t buf_len, u_int16_t dialog_token);

void
ieee80211tdls_peer_uapsd_pending_xmit(struct ieee80211_node *ni, wbuf_t wbuf);

void
ieee80211_tdls_peer_uapsd_force_sleep(struct ieee80211vap *vap, bool enable);

#if 0 & UMAC_SUPPORT_TDLS
void wlan_tdls_teardown_complete_handler(wlan_if_t vaphandle, wbuf_t wbuf, 
		ieee80211_xmit_status *ts);
#endif /* UMAC_SUPPORT_TDLS */

/*  
 * tdls ops
 */

typedef struct ieee80211_tdls ieee80211_tdls_ops_t;

#else /* UMAC_SUPPORT_TDLS */

#define IEEE80211_TDLS_MGMT_FRAME(llc) (0)
#define IEEE80211_TDLS_ENABLED(vap) (0)
#define IEEE80211_IS_TDLS_NODE(ni) (0)
#define IEEE80211_IS_TDLS_SETUP_COMPLETE(ni) (0)
#define IEEE80211_IS_TDLS_PWR_MGT(ni) (0)
#define IEEE80211_IS_TDLS_TIMEOUT(ni) (0)
#define IEEE80211_TDLS_SND_MGMT(vap, type, arg) 
#define IEEE80211_TDLS_RCV_MGMT(vap, type, arg) 
#define IEEE80211_TDLS_ATTACH(ic) 
#define IEEE80211_TDLS_DETACH(ic) 
#define IEEE80211_TDLS_IOCTL(vap, type, mac)

#define IEEE80211_TDLS_OFFCHANNEL_IOCTL(vap, mode, offChannel) 
#define IEEE80211_TDLS_CHN_SWITCH_TIME(vap, time) 
#define IEEE80211_TDLS_TIMEOUT(vap, timeout) 

#if CONFIG_RCPI
#define IEEE80211_IS_TDLSRCPI_PATH_SET(ni) (0)
#endif /* CONFIG_RCPI */
#define IEEE80211_IS_TDLS_PEER_UAPSD_ENABLED(ni) (0)
#define IEEE80211_IS_TDLS_CHANNEL_SWITCH_ENABLED(ni) (0)

static INLINE void
ieee80211tdls_set_flag(struct ieee80211_node *ni, u_int16_t flag) { return; }

static INLINE void
ieee80211tdls_clear_flag(struct ieee80211_node *ni, u_int16_t flag) { return; }

static INLINE int
ieee80211tdls_has_flag(struct ieee80211_node *ni, u_int16_t flag) { return (0); }

static INLINE void 
ieee80211tdls_add_extcap(struct ieee80211_node *ni, u_int32_t *ext_capflags) { return; }

static INLINE void
ieee80211tdls_process_extcap_ie(struct ieee80211_node *ni, u_int32_t extcap) { return; }

static INLINE void
ieee80211_tdls_pwrsave_check(struct ieee80211_node *ni, struct ieee80211_frame *wh) { return; }

static INLINE void
ieee80211tdls_peer_uapsd_pending_xmit(struct ieee80211_node *ni, wbuf_t wbuf) { return; }

static INLINE void
ieee80211_tdls_peer_uapsd_force_sleep(struct ieee80211vap *vap, bool enable) { return; }

#endif /* UMAC_SUPPORT_TDLS */

#endif /* _IEEE80211_TDLS__SHIM_H_ */
