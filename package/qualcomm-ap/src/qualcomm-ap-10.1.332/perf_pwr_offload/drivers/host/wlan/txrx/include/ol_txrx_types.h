/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
/**
 * @file ol_txrx_types.h
 * @brief Define the major data types used internally by the host datapath SW.
 */
#ifndef _OL_TXRX_TYPES__H_
#define _OL_TXRX_TYPES__H_

#include <adf_nbuf.h>         /* adf_nbuf_t */
#include <adf_os_types.h>     /* adf_os_device_t, etc. */
#include <adf_os_lock.h>      /* adf_os_spinlock */
#include <adf_os_atomic.h>    /* adf_os_atomic_t */
#include <queue.h>            /* TAILQ */

#include <htt.h>              /* htt_pkt_type, etc. */
#include <ol_cfg.h>           /* wlan_frm_fmt */
#include <ol_ctrl_api.h>      /* ol_vdev_handle, etc. */
#include <ol_osif_api.h>      /* ol_osif_vdev_handle */
#include <ol_htt_api.h>       /* htt_pdev_handle */
#include <ol_htt_rx_api.h>    /* htt_rx_pn_t */
#include <ol_htt_tx_api.h>    /* htt_msdu_info_t */
#include <ol_txrx_htt_api.h>  /* htt_tx_status */
#include <ol_txrx_ctrl_api.h> /* ol_txrx_mgmt_tx_cb */
#include <ol_txrx_osif_api.h> /* ol_txrx_tx_fp */
#include <ol_tx.h>            /* OL_TX_MUTEX_TYPE */

#include <ol_txrx_dbg.h>      /* txrx stats */
#include <ol_txrx_prot_an.h>  /* txrx protocol analysis */
#include <wdi_event_api.h>    /* WDI subscriber event list */
#include <wal_dbg_stats.h>
#include <pktlog_ac_api.h>    /* ol_pktlog_dev_handle */
/*
 * The target may allocate multiple IDs for a peer.
 * In particular, the target may allocate one ID to represent the 
 * multicast key the peer uses, and another ID to represent the 
 * unicast key the peer uses.
 */
#define MAX_NUM_PEER_ID_PER_PEER 8

#define OL_TXRX_MAC_ADDR_LEN 6

/* OL_TXRX_NUM_EXT_TIDS -
 * 16 "real" TIDs + 3 pseudo-TIDs for mgmt, mcast/bcast & non-QoS data
 */
#define OL_TXRX_NUM_EXT_TIDS 19

#define OL_TXRX_MGMT_TYPE_BASE htt_pkt_num_types
#define OL_TXRX_MGMT_NUM_TYPES 8

#define OL_TX_MUTEX_TYPE adf_os_spinlock_t
#define OL_RX_MUTEX_TYPE adf_os_spinlock_t

struct ol_txrx_pdev_t;
struct ol_txrx_vdev_t;
struct ol_txrx_peer_t;

enum ol_tx_frm_type {
    ol_tx_frm_std = 0, /* regular frame - no added header fragments */
    ol_tx_frm_tso,     /* TSO segment, with a modified IP header added */
    ol_tx_frm_audio,   /* audio frames, with a custom LLC/SNAP header added */
};

struct ol_tx_desc_t {
    adf_nbuf_t netbuf;
    void *htt_tx_desc;
#if QCA_OL_11AC_FAST_PATH
    uint16_t            id;
#endif /* QCA_OL_11AC_FAST_PATH */
    adf_os_atomic_t ref_cnt;
    enum htt_tx_status status;

    /*
     * Allow tx descriptors to be stored in (doubly-linked) lists.
     * This is mainly used for HL tx queuing and scheduling, but is
     * also used by LL+HL for batch processing of tx frames.
     */
    TAILQ_ENTRY(ol_tx_desc_t) tx_desc_list_elem;

    /*
     * Remember whether the tx frame is a regular packet, or whether
     * the driver added extra header fragments (e.g. a modified IP header
     * for TSO fragments, or an added LLC/SNAP header for audio interworking
     * data) that need to be handled in a special manner.
     * This field is filled in with the ol_tx_frm_type enum.
     */
    u_int8_t pkt_type;
};

typedef TAILQ_HEAD(, ol_tx_desc_t) ol_tx_desc_list;

union ol_tx_desc_list_elem_t {
    union ol_tx_desc_list_elem_t *next;
    struct ol_tx_desc_t tx_desc;
};

union ol_txrx_align_mac_addr_t {
    u_int8_t raw[OL_TXRX_MAC_ADDR_LEN];
    struct {
        u_int16_t bytes_ab;
        u_int16_t bytes_cd;
        u_int16_t bytes_ef;
    } align2;
    struct {
        u_int32_t bytes_abcd;
        u_int16_t bytes_ef;
    } align4;
};

struct ol_tx_stats {
    u_int32_t num_msdu;
    u_int32_t msdu_bytes;
    u_int32_t peer_id;
};
struct ol_txrx_pdev_t {
    /* ctrl_pdev - handle for querying config info */
    ol_pdev_handle ctrl_pdev;

    /* osdev - handle for mem alloc / free, map / unmap */
    adf_os_device_t osdev;

    htt_pdev_handle htt_pdev;

#if QCA_OL_11AC_FAST_PATH
    struct CE_handle    *ce_tx_hdl; /* Handle to Tx packet posting CE */
    struct CE_handle    *ce_htt_msg_hdl; /* Handle to TxRx completion CE */
#endif /* QCA_OL_11AC_FAST_PATH */

    struct {
        int is_high_latency;
    } cfg;

    /* WDI subscriber's event list */
    wdi_event_subscribe **wdi_event_list;

    /* Pktlog pdev */
    ol_pktlog_dev_handle pl_dev;

    /* standard frame type */
    enum wlan_frm_fmt frame_format;
    enum htt_pkt_type htt_pkt_type;

    /*
     * target tx credit -
     * not needed for LL, but used for HL download scheduler to keep
     * track of roughly how much space is available in the target for
     * tx frames 
     */
    adf_os_atomic_t target_tx_credit;

    /* ol_txrx_vdev list */
    TAILQ_HEAD(, ol_txrx_vdev_t) vdev_list;

    /* peer ID to peer object map (array of pointers to peer objects) */
    struct ol_txrx_peer_t **peer_id_to_obj_map;

    struct {
        unsigned mask;
        unsigned idx_bits;
        TAILQ_HEAD(, ol_txrx_peer_t) *bins;
    } peer_hash;

    /* rx specific processing */
    struct {
        struct {
            TAILQ_HEAD(, ol_rx_reorder_t) waitlist;
            u_int32_t timeout_ms;
        } defrag;
        struct {
            int defrag_timeout_check;
            int dup_check;
        } flags;
    } rx;

    /* rx proc function */
    void (*rx_opt_proc)(
        struct ol_txrx_vdev_t *vdev,
        struct ol_txrx_peer_t *peer,
        unsigned tid,
        adf_nbuf_t msdu_list);

    /* tx management delivery notification callback functions */
    struct {
        struct {
            ol_txrx_mgmt_tx_cb cb;
            void *ctxt;
        } callbacks[OL_TXRX_MGMT_NUM_TYPES];
    } tx_mgmt;

    /* tx descriptor pool */
    struct {
        int pool_size;
        union ol_tx_desc_list_elem_t *array;
        union ol_tx_desc_list_elem_t *freelist;
    } tx_desc;

    struct {
        int (*cmp)(
            union htt_rx_pn_t *new,
            union htt_rx_pn_t *old,
            int is_unicast,
            int opmode);
        int len;
    } rx_pn[htt_num_sec_types];

    /* Monitor mode interface and status storage */
    struct ol_txrx_vdev_t *monitor_vdev;
    struct ieee80211_rx_status *rx_mon_recv_status;

    /* tx mutex */
    OL_TX_MUTEX_TYPE tx_mutex;

    /*
     * peer ref mutex:
     * 1. Protect peer object lookups until the returned peer object's
     *    reference count is incremented.
     * 2. Provide mutex when accessing peer object lookup structures.
     */
    OL_RX_MUTEX_TYPE peer_ref_mutex;

#if TXRX_STATS_LEVEL != TXRX_STATS_LEVEL_OFF
    struct {
        struct {
            struct {
                struct {
                    u_int64_t ppdus;
                    u_int64_t mpdus;
                } normal;
                struct {
                    /*
                     * mpdu_bad is general -
                     * replace it with the specific counters below
                     */
                    u_int64_t mpdu_bad;
                    //u_int64_t mpdu_fcs;
                    //u_int64_t mpdu_duplicate;
                    //u_int64_t mpdu_pn_replay;
                    //u_int64_t mpdu_bad_sender; /* peer not found */
                    //u_int64_t mpdu_flushed;
                    //u_int64_t msdu_defrag_mic_err;
                } err;
            } rx;
        } priv;
        struct ol_txrx_stats pub;
    } stats;
#endif /* TXRX_STATS_LEVEL */
    /*
     * Even if the txrx protocol analyzer debug feature is disabled,
     * go ahead and allocate the handles (pointers), since it's only
     * a few bytes of mem.
     */
    //ol_txrx_prot_an_handle tx_host_reject;
    ol_txrx_prot_an_handle prot_an_tx_sent; /* sent to HTT */
    //ol_txrx_prot_an_handle tx_completed;
    ol_txrx_prot_an_handle prot_an_rx_sent; /* sent to OS shim */
    //ol_txrx_prot_an_handle rx_forwarded;

    #if defined(ENABLE_RX_REORDER_TRACE)
    struct {
        u_int32_t mask;
        u_int32_t idx;
        u_int64_t cnt;
        #define TXRX_RX_REORDER_TRACE_SIZE_LOG2 8 /* 256 entries */
        struct {
            u_int16_t seq_num;
            u_int8_t  num_mpdus;
            u_int8_t  tid;
        } *data;
    } rx_reorder_trace;
    #endif /* ENABLE_RX_REORDER_TRACE */

    #if defined(ENABLE_RX_PN_TRACE)
    struct {
        u_int32_t mask;
        u_int32_t idx;
        u_int64_t cnt;
        #define TXRX_RX_PN_TRACE_SIZE_LOG2 5 /* 32 entries */
        struct {
            struct ol_txrx_peer_t *peer;
            u_int32_t pn32;
            u_int16_t seq_num;
            u_int8_t  unicast;
            u_int8_t  tid;
        } *data;
    } rx_pn_trace;
    #endif /* ENABLE_RX_PN_TRACE */

#if PERE_IP_HDR_ALIGNMENT_WAR
    bool        host_80211_enable;
#endif

    /* Rate-control context */
    struct {
        u_int32_t is_ratectrl_on_host;
        u_int32_t dyn_bw;
    } ratectrl;

	struct ol_tx_stats tx_stats;
};

struct ol_txrx_vdev_t {
    /* pdev - the physical device that is the parent of this virtual device */
    struct ol_txrx_pdev_t *pdev;

    /*
     * osif_vdev -
     * handle to the OS shim SW's virtual device
     * that matched this txrx virtual device
     */
    ol_osif_vdev_handle osif_vdev;

    /* vdev_id - ID used to specify a particular vdev to the target */
    u_int8_t vdev_id;

    /* MAC address */
    union ol_txrx_align_mac_addr_t mac_addr;

    /* tx paused - NO LONGER NEEDED? */

    /* node in the pdev's list of vdevs */
    TAILQ_ENTRY(ol_txrx_vdev_t) vdev_list_elem;

    /* ol_txrx_peer list */
    TAILQ_HEAD(, ol_txrx_peer_t) peer_list;

    /* transmit function used by this vdev */
    ol_txrx_tx_fp tx;

    /* receive function used by this vdev to hand rx frames to the OS shim */
    ol_txrx_rx_fp osif_rx;

#if UMAC_SUPPORT_PROXY_ARP
    /* proxy arp function */
    ol_txrx_proxy_arp_fp osif_proxy_arp;
#endif
    /*
     * receive function used by this vdev to hand rx monitor promiscuous
     * 802.11 MPDU to the OS shim
     */
    ol_txrx_rx_mon_fp osif_rx_mon;

    struct {
        /*
         * If the vdev object couldn't be deleted immediately because it still
         * had some peer objects left, remember that a delete was requested,
         * so it can be deleted once all its peers have been deleted.
         */
        int pending;
        /*
         * Store a function pointer and a context argument to provide a
         * notification for when the vdev is deleted.
         */
        ol_txrx_vdev_delete_cb callback;
        void *context;
    } delete;

    /* safe mode control to bypass the encrypt and decipher process*/
    u_int32_t safemode;

    enum wlan_op_mode opmode;

    /* Rate-control context */
    /* TODO: hbrc - Move pRatectrl and vdev_rc_info in one context struct */
    void *pRateCtrl;
    struct {
        u_int8_t  data_rc;          /* HW rate code for data frame */
        u_int8_t  max_bw;           /* maxmium allowed bandwidth */
        u_int8_t  max_nss;          /* Max NSS to use */
        u_int8_t  rts_cts;          /* RTS/CTS enabled */
        u_int8_t  def_tpc;          /* default TPC value */
        u_int8_t  def_tries;        /* Total Transmission attempt */
        u_int8_t  non_data_rc;      /* HW rate for mgmt/control frames */
        u_int32_t rc_flags;         /* RC flags SGI/LDPC/STBC etc */
        u_int32_t aggr_dur_limit;   /* max duraton any aggregate may have */
        u_int8_t  tx_chain_mask[3];
    } vdev_rc_info;
};

struct ol_rx_reorder_array_elem_t {
    adf_nbuf_t head;
    adf_nbuf_t tail;
};

struct ol_rx_reorder_t {
    unsigned win_sz_mask;
    struct ol_rx_reorder_array_elem_t *array;
    /* base - single rx reorder element used for non-aggr cases */
    struct ol_rx_reorder_array_elem_t base;

    /* only used for defrag right now */
    TAILQ_ENTRY(ol_rx_reorder_t) defrag_waitlist_elem;
    u_int32_t defrag_timeout_ms;
    /* get back to parent ol_txrx_peer_t when ol_rx_reorder_t is in a
     * waitlist */
    u_int16_t tid;
};

enum {
    txrx_sec_mcast = 0,
    txrx_sec_ucast
};

struct ol_txrx_peer_t {
    struct ol_txrx_vdev_t *vdev;

    adf_os_atomic_t ref_cnt;

    /* peer ID(s) for this peer */
    u_int16_t peer_ids[MAX_NUM_PEER_ID_PER_PEER];

    union ol_txrx_align_mac_addr_t mac_addr;

    /* node in the vdev's list of peers */
    TAILQ_ENTRY(ol_txrx_peer_t) peer_list_elem;
    /* node in the hash table bin's list of peers */
    TAILQ_ENTRY(ol_txrx_peer_t) hash_list_elem;

    /*
     * per TID info -
     * stored in separate arrays to avoid alignment padding mem overhead
     */
    struct ol_rx_reorder_t tids_rx_reorder[OL_TXRX_NUM_EXT_TIDS];
    union htt_rx_pn_t      tids_last_pn[OL_TXRX_NUM_EXT_TIDS];
    u_int8_t               tids_last_pn_valid[OL_TXRX_NUM_EXT_TIDS];
    u_int16_t              tids_last_seq[OL_TXRX_NUM_EXT_TIDS];

    struct {
        enum htt_sec_type sec_type;
        u_int32_t michael_key[2]; /* relevant for TKIP */
    } security[2]; /* 0 -> multicast, 1 -> unicast */

    /*
     * rx proc function: this either is a copy of pdev's rx_opt_proc for
     * regular rx processing, or has been redirected to a /dev/null discard
     * function when peer deletion is in progress.
     */
    void (*rx_opt_proc)(
        struct ol_txrx_vdev_t *vdev,
        struct ol_txrx_peer_t *peer,
        unsigned tid,
        adf_nbuf_t msdu_list);

    /* Rate-control context */
    void *rc_node;

    /* NAWDS Flag and Bss Peer bit */
    u_int8_t nawds_enabled:1,
                  bss_peer:1
#if WDS_VENDOR_EXTENSION
                            ,
               wds_enabled:1,
        wds_tx_mcast_4addr:1,
        wds_tx_ucast_4addr:1,
             wds_rx_filter:1, /* enforce rx filter */
        wds_rx_ucast_4addr:1, /* when set, accept 4addr unicast frames    */
        wds_rx_mcast_4addr:1  /* when set, accept 4addr multicast frames  */
#endif
        ;
};

#endif /* _OL_TXRX_TYPES__H_ */
