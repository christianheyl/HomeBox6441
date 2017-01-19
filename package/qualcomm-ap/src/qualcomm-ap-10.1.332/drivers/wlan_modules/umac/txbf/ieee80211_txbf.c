/*
 *  Copyright (c) 2008 Atheros Communications Inc.  All rights reserved.
 * \brief 802.11n compliant Transmit Beamforming
 */
/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#include <ieee80211_var.h>
#include <ieee80211_channel.h>

#ifdef ATH_SUPPORT_TxBF
extern void ieee80211_send_setup( struct ieee80211vap *vap, struct ieee80211_node *ni,
    struct ieee80211_frame *wh, u_int8_t type, const u_int8_t *sa, const u_int8_t *da,
                           const u_int8_t *bssid);

enum {
    NOT_SUPPORTED         = 0,
    DELAYED_FEEDBACK      = 1,
    IMMEDIATE_FEEDBACK    = 2,
    DELAYED_AND_IMMEDIATE = 3,
};
void 
ieee80211_set_TxBF_keycache(struct ieee80211com *ic, struct ieee80211_node *ni)
{
    //struct ieee80211vap *vap = ni->ni_vap;
	 
    //IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"==>%s:keyid: %d, cipher %x,flags %x",__func__,ni->ni_ucastkey.wk_keyix
    //        ,ni->ni_ucastkey.wk_cipher->ic_cipher,ni->ni_ucastkey.wk_flags);

    if (ni->ni_ucastkey.wk_keyix == IEEE80211_KEYIX_NONE){  //allocate new key
        ni->ni_ucastkey.wk_keyix=ic->ic_txbf_alloc_key(ic, ni);
    }
    if (( ni->ni_ucastkey.wk_keyix != IEEE80211_KEYIX_NONE)&& 
        (ni->ni_ucastkey.wk_cipher->ic_cipher == IEEE80211_CIPHER_NONE)){ 
        ic->ic_txbf_set_key(ic, ni);   // update staggered sounding ,cec , mmss into key cache.
    }
}

void
ieee80211_match_txbfcapability(struct ieee80211com *ic, struct ieee80211_node *ni)
{
    union ieee80211_hc_txbf *bfer, *bfee;
    bool previous_txbf_active = 0;

    bfer = &ic->ic_txbf;
    bfee = &ni->ni_txbf;

    /* set previous link txbf status*/
    if (ni->ni_explicit_compbf || ni->ni_explicit_noncompbf || ni->ni_implicit_bf){
        previous_txbf_active = 1;
    }
    ni->ni_explicit_compbf = FALSE;
    ni->ni_explicit_noncompbf = FALSE;
    ni->ni_implicit_bf = FALSE;

    /* XXX need multiple chains to do BF?*/
    /* Skipping the TxBF, if single chain is enabled 
     * (chain 0 or chain 1 or chain 2) 
     */ 
    if ((ic->ic_tx_chainmask & (ic->ic_tx_chainmask - 1)) == 0)
        return;
    /*
     * We would prefer to do explicit BF if possible.
     * So the following checks are order sensitive.
     */
     /* suppot both delay report and immediately report*/
     
    if (bfer->explicit_comp_steering && (bfee->explicit_comp_bf != 0)) {
        ni->ni_explicit_compbf = TRUE;
        
        /* re-initial TxBF timer for previous link is not TxBF link*/
        if (previous_txbf_active == 0){
            ieee80211_init_txbf(ic, ni);
        }        
        return;
    }

    if (bfer->explicit_noncomp_steering && (bfee->explicit_noncomp_bf != 0)){
        ni->ni_explicit_noncompbf = TRUE;
        if (previous_txbf_active == 0){
            ieee80211_init_txbf(ic, ni);
        }  
        return;
    }

    if (bfer->implicit_txbf_capable && bfee->implicit_rx_capable) {
        ni->ni_implicit_bf = TRUE;
        if (previous_txbf_active == 0){
            ieee80211_init_txbf(ic, ni);
        }  
        return;
    }
}

u_int8_t *
ieee80211_add_htc(wbuf_t wbuf, int use4addr)
{
    struct ieee80211_frame *wh;
    u_int8_t *htc;

    wh = (struct ieee80211_frame *)wbuf_header(wbuf);

    /* set order bit in frame control to enable HTC field; */
    wh->i_fc[1] |= IEEE80211_FC1_ORDER;	

    if (!use4addr) {
        htc = ((struct ieee80211_qosframe_htc *)wh)->i_htc;
    } else {              					
        htc= ((struct ieee80211_qosframe_htc_addr4 *)wh)->i_htc;
    }

    htc[0] = htc[1] = htc[2] = htc[3] = 0;

    return htc;
}

void
ieee80211_request_cv_update(struct ieee80211com *ic,struct ieee80211_node *ni, wbuf_t wbuf, int use4addr)
{
    u_int8_t *htc;
    /* struct ieee80211vap *vap = ni->ni_vap;
	        
     *IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"==>%s:\n",__func__);*/

    htc = ieee80211_add_htc(wbuf, use4addr);

    /* clear update flags */
    ni->ni_bf_update_cv = 0;
    if (ni->ni_explicit_compbf) {
        /* set CSI=3 to request compressed ExBF */
        htc[2] = IEEE80211_HTC2_CSI_COMP_BF;
    } else if (ni->ni_explicit_noncompbf) {
        /* set CSI=2 to request non-compressed ExBF */
        htc[2] = IEEE80211_HTC2_CSI_NONCOMP_BF;
    }
#ifdef TXBF_TODO
     else if (ni->ni_implicit_bf) {
        /* set TRQ to request sounding from bfee in ImBF */
        ni->ni_bf_update_cv = 1;   /* still needs to request update*/
        if ((ni->ni_chwidth == IEEE80211_CWM_WIDTH40) && 
            (ic->ic_cwm_get_width(ic) == IEEE80211_CWM_WIDTH40)){
            /*40M bandwidth*/
		    if (ic->ic_rc_40M_valid) {
                /* Radio coefficient reeady, send TRQ */
                htc[0] |= IEEE80211_HTC0_TRQ;
                ni->ni_bf_update_cv = 0;   
            } else if (ic->ic_rc_calibrating == 0) {
                /* not in calibrating mode, start calibrate */
                ic->ic_rc_calibrating = 1;
                ic->ic_ap_save_join_mac(ic, ni->ni_macaddr);
                ic->ic_start_imbf_cal(ic);
            }
        } else {
            if (ic->ic_rc_20M_valid) {
                /*printk("==>%s:set TRQ\n",__func__);*/
                htc[0] |= IEEE80211_HTC0_TRQ;
                ni->ni_bf_update_cv = 0;
            } else if (ic->ic_rc_calibrating == 0) {
                ic->ic_rc_calibrating = 1 ;
                ic->ic_ap_save_join_mac(ic, ni->ni_macaddr);
                ic->ic_start_imbf_cal(ic);
            }
        }            
    }    
#endif
    return;
}

//for TxBF RC
/*
 * Send a Crlx frame of Beamforming to the specified node.
	Cal_type : 0	 ==> Cal 1
			   or	 ==> Cal 3

 */
#ifdef TXBF_TODO
int
ieee80211_send_cal_qos_nulldata(struct ieee80211_node *ni, int Cal_type)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic = ni->ni_ic;
    wbuf_t wbuf;
    struct ieee80211_frame *wh;
	struct ieee80211_qosframe_htc *wh1;

    if (ieee80211_get_home_channel(vap) !=
        ieee80211_get_current_channel(ic)) {
		
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,
		  "%s[%d] cur chan %d is not same as home chan %d\n", __func__, __LINE__,
          ieee80211_chan2ieee(ic, ic->ic_curchan), ieee80211_chan2ieee(ic, vap->iv_bsschan));
		return 0;
	}
    /*
     * XXX: It's the same as a management frame in the sense that
     * both are self-generated frames.
     */
	//DbgPrint("Into ==============%s======================\n",__FUNCTION__);
    wbuf = wbuf_alloc(ic->ic_osdev, WBUF_TX_MGMT, (sizeof(struct ieee80211_qosframe_htc)+2));
    if (wbuf == NULL) {
        return -ENOMEM;
    }
    /* setup the wireless header */
    wh = (struct ieee80211_frame *) wbuf_header(wbuf);
    ieee80211_send_setup(vap, ni, wh,IEEE80211_FC0_TYPE_DATA | IEEE80211_FC0_SUBTYPE_QOS_NULL,
        vap->iv_myaddr, ieee80211_node_get_macaddr(ni), ieee80211_node_get_bssid(ni));

	/* set order bit to enable HTC field */
    wh->i_fc[1] |= IEEE80211_FC1_ORDER;
	/*setup it  80211 header of QoS and Cal 1 or Cal 3
	
		b_0~15		Link Adaptation Control
		b_16~17		Calibration Position
		b_18~19		Calibration Sequence
		b_20~21		Reserved
		b_22~23		CSI/Steering
		b_24		NDP Announcement
		b_25~29		Reserved
		b_30		AC Constraint
		b_31		RDG/More PPDU

		Link Adaptation Contorl
		b_0			Reserved
		b_1			TRQ
		b_2~5		MAI
		b_6~8		MFSI
		b_9_15		MFB/ASELC
	*/
	wh1 = (struct ieee80211_qosframe_htc *) wbuf_header(wbuf);
	if (Cal_type) {
        /* Cal 3 */
		wh1->i_qos[0] = 0x00;
		wh1->i_qos[1] = 0x00;

		wh1->i_htc[0] = 0x00;
		wh1->i_htc[1] = 0x00;
		wh1->i_htc[2] = 0x03+0x4;//Pos 3+Calibration Sequence
		wh1->i_htc[3] = 0x00;
	} else {//Cal 1
		wh1->i_qos[0] = 0x00;
		wh1->i_qos[1] = 0x00;

		wh1->i_htc[0] = 0x02;//TRQ
		wh1->i_htc[1] = 0x00;
		wh1->i_htc[2] = 0x01+0x4;//Pos 1 + Calibration Sequence
		wh1->i_htc[3] = 0x00;
	}
  
    wbuf_set_pktlen(wbuf, (sizeof(struct ieee80211_qosframe_htc)+2));//+2 for tx_prepare to calculate the hdrlen
	//DbgPrint("%s set pktlen = %d \n",__FUNCTION__ ,wbuf_get_pktlen(wbuf));
    wbuf_set_priority(wbuf, WME_AC_BE);

    /* ic->ic_pwrsave_set_state(ic, IEEE80211_PWRSAVE_AWAKE);*/
    vap->iv_lastdata = OS_GET_TIMESTAMP();
    return ieee80211_send_mgmt(vap, ni, wbuf,true);
}
#endif

OS_TIMER_FUNC(txbf_cv_timeout)
{
    struct ieee80211_node *ni;
    OS_GET_TIMER_ARG(ni, struct ieee80211_node *);

    ni->ni_allow_cv_update = 1; // time up, allow cv update.
}

OS_TIMER_FUNC(txbf_cv_report_timeout)
{
    struct ieee80211_node *ni;

    OS_GET_TIMER_ARG(ni, struct ieee80211_node *);

    ni->ni_bf_update_cv = 1; // time up, request cv update immediately.

    if (ni->ni_sw_cv_timeout){ 
        // reset sw timer
        OS_CANCEL_TIMER(&(ni->ni_cv_timer));
        OS_SET_TIMER(&ni->ni_cv_timer, ni->ni_sw_cv_timeout);
    }
}

void ieee80211_init_txbf(struct ieee80211com *ic,struct ieee80211_node *ni)
{
    if (ni->ni_explicit_compbf || ni->ni_explicit_noncompbf || ni->ni_implicit_bf) {
        ni->ni_bf_update_cv = 0;
        ni->ni_allow_cv_update = 0;
        ic->ic_init_sw_cv_timeout(ic, ni);
        //IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,"==>%s: this node support TxBF, set initial BF ",__func__);
        if (ni->ni_txbf_timer_initialized){
            OS_FREE_TIMER(&ni->ni_cv_timer);
            OS_FREE_TIMER(&ni->ni_report_timer);
        }        
        OS_INIT_TIMER(ic->ic_osdev, &ni->ni_cv_timer, txbf_cv_timeout, ni);
        OS_INIT_TIMER(ic->ic_osdev, &ni->ni_report_timer, txbf_cv_report_timeout, ni);
        ni->ni_txbf_timer_initialized = 1;
    }
} 
#endif /* ATH_SUPPORT_TxBF */
