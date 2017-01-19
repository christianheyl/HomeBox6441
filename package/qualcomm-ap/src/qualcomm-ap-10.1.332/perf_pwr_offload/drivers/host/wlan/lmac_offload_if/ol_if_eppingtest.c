#include <osdep.h>
#include <wbuf.h>
#include "sw_version.h"
#include "ol_if_athvar.h"
#include "if_athioctl.h"
#include "osif_private.h"
#include "ol_ath.h"
#include "a_debug.h"
#include "ol_helper.h"
#include "epping_test.h"

static void epping_tx_complete_multiple(void *ctx,
        HTC_PACKET_QUEUE *pPacketQueue)
{
#ifdef EPPING_TEST
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)ctx;
    struct net_device* dev = scn->sc_osdev->netdev;
    A_STATUS status;
    HTC_ENDPOINT_ID eid;
    struct sk_buff *pktSkb;
    struct cookie *cookie;
    A_BOOL flushing = FALSE;
    struct sk_buff_head skb_queue;
    HTC_PACKET *htc_pkt;

    skb_queue_head_init(&skb_queue);

    adf_os_spin_lock_bh(&scn->data_lock);

    while (!HTC_QUEUE_EMPTY(pPacketQueue)) {
        htc_pkt = HTC_PACKET_DEQUEUE(pPacketQueue);

        status=htc_pkt->Status;
        eid=htc_pkt->Endpoint;
        pktSkb=GET_HTC_PACKET_NET_BUF_CONTEXT(htc_pkt);
        cookie = htc_pkt->pPktContext;

        ASSERT(pktSkb);
        ASSERT(htc_pkt->pBuffer == A_NETBUF_DATA(pktSkb));

        /* add this to the list, use faster non-lock API */
        __skb_queue_tail(&skb_queue,pktSkb);

        if (A_SUCCESS(status)) {
            ASSERT(htc_pkt->ActualLength == A_NETBUF_LEN(pktSkb));
        }
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("%s skb=0x%x data=0x%x len=0x%x eid=%d ",
                        __FUNCTION__, (A_UINT32)pktSkb, (A_UINT32)htc_pkt->pBuffer,
                        htc_pkt->ActualLength, eid));

        if (A_FAILED(status)) {
            if (status == A_ECANCELED) {
                /* a packet was flushed  */
                flushing = TRUE;
            }
            if (status != A_NO_RESOURCE) {
                printk("%s() -TX ERROR, status: 0x%x\n", __func__,
                        status);
            }
        } else {
            AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("OK\n"));
            flushing = FALSE;
        }

        ol_free_cookie(ctx, cookie);
    }

    adf_os_spin_unlock_bh(&scn->data_lock);

    /* free all skbs in our local list */
    while (!skb_queue_empty(&skb_queue)) {
        /* use non-lock version */
        pktSkb = __skb_dequeue(&skb_queue);
        A_NETBUF_FREE(pktSkb);
    }

    if (!flushing) {
        netif_wake_queue(dev);
    }
#endif
}

static void epping_tx_complete(void *ctx, HTC_PACKET *htc_pkt)
{
#ifdef EPPING_TEST
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)ctx;
    struct net_device* dev = scn->sc_osdev->netdev;
    A_STATUS status=htc_pkt->Status;
    HTC_ENDPOINT_ID eid=htc_pkt->Endpoint;
    struct sk_buff *pktSkb=GET_HTC_PACKET_NET_BUF_CONTEXT(htc_pkt);
    struct cookie *cookie = htc_pkt->pPktContext;
    A_BOOL flushing;

    ASSERT(pktSkb);
    if (A_SUCCESS(status)) {
        ASSERT(htc_pkt->ActualLength == A_NETBUF_LEN(pktSkb));
    }
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("%s skb=0x%x data=0x%x len=0x%x eid=%d ",
                    __FUNCTION__, (A_UINT32)pktSkb, (A_UINT32)htc_pkt->pBuffer,
                    htc_pkt->ActualLength, eid));

    if (A_FAILED(status)) {
        if (status == A_ECANCELED) {
            /* a packet was flushed  */
            flushing = TRUE;
        }
        if (status != A_NO_RESOURCE) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, ("%s() -TX ERROR, status: 0x%x\n", __func__,
                            status));
        }
    } else {
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("OK\n"));
        flushing = FALSE;
    }

    ol_free_cookie(ctx, cookie);
    A_NETBUF_FREE(pktSkb);
    if (!flushing) {
        netif_wake_queue(dev);
    }
#endif
}

#define AR6000_MAX_RX_BUFFERS 16
#define AR6000_BUFFER_SIZE 1664

static void epping_refill(void *Context, HTC_ENDPOINT_ID Endpoint)
{
#ifdef EPPING_TEST
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)Context;
    void *osBuf;
    int RxBuffers;
    int buffersToRefill;
    HTC_PACKET *pPacket;
    HTC_PACKET_QUEUE queue;

    buffersToRefill = (int)AR6000_MAX_RX_BUFFERS -
    HTCGetNumRecvBuffers(scn->htc_handle, Endpoint);

    if (buffersToRefill <= 0) {
        /* fast return, nothing to fill */
        return;
    }

    INIT_HTC_PACKET_QUEUE(&queue);

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("ar6000_rx_refill: providing htc with %d buffers at eid=%d\n",
                    buffersToRefill, Endpoint));

    for (RxBuffers = 0; RxBuffers < buffersToRefill; RxBuffers++) {
        osBuf = A_NETBUF_ALLOC(AR6000_BUFFER_SIZE);
        if (NULL == osBuf) {
            break;
        }
        /* the HTC packet wrapper is at the head of the reserved area
         * in the skb */
        pPacket = (HTC_PACKET *)(A_NETBUF_HEAD(osBuf));
        /* set re-fill info */
        SET_HTC_PACKET_INFO_RX_REFILL(pPacket,osBuf,A_NETBUF_DATA(osBuf),AR6000_BUFFER_SIZE,Endpoint);
        SET_HTC_PACKET_NET_BUF_CONTEXT(pPacket,osBuf);
        /* add to queue */
        HTC_PACKET_ENQUEUE(&queue,pPacket);
    }

    if (!HTC_QUEUE_EMPTY(&queue)) {
        /* add packets */
        HTCAddReceivePktMultiple(scn->htc_handle, &queue);
    }
#endif
}

static HTC_SEND_FULL_ACTION epping_tx_queue_full(void *Context,
        HTC_PACKET *pPacket)
{
#ifdef EPPING_TEST
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)Context;
    HTC_SEND_FULL_ACTION action = HTC_SEND_FULL_KEEP;
    netif_stop_queue(scn->sc_osdev->netdev);
    return action;
#else
    return HTC_SEND_FULL_DROP;
#endif
}

static void epping_rx(void *Context, HTC_PACKET *pPacket)
{
#ifdef EPPING_TEST
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)Context;
    struct net_device* dev = scn->sc_osdev->netdev;
    A_STATUS status = pPacket->Status;
    HTC_ENDPOINT_ID eid = pPacket->Endpoint;
    struct sk_buff *pktSkb = (struct sk_buff *)pPacket->pPktContext;

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("%s scn=0x%x eid=%d, skb=0x%x, data=0x%x, len=0x%x status:%d",
                    __FUNCTION__, (A_UINT32) scn, eid, (A_UINT32) pktSkb, (A_UINT32) pPacket->pBuffer,
                    pPacket->ActualLength, status));

    if (status != A_OK) {
        if (status != A_ECANCELED) {
            printk("RX ERR (%d) \n", status);
        }
        A_NETBUF_FREE(pktSkb);
        return;
    }

    // deliver to up layer
    if (pktSkb)
    {

        A_NETBUF_PUT(pktSkb, pPacket->ActualLength + HTC_HEADER_LEN);
        A_NETBUF_PULL(pktSkb, HTC_HEADER_LEN);

        if (EPPING_ALIGNMENT_PAD > 0) {
            A_NETBUF_PULL(pktSkb, EPPING_ALIGNMENT_PAD);
        }

        pktSkb->dev = dev;
        if ((pktSkb->dev->flags & IFF_UP) == IFF_UP) {
            pktSkb->protocol = eth_type_trans(pktSkb, pktSkb->dev);
            A_NETIF_RX_NI(pktSkb);
        } else {
            A_NETBUF_FREE(pktSkb);
        }
    }
#endif
}

A_STATUS epping_connect_service(struct ol_ath_softc_net80211 *scn)
{
#ifdef EPPING_TEST
    // setup something related to epping
    int status;
    HTC_SERVICE_CONNECT_REQ connect;
    HTC_SERVICE_CONNECT_RESP response;

    OS_MEMZERO(&connect, sizeof(connect));
    OS_MEMZERO(&response, sizeof(response));

    /* these fields are the same for all service endpoints */
    connect.EpCallbacks.pContext = scn;
    connect.EpCallbacks.EpTxCompleteMultiple = epping_tx_complete_multiple;
    connect.EpCallbacks.EpRecv = epping_rx /* rx */;
    connect.EpCallbacks.EpTxComplete =
            NULL /* epping_tx_complete use Multiple version */;
    connect.MaxSendQueueDepth = 64;

#ifdef HIF_SDIO
    connect.EpCallbacks.EpRecvRefill = epping_refill;
    connect.EpCallbacks.EpSendFull =
            epping_tx_queue_full /* ar6000_tx_queue_full */;
#elif defined(HIF_USB) || defined(HIF_PCI)
    connect.EpCallbacks.EpRecvRefill = NULL /* provided by HIF */;
    connect.EpCallbacks.EpSendFull = NULL /* provided by HIF */;
    /* disable flow control for hw flow control */
    connect.ConnectionFlags |= HTC_CONNECT_FLAGS_DISABLE_CREDIT_FLOW_CTRL;
#endif

    /* connect to service */
    connect.ServiceID = WMI_DATA_BE_SVC;
    if ((status = HTCConnectService(scn->htc_handle, &connect, &response))
            != EOK) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
                ("Failed to connect to Endpoint Ping BE service status:%d \n", status));
        return -1;;
    } else {
        AR_DEBUG_PRINTF(ATH_DEBUG_WARN,
                ("eppingtest BE endpoint:%d\n", response.Endpoint));
    }
    scn->EppingEndpoint[0] = response.Endpoint;

#if defined(HIF_PCI)
    connect.ServiceID = WMI_DATA_BK_SVC;
    if ((status = HTCConnectService(scn->htc_handle, &connect, &response)) != EOK)
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
                ("Failed to connect to Endpoint Ping BK service status:%d \n", status));
        return -1;;
    } else {
        AR_DEBUG_PRINTF(ATH_DEBUG_WARN,
                ("eppingtest BK endpoint:%d\n", response.Endpoint));
    }
    scn->EppingEndpoint[1] = response.Endpoint;
#endif
    return status;
#else
    return EOK;
#endif
}

