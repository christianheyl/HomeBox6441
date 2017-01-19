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

#include "htc_host_internal.h"

/* The following header included for exporting WMI symbols */
#include <wmi.h>
#include <wmi_host_api.h>

static A_BOOL HTCDrvRegistered = FALSE;
static HTC_host_switch_table_t  HTCHostSwTbl = {NULL,NULL};

#ifdef MAGPIE_HIF_GMAC
os_timer_t host_seek_credit_timer;
#endif

static hif_status_t HTCUsbDeviceRemovedCheck(void *Context)
{
    HTC_TARGET *target = (HTC_TARGET *)Context;

    if(target == NULL) {
        adf_os_print("%s :  #### target null!!\n", __FUNCTION__);
        return HIF_ERROR;
    }

    while (target->htc_rdy_state == HTC_RDY_NONE) {
        /* in the very beginning of init, but target is removed! */
        adf_os_print("%s: target removal happens before htc going to wait state\n",__FUNCTION__);
        ASSERT(0);
    }
    
    if (target->htc_rdy_state == HTC_RDY_WAIT) {
        adf_os_print("%s: cancel waiting for htc ready.\n", __FUNCTION__);
        target->htc_rdy_state = HTC_RDY_FAIL;
        adf_os_mutex_release(&target->htc_rdy_mutex);
    }
#ifdef __linux__        
    else if (target->spin == A_HTC_WAIT) {
        adf_os_print("%s: cancel htc waiting.\n", __FUNCTION__);
        target->spin = A_TGT_DETACH;
    }
#endif

    return HIF_OK;
}

/* registered target arrival callback from the HIF layer */
static hif_status_t HTCTargetInsertedHandler(hif_handle_t hHIF, adf_os_handle_t os_hdl)
{
    HTC_TARGET *target = NULL;
    hif_status_t status = HIF_OK;
    HTC_ENDPOINT *pEndpoint;
    HTC_CALLBACKS htcCallbacks;
    
    do {
        /* Allocate target memory */
        if ((target = (HTC_TARGET *)adf_os_mem_alloc(os_hdl, sizeof(HTC_TARGET))) == NULL) {
            adf_os_print("Unable to allocate memory for HTC instance.\n");
            status = HIF_ERROR ;
            break;
        }

        adf_os_mem_zero(target, sizeof(HTC_TARGET));
        adf_os_spinlock_init(&target->HTCLock);
        adf_os_spinlock_init(&target->HTCRxLock);
        adf_os_spinlock_init(&target->HTCTxLock);

        /* setup HIF layer callbacks */
        adf_os_mem_zero(&htcCallbacks, sizeof(HTC_CALLBACKS));
        htcCallbacks.Context = target;
        htcCallbacks.rxCompletionHandler = HTCRxCompletionHandler;
        htcCallbacks.txCompletionHandler = HTCTxCompletionHandler;
        htcCallbacks.usbStopHandler      = HTCUsbStopHandler;
        htcCallbacks.usbDeviceRemovedHandler = HTCUsbDeviceRemovedCheck;   /* Current, only Linux paltform used. */
#ifdef MAGPIE_SINGLE_CPU_CASE
        htcCallbacks.wlanTxCompletionHandler = HTCWlanTxCompletionHandler;
#endif
        HIFPostInit(hHIF, target, &htcCallbacks);
        
#ifndef __linux__
        adf_os_init_waitq(&target->wq);
#endif
        target->hif_dev = hHIF;

        adf_os_init_mutex(&target->htc_rdy_mutex);
        adf_os_mutex_acquire(&target->htc_rdy_mutex);

        /* Get HIF default pipe for HTC message exchange */
        pEndpoint = &target->EndPoint[ENDPOINT0];
        HIFGetDefaultPipe(hHIF, &pEndpoint->UL_PipeID, &pEndpoint->DL_PipeID);
        adf_os_print("[Default Pipe]UL: %x, DL: %x\n", pEndpoint->UL_PipeID, pEndpoint->DL_PipeID);

    } while (FALSE);

    if (status == HIF_OK) {
        if (HTCHostSwTbl.AddInstance) {
            /* notify upper layer host driver that target is inserted */
            status = HTCHostSwTbl.AddInstance((osdev_t)os_hdl, (HTC_HANDLE)target);
	    }
    }
    else {
         return HIF_ERROR;
    }
	
    return status;
}

/* registered removal callback from the HIF layer */
static hif_status_t HTCTargetRemovedHandler(void *instance)
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(instance);

    while (target->htc_rdy_state == HTC_RDY_NONE) {
        /* in the very beginning of init, but target is removed! */
        adf_os_print("%s: target removal happens before htc going to wait state\n", __FUNCTION__);
        OS_SLEEP(1);
    }
    
    if (target->htc_rdy_state == HTC_RDY_WAIT) {
        adf_os_print("%s: cancel waiting for htc ready.\n", __FUNCTION__);
        target->htc_rdy_state = HTC_RDY_FAIL;
        adf_os_mutex_release(&target->htc_rdy_mutex);
    }
#ifdef __linux__    
    else if (target->spin == A_HTC_WAIT) {
        adf_os_print("%s: cancel htc waiting.\n", __FUNCTION__);
        target->spin = A_TGT_DETACH;
    }
#endif    

    adf_os_spinlock_destroy(&target->HTCLock);
    adf_os_spinlock_destroy(&target->HTCRxLock);
    adf_os_spinlock_destroy(&target->HTCTxLock);

    //HTC_TARGET will be free in DeleteInstance()
    if (HTCHostSwTbl.DeleteInstance) {
        /* notify upper layer host driver that target is removed */
        HTCHostSwTbl.DeleteInstance(target->host_handle);
    }

    return HIF_OK;
}

A_STATUS HTC_host_drv_register(HTC_host_switch_table_t *sw)
{
    HTC_DRVREG_CALLBACKS htcDrvRegCallbacks;

    if (HTCDrvRegistered) {
        return A_OK;
    }

    adf_os_mem_copy(&HTCHostSwTbl, sw, sizeof(HTC_host_switch_table_t));
    adf_os_mem_zero(&htcDrvRegCallbacks, sizeof(HTC_DRVREG_CALLBACKS));

    /* setup HIF layer callbacks */
    htcDrvRegCallbacks.deviceInsertedHandler = HTCTargetInsertedHandler;
    htcDrvRegCallbacks.deviceRemovedHandler = HTCTargetRemovedHandler;

    HIF_register(&htcDrvRegCallbacks);

    HTCDrvRegistered = TRUE;

    return A_OK;

}

/* Initializes the HTC layer */
A_STATUS HTCInit(HTC_HANDLE hHTC, void *host_handle, adf_os_handle_t os_handle, HTC_INIT_INFO *pInitInfo)
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(hHTC);

        adf_os_mem_copy(&(target->HTCInitInfo), pInitInfo, sizeof(HTC_INIT_INFO));
        target->host_handle = host_handle;
        target->os_handle = os_handle;

    return A_OK;
}

A_STATUS HTCSetDefaultPipe(HTC_HANDLE hHTC, a_uint8_t ULPipeID, a_uint8_t DLPipeID)
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(hHTC);
    HTC_ENDPOINT *pEndpoint = &target->EndPoint[ENDPOINT0];

    pEndpoint->UL_PipeID = ULPipeID;
    pEndpoint->DL_PipeID = DLPipeID;

    return A_OK;
}

A_STATUS ConfigPipeCredits(HTC_TARGET *target, a_uint8_t pipeID, a_uint8_t credits)
{
    A_STATUS status = A_OK;
    adf_nbuf_t netbuf;
    adf_os_handle_t os_hdl = target->os_handle;
    HTC_CONFIG_PIPE_MSG *CfgPipeCdt;
#ifndef __linux__
    a_status_t wait_status;
#endif

    do {
        /* allocate a buffer to send */
        //netbuf = adf_nbuf_alloc(anet, sizeof(HTC_CONFIG_PIPE_MSG), HTC_HDR_LENGTH, 0);
        netbuf = adf_nbuf_alloc(os_hdl, 50, HTC_HDR_LENGTH, 0);

        if (netbuf == ADF_NBUF_NULL) {
            status = A_NO_MEMORY;
            break;
        }

        /* assemble config pipe message */
        CfgPipeCdt = (HTC_CONFIG_PIPE_MSG *)adf_nbuf_put_tail(netbuf, sizeof(HTC_CONFIG_PIPE_MSG));
        CfgPipeCdt->MessageID = adf_os_htons(HTC_MSG_CONFIG_PIPE_ID);
        CfgPipeCdt->PipeID = pipeID;
        CfgPipeCdt->CreditCount = credits;

#ifndef MAGPIE_SINGLE_CPU_CASE
#ifdef __linux__
        htc_spin_prep(&target->spin);
#endif
#endif

        /* assemble the HTC header and send to HIF layer */
        status = HTCIssueSend(target, 
                              ADF_NBUF_NULL,
                              netbuf, 
                              0, 
                              sizeof(HTC_CONFIG_PIPE_MSG),
                              ENDPOINT0);
        
		if (A_FAILED(status)) {
			break;	  
		}
        
#ifdef MAGPIE_SINGLE_CPU_CASE
        if (target->cfg_pipe_rsp_stat == HTC_CONFIGPIPE_SUCCESS) {
            status = A_OK;
        }
        else {
            status = A_ERROR;
        }
#else
#ifdef __linux__
        status = htc_spin(&target->spin);
        if (A_FAILED(status)) {
            status = A_ERROR;
            break;
        }
                       
        if (target->cfg_pipe_rsp_stat == HTC_CONFIGPIPE_SUCCESS) {
            status = A_OK;
        }
        else {
            status = A_ERROR;
        }
#else
        wait_status = adf_os_sleep_waitq(&target->wq, 100);
        if (wait_status == A_STATUS_FAILED) {
            /* timeout */
            status = A_ERROR;
        }
        else {
            if (target->cfg_pipe_rsp_stat == HTC_CONFIGPIPE_SUCCESS) {
                status = A_OK;
            }
            else {
                status = A_ERROR;
            }
        }
#endif
#endif
    } while(FALSE);

    return status;
    
}

A_STATUS HTCConfigPipeCredits(HTC_HANDLE hHTC, a_uint8_t pipeID, a_uint8_t credits)
{
    A_STATUS status = A_OK;
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(hHTC);
    status = ConfigPipeCredits(target, pipeID, credits);
    
    return status;
}

#ifndef MAGPIE_HIF_GMAC        
static void
HTCCleanupEp(HTC_TARGET *target, HTC_ENDPOINT *pEndpoint)
{
    a_uint32_t i;
    
    if (pEndpoint->ServiceID == 0) {
        return;
    }

    for (i = 0; i < pEndpoint->TxBufCnt; i++) {

        adf_nbuf_free(pEndpoint->HtcTxQueue[pEndpoint->TxQHead].buf);

        if (pEndpoint->HtcTxQueue[pEndpoint->TxQHead].hdr_bufLen != 0 &&
            pEndpoint->HtcTxQueue[pEndpoint->TxQHead].hdr_buf != NULL) {
            adf_nbuf_free(pEndpoint->HtcTxQueue[pEndpoint->TxQHead].hdr_buf);
        }

        pEndpoint->TxQHead = (pEndpoint->TxQHead + 1) % HTC_TX_QUEUE_SIZE;
    }
}

void
HTCDrainEp(HTC_HANDLE hHTC, HTC_ENDPOINT_ID Ep)
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(hHTC);
    HTC_ENDPOINT *pEndpoint = &target->EndPoint[Ep];

    LOCK_HTC_TX(target);

    HTCCleanupEp(target, pEndpoint);
    
    UNLOCK_HTC_TX(target);
}

void
HTCDrainAllEp(HTC_HANDLE hHTC)
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(hHTC);
    HTC_ENDPOINT_ID ep;
    HTC_ENDPOINT *pEndpoint;

    LOCK_HTC_TX(target);

    for (ep = ENDPOINT0; ep < ENDPOINT_MAX; ep++) {
        pEndpoint = &target->EndPoint[ep];
        HTCCleanupEp(target, pEndpoint);
    }
    
    UNLOCK_HTC_TX(target);
}
#endif
A_STATUS HTCWaitForHtcRdy(HTC_HANDLE HTCHandle)
{
    A_STATUS status = A_OK;
    a_status_t wait_status;
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);

    HIFStart(target->hif_dev);
    adf_os_print("[%s %d] Starting mutex waiting...\n", __FUNCTION__, __LINE__);
    LOCK_HTC(target);
    if (target->htc_rdy_state == HTC_RDY_NONE)
        target->htc_rdy_state = HTC_RDY_WAIT;
    UNLOCK_HTC(target);
    
    wait_status = adf_os_mutex_acquire(&target->htc_rdy_mutex);
    if ((wait_status == A_STATUS_FAILED) || 
        (target->htc_rdy_state == HTC_RDY_FAIL)) {
        /* wait mutex timeout or device removed in early stage. */
        adf_os_print("[%s %d] timeout, htc_rdy_state %d!\n", __FUNCTION__, __LINE__, target->htc_rdy_state);
        status = A_ERROR;
    }

    adf_os_print("[%s %d] Finish mutex waiting...\n", __FUNCTION__, __LINE__);

    return status;
}

#ifdef MAGPIE_HIF_GMAC
int
htc_get_creditinfo(HTC_HANDLE hHTC, HTC_ENDPOINT_ID ep) 
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(hHTC);
    HTC_ENDPOINT *pEndpoint;
    
    pEndpoint = &target->EndPoint[ep];
    return pEndpoint->CreditDist.TxCredits;
}
#endif

OS_TIMER_FUNC  (host_htc_credit_show)
{
    HTC_TARGET *target ;

    HTC_ENDPOINT_CREDIT_DIST *pCurEpDist;
   

    a_uint32_t totalCredit = 0;

    OS_GET_TIMER_ARG(target, HTC_TARGET * );

    pCurEpDist = target->EpCreditDistributionListHead;



    while (pCurEpDist != NULL) {
        adf_os_print("ep %u: %u credits\n", pCurEpDist->Endpoint, pCurEpDist->TxCredits);
        totalCredit += pCurEpDist->TxCredits;
        pCurEpDist = pCurEpDist->pNext;
    }
    adf_os_print("host total credits: %u\n", totalCredit);

    HTC_CREDIT_SHOW_TIMER_START(&target->host_htc_credit_debug_timer) ;
}


A_STATUS HTCStart(HTC_HANDLE HTCHandle)
{
    adf_nbuf_t netbuf;
    A_STATUS   status = A_OK;
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    adf_os_handle_t os_hdl = target->os_handle;
    HTC_SETUP_COMPLETE_MSG *SetupComp;
#ifdef __linux__
#ifndef MAGPIE_HIF_GMAC   
	a_uint32_t credits_hp;
    //a_uint32_t credits_mp;
    a_uint32_t credits_lp;
#endif
#endif

    do {
#ifdef MAGPIE_SINGLE_CPU_CASE
        ConfigPipeCredits(target, 1, 5 /* credits */);
#elif defined(MAGPIE_HIF_USB)
#ifdef __linux__
        credits_hp = 2;
        //credits_mp = 2;
        credits_lp = target->TargetCredits - credits_hp;
        status = ConfigPipeCredits(target, 1, (a_uint8_t)credits_lp); /* Pipe 1 = LP pipe */
        if (A_FAILED(status)) {
            status = A_ERROR;  
            break;
        }
        status = ConfigPipeCredits(target, 5, (a_uint8_t)credits_hp); /* Pipe 5 = HP pipe */
        if (A_FAILED(status)) {
            status = A_ERROR;  
            break;
        }
        //ConfigPipeCredits(target, 6, (a_uint8_t)credits_mp); /* Pipe 6 = MP pipe */
#else
        ConfigPipeCredits(target, 1, target->TargetCredits/* credits */);
#endif
#elif defined(MAGPIE_HIF_PCI)
        credits_hp = 2;
        //credits_mp = 2;
        credits_lp = target->TargetCredits - credits_hp;

        ConfigPipeCredits(target, 1, credits_lp); /* Pipe 1 = LP pipe */
        ConfigPipeCredits(target, 3, credits_hp); /* Pipe 5 = HP pipe */
#endif
        HTC_CRDIT_DIST_INIT(target) ;
        /* allocate a buffer to send */
        //netbuf = adf_nbuf_alloc(anet, sizeof(HTC_SETUP_COMPLETE_MSG), HTC_HDR_LENGTH, 0);
        netbuf = adf_nbuf_alloc(os_hdl, 50, HTC_HDR_LENGTH, 0);

        if (netbuf == ADF_NBUF_NULL) {
            status = A_NO_MEMORY;
            break;
        }

        /* assemble setup complete message */
        SetupComp = (HTC_SETUP_COMPLETE_MSG *)adf_nbuf_put_tail(netbuf, sizeof(HTC_SETUP_COMPLETE_MSG));
        SetupComp->MessageID = adf_os_htons(HTC_MSG_SETUP_COMPLETE_ID);

        /* assemble the HTC header and send to HIF layer */
        status = HTCIssueSend(target, 
                              ADF_NBUF_NULL, 
                              netbuf, 
                              0, 
                              sizeof(HTC_SETUP_COMPLETE_MSG),
                              ENDPOINT0);
        
		if (A_FAILED(status)) {
			break;	  
		}

    } while (FALSE);

    return status;
}

/* undo what was done in HTCInit() */
void HTCShutDown(HTC_HANDLE HTCHandle)
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    //HTC_CALLBACKS htcCallbacks;

    /* shutdown HIF layer */
    //HIFShutDown(target->hif_dev);

    HTC_CREDIT_SHOW_TIMER_CANCEL(&target->host_htc_credit_debug_timer);

    /* need to handle packets queued in the HTC */
#ifndef MAGPIE_HIF_GMAC        
    HTCDrainAllEp(target);
#endif    
#if 0
    HTCCleanup(target);
#endif

    /* release htc_rdy_mutex */
    adf_os_mutex_release(&target->htc_rdy_mutex);

    /* free our instance */
    adf_os_mem_free(target);
}
HIF_HANDLE HTCGetHIFHandle(HTC_HANDLE HTCHandle)
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);

    return target->hif_dev;
}

a_uint32_t HTCQueryQueueDepth(HTC_HANDLE HTCHandle, HTC_ENDPOINT_ID Ep)
{
#ifdef MAGPIE_SINGLE_CPU_CASE
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);

    return HIFQueryQueueDepth(target->hif_dev, (a_uint8_t)Ep);
#else
    //adf_os_print("[%s %d] not yet implemented for split cpu case!\n", __FUNCTION__, __LINE__);
    return 0;
#endif
}

int init_htc_host(void);
int 
init_htc_host(void)
{
	adf_os_print("HTC HOST Version 1.xx Loaded...\n");
        return 0;
}

void exit_htc_host(void);
void 
exit_htc_host(void)
{

	adf_os_print("HTC HOST UnLoaded...\n");

}

/* HTC related symbols */
adf_os_export_symbol(HTC_host_drv_register);
adf_os_export_symbol(HTCInit);
adf_os_export_symbol(HTCStart);
adf_os_export_symbol(HTCConnectService);
adf_os_export_symbol(HTCShutDown);
adf_os_export_symbol(HTCConfigPipeCredits);
adf_os_export_symbol(HTCSetDefaultPipe);
adf_os_export_symbol(HTCSendPkt);
adf_os_export_symbol(HTCTxComp);
adf_os_export_symbol(HTCTxEpUpdate);
adf_os_export_symbol(HTCGetHIFHandle); 
adf_os_export_symbol(HTCSetCreditDistribution);
adf_os_export_symbol(HTC_busy);
adf_os_export_symbol(HTCQueryQueueDepth);
#ifndef MAGPIE_HIF_GMAC        
adf_os_export_symbol(HTCDrainEp);
adf_os_export_symbol(HTCDrainAllEp);
#endif
adf_os_export_symbol(HTCWaitForHtcRdy);
#ifdef HTC_HOST_CREDIT_DIST
adf_os_export_symbol(HTCSetupCreditDist);
#endif

/* WMI related symbols */
adf_os_export_symbol(wmi_init);
adf_os_export_symbol(wmi_shutdown);
adf_os_export_symbol(wmi_connect);
adf_os_export_symbol(wmi_cmd);
adf_os_export_symbol(wmi_start);
adf_os_export_symbol(wmi_stop);
adf_os_export_symbol(wmi_reg_write_flush);
adf_os_export_symbol(wmi_reg_write_delay);
adf_os_export_symbol(wmi_reg_rmw);
adf_os_export_symbol(wmi_reg_write_single);
#ifdef MAGPIE_HIF_GMAC
adf_os_export_symbol(htc_get_creditinfo);
adf_os_export_symbol(host_seek_credit_timer);
#endif

adf_os_virt_module_init(init_htc_host);
adf_os_virt_module_exit(exit_htc_host);
//adf_os_module_dep(htc_host, adf_net);
//adf_os_module_dep(htc_host, inproc_hif);
//adf_os_virt_module_name(htc_host);

#ifndef ATH_WINHTC
MODULE_DESCRIPTION("Host HTC module");
MODULE_LICENSE("Proprietary");
#endif

