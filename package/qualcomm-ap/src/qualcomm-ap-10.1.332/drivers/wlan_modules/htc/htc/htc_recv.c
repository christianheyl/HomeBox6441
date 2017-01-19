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
#include <wmi.h>
#include <wmi_host_api.h>

static A_STATUS HTCProcessTargetReady(HTC_TARGET *target, adf_nbuf_t netbuf, HTC_FRAME_HDR *HtcHdr)
{
    a_uint8_t MaxEps; /* It seems useless for now */
    A_STATUS status = A_OK;
    HTC_ENDPOINT *pEndpoint;
    a_uint8_t *netdata;
    a_uint32_t netlen;
    HTC_READY_MSG *HtcRdy;

    adf_nbuf_peek_header(netbuf, &netdata, &netlen);
    HtcRdy = (HTC_READY_MSG *)netdata;

    /* Retrieve information in the HTC_Ready message */
    target->TargetCredits = adf_os_ntohs(HtcRdy->CreditCount);
    target->TargetCreditSize = adf_os_ntohs(HtcRdy->CreditSize);
    MaxEps = HtcRdy->MaxEndpoints;

	#ifdef HTC_TEST
	if (target->InitHtcTestInfo.TargetReadyEvent) {
	    HTC_READY_MSG HtcReadyMsg;

	    adf_os_mem_zero(&HtcReadyMsg, sizeof(HTC_READY_MSG));
	    HtcReadyMsg.MessageID = HTC_MSG_READY_ID;
	    HtcReadyMsg.CreditCount = (a_uint16_t)target->TargetCredits;
	    HtcReadyMsg.CreditSize = (a_uint16_t)target->TargetCreditSize;
	    HtcReadyMsg.MaxEndpoints = MaxEps;

	    target->InitHtcTestInfo.TargetReadyEvent(target->host_handle, HtcHdr, HtcReadyMsg);
	}
	#else

    //#ifdef MAGPIE_SINGLE_CPU_CASE
    //if (target->HTCInitInfo.TargetReadyEvent) {
    //    /* Notify upper layer driver of the total credits */
    //    target->HTCInitInfo.TargetReadyEvent(target->host_handle, target->TargetCredits);
    //}
    //#endif /* MAGPIE_SINGLE_CPU_CASE */

    #endif

    /* setup reserved endpoint 0 for HTC control message communications */
    pEndpoint = &target->EndPoint[ENDPOINT0];
    pEndpoint->ServiceID = HTC_CTRL_RSVD_SVC;
    pEndpoint->MaxMsgLength = HTC_MAX_CONTROL_MESSAGE_LENGTH;

    //adf_os_wake_waitq(&target->wq);

    target->htc_rdy_state = HTC_RDY_SUCCESS;

    adf_os_mutex_release(&target->htc_rdy_mutex);
    adf_os_print("[%s %d] wakeup mutex HTCWaitForHtcRdy!\n", __FUNCTION__, __LINE__);

    return status;

}

static A_STATUS HTCProcessConnectionRsp(HTC_TARGET *target, adf_nbuf_t netbuf, HTC_FRAME_HDR *HtcHdr)
{
    A_STATUS status = A_OK;
    a_uint8_t *netdata;
    a_uint32_t netlen;
    HTC_CONNECT_SERVICE_RESPONSE_MSG *ConnSvcRsp;
    a_uint8_t ConnectRespCode;

    adf_nbuf_peek_header(netbuf, &netdata, &netlen);

    ConnSvcRsp = (HTC_CONNECT_SERVICE_RESPONSE_MSG *)netdata;
    ConnectRespCode = ConnSvcRsp->Status;

    /* check response status */
    if (HTC_SERVICE_SUCCESS == ConnectRespCode) {
        a_uint16_t maxMsgSize = adf_os_ntohs(ConnSvcRsp->MaxMsgSize);
        HTC_ENDPOINT_ID assignedEndpoint = ConnSvcRsp->EndpointID;
        HTC_ENDPOINT *pAssignedEp;
        HTC_SERVICE_ID rspSvcID = adf_os_ntohs(ConnSvcRsp->ServiceID);
        HTC_ENDPOINT_ID tmpEpID;
        HTC_ENDPOINT *pEndpoint = NULL;

        #ifdef HTC_TEST
        HTC_CONNECT_SERVICE_RESPONSE_MSG HtcConnSvcRspMsg;

        HtcConnSvcRspMsg.MessageID = HTC_MSG_CONNECT_SERVICE_RESPONSE_ID;
        HtcConnSvcRspMsg.ServiceID = rspSvcID;
        HtcConnSvcRspMsg.Status = ConnectRespCode;
        HtcConnSvcRspMsg.EndpointID = assignedEndpoint;
        HtcConnSvcRspMsg.MaxMsgSize = maxMsgSize;
        HtcConnSvcRspMsg.ServiceMetaLength = ConnSvcRsp->ServiceMetaLength;
        #endif

        LOCK_HTC(target);
        pAssignedEp = &target->EndPoint[assignedEndpoint];

        /* find the temporal endpoint in which service configurations are saved */
        for (tmpEpID = (ENDPOINT_MAX - 1); tmpEpID > ENDPOINT0; tmpEpID--) {
            pEndpoint = &target->EndPoint[tmpEpID];
            if (pEndpoint->ServiceID == rspSvcID) {
                /* clear the service id */
                pEndpoint->ServiceID = 0;
                break;
            }
        }
       
        if (pEndpoint == NULL) {
            status = A_ERROR;
            UNLOCK_HTC(target);
            goto out;
        }

        /* Copy the configurations saved in temporal endpoint to the specified endpoint */
        pAssignedEp->ServiceID = rspSvcID;
        pAssignedEp->MaxTxQueueDepth = pEndpoint->MaxTxQueueDepth;
        pAssignedEp->EpCallBacks = pEndpoint->EpCallBacks;
        
        pAssignedEp->UL_PipeID = pEndpoint->UL_PipeID;
        pAssignedEp->DL_PipeID = pEndpoint->DL_PipeID;

        pAssignedEp->MaxMsgLength = maxMsgSize;

#if 0
        /* initialize queues related to this endpoint */
        INIT_HTC_PACKET_QUEUE(&pAssignedEp->TxQueue);
#endif

        /* set the credit distribution info for this endpoint, this information is
         * passed back to the credit distribution callback function */
        pAssignedEp->CreditDist.ServiceID = pAssignedEp->ServiceID;
        pAssignedEp->CreditDist.pHTCReserved = target;
        pAssignedEp->CreditDist.Endpoint = assignedEndpoint;
        pAssignedEp->CreditDist.TxCreditSize = target->TargetCreditSize;
        pAssignedEp->CreditDist.TxCreditsPerMaxMsg = maxMsgSize/target->TargetCreditSize;

        //adf_os_print("%s: pAssignedEp->CreditDist.TxCreditsPerMaxMsg: %u, maxMsgSize: %u, target->TargetCreditSize: %u\n", __FUNCTION__, pAssignedEp->CreditDist.TxCreditsPerMaxMsg, maxMsgSize, target->TargetCreditSize);

        if (0 == pAssignedEp->CreditDist.TxCreditsPerMaxMsg) {
            pAssignedEp->CreditDist.TxCreditsPerMaxMsg = 1;  
        }

        UNLOCK_HTC(target);

        #ifdef HTC_TEST
        //if (target->InitHtcTestInfo.ConnectRspEvent) {
        //    target->InitHtcTestInfo.ConnectRspEvent(target->host_handle, HtcHdr, HtcConnSvcRspMsg);
        //}
        #else
        //if (target->HTCInitInfo.ConnectRspEvent) {
        //    target->HTCInitInfo.ConnectRspEvent(target->host_handle, assignedEndpoint, rspSvcID);
        //}
        #endif

        target->conn_rsp_epid = assignedEndpoint;

    }
    else {
        
        #ifdef HTC_TEST
        //HTC_CONNECT_SERVICE_RESPONSE_MSG HtcConnSvcRspMsg;

        //HtcConnSvcRspMsg.MessageID = HTC_MSG_CONNECT_SERVICE_RESPONSE_ID;
        //HtcConnSvcRspMsg.ServiceID = adf_os_ntohs(ConnSvcRsp->ServiceID);
        //HtcConnSvcRspMsg.Status = ConnectRespCode;
        //HtcConnSvcRspMsg.EndpointID = ConnSvcRsp->EndpointID;
        //HtcConnSvcRspMsg.MaxMsgSize = adf_os_ntohs(ConnSvcRsp->MaxMsgSize);
        //HtcConnSvcRspMsg.ServiceMetaLength = ConnSvcRsp->ServiceMetaLength;
        //if (target->InitHtcTestInfo.ConnectRspEvent) {
        //    target->InitHtcTestInfo.ConnectRspEvent(target->host_handle, HtcHdr, HtcConnSvcRspMsg);
        //}
        #endif

        target->conn_rsp_epid = ENDPOINT_UNUSED;
        status = A_EPROTO;
    }
out:

#ifdef MAGPIE_SINGLE_CPU_CASE
#else
#ifdef __linux__
    htc_spin_over(&target->spin);
#else
    adf_os_wake_waitq(&target->wq);
#endif
#endif
    return status;
    
}

static A_STATUS HTCProcessConfigPipeRsp(HTC_TARGET *target, adf_nbuf_t netbuf, HTC_FRAME_HDR *HtcHdr)
{
    a_uint8_t *netdata;
    a_uint32_t netlen;
    A_STATUS status = A_OK;
    HTC_CONFIG_PIPE_RESPONSE_MSG *CfgPipeRsp;

    adf_nbuf_peek_header(netbuf, &netdata, &netlen);

    CfgPipeRsp = (HTC_CONFIG_PIPE_RESPONSE_MSG *)netdata;

    /* check the status of pipe configuration */
    //a_uint8_t ConfigPipeRspCode = CfgPipeRsp->Status;

#ifdef HTC_TEST
    //if (target->InitHtcTestInfo.ConfigPipeRspEvent)
    //{
    //    HTC_CONFIG_PIPE_RESPONSE_MSG HtcConfigPipeRspMsg;

    //    HtcConfigPipeRspMsg.MessageID = HTC_MSG_CONFIG_PIPE_RESPONSE_ID;
    //    HtcConfigPipeRspMsg.PipeID = CfgPipeRsp->PipeID;
    //    HtcConfigPipeRspMsg.Status = ConfigPipeRspCode;

    //    target->InitHtcTestInfo.ConfigPipeRspEvent(target->host_handle, HtcHdr, HtcConfigPipeRspMsg);
    //}
#else
    //if (HTC_CONFIGPIPE_SUCCESS == ConfigPipeRspCode && 
    //    target->HTCInitInfo.ConfigPipeRspEvent) {
        /* notify upper layer driver about the reception of pipe config response */
    //    target->HTCInitInfo.ConfigPipeRspEvent(target->host_handle, ConfigPipeRspCode);
    //}
    //else {
    //    status = A_EPROTO;
    //}
#endif

    target->cfg_pipe_rsp_stat = CfgPipeRsp->Status;
#ifdef MAGPIE_SINGLE_CPU_CASE
#else
#ifdef __linux__
    htc_spin_over(&target->spin);
#else
    adf_os_wake_waitq(&target->wq);
#endif
#endif
    return status;
						
}

static A_STATUS HTCProcessRxTrailer(HTC_TARGET *target,
                                    adf_nbuf_t netbuf,
                                    a_uint8_t *netdata, 
                                    a_uint32_t netlen, 
                                    HTC_FRAME_HDR *HtcHdr)
{
    A_STATUS status = A_OK;
    a_uint8_t trailerLen = HtcHdr->ControlBytes[0];
    a_uint16_t payloadLen = adf_os_ntohs(HtcHdr->PayloadLen);
    //adf_net_handle_t anet = target->pInstanceContext;
    HTC_RECORD_HDR *HtcRecHdr; 
    a_uint8_t len_tmp;
    a_uint8_t *pRecordBuf;

    /* advances the pointer to beginning of the trailer */
    netdata += (HTC_HDR_LENGTH + payloadLen - trailerLen);

    len_tmp = trailerLen;

    while (len_tmp > 0) {
        //a_uint8_t recordType, recordLen;

        if (len_tmp < sizeof(HTC_RECORD_HDR)) {
            status = A_EPROTO;
            break;
        }
        HtcRecHdr = (HTC_RECORD_HDR *)netdata;
        len_tmp -= sizeof(HTC_RECORD_HDR);
        netdata += sizeof(HTC_RECORD_HDR);

        if (len_tmp < HtcRecHdr->Length) {
            status = A_EPROTO;
            break;
        }

        pRecordBuf = netdata;

        switch(HtcRecHdr->RecordID) {
            case HTC_RECORD_CREDITS_1_1:
                /* Process credit report */
                HTCProcessCreditRpt(target,
                                    (HTC_CREDIT_REPORT *)pRecordBuf, 
                                    HtcRecHdr->Length ,
                                    HtcHdr->EndpointID);
                break;
        }

        /* Proceed to handle next record */
        len_tmp -= HtcRecHdr->Length;
        netdata += (/*sizeof(HTC_RECORD_HDR) + */ HtcRecHdr->Length);

    }

    /* remove received trailer */
    adf_nbuf_trim_tail(netbuf, trailerLen);

    return status;

}

hif_status_t HTCRxCompletionHandler(void *Context, adf_nbuf_t netbuf, a_uint8_t pipeID)
{
    hif_status_t status = HIF_OK;
    HTC_FRAME_HDR *HtcHdr;
    HTC_TARGET *target = (HTC_TARGET *)Context;
    //adf_os_handle_t os_hdl = target->os_handle;
    a_uint8_t *netdata;
    a_uint32_t netlen;
    a_uint8_t  fwRcvPattern = 0;

    adf_nbuf_peek_header(netbuf, &netdata, &netlen);
    //shliu: get correct whole nbuf length
    netlen = adf_nbuf_len(netbuf);

    HtcHdr = (HTC_FRAME_HDR *)netdata;

    if (HtcHdr->EndpointID >= ENDPOINT_MAX) {
        adf_os_print(" HTCRxCompletionHandler: invalid EndpointID=%d\n", HtcHdr->EndpointID);
        #ifdef NBUF_PREALLOC_POOL
        /* Do not free nbuf for pre-allocation case */
        if (pipeID == USB_REG_IN_PIPE) {
            adf_os_print("[%s] error EndID %d, received from INT pipe\n", __func__, HtcHdr->EndpointID);
        }
        #else
        adf_nbuf_free(netbuf);
        #endif
        return HIF_ERROR;
    }

    /* If the endpoint ID is 0, 
     * get the message ID and invoke the corresponding callbacks */
    if (HtcHdr->EndpointID == ENDPOINT0) {
    
        if (HtcHdr->Flags == 0) {
            HTC_UNKNOWN_MSG *HtcMsg = (HTC_UNKNOWN_MSG *)(netdata + sizeof(HTC_FRAME_HDR));
            
            adf_nbuf_pull_head(netbuf, sizeof(HTC_FRAME_HDR));  //RAY
            switch(adf_os_ntohs(HtcMsg->MessageID)) {
                case HTC_MSG_READY_ID:
                    HTCProcessTargetReady(target, netbuf, HtcHdr);
                    break;
                case HTC_MSG_CONNECT_SERVICE_RESPONSE_ID:
                    HTCProcessConnectionRsp(target, netbuf, HtcHdr);
                    break;
                case HTC_MSG_CONFIG_PIPE_RESPONSE_ID:
                    HTCProcessConfigPipeRsp(target, netbuf, HtcHdr);
                    break;
		    }
        }
        else if (HtcHdr->Flags & HTC_FLAGS_RECV_TRAILER) {
            HTC_TRAILER_PRINT(0);

            #ifdef HTC_TEST
            if (target->InitHtcTestInfo.Ep0CreditRptEvent) {
                target->InitHtcTestInfo.Ep0CreditRptEvent(target->host_handle, netbuf);

                /* Because Ep0CreditRptEvent will free the buffer, 
                 * skip the action to free buffer 
                 */
                goto out;
            }
            #else
            if (be32_to_cpu(*(a_uint32_t *)netdata) == 0x00c60000) {
                /* 
                 * Here is a WAR for 
                 * watchdog-pattern(garbage) + wait-target-ready(correct) 
                 * in the received frame
                 */

                //remove the four watchdog pattern bytes(00-C6-00-00)
			    adf_nbuf_pull_head(netbuf, 4);

                // re-peek the header again
			    adf_nbuf_peek_header(netbuf, &netdata, &netlen);
			    netlen = adf_nbuf_len(netbuf);

			    HtcHdr = (HTC_FRAME_HDR *)netdata;

                if (HtcHdr->EndpointID == ENDPOINT0) {
                    if (HtcHdr->Flags == 0) {
                        HTC_UNKNOWN_MSG *HtcMsg = (HTC_UNKNOWN_MSG *)(netdata + sizeof(HTC_FRAME_HDR));
                        
                        adf_nbuf_pull_head(netbuf, sizeof(HTC_FRAME_HDR));  //RAY
                        switch(adf_os_ntohs(HtcMsg->MessageID)) {
                            case HTC_MSG_READY_ID:
                                adf_os_print("%s_%d: Message for HTC ready\n", __FUNCTION__, __LINE__);
                                HTCProcessTargetReady(target, netbuf, HtcHdr);
                                break;
                            default:
                                adf_os_print("%s_%d: ##### Exception ! Need check ! #####\n", __FUNCTION__, __LINE__);
                                fwRcvPattern = 1;
                                break;
		                }
                    } else {
                        adf_os_print("%s_%d: ##### Exception ! Need check ! #####\n", __FUNCTION__, __LINE__);
                        fwRcvPattern = 1;
                    }
                } else {
                    adf_os_print("%s_%d: ##### Exception ! Need check ! #####\n", __FUNCTION__, __LINE__);
                    fwRcvPattern = 1;
                }
            } else {
                status = HTCProcessRxTrailer(target, netbuf, netdata, netlen, HtcHdr);
            }
            #endif
        }

        if ( fwRcvPattern == 1 ) {
            a_uint8_t      epId;
            HTC_ENDPOINT *pEndpoint;

            HIFEnableFwRcv(target->hif_dev);

            for ( epId=0; epId<ENDPOINT_MAX; epId++ ) {
                if ( target->EndPoint[epId].ServiceID == WMI_CONTROL_SVC ) {
                    adf_os_print("%s_%d: Find out the endpoint id is %d\n", __FUNCTION__, __LINE__, epId);
                    break;
                }
            }

            if ( epId != ENDPOINT_MAX )
                pEndpoint = &target->EndPoint[epId];
            else
                pEndpoint = NULL;

            if (pEndpoint && pEndpoint->EpCallBacks.EpRecv) {
                /* give the packet to the upper layer */
                pEndpoint->EpCallBacks.EpRecv(pEndpoint->EpCallBacks.pContext, netbuf, FWRecoverEpid);
            } else {
                adf_os_print("EpRecv callback is not registered! HTC is going to free the net buffer.\n");
                adf_nbuf_free(netbuf);
            }
        } else {
#ifndef NBUF_PREALLOC_POOL
    		/* !!!!!! free the buffer of HTC control messages */
    		adf_nbuf_free(netbuf);
#endif		
        }
    } else if (HtcHdr->EndpointID >= ENDPOINT_MAX) {
        adf_os_print("%s: Invalid EPID ! HTC is going to free the net buffer !\n", __func__);
        #ifdef NBUF_PREALLOC_POOL
        /* Do not free nbuf for pre-allocation case */
        if (pipeID == USB_REG_IN_PIPE) {
            adf_os_print("[%s] error EndID %d, received from INT pipe\n", __func__, HtcHdr->EndpointID);
        }
        #else
        adf_nbuf_free(netbuf);
        #endif
    } else { /* If the endpoint ID is not 0, invoke the registered EP's callback */
        if ( 0x33221199 == *((a_uint32_t *)netdata) ) {
            adf_os_print("\n%s_%d: Handle FW Crash Exception \n", __FUNCTION__, __LINE__);
            adf_os_print("%s_%d: Fatal exception (%d)", __FUNCTION__, __LINE__, *(((a_uint32_t *)netdata)+1));
            adf_os_print("%s_%d: pc       = 0x%8x\n", __FUNCTION__, __LINE__, *(((a_uint32_t *)netdata)+2));
            adf_os_print("%s_%d: badvaddr = 0x%8x\n\n", __FUNCTION__, __LINE__, *(((a_uint32_t *)netdata)+3));
            adf_nbuf_free(netbuf);
        } else if ( 0x33221299 == *((a_uint32_t *)netdata) ) {
            adf_os_print("\n%s_%d: HTC Header Endpoint ID = %d\n\n", __FUNCTION__, __LINE__, *(((a_uint32_t *)netdata)+1));
            adf_os_print("%s_%d, HTC Hdr Length = %d, netbuf length = %d\n", __FUNCTION__, __LINE__, HTC_HDR_LENGTH, netlen);
            adf_nbuf_free(netbuf);
        } else {
            do {
                HTC_ENDPOINT *pEndpoint = &target->EndPoint[HtcHdr->EndpointID];

                #ifdef HTC_TEST
                #else
                if (HtcHdr->Flags & HTC_FLAGS_RECV_TRAILER) {
                    status = HTCProcessRxTrailer(target, netbuf, netdata, netlen, HtcHdr);

                    if (A_FAILED(status)) {
                        status = A_EPROTO;
                        break;
                    }
                }

                /* remove HTC header */
                adf_nbuf_pull_head(netbuf, HTC_HDR_LENGTH);

                #endif

                if (pEndpoint->EpCallBacks.EpRecv) {
                    #ifdef HTC_TEST
                    pEndpoint->EpCallBacks.EpRecv(pEndpoint->EpCallBacks.pContext, netbuf, HtcHdr->EndpointID, pipeID);
                    #else
                    /* give the packet to the upper layer */
                    pEndpoint->EpCallBacks.EpRecv(pEndpoint->EpCallBacks.pContext, netbuf, HtcHdr->EndpointID);
                    #endif
                }
                else {
                    adf_os_print("EpRecv callback is not registered! HTC is going to free the net buffer.\n");
                    #ifdef NBUF_PREALLOC_POOL
                    /* Do not free nbuf for pre-allocation case */
                    if(pipeID == USB_REG_IN_PIPE) {
                        adf_os_print("[%s] EpRecv callback is not registered!, received from INT pipe\n",__func__);
                    }
                    #else
                    adf_nbuf_free(netbuf);
                    #endif
                }
            } while(FALSE);
        }
    }

#ifdef HTC_TEST
out:
#endif
	return ((status == 0) ? HIF_OK : HIF_ERROR);	
}

hif_status_t HTCUsbStopHandler(void *Context, a_uint8_t usb_status)
{
    hif_status_t status = HIF_OK;
    wmi_usb_stop(Context, usb_status);
    return status;
}

