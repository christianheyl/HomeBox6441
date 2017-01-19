/*
* Copyright (c) 2011 Qualcomm Atheros, Inc..
* All Rights Reserved.
* Qualcomm Atheros Confidential and Proprietary.
*/

#include <osdep.h>
#include <ieee80211_smartantenna_priv.h>
#include <ieee80211_smartantenna.h>
#include "if_athvar.h"

#if UMAC_SUPPORT_SMARTANTENNA

u_int16_t lut_train_pkts[2][MAX_HT_RATES] = {{40, 100, 140, 180, 280, 380, 420, 480, 100, 180, 280, 380, 560, 640, 640, 640, 140, 280, 420, 560, 640, 640, 640, 640},
                                            {100, 200, 300, 400, 580, 640, 640, 640, 200, 400, 580, 640, 640, 640, 640, 640, 300, 580, 640, 640, 640, 640, 640, 640}};

/**  
 *   smartantenna_setdefault_antenna: set tx antenna for all rate sets.
 *   @ni: node to which tx antenna needs to be configured.
 *   @antenna: antenna combination to set.
 *
 *   There is no return value 
 */
void smartantenna_setdefault_antenna(struct ieee80211_node *ni, u_int8_t antenna)
{
    int i = 0;
    u_int8_t temp_ant;
    struct ieee80211com *ic = ni->ni_ic;

    IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_ANY,"Setting current antenna to %s : %d \n", ether_sprintf(ni->ni_macaddr), antenna);

    ni->rate_info.selected_antenna = antenna;
    ni->previous_txant = antenna;
    ic->ic_set_selected_smantennas(ni, antenna);
    for (i = 0; i < ic->ic_smartantennaparams->num_tx_antennas; i++) {
        ni->train_state.ant_map[i] = i;
    }

    temp_ant = ni->train_state.ant_map[0];
    ni->train_state.ant_map[0] = ni->train_state.ant_map[ni->rate_info.selected_antenna];
    ni->train_state.ant_map[ni->rate_info.selected_antenna] = temp_ant;
    IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT," %s ant init - %d %d %d %d\n", __func__
            ,ni->train_state.ant_map[0],ni->train_state.ant_map[1],ni->train_state.ant_map[2]
            , ni->train_state.ant_map[3]);
}

/**  
 *   ieee80211_set_current_antenna: set tx antenna for all nodes in node table.
 *   @vap: handle to vap.
 *   @value: Byte1 - association ID of STA, If Byte1 is zero configures the same antenna for all associated STAs.
 *           Byte0 - antenna value to configure for that STA.
 *
 *   There is no return value 
 */
void ieee80211_set_current_antenna(struct ieee80211vap *vap, u_int32_t value)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node_table  *nt = &ic->ic_sta;
    struct ieee80211_node *ni = NULL, *next = NULL;
    rwlock_state_t lock_state;    
    u_int8_t antenna;
    u_int16_t associd;

    associd = (value & MASK_BYTE1)>>8; /* byte 2 holds AssocID of the node */
    associd |= 0xc000; /* In node join associd is or (|) with 0xc000 */
    antenna = (value & MASK_BYTE0);
    if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
        OS_RWLOCK_READ_LOCK(&nt->nt_nodelock, &lock_state);
        TAILQ_FOREACH_SAFE(ni, &nt->nt_node, ni_list, next) {
            
            if (IEEE80211_ADDR_EQ(ni->ni_macaddr, vap->iv_myaddr)) {
                continue;
            }

             if ((associd == ni->ni_associd) || (0xc000 == associd)) {
                 smartantenna_setdefault_antenna(ni, antenna);
             }
        }
        OS_RWLOCK_READ_UNLOCK(&nt->nt_nodelock, &lock_state);
    }
    else if (vap->iv_opmode == IEEE80211_M_STA) {    /* in sta mode we will transmit to AP only */
         smartantenna_setdefault_antenna(vap->iv_bss, antenna);
    }
}


/**  
 *   ieee80211_display_current_antenna: display tx antenna for all nodes in node table.
 *   @ic: handle to ic object.
 *
 *   There is no return value 
 */
void ieee80211_display_current_antenna(struct ieee80211com *ic)
{
    int selected_antenna;
    struct ieee80211_node_table  *nt = &ic->ic_sta;
    struct ieee80211_node *ni;
    struct ieee80211vap *vap;
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    vap = TAILQ_FIRST(&ic->ic_vaps);

    TAILQ_FOREACH(ni, &nt->nt_node, ni_list) {
        if (IEEE80211_ADDR_EQ(ni->ni_macaddr, vap->iv_myaddr)) {
                continue;
        }
        /* do not display the antenna combination that the chain is not used */ 
        selected_antenna = ni->rate_info.selected_antenna & (scn->sc_ops->get_tx_chainmask(scn->sc_dev));
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY," Tx Antennas for  node: %s > %d PerfTriggers:%d PrdTrigers:%d \n"
                              ,ether_sprintf(ni->ni_macaddr), selected_antenna, ni->perf_trigger_count, ni->prd_trigger_count);
    }
}

/**  
 *   ieee80211_smartantenna_set_param: sets configurable parameters for smart antenna module.
 *   @ic: handle to ic object.
 *   @param : parameter id.
 *   @val   : value.
 *   
 *   returns 0 - If requeste is processed.
 *          -1 - If request is failed to process.
 */
int ieee80211_smartantenna_set_param(struct ieee80211com *ic, int param, u_int32_t val)
{
    struct ieee80211vap *vap;
    u_int8_t antenna;
    int status = 0;
    vap = TAILQ_FIRST(&ic->ic_vaps);
    if (NULL == vap) {	
       return -1;  
    }

    switch (param) {
        case  ATH_PARAM_SMARTANT_TRAIN_MODE:
            ic->ic_smartantennaparams->training_mode = val & SA_TRAINING_MODE_VALID_BITS;
            break;
        case ATH_PARAM_SMARTANT_TRAIN_TYPE:
            ic->ic_smartantennaparams->training_type = val & SA_TRAINING_TYPE_VALID_BITS;
            if(val & MASK_BYTE3) {
                ic->ic_smartantennaparams->lower_bound = (val >> 24) & 0xFF;
            }
            if(val & MASK_BYTE2) {
                ic->ic_smartantennaparams->upper_bound = (val >> 16) & 0xFF;
            }
            if(val & MASK_BYTE1) {
                ic->ic_smartantennaparams->per_diff_threshold = (val >> 8) & 0xFF;
            }
            if(val & MASK_BYTE0) {
                ic->ic_smartantennaparams->config = val  & 0xFF;
            }
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s:  lower bound %d \n", __func__,ic->ic_smartantennaparams->lower_bound);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s:  upper bound %d \n", __func__,ic->ic_smartantennaparams->upper_bound);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s:  per_diff_threshold %d \n", __func__,ic->ic_smartantennaparams->per_diff_threshold);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s:  config %x\n", __func__,ic->ic_smartantennaparams->config);
            break;
        case ATH_PARAM_SMARTANT_PKT_LEN:
             ic->ic_smartantennaparams->packetlen = val;
             break;
        case ATH_PARAM_SMARTANT_NUM_PKTS:
             ic->ic_smartantennaparams->n_packets_to_train = val;
             break;
        case ATH_PARAM_SMARTANT_TRAIN_START:
             if (SA_ENABLE_HW_ALGO == ic->ic_get_smartantenna_enable(ic)) {
                 ic->ic_smartantennaparams->training_start = val;
                 ieee80211_smartantenna_training_init(vap);
             } else {
                 IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s:smart antenna is disabled !!! \n", __func__);
             }
             break;
        case ATH_PARAM_SMARTANT_NUM_ITR:
             ic->ic_smartantennaparams->hysteresis = val;
             break;
        case ATH_PARAM_SMARTANT_TRAFFIC_GEN_TIMER:
             ic->ic_smartantennaparams->traffic_gen_timer = val;
             break;
        case ATH_PARAM_SMARTANT_RETRAIN:
             if (SA_ENABLE_HW_ALGO == ic->ic_get_smartantenna_enable(ic)) {
                 if (val < RETRAIN_INTERVEL) {
                     if (val) {
                         val = RETRAIN_INTERVEL;
                         IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s: Minimum Retrain interval is %d,Configuiring Retrain inerval to: %d \n"
                                 , __func__, RETRAIN_INTERVEL, val);
                     } else {
                         IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s: Retraining is disabled \n", __func__);
                     }
                 }

                 ic->ic_smartantennaparams->retraining_enable = val;
                 if (0 == ic->ic_smartantennaparams->retraining_enable) {
                     ic->ic_sa_retraintimer = 0;
                 } else {
                     if (0 == ic->ic_sa_retraintimer) {
                         ic->ic_sa_retraintimer = 1;
                         OS_SET_TIMER(&ic->ic_smartant_retrain_timer, ic->ic_smartantennaparams->retraining_enable);
                     }
                 }
             } else {
                 IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s: smart antenna is disabled !!! \n", __func__);
             }
             break;
        case ATH_PARAM_SMARTANT_RETRAIN_THRESHOLD:
             ic->ic_smartantennaparams->num_pkt_threshold = val;
             break;
        case ATH_PARAM_SMARTANT_RETRAIN_INTERVAL:
             ic->ic_smartantennaparams->retrain_interval = val;
             break;
        case ATH_PARAM_SMARTANT_RETRAIN_DROP:
             if (val < SA_MAX_PERCENTAGE) {
                ic->ic_smartantennaparams->max_throughput_change = val;
             } else {
                 IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s: Threshold should be below 100 (in percentage) !!! \n", __func__);
             }
             break;
        case ATH_PARAM_SMARTANT_MIN_GOODPUT_THRESHOLD:
             if (val > 0 ) {
                ic->ic_smartantennaparams->min_goodput_threshold = val;
             } else {
                 IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s: Threshold should be above zero  \n", __func__);
             }
            break; 
        case ATH_PARAM_SMARTANT_GOODPUT_AVG_INTERVAL:
             if (val > 0 ) {
                ic->ic_smartantennaparams->goodput_avg_interval = val;
             } else {
                 IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s: Average Interval should be above zero  \n", __func__);
             }
            break; 
        case ATH_PARAM_SMARTANT_CURRENT_ANTENNA:
             antenna = (val & MASK_BYTE0);
             if(antenna <= ic->ic_smartantennaparams->num_tx_antennas) {
                 ieee80211_set_current_antenna(vap, val);
             } else {
                 IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s: Not a valid antenna combination !!! \n", __func__);
             }
             break;
        case ATH_PARAM_SMARTANT_DEFAULT_ANTENNA:
             if(val <= ic->ic_smartantennaparams->num_tx_antennas) {
                 ic->ic_set_default_antenna(ic, val);
             } else {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s: Not a valid antenna combination !!! \n", __func__);
             }
            break;
       
        default:
            status = -1;
            break;
    }
	
    return status;
}

/**  
 *   ieee80211_smartantenna_get_param: displays/returns configurable parameters value for smart antenna module.
 *   @ic: handle to ic object.
 *   @param: parameter id. 
 *   
 *   returns - value of requested parameter.
 *           - 0 in case of error/invalid parameter. 
 */
u_int32_t ieee80211_smartantenna_get_param(struct ieee80211com *ic, int param)
{
    struct ieee80211vap *vap;
    u_int32_t val = 0;
    
    vap = TAILQ_FIRST(&ic->ic_vaps);
    if (NULL == vap) {	
       return 0;  
    }

    switch (param) {
        case ATH_PARAM_SMARTANT_TRAIN_MODE:
            val = ic->ic_smartantennaparams->training_mode;
            break;
        case ATH_PARAM_SMARTANT_TRAIN_TYPE:
            val = ic->ic_smartantennaparams->config;
            val |=  ic->ic_smartantennaparams->lower_bound << 24;
            val |=  ic->ic_smartantennaparams->upper_bound << 16;
            val |=  ic->ic_smartantennaparams->per_diff_threshold << 8;
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s:  lower bound %d \n", __func__,ic->ic_smartantennaparams->lower_bound);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s:  upper bound %d \n", __func__,ic->ic_smartantennaparams->upper_bound);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s:  per_diff_threshold %d \n", __func__,ic->ic_smartantennaparams->per_diff_threshold);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s:  config %x\n", __func__,ic->ic_smartantennaparams->config);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"%s: train params %X \n", __func__,val);
            break;
        case ATH_PARAM_SMARTANT_PKT_LEN:
             val = ic->ic_smartantennaparams->packetlen;
             break;
        case ATH_PARAM_SMARTANT_NUM_PKTS:
             val = ic->ic_smartantennaparams->n_packets_to_train;
             break;
             break;
        case ATH_PARAM_SMARTANT_TRAIN_START:
             val = ic->ic_smartantennaparams->training_start;
             break;
        case ATH_PARAM_SMARTANT_NUM_ITR:
             val = ic->ic_smartantennaparams->hysteresis;
             break;
        case ATH_PARAM_SMARTANT_TRAFFIC_GEN_TIMER:
             val = ic->ic_smartantennaparams->traffic_gen_timer;
             break;
        case ATH_PARAM_SMARTANT_RETRAIN:
             val = ic->ic_smartantennaparams->retraining_enable;
             break;
        case ATH_PARAM_SMARTANT_RETRAIN_THRESHOLD:
             val = ic->ic_smartantennaparams->num_pkt_threshold;
             break;
        case ATH_PARAM_SMARTANT_RETRAIN_INTERVAL:
             val = ic->ic_smartantennaparams->retrain_interval;
             break;
        case ATH_PARAM_SMARTANT_RETRAIN_DROP:
             val = ic->ic_smartantennaparams->max_throughput_change;
             break;
        case ATH_PARAM_SMARTANT_MIN_GOODPUT_THRESHOLD:
             val = ic->ic_smartantennaparams->min_goodput_threshold;
             break;
        case ATH_PARAM_SMARTANT_GOODPUT_AVG_INTERVAL:
             val = ic->ic_smartantennaparams->goodput_avg_interval;
             break;
        case ATH_PARAM_SMARTANT_CURRENT_ANTENNA:
             ieee80211_display_current_antenna(ic);
             val = MASK_BYTE0; 
             break;
        case ATH_PARAM_SMARTANT_DEFAULT_ANTENNA:
             val = ic->ic_get_default_antenna(ic);
             break;
        default:
            break;
    }
    return val;
}


/**  
 *   get_num_trainpkts: computes number of training packets required for training based on rate and channel width.
 *   @ni: handle to node.
 *
 *   returns - number of packets.
 */
u_int32_t get_num_trainpkts(struct ieee80211_node *ni)
{
    u_int32_t num_trainpkts;

    if(IS_HT_RATE(ni->train_state.rate_code)) {
        if (ni->ni_htcap & IEEE80211_HTCAP_C_CHWIDTH40) {
            num_trainpkts = lut_train_pkts[1][(ni->train_state.rate_code & RATECODE_TO_MCS_MASK)];
        } else {
            num_trainpkts = lut_train_pkts[0][(ni->train_state.rate_code & RATECODE_TO_MCS_MASK)];
        }
    } else {
        num_trainpkts = SA_NUM_LEGACY_TRAIN_PKTS;
    }

    if (ni->ni_ic->ic_smartantennaparams->n_packets_to_train)
        num_trainpkts = ni->ni_ic->ic_smartantennaparams->n_packets_to_train;
  
    if (ni->ni_ic->ic_smartantennaparams->config & SA_CONFIG_INTENSETRAIN) {
        if(ni->intense_train) {
            num_trainpkts = (num_trainpkts * 2) > SA_MAX_INTENSETRAIN_PKTS ? SA_MAX_INTENSETRAIN_PKTS : (num_trainpkts *2);
        }
    }

    if (ni->ni_ic->ic_smartantennaparams->config & SA_CONFIG_SLECTSPROTALL) {
        num_trainpkts |= SA_CTS_PROT;
    } else if ((ni->train_state.extra_stats) && (ni->ni_ic->ic_smartantennaparams->config & SA_CONFIG_SLECTSPROTEXTRA)) {
        num_trainpkts |= SA_CTS_PROT;
    }

    return num_trainpkts;
}

#ifdef not_yet

/**  
 *   ieee80211_smartantenna_input: This function is used to check the incoming packets for ATH_ETH_TYPE packets (0x88BD)
 *      and look for custom protocol packets (ATH_ETH_TYPE_SMARTANTENNA_PROTO (21)) used in smart antenna training.
 *   @vap: handle to vap.
 *   @wbuf: handle to wireless buffer.
 *   @eh: pointer to ethernet header.
 *   @rs: recived wireless frame status.
 *
 *   There is no return value 
 */
void
ieee80211_smartantenna_input(struct ieee80211vap *vap, wbuf_t wbuf, struct ether_header *eh, struct ieee80211_rx_status *rs)
{
    struct athl2p_tunnel_hdr *tunHdr;
    struct ieee80211com *ic = vap->iv_ic;
    if (!ic->ic_smartantenna_init_done) return;

    if (eh->ether_type == ATH_ETH_TYPE) 
    {
        wbuf_pull(wbuf, (u_int16_t) (sizeof(*eh)));
        tunHdr = (struct athl2p_tunnel_hdr *)(wbuf_header(wbuf));
        switch (tunHdr->proto)
        {
            case ATH_ETH_TYPE_SMARTANTENNA_PROTO:
                ieee80211_smartantenna_rx_proto_msg(vap, wbuf, rs);
                break;
    
            case ATH_ETH_TYPE_SMARTANTENNA_TRAINING:
                break;

            default:
                break;     
        }
        wbuf_push(wbuf, (u_int16_t) (sizeof(*eh)));
    }
}

/**  
 *   ieee80211_smartantenna_rx_proto_msg: This function is used to process recieved smart antenna protocol frames.
 *   @vap: handle to vap.
 *   @wbuf: handle to wireless buffer.
 *   @rs: recived wireless frame status.
 *
 *   There is no return value 
 */
void ieee80211_smartantenna_rx_proto_msg(struct ieee80211vap *vap, wbuf_t mywbuf, struct ieee80211_rx_status *rs)
{
    u_int8_t *payload;
    u_int8_t command;
    payload = (u_int8_t *)wbuf_raw_data(mywbuf);
    command = payload[4];

    switch(command)
    {
        default:
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s: recieved UNKNOWN Message \n",__func__);
    }
}

/** 
 *   ieee80211_smartantenna_tx_custom_proto_pkt: This function is used to send custom protocol packets to a particular node.
 *   @vap: handle to vap.
 *   @cpp: handle to custom parameters used in preparing smart atenna protocol packet.
 *
 *   returns -  0 after successfully transmitting custom protocol packet.
 *           - -1 in case of error. 
 */
int ieee80211_smartantenna_tx_custom_proto_pkt(struct ieee80211vap *vap, struct custom_pkt_params *cpp)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ether_header *eh;			
    struct athl2p_tunnel_hdr *tunHdr;		
    int    msg_len;
    wbuf_t mywbuf;
    struct net_device *netd;
    struct ieee80211_node *ni;
    int ret;
    ni = ieee80211_find_node(&ic->ic_sta, cpp->dst_mac_address);
    
    if (ni == NULL) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s: Not able to find node for tx \n", __func__);
        return -1;
    }

    netd = (struct net_device *) wlan_vap_get_registered_handle(vap);

    /* Set msg length to the largest protocol msg payload size */
    msg_len = cpp->num_data_words; 

    mywbuf = wbuf_alloc(ic->ic_osdev,  WBUF_TX_DATA, msg_len);

    if (!mywbuf) {
        printk("wbuf allocation failed in tx_custom_pkt\n");
        ieee80211_free_node(ni); 
        return -1;
    }
    /* Copy protocol messsage data */
    wbuf_append(mywbuf,msg_len);
    OS_MEMCPY((u_int8_t *)(wbuf_raw_data(mywbuf)), cpp->msg_data, msg_len); 

    tunHdr = (struct athl2p_tunnel_hdr *) wbuf_push(mywbuf, sizeof(struct athl2p_tunnel_hdr));
    eh     = (struct ether_header *) wbuf_push(mywbuf, sizeof(struct ether_header));

    /* ATH_ETH_TYPE protocol subtype */
    tunHdr->proto = ATH_ETH_TYPE_SMARTANTENNA_PROTO;
    /* Copy the SRC & DST MAC addresses into ethernet header*/ 
    IEEE80211_ADDR_COPY(&eh->ether_shost[0], cpp->src_mac_address);
    IEEE80211_ADDR_COPY(&eh->ether_dhost[0], cpp->dst_mac_address);
    /* copy ethertype */
    eh->ether_type = htons(ATH_ETH_TYPE);
    mywbuf->dev = netd;
    ret = ieee80211_send_wbuf(vap, ni, mywbuf);
    ieee80211_free_node(ni); 
    return 0;
}   

#endif

/** 
 *   ieee80211_smartantenna_tx_custom_data_pkt: This function is used to send custom training packets to a particular node.
 *   @vap: handle to vap.
 *   @cpp: handle to custom parameters used in preparing training packet.
 *
 *   returns -  0 after successfully transmitting custom training packet.
 *           - -1 in case of error. 
 */
int 
ieee80211_smartantenna_tx_custom_data_pkt(struct ieee80211_node *ni , struct ieee80211vap *vap, struct custom_pkt_params *cpp,
                                          u_int8_t antenna, u_int8_t rateidx, u_int8_t is_last)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ether_header *eh;			
    struct athl2p_tunnel_hdr *tunHdr;		
    u_int  msg_len;
    wbuf_t mywbuf;
    struct net_device *netd;

    netd = (struct net_device *) wlan_vap_get_registered_handle(vap);

    msg_len = ic->ic_smartantennaparams->packetlen; 

    mywbuf = wbuf_alloc(ic->ic_osdev,  WBUF_TX_DATA, msg_len);

    if (!mywbuf) {
        printk("wbuf allocation failed in tx_custom_pkt\n");
        return -1;
    }

    IEEE80211_ADDR_COPY(cpp->dst_mac_address,ni->ni_macaddr);

    /* In AP Mode */
    if ((vap->iv_opmode == IEEE80211_M_HOSTAP)) {
        IEEE80211_ADDR_COPY(cpp->src_mac_address, ni->ni_bssid);
    } else if ((vap->iv_opmode == IEEE80211_M_STA)) {
        /* In STA Mode */
        IEEE80211_ADDR_COPY(cpp->src_mac_address,vap->iv_myaddr);
    }

    /* Initialize */
    wbuf_append(mywbuf, msg_len);
    memset((u_int8_t *)wbuf_raw_data(mywbuf), 0, msg_len);

    tunHdr = (struct athl2p_tunnel_hdr *) wbuf_push(mywbuf, sizeof(struct athl2p_tunnel_hdr));
    eh     = (struct ether_header *) wbuf_push(mywbuf, sizeof(struct ether_header));

    tunHdr->proto = ATH_ETH_TYPE_SMARTANTENNA_TRAINING;

    IEEE80211_ADDR_COPY(&eh->ether_shost[0], cpp->src_mac_address);
    IEEE80211_ADDR_COPY(&eh->ether_dhost[0], cpp->dst_mac_address);

    eh->ether_type = htons(ATH_ETH_TYPE);
    mywbuf->dev = netd;
    
    wbuf_set_priority(mywbuf,WME_AC_BK);
    wbuf_set_tid(mywbuf,SMARTANT_TRAIN_TID);
    wbuf_sa_set_train_packet(mywbuf);

    ieee80211_send_wbuf(vap, ni, mywbuf);  
    return 0;
}

#ifdef not_yet

/** 
 *   ieee80211_smartantenna_tx_proto_msg: This function is used to construct a custom protocol message for transmission.
 *     It populates the custom_pkt_params structure and hands it over to the ieee80211_smartantenna_tx_custom_proto_pkt function.
 *   @vap: handle to vap.
 *   @ni: handle to node.
 *   @msg_num: message id.
 *   @txantenna: transmit antenna combination.
 *   @rxantenna: recieve antenna combination.
 *   @ratecode: rate code.
 *
 *   returns -  0 after successfully transmitting custom training packet.
 *           - -1 in case of error. 
 */
int ieee80211_smartantenna_tx_proto_msg(struct ieee80211vap *vap, struct ieee80211_node *ni , u_int8_t msg_num, u_int8_t txantenna, u_int8_t rxantenna, u_int8_t ratecode)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct custom_pkt_params cpp; 
    /* In AP Mode */
    if ((vap->iv_opmode == IEEE80211_M_HOSTAP)) {
        IEEE80211_ADDR_COPY(cpp.dst_mac_address,ni->ni_macaddr);
        IEEE80211_ADDR_COPY(cpp.src_mac_address, ni->ni_bssid);
    } else if ((vap->iv_opmode == IEEE80211_M_STA)) {
        /* In STA Mode */
        IEEE80211_ADDR_COPY(cpp.dst_mac_address,ni->ni_macaddr);
        IEEE80211_ADDR_COPY(cpp.src_mac_address,vap->iv_myaddr);
    }

    cpp.msg_num = msg_num; 

    switch (msg_num) 
    {
        case SAMRTANT_SET_ANTENNA: 
         cpp.antenna = rxantenna;
         cpp.num_data_words = 6;
         cpp.msg_data    = (u_int8_t *) OS_MALLOC(ic->ic_osdev, sizeof(u_int8_t)*cpp.num_data_words, GFP_KERNEL);
         cpp.msg_data[0] = msg_num;
         cpp.msg_data[1] = txantenna;
         cpp.msg_data[2] = rxantenna;
         cpp.msg_data[3] = ratecode;
            break;
        case SAMRTANT_RECV_RXSTATS: 
         cpp.num_data_words = sizeof(struct per_ratetable)+sizeof(u_int32_t);
         cpp.msg_data    = (u_int8_t *) OS_MALLOC(ic->ic_osdev, sizeof(u_int8_t)*cpp.num_data_words, GFP_KERNEL);
         cpp.msg_data[0] = msg_num;
         OS_MEMCPY((u_int8_t *)&cpp.msg_data[1],(u_int8_t *)&rxpermap,sizeof(rxpermap)); 
            break;
        case SAMRTANT_SEND_RXSTATS: 
         cpp.antenna = rxantenna;
         cpp.num_data_words = 6;
         cpp.msg_data    = (u_int8_t *) OS_MALLOC(ic->ic_osdev, sizeof(u_int8_t)*cpp.num_data_words, GFP_KERNEL);
         cpp.msg_data[0] = msg_num;
         cpp.msg_data[1] = txantenna;
         cpp.msg_data[2] = rxantenna;
         cpp.msg_data[3] = ratecode;
            break;
        default:
            /* need to exit from the func */
            cpp.num_data_words = 0;
            break;
    }
    if (cpp.num_data_words == 0) {
        return -1;
    }

    /*
     * send protocol packet with rate control reliable rate
     */
    ieee80211_smartantenna_tx_custom_proto_pkt(vap, &cpp);

    if (cpp.msg_data != NULL) {
         OS_FREE(cpp.msg_data);
    }

    return 0;
}
#endif

/** 
 *   ieee80211_smartantenna_node_init: This function is entry point to initiate training for a particular node.
 *   @ni: handle to node.
 *
 *   There is no return value 
 */
void ieee80211_smartantenna_node_init(struct ieee80211_node *ni)
{
    struct ieee80211com           *ic = ni->ni_ic;

    ni->smartantenna_state = SMARTANTENNA_PRETRAIN;
    if ((SA_ENABLE_HW_ALGO == ic->ic_get_smartantenna_enable(ic))) {
        if (ni->ni_vap->iv_opmode == IEEE80211_M_STA) {
            ni->hybrid_train = 1;
            if (!ic->ic_smartantennaparams->traffic_gen_timer) {
                ic->ic_smartantennaparams->traffic_gen_timer = DEFAULT_STA_TRAFFIC_GEN_TIMER;
            }
        } else {
            if (!ic->ic_smartantennaparams->traffic_gen_timer) {
                ic->ic_smartantennaparams->traffic_gen_timer = DEFAULT_AP_TRAFFIC_GEN_TIMER;
            }
        }
        ni->intense_train = 1;
        sa_set_traffic_gen_timer(ic, ni);
        
        ic->ic_prepare_rateset(ic, ni);
        ni->prev_ratemax = ni->rate_info.rates[0].ratecode; /* initializing rate max to min valid rate code */
    }
}

/** 
 *   ieee80211_smartantenna_training_init: This function initates training for all the nodes present in node list.  
 *      this function is called in iwpriv context, used to intiate manual training.
 *   @vap: handle to vap.
 *
 *   There is no return value 
 */
void ieee80211_smartantenna_training_init(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node_table  *nt = &ic->ic_sta;
    struct ieee80211_node *ni;
    u_int8_t i = 0, temp_ant;

    TAILQ_FOREACH(ni, &nt->nt_node, ni_list) {
        if (IEEE80211_ADDR_EQ(ni->ni_macaddr, vap->iv_myaddr)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_DEBUG, "%s : Skipping Training for Own VAP: %s \n", __func__ , ether_sprintf(ni->ni_macaddr));
            continue;
        }

        if (!ieee80211_node_is_authorized(ni)) {
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT, "Node is not authorised MAC : %s \n",ether_sprintf(ni->ni_macaddr));
            continue;
        }
        
        ieee80211_ref_node(ni);

        if (ni->is_training == 0) {
            ni->is_training = 1;

            for (i = 0; i < ic->ic_smartantennaparams->num_tx_antennas; i++) {
                    ni->train_state.ant_map[i] = i;
            }

            temp_ant = ni->train_state.ant_map[0];
            ni->train_state.ant_map[0] = ni->train_state.ant_map[ni->rate_info.selected_antenna];
            ni->train_state.ant_map[ni->rate_info.selected_antenna] = temp_ant;
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT, "ant init - %d %d %d %d\n"
                                                  ,ni->train_state.ant_map[0], ni->train_state.ant_map[1]
                                                  ,ni->train_state.ant_map[2], ni->train_state.ant_map[3]);
            ieee80211_smartantenna_init_training(ni, ic);
        }

        ieee80211_free_node(ni);
    }  /* end of loop*/

    if (ic->ic_smartantennaparams->retraining_enable) {
        if (0 == ic->ic_sa_retraintimer) {
            ic->ic_sa_retraintimer = 1;
            OS_SET_TIMER(&ic->ic_smartant_retrain_timer, ic->ic_smartantennaparams->retraining_enable);
        }
    }

}

/** 
 *   find_min_rssi : This function finds the minimum rssi value for particular index in 2-dimentional rssi array.  
 *      this function is called in iwpriv context, used to intiate manual training.
 *   @base_addr : base address of multi dimentional array.
 *   @index : index of RSSI set.
 *   @ret: returns minimum value found in the array 
 */
static inline int8_t find_min_rssi(int8_t *base_addr, int index)
{
    int i;
    int8_t min = base_addr[index];

    for (i = 1; i < SA_MAX_RECV_CHAINS; i++) {
        if ((base_addr[(i*SA_MAX_RSSI_SAMPLES) + index] >= 0) && (min > base_addr[(i*SA_MAX_RSSI_SAMPLES) + index])) {
            min = base_addr[(i*SA_MAX_RSSI_SAMPLES) + index];
        }
    }
    return min;
}

/** 
 *   find_max_rssi : This function finds the maximum rssi value for particular inndex in 2-dimentional rssi array.  
 *      this function is called in iwpriv context, used to intiate manual training.
 *   @base_addr : base address of multi dimentional array.
 *   @index : index of RSSI set.
 *   @ret: returns minimum value found in the array 
 */
static inline int8_t find_max_rssi(int8_t *base_addr, int index)
{
    int i;
    int8_t max = base_addr[index];

    for (i = 1; i < SA_MAX_RECV_CHAINS; i++) {
        if (max < base_addr[(i*SA_MAX_RSSI_SAMPLES) + index]) {
            max = base_addr[(i*SA_MAX_RSSI_SAMPLES) + index];
        }
    }
    return max;
}

/** 
 *   MAXIMUM : This function finds the maximum of two inputs  
 *   @a: input value 1
 *   @b: input value 2
 *   @ret: returns maximul value
 */

inline int MAXIMUM(int a, int b) 
{
      return (a > b ? a : b);
}


/** 
 *   smartantenna_retrain_check: This function used to determine whether retraining is required for a particular node or not.
 *      this function has two modes, 
 *      mode 0 is used to get most pridomently rate used.
 *      mode 1 is used to determine whether retraining is required for that particular node.
 *   @ni: handle to node.
 *   @mode: mode of smartantenna_retrain_check function.
 *   @npackets: number of packets transmited to that particular node durining retrain interval.
 *
 *   returns - most predominantly used rate in retrain interval
 *           - 0 if retraining is not required.
 */
int smartantenna_retrain_check(struct ieee80211_node *ni, int mode, u_int32_t* npackets)
{
   u_int32_t  *ratestats, pratestats, max = 0;
   int i=0, nsend =0, diff = 0, threshold = 0;
   u_int8_t maxidx = 0,max_ratecode;
   struct ieee80211com *ic = ni->ni_ic;

   ni->current_goodput = ic->ic_get_smartantenna_ratestats(ni, &pratestats);
   ratestats = (u_int32_t *)pratestats;

   if (mode) {
       if (ni->gput_avg_interval < (SA_GPUT_INGNORE_INTRVAL+ic->ic_smartantennaparams->goodput_avg_interval)) {

           if (ni->gput_avg_interval >= SA_GPUT_INGNORE_INTRVAL) {
               if (ni->prev_goodput == 0) {
                   ni->prev_goodput = ni->current_goodput;
               } else {
                   ni->prev_goodput = (ni->prev_goodput+ni->current_goodput) >> 1;
               }
           }
           ni->gput_avg_interval++;
           return 0;
       }
   }

   for(i = 0; i < ni->rate_info.num_of_rates; i++) {
#if (SA_DEBUG == 2)
        IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"RateCode: 0x%02x - stats: %d \n",ni->rate_info.rates[i].ratecode, ratestats[i]);
#endif            
        if(ratestats[i]) {
            nsend=1;
        }
        if(max < ratestats[i]) {
            max=ratestats[i];
            maxidx=i;
        }
   }

   /* Clear rate stats in Lmac */
   OS_MEMZERO(ratestats, SA_MAX_RATES * sizeof(u_int32_t));
   max_ratecode = ni->rate_info.rates[maxidx].ratecode;
   
   if (!nsend) {
       max_ratecode = ni->prev_ratemax;
   }
   
   ni->prev_ratemax = max_ratecode; 

   *npackets = max;
   if (mode == 0)
       return max_ratecode;


    threshold = (ni->prev_goodput * ic->ic_smartantennaparams->max_throughput_change) / 100;
    /* 
        Threshold should be  max of %max_throughput_change of goodput or min_goodput_threshold 
        If goodput are less then %drop of goodput will be zero to avoid these condations min_goodput_threshold is configured to MIN_GOODPUT_THRESHOLD
    */
    threshold = MAXIMUM(threshold, ic->ic_smartantennaparams->min_goodput_threshold);
    diff = ni->prev_goodput > ni->current_goodput ? ni->prev_goodput-ni->current_goodput : ni->current_goodput - ni->prev_goodput;

    if (diff >= threshold) { /* throught put max_throughput_change by %drop of previous intervel good put*/
        
        /* Need to consider both directions */
        switch (ni->retrain_trigger_type)
        {
            case 0:
                ni->hysteresis++;
                if (ni->prev_goodput < ni->current_goodput)  { /* +ve trigger */
                    ni->retrain_trigger_type = 1;
                } else {
                    ni->retrain_trigger_type = -1;
                }
                break;
            case 1:
                if (ni->prev_goodput < ni->current_goodput)  { /* +ve trigger */
                    ni->hysteresis++;
                } else {
                    ni->hysteresis = 0; /* Reset hysteresis */
                    ni->retrain_trigger_type = 0;
                }
                break;
            case -1:
                if (ni->prev_goodput > ni->current_goodput)  { /* -ve trigger */
                    ni->hysteresis++;
                } else {
                    ni->hysteresis = 0; /* Reset hysteresis */
                    ni->retrain_trigger_type = 0;
                }
                break;
        }

        if (ni->hysteresis >= ic->ic_smartantennaparams->hysteresis) {
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT," Previous Goodput : %d : Current Goodput %d hysteresis: %d\n",
                                        ni->prev_goodput, ni->current_goodput, ni->hysteresis);
            ni->hysteresis = 0;
            ni->prev_goodput = 0;
            ni->gput_avg_interval = 0;
            ni->retrain_trigger_type = 0;
            return max_ratecode;
        }
    } else {
        ni->hysteresis = 0; /* reset Hysteresis */
        ni->retrain_trigger_type = 0;
        /* If not is retrain tigger monitor window */
        /* weighted average of good put 7/8 of previous goodput and 1/8 of current good put */ 
        ni->prev_goodput = (((7*ni->prev_goodput)+ni->current_goodput) >> 3);
    }
    IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT," Previous Goodput : %d : Current Goodput %d hysteresis: %d\n",
            ni->prev_goodput, ni->current_goodput, ni->hysteresis);

    return 0; 
}

/** 
 *   smartantenna_retrain_handler: This timer function loop through each node present in nodelist and triggers training if required.
 *   @timerargument: handle to ic is argument for this timer function.
 *
 *   There is no return value 
 */
static OS_TIMER_FUNC(smartantenna_retrain_handler)
{
    struct ieee80211com *ic;
    struct ieee80211_node *ni;
    int diff, rateCode;
    u_int32_t npkt;
    struct ieee80211_node_table  *nt;
    OS_GET_TIMER_ARG(ic, struct ieee80211com *);

    nt  = &ic->ic_sta;
    TAILQ_FOREACH(ni, &nt->nt_node, ni_list) {
        if (!ic->ic_get_smartantenna_enable(ic)) {
            continue;
        }
        if (IEEE80211_ADDR_EQ(ni->ni_macaddr, ni->ni_vap->iv_myaddr)) {
            continue;
        }

        /* if node is authorised */
        if (!ieee80211_node_is_authorized(ni)) {
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT, "Node is not authorised MAC : %s \n",ether_sprintf(ni->ni_macaddr));
            continue;
        }
        ieee80211_ref_node(ni); 

        if ((ni->smartantenna_state == SMARTANTENNA_TRAIN_INPROGRESS) || 
            ((ni->smartantenna_state == SMARTANTENNA_PRETRAIN) && (ni->ni_vap->iv_opmode != IEEE80211_M_STA))) {
            ieee80211_free_node(ni);
            continue;
        }

        IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT, "Retrain check for MAC : %s \n",ether_sprintf(ni->ni_macaddr));
        IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT, "Tx unicast frames (previous): %d  Tx Unicast Frames(current):%d \n"
                ,ni->ns_prev_tx_ucast, ni->ni_stats.ns_tx_ucast);

        diff = ni->ni_stats.ns_tx_ucast-ni->ns_prev_tx_ucast;
        diff = (diff>0) ? diff:-diff;
        if(diff > ic->ic_smartantennaparams->num_pkt_threshold) {
            /*
             *  Data traffic is going; 
             */
            /* update the total trasmited packets */ 
            ni->ns_prev_tx_ucast = ni->ni_stats.ns_tx_ucast;
            if((ni->retrain_miss * ic->ic_smartantennaparams->retraining_enable) >= (ic->ic_smartantennaparams->retrain_interval * 60 * 1000)) { 
                rateCode = smartantenna_retrain_check(ni, 0, &npkt); /* get the rate used from lmac */
#if SA_DEBUG
                ni->prd_trigger_count++;
#endif
                IEEE80211_DPRINTF(ni->ni_vap,IEEE80211_MSG_ANY, "TS mac: %s rssi %d rateCode 0x%02x Periodic Trigger\n",ether_sprintf(ni->ni_macaddr), ni->ni_rssi, rateCode);
            } else {
                rateCode = smartantenna_retrain_check(ni, 1, &npkt);
            }

            if (rateCode) {
                /* need to retrain */
#if SA_DEBUG
                ni->perf_trigger_count++;
#endif
                IEEE80211_DPRINTF(ni->ni_vap,IEEE80211_MSG_ANY, "TS mac: %s rssi %d rateCode 0x%02x Performance Trigger\n",ether_sprintf(ni->ni_macaddr), ni->ni_rssi, rateCode);
            } else {
                ni->retrain_miss++;  /* traffic present and training not happened */
                ieee80211_free_node(ni);
                continue;
            }

        } else {  

            ni->retrain_miss++;
            if((ni->retrain_miss * ic->ic_smartantennaparams->retraining_enable) >= (ic->ic_smartantennaparams->retrain_interval * 60 * 1000)) { 
                rateCode = smartantenna_retrain_check(ni, 0, &npkt);
#if SA_DEBUG
                ni->prd_trigger_count++;
#endif
                IEEE80211_DPRINTF(ni->ni_vap,IEEE80211_MSG_ANY, "TS mac: %s rssi %d rateCode 0x%02x Periodic Tigger\n",ether_sprintf(ni->ni_macaddr), ni->ni_rssi, rateCode);
                IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT, "--- Trigger hapended with no traffic --- \n", rateCode);
                ni->retrain_miss = 0;
                ni->ns_prev_tx_ucast = ni->ni_stats.ns_tx_ucast = 0;
                ni->smartantenna_state = SMARTANTENNA_PRETRAIN;
                if (ni->ni_vap->iv_opmode == IEEE80211_M_STA) {
                    ni->long_retrain_count++;
                    if (ni->long_retrain_count > SA_MAX_PRD_RETRAIN_MISS) {
                        ni->hybrid_train = 1;
                    }
                }
                sa_set_traffic_gen_timer(ic, ni);
            } 
            ieee80211_free_node(ni);
            continue;
        }

        /* start training */
        if (ni->is_training == 0) {
            ni->is_training = 1;
            /* Program Node for Training with antenna as current antenna
               and rate as most predominantly used rate in this intervel */

            ni->train_state.antenna = 0;
            ni->train_state.rate_code = rateCode;
            ni->current_rate_index = get_rateindex(ni,rateCode);
            ni->train_state.rateidx = ni->current_rate_index;
            ni->train_state.first_per = 1;
            ni->retrain_miss = 0;
#if SA_DEBUG
            ni->ts_trainstart = ic->ic_get_TSF32(ic);
            IEEE80211_DPRINTF(ni->ni_vap,IEEE80211_MSG_ANY, "Mac:%s Prd %d Per %d ", ether_sprintf(ni->ni_macaddr), ni->prd_trigger_count,ni->perf_trigger_count);
#endif
            IEEE80211_DPRINTF(ni->ni_vap,IEEE80211_MSG_ANY, "\n");
            
            ni->train_state.extra_sel = 0;
            ni->train_state.extra_cmp = 0;
            ic->ic_set_sa_train_params(ni, ni->train_state.ant_map[ni->train_state.antenna], ni->current_rate_index, 
                                                          get_num_trainpkts(ni)); 
            ni->smartantenna_state = SMARTANTENNA_TRAIN_INPROGRESS;
            sa_set_traffic_gen_timer(ic, ni);
        }

        ieee80211_free_node(ni);
    } /* end of loop */

    if (ic->ic_smartantennaparams->retraining_enable && (SA_ENABLE_HW_ALGO == ic->ic_get_smartantenna_enable(ic))) {
        OS_SET_TIMER(&ic->ic_smartant_retrain_timer, ic->ic_smartantennaparams->retraining_enable);
    } else {
        ic->ic_sa_retraintimer = 0;
    }
}

/** 
 *   ieee80211_smartantenna_gen_traffic: This function is used to send custom training packets to a particular node.
 *   @ni: handle to node.
 *
 *   returns -  0 after successfully transmitting custom training packet.
 */
int ieee80211_smartantenna_gen_traffic(struct ieee80211_node *ni)
{

    struct custom_pkt_params cpp;
    int i=0;
    IEEE80211_ADDR_COPY(cpp.dst_mac_address,ni->ni_macaddr);
    IEEE80211_ADDR_COPY(cpp.src_mac_address, ni->ni_bssid);
     
    for (i=0; i < NUM_TRAIN_PKTS; i++) {
        ieee80211_smartantenna_tx_custom_data_pkt(ni, ni->ni_vap, &cpp, 0, 0, 0);
    }
    
    return 0;
}


/** 
 *   smartantenna_traffic_gen_sm: This function is called in workqueue context, this function performs 
 *      1) iterrate each node and check the training status
 *      2) if mixed mode training is enabled and custom packet generation is required (ic_get_sa_trafficgen_required: function returns TRUE)
 *         then call ieee80211_smartantenna_gen_traffic to generate traffic.
 *
 *   @argument: handle to ic is argument for workqueue function.
 *
 *   There is no return value 
 */
void smartantenna_traffic_gen_sm(void *data)
{
    struct ieee80211com *ic = (struct ieee80211com *)data;
    struct ieee80211_node *ni = NULL, *next = NULL, *ni_last = NULL;
    rwlock_state_t lock_state;
    struct ieee80211_node_table *nt = &ic->ic_sta;
    u_int8_t last_node = 0;

    u_int8_t timer_required = 0;
    OS_RWLOCK_READ_LOCK(&nt->nt_nodelock, &lock_state);
    TAILQ_FOREACH_SAFE(ni, &nt->nt_node, ni_list, next)
    {
        ni_last = ni;
        if (IEEE80211_ADDR_EQ(ni->ni_macaddr, ni->ni_vap->iv_myaddr))
            continue;

        /* if node is authorised */
        if (!ieee80211_node_is_authorized(ni)) {
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT, "Node is not authorised MAC : %s \n",ether_sprintf(ni->ni_macaddr));
            continue;
        }

        ieee80211_ref_node(ni);
        if(next != NULL) {
            ieee80211_ref_node(next);
        } else {
            if(last_node) {
                IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT, " %s : Node looping error ",__func__);
            }
            last_node = 1;
        }

        if (!(ic->ic_smartantennaparams->training_mode || ni->hybrid_train)) {
            ieee80211_free_node(ni);
            if(next != NULL) {
                ieee80211_free_node(next);
            }
            continue;
        }
        timer_required = 1;
        if((ic->ic_get_sa_trafficgen_required(ni)) || (ni->smartantenna_state == SMARTANTENNA_PRETRAIN)) {
            /* generate traffic */
            OS_RWLOCK_READ_UNLOCK(&nt->nt_nodelock, &lock_state);
            ieee80211_smartantenna_gen_traffic(ni);   
            OS_RWLOCK_READ_LOCK(&nt->nt_nodelock, &lock_state);
        }

        ieee80211_free_node(ni);
        if(next != NULL) {
            ieee80211_free_node(next);
        }

    } /* end of loop */
    OS_RWLOCK_READ_UNLOCK(&nt->nt_nodelock, &lock_state);

    if (ni_last != NULL) {  /* Node is removed before cancling the timer */
        if(!timer_required) {
            SA_TRAFFICGEN_IRQ_LOCK(ic);
            if((TAILQ_NEXT(ni_last,ni_list) == NULL)) {
                OS_CANCEL_TIMER(&ic->ic_smartant_traffic_gen_timer);
                ic->ic_sa_timer = 0;
            } else {
                IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT, " %s :New node added after processing",__func__);
            }
            SA_TRAFFICGEN_IRQ_UNLOCK(ic);
        }
    } else {
        SA_TRAFFICGEN_IRQ_LOCK(ic);
        OS_CANCEL_TIMER(&ic->ic_smartant_traffic_gen_timer);
        ic->ic_sa_timer = 0;
        SA_TRAFFICGEN_IRQ_UNLOCK(ic);
    }

}

/** 
 *   smartantenna_traffic_gen_handler: This timer function schedules workqueue function and restart the ic_smartant_traffic_gen_timer. 
 *   @timeargument: handle to ic.
 *
 *   There is no return value 
 */
static OS_TIMER_FUNC(smartantenna_traffic_gen_handler)
{
    struct ieee80211com *ic;
    OS_GET_TIMER_ARG(ic, struct ieee80211com *);

    ATH_QUEUE_DELAYED_WORK(ic->ic_smart_workqueue,&ic->smartant_traffic_task,0);
    OS_SET_TIMER(&ic->ic_smartant_traffic_gen_timer, ic->ic_smartantennaparams->traffic_gen_timer);
}

/** 
 *   sa_set_traffic_gen_timer: This function starts the smartantenna traffic genataion timer. 
 *   @ic: handle to ic.
 *   @ni: handle to node.
 *
 *   There is no return value
 */ 
void sa_set_traffic_gen_timer(struct ieee80211com *ic,struct ieee80211_node *ni)
{
    if ((ic->ic_smartantennaparams->training_mode) ||
            ni->hybrid_train) {
        /* Mixed traffic */
      SA_TRAFFICGEN_IRQ_LOCK(ic);
        if(0 == ic->ic_sa_timer) {
            ic->ic_sa_timer = 1;
            OS_SET_TIMER(&ic->ic_smartant_traffic_gen_timer, ic->ic_smartantennaparams->traffic_gen_timer);
        }
      SA_TRAFFICGEN_IRQ_UNLOCK(ic);
    }
}

/** 
 *   ieee80211_smartantenna_init_training: This function performs sequence of operations that need to be done for a node 
 *     to start training process. 
 *   @ni: handle to node.
 *   @ic: handle to ic.
 *
 *   returns - 1 on success. 
 */
int ieee80211_smartantenna_init_training(struct ieee80211_node *ni, struct ieee80211com *ic)
{
    u_int8_t rateCode;
    u_int32_t npackets;
#if SA_DEBUG
    u_int32_t i;
    ni->ts_trainstart = ic->ic_get_TSF32(ic);
    IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT, "Training start:%s :mac: %s 0x%08x \n",__func__ ,ether_sprintf(ni->ni_macaddr),ni->ts_trainstart);
#endif

    ieee80211_ref_node(ni);

    if (ic->ic_smartantennaparams->training_start > 1) {
        rateCode = ic->ic_smartantennaparams->training_start;
    } else {
        rateCode = smartantenna_retrain_check(ni, 0, &npackets);
    }

#if SA_DEBUG
    for(i = 0; i< ni->rate_info.num_of_rates; i++) {
        IEEE80211_DPRINTF(ni->ni_vap,IEEE80211_MSG_SMARTANT, "%s RC 0x%02x ind %d\n",__func__,ni->rate_info.rates[i].ratecode,ni->rate_info.rates[i].rateindex);
    }
#endif

    IEEE80211_DPRINTF(ni->ni_vap,IEEE80211_MSG_ANY, "TS mac: %s rssi %d rateCode 0x%02x Assoc\n",ether_sprintf(ni->ni_macaddr), ni->ni_rssi, rateCode);
#if SA_DEBUG
    IEEE80211_DPRINTF(ni->ni_vap,IEEE80211_MSG_ANY, " Prd %d Per %d \n", ni->prd_trigger_count,ni->perf_trigger_count);
#endif
    IEEE80211_DPRINTF(ni->ni_vap,IEEE80211_MSG_ANY, "\n");

    ni->retrain_miss = 0;
    ni->train_state.antenna = 0;
    ni->train_state.rate_code = rateCode;
    ni->current_rate_index = get_rateindex(ni,rateCode);
    ni->train_state.rateidx = ni->current_rate_index;
    ni->train_state.first_per = 1;
    ni->prev_goodput = 0;
    ni->gput_avg_interval = 0;
    ni->retrain_trigger_type = 0;

    ni->train_state.extra_sel = 0;
    ni->train_state.extra_cmp = 0;
    ic->ic_set_sa_train_params(ni, ni->train_state.ant_map[ni->train_state.antenna], ni->current_rate_index,
                                                                        get_num_trainpkts(ni)); 
    ni->smartantenna_state = SMARTANTENNA_TRAIN_INPROGRESS;

    sa_set_traffic_gen_timer(ic, ni);

    ieee80211_free_node(ni);
    return 1;
}

int maxmize_min(struct ieee80211_node *ni, struct sa_train_data *tab, uint8_t x)
{
    int8_t train_rssi,current_rssi;

    train_rssi = find_min_rssi((int8_t *)ni->train_state.rssi,x);

    current_rssi = find_min_rssi((int8_t *)tab->rssi,x); 

    if (current_rssi > train_rssi) {  
        /* Maximizing the Minimum */
        return 1;
    } else if (current_rssi == train_rssi) {
        /* if we have same min RSSI values then check for MAX RSSI in that BA RSSI SET */
        train_rssi = find_max_rssi((int8_t *)ni->train_state.rssi,x);

        current_rssi = find_max_rssi((int8_t *)tab->rssi,x);

        if(current_rssi > train_rssi) {
            return 1;
        } else if (current_rssi < train_rssi) {
            return 0;
        } else {
            return -1;
        }
    }
    return 0;
}

uint8_t apply_secondary_metric(struct ieee80211_node *ni, struct sa_train_data *tab)
{
    uint32_t x = 0;
    uint32_t cant = 0, tant = 0;
    uint8_t retval;

    for (x = 0; x < SA_MAX_RSSI_SAMPLES; x++) {
        IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"Dumping RSSI index %d: Current Ant: %d %d %d Train Ant: %d %d %d \n", x,
                 ni->train_state.rssi[0][x],ni->train_state.rssi[1][x],ni->train_state.rssi[2][x],
                 tab->rssi[0][x],tab->rssi[1][x],tab->rssi[2][x]);

        retval = maxmize_min(ni, tab,x);
        if (retval == 0) {
            cant++;
        } else if (retval == 1) {
            tant++;
        }
    }
    IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"Number of times current ant %d selected: %d \n",ni->rate_info.selected_antenna,cant);
    IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"Number of times train ant %d selected: %d \n",ni->train_state.ant_map[ni->train_state.antenna],tant);
    if (cant >= tant)
        return 0;
    else
        return 1;

}

/** 
 *   ieee80211_smartantenna_update_trainstats: This is the core function for antenna selection logic, it uses PER and RSSI stats
 *     collected during process. This is callback routine from lmac layer.
 *   @ni: handle to node.
 *   @per_stats: handle to PER and RSSI stats data.
 *
 *   returns - 1 on success. 
 */
int ieee80211_smartantenna_update_trainstats(struct ieee80211_node *ni, void *per_stats)
{
    struct sa_train_data *tab  =  (struct sa_train_data *)per_stats;
    uint32_t per = 0,per_diff = 0;
    int32_t ridx;
    uint8_t train_nxt_antenna = 1;
    int8_t train_nxt_rate = 0;
    uint8_t swith_antenna = 0;
    struct ieee80211com *ic = ni->ni_ic;
    uint8_t temp_ant;
    int chainmask;
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);

    ieee80211_ref_node(ni);

    ni->smartantenna_state = SMARTANTENNA_TRAIN_HOLD; /* make sure state is not TRAIN_INPROGRESS 
                                                         otherwise traffic generator will keep on sending traffic
                                                         Add a state TRAIN_HOLD, This can be renamed later   
                                                         */
    tab->antenna = ni->train_state.ant_map[ni->train_state.antenna];
    if (ni->train_state.extra_stats) {
        if (!ni->train_state.extra_sel) {
            tab->antenna = ni->rate_info.selected_antenna;
        }
    }
    /* Move to next rate/antenna for Now */
    if (tab->nFrames) {
        per = (100*tab->nBad/tab->nFrames); 
    } else {
        IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_ANY," %s ERROR in PER stats\n",__func__);
        per = 100;
    }
    
    IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT," %s : Antenna: %02d RateIdx: 0x%02x Ratecode: 0x%02x nFrame: %03d nBad: %03d BARSSI: %02d %02d %02d PER: %d \n",
                         ether_sprintf(ni->ni_macaddr), tab->antenna, ni->train_state.rateidx, ni->train_state.rate_code,tab->nFrames, tab->nBad,tab->rssi[0][0],tab->rssi[1][0],tab->rssi[2][0], per);

    if (ni->train_state.extra_stats) {
        if (!ni->train_state.extra_sel) {
            ni->train_state.extra_sel = 1;
            ni->train_state.per =(100*(tab->nBad + ni->train_state.nbad)/(ni->train_state.nframes + tab->nFrames));
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"%s : Extra stats selant %d %d %d %d new per %d\n",__func__,tab->nBad,ni->train_state.nbad,ni->train_state.nframes,tab->nFrames,ni->train_state.per);
            ni->train_state.nbad += tab->nBad;
            ni->train_state.nframes += tab->nFrames;
            /* copy RSSI Samples */
            memcpy(&ni->train_state.rssi[0][0], &tab->rssi[0][0], (SA_MAX_RSSI_SAMPLES * SA_MAX_RECV_CHAINS));
            goto extra_stats;
        } else if (!ni->train_state.extra_cmp) {
            ni->train_state.extra_cmp = 1;
            per = (100*(tab->nBad + ni->train_state.extra_nbad)/(tab->nFrames + ni->train_state.extra_nframes)); 
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"%s : Extra stats cmpant %d %d %d %d new per %d\n",__func__,tab->nBad,ni->train_state.extra_nbad,tab->nFrames,ni->train_state.extra_nframes,per);
            ni->train_state.extra_stats = 0;
        } else {
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_ANY,"%s : error in getting extra stats \n",__func__);
        }
    }

    if (ni->rate_info.selected_antenna == ni->train_state.ant_map[ni->train_state.antenna]) {
        /* curretly selected antenna */
        if (ni->train_state.first_per) {
            ni->train_state.first_per = 0;
            ni->train_state.per = per;
            ni->train_state.nbad = tab->nBad;
            ni->train_state.nframes = tab->nFrames;
            /* copy RSSI Samples */
            memcpy(&ni->train_state.rssi[0][0], &tab->rssi[0][0], (SA_MAX_RSSI_SAMPLES * SA_MAX_RECV_CHAINS));
            ni->train_state.last_rateidx = ni->train_state.rateidx;
            if (per > ic->ic_smartantennaparams->upper_bound) {
                train_nxt_rate = -1;
            } else if (per < ic->ic_smartantennaparams->lower_bound) {
                train_nxt_rate = 1;
            }
        } else {
            /* check if PER can be used */
            if (per < ic->ic_smartantennaparams->upper_bound) {
                if (per < ic->ic_smartantennaparams->lower_bound) {
                    if (ni->train_state.last_trained > 0) {
                        train_nxt_rate = 1; 
                        ni->train_state.per = per;
                        ni->train_state.nbad = tab->nBad;
                        ni->train_state.nframes = tab->nFrames;
                        /* copy RSSI Samples */
                        memcpy(&ni->train_state.rssi[0][0], &tab->rssi[0][0], (SA_MAX_RSSI_SAMPLES * SA_MAX_RECV_CHAINS));
                    } else {
                        /* PER 100, 100, 15 in this case store the PER and trying next rate */
                        ni->train_state.per = per;
                        ni->train_state.nbad = tab->nBad;
                        ni->train_state.nframes = tab->nFrames;
                        /* copy RSSI Samples */
                        memcpy(&ni->train_state.rssi[0][0], &tab->rssi[0][0], (SA_MAX_RSSI_SAMPLES * SA_MAX_RECV_CHAINS));
                        ni->train_state.last_rateidx = ni->train_state.rateidx;
                    }
                } else {
                    ni->train_state.per = per;
                    ni->train_state.nbad = tab->nBad;
                    ni->train_state.nframes = tab->nFrames;
                    /* copy RSSI Samples */
                    memcpy(&ni->train_state.rssi[0][0], &tab->rssi[0][0], (SA_MAX_RSSI_SAMPLES * SA_MAX_RECV_CHAINS));
                    ni->train_state.last_rateidx = ni->train_state.rateidx;
                }
            } else {
                if (ni->train_state.last_trained < 0) {
                    train_nxt_rate = -1;
                    ni->train_state.per = per;
                    ni->train_state.nbad = tab->nBad;
                    ni->train_state.nframes = tab->nFrames;
                    /* copy RSSI Samples */
                    memcpy(&ni->train_state.rssi[0][0], &tab->rssi[0][0], (SA_MAX_RSSI_SAMPLES * SA_MAX_RECV_CHAINS));
                } else {
                    /* moving to one rate down for next antenna combinations */
                    ni->train_state.rateidx = sa_move_rate(ni, (0-ni->train_state.last_trained));
                    if (ni->train_state.rateidx != ni->train_state.last_rateidx) {
                        printk("\n %s ERRR2",__func__);
                    }
                }
            }
        } /* not first per */
    } else { /* Update from other than current selected antenna */
        /* compare PER and update stats */
        /* 
         * a) PER < lower bound on train other than current selected antenna
         *          Move to next higer rate
         */
        
        /* per < lowerbound && rateIndex is not Max valid rate ; In case of max validrate we wont chek for per and secondary metric */
        if ((per < ic->ic_smartantennaparams->lower_bound) && (sa_get_max_rateidx(ni) != ni->train_state.rateidx) && !ni->train_state.extra_cmp) {
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"%s: Current antenna previous tried rateidx: 0x%02x and trained antenna rateidx:0x%02x \n",ether_sprintf(ni->ni_macaddr), ni->train_state.last_rateidx, ni->train_state.rateidx);
            train_nxt_rate = 1;
            train_nxt_antenna = 0;
            if (ni->train_state.rateidx > ni->train_state.last_rateidx) {
                swith_antenna = 1;
           } else if (ni->train_state.rateidx == ni->train_state.last_rateidx) {  
                /* If rateIdx are same; save The per for comparsion on same rate 
                    for trained antena antenna againest current antenna if trained antena has UpperBound PER on next rate
                    Lower bound : 20
                    Example:  0 - 18 ;1 - 10 But for next rate 1 - 95;
                    In this case we will comapre 1-10 vs 0 -18 
                    */ 
                 ni->train_state.nextant_per = per;
                 ni->train_state.nextant_nbad = tab->nBad;
                 ni->train_state.nextant_nframes = tab->nFrames;
                 /* copy RSSI Samples to node nextant*/
                 memcpy(&ni->train_state.nextant_rssi[0][0], &tab->rssi[0][0], (SA_MAX_RSSI_SAMPLES * SA_MAX_RECV_CHAINS));
            }
        } else {
            if (ni->train_state.rateidx > ni->train_state.last_rateidx) {
                if (per < ic->ic_smartantennaparams->upper_bound) {    
                    /* current Idx < trained idx but PER is not in lower bound i
                         Example: 0 -25 on x rate
                         1 - 15 on x rate after trying x+1 we got PER < 90 ;say 65
                         1 - 65 on x+1 but trainedIdx > currentIdx; So move antenna
                     */
                    swith_antenna = 1;
                } else { /* per > upper bound: Need to compare with the previous PER */
                    
                    /*
                     *  Make current RateIdx is equal to last RateIdx  to follow normal path
                     *  Restore the previous PER
                     *  Move rate down and try Next antenna
                     */
                    ni->train_state.rateidx = sa_move_rate(ni, -1); /* This make rateIdx to previous */
                    IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"%s: previous tried rateidx: %d and rateidx:%d \n",ether_sprintf(ni->ni_macaddr),ni->train_state.last_rateidx, ni->train_state.rateidx);
                    IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"%s: PER %d and Nextant per: %d RSSI ch0:%d ch1:%d ch2:%d Previous PER: %d \n",ether_sprintf(ni->ni_macaddr),per
                                                                        ,ni->train_state.nextant_per
                                                                        ,ni->train_state.nextant_rssi[0][0]
                                                                        ,ni->train_state.nextant_rssi[1][0]
                                                                        ,ni->train_state.nextant_rssi[2][0]
                                                                        ,ni->train_state.per);
                    per = ni->train_state.nextant_per;
                    tab->nBad = ni->train_state.nextant_nbad;
                    tab->nFrames = ni->train_state.nextant_nframes;
                    /* copy RSSI Samples */
                    memcpy(&tab->rssi[0][0], &ni->train_state.nextant_rssi[0][0], (SA_MAX_RSSI_SAMPLES * SA_MAX_RECV_CHAINS));
                    train_nxt_antenna = 1;
                }
            } 
           
            if (ni->train_state.rateidx == ni->train_state.last_rateidx) {
                per_diff = per > ni->train_state.per ? (per - ni->train_state.per) : 
                    (ni->train_state.per - per);
                if (per_diff <= ic->ic_smartantennaparams->per_diff_threshold) {
                    /* apply secondary metric */
                    if ((ic->ic_smartantennaparams->config & SA_CONFIG_EXTRATRAIN) && !ni->train_state.extra_cmp) {
                        ni->train_state.extra_stats = 1;//double_train
                        ni->train_state.extra_nbad = tab->nBad;
                        ni->train_state.extra_nframes = tab->nFrames;
                        IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"%s : extra stats for ant %d and %d\n",__func__,ni->train_state.ant_map[0],ni->train_state.ant_map[ni->train_state.antenna]);
                    } else {
                        if (ic->ic_smartantennaparams->config & SA_CONFIG_EXTRATRAIN) {
                            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"%s : extra stats already taken\n",__func__);
                        }
                    }
                    if (!ni->train_state.extra_stats) {
                        swith_antenna = apply_secondary_metric(ni, tab);
                    }
                } else {
                    if (per < ni->train_state.per) {
                        swith_antenna = 1;
                    }
                }
            } /* ni->train_state.rateidx == ni->train_state.last_rateidx */
        } /* else part of per < lowerbound */

        if (swith_antenna) {
            /* choose trained antenna as current antenna */
            ni->rate_info.selected_antenna = ni->train_state.ant_map[ni->train_state.antenna]; 
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"%s: progeressive antenna selection to: %d \n", ether_sprintf(ni->ni_macaddr), ni->train_state.ant_map[ni->train_state.antenna]);
            ic->ic_set_selected_smantennas(ni, ni->rate_info.selected_antenna);

            ni->train_state.per = per;
            ni->train_state.nextant_nbad = tab->nBad;
            ni->train_state.nextant_nframes = tab->nFrames;
            /* copy RSSI Samples */
            memcpy(&ni->train_state.rssi[0][0], &tab->rssi[0][0], (SA_MAX_RSSI_SAMPLES * SA_MAX_RECV_CHAINS));
            ni->train_state.last_rateidx = ni->train_state.rateidx; /* Update last trained rateIdx; if we per > lowerbound we are not updating this previously */

            ni->train_state.extra_sel = ni->train_state.extra_cmp;

            if (per < ic->ic_smartantennaparams->lower_bound) {
                train_nxt_rate = 1;
            }
        } /* switch Antenna */
    }  /* else part of update from not current antenna */

extra_stats:
    if (ni->train_state.extra_stats) {
        if (!ni->train_state.extra_sel) {
            ni->smartantenna_state = SMARTANTENNA_TRAIN_INPROGRESS;
            ic->ic_set_sa_train_params(ni, ni->train_state.ant_map[0], ni->train_state.rateidx, get_num_trainpkts(ni));
        } else if (!ni->train_state.extra_cmp) {
            ni->smartantenna_state = SMARTANTENNA_TRAIN_INPROGRESS;
            ic->ic_set_sa_train_params(ni, ni->train_state.ant_map[ni->train_state.antenna], ni->train_state.rateidx, get_num_trainpkts(ni));
        } else {
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_ANY,"%s: error in getting extra stats\n",__func__);
        }
        ieee80211_free_node(ni);
        return 0;
    }
    ni->train_state.extra_cmp = 0;
    if (train_nxt_rate != 0) {
        /* Move one rate above and do the training with same antenna */
        ridx = sa_move_rate(ni, train_nxt_rate);
        if (ridx < 0) {
            /* We are at the extreme. no next rate  */
            train_nxt_antenna = 1;
            ni->train_state.last_rateidx = ni->train_state.rateidx;
        } else {
            ni->train_state.last_trained = train_nxt_rate;
            ni->train_state.last_rateidx = ni->train_state.rateidx;
            train_nxt_antenna = 0;
            ni->train_state.rateidx = ridx;
            ni->smartantenna_state = SMARTANTENNA_TRAIN_INPROGRESS;
            ic->ic_set_sa_train_params(ni, ni->train_state.ant_map[ni->train_state.antenna], ni->train_state.rateidx, 
                    get_num_trainpkts(ni)); 
            if (ni->train_state.ant_map[ni->train_state.antenna] == ni->rate_info.selected_antenna) {
                ni->train_state.extra_sel = 0;
            }
        }
    } 

    if (train_nxt_antenna) {
        ni->train_state.last_trained = 0;
        /* update antenna map */
       if (ni->train_state.ant_map[0] != ni->rate_info.selected_antenna) {
           temp_ant = ni->train_state.ant_map[0];
           ni->train_state.ant_map[0] = ni->train_state.ant_map[ni->train_state.antenna];
           ni->train_state.ant_map[ni->train_state.antenna] = temp_ant;
       }

        if (ni->train_state.antenna < (ni->ni_ic->ic_smartantennaparams->num_tx_antennas-1)) {
            chainmask = scn->sc_ops->get_tx_chainmask(scn->sc_dev);
            /* to skip the antenna combination for unused chain */
            do{
                ni->train_state.antenna++;
                if(ni->train_state.antenna == ni->ni_ic->ic_smartantennaparams->num_tx_antennas){
                    ni->train_state.antenna--;
                    break;
                }
                temp_ant = ni->train_state.antenna | chainmask;
            }while((temp_ant ^ chainmask) != 0);

            ni->smartantenna_state = SMARTANTENNA_TRAIN_INPROGRESS;
            ic->ic_set_sa_train_params(ni, ni->train_state.ant_map[ni->train_state.antenna], ni->train_state.rateidx,
                                                                             get_num_trainpkts(ni)); 
        } else {
            /* training completed */
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"%s: Training completed %d %d %d %d\n"
                                        , ether_sprintf(ni->ni_macaddr), ni->train_state.ant_map[0], ni->train_state.ant_map[1]
                                         ,ni->train_state.ant_map[2],ni->train_state.ant_map[3]);
            ni->smartantenna_state = SMARTANTENNA_DEFAULT;
            ni->intense_train = 0;

            if ((ni->ni_vap->iv_opmode == IEEE80211_M_HOSTAP)) {
                /* Tx antenna changes from previous  */ 
                if ((ni->previous_txant != ni->rate_info.selected_antenna)|| (ni->previous_txant != ic->ic_get_default_antenna(ic))) {
                    /* do rx antenna slection */
                    ieee80211_smartantenna_select_rx_antenna(ni);
                }
            }

            ni->previous_txant = ni->rate_info.selected_antenna; 
            ni->is_training = 0;
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"%s :Antenna Selected  %d : \n",ether_sprintf(ni->ni_macaddr), ni->rate_info.selected_antenna);
#if SA_DEBUG
            ni->ts_traindone = ic->ic_get_TSF32(ic);
            IEEE80211_DPRINTF(ni->ni_vap,IEEE80211_MSG_ANY, "TD Mac: %s Time: %d A %d\n",ether_sprintf(ni->ni_macaddr), ni->ts_traindone - ni->ts_trainstart,ni->rate_info.selected_antenna);
#endif
            
            if (ni->ni_vap->iv_opmode == IEEE80211_M_STA) {
                /* in case of station switch rx antenna selected tx antenna */
                if (ic->ic_get_default_antenna(ic) != ni->rate_info.selected_antenna) {
                    ic->ic_set_default_antenna(ic, ni->rate_info.selected_antenna);
                }
                IEEE80211_DPRINTF(ni->ni_vap,IEEE80211_MSG_ANY, "\n receive antenna %d %d", ic->ic_get_default_antenna(ic), ni->rate_info.selected_antenna);
                ni->hybrid_train = 0;
                ni->long_retrain_count = 0;
            }
            ic->ic_smartantennaparams->training_start = 0;
        } /* training completed */
    } /* train next antenna */
    ieee80211_free_node(ni);

    return 0;
}


/** 
 *   ieee80211_smartantenna_select_rx_antenna: This function selects rx antenna based on below algorithm
 *      1) In Station mode whenever TX antenna gets selected RX antenna will also be changed to the same immediately.
 *      2) In AP mode if all the station connected to use same TX antenna then AP switches it’s receive antenna to that TX antenna otherwise 
 *         receive antenna is programmed to default antenna combination.
 *   @ni_cnode: handle to node.
 *
 *   There is no return value
 */
void ieee80211_smartantenna_select_rx_antenna(struct ieee80211_node *ni_cnode)
{
    struct ieee80211com *ic = ni_cnode->ni_ic;
    struct ieee80211_node_table  *nt = &ic->ic_sta;
    uint8_t antenna = 0, first=1;
    int8_t select_rx_antenna = 0;
    struct ieee80211_node *ni = NULL, *next = NULL;
    rwlock_state_t lock_state;

    OS_RWLOCK_READ_LOCK(&nt->nt_nodelock, &lock_state);
    TAILQ_FOREACH_SAFE(ni, &nt->nt_node, ni_list, next) {

        if (IEEE80211_ADDR_EQ(ni->ni_macaddr, ni->ni_vap->iv_myaddr)) {
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_DEBUG, "%s : Skipping Training for Own VAP: %s \n", __func__ , ether_sprintf(ni->ni_macaddr));
            continue;
        }

        if (first) {
            antenna = ni->rate_info.selected_antenna;
            select_rx_antenna = 1;
            first=0;
            continue;
        }

        if (antenna != ni->rate_info.selected_antenna) {
            select_rx_antenna = 0;
            break;
        }
    } /* end of loop */
    OS_RWLOCK_READ_UNLOCK(&nt->nt_nodelock, &lock_state);

    if (select_rx_antenna) {
        /* Configure RX antenna */
        if (ni_cnode->rate_info.selected_antenna != ic->ic_get_default_antenna(ic)) {
            ic->ic_set_default_antenna(ic, ni_cnode->rate_info.selected_antenna);
            /* Update multicast antenna */
        }

        IEEE80211_DPRINTF(ni_cnode->ni_vap, IEEE80211_MSG_SMARTANT,"%s :All Node have Same TX antenna, Configuring RX antenna to: %d\n"
                , __func__, ni_cnode->rate_info.selected_antenna);
    } else {

        select_rx_antenna = ic->ic_get_sa_defant(ic);
        if(select_rx_antenna != ic->ic_get_default_antenna(ic)) {
            /* Make RX antenna to default */
            ic->ic_set_default_antenna(ic, select_rx_antenna);
            /* Update multicast antenna */
            IEEE80211_DPRINTF(ni_cnode->ni_vap, IEEE80211_MSG_SMARTANT,"%s :Configuring RX antenna to: %d\n"
                    , __func__, select_rx_antenna);
        }
    }

}


/** 
 *   sa_move_rate: This function returns next/previous valid rate index for that node.
 *   @ni: handle to node.
 *   @train_nxt_rate: next rate direction. 1 to get next higher valid rate ,-1 to get next lower valid rate.
 *
 *   returns - rate index on success. 
             - -1 in case of failure. 
 */
int32_t sa_move_rate(struct ieee80211_node *ni, int8_t train_nxt_rate)
{
    int i =0;

    if (ni->ni_ic->ic_smartantennaparams->training_start > 1) {
        /* fixed rate manual training */
        return -1;
    }
    for(i = 0; i < ni->rate_info.num_of_rates; i++) {
        if (ni->rate_info.rates[i].ratecode == ni->train_state.rate_code) {
            break;
        }
    }

    if(i == ni->rate_info.num_of_rates) {
        IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_ANY,"%s ERROR in sa rate table\n",__func__);
    }

    if(train_nxt_rate < 0) {
        if (i) {
            ni->train_state.rate_code = ni->rate_info.rates[i-1].ratecode;
            return ni->rate_info.rates[i-1].rateindex;
        } else {
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"%s already at leat rate\n",__func__);
            return -1;
        }
    } else {
        if (i >= ni->rate_info.num_of_rates-1) {
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_SMARTANT,"%s already at max rate\n",__func__);
            return -1;
        } else {
            ni->train_state.rate_code = ni->rate_info.rates[i+1].ratecode;
            return ni->rate_info.rates[i+1].rateindex;
        }
    }
}

/** 
 *   get_rateindex: This function returns rate index for a given rateCode to that node.
 *   @ni: handle to node.
 *   @rateCode: rateCode.
 *
 *   returns - rate index on success. 
             - 0 in case of failure. 
 */             
u_int8_t get_rateindex(struct ieee80211_node *ni,u_int8_t rateCode)
{
    int32_t i;

    for(i = ni->rate_info.num_of_rates-1; i >= 0; i--) {
        if (ni->rate_info.rates[i].ratecode == rateCode) {
            return ni->rate_info.rates[i].rateindex;
        }
    }
    return 0;
}

/** 
 *   sa_get_max_rateidx: This function returns max rate index for a given node.
 *   @ni: handle to node.
 *
 *   returns - maxrate index. 
*/             
int32_t sa_get_max_rateidx(struct ieee80211_node *ni)
{
    int i = ni->rate_info.num_of_rates-1;
    return ni->rate_info.rates[i].rateindex;
}

/** 
 *   ieee80211_smartantenna_attach: This is entry point of smart antenna module, all required os resources are allocated
 *      and smart atenna default values are configured in this function.
 *   @ic: handle to ieee80211com object .
 *
 *   returns - 0 on success. 
             - -EFAULT/-ENOMEM in case of failure. 
 */             
int ieee80211_smartantenna_attach(struct ieee80211com *ic)
{
    
    if (!ic) return -EFAULT; 

    ic->ic_smartantenna_init_done=0;
    ic->ic_smartantennaparams = (struct ieee80211_smartantenna_params *) 
                     OS_MALLOC(ic->ic_osdev, sizeof(struct ieee80211_smartantenna_params), GFP_KERNEL); 
    
    if (!ic->ic_smartantennaparams) {
        printk("Memory not allocated for ic->ic_smartantennaparams\n"); /* Not expected */
        return -ENOMEM;
    }

    OS_MEMSET(ic->ic_smartantennaparams, 0, sizeof(struct ieee80211_smartantenna_params));  
    /* init the default values */
    ic->ic_smartantennaparams->lower_bound = SA_PER_LOWER_BOUND;
    ic->ic_smartantennaparams->upper_bound = SA_PER_UPPER_BOUND ;
    ic->ic_smartantennaparams->per_diff_threshold = SA_PERDIFF_THRESHOLD;
    ic->ic_smartantennaparams->config = (SA_CONFIG_INTENSETRAIN | SA_CONFIG_EXTRATRAIN);
    ic->ic_smartantennaparams->training_start = 0;
    ic->ic_smartantennaparams->training_mode = SA_TRAINING_MODE_EXISTING;
    ic->ic_smartantennaparams->training_type = 0;
    ic->ic_smartantennaparams->packetlen = SA_DEFAULT_PKT_LEN;
    ic->ic_smartantennaparams->n_packets_to_train = 0;
    ic->ic_smartantennaparams->traffic_gen_timer = 0;
    ic->ic_smartantennaparams->retraining_enable = RETRAIN_INTERVEL;
    ic->ic_smartantennaparams->max_throughput_change = SA_DEFAULT_TP_THRESHOLD;
    ic->ic_smartantennaparams->retrain_interval = SA_DEFAULT_RETRAIN_INTERVAL;
    ic->ic_smartantennaparams->num_pkt_threshold = SA_DEFAULT_MIN_TP;
    ic->ic_smartantennaparams->hysteresis = SA_DEFALUT_HYSTERISYS;
    ic->ic_smartantennaparams->min_goodput_threshold = MIN_GOODPUT_THRESHOLD;
    ic->ic_smartantennaparams->goodput_avg_interval = SA_GPUT_AVERAGE_INTRVAL;

    ASSERT(NUM_TX_CHAINS >= ic->ic_num_tx_chain);
    ic->ic_smartantennaparams->num_tx_antennas =  SA_ANTENNA_COMBINATIONS;

    /* Create Work Queue */
    ic->ic_smart_workqueue =  ATH_CREATE_WQUEUE(WQNAME(smartantena_workqueue));
    ATH_CREATE_DELAYED_WORK(&ic->smartant_traffic_task, smartantenna_traffic_gen_sm, ic);
    /* Retrain Timer */
    OS_INIT_TIMER(ic->ic_osdev, &ic->ic_smartant_retrain_timer, smartantenna_retrain_handler, ic);
    /* Traffic Generation Timer */
    OS_INIT_TIMER(ic->ic_osdev, &ic->ic_smartant_traffic_gen_timer, smartantenna_traffic_gen_handler, ic);
    SA_TRAFFICGEN_LOCK_INIT(ic);
    ic->ic_smartantenna_init_done=1;
    return 0;
}

/** 
 *   ieee80211_smartantenna_detach: This is exit point of smart antenna module, all allocated os resources are freed in this function.
 *   @ic: handle to ieee80211com object .
 *
 *   returns - 0 on success. 
             - -EFAULT/-ENOMEM in case of failure. 
 */             
int ieee80211_smartantenna_detach(struct ieee80211com *ic)
{
    int err=0;

    if (!ic) return -EFAULT;

    if (!ic->ic_smartantenna_init_done) return 0;

    if (!ic->ic_smartantennaparams) {
        printk("Memory not allocated for ic->ic_smartantennaparams\n"); 
        err=-ENOMEM;
        goto bad;
    } else {
        printk("%s ic_smartantennaparams is freed \n", __func__);
        OS_FREE(ic->ic_smartantennaparams); 
        ic->ic_smartantennaparams = NULL;
    }

bad:
   ATH_FLUSH_WQUEUE(ic->ic_smart_workqueue);
   ATH_DESTROY_WQUEUE(ic->ic_smart_workqueue);
   OS_FREE_TIMER(&ic->ic_smartant_retrain_timer);
   OS_FREE_TIMER(&ic->ic_smartant_traffic_gen_timer);
   SA_TRAFFICGEN_LOCK_DESTORY(ic)
   ic->ic_smartantenna_init_done=0;

   return err;
}

#endif
