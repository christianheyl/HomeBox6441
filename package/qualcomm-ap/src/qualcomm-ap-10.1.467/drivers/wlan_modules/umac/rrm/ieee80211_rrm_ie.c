/*
 *  Copyright (c) 2009 Atheros Communications Inc.  All rights reserved.
 *  Radio Resource measurements IE parsing/processing routines.
 */

#include <ieee80211_var.h>

#if UMAC_SUPPORT_RRM

#include <ieee80211_rrm.h>
#include "ieee80211_rrm_priv.h"

/**
 * @brief 
 *
 * @param frm
 * @param ssid
 * @param len
 *
 * @return 
 */
static u_int8_t *ieee80211_add_beaconreq_ssid(u_int8_t *frm, u_int8_t* ssid, u_int len)
{
    *frm++ = IEEE80211_SUBELEMID_BR_SSID;
    *frm++ = len;
    OS_MEMCPY(frm, ssid, len);
    return frm + len;
}
/**
 * @brief 
 *
 * @param frm
 * @param binfo
 *
 * @return 
 */
static u_int8_t *ieee80211_add_beaconreq_rinfo(u_int8_t *frm, ieee80211_rrm_beaconreq_info_t *binfo)
{
    struct ieee80211_beaconreq_rinfo* rinfo = 
                              (struct ieee80211_beaconreq_rinfo *)(frm);
    int rinfo_len = sizeof(struct ieee80211_beaconreq_rinfo);
    rinfo->id = IEEE80211_SUBELEMID_BR_RINFO;
    rinfo->len = rinfo_len - 2;
    rinfo->cond = binfo->rep_cond;
    rinfo->refval = binfo->rep_thresh;
    return (frm + rinfo_len);

}

/**
 * @brief 
 *
 * @param frm
 * @param binfo
 *
 * @return 
 */
static u_int8_t *ieee80211_add_beaconreq_rdetail(u_int8_t *frm,
                       ieee80211_rrm_beaconreq_info_t *binfo)
{
    struct ieee80211_beaconrep_rdetail* rdetail = 
                              (struct ieee80211_beaconrep_rdetail *)(frm);
    int rdetail_len = sizeof(struct ieee80211_beaconrep_rdetail);
    rdetail->id = IEEE80211_SUBELEMID_BR_RDETAIL;
    rdetail->len = rdetail_len - 2;
    rdetail->level = binfo->rep_detail;
    return (frm + rdetail_len);
}

/**
 * @brief 
 *
 * @param frm
 *
 * @return 
 */
static u_int8_t *ieee80211_add_beaconreq_reqie(u_int8_t *frm)
{
    *frm++ = IEEE80211_SUBELEMID_BR_IEREQ;
    *frm++ = 5;
    *frm++ = IEEE80211_ELEMID_SSID;
    *frm++ = IEEE80211_ELEMID_RSN;
    *frm++ = IEEE80211_ELEMID_MOBILITY_DOMAIN;
    *frm++ = IEEE80211_ELEMID_RRM;
    *frm++ = IEEE80211_ELEMID_VENDOR;
    return frm;
}

/**
 * @brief 
 *
 * @param frm
 * @param chaninfo
 *
 * @return 
 */
static u_int8_t *ieee80211_add_beaconreq_chanrep(u_int8_t *frm,
                struct ieee80211_beaconreq_chaninfo *chaninfo)
{
    int i;
    *frm++ = IEEE80211_SUBELEMID_BR_CHANREP; 
    *frm++ = chaninfo->numchans + 1 /* for reg class */;
    *frm++ = chaninfo->regclass;
    for (i = 0; i < chaninfo->numchans; i++) {
        *frm++ = chaninfo->channum[i];
    }
    return frm;
}

/**
 * @brief 
 *
 * @param frm
 * @param tsminfo
 *
 * @return 
 */
static u_int8_t* ieee80211_add_tsmreq_trigrep(u_int8_t *frm,
                   ieee80211_rrm_tsmreq_info_t* tsminfo)
{
    struct ieee80211_tsmreq_trigrep* trigrep = 
                              (struct ieee80211_tsmreq_trigrep *)(frm);
    int trigrep_len = sizeof(struct ieee80211_tsmreq_trigrep);
    trigrep->id = IEEE80211_SUBELEMID_TSMREQ_TRIGREP;
    trigrep->len = trigrep_len - 2;
    trigrep->tc_avg = (tsminfo->trig_cond & IEEE80211_TSMREQ_TRIGREP_AVG) ? 1 : 0;
    trigrep->tc_cons = (tsminfo->trig_cond & IEEE80211_TSMREQ_TRIGREP_CONS) ? 1 : 0;
    trigrep->tc_delay = (tsminfo->trig_cond & IEEE80211_TSMREQ_TRIGREP_DELAY) ? 1 : 0;
    trigrep->avg_err_thresh = tsminfo->avg_err_thresh;
    trigrep->cons_err_thresh = tsminfo->cons_err_thresh;
    trigrep->delay_thresh = tsminfo->delay_thresh;
    trigrep->meas_count = tsminfo->meas_count;
    trigrep->trig_timeout = tsminfo->trig_timeout;
    return (frm + trigrep_len);
}

/* Internal Functions */

/*
 * Add measurement request beacon IE
 */

/**
 * @brief 
 *
 * @param frm
 * @param ni
 * @param binfo
 *
 * @return 
 */
u_int8_t *ieee80211_add_measreq_beacon_ie(u_int8_t *frm, struct ieee80211_node *ni, 
                       ieee80211_rrm_beaconreq_info_t *binfo)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_measreq_ie *measreq = (struct ieee80211_measreq_ie *)frm;
    struct ieee80211_beaconreq* beaconreq;
    int i;
    OS_MEMZERO(measreq, sizeof(struct ieee80211_measreq_ie));

    measreq->id = IEEE80211_ELEMID_MEASREQ;
    measreq->token = IEEE80211_MEASREQ_BR_TOKEN;
    measreq->reqmode = binfo->reqmode;
    measreq->reqtype = IEEE80211_MEASREQ_BR_TYPE;
    beaconreq = (struct ieee80211_beaconreq *)(&measreq->req[0]);
    beaconreq->regclass = binfo->regclass;
    beaconreq->channum = binfo->channum;
    beaconreq->random_ivl = htole16(binfo->random_ivl);
    beaconreq->duration = htole16(binfo->duration);
    beaconreq->mode = binfo->mode;
    IEEE80211_ADDR_COPY(beaconreq->bssid, binfo->bssid);

    frm = (u_int8_t *)(&beaconreq->subelm[0]);
    if (binfo->req_ssid == 1) {
        frm = ieee80211_add_beaconreq_ssid(frm, vap->iv_bss->ni_essid,
                      vap->iv_bss->ni_esslen);
    }
    else if (binfo->req_ssid == 2) {
        /* wildcard ssid */
        frm = ieee80211_add_beaconreq_ssid(frm, frm, 0);
    }
    frm = ieee80211_add_beaconreq_rinfo(frm, binfo);

    frm = ieee80211_add_beaconreq_rdetail(frm, binfo);

    if (binfo->req_ie) {
        frm = ieee80211_add_beaconreq_reqie(frm);
    }

    for (i = 0; i < binfo->num_chanrep; i++) {
        frm = ieee80211_add_beaconreq_chanrep(frm, &binfo->apchanrep[i]);
    }

    measreq->len = (frm - &(measreq->token));
    return frm;
}

/*
 * Add measurement request tsm IE */

 /**
 * @brief 
 *
 * @param frm
 * @param tsminfo
 *
 * @return 
 */
u_int8_t *ieee80211_add_measreq_tsm_ie(u_int8_t *frm, ieee80211_rrm_tsmreq_info_t* tsminfo)
{
    struct ieee80211_measreq_ie *measreq = (struct ieee80211_measreq_ie *)frm;
    struct ieee80211_tsmreq* tsmreq;
    OS_MEMZERO(measreq, sizeof(struct ieee80211_measreq_ie));
    measreq->id = IEEE80211_ELEMID_MEASREQ;
    measreq->token = IEEE80211_MEASREQ_TSMREQ_TOKEN;
    measreq->reqmode = tsminfo->reqmode;
    measreq->reqtype = IEEE80211_MEASREQ_TSMREQ_TYPE;
    tsmreq = (struct ieee80211_tsmreq *)(&measreq->req[0]);
    tsmreq->rand_ivl = htole16(tsminfo->rand_ivl);
    tsmreq->meas_dur = htole16(tsminfo->meas_dur);
    IEEE80211_ADDR_COPY(tsmreq->macaddr, tsminfo->macaddr);
    tsmreq->tid = tsminfo->tid;
    tsmreq->bin0_range = tsminfo->bin0_range;
    frm = (u_int8_t *)(&tsmreq->subelm[0]);
    if (tsminfo->trig_cond) {
        frm = ieee80211_add_tsmreq_trigrep(frm, tsminfo);
    }
    measreq->len = (frm - &(measreq->token));
    return frm;
}

/*
 * Add Neigbor Report IE
 */

/**
 * @brief 
 *
 * @param frm
 * @param nr_info
 *
 * @return 
 */
u_int8_t *ieee80211_add_nr_ie(u_int8_t *frm, struct ieee80211_nrresp_info* nr_info)
{
    struct ieee80211_nr_ie *nr = (struct ieee80211_nr_ie *)frm;
    OS_MEMZERO(nr, sizeof(struct ieee80211_nr_ie));
    nr->id = IEEE80211_ELEMID_NEIGHBOR_REPORT;
    IEEE80211_ADDR_COPY(nr->bssid, nr_info->bssid);
    nr->regclass = nr_info->regclass;
    nr->channum = nr_info->channum;
    nr->phytype = nr_info->phytype;

    /* TBD - Need to check actual RSN cap of the node */
    nr->bsinfo0_security = 1;

    if (nr_info->capinfo & IEEE80211_CAPINFO_SPECTRUM_MGMT) {
        nr->bsinfo0_specmgmt = 1;
    }

    /* TBD: -  This needs to be changed based WMM capabilities */
    nr->bsinfo0_qos = 1;
    nr->bsinfo0_apsd = 1;

    if (nr_info->capinfo & IEEE80211_CAPINFO_RADIOMEAS) {
        nr->bsinfo0_rrm = 1;
    }

    /* TBD - Should be based RSSI strength */
    nr->bsinfo0_ap_reach = 3;

    frm = (u_int8_t *)(&nr->subelm[0]);
    /* Add sub elements */
    nr->len = (frm - &nr->bssid[0]);
    return frm;
}

/* External Functions */

/*
 * Add RRM capability IE 
 */

/**
 * @brief 
 *
 * @param frm
 * @param ni
 *
 * @return 
 */
u_int8_t *ieee80211_add_rrm_cap_ie(u_int8_t *frm, struct ieee80211_node *ni)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_rrm_cap_ie *rrmcap = (struct ieee80211_rrm_cap_ie *)frm;
    int rrmcap_len = sizeof(struct ieee80211_rrm_cap_ie);
    if (ieee80211_vap_rrm_is_set(vap)) {
        OS_MEMZERO(rrmcap, sizeof(struct ieee80211_rrm_cap_ie));
        rrmcap->id = IEEE80211_ELEMID_RRM;
        rrmcap->len = rrmcap_len - 2;
        rrmcap->lnk_meas = 1;
        rrmcap->neig_rpt = 1;
        rrmcap->bcn_passive = 1;
        rrmcap->bcn_active = 1;
        rrmcap->bcn_table = 1;
        rrmcap->tsm_meas = 1;
        rrmcap->trig_tsm_meas = 1;
        return (frm + rrmcap_len);
    }
    else {
        return frm;
    }
}

/**
 * @brief 
 *
 * @param frm
 * @param type
 * @param token
 *
 * @return 
 */
static u_int8_t *ieee80211_add_rrm_ie(u_int8_t *frm,u_int8_t type,u_int8_t token)
{ 

    struct ieee80211_measreq_ie *measreq = (struct ieee80211_measreq_ie *)frm;
    measreq->id = IEEE80211_ELEMID_MEASREQ;
    measreq->token = token;
    measreq->reqmode = BIT_ENABLE | BIT_DUR; 
    measreq->reqtype = type;
    frm =(u_int8_t *)(&measreq->reqtype);
    frm++;/* Moving to next byte */
    return frm;
}

/**
 * @brief 
 *
 * @param frm
 * @param ni
 * @param params
 *
 * @return 
 */
u_int8_t *ieee80211_add_measreq_stastats_ie(u_int8_t *frm, struct ieee80211_node *ni, 
                       ieee80211_rrm_stastats_info_t *params)
{
    struct ieee80211_measreq_ie *measreq = (struct ieee80211_measreq_ie *)frm;
    struct ieee80211_stastatsreq *statsreq;

	frm = ieee80211_add_rrm_ie(frm,IEEE80211_MEASREQ_STA_STATS_REQ,IEEE80211_MEASREQ_STASTATS_TOKEN);

    statsreq = (struct ieee80211_stastatsreq *)(&measreq->req[0]);
    IEEE80211_ADDR_COPY(statsreq->dstmac, params->dstmac);
    statsreq->rintvl = htole16(params->r_invl);
    statsreq->mduration = htole16(params->m_dur);
    statsreq->gid = params->gid;
    frm = (u_int8_t *)(&statsreq->req[0]);
    measreq->len = (frm - &(measreq->token));
    return frm;
}


/**
 * @brief 
 *
 * @param frm
 * @param nhist
 *
 * @return 
 */
u_int8_t *ieee80211_add_nhist_opt_ie(u_int8_t *frm,ieee80211_rrm_nhist_info_t *nhist)
{
    *frm++ = IEEE80211_SUBELEMID_NHIST_CONDITION;
    *frm++ = nhist->cond;
    *frm++ = nhist->c_val;
    return frm;
}

/**
 * @brief 
 *
 * @param frm
 * @param action
 * @param n_rpt
 *
 * @return 
 */
u_int8_t *ieee80211_add_rrm_action_ie(u_int8_t *frm,u_int8_t action, u_int16_t n_rpt)
{
    struct ieee80211_action_rm_req *req = (struct ieee80211_action_rm_req*)(frm);
    req->header.ia_category = IEEE80211_ACTION_CAT_RM;
    req->header.ia_action = action;
    req->dialogtoken = IEEE80211_ACTION_RM_TOKEN;
    req->num_rpts = htole16(n_rpt);
    frm += (sizeof(struct ieee80211_action_rm_req) - 1);
    return frm;
}

/**
 * @brief 
 *
 * @param frm
 * @param ni
 * @param fr_info
 *
 * @return 
 */
u_int8_t *ieee80211_add_measreq_frame_req_ie(u_int8_t *frm, struct ieee80211_node *ni, 
                       ieee80211_rrm_frame_req_info_t  *fr_info)
{ 

    struct ieee80211_measreq_ie *measreq = (struct ieee80211_measreq_ie *)frm;
	struct ieee80211_frame_req *frame_req;
	frm = ieee80211_add_rrm_ie(frm,IEEE80211_MEASREQ_FRAME_REQ,IEEE80211_MEASREQ_FRAME_TOKEN);
	frame_req = (struct ieee80211_frame_req *)frm;
	frame_req->regclass = fr_info->regclass;
	frame_req->chnum = fr_info->chnum;
	frame_req->rintvl = htole16(fr_info->r_invl);
	frame_req->mduration = htole16(fr_info->m_dur);
	frame_req->ftype = fr_info->ftype;
	frm = (u_int8_t *)(frame_req->req);
	measreq->len = (frm - &(measreq->token));
	return frm;
}
u_int8_t *ieee80211_add_lcireq_opt_ie(u_int8_t *frm, ieee80211_rrm_lcireq_info_t *lcireq_info)
{
    *frm++ = IEEE80211_SUBELEMID_LC_AZIMUTH_CONDITION;
    *frm++ = 1; /* it is extensible will change it later */
    *frm++ = (lcireq_info->azi_res & 0x0f) | (lcireq_info->azi_type >> 4);
    
    return frm;
}

u_int8_t *ieee80211_add_measreq_lci_ie(u_int8_t *frm, struct ieee80211_node *ni, 
                       ieee80211_rrm_lcireq_info_t *lcireq_info)
{
    struct ieee80211_measreq_ie *measreq = (struct ieee80211_measreq_ie *)frm;
    struct ieee80211_lcireq *lcireq;

    frm = ieee80211_add_rrm_ie(frm,IEEE80211_MEASREQ_LCI_REQ,IEEE80211_MEASREQ_LCI_TOKEN);

    lcireq = (struct ieee80211_lcireq *)frm;
    lcireq->location = lcireq_info->location;
    lcireq->lat_res = lcireq_info->lat_res;
    lcireq->long_res =lcireq_info->long_res;
    lcireq->alt_res = lcireq_info->alt_res;
    frm = (u_int8_t *)(&lcireq->req[0]);
    
    if(lcireq_info->azi_res)
        frm = ieee80211_add_lcireq_opt_ie(frm,lcireq_info);

    measreq->len = (frm - &(measreq->token));
    return frm;
}

/**
 * @brief 
 *
 * @param frm
 * @param ni
 * @param nhist_info
 *
 * @return 
 */
u_int8_t *ieee80211_add_measreq_nhist_ie(u_int8_t *frm, struct ieee80211_node *ni, 
                       ieee80211_rrm_nhist_info_t *nhist_info)
{
    struct ieee80211_measreq_ie *measreq = (struct ieee80211_measreq_ie *)frm;

    struct ieee80211_nhistreq *nhistreq;

    frm = ieee80211_add_rrm_ie(frm,IEEE80211_MEASREQ_NOISE_HISTOGRAM_REQ,IEEE80211_MEASREQ_NHIST_TOKEN);

	nhistreq = (struct ieee80211_nhistreq *)frm;
	nhistreq->regclass = nhist_info->regclass;
	nhistreq->chnum = nhist_info->chnum;
	nhistreq->rintvl = htole16(nhist_info->r_invl);
	nhistreq->mduration = htole16(nhist_info->m_dur);

    frm = (u_int8_t *)(&nhistreq->req[0]);

    if(nhist_info->cond)
        frm = ieee80211_add_nhist_opt_ie(frm,nhist_info);

    measreq->len = (frm - &(measreq->token));

    return frm;
}

/**
 * @brief 
 *
 * @param frm
 * @param chinfo 
 *
 * @return 
 */
u_int8_t *ieee80211_add_chload_opt_ie(u_int8_t *frm,ieee80211_rrm_chloadreq_info_t *chinfo)
{
    *frm++ = IEEE80211_SUBELEMID_CHLOAD_CONDITION;
    *frm++ = chinfo->cond;
    *frm++ = chinfo->c_val;
    return frm;
}

/**
 * @brief 
 *
 * @param frm
 * @param ni
 * @param chinfo
 *
 * @return 
 */
u_int8_t *ieee80211_add_measreq_chload_ie(u_int8_t *frm, struct ieee80211_node *ni, 
                       ieee80211_rrm_chloadreq_info_t *chinfo)
{
    struct ieee80211_measreq_ie *measreq = (struct ieee80211_measreq_ie *)frm;

    struct ieee80211_chloadreq * chloadreq;

    frm = ieee80211_add_rrm_ie(frm,IEEE80211_MEASREQ_CHANNEL_LOAD_REQ,IEEE80211_MEASREQ_CHLOAD_TOKEN);

    chloadreq = (struct ieee80211_chloadreq *)(frm);

    chloadreq->regclass = chinfo->regclass;

    chloadreq->chnum = chinfo->chnum;

    chloadreq->rintvl = htole16(chinfo->r_invl);

    chloadreq->mduration = htole16(chinfo->m_dur);

    frm = (u_int8_t *)(&chloadreq->req[0]);

    if(chinfo->cond)
        frm = ieee80211_add_chload_opt_ie(frm,chinfo);

    measreq->len = (frm - &(measreq->token));

    return frm;
}
#endif /* UMAC_SUPPORT_RRM */
