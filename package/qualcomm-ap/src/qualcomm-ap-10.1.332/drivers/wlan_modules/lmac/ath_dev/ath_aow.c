/*
 * Copyright (c) 2010, Atheros Communications Inc.
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
 * 
 */

#if  ATH_SUPPORT_AOW
#include "ath_internal.h"
#include "osdep.h"
#include "ratectrl.h"

#ifndef ATH_SUPPORT_AOW_GPIO
#define ATH_SUPPORT_AOW_GPIO 0
#endif

#if  ATH_SUPPORT_AOW_GPIO
#include <ar7240.h>
#endif  /* ATH_SUPPORT_AOW_GPIO */

#define AOW_TIMER_VALUE 8   /* Timer kicks in 8ms interval */

int ath_aow_tmr_func(void *sc_ptr)
{
    struct ath_softc *sc = (struct ath_softc *)sc_ptr;

    /* Process the timer data */
    sc->sc_ieee_ops->ieee80211_consume_audio_data((struct ieee80211com *)(sc->sc_ieee),
                                                   ath_hal_gettsf64(sc->sc_ah));    
    return 0;
}
                 
                 
/* Start timer to process the audio data */                 
void ath_aow_proc_timer_init(struct ath_softc *sc)
{
    sc->sc_aow.timer.active_flag = 0;
    ath_initialize_timer( sc->sc_osdev, 
                          &sc->sc_aow.timer, 
                          AOW_TIMER_VALUE, 
                          ath_aow_tmr_func,
                          (void *)sc);
}
                 
void ath_aow_proc_timer_free(struct ath_softc *sc)
{
    if ( ath_timer_is_initialized(&sc->sc_aow.timer))
    {
        ath_cancel_timer( &sc->sc_aow.timer, CANCEL_NO_SLEEP);
        ath_free_timer( &sc->sc_aow.timer );
    }
}

void ath_aow_proc_timer_set_period(ath_dev_t dev, u_int32_t period)
{
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    if (ath_timer_is_initialized(&sc->sc_aow.timer)) {
        ath_set_timer_period(&sc->sc_aow.timer, period);
    }
}
                 
void ath_aow_proc_timer_start(ath_dev_t dev) 
{
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    if ( !ath_timer_is_active( &sc->sc_aow.timer) ) {
        /* start the timer */
        ath_start_timer(&sc->sc_aow.timer);
    }
}
                 
void ath_aow_proc_timer_stop(ath_dev_t dev) 
{
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    if ( ath_timer_is_active( &sc->sc_aow.timer) ) {
        ath_cancel_timer(&sc->sc_aow.timer, CANCEL_NO_SLEEP);
    }
}

void ath_init_ar7240_gpio_configure(void)
{
#if ATH_SUPPORT_AOW_GPIO
    /* Disable JTAG */
    u_int32_t val = ar7240_reg_rd(AR7240_GPIO_FUNCTIONS);
    ar7240_reg_wr(AR7240_GPIO_FUNCTIONS, (val | 0x1));
    /* Set the GPIO 6, 7 and 8 a output */
    val = ar7240_reg_rd(AR7240_GPIO_OE);
    val |= ((1 << 6) | (1 << 7) | (1 << 8));
    ar7240_reg_wr(AR7240_GPIO_OE, val);
#endif /* ATH_SUPPORT_AOW_GPIO */   
}

void ath_gpio12_toggle(ath_dev_t dev, u_int16_t flag)
{
#if ATH_SUPPORT_AOW_GPIO    
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);

    /*
     * XXX : I2S uses some of the GPIOs so avoid
     * fiddling with them for the time being
     *
     */
    if (flag) {
        ar7240_reg_wr(AR7240_GPIO_SET, 1 << 8);    
    } else {
        ar7240_reg_wr(AR7240_GPIO_CLEAR, 1 << 8);
    }         
#endif   /* ATH_SUPPORT_AOW_GPIO */ 
}

void ath_hwtimer_comp_int(void *arg)
{
    struct ath_softc *sc = (struct ath_softc *)arg;
    u_int32_t next_period;
    ATH_AOW_IRQ_LOCK(sc);
    /* 
    * XXX : I2S uses some of the GPIOs so avoid
    * fiddling with them for the time being
    *
    */
    
    if (sc->sc_aow.tgl_state) {
#if  ATH_SUPPORT_AOW_GPIO
        ar7240_reg_wr(AR7240_GPIO_SET, 1 << 7);    
#endif  /* ATH_SUPPORT_AOW_GPIO */        
    } else {
#if  ATH_SUPPORT_AOW_GPIO
      ar7240_reg_wr(AR7240_GPIO_CLEAR, 1 << 7);
#endif  /* ATH_SUPPORT_AOW_GPIO */        
    }         
    
    sc->sc_aow.tgl_state = !sc->sc_aow.tgl_state;
    
    next_period = sc->sc_ieee_ops->ieee80211_i2s_write_interrupt((struct ieee80211com *)(sc->sc_ieee));
    if( sc->sc_aow.tmr_running ) {
        ath_gen_timer_stop(sc, sc->sc_aow.hwtimer);
        sc->sc_aow.tmr_tsf += next_period;
        ath_gen_timer_start(sc, sc->sc_aow.hwtimer, sc->sc_aow.tmr_tsf, 0);
    }
    ATH_AOW_IRQ_UNLOCK(sc);

}

void ath_hwtimer_overflow_int(void *arg)
{
    struct ath_softc *sc = (struct ath_softc *)arg;

    /* Hope it does not overflow by more then 4 ms */
    sc->sc_aow.tmr_tsf += ATH_AOW_TIMER_INTERRUPT_INTERVAL;
    ath_gen_timer_start(sc, sc->sc_aow.hwtimer, sc->sc_aow.tmr_tsf, 0);    
}

void ath_hwtimer_start(ath_dev_t dev, u_int32_t tsfStart, u_int32_t period)
{
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    /* Check if the timer has been attached */
    if (!ath_hal_hasgentimer(sc->sc_ah)) return;
    if (!sc->sc_aow.tmr_init_done) {
        sc->sc_hasgentimer = 1;  
    
        /* 
         * ATH_SUPPORT_AOW : Original call to ath_gen_timer_alloc
         * XXX : Fix the trigger of compare interrupt 
         */

        sc->sc_aow.hwtimer =  ath_gen_timer_alloc(sc,
                                           HAL_GEN_TIMER_TSF,
                                           ath_hwtimer_comp_int,
                                           ath_hwtimer_overflow_int,
                                           NULL,
                                           sc);

        
        /*
         * FIXME : AOW
         * ATH_SUPPORT_AOW : Temp hack, we pass comp_int function 
         * for both compare and overflow conditions
         * 
         * XXX : We cannot use ath_hwtimer_comp_int for overflow
         * condiftions. This has fatal effect on the I2S.
         *
         */


        if( sc->sc_aow.hwtimer == NULL ) return;
        sc->sc_aow.tmr_init_done = 1;
    } 
        
    if( !sc->sc_aow.tmr_running ) {
        sc->sc_aow.tmr_running = 1;
        ath_init_ar7240_gpio_configure();
        sc->sc_aow.tmr_tsf = tsfStart;
        ath_gen_timer_start(sc, sc->sc_aow.hwtimer, sc->sc_aow.tmr_tsf, 0);
    }
}

void ath_hwtimer_stop(ath_dev_t dev)
{
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    if( sc->sc_aow.tmr_running ) {
        ath_gen_timer_stop( sc, sc->sc_aow.hwtimer);
        sc->sc_aow.tmr_running = 0;
    }
}

/*
 * Channel set command consists of
 * - Channel Set command (4 bytes)
 * - Channel (4 bytes)
 * - MAC Address (6 bytes)
 */
#define AOW_CHAN_SET_COMMAND_SIZE (4 + 4 + 6)

int ath_aow_set_audio_channel(ath_dev_t dev, void *pkt, int len)
{
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    struct ieee80211com *ic = (struct ieee80211com*)(sc->sc_ieee);
    struct ether_addr macaddr;
    int channel;
    int command;
    int ret = AH_FALSE;

    KASSERT(pkt, ("AOW, Null Packet"));


    /*
     * Command format
     * | Command | Channel | Mac Address |
     * |    4    |    4    |    6        |
     *
     */

    if (len == AOW_CHAN_SET_COMMAND_SIZE) {
        memcpy(&command, pkt, sizeof(int));
        
        if (command == ATH_AOW_SET_CHANNEL_COMMAND) {
          memcpy(&channel, pkt + sizeof(int), sizeof(int));
          ATH_ADDR_COPY(&macaddr, pkt + (2 * sizeof(int)));
          ic->ic_set_audio_channel(ic, channel, &macaddr);
          ret = AH_TRUE;
        }
    }
    return ret;
}

int ath_aow_set_audio_ch(ath_dev_t dev, u_int32_t channel, u_int32_t mode, u_int8_t* addr)
{
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    struct ieee80211com *ic = (struct ieee80211com*)(sc->sc_ieee);
    int ret = AH_FALSE;

    if (mode) {
       ic->ic_set_audio_channel(ic, (int)channel, (struct ether_addr*)addr);
       ret = AH_TRUE;
    } else {
        ic->ic_remove_audio_channel(ic, (int)channel, (struct ether_addr*)addr);
        ret = AH_TRUE;
    }
    return ret;
}


int ath_aow_send2all(ath_dev_t dev, void *pkt, int len, u_int32_t seqno, u_int64_t tsf)
{

    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    struct ieee80211com *ic = (struct ieee80211com *)(sc->sc_ieee);
    struct ieee80211vap *vap;

    if(!pkt)
      return -1;

    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
        if (vap) {
            if (sc->sc_ieee_ops->ieee80211_send2all_nodes)
                sc->sc_ieee_ops->ieee80211_send2all_nodes(vap, pkt, len, seqno, tsf);
        }            
    }

    return 0;
}
int ath_aow_send_to_host(ath_dev_t dev, u_int8_t* indata, u_int32_t insize, u_int16_t type, u_int16_t subtype)
{
    int ret = 0;
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    struct ieee80211com *ic = (struct ieee80211com *)(sc->sc_ieee);
    ret = sc->sc_ieee_ops->ieee80211_aow_send_to_host(ic, indata, insize, type, subtype, NULL);
    return ret;
}

int ath_aow_update_ie(ath_dev_t dev, char* indata, int insize, int action)
{
    int ret = 0;
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    struct ieee80211com *ic = (struct ieee80211com *)(sc->sc_ieee);
    ret = sc->sc_ieee_ops->ieee80211_aow_update_ie(ic, indata, insize, action);
    return ret;
}

int ath_aow_request_version(ath_dev_t dev)
{
    int ret = 0;
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    struct ieee80211com *ic = (struct ieee80211com *)(sc->sc_ieee);
    ret = sc->sc_ieee_ops->ieee80211_aow_request_version(ic);
    return ret;

}

int ath_aow_update_volume(ath_dev_t dev, char* indata, int insize)
{
    int ret = 0;
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    struct ieee80211com *ic = (struct ieee80211com *)(sc->sc_ieee);
    ret = sc->sc_ieee_ops->ieee80211_aow_update_volume(ic, indata, insize);
    return ret;
}

int ath_aow_process_event_ctrl(ath_dev_t dev, char* indata, int insize)
{
    aow_event_pkt_hdr_t* h;
    u_int8_t* data;
    u_int32_t dlen;

    data = indata + ATH_AOW_EVENT_PKT_HEADER_LEN;
    dlen = insize - ATH_AOW_EVENT_PKT_HEADER_LEN;

    h = (aow_event_pkt_hdr_t*)indata;
    switch(h->sub_type) {
        case AOW_EVENT_ADD_IE:
        case AOW_EVENT_CHANGE_NW_ID:
        case AOW_EVENT_UPDATE_IE:
            ath_aow_update_ie(dev, data, dlen, 1);
            break;
        case AOW_EVENT_DEL_IE:
            ath_aow_update_ie(dev, data, dlen, 0);
            break;
        case AOW_EVENT_REQ_VERSION:
            ath_aow_request_version(dev);
            break;
        case AOW_EVENT_UPDATE_VOLUME:
            ath_aow_update_volume(dev, data, dlen);
            break;
        default:
            break;
    }

    return 0;
}

int ath_aow_process_event_data(ath_dev_t dev, char* indata, int insize, u_int32_t seqno, u_int64_t tsf)
{
    u_int8_t* pdata = indata + ATH_AOW_EVENT_PKT_HEADER_LEN;
    u_int32_t len  = insize - ATH_AOW_EVENT_PKT_HEADER_LEN;
    ath_aow_send2all(dev, pdata, len, seqno, tsf);
    return 0;
}


u_int32_t ath_get_tx_rate_info(ath_dev_t dev, ath_node_t node)
{
    struct ath_node *an = ATH_NODE(node);
    int i = 0;
    if (an) {
        for (i = 0; i < MAX_TX_RATE_TBL; i++) {
            //printk("RI[%d] : PER[%d]\n", i, an->an_rc_node->txRateCtrlViVo.state[i].per);
            //printk("RI[%d] : PER[%d]\n", i, an->an_rc_node->txRateCtrl.state[i].per);
        }
        //printk("RSSI = %d\n", an->an_rc_node->txRateCtrlViVo.rssiLast);
        //printk("RSSI = %d\n", an->an_rc_node->txRateCtrl.rssiLast);
    }
    return 0;
}

int ath_aow_control(ath_dev_t dev, u_int id,
                void *indata, u_int32_t insize,
                void *outdata, u_int32_t *outsize,
                u_int32_t seqno, u_int64_t tsf)
{
    aow_event_pkt_hdr_t* h;

    /*
     * Handle the channel settings here
     * XXX : Not the best of the ways, but serves the
     *        purpose
     *
     */

    if (insize == AOW_CHAN_SET_COMMAND_SIZE) {
        if (ath_aow_set_audio_channel(dev, indata, insize))
            return 0;
    }

    h = (aow_event_pkt_hdr_t*)indata;
    if (h->signature == ATH_AOW_MAGIC_NUMBER) {
        switch(h->type) {
            case AOW_EVENT_PKT_DATA:
                ath_aow_process_event_data(dev, indata, insize, seqno, tsf);
                break;

            case AOW_EVENT_PKT_CTRL:
                ath_aow_process_event_ctrl(dev, indata, insize);
                break;

            case AOW_EVENT_PKT_TO_HOST:
                {
                    u_int8_t* data = (u_int8_t*)indata + ATH_AOW_EVENT_PKT_HEADER_LEN;
                    u_int32_t len = insize - ATH_AOW_EVENT_PKT_HEADER_LEN;
                    ath_aow_send_to_host(dev, data, len, ATH_HOST_PKT_DATA, 0);
                }
                break;

            case AOW_EVENT_EVENT_PKT_TO_HOST:
                {
                    u_int8_t* data = (u_int8_t*)indata + ATH_AOW_EVENT_PKT_HEADER_LEN;
                    u_int32_t len = insize - ATH_AOW_EVENT_PKT_HEADER_LEN;
                    ath_aow_send_to_host(dev, data, len, ATH_HOST_PKT_EVENT, h->sub_type);
                }
                break;

            case AOW_EVENT_SET_CH_ADDR_MAP:
                {
                    u_int8_t* data = indata + ATH_AOW_EVENT_PKT_HEADER_LEN;
                    aow_ctrl_channel_addr_msg_t* m;
                    m = (aow_ctrl_channel_addr_msg_t*)data;
                    ath_aow_set_audio_ch(dev, m->channel, m->mode, m->addr);
                }
                break;

            default:
                break;
        }
        return 0;
    }

    ath_aow_send2all(dev, indata, insize, seqno, tsf);
    return 0;
}

u_int32_t ath_aow_tx_ctrl(ath_dev_t dev,
                          u_int8_t* data,
                          u_int32_t datalen,
                          u_int64_t tsf) 
{
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    
    if (sc->sc_ieee_ops->ieee80211_aow_tx_ctrl) {
        return sc->sc_ieee_ops->ieee80211_aow_tx_ctrl(data, datalen, tsf);
    } else {
        return -1;
    }
}

u_int32_t
ath_get_aow_seqno(ath_dev_t dev)
{
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    sc->sc_aow.seq_no++;
    return sc->sc_aow.seq_no;
}    

void ath_aow_init(struct ath_softc *sc)
{
    sc->sc_aow.sw_retry_limit = ATH_MAX_SW_RETRIES;
    ath_aow_proc_timer_init(sc);
    ATH_AOW_LOCK_INIT(sc);
}

void ath_aow_tmr_cleanup(struct ath_softc *sc)
{
    if (sc->sc_hasgentimer) {
        if( sc->sc_aow.tmr_init_done ) {
            if (sc->sc_aow.tmr_running) { 
                ath_gen_timer_stop(sc, sc->sc_aow.hwtimer);
            }
            ath_gen_timer_free(sc, sc->sc_aow.hwtimer);
        }
    }
    ath_aow_proc_timer_free(sc);
}

/* 
 * AoW Extended Statistics:
 * Update transmission related status information.
 */
void ath_aow_update_txstatus(struct ath_softc *sc,
                             struct ath_buf *bf,
                             struct ath_tx_status *ts)
{
    struct ieee80211com *ic = (struct ieee80211com*)(sc->sc_ieee);
    ic->ic_aow_record_txstatus(ic, bf, ts);
}

/* 
 * Calculate time remaining to deliver AoW MPDU.
 * Return values:
 * +ve value : Rx timeout in usec.
 * 0         : AoW MPDU delivery time has arrived/has been exceeded.
 * -ENOENT   : Non-AoW MPDU.
 * -EINVAL   : Invalid argument.
 */
int32_t ath_aow_calc_timeremaining(struct ath_softc *sc, wbuf_t wbuf)
{
    struct audio_pkt *apkt = NULL;
    u_int64_t cur_tsf = 0;
    int32_t time_remaining = 0;
    int16_t apktindex = -1;
    
    if (NULL == wbuf) {
        return -EINVAL;
    }

    apktindex = ATH_AOW_GET_AOWPKTINDEX(sc, wbuf);
    
    if (apktindex < 0) {
        return (int32_t)apktindex;
    }

    apkt = ATH_AOW_ACCESS_APKT(wbuf, apktindex);
    
    cur_tsf = ath_hal_gettsf64(sc->sc_ah);
    time_remaining = sc->sc_aow.latency_us -
                        (sc->sc_aow.rx_proc_time + ATH_RXTIMEOUT_BUFFER);
    time_remaining -= (cur_tsf < apkt->timestamp) ? 0:(cur_tsf - apkt->timestamp);        
    
    if (time_remaining > 0) {
        return time_remaining;
    } else {
        return 0;
    }
}

#endif /* ATH_SUPPORT_AOW */
