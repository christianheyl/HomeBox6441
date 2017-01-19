/*******************************************************************************
**
** FILE NAME    : ifx_ppa_api_mib.c
** PROJECT      : PPA
** MODULES      : PPA API (Generic MIB APIs)
**
** DATE         : 3 NOV 2008
** AUTHOR       : Xu Liang
** DESCRIPTION  : PPA Protocol Stack Hook API Miscellaneous Functions
** COPYRIGHT    :              Copyright (c) 2009
**                          Lantiq Deutschland GmbH
**                   Am Campeon 3; 85579 Neubiberg, Germany
**
**   For licensing information, see the file 'LICENSE' in the root folder of
**   this software module.
**
** HISTORY
** $Date        $Author         $Comment
** 03 NOV 2008  Xu Liang        Initiate Version
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#if defined(CONFIG_IFX_PPA_API_PROC)
#include <linux/proc_fs.h>
#endif

#include <net/ifx_ppa_api.h>
#include <net/ifx_ppa_ppe_hal.h>

#include "ifx_ppa_api_misc.h"
#include "ifx_ppa_api_netif.h"
#include "ifx_ppa_api_session.h"
#include "ifx_ppe_drv_wrapper.h"

#include "ifx_ppa_api_mib.h"
#include "../platform/ifx_ppa_datapath.h"
#include "ifx_ppa_api_qos.h"


PPA_LOCK  g_general_lock;
//Port MIB varabile
static uint32_t last_jiffy_port_mib;
static PPA_PORT_MIB g_port_mib_accumulated;         /* accumulatd mib counter */
static PPA_PORT_MIB g_port_mib_last_instant_read;   /* last instant read counter */
static PPA_PORT_MIB g_port_mib_accumulated_last;    /* last accumulatd mib counter */


//QoS queue MIB varabiles
static PPA_QOS_STATUS g_qos_mib_accumulated[IFX_MAX_PORT_NUM];          /* accumulatd mib counter */
static PPA_QOS_STATUS g_qos_mib_last_instant_read[IFX_MAX_PORT_NUM];  /* last instant read counter */
static uint32_t last_jiffy_qos_mib[IFX_MAX_PORT_NUM];
static PPA_QOS_STATUS g_qos_mib_accumulated_last[IFX_MAX_PORT_NUM];   /* last accumulatd mib counter */

void reset_local_mib(void)
{
    uint32_t curr_jiffy=jiffies;
    int i;
    
    ppa_lock_get(&g_general_lock);
    last_jiffy_port_mib = curr_jiffy;
    ppa_memset( &g_port_mib_accumulated, 0, sizeof(g_port_mib_accumulated));
    ppa_memset( &g_port_mib_last_instant_read, 0, sizeof(g_port_mib_last_instant_read));
    ppa_memset( &g_port_mib_accumulated_last, 0, sizeof(g_port_mib_accumulated_last));

    
    for(i=0; i<IFX_MAX_PORT_NUM; i++ )
        last_jiffy_qos_mib[i] = curr_jiffy;
    ppa_memset( &g_qos_mib_accumulated, 0, sizeof(g_qos_mib_accumulated));
    ppa_memset( &g_qos_mib_last_instant_read, 0, sizeof(g_qos_mib_last_instant_read));
    ppa_memset( &g_qos_mib_accumulated_last, 0, sizeof(g_qos_mib_accumulated_last));
    ppa_lock_release(&g_general_lock);
    
}

static void update_port_mib64_item(uint64_t *curr, uint64_t *last, uint64_t *accumulated)
{
    if( *curr >= *last) 
        *accumulated += (*curr - *last);
    else
        *accumulated += ((uint64_t)*curr + (uint64_t)WRAPROUND_32BITS - *last);
    *last = *curr;

}
//note, so far only ioctl will set rate_flag to 1, otherwise it will be zero in ppa timer
int32_t ppa_update_qos_mib(PPA_QOS_STATUS *status, uint32_t rate_flag, uint32_t flag)
{
    uint32_t i, curr_jiffy, port_id;
    int32_t num;

    if( !status ) return IFX_FAILURE;
    if( status->qos_queue_portid >= IFX_MAX_PORT_NUM ) return IFX_FAILURE;
    
    num = ifx_ppa_get_qos_qnum(status->qos_queue_portid, 0 );
    if( num <= 0 ) 
    {
        ppa_debug(DBG_ENABLE_MASK_QOS,"ifx_ppa_get_qos_qnum failed for ifx_ppa_get_qos_qnum=%d\n",num);
        return IFX_FAILURE;
    }
    ppa_lock_get(&g_general_lock);
    if( num > IFX_MAX_QOS_QUEUE_NUM ) 
        num = IFX_MAX_QOS_QUEUE_NUM;
    status->max_buffer_size = num;
    port_id = status->qos_queue_portid;    
    
    if( ifx_ppa_drv_get_qos_status( status, flag) != IFX_SUCCESS) 
    {
        ppa_lock_release(&g_general_lock);
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ifx_ppa_drv_get_qos_status failed\n");
        return IFX_FAILURE;   
    }
    curr_jiffy = jiffies;
    
    for(i=0; i<status->max_buffer_size; i++)
    {   
        update_port_mib64_item( &status->mib[i].mib.total_rx_pkt,                &g_qos_mib_last_instant_read[port_id].mib[i].mib.total_rx_pkt,                 &g_qos_mib_accumulated[port_id].mib[i].mib.total_rx_pkt);
        update_port_mib64_item( &status->mib[i].mib.total_rx_bytes,              &g_qos_mib_last_instant_read[port_id].mib[i].mib.total_rx_bytes,               &g_qos_mib_accumulated[port_id].mib[i].mib.total_rx_bytes);
        
        update_port_mib64_item( &status->mib[i].mib.total_tx_pkt,                &g_qos_mib_last_instant_read[port_id].mib[i].mib.total_tx_pkt,                 &g_qos_mib_accumulated[port_id].mib[i].mib.total_tx_pkt);
        update_port_mib64_item( &status->mib[i].mib.total_tx_bytes,              &g_qos_mib_last_instant_read[port_id].mib[i].mib.total_tx_bytes,               &g_qos_mib_accumulated[port_id].mib[i].mib.total_tx_bytes);

        update_port_mib64_item( &status->mib[i].mib.cpu_path_small_pkt_drop_cnt, &g_qos_mib_last_instant_read[port_id].mib[i].mib.cpu_path_small_pkt_drop_cnt,  &g_qos_mib_accumulated[port_id].mib[i].mib.cpu_path_small_pkt_drop_cnt);
        update_port_mib64_item( &status->mib[i].mib.cpu_path_total_pkt_drop_cnt, &g_qos_mib_last_instant_read[port_id].mib[i].mib.cpu_path_total_pkt_drop_cnt,  &g_qos_mib_accumulated[port_id].mib[i].mib.cpu_path_total_pkt_drop_cnt);

        update_port_mib64_item( &status->mib[i].mib.fast_path_small_pkt_drop_cnt,&g_qos_mib_last_instant_read[port_id].mib[i].mib.fast_path_small_pkt_drop_cnt, &g_qos_mib_accumulated[port_id].mib[i].mib.fast_path_small_pkt_drop_cnt);
        update_port_mib64_item( &status->mib[i].mib.fast_path_total_pkt_drop_cnt,&g_qos_mib_last_instant_read[port_id].mib[i].mib.fast_path_total_pkt_drop_cnt, &g_qos_mib_accumulated[port_id].mib[i].mib.fast_path_total_pkt_drop_cnt);

        if( rate_flag )
        {   
            status->mib[i].mib.tx_diff = ( g_qos_mib_accumulated[port_id].mib[i].mib.total_tx_bytes >= g_qos_mib_accumulated_last[port_id].mib[i].mib.total_tx_bytes )? \
                            (g_qos_mib_accumulated[port_id].mib[i].mib.total_tx_bytes - g_qos_mib_accumulated_last[port_id].mib[i].mib.total_tx_bytes) : \
                            (g_qos_mib_accumulated[port_id].mib[i].mib.total_tx_bytes + (uint64_t)WRAPROUND_32BITS - g_qos_mib_accumulated_last[port_id].mib[i].mib.total_tx_bytes);

            status->mib[i].mib.tx_diff_L1 = status->mib[i].mib.tx_diff + (g_qos_mib_accumulated[port_id].mib[i].mib.total_tx_pkt - g_qos_mib_accumulated_last[port_id].mib[i].mib.total_tx_pkt) * status->overhd_bytes;


            status->mib[i].mib.tx_diff_jiffy = ( curr_jiffy > last_jiffy_qos_mib[port_id]) ? \
                          (curr_jiffy - last_jiffy_qos_mib[port_id] ): \
                          (curr_jiffy + (uint32_t )WRAPROUND_32BITS - last_jiffy_qos_mib[port_id] );
            
            status->mib[i].mib.sys_hz = HZ;
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"port[%d] queue[%d] bytes=%010llu(%010llu-%010llu) jiffy=%010llu(%010u-%010u) overhead=%010u pkts=%010u\n",
                                                   port_id, i,  
                                                   status->mib[i].mib.tx_diff,
                                                   g_qos_mib_accumulated[port_id].mib[i].mib.total_tx_bytes, 
                                                   g_qos_mib_accumulated_last[port_id].mib[i].mib.total_tx_bytes, 
                                                   status->mib[i].mib.tx_diff_jiffy, 
                                                   curr_jiffy,
                                                   last_jiffy_qos_mib[port_id],
                                                   status->overhd_bytes,
                                                   (uint32_t)(g_qos_mib_accumulated[port_id].mib[i].mib.total_tx_pkt - g_qos_mib_accumulated_last[port_id].mib[i].mib.total_tx_pkt));
        }
    }

    if( rate_flag )
    {
        g_qos_mib_accumulated_last[port_id] = g_qos_mib_accumulated[port_id];
        last_jiffy_qos_mib[port_id] = curr_jiffy;
    }
   
    ppa_lock_release(&g_general_lock);

    return IFX_SUCCESS;
}

//note, so far only ioctl will set rate_flag to 1, otherwise it will be zero in ppa timer
int32_t ppa_update_port_mib(PPA_PORT_MIB *mib, uint32_t rate_flag, uint32_t flag)
{
    uint32_t i, curr_jiffy;
    
    if( !mib ) return IFX_FAILURE;
    ppa_lock_get(&g_general_lock);
    
    if( ifx_ppa_drv_get_ports_mib(mib , 0)  != IFX_SUCCESS ) 
    {
        ppa_lock_release(&g_general_lock);
        return IFX_FAILURE;
    }
    
    for(i=0; i<mib->port_num; i++)
    {   
        update_port_mib64_item( (void *)&mib->mib_info[i].ig_fast_brg_pkts,  (void *)&g_port_mib_last_instant_read.mib_info[i].ig_fast_brg_pkts,  (void *)&g_port_mib_accumulated.mib_info[i].ig_fast_brg_pkts);
        update_port_mib64_item( (void *)&mib->mib_info[i].ig_fast_brg_bytes, (void *)&g_port_mib_last_instant_read.mib_info[i].ig_fast_brg_bytes, (void *)&g_port_mib_accumulated.mib_info[i].ig_fast_brg_bytes);
        
        update_port_mib64_item( (void *)&mib->mib_info[i].ig_fast_rt_ipv4_udp_pkts, (void *)&g_port_mib_last_instant_read.mib_info[i].ig_fast_rt_ipv4_udp_pkts, (void *)&g_port_mib_accumulated.mib_info[i].ig_fast_rt_ipv4_udp_pkts);
        update_port_mib64_item( (void *)&mib->mib_info[i].ig_fast_rt_ipv4_tcp_pkts, (void *)&g_port_mib_last_instant_read.mib_info[i].ig_fast_rt_ipv4_tcp_pkts, (void *)&g_port_mib_accumulated.mib_info[i].ig_fast_rt_ipv4_tcp_pkts);
        update_port_mib64_item( (void *)&mib->mib_info[i].ig_fast_rt_ipv4_mc_pkts,  (void *)&g_port_mib_last_instant_read.mib_info[i].ig_fast_rt_ipv4_mc_pkts,  (void *)&g_port_mib_accumulated.mib_info[i].ig_fast_rt_ipv4_mc_pkts);
        update_port_mib64_item( (void *)&mib->mib_info[i].ig_fast_rt_ipv4_bytes,    (void *)&g_port_mib_last_instant_read.mib_info[i].ig_fast_rt_ipv4_bytes,    (void *)&g_port_mib_accumulated.mib_info[i].ig_fast_rt_ipv4_bytes);

        update_port_mib64_item( (void *)&mib->mib_info[i].ig_fast_rt_ipv6_udp_pkts, (void *)&g_port_mib_last_instant_read.mib_info[i].ig_fast_rt_ipv6_udp_pkts, (void *)&g_port_mib_accumulated.mib_info[i].ig_fast_rt_ipv6_udp_pkts);
        update_port_mib64_item( (void *)&mib->mib_info[i].ig_fast_rt_ipv6_tcp_pkts, (void *)&g_port_mib_last_instant_read.mib_info[i].ig_fast_rt_ipv6_tcp_pkts, (void *)&g_port_mib_accumulated.mib_info[i].ig_fast_rt_ipv6_tcp_pkts);
        update_port_mib64_item( (void *)&mib->mib_info[i].ig_fast_rt_ipv6_mc_pkts,  (void *)&g_port_mib_last_instant_read.mib_info[i].ig_fast_rt_ipv6_mc_pkts,  (void *)&g_port_mib_accumulated.mib_info[i].ig_fast_rt_ipv6_mc_pkts);
        update_port_mib64_item( (void *)&mib->mib_info[i].ig_fast_rt_ipv6_bytes,    (void *)&g_port_mib_last_instant_read.mib_info[i].ig_fast_rt_ipv6_bytes,    (void *)&g_port_mib_accumulated.mib_info[i].ig_fast_rt_ipv6_bytes);
        
        update_port_mib64_item( (void *)&mib->mib_info[i].ig_cpu_pkts,   (void *)&g_port_mib_last_instant_read.mib_info[i].ig_cpu_pkts,   (void *)&g_port_mib_accumulated.mib_info[i].ig_cpu_pkts);
        update_port_mib64_item( (void *)&mib->mib_info[i].ig_cpu_bytes,  (void *)&g_port_mib_last_instant_read.mib_info[i].ig_cpu_bytes,  (void *)&g_port_mib_accumulated.mib_info[i].ig_cpu_bytes);
        update_port_mib64_item( (void *)&mib->mib_info[i].ig_drop_pkts,  (void *)&g_port_mib_last_instant_read.mib_info[i].ig_drop_pkts,  (void *)&g_port_mib_accumulated.mib_info[i].ig_drop_pkts);
        update_port_mib64_item( (void *)&mib->mib_info[i].ig_drop_bytes, (void *)&g_port_mib_last_instant_read.mib_info[i].ig_drop_bytes, (void *)&g_port_mib_accumulated.mib_info[i].ig_drop_bytes);
        update_port_mib64_item( (void *)&mib->mib_info[i].eg_fast_pkts,  (void *)&g_port_mib_last_instant_read.mib_info[i].eg_fast_pkts,  (void *)&g_port_mib_accumulated.mib_info[i].eg_fast_pkts);            

        if( rate_flag )
        { //in the future we need to add more statistics here
        }
    }

    if( rate_flag )
    {
        curr_jiffy = jiffies;
        
        if( last_jiffy_port_mib > 0 )
        {
            /*do rate calculation */
        }

        last_jiffy_port_mib = curr_jiffy;
        g_port_mib_accumulated_last = g_port_mib_accumulated;
    }
    
    ppa_lock_release(&g_general_lock);

    return IFX_SUCCESS;
}


//We don't really clear session's acceleration mib in PPE FW, but instead, we just record the mib counter for adjustment only
static int32_t set_prev_clear_session_mib(uint32_t flag)
{
    struct session_list_item *pp_item;
    uint32_t pos = 0;

    if ( ppa_session_start_iteration(&pos, &pp_item) == IFX_SUCCESS )
    {
        do
        {   
            pp_item->prev_clear_acc_bytes = pp_item->acc_bytes;
            pp_item->prev_clear_mips_bytes = pp_item->mips_bytes;            
            
        } while ( ppa_session_iterate_next(&pos, &pp_item) == IFX_SUCCESS  );
    }
    
    ppa_session_stop_iteration();
    return IFX_SUCCESS;
}

static int32_t set_prev_clear_mc_mib(uint32_t flag)
{
    struct mc_group_list_item *pp_item;
    uint32_t pos = 0;

    if ( ppa_mc_group_start_iteration(&pos, &pp_item) == IFX_SUCCESS )
    {
        do
        {   
            pp_item->prev_clear_acc_bytes = pp_item->acc_bytes;
            
        } while ( ppa_mc_group_iterate_next(&pos, &pp_item) == IFX_SUCCESS  );
    }
    
    ppa_mc_group_stop_iteration();
    return IFX_SUCCESS;
}

static int32_t set_prev_clear_netif_mib(uint32_t flag)
{
    struct netif_info *p_netif;
    uint32_t pos = 0;

#if defined(CONFIG_IFX_PPA_IF_MIB) && CONFIG_IFX_PPA_IF_MIB	
    if ( ppa_netif_start_iteration(&pos, &p_netif) == IFX_SUCCESS )
    {
        do
        {   
            p_netif->prev_clear_acc_rx = p_netif->acc_rx;
            p_netif->prev_clear_acc_tx = p_netif->acc_tx;            
        } while ( ppa_netif_iterate_next(&pos, &p_netif) == IFX_SUCCESS  );
    }    
    ppa_netif_stop_iteration();
    
#endif

    return IFX_SUCCESS;

}

int32_t ifx_ppa_get_dsl_mib(PPA_DSL_QUEUE_MIB *mib, uint32_t flag)
{
    int32_t res;
    
    ppa_lock_get(&g_general_lock);
    if( mib ) mib->flag = flag;
    res = ifx_ppa_drv_get_dsl_mib(mib, 0);

    ppa_lock_release(&g_general_lock);
    return res;
}

int32_t ifx_ppa_clear_dsl_mib(PPA_DSL_QUEUE_MIB *mib, uint32_t flag)
{
    int32_t res;
    
    if(!ifx_ppa_drv_hal_generic_hook ) 
        return IFX_FAILURE;
    ppa_lock_get(&g_general_lock);
    res = ifx_ppa_drv_hal_generic_hook(PPA_GENERIC_HAL_CLEAR_DSL_MIB,(void *)mib, 0 ); 
    ppa_lock_release(&g_general_lock);
    return res;
}

int32_t ppa_ioctl_get_dsl_mib(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info)
{
    int res=IFX_FAILURE;

    
    ppa_memset(&cmd_info->dsl_mib_info.mib, 0, sizeof(cmd_info->dsl_mib_info.mib) );

    res = ifx_ppa_get_dsl_mib( &cmd_info->dsl_mib_info.mib, cmd_info->dsl_mib_info.flags);
    
    if ( ppa_copy_to_user( (void *)arg, &cmd_info->dsl_mib_info, sizeof(cmd_info->dsl_mib_info)) != 0 )
        return IFX_FAILURE;

    return res;
}

int32_t ppa_ioctl_clear_dsl_mib(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info)
{
    int32_t res = ifx_ppa_clear_dsl_mib( &cmd_info->dsl_mib_info.mib, cmd_info->dsl_mib_info.flags);    
    
    return res;
}

int32_t ifx_ppa_get_ports_mib(PPA_PORT_MIB *mib,uint32_t rate_flag, uint32_t flag)
{
   return ppa_update_port_mib( mib, rate_flag, flag );    
}

int32_t ppa_ioctl_get_ports_mib(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info)
{
    int res=IFX_FAILURE;

    ppa_memset(&cmd_info->port_mib_info.mib, 0, sizeof(cmd_info->port_mib_info.mib) );

    res = ifx_ppa_get_ports_mib( &cmd_info->port_mib_info.mib, 1, cmd_info->port_mib_info.flags );
    
    if ( ppa_copy_to_user( (void *)arg, &cmd_info->port_mib_info, sizeof(cmd_info->port_mib_info)) != 0 )
        return IFX_FAILURE;

    return res;
}

int32_t ifx_ppa_clear_port_mib(PPA_PORT_MIB *mib, uint32_t flag)
{
    int32_t res;
    if(!ifx_ppa_drv_hal_generic_hook ) 
        return IFX_FAILURE;

    ppa_lock_get(&g_general_lock);
    res = ifx_ppa_drv_datapath_generic_hook(PPA_GENERIC_DATAPATH_CLEAR_MIB,(void *)mib, 0 ); 
    ppa_lock_release(&g_general_lock);
    reset_local_mib();
    set_prev_clear_session_mib(0);
    set_prev_clear_mc_mib(0);
    set_prev_clear_netif_mib(0);
    return res;
}

int32_t ppa_ioctl_clear_ports_mib(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info)
{
    int32_t res;
    
    ppa_check_hit_stat_clear_mib(PPA_CMD_CLEAR_PORT_MIB);

    if( cmd_info )
        res = ifx_ppa_clear_port_mib( &cmd_info->port_mib_info.mib, cmd_info->port_mib_info.flags);    
    else 
        res = ifx_ppa_clear_port_mib( NULL, 0);    
    
    return res;
}

EXPORT_SYMBOL(g_general_lock);
EXPORT_SYMBOL(ifx_ppa_get_ports_mib);
EXPORT_SYMBOL(ppa_ioctl_get_dsl_mib);
EXPORT_SYMBOL(ppa_ioctl_clear_dsl_mib);
EXPORT_SYMBOL(ppa_ioctl_get_ports_mib);
EXPORT_SYMBOL(ppa_ioctl_clear_ports_mib);
EXPORT_SYMBOL(ppa_update_port_mib);
EXPORT_SYMBOL(ppa_update_qos_mib);
EXPORT_SYMBOL(reset_local_mib);
EXPORT_SYMBOL(ifx_ppa_get_dsl_mib);
