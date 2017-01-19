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
#include "ol_if_athvar.h"
//define the aligned (32) size for each chain mask and BW
#define TONE_LEGACY_20M_1chain_ALIGN 128
#define TONE_LEGACY_20M_2chain_ALIGN 224
#define TONE_LEGACY_20M_3chain_ALIGN 320
#define TONE_VHT_20M_1chain_ALIGN 128
#define TONE_VHT_20M_2chain_ALIGN 224
#define TONE_VHT_20M_3chain_ALIGN 352
#define TONE_VHT_40M_1chain_ALIGN 352
#define TONE_VHT_40M_2chain_ALIGN 672
#define TONE_VHT_40M_3chain_ALIGN 1024
#define TONE_VHT_80M_1chain_ALIGN 512
#define TONE_VHT_80M_2chain_ALIGN 992
#define TONE_VHT_80M_3chain_ALIGN 1472

u_int16_t bw_size [4][3] = 
{
  {TONE_LEGACY_20M_1chain_ALIGN, TONE_LEGACY_20M_2chain_ALIGN,TONE_LEGACY_20M_3chain_ALIGN},
  {TONE_VHT_20M_1chain_ALIGN, TONE_VHT_20M_2chain_ALIGN, TONE_VHT_20M_3chain_ALIGN},
  {TONE_VHT_40M_1chain_ALIGN, TONE_VHT_40M_2chain_ALIGN, TONE_VHT_40M_3chain_ALIGN},
  {TONE_VHT_80M_1chain_ALIGN, TONE_VHT_80M_2chain_ALIGN, TONE_VHT_80M_3chain_ALIGN}
};

/*
 * Define the sting for each RTT request type
 * for debug print purpose only  
 */

char* error_indicator[] = {
    "RTT_COMMAND_HEADER_ERROR",
    "RTT_COMMAND_ERROR",
    "RTT_MODULE_BUSY",
    "RTT_TOO_MANY_STA",
    "RTT_NO_RESOURCE",
    "RTT_VDEV_ERROR",
    "RTT_TRANSIMISSION_ERROR",
    "RTT_TM_TIMER_EXPIRE",
    "RTT_FRAME_TYPE_NOSUPPORT",
    "RTT_TIMER_EXPIRE"
};
#define RTT_NULL_FRAME 0
#define RTT_QOSNULL_FRAME (1 << 2)
#define RTT_TM_FRAME (2 << 2)
#define RTT_PEER_TM (1 << 13)

/* 
 * Calculate how many chains in a mask
 * mask -- chain mask  return: Number of chains
 */
static u_int8_t rtt_get_chain_no(u_int16_t mask) {
    u_int8_t counter = 0;
    
    while (mask > 0) {
        if(mask & 1) counter++;
        mask = mask >> 1;
    }
    return counter;
}

/*
 * print out the rtt measurement response on host
 * for debug purpose
 */
void rtt_print_resp_header(wmi_rtt_event_hdr *header) 
{//print event header

    u_int8_t dest_mac[6];
    adf_os_print("%s:", __func__);

    adf_os_print("\n==================Measurement Report====================\n");
    adf_os_print("Request ID is:0x%x\n", header->req_id);
    adf_os_print("Request result is:0x%x\n", header->result);
    WMI_MAC_ADDR_TO_CHAR_ARRAY(&header->dest_mac, dest_mac);
    adf_os_print("Request dest_mac is:%x%x%x%x%x%x\n", dest_mac[0],dest_mac[1],dest_mac[2],dest_mac[3],dest_mac[4],dest_mac[5]);
}

void rtt_print_meas_resp_body(wmi_rtt_meas_event *body)
{
    u_int8_t mask;
    u_int8_t bw;
    u_int8_t *p;
    u_int8_t index, index1;
    u_int16_t size;

    adf_os_print("%s:", __func__);
    if(body) {
        mask = body->rx_chain & 0xf;
        adf_os_print("Rx chain mask is:0x%x\n",mask);
       //calculate how many chains in mask
        bw = (body->rx_chain & 0x30) >> 4;
        adf_os_print("Rx bw is:%d\n",bw);

        adf_os_print("tod (10 ns) is:%d\n", body->tod);
        adf_os_print("toa (10 ns) is:%d\n", body->toa);
 
        p =(u_int8_t *) (++body);
        for (index = 0; index <4; index++ ) {
	    if(mask & (1 << index)) {
	        adf_os_print("RSSI of chain index %d is:%d\n",index, *((u_int32_t *)p));
	        p += sizeof(u_int32_t);
                size = bw_size[bw][rtt_get_chain_no(mask)];
                for(index1 = 0; index1 < size; index1++) {
		    if(*p != 0x88) {
		        adf_os_print("Channel dump is wrong in chain index %d", index);
                        return;
		    }
                    p++;
	        }
                adf_os_print("Channel dump is correct in chain index %d\n", index);
	    }
        }
    } else {
        adf_os_print("Error! Null Point!\n") ;  
    }
    return;
}

/*
 * event handler for  RTT measurement response
 * data  -- rtt measurement response from fw
 */
static int
wmi_rtt_meas_report_event_handler(ol_scn_t scn, u_int8_t *data,
				    u_int16_t datalen, void *context) 
{
    adf_os_print("%s: data=%p, datalen=%d\n", __func__, data, datalen);
    if (!data) {
        adf_os_print("Get NULL point message from FW\n");
        return -1;
    }

    rtt_print_resp_header((wmi_rtt_event_hdr *)data);
    data += sizeof(wmi_rtt_event_hdr);
    rtt_print_meas_resp_body((wmi_rtt_meas_event *)data);
    return 0;
}

/*
 * event handler for TSF measurement response
 * data  -- TSF measurement response from fw
 */
static int
wmi_tsf_meas_report_event_handle(ol_scn_t scn, u_int8_t *data,
				    u_int16_t datalen, void *context) 
{
  //ToDo
  return 0;

}

/*
 * event handler for RTT Error report
 * data  -- rtt measurement response from fw
 */
static int
wmi_error_report_event_handle(ol_scn_t scn, u_int8_t *data,
				    u_int16_t datalen, void *context) 
{
    adf_os_print("%s: data=%p, datalen=%d\n", __func__, data, datalen);
    if (!data) {
        adf_os_print("Get NULL point message from FW\n");
        return -1;
    }
    rtt_print_resp_header((wmi_rtt_event_hdr *)data);
    data += sizeof(wmi_rtt_event_hdr);
    adf_os_print("%s\n",error_indicator[*((WMI_RTT_ERROR_INDICATOR *)data)] );
    return 1;
}


/*
 * RTT measurement response handler attach functions for offload solutions
 */
void
ol_ath_rtt_meas_report_attach(struct ieee80211com *ic)
{
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);

    adf_os_print("%s: called\n", __func__);

    /* Register WMI event handlers */
    wmi_unified_register_event_handler(scn->wmi_handle,
        WMI_RTT_MEASUREMENT_REPORT_EVENTID,
        wmi_rtt_meas_report_event_handler,
        NULL);

    wmi_unified_register_event_handler(scn->wmi_handle,
        WMI_TSF_MEASUREMENT_REPORT_EVENTID,
        wmi_tsf_meas_report_event_handle,
        NULL);

    wmi_unified_register_event_handler(scn->wmi_handle,
        WMI_RTT_ERROR_REPORT_EVENTID,
        wmi_error_report_event_handle,
        NULL);
    return;
}

/*
 * Send RTT measurement Command to FW (for test purpose only)
 * here we encode two STA request
 * first will trigger error report
 * second will trigger RTT measurement report
 */
void
ol_ath_rtt_meas_req(wmi_unified_t  wmi_handle)
{
  //struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(ic);
    wmi_buf_t buf;
    u_int8_t *p;
    int len;
    u_int8_t peer[6];  //ap
    u_int8_t spoof[6];
    wmi_rtt_measreq_head *head;
    wmi_rtt_measreq_body *body;

    adf_os_print("%s:", __func__);
    //Temporarily, hardcoding peer mac address for test purpose
    //will be removed once RTT host has been developed
    
    len = sizeof(wmi_rtt_measreq_head) + 2 * sizeof(wmi_rtt_measreq_body);
    buf = wmi_buf_alloc(wmi_handle, len);
    A_ASSERT(buf != NULL);
    p = (u_int8_t *) wmi_buf_data(buf);

    //encode header
    head = (wmi_rtt_measreq_head *) p;
    head->req_id = 0x8f; 
    head->sta_num = 5;
    p = p + sizeof(wmi_rtt_measreq_head);
    //ToDo channel info for switch
    
    //encode first body
    body = (wmi_rtt_measreq_body *) p;
    body->control_flag = 0;
    body->control_flag |=  RTT_PEER_TM; //default is NULL_FRAME
    peer[5] = 0x34;
    peer[4] = 0x56;
    peer[3] = 0x12;
    peer[2] = 0x7f;
    peer[1] = 0x03;
    peer[0] = 0x00;
    WMI_CHAR_ARRAY_TO_MAC_ADDR(((u_int8_t *)peer),&body->dest_mac) ;
    memset(spoof, 0 , 6);
    WMI_CHAR_ARRAY_TO_MAC_ADDR(((u_int8_t *)spoof),&body->spoof_bssid) ;
    body->vdev_id = 0;
    body->num_meas = 3;
    //rate control is not support now
    //ToDo with Ashish
    body->trans_rate = 0x0c;
    body->retry = 10;
    body->time_out = 80;
    body->report_type = 0;
    body++;
    //encode second body
    memcpy(body, p, sizeof(wmi_rtt_measreq_body));
    peer[5] = 0x38;
    peer[4] = 0x56;
    peer[3] = 0x12;
    peer[2] = 0x7f;
    peer[1] = 0x03;
    peer[0] = 0x00;
    WMI_CHAR_ARRAY_TO_MAC_ADDR(((A_UINT8 *)peer),&body->dest_mac) ;
    body->num_meas = 1;
    wmi_unified_cmd_send(wmi_handle, buf, len, WMI_RTT_MEASREQ_CMDID);
    printk("send rtt cmd to FW\n");
}
