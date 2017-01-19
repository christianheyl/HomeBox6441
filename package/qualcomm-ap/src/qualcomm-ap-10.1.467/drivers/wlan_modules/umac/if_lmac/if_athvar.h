/*
 * Copyright (c) 2010, Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */

/*
 * Defintions for the Atheros Wireless LAN controller driver.
 */
#ifndef _DEV_ATH_ATHVAR_H
#define _DEV_ATH_ATHVAR_H

#include <osdep.h>
#include "ieee80211_channel.h"
#include "ieee80211_proto.h"
#include "ieee80211_rateset.h"
#include "ieee80211_regdmn.h"
#include "ieee80211_target.h"
#include "ieee80211_wds.h"
#include "ieee80211_resmgr_oc_scheduler.h"

#include "asf_amem.h"
#include "adf_os_lock.h"
#include "ath_dev.h"
#include "ath_internal.h"

#ifndef REMOVE_PKT_LOG
#include "pktlog.h"
#endif

#ifdef ATH_SUPPORT_HTC
struct ath_usb_p2p_action_queue {
    wbuf_t  wbuf;
    struct ath_usb_p2p_action_queue *next;
    int deleted;
};

struct ath_swba_data {
    atomic_t                flags;
    a_uint64_t              currentTsf;
    a_uint8_t               beaconPendingCount;
};
#endif
 
struct ath_cwm;
#if (UMAC_SUPPORT_TDLS == 1) && (ATH_TDLS_AUTO_CONNECT == 1)
enum off_table_status {
	status_idle = 0,
    status_discovery = 1,
	status_normal = 2,
	status_delay = 3,
	status_connect = 4,
    status_timeout = 5,
};
struct discovery_addr {
    u_int8_t                    mac_addr[IEEE80211_ADDR_LEN];
    struct ieee80211vap         *vap;
    os_timer_t  	            discovery_timer;
};
struct off_mac_addr {
    u_int8_t                    mac_addr[IEEE80211_ADDR_LEN];
    os_timer_t  	            tdls_timer;
    u_int8_t                    get_discovery_response;
    u_int8_t                    response_rssi_1;
    u_int8_t                    response_rssi_2;
    struct discovery_addr       discovery_one;
    enum off_table_status       status;
    struct ath_softc_net80211   *scn;
    struct ieee80211vap         *vap;
};
#define MAX_TDLS_TRY_NUM	4
#define TDLS_RECOVER_TIMEOUT     1*1000
#define TDLS_DISCOVERY_TIMEOUT   5*1000
#define	TDLS_NORMAL_TIMEOUT 	 1*1000
#define TDLS_DELAY_TIMEOUT      20*1000
#define TDLS_CONNECT_TIMEOUT    15*1000
#define TDLS_OFF_TABLE_TIMEOUT	60
struct tdls_off_table {
    u_int8_t		try_count;
    struct off_mac_addr	try_mac_addr[MAX_TDLS_TRY_NUM];
};

#define MAX_TEARDOWN_BLOCK_NUM 16
#define TD_BLOCK_TIMEOUT       300
struct teardown_block_mac_addr {
    u_int8_t        mac_addr[IEEE80211_ADDR_LEN];
    os_timer_t  	teardown_block_timer;
    u_int8_t        is_occupied;
    struct ath_softc_net80211   *scn;
    systime_t       tstamp;
};
struct teardown_block_table {
    u_int8_t		block_count;
    struct teardown_block_mac_addr	block_mac_addr[MAX_TEARDOWN_BLOCK_NUM];
};

#define MAX_TDLS_INCAPABLE_NUM 32
#define TDLS_MISS_COUNT_LIMIT  3
struct tdls_incapable_mac_addr {
    u_int8_t        mac_addr[IEEE80211_ADDR_LEN];
    u_int8_t        miss_couunt;
    systime_t       tstamp;
};
struct tdls_incapable_table {
    u_int8_t        incapble_count;
    struct tdls_incapable_mac_addr  incapable_mac_addr[MAX_TDLS_INCAPABLE_NUM];
};

#define MAX_WEAK_PEER_NUM 16
#define WEAK_PEER_TIMEOUT 300
#define TDLS_SETUP_RSSI_MARGIN 10
#define TDLS_SETUP_RECOVER_RSSI 70
#define TDLS_SETUP_RSSI_UPPER_BOUNDARY 40
#define TDLS_SETUP_RSSI_LOWER_BOUNDARY 20
#define TDLS_SETUP_RSSI_OFFSET 5
#define TDLS_SETUP_PATH_SEL_PERIOD 60
struct tdls_weak_peer_mac_addr {
    u_int8_t        mac_addr[IEEE80211_ADDR_LEN];
    os_timer_t  	weak_block_timer;
    u_int8_t        is_occupied;
    struct ath_softc_net80211   *scn;
    u_int8_t        rssi_1;
    u_int8_t        rssi_2;
    systime_t       tstamp;
};
struct tdls_weak_peer_table {
    u_int8_t        weak_peer_count;
    struct tdls_weak_peer_mac_addr weak_peer_mac_addr[MAX_WEAK_PEER_NUM];
};

#endif /* end of (UMAC_SUPPORT_TDLS == 1) && (ATH_TDLS_AUTO_CONNECT == 1) */
struct ath_softc_net80211 {
    struct ieee80211com     sc_ic;      /* NB: base class, must be first */
    struct pktlog_handle_t *pl_dev;     /* Must be in the second position */
    ath_dev_t               sc_dev;     /* Atheros device handle */
    struct ath_ops          *sc_ops;    /* Atheros device callback table */

    void                    (*sc_node_cleanup)(struct ieee80211_node *);
    void                    (*sc_node_free)(struct ieee80211_node *);

    osdev_t                 sc_osdev;   /* handle to use OS-independent services */

    int                     sc_nstavaps; /* # of station vaps */
    u_int                   sc_syncbeacon:1,    /* sync/resync beacon timers */
                            sc_isscan:1,        /* scanning */
                            sc_splitmic:1,      /* split TKIP MIC keys */
                            sc_mcastkey:1;      /* mcast key cache search */
    u_int32_t               sc_prealloc_idmask;   /* preallocated vap id bitmap: can only support 32 vaps */
    atomic_t                sc_dev_enabled;    /* dev is enabled */ 

    int                     sc_ac2q[WME_NUM_AC];/* WMM AC -> h/w qnum */
    int                     sc_uapsd_qnum;      /* UAPSD h/w qnum */
    int                     sc_beacon_qnum;     /* beacon h/w qnum */
    u_int32_t               cached_ic_flags;    
    
    struct ieee80211_node	*sc_keyixmap[ATH_KEYMAX];/* key ix->node map */
    
    struct ath_cwm          *sc_cwm;            /* Channel Width Management */
    TAILQ_HEAD(ath_amsdu_txq,ath_amsdu_tx)    sc_amsdu_txq;    /* amsdu tx requests */
    struct ath_timer        sc_amsdu_flush_timer; /* amsdu flush timer */
    spinlock_t              amsdu_txq_lock;    /* amsdu spinlock */
    spinlock_t              sc_keyixmap_lock;  /* keyix map spinlock */
    unsigned long           sc_keyixmap_lock_flags;
    u_int                   sc_wow_enabled;
    u_int                   sc_wow_wake_from_wow;
#ifdef ATH_SUPPORT_HTC
    u_int                   sc_htc_wmm_update_enabled;
    struct ath_usb_p2p_action_queue *sc_p2p_action_queue_head;
    struct ath_usb_p2p_action_queue *sc_p2p_action_queue_tail;
    spinlock_t              p2p_action_queue_lock;
    unsigned long           p2p_action_queue_flags;
    struct ath_swba_data    sc_htc_swba_data;
    u_int32_t               sc_htc_nodekickmask;
    u_int32_t               sc_htc_txRateKbps;  /* per-device xmit rate updated by WMI_TXRATE_EVENTID event */
    int                     sc_htc_delete_in_progress; /* flag to indicate delete in progress */
#endif

    struct {
        asf_amem_instance_handle handle;
        adf_os_spinlock_t        lock;
    } amem;
#if (UMAC_SUPPORT_TDLS == 1) && (ATH_TDLS_AUTO_CONNECT == 1)
    struct tdls_off_table       sc_tdls_off_table;
    struct teardown_block_table sc_teardown_block_table;
    struct tdls_incapable_table sc_incapable_table;
    struct tdls_weak_peer_table sc_weak_peer_table;
    os_timer_t  	sc_tdls_path_select_timer;
#endif
#if ATH_SUPPORT_WIFIPOS     
    int                     sc_wifipos_qnum;     /* wifipos h/w qnum */
#endif
};
#define ATH_SOFTC_NET80211(_ic)     ((struct ath_softc_net80211 *)(_ic))

struct ath_vap_net80211 {
    struct ieee80211vap         av_vap;     /* NB: base class, must be first */
    struct ath_softc_net80211   *av_sc;     /* back pointer to softc */
    int                         av_if_id;   /* interface id */

    struct ieee80211_beacon_offsets av_beacon_offsets;

};
#define ATH_VAP_NET80211(_vap)      ((struct ath_vap_net80211 *)(_vap))

#define ATH_P2PDEV_IF_ID 1

/* 
 * Units in kbps for the an_lasttxrate and an_lastrxrate fields. These 
 * two fields are only 16 bits wide and the max. rate is 600,000 kbps.
 * LAST_RATE_UNITS is used to scale these two fields to fit into 16 bits.
 */
#define LAST_RATE_UNITS     16

struct ath_node_net80211 {
    struct ieee80211_node       an_node;     /* NB: base class, must be first */
    ath_node_t                  an_sta;
    struct ath_ff               *an_ff;       /* fast frame buf */
    struct ath_amsdu            *an_amsdu;    /* AMSDU frame buf */
};

#if event-mechanism
enum ath_event_type {
    ATH_DFS_WAIT_CLEAR_EVENT,
    ATH_END_EVENT,
};

typedef struct ath_net80211_event {

    enum ath_event_type ath_event_id;
    void *ath_event_data;
} ATH_NET80211_EVENT;
#endif

#define ATH_NODE_NET80211(_ni)      ((struct ath_node_net80211 *)(_ni))

#define NET80211_HANDLE(_ieee)      ((struct ieee80211com *)(_ieee))

#define ATH_TSF_TIMER(_tsf_timer)   ((struct ath_gen_timer *)(_tsf_timer))
#define NET80211_TSF_TIMER(_tsf_timer)  \
                                    ((struct ieee80211_tsf_timer *)(_tsf_timer))
#define ATH_TSF_TIMER_FUNC(_tsf_timer_func)  \
                                    ((ath_hwtimer_function)(_tsf_timer_func))

#define ATH_BCN_NOTIFY_FUNC(_tx_beacon_notify_func)  \
                                    ((ath_notify_tx_beacon_function)(_tx_beacon_notify_func))

#define ATH_VAP_NOTIFY_FUNC(_vap_change_notify_func)  \
                                    ((ath_vap_info_notify_func)(_vap_change_notify_func))

int ath_attach(u_int16_t devid, void *base_addr, struct ath_softc_net80211 *scn, osdev_t osdev,
               struct ath_reg_parm *ath_conf_parm, struct hal_reg_parm *hal_conf_parm, IEEE80211_REG_PARAMETERS *ieee80211_conf_parm);
int ath_detach(struct ath_softc_net80211 *scn);
int ath_resume(struct ath_softc_net80211 *scn);
int ath_suspend(struct ath_softc_net80211 *scn);
int ath_vap_down(struct ieee80211vap *vap);
int ath_vap_dfs_cac(struct ieee80211vap *vap);
int ath_vap_stopping(struct ieee80211vap *vap);
int ath_net80211_rx(ieee80211_handle_t ieee, wbuf_t wbuf, ieee80211_rx_status_t *rx_status, u_int16_t keyix);
int ath_get_netif_settings(ieee80211_handle_t);
void ath_mcast_merge(ieee80211_handle_t, u_int32_t mfilt[2]);
int ath_tx_send(wbuf_t wbuf);
int ath_tx_mgt_send(struct ieee80211com *ic, wbuf_t wbuf);
int ath_tx_prepare(struct ath_softc_net80211 *scn, wbuf_t wbuf, int nextfraglen, ieee80211_tx_control_t *txctl);
WIRELESS_MODE ath_ieee2wmode(enum ieee80211_phymode mode);
u_int32_t ath_set_ratecap(struct ath_softc_net80211 *scn, struct ieee80211_node *ni,
                         struct ieee80211vap *vap);
//int16_t ath_net80211_get_noisefloor(struct ieee80211com *ic, struct ieee80211_channel *chan);
void ath_net80211_dfs_test_return(ieee80211_handle_t ieee, u_int8_t ieeeChan);
void ath_net80211_mark_dfs(ieee80211_handle_t, struct ieee80211_channel *);
void ath_net80211_change_channel(ieee80211_handle_t ieee, struct ieee80211_channel *chan);
void ath_net80211_switch_mode_static20(ieee80211_handle_t ieee);
void ath_net80211_switch_mode_dynamic2040(ieee80211_handle_t ieee);
int  ath_net80211_input(ieee80211_node_t node, wbuf_t wbuf, ieee80211_rx_status_t *rx_status);
#if defined(MAGPIE_HIF_GMAC) || defined(MAGPIE_HIF_USB)
u_int32_t ath_net80211_htc_node_getrate(const struct ieee80211_node *ni, u_int8_t type);
#endif

void ath_net80211_bf_rx(struct ieee80211com *ic, wbuf_t wbuf, ieee80211_rx_status_t *status);

static INLINE void    
ath_node_set_fixedrate_proc(struct ath_softc_net80211 *scn,
                            const u_int8_t macaddr[IEEE80211_ADDR_LEN],
                            u_int8_t fixed_ratecode)
{   
    struct ieee80211_node *ni;
    struct ath_node *an;
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    struct ieee80211com *ic = (struct ieee80211com *)sc->sc_ieee;

#ifdef IEEE80211_DEBUG_REFCNT
    if (!(ic->ic_ieee80211_find_node_debug && ic->ic_ieee80211_unref_node)) {
        printk("%s: ic->ic_ieee80211_find_node_debug %p ic->ic_ieee80211_unref_node %p\n",
            __func__, ic->ic_ieee80211_find_node_debug, ic->ic_ieee80211_unref_node);
        return;
    }
    ni = ic->ic_ieee80211_find_node_debug(&ic->ic_sta, macaddr);
#else
    if (!(ic->ic_ieee80211_find_node && ic->ic_ieee80211_unref_node)) {
        printk("%s: ic->ic_ieee80211_find_node %p ic->ic_ieee80211_unref_node %p\n",
            __func__, ic->ic_ieee80211_find_node, ic->ic_ieee80211_unref_node);
        return;
    }
    ni = ic->ic_ieee80211_find_node(&ic->ic_sta, macaddr);
#endif
    if (!ni) {
        printk("%s: No node found!\n", __func__);
        return;
    }

    an = (struct ath_node *)((ATH_NODE_NET80211(ni))->an_sta);
    if (an && sc->sc_ath_ops.node_setfixedrate)
        sc->sc_ath_ops.node_setfixedrate(an, fixed_ratecode);
    /* 
     * find_node has incremented the node reference count. 
     * take care of that 
     */
    ic->ic_ieee80211_unref_node (ni);
    return;
}

#ifdef ATHR_RNWF
#define ATH_DEFINE_TXCTL(_txctl, _wbuf)    \
    ieee80211_tx_control_t *_txctl = (ieee80211_tx_control_t *)wbuf_get_context(_wbuf)
#else
#define ATH_DEFINE_TXCTL(_txctl, _wbuf)    \
    ieee80211_tx_control_t ltxctl;         \
    ieee80211_tx_control_t *_txctl = &ltxctl
#endif
#if ATH_SUPPORT_CFEND
wbuf_t ath_net80211_cfend_alloc(ieee80211_handle_t ieee);
#endif
#if UMAC_SUPPORT_OPMODE_APONLY
#define ath_net80211_rx_monitor(_ic, _wbuf, _rxs)
#else
void ath_net80211_rx_monitor(struct ieee80211com *ic, wbuf_t wbuf, ieee80211_rx_status_t *rx_status);
#endif
#if (UMAC_SUPPORT_TDLS == 1) && (ATH_TDLS_AUTO_CONNECT == 1)
void ath_tdls_teardown_block_check(struct ieee80211com *ic, struct ieee80211_node *ni_tdls, char * sa);
#endif

#ifdef ATH_SUPPORT_TxBF
#define ath_update_txbf_tx_status( _ts, _tx_status)         \
        do {                                                \
            (_ts).ts_txbfstatus = (_tx_status)->txbf_status;\
            (_ts).ts_tstamp     = (_tx_status)->tstamp;     \
        } while (0)
        
#define ath_txbf_update_rx_status( _rs, _rx_status) \
        (_rs)->rs_rpttstamp = (_rx_status)->rpttstamp
#else
#define ath_update_txbf_tx_status(_ts, _tx_status)
#define ath_txbf_update_rx_status(_rs, _rx_status)
#endif
#endif /* _DEV_ATH_ATHVAR_H  */
