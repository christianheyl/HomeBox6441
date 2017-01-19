/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef _IEEE80211_RPTPLACEMENT_H_
#define _IEEE80211_RPTPLACEMENT_H_

#ifndef UMAC_SUPPORT_RPTPLACEMENT
#define UMAC_SUPPORT_RPTPLACEMENT 0
#endif

#define  MAXNUMSTAS           10
#define  MAX_PROTO_PKT_SIZE   (MAXNUMSTAS * 3 + 2)*4

#ifndef NETLINK_RPTGPUT
#define NETLINK_RPTGPUT (NETLINK_GENERIC + 2)
#endif

struct sta_topology {
    u_int8_t  mac_address[IEEE80211_ADDR_LEN];
    u_int32_t goodput;
};

struct ieee80211_rptgput {
    u_int32_t  sta_count;
    struct sta_topology sta_top_map[MAXNUMSTAS];
    void *rptgput_sock;
    wbuf_t rptgput_wbuf;
    u_int8_t rootap_mac_address[IEEE80211_ADDR_LEN];
    u_int8_t rpt_mac_address[IEEE80211_ADDR_LEN];
    u_int8_t num_nw_stas;
    u_int8_t sta1_mac_address[IEEE80211_ADDR_LEN];
    u_int8_t sta2_mac_address[IEEE80211_ADDR_LEN]; 
    u_int8_t sta3_mac_address[IEEE80211_ADDR_LEN]; 
    u_int8_t sta4_mac_address[IEEE80211_ADDR_LEN]; 
    u_int32_t gputmode;
    u_int32_t rpt_status;
    u_int32_t rpt_assoc_status;
    u_int32_t numstas;
    u_int32_t sta1route;
    u_int32_t sta2route; 
    u_int32_t sta3route; 
    u_int32_t sta4route; 
};

typedef struct rptgput_msg {
    u_int32_t  sta_count;
    struct sta_topology sta_top_map[MAXNUMSTAS];
} RPTGPUT_MSG;

struct ieee80211_rptplacement_params {
    u_int32_t rpt_custproto_en;
    u_int32_t rpt_gputcalc_en;
    u_int32_t rpt_devup; 
    u_int32_t rpt_macdev;
    u_int32_t rpt_macaddr1;
    u_int32_t rpt_macaddr2;
    u_int32_t rpt_gputmode; 
    u_int32_t rpt_txprotomsg;
    u_int32_t rpt_rxprotomsg;
    u_int32_t rpt_status;
    u_int32_t rpt_assoc;
    u_int32_t rpt_numstas;
    u_int32_t rpt_sta1route; 
    u_int32_t rpt_sta2route;
    u_int32_t rpt_sta3route;
    u_int32_t rpt_sta4route;
};

#if UMAC_SUPPORT_RPTPLACEMENT 
typedef struct ieee80211_rptplacement_params  *ieee80211_rptplacement_params_t;
#else
typedef void *ieee80211_rptplacement_params_t;
#endif

#define MAX_RPTGPUT_PAYLOAD       (sizeof(RPTGPUT_MSG)+100) /*For netlink bcast msg*/


#if UMAC_SUPPORT_RPTPLACEMENT
int  ieee80211_rptplacement_attach(struct ieee80211com *ic);
int  ieee80211_rptplacement_detach(struct ieee80211com *ic);
void ieee80211_rptplacement_input(struct ieee80211vap *vap, wbuf_t wbuf, struct ether_header *eh);
void ieee80211_rptplacement_get_mac_addr(struct ieee80211vap *vap, u_int32_t device);
void ieee80211_rptplacement_set_mac_addr(u_int8_t *mac_address, u_int32_t word1, u_int32_t word2);
void ieee80211_rptplacement_get_status(struct ieee80211com *ic, u_int32_t word);
void ieee80211_rptplacement_get_rptassoc(struct ieee80211com *ic, u_int32_t word);
void ieee80211_rptplacement_get_gputmode(struct ieee80211com *ic, u_int32_t word);
void ieee80211_rptplacement_get_numstas(struct ieee80211com *ic, u_int32_t word);
void ieee80211_rptplacement_get_sta1route(struct ieee80211com *ic, u_int32_t word);
void ieee80211_rptplacement_get_sta2route(struct ieee80211com *ic, u_int32_t word);
void ieee80211_rptplacement_get_sta3route(struct ieee80211com *ic, u_int32_t word);
void ieee80211_rptplacement_get_sta4route(struct ieee80211com *ic, u_int32_t word);
int  ieee80211_rptplacement_gput_est_init(struct ieee80211vap *vap, int ap0_rpt1);
int  ieee80211_rptplacement_tx_proto_msg(struct ieee80211vap *vap, u_int32_t msg_num);
void ieee80211_rptplacement_set_param(wlan_if_t vaphandle, ieee80211_param param, u_int32_t val);
u_int32_t ieee80211_rptplacement_get_param(wlan_if_t vaphandle, ieee80211_param param);
#else
static inline int  ieee80211_rptplacement_attach(struct ieee80211com *ic)
{
    return 0;
}
static inline int  ieee80211_rptplacement_detach(struct ieee80211com *ic)
{
    return 0;
}
#define ieee80211_rptplacement_input(vap, wbuf, eh) /**/
#define ieee80211_rptplacement_get_mac_addr(vap, device)  /**/
#define ieee80211_rptplacement_set_mac_addr(mac_address, word1, word2) /**/
#define ieee80211_rptplacement_get_status(ic, word) /**/
#define ieee80211_rptplacement_get_rptassoc(ic, word) /**/
#define ieee80211_rptplacement_get_gputmode(ic, word) /**/
#define ieee80211_rptplacement_get_numstas(ic, word) /**/
#define ieee80211_rptplacement_get_sta1route(ic, word) /**/
#define ieee80211_rptplacement_get_sta2route(ic, word) /**/
#define ieee80211_rptplacement_get_sta3route(ic, word) /**/
#define ieee80211_rptplacement_get_sta4route(ic, word) /**/
#define ieee80211_rptplacement_gput_est_init(vap, ap0_rpt1) 0 /**/
#define ieee80211_rptplacement_tx_proto_msg(vap, msg_num) 0 /**/
#define ieee80211_rptplacement_set_param(vaphandle, param, val) /**/
#define ieee80211_rptplacement_get_param(vaphandle, param) 0 /**/
#endif

#endif


