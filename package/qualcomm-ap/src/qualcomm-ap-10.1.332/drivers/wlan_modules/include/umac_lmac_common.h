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
 * This module define the constant and enum that are used between the upper
 * net80211 layer and lower device ath layer.
 * NOTE: Please do not add any functional prototype in here and also do not
 * include any other header files.
 * Only constants, enum, and structure definitions.
 */

#ifndef UMAC_LMAC_COMMON_H
#define UMAC_LMAC_COMMON_H

/* The following define the various VAP Information Types to register for callbacks */
typedef u_int32_t   ath_vap_infotype;
#define ATH_VAP_INFOTYPE_SLOT_OFFSET    (1<<0)

#if UMAC_SUPPORT_SMARTANTENNA
#define SA_MAX_RSSI_SAMPLES 10 /* MAX RSSI sample collected from training stats */
#define SA_MAX_RECV_CHAINS 3   /* MAX receive chains */
#define SA_ENABLE_HW_ALGO 3    /* setting these bits enables smart antenna */

#if (NUM_TX_CHAINS > 4)
#error "Unsupported Number of TX chains for Smart Antenna"
#endif

#if (NUM_TX_CHAINS == 4)
#define SA_ANTENNA_COMBINATIONS (SA_NUMANT_PERCHAIN*SA_NUMANT_PERCHAIN*SA_NUMANT_PERCHAIN*SA_NUMANT_PERCHAIN) /* Max antenna combination */
#elif (NUM_TX_CHAINS == 3)
#define SA_ANTENNA_COMBINATIONS (SA_NUMANT_PERCHAIN*SA_NUMANT_PERCHAIN*SA_NUMANT_PERCHAIN) /* Max antenna combination */
#elif (NUM_TX_CHAINS == 2)
#define SA_ANTENNA_COMBINATIONS (SA_NUMANT_PERCHAIN*SA_NUMANT_PERCHAIN) /* Max antenna combination */
#else 
#define SA_ANTENNA_COMBINATIONS (SA_NUMANT_PERCHAIN) /* Max antenna combination */
#endif

#if SA_11N_SUPPORT
#define SA_MAX_RATES (12 + (NUM_TX_CHAINS << 3))  /* 4 (11b rates) + 8 (11g) + 8 (Ht rates) * num tx chains */
#else 
#define SA_MAX_RATES 12  /* 4 (11b rates) + 8 (11g) */ 
#endif


struct sa_train_data {
    u_int8_t antenna;    /* anntenna for which stats are colleted*/
    u_int8_t ratecode;   /* rateCode at which training is happening */
    u_int16_t nFrames;   /* Total number of trasmited frames */
    u_int16_t nBad;      /* Total number of failed frames */
    int8_t rssi[SA_MAX_RECV_CHAINS][SA_MAX_RSSI_SAMPLES];  /* Block ACK/ACK RSSI for all chains */
    u_int16_t last_nFrames; /* packets sent in last interval*/
    u_int16_t numpkts;   /* Number of packets required for training*/
    u_int8_t cts_prot;   /* CTS protection flag */
    u_int8_t samples;    /* number of RSSI smaples */
};

struct sa_ratetoindex {     /* Valid Rate Index table from RC module */
    u_int8_t ratecode;   /* rate code */
    u_int8_t rateindex;  /* rate index */
};  
    
struct sa_rate_info {
    struct sa_ratetoindex rates[SA_MAX_RATES];  /* max rates */
    u_int8_t num_of_rates; /* number of rates */
    u_int8_t selected_antenna; /* selected antenna */
};
#endif


#ifdef ATH_USB
#define    ATH_BCBUF    8        /* should match ATH_USB_BCBUF defined in ath_usb.h */
#elif ATH_SUPPORT_AP_WDS_COMBO
#define    ATH_BCBUF    16       /* number of beacon buffers for 16 VAP support */
#elif IEEE80211_PROXYSTA
#define    ATH_BCBUF    17       /* number of beacon buffers for 17 VAP support (STA + 16 proxies)*/
#else
#define    ATH_BCBUF    16        /* number of beacon buffers */
#endif


#if ATH_SUPPORT_AP_WDS_COMBO

/*
 * Define the scheme that we select MAC address for multiple AP-WDS combo VAPs on the same radio.
 * i.e. combination of normal & WDS AP vaps.
 *
 * The normal AP vaps use real MAC address while WDS AP vaps use virtual MAC address.
 * The very first AP VAP will just use the MAC address from the EEPROM.
 * 
 * - Default HW mac address maps to index 0
 * - Index 1 maps to default HW mac addr + 1
 * - Index 2 maps to default HW mac addr + 2 ...
 * 
 * Real MAC address is calculated as follows:
 * for 8 vaps, 3 bits of the last byte are used (bits 4,5,6 and mask of 7) for generating the 
 * BSSID of the VAP.
 * 
 * Virtual MAC address is calculated as follows:
 * we set the Locally administered bit (bit 1 of first byte) in MAC address,
 * and use the bits 4,5,6 for generating the BSSID of the VAP.
 *
 * For each Real MAC address there will be a corresponding virtual MAC address
 *
 * BSSID bits are used to generate index during GET operation.
 * No of bits used depends on ATH_BCBUF value.
 */

#define ATH_SET_VAP_BSSID_MASK(bssid_mask)							\
    do {											\
	((bssid_mask)[0] &= ~(((ATH_BCBUF - 1) << 4) | 0x02));                                  \
	((bssid_mask)[IEEE80211_ADDR_LEN - 1] &= ~((ATH_BCBUF >> 1) - 1));                      \
    } while(0)

#define ATH_GET_VAP_ID(bssid, hwbssid, id)                                                     	\
    do {											\
	id = bssid[IEEE80211_ADDR_LEN - 1] & ((ATH_BCBUF >> 1) - 1);                            \
	if (bssid[0] & 0x02) id += (ATH_BCBUF >> 1);                                            \
    } while(0)

#define ATH_SET_VAP_BSSID(bssid, hwbssid, id)                                                  	\
    do {											\
	u_int8_t hw_bssid = (hwbssid[0] >> 4) & (ATH_BCBUF - 1);                               	\
	u_int8_t tmp_bssid;									\
	u_int8_t tmp_id = id;									\
	    											\
	if (tmp_id > ((ATH_BCBUF >> 1) - 1)) {                                                     	\
           tmp_id -= (ATH_BCBUF >> 1) ;								\
	   (bssid)[0] &= ~((ATH_BCBUF - 1) << 4);						\
           tmp_bssid = ((1 + hw_bssid) & (ATH_BCBUF - 1));                                    	\
	   (bssid)[0] |= (((tmp_bssid) << 4) | 0x02);                                         	\
	}											\
	hw_bssid = (hwbssid[IEEE80211_ADDR_LEN - 1]) & ((ATH_BCBUF >> 1) - 1);                   \
	tmp_bssid = ((tmp_id + hw_bssid) & ((ATH_BCBUF >> 1) - 1));                                 \
	(bssid)[IEEE80211_ADDR_LEN - 1] |= tmp_bssid;                                          	\
    } while(0)

#else	    

/*
 * Define the scheme that we select MAC address for multiple BSS on the same radio.
 * The very first VAP will just use the MAC address from the EEPROM.
 * For the next 3 VAPs, we set the Locally administered bit (bit 1) in MAC address,
 * and use the next bits as the index of the VAP.
 *
 * The logic used below is as follows:
 * - Default HW mac address maps to index 0
 * - Index 1 maps to default HW mac addr + 1
 * - Index 2 maps to default HW mac addr + 2 ...
 * The macros are used to generate new BSSID bits based on index and also 
 * BSSID bits are used to generate index during GET operation.
 * No of bits used depends on ATH_BCBUF value.
 * e.g for 8 vaps, 3 bits are used (bits 4,5,6 and mask of 7).
 *
 */

#define ATH_SET_VAP_BSSID_MASK(bssid_mask)      ((bssid_mask)[0] &= ~(((ATH_BCBUF-1) << 4) | 0x02))

#define ATH_GET_VAP_ID(bssid, hwbssid, id)                              \
    do {                                                                \
       u_int8_t hw_bssid = (hwbssid[0] >> 4) & (ATH_BCBUF - 1);         \
       u_int8_t tmp_bssid = (bssid[0] >> 4) & (ATH_BCBUF - 1);          \
       if (bssid[0] & 0x02)  {                                          \
           id = (((tmp_bssid + ATH_BCBUF) - hw_bssid) & (ATH_BCBUF - 1)) + 1;\
       } else {                                                         \
           id =0;                                                       \
       }                                                                \
    } while (0)
    
       
#define ATH_SET_VAP_BSSID(bssid, hwbssid, id)                        \
    do {                                                             \
        if (id) {                                                    \
            u_int8_t hw_bssid = (hwbssid[0] >> 4) & (ATH_BCBUF - 1); \
            u_int8_t tmp_bssid;                                      \
                                                                     \
            (bssid)[0] &= ~((ATH_BCBUF - 1) << 4);                   \
            tmp_bssid = (((id-1) + hw_bssid) & (ATH_BCBUF - 1));     \
            (bssid)[0] |= (((tmp_bssid) << 4) | 0x02);               \
        }                                                            \
    } while(0)
#endif

#if ATH_WOW_OFFLOAD
/* Various parameters that are likely to have changed during
 * host sleep. These need to be updated to the supplicant/WLAN 
 * driver on host wakeup */
enum {
    WOW_OFFLOAD_REPLAY_CNTR,
    WOW_OFFLOAD_KEY_TSC,
    WOW_OFFLOAD_TX_SEQNUM,
};

/* Information needed from the WLAN driver for
 * offloading GTK rekeying on embedded CPU */
struct wow_offload_misc_info {
#define WOW_NODE_QOS 0x1
    u_int32_t flags;
    u_int8_t myaddr[IEEE80211_ADDR_LEN];
    u_int8_t bssid[IEEE80211_ADDR_LEN];
    u_int16_t tx_seqnum;
    u_int16_t ucast_keyix;
#define WOW_CIPHER_NONE 0x0
#define WOW_CIPHER_AES  0x1
#define WOW_CIPHER_TKIP 0x2
#define WOW_CIPHER_WEP  0x3
    u_int32_t cipher;
    u_int64_t keytsc;
};
#endif /* ATH_WOW_OFFLOAD */

/*
 * *************************
 * Update PHY stats
 * *************************
 */
#if !ATH_SUPPORT_STATS_APONLY
#define WLAN_PHY_STATS(_phystat, _field)	_phystat->_field ++
#else
#define WLAN_PHY_STATS(_phystat, _field)	
#endif

#endif  //UMAC_LMAC_COMMON_H
