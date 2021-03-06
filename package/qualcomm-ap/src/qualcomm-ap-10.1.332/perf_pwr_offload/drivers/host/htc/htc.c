//------------------------------------------------------------------------------
// <copyright file="htc.c" company="Atheros">
//    Copyright (c) 2007-2010 Atheros Corporation.  All rights reserved.
// $ATH_LICENSE_HOSTSDK0_C$
//------------------------------------------------------------------------------
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================
#include "htc_internal.h"
#include <adf_nbuf.h>     /* adf_nbuf_t */
#include <adf_os_types.h> /* adf_os_print */
#include <epping_test.h>

#ifdef DEBUG
static ATH_DEBUG_MASK_DESCRIPTION g_HTCDebugDescription[] = {
    { ATH_DEBUG_SEND , "Send"},
    { ATH_DEBUG_RECV , "Recv"},
    { ATH_DEBUG_SYNC , "Sync"},
    { ATH_DEBUG_DUMP , "Dump Data (RX or TX)"},
    { ATH_DEBUG_SETUP, "Setup"},
};


ATH_DEBUG_INSTANTIATE_MODULE_VAR(htc,
                                 "htc",
                                 "Host Target Communications",
                                 ATH_DEBUG_MASK_DEFAULTS | ATH_DEBUG_INFO | ATH_DEBUG_SETUP,
                                 ATH_DEBUG_DESCRIPTION_COUNT(g_HTCDebugDescription),
                                 g_HTCDebugDescription);

#endif

extern unsigned int htc_credit_flow;

static void ResetEndpointStates(HTC_TARGET *target);

static void DestroyHTCTxCtrlPacket(HTC_PACKET *pPacket)
{
    adf_nbuf_t netbuf;
    netbuf = (adf_nbuf_t)GET_HTC_PACKET_NET_BUF_CONTEXT(pPacket);
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("free ctrl netbuf :0x%p \n", netbuf));
    if (netbuf != NULL) {
        adf_nbuf_free(netbuf);
    }

    A_FREE(pPacket);
}


static HTC_PACKET *BuildHTCTxCtrlPacket(adf_os_device_t osdev)
{
    HTC_PACKET *pPacket = NULL;
    adf_nbuf_t netbuf;

    do {
        pPacket = (HTC_PACKET *)A_MALLOC(sizeof(HTC_PACKET));
        if (NULL == pPacket) {
            break;
        }
        A_MEMZERO(pPacket,sizeof(HTC_PACKET));
		netbuf = adf_nbuf_alloc(osdev, HTC_CONTROL_BUFFER_SIZE, 20, 4, TRUE);
        if (NULL == netbuf) {
            A_FREE(pPacket);
            pPacket = NULL;
	        adf_os_print("%s: nbuf alloc failed\n",__func__);
            break;
        }
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("alloc ctrl netbuf :0x%p \n", netbuf));
        SET_HTC_PACKET_NET_BUF_CONTEXT(pPacket, netbuf);
    } while (FALSE);

    return pPacket;
}

void HTCFreeControlTxPacket(HTC_TARGET *target, HTC_PACKET *pPacket)
{

#ifdef TODO_FIXME
    LOCK_HTC(target);
    HTC_PACKET_ENQUEUE(&target->ControlBufferTXFreeList,pPacket);
    UNLOCK_HTC(target);
    /* TODO_FIXME netbufs cannot be RESET! */
#else
    DestroyHTCTxCtrlPacket(pPacket);
#endif

}

HTC_PACKET *HTCAllocControlTxPacket(HTC_TARGET *target)
{
#ifdef TODO_FIXME
    HTC_PACKET *pPacket;

    LOCK_HTC(target);
    pPacket = HTC_PACKET_DEQUEUE(&target->ControlBufferTXFreeList);
    UNLOCK_HTC(target);

    return pPacket;
#else
    return BuildHTCTxCtrlPacket(target->osdev);
#endif
}


/* cleanup the HTC instance */
static void HTCCleanup(HTC_TARGET *target)
{
    HTC_PACKET *pPacket;
    //adf_nbuf_t netbuf;

    if (target->hif_dev != NULL) {
        HIFDetachHTC(target->hif_dev);
        target->hif_dev = NULL;
    }

    while (TRUE) {
        pPacket = AllocateHTCPacketContainer(target);
        if (NULL == pPacket) {
            break;
        }
        A_FREE(pPacket);
    }

#ifdef TODO_FIXME
    while (TRUE) {
        pPacket = HTCAllocControlTxPacket(target);
        if (NULL == pPacket) {
            break;
        }
        netbuf = (adf_nbuf_t)GET_HTC_PACKET_NET_BUF_CONTEXT(pPacket);
        if (netbuf != NULL) {
            adf_nbuf_free(netbuf);
        }

        A_FREE(pPacket);
    }
#endif

    adf_os_spinlock_destroy(&target->HTCLock);
    adf_os_spinlock_destroy(&target->HTCRxLock);
    adf_os_spinlock_destroy(&target->HTCTxLock);

    /* free our instance */
    A_FREE(target);
}

/* registered target arrival callback from the HIF layer */
HTC_HANDLE HTCCreate(void *hHIF, HTC_INIT_INFO *pInfo, adf_os_device_t osdev)
{
    MSG_BASED_HIF_CALLBACKS htcCallbacks;
    HTC_ENDPOINT            *pEndpoint=NULL;
    HTC_TARGET              *target = NULL;
    int                     i;

    AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("+HTCCreate ..  HIF :%p \n",hHIF));

    A_REGISTER_MODULE_DEBUG_INFO(htc);

    if ((target = (HTC_TARGET *)A_MALLOC(sizeof(HTC_TARGET))) == NULL) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("HTC : Unable to allocate memory\n"));
        return NULL;
    }

    A_MEMZERO(target, sizeof(HTC_TARGET));

    adf_os_spinlock_init(&target->HTCLock);
    adf_os_spinlock_init(&target->HTCRxLock);
    adf_os_spinlock_init(&target->HTCTxLock);

    do {
        A_MEMCPY(&target->HTCInitInfo,pInfo,sizeof(HTC_INIT_INFO));
        target->host_handle = pInfo->pContext;
		target->osdev = osdev;

        ResetEndpointStates(target);

        INIT_HTC_PACKET_QUEUE(&target->ControlBufferTXFreeList);

        for (i = 0; i < HTC_PACKET_CONTAINER_ALLOCATION; i++) {
            HTC_PACKET *pPacket = (HTC_PACKET *)A_MALLOC(sizeof(HTC_PACKET));
            if (pPacket != NULL) {
                A_MEMZERO(pPacket,sizeof(HTC_PACKET));
                FreeHTCPacketContainer(target,pPacket);
            }
        }

#ifdef TODO_FIXME
        for (i = 0; i < NUM_CONTROL_TX_BUFFERS; i++) {
            pPacket = BuildHTCTxCtrlPacket();
            if (NULL == pPacket) {
                break;
            }
            HTCFreeControlTxPacket(target,pPacket);
        }
#endif

        /* setup HIF layer callbacks */
        A_MEMZERO(&htcCallbacks, sizeof(MSG_BASED_HIF_CALLBACKS));
        htcCallbacks.Context = target;
        htcCallbacks.rxCompletionHandler = HTCRxCompletionHandler;
        htcCallbacks.txCompletionHandler = HTCTxCompletionHandler;
        htcCallbacks.txResourceAvailHandler = HTCTxResourceAvailHandler;
        htcCallbacks.fwEventHandler = HTCFwEventHandler;
        target->hif_dev = hHIF;

        /* Get HIF default pipe for HTC message exchange */
        pEndpoint = &target->EndPoint[ENDPOINT_0];

        HIFPostInit(hHIF, target, &htcCallbacks);
        HIFGetDefaultPipe(hHIF, &pEndpoint->UL_PipeID, &pEndpoint->DL_PipeID);

    } while (FALSE);

    HTCRecvInit(target);

    AR_DEBUG_PRINTF(ATH_DEBUG_INFO, ("-HTCCreate (0x%p) \n", target));

    return (HTC_HANDLE)target;
}

void  HTCDestroy(HTC_HANDLE HTCHandle)
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("+HTCDestroy ..  Destroying :0x%p \n",target));
    HTCCleanup(target);
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("-HTCDestroy \n"));
}

/* get the low level HIF device for the caller , the caller may wish to do low level
 * HIF requests */
void *HTCGetHifDevice(HTC_HANDLE HTCHandle)
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    return target->hif_dev;
}

void HTCControlTxComplete(void *Context, HTC_PACKET *pPacket)
{
    HTC_TARGET *target = (HTC_TARGET *)Context;
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("+-HTCControlTxComplete 0x%p (l:%d) \n",pPacket,pPacket->ActualLength));
    HTCFreeControlTxPacket(target,pPacket);
}

/* TODO, this is just a temporary max packet size */
#define MAX_MESSAGE_SIZE 1536

A_STATUS HTCSetupTargetBufferAssignments(HTC_TARGET *target)
{
    HTC_SERVICE_TX_CREDIT_ALLOCATION    *pEntry;
    A_STATUS                            status;
    int                                 credits;
    int                                 creditsPerMaxMsg;
#ifdef HIF_USB
    unsigned int                        hif_usbaudioclass=0;
#else
    unsigned int                        hif_usbaudioclass=0;
#endif

    creditsPerMaxMsg = MAX_MESSAGE_SIZE / target->TargetCreditSize;
    if (MAX_MESSAGE_SIZE % target->TargetCreditSize) {
        creditsPerMaxMsg++;
    }

    /* TODO, this should be configured by the caller! */

    credits = target->TotalTransmitCredits;
    pEntry = &target->ServiceTxAllocTable[0];
#if defined(HIF_USB)
    target->avail_tx_credits = target->TotalTransmitCredits - 1 ;
#endif
    status = A_NO_RESOURCE;
#if defined(HIF_PCI) || defined(HIF_SIM) || (defined(CONFIG_HL_SUPPORT) && !defined(AR6004_HW))/* TODO: maybe change to HIF_SDIO later */
                                                                           /* TODO: maybe check ROME service for USB service */
    /*
     * for PCIE allocate all credists/HTC buffers to WMI.
     * no buffers are used/required for data. data always
     * remains on host.
     */ 
    hif_usbaudioclass = 0; /* to keep compiler happy */
    /* for PCIE allocate all credits to wmi for now */
    status = A_OK;
    pEntry++;
    pEntry->ServiceID = WMI_CONTROL_SVC;
    pEntry->CreditAllocation = credits;

#ifdef EPPING_TEST
    pEntry++;
    pEntry->ServiceID = WMI_DATA_BE_SVC;
    pEntry->CreditAllocation = credits >> 1;
    pEntry++;
    pEntry->ServiceID = WMI_DATA_BK_SVC;
    pEntry->CreditAllocation = credits >> 1;
#endif
#else
    do {

        if(hif_usbaudioclass)
        {
          AR_DEBUG_PRINTF(ATH_DEBUG_ANY,("HTCSetupTargetBufferAssignments For USB Audio Class- Total:%d\n",credits));
          pEntry++;
          pEntry++;
          //Setup VO Service To have Max Credits
          pEntry->ServiceID = WMI_DATA_VO_SVC;
          pEntry->CreditAllocation = (credits -6);
          if (pEntry->CreditAllocation == 0) {
            pEntry->CreditAllocation++;
            }
          credits -= (int)pEntry->CreditAllocation;
          if (credits <= 0) {
                   break;
            }
          pEntry++;
          pEntry->ServiceID = WMI_CONTROL_SVC;
          pEntry->CreditAllocation = creditsPerMaxMsg;
          credits -= (int)pEntry->CreditAllocation;
          if (credits <= 0) {
             break;
            }
            /*
             * HTT_DATA_MSG_SVG is unidirectional from target -> host,
             * so no target buffers are needed.
             */
          /* leftovers go to best effort */
          pEntry++;
          pEntry++;
          pEntry->ServiceID = WMI_DATA_BE_SVC;
          pEntry->CreditAllocation = (A_UINT8)credits;
          status = A_OK;
          break;
        }

        pEntry++;
        pEntry->ServiceID = WMI_DATA_VI_SVC;
        pEntry->CreditAllocation = credits / 4;
        if (pEntry->CreditAllocation == 0) {
            pEntry->CreditAllocation++;
        }
        credits -= (int)pEntry->CreditAllocation;
        if (credits <= 0) {
            break;
        }
        pEntry++;
        pEntry->ServiceID = WMI_DATA_VO_SVC;
        pEntry->CreditAllocation = credits / 4;
        if (pEntry->CreditAllocation == 0) {
            pEntry->CreditAllocation++;
        }
        credits -= (int)pEntry->CreditAllocation;
        if (credits <= 0) {
            break;
        }
        pEntry++;
        pEntry->ServiceID = WMI_CONTROL_SVC;
        pEntry->CreditAllocation = creditsPerMaxMsg;
        credits -= (int)pEntry->CreditAllocation;
        if (credits <= 0) {
            break;
        }
        /*
         * HTT_DATA_MSG_SVG is unidirectional from target -> host,
         * so no target buffers are needed.
         */
        pEntry++;
        pEntry->ServiceID = WMI_DATA_BK_SVC;
        pEntry->CreditAllocation = creditsPerMaxMsg;
        credits -= (int)pEntry->CreditAllocation;
        if (credits <= 0) {
            break;
        }
        /* leftovers go to best effort */
        pEntry++;
        pEntry->ServiceID = WMI_DATA_BE_SVC;
        pEntry->CreditAllocation = (A_UINT8)credits;
        status = A_OK;

    } while (FALSE);

#endif

    if (A_SUCCESS(status)) {
        int i;
        for (i = 0; i < HTC_MAX_SERVICE_ALLOC_ENTRIES; i++) {
            if (target->ServiceTxAllocTable[i].ServiceID != 0) {
                AR_DEBUG_PRINTF(ATH_DEBUG_INIT,("HTC Service Index : %d TX : 0x%2.2X : alloc:%d \n",
                        i,
                        target->ServiceTxAllocTable[i].ServiceID,
                        target->ServiceTxAllocTable[i].CreditAllocation));
            }
        }
    }

    return status;
}

A_UINT8 HTCGetCreditAllocation(HTC_TARGET *target, A_UINT16 ServiceID)
{
    A_UINT8 allocation = 0;
    int     i;

    for (i = 0; i < HTC_MAX_SERVICE_ALLOC_ENTRIES; i++) {
        if (target->ServiceTxAllocTable[i].ServiceID == ServiceID) {
            allocation = target->ServiceTxAllocTable[i].CreditAllocation;
        }
    }

    if (0 == allocation) {
        AR_DEBUG_PRINTF(ATH_DEBUG_INIT,("HTC Service TX : 0x%2.2X : allocation is zero! \n",ServiceID));
    }

    return allocation;
}


A_STATUS HTCWaitTarget(HTC_HANDLE HTCHandle)
{
    A_STATUS                status = A_OK;
    HTC_TARGET              *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    HTC_READY_EX_MSG        *pReadyMsg;
    HTC_SERVICE_CONNECT_REQ  connect;
    HTC_SERVICE_CONNECT_RESP resp;
    HTC_READY_MSG *rdy_msg;
    A_UINT16 htc_rdy_msg_id;

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("HTCWaitTarget - Enter (target:0x%p) \n", HTCHandle));
    AR_DEBUG_PRINTF(ATH_DEBUG_ANY, ("+HWT\n"));

    do {

        HIFStart(target->hif_dev);

        status = HTCWaitRecvCtrlMessage(target);

        if (A_FAILED(status)) {
            break;
        }

        if (target->CtrlResponseLength < (sizeof(HTC_READY_EX_MSG))) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("Invalid HTC Ready Msg Len:%d! \n",target->CtrlResponseLength));
            status = A_ECOMM;
            break;
        }

        pReadyMsg = (HTC_READY_EX_MSG *)target->CtrlResponseBuffer;

        rdy_msg = &pReadyMsg->Version2_0_Info;
        htc_rdy_msg_id = HTC_GET_FIELD(rdy_msg, HTC_READY_MSG, MESSAGEID);
        if (htc_rdy_msg_id != HTC_MSG_READY_ID) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
                        ("Invalid HTC Ready Msg : 0x%X ! \n",htc_rdy_msg_id));
            status = A_ECOMM;
            break;
        }

        target->TotalTransmitCredits = HTC_GET_FIELD(rdy_msg, HTC_READY_MSG, CREDITCOUNT); 
        target->TargetCreditSize = (int)HTC_GET_FIELD(rdy_msg, HTC_READY_MSG, CREDITSIZE);

        AR_DEBUG_PRINTF(ATH_DEBUG_INIT, ("Target Ready! : transmit resources : %d size:%d\n",
                target->TotalTransmitCredits, target->TargetCreditSize));

        if ((0 == target->TotalTransmitCredits) || (0 == target->TargetCreditSize)) {
            status = A_ECOMM;
            break;
        }
            /* done processing */
        target->CtrlResponseProcessing = FALSE;

        HTCSetupTargetBufferAssignments(target);

            /* setup our pseudo HTC control endpoint connection */
        A_MEMZERO(&connect,sizeof(connect));
        A_MEMZERO(&resp,sizeof(resp));
        connect.EpCallbacks.pContext = target;
        connect.EpCallbacks.EpTxComplete = HTCControlTxComplete;
        connect.EpCallbacks.EpRecv = HTCControlRxComplete;
        connect.MaxSendQueueDepth = NUM_CONTROL_TX_BUFFERS;
        connect.ServiceID = HTC_CTRL_RSVD_SVC;

            /* connect fake service */
        status = HTCConnectService((HTC_HANDLE)target,
                                   &connect,
                                   &resp);

    } while (FALSE);

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("HTCWaitTarget - Exit (%d)\n",status));
    AR_DEBUG_PRINTF(ATH_DEBUG_ANY, ("-HWT\n"));
    return status;
}

/* start HTC, this is called after all services are connected */
static A_STATUS HTCConfigTargetHIFPipe(HTC_TARGET *target)
{

    return A_OK;
}

static void ResetEndpointStates(HTC_TARGET *target)
{
    HTC_ENDPOINT        *pEndpoint;
    int                  i;

    for (i = ENDPOINT_0; i < ENDPOINT_MAX; i++) {
        pEndpoint = &target->EndPoint[i];
        pEndpoint->ServiceID = 0;
        pEndpoint->MaxMsgLength = 0;
        pEndpoint->MaxTxQueueDepth = 0;
        pEndpoint->Id = i;
        INIT_HTC_PACKET_QUEUE(&pEndpoint->TxQueue);
        INIT_HTC_PACKET_QUEUE(&pEndpoint->TxLookupQueue);
        INIT_HTC_PACKET_QUEUE(&pEndpoint->RxBufferHoldQueue);
        pEndpoint->target = target;
        //pEndpoint->TxCreditFlowEnabled = (A_BOOL)htc_credit_flow;
        pEndpoint->TxCreditFlowEnabled = (A_BOOL)1;
        adf_os_atomic_init(&pEndpoint->TxProcessCount);
    }
}

A_STATUS HTCStart(HTC_HANDLE HTCHandle)
{
    adf_nbuf_t                 netbuf;
    A_STATUS                   status = A_OK;
    HTC_TARGET                 *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    HTC_SETUP_COMPLETE_EX_MSG  *pSetupComp;
    HTC_PACKET                 *pSendPacket;

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("HTCStart Enter\n"));

    do {

        HTCConfigTargetHIFPipe(target);

            /* allocate a buffer to send */
        pSendPacket = HTCAllocControlTxPacket(target);
        if (NULL == pSendPacket) {
            AR_DEBUG_ASSERT(FALSE);
	        adf_os_print("%s: allocControlTxPacket failed\n",__func__);
            status = A_NO_MEMORY;
            break;
        }

        netbuf = (adf_nbuf_t)GET_HTC_PACKET_NET_BUF_CONTEXT(pSendPacket);
            /* assemble setup complete message */
        adf_nbuf_put_tail(netbuf, sizeof(HTC_SETUP_COMPLETE_EX_MSG));
        pSetupComp = (HTC_SETUP_COMPLETE_EX_MSG *) adf_nbuf_data(netbuf);
        A_MEMZERO(pSetupComp,sizeof(HTC_SETUP_COMPLETE_EX_MSG));

        HTC_SET_FIELD(pSetupComp, HTC_SETUP_COMPLETE_EX_MSG,
                    MESSAGEID, HTC_MSG_SETUP_COMPLETE_EX_ID);

        //if (!htc_credit_flow) {
        if (0) {
            AR_DEBUG_PRINTF(ATH_DEBUG_INIT, ("HTC will not use TX credit flow control\n"));
            pSetupComp->SetupFlags |= HTC_SETUP_COMPLETE_FLAGS_DISABLE_TX_CREDIT_FLOW;
        } else {
            AR_DEBUG_PRINTF(ATH_DEBUG_INIT, ("HTC using TX credit flow control\n"));
        }

        SET_HTC_PACKET_INFO_TX(pSendPacket,
                               NULL,
                               (A_UINT8 *)pSetupComp,
                               sizeof(HTC_SETUP_COMPLETE_EX_MSG),
                               ENDPOINT_0,
                               HTC_SERVICE_TX_PACKET_TAG);

        status = HTCSendPkt((HTC_HANDLE)target,pSendPacket);
        if (A_FAILED(status)) {
            break;
        }

    } while (FALSE);

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("HTCStart Exit\n"));
    return status;
}


/* stop HTC communications, i.e. stop interrupt reception, and flush all queued buffers */
void HTCStop(HTC_HANDLE HTCHandle)
{
    HTC_TARGET    *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    int           i;
    HTC_ENDPOINT  *pEndpoint;
#ifdef RX_SG_SUPPORT
    adf_nbuf_t netbuf;
    adf_nbuf_queue_t *rx_sg_queue = &target->RxSgQueue;
#endif

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("+HTCStop \n"));

        /* cleanup endpoints */
    for (i = 0; i < ENDPOINT_MAX; i++) {
        pEndpoint = &target->EndPoint[i];
        HTCFlushRxHoldQueue(target,pEndpoint);
        HTCFlushEndpointTX(target,pEndpoint,HTC_TX_PACKET_TAG_ALL);
    }

    /* Note: HTCFlushEndpointTX for all endpoints should be called before
     * HIFStop - otherwise HTCTxCompletionHandler called from 
     * hif_send_buffer_cleanup_on_pipe for residual tx frames in HIF layer,
     * might queue the packet again to HIF Layer - which could cause tx
     * buffer leak
     */ 

    HIFStop(target->hif_dev);

#ifdef RX_SG_SUPPORT
    LOCK_HTC_RX(target);
    while ((netbuf = adf_nbuf_queue_remove(rx_sg_queue)) != NULL) {
        adf_nbuf_free(netbuf);
    }
    RESET_RX_SG_CONFIG(target);
    UNLOCK_HTC_RX(target);
#endif

    ResetEndpointStates(target);

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("-HTCStop \n"));
}



void HTCDumpCreditStates(HTC_HANDLE HTCHandle)
{
    HTC_TARGET    *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    HTC_ENDPOINT  *pEndpoint;
    int            i;

    for (i = 0; i < ENDPOINT_MAX; i++) {
        pEndpoint = &target->EndPoint[i];
        if (0 == pEndpoint->ServiceID) {
            continue;
        }
        AR_DEBUG_PRINTF(ATH_DEBUG_ANY, ("--- EP : %d  ServiceID: 0x%X    --------------\n",
                        pEndpoint->Id, pEndpoint->ServiceID));
        AR_DEBUG_PRINTF(ATH_DEBUG_ANY, (" TxCredits          : %d \n", pEndpoint->TxCredits));
        AR_DEBUG_PRINTF(ATH_DEBUG_ANY, (" TxCreditSize       : %d \n", pEndpoint->TxCreditSize));
        AR_DEBUG_PRINTF(ATH_DEBUG_ANY, (" TxCreditsPerMaxMsg : %d \n", pEndpoint->TxCreditsPerMaxMsg));
        AR_DEBUG_PRINTF(ATH_DEBUG_ANY, (" TxQueueDepth       : %d \n", HTC_PACKET_QUEUE_DEPTH(&pEndpoint->TxQueue)));
        AR_DEBUG_PRINTF(ATH_DEBUG_ANY, ("----------------------------------------------------\n"));
    }
}


A_BOOL HTCGetEndpointStatistics(HTC_HANDLE               HTCHandle,
                                HTC_ENDPOINT_ID          Endpoint,
                                HTC_ENDPOINT_STAT_ACTION Action,
                                HTC_ENDPOINT_STATS       *pStats)
{
#ifdef HTC_EP_STAT_PROFILING
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    A_BOOL     clearStats = FALSE;
    A_BOOL     sample = FALSE;

    switch (Action) {
        case HTC_EP_STAT_SAMPLE :
            sample = TRUE;
            break;
        case HTC_EP_STAT_SAMPLE_AND_CLEAR :
            sample = TRUE;
            clearStats = TRUE;
            break;
        case HTC_EP_STAT_CLEAR :
            clearStats = TRUE;
            break;
        default:
            break;
    }

    A_ASSERT(Endpoint < ENDPOINT_MAX);

        /* lock out TX and RX while we sample and/or clear */
    LOCK_HTC_TX(target);
    LOCK_HTC_RX(target);

    if (sample) {
        A_ASSERT(pStats != NULL);
            /* return the stats to the caller */
        A_MEMCPY(pStats, &target->EndPoint[Endpoint].EndPointStats, sizeof(HTC_ENDPOINT_STATS));
    }

    if (clearStats) {
            /* reset stats */
        A_MEMZERO(&target->EndPoint[Endpoint].EndPointStats, sizeof(HTC_ENDPOINT_STATS));
    }

    UNLOCK_HTC_RX(target);
    UNLOCK_HTC_TX(target);

    return TRUE;
#else
    return FALSE;
#endif
}

