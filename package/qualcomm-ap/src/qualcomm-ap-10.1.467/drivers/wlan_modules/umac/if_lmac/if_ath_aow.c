/*
 * Copyright (c) 2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 */



#if  ATH_SUPPORT_AOW

#include "if_athvar.h"
#include "if_ath_aow.h"

#define DEFAULT_AOW_LATENCY_INMSEC          23
#define DEFAULT_AOW_PROC_TIME                5000 
/* Number of RTS retries per transmission series. Does not include 
   the first attempt. */
#define ATH_AOW_MIN_RTS_RETRIES             (0)
#define ATH_AOW_MAX_RTS_RETRIES             (3)
#define ATH_AOW_DEFAULT_RTS_RETRIES         (0)

int ath_get_aow_macaddr(struct ieee80211com *ic, int channel, int index, struct ether_addr *macaddr)
{

    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    int ret = AH_FALSE;

    if ((channel < 0) || (channel > (AOW_MAX_AUDIO_CHANNELS - 1))) {
        return AH_FALSE;
    }        

    if (sc->sc_aow.chan_addr[channel].valid)  {
        IEEE80211_ADDR_COPY(macaddr, &sc->sc_aow.chan_addr[channel].addr[index]);
        ret = AH_TRUE;
    }        

    return ret;
}

int ath_get_aow_chan_seqno(struct ieee80211com *ic, int channel, int* seqno)
{
    
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);

    if ((channel < 0) || (channel > (AOW_MAX_AUDIO_CHANNELS - 1))) {
        return AH_FALSE;
    }        

    if (sc->sc_aow.chan_addr[channel].valid) {
        *seqno = sc->sc_aow.chan_addr[channel].seqno++;
    }        

    return AH_TRUE;
}    


void ath_set_audio_channel(struct ieee80211com *ic, int channel, struct ether_addr *macaddr)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    int index = 0;
    u_int8_t event_subtype = CM_CHANNEL_ADDRESS_SET_PASS;


    if ((channel < 0) || (channel > (AOW_MAX_AUDIO_CHANNELS - 1))) {
        event_subtype = CM_CHANNEL_ADDRESS_SET_FAIL;
        goto err;
        return;
    }        

    if ((sc->sc_aow.mapped_recv_cnt >= AOW_MAX_RECEIVER_COUNT) ||
        (sc->sc_aow.chan_addr[channel].dst_cnt >= AOW_MAX_RECEIVER_COUNT)) {
        IEEE80211_AOW_DPRINTF("\nSet error : Max Limit reached\n");
        ath_list_audio_channel(ic);
        event_subtype = CM_CHANNEL_ADDRESS_SET_FAIL;
        goto err;
        return;
    }

    sc->sc_aow.chan_addr[channel].channel = channel;
    sc->sc_aow.chan_addr[channel].valid   = AH_TRUE;

    index = sc->sc_aow.chan_addr[channel].dst_cnt;
    IEEE80211_ADDR_COPY(&sc->sc_aow.chan_addr[channel].addr[index], macaddr);
    sc->sc_aow.chan_addr[channel].dst_cnt++;
    sc->sc_aow.mapped_recv_cnt++;

    /* set the channel bit flag to optimize the check on transmit */
    ic->ic_aow.channel_set_flag = ic->ic_aow.channel_set_flag | (1 << channel);
err:    
    ieee80211_aow_send_to_host(ic, &event_subtype, sizeof(event_subtype), AOW_HOST_PKT_EVENT, event_subtype, NULL);
}


void ath_remove_audio_channel(struct ieee80211com* ic, int channel, struct ether_addr* macaddr)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    int i = 0;

    if ((channel < 0) || (channel > (AOW_MAX_AUDIO_CHANNELS - 1))) {
        return;
    }        

    for (i = 0 ; i < AOW_MAX_RECEIVER_COUNT; i++) {
        if (!IS_ETHER_ADDR_NULL(sc->sc_aow.chan_addr[channel].addr[i].octet)) {
            if (IEEE80211_ADDR_EQ(&sc->sc_aow.chan_addr[channel].addr[i], macaddr)) {
                memset(&sc->sc_aow.chan_addr[channel].addr[i], 0 , IEEE80211_ADDR_LEN);
                sc->sc_aow.chan_addr[channel].dst_cnt--;
                sc->sc_aow.mapped_recv_cnt--;
            }
        }
    }

    if (!sc->sc_aow.chan_addr[channel].dst_cnt) {
        sc->sc_aow.chan_addr[channel].valid = AH_FALSE;
        sc->sc_aow.chan_addr[channel].seqno = 0;
        ic->ic_aow.channel_set_flag &= ~(1 << channel);
    }        
}

void ath_list_audio_channel(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    int i = 0;
    int j = 0;

    for (i = 0; i < AOW_MAX_AUDIO_CHANNELS; i++) {
        if (sc->sc_aow.chan_addr[i].valid) {
            IEEE80211_AOW_DPRINTF("\nAudio channel = %d\n", sc->sc_aow.chan_addr[i].channel);
            IEEE80211_AOW_DPRINTF("-------------------\n");
            //for (j = 0; j < sc->sc_aow.chan_addr[i].dst_cnt; j++) {
            for (j = 0; j < AOW_MAX_RECEIVER_COUNT; j++) {
                if (!IS_ETHER_ADDR_NULL(sc->sc_aow.chan_addr[i].addr[j].octet))
                    IEEE80211_AOW_DPRINTF("%s\n",ether_sprintf((char*)&sc->sc_aow.chan_addr[i].addr[j]));
            }                     
        }                    
    }                    
}

int ath_get_num_mapped_dst(struct ieee80211com *ic, int channel)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);

    return sc->sc_aow.chan_addr[channel].dst_cnt;
}    
    

void ath_clear_audio_channel_list(struct ieee80211com *ic, u_int32_t val)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    int i = 0;
    int j = 0;
    int k = 0;

    /* clear the mapped receiver count */
    sc->sc_aow.mapped_recv_cnt = 0;

    if (val < 0) {
        IEEE80211_AOW_DPRINTF("AoW Error: Invalid channel index\n");
        return;
    }        

    if (val < AOW_MAX_AUDIO_CHANNELS) {
        sc->sc_aow.chan_addr[val].channel = 0;
        sc->sc_aow.chan_addr[val].valid = AH_FALSE;
        sc->sc_aow.chan_addr[val].seqno = 0;
        sc->sc_aow.chan_addr[val].dst_cnt = 0;

        for ( k = 0; k < AOW_MAX_RECEIVER_COUNT; k++) {
            OS_MEMSET(&sc->sc_aow.chan_addr[val].addr[k], 0x00, sizeof(struct ether_addr));
        }            

        /* clear the set channel map */            
        ic->ic_aow.channel_set_flag &= ~(1 << val);

    } else {
        for (i = 0; i < AOW_MAX_AUDIO_CHANNELS; i++) {
            sc->sc_aow.chan_addr[i].channel = 0;
            sc->sc_aow.chan_addr[i].valid = AH_FALSE;
            sc->sc_aow.chan_addr[i].seqno = 0;
            sc->sc_aow.chan_addr[i].dst_cnt = 0;
            for (j = 0; j < AOW_MAX_RECEIVER_COUNT; j++)
                OS_MEMSET(&sc->sc_aow.chan_addr[i].addr[j], 0x00, sizeof(struct ether_addr));
        }        

        /* clear the set channel map */
        ic->ic_aow.channel_set_flag = 0;
    }
}

void ath_set_swretries(struct ieee80211com *ic, u_int32_t val)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    sc->sc_aow.sw_retry_limit = val;
}

void ath_set_aow_playlocal(struct ieee80211com *ic, u_int32_t val)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    sc->sc_aow.playlocal = val;

    if (val) {
        aow_i2s_init(ic);
    } else {
        aow_i2s_deinit(ic);
        CLR_I2S_STOP_FLAG(ic->ic_aow.i2s_flags);
    }
}

u_int32_t ath_get_swretries(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    return sc->sc_aow.sw_retry_limit;

}

void ath_set_aow_rtsretries(struct ieee80211com *ic, u_int16_t val)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    u_int16_t rtstries = (val + 1);
    int val_for_mincmp = val; /* To keep the compiler happy. */
   
    if((val_for_mincmp < ATH_AOW_MIN_RTS_RETRIES) ||
       (val > ATH_AOW_MAX_RTS_RETRIES)) {
        IEEE80211_AOW_DPRINTF("Invalid value (%u) for rts_retries. "
                               "Min: %u Max: %u\n",
                               val,
                               ATH_AOW_MIN_RTS_RETRIES,
                               ATH_AOW_MAX_RTS_RETRIES);
        return;
    }

    scn->sc_ops->update_txqproperty(scn->sc_dev,
                                    sc->sc_haltype2q[HAL_WME_AC_VO],
                                    TXQ_PROP_SHORT_RETRY_LIMIT,
                                    &rtstries);

    return;
}

u_int16_t ath_get_aow_rtsretries(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    u_int16_t rtstries = 0;

    scn->sc_ops->get_txqproperty(scn->sc_dev,
                                 sc->sc_haltype2q[HAL_WME_AC_VO],
                                 TXQ_PROP_SHORT_RETRY_LIMIT,
                                 &rtstries);

    return (rtstries - 1);
}

void ath_set_aow_er(struct ieee80211com *ic, u_int32_t val)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    
    sc->sc_aow.er = val ? 1:0;

    /* ER not supported in this release */
    IEEE80211_AOW_DPRINTF("Error Recovery : Not supported\n");
    sc->sc_aow.er = 0;
}

u_int32_t ath_get_aow_er(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    return sc->sc_aow.er;
}

void ath_set_aow_ec(struct ieee80211com *ic, u_int32_t val)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    sc->sc_aow.ec = val ? 1:0;
}

u_int32_t ath_get_aow_ec(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    return sc->sc_aow.ec;
}


void ath_set_aow_ec_ramp(struct ieee80211com *ic, u_int32_t val)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    sc->sc_aow.ec_ramp = val ? 1:0;
}

u_int32_t ath_get_aow_ec_ramp(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    return sc->sc_aow.ec_ramp;
}

void ath_set_aow_ec_fmap(struct ieee80211com *ic, u_int32_t val)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    sc->sc_aow.ec_fmap = val;
}

u_int32_t ath_get_aow_ec_fmap(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    return sc->sc_aow.ec_fmap;
}

void ath_set_aow_as(struct ieee80211com* ic, u_int32_t val)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    sc->sc_aow.as = val ? 1:0;
}

void ath_set_aow_assoc_policy(struct ieee80211com*ic, u_int32_t val)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    sc->sc_aow.aow_assoc_only = val ? 1:0;
}

void ath_set_aow_audio_ch(struct ieee80211com* ic, u_int32_t val)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    sc->sc_aow.audio_ch = val;
}

u_int32_t ath_get_aow_as(struct ieee80211com* ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    return sc->sc_aow.as;
}

u_int32_t ath_get_aow_assoc_policy(struct ieee80211com* ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    return sc->sc_aow.aow_assoc_only;
}

u_int32_t ath_get_aow_audio_ch(struct ieee80211com* ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    return sc->sc_aow.audio_ch;
}
void ath_set_aow_es(struct ieee80211com *ic, u_int32_t val)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    int retval;

    if (val) {
        if (sc->sc_aow.ess) {
            ic->ic_set_aow_ess(ic, 0);
        }

        if ((retval = aow_es_base_init(ic)) < 0) {
            IEEE80211_AOW_DPRINTF("Ext Stats Init failed. Turning off ES\n");
            sc->sc_aow.es = 0;
        } else {
            sc->sc_aow.es = 1;
        }

    } else {
        aow_es_base_deinit(ic);
        sc->sc_aow.es = 0;
    }
}

u_int32_t ath_get_aow_es(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    return sc->sc_aow.es;
}

void ath_set_aow_ess(struct ieee80211com *ic, u_int32_t val)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    int retval;

    if (val) {
        if (sc->sc_aow.es) {
            ic->ic_set_aow_es(ic, 0);
        }

        if ((retval = aow_es_base_init(ic)) < 0) {
            IEEE80211_AOW_DPRINTF("Ext Stats Init failed. Turning off ESS\n");
            sc->sc_aow.ess = 0;
        } else {
            aow_essc_init(ic);
            sc->sc_aow.ess = 1;
        }
    } else {
        aow_es_base_deinit(ic);
        aow_essc_init(ic);
        sc->sc_aow.ess = 0;
    }
}

u_int32_t ath_get_aow_ess(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    return sc->sc_aow.ess;
}

void ath_set_aow_ess_count(struct ieee80211com *ic, u_int32_t val)
{
    aow_essc_setall(ic, val);
}

void ath_aow_clear_estats(struct ieee80211com *ic)
{
    aow_clear_ext_stats(ic);
}

u_int32_t ath_aow_get_estats(struct ieee80211com *ic)
{
    aow_print_ext_stats(ic);
    return 0;
}

void ath_set_aow_frame_size(struct ieee80211com *ic, u_int32_t val)
{
    if (val != AOW_FRAME_SIZE_MIN && val != AOW_FRAME_SIZE_MAX) {
       IEEE80211_AOW_DPRINTF("%u is an invalid value. "
                             "Allowed values: 1, 4\n",
                             val);
       return;
    }
    
    if (ic->ic_aow.frame_size_set) {
       IEEE80211_AOW_DPRINTF("Audio frame size already set to USB. "
                             "Please reboot and change frame size before "
                             "executing confusbdev.\n");
       return;
    }

    ic->ic_aow.frame_size = val;
    
}

u_int32_t ath_get_aow_frame_size(struct ieee80211com *ic)
{
    return ic->ic_aow.frame_size;
}

void ath_set_aow_alt_setting(struct ieee80211com *ic, u_int32_t val)
{
    if (val < AOW_ALT_SETTING_MIN || val > AOW_ALT_SETTING_MAX) {
       IEEE80211_AOW_DPRINTF("%u is an invalid value. "
                             "Allowed values: 1 - 8\n",
                             val);
       return;
    }
    
    if (ic->ic_aow.alt_setting_set) {
       IEEE80211_AOW_DPRINTF("Required alt setting already configured into USB."
                             " Please reboot and change alt setting before"
                             " executing confusbdev.\n");
       return;
    }

    ic->ic_aow.alt_setting = val;
    
}

u_int32_t ath_get_aow_tx_rate_info(struct ieee80211_node* ni)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_node_net80211 *an = (struct ath_node_net80211 *)ni;
    
    if (scn->sc_ops->ath_get_tx_rate_info)
        return scn->sc_ops->ath_get_tx_rate_info(scn->sc_dev, an->an_sta);

    return 0;
}

u_int32_t ath_get_aow_alt_setting(struct ieee80211com *ic)
{
    return ic->ic_aow.alt_setting;
}


void ath_aow_record_txstatus(struct ieee80211com *ic, struct ath_buf *bf, struct ath_tx_status *ts)
{
    aow_update_txstatus(ic, bf, ts);
}

void ath_set_aow_latency(struct ieee80211com *ic, u_int32_t val)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);

    if (!is_aow_audio_stopped(ic)) {
       IEEE80211_AOW_DPRINTF("Device busy\n");
       return; 
    }

    if ((val < (AOW_MIN_LATENCY / 1000)) || (val > (AOW_MAX_LATENCY / 1000))) {
       IEEE80211_AOW_DPRINTF("Invalid value. Min:%ums Max:%ums\n",
                             (AOW_MIN_LATENCY / 1000),
                             (AOW_MAX_LATENCY / 1000));
       return;
    }

    sc->sc_aow.latency = val;
    sc->sc_aow.latency_us = val * 1000;

    sc->sc_rxtimeout[WME_AC_VO] = sc->sc_aow.latency -
        (ic->ic_get_aow_rx_proc_time(ic) + ATH_RXTIMEOUT_BUFFER)/1000;

    if (AOW_ES_ENAB(ic)) {
        //Reset ES
        ic->ic_set_aow_es(ic, 0);
        ic->ic_set_aow_es(ic, 1);
    } else if (AOW_ESS_ENAB(ic)) {
        //Reset ESS
        ic->ic_set_aow_ess(ic, 0);
        ic->ic_set_aow_ess(ic, 1);
    }
}

u_int32_t ath_get_aow_playlocal(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    return sc->sc_aow.playlocal;
}

u_int32_t ath_get_aow_latency(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    return sc->sc_aow.latency;
}

u_int64_t ath_get_aow_latency_us(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    return sc->sc_aow.latency_us;
}

u_int64_t
ath_get_aow_tsf_64(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    return scn->sc_ops->ath_get_tsf64(scn->sc_dev);
}    

void if_ath_start_aow_timer(struct ieee80211com *ic, u_int32_t startTime, u_int32_t period)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    scn->sc_ops->ath_start_aow_timer(scn->sc_dev, startTime, period); 
}

void if_ath_stop_aow_timer(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    scn->sc_ops->ath_stop_aow_timer(scn->sc_dev);
}

void if_ath_gpio11_toggle(struct ieee80211com *ic, u_int16_t flg)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    scn->sc_ops->ath_gpio11_toggle_ptr(scn->sc_dev, flg);
}


void if_ath_aow_proc_timer_start(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    scn->sc_ops->ath_aow_proc_timer_start(scn->sc_dev);    
}

void if_ath_aow_proc_timer_stop(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    scn->sc_ops->ath_aow_proc_timer_stop(scn->sc_dev);    
}

void if_ath_aow_proc_timer_set_period(struct ieee80211com *ic, u_int32_t period)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    scn->sc_ops->ath_aow_proc_timer_set_period( scn->sc_dev, period);
}

void ath_aow_reset(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    ath_internal_reset(sc);
}

void ath_aow_set_rx_proc_time(struct ieee80211com *ic, u_int32_t rx_proc_time)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    sc->sc_aow.rx_proc_time = rx_proc_time;
}

u_int32_t ath_aow_get_rx_proc_time(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    return sc->sc_aow.rx_proc_time;
}

extern void ieee80211_send2all_nodes(void *reqvap, void *data, int len, u_int32_t seqno, u_int64_t tsf);

void ath_aow_attach(struct ieee80211com* ic, struct ath_softc_net80211 *scn)
{
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    ic->ic_set_swretries = ath_set_swretries;
    ic->ic_get_swretries = ath_get_swretries;
    ic->ic_set_aow_rtsretries = ath_set_aow_rtsretries;
    ic->ic_get_aow_rtsretries = ath_get_aow_rtsretries;
    ic->ic_set_aow_latency = ath_set_aow_latency;
    ic->ic_get_aow_latency = ath_get_aow_latency;
    ic->ic_get_aow_latency_us = ath_get_aow_latency_us;
    ic->ic_get_aow_tsf_64 = ath_get_aow_tsf_64;
    ic->ic_start_aow_inter = if_ath_start_aow_timer;
    ic->ic_stop_aow_inter = if_ath_stop_aow_timer;
    ic->ic_gpio11_toggle = if_ath_gpio11_toggle;
    ic->ic_set_audio_channel = ath_set_audio_channel;
    ic->ic_remove_audio_channel = ath_remove_audio_channel;
    ic->ic_get_num_mapped_dst = ath_get_num_mapped_dst;
    ic->ic_get_aow_macaddr = ath_get_aow_macaddr;
    ic->ic_get_aow_chan_seqno = ath_get_aow_chan_seqno;
    ic->ic_list_audio_channel = ath_list_audio_channel;
    ic->ic_set_aow_playlocal = ath_set_aow_playlocal;
    ic->ic_get_aow_playlocal = ath_get_aow_playlocal;
    ic->ic_aow_clear_audio_channels = ath_clear_audio_channel_list;
    ic->ic_aow_proc_timer_start = if_ath_aow_proc_timer_start;
    ic->ic_aow_proc_timer_stop = if_ath_aow_proc_timer_stop;
    ic->ic_set_aow_er = ath_set_aow_er;
    ic->ic_get_aow_er = ath_get_aow_er;
    ic->ic_set_aow_ec = ath_set_aow_ec;
    ic->ic_get_aow_ec = ath_get_aow_ec;
    ic->ic_get_aow_ec_ramp = ath_get_aow_ec_ramp;
    ic->ic_set_aow_ec_ramp = ath_set_aow_ec_ramp;
    ic->ic_set_aow_ec_fmap = ath_set_aow_ec_fmap;
    ic->ic_get_aow_ec_fmap = ath_get_aow_ec_fmap;
    ic->ic_set_aow_es = ath_set_aow_es;
    ic->ic_get_aow_es = ath_get_aow_es;
    ic->ic_set_aow_as = ath_set_aow_as;
    ic->ic_get_aow_as = ath_get_aow_as;
    ic->ic_set_aow_assoc_policy = ath_set_aow_assoc_policy;
    ic->ic_get_aow_assoc_policy = ath_get_aow_assoc_policy;
    ic->ic_set_aow_audio_ch = ath_set_aow_audio_ch;
    ic->ic_get_aow_audio_ch = ath_get_aow_audio_ch;
    ic->ic_set_aow_ess = ath_set_aow_ess;
    ic->ic_get_aow_ess = ath_get_aow_ess;
    ic->ic_set_aow_ess_count = ath_set_aow_ess_count;
    ic->ic_aow_clear_estats = ath_aow_clear_estats;
    ic->ic_aow_get_estats = ath_aow_get_estats;
    ic->ic_get_aow_frame_size = ath_get_aow_frame_size;
    ic->ic_set_aow_frame_size = ath_set_aow_frame_size;
    ic->ic_get_aow_alt_setting = ath_get_aow_alt_setting;
    ic->ic_set_aow_alt_setting = ath_set_aow_alt_setting;
    ic->ic_aow_record_txstatus = ath_aow_record_txstatus;
    ic->ic_aow_reset = ath_aow_reset;
    ic->ic_aow_proc_timer_set_period = if_ath_aow_proc_timer_set_period;
    ic->ic_get_aow_tx_rate_info = ath_get_aow_tx_rate_info;
    ic->ic_set_aow_rx_proc_time = ath_aow_set_rx_proc_time;
    ic->ic_get_aow_rx_proc_time = ath_aow_get_rx_proc_time;

    sc->sc_aow.latency = DEFAULT_AOW_LATENCY_INMSEC;
    sc->sc_aow.latency_us = sc->sc_aow.latency * 1000;
    sc->sc_aow.rx_proc_time = DEFAULT_AOW_PROC_TIME;

    sc->sc_aow.as = AH_TRUE;
    sc->sc_aow.ec = AH_FALSE;
    sc->sc_aow.ec_ramp = AH_TRUE;
    sc->sc_aow.aow_assoc_only = AH_TRUE;

    ath_set_aow_rtsretries(ic, ATH_AOW_DEFAULT_RTS_RETRIES);
    
    /* Set rx processing time to safe value */
    ic->ic_set_aow_rx_proc_time(ic,
                                AOW_PKT_RX_PROC_THRESH_1_FRAME + AOW_PKT_RX_PROC_BUFFER);
    
    sc->sc_rxtimeout[WME_AC_VO] = sc->sc_aow.latency -
        (ic->ic_get_aow_rx_proc_time(ic) + ATH_RXTIMEOUT_BUFFER)/1000;
}

int32_t ath_net80211_aow_request_version(ieee80211_handle_t ieee)
{
    struct ieee80211com *ic = NET80211_HANDLE(ieee);
    return ieee80211_aow_request_version(ic);
}

int32_t ath_net80211_aow_update_ie(ieee80211_handle_t ieee, u_int8_t* data, u_int32_t len, u_int32_t action)
{
    struct ieee80211com *ic = NET80211_HANDLE(ieee);
    if (action) {
        return ieee80211_update_aow_ie(ic, data, len);
    }else{
        return ieee80211_delete_aow_ie(ic);
    }
}

int32_t ath_net80211_aow_update_volume(ieee80211_handle_t ieee, u_int8_t* data, u_int32_t len)
{
    struct ieee80211com *ic = NET80211_HANDLE(ieee);
    return ieee80211_aow_update_volume(ic, data, len);
}


void ath_net80211_aow_send2all_nodes(void *reqvap,
                                     void *data, int len, u_int32_t seqno,  u_int64_t tsf)
{
    ieee80211_send2all_nodes(reqvap, data, len, seqno, tsf);
}

u_int32_t ath_net80211_aow_tx_ctrl(u_int8_t* data, u_int32_t datalen, u_int64_t tsf)
{
    return ieee80211_aow_tx_ctrl(data, datalen, tsf);
}

u_int32_t ath_net80211_i2s_write_interrupt(ieee80211_handle_t ieee)
{
    struct ieee80211com *ic = NET80211_HANDLE(ieee);
    return ieee80211_i2s_write_interrupt(ic);
}

int ath_net80211_aow_consume_audio_data(ieee80211_handle_t ieee, u_int64_t tsf)
{
    struct ieee80211com *ic = NET80211_HANDLE(ieee);
    return ieee80211_consume_audio_data( ic, tsf);
}

int16_t ath_net80211_aow_apktindex(ieee80211_handle_t ieee, wbuf_t wbuf)
{
    struct ieee80211com *ic = NET80211_HANDLE(ieee);
    return ieee80211_aow_apktindex(ic, wbuf);
}

int32_t ath_net80211_aow_send_to_host(ieee80211_handle_t ieee, u_int8_t* data, 
                                      u_int32_t len, u_int16_t type, u_int16_t subtype, u_int8_t* addr)
{
    struct ieee80211com *ic = NET80211_HANDLE(ieee);
    return ieee80211_aow_send_to_host(ic, data, len, type, subtype, NULL);
}

void ath_net80211_aow_l2pe_record(struct ieee80211com *ic, bool is_success)
{
    aow_l2pe_record(ic, is_success);
}


u_int32_t ath_net80211_aow_chksum_crc32 (unsigned char *block, unsigned int length)
{
    return chksum_crc32(block, length);
}

u_int32_t ath_net80211_aow_cumultv_chksum_crc32(u_int32_t crc_prev,
                                       unsigned char *block,
                                       unsigned int length)
{
    return cumultv_chksum_crc32(crc_prev, block, length);
}

#endif  /* ATH_SUPPORT_AOW */
