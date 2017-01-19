#ifndef _HTC_IEEE_COMMON_H
#define _HTC_IEEE_COMMON_H

/* Please make sure the size of ALL headers is on word alignment */
struct ieee80211com_target {
    a_uint32_t    ic_flags;       
    a_uint32_t    ic_flags_ext;         /* extension of state flags */
    a_uint32_t    ic_ampdu_limit ;      /* A-AMPDU length limit*/
    a_uint8_t     ic_ampdu_subframes;   /* A AMPDU subfrmae limit */
    a_uint8_t 	  ic_tx_chainmask;
    a_uint8_t     ic_tx_chainmask_legacy;
    a_uint8_t	  ic_rtscts_ratecode;		 
    a_uint8_t	  ic_protmode;		 
};

struct ieee80211vap_target
{
    a_uint8_t               iv_vapindex;
    a_uint8_t               iv_des_bssid[IEEE80211_ADDR_LEN];
    enum ieee80211_opmode   iv_opmode;             
    a_uint8_t               iv_myaddr[IEEE80211_ADDR_LEN];
    a_uint8_t               iv_ni_bssid[IEEE80211_ADDR_LEN];
    a_uint32_t              iv_create_flags; /* vap create flags */
    a_uint32_t              iv_flags;                   
    a_uint32_t              iv_flags_ext;              
    a_uint16_t              iv_ps_sta;               
    a_uint16_t              iv_rtsthreshold;
    a_uint8_t               iv_ath_cap;
    a_uint8_t               iv_node ;
    a_int8_t                iv_mcast_rate;
    a_uint8_t               iv_nodeindex;
    struct ieee80211_node_target *iv_bss;
};

#if ENCAP_OFFLOAD
struct ieee80211vap_update_tgt {
    a_uint32_t      iv_vapindex;
    a_uint32_t      iv_flags;
    a_uint32_t      iv_flags_ext;
    a_uint16_t      iv_rtsthreshold;
};
#endif

#ifdef MAGPIE_UAPSD
struct ieee80211_node_tgt_uapsd
{
    a_uint16_t ni_uapsd_trigseq[WME_NUM_AC]; /* trigger suppression on retry */
};
#endif

struct ieee80211_key_target {
    a_int32_t dummy ;
};

struct ieee80211_node_target
{
    uint16_t        ni_associd;	/* assoc response */
    uint16_t        ni_txpower;	/* current transmit power */
    struct ieee80211_key_target	ni_ucastkey;	/* unicast key */
    a_uint8_t       ni_macaddr[IEEE80211_ADDR_LEN];
    a_uint8_t       ni_bssid[IEEE80211_ADDR_LEN];
    a_uint8_t       ni_nodeindex;
    a_uint8_t       ni_vapindex;
    a_uint8_t       ni_vapnode;
    a_uint32_t      ni_flags;	/* special-purpose state */
		/* 11n Capabilities */
    a_uint16_t 		ni_htcap;	/* HT capabilities */
    a_uint8_t       ni_valid ;
    a_uint8_t       ni_kickoutlimit;
    a_uint16_t      ni_capinfo;	
    //struct ieee80211com_target  *ni_ic;
    //struct ieee80211vap_target	*ni_vap;
    a_uint32_t      ni_ic;
    a_uint32_t	    ni_vap;
    a_uint16_t      ni_txseqmgmt;    
    a_uint8_t       ni_is_vapnode;	 
    a_uint16_t      ni_maxampdu;	/* maximum rx A-MPDU length */
    a_uint16_t      ni_iv16;
    a_uint32_t      ni_iv32;
#ifdef ENCAP_OFFLOAD
    a_uint64_t      ni_ucast_keytsc;                  /* unicast key transmit sequence counter */
    a_uint64_t      ni_mcast_keytsc;                  /* multicast key transmit sequence counter */
#endif    
#ifdef MAGPIE_UAPSD
    a_uint8_t                    ni_uapsd;  /* U-APSD per-node WMM STA Qos Info field */
    a_uint8_t ni_uapsd_maxsp; /* maxsp from flags above */
    struct ieee80211_node_tgt_uapsd ni_uapsd_pvt;
#endif
    a_uint32_t    dummy;
    a_uint32_t    dummy2;
#ifdef MAGPIE_HIF_GMAC
    a_uint32_t    ni_ratekbps;
#endif        
};


#endif 
