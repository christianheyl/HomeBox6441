/*
 * Copyright (c) 2010, Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* Header files */

#include <osdep.h>
#include <adf_os_mem.h>
#include <adf_os_lock.h>
#include <adf_os_io.h>
#include <adf_os_waitq.h>
#include <adf_nbuf.h>
#include <adf_os_defer.h>
#include <athdefs.h>
#include "a_types.h"
#include "a_osapi.h"
#include "htc_host_api.h"
#include "wmi.h"
#include "wmi_host_api.h"
#include "wmi_host.h"
#include "hif.h"

#define MAX_CMD_NUMBER  62//31//62
struct registerWrite {
    u_int32_t reg;
    u_int32_t val;
} /*data*/;

struct registerRMW {
    u_int32_t reg; /* reg addr */
    u_int32_t clr; /* clear bit mask */
    u_int32_t set; /* bits to be written onto */
} /*data*/;

struct registerMultiWrite {
    struct registerWrite data[MAX_CMD_NUMBER];
};

struct registerMultiWrite multiBuf;
u_int32_t multiBufIndex = 0;

#ifdef WMI_RETRY
static A_STATUS wmi_cmd_issue(struct wmi_t *wmip,
                              adf_nbuf_t netbuf,
                              WMI_COMMAND_ID cmdId,
                              a_uint16_t cmdLen,
                              a_uint16_t retry);
#else
static A_STATUS wmi_cmd_issue(struct wmi_t *wmip, 
                              adf_nbuf_t netbuf, 
                              WMI_COMMAND_ID cmdId, 
                              a_uint16_t cmdLen);
#endif

static void wmi_cmd_send_complete(struct wmi_t * wmip, adf_nbuf_t netbuf);
static A_STATUS wmi_control_rx(struct wmi_t *wmip, adf_nbuf_t netbuf);
//static void WmiCtrl_EpTxComplete(void *Context, adf_nbuf_t *bufArray, a_uint8_t bufNum);
static void WmiCtrl_EpTxComplete(void *Context, adf_nbuf_t buf, HTC_ENDPOINT_ID bufNum);
#ifdef HTC_TEST
static void WmiCtrl_EpRecv(void *Context, adf_nbuf_t netbuf, HTC_ENDPOINT_ID Ep, a_uint8_t pipeID);
#else
static void WmiCtrl_EpRecv(void *Context, adf_nbuf_t netbuf, HTC_ENDPOINT_ID Ep);
#endif

#ifdef WMI_RETRY
void wmi_retry_timer(void *arg);
static void wmi_rsp_callback(void *Context, WMI_COMMAND_ID cmdId, a_uint16_t cmdLen, adf_nbuf_t netbuf,a_uint16_t seqId);
#else
static void wmi_rsp_callback(void *Context, WMI_COMMAND_ID cmdId, a_uint16_t cmdLen, adf_nbuf_t netbuf);
#endif

void wmi_evthdl_tasklet(void *Context);

void 
wmi_evthdl_tasklet(void *Context)
{
    a_uint8_t *netdata;
    a_uint32_t netlen;
    a_uint16_t evt_Id;
    adf_nbuf_t netbuf;
    struct wmi_t *wmip = (struct wmi_t *)Context;

    while (1) {
        /* dequeue */
        LOCK_WMI(wmip);
        
        if (wmip->evtHead == wmip->evtTail) {
            //adf_os_print("[%s] WMI queue is empty !! Should not happen!!\n", __FUNCTION__);
            UNLOCK_WMI(wmip);
            //adf_os_assert(0);
            return;
        }
        
        netbuf = wmip->wmi_evt_queue[wmip->evtHead].buf;
        evt_Id = wmip->wmi_evt_queue[wmip->evtHead].evtid;
        netdata = wmip->wmi_evt_queue[wmip->evtHead].netdata;
        netlen = wmip->wmi_evt_queue[wmip->evtHead].netlen;
    
        wmip->evtHead = (wmip->evtHead + 1) % WMI_EVT_QUEUE_SIZE;
        
        UNLOCK_WMI(wmip);
    
        /* invoke the wmi event handler */
        wmip->wmi_cb(wmip->devt, evt_Id, netdata, netlen);
    
        /* free the netbuf */
        adf_nbuf_free(netbuf);
    } /* end of while(1) */
}

/* WMI framework functions */
void * wmi_init(void *devt, adf_os_handle_t os_hdl, wmi_event_callback_t cb, wmi_stop_callback_t stop_cb)
{
    struct wmi_t *wmip;
#ifdef NBUF_PREALLOC_POOL   
    a_uint16_t headroom = HTC_HDR_LENGTH + sizeof(WMI_CMD_HDR);
#endif    

    wmip = adf_os_mem_alloc(os_hdl, sizeof(struct wmi_t));
    if (wmip == NULL) {
        return (NULL);
    }

    adf_os_mem_zero(wmip, sizeof(*wmip));
    adf_os_spinlock_init(&wmip->wmi_lock);

#ifdef MAGPIE_SINGLE_CPU_CASE
    adf_os_spinlock_init(&wmip->wmi_op_lock);
#else
    adf_os_init_mutex(&wmip->wmi_op_mutex);
    adf_os_init_mutex(&wmip->wmi_cmd_mutex);
    adf_os_mutex_acquire(&wmip->wmi_cmd_mutex);
#endif

    adf_os_spinlock_init(&wmip->wmi_op_lock_bh);
    
#ifdef NBUF_PREALLOC_POOL
	wmip->wmi_cmd_nbuf = adf_nbuf_alloc(os_hdl, WMI_CMD_MAXBYTES+headroom, headroom, 0);
    if ( wmip->wmi_cmd_nbuf == ADF_NBUF_NULL ) {
        goto bad;
    }
#endif    

    wmip->wmi_cb = cb;
    wmip->wmi_stop_cb = stop_cb;
    wmip->devt = devt;
    wmip->os_hdl = os_hdl;

#ifndef __linux__
    adf_os_init_waitq(&wmip->wq);
#endif

    wmip->wmi_in_progress = 0;

    wmip->evtHead = 0;
    wmip->evtTail = 0;
    adf_os_create_bh(os_hdl, &wmip->wmi_evt_bh, wmi_evthdl_tasklet, wmip);
    
#ifdef WMI_RETRY
    adf_os_timer_init(NULL, &wmip->wmi_retry_timer_instance, wmi_retry_timer, wmip);
#endif
    return (wmip);

#ifdef NBUF_PREALLOC_POOL
bad:
    adf_os_mem_free(wmip);
    return NULL;
#endif    
}

void wmi_shutdown(void *wmi_handle)
{
    struct wmi_t * wmip = (struct wmi_t *)wmi_handle;

    if (wmip == NULL) {
        return;
    }
    wmi_stop(wmi_handle);

    adf_os_destroy_bh(wmip->os_hdl, &wmip->wmi_evt_bh);

    adf_os_spinlock_destroy(&wmip->wmi_lock);

#ifdef NBUF_PREALLOC_POOL       
    if ( wmip->wmi_cmd_nbuf != ADF_NBUF_NULL ) {
        adf_nbuf_free(wmip->wmi_cmd_nbuf);
    }
#endif    
    adf_os_mem_free(wmip);

}

A_STATUS wmi_connect(HTC_HANDLE hHTC, void *hWMI, HTC_ENDPOINT_ID *pWmi_ctrl_epid)
{
    A_STATUS status;
    struct wmi_t *wmip = (struct wmi_t *)hWMI;
    HTC_SERVICE_CONNECT_REQ connect;

    /* store the HTC handle */
    wmip->HtcHandle = hHTC;

    adf_os_mem_zero(&connect,sizeof(connect));

    /* It is the 'Context' parameter when EP callbacks are invoked. */
    connect.EpCallbacks.pContext = wmip;
    connect.EpCallbacks.EpTxComplete = WmiCtrl_EpTxComplete;
    connect.EpCallbacks.EpRecv = WmiCtrl_EpRecv;
    connect.ServiceID = WMI_CONTROL_SVC;
#ifdef MAGPIE_SINGLE_CPU_CASE
    //connect.UL_PipeID = 1;//HIF_USB_PIPE_COMMAND;
    //connect.DL_PipeID = 1;//HIF_USB_PIPE_INTERRUPT;
#else
    //connect.UL_PipeID = 1;
    //connect.DL_PipeID = 2;
#endif

    status = HTCConnectService(hHTC, &connect, &wmip->wmi_endpoint_id);

	//adf_os_print(">> wmi ENDPOINT ID = %x \n",wmip->wmi_endpoint_id);

    *pWmi_ctrl_epid = wmip->wmi_endpoint_id;

    return status;
}

static void wmi_cmd_send_complete(struct wmi_t * wmip, adf_nbuf_t netbuf)
{
#ifdef NBUF_PREALLOC_POOL
    a_uint8_t *netdata;
    a_uint32_t netlen;
    adf_nbuf_pull_head(netbuf, sizeof(WMI_CMD_HDR));

    adf_nbuf_peek_header(netbuf, &netdata, &netlen);

    adf_nbuf_trim_tail(netbuf, netlen);
#else    
    adf_nbuf_free(netbuf);
#endif
}

static A_STATUS wmi_control_rx(struct wmi_t *wmip, adf_nbuf_t netbuf)
{
    A_STATUS status = A_OK;
    a_uint16_t cmd_evt_Id, seqId;
    //adf_os_handle_t os_hdl = wmip->os_hdl;
    WMI_CMD_HDR *wmi_cmd_hdr;
    a_uint8_t *netdata;
    a_uint32_t netlen;

#ifdef NBUF_PREALLOC_POOL
    netbuf = adf_nbuf_copy(netbuf);
    if(netbuf == NULL) {
        adf_os_print("[%s] allocate nbuf fail !\n",__FUNCTION__);
        return A_NO_RESOURCE;
    }
#endif

    adf_nbuf_peek_header(netbuf, &netdata, &netlen);

    wmi_cmd_hdr = (WMI_CMD_HDR *)netdata;

    cmd_evt_Id = adf_os_ntohs(wmi_cmd_hdr->commandId);
    seqId = adf_os_ntohs(wmi_cmd_hdr->seqNo);

    netdata += sizeof(WMI_CMD_HDR);
    netlen -= sizeof(WMI_CMD_HDR);

    if (cmd_evt_Id & 0x1000) {    /* WMI event */
        /* enqueue netbuf */
        LOCK_WMI(wmip);

        if ((wmip->evtTail + 1) % WMI_EVT_QUEUE_SIZE == wmip->evtHead) {
            adf_os_print("[%s] WMI event queue full, going to free this wmi event.\n", __FUNCTION__);
            UNLOCK_WMI(wmip);

            return A_NO_RESOURCE;
        }
        
        wmip->wmi_evt_queue[wmip->evtTail].buf = netbuf;
        wmip->wmi_evt_queue[wmip->evtTail].evtid = cmd_evt_Id;
        wmip->wmi_evt_queue[wmip->evtTail].netdata = netdata;
        wmip->wmi_evt_queue[wmip->evtTail].netlen = netlen;

        wmip->evtTail = (wmip->evtTail + 1) % WMI_EVT_QUEUE_SIZE;

        UNLOCK_WMI(wmip);

        /* schedule the tasklet to handle this WMI event */
        adf_os_sched_bh(wmip->os_hdl, &wmip->wmi_evt_bh);

        //wmip->wmi_cb(wmip->devt, cmd_evt_Id, netdata, netlen);
    }
    else {    /* WMI command response */
#ifdef WMI_RETRY
        wmi_rsp_callback(wmip, cmd_evt_Id, (a_uint16_t)netlen, netbuf,seqId);
#else        
        wmi_rsp_callback(wmip, cmd_evt_Id, netlen, netbuf);
#endif        
        adf_nbuf_free(netbuf);
    }

    return status;

}

//static void WmiCtrl_EpTxComplete(void *Context, adf_nbuf_t *bufArray, a_uint8_t bufNum)
static void WmiCtrl_EpTxComplete(void *Context, adf_nbuf_t buf, HTC_ENDPOINT_ID bufNum)
{
    struct wmi_t *wmip = (struct wmi_t *)Context;
    //wmi_cmd_send_complete(wmip, bufArray[0]);
    wmi_cmd_send_complete(wmip, buf);
}

#ifdef HTC_TEST
static void WmiCtrl_EpRecv(void *Context, adf_nbuf_t netbuf, HTC_ENDPOINT_ID Ep, a_uint8_t pipeID)
#else
static void WmiCtrl_EpRecv(void *Context, adf_nbuf_t netbuf, HTC_ENDPOINT_ID Ep)
#endif
{
    A_STATUS status = A_OK;
    struct wmi_t *wmip = (struct wmi_t *)Context;
    //adf_os_handle_t os_hdl = wmip->os_hdl;

    /* if wmi_stop_flag is set, don't handle this late wmi event or wmi cmd response */
    if (wmip->wmi_stop_flag == TRUE) {
#ifndef NBUF_PREALLOC_POOL    	
        adf_nbuf_free(netbuf);
#endif        
        return;
    }

    if ( Ep == FWRecoverEpid ) {
        adf_os_print("%s_%d: Recv fw rcv pattern\n", __FUNCTION__, __LINE__);
        wmi_stop(wmip);
        adf_nbuf_free(netbuf);
    } else {
        status = wmi_control_rx(wmip, netbuf);
        if (A_FAILED(status)) {
#ifndef NBUF_PREALLOC_POOL    	
            adf_nbuf_free(netbuf);
#endif        
        }
    }
}

#ifdef WMI_RETRY
static A_STATUS wmi_cmd_issue(struct wmi_t *wmip,
                              adf_nbuf_t netbuf,
                              WMI_COMMAND_ID cmdId,
                              a_uint16_t cmdLen,
                              a_uint16_t retry)
#else
static A_STATUS wmi_cmd_issue(struct wmi_t *wmip, 
                              adf_nbuf_t netbuf, 
                              WMI_COMMAND_ID cmdId, 
                              a_uint16_t cmdLen)
#endif
{
    A_STATUS status = A_OK;
    WMI_CMD_HDR *wmi_cmd_hdr;

#ifdef WMI_RETRY
    wmip->wmi_last_sent_cmd = cmdId;
#endif    
    /* wmi header */
    wmi_cmd_hdr = (WMI_CMD_HDR *)adf_nbuf_push_head(netbuf, sizeof(WMI_CMD_HDR));
    wmi_cmd_hdr->commandId = adf_os_htons(cmdId);
#ifdef WMI_RETRY
    if(retry)
        wmi_cmd_hdr->seqNo = adf_os_htons(wmip->txSeqId);
    else
#endif    
      {   
        wmip->txSeqId++;
        wmi_cmd_hdr->seqNo = adf_os_htons(wmip->txSeqId);
      }
	
    /* invoke HTC API to send the WMI command packet */
    if ((status = HTCSendPkt(wmip->HtcHandle, ADF_NBUF_NULL, netbuf, wmip->wmi_endpoint_id)) != A_OK) {
        adf_os_print("[%s %d] Failed to send WMI cmd.\n", __FUNCTION__, __LINE__);
    }

    return status;
}

#ifdef WMI_RETRY
char wmi_data[512];
void
wmi_retry_timer(void *arg)
{
    struct wmi_t *wmip = (struct wmi_t *)arg;
    WMI_COMMAND_ID cmdId;
    a_uint16_t length;
    adf_nbuf_t netbuf = NULL;
    a_uint8_t *pData;
    a_uint16_t headroom = HTC_HDR_LENGTH + sizeof(WMI_CMD_HDR)+20;
    A_STATUS status = A_OK;

    cmdId = wmip->wmi_last_sent_cmd;
    length = wmip->wmi_retrycmdLen;

    printk(" wmi timer enter %lu   \n",jiffies);
    if(wmip->LastSeqId == wmip->txSeqId){
        adf_os_print("Response already recieved \n");
        return;
    } 
    printk("copylenght %d \n",length);
    netbuf = adf_nbuf_alloc(wmip->os_hdl, headroom + length, headroom, 0);

    if (netbuf == ADF_NBUF_NULL) {
        adf_os_print("nbuf allocation for WMI retry echo cmd failed!");

        adf_os_timer_start(&wmip->wmi_retry_timer_instance, 1000);
        return;
    }
    printk("length of alloc  %d  buf %x \n",adf_nbuf_len(netbuf),(int)netbuf);

    pData = (a_uint8_t *)adf_nbuf_put_tail(netbuf, length);
    adf_os_mem_copy(pData,&wmi_data[0], length);

    printk(" %x wmi retry  %d seq  %d  \n",(int)wmip ,wmip->wmi_retrycnt,wmip->txSeqId);
    wmip->wmi_retrycnt++;

    adf_os_timer_start(&wmip->wmi_retry_timer_instance, 1000);

    printk("wmi_cmd issue retry  \n");
    status = wmi_cmd_issue(wmip, netbuf, cmdId, length,1);
    if (A_FAILED(status)) {
        printk("wmi cmd issue fail for retyr");

    }

    return;
}
#endif

static void 
#ifdef WMI_RETRY
wmi_rsp_callback(void *Context, WMI_COMMAND_ID cmdId, a_uint16_t cmdLen, adf_nbuf_t netbuf,a_uint16_t seqId)
#else    
wmi_rsp_callback(void *Context, WMI_COMMAND_ID cmdId, a_uint16_t cmdLen, adf_nbuf_t netbuf)
#endif
{
    struct wmi_t *wmip = (struct wmi_t *)Context;

    a_uint8_t *netdata;
    a_uint32_t netlen;
    //adf_net_handle_t anet = wmip->devt;

#ifdef WMI_RETRY
    if(wmip->LastSeqId == seqId){
        adf_os_print("Already recv reject cur %d last %d seq %d \n",wmip->txSeqId,wmip->LastSeqId,seqId);
        return ;
    }

    if(wmip->txSeqId == seqId){
        wmip->LastSeqId =seqId;
    }
    else if( (wmip->txSeqId -1) == seqId){
        adf_os_print("Duplicate Or retry Resp reject \n");
        return ;
    }
    else{
        adf_os_print("Unhandled case expexted %d recv %d txseq %d  \n",wmip->LastSeqId,seqId,wmip->txSeqId);
        return ;
    }

    adf_os_timer_cancel(&wmip->wmi_retry_timer_instance);
    wmip->wmi_retrycnt =0;
    wmip->wmi_retrycmdLen=0;
#endif
    
    adf_nbuf_pull_head(netbuf, sizeof(WMI_CMD_HDR));

    adf_nbuf_peek_header(netbuf, &netdata, &netlen);

    if (wmip->cmd_rsp_buf != NULL && wmip->cmd_rsp_len != 0) {
        adf_os_mem_copy(wmip->cmd_rsp_buf, netdata, wmip->cmd_rsp_len);
    }

    //adf_nbuf_free(anet, netbuf);
#ifdef MAGPIE_SINGLE_CPU_CASE
#else
#ifdef __linux__
    adf_os_mutex_release(&wmip->wmi_cmd_mutex);
#else
    adf_os_wake_waitq(&wmip->wq);
#endif
#endif
}

void 
wmi_start(void *wmi_handle)
{
    struct wmi_t* wmip = (struct wmi_t *)wmi_handle;
#ifndef __linux__
    adf_os_reset_waitq(&wmip->wq);
#endif
#ifdef MAGPIE_SINGLE_CPU_CASE
    adf_os_spin_lock_irq(&wmip->wmi_op_lock, wmip->wmi_op_flags);
#else
    if (adf_os_mutex_acquire(&wmip->wmi_op_mutex)) {
        adf_os_print("[%s %d] Get mutex not successfully.\n", __FUNCTION__, __LINE__);
        return;
    }
#endif

    wmip->wmi_stop_flag = FALSE;
    wmip->wmi_usb_stop_flag = FALSE;

#ifdef MAGPIE_SINGLE_CPU_CASE
    adf_os_spin_unlock_irq(&wmip->wmi_op_lock, wmip->wmi_op_flags);
#else
    adf_os_mutex_release(&wmip->wmi_op_mutex);
#endif

}

void
wmi_stop(void *wmi_handle)
{
    struct wmi_t* wmip = (struct wmi_t *)wmi_handle;
    int ret;

    /* stop cb */
    if (wmip->wmi_stop_cb)
        wmip->wmi_stop_cb(wmip->devt);

#ifdef MAGPIE_SINGLE_CPU_CASE
    adf_os_spin_lock_irq(&wmip->wmi_op_lock, wmip->wmi_op_flags);
#else
//    if (adf_os_mutex_acquire(&wmip->wmi_op_mutex)) {
//        adf_os_print("[%s %d] Get mutex not successfully.\n", __FUNCTION__, __LINE__);
//        return;
//    }
#endif

    wmip->wmi_stop_flag = TRUE;

#ifdef MAGPIE_SINGLE_CPU_CASE
    adf_os_spin_unlock_irq(&wmip->wmi_op_lock, wmip->wmi_op_flags);
#else
//    adf_os_mutex_release(&wmip->wmi_op_mutex);
#endif
#ifdef WMI_RETRY
    adf_os_print("Cancel retry timer \n");
    adf_os_timer_cancel(&wmip->wmi_retry_timer_instance);
#endif

#ifdef MAGPIE_SINGLE_CPU_CASE
#else
#ifdef __linux__
    /* try lock before release it to keep semaphore count consistant */
    ret =down_trylock(&wmip->wmi_cmd_mutex);		
    adf_os_mutex_release(&wmip->wmi_cmd_mutex);
#else
    adf_os_wake_waitq(&wmip->wq);
#endif
#endif

}

struct WMI_COMMAND_STRUCT
{
	a_uint8_t     flag;     /* displaby on/off */
    a_uint8_t*    str;      /* display message string */
};

struct WMI_COMMAND_STRUCT wmi_command_str_table[] =
{
	{0, ""},
	{0, "WMI_ECHO_CMDID"},
    {0, "WMI_ACCESS_MEMORY_CMDID"},

    /* Commands to Target */
    {1, "WMI_DISABLE_INTR_CMDID"},
    {1, "WMI_ENABLE_INTR_CMDID"},
    {1, "WMI_RX_LINK_CMDID"},
    {0, "WMI_ATH_INIT_CMDID"},
    {1, "WMI_ABORT_TXQ_CMDID"},
    {1, "WMI_STOP_TX_DMA_CMDID"},
    {1, "WMI_STOP_DMA_RECV_CMDID"},
    {1, "WMI_ABORT_TX_DMA_CMDID"},
    {1, "WMI_DRAIN_TXQ_CMDID"},
    {1, "WMI_DRAIN_TXQ_ALL_CMDID"},
    {1, "WMI_START_RECV_CMDID"},
    {1, "WMI_STOP_RECV_CMDID"},
    {1, "WMI_FLUSH_RECV_CMDID"},
    {0, "WMI_SET_MODE_CMDID"},
    {1, "WMI_RESET_CMDID"},
    {0, "WMI_NODE_CREATE_CMDID"},
    {0, "WMI_NODE_REMOVE_CMDID"},
    {0, "WMI_VAP_REMOVE_CMDID"},
    {0, "WMI_VAP_CREATE_CMDID"},
    {0, "WMI_BEACON_UPDATE_CMDID"},
    {0, "WMI_REG_READ_CMDID"},
    {0, "WMI_REG_WRITE_CMDID"},
	{1, "WMI_RC_STATE_CHANGE_CMDID"},
	{1, "WMI_RC_RATE_UPDATE_CMDID"},
	{0, "WMI_DEBUG_INFO_CMDID"},
	{1, "WMI_HOST_ATTACH"},
	{1, "WMI_TARGET_IC_UPDATE_CMDID"},
	{1, "WMI_TGT_STATS_CMDID"},
    {1, "WMI_TX_AGGR_ENABLE_CMDID"},
    {1, "WMI_TGT_DETACH_CMDID"},
    {1, "WMI_TGT_SET_DESC_TPC"},
    {1, "WMI_BT_COEX_CMDID"},
    {1, "WMI_TX99_START_CMDID"},
    {1, "WMI_TX99_STOP_CMDID"},
    {1, "WMI_NODE_UPDATE_CMDID"},
    {0, "WMI_PS_SET_STATE_CMD"},
    {0, "WMI_ENABLE_BEACON_FILTER_CMD"},
    {1, "WMI_NODE_GETRATE_CMDID"},
    {1, "WMI_RATETABLE_TXPOWER_CMDID"},
#ifdef MAGPIE_HIF_GMAC
    {1, "WMI_HOST_SEEK_CREDIT"},
#endif        
#if ENCAP_OFFLOAD    
    {1, "WMI_VAP_UPDATE_CMDID"},
#endif

};


/* debug print cmmand name */
void wmi_cmd_print(WMI_COMMAND_ID cmdId)
{
	if (cmdId >= (sizeof(wmi_command_str_table) / sizeof(struct WMI_COMMAND_STRUCT)))
	{
		adf_os_print("error cmdId!!!\n");
		return;
	}
	if (wmi_command_str_table[cmdId].flag)
	{
		adf_os_print("%s: %s\n", __func__, wmi_command_str_table[cmdId].str);
	}
}

/* WMI helper functions */
A_STATUS __ahdecl
wmi_cmd(void *wmi_handle, WMI_COMMAND_ID cmdId, a_uint8_t *pCmdBuffer, a_uint32_t length, a_uint8_t *pRspBuffer, a_uint32_t rspLen, a_uint32_t timeout)
{
    struct wmi_t *wmip = (struct wmi_t *)wmi_handle;
    A_STATUS status = A_OK;
#ifndef NBUF_PREALLOC_POOL    
    a_uint16_t headroom = HTC_HDR_LENGTH + sizeof(WMI_CMD_HDR);
#endif    
    adf_os_handle_t os_hdl;
    adf_nbuf_t netbuf;
    a_uint8_t *pData;

    /* debug print message */
    /* wmi_cmd_print(cmdId); */

    if (wmip == NULL) {
        adf_os_print("WMI handle is NULL!");
        status = A_EINVAL;
        goto _err_tx_out;
    }

    if (wmip->wmi_endpoint_id == 0) {
        adf_os_print("WMI endpoint ID is invalid!");
        status = A_EPROTO;
        goto _err_tx_out;
    }
#ifdef NBUF_PREALLOC_POOL
    adf_os_assert( length <= WMI_CMD_MAXBYTES );
#endif
    os_hdl = wmip->os_hdl;

#ifdef MAGPIE_SINGLE_CPU_CASE
    adf_os_spin_lock_irq(&wmip->wmi_op_lock, wmip->wmi_op_flags);
#else
    //adf_os_spin_lock_bh(&wmip->wmi_op_lock_bh);
    adf_os_mutex_acquire(&wmip->wmi_op_mutex);

    //if (adf_os_mutex_acquire(&wmip->wmi_op_mutex)) {
    if (0) {
        adf_os_print("adf_os_mutex_acquire returns not zero!!\n");
        status = A_ECANCELED;
        goto _err_tx_out1;
    }
#endif

#ifdef NBUF_PREALLOC_POOL
		netbuf = wmip->wmi_cmd_nbuf;
#else
		/* allocate memory for command */
#ifdef ATH_WINHTC
    netbuf = adf_nbuf_alloc(os_hdl, length, headroom, 0);
#else
    /* linux correct path: headroom + length */
    netbuf = adf_nbuf_alloc(os_hdl, headroom + length, headroom, 0);
#endif
#endif

    if (netbuf == ADF_NBUF_NULL) {
        adf_os_print("nbuf allocation for WMI echo cmd failed!");
        status = A_NO_MEMORY;
#ifdef MAGPIE_SINGLE_CPU_CASE
        adf_os_spin_unlock_irq(&wmip->wmi_op_lock, wmip->wmi_op_flags);
#else
        adf_os_mutex_release(&wmip->wmi_op_mutex);
#endif        
        goto _err_tx_out;
    }

    if (length != 0 && pCmdBuffer != NULL) {
        pData = (a_uint8_t *)adf_nbuf_put_tail(netbuf, length);
        adf_os_mem_copy(pData, pCmdBuffer, length);
#ifdef WMI_RETRY
        adf_os_mem_copy(&wmi_data[0], pCmdBuffer, length);
#endif
    }

    if (wmip->wmi_usb_stop_flag == TRUE){
        wmip->wmi_stop_flag = TRUE;
    }
    /* check if wmi_stop_flag is set */
    if (wmip->wmi_stop_flag == TRUE) {
        //adf_os_print("wmi is stopped...\n");
        status = A_ERROR;
        goto _err_tx_out2;
    }

    /* record the rsp buffer and length */
    wmip->cmd_rsp_buf = pRspBuffer;
    wmip->cmd_rsp_len = rspLen;

#ifdef WMI_RETRY
    wmip->wmi_retrycmdLen = length;
    adf_os_timer_start(&wmip->wmi_retry_timer_instance, 1000);
#endif

#ifdef WMI_RETRY
    status = wmi_cmd_issue(wmip, netbuf, cmdId, length, 0);
#else    
    status = wmi_cmd_issue(wmip, netbuf, cmdId, length);
#endif    
    
    if (A_FAILED(status)) {
        goto _err_tx_out2;
    }

#ifdef MAGPIE_SINGLE_CPU_CASE
#else
#ifdef __linux__
    adf_os_mutex_acquire(&wmip->wmi_cmd_mutex);
#else
    if((adf_os_sleep_waitq(&wmip->wq, 10000)) == A_STATUS_FAILED)
    {
        if(wmip->wmiCmdFailCnt++ < 5)
        {
            adf_os_print("wmi_cmd() , %dth try\n",wmip->wmiCmdFailCnt);
//            goto again;
        }
        else
        {
            adf_os_print("wmi_cmd() too long , exit\n");
            wmip->wmi_stop_flag = TRUE;
        }
    }
    else
    {
        wmip->wmiCmdFailCnt = 0;
    }
#endif /* #ifdef __linux__ */
#endif /* #ifdef MAGPIE_SINGLE_CPU_CASE */

#ifdef MAGPIE_SINGLE_CPU_CASE
    adf_os_spin_unlock_irq(&wmip->wmi_op_lock, wmip->wmi_op_flags);
#else
    adf_os_mutex_release(&wmip->wmi_op_mutex);
#endif

    return status;

_err_tx_out2:
#ifdef NBUF_PREALLOC_POOL
    wmi_cmd_send_complete(wmip, netbuf);
#endif

#ifdef MAGPIE_SINGLE_CPU_CASE
    adf_os_spin_unlock_irq(&wmip->wmi_op_lock, wmip->wmi_op_flags);
#else
    adf_os_mutex_release(&wmip->wmi_op_mutex);
#endif

_err_tx_out1:
#ifndef NBUF_PREALLOC_POOL	
    adf_nbuf_free(netbuf);
#endif    
_err_tx_out:
    return status;
}


void __ahdecl
wmi_reg_write_single(void *wmi_handle, u_int reg, u_int32_t val)
{
    u_int32_t cmd_status = -1;
    struct registerWrite data;
    u_int32_t rsp_status;

    data.reg = cpu_to_be32(reg);
    data.val = cpu_to_be32(val);

    cmd_status = wmi_cmd(wmi_handle, 
        WMI_REG_WRITE_CMDID, 
        (u_int8_t *) &data, 
        sizeof(struct registerWrite), 
        (u_int8_t *) &rsp_status,
        sizeof(rsp_status),
        100 /* timeout unit? */);

    if (cmd_status) {
        if (cmd_status != -1)
            /*ath_hal_printf(NULL, "Host : WMI COMMAND WRITE FAILURE stat = %x\n", cmd_status);*/
        return;
    }
}
void __ahdecl
wmi_reg_rmw(void *wmi_handle, u_int reg, u_int32_t clr, u_int32_t set)
{
    u_int32_t cmd_status = -1;
    struct registerRMW data;
    u_int32_t rsp_status;

    data.reg = cpu_to_be32(reg);
    data.clr = cpu_to_be32(clr);
    data.set = cpu_to_be32(set);

    cmd_status = wmi_cmd(wmi_handle, 
        WMI_REG_RMW_CMDID, 
        (u_int8_t *) &data, 
        sizeof(struct registerRMW), 
        (u_int8_t *) &rsp_status,
        sizeof(rsp_status),
        100 /* timeout unit? */);

    if (cmd_status) {
        if (cmd_status != -1)
            adf_os_print("WMI_REG_RMW_CMDID failed\n");
        return;
    }
}


void __ahdecl
wmi_reg_write_delay(void *wmi_handle, u_int reg, u_int32_t val)
{
    u_int32_t cmd_status = -1;
    u_int32_t rsp_status;
    struct registerMultiWrite multidata;
    u_int write_count = 0;
	u_int i;
    struct wmi_t *wmip = (struct wmi_t *)wmi_handle;

    /* lock */
    adf_os_mutex_acquire(&wmip->wmi_op_mutex);

    //adf_os_print("%s: reg = 0x%x, val = 0x%x\n", __FUNCTION__, reg, val);

    /* store to buffer */
    multiBuf.data[multiBufIndex].reg = cpu_to_be32(reg);
    multiBuf.data[multiBufIndex].val = cpu_to_be32(val);
    multiBufIndex++;
        
    /* if buffer full, form multiple write */
    if (multiBufIndex == MAX_CMD_NUMBER)
    {
        for (i=0; i<multiBufIndex; i++)
        {
            multidata.data[i].reg = multiBuf.data[i].reg;
            multidata.data[i].val = multiBuf.data[i].val;
        }
        write_count = multiBufIndex;
        multiBufIndex = 0;
    }
    
    /* unlock */
    adf_os_mutex_release(&wmip->wmi_op_mutex);

    /* Perform multi write */
    if (write_count != 0)
    {
        cmd_status = wmi_cmd(wmi_handle, 
                WMI_REG_WRITE_CMDID, 
                (u_int8_t*)&(multidata), 
                sizeof(struct registerWrite)*write_count, 
                (u_int8_t*)&rsp_status,
                sizeof(rsp_status),
                100 /* timeout unit? */);
    
        if(cmd_status){
            //ath_hal_printf(NULL, "Host : WMI COMMAND WRITE FAILURE stat = %x\n", cmd_status);
            return;
        }
    }
}

void __ahdecl
wmi_reg_write_flush(void *wmi_handle)
{
    u_int32_t cmd_status = -1;
    u_int32_t rsp_status;
    struct registerMultiWrite multidata;
    u_int write_count = 0;
	u_int i;
    struct wmi_t *wmip = (struct wmi_t *)wmi_handle;

    /* lock */
    adf_os_mutex_acquire(&wmip->wmi_op_mutex);
        
    /* if buffer note empty, form multiple write */
    if (multiBufIndex != 0)
    {
        for (i=0; i<multiBufIndex; i++)
        {
            multidata.data[i].reg = multiBuf.data[i].reg;
            multidata.data[i].val = multiBuf.data[i].val;
        }
        write_count = multiBufIndex;
        multiBufIndex = 0;
    }

    /* unlock */
    adf_os_mutex_release(&wmip->wmi_op_mutex);

    /* Perform multi write */
    if (write_count != 0)
    {
        cmd_status = wmi_cmd(wmi_handle, 
                WMI_REG_WRITE_CMDID, 
                (u_int8_t*)&multidata, 
                sizeof(struct registerWrite)*write_count, 
                (u_int8_t*)&rsp_status,
                sizeof(rsp_status),
                100 /* timeout unit? */);
    
        if(cmd_status){
            //ath_hal_printf(NULL, "Host : WMI COMMAND WRITE FAILURE stat = %x\n", cmd_status);
            return;
        }
    }
}

void __ahdecl
wmi_usb_stop(void *wmi_handle,a_uint8_t usb_status)
{
    struct wmi_t *wmip = (struct wmi_t *)wmi_handle;
    wmip->wmi_usb_stop_flag = usb_status;
    if(usb_status)
        wmi_stop(wmi_handle);
}
