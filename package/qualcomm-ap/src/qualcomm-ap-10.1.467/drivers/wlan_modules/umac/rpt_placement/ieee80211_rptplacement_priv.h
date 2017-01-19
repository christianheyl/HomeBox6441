#ifndef _IEEE80211_RPTPLACEMENT_PRIV_H_
#define _IEEE80211_RPTPLACEMENT_PRIV_H_

#include <ieee80211_var.h>
#include <if_athproto.h>
#include <ath_internal.h>


#define  MAXNUMSTAS           10
#define  MAX_PROTO_PKT_SIZE   (MAXNUMSTAS * 3 + 2)*4

#ifndef NETLINK_RPTGPUT
#define NETLINK_RPTGPUT (NETLINK_GENERIC + 2)
#endif

struct custom_pkt_params{
    u_int8_t   src_mac_address[IEEE80211_ADDR_LEN];
    u_int8_t   dst_mac_address[IEEE80211_ADDR_LEN];
    u_int8_t   msg_num;
    u_int8_t   num_data_words; 
    u_int32_t  *msg_data;
};

enum custom_proto_pkt_type{
    AP_RPT_CM1   = 1,  /* MSG #1 from AP to RPT to change mode               */
    RPT_AP_ACK1  = 2,  /* MSG #2 from RPT to AP to ack MSG #1                */
    RPT_AP_GPT   = 3,  /* MSG #3 is Goodput table from RPT to AP             */
    AP_RPT_ACK2  = 4,  /* MSG #4 from AP to RPT to ack MSG #3                */ 
    RPT_AP_CM2   = 5,  /* MSG #5 from RPT to AP to change mode               */
    AP_RPT_ASSOC = 6,  /* MSG #6 is ASSOC status from AP to RPT              */
    AP_STA_ROUTE = 7,  /* MSG #7 is routing table from AP to STAs            */ 
    RPT_AP_ACK3  = 8,  /* MSG #8 from RPT to AP to ack MSG #6 or MSG #7      */
    AP_RPT_DONE  = 9  /* MSG #9 from AP to RPT to indicate end of training  */ 
};

enum ath_eth_rptplacement_proto_type{
    ATH_ETH_TYPE_RPT_TRAINING = 18,   /* Used by rpt placement training pkts */
    ATH_ETH_TYPE_RPT_MSG      = 19    /* Used by rpt placement custom protocol pkts */
};

enum rptplacement_dev_type{           /*Used to set the MAC addresses of devices */
    STA1   = 1,   /* Station # 1 */
    STA2   = 2,   /* Station # 2 */ 
    STA3   = 3,   /* Station # 3 */
    STA4   = 4,   /* Station # 4 */
    ROOTAP = 5,   /* Station # 5 */
    RPT    = 6    /* Station # 6 */
};

#define RPT_TRAINING_PAYLOAD_SIZE 1500
#define RPT_NUM_AVGS               128
#define RPT_NUM_PKTS_PER_AVG      1000
#define RPT_GPUT_THRESH           RPT_NUM_AVGS*50           /* 50Mbps per average */         

/* Custom protocol packet wbuf offsets */
#define RPT_BUF_MSG_NUM1   4
#define RPT_BUF_MSG_NUM2   5
#define RPT_BUF_MSG_NUM3   6
#define RPT_BUF_MSG_NUM4   7
#define RPT_BUF_STA_CNT1   8
#define RPT_BUF_STA_CNT2   9
#define RPT_BUF_STA_CNT3   10
#define RPT_BUF_STA_CNT4   11
#define RPT_BUF_STATUS1    12
#define RPT_BUF_STATUS2    13
#define RPT_BUF_ASSOC1     14
#define RPT_BUF_ASSOC2     15
#define RPT_BUF_MAC1_0     12
#define RPT_BUF_MAC1_1     13
#define RPT_BUF_MAC1_2     14
#define RPT_BUF_MAC1_3     15
#define RPT_BUF_MAC1_4     18
#define RPT_BUF_MAC1_5     19
#define RPT_BUF_GPUT1      20
#define RPT_BUF_GPUT2      21
#define RPT_BUF_GPUT3      22
#define RPT_BUF_GPUT4      23

int  ieee80211_rptplacement_tx_custom_data_pkt(struct ieee80211vap *vap, 
                                               struct custom_pkt_params *cpp);
int  ieee80211_rptplacement_tx_custom_proto_pkt(struct ieee80211vap *vap, 
                                                struct custom_pkt_params *cpp);
u_int32_t ieee80211_rptplacement_gput_calc(struct ieee80211vap *vap,  
                                           struct ieee80211_node_table *nt, 
                                           u_int8_t vap_addr[][IEEE80211_ADDR_LEN],
                                           u_int32_t sta_count, u_int32_t ap0_rpt1);
void ieee80211_rptplacement_display_goodput_table(struct ieee80211com *ic);
void ieee80211_rptplacement_rx_proto_msg(struct ieee80211vap *vap, wbuf_t mywbuf);
int  ieee80211_rptplacement_rx_gput_table(struct ieee80211vap *vap, wbuf_t mywbuf);
int  ieee80211_rptplacement_rx_assoc_status(struct ieee80211vap *vap, wbuf_t mywbuf);
int  ieee80211_rptplacement_rx_routingtable(struct ieee80211vap *vap, wbuf_t mywbuf);
void ieee80211_rptplacement_create_msg(struct ieee80211com *ic);
void ieee80211_rptplacement_prep_wbuf(struct ieee80211com *ic);
void ieee80211_rptplacement_bcast_msg(struct ieee80211com *ic);
int  ieee80211_rptplacement_init_netlink(struct ieee80211com *ic);
int  ieee80211_rptplacement_delete_netlink(struct ieee80211com *ic);
#endif

