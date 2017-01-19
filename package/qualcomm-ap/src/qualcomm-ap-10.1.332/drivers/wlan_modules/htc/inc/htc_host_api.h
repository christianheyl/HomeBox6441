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

#ifndef _HTC_HOST_API_H_
#define _HTC_HOST_API_H_

/**
 * @file htc_host_api.h
 * Host-Target-Communication API.
 * The target is modeled to have one or more "services". Each service uses an
 * "endpoint" to send messages to and fro. One or more endpoints is mapped to a
 * "pipe" which is a h/w transport. The number of pipes in the system is based
 * on the underlying interface (USB/PCI/PCIe/Ethernet/). Each pipe is setup
 * with "credits" which represent available target buffers.
 */ 

#include <htc.h>
#include <htc_services.h>
#include "adf_nbuf.h"
#include "hif.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Opaque HTC handle returned by HTCInit(..).
 */ 
typedef void *HTC_HANDLE;

/**
 * @brief Identifier for a target service.
 */ 
typedef a_uint16_t HTC_SERVICE_ID;

/**
 * @brief Callbacks for HTC_host_drv_register(..).
 */
typedef struct _HTC_host_switch_table_s {
    int    (*AddInstance)(osdev_t os_hdl, HTC_HANDLE handle);
    void   (*DeleteInstance)(void *Instance);
} HTC_host_switch_table_t;

#ifdef HTC_HOST_CREDIT_DIST

void host_htc_credit_update(void *dev, HTC_ENDPOINT_ID epid, a_uint32_t credits);
#define HTC_CREDIT_UPDATE_FN host_htc_credit_update

#else

#define HTC_CREDIT_UPDATE_FN NULL
#endif    

/**
 * @brief Callbacks for HTCInit(..).
 */ 
typedef struct _HTC_INIT_INFO {
    void   (*HTCSchedNextEvent)(void *Context, HTC_ENDPOINT_ID epid);
    void   (*HTCCreditUpdateEvent)(void *Context, HTC_ENDPOINT_ID epid, a_uint32_t credits);
} HTC_INIT_INFO;

/**
 * @brief per service connection send completion 
 */
//typedef void   (*HTC_EP_SEND_PKT_COMPLETE)(void *, adf_nbuf_t *, a_uint8_t);
typedef void   (*HTC_EP_SEND_PKT_COMPLETE)(void *, adf_nbuf_t, HTC_ENDPOINT_ID);

/**
 * @brief per service connection pkt received 
 */
typedef void   (*HTC_EP_RECV_PKT)(void *, adf_nbuf_t, HTC_ENDPOINT_ID);

/**
 * @brief Callbacks per endpoint.
 */ 
typedef struct _HTC_EP_CALLBACKS {
    void                     *pContext;     /**< context for each callback */
    HTC_EP_SEND_PKT_COMPLETE EpTxComplete;  /**< tx completion callback for connected endpoint */
    HTC_EP_RECV_PKT          EpRecv;        /**< receive callback for connected endpoint */
} HTC_EP_CALLBACKS;
 
/**
 * @brief service connection information 
 */
typedef struct _HTC_SERVICE_CONNECT_REQ {
    HTC_SERVICE_ID   ServiceID;                 /**< service ID to connect to */
    a_uint16_t         ConnectionFlags;           /**< connection flags, see htc protocol definition */
    a_uint8_t         *pMetaData;                 /**< ptr to optional service-specific meta-data */
    a_uint8_t          MetaDataLength;            /**< optional meta data length */
    HTC_EP_CALLBACKS EpCallbacks;               /**< endpoint callbacks */
    int              MaxSendQueueDepth;         /**< maximum depth of any send queue */
} HTC_SERVICE_CONNECT_REQ;

/**
 * @brief service connection response information 
 */
typedef struct _HTC_SERVICE_CONNECT_RESP {
    a_uint8_t     *pMetaData;             /**< caller supplied buffer to optional meta-data */
    a_uint8_t     BufferLength;           /**< length of caller supplied buffer */
    a_uint8_t     ActualLength;           /**< actual length of meta data */
    HTC_ENDPOINT_ID Endpoint;           /**< endpoint to communicate over */
    int         MaxMsgLength;           /**< max length of all messages over this endpoint */
    a_uint8_t     ConnectRespCode;        /**< connect response code from target */
} HTC_SERVICE_CONNECT_RESP;

/**
 * @brief endpoint credit distribution structure
 */
typedef struct _HTC_ENDPOINT_CREDIT_DIST {
    struct _HTC_ENDPOINT_CREDIT_DIST *pNext;
    struct _HTC_ENDPOINT_CREDIT_DIST *pPrev;
    HTC_SERVICE_ID      ServiceID;          /**< Service ID (set by HTC) */
    HTC_ENDPOINT_ID     Endpoint;           /**< endpoint for this distribution struct (set by HTC) */
    a_uint32_t            DistFlags;          /**< distribution flags, distribution function can
                                               set default activity using SET_EP_ACTIVE() macro */
    a_uint32_t            TxCreditsNorm;      /**< credits for normal operation, anything above this
                                               indicates the endpoint is over-subscribed, this field
                                               is only relevant to the credit distribution function */
    a_uint32_t            TxCreditsMin;       /**< floor for credit distribution, this field is
                                               only relevant to the credit distribution function */
    a_uint32_t            TxCreditsAssigned;  /**< number of credits assigned to this EP, this field 
                                               is only relevant to the credit dist function */
    a_uint32_t            TxCredits;          /**< current credits available, this field is used by
                                               HTC to determine whether a message can be sent or
                                               must be queued */
    a_uint32_t            TxCreditsToDist;    /**< pending credits to distribute on this endpoint, this
                                               is set by HTC when credit reports arrive.
                                               The credit distribution functions sets this to zero
                                               when it distributes the credits */  
    a_uint32_t            TxCreditsSeek;      /**< this is the number of credits that the current pending TX
                                               packet needs to transmit.  This is set by HTC when
                                               and endpoint needs credits in order to transmit */  
    a_uint32_t            TxCreditSize;       /**< size in bytes of each credit (set by HTC) */
    a_uint32_t            TxCreditsPerMaxMsg; /**< credits required for a maximum sized messages (set by HTC) */ 
    void                *pHTCReserved;      /**< reserved for HTC use */
} HTC_ENDPOINT_CREDIT_DIST;

#define HTC_EP_ACTIVE                            (1 << 31)

/**
 * @brief macro to check if an endpoint has gone active, useful for credit
 * distributions 
 */
#define IS_EP_ACTIVE(epDist)  ((epDist)->DistFlags & HTC_EP_ACTIVE)
#define SET_EP_ACTIVE(epDist) (epDist)->DistFlags |= HTC_EP_ACTIVE

/*
 * @brief credit distibution code that is passed into the distrbution function,
 * there are mandatory and optional codes that must be handled 
 */
typedef enum _HTC_CREDIT_DIST_REASON {
    HTC_CREDIT_DIST_SEND_COMPLETE = 0,     /**< credits available as a result of completed 
                                                send operations (MANDATORY) resulting in credit reports */
    HTC_CREDIT_DIST_ACTIVITY_CHANGE = 1,   /**< a change in endpoint activity occured (OPTIONAL) */
    HTC_CREDIT_DIST_SEEK_CREDITS,          /**< an endpoint needs to "seek" credits (OPTIONAL) */ 
    HTC_DUMP_CREDIT_STATE                  /**< for debugging, dump any state information that is kept by
                                                the distribution function */
} HTC_CREDIT_DIST_REASON;

typedef void (*HTC_CREDIT_DIST_CALLBACK)(void                     *Context, 
                                         HTC_ENDPOINT_CREDIT_DIST *pEPList,
                                         HTC_CREDIT_DIST_REASON   Reason);
                                         
typedef void (*HTC_CREDIT_INIT_CALLBACK)(void *Context,
                                         HTC_ENDPOINT_CREDIT_DIST *pEPList, 
                                         a_uint32_t                 TotalCredits);

/**
 * @brief endpoint statistics action 
 */
typedef enum _HTC_ENDPOINT_STAT_ACTION {
    HTC_EP_STAT_SAMPLE = 0,                /**< only read statistics */
    HTC_EP_STAT_SAMPLE_AND_CLEAR = 1,      /**< sample and immediately clear statistics */
    HTC_EP_STAT_CLEAR                      /**< clear only */
} HTC_ENDPOINT_STAT_ACTION;
 
/*
 * @brief endpoint statistics 
 */
typedef struct _HTC_ENDPOINT_STATS {
    a_uint32_t  TxCreditLowIndications; /**< number of times the host set the credit-low flag in a send message on 
                                            this endpoint */
    a_uint32_t  TxIssued;               /**< running count of TX packets issued */
    a_uint32_t  TxDropped;              /**< tx packets that were dropped */
    a_uint32_t  TxCreditRpts;           /**< running count of total credit reports received for this endpoint */
    a_uint32_t  TxCreditRptsFromRx;     /**< credit reports received from this endpoint's RX packets */
    a_uint32_t  TxCreditRptsFromOther;  /**< credit reports received from RX packets of other endpoints */
    a_uint32_t  TxCreditRptsFromEp0;    /**< credit reports received from endpoint 0 RX packets */
    a_uint32_t  TxCreditsFromRx;        /**< count of credits received via Rx packets on this endpoint */
    a_uint32_t  TxCreditsFromOther;     /**< count of credits received via another endpoint */
    a_uint32_t  TxCreditsFromEp0;       /**< count of credits received via another endpoint */
    a_uint32_t  TxCreditsConsummed;     /**< count of consummed credits */
    a_uint32_t  TxCreditsReturned;      /**< count of credits returned */
    a_uint32_t  RxReceived;             /**< count of RX packets received */
    a_uint32_t  RxLookAheads;           /**< count of lookahead records
                                         found in messages received on this endpoint */ 
} HTC_ENDPOINT_STATS;

typedef volatile a_uint32_t  htc_spin_t;

/* ------ Function Prototypes ------ */
A_STATUS HTC_host_drv_register(HTC_host_switch_table_t *sw);

/**
  @brief: Initialize HTC
  @param[in]:  host_handle - host driver specific handle, opague to HTC
               os_handle - platform dependent adf os handle, opague to HTC
               pInfo - initialization information
  @param[out]: 
  @return: A_OK on success
  @note: The caller initializes global HTC state and registers various instance
          notification callbacks (see HTC_INIT_INFO).
  @see: HTCShutdown()
*/
A_STATUS HTCInit(HTC_HANDLE hHTC, void *host_handle, adf_os_handle_t os_handle, HTC_INIT_INFO *pInfo);

/**
  @brief: Start target service communications
  @param[in]:  HTCHandle - HTC handle
  @param[out]: 
  @return: 
  @note: This API indicates to the target that the service connection phase is complete
         and the target can freely start all connected services.  This API should only be
         called AFTER all service connections have been made.  TCStart will issue a
         SETUP_COMPLETE message to the target to indicate that all service connections 
         have been made and the target can start communicating over the endpoints.
  @see: HTCConnectService()
*/                                                                     
A_STATUS    HTCStart(HTC_HANDLE HTCHandle);

/**
  @brief: Connect to an HTC service synchronously
  @param[in]:  HTCHandle - HTC handle
               pReq - connection details
               pConn_rsp_epid - endpoint to communicate over
  @return: 
  @note:  Service connections must be performed before HTCStart.  User provides callback handlers
          for various endpoint events.
  @see: HTCStart
*/  
A_STATUS    HTCConnectService(HTC_HANDLE HTCHandle, 
                              HTC_SERVICE_CONNECT_REQ  *pReq,
                              HTC_ENDPOINT_ID *pConn_rsp_epid);

/**
  @brief: Shutdown HTC
  @param[in]: 
  @param[out]: 
  @return: 
  @note:  This cleans up all resources allocated by HTCInit().
  @see: HTCInit()
*/ 
void        HTCShutDown(HTC_HANDLE HTCHandle);

/**
 * @brief: Configure a pipe with credits.
 * @param[in]: hHTC - HTC Handle
 *             pipeID - identifies the pipe
 *             credits - number of credits (target buffers).
 * @param[out]:
 * @return: Status of operation
 */ 
A_STATUS HTCConfigPipeCredits(HTC_HANDLE hHTC, a_uint8_t pipeID, a_uint8_t credits);

/**
 * @brief: Configures the pipes for endpoint 0 (reserved for boot)
 * @param[in]: hHTC - HTC Handle
 *             ULPipeID - Pipe to use for uplink
 *             DLPipeID - Pipe to use for downlink
 * @param[out]:
 * @return: Status of operation
 */             
A_STATUS HTCSetDefaultPipe(HTC_HANDLE hHTC, a_uint8_t ULPipeID, a_uint8_t DLPipeID);

void HTCTxComp(HTC_HANDLE hHTC,
                    adf_nbuf_t buf,
                    HTC_ENDPOINT_ID Ep);
/**
 * @brief: Send a packet to a service using an endpoint.
 * @param[in]: hHTC - HTC handle
 *             hdr_buf - tx header net buffer if necessary (i.e. headroom of original buf is not 
 *                       enough for tx header)
 *             buf - net buffer
 *             Ep - endpoint
 * @param[out]: 
 * @return: Status of send operation
 */            
A_STATUS HTCSendPkt(HTC_HANDLE hHTC,
                    adf_nbuf_t hdr_buf,
                    adf_nbuf_t buf,
                    HTC_ENDPOINT_ID Ep);

/**
 * @brief: Try to send packets still queued in the host HTC layer.
 * @param[in]: hHTC - HTC handle
 * @param[out]:
 * @return: Status of send operation
 */
A_STATUS HTCTrySendAll(HTC_HANDLE hHTC);

void HTCTxEpUpdate(HTC_HANDLE       hHTC, 
				   HTC_ENDPOINT_ID  Ep,
				   a_uint32_t	        aifs, 
				   a_uint32_t	        cwmin, 
				   a_uint32_t	        cwmax);

/**
 * @brief: Get the underlying HIF device handle
 * @param[in]: HTCHandle - HTC handle returned by HTCInit(..)
 * @param[out]: 
 * @return: opaque HIF device handle
 */
HIF_HANDLE HTCGetHIFHandle(HTC_HANDLE HTCHandle);

/**
 * @brief: Set credit distribution parameters
 * @param[in]: HTCHandle - HTC handle
 *             pCreditDistCont - caller supplied context to pass into distribution functions
 *             CreditDistFunc - Distribution function callback
 *             CreditDistInit - Credit Distribution initialization callback
 *             ServicePriorityOrder - Array containing list of service IDs, lowest index is 
 *                                    highest priority
 *             ListLength - number of elements in ServicePriorityOrder
 * @param[out]: 
 * @return: 
 * @note: The user can set a custom credit distribution function to handle special requirements
 *        for each endpoint.  A default credit distribution routine can be used by setting
 *        CreditInitFunc to NULL.  The default credit distribution is only provided for simple
 *        "fair" credit distribution without regard to any prioritization.
 */
void HTCSetCreditDistribution(HTC_HANDLE               HTCHandle,
                              void                     *pCreditDistContext,
                              HTC_CREDIT_DIST_CALLBACK CreditDistFunc,
                              HTC_CREDIT_INIT_CALLBACK CreditInitFunc,
                              HTC_SERVICE_ID           ServicePriorityOrder[],
                              a_uint32_t                 ListLength);

/**
 * @brief: Query queue depth (hardware WLAN queue for single cpu, htc endpoint queue 
 *         for multiple cpu)
 * @param[in]: HTCHandle - HTC handle returned by HTCInit(..)
 *             Ep - endpoint
 * @param[out]: 
 * @return: Number of queue depth
 */
a_uint32_t HTCQueryQueueDepth(HTC_HANDLE HTCHandle, HTC_ENDPOINT_ID Ep);

A_BOOL
HTC_busy(HTC_HANDLE HTCHandle, HTC_ENDPOINT_ID Ep, a_uint32_t nPkts);

void
HTCDrainEp(HTC_HANDLE hHTC, HTC_ENDPOINT_ID Ep);

void
HTCDrainAllEp(HTC_HANDLE hHTC);

A_STATUS HTCWaitForHtcRdy(HTC_HANDLE HTCHandle);

#ifdef __linux__
#include <adf_os_time.h>

enum htc_wait_t{
    A_HTC_WAIT_DONE,
    A_HTC_WAIT,
    A_TGT_DETACH
};

static inline void
htc_spin_prep(htc_spin_t  *condp)
{
    *condp = A_HTC_WAIT;
}

#define HTC_WAITING_TIMEOUT 1000
static inline int
htc_spin(htc_spin_t  *condp)
{
    adf_os_time_t start_time = 0, end_time = 0;

    start_time = OS_GET_TICKS();
    do {
        end_time = OS_GET_TICKS();
        if( (CONVERT_SYSTEM_TIME_TO_MS(end_time - start_time)) > HTC_WAITING_TIMEOUT) {
            return A_ERROR;
    }
    }while ( *condp == A_HTC_WAIT ); 
    
    if( *condp == A_TGT_DETACH)
        return A_ERROR;
    else
        return A_OK;
}
static inline void
htc_spin_over(htc_spin_t  *condp)
{
    *condp = A_HTC_WAIT_DONE;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _HTC_HOST_API_H_ */
