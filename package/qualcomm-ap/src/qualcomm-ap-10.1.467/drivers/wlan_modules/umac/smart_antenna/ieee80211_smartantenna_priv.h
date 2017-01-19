/*
* Copyright (c) 2011 Qualcomm Atheros, Inc..
* All Rights Reserved.
* Qualcomm Atheros Confidential and Proprietary.
*/

#ifndef _IEEE80211_SMARTANTENNA_PRIV_H_
#define _IEEE80211_SMARTANTENNA_PRIV_H_

#include <ieee80211_var.h>
#include <if_athproto.h>
#include <ath_internal.h>


#define  MAX_SMAT_PROTO_PKT_SIZE  200  /* antenna Number and Prot type */
#define SAMRTANT_SET_ANTENNA 3
#define SAMRTANT_SEND_RXSTATS 4
#define SAMRTANT_RECV_RXSTATS 5

struct custom_pkt_params{
    u_int8_t   src_mac_address[IEEE80211_ADDR_LEN];
    u_int8_t   dst_mac_address[IEEE80211_ADDR_LEN];
    u_int8_t   msg_num;
    u_int8_t   antenna;
    u_int8_t   num_data_words; 
    u_int8_t  *msg_data;
};

#define  ATH_ETH_TYPE_SMARTANTENNA_TRAINING  20
#define  ATH_ETH_TYPE_SMARTANTENNA_PROTO     21 
#endif

