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

#ifdef MAGPIE_HIF_GMAC
extern os_timer_t host_seek_credit_timer;
#endif

/* call the distribute credits callback with the distribution */
#define DO_DISTRIBUTION(t,reason,description,pList) \
{                                             \
         (t)->DistributeCredits((t)->pCredDistContext,                        \
                           (pList),                                      \
                           (reason));                                    \
}
#ifndef ATH_HTC_TX_SCHED

A_STATUS HTCTxEnqueuePkts(HTC_TARGET *target, 
                               HTC_ENDPOINT *pEndpoint, 
                                 adf_nbuf_t hdr_buf,
                                 adf_nbuf_t buf);

#endif


A_STATUS HTCTrySend(HTC_TARGET      *target, 
                    HTC_ENDPOINT    *pEndpoint, 
                    adf_nbuf_t      hdr_buf,
                    adf_nbuf_t      buf);

#ifdef MAGPIE_SINGLE_CPU_CASE
void HTCWlanTxCompletionHandler(void *Context, a_uint8_t epnum)
{
    HTC_TARGET *target = (HTC_TARGET *)Context;

    /* endpoint id check */
    if (epnum >= ENDPOINT_MAX) {
        adf_os_print("[%s %d] Endpoint %u is invalid!\n", __FUNCTION__, __LINE__, epnum);
        adf_os_assert(0);
	}

    if (target->HTCInitInfo.HTCSchedNextEvent) {
        target->HTCInitInfo.HTCSchedNextEvent(target->host_handle, (HTC_ENDPOINT_ID)epnum);
    }
    else {
        adf_os_print("[%s %d] HTCSchedNextEvent handler is not registered!\n", __FUNCTION__, __LINE__);
    }
}
#endif

#ifndef HTC_HOST_CREDIT_DIST
a_uint16_t HTCGetTxBufCnt(HTC_TARGET *target, HTC_ENDPOINT *pEndpoint)
{
    a_uint16_t TxBufCnt;

    LOCK_HTC_TX(target);
    TxBufCnt = pEndpoint->TxBufCnt;
    UNLOCK_HTC_TX(target);

    return TxBufCnt;
}

A_STATUS HTCNeedReschedule(HTC_TARGET *target, HTC_ENDPOINT *pEndpoint)
{
    A_STATUS status = A_OK;
    a_uint16_t QNum;
    a_uint16_t QThreshold;

    QNum = HIFGetQueueNumber(target->hif_dev, pEndpoint->UL_PipeID);
    QThreshold = HIFGetQueueThreshold(target->hif_dev, pEndpoint->UL_PipeID);
       
    if (QNum >= QThreshold) {
        status = A_ERROR;
    }

    return status;
}
#endif

hif_status_t HTCTxCompletionHandler(void *Context, adf_nbuf_t netbuf)
{
    HTC_TARGET *target = (HTC_TARGET *)Context;
    //adf_os_handle_t os_hdl = target->os_handle;
    a_uint8_t *netdata;
    a_uint32_t netlen;
    HTC_FRAME_HDR *HtcHdr;
    a_uint8_t EpID;
    HTC_ENDPOINT *pEndpoint;
#ifndef HTC_HOST_CREDIT_DIST
    a_int32_t i;
#endif
    
    adf_nbuf_peek_header(netbuf, &netdata, &netlen);

    HtcHdr = (HTC_FRAME_HDR *)netdata;

    EpID = HtcHdr->EndpointID;
    pEndpoint = &target->EndPoint[EpID];

    if (EpID == ENDPOINT0) {
        adf_nbuf_free(netbuf);
    }
    else {
        /* gather tx completion counts */
        //HTC_AGGRNUM_REC *pAggrNumRec;

        //LOCK_HTC_TX(target);
        //pAggrNumRec = HTC_GET_REC_AT_HEAD(&pEndpoint->AggrNumRecQueue);
        
        //aggrNum = pAggrNumRec->AggrNum;
        //UNLOCK_HTC_TX(target);

        //if ((++pEndpoint->CompletedTxCnt) == aggrNum) {
            
            //pEndpoint->CompletedTxCnt = 0;

#if 0
            LOCK_HTC_TX(target);
            
            /* Dequeue from endpoint and then enqueue to target */
            pAggrNumRec = HTC_AGGRNUMREC_DEQUEUE(&pEndpoint->AggrNumRecQueue);
            HTC_AGGRNUMREC_ENQUEUE(&target->FreeAggrNumRecQueue, pAggrNumRec);
            
            UNLOCK_HTC_TX(target);
#endif

            /* remove HTC header */
            adf_nbuf_pull_head(netbuf, HTC_HDR_LENGTH);

            #if 1    /* freeing the net buffer instead of handing this buffer to upper layer driver */
            /* nofity upper layer */            
            if (pEndpoint->EpCallBacks.EpTxComplete) {
                /* give the packet to the upper layer */
                pEndpoint->EpCallBacks.EpTxComplete(/*dev*/pEndpoint->EpCallBacks.pContext, netbuf, EpID/*aggrNum*/);
            }
            else {
                adf_nbuf_free(netbuf);
            }
            #else
            adf_nbuf_free(netbuf);
            #endif
        //}

    }

#ifndef HTC_HOST_CREDIT_DIST
    /* Check whether there is any pending buffer needed */
    /* to be sent */
    if (pEndpoint->UL_PipeID == 1) {
        if (HTCNeedReschedule(target, pEndpoint) == A_OK) {
            for (i = ENDPOINT_MAX - 1; i >= 0; i--) {
                pEndpoint = &target->EndPoint[i];
                
                if (HTCGetTxBufCnt(target, pEndpoint) > 0)  {
                        HTCTrySend(target, NULL, ADF_NBUF_NULL, ADF_NBUF_NULL);
                        break;
                }
            }
        }
    }
#endif    
    
    return HIF_OK;
}

A_STATUS HTCIssueSend(HTC_TARGET *target, 
                      adf_nbuf_t hdr_buf,
                      adf_nbuf_t netbuf, 
                      a_uint8_t SendFlags, 
                      a_uint16_t len, 
                      a_uint8_t EpID)
{
    a_uint8_t pipeID;
    A_STATUS status = A_OK;
    //adf_net_handle_t anet = target->pInstanceContext;
    HTC_ENDPOINT *pEndpoint = &target->EndPoint[EpID];
    HTC_FRAME_HDR *HtcHdr;
    adf_nbuf_t tmp_nbuf;
    //printk("bharath %s \n",__FUNCTION__);
    if (hdr_buf == ADF_NBUF_NULL) {
        /* HTC header needs to be added on the data nbuf */
        tmp_nbuf = netbuf;
    }
    else {
        tmp_nbuf = hdr_buf;
    }

    /* setup HTC frame header */
    HtcHdr = (HTC_FRAME_HDR *)adf_nbuf_push_head(tmp_nbuf, sizeof(HTC_FRAME_HDR));
    adf_os_assert(HtcHdr);
    if ( HtcHdr == NULL ) {
        adf_os_print("%s_%d: HTC Header is NULL !!!\n", __FUNCTION__, __LINE__);
        return A_ERROR;
    }

    HtcHdr->EndpointID = EpID;
    HtcHdr->Flags = SendFlags;
    HtcHdr->PayloadLen = adf_os_htons(len);
    
    HTC_ADD_CREDIT_SEQNO(target,pEndpoint,len,HtcHdr);
   /* lookup the pipe id by the endpoint id */
    pipeID = pEndpoint->UL_PipeID;

//    if (EpID == ENDPOINT0) {
        /* send the buffer to the HIF layer */
        status = HIFSend(target->hif_dev/*anet*/, pipeID, hdr_buf, netbuf);
//    }

#ifdef NBUF_PREALLOC_POOL
    if ( A_FAILED(status) ) {
        adf_nbuf_pull_head(netbuf, HTC_HDR_LENGTH);
    }
#endif

    return status;
}

A_STATUS HTCReclaimCredits(HTC_TARGET *target,
                           a_uint32_t msglen,
                           HTC_ENDPOINT *pEndpoint)
{
    a_uint32_t creditsRequired, remainder;

    /* figure out how many credits this message requires */
    creditsRequired = (msglen / target->TargetCreditSize);
    remainder = (msglen % target->TargetCreditSize);

    if (remainder) {
        creditsRequired++;
    }

    LOCK_HTC_TX(target);

    pEndpoint->CreditDist.TxCredits += creditsRequired;

    UNLOCK_HTC_TX(target);

    return HIF_OK;
}

#ifndef ATH_HTC_TX_SCHED


A_STATUS HTCTrySend(HTC_TARGET      *target, 
                    HTC_ENDPOINT    *pEndpoint, 
                    adf_nbuf_t      hdr_buf,
                    adf_nbuf_t      buf)
{
    a_uint8_t SendFlags = 0;
    a_uint16_t payloadLen;
    a_uint8_t EpID;
    A_STATUS status = A_OK;
#if 0 
    a_uint32_t creditsRequired, remainder;
#endif
    a_int32_t i;
    
    if ( (pEndpoint) 
         && (buf != ADF_NBUF_NULL) ) 
    {
        /* packet was supplied to be queued */
        status = HTCTxEnqueuePkts(target, pEndpoint, hdr_buf, buf);

        if (A_FAILED(status)) {
            if (buf)     { adf_nbuf_free(buf);     }
            if (hdr_buf) { adf_nbuf_free(hdr_buf); }
            //adf_os_print("%s: Not enough queue spaces for enqueuing, epid = %d, enqueue fail cnt = %d\n", 
            //              __func__, 
            //              pEndpoint->CreditDist.Endpoint, ++(pEndpoint->stat_enqfail));
        }
    }

    /* now drain the TX queue for transmission as long as we have enough credits */

    LOCK_HTC_TX(target);

    for (i = ENDPOINT_MAX - 1; i >= 0; i--) {
        a_uint32_t availableTxCnt = 0xFFFFFFFF;
        a_uint8_t  isDataService = 0;

        pEndpoint = &target->EndPoint[i];
        EpID = pEndpoint->CreditDist.Endpoint;

        if (pEndpoint->ServiceID == 0)
            continue;

        /* calculate available tx count base on the de-queue level */
        if ((pEndpoint->ServiceID == WMI_DATA_BE_SVC)
            || (pEndpoint->ServiceID == WMI_DATA_BK_SVC)
            || (pEndpoint->ServiceID == WMI_DATA_VI_SVC)
            || (pEndpoint->ServiceID == WMI_DATA_VO_SVC)) {
            isDataService = 1;
            if (pEndpoint->tqi_dqlevel < 4) {
                availableTxCnt  = 4- pEndpoint->tqi_dqlevel;
            } else {
                availableTxCnt = 1;
            }
            //adf_os_print("%s: epid = %d, i = %d, dqlevel = %d, txcnt = %d\n", __FUNCTION__, EpID, i, pEndpoint->tqi_dqlevel, availableTxCnt);
        }

        while (availableTxCnt-- > 0) {
            adf_nbuf_t tmpbuf, tmp_hdr_buf;
            a_uint16_t tmplen, tmp_hdr_len;
            a_uint16_t freeQNum, maxQNum, toNext;
            
            if (pEndpoint->TxBufCnt == 0) {
                break;
            }
            
            if ( (freeQNum = HIFGetFreeQueueNumber(target->hif_dev, pEndpoint->UL_PipeID)) == 0 ) {
                //adf_os_print("%s: USB TxQ full, epid = %d, pipe id = %d, count = %d\n", 
                //              __func__, EpID, pEndpoint->UL_PipeID, ++(pEndpoint->stat_usbqfull));
                break;
            }

            if (isDataService) {
                toNext = 0;
                maxQNum = HIFGetMaxQueueNumber(target->hif_dev, pEndpoint->UL_PipeID);
                //adf_os_print("%s: epid = %d, freeQNum = %d, maxQNum = %d, availableTxCnt = %d\n", __FUNCTION__, EpID, freeQNum, maxQNum, availableTxCnt);
                switch (pEndpoint->tqi_dqlevel) {
                case 0:
                    if (freeQNum == 0) {toNext = 1;}
                        break;
                case 1:
                    if (freeQNum <= maxQNum/2) {toNext = 1;}        //4/8
                        break;
                case 2:
                    if (freeQNum <= (maxQNum*5)/8) {toNext = 1;}    //5/8
                        break;
                case 3:
                default:
                    if (freeQNum <= (maxQNum*3)/4) {toNext = 1;}    //6/8
                        break;
                }

                if (toNext == 1) {
                //    adf_os_print("%s: To Next Endpoint, epid = %d, freeQNum = %d, maxQNum = %d\n", __FUNCTION__, EpID, freeQNum, maxQNum);
                    break;
                }
            }

            /* get packet at head, but don't remove it */
            tmp_hdr_buf = pEndpoint->HtcTxQueue[pEndpoint->TxQHead].hdr_buf;
            tmp_hdr_len = pEndpoint->HtcTxQueue[pEndpoint->TxQHead].hdr_bufLen;
            tmpbuf = pEndpoint->HtcTxQueue[pEndpoint->TxQHead].buf;
            tmplen = pEndpoint->HtcTxQueue[pEndpoint->TxQHead].bufLen;

    #if 0 
    
            /* figure out how many credits this message requires */
            creditsRequired = (tmplen + tmp_hdr_len + HTC_HDR_LENGTH) / target->TargetCreditSize;
            remainder = (tmplen + tmp_hdr_len + HTC_HDR_LENGTH) % target->TargetCreditSize;
    
            if (remainder) {
                creditsRequired++;
            }
    
            adf_os_print("creditsRequired: %u, tmplen + HTC_HDR_LENGTH: %u, target->TargetCreditSize: %u, pEndpoint->CreditDist.TxCredits: %u\n", creditsRequired, tmplen + HTC_HDR_LENGTH, target->TargetCreditSize, pEndpoint->CreditDist.TxCredits);
    
            /* not enough credits */
            if (pEndpoint->CreditDist.TxCredits < creditsRequired) {
    
                /* set how many credits we need  */
                pEndpoint->CreditDist.TxCreditsSeek = creditsRequired - pEndpoint->CreditDist.TxCredits;
    
                DO_DISTRIBUTION(target,
                                HTC_CREDIT_DIST_SEEK_CREDITS,
                                "Seek Credits",
                                &pEndpoint->CreditDist);
    
                pEndpoint->CreditDist.TxCreditsSeek = 0;
    
                if (pEndpoint->CreditDist.TxCredits < creditsRequired) {
                    /* still not enough credits to send, leave packet in the queue */
					status = A_CREDIT_UNAVAILABLE;                    
					break;
                }
            }
    
            pEndpoint->CreditDist.TxCredits -= creditsRequired;
    
            //adf_os_print("pEndpoint->CreditDist.TxCreditsPerMaxMsg: %u\n", pEndpoint->CreditDist.TxCreditsPerMaxMsg);
    
            /* check if we need credits back from the target */
            if (pEndpoint->CreditDist.TxCredits < pEndpoint->CreditDist.TxCreditsPerMaxMsg) {
                /* we are getting low on credits, see if we can ask for more from the distribution function */
                pEndpoint->CreditDist.TxCreditsSeek = pEndpoint->CreditDist.TxCreditsPerMaxMsg - pEndpoint->CreditDist.TxCredits;
    
                DO_DISTRIBUTION(target,
                                HTC_CREDIT_DIST_SEEK_CREDITS,
                                "Seek Credits",
                                &pEndpoint->CreditDist);
    
                pEndpoint->CreditDist.TxCreditsSeek = 0;
    
                if (pEndpoint->CreditDist.TxCredits < pEndpoint->CreditDist.TxCreditsPerMaxMsg) {
                    /* tell the target we need credits ASAP! */
                    SendFlags |= HTC_FLAGS_NEED_CREDIT_UPDATE;
                    adf_os_print("SendFlags: %u\n", SendFlags);
                }
    
    #endif   
    
                payloadLen = tmp_hdr_len + tmplen;
    
                /* now we can fully dequeue */
                pEndpoint->TxQHead = (pEndpoint->TxQHead + 1) % HTC_TX_QUEUE_SIZE;
                pEndpoint->TxBufCnt--;
    
                UNLOCK_HTC_TX(target);

                status = HTCIssueSend(target, tmp_hdr_buf, tmpbuf, SendFlags, payloadLen, EpID);

                LOCK_HTC_TX(target);

                if (A_FAILED(status)) {
                    adf_os_print("%s: Fail to send pkt to HIF, cnt = %d, usesendfail = %d\n", 
                           __func__, EpID, ++(pEndpoint->stat_usbsendfail));
#if 0
        			/* reclaim the credit due to failure to send */
            		HTCReclaimCredits(target, HTC_HDR_LENGTH + payloadLen, pEndpoint);
#endif        
                    if (tmpbuf)      { adf_nbuf_free(tmpbuf);      }
                    if (tmp_hdr_buf) { adf_nbuf_free(tmp_hdr_buf); }
                    break;
                }
    
    #if 0 
            }
    #endif
        }
    } /* for (i = ENDPOINT_MAX - 1; i >= 0; i--) */
    
    UNLOCK_HTC_TX(target);
#if 0 
       return status;
#else 
       return A_OK;
#endif  
}

A_STATUS HTCTrySendAll(HTC_HANDLE hHTC)
{
	HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(hHTC);

	HTCTrySend(target, NULL, ADF_NBUF_NULL, ADF_NBUF_NULL);

	return A_OK;
}
#endif



A_STATUS HTCCheckCredits(HTC_TARGET *target,
                         a_uint32_t msglen,
                         HTC_ENDPOINT *pEndpoint,
                         a_uint8_t *pSendFlags)
{
    a_uint32_t creditsRequired, remainder;

    /* figure out how many credits this message requires */
    creditsRequired = (msglen / target->TargetCreditSize);
    remainder = (msglen % target->TargetCreditSize);

    if (remainder) {
        creditsRequired++;
    }

    LOCK_HTC_TX(target);

    //adf_os_print("creditsRequired: %u, msglen: %u, target->TargetCreditSize: %u, pEndpoint->CreditDist.TxCredits: %u\n", creditsRequired, msglen, target->TargetCreditSize, pEndpoint->CreditDist.TxCredits);

    /* not enough credits */
    if (pEndpoint->CreditDist.TxCredits < creditsRequired) {

        /* set how many credits we need  */
        pEndpoint->CreditDist.TxCreditsSeek = creditsRequired - pEndpoint->CreditDist.TxCredits;

        DO_DISTRIBUTION(target,
                        HTC_CREDIT_DIST_SEEK_CREDITS,
                        "Seek Credits",
                        &pEndpoint->CreditDist);

        pEndpoint->CreditDist.TxCreditsSeek = 0;

        if (pEndpoint->CreditDist.TxCredits < creditsRequired) {
            /* still not enough credits to send, leave packet in the queue */
            UNLOCK_HTC_TX(target);
            //adf_os_print("htc: credit is unavailable: %u, pEndpoint->CreditDist.TxCredits: %u, creditsRequired: %u\n", A_CREDIT_UNAVAILABLE, pEndpoint->CreditDist.TxCredits, creditsRequired);
            return A_CREDIT_UNAVAILABLE;
        }
    }

    pEndpoint->CreditDist.TxCredits -= creditsRequired;
    /* check if we need credits back from the target */
    if (pEndpoint->CreditDist.TxCredits < pEndpoint->CreditDist.TxCreditsPerMaxMsg) {
        /* we are getting low on credits, see if we can ask for more from the distribution function */
        pEndpoint->CreditDist.TxCreditsSeek = pEndpoint->CreditDist.TxCreditsPerMaxMsg - pEndpoint->CreditDist.TxCredits;

        DO_DISTRIBUTION(target,
                        HTC_CREDIT_DIST_SEEK_CREDITS,
                        "Seek Credits",
                        &pEndpoint->CreditDist);

        pEndpoint->CreditDist.TxCreditsSeek = 0;

        if (pEndpoint->CreditDist.TxCredits < pEndpoint->CreditDist.TxCreditsPerMaxMsg) {
            /* tell the target we need credits ASAP! */
            *pSendFlags |= HTC_FLAGS_NEED_CREDIT_UPDATE;
            //adf_os_print("SendFlags: %u\n", *pSendFlags);
        }
    }

    UNLOCK_HTC_TX(target);
    return A_OK;
}

void HTCTxComp(HTC_HANDLE hHTC,
               adf_nbuf_t buf,
               HTC_ENDPOINT_ID ep)
{
#ifdef __linux__
    HTC_ENDPOINT *pEndpoint;
    HTC_TARGET   *target = GET_HTC_TARGET_FROM_HANDLE(hHTC);

    pEndpoint = &target->EndPoint[ep];

    if (pEndpoint->EpCallBacks.EpTxComplete != NULL) {
        pEndpoint->EpCallBacks.EpTxComplete( pEndpoint->EpCallBacks.pContext, buf, ep);
    }
#endif
}

A_STATUS HTCSendPkt(HTC_HANDLE hHTC,
                    adf_nbuf_t hdr_buf,
                    adf_nbuf_t buf,
                    HTC_ENDPOINT_ID Ep)
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(hHTC);
    A_STATUS status = A_OK;
    HTC_ENDPOINT *pEndpoint;

#ifndef ATH_HTC_TX_SCHED
    do {
	    if (Ep >= ENDPOINT_MAX) {
            adf_os_print("Ep %u is invalid!\n", Ep);
            if (hdr_buf) {
                adf_nbuf_free(hdr_buf);
            }
            if (buf) {
                adf_nbuf_free(buf);
            }
		    status = A_EPROTO;
		    break;
	    }

	    pEndpoint = &target->EndPoint[Ep];

#if 0
        LOCK_HTC_TX(target);
        /* record number of buffers in corresponding endpoint */
        if ((pAggrNumRec = HTC_AGGRNUMREC_DEQUEUE(&target->FreeAggrNumRecQueue)) == NULL) {
            adf_os_print("%s no more free aggr record\n", __FUNCTION__);
            status = A_NO_RESOURCE;
            UNLOCK_HTC_TX(target);
            break;
        }
        pAggrNumRec->AggrNum = num;
        pAggrNumRec->bufArray = bufArray;

        /* buffers were supplied to be queued */
        HTC_AGGRNUMREC_ENQUEUE(&pEndpoint->AggrNumRecQueue, pAggrNumRec);
        UNLOCK_HTC_TX(target);
#endif

        status = HTCTrySend(target, pEndpoint, hdr_buf, buf);

    //    adf_os_print("return value of HTCTrySend: %u\n", status);

    } while(FALSE);
#else
	a_uint16_t payloadLen;
    a_uint8_t SendFlags = 0;

    pEndpoint = &target->EndPoint[Ep];
    if ( HIFGetFreeQueueNumber(target->hif_dev, pEndpoint->UL_PipeID) == 0 )
            return -1 ;

    if(hdr_buf != NULL)
        payloadLen = (a_uint16_t) (adf_nbuf_len(hdr_buf) + adf_nbuf_len(buf));
    else
        payloadLen = (a_uint16_t) adf_nbuf_len(buf)  ;

#ifdef HTC_HOST_CREDIT_DIST
    /* check if credits are enough for sending this packet */
#ifdef WMI_RETRY
    if((Ep != 1 ) && (Ep != 2)) 
#endif
    {
        status = HTCCheckCredits(target, HTC_HDR_LENGTH + payloadLen, pEndpoint, &SendFlags);
        if (A_FAILED(status)) {
            //adf_os_print("credits are not enough for sending the message!\n");
            return status;
        }
    }
#endif
    status = HTCIssueSend(target, hdr_buf, buf, SendFlags, payloadLen, Ep);

    if (A_FAILED(status)) {
        HTCReclaimCredits(target, HTC_HDR_LENGTH + payloadLen, pEndpoint);
        return -1 ;
    }

#endif
    return status;
}

#ifndef ATH_HTC_TX_SCHED
A_STATUS HTCTxEnqueuePkts(HTC_TARGET *target, 
                               HTC_ENDPOINT *pEndpoint, 
                                 adf_nbuf_t hdr_buf,
                                 adf_nbuf_t buf)
{
    //a_uint32_t i;
    A_STATUS status = A_OK;

    LOCK_HTC_TX(target);

    if (pEndpoint->TxBufCnt >= HTC_TX_QUEUE_SIZE) {
        UNLOCK_HTC_TX(target);
        //adf_os_print("HTC Tx queue is full!\n");
        return A_NO_RESOURCE;
    }

    pEndpoint->HtcTxQueue[pEndpoint->TxQTail].hdr_buf = hdr_buf;
    if (hdr_buf == ADF_NBUF_NULL) {
        pEndpoint->HtcTxQueue[pEndpoint->TxQTail].hdr_bufLen = 0;
    }
    else {
        pEndpoint->HtcTxQueue[pEndpoint->TxQTail].hdr_bufLen = adf_nbuf_len(hdr_buf);
    }
    pEndpoint->HtcTxQueue[pEndpoint->TxQTail].buf = buf;
    pEndpoint->HtcTxQueue[pEndpoint->TxQTail].bufLen = adf_nbuf_len(buf);

    pEndpoint->TxQTail = (pEndpoint->TxQTail + 1) % HTC_TX_QUEUE_SIZE;
    pEndpoint->TxBufCnt += 1;
    
    UNLOCK_HTC_TX(target);

    return (status? A_ERROR : A_OK);
}
#endif
/* process credit reports and call distribution function */
void HTCProcessCreditRpt(HTC_TARGET *target, HTC_CREDIT_REPORT *pRpt0, a_uint32_t RecLen, HTC_ENDPOINT_ID FromEndpoint)
{
    a_uint32_t i;
    a_uint32_t NumEntries;

#ifdef HTC_HOST_CREDIT_DIST
    a_uint32_t totalCredits = 0;
    a_uint8_t doDist = FALSE;
    HTC_ENDPOINT *pEndpoint;

    a_uint16_t seq_diff;
    a_uint16_t tgt_seq;
#endif
    HTC_CREDIT_REPORT_1_1 *pRpt = (HTC_CREDIT_REPORT_1_1 *)pRpt0 ;
    

    NumEntries = RecLen / (sizeof(HTC_CREDIT_REPORT_1_1)) ;
    /* lock out TX while we update credits */
    LOCK_HTC_TX(target);

    for (i = 0; i < NumEntries; i++, pRpt++) {

        if (pRpt->EndpointID >= ENDPOINT_MAX) {
            adf_os_assert(0);
            break;
        }
#ifdef HTC_HOST_CREDIT_DIST
        pEndpoint = &target->EndPoint[pRpt->EndpointID];

        tgt_seq =  adf_os_ntohs(pRpt->TgtCreditSeqNo);
        if (ENDPOINT0 == pRpt->EndpointID) {
            /* always give endpoint 0 credits back */
            seq_diff =  (tgt_seq - pEndpoint->LastCreditSeq)  & (HTC_SEQ_MAX -1);

            pEndpoint->CreditDist.TxCredits += seq_diff;
            pEndpoint->LastCreditSeq = tgt_seq;

        }
        else {
            /* for all other endpoints, update credits to distribute, the distribution function
             * will handle giving out credits back to the endpoints */

#ifdef MAGPIE_HIF_GMAC
            if ( pRpt->EndpointID == 6 ) 
                OS_SET_TIMER(&host_seek_credit_timer, 1000);
#endif            
            
            seq_diff =  (tgt_seq - pEndpoint->LastCreditSeq)  & (HTC_SEQ_MAX -1);

            pEndpoint->CreditDist.TxCreditsToDist += seq_diff;
            pEndpoint->LastCreditSeq = tgt_seq;
            /* flag that we have to do the distribution */
            doDist = TRUE;
        }

        totalCredits += seq_diff;
#endif
        
    }
#ifdef HTC_HOST_CREDIT_DIST
    if (doDist) {
        /* this was a credit return based on a completed send operations
         * note, this is done with the lock held */       
        DO_DISTRIBUTION(target,
                        HTC_CREDIT_DIST_SEND_COMPLETE,
                        "Send Complete",
                        target->EpCreditDistributionListHead->pNext);        
    }
#endif
    UNLOCK_HTC_TX(target);
    
}



#if 0 /*: Removed temp not yet used in Credit distr */
/* check TX queues to drain */
static INLINE void HTCCheckEndpointTxQueues(HTC_TARGET *target)
{
    HTC_ENDPOINT *pEndpoint;
    HTC_ENDPOINT_CREDIT_DIST *pDistItem;

    pDistItem = target->EpCreditDistributionListHead;

    /* run through the credit distribution list to see
     * if there are packets queued
     * NOTE: no locks need to be taken since the distribution list
     * is not dynamic (cannot be re-ordered) and we are not modifying any state */
    while (pDistItem != NULL) {
        pEndpoint = (HTC_ENDPOINT *)pDistItem->pHTCReserved;
        if (pEndpoint->TxBufCnt > 0) {
            /* try to start the stalled queue, this list is ordered by priority. 
             * Highest priority queue get's processed first, if there are credits available the
             * highest priority queue will get a chance to reclaim credits from lower priority
             * ones */
            HTCTrySend(target, pEndpoint, ADF_NBUF_NULL, ADF_NBUF_NULL);
        }
        pDistItem = pDistItem->pNext;
    }
}
#endif
A_BOOL
HTC_busy(HTC_HANDLE HTCHandle, HTC_ENDPOINT_ID Ep, a_uint32_t nPkts)
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    A_BOOL busy_flag;
    HTC_ENDPOINT *pEndpoint;

	if (Ep >= ENDPOINT_MAX) {
		adf_os_print("Ep %u is invalid!\n", Ep);
		adf_os_assert(0);
	}

	pEndpoint = &target->EndPoint[Ep];

    LOCK_HTC_TX(target);
    
    if (pEndpoint->TxBufCnt >= nPkts) {
        busy_flag = TRUE;
    }
    else {
        busy_flag = FALSE;
    }
    
    UNLOCK_HTC_TX(target);

    return busy_flag;
}

void 
HTCTxEpUpdate(HTC_HANDLE	    hHTC, 
              HTC_ENDPOINT_ID   Ep,
              a_uint32_t          aifs, 
              a_uint32_t          cwmin, 
              a_uint32_t          cwmax)
{
    struct HTC_EP_PRIORITY_SORT {
        HTC_ENDPOINT_ID             epID;
        struct HTC_EP_PRIORITY_SORT *pNext;
        struct HTC_EP_PRIORITY_SORT *pPre;
    };

    HTC_TARGET                  *target = GET_HTC_TARGET_FROM_HANDLE(hHTC);
    HTC_ENDPOINT                *pEndpoint;
    HTC_ENDPOINT_ID             dataEpID[4]; /* 0:BK, 1:BE, 2:VI, 3:VO */
    struct HTC_EP_PRIORITY_SORT header, sort[4], *tmp;
    a_int32_t                     i;
    a_uint32_t                    lastPriority;

    /* check if the epid validation */
    if (Ep >= ENDPOINT_MAX) {
        adf_os_print("%s: Ep %u is invalid !\n", __func__, Ep);
        return;
    }
    
    /* check if the endpoint is for the data frame */
    pEndpoint = &target->EndPoint[Ep];
    if ((pEndpoint->ServiceID != WMI_DATA_BE_SVC)
        && (pEndpoint->ServiceID != WMI_DATA_BK_SVC)
        && (pEndpoint->ServiceID != WMI_DATA_VI_SVC)
        && (pEndpoint->ServiceID != WMI_DATA_VO_SVC)) {
            adf_os_print("%s: ServiceID %u is invalid !\n", __func__, pEndpoint->ServiceID);
            return;
    }

    /* save tx queue parameters */
    pEndpoint->tqi_aifs = aifs;
	pEndpoint->tqi_cwmin = cwmin;
    pEndpoint->tqi_cwmax = cwmax;
	pEndpoint->tqi_priority = aifs + cwmin;

    /* search data transmission endpoints */
    for (i = 0; i < ENDPOINT_MAX; i++) {
        switch (target->EndPoint[i].ServiceID) {
		case WMI_DATA_BE_SVC:
			dataEpID[1] = i;
			break;
		case WMI_DATA_BK_SVC:
			dataEpID[0] = i;
			break;
		case WMI_DATA_VI_SVC:
			dataEpID[2] = i;
			break;
		case WMI_DATA_VO_SVC:
			dataEpID[3] = i;
			break;
		}
	}

    /* sort endpoints by priority */
    header.pNext = header.pPre = NULL;
    for (i = 3; i >= 0; i--) {
        /* VO->VI->BE->BK */
	    sort[i].epID = dataEpID[i];

	    if (header.pNext == NULL) {
		    header.pNext = header.pPre = &(sort[i]);
		    sort[i].pPre = &header;
            sort[i].pNext = NULL;
	    } else {
		    tmp = header.pNext;
		    do {
			    if (target->EndPoint[sort[i].epID].tqi_priority < target->EndPoint[tmp->epID].tqi_priority) {
				    sort[i].pNext = tmp;
				    sort[i].pPre = tmp->pPre;
				    tmp->pPre = &(sort[i]);
				    sort[i].pPre->pNext = &(sort[i]);
				    break;
			    }					
		    } while ((tmp = tmp->pNext));

		    if (tmp == NULL) {
			    header.pPre->pNext = &(sort[i]);
			    sort[i].pPre = header.pPre;
                sort[i].pNext = NULL;
			    header.pPre = &(sort[i]);
		    }
	    }
    }

    /* assign endpoints de-queue level */
    i = 0;
    tmp = header.pNext;
    lastPriority = target->EndPoint[tmp->epID].tqi_priority;
    do {
        if (lastPriority != target->EndPoint[tmp->epID].tqi_priority) {
            target->EndPoint[tmp->epID].tqi_dqlevel = ++i;
        } else {
            target->EndPoint[tmp->epID].tqi_dqlevel = i;
        }

        lastPriority = target->EndPoint[tmp->epID].tqi_priority;

#if 0
        adf_os_print("%s: EPID = %d, ServiceID = 0x%x, dqlevel = %d, priority = %d, aifs = %d, cwmin = %d, cwmax = %d\n", 
                __func__, tmp->epID, target->EndPoint[tmp->epID].ServiceID, 
                target->EndPoint[tmp->epID].tqi_dqlevel, target->EndPoint[tmp->epID].tqi_priority,
                target->EndPoint[tmp->epID].tqi_aifs, target->EndPoint[tmp->epID].tqi_cwmin,
                target->EndPoint[tmp->epID].tqi_cwmax);
#endif
	} while ((tmp = tmp->pNext));
}
