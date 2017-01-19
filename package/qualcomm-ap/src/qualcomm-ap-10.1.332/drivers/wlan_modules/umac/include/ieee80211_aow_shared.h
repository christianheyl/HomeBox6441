/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
/*
 * =====================================================================================
 *
 *       Filename:  ieee80211_aow_shared.h
 *
 *    Description:  Header file for AOW data structures shared between driver and
                    application space.
 *
 *        Version:  1.0
 *        Created:  Wednesday 23 June 2010 16:47:44  IST
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Krishna C. Rao
 *        Company:  Atheros
 *
 * =====================================================================================
 */

#ifndef IEEE80211_AOW_SHARED_H
#define IEEE80211_AOW_SHARED_H

#ifndef NETLINK_ATHEROS_AOW_ES
#define NETLINK_ATHEROS_AOW_ES      (NETLINK_GENERIC + 3)
#endif

#ifndef NETLINK_ATHEROS_AOW_CM
#define NETLINK_ATHEROS_AOW_CM      (NETLINK_GENERIC + 4)
#endif

/* Keep the below in sync with AOW_MAX_RECEIVER_COUNT in
   lmac/ath_dev/ath_aow.h */
#define AOW_MAX_RECVRS              (10)

#define ATH_AOW_TIME_PER_MPDU       (2)                    /* In ms */

#define NUM_MICROSEC_PER_SEC        (1000000)
#define NUM_MILLISEC_PER_SEC        (1000)

/* Masks for WLAN and subframe sequence nos. */
#define ATH_AOW_PL_SUBFRME_SEQ_S    (0)
#define ATH_AOW_PL_SUBFRME_SEQ_MASK (0x3)
#define ATH_AOW_PL_WLAN_SEQ_S       (2)
#define ATH_AOW_PL_WLAN_SEQ_MASK    (0xFFF)

/* Per-MPDU Packet Lost Indication Rx side status codes */
#define ATH_AOW_PL_INFO_MPDU_LOST   (1)
#define ATH_AOW_PL_INFO_DELAYED     (2)
#define ATH_AOW_PL_INFO_INTIME      (3)

/* Masks for Tx attempts and Tx status info */
#define ATH_AOW_PL_NUM_ATTMPTS_S    (0)
#define ATH_AOW_PL_NUM_ATTMPTS_MASK (0xFF)
#define ATH_AOW_PL_TX_STATUS_S      (8)
#define ATH_AOW_PL_TX_STATUS_MASK   (0xF)
#define ATH_AOW_PL_RECVR_CODE_S     (12)
#define ATH_AOW_PL_RECVR_CODE_MASK  (0xF)

/* Per-MPDU Packet Lost Indication Tx side status codes */
/* Note: We can add more codes to give the reason for failure.
   For now, ATH_AOW_PL_INFO_TX_FAIL is a plain code without
   the reason for failure. */
#define ATH_AOW_PL_INFO_TX_SUCCESS  (1)
#define ATH_AOW_PL_INFO_TX_FAIL     (2)

/* Mask defs for Netlink header information */
#define ATH_AOW_NL_TYPE_S           (0)
/* Though 1 bit should be enough for now, we provision for
   future scenarious where there might be more than 2 types */
#define ATH_AOW_NL_TYPE_MASK        (0x3)

#define ATH_AOW_NODE_TYPE_TX        (1)
#define ATH_AOW_NODE_TYPE_RX        (2)

/* AoW Control packet related limits */
#define AOW_CONTROL_MAXBYTES        (512)

#if ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG
struct aow_advncd_txinfo {
    
    /* Taken from struct ath_tx_status */

    u_int32_t    ts_tstamp;     /* h/w assigned timestamp */
    u_int16_t    ts_seqnum;     /* h/w assigned sequence number */
    u_int8_t     ts_status;     /* frame status, 0 => xmit ok */
    u_int8_t     ts_ratecode;   /* h/w transmit rate code */
    u_int8_t     ts_rateindex;  /* h/w transmit rate index */
    int8_t       ts_rssi;       /* tx ack RSSI */
    u_int8_t     ts_shortretry; /* # short retries */
    u_int8_t     ts_longretry;  /* # long retries */
    u_int8_t     ts_virtcol;    /* virtual collision count */
    u_int8_t     ts_antenna;    /* antenna information */

        /* Additional status information */
    u_int8_t     ts_flags;       /* misc flags */
    int8_t       ts_rssi_ctl0;   /* tx ack RSSI [ctl, chain 0] */
    int8_t       ts_rssi_ctl1;   /* tx ack RSSI [ctl, chain 1] */
    int8_t       ts_rssi_ctl2;   /* tx ack RSSI [ctl, chain 1] */
    int8_t       ts_rssi_ext0;   /* tx ack RSSI [ext, chain 1] */
    int8_t       ts_rssi_ext1;   /* tx ack RSSI [ext, chain 1] */
    int8_t       ts_rssi_ext2;   /* tx ack RSSI [ext, chain 1] */
    u_int8_t     queue_id;       /* queue number */
    u_int16_t    desc_id;        /* descriptor identifier */

    u_int32_t    ba_low;         /* blockack bitmap low */
    u_int32_t    ba_high;        /* blockack bitmap high */
    u_int32_t    evm0;           /* evm bytes */
    u_int32_t    evm1;
    u_int32_t    evm2;

    u_int8_t     ts_txbfstatus;  /* Tx bf status */ 
#ifdef ATH_SUPPORT_TxBF
#define	AR_BW_Mismatch      0x1
#define	AR_Stream_Miss      0x2
#define	AR_CV_Missed        0x4
#define AR_Dest_Miss        0x8
#define AR_Expired          0x10
#endif
    u_int8_t     tid;           /*TID of the transmit unit in case of aggregate */

    /* Taken from struct ath_buf */
    u_int32_t               bf_status;
    u_int32_t               bf_flags;    /* tx descriptor flags */
#if ATH_SUPPORT_IQUE && ATH_SUPPORT_IQUE_EXT
    u_int32_t               bf_txduration;/* Tx duration of this buf */
#endif
    u_int16_t               bf_avail_buf;
    u_int16_t               bf_reftxpower;  /* reference tx power */

    /* Taken from struct struct ath_buf_state */
    int                     bfs_nframes;        /* # frames in aggregate */
    u_int16_t               bfs_al;             /* length of aggregate */
    u_int16_t               bfs_frmlen;         /* length of frame */
    int                     bfs_seqno;          /* sequence number */
    int                     bfs_tidno;          /* tid of this frame */
    int                     bfs_retries;        /* current retries */
    u_int8_t                bfs_useminrate;     /* use minrate */
    u_int8_t                bfs_ismcast;        /* is mcast packet */
    u_int8_t                bfs_isdata;         /* is a data frame/aggregate */
    u_int8_t                bfs_isaggr;         /* is an aggregate */
    u_int8_t                bfs_isampdu;        /* is an a-mpdu, aggregate or not */
    u_int8_t                bfs_ht;             /* is an HT frame */
    u_int8_t                bfs_isretried;      /* is retried */
    u_int8_t                bfs_isxretried;     /* is excessive retried */
    u_int8_t                bfs_shpreamble;     /* is short preamble */
    u_int8_t                bfs_isbar;          /* is a BAR */
    u_int8_t                bfs_ispspoll;       /* is a PS-Poll */
    u_int8_t                bfs_aggrburst;      /* is a aggr burst */
    u_int8_t                bfs_calcairtime;    /* requests airtime be calculated when set for tx frame */
#ifdef ATH_SUPPORT_UAPSD
    u_int8_t                bfs_qosnulleosp;    /* is QoS null EOSP frame */
#endif
    u_int8_t                bfs_ispaprd;        /* is PAPRD frame */
    u_int8_t                bfs_isswaborted;    /* is the frame marked as sw aborted*/
#if ATH_SUPPORT_CFEND
    u_int8_t                bfs_iscfend;        /* is QoS null EOSP frame */
#endif
#ifdef ATH_SWRETRY
    u_int8_t                bfs_isswretry;      /* is the frame marked for swretry*/
    int                     bfs_swretries;      /* number of swretries made*/
    int                     bfs_totaltries;     /* total tries including hw retries*/
#endif
    int                     bfs_qnum;           /* h/w queue number */
    int                     bfs_rifsburst_elem; /* RIFS burst/bar */
    int                     bfs_nrifsubframes;  /* # of elements in burst */
    u_int8_t                bfs_txbfstatus;     /* for TxBF , txbf status from TXS*/

    int                     loopcount;

} __packed;
#endif /* ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG */

#ifdef ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
struct aow_advncd_rxinfo {
    u_int32_t latency;
    u_int64_t timestamp;
    u_int64_t rxTime;           /* TSF time when audio data was received by driver */
    u_int64_t deliveryTime;     /* TSF time when audio data was delivered to AoW module */
} __packed;
#endif /* ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG */

#if ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG && ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
#define AOW_NL_DATA_MAX_SIZE        (32 + sizeof(struct aow_advncd_txinfo) + sizeof(struct aow_advncd_rxinfo)) 
#elif ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG
#define AOW_NL_DATA_MAX_SIZE        (32 + sizeof(struct aow_advncd_txinfo)) 
#elif ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
#define AOW_NL_DATA_MAX_SIZE        (32 + sizeof(struct aow_advncd_rxinfo)) 
#else
#define AOW_NL_DATA_MAX_SIZE        (32) 
#endif

/* Per-MPDU Packet Lost Indication */

/* Note: If any of the below structures sent over Netlink is changed,
   ensure that the value of AOW_NL_DATA_MAX_SIZE is kept in sync with the 
   change. */

struct pl_element {
    u_int32_t    seqno;
    
    /* Will carry:
       a) Subframe sequence no. (2 least significant bits)
       b) WLAN sequence no. (Next 12 significant bits) 
    */
    u_int16_t    subfrme_wlan_seqnos;
    
    /* Will carry transmit side or receive side info as appropriate.
       If we are a transmitter: info = *) No. of retries for this MPDU
                                       *) Tx status
                                       *) Reciever code (sent blank by
                                          the driver -can be filled & used by 
                                          the application for optimized
                                          storage and processing).
       If we are a receiver:    info = One of the ATH_AOW_PL_INFO
                                       status codes.
     */
    u_int16_t    info;

#if ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG
    struct aow_advncd_txinfo atxinfo;    
#endif

#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
    struct aow_advncd_rxinfo arxinfo;    
#endif
} __packed;


/* Header information for data sent over Netlink */
struct aow_nl_packet_body_tx {
    struct pl_element elmnt;
    struct ether_addr recvr; 
} __packed;

struct aow_nl_packet_body_rx {
     struct pl_element elmnt;
} __packed;

typedef u_int16_t aow_nl_packet_hdr_t;

struct aow_nl_packet {
    /* Currently, this contains a flag which informs the receiver if 
       this is transmitter or receiver info. Can be used for 
       other purposes in the future */
    aow_nl_packet_hdr_t header;

    union {
        struct aow_nl_packet_body_tx txdata;
        struct aow_nl_packet_body_rx rxdata;
    } body; 
} __packed;

#define AOW_NL_PACKET_TX_SIZE (sizeof(struct aow_nl_packet_body_tx)\
                               + sizeof(aow_nl_packet_hdr_t))
#define AOW_NL_PACKET_RX_SIZE (sizeof(struct aow_nl_packet_body_rx)\
                               + sizeof(aow_nl_packet_hdr_t))

/*
 * @brief   : AoW message header format
 */
typedef struct aow_ctrl_msg_hdr {
    u_int16_t seq_no;           /* packet sequence number */
    u_int8_t src_addr;         /* source address */
    u_int8_t dst_addr;          /* destination address */
    u_int16_t len;               /* length of the packet */
    u_int16_t desc;             /* info field */
} __packed aow_ctrl_msg_hdr_t;


/*
 * @breief  : AoW message 
 */

#define AOW_CTRL_MSG_MAX_LENGTH     512
typedef struct aow_ctrl_msg {
    aow_ctrl_msg_hdr_t h;
    u_int8_t data[AOW_CTRL_MSG_MAX_LENGTH];
}__packed aow_ctrl_msg_t;    

#define AOW_CM_LOCAL_COMMAND_FLAG   0x8000


/*
 * @brief   : Connection manager command types
 */
typedef enum {
    AOW_INIT_NW,
    AOW_START_NW,
    AOW_STOP_NW,
    AOW_DISCONNECT_NW,
    AOW_CHANGE_NW_ID,
    AOW_UPDATE_SECURITY_MODE,
    AOW_CHANGE_SECURITY_MODE,
    AOW_SLEEP_NW,
    AOW_GET_CURRENT_STATUS,
    AOW_SET_PARAMETERS,
    AOW_ADD_IE,
    AOW_DEL_IE,
    AOW_INIT_SECURITY_MODE,
    AOW_DEVICE_TYPE,
    AOW_BEACON_INTERVAL,
    AOW_DISCONNECT_DEVICE,
    AOW_SET_AUD_CHANNEL,
    AOW_SEND_AUD_PROTO_DATA,
    AOW_UPDATE_VOLUME,
    AOW_SET_MEMORY,
    AOW_REQUEST_VERSION,
    AOW_REQUEST_SEND_BUF_STATUS,
    AOW_REQUEST_GET_MEMORY,

    /* This indicates the number of comamnd types */
    AOW_MAX_COMMAND_TYPES,

} AOW_CM_COMMAND_TYPE;



/*
 * @brief   : Connection manager events
 */

typedef enum {
    AOW_INIT_NW_CNF,
    AOW_START_NW_CNF,
    AOW_CONNECTED_IND,
    AOW_DISCONNECTED_IND,
    AOW_BDV_NOT_FOUND_IND,
    AOW_CONNECTION_FAIL_IND,
    AOW_UPDATE_SECURITY_MODE_CNF,
    AOW_CHANGE_SECURITY_MODE_CNF,
    AOW_CHANGE_NW_ID_CNF,
    AOW_CURRENT_STATE_IND,
    AOW_LINK_QUALITY_IND,
    AOW_STATE_UPDATE_IND,

    /* This indicates the number of event types */
    AOW_MAX_EVENT_TYPES,
} AOW_CM_EVENT_TYPE;    


typedef struct aow_ctrl_init_nw_msg {
    u_int8_t ssid_len;
    u_int8_t psk0_len;
    u_int8_t psk1_len;
    u_int8_t psk2_len;
    u_int8_t psk3_len;
    u_int8_t psk4_len;
    u_int8_t pad0;
    u_int8_t pad1;
}aow_ctrl_init_nw_msg_t;    

#define MAX_AOW_CTRL_DATA_LENGTH 48

typedef struct aow_ctrl_data_msg {
    u_int32_t length;
    u_int8_t addr[6];
    u_int8_t data[MAX_AOW_CTRL_DATA_LENGTH];
}aow_ctrl_data_msg_t;    

#endif /* IEEE80211_AOW_SHARED_H */
