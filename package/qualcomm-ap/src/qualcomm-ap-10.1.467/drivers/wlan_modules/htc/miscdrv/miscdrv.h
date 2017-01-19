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

#ifndef _MISCDRV_H_
#define _MISCDRV_H_

/* Structure that is the state information for the default credit distribution callback.
 * Drivers should instantiate (zero-init as well) this structure in their driver instance
 * and pass it as a context to the HTC credit distribution functions */
typedef struct _COMMON_CREDIT_STATE_INFO {
    a_uint32_t TotalAvailableCredits;      /* total credits in the system at startup */
    a_uint32_t CurrentFreeCredits;         /* credits available in the pool that have not been
                                            given out to endpoints */
    HTC_ENDPOINT_CREDIT_DIST *pLowestPriEpDist;  /* pointer to the lowest priority endpoint dist struct */
} COMMON_CREDIT_STATE_INFO;
#ifdef ATH_SUPPORT_HTC
#ifdef HTC_HOST_CREDIT_DIST
A_STATUS HTCSetupCreditDist(HTC_HANDLE HTCHandle, void  *pCredInfo);

#define HTC_CRDIT_DIST_INIT(target) \
    adf_os_assert(target->InitCredits != NULL);  \
    adf_os_assert(target->EpCreditDistributionListHead != NULL); \
    adf_os_assert(target->EpCreditDistributionListHead->pNext != NULL);  \
    target->InitCredits(target->pCredDistContext, \
            target->EpCreditDistributionListHead->pNext, \
            target->TargetCredits); \
   OS_INIT_TIMER(target->os_handle, &target->host_htc_credit_debug_timer, host_htc_credit_show, target);
   /*OS_SET_TIMER(&target->host_htc_credit_debug_timer, HTC_CREDIT_TIMER_VAL);*/ 

#define  HTC_CREDIT_SHOW_TIMER_CANCEL(_timerhandle)    OS_CANCEL_TIMER(_timerhandle)

#define HTC_CREDIT_SHOW_TIMER_START(_timerhandle) \
    OS_SET_TIMER(_timerhandle, HTC_CREDIT_TIMER_VAL);
#define  HTC_ADD_CREDIT_SEQNO(_target,_pEndpoint,_len,_HtcHdr)   \
    { \
    a_uint8_t creditsused ,remainder ; \
    creditsused = (_len / _target->TargetCreditSize); \
    remainder   = (_len % _target->TargetCreditSize); \
    if (remainder) {  creditsused++;    } \
    HTC_SEQADD(_pEndpoint->EpSeqNum,creditsused,HTC_SEQ_MAX); \
    _HtcHdr->HostSeqNum = adf_os_htons(_pEndpoint->EpSeqNum); \
    }
 

#else
#define HTC_CRDIT_DIST_INIT(target) 
#define HTCSetupCreditDist(HTCHandle,pCredInfo) 
#define  HTC_ADD_CREDIT_SEQNO(_target,_pEndpoint,_len,_HtcHdr)   
#define  HTC_CREDIT_SHOW_TIMER_CANCEL(_timerhandle)  
#define HTC_CREDIT_SHOW_TIMER_START(_timerhandle) 

#endif


#else

#define HTC_CRDIT_DIST_INIT(target) 
#define HTCSetupCreditDist(HTCHandle,pCredInfo) 
#define  HTC_ADD_CREDIT_SEQNO(_target,_pEndpoint,_len,_HtcHdr)   
#define  HTC_CREDIT_SHOW_TIMER_CANCEL(_timerhandle)  
#define HTC_CREDIT_SHOW_TIMER_START(_timerhandle) 

#endif


#endif
