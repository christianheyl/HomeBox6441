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

static void HTCDefaultCreditInit(void                     *Context,
                                 HTC_ENDPOINT_CREDIT_DIST *pEPList, 
                                 a_uint32_t                 TotalCredits);

static void HTCDefaultCreditDist(void                     *Context, 
                          HTC_ENDPOINT_CREDIT_DIST *pEPDistList,
                          HTC_CREDIT_DIST_REASON   Reason);

A_STATUS HTCConnectService(HTC_HANDLE HTCHandle, HTC_SERVICE_CONNECT_REQ *pConnectReq, HTC_ENDPOINT_ID *pConn_rsp_epid)
{
    adf_nbuf_t netbuf;
    A_STATUS status = A_OK;
    //a_uint32_t maxMsgSize = 0;
    HTC_ENDPOINT *pEndpoint = NULL;
    HTC_ENDPOINT_ID assignedEndpoint;
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    adf_os_handle_t os_hdl = target->os_handle;
    HTC_CONNECT_SERVICE_MSG *ConnSvc;
#ifndef __linux__
    a_status_t wait_status;
#endif

    do {
        /* save the pipe id for non-endpoint 0 */
        #if 0
        target->ULPipeIdMapping[assignedEndpoint] = pConnectReq->UL_PipeID;
        target->DLPipeIdMapping[assignedEndpoint] = pConnectReq->DL_PipeID;
        #endif
        
        
        LOCK_HTC(target);
        /* find an available endpoint structure to temporarily save user configurations */
        for (assignedEndpoint = (ENDPOINT_MAX - 1); assignedEndpoint > ENDPOINT0; assignedEndpoint--) {
            pEndpoint = &target->EndPoint[assignedEndpoint];
            if (pEndpoint->ServiceID == 0) {
                break;
            }
        }

        //DbgPrint("assignedEndpoint: %u", assignedEndpoint);
        
        if (pEndpoint == NULL) {
            status = A_ERROR;
            UNLOCK_HTC(target);
            break;
        }

        /* setup the endpoint */
        pEndpoint->ServiceID = pConnectReq->ServiceID; /* this marks the endpoint in use */        
        pEndpoint->MaxTxQueueDepth = pConnectReq->MaxSendQueueDepth;

        /* save the pipe id for non-endpoint 0 */
        #if 0
        pEndpoint->UL_PipeID = pConnectReq->UL_PipeID;
        pEndpoint->DL_PipeID = pConnectReq->DL_PipeID;
        #else
        pEndpoint->UL_PipeID = MapSvc2ULPipe(pConnectReq->ServiceID);
        pEndpoint->DL_PipeID = MapSvc2DLPipe(pConnectReq->ServiceID);
        adf_os_print("svc %d UL %d DL %d\n", pConnectReq->ServiceID, pEndpoint->UL_PipeID, pEndpoint->DL_PipeID);
        #endif

        /* copy all the callbacks */
        pEndpoint->EpCallBacks = pConnectReq->EpCallbacks;

        UNLOCK_HTC(target);
        
        /* allocate a buffer to send a connect service message */
        netbuf = adf_nbuf_alloc(os_hdl, 
                                sizeof(HTC_CONNECT_SERVICE_MSG) + pConnectReq->MetaDataLength +HTC_HDR_LENGTH, 
                                HTC_HDR_LENGTH, 0);

        if (netbuf == ADF_NBUF_NULL) {
            status = A_NO_MEMORY;
            break;
        }

        ConnSvc = (HTC_CONNECT_SERVICE_MSG *)
                  adf_nbuf_put_tail(netbuf, sizeof(HTC_CONNECT_SERVICE_MSG) + pConnectReq->MetaDataLength);

        /* assemble connect service message */
        ConnSvc->MessageID = adf_os_htons(HTC_MSG_CONNECT_SERVICE_ID);
        ConnSvc->ServiceID = adf_os_htons(pConnectReq->ServiceID);
        ConnSvc->ConnectionFlags = adf_os_htons(pConnectReq->ConnectionFlags);

        #if 0
        ConnSvc->DownLinkPipeID = pConnectReq->DL_PipeID;
        ConnSvc->UpLinkPipeID = pConnectReq->UL_PipeID;
        #else
        ConnSvc->DownLinkPipeID = pEndpoint->DL_PipeID;
        ConnSvc->UpLinkPipeID = pEndpoint->UL_PipeID;
        #endif
        			
        /* check if caller wants to transfer meta data */		
        if ((pConnectReq->pMetaData != NULL) &&
            (pConnectReq->MetaDataLength <= HTC_SERVICE_META_DATA_MAX_LENGTH)) {
            a_uint8_t *MetaData = ((a_uint8_t *)ConnSvc) + sizeof(HTC_CONNECT_SERVICE_MSG);

            ConnSvc->ServiceMetaLength = pConnectReq->MetaDataLength;
            adf_os_mem_copy(&MetaData, pConnectReq->pMetaData, pConnectReq->MetaDataLength);
        }

#ifdef MAGPIE_SINGLE_CPU_CASE
#else
#ifdef __linux__
        htc_spin_prep(&target->spin);
#endif
#endif

        status = HTCIssueSend(target, 
                              ADF_NBUF_NULL,
                              netbuf, 
                              0, 
                              sizeof(HTC_CONNECT_SERVICE_MSG) + pConnectReq->MetaDataLength, 
                              ENDPOINT0);
	
        if (A_FAILED(status)) {
            break;    
        }

#ifdef MAGPIE_SINGLE_CPU_CASE
#else
#ifdef __linux__
        status = htc_spin(&target->spin);
        if (A_FAILED(status)) {
            status = A_ERROR;
            break;
        }
#else
        wait_status = adf_os_sleep_waitq(&target->wq, 100);

        if (wait_status == A_STATUS_FAILED) {
            /* timeout */
            status = A_ERROR;
        }
        else {
            status = A_OK;
        }
#endif

#endif

        *pConn_rsp_epid = target->conn_rsp_epid;

	} while (FALSE);

	return status;
	
}

static void AddToEndpointDistList(HTC_TARGET *target, HTC_ENDPOINT_CREDIT_DIST *pEpDist)
{
    HTC_ENDPOINT_CREDIT_DIST *pCurEntry = NULL, *pLastEntry = NULL;

    if (NULL == target->EpCreditDistributionListHead) {
        target->EpCreditDistributionListHead = pEpDist;
        pEpDist->pNext = NULL;
        pEpDist->pPrev = NULL;
        return;
    }

    /* queue to the end of the list, this does not have to be very
     * fast since this list is built at startup time */
    pCurEntry = target->EpCreditDistributionListHead;
    while (pCurEntry) {
        pLastEntry = pCurEntry;
        pCurEntry = pCurEntry->pNext;
    }

    pLastEntry->pNext = pEpDist;
    pEpDist->pPrev = pLastEntry;
    pEpDist->pNext = NULL;

}

/* default credit init callback */
static void HTCDefaultCreditInit(void                     *Context,
                                 HTC_ENDPOINT_CREDIT_DIST *pEPList, 
                                 a_uint32_t                 TotalCredits)
{
    HTC_ENDPOINT_CREDIT_DIST *pCurEpDist;
    a_uint32_t  totalEps = 0;

    pCurEpDist = pEPList;
    /* first run through the list and figure out how many endpoints we are dealing with */
    while (pCurEpDist != NULL) {
        pCurEpDist = pCurEpDist->pNext;
        totalEps++;
    }

        pCurEpDist = pEPList;

    /* run through the list and set minimum and normal credits and
     * provide the endpoint with some credits to start */
    while (pCurEpDist != NULL) {
                /* our minimum is set for at least 1 max message */
        pCurEpDist->TxCreditsMin = pCurEpDist->TxCreditsPerMaxMsg;        

        /* this value is ignored by our credit alg, since we do 
         * not dynamically adjust credits, this is the policy of
         * the "default" credit distribution, something simple and easy */
        pCurEpDist->TxCreditsNorm = 0xFFFF;

        /* give the endpoint minimum credits */
 /* assigning credits to htc services for xMII interface */
        switch (pCurEpDist->ServiceID) {
            case WMI_CONTROL_SVC:
                pCurEpDist->TxCredits = 2;
                pCurEpDist->TxCreditsAssigned = 2;
                break;
            case WMI_BEACON_SVC:
                pCurEpDist->TxCredits = 4;
                pCurEpDist->TxCreditsAssigned = 4;
                break;
            case WMI_CAB_SVC:
                pCurEpDist->TxCredits = 1;
                pCurEpDist->TxCreditsAssigned = 1;
                break;
            case WMI_UAPSD_SVC:
                pCurEpDist->TxCredits = 4;
                pCurEpDist->TxCreditsAssigned = 4;
                break;
            case WMI_MGMT_SVC:
                pCurEpDist->TxCredits = 2;
                pCurEpDist->TxCreditsAssigned = 2;
                break;
            case WMI_DATA_VO_SVC:
                pCurEpDist->TxCredits = 4;
                pCurEpDist->TxCreditsAssigned = 4;
                break;
            case WMI_DATA_VI_SVC:
                pCurEpDist->TxCredits = 4;
                pCurEpDist->TxCreditsAssigned = 4;
                break;
            case WMI_DATA_BE_SVC:
                pCurEpDist->TxCredits = 41;
                pCurEpDist->TxCreditsAssigned = 41;
                break;
            case WMI_DATA_BK_SVC:
                pCurEpDist->TxCredits = 4;
                pCurEpDist->TxCreditsAssigned = 4;
                break;
            default:
                adf_os_assert(0); /* invalid service */
        }

        pCurEpDist = pCurEpDist->pNext;
    }

}

/* default credit distribution callback, NOTE, this callback holds the TX lock */
void HTCDefaultCreditDist(void                     *Context, 
                          HTC_ENDPOINT_CREDIT_DIST *pEPDistList,
                          HTC_CREDIT_DIST_REASON   Reason)
{
    HTC_TARGET *target = (HTC_TARGET *)Context;
    HTC_ENDPOINT_CREDIT_DIST *pCurEpDist;

    if (Reason == HTC_CREDIT_DIST_SEND_COMPLETE) {
        pCurEpDist = pEPDistList;

        /* simple distribution */
        while (pCurEpDist != NULL) {
            if (pCurEpDist->TxCreditsToDist > 0) {
                /* just give the endpoint back the credits */
                pCurEpDist->TxCredits += pCurEpDist->TxCreditsToDist;
                if((pCurEpDist->ServiceID != WMI_CONTROL_SVC )&& (pCurEpDist->ServiceID != WMI_BEACON_SVC)){
                    if(pCurEpDist->TxCredits >  pCurEpDist->TxCreditsAssigned ){
                        adf_os_print("Credits gone bad saturating %d to %d  service id %d  \n",pCurEpDist->TxCredits, pCurEpDist->TxCreditsAssigned,pCurEpDist->ServiceID ); 
                        pCurEpDist->TxCredits = pCurEpDist->TxCreditsAssigned ;

                    }
                }


                /* notify upper layer driver of credit update */
                if (pCurEpDist->ServiceID != WMI_CONTROL_SVC && target->HTCInitInfo.HTCCreditUpdateEvent) {
                    target->HTCInitInfo.HTCCreditUpdateEvent(target->os_handle, pCurEpDist->Endpoint, pCurEpDist->TxCreditsToDist);
                }
                pCurEpDist->TxCreditsToDist = 0;
            }
            pCurEpDist = pCurEpDist->pNext;
        }
    }
    
    /* note we do not need to handle the other reason codes as this is a very 
     * simple distribution scheme, no need to seek for more credits or handle inactivity */

}

void HTCSetCreditDistribution(HTC_HANDLE               HTCHandle,
                              void                     *pCreditDistContext,
                              HTC_CREDIT_DIST_CALLBACK CreditDistFunc,
                              HTC_CREDIT_INIT_CALLBACK CreditInitFunc,
                              HTC_SERVICE_ID           ServicePriorityOrder[],
                              a_uint32_t                 ListLength)
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    a_uint32_t i, ep;

    if (CreditInitFunc != NULL) {
        /* caller has supplied their own distribution functions */
        target->InitCredits = CreditInitFunc;
        adf_os_assert(CreditDistFunc != NULL);
        target->DistributeCredits = CreditDistFunc;
        target->pCredDistContext = pCreditDistContext;
    }
    else {
        /* caller wants HTC to do distribution */
        /* if caller wants service to handle distributions then
         * it must set both of these to NULL! */
        adf_os_assert(CreditDistFunc == NULL);
        target->InitCredits = HTCDefaultCreditInit;
        target->DistributeCredits = HTCDefaultCreditDist;
        target->pCredDistContext = target;
    }

    
    /* always add HTC control endpoint first, we only expose the list after the
     * first one, this is added for TX queue checking */
    AddToEndpointDistList(target, &target->EndPoint[ENDPOINT0].CreditDist);

    /* build the list of credit distribution structures in priority order
     * supplied by the caller, these will follow endpoint 0 */
    for (i = 0; i < ListLength; i++) {
        /* match services with endpoints and add the endpoints to the distribution list
         * in FIFO order */
        for (ep = ENDPOINT1; ep < ENDPOINT_MAX; ep++) {
            if (target->EndPoint[ep].ServiceID == ServicePriorityOrder[i]) {
                /* queue this one to the list */
                AddToEndpointDistList(target, &target->EndPoint[ep].CreditDist);
                break;
            }
        }
    }

}

