/*
 * Copyright (c) 2010, Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */

#include "ieee80211_mlme_priv.h"    /* Private to MLME module */
#include <ieee80211_smartantenna.h>
#include <ieee80211_target.h>
#include <ieee80211_smartantenna.h>

#if UMAC_SUPPORT_AP || UMAC_SUPPORT_BTAMP
void ieee80211_mlme_recv_assoc_request(struct ieee80211_node *ni,
                                       u_int8_t reassoc,u_int8_t *vendor_ie, wbuf_t wbuf)
{
    struct ieee80211vap           *vap = ni->ni_vap;
    struct ieee80211com           *ic = ni->ni_ic;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    u_int8_t                      newassoc = (ni->ni_associd == 0);
    wbuf_t                        resp_wbuf;
    u_int16_t                     assocstatus;
    ieee80211_mlme_event          event;
    u_int8_t                      flag=0;
    u_int8_t                      node_leave = 0,isvht;

    /* AP  must be up and running */
    if (!mlme_priv->im_connection_up || ieee80211_vap_ready_is_clear(vap)) {
        return;
    }
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    if (ieee80211_node_join(ni)) {
        /* Association Failure */
        assocstatus = IEEE80211_REASON_ASSOC_TOOMANY;
    } else {
        assocstatus = IEEE80211_STATUS_SUCCESS;
        if (!reassoc) {
            ieee80211_admctl_node_leave(vap, ni);
        }

        /* Indicate that a new node has associated */
        event.type = IEEE80211_MLME_EVENT_STA_JOIN;
        event.u.event_sta.sta_count= vap->iv_sta_assoc;
        event.u.event_sta.sta_ps_count= vap->iv_ps_sta;
        event.u.event_sta.ni = ni;
        ieee80211_mlme_deliver_event(mlme_priv,&event);
    }

    /* Clear any previously cached status */
#ifdef ATHR_RNWF
    if(vap->iv_opmode==IEEE80211_M_IBSS && ic->ic_softap_enable){
        ni->ni_assocstatus = IEEE80211_STATUS_SUCCESS;
    }
    else{
        ni->ni_assocstatus = IEEE80211_STATUS_UNSPECIFIED;
    }
#else
    ni->ni_assocstatus = assocstatus;
#endif

#if ATH_SUPPORT_AOW

    /* 
     * When AOW is enabled, association is allowed 
     * only if the AOW ID matches
     */

    if (IEEE80211_ENAB_AOW(ic) && IS_AOW_ASSOC_SET(ic)) {
        if (!IS_NI_AOW_CAPABLE(ni)) {
            ni->ni_assocstatus = IEEE80211_STATUS_UNSPECIFIED;
        } else {
            aow_ie_t *ie = (aow_ie_t*)ni->ni_aow_ie;

            /* Check the SAIR compatibility */
            if (ic->ic_aow.ie.id == ie->id) {
                ni->ni_assocstatus = IEEE80211_STATUS_SUCCESS;
            } else {
                ni->ni_assocstatus = IEEE80211_STATUS_UNSPECIFIED;
            }
        }            
    }

    if ((assocstatus == IEEE80211_STATUS_SUCCESS))
        ieee80211_aow_join_indicate(ic, AOW_STA_CONNECTED, ni);

#endif  /* ATH_SUPPORT_AOW */

    /* Setup association response frame before indication */
    resp_wbuf = ieee80211_setup_assocresp(ni, NULL, reassoc, assocstatus, NULL);
    if (!resp_wbuf)
        assocstatus = IEEE80211_REASON_UNSPECIFIED;

    /* Move this down after sending the Assoc resp, so that the EAPOL
     * frame that is sent as consequence of this event, doesn't go OTA
     * before the Assoc Resp frame on some partial offload platforms. */

    /* Windows Platform have to call indication first to update ni->assocstatus 
    * value, Keep original alg for Windows Platform.
    */ 
#if ATHR_RNWF
    if (reassoc) {
        IEEE80211_DELIVER_EVENT_MLME_REASSOC_INDICATION(vap, ni->ni_macaddr,
                                                      assocstatus, wbuf, resp_wbuf);
    }
    if (!reassoc) {
        IEEE80211_DELIVER_EVENT_MLME_ASSOC_INDICATION(vap, ni->ni_macaddr,
                                                    assocstatus, wbuf, resp_wbuf);
    }
#endif
    /* Memory allocation failure, no point continuing */
    if (!resp_wbuf)
        return;

    /* Association rejection from above */
    if (ni->ni_assocstatus != IEEE80211_STATUS_SUCCESS) {

        /* Update already formed association response and send it out */
        ieee80211_setup_assocresp(ni, resp_wbuf, reassoc, ni->ni_assocstatus, NULL);
        ieee80211_send_mgmt(vap,ni, resp_wbuf,false);

        /* Flag to remove the node from node table */
        node_leave = 1; 
    } else {
        /* Wait for application to trigger mlme response for assoc */
        if (ieee80211_vap_trigger_mlme_resp_is_set(vap)) {
            wbuf_complete(resp_wbuf);
        }
        else {
            flag = TRUE;
        }

        ni->ni_assocuptime = OS_GET_TICKS();
#if UMAC_SUPPORT_WNM
        ni->ni_wnm->last_rcvpkt_tstamp =  ni->ni_assocuptime;
#endif

        isvht = (ni->ni_flags & IEEE80211_NODE_VHT);
        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC | IEEE80211_MSG_DEBUG, ni,
            "station %sassociated at aid %d: %s preamble, %s slot time"
            "%s%s%s%s cap 0x%x\n"
            , newassoc ? "" : "re"
            , IEEE80211_NODE_AID(ni)
            , ic->ic_flags & IEEE80211_F_SHPREAMBLE ? "short" : "long"
            , ic->ic_flags & IEEE80211_F_SHSLOT ? "short" : "long"
            , ic->ic_flags & IEEE80211_F_USEPROT ? ", protection" : ""
            , ni->ni_flags & IEEE80211_NODE_QOS ? ", QoS" : ""
            , isvht ? "VHT" : (ni->ni_flags & IEEE80211_NODE_HT ? ", HT" : "")
            , (ni->ni_flags & IEEE80211_NODE_HT)  ? 
                       (ni->ni_htcap & IEEE80211_HTCAP_C_CHWIDTH40 ? "40" : "20") : ""
            , ni->ni_capinfo
        );
        /* give driver a chance to setup state like ni_txrate */
        if (ic->ic_newassoc != NULL)
            ic->ic_newassoc(ni, newassoc);

        if(flag) {
            ieee80211_send_mgmt(vap,ni,resp_wbuf,false);
        }
        /*
         * When 802.1x is not in use mark the port
         * authorized at this point so traffic can flow.
         */
        if ((ni->ni_authmode != IEEE80211_AUTH_8021X) &&
             (!RSN_AUTH_IS_SHARED_KEY(&vap->iv_rsn) || (ni->ni_authmode != IEEE80211_AUTH_SHARED)))
            ieee80211_node_authorize(ni);

        /* Update MIMO powersave flags and node rates */
        ieee80211_update_noderates(ni);
        if((ni->ni_capinfo & IEEE80211_CAPINFO_RADIOMEAS)
                && ieee80211_vap_rrm_is_set(vap))
        {
            ieee80211_set_node_rrm(ni,TRUE);
        } 
        else {
            ieee80211_set_node_rrm(ni,FALSE);
        } 
#if UMAC_SUPPORT_SMARTANTENNA
        if (!IEEE80211_VAP_IS_PRIVACY_ENABLED(vap) && !(ni->ni_capinfo & IEEE80211_CAPINFO_PRIVACY)) {
            ieee80211_smartantenna_node_init(ni);
        }
#endif                
            /* need to add a station join notification */ 
    }
    
    /* Now send the notification and remove the node if needed */
#ifndef ATHR_RNWF
    if (reassoc) {                                                                 
	IEEE80211_DELIVER_EVENT_MLME_REASSOC_INDICATION(vap, ni->ni_macaddr,       
							assocstatus, wbuf, resp_wbuf);
    } else {                                                                       
	IEEE80211_DELIVER_EVENT_MLME_ASSOC_INDICATION(vap, ni->ni_macaddr,
						      assocstatus, wbuf, resp_wbuf);
    }
#endif

    if (node_leave) {                                                              
	IEEE80211_NODE_LEAVE(ni);                                                  
    }                                                                              
}

/* 
 *  create a insfra structure network (Host AP mode).  
 */

static void ieee80211_mlme_create_infra_continue(struct ieee80211vap *vap)
{
    /* Update channel and rates of the node */
    ieee80211_node_set_chan(vap->iv_bss);
    vap->iv_cur_mode = ieee80211_chan2mode(vap->iv_bss->ni_chan);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s \n", __func__);
#if UMAC_SUPPORT_BTAMP
    /*
     * For BTAMP vap, HT may be disabled based on iv_des_mode.
     *
     * TBD: Other phy properties are not controllable, such as preamble, slot, ERP,
     *      because ic is referenced to set the above properties.
     */
    if (IEEE80211_IS_CHAN_11N(vap->iv_bss->ni_chan)) {
        if (vap->iv_opmode == IEEE80211_M_BTAMP)
            vap->iv_cur_mode = vap->iv_des_mode;
    }
#endif

#ifdef ATHR_RNWF
    ieee80211_vap_start(vap);
#else
    if (ieee80211_dfs_cac_start(vap->iv_ic)) {
         /* NON DFS channel, Start host ap */
        ieee80211_vap_start(vap);
     }
#endif
}

int 
mlme_create_infra_bss(struct ieee80211vap *vap)    
{
    struct ieee80211com         *ic = vap->iv_ic;
    struct ieee80211_channel    *chan = NULL;
    ieee80211_ssid              *ssid = NULL;
    int                         n_ssid;
    int                         error = 0;

    n_ssid = ieee80211_get_desired_ssid(vap, 0,&ssid);

    if (ssid == NULL)
        return EINVAL;
        
    /* 
     * if there is a scan in progress.
     * then there is a vap currently scanning and the chip
     * is off on a different channel. we can not bring up 
     * vap at this point.  Scan can be in progress for 
     * independant repeater vaps, since they do not change channels.
     */
    /* 
   * WAR: if ACS also in progress, then scan should be done.
   * Just in scan post event and VAP could bring up.
   */
   
   /*
    * When the resmgr is active, do not fail vap creation even if a scan is in progress
    */
    if (!ieee80211_resmgr_active(ic) && wlan_scan_in_progress(vap) && !wlan_autoselect_in_progress(vap)
        && !ieee80211_vap_vap_ind_is_set(vap)) {
        return EAGAIN;
    }

    /* create BSS node for infra network */
    error = ieee80211_create_infra_bss(vap,ssid->ssid, ssid->len);

    if (error) {
        goto err;
    }

    /*
     * at this point the bss node (vap->iv_bss) has 2 references.
     * one for the fact that it is part of the node table.
     * the second one for being a bss node and being referred from 
     * vap->iv_bss.
     */

    chan =  vap->iv_des_chan[vap->iv_des_mode];

    if (chan == NULL) {
        return EINVAL;
    }

    if (IEEE80211_IS_CHAN_DISALLOW_HOSTAP(chan)) {
        return EINVAL;
    }

    if(IEEE80211_IS_CHAN_2GHZ(chan)) {
        ieee80211_vap_doth_clear(vap);
    }

    /*
     * issue a vap start request to resource manager.
     * if the function returns EOK (0) then its ok to change the channel synchronously
     * if the function returns EBUSY  then resource manager will 
     * switch channel asynchronously and post an event event handler registred by vap and
     * vap handler will intern call the wlan_mlme_join_infra_continue .
     */
    error = ieee80211_resmgr_vap_start(ic->ic_resmgr,vap,chan,MLME_REQ_ID,0);
    if (error == EOK) { /* no resource manager in place */
        int numvaps = ieee80211_vaps_active(ic);
        /* 
         * if there is a vap already running.
         * ignore the desired channel and use the
         * operating channel of the other vap.
         */
        /* so that cwm can do its own crap. need to untie from state */
        /* vap join is called here to wake up the chip if it is in sleep state */
        ieee80211_vap_join(vap);

        if (numvaps == 0) {
            if (error == EOK ) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: Setting channel number %d\n", __func__, chan->ic_ieee);
                ieee80211_set_channel(ic, chan);

                vap->iv_bsschan = ic->ic_curchan;   /* ieee80211 Layer - Default Configuration */
            }

            /* XXX reset erp state */
            ieee80211_reset_erp(ic, ic->ic_curmode, vap->iv_opmode);
            ieee80211_wme_initparams(vap);
        } else {
           vap->iv_bsschan = ic->ic_curchan;    /* get the current channel */
        }
        ieee80211_mlme_create_infra_continue(vap);
    }

err:
    return error;
}
void ieee80211_mlme_create_infra_continue_async(struct ieee80211vap *vap, int32_t status)
{
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: status %d\n", __func__,status);
    if (status == EOK) {
        ieee80211_mlme_create_infra_continue(vap);
    }

    IEEE80211_DELIVER_EVENT_MLME_JOIN_COMPLETE_INFRA(vap, (status == EOK) ? IEEE80211_STATUS_SUCCESS : IEEE80211_STATUS_REFUSED);
}

/*
 * function to handle shared auth in HOST AP mode.
 */

u_int16_t
mlme_auth_shared(struct ieee80211_node *ni, u_int16_t seq, u_int16_t status, 
                 u_int8_t *challenge,u_int16_t challenge_len)
{
    struct ieee80211vap    *vap = ni->ni_vap;
    struct ieee80211com    *ic = ni->ni_ic;
    u_int16_t              estatus = IEEE80211_STATUS_SUCCESS;

    /*
     * NB: this can happen as we allow pre-shared key
     * authentication to be enabled w/o wep being turned
     * on so that configuration of these can be done
     * in any order.  It may be better to enforce the
     * ordering in which case this check would just be
     * for sanity/consistency.
     */
    estatus = 0;            /* NB: silence compiler */
    if (!IEEE80211_VAP_IS_PRIVACY_ENABLED(vap)) {
        IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH,
                           ni->ni_macaddr, "shared key auth",
                           "%s", " PRIVACY is disabled");
        estatus = IEEE80211_STATUS_ALG;
    }

    if (estatus == IEEE80211_STATUS_SUCCESS) {
        switch (seq) {
        case IEEE80211_AUTH_SHARED_CHALLENGE:
        case IEEE80211_AUTH_SHARED_RESPONSE:
            if (challenge == NULL) {
                IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH,
                                   ni->ni_macaddr, "%s\n", "shared key auth no challenge");
                vap->iv_stats.is_rx_bad_auth++;
                estatus = IEEE80211_STATUS_CHALLENGE;
            } else if (challenge_len != IEEE80211_CHALLENGE_LEN) {
                IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, ni->ni_macaddr, 
                                   "shared key auth bad challenge len %d", challenge_len);
                vap->iv_stats.is_rx_bad_auth++;
                estatus = IEEE80211_STATUS_CHALLENGE;
            }
        default:
            break;
        }
    }

    if (estatus == IEEE80211_STATUS_SUCCESS) {
        switch (seq) {
        case IEEE80211_AUTH_SHARED_REQUEST:
            if (ni->ni_challenge == NULL)
                ni->ni_challenge = (u_int32_t *)OS_MALLOC(ic->ic_osdev ,IEEE80211_CHALLENGE_LEN,0);
            if (ni->ni_challenge == NULL) {
                IEEE80211_NOTE(ni->ni_vap,
                               IEEE80211_MSG_DEBUG | IEEE80211_MSG_AUTH, ni,
                               "%s", "shared key challenge alloc failed");
                /* XXX statistic */
                estatus = IEEE80211_STATUS_UNSPECIFIED;
            } else {
                /*
                 * get random bytes for challenge text.
                 */
         
                OS_GET_RANDOM_BYTES(ni->ni_challenge,
                                    IEEE80211_CHALLENGE_LEN);
                IEEE80211_NOTE(vap,
                               IEEE80211_MSG_DEBUG | IEEE80211_MSG_AUTH, ni,
                               "%s", "shared key auth request \n");
                ieee80211_send_auth(ni,(seq + 1),0,(u_int8_t *)ni->ni_challenge,IEEE80211_CHALLENGE_LEN,NULL);
            }
            break;
        case IEEE80211_AUTH_SHARED_RESPONSE:
            if (ni->ni_challenge == NULL) {
                IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH,
                                   ni->ni_macaddr, "shared key response",
                                   "%s", "no challenge recorded");
                vap->iv_stats.is_rx_bad_auth++;
                estatus = IEEE80211_STATUS_CHALLENGE;
            } else if (memcmp(ni->ni_challenge, challenge,
                              challenge_len) != 0) {
                IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH,
                                   ni->ni_macaddr, "shared key response",
                                   "%s", "challenge mismatch");
                vap->iv_stats.is_rx_auth_fail++;
                estatus = IEEE80211_STATUS_CHALLENGE;
            } else {
                IEEE80211_NOTE(vap, IEEE80211_MSG_DEBUG | IEEE80211_MSG_AUTH, ni,
                               "station authenticated (%s)\n", "shared key");
                ieee80211_node_authorize(ni);
                /*
                 * shared auth success.
                 */
                ieee80211_send_auth(ni,(seq + 1),0, NULL,0,NULL);
            }
            break;
        default:
            IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH,
                               ni->ni_macaddr, "shared key auth ",
                               "bad seq %d \n", seq);
            vap->iv_stats.is_rx_bad_auth++;
            estatus = IEEE80211_STATUS_SEQUENCE;
            break;
        }
    }

    /*
     * Send an error response.
     */
    if (estatus != IEEE80211_STATUS_SUCCESS) {
        ieee80211_send_auth(ni,(seq + 1),estatus, NULL,0,NULL);
    }

    return estatus;
}

void mlme_recv_auth_ap(struct ieee80211_node *ni,
                       u_int16_t algo, u_int16_t seq, u_int16_t status_code,
                       u_int8_t *challenge, u_int8_t challenge_length, wbuf_t wbuf)
{

    struct ieee80211vap           *vap = ni->ni_vap;
    struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;
    struct ieee80211_frame        *wh;
    u_int16_t                     indication_status = IEEE80211_STATUS_SUCCESS,response_status = IEEE80211_STATUS_SUCCESS ;
    bool                          send_auth_response=true,indicate=true;;

    wh = (struct ieee80211_frame *) wbuf_header(wbuf);
    /* AP must be up and running */
    if (!mlme_priv->im_connection_up || ieee80211_vap_ready_is_clear(vap)) {
        return;
    }


    IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, wh->i_addr2,
                       "recv auth frame with algorithm %d seq %d \n", algo, seq);

    do {
        bool create_new_node = TRUE;

        /* Always remove the old client node. Otherwise, station count can be wrong */
        if (ni != vap->iv_bss)  {
            /* Call MLME indication handler if node is in associated state */
            if (seq == IEEE80211_AUTH_OPEN_REQUEST ||
                seq == IEEE80211_AUTH_SHARED_REQUEST) {
                /* if receive the re-auth frame without any disassoc check if node is at power save mode
                   let the sta leave the power save state. */
                if ((ni->ni_flags & IEEE80211_NODE_PWR_MGT) == IEEE80211_NODE_PWR_MGT)
                {
                    ieee80211_mlme_node_pwrsave_ap(ni,0);
                }
                /* Leave the node only if PMF not enabled */
                if (!ieee80211_is_pmf_enabled(vap, ni) &&
                    !ieee80211_vap_trigger_mlme_resp_is_set(vap)) {
                    ieee80211_ref_node(ni);
                    if(IEEE80211_NODE_LEAVE(ni)) {
                        IEEE80211_DELIVER_EVENT_MLME_DISASSOC_INDICATION(vap,
                                                                           ni->ni_macaddr,
                                                                           IEEE80211_REASON_ASSOC_LEAVE);
                    }
                    ieee80211_free_node(ni);
                } else {
                    /* PMF is enabled so don't create new node */
                    create_new_node = FALSE;
                }
            } else {
                /* Second auth in AUTH_ALG_SHARED */
                create_new_node = FALSE;
            }
        }

        if (create_new_node) {
            /* create a node for the station */

            /* 
             * If the VAP is in forced paused state, then we cannot create
             * a new node because of synchronization issues. i.e. the vap is
             * in forced paused state but this new node is in unpause state.
             * But the P2P station client should not be sending packets during
             * NOA sleep anyway. So, refused this connection.
             * If the VAP is in paused state, then we also cannot create
             * a new node because of sync. issues. i.e. there may be another thread
             * that is unpausing this VAP and our new node creation may be
             * corrupted since these 2 threads are not synchronized.
             */
            if ((ieee80211_vap_is_force_paused(vap)) ||
                (ieee80211_vap_is_paused(vap)))
            {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s: Reject this Auth since VAP is in pause or forced paused.\n", 
                                  __func__);
                indication_status = IEEE80211_STATUS_OTHER;
                return;
            }

            ni = ieee80211_dup_bss(vap, wh->i_addr2);
            if (ni == NULL) {
                indication_status = IEEE80211_STATUS_OTHER;
                return;
            }
        
            /* override bss authmode for shared auth request algorithm*/
            if (algo  == IEEE80211_AUTH_ALG_SHARED)
                ni->ni_authmode = IEEE80211_AUTH_SHARED;
        } else {
           ieee80211_ref_node(ni);
        }

        /* Validate algo */
        if (algo == IEEE80211_AUTH_ALG_SHARED && !RSN_AUTH_IS_SHARED_KEY(&vap->iv_rsn)) {
            response_status = IEEE80211_STATUS_ALG;
            indication_status = IEEE80211_STATUS_ALG;
            break;
        } 

        if (algo == IEEE80211_AUTH_ALG_OPEN && RSN_AUTH_IS_SHARED_KEY(&vap->iv_rsn) && 
            !RSN_AUTH_IS_OPEN(&vap->iv_rsn)) {
            response_status = IEEE80211_STATUS_ALG;
            indication_status = IEEE80211_STATUS_ALG;
            break;
        } 

        /*
         * Consult the ACL policy module if setup.
         */
        if (!ieee80211_acl_check(vap, wh->i_addr2)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACL,
                              "[%s] auth: disallowed by ACL \n",ether_sprintf(wh->i_addr2));
            response_status = IEEE80211_STATUS_REFUSED;
            indication_status = IEEE80211_STATUS_REFUSED;
            vap->iv_stats.is_rx_acl++;
            break;
        }

        if (IEEE80211_VAP_IS_COUNTERM_ENABLED(vap)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_AUTH | IEEE80211_MSG_CRYPTO,
                              "[%s] auth: TKIP countermeasures enabled \n",ether_sprintf(wh->i_addr2));
            vap->iv_stats.is_rx_auth_countermeasures++;
            response_status = IEEE80211_REASON_MIC_FAILURE;
            indication_status = IEEE80211_STATUS_REFUSED;
            break;
        }
        /*
         * reject auth if there are too many STAs already associated.
         */
        if (vap->iv_sta_assoc >= vap->iv_max_aid) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_AUTH,
                              "[%s] num auth'd STAs is %d, max is %d, rejecting "
                              "new auth\n", ether_sprintf(wh->i_addr2), 
                              vap->iv_sta_assoc, vap->iv_max_aid);

            response_status = IEEE80211_STATUS_TOOMANY;
            indication_status = IEEE80211_STATUS_TOOMANY;
            break;
        }
        if (algo == IEEE80211_AUTH_ALG_OPEN) {
            if (seq != IEEE80211_AUTH_OPEN_REQUEST) {
                response_status = IEEE80211_STATUS_SEQUENCE;
                indication_status = IEEE80211_STATUS_SEQUENCE;
                break;
            }    
        } else if (algo == IEEE80211_AUTH_ALG_SHARED) {
            response_status = indication_status = mlme_auth_shared(ni,seq,status_code,challenge,challenge_length);
            send_auth_response=false;
            if (seq == IEEE80211_AUTH_SHARED_REQUEST && response_status == IEEE80211_STATUS_SUCCESS)
                indicate=false;
            break;
        } else if(algo == IEEE80211_AUTH_ALG_FT) {
                /* TODO: decide what all need to be done for FT frame */
                /*response_status =;
                  indication_status = ;
                  send_auth_response=;
                  indicate=;
                  vap->iv_stats.xxxx++*/
            break;
        } else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_AUTH | IEEE80211_MSG_CRYPTO,
                              "[%s] auth: unsupported algorithm %d \n",ether_sprintf(wh->i_addr2),algo);
            vap->iv_stats.is_rx_auth_unsupported++;
            response_status = IEEE80211_STATUS_ALG;
            indication_status = IEEE80211_STATUS_ALG;
            break;
        }
    } while (FALSE);

    if (indicate ) {
        IEEE80211_DELIVER_EVENT_MLME_AUTH_INDICATION(vap, ni->ni_macaddr, 
                                                         indication_status);
    }

    if (ieee80211_vap_trigger_mlme_resp_is_set(vap)) {
        /* Wait for application to trigger mlme response for auth */
        ni->ni_authalg = algo;
        ni->ni_authstatus = response_status;
    } else {
        if (send_auth_response) {
            ieee80211_send_auth(ni, seq + 1, response_status, NULL, 0, NULL);
        }

        IEEE80211_DELETE_NODE_TARGET(ni, ni->ni_ic, vap, 0);
        if (indication_status != IEEE80211_STATUS_SUCCESS ){
            /* auth is not success, remove the node from node table*/
            IEEE80211_NODE_LEAVE(ni);
        }
    }
    /*
     * release the reference created at the begining of the case above
     * either by alloc_node or ref_node.
     */ 
    ieee80211_free_node(ni);
}

void
ieee80211_mlme_node_leave_ap(struct ieee80211_node *ni)
{
    struct ieee80211vap         *vap = ni->ni_vap;
    struct ieee80211_mlme_priv  *mlme_priv = vap->iv_mlme_priv;
    ieee80211_mlme_event          event;

    ASSERT(vap != NULL);
    ASSERT(vap->iv_opmode != IEEE80211_M_STA);

    event.u.event_sta.sta_count= vap->iv_sta_assoc;
    event.u.event_sta.sta_ps_count= vap->iv_ps_sta;
    event.u.event_sta.ni = ni;

    event.type = IEEE80211_MLME_EVENT_STA_LEAVE;
    ieee80211_mlme_deliver_event(mlme_priv,&event);

    /* NB: preserve ni_table */
    if (ieee80211node_has_flag(ni, IEEE80211_NODE_PWR_MGT)) {

        vap->iv_ps_sta--;
        ieee80211node_clear_flag(ni, IEEE80211_NODE_PWR_MGT);
#if NOT_YET
        if (ieee80211node_has_flag(ni, IEEE80211_NODE_UAPSD_TRIG)) {
            ieee80211node_clear_flag(ni, IEEE80211_NODE_UAPSD_TRIG);
            IEEE80211_UAPSD_LOCK(ni->ni_ic);
            ni->ni_ic->ic_uapsdmaxtriggers--;
            IEEE80211_UAPSD_UNLOCK(ni->ni_ic);
        }
#endif
    
        event.u.event_sta.sta_ps_count= vap->iv_ps_sta;
        event.type = IEEE80211_MLME_EVENT_STA_EXIT_PS;
        ieee80211_mlme_deliver_event(mlme_priv,&event);
    }


}

void
ieee80211_mlme_node_pwrsave_ap(struct ieee80211_node *ni, int enable)
{
    struct ieee80211vap *vap = ni->ni_vap;
#ifdef ATH_SWRETRY
    struct ieee80211com *ic = ni->ni_ic;
#endif
    ieee80211_mlme_event          event;

    if  ( ((ni->ni_flags & IEEE80211_NODE_PWR_MGT) != 0) ^ enable) {
        struct ieee80211_mlme_priv    *mlme_priv = vap->iv_mlme_priv;

        if (enable) {
            vap->iv_ps_sta++;
            ni->ni_flags |= IEEE80211_NODE_PWR_MGT;
#ifdef ATH_SWRETRY
            if (ic->ic_node_psupdate) {
                ic->ic_node_psupdate(ni, 1, 1);
                IEEE80211_NOTE(vap, IEEE80211_MSG_POWER, ni, "%s", "pause LMAC node\n");
            }
#endif
            IEEE80211_NOTE(vap, IEEE80211_MSG_POWER, ni,
                           "power save mode on, %u sta's in ps mode\n", vap->iv_ps_sta);
            ieee80211node_pause(ni);
            event.type = IEEE80211_MLME_EVENT_STA_ENTER_PS;
        } else {

            vap->iv_ps_sta--;
            ni->ni_flags &= ~IEEE80211_NODE_PWR_MGT;
#ifdef ATH_SWRETRY
            if (ic->ic_node_psupdate) {
                ic->ic_node_psupdate(ni, 0, 1);
                IEEE80211_NOTE(vap, IEEE80211_MSG_POWER, ni, "%s", "unpause LMAC node\n");
            }
#endif
            ieee80211node_unpause(ni);
            IEEE80211_NOTE(vap, IEEE80211_MSG_POWER, ni,
                           "power save mode off, %u sta's in ps mode\n", vap->iv_ps_sta);
            event.type = IEEE80211_MLME_EVENT_STA_EXIT_PS;

        }

        event.u.event_sta.sta_count= vap->iv_sta_assoc;
        event.u.event_sta.sta_ps_count= vap->iv_ps_sta;
        event.u.event_sta.ni = ni;
        ieee80211_mlme_deliver_event(mlme_priv,&event);
    }
}


#endif /* UMAC_SUPPORT_AP */
