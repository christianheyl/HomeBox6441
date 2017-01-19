/*
 * Copyright (c) 2011, Atheros Communications Inc.
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

/*
 * Host WMI unified implementation
 */
#include "athdefs.h"
#include "a_types.h"
#include "a_osapi.h"
#include "a_debug.h"
#include "ol_defines.h"
#include "ol_if_ath_api.h"
#include "ol_helper.h"
#include "htc_api.h"
#include "dbglog_host.h"
#include "wmi.h"
#include "wmi_unified_priv.h"
#include "htt.h"

#ifdef ATHR_WIN_NWF
#define ATH_MODULE_NAME wmi
#ifdef DEBUG
#define ATH_DEBUG_WMI  ATH_DEBUG_MAKE_MODULE_MASK(0)
static ATH_DEBUG_MASK_DESCRIPTION wmi_debug_desc[] = {
    { ATH_DEBUG_WMI , "WMI Tracing"},
};

ATH_DEBUG_INSTANTIATE_MODULE_VAR(wmi,
                                 "wmi",
                                 "Wireless Module Interface",
                                 ATH_DEBUG_MASK_DEFAULTS,
                                 ATH_DEBUG_DESCRIPTION_COUNT(wmi_debug_desc),
                                 wmi_debug_desc);

#endif
#endif

#if ATH_PERF_PWR_OFFLOAD


/* WMI buffer APIs */

wmi_buf_t
wmi_buf_alloc(wmi_unified_t wmi_handle, int len)
{
    wmi_buf_t wmi_buf;
    /* NOTE: For now the wbuf type is used as WBUF_TX_CTL
     * But this need to be changed appropriately to reserve
     * proper headroom for wmi_buffers
     */
    wmi_buf = wbuf_alloc(wmi_handle->osdev, WBUF_TX_CTL, len);
    if( NULL == wmi_buf )
    {
        /* wbuf_alloc returns NULL if the internel pool in wmi_handle->osdev
         * is empty
         */
        return NULL;
    }

    
    /* Clear the wmi buffer */
    OS_MEMZERO(wbuf_header(wmi_buf), len);
	
    /*
     * Set the length of the buffer to match the allocation size.
     */
    wbuf_set_pktlen(wmi_buf, len);
    return wmi_buf;
}

#define htt_host_data_dl_len(_osbuf)  (HTC_HDR_ALIGNMENT_PADDING + HTT_SW_MSDU_DESC_LEN + HTT_SDU_HDR_LEN)

/* WMI command API */
int
wmi_unified_cmd_send(wmi_unified_t wmi_handle, wmi_buf_t buf, int len, WMI_CMD_ID cmd_id)
{
    A_STATUS status;
    struct cookie *cookie; 

    if (wbuf_push(buf, sizeof(WMI_CMD_HDR)) == NULL) {
        return -ENOMEM;
    }

    WMI_SET_FIELD(wbuf_header(buf), WMI_CMD_HDR, COMMANDID, cmd_id);
    //WMI_CMD_HDR_SET_DEVID(cmd_hdr, 0); // unused

    cookie = ol_alloc_cookie(wmi_handle);
    if (!cookie) {
        return -ENOMEM;
    }

    cookie->PacketContext = buf;
    SET_HTC_PACKET_INFO_TX(&cookie->HtcPkt,
                           cookie,
                           wbuf_header(buf),
                           len+sizeof(WMI_CMD_HDR),
                           /* htt_host_data_dl_len(buf)+20 */
                           wmi_handle->wmi_endpoint_id,
                           0/*htc_tag*/);

    SET_HTC_PACKET_NET_BUF_CONTEXT(&cookie->HtcPkt, buf);


    status = HTCSendPkt(wmi_handle->htc_handle, &cookie->HtcPkt); 

    return ((status == A_OK) ? EOK : -1);
}


/* WMI Event handler register API */

int wmi_unified_register_event_handler(wmi_unified_t wmi_handle,
                                       WMI_EVT_ID event_id,
                                       wmi_unified_event_handler handler_func,
                                       void* cookie)
{
    u_int16_t handler_id;
    handler_id = (event_id - WMI_START_EVENTID);
    if (handler_id >= WMI_UNIFIED_MAX_EVENT) {
        printk("%s : unkown event id : 0x%x  event handler id 0x%x \n",
                __func__, event_id, handler_id);
        return -1;
    }
    if (wmi_handle->event_handler[handler_id]) {
        printk("%s : event handler is already registered: event id 0x%x \n",
                __func__, event_id);
        return -1;
    }
    wmi_handle->event_handler[handler_id] = handler_func;
    wmi_handle->event_handler_cookie[handler_id] = cookie;
    return 0;
}

int wmi_unified_unregister_event_handler(wmi_unified_t wmi_handle,
                                       WMI_EVT_ID event_id)
{
    u_int16_t handler_id;
    handler_id = (event_id - WMI_START_EVENTID);
    if (handler_id >= WMI_UNIFIED_MAX_EVENT) {
        printk("%s : unkown event id : 0x%x  event handler id 0x%x \n",
                __func__, event_id, handler_id);
        return -1;
    }
    if (wmi_handle->event_handler[handler_id] == NULL) {
        printk("%s : event handler is not registered: event id 0x%x \n",
                __func__, event_id);
        return -1;
    }
    wmi_handle->event_handler[handler_id] = NULL;
    wmi_handle->event_handler_cookie[handler_id] = NULL;
    return 0;
}


static int wmi_unified_event_rx(struct wmi_unified *wmi_handle, wmi_buf_t evt_buf)
{
    u_int16_t id;
    u_int8_t *event;
    u_int16_t len;
    int status = -1;
    u_int16_t handler_id;

    ASSERT(evt_buf != NULL);

    id = WMI_GET_FIELD(wbuf_header(evt_buf), WMI_CMD_HDR, COMMANDID);
    handler_id = (id - WMI_START_EVENTID);

    if (wbuf_pull(evt_buf, sizeof(WMI_CMD_HDR)) == NULL) {
        //A_DPRINTF(DBG_WMI, (DBGFMT "bad packet 1\n", DBGARG));
        //wmip->wmi_stats.cmd_len_err++;
        goto end;
    }
    if (handler_id >= WMI_UNIFIED_MAX_EVENT) {
        printk("%s : unkown event id : 0x%x  event handler id 0x%x \n", 
                __func__, id, handler_id);
        goto end;
    }
   
    event = wbuf_header(evt_buf);
    len = wbuf_get_pktlen(evt_buf);


    if (!wmi_handle->event_handler[handler_id]) {
        printk("%s : no registered event handler : event id 0x%x \n", 
                __func__, id);
        goto end;
    }


    /* Call the WMI registered event handler */
    status = wmi_handle->event_handler[handler_id](wmi_handle->scn_handle, event, len,
                                           wmi_handle->event_handler_cookie[handler_id]);

end:
    wbuf_free(evt_buf);
    return status;
}

static int
wmi_service_ready_event_rx(struct wmi_unified *wmi_handle, A_UINT8 *datap, int len)
{
    wmi_service_ready_event *ev = (wmi_service_ready_event *)datap;

    if (len < sizeof(wmi_service_ready_event)) {
        printk("%s:  WMI UNIFIED SERVICE READY event - invalid length \n", __func__);
        return A_EINVAL;
    }

    printk("%s:  WMI UNIFIED SERVICE READY event \n", __func__);
    ol_ath_service_ready_event(wmi_handle->scn_handle, ev);
    
    return A_OK;
}

static int
wmi_ready_event_rx(struct wmi_unified *wmi_handle, A_UINT8 *datap, int len)
{
    wmi_ready_event *ev = (wmi_ready_event *)datap;

    if (len < sizeof(wmi_ready_event)) {
        printk("%s:  WMI UNIFIED READY event - invalid length \n", __func__);
        return A_EINVAL;
    }

    printk("%s:  WMI UNIFIED READY event \n", __func__);
    ol_ath_ready_event(wmi_handle->scn_handle, ev);


    return A_OK;
}

/*
 * Temporarily added to support older WMI events. We should move all events to unified
 * when the target is ready to support it.
 */
void
wmi_control_rx(void *ctx, HTC_PACKET *htc_packet)
{
    u_int16_t id;
    u_int8_t *data;
    u_int32_t len;
    int status = EOK;
    wmi_buf_t evt_buf;
    struct wmi_unified *wmi_handle = (struct wmi_unified *) ctx;
 
    evt_buf =  (wmi_buf_t)  htc_packet->pPktContext;

    /** 
     * This is  a HACK due to a Hack/WAR in HTC !!.
     * the head of the wbuf still contains the HTC header
     * but the length excludes htc header.  
     */
    wbuf_set_pktlen(evt_buf, htc_packet->ActualLength +  HTC_HEADER_LEN);
    wbuf_pull(evt_buf, HTC_HEADER_LEN);

    id = WMI_GET_FIELD(wbuf_header(evt_buf), WMI_CMD_HDR, COMMANDID);

    if ((id >= WMI_START_EVENTID) && (id <= WMI_END_EVENTID)) {
        status = wmi_unified_event_rx(wmi_handle, evt_buf);
        return ;
    }

    if (wbuf_pull(evt_buf, sizeof(WMI_CMD_HDR)) == NULL) {
        status = -1;
        goto end;
    }

    data = wbuf_header(evt_buf);
    len = wbuf_get_pktlen(evt_buf);

    switch(id)
    {
        default:
            printk("%s: Unhandled WMI command %d\n", __func__, id);
            break;
        case WMI_SERVICE_READY_EVENTID:
            status = wmi_service_ready_event_rx(wmi_handle, data, len);
            break;
        case WMI_READY_EVENTID:
            status = wmi_ready_event_rx(wmi_handle, data, len);
            break;
        case WMI_WLAN_VERSION_EVENTID:
            printk("%s: Handle WMI_VERSION_EVENTID\n", __func__);
            break;
        case WMI_REGDOMAIN_EVENTID:
            printk("%s: Handle WMI_REGDOMAIN_EVENTID\n", __func__);
            break;
    }

end:
    wbuf_free(evt_buf);
}

/* WMI Initialization functions */

void *
wmi_unified_attach(ol_scn_t scn_handle, osdev_t osdev)
{
    struct wmi_unified *wmi_handle;
    wmi_handle = (struct wmi_unified *)OS_MALLOC(osdev, sizeof(struct wmi_unified), GFP_ATOMIC);
    if (wmi_handle == NULL) {
        printk("allocation of wmi handle failed %d \n",sizeof(struct wmi_unified));
        return NULL;
    }
    OS_MEMZERO(wmi_handle, sizeof(struct wmi_unified));
    wmi_handle->scn_handle = scn_handle;
    wmi_handle->osdev = osdev;
    return wmi_handle;
}

void
wmi_unified_detach(struct wmi_unified* wmi_handle)
{
    if (wmi_handle != NULL) {
        OS_FREE(wmi_handle);
        wmi_handle = NULL;
    }
}

void   wmi_htc_tx_complete(void *ctx,HTC_PACKET *htc_pkt)
{

    wmi_buf_t wmi_cmd_buf=GET_HTC_PACKET_NET_BUF_CONTEXT(htc_pkt);
    struct cookie *cookie = htc_pkt->pPktContext;

    ASSERT(wmi_cmd_buf);
    wbuf_free(wmi_cmd_buf);
    ol_free_cookie(ctx, cookie);
}

int
wmi_unified_connect_htc_service(struct wmi_unified * wmi_handle, void *htc_handle)
{

    int status;
    HTC_SERVICE_CONNECT_RESP response;
    HTC_SERVICE_CONNECT_REQ connect;

    OS_MEMZERO(&connect, sizeof(connect));
    OS_MEMZERO(&response, sizeof(response));

    /* meta data is unused for now */
    connect.pMetaData = NULL;
    connect.MetaDataLength = 0;
    /* these fields are the same for all service endpoints */
    connect.EpCallbacks.pContext = wmi_handle;
    connect.EpCallbacks.EpTxCompleteMultiple = NULL /* Control path completion ar6000_tx_complete */;
    connect.EpCallbacks.EpRecv = wmi_control_rx /* Control path rx */;
    connect.EpCallbacks.EpRecvRefill = NULL /* ar6000_rx_refill */;
    connect.EpCallbacks.EpSendFull = NULL /* ar6000_tx_queue_full */;
    connect.EpCallbacks.EpTxComplete = wmi_htc_tx_complete /* ar6000_tx_queue_full */;

    /* connect to control service */
    connect.ServiceID = WMI_CONTROL_SVC;

    if ((status = HTCConnectService(htc_handle, &connect, &response)) != EOK)
    {
        printk(" Failed to connect to WMI CONTROL  service status:%d \n",  status);
        return -1;;
    }
    wmi_handle->wmi_endpoint_id = response.Endpoint;
    wmi_handle->htc_handle = htc_handle;
    
    return EOK;
}
#endif
