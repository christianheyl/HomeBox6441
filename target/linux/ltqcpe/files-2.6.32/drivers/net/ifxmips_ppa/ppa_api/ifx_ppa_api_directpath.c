/*******************************************************************************
**
** FILE NAME    : ifx_ppa_api_directpath.c
** PROJECT      : PPA
** MODULES      : PPA API (Routing/Bridging Acceleration APIs)
**
** DATE         : 19 NOV 2008
** AUTHOR       : Xu Liang
** DESCRIPTION  : PPA Protocol Stack Hook API Directpath Functions
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
** 19 NOV 2008  Xu Liang        Initiate Version
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
#include <net/ifx_ppa_api_directpath.h>
#include <net/ifx_ppa_ppe_hal.h>
#include "ifx_ppa_api_misc.h"
#include "ifx_ppa_api_netif.h"
#include "ifx_ppe_drv_wrapper.h"
#include "ifx_ppa_api_misc.h"
#include "ifx_ppa_api_tools.h"


/*
 * ####################################
 *              Definition
 * ####################################
 */



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

static void update_itf_info(void);



/*
 * ####################################
 *           Global Variable
 * ####################################
 */

static PPA_LOCK g_directpath_port_lock;
static uint32_t g_start_ifid = ~0, g_end_ifid = ~0;



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

static void update_itf_info(void)
{
    uint32_t i;
    PPE_IFINFO if_info;
    uint32_t tmp_flags;

    ppa_memset(&if_info, 0, sizeof(if_info));

    for ( i = g_start_ifid; i < g_end_ifid; i++ )
    {
        if_info.port = i;
        if ( ifx_ppa_drv_get_phys_port_info(&if_info, 0) == IFX_SUCCESS
            && (if_info.if_flags & (IFX_PPA_PHYS_PORT_FLAGS_VALID | IFX_PPA_PHYS_PORT_FLAGS_TYPE_MASK)) == (IFX_PPA_PHYS_PORT_FLAGS_VALID | IFX_PPA_PHYS_PORT_FLAGS_TYPE_EXT)
            && (if_info.if_flags & (IFX_PPA_PHYS_PORT_FLAGS_EXT_CPU0 | IFX_PPA_PHYS_PORT_FLAGS_EXT_CPU1)) != 0 )
        {
            tmp_flags = ifx_ppa_drv_g_ppe_directpath_data[i - g_start_ifid].flags & ~PPE_DIRECTPATH_LANWAN_MASK;
            if ( (if_info.if_flags & IFX_PPA_PHYS_PORT_FLAGS_MODE_LAN) )
                tmp_flags |= PPE_DIRECTPATH_LAN;
            if ( (if_info.if_flags & IFX_PPA_PHYS_PORT_FLAGS_MODE_WAN) )
                tmp_flags |= PPE_DIRECTPATH_WAN;
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "%s: if_info.if_flags = 0x%08x, direct_flag[%d]=0x%08x, tmp_flags=0x%08x\n", __FUNCTION__, if_info.if_flags, i - g_start_ifid, ifx_ppa_drv_g_ppe_directpath_data[i - g_start_ifid].flags, tmp_flags);
            ifx_ppa_drv_g_ppe_directpath_data[i - g_start_ifid].flags = tmp_flags;
        }
    }
}

static PPA_BUF *__remove_directpath_dev_from_datapath(int if_id)
{
    //unsigned long sys_flag;
    uint32_t tmp_flags;
    
#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_SIZE) || defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_PKTS)
    PPA_BUF *skb_list = NULL;
#endif
    
    if ( (ifx_ppa_drv_g_ppe_directpath_data[if_id].flags & PPE_DIRECTPATH_DATA_ENTRY_VALID) )
    {
        ppa_phys_port_remove(if_id + g_start_ifid);
        //sys_flag = ppa_disable_int();
        tmp_flags = ifx_ppa_drv_g_ppe_directpath_data[if_id].flags & PPE_DIRECTPATH_MASK;
#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_SIZE) || defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_PKTS)
        ppe_lock_get(&ifx_ppa_drv_g_ppe_directpath_data[if_id].txq_lock);
        skb_list = ifx_ppa_drv_g_ppe_directpath_data[if_id].skb_list;
        ifx_ppa_drv_g_ppe_directpath_data[if_id].skb_list = NULL;
        ppe_lock_release(&ifx_ppa_drv_g_ppe_directpath_data[if_id].txq_lock);
#endif  
        ppa_memset(&ifx_ppa_drv_g_ppe_directpath_data[if_id], 0, sizeof(ifx_ppa_drv_g_ppe_directpath_data[if_id]));
        ifx_ppa_drv_g_ppe_directpath_data[if_id].flags = tmp_flags;
        //ppa_enable_int(sys_flag);
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"directp unregiter ok and restore direct_flag[%d]=%x\n", if_id, ifx_ppa_drv_g_ppe_directpath_data[if_id].flags );
    }

#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_SIZE) || defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_PKTS)
    return skb_list;
#endif
    return NULL;
}

#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_SIZE) || defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_PKTS)
static int remove_directpath_queue(PPA_BUF *skb_list)
{
    PPA_BUF *skb, *skb_list_bak;

    if ( skb_list )
    {
      skb_list_bak = skb_list;
      do
      {
          skb = skb_list;
          skb_list = ppa_buf_get_next(skb_list);
          ppa_buf_free(skb);
      } while ( skb_list != NULL && skb_list != skb_list_bak );
    }
    return 0;
}
#endif

/*
 * ####################################
 *           Global Function
 * ####################################
 */

/*
 *  PPA DirectPath Functions
 */

#if defined(CONFIG_ACCL_11AC) || defined(CONFIG_ACCL_11AC_MODULE)
int32_t ifx_ppa_directlink_register_dev(int32_t *p_if_id, PPA_DTLK_T *dtlk, PPA_DIRECTPATH_CB *pDirectpathCb,uint32_t flags)
{
   int32_t ret = IFX_SUCCESS;
   uint32_t if_id = ~0;

   if(!ifx_ppa_is_init()) 
      return IFX_EINVAL;
   
   if(!p_if_id || !dtlk || !dtlk->dev)
      return IFX_EINVAL;

   if(dtlk->flags & PPE_F_DTLK_REGISTER){//register directlink
       printk("%s: register directlink\n", __func__); 
       if((ret = datapath_dtlk_register(p_if_id, dtlk)) != IFX_SUCCESS){
           return ret;
       }
   }
   
   if(dtlk->flags & PPE_F_DTLK_DP_REGISTER){//register to directpath
       if((ret=ifx_ppa_directpath_register_dev(&if_id,dtlk->dev,pDirectpathCb,flags)) == IFX_SUCCESS){
            printk("%s: register directpath\n", __func__); 
            if((ret = datapath_dtlk_update(*p_if_id,dtlk, &if_id)) != IFX_SUCCESS)
                return ret;
       }else{
            return ret;
       }
   }else if(dtlk->flags & PPE_F_DTLK_DP_DEREGISTER){//deregister from directpath
        printk("%s: de-register directpath\n", __func__); 
        datapath_dtlk_update(*p_if_id,dtlk, &if_id);
        if((ret = ifx_ppa_directpath_register_dev(&if_id,dtlk->dev,pDirectpathCb,flags))!= IFX_SUCCESS)
            return ret;
   }

   if(dtlk->flags & PPE_F_DTLK_DEREGISTER){//deregister from directlink
        printk("%s: de-register directlink\n", __func__); 
        ret = datapath_dtlk_register(p_if_id, dtlk);
   }
   
   return ret;      
}
#endif

int32_t ifx_ppa_directpath_register_dev(uint32_t *p_if_id, PPA_NETIF *netif, PPA_DIRECTPATH_CB *pDirectpathCb, uint32_t flags)
{
    int32_t ret;
    uint32_t if_id;
    uint32_t tmp_flags;
    PPA_BUF *skb_list = NULL;

    if( !ifx_ppa_is_init() ) return IFX_EINVAL;

    if ( p_if_id == NULL || !ifx_ppa_drv_g_ppe_directpath_data  )
        return IFX_EINVAL;

    if ( (flags & PPA_F_DIRECTPATH_REGISTER) )
    {
        if ( !(flags & PPA_F_DIRECTPATH_ETH_IF) )
            //  currently, only ethernet interface is supported
            return IFX_EINVAL;

        if ( !netif || !pDirectpathCb
            || !pDirectpathCb->rx_fn )
            return IFX_EINVAL;

        if ( ppa_get_netif_name(netif) == NULL )
            return IFX_EINVAL;

        tmp_flags = (flags & PPA_F_DIRECTPATH_WAN) ? PPE_DIRECTPATH_WAN : PPE_DIRECTPATH_LAN;
        if ( (flags & PPA_F_DIRECTPATH_CORE1) )
            tmp_flags |= PPE_DIRECTPATH_CORE1;
        else
            tmp_flags |= PPE_DIRECTPATH_CORE0;
        if ( (flags & PPA_F_DIRECTPATH_ETH_IF) )
            tmp_flags |= PPE_DIRECTPATH_ETH;

        //  update flags which could be changed on the fly, e.g. AR9/VR9/AR10 - LAN/WAN could be changed
        update_itf_info();

        ppa_lock_get(&g_directpath_port_lock);

        //first check whether the interface already added into PPA directpath or not
        for ( if_id = 0; if_id < g_end_ifid; if_id ++ )
        {
            if( ifx_ppa_drv_g_ppe_directpath_data[if_id].netif && ( ifx_ppa_drv_g_ppe_directpath_data[if_id].netif == netif ) )
            {
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "The interface already in PPA directpath already. Its netif pointer is :%p\n", netif);
                if(p_if_id ) *p_if_id = ifx_ppa_drv_g_ppe_directpath_data[if_id].ifid ;
                ppa_lock_release(&g_directpath_port_lock);
                return IFX_SUCCESS;
            }
        }

        for ( if_id = 0;
              if_id < g_end_ifid - g_start_ifid && (ifx_ppa_drv_g_ppe_directpath_data[if_id].flags & (PPE_DIRECTPATH_DATA_ENTRY_VALID | PPE_DIRECTPATH_ITF_TYPE_MASK | tmp_flags)) != tmp_flags;
              if_id++ )
        {
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "%s: direct_flag[%d]=0x%08x, tmp_flags=0x%08x\n", __FUNCTION__, if_id, ifx_ppa_drv_g_ppe_directpath_data[if_id].flags, tmp_flags);
        }
        if ( if_id < g_end_ifid - g_start_ifid
            && ppa_phys_port_add(ppa_get_netif_name(netif), if_id + g_start_ifid) == IFX_SUCCESS )
        {
            tmp_flags = (ifx_ppa_drv_g_ppe_directpath_data[if_id].flags & PPE_DIRECTPATH_MASK) | PPE_DIRECTPATH_DATA_ENTRY_VALID | PPE_DIRECTPATH_DATA_RX_ENABLE;
            ppa_memset(&ifx_ppa_drv_g_ppe_directpath_data[if_id], 0, sizeof(ifx_ppa_drv_g_ppe_directpath_data[if_id]));
#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_SIZE) || defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_PKTS)
            ppe_lock_init(&ifx_ppa_drv_g_ppe_directpath_data[if_id].txq_lock);
#endif
            ifx_ppa_drv_g_ppe_directpath_data[if_id].callback   = *pDirectpathCb;
            ifx_ppa_drv_g_ppe_directpath_data[if_id].netif      = netif;
            ifx_ppa_drv_g_ppe_directpath_data[if_id].ifid       = if_id + g_start_ifid;
            ifx_ppa_drv_g_ppe_directpath_data[if_id].flags      = tmp_flags;
            *p_if_id = if_id + g_start_ifid;
            ret = IFX_SUCCESS;
        }
        else
        {
            ret = IFX_FAILURE;
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "direct register fail\n" );
        }
        ppa_lock_release(&g_directpath_port_lock);

        return ret;
    }
    else
    {
        if ( *p_if_id < g_start_ifid || *p_if_id >= g_end_ifid )
        {
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"directp unregister wrong id: %d\n", *p_if_id);
            return IFX_EINVAL;
        }

        if_id = *p_if_id - g_start_ifid;
        ppa_lock_get(&g_directpath_port_lock);
        skb_list=__remove_directpath_dev_from_datapath(if_id);       
        ppa_lock_release(&g_directpath_port_lock);

#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_SIZE) || defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_PKTS)
        remove_directpath_queue(skb_list);
#endif
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"unregiter directpath ok\n");
        return IFX_SUCCESS;
    }
}

int32_t ifx_ppa_directpath_send(uint32_t rx_if_id, PPA_BUF *skb, int32_t len, uint32_t flags)
{
#if !defined(CONFIG_IFX_PPA_API_DIRECTPATH_BRIDGING)
    uint8_t mac[PPA_ETH_ALEN] = {0};
    uint8_t netif_mac[PPA_ETH_ALEN] = {0};
#endif
    int32_t ret = IFX_SUCCESS;

#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_IMQ)
    int32_t (*tmp_imq_queue)(PPA_BUF *skb, uint32_t portID) = ppa_hook_directpath_enqueue_to_imq_fn;
#endif        
    if( skb == NULL )
    {
        return IFX_EINVAL;
    }
    
    if( !ifx_ppa_is_init() ) 
    {
        PPA_SKB_FREE(skb);
        return IFX_EINVAL;
    }

    if(flags & 3) { //directlink function call
        return ifx_ppa_drv_directpath_send(rx_if_id, skb, len, flags);        
    }
    
    if( !ifx_ppa_drv_g_ppe_directpath_data ) 
    {
        PPA_SKB_FREE(skb);
        return IFX_EINVAL;
    }

    ppa_lock_get(&g_directpath_port_lock);
    
    if ( rx_if_id < g_start_ifid || rx_if_id >= g_end_ifid )
    {
        ret = IFX_EINVAL;
        goto __DIRETPATH_TX_EXIT;
    }

    if ( !(ifx_ppa_drv_g_ppe_directpath_data[rx_if_id - g_start_ifid].flags & PPE_DIRECTPATH_DATA_ENTRY_VALID) )
    {
        ret = IFX_EPERM;
        goto __DIRETPATH_TX_EXIT;
    }

#if !defined(CONFIG_IFX_PPA_API_DIRECTPATH_BRIDGING)
    ppa_get_pkt_rx_dst_mac_addr(skb, mac);
    ppa_get_netif_hwaddr(ifx_ppa_drv_g_ppe_directpath_data[rx_if_id - g_start_ifid].netif, netif_mac, 0);
    if ( ppa_memcmp(mac, /* ifx_ppa_drv_g_ppe_directpath_data[rx_if_id - g_start_ifid].mac */ netif_mac, PPA_ETH_ALEN) != 0 )
    {
        //  bridge
        rx_if_id -= g_start_ifid;
        if ( (ifx_ppa_drv_g_ppe_directpath_data[rx_if_id].flags & PPE_DIRECTPATH_DATA_RX_ENABLE) )
        {
            ifx_ppa_drv_g_ppe_directpath_data[rx_if_id].callback.rx_fn(ifx_ppa_drv_g_ppe_directpath_data[rx_if_id].netif, NULL, skb, len);
            ret = IFX_SUCCESS;
            goto __DIRETPATH_TX_EXIT;
        }
        else
        {
            ret = IFX_EINVAL;
            goto __DIRETPATH_TX_EXIT;
        }
    }
#endif


#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_IMQ)
    if( ppa_directpath_imq_en_flag && tmp_imq_queue )
    {
         if( tmp_imq_queue(skb, rx_if_id) == 0 )
        { 
            ppa_lock_release(&g_directpath_port_lock);
            return 0;
        }
   }
#endif
   ret= ifx_ppa_drv_directpath_send(rx_if_id, skb, len, flags);
   ppa_lock_release(&g_directpath_port_lock);
   return ret;
   

__DIRETPATH_TX_EXIT:
   if(ret != IFX_SUCCESS && skb){
        PPA_SKB_FREE(skb);
   }

   ppa_lock_release(&g_directpath_port_lock);
   return ret;

}

#ifdef CONFIG_QUALCOMM_AP_PPA
//++Add by Maxx_chen
/* ARCADYAN #ARC_RC_SD2#, 2013/4/16, Dino, Make specific PPA path(rmv redundent) for wireless drv */
int32_t ifx_ppa_directpath_send_spec(uint32_t rx_if_id, PPA_BUF *skb, int32_t len, uint32_t flags)
{
#if !defined(CONFIG_IFX_PPA_API_DIRECTPATH_BRIDGING)
    uint8_t mac[PPA_ETH_ALEN] = {0};
#endif

    if( !ifx_ppa_drv_g_ppe_directpath_data ) return IFX_EINVAL;

    if ( rx_if_id < g_start_ifid || rx_if_id >= g_end_ifid || skb == NULL  )
        return IFX_EINVAL;

    if ( !(ifx_ppa_drv_g_ppe_directpath_data[rx_if_id - g_start_ifid].flags & PPE_DIRECTPATH_DATA_ENTRY_VALID) )
        return IFX_EPERM;

#if !defined(CONFIG_IFX_PPA_API_DIRECTPATH_BRIDGING)
    ppa_get_pkt_rx_dst_mac_addr(skb, mac);
    if ( ppa_memcmp(mac, ifx_ppa_drv_g_ppe_directpath_data[rx_if_id - g_start_ifid].mac, PPA_ETH_ALEN) != 0 )
    {
        //  bridge
        rx_if_id -= g_start_ifid;
        if ( (ifx_ppa_drv_g_ppe_directpath_data[rx_if_id].flags & PPE_DIRECTPATH_DATA_RX_ENABLE) )
        {
            ifx_ppa_drv_g_ppe_directpath_data[rx_if_id].callback.rx_fn(ifx_ppa_drv_g_ppe_directpath_data[rx_if_id].netif, NULL, skb, len);
            return IFX_SUCCESS;
        }
        else
            return IFX_EAGAIN;
    }
#endif

#if defined(CONFIG_IFX_PPA_API_DIRECTPATH_TASKLET)
  #warning CONFIG_IFX_PPA_API_DIRECTPATH_TASKLET not implemented yet
    return IFX_ENOTIMPL;
#else
    return ifx_ppa_drv_directpath_send_spec(rx_if_id, skb, len, flags);
#endif
}
//--Add by Maxx_chen
#endif

#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_IMQ)
int32_t ifx_ppa_directpath_reinject_from_imq(int32_t rx_if_id, PPA_BUF *skb, int32_t len, uint32_t flags)
{
#if !defined(CONFIG_IFX_PPA_API_DIRECTPATH_BRIDGING)
    uint8_t mac[PPA_ETH_ALEN] = {0};
    uint8_t netif_mac[PPA_ETH_ALEN] = {0};
#endif
    int32_t ret = IFX_SUCCESS;

    if( skb == NULL )
    {
        return IFX_EINVAL;
    }
    
    if( !ifx_ppa_is_init() ) 
    {
        PPA_SKB_FREE(skb);
        return IFX_EINVAL;
    }
    if( !ifx_ppa_drv_g_ppe_directpath_data ) 
    {
        PPA_SKB_FREE(skb);
        return IFX_EINVAL;
    }

    ppa_lock_get(&g_directpath_port_lock);
    
    if ( rx_if_id < g_start_ifid || rx_if_id >= g_end_ifid )
    {
        ret = IFX_EINVAL;
        goto __DIRETPATH_TX_EXIT;
    }

    if ( !(ifx_ppa_drv_g_ppe_directpath_data[rx_if_id - g_start_ifid].flags & PPE_DIRECTPATH_DATA_ENTRY_VALID) )
    {
        ret = IFX_EPERM;
        goto __DIRETPATH_TX_EXIT;
    }

    ret = ifx_ppa_drv_directpath_send(rx_if_id, skb, len, flags);
    ppa_lock_release(&g_directpath_port_lock);
    return ret;
    

__DIRETPATH_TX_EXIT:
   if(ret != IFX_SUCCESS && skb){
        PPA_SKB_FREE(skb);
   }

   ppa_lock_release(&g_directpath_port_lock);
   return ret;

}
#endif

int32_t ifx_ppa_directpath_rx_stop(uint32_t if_id, uint32_t flags)
{
    int32_t ret;

    if( !ifx_ppa_is_init() ) return IFX_EINVAL;
    if( !ifx_ppa_drv_g_ppe_directpath_data ) return IFX_EINVAL;

    ppa_lock_get(&g_directpath_port_lock);
    if ( if_id >= g_start_ifid && if_id < g_end_ifid
        && (ifx_ppa_drv_g_ppe_directpath_data[if_id - g_start_ifid].flags & PPE_DIRECTPATH_DATA_ENTRY_VALID) )
    {
        ifx_ppa_drv_g_ppe_directpath_data[if_id - g_start_ifid].flags &= ~PPE_DIRECTPATH_DATA_RX_ENABLE;
       ret = ifx_ppa_drv_directpath_rx_stop(if_id, flags) == 0 ? IFX_SUCCESS : IFX_FAILURE;
    }
    else
        ret = IFX_EINVAL;
    ppa_lock_release(&g_directpath_port_lock);

    return ret;
}

int32_t ifx_ppa_directpath_rx_restart(uint32_t if_id, uint32_t flags)
{
    int32_t ret;

    if( !ifx_ppa_is_init() ) return IFX_EINVAL;
    if( !ifx_ppa_drv_g_ppe_directpath_data ) return IFX_EINVAL;

    ppa_lock_get(&g_directpath_port_lock);
    if ( if_id >= g_start_ifid && if_id < g_end_ifid
        && (ifx_ppa_drv_g_ppe_directpath_data[if_id - g_start_ifid].flags & PPE_DIRECTPATH_DATA_ENTRY_VALID) )
    {
        ifx_ppa_drv_g_ppe_directpath_data[if_id - g_start_ifid].flags |= PPE_DIRECTPATH_DATA_RX_ENABLE;
        ret = ifx_ppa_drv_directpath_rx_start(if_id, flags) == 0 ? IFX_SUCCESS : IFX_FAILURE;

    }
    else
        ret = IFX_EINVAL;
    ppa_lock_release(&g_directpath_port_lock);

    return ret;
}

PPA_NETIF *ifx_ppa_get_netif_for_ppa_ifid(uint32_t if_id)
{
    PPA_NETIF *ret = NULL;

    if( !ifx_ppa_is_init() ) return NULL;
    if( !ifx_ppa_drv_g_ppe_directpath_data ) return NULL;

    ppa_lock_get(&g_directpath_port_lock);
    if ( if_id >= g_start_ifid && if_id < g_end_ifid )
    {
        if_id -= g_start_ifid;
        if ( (ifx_ppa_drv_g_ppe_directpath_data[if_id].flags & PPE_DIRECTPATH_DATA_ENTRY_VALID) )
            ret = ifx_ppa_drv_g_ppe_directpath_data[if_id].netif;
    }
    ppa_lock_release(&g_directpath_port_lock);

    return ret;
}

int32_t ifx_ppa_get_ifid_for_netif(PPA_NETIF *netif)
{
    uint32_t if_id;

    if( !ifx_ppa_is_init() ) return IFX_EINVAL;
    if( !ifx_ppa_drv_g_ppe_directpath_data ) return IFX_EINVAL;

    if ( netif == NULL )
        return IFX_EINVAL;

    ppa_lock_get(&g_directpath_port_lock);
    for ( if_id = 0; if_id < g_end_ifid - g_start_ifid; if_id++ )
        if ( (ifx_ppa_drv_g_ppe_directpath_data[if_id].flags & PPE_DIRECTPATH_DATA_ENTRY_VALID)
            && ppa_is_netif_equal(netif, ifx_ppa_drv_g_ppe_directpath_data[if_id].netif) )
        {
            ppa_lock_release(&g_directpath_port_lock);
            return if_id + g_start_ifid;
        }
    ppa_lock_release(&g_directpath_port_lock);

    return IFX_FAILURE;
}

int32_t ppa_directpath_data_start_iteration(uint32_t *ppos, struct ppe_directpath_data **info)
{
    uint32_t l;

    ppa_lock_get(&g_directpath_port_lock);
	
    if( !ifx_ppa_is_init() ) return IFX_EINVAL;
    
    if ( g_start_ifid == ~0 )
    {
        *info = NULL;
        return IFX_FAILURE;
    }

    if( !ifx_ppa_drv_g_ppe_directpath_data ) return IFX_EINVAL;

    l = *ppos;
    if ( l + g_start_ifid < g_end_ifid )
    {
        ++*ppos;
        *info = ifx_ppa_drv_g_ppe_directpath_data + l;
        return IFX_SUCCESS;
    }
    else
    {
        *info = NULL;
        return IFX_FAILURE;
    }
}

int32_t ppa_directpath_data_iterate_next(uint32_t *ppos, struct ppe_directpath_data **info)
{
    if( !ifx_ppa_is_init() ) return IFX_EINVAL;
    if( !ifx_ppa_drv_g_ppe_directpath_data ) return IFX_EINVAL;

    if ( *ppos + g_start_ifid < g_end_ifid )
    {
        *info = ifx_ppa_drv_g_ppe_directpath_data + *ppos;
        ++*ppos;
        return IFX_SUCCESS;
    }
    else
    {
        *info = NULL;
        return IFX_FAILURE;
    }
}

void ppa_directpath_data_stop_iteration(void)
{
    ppa_lock_release(&g_directpath_port_lock);
}

void ppa_directpath_get_ifid_range(uint32_t *p_start_ifid, uint32_t *p_end_ifid)
{
    if ( p_start_ifid )
        *p_start_ifid = g_start_ifid;
    if ( p_end_ifid )
        *p_end_ifid = g_end_ifid;
}



/*
 * ####################################
 *           Init/Cleanup API
 * ####################################
 */

int32_t ppa_api_directpath_init(void)
{
    uint32_t i;
    uint32_t last_valid_ifid;
    uint32_t tmp_flags;
    PPE_IFINFO if_info;
    PPE_COUNT_CFG count={0};

    if ( !ifx_ppa_drv_g_ppe_directpath_data )
        return IFX_EINVAL;

    ifx_ppa_drv_get_number_of_phys_port(&count, 0);
    last_valid_ifid = count.num - 1;
    ppa_memset( &if_info, 0, sizeof(if_info) );
    for ( i = 0; i < count.num; i++ )
    {
        if_info.port = i;
        ifx_ppa_drv_get_phys_port_info(&if_info, 0);
        if ( (if_info.if_flags & (IFX_PPA_PHYS_PORT_FLAGS_VALID | IFX_PPA_PHYS_PORT_FLAGS_TYPE_MASK)) == (IFX_PPA_PHYS_PORT_FLAGS_VALID | IFX_PPA_PHYS_PORT_FLAGS_TYPE_EXT)
            && (if_info.if_flags  & (IFX_PPA_PHYS_PORT_FLAGS_EXT_CPU0 | IFX_PPA_PHYS_PORT_FLAGS_EXT_CPU1)) != 0 )
        {
            if ( g_start_ifid == ~0 )
                g_start_ifid = i;
            last_valid_ifid = i;
            tmp_flags = PPE_DIRECTPATH_ETH;
            if ( (if_info.if_flags  & IFX_PPA_PHYS_PORT_FLAGS_EXT_CPU0) )
                tmp_flags |= PPE_DIRECTPATH_CORE0;
            if ( (if_info.if_flags  & IFX_PPA_PHYS_PORT_FLAGS_EXT_CPU1) )
                tmp_flags |= PPE_DIRECTPATH_CORE1;
            if ( (if_info.if_flags & IFX_PPA_PHYS_PORT_FLAGS_MODE_LAN) )
                tmp_flags |= PPE_DIRECTPATH_LAN;
            if ( (if_info.if_flags & IFX_PPA_PHYS_PORT_FLAGS_MODE_WAN) )
                tmp_flags |= PPE_DIRECTPATH_WAN;
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "%s: if_info.if_flags = 0x%08x, direct_flag[%d]=0x%08x, tmp_flags=0x%08x\n", __FUNCTION__, if_info.if_flags, i - g_start_ifid, ifx_ppa_drv_g_ppe_directpath_data[i - g_start_ifid].flags, tmp_flags);
            ifx_ppa_drv_g_ppe_directpath_data[i - g_start_ifid].flags = tmp_flags;
        }
    }
    g_end_ifid = last_valid_ifid + 1;

    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "directpath start id: %d, end id: %d\n", g_start_ifid, g_end_ifid);

    return IFX_SUCCESS;
}

void ppa_api_directpath_exit(void)
{
    uint32_t i;  
    PPA_BUF *skb_list = NULL;



    if ( g_end_ifid != ~0 )
    {
        if( ifx_ppa_drv_g_ppe_directpath_data )
        {
            ppa_lock_get(&g_directpath_port_lock);
            for ( i = 0; i < g_end_ifid - g_start_ifid; i++ )
            {
                if( ifx_ppa_drv_g_ppe_directpath_data[i].flags & PPE_DIRECTPATH_DATA_ENTRY_VALID) 
                {
                    skb_list=__remove_directpath_dev_from_datapath(i);      
#if defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_SIZE) || defined(CONFIG_IFX_PPA_DIRECTPATH_TX_QUEUE_PKTS)
                    remove_directpath_queue(skb_list);                    
#endif                    
                }
            }
            g_start_ifid = g_end_ifid = ~0;
            ppa_lock_release(&g_directpath_port_lock);
        }
    }

}

int32_t ppa_api_directpath_create(void)
{
    if ( ppa_lock_init(&g_directpath_port_lock) )
    {
        err("Failed in creating lock for directpath port.");
        return IFX_EIO;
    }

    return IFX_SUCCESS;
}

void ppa_api_directpath_destroy(void)
{
    ppa_lock_destroy(&g_directpath_port_lock);
}

EXPORT_SYMBOL(ppa_directpath_data_start_iteration);
EXPORT_SYMBOL(ppa_directpath_data_iterate_next);
EXPORT_SYMBOL(ppa_directpath_data_stop_iteration);
EXPORT_SYMBOL(ppa_directpath_get_ifid_range);
#if defined(IFX_PPA_DIRECTPATH_TX_IMQ)
    EXPORT_SYMBOL(ifx_ppa_directpath_reinject_from_imq);
#endif
