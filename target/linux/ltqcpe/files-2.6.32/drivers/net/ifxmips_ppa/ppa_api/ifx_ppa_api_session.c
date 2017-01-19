/*******************************************************************************
**
** FILE NAME    : ifx_ppa_api_session.c
** PROJECT      : PPA
** MODULES      : PPA API (Routing/Bridging Acceleration APIs)
**
** DATE         : 4 NOV 2008
** AUTHOR       : Xu Liang
** DESCRIPTION  : PPA Protocol Stack Hook API Session Operation Functions
** COPYRIGHT    :              Copyright (c) 2009
**                          Lantiq Deutschland GmbH
**                   Am Campeon 3; 85579 Neubiberg, Germany
**
**   For licensing information, see the file 'LICENSE' in the root folder of
**   this software module.
**
** HISTORY
** $Date        $Author                $Comment
** 04 NOV 2008  Xu Liang               Initiate Version
** 10 DEC 2012  Manamohan Shetty       Added the support for RTP,MIB mode and CAPWAP 
**                                     Features 
*******************************************************************************/



/*
 * ####################################
 *              Head File
 * ####################################
 */

/*
 *  Common Head File
 */
#include <linux/autoconf.h>
//#include <linux/kernel.h>
//#include <linux/module.h>
//#include <linux/version.h>
//#include <linux/types.h>
//#include <linux/init.h>
//#include <linux/slab.h>
//#if defined(CONFIG_IFX_PPA_API_PROC)
//#include <linux/proc_fs.h>
//#endif
//#include <linux/netdevice.h>
//#include <linux/in.h>
//#include <net/sock.h>
//#include <net/ip_vs.h>
//#include <asm/time.h>

/*
 *  PPA Specific Head File
 */
#include <net/ifx_ppa_api.h>
#include <net/ifx_ppa_ppe_hal.h>
#include "ifx_ppa_api_misc.h"
#include "ifx_ppa_api_netif.h"
#include "ifx_ppa_api_session.h"
#include "ifx_ppa_api_mib.h"
#include "ifx_ppe_drv_wrapper.h"
#include "ifx_ppa_api_tools.h"
#include <net/ifx_ppa_stack_al.h>
#if defined(CONFIG_CPU_FREQ) || defined(CONFIG_IFX_PMCU) || defined(CONFIG_IFX_PMCU_MODULE)
#include "ifx_ppa_api_pwm.h"
#endif
#include <linux/swap.h>

/*
 * ####################################
 *              Definition
 * ####################################
 */



/*
 *  hash calculation
 */

#define MIN_POLLING_INTERVAL 1
uint32_t sys_mem_check_flag=0;
uint32_t stop_session_add_threshold_mem=500; //unit is K
#define K(x) ((x) << (PAGE_SHIFT - 10))



/*
 * ####################################
 *              Data Type
 * ####################################
 */



/*
 * ####################################
 *             Declaration
 * ####################################
 */

/*
 *  implemented in PPA PPE Low Level Driver (Data Path)
 */


//  routing session list item operation
static INLINE void ppa_init_session_list_item(struct session_list_item *);
/*static INLINE */struct session_list_item *ppa_alloc_session_list_item(void);
/*static INLINE*/ void ppa_free_session_list_item(struct session_list_item *);
/*static INLINE*/ void __ppa_insert_session_item(struct session_list_item *);
static INLINE void ppa_remove_session_item(struct session_list_item *);
static void ppa_free_session_list(void);
void ppa_uc_get_htable_lock(void);
void ppa_uc_release_htable_lock(void);

// to ckeck the global hardware acceleration enabled flag
static INLINE int32_t ppa_hw_accelerate(PPA_BUF *ppa_buf, PPA_SESSION *p_session);

//  multicast routing group list item operation
void ppa_init_mc_group_list_item(struct mc_group_list_item *);
static INLINE struct mc_group_list_item *ppa_alloc_mc_group_list_item(void);
static INLINE void ppa_free_mc_group_list_item(struct mc_group_list_item *);
static INLINE void ppa_insert_mc_group_item(struct mc_group_list_item *);
static INLINE void ppa_remove_mc_group_item(struct mc_group_list_item *);
static void ppa_free_mc_group_list(void);

//  routing session timeout help function
static INLINE uint32_t ppa_get_default_session_timeout(void);
static void ppa_check_hit_stat(unsigned long);
static void ppa_mib_count(unsigned long);

//  bridging session list item operation
static INLINE void ppa_bridging_init_session_list_item(struct bridging_session_list_item *);
static INLINE struct bridging_session_list_item *ppa_bridging_alloc_session_list_item(void);
static INLINE void ppa_bridging_free_session_list_item(struct bridging_session_list_item *);
static INLINE void ppa_bridging_insert_session_item(struct bridging_session_list_item *);
static INLINE void ppa_bridging_remove_session_item(struct bridging_session_list_item *);
static void ppa_free_bridging_session_list(void);

//  bridging session timeout help function
static INLINE uint32_t ppa_bridging_get_default_session_timeout(void);
static void ppa_bridging_check_hit_stat(unsigned long);

//  help function for special function
static INLINE void ppa_remove_routing_sessions_on_netif(PPA_IFNAME *);
static INLINE void ppa_remove_mc_groups_on_netif(PPA_IFNAME *);
static INLINE void ppa_remove_bridging_sessions_on_netif(PPA_IFNAME *);

#if defined(CONFIG_LTQ_PPA_API_SW_FASTPATH)
extern volatile uint8_t g_sw_fastpath_enabled;
#endif
/*
 * ####################################
 *           Global Variable
 * ####################################
 */

/*
 *  routing session table
 */
static PPA_LOCK                     g_session_list_lock;
static PPA_HLIST_HEAD               g_session_list_hash_table[SESSION_LIST_HASH_TABLE_SIZE];
//static uint32_t                     g_session_list_length = 0;
static PPA_MEM_CACHE               *g_session_item_cache = NULL;
static PPA_TIMER                    g_hit_stat_timer;
/*static*/ uint32_t                     g_hit_polling_time = DEFAULT_HIT_POLLING_TIME;
static PPA_TIMER                    g_mib_cnt_timer;
static uint32_t                     g_mib_polling_time = DEFAULT_MIB_POLLING_TIME;
static PPA_ATOMIC                   g_hw_session_cnt; /*including unicast & multicast sessions */

/*
 *  multicast routing session table
 */
static PPA_LOCK                     g_mc_group_list_lock;
static PPA_HLIST_HEAD               g_mc_group_list_hash_table[SESSION_LIST_MC_HASH_TABLE_SIZE];
//static uint32_t                     g_mc_group_list_length = 0;
static PPA_MEM_CACHE               *g_mc_group_item_cache = NULL;


#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
static PPA_LOCK                     g_capwap_group_list_lock;
//static PPA_HLIST_HEAD           capwap_list_head;
static PPA_INIT_LIST_HEAD(capwap_list_head);
static PPA_MEM_CACHE               *g_capwap_group_item_cache = NULL;
#endif

/*
 *  bridging session table
 */
static PPA_LOCK                     g_bridging_session_list_lock;
static PPA_HLIST_HEAD               g_bridging_session_list_hash_table[BRIDGING_SESSION_LIST_HASH_TABLE_SIZE];
static PPA_MEM_CACHE               *g_bridging_session_item_cache = NULL;
static PPA_TIMER                    g_bridging_hit_stat_timer;
static uint32_t                     g_bridging_hit_polling_time = DEFAULT_BRIDGING_HIT_POLLING_TIME;



/*
 * ####################################
 *           Extern Variable
 * ####################################
 */



/*
 * ####################################
 *            Local Function
 * ####################################
 */

/*
 *  routing session list item operation
 */

static INLINE void ppa_init_session_list_item(struct session_list_item *p_item)
{
    ppa_memset(p_item, 0, sizeof(*p_item));
    //ppa_lock_init(&p_item->uc_lock);
    PPA_INIT_HLIST_NODE(&p_item->hlist);
    p_item->mtu             = g_ppa_ppa_mtu;
    p_item->routing_entry   = ~0;
    p_item->pppoe_entry     = ~0;
    p_item->mtu_entry       = ~0;
    p_item->src_mac_entry   = ~0;
    p_item->out_vlan_entry  = ~0;
    p_item->tunnel_idx      = ~0;
#if defined(L2TP_CONFIG) && L2TP_CONFIG
    p_item->l2tptunnel_idx  = ~0;
#endif
}

/*static INLINE */ struct session_list_item *ppa_alloc_session_list_item(void)
{
    struct session_list_item *p_item;

    p_item = ppa_mem_cache_alloc(g_session_item_cache);
    if ( p_item )
        ppa_init_session_list_item(p_item);

    return p_item;
}

/*static INLINE */void ppa_free_session_list_item(struct session_list_item *p_item)
{
#if defined(SESSION_STATISTIC_DEBUG) && SESSION_STATISTIC_DEBUG 
    PPE_IPV6_INFO ip6_info;
    if( p_item->src_ip6_index )
    {
        ip6_info.ipv6_entry = p_item->src_ip6_index - 1;
        ifx_ppa_drv_del_ipv6_entry(&ip6_info, 0);
    }
    if( p_item->dst_ip6_index )
    {
        ip6_info.ipv6_entry = p_item->dst_ip6_index - 1;
        ifx_ppa_drv_del_ipv6_entry(&ip6_info, 0);
    }
#endif
    ppa_hw_del_session(p_item);
#if defined(CONFIG_LTQ_PPA_API_SW_FASTPATH)
    ppa_sw_del_session(p_item);
#endif
    ppa_mem_cache_free(p_item, g_session_item_cache);
}

#if 0
/*static INLINE*/void ppa_insert_session_item(struct session_list_item *p_item)
{
    uint32_t idx;

    idx = SESSION_LIST_HASH_VALUE(p_item->session, p_item->flags & SESSION_IS_REPLY);
    ppa_uc_get_htable_lock();
    ppa_hlist_add_head(&p_item->hlist, &g_session_list_hash_table[idx]);    
    //g_session_list_length++;
    ppa_uc_release_htable_lock();
}
#endif

/*static INLINE*/ void __ppa_insert_session_item(struct session_list_item *p_item)
{
    uint32_t idx;

    idx = SESSION_LIST_HASH_VALUE(p_item->session, p_item->flags & SESSION_IS_REPLY);
    ppa_hlist_add_head(&p_item->hlist, &g_session_list_hash_table[idx]);
}

static INLINE void ppa_delete_uc_item(struct session_list_item *p_item)
{
    ppa_remove_session_item(p_item);
    ppa_free_session_list_item(p_item);
}

static INLINE void ppa_remove_session_item(struct session_list_item *p_item)
{
    ppa_hlist_del(&p_item->hlist);   
}

static void ppa_free_session_list(void)
{
    PPA_HLIST_NODE *node, *tmp;
    struct session_list_item *p_item;
    uint32_t idx;

    ppa_uc_get_htable_lock();

    for(idx = 0; idx < NUM_ENTITY(g_session_list_hash_table); idx ++){
        ppa_hlist_for_each_entry_safe(p_item, node, tmp, &g_session_list_hash_table[idx], hlist){
            ppa_delete_uc_item(p_item);
        }
    }
    
    ppa_uc_release_htable_lock();
}

int32_t ifx_ppa_session_delete(PPA_SESSION *p_session, uint32_t flags)
{
    int32_t ret = IFX_FAILURE;
    uint32_t flag_template[2] = {PPA_F_SESSION_ORG_DIR, PPA_F_SESSION_REPLY_DIR};
    struct session_list_item *p_item;
    uint32_t i;

    ppa_uc_get_htable_lock();

    for ( i = 0; i < NUM_ENTITY(flag_template); i++ )
    {
        if ( !(flags & flag_template[i]) )
            continue;

        //  i = 0, org dir, i = 1, reply dir
        if ( __ppa_lookup_session(p_session, i, &p_item) != IFX_PPA_SESSION_EXISTS )
            continue;


        dump_list_item(p_item, "ifx_ppa_session_delete");
        ppa_remove_session_item(p_item);        
        ppa_free_session_list_item(p_item);

        ret = IFX_SUCCESS;
    }
    
    ppa_uc_release_htable_lock();
    
    return ret;
}

int32_t ifx_ppa_speed_handle_frame(PPA_BUF *ppa_buf, PPA_SESSION *p_session, uint32_t flags)
{
    struct session_list_item *p_item = NULL;
    int32_t ret = IFX_PPA_SESSION_NOT_FILTED;

    ppa_uc_get_htable_lock();
    if ( __ppa_lookup_session(p_session, flags & PPA_F_SESSION_REPLY_DIR, &p_item) == IFX_PPA_SESSION_EXISTS ){
        ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT,"session exists, p_item = 0x%08x\n", (u32)p_item);
        if (flags & PPA_F_BEFORE_NAT_TRANSFORM ){
            p_item->num_adds++;             //  every packet will come to here two times ( before nat and after nat). so make sure only count once
            p_item->mips_bytes += ppa_buf->len + PPA_ETH_HLEN + PPA_ETH_CRCLEN;
            ret = IFX_PPA_SESSION_FILTED;
        }
        else{
            if (p_item->flags & (SESSION_ADDED_IN_HW | SESSION_NOT_ACCEL_FOR_MGM | SESSION_NOT_ACCELABLE ) ){
                /*
                             * Session exists, but this packet will take s/w path nonetheless.
                             * Can happen for individual pkts of a session, or for complete sessions!
                             * For eg., if WAN upstream acceleration is disabled.
                             */
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"session exists for flags=0x%x\n", p_item->flags  ); 
                if( p_item->flags & (SESSION_ADDED_IN_HW ) ) ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT,"session exists for HW already\n");
                if( p_item->flags & (SESSION_NOT_ACCEL_FOR_MGM ) ) ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT,"session exists for SESSION_NOT_ACCEL_FOR_MGM already\n");
                if( p_item->flags & (SESSION_NOT_ACCELABLE ) ) ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT,"session exists for SESSION_NOT_ACCELABLE already\n");            
                ret = IFX_PPA_SESSION_FILTED;
                goto __PPA_FILTERED;
            }

            //  not enough hit
            if ( p_item->num_adds < g_ppa_min_hits )
            {
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"p_item->num_adds (%d) < g_ppa_min_hits (%d)\n", p_item->num_adds, g_ppa_min_hits);
                SET_DBG_FLAG(p_item, SESSION_DBG_NOT_REACH_MIN_HITS);
                ret = IFX_PPA_SESSION_FILTED;
                goto __PPA_FILTERED;
            }
            else
            {
                CLR_DBG_FLAG(p_item, SESSION_DBG_NOT_REACH_MIN_HITS);
            }

            //  check if session needs to be handled in MIPS for conntrack handling (e.g. ALG)
            if ( ppa_check_is_special_session(ppa_buf, p_session) )
            {
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"can not accelerate: %08X\n", (uint32_t)p_session);
                p_item->flags |= SESSION_NOT_ACCELABLE;
                SET_DBG_FLAG(p_item, SESSION_DBG_ALG);
                ret = IFX_PPA_SESSION_FILTED;
                goto __PPA_FILTERED;
            }

            //  for TCP, check whether connection is established
            if ( ppa_get_pkt_ip_proto(ppa_buf) == IFX_IPPROTO_TCP )
            {
                if ( !ppa_is_tcp_established(p_item->session) )
                {
                    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"tcp not established: %08X\n", (uint32_t)p_session);
                    SET_DBG_FLAG(p_item, SESSION_DBG_TCP_NOT_ESTABLISHED);
                    ret = IFX_PPA_SESSION_FILTED;
                    goto __PPA_FILTERED;
                }
                else
                {
                    p_item->flags |= SESSION_IS_TCP;
                    CLR_DBG_FLAG(p_item, SESSION_DBG_TCP_NOT_ESTABLISHED);
                }
            }
        }
    }
    else{
        if(!(flags & PPA_F_BEFORE_NAT_TRANSFORM) ){//if no record when in after nat, filter it.
            ret = IFX_PPA_SESSION_FILTED;
            goto __PPA_FILTERED;
        }
    }

__PPA_FILTERED:

    ppa_uc_release_htable_lock();
    return ret;
}

#if defined(SESSION_STATISTIC_DEBUG) && SESSION_STATISTIC_DEBUG 
static int32_t inline __update_session_hash(struct session_list_item *p_item)
{
    PPE_SESSION_HASH hash;
    int32_t res;
    PPE_IPV6_INFO ip6_info;

    if( p_item->flag2 & SESSION_FLAG2_HASH_INDEX_DONE ) return IFX_SUCCESS;
    
    hash.f_is_lan = ( p_item->flags & SESSION_LAN_ENTRY ) ? 1:0;
    if( p_item->flags & SESSION_IS_IPV6 )
    {   
        if( !p_item->src_ip6_index )
        {
#ifdef CONFIG_IPV6
            ppa_memcpy( ip6_info.ip.ip6, p_item->src_ip.ip6, sizeof(ip6_info.ip.ip6) );
#endif
            if(ifx_ppa_drv_add_ipv6_entry(&ip6_info, 0) == IFX_SUCCESS)
            {
                p_item->src_ip6_index = ip6_info.ipv6_entry + 1;
                p_item->src_ip6_psuedo_ip = ip6_info.psuedo_ip;
            }
            else
            {
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"Failed to add new IPV6.\n");
                return IFX_FAILURE;
            }
        }
        if( !p_item->dst_ip6_index )
        {
#ifdef CONFIG_IPV6
            ppa_memcpy( ip6_info.ip.ip6, p_item->dst_ip.ip6, sizeof(ip6_info.ip.ip6) );
#endif
            if(ifx_ppa_drv_add_ipv6_entry(&ip6_info, 0) == IFX_SUCCESS)
            {
                p_item->dst_ip6_index = ip6_info.ipv6_entry + 1;
                p_item->dst_ip6_psuedo_ip = ip6_info.psuedo_ip;
            }
            else
            {
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"Failed to add new IPV6.\n");
                return IFX_FAILURE;
            }
            hash.src_ip = p_item->src_ip6_psuedo_ip;
            hash.dst_ip = p_item->dst_ip6_psuedo_ip;
        }
    }
    else
    {        
        hash.src_ip = p_item->src_ip.ip;
        hash.dst_ip = p_item->dst_ip.ip;        
    }
    
    hash.src_port = p_item->src_port;
    hash.dst_port = p_item->dst_port;
    res = ifx_ppa_get_session_hash(&hash, 0);

    p_item->hash_index = hash.hash_index;
    p_item->hash_table_id = hash.hash_table_id;
    p_item->flag2 |= SESSION_FLAG2_HASH_INDEX_DONE;
    return res;
}
#endif

#if 1
int32_t ifx_ppa_update_session(PPA_BUF *ppa_buf, PPA_SESSION *p_session, uint32_t flags)
{
    uint32_t ret = IFX_PPA_SESSION_NOT_ADDED;
    struct session_list_item *p_item = NULL;

    ppa_uc_get_htable_lock();    
    if ( __ppa_lookup_session(p_session, flags & PPA_F_SESSION_REPLY_DIR, &p_item) != IFX_PPA_SESSION_EXISTS ){
        int8_t strbuf[64];
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"post routing point but no session exist: dst_ip = %s\n",
            ppa_get_pkt_ip_string(ppa_get_pkt_dst_ip(ppa_buf), ppa_is_pkt_ipv6(ppa_buf), strbuf));
        ppa_uc_release_htable_lock();
        return IFX_PPA_SESSION_NOT_ADDED;
    }

#if defined(SKB_PRIORITY_DEBUG) && SKB_PRIORITY_DEBUG    
    if ( g_ppa_dbg_enable & DBG_ENABLE_MASK_MARK_TEST ) //for test qos queue only depends on mark last 4 bits value
    {
        p_item->priority = ppa_api_set_test_qos_priority_via_mark(ppa_buf );
    }
    else if ( g_ppa_dbg_enable & DBG_ENABLE_MASK_PRI_TEST ) //for test qos queue only depends on tos last 4 bits value
    {
        p_item->priority = ppa_api_set_test_qos_priority_via_tos(ppa_buf );
    }
    else 
    {
        p_item->priority = ppa_get_pkt_priority(ppa_buf);        
    }
    p_item->mark = ppa_get_skb_mark(ppa_buf);
    
#endif

    //  get information needed by hardware/firmware
    if ( (ret = ppa_update_session(ppa_buf, p_item, flags)) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"update session fail\n");
        goto __UPDATE_FAIL;
    }

    //dump_list_item(p_item, "after ppa_update_session");

    //  protect before ARP
    if ( !(p_item->flags & SESSION_TX_ITF_IPOA_PPPOA_MASK)
        && p_item->dst_mac[0] == 0
        && p_item->dst_mac[1] == 0
        && p_item->dst_mac[2] == 0
        && p_item->dst_mac[3] == 0
        && p_item->dst_mac[4] == 0
        && p_item->dst_mac[5] == 0 )
    {
        SET_DBG_FLAG(p_item, SESSION_DBG_ZERO_DST_MAC);
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"sesion exist for zero mac\n");
        goto __UPDATE_FAIL;
    }
    CLR_DBG_FLAG(p_item, SESSION_DBG_ZERO_DST_MAC);

#if defined(SESSION_STATISTIC_DEBUG) && SESSION_STATISTIC_DEBUG  
    if( __update_session_hash(p_item) != IFX_SUCCESS )
    {
        SET_DBG_FLAG(p_item, SESSION_DBG_UPDATE_HASH_FAIL);
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"__update_session_hash failed\n");
        goto __UPDATE_FAIL;
    }
    CLR_DBG_FLAG(p_item, SESSION_DBG_UPDATE_HASH_FAIL);
    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"hash table/id:%d:%d\n", p_item->hash_table_id, p_item->hash_index);
#endif
// place holder for some explicit criteria based on which the session must go through SW acceleration
  if(ppa_hw_accelerate(ppa_buf, p_session))
  {
    if ( (ret = ppa_hw_add_session(p_item, 0)) != IFX_SUCCESS )
    {
        p_item->flag2 |= SESSION_FLAG2_ADD_HW_FAIL;  //PPE hash full in this hash index, or IPV6 table full ,...
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ppa_hw_add_session(p_item) fail\n");
#if defined(CONFIG_LTQ_PPA_API_SW_FASTPATH)
	// add to hardware failed.. so this session needs to go through software acceleration
    	ppa_sw_add_session(ppa_buf, p_item);
    	ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"entry added in sw acceleration\n");
#endif
    }
    else
    {
        p_item->flag2 &= ~SESSION_FLAG2_ADD_HW_FAIL;
    }
  }
  else
  {
    // entry criteria to hardware failed so the session needs to put into software acceleration
#if defined(CONFIG_LTQ_PPA_API_SW_FASTPATH)
    ppa_sw_add_session(ppa_buf, p_item);
    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"entry added in sw acceleration\n");
#endif
  }  

    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"hardware/firmware entry added\n");
    ret = IFX_PPA_SESSION_ADDED;
    
__UPDATE_FAIL:
    ppa_uc_release_htable_lock();
    return ret;
}

#else
int32_t ifx_ppa_update_session(PPA_BUF *ppa_buf, PPA_SESSION *p_session, uint32_t flags)
{
    uint32_t ret = IFX_PPA_SESSION_NOT_ADDED;
    struct session_list_item *p_item = NULL;
    struct session_list_item tmp_item;

    ppa_uc_get_htable_lock();
    if ( __ppa_lookup_session(p_session, flags & PPA_F_SESSION_REPLY_DIR, &p_item) != IFX_PPA_SESSION_EXISTS ){
        int8_t strbuf[64];
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"post routing point but no session exist: dst_ip = %s\n",
            ppa_get_pkt_ip_string(ppa_get_pkt_dst_ip(ppa_buf), ppa_is_pkt_ipv6(ppa_buf), strbuf));
        return IFX_PPA_SESSION_NOT_ADDED;
    }  
    tmp_item = *p_item;
    ppa_uc_release_htable_lock();

#if defined(SKB_PRIORITY_DEBUG) && SKB_PRIORITY_DEBUG
    if ( g_ppa_dbg_enable & DBG_ENABLE_MASK_MARK_TEST ) //for test qos queue only depends on mark last 4 bits value
    {
        p_item->priority = ppa_api_set_test_qos_priority_via_mark(ppa_buf );
    }
    else if ( g_ppa_dbg_enable & DBG_ENABLE_MASK_PRI_TEST ) //for test qos queue only depends on tos last 4 bits value
    {
        tmp_item.priority = ppa_api_set_test_qos_priority_via_tos(ppa_buf );
    }
    else 
    {
        tmp_item.priority = ppa_get_pkt_priority(ppa_buf);        
    }
    tmp_item.mark = ppa_get_skb_mark(ppa_buf);
    
#endif
    

    //  get information needed by hardware/firmware
    if ( (ret = ppa_update_session(ppa_buf, &tmp_item, flags)) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"update session fail\n");
        return ret;
    }

    dump_list_item(&tmp_item, "after ppa_update_session");

    //  protect before ARP
    if ( !(tmp_item.flags & SESSION_TX_ITF_IPOA_PPPOA_MASK)
        && tmp_item.dst_mac[0] == 0
        && tmp_item.dst_mac[1] == 0
        && tmp_item.dst_mac[2] == 0
        && tmp_item.dst_mac[3] == 0
        && tmp_item.dst_mac[4] == 0
        && tmp_item.dst_mac[5] == 0 )
    {
        SET_DBG_FLAG(&tmp_item, SESSION_DBG_ZERO_DST_MAC);
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"sesion exist for zero mac\n");
        return IFX_FAILURE;
    }
    CLR_DBG_FLAG(&tmp_item, SESSION_DBG_ZERO_DST_MAC);

    ppa_uc_get_htable_lock();
    if ( __ppa_lookup_session(p_session, flags & PPA_F_SESSION_REPLY_DIR, &p_item) != IFX_PPA_SESSION_EXISTS )
    {
        int8_t strbuf[64];
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "post routing point not exit after ppa_update_session: dst_ip = %s",
            ppa_get_pkt_ip_string(ppa_get_pkt_dst_ip(ppa_buf), ppa_is_pkt_ipv6(ppa_buf), strbuf));
        ppa_uc_release_htable_lock();;
        return IFX_PPA_SESSION_NOT_ADDED;
    }
    ppa_memcpy( ((char *)p_item) + sizeof(p_item->hlist), ((char *)&tmp_item) + sizeof(p_item->hlist), sizeof(tmp_item)- sizeof(p_item->hlist) ); //skip first pointer next

#if defined(SESSION_STATISTIC_DEBUG) && SESSION_STATISTIC_DEBUG  
    if( __update_session_hash(p_item) != IFX_SUCCESS )
    {
        SET_DBG_FLAG(p_item, SESSION_DBG_UPDATE_HASH_FAIL);
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"__update_session_hash failed\n");
        ppa_uc_release_htable_lock();;
        return IFX_FAILURE;
    }
    CLR_DBG_FLAG(p_item, SESSION_DBG_UPDATE_HASH_FAIL);
    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"hash table/id:%d:%d\n", p_item->hash_table_id, p_item->hash_index);
#endif
    if ( (ret = ppa_hw_add_session(p_item, 0)) != IFX_SUCCESS )
    {
        p_item->flag2 |= SESSION_FLAG2_ADD_HW_FAIL;  //PPE hash full in this hash index, or IPV6 table full ,...
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ppa_hw_add_session(p_item) fail\n");
        ppa_uc_release_htable_lock();;
        return IFX_FAILURE;
    }
    else
    {
        p_item->flag2 &= ~SESSION_FLAG2_ADD_HW_FAIL;
    }

    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"hardware/firmware entry added\n");
    ret = IFX_PPA_SESSION_ADDED;
    
__UPDATE_FAIL:
    ppa_uc_release_htable_lock();;
    return ret;
}
#endif

uint32_t ppa_get_routing_session_count(uint16_t bf_lan, uint32_t count_flag, uint32_t hash_index)
{
    struct session_list_item *p_item;
    PPA_HLIST_NODE *node;
    uint32_t i;
    uint32_t count = 0, start_pos=0;
    uint32_t session_flag;

    if( hash_index ) 
    {
        start_pos = hash_index -1;        
    }
        

    if( bf_lan == 0 )        
    {
        session_flag = SESSION_WAN_ENTRY;
    }
    else if ( bf_lan == 1 )
    {
        session_flag = SESSION_LAN_ENTRY;
    }
    else if ( bf_lan == 2 )  /*non lan/wan, it means unknow session */
    {          
#if 1    
        ppa_uc_get_htable_lock();
        for(i = start_pos; i < NUM_ENTITY(g_session_list_hash_table); i ++)
        {
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"i=%d\n", i);
            ppa_hlist_for_each_entry(p_item,node,&g_session_list_hash_table[i],hlist)
            {                
                 if( !(p_item->flags & SESSION_LAN_ENTRY) && !(p_item->flags & SESSION_WAN_ENTRY) )
                 {                  
                    count++;               
                 }
                 ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"p_item=%p with index=%u count=%u\n", p_item, i, count);
            }
            if( hash_index ) break;
        }
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ppa_get_non_lan_wan_routing_session_count=%d\n", count);
        ppa_uc_release_htable_lock();
#else
        do
       {   
            
            uint32_t   session_flag;
            struct session_list_item *pp_item;
            struct session_list_item *p_item_one_index;
            uint32_t pos = 0;
            int32_t ret=IFX_SUCCESS;
            uint32_t i, tmp_num;
            
            p_item_one_index = NULL;
            count = 0;
            if( (ret == ppa_session_get_items_via_index(pos, &p_item_one_index,0, &tmp_num)) == IFX_INDEX_OVERFLOW ) //end of list
            {   
                break;
            }
            pos ++;

            if( (ret == IFX_ENOMEM) && !p_item_one_index )
            {
                err("p_item_one_index NULL for no memory for hash index %d\n", pos);
                break;
            }
            if( ret != IFX_SUCCESS ) continue;     
                                     
            for(i=0; i<tmp_num; i++)
            {
                pp_item = &p_item_one_index[i];
                if( !(pp_item->flags & SESSION_LAN_ENTRY) && !(pp_item->flags & SESSION_WAN_ENTRY) )
                {
                    count ++;                    
                }              
            }

            ppa_free(p_item_one_index);
            p_item_one_index = NULL;

       } while(1);
#endif

        return count;
    }
    else
    {
        critial_err("wrong bf_flab value:%u\n", bf_lan);
        return 0;
    }
    
    ppa_uc_get_htable_lock();

    for(i = start_pos; i < NUM_ENTITY(g_session_list_hash_table); i ++){
        ppa_hlist_for_each_entry(p_item,node,&g_session_list_hash_table[i],hlist){
            if(p_item->flags & session_flag){
                if( count_flag == 0 ||/* get all PPA sessions with acceleratted and non-accelearted  */
                ( (count_flag == SESSION_ADDED_IN_HW) && (p_item->flags & SESSION_ADDED_IN_HW) )  || /*get all accelerated sessions only */
                ( (count_flag == SESSION_ADDED_IN_SW) && (p_item->flags & SESSION_ADDED_IN_SW) )  || /*get all software accelerated sessions only */
                ( (count_flag == SESSION_NON_ACCE_MASK) && !(p_item->flags & SESSION_ADDED_IN_HW ) && !(p_item->flags & SESSION_ADDED_IN_SW )) ){
                    count++;
                    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ppa_get_routing_session_count=%d\n", count);
                }
            }
        }
        if( hash_index ) break;
    }

    ppa_uc_release_htable_lock();
    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ppa_get_routing_session_count=%d with count_flag=%x\n", count, count_flag);

    return count;
}



/*
 *  multicast routing group list item operation
 */

void ppa_init_mc_group_list_item(struct mc_group_list_item *p_item)
{
    ppa_memset(p_item, 0, sizeof(*p_item));
    PPA_INIT_HLIST_NODE(&p_item->mc_hlist);
    p_item->mc_entry        = ~0;
    p_item->src_mac_entry   = ~0;
    p_item->out_vlan_entry  = ~0;
    p_item->dst_ipv6_entry  = ~0;
    p_item->src_ipv6_entry  = ~0;
}

static INLINE struct mc_group_list_item *ppa_alloc_mc_group_list_item(void)
{
    struct mc_group_list_item *p_item;

    p_item = ppa_mem_cache_alloc(g_mc_group_item_cache);
    if ( p_item )
        ppa_init_mc_group_list_item(p_item);

    return p_item;
}

static INLINE void ppa_free_mc_group_list_item(struct mc_group_list_item *p_item)
{
    ppa_hw_del_mc_group(p_item);
    ppa_mem_cache_free(p_item, g_mc_group_item_cache);
}

static INLINE void ppa_insert_mc_group_item(struct mc_group_list_item *p_item)
{
    uint32_t idx;

    idx = SESSION_LIST_MC_HASH_VALUE(p_item->ip_mc_group.ip.ip);
    ppa_hlist_add_head(&p_item->mc_hlist, &g_mc_group_list_hash_table[idx]);
    //g_mc_group_list_length++;
}

static INLINE void ppa_remove_mc_group_item(struct mc_group_list_item *p_item)
{
   ppa_hlist_del(&p_item->mc_hlist);
}

static void ppa_free_mc_group_list(void)
{
    struct mc_group_list_item *p_mc_item;
    PPA_HLIST_NODE *node, *tmp;
    int i;

    ppa_mc_get_htable_lock();
    for ( i = 0; i < NUM_ENTITY(g_mc_group_list_hash_table); i++ )
    {
        ppa_hlist_for_each_entry_safe(p_mc_item, node, tmp, &g_mc_group_list_hash_table[i], mc_hlist){
            ppa_free_mc_group_list_item(p_mc_item);
        }
    }
    ppa_mc_release_htable_lock();
}

/*
 *  routing session timeout help function
 */

static INLINE uint32_t ppa_get_default_session_timeout(void)
{
    return DEFAULT_TIMEOUT_IN_SEC;
}

#if defined(CONFIG_IFX_PPA_IF_MIB) && CONFIG_IFX_PPA_IF_MIB
int32_t update_netif_mib(PPA_IFNAME *ifname, uint64_t new_mib, uint8_t is_rx, uint32_t reset_flag)
{
    struct netif_info *ifinfo;
    int i;

    if(!ifname){
        return IFX_SUCCESS;
    }
    
    if ( ppa_netif_lookup(ifname, &ifinfo) == IFX_SUCCESS )
    {
        struct netif_info *sub_ifinfo;
        if( is_rx ) 
        {
            if( reset_flag ) 
                ifinfo->acc_rx = 0;
            else
                ifinfo->acc_rx += new_mib;
        }
        else 
        {
            if( reset_flag )
                ifinfo->acc_tx = 0;
            else
                ifinfo->acc_tx += new_mib;
        }
        
        for(i=0; i<ifinfo->sub_if_index; i++)
        {
            if ( ppa_netif_lookup(ifinfo->sub_if_name[i], &sub_ifinfo) == IFX_SUCCESS )
            {
                if( is_rx ) 
                {
                    if( reset_flag )
                        sub_ifinfo->acc_rx =0 ;
                    else
                        sub_ifinfo->acc_rx += new_mib;
                    
                }
                else
                {
                    if( reset_flag )
                        sub_ifinfo->acc_tx = 0;
                    else
                        sub_ifinfo->acc_tx += new_mib;
                }
                ppa_netif_put(sub_ifinfo);
            }
        }
        ppa_netif_put(ifinfo);
    }     
    return IFX_SUCCESS;
}
#endif

static void ppa_mib_count(unsigned long dummy)
{
#if defined(CONFIG_IFX_PPA_IF_MIB) && CONFIG_IFX_PPA_IF_MIB

    PPA_PORT_MIB     *port_mib = NULL;
    PPA_QOS_STATUS   *qos_mib  = NULL;
    
    uint32_t   i;
    
    //update port mib without update rate_mib
    port_mib = ppa_malloc(sizeof(PPA_PORT_MIB));
    if( port_mib )
    {
        ppa_update_port_mib(port_mib,0, 0);
        ppa_free(port_mib);
    }

    //update qos queue mib without update rate_mib
    qos_mib = ppa_malloc(sizeof(PPA_QOS_STATUS));
    if(qos_mib)
    {
        for(i=0; i<IFX_MAX_PORT_NUM; i++)
        {
            ppa_memset(qos_mib, 0, sizeof(qos_mib));
            qos_mib->qos_queue_portid = i;
            ppa_update_qos_mib(qos_mib,0, 0);
        }
        ppa_free(qos_mib);
    }

    //restart timer
    ppa_timer_add(&g_mib_cnt_timer, g_mib_polling_time);
    return;
    
#endif
}


void ppa_check_hit_stat_clear_mib(int32_t flag)
{
    struct session_list_item  *p_item    = NULL;
    struct mc_group_list_item *p_mc_item = NULL;
    PPA_HLIST_NODE            *node      = NULL;
    
    uint32_t i;
    PPE_ROUTING_INFO route = {0};
    PPE_MC_INFO      mc    = {0};
    uint64_t         tmp   = 0;
    

    ppa_uc_get_htable_lock();
    for ( i = 0; i < NUM_ENTITY(g_session_list_hash_table); i++ )
    {
        ppa_hlist_for_each_entry(p_item, node,&g_session_list_hash_table[i], hlist){
            tmp = 0;
            route.entry = p_item->routing_entry;     
            
            if( flag & PPA_CMD_CLEAR_PORT_MIB )   {               
                p_item->acc_bytes = 0;
                p_item->mips_bytes = 0;
                p_item->last_bytes = 0;
                p_item->prev_sess_bytes=0;
                p_item->prev_clear_acc_bytes=0;
                p_item->prev_clear_mips_bytes = 0;
                if( p_item->flags & SESSION_ADDED_IN_HW)
                {
                    route.bytes = 0; route.f_hit = 0;
                    ifx_ppa_drv_get_routing_entry_bytes(&route, flag); /* clear session mib */
                    ifx_ppa_drv_test_and_clear_hit_stat(&route, 0);  /* clear hit */
                }
             #if defined(CONFIG_IFX_PPA_IF_MIB) && CONFIG_IFX_PPA_IF_MIB
                if( p_item->rx_if )
                    update_netif_mib(ppa_get_netif_name(p_item->rx_if), tmp, 1, 1);
                if( p_item->br_rx_if )
                    update_netif_mib(ppa_get_netif_name(p_item->br_rx_if), tmp, 1, 1);
                if( p_item->tx_if ) 
                    update_netif_mib(ppa_get_netif_name(p_item->tx_if), tmp, 0, 1);
                if( p_item->br_tx_if )
                    update_netif_mib(ppa_get_netif_name(p_item->br_tx_if), tmp, 0, 1);
            #endif
  
                //printk("ppa_check_hit_stat_clear_mib: need clear\n");
                continue;
            }
            
            if(!(p_item->flags & SESSION_ADDED_IN_HW))
                continue;    
            route.bytes = 0; route.f_hit = 0;
            ifx_ppa_drv_test_and_clear_hit_stat(&route, 0);
            if( route.bytes > WRAPROUND_SESSION_MIB ) {
			   err( "why route.bytes(%llu) > WRAPROUND_SESSION_MIB(%llu)\n", route.bytes, (uint64_t)WRAPROUND_SESSION_MIB );
			   printk("why route.bytes(%llu) > WRAPROUND_SESSION_MIB(%llu)\n", route.bytes, (uint64_t) WRAPROUND_SESSION_MIB );
            }
            
            if ( route.f_hit )   {
                p_item->last_hit_time = ppa_get_time_in_sec();  
#if defined(CONFIG_NF_CONNTRACK_SUPPORT) || defined(CONFIG_NF_CONNTRACK)
                p_item->session->last_exp_refresh_time = jiffies;
#endif
                ifx_ppa_drv_get_routing_entry_bytes(&route, flag); //flag means clear mib counter or not
                if( (uint32_t)(route.bytes) >= p_item->last_bytes)                
                    tmp = route.bytes - (uint64_t)p_item->last_bytes;
                else
                    tmp = route.bytes + (uint64_t)WRAPROUND_SESSION_MIB -(uint64_t)p_item->last_bytes; 
                if( tmp >= (uint64_t)WRAPROUND_SESSION_MIB ) 
                    err("The handling of Session bytes wrappround wrongly \n") ;  
                p_item->acc_bytes = p_item->acc_bytes + tmp;
                p_item->last_bytes = (uint32_t)route.bytes;

            #if defined(CONFIG_IFX_PPA_IF_MIB) && CONFIG_IFX_PPA_IF_MIB
                if( p_item->rx_if )
                    update_netif_mib(ppa_get_netif_name(p_item->rx_if), tmp, 1, 0);
                if( p_item->br_rx_if )
                    update_netif_mib(ppa_get_netif_name(p_item->br_rx_if), tmp, 1, 0);
                if( p_item->tx_if ) 
                    update_netif_mib(ppa_get_netif_name(p_item->tx_if), tmp, 0, 0);
                if( p_item->br_tx_if )
                    update_netif_mib(ppa_get_netif_name(p_item->br_tx_if), tmp, 0, 0);
                }
            #endif

            }
        }
    ppa_uc_release_htable_lock();
      
    ppa_mc_get_htable_lock();   
    for (i = 0; i < NUM_ENTITY(g_mc_group_list_hash_table); i ++){
        ppa_hlist_for_each_entry(p_mc_item, node, &g_mc_group_list_hash_table[i], mc_hlist){
             tmp = 0;
             mc.p_entry = p_mc_item->mc_entry;

             if( flag & PPA_CMD_CLEAR_PORT_MIB )   {
                if( p_mc_item->flags & SESSION_ADDED_IN_HW)
                    ifx_ppa_drv_get_mc_entry_bytes(&mc, flag);/* flag means clear mib counter or not */
                p_mc_item->acc_bytes = 0;
                p_mc_item->last_bytes = 0;
                mc.f_hit = 0; mc.bytes = 0;
                ifx_ppa_drv_test_and_clear_mc_hit_stat(&mc, 0); /*clear hit */
                
            #if defined(CONFIG_IFX_PPA_IF_MIB) && CONFIG_IFX_PPA_IF_MIB
                 if( p_mc_item->src_netif )
                    update_netif_mib(ppa_get_netif_name(p_mc_item->src_netif), tmp, 1, 1);

                 for(i=0; i<p_mc_item->num_ifs; i++ )
                    if( p_mc_item->netif[i])
                        update_netif_mib(ppa_get_netif_name(p_mc_item->netif[i]), tmp, 0, 1);
            #endif
   
                
                //printk("ppa_check_hit_stat_clear_mib: need clear\n");
                continue;
            }
            if(!(p_mc_item->flags & SESSION_ADDED_IN_HW))
                continue;
            mc.f_hit = 0; mc.bytes = 0;
            ifx_ppa_drv_test_and_clear_mc_hit_stat(&mc, 0);
            if(mc.f_hit){
                p_mc_item->last_hit_time = ppa_get_time_in_sec();
                ifx_ppa_drv_get_mc_entry_bytes(&mc, flag);

            if( (uint32_t ) mc.bytes >= p_mc_item->last_bytes){
                tmp = mc.bytes - (uint64_t)p_mc_item->last_bytes;
            } else {
                tmp = mc.bytes + (uint64_t)WRAPROUND_32BITS - (uint64_t)p_mc_item->last_bytes;
            }
            p_mc_item->acc_bytes += tmp;

            p_mc_item->last_bytes = mc.bytes;

            #if defined(CONFIG_IFX_PPA_IF_MIB) && CONFIG_IFX_PPA_IF_MIB
                 if( p_mc_item->src_netif )
                    update_netif_mib(ppa_get_netif_name(p_mc_item->src_netif), tmp, 1, 0);

                 for(i=0; i<p_mc_item->num_ifs; i++ )
                    if( p_mc_item->netif[i])
                        update_netif_mib(ppa_get_netif_name(p_mc_item->netif[i]), tmp, 0, 0);
            #endif
            }
            
        }
    }
    ppa_mc_release_htable_lock();

}

static void ppa_check_hit_stat(unsigned long dummy)
{
    ppa_check_hit_stat_clear_mib(0);
    //restart timer
    ppa_timer_add(&g_hit_stat_timer, g_hit_polling_time);
    //printk("jiffy=%lu, HZ=%lu, poll_timer=%lu\n", jiffies, HZ, ppa_api_get_session_poll_timer());
}


/*
 *  bridging session list item operation
 */

static INLINE void ppa_bridging_init_session_list_item(struct bridging_session_list_item *p_item)
{
    ppa_memset(p_item, 0, sizeof(*p_item));
    p_item->bridging_entry = ~0;
    PPA_INIT_HLIST_NODE(&p_item->br_hlist);
}

static INLINE struct bridging_session_list_item *ppa_bridging_alloc_session_list_item(void)
{
    struct bridging_session_list_item *p_item;

    p_item = ppa_mem_cache_alloc(g_bridging_session_item_cache);
    if ( p_item )
        ppa_bridging_init_session_list_item(p_item);

    return p_item;
}

static INLINE void ppa_bridging_free_session_list_item(struct bridging_session_list_item *p_item)
{
    ppa_bridging_hw_del_session(p_item);
    ppa_mem_cache_free(p_item, g_bridging_session_item_cache);
}

static INLINE void ppa_bridging_insert_session_item(struct bridging_session_list_item *p_item)
{
    uint32_t idx;

    idx = PPA_BRIDGING_SESSION_LIST_HASH_VALUE(p_item->mac);
    ppa_hlist_add_head(&p_item->br_hlist, &g_bridging_session_list_hash_table[idx]);
}

static INLINE void ppa_bridging_remove_session_item(struct bridging_session_list_item *p_item)
{
    ppa_hlist_del(&p_item->br_hlist);
}

static void ppa_free_bridging_session_list(void)
{
    struct bridging_session_list_item *p_br_item;
    PPA_HLIST_NODE *node, *tmp;
    int i;

    ppa_br_get_htable_lock();
    
    for ( i = 0; i < NUM_ENTITY(g_bridging_session_list_hash_table); i++ )
    {
        ppa_hlist_for_each_entry_safe(p_br_item, node, tmp, &g_bridging_session_list_hash_table[i], br_hlist){             
            ppa_bridging_remove_session(p_br_item);
        }
    }
    ppa_br_release_htable_lock();
}

/*
 *  bridging session timeout help function
 */

static INLINE uint32_t ppa_bridging_get_default_session_timeout(void)
{
    return DEFAULT_BRIDGING_TIMEOUT_IN_SEC;
}

static void ppa_bridging_check_hit_stat(unsigned long dummy)
{
    struct bridging_session_list_item *p_item;
    uint32_t i;
    PPA_HLIST_NODE *node;
    PPE_BR_MAC_INFO br_mac={0};

    ppa_br_get_htable_lock();
    for ( i = 0; i < NUM_ENTITY(g_bridging_session_list_hash_table); i++ )
    {
        ppa_hlist_for_each_entry(p_item, node, &g_bridging_session_list_hash_table[i],br_hlist){
            br_mac.p_entry = p_item->bridging_entry;
            if ( !(p_item->flags & SESSION_STATIC) )
            {
                ifx_ppa_drv_test_and_clear_bridging_hit_stat( &br_mac, 0);
                if( br_mac.f_hit ) 
                    p_item->last_hit_time = ppa_get_time_in_sec();
            }
        }
    }
    ppa_br_release_htable_lock();

    ppa_timer_add(&g_bridging_hit_stat_timer, g_bridging_hit_polling_time);
}

uint32_t ppa_get_br_count (uint32_t count_flag)
{
    PPA_HLIST_NODE *node;
    uint32_t count = 0;
    uint32_t idx;

    ppa_br_get_htable_lock();
    
    for(idx = 0; idx < NUM_ENTITY(g_bridging_session_list_hash_table); idx ++){
        ppa_hlist_for_each(node,&g_bridging_session_list_hash_table[idx]){
            count ++;
        }
    }

    ppa_br_release_htable_lock();

    return count;
}


/*
 *  help function for special function
 */

static INLINE void ppa_remove_routing_sessions_on_netif(PPA_IFNAME *ifname)
{
    uint32_t idx;
    struct session_list_item *p_item = NULL;
    PPA_HLIST_NODE *node, *tmp;

    ppa_uc_get_htable_lock();
    for ( idx = 0; idx < NUM_ENTITY(g_session_list_hash_table); idx++ )
    {
        ppa_hlist_for_each_entry_safe(p_item, node, tmp, &g_session_list_hash_table[idx], hlist){
            if ( ppa_is_netif_name(p_item->rx_if, ifname) || ppa_is_netif_name(p_item->tx_if, ifname) ){
                ppa_delete_uc_item(p_item);
            }
        }
    }
    ppa_uc_release_htable_lock();
    
}

static INLINE void ppa_remove_mc_groups_on_netif(PPA_IFNAME *ifname)
{
    uint32_t idx;
    struct mc_group_list_item *p_mc_item;
    PPA_HLIST_NODE *node, *tmp;
    int i;

    ppa_lock_get(&g_mc_group_list_lock);
    for ( idx = 0; idx < NUM_ENTITY(g_mc_group_list_hash_table); idx++ ){
        ppa_hlist_for_each_entry_safe(p_mc_item, node, tmp, &g_mc_group_list_hash_table[idx],mc_hlist){
            
            for ( i = 0; i < p_mc_item->num_ifs; i++ ){
                if ( ppa_is_netif_name(p_mc_item->netif[i], ifname) ){
                    p_mc_item->netif[i] = NULL;
                    break;
                }
            }

            if(i >= p_mc_item->num_ifs)
                continue;
            
            p_mc_item->num_ifs --;
            if(!p_mc_item->num_ifs){
                ppa_remove_mc_group(p_mc_item);
            }
            else{
                for(i = i + 1; i < p_mc_item->num_ifs + 1; i ++){//shift other netif on the list
                    p_mc_item->netif[i - 1] = p_mc_item->netif[i];
                    p_mc_item->ttl[i - 1] = p_mc_item->ttl[i];
                }
                p_mc_item->if_mask = p_mc_item->if_mask >> 1; //should we remove this item???
            }               
        }
    }
        
    ppa_lock_release(&g_mc_group_list_lock);
}

static INLINE void ppa_remove_bridging_sessions_on_netif(PPA_IFNAME *ifname)
{
    uint32_t idx;
    struct bridging_session_list_item *p_br_item;
    PPA_HLIST_NODE *node, *tmp;
    
    ppa_br_get_htable_lock();
    for ( idx = 0; idx < NUM_ENTITY(g_bridging_session_list_hash_table); idx++ )
    {
        ppa_hlist_for_each_entry_safe(p_br_item, node, tmp, &g_bridging_session_list_hash_table[idx],br_hlist){
            if ( ppa_is_netif_name(p_br_item->netif, ifname)){
                ppa_bridging_remove_session(p_br_item);
            }
        }
    }
    ppa_br_release_htable_lock();

    return;
}



/*
 * ####################################
 *           Global Function
 * ####################################
 */

/*
 *  routing session operation
 */

int32_t ppa_lookup_session(PPA_SESSION *p_session, uint32_t is_reply, struct session_list_item **pp_item)
{
    int32_t ret;
    struct session_list_item *p_uc_item;
    PPA_HLIST_NODE *node;
    uint32_t idx;

    if( !pp_item  )
        ppa_debug(DBG_ENABLE_MASK_ASSERT,"pp_item == NULL\n");

    ret = IFX_PPA_SESSION_NOT_ADDED;

    idx = SESSION_LIST_HASH_VALUE(p_session, is_reply);

    *pp_item = NULL;
    ppa_uc_get_htable_lock();
    
    ppa_hlist_for_each_entry(p_uc_item, node, &g_session_list_hash_table[idx], hlist){
        if(ppa_is_session_equal(p_session, p_uc_item->session)){
            ret = IFX_PPA_SESSION_EXISTS;
            *pp_item = p_uc_item;
            break;
        }
    }
    ppa_uc_release_htable_lock();

    return ret;
}

int32_t __ppa_lookup_session(PPA_SESSION *p_session, uint32_t is_reply, struct session_list_item **pp_item)
{
    int32_t ret;
    struct session_list_item *p_uc_item;
    PPA_HLIST_NODE *node;
    uint32_t idx;

    if( !pp_item  )
        ppa_debug(DBG_ENABLE_MASK_ASSERT,"pp_item == NULL\n");

    ret = IFX_PPA_SESSION_NOT_ADDED;

    idx = SESSION_LIST_HASH_VALUE(p_session, is_reply);
    
    ppa_hlist_for_each_entry(p_uc_item, node, &g_session_list_hash_table[idx], hlist){
        if(ppa_is_session_equal(p_session, p_uc_item->session)){
            ret = IFX_PPA_SESSION_EXISTS;
            *pp_item = p_uc_item;
            break;
        }
    }

    return ret;
}
  
int32_t ppa_add_session(PPA_BUF *ppa_buf, PPA_SESSION *p_session, uint32_t flags)
{
    struct session_list_item *p_item;
    uint32_t is_reply = (flags & PPA_F_SESSION_REPLY_DIR) ? 1 : 0;
    int32_t ret = IFX_SUCCESS;
    struct sysinfo sysinfo;
    
    if( sys_mem_check_flag )    
    {        
        si_meminfo(&sysinfo);        
        si_swapinfo(&sysinfo);        
        //printk(KERN_ERR "freeram=%d K\n", K(sysinfo.freeram) );        
        if( K(sysinfo.freeram) <= stop_session_add_threshold_mem )        
        {            
            err("System memory too low: %lu K bytes\n", K(sysinfo.freeram)) ;              
            return IFX_ENOMEM;        
        }    
    }


    ppa_uc_get_htable_lock();
    
    if(__ppa_lookup_session(p_session, is_reply, &p_item) == IFX_PPA_SESSION_EXISTS){
        goto __ADD_SESSION_DONE;
    }

    p_item = ppa_alloc_session_list_item();
    if ( !p_item )
    {
        ret = IFX_ENOMEM;
        ppa_debug(DBG_ENABLE_MASK_ERR,"failed in memory allocation\n");
        goto __ADD_SESSION_DONE;
    }
    //dump_list_item(p_item, "ppa_add_session (after init)");
    
    p_item->session = p_session;
    if ( (flags & PPA_F_SESSION_REPLY_DIR) )
        p_item->flags    |= SESSION_IS_REPLY;
    //lock item
    
    __ppa_insert_session_item(p_item);
    
    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ppa_get_session(ppa_buf) = %08X\n", (uint32_t)ppa_get_session(ppa_buf));

    p_item->ip_proto      = ppa_get_pkt_ip_proto(ppa_buf);
    p_item->ip_tos        = ppa_get_pkt_ip_tos(ppa_buf);
    p_item->src_ip        = ppa_get_pkt_src_ip(ppa_buf);
    p_item->src_port      = ppa_get_pkt_src_port(ppa_buf);
    p_item->dst_ip        = ppa_get_pkt_dst_ip(ppa_buf);
    p_item->dst_port      = ppa_get_pkt_dst_port(ppa_buf);
    p_item->rx_if         = ppa_get_pkt_src_if(ppa_buf);
    p_item->timeout       = ppa_get_default_session_timeout();
    p_item->last_hit_time = ppa_get_time_in_sec();

    if(ppa_is_pkt_ipv6(ppa_buf)){
        p_item->flags    |= SESSION_IS_IPV6;
    }
#if defined(SKB_PRIORITY_DEBUG) && SKB_PRIORITY_DEBUG
    if ( g_ppa_dbg_enable & DBG_ENABLE_MASK_MARK_TEST ) //for test qos queue only depends on mark last 4 bits value
    {
       p_item->priority = ppa_api_set_test_qos_priority_via_mark(ppa_buf );
    }
    else if ( g_ppa_dbg_enable & DBG_ENABLE_MASK_PRI_TEST ) //for test qos queue only depends on tos last 4 bits value
    {
        p_item->priority  = ppa_api_set_test_qos_priority_via_tos(ppa_buf );
    }
    else 
        p_item->priority  = ppa_get_pkt_priority(ppa_buf);

    p_item->mark = ppa_get_skb_mark(ppa_buf);
#endif
    ppa_get_pkt_rx_src_mac_addr(ppa_buf, p_item->src_mac);

    p_item->mips_bytes   += ppa_buf->len + PPA_ETH_HLEN + PPA_ETH_CRCLEN;
    p_item->num_adds ++;

    //dump_list_item(p_item, "ppa_add_session (after Add)");
    ppa_uc_release_htable_lock();

    return ret;
        
__ADD_SESSION_DONE:
    //release table lock
    ppa_uc_release_htable_lock();
    
    return ret;
}

int32_t ppa_update_session(PPA_BUF *ppa_buf, struct session_list_item *p_item, uint32_t flags)
{
    int32_t ret = IFX_SUCCESS,swa_ret = IFX_SUCCESS;
    PPA_NETIF *netif;
    PPA_IPADDR ip;
    uint32_t port;
    uint32_t dscp;
    struct netif_info *rx_ifinfo, *tx_ifinfo;
    //uint32_t vlan_tag;
    int f_is_ipoa_or_pppoa = 0;
    int qid;
#if defined(L2TP_CONFIG) && L2TP_CONFIG
    PPA_IFNAME new_rxifname[PPA_IF_NAME_SIZE];
    PPA_IFNAME new_txifname[PPA_IF_NAME_SIZE];
    PPA_NETIF *new_rxnetif;
    PPA_NETIF *new_txnetif;
#endif

#ifdef CONFIG_LTQ_MINI_JUMBO_FRAME_SUPPORT
    int extra_hdr_bytes=0;
    int tx_mtu=0; 
#endif



    p_item->tx_if = ppa_get_pkt_dst_if(ppa_buf);
    p_item->mtu  = g_ppa_ppa_mtu; //reset MTU since later it will be updated

    /*
     *  update and get rx/tx information
     */

    if ( ppa_netif_update(p_item->rx_if, NULL) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"failed in collecting info of rx_if (%s)\n", ppa_get_netif_name(p_item->rx_if));
        SET_DBG_FLAG(p_item, SESSION_DBG_RX_IF_UPDATE_FAIL);
        return IFX_EAGAIN;
    }

    if ( ppa_netif_update(p_item->tx_if, NULL) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"failed in collecting info of tx_if (%s)\n", ppa_get_netif_name(p_item->tx_if));
        SET_DBG_FLAG(p_item, SESSION_DBG_TX_IF_UPDATE_FAIL);
        return IFX_EAGAIN;
    }

    if ( ppa_netif_lookup(ppa_get_netif_name(p_item->rx_if), &rx_ifinfo) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"failed in getting info structure of rx_if (%s)\n", ppa_get_netif_name(p_item->rx_if));
        SET_DBG_FLAG(p_item, SESSION_DBG_RX_IF_NOT_IN_IF_LIST);
        return IFX_ENOTPOSSIBLE;
    }

    if ( ppa_netif_lookup(ppa_get_netif_name(p_item->tx_if), &tx_ifinfo) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"failed in getting info structure of tx_if (%s)\n", ppa_get_netif_name(p_item->tx_if));
        SET_DBG_FLAG(p_item, SESSION_DBG_TX_IF_NOT_IN_IF_LIST);
        ppa_netif_put(rx_ifinfo);
        return IFX_ENOTPOSSIBLE;
    }

    /*
     *  PPPoE is highest level, collect PPPoE information
     */

    p_item->flags &= ~SESSION_VALID_PPPOE;

    if ( (rx_ifinfo->flags & (NETIF_WAN_IF | NETIF_PPPOE)) == (NETIF_WAN_IF | NETIF_PPPOE) )
    {
        //  src interface is WAN and PPPoE
        p_item->pppoe_session_id = rx_ifinfo->pppoe_session_id;
        p_item->flags |= SESSION_VALID_PPPOE;
        SET_DBG_FLAG(p_item, SESSION_DBG_RX_PPPOE);
    }

    //  if destination interface is PPPoE, it covers the previous setting
    if ( (tx_ifinfo->flags & (NETIF_WAN_IF | NETIF_PPPOE)) == (NETIF_WAN_IF | NETIF_PPPOE) )
    {
        if( (p_item->flags & SESSION_VALID_PPPOE) )
            ppa_debug(DBG_ENABLE_MASK_ASSERT,"both interfaces are WAN PPPoE interface, not possible\n");
        p_item->pppoe_session_id = tx_ifinfo->pppoe_session_id;
        p_item->flags |= SESSION_VALID_PPPOE;
        SET_DBG_FLAG(p_item, SESSION_DBG_TX_PPPOE);
        //  adjust MTU to ensure ethernet frame size does not exceed 1518 (without VLAN)
#ifdef CONFIG_LTQ_MINI_JUMBO_FRAME_SUPPORT
       extra_hdr_bytes +=8;
#else
        p_item->mtu -= 8;
#endif
    }
#if defined(L2TP_CONFIG) && L2TP_CONFIG
    if ( (rx_ifinfo->flags & (NETIF_WAN_IF | NETIF_PPPOL2TP)) == (NETIF_WAN_IF | NETIF_PPPOL2TP) )
    {
        //  src interface is WAN and PPPOL2TP
        p_item->pppol2tp_session_id = rx_ifinfo->pppol2tp_session_id; //Add tunnel id also
	p_item->pppol2tp_tunnel_id = rx_ifinfo->pppol2tp_tunnel_id;
        p_item->flags |= SESSION_VALID_PPPOL2TP;
        SET_DBG_FLAG(p_item, SESSION_DBG_RX_PPPOL2TP);
	if(ppa_pppol2tp_get_base_netif(p_item->rx_if, new_rxifname) == IFX_SUCCESS )
	{
	    new_rxnetif = ppa_get_netif(new_rxifname);
	    if( ppa_check_is_ppp_netif(new_rxnetif))
	    {
                if ( ppa_check_is_pppoe_netif(new_rxnetif) )
	        {
        	    p_item->pppoe_session_id = rx_ifinfo->pppoe_session_id;
        	    p_item->flags |= SESSION_VALID_PPPOE;
        	    SET_DBG_FLAG(p_item, SESSION_DBG_RX_PPPOE);
		}
	    }
	}
    }

    if ( (tx_ifinfo->flags & (NETIF_WAN_IF | NETIF_PPPOL2TP)) == (NETIF_WAN_IF | NETIF_PPPOL2TP) )
    {
        if( (p_item->flags & SESSION_VALID_PPPOL2TP) )
            ppa_debug(DBG_ENABLE_MASK_ASSERT,"both interfaces are WAN PPPoL2TP interface, not possible\n");
        p_item->pppol2tp_session_id = tx_ifinfo->pppol2tp_session_id;
        p_item->pppol2tp_tunnel_id = tx_ifinfo->pppol2tp_tunnel_id;
        p_item->flags |= SESSION_VALID_PPPOL2TP;
        SET_DBG_FLAG(p_item, SESSION_DBG_TX_PPPOL2TP);
#ifdef CONFIG_LTQ_MINI_JUMBO_FRAME_SUPPORT
       extra_hdr_bytes +=48;
#else
        p_item->mtu -= 48;
#endif
	if(ppa_pppol2tp_get_base_netif(p_item->tx_if, new_txifname) == IFX_SUCCESS )
	{
	    new_txnetif = ppa_get_netif(new_txifname);
	    if( ppa_check_is_ppp_netif(new_txnetif))
	    {
                if ( ppa_check_is_pppoe_netif(new_txnetif) )
	        {
        	    p_item->pppoe_session_id = tx_ifinfo->pppoe_session_id;
        	    p_item->flags |= SESSION_VALID_PPPOE;
        	    SET_DBG_FLAG(p_item, SESSION_DBG_TX_PPPOE);
        	    //  adjust MTU to ensure ethernet frame size does not exceed 1518 (without VLAN)
        	    p_item->mtu -= 8;
		}
	    }	
	}
    }
#endif

    /*
     *  detect bridge and get the real effective device under this bridge
     *  do not support VLAN interface created on bridge
     */

    if ( (rx_ifinfo->flags & (NETIF_BRIDGE | NETIF_PPPOE)) == NETIF_BRIDGE )
    //  can't handle PPPoE over bridge properly, because src mac info is corrupted
    {
        if ( !(rx_ifinfo->flags & NETIF_PHY_IF_GOT)
            || (netif = ppa_get_netif(rx_ifinfo->phys_netif_name)) == NULL )
        {
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"failed in get underlying interface of PPPoE interface (RX)\n");
            ret = IFX_ENOTPOSSIBLE;
            goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
        }
        while ( (rx_ifinfo->flags & NETIF_BRIDGE) )
        {
            if ( (ret = ppa_get_br_dst_port_with_mac(netif, p_item->src_mac, &netif)) != IFX_SUCCESS )
            {
                SET_DBG_FLAG(p_item, SESSION_DBG_SRC_BRG_IF_NOT_IN_BRG_TBL);
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ppa_get_br_dst_port_with_mac fail\n");
                if ( ret != IFX_EAGAIN )
                    ret = IFX_FAILURE;
                goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
            }
            else
            {
#if defined(CONFIG_IFX_PPA_IF_MIB) && CONFIG_IFX_PPA_IF_MIB         
                p_item->br_rx_if = netif;
#endif
                CLR_DBG_FLAG(p_item, SESSION_DBG_SRC_BRG_IF_NOT_IN_BRG_TBL);
            }

            if ( ppa_netif_update(netif, NULL) != IFX_SUCCESS )
            {
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"failed in collecting info of dst_rx_if (%s)\n", ppa_get_netif_name(netif));
                SET_DBG_FLAG(p_item, SESSION_DBG_RX_IF_UPDATE_FAIL);
                ret = IFX_EAGAIN;
                goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
            }

            ppa_netif_put(rx_ifinfo);

            if ( ppa_netif_lookup(ppa_get_netif_name(netif), &rx_ifinfo) != IFX_SUCCESS )
            {
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"failed in getting info structure of dst_rx_if (%s)\n", ppa_get_netif_name(netif));
                SET_DBG_FLAG(p_item, SESSION_DBG_SRC_IF_NOT_IN_IF_LIST);
                ppa_netif_put(tx_ifinfo);
                return IFX_ENOTPOSSIBLE;
            }
        }
    }

    if ( (tx_ifinfo->flags & NETIF_BRIDGE) )
    {
        if ( !(tx_ifinfo->flags & NETIF_PHY_IF_GOT)
            || (netif = ppa_get_netif(tx_ifinfo->phys_netif_name)) == NULL )
        {
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"failed in get underlying interface of PPPoE interface (TX)\n");
            ret = IFX_ENOTPOSSIBLE;
            goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
        }
        while ( (tx_ifinfo->flags & NETIF_BRIDGE) )
        {
            if ( (ret = ppa_get_br_dst_port(netif, ppa_buf, &netif)) != IFX_SUCCESS )
            {
                SET_DBG_FLAG(p_item, SESSION_DBG_DST_BRG_IF_NOT_IN_BRG_TBL);
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ppa_get_br_dst_port fail\n");
                if ( ret != IFX_EAGAIN )
                    ret = IFX_FAILURE;
                goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
            }
            else
            {
#if defined(CONFIG_IFX_PPA_IF_MIB) && CONFIG_IFX_PPA_IF_MIB
                p_item->br_tx_if = netif;
#endif
                CLR_DBG_FLAG(p_item, SESSION_DBG_DST_BRG_IF_NOT_IN_BRG_TBL);
            }

            if ( ppa_netif_update(netif, NULL) != IFX_SUCCESS )
            {
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"failed in collecting info of dst_tx_if (%s)\n", ppa_get_netif_name(netif));
                SET_DBG_FLAG(p_item, SESSION_DBG_TX_IF_UPDATE_FAIL);
                ret = IFX_EAGAIN;
                goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
            }

            ppa_netif_put(tx_ifinfo);

            if ( ppa_netif_lookup(ppa_get_netif_name(netif), &tx_ifinfo) != IFX_SUCCESS )
            {
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"failed in getting info structure of dst_tx_if (%s)\n", ppa_get_netif_name(netif));
                SET_DBG_FLAG(p_item, SESSION_DBG_DST_IF_NOT_IN_IF_LIST);
                ppa_netif_put(rx_ifinfo);
                return IFX_ENOTPOSSIBLE;
            }
        }
    }

    /*
     *  check whether physical port is determined or not
     */

    if ( !(tx_ifinfo->flags & NETIF_PHYS_PORT_GOT) )
    {
        ret = IFX_FAILURE;
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"tx no NETIF_PHYS_PORT_GOT\n");
        goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
    }

    /*
     *  decide which table to insert session, LAN side table or WAN side table
     */

    if ( (rx_ifinfo->flags & (NETIF_LAN_IF | NETIF_WAN_IF)) == (NETIF_LAN_IF | NETIF_WAN_IF) )
    {
        switch ( tx_ifinfo->flags & (NETIF_LAN_IF | NETIF_WAN_IF) )
        {
        case NETIF_LAN_IF: p_item->flags |= SESSION_WAN_ENTRY; break;
        case NETIF_WAN_IF: p_item->flags |= SESSION_LAN_ENTRY; break;
        default:
            ret = IFX_FAILURE;
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"tx_ifinfo->flags wrong LAN/WAN flag\n");
            goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
        }
    }
    else
    {
        switch ( rx_ifinfo->flags & (NETIF_LAN_IF | NETIF_WAN_IF) )
        {
        case NETIF_LAN_IF: p_item->flags |= SESSION_LAN_ENTRY; break;
        case NETIF_WAN_IF: p_item->flags |= SESSION_WAN_ENTRY; break;
        default:
            ret = IFX_FAILURE;
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"rx_ifinfo->flags wrong LAN/WAN flag\n");
            goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
        }
    }
    
    /*
     *  Check 6RD
     */
    
    if((p_item->flags & SESSION_LAN_ENTRY  && p_item->tx_if ->type == ARPHRD_SIT) || 
        (p_item->flags & SESSION_WAN_ENTRY  && p_item->rx_if ->type == ARPHRD_SIT)){
        p_item->flags |= SESSION_TUNNEL_6RD;
    }
	if(p_item->flags & SESSION_LAN_ENTRY  && (p_item->flags & SESSION_TUNNEL_6RD) ){
#ifdef CONFIG_LTQ_MINI_JUMBO_FRAME_SUPPORT
       		extra_hdr_bytes +=20;
#else
		//adjust mtu size,need to reserve ipv4 header space
		p_item->mtu -= 20; 
#endif
	}

    /*
        *  Check Dslite
        */
#if defined(CONFIG_IFX_PPA_DSLITE) && CONFIG_IFX_PPA_DSLITE
    if((p_item->flags & SESSION_LAN_ENTRY  && p_item->tx_if ->type == ARPHRD_TUNNEL6) || 
        (p_item->flags & SESSION_WAN_ENTRY  && p_item->rx_if ->type == ARPHRD_TUNNEL6)){
        p_item->flags |= SESSION_TUNNEL_DSLITE;
    }
#endif
	if(p_item->flags & SESSION_LAN_ENTRY && (p_item->flags & SESSION_TUNNEL_DSLITE)){
#ifdef CONFIG_LTQ_MINI_JUMBO_FRAME_SUPPORT
       	    extra_hdr_bytes +=40;
#else
             //adjust mtu size, to reserve ipv6 header space
            p_item->mtu -= 40;
#endif
	}

    /*
     *  collect VLAN information (outer/inner)
     */

    //  do not support VLAN interface created on bridge

    if ( (rx_ifinfo->flags & NETIF_VLAN_CANT_SUPPORT) || (tx_ifinfo->flags & NETIF_VLAN_CANT_SUPPORT) )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"physical interface has limited VLAN support\n");
        p_item->flags |= SESSION_NOT_ACCELABLE;
        if( rx_ifinfo->flags & NETIF_VLAN_CANT_SUPPORT )
            SET_DBG_FLAG(p_item, SESSION_DBG_RX_VLAN_CANT_SUPPORT);
        else
            SET_DBG_FLAG(p_item, SESSION_DBG_TX_VLAN_CANT_SUPPORT);
        goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
    }
    CLR_DBG_FLAG(p_item, SESSION_DBG_RX_VLAN_CANT_SUPPORT);
    CLR_DBG_FLAG(p_item, SESSION_DBG_TX_VLAN_CANT_SUPPORT);

    if ( (rx_ifinfo->flags & NETIF_VLAN_OUTER) )
        p_item->flags |= SESSION_VALID_OUT_VLAN_RM;
    if ( (tx_ifinfo->flags & NETIF_VLAN_OUTER) )
    {
        if( tx_ifinfo->out_vlan_netif == NULL )
        {
            p_item->out_vlan_tag = tx_ifinfo->outer_vid; //  ignore prio and cfi
        }
        else
        {
            p_item->out_vlan_tag = ( tx_ifinfo->outer_vid & PPA_VLAN_TAG_MASK ) | ppa_vlan_dev_get_egress_qos_mask(tx_ifinfo->out_vlan_netif, ppa_buf);
        }

        p_item->flags |= SESSION_VALID_OUT_VLAN_INS;
    }

    if ( (rx_ifinfo->flags & NETIF_VLAN_INNER) )
        p_item->flags |= SESSION_VALID_VLAN_RM;
    if ( (tx_ifinfo->flags & NETIF_VLAN_INNER) )
    {
        if( tx_ifinfo->in_vlan_netif == NULL )
        {
            p_item->new_vci = tx_ifinfo->inner_vid ;     //  ignore prio and cfi
        }
        else
        {
            p_item->new_vci = ( tx_ifinfo->inner_vid & PPA_VLAN_TAG_MASK ) | ppa_vlan_dev_get_egress_qos_mask(tx_ifinfo->in_vlan_netif, ppa_buf);
        }
        p_item->flags |= SESSION_VALID_VLAN_INS;
    }

    /*
     *  decide destination list
     *  if tx interface is based on DSL, determine which PVC it is (QID)
     */

    p_item->dest_ifid = tx_ifinfo->phys_port;
    if ( (tx_ifinfo->flags & NETIF_PHY_ATM) )
    {
        qid = ifx_ppa_drv_get_netif_qid_with_pkt(ppa_buf, tx_ifinfo->vcc, 1);
        if ( qid >= 0 )
            p_item->dslwan_qid = qid;
        else
            p_item->dslwan_qid = tx_ifinfo->dslwan_qid;
        p_item->flags |= SESSION_VALID_DSLWAN_QID;
        if ( (tx_ifinfo->flags & NETIF_EOA) )
        {
            SET_DBG_FLAG(p_item, SESSION_DBG_TX_BR2684_EOA);
        }
        else if ( (tx_ifinfo->flags & NETIF_IPOA) )
        {
            p_item->flags |= SESSION_TX_ITF_IPOA;
            SET_DBG_FLAG(p_item, SESSION_TX_ITF_IPOA);
            f_is_ipoa_or_pppoa = 1;
        }
        else if ( (tx_ifinfo->flags & NETIF_PPPOATM) )
        {
            p_item->flags |= SESSION_TX_ITF_PPPOA;
            SET_DBG_FLAG(p_item, SESSION_TX_ITF_PPPOA);
            f_is_ipoa_or_pppoa = 1;
        }
    }
    else
    {
        netif = ppa_get_netif(tx_ifinfo->phys_netif_name);
        
        qid = ifx_ppa_drv_get_netif_qid_with_pkt(ppa_buf, netif, 0);
        if ( qid >= 0 )
        {
            p_item->dslwan_qid = qid;
            p_item->flags |= SESSION_VALID_DSLWAN_QID;
        }
    }

    /*
     *  collect src IP/Port, dest IP/Port information
     */

    //  only port change with same IP not supported here, not really useful
    ppa_memset( &p_item->nat_ip, 0, sizeof(p_item->nat_ip) );
    p_item->nat_port= 0;
    ip = ppa_get_pkt_src_ip(ppa_buf);
    if ( ppa_memcmp(&ip, &p_item->src_ip, ppa_get_pkt_ip_len(ppa_buf)) != 0 )
    {
        if( p_item->flags & SESSION_LAN_ENTRY )
        {
            p_item->nat_ip = ip;
            p_item->flags |= SESSION_VALID_NAT_IP;
        }
        else
        {  //cannot acclerate
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"WAN Session cannot edit src ip\n");
            p_item->flags |= SESSION_NOT_ACCELABLE;
            SET_DBG_FLAG(p_item, SESSION_DBG_PPE_EDIT_LIMIT);
            ret = IFX_EAGAIN;
            goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
        }
    }

    port = ppa_get_pkt_src_port(ppa_buf);
    if ( port != p_item->src_port )
    {
         if( p_item->flags & SESSION_LAN_ENTRY )
         {
            p_item->nat_port = port;
            p_item->flags |= SESSION_VALID_NAT_PORT | SESSION_VALID_NAT_SNAT;
         }
         else
         {  //cannot acclerate
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"WAN Session cannot edit src port\n");
            p_item->flags |= SESSION_NOT_ACCELABLE;
            SET_DBG_FLAG(p_item, SESSION_DBG_PPE_EDIT_LIMIT);
            ret = IFX_EAGAIN;
            goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
         }
    }    
    
    ip = ppa_get_pkt_dst_ip(ppa_buf);
    if ( ppa_memcmp(&ip, &p_item->dst_ip, ppa_get_pkt_ip_len(ppa_buf)) != 0 )
    {
        if( (p_item->flags & SESSION_WAN_ENTRY) && ( ppa_zero_ip(p_item->nat_ip) )  )
        {
            p_item->nat_ip = ip;
            p_item->flags |= SESSION_VALID_NAT_IP;
        }
        else
        { //cannot accelerate
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"LAN Session cannot edit dst ip\n");
            p_item->flags |= SESSION_NOT_ACCELABLE;
            SET_DBG_FLAG(p_item, SESSION_DBG_PPE_EDIT_LIMIT);
            ret = IFX_EAGAIN;
            goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
        }
    }

    port = ppa_get_pkt_dst_port(ppa_buf);
    if ( (port  != p_item->dst_port) && ( ! p_item->nat_port  ) )
    {
        if( p_item->flags & SESSION_WAN_ENTRY )
        {
            p_item->nat_port = port;
            p_item->flags |= SESSION_VALID_NAT_PORT;
        }
        else
        { //cannot accelerate
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"LAN Session cannot edit dst port\n");
            p_item->flags |= SESSION_NOT_ACCELABLE;
            SET_DBG_FLAG(p_item, SESSION_DBG_PPE_EDIT_LIMIT);
            ret = IFX_EAGAIN;
            goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
        }
   }
   CLR_DBG_FLAG(p_item, SESSION_DBG_PPE_EDIT_LIMIT);

    if ( (tx_ifinfo->flags & (SESSION_LAN_ENTRY | NETIF_PPPOE)) == (SESSION_LAN_ENTRY | NETIF_PPPOE) )
    { //cannot accelerate
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"Non-WAN Session cannot add ppp header\n");
        p_item->flags |= SESSION_NOT_ACCELABLE;
        ret = IFX_EAGAIN;
        goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
    }
    
    /*
     *  calculate new DSCP value if necessary
     */

    dscp = ppa_get_pkt_ip_tos(ppa_buf);
    if ( dscp != p_item->ip_tos )
    {
        p_item->new_dscp = dscp >> 2;
        p_item->flags |= SESSION_VALID_NEW_DSCP;
    }

    /*
     *  IPoA/PPPoA does not have MAC address
     */

    if ( f_is_ipoa_or_pppoa )
    {
        //goto PPA_UPDATE_SESSION_DONE_SHOTCUT;
        goto PPA_UPDATE_SESSION_DONE_SKIP_MAC;
    }

/* Updated the 6rd destination address */
	 if(p_item->flags & SESSION_LAN_ENTRY  && (p_item->flags & SESSION_TUNNEL_6RD) )
    {
        p_item->sixrd_daddr = ppa_get_6rdtunel_dst_ip(ppa_buf,p_item->tx_if); 
    }

    /*
     *  get new dest MAC address for ETH, EoA
     */

    if ( ppa_get_dst_mac(ppa_buf, p_item->session, p_item->dst_mac,p_item->sixrd_daddr) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"session:%x can not get dst mac!\n", (u32)ppa_get_session(ppa_buf));
        SET_DBG_FLAG(p_item, SESSION_DBG_GET_DST_MAC_FAIL);
        ret = IFX_EAGAIN;
    }
    else{
        CLR_DBG_FLAG(p_item, SESSION_DBG_GET_DST_MAC_FAIL);
    }

#ifdef CONFIG_LTQ_MINI_JUMBO_FRAME_SUPPORT
	tx_mtu =  ppa_get_mtu(p_item->tx_if);
/* Only reduce the session's MTU, if the Tx If cannot handle the exta header bytes for the session - not on global basis */
       if ((g_ppa_ppa_mtu + extra_hdr_bytes) > tx_mtu) {
               p_item->mtu -= (g_ppa_ppa_mtu + extra_hdr_bytes - tx_mtu);
       } 
       else
       {
               p_item->mtu += (tx_mtu - ( g_ppa_ppa_mtu + extra_hdr_bytes ));
       }
 
#endif
PPA_UPDATE_SESSION_DONE_SKIP_MAC:
#if defined(CONFIG_LTQ_PPA_API_SW_FASTPATH)
    if(p_item == NULL)
    {
       printk("\n line=%d,function=%s\n",__LINE__,__FUNCTION__);
       return;
    }
    swa_ret = ppa_sw_update_session(ppa_buf, p_item, tx_ifinfo);
    if((swa_ret != IFX_SUCCESS ) && (swa_ret != IFX_EINVAL)) {
	ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"session:%x can not get software acceleration parameters!\n",(u32)ppa_get_session(ppa_buf));
	SET_DBG_FLAG(p_item, SESSION_DBG_SW_ACC_HEADER);
	ret = IFX_EAGAIN;
    }
#endif

PPA_UPDATE_SESSION_DONE_SHOTCUT:
    ppa_netif_put(rx_ifinfo);
    ppa_netif_put(tx_ifinfo);
    return ret;
}

int32_t ppa_update_session_extra(PPA_SESSION_EXTRA *p_extra, struct session_list_item *p_item, uint32_t flags)
{
    if ( (flags & PPA_F_SESSION_NEW_DSCP) )
    {
        if ( p_extra->dscp_remark )
        {
            p_item->flags |= SESSION_VALID_NEW_DSCP;
            p_item->new_dscp = p_extra->new_dscp;
        }
        else
            p_item->flags &= ~SESSION_VALID_NEW_DSCP;
    }

    if ( (flags & PPA_F_SESSION_VLAN) )
    {
        if ( p_extra->vlan_insert )
        {
            p_item->flags |= SESSION_VALID_VLAN_INS;
            p_item->new_vci = ((uint32_t)p_extra->vlan_prio << 13) | ((uint32_t)p_extra->vlan_cfi << 12) | p_extra->vlan_id;
        }
        else
        {
            p_item->flags &= ~SESSION_VALID_VLAN_INS;
            p_item->new_vci = 0;
        }

        if ( p_extra->vlan_remove )
            p_item->flags |= SESSION_VALID_VLAN_RM;
        else
            p_item->flags &= ~SESSION_VALID_VLAN_RM;
    }

    if ( (flags & PPA_F_MTU) )
    {
        p_item->flags |= SESSION_VALID_MTU;
        p_item->mtu = p_extra->mtu;
    }

    if ( (flags & PPA_F_SESSION_OUT_VLAN) )
    {
        if ( p_extra->out_vlan_insert )
        {
            p_item->flags |= SESSION_VALID_OUT_VLAN_INS;
            p_item->out_vlan_tag = p_extra->out_vlan_tag;
        }
        else
        {
            p_item->flags &= ~SESSION_VALID_OUT_VLAN_INS;
            p_item->out_vlan_tag = 0;
        }

        if ( p_extra->out_vlan_remove )
            p_item->flags |= SESSION_VALID_OUT_VLAN_RM;
        else
            p_item->flags &= ~SESSION_VALID_OUT_VLAN_RM;
    }

    if ( (flags & PPA_F_ACCEL_MODE) ) //only for hook and ioctl, not for ppe fw and HAL, It is mainly for session management
    {
        if( p_extra->accel_enable ) {
	/* If it needs to be added to h/w we simply reset flags which will make session go through linux stack */
            p_item->flags &= ~SESSION_NOT_ACCEL_FOR_MGM;
#if defined(CONFIG_LTQ_PPA_API_SW_FASTPATH)
	    p_item->flags &= ~SESSION_ADDED_IN_SW;
#endif
	} else {
#if defined(CONFIG_LTQ_PPA_HANDLE_CONNTRACK_SESSIONS)
		ppa_session_limit_update_flags(p_item,0);
#else
            p_item->flags |= SESSION_NOT_ACCEL_FOR_MGM;
#endif
	}
    }

    if ( (flags & PPA_F_PPPOE) ) //only for hook and ioctl, not for ppe fw and HAL. At present it only for test purpose
    {
        if( p_extra->pppoe_id)
        {   //need to add or replace old pppoe
            p_item->flags |= SESSION_VALID_PPPOE;
            p_item->pppoe_session_id = p_extra->pppoe_id;
        }
        else
        {   //need to change pppoe termination to transparent
            p_item->flags &= ~SESSION_VALID_PPPOE;
            p_item->pppoe_session_id = 0;
        }
    }

    return IFX_SUCCESS;
}

void dump_list_item(struct session_list_item *p_item, char *comment)
{
#if defined(DEBUG_DUMP_LIST_ITEM) && DEBUG_DUMP_LIST_ITEM
	int8_t strbuf[64];
    if ( !(g_ppa_dbg_enable & DBG_ENABLE_MASK_DUMP_ROUTING_SESSION) )
        return;
    
    if( max_print_num == 0 ) return;    
    
    if ( comment )
        printk("dump_list_item - %s\n", comment);
    else
        printk("dump_list_item\n");
    printk("  hlist            = %08X\n", (uint32_t)&p_item->hlist);
    printk("  session          = %08X\n", (uint32_t)p_item->session);
    printk("  ip_proto         = %08X\n", p_item->ip_proto);
    printk("  src_ip           = %s\n",   ppa_get_pkt_ip_string(p_item->src_ip, p_item->flags & SESSION_IS_IPV6, strbuf));
    printk("  src_port         = %d\n",   p_item->src_port);
    printk("  src_mac[6]       = %s\n",   ppa_get_pkt_mac_string(p_item->src_mac, strbuf));
    printk("  dst_ip           = %s\n",   ppa_get_pkt_ip_string(p_item->dst_ip, p_item->flags & SESSION_IS_IPV6, strbuf));
    printk("  dst_port         = %d\n",   p_item->dst_port);
    printk("  dst_mac[6]       = %s\n",   ppa_get_pkt_mac_string(p_item->dst_mac, strbuf));
    printk("  nat_ip           = %s\n",   ppa_get_pkt_ip_string(p_item->nat_ip, p_item->flags & SESSION_IS_IPV6, strbuf));
    printk("  nat_port         = %d\n",   p_item->nat_port);
    printk("  rx_if            = %08X\n", (uint32_t)p_item->rx_if);
    printk("  tx_if            = %08X\n", (uint32_t)p_item->tx_if);
    printk("  timeout          = %d\n",   p_item->timeout);
    printk("  last_hit_time    = %d\n",   p_item->last_hit_time);
    printk("  num_adds         = %d\n",   p_item->num_adds);
    printk("  pppoe_session_id = %d\n",   p_item->pppoe_session_id);
#if defined(L2TP_CONFIG) && L2TP_CONFIG
    printk("  pppol2tp_session_id = %d\n",   p_item->pppol2tp_session_id);
    printk("  pppol2tp_tunnel_id = %d\n",   p_item->pppol2tp_tunnel_id);
#endif
    printk("  new_dscp         = %d\n",   p_item->new_dscp);
    printk("  new_vci          = %08X\n",  p_item->new_vci);
    printk("  mtu              = %d\n",   p_item->mtu);
    printk("  flags            = %08X\n", p_item->flags);
    printk("  routing_entry    = %08X\n", p_item->routing_entry);
    printk("  pppoe_entry      = %08X\n", p_item->pppoe_entry);
    printk("  mtu_entry        = %08X\n", p_item->mtu_entry);
    printk("  src_mac_entry    = %08X\n", p_item->src_mac_entry);

   if( max_print_num != ~0 ) max_print_num--;

#endif
}

int32_t ppa_session_start_iteration(uint32_t *ppos, struct session_list_item **pp_item)
{
    PPA_HLIST_NODE *node = NULL;
    int idx;
    uint32_t l;

    l = *ppos + 1;

    ppa_uc_get_htable_lock();

    if( !ifx_ppa_is_init() )
    {
       *pp_item = NULL;
       return IFX_FAILURE; 
    }

    for ( idx = 0; l && idx < NUM_ENTITY(g_session_list_hash_table); idx++ )
    {
        ppa_hlist_for_each(node, &g_session_list_hash_table[idx]){
            if ( !--l )
                break;
        }
    }

    if ( l == 0 && node )
    {
        ++*ppos;
        *pp_item = ppa_hlist_entry(node, struct session_list_item, hlist);
        return IFX_SUCCESS;
    }
    else
    {
        *pp_item = NULL;
        return IFX_FAILURE;
    }
}

int32_t ppa_session_iterate_next(uint32_t *ppos, struct session_list_item **pp_item)
{
    uint32_t idx;

    if ( *pp_item == NULL )
        return IFX_FAILURE;

    if ( (*pp_item)->hlist.next != NULL )
    {
        ++*ppos;
        *pp_item = ppa_hlist_entry((*pp_item)->hlist.next, struct session_list_item, hlist);
        return IFX_SUCCESS;
    }
    else
    {
        for ( idx = SESSION_LIST_HASH_VALUE((*pp_item)->session, (*pp_item)->flags & SESSION_IS_REPLY) + 1;
              idx < NUM_ENTITY(g_session_list_hash_table);
              idx++ )
            if ( g_session_list_hash_table[idx].first != NULL )
            {
                ++*ppos;
                *pp_item = ppa_hlist_entry(g_session_list_hash_table[idx].first, struct session_list_item, hlist);
                return IFX_SUCCESS;
            }
        *pp_item = NULL;
        return IFX_FAILURE;
    }
}

void ppa_session_stop_iteration(void)
{
    ppa_uc_release_htable_lock();
}

int32_t ppa_session_get_items_via_index(uint32_t index, struct session_list_item **pp_item, uint32_t max_len, uint32_t *copy_len, uint32_t flag)
{  //note, pp_item will allocate memory for pp_item 
    struct session_list_item *p, *p_tmp = NULL;
    uint32_t i=0, count=0;
    PPA_HLIST_NODE *node = NULL;    
//    #define MAX_ITEM_PER_HASH  12
   
    if( !copy_len )
    {
        critial_err("copy_len is NULL\n");
        return IFX_FAILURE;
    }
    *copy_len = 0;
    
    if( index >= NUM_ENTITY(g_session_list_hash_table) ) 
    {
        return IFX_INDEX_OVERFLOW;
    }
#if defined(MAX_ITEM_PER_HASH)           
    count=MAX_ITEM_PER_HASH;
#else    
    count = get_ppa_sesssion_num_in_one_hash(index);
#endif    
    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "Session[%d] has %d items\n", index, count );    
    
    if( count == 0 ) return IFX_SUCCESS;    
    if( !*pp_item ) 
    { //If the buffer is not allocated yet, then allocate it  
        p_tmp = ppa_malloc( sizeof(struct session_list_item) * ( count + 1 ) );
        if( p_tmp == NULL ) 
        {
            err("ppa_malloc failed to get %d bytes memory\n", sizeof(struct session_list_item) * count  );
            return IFX_ENOMEM;
        }
        ppa_memset( (void *)p_tmp, 0, sizeof(struct session_list_item) * count);
        *pp_item = p_tmp;
        max_len= count;
    }
    else /*buffer is preallocated already */
    {
       p_tmp = *pp_item ;
       if( count > max_len ) count = max_len;
    }

    if( count > 100 )
        err("Why counter=%d in one single hash index\n", count);

    ppa_uc_get_htable_lock();
    ppa_hlist_for_each_entry(p,node,&g_session_list_hash_table[index],hlist)   
    {      
        if(i >= count ){
            break;
        }
        ppa_memcpy( &p_tmp[i], p, sizeof(struct session_list_item) );

         /*add below codes for session management purpose from shivaji --start*/
        if( (flag & SESSION_BYTE_STAMPING) && (p->flags & SESSION_ADDED_IN_HW) )
        {
            p->prev_sess_bytes = p->acc_bytes - p->prev_clear_acc_bytes;
        }
        else if( ( flag & SESSION_BYTE_STAMPING) && !(p->flags & SESSION_ADDED_IN_HW))
        {
            p->prev_sess_bytes = p->mips_bytes;
        }
        
        i++;
        if( i > 100 )
        {
            err("Why i=%d in one single hash index\n", i);
            break;
        }
    } 

    *copy_len = i;    
    
    ppa_uc_release_htable_lock();

    return IFX_SUCCESS;
}

#if defined(L2TP_CONFIG) && L2TP_CONFIG

uint16_t  checksum_l2tp(uint8_t* ip, int len)
{
   uint32_t chk_sum = 0;  

   while(len > 1){
      //chk_sum += *((uint16_t*) ip)++;
      chk_sum += *((uint16_t*) ip);
      ip +=2;
      if(chk_sum & 0x80000000)
         chk_sum = (chk_sum & 0xFFFF) + (chk_sum >> 16);
         len -= 2;
   }

   if(len)      
      chk_sum += (uint16_t) *(uint8_t *)ip;
     
   while(chk_sum>>16)
      chk_sum = (chk_sum & 0xFFFF) + (chk_sum >> 16);

   return ~chk_sum;

}
void ppa_pppol2tp_get_l2tpinfo(struct net_device *dev, PPA_L2TP_INFO *l2tpinfo)
{
    uint32_t outer_srcip;
    uint32_t outer_dstip;
    uint32_t ip_chksum;
    uint32_t udp_chksum;
    struct l2tp_ip_hdr iphdr;
    struct l2tp_udp_hdr udphdr;

    ppa_memset(&iphdr, 0, sizeof(iphdr) );
    ppa_memset(&udphdr, 0, sizeof(udphdr) );

    ppa_pppol2tp_get_src_addr(dev,&outer_srcip);
    ppa_pppol2tp_get_dst_addr(dev,&outer_dstip);

    iphdr.ihl = 0x05;
    iphdr.version = 0x4;
    iphdr.tos = 0x0;
    iphdr.tot_len = 0x0;
    iphdr.id = 0x0;
    iphdr.frag_off = 0x0;
    iphdr.ttl = 0x40;
    iphdr.protocol = 0x11;
    iphdr.saddr = outer_srcip;
    iphdr.daddr = outer_dstip;
    iphdr.check = 0x0;
    ip_chksum = checksum_l2tp((uint8_t *)(&iphdr),IP_HEADER_LEN*4);

    udphdr.saddr = outer_srcip;
    udphdr.daddr = outer_dstip;
    udphdr.src_port = 0x06a5;
    udphdr.dst_port = 0x06a5;
    udphdr.protocol = 0x0011;
    udp_chksum = checksum_l2tp((uint8_t *)(&udphdr),(sizeof(struct l2tp_udp_hdr) - 2 ));
    
    l2tpinfo->ip_chksum = ip_chksum;
    l2tpinfo->udp_chksum = udp_chksum;
    l2tpinfo->source_ip = outer_srcip;
    l2tpinfo->dest_ip = outer_dstip;

}
#endif

static INLINE int32_t ppa_hw_accelerate(PPA_BUF *ppa_buf, PPA_SESSION *p_session)
{

  /* evaluate the condition based on some logic */
  /* current logic to check is ppe fastpath is enabled */
  return g_ppe_fastpath_enabled;

}

/*
 *  routing session hardware/firmware operation
 */

int32_t ppa_hw_add_session(struct session_list_item *p_item, uint32_t f_test)
{
    uint32_t dest_list = 0;
    PPE_ROUTING_INFO route={0};
#if defined(L2TP_CONFIG) && L2TP_CONFIG
    PPA_L2TP_INFO l2tpinfo={0};
#endif
    
    int ret = IFX_SUCCESS;

#if defined(L2TP_CONFIG) && L2TP_CONFIG
    ppa_memset( &l2tpinfo, 0, sizeof(l2tpinfo) );
#endif
    //  Only add session in H/w when the called from the post-NAT hook
    ppa_memset( &route, 0, sizeof(route) );
    route.src_mac.mac_ix = ~0;
    route.pppoe_info.pppoe_ix = ~0;
#if defined(L2TP_CONFIG) && L2TP_CONFIG
    route.pppol2tp_info.pppol2tp_ix = ~0;
#endif
    route.out_vlan_info.vlan_entry = ~0;
    route.mtu_info.mtu_ix = ~0;
    
    route.route_type = (p_item->flags & SESSION_VALID_NAT_IP) ? ((p_item->flags & SESSION_VALID_NAT_PORT) ? IFX_PPA_ROUTE_TYPE_NAPT : IFX_PPA_ROUTE_TYPE_NAT) : IFX_PPA_ROUTE_TYPE_IPV4;
    if ( (p_item->flags & SESSION_VALID_NAT_IP) )
    {
        route.new_ip.f_ipv6 = 0;
        memcpy( &route.new_ip.ip, &p_item->nat_ip, sizeof(route.new_ip.ip) ); //since only IPv4 support NAT, translate it to IPv4 format
    }
    if ( (p_item->flags & SESSION_VALID_NAT_PORT) )
        route.new_port = p_item->nat_port;

    if ( (p_item->flags & (SESSION_VALID_PPPOE | SESSION_LAN_ENTRY)) == (SESSION_VALID_PPPOE | SESSION_LAN_ENTRY) )
    {
        route.pppoe_info.pppoe_session_id = p_item->pppoe_session_id;
        if ( ifx_ppa_drv_add_pppoe_entry( &route.pppoe_info, 0) == IFX_SUCCESS )
            p_item->pppoe_entry = route.pppoe_info.pppoe_ix;
        else
        {
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"add pppoe_entry error\n");
            SET_DBG_FLAG(p_item, SESSION_DBG_ADD_PPPOE_ENTRY_FAIL);
            goto SESSION_VALID_PPPOE_ERROR;
        }
    }
#if defined(L2TP_CONFIG) && L2TP_CONFIG
    if ( (p_item->flags & (SESSION_VALID_PPPOL2TP | SESSION_LAN_ENTRY)) == (SESSION_VALID_PPPOL2TP | SESSION_LAN_ENTRY) )
    {
	route.pppol2tp_info.pppol2tp_session_id = p_item->pppol2tp_session_id;
	route.pppol2tp_info.pppol2tp_tunnel_id = p_item->pppol2tp_tunnel_id;
        route.l2tptnnl_info.tunnel_type = SESSION_VALID_PPPOL2TP;
        route.l2tptnnl_info.tx_dev = p_item->tx_if;
	l2tpinfo.tunnel_id = p_item->pppol2tp_tunnel_id;
	l2tpinfo.session_id = p_item->pppol2tp_session_id;
        l2tpinfo.tunnel_type = SESSION_VALID_PPPOL2TP;
	ppa_pppol2tp_get_l2tpinfo(p_item->tx_if,&l2tpinfo);
        if(ifx_ppa_drv_add_l2tptunnel_entry(&l2tpinfo, 0) == IFX_SUCCESS){
            p_item->l2tptunnel_idx = l2tpinfo.tunnel_idx;
        }else{
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"add tunnel l2tp entry error\n");
            goto MTU_ERROR;
        }
    }
    
    if(p_item->flags & SESSION_VALID_PPPOL2TP){
        route.l2tptnnl_info.tunnel_idx = p_item->l2tptunnel_idx << 1 | 1;
    }
#endif

    route.mtu_info.mtu = p_item->mtu;
    if ( ifx_ppa_drv_add_mtu_entry(&route.mtu_info, 0) == IFX_SUCCESS )
    {
        p_item->mtu_entry = route.mtu_info.mtu_ix;
        p_item->flags |= SESSION_VALID_MTU;
    }
    else
    {
        SET_DBG_FLAG(p_item, SESSION_DBG_ADD_MTU_ENTRY_FAIL);
        goto MTU_ERROR;
    }

    if((p_item->flags & (SESSION_TUNNEL_6RD | SESSION_LAN_ENTRY)) == (SESSION_TUNNEL_6RD | SESSION_LAN_ENTRY)){
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"add 6RD entry to FW, tx dev: %s\n", p_item->tx_if->name);
        route.tnnl_info.tunnel_type = SESSION_TUNNEL_6RD;
        route.tnnl_info.tx_dev = p_item->tx_if;
        if(ifx_ppa_drv_add_tunnel_entry(&route.tnnl_info, 0) == IFX_SUCCESS){
            p_item->tunnel_idx = route.tnnl_info.tunnel_idx;
        }else{
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"add tunnel 6rd entry error\n");
            goto MTU_ERROR;
        }
    }
    if(p_item->flags & SESSION_TUNNEL_6RD){
        route.tnnl_info.tunnel_idx = p_item->tunnel_idx << 1 | 1;
/* For 6Rd acceleration, new_ip will have the 6rd destination address */
        route.new_ip.f_ipv6 = 0;
        route.new_ip.ip.ip = p_item->sixrd_daddr; 
    }

    if((p_item->flags & (SESSION_TUNNEL_DSLITE | SESSION_LAN_ENTRY)) == (SESSION_TUNNEL_DSLITE | SESSION_LAN_ENTRY)){
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"add Dslite entry to FW, tx dev: %s\n", p_item->tx_if->name);
        route.tnnl_info.tunnel_type = SESSION_TUNNEL_DSLITE;
        route.tnnl_info.tx_dev = p_item->tx_if;
        if(ifx_ppa_drv_add_tunnel_entry(&route.tnnl_info, 0) == IFX_SUCCESS){
            p_item->tunnel_idx = route.tnnl_info.tunnel_idx;
        }else{
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"add tunnel dslite entry error\n");
            goto MTU_ERROR;
        }
    }
    if(p_item->flags & SESSION_TUNNEL_DSLITE){
        route.tnnl_info.tunnel_idx = p_item->tunnel_idx << 1 | 1;
    }

    if ( !(p_item->flags & SESSION_TX_ITF_IPOA_PPPOA_MASK) )
    {
        if( !f_test )
            ppa_get_netif_hwaddr(p_item->tx_if, route.src_mac.mac, 1);
        else //for testing only: used for ioctl to add a fake routing accleration entry in PPE 
            ppa_memcpy(route.src_mac.mac, p_item->src_mac, sizeof(route.src_mac.mac) );
        if ( ifx_ppa_drv_add_mac_entry(&route.src_mac, 0) == IFX_SUCCESS )
        {
            p_item->src_mac_entry = route.src_mac.mac_ix;
            p_item->flags |= SESSION_VALID_NEW_SRC_MAC;
        }
        else
        {
            SET_DBG_FLAG(p_item, SESSION_DBG_ADD_MAC_ENTRY_FAIL);
            goto NEW_SRC_MAC_ERROR;
        }
    }

    if ( (p_item->flags & SESSION_VALID_OUT_VLAN_INS) )
    {
        route.out_vlan_info.vlan_id = p_item->out_vlan_tag;
        if ( ifx_ppa_drv_add_outer_vlan_entry( &route.out_vlan_info, 0 ) == IFX_SUCCESS )
        {            
            p_item->out_vlan_entry = route.out_vlan_info.vlan_entry;
        }
        else
        {
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"add out_vlan_ix error\n");
            SET_DBG_FLAG(p_item, SESSION_DBG_ADD_OUT_VLAN_ENTRY_FAIL);
            goto OUT_VLAN_ERROR;
        }
    }

    if ( (p_item->flags & SESSION_VALID_NEW_DSCP) )
        route.new_dscp = p_item->new_dscp;

    route.dest_list = 1 << p_item->dest_ifid;
    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"dest_list = %02X\n", dest_list);

    route.f_is_lan = (p_item->flags & SESSION_LAN_ENTRY)?1:0;
    ppa_memcpy(route.new_dst_mac, p_item->dst_mac, PPA_ETH_ALEN);
    route.dst_port = p_item->dst_port;
    route.src_port = p_item->src_port;
    route.f_is_tcp = p_item->flags & SESSION_IS_TCP;
    route.f_new_dscp_enable = p_item->flags & SESSION_VALID_NEW_DSCP;
    route.f_vlan_ins_enable =p_item->flags & SESSION_VALID_VLAN_INS;
    route.new_vci = p_item->new_vci;
    route.f_vlan_rm_enable = p_item->flags & SESSION_VALID_VLAN_RM;
    route.pppoe_mode = p_item->flags & SESSION_VALID_PPPOE;
    route.f_out_vlan_ins_enable =p_item->flags & SESSION_VALID_OUT_VLAN_INS;    
    route.f_out_vlan_rm_enable = p_item->flags & SESSION_VALID_OUT_VLAN_RM;
    route.dslwan_qid = p_item->dslwan_qid;
    
#if defined(CONFIG_IFX_PPA_IPv6_ENABLE)     
    if( p_item->flags & SESSION_IS_IPV6 )
    {
        route.src_ip.f_ipv6 = 1;
        ppa_memcpy( route.src_ip.ip.ip6, p_item->src_ip.ip6, sizeof(route.src_ip.ip.ip6));

        route.dst_ip.f_ipv6 = 1;
        ppa_memcpy( route.dst_ip.ip.ip6, p_item->dst_ip.ip6, sizeof(route.dst_ip.ip.ip6)); 
    }else
#endif
    {
        route.src_ip.f_ipv6 = 0;
        route.src_ip.ip.ip= p_item->src_ip.ip;

        route.dst_ip.f_ipv6 = 0;
        route.dst_ip.ip.ip= p_item->dst_ip.ip; 

        route.new_ip.f_ipv6 = 0;
    }
    p_item->collision_flag = 1; // for ASE/Danube back-compatible
     if ( (ret = ifx_ppa_drv_add_routing_entry( &route, 0) ) == IFX_SUCCESS )
    {
        p_item->routing_entry = route.entry;
        p_item->flags |= SESSION_ADDED_IN_HW;
        p_item->collision_flag = route.collision_flag;
        if(ppa_atomic_inc(&g_hw_session_cnt) == 1){
#if defined(CONFIG_CPU_FREQ) || defined(CONFIG_IFX_PMCU) || defined(CONFIG_IFX_PMCU_MODULE)
            ifx_ppa_pwm_activate_module();
#endif
        }
        
        return IFX_SUCCESS;
    }
    

    //  fail in add_routing_entry
    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"fail in add_routing_entry\n");
    p_item->out_vlan_entry = ~0;
    ifx_ppa_drv_del_outer_vlan_entry( &route.out_vlan_info, 0);
OUT_VLAN_ERROR:
    p_item->src_mac_entry = ~0;
    ifx_ppa_drv_del_mac_entry( &route.src_mac, 0);
NEW_SRC_MAC_ERROR:
    p_item->mtu_entry = ~0;
    ifx_ppa_drv_del_mtu_entry(&route.mtu_info, 0);
MTU_ERROR:
    p_item->pppoe_entry = ~0;    
    ifx_ppa_drv_del_pppoe_entry( &route.pppoe_info, 0);
SESSION_VALID_PPPOE_ERROR:
    return ret;
}

int32_t ppa_hw_update_session_extra(struct session_list_item *p_item, uint32_t flags)
{    
    PPE_ROUTING_INFO route={0};

    route.mtu_info.mtu_ix = ~0;
    route.pppoe_info.pppoe_ix = ~0;
    route.out_vlan_info.vlan_entry = ~0;

    if ( (flags & PPA_F_SESSION_NEW_DSCP) )
        route.update_flags |= IFX_PPA_UPDATE_ROUTING_ENTRY_NEW_DSCP_EN | IFX_PPA_UPDATE_ROUTING_ENTRY_NEW_DSCP;

    if ( (flags & PPA_F_SESSION_VLAN) )
        route.update_flags |= IFX_PPA_UPDATE_ROUTING_ENTRY_VLAN_INS_EN | IFX_PPA_UPDATE_ROUTING_ENTRY_NEW_VCI | IFX_PPA_UPDATE_ROUTING_ENTRY_VLAN_RM_EN;

    if ( (flags & PPA_F_PPPOE) )
    {   
        uint8_t f_new_pppoe=0;

        if( p_item->pppoe_session_id == 0 )
        { //need to disable pppoe flag, ie, change to PPPOE transparent

            if( p_item->pppoe_entry != ~0 )
            {
                ifx_ppa_drv_del_pppoe_entry( &route.pppoe_info, 0);
                p_item->pppoe_entry = ~0;
            }
            
            route.pppoe_mode=0;
            route.update_flags |= IFX_PPA_UPDATE_ROUTING_ENTRY_PPPOE_MODE;

            route.pppoe_info.pppoe_ix = 0;
            route.update_flags |= IFX_PPA_UPDATE_ROUTING_ENTRY_PPPOE_IX;
        }
        else
        { //need to add or replace old pppoe flag
            if( p_item->pppoe_entry != ~0 )
            { //already have pppoe entry. so check whether need to replace or not
                route.pppoe_info.pppoe_ix = p_item->pppoe_entry;
                if ( ifx_ppa_drv_get_pppoe_entry( &route.pppoe_info, 0) == IFX_SUCCESS )
                {
                    if ( route.pppoe_info.pppoe_session_id != p_item->pppoe_session_id )
                    {
                        ifx_ppa_drv_del_pppoe_entry( &route.pppoe_info, 0);
                        p_item->pppoe_entry = ~0;
                        f_new_pppoe=1;
                    }                
                }
            }
            else
            {
                f_new_pppoe=1;
            }
        
            if( f_new_pppoe )
            {
                 //  create new PPPOE entry
                route.pppoe_info.pppoe_session_id = p_item->pppoe_session_id;
                if ( ifx_ppa_drv_add_pppoe_entry( &route.pppoe_info, 0) == IFX_SUCCESS )
                {
                    //  success
                    p_item->pppoe_entry = route.pppoe_info.pppoe_ix;
                    route.update_flags |= IFX_PPA_UPDATE_ROUTING_ENTRY_PPPOE_IX;

                    route.pppoe_mode=1;
                    route.update_flags |= IFX_PPA_UPDATE_ROUTING_ENTRY_PPPOE_MODE;
                }
                else
                    return IFX_EAGAIN;
            }       
        }
    }

    if ( (flags & PPA_F_MTU) )
    {
        route.mtu_info.mtu_ix = p_item->mtu_entry;
        if ( ifx_ppa_drv_get_mtu_entry( &route.mtu_info, 0) == IFX_SUCCESS )
        {
            if ( route.mtu_info.mtu == p_item->mtu )
            {
                //  entry not changed
                route.mtu_info.mtu_ix = p_item->mtu_entry;
                goto PPA_HW_UPDATE_SESSION_EXTRA_MTU_GOON;
            }
            else
            {
                //  entry changed, so delete old first and create new one later
                ifx_ppa_drv_del_mtu_entry( &route.mtu_info, 0);
                p_item->mtu_entry = ~0;
            }
        }
        //  create new MTU entry
        route.mtu_info.mtu = p_item->mtu;
        if ( ifx_ppa_drv_add_mtu_entry( &route.mtu_info, 0) == IFX_SUCCESS )
        {
            //  success
            p_item->mtu_entry = route.mtu_info.mtu_ix;
            route.update_flags |= IFX_PPA_UPDATE_ROUTING_ENTRY_MTU_IX;
        }
        else
            return IFX_EAGAIN;
    }
PPA_HW_UPDATE_SESSION_EXTRA_MTU_GOON:

    if ( (flags & PPA_F_SESSION_OUT_VLAN) )
    {
        route.out_vlan_info.vlan_entry = p_item->out_vlan_entry;
        if ( ifx_ppa_drv_get_outer_vlan_entry(&route.out_vlan_info, 0) == IFX_SUCCESS )
        {
            if ( route.out_vlan_info.vlan_id == p_item->out_vlan_tag )
            {
                //  entry not changed
                goto PPA_HW_UPDATE_SESSION_EXTRA_OUT_VLAN_GOON;
            }
            else
            {
                //  entry changed, so delete old first and create new one later                
                ifx_ppa_drv_del_outer_vlan_entry(&route.out_vlan_info, 0);
                p_item->out_vlan_entry = ~0;
            }
        }
        //  create new OUT VLAN entry
        route.out_vlan_info.vlan_id = p_item->out_vlan_tag;        
        if ( ifx_ppa_drv_add_outer_vlan_entry( &route.out_vlan_info, 0) == IFX_SUCCESS )
        {
            p_item->out_vlan_entry = route.out_vlan_info.vlan_entry;
            route.update_flags |= IFX_PPA_UPDATE_ROUTING_ENTRY_OUT_VLAN_IX;
        }
        else
            return IFX_EAGAIN;

        route.update_flags |= IFX_PPA_UPDATE_ROUTING_ENTRY_OUT_VLAN_INS_EN | IFX_PPA_UPDATE_ROUTING_ENTRY_OUT_VLAN_RM_EN;
    }
PPA_HW_UPDATE_SESSION_EXTRA_OUT_VLAN_GOON:

    route.entry = p_item->routing_entry;
    route.f_new_dscp_enable = p_item->flags & SESSION_VALID_NEW_DSCP;
    if ( (p_item->flags & SESSION_VALID_NEW_DSCP) )
        route.new_dscp = p_item->new_dscp;
    
    route.f_vlan_ins_enable =p_item->flags & SESSION_VALID_VLAN_INS;
    route.new_vci = p_item->new_vci;
    
    route.f_vlan_rm_enable = p_item->flags & SESSION_VALID_VLAN_RM;

    route.f_out_vlan_ins_enable =p_item->flags & SESSION_VALID_OUT_VLAN_INS;
    route.out_vlan_info.vlan_entry = p_item->out_vlan_entry,    
    
    route.f_out_vlan_rm_enable = p_item->flags & SESSION_VALID_OUT_VLAN_RM;
    
    
    ifx_ppa_drv_update_routing_entry(&route, 0);
    return IFX_SUCCESS;
}

void ppa_hw_del_session(struct session_list_item *p_item)
{
    PPE_ROUTING_INFO route={0};    
#if defined(L2TP_CONFIG) && L2TP_CONFIG 
    PPA_L2TP_INFO l2tpinfo={0};    
#endif

    if ( (p_item->flags & SESSION_ADDED_IN_HW) )
    {
        //  when init, these entry values are ~0, the max the number which can be detected by these functions
        route.entry = p_item->routing_entry;
#if defined(CONFIG_IFX_PPA_IF_MIB) && CONFIG_IFX_PPA_IF_MIB
        route.bytes = 0; route.f_hit = 0;
        ifx_ppa_drv_test_and_clear_hit_stat( &route, 0);
        if ( route.f_hit )   
        {               
            uint64_t tmp;
            
            ifx_ppa_drv_get_routing_entry_bytes(&route, 0);
    
            if( (uint32_t )route.bytes >= p_item->last_bytes)   
                tmp = (route.bytes - (uint64_t)p_item->last_bytes);
		    else
                tmp = (route.bytes + (uint64_t)WRAPROUND_SESSION_MIB - (uint64_t)p_item->last_bytes );
            p_item->acc_bytes += tmp;
		    p_item->last_bytes = route.bytes;

            //update mib interfaces
            update_netif_mib(ppa_get_netif_name(p_item->rx_if), tmp, 1, 0);
            if( p_item->br_rx_if ) update_netif_mib(ppa_get_netif_name(p_item->br_rx_if), tmp, 1, 0);
            update_netif_mib(ppa_get_netif_name(p_item->tx_if), tmp, 0, 0);
            if( p_item->br_tx_if ) update_netif_mib(ppa_get_netif_name(p_item->br_tx_if), tmp, 0, 0);
        }
#endif        
        ifx_ppa_drv_del_routing_entry(&route, 0);
        p_item->routing_entry = ~0;

        route.out_vlan_info.vlan_entry = p_item->out_vlan_entry;
        ifx_ppa_drv_del_outer_vlan_entry(&route.out_vlan_info, 0);
        p_item->out_vlan_entry = ~0;

        route.pppoe_info.pppoe_ix = p_item->pppoe_entry;
        ifx_ppa_drv_del_pppoe_entry(&route.pppoe_info, 0);
        p_item->pppoe_entry = ~0;

        route.mtu_info.mtu_ix = p_item->mtu_entry;
        ifx_ppa_drv_del_mtu_entry( &route.mtu_info, 0);
        p_item->mtu_entry = ~0;

        route.src_mac.mac_ix = p_item->src_mac_entry;
        ifx_ppa_drv_del_mac_entry( &route.src_mac, 0);
        p_item->src_mac_entry = ~0;

        route.tnnl_info.tunnel_idx = p_item->tunnel_idx;
        if(p_item->flags & SESSION_TUNNEL_6RD){
            route.tnnl_info.tunnel_type = SESSION_TUNNEL_6RD;
        }else if(p_item->flags & SESSION_TUNNEL_DSLITE){
        	route.tnnl_info.tunnel_type = SESSION_TUNNEL_DSLITE;
        }
        ifx_ppa_drv_del_tunnel_entry(&route.tnnl_info, 0);
       
#if defined(L2TP_CONFIG) && L2TP_CONFIG 
	route.l2tptnnl_info.tunnel_idx = p_item->l2tptunnel_idx;
        l2tpinfo.tunnel_idx = p_item->l2tptunnel_idx;
        if(p_item->flags & SESSION_VALID_PPPOL2TP){
            route.l2tptnnl_info.tunnel_type = SESSION_VALID_PPPOL2TP;
            l2tpinfo.tunnel_type = SESSION_VALID_PPPOL2TP;
	}
        ifx_ppa_drv_del_l2tptunnel_entry(&l2tpinfo, 0);
#endif
        
        p_item->flags &= ~SESSION_ADDED_IN_HW;

        if(ppa_atomic_dec(&g_hw_session_cnt) == 0){
#if defined(CONFIG_CPU_FREQ) || defined(CONFIG_IFX_PMCU) || defined(CONFIG_IFX_PMCU_MODULE)
            ifx_ppa_pwm_deactivate_module();
#endif
        }
    }
}


uint32_t is_ip_zero(IP_ADDR_C *ip)
{
	if(ip->f_ipv6){
		return ((ip->ip.ip6[0] | ip->ip.ip6[1] | ip->ip.ip6[2] | ip->ip.ip6[3]) == 0);
	}else{
		return (ip->ip.ip == 0);
	}
}

uint32_t ip_equal(IP_ADDR_C *dst_ip, IP_ADDR_C *src_ip)
{
	if(dst_ip->f_ipv6){
		return (((dst_ip->ip.ip6[0] ^ src_ip->ip.ip6[0] ) |
			     (dst_ip->ip.ip6[1] ^ src_ip->ip.ip6[1] ) |
			     (dst_ip->ip.ip6[2] ^ src_ip->ip.ip6[2] ) |
			     (dst_ip->ip.ip6[3] ^ src_ip->ip.ip6[3] )) == 0);
	}else{
		return ( (dst_ip->ip.ip ^ src_ip->ip.ip) == 0);
	}
}

unsigned int is_ip_allbit1(IP_ADDR_C *ip)
{
    if(ip->f_ipv6){
		return ((~ip->ip.ip6[0] | ~ip->ip.ip6[1] | ~ip->ip.ip6[2] | ~ip->ip.ip6[3]) == 0);
	}else{
		return (~ip->ip.ip == 0);
	}
}



/*
 *  multicast routing operation
 */

int32_t __ppa_lookup_mc_group(IP_ADDR_C *ip_mc_group, IP_ADDR_C *src_ip, struct mc_group_list_item **pp_item)
{
    uint32_t idx;
    PPA_HLIST_NODE *node;
    struct mc_group_list_item *p_mc_item = NULL;
    

    if( !pp_item ) 
        ppa_debug(DBG_ENABLE_MASK_ASSERT,"pp_item == NULL\n");

    idx = SESSION_LIST_MC_HASH_VALUE(ip_mc_group->ip.ip);
    ppa_hlist_for_each_entry(p_mc_item,node,&g_mc_group_list_hash_table[idx],mc_hlist){
        if(ip_equal(&p_mc_item->ip_mc_group, ip_mc_group)){//mc group ip match
			if(ip_equal(&p_mc_item->source_ip, src_ip)){//src ip match
			    *pp_item = p_mc_item;
				return IFX_PPA_SESSION_EXISTS;
			}else{
				if(is_ip_zero(&p_mc_item->source_ip) || is_ip_zero(src_ip)){
					*pp_item = NULL;
                    return IFX_PPA_MC_SESSION_VIOLATION;
				}
			}
			
		}
    }

    return IFX_PPA_SESSION_NOT_ADDED;
}



/*
  *  delete mc groups
  *  if SSM flag (Sourceip Specific Mode) is 1 , then match both dest ip and source ip
  *  if SSM flag is 0, then only match dest ip
  */
void ppa_delete_mc_group(PPA_MC_GROUP *ppa_mc_entry)
{
    struct mc_group_list_item *p_mc_item = NULL;
    uint32_t idx;
    PPA_HLIST_NODE *node, *node_next;

    idx = SESSION_LIST_MC_HASH_VALUE(ppa_mc_entry->ip_mc_group.ip.ip);

    ppa_mc_get_htable_lock();

    ppa_hlist_for_each_entry_safe(p_mc_item,node,node_next,&g_mc_group_list_hash_table[idx],mc_hlist){
		if(ip_equal(&p_mc_item->ip_mc_group, &ppa_mc_entry->ip_mc_group)){//mc group ip match
		    if(!ppa_mc_entry->SSM_flag || ip_equal(&p_mc_item->source_ip, &ppa_mc_entry->source_ip) ){
                ppa_remove_mc_group_item(p_mc_item);
                ppa_free_mc_group_list_item(p_mc_item);
                if(ppa_mc_entry->SSM_flag){//src ip specific, so only one can get matched
                    break;
                }
            }
		}        
    }

    ppa_mc_release_htable_lock();
    return;
}

static int32_t ppa_mc_itf_get(char *itf, struct netif_info **pp_netif_info)
{
    if(!itf){
        return IFX_FAILURE;
    }

    if( ppa_netif_update(NULL, itf) != IFX_SUCCESS )
        return IFX_FAILURE;

    return ppa_netif_lookup(itf, pp_netif_info);

}

static int32_t ppa_mc_check_src_itf(char *itf, struct mc_group_list_item *p_item)
{
    struct netif_info *p_netif_info = NULL;

    if(itf != NULL){
        if(ppa_mc_itf_get(itf, &p_netif_info) != IFX_SUCCESS
            || !(p_netif_info->flags & NETIF_PHYS_PORT_GOT) ){
            return IFX_FAILURE;
        }
        p_item->src_netif = p_netif_info->netif;
        if(p_netif_info->flags & NETIF_VLAN_INNER){
            p_item->flags |= SESSION_VALID_VLAN_RM;
        }
        if(p_netif_info->flags & NETIF_VLAN_OUTER){
            p_item->flags |= SESSION_VALID_OUT_VLAN_RM;
        }
        if(p_netif_info->netif->type == ARPHRD_SIT){//6rd Device
			p_item->flags |= SESSION_TUNNEL_6RD;
        }
		if(p_netif_info->netif->type == ARPHRD_TUNNEL6){//dslite Device
			p_item->flags |= SESSION_TUNNEL_DSLITE;
        }
        ppa_netif_put(p_netif_info);
    }
    else{
        p_item->src_netif = NULL;
    }

    return IFX_SUCCESS;
}

static int32_t ppa_mc_check_dst_itf(PPA_MC_GROUP *p_mc_group, struct mc_group_list_item *p_item)
{
    int i = 0;
    int first_dst = 1;
    struct netif_info *p_netif_info = NULL;
    struct netif_info *p_br_netif   = NULL;
    PPA_NETIF *br_dev;
    
    for(i = 0; i < p_mc_group->num_ifs; i ++){
        if(ppa_mc_itf_get(p_mc_group->array_mem_ifs[i].ifname, &p_netif_info) != IFX_SUCCESS
            || !(p_netif_info->flags & NETIF_PHYS_PORT_GOT) ){
            return IFX_FAILURE;
        }

        if(first_dst){
            first_dst = 0;
            if(!p_item->bridging_flag){//route mode, need replace the src mac address
                //if the dst device is in the bridge, we try to get bridge's src mac address.  
                br_dev = ppa_get_br_dev(p_netif_info->netif);
                if(br_dev != NULL && 
                    ppa_mc_itf_get(br_dev->name,&p_br_netif) == IFX_SUCCESS){
                       p_item->src_mac_entry = p_br_netif->mac_entry;
                       ppa_netif_put(p_br_netif);
                }
                else{
                       p_item->src_mac_entry = p_netif_info->mac_entry;
                }
                p_item->flags |= SESSION_VALID_SRC_MAC;
               
            }
            //if no vlan,reset value to zero in case it is update
            p_item->new_vci = p_netif_info->flags & NETIF_VLAN_INNER ? p_netif_info->inner_vid : 0;
            p_item->out_vlan_tag = p_netif_info->flags & NETIF_VLAN_OUTER ? p_netif_info->outer_vid : ~0;
            p_item->flags |= p_netif_info->flags & NETIF_VLAN_INNER ? SESSION_VALID_VLAN_INS : 0;
            p_item->flags |= p_netif_info->flags & NETIF_VLAN_OUTER ? SESSION_VALID_OUT_VLAN_INS : 0;

        }
        else{
            if((p_netif_info->flags & NETIF_VLAN_INNER && !(p_item->flags & SESSION_VALID_VLAN_INS))
                || (!(p_netif_info->flags & NETIF_VLAN_INNER) && p_item->flags & SESSION_VALID_VLAN_INS)
                || (p_netif_info->flags & NETIF_VLAN_OUTER && !(p_item->flags & SESSION_VALID_OUT_VLAN_INS))
                || (!(p_netif_info->flags & NETIF_VLAN_OUTER) && p_item->flags & SESSION_VALID_OUT_VLAN_INS)
                || ((p_item->flags & SESSION_VALID_VLAN_INS) && p_item->new_vci != p_netif_info->inner_vid) 
                || ((p_item->flags & SESSION_VALID_OUT_VLAN_INS) && p_item->out_vlan_tag != p_netif_info->outer_vid)
               ) {
                goto DST_VLAN_FAIL;
            }   
        }

        p_item->dest_ifid |= 1 << p_netif_info->phys_port; 
        p_item->netif[i] = p_netif_info->netif;
        p_item->ttl[i]   = p_mc_group->array_mem_ifs[i].ttl;
        p_item->if_mask |= 1 << i; 
        ppa_netif_put(p_netif_info);
        
    }
    p_item->num_ifs = p_mc_group->num_ifs;

    return IFX_SUCCESS;

DST_VLAN_FAIL:
    ppa_netif_put(p_netif_info);
    return IFX_FAILURE;
}


int32_t ppa_mc_group_setup(PPA_MC_GROUP *p_mc_group, struct mc_group_list_item *p_item, uint32_t flags)
{
   
    p_item->bridging_flag = p_mc_group->bridging_flag;
    if(!p_item->bridging_flag){
        p_item->flags |=  SESSION_VALID_PPPOE;   //  firmware will remove PPPoE header, if and only if the PPPoE header available
    }
    p_item->SSM_flag = p_mc_group->SSM_flag;

    //check src interface
    if(ppa_mc_check_src_itf(p_mc_group->src_ifname, p_item) != IFX_SUCCESS)
        return IFX_FAILURE;

    //check dst interface
    if(ppa_mc_check_dst_itf(p_mc_group, p_item) != IFX_SUCCESS){
        return IFX_FAILURE;
    }

    //  multicast  address
    ppa_memcpy( &p_item->ip_mc_group, &p_mc_group->ip_mc_group, sizeof(p_mc_group->ip_mc_group ) ) ;
    //  source ip address
    ppa_memcpy( &p_item->source_ip, &p_mc_group->source_ip, sizeof(p_mc_group->source_ip) ) ;
    p_item->dslwan_qid = p_mc_group->dslwan_qid;

    //update info give by hook function (extra info: e.g. extra vlan )
    if ( (flags & PPA_F_SESSION_VLAN) )
    {
        if ( p_mc_group->vlan_insert )
        {
            p_item->flags |= SESSION_VALID_VLAN_INS;
            p_item->new_vci = ((uint32_t)p_mc_group->vlan_prio << 13) | ((uint32_t)p_mc_group->vlan_cfi << 12) | (uint32_t)p_mc_group->vlan_id;
        }

        if ( p_mc_group->vlan_remove )
            p_item->flags |= SESSION_VALID_VLAN_RM;
        
    }     

    if ( (flags & PPA_F_SESSION_OUT_VLAN) )
    {
        if ( p_mc_group->out_vlan_insert )
        {
            p_item->flags |= SESSION_VALID_OUT_VLAN_INS;
            p_item->out_vlan_tag = p_mc_group->out_vlan_tag;
        }

        if ( p_mc_group->out_vlan_remove )
            p_item->flags |= SESSION_VALID_OUT_VLAN_RM;
        
    }
   
    if ( (flags & PPA_F_SESSION_NEW_DSCP) )
    {
        if ( p_mc_group->new_dscp_en )
        {
            p_item->new_dscp = p_mc_group->new_dscp;
            p_item->flags |= SESSION_VALID_NEW_DSCP;
        }
        
    }

    return IFX_SUCCESS;
    
}

/*
    ppa_mc_group_checking: check whether it is valid acceleration session. the result value :
    1) if not found any valid downstream interface, includes num_ifs is zero: return IFX_FAILURE
    2) if downstream interface's VLAN tag not complete same: return IFX_FAILURE
    3) if p_item is NULL: return IFX_ENOMEM;
    
    
*/
int32_t ppa_mc_group_checking(PPA_MC_GROUP *p_mc_group, struct mc_group_list_item *p_item, uint32_t flags)
{
    struct netif_info *p_netif_info;
    uint32_t bit;
    uint32_t idx;
    uint32_t i, bfAccelerate=1, tmp_flag = 0, tmp_out_vlan_tag=0;
    uint16_t  tmp_new_vci=0, bfFirst = 1 ;
    uint8_t netif_mac[PPA_ETH_ALEN], tmp_mac[PPA_ETH_ALEN];
    
    if ( !p_item )
        return IFX_ENOMEM;

    //before updating p_item, need to clear some previous values, but cannot memset all p_item esp for p_item's next pointer.
    p_item->dest_ifid = 0;
    //p_item->flags = 0;  //don't simple clear original flag value, esp for flag "SESSION_ADDED_IN_HW"
    p_item->if_mask = 0;
    p_item->new_dscp = 0;
    
    p_item->bridging_flag = p_mc_group->bridging_flag;

    for ( i = 0, bit = 1, idx = 0; i < PPA_MAX_MC_IFS_NUM && idx < p_mc_group->num_ifs; i++, bit <<= 1 )
    {
        if ( p_mc_group->if_mask & bit)
        {
            ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP,"group checking itf: %s\n", p_mc_group->array_mem_ifs[i].ifname);
            if ( ppa_netif_lookup(p_mc_group->array_mem_ifs[i].ifname, &p_netif_info) == IFX_SUCCESS )
            {
                //  dest interface
                if ( ppa_netif_update(NULL, p_mc_group->array_mem_ifs[i].ifname) != IFX_SUCCESS
                    || !(p_netif_info->flags & NETIF_PHYS_PORT_GOT) )
                {
                    ppa_netif_put(p_netif_info);
                    ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "Warning: No PHYS found for interface %s\n", p_mc_group->array_mem_ifs[i].ifname);
                    bfAccelerate = 0;
                    break;
                }
                if ( bfFirst )
                {  /* keep the first interface's flag. Make sure all interface's vlan action should be same, otherwise PPE FW cannot do it */
                    tmp_flag = p_netif_info->flags;
                    tmp_new_vci = p_netif_info->inner_vid;
                    tmp_out_vlan_tag = p_netif_info->outer_vid;
                    //if the multicast entry has multiple output interface, make sure they must has same MAC address
                    //the devices in the bridge will get the bridge's mac address.
                    ppa_get_netif_hwaddr(p_netif_info->netif,netif_mac, 1);
                    bfFirst = 0;
                }
                else
                {
                    if ( ( tmp_flag & ( NETIF_VLAN_OUTER | NETIF_VLAN_INNER ) )  != ( p_netif_info->flags & ( NETIF_VLAN_OUTER | NETIF_VLAN_INNER ) ) )
                    {
                        bfAccelerate = 0;
                        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "ppa_add_mc_group not same flag (%0x_%0x)\n", tmp_flag & (NETIF_VLAN_OUTER | NETIF_VLAN_INNER ), p_netif_info->flags & (NETIF_VLAN_OUTER | NETIF_VLAN_INNER) );
                        ppa_netif_put(p_netif_info);
                        break;
                    }
                    else if ( tmp_out_vlan_tag != p_netif_info->outer_vid )
                    {
                        bfAccelerate = 0;
                        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "ppa_add_mc_group not same out vlan tag (%0x_%0x)\n", tmp_out_vlan_tag, p_netif_info->outer_vid);
                        ppa_netif_put(p_netif_info);
                        break;
                    }
                    else if ( tmp_new_vci != p_netif_info->inner_vid )
                    {
                        bfAccelerate = 0;
                        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "ppa_add_mc_group not same inner vlan (%0x_%0x)\n", tmp_new_vci , p_netif_info->inner_vid);
                        ppa_netif_put(p_netif_info);
                        break;
                    }

                    ppa_get_netif_hwaddr(p_netif_info->netif,tmp_mac, 1);
                    if(ppa_memcmp(netif_mac, tmp_mac, sizeof(tmp_mac))){
                        bfAccelerate = 0;
                        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "ppa_add_mc_group not same mac address\n");
                        ppa_netif_put(p_netif_info);
                        break;
                    }
                }

                p_item->dest_ifid |= 1 << p_netif_info->phys_port;  //  sgh change xuliang's original architecture, but for unicast routing/bridging, still keep old one

                if( !(p_netif_info->flags & NETIF_MAC_ENTRY_CREATED ) )
                    ppa_debug(DBG_ENABLE_MASK_ASSERT,"ETH physical interface must have MAC address\n");
                p_item->src_mac_entry = p_netif_info->mac_entry;
                if ( !p_mc_group->bridging_flag )
                    p_item->flags |= SESSION_VALID_SRC_MAC;
                else //otherwise clear this bit in case it is set beofre calling this API
                {
                    p_item->flags &= ~SESSION_VALID_SRC_MAC;
                }
                
                p_item->netif[idx] = p_netif_info->netif;
                p_item->ttl[idx]   = p_mc_group->array_mem_ifs[i].ttl;
                p_item->if_mask |= 1 << idx;

                ppa_netif_put(p_netif_info);

                idx++;
            }
            else
            {
                bfAccelerate = 0;
                ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "ppa_add_mc_group cannot find the interface:%s)\n", p_mc_group->array_mem_ifs[i].ifname);
                break;
            }
        }
    }

    if ( bfAccelerate == 0 || idx == 0 || (!p_mc_group->bridging_flag && !(p_item->flags & SESSION_VALID_SRC_MAC)) )
    {        
        return IFX_FAILURE;
    }

    //  VLAN
    if( !(tmp_flag & NETIF_VLAN_CANT_SUPPORT ))
        ppa_debug(DBG_ENABLE_MASK_ASSERT,"MC processing can support two layers of VLAN only\n");
    if ( (tmp_flag & NETIF_VLAN_OUTER) )
    {
        p_item->out_vlan_tag = tmp_out_vlan_tag;
        p_item->flags |= SESSION_VALID_OUT_VLAN_INS;
        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "set SESSION_VALID_OUT_VLAN_INS:%x_%x\n", p_item->out_vlan_tag, tmp_new_vci);
    }
    else //otherwise clear this bit in case it is set beofre calling this API
    {
        p_item->flags &= ~SESSION_VALID_OUT_VLAN_INS;
        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "unset SESSION_VALID_OUT_VLAN_INS\n");
    }
    
    if ( (tmp_flag & NETIF_VLAN_INNER) )
    {
        p_item->new_vci = tmp_new_vci;

        p_item->flags |= SESSION_VALID_VLAN_INS;
        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "set SESSION_VALID_VLAN_INS:%x\n", p_item->new_vci);
    }
    else //otherwise clear this bit in case it is set beofre calling this API
    {
        p_item->flags &= ~SESSION_VALID_VLAN_INS;
        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "unset SESSION_VALID_VLAN_INS\n");
    }

   //PPPOE
    if ( !p_mc_group->bridging_flag )
        p_item->flags |= SESSION_VALID_PPPOE;   //  firmware will remove PPPoE header, if and only if the PPPoE header available
    else //otherwise clear this bit in case it is set beofre calling this API
    {
        p_item->flags &= ~SESSION_VALID_PPPOE;
    }

    //  multicast  address
    ppa_memcpy( &p_item->ip_mc_group, &p_mc_group->ip_mc_group, sizeof(p_mc_group->ip_mc_group ) ) ;
    //  source ip address
    ppa_memcpy( &p_item->source_ip, &p_mc_group->source_ip, sizeof(p_mc_group->source_ip) ) ;

    if ( p_mc_group->src_ifname && ppa_netif_lookup(p_mc_group->src_ifname, &p_netif_info) == IFX_SUCCESS )
    {
        //  src interface

        if ( ppa_netif_update(NULL, p_mc_group->src_ifname) == IFX_SUCCESS
            && (p_netif_info->flags & NETIF_PHYS_PORT_GOT) )
        {
            //  PPPoE
            if ( !p_mc_group->bridging_flag && (p_netif_info->flags & NETIF_PPPOE) )
                p_item->flags |= SESSION_VALID_PPPOE;
            else //otherwise clear this bit in case it is set beofre calling this API
            {
                p_item->flags &= ~SESSION_VALID_PPPOE;
            }

            //  VLAN
            if( !(p_netif_info->flags & NETIF_VLAN_CANT_SUPPORT ))
                ppa_debug(DBG_ENABLE_MASK_ASSERT, "MC processing can support two layers of VLAN only\n");
            if ( (p_netif_info->flags & NETIF_VLAN_OUTER) )
                p_item->flags |= SESSION_VALID_OUT_VLAN_RM;
            else //otherwise clear this bit in case it is set beofre calling this API
            {
                p_item->flags &= ~SESSION_VALID_OUT_VLAN_RM;
            }
            
            if ( (p_netif_info->flags & NETIF_VLAN_INNER) )
                p_item->flags |= SESSION_VALID_VLAN_RM;
             else //otherwise clear this bit in case it is set beofre calling this API
            {
                p_item->flags &= ~SESSION_VALID_VLAN_RM;
            }

			if(p_netif_info->netif->type == ARPHRD_SIT){//6rd Device
				p_item->flags |= SESSION_TUNNEL_6RD;
			}else{
				p_item->flags &= ~SESSION_TUNNEL_6RD;
			}

			if(p_netif_info->netif->type == ARPHRD_TUNNEL6){//dslite Device
				p_item->flags |= SESSION_TUNNEL_DSLITE;
			}else{
				p_item->flags &= ~SESSION_TUNNEL_DSLITE;
			}
             
        }
        else  /*not allowed to support non-physical interfaces,like bridge */
        {
            ppa_netif_put(p_netif_info);
            return IFX_FAILURE;
        }
        p_item->src_netif = p_netif_info->netif;

        ppa_netif_put(p_netif_info);
    }

    p_item->num_ifs = idx;

    p_item->dslwan_qid = p_mc_group->dslwan_qid;

    //force update some status by hook itself
    if ( (flags & PPA_F_SESSION_VLAN) )
    {
        if ( p_mc_group->vlan_insert )
        {
            p_item->flags |= SESSION_VALID_VLAN_INS;
            p_item->new_vci = ((uint32_t)p_mc_group->vlan_prio << 13) | ((uint32_t)p_mc_group->vlan_cfi << 12) | (uint32_t)p_mc_group->vlan_id;
        }
        else
        {
            p_item->flags &= ~SESSION_VALID_VLAN_INS;
            p_item->new_vci = 0;
        }

        if ( p_mc_group->vlan_remove )
            p_item->flags |= SESSION_VALID_VLAN_RM;
        else
            p_item->flags &= ~SESSION_VALID_VLAN_RM;
    }     

    if ( (flags & PPA_F_SESSION_OUT_VLAN) )
    {
        if ( p_mc_group->out_vlan_insert )
        {
            p_item->flags |= SESSION_VALID_OUT_VLAN_INS;
            p_item->out_vlan_tag = p_mc_group->out_vlan_tag;
        }
        else
        {
            p_item->flags &= ~SESSION_VALID_OUT_VLAN_INS;
            p_item->out_vlan_tag = 0;
        }

        if ( p_mc_group->out_vlan_remove )
            p_item->flags |= SESSION_VALID_OUT_VLAN_RM;
        else
            p_item->flags &= ~SESSION_VALID_OUT_VLAN_RM;
    }
   
    if ( (flags & PPA_F_SESSION_NEW_DSCP) )
    {
        if ( p_mc_group->new_dscp_en )
        {
            p_item->new_dscp = p_mc_group->new_dscp;
            p_item->flags |= SESSION_VALID_NEW_DSCP;
        }
        else
            p_item->new_dscp &= ~SESSION_VALID_NEW_DSCP;
    }
    
    ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "mc flag:%x\n", p_item->flags);
    return IFX_SUCCESS;
}    

int32_t ppa_add_mc_group(PPA_MC_GROUP *p_mc_group, struct mc_group_list_item **pp_item, uint32_t flags)
{
    struct mc_group_list_item *p_item;

    p_item = ppa_alloc_mc_group_list_item();
    if ( !p_item )
        return IFX_ENOMEM;
     
    //if( ppa_mc_group_checking(p_mc_group, p_item, flags ) !=IFX_SUCCESS )
    if( ppa_mc_group_setup(p_mc_group, p_item, flags ) != IFX_SUCCESS )
    {
        ppa_mem_cache_free(p_item, g_mc_group_item_cache);
        return IFX_FAILURE;
    }
    
    ppa_insert_mc_group_item(p_item);

    *pp_item = p_item;

    return IFX_SUCCESS;
}

int32_t ppa_update_mc_group_extra(PPA_SESSION_EXTRA *p_extra, struct mc_group_list_item *p_item, uint32_t flags)
{
    if ( (flags & PPA_F_SESSION_NEW_DSCP) )
    {
        if ( p_extra->dscp_remark )
        {
            p_item->flags |= SESSION_VALID_NEW_DSCP;
            p_item->new_dscp = p_extra->new_dscp;
        }
        else
            p_item->flags &= ~SESSION_VALID_NEW_DSCP;
    }

    if ( (flags & PPA_F_SESSION_VLAN) )
    {
        if ( p_extra->vlan_insert )
        {
            p_item->flags |= SESSION_VALID_VLAN_INS;
            p_item->new_vci = ((uint32_t)p_extra->vlan_prio << 13) | ((uint32_t)p_extra->vlan_cfi << 12) | p_extra->vlan_id;
        }
        else
        {
            p_item->flags &= ~SESSION_VALID_VLAN_INS;
            p_item->new_vci = 0;
        }

        if ( p_extra->vlan_remove )
            p_item->flags |= SESSION_VALID_VLAN_RM;
        else
            p_item->flags &= ~SESSION_VALID_VLAN_RM;
    }

    if ( (flags & PPA_F_SESSION_OUT_VLAN) )
    {
        if ( p_extra->out_vlan_insert )
        {
            p_item->flags |= SESSION_VALID_OUT_VLAN_INS;
            p_item->out_vlan_tag = p_extra->out_vlan_tag;
        }
        else
        {
            p_item->flags &= ~SESSION_VALID_OUT_VLAN_INS;
            p_item->out_vlan_tag = 0;
        }

        if ( p_extra->out_vlan_remove )
            p_item->flags |= SESSION_VALID_OUT_VLAN_RM;
        else
            p_item->flags &= ~SESSION_VALID_OUT_VLAN_RM;
    }

    if ( p_extra->dslwan_qid_remark )
        p_item->dslwan_qid = p_extra->dslwan_qid;

    return IFX_SUCCESS;
}

void ppa_remove_mc_group(struct mc_group_list_item *p_item)
{
    ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "ppa_remove_mc_group:  remove %d from PPA\n", p_item->mc_entry);
    ppa_remove_mc_group_item(p_item);

    ppa_free_mc_group_list_item(p_item);
}

int32_t ppa_mc_group_start_iteration(uint32_t *ppos, struct mc_group_list_item **pp_item)
{
    struct mc_group_list_item *p = NULL;
    PPA_HLIST_NODE *node;
    int idx;
    uint32_t l;

    l = *ppos + 1;

    ppa_lock_get(&g_mc_group_list_lock);

    
    if( !ifx_ppa_is_init() )
    {
       *pp_item = NULL;
       return IFX_FAILURE; 
    }

    for ( idx = 0; l && idx < NUM_ENTITY(g_mc_group_list_hash_table); idx++ )
    {
        ppa_hlist_for_each_entry(p, node, &g_mc_group_list_hash_table[idx],mc_hlist){
            if ( !--l )
                break;
        }
    }

    if ( l == 0 && p )
    {
        ++*ppos;
        *pp_item = p;
        return IFX_SUCCESS;
    }
    else
    {
        *pp_item = NULL;
        return IFX_FAILURE;
    }
}

int32_t ppa_mc_group_iterate_next(uint32_t *ppos, struct mc_group_list_item **pp_item)
{
    uint32_t idx;

    if ( *pp_item == NULL )
        return IFX_FAILURE;

    if ( (*pp_item)->mc_hlist.next != NULL )
    {
        ++*ppos;
        *pp_item = ppa_hlist_entry((*pp_item)->mc_hlist.next, struct mc_group_list_item, mc_hlist);
        return IFX_SUCCESS;
    }
    else
    {
        for ( idx = SESSION_LIST_MC_HASH_VALUE((*pp_item)->ip_mc_group.ip.ip) + 1;
              idx < NUM_ENTITY(g_mc_group_list_hash_table);
              idx++ )
            if ( g_mc_group_list_hash_table[idx].first != NULL )
            {
                ++*ppos;
                *pp_item = ppa_hlist_entry(g_mc_group_list_hash_table[idx].first,struct mc_group_list_item, mc_hlist);
                return IFX_SUCCESS;
            }
        *pp_item = NULL;
        return IFX_FAILURE;
    }
}

#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG

static INLINE void ppa_remove_capwap_group_item(struct capwap_group_list_item *p_item)
{
   ppa_list_del(&p_item->capwap_list);
}

INLINE void ppa_free_capwap_group_list_item(struct capwap_group_list_item *p_item)
{
    ppa_mem_cache_free(p_item, g_capwap_group_item_cache);
}



void ppa_remove_capwap_group(struct capwap_group_list_item *p_item)
{
    ppa_remove_capwap_group_item(p_item);
    ppa_free_capwap_group_list_item(p_item);
}




static int32_t ppa_capwap_check_dst_itf(PPA_CMD_CAPWAP_INFO *p_capwap_group,struct capwap_group_list_item *p_item)
{
    int i = 0;
    struct netif_info *p_netif_info = NULL;
    
    for(i = 0; i < p_capwap_group->num_ifs; i ++){
        //if(p_capwap_group->lan_ifname[i] != NULL)
        if(p_capwap_group->lan_ifname[i][0] != '\0')
        {
            if(ppa_mc_itf_get(p_capwap_group->lan_ifname[i], &p_netif_info) != IFX_SUCCESS || !(p_netif_info->flags & NETIF_PHYS_PORT_GOT) ){
               return IFX_FAILURE;
            }
        //if( (p_capwap_group->src_mac[0] == 0) && (p_capwap_group->src_mac[1] == 0) && (p_capwap_group->src_mac[2] == 0) && (p_capwap_group->src_mac[3] == 0) && (p_capwap_group->src_mac[4] == 0) && (p_capwap_group->src_mac[5] == 0) && (p_capwap_group->src_mac[6] == 0))
            if(ppa_memcmp(p_capwap_group->src_mac, "\0\0\0\0\0", PPA_ETH_ALEN) == 0)
            {
               ppa_memcpy(p_capwap_group->src_mac,p_netif_info->mac,PPA_ETH_ALEN);
               ppa_memcpy(p_item->src_mac,p_netif_info->mac,PPA_ETH_ALEN);
            }
            p_capwap_group->dest_ifid |= 1 << p_netif_info->phys_port; 
            p_item->netif[i] = p_netif_info->netif;
            p_item->if_mask |= 1 << i; 
            ppa_netif_put(p_netif_info);
        }
        else
        {
            p_capwap_group->dest_ifid |= 1 << p_capwap_group->phy_port_id[i]; 
            p_item->if_mask |= 1 << i; 
            ppa_memcpy(p_item->src_mac,p_capwap_group->src_mac,PPA_ETH_ALEN);
            p_item-> phy_port_id[i] = p_capwap_group->phy_port_id[i];
        }
        
    } //End of for loop
    p_item->num_ifs = p_capwap_group->num_ifs;
    
    return IFX_SUCCESS;


}




uint16_t  checksum(uint8_t* ip, int len)
{
   uint32_t chk_sum = 0;  

   while(len > 1){
      //chk_sum += *((uint16_t*) ip)++;
      chk_sum += *((uint16_t*) ip);
      ip +=2;
      if(chk_sum & 0x80000000)
         chk_sum = (chk_sum & 0xFFFF) + (chk_sum >> 16);
         len -= 2;
   }

   if(len)      
      chk_sum += (uint16_t) *(uint8_t *)ip;
     
   while(chk_sum>>16)
      chk_sum = (chk_sum & 0xFFFF) + (chk_sum >> 16);

   return ~chk_sum;

}


static INLINE struct capwap_group_list_item *ppa_alloc_capwap_group_list_item(void)
{
    struct capwap_group_list_item *p_item;

    p_item = ppa_mem_cache_alloc(g_capwap_group_item_cache);
    if ( p_item )
    {
        ppa_memset(p_item, 0, sizeof(*p_item));
    }
    return p_item;
}

int32_t ppa_capwap_group_setup(PPA_CMD_CAPWAP_INFO *p_capwap_group, struct capwap_group_list_item *p_item)
{
    struct capwap_iphdr cw_hdr;

    struct udp_ipv4_psedu_hdr pseduo_hdr;

    ppa_memset(&pseduo_hdr, 0, sizeof(pseduo_hdr) );
    ppa_memset(&cw_hdr, 0, sizeof(cw_hdr) );
    
    //check dst interface
    if(ppa_capwap_check_dst_itf(p_capwap_group,p_item) != IFX_SUCCESS){
        return IFX_FAILURE;
    }
    p_item->directpath_portid = p_capwap_group->directpath_portid;
    p_item->qid = p_capwap_group->qid;
    //Ethernet header 
    ppa_memcpy(p_item->dst_mac,p_capwap_group->dst_mac,PPA_ETH_ALEN);
   

    //IP header
    p_item->tos = p_capwap_group->tos;
    p_item->ttl = p_capwap_group->ttl;
#if 0
    p_item->ver = IP_VERSION;
    p_item->head_len = IP_HEADER_LEN;
    p_item->total_len = IP_TOTAL_LEN;  
    p_item->tos = p_capwap_group->tos;
    p_item->ident = IP_IDENTIFIER;
    p_item->ip_flags = IP_FLAGS; 
    p_item->ip_frag_off = IP_FRAG_OFF;
    p_item->ttl = p_capwap_group->ttl;
    p_item->proto = IP_PROTO;
    p_item->ip_chksum, 
#endif
    cw_hdr.version = IP_VERSION;
    cw_hdr.ihl = IP_HEADER_LEN;
    cw_hdr.tos = p_capwap_group->tos;
    cw_hdr.tot_len = IP_TOTAL_LEN;  
    cw_hdr.id = IP_IDENTIFIER;
    cw_hdr.frag_off = IP_FRAG_OFF;
    cw_hdr.ttl = p_capwap_group->ttl;
    cw_hdr.protocol = IP_PROTO_UDP;
    cw_hdr.check = 0; 
    cw_hdr.saddr = p_capwap_group->source_ip.ip.ip;
    cw_hdr.daddr = p_capwap_group->dest_ip.ip.ip;

     p_capwap_group->ip_chksum = checksum((uint8_t *)(&cw_hdr),IP_HEADER_LEN*4);
    
    //  source ip address
    ppa_memcpy( &p_item->source_ip, &p_capwap_group->source_ip, sizeof(p_capwap_group->source_ip) ) ;

    //  destination ip address
    ppa_memcpy( &p_item->dest_ip, &p_capwap_group->dest_ip, sizeof(p_capwap_group->dest_ip) ) ;
    
    // UDP header
    pseduo_hdr.saddr = p_capwap_group->source_ip.ip.ip;
    pseduo_hdr.daddr = p_capwap_group->dest_ip.ip.ip;
    pseduo_hdr.protocol = IP_PROTO_UDP;
    pseduo_hdr.udp_length = IP_PSEUDO_UDP_LENGTH;
    pseduo_hdr.src_port = p_capwap_group->source_port;
    pseduo_hdr.dst_port = p_capwap_group->dest_port;
    pseduo_hdr.length = UDP_TOTAL_LENGTH;
    pseduo_hdr.checksum = 0;
    
    p_capwap_group->udp_chksum = checksum((uint8_t *)(&pseduo_hdr),sizeof(struct udp_ipv4_psedu_hdr));
    
    p_item->source_port = p_capwap_group->source_port;
    p_item->dest_port = p_capwap_group->dest_port;
  
    //UDP checksum


    // CAPWAP RID WBID T
    p_item->rid = p_capwap_group->rid;
    p_item->wbid = p_capwap_group->wbid;
    p_item->t_flag = p_capwap_group->t_flag;

    p_item->max_frg_size = p_capwap_group->max_frg_size;

    return IFX_SUCCESS;
    
}

int32_t __ppa_lookup_capwap_group_add(IP_ADDR_C *src_ip, IP_ADDR_C *dst_ip,uint8_t directpath_portid, struct capwap_group_list_item **pp_item)
{
    PPA_LIST_NODE *node;
    struct capwap_group_list_item *p_capwap_item = NULL;
    

    if( !pp_item ) 
        ppa_debug(DBG_ENABLE_MASK_ASSERT,"pp_item == NULL\n");

    ppa_list_for_each(node, &capwap_list_head) {
    p_capwap_item = ppa_list_entry(node, struct capwap_group_list_item , capwap_list);
    if( ip_equal(&p_capwap_item->source_ip, src_ip) && ip_equal(&p_capwap_item->dest_ip, dst_ip)  && (p_capwap_item->directpath_portid == directpath_portid)) {
			    *pp_item = p_capwap_item;
				return IFX_PPA_CAPWAP_EXISTS;
			}
    else
         if( (ip_equal(&p_capwap_item->source_ip, src_ip)) && (ip_equal(&p_capwap_item->dest_ip, dst_ip))) {
			    *pp_item = p_capwap_item;
				return IFX_PPA_CAPWAP_EXISTS;
			}
         else
            if( p_capwap_item->directpath_portid == directpath_portid) {
			    *pp_item = p_capwap_item;
				return IFX_PPA_CAPWAP_EXISTS;
			}

    }

    return IFX_PPA_CAPWAP_NOT_ADDED;
}


int32_t __ppa_lookup_capwap_group(IP_ADDR_C *src_ip, IP_ADDR_C *dst_ip,uint8_t directpath_portid, struct capwap_group_list_item **pp_item)
{
    PPA_LIST_NODE *node;
    struct capwap_group_list_item *p_capwap_item = NULL;
    

    if( !pp_item ) 
        ppa_debug(DBG_ENABLE_MASK_ASSERT,"pp_item == NULL\n");

    ppa_list_for_each(node, &capwap_list_head) {
    p_capwap_item = ppa_list_entry(node, struct capwap_group_list_item , capwap_list);
    if(ip_equal(&p_capwap_item->source_ip, src_ip)){//capwap group source ip match
			if(ip_equal(&p_capwap_item->dest_ip, dst_ip)){//capwap group destination ip match
			if(p_capwap_item->directpath_portid == directpath_portid){//capwap group directpath portId match
			    *pp_item = p_capwap_item;
				return IFX_PPA_CAPWAP_EXISTS;
			}
      }
     }
    }

    return IFX_PPA_CAPWAP_NOT_ADDED;
}

int32_t ppa_add_capwap_group(PPA_CMD_CAPWAP_INFO *p_capwap_group, struct capwap_group_list_item **pp_item)
{

    struct capwap_group_list_item *p_item;

    p_item = ppa_alloc_capwap_group_list_item();
    if ( !p_item )
        return IFX_ENOMEM;
     
    if( ppa_capwap_group_setup(p_capwap_group, p_item ) != IFX_SUCCESS )
    {
        ppa_mem_cache_free(p_item, g_capwap_group_item_cache);
        return IFX_FAILURE;
    }
    
    ppa_list_add(&p_item->capwap_list,&capwap_list_head);

    *pp_item = p_item;

    return IFX_SUCCESS;
}


int32_t __ppa_capwap_group_update(PPA_CMD_CAPWAP_INFO *p_capwap_entry, struct
                capwap_group_list_item *p_item)
{
    struct capwap_group_list_item *p_capwap_item;

    ASSERT(p_item != NULL, "p_item == NULL");
    p_capwap_item = ppa_alloc_capwap_group_list_item();
    if ( !p_capwap_item )
        goto UPDATE_CAPWAP_FAIL;
    
    if(ppa_capwap_group_setup(p_capwap_entry, p_capwap_item) != IFX_SUCCESS){
        goto UPDATE_CAPWAP_FAIL;
    }

    p_capwap_entry->p_entry = p_item->p_entry;
    ppa_list_delete(&p_item->capwap_list);

    if ( ifx_ppa_drv_delete_capwap_entry(p_capwap_entry,0 ) != IFX_SUCCESS )
        goto UPDATE_CAPWAP_FAIL;
    
    ppa_free_capwap_group_list_item(p_item); //after replace, the old item will already be removed from the link list table
   
    ppa_list_add(&p_capwap_item->capwap_list,&capwap_list_head);
    
    if ( ifx_ppa_drv_add_capwap_entry(p_capwap_entry,0 ) == IFX_SUCCESS ) {
    
         p_capwap_item->p_entry = p_capwap_entry->p_entry;
    } 
    else
    {
        goto HW_ADD_CAPWAP_FAIL;
    }
       
    return IFX_SUCCESS;

UPDATE_CAPWAP_FAIL:
    if(p_item){
      ifx_ppa_drv_delete_capwap_entry(p_capwap_entry,0 );
      ppa_remove_capwap_group(p_item);
    }

HW_ADD_CAPWAP_FAIL:
    if(p_capwap_item){
        ppa_remove_capwap_group(p_capwap_item);
    }

    return IFX_FAILURE;
    
}

int32_t ppa_capwap_start_iteration(uint32_t i, struct capwap_group_list_item **pp_item)
{
    int count =0;
    PPA_LIST_NODE *node;
    struct capwap_group_list_item *p_capwap_item = NULL;
    
    ppa_capwap_get_table_lock();
    ppa_list_for_each(node, &capwap_list_head) {
      p_capwap_item = ppa_list_entry(node, struct capwap_group_list_item , capwap_list);

      if(count == i)
      {
         *pp_item = p_capwap_item;
         ppa_capwap_release_table_lock();
	     return IFX_SUCCESS;
      }
      count+=1;
    }

    ppa_capwap_release_table_lock();
	return IFX_FAILURE;
}

#endif




#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG

void ppa_capwap_group_stop_iteration(void)
{
    ppa_lock_release(&g_capwap_group_list_lock);
}

void ppa_capwap_get_table_lock(void)
{
    ppa_lock_get(&g_capwap_group_list_lock);
}

void ppa_capwap_release_table_lock(void)
{
    ppa_lock_release(&g_capwap_group_list_lock);
}

#endif

void ppa_mc_group_stop_iteration(void)
{
    ppa_lock_release(&g_mc_group_list_lock);
}

void ppa_mc_get_htable_lock(void)
{
    ppa_lock_get(&g_mc_group_list_lock);
}

void ppa_mc_release_htable_lock(void)
{
    ppa_lock_release(&g_mc_group_list_lock);
}

void ppa_uc_get_htable_lock(void)
{
    ppa_lock_get(&g_session_list_lock);
}

void ppa_uc_release_htable_lock(void)
{
    ppa_lock_release(&g_session_list_lock);
}

void ppa_br_get_htable_lock(void)
{
    ppa_lock_get(&g_bridging_session_list_lock);
}

void ppa_br_release_htable_lock(void)
{
    ppa_lock_release(&g_bridging_session_list_lock);
}

uint32_t ppa_get_mc_group_count(uint32_t count_flag)
{
    uint32_t count = 0, idx;
    PPA_HLIST_NODE *node;

    ppa_lock_get(&g_mc_group_list_lock);

    for(idx = 0; idx < NUM_ENTITY(g_mc_group_list_hash_table); idx ++){
        ppa_hlist_for_each(node, &g_mc_group_list_hash_table[idx]){
            count ++;
        }
    }
    
    ppa_lock_release(&g_mc_group_list_lock);

    return count;
}

#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
uint32_t ppa_get_capwap_count(void)
{
    uint32_t count = 0;
    PPA_LIST_NODE *node;

    ppa_lock_get(&g_capwap_group_list_lock);

    ppa_list_for_each(node, &capwap_list_head) {
        count ++;
    }

    ppa_lock_release(&g_capwap_group_list_lock);

    return count;
}
#endif


static void ppa_mc_group_replace(struct mc_group_list_item *old, struct mc_group_list_item *new)
{
    ppa_hlist_replace(&old->mc_hlist,&new->mc_hlist);
}

/*
    1. Create a new mc_item & replace the original one
    2. Delete the old one
    3. Add the entry to PPE
*/
int32_t __ppa_mc_group_update(PPA_MC_GROUP *p_mc_entry, struct mc_group_list_item *p_item, uint32_t flags)
{
    struct mc_group_list_item *p_mc_item;

    ASSERT(p_item != NULL, "p_item == NULL");
    p_mc_item = ppa_alloc_mc_group_list_item();
    if ( !p_mc_item )
        goto UPDATE_FAIL;
    
    if(ppa_mc_group_setup(p_mc_entry, p_mc_item, flags) != IFX_SUCCESS){
        goto UPDATE_FAIL;
    }

#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
    p_mc_item->RTP_flag =p_item->RTP_flag;
#endif

    ppa_mc_group_replace(p_item, p_mc_item);
    ppa_free_mc_group_list_item(p_item); //after replace, the old item will already be removed from the link list table
    if(!p_mc_item->src_netif){//NO src itf, cannot add to ppe
        return IFX_SUCCESS;
    }
    if(ppa_hw_add_mc_group(p_mc_item) != IFX_SUCCESS){
        goto UPDATE_FAIL;
    }
       
    return IFX_SUCCESS;
    
UPDATE_FAIL:
    if(p_mc_item){
        ppa_remove_mc_group(p_mc_item);
    }

    return IFX_FAILURE;
    
}


/*
  * multicast routing fw entry update
  */

int32_t ppa_update_mc_hw_group(struct mc_group_list_item *p_item)
{
    PPE_MC_INFO mc = {0};

    mc.p_entry = p_item->mc_entry;
    //update dst interface list
    mc.dest_list = p_item->dest_ifid;

    mc.out_vlan_info.vlan_entry = ~0; 
    if ( (p_item->flags & SESSION_VALID_OUT_VLAN_INS) )
    {
        mc.out_vlan_info.vlan_id = p_item->out_vlan_tag;
        if ( ifx_ppa_drv_add_outer_vlan_entry( &mc.out_vlan_info, 0) == 0 )
        {
            p_item->out_vlan_entry = mc.out_vlan_info.vlan_entry;
        }
        else
        {
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"add out_vlan_ix error\n");
            goto OUT_VLAN_ERROR;
        }
    }
    
    mc.route_type = p_item->bridging_flag ? IFX_PPA_ROUTE_TYPE_NULL : IFX_PPA_ROUTE_TYPE_IPV4;
    mc.f_vlan_ins_enable =p_item->flags & SESSION_VALID_VLAN_INS;
    mc.new_vci = p_item->new_vci;
    mc.f_vlan_rm_enable = p_item->flags & SESSION_VALID_VLAN_RM;
    mc.f_src_mac_enable = p_item->flags & SESSION_VALID_SRC_MAC;
    mc.src_mac_ix = p_item->src_mac_entry;
    mc.pppoe_mode = p_item->flags & SESSION_VALID_PPPOE;
    mc.f_out_vlan_ins_enable =p_item->flags & SESSION_VALID_OUT_VLAN_INS;
    mc.out_vlan_info.vlan_entry = p_item->out_vlan_entry;
    mc.f_out_vlan_rm_enable =  p_item->flags & SESSION_VALID_OUT_VLAN_RM;
    mc.f_new_dscp_enable = p_item->flags & SESSION_VALID_NEW_DSCP;
	mc.f_tunnel_rm_enable = p_item->flags & (SESSION_TUNNEL_6RD | SESSION_TUNNEL_DSLITE); //for only downstream multicast acceleration
    mc.new_dscp = p_item->new_dscp;
    mc.dest_qid = p_item->dslwan_qid;

    ifx_ppa_drv_update_wan_mc_entry(&mc, 0);

    return IFX_SUCCESS;

OUT_VLAN_ERROR:

    return IFX_EAGAIN;
    
}


/*
 *  multicast routing hardware/firmware operation
 */

int32_t ppa_hw_add_mc_group(struct mc_group_list_item *p_item)
{
    PPE_MC_INFO mc={0};

    mc.out_vlan_info.vlan_entry = ~0;    
    mc.route_type = p_item->bridging_flag ? IFX_PPA_ROUTE_TYPE_NULL : IFX_PPA_ROUTE_TYPE_IPV4;

    //  must be LAN port
    //dest_list = 1 << p_item->dest_ifid;   //  sgh remove it since it is already shifted already
    mc.dest_list = p_item->dest_ifid;          //  due to multiple destination support, the dest_ifid here is a bitmap of destination rather than ifid

    if ( (p_item->flags & SESSION_VALID_OUT_VLAN_INS) )
    {
        mc.out_vlan_info.vlan_id = p_item->out_vlan_tag;
        if ( ifx_ppa_drv_add_outer_vlan_entry( &mc.out_vlan_info, 0) == 0 )
        {
            p_item->out_vlan_entry = mc.out_vlan_info.vlan_entry;
        }
        else
        {
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"add out_vlan_ix error\n");
            goto OUT_VLAN_ERROR;
        }
    }

	if(p_item->ip_mc_group.f_ipv6){//ipv6
		ppa_memcpy(mc.dst_ipv6_info.ip.ip6, p_item->ip_mc_group.ip.ip6, sizeof(IP_ADDR));
		if(ifx_ppa_drv_add_ipv6_entry(&mc.dst_ipv6_info, 0) == IFX_SUCCESS){
			p_item->dst_ipv6_entry = mc.dst_ipv6_info.ipv6_entry;
			mc.dest_ip_compare = mc.dst_ipv6_info.psuedo_ip;
		}else{
			goto DST_IPV6_ERROR;
		}
	}else{//ipv4
    	mc.dest_ip_compare = p_item->ip_mc_group.ip.ip;
	}

	if(is_ip_zero(&p_item->source_ip)){
		mc.src_ip_compare = PSEUDO_MC_ANY_SRC;
	}else if(p_item->source_ip.f_ipv6){//ipv6
		ppa_memcpy(mc.src_ipv6_info.ip.ip6, p_item->source_ip.ip.ip6, sizeof(IP_ADDR));
		if(ifx_ppa_drv_add_ipv6_entry(&mc.src_ipv6_info, 0) == IFX_SUCCESS){
			p_item->src_ipv6_entry = mc.src_ipv6_info.ipv6_entry;
			mc.src_ip_compare = mc.src_ipv6_info.psuedo_ip;
		}else{
			goto SRC_IPV6_ERROR;
		}
	}else{
		mc.src_ip_compare = p_item->source_ip.ip.ip;
	}

    ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "mc group ip:%u.%u.%u.%u\n", NIPQUAD(mc.dest_ip_compare));
    ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "mc src ip:%u.%u.%u.%u\n", NIPQUAD(mc.src_ip_compare));
	
    mc.f_vlan_ins_enable =p_item->flags & SESSION_VALID_VLAN_INS;
    mc.new_vci = p_item->new_vci;
    mc.f_vlan_rm_enable = p_item->flags & SESSION_VALID_VLAN_RM;
    mc.f_src_mac_enable = p_item->flags & SESSION_VALID_SRC_MAC;
    mc.src_mac_ix = p_item->src_mac_entry;
    mc.pppoe_mode = p_item->flags & SESSION_VALID_PPPOE;
    mc.f_out_vlan_ins_enable =p_item->flags & SESSION_VALID_OUT_VLAN_INS;
    mc.out_vlan_info.vlan_entry = p_item->out_vlan_entry;
    mc.f_out_vlan_rm_enable =  p_item->flags & SESSION_VALID_OUT_VLAN_RM;
    mc.f_new_dscp_enable = p_item->flags & SESSION_VALID_NEW_DSCP;
	mc.f_tunnel_rm_enable = p_item->flags & (SESSION_TUNNEL_6RD | SESSION_TUNNEL_DSLITE); //for only downstream multicast acceleration
    mc.new_dscp = p_item->new_dscp;
    mc.dest_qid = p_item->dslwan_qid;

#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
    mc.sample_en = p_item->RTP_flag;  /*!< rtp flag */
#endif

    if ( ifx_ppa_drv_add_wan_mc_entry( &mc, 0) == IFX_SUCCESS )
    {
        p_item->mc_entry = mc.p_entry;
        p_item->flags |= SESSION_ADDED_IN_HW;

        if(ppa_atomic_inc(&g_hw_session_cnt) == 1){        
#if defined(CONFIG_CPU_FREQ) || defined(CONFIG_IFX_PMCU) || defined(CONFIG_IFX_PMCU_MODULE)
            ifx_ppa_pwm_activate_module();
#endif
        }
   
        return IFX_SUCCESS;
    }

	ifx_ppa_drv_del_ipv6_entry(&mc.src_ipv6_info,0);
	p_item->src_ipv6_entry = ~0;
SRC_IPV6_ERROR:
	ifx_ppa_drv_del_ipv6_entry(&mc.dst_ipv6_info,0);
	p_item->dst_ipv6_entry = ~0;
DST_IPV6_ERROR:
    p_item->out_vlan_entry = ~0;
    ifx_ppa_drv_del_outer_vlan_entry(&mc.out_vlan_info, 0);
OUT_VLAN_ERROR:
    return IFX_EAGAIN;
}

int32_t ppa_hw_update_mc_group_extra(struct mc_group_list_item *p_item, uint32_t flags)
{
    uint32_t update_flags = 0;
    PPE_MC_INFO mc={0};

    if ( (flags & PPA_F_SESSION_NEW_DSCP) )
        update_flags |= IFX_PPA_UPDATE_WAN_MC_ENTRY_NEW_DSCP_EN | IFX_PPA_UPDATE_WAN_MC_ENTRY_NEW_DSCP;

    if ( (flags & PPA_F_SESSION_VLAN) )
        update_flags |=IFX_PPA_UPDATE_WAN_MC_ENTRY_VLAN_INS_EN |IFX_PPA_UPDATE_WAN_MC_ENTRY_NEW_VCI | IFX_PPA_UPDATE_WAN_MC_ENTRY_VLAN_RM_EN;

    if ( (flags & PPA_F_SESSION_OUT_VLAN) )
    {
        mc.out_vlan_info.vlan_entry = p_item->out_vlan_entry;
        if ( ifx_ppa_drv_get_outer_vlan_entry( &mc.out_vlan_info , 0) == IFX_SUCCESS )
        {            
            if ( mc.out_vlan_info.vlan_id == p_item->out_vlan_tag )
            {
                //  entry not changed
                goto PPA_HW_UPDATE_MC_GROUP_EXTRA_OUT_VLAN_GOON;
            }
            else
            {
                //  entry changed, so delete old first and create new one later
                ifx_ppa_drv_del_outer_vlan_entry(&mc.out_vlan_info, 0);
                p_item->out_vlan_entry = ~0;
            }
        }
        //  create new OUT VLAN entry
        mc.out_vlan_info.vlan_id = p_item->out_vlan_tag;
        if ( ifx_ppa_drv_add_outer_vlan_entry( &mc.out_vlan_info, 0) == IFX_SUCCESS )
        {
            p_item->out_vlan_entry = mc.out_vlan_info.vlan_entry;
            update_flags |= IFX_PPA_UPDATE_WAN_MC_ENTRY_OUT_VLAN_IX;
        }
        else
        {
            ppa_debug(DBG_ENABLE_MASK_ERR,"add_outer_vlan_entry fail\n");
            return IFX_EAGAIN;
        }

        update_flags |= IFX_PPA_UPDATE_WAN_MC_ENTRY_OUT_VLAN_INS_EN | IFX_PPA_UPDATE_WAN_MC_ENTRY_OUT_VLAN_RM_EN ; //IFX_PPA_UPDATE_ROUTING_ENTRY_OUT_VLAN_INS_EN | IFX_PPA_UPDATE_ROUTING_ENTRY_OUT_VLAN_RM_EN;
    }
PPA_HW_UPDATE_MC_GROUP_EXTRA_OUT_VLAN_GOON:
    update_flags |= IFX_PPA_UPDATE_WAN_MC_ENTRY_DEST_QID;  //sgh chnage to update qid, since there is no such flag defined at present

    mc.p_entry = p_item->mc_entry;
    mc.f_vlan_ins_enable = p_item->flags & SESSION_VALID_VLAN_INS;
    mc.new_vci = p_item->new_vci;
    mc.f_vlan_rm_enable = p_item->flags & SESSION_VALID_VLAN_RM;
    mc.f_src_mac_enable = p_item->flags & SESSION_VALID_SRC_MAC;
    mc.src_mac_ix = p_item->src_mac_entry;
    mc.pppoe_mode = p_item->flags & SESSION_VALID_PPPOE;
    mc.f_out_vlan_ins_enable = p_item->flags & SESSION_VALID_OUT_VLAN_INS;
    mc.out_vlan_info.vlan_entry= p_item->out_vlan_entry;
    mc.f_out_vlan_rm_enable = p_item->flags & SESSION_VALID_OUT_VLAN_RM;
    mc.f_new_dscp_enable= p_item->flags & SESSION_VALID_NEW_DSCP;
    mc.new_dscp = p_item->new_dscp;
    mc.dest_qid = p_item->dslwan_qid;
    mc.dest_list = 0;
    mc.update_flags= update_flags;
    ifx_ppa_drv_update_wan_mc_entry(&mc, 0);

    return IFX_SUCCESS;
}

#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE

int32_t ppa_mc_entry_rtp_set(PPA_MC_GROUP *ppa_mc_entry)
{
    struct mc_group_list_item *p_mc_item = NULL;
    uint32_t idx;
    PPA_HLIST_NODE *node, *node_next;
    int32_t entry_found = 0;

    idx = SESSION_LIST_MC_HASH_VALUE(ppa_mc_entry->ip_mc_group.ip.ip);

    ppa_mc_get_htable_lock();

    ppa_hlist_for_each_entry_safe(p_mc_item,node,node_next,&g_mc_group_list_hash_table[idx],mc_hlist){
		if(ip_equal(&p_mc_item->ip_mc_group, &ppa_mc_entry->ip_mc_group)){//mc group ip match
		   if( is_ip_zero(&ppa_mc_entry->source_ip) || ip_equal(&p_mc_item->source_ip, &ppa_mc_entry->source_ip) ){
               entry_found = 1;
               if(ppa_mc_entry->RTP_flag != p_mc_item->RTP_flag)
               {
                  p_mc_item->RTP_flag = ppa_mc_entry->RTP_flag;
                  if ( ppa_hw_set_mc_rtp(p_mc_item) != IFX_SUCCESS )
                  {
                      ppa_mc_release_htable_lock();
                      return IFX_FAILURE;
                  }
               }
		         if( ip_equal(&p_mc_item->source_ip, &ppa_mc_entry->source_ip) ){
                  break;
               }
           }
        }
		}        
    ppa_mc_release_htable_lock();
    if(entry_found == 1)
         return IFX_SUCCESS;
    else
         return IFX_FAILURE;
}

int32_t ppa_hw_set_mc_rtp(struct mc_group_list_item *p_item)
{
    PPE_MC_INFO mc={0};
    mc.p_entry = p_item->mc_entry;
    mc.sample_en = p_item->RTP_flag;
    ifx_ppa_drv_set_wan_mc_rtp(&mc);

    return IFX_SUCCESS;
}

int32_t ppa_hw_get_mc_rtp_sampling_cnt(struct mc_group_list_item *p_item)
{
    PPE_MC_INFO mc={0};
    mc.p_entry = p_item->mc_entry;
    ifx_ppa_drv_get_mc_rtp_sampling_cnt(&mc);
    p_item->rtp_pkt_cnt = mc.rtp_pkt_cnt;  /*!< RTP packet mib */
    p_item->rtp_seq_num = mc.rtp_seq_num;  /*!< RTP sequence number */
    return IFX_SUCCESS;
}

#endif



void ppa_hw_del_mc_group(struct mc_group_list_item *p_item)
{
    if ( (p_item->flags & SESSION_ADDED_IN_HW) )
    {
        PPE_MC_INFO mc={0};
        PPE_OUT_VLAN_INFO out_vlan={0};
#if defined(CONFIG_IFX_PPA_IF_MIB) && CONFIG_IFX_PPA_IF_MIB        
        uint64_t tmp;
        uint32_t i;        
#endif
        mc.p_entry = p_item->mc_entry;
#if defined(CONFIG_IFX_PPA_IF_MIB) && CONFIG_IFX_PPA_IF_MIB
        ifx_ppa_drv_test_and_clear_mc_hit_stat(&mc, 0);
        if(mc.f_hit){
            ifx_ppa_drv_get_mc_entry_bytes(&mc, 0);

            if( (uint32_t)mc.bytes >= p_item->last_bytes){
                tmp = mc.bytes - (uint64_t)p_item->last_bytes;

            } else {
                tmp = mc.bytes + (uint64_t)WRAPROUND_32BITS - (uint64_t)p_item->last_bytes;
            }
            p_item->acc_bytes += tmp;
            p_item->last_bytes = (uint32_t)mc.bytes;

             if( p_item->src_netif )
                update_netif_mib(ppa_get_netif_name(p_item->src_netif), tmp, 1, 0 );

             for(i=0; i<p_item->num_ifs; i++ )
                if( p_item->netif[i])
                    update_netif_mib(ppa_get_netif_name(p_item->netif[i]), tmp, 0, 0);
        }
#endif
        
        out_vlan.vlan_entry = p_item->out_vlan_entry;
        //  when init, these entry values are ~0, the max the number which can be detected by these functions
        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "ppa_hw_del_mc_group:  remove %d from HW\n", p_item->mc_entry);
        ifx_ppa_drv_del_wan_mc_entry(&mc, 0);
        p_item->mc_entry = ~0;
        if(p_item->dst_ipv6_entry != ~0){
            mc.dst_ipv6_info.ipv6_entry = p_item->dst_ipv6_entry;
            ifx_ppa_drv_del_ipv6_entry(&mc.dst_ipv6_info,0);
            p_item->dst_ipv6_entry = ~0;
        }

        if(p_item->src_ipv6_entry != ~0){
           mc.src_ipv6_info.ipv6_entry = p_item->src_ipv6_entry;
           ifx_ppa_drv_del_ipv6_entry(&mc.src_ipv6_info,0);
           p_item->src_ipv6_entry = ~0;
        }

        //  taken from netif_info, so don't need to be removed from MAC table
        p_item->src_mac_entry = ~0;
        
        ifx_ppa_drv_del_outer_vlan_entry( &out_vlan, 0);
        p_item->out_vlan_entry = ~0;
        p_item->flags &= ~SESSION_ADDED_IN_HW;
        
        if(ppa_atomic_dec(&g_hw_session_cnt) == 0){
#if defined(CONFIG_CPU_FREQ) || defined(CONFIG_IFX_PMCU) || defined(CONFIG_IFX_PMCU_MODULE)
            ifx_ppa_pwm_deactivate_module();
#endif
        }
    }
}

/*
 *  routing polling timer
 */

void ppa_set_polling_timer(uint32_t polling_time, uint32_t force)
{
    if( g_hit_polling_time <= MIN_POLLING_INTERVAL )
    { //already using minimal interval already
        return;
    }
    else if ( polling_time < g_hit_polling_time )
    {
        //  remove timer
        ppa_timer_del(&g_hit_stat_timer);
        //  timeout can not be zero
        g_hit_polling_time = polling_time < MIN_POLLING_INTERVAL ? MIN_POLLING_INTERVAL : polling_time;

        //  check hit stat in case the left time is less then the new timeout
        ppa_check_hit_stat(0);  //  timer is added in this function
    }
    else if ( (polling_time > g_hit_polling_time) && force )
    {
        g_hit_polling_time = polling_time;
    }
}

/*
 *  bridging session operation
 */

int32_t __ppa_bridging_lookup_session(uint8_t *mac, PPA_NETIF *netif, struct bridging_session_list_item **pp_item)
{
    int32_t ret;
    struct bridging_session_list_item *p_br_item;
    PPA_HLIST_NODE *node;
    uint32_t idx;

    if( !pp_item  ) 
        ppa_debug(DBG_ENABLE_MASK_ASSERT,"pp_item == NULL\n");

    ret = IFX_PPA_SESSION_NOT_ADDED;

    idx = PPA_BRIDGING_SESSION_LIST_HASH_VALUE(mac);

    *pp_item = NULL;
    ppa_hlist_for_each_entry(p_br_item, node, &g_bridging_session_list_hash_table[idx],br_hlist){
        if(ppa_memcmp(mac, p_br_item->mac, PPA_ETH_ALEN) == 0){
            *pp_item = p_br_item;
            ret = IFX_PPA_SESSION_EXISTS;
        }
    }

    return ret;
}

int32_t ppa_bridging_add_session(uint8_t *mac, PPA_NETIF *netif, struct bridging_session_list_item **pp_item, uint32_t flags)
{
    struct bridging_session_list_item *p_item;
    struct netif_info *ifinfo;

    if ( ppa_netif_update(netif, NULL) != IFX_SUCCESS )
        return IFX_ENOTPOSSIBLE;

    if ( ppa_netif_lookup(ppa_get_netif_name(netif), &ifinfo) != IFX_SUCCESS )
        return IFX_FAILURE;

#if !defined(CONFIG_IFX_PPA_API_DIRECTPATH_BRIDGING)  
    if( ifx_ppa_get_ifid_for_netif(netif) > 0 ) return IFX_FAILURE;   // no need to learn and program mac address in ppe/switch if directpath bridging feature is disabled
#endif

    p_item = ppa_bridging_alloc_session_list_item();
    if ( !p_item )
    {
        ppa_netif_put(ifinfo);
        return IFX_ENOMEM;
    }

    dump_bridging_list_item(p_item, "ppa_bridging_add_session (after init)");

    ppa_memcpy(p_item->mac, mac, PPA_ETH_ALEN);
    p_item->netif         = netif;
    p_item->timeout       = ppa_bridging_get_default_session_timeout();
    p_item->last_hit_time = ppa_get_time_in_sec();

    //  TODO: vlan related fields

    //  decide destination list
    /*yixin: VR9 need to add mac address to switch only if the source mac not from switch port  */
    if ( !(ifinfo->flags & NETIF_PHYS_PORT_GOT) )
    {
        PPE_COUNT_CFG count={0};
        ifx_ppa_drv_get_number_of_phys_port( &count, 0);
        p_item->dest_ifid =  count.num + 1;   //trick here: it will be used for PPE Driver's HAL     
    }else{
        p_item->dest_ifid = ifinfo->phys_port;
    }
    if ( (ifinfo->flags & NETIF_PHY_ATM) )
        p_item->dslwan_qid = ifinfo->dslwan_qid;

    if ( (flags & PPA_F_STATIC_ENTRY) )
    {
        p_item->flags    |= SESSION_STATIC;
        p_item->timeout   = ~0; //  max timeout
    }

    if ( (flags & PPA_F_DROP_PACKET) )
        p_item->flags    |= SESSION_DROP;

    ppa_bridging_insert_session_item(p_item);

    dump_bridging_list_item(p_item, "ppa_bridging_add_session (after setting)");

    *pp_item = p_item;

    ppa_netif_put(ifinfo);

    return IFX_SUCCESS;
}

void ppa_bridging_remove_session(struct bridging_session_list_item *p_item)
{
    ppa_bridging_remove_session_item(p_item);

    ppa_bridging_free_session_list_item(p_item);
}

void dump_bridging_list_item(struct bridging_session_list_item *p_item, char *comment)
{
#if defined(DEBUG_DUMP_LIST_ITEM) && DEBUG_DUMP_LIST_ITEM

    if ( !(g_ppa_dbg_enable & DBG_ENABLE_MASK_DUMP_BRIDGING_SESSION) )
        return;

    if ( comment )
        printk("dump_bridging_list_item - %s\n", comment);
    else
        printk("dump_bridging_list_item\n");
    printk("  next             = %08X\n", (uint32_t)&p_item->br_hlist);
    printk("  mac[6]           = %02x:%02x:%02x:%02x:%02x:%02x\n", p_item->mac[0], p_item->mac[1], p_item->mac[2], p_item->mac[3], p_item->mac[4], p_item->mac[5]);
    printk("  netif            = %08X\n", (uint32_t)p_item->netif);
    printk("  timeout          = %d\n",   p_item->timeout);
    printk("  last_hit_time    = %d\n",   p_item->last_hit_time);
    printk("  flags            = %08X\n", p_item->flags);
    printk("  bridging_entry   = %08X\n", p_item->bridging_entry);

#endif
}

int32_t ppa_bridging_session_start_iteration(uint32_t *ppos, struct bridging_session_list_item **pp_item)
{
    PPA_HLIST_NODE *node = NULL;
    int idx;
    uint32_t l;

    l = *ppos + 1;

    ppa_br_get_htable_lock();

    for ( idx = 0; l && idx < NUM_ENTITY(g_bridging_session_list_hash_table); idx++ )
    {
        ppa_hlist_for_each(node, &g_bridging_session_list_hash_table[idx]){
            if ( !--l )
                break;
        }
    }

    if ( l == 0 && node )
    {
        ++*ppos;
        *pp_item = ppa_hlist_entry(node, struct bridging_session_list_item, br_hlist);
        return IFX_SUCCESS;
    }
    else
    {
        *pp_item = NULL;
        return IFX_FAILURE;
    }
}

int32_t ppa_bridging_session_iterate_next(uint32_t *ppos, struct bridging_session_list_item **pp_item)
{
    uint32_t idx;

    if ( *pp_item == NULL )
        return IFX_FAILURE;

    if ( (*pp_item)->br_hlist.next != NULL )
    {
        ++*ppos;
        *pp_item = ppa_hlist_entry((*pp_item)->br_hlist.next, struct bridging_session_list_item, br_hlist);
        return IFX_SUCCESS;
    }
    else
    {
        for ( idx = PPA_BRIDGING_SESSION_LIST_HASH_VALUE((*pp_item)->mac) + 1;
              idx < NUM_ENTITY(g_bridging_session_list_hash_table);
              idx++ )
            if ( g_bridging_session_list_hash_table[idx].first != NULL )
            {
                ++*ppos;
                *pp_item = ppa_hlist_entry(g_bridging_session_list_hash_table[idx].first, struct bridging_session_list_item, br_hlist);
                return IFX_SUCCESS;
            }
        *pp_item = NULL;
        return IFX_FAILURE;
    }
}

void ppa_bridging_session_stop_iteration(void)
{
    ppa_br_release_htable_lock();
}

/*
 *  bridging session hardware/firmware operation
 */

int32_t ppa_bridging_hw_add_session(struct bridging_session_list_item *p_item)
{
    PPE_BR_MAC_INFO br_mac={0};

    br_mac.port = p_item->dest_ifid;

    if ( (p_item->flags & SESSION_DROP) )
        br_mac.dest_list = 0;  //  no dest list, dropped
    else
        br_mac.dest_list = 1 << p_item->dest_ifid;

    ppa_memcpy(br_mac.mac, p_item->mac, sizeof(br_mac.mac));
    br_mac.f_src_mac_drop = p_item->flags & SESSION_SRC_MAC_DROP_EN;
    br_mac.dslwan_qid = p_item->dslwan_qid;

    if ( ifx_ppa_drv_add_bridging_entry(&br_mac, 0) == IFX_SUCCESS )
    {
        p_item->bridging_entry = br_mac.p_entry;
        p_item->flags |= SESSION_ADDED_IN_HW;
        return IFX_SUCCESS;
    }

    return IFX_FAILURE;
}

void ppa_bridging_hw_del_session(struct bridging_session_list_item *p_item)
{
    if ( (p_item->flags & SESSION_ADDED_IN_HW) )
    {
        PPE_BR_MAC_INFO br_mac={0};
        br_mac.p_entry = p_item->bridging_entry;
        ifx_ppa_drv_del_bridging_entry(&br_mac, 0);
        p_item->bridging_entry = ~0;

        p_item->flags &= ~SESSION_ADDED_IN_HW;
    }
}

/*
 *  bridging polling timer
 */

void ppa_bridging_set_polling_timer(uint32_t polling_time)
{
    if ( polling_time < g_bridging_hit_polling_time )
    {
        //  remove timer
        ppa_timer_del(&g_bridging_hit_stat_timer);

        //  timeout can not be zero
        g_bridging_hit_polling_time = polling_time < 1 ? 1 : polling_time;

        //  check hit stat in case the left time is less then the new timeout
        ppa_bridging_check_hit_stat(0); //  timer is added in this function
    }
}

/*
 *  special function
 */

void ppa_remove_sessions_on_netif(PPA_IFNAME *ifname)
{
    ppa_remove_routing_sessions_on_netif(ifname);
    ppa_remove_mc_groups_on_netif(ifname);
    ppa_remove_bridging_sessions_on_netif(ifname);
}




/*
 * ####################################
 *           Init/Cleanup API
 * ####################################
 */
#define DUMP_HASH_TBL_DEBUG 0 
#if DUMP_HASH_TBL_DEBUG
/* need outside lock */
static void dump_hash_table(char *htable_name, PPA_HLIST_HEAD *htable, uint32_t t_size)
{
    int idx;
    PPA_HLIST_NODE *node;

    printk("dump htable: %s\n", htable_name);
    
    for(idx = 0; idx < t_size; idx ++){
        printk("[%d]: first: 0x%x\n", idx, (uint32_t)htable[idx].first);
        ppa_hlist_for_each(node, &htable[idx]){
            printk("node:0x%x\n", (uint32_t) node);
            if(node == htable[idx].first){
                printk("WARNING: node has the same address as hash table!!!\n");
                break;
            }
        }
    }
    
}
#endif

int32_t ppa_api_session_manager_init(void)
{
    int i;

    /* init hash list */
    for(i = 0; i < SESSION_LIST_MC_HASH_TABLE_SIZE; i ++){
        PPA_INIT_HLIST_HEAD(&g_mc_group_list_hash_table[i]);
    }

    for(i = 0; i < BRIDGING_SESSION_LIST_HASH_TABLE_SIZE; i ++){
        PPA_INIT_HLIST_HEAD(&g_bridging_session_list_hash_table[i]);
    }

    for(i = 0; i < SESSION_LIST_HASH_TABLE_SIZE; i ++){
        PPA_INIT_HLIST_HEAD(&g_session_list_hash_table[i]);
    }

    //PPA_INIT_HLIST_HEAD(&capwap_list_head);

    //  start timer
    ppa_timer_init(&g_hit_stat_timer,   ppa_check_hit_stat);
    ppa_timer_add (&g_hit_stat_timer,   g_hit_polling_time);
    ppa_timer_init(&g_mib_cnt_timer,    ppa_mib_count);
    ppa_timer_add (&g_mib_cnt_timer,    g_mib_polling_time);
    ppa_timer_init(&g_bridging_hit_stat_timer, ppa_bridging_check_hit_stat);
    ppa_timer_add (&g_bridging_hit_stat_timer, g_bridging_hit_polling_time);

    return IFX_SUCCESS;
}

void ppa_api_session_manager_exit(void)
{
    ppa_timer_del(&g_hit_stat_timer);
    ppa_timer_del(&g_bridging_hit_stat_timer);
    ppa_timer_del(&g_mib_cnt_timer);

    ppa_free_session_list();
    ppa_free_mc_group_list();
    ppa_free_bridging_session_list();

    ppa_kmem_cache_shrink(g_session_item_cache);
    ppa_kmem_cache_shrink(g_mc_group_item_cache);
    ppa_kmem_cache_shrink(g_bridging_session_item_cache);
}

int32_t ppa_api_session_manager_create(void)
{
    if ( ppa_mem_cache_create("ppa_session_item", sizeof(struct session_list_item), &g_session_item_cache) )
    {
        ppa_debug(DBG_ENABLE_MASK_ERR,"Failed in creating mem cache for routing session list.\n");
        goto PPA_API_SESSION_MANAGER_CREATE_FAIL;
    }

    if ( ppa_mem_cache_create("mc_group_item", sizeof(struct mc_group_list_item), &g_mc_group_item_cache) )
    {
        ppa_debug(DBG_ENABLE_MASK_ERR,"Failed in creating mem cache for multicast group list.\n");
        goto PPA_API_SESSION_MANAGER_CREATE_FAIL;
    }

    if ( ppa_mem_cache_create("bridging_sess_item", sizeof(struct bridging_session_list_item), &g_bridging_session_item_cache) )
    {
        ppa_debug(DBG_ENABLE_MASK_ERR,"Failed in creating mem cache for bridging session list.\n");
        goto PPA_API_SESSION_MANAGER_CREATE_FAIL;
    }

#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
    if ( ppa_mem_cache_create("capwap_group_item", sizeof(struct capwap_group_list_item), &g_capwap_group_item_cache) )
    {
        ppa_debug(DBG_ENABLE_MASK_ERR,"Failed in creating mem cache for capwap group list.\n");
        goto PPA_API_SESSION_MANAGER_CREATE_FAIL;
    }
#endif


    if ( ppa_lock_init(&g_session_list_lock) )
    {
        ppa_debug(DBG_ENABLE_MASK_ERR,"Failed in creating lock for routing session list.\n");
        goto PPA_API_SESSION_MANAGER_CREATE_FAIL;
    }

    if ( ppa_lock_init(&g_mc_group_list_lock) )
    {
        ppa_debug(DBG_ENABLE_MASK_ERR,"Failed in creating lock for multicast group list.\n");
        goto PPA_API_SESSION_MANAGER_CREATE_FAIL;
    }

    if ( ppa_lock_init(&g_bridging_session_list_lock) )
    {
        ppa_debug(DBG_ENABLE_MASK_ERR,"Failed in creating lock for bridging session list.\n");
        goto PPA_API_SESSION_MANAGER_CREATE_FAIL;
    }
    if ( ppa_lock_init(&g_general_lock) )
    {
        ppa_debug(DBG_ENABLE_MASK_ERR,"Failed in creating lock for general mib conter.\n");
        goto PPA_API_SESSION_MANAGER_CREATE_FAIL;
    }
#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
    ppa_lock_init(&g_capwap_group_list_lock);
#endif

    reset_local_mib();
    ppa_atomic_set(&g_hw_session_cnt,0);

    return IFX_SUCCESS;

PPA_API_SESSION_MANAGER_CREATE_FAIL:
    ppa_api_session_manager_destroy();
    return IFX_EIO;
}

void ppa_api_session_manager_destroy(void)
{
    if ( g_session_item_cache )
    {
        ppa_mem_cache_destroy(g_session_item_cache);
        g_session_item_cache = NULL;
    }

    if ( g_mc_group_item_cache )
    {
        ppa_mem_cache_destroy(g_mc_group_item_cache);
        g_mc_group_item_cache = NULL;
    }

    if ( g_bridging_session_item_cache )
    {
        ppa_mem_cache_destroy(g_bridging_session_item_cache);
        g_bridging_session_item_cache = NULL;
    }

#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
    if ( g_capwap_group_item_cache )
    {
        ppa_mem_cache_destroy(g_capwap_group_item_cache);
        g_capwap_group_item_cache = NULL;
    }
#endif

    ppa_lock_destroy(&g_session_list_lock);

    ppa_lock_destroy(&g_mc_group_list_lock);
    ppa_lock_destroy(&g_general_lock);

    ppa_lock_destroy(&g_bridging_session_list_lock);
#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG
    ppa_lock_destroy(&g_capwap_group_list_lock);
#endif
}

uint32_t ppa_api_get_session_poll_timer(void)
{
    return g_hit_polling_time;
}

uint32_t ppa_api_set_test_qos_priority_via_tos(PPA_BUF *ppa_buf)
{
    uint32_t pri = ppa_get_pkt_ip_tos(ppa_buf) % 8;
    ppa_set_pkt_priority( ppa_buf, pri );
    return pri;
}

uint32_t ppa_api_set_test_qos_priority_via_mark(PPA_BUF *ppa_buf)
{
    uint32_t pri = ppa_get_skb_mark(ppa_buf) % 8;
    ppa_set_pkt_priority( ppa_buf, pri );
    return pri;
}

int ppa_get_hw_session_cnt(void)
{
    return ppa_atomic_read(&g_hw_session_cnt);
}


int32_t get_ppa_hash_count(PPA_CMD_COUNT_INFO *count_info)
{
    if(count_info)
    {
        count_info->count = SESSION_LIST_HASH_TABLE_SIZE;
    }

    return IFX_SUCCESS;
}

uint32_t get_ppa_sesssion_num_in_one_hash(uint32_t i)
{
    uint32_t num = 0;
    PPA_HLIST_NODE *node;

    if( i >= SESSION_LIST_HASH_TABLE_SIZE ) return 0;
    
    ppa_uc_get_htable_lock();
    ppa_hlist_for_each(node,&g_session_list_hash_table[i])
    {
       num ++;
       if( num > 100 )
        {
            err("Why num=%d in one single hash index\n", num);
            break;
        }
    }
    ppa_uc_release_htable_lock();
  
    return num;    
}



EXPORT_SYMBOL(ppa_bridging_session_start_iteration);
EXPORT_SYMBOL(ppa_bridging_session_iterate_next);
EXPORT_SYMBOL(ppa_bridging_session_stop_iteration);
EXPORT_SYMBOL(ppa_session_start_iteration);
EXPORT_SYMBOL(ppa_session_iterate_next);
EXPORT_SYMBOL(ppa_session_stop_iteration);
EXPORT_SYMBOL(ppa_mc_group_start_iteration);
EXPORT_SYMBOL(ppa_mc_group_iterate_next);
EXPORT_SYMBOL(ppa_mc_group_stop_iteration);
EXPORT_SYMBOL(ppa_alloc_session_list_item);
EXPORT_SYMBOL(ppa_free_session_list_item);
EXPORT_SYMBOL(g_hit_polling_time);
EXPORT_SYMBOL(ppa_api_get_session_poll_timer);
EXPORT_SYMBOL(ppa_session_get_items_via_index);
EXPORT_SYMBOL(get_ppa_hash_count);
EXPORT_SYMBOL(get_ppa_sesssion_num_in_one_hash);
EXPORT_SYMBOL(ppa_uc_get_htable_lock);
EXPORT_SYMBOL(ppa_uc_release_htable_lock);
EXPORT_SYMBOL(ppa_check_hit_stat_clear_mib);
EXPORT_SYMBOL(__ppa_lookup_session);
