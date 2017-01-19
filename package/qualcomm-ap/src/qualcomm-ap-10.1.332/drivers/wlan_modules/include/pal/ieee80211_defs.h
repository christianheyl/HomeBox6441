/*
 * Copyright (c) 2007 Atheros Communications Inc.
 * All rights reserved.
 *
 * This file contains the IEEE802.11 protocol frame definitions.
 *
 */
/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */


#ifndef __IEEE80211_DEFS_H__
#define __IEEE80211_DEFS_H__

#ifndef PAL_USER_SPACE_CODE
#define __ATTRIB_PACK
#endif

/*
 * 802.11 protocol definitions.
 */

#define IEEE80211_ADDR_LEN  6       /* size of 802.11 address */

/* IEEE 802.11 PLCP header */
struct ieee80211_plcp_hdr {
    A_UINT16    i_sfd;
    A_UINT8     i_signal;
    A_UINT8     i_service;
    A_UINT16    i_length;
    A_UINT16    i_crc;
} __ATTRIB_PACK;

#define IEEE80211_PLCP_SFD      0xF3A0 
#define IEEE80211_PLCP_SERVICE  0x00

#define IEEE80211_NWID_LEN  32

typedef struct ssid_s {
    A_UINT8 ssid_len;
    A_UINT8 ssid[IEEE80211_NWID_LEN];
} ssid_t;

/*
 * 802.11 protocol crypto-related definitions.
 */

#define    IEEE80211_KEYBUF_SIZE    16
#define    IEEE80211_TX_MICKEY_LEN  8
#define    IEEE80211_RX_MICKEY_LEN  8
/* space for both tx+rx keys */
#define    IEEE80211_MICBUF_SIZE    (IEEE80211_RX_MICKEY_LEN + \
                                     IEEE80211_TX_MICKEY_LEN)    

typedef struct ieee80211_wep_key_s {
    A_UINT8 wk_keylen;
    A_UINT8 wk_key[IEEE80211_KEYBUF_SIZE];
} ieee80211_wep_key_t;

#define MAX_GROUP_KEY_INDEX 3 

/*
 * 802.11 rate set.
 */
#define IEEE80211_RATE_SIZE 8       /* 802.11 standard */
#define IEEE80211_RATE_MAXSIZE  15      /* max rates we'll handle */

struct ieee80211_rateset {
    A_UINT8 rs_nrates;
    A_UINT8 rs_rates[IEEE80211_RATE_MAXSIZE];
};

/*
 * 802.11g protection mode.
 */
enum ieee80211_protmode {
    IEEE80211_PROT_NONE     = 0,    /* no protection */
    IEEE80211_PROT_CTSONLY  = 1,    /* CTS to self */
    IEEE80211_PROT_RTSCTS   = 2,    /* RTS-CTS */
};

#define WMM_NUM_AC      4   /* 4 AC categories */

struct wmmParams {
    A_UINT8     wmmp_acm;           /* ACM parameter */
    A_UINT8     wmmp_aifsn;         /* AIFSN parameters */
    A_UINT8     wmmp_logcwmin;      /* cwmin in exponential form */
    A_UINT8     wmmp_logcwmax;      /* cwmax in exponential form */
    A_UINT16    wmmp_txopLimit;     /* txopLimit */
#ifdef NOACK_SUPPORT            
    A_UINT8     wmmp_noackPolicy;   /* No-Ack Policy: 0=ack, 1=no-ack */
#endif    
};

struct chanAccParams{
    A_INT8              cap_info;  /* ver. of the current param set */
    struct wmmParams    cap_wmmParams[WMM_NUM_AC]; /*WMM params for each access class */ 
};

/*
 * generic definitions for IEEE 802.11 frames
 */
struct ieee80211_frame {
    A_UINT8 i_fc[2];
    A_UINT8 i_dur[2];
    A_UINT8 i_addr1[IEEE80211_ADDR_LEN];
    A_UINT8 i_addr2[IEEE80211_ADDR_LEN];
    A_UINT8 i_addr3[IEEE80211_ADDR_LEN];
    A_UINT8 i_seq[2];
    /* possibly followed by addr4[IEEE80211_ADDR_LEN]; */
    /* see below */
} __ATTRIB_PACK;

struct ieee80211_pspoll_frame {
	A_UINT8	i_fc[2];
	A_UINT8	i_aid[2];
	A_UINT8	i_bssid[IEEE80211_ADDR_LEN];
	A_UINT8	i_transAddr[IEEE80211_ADDR_LEN];
} __ATTRIB_PACK;

struct ieee80211_qosframe {
    A_UINT8 i_fc[2];
    A_UINT8 i_dur[2];
    A_UINT8 i_addr1[IEEE80211_ADDR_LEN];
    A_UINT8 i_addr2[IEEE80211_ADDR_LEN];
    A_UINT8 i_addr3[IEEE80211_ADDR_LEN];
    A_UINT8 i_seq[2];
    A_UINT8 i_qos[2];
} __ATTRIB_PACK;

struct ieee80211_qoscntl {
    A_UINT8 i_qos[2];
};

struct ieee80211_frame_addr4 {
    A_UINT8 i_fc[2];
    A_UINT8 i_dur[2];
    A_UINT8 i_addr1[IEEE80211_ADDR_LEN];
    A_UINT8 i_addr2[IEEE80211_ADDR_LEN];
    A_UINT8 i_addr3[IEEE80211_ADDR_LEN];
    A_UINT8 i_seq[2];
    A_UINT8 i_addr4[IEEE80211_ADDR_LEN];
} __ATTRIB_PACK;


struct ieee80211_qosframe_addr4 {
    A_UINT8 i_fc[2];
    A_UINT8 i_dur[2];
    A_UINT8 i_addr1[IEEE80211_ADDR_LEN];
    A_UINT8 i_addr2[IEEE80211_ADDR_LEN];
    A_UINT8 i_addr3[IEEE80211_ADDR_LEN];
    A_UINT8 i_seq[2];
    A_UINT8 i_addr4[IEEE80211_ADDR_LEN];
    A_UINT8 i_qos[2];
} __ATTRIB_PACK;

#define IEEE80211_FC0_VERSION_MASK      0x03
#define IEEE80211_FC0_VERSION_SHIFT     0
#define IEEE80211_FC0_VERSION_0         0x00
#define IEEE80211_FC0_TYPE_MASK         0x0c
#define IEEE80211_FC0_TYPE_SHIFT        2
#define IEEE80211_FC0_TYPE_MGT          0x00
#define IEEE80211_FC0_TYPE_CTL          0x04
#define IEEE80211_FC0_TYPE_DATA         0x08

#define IEEE80211_FC0_SUBTYPE_MASK      0xf0
#define IEEE80211_FC0_SUBTYPE_SHIFT     4
/* for TYPE_MGT */
#define IEEE80211_FC0_SUBTYPE_ASSOC_REQ     0x00
#define IEEE80211_FC0_SUBTYPE_ASSOC_RESP    0x10
#define IEEE80211_FC0_SUBTYPE_REASSOC_REQ   0x20
#define IEEE80211_FC0_SUBTYPE_REASSOC_RESP  0x30
#define IEEE80211_FC0_SUBTYPE_PROBE_REQ     0x40
#define IEEE80211_FC0_SUBTYPE_PROBE_RESP    0x50
#define IEEE80211_FC0_SUBTYPE_BEACON        0x80
#define IEEE80211_FC0_SUBTYPE_ATIM          0x90
#define IEEE80211_FC0_SUBTYPE_DISASSOC      0xa0
#define IEEE80211_FC0_SUBTYPE_AUTH          0xb0
#define IEEE80211_FC0_SUBTYPE_DEAUTH        0xc0
#define IEEE80211_FCO_SUBTYPE_ACTION        0xd0
/* for TYPE_CTL */
#define IEEE80211_FC0_SUBTYPE_PS_POLL       0xa0
#define IEEE80211_FC0_SUBTYPE_RTS       0xb0
#define IEEE80211_FC0_SUBTYPE_CTS       0xc0
#define IEEE80211_FC0_SUBTYPE_ACK       0xd0
#define IEEE80211_FC0_SUBTYPE_CF_END        0xe0
#define IEEE80211_FC0_SUBTYPE_CF_END_ACK    0xf0
/* for TYPE_DATA (bit combination) */
#define IEEE80211_FC0_SUBTYPE_DATA      0x00
#define IEEE80211_FC0_SUBTYPE_CF_ACK        0x10
#define IEEE80211_FC0_SUBTYPE_CF_POLL       0x20
#define IEEE80211_FC0_SUBTYPE_CF_ACPL       0x30
#define IEEE80211_FC0_SUBTYPE_NODATA        0x40
#define IEEE80211_FC0_SUBTYPE_CFACK     0x50
#define IEEE80211_FC0_SUBTYPE_CFPOLL        0x60
#define IEEE80211_FC0_SUBTYPE_CF_ACK_CF_ACK 0x70
#define IEEE80211_FC0_SUBTYPE_QOS       0x80
#define IEEE80211_FC0_SUBTYPE_QOS_NULL      0xc0

#define IEEE80211_FC1_DIR_MASK          0x03
#define IEEE80211_FC1_DIR_NODS          0x00    /* STA->STA */
#define IEEE80211_FC1_DIR_TODS          0x01    /* STA->AP  */
#define IEEE80211_FC1_DIR_FROMDS        0x02    /* AP ->STA */
#define IEEE80211_FC1_DIR_DSTODS        0x03    /* AP ->AP  */

#define IEEE80211_FC1_MORE_FRAG         0x04
#define IEEE80211_FC1_RETRY             0x08
#define IEEE80211_FC1_PWR_MGT           0x10
#define IEEE80211_FC1_MORE_DATA         0x20
#define IEEE80211_FC1_WEP               0x40
#define IEEE80211_FC1_ORDER             0x80

#define IEEE80211_SEQ_FRAG_MASK         0x000f
#define IEEE80211_SEQ_FRAG_SHIFT        0
#define IEEE80211_SEQ_SEQ_MASK          0xfff0
#define IEEE80211_SEQ_SEQ_SHIFT         4

#define IEEE80211_QOS_TXOP              0x00ff
/* bit 8 is reserved */
#define IEEE80211_QOS_ACKPOLICY         0x60
#define IEEE80211_QOS_ACKPOLICY_S       5
#define IEEE80211_QOS_ESOP              0x10
#define IEEE80211_QOS_ESOP_S            4
#define IEEE80211_QOS_TID               0x0f



#define IEEE80211_CAPINFO_ESS               0x0001
#define IEEE80211_CAPINFO_IBSS              0x0002
#define IEEE80211_CAPINFO_CF_POLLABLE       0x0004
#define IEEE80211_CAPINFO_CF_POLLREQ        0x0008
#define IEEE80211_CAPINFO_PRIVACY           0x0010
#define IEEE80211_CAPINFO_SHORT_PREAMBLE    0x0020
#define IEEE80211_CAPINFO_PBCC              0x0040
#define IEEE80211_CAPINFO_CHNL_AGILITY      0x0080
/* bits 8-9 are reserved */
#define IEEE80211_CAPINFO_SHORT_SLOTTIME    0x0400
#define IEEE80211_CAPINFO_APSD              0x0800
/* bit 12 is reserved */
#define IEEE80211_CAPINFO_DSSSOFDM          0x2000



/*
 * WMM/802.11e information element.
 */
struct ieee80211_ie_wmm {
    A_UINT8 wmm_id;     /* IEEE80211_ELEMID_VENDOR */
    A_UINT8 wmm_len;    /* length in bytes */
    A_UINT8 wmm_oui[3]; /* 0x00, 0x50, 0xf2 */
    A_UINT8 wmm_type;   /* OUI type */
    A_UINT8 wmm_subtype;    /* OUI subtype */
    A_UINT8 wmm_version;    /* spec revision */
    A_UINT8 wmm_info;   /* QoS info */
} __ATTRIB_PACK;

/*
 * 802.11e QBSS Information element.
 */
struct ieee80211_qbss_ie {
    A_UINT8  qbss_id;
    A_UINT8  qbss_len;
    A_UINT16 qbss_stationCount;
    A_UINT8  qbss_channelUtilization;
    A_UINT16 qbss_aac;
}__ATTRIB_PACK;

/*
 * WMM/802.11e Tspec Element
 */
typedef struct wmm_tspec_ie_t {
    A_UINT8     elementId;
    A_UINT8     len;
    A_UINT8     oui[3];
    A_UINT8     ouiType;
    A_UINT8     ouiSubType;
    A_UINT8     version;
    A_UINT16    tsInfo_info;
    A_UINT8     tsInfo_reserved;
    A_UINT16    nominalMSDU;
    A_UINT16    maxMSDU;
    A_UINT32    minServiceInt;
    A_UINT32    maxServiceInt;
    A_UINT32    inactivityInt;
    A_UINT32    suspensionInt;
    A_UINT32    serviceStartTime;
    A_UINT32    minDataRate;
    A_UINT32    meanDataRate;
    A_UINT32    peakDataRate;
    A_UINT32    maxBurstSize;
    A_UINT32    delayBound;
    A_UINT32    minPhyRate;
    A_UINT16    sba;
    A_UINT16    mediumTime;
} __ATTRIB_PACK WMM_TSPEC_IE;

enum ACTION_CATEGORY {
    ACTION_CATEGORY_CODE_SPECMGMT             = 0,
    ACTION_CATEGORY_CODE_QOS                  = 1,
    ACTION_CATEGORY_CODE_DLS                  = 2,
    ACTION_CATEGORY_CODE_BLOCK_ACK            = 3,
    ACTION_CATEGORY_CODE_RADIO_MEASUREMENT    = 5,
    ACTION_CATEGORY_CODE_TSPEC                = 17,
};

enum ACTION_FRAME_FORMAT_SPEC_MGMT {
    ACTION_CODE_MEASUREMENT_REQUEST           = 0,
    ACTION_CODE_MEASUREMENT_REPORT            = 1,
    ACTION_CODE_TPC_REQUEST                   = 2,
    ACTION_CODE_TPC_REPORT                    = 3,
    ACTION_CODE_CHANNEL_SWITCH_ANNOUNCEMENT   = 4,
};

enum ACTION_FRAME_FORMAT_TSPECS {
    ACTION_CODE_TSPEC_ADDTS                   = 0,
    ACTION_CODE_TSPEC_ADDTS_RESP              = 1,
    ACTION_CODE_TSPEC_DELTS                   = 2,
};

enum ACTION_FRAME_FORMAT_RM {
    ACTION_CODE_RADIO_MEASUREMENT_REQUEST     = 0,
    ACTION_CODE_RADIO_MEASUREMENT_REPORT      = 1,
    ACTION_CODE_LINK_MEASUREMENT_REQUEST      = 2,
    ACTION_CODE_LINK_MEASUREMENT_REPORT       = 3,
    ACTION_CODE_NEIGHBOR_REPORT_REQUEST       = 4,
    ACTION_CODE_NEIGHBOR_REPORT_RESPONSE      = 5,
    ACTION_CODE_MEASUREMENT_PILOT             = 6,
};

#define WMM_TSPEC_INFO_LEN          61

#define TSPEC_USER_PRIORITY_MASK    0x7
#define TSPEC_USER_PRIORITY_S       11

#define TSPEC_PS_MASK               0x1
#define TSPEC_PS_S                  10

#define TSPEC_ACCESS_POLICY_MASK    0x3
#define TSPEC_ACCESS_POLICY_S       7

#define TSPEC_DIRECTION_MASK        0x3
#define TSPEC_DIRECTION_S           5

#define TSPEC_TSID_MASK             0xF
#define TSPEC_TSID_S                1

#define TSPEC_TRAFFIC_TYPE_MASK     0x1
#define TSPEC_TRAFFIC_TYPE_S        0



/*
 * WMM AC parameter field
 */

struct ieee80211_wmm_acparams {
    A_UINT8     acp_aci_aifsn;
    A_UINT8     acp_logcwminmax;
    A_UINT16    acp_txop;
} __ATTRIB_PACK;

/*
 * WMM Parameter Element
 */

struct ieee80211_wmm_param {
    A_UINT8 param_id;
    A_UINT8 param_len;
    A_UINT8 param_oui[3];
    A_UINT8 param_oui_type;
    A_UINT8 param_oui_sybtype;
    A_UINT8 param_version;
    A_UINT8 param_qosInfo;
    A_UINT8 param_reserved;
    struct ieee80211_wmm_acparams   params_acParams[WMM_NUM_AC];
} __ATTRIB_PACK;

#ifdef PAL_USER_SPACE_CODE
typedef struct ieee80211_ath_capInfo {
    A_UINT8     usePowerSaving:1;
    A_UINT8     reserved:7;
} ATHADVCAP_INFO;
#else
typedef struct ieee80211_ath_capInfo {
    A_UINT8     usePowerSaving;
} ATHADVCAP_INFO;
#endif

/*
 * Atheros Advanced Capability information element.
 */
struct ieee80211_ie_athAdvCap {
    A_UINT8         athAdvCap_id;       /* IEEE80211_ELEMID_VENDOR */
    A_UINT8         athAdvCap_len;      /* length in bytes */
    A_UINT8         athAdvCap_oui[3];   /* 0x00, 0x03, 0x7f */
    A_UINT8         athAdvCap_type;     /* OUI type */
    A_UINT8         athAdvCap_subtype;  /* OUI subtype */
    A_UINT8         athAdvCap_version;  /* spec revision */
    ATHADVCAP_INFO  athAdvCap_capability;   /* Capability info */
    A_UINT16        athAdvCap_defTxTimeout;
    A_UINT16        athAdvCap_defRxTimeout;
} __ATTRIB_PACK;

/*
 * Management Notification Frame
 */
struct ieee80211_mnf {
    A_UINT8 mnf_category;
    A_UINT8 mnf_action;
    A_UINT8 mnf_dialog;
    A_UINT8 mnf_status;
} __ATTRIB_PACK;
#define MNF_SETUP_REQ   0
#define MNF_SETUP_RESP  1
#define MNF_TEARDOWN    2


/*
 * 802.11i/WPA information element (maximally sized).
 */
struct ieee80211_ie_wpa {
    A_UINT8     wpa_id;     /* IEEE80211_ELEMID_VENDOR */
    A_UINT8     wpa_len;    /* length in bytes */
    A_UINT8     wpa_oui[3]; /* 0x00, 0x50, 0xf2 */
    A_UINT8     wpa_type;   /* OUI type */
    A_UINT16    wpa_version;    /* spec revision */
    A_UINT32    wpa_mcipher[1]; /* multicast/group key cipher */
    A_UINT16    wpa_uciphercnt; /* # pairwise key ciphers */
    A_UINT32    wpa_uciphers[8];/* ciphers */
    A_UINT16    wpa_authselcnt; /* authentication selector cnt*/
    A_UINT32    wpa_authsels[8];/* selectors */
    A_UINT16    wpa_caps;   /* 802.11i capabilities */
    A_UINT16    wpa_pmkidcnt;   /* 802.11i pmkid count */
    A_UINT16    wpa_pmkids[8];  /* 802.11i pmkids */
} __ATTRIB_PACK;

/*
 * Management information element payloads.
 */

enum {
    IEEE80211_ELEMID_SSID       = 0,
    IEEE80211_ELEMID_RATES      = 1,
    IEEE80211_ELEMID_FHPARMS    = 2,
    IEEE80211_ELEMID_DSPARMS    = 3,
    IEEE80211_ELEMID_CFPARMS    = 4,
    IEEE80211_ELEMID_TIM        = 5,
    IEEE80211_ELEMID_IBSSPARMS  = 6,
    IEEE80211_ELEMID_COUNTRY    = 7,
    IEEE80211_ELEMID_QBSS       = 11,
    IEEE80211_ELEMID_CHALLENGE  = 16,
    /* 17-31 reserved for challenge text extension */
    IEEE80211_ELEMID_PWRCNSTR   = 32,
    IEEE80211_ELEMID_PWRCAP     = 33,
    IEEE80211_ELEMID_TPCREQ     = 34,
    IEEE80211_ELEMID_TPCREP     = 35,
    IEEE80211_ELEMID_SUPPCHAN   = 36,
    IEEE80211_ELEMID_CHANSWITCH = 37,
    IEEE80211_ELEMID_MEASREQ    = 38,
    IEEE80211_ELEMID_MEASREP    = 39,
    IEEE80211_ELEMID_QUIET      = 40,
    IEEE80211_ELEMID_IBSSDFS    = 41,
    IEEE80211_ELEMID_ERP        = 42,
    IEEE80211_ELEMID_RSN        = 48,
    IEEE80211_ELEMID_XRATES     = 50,
    IEEE80211_ELEMID_TPC        = 150,
    IEEE80211_ELEMID_CCKM       = 156,
    IEEE80211_ELEMID_VENDOR     = 221,  /* vendor private */
};

#define IEEE80211_SSID_MAXLEN   32

typedef struct {
    A_UINT8 elemId;
    A_UINT8 length;
    A_UINT8 txpower;
    A_UINT8 linkMargin;
} __ATTRIB_PACK tpc_report_t;

typedef struct {
    A_UINT8 elemId;
    A_UINT8 length;
} __ATTRIB_PACK tpc_request_t;

struct ieee80211_tim_ie {
    A_UINT8 tim_ie;         /* IEEE80211_ELEMID_TIM */
    A_UINT8 tim_len;
    A_UINT8 dtim_count;     /* DTIM count */
    A_UINT8 dtim_period;        /* DTIM period */
    A_UINT8 tim_bitctl;     /* bitmap control */
    A_UINT8 tim_bitmap[1];      /* variable-length bitmap */
} __ATTRIB_PACK;

#define IEEE80211_COUNTRYIE_MIN_LEN     6
#define IEEE80211_COUNTRYIE_BAND_MAX    4

struct ieee80211_country_ie {
    A_UINT8 ie;         /* IEEE80211_ELEMID_COUNTRY */
    A_UINT8 len;
    A_UINT8 cc[3];          /* ISO CC+(I)ndoor/(O)utdoor */
    struct country_ie_band {
        A_UINT8 schan;          /* starting channel */
        A_UINT8 nchan;          /* number channels */
        A_UINT8 maxtxpwr;       /* tx power cap */
    } band[IEEE80211_COUNTRYIE_BAND_MAX] __ATTRIB_PACK;
} __ATTRIB_PACK;


#define IEEE80211_SUB_BAND_MAX   20
struct ieee80211_powercaps_ie {
   A_UINT8   ie;
   A_UINT8   len;
   A_INT8    minPwr;
   A_INT8    maxPwr;
} __ATTRIB_PACK;

struct ieee80211_suppchan_ie {
   A_UINT8   ie;
   A_UINT8   len;
   struct subband_ {
      A_UINT8   schan;
      A_UINT8   nchan;
   } subband[IEEE80211_SUB_BAND_MAX] __ATTRIB_PACK;
} __ATTRIB_PACK;

#define IEEE80211_CHALLENGE_LEN     128

#define IEEE80211_TPCREP_LEN            4
#define IEEE80211_PWR_CONSTRNT_LEN      1
#define IEEE80211_CSA_IE_LEN            3

#define IEEE80211_RATE_BASIC        0x80
#define IEEE80211_RATE_VAL      0x7f

/* EPR information element flags */
#define IEEE80211_ERP_NON_ERP_PRESENT   0x01
#define IEEE80211_ERP_USE_PROTECTION    0x02
#define IEEE80211_ERP_LONG_PREAMBLE 0x04

#define IEEE80211_RSN_IE_LEN                 22

#endif /* __IEEE80211_DEFS_H__ */
