#ifndef __IFX_PPA_API_SESSION_H__20081104_1139__
#define __IFX_PPA_API_SESSION_H__20081104_1139__



/*******************************************************************************
**
** FILE NAME    : ifx_ppa_api_session.h
** PROJECT      : PPA
** MODULES      : PPA API (Routing/Bridging Acceleration APIs)
**
** DATE         : 4 NOV 2008
** AUTHOR       : Xu Liang
** DESCRIPTION  : PPA Protocol Stack Hook API Session Operation Functions Header
**                File
** COPYRIGHT    :              Copyright (c) 2009
**                          Lantiq Deutschland GmbH
**                   Am Campeon 3; 85579 Neubiberg, Germany
**
**   For licensing information, see the file 'LICENSE' in the root folder of
**   this software module.
**
** HISTORY
** $Date        $Author         $Comment
** 04 NOV 2008  Xu Liang        Initiate Version
*******************************************************************************/
/*! \file ifx_ppa_api_session.h
    \brief This file contains all ppa routing session api.
*/

/** \defgroup PPA_SESSION_API PPA Session API
    \brief  PPA Session API provide API to get/set/delete/modify all ppa routing session
            - ifx_ppa_api_session.h: Header file for PPA PROC API
            - ifx_ppa_api_session.c: C Implementation file for PPA API
*/
/* @{ */ 


/*
 * ####################################
 *              Definition
 * ####################################
 */
 /*
 *  default settings
 */
#define DEFAULT_TIMEOUT_IN_SEC                  3600    //  1 hour
#define DEFAULT_BRIDGING_TIMEOUT_IN_SEC         60      //  1 minute
#define DEFAULT_MTU                             1500    //  IP frame size (including IP header)
#define DEFAULT_CH_ID                           0
#define DEFAULT_HIT_POLLING_TIME                3       //  change to 3 seconds from original 1 minute for MIB Poll requirement
#define DEFAULT_MIB_POLLING_TIME                3       //  3 seconds
#define DEFAULT_BRIDGING_HIT_POLLING_TIME       2       //  2 seconds


#define PPA_IS_NAT_SESSION(flags)               ((flags) & (SESSION_VALID_NAT_IP | SESSION_VALID_NAT_PORT))

#if defined(ENABLE_SESSION_DEBUG_FLAGS) && ENABLE_SESSION_DEBUG_FLAGS
  #define SESSION_DBG_NOT_REACH_MIN_HITS        0x00000001
  #define SESSION_DBG_ALG                       0x00000002
  #define SESSION_DBG_ZERO_DST_MAC              0x00000004
  #define SESSION_DBG_TCP_NOT_ESTABLISHED       0x00000008
  #define SESSION_DBG_RX_IF_NOT_IN_IF_LIST      0x00000010
  #define SESSION_DBG_TX_IF_NOT_IN_IF_LIST      0x00000020
  #define SESSION_DBG_RX_IF_UPDATE_FAIL         0x00000040
  #define SESSION_DBG_TX_IF_UPDATE_FAIL         0x00000080
  #define SESSION_DBG_SRC_BRG_IF_NOT_IN_BRG_TBL 0x00000100
  #define SESSION_DBG_SRC_IF_NOT_IN_IF_LIST     0x00000200
  #define SESSION_DBG_DST_BRG_IF_NOT_IN_BRG_TBL 0x00000400
  #define SESSION_DBG_DST_IF_NOT_IN_IF_LIST     0x00000800
  #define SESSION_DBG_ADD_PPPOE_ENTRY_FAIL      0x00001000
  #define SESSION_DBG_ADD_MTU_ENTRY_FAIL        0x00002000
  #define SESSION_DBG_ADD_MAC_ENTRY_FAIL        0x00004000
  #define SESSION_DBG_ADD_OUT_VLAN_ENTRY_FAIL   0x00008000
  #define SESSION_DBG_RX_PPPOE                  0x00010000
  #define SESSION_DBG_TX_PPPOE                  0x00020000
  #define SESSION_DBG_TX_BR2684_EOA             0x00040000
  #define SESSION_DBG_TX_BR2684_IPOA            0x00080000
  #define SESSION_DBG_TX_PPPOA                  0x00100000
  #define SESSION_DBG_GET_DST_MAC_FAIL          0x00200000
  #define SESSION_DBG_RX_INNER_VLAN             0x00400000
  #define SESSION_DBG_RX_OUTER_VLAN             0x00800000
  #define SESSION_DBG_TX_INNER_VLAN             0x01000000
  #define SESSION_DBG_TX_OUTER_VLAN             0x02000000
  #define SESSION_DBG_RX_VLAN_CANT_SUPPORT      0x04000000
  #define SESSION_DBG_TX_VLAN_CANT_SUPPORT      0x08000000
  #define SESSION_DBG_UPDATE_HASH_FAIL          0x10000000
  #define SESSION_DBG_PPE_EDIT_LIMIT            0x20000000
#if defined(L2TP_CONFIG) && L2TP_CONFIG
  #define SESSION_DBG_TX_PPPOL2TP               0x40000000
  #define SESSION_DBG_RX_PPPOL2TP               0x80000000
#endif
  #define SESSION_DBG_SW_ACC_HEADER             0x40000000


  #define SET_DBG_FLAG(pitem, flag)             ((pitem)->debug_flags |= (flag))
  #define CLR_DBG_FLAG(pitem, flag)             ((pitem)->debug_flags &= ~(flag))
#else
  #define SET_DBG_FLAG(pitem, flag)             do { } while ( 0 )
  #define CLR_DBG_FLAG(pitem, flag)             do { } while ( 0 )
#endif


#define SKB_PRIORITY_DEBUG 1

#define PSEUDO_MC_ANY_SRC  						~0

#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG

#define  ETHER_TYPE                             0x0800
#define  IP_VERSION                             0x4
#define  IP_HEADER_LEN                          0x5
#define  IP_TOTAL_LEN                           0x0
#define  IP_IDENTIFIER                          0x0
#define  IP_FLAG                                0x0
#define  IP_FRAG_OFF                            0x0
#define  IP_PROTO_UDP                           0x11 
#define  UDP_TOTAL_LENGTH                       0x0
#define  IP_PSEUDO_UDP_LENGTH                   0x0 //???

#endif






/*
 * ####################################
 *              Data Type
 * ####################################
 */
struct session_list_item {

    PPA_HLIST_NODE              hlist;
    PPA_SESSION                *session;
    //PPA_LOCK                    uc_lock;
    //PPA_SESSION_EXTRA           session_extra;
    //uint16_t                    ip_proto;
    uint8_t                     ip_proto;
    //uint16_t                    ip_tos;
    uint8_t                     ip_tos;
    PPA_IPADDR					src_ip;
    uint16_t                    src_port;
    uint8_t                     src_mac[PPA_ETH_ALEN];
    PPA_IPADDR                  dst_ip;
    uint16_t                    dst_port;
    uint8_t                     dst_mac[PPA_ETH_ALEN];
    PPA_IPADDR					nat_ip;         //  IP address to be replaced by NAT if NAT applies
    uint16_t                    nat_port;       //  Port to be replaced by NAT if NAT applies
    uint16_t                    num_adds;       //  Number of attempts to add session
    PPA_NETIF                  *rx_if;
    PPA_NETIF                  *tx_if;
    uint32_t                    timeout;
    uint32_t                    last_hit_time;  //  Updated by bookkeeping thread
    uint32_t                    sixrd_daddr;
    //uint32_t                    new_dscp;
    uint16_t                    new_dscp;
    uint16_t                    pppoe_session_id;
#if defined(L2TP_CONFIG) && L2TP_CONFIG
    uint16_t                    pppol2tp_session_id;
    uint16_t                    pppol2tp_tunnel_id;
#endif
    uint16_t                    new_vci;
    uint32_t                    out_vlan_tag;
    uint32_t                    mtu;
    uint16_t                    dslwan_qid;
    uint16_t                    dest_ifid;

    uint32_t                    flags;          //  Internal flag : SESSION_IS_REPLY, SESSION_IS_TCP,
                                                //                  SESSION_ADDED_IN_HW, SESSION_NOT_ACCEL_FOR_MGM
                                                //                  SESSION_VALID_NAT_IP, SESSION_VALID_NAT_PORT,
                                                //                  SESSION_VALID_VLAN_INS, SESSION_VALID_VLAN_RM,
                                                //                  SESSION_VALID_OUT_VLAN_INS, SESSION_VALID_OUT_VLAN_RM,
                                                //                  SESSION_VALID_PPPOE, SESSION_VALID_NEW_SRC_MAC,
                                                //                  SESSION_VALID_MTU, SESSION_VALID_NEW_DSCP,
                                                //                  SESSION_VALID_DSLWAN_QID,
                                                //                  SESSION_TX_ITF_IPOA, SESSION_TX_ITF_PPPOA
                                                //                  SESSION_LAN_ENTRY, SESSION_WAN_ENTRY, SESSION_IS_IPV6
#if defined(CONFIG_LTQ_PPA_API_SW_FASTPATH)
    void			*sah;	
#endif
#if defined(ENABLE_SESSION_DEBUG_FLAGS) && ENABLE_SESSION_DEBUG_FLAGS
    uint32_t                    debug_flags;
#endif

    uint32_t                    routing_entry;
    uint32_t                    pppoe_entry;
    uint32_t                    mtu_entry;
    uint32_t                    src_mac_entry;
    uint32_t                    out_vlan_entry;
#if defined(SKB_PRIORITY_DEBUG) && SKB_PRIORITY_DEBUG
    uint32_t                    priority;    //skb priority. for debugging only
    uint32_t                    mark;    //skb mark value. for debugging only
#endif
    uint64_t                    mips_bytes; //bytes handled by mips
    uint64_t                    acc_bytes; //bytes handled by PPE acceleration
    uint32_t                    last_bytes; //last updated bytes handled by PPE acceleration
    uint64_t                    prev_sess_bytes; //last updated bytes. This is for PPA session management purpose
    uint64_t                    prev_clear_acc_bytes;  //last cleared bytes. We don't really clear session's acceleration mib in PPE FW, but instead, we just record the mib counter for adjustment later
    uint64_t                    prev_clear_mips_bytes;  //last cleared bytes. We don't really clear session's acceleration mib in PPE FW, but instead, we just record the mib counter for adjustment later
    //uint32_t                    tunnel_idx; //tunnel idx for PPE action table
    uint8_t                    tunnel_idx; //tunnel idx for PPE action table
#if defined(L2TP_CONFIG) && L2TP_CONFIG
    uint32_t                    l2tptunnel_idx; //l2tptunnel idx for PPE action table
#endif
    uint8_t                     collision_flag; // 1 mean the entry is in collsion table or no hashed table, like ASE/Danube

#if defined(CONFIG_IFX_PPA_IF_MIB) && CONFIG_IFX_PPA_IF_MIB
    PPA_NETIF                   *br_tx_if;  //record its subinterface name under bridge interface
    PPA_NETIF                   *br_rx_if;  //record its subinterface name under bridge interface
#endif    

#if defined(SESSION_STATISTIC_DEBUG) && SESSION_STATISTIC_DEBUG 
    /*below variable is used for session management debugging purpose */
    uint32_t                     flag2; //SESSION_FLAG2_HASH_INDEX_DONE/SESSION_FLAG2_ADD_HW_FAIL 
    uint16_t                     hash_index;
    uint16_t                     hash_table_id; /* 0-first hash table, 1 WAN */
    uint16_t                     src_ip6_index;  /* Note, 0 means not valid data. so for its correct value, it should be "real index + 1 "  */
    uint16_t                     src_ip6_psuedo_ip; 
    uint16_t                     dst_ip6_index;  /* Note, 0 means not valid data. so for its correct value, it should be "real index + 1 " */
    uint16_t                     dst_ip6_psuedo_ip;    
#endif   

};

struct mc_group_list_item {
    PPA_HLIST_NODE              mc_hlist;

    IP_ADDR_C                   ip_mc_group;
    uint32_t                    num_ifs;
    PPA_NETIF                  *netif[PPA_MAX_MC_IFS_NUM]; //the interface list where mc traffic will be delivered to
    uint32_t                    ttl[PPA_MAX_MC_IFS_NUM];
    uint32_t                    if_mask;
    PPA_NETIF                  *src_netif;  //the interface which received the mc streaming traffic 
    uint16_t                    new_dscp;
    uint16_t                    new_vci;
    uint32_t                    out_vlan_tag;
    uint16_t                    dslwan_qid;
    uint16_t                    dest_ifid;

    uint32_t                    flags;          //  Internal flag : SESSION_IS_REPLY,
                                                //                  SESSION_ADDED_IN_HW, SESSION_NOT_ACCEL_FOR_MGM
                                                //                  SESSION_VALID_NAT_IP, SESSION_VALID_NAT_PORT,
                                                //                  SESSION_VALID_VLAN_INS, SESSION_VALID_VLAN_RM,
                                                //                  SESSION_VALID_OUT_VLAN_INS, SESSION_VALID_OUT_VLAN_RM,
                                                //                  SESSION_VALID_PPPOE, SESSION_VALID_SRC_MAC,
                                                //                  SESSION_VALID_MTU, SESSION_VALID_NEW_DSCP,
                                                //                  SESSION_VALID_DSLWAN_QID,
                                                //                  SESSION_TX_ITF_IPOA, SESSION_TX_ITF_PPPOA
                                                //                  SESSION_LAN_ENTRY, SESSION_WAN_ENTRY
#if defined(ENABLE_SESSION_DEBUG_FLAGS) && ENABLE_SESSION_DEBUG_FLAGS
    uint32_t                    debug_flags;
#endif

    uint32_t                    mc_entry;
    uint32_t                    src_mac_entry;
    uint32_t                    out_vlan_entry;
	uint32_t					dst_ipv6_entry;
	uint32_t					src_ipv6_entry;
    uint32_t                    bridging_flag;  //  sgh add: 0 - routing mode/igmp proxy, 1 - bring mode/igmp snooping.
    uint8_t                     SSM_flag;   /*!< Set the flag if source specific forwarding is required default 0*/ 
#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
    uint8_t                     RTP_flag;   /*!< rtp flag */
    uint32_t                    rtp_pkt_cnt;  /*!< RTP packet mib */
    uint32_t                    rtp_seq_num;  /*!< RTP sequence number */
#endif

    IP_ADDR_C            		source_ip;    /*!<  source ip address */
    uint32_t                    last_hit_time;  //  Updated by timer
    uint64_t                    acc_bytes; //bytes handled by PPE acceleration
    uint64_t                    prev_clear_acc_bytes; //
    uint32_t                    last_bytes; //last updated bytes handled by PPE acceleration
};

#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
struct capwap_group_list_item {
    PPA_LIST_NODE               capwap_list;

    uint32_t                    num_ifs;
    PPA_NETIF                  *netif[PPA_MAX_CAPWAP_IFS_NUM]; //destination interface list for the upstream
    uint32_t                    if_mask;
    uint8_t                     phy_port_id[PPA_MAX_CAPWAP_IFS_NUM]; /*!< physical port Id for upstream */

    uint8_t                     directpath_portid;  
    uint8_t                     qid; /*!< PPE FW QoS Queue Id */

    uint8_t                     dst_mac[PPA_ETH_ALEN]; /*!< destination mac address */
    uint8_t                     src_mac[PPA_ETH_ALEN]; /*!< source mac address */
   
    uint8_t                     tos; /*IPV4 header TOS */
    uint8_t                     ttl; /*IPV4 header TTL */
    IP_ADDR_C                   source_ip;  /*!< source ip */
    IP_ADDR_C                   dest_ip;    /*!< destination ip */
    
    uint16_t                     source_port; /* UDP source port */
    uint16_t                     dest_port; /* UDP destination port */
    
    uint8_t                     rid; /* CAPWAP RID */
    uint8_t                     wbid; /* CAPWAP WBID */
    uint8_t                     t_flag; /* CAPWAP T */
    uint32_t                    max_frg_size; /*!< Maximum Fragment size */

    uint32_t                    p_entry;

};


struct capwap_iphdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
               uint8_t  ihl:4,
              version:4;
#elif defined (__BIG_ENDIAN_BITFIELD)
               uint8_t  version:4,
                  ihl:4;
#endif
               uint8_t  tos;
               uint16_t tot_len;
               uint16_t id;
               uint16_t frag_off;
               uint8_t  ttl;
               uint8_t  protocol;
               uint16_t check;
               uint32_t saddr;
               uint32_t daddr;
}; 

struct         udp_ipv4_psedu_hdr {
               uint32_t saddr;
               uint32_t daddr;
               uint8_t  rsvd;
               uint8_t  protocol;
               uint16_t udp_length;
               uint16_t src_port;
               uint16_t dst_port;
               uint16_t length;
               uint16_t checksum;
}; 

#endif
#if defined(L2TP_CONFIG) && L2TP_CONFIG
struct         l2tp_udp_hdr {
               uint32_t saddr;
               uint32_t daddr;
//               uint8_t  rsvd;
//               uint16_t udp_length;
               uint16_t src_port;
               uint16_t dst_port;
               uint16_t  protocol;
//               uint16_t length;
//               uint16_t checksum;
};

struct l2tp_ip_hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
        __u8    ihl:4,
                version:4;
#elif defined (__BIG_ENDIAN_BITFIELD)
        __u8    version:4,
                ihl:4;
#else
#error  "Please fix <asm/byteorder.h>"
#endif
        __u8    tos;
        __be16  tot_len;
        __be16  id;
        __be16  frag_off;
        __u8    ttl;
        __u8    protocol;
        __sum16 check;
        __be32  saddr;
        __be32  daddr;
        /*The options start here. */
};
#endif



struct bridging_session_list_item {
    PPA_HLIST_NODE              br_hlist;

    uint8_t                     mac[PPA_ETH_ALEN];
    PPA_NETIF                  *netif;
    uint16_t                    vci;
    uint16_t                    new_vci;
    uint32_t                    timeout;
    uint32_t                    last_hit_time;  //  Updated by bookkeeping thread
    uint16_t                    dslwan_qid;
    uint16_t                    dest_ifid;

    uint32_t                    flags;          //  Internal flag : 
                                                //                  SESSION_ADDED_IN_HW, SESSION_NOT_ACCEL_FOR_MGM
                                                //                  SESSION_STATIC, SESSION_DROP
                                                //                  SESSION_VALID_VLAN_INS, SESSION_VALID_VLAN_RM
                                                //                  SESSION_SRC_MAC_DROP_EN
#if defined(ENABLE_SESSION_DEBUG_FLAGS) && ENABLE_SESSION_DEBUG_FLAGS
    uint32_t                    debug_flags;
#endif

    uint32_t                    bridging_entry;
};



/*
 * ####################################
 *             Declaration
 * ####################################
 */

/*
 *  Operation Functions
 */

//  routing session operation
int32_t ppa_lookup_session(PPA_SESSION *, uint32_t, struct session_list_item **);
int32_t __ppa_lookup_session(PPA_SESSION *, uint32_t, struct session_list_item **);
int32_t ppa_add_session(PPA_BUF *, PPA_SESSION *, uint32_t);
int32_t ppa_update_session(PPA_BUF *, struct session_list_item *, uint32_t);
int32_t ppa_update_session_extra(PPA_SESSION_EXTRA *, struct session_list_item *, uint32_t);
int32_t ifx_ppa_session_delete(PPA_SESSION *, uint32_t);
int32_t ifx_ppa_speed_handle_frame(PPA_BUF *, PPA_SESSION *, uint32_t);
int32_t ifx_ppa_update_session(PPA_BUF *, PPA_SESSION *, uint32_t);
void dump_list_item(struct session_list_item *, char *);
void ppa_uc_get_item_lock(struct session_list_item *);
void ppa_uc_release_item_lock(struct session_list_item *);
uint32_t ppa_get_routing_session_count(uint16_t, uint32_t, uint32_t);


int32_t ppa_session_start_iteration(uint32_t *, struct session_list_item **);
int32_t ppa_session_iterate_next(uint32_t *, struct session_list_item **);
void ppa_session_stop_iteration(void);
int  ppa_get_hw_session_cnt(void);

//  routing session hardware/firmware operation
int32_t ppa_hw_add_session(struct session_list_item *, uint32_t);
int32_t ppa_hw_update_session_extra(struct session_list_item *, uint32_t);
void ppa_hw_del_session(struct session_list_item *);

//  multicast routing operation
int32_t __ppa_lookup_mc_group(IP_ADDR_C *, IP_ADDR_C *, struct mc_group_list_item **);
int32_t ppa_mc_group_setup(PPA_MC_GROUP *p_mc_group, struct mc_group_list_item *p_item, uint32_t flags);
int32_t ppa_mc_group_checking(PPA_MC_GROUP *, struct mc_group_list_item *, uint32_t);
int32_t ppa_add_mc_group(PPA_MC_GROUP *, struct mc_group_list_item **, uint32_t);
int32_t ppa_update_mc_group_extra(PPA_SESSION_EXTRA *, struct mc_group_list_item *, uint32_t);
void ppa_remove_mc_group(struct mc_group_list_item *);
void ppa_delete_mc_group(PPA_MC_GROUP *ppa_mc_entry);


int32_t ppa_mc_group_start_iteration(uint32_t *, struct mc_group_list_item **);
int32_t ppa_mc_group_iterate_next(uint32_t *, struct mc_group_list_item **);
void ppa_mc_group_stop_iteration(void);
void ppa_mc_get_htable_lock(void);
void ppa_mc_release_htable_lock(void);
uint32_t ppa_get_mc_group_count(uint32_t);


//  multicast routing hardware/firmware operation
int32_t ppa_hw_add_mc_group(struct mc_group_list_item *);
int32_t ppa_hw_update_mc_group_extra(struct mc_group_list_item *, uint32_t);
#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
int32_t ppa_hw_set_mc_rtp(struct mc_group_list_item *);
int32_t ppa_hw_get_mc_rtp_sampling_cnt(struct mc_group_list_item *);
#endif
void ppa_hw_del_mc_group(struct mc_group_list_item *);
int32_t ppa_update_mc_hw_group(struct mc_group_list_item *);
int32_t __ppa_mc_group_update(PPA_MC_GROUP *, struct mc_group_list_item *, uint32_t );

#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
int32_t __ppa_lookup_capwap_group(IP_ADDR_C *, IP_ADDR_C *,uint8_t, struct capwap_group_list_item **);
int32_t __ppa_lookup_capwap_group_add(IP_ADDR_C *, IP_ADDR_C *,uint8_t, struct capwap_group_list_item **);

int32_t __ppa_capwap_group_update(PPA_CMD_CAPWAP_INFO *, struct capwap_group_list_item *);
int32_t ppa_add_capwap_group(PPA_CMD_CAPWAP_INFO*, struct capwap_group_list_item** );
void ppa_remove_capwap_group(struct capwap_group_list_item*);

void ppa_capwap_group_stop_iteration(void);
void ppa_capwap_release_table_lock(void);
void ppa_capwap_get_table_lock(void);
void ppa_free_capwap_group_list_item(struct capwap_group_list_item *);
int32_t ppa_capwap_start_iteration(uint32_t , struct capwap_group_list_item **);
uint32_t ppa_get_capwap_count(void);
#endif

//  routing polling timer
void ppa_set_polling_timer(uint32_t, uint32_t);

// ip test functions
uint32_t is_ip_zero(IP_ADDR_C *);
uint32_t ip_equal(IP_ADDR_C *, IP_ADDR_C *);
unsigned int is_ip_allbit1(IP_ADDR_C *ip);


//  bridging session operation
int32_t __ppa_bridging_lookup_session(uint8_t *, PPA_NETIF *, struct bridging_session_list_item **);
int32_t ppa_bridging_add_session(uint8_t *, PPA_NETIF *, struct bridging_session_list_item **, uint32_t);
void ppa_bridging_remove_session(struct bridging_session_list_item *);
void dump_bridging_list_item(struct bridging_session_list_item *, char *);

int32_t ppa_bridging_session_start_iteration(uint32_t *, struct bridging_session_list_item **);
int32_t ppa_bridging_session_iterate_next(uint32_t *, struct bridging_session_list_item **);
void ppa_bridging_session_stop_iteration(void);
uint32_t ppa_get_br_count (uint32_t);

void ppa_br_get_htable_lock(void);
void ppa_br_release_htable_lock(void);

//  bridging session hardware/firmware operation
int32_t ppa_bridging_hw_add_session(struct bridging_session_list_item *);
void ppa_bridging_hw_del_session(struct bridging_session_list_item *);

//  bridging polling timer
void ppa_bridging_set_polling_timer(uint32_t);

//  special function;
void ppa_remove_sessions_on_netif(PPA_IFNAME *);
void ppa_free_session_list_item(struct session_list_item *p_item);
/*
 *  Init/Uninit Functions
 */
int32_t ppa_api_session_manager_init(void);
void ppa_api_session_manager_exit(void);
int32_t ppa_api_session_manager_create(void);
void ppa_api_session_manager_destroy(void);

struct session_list_item *ppa_alloc_session_list_item(void);
void __ppa_insert_session_item(struct session_list_item *);
uint32_t ppa_api_get_session_poll_timer(void);
uint32_t ppa_api_set_test_qos_priority_via_mark(PPA_BUF *ppa_buf);
uint32_t ppa_api_set_test_qos_priority_via_tos(PPA_BUF *ppa_buf);
void ppa_check_hit_stat_clear_mib(int32_t flag);

/* @} */

void ppa_init_mc_group_list_item(struct mc_group_list_item *);
int32_t get_ppa_hash_count(PPA_CMD_COUNT_INFO *count_info);
uint32_t get_ppa_sesssion_num_in_one_hash(uint32_t i);
int32_t ppa_session_get_items_via_index(uint32_t index, struct session_list_item **pp_item, uint32_t max_len, uint32_t *copy_len, uint32_t flag);
void ppa_uc_get_htable_lock(void);
void ppa_uc_release_htable_lock(void);
extern uint32_t g_hit_polling_time;


#endif  //  __IFX_PPA_API_SESSION_H__20081104_1139__
