/*
 * Copyright (c) 2010, Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */

/*
 * Public Interface for AoW control module
 */

#ifndef _DEV_ATH_AOW_H
#define _DEV_ATH_AOW_H

#if  ATH_SUPPORT_AOW

#include "ath_hwtimer.h"
#include "ieee80211_aow.h"

/* defines */

/*
 * TODO : The shared macros between the app (mplay) and
 *        the driver should be moved to a sperate header file
 */

/* Keep the below in sync with AOW_MAX_RECVRS in 
   umac/include/ieee80211_aow_shared.h */
#define AOW_MAX_RECVRS (10)   

#define ATH_AOW_MAGIC_NUMBER    0xdeadbeef
#define MAX_AOW_PKT_SIZE        840*2
#define IOCTL_AOW_DATA          (1)
#define IOCTL_AOW_CTRL          (2)
#define AOW_TSF_TIMER_OFFSET     8000
#define ATH_AOW_TIMER_INTERRUPT_INTERVAL    2000

#define ATH_RXTIMEOUT_BUFFER    (500)

#define ATH_SW_RETRY_LIMIT(_sc) (_sc->sc_aow.sw_retry_limit)
#define ATH_ENAB_AOW_ER(_sc)    (_sc->sc_aow.er)
#define ATH_ENAB_AOW_EC(_sc)    (_sc->sc_aow.ec)
#define ATH_ENAB_AOW_ES(_sc)    (_sc->sc_aow.es)
#define ATH_ENAB_AOW_ESS(_sc)   (_sc->sc_aow.ess)
#define ATH_ENAB_AS(_sc)        (_sc->sc_aow.as)

#define ATH_ENAB_AOW(_sc)       (1)

typedef enum ATH_AOW_HOST_PKT_TYPE {
    ATH_HOST_PKT_DATA,
    ATH_HOST_PKT_CTRL,
    ATH_HOST_PKT_EVENT,
} ATH_AOW_HOST_PKT_TYPE_T;    

/* data structures */

/*
 * Mapping between the AoW audio channel and respective
 * destination MAC address.
 * 
 * The following information is stored
 * - Channel 
 * - Valid Flag
 * - Respective AoW sequence number
 * - Destination MAC Address
 *
 */

typedef struct aow_chandst {
    int channel;                                        /* Logical audio channel */
    int valid;                                          /* Valid if channel set */
    int connection_id;                                  /* Similar to Assoc Id, updated by connection manager */
    unsigned int dst_cnt;                               /* Number of destination */
    unsigned int seqno;                                 /* Seq no unique to channel */
    struct ether_addr addr[AOW_MAX_RECEIVER_COUNT];     /* Destination Mac address */
}aow_chandst_t;    


typedef struct ath_aow {
    u_int    sw_retry_limit;    /* controls the software retries */
    u_int    latency;       /* controls the end2end latency */
    u_int    rx_proc_time;  /* the rx proc. time in usec - for rx_timeout*/  
    u_int8_t er;            /* error recovery feature flag */
    u_int8_t ec;            /* error concelement feature flag */
    u_int8_t ec_ramp;       /* Error concelement ramp flag */
    u_int8_t ec_fmap;       /* test handle, indicates the failmap */
    u_int    es;            /* extended statistics feature flag */
    u_int    ess;           /* extended statistics (synchronized) feature flag */
    u_int    as;            /* audio sync feature flag */
    u_int    aow_assoc_only; /* allow only AoW capable devices */
    u_int    audio_ch;      /* audio channel selector at the rx */
    
    u_int64_t latency_us;   /* latency in micro seconds */
    u_int32_t seq_no;           /* aow sequence number, map it to Node later */
    u_int32_t playlocal;    /* local playback flag */

    u_int16_t tgl_state;
    u_int16_t overflow_state;
    u_int16_t int_started;
    u_int16_t tmr_init_done;
    u_int16_t tmr_running;
    u_int32_t tmr_tsf;
    /* spin lock related */
    spinlock_t  lock;
    unsigned long lock_flags;

    /* HW timer for I2S Start */
    struct ath_gen_timer *hwtimer;
    u_int32_t timer_period;

    /* channel, mac address mapping */
    struct aow_chandst   chan_addr[AOW_MAX_AUDIO_CHANNELS];

    /* total number of channel mapped */
    int mapped_recv_cnt;
    
    /* AoW timer */
    struct ath_timer timer;

}ath_aow_t;   

typedef enum AOW_EVENT_PKT_TYPE {
    AOW_EVENT_PKT_DATA,
    AOW_EVENT_PKT_CTRL,
    AOW_EVENT_PKT_TO_HOST,
    AOW_EVENT_EVENT_PKT_TO_HOST,
    AOW_EVENT_SET_CH_ADDR_MAP,
}AOW_EVENT_PKT_TYPE_T;    

typedef enum AOW_EVENT_PKT_SUB_TYPE {
    AOW_EVENT_ADD_IE,
    AOW_EVENT_UPDATE_IE,
    AOW_EVENT_CHANGE_NW_ID,
    AOW_EVENT_DEL_IE,
    AOW_EVENT_REQ_VERSION,
    AOW_EVENT_UPDATE_VOLUME,
}AOW_EVENT_PKT_SUB_TYPE_T;    


typedef struct aow_event_pkt_hdr {
    u_int32_t signature;
    u_int32_t sequence_no;
    u_int32_t length;
    AOW_EVENT_PKT_TYPE_T type;
    AOW_EVENT_PKT_SUB_TYPE_T sub_type;
}aow_event_pkt_hdr_t;    

#define ATH_AOW_EVENT_PKT_HEADER_LEN    (sizeof(aow_event_pkt_hdr_t))

/**
 * @brief   : Template for AoW channel address Message
 */
typedef struct aow_ctrl_channel_addr_msg {
    u_int32_t channel;
    u_int32_t mode;
    u_int8_t  addr[6];
}aow_ctrl_channel_addr_msg_t;    

/* functions */

int ath_aow_send2all(ath_dev_t dev,
                     void *pkt,
                     int len,
                     u_int32_t seqno,
                     u_int64_t tsf);

int ath_aow_control(ath_dev_t dev,
                    u_int id,
                    void *indata,
                    u_int32_t insize,
                    void *outdata,
                    u_int32_t *outsize,
                    u_int32_t seqno,
                    u_int64_t tsf);

u_int32_t ath_get_aow_seqno(ath_dev_t dev);

void ath_init_ar7240_gpio_configure(void);
void ath_gpio12_toggle(ath_dev_t dev, u_int16_t flag);
void ath_hwtimer_comp_int(void *arg);
void ath_hwtimer_overflow_int(void *arg);
void ath_hwtimer_start(ath_dev_t dev, u_int32_t tsfStart, u_int32_t period);
void ath_hwtimer_stop(ath_dev_t dev);
int ath_aow_send2all(ath_dev_t dev, void *pkt, int len, u_int32_t seqno, u_int64_t tsf);
int ath_aow_control(ath_dev_t dev, u_int id,
                void *indata, u_int32_t insize,
                void *outdata, u_int32_t *outsize,
                u_int32_t seqno, u_int64_t tsf);
u_int32_t ath_aow_tx_ctrl(ath_dev_t dev,
                          u_int8_t* data,
                          u_int32_t datalen,
                          u_int64_t tsf);
u_int32_t ath_get_aow_seqno(ath_dev_t dev);
void ath_aow_proc_timer_init(struct ath_softc *sc);
void ath_aow_proc_timer_free( struct ath_softc *sc);
void ath_aow_proc_timer_start( ath_dev_t dev); 
void ath_aow_proc_timer_stop( ath_dev_t dev);
void ath_aow_proc_timer_set_period( ath_dev_t dev, u_int32_t period);
void ath_aow_init(struct ath_softc *sc);

int ath_gen_timer_attach(struct ath_softc *sc);
void ath_gen_timer_detach(struct ath_softc *sc);
void ath_gen_timer_free(struct ath_softc *sc, struct ath_gen_timer *timer);
void ath_gen_timer_start(struct ath_softc *sc, struct ath_gen_timer *timer,
                         u_int32_t timer_next, u_int32_t period);
void ath_gen_timer_stop(struct ath_softc *sc, struct ath_gen_timer *timer);
void ath_gen_timer_isr(struct ath_softc *sc);
void ath_aow_tmr_cleanup(struct ath_softc *sc);
void ath_aow_update_txstatus(struct ath_softc *sc,
                             struct ath_buf *bf,
                             struct ath_tx_status *ts);

extern int32_t ath_aow_calc_timeremaining(struct ath_softc *sc, wbuf_t wbuf);
extern u_int32_t ath_get_tx_rate_info(ath_dev_t dev, ath_node_t node);

#else   /* ATH_SUPPORT_AOW */

/*
 * Macros and functions definitions when 
 * the AOW feature is not defined
 *
 */

#define ATH_ENAB_AOW(_sc)       (0) 
#define ATH_ENAB_AOW_ER(_sc)    (0)
#define ATH_ENAB_AOW_ES(_sc)    (0)
#define ATH_ENAB_AOW_ESS(_sc)   (0)
#define ATH_ENAB_AOW_AS(_sc)    (0)
#define ATH_SW_RETRY_LIMIT(_sc) ATH_MAX_SW_RETRIES

#define ath_aow_init(a)                 do{}while(0)
#define ath_aow_tmr_attach(a)           do{}while(0)
#define ath_aow_tmr_cleanup(a)          do{}while(0)
#define ath_aow_update_txstatus(a,b,c)  do{}while(0)
#define ath_aow_proc_timer_set_period(a, b) do{}while(0)

#endif  /* ATH_SUPPORT_AOW */

#endif  /* _DEV_ATH_AOW_H */
