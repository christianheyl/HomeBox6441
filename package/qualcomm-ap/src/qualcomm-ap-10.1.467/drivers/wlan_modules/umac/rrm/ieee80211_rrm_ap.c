/*
 * Copyright (c) 2013 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */


/*
 *  Radio Resource measurements message handlers for AP.
 */
#include <ieee80211_var.h>
#include "ieee80211_rrm_priv.h"

#if UMAC_SUPPORT_RRM

/*
 * Format and send neighbor report response
 */

/**
 * @brief Routine to generate neighbor report  
 *
 * @param vap
 * @param ni
 * @param nrreq_info
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int ieee80211_send_neighbor_resp(wlan_if_t vap, wlan_node_t ni,
                                        ieee80211_rrm_nrreq_info_t* nrreq_info)
{
    wbuf_t wbuf = NULL;
    u_int8_t *frm = NULL;
    struct ieee80211_rrm_cbinfo cb_info;
    struct ieee80211_action_nr_resp *nr_resp;
    RRM_FUNCTION_ENTER;

    wbuf = ieee80211_getmgtframe(ni, IEEE80211_FC0_SUBTYPE_ACTION, &frm, 0);

    if (wbuf == NULL) {
        return -ENOMEM;
    }

    nr_resp = (struct ieee80211_action_nr_resp*)(frm);
    nr_resp->header.ia_category = IEEE80211_ACTION_CAT_RM;
    nr_resp->header.ia_action = IEEE80211_ACTION_NR_RESP;
    nr_resp->dialogtoken = nrreq_info->dialogtoken;
    cb_info.frm = (u_int8_t *)(&nr_resp->resp_ies[0]);
    cb_info.ssid = nrreq_info->ssid; 
    cb_info.ssid_len = nrreq_info->ssid_len;
    /* Iterate the scan table to build Neighbor report */
    wlan_scan_table_iterate(vap, ieee80211_fill_nrinfo, &cb_info);
    wbuf_set_pktlen(wbuf, (cb_info.frm - (u_int8_t*)wbuf_header(wbuf)));

    RRM_FUNCTION_EXIT;
    return ieee80211_send_mgmt(vap, ni, wbuf, false);
}

/*
 * Neighbor report request handler
 */

/**
 * @brief Routine to parse neighnor report request  
 *
 * @param vap
 * @param ni
 * @param nr_req
 * @param frm_len
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int ieee80211_recv_neighbor_req(wlan_if_t vap, wlan_node_t ni, 
                                struct ieee80211_action_nr_req *nr_req, int frm_len)
{
    ieee80211_rrm_nrreq_info_t nrreq_info;
    u_int8_t elem_id, len;
    u_int8_t *frm, *sfrm;
    RRM_FUNCTION_ENTER;

    sfrm = frm = &nr_req->req_ies[0];
    nrreq_info.dialogtoken = nr_req->dialogtoken;
    while ((frm-sfrm) < frm_len) {
        elem_id = *frm++;
        len = *frm++;
        switch (elem_id) {
            case IEEE80211_ELEMID_SSID:
                OS_MEMCPY(nrreq_info.ssid,frm, len);
                nrreq_info.ssid_len = len;
                break;
            default :
                break;
        }
        frm += len;
    }
    RRM_FUNCTION_EXIT;
    /* send Neigbor info */
    return ieee80211_send_neighbor_resp(vap, ni, &nrreq_info);
}

/**
 * @brief to parse rrm report , 
 * its main entry point for parsing reports
 * @param vap
 * @param frm
 * @param frm_len
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

static int ieee80211_rrm_handle_report(wlan_if_t vap, u_int8_t *frm, u_int32_t frm_len) 
{

    struct ieee80211_measrsp_ie *rsp= (struct ieee80211_measrsp_ie *) frm;

    RRM_FUNCTION_ENTER;
    if((rsp->id !=IEEE80211_ELEMID_MEASREP)
            ||(rsp->rspmode & (BIT_INCAPABLE | BIT_LATE | BIT_REFUSED))
            ||(rsp->len > frm_len))
        return -EINVAL;
    switch(rsp->rsptype) 
    {
        case IEEE80211_MEASRSP_BASIC_REPORT:
            break;
        case IEEE80211_MEASRSP_CCA_REPORT:
            break;
        case IEEE80211_MEASRSP_RPI_HISTOGRAM_REPORT:
            break;
        case IEEE80211_MEASRSP_CHANNEL_LOAD_REPORT:
            ieee80211_rrm_chload_report(vap,frm,frm_len);
            break;
        case IEEE80211_MEASRSP_NOISE_HISTOGRAM_REPORT:
            ieee80211_rrm_nhist_report(vap,frm,frm_len);
            break;
        case IEEE80211_MEASRSP_BEACON_REPORT:
            ieee80211_rrm_beacon_measurement_report(vap,frm,frm_len);
            break;
        case IEEE80211_MEASRSP_FRAME_REPORT:
            ieee80211_rrm_frm_report(vap,frm,frm_len);
            break;
        case IEEE80211_MEASRSP_STA_STATS_REPORT:
            ieee80211_rrm_stastats_report(vap,frm,frm_len);
            break;
        case IEEE80211_MEASRSP_LCI_REPORT:
            ieee80211_rrm_lci_report(vap,frm,frm_len);
            break;
        case IEEE80211_MEASRSP_TSM_REPORT:
            ieee80211_rrm_tsm_report(vap,frm,frm_len);
            break;
        default:
            RRM_DEBUG(vap,RRM_DEBUG_VERBOSE,"%s Unknown rsptype  %d\n",__func__,rsp->rsptype);
            break;
    }
    RRM_FUNCTION_EXIT;
    return EOK;
}

/**
 * @brief Routine to parse linkmeasuremnt response  
 *
 * @param vap
 * @param ni
 * @param frm
 * @param frm_len
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int ieee80211_recv_lm_rsp(wlan_if_t vap, wlan_node_t ni,
        u_int8_t *frm, u_int32_t frm_len) 
{
    struct ieee80211_action_lm_rsp *rsp = (struct ieee80211_action_lm_rsp *)frm;
    ieee80211_rrm_node_stats_t *stats =(ieee80211_rrm_node_stats_t *)ni->ni_rrm_stats;
    ieee80211_rrm_lm_data_t *ni_lm = &(stats->lm_data);
    
    RRM_FUNCTION_ENTER;

    if( (rsp->dialogtoken != IEEE80211_ACTION_LM_TOKEN) 
            || (frm_len <  sizeof(struct ieee80211_action_lm_rsp))) 
    {
        return -EINVAL;
    }
    RRM_DEBUG(vap,RRM_DEBUG_VERBOSE,"%s: tx_power %d lmargin %d rxant %d rcpi %d rsni %d\n",
              __func__, rsp->tpc.tx_pow,rsp->tpc.lmargin,rsp->rxant,rsp->rcpi,rsp->rsni);

    ni_lm->tx_pow = rsp->tpc.tx_pow;
    ni_lm->lmargin = rsp->tpc.lmargin;
    ni_lm->rxant = rsp->rxant;
    ni_lm->rxant = rsp->txant;
    ni_lm->rcpi = rsp->rcpi;
    ni_lm->rsni = rsp->rsni;

    RRM_FUNCTION_EXIT;
    return EOK;
}

/**
 * @brief To parse all measuremnt response  
 *
 * @param vap
 * @param ni
 * @param rm_rsp
 * @param frm_len
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int ieee80211_recv_radio_measurement_rsp(wlan_if_t vap, wlan_node_t ni,
        u_int8_t *frm, u_int32_t frm_len)
{
    RRM_FUNCTION_ENTER;

    if(!(ieee80211_vap_rrm_is_set(vap) && ieee80211_node_is_rrm(vap,ni->ni_macaddr)))
        return -EINVAL;
    
    frm +=2; /* catagory + action */

    if(*frm != IEEE80211_ACTION_RM_TOKEN)
        return -EINVAL;

    vap->rrm->ni = ni;
    vap->rrm->rrm_vap = vap;

    frm++; /* dialogue token */
    frm_len -= (sizeof(struct ieee80211_action_rm_rsp) -1 );
     
    ieee80211_rrm_handle_report(vap,frm,frm_len);

    RRM_FUNCTION_EXIT;
    return EOK;
}

/**
 * @brief  parse channel load report 
 *
 * @param vap
 * @param ni
 * @param chload
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int ieee80211_rrm_chload_report(wlan_if_t vap,u_int8_t *frm,u_int32_t frm_len)
{
    struct ieee80211_measrsp_ie *mie= (struct ieee80211_measrsp_ie *) frm;
    struct ieee80211_chloadrsp *chload = (struct ieee80211_chloadrsp *) (&mie->rsp[0]);
    u_int8_t elen = mie->len,*elmid;

    RRM_FUNCTION_ENTER;

    if(( mie->token != IEEE80211_MEASREQ_CHLOAD_TOKEN) || (elen < RRM_MIN_RSP_LEN)) /* minimum length should be three */
        return -EINVAL;

    /* element id  len  measurement token  measurement mode  measurement type  
        1 +         1      +1                    +1         +     1 */

    elen -= ((sizeof(struct ieee80211_measrsp_ie) - 3 /* elementid + length + rsp */)
            + (sizeof(struct ieee80211_chloadrsp) - 1));

    /* Rclass  Channel  Mstart  mduration  channel load 
       1 +      +1      +8        2          +1    */

    frm += ((sizeof(struct ieee80211_measrsp_ie) -1)
            + (sizeof(struct ieee80211_chloadrsp) - 1 ));

    RRM_DEBUG(vap, RRM_DEBUG_INFO, 
              "%s : duration %d chnum %d regclass %d\n", __func__,  
               chload->mduration, chload->chnum, chload->regclass);

    chload->chload= (chload->chload *100)/255; /* as per specification */ 

    RRM_DEBUG(vap, RRM_DEBUG_INFO,"%s : chnum %d chload %d\n", 
               __func__, chload->chnum, chload->chload);

    vap->rrm->u_chload[chload->chnum] = chload->chload;
    
    while(elen  > sizeof(struct ieee80211_ie_header) )
    {
        u_int8_t info_len ;

        elmid = frm++;
        info_len = *frm++;
        
        if (info_len == 0) {
            frm++;    /* next IE */
            continue;
        }

        if (elen < info_len) {
            /* Incomplete/bad info element */
            return -EINVAL;
        }

        switch(*elmid)
        {
            case IEEE80211_ELEMID_VENDOR:
                /*handle vendor ie*/
                break;
            default:
                break;
        }

        elen -= info_len;
        frm += info_len;
    }

    set_chload_window(vap->rrm,chload->chload);
    RRM_FUNCTION_EXIT;
    return EOK;
}

/**
 * @brief Frame report parsing  
 *
 * @param vap
 * @param frm
 * @param frm_len
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int ieee80211_rrm_frm_report(wlan_if_t vap,u_int8_t *frm, u_int32_t frm_len) 
{
    struct ieee80211_measrsp_ie *mie = (struct ieee80211_measrsp_ie *) frm;
    u_int8_t elen = mie->len,*elmid,info_len =0;
    ieee80211_rrm_t rrm = vap->rrm;
    wlan_node_t ni = rrm->ni;
    ieee80211_rrm_frmcnt_data_t *frm_data = NULL;
    struct ieee80211_frmcnt *frm_rpt = NULL;
    int i=0;
    ieee80211_rrm_node_stats_t *stats = (ieee80211_rrm_node_stats_t *)ni->ni_rrm_stats;

    RRM_FUNCTION_ENTER;
    
    if( mie->token != IEEE80211_MEASREQ_FRAME_TOKEN || mie->len < RRM_MIN_RSP_LEN ) /* minimum length should be three */
        return -EINVAL;

    /* element id  len  measurement token  measurement mode  measurement type  
        1 +         1      +1                    +1         +     1 */
    elen -= ((sizeof(struct ieee80211_measrsp_ie) - 3 /* elementid + length + rsp */)
            + (sizeof(struct ieee80211_frm_rsp) - 1));
    frm += ((sizeof(struct ieee80211_measrsp_ie) -1)
            + (sizeof(struct ieee80211_frm_rsp) - 1 ));

    while(elen  > sizeof(struct ieee80211_ie_header) ) {
        elmid = frm++;
        info_len = *frm++;
        
        if (info_len == 0) {
            frm++;    /* next IE */
            continue;
        }

        if (elen < info_len) {
            /* Incomplete/bad info element */
            return -EINVAL;
        }
        switch(*elmid) {
            case IEEE80211_SUBELEMID_FR_REPORT_RESERVED:
                break;
            case IEEE80211_SUBELEMID_FR_REPORT_COUNT:
                {
                    u_int8_t cnt = info_len/sizeof(struct ieee80211_frmcnt);
                    for (i = 0; i < cnt; i++) {
                        frm_rpt = (struct ieee80211_frmcnt *)frm;
                        frm_data = &(stats->frmcnt_data[i]);
                        OS_MEMCPY(&frm_data->ta[0], frm_rpt->ta, IEEE80211_ADDR_LEN);
                        OS_MEMCPY(&frm_data->bssid[0], frm_rpt->bssid, IEEE80211_ADDR_LEN);
                        frm_data->phytype = frm_rpt->phytype;
                        frm_data->arcpi = frm_rpt->arcpi;
                        frm_data->lrsni = frm_rpt->lrsni;
                        frm_data->lrcpi = frm_rpt->lrcpi;
                        frm_data->antid = frm_rpt->antid;
                        frm_data->frmcnt = frm_rpt->frmcnt;
                        frm += sizeof(struct ieee80211_frmcnt);
                    }
                }
                break;
            case IEEE80211_ELEMID_VENDOR:
                /* TBD */
                break;
            default:
                break;
        }
        elen -=info_len;
        frm +=info_len;
    }

    RRM_FUNCTION_EXIT;
    return EOK;
}

/**
 * @brief Traffic stream matrics  
 *
 * @param vap
 * @param frm
 * @param frm_len
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int ieee80211_rrm_tsm_report(wlan_if_t vap,u_int8_t *frm, u_int32_t frm_len)
{
    struct ieee80211_measrsp_ie *mie = (struct ieee80211_measrsp_ie *) frm;
    struct ieee80211_tsm_rsp *rsp =(struct ieee80211_tsm_rsp *)(&mie->rsp[0]);
    u_int8_t elen = mie->len,*elmid,info_len = 0;
    ieee80211_rrm_t rrm = vap->rrm;
    wlan_node_t ni = rrm->ni;
    ieee80211_rrm_node_stats_t *stats =(ieee80211_rrm_node_stats_t *)ni->ni_rrm_stats;
    ieee80211_rrm_tsm_data_t *tsm = &stats->tsm_data;

    RRM_FUNCTION_ENTER;

    if((mie->token != IEEE80211_MEASREQ_TSMREQ_TOKEN) || (mie->len < 3)) /* minimum length should be three */
        return -EINVAL;

    /* element id  len  measurement token  measurement mode  measurement type  
        1 +         1      +1                    +1         +     1 */
    elen -= ((sizeof(struct ieee80211_measrsp_ie) - 3 /* elementid + length + rsp */)
            + (sizeof(struct ieee80211_tsm_rsp) - 1));

    /* tsm response */ 
    frm += ( (sizeof(struct ieee80211_measrsp_ie) -1)
            + (sizeof(struct ieee80211_tsm_rsp) - 1));

    tsm->tid = rsp->tid;
    tsm->tx_cnt = rsp->tx_cnt;
    tsm->discnt = rsp->discnt;
    tsm->multirtycnt = rsp->multirtycnt;
    tsm->cfpoll = rsp->cfpoll;
    tsm->qdelay = rsp->qdelay;
    tsm->txdelay = rsp->txdelay;
    tsm->brange = rsp->brange;
    OS_MEMCPY(tsm->mac,rsp->mac,IEEE80211_ADDR_LEN);
    OS_MEMCPY(tsm->bin,rsp->bin,IEEE80211_ADDR_LEN);
    
    while(elen  > sizeof(struct ieee80211_ie_header)) {

        elmid = frm++;
        info_len = *frm++;
        
        if (info_len == 0) {
            frm++;    /* next IE */
            continue;
        }

        if (elen < info_len) {
            /* Incomplete/bad info element */
            return -EINVAL;
        }

        switch(*elmid)
        {
            case IEEE80211_ELEMID_VENDOR:
                /*handle vendor ie*/
                break;
            default:
                break;
        }
        elen -= info_len;
        frm += info_len;
    }
    RRM_FUNCTION_EXIT;
    return EOK;
}

/**
 * @brief station stats report  
 *
 * @param vap
 * @param frm
 * @param frm_len
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int ieee80211_rrm_stastats_report(wlan_if_t vap, u_int8_t *frm, u_int32_t frm_len)
{
    struct ieee80211_measrsp_ie *mie = (struct ieee80211_measrsp_ie *) frm;
    struct ieee80211_stastatsrsp *rsp =(struct ieee80211_stastatsrsp *)(&mie->rsp[0]);
    ieee80211_rrm_t rrm = vap->rrm;
    wlan_node_t ni = rrm->ni;
    ieee80211_rrm_node_stats_t *stats = ni->ni_rrm_stats;
    u_int8_t elen = mie->len,*elmid,info_len =0;

    RRM_FUNCTION_ENTER;
    if( mie->token != IEEE80211_MEASREQ_STASTATS_TOKEN  || mie->len < RRM_MIN_RSP_LEN ) /* minimum length should be three */
        return -EINVAL;

    /* element id  len  measurement token  measurement mode  measurement type  
        1 +         1      +1                    +1         +     1 */

    elen -=(sizeof(struct ieee80211_measrsp_ie) - 3 /* elementid + length + rsp */);
    frm +=(sizeof(struct ieee80211_measrsp_ie) -1);

    switch(rsp->gid)
    {
        case IEEE80211_STASTATS_GID0: {
                ieee80211_rrm_statsgid0_t *gid = &stats->gid0;
                OS_MEMCPY(gid,&rsp->stats.gid0,sizeof(ieee80211_rrm_statsgid0_t)); 
                set_frmcnt_window(rrm,rsp->stats.gid0.txfrmcnt);
                info_len = sizeof(ieee80211_rrm_statsgid0_t) - 1;
            }
            break;
        case IEEE80211_STASTATS_GID1: {
                ieee80211_rrm_statsgid1_t *gid = &stats->gid1;
                OS_MEMCPY(gid,&rsp->stats.gid1,sizeof(ieee80211_rrm_statsgid1_t)); 
                set_ackfail_window(rrm,rsp->stats.gid1.ackfail);
                info_len = sizeof(ieee80211_rrm_statsgid1_t) - 1;
            }
            break;
        case IEEE80211_STASTATS_GID2:
        case IEEE80211_STASTATS_GID3:
        case IEEE80211_STASTATS_GID4:
        case IEEE80211_STASTATS_GID5:
        case IEEE80211_STASTATS_GID6:
        case IEEE80211_STASTATS_GID7:
        case IEEE80211_STASTATS_GID8:
        case IEEE80211_STASTATS_GID9: {
                ieee80211_rrm_statsgidupx_t *gid = &stats->gidupx[rsp->gid - 2];
                OS_MEMCPY(gid,&rsp->stats.upstats,sizeof(ieee80211_rrm_statsgidupx_t)); 
                info_len = sizeof(ieee80211_rrm_statsgidupx_t) - 1;
            }
            break;
        case IEEE80211_STASTATS_GID10: {
                ieee80211_rrm_statsgid10_t *gid = &stats->gid10;
                OS_MEMCPY(gid,&rsp->stats.gid10,sizeof(ieee80211_rrm_statsgid10_t)); 
                set_stcnt_window(rrm,rsp->stats.gid10.st_cnt);
                set_be_window(rrm,rsp->stats.gid10.be_avg_delay);
                set_vo_window(rrm,rsp->stats.gid10.vo_avg_delay);
                info_len = sizeof(ieee80211_rrm_statsgid10_t) - 1;
            }
            break;
        default:
            break;
    }
    info_len += (sizeof(rsp->m_intvl) + sizeof(rsp->gid));
    frm += info_len;
    elen-= info_len;
    while(elen > sizeof(struct ieee80211_ie_header)) {
        elmid = frm++;
        info_len = *frm++;
        if (info_len == 0) {
            frm++;    /* next IE */
            continue;
        }
        if (elen < info_len) {
            /* Incomplete/bad info element */
            return -EINVAL;
        }
        switch(*elmid) {
            case IEEE80211_ELEMID_VENDOR:
                /*TBD*/
                break;
            default:
                break;
        }
        elen -= info_len;
        frm += info_len;
    }

    RRM_FUNCTION_EXIT;
    return EOK;
}

/**
 * @brief Noise histogram report parsing 
 *
 * @param vap
 * @param ni
 * @param rpt
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int ieee80211_rrm_nhist_report(wlan_if_t vap, u_int8_t *frm, u_int32_t frm_len) 
{ 

    struct ieee80211_measrsp_ie *mie= (struct ieee80211_measrsp_ie *) frm;
    struct ieee80211_nhistrsp  *nhist = (struct ieee80211_nhistrsp *) (&mie->rsp[0]);
    u_int8_t elen = mie->len,*elmid;
    ieee80211_rrm_noise_data_t *unhist = &vap->rrm->user_nhist_resp[nhist->chnum];
    
    RRM_FUNCTION_ENTER;

    if( mie->token != IEEE80211_MEASREQ_NHIST_TOKEN || elen < RRM_MIN_RSP_LEN ) /*minimum elen should be three */
        return -EINVAL;

    /* element id  len  measurement token  measurement mode  measurement type  
        1 +         1      +1                    +1         +     1 */
    elen -= ((sizeof(struct ieee80211_measrsp_ie) - 3 /* elementid + length + rsp */)
            + (sizeof(struct ieee80211_nhistrsp) - 1));

    /* Rclass  Channel  Mstart  mduration  antennaid ANPI  IPI
       1 +      +1      +8        2          +1       + 1  10 */
    frm += ((sizeof(struct ieee80211_measrsp_ie) -1)
            + (sizeof(struct ieee80211_nhistrsp) - 1));
    unhist->anpi = (int8_t)nhist->anpi;
    unhist->antid = nhist->antid;
#define IPI_SIZE 11    
    OS_MEMCPY(unhist->ipi,nhist->ipi,IPI_SIZE);
#undef IPI_SIZE    
    
    RRM_DEBUG(vap, RRM_DEBUG_INFO, 
              "%s : duration %d chnum %d regclass %d anpi %d antid %d  anpi %d\n ", __func__,  
               nhist->mduration, nhist->chnum, nhist->regclass, nhist->anpi, nhist->antid, (int8_t) nhist->anpi);

    while(elen  > sizeof(struct ieee80211_ie_header) )
    {
        u_int8_t info_len ;

        elmid = frm++;
        info_len = *frm++;
        
        if (info_len == 0) {
            frm++;    /* next IE */
            continue;
        }

        if (elen < info_len) {
            /* Incomplete/bad info element */
            return -EINVAL;
        }
        switch(*elmid)
        {
            case IEEE80211_ELEMID_VENDOR:
                /*handle vendor ie*/
                break;
            default:
                break;
        }
        elen-=info_len;
        frm +=info_len;
    }
    set_anpi_window(vap->rrm,unhist->anpi);
    RRM_FUNCTION_EXIT;
    return EOK;
}
/**
 * @brief i Location configuration request 
 *
 * @param lcirpt_data
 * @param lci_entry
 */

static void ieee80211_rrm_lci_report_parse(const u_int8_t *lcirpt_data,
                                           ieee80211_rrm_lci_data_t *lci_entry)
{
    u_int32_t param, bit;
    u_int32_t param_val;
    u_int32_t idx, bit_idx;
    u_int32_t bit_off, bit_len;
    struct rrm_lci_report_struct {
        u_int32_t bit_offset;
        u_int32_t bit_len;
    }lcirpt_fields[IEEE80211_RRM_LCI_LAST] =  {
        {0,   8}, /* IEEE80211_RRM_LCI_ID */
        {8,   8}, /* IEEE80211_RRM_LCI_LEN */
        {16,  6}, /* IEEE80211_RRM_LCI_LAT_RES */
        {22, 25}, /* IEEE80211_RRM_LCI_LAT_FRAC */
        {47,  9}, /* IEEE80211_RRM_LCI_LAT_INT */
        {56,  6}, /* IEEE80211_RRM_LCI_LONG_RES */
        {62, 25}, /* IEEE80211_RRM_LCI_LONG_FRAC */
        {87,  9}, /* IEEE80211_RRM_LCI_LONG_INT */  
        {96,  4}, /* IEEE80211_RRM_LCI_ALT_TYPE */
        {100, 6}, /* IEEE80211_RRM_LCI_ALT_RES */
        {106, 8}, /* IEEE80211_RRM_LCI_ALT_FRAC */
        {114,22}, /* IEEE80211_RRM_LCI_ALT_INT */
        {136, 8}, /* IEEE80211_RRM_LCI_DATUM */
    };
    for (param = IEEE80211_RRM_LCI_ID; param < IEEE80211_RRM_LCI_LAST; param++) 
    {
        param_val = 0;
        bit_off = lcirpt_fields[param].bit_offset;
        bit_len = lcirpt_fields[param].bit_len;
        for (bit = 0; bit < bit_len; bit++)
        {
            idx = ((bit_off )>>3); /*dividing by 8 */
            bit_idx = ((bit_off) % 8);
            if (lcirpt_data[idx] & (0x1 << bit_idx))
            {
                param_val |= (1 << bit);
            }
            else
            {
                param_val |= (0 << bit);
            }
            bit_off++;
        }
        switch(param)
        {
            case IEEE80211_RRM_LCI_ID:
                lci_entry->id = (u_int8_t)param_val;
                break;
            case IEEE80211_RRM_LCI_LEN:
                lci_entry->len = (u_int8_t)param_val;
                break;
            case IEEE80211_RRM_LCI_LAT_RES:
                lci_entry->lat_res = (u_int8_t)param_val;
                break;
            case IEEE80211_RRM_LCI_LAT_FRAC:
                lci_entry->lat_frac = (u_int32_t)param_val;
                break;
            case IEEE80211_RRM_LCI_LAT_INT:
                lci_entry->lat_integ = (u_int16_t)param_val;
                break;
            case IEEE80211_RRM_LCI_LONG_RES:
                lci_entry->long_res = (u_int8_t)param_val;
                break;
            case IEEE80211_RRM_LCI_LONG_FRAC:
                lci_entry->long_frac = (u_int32_t)param_val;
                break;
            case IEEE80211_RRM_LCI_LONG_INT:
                lci_entry->long_integ = (u_int16_t)param_val;
                break;
            case IEEE80211_RRM_LCI_ALT_TYPE:
                lci_entry->alt_type = (u_int8_t) param_val;
                break;
            case IEEE80211_RRM_LCI_ALT_RES:
                lci_entry->alt_res = (u_int8_t) param_val;
                break;
            case IEEE80211_RRM_LCI_ALT_FRAC:
                lci_entry->alt_frac = (u_int8_t) param_val;
                break;
            case IEEE80211_RRM_LCI_ALT_INT:
                lci_entry->alt_integ = (u_int32_t) param_val;
                break;
            case IEEE80211_RRM_LCI_DATUM:
                lci_entry->datum = (u_int8_t) param_val;
                break;
            default:
                break;
        }
    }
    return;
}

/**
 * @brief Parse location configuration report 
 *
 * @param vap
 * @param ni
 * @param rpt
 *
 * @return 
 * @return on success return 0.
 *         on failure returns -ve value.
 */

int ieee80211_rrm_lci_report(wlan_if_t vap, u_int8_t *frm, u_int32_t frm_len)
{
    struct ieee80211_measrsp_ie *mie= (struct ieee80211_measrsp_ie *) frm;
    struct ieee80211_lcirsp  *lcirpt = (struct ieee80211_lcirsp *) (&mie->rsp[0]);
    ieee80211_rrm_lci_data_t lci_entry;
    ieee80211_rrm_t rrm = vap->rrm;
    wlan_node_t ni = rrm->ni;
    u_int16_t azimuth_report;
    u_int8_t *lci_data = (&lcirpt->lci_data[0]);
    u_int8_t elen = mie->len,*elmid;
    ieee80211_rrm_node_stats_t *stats =(ieee80211_rrm_node_stats_t *) ni->ni_rrm_stats;

    RRM_FUNCTION_ENTER;

    if( mie->token != IEEE80211_MEASREQ_LCI_TOKEN || elen < RRM_MIN_RSP_LEN ) /*minimum elen should be three */
        return -EINVAL;

    /* element id  len  measurement token  measurement mode  measurement type  
        1 +         1      +1                    +1         +     1 */
    elen -= ((sizeof(struct ieee80211_measrsp_ie) - 3 /* elementid + length + rsp */)
            + (sizeof(struct ieee80211_lcirsp) - 1));
    frm += ((sizeof(struct ieee80211_measrsp_ie) -1)
            + (sizeof(struct ieee80211_lcirsp) - 1));

    OS_MEMSET(&lci_entry, 0x0, sizeof(lci_entry));
    ieee80211_rrm_lci_report_parse(lci_data, &lci_entry);

    while(elen  > sizeof(struct ieee80211_ie_header) )
    {
        u_int8_t info_len ;

        elmid = frm++;
        info_len = *frm++;
        
        if (info_len == 0) {
            frm++;    /* next IE */
            continue;
        }

        if (elen < info_len) {
            /* Incomplete/bad info element */
            return -EINVAL;
        }
        switch(*elmid)
        {
            case IEEE80211_SUBELEMID_LCI_AZIMUTH_REPORT:
                OS_MEMCPY(&azimuth_report, frm, 2);
                lci_entry.azi_type =  (azimuth_report >> 2) & 0x1;
                lci_entry.azi_res = (azimuth_report >> 3) & 0xf;
                lci_entry.azimuth = (azimuth_report >> 7) & 0x1ff;
            case IEEE80211_SUBELEMID_LCI_VENDOR:
            case IEEE80211_SUBELEMID_LCI_RESERVED:
                /*handle vendor ie*/
                break;
            default:
                break;
        }
        elen-=info_len;
        frm +=info_len;
    }
    if (ni->ni_rrmlci_loc == 0)
        OS_MEMCPY(&stats->ni_vap_lciinfo, &lci_entry, sizeof(lci_entry));
    else if (ni->ni_rrmlci_loc == 1)
        OS_MEMCPY(&stats->ni_rrm_lciinfo, &lci_entry, sizeof(lci_entry));

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_RRM,
            " \t%2x %2x %2x %2x %2x %2x\t %d \t %d  %d  %d  %d \n",
            lci_entry.id, lci_entry.len,
            lci_entry.lat_res,lci_entry.lat_frac,
            lci_entry.lat_integ,lci_entry.long_res,
            lci_entry.long_frac,lci_entry.long_integ,
            lci_entry.alt_res, lci_entry.alt_integ, lci_entry.alt_frac);

    RRM_FUNCTION_EXIT;
    return EOK;
} 


/**
 * @brief Routine to flush beacon table , will be called every time beacon report come  
 *
 * @param vap
 */

void ieee80211_flush_beacontable(wlan_if_t vap)
{
    struct ieee80211_beacon_report_table *btable = vap->rrm->beacon_table; 
    RRM_FUNCTION_ENTER ; 

    RRM_BTABLE_LOCK(btable);
    {
        struct ieee80211_beacon_entry   *current_beacon_entry,*beacon;
        struct ieee80211_beacon_report *report;
        /* limit the scope of this variable */ 
        
        TAILQ_FOREACH_SAFE(current_beacon_entry ,&(btable->entry),blist,beacon) 
        {
            report = &(current_beacon_entry->report);
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_RRM,
                    "\t%2x %2x %2x %2x %2x %2x\t %d \t %d \n",
                    report->bssid[0],report->bssid[1],
                    report->bssid[2],report->bssid[3],
                    report->bssid[4],report->bssid[5],
                    report->ch_num,report->rcpi);
            TAILQ_REMOVE(&(btable->entry), current_beacon_entry, blist);
            OS_FREE(current_beacon_entry);
        }
    }
    RRM_BTABLE_UNLOCK(btable);
    RRM_FUNCTION_EXIT;
    return;
}

/**
 * @brief Routine to add beacon entry in list 
 *
 * @param vap
 * @param bcnrpt
 */

void add_beacon_entry(wlan_if_t vap ,struct ieee80211_beacon_report *bcnrpt)
{
    struct ieee80211_beacon_report_table *btable = vap->rrm->beacon_table; 
    uint8_t flag =0;

    RRM_BTABLE_LOCK(btable);
    {
        struct ieee80211_beacon_entry   *current_beacon_entry;
        struct ieee80211_beacon_report *report;
        /* limit the scope of this variable */ 
        TAILQ_FOREACH(current_beacon_entry ,&(btable->entry),blist) 
        {
            report = &current_beacon_entry->report;
            if(IEEE80211_ADDR_EQ(report->bssid,bcnrpt->bssid)) {
                flag = 1;
                break;
            }
        }
        if(!flag) { /* Entry not found */  
            current_beacon_entry  = (struct ieee80211_beacon_entry *)
                OS_MALLOC(vap->rrm->rrm_osdev, sizeof(struct ieee80211_beacon_entry), 0);
            OS_MEMZERO(current_beacon_entry, sizeof(struct ieee80211_beacon_entry));
            report = &current_beacon_entry->report;
            TAILQ_INSERT_TAIL(&(btable->entry), current_beacon_entry, blist);
        }
        report->reg_class = bcnrpt->reg_class;           
        report->ch_num = bcnrpt->ch_num;
        report->frame_info = bcnrpt->frame_info ;              
        report->rcpi= bcnrpt->rcpi;   
        report->rsni = bcnrpt->rsni;                                   
        IEEE80211_ADDR_COPY(report->bssid,bcnrpt->bssid);
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_RRM,
                " \t%2x %2x %2x %2x %2x %2x\t %d \t %d \n",
                report->bssid[0],report->bssid[1],
                report->bssid[2],report->bssid[3], 
                report->bssid[4],report->bssid[5],
                report->ch_num,report->rcpi);
    }
    RRM_BTABLE_UNLOCK(btable);
}

/**
 * @brief Parse beacon measurement report  
 *
 * @param vap
 * @param ni
 * @param rsp
 * @param frm_len
 *
 * @return 
 */

int ieee80211_rrm_beacon_measurement_report(wlan_if_t vap,
        u_int8_t *sfrm,u_int32_t frm_len)
{
    struct ieee80211_beacon_report  *bcnrpt ; 
    struct ieee80211_measrsp_ie *mie ;
    u_int8_t *efrm = sfrm + frm_len;

    RRM_FUNCTION_ENTER;

    ieee80211_flush_beacontable(vap); 

    while (sfrm < efrm ) 
    {
        mie= (struct ieee80211_measrsp_ie *) sfrm;

        if ( mie->len == 0) {
            sfrm++; /*id */
            sfrm++; /*len */
            continue;
        }
        if (mie->len < sizeof(struct ieee80211_ie_header) ||
                sfrm + sizeof(struct ieee80211_ie_header) + mie->len > efrm) {
            break;
        }
        switch(mie->id)
        {
            case IEEE80211_ELEMID_MEASREP:
            {
                u_int8_t min_ie_len = (sizeof(struct ieee80211_measrsp_ie) - 1) +
                                (sizeof (struct ieee80211_beacon_report) - 1);

                /* we currently use the same token id for all requests
                 * instead of an incrementing token id to match 
                 * responses with corresponding requests
                 */
                if (mie->len >= min_ie_len &&
                            mie->token == IEEE80211_MEASREQ_BR_TOKEN) {
                    bcnrpt = (struct ieee80211_beacon_report *)(sfrm + 
                                    sizeof(struct ieee80211_measrsp_ie) - 1);
                    /* search for this bssid in all available list */
                    add_beacon_entry(vap, bcnrpt);
                }
                sfrm += mie->len + sizeof(struct ieee80211_ie_header);
                break;    
            }
            case IEEE80211_SUBELEMID_BREPORT_FRAME_BODY:
                /*handle Frame body here */
                /* XXX: follow-thru to default case */
            case IEEE80211_SUBELEMID_BREPORT_RESERVED:
                /*Reserved   */
                /* XXX: follow-thru to default case */
            default:
                sfrm += mie->len + sizeof(struct ieee80211_ie_header);
                break;
        }
    }
    RRM_FUNCTION_EXIT;

    return EOK;
}     

#endif /* UMAC_SUPPORT_RRM */
