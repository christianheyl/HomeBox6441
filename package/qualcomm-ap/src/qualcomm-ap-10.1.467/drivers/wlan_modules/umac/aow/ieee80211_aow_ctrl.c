/*
 * Copyright (c) 2013 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

/*
 * =====================================================================================
 *
 *       Filename:  ieee80211_aow_ctrl.c
 *
 *    Description:  AoW control interface related functions
 *
 *        Version:  1.0
 *        Created:  10/08/2010 11:59:36 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  S.Karthikeyan (), 
 *        Company:  Atheros Communication
 *
 * =====================================================================================
 */

#include <ieee80211_var.h>
#include <ieee80211_aow.h>
#include <osdep.h>
#include "if_athvar.h"
#include "ah.h"

#if ATH_SUPPORT_AOW

/*
 * @brief   : helper for human readable command names
 */
char aow_cmd_name[][64] = {
    "Init NW",
    "Start NW",
    "Stop NW",
    "Disconnect NW",
    "Change NW id",
    "Update security mode",
    "Change security mode",
    "Sleep NW",
    "Get current stats",
    "Set parameters",
    "power_on",
    "power_off",
    "req_dat_status",
    "stt_dat_status",
    "req_dat_sub_unit_status",
    "stt_dat_sub_unit_status",
    "req_dat_version",
    "stt_dat_version",
    "req_dat_profile",
    "rsp_dat_profile",
    "req_dat_stream_info",
    "res_dat_stream_info",
    "req_nw_status",
    "stt_nw_status",
    "req_nw_version",
    "stt_nw_version",
    "req_nw_vacs_config",
    "stt_nw_vacs_config",
    "link_com_link",
    "link_stt_link",

};

/*
 * @brief   : helper for human readable command names
 */
char aow_event_names[][64] = {
    "Init NW confirmation",
    "Start NW confirmation",
    "Connect indication",
    "BDV not found",
    "Connection failed",
    "Update security mode confirmation",
    "Change security mode confirmation"
    "Change NW id confirmation",
    "Current state indication",
    "Link quality indication",
    "State update indication"
};    

extern aow_info_t aowinfo;

static void print_aow_ctrl_msg(aow_ctrl_msg_t* m);

u_int16_t get_ctrl_msg_seq_no(aow_ctrl_msg_t* m)
{
    u_int16_t seq_no = __bswap16(m->h.seq_no);
    return seq_no;
}

u_int16_t get_ctrl_msg_cmd_type(aow_ctrl_msg_t* m)
{
    u_int16_t cmd  = (u_int16_t)__bswap16(m->h.desc) & (~AOW_CM_LOCAL_COMMAND_FLAG);
    return cmd;
}    

u_int16_t get_ctrl_msg_cmd_desc(aow_ctrl_msg_t *m)
{
    u_int16_t desc = (u_int16_t)__bswap16(m->h.desc);
    return desc;
}

#define IS_AOW_CTRL_MSG_LOCAL(m)    (__bswap16(m->h.desc) & AOW_CM_LOCAL_COMMAND_FLAG)
#define GET_AOW_CTRL_DST_CHANNEL(m) (m->h.dst_addr);


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ieee80211_aow_ctrl_cmd_handler
 *  Description:  This function handles the received control command
 * =====================================================================================
 */

void print_buf(char *buf, int len);
int32_t ieee80211_aow_ctrl_cmd_handler(struct ieee80211com *ic, wbuf_t wbuf,
                                       struct ieee80211_rx_status *rs)
{
    struct audio_pkt *apkt;
    struct ether_header *eh;
    u_int8_t* data;
    u_int32_t datalen;

    eh = (struct ether_header*)wbuf_header(wbuf);

    apkt = GET_AOW_PKT(wbuf);

    if (apkt->signature != ATH_AOW_SIGNATURE) {
        return 0;
    }        

    if (apkt->pkt_type != AOW_CTRL_PKT) {
        return 0;
    }        
    
    ic->ic_aow.rx_ctrl_framecount++;

    data = GET_AOW_DATA(wbuf);
    datalen = GET_AOW_DATA_LEN(wbuf);
            
    if (!is_aow_usb_calls_registered()) {
        return 0;
    }

    ieee80211_aow_send_to_host(ic, data, datalen, AOW_HOST_PKT_DATA, 0, eh->ether_shost);
 
    return 0;
}

int32_t ieee80211_aow_send_to_host(struct ieee80211com* ic, u_int8_t* data, 
                                     u_int32_t len, u_int16_t type, u_int16_t subtype, u_int8_t* addr)
{
    u_int8_t buf[AOW_MAX_HOST_PKT_SIZE];
    aow_host_pkt_t *p;
    u_int32_t plen = 0;

    if (len > (AOW_MAX_HOST_PKT_SIZE - AOW_HOST_PKT_SIZE)) {
        return -EINVAL;
    }        

    if (!is_aow_usb_calls_registered()) {
        return -EINVAL;
    }        

    p = (aow_host_pkt_t*)buf;        

    p->signature = ATH_AOW_SIGNATURE;
    p->type = type;
    p->subtype = subtype;
    p->length = len;
    p->data = buf + AOW_HOST_PKT_SIZE;
    if (addr) {
        OS_MEMCPY(p->addr, addr, IEEE80211_ADDR_LEN);
    }
    plen = len + AOW_HOST_PKT_SIZE;
    OS_MEMCPY(p->data, data, len);

    wlan_aow_dev.rx.recv_ctrl((u_int8_t*)p, plen);
    return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  wlan_aow_pack_and_tx_cmd_pkts
 *  Description:  This function packs the different command packets into single packet
 *                and transmit
 * =====================================================================================
 */


u_int32_t wlan_aow_pack_and_tx_cmd_pkts(struct ieee80211com* ic, 
                                  u_int8_t* d1, u_int32_t dlen1, 
                                  u_int8_t* d2, u_int32_t dlen2,    
                                  u_int8_t* d3, u_int32_t dlen3) 
{                                 
    u_int8_t cmd_pkt[AOW_PROTO_SUPER_PKT_LEN];                                 
    u_int32_t dlen = dlen1 + dlen2 + dlen3;
    u_int64_t tsf = ic->ic_get_aow_tsf_64(ic);

    OS_MEMCPY(cmd_pkt, d1, dlen1);
    OS_MEMCPY(cmd_pkt + dlen1, d2, dlen2);
    OS_MEMCPY(cmd_pkt + dlen1 + dlen2, d3, dlen3);

    ieee80211_aow_tx_ctrl(cmd_pkt, dlen, tsf);
    return 0;


}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ieee80211_send_aow_ctrl_ipformat
 *  Description:  This function sends the AoW control, in the ipformat
 * =====================================================================================
 */

int32_t ieee80211_send_aow_ctrl_ipformat(struct ieee80211_node* ni,
                                         void* pkt,
                                         u_int32_t len,
                                         u_int32_t seqno,
                                         u_int64_t tsf,
                                         u_int32_t audio_channel,
                                         bool setlogSync)
{

    struct ieee80211com *ic = aowinfo.ic;
    struct ieee80211vap *vap = ni->ni_vap;
    wbuf_t wbuf;
    struct ether_header *eh;

    char* pdata = NULL;
    struct audio_pkt* apkt = NULL;
    
    int i = 0;


    u_int32_t offset;
    int32_t retVal;
    u_int32_t total_len;
    u_int8_t* frame;

    total_len = sizeof(struct audio_pkt) + len + ATH_QOS_FIELD_SIZE;

    wbuf = ieee80211_get_aow_frame(ni, &frame, total_len);

    if (wbuf == NULL) {
        vap->iv_stats.is_tx_nobuf++;
        ieee80211_free_node(ni);
        return -ENOMEM;
    }        

    offset = ATH_QOS_FIELD_SIZE + sizeof(struct audio_pkt);

    /* prepare the ethernet header */
    wbuf_push(wbuf, sizeof(struct ether_header));
    eh = (struct ether_header*)wbuf_header(wbuf);

    /* prepare te ether header */
    IEEE80211_ADDR_COPY(eh->ether_dhost, ni->ni_macaddr);
    IEEE80211_ADDR_COPY(eh->ether_shost, vap->iv_myaddr);
    eh->ether_type = ETHERTYPE_IP;

    pdata = (char*)&eh[1];
    apkt = (struct audio_pkt*)pdata;

    apkt->signature = ATH_AOW_SIGNATURE;
    apkt->seqno = seqno;
    apkt->timestamp = tsf;
    apkt->pkt_len = len;
    apkt->pkt_type = AOW_CTRL_PKT;
    apkt->params |= (setlogSync & ATH_AOW_PARAMS_LOGSYNC_FLG_MASK)<< ATH_AOW_PARAMS_LOGSYNC_FLG_S; 
    apkt->audio_channel = audio_channel;

    /* update the volume info */
    for (i = 0; i < AOW_MAX_AUDIO_CHANNELS; i++) {
        OS_MEMCPY(apkt->volume_info.ch[i].info, ic->ic_aow.volume_info.ch[i].info, AOW_VOLUME_DATA_LENGTH);
    }                

    memcpy((pdata + offset), pkt, len);
    retVal = ath_tx_send((wbuf_t)wbuf);

    ieee80211_free_node(ni);
    return retVal;


}

/*
 * @brief       : print raw buffer
 * @param[in]   : char : pointer to data
 * @param[in]   : len  : length of data to be printed
 */

void print_buf(char *buf, int len)
{
    int i = 0;
    while (i < len) {
        IEEE80211_AOW_DPRINTF("0x%02x ", ((unsigned)(unsigned char)*(buf + i)));
        if ( (i) && !(i % 8))
            IEEE80211_AOW_DPRINTF("\n");
        i++;            
    }
    IEEE80211_AOW_DPRINTF("\n");
}    

/*
 * @brief       : print aow control message
 * @param[in]   : m : pointer to aow control message
 */

void print_aow_ctrl_msg(aow_ctrl_msg_t* m)
{
    int cmd  = get_ctrl_msg_cmd_type(m);
    int type = IS_AOW_CTRL_MSG_LOCAL(m);
    IEEE80211_AOW_DPRINTF("\nMSG ----------------------------\n");
    IEEE80211_AOW_DPRINTF("seq no   = %d\n", (u_int16_t)get_ctrl_msg_seq_no(m));
    IEEE80211_AOW_DPRINTF("src addr = %d\n", m->h.src_addr);
    IEEE80211_AOW_DPRINTF("dst addr = %d\n", m->h.dst_addr);
    IEEE80211_AOW_DPRINTF("len      = %d\n", m->h.len);
    IEEE80211_AOW_DPRINTF("desc     = %d\n", get_ctrl_msg_cmd_desc(m));
    IEEE80211_AOW_DPRINTF("type     = %s\n", (type)?"local":"remote");
    if (cmd < AOW_MAX_COMMAND_TYPES) {
        IEEE80211_AOW_DPRINTF("command  = %s\n", aow_cmd_name[cmd]);
    } else {
        IEEE80211_AOW_DPRINTF("err (%d) : invalid command type\n", cmd);
    }
#if 0    
    IEEE80211_AOW_DPRINTF("\nDATA --------------------------\n");
    print_buf((char*)m->data, m->h.len);
#endif    
}        

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ieee80211_aow_tx_ctrl
 *  Description:  Function to transmit AoW control commands, strongly connected to
 *                command format
 *         TODO:  Implement control packet sequence number scheme
 *                Implement stats related to control packet
 *
 * =====================================================================================
 */

u_int32_t ieee80211_aow_tx_ctrl(u_int8_t* data, u_int32_t datalen, u_int64_t tsf)
{
    aow_ctrl_data_msg_t* pd = NULL;
    struct ieee80211_node *ni = NULL;
    struct ieee80211com *ic = aowinfo.ic;
    int is_cmd_local;
    u_int32_t cmd = 0;
    u_int8_t event_subtype = CM_SEND_BUFFER_STATUS_FAIL;

    aow_ctrl_msg_t* mh = (aow_ctrl_msg_t*)data;
    is_cmd_local = IS_AOW_CTRL_MSG_LOCAL(mh);
    cmd = get_ctrl_msg_cmd_type(mh);

    /* 
     * The command is meant for this device,
     * pass it to athcm connection manager
     */
    if (is_cmd_local) {
        aow_ci_send(ic, data, datalen);
        return 0;
    } 

    pd = (aow_ctrl_data_msg_t*)(mh->data);
    ni = ieee80211_find_node(&ic->ic_sta, pd->addr);

    if (ni) {
        ieee80211_send_aow_ctrl_ipformat(ni, pd->data, __bswap32(pd->length) , 0, tsf, 0, 0);
        event_subtype = CM_SEND_BUFFER_STATUS_PASS;
    }
    ieee80211_aow_send_to_host(ic, &event_subtype, sizeof(event_subtype), AOW_HOST_PKT_EVENT, event_subtype, NULL);
    return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  aow_get_channel_id
 *  Description:  Extracts the audio channel index from the AoW command packet
 *                Returns the bit map for set channels
 * =====================================================================================
 */

u_int32_t aow_get_channel_id(struct ieee80211com* ic, aow_proto_pkt_t* pkt)
{
    u_int32_t channels= 0;

    if (IS_AOW_PROTO_CH_0_SET(pkt->header)) {
        channels |= 0x1;
    }

    if (IS_AOW_PROTO_CH_1_SET(pkt->header)) {
        channels |= 0x2;
    }

    if (IS_AOW_PROTO_CH_2_SET(pkt->header)) {
        channels |= 0x4;
    }        

    if (IS_AOW_PROTO_CH_3_SET(pkt->header)) {
        channels |= 0x8;
    }        

    return channels;

}

u_int32_t ieee80211_aow_sim_ctrl_msg(struct ieee80211com* ic, u_int32_t val)
{
    IEEE80211_AOW_DPRINTF("Not Supported\n");
    return 0;
}

u_int32_t ieee80211_aow_disconnect_device(struct ieee80211com* ic, u_int32_t channel)
{
    struct ether_addr macaddr;
    struct ieee80211_node *ni = NULL;

    if (!aow_get_macaddr(channel, 0, &macaddr)) {
        return FALSE;
    }

    if ((ni = ieee80211_find_node(&ic->ic_sta, macaddr.octet)) != NULL) {
        ieee80211_send_disassoc(ni, IEEE80211_REASON_UNSPECIFIED);
        IEEE80211_NODE_LEAVE(ni);
        ieee80211_free_node(ni);
    }

    return TRUE;
}



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  aow_print_ctrl_pkt
 *  Description:  Debug utility function
 *                
 * =====================================================================================
 */

void aow_print_ctrl_pkt(aow_proto_pkt_t* pkt)
{
    u_int32_t i = 0;
    IEEE80211_AOW_DPRINTF("Protocol header = 0x%x\n", pkt->header);
    IEEE80211_AOW_DPRINTF("NW address    = 0x%x\n", pkt->address);
    IEEE80211_AOW_DPRINTF("Data length     = 0x%x\n", pkt->dlen);
    IEEE80211_AOW_DPRINTF("Checksum        = 0x%x\n", pkt->chksum);
    
    for (i = 0; i < pkt->dlen; i++)
        IEEE80211_AOW_DPRINTF("0x%x \n", pkt->data[i]);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  aow_print_ctrl_pkt
 *  Description:  Debug utility function
 *                
 * =====================================================================================
 */
u_int32_t ieee80211_aow_get_ctrl_cmd(struct ieee80211com* ic)
{
    IEEE80211_AOW_DPRINTF("This feature is not Supported\n");
    return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  aow_print_ctrl_pkt
 *  Description:  Debug utility function to dump contents of a buffer in hex
 *                
 * =====================================================================================
 */
void ieee80211_aow_hexdump(unsigned char *buf, unsigned int len)
{
    if (0 == len) {
        return;
    }

    IEEE80211_AOW_INFO_DPRINTF("Hexdump of received control packet:\n");
    
    len--;
    IEEE80211_AOW_INFO_DPRINTF("%02x", *buf++);

    while (len--) {
        IEEE80211_AOW_DPRINTF("%02x", *buf++);
    }

    IEEE80211_AOW_DPRINTF("\n");
}

#endif  /* ATH_SUPPORT_AOW */
