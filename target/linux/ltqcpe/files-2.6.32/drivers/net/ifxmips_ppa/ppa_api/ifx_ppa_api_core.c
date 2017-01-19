/*******************************************************************************
**
** FILE NAME    : ifx_ppa_api_core.c
** PROJECT      : PPA
** MODULES      : PPA API (Routing/Bridging Acceleration APIs)
**
** DATE         : 3 NOV 2008
** AUTHOR       : Xu Liang
** DESCRIPTION  : PPA Protocol Stack Hook API Implementation
** COPYRIGHT    :              Copyright (c) 2009
**                          Lantiq Deutschland GmbH
**                   Am Campeon 3; 85579 Neubiberg, Germany
**data
**   For licensing information, see the file 'LICENSE' in the root folder of
**   this software module.
**
** HISTORY
** $Date        $Author                $Comment
** 03 NOV 2008  Xu Liang               Initiate Version
** 10 DEC 2012  Manamohan Shetty       Added the support for RTP,MIB mode and CAPWAP 
**                                     Features 
*******************************************************************************/



/*
 * ####################################
 *              Version No.
 * ####################################
 */

#define VER_FAMILY      0x60        //  bit 0: res
                                    //      1: Danube
                                    //      2: Twinpass
                                    //      3: Amazon-SE
                                    //      4: res
                                    //      5: AR9
                                    //      6: GR9
#define VER_DRTYPE      0x20        //  bit 0: Normal Data Path driver
                                    //      1: Indirect-Fast Path driver
                                    //      2: HAL driver
                                    //      3: Hook driver
                                    //      4: Stack/System Adaption Layer driver
                                    //      5: PPA API driver
#define VER_INTERFACE   0x07        //  bit 0: MII 0
                                    //      1: MII 1
                                    //      2: ATM WAN
                                    //      3: PTM WAN
#define VER_ACCMODE     0x03        //  bit 0: Routing
                                    //      1: Bridging
#define VER_MAJOR       0
#define VER_MID         0
#define VER_MINOR       4



/*
 * ####################################
 *              Head File
 * ####################################
 */

/*
 *  Common Head File
 */

/*
 *  PPA Specific Head File
 */
#include "ifx_ppa_ss.h"
#include <net/ifx_ppa_api_common.h>
#include <net/ifx_ppa_api.h>
#include "ifx_ppe_drv_wrapper.h"
#include <net/ifx_ppa_ppe_hal.h>
#include "ifx_ppa_api_misc.h"
#include "ifx_ppa_api_netif.h"
#include "ifx_ppa_api_session.h"
#include <net/ifx_ppa_hook.h>
#include "ifx_ppa_api_core.h"
#include "ifx_ppa_api_tools.h"
#if defined(CONFIG_CPU_FREQ) || defined(CONFIG_IFX_PMCU) || defined(CONFIG_IFX_PMCU_MODULE)
#include "ifx_ppa_api_pwm.h"
#endif
#if defined(CONFIG_IFX_PPA_MFE) && CONFIG_IFX_PPA_MFE
#include "ifx_ppa_api_mfe.h"
#endif
#ifdef CONFIG_IFX_PPA_QOS
#include "ifx_ppa_api_qos.h"
#endif
#include "ifx_ppa_api_mib.h"



/*
 * ####################################
 *              Definition
 * ####################################
 */

#define MIN_HITS                        2


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

#if defined(CONFIG_IFX_PPA_API_DIRECTPATH)
  int32_t ppa_api_directpath_init(void);
  void ppa_api_directpath_exit(void);
#endif


/*
 * ####################################
 *           Global Variable
 * ####################################
 */

static uint32_t g_broadcast_bridging_entry = ~0;

uint32_t g_ppa_min_hits = MIN_HITS;
static uint8_t g_bridging_mac_learning = 1;
uint32_t g_ppa_ppa_mtu=DEFAULT_MTU;  /*maximum frame size from ip header to end of the data payload, not including MAC header/pppoe header/vlan */

volatile uint8_t g_ppe_fastpath_enabled = 1;

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

static int32_t ppa_init(PPA_INIT_INFO *p_info, uint32_t flags)
{
    int32_t ret;
    uint8_t broadcast_mac[PPA_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint32_t i;
    PPA_MAX_ENTRY_INFO entry={0};
    PPE_FAST_MODE_CFG fast_mode={0};
    PPE_WAN_VID_RANGE vlan_id;
    PPE_ACC_ENABLE acc_cfg={0};
    PPE_BR_MAC_INFO br_mac={0};

    if ( ifx_ppa_drv_hal_init(0) != IFX_SUCCESS )
    {
        ret = IFX_EIO;
        goto HAL_INIT_ERROR;
    }

    ifx_ppa_drv_get_max_entries(&entry, 0);

    ret = IFX_EINVAL;
    if ( p_info->max_lan_source_entries + p_info->max_wan_source_entries > (entry.max_lan_entries + entry.max_wan_entries ))
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"Two many entries:%d > %d\n",  p_info->max_lan_source_entries + p_info->max_wan_source_entries , (entry.max_lan_entries + entry.max_wan_entries ) );
        goto MAX_SOURCE_ENTRIES_ERROR;
    }
    if ( p_info->max_mc_entries > entry.max_mc_entries)
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"Two many multicast entries:%d > %d\n",   p_info->max_mc_entries , entry.max_mc_entries);
        goto MAX_MC_ENTRIES_ERROR;
    }
    if ( p_info->max_bridging_entries > entry.max_bridging_entries)
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"Two many bridge entries:%d > %d\n",   p_info->max_bridging_entries , entry.max_bridging_entries);
        goto MAX_BRG_ENTRIES_ERROR;
    }

    //  disable accelation mode by default
    acc_cfg.f_is_lan = 1;
    acc_cfg.f_enable = IFX_PPA_ACC_MODE_NONE;
    if( ifx_ppa_drv_set_acc_mode( &acc_cfg, 0) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_drv_set_acc_mode lan fail\n");
    }
    acc_cfg.f_is_lan = 0;
    acc_cfg.f_enable = IFX_PPA_ACC_MODE_NONE;
    if( ifx_ppa_drv_set_acc_mode( &acc_cfg, 0) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_drv_set_acc_mode  wan fail\n");
    }

    if ( (entry.max_lan_entries + entry.max_wan_entries ) || entry.max_mc_entries )
    {
        PPE_ROUTING_CFG cfg;

        //set LAN acceleration
        ppa_memset( &cfg, 0, sizeof(cfg));
        cfg.f_is_lan = 1;
        cfg.entry_num = p_info->max_lan_source_entries;
        cfg.mc_entry_num = 0;
        cfg.f_ip_verify = p_info->lan_rx_checks.f_ip_verify;
        cfg.f_tcpudp_verify=p_info->lan_rx_checks.f_tcp_udp_verify;
        cfg.f_tcpudp_err_drop=p_info->lan_rx_checks.f_tcp_udp_err_drop;
        cfg.f_drop_on_no_hit = p_info->lan_rx_checks.f_drop_on_no_hit;
        cfg.f_mc_drop_on_no_hit = 0;
        cfg.flags = IFX_PPA_SET_ROUTE_CFG_ENTRY_NUM | IFX_PPA_SET_ROUTE_CFG_IP_VERIFY | IFX_PPA_SET_ROUTE_CFG_TCPUDP_VERIFY | IFX_PPA_SET_ROUTE_CFG_DROP_ON_NOT_HIT;
        if( ifx_ppa_drv_set_route_cfg(&cfg, 0) != IFX_SUCCESS )
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_drv_set_route_cfg lan fail\n");

        //set WAN acceleration
        ppa_memset( &cfg, 0, sizeof(cfg));
        cfg.f_is_lan = 0;
        cfg.entry_num = p_info->max_wan_source_entries;
        cfg.mc_entry_num = p_info->max_mc_entries;
        cfg.f_ip_verify = p_info->wan_rx_checks.f_ip_verify;
        cfg.f_tcpudp_verify=p_info->wan_rx_checks.f_tcp_udp_verify;
        cfg.f_tcpudp_err_drop=p_info->wan_rx_checks.f_tcp_udp_err_drop;
        cfg.f_drop_on_no_hit = p_info->wan_rx_checks.f_drop_on_no_hit;
        cfg.f_mc_drop_on_no_hit =  p_info->wan_rx_checks.f_mc_drop_on_no_hit;
        cfg.flags = IFX_PPA_SET_ROUTE_CFG_ENTRY_NUM | IFX_PPA_SET_ROUTE_CFG_MC_ENTRY_NUM | IFX_PPA_SET_ROUTE_CFG_IP_VERIFY | IFX_PPA_SET_ROUTE_CFG_TCPUDP_VERIFY | IFX_PPA_SET_ROUTE_CFG_DROP_ON_NOT_HIT | IFX_PPA_SET_ROUTE_CFG_MC_DROP_ON_NOT_HIT;
        if( ifx_ppa_drv_set_route_cfg( &cfg, 0) != IFX_SUCCESS )
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_drv_set_route_cfg wan fail\n");
    }

    if ( entry.max_bridging_entries )
    {
        PPE_BRDG_CFG br_cfg;
        PPE_COUNT_CFG count={0};
        if ( ifx_ppa_drv_get_number_of_phys_port( &count, 0) != IFX_SUCCESS )
        {
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_drv_get_number_of_phys_port fail\n");
        }

        ppa_memset( &br_cfg, 0, sizeof(br_cfg));
        br_cfg.entry_num = p_info->max_bridging_entries;
        br_cfg.br_to_src_port_mask = (1 << count.num) - 1;   //  br_to_src_port_mask
        br_cfg.flags = IFX_PPA_SET_BRIDGING_CFG_ENTRY_NUM | IFX_PPA_SET_BRIDGING_CFG_BR_TO_SRC_PORT_EN | IFX_PPA_SET_BRIDGING_CFG_DEST_VLAN_EN | IFX_PPA_SET_BRIDGING_CFG_SRC_VLAN_EN | IFX_PPA_SET_BRIDGING_CFG_MAC_CHANGE_DROP;

        if ( ifx_ppa_drv_set_bridging_cfg( &br_cfg, 0) != IFX_SUCCESS )
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_drv_set_bridging_cfg  fail\n");
    }

    if ( (ret = ppa_api_netif_manager_init()) != IFX_SUCCESS || !IFX_PPA_IS_PORT_CPU0_AVAILABLE() )
    {
        if( ret != IFX_SUCCESS )
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ppa_api_netif_manager_init  fail\n");
        else
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"CPU0 not available\n");
        goto PPA_API_NETIF_CREATE_INIT_FAIL;
    }

    if ( (ret = ppa_api_session_manager_init()) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ppa_api_session_manager_init  fail\n");
        goto PPA_API_SESSION_MANAGER_INIT_FAIL;
    }
    for ( i = 0; i < p_info->num_lanifs; i++ )
        if ( p_info->p_lanifs[i].ifname != NULL && ppa_netif_add(p_info->p_lanifs[i].ifname, 1, NULL, NULL,0) != IFX_SUCCESS )
            ppa_debug(DBG_ENABLE_MASK_ERR,"Failed in adding LAN side network interface - %s, reason could be no sufficient memory or LAN/WAN rule violation with physical network interface.\n", p_info->p_lanifs[i].ifname);
    for ( i = 0; i < p_info->num_wanifs; i++ )
        if ( p_info->p_wanifs[i].ifname != NULL && ppa_netif_add(p_info->p_wanifs[i].ifname, 0, NULL, NULL,0) != IFX_SUCCESS )
            ppa_debug(DBG_ENABLE_MASK_ERR,"Failed in adding WAN side network interface - %s, reason could be no sufficient memory or LAN/WAN rule violation with physical network interface.\n", p_info->p_wanifs[i].ifname);

#if defined(CONFIG_IFX_PPA_API_DIRECTPATH)
    if ( (ret = ppa_api_directpath_init()) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ppa_api_directpath_init fail - %d\n", ret);
        goto PPA_API_DIRECTPATH_INIT_FAIL;
    }
#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_IMQ)
	ppa_hook_directpath_reinject_from_imq_fn = ifx_ppa_directpath_reinject_from_imq;
	ppa_directpath_imq_en_flag = 0;
#endif
#endif

/***** network interface mode will not be set by PPA API
    it should be configured by low level driver (eth/atm driver)
    #ifdef CONFIG_IFX_PPA_A4
        set_if_type(eth0_iftype, 0);
        set_wan_vlan_id((0x010 << 16) | 0x000);  //  high 16 bits is upper of WAN VLAN ID, low 16 bits is min WAN VLAN ID
    #else
        set_if_type(eth0_iftype, 0);
        set_if_type(eth1_iftype, 1);
    #endif
*/
    //  this is default setting for LAN/WAN mix mode to use VLAN tag to differentiate LAN/WAN traffic
    vlan_id.vid =(0x010 << 16) | 0x000; //  high 16 bits is upper of WAN VLAN ID, low 16 bits is min WAN VLAN ID
    if( ifx_ppa_drv_set_mixed_wan_vlan_id( &vlan_id, 0) != IFX_SUCCESS )//  high 16 bits is upper of WAN VLAN ID, low 16 bits is min WAN VLAN ID
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_drv_set_mixed_wan_vlan_id fail\n");

/*** WFQ is hidden and not to be changed after firmware running. ***
  set_if_wfq(ETX_WFQ_D4, 1);
  set_fastpath_wfq(FASTPATH_WFQ_D4);
  set_dplus_wfq(DPLUS_WFQ_D4);
*/
    for ( i = 0; i < 8; i++ )
    {
        PPE_DEST_LIST cfg;
        cfg.uc_dest_list = 1 << IFX_PPA_PORT_CPU0;
        cfg.mc_dest_list = 1 << IFX_PPA_PORT_CPU0;
        cfg.if_no = i;
        if( ifx_ppa_drv_set_default_dest_list( &cfg, 0 ) != IFX_SUCCESS )
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_drv_set_default_dest_list fail\n");
    }

    /*Note, FAST_CPU1_DIRECT is only used for Danube twinpass product */
    fast_mode.mode = IFX_PPA_SET_FAST_MODE_CPU1_INDIRECT | IFX_PPA_SET_FAST_MODE_ETH1_DIRECT;
    fast_mode.flags = IFX_PPA_SET_FAST_MODE_CPU1 | IFX_PPA_SET_FAST_MODE_ETH1;
    if( ifx_ppa_drv_set_fast_mode( &fast_mode, 0 ) != IFX_SUCCESS )
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_drv_set_fast_mode fail\n");

    br_mac.port = IFX_PPA_PORT_CPU0;
    ppa_memcpy(br_mac.mac, broadcast_mac, sizeof(br_mac.mac));
    br_mac.f_src_mac_drop = 0;
    br_mac.dslwan_qid = 0;
    br_mac.dest_list = IFX_PPA_DEST_LIST_CPU0;
    br_mac.p_entry = g_broadcast_bridging_entry;

    if ( ifx_ppa_drv_add_bridging_entry(&br_mac, 0) != IFX_SUCCESS )
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_drv_add_bridging_entry broadcast fail\n");
    g_broadcast_bridging_entry = br_mac.p_entry;

    if( p_info->add_requires_min_hits )
        g_ppa_min_hits = p_info->add_requires_min_hits;

#ifdef CONFIG_IFX_PPA_QOS_RATE_SHAPING
    if( ifx_ppa_drv_init_qos_rate(0) != IFX_SUCCESS ){
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_drv_init_qos_rate  fail\n");
    }
#endif /* CONFIG_IFX_PPA_QOS_RATE_SHAPING */

#ifdef CONFIG_IFX_PPA_QOS_WFQ
    if( ifx_ppa_drv_init_qos_wfq(0) != IFX_SUCCESS ){
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_drv_init_qos_wfq  fail\n");
    }
#endif /* CONFIG_IFX_PPA_QOS_WFQ */



#if defined(CONFIG_CPU_FREQ) || defined(CONFIG_IFX_PMCU) || defined(CONFIG_IFX_PMCU_MODULE)
    ifx_ppa_pwm_init();
#endif

#if defined(CONFIG_IFX_PPA_MFE) && CONFIG_IFX_PPA_MFE    
    if( ifx_ppa_multifield_control(0, 0) != IFX_SUCCESS ) //by default, disable it
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_multifield_control  fail\n");

#endif /* CONFIG_IFX_PPA_MFE */

    ppa_set_init_status(PPA_INIT_STATE);

    return IFX_SUCCESS;

PPA_API_SESSION_MANAGER_INIT_FAIL:
    ppa_api_session_manager_exit();
#if defined(CONFIG_IFX_PPA_API_DIRECTPATH)
PPA_API_DIRECTPATH_INIT_FAIL:
    ppa_api_directpath_exit();
#endif
PPA_API_NETIF_CREATE_INIT_FAIL:
    ppa_api_netif_manager_exit();
MAX_BRG_ENTRIES_ERROR:
MAX_MC_ENTRIES_ERROR:
MAX_SOURCE_ENTRIES_ERROR:
    ifx_ppa_drv_hal_exit(0);
HAL_INIT_ERROR:
    ppa_debug(DBG_ENABLE_MASK_ERR,"failed in PPA init\n");
    return ret;
}

static void ppa_exit(void)
{
    PPE_BR_MAC_INFO br_mac={0};
    
#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_IMQ)
   ppa_hook_directpath_reinject_from_imq_fn = NULL;
   ppa_directpath_imq_en_flag = 0;
#endif
    ppa_set_init_status(PPA_UNINIT_STATE);
    ppa_synchronize_rcu();

#if defined(CONFIG_CPU_FREQ) || defined(CONFIG_IFX_PMCU) || defined(CONFIG_IFX_PMCU_MODULE)
    ifx_ppa_pwm_exit();
#endif

#if defined(CONFIG_IFX_PPA_MFE) && CONFIG_IFX_PPA_MFE
    ifx_ppa_multifield_control(0, 0); //by default, disable it
    ifx_ppa_quick_del_multifield_flow(-1, 0);
#endif //end of CONFIG_IFX_PPA_MFE

    ifx_ppa_vlan_filter_del_all(0); //sgh add,

    br_mac.p_entry = g_broadcast_bridging_entry;
    ifx_ppa_drv_del_bridging_entry(&br_mac, 0);

#if defined(CONFIG_IFX_PPA_API_DIRECTPATH)
     ppa_api_directpath_exit();  //must put before ppa_api_netif_manager_exit since directpath exit will call netif related API
#endif

    ppa_api_session_manager_exit();

    ppa_api_netif_manager_exit();

    ifx_ppa_drv_hal_exit(0);
    
}



/*
 * ####################################
 *           Global Function
 * ####################################
 */

/*
 *  Function for internal use
 */


/*
 *  PPA Initialization Functions
 */

void ifx_ppa_subsystem_id(uint32_t *p_family,
                          uint32_t *p_type,
                          uint32_t *p_if,
                          uint32_t *p_mode,
                          uint32_t *p_major,
                          uint32_t *p_mid,
                          uint32_t *p_minor,
                          uint32_t *p_tag)
{
    if ( p_family )
        *p_family = 0;

    if ( p_type )
        *p_type = 0;

    if ( p_if )
        *p_if = 0;

    if ( p_mode )
        *p_mode = 0;

    if ( p_major )
        *p_major = PPA_SUBSYSTEM_MAJOR;

    if ( p_mid )
        *p_mid = PPA_SUBSYSTEM_MID;

    if ( p_minor )
        *p_minor = PPA_SUBSYSTEM_MINOR;

    if( p_tag ) 
        *p_tag = PPA_SUBSYSTEM_TAG;
}

void ifx_ppa_get_api_id(uint32_t *p_family,
                        uint32_t *p_type,
                        uint32_t *p_if,
                        uint32_t *p_mode,
                        uint32_t *p_major,
                        uint32_t *p_mid,
                        uint32_t *p_minor)
{
    if ( p_family )
        *p_family = VER_FAMILY;

    if ( p_type )
        *p_type = VER_DRTYPE;

    if ( p_if )
        *p_if = VER_INTERFACE;

    if ( p_mode )
        *p_mode = VER_ACCMODE;

    if ( p_major )
        *p_major = VER_MAJOR;

    if ( p_mid )
        *p_mid = VER_MID;

    if ( p_minor )
        *p_minor = VER_MINOR;
}

int32_t ifx_ppa_init(PPA_INIT_INFO *p_info, uint32_t flags)
{
    int32_t ret;

    if ( !p_info )
        return IFX_EINVAL;

    if ( ifx_ppa_is_init() )
        ppa_exit();

    if ( (ret = ppa_init(p_info, flags)) == IFX_SUCCESS )
        printk("ifx_ppa_init - init succeeded\n");
    else
        printk("ifx_ppa_init - init failed (%d)\n", ret);

    return ret;
}

void ifx_ppa_exit(void)
{
    if ( ifx_ppa_is_init() )
        ppa_exit();
}

/*
 *  PPA Enable/Disable and Status Functions
 */

int32_t ifx_ppa_enable(uint32_t lan_rx_ppa_enable, uint32_t wan_rx_ppa_enable, uint32_t flags)
{
    u32 sys_flag;
    PPE_ACC_ENABLE acc_cfg;

    if ( ifx_ppa_is_init() )
    {
        lan_rx_ppa_enable = lan_rx_ppa_enable ? IFX_PPA_ACC_MODE_ROUTING : IFX_PPA_ACC_MODE_NONE;
        wan_rx_ppa_enable = wan_rx_ppa_enable ? IFX_PPA_ACC_MODE_ROUTING : IFX_PPA_ACC_MODE_NONE;
        sys_flag = ppa_disable_int();
        acc_cfg.f_is_lan =1;
        acc_cfg.f_enable = lan_rx_ppa_enable;
        ifx_ppa_drv_set_acc_mode(&acc_cfg, 0);

        acc_cfg.f_is_lan =0;
        acc_cfg.f_enable = wan_rx_ppa_enable;
        ifx_ppa_drv_set_acc_mode(&acc_cfg, 0);
        ppa_enable_int(sys_flag);
        return IFX_SUCCESS;
    }
    return IFX_FAILURE;
}


#if defined(MIB_MODE_ENABLE) && MIB_MODE_ENABLE

/*
 *  PPA Set Unicast/multicast session mib mode configuration 
 */

int32_t ifx_ppa_set_mib_mode(uint8_t mib_mode)
{
    PPE_MIB_MODE_ENABLE mib_cfg;

    mib_cfg.session_mib_unit = mib_mode; 
    
    ifx_ppa_drv_set_mib_mode(&mib_cfg,0);

    return IFX_SUCCESS;
}

/*
 *  PPA Get Unicast/multicast session mib mode configuration 
 */

int32_t ifx_ppa_get_mib_mode(uint8_t *mib_mode)
{
    PPE_MIB_MODE_ENABLE mib_cfg;
    
    ifx_ppa_drv_get_mib_mode(&mib_cfg);

    *mib_mode =mib_cfg.session_mib_unit; 

    return IFX_SUCCESS;
}

#endif

int32_t ifx_ppa_get_status(uint32_t *lan_rx_ppa_enable, uint32_t *wan_rx_ppa_enable, uint32_t flags)
{
    if ( ifx_ppa_is_init() )
    {
        PPE_ACC_ENABLE cfg;

        cfg.f_is_lan = 1;
        ifx_ppa_drv_get_acc_mode(&cfg, 0);
		if( lan_rx_ppa_enable ) *lan_rx_ppa_enable = cfg.f_enable;

        cfg.f_is_lan = 0;
        ifx_ppa_drv_get_acc_mode(&cfg, 0);
		if( wan_rx_ppa_enable ) *wan_rx_ppa_enable = cfg.f_enable;
        return IFX_SUCCESS;
    }
    return IFX_FAILURE;
}

static INLINE int32_t ifx_ppa_pkt_filter(PPA_BUF *ppa_buf, uint32_t flags)
{
    //basic pkt filter
    PPA_NETIF *tx_if;
    PPA_IFNAME *ifname;
    struct netif_info *p_info;
    uint32_t flag;
    
    //  ignore packets output by the device
    if ( ppa_is_pkt_host_output(ppa_buf) )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT,"ppa_is_pkt_host_output\n");
        return IFX_PPA_SESSION_FILTED;
    }

    //  ignore incoming broadcast
    if ( ppa_is_pkt_broadcast(ppa_buf) )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT,"ppa_is_pkt_broadcast\n");
        return IFX_PPA_SESSION_FILTED;
    }

    //  ignore multicast packet in unitcast routing but learn multicast source interface automatically
    if (  ppa_is_pkt_multicast(ppa_buf) )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT, "ppa_is_pkt_multicast\n");

        /*auto learn multicast source interface*/
        if ( (flags & PPA_F_BEFORE_NAT_TRANSFORM ) && ppa_hook_multicast_pkt_srcif_add_fn )
            ppa_hook_multicast_pkt_srcif_add_fn(ppa_buf, NULL);

        return IFX_PPA_SESSION_FILTED;
    }

    //  ignore loopback packet
    if ( ppa_is_pkt_loopback(ppa_buf) )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT,"ppa_is_pkt_loopback\n");
        return IFX_PPA_SESSION_FILTED;
    }

    //  ignore protocols other than TCP/UDP, since some of them (e.g. ICMP) can't be handled safe in this arch
    if ( ppa_get_pkt_ip_proto(ppa_buf) != IFX_IPPROTO_UDP && ppa_get_pkt_ip_proto(ppa_buf) != IFX_IPPROTO_TCP )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT,"protocol: %d\n", (uint32_t)ppa_get_pkt_ip_proto(ppa_buf));
        return IFX_PPA_SESSION_FILTED;
    }

    //  ignore special broadcast type (windows netbios)
    if ( ppa_get_pkt_src_port(ppa_buf) == 137 || ppa_get_pkt_dst_port(ppa_buf) == 137
        || ppa_get_pkt_src_port(ppa_buf) == 138 || ppa_get_pkt_dst_port(ppa_buf) == 138 )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT,"src port = %d, dst port = %d\n", ppa_get_pkt_src_port(ppa_buf), ppa_get_pkt_dst_port(ppa_buf));
        return IFX_PPA_SESSION_FILTED;
    }

    //  ignore fragment packet
    if ( ppa_is_pkt_fragment(ppa_buf) )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT,"fragment\n");
        return IFX_PPA_SESSION_FILTED;
    }

    tx_if = ppa_get_pkt_dst_if(ppa_buf);
    if (tx_if)
    {
    	ifname = ppa_get_netif_name(tx_if);
    }
    else
    {
	goto skip_lookup;
    }
    if( ppa_netif_lookup(ifname, &p_info) != IFX_SUCCESS )
    {
	return IFX_ENOTAVAIL;
    }

    flag = p_info->flags;

skip_lookup:
    
    if(!(flags & PPA_F_BEFORE_NAT_TRANSFORM) ){
        //  handle routed packet only
    	if( !(flag & NETIF_PPPOL2TP) )
	{
            if ( !ppa_is_pkt_routing(ppa_buf) )
            {
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"not routing packet\n");
                return IFX_PPA_SESSION_FILTED;
            }
	}
    }

    return IFX_PPA_SESSION_NOT_FILTED; 

}

int32_t ppa_hook_set_ppe_fastpath_enable(uint32_t f_enable, uint32_t flags)
{
    g_ppe_fastpath_enabled = f_enable;
    return IFX_SUCCESS;
}

int32_t ppa_hook_get_ppe_fastpath_enable(uint32_t *f_enable, uint32_t flags)
{
    if( f_enable )
        *f_enable = g_ppe_fastpath_enabled;
    return IFX_SUCCESS;
}


/*
 *  PPA Routing Session Operation Functions
 */

int32_t ifx_ppa_session_add(PPA_BUF *ppa_buf, PPA_SESSION *p_session, uint32_t flags)
{
    int32_t ret;

    if(!ppa_buf){
        return IFX_PPA_SESSION_NOT_ADDED;
    }
     //get session
    if ( !p_session )
    {
        p_session = ppa_get_session(ppa_buf);
        if ( !p_session )
        {
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"no conntrack:%s\n", flags & PPA_F_BEFORE_NAT_TRANSFORM?"Before NAT":"After NAT");
            return IFX_PPA_SESSION_NOT_ADDED;
        }
    }

    /* Speed up software path. In case session exists and certain flags are set
           short-circuit the loop */
    if((ret = ifx_ppa_speed_handle_frame(ppa_buf, p_session, flags)) == IFX_PPA_SESSION_FILTED){
        return ret;
    }

    if((ret = ifx_ppa_pkt_filter(ppa_buf, flags)) == IFX_PPA_SESSION_FILTED){
        return ret;
    }

    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"%s\n", flags & PPA_F_BEFORE_NAT_TRANSFORM?"Before NAT":"After NAT");
    if ( (flags & PPA_F_BEFORE_NAT_TRANSFORM) )
    {
        if ( ppa_add_session(ppa_buf, p_session, flags) != IFX_SUCCESS )
        {
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ppa_add_session failed\n");
            return IFX_PPA_SESSION_NOT_ADDED;
        }
        ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT,"session exists for before-NAT\n");

        return IFX_PPA_SESSION_ADDED;
    }
    else
    {
        //  in case compiler optimization problem
        PPA_SYNC();
        ret = ifx_ppa_update_session(ppa_buf, p_session, flags);
        return ret;
        
    }
}

int32_t ifx_ppa_session_modify(PPA_SESSION *p_session, PPA_SESSION_EXTRA *p_extra, uint32_t flags)
{
    struct session_list_item *p_item;
    int32_t ret = IFX_FAILURE;

    ppa_uc_get_htable_lock();
    if ( __ppa_lookup_session(p_session, flags & PPA_F_SESSION_REPLY_DIR, &p_item) == IFX_PPA_SESSION_EXISTS )
    {
        ppa_update_session_extra(p_extra, p_item, flags);
        if ( (p_item->flags & SESSION_ADDED_IN_HW) && (flags != 0)  )
        {
            if( !(p_item->flags & SESSION_NOT_ACCEL_FOR_MGM) 
#ifdef CONFIG_LTQ_PPA_API_SW_FASTPATH
		&& !(p_item->flags & SESSION_ADDED_IN_SW)
#endif  
	   ) {
                if ( ppa_hw_update_session_extra(p_item, flags) != IFX_SUCCESS )
                {
                    //  update failed
                    ppa_hw_del_session(p_item);
                    p_item->flags &= ~SESSION_ADDED_IN_HW;
#ifdef CONFIG_LTQ_PPA_API_SW_FASTPATH
		    // session was in hardware an the modification failed; so the session is moved out of HW and put in SW fastpath 
                    p_item->flags |= SESSION_ADDED_IN_SW;
#endif
                    goto __MODIFY_DONE;
                }
            }
            else //just remove the accelerated session from PPE FW, no need to update other flags since PPA hook will rewrite them.
            {
                ppa_hw_del_session(p_item);
                p_item->flags &= ~SESSION_ADDED_IN_HW;
            }
        }
        ret = IFX_SUCCESS;
    }
    else{
        ret = IFX_FAILURE;
    }

__MODIFY_DONE:
    ppa_uc_release_htable_lock();
    return ret;
}

int32_t ifx_ppa_session_get(PPA_SESSION ***pp_sessions, PPA_SESSION_EXTRA **pp_extra, int32_t *p_num_entries, uint32_t flags)
{
//#warning ifx_ppa_session_get is not implemented
    return IFX_ENOTIMPL;
}


#if defined(CAP_WAP_CONFIG) && CAP_WAP_CONFIG

int32_t ifx_ppa_capwap_check_ip(PPA_CMD_CAPWAP_INFO *ppa_capwap_entry)
{
    if( is_ip_zero(&ppa_capwap_entry->source_ip) )
    {
        ppa_debug(DBG_ENABLE_MASK_DUMP_CAPWAP_GROUP, "ifx_ppa_capwap_group_update not support zero ip address\n");
        return IFX_FAILURE;
    }

    if( is_ip_zero(&ppa_capwap_entry->dest_ip) )
    {
        ppa_debug(DBG_ENABLE_MASK_DUMP_CAPWAP_GROUP, "ifx_ppa_capwap_group_update not support zero ip address\n");
        return IFX_FAILURE;
    }


    if(is_ip_allbit1(&ppa_capwap_entry->source_ip)){
        if(g_ppa_dbg_enable & DBG_ENABLE_MASK_DUMP_CAPWAP_GROUP){
            ppa_debug(DBG_ENABLE_MASK_DUMP_CAPWAP_GROUP, "source ip not support all bit 1 ip address, except dbg enabled\n");
        }else{
            return IFX_FAILURE;
        }
    }

    if(is_ip_allbit1(&ppa_capwap_entry->dest_ip)){
        if(g_ppa_dbg_enable & DBG_ENABLE_MASK_DUMP_CAPWAP_GROUP){
            ppa_debug(DBG_ENABLE_MASK_DUMP_CAPWAP_GROUP, "destination ip not support all bit 1 ip address, except dbg enabled\n");
        }else{
            return IFX_FAILURE;
        }
    }

    return IFX_SUCCESS;
}

int32_t ifx_ppa_capwap_add_entry(PPA_CMD_CAPWAP_INFO *ppa_capwap_entry)
{
    struct capwap_group_list_item *p_item;
    //  add new capwap groups
    if ( ppa_add_capwap_group(ppa_capwap_entry, &p_item) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DUMP_CAPWAP_GROUP, "ppa_add_capwap_group fail\n");
        return IFX_PPA_CAPWAP_NOT_ADDED;
    }

    if ( ifx_ppa_drv_add_capwap_entry(ppa_capwap_entry,0 ) == IFX_SUCCESS )
    {
         p_item->p_entry = ppa_capwap_entry->p_entry;
    }
    else
    {
        ppa_debug(DBG_ENABLE_MASK_DUMP_CAPWAP_GROUP,
                        "ppa_hw_add_capwap_group(%d.%d.%d.%d): fail",
                        ppa_capwap_entry->source_ip.ip.ip >> 24,
                        (ppa_capwap_entry->source_ip.ip.ip >> 16) & 0xFF,
                        (ppa_capwap_entry->source_ip.ip.ip >> 8) & 0xFF,
                        ppa_capwap_entry->source_ip.ip.ip & 0xFF);
        ppa_remove_capwap_group(p_item);
        return IFX_PPA_CAPWAP_NOT_ADDED;
    }
    ppa_debug(DBG_ENABLE_MASK_DUMP_CAPWAP_GROUP, "capwap hardware added\n");

    return IFX_PPA_CAPWAP_ADDED;
}

int32_t ifx_ppa_capwap_del_entry(PPA_CMD_CAPWAP_INFO *ppa_capwap_entry,struct
                capwap_group_list_item *p_item)
{
    
    ppa_capwap_entry->p_entry = p_item->p_entry;

    if ( ifx_ppa_drv_delete_capwap_entry(ppa_capwap_entry,0 ) != IFX_SUCCESS )
         return IFX_FAILURE;
    
    ppa_list_delete(&p_item->capwap_list);
    ppa_free_capwap_group_list_item(p_item); 
    
    return IFX_SUCCESS;
    
}


/*
 *  PPA CAPWAP configuration Functions
 */
int32_t ifx_ppa_capwap_update(PPA_CMD_CAPWAP_INFO *ppa_capwap_entry)
{
    struct capwap_group_list_item *p_item;
    int32_t ret;

    { //for print debug information only
            ppa_debug(DBG_ENABLE_MASK_DUMP_CAPWAP_GROUP, "ifx_ppa_capwap_update for source ip: %d.%d.%d.%d \n", NIPQUAD(ppa_capwap_entry->source_ip.ip.ip));
            ppa_debug(DBG_ENABLE_MASK_DUMP_CAPWAP_GROUP, "destination ip : %d.%d.%d.%d \n", NIPQUAD(ppa_capwap_entry->dest_ip.ip.ip));

    }

    if(ifx_ppa_capwap_check_ip(ppa_capwap_entry) != IFX_SUCCESS)
        return IFX_FAILURE;
    

    ppa_capwap_get_table_lock();
    ret = __ppa_lookup_capwap_group_add(&ppa_capwap_entry->source_ip, &ppa_capwap_entry->dest_ip,ppa_capwap_entry->directpath_portid,&p_item);
    if ( ret  == IFX_PPA_CAPWAP_NOT_ADDED ){
        ret =  ifx_ppa_capwap_add_entry(ppa_capwap_entry);
        if (ret == IFX_PPA_CAPWAP_NOT_ADDED)
                ret = IFX_FAILURE;
        else
                ret = IFX_SUCCESS;
    }
    else
    {
        ret = __ppa_capwap_group_update(ppa_capwap_entry, p_item);
    }

    ppa_capwap_release_table_lock();

    return ret;
}

int32_t ifx_ppa_capwap_delete(PPA_CMD_CAPWAP_INFO *ppa_capwap_entry)
{
    struct capwap_group_list_item *p_item;
    int32_t ret;

    { //for print debug information only
            ppa_debug(DBG_ENABLE_MASK_DUMP_CAPWAP_GROUP, "ifx_ppa_capwap_delete for source ip: %d.%d.%d.%d \n", NIPQUAD(ppa_capwap_entry->source_ip.ip.ip));
            ppa_debug(DBG_ENABLE_MASK_DUMP_CAPWAP_GROUP, "destination ip : %d.%d.%d.%d \n", NIPQUAD(ppa_capwap_entry->dest_ip.ip.ip));


    }

    if(ifx_ppa_capwap_check_ip(ppa_capwap_entry) != IFX_SUCCESS)
        return IFX_FAILURE;
    
    ppa_capwap_get_table_lock();
    
    ret = __ppa_lookup_capwap_group(&ppa_capwap_entry->source_ip, &ppa_capwap_entry->dest_ip,ppa_capwap_entry->directpath_portid, &p_item);
    if ( ret  == IFX_PPA_CAPWAP_EXISTS ){
        ret =  ifx_ppa_capwap_del_entry(ppa_capwap_entry,p_item);
    }
    else
            ret = IFX_FAILURE;

    ppa_capwap_release_table_lock();

    return ret;
}


#endif


int32_t ifx_ppa_mc_check_ip(PPA_MC_GROUP *ppa_mc_entry)
{
    if( is_ip_zero(&ppa_mc_entry->ip_mc_group) )
    {
        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "ifx_ppa_mc_group_update not support zero ip address\n");
        return IFX_FAILURE;
    }

    if(is_ip_allbit1(&ppa_mc_entry->source_ip)){
        if(g_ppa_dbg_enable & DBG_ENABLE_MASK_DUMP_MC_GROUP){
            ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "source ip not support all bit 1 ip address, except dbg enabled\n");
        }else{
            return IFX_FAILURE;
        }
    }
    if(is_ip_zero(&ppa_mc_entry->source_ip)){
		if ( ppa_mc_entry->SSM_flag == 1 ){//Must provide src ip if SSM_flag is set
		    ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "SMM flag set but no souce ip provided\n");
			return IFX_FAILURE;
		}
    }else if(ppa_mc_entry->ip_mc_group.f_ipv6 != ppa_mc_entry->source_ip.f_ipv6){ //mc group ip & source ip must both be ipv4 or ipv6
        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "MC group IP and source ip not in same IPv4/IPv6 type\n");
		return IFX_FAILURE;
	}

    return IFX_SUCCESS;
}


int32_t ifx_ppa_mc_add_entry(PPA_MC_GROUP *ppa_mc_entry, uint32_t flags)
{
    struct mc_group_list_item *p_item;
    //  add new mc groups
    if ( ppa_add_mc_group(ppa_mc_entry, &p_item, flags) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "ppa_add_mc_group fail\n");
        return IFX_PPA_SESSION_NOT_ADDED;
    }

    if ( p_item->src_netif == NULL )    // only added in PPA level, not PPE FW level since source interface not get yet.
    {
        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "IGMP request no src_netif. No acceleration !\n");
        return IFX_PPA_SESSION_ADDED;
    }
    if ( ppa_hw_add_mc_group(p_item) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "ppa_hw_add_mc_group(%d.%d.%d.%d): fail", ppa_mc_entry->ip_mc_group.ip.ip >> 24, (ppa_mc_entry->ip_mc_group.ip.ip >> 16) & 0xFF, (ppa_mc_entry->ip_mc_group.ip.ip >> 8) & 0xFF, ppa_mc_entry->ip_mc_group.ip.ip & 0xFF);
        ppa_remove_mc_group(p_item);
        return IFX_PPA_SESSION_NOT_ADDED;
    }
    ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "hardware added\n");

    return IFX_PPA_SESSION_ADDED;
}

/*
 *  PPA Multicast Routing Session Operation Functions
 */

int32_t ifx_ppa_mc_group_update(PPA_MC_GROUP *ppa_mc_entry, uint32_t flags)
{
    struct mc_group_list_item *p_item;
    int32_t ret;

    { //for print debug information only
        if(ppa_mc_entry->ip_mc_group.f_ipv6 == 0){
            ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "ifx_ppa_mc_group_update for group: %d.%d.%d.%d \n", NIPQUAD(ppa_mc_entry->ip_mc_group.ip.ip));
            ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "source ip : %d.%d.%d.%d \n", NIPQUAD(ppa_mc_entry->source_ip.ip.ip));
        }else{
            ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "group ip: "NIP6_FMT"\n", NIP6(ppa_mc_entry->ip_mc_group.ip.ip6));
            ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "source ip: "NIP6_FMT"\n", NIP6(ppa_mc_entry->source_ip.ip.ip6));
        }
        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "from %s ", ppa_mc_entry->src_ifname ? ppa_mc_entry->src_ifname: "NULL" );

        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "to ");
        if( ppa_mc_entry->num_ifs ==0 ) ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "NULL" );
        else
        {
            int i, bit;
            for(i=0, bit=1; i<ppa_mc_entry->num_ifs; i++ )
            {
                if ( ppa_mc_entry->if_mask & bit )
                    ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "%s ", ppa_mc_entry->array_mem_ifs[i].ifname? ppa_mc_entry->array_mem_ifs[i].ifname:"NULL");

                bit = bit<<1;
            }
        }
        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "with ssm=%d flags=%x\n",  ppa_mc_entry->SSM_flag, flags );
        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "lan itf num:%d, mask:%x\n",  ppa_mc_entry->num_ifs, ppa_mc_entry->if_mask);
    }

    if(ifx_ppa_mc_check_ip(ppa_mc_entry) != IFX_SUCCESS)
        return IFX_FAILURE;

    if(ppa_mc_entry->num_ifs == 0){//delete action: if SMM flag == 0, don't care src ip
        ppa_delete_mc_group(ppa_mc_entry);
        return IFX_SUCCESS;
    }
    

    ppa_mc_get_htable_lock();
    ret = __ppa_lookup_mc_group(&ppa_mc_entry->ip_mc_group, &ppa_mc_entry->source_ip, &p_item);
    if(ret == IFX_PPA_MC_SESSION_VIOLATION){//Cannot add or update
        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "IGMP violation, cannot be added or updated\n");
        ret = IFX_FAILURE;
    }
    else if ( ret  != IFX_PPA_SESSION_EXISTS ){
        ret =  ifx_ppa_mc_add_entry(ppa_mc_entry, flags);
    }
    else
    {
        ret = __ppa_mc_group_update(ppa_mc_entry, p_item, flags);
    }

    ppa_mc_release_htable_lock();

    return ret;
}

int32_t ifx_ppa_mc_group_get(IP_ADDR_C  ip_mc_group, IP_ADDR_C src_ip, PPA_MC_GROUP *ppa_mc_entry, uint32_t flags)
{
    struct mc_group_list_item *p_item = NULL;
    PPA_IFNAME *ifname;
    uint32_t idx;
    uint32_t bit;
    uint32_t i;

    ASSERT(ppa_mc_entry != NULL, "ppa_mc_entry == NULL");

    ppa_mc_get_htable_lock();
    if ( __ppa_lookup_mc_group(&ip_mc_group, &src_ip,  &p_item) != IFX_PPA_SESSION_EXISTS ){
        ppa_mc_release_htable_lock();
        return IFX_ENOTAVAIL;
    }

    ppa_memcpy( &ppa_mc_entry->ip_mc_group, &p_item->ip_mc_group, sizeof( ppa_mc_entry->ip_mc_group  ) );
    ppa_memcpy( &ppa_mc_entry->source_ip, &p_item->source_ip, sizeof( ppa_mc_entry->ip_mc_group ));


    for ( i = 0, bit = 1, idx = 0; i < PPA_MAX_MC_IFS_NUM; i++, bit <<= 1 )
        if ( (p_item->if_mask & bit) )
        {
            ifname = ppa_get_netif_name(p_item->netif[i]);
            if ( ifname )
            {
                ppa_mc_entry->array_mem_ifs[idx].ifname = ifname;
                ppa_mc_entry->array_mem_ifs[idx].ttl    = p_item->ttl[i];
                ppa_mc_entry->if_mask |= (1 << idx);
                idx++;
            }
        }

    ppa_mc_entry->num_ifs = idx;
    //ppa_mc_entry->if_mask = (1 << idx) - 1;
    ppa_mc_release_htable_lock();

    return IFX_SUCCESS;
}

int32_t ifx_ppa_mc_entry_modify(IP_ADDR_C ip_mc_group, IP_ADDR_C src_ip, PPA_MC_GROUP *ppa_mc_entry, PPA_SESSION_EXTRA *p_extra, uint32_t flags)
{
    struct mc_group_list_item *p_item;
    int32_t ret = IFX_FAILURE;

    ppa_mc_get_htable_lock();
    if ( __ppa_lookup_mc_group(&ip_mc_group, &src_ip, &p_item) == IFX_PPA_SESSION_EXISTS )
    {
        ppa_update_mc_group_extra(p_extra, p_item, flags);
        if ( (p_item->flags & SESSION_ADDED_IN_HW) )
        {
            if ( ppa_hw_update_mc_group_extra(p_item, flags) != IFX_SUCCESS )
            {
                ppa_remove_mc_group(p_item);
                ret = IFX_FAILURE;
            }
        }
        ret = IFX_SUCCESS;
    }
    ppa_mc_release_htable_lock();

    return ret;
}

int32_t ifx_ppa_mc_entry_get(IP_ADDR_C ip_mc_group, IP_ADDR_C src_ip, PPA_SESSION_EXTRA *p_extra, uint32_t flags)
{
    struct mc_group_list_item *p_item;
    int32_t ret = IFX_FAILURE;

    if ( !p_extra )
        return IFX_EINVAL;

    ppa_mc_get_htable_lock();
    if ( __ppa_lookup_mc_group(&ip_mc_group, &src_ip, &p_item) == IFX_PPA_SESSION_EXISTS )
    {
        ppa_memset(p_extra, 0, sizeof(*p_extra));

        p_extra->session_flags = flags;

        if ( (flags & PPA_F_SESSION_NEW_DSCP) )
        {
            if ( (p_item->flags & SESSION_VALID_NEW_DSCP) )
            {
                p_extra->dscp_remark = 1;
                p_extra->new_dscp = p_item->new_dscp;
            }
        }

        if ( (flags & PPA_F_SESSION_VLAN) )
        {
            if ( (p_item->flags & SESSION_VALID_VLAN_INS) )
            {
                p_extra->vlan_insert = 1;
                p_extra->vlan_prio   = p_item->new_vci >> 13;
                p_extra->vlan_cfi    = (p_item->new_vci >> 12) & 0x01;
                p_extra->vlan_id     = p_item->new_vci & ((1 << 12) - 1);
            }

            if ( (p_item->flags & SESSION_VALID_VLAN_RM) )
                p_extra->vlan_remove = 1;
        }

        if ( (flags & PPA_F_SESSION_OUT_VLAN) )
        {
            if ( (p_item->flags & SESSION_VALID_OUT_VLAN_INS) )
            {
                p_extra->out_vlan_insert = 1;
                p_extra->out_vlan_tag    = p_item->out_vlan_tag;
            }

            if ( (p_item->flags & SESSION_VALID_OUT_VLAN_RM) )
                p_extra->out_vlan_remove = 1;
        }

         p_extra->dslwan_qid_remark = 1;
         p_extra->dslwan_qid        = p_item->dslwan_qid;
         p_extra->out_vlan_tag      = p_item->out_vlan_tag;

        ret = IFX_SUCCESS;
    }
    
    ppa_mc_release_htable_lock();

    return ret;
}

#if defined(RTP_SAMPLING_ENABLE) && RTP_SAMPLING_ENABLE
int32_t ifx_ppa_mc_entry_rtp_get(IP_ADDR_C ip_mc_group, IP_ADDR_C src_ip, uint8_t* p_RTP_flag)
{
    struct mc_group_list_item *p_item;
    int32_t ret = IFX_FAILURE;

    if ( !p_RTP_flag )
        return IFX_EINVAL;

    ppa_mc_get_htable_lock();
    if ( __ppa_lookup_mc_group(&ip_mc_group, &src_ip, &p_item) == IFX_PPA_SESSION_EXISTS )
    {
        *p_RTP_flag = p_item->RTP_flag;
        ret = IFX_SUCCESS;
    }
    
    ppa_mc_release_htable_lock();

    return ret;
}

#endif


/*
 *  PPA Unicast Session Timeout Functions
 */

int32_t ifx_ppa_inactivity_status(PPA_U_SESSION *p_session)
{
    int f_flag;
    int f_timeout;
    int32_t ret, ret_reply;
    struct session_list_item *p_item, *p_item_reply;

    ppa_uc_get_htable_lock();
    p_item = p_item_reply = NULL;
    ret = __ppa_lookup_session((PPA_SESSION *)p_session, 0, &p_item);

    if ( (ret == IFX_PPA_SESSION_EXISTS && p_item->ip_proto == IFX_IPPROTO_TCP) 
          && !ppa_is_tcp_established(p_session) ){
          
          ppa_uc_release_htable_lock();
          return IFX_PPA_TIMEOUT;
    }

    f_flag = 0;
    f_timeout = 1;

    if ( ret == IFX_PPA_SESSION_EXISTS && (p_item->flags & (SESSION_ADDED_IN_HW | SESSION_ADDED_IN_SW)) )
    {
        uint32_t tmp;

        f_flag = 1;

        tmp = ppa_get_time_in_sec();
        if ( p_item->timeout >= tmp - p_item->last_hit_time)
        {
            f_timeout = 0;
            ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT,"session %08x, p_item->timeout (%d) + p_item->last_hit_time (%d) > ppa_get_time_in_sec (%d)\n", (unsigned int)p_session, p_item->timeout, p_item->last_hit_time, tmp);
        }
        else
        {
            ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT,"session %08x, p_item->timeout (%d) + p_item->last_hit_time (%d) <= ppa_get_time_in_sec (%d)\n", (unsigned int)p_session, p_item->timeout, p_item->last_hit_time, tmp);
        }
    }

    
    ret_reply = __ppa_lookup_session((PPA_SESSION *)p_session, 1, &p_item_reply);

    if ( ret_reply == IFX_PPA_SESSION_EXISTS && (p_item_reply->flags & (SESSION_ADDED_IN_HW | SESSION_ADDED_IN_SW)) )
    {
        uint32_t tmp;

        f_flag = 1;

        tmp = ppa_get_time_in_sec();
        if ( p_item_reply->timeout >= tmp - p_item_reply->last_hit_time)
        {
            f_timeout = 0;
            ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT,"session %08x, p_item_reply->timeout (%d) + p_item_reply->last_hit_time (%d) > ppa_get_time_in_sec (%d)\n", (unsigned int)p_session, p_item_reply->timeout, p_item_reply->last_hit_time, tmp);
        }
        else
        {
            ppa_debug(DBG_ENABLE_MASK_DEBUG2_PRINT,"session %08x, p_item_reply->timeout (%d) + p_item_reply->last_hit_time (%d) <= ppa_get_time_in_sec (%d)\n", (unsigned int)p_session, p_item_reply->timeout, p_item_reply->last_hit_time, tmp);
        }
    }

    ppa_uc_release_htable_lock();

    if(g_ppa_dbg_enable & DBG_ENABLE_MASK_SESSION){//if session dbg enable, keep it from timeout
        return IFX_PPA_HIT;
    }
    //  not added in hardware
    if ( !f_flag )
        return IFX_PPA_SESSION_NOT_ADDED;

    return f_timeout ? IFX_PPA_TIMEOUT : IFX_PPA_HIT;
    
}

int32_t ifx_ppa_set_session_inactivity(PPA_U_SESSION *p_session, int32_t timeout)
{
    int32_t ret, ret_reply;
    struct session_list_item *p_item, *p_item_reply;

    if( p_session == NULL ) //for modifying ppa routing hook timer purpose
    {
        ppa_set_polling_timer(timeout, 1);
        return IFX_SUCCESS;
    }

    ppa_uc_get_htable_lock();
    ret = __ppa_lookup_session((PPA_SESSION *)p_session, 0, &p_item);
    if (ret == IFX_PPA_SESSION_EXISTS){
        p_item->timeout = timeout;
    }
    
    ret_reply = __ppa_lookup_session((PPA_SESSION *)p_session, 1, &p_item_reply);

    if (ret_reply == IFX_PPA_SESSION_EXISTS){
        p_item_reply->timeout = timeout;
    }
    ppa_uc_release_htable_lock();
    
    ppa_set_polling_timer(timeout, 0);

    return IFX_SUCCESS;
}

/*
 *  PPA Bridge Session Operation Functions
 */

int32_t ifx_ppa_bridge_entry_add(uint8_t *mac_addr, PPA_NETIF *netif, uint32_t flags)
{
    struct bridging_session_list_item *p_item;
    int32_t ret = IFX_PPA_SESSION_ADDED;
    

    if( !g_bridging_mac_learning ) return IFX_FAILURE;

    ppa_br_get_htable_lock();
    if ( __ppa_bridging_lookup_session(mac_addr, netif, &p_item) == IFX_PPA_SESSION_EXISTS )
    {
        if ( (p_item->flags & SESSION_ADDED_IN_HW) ){    //  added in hardware/firmware
            ret = IFX_PPA_SESSION_ADDED;
            goto __BR_SESSION_ADD_DONE;
        }
    }
    else if ( ppa_bridging_add_session(mac_addr, netif, &p_item, flags) != 0 ){
        ret = IFX_PPA_SESSION_NOT_ADDED;
        goto __BR_SESSION_ADD_DONE;
    }

    if ( ppa_bridging_hw_add_session(p_item) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ppa_bridging_hw_add_session(%02x:%02x:%02x:%02x:%02x:%02x): fail\n", (uint32_t)p_item->mac[0], (uint32_t)p_item->mac[1], (uint32_t)p_item->mac[2], (uint32_t)p_item->mac[3], (uint32_t)p_item->mac[4], (uint32_t)p_item->mac[5]);
        ret =  IFX_PPA_SESSION_NOT_ADDED;
        goto __BR_SESSION_ADD_DONE;
    }

    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"hardware added\n");

__BR_SESSION_ADD_DONE:
    ppa_br_release_htable_lock();
    return ret;
}

int32_t ifx_ppa_hook_bridge_enable(uint32_t f_enable, uint32_t flags)
{
    g_bridging_mac_learning = f_enable;
    return IFX_SUCCESS;
}

int32_t ifx_ppa_hook_get_bridge_status(uint32_t *f_enable, uint32_t flags)
{
    if( f_enable )
        *f_enable = g_bridging_mac_learning;
    return IFX_SUCCESS;
}

int32_t ifx_ppa_bridge_entry_delete(uint8_t *mac_addr, uint32_t flags)
{
    struct bridging_session_list_item *p_item;
    int32_t ret = IFX_FAILURE;
    
    if( !g_bridging_mac_learning ) return IFX_FAILURE;

    ppa_br_get_htable_lock();
    if ( __ppa_bridging_lookup_session(mac_addr, NULL, &p_item) != IFX_PPA_SESSION_EXISTS ){
        goto __BR_SESSION_DELETE_DONE;
    }

    dump_bridging_list_item(p_item, "ifx_ppa_bridge_entry_delete");

    //  ppa_bridging_remove_session->ppa_bridging_free_session_list_item->ppa_bridging_hw_del_session will delete MAC entry from Firmware/Hardware
    ppa_bridging_remove_session(p_item);
    ret = IFX_SUCCESS;

__BR_SESSION_DELETE_DONE:
    ppa_br_release_htable_lock();

    return ret;
}

int32_t ifx_ppa_bridge_entry_hit_time(uint8_t *mac_addr, uint32_t *p_hit_time)
{
    struct bridging_session_list_item *p_item;
    int32_t ret = IFX_PPA_SESSION_NOT_ADDED;

    ppa_br_get_htable_lock();
    if ( __ppa_bridging_lookup_session(mac_addr, NULL, &p_item) == IFX_PPA_SESSION_EXISTS ){
        *p_hit_time = p_item->last_hit_time;
        ret = IFX_PPA_HIT;
    }

    ppa_br_release_htable_lock();
    return ret;
}

int32_t ifx_ppa_bridge_entry_inactivity_status(uint8_t *mac_addr)
{
    struct bridging_session_list_item *p_item;
    int32_t ret = IFX_PPA_HIT;

    ppa_br_get_htable_lock();
    if ( __ppa_bridging_lookup_session(mac_addr, NULL, &p_item) != IFX_PPA_SESSION_EXISTS ){
        ret = IFX_PPA_SESSION_NOT_ADDED;
        goto __BR_INACTIVITY_DONE;
    }

    //  not added in hardware
    if ( !(p_item->flags & SESSION_ADDED_IN_HW) ){
        ret = IFX_PPA_SESSION_NOT_ADDED;
        goto __BR_INACTIVITY_DONE;
    }

    if ( (p_item->flags & SESSION_STATIC) ){
        ret = IFX_PPA_HIT;
        goto __BR_INACTIVITY_DONE;
    }

    if ( p_item->timeout < ppa_get_time_in_sec() - p_item->last_hit_time ){  //  use < other than <= to avoid "false positives"
        ret = IFX_PPA_TIMEOUT;
        goto __BR_INACTIVITY_DONE;
    }

__BR_INACTIVITY_DONE:
    ppa_br_release_htable_lock();
    return ret;
}

int32_t ifx_ppa_set_bridge_entry_timeout(uint8_t *mac_addr, uint32_t timeout)
{
    struct bridging_session_list_item *p_item;
    int32_t ret = IFX_SUCCESS;
    
    ppa_br_get_htable_lock();
    if ( __ppa_bridging_lookup_session(mac_addr, NULL, &p_item) != IFX_PPA_SESSION_EXISTS ){
        ret = IFX_FAILURE;
        goto __BR_TIMEOUT_DONE;
    }
    
    ppa_br_release_htable_lock();
    if ( !(p_item->flags & SESSION_STATIC) )
        p_item->timeout = timeout;

    ppa_bridging_set_polling_timer(timeout);

    return IFX_SUCCESS;

__BR_TIMEOUT_DONE:
    ppa_br_release_htable_lock();

    return IFX_SUCCESS;
}

/*
 *  PPA Bridge VLAN Config Functions
 */

int32_t ifx_ppa_set_bridge_if_vlan_config(PPA_NETIF *netif, PPA_VLAN_TAG_CTRL *vlan_tag_control, PPA_VLAN_CFG *vlan_cfg, uint32_t flags)
{
    struct netif_info *ifinfo;
    PPE_BRDG_VLAN_CFG cfg={0};

    if ( ppa_netif_update(netif, NULL) != IFX_SUCCESS )
        return IFX_FAILURE;

    if ( ppa_netif_lookup(ppa_get_netif_name(netif), &ifinfo) != IFX_SUCCESS )
        return IFX_FAILURE;

    if ( !(ifinfo->flags & NETIF_PHYS_PORT_GOT) )
    {
        ppa_netif_put(ifinfo);
        return IFX_FAILURE;
    }

    cfg.if_no = ifinfo->phys_port;
    cfg.f_eg_vlan_insert= vlan_tag_control->insertion | vlan_tag_control->replace;
    cfg.f_eg_vlan_remove= vlan_tag_control->remove | vlan_tag_control->replace;
    cfg.f_ig_vlan_aware = vlan_cfg->vlan_aware | (vlan_tag_control->unmodified ? 0 : 1) | vlan_tag_control->insertion | vlan_tag_control->remove | vlan_tag_control->replace |(vlan_tag_control->out_unmodified ? 0 : 1) | vlan_tag_control->out_insertion | vlan_tag_control->out_remove | vlan_tag_control->out_replace;
    cfg.f_ig_src_ip_based = vlan_cfg->src_ip_based_vlan,
    cfg.f_ig_eth_type_based= vlan_cfg->eth_type_based_vlan;
    cfg.f_ig_vlanid_based = vlan_cfg->vlanid_based_vlan;
    cfg.f_ig_port_based= vlan_cfg->port_based_vlan;
    cfg.f_eg_out_vlan_insert = vlan_tag_control->out_insertion | vlan_tag_control->out_replace;
    cfg.f_eg_out_vlan_remove = vlan_tag_control->out_remove | vlan_tag_control->out_replace;
    cfg.f_ig_out_vlan_aware = vlan_cfg->out_vlan_aware | (vlan_tag_control->out_unmodified ? 0 : 1) | vlan_tag_control->out_insertion | vlan_tag_control->out_remove | vlan_tag_control->out_replace;
    ifx_ppa_drv_set_bridge_if_vlan_config( &cfg, 0);

    ppa_netif_put(ifinfo);

    return IFX_SUCCESS;
}

int32_t ifx_ppa_get_bridge_if_vlan_config(PPA_NETIF *netif, PPA_VLAN_TAG_CTRL *vlan_tag_control, PPA_VLAN_CFG *vlan_cfg, uint32_t flags)
{
    struct netif_info *ifinfo;
    PPE_BRDG_VLAN_CFG cfg={0};

    if ( ppa_netif_update(netif, NULL) != IFX_SUCCESS )
        return IFX_FAILURE;

    if ( ppa_netif_lookup(ppa_get_netif_name(netif), &ifinfo) != IFX_SUCCESS )
        return IFX_FAILURE;

    if ( !(ifinfo->flags & NETIF_PHYS_PORT_GOT) )
    {
        ppa_netif_put(ifinfo);
        return IFX_FAILURE;
    }

    cfg.if_no = ifinfo->phys_port;
    ifx_ppa_drv_get_bridge_if_vlan_config( &cfg, 0);

    vlan_tag_control->unmodified    = cfg.f_ig_vlan_aware ? 0 : 1;
    vlan_tag_control->insertion     = cfg.f_eg_vlan_insert ? 1 : 0;
    vlan_tag_control->remove        = cfg.f_eg_vlan_remove ? 1 : 0;
    vlan_tag_control->replace       = cfg.f_eg_vlan_insert && cfg.f_eg_vlan_remove ? 1 : 0;
    vlan_cfg->vlan_aware            = cfg.f_ig_vlan_aware ? 1 : 0;
    vlan_cfg->src_ip_based_vlan     = cfg.f_ig_src_ip_based ? 1 : 0;
    vlan_cfg->eth_type_based_vlan   = cfg.f_ig_eth_type_based ? 1 : 0;
    vlan_cfg->vlanid_based_vlan     = cfg.f_ig_vlanid_based ? 1 : 0;
    vlan_cfg->port_based_vlan       = cfg.f_ig_port_based ? 1 : 0;
    vlan_tag_control->out_unmodified    = cfg.f_ig_out_vlan_aware ? 0 : 1;
    vlan_tag_control->out_insertion     = cfg.f_eg_out_vlan_insert ? 1 : 0;
    vlan_tag_control->out_remove        = cfg.f_eg_out_vlan_remove ? 1 : 0;
    vlan_tag_control->out_replace       = cfg.f_eg_out_vlan_insert && cfg.f_eg_out_vlan_remove ? 1 : 0;
    vlan_cfg->out_vlan_aware            = cfg.f_ig_out_vlan_aware ? 1 : 0;

    ppa_netif_put(ifinfo);

    return IFX_SUCCESS;
}

int32_t ifx_ppa_vlan_filter_add(PPA_VLAN_MATCH_FIELD *vlan_match_field, PPA_VLAN_INFO *vlan_info, uint32_t flags)
{
    struct netif_info *ifinfo;
    int i;
    PPE_BRDG_VLAN_FILTER_MAP vlan_filter={0};

    vlan_filter.out_vlan_info.vlan_id= vlan_info->out_vlan_id;
    if ( ifx_ppa_drv_add_outer_vlan_entry( &vlan_filter.out_vlan_info , 0) != IFX_SUCCESS )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"add out_vlan_ix error for out vlan id:%d\n", vlan_info->out_vlan_id);
        return IFX_FAILURE;
    }

    switch ( vlan_match_field->match_flags )
    {
    case PPA_F_VLAN_FILTER_IFNAME:
        if ( ppa_netif_update(NULL, vlan_match_field->match_field.ifname) != IFX_SUCCESS )
            return IFX_FAILURE;
        if ( ppa_netif_lookup(vlan_match_field->match_field.ifname, &ifinfo) != IFX_SUCCESS )
            return IFX_FAILURE;
        if ( !(ifinfo->flags & NETIF_PHYS_PORT_GOT) )
        {
            ppa_netif_put(ifinfo);
            return IFX_FAILURE;
        }
        vlan_filter.ig_criteria_type    = IFX_PPA_BRG_VLAN_IG_COND_TYPE_DEF;
        vlan_filter.ig_criteria         = ifinfo->phys_port;
        ppa_netif_put(ifinfo);
        break;
    case PPA_F_VLAN_FILTER_IP_SRC:
        vlan_filter.ig_criteria_type    = IFX_PPA_BRG_VLAN_IG_COND_TYPE_SRC_IP;
        vlan_filter.ig_criteria         = vlan_match_field->match_field.ip_src;
        break;
    case PPA_F_VLAN_FILTER_ETH_PROTO:
        vlan_filter.ig_criteria_type    = IFX_PPA_BRG_VLAN_IG_COND_TYPE_ETH_TYPE;
        vlan_filter.ig_criteria         = vlan_match_field->match_field.eth_protocol;
        break;
    case PPA_F_VLAN_FILTER_VLAN_TAG:
        vlan_filter.ig_criteria_type    = IFX_PPA_BRG_VLAN_IG_COND_TYPE_VLAN;
        vlan_filter.ig_criteria         = vlan_match_field->match_field.ingress_vlan_tag;
        break;
    default:
        return IFX_FAILURE;
    }

    vlan_filter.new_vci             = vlan_info->vlan_vci;

    vlan_filter.vlan_port_map = 0;
    for ( i = 0; i < vlan_info->num_ifs; i++ )
    {
        if ( ppa_netif_update(NULL, vlan_info->vlan_if_membership[i].ifname) != IFX_SUCCESS )
            continue;
        if ( ppa_netif_lookup(vlan_info->vlan_if_membership[i].ifname, &ifinfo) != IFX_SUCCESS )
            continue;
        if ( (ifinfo->flags & NETIF_PHYS_PORT_GOT) )
            vlan_filter.vlan_port_map |= 1 << ifinfo->phys_port;
        ppa_netif_put(ifinfo);
    }

    vlan_filter.dest_qos = vlan_info->qid;
    vlan_filter.in_out_etag_ctrl = vlan_info->inner_vlan_tag_ctrl | vlan_info->out_vlan_tag_ctrl;

    return ifx_ppa_drv_add_vlan_map( &vlan_filter, 0);
}

int32_t ifx_ppa_vlan_filter_del(PPA_VLAN_MATCH_FIELD *vlan_match_field, PPA_VLAN_INFO *vlan_info, uint32_t flags)
{
    PPE_BRDG_VLAN_FILTER_MAP filter={0};
    PPE_VFILTER_COUNT_CFG vfilter_count={0};
    struct netif_info *ifinfo;
    uint32_t i;
    uint8_t bfMatch=0;
    int32_t res;

    switch ( vlan_match_field->match_flags )
    {
    case PPA_F_VLAN_FILTER_IFNAME:
        if ( ppa_netif_update(NULL, vlan_match_field->match_field.ifname) != IFX_SUCCESS )
            return IFX_FAILURE;
        if ( ppa_netif_lookup(vlan_match_field->match_field.ifname, &ifinfo) != IFX_SUCCESS )
            return IFX_FAILURE;
        if ( !(ifinfo->flags & NETIF_PHYS_PORT_GOT) )
        {
            ppa_netif_put(ifinfo);
            return IFX_FAILURE;
        }
        filter.ig_criteria_type    = IFX_PPA_BRG_VLAN_IG_COND_TYPE_DEF;
        filter.ig_criteria         = ifinfo->phys_port;
        ppa_netif_put(ifinfo);
        break;
    case PPA_F_VLAN_FILTER_IP_SRC:
        filter.ig_criteria_type    = IFX_PPA_BRG_VLAN_IG_COND_TYPE_SRC_IP;
        filter.ig_criteria         = vlan_match_field->match_field.ip_src;
        break;
    case PPA_F_VLAN_FILTER_ETH_PROTO:
        filter.ig_criteria_type    = IFX_PPA_BRG_VLAN_IG_COND_TYPE_ETH_TYPE;
        filter.ig_criteria         = vlan_match_field->match_field.eth_protocol;
        break;
    case PPA_F_VLAN_FILTER_VLAN_TAG:
        filter.ig_criteria_type    = IFX_PPA_BRG_VLAN_IG_COND_TYPE_VLAN;
        filter.ig_criteria         = vlan_match_field->match_field.ingress_vlan_tag;
        break;
    default:
        return IFX_FAILURE;
    }

    //check whether there is such kind of vlan filter to delete
    vfilter_count.vfitler_type = filter.ig_criteria_type;
    ifx_ppa_drv_get_max_vfilter_entries( &vfilter_count, 0 );
    for ( i = 0; i < vfilter_count.num; i++ )
    {
        filter.entry = i;
        if ( (res = ifx_ppa_drv_get_vlan_map(&filter , 0) ) != -1 ) //get fail. break;
        {
            bfMatch = 1;
            break;
        }
    }

    if ( !bfMatch )
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_vlan_filter_del: canot find such kinds of vlan filter\n");
        return IFX_FAILURE;
    }
    if ( res == 0 ) //blank item
        return IFX_SUCCESS;

    ifx_ppa_drv_del_vlan_map( &filter, 0 );

    ifx_ppa_drv_del_outer_vlan_entry( &filter.out_vlan_info, 0);

    return IFX_SUCCESS;
}

int32_t ifx_ppa_vlan_filter_get_all(int32_t *num_filters, PPA_VLAN_FILTER_CONFIG *vlan_filters, uint32_t flags)
{
//#warning ifx_ppa_vlan_filter_get_all is not implemented, too many memory allocation problem
    return IFX_ENOTIMPL;
}

int32_t ifx_ppa_vlan_filter_del_all(uint32_t flags)
{
    int32_t i, j;
    uint32_t vlan_filter_type[]={PPA_F_VLAN_FILTER_IFNAME, PPA_F_VLAN_FILTER_IP_SRC, PPA_F_VLAN_FILTER_ETH_PROTO, PPA_F_VLAN_FILTER_VLAN_TAG};
    PPE_BRDG_VLAN_FILTER_MAP filter={0};
    PPE_VFILTER_COUNT_CFG vfilter_count={0};

    for ( i = 0; i < NUM_ENTITY(vlan_filter_type); i++ )
    {
        vfilter_count.vfitler_type = vlan_filter_type[i];
        ifx_ppa_drv_get_max_vfilter_entries( &vfilter_count, 0);

        for ( j = 0; j < vfilter_count.num; j++ )
        {
            filter.ig_criteria_type = vlan_filter_type[i];
            filter.entry = j;
            if( ifx_ppa_drv_get_vlan_map( &filter, 0)  == 1 )
            {
                ifx_ppa_drv_del_outer_vlan_entry( &filter.out_vlan_info, 0);
            }
        }
    }

    ifx_ppa_drv_del_all_vlan_map( 0);
    return IFX_SUCCESS;
}

/*
 *  PPA MIB Counters Operation Functions
 */

int32_t ifx_ppa_get_if_stats(PPA_IFNAME *ifname, PPA_IF_STATS *p_stats, uint32_t flags)
{
    struct netif_info *p_info;
    uint32_t port_flags;
    PPE_ITF_MIB_INFO itf_mib={0};

    if ( !ifname || !p_stats )
        return IFX_EINVAL;

    if ( ppa_netif_lookup(ifname, &p_info) != IFX_SUCCESS )
        return IFX_EIO;
    itf_mib.itf= p_info->phys_port;
    port_flags = p_info->flags;
    ppa_netif_put(p_info);

    if ( !(port_flags & NETIF_PHYS_PORT_GOT) )
        return IFX_EIO;

    ifx_ppa_drv_get_itf_mib(&itf_mib, 0);

    p_stats->rx_pkts            = itf_mib.mib.ig_cpu_pkts;
    p_stats->tx_discard_pkts    = itf_mib.mib.ig_drop_pkts;
    p_stats->rx_bytes           = itf_mib.mib.ig_cpu_bytes;

    return IFX_SUCCESS;
}

int32_t ifx_ppa_get_accel_stats(PPA_IFNAME *ifname, PPA_ACCEL_STATS *p_stats, uint32_t flags)
{
    struct netif_info *p_info;
    uint32_t port;
    uint32_t port_flags;
    PPE_ITF_MIB_INFO mib = {0};

    if ( !ifname || !p_stats )
        return IFX_EINVAL;

    if ( ppa_netif_lookup(ifname, &p_info) != IFX_SUCCESS )
        return IFX_EIO;
    port = p_info->phys_port;
    port_flags = p_info->flags;
    ppa_netif_put(p_info);

    if ( !(port_flags & NETIF_PHYS_PORT_GOT) )
        return IFX_EIO;

    mib.itf = p_info->phys_port;
    mib.flag = flags;
    ifx_ppa_drv_get_itf_mib(&mib, 0);

    p_stats->fast_routed_tcp_pkts       = mib.mib.ig_fast_rt_ipv4_tcp_pkts + mib.mib.ig_fast_rt_ipv6_tcp_pkts;
    p_stats->fast_routed_udp_pkts       = mib.mib.ig_fast_rt_ipv4_udp_pkts + mib.mib.ig_fast_rt_ipv6_udp_pkts;
    p_stats->fast_routed_udp_mcast_pkts = mib.mib.ig_fast_rt_ipv4_mc_pkts;
    p_stats->fast_drop_pkts             = mib.mib.ig_drop_pkts;
    p_stats->fast_drop_bytes            = mib.mib.ig_drop_bytes;
    p_stats->fast_ingress_cpu_pkts      = mib.mib.ig_cpu_pkts;
    p_stats->fast_ingress_cpu_bytes     = mib.mib.ig_cpu_bytes;
    p_stats->fast_bridged_ucast_pkts    = mib.mib.ig_fast_brg_pkts;
    p_stats->fast_bridged_bytes         = mib.mib.ig_fast_brg_bytes;

    return IFX_SUCCESS;
}

/*
 *  PPA Network Interface Operation Functions
 */

int32_t ifx_ppa_set_if_mac_address(PPA_IFNAME *ifname, uint8_t *mac_addr, uint32_t flags)
{
    struct netif_info *ifinfo;
    PPE_ROUTE_MAC_INFO mac_info={0};

    if ( !ifname || !mac_addr )
        return IFX_EINVAL;

    if ( ppa_netif_lookup(ifname, &ifinfo) != IFX_SUCCESS )
        return IFX_FAILURE;

    if ( (ifinfo->flags & NETIF_MAC_ENTRY_CREATED) )
    {
        mac_info.mac_ix= ifinfo->mac_entry;
        ifx_ppa_drv_del_mac_entry(&mac_info, 0);
        ifinfo->mac_entry = ~0;
        ifinfo->flags &= ~NETIF_MAC_ENTRY_CREATED;
    }

    ppa_memcpy(ifinfo->mac, mac_addr, PPA_ETH_ALEN);
    ifinfo->flags |= NETIF_MAC_AVAILABLE;

    ppa_memcpy(mac_info.mac, mac_addr, sizeof(mac_info.mac));
    if ( ifx_ppa_drv_add_mac_entry( &mac_info, 0) == IFX_SUCCESS )
    {
        ifinfo->mac_entry = mac_info.mac_ix;
        ifinfo->flags |= NETIF_MAC_ENTRY_CREATED;
    }

    ppa_netif_put(ifinfo);

    return IFX_SUCCESS;
}

int32_t ifx_ppa_get_if_mac_address(PPA_IFNAME *ifname, uint8_t *mac_addr, uint32_t flags)
{
    int32_t ret;
    struct netif_info *ifinfo;

    if ( !ifname || !mac_addr )
        return IFX_EINVAL;

    if ( ppa_netif_lookup(ifname, &ifinfo) != IFX_SUCCESS )
        return IFX_FAILURE;

    if ( (ifinfo->flags & NETIF_MAC_AVAILABLE) )
    {
        ppa_memcpy(mac_addr,ifinfo->mac, PPA_ETH_ALEN);
        ret = IFX_SUCCESS;
    }
    else
        ret = IFX_FAILURE;

    ppa_netif_put(ifinfo);

    return ret;
}

int32_t ifx_ppa_add_if(PPA_IFINFO *ifinfo, uint32_t flags)
{
    if ( !ifinfo )
        return IFX_EINVAL;

    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_add_if with force_wanitf_flag=%d in ifx_ppa_add_if\n", ifinfo->force_wanitf_flag );
    return ppa_netif_add(ifinfo->ifname, ifinfo->if_flags & PPA_F_LAN_IF, NULL, ifinfo->ifname_lower, ifinfo->force_wanitf_flag);
}

int32_t ifx_ppa_del_if(PPA_IFINFO *ifinfo, uint32_t flags)
{
    ppa_netif_remove(ifinfo->ifname, ifinfo->if_flags & PPA_F_LAN_IF);

    ppa_remove_sessions_on_netif(ifinfo->ifname);

    return IFX_SUCCESS;
}

int32_t ifx_ppa_get_if(int32_t *num_ifs, PPA_IFINFO **ifinfo, uint32_t flags)
{
    uint32_t pos = 0;
    struct netif_info *info;
    int32_t num = 0;
    PPA_IFINFO *p_ifinfo;

    if ( !num_ifs || !ifinfo )
        return IFX_EINVAL;

    p_ifinfo = (PPA_IFINFO *)ppa_malloc(100 * sizeof(PPA_IFINFO));  //  assume max 100 netif
    if(!p_ifinfo){
        return IFX_ENOMEM;
    }

    if ( ppa_netif_start_iteration(&pos, &info) != IFX_SUCCESS ){
        ppa_free(p_ifinfo);
        return IFX_FAILURE;
    }

    do
    {
        if ( (info->flags & NETIF_LAN_IF) )
        {
            p_ifinfo[num].ifname = info->name;
            p_ifinfo[num].if_flags = PPA_F_LAN_IF;
            num++;
        }
        if ( (info->flags & NETIF_WAN_IF) )
        {
            p_ifinfo[num].ifname = info->name;
            p_ifinfo[num].if_flags = 0;
            num++;
        }
    } while ( ppa_netif_iterate_next(&pos, &info) == IFX_SUCCESS );

    ppa_netif_stop_iteration();

    *num_ifs = num;
    *ifinfo = p_ifinfo;

    return IFX_SUCCESS;
}

int32_t ifx_ppa_multicast_pkt_srcif_add(PPA_BUF *pkt_buf, PPA_NETIF *rx_if)
{
    IP_ADDR_C ip={0};
    IP_ADDR_C src_ip={0};
    struct mc_group_list_item *p_item;
    struct netif_info *p_netif_info;
    int32_t res = IFX_PPA_SESSION_NOT_ADDED;
    int32_t ret;

    if( !rx_if )
    {
        rx_if = ppa_get_pkt_src_if( pkt_buf);
    }

    if(ppa_get_multicast_pkt_ip( pkt_buf, &ip, &src_ip) != IFX_SUCCESS){
        return res;
    }

    if(is_ip_zero(&ip))
    {
        return res;
    }

    ppa_mc_get_htable_lock();
    ret = __ppa_lookup_mc_group(&ip, &src_ip, &p_item);

    if (ret == IFX_PPA_MC_SESSION_VIOLATION ){//if violation, there is a item with src ip all zero, so search again with src ip zero
        ppa_memset(&src_ip, 0, sizeof(src_ip));
        ret = __ppa_lookup_mc_group(&ip, &src_ip, &p_item);
    }

    if ( ret == IFX_PPA_SESSION_EXISTS )
    {
        if( p_item->src_netif &&  p_item->src_netif != rx_if )
        { /*at present, we don't allowed to change multicast src_if */
            ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "Not matched src if: original srcif is %s, but new srcif is %s: %d.%d.%d.%d\n", ppa_get_netif_name(p_item->src_netif), ppa_get_netif_name(rx_if), ip.ip.ip >> 24, (ip.ip.ip >> 16) & 0xFF, (ip.ip.ip >> 8) & 0xFF, ip.ip.ip & 0xFF);
            goto ENTRY_ADD_EXIT;
        }
        if( p_item->flags & SESSION_ADDED_IN_HW )
        { //already added into HW. no change here
            res = IFX_PPA_SESSION_ADDED;
            goto ENTRY_ADD_EXIT;
        }

        if( ppa_is_netif_bridged(NULL, rx_if) )
            p_item->bridging_flag =1; //If the receive interface is in bridge, then regard it as bridge mode
        else
            p_item->bridging_flag =0;

        //  add to HW if possible
        if ( ppa_netif_update(rx_if, NULL) != IFX_SUCCESS ||  ppa_netif_lookup( ppa_get_netif_name( rx_if), &p_netif_info) != IFX_SUCCESS )
        {
            ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "ifx_ppa_multicast_pkt_srcif_add cannot get interface %s for  multicast session info: %d.%d.%d.%d\n", ppa_get_netif_name(rx_if), ip.ip.ip >> 24, (ip.ip.ip >> 16) & 0xFF, (ip.ip.ip >> 8) & 0xFF, ip.ip.ip & 0xFF) ;
            res = IFX_PPA_SESSION_NOT_ADDED;
            goto ENTRY_ADD_EXIT;
        }

        ppa_netif_put(p_netif_info);

        if ( p_netif_info->flags & NETIF_PHYS_PORT_GOT )
        {
            //  PPPoE and source mac
            if ( !p_item->bridging_flag )
            {
                if( p_netif_info->flags & NETIF_PPPOE )
                    p_item->flags |= SESSION_VALID_PPPOE;
            }

            //  VLAN
            if(p_netif_info->flags & NETIF_VLAN_CANT_SUPPORT ) 
                ppa_debug(DBG_ENABLE_MASK_ASSERT,"MC processing can support two layers of VLAN only\n");

            if ( (p_netif_info->flags & NETIF_VLAN_OUTER) )
                p_item->flags |= SESSION_VALID_OUT_VLAN_RM;
            if ( (p_netif_info->flags & NETIF_VLAN_INNER) )
                p_item->flags |= SESSION_VALID_VLAN_RM;

            p_item->src_netif = p_netif_info->netif;

            if( ppa_hw_add_mc_group(p_item) != IFX_SUCCESS )
            {
                ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "ppa_hw_add_mc_group(%d.%d.%d.%d): fail ???\n", ip.ip.ip >> 24, (ip.ip.ip >> 16) & 0xFF, (ip.ip.ip >> 8) & 0xFF, ip.ip.ip& 0xFF);
            }
            else
            {
                res = IFX_PPA_SESSION_ADDED;
                ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "ppa_hw_add_mc_group(%d.%d.%d.%d): sucessfully\n", ip.ip.ip >> 24, (ip.ip.ip >> 16) & 0xFF, (ip.ip.ip >> 8) & 0xFF, ip.ip.ip & 0xFF);
                ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "%s: update src interface:(%s)\n", __FUNCTION__, p_netif_info->netif->name );
            }
        }
    }
    else
    {
        ppa_debug(DBG_ENABLE_MASK_DUMP_MC_GROUP, "Not found the multicast group in existing list:%u.%u.%u.%u\n", NIPQUAD((ip.ip.ip)) );
    }

ENTRY_ADD_EXIT:
    ppa_mc_release_htable_lock();

    return res;
}

int32_t ifx_ppa_hook_wan_mii0_vlan_range_add(PPA_VLAN_RANGE *vlan_range, uint32_t flags)
{
    if ( vlan_range )
    {
        PPE_WAN_VID_RANGE vid={0};
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"vlanrange: from %x to %x\n", vlan_range->start_vlan_range, vlan_range->end_vlan_range);
        vid.vid = (vlan_range->start_vlan_range & 0xFFF ) | ( (vlan_range->end_vlan_range & 0xFFF ) << 16) ;
        ifx_ppa_drv_set_mixed_wan_vlan_id( &vid, 0 );

        return IFX_SUCCESS;
    }
    return IFX_FAILURE;
}

int32_t ifx_ppa_hook_wan_mii0_vlan_range_del(PPA_VLAN_RANGE *vlan_range, int32_t flags)
{
    if ( vlan_range )
    {
        PPE_WAN_VID_RANGE vid = {0};
        vid.vid = (vlan_range->start_vlan_range & 0xFFF ) | ( (vlan_range->end_vlan_range & 0xFFF ) << 16) ;
        ifx_ppa_drv_set_mixed_wan_vlan_id(&vid, 0);
        return IFX_SUCCESS;
    }

    return IFX_FAILURE;
}

int32_t ifx_ppa_hook_wan_mii0_vlan_ranges_get(int32_t *num_ranges, PPA_VLAN_RANGE *vlan_ranges, uint32_t flags)
{
    if ( vlan_ranges && num_ranges )
    {
        PPE_WAN_VID_RANGE vlanid = {0};

        ifx_ppa_drv_get_mixed_wan_vlan_id(&vlanid, 0);

        vlan_ranges->start_vlan_range = vlanid.vid& 0xFFF;
        vlan_ranges->end_vlan_range = ( vlanid.vid>> 12 ) & 0xFFF;

        *num_ranges = 1;
        return IFX_SUCCESS;
    }

    return IFX_FAILURE;
}

int32_t ifx_ppa_get_max_entries(PPA_MAX_ENTRY_INFO *max_entry, uint32_t flags)
{
    if( !max_entry ) return IFX_FAILURE;

    ifx_ppa_drv_get_max_entries(max_entry, 0);

    return IFX_SUCCESS;
}

int32_t ppa_ip_sprintf( char *buf, PPA_IPADDR ip, uint32_t flag)
{
    int32_t len=0;
    if( buf)  {
#ifdef CONFIG_IFX_PPA_IPv6_ENABLE
        if( flag & SESSION_IS_IPV6 ) {
            len = ppa_sprintf(buf, "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x", NIP6(ip.ip6) );
        }
        else
#endif
            len = ppa_sprintf(buf, "%u.%u.%u.%u", NIPQUAD(ip.ip) );

    }

    return len;
}

int32_t ppa_ip_comare( PPA_IPADDR ip1, PPA_IPADDR ip2, uint32_t flag )
{
#ifdef CONFIG_IFX_PPA_IPv6_ENABLE
    if( flag & SESSION_IS_IPV6 )
    {
        return ppa_memcmp(ip1.ip6, ip2.ip6, sizeof(ip1.ip6) );
    }
    else
#endif
    {
         return ppa_memcmp(&ip1.ip, &ip2.ip, sizeof(ip1.ip) );
    }
}


int32_t ppa_zero_ip( PPA_IPADDR ip)
{
    PPA_IPADDR zero_ip={0};

    return ( ppa_ip_comare(ip, zero_ip, 0 )==0 ) ? 1:0;
}

EXPORT_SYMBOL(ifx_ppa_init);
EXPORT_SYMBOL(ifx_ppa_exit);
EXPORT_SYMBOL(ifx_ppa_enable);
EXPORT_SYMBOL(ifx_ppa_add_if);
EXPORT_SYMBOL(ifx_ppa_del_if);
EXPORT_SYMBOL(ifx_ppa_get_max_entries);
EXPORT_SYMBOL( ppa_ip_sprintf);
EXPORT_SYMBOL(ppa_ip_comare);
EXPORT_SYMBOL(g_ppa_ppa_mtu);
EXPORT_SYMBOL(g_ppa_min_hits);
EXPORT_SYMBOL(ppa_zero_ip);
EXPORT_SYMBOL(g_ppe_fastpath_enabled);
