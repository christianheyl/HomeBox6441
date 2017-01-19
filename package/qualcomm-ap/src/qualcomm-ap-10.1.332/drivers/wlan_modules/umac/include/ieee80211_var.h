/*
 *  Copyright (c) 2008 Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */

#ifndef _ATH_STA_IEEE80211_VAR_H
#define _ATH_STA_IEEE80211_VAR_H

/*
 * Emulation layer of net80211 module. In current NDIS driver implementation,
 * it acts as a shim layer between ATH layer and sample driver's STATION layer.
 */

#include <wlan_opts.h>
#include <ieee80211_options.h>
#include <_ieee80211.h>
#include <ieee80211.h>
#include <ieee80211_api.h>
#include <ieee80211_crypto.h>
#include <ieee80211_crypto_wep_mbssid.h>
#include <ieee80211_node.h>
#include <ieee80211_proto.h>
#include <ieee80211_power.h>
#include <ieee80211_config.h>
#include <ieee80211_mlme.h>
#include <ieee80211_data.h>
#include <ieee80211_vap.h>
#include <ieee80211_vap_pause.h>
#include <ieee80211_resmgr.h>
#include <ieee80211_acs.h>
#include <ieee80211_ique.h>
#include <ieee80211_acl.h>
#include <ieee80211_aow.h>
#include <ieee80211_ald.h>
#include <ieee80211_notify_tx_bcn.h>
#include <ieee80211_vap_ath_info.h>
#include <ieee80211_tsftimer.h>
#include <ieee80211_mlme_app_ie.h>
#include <ieee80211_rptplacement.h> 
#include <ieee80211_wifipos.h>
#include <ieee80211_tdls.h> 
#include <ieee80211_wps.h>  
#include <ieee80211_admctl.h>
#include <ieee80211_rrm_proto.h>
#include <ieee80211_wnm.h>
#include <ieee80211_quiet.h>
#include <asf_print.h>
#include <ieee80211_smartantenna.h>
#include <ieee80211_prdperfstats.h>
#include <ieee80211_dfs.h>
#include <adf_os_types.h>

#ifdef ATH_SUPPORT_DFS
#include "ath_dfs_structs.h"
#endif

#ifdef ATH_SUPPORT_TxBF
#include <ieee80211_txbf.h>
#endif

#if ATH_SW_WOW
#include <ieee80211_wow.h>
#endif
/*
 * NOTE: Although we keep the names of data structures (ieee80211com
 * and ieee80211vap), each OS will have different implementation of these
 * data structures. Linux/BSD driver will have the full implementation,
 * while in NDIS6.0 driver, this is just a glue between STATION layer
 * and ATH layer. ATH layer should use API's to access ic, iv, and ni,, * instead of referencing the pointer directly.
 */

#include <sys/queue.h>

#define IEEE80211_TXPOWER_MAX        100   /* .5 dbM (XXX units?) */
#define IEEE80211_TXPOWER_MIN        0     /* kill radio */

#define IEEE80211_DTIM_MAX           255   /* max DTIM period */
#define IEEE80211_DTIM_MIN           1     /* min DTIM period */
#define IEEE80211_DTIM_DEFAULT       1     /* default DTIM period */

#define IEEE80211_LINTVAL_MAX        10   /* max listen interval */
#define IEEE80211_LINTVAL_MIN        1    /* min listen interval */
#define IEEE80211_LINTVAL_DEFAULT    1    /* default listen interval */

#define IEEE80211_BINTVAL_MAX        10000 /* max beacon interval (TU's) */
#define IEEE80211_BINTVAL_MIN        10    /* min beacon interval (TU's) */

#if ATH_SUPPORT_AP_WDS_COMBO
#define IEEE80211_BINTVAL_DEFAULT    400   /* default beacon interval (TU's) for 16 vap support */
#else
#define IEEE80211_BINTVAL_DEFAULT    100   /* default beacon interval (TU's) */
#endif

#define LIMIT_BEACON_PERIOD(_intval)                \
    do {                                            \
        if ((_intval) > IEEE80211_BINTVAL_MAX)      \
            (_intval) = IEEE80211_BINTVAL_MAX;      \
        else if ((_intval) < IEEE80211_BINTVAL_MIN) \
            (_intval) = IEEE80211_BINTVAL_MIN;      \
    } while (FALSE)

#define LIMIT_DTIM_PERIOD(_intval)                \
    do {                                            \
        if ((_intval) > IEEE80211_DTIM_MAX)      \
            (_intval) = IEEE80211_DTIM_MAX;      \
        else if ((_intval) < IEEE80211_DTIM_MIN) \
            (_intval) = IEEE80211_DTIM_MIN;      \
    } while (FALSE)

#define LIMIT_LISTEN_INTERVAL(_intval)                \
    do {                                            \
        if ((_intval) > IEEE80211_LINTVAL_MAX)      \
            (_intval) = IEEE80211_LINTVAL_MAX;      \
        else if ((_intval) < IEEE80211_LINTVAL_MIN) \
            (_intval) = IEEE80211_LINTVAL_MIN;      \
    } while (FALSE)

/* Definitions for valid VHT MCS map */
#define VHT_MCSMAP_NSS1_MCS0_7  0xfffc  /* NSS=1 MCS 0-7 */
#define VHT_MCSMAP_NSS2_MCS0_7  0xfff0  /* NSS=2 MCS 0-7 */
#define VHT_MCSMAP_NSS3_MCS0_7  0xffc0  /* NSS=3 MCS 0-7 */
#define VHT_MCSMAP_NSS1_MCS0_8  0xfffd  /* NSS=1 MCS 0-8 */
#define VHT_MCSMAP_NSS2_MCS0_8  0xfff5  /* NSS=2 MCS 0-8 */
#define VHT_MCSMAP_NSS3_MCS0_8  0xffd5  /* NSS=3 MCS 0-8 */
#define VHT_MCSMAP_NSS1_MCS0_9  0xfffe  /* NSS=1 MCS 0-9 */
#define VHT_MCSMAP_NSS2_MCS0_9  0xfffa  /* NSS=2 MCS 0-9 */
#define VHT_MCSMAP_NSS3_MCS0_9  0xffea  /* NSS=3 MCS 0-9 */

/* Definitions for valid VHT MCS mask */
#define VHT_MCSMAP_NSS1_MASK   0xfffc   /* Single stream mask */
#define VHT_MCSMAP_NSS2_MASK   0xfff0   /* Dual stream mask */
#define VHT_MCSMAP_NSS3_MASK   0xffc0   /* Tri stream mask */

#define IEEE80211_BGSCAN_INTVAL_MIN            15  /* min bg scan intvl (secs) */
#define IEEE80211_BGSCAN_INTVAL_DEFAULT    (5*60)  /* default bg scan intvl */

#define IEEE80211_BGSCAN_IDLE_MIN             100  /* min idle time (ms) */
#define IEEE80211_BGSCAN_IDLE_DEFAULT         250  /* default idle time (ms) */

#define IEEE80211_COVERAGE_CLASS_MAX           31  /* max coverage class */
#define IEEE80211_REGCLASSIDS_MAX              10  /* max regclass id list */

#define IEEE80211_PS_SLEEP                    0x1  /* STA is in power saving mode */
#define IEEE80211_PS_MAX_QUEUE                 50  /* maximum saved packets */

#define IEEE80211_MINIMUM_BMISS_TIME          500 /* minimum time without a beacon required for a bmiss */
#define IEEE80211_DEFAULT_BMISS_COUNT_MAX           3 /* maximum consecutive bmiss allowed */
#define IEEE80211_DEFAULT_BMISS_COUNT_RESET         2 /* number of  bmiss allowed before reset */

#define IEEE80211_BMISS_LIMIT                  15

#define IEEE80211_MAX_MCAST_LIST_SIZE          32 /* multicast list size */

#define IEEE80211_APPIE_MAX_FRAMES             10 /* max number frame types that can have app ie buffer setup */
#define IEEE80211_APPIE_MAX                  1024 /* max appie buffer size */
#define IEEE80211_OPTIE_MAX                   256 /* max optie buffer size */
#define IEEE80211_MAX_PRIVACY_FILTERS           4 /* max privacy filters */
#define IEEE80211_MAX_PMKID                     3 /* max number of PMKIDs */
#define IEEE80211_MAX_MISC_EVENT_HANDLERS       4 
#define IEEE80211_MAX_RESMGR_EVENT_HANDLERS    16 
#define IEEE80211_MAX_DEVICE_EVENT_HANDLERS     4 
#define IEEE80211_MAX_VAP_EVENT_HANDLERS        8
#define IEEE80211_MAX_VAP_MLME_EVENT_HANDLERS   4

#define IEEE80211_INACT_WAIT                   15  /* inactivity interval (secs) */

#define IEEE80211_MAGIC_ENGDBG           0x444247  /* Magic number for debugging purposes */
#define IEEE80211_DEFULT_KEEP_ALIVE_TIMEOUT    60  /* default keep alive timeout */
#define IEEE80211_FRAG_TIMEOUT                  1  /* fragment timeout in sec */

#define IEEE80211_MS_TO_TU(x)                (((x) * 1000) / 1024)
#define IEEE80211_TU_TO_MS(x)                (((x) * 1024) / 1000)

#define IEEE80211_USEC_TO_TU(x)                ((x) >> 10)  /* (x)/1024 */
#define IEEE80211_TU_TO_USEC(x)                ((x) << 10)  /* (x)X1024 */ 
/*
 * Macros used for RSSI calculation.
 */
#define ATH_RSSI_EP_MULTIPLIER               (1<<7)  /* pow2 to optimize out * and / */

#define ATH_RSSI_LPF_LEN                     10
#define ATH_RSSI_DUMMY_MARKER                0x127
#define ATH_EP_MUL(x, mul)                   ((x) * (mul))
#define ATH_EP_RND(x, mul)                   ((((x)%(mul)) >= ((mul)/2)) ? ((x) + ((mul) - 1)) / (mul) : (x)/(mul))
#define ATH_RSSI_GET(x)                      ATH_EP_RND(x, ATH_RSSI_EP_MULTIPLIER)

#define RSSI_LPF_THRESHOLD                   -20

#define ATH_RSSI_OUT(x)                      (((x) != ATH_RSSI_DUMMY_MARKER) ? (ATH_EP_RND((x), ATH_RSSI_EP_MULTIPLIER)) : ATH_RSSI_DUMMY_MARKER)
#define ATH_RSSI_IN(x)                       (ATH_EP_MUL((x), ATH_RSSI_EP_MULTIPLIER))
#define ATH_LPF_RSSI(x, y, len) \
    ((x != ATH_RSSI_DUMMY_MARKER) ? ((((x) << 3) + (y) - (x)) >> 3) : (y))
#define ATH_RSSI_LPF(x, y) do {                     \
    if ((y) >= RSSI_LPF_THRESHOLD)                         \
        x = ATH_LPF_RSSI((x), ATH_RSSI_IN((y)), ATH_RSSI_LPF_LEN);  \
} while (0)
#define ATH_ABS_RSSI_LPF(x, y) do {                     \
    if ((y) >= (RSSI_LPF_THRESHOLD + ATH_DEFAULT_NOISE_FLOOR))      \
        x = ATH_LPF_RSSI((x), ATH_RSSI_IN((y)), ATH_RSSI_LPF_LEN);  \
} while (0)

#define IEEE80211_PWRCONSTRAINT_VAL(vap) \
    (((vap)->iv_bsschan->ic_maxregpower > (vap->iv_ic)->ic_curchanmaxpwr) ? \
        (vap)->iv_bsschan->ic_maxregpower - (vap->iv_ic)->ic_curchanmaxpwr : 0)

typedef struct ieee80211_country_entry{
    u_int16_t   countryCode;  /* HAL private */
    u_int16_t   regDmnEnum;   /* HAL private */
    u_int16_t   regDmn5G;
    u_int16_t   regDmn2G;
    u_int8_t    isMultidomain;
    u_int8_t    iso[3];       /* ISO CC + (I)ndoor/(O)utdoor or both ( ) */
} IEEE80211_COUNTRY_ENTRY;

/*
 * 802.11 control state is split into a common portion that maps
 * 1-1 to a physical device and one or more "Virtual AP's" (VAP)
 * that are bound to an ieee80211com instance and share a single
 * underlying device.  Each VAP has a corresponding OS device
 * entity through which traffic flows and that applications use
 * for issuing ioctls, etc.
 */

struct ieee80211vap;
//typedef  void *ieee80211_vap_state_priv_t;
typedef int ieee80211_vap_state_priv_t;

#ifdef ATH_SUPPORT_HTC
#define HTC_MAX_VAP_NUM             5

#if defined(MAGPIE_HIF_USB) && defined(ENCAP_OFFLOAD)
#define HTC_MAX_NODE_NUM            13
#else
#define HTC_MAX_NODE_NUM            8
#endif


struct target_vap_info
{
    u_int16_t vap_valid;
    u_int8_t vap_macaddr[IEEE80211_ADDR_LEN];
    u_int8_t iv_vapindex;
    enum ieee80211_opmode iv_opmode;                       /* operation mode */
    u_int16_t  iv_rtsthreshold;
    u_int8_t   iv_mcast_rate;                              /* Multicast rate (Kbps) */
};

struct target_node_info
{
    u_int16_t ni_valid;
    u_int8_t ni_macaddr[IEEE80211_ADDR_LEN];
    u_int8_t ni_nodeindex;
    u_int8_t ni_vapindex;   
    u_int8_t ni_vapnode;
    u_int8_t ni_is_vapnode;
};
#endif /* #ifdef ATH_SUPPORT_HTC */

#if ATH_SUPPORT_WIRESHARK
/* avoid inclusion of ieee80211_radiotap.h everywhere */
struct ath_rx_radiotap_header;
#endif

#ifndef ATH_SUPPORT_HTC
typedef spinlock_t ieee80211_ic_state_lock_t;
#else
typedef htclock_t  ieee80211_ic_state_lock_t;
#endif

typedef struct ieee80211_ven_ie {
    u_int8_t                      ven_ie[IEEE80211_MAX_IE_LEN];
    u_int8_t                      ven_ie_len;
    u_int8_t                      ven_oui[3];
    bool                          ven_oui_set;
} IEEE80211_VEN_IE;



typedef struct ieee80211_vht_mcs {
    u_int16_t     mcs_map;      /* Max MCS for each SS */
    u_int16_t     data_rate;    /* Max data rate */
} ieee80211_vht_mcs_t;

typedef struct ieee80211_vht_mcs_set {
    ieee80211_vht_mcs_t     rx_mcs_set; /* B0-B15 Max Rx MCS for each SS
                                            B16-B28 Max Rx data rate
                                            B29-B31 reserved */
    ieee80211_vht_mcs_t     tx_mcs_set; /* B32-B47 Max Tx MCS for each SS
                                            B48-B60 Max Tx data rate
                                            B61-B63 reserved */
}ieee80211_vht_mcs_set_t;


/*
 * Data common to one or more virtual AP's.  State shared by
 * the underlying device and the net80211 layer is exposed here;
 * e.g. device-specific callbacks.
 */

#ifdef ATH_HTC_MII_RXIN_TASKLET    
typedef struct ieee80211_recv_mgt_args {
    struct ieee80211_node *ni;
    a_int32_t subtype;
    a_int32_t rssi;
    a_uint32_t rstamp;
}ieee80211_recv_mgt_args_t;

typedef struct _nawds_dentry{
    TAILQ_ENTRY(_nawds_dentry)  nawds_dlist;
    struct ieee80211vap              *vap ;
    u_int8_t                  mac[IEEE80211_ADDR_LEN];

}nawds_dentry_t;

void _ath_htc_netdeferfn_init(struct ieee80211com *ic);
void _ath_htc_netdeferfn_cleanup(struct ieee80211com *ic);


#define ATH_HTC_NETDEFERFN_INIT(_ic) _ath_htc_netdeferfn_init(_ic)
#define ATH_HTC_NETDEFERFN_CLEANUP(_ic) _ath_htc_netdeferfn_cleanup(_ic)


#else
#define ATH_HTC_NETDEFERFN_INIT(_ic) 
#define ATH_HTC_NETDEFERFN_CLEANUP(_ic) 
#endif

typedef struct ieee80211com {
    osdev_t                       ic_osdev; /* OS opaque handle */
    adf_os_device_t               ic_adf_dev; /* ADF opaque handle */
    bool                          ic_initialized;
    ieee80211_scanner_t           ic_scanner;
    ieee80211_scan_table_t        ic_scan_table;
    spinlock_t                    ic_lock; /* lock to protect access to ic data */
    ieee80211_ic_state_lock_t     ic_state_lock; /* lock to protect access to ic/sc state */

    TAILQ_HEAD(, ieee80211vap)    ic_vaps; /* list of vap instances */
    enum ieee80211_phytype        ic_phytype; /* XXX wrong for multi-mode */
    enum ieee80211_opmode         ic_opmode;  /* operation mode */

    u_int8_t                      ic_myaddr[IEEE80211_ADDR_LEN]; /* current mac address */
    u_int8_t                      ic_my_hwaddr[IEEE80211_ADDR_LEN]; /* mac address from EEPROM */

    u_int8_t                      ic_broadcast[IEEE80211_ADDR_LEN];

    u_int32_t                     ic_flags;       /* state flags */
    u_int32_t                     ic_flags_ext;   /* extension of state flags */
    u_int32_t                     ic_wep_tkip_htrate      :1, /* Enable htrate for wep and tkip */
                                  ic_non_ht_ap            :1, /* non HT AP found flag */
                                  ic_block_dfschan        :1, /* block the use of DFS channels flag */
                                  ic_doth                 :1, /* 11.h flag */
                                  ic_off_channel_support  :1, /* Off-channel support enabled */ //subrat
                                  ic_ht20Adhoc            :1,
                                  ic_ht40Adhoc            :1,
                                  ic_htAdhocAggr          :1,
                                  ic_disallowAutoCCchange :1, /* disallow CC change when assoc completes */
                                  ic_p2pDevEnable         :1, /* Is P2P Enabled? */
                                  ic_ignoreDynamicHalt    :1, /* disallowed  */
                                  ic_override_proberesp_ie:1, /* overwrite probe response IE with beacon IE */
                                  ic_2g_csa               :1;


    u_int32_t                     ic_caps;        /* capabilities */
    u_int32_t                     ic_caps_ext;    /* extension of capabilities */
    u_int8_t                      ic_ath_cap;     /* Atheros adv. capablities */
    u_int8_t                      ic_roaming;     /* Assoc state machine roaming mode */
    u_int32_t                     ic_ath_extcap;     /* Atheros extended capablities */
    u_int8_t                      ic_nopened; /* vap's been opened */
    struct ieee80211_rateset      ic_sup_rates[IEEE80211_MODE_MAX];
    struct ieee80211_rateset      ic_sup_half_rates;
    struct ieee80211_rateset      ic_sup_quarter_rates;
    struct ieee80211_rateset      ic_sup_ht_rates[IEEE80211_MODE_MAX];
    u_int32_t                     ic_modecaps;    /* set of mode capabilities */
    u_int16_t                     ic_curmode;     /* current mode */
    u_int16_t                     ic_intval;      /* default beacon interval for AP, ADHOC */
    u_int16_t                     ic_lintval;     /* listen interval for STATION */
    u_int16_t                     ic_lintval_assoc;  /* listen interval to use in association for STATION */
    u_int16_t                     ic_holdover;    /* PM hold over duration */
    u_int16_t                     ic_bmisstimeout;/* beacon miss threshold (ms) */
    u_int16_t                     ic_txpowlimit;  /* global tx power limit */
    u_int16_t                     ic_uapsdmaxtriggers; /* max triggers that could arrive */
    u_int8_t                      ic_coverageclass; /* coverage class */
	u_int8_t					  ic_rssi_ctl[3]; /*RSSI of RX chains*/

    /* 11n (HT) state/capabilities */             
    u_int16_t                     ic_htflags;            /* HT state flags */
    u_int16_t                     ic_htcap;              /* HT capabilities */
    u_int16_t                     ic_htextcap;           /* HT extended capabilities */ 
    u_int8_t                      ic_maxampdu;           /* maximum rx A-MPDU factor */
    u_int8_t                      ic_mpdudensity;        /* MPDU density */
    u_int8_t                      ic_enable2GHzHt40Cap;  /* HT40 supported on 2GHz channels */
    u_int8_t                      ic_weptkipaggr_rxdelim; /* Atheros proprietary wep/tkip aggr mode - rx delim count */
    u_int16_t                     ic_channelswitchingtimeusec; /* Channel Switching Time in usec */
    u_int8_t                      ic_no_weather_radar_chan;  /* skip weather radar channels*/
#ifdef ATH_SUPPORT_TxBF
    union ieee80211_hc_txbf       ic_txbf;                /* Tx Beamforming capabilities */
#ifdef TXBF_TODO
    u_int8_t                      ic_rc_calibrating:1,    /*for RC calibration*/
                                  ic_rc_40M_valid:1,
                                  ic_rc_20M_valid:1,
                                  ic_rc_wait_csi:1;
    void    (*ic_get_pos2_data)(struct ieee80211com *ic, u_int8_t **p_data, 
                u_int16_t* p_len, void **rx_status);
    void    (*ic_csi_report_send)(struct ieee80211_node *ni,u_int8_t *csi_buf,
                            u_int16_t buf_len, u_int8_t *mimo_control);
    int     (*ic_ieee80211_send_cal_qos_nulldata)(struct ieee80211_node *ni, int Cal_type);
    bool    (*ic_txbfrcupdate)(struct ieee80211com *ic, void *rx_status,
                u_int8_t *local_h, u_int8_t *CSIFrame, u_int8_t NESSA, u_int8_t NESSB, int BW);
    void    (*ic_ap_save_join_mac)(struct ieee80211com *ic, u_int8_t *join_macaddr);
    void    (*ic_start_imbf_cal)(struct ieee80211com *ic);
#endif
    void    (*ic_v_cv_send)(struct ieee80211_node *ni, u_int8_t	*data_buf,
                u_int16_t buf_len);
    int     (*ic_txbf_alloc_key)(struct ieee80211com *ic, struct ieee80211_node *ni);
    void    (*ic_txbf_set_key)(struct ieee80211com *ic, struct ieee80211_node *ni);
    void    (*ic_init_sw_cv_timeout)(struct ieee80211com *ic, struct ieee80211_node *ni);
	int     (*ic_set_txbf_caps)(struct ieee80211com *ic);
#ifdef TXBF_DEBUG
	void    (*ic_txbf_check_cvcache)(struct ieee80211com *ic, struct ieee80211_node *ni);
#endif
	void    (*ic_txbf_stats_rpt_inc)(struct ieee80211com *ic, struct ieee80211_node *ni);
	void    (*ic_txbf_set_rpt_received)(struct ieee80211com *ic, struct ieee80211_node *ni);
#endif
#ifdef IEEE80211_DEBUG_REFCNT
#define IC_IEEE80211_FIND_NODE(_ic,_nt,_macaddr)  (_ic)->ic_ieee80211_find_node_debug((_nt),(_macaddr), __FUNCTION__, __LINE__)
    struct  ieee80211_node*  (*ic_ieee80211_find_node_debug)(struct ieee80211_node_table *nt, const u_int8_t *macaddr,char* func, int line);
#else
#define IC_IEEE80211_FIND_NODE(_ic,_nt,_macaddr)  (_ic)->ic_ieee80211_find_node((_nt),(_macaddr))
    struct  ieee80211_node*  (*ic_ieee80211_find_node)(struct ieee80211_node_table *nt, const u_int8_t *macaddr);
#endif //IEEE80211_DEBUG_REFCNT
    /* Returns type of driver : Direct attach / Offload */
    bool    (*ic_is_mode_offload)(struct ieee80211com *ic);
    void    (*ic_ieee80211_unref_node)(struct ieee80211_node *ni);

    /*
     * 11n A-MPDU and A-MSDU support
     */
    int                           ic_ampdu_limit;     /* A-MPDU length limit */
    int                           ic_ampdu_density;   /* A-MPDU density */
    int                           ic_ampdu_subframes; /* A-MPDU subframe limit */
    int                           ic_amsdu_limit;     /* A-MSDU length limit */
    u_int32_t                     ic_amsdu_max_size;  /* A-MSDU buffer max size */

    /*
     * Global chain select mask for tx and rx. This mask is used only for 11n clients.
     * For legacy clients this mask is over written by a tx chain mask of 1. Rx chain
     * mask remains the global value for legacy clients.
     */
    u_int8_t                      ic_tx_chainmask;
    u_int8_t                      ic_rx_chainmask;

    /* actual number of rx/tx chains supported by HW */
    u_int8_t                      ic_num_rx_chain;
    u_int8_t                      ic_num_tx_chain;

#if ATH_SUPPORT_WAPI
    /* Max number of rx/tx chains supported by WAPI HW engine */
    u_int8_t                      ic_num_wapi_tx_maxchains;
    u_int8_t                      ic_num_wapi_rx_maxchains;
#endif
    /*
     * Number of spatial streams supported.
     */
    u_int8_t                      ic_spatialstreams;

    /* Current country information */
    char                          ic_country_iso[4];     /* ISO + 'I'/'O' and a NULL for align*/
    u_int8_t                      ic_multiDomainEnabled;
    IEEE80211_COUNTRY_ENTRY       ic_country;   /* Current country/regdomain */
    u_int8_t                      ic_isdfsregdomain; /* operating in DFS domain ? */

#if UMAC_SUPPORT_TDLS
    /* TDLS information */
    struct ieee80211_tdls         *ic_tdls;
#define tdls_recv_mgmt            ic_tdls->recv_mgmt
#define tdls_recv_null_data       ic_tdls->recv_null_data
    u_int32_t       iv_tdls_macaddr1;
    u_int32_t       iv_tdls_macaddr2;
    u_int32_t       iv_tdls_action;
#if ATH_TDLS_AUTO_CONNECT
    u_int8_t        ic_tdls_auto_enable;
    u_int16_t       ic_off_table_timeout;
    u_int16_t       ic_teardown_block_timeout;
    u_int16_t       ic_weak_peer_timeout;
    u_int8_t        ic_tdls_setup_margin;
    u_int8_t        ic_tdls_upper_boundary;
    u_int8_t        ic_tdls_lower_boundary;
    u_int8_t        ic_tdls_path_select_enable;
    u_int8_t        ic_tdls_setup_offset;
    u_int16_t       ic_path_select_period;
    u_int8_t        ic_tdls_cleaning;
    u_int8_t        ic_tdls_pathsel_timer_cnt;
    void            (*ic_tdls_clean)(struct ieee80211vap *vap);
    void            (*ic_recv_tdls_disc_resp)(struct ieee80211_node *ni, u_int8_t *addr);
    void            (*ic_tdls_path_select_attach)(struct ieee80211vap *vap);
    void            (*ic_tdls_path_select_detach)(struct ieee80211vap *vap);
    void            (*ic_tdls_table_query)(struct ieee80211vap *vap);
    void            (*ic_recv_tdls_sord_req)(struct ieee80211com *ic, u_int8_t *addr);
#endif
#endif /* UMAC_SUPPORT_TDLS */
    /*
     * Channel state:
     *
     * ic_channels is the set of available channels for the device;
     *    it is setup by the driver
     * ic_nchans is the number of valid entries in ic_channels
     * ic_chan_avail is a bit vector of these channels used to check
     *    whether a channel is available w/o searching the channel table.
     * ic_chan_active is a (potentially) constrained subset of
     *    ic_chan_avail that reflects any mode setting or user-specified
     *    limit on the set of channels to use/scan
     * ic_curchan is the current channel the device is set to; it may
     *    be different from ic_bsschan when we are off-channel scanning
     *    or otherwise doing background work
     * ic_bsschan is the channel selected for operation; it may
     *    be undefined (IEEE80211_CHAN_ANYC)
     * ic_prevchan is a cached ``previous channel'' used to optimize
     *    lookups when switching back+forth between two channels
     *    (e.g. for dynamic turbo)
     */
    int                           ic_nchans;  /* # entries in ic_channels */
    struct ieee80211_channel      ic_channels[IEEE80211_CHAN_MAX+1];
    u_int8_t                      ic_chan_avail[IEEE80211_CHAN_BYTES];
    u_int8_t                      ic_chan_active[IEEE80211_CHAN_BYTES];
    struct ieee80211_channel      *ic_curchan;   /* current channel */
    struct ieee80211_channel      *ic_prevchan;  /* previous channel */
#ifdef ATH_EXT_AP
    struct mi_node *ic_miroot; /* EXTAP MAC - IP table Root */
#endif

    /* regulatory class ids */
    u_int                         ic_nregclass;  /* # entries in ic_regclassids */
    u_int8_t                      ic_regclassids[IEEE80211_REGCLASSIDS_MAX];

    struct ieee80211_node_table   ic_sta; /* stations/neighbors */

#ifdef IEEE80211_DEBUG_NODELEAK
    TAILQ_HEAD(, ieee80211_node)  ic_nodes;/* information of all nodes */
    ieee80211_node_lock_t         ic_nodelock;    /* on the list */
#endif
    os_timer_t                    ic_inact_timer;
#if UMAC_SUPPORT_WNM
    os_timer_t                    ic_bssload_timer;
    u_int32_t                     ic_wnm_bss_count; 
    u_int32_t                     ic_wnm_bss_active;

#endif

    /* XXX multi-bss: split out common/vap parts? */
    struct ieee80211_wme_state    ic_wme;  /* WME/WMM state */
    struct ieee80211_wme_tspec    ic_sigtspec; /* Signalling tspec */
    struct ieee80211_wme_tspec    ic_datatspec; /* Data tspec */

    enum ieee80211_protmode       ic_protmode;    /* 802.11g protection mode */
    u_int16_t                     ic_sta_assoc;    /* stations associated */
    u_int16_t                     ic_nonerpsta;   /* # non-ERP stations */
    u_int16_t                     ic_longslotsta; /* # long slot time stations */
    u_int16_t                     ic_ht_sta_assoc;/* ht capable stations */
    u_int16_t                     ic_ht_gf_sta_assoc;/* ht greenfield capable stations */
    u_int16_t                     ic_ht40_sta_assoc; /* ht40 capable stations */
    systime_t                     ic_last_non_ht_sta; /* the last time a non ht sta is seen on channel */
    systime_t                     ic_last_nonerp_present; /* the last time a non ERP beacon is seen on channel */
    int8_t                        ic_ht20_only;

    /* 
     *  current channel max power, used to compute Power Constraint IE.
     *
     *  NB: local power constraint depends on the channel, but assuming it must
     *     be detected dynamically, we cannot maintain a table (i.e., will not
     *     know value until change to channel and detect).
     */
    u_int8_t                      ic_curchanmaxpwr;
    u_int8_t                      ic_chanchange_tbtt;
    u_int8_t                      ic_chanchange_chan;
    u_int16_t                     ic_chanchange_chwidth;
    struct ieee80211_channel      *ic_chanchange_channel;

#if !ATH_SUPPORT_STATS_APONLY
    struct ieee80211_phy_stats    ic_phy_stats[IEEE80211_MODE_MAX]; /* PHY counters */
#endif
    
    /*
     *  RX filter desired by upper layers.  Note this contains some bits which
     *  must be filtered by sw since the hw supports only a subset of possible
     *  filter actions.
     */
    u_int32_t                     ic_os_rxfilter;
    ieee80211_resmgr_t            ic_resmgr; /* opage handle to resource manager */

    ieee80211_tsf_timer_mgr_t     ic_tsf_timer;     /* opaque handle to TSF Timer manager */

    ieee80211_notify_tx_bcn_mgr_t ic_notify_tx_bcn_mgr; /* opaque handle to "Notify Tx Beacon" manager */
#if UMAC_SUPPORT_RPTPLACEMENT         /* iwpriv variables*/ 
    ieee80211_rptplacement_params_t  ic_rptplacementparams;
    u_int8_t                         ic_rptplacement_init_done;
#endif

#if UMAC_SUPPORT_SMARTANTENNA
    struct ieee80211_smartantenna_params  *ic_smartantennaparams; /* pointer for smart antenna params */
    u_int8_t                         ic_smartantenna_init_done;   /* indication for smart antenna initialization */
    spinlock_t                       sc_sa_trafficgen_lock;       /* spin lock for traffic generation */
    unsigned long                    sc_sa_trafficgen_lock_flags; /* storate for flags during usage of trafficgen spin lock */
    
    adf_os_workqueue_t *ic_smart_workqueue;                       /* dedicated worker thread for smart antenna module */
    adf_os_delayed_work_t smartant_task;                          /* work queue for smart antenna */
    adf_os_delayed_work_t smartant_traffic_task;                  /* work queue for traffic generation */

    u_int8_t                         ic_sa_timer;                 /* status of traffic gen timer. setting indicates timer is running */
    u_int8_t                         ic_sa_retraintimer;          /* status of retrain timer.setting indicates timer is running */

    os_timer_t  ic_smartant_retrain_timer;                        /* retrain timer declaration */
    os_timer_t  ic_smartant_traffic_gen_timer;                    /* traffic gen timer declaration */
#endif
   
#ifdef ATH_COALESCING
    /*
     * NB Tx Coalescing is a per-device setting to prevent non-PCIe (especially cardbus)
     * 11n devices running into underrun conditions on certain PCI chipsets.
     */
    ieee80211_coalescing_state    ic_tx_coalescing;
#endif
    IEEE80211_REG_PARAMETERS      ic_reg_parm;
    wlan_dev_event_handler_table  *ic_evtable[IEEE80211_MAX_DEVICE_EVENT_HANDLERS]; 
    void                          *ic_event_arg[IEEE80211_MAX_DEVICE_EVENT_HANDLERS]; 

    struct asf_print_ctrl         ic_print;
    u_int8_t                      ic_intolerant40;      /* intolerant 40 */
    u_int8_t                      ic_enable2040Coexist; /* Enable 20/40 coexistence */
    u_int8_t                      ic_tspec_active;      /* TSPEC is negotiated */
    u_int8_t                      cw_inter_found;   /* Flag to handle CW interference */
    u_int8_t                      ic_eacs_done;     /* Flag to indicate eacs status */
    ieee80211_acs_t               ic_acs;   /* opaque handle to acs */ 
    struct ieee80211_key          ic_bcast_proxykey;   /* default/broadcast for proxy STA */
    bool                          ic_softap_enable; /* For Lenovo SoftAP */

#ifdef ATH_BT_COEX
    enum ieee80211_opmode         ic_bt_coex_opmode; /* opmode for BT coex */
#endif

    u_int32_t                     ic_minframesize; /* Min framesize that can be recelived */

    /*
     * Table of registered cipher modules.
     */
    const struct ieee80211_cipher *ciphers[IEEE80211_CIPHER_MAX];
    int                           ic_ldpcsta_assoc; 

    u_int32_t                     ic_chanbwflag;

#if UMAC_SUPPORT_ADMCTL
    u_int16_t                     ic_mediumtime_reserved;
    void 						  (*ic_node_update_dyn_uapsd)(struct ieee80211_node *ni, uint8_t ac, int8_t deli, int8_t trig);
    void 						  *ic_admctl_rt[IEEE80211_MODE_MAX];
#endif

    /* virtual ap create/delete */
    struct ieee80211vap     *(*ic_vap_create)(struct ieee80211com *,
                                              int opmode, int scan_priority_base, int flags,
                                              const u_int8_t bssid[IEEE80211_ADDR_LEN]);
    void                    (*ic_vap_delete)(struct ieee80211vap *);

    int                     (*ic_vap_alloc_macaddr) (struct ieee80211com *ic, u_int8_t *bssid);
    int                     (*ic_vap_free_macaddr) (struct ieee80211com *ic, u_int8_t *bssid);

    /* send management frame to driver, like hardstart */
    int                     (*ic_mgtstart)(struct ieee80211com *, wbuf_t);
    
    /* reset device state after 802.11 parameter/state change */
    int                     (*ic_init)(struct ieee80211com *);
    int                     (*ic_reset_start)(struct ieee80211com *, bool no_flush);
    int                     (*ic_reset)(struct ieee80211com *);
    int                     (*ic_reset_end)(struct ieee80211com *, bool no_flush);
    int                     (*ic_start)(struct ieee80211com *);
    int                     (*ic_stop)(struct ieee80211com *);

    /* macaddr */
    int                     (*ic_set_macaddr)(struct ieee80211com *, u_int8_t *macaddr);
    
    /* regdomain */
    void                    (*ic_get_currentCountry)(struct ieee80211com *,
                                                     IEEE80211_COUNTRY_ENTRY *ctry);
    int                     (*ic_set_country)(struct ieee80211com *, char *isoName, u_int16_t cc, enum ieee80211_clist_cmd cmd);
    int                     (*ic_set_regdomain)(struct ieee80211com *, int regdomain);
    int                     (*ic_get_regdomain)(struct ieee80211com *);
    int                     (*ic_get_dfsdomain)(struct ieee80211com *);
    int                     (*ic_set_quiet)(struct ieee80211_node *, u_int8_t *quiet_elm);
    u_int16_t               (*ic_find_countrycode)(struct ieee80211com *, char* isoName);

    /* update device state for 802.11 slot time change */
    void                    (*ic_updateslot)(struct ieee80211com *);

    /* new station association callback/notification */
    void                    (*ic_newassoc)(struct ieee80211_node *, int);

    /* notify received beacon */
    void                    (*ic_beacon_update)(struct ieee80211_node *, int rssi);

    /* node state management */
    struct ieee80211_node   *(*ic_node_alloc)(struct ieee80211vap *, const u_int8_t *macaddr, bool tmpnode);
    void                    (*ic_node_free)(struct ieee80211_node *);
    void                    (*ic_node_cleanup)(struct ieee80211_node *);
    int8_t                  (*ic_node_getrssi)(const struct ieee80211_node *, int8_t ,u_int8_t);
    u_int32_t               (*ic_node_getrate)(const struct ieee80211_node *, u_int8_t type);
    void                    (*ic_node_authorize)(struct ieee80211_node *,u_int32_t authorize);
    
    /* scanning support */
    void                    (*ic_scan_start)(struct ieee80211com *);
    void                    (*ic_scan_end)(struct ieee80211com *);
    void                    (*ic_led_scan_start)(struct ieee80211com *);
    void                    (*ic_led_scan_end)(struct ieee80211com *);
    int                     (*ic_set_channel)(struct ieee80211com *);
#if ATH_SUPPORT_WIFIPOS
    int                     (*ic_lean_set_channel)(struct ieee80211com *);
    void                    (*ic_pause_node)(struct ieee80211com *, struct ieee80211_node *, bool pause );
    void                    (*ic_resched_txq)(struct ieee80211com *ic);
    bool                    (*ic_disable_hwq)(struct ieee80211com *ic, u_int16_t mask);
    int                     (*ic_vap_reap_txqs)(struct ieee80211com *ic, struct ieee80211vap *vap);
#endif
    /* change power state */
    void                    (*ic_pwrsave_set_state)(struct ieee80211com *, IEEE80211_PWRSAVE_STATE newstate);

    /* update protmode */
    void                    (*ic_update_protmode)(struct ieee80211com *);

    /* get hardware txq depth */
    u_int32_t               (*ic_txq_depth)(struct ieee80211com *);
    /* get hardware txq depth per ac*/
    u_int32_t               (*ic_txq_depth_ac)(struct ieee80211com *, int ac);
    
    /* mhz to ieee conversion */
    u_int                   (*ic_mhz2ieee)(struct ieee80211com *, u_int freq, u_int flags);

    /* channel width change */
    void                    (*ic_chwidth_change)(struct ieee80211_node *ni);
    
    /* Nss change */
    void                    (*ic_nss_change)(struct ieee80211_node *ni);
    /* aggregation support */
    u_int8_t                ic_addba_mode; /* ADDBA mode auto or manual */
    void                    (*ic_set_ampduparams)(struct ieee80211_node *ni);
    void                    (*ic_set_weptkip_rxdelim)(struct ieee80211_node *ni, u_int8_t rxdelim);
    void                    (*ic_addba_requestsetup)(struct ieee80211_node *ni,
                                                     u_int8_t tidno,
                                                     struct ieee80211_ba_parameterset *baparamset,
                                                     u_int16_t *batimeout,
                                                     struct ieee80211_ba_seqctrl *basequencectrl,
                                                     u_int16_t buffersize
                                                     );
    void                    (*ic_addba_responsesetup)(struct ieee80211_node *ni,
                                                      u_int8_t tidno,
                                                      u_int8_t *dialogtoken, u_int16_t *statuscode,
                                                      struct ieee80211_ba_parameterset *baparamset,
                                                      u_int16_t *batimeout
                                                      );
    int                     (*ic_addba_requestprocess)(struct ieee80211_node *ni,
                                                       u_int8_t dialogtoken,
                                                       struct ieee80211_ba_parameterset *baparamset,
                                                       u_int16_t batimeout,
                                                       struct ieee80211_ba_seqctrl basequencectrl
                                                       );
    void                    (*ic_addba_responseprocess)(struct ieee80211_node *ni,
                                                        u_int16_t statuscode,
                                                        struct ieee80211_ba_parameterset *baparamset,
                                                        u_int16_t batimeout
                                                        );
    void                    (*ic_addba_clear)(struct ieee80211_node *ni);
    void                    (*ic_delba_process)(struct ieee80211_node *ni,
                                                struct ieee80211_delba_parameterset *delbaparamset,
                                                u_int16_t reasoncode
                                                );
    int                     (*ic_addba_send)(struct ieee80211_node *ni,
                                             u_int8_t tid,
                                             u_int16_t buffersize);
    void                    (*ic_delba_send)(struct ieee80211_node *ni,
                                             u_int8_t tid,
                                             u_int8_t initiator,
                                             u_int16_t reasoncode);
    void                    (*ic_addba_status)(struct ieee80211_node *ni,
                                               u_int8_t tid,
                                               u_int16_t *status);
    void                    (*ic_addba_setresponse)(struct ieee80211_node *ni,
                                                    u_int8_t tid,
                                                    u_int16_t statuscode);
    void                    (*ic_send_singleamsdu)(struct ieee80211_node *ni,
                                                    u_int8_t tid);
    void                    (*ic_addba_clearresponse)(struct ieee80211_node *ni);
    void                    (*ic_sm_pwrsave_update)(struct ieee80211_node *ni, int, int, int);
    /* update station power save state when operating in AP mode */
    void                    (*ic_node_psupdate)(struct ieee80211_node *, int, int);
    
    /* To get the number of frames queued up in lmac */
    int                     (*ic_node_queue_depth)(struct ieee80211_node *);

    int16_t                 (*ic_get_noisefloor)(struct ieee80211com *, struct ieee80211_channel *);
    void                    (*ic_get_chainnoisefloor)(struct ieee80211com *, struct ieee80211_channel *, int16_t *);
#if ATH_SUPPORT_VOW_DCS
    void                    (*ic_disable_dcsim)(struct ieee80211com *);
#endif
    void                    (*ic_set_config)(struct ieee80211vap *);

    void                    (*ic_set_safemode)(struct ieee80211vap *, int);

    int                     (*ic_rmgetcounters)(struct ieee80211com *ic, struct ieee80211_mib_cycle_cnts *pCnts);
 
     void                     (*ic_set_rx_sel_plcp_header)(struct ieee80211com *ic, 
                                                           int8_t sel, int8_t query);

#ifdef ATHR_RNWF
    void                    (*ic_rmclearcounters)(struct ieee80211com *ic);
    int                     (*ic_rmupdatecounters)(struct ieee80211com *ic, struct ath_mib_mac_stats *pStats);
#endif
    u_int8_t                (*ic_rcRateValueToPer)(struct ieee80211com *ic, struct ieee80211_node *ni, int txRateKbps);

    /* to get TSF values */
    u_int32_t               (*ic_get_TSF32)(struct ieee80211com *ic);
#ifdef ATH_USB
    /* to get generic timer expired tsf time, this is used to reduce the wmi command delay */
    u_int32_t               (*ic_get_target_TSF32)(struct ieee80211com *ic);
#endif
    u_int64_t               (*ic_get_TSF64)(struct ieee80211com *ic);
#if ATH_SUPPORT_WIFIPOS
    u_int64_t               (*ic_get_TSFTSTAMP)(struct ieee80211com *ic);
#endif

    /* To Set Transmit Power Limit */
    void                    (*ic_set_txPowerLimit)(struct ieee80211com *ic, u_int32_t limit, u_int16_t tpcInDb, u_int32_t is2Ghz);

	void                    (*ic_set_txPowerAdjust)(struct ieee80211com *ic, int32_t adjust, u_int32_t is2Ghz);

    /* To get Transmit power in 11a common mode */
    u_int8_t                (*ic_get_common_power)(struct ieee80211com *ic, struct ieee80211_channel *chan);

    /* To get Maximum Transmit Phy rate */
    u_int32_t               (*ic_get_maxphyrate)(struct ieee80211com *ic, struct ieee80211_node *ni);

    /* Set Rx Filter */
    void                    (*ic_set_rxfilter)(struct ieee80211com *ic,u_int32_t filter);

    /* Get Mfg Serial Num */
    int                     (*ic_get_mfgsernum)(struct ieee80211com *ic, u_int8_t *pSn, int limit);
#ifdef ATHR_RNWF
    /* Get Channel Data */
    int                     (*ic_get_chandata)(struct ieee80211com *ic, struct ieee80211_channel *pChan, struct ath_chan_data *pData);
#endif

    /* Get Current RSSI */
    u_int32_t               (*ic_get_curRSSI)(struct ieee80211com *ic);

#ifdef ATH_SWRETRY
    /* Enable/Disable software retry */
    void                    (*ic_set_swretrystate)(struct ieee80211_node *ni, int flag);
    /* Schedule one single frame in LMAC upon PS-Poll, return 0 if scheduling a LMAC frame successfully */
    int                     (*ic_handle_pspoll)(struct ieee80211com *ic, struct ieee80211_node *ni);
    /* Check whether there is pending frame in LMAC tid Q's, return 0 if there exists */
    int                     (*ic_exist_pendingfrm_tidq)(struct ieee80211com *ic, struct ieee80211_node *ni);
    int                     (*ic_reset_pause_tid)(struct ieee80211com *ic, struct ieee80211_node *ni);	
#endif
    u_int32_t               (*ic_get_wpsPushButton)(struct ieee80211com *ic);
    void            (*ic_green_ap_set_enable)( struct ieee80211com *ic, int val );
    int             (*ic_green_ap_get_enable)( struct ieee80211com *ic);
    void            (*ic_green_ap_set_transition_time)( struct ieee80211com *ic, int val );
    int             (*ic_green_ap_get_transition_time)( struct ieee80211com *ic);
    void            (*ic_green_ap_set_on_time)( struct ieee80211com *ic, int val );
    int             (*ic_green_ap_get_on_time)( struct ieee80211com *ic);
    void            (*ic_green_ap_set_print_level)(struct ieee80211com *ic, int val);
    int             (*ic_green_ap_get_print_level)(struct ieee80211com *ic);
    int16_t         (*ic_get_cur_chan_nf)(struct ieee80211com *ic);
    void            (*ic_get_cur_chan_stats)(struct ieee80211com *ic, struct ieee80211_chan_stats *chan_stats);
    int32_t         (*ic_ath_send_rssi)(struct ieee80211com *ic, u_int8_t *macaddr, struct ieee80211com *vap);
    int32_t         (*ic_ath_request_stats)(struct ieee80211com *ic,  void *cmd);
    int32_t         (*ic_ath_enable_ap_stats)(struct ieee80211com *ic, u_int8_t val);
    /* update PHY counters */
    void                    (*ic_update_phystats)(struct ieee80211com *ic, enum ieee80211_phymode mode);
    void                    (*ic_clear_phystats)(struct ieee80211com *ic);
    void                    (*ic_log_text)(struct ieee80211com *ic,char *text);
    int                     (*ic_set_chain_mask)(struct ieee80211com *ic, ieee80211_device_param type, 
                                                 u_int32_t mask);
    bool                    (*ic_need_beacon_sync)(struct ieee80211com *ic); /* check if ath is waiting for beacon sync */
#ifdef IEEE80211_DEBUG_NODELEAK
    void                    (*ic_print_nodeq_info)(struct ieee80211_node *ni);
#endif
    void                    (*ic_bss_to40)(struct ieee80211com *);
    void                    (*ic_bss_to20)(struct ieee80211com *);
#ifdef ATH_SUPPORT_HTC
    void                    (*ic_add_node_target)(struct ieee80211com *ic, void *vap, int size);
    void                    (*ic_add_vap_target)(struct ieee80211com *ic, void *vap, int size);
    void                    (*ic_update_node_target)(struct ieee80211com *ic, void *vap, int size);    
#if ENCAP_OFFLOAD
    void                    (*ic_update_vap_target)(struct ieee80211com *ic, void *vap, int size);
#endif    
    void                    (*ic_htc_ic_update_target)(struct ieee80211com *ic, void *vap, int size);
    void                    (*ic_delete_vap_target)(struct ieee80211com *ic, void *vap, int size);
    void                    (*ic_delete_node_target)(struct ieee80211com *ic, void *vap, int size);
    void                    (*ic_get_config_chainmask)(struct ieee80211com *ic, void *vap);
    struct target_vap_info  target_vap_bitmap[HTC_MAX_VAP_NUM];
    struct target_node_info target_node_bitmap[HTC_MAX_NODE_NUM];
    int                     ic_delete_in_progress;  /* flag to indicate delete in progress */
#endif

#ifdef ATH_SUPPORT_CWM
    enum ieee80211_cwm_extprotmode    (*ic_cwm_get_extprotmode)(struct ieee80211com *ic);
    enum ieee80211_cwm_extprotspacing (*ic_cwm_get_extprotspacing)(struct ieee80211com *ic);
    enum ieee80211_cwm_mode           (*ic_cwm_get_mode)(struct ieee80211com *ic);
    enum ieee80211_cwm_width          (*ic_cwm_get_width)(struct ieee80211com *ic);
    u_int32_t                         (*ic_cwm_get_enable)(struct ieee80211com *ic);
    u_int32_t                         (*ic_cwm_get_extbusythreshold)(struct ieee80211com *ic);
    int8_t                            (*ic_cwm_get_extoffset)(struct ieee80211com *ic);
    
    void                              (*ic_cwm_set_extprotmode)(struct ieee80211com *ic, enum ieee80211_cwm_extprotmode mode);
    void                              (*ic_cwm_set_extprotspacing)(struct ieee80211com *ic, enum ieee80211_cwm_extprotspacing sp);
    void                              (*ic_cwm_set_enable)(struct ieee80211com *ic, u_int32_t en);
    void                              (*ic_cwm_set_extoffset)(struct ieee80211com *ic, int8_t val);
    void                              (*ic_cwm_set_extbusythreshold)(struct ieee80211com *ic, u_int32_t threshold);
    void                              (*ic_cwm_set_mode)(struct ieee80211com *ic, enum ieee80211_cwm_mode mode);
    void                              (*ic_cwm_set_width)(struct ieee80211com *ic, enum ieee80211_cwm_width width);
#endif /* ATH_SUPPORT_CWM */
    
    struct ieee80211_tsf_timer *  
                            (*ic_tsf_timer_alloc)(struct ieee80211com *ic,
                                                    tsftimer_clk_id clk_id,
                                                    ieee80211_tsf_timer_function trigger_action,
                                                    ieee80211_tsf_timer_function overflow_action,
                                                    ieee80211_tsf_timer_function outofrange_action,
                                                    void *arg);

    void                    (*ic_tsf_timer_free)(struct ieee80211com *ic, struct ieee80211_tsf_timer *timer);

    void                    (*ic_tsf_timer_start)(struct ieee80211com *ic, struct ieee80211_tsf_timer *timer,
                                                    u_int32_t timer_next, u_int32_t period);

    void                    (*ic_tsf_timer_stop)(struct ieee80211com *ic, struct ieee80211_tsf_timer *timer);

#define IEEE80211_IS_TAPMON_ENABLED(__ic) 0
#if ATH_SUPPORT_WIRESHARK
    void            (*ic_fill_radiotap_hdr)(struct ieee80211com *ic, struct ath_rx_radiotap_header *rh, wbuf_t wbuf);
#if _MAVERICK_STA_
    void            (*ic_set_tapmon_enable)(struct ieee80211com *ic, int enable);
    int             (*ic_is_tapmon_enabled)(struct ieee80211com *ic);
#undef IEEE80211_IS_TAPMON_ENABLED
#define IEEE80211_IS_TAPMON_ENABLED(__ic) ((__ic)->ic_is_tapmon_enabled(__ic) == 1)
#endif /* _MAVERICK_STA_ */
#endif /* ATH_SUPPORT_WIRESHARK */
#ifdef ATH_BT_COEX
    int                     (*ic_get_bt_coex_info)(struct ieee80211com *ic, u_int32_t infoType);
#endif
    /* Get MFP support */
    u_int32_t               (*ic_get_mfpsupport)(struct ieee80211com *ic);

    /* Set hw MFP Qos bits */
    void                    (*ic_set_hwmfpQos)(struct ieee80211com *ic, u_int32_t dot11w);
#ifdef ATH_SUPPORT_IQUE
    /* Set IQUE parameters: AC, Rate Control, and HBR parameters */
    void        (*ic_set_acparams)(struct ieee80211com *ic, u_int8_t ac, u_int8_t use_rts,
                                   u_int8_t aggrsize_scaling, u_int32_t min_kbps);
    void        (*ic_set_rtparams)(struct ieee80211com *ic, u_int8_t rt_index,
                                   u_int8_t perThresh, u_int8_t probeInterval);
    void        (*ic_get_iqueconfig)(struct ieee80211com *ic);
    void        (*ic_set_hbrparams)(struct ieee80211vap *, u_int8_t ac, u_int8_t enable, u_int8_t per);
#endif /*ATH_SUPPORT_IQUE*/

#if UMAC_SUPPORT_RPTPLACEMENT
    struct ieee80211_rptgput   *ic_rptgput;
#endif

    u_int32_t   (*ic_get_goodput)(struct ieee80211_node *ni);

#if  ATH_SUPPORT_GREEN_AP
    struct ath_green_ap *ic_green_ap;
    spinlock_t  green_ap_ps_lock;
#endif  /* ATH_SUPPORT_GREEN_AP */

#if ATH_SUPPORT_SPECTRAL

    /* SPECTRAL Related data */
    void    *ic_spectral;
    int     (*ic_spectral_control)(struct ieee80211com *ic, u_int id,
                              void *indata, u_int32_t insize,
                              void *outdata, u_int32_t *outsize);

    /*  EACS with spectral analysis*/
    int                  (*ic_get_control_duty_cycle)(struct ieee80211com *ic);
    int                  (*ic_get_extension_duty_cycle)(struct ieee80211com *ic);
    void                (*ic_start_spectral_scan)(struct ieee80211com *ic);
    void                (*ic_stop_spectral_scan)(struct ieee80211com *ic);
#endif
#if ATH_SLOW_ANT_DIV
    void                 (*ic_antenna_diversity_suspend)(struct ieee80211com *ic);
    void                 (*ic_antenna_diversity_resume)(struct ieee80211com *ic);
#endif

    u_int8_t                (*ic_get_ctl_by_country)(struct ieee80211com *ic, u_int8_t *country, bool is2G);
    u_int16_t              (*ic_dfs_usenol)(struct ieee80211com *ic);
    u_int16_t              (*ic_dfs_isdfsregdomain)(struct ieee80211com *ic);

#if  ATH_SUPPORT_AOW
    /* function pointers : AoW feature */
    void (*ic_set_swretries)(struct ieee80211com *ic, u_int32_t val);
    void (*ic_set_aow_rtsretries)(struct ieee80211com *ic, u_int16_t val);
    void (*ic_aow_clear_audio_channels)(struct ieee80211com *ic, u_int32_t val);
    u_int32_t (*ic_get_swretries)(struct ieee80211com *ic);
    u_int16_t (*ic_get_aow_rtsretries)(struct ieee80211com *ic);
    void (*ic_set_aow_latency)(struct ieee80211com *ic, u_int32_t val);
    void (*ic_set_aow_playlocal)(struct ieee80211com *ic, u_int32_t val);

    void (*ic_set_aow_er)(struct ieee80211com *ic, u_int32_t val);
    u_int32_t (*ic_get_aow_er)(struct ieee80211com *ic);

    void (*ic_set_aow_ec)(struct ieee80211com *ic, u_int32_t val);
    u_int32_t (*ic_get_aow_ec)(struct ieee80211com *ic);

    void (*ic_set_aow_ec_ramp)(struct ieee80211com *ic, u_int32_t val);
    u_int32_t (*ic_get_aow_ec_ramp)(struct ieee80211com *ic);
    
    void (*ic_set_aow_ec_fmap)(struct ieee80211com *ic, u_int32_t val);
    u_int32_t (*ic_get_aow_ec_fmap)(struct ieee80211com *ic);

    void (*ic_set_aow_es)(struct ieee80211com *ic, u_int32_t val);
    u_int32_t (*ic_get_aow_es)(struct ieee80211com *ic);

    void (*ic_set_aow_ess)(struct ieee80211com *ic, u_int32_t val);
    u_int32_t (*ic_get_aow_ess)(struct ieee80211com *ic);

    void (*ic_set_aow_as)(struct ieee80211com *ic, u_int32_t val);
    u_int32_t (*ic_get_aow_as)(struct ieee80211com *ic);

    void (*ic_set_aow_assoc_policy)(struct ieee80211com* ic, u_int32_t val);
    u_int32_t(*ic_get_aow_assoc_policy)(struct ieee80211com* ic);

    void (*ic_set_aow_audio_ch)(struct ieee80211com *ic, u_int32_t val);
    u_int32_t (*ic_get_aow_audio_ch)(struct ieee80211com *ic);

    void (*ic_set_aow_ess_count)(struct ieee80211com *ic, u_int32_t val);
    
    void (*ic_aow_clear_estats)(struct ieee80211com *ic);
    u_int32_t (*ic_aow_get_estats)(struct ieee80211com *ic);
    
    void (*ic_set_aow_frame_size)(struct ieee80211com *ic, u_int32_t val);
    u_int32_t (*ic_get_aow_frame_size)(struct ieee80211com *ic);

    void (*ic_set_aow_alt_setting)(struct ieee80211com *ic, u_int32_t val);
    u_int32_t (*ic_get_aow_alt_setting)(struct ieee80211com *ic);

    void (*ic_set_aow_rx_proc_time)(struct ieee80211com *ic, u_int32_t rx_proc_time);
    u_int32_t (*ic_get_aow_rx_proc_time)(struct ieee80211com *ic);
    
    void (*ic_aow_record_txstatus)(struct ieee80211com *ic,
                                   struct ath_buf *bf,
                                   struct ath_tx_status *ts);
    
    u_int32_t (*ic_get_aow_latency)(struct ieee80211com *ic);
    u_int32_t (*ic_get_aow_playlocal)(struct ieee80211com *ic);
    u_int64_t (*ic_get_aow_latency_us)(struct ieee80211com *ic);
    u_int64_t (*ic_get_aow_tsf_64)(struct ieee80211com *ic);
    
    void      (*ic_set_audio_channel)(struct ieee80211com *ic, int channel, struct ether_addr* addr);
    void      (*ic_remove_audio_channel)(struct ieee80211com *ic, int channel, struct ether_addr* addr);
    int       (*ic_get_num_mapped_dst)(struct ieee80211com *ic, int channel);
    int       (*ic_get_aow_macaddr)(struct ieee80211com *ic, int channel, int index, struct ether_addr* addr);
    int       (*ic_get_aow_chan_seqno)(struct ieee80211com *ic, int channel, int* seqno);
    void      (*ic_list_audio_channel)(struct ieee80211com *ic);
    void      (*ic_start_aow_inter)(struct ieee80211com *ic, u_int32_t startTime, u_int32_t period);
    void      (*ic_stop_aow_inter)(struct ieee80211com *ic);
    void      (*ic_gpio11_toggle)(struct ieee80211com *ic, u_int16_t flg);
    void      (*ic_aow_proc_timer_start)(struct ieee80211com *ic);
    void      (*ic_aow_proc_timer_stop)(struct ieee80211com *ic);
    void      (*ic_aow_proc_timer_set_period)(struct ieee80211com *ic, u_int32_t period);    
    
    void      (*ic_aow_reset)(struct ieee80211com *ic);

    u_int32_t (*ic_get_aow_tx_rate_info)(struct ieee80211_node *ni);

    ieee80211_aow_t ic_aow;
#endif  /* ATH_SUPPORT_AOW */
    int                     (*ic_reg_notify_tx_bcn)(struct ieee80211com *ic,
                                                    ieee80211_tx_bcn_notify_func callback,
                                                    void *arg);
    int                     (*ic_dereg_notify_tx_bcn)(struct ieee80211com *ic);

    int                     (*ic_vap_pause_control)(struct ieee80211com *ic, struct ieee80211vap *vap, bool pause); /* if vap is null, pause all vaps */
    void                    (*ic_enable_rifs_ldpcwar) (struct ieee80211_node *ni, bool value);
    void                    (*ic_process_uapsd_trigger) (struct ieee80211com *ic, struct ieee80211_node *ni, bool enforce_sp_limit, bool *sent_eosp);
    int                     (*ic_is_hwbeaconproc_active) (struct ieee80211com *ic);
    void                    (*ic_hw_beacon_rssi_threshold_enable)(struct ieee80211com *ic,
                                                u_int32_t rssi_threshold);
    void                    (*ic_hw_beacon_rssi_threshold_disable)(struct ieee80211com *ic);

#ifdef MAGPIE_HIF_GMAC    
    u_int32_t               ic_chanchange_cnt;   
#endif
#if UMAC_SUPPORT_VI_DBG
    struct ieee80211_vi_dbg   *ic_vi_dbg;
    struct ieee80211_vi_dbg_params  *ic_vi_dbg_params;
    void   (*ic_set_vi_dbg_restart)(struct ieee80211com *ic);
    void   (*ic_set_vi_dbg_log) (struct ieee80211com *ic, bool enable);
#endif
    struct ieee80211_quiet_param *ic_quiet;

#if UMAC_SUPPORT_SMARTANTENNA
    /* Program default antenna */
    void        (*ic_set_default_antenna)(struct ieee80211com *ic, u_int32_t antenna);
    void        (*ic_set_selected_smantennas) (struct ieee80211_node *ni, int antenna);
    /* get default antenna programmed in LMAC */
    u_int32_t   (*ic_get_default_antenna)(struct ieee80211com *ic);
    /* get valid rates and rateIndex from rate control module */
    void        (*ic_prepare_rateset)(struct ieee80211com *ic, struct ieee80211_node *ni);
    int8_t      (*ic_get_smartantenna_enable) (struct ieee80211com *ic);
    u_int32_t   (*ic_get_smartantenna_ratestats) (struct ieee80211_node *ni, void *rate_stats);
    void        (*ic_set_sa_train_params) (struct ieee80211_node *ni, u_int8_t ant , u_int32_t rateidx, u_int32_t flags_numpkts);
    int         (*ic_get_sa_trafficgen_required) (struct ieee80211_node *ni);
    uint8_t     (*ic_get_sa_defant) (struct ieee80211com *ic);

#endif    
#ifdef ATH_HTC_MII_RXIN_TASKLET    
    adf_nbuf_queue_t      ic_mgmt_nbufqueue;
    os_mgmt_lock_t        ic_mgmt_lock;
    atomic_t              ic_mgmt_deferflags;
    
    TAILQ_HEAD(, ieee80211_node)  ic_nodeleave_queue;
    os_nodeleave_lock_t           ic_nodeleave_lock;
    atomic_t                      ic_nodeleave_deferflags;
    TAILQ_HEAD(, _nawds_dentry)       ic_nawdslearnlist;
    os_nawdsdefer_lock_t          ic_nawdsdefer_lock;
    atomic_t                      ic_nawds_deferflags;
#endif
#ifdef DBG
    u_int32_t               (*ic_hw_reg_read)(struct ieee80211com *ic, u_int32_t);
#endif
#if UMAC_SUPPORT_PERIODIC_PERFSTATS
    spinlock_t                          ic_prdperfstats_lock;
    os_timer_t                          ic_prdperfstats_timer;
    u_int8_t                            ic_prdperfstats_is_started;
    struct ieee80211_prdperfstat_thrput ic_thrput; 
    struct ieee80211_prdperfstat_per    ic_per; 
#endif /* UMAC_SUPPORT_PERIODIC_PERFSTATS */
    int                               (*ic_dfs_attached)(struct ieee80211com *ic);
#ifdef ATH_SUPPORT_DFS
    void          *ic_dfs;
    struct ieee80211_dfs_state ic_dfs_state;
    int                               (*ic_dfs_attach)(struct ieee80211com *ic, void *pCap, void *radar_info);
    int                               (*ic_dfs_detach)(struct ieee80211com *ic);
    int                               (*ic_dfs_enable)(struct ieee80211com *ic, int *is_fastclk, void *);
    int                               (*ic_dfs_disable)(struct ieee80211com *ic);
    int                               (*ic_get_ext_busy)(struct ieee80211com *ic);
    int                               (*ic_get_mib_cycle_counts_pct)(struct ieee80211com *ic, 
                                            u_int32_t *rxc_pcnt, u_int32_t *rxf_pcnt, u_int32_t *txf_pcnt);
    int                               (*ic_dfs_get_thresholds)(struct ieee80211com *ic,
                                            void *pe);

    int                               (*ic_dfs_debug)(struct ieee80211com *ic, int type, void *data);
    /*
     * Update the channel list with the current set of DFS
     * NOL entries.
     *
     * + 'cmd' indicates what to do; for now it should just
     *   be DFS_NOL_CLIST_CMD_UPDATE which will update all
     *   channels, given the _entire_ NOL. (Rather than
     *   the earlier behaviour with clist_update, which
     *   was to either add or remove a set of channel
     *   entries.)
     */
    void    (*ic_dfs_clist_update)(struct ieee80211com *ic, int cmd,
              struct dfs_nol_chan_entry *, int nentries);

#endif
    void                             (*ic_dfs_notify_radar)(struct ieee80211com *ic, struct ieee80211_channel *chan);
    void                             (*ic_dfs_unmark_radar)(struct ieee80211com *ic, struct ieee80211_channel *chan);
    struct ieee80211_channel *(*ic_find_channel)(struct ieee80211com *ic, int freq, u_int32_t flags);
    unsigned int                     (*ic_ieee2mhz)(u_int chan, u_int flags);
    int         (*ic_dfs_control)(struct ieee80211com *ic, u_int id,
                                   void *indata, u_int32_t insize,
                                   void *outdata, u_int32_t *outsize);
    void                              (*ic_start_csa)(struct ieee80211com *ic,
                                          u_int8_t ieeeChan);

    u_int64_t                           (*ic_get_tx_hw_retries)(struct ieee80211com *ic);
    u_int64_t                           (*ic_get_tx_hw_success)(struct ieee80211com *ic);
    void                                (*ic_rate_node_update)(struct ieee80211com *ic, 
                                                               struct ieee80211_node *ni,
                                                               int isnew);

    /* isr dynamic aponly mode control */
    bool                                ic_aponly;

#if LMAC_SUPPORT_POWERSAVE_QUEUE
    u_int8_t                            (*ic_get_lmac_pwrsaveq_len)(struct ieee80211com *ic,
                                                                    struct ieee80211_node *ni, u_int8_t frame_type);
    int                                 (*ic_node_pwrsaveq_send)(struct ieee80211com *ic, 
                                                              struct ieee80211_node *ni, u_int8_t frame_type);
    void                                (*ic_node_pwrsaveq_flush)(struct ieee80211com *ic, struct ieee80211_node *ni);
    int                                 (*ic_node_pwrsaveq_drain)(struct ieee80211com *ic, struct ieee80211_node *ni);
    int                                 (*ic_node_pwrsaveq_age)(struct ieee80211com *ic, struct ieee80211_node *ni);
    void                                (*ic_node_pwrsaveq_get_info)(struct ieee80211com *ic,
                                                                  struct ieee80211_node *ni, ieee80211_node_saveq_info *info);
    void                                (*ic_node_pwrsaveq_set_param)(struct ieee80211com *ic,
                                                                   struct ieee80211_node *ni, enum ieee80211_node_saveq_param param,
                                                                   u_int32_t val);
#endif
#if ATH_SUPPORT_FLOWMAC_MODULE
    int                                 (*ic_get_flowmac_enabled_State)(struct ieee80211com *ic);
#endif
    ieee80211_resmgr_t                  (*ic_resmgr_create)(struct ieee80211com *ic, ieee80211_resmgr_mode mode);
    int                                 (*ic_scan_attach)(ieee80211_scanner_t *ss,
                                         struct ieee80211com *ic, 
                                         osdev_t osdev, bool (*is_connected)(wlan_if_t),
                                         bool (*is_txq_empty)(wlan_dev_t),
                                         bool (*has_pending_sends)(wlan_dev_t));
    void                                (*ic_beacon_probe_template_update)(struct ieee80211_node *ni);
    int                                 (*ic_vap_set_param)(struct ieee80211vap *vap,
                                         ieee80211_param param, u_int32_t val);
    void                                (*ic_node_add_wds_entry)(struct ieee80211com *ic,
                                         u_int8_t *dest_mac, u_int8_t *peer_mac);
    void                                (*ic_node_del_wds_entry)(struct ieee80211com *ic,
                                         u_int8_t *dest_mac);

#if ATH_WOW
    /* Wake on Wireless used on clients to wake up the system with a magic packet */
    int                                 (*ic_wow_get_support)(struct ieee80211com *ic);
    int                                 (*ic_wow_enable)(struct ieee80211com *ic, int clearbssid);
    int                                 (*ic_wow_wakeup)(struct ieee80211com *ic);
    int                                 (*ic_wow_add_wakeup_event)(struct ieee80211com *ic, u_int32_t type);
    void                                (*ic_wow_set_events)(struct ieee80211com *ic, u_int32_t);
    int                                 (*ic_wow_get_events)(struct ieee80211com *ic);
    int                                 (*ic_wow_add_wakeup_pattern)
                                            (struct ieee80211com *ic, u_int8_t *, u_int8_t *, u_int32_t);
    int                                 (*ic_wow_remove_wakeup_pattern)(struct ieee80211com *ic, u_int8_t *, u_int8_t *);
    int                                 (*ic_wow_get_wakeup_pattern)
                                            (struct ieee80211com *ic, u_int8_t *,u_int32_t *, u_int32_t *);
    int                                 (*ic_wow_get_wakeup_reason)(struct ieee80211com *ic);
    int                                 (*ic_wow_matchpattern_exact)(struct ieee80211com *ic);
    void                                (*ic_wow_set_duration)(struct ieee80211com *ic, u_int16_t);
    void                                (*ic_wow_set_timeout)(struct ieee80211com *ic, u_int32_t);
	u_int32_t                           (*ic_wow_get_timeout)(struct ieee80211com *ic); 
#endif /* ATH_WOW */

    u_int32_t                            ic_vhtcap;              /* VHT capabilities */
    ieee80211_vht_mcs_set_t              ic_vhtcap_max_mcs;      /* VHT Supported MCS set */
    u_int16_t                            ic_vhtop_basic_mcs;     /* VHT Basic MCS set */
    void                                (*ic_power_attach)(struct ieee80211com *ic);
    void                                (*ic_power_detach)(struct ieee80211com *ic);
    void                                (*ic_power_vattach)(struct ieee80211vap *vap, int fullsleepEnable, 
                                            u_int32_t sleepTimerPwrSaveMax, u_int32_t sleepTimerPwrSave, u_int32_t sleepTimePerf, 
                                            u_int32_t inactTimerPwrsaveMax, u_int32_t inactTimerPwrsave, u_int32_t inactTimerPerf, 
                                            u_int32_t smpsDynamic, u_int32_t pspollEnabled);
    void                                (*ic_power_vdetach)(struct ieee80211vap *vap);
    int                                 (*ic_power_sta_set_pspoll)(struct ieee80211vap *vap, u_int32_t pspoll);
    int                                 (*ic_power_sta_set_pspoll_moredata_handling)(struct ieee80211vap *vap, ieee80211_pspoll_moredata_handling mode);
    u_int32_t                           (*ic_power_sta_get_pspoll)(struct ieee80211vap *vap);
    ieee80211_pspoll_moredata_handling  (*ic_power_sta_get_pspoll_moredata_handling)(struct ieee80211vap *vap);
    int                                 (*ic_power_set_mode)(struct ieee80211vap* vap, ieee80211_pwrsave_mode mode);
    ieee80211_pwrsave_mode              (*ic_power_get_mode)(struct ieee80211vap* vap);
    u_int32_t                           (*ic_power_get_apps_state)(struct ieee80211vap* vap);
    int                                 (*ic_power_set_inactive_time)(struct ieee80211vap* vap, ieee80211_pwrsave_mode mode, u_int32_t inactive_time);
    u_int32_t                           (*ic_power_get_inactive_time)(struct ieee80211vap* vap, ieee80211_pwrsave_mode mode);
    int                                 (*ic_power_force_sleep)(struct ieee80211vap* vap, bool enable);
    int                                 (*ic_power_set_ips_pause_notif_timeout)(struct ieee80211vap* vap, u_int16_t pause_notif_timeout);
    u_int16_t                           (*ic_power_get_ips_pause_notif_timeout)(struct ieee80211vap* vap);
    int                                 (*ic_power_sta_send_keepalive)(struct ieee80211vap *vap);
    int                                 (*ic_power_sta_pause)(struct ieee80211vap *vap, u_int32_t timeout);
    int                                 (*ic_power_sta_unpause)(struct ieee80211vap *vap);
    void                                (*ic_power_sta_event_dtim)(struct ieee80211vap *vap);
    void                                (*ic_power_sta_event_tim)(struct ieee80211vap *vap);
    int                                 (*ic_power_sta_unregister_event_handler)(struct ieee80211vap *vap, ieee80211_sta_power_event_handler evhandler, void *arg);
    int                                 (*ic_power_sta_register_event_handler)(struct ieee80211vap *vap, ieee80211_sta_power_event_handler evhandler, void *arg);
    void                                (*ic_power_sta_tx_start)(struct ieee80211vap *vap);
    void                                (*ic_power_sta_tx_end)(struct ieee80211vap *vap);
     /* ACS APIs for offload architecture */
    void                                (*ic_hal_get_chan_info)(struct ieee80211com *ic, u_int8_t flags);

    /* multicast -> unicast APIs */
    void                                (*ic_mcast_group_update)(struct ieee80211com *ic, int action, int wildcard, u_int8_t *mcast_ip_addr, int mcast_ip_addr_bytes, u_int8_t *ucast_mac_addr);
} IEEE80211COM, *PIEEE80211COM;

typedef  int  (* ieee80211_vap_input_mgmt_filter ) (struct ieee80211_node *ni, wbuf_t wbuf, 
                                                     int subtype, struct ieee80211_rx_status *rs); 
typedef  int  (* ieee80211_vap_output_mgmt_filter ) (wbuf_t wbuf);

#if ATH_WOW
struct ieee80211_wowpattern {
    u_int8_t wp_delayed:1,
             wp_set:1,
             wp_valid:1;
    u_int8_t wp_type;
#define IEEE80211_T_WOW_DEAUTH             1  /* Deauth Pattern */
#define IEEE80211_T_WOW_DISASSOC           2  /* Disassoc Pattern */
#define IEEE80211_T_WOW_MINPATTERN         IEEE80211_T_WOW_DEAUTH
#define IEEE80211_T_WOW_MAXPATTERN         8
};
#endif


typedef struct _APP_IE_LIST_HEAD {
    struct app_ie_entry     *lh_first;    /* first element */
    u_int16_t               total_ie_len;
    bool                    changed;      /* indicates that the IE contents have changed */
} APP_IE_LIST_HEAD;

#ifdef ATH_SUPPORT_HTC
#define NAWDS_LOCK_INIT(_p_lock)                   OS_HTC_LOCK_INIT(_p_lock)
#define NAWDS_WRITE_LOCK(_p_lock , _flags)         OS_HTC_LOCK(_p_lock)
#define NAWDS_WRITE_UNLOCK(_p_lock , _flags)       OS_HTC_UNLOCK(_p_lock)
#define nawds_rwlock_state_t(_lock_state)
typedef htclock_t  nawdslock_t;
#else

#define NAWDS_LOCK_INIT(_p_lock)                   OS_RWLOCK_INIT(_p_lock)
#define NAWDS_WRITE_LOCK(_p_lock , _flags)         OS_RWLOCK_WRITE_LOCK(_p_lock , _flags)
#define NAWDS_WRITE_UNLOCK(_p_lock , _flags)       OS_RWLOCK_WRITE_UNLOCK(_p_lock , _flags)
#define nawds_rwlock_state_t(_lock_state)          rwlock_state_t (_lock_stat)
typedef rwlock_t nawdslock_t;

#endif


#if UMAC_SUPPORT_NAWDS

#ifndef UMAC_MAX_NAWDS_REPEATER
#error NAWDS feature is enabled but UMAC_MAX_NAWDS_REPEATER is not defined
#endif

struct ieee80211_nawds_repeater {
#define NAWDS_REPEATER_CAP_HT20            0x01
#define NAWDS_REPEATER_CAP_HT2040          0x02
#define NAWDS_REPEATER_CAP_DS              0x04
#define NAWDS_REPEATER_CAP_TS              0x08
#define NAWDS_REPEATER_CAP_TXBF            0x10
#define NAWDS_REPEATER_RESERVED            0x11

    /* VHT Capability */
#define NAWDS_REPEATER_CAP_11ACVHT20       0x20
#define NAWDS_REPEATER_CAP_11ACVHT40       0x40
#define NAWDS_REPEATER_CAP_11ACVHT80       0x80

#define NAWDS_INVALID_CAP_MODE             0xA0
    u_int8_t caps;
    u_int8_t mac[IEEE80211_ADDR_LEN];
};

enum ieee80211_nawds_mode {
    IEEE80211_NAWDS_DISABLED = 0,
    IEEE80211_NAWDS_STATIC_REPEATER,
    IEEE80211_NAWDS_STATIC_BRIDGE,
    IEEE80211_NAWDS_LEARNING_REPEATER,
    IEEE80211_NAWDS_LEARNING_BRIDGE,
};

struct ieee80211_nawds {
    nawdslock_t lock;
    enum ieee80211_nawds_mode mode;
    u_int8_t defcaps;
    u_int8_t override;
    struct ieee80211_nawds_repeater repeater[UMAC_MAX_NAWDS_REPEATER];
};
#endif

struct ieee80211_tim_set {
	int set;
	u_int16_t aid;
};

typedef struct ieee80211vap {
    TAILQ_ENTRY(ieee80211vap)         iv_next;    /* list of vap instances */

#if WAR_DELETE_VAP
    void                              *iv_athvap; /* opaque ath vap pointer */
#endif

    struct ieee80211com               *iv_ic;     /* back ptr to common state */
//    struct net_device_stats           iv_devstats; /* interface statistics */
    u_int                             iv_unit;    /* virtual AP unit */

    void                              *iv_priv;   /* for extending vap functionality */
    os_if_t                           iv_ifp;     /* opaque handle to OS-specific interface */
    wlan_event_handler_table          *iv_evtable;/* vap event handlers */
    
    os_handle_t                       iv_mlme_arg[IEEE80211_MAX_VAP_MLME_EVENT_HANDLERS];  /* opaque handle used for mlme event handler */
    wlan_mlme_event_handler_table     *iv_mlme_evtable[IEEE80211_MAX_VAP_MLME_EVENT_HANDLERS];  /* mlme event handlers */
    
    os_handle_t                       iv_misc_arg[IEEE80211_MAX_MISC_EVENT_HANDLERS];
    wlan_misc_event_handler_table     *iv_misc_evtable[IEEE80211_MAX_MISC_EVENT_HANDLERS];

    void*                             iv_event_handler_arg[IEEE80211_MAX_VAP_EVENT_HANDLERS];
    ieee80211_vap_event_handler       iv_event_handler[IEEE80211_MAX_VAP_EVENT_HANDLERS];

    os_if_t                           iv_ccx_arg;  /* opaque handle used for ccx handler */
    wlan_ccx_handler_table            *iv_ccx_evtable;  /* ccx handlers */

    u_int32_t                         iv_debug;   /* debug msg flags */

    struct asf_print_ctrl             iv_print;

    u_int8_t                          iv_myaddr[IEEE80211_ADDR_LEN]; /* current mac address */
    u_int8_t                          iv_my_hwaddr[IEEE80211_ADDR_LEN]; /* mac address from EEPROM */

    enum ieee80211_opmode             iv_opmode;  /* operation mode */
    int                               iv_scan_priority_base;  /* Base value used to determine priority of scans requested by this VAP */
    u_int32_t                         iv_create_flags;   /* vap create flags */

    u_int32_t                         iv_flags;   /* state flags */
    u_int32_t                         iv_flags_ext;   /* extension of state flags */
/* not multiple thread safe */
    u_int32_t                         iv_deleted:1,  /* if the vap deleted by user */
                                      iv_active:1,   /* if the vap is active */
                                      iv_scanning:1, /* if the vap is scanning */
                                      iv_smps:1,     /* if the vap is in static mimo ps state */
                                      iv_ready:1,    /* if the vap is ready to send receive data */
                                      iv_standby:1,  /* if the vap is temporarily stopped */
                                      iv_cansleep:1, /* if the vap can sleep*/
                                      iv_sw_bmiss:1, /* use sw bmiss timer */
                                      iv_copy_beacon:1, /* enable beacon copy */
                                      iv_wapi:1,
                                      iv_sta_fwd:1, /* set to enable sta-fws fweature */
                                      iv_dynamic_mimo_ps, /* dynamic mimo ps enabled */
                                      iv_doth:1,     /* 802.11h enabled */
                                      iv_country_ie:1,  /* Country IE enabled */
                                      iv_wme:1, /* wme enabled */
                                      iv_off_channel_support, /* Off-channel support enabled */ //subrat
                                      iv_dfswait:1,   /* if the vap is in dfswait state */
                                      iv_erpupdate:1, /* if the vap has erp update set */
                                      iv_needs_scheduler:1, /* this vap needs scheduler for off channel operation */
                                      
                                      iv_no_multichannel:1, /*does not want to trigger multi channel operation 
                                                            instead follow master vaps channel (for AP/GO Vaps) */
                                      
                                      iv_vap_ind:1,   /* if the vap has wds independance set */
                                      iv_forced_sleep:1,        /*STA in forced sleep set PS bit for all outgoing frames */
                                      iv_bssload:1, /* QBSS load IE enabled */
                                      iv_bssload_update:1, /* update bssload IE in beacon */
                                      iv_rrm:1, /* RRM Capabilities */
                                      iv_wnm:1, /* WNM Capabilities */
                                      iv_proxyarp:1, /* WNM Proxy ARP Capabilities */
                                      iv_dgaf_disable:1, /* Hotspot 2.0 DGAF Disable bit */
                                      iv_ap_reject_dfs_chan:1,  /* SoftAP to reject resuming in DFS channels */
                                      iv_smartnet_enable:1,  /* STA SmartNet enabled */
                                      iv_trigger_mlme_resp:1,  /* Trigger mlme response */
                                      iv_mfp_test:1,   /* test flag for MFP */
                                      iv_sgi:1,        /* Short Guard Interval Enable:1 Disable:0 */
                                      iv_ldpc:1;       /* LDPC Enable:1 Disable:0 */
        
    enum ieee80211_state    iv_state;    /* state machine state */


    u_int8_t                          iv_rescan; 
/* multiple thread safe */
    u_int32_t                         iv_caps;    /* capabilities */
    u_int16_t                         iv_ath_cap; /* Atheros adv. capablities */
    u_int8_t                          iv_chanchange_count; /* 11h counter for channel change */
    int                               iv_mcast_rate; /* Multicast rate (Kbps) */
    int                               iv_mcast_fixedrate; /* fixed rate for multicast */
    u_int32_t                         iv_node_count; /* node count */
    atomic_t                          iv_rx_gate; /* pending rx threads */
    struct ieee80211_stats            iv_stats; /* for backward compatibility */
    struct ieee80211_mac_stats        iv_unicast_stats;   /* mac statistics for unicast frames */
    struct ieee80211_mac_stats        iv_multicast_stats; /* mac statistics for multicast frames */
    struct tkip_countermeasure        iv_unicast_counterm;  /* unicast tkip countermeasure */
    struct tkip_countermeasure        iv_multicast_counterm;  /* unicast tkip countermeasure */

    spinlock_t                        iv_lock; /* lock to protect access to vap object data */
    u_int32_t                         *iv_aid_bitmap; /* association id map */
    u_int16_t                         iv_max_aid;
    u_int16_t                         iv_sta_assoc;   /* stations associated */
    u_int16_t                         iv_ps_sta;  /* stations in power save */
    u_int16_t                         iv_ps_pending;  /* ps sta's w/ pending frames */
    u_int8_t                          iv_dtim_period; /* DTIM period */
    u_int8_t                          iv_dtim_count;  /* DTIM count from last bcn */
    u_int8_t                          iv_atim_window; /* ATIM window */
    u_int8_t                          *iv_tim_bitmap; /* power-save stations w/ data*/
    u_int16_t                         iv_tim_len;     /* ic_tim_bitmap size (bytes) */
                                      /* set/unset aid pwrsav state */
    void                              (*iv_set_tim)(struct ieee80211_node *, int,bool isr_context);
    int                               (*iv_alloc_tim_bitmap)(struct ieee80211vap *vap);
    struct ieee80211_node             *iv_bss;    /* information for this node */
    struct ieee80211_rateset          iv_op_rates[IEEE80211_MODE_MAX]; /* operational rate set by os */
    struct ieee80211_fixed_rate       iv_fixed_rate;  /* 802.11 rate or -1 */
    u_int32_t                         iv_fixed_rateset;  /* 802.11 rate set or -1(invalid) */
    u_int32_t                         iv_fixed_retryset; /* retries */
    u_int16_t                         iv_rtsthreshold;
    u_int16_t                         iv_fragthreshold;
    u_int16_t                         iv_def_txkey;   /* default/group tx key index */
#if ATH_SUPPORT_AP_WDS_COMBO
    u_int8_t                          iv_no_beacon;   /* VAP does not transmit beacon/probe resp. */
#endif
    struct ieee80211_key              iv_nw_keys[IEEE80211_WEP_NKID];

    struct ieee80211_key              iv_igtk_key;

    struct ieee80211_rsnparms         iv_rsn;         /* rsn information */
    unsigned long                     iv_assoc_comeback_time;    /* assoc comeback information */

    ieee80211_privacy_exemption       iv_privacy_filters[IEEE80211_MAX_PRIVACY_FILTERS];    /* privacy filters */
    u_int32_t                         iv_num_privacy_filters;
    ieee80211_pmkid_entry             iv_pmkid_list[IEEE80211_MAX_PMKID];
    u_int16_t                         iv_pmkid_count;
    u_int8_t                          iv_wps_mode;    /* WPS mode */
    u_int8_t                          iv_wep_mbssid;    /* force multicast wep keys in first 4 entries 0=yes, 1=no */

    enum ieee80211_phymode            iv_des_mode; /* desired wireless mode for this interface. */
    u_int16_t                         iv_des_modecaps;   /* set of desired phy modes for this VAP */
    enum ieee80211_phymode            iv_cur_mode; /* current wireless mode for this interface. */
    struct ieee80211_channel          *iv_des_chan[IEEE80211_MODE_MAX]; /* desired channel for each PHY */
    struct ieee80211_channel          *iv_bsschan;   /* bss channel */
    u_int8_t                          iv_des_ibss_chan;   /* desired ad-hoc channel */
    u_int8_t                          iv_rateCtrlEnable;  /* enable rate control */
    u_int8_t                          iv_rc_txrate_fast_drop_en;    /* enable tx rate fast drop*/
    int                               iv_des_nssid;       /* # desired ssids */
    ieee80211_ssid                    iv_des_ssid[IEEE80211_SCAN_MAX_SSID];/* desired ssid list */

    int                               iv_bmiss_count;
    int                               iv_bmiss_count_for_reset;
    int                               iv_bmiss_count_max;
    systime_t                         iv_last_beacon_time;         /* absolute system time, not TSF */
    systime_t                         iv_last_directed_frame;      /* absolute system time; not TSF */
    systime_t                         iv_last_ap_frame;            /* absolute system time; not TSF */
    systime_t                         iv_last_traffic_indication;  /* absolute system time; not TSF */
    systime_t                         iv_lastdata;                 /* absolute system time; not TSF */
    u_int64_t                         iv_txrxbytes;                /* No. of tx/rx bytes  */
    ieee80211_power_t                 iv_power;                    /* handle private to power module */
    ieee80211_sta_power_t             iv_pwrsave_sta;
    ieee80211_pwrsave_smps_t          iv_pwrsave_smps;
    u_int16_t                         iv_keep_alive_timeout;       /* keep alive time out */
    u_int16_t                         iv_inact_count;               /* inactivity count */
    u_int8_t                          iv_smps_rssithresh;
    u_int8_t                          iv_smps_datathresh;
    unsigned long                     iv_pending_sends;

    u_int8_t                          iv_lastbcn_phymode_mismatch;        /* Phy mode mismatch between scan entry, BSS */


    /* NEW APP IE implementation. Note, we need to obselete the old one */
    APP_IE_LIST_HEAD                  iv_app_ie_list[IEEE80211_APPIE_MAX_FRAMES];

    /* TODO: we need to obselete the use of these 2 fields */
    /* app ie buffer */
    struct ieee80211_app_ie_t         iv_app_ie[IEEE80211_APPIE_MAX_FRAMES];
    u_int16_t                         iv_app_ie_maxlen[IEEE80211_APPIE_MAX_FRAMES];
 
    IEEE80211_VEN_IE                 *iv_venie;
    
    /* opt ie buffer - currently used for probereq and assocreq */
    struct ieee80211_app_ie_t         iv_opt_ie;
    u_int16_t                         iv_opt_ie_maxlen;

    /* Copy of the beacon frame */
    u_int8_t                          *iv_beacon_copy_buf;
    int                               iv_beacon_copy_len;

    /* country ie data */
    u_int16_t                         iv_country_ie_chanflags;
    struct ieee80211_ie_country       iv_country_ie_data; /* country info element */

    u_int8_t                      iv_mlmeconnect;     /* Assoc state machine roaming mode */

    /* U-APSD Settings */
    u_int8_t                          iv_uapsd;
    u_int8_t                          iv_wmm_enable;
    u_int8_t                          iv_wmm_power_save;

    ieee80211_mlme_priv_t             iv_mlme_priv;    /* opaque handle to mlme private information */ 

    ieee80211_scan_table_t            iv_scan_table;          /* bss list */
    ieee80211_aplist_config_t         iv_aplist_config;

    ieee80211_candidate_aplist_t      iv_candidate_aplist;    /* opaque handle to aplist private information */ 

    ieee80211_resmgr_vap_priv_t       iv_resmgr_priv;         /* opaque handle with resource manager private info */ 

    ieee80211_vap_pause_info          iv_pause_info;          /* pause private info */ 

    ieee80211_vap_state_info          iv_state_info;          /* vap private state information */ 
    ieee80211_txrx_event_info         iv_txrx_event_info;     /* txrx event handler private data */
    ieee80211_acl_t                   iv_acl;   /* opaque handle to acl */ 
    ieee80211_vap_ath_info_t          iv_vap_ath_info_handle; /* opaque handle for VAP_ATH_INFO */
    enum ieee80211_protmode           iv_protmode;            /* per vap 802.11g protection mode */

    u_int8_t                          iv_ht40_intolerant;
    u_int8_t                          iv_chwidth;
    u_int8_t                          iv_chextoffset;
    u_int8_t                          iv_disable_HTProtection;
    u_int32_t                         iv_chscaninit;
    int                               iv_proxySTA;

#if UMAC_SUPPORT_TDLS
    /* TDLS information */
    u_int32_t                         iv_tdls_macaddr1;
    u_int32_t                         iv_tdls_macaddr2;
    u_int32_t                         iv_tdls_action;
    u_int8_t                          iv_tdls_dialog_token;
    u_int8_t                          iv_tdls_channel_switch_control;
    struct ieee80211_tdls             *iv_tdlslist;
    ieee80211_tdls_ops_t              iv_tdls_ops;	
#endif /* UMAC_SUPPORT_TDLS */

    /* Optional feature: mcast enhancement */
#if ATH_SUPPORT_IQUE
    struct ieee80211_ique_me          *iv_me;
    struct ieee80211_hbr_list         *iv_hbr_list;
#endif
    struct ieee80211_ique_ops         iv_ique_ops;  
    struct ieee80211_beacon_info      iv_beacon_info[100]; /*BSSID and RSSI info*/
    u_int8_t                          iv_beacon_info_count;
    u_int8_t                          iv_esslen;
    u_int8_t                          iv_essid[IEEE80211_NWID_LEN+1];
    struct ieee80211_ibss_peer_list   iv_ibss_peer[8]; /*BSSID and RSSI info*/
    u_int8_t                          iv_ibss_peer_count;

    /* channel_change_done is a flag value used to indicate that a channel 
     * change happend for this VAP. This information (for now) is used to update 
     * the beacon information and then reset. This is needed in case of DFS channel change 
     */
    u_int8_t channel_change_done; 
    
    ieee80211_vap_input_mgmt_filter   iv_input_mgmt_filter;   /* filter  input mgmt frames */
    ieee80211_vap_output_mgmt_filter  iv_output_mgmt_filter;  /* filter outpur mgmt frames */
    int                               (*iv_up)(struct ieee80211vap *);
    int                               (*iv_join)(struct ieee80211vap *);
    int                               (*iv_down)(struct ieee80211vap *);
    int                               (*iv_listen)(struct ieee80211vap *);
    int                               (*iv_stopping)(struct ieee80211vap *);
    int                               (*iv_dfs_cac)(struct ieee80211vap *);
    void                              (*iv_update_ps_mode)(struct ieee80211vap *);
                                
    int                               (*iv_key_alloc)(struct ieee80211vap *,
                                                      struct ieee80211_key *);
    int                               (*iv_key_delete)(struct ieee80211vap *, 
                                                       const struct ieee80211_key *,
                                                       struct ieee80211_node *);
    int                               (*iv_key_map)(struct ieee80211vap *, 
                                                     const struct ieee80211_key *,
                                                     const u_int8_t bssid[IEEE80211_ADDR_LEN],
                                                     struct ieee80211_node *);
    int                               (*iv_key_set)(struct ieee80211vap *,
                                                    const struct ieee80211_key *,
                                                    const u_int8_t mac[IEEE80211_ADDR_LEN]);
    void                              (*iv_key_update_begin)(struct ieee80211vap *);
    void                              (*iv_key_update_end)(struct ieee80211vap *);
    void                              (*iv_update_node_txpow)(struct ieee80211vap *, u_int16_t , u_int8_t *);
    int                               (*iv_set_proxysta)(struct ieee80211vap *vap, int enable);
    
    int                               (*iv_reg_vap_ath_info_notify)(struct ieee80211vap *,
                                                                    ath_vap_infotype, 
                                                                    ieee80211_vap_ath_info_notify_func,
                                                                    void *);
    int                               (*iv_vap_ath_info_update_notify)(struct ieee80211vap *,
                                                                       ath_vap_infotype);
    int                               (*iv_dereg_vap_ath_info_notify)(struct ieee80211vap *);
    int                               (*iv_vap_ath_info_get)(struct ieee80211vap *, 
                                                             ath_vap_infotype, 
                                                             u_int32_t *, u_int32_t *);
#if ATH_ANT_DIV_COMB
    void                              (*iv_sa_normal_scan_handle)(struct ieee80211vap *, enum ieee80211_state_event);
#endif
    int                               (*iv_vap_send)(struct ieee80211vap *, wbuf_t);
    void*                             (*iv_vap_get_ol_data_handle)(struct ieee80211vap *);
#if ATH_WOW
    u_int8_t                          iv_wowflags;         /* Flags for wow */
#define IEEE80211_F_WOW_DEAUTH             1               /* Deauth Pattern */
#define IEEE80211_F_WOW_DISASSOC           2               /* Disassoc Pattern */
    struct ieee80211_wowpattern       iv_patterns[8];      /* Patterns status */
#if ATH_WOW_OFFLOAD
    int                               (*iv_vap_wow_offload_rekey_misc_info_set)(struct ieee80211com *,
                                                            struct wow_offload_misc_info *);
    int                               (*iv_vap_wow_offload_info_get)(struct ieee80211com *,
                                                            void *buf, u_int32_t param);
    int                               (*iv_vap_wow_offload_txseqnum_update)(struct ieee80211com *,
                                                            struct ieee80211_node *ni, u_int32_t tidno, u_int16_t seqnum);
#endif /* ATH_WOW_OFFLOAD */
#endif
    u_int8_t                          iv_ccmpsw_seldec;  /* Enable/Disable encrypt/decrypt of frames selectively ( frames with KEYMISS) */    
    u_int16_t                         iv_mgt_rate;       /* rate to be used for management rates */  
#if UMAC_SUPPORT_NAWDS
    struct ieee80211_nawds            iv_nawds;
#endif
#if WDS_VENDOR_EXTENSION
    u_int8_t                          iv_wds_rx_policy;
#define WDS_POLICY_RX_UCAST_4ADDR       0x01
#define WDS_POLICY_RX_MCAST_4ADDR       0x02
#define WDS_POLICY_RX_DEFAULT           0x03
#define WDS_POLICY_RX_MASK              0x03
#endif

    ieee80211_vap_tsf_offset          iv_tsf_offset;    /* TSF-related data utilized for concurrent multi-channel operations */
#if defined(ATH_CWMIN_WORKAROUND) && defined(ATH_SUPPORT_HTC) && defined(ATH_USB)
    u_int8_t                          iv_vi_need_cwmin_workaround;
#endif /*ATH_CWMIN_WORKAROUND ,ATH_SUPPORT_HTC ,ATH_USB*/
#ifdef ATH_SUPPORT_QUICK_KICKOUT
    u_int8_t						  iv_sko_th;        /* station kick out threshold */ 
#endif /*ATH_SUPPORT_QUICK_KICKOUT*/   
    struct ieee80211_chanutil_info     chanutil_info; /* Channel Utilization information */
#if UMAC_SUPPORT_CHANUTIL_MEASUREMENT
    u_int8_t                           iv_chanutil_enab;
#endif /* UMAC_SUPPORT_CHANUTIL_MEASUREMENT */

    /* add flag to enable/disable auto-association */
    u_int8_t    auto_assoc;
                              /* in activity timeouts */
    u_int8_t    iv_inact_init; 
    u_int8_t    iv_inact_auth;
    u_int8_t    iv_inact_run;
    u_int8_t    iv_inact_probe;
#if ATH_SW_WOW
    u_int8_t                          iv_sw_wow;           /* Flags for sw wow */
#endif
#ifdef ATH_SUPPORT_TxBF
    u_int8_t    iv_txbfmode;
    u_int8_t    iv_autocvupdate;
    u_int8_t    iv_cvupdateper;
#endif
    struct ieee80211_node             *iv_ni;
    struct ieee80211_channel          *iv_cswitch_chan;
    u_int8_t                          iv_cswitch_rxd;
    os_timer_t                        iv_cswitch_timer;
#ifdef MAGPIE_HIF_GMAC
    u_int8_t    iv_chanswitch;
#endif    
#if ATH_SUPPORT_WAPI
    u32    iv_wapi_urekey_pkts;/*wapi unicast rekey packets, 0 for disable*/
    u32    iv_wapi_mrekey_pkts;/*wapi muiticast rekey packets, 0 for disable*/	
#endif

#ifdef  ATH_SUPPORT_AOW
    ieee80211_vap_aow_info_t    iv_aow;
#endif  /* ATH_SUPPORT_AOW */

#if ATH_SUPPORT_IBSS_DFS
    struct ieee80211_ibssdfs_ie iv_ibssdfs_ie_data; /* IBSS DFS element */
    u_int8_t                    iv_measrep_action_count_per_tbtt;
    u_int8_t                    iv_csa_action_count_per_tbtt;
    u_int8_t                    iv_ibssdfs_recovery_count;
    u_int8_t                    iv_ibssdfs_state;
    u_int8_t                    iv_ibss_dfs_csa_threshold;
    u_int8_t                    iv_ibss_dfs_csa_measrep_limit;
    u_int8_t                    iv_ibss_dfs_enter_recovery_threshold_in_tbtt;
#endif /* ATH_SUPPORT_IBSS_DFS */

#define IEEE80211_SCAN_BAND_ALL            (0)
#define IEEE80211_SCAN_BAND_2G_ONLY        (1)
#define IEEE80211_SCAN_BAND_5G_ONLY        (2)
#define IEEE80211_SCAN_BAND_CHAN_ONLY      (3)
    u_int8_t                    iv_scan_band;       /* only scan channels of requested band */
    u_int32_t                   iv_scan_chan;       /* only scan specific channel */

#define IEEE80211_SUPPORTED_MAXIMUM_CLIENTS   0xFFFF          
    u_int8_t                    iv_is_sta_limit_reached;        /* flag to specify soft limit maximu accepted clients */
    u_int16_t                   iv_maximum_clients_number;      /* to add soft limit for maximum client numbers */
    struct ieee80211_quiet_param *iv_quiet;
#if ATH_SUPPORT_IBSS_NETLINK_NOTIFICATION
#define IBSS_RSSI_CLASS_MAX 7
    u_int8_t			iv_ibss_rssi_monitor;
    u_int8_t			iv_ibss_rssi_class[IBSS_RSSI_CLASS_MAX];
    u_int8_t			iv_ibss_rssi_hysteresis;
#endif
#if ATH_SUPPORT_LINKDIAG_EXT
    struct ath_linkdiag *iv_ald;
#endif
#if UMAC_SUPPORT_RRM
    ieee80211_rrm_t             rrm; /* handle for rrm  */
#endif
#if ATH_SUPPORT_FLOWMAC_MODULE
    int                         iv_dev_stopped;
    int                         iv_flowmac;
#endif
#if UMAC_SUPPORT_WNM
    ieee80211_wnm_t             wnm; /* handle for wnm  */
#endif
#if ATH_SUPPORT_HS20
    u_int8_t                    iv_hessid[IEEE80211_ADDR_LEN];
    u_int8_t                    iv_access_network_type;
    u_int32_t                   iv_hotspot_xcaps;
#endif
    u_int8_t iv_wep_keycache; /* static wep keys are allocated in first four slots in keycahe */
#if ATH_SUPPORT_WPA_SUPPLICANT_CHECK_TIME
    int                 iv_rejoint_attemp_time;
#endif  
#if ATH_SUPPORT_WIFIPOS
    int                         iv_wakeup;
    u_int64_t                   iv_tsf_sync;
    u_int64_t                   iv_local_tsf_tstamp;
#endif
    u_int8_t iv_send_deauth; /* for sending deauth instead of disassoc while doing apdown */
    struct ieee80211_tim_set iv_tim_infor;
#if UMAC_SUPPORT_WNM
    u_int16_t           iv_wnmsleep_intval;
#endif
#if ATH_SUPPORT_SIMPLE_CONFIG_EXT
    u_int8_t            iv_nopbn;  /* no push button notification */
#endif
#if UMAC_PER_PACKET_DEBUG
#define PROC_FNAME_SIZE 20
    int16_t iv_userrate;
    int8_t iv_userretries;
    int8_t iv_usertxpower;
    int8_t iv_usertxchainmask;
    struct proc_dir_entry   *iv_proc_entry;
    struct proc_dir_entry   *iv_proc_root;
    u_int8_t                 iv_proc_fname[PROC_FNAME_SIZE];
#endif

    /* vap tx dynamic aponly mode control */ 
    bool                    iv_aponly;
#if ATH_PERF_PWR_OFFLOAD
    void                        *iv_txrx_handle;
#endif
    u_int8_t                   iv_vht_fixed_mcs;        /* VHT Fixed MCS Index */
    u_int8_t                   iv_nss;                  /* Spatial Stream Count */
    u_int8_t                   iv_tx_stbc;              /* TX STBC Enable:1 Disable:0 */
    u_int8_t                   iv_rx_stbc;              /* RX STBC Enable:(1,2,3) Disable:0 */
    u_int8_t                   iv_opmode_notify;        /* Op Mode notification On:1 Off:0 */
    u_int8_t                   iv_rtscts_enabled;       /* RTS-CTS 1: enabled, 0: disabled */
    int                        iv_bcast_fixedrate;      /* Bcast data rate */
    u_int16_t                  iv_vht_mcsmap;           /* VHT MCS MAP */
} IEEE80211VAP, *PIEEE80211VAP;

#ifndef __ubicom32__
#define IEEE80211_ADDR_EQ(a1,a2)        (OS_MEMCMP(a1, a2, IEEE80211_ADDR_LEN) == 0)
#define IEEE80211_ADDR_COPY(dst,src)    OS_MEMCPY(dst, src, IEEE80211_ADDR_LEN)
#else
#define IEEE80211_ADDR_EQ(a1,a2)        (OS_MACCMP(a1, a2) == 0)
#define IEEE80211_ADDR_COPY(dst,src)    OS_MACCPY(dst, src)
#endif
#define IEEE80211_ADDR_IS_VALID(a)      (a[0]!=0 || a[1]!=0 ||a[2]!= 0 || a[3]!=0 || a[4]!=0 || a[5]!=0)
#define IEEE80211_SSID_IE_EQ(a1,a2)     (((((char*) a1)[1]) == (((char*) a2)[1])) && (OS_MEMCMP(a1, a2, 2+(((char*) a1)[1])) == 0))

/* ic_flags */
#define IEEE80211_F_FF                   0x00000001  /* CONF: ATH FF enabled */
#define IEEE80211_F_TURBOP               0x00000002  /* CONF: ATH Turbo enabled*/
#define IEEE80211_F_PROMISC              0x00000004  /* STATUS: promiscuous mode */
#define IEEE80211_F_ALLMULTI             0x00000008  /* STATUS: all multicast mode */
/* NB: this is intentionally setup to be IEEE80211_CAPINFO_PRIVACY */
#define IEEE80211_F_PRIVACY              0x00000010  /* CONF: privacy enabled */
#define IEEE80211_F_PUREG                0x00000020  /* CONF: 11g w/o 11b sta's */
#define IEEE80211_F_SCAN                 0x00000080  /* STATUS: scanning */
#define IEEE80211_F_SIBSS                0x00000200  /* STATUS: start IBSS */
/* NB: this is intentionally setup to be IEEE80211_CAPINFO_SHORT_SLOTTIME */
#define IEEE80211_F_SHSLOT               0x00000400  /* STATUS: use short slot time*/
#define IEEE80211_F_PMGTON               0x00000800  /* CONF: Power mgmt enable */
#define IEEE80211_F_DESBSSID             0x00001000  /* CONF: des_bssid is set */
#define IEEE80211_F_BGSCAN               0x00004000  /* CONF: bg scan enabled */
#define IEEE80211_F_SWRETRY              0x00008000  /* CONF: sw tx retry enabled */
#define IEEE80211_F_TXPOW_FIXED          0x00010000  /* TX Power: fixed rate */
#define IEEE80211_F_IBSSON               0x00020000  /* CONF: IBSS creation enable */
#define IEEE80211_F_SHPREAMBLE           0x00040000  /* STATUS: use short preamble */
#define IEEE80211_F_DATAPAD              0x00080000  /* CONF: do alignment pad */
#define IEEE80211_F_USEPROT              0x00100000  /* STATUS: protection enabled */
#define IEEE80211_F_USEBARKER            0x00200000  /* STATUS: use barker preamble*/
#define IEEE80211_F_TIMUPDATE            0x00400000  /* STATUS: update beacon tim */
#define IEEE80211_F_WPA1                 0x00800000  /* CONF: WPA enabled */
#define IEEE80211_F_WPA2                 0x01000000  /* CONF: WPA2 enabled */
#define IEEE80211_F_WPA                  0x01800000  /* CONF: WPA/WPA2 enabled */
#define IEEE80211_F_DROPUNENC            0x02000000  /* CONF: drop unencrypted */
#define IEEE80211_F_COUNTERM             0x04000000  /* CONF: TKIP countermeasures */
#define IEEE80211_F_HIDESSID             0x08000000  /* CONF: hide SSID in beacon */
#define IEEE80211_F_NOBRIDGE             0x10000000  /* CONF: disable internal bridge */
#define IEEE80211_F_WMEUPDATE            0x20000000  /* STATUS: update beacon wme */
#define IEEE80211_F_COEXT_DISABLE        0x40000000  /* CONF: DISABLE 2040 coexistance */
#define IEEE80211_F_CHANSWITCH           0x80000000  /* force chanswitch */

/* ic_flags_ext and/or iv_flags_ext */
#define IEEE80211_FEXT_WDS                 0x00000001  /* CONF: 4 addr allowed */
#define IEEE80211_FEXT_COUNTRYIE           0x00000002  /* CONF: enable country IE */
#define IEEE80211_FEXT_SCAN_PENDING        0x00000004  /* STATE: scan pending */
#define IEEE80211_FEXT_BGSCAN              0x00000008  /* STATE: enable full bgscan completion */
#define IEEE80211_FEXT_UAPSD               0x00000010  /* CONF: enable U-APSD */
#define IEEE80211_FEXT_SLEEP               0x00000020  /* STATUS: sleeping */
#define IEEE80211_FEXT_EOSPDROP            0x00000040  /* drop uapsd EOSP frames for test */
#define IEEE80211_FEXT_MARKDFS             0x00000080  /* Enable marking of dfs interfernce */
#define IEEE80211_FEXT_REGCLASS            0x00000100  /* CONF: send regclassids in country ie */
#define IEEE80211_FEXT_BLKDFSCHAN          0x00000200  /* CONF: block the use of DFS channels */
#define IEEE80211_FEXT_CCMPSW_ENCDEC       0x00000400 /* enable or disable s/w ccmp encrypt decrypt support */
#define IEEE80211_FEXT_HIBERNATION         0x00000800  /* STATE: hibernating */
#define IEEE80211_FEXT_SAFEMODE            0x00001000  /* CONF: MSFT safe mode         */
#define IEEE80211_FEXT_DESCOUNTRY          0x00002000  /* CONF: desired country has been set */
#define IEEE80211_FEXT_PWRCNSTRIE          0x00004000  /* CONF: enable power capability or contraint IE */
#define IEEE80211_FEXT_DOT11D              0x00008000  /* STATUS: 11D in used */
#define IEEE80211_FEXT_RADAR               0x00010000  /* STATUS: 11D channel-switch detected */
#define IEEE80211_FEXT_AMPDU               0x00020000  /* CONF: A-MPDU supported */
#define IEEE80211_FEXT_AMSDU               0x00040000  /* CONF: A-MSDU supported */
#define IEEE80211_FEXT_HTPROT              0x00080000  /* CONF: HT traffic protected */
#define IEEE80211_FEXT_RESET               0x00100000  /* CONF: Reset once */
#define IEEE80211_FEXT_APPIE_UPDATE        0x00200000  /* STATE: beacon APP IE updated */
#define IEEE80211_FEXT_IGNORE_11D_BEACON   0x00400000  /* CONF: ignore 11d beacon */
#define IEEE80211_FEXT_WDS_AUTODETECT      0x00800000  /* CONF: WDS auto Detect/DELBA */
#define IEEE80211_FEXT_PUREB               0x01000000  /* 11b only without 11g stations */
#define IEEE80211_FEXT_HTRATES             0x02000000  /* disable HT rates */
#define IEEE80211_FEXT_HTVIE               0x04000000  /* HT CAP IE present */
//#define IEEE80211_FEXT_DUPIE             0x08000000  /* dupie (ANA,pre ANA ) */
#ifdef ATH_EXT_AP
#define IEEE80211_FEXT_AP                  0x08000000  /* Extender AP */
#endif
#define IEEE80211_FEXT_DELIVER_80211       0x10000000  /* CONF: deliver rx frames with 802.11 header */
#define IEEE80211_FEXT_SEND_80211          0x20000000  /* CONF: os sends down tx frames with 802.11 header */
#define IEEE80211_FEXT_WDS_STATIC          0x40000000  /* CONF: statically configured WDS */
#define IEEE80211_FEXT_PURE11N             0x80000000  /* CONF: pure 11n mode */

/* ic_caps */
#define IEEE80211_C_WEP                  0x00000001  /* CAPABILITY: WEP available */
#define IEEE80211_C_TKIP                 0x00000002  /* CAPABILITY: TKIP available */
#define IEEE80211_C_AES                  0x00000004  /* CAPABILITY: AES OCB avail */
#define IEEE80211_C_AES_CCM              0x00000008  /* CAPABILITY: AES CCM avail */
#define IEEE80211_C_HT                   0x00000010  /* CAPABILITY: 11n HT available */
#define IEEE80211_C_CKIP                 0x00000020  /* CAPABILITY: CKIP available */
#define IEEE80211_C_FF                   0x00000040  /* CAPABILITY: ATH FF avail */
#define IEEE80211_C_TURBOP               0x00000080  /* CAPABILITY: ATH Turbo avail*/
#define IEEE80211_C_IBSS                 0x00000100  /* CAPABILITY: IBSS available */
#define IEEE80211_C_PMGT                 0x00000200  /* CAPABILITY: Power mgmt */
#define IEEE80211_C_HOSTAP               0x00000400  /* CAPABILITY: HOSTAP avail */
#define IEEE80211_C_AHDEMO               0x00000800  /* CAPABILITY: Old Adhoc Demo */
#define IEEE80211_C_SWRETRY              0x00001000  /* CAPABILITY: sw tx retry */
#define IEEE80211_C_TXPMGT               0x00002000  /* CAPABILITY: tx power mgmt */
#define IEEE80211_C_SHSLOT               0x00004000  /* CAPABILITY: short slottime */
#define IEEE80211_C_SHPREAMBLE           0x00008000  /* CAPABILITY: short preamble */
#define IEEE80211_C_MONITOR              0x00010000  /* CAPABILITY: monitor mode */
#define IEEE80211_C_TKIPMIC              0x00020000  /* CAPABILITY: TKIP MIC avail */
#define IEEE80211_C_WAPI                 0x00040000  /* CAPABILITY: ATH WAPI avail */
#define IEEE80211_C_WDS_AUTODETECT       0x00080000  /* CONF: WDS auto Detect/DELBA */
#define IEEE80211_C_WPA1                 0x00800000  /* CAPABILITY: WPA1 avail */
#define IEEE80211_C_WPA2                 0x01000000  /* CAPABILITY: WPA2 avail */
#define IEEE80211_C_WPA                  0x01800000  /* CAPABILITY: WPA1+WPA2 avail*/
#define IEEE80211_C_BURST                0x02000000  /* CAPABILITY: frame bursting */
#define IEEE80211_C_WME                  0x04000000  /* CAPABILITY: WME avail */
#define IEEE80211_C_WDS                  0x08000000  /* CAPABILITY: 4-addr support */
#define IEEE80211_C_WME_TKIPMIC          0x10000000  /* CAPABILITY: TKIP MIC for QoS frame */
#define IEEE80211_C_BGSCAN               0x20000000  /* CAPABILITY: bg scanning */
#define IEEE80211_C_UAPSD                0x40000000  /* CAPABILITY: UAPSD */
#define IEEE80211_C_DOTH                 0x80000000  /* CAPABILITY: enabled 11.h */

/* XXX protection/barker? */

#define IEEE80211_C_CRYPTO         0x0000002f  /* CAPABILITY: crypto alg's */

/* ic_caps_ext */
#define IEEE80211_CEXT_FASTCC      0x00000001  /* CAPABILITY: fast channel change */
#define IEEE80211_CEXT_P2P              0x00000002  /* CAPABILITY: P2P */
#define IEEE80211_CEXT_MULTICHAN        0x00000004  /* CAPABILITY: Multi-Channel Operations */
#define IEEE80211_CEXT_PERF_PWR_OFLD    0x00000008  /* CAPABILITY: the device supports perf and power offload */
#define IEEE80211_CEXT_11AC             0x00000010  /* CAPABILITY: the device supports 11ac */

/* Accessor APIs */

#define IEEE80211_IS_UAPSD_ENABLED(_ic)             ((_ic)->ic_flags_ext & IEEE80211_FEXT_UAPSD)
#define IEEE80211_UAPSD_ENABLE(_ic)                 ((_ic)->ic_flags_ext |= IEEE80211_FEXT_UAPSD)
#define IEEE80211_UAPSD_DISABLE(_ic)                ((_ic)->ic_flags_ext &= ~IEEE80211_FEXT_UAPSD)

#define IEEE80211_IS_SLEEPING(_ic)                  ((_ic)->ic_flags_ext & IEEE80211_FEXT_SLEEP)
#define IEEE80211_GOTOSLEEP(_ic)                    ((_ic)->ic_flags_ext |= IEEE80211_FEXT_SLEEP)
#define IEEE80211_WAKEUP(_ic)                       ((_ic)->ic_flags_ext &= ~IEEE80211_FEXT_SLEEP)

#define IEEE80211_IS_HIBERNATING(_ic)               ((_ic)->ic_flags_ext & IEEE80211_FEXT_HIBERNATION)
#define IEEE80211_GOTOHIBERNATION(_ic)              ((_ic)->ic_flags_ext |= IEEE80211_FEXT_HIBERNATION)
#define IEEE80211_WAKEUPFROMHIBERNATION(_ic)        ((_ic)->ic_flags_ext &= ~IEEE80211_FEXT_HIBERNATION)

#define IEEE80211_IS_PROTECTION_ENABLED(_ic)        ((_ic)->ic_flags & IEEE80211_F_USEPROT)
#define IEEE80211_ENABLE_PROTECTION(_ic)            ((_ic)->ic_flags |= IEEE80211_F_USEPROT)
#define IEEE80211_DISABLE_PROTECTION(_ic)           ((_ic)->ic_flags &= ~IEEE80211_F_USEPROT)

#define IEEE80211_IS_SHPREAMBLE_ENABLED(_ic)        ((_ic)->ic_flags & IEEE80211_F_SHPREAMBLE)
#define IEEE80211_ENABLE_SHPREAMBLE(_ic)            ((_ic)->ic_flags |= IEEE80211_F_SHPREAMBLE)
#define IEEE80211_DISABLE_SHPREAMBLE(_ic)           ((_ic)->ic_flags &= ~IEEE80211_F_SHPREAMBLE)

#define IEEE80211_IS_CAP_SHPREAMBLE_ENABLED(_ic)    ((_ic)->ic_caps & IEEE80211_C_SHPREAMBLE)
#define IEEE80211_ENABLE_CAP_SHPREAMBLE(_ic)        ((_ic)->ic_caps |= IEEE80211_C_SHPREAMBLE)
#define IEEE80211_DISABLE_CAP_SHPREAMBLE(_ic)       ((_ic)->ic_caps &= ~IEEE80211_C_SHPREAMBLE)


#define IEEE80211_IS_BARKER_ENABLED(_ic)            ((_ic)->ic_flags & IEEE80211_F_USEBARKER)
#define IEEE80211_ENABLE_BARKER(_ic)                ((_ic)->ic_flags |= IEEE80211_F_USEBARKER)
#define IEEE80211_DISABLE_BARKER(_ic)               ((_ic)->ic_flags &= ~IEEE80211_F_USEBARKER)

#define IEEE80211_IS_SHSLOT_ENABLED(_ic)            ((_ic)->ic_flags & IEEE80211_F_SHSLOT)
#define IEEE80211_ENABLE_SHSLOT(_ic)                ((_ic)->ic_flags |= IEEE80211_F_SHSLOT)
#define IEEE80211_DISABLE_SHSLOT(_ic)               ((_ic)->ic_flags &= ~IEEE80211_F_SHSLOT)

#define IEEE80211_IS_DATAPAD_ENABLED(_ic)           ((_ic)->ic_flags & IEEE80211_F_DATAPAD)
#define IEEE80211_ENABLE_DATAPAD(_ic)               ((_ic)->ic_flags |= IEEE80211_F_DATAPAD)
#define IEEE80211_DISABLE_DATAPAD(_ic)              ((_ic)->ic_flags &= ~IEEE80211_F_DATAPAD)

#define IEEE80211_IS_COUNTRYIE_ENABLED(_ic)         ((_ic)->ic_flags_ext & IEEE80211_FEXT_COUNTRYIE)
#define IEEE80211_ENABLE_COUNTRYIE(_ic)             ((_ic)->ic_flags_ext |= IEEE80211_FEXT_COUNTRYIE)
#define IEEE80211_DISABLE_COUNTRYIE(_ic)            ((_ic)->ic_flags_ext &= ~IEEE80211_FEXT_COUNTRYIE)

#define IEEE80211_IS_11D_ENABLED(_ic)               ((_ic)->ic_flags_ext & IEEE80211_FEXT_DOT11D)
#define IEEE80211_ENABLE_11D(_ic)                   ((_ic)->ic_flags_ext |= IEEE80211_FEXT_DOT11D)
#define IEEE80211_DISABLE_11D(_ic)                  ((_ic)->ic_flags_ext &= ~IEEE80211_FEXT_DOT11D)

#define IEEE80211_HAS_DESIRED_COUNTRY(_ic)          ((_ic)->ic_flags_ext & IEEE80211_FEXT_DESCOUNTRY)
#define IEEE80211_SET_DESIRED_COUNTRY(_ic)          ((_ic)->ic_flags_ext |= IEEE80211_FEXT_DESCOUNTRY)
#define IEEE80211_CLEAR_DESIRED_COUNTRY(_ic)        ((_ic)->ic_flags_ext &= ~IEEE80211_FEXT_DESCOUNTRY)

#define IEEE80211_IS_11D_BEACON_IGNORED(_ic)        ((_ic)->ic_flags_ext & IEEE80211_FEXT_IGNORE_11D_BEACON)
#define IEEE80211_ENABLE_IGNORE_11D_BEACON(_ic)     ((_ic)->ic_flags_ext |= IEEE80211_FEXT_IGNORE_11D_BEACON)
#define IEEE80211_DISABLE_IGNORE_11D_BEACON(_ic)    ((_ic)->ic_flags_ext &= ~IEEE80211_FEXT_IGNORE_11D_BEACON)

#define IEEE80211_IS_RADAR_ENABLED(_ic)             ((_ic)->ic_flags_ext & IEEE80211_FEXT_RADAR)
#define IEEE80211_ENABLE_RADAR(_ic)                 ((_ic)->ic_flags_ext |= IEEE80211_FEXT_RADAR)
#define IEEE80211_DISABLE_RADAR(_ic)                ((_ic)->ic_flags_ext &= ~IEEE80211_FEXT_RADAR)

#define IEEE80211_IS_HTVIE_ENABLED(_ic)             ((_ic)->ic_flags_ext & IEEE80211_FEXT_HTVIE)
#define IEEE80211_ENABLE_HTVIE(_ic)                 ((_ic)->ic_flags_ext |= IEEE80211_FEXT_HTVIE)
#define IEEE80211_DISABLE_HTVIE(_ic)                ((_ic)->ic_flags_ext &= ~IEEE80211_FEXT_HTVIE)

#define IEEE80211_IS_AMPDU_ENABLED(_ic)             ((_ic)->ic_flags_ext & IEEE80211_FEXT_AMPDU)
#define IEEE80211_ENABLE_AMPDU(_ic)                 ((_ic)->ic_flags_ext |= IEEE80211_FEXT_AMPDU)
#define IEEE80211_DISABLE_AMPDU(_ic)                ((_ic)->ic_flags_ext &= ~IEEE80211_FEXT_AMPDU)

#define IEEE80211_GET_BCAST_ADDR(_c)                ((_c)->ic_broadcast)

#ifdef ATH_SUPPORT_HTC
#define IEEE80211_STATE_LOCK_INIT(_ic)             OS_HTC_LOCK_INIT(&(_ic)->ic_state_lock)
#define IEEE80211_STATE_LOCK_DESTROY(_ic)          OS_HTC_LOCK_DESTROY(&(_ic)->ic_state_lock)
#define IEEE80211_STATE_LOCK(_ic)                  OS_HTC_LOCK(&(_ic)->ic_state_lock)     
#define IEEE80211_STATE_UNLOCK(_ic)                OS_HTC_UNLOCK(&(_ic)->ic_state_lock)

#define	IEEE80211_STATE_P2P_ACTION_LOCK_INIT(_scn)      OS_HTC_P2P_LOCK_INIT(&(_scn)->p2p_action_queue_lock)
#define	IEEE80211_STATE_P2P_ACTION_LOCK_DESTROY(_scn)   OS_HTC_P2P_LOCK_DESTROY(&(_scn)->p2p_action_queue_lock)
#define	IEEE80211_STATE_P2P_ACTION_LOCK_IRQ(_scn)       OS_HTC_P2P_LOCK_IRQ(&(_scn)->p2p_action_queue_lock, _scn->p2p_action_queue_flags)
#define	IEEE80211_STATE_P2P_ACTION_UNLOCK_IRQ(_scn)     OS_HTC_P2P_UNLOCK_IRQ(&(_scn)->p2p_action_queue_lock, _scn->p2p_action_queue_flags)

#define	IEEE80211_KEYMAP_LOCK(_scn)                OS_KEYMAP_LOCK(&(_scn)->sc_keyixmap_lock, _scn->sc_keyixmap_lock_flags)
#define	IEEE80211_KEYMAP_UNLOCK(_scn)              OS_KEYMAP_UNLOCK(&(_scn)->sc_keyixmap_lock, _scn->sc_keyixmap_lock_flags)

#define	IEEE80211_STAT_LOCK(_vaplock)              OS_STAT_LOCK((_vaplock))
#define	IEEE80211_STAT_UNLOCK(_vaplock)            OS_STAT_UNLOCK((_vaplock))

#define IEEE80211_COMM_LOCK(_ic)                   spin_lock_dpc(&(_ic)->ic_lock)
#define IEEE80211_COMM_UNLOCK(_ic)                 spin_unlock_dpc(&(_ic)->ic_lock)

typedef htclock_t ieee80211_p2p_gosche_lock_t;
#define IEEE80211_P2P_GOSCHE_LOCK_INIT(_gos)       OS_HTC_LOCK_INIT(&((_gos)->lock))
#define IEEE80211_P2P_GOSCHE_LOCK_DESTROY(_gos)    OS_HTC_LOCK_DESTROY(&((_gos)->lock))
#define IEEE80211_P2P_GOSCHE_LOCK(_gos)            OS_HTC_LOCK(&((_gos)->lock))
#define IEEE80211_P2P_GOSCHE_UNLOCK(_gos)          OS_HTC_UNLOCK(&((_gos)->lock))

typedef htclock_t ieee80211_tsf_timer_lock_t;
#define IEEE80211_TSF_TIMER_LOCK_INIT(_tsf)        OS_HTC_LOCK_INIT(&((_tsf)->lock))
#define IEEE80211_TSF_TIMER_LOCK_DESTROY(_tsf)     OS_HTC_LOCK_DESTROY(&((_tsf)->lock))
#define IEEE80211_TSF_TIMER_LOCK(_tsf)             OS_HTC_LOCK(&((_tsf)->lock))
#define IEEE80211_TSF_TIMER_UNLOCK(_tsf)           OS_HTC_UNLOCK(&((_tsf)->lock))

typedef htclock_t ieee80211_resmgr_oc_sched_lock_t;
#define IEEE80211_RESMGR_OCSCHE_LOCK_INIT(_ocslock)     OS_HTC_LOCK_INIT(&((_ocslock)->scheduler_lock))
#define IEEE80211_RESMGR_OCSCHE_LOCK_DESTROY(_ocslock)  OS_HTC_LOCK_DESTROY(&((_ocslock)->scheduler_lock))
#define IEEE80211_RESMGR_OCSCHE_LOCK(_ocslock)          OS_HTC_LOCK(&((_ocslock)->scheduler_lock))
#define IEEE80211_RESMGR_OCSCHE_UNLOCK(_ocslock)        OS_HTC_UNLOCK(&((_ocslock)->scheduler_lock))

#define IEEE80211_VAP_LOCK(_vap)                   spin_lock_dpc(&(_vap)->iv_lock)
#define IEEE80211_VAP_UNLOCK(_vap)                 spin_unlock_dpc(&(_vap)->iv_lock)
#else
#define IEEE80211_STATE_LOCK_INIT(_ic)             spin_lock_init(&(_ic)->ic_state_lock)
#define IEEE80211_STATE_LOCK_DESTROY(_ic)
#define IEEE80211_STATE_LOCK(_ic)                  spin_lock(&(_ic)->ic_state_lock)
#define IEEE80211_STATE_UNLOCK(_ic)                spin_unlock(&(_ic)->ic_state_lock)

#if (ATH_SUPPORT_UAPSD)
#define	IEEE80211_KEYMAP_LOCK(_scn)                spin_lock_irqsave(&(_scn)->sc_keyixmap_lock, _scn->sc_keyixmap_lock_flags)
#define	IEEE80211_KEYMAP_UNLOCK(_scn)              spin_unlock_irqrestore(&(_scn)->sc_keyixmap_lock, _scn->sc_keyixmap_lock_flags)
#else
#define	IEEE80211_KEYMAP_LOCK(_scn)                spin_lock(&(_scn)->sc_keyixmap_lock)
#define	IEEE80211_KEYMAP_UNLOCK(_scn)              spin_unlock(&(_scn)->sc_keyixmap_lock)
#endif

#define IEEE80211_COMM_LOCK(_ic)                   spin_lock(&(_ic)->ic_lock)
#define IEEE80211_COMM_UNLOCK(_ic)                 spin_unlock(&(_ic)->ic_lock)

#define	IEEE80211_STAT_LOCK(_vaplock)              spin_lock((_vaplock))
#define	IEEE80211_STAT_UNLOCK(_vaplock)            spin_unlock((_vaplock))

typedef spinlock_t ieee80211_p2p_gosche_lock_t;
#define IEEE80211_P2P_GOSCHE_LOCK_INIT(_gos)       spin_lock_init(&((_gos)->lock));
#define IEEE80211_P2P_GOSCHE_LOCK_DESTROY(_gos)    spin_lock_destroy(&((_gos)->lock))
#define IEEE80211_P2P_GOSCHE_LOCK(_gos)            spin_lock(&((_gos)->lock))
#define IEEE80211_P2P_GOSCHE_UNLOCK(_gos)          spin_unlock(&((_gos)->lock))

typedef spinlock_t ieee80211_tsf_timer_lock_t;
#define IEEE80211_TSF_TIMER_LOCK_INIT(_tsf)        spin_lock_init(&((_tsf)->lock));
#define IEEE80211_TSF_TIMER_LOCK_DESTROY(_tsf)     spin_lock_destroy(&((_tsf)->lock))
#define IEEE80211_TSF_TIMER_LOCK(_tsf)             spin_lock(&((_tsf)->lock))
#define IEEE80211_TSF_TIMER_UNLOCK(_tsf)           spin_unlock(&((_tsf)->lock))

typedef spinlock_t ieee80211_resmgr_oc_sched_lock_t;
#define IEEE80211_RESMGR_OCSCHE_LOCK_INIT(_ocslock)     spin_lock_init(&((_ocslock)->scheduler_lock));
#define IEEE80211_RESMGR_OCSCHE_LOCK_DESTROY(_ocslock)  spin_lock_destroy(&((_ocslock)->scheduler_lock))
#define IEEE80211_RESMGR_OCSCHE_LOCK(_ocslock)          spin_lock(&((_ocslock)->scheduler_lock))
#define IEEE80211_RESMGR_OCSCHE_UNLOCK(_ocslock)        spin_unlock(&((_ocslock)->scheduler_lock))

#define IEEE80211_VAP_LOCK(_vap)                   spin_lock(&_vap->iv_lock);
#define IEEE80211_VAP_UNLOCK(_vap)                 spin_unlock(&_vap->iv_lock);
#endif

#ifndef  ATH_BEACON_DEFERRED_PROC
#ifdef  ATH_SUPPORT_HTC
#define OS_BEACON_DECLARE_AND_RESET_VAR(_flags)                /* Do nothing */
#define OS_BEACON_READ_LOCK(_p_lock , _p_state, _flags)        OS_RWLOCK_READ_LOCK(_p_lock , _p_state)
#define OS_BEACON_READ_UNLOCK(_p_lock , _p_state, _flags)      OS_RWLOCK_READ_UNLOCK(_p_lock , _p_state)
#define OS_BEACON_WRITE_LOCK(_p_lock , _p_state, _flags)       OS_RWLOCK_WRITE_LOCK(_p_lock , _p_state)
#define OS_BEACON_WRITE_UNLOCK(_p_lock , _p_state, _flags)     OS_RWLOCK_WRITE_UNLOCK(_p_lock , _p_state)
#else
#define OS_BEACON_DECLARE_AND_RESET_VAR(_flags)                unsigned long _flags = 0
#define OS_BEACON_READ_LOCK(_p_lock , _p_state, _flags)        OS_RWLOCK_READ_LOCK_IRQSAVE(_p_lock , _p_state, _flags)
#define OS_BEACON_READ_UNLOCK(_p_lock , _p_state, _flags)      OS_RWLOCK_READ_UNLOCK_IRQRESTORE(_p_lock , _p_state, _flags)
#define OS_BEACON_WRITE_LOCK(_p_lock , _p_state, _flags)       OS_RWLOCK_WRITE_LOCK_IRQSAVE(_p_lock , _p_state, _flags)
#define OS_BEACON_WRITE_UNLOCK(_p_lock , _p_state, _flags)     OS_RWLOCK_WRITE_UNLOCK_IRQRESTORE(_p_lock , _p_state, _flags)
#endif
#else
#define OS_BEACON_DECLARE_AND_RESET_VAR(_flags)                /* Do nothing */
#define OS_BEACON_READ_LOCK(_p_lock , _p_state, _flags)        OS_RWLOCK_READ_LOCK(_p_lock , _p_state)
#define OS_BEACON_READ_UNLOCK(_p_lock , _p_state, _flags)      OS_RWLOCK_READ_UNLOCK(_p_lock , _p_state)
#define OS_BEACON_WRITE_LOCK(_p_lock , _p_state, _flags)       OS_RWLOCK_WRITE_LOCK(_p_lock , _p_state)
#define OS_BEACON_WRITE_UNLOCK(_p_lock , _p_state, _flags)     OS_RWLOCK_WRITE_UNLOCK(_p_lock , _p_state)
#endif

     
#ifdef ATH_EXT_AP
#define IEEE80211_VAP_EXT_AP_ENABLE(_v)             ((_v)->iv_flags_ext |= IEEE80211_FEXT_AP)
#define IEEE80211_VAP_EXT_AP_DISABLE(_v)            ((_v)->iv_flags_ext &= ~IEEE80211_FEXT_AP)
#define IEEE80211_VAP_IS_EXT_AP_ENABLED(_v)         ((_v)->iv_flags_ext & IEEE80211_FEXT_AP)
#endif

/* 
 * Some times hardware passes the frames without decryption. S/W can 
 * choose to decrypt them or to drop them. When enabled, all the frames
 * with KEYMISS set, would be decrypted in s/w and if not they will be
 * ignored
 */
#define IEEE80211_VAP_CCMPSW_ENCDEC_ENABLE(_v) \
            ((_v)->iv_ccmpsw_seldec = 1)

#define IEEE80211_VAP_CCMPSW_ENCDEC_DISABLE(_v)\
            ((_v)->iv_ccmpsw_seldec = 0)
        

#define IC_FLAG_FUNCS(xyz) \
     static INLINE int ieee80211_ic_##xyz##_is_set (struct ieee80211com *_ic) { \
        return (_ic->ic_##xyz == 1); \
     } \
     static INLINE int ieee80211_ic_##xyz##_is_clear (struct ieee80211com *_ic) { \
        return (_ic->ic_##xyz == 0); \
     } \
     static INLINE void ieee80211_ic_##xyz##_set (struct ieee80211com *_ic) { \
        _ic->ic_##xyz = 1; \
     } \
     static INLINE void  ieee80211_ic_##xyz##_clear (struct ieee80211com *_ic) { \
        _ic->ic_##xyz = 0; \
     }

IC_FLAG_FUNCS(wep_tkip_htrate)
IC_FLAG_FUNCS(non_ht_ap)
IC_FLAG_FUNCS(block_dfschan)
IC_FLAG_FUNCS(doth)
IC_FLAG_FUNCS(off_channel_support)
IC_FLAG_FUNCS(ht20Adhoc)
IC_FLAG_FUNCS(ht40Adhoc)
IC_FLAG_FUNCS(htAdhocAggr)
IC_FLAG_FUNCS(disallowAutoCCchange)
IC_FLAG_FUNCS(ignoreDynamicHalt)
IC_FLAG_FUNCS(p2pDevEnable)
IC_FLAG_FUNCS(override_proberesp_ie)
IC_FLAG_FUNCS(2g_csa)

#define IEEE80211_VAP_IS_DROP_UNENC(_v)             ((_v)->iv_flags & IEEE80211_F_DROPUNENC)
#define IEEE80211_VAP_DROP_UNENC_ENABLE(_v)         ((_v)->iv_flags |= IEEE80211_F_DROPUNENC)
#define IEEE80211_VAP_DROP_UNENC_DISABLE(_v)        ((_v)->iv_flags &= ~IEEE80211_F_DROPUNENC)

#define IEEE80211_VAP_COUNTERM_ENABLE(_v)           ((_v)->iv_flags |= IEEE80211_F_COUNTERM)
#define IEEE80211_VAP_COUNTERM_DISABLE(_v)          ((_v)->iv_flags &= ~IEEE80211_F_COUNTERM)
#define IEEE80211_VAP_IS_COUNTERM_ENABLED(_v)       ((_v)->iv_flags & IEEE80211_F_COUNTERM)

#define IEEE80211_VAP_WPA_ENABLE(_v)                ((_v)->iv_flags |= IEEE80211_F_WPA)
#define IEEE80211_VAP_WPA_DISABLE(_v)               ((_v)->iv_flags &= ~IEEE80211_F_WPA)
#define IEEE80211_VAP_IS_WPA_ENABLED(_v)            ((_v)->iv_flags & IEEE80211_F_WPA)

#define IEEE80211_VAP_PUREG_ENABLE(_v)              ((_v)->iv_flags |= IEEE80211_F_PUREG)
#define IEEE80211_VAP_PUREG_DISABLE(_v)             ((_v)->iv_flags &= ~IEEE80211_F_PUREG)
#define IEEE80211_VAP_IS_PUREG_ENABLED(_v)          ((_v)->iv_flags & IEEE80211_F_PUREG)

#define IEEE80211_VAP_PRIVACY_ENABLE(_v)            ((_v)->iv_flags |= IEEE80211_F_PRIVACY)
#define IEEE80211_VAP_PRIVACY_DISABLE(_v)           ((_v)->iv_flags &= ~IEEE80211_F_PRIVACY)
#define IEEE80211_VAP_IS_PRIVACY_ENABLED(_v)        ((_v)->iv_flags & IEEE80211_F_PRIVACY)

#define IEEE80211_VAP_HIDESSID_ENABLE(_v)           ((_v)->iv_flags |= IEEE80211_F_HIDESSID)
#define IEEE80211_VAP_HIDESSID_DISABLE(_v)          ((_v)->iv_flags &= ~IEEE80211_F_HIDESSID)
#define IEEE80211_VAP_IS_HIDESSID_ENABLED(_v)       ((_v)->iv_flags & IEEE80211_F_HIDESSID)

#define IEEE80211_VAP_NOBRIDGE_ENABLE(_v)           ((_v)->iv_flags |= IEEE80211_F_NOBRIDGE)
#define IEEE80211_VAP_NOBRIDGE_DISABLE(_v)          ((_v)->iv_flags &= ~IEEE80211_F_NOBRIDGE)
#define IEEE80211_VAP_IS_NOBRIDGE_ENABLED(_v)       ((_v)->iv_flags & IEEE80211_F_NOBRIDGE)

#define IEEE80211_VAP_IS_TIMUPDATE_ENABLED(_v)      ((_v)->iv_flags_ext & IEEE80211_F_TIMUPDATE)
#define IEEE80211_VAP_TIMUPDATE_ENABLE(_v)          ((_v)->iv_flags_ext |= IEEE80211_F_TIMUPDATE)
#define IEEE80211_VAP_TIMUPDATE_DISABLE(_v)         ((_v)->iv_flags_ext &= ~IEEE80211_F_TIMUPDATE)

#define IEEE80211_VAP_IS_UAPSD_ENABLED(_v)          ((_v)->iv_flags_ext & IEEE80211_FEXT_UAPSD)
#define IEEE80211_VAP_UAPSD_ENABLE(_v)              ((_v)->iv_flags_ext |= IEEE80211_FEXT_UAPSD)
#define IEEE80211_VAP_UAPSD_DISABLE(_v)             ((_v)->iv_flags_ext &= ~IEEE80211_FEXT_UAPSD)

#define IEEE80211_VAP_IS_SLEEPING(_v)               ((_v)->iv_flags_ext & IEEE80211_FEXT_SLEEP)
#define IEEE80211_VAP_GOTOSLEEP(_v)                 ((_v)->iv_flags_ext |= IEEE80211_FEXT_SLEEP)
#define IEEE80211_VAP_WAKEUP(_v)                    ((_v)->iv_flags_ext &= ~IEEE80211_FEXT_SLEEP)

#define IEEE80211_VAP_IS_EOSPDROP_ENABLED(_v)       ((_v)->iv_flags_ext & IEEE80211_FEXT_EOSPDROP)
#define IEEE80211_VAP_EOSPDROP_ENABLE(_v)           ((_v)->iv_flags_ext |= IEEE80211_FEXT_EOSPDROP)
#define IEEE80211_VAP_EOSPDROP_DISABLE(_v)          ((_v)->iv_flags_ext &= ~IEEE80211_FEXT_EOSPDROP)

#define IEEE80211_VAP_IS_HTRATES_ENABLED(_v)        ((_v)->iv_flags_ext & IEEE80211_FEXT_HTRATES)
#define IEEE80211_VAP_HTRATES_ENABLE(_v)            ((_v)->iv_flags_ext |= IEEE80211_FEXT_HTRATES)
#define IEEE80211_VAP_HTRATES_DISABLE(_v)           ((_v)->iv_flags_ext &= ~IEEE80211_FEXT_HTRATES)

#define IEEE80211_VAP_SAFEMODE_ENABLE(_v)           ((_v)->iv_flags_ext |= IEEE80211_FEXT_SAFEMODE)
#define IEEE80211_VAP_SAFEMODE_DISABLE(_v)          ((_v)->iv_flags_ext &= ~IEEE80211_FEXT_SAFEMODE)
#define IEEE80211_VAP_IS_SAFEMODE_ENABLED(_v)       ((_v)->iv_flags_ext & IEEE80211_FEXT_SAFEMODE)

#define IEEE80211_VAP_DELIVER_80211_ENABLE(_v)      ((_v)->iv_flags_ext |= IEEE80211_FEXT_DELIVER_80211)
#define IEEE80211_VAP_DELIVER_80211_DISABLE(_v)     ((_v)->iv_flags_ext &= ~IEEE80211_FEXT_DELIVER_80211)
#define IEEE80211_VAP_IS_DELIVER_80211_ENABLED(_v)  ((_v)->iv_flags_ext & IEEE80211_FEXT_DELIVER_80211)

#define IEEE80211_VAP_SEND_80211_ENABLE(_v)         ((_v)->iv_flags_ext |= IEEE80211_FEXT_SEND_80211)
#define IEEE80211_VAP_SEND_80211_DISABLE(_v)        ((_v)->iv_flags_ext &= ~IEEE80211_FEXT_SEND_80211)
#define IEEE80211_VAP_IS_SEND_80211_ENABLED(_v)     ((_v)->iv_flags_ext & IEEE80211_FEXT_SEND_80211)

#define IEEE80211_VAP_WDS_ENABLE(_v)                ((_v)->iv_flags_ext |= IEEE80211_FEXT_WDS)
#define IEEE80211_VAP_WDS_DISABLE(_v)               ((_v)->iv_flags_ext &= ~IEEE80211_FEXT_WDS)
#define IEEE80211_VAP_IS_WDS_ENABLED(_v)            ((_v)->iv_flags_ext & IEEE80211_FEXT_WDS)

#define IEEE80211_VAP_STATIC_WDS_ENABLE(_v)         ((_v)->iv_flags_ext |= IEEE80211_FEXT_WDS_STATIC)
#define IEEE80211_VAP_STATIC_WDS_DISABLE(_v)        ((_v)->iv_flags_ext &= ~IEEE80211_FEXT_WDS_STATIC)
#define IEEE80211_VAP_IS_STATIC_WDS_ENABLED(_v)     ((_v)->iv_flags_ext & IEEE80211_FEXT_WDS_STATIC)

#define IEEE80211_VAP_WDS_AUTODETECT_ENABLE(_v)     ((_v)->iv_flags_ext |= IEEE80211_FEXT_WDS_AUTODETECT)
#define IEEE80211_VAP_WDS_AUTODETECT_DISABLE(_v)    ((_v)->iv_flags_ext &= ~IEEE80211_FEXT_WDS_AUTODETECT)
#define IEEE80211_VAP_IS_WDS_AUTODETECT_ENABLED(_v) ((_v)->iv_flags_ext & IEEE80211_FEXT_WDS_AUTODETECT)

#define IEEE80211_VAP_PURE11N_ENABLE(_v)            ((_v)->iv_flags_ext |= IEEE80211_FEXT_PURE11N)
#define IEEE80211_VAP_PURE11N_DISABLE(_v)           ((_v)->iv_flags_ext &= ~IEEE80211_FEXT_PURE11N)
#define IEEE80211_VAP_IS_PURE11N_ENABLED(_v)        ((_v)->iv_flags_ext & IEEE80211_FEXT_PURE11N)

#define IEEE80211_VAP_PUREB_ENABLE(_v)              ((_v)->iv_flags_ext |= IEEE80211_FEXT_PUREB)
#define IEEE80211_VAP_PUREB_DISABLE(_v)             ((_v)->iv_flags_ext &= ~IEEE80211_FEXT_PUREB)
#define IEEE80211_VAP_IS_PUREB_ENABLED(_v)          ((_v)->iv_flags_ext & IEEE80211_FEXT_PUREB)

#define IEEE80211_VAP_APPIE_UPDATE_ENABLE(_v)       ((_v)->iv_flags_ext |= IEEE80211_FEXT_APPIE_UPDATE)
#define IEEE80211_VAP_APPIE_UPDATE_DISABLE(_v)      ((_v)->iv_flags_ext &= ~IEEE80211_FEXT_APPIE_UPDATE)
#define IEEE80211_VAP_IS_APPIE_UPDATE_ENABLED(_v)   ((_v)->iv_flags_ext & IEEE80211_FEXT_APPIE_UPDATE)

#define IEEE80211_VAP_AUTOASSOC_ENABLE(_v)       ((_v)->auto_assoc = 1)
#define IEEE80211_VAP_AUTOASSOC_DISABLE(_v)      ((_v)->auto_assoc = 0)
#define IEEE80211_VAP_IS_AUTOASSOC_ENABLED(_v)   ((_v)->auto_assoc == 1) ? 1 : 0

#define VAP_FLAG_FUNCS(xyz) \
     static INLINE int ieee80211_vap_##xyz##_is_set (struct ieee80211vap *_vap) { \
        return (_vap->iv_##xyz == 1); \
     } \
     static INLINE int ieee80211_vap_##xyz##_is_clear (struct ieee80211vap *_vap) { \
        return (_vap->iv_##xyz == 0); \
     } \
     static INLINE void ieee80211_vap_##xyz##_set (struct ieee80211vap *_vap) { \
        _vap->iv_##xyz =1; \
     } \
     static INLINE void  ieee80211_vap_##xyz##_clear (struct ieee80211vap *_vap) { \
        _vap->iv_##xyz = 0; \
     } 

VAP_FLAG_FUNCS(deleted) 
VAP_FLAG_FUNCS(active)
VAP_FLAG_FUNCS(ready) 
VAP_FLAG_FUNCS(smps) 
VAP_FLAG_FUNCS(sw_bmiss) 
VAP_FLAG_FUNCS(copy_beacon) 
VAP_FLAG_FUNCS(wapi)
VAP_FLAG_FUNCS(cansleep)
VAP_FLAG_FUNCS(sta_fwd)
VAP_FLAG_FUNCS(scanning)
VAP_FLAG_FUNCS(standby)
VAP_FLAG_FUNCS(dynamic_mimo_ps)
VAP_FLAG_FUNCS(wme)
VAP_FLAG_FUNCS(doth)
VAP_FLAG_FUNCS(country_ie)
VAP_FLAG_FUNCS(off_channel_support)
VAP_FLAG_FUNCS(dfswait)
VAP_FLAG_FUNCS(erpupdate)
VAP_FLAG_FUNCS(vap_ind)
VAP_FLAG_FUNCS(needs_scheduler)
VAP_FLAG_FUNCS(forced_sleep)
VAP_FLAG_FUNCS(no_multichannel)
VAP_FLAG_FUNCS(bssload)
VAP_FLAG_FUNCS(bssload_update)
VAP_FLAG_FUNCS(rrm)
VAP_FLAG_FUNCS(wnm)
VAP_FLAG_FUNCS(ap_reject_dfs_chan)
VAP_FLAG_FUNCS(smartnet_enable)
VAP_FLAG_FUNCS(trigger_mlme_resp)
VAP_FLAG_FUNCS(mfp_test)
VAP_FLAG_FUNCS(proxyarp)
VAP_FLAG_FUNCS(dgaf_disable)
#if ATH_SUPPORT_SIMPLE_CONFIG_EXT
VAP_FLAG_FUNCS(nopbn)
#endif

/* TBD: There should be only one ic_evtable */
#define IEEE80211COM_DELIVER_VAP_EVENT(_ic,_osif,_evt)  do {             \
        int i;                                                                                 \
        IEEE80211_COMM_LOCK(ic);                                                               \
        for(i=0;i<IEEE80211_MAX_DEVICE_EVENT_HANDLERS; ++i) {                                  \
            if ( _ic->ic_evtable[i]  && _ic->ic_evtable[i]->wlan_dev_vap_event) {              \
                (* _ic->ic_evtable[i]->wlan_dev_vap_event)(_ic->ic_event_arg[i],_ic,_osif,_evt);\
            }                                                                                  \
        }                                                                                      \
        IEEE80211_COMM_UNLOCK(ic);                                                             \
    } while(0)

/* Atheros ABOLT definitions */
#define IEEE80211_ABOLT_TURBO_G        0x01    /* Legacy Turbo G */
#define IEEE80211_ABOLT_TURBO_PRIME    0x02    /* Turbo Prime */
#define IEEE80211_ABOLT_COMPRESSION    0x04    /* Compression */
#define IEEE80211_ABOLT_FAST_FRAME     0x08    /* Fast Frames */
#define IEEE80211_ABOLT_BURST          0x10    /* Bursting */
#define IEEE80211_ABOLT_WME_ELE        0x20    /* WME based cwmin/max/burst tuning */
#define IEEE80211_ABOLT_XR             0x40    /* XR */
#define IEEE80211_ABOLT_AR             0x80    /* AR switches out based on adjacent non-turbo traffic */

/* Atheros Advanced Capabilities ABOLT definition */
#define IEEE80211_ABOLT_ADVCAP                  \
    (IEEE80211_ABOLT_TURBO_PRIME |              \
    IEEE80211_ABOLT_COMPRESSION |               \
    IEEE80211_ABOLT_FAST_FRAME |                \
    IEEE80211_ABOLT_XR |                        \
    IEEE80211_ABOLT_AR |                        \
    IEEE80211_ABOLT_BURST |                     \
    IEEE80211_ABOLT_WME_ELE)

/* check if a capability was negotiated for use */
#define IEEE80211_ATH_CAP(vap, ni, bit) \
    ((ni)->ni_ath_flags & (vap)->iv_ath_cap & (bit))

/*
 * flags to be passed to ieee80211_vap_create function .
 */
#define IEEE80211_CLONE_BSSID   0x0001      /* allocate unique mac/bssid */
#define IEEE80211_NO_STABEACONS 0x0002      /* Do not setup the station beacon timers */
#define IEEE80211_CLONE_WDS             0x0004  /* enable WDS processing */
#define IEEE80211_CLONE_WDSLEGACY       0x0008  /* legacy WDS operation */
#define IEEE80211_PRIMARY_VAP           0x0010  /* primary vap */
#define IEEE80211_P2PDEV_VAP            0x0020  /* p2pdev vap */
#define IEEE80211_P2PGO_VAP             0x0040  /* p2p-go vap */
#define IEEE80211_P2PCLI_VAP            0x0080  /* p2p-client vap */
#define IEEE80211_P2P_DEVICE            (IEEE80211_P2PDEV_VAP | IEEE80211_P2PGO_VAP | IEEE80211_P2PCLI_VAP)

#define NET80211_MEMORY_TAG     '11tN'

/* ic_htflags */
#define IEEE80211_HTF_SHORTGI40     0x0001
#define IEEE80211_HTF_SHORTGI20     0x0002

/* MFP support values */
typedef enum _ieee80211_mfp_type{
    IEEE80211_MFP_QOSDATA,
    IEEE80211_MFP_PASSTHRU,
    IEEE80211_MFP_HW_CRYPTO
} ieee80211_mfp_type;

void ieee80211_start_running(struct ieee80211com *ic);
void ieee80211_stop_running(struct ieee80211com *ic);
int ieee80211com_register_event_handlers(struct ieee80211com *ic, 
                                     void *event_arg,
                                     wlan_dev_event_handler_table *evtable);
int ieee80211com_unregister_event_handlers(struct ieee80211com *ic, 
                                     void *event_arg,
                                     wlan_dev_event_handler_table *evtable);

u_int16_t ieee80211_vaps_active(struct ieee80211com *ic);
u_int16_t ieee80211_vaps_ready(struct ieee80211com *ic, enum ieee80211_opmode opmode);

int ieee80211_vap_setup(struct ieee80211com *ic, struct ieee80211vap *vap,
                        int opmode, int scan_priority_base, int flags,
                        const u_int8_t bssid[IEEE80211_ADDR_LEN]);

int ieee80211_ifattach(struct ieee80211com *ic, IEEE80211_REG_PARAMETERS *);
void ieee80211_ifdetach(struct ieee80211com *ic);

int ieee80211_vap_attach(struct ieee80211vap *vap);
void ieee80211_vap_detach(struct ieee80211vap *vap);
void ieee80211_vap_free(struct ieee80211vap *vap);

int ieee80211_vap_update_superag_cap(struct ieee80211vap *vap, int en_superag);
int ieee80211_vap_match_ssid(struct ieee80211vap *vap, const u_int8_t *ssid, u_int8_t ssidlen);

int ieee80211_vap_register_events(struct ieee80211vap *vap, wlan_event_handler_table *evtab);
int ieee80211_vap_register_mlme_events(struct ieee80211vap *vap, os_handle_t oshandle, wlan_mlme_event_handler_table *evtab);
int ieee80211_vap_unregister_mlme_events(struct ieee80211vap *vap,os_handle_t oshandle, wlan_mlme_event_handler_table *evtab);
int ieee80211_vap_register_misc_events(struct ieee80211vap *vap, os_handle_t oshandle, wlan_misc_event_handler_table *evtab);
int ieee80211_vap_unregister_misc_events(struct ieee80211vap *vap,os_handle_t oshandle, wlan_misc_event_handler_table *evtab);
int ieee80211_vap_register_ccx_events(struct ieee80211vap *vap, os_if_t osif, wlan_ccx_handler_table *evtab);
ieee80211_aplist_config_t ieee80211_vap_get_aplist_config(struct ieee80211vap *vap);
ieee80211_candidate_aplist_t ieee80211_vap_get_aplist(struct ieee80211vap *vap);
ieee80211_scan_table_t ieee80211_vap_get_scan_table(struct ieee80211vap *vap);

systime_t ieee80211_get_last_data_timestamp(wlan_if_t vaphandle);
systime_t ieee80211_get_directed_frame_timestamp(wlan_if_t vaphandle);
systime_t ieee80211_get_last_ap_frame_timestamp(wlan_if_t vaphandle);
systime_t ieee80211_get_traffic_indication_timestamp(wlan_if_t vaphandle);
bool ieee80211_is_connected(wlan_if_t vaphandle);

/*
 * IC-based functions that gather information from all VAPs.
 */
 
/*
 * ieee80211com_get_traffic_indication_timestamp
 *     returns most recent data traffic timestamp in all of the IC's VAPs.
 */
systime_t ieee80211com_get_traffic_indication_timestamp(struct ieee80211com *ic);

/*
 * ieee80211_get_vap_opmode_count
 *     returns number of VAPs of each type currently active in an IC.
 */
void
ieee80211_get_vap_opmode_count(struct ieee80211com *ic, 
                               struct ieee80211_vap_opmode_count *vap_opmode_count);

int ieee80211_regdmn_reset(struct ieee80211com *ic);

int
ieee80211_has_weptkipaggr(struct ieee80211_node *ni);

void ieee80211_amsdu_encap(struct ieee80211vap *vap, wbuf_t amsdu_m, wbuf_t m, u_int16_t framelen, int prepend_ether);
int ieee80211_amsdu_check(struct ieee80211vap *vap, wbuf_t m);
int ieee80211_8023frm_amsdu_check(wbuf_t wbuf);

static INLINE osdev_t 
ieee80211com_get_oshandle(struct ieee80211com *ic)
{
     return ic->ic_osdev;
}

/*
 * Capabilities
 */
static INLINE void
ieee80211com_set_cap(struct ieee80211com *ic, u_int32_t cap)
{
    ic->ic_caps |= cap;
}

static INLINE void
ieee80211com_clear_cap(struct ieee80211com *ic, u_int32_t cap)
{
    ic->ic_caps &= ~cap;
}

static INLINE int
ieee80211com_has_cap(struct ieee80211com *ic, u_int32_t cap)
{
    return ((ic->ic_caps & cap) != 0);
}

/*
 * Extended Capabilities
 */
static INLINE void
ieee80211com_set_cap_ext(struct ieee80211com *ic, u_int32_t cap_ext)
{
    ic->ic_caps_ext |= cap_ext;
}

static INLINE void
ieee80211com_clear_cap_ext(struct ieee80211com *ic, u_int32_t cap_ext)
{
    ic->ic_caps_ext &= ~cap_ext;
}

static INLINE int
ieee80211com_has_cap_ext(struct ieee80211com *ic, u_int32_t cap_ext)
{
    return ((ic->ic_caps_ext & cap_ext) != 0);
}
/*
 * Atheros Capabilities
 */
static INLINE void
ieee80211com_set_athcap(struct ieee80211com *ic, u_int32_t athcap)
{
    ic->ic_ath_cap |= athcap;
}

static INLINE void
ieee80211com_clear_athcap(struct ieee80211com *ic, u_int32_t athcap)
{
    ic->ic_ath_cap &= ~athcap;
}

static INLINE int
ieee80211com_has_athcap(struct ieee80211com *ic, u_int32_t athcap)
{
    return ((ic->ic_ath_cap & athcap) != 0);
}

/*
 * Atheros State machine Roaming capabilities
 */


static INLINE void
ieee80211com_set_roaming(struct ieee80211com *ic, u_int8_t roaming)
{
    ic->ic_roaming = roaming;
}

static INLINE u_int8_t 
ieee80211com_get_roaming(struct ieee80211com *ic)
{
    return ic->ic_roaming;
}

/*
 * Atheros Extended Capabilities
 */
static INLINE void
ieee80211com_set_athextcap(struct ieee80211com *ic, u_int32_t athextcap)
{
    ic->ic_ath_extcap |= athextcap;
}

static INLINE void
ieee80211com_clear_athextcap(struct ieee80211com *ic, u_int32_t athextcap)
{
    ic->ic_ath_extcap &= ~athextcap;
}

static INLINE int
ieee80211com_has_athextcap(struct ieee80211com *ic, u_int32_t athextcap)
{
    return ((ic->ic_ath_extcap & athextcap) != 0);
}

/* to check if node need, extra delimeter fix */
static INLINE int
ieee80211com_has_extradelimwar(struct ieee80211com *ic)
{
    return (ic->ic_ath_extcap & IEEE80211_ATHEC_EXTRADELIMWAR) ;
}

/*
 * 11n
 */
static INLINE void
ieee80211com_set_htcap(struct ieee80211com *ic, u_int16_t htcap)
{
    ic->ic_htcap |= htcap;
}

static INLINE void
ieee80211com_clear_htcap(struct ieee80211com *ic, u_int16_t htcap)
{
    ic->ic_htcap &= ~htcap;
}

static INLINE int
ieee80211com_has_htcap(struct ieee80211com *ic, u_int16_t htcap)
{
    return ((ic->ic_htcap & htcap) != 0);
}

static INLINE void
ieee80211com_set_htextcap(struct ieee80211com *ic, u_int16_t htextcap)
{
    ic->ic_htextcap |= htextcap;
}

static INLINE void
ieee80211com_clear_htextcap(struct ieee80211com *ic, u_int16_t htextcap)
{
    ic->ic_htextcap &= ~htextcap;
}

static INLINE int
ieee80211com_has_htextcap(struct ieee80211com *ic, u_int16_t htextcap)
{
    return ((ic->ic_htextcap & htextcap) != 0);
}

static INLINE void
ieee80211com_set_htflags(struct ieee80211com *ic, u_int16_t htflags)
{
    ic->ic_htflags |= htflags;
}

static INLINE void
ieee80211com_clear_htflags(struct ieee80211com *ic, u_int16_t htflags)
{
    ic->ic_htflags &= ~htflags;
}

static INLINE int
ieee80211com_has_htflags(struct ieee80211com *ic, u_int16_t htflags)
{
    return ((ic->ic_htflags & htflags) != 0);
}

static INLINE void
ieee80211com_set_maxampdu(struct ieee80211com *ic, u_int8_t maxampdu)
{
    ic->ic_maxampdu = maxampdu;
}

static INLINE u_int8_t
ieee80211com_get_mpdudensity(struct ieee80211com *ic)
{
    return ic->ic_mpdudensity;
}

static INLINE void
ieee80211com_set_mpdudensity(struct ieee80211com *ic, u_int8_t mpdudensity)
{
    ic->ic_mpdudensity = mpdudensity;
}

static INLINE u_int8_t
ieee80211com_get_weptkipaggr_rxdelim(struct ieee80211com *ic)
{
    return (ic->ic_weptkipaggr_rxdelim);
}

static INLINE void
ieee80211com_set_weptkipaggr_rxdelim(struct ieee80211com *ic, u_int8_t weptkipaggr_rxdelim)
{
    ic->ic_weptkipaggr_rxdelim = weptkipaggr_rxdelim;
}

static INLINE u_int16_t
ieee80211com_get_channel_switching_time_usec(struct ieee80211com *ic)
{
    return (ic->ic_channelswitchingtimeusec);
}

static INLINE void
ieee80211com_set_channel_switching_time_usec(struct ieee80211com *ic, u_int16_t channel_switching_time_usec)
{
    ic->ic_channelswitchingtimeusec = channel_switching_time_usec;
}

/*
 * PHY type
 */
static INLINE enum ieee80211_phytype
ieee80211com_get_phytype(struct ieee80211com *ic)
{
    return ic->ic_phytype;
}

static INLINE void
ieee80211com_set_phytype(struct ieee80211com *ic, enum ieee80211_phytype phytype)
{
    ic->ic_phytype = phytype;
}

/*
 * 11ac
 */
static INLINE void
ieee80211com_set_vhtcap(struct ieee80211com *ic, u_int32_t vhtcap)
{
    ic->ic_vhtcap |= vhtcap;
}

static INLINE void
ieee80211com_clear_vhtcap(struct ieee80211com *ic, u_int32_t vhtcap)
{
    ic->ic_vhtcap &= ~vhtcap;
}

static INLINE int
ieee80211com_has_vhtcap(struct ieee80211com *ic, u_int32_t vhtcap)
{
    return ((ic->ic_vhtcap & vhtcap) != 0);
}

static INLINE void
ieee80211com_set_vht_mcs_map(struct ieee80211com *ic, u_int16_t mcs_map)
{
             ic->ic_vhtcap_max_mcs.rx_mcs_set.mcs_map = 
             ic->ic_vhtcap_max_mcs.tx_mcs_set.mcs_map = mcs_map; 
}

static INLINE void
ieee80211com_set_vht_high_data_rate(struct ieee80211com *ic, u_int16_t datarate)
{
        ic->ic_vhtcap_max_mcs.rx_mcs_set.data_rate = 
        ic->ic_vhtcap_max_mcs.tx_mcs_set.data_rate = datarate;
}

static INLINE void
ieee80211com_set_vhtop_basic_mcs_map(struct ieee80211com *ic, u_int16_t basic_mcs_map)
{
    ic->ic_vhtop_basic_mcs = basic_mcs_map;
}

/*
 * XXX these need to be here for IEEE80211_F_DATAPAD
 */

/*
 * Return the space occupied by the 802.11 header and any
 * padding required by the driver.  This works for a
 * management or data frame.
 */
static INLINE int
ieee80211_hdrspace(struct ieee80211com *ic, const void *data)
{
    int size = ieee80211_hdrsize(data);

    if (ic->ic_flags & IEEE80211_F_DATAPAD)
        size = roundup(size, sizeof(u_int32_t));

    return size;
}

/*
 * Like ieee80211_hdrspace, but handles any type of frame.
 */
static INLINE int
ieee80211_anyhdrspace(struct ieee80211com *ic, const void *data)
{
    int size = ieee80211_anyhdrsize(data);

    if (ic->ic_flags & IEEE80211_F_DATAPAD)
        size = roundup(size, sizeof(u_int32_t));

    return size;
}

static INLINE struct wmeParams *
ieee80211com_wmm_chanparams(struct ieee80211com *ic, int ac)
{
    ASSERT(ac < WME_NUM_AC);
    return &ic->ic_wme.wme_chanParams.cap_wmeParams[ac];
}

/*
 * chainmask
 */
static INLINE void
ieee80211com_set_tx_chainmask(struct ieee80211com *ic, u_int8_t chainmask)
{
    ic->ic_tx_chainmask = chainmask;
}

static INLINE void
ieee80211com_set_rx_chainmask(struct ieee80211com *ic, u_int8_t chainmask)
{
    ic->ic_rx_chainmask = chainmask;
}

static INLINE u_int8_t
ieee80211com_get_tx_chainmask(struct ieee80211com *ic)
{
    return ic->ic_tx_chainmask;
}

static INLINE u_int8_t
ieee80211com_get_rx_chainmask(struct ieee80211com *ic)
{
    return ic->ic_rx_chainmask;
}

static INLINE void
ieee80211com_set_spatialstreams(struct ieee80211com *ic, u_int8_t stream)
{
    ic->ic_spatialstreams = stream;
}

static INLINE void
ieee80211com_set_num_tx_chain(struct ieee80211com *ic, u_int8_t num_chain)
{
    ic->ic_num_tx_chain = num_chain;
}

static INLINE void
ieee80211com_set_num_rx_chain(struct ieee80211com *ic, u_int8_t num_chain)
{
    ic->ic_num_rx_chain = num_chain;
}

#if ATH_SUPPORT_WAPI
static INLINE void
ieee80211com_set_wapi_max_tx_chains(struct ieee80211com *ic, u_int8_t num_chain)
{
    ic->ic_num_wapi_tx_maxchains = num_chain;
}

static INLINE void
ieee80211com_set_wapi_max_rx_chains(struct ieee80211com *ic, u_int8_t num_chain)
{
    ic->ic_num_wapi_rx_maxchains = num_chain;
}
#endif

static INLINE u_int16_t
ieee80211com_get_txpowerlimit(struct ieee80211com *ic)
{
    return ic->ic_txpowlimit;
}

static INLINE void
ieee80211com_set_txpowerlimit(struct ieee80211com *ic, u_int16_t txpowlimit)
{
    ic->ic_txpowlimit = txpowlimit;
}

/*
 * Channel
 */
static INLINE void
ieee80211com_set_curchanmaxpwr(struct ieee80211com *ic, u_int8_t maxpower)
{
    ic->ic_curchanmaxpwr = maxpower;
}

static INLINE u_int8_t
ieee80211com_get_curchanmaxpwr(struct ieee80211com *ic)
{
    return ic->ic_curchanmaxpwr;
}

static INLINE  struct ieee80211_channel*   
ieee80211com_get_curchan(struct ieee80211com *ic)
{
    return ic->ic_curchan;   /* current channel */
}

static INLINE void
ieee80211_set_tspecActive(struct ieee80211com *ic, u_int8_t val)
{
    ic->ic_tspec_active = val;
}

static INLINE int
ieee80211_is_tspecActive(struct ieee80211com *ic)
{
    return ic->ic_tspec_active;
}

static INLINE u_int32_t
ieee80211_get_tsf32(struct ieee80211com *ic)
{
    return ic->ic_get_TSF32(ic);
}

#if ATH_SUPPORT_WIFIPOS
static INLINE u_int64_t
ieee80211_get_tsftstamp(struct ieee80211com *ic)
{
    return ic->ic_get_TSFTSTAMP(ic);
}
#endif
/*
 * Pre-conditions for ForcePPM to be enabled.
 */
#define ieee80211com_can_enable_force_ppm(_ic)  0

/*                                                  
 * internal macro to iterate through vaps.
 */                                                 
#if ATH_SUPPORT_AP_WDS_COMBO
#define IEEE80211_MAX_VAPS 16
#else
#define IEEE80211_MAX_VAPS 16
#endif
/*
 * Need nt_nodelock since iv_bss could have changed.
 * TBD: make ic_lock a read/write lock to reduce overhead in input_all
 */
#define ieee80211_iterate_vap_list_internal(ic,iter_func,arg,vaps_count)             \
do {                                                                                 \
    struct ieee80211vap *_vap;                                                       \
    struct ieee80211_node *bss_node[IEEE80211_MAX_VAPS];                             \
    u_int16_t    idx;                                                                \
    vaps_count=0;                                                                    \
    IEEE80211_COMM_LOCK(ic);                                                         \
    TAILQ_FOREACH(_vap, &ic->ic_vaps, iv_next) {                                     \
        if (ieee80211_vap_deleted_is_set(_vap))                                       \
            continue;                                                                \
        bss_node[vaps_count++] = ieee80211_ref_bss_node(_vap);                       \
        KASSERT((vaps_count <= IEEE80211_MAX_VAPS), ("Max Vap count reached \n"));    \
    }                                                                                \
    IEEE80211_COMM_UNLOCK(ic);                                                       \
    for (idx=0; idx<vaps_count; ++idx) {                                             \
         if (bss_node[idx]) {                                                        \
             iter_func(arg, bss_node[idx]->ni_vap, (idx == (vaps_count -1)));        \
             ieee80211_free_node(bss_node[idx]);                                     \
        }                                                                            \
    }                                                                                \
} while(0)


/*
 * Key update synchronization methods.  XXX should not be visible.
 */
static INLINE void
ieee80211_key_update_begin(struct ieee80211vap *vap)
{
#ifdef ATH_SUPPORT_HTC
        vap->iv_key_update_begin(vap);
#endif
}
static INLINE void
ieee80211_key_update_end(struct ieee80211vap *vap)
{
#ifdef ATH_SUPPORT_HTC
        vap->iv_key_update_end(vap);
#endif
}

/*
 * Return the bssid of a frame.
 */
static INLINE const u_int8_t *
ieee80211vap_getbssid(struct ieee80211vap *vap, const struct ieee80211_frame *wh)
{
    if (vap->iv_opmode == IEEE80211_M_STA)
        return wh->i_addr2;
    if ((wh->i_fc[1] & IEEE80211_FC1_DIR_MASK) != IEEE80211_FC1_DIR_NODS)
        return wh->i_addr1;
    if ((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) == IEEE80211_FC0_SUBTYPE_PS_POLL)
        return wh->i_addr1;
    return wh->i_addr3;
}

/*
 * Operation Mode
 */
static INLINE enum ieee80211_opmode
ieee80211vap_get_opmode(struct ieee80211vap *vap)
{
    return vap->iv_opmode;
}

/*
 * Misc
 */

static INLINE struct ieee80211_node *
ieee80211vap_get_bssnode(struct ieee80211vap *vap)
{
    return vap->iv_bss;
}

static INLINE void
ieee80211vap_get_macaddr(struct ieee80211vap *vap, u_int8_t *macaddr)
{
    /* the same as IC for extensible STA mode */
    IEEE80211_ADDR_COPY(macaddr, vap->iv_myaddr);
}

static INLINE void
ieee80211vap_set_macaddr(struct ieee80211vap *vap, u_int8_t *macaddr)
{
    /* Normally shouldn't be called for a station */
    IEEE80211_ADDR_COPY(vap->iv_myaddr, macaddr);
}

static INLINE u_int16_t
ieee80211vap_get_rtsthreshold(struct ieee80211vap *vap)
{
    return vap->iv_rtsthreshold;
}

static INLINE void * 
ieee80211vap_get_private_data(struct ieee80211vap *vap)
{
    return vap->iv_priv;
}

static INLINE void  
ieee80211vap_set_private_data(struct ieee80211vap *vap , void *priv_data)
{
    vap->iv_priv = priv_data;
}

static INLINE IEEE80211_VEN_IE *
ieee80211vap_get_venie(struct ieee80211vap *vap)
{
    return vap->iv_venie;
}

static INLINE IEEE80211_VEN_IE *
ieee80211vap_init_venie(struct ieee80211vap *vap)
{
    vap->iv_venie = (IEEE80211_VEN_IE *) OS_MALLOC(vap->iv_ic->ic_osdev,
     					sizeof(IEEE80211_VEN_IE), GFP_KERNEL);
    return vap->iv_venie;
}

static INLINE void ieee80211vap_delete_venie(struct ieee80211vap *vap)
{
    if(vap->iv_venie) {
        OS_FREE(vap->iv_venie);
        vap->iv_venie = NULL;
    }
}

/**
 * set(register) input filter management function callback.
 * @param vap                        : pointer to vap
 * @param mgmt_filter_function       : input management filter function calback.
 * @return the value of old filter function.
 *  the input management function is called for every received management frame.
 *  if the call back function returns true frame will be dropped.
 *  if the call back function returns false then hte frame will be passed down to mlme.
 * *** NOTE: the call back function is called even if the vap is not active.
 */
static INLINE  ieee80211_vap_input_mgmt_filter 
ieee80211vap_set_input_mgmt_filter(struct ieee80211vap *vap , ieee80211_vap_input_mgmt_filter mgmt_filter_func)
{
     ieee80211_vap_input_mgmt_filter old_filter=vap->iv_input_mgmt_filter; 
     vap->iv_input_mgmt_filter = mgmt_filter_func; 
     return old_filter;
}

/**
 * set(register) output filter management function callback.
 * @param vap                        : pointer to vap
 * @param mgmt_filter_function       : output management filter function calback.
 * @return the value of old filter function.
 *  the output management function is called for every transimitted management frame.
 *   just before handing over the frame to lmac.
 *  if the call back function returns true frame will be dropped.
 *  if the call back function returns false then hte frame will be passed down to lmac.
 */
static INLINE  ieee80211_vap_output_mgmt_filter 
ieee80211vap_set_output_mgmt_filter_func(struct ieee80211vap *vap , ieee80211_vap_output_mgmt_filter mgmt_output_filter_func)
{
     ieee80211_vap_output_mgmt_filter old_func=vap->iv_output_mgmt_filter; 
     vap->iv_output_mgmt_filter = mgmt_output_filter_func; 
     return old_func;
}

static INLINE u_int16_t
ieee80211vap_get_fragthreshold(struct ieee80211vap *vap)
{
    return vap->iv_fragthreshold;
}

static INLINE int
ieee80211vap_has_pssta(struct ieee80211vap *vap)
{
    return (vap->iv_ps_sta != 0);
}

/*
 * With WEP and TKIP encryption algorithms:
 * Disable 11n if IEEE80211_FEXT_WEP_TKIP_HTRATE is not set.
 * Check for Mixed mode, if CIPHER is set to TKIP
 */
static INLINE int
ieee80211vap_htallowed(struct ieee80211vap *vap)
{
    struct ieee80211_rsnparms *rsn = &vap->iv_rsn;
    struct ieee80211com *ic = vap->iv_ic;

    switch (vap->iv_cur_mode) {
    case IEEE80211_MODE_11A:
    case IEEE80211_MODE_11B:
    case IEEE80211_MODE_11G:
    case IEEE80211_MODE_FH:
    case IEEE80211_MODE_TURBO_A:
    case IEEE80211_MODE_TURBO_G:
        return 0;
    default:
        break;
    }


    if (!ieee80211_ic_wep_tkip_htrate_is_set(ic) &&
        IEEE80211_VAP_IS_PRIVACY_ENABLED(vap) &&
        (RSN_CIPHER_IS_WEP(rsn) ||
         (RSN_CIPHER_IS_TKIP(rsn) && !RSN_CIPHER_IS_CCMP(rsn))))
        return 0;
    else if (vap->iv_opmode == IEEE80211_M_IBSS)
        return (ieee80211_ic_ht20Adhoc_is_set(ic) || ieee80211_ic_ht40Adhoc_is_set(ic));
    else
        return 1;
}

static INLINE int
ieee80211vap_vhtallowed(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;

    /* Don't allow VHT if HT is not allowed */
    if (!ieee80211vap_htallowed(vap)){
        return 0;
    }
    
    /* Don't allow VHT if mode is HT only  */
    switch (vap->iv_cur_mode) {
        case IEEE80211_MODE_11NA_HT20:
        case IEEE80211_MODE_11NG_HT20:
        case IEEE80211_MODE_11NA_HT40PLUS:
        case IEEE80211_MODE_11NA_HT40MINUS:
        case IEEE80211_MODE_11NG_HT40PLUS:
        case IEEE80211_MODE_11NG_HT40MINUS:
            return 0;
        default:
            break;
    }

    if (ic->ic_vhtcap) {
        return 1;
    }

    return 0;
}

/*
 * Atheros Capabilities
 */
static INLINE void
ieee80211vap_set_athcap(struct ieee80211vap *vap, u_int32_t athcap)
{
    vap->iv_ath_cap |= athcap;
}

static INLINE void
ieee80211vap_clear_athcap(struct ieee80211vap *vap, u_int32_t athcap)
{
    vap->iv_ath_cap &= ~athcap;
}

static INLINE int
ieee80211vap_has_athcap(struct ieee80211vap *vap, u_int32_t athcap)
{
    return ((vap->iv_ath_cap & athcap) != 0);
}

#define printf  printk

#define CHK_IEEE80211_MSG_BASE(_ctxt, _prefix, _m)          \
    ((_ctxt)->_prefix.category_mask[(_m) >> 3] &            \
     (1 << ((_m) & 0x7)))
#define ieee80211_msg_ic(_ic, _m)               \
    CHK_IEEE80211_MSG_BASE(_ic, ic_print, _m)
#define ieee80211_msg(_vap, _m)                 \
    CHK_IEEE80211_MSG_BASE(_vap, iv_print, _m)
#define ieee80211_msg_dumppkts(_vap) \
        ieee80211_msg(_vap, IEEE80211_MSG_DUMPPKTS)
/*
 * if os does not define the 
 * debug temp buf size, define
 * a default size.
 */ 
#ifndef OS_TEMP_BUF_SIZE 
#define OS_TEMP_BUF_SIZE 256
#endif

void ieee80211com_note(struct ieee80211com *ic, const char *fmt, ...);
void ieee80211_note(struct ieee80211vap *vap, const char *fmt, ...);
void ieee80211_note_frame(struct ieee80211vap *vap,
                                    struct ieee80211_frame *wh,const  char *fmt, ...);
void ieee80211_note_mac(struct ieee80211vap *vap, u_int8_t *mac,const  char *fmt, ...);
void ieee80211_discard_ie(struct ieee80211vap *vap, const char *type, const char *fmt, ...);
void ieee80211_discard_frame(struct ieee80211vap *vap, 
                             const struct ieee80211_frame *wh, 
                             const char *type, const char *fmt, ...);
void ieee80211_discard_mac(struct ieee80211vap *vap, u_int8_t *mac, 
                           const char *type, const char *fmt, ...);

int ieee80211_set_igtk_key(struct ieee80211vap *vap, u_int16_t keyix, ieee80211_keyval *kval);

int ieee80211_cmac_calc_mic(struct ieee80211_key *key, u_int8_t *aad, 
                                  u_int8_t *pkt, u_int32_t pktlen , u_int8_t *mic);

extern void
ieee80211_set_vht_rates(struct ieee80211com *ic, struct ieee80211vap  *vap);
#if ATH_DEBUG
/* Note: verbosity level, _verbo, not implemented yet */
#define IEEE80211_DPRINTF_IC(_ic, _verbo, _m, _fmt, ...) do {           \
        if (ieee80211_msg_ic(_ic, _m) &&                                \
            (_verbo <= IEEE80211_VERBOSE_LOUD)) {                       \
            ieee80211com_note((_ic), _fmt, ##__VA_ARGS__);              \
        }                                                               \
    } while (0)
#define ieee80211_dprintf_ic_init(_ic)                                  \
    do {                                                                \
        (_ic)->ic_print.name = "IEEE80211_IC";                          \
        (_ic)->ic_print.num_bit_specs =                                 \
            IEEE80211_MSG_LIST_LENGTH(ieee80211_msg_categories);        \
        (_ic)->ic_print.bit_specs = ieee80211_msg_categories;           \
        (_ic)->ic_print.custom_ctxt = NULL;                             \
        (_ic)->ic_print.custom_print = NULL;                            \
        asf_print_mask_set(&(_ic)->ic_print, IEEE80211_DEBUG_DEFAULT, 1); \
        if (IEEE80211_DEBUG_DEFAULT < ASF_PRINT_MASK_BITS) {            \
            asf_print_mask_set(&(_ic)->ic_print, IEEE80211_MSG_ANY, 1); \
        }                                                               \
        asf_print_ctrl_register(&(_ic)->ic_print);                      \
    } while (0)
#define ieee80211_dprintf_ic_deregister(_ic)    \
    asf_print_ctrl_unregister(&(_ic)->ic_print)
#define ieee80211_dprintf_init(_vap)                                    \
    do {                                                                \
        (_vap)->iv_print.name = "IEEE80211";                            \
        (_vap)->iv_print.num_bit_specs =                                \
            IEEE80211_MSG_LIST_LENGTH(ieee80211_msg_categories);        \
        (_vap)->iv_print.bit_specs = ieee80211_msg_categories;          \
        (_vap)->iv_print.custom_ctxt = NULL;                            \
        (_vap)->iv_print.custom_print = NULL;                           \
        asf_print_ctrl_register(&(_vap)->iv_print);                     \
    } while (0)
#define ieee80211_dprintf_deregister(_vap)          \
    asf_print_ctrl_unregister(&(_vap)->iv_print)

#define IEEE80211_DPRINTF_VB(_vap, _verbo, _m, _fmt, ...) do {              \
        if ((_verbo <= IEEE80211_VERBOSE_LOUD) && ieee80211_msg(_vap, _m))  \
            ieee80211_note(_vap, _fmt, __VA_ARGS__);                        \
    }while (0)
#define IEEE80211_MSG_LIST_LENGTH(_list) ARRAY_LENGTH(_list)

#if defined (_MAVERICK_STA_)/*Maverick Specific*/
#define IEEE80211_DPRINTF(_vap, _m, _fmt, ...) do {                     \
        if ( (_vap) && ieee80211_msg(_vap, _m))                                    \
            ieee80211_note(_vap, _fmt, __VA_ARGS__);                  \
    } while (0)

#else
#define IEEE80211_DPRINTF(_vap, _m, _fmt, ...) do {                     \
if (ieee80211_msg(_vap, _m))                                    \
ieee80211_note(_vap, _fmt, ##__VA_ARGS__);                  \
} while (0)
#endif/*_MAVERICK_STA_*/

#define IEEE80211_NOTE(_vap, _m, _ni, _fmt, ...) do {                   \
        if (ieee80211_msg(_vap, _m))                                    \
            ieee80211_note_mac(_vap, (_ni)->ni_macaddr, _fmt, ##__VA_ARGS__); \
    } while (0)
#define IEEE80211_NOTE_MAC(_vap, _m, _mac, _fmt, ...) do {              \
        if (ieee80211_msg(_vap, _m))                                    \
            ieee80211_note_mac(_vap, _mac, _fmt, ##__VA_ARGS__);        \
    } while (0)
#define IEEE80211_NOTE_FRAME(_vap, _m, _wh, _fmt, ...) do {             \
        if (ieee80211_msg(_vap, _m))                                    \
            ieee80211_note_frame(_vap, _wh, _fmt, ##__VA_ARGS__);       \
    } while (0)
#else
#define IEEE80211_DPRINTF_IC(_ic, _verbo, _m, _fmt, ...)
#define IEEE80211_DPRINTF_VB(_vap, _verbo, _m, _fmt, ...)
#define IEEE80211_MSG_LIST_LENGTH(_list) 0
#define IEEE80211_DPRINTF(_vap, _m, _fmt, ...)
#define IEEE80211_NOTE(_vap, _m, _ni, _fmt, ...)
#define IEEE80211_NOTE_MAC(_vap, _m, _mac, _fmt, ...)
#define IEEE80211_NOTE_FRAME(_vap, _m, _wh, _fmt, ...)
#define IEEE80211_DPRINTF_IC(_ic, _verbo, _m, _fmt, ...)
#define ieee80211_dprintf_ic_init(_ic)
#define ieee80211_dprintf_ic_deregister(_ic)
#define ieee80211_dprintf_init(_vap)
#define ieee80211_dprintf_deregister(_vap)
#endif /* ATH_DEBUG */

#define IEEE80211_DISCARD(_vap, _m, _wh, _type, _fmt, ...) do {         \
    if (ieee80211_msg((_vap), (_m)))                                    \
        ieee80211_discard_frame(_vap, _wh, _type, _fmt, __VA_ARGS__);   \
    } while (0)
#define IEEE80211_DISCARD_MAC(_vap, _m, _mac, _type, _fmt, ...) do {    \
    if (ieee80211_msg((_vap), (_m)))                                    \
        ieee80211_discard_mac(_vap, _mac, _type, _fmt, __VA_ARGS__);    \
    } while (0)
#define IEEE80211_DISCARD_IE(_vap, _m, _type, _fmt, ...) do {           \
    if (ieee80211_msg((_vap), (_m)))                                    \
        ieee80211_discard_ie(_vap, _type, _fmt, __VA_ARGS__);           \
    } while (0)

#ifdef ATH_CWMIN_WORKAROUND

#ifdef ATH_SUPPORT_HTC
#ifdef ATH_USB
#define IEEE80211_VI_NEED_CWMIN_WORKAROUND_INIT(_v)    \
            ((_v)->iv_vi_need_cwmin_workaround = true)
#define VAP_NEED_CWMIN_WORKAROUND(_v) ((_v)->iv_vi_need_cwmin_workaround)
#else
#define IEEE80211_VI_NEED_CWMIN_WORKAROUND_INIT(_v)
#define VAP_NEED_CWMIN_WORKAROUND(_v) (0)
#endif /* #ifdef ATH_USB */
#endif /* #ifdef ATH_SUPPORT_HTC */

#else
#define IEEE80211_VI_NEED_CWMIN_WORKAROUND_INIT(_v)
#define VAP_NEED_CWMIN_WORKAROUND(_v) (0)
#endif /* #ifdef ATH_CWMIN_WORKAROUND */

#endif /* end of _ATH_STA_IEEE80211_VAR_H */
