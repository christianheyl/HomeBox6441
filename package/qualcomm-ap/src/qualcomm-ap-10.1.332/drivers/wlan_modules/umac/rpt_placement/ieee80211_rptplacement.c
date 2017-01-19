#include <ieee80211_rptplacement_priv.h>

#if UMAC_SUPPORT_RPTPLACEMENT

void
ieee80211_rptplacement_set_param(wlan_if_t vaphandle, ieee80211_param param, u_int32_t val)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic; 

    /* make sure that we have proper init state before acting */
    if (ic && !ic->ic_rptplacement_init_done) return;

    switch (param) {
        case IEEE80211_RPT_CUSTPROTO_ENABLE:
            ic->ic_rptplacementparams->rpt_custproto_en = val;
            break;
        case IEEE80211_RPT_GPUTCALC_ENABLE:
            ic->ic_rptplacementparams->rpt_gputcalc_en = val;
            break;
        case IEEE80211_RPT_DEVUP:
            ic->ic_rptplacementparams->rpt_devup = val;
            break;
        case IEEE80211_RPT_MACDEV:
            ic->ic_rptplacementparams->rpt_macdev = val;
            break;
        case IEEE80211_RPT_MACADDR1:
            ic->ic_rptplacementparams->rpt_macaddr1 = val;
            break;
        case IEEE80211_RPT_MACADDR2:
            ic->ic_rptplacementparams->rpt_macaddr2 = val;
            break;
        case IEEE80211_RPT_GPUTMODE:
            ic->ic_rptplacementparams->rpt_gputmode = val;
            break;
        case IEEE80211_RPT_TXPROTOMSG:
            ic->ic_rptplacementparams->rpt_txprotomsg = val;
            break;
        case IEEE80211_RPT_RXPROTOMSG:
            ic->ic_rptplacementparams->rpt_rxprotomsg = val;
            break;
        case IEEE80211_RPT_STATUS:
            ic->ic_rptplacementparams->rpt_status = val;
            break;
        case IEEE80211_RPT_ASSOC:
            ic->ic_rptplacementparams->rpt_assoc = val;
            break;
        case IEEE80211_RPT_NUMSTAS:
            ic->ic_rptplacementparams->rpt_numstas = val;
            break;
        case IEEE80211_RPT_STA1ROUTE:
            ic->ic_rptplacementparams->rpt_sta1route = val;
            break;
        case IEEE80211_RPT_STA2ROUTE:
            ic->ic_rptplacementparams->rpt_sta2route = val;
            break;
        case IEEE80211_RPT_STA3ROUTE:
            ic->ic_rptplacementparams->rpt_sta3route = val;
            break;
        case IEEE80211_RPT_STA4ROUTE:
            ic->ic_rptplacementparams->rpt_sta4route = val;
            break;
        default:
            break;
    }
}

u_int32_t
ieee80211_rptplacement_get_param(wlan_if_t vaphandle, ieee80211_param param)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;  
    u_int32_t val = 0;

    if (ic && !ic->ic_rptplacement_init_done)
        return 0;

    switch (param) {
        case IEEE80211_RPT_CUSTPROTO_ENABLE:
            val = ic->ic_rptplacementparams->rpt_custproto_en;
            break;
        case IEEE80211_RPT_GPUTCALC_ENABLE:
            val = ic->ic_rptplacementparams->rpt_gputcalc_en;
            break;
        case IEEE80211_RPT_DEVUP:
            val = ic->ic_rptplacementparams->rpt_devup;
            break;
        case IEEE80211_RPT_MACDEV:
            val = ic->ic_rptplacementparams->rpt_macdev;
            break;
        case IEEE80211_RPT_MACADDR1:
            val = ic->ic_rptplacementparams->rpt_macaddr1;
            break;
        case IEEE80211_RPT_MACADDR2:
            val = ic->ic_rptplacementparams->rpt_macaddr2;
            break;
        case IEEE80211_RPT_GPUTMODE:
            val = ic->ic_rptplacementparams->rpt_gputmode;
            break;
        case IEEE80211_RPT_TXPROTOMSG:
            val = ic->ic_rptplacementparams->rpt_txprotomsg;
            break;
        case IEEE80211_RPT_RXPROTOMSG:
            val = ic->ic_rptplacementparams->rpt_rxprotomsg;
            break;
        case IEEE80211_RPT_STATUS:
            val = ic->ic_rptplacementparams->rpt_status;
            break;
        case IEEE80211_RPT_ASSOC:
            val = ic->ic_rptplacementparams->rpt_assoc;
            break;
        case IEEE80211_RPT_NUMSTAS:
            val = ic->ic_rptplacementparams->rpt_numstas;
            break;
        case IEEE80211_RPT_STA1ROUTE:
            val = ic->ic_rptplacementparams->rpt_sta1route;
            break;
        case IEEE80211_RPT_STA2ROUTE:
            val = ic->ic_rptplacementparams->rpt_sta2route;
            break;
        case IEEE80211_RPT_STA3ROUTE:
            val = ic->ic_rptplacementparams->rpt_sta3route;
            break;
        case IEEE80211_RPT_STA4ROUTE:
            val = ic->ic_rptplacementparams->rpt_sta4route;
            break;
        default:
            break;
    }
    return val;
}

/* The function is used  to set the MAC address  of a device set via iwpriv */
void ieee80211_rptplacement_set_mac_addr(u_int8_t *mac_address, u_int32_t word1, u_int32_t word2)
{
    mac_address[0] =  (u_int8_t) ((word1 & 0xff000000) >> 24);
    mac_address[1] =  (u_int8_t) ((word1 & 0x00ff0000) >> 16);
    mac_address[2] =  (u_int8_t) ((word1 & 0x0000ff00) >> 8);
    mac_address[3] =  (u_int8_t) ((word1 & 0x000000ff));
    mac_address[4] =  (u_int8_t) ((word2 & 0x0000ff00) >> 8);
    mac_address[5] =  (u_int8_t) ((word2 & 0x000000ff));
}

/* The function is used  to get the MAC address of a device set via iwpriv. It then
   identifies the device (RootAP, RPT or STAs) and accordingly sets the MAC address
   in the rptgput stucture */
void ieee80211_rptplacement_get_mac_addr(struct ieee80211vap *vap, u_int32_t device)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    u_int32_t mac_addr1 = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_MACADDR1);
    u_int32_t mac_addr2 = ieee80211_rptplacement_get_param(vap, IEEE80211_RPT_MACADDR2);

    switch(device) {
        case STA1:
            ieee80211_rptplacement_set_mac_addr(rptgput->sta1_mac_address, mac_addr1, mac_addr2);
            break; 
        case STA2:
            ieee80211_rptplacement_set_mac_addr(rptgput->sta2_mac_address, mac_addr1, mac_addr2);
            break; 
        case STA3:
            ieee80211_rptplacement_set_mac_addr(rptgput->sta3_mac_address, mac_addr1, mac_addr2);
            break; 
        case STA4:
            ieee80211_rptplacement_set_mac_addr(rptgput->sta4_mac_address, mac_addr1, mac_addr2);
            break; 
        case ROOTAP:
            ieee80211_rptplacement_set_mac_addr(rptgput->rootap_mac_address, mac_addr1, mac_addr2);
            break; 
        case RPT:
            ieee80211_rptplacement_set_mac_addr(rptgput->rpt_mac_address, mac_addr1, mac_addr2);
            break;
        default:
            break;
    }
}

/* Get Repeater placement status */
void ieee80211_rptplacement_get_status(struct ieee80211com *ic, u_int32_t word)
{
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    rptgput->rpt_status  = (word & 0x0000000F);
    rptgput->num_nw_stas = ((word & 0x000000F0) >> 4);
}

/* Get RPT Association status */
void ieee80211_rptplacement_get_rptassoc(struct ieee80211com *ic, u_int32_t word)
{
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    rptgput->rpt_assoc_status = word;
}

/* Indicates if gooput calculator should function in AP/RPT mode */
void ieee80211_rptplacement_get_gputmode(struct ieee80211com *ic, u_int32_t word)
{
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    rptgput->gputmode =  word;
}

/* Get number of STAs that will participate in training*/
void ieee80211_rptplacement_get_numstas(struct ieee80211com *ic, u_int32_t word)
{
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    rptgput->numstas = word;
}

/* Get routing table for STA#1 */
void ieee80211_rptplacement_get_sta1route(struct ieee80211com *ic, u_int32_t word)
{
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    rptgput->sta1route = word;
}

/* Get routing table for STA#2 */
void ieee80211_rptplacement_get_sta2route(struct ieee80211com *ic, u_int32_t word)
{
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    rptgput->sta2route = word;
}

/* Get routing table for STA#3 */
void ieee80211_rptplacement_get_sta3route(struct ieee80211com *ic, u_int32_t word)
{
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    rptgput->sta3route = word;
}

/* Get routing table for STA#4 */
void ieee80211_rptplacement_get_sta4route(struct ieee80211com *ic, u_int32_t word)
{
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    rptgput->sta4route = word;
}

/* This function is used to send training data packets using the Atheros custom
   88bd protocol. Packet structure:
    802.3 HDR + ATHHDR +  Payload 
        14        4        1500 
*/
int ieee80211_rptplacement_tx_custom_data_pkt(struct ieee80211vap *vap, struct custom_pkt_params *cpp)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ether_header *eh;			
    struct athl2p_tunnel_hdr *tunHdr;		
    u_int  msg_len;
    wbuf_t mywbuf;
    struct net_device *netd;
    struct ieee80211_node *ni;
    
    ni = ieee80211_find_node(&ic->ic_sta, cpp->dst_mac_address);
    if (ni == NULL) {
        return -1;
    }
    netd = (struct net_device *) wlan_vap_get_registered_handle(vap);

    /* 1500 byte payload is used for training */
    msg_len = RPT_TRAINING_PAYLOAD_SIZE; 

    /* wbuf_alloc allocs length + MIN_HEAD_ROOM for WBUF_TX_DATA and reserves
       MIN_HEAD_ROOM. MIN_HEAD_ROOM is 64 bytes which is greater than the size
       of ETH + ATHR HDR. Remaining bytes go waste - Don't care */
    mywbuf = wbuf_alloc(ic->ic_osdev,  WBUF_TX_DATA, msg_len);

    if (!mywbuf) {
        printk("wbuf allocation failed in tx_custom_pkt\n");
        return -1;
    }

    /* Initialize */
    wbuf_append(mywbuf, msg_len);
    memset((u_int8_t *)wbuf_raw_data(mywbuf), 0, msg_len);

    tunHdr = (struct athl2p_tunnel_hdr *) wbuf_push(mywbuf, sizeof(struct athl2p_tunnel_hdr));
    eh     = (struct ether_header *) wbuf_push(mywbuf, sizeof(struct ether_header));

    /* ATH_ETH_TYPE protocol subtype */
    tunHdr->proto = ATH_ETH_TYPE_RPT_TRAINING;

    /* Copy the SRC & DST MAC addresses into ethernet header*/ 
    IEEE80211_ADDR_COPY(&eh->ether_shost[0], cpp->src_mac_address);
    IEEE80211_ADDR_COPY(&eh->ether_dhost[0], cpp->dst_mac_address);

    /* copy ethertype */
    eh->ether_type = htons(ATH_ETH_TYPE);
    mywbuf->dev = netd;
    ieee80211_send_wbuf(vap, ni, mywbuf);  
    ieee80211_free_node(ni);   
    return 0;
}

/* This function is used to send custom protocol packets using the Atheros 
   custom 88bd protocol. Packet structure:
    802.3 HDR + ATHHDR +  Payload 
        14        4         
*/
int ieee80211_rptplacement_tx_custom_proto_pkt(struct ieee80211vap *vap, struct custom_pkt_params *cpp)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ether_header *eh;			
    struct athl2p_tunnel_hdr *tunHdr;		
    int    msg_len;
    wbuf_t mywbuf;
    struct net_device *netd;
    struct ieee80211_node *ni;

    ni = ieee80211_find_node(&ic->ic_sta, cpp->dst_mac_address);
    
    if (ni == NULL) {
        return -1;
    }

    netd = (struct net_device *) wlan_vap_get_registered_handle(vap);

    /* Set msg length to the largest protocol msg payload size */
    msg_len = MAX_PROTO_PKT_SIZE; 


    /* wbuf_alloc allocs length + MIN_HEAD_ROOM for WBUF_TX_DATA and reserves
       MIN_HEAD_ROOM. MIN_HEAD_ROOM is 64 bytes which is greater than the size
       of ETH + ATHR HDR. Remaining bytes go waste - Don't care */
    mywbuf = wbuf_alloc(ic->ic_osdev,  WBUF_TX_DATA, msg_len);

    if (!mywbuf) {
        printk("wbuf allocation failed in tx_custom_pkt\n");
        return -1;
    }

    /* Copy protocol messsage data */
    wbuf_append(mywbuf, cpp->num_data_words*4);
    OS_MEMCPY((u_int8_t *)(wbuf_raw_data(mywbuf)), cpp->msg_data, cpp->num_data_words*4); 
    
    tunHdr = (struct athl2p_tunnel_hdr *) wbuf_push(mywbuf, sizeof(struct athl2p_tunnel_hdr));
    eh     = (struct ether_header *) wbuf_push(mywbuf, sizeof(struct ether_header));

    /* ATH_ETH_TYPE protocol subtype */
    tunHdr->proto = ATH_ETH_TYPE_RPT_MSG;
    /* Copy the SRC & DST MAC addresses into ethernet header*/ 
    IEEE80211_ADDR_COPY(&eh->ether_shost[0], cpp->src_mac_address);
    IEEE80211_ADDR_COPY(&eh->ether_dhost[0], cpp->dst_mac_address);
    /* copy ethertype */
    eh->ether_type = htons(ATH_ETH_TYPE);
    mywbuf->dev = netd;
    ieee80211_send_wbuf(vap, ni, mywbuf);    
    ieee80211_free_node(ni); 
    return 0;
}

/* This function is used to populate the goodput table. A list of associated STAs is obtained 
   by parsing through the node table. This list of STAs is populated into the goodput table &
   the goodput to each of the STAs is estimated by sending training pkts to them & extracting 
   the estimated goodput based on the received ACKs from the rate control module. 
*/ 
u_int32_t ieee80211_rptplacement_gput_calc(struct ieee80211vap *vap,  
                                           struct ieee80211_node_table *nt, 
                                           u_int8_t vap_addr[][IEEE80211_ADDR_LEN],
                                           u_int32_t sta_count, u_int32_t ap0_rpt1)
{
    struct ieee80211com *ic = vap->iv_ic; 
    struct ieee80211_node *ni;
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput; 
    u_int32_t loop_count = 0, ave_goodput = 0, loop_count1 = 0, goodput = 0; 
    struct custom_pkt_params cpp;
    int    condition1, condition2, condition3;

    /* Parse throgh entires of the node table to come up with list of nodes to which 
       goodput must be estimated */
    TAILQ_FOREACH(ni, &nt->nt_node, ni_list) {

        /* 1. Ensure that the node's MAC address does not match that of VAP list. (condition 1)
           2. To get a list of associated nodes,ensure that the node's BSSID matches the 
              Root AP MAC address in AP mode (gputmode=0) or the Repeater MAC address in 
              RPT mode (gputmode=1). (condition 2)
           3. In the Repeater mode of operation, we have one more mode where the repeater 
              will be a STA that associates with the Root AP. (condition 3)*/
        condition1 = (IEEE80211_ADDR_EQ(ni->ni_macaddr, vap_addr[0]) != 1) && 
                     (IEEE80211_ADDR_EQ(ni->ni_macaddr, vap_addr[1]) != 1);
        condition2 = (((IEEE80211_ADDR_EQ(ni->ni_bssid, rptgput->rootap_mac_address) == 1) && (rptgput->gputmode == 0)) ||
                      ((IEEE80211_ADDR_EQ(ni->ni_bssid, rptgput->rpt_mac_address) == 1)    && (rptgput->gputmode == 1))) &&
                     (IEEE80211_ADDR_EQ(ni->ni_bssid, vap_addr[0]) == 1) &&  (ap0_rpt1 == 0);
        condition3 = (IEEE80211_ADDR_EQ(ni->ni_bssid, rptgput->rootap_mac_address) == 1) && (ap0_rpt1 == 1);

        if (condition1 && (condition2 || condition3)) {
            /* Populate goodput table with the node's MAC address */
            for (loop_count = 0; loop_count < IEEE80211_ADDR_LEN; loop_count++) {
                rptgput->sta_top_map[sta_count].mac_address[loop_count]  =  ni->ni_macaddr[loop_count];
            }
            goodput     = 0;
            ave_goodput = 0;

            IEEE80211_ADDR_COPY(cpp.dst_mac_address, rptgput->sta_top_map[sta_count].mac_address);
            IEEE80211_ADDR_COPY(cpp.src_mac_address, ni->ni_bssid);

            /* Send training pkts to each of the STAs for estimating goodput */
            for (loop_count1 = 0; loop_count1 < RPT_NUM_AVGS; loop_count1++) { 
                printk(".");
                for (loop_count = 0; loop_count < RPT_NUM_PKTS_PER_AVG; loop_count++) { 
                    ieee80211_rptplacement_tx_custom_data_pkt(vap, &cpp);
                }
                OS_SLEEP(msecs_to_jiffies(50)); /* wait 50ms */
                goodput = ic->ic_get_goodput(ni);
                ave_goodput += goodput/100;     /* PER % - div not done in rate table */
            }
            if (ave_goodput > RPT_GPUT_THRESH) {/* 50Mbps * 32 */
                rptgput->sta_top_map[sta_count].goodput = ((ave_goodput/RPT_NUM_AVGS)*9/10); /*10% headroom*/
            } else {  
                rptgput->sta_top_map[sta_count].goodput = ((ave_goodput/RPT_NUM_AVGS)*3/4); /*25% headroom*/
            }
            sta_count++;
        }
        OS_SLEEP(HZ); /* wait 1s */ 
    }
    return sta_count;
}

/* This function is used to display the recorded goodput table */
void ieee80211_rptplacement_display_goodput_table(struct ieee80211com *ic)
{
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput; 
    int loop_count = 0;

    printk("\n# STAs = %d\n", rptgput->sta_count);
    printk("============================================\n");
    printk("Goodput calculator summary for this position\n");
    printk("============================================\n");
    for (loop_count = 0; loop_count < rptgput->sta_count; loop_count++) {
        printk("STA: %s - %d kbps\n", ether_sprintf(rptgput->sta_top_map[loop_count].mac_address), rptgput->sta_top_map[loop_count].goodput);
    }
}

int ieee80211_rptplacement_gput_est_init(struct ieee80211vap *vap, int ap0_rpt1)
{
    struct net_device *netd;
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node_table  *nt = &ic->ic_sta;
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput; 
    struct ieee80211vap *v;
    u_int8_t  vap_addr[2][IEEE80211_ADDR_LEN];
    u_int32_t sta_count = 0;
    int loop_count = 0, loop_count1 = 0;  

    /* Get the VAP addresses */
    TAILQ_FOREACH(v, &ic->ic_vaps, iv_next) {
        for (loop_count1 = 0; loop_count1 <6; loop_count1++) {
            vap_addr[loop_count][loop_count1] = v->iv_myaddr[loop_count1];
        }
        loop_count++;
    }

    printk("\nRepeater training is in process"); 

    netd = (struct net_device *) wlan_vap_get_registered_handle(vap);
    sta_count = 0;
    sta_count = ieee80211_rptplacement_gput_calc(vap, nt, vap_addr, sta_count, ap0_rpt1); 

    if (ap0_rpt1 == 1) { 
        v = TAILQ_NEXT(vap, iv_next);
        netd = (struct net_device *) wlan_vap_get_registered_handle(v);
        sta_count = ieee80211_rptplacement_gput_calc(v, nt, vap_addr, sta_count, ap0_rpt1);
     }

    rptgput->sta_count = sta_count;
    ieee80211_rptplacement_display_goodput_table(ic); 

    for (loop_count = 0; loop_count < 1; loop_count++) {
        OS_SLEEP(HZ); /* wait 1s */
    }
    ieee80211_rptplacement_tx_proto_msg(vap, RPT_AP_GPT);
    /* Send the rpt goodput table  to the socket */
    ieee80211_rptplacement_create_msg(ic);
    return 0;
}

/* This function is used to construct a custom protocol message for transmission.
   It populates the custom_pkt_params structure and hands it over to the
   ieee80211_rptplacement_tx_custom_proto_pkt function */
int ieee80211_rptplacement_tx_proto_msg(struct ieee80211vap *vap, u_int32_t msg_num)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    struct custom_pkt_params cpp; 
    int    i; 

    if (!rptgput->gputmode) {
        IEEE80211_ADDR_COPY(cpp.dst_mac_address, rptgput->rpt_mac_address);
        IEEE80211_ADDR_COPY(cpp.src_mac_address, rptgput->rootap_mac_address);
    } else {
        IEEE80211_ADDR_COPY(cpp.dst_mac_address, rptgput->rootap_mac_address);
        IEEE80211_ADDR_COPY(cpp.src_mac_address, rptgput->rpt_mac_address);
    } 
    cpp.msg_num = msg_num; 

    switch (msg_num) {
        case AP_RPT_CM1: 
        case RPT_AP_ACK1: 
        case AP_RPT_ACK2: 
        case RPT_AP_CM2: 
        case RPT_AP_ACK3: 
        case AP_RPT_DONE:
            cpp.num_data_words = 1;
            cpp.msg_data    = (u_int32_t *) OS_MALLOC(ic->ic_osdev, sizeof(u_int32_t), GFP_KERNEL);
            if (cpp.msg_data == NULL) {
                return -ENOMEM;
            } 
            cpp.msg_data[0] = (msg_num << 24) + (msg_num << 16) + (msg_num << 8)  + msg_num; 
            break;
        case RPT_AP_GPT:
            cpp.num_data_words = 2 + (rptgput->sta_count * 3) ;
            cpp.msg_data    = (u_int32_t *) OS_MALLOC(ic->ic_osdev, 
                                                 sizeof(u_int32_t)*cpp.num_data_words, GFP_KERNEL);
            if (cpp.msg_data == NULL) {
                return -ENOMEM;
            } 
            cpp.msg_data[0] = (msg_num << 24) + (msg_num << 16) + (msg_num << 8)  + msg_num; 
            cpp.msg_data[1] = rptgput->sta_count;
            for (i = 0; i < rptgput->sta_count; i++) {
                 cpp.msg_data[i * 3 + 2] = (rptgput->sta_top_map[i].mac_address[0] << 24) + 
                                           (rptgput->sta_top_map[i].mac_address[1] << 16) +
                                           (rptgput->sta_top_map[i].mac_address[2] << 8)  +
                                            rptgput->sta_top_map[i].mac_address[3];
                 cpp.msg_data[i * 3 + 3] = ((rptgput->sta_top_map[i].mac_address[4] << 8) +
                                            rptgput->sta_top_map[i].mac_address[5]) & (0x0000FFFF);
                 cpp.msg_data[i * 3 + 4] = rptgput->sta_top_map[i].goodput;
            }
            break;
        case AP_RPT_ASSOC:
            cpp.num_data_words = 3;
            cpp.msg_data    = (u_int32_t *) OS_MALLOC(ic->ic_osdev, sizeof(u_int32_t)*3, GFP_KERNEL);
            if (cpp.msg_data == NULL) {
                return -ENOMEM;
            } 
            cpp.msg_data[0] = (msg_num << 24) + (msg_num << 16) + (msg_num << 8)  + msg_num; 
            cpp.msg_data[1] = (rptgput->num_nw_stas << 24) + (rptgput->num_nw_stas << 16) +
                              (rptgput->num_nw_stas << 8)  + rptgput->num_nw_stas;
            cpp.msg_data[2] = ((rptgput->rpt_status & 0x0000000f) << 24) +
                              ((rptgput->rpt_status & 0x0000000f) << 16) +
                              ((rptgput->rpt_assoc_status & 0x000000ff) << 8) +
                              (rptgput->rpt_assoc_status & 0x000000ff);
            break;
        case AP_STA_ROUTE:
            cpp.num_data_words = 2 + (rptgput->numstas * 3);
            cpp.msg_data    = (u_int32_t *) OS_MALLOC(ic->ic_osdev, sizeof(u_int32_t) *
                                                      cpp.num_data_words, GFP_KERNEL);
            if (cpp.msg_data == NULL) {
                return -ENOMEM;
            } 
            cpp.msg_data[0] = (msg_num << 24) + (msg_num << 16) + (msg_num << 8)  + msg_num; 
            cpp.msg_data[1] =  (rptgput->numstas << 24) + (rptgput->numstas << 16) +
                               (rptgput->numstas << 8)  + rptgput->numstas;
            cpp.msg_data[2] =  (rptgput->sta1_mac_address[0] << 24) + 
                               (rptgput->sta1_mac_address[1] << 16) +
                               (rptgput->sta1_mac_address[2] << 8)  +
                                rptgput->sta1_mac_address[3];
            cpp.msg_data[3] = ((rptgput->sta1_mac_address[4] << 8) +
                                rptgput->sta1_mac_address[5]) & (0x0000FFFF);
            cpp.msg_data[4] =  rptgput->sta1route;
            cpp.msg_data[5] =  (rptgput->sta2_mac_address[0] << 24) + 
                               (rptgput->sta2_mac_address[1] << 16) +
                               (rptgput->sta2_mac_address[2] << 8)  +
                                rptgput->sta2_mac_address[3];
            cpp.msg_data[6] = ((rptgput->sta2_mac_address[4] << 8) +
                                rptgput->sta2_mac_address[5]) & (0x0000FFFF);
            cpp.msg_data[7] =  rptgput->sta2route;
            cpp.msg_data[8] =  (rptgput->sta3_mac_address[0] << 24) + 
                               (rptgput->sta3_mac_address[1] << 16) +
                               (rptgput->sta3_mac_address[2] << 8)  +
                                rptgput->sta3_mac_address[3];
            cpp.msg_data[9]  = ((rptgput->sta3_mac_address[4] << 8) +
                                 rptgput->sta3_mac_address[5]) & (0x0000FFFF);
            cpp.msg_data[10] = rptgput->sta3route;
            cpp.msg_data[11] = (rptgput->sta4_mac_address[0] << 24) + 
                               (rptgput->sta4_mac_address[1] << 16) +
                               (rptgput->sta4_mac_address[2] << 8)  +
                                rptgput->sta4_mac_address[3];
            cpp.msg_data[12] = ((rptgput->sta4_mac_address[4] << 8) +
                                rptgput->sta4_mac_address[5]) & (0x0000FFFF);
            cpp.msg_data[13] = rptgput->sta4route;
            break;
        default:  
            break;
        }
    ieee80211_rptplacement_tx_custom_proto_pkt(vap, &cpp);
    if (cpp.msg_data != NULL) {
        OS_FREE(cpp.msg_data);
    }
    return 0;
}
/* This function is used to check the incoming packets for ATH_ETH_TYPE packets (0x88BD) & look
   for custom protocol packets (protocol 0x19) used in the repeater placement training
*/
void
ieee80211_rptplacement_input(struct ieee80211vap *vap, wbuf_t wbuf, struct ether_header *eh)
{
    struct athl2p_tunnel_hdr *tunHdr;
    struct ieee80211com *ic = vap->iv_ic; 

    /* do nothing either rptplacement not-up or failed to init */
    if (!ic->ic_rptplacement_init_done) return;

    if (ic->ic_rptplacementparams->rpt_custproto_en == 1 && eh->ether_type == ATH_ETH_TYPE) {
        wbuf_pull(wbuf, (u_int16_t) (sizeof(*eh)));
        tunHdr = (struct athl2p_tunnel_hdr *)(wbuf_header(wbuf));
        if (tunHdr->proto == ATH_ETH_TYPE_RPT_MSG) {
            ieee80211_rptplacement_rx_proto_msg(vap, wbuf);
        }
        wbuf_push(wbuf, (u_int16_t) (sizeof(*eh)));
    }
}

/* This function is used to determine the message  type of the received custom 
   protocol packet. If the incoming packet is not RPT_AP_GPT or AP_RPT_ASSOC or
   AP_STA_ROUTE, it will set the rxprotomsg iwpriv. If the incoming message is
   of these types, it will call the appropiate function for processing the 
   contents of the message */ 
void ieee80211_rptplacement_rx_proto_msg(struct ieee80211vap *vap, wbuf_t mywbuf)
{
    u_int8_t prot_msg;
    u_int8_t *payload;
    payload = (u_int8_t *)wbuf_raw_data(mywbuf);

    if ((payload[RPT_BUF_MSG_NUM1] == payload[RPT_BUF_MSG_NUM2]) &&
        (payload[RPT_BUF_MSG_NUM2] == payload[RPT_BUF_MSG_NUM3]) &&
        (payload[RPT_BUF_MSG_NUM3] == payload[RPT_BUF_MSG_NUM4])) {
        if (payload[RPT_BUF_MSG_NUM1] == RPT_AP_GPT) {
            ieee80211_rptplacement_rx_gput_table(vap, mywbuf);
        } else if (payload[RPT_BUF_MSG_NUM1] == AP_RPT_ASSOC) {
            ieee80211_rptplacement_rx_assoc_status(vap, mywbuf);
        } else if (payload[RPT_BUF_MSG_NUM1] == AP_STA_ROUTE) {
            ieee80211_rptplacement_rx_routingtable(vap, mywbuf);
        } else {
            prot_msg = payload[RPT_BUF_MSG_NUM1];
            ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_RXPROTOMSG, prot_msg);
        }  
    }
}

/* This function is used to set the training & association status based on the 
   contents of the AP_RPT_ASSOC message. It then pushes this information to 
   user space using netlink sockets */
int ieee80211_rptplacement_rx_assoc_status(struct ieee80211vap *vap, wbuf_t mywbuf)
{
    u_int32_t sta_count, loop_count, rpt_assoc_status;
    struct ieee80211com *ic = vap->iv_ic; 
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    u_int8_t *payload = (u_int8_t *)wbuf_raw_data(mywbuf);

    if ((payload[RPT_BUF_STA_CNT1] == payload[RPT_BUF_STA_CNT2]) &&
        (payload[RPT_BUF_STA_CNT2] == payload[RPT_BUF_STA_CNT3]) &&
        (payload[RPT_BUF_STA_CNT3] == payload[RPT_BUF_STA_CNT4])) {
        sta_count = payload[RPT_BUF_STA_CNT1] & 0x000000FF;
    } else {
        return -1; 
    } 

    if (sta_count > 4) 
       return -1;

    if ((payload[RPT_BUF_STATUS1] == payload[RPT_BUF_STATUS2]) &&
        (payload[RPT_BUF_ASSOC1] == payload[RPT_BUF_ASSOC2])) {
        rpt_assoc_status = (payload[RPT_BUF_STATUS1] << 24) + (payload[RPT_BUF_STATUS2] << 16) +
                           (payload[RPT_BUF_ASSOC1] << 8)  +  payload[RPT_BUF_ASSOC1];
    } else {
        return 0; 
    } 

    printk("\n #STAs = %d\n", sta_count);
    rptgput->sta_count = sta_count;

    switch(sta_count) {
        case 1: 
            IEEE80211_ADDR_COPY(rptgput->sta_top_map[0].mac_address, rptgput->sta1_mac_address);
            break;
        case 2:
            IEEE80211_ADDR_COPY(rptgput->sta_top_map[0].mac_address, rptgput->sta1_mac_address);   
            IEEE80211_ADDR_COPY(rptgput->sta_top_map[1].mac_address, rptgput->sta2_mac_address);
            break;
        case 3:
            IEEE80211_ADDR_COPY(rptgput->sta_top_map[0].mac_address, rptgput->sta1_mac_address);   
            IEEE80211_ADDR_COPY(rptgput->sta_top_map[1].mac_address, rptgput->sta2_mac_address);
            IEEE80211_ADDR_COPY(rptgput->sta_top_map[2].mac_address, rptgput->sta3_mac_address);
            break;     
        case 4:
            IEEE80211_ADDR_COPY(rptgput->sta_top_map[0].mac_address, rptgput->sta1_mac_address);   
            IEEE80211_ADDR_COPY(rptgput->sta_top_map[1].mac_address, rptgput->sta2_mac_address);
            IEEE80211_ADDR_COPY(rptgput->sta_top_map[2].mac_address, rptgput->sta3_mac_address);
            IEEE80211_ADDR_COPY(rptgput->sta_top_map[3].mac_address, rptgput->sta4_mac_address);
            break;
        default:
            break;
    }

    for (loop_count = 0; loop_count < sta_count; loop_count++) {
        rptgput->sta_top_map[loop_count].goodput = rpt_assoc_status;
    }

    printk("=========================================\n");
    printk("Received STATUS & ASSOC for this position\n");
    printk("=========================================\n");
    for (loop_count = 0; loop_count < sta_count; loop_count++) {
        printk("STA: %s - LED ASSOC Status %d\n", ether_sprintf(rptgput->sta_top_map[loop_count].mac_address), rptgput->sta_top_map[loop_count].goodput);
    }

    ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_RXPROTOMSG, payload[RPT_BUF_MSG_NUM1]);

    /* Send the rpt goodput table to the socket */ 
    ieee80211_rptplacement_create_msg(ic);
    return 0;
}

/* This function is used to set the goodput table  based on the contents of the 
   RPT_AP_GPT message. It then pushes this information to user space using 
   netlink sockets */
int ieee80211_rptplacement_rx_gput_table(struct ieee80211vap *vap, wbuf_t mywbuf)
{
    u_int32_t sta_count, loop_count;
    struct ieee80211com *ic = vap->iv_ic; 
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    u_int8_t *payload = (u_int8_t *)wbuf_raw_data(mywbuf); 

    sta_count = (payload[RPT_BUF_STA_CNT1] << 24) + (payload[RPT_BUF_STA_CNT2] << 16) +
                (payload[RPT_BUF_STA_CNT3] << 8)  + payload[RPT_BUF_STA_CNT4];

    if (sta_count > MAXNUMSTAS) {
        printk("STA count exceeds MAXNUMSTAS\n");
        return -1;
    }

    printk("\n #STAs = %d\n", sta_count);

    rptgput->sta_count = sta_count;

    for (loop_count = 0; loop_count < sta_count; loop_count++) {
        rptgput->sta_top_map[loop_count].mac_address[0] = payload[RPT_BUF_MAC1_0 + RPT_BUF_MAC1_0*loop_count];
        rptgput->sta_top_map[loop_count].mac_address[1] = payload[RPT_BUF_MAC1_1 + RPT_BUF_MAC1_0*loop_count];
        rptgput->sta_top_map[loop_count].mac_address[2] = payload[RPT_BUF_MAC1_2 + RPT_BUF_MAC1_0*loop_count];
        rptgput->sta_top_map[loop_count].mac_address[3] = payload[RPT_BUF_MAC1_3 + RPT_BUF_MAC1_0*loop_count];
        rptgput->sta_top_map[loop_count].mac_address[4] = payload[RPT_BUF_MAC1_4 + RPT_BUF_MAC1_0*loop_count];
        rptgput->sta_top_map[loop_count].mac_address[5] = payload[RPT_BUF_MAC1_5 + RPT_BUF_MAC1_0*loop_count];
        
        rptgput->sta_top_map[loop_count].goodput = (payload[RPT_BUF_GPUT1 + RPT_BUF_MAC1_0*loop_count] << 24) +
                                                   (payload[RPT_BUF_GPUT2 + RPT_BUF_MAC1_0*loop_count] << 16) +
                                                   (payload[RPT_BUF_GPUT3 + RPT_BUF_MAC1_0*loop_count] << 8)  +
                                                    payload[RPT_BUF_GPUT4 + RPT_BUF_MAC1_0*loop_count];
    }

    ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_RXPROTOMSG, payload[RPT_BUF_MSG_NUM1]);
 
    printk("============================================\n");
    printk("Received  goodput  summary for this position\n");
    printk("============================================\n");
    for (loop_count = 0; loop_count < sta_count; loop_count++) {
        printk("STA: %s - %d kbps\n", ether_sprintf(rptgput->sta_top_map[loop_count].mac_address), rptgput->sta_top_map[loop_count].goodput);
    }

    /* Send the received goodput table to the socket */  
    ieee80211_rptplacement_create_msg(ic);
    return 0;
}

/* This function is used to set the routing table  based on the contents of the 
   AP_STA_ROUTE message. It then pushes this information to user space using 
   netlink sockets */
int ieee80211_rptplacement_rx_routingtable(struct ieee80211vap *vap, wbuf_t mywbuf)
{
    u_int32_t sta_count, loop_count;
    struct ieee80211com *ic = vap->iv_ic; 
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    u_int8_t *payload = (u_int8_t *)wbuf_raw_data(mywbuf);

    if ((payload[RPT_BUF_STA_CNT1] == payload[RPT_BUF_STA_CNT2]) &&
        (payload[RPT_BUF_STA_CNT2] == payload[RPT_BUF_STA_CNT3]) &&
        (payload[RPT_BUF_STA_CNT3] == payload[RPT_BUF_STA_CNT4])) {
        sta_count = payload[RPT_BUF_STA_CNT1] & 0x000000FF;
    } else {
        return -1; 
    } 

    if (sta_count > 4) 
       return -1;

    printk("\n #STAs = %d\n", sta_count);

    rptgput->sta_count = sta_count;

    for (loop_count = 0; loop_count < sta_count; loop_count++) {
        rptgput->sta_top_map[loop_count].mac_address[0] = payload[RPT_BUF_MAC1_0 + RPT_BUF_MAC1_0*loop_count];
        rptgput->sta_top_map[loop_count].mac_address[1] = payload[RPT_BUF_MAC1_1 + RPT_BUF_MAC1_0*loop_count];
        rptgput->sta_top_map[loop_count].mac_address[2] = payload[RPT_BUF_MAC1_2 + RPT_BUF_MAC1_0*loop_count];
        rptgput->sta_top_map[loop_count].mac_address[3] = payload[RPT_BUF_MAC1_3 + RPT_BUF_MAC1_0*loop_count];
        rptgput->sta_top_map[loop_count].mac_address[4] = payload[RPT_BUF_MAC1_4 + RPT_BUF_MAC1_0*loop_count];
        rptgput->sta_top_map[loop_count].mac_address[5] = payload[RPT_BUF_MAC1_5 + RPT_BUF_MAC1_0*loop_count];
        rptgput->sta_top_map[loop_count].goodput = (payload[RPT_BUF_GPUT1 + RPT_BUF_MAC1_0*loop_count] << 24) +
                                                   (payload[RPT_BUF_GPUT2 + RPT_BUF_MAC1_0*loop_count] << 16) +
                                                   (payload[RPT_BUF_GPUT3 + RPT_BUF_MAC1_0*loop_count] << 8)  +
                                                    payload[RPT_BUF_GPUT4 + RPT_BUF_MAC1_0*loop_count];
    }

    printk("======================\n");
    printk("Received Routing table\n");
    printk("======================\n");
    for (loop_count = 0; loop_count < sta_count; loop_count++) {
        printk("STA: %s %d\n", ether_sprintf(rptgput->sta_top_map[loop_count].mac_address), rptgput->sta_top_map[loop_count].goodput);
    }
    ieee80211_rptplacement_set_param(vap, IEEE80211_RPT_RXPROTOMSG, payload[RPT_BUF_MSG_NUM1]);

    /* Send the rpt goodput table  to the socket */
    ieee80211_rptplacement_create_msg(ic);
    return 0;

}

int ieee80211_rptplacement_attach(struct ieee80211com *ic)
{
    int retVal; 
    
    if (!ic) return -EFAULT; 

    ic->ic_rptplacement_init_done=0;

    ic->ic_rptplacementparams = (ieee80211_rptplacement_params_t) 
                     OS_MALLOC(ic->ic_osdev, sizeof(struct ieee80211_rptplacement_params), GFP_KERNEL); 
    
    if (!ic->ic_rptplacementparams) {
        printk("Memory not allocated for ic->ic_rptplacementparams\n"); /* Not expected */
        return -ENOMEM;
    }

    OS_MEMSET(ic->ic_rptplacementparams, 0, sizeof(struct ieee80211_rptplacement_params));  
    ic->ic_rptplacementparams->rpt_devup = 0;

    ic->ic_rptgput = (struct ieee80211_rptgput *) 
                     OS_MALLOC(ic->ic_osdev, sizeof(struct ieee80211_rptgput), GFP_KERNEL);

    if (!ic->ic_rptgput) {
        printk("Memory not allocated for ic->ic_rptgput\n"); /* Not expected */
        OS_FREE(ic->ic_rptplacementparams);
        ic->ic_rptplacementparams = NULL;
        return -ENOMEM;
    }

    /* Init netlink and check for failure */
    ic->ic_rptgput->rptgput_sock = NULL;
    ic->ic_rptgput->rptgput_wbuf = NULL;

    retVal = ieee80211_rptplacement_init_netlink(ic);
	
    if (retVal) { 
        /* init netlink failed. Free up allocated memory and scoot */ 
        printk("netlink_kernel_create failed for ic->ic_rptgput\n"); /* Not expected */
        OS_FREE(ic->ic_rptplacementparams); 
        ic->ic_rptplacementparams = NULL;
        OS_FREE(ic->ic_rptgput);
        ic->ic_rptgput = NULL;
        return retVal;
    }

    ic->ic_rptplacement_init_done=1;

    return 0;
}

int
ieee80211_rptplacement_detach(struct ieee80211com *ic)
{
    int err=0;

    if (!ic) return -EFAULT;

    /* do not care if not initialized */
    if (!ic->ic_rptplacement_init_done) return 0;

    /* Delete netlink socket */
    ieee80211_rptplacement_delete_netlink(ic);
    if(!ic->ic_rptgput) {
        printk("Memory not allocated for ic->ic_rptgput\n"); 
        err=-ENOMEM;
        goto bad;
    } else {
        OS_FREE(ic->ic_rptgput);
        ic->ic_rptgput = NULL;  
    }
    if (!ic->ic_rptplacementparams) {
        printk("Memory not allocated for ic->ic_rptplacementparams\n"); 
        err=-ENOMEM;
        goto bad;
    } else {
        printk("%s ic_rptplacementparams is freed \n", __func__);
        OS_FREE(ic->ic_rptplacementparams); 
        ic->ic_rptplacementparams = NULL;
    }
bad:
   /* we should be doing this only once */
   ic->ic_rptplacement_init_done=0;
   return err;
}

/*Netlink functions used by Repeater Placement Feature*/

/* Funtion that is used to form netlink message, allocate buffer and broadcast */
void
ieee80211_rptplacement_create_msg(struct ieee80211com *ic)
{
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    RPTGPUT_MSG rptgputmsg;
    void *rptgput_nlh;
    int i; 
 
    rptgputmsg.sta_count = ic->ic_rptgput->sta_count;
    for (i = 0; i < MAXNUMSTAS; i++) {
        /* Copy the rptgput data into a local buffer */
        OS_MEMCPY(&rptgputmsg.sta_top_map[i], &ic->ic_rptgput->sta_top_map[i], 
               sizeof(struct sta_topology));
    }
    ieee80211_rptplacement_prep_wbuf(ic);
 
    /* Check if the wbuf is valid */
    if (rptgput->rptgput_wbuf != NULL) {
        rptgput_nlh = wbuf_raw_data(rptgput->rptgput_wbuf);
        /* Copy the data into the wbuf */
        OS_MEMCPY(OS_NLMSG_DATA(rptgput_nlh), &rptgputmsg, OS_NLMSG_SPACE(sizeof(RPTGPUT_MSG)));
        /* Broadcast the rptgput data buffer to the user space */
        ieee80211_rptplacement_bcast_msg(ic);
        /* Reset the data buffer pointer and clear the data buffer */
        memset(&rptgputmsg, 0, sizeof(RPTGPUT_MSG));
    }
}

/* This function is used to allocate buffer for the message that has
   to be sent over the netlink socket */
void
ieee80211_rptplacement_prep_wbuf(struct ieee80211com *ic)
{
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    void *rptgput_nlh;

    /* Allocate the buffer for wbuf */
    rptgput->rptgput_wbuf = wbuf_alloc(ic->ic_osdev, WBUF_MAX_TYPE, 
                                      MAX_RPTGPUT_PAYLOAD);

    if (rptgput->rptgput_wbuf != NULL) {
        /* Change the data pointers */
        wbuf_append(rptgput->rptgput_wbuf, MAX_RPTGPUT_PAYLOAD);
        rptgput_nlh = wbuf_raw_data(rptgput->rptgput_wbuf);
        OS_SET_NETLINK_HEADER(rptgput_nlh, OS_NLMSG_SPACE(sizeof(RPTGPUT_MSG)), 0, 0, 0, 0);

        /* sender is in group 1<<0 */
        wbuf_set_netlink_pid(rptgput->rptgput_wbuf, 0);  /* from kernel */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
        wbuf_set_netlink_dst_pid(rptgput->rptgput_wbuf, 0);  /* multicast */
#endif        

        /* to mcast group 1<<0 */
        wbuf_set_netlink_dst_group(rptgput->rptgput_wbuf, 1);
    } else {
        printk("wbuf allocation failed in rptgput_prep_wbuf\n");
    }
}

/* This function is used to broadcast the rptgput message through the netlink 
   socket */
void
ieee80211_rptplacement_bcast_msg(struct ieee80211com *ic)
{
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;

    if (!ic->ic_rptplacement_init_done) return ;
    if ((rptgput==NULL) || (rptgput->rptgput_sock==NULL) || (rptgput->rptgput_wbuf==NULL)) {
        printk("NULL pointers in rptgput_bcast_msg\n");
        return;
    }
    OS_NETLINK_BCAST(rptgput->rptgput_sock, rptgput->rptgput_wbuf, 0,1, GFP_KERNEL);
}

/* This function initlizes the netlink socket */
int
ieee80211_rptplacement_init_netlink(struct ieee80211com *ic)
{
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;
    
    if (rptgput == NULL) {
        printk("Memory not allocated for ic->ic_rptgput\n"); 
        return -EIO;
    }
    if (rptgput->rptgput_sock == NULL) {
        rptgput->rptgput_sock = OS_NETLINK_CREATE(NETLINK_RPTGPUT, 1, NULL, NULL);
        printk("Netlink socket created for rptgput\n");
        if (rptgput->rptgput_sock == NULL) {
            rptgput->rptgput_sock = OS_NETLINK_CREATE(NETLINK_RPTGPUT+1, 1, NULL, NULL);
            if (rptgput->rptgput_sock == NULL) {
                printk("netlink_kernel_create failed for ic->ic_rptgput\n"); 
                return -ENODEV;
            }
        }
    }
    return 0;
}

/* This function deletes the netlink socket */
int
ieee80211_rptplacement_delete_netlink(struct ieee80211com *ic)
{
    struct ieee80211_rptgput *rptgput = ic->ic_rptgput;

    if((rptgput != NULL) && (rptgput->rptgput_sock != NULL)) {
        OS_SOCKET_RELEASE(rptgput->rptgput_sock);
        rptgput->rptgput_sock = NULL;
    }
    printk("Deleted netlink socket to rpttool\n");
    return 0;
}
#endif
///////////////////////////////////////////////////////////////////////////////
