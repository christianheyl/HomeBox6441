/*
 *  Copyright (c) 2009 Atheros Communications Inc.  All rights reserved.
 */


/*
 *  Radio Resource measurements message handlers.
 */
#include <ieee80211_var.h>
#include "ieee80211_rrm_priv.h"
#include <ieee80211_channel.h>

#if UMAC_SUPPORT_RRM
/*
 * RRM action frames recv handlers
 */

static void ieee80211_rrm_scan_evhandler(struct ieee80211vap *originator, ieee80211_scan_event *event, void *arg);

/**
 * @brief interaction with frame input module  
 *
 * @param vap
 * @param ni
 * @param action
 * @param frm
 * @param frm_len
 *
 * @return on success return 0.
 *         on failure returns -ve value.
 */
int ieee80211_rrm_recv_action(wlan_if_t vap, wlan_node_t ni, u_int8_t action,
        u_int8_t *frm, u_int32_t frm_len)
{

#if UMAC_SUPPORT_RRM_DEBUG
    u_int8_t *act_string = NULL;
    static u_int8_t *action_names[] = {
        "Radio Req",
        "Radio Resp",
        "Link  Req",
        "Link Resp",
        "Neighbor Req",
        "Neighor Resp",
        "none"
    };
    if (action < 6)
        act_string = action_names[action];
    else
        act_string = action_names[6];

    RRM_DEBUG(vap,RRM_DEBUG_VERBOSE,"Received Frame: Action %s Len %d\n",act_string,frm_len); 
#endif

    switch (action) {
        case IEEE80211_ACTION_RM_REQ:
            ieee80211_recv_radio_measurement_req(vap, ni,
                    (struct ieee80211_action_rm_req *)frm, frm_len);
            break;
        case IEEE80211_ACTION_RM_RESP:
            ieee80211_recv_radio_measurement_rsp(vap, ni,
                    frm, frm_len);
            break;
        case IEEE80211_ACTION_LM_REQ:
            break;
        case IEEE80211_ACTION_LM_RESP:
            ieee80211_recv_lm_rsp(vap, ni,frm, frm_len);
            break;
        case IEEE80211_ACTION_NR_REQ:
            ieee80211_recv_neighbor_req(vap, ni,
                    (struct ieee80211_action_nr_req *)frm, frm_len);
            break;
        case IEEE80211_ACTION_NR_RESP:
            break;
        default:
            break;
    }
    return EOK;
}
/**
 * @brief 
 * Go through the scan list and fill the neighbor
 * AP's information
 *
 * @param arg
 * @param se
 *
 * @return 
 * @return Always success return 0.

 */

int ieee80211_fill_nrinfo(void *arg, wlan_scan_entry_t se)
{
    struct ieee80211_rrm_cbinfo *cb_info = (struct ieee80211_rrm_cbinfo *)arg;
    struct ieee80211_nrresp_info ninfo;
    struct ieee80211_channel *chan;
    u_int8_t scan_ssid_len;
    u_int8_t *scan_ssid = NULL;

    scan_ssid = ieee80211_scan_entry_ssid(se, &scan_ssid_len);
    if (cb_info->ssid && (cb_info->ssid_len == scan_ssid_len) &&
            (OS_MEMCMP(cb_info->ssid, scan_ssid, scan_ssid_len) == 0)) {
        OS_MEMZERO(&ninfo, sizeof(struct ieee80211_nrresp_info));
        IEEE80211_ADDR_COPY(ninfo.bssid, ieee80211_scan_entry_bssid(se));
        chan = ieee80211_scan_entry_channel(se);
        ninfo.channum = chan->ic_ieee;
        ninfo.capinfo = ieee80211_scan_entry_capinfo(se);
        cb_info->frm = ieee80211_add_nr_ie(cb_info->frm, &ninfo);
    }
    return EOK;
}

/**
 * @brief 
 * Go through the scan list and fill the beacon request 
 * information
 *
 * @param arg
 * @param se
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int ieee80211_fill_bcnreq_info(void *arg, wlan_scan_entry_t se)
{
    struct ieee80211_rrm_cbinfo *cb_info = (struct ieee80211_rrm_cbinfo *)arg;
    struct ieee80211_beacon_report *bcnrep=NULL;
    u_int8_t *scan_bssid = NULL,*start=NULL;
    unsigned long tsf;
    struct ieee80211_channel *chan=NULL;
    struct ieee80211_measrsp_ie *bcn_measrsp=NULL;
    u_int8_t bcast[6]={0xff,0xff,0xff,0xff,0xff,0xff};

    scan_bssid = ieee80211_scan_entry_bssid(se); 
    start = (u_int8_t *)(cb_info->frm);

    if(IEEE80211_ADDR_EQ(scan_bssid,cb_info->bssid)) {
        chan = ieee80211_scan_entry_channel(se);
        bcn_measrsp = (struct ieee80211_measrsp_ie *)start;
        bcn_measrsp->id = IEEE80211_ELEMID_MEASREP;
        bcn_measrsp->token=1; // params->rep_dialogtoken; will introduce later 
        bcn_measrsp->rspmode = 0x00; /* Need to validate */
        bcn_measrsp->rsptype = IEEE80211_MEASRSP_BEACON_REPORT;
        bcnrep =(struct ieee80211_beacon_report *)&bcn_measrsp->rsp[0];
        chan = ieee80211_scan_entry_channel(se);
        bcnrep->reg_class = 0x00; /* need to check */
        bcnrep->ch_num = chan->ic_ieee;
        OS_MEMCPY(bcnrep->ms_time,ieee80211_scan_entry_tsf(se),8);
        bcnrep->mduration =cb_info->duration;
        bcnrep->rcpi= ieee80211_scan_entry_rssi(se);
        bcnrep->rsni = bcnrep->rcpi;
        IEEE80211_ADDR_COPY(bcnrep->bssid,scan_bssid);
        bcnrep->ant_id=0x00;
        tsf = OS_GET_TIMESTAMP();
        OS_MEMCPY(bcnrep->parent_tsf,&tsf,4);
        cb_info->frm = &bcnrep->ies[0];
        bcn_measrsp->len = cb_info->frm - start - 2;
        return -EBUSY; /* to stop it */
    } else if (IEEE80211_ADDR_EQ(cb_info->bssid,bcast )) {
        chan = ieee80211_scan_entry_channel(se);
        scan_bssid = ieee80211_scan_entry_bssid(se);
        if(chan->ic_ieee == cb_info->chnum || (!cb_info->chnum)) {
            bcn_measrsp = (struct ieee80211_measrsp_ie *)start;
            bcn_measrsp->id = IEEE80211_ELEMID_MEASREP;
            bcn_measrsp->token=  1 ; // params->rep_dialogtoken; will introduce later 
            bcn_measrsp->rspmode = 0x00; /* Need to validate */
            bcn_measrsp->rsptype = IEEE80211_MEASRSP_BEACON_REPORT;
            bcnrep =(struct ieee80211_beacon_report *)&bcn_measrsp->rsp[0];
            chan = ieee80211_scan_entry_channel(se);
            bcnrep->reg_class = 0x00; /* need to check */
            bcnrep->ch_num = chan->ic_ieee;
            OS_MEMCPY(bcnrep->ms_time,ieee80211_scan_entry_tsf(se),8);
            bcnrep->mduration =cb_info->duration;
            bcnrep->rcpi= ieee80211_scan_entry_rssi(se);
            bcnrep->rsni = bcnrep->rcpi;
            IEEE80211_ADDR_COPY(bcnrep->bssid,scan_bssid);
            bcnrep->ant_id=0x00;
            tsf = OS_GET_TIMESTAMP();
            OS_MEMCPY(bcnrep->parent_tsf,&tsf,4);
            cb_info->frm = &bcnrep->ies[0];
            bcn_measrsp->len = cb_info->frm - start - 2;
        }    
    }
    return EOK;
}
/**
 * send beacon measurement request.
 * @param vaphandle         : handle to the VAP 
 * @param macaddr           : mac addr to send the request 
 * @param binfo             : beacon measurement request information 
 *
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int wlan_send_beacon_measreq(wlan_if_t vaphandle, u_int8_t *macaddr,
                             ieee80211_rrm_beaconreq_info_t *binfo)
{
    wbuf_t wbuf = NULL;
    u_int8_t *frm = NULL;
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211_node *ni=NULL;
    ieee80211_rrm_t rrm = vap->rrm;
    int error = 0;

    RRM_FUNCTION_ENTER;

    if(IEEE80211_ADDR_EQ(vap->iv_myaddr,macaddr)) {
        struct ieee80211_rrmreq_info *params=NULL;

        ni = ieee80211_find_node(&vap->iv_ic->ic_sta, macaddr);

        if(NULL == ni)
            return -EINVAL;

        rrm->ni = ni;
        params = (struct ieee80211_rrmreq_info *)
            OS_MALLOC(vap->rrm->rrm_osdev, sizeof(struct ieee80211_rrmreq_info), 0);

        if(NULL == params)
        {
            ieee80211_free_node(ni);
            return -ENOMEM;
        }
        params->chnum = binfo->channum; 
        params->rm_dialogtoken = IEEE80211_ACTION_RM_TOKEN; 
        IEEE80211_ADDR_COPY(params->bssid,binfo->bssid);
        ieee80211_rrm_set_report_pending(vap,IEEE80211_MEASREQ_BR_TYPE,(void *)params);
        wlan_scan_table_flush(vap);

        if(ieee80211_rrm_scan_start(vap, true)) {
            ieee80211_rrm_free_report(vap->rrm);
            ieee80211_free_node(ni);
            return -EBUSY;
        }
        ieee80211_free_node(ni);
        RRM_FUNCTION_EXIT;

        return EOK;
    } else {        
        if(!(ieee80211_vap_rrm_is_set(vap) && ieee80211_node_is_rrm(vap,macaddr)))
            return -EINVAL;

        ni = ieee80211_find_node(&vap->iv_ic->ic_sta, macaddr);

        if (ni == NULL) 
            return -EINVAL;

        wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm, 0);

        if (wbuf == NULL) {
            error = -ENOMEM;
            goto exit;
        }

        frm = ieee80211_add_rrm_action_ie(frm,IEEE80211_ACTION_RM_REQ, binfo->num_rpt);
        frm = ieee80211_add_measreq_beacon_ie((u_int8_t *)(frm), ni, binfo);
        wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));

        error = ieee80211_send_mgmt(vap, ni, wbuf, false);
        RRM_FUNCTION_EXIT;

exit:
        /* claim node immediately */
        ieee80211_free_node(ni);
        return error;
    }
    return -ENOSPC;
}
/**
 * send traffic stream measurement request.
 * @param vaphandle         : handle to the VAP 
 * @param macaddr           : mac addr to send the request 
 * @param binfo             : traffic stream measurement request information
 *
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int wlan_send_tsm_measreq(wlan_if_t vaphandle, u_int8_t *macaddr,
                          ieee80211_rrm_tsmreq_info_t *tsminfo)
{
    wbuf_t wbuf = NULL;
    u_int8_t *frm = NULL;
    struct ieee80211vap   *vap = vaphandle;
    struct ieee80211_node *ni=NULL;
    int error = 0;

    RRM_FUNCTION_ENTER;

    if(!(ieee80211_vap_rrm_is_set(vap) && ieee80211_node_is_rrm(vap,macaddr)))
        return -EINVAL;

    ni = ieee80211_find_node(&vap->iv_ic->ic_sta, macaddr);

    if (NULL == ni) {
        return -EINVAL;
    }

    wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm, 0);

    if (wbuf == NULL) {
        error = -ENOMEM;
        goto exit;
    }

    frm = ieee80211_add_rrm_action_ie(frm,IEEE80211_ACTION_RM_REQ, tsminfo->num_rpt);

    frm = ieee80211_add_measreq_tsm_ie(frm, tsminfo);

    wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));
    error =  ieee80211_send_mgmt(vap, ni, wbuf, false);

exit:
    /* claim node immediately */
    ieee80211_free_node(ni);
    RRM_FUNCTION_EXIT;
    return error;
}

/**
 * send neighbor report.
 * @param vaphandle         : handle to the VAP 
 * @param macaddr           : mac addr to send the request 
 * @param nrinfo            : neighbor request info 
 *
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int wlan_send_neig_report(wlan_if_t vaphandle, u_int8_t *macaddr,
                          ieee80211_rrm_nrreq_info_t *nrinfo)
{
    struct ieee80211vap      *vap = vaphandle;
    struct ieee80211_node    *ni;
    int error = 0;

    RRM_FUNCTION_ENTER;
    if(!(ieee80211_vap_rrm_is_set(vap) && ieee80211_node_is_rrm(vap,macaddr)))
        return -EINVAL;

    ni = ieee80211_find_node(&vap->iv_ic->ic_sta, macaddr);

    if (ni == NULL) {
        return -EINVAL;
    }

    error = ieee80211_send_neighbor_resp(vap, ni, nrinfo);

    /* claim node immediately */
    ieee80211_free_node(ni);
    RRM_FUNCTION_EXIT;
    return error;
}

/**
 * send link measurement request.
 * @param vaphandle         : handle to the VAP 
 * @param macaddr           : mac addr to send the request 
 *
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int wlan_send_link_measreq(wlan_if_t vaphandle, u_int8_t *macaddr)
{
    wbuf_t wbuf = NULL;
    u_int8_t *frm = NULL;
    struct ieee80211vap  *vap = vaphandle;
    struct ieee80211_node *ni=NULL;
    struct ieee80211_action_lm_req *lm_req=NULL;
    int error = 0;
    struct ieee80211com  *ic = vap->iv_ic;

    RRM_FUNCTION_ENTER;

    if(!(ieee80211_vap_rrm_is_set(vap) && ieee80211_node_is_rrm(vap,macaddr)))
        return -EINVAL;

    ni = ieee80211_find_node(&vap->iv_ic->ic_sta, macaddr);

    if (ni == NULL) 
        return -EINVAL;

    wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm, 0);

    if (wbuf == NULL) {
        error = -ENOMEM;
        goto exit;
    }

    lm_req = (struct ieee80211_action_lm_req*)(frm);
    lm_req->header.ia_category = IEEE80211_ACTION_CAT_RM;
    lm_req->header.ia_action = IEEE80211_ACTION_LM_REQ;
    lm_req->dialogtoken = IEEE80211_ACTION_LM_TOKEN;
    lm_req->txpwr_used = (u_int8_t)(ieee80211_node_get_txpower(ni)>>1); /* stored in 0.5 dbm */
    lm_req->max_txpwr =  ic->ic_curchanmaxpwr;
    frm = &lm_req->opt_ies[0];

    wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));
    error = ieee80211_send_mgmt(vap, ni, wbuf, false);

exit:
    RRM_FUNCTION_EXIT;

    /* claim node immediately */
    ieee80211_free_node(ni);

    return error;
}

/**
 * @brief 
 *
 * @param vap
 * @param macaddr
 * @param stastats
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */
int wlan_send_stastats_req(wlan_if_t vap, u_int8_t *macaddr, ieee80211_rrm_stastats_info_t *stastats)
{
    struct ieee80211_node *ni=NULL;
    wbuf_t wbuf = NULL;
    u_int8_t *frm = NULL;
    struct ieee80211com    *ic = vap->iv_ic;
    int error = 0;

    RRM_FUNCTION_ENTER;
    if(!(ieee80211_vap_rrm_is_set(vap) && ieee80211_node_is_rrm(vap,macaddr)))
        return -EINVAL;

    ni = ieee80211_find_node(&ic->ic_sta, macaddr);


    if (ni == NULL)
        return -EINVAL;
    
    wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm,0);

    if (wbuf == NULL) {
        error = -ENOMEM;
        goto exit;
    }

	frm = ieee80211_add_rrm_action_ie(frm,IEEE80211_ACTION_RM_REQ, stastats->num_rpts);

    frm = ieee80211_add_measreq_stastats_ie(frm, ni, stastats);

    wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));
    error = ieee80211_send_mgmt(vap, ni, wbuf, false);

    RRM_FUNCTION_EXIT;
exit:

    /* claim node immediately */
    ieee80211_free_node(ni);
    return error;
}
/**
 * @brief
 *
 * @param vaphandle
 * @param macaddr
 * @param chinfo
 *
 * @return
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int wlan_send_lci_req(wlan_if_t vaphandle, u_int8_t *macaddr,
        ieee80211_rrm_lcireq_info_t *lcireq_info)
{
    struct ieee80211vap    *vap = vaphandle;
    struct ieee80211com    *ic = vap->iv_ic;
    struct ieee80211_node  *ni=NULL;
    wbuf_t wbuf = NULL;
    u_int8_t *frm = NULL;
    int error=0;

    RRM_FUNCTION_ENTER;

    if(!(ieee80211_vap_rrm_is_set(vap) && ieee80211_node_is_rrm(vap,macaddr)))
        return -EINVAL;

    ni = ieee80211_find_node(&ic->ic_sta, macaddr);
    if (ni == NULL) {
        return -EINVAL;
    }
  
    /* Save the location subject value to determine
     * the received report contains LCI of requesting/reporting
     * station 
     */
    ni->ni_rrmlci_loc = lcireq_info->location;  
    wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm,0);
    if (wbuf == NULL) {
        error = -ENOMEM;
        goto exit;
    }

    frm = ieee80211_add_rrm_action_ie(frm,IEEE80211_ACTION_RM_REQ, lcireq_info->num_rpts);
    frm = ieee80211_add_measreq_lci_ie(frm, ni, lcireq_info);
    wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));
    error = ieee80211_send_mgmt(vap, ni, wbuf, false);
    RRM_FUNCTION_EXIT;

exit:
    ieee80211_free_node(ni);
    return error;
}
/**
 * @brief
 *
 * @param vaphandle
 * @param macaddr
 * @param chinfo
 *
 * @return
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int wlan_get_rrmstats(wlan_if_t vaphandle, u_int8_t *macaddr,
                      ieee80211_rrmstats_t *rrmstats)
{
    struct ieee80211vap    *vap = vaphandle;
    struct ieee80211com    *ic = vap->iv_ic;
    struct ieee80211_rrm   *rrm = vap->rrm;
    struct ieee80211_node  *ni=NULL;
    u_int8_t zero_mac[IEEE80211_ADDR_LEN] = {0x00,0x00,0x00,0x00,0x00,0x00};
    u_int32_t chnum;

    RRM_FUNCTION_ENTER;

    if(!(ieee80211_vap_rrm_is_set(vap))) 
        return -EINVAL;

    if (!IEEE80211_ADDR_EQ(zero_mac, macaddr) && ieee80211_node_is_rrm(vap,macaddr)) {
        ni = ieee80211_find_node(&ic->ic_sta, macaddr);
        if (ni == NULL) {
            return -EINVAL;
        }
        OS_MEMCPY(&rrmstats->ni_rrm_stats, ni->ni_rrm_stats, sizeof(rrmstats->ni_rrm_stats));
        ieee80211_free_node(ni);
        RRM_FUNCTION_EXIT;
        return EOK;
    }
    for (chnum = 1; chnum < IEEE80211_CHAN_MAX; chnum++) {
        if (rrm->u_chload[chnum] != 0) {
            rrmstats->chann_load[chnum] = rrm->u_chload[chnum]; 
        }
        if ((OS_MEMCMP(&rrm->user_nhist_resp[chnum], &rrmstats->noise_data[chnum],
                        sizeof(ieee80211_rrm_noise_data_t))) != 0) {
            OS_MEMCPY(&rrmstats->noise_data[chnum], &rrm->user_nhist_resp[chnum],
                    sizeof(ieee80211_rrm_noise_data_t));
        }
    }
    RRM_FUNCTION_EXIT;
    return EOK;    
}

/**
 * @brief  to send beacon report to upper layers 
 *
 * @param vaphandle
 * @param macaddr
 * @param bcnrpt
 *
 * @return zero if no more entries are present
 */
int wlan_get_bcnrpt(wlan_if_t vap, u_int8_t *macaddr, u_int32_t index,
        ieee80211_bcnrpt_t  *bcnrpt)
{
    int cnt = 1;
    struct ieee80211_beacon_report_table *btable = vap->rrm->beacon_table; 

    if(btable == NULL) {
        bcnrpt->more = 0;
        return EOK ;
    }
    RRM_BTABLE_LOCK(btable);
    {
        struct ieee80211_beacon_entry   *current_beacon_entry;
        u_int8_t flag=0;
        TAILQ_FOREACH(current_beacon_entry ,&(btable->entry),blist) {
            struct ieee80211_beacon_report *report = &current_beacon_entry->report;
            if(cnt == index) {
                IEEE80211_ADDR_COPY(bcnrpt->bssid,report->bssid);
                bcnrpt->rsni = report->rsni;
                bcnrpt->rcpi = report->rcpi;
                bcnrpt->chnum = report->ch_num;
                flag = 1;
                break;
            }
            cnt++;
        }

        if(flag)
            bcnrpt->more = cnt;
        else 
            bcnrpt->more = 0;
    }
    RRM_BTABLE_UNLOCK(btable);
    return EOK;
}

/**
 * @brief send channel load request from ap  
 *
 * @param vaphandle
 * @param macaddr
 * @param chinfo
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int wlan_send_chload_req(wlan_if_t vaphandle, u_int8_t *macaddr,
        ieee80211_rrm_chloadreq_info_t *chinfo)
{

    struct ieee80211vap    *vap = vaphandle;
    struct ieee80211com    *ic = vap->iv_ic;
    struct ieee80211_node  *ni=NULL;
    wbuf_t wbuf = NULL;
    u_int8_t *frm = NULL;
    int error=0; 

    RRM_FUNCTION_ENTER;

    if(!(ieee80211_vap_rrm_is_set(vap) && ieee80211_node_is_rrm(vap,macaddr)))
        return -EINVAL;

    ni = ieee80211_find_node(&ic->ic_sta, macaddr);

    if (ni == NULL) {
        return -EINVAL;
    }

    wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm,0);


    if (wbuf == NULL) {
        error = -ENOMEM;
        goto exit;
    }

    frm = ieee80211_add_rrm_action_ie(frm,IEEE80211_ACTION_RM_REQ, chinfo->num_rpts);

    frm = ieee80211_add_measreq_chload_ie(frm, ni, chinfo);

    wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));

    error = ieee80211_send_mgmt(vap, ni, wbuf, false);
    RRM_FUNCTION_EXIT;
exit:

    ieee80211_free_node(ni);
    return error;
}
/**
 * @brief to send Noise histogram from ap 
 *
 * @param vap
 * @param macaddr
 * @param nhist
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int wlan_send_nhist_req(wlan_if_t vap, u_int8_t *macaddr, ieee80211_rrm_nhist_info_t *nhist)
{
    wbuf_t wbuf = NULL;
    u_int8_t *frm = NULL;
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni=NULL;
    int error = 0;

    RRM_FUNCTION_ENTER;

    if(!(ieee80211_vap_rrm_is_set(vap) && ieee80211_node_is_rrm(vap,macaddr)))
        return -EINVAL;

    ni = ieee80211_find_node(&ic->ic_sta, macaddr);

    if (ni == NULL) {
        return -EINVAL;
    }

    wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm,0);

    if (wbuf == NULL) {
        error = -ENOMEM;
        goto exit;
    }

    frm = ieee80211_add_rrm_action_ie(frm,IEEE80211_ACTION_RM_REQ, nhist->num_rpts);
    frm = ieee80211_add_measreq_nhist_ie(frm, ni, nhist);
    wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));
    error = ieee80211_send_mgmt(vap, ni, wbuf, false);
    RRM_FUNCTION_EXIT;
exit:

    ieee80211_free_node(ni);
    return error;
}

/**
 * @brief send frame request from ap  
 *
 * @param vap
 * @param macaddr
 * @param fr_info
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int wlan_send_frame_request(wlan_if_t vap, u_int8_t *macaddr, 
                            ieee80211_rrm_frame_req_info_t  *fr_info)
{
    wbuf_t wbuf = NULL;
    u_int8_t *frm = NULL;
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni;
    int error = 0;

    RRM_FUNCTION_ENTER;

    if(!(ieee80211_vap_rrm_is_set(vap) && ieee80211_node_is_rrm(vap,macaddr)))
        return -EINVAL;

    ni = ieee80211_find_node(&ic->ic_sta, macaddr);

    if (ni == NULL)
        return -EINVAL;

    wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm,0);

    if (wbuf == NULL) {
        error = -ENOMEM;
        goto exit;
    }

    frm = ieee80211_add_rrm_action_ie(frm,IEEE80211_ACTION_RM_REQ, fr_info->num_rpts);
    frm = ieee80211_add_measreq_frame_req_ie(frm, ni,fr_info);
    wbuf_set_pktlen(wbuf, (frm - (u_int8_t*)wbuf_header(wbuf)));
    error = ieee80211_send_mgmt(vap, ni, wbuf, false);
    RRM_FUNCTION_EXIT;
exit:
    ieee80211_free_node(ni);
    return error;
}

/**
 * @brief free scan resource after scan   
 *
 * @param rrm
 * @return void
 */

static void ieee80211_rrm_free_scan_resource(ieee80211_rrm_t rrm)
{
    int    rc;
    rc = wlan_scan_unregister_event_handler(rrm->rrm_vap, 
                                            ieee80211_rrm_scan_evhandler, 
                                            (void *)rrm);
    if (rc != EOK) {
        IEEE80211_DPRINTF(rrm->rrm_vap, IEEE80211_MSG_RRM,
        "%s: wlan_scan_unregister_event_handler() failed handler=%p,%p rc=%x\n",
          __func__, ieee80211_rrm_scan_evhandler, rrm, rc);
    }
    wlan_scan_clear_requestor_id(rrm->rrm_vap, rrm->rrm_scan_requestor);
}



/**
 * @brief scan event handler , will be called at every channel switch  
 *
 * @param originator
 * @param event
 * @param arg
 * @return void
 */

static void ieee80211_rrm_scan_evhandler(struct ieee80211vap *originator, ieee80211_scan_event *event, void *arg)
{
    ieee80211_rrm_t rrm = (ieee80211_rrm_t)arg;
    struct ieee80211_chan_stats chan_stats;
    struct ieee80211com *ic = originator->iv_ic;
    u_int32_t temp = 0;

    RRM_DEBUG(rrm->rrm_vap, RRM_DEBUG_VERBOSE,"%s scan_id %08X event %d reason %d \n",
              __func__, event->scan_id, event->type, event->reason);

    if (event->type == IEEE80211_SCAN_RADIO_MEASUREMENT_START) {
        /* get initial chan stats for the current channel */
        int ieee_chan_num = ieee80211_chan2ieee(originator->iv_ic,event->chan);
        (void)ic->ic_get_cur_chan_stats(ic, &chan_stats);
        rrm->rrm_chan_load[ieee_chan_num] = chan_stats.chan_clr_cnt;
        rrm->rrm_cycle_count[ieee_chan_num] = chan_stats.cycle_cnt;

        RRM_DEBUG(rrm->rrm_vap,RRM_DEBUG_VERBOSE, "%s: initial_stats noise %d ch: %d ch load: %d cycle cnt %d \n ", 
                  __func__, rrm->rrm_noisefloor[ieee_chan_num], ieee_chan_num,
                  chan_stats.chan_clr_cnt,chan_stats.cycle_cnt );
    }
    if(event->type == IEEE80211_SCAN_RADIO_MEASUREMENT_END)  {
        int ieee_chan_num = ieee80211_chan2ieee(originator->iv_ic,event->chan);
        /* Get the noise floor value */
        if (ieee_chan_num != IEEE80211_CHAN_ANY) {
            rrm->rrm_noisefloor[ieee_chan_num] = ic->ic_get_cur_chan_nf(ic);
        }
#define MIN_CLEAR_CNT_DIFF 1000
        /* get final chan stats for the current channel*/
       (void) ic->ic_get_cur_chan_stats(ic, &chan_stats);
       rrm->rrm_chan_load[ieee_chan_num] = chan_stats.chan_clr_cnt - rrm->rrm_chan_load[ieee_chan_num] ;
       rrm->rrm_cycle_count[ieee_chan_num] = chan_stats.cycle_cnt - rrm->rrm_cycle_count[ieee_chan_num] ;

       /* make sure when new clr_cnt is more than old clr cnt, ch utilization is non-zero */
       if (rrm->rrm_chan_load[ieee_chan_num] > MIN_CLEAR_CNT_DIFF ) {
           temp = (u_int32_t)(255* rrm->rrm_chan_load[ieee_chan_num]); /* we are taking % w,r,t 255 as per specification */
           temp = (u_int32_t)( temp/(rrm->rrm_cycle_count[ieee_chan_num]));
           rrm->rrm_chan_load[ieee_chan_num] = MAX( 1,temp);
       } else 
           rrm->rrm_chan_load[ieee_chan_num] = 0;

       RRM_DEBUG(rrm->rrm_vap,RRM_DEBUG_VERBOSE,"%s: final_stats noise: %d ch: %d ch load: %d cycle cnt: %d \n ",
                 __func__,rrm->rrm_noisefloor[ieee_chan_num], ieee_chan_num,
                 rrm->rrm_chan_load[ieee_chan_num], rrm->rrm_cycle_count[ieee_chan_num] );
#undef MIN_CLEAR_CNT_DIFF
    }

    if(event->type == IEEE80211_SCAN_COMPLETED) {
        ieee80211_rrm_free_scan_resource(rrm);
        if(rrm->pending_report)
            ieee80211_send_report(rrm);
    }
    return;
}


/**
 * @brief to start scan from rrm  
 *
 * @param vap
 * @param active_scan
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int ieee80211_rrm_scan_start(wlan_if_t vap, bool active_scan)
{
    ieee80211_rrm_t rrm;
    ieee80211_scan_params *scan_params=NULL;
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_rrmreq_info *rrm_req_params=NULL;
    enum ieee80211_opmode opmode = wlan_vap_get_opmode(vap);
    bool is_connected = wlan_is_connected(vap);
    struct ieee80211_channel *chan;

    rrm = vap->rrm;

    rrm_req_params = rrm->rrmcb;

    RRM_FUNCTION_ENTER;

    if(wlan_scan_register_event_handler(vap, ieee80211_rrm_scan_evhandler, (void *)rrm) == ENOSPC )
        return -EBUSY;  

    if(wlan_scan_get_requestor_id(vap, (u_int8_t*)"rrm", &rrm->rrm_scan_requestor) == EINVAL) {
        wlan_scan_unregister_event_handler(vap, 
                                  ieee80211_rrm_scan_evhandler, 
                                  (void *)rrm);
        return -EBUSY;
    }

    scan_params = (ieee80211_scan_params *)
                 OS_MALLOC(rrm->rrm_osdev, sizeof(*scan_params), GFP_KERNEL);

    if (NULL == scan_params)
        return -ENOMEM;

    OS_MEMZERO(scan_params, sizeof(ieee80211_scan_params));

    if ((opmode == IEEE80211_M_STA) || (opmode == IEEE80211_M_IBSS)) {
        wlan_set_default_scan_parameters(vap,scan_params,opmode,active_scan,
                                           true,is_connected,true,0,NULL,0);
        scan_params->type = IEEE80211_SCAN_BACKGROUND;
        scan_params->flags &= ~IEEE80211_SCAN_ALLBANDS;
        scan_params->flags |= IEEE80211_SCAN_ADD_BCAST_PROBE;
        scan_params->repeat_probe_time =  30;
        scan_params->min_dwell_time_active = scan_params->max_dwell_time_active = 110;
        
        if(rrm_req_params->chnum) {
            scan_params->type = IEEE80211_SCAN_RADIO_MEASUREMENTS;
            scan_params->num_channels = 1; 
            scan_params->chan_list =(u_int32_t *)OS_MALLOC(ic->ic_osdev,
                    scan_params->num_channels * sizeof(u_int32_t),
                    GFP_KERNEL);

            chan = ieee80211_find_dot11_channel(ic, rrm_req_params->chnum, IEEE80211_MODE_AUTO);

            if (chan == NULL) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_RRM,
                        "%s: Invalid channel passed for rrm request: %u\n", __func__, rrm_req_params->chnum);
                return -EINVAL;
            }
            scan_params->chan_list[0] = rrm_req_params->chnum;
        }
    } else if (opmode == IEEE80211_M_HOSTAP) {
        wlan_set_default_scan_parameters(vap,scan_params,opmode,active_scan,true,true,true,0,NULL,0);
        scan_params->flags = IEEE80211_SCAN_PASSIVE;
        scan_params->type = IEEE80211_SCAN_FOREGROUND;
        scan_params->min_dwell_time_passive = 200;
        scan_params->max_dwell_time_passive = 300;
    } else {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_RRM,
                "%s: rrm request received in incorrect vap mode\n", __func__);
        return -EINVAL;
    } 
    switch (vap->iv_des_mode) {
        case IEEE80211_MODE_11A:
        case IEEE80211_MODE_TURBO_A:
        case IEEE80211_MODE_11NA_HT20:
        case IEEE80211_MODE_11NA_HT40PLUS:
        case IEEE80211_MODE_11NA_HT40MINUS:
        case IEEE80211_MODE_11NA_HT40:
            scan_params->flags |= IEEE80211_SCAN_5GHZ;
            break;
        case IEEE80211_MODE_11B:
        case IEEE80211_MODE_11G:
        case IEEE80211_MODE_TURBO_G:
        case IEEE80211_MODE_11NG_HT20:
        case IEEE80211_MODE_11NG_HT40PLUS:
        case IEEE80211_MODE_11NG_HT40MINUS:
        case IEEE80211_MODE_11NG_HT40:
            scan_params->flags |= IEEE80211_SCAN_2GHZ;
            break;
        default:
            scan_params->flags |= IEEE80211_SCAN_ALLBANDS;
            break;
    }
    if (wlan_scan_start(vap, scan_params, rrm->rrm_scan_requestor,
                     IEEE80211_SCAN_PRIORITY_LOW, &(rrm->rrm_scan_id))) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_RRM,
                         "%s: Issue a scan fail.\n", __func__);
        ieee80211_rrm_free_scan_resource(rrm);
        return -EINVAL;
    }

    if (scan_params->chan_list)
        OS_FREE(scan_params->chan_list);

    OS_FREE(scan_params);

    RRM_FUNCTION_EXIT;
    return EOK;
}

/**
 * @brief setting report as pending  
 *
 * @param vap
 * @param type
 * @param cbinfo
 *
 * @return 
 * @return on success return 0.
 */

int ieee80211_rrm_set_report_pending(wlan_if_t vap,u_int8_t type,void *cbinfo)
{
    ieee80211_rrm_t rrm;
    rrm = vap->rrm;
    rrm->pending_report = TRUE;
    rrm->pending_report_type = type;
    rrm->rrmcb = cbinfo;
    return EOK;
}

/**
 * @brief 
 *
 * @param rrm
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int ieee80211_rrm_free_report(ieee80211_rrm_t rrm)
{
    if(rrm->rrmcb) {
        OS_FREE(rrm->rrmcb);
        rrm->pending_report = FALSE;
        rrm->rrm_in_progress = 0;
        rrm->rrmcb = NULL;
    }
    return EOK;
}

/**
 * @brief 
 *
 * @param vap
 * @param mac
 *
 * @return 
 * @return on success return 1.
 */

int ieee80211_node_is_rrm(wlan_if_t vap,char *mac)
{
    struct ieee80211_node *ni  = NULL;
    struct ieee80211_node_table *nt = &vap->iv_ic->ic_sta;

    TAILQ_FOREACH(ni, &nt->nt_node, ni_list) {
      if(ni->ni_flags & IEEE80211_NODE_RRM  
         &&(IEEE80211_ADDR_EQ(ni->ni_macaddr,mac)))
          return 1; 
    }
    return EOK;   
}

/**
 * @brief will be called from association to mark node as rrm  
 *
 * @param ni
 * @param flag
 */
void ieee80211_set_node_rrm(struct ieee80211_node *ni,uint8_t flag)
{
    
#if UMAC_SUPPORT_RRM_DEBUG    
    ieee80211_vap_t vap = ni->ni_vap;
#endif

    RRM_FUNCTION_ENTER;

    if(flag) {
        ni->ni_rrm_stats =(ieee80211_rrm_node_stats_t *)OS_MALLOC(ni->ni_ic, sizeof(ieee80211_rrm_node_stats_t), 0); 
        if( NULL == ni->ni_rrm_stats) {
            IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_RRM," can not allocate memory %s \n",__func__);
            return;
        }
        ni->ni_flags |= IEEE80211_NODE_RRM;
        OS_GET_RANDOM_BYTES(&ni->ni_rrmreq_type,sizeof(ni->ni_rrmreq_type));
        ni->ni_rrmreq_type %= 5;
    }
    else {
        if(ni->ni_rrm_stats)
            OS_FREE(ni->ni_rrm_stats);
        ni->ni_flags &= ~IEEE80211_NODE_RRM;
        ni->ni_rrmreq_type = 0;
    }

    RRM_FUNCTION_EXIT;
    return;
}


/**
 * @brief to attach rrm module from vap attach  
 *
 * @param ic
 * @param vap
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int ieee80211_rrm_vattach(struct ieee80211com *ic,ieee80211_vap_t vap)
{
    ieee80211_rrm_t  *rrm = &vap->rrm;
    struct ieee80211_beacon_report_table *btable=NULL;
    enum ieee80211_opmode opmode = wlan_vap_get_opmode(vap);
    

    if (*rrm)
        return -EINPROGRESS; /* already attached ? */

    *rrm = (ieee80211_rrm_t) OS_MALLOC(ic->ic_osdev, sizeof(struct ieee80211_rrm), 0);

    if(NULL == *rrm)
        return -ENOMEM;

    RRM_FUNCTION_ENTER;

    OS_MEMZERO(*rrm, sizeof(struct ieee80211_rrm));

    btable =  (struct ieee80211_beacon_report_table *)
        OS_MALLOC(ic->ic_osdev, (sizeof(struct ieee80211_beacon_report_table)), 0);

    if(NULL == btable)
        return -ENOMEM;
 
    if (opmode == IEEE80211_M_IBSS) /* making it enabled by default for adhoc */
        ieee80211_vap_rrm_set(vap);

    OS_MEMZERO(btable, sizeof(struct ieee80211_beacon_report_table));

    TAILQ_INIT(&(btable->entry));
    
    /* XXX lock alloc failure ?*/
    RRM_BTABLE_LOCK_INIT(btable);
    (*rrm)->rrm_osdev = ic->ic_osdev; 
    (*rrm)->rrm_vap = vap; 
    rrm_attach_misc(*rrm);
    (*rrm)->beacon_table = btable;
#if UMAC_SUPPORT_RRM_DEBUG
    (*rrm)->rrm_dbg_lvl = RRM_DEBUG_DEFAULT;
#endif

    RRM_FUNCTION_EXIT;

    return EOK;
}

/**
 * @brief at time of shutdown  
 *
 * @param vap
 *
 * @return 
 */
int ieee80211_rrm_vdetach(ieee80211_vap_t vap)
{
    
    ieee80211_rrm_t *rrm = &vap->rrm;

    if (*rrm == NULL) 
        return -EINPROGRESS; /* already detached ? */

    RRM_FUNCTION_ENTER;
    rrm_detach_misc(*rrm);

    RRM_BTABLE_LOCK_DESTROY((*rrm)->beacon_table);

    OS_FREE((*rrm)->beacon_table);

    OS_FREE(*rrm);

    *rrm = NULL;

    return EOK;
}
#if UMAC_SUPPORT_RRM_DEBUG
/**
 * @brief set debug level  
 *
 * @param vap
 * @param val
 *
 * @return 
 */
int ieee80211_rrmdbg_set(wlan_if_t vap, u_int32_t val)
{
  ieee80211_rrm_t rrm = vap->rrm;
  rrm->rrm_dbg_lvl = val;
  return EOK; 
}

/**
 * @brief get debug level  
 *
 * @param vap
 *
 * @return 
 */
int ieee80211_rrmdbg_get(wlan_if_t vap)
{
  return vap->rrm->rrm_dbg_lvl;
}
#endif
#endif /* UMAC_SUPPORT_RRM */
