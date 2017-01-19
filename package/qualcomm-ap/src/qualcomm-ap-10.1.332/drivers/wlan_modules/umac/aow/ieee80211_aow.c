/*
 *  Copyright (c) 2008 Atheros Communications Inc.  All rights reserved.
 */

/*
 * IEEE80211 AoW Handler.
 */

#include <ieee80211_var.h>
#include <ieee80211_aow.h>
#include <osdep.h>
#include "if_athvar.h"
#include "ah.h"

#if  ATH_SUPPORT_AOW

#include "ieee80211_aow.h"

#if ATH_SUPPORT_AOW_DEBUG

#define AOW_MAX_TSF_COUNT   256
#define AOW_DBG_PRINT_LIMIT 300

typedef struct aow_dbg_stats {
    u_int64_t tsf[AOW_MAX_TSF_COUNT];
    u_int64_t sum;
    u_int64_t avg;
    u_int64_t min;
    u_int64_t max;
    u_int64_t pretsf;
    u_int64_t ref_p_tsf;
    u_int32_t prnt_count;
    u_int32_t index;
    u_int32_t wait;
}aow_dbg_stats_t;

struct aow_dbg_stats dbgstats;

static int aow_dbg_init(struct aow_dbg_stats* p);
static int aow_update_dbg_stats(struct aow_dbg_stats* p, u_int64_t val);

#endif  /* ATH_SUPPORT_AOW_DEBUG */

#if ATH_SUPPORT_AOW_TXSCHED

/* Codes used to indicate result of comparison of 
   two AoW wbuf staging entries. */
#define AWS_COMPR_L_GT_R   -1
#define AWS_COMPR_L_EQ_R    0
#define AWS_COMPR_L_LT_R    1

#endif /* ATH_SUPPORT_AOW_TXSCHED */

/* global defines */
aow_info_t aowinfo;


/* static function declarations */

#if ATH_SUPPORT_TX_SCHED

static int aws_comparator(aow_wbuf_staging_entry *left,
                          aow_wbuf_staging_entry *right);

static void aow_sort_aws_entries(aow_wbuf_staging_entry **aws_entry_ptr,
                                int aws_entry_count);
#endif

static void aow_update_ec_stats(struct ieee80211com* ic, int fmap);

#if ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG
static void aow_populate_advncd_txinfo(struct aow_advncd_txinfo *atxinfo,
                                       struct ath_tx_status *ts,
                                       struct ath_buf *bf,
                                       int loopcount);
#endif

static bool aow_istxsuccess(struct ieee80211com *ic,
                            struct ath_buf *bf,
                            struct ath_tx_status *ts,
                            bool is_aggr);

static int map_recvr_to_mcs_stats(struct ieee80211com *ic,
                                  struct ether_addr* recvr_addr,
                                  struct mcs_stats** mstats);

static inline int aow_l2pe_print_inst(struct ieee80211com *ic,
                                      struct l2_pkt_err_stats *l2pe_inst);

static int aow_l2pe_fifo_init(struct ieee80211com *ic);
static int aow_l2pe_fifo_reset(struct ieee80211com *ic);
static int aow_l2pe_fifo_enqueue(struct ieee80211com *ic,
                                 struct l2_pkt_err_stats *l2pe_instnce);
static int aow_l2pe_fifo_copy(struct ieee80211com *ic,
                              struct l2_pkt_err_stats *aow_l2pe_stats,
                              int *count);
static void aow_l2pe_fifo_deinit(struct ieee80211com *ic);

static inline void aow_l2peinst_init(struct l2_pkt_err_stats *l2pe_inst,
                                     u_int32_t srNo,
                                     systime_t start_time);

static inline int aow_plr_print_inst(struct ieee80211com *ic,
                                     struct pkt_lost_rate_stats *plr_inst);
static int aow_plr_fifo_init(struct ieee80211com *ic);
static int aow_plr_fifo_reset(struct ieee80211com *ic);
static int aow_plr_fifo_enqueue(struct ieee80211com *ic,
                                struct pkt_lost_rate_stats *plr_instnce);
static int aow_plr_fifo_copy(struct ieee80211com *ic,
                             struct pkt_lost_rate_stats *aow_plr_stats,
                             int *count);
static void aow_plr_fifo_deinit(struct ieee80211com *ic);
static inline void aow_plrinst_init(struct pkt_lost_rate_stats *plr_inst,
                                    u_int32_t srNo,
                                    u_int32_t seqno,
                                    u_int16_t subseqno,
                                    systime_t start_time);
static inline void aow_plrinst_update_start(struct pkt_lost_rate_stats *plr_inst,
                                            u_int32_t seqno,
                                            u_int16_t subseqno);
static inline void aow_plrinst_update_end(struct pkt_lost_rate_stats *plr_inst,
                                          u_int32_t seqno,
                                          u_int16_t subseqno);

static inline int uint_floatpt_division(const u_int32_t dividend,
                                        const u_int32_t divisor,
                                        u_int32_t *integer,
                                        u_int32_t *fraction,
                                        const u_int32_t precision_mult);

static inline long aow_compute_nummpdus(u_int16_t frame_size,
                                        u_int32_t start_seqno,
                                        u_int8_t start_subseqno,
                                        u_int32_t end_seqno,
                                        u_int8_t end_subseqno);


/* AoW device */
aow_dev_t wlan_aow_dev;
i2s_rx_dev_t *i2s;


int16_t dummy_i2s_data[AOW_I2S_WRITE_STREO_SAMPLE_SIZE*2];


audio_params_t audio_params_array[SAMP_MAX_RATE_NUM_ELEMENTS] = {
    /* SAMP_RATE_48_SAMP_SIZE_16 */
    {
        SAMP_RATE_48k_SAMP_SIZE_16, /* audio_param_id */
        48*2, /*samples_per_ms */
        16, /*sample_size_bits */
        2,  /*sample_size_bytes */      
        4,  /* sample_size_bytes_stereo */
        2000,  /* mpdu_time_slice */
        /* mpdu_size = samples_per_ms * sample_size_bytes_stereo * 
         * mpdu_time_slice/2 (in 16 bit word */
        192,
        192, /* mpdu_samples in 2 ms frame */
        ATH_AOW_NUM_MPDU, /*mpdu_per_audio_frame */
        768, /* dient_buf_size = mpdu_size * mpdu_per_audio_frame */
        768, /* sample_per_audio_frame = mpdu_size * mpdu_per_audio_frame */
        2304,/* i2s_audio_buf_size = dient_buf_size * 3, factor of 3 is based on trial */     
        12, /* i2s_write_size - 3 samples, based for correct audio sync */  
        2, /* clock_sync_drop_size is 2 16 bit words */
        3, /* clock_correction_sz -- 3 sample add or remove */
        192, /*start_mute_sz = 48*2(stereo)*2(milli seconds */
        AOW_MUTE_RAMP_DOWN_SIZE_16_BIT,  /* ramp_size  */           
    },
    /* SAMP_RATE_48_SAMP_SIZE_24 */
    {
        SAMP_RATE_48k_SAMP_SIZE_24, /* audio_param_id */
        48*2, /*samples_per_ms is */
        24, /*sample_size_bits */     
        3,  /*sample_size_bytes */      
        6,  /*sample_size_bytes_stereo */
        2000,  /* mpdu_time_slice */
        /* mpdu_size = samples_per_ms * sample_size_bytes_stereo * 
        * mpdu_time_slice/2 (in 16 bit word) */
        288,
        192, /* mpdu_samples in 2 ms frame */
        ATH_AOW_NUM_MPDU, /*mpdu_per_audio_frame */
        1152, /* dient_buf_size = mpdu_size * mpdu_per_audio_frame */
        768, /* sample_per_audio_frame = mpdu_size * mpdu_per_audio_frame */
        3456,/* i2s_audio_buf_size = dient_buf_size * 3, factor of 3 is based on trial */        
        18, /* i2s_write_size - 3 samples, based for correct audio sync */             
        3, /* clock_sync_drop_size is 3 16 bit words */
        3, /* clock_correction_sz -- 3 sample add or remove */           
        288, /*start_mute_sz = 48*1.5(3 bytes = 1.5 words/sample)*2(stereo)*2(milli seconds)*/           
        AOW_MUTE_RAMP_DOWN_SIZE_24_BIT,  /* ramp_size */           

    },
};

static u_int8_t aow_rates[] = {
    12 | IEEE80211_RATE_BASIC,
    18,
    24 | IEEE80211_RATE_BASIC,
    36,
    48|IEEE80211_RATE_BASIC,
    72,
    96,
    108
};  /* OFDM rates 6,9,12,18,24,36,48,54M */


/******************** ERROR RECOVERY FEATURE RELATED *************************/

/* crc_tab[] -- this crcTable is being build by chksum_crc32GenTab().
 *		so make sure, you call it before using the other
 *		functions!
 */
u_int32_t crc_tab[256];

/* fuction prototypes */

/* chksum_crc() -- to a given block, this one calculates the
 *				crc32-checksum until the length is
 *				reached. the crc32-checksum will be
 *				the result.
 */
u_int32_t chksum_crc32 (unsigned char *block, u_int32_t length)
{
   register unsigned long crc;
   unsigned long i;

   crc = 0xFFFFFFFF;
   for (i = 0; i < length; i++)
   {
      crc = ((crc >> 8) & 0x00FFFFFF) ^ crc_tab[(crc ^ *block++) & 0xFF];
   }
   return (crc ^ 0xFFFFFFFF);
}

/* cumultv_chksum_crc() -- Continues checksum from a previous operation of
 * chksum_crc32()
 */
u_int32_t cumultv_chksum_crc32(u_int32_t crc_prev,
                               unsigned char *block,
                               u_int32_t length)
{
   register unsigned long crc;
   unsigned long i;

   crc = crc_prev ^ 0xFFFFFFFF;
   for (i = 0; i < length; i++)
   {
      crc = ((crc >> 8) & 0x00FFFFFF) ^ crc_tab[(crc ^ *block++) & 0xFF];
   }
   return (crc ^ 0xFFFFFFFF);
}

/* chksum_crc32gentab() --      to a global crc_tab[256], this one will
 *				calculate the crcTable for crc32-checksums.
 *				it is generated to the polynom [..]
 */

void chksum_crc32gentab (void)
{
   unsigned long crc, poly;
   int i, j;

   poly = 0xEDB88320L;
   for (i = 0; i < 256; i++)
   {
      crc = i;
      for (j = 8; j > 0; j--)
      {
         if (crc & 1)
         {
            crc = (crc >> 1) ^ poly;
         }
         else
         {
            crc >>= 1;
         }
      }
      crc_tab[i] = crc;
   }
}


/******************** ERROR RECOVERY FEATURE END  *****************************/


/*
 * function : aow_i2s_init
 * --------------------------
 * Initializes the i2s device
 */

int aow_i2s_init(struct ieee80211com* ic)
{
    int ret = FALSE;

    if (!IS_I2S_OPEN(ic)) {
        if (!i2s->open()) {
            i2s->clk();
            i2s->set_dsize(ic->ic_aow.cur_audio_params->sample_size_bits); 
            SET_I2S_OPEN_STATE(ic, TRUE);
            CLR_I2S_STOP_FLAG(ic->ic_aow.i2s_flags);
            CLR_I2S_START_FLAG(ic->ic_aow.i2s_flags);
            CLR_I2S_PAUSE_FLAG(ic->ic_aow.i2s_flags);
            SET_I2S_DMA_START(ic->ic_aow.i2s_flags);
            ic->ic_aow.i2s_open_count++;
            ret = TRUE;
        }
    }
    return ret;        
}    

/*
 * function : aow_i2s_deinit
 * -------------------------
 * Resets the I2S device 
 */

int aow_i2s_deinit(struct ieee80211com* ic)
{
    
    if (IS_I2S_OPEN(ic)) {
        
        i2s->is_desc_busy(0);
        if (!i2s->close()) {
            IEEE80211_AOW_DPRINTF("I2S device close failed\n");
        }            
        CLR_I2S_STOP_FLAG(ic->ic_aow.i2s_flags);
        CLR_I2S_START_FLAG(ic->ic_aow.i2s_flags);
        CLR_I2S_DMA_START(ic->ic_aow.i2s_flags);
        SET_I2S_OPEN_STATE(ic, FALSE);
        ic->ic_aow.i2s_close_count++;
    }        

    return TRUE;
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  dbg_gpio_toggle
 *  Description:  Wrapper for toggling the gpio. Makes the code clean
 * =====================================================================================
 */

static inline void dbg_gpio_toggle(struct ieee80211com* ic, int tgl)
{
    ic->ic_gpio11_toggle(ic, tgl);
}   


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  is_aow_audio_stopped
 *  Description:  Indicates the audio stream (rx) is stopped or not
 * =====================================================================================
 */

int is_aow_audio_stopped(struct ieee80211com* ic)
{
    int status = FALSE;
    
    if ((ic->ic_aow.ctrl.aow_state == AOW_STOP_STATE) &&
        (ic->ic_aow.ctrl.aow_sub_state == AOW_STOP_RECEIVE_STATE)) {
        status = TRUE;
    }        

    return status;
}



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  copy_audio_data_from_pkt_2ms_frame
 *  Description:  This function copies the data from incoming audip packet into appropriate
 *                placeholder. The audio data arrives in induvidual packets of 4, which 
 *                when combined gives the actual 8ms worth of audio data.
 *
 *                This function also simulates the packet failure, this depends on the 
 *                user programmed ec_fmap value.
 * =====================================================================================
 */
static void copy_audio_data_from_pkt_2ms_frame(struct ieee80211com* ic,
                                     struct audio_pkt *apkt,
                                     u_int16_t rxseq,
                                     u_int64_t curTime,
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                                     u_int64_t rxTime,
#endif
                                     char* data)
{
    /* Extract the cur_info index, based on apkt sequence number(mask) */
    audio_info_1_frame *cur_info = &ic->ic_aow.info_1_frame[apkt->seqno & AUDIO_INFO_BUF_SIZE_1_FRAME_MASK];
    audio_stats *cur_stats  = &ic->ic_aow.stats;
    
    /* simcount is used to space the effect of packet drop simulation */
    u_int32_t sub_frame_idx = 0;

//    printk("Len %d packet idx %d seqno %d\n", apkt->pkt_len, apkt->seqno, apkt->seqno & AUDIO_INFO_BUF_SIZE_1_FRAME_MASK);
            
    /* Check to see if error conceal is enabled */
    ic->ic_aow.ctrl.pkt_drop_sim_cnt++;
    /* Check if the packet drop flag is set */
    if (ic->ic_get_aow_ec_fmap(ic)) {
        u_int16_t simMask = ic->ic_aow.ctrl.pkt_drop_sim_cnt & 0xff;
        u_int32_t flag = ic->ic_get_aow_ec_fmap(ic);   
        u_int16_t cnt;  
        /* Loop through to figure out if the packet is to be dropped */        
        for (cnt = 0; cnt < 4; cnt++ ) {
            if ((flag >>cnt) & 0x1) {
                if (simMask == cnt) {
                    /* This packet is to be dropped */
                    return;                    
                } 
            }
        }
    }
    /* Check if the buffer is already in use */
    if (cur_info->inUse) {
        /* Should not happen, increment error counter */
        cur_stats->mpdu_not_consumed++;
    }
    /* Set the in use flag and copy the audio info */
    cur_info->inUse     = 1;
    cur_info->startTime = apkt->timestamp;
    cur_info->seqNo     = apkt->seqno;
    cur_info->params    = apkt->params;
    cur_info->datalen   = apkt->pkt_len; 
    //cur_info->localSimCount = simcount;
    /*
    * If the AOW_NO_EC_SIMULATE simulate flag is not defined, then the
    * data is just copied onto the buffers, else the packet drop condition
    * is simulated depending upon the user programmed index
    */

    /* Mark the index */
    cur_info->foundBlocks = 1;

    /* Copy the data */
    memcpy(cur_info->audBuffer, data, sizeof( u_int16_t) * apkt->pkt_len);

    cur_info->WLANSeqNos[sub_frame_idx] = rxseq;
    cur_info->logSync[sub_frame_idx] = 
            AOW_ESS_SYNC_SET(apkt->params);

#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
    cur_info->rxTime[sub_frame_idx] = rxTime;
    cur_info->deliveryTime[sub_frame_idx] = curTime;
#endif /* ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG */

}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ieee80211_process_audio_data_2ms_frame
 *  Description:  This function is top level wrapper to copy the incoming audo data
 *                in the relevant location. Here, the frame size is assumed as 2ms
 * =====================================================================================
 */

int ieee80211_process_audio_data_2ms_frame(struct ieee80211vap *vap,  
                                 struct audio_pkt *apkt, 
                                 u_int16_t rxseq,
                                 u_int64_t curTime,
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                                 u_int64_t rxTime,
#endif 
                                 char *data)
{
    struct ieee80211com *ic = vap->iv_ic;
//    audio_info_1_frame *cur_info    = &ic->ic_aow.info_1_frame[apkt->seqno & AUDIO_INFO_BUF_SIZE_1_FRAME_MASK];
    u_int32_t sub_frame_idx = 0;
    audio_stats *cur_stats  = &ic->ic_aow.stats;
    u_int32_t air_delay = 0;
    u_int64_t aow_latency;// = ic->ic_get_aow_latency_us(ic);    /* get latency */
    KASSERT((ic != NULL), ("ic Null pointer"));

    aow_latency = ic->ic_get_aow_latency_us(ic);

    /* This is an Rx, set the Rx pointer to 1 */
    cur_stats->rxFlg = 1;
    sub_frame_idx = 0;
   
    /* get air_delay */
    if (curTime > apkt->timestamp) {
        air_delay = curTime - apkt->timestamp;
    } /* else air_delay remains zero. Rollover will need centuries
         of continuous play to happen :) So we don't waste processing
         cycles handling it. */
    
    /* 
    * Check if sequence number is already stored
    * Check if the packet came on time 
    */

    if (AOW_ES_ENAB(ic) ||
        (AOW_ESS_ENAB(ic) &&  AOW_ESS_SYNC_SET(apkt->params))) {
        aow_lh_add(ic, air_delay);
    } 

    /*
        * Check if we have enough time to process this data
        *
        * ---|-----------------|---------------|------------------
        *   T[tsf]            R[tsf]          Limit
        *   
        *   T[tsf] = Transmit side TSF
        *   R[tsf] = Receive side TSF i.e. curTsf
        *   Limit  = AoW Latency e.g 17ms
        *   P(t)   = Processing time e.g 8.5ms
        *
    */
    /* Check if we are within time */ 
    if (air_delay < (aow_latency - AOW_PKT_RX_PROC_THRESH_1_FRAME - 
        AOW_PKT_RX_PROC_BUFFER) ) {
            copy_audio_data_from_pkt_2ms_frame(ic,
                                         apkt,
                                         rxseq,
                                         curTime,
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                                         rxTime,
#endif
                                         data);
        } else {
            /* The audio packet has arrived very late, ignore it */
            cur_stats->late_mpdu++;
            if (AOW_ES_ENAB(ic)
                || (AOW_ESS_ENAB(ic) &&  AOW_ESS_SYNC_SET(apkt->params))) {
                aow_nl_send_rxpl(ic,
                                 apkt->seqno,
                                 sub_frame_idx,
                                 rxseq,
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                                 apkt->timestamp,
                                 rxTime,
                                 curTime,                           
#endif /* ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG */
                                 ATH_AOW_PL_INFO_DELAYED);

                aow_plr_record(ic,
                               apkt->seqno,
                               sub_frame_idx,
                               ATH_AOW_PLR_TYPE_LATE_MPDU);
                }                         
        }

        return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  copy_audio_data_from_pkt
 *  Description:  This function copies the data from incoming audip packet into appropriate
 *                placeholder. The audio data arrives in induvidual packets of 4, which 
 *                when combined gives the actual 8ms worth of audio data.
 *
 *                This function also simulates the packet failure, this depends on the 
 *                user programmed ec_fmap value.
 * =====================================================================================
 */
static void copy_audio_data_from_pkt(struct ieee80211com* ic,
                                     struct audio_pkt *apkt,
                                     u_int16_t rxseq,
                                     u_int64_t curTime,
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                                     u_int64_t rxTime,
#endif
                                     char* data,
                                     u_int32_t new_pkt)
{
    /* Extract the cur_info index, based on apkt sequence number(mask) */
    audio_info *cur_info        = &ic->ic_aow.info[apkt->seqno & AUDIO_INFO_BUF_SIZE_MASK];
    /* simcount is used to space the effect of packet drop simulation */
    static u_int8_t simcount    = 0;
    u_int32_t sub_frame_idx = 0;

    /* 
    * If the received frame is first of the AMPDU, 
    * initialize necessary cur_info members
    */
    if (new_pkt) {
        cur_info->inUse     = 1;
        cur_info->startTime = apkt->timestamp;
        cur_info->seqNo     = apkt->seqno;
        cur_info->params    = apkt->params;
        cur_info->datalen   = apkt->pkt_len;
        cur_info->localSimCount = simcount;
        simcount = (simcount + 1) % AOW_EC_ERROR_SIMULATION_FREQUENCY;  
    }
    sub_frame_idx = apkt->params & ATH_AOW_PARAMS_SUB_FRAME_NO_MASK;
    /*
    * If the AOW_NO_EC_SIMULATE simulate flag is not defined, then the
    * data is just copied onto the buffers, else the packet drop condition
    * is simulated depending upon the user programmed index
    */

#if  AOW_NO_EC_SIMULATE  
    /* Mark the index */
    cur_info->foundBlocks |= 1 << sub_frame_idx;

    /* Copy the data */
    memcpy(cur_info->audBuffer[sub_frame_idx], 
           data, sizeof( u_int16_t) * (apkt->pkt_len >> ATH_AOW_NUM_MPDU_LOG2));

    cur_info->WLANSeqNos[sub_frame_idx] = rxseq;
    cur_info->logSync[sub_frame_idx] = 
            AOW_ESS_SYNC_SET(apkt->params);

#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
    cur_info->rxTime[sub_frame_idx] = rxTime;
    cur_info->deliveryTime[sub_frame_idx] = curTime;
#endif /* ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG */

#else  /* AOW_NO_EC_SIMULATE */

    {
        /* Simulate packet dropped condition */

        int fmap = ic->ic_get_aow_ec_fmap(ic);

        if ((fmap & (1 << sub_frame_idx))) {
            if (cur_info->localSimCount == (AOW_EC_ERROR_SIMULATION_FREQUENCY - 1)) {
                /* Skip the packets */
                /* update the stats */
                aow_update_ec_stats(ic, fmap);
            } else {
                cur_info->foundBlocks |= 1 << sub_frame_idx;
                memcpy(cur_info->audBuffer[sub_frame_idx], 
                       data, sizeof( u_int16_t) * (apkt->pkt_len >> ATH_AOW_NUM_MPDU_LOG2));
                cur_info->WLANSeqNos[sub_frame_idx] = rxseq;
                cur_info->logSync[sub_frame_idx] = 
                        AOW_ESS_SYNC_SET(apkt->params);
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
              cur_info->rxTime[sub_frame_idx] = rxTime;
              cur_info->deliveryTime[sub_frame_idx] = curTime;
#endif /* ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG */

            }
                        
        } else {
            cur_info->foundBlocks |= 1 << sub_frame_idx;
            memcpy(cur_info->audBuffer[sub_frame_idx], 
                   data, sizeof( u_int16_t) * (apkt->pkt_len >> ATH_AOW_NUM_MPDU_LOG2));
            cur_info->WLANSeqNos[sub_frame_idx] = rxseq;
            cur_info->logSync[sub_frame_idx] = 
                    AOW_ESS_SYNC_SET(apkt->params);
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
            cur_info->rxTime[sub_frame_idx] = rxTime;
            cur_info->deliveryTime[sub_frame_idx] = curTime;
#endif /* ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG */

        }
    }
#endif  /* AOW_NO_EC_SIMULATE */
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ieee80211_process_audio_data
 *  Description:  This function is top level wrapper to copy the incoming audo data
 *                in the relevant location
 * =====================================================================================
 */

int ieee80211_process_audio_data(struct ieee80211vap *vap,  
                                 struct audio_pkt *apkt, 
                                 u_int16_t rxseq,
                                 u_int64_t curTime,
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                                 u_int64_t rxTime,
#endif 
                                 char *data)
{
    struct ieee80211com *ic = vap->iv_ic;
    u_int64_t aow_latency   = ic->ic_get_aow_latency_us(ic);    /* get latency */
    audio_info *cur_info    = &ic->ic_aow.info[apkt->seqno & AUDIO_INFO_BUF_SIZE_MASK];
    audio_stats *cur_stats  = &ic->ic_aow.stats;
    u_int32_t sub_frame_idx = 0;
    u_int32_t air_delay = 0;

    /* This is an Rx, set the Rx pointer to 1 */
    cur_stats->rxFlg = 1;
    sub_frame_idx = apkt->params & ATH_AOW_PARAMS_SUB_FRAME_NO_MASK;

    AOW_IRQ_LOCK(ic);

    /* get air_delay */
    if (curTime > apkt->timestamp) {
        air_delay = curTime - apkt->timestamp;
    } /* else air_delay remains zero. Rollover will need centuries
         of continuous play to happen :) So we don't waste processing
         cycles handling it. */

    /* 
     * Check if sequence number is already stored
     * Check if the packet came on time 
     */

    if (AOW_ES_ENAB(ic) ||
        (AOW_ESS_ENAB(ic) &&  AOW_ESS_SYNC_SET(apkt->params))) {
        aow_lh_add(ic, air_delay);
    } 

    /*
     * Check if we have enough time to process this data
     *
     * ---|-----------------|---------------|------------------
     *   T[tsf]            R[tsf]          Limit
     *   
     *   T[tsf] = Transmit side TSF
     *   R[tsf] = Receive side TSF i.e. curTsf
     *   Limit  = AoW Latency e.g 17ms
     *   P(t)   = Processing time e.g 8.5ms
     *
     */
   
    if (air_delay < (aow_latency - AOW_PKT_RX_PROC_THRESHOLD) ) {

        if (cur_info->inUse ) {
            /* Already in use, check if the sequency number matches */
            if (cur_info->seqNo == apkt->seqno ) {
               copy_audio_data_from_pkt(ic,
                                        apkt,
                                        rxseq,
                                        curTime,
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                                        rxTime,
#endif
                                        data,
                                        AOW_OLD_PKT);
            } else {
                /* The packet has not been cleared on time. Log this */
                OS_MEMSET(cur_info, 0, sizeof(audio_info));
                cur_stats->mpdu_not_consumed++;
                copy_audio_data_from_pkt(ic,
                                         apkt,
                                         rxseq,
                                         curTime,
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                                         rxTime,
#endif
                                         data,
                                         AOW_NEW_PKT);
            }
        } else {
            /* New packet received, start the receive process */
            copy_audio_data_from_pkt(ic,
                                     apkt,
                                     rxseq,
                                     curTime,
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                                     rxTime,
#endif
                                     data,
                                     AOW_NEW_PKT);
        } 
    } else {
        /* The audio packet has arrived very late, ignore it */
        cur_stats->late_mpdu++;

        if (AOW_ES_ENAB(ic)
            || (AOW_ESS_ENAB(ic) &&  AOW_ESS_SYNC_SET(apkt->params))) {
            aow_nl_send_rxpl(ic,
                             apkt->seqno,
                             sub_frame_idx,
                             rxseq,
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                             apkt->timestamp,
                             rxTime,
                             curTime,                           
#endif /* ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG */
                             ATH_AOW_PL_INFO_DELAYED);

            aow_plr_record(ic,
                           apkt->seqno,
                           sub_frame_idx,
                           ATH_AOW_PLR_TYPE_LATE_MPDU);
        }                         
    }

    AOW_IRQ_UNLOCK(ic);
    return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  aow_init_audio_sync_info
 *
 *  Description:  Initializes the audio sync related variables, called in the process
 *                process audio function, when the audio starts
 * =====================================================================================
 */
void aow_init_audio_sync_info(struct ieee80211com* ic)
{
    /* Set the audio pointer to some 2ms worth of data and set data to zero */
    ic->ic_aow.audio_data_wr_ptr = ic->ic_aow.cur_audio_params->start_mute_sz;
    
    if( ic->ic_aow.frame_size == 1) { 
        /* In case of frame size 1, the start is 4ms */
        ic->ic_aow.audio_data_wr_ptr *= 2;
    }
    
    OS_MEMZERO(ic->ic_aow.audio_data_buf, 
               sizeof(u_int16_t) * ic->ic_aow.audio_data_wr_ptr);
    ic->ic_aow.audio_data_rd_ptr = 0;
//    ic->ic_aow.sync_stats.wlan_data_buf_idx = 0;
    ic->ic_aow.sync_stats.samp_consumed = 0;
    ic->ic_aow.sync_stats.time_consumed = 0;
    ic->ic_aow.sync_stats.sec_counter   = 0;
    ic->ic_aow.sync_stats.startState = 0;
    ic->ic_aow.sync_stats.samp_to_adj = 0;
//    ic->ic_aow.sync_stats.tot_samp_adj = 0;
    ic->ic_aow.sync_stats.filt_cnt = 0;
    ic->ic_aow.sync_stats.filt_val = 0;
    ic->ic_aow.ctrl.num_missing = 0;
    ic->ic_aow.ctrl.ramp_down_set = 0;  
    ic->ic_aow.sync_stats.words_to_write = ic->ic_aow.audio_data_wr_ptr;
    /* Init pkt_drop_sim_cnt to some number other then 0,1,2,3 to
     * avoid packet drop in the first few packets 
     */
    ic->ic_aow.ctrl.pkt_drop_sim_cnt = 20; 
}

static void ieee80211_start_resume_i2s(struct ieee80211com* ic)
{
    if (ic->ic_aow.i2s_dmastart) {
        ic->ic_aow.i2s_dmastart = AH_FALSE;
        CLR_I2S_DMA_START(ic->ic_aow.i2s_flags);
        i2s->get_tx_sample_count();  // Clear the sample counter
        i2s->dma_start(0);
        ic->ic_aow.i2s_dma_start++;
    } else {
        /* 
        * The resume must to triggered, else the audio can stop
        * for some reasons, where the I2S stops when the descriptors
        * are held by the CPU
        */
        i2s->dma_resume(0);
    }
}
       
static int16_t fixed_point_16x16_res16(int16_t a, int16_t b)
{
    return (((int32_t)a) * ((int32_t)b)) >> 15;              
}
/* count_lead_zeros
 * give the number of bits to be shifted to make it left justfied 
 * 1.31 fixed point number */       
static u_int16_t count_lead_zeros(int32_t a)
{
    u_int16_t ret_val = 0;
    if (!a) return 0;
    if (a < 0) a = -a;
    while ( a >>= 1) ret_val++;    
    return 30-ret_val; 
}
/* The ramp down/up taps are from 32 points in floor(cos(pi/64:pi/64:pi/2) * 2^15) 
 * to obtain fixed point taps. The taps are then normalized to 1.15 fixed point to get
 * tap_exp and tap_val. 
 */
static u_int16_t tap_exp[AOW_MUTE_RAMP_DWN_NUM_TAPS] = { 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 3, 4, 0
};

static u_int16_t tap_val[AOW_MUTE_RAMP_DWN_NUM_TAPS] = { 
    32728, 32610, 32413, 32138, 31785, 31357, 30852, 30273, 29621, 28898, 28106, 
    27245, 26319, 25330, 24279, 23170, 22005, 20787, 19519, 18204, 16846, 30893,
    28020, 25079, 22078, 19024, 31847, 25570, 19232, 25694, 25725, 0
};
       
/* Does a 16x16 or 24x16 fixed point multiply and gives the result in 1.15 or 1.23
 * format respectivily. The input is assumed to be left justfied fixed point 1.15 
 * or 1.23. Overflow occurs in -1*-1 condition, which should not happen
 * because the taps are all positive
 */       
static int32_t mul_24_16_bit( struct ieee80211com *ic, int32_t val, u_int16_t idx)
{
    /* First exponent by counting lead zeros */
    u_int16_t exp1 = count_lead_zeros(val);
    /* Second exponent from stored tap */
    u_int16_t exp2 = tap_exp[idx];
    /* This is used to normalize the result to 24 or 16 bit, based on sample size */
    u_int16_t exp3 = (ic->ic_aow.cur_audio_params->sample_size_bits == 24) ? 7 : 15;
    u_int32_t ret_val;
    /* Left justify the first value */
    val <<= exp1;    
    /* Perform multiply and shift */
    ret_val = ((val>>16) * tap_val[idx]) >> (exp1 + exp2 + exp3);
      
    return ret_val;
}
       
/* compute_rampdown_function --
 * Computes the ramp down or ramp up using the taps provided. If the ramp_down
 * parameter is set, ramp down is performed, else ramp up
 */
static void compute_rampdown_function( struct ieee80211com *ic, 
                                       u_int16_t *inData, 
                                       u_int16_t ramp_down)
{
    /* Compute the start index. The taps are set in ramp_down order, 
     * for ramp up, the index needs to be inverted 
     */ 
    int16_t idx = ramp_down ? 0:AOW_MUTE_RAMP_DWN_NUM_TAPS-1;
    /* The traverse diriction is reverse for ramp up */
    int16_t idx_inc = ramp_down ? 1:-1;             
    u_int16_t cnt;
    int32_t val;
    u_int8_t *locPtr = (u_int8_t *)inData;
    /* This min_ext is used to set the MS Byte in the MS position of the 32 bit integer,
     * based on the sample size */
    u_int16_t min_ext = 32 - ic->ic_aow.cur_audio_params->sample_size_bits;  
    u_int16_t charCnt;  
    for (cnt=0; cnt < AOW_MUTE_RAMP_DWN_NUM_TAPS; cnt++ ) {
        val = 0;
        /* Extracts the left Mono sample. The number of char extracted is based on the sample size */
        for( charCnt = 0; charCnt < ic->ic_aow.cur_audio_params->sample_size_bytes; charCnt++ ) {
            val |= (locPtr[charCnt] << (min_ext + 8*charCnt));                         
        }
        /* Obtain the ramp down value */
        val = mul_24_16_bit(ic, val, idx);
        /* Save it back in the byte stream in the correct endiness */
        for( charCnt = 0; charCnt < ic->ic_aow.cur_audio_params->sample_size_bytes; charCnt++ ) {
            *locPtr++ = (val >> (8*charCnt))&0xff;
        }
        val=0;
        /* Extracts the right Mono sample. The number of char extracted is based on the sample size */
        for( charCnt = 0; charCnt < ic->ic_aow.cur_audio_params->sample_size_bytes; charCnt++ ) {
            val |= (locPtr[charCnt] << (min_ext + 8*charCnt));                         
        }
        /* Obtain the ramp down value */
        val = mul_24_16_bit(ic, val, idx);
        /* Save it back in the byte stream in the correct endiness */
        for( charCnt = 0; charCnt < ic->ic_aow.cur_audio_params->sample_size_bytes; charCnt++ ) {
            *locPtr++ = (val >> (8*charCnt))&0xff;
        }
        /* Get the next tap index */
        idx += idx_inc;
    }
}
       
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ieee80211_consume_audio_data_2ms_frame
 * 
 *  Description:  Part of audio receive functionality. This function consumes the audio
 *                received and processed by process function, this function will get 
 *                called every 8ms interval. If audio data is present then this function
 *                program the i2s descriptors.
 * =====================================================================================   
 */
int ieee80211_consume_audio_data_2ms_frame(struct ieee80211com *ic)
{
    u_int16_t cnt           = 0; 
    u_int16_t fndPktToPly   = 0;
    u_int16_t packet_after_miss = 0;
    u_int16_t *deintPtr     = NULL;
    audio_info_1_frame *cur_info    = NULL;
    audio_stats *cur_stats  = &ic->ic_aow.stats;
    u_int64_t curTime;
       
    /* Check if we are in the stopped condition, if so, return */
    if (AOW_GET_AUDIO_SYNC_CTRL_STATE(ic) == AOW_STOP_STATE) {
        /* Should not be here */
        IEEE80211_AOW_DPRINTF("In consume audio Returning Audio Stopped\n");
        return 0;
    }        
    /* Clear this flag to avoid spurious ramp downs */
    ic->ic_aow.ctrl.ramp_down_set = 0;
    /*
    * Form this point, the timer function for consume audio must
    * be called every 8ms interval
    *
    */
    cur_stats->consume_aud_cnt++;
    curTime = ic->ic_get_aow_tsf_64(ic);                        
    
    /* If the current packet is the first packet to be played, set the
     * last packet to one before it 
     */
    if (AOW_GET_AUDIO_SYNC_CTRL_SUB_STATE(ic) == AOW_START_RECEIVE_STATE) {
        ic->ic_aow.ctrl.last_seqNo = ic->ic_aow.ctrl.first_rx_seq - 1;
        AOW_SET_AUDIO_SYNC_CTRL_SUB_STATE(ic, AOW_STEADY_RECEIVE_STATE);
    }

    cnt = (ic->ic_aow.ctrl.last_seqNo + 1) & AUDIO_INFO_BUF_SIZE_1_FRAME_MASK;
    /* Check if in use */
    if (!ic->ic_aow.info_1_frame[cnt].inUse ) {
        cnt = AUDIO_INFO_BUF_SIZE_1_FRAME;
    }
    
    if (cnt < AUDIO_INFO_BUF_SIZE_1_FRAME) {
        /* Found a packet. Check if the we were missing packets before */    
        if (ic->ic_aow.ctrl.num_missing) {
            /* The previous packet was missing. Reset the counter */
            ic->ic_aow.ctrl.num_missing = 0;
            /* Set the packet after missing flag to do ramp up */
            packet_after_miss = 1; 
        }
        cur_info = &ic->ic_aow.info_1_frame[cnt];
        ic->ic_aow.ctrl.last_seqNo++;
        curTime = ic->ic_get_aow_tsf_64(ic);                        
        cur_stats->lastPktTime = cur_info->startTime;
        cur_stats->num_mpdus++;

         if (AOW_ES_ENAB(ic) ||
            (AOW_ESS_ENAB(ic) && cur_info->logSync[0])) {
                aow_nl_send_rxpl(ic,
                                 cur_info->seqNo,
                                 0,   
                                 cur_info->WLANSeqNos[0],
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                                 cur_info->startTime,                            
                                 cur_info->rxTime[0],                            
                                 cur_info->deliveryTime[0],                            
#endif /* ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG */
                                 ATH_AOW_PL_INFO_INTIME);
     
                aow_plr_record(ic,
                               cur_info->seqNo,
                               0,   
                               ATH_AOW_PLR_TYPE_NUM_MPDUS);
        }     

        /* Copy the data (and some debug stuff */
        {
            /* Copy the Audio data into the buffer */
            unsigned int dataPerMpdu = cur_info->datalen;
            deintPtr = ic->ic_aow.audio_data_deintBuf;
            fndPktToPly = 1;       
            
            if (ic->ic_aow.ctrl.capture_data) {
                /* Debug capture data */
                memcpy(&ic->ic_aow.stats.before_deint[0], cur_info->audBuffer, 
                        sizeof(u_int16_t)*(dataPerMpdu));
            }
            memcpy(deintPtr, cur_info->audBuffer, 
                    sizeof(u_int16_t)*(dataPerMpdu));
            if (ic->ic_aow.ctrl.capture_data) {
                memcpy(&ic->ic_aow.stats.after_deint[0],deintPtr, 
                        sizeof(u_int16_t)*dataPerMpdu);
                ic->ic_aow.ctrl.capture_data = 0;
                ic->ic_aow.ctrl.capture_done = 1;
            }
            /* Zero out the remaining data */
            if (cur_info->datalen < ic->ic_aow.cur_audio_params->mpdu_size) {
                OS_MEMSET(deintPtr + cur_info->datalen, 
                    0,  
                    sizeof(u_int16_t) * 
                    (ic->ic_aow.cur_audio_params->mpdu_size - cur_info->datalen));
            }
        }
        ic->ic_aow.playTime = cur_info->startTime + ic->ic_get_aow_latency_us(ic) - 2000;
#if 0
        if (ic->ic_aow.sync_stats.wlan_data_buf_idx < MAX_WLAN_PTR_LOG) {
            ic->ic_aow.sync_stats.wlan_wr_ptr_before[ic->ic_aow.sync_stats.wlan_data_buf_idx] = 
                    (int32_t)(curTime - cur_info->startTime);
            ic->ic_aow.sync_stats.wlan_rd_ptr_after[ic->ic_aow.sync_stats.wlan_data_buf_idx] = 
                    (int32_t)(ic->ic_aow.playTime-curTime);
            ic->ic_aow.sync_stats.wlan_data_buf_idx++;
        }
#endif
        /* Clear up the info */
        OS_MEMSET( cur_info, 0, sizeof(audio_info_1_frame));
    } else {
        /* Did not find the next packet */    
        /* Copy zero samples into the play buffer */
        /* Increment the missing sequence numbers and the next expected sequence number*/

        ic->ic_aow.ctrl.last_seqNo++;
        ic->ic_aow.ctrl.num_missing++;
        
        /* Check if too many packets have been missing */
        if (ic->ic_aow.ctrl.num_missing > AOW_MAX_MISSING_PKT_1_FRAME) {

            IEEE80211_AOW_DPRINTF("Audio Stopped\n");
            ieee80211_aow_cmd_handler(ic, ATH_AOW_RESET_COMMAND );
            AOW_SET_AUDIO_SYNC_CTRL_STATE(ic, AOW_STOP_STATE);
            AOW_SET_AUDIO_SYNC_CTRL_SUB_STATE(ic, AOW_STOP_RECEIVE_STATE);

            if (AOW_ES_ENAB(ic) || AOW_ESS_ENAB(ic)) {
                aow_l2pe_audstopped(ic);
                aow_plr_audstopped(ic);
            }

            /* The audio stopped, so this are not missing packets.
            * Remove them for the stats to maintain sanity 
            */
            ic->ic_aow.ctrl.num_missing -= AOW_MAX_MISSING_PKT_1_FRAME;    
        } else {
            fndPktToPly = 1;       
            deintPtr = ic->ic_aow.zero_data;
            ic->ic_aow.playTime += 2000;
        }
    }
    if (fndPktToPly) {
        /* Write the data to I2S buffer*/
        u_int32_t sampCnt;
        int16_t drop_size = 0;
        u_int16_t num_words_to_cp = ic->ic_aow.cur_audio_params->mpdu_size;
        u_int16_t *dest_ptr = ic->ic_aow.audio_data_buf;
        
        cur_stats->ampdu_size_short++;     
        /* Check if we need to add or drop packets */
        if (ic->ic_aow.sync_stats.samp_to_adj > AOW_SYNC_SAMPLE_ADJUST_COUNT) {
            /* Samples are to be added */
            drop_size = ic->ic_aow.cur_audio_params->clock_sync_drop_size;
        } else if (ic->ic_aow.sync_stats.samp_to_adj < -AOW_SYNC_SAMPLE_ADJUST_COUNT) {
            /* Samples are to be dropped */
            drop_size = -ic->ic_aow.cur_audio_params->clock_sync_drop_size;
        } /* Else the drop size is zero */
        
        /* Check if we need to do ramp up */
        if (packet_after_miss && AOW_EC_RAMP(ic)) {
            /* This is a packet after misssing packet, do ramp up */
            compute_rampdown_function(ic, deintPtr, 0);                             
            ic->ic_aow.ec.index0_fixed++;
        }
        /* This number is choosen so that it is a common multiple of 2,3, or 4,  
         * which is the stereo sample size for 16, 24 or 32 bits and also when multipled by 
         * three is well within the 2ms sample buffer size
         */
#define CPY_STEP_SIZE (24)
        /* Copy with/without sample drop. The source pointer is shifted forward
         * by drop size to drop sample or pushed back by drop size to repeat 
         * samples. The last copy outside the loop copies the rest of the 
         * rest of the samples. 
         */    
        for (sampCnt = 0; sampCnt < 3; sampCnt++) {
            OS_MEMCPY( dest_ptr, deintPtr, CPY_STEP_SIZE * sizeof(u_int16_t));
            dest_ptr += CPY_STEP_SIZE;
            deintPtr += (CPY_STEP_SIZE + drop_size);
        }
        /* Copy the remaining data */
        OS_MEMCPY( dest_ptr, deintPtr, 
                   (num_words_to_cp - (CPY_STEP_SIZE + drop_size)*sampCnt)* 
                           sizeof(u_int16_t));
        /* compute the number of words to be written to the I2S buffer */
        ic->ic_aow.sync_stats.words_to_write = num_words_to_cp - sampCnt*drop_size;
        /* Update the audio sync count */
        if (drop_size > 0) { 
            ic->ic_aow.sync_stats.samp_to_adj -= 6;
            ic->ic_aow.sync_stats.tot_samp_adj -=6;    
        } else if (drop_size < 0) {
            ic->ic_aow.sync_stats.samp_to_adj += 6;
            ic->ic_aow.sync_stats.tot_samp_adj +=6;    
        }
        /* Check to see if error conceal ramp is enabled */
        if (AOW_EC_RAMP(ic)) {
            /* Check if ramp down needs to be done */
            if (!ic->ic_aow.ctrl.num_missing) {
                /* This packet was not a zero filled missing packet. Check if next 
                 * packet has arrived
                 */            
                cnt = (ic->ic_aow.ctrl.last_seqNo + 1) & AUDIO_INFO_BUF_SIZE_1_FRAME_MASK;
                if (!ic->ic_aow.info_1_frame[cnt].inUse ) {
                    /* The next packet has not yet arrived 
                     * Save the buffer and do ramp down, since it is less probable that 
                     * the packet will arrive in the next 2ms  
                     */
                    dest_ptr = ic->ic_aow.audio_data_buf + ic->ic_aow.sync_stats.words_to_write
                            - ic->ic_aow.cur_audio_params->ramp_size;
                    OS_MEMCPY( ic->ic_aow.ramp_down_buf,
                            dest_ptr,
                            ic->ic_aow.cur_audio_params->ramp_size * sizeof(u_int16_t));
                    compute_rampdown_function( ic, dest_ptr, 1);
                    ic->ic_aow.ctrl.ramp_down_set = 1;    
                }
            }
        }
        
#if 0  
        /* Some debug stuff, let it be for the time being */
        if ((ic->ic_aow.sync_stats.wlan_data_buf_idx < MAX_WLAN_PTR_LOG)) {
        ic->ic_aow.sync_stats.wlan_wr_ptr_after[ic->ic_aow.sync_stats.wlan_data_buf_idx] = ic->ic_aow.audio_data_wr_ptr;
    }
#endif
    }

    return 0;
           
}       

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ieee80211_program_i2s_2ms_frame
 *              
 *  Description:  Part of audio receive function, programs the raw audio to i2s descrip
 *                -tors, takes of sample add and sample drop for audio sync function.
 * =====================================================================================
 */

static u_int32_t ieee80211_program_i2s_2ms_frame(struct ieee80211com* ic)
{
    u_int32_t i2s_status = TRUE;
    u_int64_t cur_tsf = 0;
    u_int16_t num_bytes_to_copy = ic->ic_aow.cur_audio_params->i2s_write_size;
    u_int16_t tot_bytes = ic->ic_aow.sync_stats.words_to_write * sizeof(u_int16_t); 
    u_int8_t *aud_ptr = (char *)ic->ic_aow.audio_data_buf;
    u_int16_t cnt;
    audio_stats *cur_stats  = &ic->ic_aow.stats;
    u_int32_t computed_period = ic->ic_aow.cur_audio_params->mpdu_time_slice;
    u_int16_t used_desc; 
#if 0
    /* store ptrs for debug/stats purpose */
    if ((ic->ic_aow.sync_stats.wlan_data_buf_idx < MAX_WLAN_PTR_LOG)) {
        ic->ic_aow.sync_stats.wlan_rd_ptr_before[ic->ic_aow.sync_stats.wlan_data_buf_idx] = rd_ptr;
        ic->ic_aow.sync_stats.wlan_wr_ptr_before[ic->ic_aow.sync_stats.wlan_data_buf_idx] = wr_ptr; 
    }
#endif
    /* Check if ramp down flag is set. If so, check if the next packet has arrived. If so,
     * remove the ramp down and copy the original data 
     */
    if (ic->ic_aow.ctrl.ramp_down_set) {
        cnt = (ic->ic_aow.ctrl.last_seqNo + 1) & AUDIO_INFO_BUF_SIZE_1_FRAME_MASK;
        if (ic->ic_aow.info_1_frame[cnt].inUse ) {
            /* Next packet has arrived, so copy back the original 
             * data in place of ramp down data */
            u_int16_t *dest_ptr = ic->ic_aow.audio_data_buf + ic->ic_aow.sync_stats.words_to_write
                    - ic->ic_aow.cur_audio_params->ramp_size;
            OS_MEMCPY( dest_ptr,
                       ic->ic_aow.ramp_down_buf,
                       ic->ic_aow.cur_audio_params->ramp_size*sizeof(u_int16_t));
        } else {
            /* Increment the counter for ramp down */
            cur_stats->num_missing_mpdu_hist[1]++;
        }       
    }     
    
    used_desc = i2s->get_used_desc_count(); 
    if (!used_desc) ic->ic_aow.ec.index2_fixed++;
    
    /* The number of bytes per discriptor depends on the sample size. The goal
    * is to copy 3 stereo samples. If sample size is 16, number of bytes is 
    * 2(2 bytes/sample) * 2 (Stereo)* 3 (samples), which is the default. 
    * Similar computations can be made for 24 and 32 bytes sample size and 
    * the answer is below
    */ 
    do {
        num_bytes_to_copy = (tot_bytes > num_bytes_to_copy) ?
                num_bytes_to_copy: tot_bytes;
        
        /* program i2s descriptors with audio data */
        if (!AOW_ES_ENAB(ic) && !AOW_ESS_ENAB(ic)) {   
            i2s_status = i2s->write(num_bytes_to_copy, 
                                          aud_ptr);
        } else {
            /* When ESS enabled, write zeros to I2S descriptors */
            i2s_status = i2s->write(num_bytes_to_copy,
                                          (char*)dummy_i2s_data);
        }

        /* check if we have empty i2s descriptors left */
        if (i2s_status == TRUE) {
            tot_bytes -= num_bytes_to_copy;
            aud_ptr += num_bytes_to_copy;
            if (!tot_bytes) i2s_status = FALSE; 
        } 
        else if (tot_bytes) {
            /* Only used for debug */
            cur_stats->missing_pos_cnt[0]++;
        }
        /* exit if there are no empty i2s descriptors */
    } while( i2s_status == TRUE );
    
    if (IS_I2S_NEED_DMA_START(ic->ic_aow.i2s_flags)) {
        ic->ic_aow.i2s_dmastart = TRUE;
    }

    cur_tsf = ic->ic_get_aow_tsf_64(ic);

    /* Check if we are in the stable mode */
    if (ic->ic_aow.sync_stats.startState > 2) {
        int32_t timeDiff;
        timeDiff = (int32_t)(cur_tsf - ic->ic_aow.sync_stats.prev_tsf);
        ic->ic_aow.sync_stats.time_consumed += timeDiff;

        timeDiff = (int32_t)(ic->ic_aow.playTime-cur_tsf);
        ic->ic_aow.sync_stats.filt_val += timeDiff; 
        ic->ic_aow.sync_stats.sec_counter++;  
                        
        /* 
        * the sync counter threshold is 500, corresponding to 1 second time
        * interval
        *
        */
        if (ic->ic_aow.sync_stats.sec_counter == AOW_SYNC_COUNTER_THRESHOLD_1_FRAME) {
            ic->ic_aow.sync_stats.samp_consumed = i2s->get_tx_sample_count();

            /* if the time is greater than the limit, mark underflow */
            if (ic->ic_aow.sync_stats.time_consumed > AOW_SYNC_EXPECTED_CONSUME_TIME_LIMIT)
                ic->ic_aow.sync_stats.underflow++;

            /* Since the 500 frames are used to compute the time difference, the average time
             * difference is total time differece/500, which is the same as floor(2^15/500) * 2^-15. 
             * The value in the brace is 65 and the same is implemented below 
             */
            ic->ic_aow.sync_stats.filt_val = (ic->ic_aow.sync_stats.filt_val*65)>>15; 
            /* Check if the value is within range before applying correction */
            if (ic->ic_aow.sync_stats.filt_val > AOW_SYNC_CONSUME_TIME_DELTA) {
                ic->ic_aow.sync_stats.filt_val = AOW_SYNC_CONSUME_TIME_DELTA;
            } else if (ic->ic_aow.sync_stats.filt_val < -AOW_SYNC_CONSUME_TIME_DELTA) {
                ic->ic_aow.sync_stats.filt_val = -AOW_SYNC_CONSUME_TIME_DELTA;
            }
            computed_period += ic->ic_aow.sync_stats.filt_val;
            
            if (AOW_AS_ENAB(ic)) {
                
                if (used_desc > AOW_SYNC_USED_DESC_MAX_THRES) {
                    ic->ic_aow.sync_stats.samp_to_adj += 
                        (used_desc - AOW_SYNC_USED_DESC_MAX_THRES) * AOW_SYNC_SAMPLE_ADJUST_SIZE;
                } else if (used_desc < AOW_SYNC_USED_DESC_MIN_THRES) {
                    ic->ic_aow.sync_stats.samp_to_adj -= 
                        (AOW_SYNC_USED_DESC_MIN_THRES - used_desc) * AOW_SYNC_SAMPLE_ADJUST_SIZE;
                } 
            }             
            
            if ((ic->ic_aow.sync_stats.wlan_data_buf_idx < MAX_WLAN_PTR_LOG)) {
                ic->ic_aow.sync_stats.wlan_wr_ptr_after[ic->ic_aow.sync_stats.wlan_data_buf_idx] = ic->ic_aow.sync_stats.samp_consumed; 
                ic->ic_aow.sync_stats.wlan_wr_ptr_before[ic->ic_aow.sync_stats.wlan_data_buf_idx] = ic->ic_aow.sync_stats.filt_val;
                ic->ic_aow.sync_stats.i2s_disCnt[ic->ic_aow.sync_stats.wlan_data_buf_idx] = ic->ic_aow.sync_stats.samp_to_adj;
                ic->ic_aow.sync_stats.i2s_timeDiff[ic->ic_aow.sync_stats.wlan_data_buf_idx] = ic->ic_aow.sync_stats.time_consumed; 
                ic->ic_aow.sync_stats.wlan_rd_ptr_before[ic->ic_aow.sync_stats.wlan_data_buf_idx] = used_desc*6;
                ic->ic_aow.sync_stats.wlan_data_buf_idx++;
            }
            ic->ic_aow.sync_stats.filt_val = 0;
            ic->ic_aow.sync_stats.samp_consumed = 0;
            ic->ic_aow.sync_stats.time_consumed = 0;
            ic->ic_aow.sync_stats.sec_counter   = 0;  
        }
    } else {
        /* 
        * clear the sample count register by reading it,
        * else it will give spurious value next cycle
        *
        */
        i2s->get_tx_sample_count();
    }                

//    set_aow_rd_ptr(ic, rd_ptr);
    ic->ic_aow.sync_stats.prev_tsf = cur_tsf;
    ic->ic_aow.sync_stats.startState++;
    return computed_period;
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ieee80211_i2s_write_interrupt_2ms_frame
 *
 *  Description:  This function is core or receive state machine, will get invoked at 
 *                every 2ms interval, when there is an audio traffic. The receive state
 *                machine moves from IDLE, CONSUME, I2S Program, I2S Start cycle for
 *                every 2ms interval.
 * =====================================================================================
 */
u_int32_t ieee80211_i2s_write_interrupt_2ms_frame(struct ieee80211com *ic)
{
    u_int32_t period_val = 2000;
    if (AOW_GET_AUDIO_SYNC_CTRL_STATE(ic) == AOW_STOP_STATE) return TRUE; 
    AOW_IRQ_LOCK(ic);
    /* Write the data that is there to the I2S */
//    dbg_gpio_toggle(ic, GPIO_HIGH);
#if 1
    period_val = ieee80211_program_i2s_2ms_frame(ic);
#endif
    ieee80211_start_resume_i2s(ic);
    /* Take care of the next set of data */
    ieee80211_consume_audio_data_2ms_frame(ic);
    //dbg_gpio_toggle(ic, GPIO_LOW);
    AOW_IRQ_UNLOCK(ic);
    return period_val;
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ieee80211_consume_audio_data
 * 
 *  Description:  Part of audio receive functionality. This function consumes the audio
 *                received and processed by process function, this function will get 
 *                called every 8ms interval. If audio data is present then this function
 *                program the i2s descriptors.
 * =====================================================================================
 */
int ieee80211_consume_audio_data(struct ieee80211com *ic, u_int64_t curTime )
{

    u_int16_t cnt           = 0; 
    u_int16_t fcnt          = 0;
    u_int16_t miss_cnt      = 0;
    u_int16_t fndPktToPly   = 0;
    u_int16_t *deintPtr     = NULL;
    audio_info *cur_info    = NULL;
    audio_stats *cur_stats  = &ic->ic_aow.stats;
 
    /* Check if we are in the stopped condition, if so, return */
    if (AOW_GET_AUDIO_SYNC_CTRL_STATE(ic) == AOW_STOP_STATE) {
        /* Should not be here */
        IEEE80211_AOW_DPRINTF("Returning Audio Stopped\n");
        AOW_IRQ_UNLOCK(ic);
        return 0;
    }        

    /*
     * Form this point, the timer function for consume audio must
     * be called every 8ms interval
     *
     */


    cur_stats->consume_aud_cnt++;
    curTime = ic->ic_get_aow_tsf_64(ic);                        
   
    /* Check it this is the start state */
    if (AOW_GET_AUDIO_SYNC_CTRL_SUB_STATE(ic) == AOW_START_RECEIVE_STATE) {
        /* 
        * This is the first packet after starting. 
        * Pick out the earliest 
        */
        u_int64_t oldTimeStmp = 0xffffffffffffffffULL;    
        u_int16_t minSeqIdx     = 0;

        for (cnt = 0; cnt < AUDIO_INFO_BUF_SIZE; cnt++) {
            /* 
             * This is the first sample after starting. 
             * Pick out the earliest 
             */
            cur_info = &ic->ic_aow.info[cnt];

            if (cur_info->inUse ) {
                /* If used, find if it is one with the oldest time stamp */
                if (cur_info->startTime <= oldTimeStmp ) {
                    oldTimeStmp = cur_info->startTime;
                    minSeqIdx = cnt;
                }
            }
        }
        /* count should have the next sequence number to be played */
        cnt = minSeqIdx; 
        ic->ic_aow.ctrl.last_seqNo = ic->ic_aow.info[cnt].seqNo - 1;
        AOW_SET_AUDIO_SYNC_CTRL_SUB_STATE(ic, AOW_STEADY_RECEIVE_STATE);

    } else {
        /* In the play condition, look for the next */
        for (cnt = 0; cnt < AUDIO_INFO_BUF_SIZE ; cnt++) {
            /* Check the next sequence number to be played if state is not */
            cur_info = &ic->ic_aow.info[cnt];
            if (cur_info->seqNo < (ic->ic_aow.ctrl.last_seqNo + 1)) {
                /* This should not happen. Clear it any way */
                OS_MEMSET(cur_info, 0, sizeof(audio_info));
            }
            
            if (cur_info->seqNo == (ic->ic_aow.ctrl.last_seqNo + 1)) {
                /* The next sequence number has been found  */
                break;    
            }
        }
    }

    if (cnt < AUDIO_INFO_BUF_SIZE) {

        /* Found a packet, do de-interleave, EC and all the fun stuff */    
        ic->ic_aow.ctrl.last_seqNo++;
        ic->ic_aow.ctrl.num_missing = 0;
        
        cur_info = &ic->ic_aow.info[cnt];
        curTime = ic->ic_get_aow_tsf_64(ic);                        
        cur_stats->lastPktTime = cur_info->startTime;
        cur_stats->num_ampdus++;
        
        miss_cnt = 0;
        
        for (fcnt = 0; fcnt < ATH_AOW_NUM_MPDU; fcnt++) {
            if (!(cur_info->foundBlocks & (1 << fcnt))) {   
                miss_cnt++;
                cur_stats->missing_pos_cnt[fcnt]++;
            } else {
                cur_stats->num_mpdus++;
                if (AOW_ES_ENAB(ic) ||
                    (AOW_ESS_ENAB(ic) && cur_info->logSync[fcnt])) {
                    aow_nl_send_rxpl(ic,
                                     cur_info->seqNo,
                                     fcnt,
                                     cur_info->WLANSeqNos[fcnt],
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                                     cur_info->startTime,                            
                                     cur_info->rxTime[fcnt],                            
                                     cur_info->deliveryTime[fcnt],                            
#endif /* ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG */
                                     ATH_AOW_PL_INFO_INTIME);
                            
                    aow_plr_record(ic,
                                   cur_info->seqNo,
                                   fcnt,
                                   ATH_AOW_PLR_TYPE_NUM_MPDUS);
                    }                         
            }
        }
                
        cur_stats->num_missing_mpdu_hist[miss_cnt]++;
        cur_stats->num_missing_mpdu += miss_cnt;
        cur_stats->datalenCnt++;

        {
            /* Copy the Audio data into the buffer */
            unsigned int samp_cnt;
            deintPtr = ic->ic_aow.audio_data_deintBuf;
            fndPktToPly = 1;       

            /* pass through the error concealment feature */
            if (AOW_EC_ENAB(ic)) {
                aow_ec_handler(ic, cur_info);
            }
                        

            /* Check if deinterleave has to be done */
            if ((cur_info->params >> ATH_AOW_PARAMS_INTRPOL_ON_FLG_S) & ATH_AOW_PARAMS_INTRPOL_ON_FLG_MASK) {

                /* Interleaving is on, deinterleave the packets */
                for (fcnt = 0; fcnt < cur_info->datalen; fcnt += 2 ) {
                    samp_cnt = fcnt >> 1; /* This is for the left and the right sample */
                    deintPtr[fcnt] = cur_info->audBuffer[samp_cnt & ATH_AOW_MPDU_MOD_MASK][ ((samp_cnt >> ATH_AOW_NUM_MPDU_LOG2) << 1)];
                    deintPtr[fcnt+1] = cur_info->audBuffer[samp_cnt & ATH_AOW_MPDU_MOD_MASK][ ((samp_cnt >> ATH_AOW_NUM_MPDU_LOG2) << 1) + 1];
                }

            } else { /* No interleaving case, just copy the buffers */

                unsigned int dataPerMpdu = cur_info->datalen >> ATH_AOW_NUM_MPDU_LOG2;

                {
                    for (fcnt = 0; fcnt < ATH_AOW_NUM_MPDU; fcnt++) {
                        if (ic->ic_aow.ctrl.capture_data) {
                            memcpy(&ic->ic_aow.stats.before_deint[fcnt], cur_info->audBuffer[fcnt], 
                                sizeof(u_int16_t)*(dataPerMpdu));
                        }
                        memcpy(&deintPtr[dataPerMpdu*fcnt], cur_info->audBuffer[fcnt], 
                                sizeof(u_int16_t)*(dataPerMpdu));
                        
                    }
                    if (ic->ic_aow.ctrl.capture_data) {
                        memcpy(&ic->ic_aow.stats.after_deint[0],&deintPtr[0], 
                            sizeof(u_int16_t)*(dataPerMpdu*ATH_AOW_NUM_MPDU));
                        ic->ic_aow.ctrl.capture_data = 0;
                        ic->ic_aow.ctrl.capture_done = 1;
                    }

                }
            }

            if (cur_info->datalen < ATH_AOW_NUM_SAMP_PER_AMPDU) {
                OS_MEMSET(deintPtr + cur_info->datalen, 0,  ic->ic_aow.cur_audio_params->sample_size_bytes * (ATH_AOW_NUM_SAMP_PER_AMPDU - cur_info->datalen));
            }
        }

        /* Clear up the info */
        OS_MEMSET( cur_info, 0, sizeof(audio_info));
    } else {
        /* Did not find the next packet */    
        /* Copy zero samples into the play buffer */
        /* Increment the missing sequence numbers and the next expected sequence number*/

        ic->ic_aow.ctrl.last_seqNo++;
        ic->ic_aow.ctrl.num_missing++;

        /* Check if too many packets have been missing */
        if (ic->ic_aow.ctrl.num_missing > AOW_MAX_MISSING_PKT) {

            IEEE80211_AOW_DPRINTF("Audio Stopped\n");
            ieee80211_aow_cmd_handler(ic, ATH_AOW_RESET_COMMAND );
            AOW_SET_AUDIO_SYNC_CTRL_STATE(ic, AOW_STOP_STATE);
            AOW_SET_AUDIO_SYNC_CTRL_SUB_STATE(ic, AOW_STOP_RECEIVE_STATE);

            if (AOW_ES_ENAB(ic) || AOW_ESS_ENAB(ic)) {
                aow_l2pe_audstopped(ic);
                aow_plr_audstopped(ic);
            }

            /* The audio stopped, so this are not missing packets.
             * Remove them for the stats to maintain sanity 
             */
            ic->ic_aow.ctrl.num_missing -= AOW_MAX_MISSING_PKT;    
        } else {
            fndPktToPly = 1;       
            deintPtr = ic->ic_aow.zero_data;
        }
    }
    if (fndPktToPly) {
        /* Write the data to I2S */
        u_int32_t sampCnt;
        u_int32_t wr_ptr = ic->ic_aow.audio_data_wr_ptr;
        u_int32_t rd_ptr = ic->ic_aow.audio_data_rd_ptr;
        u_int16_t drop_size = ic->ic_aow.cur_audio_params->clock_sync_drop_size;
        u_int16_t num_words_to_cp = ic->ic_aow.cur_audio_params->dient_buf_size;
        cur_stats->ampdu_size_short++;     
        
        if (ic->ic_aow.sync_stats.samp_to_adj > AOW_SYNC_SAMPLE_ADJUST_COUNT) {
            /* There are extra samples. Move the read pointer ahead */
            /* The I2S clock is slower, drop 3 samples */
            /* In case of 16 bit, the sample size is 2 bytes, in case of 24 
             * bits, sample size is 3 bytes, in case 32 bits, sample size is
             * 4 bytes. Assuming stereo channel, the number
             * of bytes to drop gets doubled, which is what is set in the 
             * drop_size variable 
             */
                    
            for (sampCnt = 0; sampCnt < num_words_to_cp; sampCnt += drop_size) {
                if ((sampCnt == 60) ||
                    (sampCnt == 120)||
                    (sampCnt == 240)) 
                {
                    /* Skip copying this sample */
                    continue;
                }
                ic->ic_aow.audio_data_buf[wr_ptr++] =  deintPtr[sampCnt];
                ic->ic_aow.audio_data_buf[wr_ptr++] =  deintPtr[sampCnt+1];
        
                if (drop_size > 2) { /* 24 bit sample size */
                    ic->ic_aow.audio_data_buf[wr_ptr++] =  deintPtr[sampCnt+2];
                } 
                if (wr_ptr >= AUDIO_WLAN_BUF_LEN) 
                    wr_ptr = 0;
    
                if (wr_ptr == rd_ptr) {
                    ic->ic_aow.sync_stats.overflow++;
                    ic->ic_aow.sync_stats.i2s_sample_dropped_count++;
                }
            }
            ic->ic_aow.sync_stats.samp_to_adj -= 6;            
        } else if (ic->ic_aow.sync_stats.samp_to_adj < -AOW_SYNC_SAMPLE_ADJUST_COUNT) {
            /* The clock is too fast. Repeat a few samples */
            for (sampCnt = 0; sampCnt < num_words_to_cp; sampCnt += drop_size) {
                if ((sampCnt == 60)||
                     (sampCnt == 120)||
                     (sampCnt == 240)) 
                {
                    /* Copy the previous incoming sample again */
                    if (drop_size > 2) {  /* 24 bit case */
                        ic->ic_aow.audio_data_buf[wr_ptr++] =  deintPtr[sampCnt-3];
                    } 
                    ic->ic_aow.audio_data_buf[wr_ptr++] =  deintPtr[sampCnt-2];
                    ic->ic_aow.audio_data_buf[wr_ptr++] =  deintPtr[sampCnt-1];
    
                    if (wr_ptr >= AUDIO_WLAN_BUF_LEN) 
                        wr_ptr = 0;
    
                    if (wr_ptr == rd_ptr) {
                        ic->ic_aow.sync_stats.overflow++;
                        ic->ic_aow.sync_stats.i2s_sample_dropped_count++;
                    }
                    
                }
                
                ic->ic_aow.audio_data_buf[wr_ptr++] =  deintPtr[sampCnt];
                ic->ic_aow.audio_data_buf[wr_ptr++] =  deintPtr[sampCnt+1];
                if (drop_size > 2) {
                    ic->ic_aow.audio_data_buf[wr_ptr++] =  deintPtr[sampCnt+2];
                } 
    
                if (wr_ptr >= AUDIO_WLAN_BUF_LEN) 
                    wr_ptr = 0;
    
                if (wr_ptr == rd_ptr) {
                    ic->ic_aow.sync_stats.overflow++;
                    ic->ic_aow.sync_stats.i2s_sample_dropped_count++;
                }
            }
            ic->ic_aow.sync_stats.samp_to_adj += 6;            
        } else { /* Things are OK, just copy the data */
            for (sampCnt = 0; sampCnt < num_words_to_cp; sampCnt += drop_size) {
                ic->ic_aow.audio_data_buf[wr_ptr++] =  deintPtr[sampCnt];
                ic->ic_aow.audio_data_buf[wr_ptr++] =  deintPtr[sampCnt+1];
                if (drop_size > 2) {
                    ic->ic_aow.audio_data_buf[wr_ptr++] =  deintPtr[sampCnt+2];
                } 
    
                if (wr_ptr >= AUDIO_WLAN_BUF_LEN) 
                    wr_ptr = 0;
    
                if (wr_ptr == rd_ptr) {
                    ic->ic_aow.sync_stats.overflow++;
                    ic->ic_aow.sync_stats.i2s_sample_dropped_count++;
                }
            }
        }
        ic->ic_aow.audio_data_wr_ptr = wr_ptr;
        if (IS_I2S_NEED_DMA_START(ic->ic_aow.i2s_flags)) {
            ic->ic_aow.i2s_dmastart = TRUE;
        }
#if 0
        if ((ic->ic_aow.sync_stats.wlan_data_buf_idx < MAX_WLAN_PTR_LOG)) {
            ic->ic_aow.sync_stats.wlan_wr_ptr_after[ic->ic_aow.sync_stats.wlan_data_buf_idx] = ic->ic_aow.audio_data_wr_ptr;
        }
#endif
    }

    return 0;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ieee80211_program_i2s
 *              
 *  Description:  Part of audio receive function, programs the raw audio to i2s descrip
 *                -tors, takes of sample add and sample drop for audio sync function.
 * =====================================================================================
 */

static void ieee80211_program_i2s(struct ieee80211com* ic)
{
    int32_t time_correction;
    u_int32_t i2s_status = 0;
    u_int32_t wr_ptr = 0;
    u_int32_t rd_ptr = 0;
    u_int32_t rd_ptr_bkup = 0;
    u_int64_t cur_tsf = 0;

    u_int16_t num_bytes_to_copy = ic->ic_aow.cur_audio_params->i2s_write_size;

    AOW_SET_AUDIO_SYNC_CTRL_STATE(ic, AOW_I2S_START_STATE);
    wr_ptr = get_aow_wr_ptr(ic);
    rd_ptr = get_aow_rd_ptr(ic);
    i2s_status = TRUE;

  
    if (rd_ptr == wr_ptr) 
        return;
    /* The number of bytes per discriptor depends on the sample size. The goal
     * is to copy 3 stereo samples. If sample size is 16, number of bytes is 
     * 2(2 bytes/sample) * 2 (Stereo)* 3 (samples), which is the default. 
     * Similar computations can be made for 24 and 32 bytes sample size and 
     * the answer is below
     */ 

    /* store ptrs for debug/stats purpose */
    if ((ic->ic_aow.sync_stats.wlan_data_buf_idx < MAX_WLAN_PTR_LOG)) {
        ic->ic_aow.sync_stats.wlan_rd_ptr_before[ic->ic_aow.sync_stats.wlan_data_buf_idx] = rd_ptr;
        ic->ic_aow.sync_stats.wlan_wr_ptr_before[ic->ic_aow.sync_stats.wlan_data_buf_idx] = wr_ptr; 
    }

    /* 
     * Program the received audio data, to the i2s descriptor, till all
     * i2s descriptors are full.
     */

    do {
        /* program i2s descriptors with audio data */
        if (!AOW_ES_ENAB(ic) && !AOW_ESS_ENAB(ic)) {   
            i2s_status = i2s->write(num_bytes_to_copy, 
                                (char*)&ic->ic_aow.audio_data_buf[rd_ptr]);
        } else {
            /* When ESS enabled, write zeros to I2S descriptors */
            i2s_status = i2s->write(num_bytes_to_copy,
                               (char*)dummy_i2s_data);
        }

        
        /* check if we have empty i2s descriptors left */
        if (i2s_status == TRUE) {
            rd_ptr_bkup = rd_ptr;   /* backup for sample repeat */
            rd_ptr += AOW_AUDIO_SYNC_SAMPLE_CORRECTION_COUNT;
            if (ic->ic_aow.cur_audio_params->sample_size_bits == 24) rd_ptr+= 3;
            if (ic->ic_aow.cur_audio_params->sample_size_bits == 32) rd_ptr+= 3;
            
            if (rd_ptr == AUDIO_WLAN_BUF_LEN )
                rd_ptr -= AUDIO_WLAN_BUF_LEN;

            /* 
             * if we have no data and i2s still have empty descriptor, the
             * repeat the previous audio sample, the repeat sample size is
             * 6 streo sample
             *
             */
            if ((rd_ptr == wr_ptr ) && (ic->ic_aow.sync_stats.startState)) {
                rd_ptr = rd_ptr_bkup;
                ic->ic_aow.sync_stats.i2s_sample_repeat_count++;
            }
        }

    /* exit if there are no empty i2s descriptors */
    } while( i2s_status == TRUE );

    cur_tsf = ic->ic_get_aow_tsf_64(ic);

    /* TODO : Add separate audio sync ioctl */
    if ((ic->ic_aow.sync_stats.startState > 2)  && AOW_AS_ENAB(ic)) {
        u_int32_t timeDiff;
        timeDiff = (u_int32_t)(cur_tsf - ic->ic_aow.sync_stats.prev_tsf);

        ic->ic_aow.sync_stats.time_consumed += timeDiff;
        ic->ic_aow.sync_stats.sec_counter++;  

        /* 
         * the sync counter threshold is 125, corresponding to 1 second time
         * interval
         *
         */
        if (ic->ic_aow.sync_stats.sec_counter == AOW_SYNC_COUNTER_THRESHOLD) {
            u_int32_t com_consumed;
            ic->ic_aow.sync_stats.samp_consumed = i2s->get_tx_sample_count();

            /* the expected consumed count is 768 * 125 */
            com_consumed = AOW_SYNC_EXPECTED_CONSUMED_SAMPLE_COUNT;

            /* if the time is greater than the limit, mark underflow */
            if (ic->ic_aow.sync_stats.time_consumed > AOW_SYNC_EXPECTED_CONSUME_TIME_LIMIT)
                ic->ic_aow.sync_stats.underflow++;

            time_correction = (ic->ic_aow.sync_stats.time_consumed - AOW_SYNC_EXPECTED_CONSUME_TIME_LOW_WM);

            /*
             * The time to correct is with in the band of 200ms 
             * ceil the value for time to correct
             */
            if (time_correction < AOW_SYNC_CONSUME_TIME_DELTA)
                time_correction = time_correction + AOW_SYNC_CORRECTION_FACTOR;
           
            time_correction /= AOW_SYNC_CORRECTION_TIME_UNIT;

            if (time_correction < 0)
                time_correction = 0;
            else if (time_correction > AOW_SYNC_CORRECTION_TIME_UNIT)
                time_correction = AOW_SYNC_CORRECTION_TIME_UNIT;

            time_correction -= 10;

            /* the calculated time for correction is for mono samples,
             * this should be multipled by two for stereo samples
             */
            ic->ic_aow.sync_stats.samp_to_adj += 
                    (int32_t)(com_consumed - ic->ic_aow.sync_stats.samp_consumed + (time_correction << 1));
            
            /* Clip the correction factor so that it does not go out of hand */
            if (ic->ic_aow.sync_stats.samp_to_adj > AOW_SYNC_SAMPLE_ADJUST_LIMIT) {
                ic->ic_aow.sync_stats.samp_to_adj = AOW_SYNC_SAMPLE_ADJUST_LIMIT;
            }

            if (ic->ic_aow.sync_stats.samp_to_adj < -AOW_SYNC_SAMPLE_ADJUST_LIMIT) {
                ic->ic_aow.sync_stats.samp_to_adj = -AOW_SYNC_SAMPLE_ADJUST_LIMIT;                          
            }

            if ((ic->ic_aow.sync_stats.wlan_data_buf_idx < MAX_WLAN_PTR_LOG)) {
                ic->ic_aow.sync_stats.wlan_wr_ptr_after[ic->ic_aow.sync_stats.wlan_data_buf_idx] = ic->ic_aow.sync_stats.samp_consumed; 
                ic->ic_aow.sync_stats.i2s_disCnt[ic->ic_aow.sync_stats.wlan_data_buf_idx] = ic->ic_aow.sync_stats.samp_to_adj;
                ic->ic_aow.sync_stats.i2s_timeDiff[ic->ic_aow.sync_stats.wlan_data_buf_idx] = ic->ic_aow.sync_stats.time_consumed; 
                ic->ic_aow.sync_stats.wlan_rd_ptr_after[ic->ic_aow.sync_stats.wlan_data_buf_idx] = rd_ptr;
            }

            ic->ic_aow.sync_stats.samp_consumed = 0;
            ic->ic_aow.sync_stats.time_consumed = 0;
            ic->ic_aow.sync_stats.sec_counter   = 0;  
            ic->ic_aow.sync_stats.wlan_data_buf_idx++;
        }
    } else {
        /* 
         * clear the sample count register by reading it,
         * else it will give spurious value next cycle
         *
         */
        i2s->get_tx_sample_count();
    }                

    set_aow_rd_ptr(ic, rd_ptr);
    ic->ic_aow.sync_stats.prev_tsf = cur_tsf;
    ic->ic_aow.sync_stats.startState++;
}


#define MY_DEBUG_LOG_IDX (1)
#define AOW_LOG_IDX2CPTR (3000)


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ieee80211_i2s_write_interrupt
 *
 *  Description:  This function is core or receive state machine, will get invoked at 
 *                every 2ms interval, when there is an audio traffic. The receive state
 *                machine moves from IDLE, CONSUME, I2S Program, I2S Start cycle for
 *                every 2ms interval.
 * =====================================================================================
 */
u_int32_t ieee80211_i2s_write_interrupt(struct ieee80211com *ic)
{
#if 1
    if (ic->ic_aow.frame_size == 1) {
        return ieee80211_i2s_write_interrupt_2ms_frame(ic);
    } else {
        AOW_IRQ_LOCK(ic);
        switch(AOW_GET_AUDIO_SYNC_CTRL_STATE(ic)) {
    
            case AOW_STOP_STATE:
                /* do nothing */
                break;
    
            case AOW_IDLE_STATE:
                dbg_gpio_toggle(ic, GPIO_HIGH);
                AOW_SET_AUDIO_SYNC_CTRL_STATE(ic, AOW_CONSUME_STATE);
                break;
    
            case AOW_CONSUME_STATE:
                dbg_gpio_toggle(ic, GPIO_LOW);
                AOW_SET_AUDIO_SYNC_CTRL_STATE(ic, AOW_I2S_PROGRAM_STATE);
                ieee80211_consume_audio_data(ic, ic->ic_get_aow_tsf_64(ic));
                break;
    
            case AOW_I2S_PROGRAM_STATE:
                dbg_gpio_toggle(ic, GPIO_HIGH);
                ieee80211_program_i2s(ic);
                break;
    
            case AOW_I2S_START_STATE:
                dbg_gpio_toggle(ic, GPIO_LOW);
                AOW_SET_AUDIO_SYNC_CTRL_STATE(ic, AOW_IDLE_STATE);
                ieee80211_start_resume_i2s(ic);
                break;
            default:
                break;
        }            
    }
    AOW_IRQ_UNLOCK(ic);
    return 2000;
#endif
}


void ieee80211_set_audio_data_capture(struct ieee80211com *ic)
{
    audio_stats *cur_stats = &ic->ic_aow.stats;
    ic->ic_aow.ctrl.capture_done = 0;    
    ic->ic_aow.stats.dbgCnt = 0;
    ic->ic_aow.stats.prevTsf = 0;        
    
    /* Zero the memory */
    OS_MEMZERO(cur_stats->before_deint[0], ATH_AOW_NUM_SAMP_PER_MPDU*2*sizeof(int16_t));
    OS_MEMZERO(cur_stats->before_deint[1], ATH_AOW_NUM_SAMP_PER_MPDU*2*sizeof(int16_t));
    OS_MEMZERO(cur_stats->before_deint[2], ATH_AOW_NUM_SAMP_PER_MPDU*2*sizeof(int16_t));
    OS_MEMZERO(cur_stats->before_deint[3], ATH_AOW_NUM_SAMP_PER_MPDU*2*sizeof(int16_t));
    OS_MEMZERO(cur_stats->after_deint, ATH_AOW_NUM_SAMP_PER_AMPDU*2*sizeof(int16_t));
    ic->ic_aow.ctrl.capture_data = 1;
}

void ieee80211_set_force_aow_data(struct ieee80211com *ic, int32_t flg)
{
    ic->ic_aow.ctrl.force_input = !!flg;        
}


void ieee80211_audio_print_capture_data(struct ieee80211com *ic)
{
    audio_stats *cur_stats = &ic->ic_aow.stats;
    u_int32_t cnt,sub_frm_cnt;

    AOW_LOCK(ic);

    if (ic->ic_aow.ctrl.capture_done ) {
        if (cur_stats->rxFlg ) {
            IEEE80211_AOW_DPRINTF("Before De-Interleave\n");
            for ( sub_frm_cnt = 0; sub_frm_cnt < ATH_AOW_NUM_MPDU; sub_frm_cnt++) {
                IEEE80211_AOW_DPRINTF("\n***Subframe %d data\n", sub_frm_cnt);
                for (cnt = 0; cnt < ATH_AOW_NUM_SAMP_PER_MPDU/4; cnt++ ) {
                    IEEE80211_AOW_DPRINTF(" Row %d 0x%04x   0x%04x     0x%04x   0x%04x     0x%04x   0x%04x      0x%04x   0x%04x\n",
                        cnt*8+1,
                        cur_stats->before_deint[sub_frm_cnt][cnt*8],                
                        cur_stats->before_deint[sub_frm_cnt][cnt*8+1],                
                        cur_stats->before_deint[sub_frm_cnt][cnt*8+2],                
                        cur_stats->before_deint[sub_frm_cnt][cnt*8+3],                
                        cur_stats->before_deint[sub_frm_cnt][cnt*8+4],                
                        cur_stats->before_deint[sub_frm_cnt][cnt*8+5],                
                        cur_stats->before_deint[sub_frm_cnt][cnt*8+6],                
                        cur_stats->before_deint[sub_frm_cnt][cnt*8+7]);                
                }
            }
            IEEE80211_AOW_DPRINTF("\n\n***After de-interleave, samples 0(L/R) 1(L/R) 2(L/R) 3(L/R)\n                             4(L/R) 5(L/R) 6(L/R) 7(L/R)....\n");
            for (cnt = 0; cnt < ATH_AOW_NUM_SAMP_PER_AMPDU/4; cnt++ ) {
                IEEE80211_AOW_DPRINTF(" Row %d 0x%04x   0x%04x     0x%04x   0x%04x     0x%04x   0x%04x      0x%04x   0x%04x\n",
                        cnt*8+1,
                        cur_stats->after_deint[cnt*8],
                        cur_stats->after_deint[cnt*8+1],
                        cur_stats->after_deint[cnt*8+2],
                        cur_stats->after_deint[cnt*8+3],
                        cur_stats->after_deint[cnt*8+4],
                        cur_stats->after_deint[cnt*8+5],
                        cur_stats->after_deint[cnt*8+6],
                        cur_stats->after_deint[cnt*8+7]);
            }
        } else {
            IEEE80211_AOW_DPRINTF("Last few time stamps starting %d\n", cur_stats->dbgCnt); 
            for (cnt = 0; cnt < AOW_DBG_WND_LEN/4; cnt++)
                IEEE80211_AOW_DPRINTF(" tsfDiff[%d] = %5d tsfDiff[%d] = %5d tsfDiff[%d] = %5d tsfDiff[%d] = %5d\n", 
                       cnt * 4, cur_stats->datalenBuf[cnt*4],
                       cnt * 4+1, cur_stats->datalenBuf[cnt*4+1],
                       cnt * 4+2, cur_stats->datalenBuf[cnt*4+2],
                       cnt * 4+3, cur_stats->datalenBuf[cnt*4+3]
                      );    
            
        }
        ic->ic_aow.ctrl.capture_done = 0;
    } else {
        IEEE80211_AOW_DPRINTF("Data capture not done\n");    
    }

    AOW_UNLOCK(ic);
}


void ieee80211_audio_stat_print(struct ieee80211com *ic)
{
    u_int16_t cnt;
    audio_stats *cur_stats = &ic->ic_aow.stats;

    AOW_LOCK(ic);

    if (cur_stats->rxFlg ) {
        /* Print the receive side data */
        IEEE80211_AOW_DPRINTF("\nAoW Statistics\n");
        IEEE80211_AOW_DPRINTF("--------------------------------------------\n");
        IEEE80211_AOW_DPRINTF("Number of times stats function called %d and aborted %d\n",
               cur_stats->consume_aud_cnt ,cur_stats->consume_aud_abort ); 
        IEEE80211_AOW_DPRINTF("Number of AMPDUs received                = %d\n", cur_stats->num_ampdus);
        IEEE80211_AOW_DPRINTF("Number of MPDUs received without fail    = %d\n", cur_stats->num_mpdus);
        IEEE80211_AOW_DPRINTF("Number of Missing/corrupted MPDUs        = %d\n", cur_stats->num_missing_mpdu);
        
        IEEE80211_AOW_DPRINTF("\nNumber of MPDUs missing per received AMPDU\n");
        IEEE80211_AOW_DPRINTF("--------------------------------------------\n");
        for ( cnt = 0; cnt < ATH_AOW_NUM_MPDU; cnt++) {
            IEEE80211_AOW_DPRINTF(" %d  MPDUs missing   = %d times\n", cnt, cur_stats->num_missing_mpdu_hist[cnt]); 
        }
        IEEE80211_AOW_DPRINTF("\nPosition of MPDUs missing\n");
        IEEE80211_AOW_DPRINTF("--------------------------------------------\n");
        for (cnt = 0; cnt < ATH_AOW_NUM_MPDU; cnt++) {
            IEEE80211_AOW_DPRINTF(" %d  pos  MPDUs missing %d times\n", cnt+1, cur_stats->missing_pos_cnt[cnt]); 
        }
        IEEE80211_AOW_DPRINTF("\n");
        IEEE80211_AOW_DPRINTF("Number of MPDUs that came late       = %d\n", cur_stats->late_mpdu);
        IEEE80211_AOW_DPRINTF("MPDU not cleared on time             = %d\n", cur_stats->mpdu_not_consumed);
        IEEE80211_AOW_DPRINTF("Call to I2S DMA write                = %d\n", cur_stats->ampdu_size_short);
        IEEE80211_AOW_DPRINTF("Too late for DMA write               = %d\n", cur_stats->too_late_ampdu);
        IEEE80211_AOW_DPRINTF("Number of packets missing            = %d\n", cur_stats->numPktMissing);
#if ATH_SUPPORT_AOW_TIMESTAMP_DELTA_DEBUG
        IEEE80211_AOW_DPRINTF("Number of -ve timestamp deltas       = %d\n", cur_stats->timestamp_nve_delta);
        IEEE80211_AOW_DPRINTF("Number of zero timestamp deltas      = %d\n", cur_stats->timestamp_zero_delta);
        IEEE80211_AOW_DPRINTF("Number of small +ve timestamp deltas = %d\n", cur_stats->timestamp_tiny_pve_delta);
#endif

        
    } else {
        /* Print the transmit side data */
    }

    AOW_UNLOCK(ic);
}

void ieee80211_audio_stat_clear(struct ieee80211com *ic)
{
    ieee80211_aow_t* paow = &ic->ic_aow;

    AOW_LOCK(ic);

    if (is_aow_audio_stopped(ic)) {

        OS_MEMSET(&paow->stats, 0, sizeof(audio_stats));      

        paow->tx_ctrl_framecount = 0;
        paow->rx_ctrl_framecount = 0;
        paow->ok_framecount = 0;
        paow->nok_framecount = 0;
        paow->tx_framecount = 0;
        paow->macaddr_not_found = 0;
        paow->node_not_found = 0;
        paow->i2s_open_count = 0;
        paow->i2s_close_count = 0;
        paow->i2s_dma_start = 0;
        paow->er.bad_fcs_count = 0;
        paow->er.recovered_frame_count = 0;
        paow->er.aow_er_crc_invalid = 0;
        paow->er.aow_er_crc_valid = 0;
        paow->ec.index0_bad = 0;
        paow->ec.index1_bad = 0;
        paow->ec.index2_bad = 0;
        paow->ec.index3_bad = 0;
        paow->ec.index0_fixed = 0;
        paow->ec.index1_fixed = 0;
        paow->ec.index2_fixed = 0;
        paow->ec.index3_fixed = 0;
        paow->sync_stats.overflow = 0;
        paow->sync_stats.underflow = 0;
        paow->sync_stats.i2s_sample_repeat_count = 0;
        paow->sync_stats.i2s_sample_dropped_count = 0;
        OS_MEMSET(&paow->sync_stats, 0 , sizeof(sync_stats_t));
        ieee80211_aow_clear_i2s_stats(ic);
#if ATH_SUPPORT_AOW_DEBUG
        aow_dbg_init(&dbgstats);
#endif  /* ATH_SUPPORT_AOW_DEBUG */

    } else {
        IEEE80211_AOW_DPRINTF("Device busy\n");
    }

    AOW_UNLOCK(ic);
}      
        

/*
 * function : ieee80211_aow_attach
 * -----------------------------
 * AoW Init Routine
 *
 */


void ieee80211_aow_attach(struct ieee80211com *ic)
{
    static int callcount = 0;

    callcount++;

    /* The last buffer in audio data is used for the zero buffer. Init it seperate and set it to zero */
    OS_MEMSET(&ic->ic_aow.stats, 0, sizeof(audio_stats));

    /* Clear the zero buffer (the last one) */
//    OS_MEMSET(ic->ic_aow.audio_data, 0, sizeof(u_int16_t)*ATH_AOW_NUM_SAMP_PER_AMPDU);    
    
    AOW_SET_AUDIO_SYNC_CTRL_STATE(ic, AOW_STOP_STATE);
    AOW_SET_AUDIO_SYNC_CTRL_SUB_STATE(ic, AOW_STOP_RECEIVE_STATE);

    /* Set the interlever to default */
    ic->ic_aow.interleave = TRUE;

    /* Clear extended stats */
    OS_MEMSET(&ic->ic_aow.estats, 0, sizeof(ic->ic_aow.estats));
    /* To guard against a platform on which NULL is not zero */ 
    ic->ic_aow.estats.aow_latency_stats = NULL;
    ic->ic_aow.estats.aow_l2pe_stats = NULL;
    ic->ic_aow.estats.aow_plr_stats = NULL;
    ic->ic_aow.frame_size = 1;
    ic->ic_aow.alt_setting = 7;
    
    /* Initlize the spinlock */
    AOW_LOCK_INIT(ic);
    AOW_ER_SYNC_LOCK_INIT(ic);
    AOW_LH_LOCK_INIT(ic);
    AOW_ESSC_LOCK_INIT(ic); 
    AOW_L2PE_LOCK_INIT(ic); 
    AOW_PLR_LOCK_INIT(ic);
    AOW_MCSMAP_LOCK_INIT(ic);
#if ATH_SUPPORT_AOW_TXSCHED
    AOW_WBUFSTAGING_LOCK_INIT(ic);
#endif 
// 
    /* Enable the CM sockets */
    aow_init_ci(ic);        
    /* generate the CRC32 table */
    chksum_crc32gentab();


    /*
     * For now, the first wifiX interface for which
     * ieee80211_aow_attach is called will be designated as the 
     * primary interface with which peripheral AoW transports,
     * i.e USB, I2S, etc will be associated. We recored this
     * primary interface into aowinfo
     */

    if (1 == callcount) {
        /* init the globals */
        aowinfo.ic = ic;
    }

#if ATH_SUPPORT_AOW_DEBUG
    aow_dbg_init(&dbgstats);
#endif  /* ATH_SUPPORT_AOW_DEBUG */

    /* init the AoW IE template */
    init_aow_ie(ic);

    /* init version number */
    init_aow_verinfo(ic);

    /* Set some of the variables */
    ic->ic_aow.sync_stats.prev_tsf = 0;

    /* Populate the i2s related functions */
    aow_register_i2s_calls_to_wlan(&wlan_aow_dev);
    i2s = &wlan_aow_dev.rx.i2s_ctrl;
    return;
}
        
/*
 * function : ieee80211_audio_receive
 * -----------------------------------------------
 * IEEE80211 AoW Receive handler, called from the
 * data delivery function.
 *
 */

int ieee80211_audio_receive(struct ieee80211vap *vap, 
                            wbuf_t wbuf, 
                            struct ieee80211_rx_status *rs)
{
    int32_t time_to_play = 0;
    u_int32_t air_delay = 0;
    u_int32_t play_ch = 0;

    u_int64_t aow_latency = 0;
    u_int64_t cur_tsf = 0;
    u_int64_t play_margin;
    char *data;

    struct ieee80211com *ic = vap->iv_ic;
    struct audio_pkt *apkt;
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
    struct ath_buf *bf = ATH_GET_RX_CONTEXT_BUF(wbuf);
#endif

#if ATH_SUPPORT_AOW_TIMESTAMP_DELTA_DEBUG
    audio_stats *cur_stats  = &ic->ic_aow.stats;
#endif

    u_int32_t datalen = 0;
    apkt = (struct audio_pkt *)((char*)(wbuf_raw_data(wbuf)) + sizeof(struct ether_header));


    /* check for valid AOW packet */
    if (apkt->signature != ATH_AOW_SIGNATURE) {
        return 0;
    }        

    /* processing of control packet not in place yet */
    if (apkt->pkt_type == AOW_CTRL_PKT) {
        ieee80211_aow_ctrl_cmd_handler(ic, wbuf, rs);
        return 0;
    }        

    /* copy the volume information */
    if (IS_VOLUME_INFO_CHANGED(ic, apkt)) {
        IEEE80211_AOW_UPDATE_RX_VOLUME_INFO(ic, apkt);
        ieee80211_aow_send_to_host(ic, (u_int8_t*)ic->ic_aow.volume_info.ch, sizeof(ch_volume_data_t), 
                                   AOW_HOST_PKT_EVENT, CM_VOLUME_CHANGE_INDICATION, NULL);
    }


    /* check if audio channel is configured, if yes, then play only the selected channel */
    /* XXX : Check the stats update */
    if (AOW_IS_RX_CHANNEL_ENABLED(ic)) {
        play_ch = AOW_GET_AUDIO_CHANNEL_TO_PLAY(ic);
        if (play_ch != apkt->audio_channel)
            return 0;
    }

    /* Record ES/ESS statistics for AoW L2 Packet Error Stats */

    /* 
     * The FCS fail case would have already been handled by now. 
     * We only handle the FCS pass case. But we check the CRC again 
     * just in case ER too is enabled.
     */
    if ((AOW_ES_ENAB(ic) || (AOW_ESS_ENAB(ic) && AOW_ESS_SYNC_SET(apkt->params)))
            && !(rs->rs_flags & IEEE80211_RX_FCS_ERROR)) {
            u_int32_t crc32;

            crc32 = chksum_crc32((unsigned char*)wbuf_raw_data(wbuf),
                                 sizeof(struct ether_header) +
                                 sizeof(struct audio_pkt) -
                                 sizeof(u_int32_t));

            if (crc32 == apkt->crc32) {
                aow_l2pe_record(ic, true);
            }
    }

    /* Handle the Error Recovery logic, if enabled */
    if (AOW_ER_ENAB(ic) && (aow_er_handler(vap, wbuf, rs) == AOW_ER_DATA_NOK))
        return 0;

    /* extract the audio packet pointers */
    data = (char *)(wbuf_raw_data(wbuf)) + sizeof(struct ether_header) + sizeof(struct audio_pkt) + ATH_QOS_FIELD_SIZE;

    aow_latency = ic->ic_get_aow_latency_us(ic);    /* get latency */
    cur_tsf     = ic->ic_get_aow_tsf_64(ic);        /* get tsf */

    /* get air_delay */
    if (cur_tsf > apkt->timestamp) {
        air_delay = cur_tsf - apkt->timestamp;
    } /* else air_delay remains zero. Rollover will need centuries
         of continuous play to happen :) So we don't waste processing
         cycles handling it. */

#if ATH_SUPPORT_AOW_TIMESTAMP_DELTA_DEBUG
    if (cur_tsf < apkt->timestamp) {
        cur_stats->timestamp_nve_delta++; 
    } else if (air_delay == 0) {
        cur_stats->timestamp_zero_delta++; 
    } else if (air_delay < 200) {
        cur_stats->timestamp_tiny_pve_delta++; 
    }
#endif

    /* get the data length for the received audio packet */
    datalen = wbuf_get_pktlen(wbuf) - sizeof(struct ether_header) - sizeof(struct audio_pkt) - ATH_QOS_FIELD_SIZE;

    /*
     * If Error Recovery is enabled and frame is FCS bad, then
     * get the data length from the validated AoW header
     */

    if (AOW_ER_ENAB(ic) && (rs->rs_flags & IEEE80211_RX_FCS_ERROR))
        datalen = apkt->pkt_len;
    
    ic->ic_aow.frame_size = 
            ((apkt->params >> ATH_AOW_FRAME_SIZE_S) & ATH_AOW_FRAME_SIZE_MASK) ? 4:1;

    play_margin = (ic->ic_aow.frame_size == 1) ? 
            AOW_PKT_RX_PROC_THRESH_1_FRAME:AOW_PKT_RX_PROC_THRESHOLD;
//    play_margin = AOW_PKT_RX_PROC_THRESHOLD;
    if (air_delay < (aow_latency - play_margin - AOW_PKT_RX_PROC_BUFFER)) {
        time_to_play = aow_latency - air_delay;
    } else {
        time_to_play = 0;
        ic->ic_aow.nok_framecount++;
        if (AOW_ES_ENAB(ic)
                || (AOW_ESS_ENAB(ic) &&  AOW_ESS_SYNC_SET(apkt->params))) {
            aow_nl_send_rxpl(ic,
                             apkt->seqno,
                             apkt->params & ATH_AOW_PARAMS_SUB_FRAME_NO_MASK,
                             rs->rs_rxseq,
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                             apkt->timestamp,
                             bf->bf_rx_tsftime,
                             cur_tsf,                           
#endif /* ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG */
                             ATH_AOW_PL_INFO_DELAYED);

            aow_lh_add(ic, air_delay);

            aow_plr_record(ic,
                           apkt->seqno,
                           apkt->params & ATH_AOW_PARAMS_SUB_FRAME_NO_MASK,
                           ATH_AOW_PLR_TYPE_NOK_FRMCNT);
        }  
    }

    /* get the future time to play */
    if (time_to_play ) {
        AOW_IRQ_LOCK(ic);
        if (ic->ic_aow.ctrl.aow_state == AOW_STOP_STATE ) {   

            /*
             * ---|-----------|------------|------------|------------|---
             *
             *   T[tsf]       Interrupt1   Interrupt2   Interrupt3   Interrupt4 ...
             *
             * Time for Interrupt1 = Transmit TSF + AoW Latency - 4000
             * Time for Interrupt2 = Time fo Interrupt1 + 2ms
             * Time for Interrupt3 = Time fo Interrupt2 + 2ms
             * Time for Interrupt4 = Time fo Interrupt3 + 2ms
             * and so on....
             *
             * The cycle of processing is as follows
             *
             *  --- Consume ---> Program I2S ----> I2S Start ----> Idle ---> Consume
             *
             */
            ic->ic_aow.ctrl.aow_state = AOW_CONSUME_STATE;
            ic->ic_aow.ctrl.aow_sub_state = AOW_START_RECEIVE_STATE;
            /* Figure out the frame size */
            ic->ic_start_aow_inter(ic, (u_int32_t)(apkt->timestamp + aow_latency - play_margin-75), 2000);
            {
                u_int16_t aud_param_idx = 
                        (apkt->params >> ATH_AOW_PARAMS_SAMP_SIZE_MASK_S) & ATH_AOW_PARAMS_SAMP_SIZE_MASK;
                /* Sanity Check, if some undefined value has been passed, set it to some valid index */
                if (aud_param_idx >= SAMP_MAX_RATE_NUM_ELEMENTS) aud_param_idx = SAMP_RATE_48k_SAMP_SIZE_16;
                ic->ic_aow.cur_audio_params = &audio_params_array[aud_param_idx];
            }
#if 0
            IEEE80211_AOW_DPRINTF("Data Length %d, interleave %s frame size %d\n", 
                                  apkt->pkt_len,
                                  (apkt->params >> ATH_AOW_PARAMS_INTRPOL_ON_FLG_S &0x1) ? "On":"Off",
                                  ic->ic_aow.frame_size);
#endif
            aow_init_audio_sync_info(ic);
            ieee80211_aow_cmd_handler(ic, ATH_AOW_START_COMMAND);
            ic->ic_aow.ctrl.first_rx_seq = apkt->seqno;
            /* Set the correct processing time */
            ic->ic_set_aow_rx_proc_time(ic, play_margin + AOW_PKT_RX_PROC_BUFFER);
        }   
        /* call the process function to process the data */
        if (ic->ic_aow.frame_size == 1) { // Frame size = 1
            ieee80211_process_audio_data_2ms_frame(vap,
                                             apkt,
                                             rs->rs_rxseq,
                                             cur_tsf,
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                                             bf->bf_rx_tsftime,
#endif

                                             data);
        } else {    
            ieee80211_process_audio_data(vap,
                                         apkt,
                                         rs->rs_rxseq,
                                         cur_tsf,
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                                         bf->bf_rx_tsftime,
#endif

                                         data);
        }
        ic->ic_aow.ok_framecount++;
#if 0
        //Anil Debug
        if (ic->ic_aow.sync_stats.overflow < MAX_WLAN_PTR_LOG) {
            ic->ic_aow.sync_stats.wlan_wr_ptr_after[ic->ic_aow.sync_stats.overflow] = apkt->seqno;
            ic->ic_aow.sync_stats.overflow++;
        }
#endif        
        AOW_IRQ_UNLOCK(ic);
    }

    return 0;

}adf_os_export_symbol(ieee80211_audio_receive);


void aow_clear_buffers(struct ieee80211com* ic)
{
    u_int16_t cnt;
    if (ic->ic_aow.frame_size == 1) 
    {
        for (cnt = 0; cnt < AUDIO_INFO_BUF_SIZE_1_FRAME; cnt++) {
            OS_MEMZERO(&ic->ic_aow.info_1_frame[cnt], sizeof(audio_info_1_frame));
        }        
    } else {
        for (cnt = 0; cnt < AUDIO_INFO_BUF_SIZE; cnt++) {
            OS_MEMZERO(&ic->ic_aow.info[cnt], sizeof(audio_info));
        }        
    }
}

/*
 * function : ieee80211_aow_cmd_handler
 * -------------------------------------------------------------
 * Handles the AoW commands, now deprecated at the transmit node
 * The AoW receive logic, calls this function depending on the
 * current receive state
 *
 */
int ieee80211_aow_cmd_handler(struct ieee80211com* ic, int command)
{ 
    int is_command = FALSE;

    switch (command) {
        case ATH_AOW_RESET_COMMAND:
        {

            /* 
             * Testing : Fix the i2s hang, issue.
             * Sleep for some time and allow the i2s to
             * drain the audio packets, this function
             * can be called from ISR context, we cannot
             * use OS_SLEEP()
             *
             */
#if 0
             {
                int i = 0xffff;
                while(i--);
             }
#endif
            ic->ic_stop_aow_inter(ic);
            aow_i2s_deinit(ic);
            SET_I2S_STOP_FLAG(ic->ic_aow.i2s_flags);
            CLR_I2S_START_FLAG(ic->ic_aow.i2s_flags);
            is_command = TRUE;
            /* Set rx processing time to safe value  
             */
            ic->ic_set_aow_rx_proc_time(ic, AOW_PKT_RX_PROC_THRESHOLD + AOW_PKT_RX_PROC_BUFFER);
            IEEE80211_AOW_DPRINTF("AoW : Received RESET command\n");
         }
         break;

        case ATH_AOW_START_COMMAND:
        {

            aow_i2s_init(ic);
            SET_I2S_START_FLAG(ic->ic_aow.i2s_flags);
            CLR_I2S_STOP_FLAG(ic->ic_aow.i2s_flags);
            is_command = TRUE;
            aow_clear_buffers(ic);
#if 0
            IEEE80211_AOW_DPRINTF("AoW : Received START command sample size = %d\n", ic->ic_aow.cur_audio_params->sample_size_bits);
#endif           
        }
        break;
        
        default:
            IEEE80211_AOW_DPRINTF("Aow : Unknown command\n");
            break;

    }

    return is_command;
}    



/*
 * function : ieee80211_aow_detach
 * ----------------------------------------
 * Cleanup handler for IEEE80211 AoW module
 *
 */
int ieee80211_aow_detach(struct ieee80211com *ic)
{

    if (IS_I2S_OPEN(ic)) {
        i2s->close();
        SET_I2S_OPEN_STATE(ic, FALSE);
    }        

    AOW_LOCK(ic);

    // Just in case the deinit has not been called.
    aow_lh_deinit(ic);
    aow_am_deinit(ic);
    aow_l2pe_deinit(ic);
    aow_plr_deinit(ic);
    aow_deinit_ci(ic);
    
    AOW_UNLOCK(ic);
    AOW_LOCK_DESTROY(ic);
    AOW_LH_LOCK_DESTROY(ic);
    AOW_ESSC_LOCK_DESTROY(ic);
    AOW_L2PE_LOCK_DESTROY(ic);
    AOW_PLR_LOCK_DESTROY(ic);
    AOW_MCSMAP_LOCK_DESTROY(ic);
#if ATH_SUPPORT_AOW_TXSCHED
    AOW_WBUFSTAGING_LOCK_DESTROY(ic);
#endif

    return 0;
}

/**
 * @brief           Return position of AoW Audio Packet in non-decapsulated
 *                  wbuf. 
 * 
 * This function is intended for preliminary deep inspection. For efficiency,
 * it is kept simple and doesn't handle all possible cases - only those likely
 * to occur when AoW is used, and when there is a high probability that the wbuf
 * contains an AoW-related frame.
 *
 * WAPI is not yet handled (because AoW is intended to be used with P2P, and
 * it doesn't look like WAPI will be used with AoW-P2P as at present).
 * TODO: Add WAPI support if required by customers.
 *
 * @param[in] ic    Handle to ieee80211com data structure.
 * @param[in] wbuf  WLAN buffer to be searched for the AoW Audio Packet.
 * @return          Position of AoW Audio Packet in wbuf on success,
 *                  -EINVAL if an argument is invalid/has invalid contents,
 *                  -ENOENT if no AoW Audio Packet is found.
 * 
 */

int16_t ieee80211_aow_apktindex(struct ieee80211com *ic, wbuf_t wbuf)
{
    const u_int8_t *kidp = NULL;
    const u_int8_t *hdr = NULL;
    struct ieee80211_frame *wh = NULL;
    struct audio_pkt *apkt = NULL;
    int hdrlen = 0;
    int16_t aow_posn = 0;
    u_int16_t wbuf_pktlen = 0;

    if (NULL == wbuf) {
        return -EINVAL;
    }

    hdr = wbuf_header(wbuf);
    wh  = (struct ieee80211_frame *)hdr;
    hdrlen = ieee80211_anyhdrspace(ic, hdr);
    wbuf_pktlen = wbuf_get_pktlen(wbuf);
    
    aow_posn += hdrlen;

    if (aow_posn > wbuf_pktlen) {
        /* Huh?? */
        return -EINVAL;
    }

    if (wh->i_fc[1] & IEEE80211_FC1_WEP) {
        aow_posn += (IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN);

        if (aow_posn > wbuf_pktlen) {
            return -EINVAL;
        }

        kidp = hdr + hdrlen + IEEE80211_WEP_IVLEN;

        if ((*kidp) & IEEE80211_WEP_EXTIV) {
            /* TKIP/AES-CCMP */
            aow_posn += IEEE80211_WEP_EXTIVLEN;
            
            if (aow_posn > wbuf_pktlen) {
                return -EINVAL;
            }
        }
    }

    aow_posn += sizeof(struct llc);
    
    if (aow_posn > wbuf_pktlen) {
        return -ENOENT;
    }

    apkt = (struct audio_pkt*)(hdr + aow_posn);

    if (apkt->signature != ATH_AOW_SIGNATURE) {
        return -ENOENT;
    }

    return aow_posn;
}

/******************************************************************************

    Functions relating to AoW PoC 2
    Interface fu:nctions between USB and Wlan

******************************************************************************/    


int aow_get_macaddr(int channel, int index, struct ether_addr *macaddr)
{
    struct ieee80211com *ic = aowinfo.ic;
    return ic->ic_get_aow_macaddr(ic, channel, index, macaddr);
}

int aow_get_num_dst(int channel)
{
    struct ieee80211com *ic = aowinfo.ic;
    return ic->ic_get_num_mapped_dst(ic, channel);
}

void wlan_get_tsf(u_int64_t* tsf)
{
#if 1
    struct ieee80211com* ic = aowinfo.ic;
    *tsf = ic->ic_get_aow_tsf_64(ic);
#else
    u_int64_t cur_tsf;
    int32_t timeDiff;
    struct ieee80211com* ic = aowinfo.ic;
    u_int32_t inc_fact =
        ic->ic_aow.cur_audio_params->mpdu_time_slice * 
        ic->ic_aow.frame_size;
    u_int32_t hist_fact = 2000 * 3;
    if (ic->ic_aow.frame_size == 4) {
        hist_fact = 12000;        
    }
    cur_tsf = ic->ic_get_aow_tsf_64(ic);
    timeDiff = (int32_t)(cur_tsf - ic->ic_aow.sync_stats.prev_tsf);
    *tsf = cur_tsf;
    ic->ic_aow.sync_stats.prev_tsf = cur_tsf;
    if ((timeDiff > hist_fact) || (timeDiff < 0) ) {
        ic->ic_aow.sync_stats.sec_counter = 0;
        ic->ic_aow.sync_stats.time_consumed = 0;
        /* Clear the stats */
        aow_init_audio_sync_info(ic);
        ic->ic_aow.sync_stats.startState = 0;
    } else {
        ic->ic_aow.sync_stats.time_consumed += timeDiff;
        ic->ic_aow.sync_stats.sec_counter++;
#if 0
        if (ic->ic_aow.sync_stats.wlan_data_buf_idx < MAX_WLAN_PTR_LOG) {
            ic->ic_aow.sync_stats.wlan_wr_ptr_before[ic->ic_aow.sync_stats.wlan_data_buf_idx] = 
                    timeDiff;
            ic->ic_aow.sync_stats.wlan_wr_ptr_after[ic->ic_aow.sync_stats.wlan_data_buf_idx] = 
                                ic->ic_aow.sync_stats.time_consumed;
            ic->ic_aow.sync_stats.wlan_data_buf_idx++;
        } 
#endif   
        if (ic->ic_aow.sync_stats.sec_counter == 500) {
            timeDiff = (int32_t)(ic->ic_aow.sync_stats.time_consumed - 
                500* inc_fact);
            if ((timeDiff < 100) && (timeDiff > -100)) { 
                ic->ic_aow.sync_stats.filt_val = 
                        (ic->ic_aow.sync_stats.filt_val >> 1) +
                        (timeDiff >> 1);
            }
            if (ic->ic_aow.sync_stats.wlan_data_buf_idx < MAX_WLAN_PTR_LOG) {
                ic->ic_aow.sync_stats.wlan_wr_ptr_before[ic->ic_aow.sync_stats.wlan_data_buf_idx] = 
                        timeDiff;
                ic->ic_aow.sync_stats.wlan_wr_ptr_after[ic->ic_aow.sync_stats.wlan_data_buf_idx] = 
                        ic->ic_aow.sync_stats.time_consumed;
                ic->ic_aow.sync_stats.wlan_rd_ptr_before[ic->ic_aow.sync_stats.wlan_data_buf_idx] = 
                        ic->ic_aow.sync_stats.sec_counter;
                ic->ic_aow.sync_stats.wlan_rd_ptr_after[ic->ic_aow.sync_stats.wlan_data_buf_idx] = 
                    ic->ic_aow.sync_stats.filt_val;
                ic->ic_aow.sync_stats.wlan_data_buf_idx++;
            } 
            /* Clear the counters */
            ic->ic_aow.sync_stats.time_consumed = 0;
            ic->ic_aow.sync_stats.sec_counter = 0;
        }
    }   
#if ATH_SUPPORT_AOW_DEBUG
    /* update the aow stats */
    aow_update_dbg_stats(&dbgstats, (*tsf - dbgstats.ref_p_tsf));
    /* store the complete tsf for future use */
    dbgstats.ref_p_tsf = *tsf;
#endif  /* ATH_SUPPORT_AOW_DEBUG */
#endif
}EXPORT_SYMBOL(wlan_get_tsf);


/**
 * @brief       Set Audio Parameters.
 * 
 * @param[in]   audiotype       Audio type (combination of sampling rate and
 *                              bits per sample).
 */

void wlan_aow_set_audioparams(audio_type_t audiotype)
{
    struct ieee80211com *ic = aowinfo.ic;

    if (SAMP_RATE_48k_SAMP_SIZE_24 == audiotype) {
        ic->ic_aow.cur_audio_params = &audio_params_array[SAMP_RATE_48k_SAMP_SIZE_24];
    } else {
        ic->ic_aow.cur_audio_params = &audio_params_array[SAMP_RATE_48k_SAMP_SIZE_16];
    }

} EXPORT_SYMBOL(wlan_aow_set_audioparams);


/*
 * function : wlan_aow_tx
 * --------------------------------------
 * transmit handler for AoW audio packets 
 *
 */

int wlan_aow_tx(char *data, int datalen, int channel, u_int64_t tsf)
{
    struct ether_addr macaddr;
    struct ieee80211_node *ni = NULL;
    struct ieee80211com *ic = aowinfo.ic;

    int dst_cnt = 0;
    int seqno = 0;
    int playval;
    int play_local;
    int play_channel;
    bool setlogSync = FALSE;
    int done = FALSE;
    int get_seqno = TRUE;
    int j = 0;
    u_int64_t max_tx_delay = ic->ic_get_aow_latency_us(ic) - AOW_PKT_TX_THRESH_1_FRAME;

    /* get the number of destination, to stream */
    dst_cnt = aow_get_num_dst(channel);

    for (j = 0; j < dst_cnt; j++) {

        /* check for the valid channel */
        if (!(ic->ic_aow.channel_set_flag & (1 << channel))) {
            ic->ic_aow.macaddr_not_found++;
            return FALSE;
        }

        /* get destination */
        if (!aow_get_macaddr(channel, j, &macaddr)) {
            ic->ic_aow.macaddr_not_found++;
            continue;
        }        

        /* get the destination */
        ni = ieee80211_find_node(&ic->ic_sta, macaddr.octet);

        /* valid destination */
        if (!ni) {
            ic->ic_aow.node_not_found++;
            continue;
        }
        
        /* Whether procedures related to new association are done. */
        if (!(ni->ni_new_assoc_done)) {
            continue;
        } 

        /* Check it it is too late to play this packet */
        if ((ic->ic_get_aow_tsf_64(ic) -tsf) > max_tx_delay) {
            ic->ic_aow.nok_framecount++;
            /* update the seqno and setlogSync flag */
            if (get_seqno == TRUE) {
                ic->ic_get_aow_chan_seqno(ic, channel, &seqno);
            
                if (AOW_ESS_ENAB(ic)) {
                    setlogSync = aow_essc_testdecr(ic, channel);
                } else {
                    setlogSync = 0;
                }
                get_seqno = FALSE;
            }
            continue;            
        }
        /* check if play local is enabled */
        playval = ic->ic_get_aow_playlocal(ic);
        
        if ((playval & ATH_AOW_PLAY_LOCAL_MASK) && !done) {

            play_local   = playval & ATH_AOW_PLAY_LOCAL_MASK;
            play_channel = playval >> ATH_AOW_PLAY_LOCAL_MASK;

            if (play_local && (channel == play_channel)) {

                u_int64_t aow_latency = ic->ic_get_aow_latency_us(ic);

                int playcount = datalen/AOW_I2S_DATA_BLOCK_SIZE;
                int left      = datalen % AOW_I2S_DATA_BLOCK_SIZE;

                char *pdata   = data;

                while (playcount--) {
                    i2s->write(AOW_I2S_DATA_BLOCK_SIZE, pdata);
                    pdata += AOW_I2S_DATA_BLOCK_SIZE;
                }                

                if (left) {
                    i2s->write(left, pdata);
                }                
                if (IS_I2S_NEED_DMA_START(ic->ic_aow.i2s_flags)) {
                    ic->ic_aow.i2s_dmastart = TRUE;
                }

                ic->ic_start_aow_inter(ic, (u_int32_t)(tsf + aow_latency), AOW_PACKET_INTERVAL);
            }
            
            done = TRUE;
        }

        /* up the transmit count */
        ic->ic_aow.tx_framecount++;

        /* update the seqno and setlogSync flag */
        if (get_seqno == TRUE) {
            ic->ic_get_aow_chan_seqno(ic, channel, &seqno);
            
            if (AOW_ESS_ENAB(ic)) {
                setlogSync = aow_essc_testdecr(ic, channel);
            } else {
                setlogSync = 0;
            }
            
            get_seqno = FALSE;
        }            
       
        /* now send the data */
        ieee80211_send_aow_data_ipformat(ni, data, datalen, seqno, tsf, channel, setlogSync);
    }
    return TRUE;

}EXPORT_SYMBOL(wlan_aow_tx);

/**
 * @brief                 Inform USB about input frame size to be used.
 * 
 * @param[in]  framesize  Frame size multiplier.
 *                        Valid values: 1 (2 ms frames)
 *                                    : 4 (8 ms frames)
 *
 * @return     0 on success, negative value on failure.
 */

int ieee80211_aow_setframesize_to_usb(unsigned int framesize)
{
    int ret = 0;

    if ((framesize != 1) && (framesize != 4)) {
        return -EINVAL;
    }

    if (wlan_aow_dev.rx.set_frame_size) {
        ret = wlan_aow_dev.rx.set_frame_size(framesize);

        if (ret < 0) { 
            IEEE80211_AOW_DPRINTF("Could not set frame size %u "
                                  "into USB. ret=%d\n",
                                  framesize,
                                  ret);
            return ret;
        }    
    } else {
        return -ENOENT;
    }
    
    return 0;    
} 

/**
 * @brief    Inform USB about alt setting to expect.
 *
 * This is a temporary requirement to work around some USB issues.
 * 
 * @param[in]  altsetting  Alt setting to expect. Valid values: 1-8
 *
 * @return     0 on success, negative value on failure.
 */

int ieee80211_aow_setaltsetting_to_usb(unsigned int altsetting)
{
    int ret = 0;

    if (altsetting < 1 || altsetting > 8) {
        return -EINVAL;
    }

    if (wlan_aow_dev.rx.set_alt_setting) {
        ret = wlan_aow_dev.rx.set_alt_setting(altsetting);

        if (ret < 0) { 
            IEEE80211_AOW_DPRINTF("Could not set alt setting %u "
                                  "into USB. ret=%d\n",
                                  altsetting,
                                  ret);
            return ret;
        }    
    } else {
        return -ENOENT;
    }
    
    return 0;    
} 


#if ATH_SUPPORT_AOW_TXSCHED

/**
 * @brief   Compare two AoW wbuf staging entries.
 * 
 * Current policy:
 * 1) Order of decreasing goodput is used.
 * 2) Beyond checking for goodput, we do not touch the order in which the wbufs
 *    have been staged.
 *
 * @param   left              Pointer to Left Hand Side AoW wbuf staging entry.
 * @param   right             Pointer to Right Hand Side AoW wbuf staging entry.
 *
 * @return  AWS_COMPR_L_GT_R  If the left entry is greater than the right one.
 * @return  AWS_COMPR_L_LT_R  If the left entry is lesser than the right one.
 * @return  AWS_COMPR_L_EQ_R  If both entries are equal.
 *
 * @note                      Having a separate comparator gives us flexibility
 *                            to change scheduling policy easily.
 *
 */

static int aws_comparator(aow_wbuf_staging_entry *left,
                          aow_wbuf_staging_entry *right)
{
#define GOODPUT(_aws)  ((_aws)->ni->aow_min_goodput)
#define STGORDER(_aws) ((_aws)->staging_order)

    /* Order required: decreasing goodput */
    if (GOODPUT(left) > GOODPUT(right)) {
        return AWS_COMPR_L_LT_R;
    } else if (GOODPUT(left) < GOODPUT(right)) {
        return AWS_COMPR_L_GT_R;
    }

    /* Goodput is the same. We consider original Tx
       order as determined at staging point. */

    if (STGORDER(left) < STGORDER(right)) {
        return AWS_COMPR_L_LT_R;
    } else if (STGORDER(left) > STGORDER(right)) {
        return AWS_COMPR_L_GT_R;
    }

    return AWS_COMPR_L_EQ_R;

#undef GOODPUT
#undef STGORDER
}

/**
 * @brief   Sort AoW wbuf staging entries.
 *
 * @param aws_entry_ptr    Array of pointers to the aow_wbuf_staging_entry
 *                         structures to be sorted.
 * @param aws_entry_count  Number of aow_wbuf_staging_entry structures to
 *                         be sorted.
 */
static void aow_sort_aws_entries(aow_wbuf_staging_entry **aws_entry_ptr,
                                 int aws_entry_count)
{
    int i, j;
    aow_wbuf_staging_entry *temp = NULL;

    if (aws_entry_count < 2) {
        return;
    }
    
    /* Very small no. of entries expected. Hence a simple
       sort mechanism will be more efficient than complex ones. */

    for (i = 0; i < aws_entry_count; i++) {
        for (j = i + 1; j < aws_entry_count; j++) {
            if (AWS_COMPR_L_GT_R ==
                    (aws_comparator(aws_entry_ptr[i], aws_entry_ptr[j]))) {
                temp = aws_entry_ptr[i];
                aws_entry_ptr[i] = aws_entry_ptr[j];
                aws_entry_ptr[j] = temp;
            }
        }
    }
}

/**
 * @brief   Dispatch the wbufs which have been staged for transmission.
 *
 * @return  0 on success, negative error code on failure.
 */
int wlan_aow_dispatch_data()
{
    struct ieee80211com *ic = aowinfo.ic;
    int i = 0;
    int count = 0;
    int retVal = 0;
    aow_wbuf_staging_entry *entry = NULL;
    
    AOW_WBUFSTAGING_LOCK(ic);
   
    count = ic->ic_aow.aws_entry_count;

    if (0 == count) {
        AOW_WBUFSTAGING_UNLOCK(ic);
        return -ENOENT;
    }

    aow_sort_aws_entries(ic->ic_aow.aws_entry_ptr, count);

    for (i = 0; i < count; i++)
    {
       entry = ic->ic_aow.aws_entry_ptr[i];
       retVal |= ath_tx_send(entry->wbuf);
       entry->ni->aow_min_goodput_valid = FALSE;
    } 
   
    /* Wipe the slate clean */ 
    memset(ic->ic_aow.aws_entry, 0, sizeof(ic->ic_aow.aws_entry));
    memset(ic->ic_aow.aws_entry_ptr, 0, sizeof(ic->ic_aow.aws_entry_ptr));
    ic->ic_aow.aws_entry_count = 0;
    
    AOW_WBUFSTAGING_UNLOCK(ic);

    return retVal;

}EXPORT_SYMBOL(wlan_aow_dispatch_data);
#endif /* ATH_SUPPORT_AOW_TXSCHED */

void ieee80211_aow_clear_i2s_stats(struct ieee80211com *ic)
{
    i2s_clear_stats();
}    

void ieee80211_aow_i2s_stats(struct ieee80211com* ic)
{
    i2s_stats_t stats;
    i2s_get_stats(&stats);

    IEEE80211_AOW_DPRINTF("I2S Write fail   = %d\n", stats.write_fail);
    IEEE80211_AOW_DPRINTF("I2S Rx underflow = %d\n", stats.rx_underflow);
    IEEE80211_AOW_DPRINTF("Tasklet Count    = %d\n", stats.tasklet_count);
    IEEE80211_AOW_DPRINTF("Pkt repeat Count = %d\n", stats.repeat_count);

    if (stats.aow_sync_enabled) {
        IEEE80211_AOW_DPRINTF("SYNC overflow  = %d\n", stats.sync_buf_full);
        IEEE80211_AOW_DPRINTF("SYNC underflow = %d\n", stats.sync_buf_empty);
    }
}

void ieee80211_aow_ec_stats(struct ieee80211com* ic)
{
    if (!AOW_EC_ENAB(ic))
        return;

    IEEE80211_AOW_DPRINTF("Index0 Fixed = %d\n", ic->ic_aow.ec.index0_fixed);
    IEEE80211_AOW_DPRINTF("Index1 Fixed = %d\n", ic->ic_aow.ec.index1_fixed);
    IEEE80211_AOW_DPRINTF("Index2 Fixed = %d\n", ic->ic_aow.ec.index2_fixed);
    IEEE80211_AOW_DPRINTF("Index3 Fixed = %d\n", ic->ic_aow.ec.index3_fixed);

}

void ieee80211_aow_ec_ramp_stats(struct ieee80211com* ic)
{
    if (!AOW_EC_RAMP(ic))
        return;

    IEEE80211_AOW_DPRINTF("Ramp up = %d\n", ic->ic_aow.ec.index0_fixed);
    IEEE80211_AOW_DPRINTF("Ramp down = %d\n", ic->ic_aow.ec.index1_fixed);

}

void ieee80211_aow_clear_stats(struct ieee80211com* ic)
{


}

/*
 * Error concealment feature
 *
 */
int aow_ec_attach(struct ieee80211com *ic)
{
    OS_MEMSET(ic->ic_aow.ec.prev_data, 0, sizeof(ic->ic_aow.ec.prev_data));
    return 0;
}

int aow_ec_deattach(struct ieee80211com *ic)
{
    return 0;
}

/* 
 * Error recovery feature
 *
 */

/* initalize the error recovery state and variables */
int aow_er_attach(struct ieee80211com *ic)
{
    ic->ic_aow.er.cstate = ER_IDLE;
    ic->ic_aow.er.expected_seqno = 0;

    if (!ic->ic_aow.er.data)
        return FALSE;

    return TRUE;
}

/* deinitalize the error recovery state and variables */
int aow_er_deattach(struct ieee80211com *ic)
{
    if (ic->ic_aow.er.data) {
        OS_FREE(ic->ic_aow.er.data);
    }        
    return FALSE;
}

/* handler for aow error recovery feature */
int aow_er_handler(struct ieee80211vap *vap, 
                   wbuf_t wbuf, 
                   struct ieee80211_rx_status *rs)
{
    int crc32;
    struct audio_pkt *apkt;
    struct ieee80211com *ic = vap->iv_ic;
    char *data;
    int ret = AOW_ER_DATA_NOK;
    int i = 0;
    int datalen;

    KASSERT((ic != NULL), ("ic Null pointer"));

    AOW_ER_SYNC_IRQ_LOCK(ic);

    apkt = (struct audio_pkt *)((char*)(wbuf_raw_data(wbuf)) + 
           sizeof(struct ether_header));

    data = (char *)(wbuf_raw_data(wbuf)) + 
           sizeof(struct ether_header) + 
           sizeof(struct audio_pkt) + 
           ATH_QOS_FIELD_SIZE;

    datalen = wbuf_get_pktlen(wbuf) - 
              sizeof(struct ether_header) - 
              sizeof(struct audio_pkt) - 
              ATH_QOS_FIELD_SIZE;

    KASSERT(apkt != NULL, ("Audio Null Pointer"));              
    KASSERT(data != NULL, ("Data Null Pointer"));

    if (!(rs->rs_flags & IEEE80211_RX_FCS_ERROR)) {

        /* Handle the FCS ok case */

        if (ic->ic_aow.er.expected_seqno == apkt->seqno) {
            /* Received the expected frame,
             * reset the cached data buffer
             * clear er related state
             */
            
            ic->ic_aow.er.cstate = ER_IDLE;
            ic->ic_aow.er.flag &= ~AOW_ER_DATA_VALID;
        }            
        
        /* Clear the AoW ER state machine, as the packets come in
         * order, this is ok 
         */
        ic->ic_aow.er.expected_seqno = apkt->seqno++;
        ic->ic_aow.er.cstate = ER_IDLE;
        ic->ic_aow.er.count = 0;
        ic->ic_aow.er.flag &= ~AOW_ER_DATA_VALID;
        ret = AOW_ER_DATA_OK;

    } else {

        /* Handle the FCS nok case */

        //TODO: Cover sender & receiver addresses in the CRC

        crc32 = chksum_crc32((unsigned char*)apkt, (sizeof(struct audio_pkt) - sizeof(int)));

        if (crc32 == apkt->crc32) {

            IEEE80211_AOW_DPRINTF("Bad FCS frame, AoW CRC pass ES = %d PS = %d\n", ic->ic_aow.er.expected_seqno, apkt->seqno);

            ic->ic_aow.er.aow_er_crc_valid++;

            /* AoW header is valid */
            if (!(ic->ic_aow.er.flag & AOW_ER_DATA_VALID)) {

               /* first instance, store the data */
               KASSERT((apkt->pkt_len <= ATH_AOW_NUM_SAMP_PER_AMPDU), ("Invalid data length"));
               memcpy(ic->ic_aow.er.data, data, apkt->pkt_len);

               /* set the er state info */
               ic->ic_aow.er.flag |= AOW_ER_DATA_VALID;
               ic->ic_aow.er.datalen = apkt->pkt_len;
               ic->ic_aow.er.count++;

            } else {

               /* consecutive data, recover data from them */
               /* xor the incoming data to stored buffer */ 
               KASSERT((ic->ic_aow.er.datalen <= ATH_AOW_NUM_SAMP_PER_AMPDU), ("Invalid data length"));
               for (i = 0; i < ic->ic_aow.er.datalen; i++) {
                   ic->ic_aow.er.data[i] |= data[i];
               }                   
                
               /* update the er state */
               ic->ic_aow.er.count++;
            }

            /* check the err packet occurance had exceeded the limit */
            if (ic->ic_aow.er.count > AOW_ER_MAX_RETRIES) {

                IEEE80211_AOW_DPRINTF("Aow ER : Passing the recovered data\n");

                /* pass the data for audio processing */
                KASSERT(data != NULL, ("Data Null Pointer"));
                KASSERT(ic->ic_aow.er.datalen <= ATH_AOW_NUM_SAMP_PER_AMPDU, ("Invalid datalength"));
                memcpy(data, ic->ic_aow.er.data, ic->ic_aow.er.datalen);

                /* reset the er state */
                ic->ic_aow.er.count = 0;
                ic->ic_aow.er.flag &= ~AOW_ER_DATA_VALID;
                ic->ic_aow.er.datalen = 0;
                ic->ic_aow.er.recovered_frame_count++;

                ret = AOW_ER_DATA_OK;
            }

        } else {
            ic->ic_aow.er.aow_er_crc_invalid++;
            /* AoW header is invalid */
        }
    }

    AOW_ER_SYNC_IRQ_UNLOCK(ic);

    return ret;
    
}

/*
 * This function conceals error for single 
 * packet losses
 *
 * X01  X11 X21 X31  |
 * X02  X12 X22 X32  |
 * X03  X13 X23 X33  |
 *
 *
 * The formula to recover the data is given
 * below
 *
 * If the failed frame index is 0 or 3
 * -----------------------------------
 * R = Row, Failed index
 * C = Coloum, the data elements
 *
 * For Sub frame index 0
 * ---------------------
 * Off set of two is required for Left and Right channels
 * The Left and Right channels alternate
 * X[R][C] = (X[3][C - 2] + X[1][C])/2
 *
 * For Sub frame index 3
 * ---------------------
 * Off set of two is required for Left and Right channels
 * The Left and Right channels alternate
 * X[R][C] = (X[2][C] + X[0][C + 2])/2
 * 
 * If the failed frame index is 1 or 2
 * -----------------------------------
 * R = Row, Failed index
 * C = Coloum, the data elements
 * 
 * X[R][C] = (X[R -1][C] + X[(R+1)][C]) >> 1
 *
 * Example : (X12 = X02 + X22) / 2
 * 
 */

int aow_ec(struct ieee80211com *ic, audio_info* info, int index)
{
    u_int32_t row, col;
    u_int32_t pre_row;
    u_int32_t post_row;
    u_int16_t result;

    row = index;

    switch(index) {
        case AOW_SUB_FRAME_IDX_0:
           pre_row = AOW_SUB_FRAME_IDX_3;
           post_row = AOW_SUB_FRAME_IDX_1;

           /* TODO : Update with actual diff value 
            * Currently we map the next sample in the buffer
            * for Left and Right channels
            */
           info->audBuffer[row][0] = info->audBuffer[AOW_SUB_FRAME_IDX_1][0];
           info->audBuffer[row][1] = info->audBuffer[AOW_SUB_FRAME_IDX_1][1];

           for (col = 2; col < ATH_AOW_NUM_SAMP_PER_MPDU; col++) {
               result = ((((int16_t)__bswap16(info->audBuffer[pre_row][col - 2])) >> 1) +
                     (((int16_t)__bswap16(info->audBuffer[post_row][col])) >> 1));
               info->audBuffer[row][col] = __bswap16(result);
           }                    
           break;

        case AOW_SUB_FRAME_IDX_3:                
            pre_row  = AOW_SUB_FRAME_IDX_2;
            post_row = AOW_SUB_FRAME_IDX_0;

            for (col = 0; col < ATH_AOW_NUM_SAMP_PER_MPDU - 2; col++) {
                result = ((((int16_t)__bswap16(info->audBuffer[pre_row][col])) >> 1) +
                      (((int16_t)__bswap16(info->audBuffer[post_row][col + 2])) >> 1));
        
                info->audBuffer[row][col] = __bswap16(result);
            }                    

            /* TODO : Update with actual diff value 
             * Currently we map the previous sample in the buffer
             * for Left and Right channels
             */
            info->audBuffer[row][ATH_AOW_NUM_SAMP_PER_MPDU - 2] = info->audBuffer[AOW_SUB_FRAME_IDX_2][ATH_AOW_NUM_SAMP_PER_MPDU - 2];
            info->audBuffer[row][ATH_AOW_NUM_SAMP_PER_MPDU - 1] = info->audBuffer[AOW_SUB_FRAME_IDX_2][ATH_AOW_NUM_SAMP_PER_MPDU - 1];
            break;                    

        case AOW_SUB_FRAME_IDX_1:
        case AOW_SUB_FRAME_IDX_2:

            /* get the pre and post row index */
            pre_row = row - 1;
            post_row = row + 1; 

            for (col = 0; col < ATH_AOW_NUM_SAMP_PER_MPDU; col++) {
                result = ((((int16_t)__bswap16(info->audBuffer[pre_row][col])) >> 1) +
                          (((int16_t)__bswap16(info->audBuffer[post_row][col])) >> 1));
                
                info->audBuffer[row][col] = __bswap16(result);
            }
            break;
    }

    return 0;
}


/*
 * Function to recover multiple error 
 *
 */

int aow_ecm(struct ieee80211com *ic, audio_info* info, int failmap)
{
    if (failmap & 0x1) {
        aow_ec(ic, info, 0);
        ic->ic_aow.ec.index0_fixed++;
    }                

    if (failmap & 0x2) {
        aow_ec(ic, info, 1);
        ic->ic_aow.ec.index1_fixed++;
    }        

    if (failmap & 0x4) {
        aow_ec(ic, info, 2);
        ic->ic_aow.ec.index2_fixed++;
    }        

    if (failmap & 0x8) {
        aow_ec(ic, info, 3);
        ic->ic_aow.ec.index3_fixed++;
    }        

    return 0;        
}

/*
 * Find the number of bits set in integer
 *
 */
int num_of_bits_set(int i)
{
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}

/*
 * AoW Error Concealment function handler 
 *
 */

int aow_ec_handler(struct ieee80211com *ic, audio_info *info)
{

    /* find the number of failed frame */
    int failmap = (~(info->foundBlocks) & AOW_EC_SUB_FRAME_MASK);
    int failed_frame_count = num_of_bits_set((~(info->foundBlocks) & AOW_EC_SUB_FRAME_MASK));

    if (failed_frame_count == 0)
        return 0;

    aow_ecm(ic, info, failmap);
    return 0;
}

static void aow_update_ec_stats(struct ieee80211com* ic, int fmap)
{
    if (fmap & 0x1)
        ic->ic_aow.ec.index0_bad++;
    if (fmap & 0x2)
        ic->ic_aow.ec.index1_bad++;
    if (fmap & 0x4)
        ic->ic_aow.ec.index2_bad++;
    if (fmap & 0x8)
        ic->ic_aow.ec.index3_bad++;
}

/*
 * function : ieee80211_aow_get_stats
 * ----------------------------------
 * Print the AoW Related stats to console
 *
 */

int ieee80211_aow_get_stats(struct ieee80211com*ic)
{
    if (is_aow_audio_stopped(ic)) {

        /* print AoW Aggregation stats */
        ieee80211_audio_stat_print(ic);

        /* Disable the GPIO Interrupts */
        ic->ic_stop_aow_inter(ic);

        /* Simple implementation */
        IEEE80211_AOW_DPRINTF("\nTransmit Stats\n");
        IEEE80211_AOW_DPRINTF("--------------------------------------------\n");
        IEEE80211_AOW_DPRINTF("Tx frames      = %d\n", ic->ic_aow.tx_framecount);
        IEEE80211_AOW_DPRINTF("Addr not found = %d\n", ic->ic_aow.macaddr_not_found);
        IEEE80211_AOW_DPRINTF("Node not found = %d\n", ic->ic_aow.node_not_found);
        IEEE80211_AOW_DPRINTF("CTRL frames    = %d\n", ic->ic_aow.tx_ctrl_framecount);

        IEEE80211_AOW_DPRINTF("\nReceive Stats\n");
        IEEE80211_AOW_DPRINTF("--------------------------------------------\n");
        IEEE80211_AOW_DPRINTF("CTRL frames    = %d\n", ic->ic_aow.rx_ctrl_framecount);

        IEEE80211_AOW_DPRINTF("--------------------------------------------\n");
        IEEE80211_AOW_DPRINTF("Intime frames  = %d\n", ic->ic_aow.ok_framecount);
        IEEE80211_AOW_DPRINTF("Delayed frames = %d\n", ic->ic_aow.nok_framecount);
        IEEE80211_AOW_DPRINTF("Latency        = %d\n", ic->ic_get_aow_latency(ic));

        IEEE80211_AOW_DPRINTF("\nI2S Stats\n");
        IEEE80211_AOW_DPRINTF("--------------------------------------------\n");
        IEEE80211_AOW_DPRINTF("I2S Open       = %d\n", ic->ic_aow.i2s_open_count);
        IEEE80211_AOW_DPRINTF("I2S Close      = %d\n", ic->ic_aow.i2s_close_count);
        IEEE80211_AOW_DPRINTF("I2S DMA start  = %d\n", ic->ic_aow.i2s_dma_start);

        ieee80211_aow_i2s_stats(ic);

        if (AOW_EC_ENAB(ic)) {
            IEEE80211_AOW_DPRINTF("\nError Concealment Stats\n");
            IEEE80211_AOW_DPRINTF("--------------------------------------------\n");
            ieee80211_aow_ec_stats(ic);
        }        

        if (AOW_EC_RAMP(ic)) {
            IEEE80211_AOW_DPRINTF("\nError Concealment ramp stats\n");
            IEEE80211_AOW_DPRINTF("--------------------------------------------\n");
            ieee80211_aow_ec_ramp_stats(ic);
        }        
        
        ieee80211_aow_sync_stats(ic);

        print_aow_ie(ic);

        IEEE80211_AOW_DPRINTF("\n");
    } else {
        IEEE80211_AOW_DPRINTF("Device busy\n");
    }

    return 0;
}

/*
 * function : ieee80211_send_aow_data_ipformat
 * --------------------------------------------------------------
 * This function sends the AoW data, in the ipformat. 
 * The IP header is appended to the AoW data and the normal
 * transmit path is invoked, to make sure that it has aggregation
 * and other features enabled on its way. This function handles 2ms or 8ms
 * frames and 16/24 bit sample size
 */
int ieee80211_send_aow_data_ipformat(struct ieee80211_node* ni, 
                                     void *pkt,
                                     int len, 
                                     u_int32_t seqno, 
                                     u_int64_t tsf,
                                     u_int32_t audio_channel,
                                     bool setlogSync)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic = ni->ni_ic;
    wbuf_t wbuf[ATH_AOW_NUM_MPDU];
    struct ether_header *eh;

    int total_len = 0;
    int retVal    = 0;
    int16_t cnt, sampCnt;

    u_int8_t *frame;
    
    u_int16_t tmpBuf[ATH_AOW_NUM_MPDU][ATH_AOW_NUM_SAMP_PER_MPDU*2];
    u_int16_t mpdu_len;
    u_int16_t num_samp = len >> 1; //Get the length in 16 bits */
    u_int16_t *dataPtr = (u_int16_t *)pkt;

    /* 
    * Packet Format
    *
    * |---------|-----|-------|------------|----------|------------|-------------|--------|
    * | Header  | Sig | Seqno | Timestamp  |  length  | Parameters | Audio data  | CRC    |
    * |---------|-----|-------|------------|----------|------------|-------------|--------|
    *
    * The legth we get the ioctl interface is gives the audio data length
     * The frame length is a sum of:
    *      - 802.11 mac header, assumes QoS
    *      - AOW signature
    *      - AOW sequence number
    *      - AOW Timestamp
    *      - AOW Length (Of all the packets in the AMPDU)
    *      - Parameters (2 bytes )
    *      - CRC 
    *
    * 
    */


     /* 
    * Some assumptions about the data size here, assume it is 8ms = 48*2*8 samples = 768 16 bit words
    * MPDU length is always = 192 words
     */

    /* Debug feature to force the input to known value */
    if (ic->ic_aow.ctrl.force_input )
    {
        for (sampCnt = 0; sampCnt < num_samp; sampCnt++) {
            dataPtr[sampCnt] = sampCnt;
        }
    }
    
    /* data length of each MPDU depends of the number of audio sub-frames for every 
     * packet
     */
    mpdu_len = num_samp; //1 sub-frame/frame
    if (ic->ic_aow.frame_size == 4) {
        mpdu_len = num_samp/ic->ic_aow.frame_size;
    }
    total_len = mpdu_len * sizeof(u_int16_t) + sizeof(struct audio_pkt) + ATH_QOS_FIELD_SIZE;

    for (cnt = 0; cnt < ic->ic_aow.frame_size; cnt++) {
        memcpy( tmpBuf[cnt], dataPtr + cnt*mpdu_len, sizeof(u_int16_t)*mpdu_len);        
    }

    /* allocate the wbuf for the frame */
    for (cnt = 0; cnt < ic->ic_aow.frame_size; cnt++ ) {
        wbuf[cnt] = ieee80211_get_aow_frame(ni, &frame, total_len);
        /* Check if there is enough memory. If not, free all the buffers and exit */
        if (wbuf[cnt] == NULL) {
            for (; cnt >= 0; cnt--) {
                wbuf_free(wbuf[cnt]);
            }   
            vap->iv_stats.is_tx_nobuf++;
            ieee80211_free_node(ni);

            return -ENOMEM;
        }        
    }
    
    for (cnt = 0; cnt < ic->ic_aow.frame_size; cnt++ ) {

        /* take the size of QOS, SIG, TS into account */
        int offset = ATH_QOS_FIELD_SIZE + sizeof(struct audio_pkt);

        /* prepare the ethernet header */
        eh = (struct ether_header*)wbuf_push(wbuf[cnt], sizeof(struct ether_header));
        eh = (struct ether_header*)wbuf_raw_data(wbuf[cnt]);
        eh = (struct ether_header*)wbuf_header(wbuf[cnt]);
    
        /* prepare the ethernet header */
        IEEE80211_ADDR_COPY(eh->ether_dhost, ni->ni_macaddr);
        IEEE80211_ADDR_COPY(eh->ether_shost, vap->iv_myaddr);
        eh->ether_type = ETHERTYPE_IP;
    
        /* copy the user data to the skb */
        {
            char* pdata = (char*)&eh[1];
            struct audio_pkt *apkt = (struct audio_pkt*)pdata;
    
            apkt->signature = ATH_AOW_SIGNATURE;
            apkt->seqno = seqno;
            apkt->timestamp = tsf;
            /* Set the length field */
            apkt->pkt_len = num_samp;
            apkt->pkt_type = AOW_DATA_PKT;
            /* Set the parameter field */
            apkt->params = cnt;     
            apkt->params |= (0 & ATH_AOW_PARAMS_INTRPOL_ON_FLG_MASK)<<ATH_AOW_PARAMS_INTRPOL_ON_FLG_S;
            /* If setlogSync is true, it automatically signifies
            that ESS is enabled. We optimize on processing */
            apkt->params |= (setlogSync & ATH_AOW_PARAMS_LOGSYNC_FLG_MASK)<< ATH_AOW_PARAMS_LOGSYNC_FLG_S; 
            apkt->params |= (ic->ic_aow.cur_audio_params->audio_param_id & ATH_AOW_PARAMS_SAMP_SIZE_MASK) << ATH_AOW_PARAMS_SAMP_SIZE_MASK_S;
            apkt->params |= (ic->ic_aow.frame_size == 4) << ATH_AOW_FRAME_SIZE_S;
            /* set the audio channel */
            apkt->audio_channel = audio_channel;

            /* update the volume info */
            IEEE80211_AOW_INSERT_VOLUME_INFO(ic, apkt);
            

            if (AOW_ER_ENAB(ic) || AOW_ES_ENAB(ic) || setlogSync) {
                apkt->crc32 = chksum_crc32((unsigned char*)eh,
                                            (sizeof(struct ether_header) +
                                                    sizeof(struct audio_pkt) -
                                                    sizeof(int)));
            }

            /* copy the user data to skb */
            memcpy((pdata + offset), tmpBuf[cnt], mpdu_len * sizeof(u_int16_t));
            if (!(cnt)) {
                if (ic->ic_aow.ctrl.capture_data ) {
                    if (ic->ic_aow.stats.prevTsf ) { 
                        ic->ic_aow.stats.datalenBuf[ic->ic_aow.stats.dbgCnt] = (u_int32_t)(apkt->timestamp - ic->ic_aow.stats.prevTsf);
                        ic->ic_aow.stats.sumTime += ic->ic_aow.stats.datalenBuf[ic->ic_aow.stats.dbgCnt];
                        if (ic->ic_aow.stats.datalenBuf[ic->ic_aow.stats.dbgCnt] > ATH_AOW_STATS_THRESHOLD ) ic->ic_aow.stats.grtCnt++;
                        
                        ic->ic_aow.stats.datalenLen[ic->ic_aow.stats.dbgCnt++] = apkt->pkt_len;    
                        if (ic->ic_aow.stats.dbgCnt == AOW_DBG_WND_LEN) {
                            ic->ic_aow.ctrl.capture_data = 0;
                            ic->ic_aow.ctrl.capture_done = 1;    
                        }
                    }
                    ic->ic_aow.stats.prevTsf = apkt->timestamp;
                }
            }
           
        }

#if ATH_SUPPORT_AOW_TXSCHED
       retVal |= ieee80211_aow_tx_stage(ic, (wbuf_t)wbuf[cnt]);
#else
       /* invoke the normal transmit path */
       retVal |= ath_tx_send((wbuf_t)wbuf[cnt]);
#endif
    }

    ieee80211_free_node(ni);
    return retVal;
}

/*
 * function : ieee80211_send_aow_data_ipformat_old
 * --------------------------------------------------------------
 * This function sends the AoW data, in the ipformat. 
 * The IP header is appended to the AoW data and the normal
 * transmit path is invoked, to make sure that it has aggregation
 * and other features enabled on its way
 * NOTE: This function does not support 24 bit sample size and will be 
 * cleaned up sometime in future. Just kept as reference for some time ...
 */
int ieee80211_send_aow_data_ipformat_old(struct ieee80211_node* ni, 
                                     void *pkt,
                                     int len, 
                                     u_int32_t seqno, 
                                     u_int64_t tsf,
                                     u_int32_t audio_channel,
                                     bool setlogSync)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic = ni->ni_ic;
    wbuf_t wbuf[ATH_AOW_NUM_MPDU];
    struct ether_header *eh;

    int total_len = 0;
    int retVal    = 0;

    int16_t cnt, sampCnt;

    u_int8_t *frame;
    
    u_int16_t tmpBuf[ATH_AOW_NUM_MPDU][ATH_AOW_NUM_SAMP_PER_MPDU];
    u_int16_t mpdu_len;
    u_int16_t num_samp = len >> 1; /* assuming 16 bit audio samples */
    u_int16_t *dataPtr = (u_int16_t *)pkt;
#if ATH_SUPPORT_AOW_SAMPSIZE_SIM
    u_int16_t  extra_sampsim_len = 0;
#endif



    /* 
     * Packet Format
     *
     * |---------|-----|-------|------------|----------|------------|-------------|--------|
     * | Header  | Sig | Seqno | Timestamp  |  length  | Parameters | Audio data  | CRC    |
     * |---------|-----|-------|------------|----------|------------|-------------|--------|
     *
     * The legth we get the ioctl interface is gives the audio data length
     * The frame length is a sum of:
     *      - 802.11 mac header, assumes QoS
     *      - AOW signature
     *      - AOW sequence number
     *      - AOW Timestamp
     *      - AOW Length (Of all the packets in the AMPDU)
     *      - Parameters (2 bytes )
     *      - CRC 
     *
     * 
     */


     /* 
      * Some assumptions about the data size here, assume it is 8ms = 48*2*8 samples = 768 16 bit words
      * MPDU length is always = 192 words
      */

    /* Debug feature to force the input to known value */
    if (ic->ic_aow.ctrl.force_input )
    {
        for (sampCnt = 0; sampCnt < num_samp; sampCnt++) {
            dataPtr[sampCnt] = sampCnt;
        }
    }

    /* Interleave and copy the data into tmp buffers */
    if (AOW_INTERLEAVE_ENAB(ic)) {

        for (cnt = 0; cnt < num_samp; cnt += 2) {
            sampCnt = cnt >> 1; /* This is for the left and the right sample */
   
            tmpBuf[sampCnt & ATH_AOW_MPDU_MOD_MASK][ ((sampCnt >> ATH_AOW_NUM_MPDU_LOG2) << 1)] = dataPtr[cnt];
            tmpBuf[sampCnt & ATH_AOW_MPDU_MOD_MASK][ ((sampCnt >> ATH_AOW_NUM_MPDU_LOG2) << 1) + 1] = dataPtr[cnt+1];
        }

    } else {

        u_int32_t sub_frame_len = (num_samp + ATH_AOW_MPDU_MOD_MASK) >> ATH_AOW_NUM_MPDU_LOG2;

        for (cnt = 0; cnt < ATH_AOW_NUM_MPDU; cnt++) {
            memcpy( tmpBuf[cnt], dataPtr + cnt*sub_frame_len, sizeof(u_int16_t)*sub_frame_len);        
        }
    }

    /* get the total length of the AOW packet */
    mpdu_len  = (num_samp + ATH_AOW_PARAMS_SUB_FRAME_NO_MASK) >> ATH_AOW_NUM_MPDU_LOG2;
    total_len = mpdu_len * sizeof(u_int16_t) + sizeof(struct audio_pkt) + ATH_QOS_FIELD_SIZE;
#if ATH_SUPPORT_AOW_SAMPSIZE_SIM 
    /* Simulate larger sample size by adding more bytes at the end of the MPDU */
    if (ic->ic_aow.cur_audio_params->sample_size_bits > 16) {
        extra_sampsim_len = 
            ((ic->ic_aow.cur_audio_params->sample_size_bits / 8) - ATH_AOW_SAMPLE_SIZE) * mpdu_len;
        total_len += extra_sampsim_len;
    }
#endif
    
    /* allocate the wbuf for the wbuf */
    for (cnt = 0; cnt < ATH_AOW_NUM_MPDU; cnt++ ) {

        wbuf[cnt] = ieee80211_get_aow_frame(ni, &frame, total_len);

        /* Check if there is enough memory. If not, free all the buffers and exit */
        if (wbuf[cnt] == NULL) {

            for (; cnt >= 0; cnt--) {
                wbuf_free(wbuf[cnt]);
            }   

            vap->iv_stats.is_tx_nobuf++;
            ieee80211_free_node(ni);

            return -ENOMEM;
        }        
    }
    
    
    for (cnt = 0; cnt < ATH_AOW_NUM_MPDU; cnt++ ) {

        /* take the size of QOS, SIG, TS into account */
        int offset = ATH_QOS_FIELD_SIZE + sizeof(struct audio_pkt);

        /* prepare the ethernet header */
        eh = (struct ether_header*)wbuf_push(wbuf[cnt], sizeof(struct ether_header));
        eh = (struct ether_header*)wbuf_raw_data(wbuf[cnt]);
        eh = (struct ether_header*)wbuf_header(wbuf[cnt]);
    
        /* prepare the ethernet header */
        IEEE80211_ADDR_COPY(eh->ether_dhost, ni->ni_macaddr);
        IEEE80211_ADDR_COPY(eh->ether_shost, vap->iv_myaddr);
        eh->ether_type = ETHERTYPE_IP;
    
        /* copy the user data to the skb */
        {
            char* pdata = (char*)&eh[1];
            struct audio_pkt *apkt = (struct audio_pkt*)pdata;
    
            apkt->signature = ATH_AOW_SIGNATURE;
            apkt->seqno = seqno;
            apkt->timestamp = tsf;
            /* Set the length field */
            apkt->pkt_len = num_samp;
            apkt->pkt_type = AOW_DATA_PKT;
            /* Set the parameter field */
            apkt->params = cnt;     
            apkt->params |= ((AOW_INTERLEAVE_ENAB(ic)) & ATH_AOW_PARAMS_INTRPOL_ON_FLG_MASK)<<ATH_AOW_PARAMS_INTRPOL_ON_FLG_S;
            /* If setlogSync is true, it automatically signifies
               that ESS is enabled. We optimize on processing */
            apkt->params |= (setlogSync & ATH_AOW_PARAMS_LOGSYNC_FLG_MASK)<< ATH_AOW_PARAMS_LOGSYNC_FLG_S; 

            /* set the audio channel */
            apkt->audio_channel = audio_channel;

            /* update the volume info */
            IEEE80211_AOW_INSERT_VOLUME_INFO(ic, apkt);

            if (AOW_ER_ENAB(ic) || AOW_ES_ENAB(ic) || setlogSync) {
                apkt->crc32 = chksum_crc32((unsigned char*)eh,
                                           (sizeof(struct ether_header) +
                                            sizeof(struct audio_pkt) -
                                            sizeof(int)));
            }

            /* copy the user data to skb */
            memcpy((pdata + offset), tmpBuf[cnt], mpdu_len * sizeof(u_int16_t));
            if (!(cnt)) {
                if (ic->ic_aow.ctrl.capture_data ) {
                    if (ic->ic_aow.stats.prevTsf ) { 
                        ic->ic_aow.stats.datalenBuf[ic->ic_aow.stats.dbgCnt] = (u_int32_t)(apkt->timestamp - ic->ic_aow.stats.prevTsf);
                        ic->ic_aow.stats.sumTime += ic->ic_aow.stats.datalenBuf[ic->ic_aow.stats.dbgCnt];
                        if (ic->ic_aow.stats.datalenBuf[ic->ic_aow.stats.dbgCnt] > ATH_AOW_STATS_THRESHOLD ) ic->ic_aow.stats.grtCnt++;
                        
                        ic->ic_aow.stats.datalenLen[ic->ic_aow.stats.dbgCnt++] = apkt->pkt_len;    
                        if (ic->ic_aow.stats.dbgCnt == AOW_DBG_WND_LEN) {
                            ic->ic_aow.ctrl.capture_data = 0;
                            ic->ic_aow.ctrl.capture_done = 1;    
                        }
                    }
                    ic->ic_aow.stats.prevTsf = apkt->timestamp;
                }
            }
           
#if ATH_SUPPORT_AOW_SAMPSIZE_SIM
            if (extra_sampsim_len > 0) {
                memset((pdata + offset + (mpdu_len * ATH_AOW_SAMPLE_SIZE)),
                       0,
                       extra_sampsim_len);
            }
#endif
        }

#if ATH_SUPPORT_AOW_TXSCHED
        retVal |= ieee80211_aow_tx_stage(ic, (wbuf_t)wbuf[cnt]);
#else
       /* invoke the normal transmit path */
        retVal |= ath_tx_send((wbuf_t)wbuf[cnt]);
#endif
    }

    ieee80211_free_node(ni);
    return retVal;


}

#if ATH_SUPPORT_AOW_TXSCHED
/**
 * @brief  Stage wbuf for later dispatch
 * 
 * This is required for later scheduling transmissions in the required order.
 * Once the lower link providing us the data (e.g. USB) signals that the data
 * is to be dispatched, all the wbufs staged will be sent down the normal
 * transmit path.
 * 
 * @param[in] ic        Handle for ic data structure.
 * @param[in] wbuf      wbuf to be staged.
 * @return    0         On success.
 * @return   -ENOMEM    If max AoW wbuf staging entries
 *                      reached.
 *
 */
int ieee80211_aow_tx_stage(struct ieee80211com *ic, wbuf_t wbuf)
{
    aow_wbuf_staging_entry *aws_entry = NULL;
    u_int32_t goodput = 0;
    int count = 0;

    AOW_WBUFSTAGING_LOCK(ic);
    
    if (AOW_MAX_AWS_ENTRIES == ic->ic_aow.aws_entry_count) {
        IEEE80211_AOW_DPRINTF("WARNING: Max AoW WBUF Staging Entries (%u) "
                              "reached! Not accepting wbuf.\n",
                              AOW_MAX_AWS_ENTRIES);
        AOW_WBUFSTAGING_UNLOCK(ic);
        return -ENOMEM;
    }
    
    ic->ic_aow.aws_entry_count++;

    count = ic->ic_aow.aws_entry_count;

    aws_entry = &(ic->ic_aow.aws_entry[count - 1]);
    ic->ic_aow.aws_entry_ptr[count - 1] = aws_entry;

    aws_entry->staging_order = count;
    aws_entry->ni = wbuf_get_node(wbuf);
    aws_entry->wbuf = wbuf;
    
    goodput = ic->ic_get_goodput(wbuf_get_node(wbuf));
    
    if (!(aws_entry->ni->aow_min_goodput_valid)) {
        aws_entry->ni->aow_min_goodput = goodput;
        aws_entry->ni->aow_min_goodput_valid = TRUE;
    } else if (aws_entry->ni->aow_min_goodput > goodput) {
        aws_entry->ni->aow_min_goodput = goodput;
    }

    AOW_WBUFSTAGING_UNLOCK(ic);

    return 0;
}
#endif

/*
 * function : ieee80211_get_aow_frame
 * --------------------------------------------------------
 * Allocates the frame for the given length and returns the
 * pointer to SKB
 *
 */
wbuf_t 
ieee80211_get_aow_frame(struct ieee80211_node* ni, 
                        u_int8_t **frm, 
                        u_int32_t pktlen)
{
    struct ieee80211com *ic = ni->ni_ic;
    wbuf_t wbuf;

    wbuf = wbuf_alloc(ic->ic_osdev, WBUF_TX_DATA, pktlen);
    if (wbuf == NULL)
        return NULL;

    wbuf_set_node(wbuf, ieee80211_ref_node(ni));
    wbuf_set_priority(wbuf, WME_AC_VO);
    wbuf_set_tid(wbuf, WME_AC_TO_TID(WME_AC_VO));
    wbuf_set_pktlen(wbuf, pktlen);

    return wbuf;
}

/* Init and DeInit operations for Extended Statistics */

/*
 * function : aow_es_base_init
 * ----------------------------------
 * Extended stats: Base Initialization (common to
 * synchronized and unsynchronized modes).
 *
 */
int aow_es_base_init(struct ieee80211com *ic)
{
    int retval;
    
    if ((retval = aow_lh_init(ic)) < 0) {
        return retval;
    }

    if ((retval = aow_am_init(ic)) < 0) {
        aow_lh_deinit(ic);
        return retval;
    }
    
    if ((retval =  aow_l2pe_init(ic)) < 0) {
        aow_lh_deinit(ic);
        aow_am_deinit(ic);
        return retval;
    }
    
    if ((retval =  aow_plr_init(ic)) < 0) {
        aow_lh_deinit(ic);
        aow_am_deinit(ic);
        aow_l2pe_deinit(ic);
        return retval;
    }

    OS_MEMSET(&ic->ic_aow.estats.aow_mcs_stats,
              0,
              sizeof(ic->ic_aow.estats.aow_mcs_stats));

    OS_MEMSET(dummy_i2s_data, 0, sizeof(dummy_i2s_data));

    return 0;
}

/*
 * function : aow_es_base_deinit
 * ----------------------------------
 * Extended stats: Base De-Initialization (common to
 * synchronized and unsynchronized modes).
 */

void aow_es_base_deinit(struct ieee80211com *ic)
{
    aow_lh_deinit(ic);
    aow_am_deinit(ic);
    aow_l2pe_deinit(ic);
    aow_plr_deinit(ic);

    OS_MEMSET(&ic->ic_aow.estats.aow_mcs_stats,
              0,
              sizeof(ic->ic_aow.estats.aow_mcs_stats));
}

/*
 * function : aow_clear_ext_stats
 * ----------------------------------
 * Extended stats: Clear extended stats
 *
 */
void aow_clear_ext_stats(struct ieee80211com *ic)
{
    if (AOW_ES_ENAB(ic) || AOW_ESS_ENAB(ic)) {
        aow_lh_reset(ic);
        aow_l2pe_reset(ic);
        aow_plr_reset(ic);

        OS_MEMSET(&ic->ic_aow.estats.aow_mcs_stats,
                  0,
                  sizeof(ic->ic_aow.estats.aow_mcs_stats));
    }
}

/*
 * function : aow_print_ext_stats
 * ----------------------------------
 * Extended stats: Print extended stats
 *
 */
void aow_print_ext_stats(struct ieee80211com * ic)
{
    if (!AOW_ES_ENAB(ic) && !AOW_ESS_ENAB(ic)) {
        return;
    }

    if (is_aow_audio_stopped(ic)) {
        if (ic->ic_aow.node_type == ATH_AOW_NODE_TYPE_RX) {
            aow_lh_print(ic);
            aow_l2pe_print(ic);
            aow_plr_print(ic);
        } else if (ic->ic_aow.node_type == ATH_AOW_NODE_TYPE_TX) {
            aow_mcss_print(ic);
        }
    } else {
        IEEE80211_AOW_DPRINTF("Device busy\n");
    }
}

/* Operations on Latency Histogram (LH) records */

/*
 * function : aow_lh_init
 * ----------------------------------
 * Extended stats: Init latency histogram recording
 *
 */
int aow_lh_init(struct ieee80211com *ic)
{
   u_int64_t aow_latency;
   u_int32_t aow_latency_max_bin; 

   aow_latency = ic->ic_get_aow_latency_us(ic);

   if (aow_latency > ATH_AOW_LH_MAX_LATENCY_US)
   {
        IEEE80211_AOW_DPRINTF("Max aow_latency covered by "
                              "Latency Histogram: %u microseconds\n",
                              ATH_AOW_LH_MAX_LATENCY_US);
        return -EINVAL;
   }

   AOW_LH_LOCK(ic);    
   
   ic->ic_aow.estats.aow_latency_max_bin =
       (u_int32_t)aow_latency/ATH_AOW_LATENCY_RANGE_SIZE;

   aow_latency_max_bin = ic->ic_aow.estats.aow_latency_max_bin;

   ic->ic_aow.estats.aow_latency_stats =
       (u_int32_t *)OS_MALLOC(ic->ic_osdev,
                                 aow_latency_max_bin * sizeof(u_int32_t),
                                 GFP_KERNEL);
    
   if (NULL == ic->ic_aow.estats.aow_latency_stats)
   {
        IEEE80211_AOW_DPRINTF("%s: Could not allocate memory\n", __func__);
        AOW_LH_UNLOCK(ic);
        return -ENOMEM;
   }

   OS_MEMSET(ic->ic_aow.estats.aow_latency_stats,
          0,
          aow_latency_max_bin * sizeof(u_int32_t));

   ic->ic_aow.estats.aow_latency_stat_expired = 0;

   AOW_LH_UNLOCK(ic);

   return 0; 
}

/*
 * function : aow_lh_reset
 * ----------------------------------
 * Extended stats: Reset latency histogram recording
 *
 */
int aow_lh_reset(struct ieee80211com *ic)
{
   aow_lh_deinit(ic);
   return aow_lh_init(ic);
}

/*
 * function : aow_record_latency
 * ----------------------------------
 * Extended stats: Increment count in latency histogram bin for MPDUs received
 * before expiry of max AoW latency.
 *
 */
int aow_lh_add(struct ieee80211com *ic, const u_int64_t latency)
{
   u_int64_t aow_latency;

   aow_latency = ic->ic_get_aow_latency_us(ic);

   if (latency >= aow_latency) {
       return aow_lh_add_expired(ic);
   }
   
   AOW_LH_LOCK(ic);

   if (NULL == ic->ic_aow.estats.aow_latency_stats)
   {
       AOW_LH_UNLOCK(ic); 
       return -1; 
   } 

   // TODO: If profiling results show a slowdown, we can explore
   // the possibility of some optimized logic instead of division.
   ic->ic_aow.estats.aow_latency_stats[\
       (u_int32_t)latency/ATH_AOW_LATENCY_RANGE_SIZE]++;
    
   AOW_LH_UNLOCK(ic); 
    
   return 0;   
}

/*
 * function : aow_lh_add_expired
 * ----------------------------------
 * Extended stats: Increment count in latency histogram bin for MPDUs received
 * after expiry of max AoW latency.
 *
 */
int aow_lh_add_expired(struct ieee80211com *ic)
{
   AOW_LH_LOCK(ic);    
  
   /* This serves as a check to ensure LH is active */ 
   if (NULL == ic->ic_aow.estats.aow_latency_stats)
   {
       AOW_LH_UNLOCK(ic); 
       return -1; 
   } 

   ic->ic_aow.estats.aow_latency_stat_expired++; 

   AOW_LH_UNLOCK(ic);

   return 0;
}

/*
 * function : aow_lh_print
 * ----------------------------------
 * Extended stats: Print contents of latency histogram
 *
 */
int aow_lh_print(struct ieee80211com *ic)
{
    int i;

    AOW_LH_LOCK(ic);    
   
    if (NULL == ic->ic_aow.estats.aow_latency_stats) {
        IEEE80211_AOW_DPRINTF("-------------------------------\n");
        IEEE80211_AOW_DPRINTF("Latency Histogram not available\n");
        IEEE80211_AOW_DPRINTF("-------------------------------\n");
        AOW_LH_UNLOCK(ic);    
        return -1;
    }

    IEEE80211_AOW_DPRINTF("--------------------------------------------\n");
    IEEE80211_AOW_DPRINTF("\nLatency Histogram\n");
    IEEE80211_AOW_DPRINTF("--------------------------------------------\n");
        
    for (i = 0; i < ic->ic_aow.estats.aow_latency_max_bin; i++)
    {
        IEEE80211_AOW_DPRINTF("%-5u to %-5u usec : %u\n",
                              ATH_AOW_LATENCY_RANGE_SIZE * i,
                              ATH_AOW_LATENCY_RANGE_SIZE * (i + 1) - 1,
                              ic->ic_aow.estats.aow_latency_stats[i]);
    }
     
    IEEE80211_AOW_DPRINTF("%-5u usec and above: %u\n",
                          ATH_AOW_LATENCY_RANGE_SIZE * i,
                          ic->ic_aow.estats.aow_latency_stat_expired);

    AOW_LH_UNLOCK(ic);

    return 0;
}

/*
 * function : aow_lh_deinit
 * ----------------------------------
 * Extended stats: De-init latency histogram recording
 *
 */
void aow_lh_deinit(struct ieee80211com *ic)
{
    AOW_LH_LOCK(ic);

    if (ic->ic_aow.estats.aow_latency_stats != NULL)
    {    
        OS_FREE(ic->ic_aow.estats.aow_latency_stats);
        ic->ic_aow.estats.aow_latency_stats = NULL;
        ic->ic_aow.estats.aow_latency_stat_expired = 0;
    }

    AOW_LH_UNLOCK(ic);    
}

#if ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG
static void aow_populate_advncd_txinfo(struct aow_advncd_txinfo *atxinfo,
                                       struct ath_tx_status *ts,
                                       struct ath_buf *bf,
                                       int loopcount)
{
    atxinfo->ts_tstamp = ts->ts_tstamp;
    atxinfo->ts_seqnum = ts->ts_seqnum;
    atxinfo->ts_status = ts->ts_status;
    atxinfo->ts_ratecode = ts->ts_ratecode;
    atxinfo->ts_rateindex = ts->ts_rateindex;
    atxinfo->ts_rssi = ts->ts_rssi;
    atxinfo->ts_shortretry = ts->ts_shortretry;
    atxinfo->ts_longretry = ts->ts_longretry;
    atxinfo->ts_virtcol = ts->ts_virtcol;
    atxinfo->ts_antenna = ts->ts_antenna;
    atxinfo->ts_flags = ts->ts_flags;
    atxinfo->ts_rssi_ctl0 = ts->ts_rssi_ctl0;
    atxinfo->ts_rssi_ctl1 = ts->ts_rssi_ctl1;
    atxinfo->ts_rssi_ctl2 = ts->ts_rssi_ctl2;
    atxinfo->ts_rssi_ext0 = ts->ts_rssi_ext0;
    atxinfo->ts_rssi_ext1 = ts->ts_rssi_ext1;
    atxinfo->ts_rssi_ext2 = ts->ts_rssi_ext2;
    atxinfo->queue_id = ts->queue_id;
    atxinfo->desc_id = ts->desc_id;
    atxinfo->ba_low = ts->ba_low;
    atxinfo->ba_high = ts->ba_high;
    atxinfo->evm0 = ts->evm0;
    atxinfo->evm1 = ts->evm1;
    atxinfo->evm2 = ts->evm2;
    atxinfo->ts_txbfstatus = ts->ts_txbfstatus;
    atxinfo->tid = ts->tid;

    atxinfo->bf_avail_buf = bf->bf_avail_buf;
    atxinfo->bf_status = bf->bf_status;
    atxinfo->bf_flags = bf->bf_flags;
    atxinfo->bf_reftxpower = bf->bf_reftxpower;
#if ATH_SUPPORT_IQUE && ATH_SUPPORT_IQUE_EXT
    atxinfo->bf_txduration = bf->bf_txduration;
#endif

    atxinfo->bfs_nframes = bf->bf_state.bfs_nframes;
    atxinfo->bfs_al = bf->bf_state.bfs_al;
    atxinfo->bfs_frmlen = bf->bf_frmlen;
    atxinfo->bfs_seqno = bf->bf_state.bfs_seqno;
    atxinfo->bfs_tidno = bf->bf_tidno;
    atxinfo->bfs_retries = bf->bf_state.bfs_retries;
    atxinfo->bfs_useminrate = bf->bf_useminrate;
    atxinfo->bfs_ismcast = bf->bf_ismcast;
    atxinfo->bfs_isdata = bf->bf_isdata;
    atxinfo->bfs_isaggr = bf->bf_state.bfs_isaggr;
    atxinfo->bfs_isampdu = bf->bf_state.bfs_isampdu;
    atxinfo->bfs_ht = bf->bf_state.bfs_ht;
    atxinfo->bfs_isretried = bf->bf_state.bfs_isretried;
    atxinfo->bfs_isxretried = bf->bf_state.bfs_isxretried;
    atxinfo->bfs_shpreamble = bf->bf_shpreamble;
    atxinfo->bfs_isbar = bf->bf_isbar;
    atxinfo->bfs_ispspoll = bf->bf_ispspoll;
    atxinfo->bfs_aggrburst = bf->bf_state.bfs_aggrburst;
    atxinfo->bfs_calcairtime = bf->bf_calcairtime;
#ifdef ATH_SUPPORT_UAPSD
    atxinfo->bfs_qosnulleosp = bf->bf_state.bfs_qosnulleosp;
#endif
    atxinfo->bfs_ispaprd = bf->bf_state.bfs_ispaprd;
    atxinfo->bfs_isswaborted = bf->bf_state.bfs_isswaborted;
#if ATH_SUPPORT_CFEND
    atxinfo->bfs_iscfend = bf->bf_state.bfs_iscfend;
#endif
#ifdef ATH_SWRETRY
    atxinfo->bfs_isswretry = bf->bf_state.bfs_isswretry;
    atxinfo->bfs_swretries = bf->bf_state.bfs_swretries;
    atxinfo->bfs_totaltries = bf->bf_state.bfs_totaltries;
#endif
    atxinfo->bfs_qnum = bf->bf_state.bfs_qnum;
    atxinfo->bfs_rifsburst_elem = bf->bf_state.bfs_rifsburst_elem;
    atxinfo->bfs_nrifsubframes = bf->bf_state.bfs_nrifsubframes;
    atxinfo->bfs_txbfstatus = bf->bf_state.bfs_txbfstatus;
    
    atxinfo->loopcount = loopcount;

}
#endif /* ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG */

/* Operations to update Tx completion information */

/*
 * function : aow_istxsuccess
 * ----------------------------------
 * Find if transmission of MPDU was successful
 *
 */
static bool aow_istxsuccess(struct ieee80211com *ic,
                            struct ath_buf *bf,
                            struct ath_tx_status *ts,
                            bool is_aggr)
{
    u_int32_t ba_bitmap[AOW_WME_BA_BMP_SIZE >> 5];
    
    if (!is_aggr) {
        if (ts->ts_status == 0) {
            return true;
        } else {
            return false;
        }
    }
  
    /*TODO: Optimize the code to carry out the 8 byte bitmap creation once
            at a single point for an aggregate */

    if (ts->ts_status == 0 ) {
        if (ATH_AOW_DS_TX_BA(ts)) {
            OS_MEMCPY(ba_bitmap, ATH_AOW_DS_BA_BITMAP(ts), AOW_WME_BA_BMP_SIZE >> 3);
        } else {
            OS_MEMZERO(ba_bitmap, AOW_WME_BA_BMP_SIZE >> 3);
        }
    } else {
        OS_MEMZERO(ba_bitmap, AOW_WME_BA_BMP_SIZE >> 3);
    }

    if (ATH_AOW_BA_ISSET(ba_bitmap,
                         ATH_AOW_BA_INDEX(ATH_AOW_DS_BA_SEQ(ts),
                                          bf->bf_seqno))) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief        Map receiver address to the corresponding MCS stats group entry.
 *               If no mapping exists and there is space for a new one, create a
 *               new one.
 * 
 * @param[in]    ic          Pointer to ieee80211com structure containing the MCS
 *                           stats group entries.
 * @param[in]    recvr_addr  MAC address of receiver.
 * @param[out]   mstats      Double pointer using which pointer to MCS stats group
 *                           entry should be returned. 
 *
 * @return       0 on success
 * @return       -EINVAL on invalid argument.
 * @return       -ENOMEM if there is no space to allocate a new entry.
 *
 * @note         It is assumed that the calling function has checked for validity
 *               of recvr_addr by looking for the AoW signature (but a NULL check
 *               is done anyway).
 */

static int map_recvr_to_mcs_stats(struct ieee80211com *ic,
                                  struct ether_addr* recvr_addr,
                                  struct mcs_stats** mstats)
{
    /* Note: We could use more sophisticated means to carry out this mapping
       if required in the future.
       However, for now we keep it simple since the performance gain may not
       be required during Extended Stats measurement. */
    
    int i = 0;
    int empty_slot = -1;
    
    *mstats = NULL;

    if (!recvr_addr) {
        IEEE80211_AOW_DPRINTF("Receiver address is NULL\n");
        return -EINVAL;
    }

    AOW_MCSMAP_LOCK(ic);

    for (i = 0; i < AOW_MAX_RECVRS; i++)
    {
       if (ic->ic_aow.estats.aow_mcs_stats[i].populated) {
            if (!memcmp(recvr_addr,
                        &ic->ic_aow.estats.aow_mcs_stats[i].recvr_addr,
                        sizeof(struct ether_addr)))
            {
                *mstats = &ic->ic_aow.estats.aow_mcs_stats[i];
                break;
            }
       } else {
           /* Entries are stored one after the other, without any gap */
           empty_slot = i;
           break;
       }
    }
    
    if (!(*mstats)) {
        if (-1 != empty_slot) {
            /* There is space for a new entry. */

            *mstats = &ic->ic_aow.estats.aow_mcs_stats[empty_slot];
            
            memcpy(&(*mstats)->recvr_addr,
                   recvr_addr,
                   sizeof(struct ether_addr));

            (*mstats)->populated = 1;

        } else {
            /* We should never hit this. If we do,
               it indicates broken code somewhere! */
            IEEE80211_AOW_DPRINTF("No space to accomodate a new MCS stats entry\n");
            AOW_MCSMAP_UNLOCK(ic);
            return -ENOMEM;
        }
    }

    AOW_MCSMAP_UNLOCK(ic);
    return 0;
}


/*
 * function : aow_update_mcsinfo
 * ----------------------------------
 * Extended stats: Update MCS information for
 * a given MPDU.
 *
 */
void aow_update_mcsinfo(struct ieee80211com *ic,
                        struct ath_buf *bf,
                        struct ether_addr *recvr_addr,
                        struct ath_tx_status *ts,
                        bool is_success,
                        int *num_attempts)
{

    int i = 0, j = 0;
    u_int8_t    rix;
    u_int8_t    tries;
    int         max_prefinal_index;
    int         lnum_attempts = 0;
    bool        is_recprefinal_enab = true;
    bool        is_recfinal_enab = true;
    /* Index into our AoW specific mcs_info array */
    u_int8_t    rateIndex;
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ath_softc *sc = ATH_DEV_TO_SC(scn->sc_dev);
    const HAL_RATE_TABLE *rt = sc->sc_rates[sc->sc_curmode];
    int ret = 0;
    
    struct mcs_stats *mstats = NULL;

    *num_attempts = 0; 

    ret = map_recvr_to_mcs_stats(ic, recvr_addr, &mstats);

    if (ret) {
        return;
    }
    
    max_prefinal_index = ts->ts_rateindex - 1;

    for (i = 0; i <= max_prefinal_index; i++)
    {
        rix = bf->bf_rcs[i].rix;
        tries =  bf->bf_rcs[i].tries;
        
        rateIndex = rt->info[rix].rate_code - AOW_ATH_MCS0;

        if (rateIndex > (ATH_AOW_NUM_MCS - 1) || tries > ATH_AOW_TXMAXTRY) {
            is_recprefinal_enab = false;
        }
        
        for (j = 0; j < tries; j++)
        {
            if (is_recprefinal_enab) {
                mstats->mcs_info[rateIndex].nok_txcount[j]++;
            }
            lnum_attempts++;
        }
    }
    
    rix = bf->bf_rcs[ts->ts_rateindex].rix;
    tries =  bf->bf_rcs[ts->ts_rateindex].tries;
        
    rateIndex = rt->info[rix].rate_code - AOW_ATH_MCS0;

    if (rateIndex > (ATH_AOW_NUM_MCS - 1) || tries > ATH_AOW_TXMAXTRY) {
        is_recfinal_enab = false;        
        return;
    }
        
    for (j = 0; j < ts->ts_longretry; j++)
    {
        if (is_recfinal_enab) {
            mstats->mcs_info[rateIndex].nok_txcount[j]++;
        }
        lnum_attempts++;
    }
    
    if (!is_success) {
        if (ts->ts_status == 0 && bf->bf_isaggr) {
            /* The MPDU was part of an aggregate transmission which succeeded
               Increment failure count for this unlucky MPDU alone. */
             if (is_recfinal_enab) {
                mstats->mcs_info[rateIndex].nok_txcount[j]++;
            }
            lnum_attempts++;
        }
        
        *num_attempts = lnum_attempts;

        return;
    }

    if (is_recfinal_enab) {
        mstats->mcs_info[rateIndex].ok_txcount[j]++;
    }
    lnum_attempts++;
    *num_attempts = lnum_attempts;
}

/*
 * function : aow_update_txstatus
 * ----------------------------------
 * Extended stats: Update Tx status
 *
 */
void aow_update_txstatus(struct ieee80211com *ic, struct ath_buf *bf, struct ath_tx_status *ts)
{
    u_int16_t txstatus;
    wbuf_t wbuf;
    struct audio_pkt *apkt;
    struct ether_addr recvr_addr;
    struct ath_buf *bfl;
    bool is_success = false;
    bool is_aggr = false;
    int num_attempts = 0;
    int16_t apktindex = -1;
#if ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG
    struct aow_advncd_txinfo atxinfo;
    int loopcount = 0;
#endif

    if (ic != aowinfo.ic ||
        (!AOW_ES_ENAB(ic) && !AOW_ESS_ENAB(ic))) {
        return;
    }

    is_aggr = bf->bf_isaggr;
   
    bfl = bf;

    while (bfl != NULL)
    {
        wbuf = (wbuf_t)bfl->bf_mpdu;

        apktindex = ieee80211_aow_apktindex(ic, wbuf);

        if (apktindex < 0) {
            return;
        }

        apkt = ATH_AOW_ACCESS_APKT(wbuf, apktindex);
        
#if ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG
        loopcount++;
#endif
        if (!AOW_ES_ENAB(ic) && AOW_ESS_ENAB(ic)
            &&  !AOW_ESS_SYNC_SET(apkt->params)) {
            bfl =  bfl->bf_next;
            continue;
        }

        is_success = aow_istxsuccess(ic, bfl, ts, is_aggr);
        
        IEEE80211_ADDR_COPY(&recvr_addr, ATH_AOW_ACCESS_RECVR(wbuf)); 

        /* We pass bf instead of bfl because there is a chance
           that the rate series array for bfl would not be valid. */
        aow_update_mcsinfo(ic, bf, &recvr_addr, ts, is_success, &num_attempts);

        txstatus =
            is_success ? ATH_AOW_PL_INFO_TX_SUCCESS : ATH_AOW_PL_INFO_TX_FAIL;

#if ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG
           aow_populate_advncd_txinfo(&atxinfo, ts, bfl, loopcount);
#endif

           aow_nl_send_txpl(ic,
                         recvr_addr,
                         apkt->seqno,
                         apkt->params & ATH_AOW_PARAMS_SUB_FRAME_NO_MASK,
                         ATH_AOW_ACCESS_WLANSEQ(wbuf),
                         txstatus,
#if ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG
                         &atxinfo,
#endif
                         num_attempts);

        bfl =  bfl->bf_next;
    }
}


/* Application Messaging (AM) Services. Netlink is used. */

/*
 * function : aow_am_init
 * ----------------------------------
 * Application Messaging: Initialize Netlink operation.
 *
 */
int aow_am_init(struct ieee80211com *ic)
{
    if (ic->ic_aow.aow_nl_sock == NULL) {
        ic->ic_aow.aow_nl_sock = 
            (struct sock *) OS_NETLINK_CREATE(NETLINK_ATHEROS_AOW_ES,
                                              1,
                                              NULL,
                                              THIS_MODULE);

        if (ic->ic_aow.aow_nl_sock == NULL) {
            IEEE80211_AOW_DPRINTF("OS_NETLINK_CREATE() failed\n");
            return -ENODEV;
        }
    }
    return 0;
}

/*
 * function : aow_am_deinit
 * ----------------------------------
 * Application Messaging: De-Initialize Netlink operation.
 *
 */
void aow_am_deinit(struct ieee80211com *ic)
{
    if (ic->ic_aow.aow_nl_sock) {
            OS_SOCKET_RELEASE(ic->ic_aow.aow_nl_sock);
            ic->ic_aow.aow_nl_sock = NULL;
    }
}

/*
 * function : aow_am_send
 * ----------------------------------
 * Application Messaging: Send data using netlink.
 *
 */
int aow_am_send(struct ieee80211com *ic, char *data, int size)
{
    wbuf_t  nlwbuf;
    void    *nlh;
    
    if (ic->ic_aow.aow_nl_sock == NULL) {
        return -1;
    }
   
    nlwbuf = wbuf_alloc(ic->ic_osdev,
                        WBUF_MAX_TYPE,
                        OS_NLMSG_SPACE(AOW_NL_DATA_MAX_SIZE));

    if (nlwbuf != NULL) {
        wbuf_append(nlwbuf, OS_NLMSG_SPACE(AOW_NL_DATA_MAX_SIZE));
    } else {        
        IEEE80211_AOW_DPRINTF("%s: wbuf_alloc() failed\n", __func__);
        return -1;
    }
    
    nlh = wbuf_raw_data(nlwbuf);
    OS_SET_NETLINK_HEADER(nlh, NLMSG_LENGTH(size), 0, 0, 0, 0);

    /* sender is in group 1<<0 */
    wbuf_set_netlink_pid(nlwbuf, 0);      /* from kernel */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
    wbuf_set_netlink_dst_pid(nlwbuf, 0);  /* multicast */
#endif 
    /* to mcast group 1<<0 */
    wbuf_set_netlink_dst_group(nlwbuf, 1);
    
    OS_MEMCPY(OS_NLMSG_DATA(nlh), data, OS_NLMSG_SPACE(size));

    OS_NETLINK_BCAST(ic->ic_aow.aow_nl_sock, nlwbuf, 0, 1, GFP_ATOMIC);

    return 0;
}


/*
 * function : aow_nl_send_rxpl
 * ----------------------------------
 * Extended stats: Send a pl_element entry (Rx) to the application
 * layer via Netlink socket.
 *
 */
int aow_nl_send_rxpl(struct ieee80211com *ic,
                     u_int32_t seqno,
                     u_int8_t subfrme_seqno,
                     u_int16_t wlan_seqno,
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                     u_int64_t timestamp,
                     u_int64_t rxTime,
                     u_int64_t deliveryTime,
#endif
                     u_int8_t rxstatus)
{
    struct aow_nl_packet nlpkt;

    if (!AOW_ES_ENAB(ic) && !AOW_ESS_ENAB(ic)) {
        return -EINVAL;
    }

    ic->ic_aow.node_type = ATH_AOW_NODE_TYPE_RX;
    
    nlpkt.header = ATH_AOW_NODE_TYPE_RX & ATH_AOW_NL_TYPE_MASK;
    
    nlpkt.body.rxdata.elmnt.seqno = seqno;
    
    nlpkt.body.rxdata.elmnt.subfrme_wlan_seqnos = 
        subfrme_seqno & ATH_AOW_PL_SUBFRME_SEQ_MASK; 
    
    nlpkt.body.rxdata.elmnt.subfrme_wlan_seqnos |=
        ((wlan_seqno & ATH_AOW_PL_WLAN_SEQ_MASK) << ATH_AOW_PL_WLAN_SEQ_S);
    
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
    nlpkt.body.rxdata.elmnt.arxinfo.timestamp = timestamp;
    nlpkt.body.rxdata.elmnt.arxinfo.rxTime = rxTime;
    nlpkt.body.rxdata.elmnt.arxinfo.deliveryTime = deliveryTime;
#endif

    nlpkt.body.rxdata.elmnt.info = rxstatus;
    
    aow_am_send(ic,
                (char*)&nlpkt,
                AOW_NL_PACKET_RX_SIZE);
    
    return 0;
}

/*
 * function : aow_nl_send_txpl
 * ----------------------------------
 * Extended stats: Send a pl_element entry (Tx) to the application
 * layer via Netlink socket.
 *
 */
int aow_nl_send_txpl(struct ieee80211com *ic,
                     struct ether_addr recvr_addr,
                     u_int32_t seqno,
                     u_int8_t subfrme_seqno,
                     u_int16_t wlan_seqno,
                     u_int8_t txstatus,
#if ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG
                     struct aow_advncd_txinfo *atxinfo,
#endif
                     u_int8_t numretries)
{
    struct aow_nl_packet nlpkt;

    if (!AOW_ES_ENAB(ic) && !AOW_ESS_ENAB(ic)) {
        return -EINVAL;
    }

    ic->ic_aow.node_type = ATH_AOW_NODE_TYPE_TX;

    nlpkt.header = ATH_AOW_NODE_TYPE_TX & ATH_AOW_NL_TYPE_MASK;
    
    nlpkt.body.txdata.elmnt.seqno               = seqno;
    
    nlpkt.body.txdata.elmnt.subfrme_wlan_seqnos =
        subfrme_seqno & ATH_AOW_PL_SUBFRME_SEQ_MASK; 
    
    nlpkt.body.txdata.elmnt.subfrme_wlan_seqnos |=
        ((wlan_seqno & ATH_AOW_PL_WLAN_SEQ_MASK) << ATH_AOW_PL_WLAN_SEQ_S);

    nlpkt.body.txdata.elmnt.info                =
        numretries & ATH_AOW_PL_NUM_ATTMPTS_MASK;

    nlpkt.body.txdata.elmnt.info                |=
        (txstatus & ATH_AOW_PL_TX_STATUS_MASK) << ATH_AOW_PL_TX_STATUS_S;

#if ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG
    OS_MEMCPY(&nlpkt.body.txdata.elmnt.atxinfo,
              atxinfo,
              sizeof(struct aow_advncd_txinfo));
#endif

    IEEE80211_ADDR_COPY(&nlpkt.body.txdata.recvr, &recvr_addr);

    aow_am_send(ic,
                (char*)&nlpkt,
                AOW_NL_PACKET_TX_SIZE);
    
    return 0;
}

/* Operations on ESS Counts */

/*
 * function : aow_essc_init
 * ----------------------------------
 * Extended stats (Synchronized): Initialize ESS Count Maintenance
 *
 */
void aow_essc_init(struct ieee80211com *ic)
{
   AOW_ESSC_LOCK(ic);
   
   OS_MEMSET(ic->ic_aow.ess_count, 0, sizeof(ic->ic_aow.ess_count)); 

   AOW_ESSC_UNLOCK(ic);
}

/*
 * function : aow_essc_setall
 * ----------------------------------
 * Extended stats (Synchronized): Set ESS Count for all channels
 *
 */

void aow_essc_setall(struct ieee80211com *ic, u_int32_t val)
{
    int i;

    if (!AOW_ESS_ENAB(ic)) {
        return;
    }

    AOW_ESSC_LOCK(ic);
    
    for (i = 0; i < AOW_MAX_AUDIO_CHANNELS; i++)
    {
        ic->ic_aow.ess_count[i] = val;
    }


    AOW_ESSC_UNLOCK(ic);
}

/*
 * function : aow_essc_testdecr
 * ----------------------------------
 * Extended stats (Synchronized): Test and decrement ESS Count for a given
 * channel.
 * - If the count is non-zero, then it is decremented and true is returned.
 * - If the count is zero or ESS is not enabled, then false is returned.
 */
bool aow_essc_testdecr(struct ieee80211com *ic, int channel)
{
    if (!AOW_ESS_ENAB(ic)) {
        return false;
    }

    AOW_ESSC_LOCK(ic);
   
    if (ic->ic_aow.ess_count[channel] == 0) {
        AOW_ESSC_UNLOCK(ic);
        return false;
    } else {
        ic->ic_aow.ess_count[channel]--;
        AOW_ESSC_UNLOCK(ic);
        return true;
    }

    AOW_ESSC_UNLOCK(ic);
}

/*
 * function : aow_essc_deinit
 * ----------------------------------
 * Extended stats (Synchronized): De-Initialize ESS Count Maintenance
 *
 */
void aow_essc_deinit(struct ieee80211com *ic)
{
    AOW_ESSC_LOCK(ic);
   
    OS_MEMSET(ic->ic_aow.ess_count, 0, sizeof(ic->ic_aow.ess_count)); 

    AOW_ESSC_UNLOCK(ic);
}

/* Operations on L2 Packet Error Stats */

/*
 * function: aow_l2pe_init()
 * ---------------------------------
 * Extended stats: Initialize L2 Packet Error Stats measurement.
 *
 */
int aow_l2pe_init(struct ieee80211com *ic)
{
    int retval;

    AOW_L2PE_LOCK(ic);    
    
    if ((retval = aow_l2pe_fifo_init(ic)) < 0) {
       AOW_L2PE_UNLOCK(ic);    
       return retval;
    } 

    OS_MEMSET(&ic->ic_aow.estats.l2pe_curr,
              0,
              sizeof(ic->ic_aow.estats.l2pe_curr));
    
    OS_MEMSET(&ic->ic_aow.estats.l2pe_cumltv,
              0,
              sizeof(ic->ic_aow.estats.l2pe_cumltv));

    ic->ic_aow.estats.l2pe_next_srNo = 1;

    ic->ic_aow.estats.l2pe_curr.isInUse = false;
    ic->ic_aow.estats.l2pe_curr.wasAudStopped = false;

    ic->ic_aow.estats.l2pe_cumltv.start_time =
        OS_GET_TIMESTAMP();
    
    AOW_L2PE_UNLOCK(ic);    
    
    return 0;
}

/*
 * function: aow_l2pe_deinit()
 * ---------------------------------
 * Extended stats: De-Initialize L2 Packet Error Stats measurement.
 *
 */
void aow_l2pe_deinit(struct ieee80211com *ic)
{
    AOW_L2PE_LOCK(ic);    
    
    aow_l2pe_fifo_deinit(ic);

    OS_MEMSET(&ic->ic_aow.estats.l2pe_curr,
              0,
              sizeof(ic->ic_aow.estats.l2pe_curr));
    
    OS_MEMSET(&ic->ic_aow.estats.l2pe_cumltv,
              0,
              sizeof(ic->ic_aow.estats.l2pe_cumltv));

    ic->ic_aow.estats.l2pe_next_srNo = 0;
    
    ic->ic_aow.estats.l2pe_curr.isInUse = false;
    ic->ic_aow.estats.l2pe_curr.wasAudStopped = false;
    
    AOW_L2PE_UNLOCK(ic);    
}


/*
 * function: aow_l2pe_reset()
 * ---------------------------------
 * Extended stats: Re-Set L2 Packet Error Stats measurement.
 *
 */
void aow_l2pe_reset(struct ieee80211com *ic)
{
    AOW_L2PE_LOCK(ic);    
    
    aow_l2pe_fifo_reset(ic);
    
    OS_MEMSET(&ic->ic_aow.estats.l2pe_curr,
              0,
              sizeof(ic->ic_aow.estats.l2pe_curr));
    
    OS_MEMSET(&ic->ic_aow.estats.l2pe_cumltv,
              0,
              sizeof(ic->ic_aow.estats.l2pe_cumltv));

    ic->ic_aow.estats.l2pe_next_srNo = 1;
    
    ic->ic_aow.estats.l2pe_curr.isInUse = false;
    ic->ic_aow.estats.l2pe_curr.wasAudStopped = false;
    
    AOW_L2PE_UNLOCK(ic);    

}

/*
 * function: aow_l2pe_audstopped()
 * ---------------------------------
 * Extended stats: Inform the L2 Packet Error Stats measurement
 * module that audio stop was detected.
 *
 */
void aow_l2pe_audstopped(struct ieee80211com *ic)
{
    ic->ic_aow.estats.l2pe_curr.wasAudStopped = true;
}

/*
 * function: aow_l2pe_record()
 * ---------------------------------
 * Extended stats: Record an L2 Packet Error/Success case.
 *
 */
int aow_l2pe_record(struct ieee80211com *ic, bool is_success)
{
    systime_t curr_time;
    systime_t prev_start_time; 

    if (NULL == ic->ic_aow.estats.aow_l2pe_stats) {
        return -EINVAL;
    }
    
    AOW_L2PE_LOCK(ic); 
    
    curr_time = OS_GET_TIMESTAMP();

    prev_start_time = ic->ic_aow.estats.l2pe_curr.start_time;

    if (!ic->ic_aow.estats.l2pe_curr.isInUse) {
        aow_l2peinst_init(&ic->ic_aow.estats.l2pe_curr,
                          ic->ic_aow.estats.l2pe_next_srNo++,
                          curr_time);

    } else if (ic->ic_aow.estats.l2pe_curr.wasAudStopped) { 
        
        aow_l2pe_fifo_enqueue(ic, &ic->ic_aow.estats.l2pe_curr);

        aow_l2peinst_init(&ic->ic_aow.estats.l2pe_curr,
                          ic->ic_aow.estats.l2pe_next_srNo++,
                          curr_time);

    } else if (curr_time - prev_start_time >= ATH_AOW_ES_GROUP_INTERVAL_MS) {
        aow_l2pe_fifo_enqueue(ic, &ic->ic_aow.estats.l2pe_curr);
        aow_l2peinst_init(&ic->ic_aow.estats.l2pe_curr,
                          ic->ic_aow.estats.l2pe_next_srNo++,
                          prev_start_time + ATH_AOW_ES_GROUP_INTERVAL_MS);
    }
    
    if (is_success) {
        ic->ic_aow.estats.l2pe_curr.fcs_ok++;
        ic->ic_aow.estats.l2pe_cumltv.fcs_ok++;
    } else {
        ic->ic_aow.estats.l2pe_curr.fcs_nok++;
        ic->ic_aow.estats.l2pe_cumltv.fcs_nok++;
    
    }
    
    ic->ic_aow.estats.l2pe_cumltv.end_time = curr_time;
    
    AOW_L2PE_UNLOCK(ic);    

    return 0;
}

/*
 * function: aow_l2pe_print_inst()
 * ---------------------------------
 * Extended stats: Print L2 Packet Error Rate statistics for 
 * an L2PE group instance.
 *
 */
static inline int aow_l2pe_print_inst(struct ieee80211com *ic,
                                      struct l2_pkt_err_stats *l2pe_inst)
{
    u_int32_t ratio_int_part;
    u_int32_t ratio_frac_part;
    int retval;
    
    IEEE80211_AOW_DPRINTF("No. of MPDUs with FCS passed = %u\n",
                          l2pe_inst->fcs_ok);
    IEEE80211_AOW_DPRINTF("No. of MPDUs with FCS failed = %u\n",
                          l2pe_inst->fcs_nok);

    if (0 == l2pe_inst->fcs_ok && 0 == l2pe_inst->fcs_nok)
    {
       IEEE80211_AOW_DPRINTF("L2 Packet Error Rate : NA\n");

    } else {

        if ((retval = uint_floatpt_division(l2pe_inst->fcs_nok,
                                            l2pe_inst->fcs_ok + l2pe_inst->fcs_nok,
                                            &ratio_int_part,
                                            &ratio_frac_part,
                                            ATH_AOW_ES_PRECISION_MULT)) < 0) {
            return retval;
        }
    
       IEEE80211_AOW_DPRINTF("L2 Packet Error Rate = %u.%0*u\n",
                             ratio_int_part,
                             ATH_AOW_ES_PRECISION,
                             ratio_frac_part);
    }

    return 0;
}

/*
 * function: aow_l2pe_print()
 * ---------------------------------
 * Extended stats: Print L2 Packet Error Rate statistics.
 *
 */
int aow_l2pe_print(struct ieee80211com *ic)
{
    struct l2_pkt_err_stats l2pe_stats[ATH_AOW_MAX_ES_GROUPS - 1];
    int i = 0;
    int count = 0;
    int retval = 0;

    AOW_L2PE_LOCK(ic);
    
    if (NULL == ic->ic_aow.estats.aow_l2pe_stats) {
        IEEE80211_AOW_DPRINTF("\n------------------------------------------\n");
        IEEE80211_AOW_DPRINTF("\nL2 Packet Error Statistics not available\n");
        IEEE80211_AOW_DPRINTF("------------------------------------------\n");
        AOW_L2PE_UNLOCK(ic);
        return -1;
    }

    IEEE80211_AOW_DPRINTF("\n--------------------------------------------\n");
    IEEE80211_AOW_DPRINTF("\nL2 Packet Error Statistics\n");
    IEEE80211_AOW_DPRINTF("--------------------------------------------\n");
        
    retval = aow_l2pe_fifo_copy(ic, l2pe_stats, &count);

    if (retval >= 0 || ic->ic_aow.estats.l2pe_curr.isInUse) {
        
        IEEE80211_AOW_DPRINTF("\nStats at %ds time intervals"
                          " (from oldest to latest):\n\n",
                          ATH_AOW_ES_GROUP_INTERVAL);

        if (retval >= 0)
        {
            for (i = 0; i < count; i++) {
                IEEE80211_AOW_DPRINTF("Sr. No. %u\n", l2pe_stats[i].srNo);
                aow_l2pe_print_inst(ic, &l2pe_stats[i]);
                IEEE80211_AOW_DPRINTF("\n");
            }
        }

        if (ic->ic_aow.estats.l2pe_curr.isInUse) {
            IEEE80211_AOW_DPRINTF("Sr. No. %d\n",
                                  ic->ic_aow.estats.l2pe_curr.srNo);
            aow_l2pe_print_inst(ic, &ic->ic_aow.estats.l2pe_curr);
            IEEE80211_AOW_DPRINTF("\n");
        }
    }

    IEEE80211_AOW_DPRINTF("Cumulative Stats:\n");
    aow_l2pe_print_inst(ic, &ic->ic_aow.estats.l2pe_cumltv);
    IEEE80211_AOW_DPRINTF("\n");
    
    AOW_L2PE_UNLOCK(ic);

    return 0; 
}
 
/* Operations on FIFO for L2 Packet Error Stats */
/* The FIFO will contain (ATH_AOW_MAX_ES_GROUPS - 1) entries */

/* TODO: Low priority: Create a common FIFO framework for 
   L2 Packet Errors and Packet Loss Stats */

/*
 * function: aow_l2pe_fifo_init()
 * ---------------------------------
 * Extended stats: Initialize FIFO for L2 Packet Error Stats.
 *
 * This function is to be called ONLY by the aow_l2pe*() functions.
 *
 */
static int aow_l2pe_fifo_init(struct ieee80211com *ic)
{
   ic->ic_aow.estats.aow_l2pe_stats =
       (struct l2_pkt_err_stats *)OS_MALLOC(ic->ic_osdev,
                                            ATH_AOW_MAX_ES_GROUPS *\
                                               sizeof(struct l2_pkt_err_stats),
                                            GFP_KERNEL);
    
   if (NULL == ic->ic_aow.estats.aow_l2pe_stats)
   {
        IEEE80211_AOW_DPRINTF("%s: Could not allocate memory\n", __func__);
        return -ENOMEM;
   }

   OS_MEMSET(ic->ic_aow.estats.aow_l2pe_stats,
             0,
             ATH_AOW_MAX_ES_GROUPS * sizeof(struct l2_pkt_err_stats));

   ic->ic_aow.estats.aow_l2pe_stats_write = 0;
   ic->ic_aow.estats.aow_l2pe_stats_read = 0;

   return 0; 
}

/*
 * function: aow_l2pe_fifo_reset()
 * ---------------------------------
 * Extended stats: Reset FIFO for L2 Packet Error Stats.
 *
 * This function is to be called ONLY by the aow_l2pe*() functions.
 *
 */
static int aow_l2pe_fifo_reset(struct ieee80211com *ic)
{
    if (NULL == ic->ic_aow.estats.aow_l2pe_stats) {
        return -EINVAL;
    }
    
    ic->ic_aow.estats.aow_l2pe_stats_write = 0;
    ic->ic_aow.estats.aow_l2pe_stats_read = 0;
    
    OS_MEMSET(ic->ic_aow.estats.aow_l2pe_stats,
             0,
             ATH_AOW_MAX_ES_GROUPS * sizeof(struct l2_pkt_err_stats));

    return 0;
}

/*
 * function: aow_l2pe_fifo_enqueue()
 * ---------------------------------
 * Extended stats: Enqueue an L2 Packet Error Stats Group instance into
 * into the FIFO.
 *
 * This function is to be called ONLY by the aow_l2pe*() functions.
 */
static int aow_l2pe_fifo_enqueue(struct ieee80211com *ic,
                                 struct l2_pkt_err_stats *l2pe_instnce)
{
    int aow_l2pe_stats_write;

    if (NULL == ic->ic_aow.estats.aow_l2pe_stats) {
        return -EINVAL;
    }

    aow_l2pe_stats_write = ic->ic_aow.estats.aow_l2pe_stats_write;
  
    /* We have the design option of directly storing pointers into the FIFO, or
       querying the FIFO for the pointer for writing information.
       However, the OS_MEMCPY() involves very few bytes, and that too, mostly
       every ATH_AOW_ES_GROUP_INTERVAL seconds, which is very large.
       Hence, we adopt a cleaner approach. */

    OS_MEMCPY(&ic->ic_aow.estats.aow_l2pe_stats[aow_l2pe_stats_write],
              l2pe_instnce,
              sizeof(struct l2_pkt_err_stats));

    ic->ic_aow.estats.aow_l2pe_stats_write++;
    if (ic->ic_aow.estats.aow_l2pe_stats_write == ATH_AOW_MAX_ES_GROUPS) {
        ic->ic_aow.estats.aow_l2pe_stats_write = 0;
    }
    
    if (ic->ic_aow.estats.aow_l2pe_stats_write == ic->ic_aow.estats.aow_l2pe_stats_read) {
        ic->ic_aow.estats.aow_l2pe_stats_read++;
        if (ic->ic_aow.estats.aow_l2pe_stats_read == ATH_AOW_MAX_ES_GROUPS) {
            ic->ic_aow.estats.aow_l2pe_stats_read = 0;
        }
    }

    return 0;
}

/*
 * function : aow_l2pe_fifo_copy()
 * ----------------------------------
 * Extended stats: Copy contents of L2 Packet Error Stats FIFO into an
 * array, in increasing order of age.
 * The FIFO contents are not deleted.
 * -EOVERFLOW is returned if the FIFO is empty.
 * (Note: EOVERFLOW is the closest OS independant error definition that seems
 * to be currently available).
 *
 * This function is to be called ONLY by the aow_l2pe*() functions.
 *
 */
static int aow_l2pe_fifo_copy(struct ieee80211com *ic,
                              struct l2_pkt_err_stats *aow_l2pe_stats,
                              int *count)
{
    int writepos = ic->ic_aow.estats.aow_l2pe_stats_write;
    int readpos = ic->ic_aow.estats.aow_l2pe_stats_read;
    
    int ph1copy_start = -1; /* Starting position for phase 1 copy */
    int ph1copy_count = 0;  /* Byte count for phase 1 copy */
    int ph2copy_start = -1; /* Starting position for phase 2 copy */
    int ph2copy_count = 0;  /* Byte count for phase 2 copy */

    if (writepos == readpos) {
        return -EOVERFLOW;
    }

    *count = 0;
    
    ph1copy_start = readpos;
    
    if (readpos < writepos) {
        /* writepos itself contains no valid data */
        ph1copy_count = (writepos - readpos);
        /* Phase 2 copy not required */
        ph2copy_count = 0;
    } else { 
        ph1copy_count = ATH_AOW_MAX_ES_GROUPS - readpos;
        if (writepos == 0) {
            ph2copy_count = 0;
        } else {
            ph2copy_start = 0;
            ph2copy_count = writepos;
        }
    }

    OS_MEMCPY(aow_l2pe_stats,
              ic->ic_aow.estats.aow_l2pe_stats + ph1copy_start,
              ph1copy_count * sizeof(struct l2_pkt_err_stats));

    if (ph2copy_count > 0)
    {
        OS_MEMCPY(aow_l2pe_stats + ph1copy_count,
                  ic->ic_aow.estats.aow_l2pe_stats + ph2copy_start,
                  ph2copy_count * sizeof(struct l2_pkt_err_stats));
    }
    
    *count = ph1copy_count + ph2copy_count;

    return 0;
}

/*
 * function: aow_l2pe_fifo_deinit()
 * ---------------------------------
 * Extended stats: De-Initialize FIFO for L2 Packet Error Stats.
 *
 * This function is to be called ONLY by the aow_l2pe*() functions.
 *
 */
static void aow_l2pe_fifo_deinit(struct ieee80211com *ic)
{
    if (ic->ic_aow.estats.aow_l2pe_stats != NULL)
    {    
        OS_FREE(ic->ic_aow.estats.aow_l2pe_stats);
        ic->ic_aow.estats.aow_l2pe_stats = NULL;
        ic->ic_aow.estats.aow_l2pe_stats_write = 0;
        ic->ic_aow.estats.aow_l2pe_stats_read = 0;
    }
}

/*
 * function: aow_l2peinst_init()
 * ---------------------------------
 * Extended stats: Initialize an L2 Packet Error Stats Group instance.
 *
 * This function is to be called ONLY by the aow_l2pe*() functions.
 *
 */
static inline void aow_l2peinst_init(struct l2_pkt_err_stats *l2pe_inst,
                                     u_int32_t srNo,
                                     systime_t start_time)
{
    l2pe_inst->srNo = srNo;
    l2pe_inst->start_time = start_time;
    l2pe_inst->fcs_ok = 0;
    l2pe_inst->fcs_nok = 0;
    l2pe_inst->isInUse = true;
    l2pe_inst->wasAudStopped = false;
}

/* Operations on Packet Lost Rate Records */

/*
 * function: aow_plr_init()
 * ---------------------------------
 * Extended stats: Initialize Packet Lost Rate measurement.
 *
 */
int aow_plr_init(struct ieee80211com *ic)
{
    int retval;

    AOW_PLR_LOCK(ic);    
    
    if ((retval = aow_plr_fifo_init(ic)) < 0) {
       AOW_PLR_UNLOCK(ic);    
       return retval;
    } 

    OS_MEMSET(&ic->ic_aow.estats.plr_curr,
              0,
              sizeof(ic->ic_aow.estats.plr_curr));
    
    OS_MEMSET(&ic->ic_aow.estats.plr_cumltv,
              0,
              sizeof(ic->ic_aow.estats.plr_cumltv));

    ic->ic_aow.estats.plr_next_srNo = 1;

    ic->ic_aow.estats.plr_curr.isInUse = false;
    ic->ic_aow.estats.plr_curr.wasAudStopped = false;

    ic->ic_aow.estats.plr_cumltv.isInUse = false;
    
    ic->ic_aow.estats.plr_cumltv.start_time =
        OS_GET_TIMESTAMP();
    
    AOW_PLR_UNLOCK(ic);    
    
    return 0;
}

/*
 * function: aow_plr_deinit()
 * ---------------------------------
 * Extended stats: De-Initialize Packet Lost Rate measurement.
 *
 */
void aow_plr_deinit(struct ieee80211com *ic)
{
    AOW_PLR_LOCK(ic);    
    
    aow_plr_fifo_deinit(ic);

    OS_MEMSET(&ic->ic_aow.estats.plr_curr,
              0,
              sizeof(ic->ic_aow.estats.plr_curr));
    
    OS_MEMSET(&ic->ic_aow.estats.plr_cumltv,
              0,
              sizeof(ic->ic_aow.estats.plr_cumltv));

    ic->ic_aow.estats.plr_next_srNo = 0;
    
    ic->ic_aow.estats.plr_curr.isInUse = false;
    ic->ic_aow.estats.plr_curr.wasAudStopped = false;
    
    ic->ic_aow.estats.plr_cumltv.isInUse = false;
    
    AOW_PLR_UNLOCK(ic);    
}


/*
 * function: aow_plr_reset()
 * ---------------------------------
 * Extended stats: Re-Set Packet Lost Rate measurement.
 *
 */
void aow_plr_reset(struct ieee80211com *ic)
{
    AOW_PLR_LOCK(ic);    
    
    aow_plr_fifo_reset(ic);
    
    OS_MEMSET(&ic->ic_aow.estats.plr_curr,
              0,
              sizeof(ic->ic_aow.estats.plr_curr));
    
    OS_MEMSET(&ic->ic_aow.estats.plr_cumltv,
              0,
              sizeof(ic->ic_aow.estats.plr_cumltv));

    ic->ic_aow.estats.plr_next_srNo = 1;
    
    ic->ic_aow.estats.plr_curr.isInUse = false;
    ic->ic_aow.estats.plr_curr.wasAudStopped = false;
    
    ic->ic_aow.estats.plr_cumltv.isInUse = false;
    
    AOW_PLR_UNLOCK(ic);    

}

/*
 * function: aow_plr_audstopped()
 * ---------------------------------
 * Extended stats: Inform the Packet Lost Rate measurement
 * module that audio stop was detected.
 *
 */
void aow_plr_audstopped(struct ieee80211com *ic)
{
    ic->ic_aow.estats.plr_curr.wasAudStopped = true;
}

/*
 * function: aow_plr_record()
 * ---------------------------------
 * Extended stats: Record an increment in a Packet Lost Rate related
 * statistic.
 *
 */
int aow_plr_record(struct ieee80211com *ic,
                   u_int32_t seqno,
                   u_int16_t subseqno,
                   u_int8_t type)
{
    systime_t curr_time;
    systime_t prev_start_time;

    if (NULL == ic->ic_aow.estats.aow_plr_stats) {
        return -EINVAL;
    }
    
    AOW_PLR_LOCK(ic); 
    
    curr_time = OS_GET_TIMESTAMP();

    prev_start_time = ic->ic_aow.estats.plr_curr.start_time;

    if (!ic->ic_aow.estats.plr_cumltv.isInUse) {
        aow_plrinst_init(&ic->ic_aow.estats.plr_cumltv,
                          0,
                          seqno,
                          subseqno,
                          ic->ic_aow.estats.plr_cumltv.start_time);
    }

    if (!ic->ic_aow.estats.plr_curr.isInUse) {
        aow_plrinst_init(&ic->ic_aow.estats.plr_curr,
                          ic->ic_aow.estats.plr_next_srNo++,
                          seqno,
                          subseqno,
                          curr_time);

    } else if (ic->ic_aow.estats.plr_curr.wasAudStopped) { 
        
        aow_plr_fifo_enqueue(ic, &ic->ic_aow.estats.plr_curr);

        aow_plrinst_init(&ic->ic_aow.estats.plr_curr,
                          ic->ic_aow.estats.plr_next_srNo++,
                          seqno,
                          subseqno,
                          curr_time);

    } else if (curr_time - prev_start_time >= ATH_AOW_ES_GROUP_INTERVAL_MS) {
        aow_plr_fifo_enqueue(ic, &ic->ic_aow.estats.plr_curr);
        aow_plrinst_init(&ic->ic_aow.estats.plr_curr,
                          ic->ic_aow.estats.plr_next_srNo++,
                          seqno,
                          subseqno,
                          prev_start_time + ATH_AOW_ES_GROUP_INTERVAL_MS);
    }
   
    aow_plrinst_update_start(&ic->ic_aow.estats.plr_cumltv, seqno, subseqno); 
    aow_plrinst_update_start(&ic->ic_aow.estats.plr_curr, seqno, subseqno); 
    
    switch(type)
    {
        case ATH_AOW_PLR_TYPE_NUM_MPDUS:
            ic->ic_aow.estats.plr_curr.num_mpdus++;
            ic->ic_aow.estats.plr_cumltv.num_mpdus++;
            break;

        case ATH_AOW_PLR_TYPE_NOK_FRMCNT:
            ic->ic_aow.estats.plr_curr.nok_framecount++;
            ic->ic_aow.estats.plr_cumltv.nok_framecount++;
            break;

        case ATH_AOW_PLR_TYPE_LATE_MPDU:
            ic->ic_aow.estats.plr_curr.late_mpdu++;
            ic->ic_aow.estats.plr_cumltv.late_mpdu++;
            break;
    }
    
    
    ic->ic_aow.estats.plr_cumltv.end_time = curr_time;
    aow_plrinst_update_end(&ic->ic_aow.estats.plr_cumltv, seqno, subseqno); 
    aow_plrinst_update_end(&ic->ic_aow.estats.plr_curr, seqno, subseqno); 
    
    AOW_PLR_UNLOCK(ic);    

    return 0;
}

/*
 * function: aow_plr_print_inst()
 * ---------------------------------
 * Extended stats: Print Packet Lost Rate statistics for 
 * a PLR group instance.
 *
 */
static inline int aow_plr_print_inst(struct ieee80211com *ic,
                                     struct pkt_lost_rate_stats *plr_inst)
{
    u_int32_t ratio_int_part;
    u_int32_t ratio_frac_part;
    long all_mpdus = 0;
    u_int32_t lost_mpdus = 0;
    int retval;
    
    IEEE80211_AOW_DPRINTF("Starting sequence no = %u\n",
                          plr_inst->start_seqno);
    
    IEEE80211_AOW_DPRINTF("Starting subsequence no = %hu\n",
                          plr_inst->start_subseqno);
    
    IEEE80211_AOW_DPRINTF("Ending sequence no = %u\n",
                          plr_inst->end_seqno);
    
    IEEE80211_AOW_DPRINTF("Ending subsequence no = %hu\n",
                          plr_inst->end_subseqno);

    /* TODO: As at present, we allow frame size changes only
       on reboot. However, if start allowing dynamic changes,
       then this code will need to be tweaked. */
 
    if (!plr_inst->isInUse) {
        all_mpdus = 0;
    } else if ((all_mpdus = aow_compute_nummpdus(ic->ic_aow.frame_size,
                                                 plr_inst->start_seqno,
                                                 plr_inst->start_subseqno,
                                                 plr_inst->end_seqno,
                                                 plr_inst->end_subseqno)) < 0) {
        IEEE80211_AOW_DPRINTF("Sequence no. roll-over detected\n");
        return -1;
    }

    lost_mpdus = all_mpdus - plr_inst->num_mpdus;

    IEEE80211_AOW_DPRINTF("Number of MPDUs received without fail = %u\n",
                          plr_inst->num_mpdus);
    
    IEEE80211_AOW_DPRINTF("Delayed frames (Receive stats) = %u\n",
                          plr_inst->nok_framecount);
    
    IEEE80211_AOW_DPRINTF("Number of MPDUs that came late = %u\n",
                          plr_inst->late_mpdu);
    
    IEEE80211_AOW_DPRINTF("Number of all MPDUs = %u\n",
                          (u_int32_t)all_mpdus);
    
    IEEE80211_AOW_DPRINTF("Number of lost MPDUs = %u\n",
                          lost_mpdus);

    
    if (0 == all_mpdus)
    {
       IEEE80211_AOW_DPRINTF("Packet Lost Rate : NA\n");

    } else {

        if ((retval = uint_floatpt_division((u_int32_t)lost_mpdus,
                                            all_mpdus,
                                            &ratio_int_part,
                                            &ratio_frac_part,
                                            ATH_AOW_ES_PRECISION_MULT)) < 0) {
            return retval;
        }
    
       IEEE80211_AOW_DPRINTF("Packet Lost Rate = %u.%0*u\n",
                             ratio_int_part,
                             ATH_AOW_ES_PRECISION,
                             ratio_frac_part);
    }

    return 0;
}

/*
 * function: aow_plr_print()
 * ---------------------------------
 * Extended stats: Print Packet Lost Rate statistics.
 *
 */
int aow_plr_print(struct ieee80211com *ic)
{
    struct pkt_lost_rate_stats plr_stats[ATH_AOW_MAX_ES_GROUPS - 1];
    int i = 0;
    int count = 0;
    int retval = 0;

    AOW_PLR_LOCK(ic);
    
    if (NULL == ic->ic_aow.estats.aow_plr_stats) {
        IEEE80211_AOW_DPRINTF("\n------------------------------------------\n");
        IEEE80211_AOW_DPRINTF("\nPacket Lost Rate Statistics not available\n");
        IEEE80211_AOW_DPRINTF("------------------------------------------\n");
        AOW_PLR_UNLOCK(ic);
        return -1;
    }

    IEEE80211_AOW_DPRINTF("\n--------------------------------------------\n");
    IEEE80211_AOW_DPRINTF("\nPacket Lost Rate Statistics\n");
    IEEE80211_AOW_DPRINTF("--------------------------------------------\n");
        
    retval = aow_plr_fifo_copy(ic, plr_stats, &count);

    if (retval >= 0 || ic->ic_aow.estats.plr_curr.isInUse) {
        
        IEEE80211_AOW_DPRINTF("\nStats at %ds time intervals"
                          " (from oldest to latest):\n\n",
                          ATH_AOW_ES_GROUP_INTERVAL);

        if (retval >= 0)
        {
            for (i = 0; i < count; i++) {
                IEEE80211_AOW_DPRINTF("Sr. No. %u\n", plr_stats[i].srNo);
                aow_plr_print_inst(ic, &plr_stats[i]);
                IEEE80211_AOW_DPRINTF("\n");
            }
        }

        if (ic->ic_aow.estats.plr_curr.isInUse) {
            IEEE80211_AOW_DPRINTF("Sr. No. %d\n",
                                  ic->ic_aow.estats.plr_curr.srNo);
            aow_plr_print_inst(ic, &ic->ic_aow.estats.plr_curr);
            IEEE80211_AOW_DPRINTF("\n");
        }
    }

    IEEE80211_AOW_DPRINTF("Cumulative Stats:\n");
    aow_plr_print_inst(ic, &ic->ic_aow.estats.plr_cumltv);
    IEEE80211_AOW_DPRINTF("\n");
    
    AOW_PLR_UNLOCK(ic);

    return 0; 
}
 
/* Operations on FIFO for Packet Lost Rate */
/* The FIFO will contain (ATH_AOW_MAX_ES_GROUPS - 1) entries */

/* TODO: Low priority: Create a common FIFO framework for 
   L2 Packet Errors and Packet Loss Stats */

/*
 * function: aow_plr_fifo_init()
 * ---------------------------------
 * Extended stats: Initialize FIFO for Packet Lost Rate.
 *
 * This function is to be called ONLY by the aow_plr*() functions.
 *
 */
static int aow_plr_fifo_init(struct ieee80211com *ic)
{
   ic->ic_aow.estats.aow_plr_stats =
       (struct pkt_lost_rate_stats *)OS_MALLOC(ic->ic_osdev,
                                            ATH_AOW_MAX_ES_GROUPS *\
                                               sizeof(struct pkt_lost_rate_stats),
                                            GFP_KERNEL);
    
   if (NULL == ic->ic_aow.estats.aow_plr_stats)
   {
        IEEE80211_AOW_DPRINTF("%s: Could not allocate memory\n", __func__);
        return -ENOMEM;
   }

   OS_MEMSET(ic->ic_aow.estats.aow_plr_stats,
             0,
             ATH_AOW_MAX_ES_GROUPS * sizeof(struct pkt_lost_rate_stats));

   ic->ic_aow.estats.aow_plr_stats_write = 0;
   ic->ic_aow.estats.aow_plr_stats_read = 0;

   return 0; 
}

/*
 * function: aow_plr_fifo_reset()
 * ---------------------------------
 * Extended stats: Reset FIFO for Packet Lost Rate.
 *
 * This function is to be called ONLY by the aow_plr*() functions.
 *
 */
static int aow_plr_fifo_reset(struct ieee80211com *ic)
{
    if (NULL == ic->ic_aow.estats.aow_plr_stats) {
        return -EINVAL;
    }
    
    ic->ic_aow.estats.aow_plr_stats_write = 0;
    ic->ic_aow.estats.aow_plr_stats_read = 0;
    
    OS_MEMSET(ic->ic_aow.estats.aow_plr_stats,
             0,
             ATH_AOW_MAX_ES_GROUPS * sizeof(struct pkt_lost_rate_stats));

    return 0;
}

/*
 * function: aow_plr_fifo_enqueue()
 * ---------------------------------
 * Extended stats: Enqueue an Packet Lost Rate Group instance into
 * into the FIFO.
 *
 * This function is to be called ONLY by the aow_plr*() functions.
 */
static int aow_plr_fifo_enqueue(struct ieee80211com *ic,
                                struct pkt_lost_rate_stats *plr_instnce)
{
    int aow_plr_stats_write;

    if (NULL == ic->ic_aow.estats.aow_plr_stats) {
        return -EINVAL;
    }

    aow_plr_stats_write = ic->ic_aow.estats.aow_plr_stats_write;
  
    /* We have the design option of directly storing pointers into the FIFO, or
       querying the FIFO for the pointer for writing information.
       However, the OS_MEMCPY() involves very few bytes, and that too, mostly
       every ATH_AOW_ES_GROUP_INTERVAL seconds, which is very large.
       Hence, we adopt a cleaner approach. */

    OS_MEMCPY(&ic->ic_aow.estats.aow_plr_stats[aow_plr_stats_write],
              plr_instnce,
              sizeof(struct pkt_lost_rate_stats));

    ic->ic_aow.estats.aow_plr_stats_write++;
    if (ic->ic_aow.estats.aow_plr_stats_write == ATH_AOW_MAX_ES_GROUPS) {
        ic->ic_aow.estats.aow_plr_stats_write = 0;
    }
    
    if (ic->ic_aow.estats.aow_plr_stats_write == ic->ic_aow.estats.aow_plr_stats_read) {
        ic->ic_aow.estats.aow_plr_stats_read++;
        if (ic->ic_aow.estats.aow_plr_stats_read == ATH_AOW_MAX_ES_GROUPS) {
            ic->ic_aow.estats.aow_plr_stats_read = 0;
        }
    }

    return 0;
}

/*
 * function : aow_plr_fifo_copy()
 * ----------------------------------
 * Extended stats: Copy contents of Packet Lost Rate FIFO into an
 * array, in increasing order of age.
 * The FIFO contents are not deleted.
 * -EOVERFLOW is returned if the FIFO is empty.
 * (Note: EOVERFLOW is the closest OS independant error definition that seems
 * to be currently available).
 *
 * This function is to be called ONLY by the aow_plr*() functions.
 *
 */
static int aow_plr_fifo_copy(struct ieee80211com *ic,
                             struct pkt_lost_rate_stats *aow_plr_stats,
                             int *count)
{
    int writepos = ic->ic_aow.estats.aow_plr_stats_write;
    int readpos = ic->ic_aow.estats.aow_plr_stats_read;
    
    int ph1copy_start = -1; /* Starting position for phase 1 copy */
    int ph1copy_count = 0;  /* Byte count for phase 1 copy */
    int ph2copy_start = -1; /* Starting position for phase 2 copy */
    int ph2copy_count = 0;  /* Byte count for phase 2 copy */

    if (writepos == readpos) {
        return -EOVERFLOW;
    }

    *count = 0;
    
    ph1copy_start = readpos;
    
    if (readpos < writepos) {
        /* writepos itself contains no valid data */
        ph1copy_count = (writepos - readpos);
        /* Phase 2 copy not required */
        ph2copy_count = 0;
    } else { 
        ph1copy_count = ATH_AOW_MAX_ES_GROUPS - readpos;
        if (writepos == 0) {
            ph2copy_count = 0;
        } else {
            ph2copy_start = 0;
            ph2copy_count = writepos;
        }
    }

    OS_MEMCPY(aow_plr_stats,
              ic->ic_aow.estats.aow_plr_stats + ph1copy_start,
              ph1copy_count * sizeof(struct pkt_lost_rate_stats));

    if (ph2copy_count > 0)
    {
        OS_MEMCPY(aow_plr_stats + ph1copy_count,
                  ic->ic_aow.estats.aow_plr_stats + ph2copy_start,
                  ph2copy_count * sizeof(struct pkt_lost_rate_stats));
    }
    
    *count = ph1copy_count + ph2copy_count;

    return 0;
}

/*
 * function: aow_plr_fifo_deinit()
 * ---------------------------------
 * Extended stats: De-Initialize FIFO for Packet Lost Rate.
 *
 * This function is to be called ONLY by the aow_plr*() functions.
 *
 */
static void aow_plr_fifo_deinit(struct ieee80211com *ic)
{
    if (ic->ic_aow.estats.aow_plr_stats != NULL)
    {    
        OS_FREE(ic->ic_aow.estats.aow_plr_stats);
        ic->ic_aow.estats.aow_plr_stats = NULL;
        ic->ic_aow.estats.aow_plr_stats_write = 0;
        ic->ic_aow.estats.aow_plr_stats_read = 0;
    }
}

/*
 * function: aow_plrinst_init()
 * ---------------------------------
 * Extended stats: Initialize an Packet Lost Rate Group instance.
 *
 * This function is to be called ONLY by the aow_plr*() functions.
 *
 */
static inline void aow_plrinst_init(struct pkt_lost_rate_stats *plr_inst,
                                    u_int32_t srNo,
                                    u_int32_t seqno,
                                    u_int16_t subseqno,
                                    systime_t start_time)
{
    plr_inst->srNo = srNo;
    plr_inst->start_seqno = seqno;
    plr_inst->start_subseqno = subseqno;
    plr_inst->start_time = start_time;
    plr_inst->num_mpdus = 0;
    plr_inst->nok_framecount = 0;
    plr_inst->late_mpdu = 0;
    plr_inst->isInUse = true;
    plr_inst->wasAudStopped = false;
}

/*
 * function: aow_plrinst_update_start()
 * ---------------------------------
 * Extended stats: Check and rewind start sequence numbers in Packet Lost
 * Rate Group instance if required.
 *
 * This function is to be called ONLY by the aow_plr*() functions.
 *
 */
static inline void aow_plrinst_update_start(struct pkt_lost_rate_stats *plr_inst,
                                            u_int32_t seqno,
                                            u_int16_t subseqno)
{
    if (seqno < plr_inst->start_seqno) {
        plr_inst->start_seqno = seqno;
        plr_inst->start_subseqno = subseqno;
    } else if ((seqno == plr_inst->start_seqno) &&
               (subseqno < plr_inst->start_subseqno)) {
        plr_inst->start_subseqno = subseqno;
    }
}

/*
 * function: aow_plrinst_update_end()
 * ---------------------------------
 * Extended stats: Check and fast-forwad end sequence numbers in Packet Lost
 * Rate Group instance if required.
 *
 * This function is to be called ONLY by the aow_plr*() functions.
 *
 */
static inline void aow_plrinst_update_end(struct pkt_lost_rate_stats *plr_inst,
                                          u_int32_t seqno,
                                          u_int16_t subseqno)
{
    if (seqno > plr_inst->end_seqno) {
        plr_inst->end_seqno = seqno;
        plr_inst->end_subseqno = subseqno;
    } else if ((seqno == plr_inst->end_seqno) &&
               (subseqno > plr_inst->end_subseqno)) {
        plr_inst->end_subseqno = subseqno;
    }
}


/*
 * function: aow_mcss_print()
 * ---------------------------------
 * Extended stats: Print MCS statistics.
 *
 */
void aow_mcss_print(struct ieee80211com *ic)
{
    int i, j, k;
    struct mcs_stats *mstats = NULL;
    struct ether_addr *addr = NULL;
    
    IEEE80211_AOW_DPRINTF("\n--------------------------------------------\n");
    IEEE80211_AOW_DPRINTF("MCS Statistics\n");
    IEEE80211_AOW_DPRINTF("--------------------------------------------\n");

    for (i = 0; i < AOW_MAX_RECVRS; i++) {

        mstats = &ic->ic_aow.estats.aow_mcs_stats[i];

        if (!mstats->populated) {
            break;
        }
        
        addr = &ic->ic_aow.estats.aow_mcs_stats[i].recvr_addr;

        /* TODO: Can use a ready-made pretty print function if available */
        IEEE80211_AOW_DPRINTF("\n\nReceiver MAC address: "
                              "%02x:%02x:%02x:%02x:%02x:%02x\n",
                              addr->octet[0],
                              addr->octet[1],
                              addr->octet[2],
                              addr->octet[3],
                              addr->octet[4],
                              addr->octet[5]);

        IEEE80211_AOW_DPRINTF("--------------------------------------------\n");
        
        for (j = 0; j < ATH_AOW_NUM_MCS; j++)
        {
            IEEE80211_AOW_DPRINTF("\nMCS%d:\n", j);
            IEEE80211_AOW_DPRINTF("Attempt No    Successes        Failures\n");

            for (k = 0; k < ATH_AOW_TXMAXTRY; k++) {
                IEEE80211_AOW_DPRINTF("%-13u %-16u %-u\n",
                                      k + 1,
                                      mstats->mcs_info[j].ok_txcount[k],
                                      mstats->mcs_info[j].nok_txcount[k]);
            }
        }
    }
}

/*
 * function: uint_floatpt_division
 * ---------------------------------
 * Carry out floating point division on u_int32_teger
 * dividend and divisor, to obtain u_int32_t integer
 * component, and fraction component expressed as an
 * u_int32_t by multiplying it by a power of 10, 'precision_mult'
 *
 * Note: This is a simple function which will not handle large
 * values. The values it can handle will depend upon precision_mult.
 * Besides, it doesn't process the case where dividend is
 * larger than divisor.
 * It is written for use in ES/ESS extended stat reporting (where
 * the dividend is always <= divisor).
 * We avoid use of softfloat packages for efficiency.
 *
 */
static inline int uint_floatpt_division(const u_int32_t dividend,
                                        const u_int32_t divisor,
                                        u_int32_t *integer,
                                        u_int32_t *fraction,
                                        const u_int32_t precision_mult)
{
    
#define ATH_AOW_UINT_MAX_VAL 4294967295U
    
    if ((0 == divisor) ||
        (precision_mult > ATH_AOW_ES_MAX_PRECISION_MULT) ||
        (dividend > (ATH_AOW_UINT_MAX_VAL/precision_mult)) ||
        (dividend > divisor)) {
        return -EINVAL;
    }

    if (dividend == divisor) {
        *integer = 1;
        *fraction = 0;
    } else {
        *integer = 0;
        *fraction = (dividend * precision_mult) / divisor ;
    }

    return 0;
}

/*
 * function: aow_compute_nummpdus()
 * ---------------------------------
 * Compute the number of MPDUs given the 
 * starting and ending AoW sequence+subsequence numbers.
 * 
 * This is a simple function intended primarily for use in
 * ES/ESS stats. It does not handle sequence number roll-over,
 * as at present (it merely returns -1 in this case).
 * TODO: Handle rollover, if required in the future.
 *
 */
static inline long aow_compute_nummpdus(u_int16_t frame_size,
                                        u_int32_t start_seqno,
                                        u_int8_t start_subseqno,
                                        u_int32_t end_seqno,
                                        u_int8_t end_subseqno)
{
    long num_mpdus = 0;

    num_mpdus = (end_subseqno - start_subseqno) + 
                (end_seqno - start_seqno) * frame_size
                + 1;
                

    if (num_mpdus < 0) {
        return -1;
    } else {
        return num_mpdus;
    }
}


#if ATH_SUPPORT_AOW_DEBUG

/*
 * function : aow_dbg_init
 * ----------------------------------
 * Initialize the stats members, currently used to debug Clock Sync
 *
 */
static int aow_dbg_init(struct aow_dbg_stats* p)
{
    int i = 0;

    p->index = 0;
    p->sum = 0;
    p->max = 0;
    p->min = 0xffffffff;
    p->pretsf = 0;
    p->prnt_count = 0;
    p->wait = TRUE;

    for (i = 0; i < AOW_MAX_TSF_COUNT; i++) {
        p->tsf[i] = 0;
    }        

    return 0;
}

/*
 * function : aow_update_dbg_stats
 * ----------------------------------
 * Update the time related stats
 *
 */

static int aow_update_dbg_stats(struct aow_dbg_stats *p, u_int64_t val)
{
    p->pretsf = p->tsf[p->index];
    p->tsf[p->index] = val;
    p->sum = p->sum + val - p->pretsf;
    p->avg = p->sum >> 8;


    /* wait for one complete cycle to remove the wrong
     * values
     */
    if (!p->wait) {
        if (p->min > p->tsf[p->index])
            p->min = p->tsf[p->index];

        if (p->max < p->tsf[p->index])
            p->max = p->tsf[p->index];
    }        

    p->index = ((p->index + 1) & 0xff);
    p->prnt_count++;

    if (p->prnt_count == AOW_DBG_PRINT_LIMIT) {
        IEEE80211_AOW_DPRINTF("avg : %llu  min : %llu  max : %llu \n", 
            p->avg, p->min, p->max);
        p->prnt_count = 0;
        p->wait = FALSE;
    }        
}

#endif  /* ATH_SUPPORT_AOW_DEBUG */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ieee80211_aow_detail_sync_stats
 *  Description:  Prints detail Audio Sync stats
 * =====================================================================================
 */

void ieee80211com_aow_detail_sync_stats(struct ieee80211com *ic)
{

    u_int32_t cnt = 0;
    audio_stats *cur_stats  = &ic->ic_aow.stats;
    if (cur_stats->rxFlg ) {
        IEEE80211_AOW_DPRINTF("-----------------------------------------------------------------------------------------------------------\n");
        IEEE80211_AOW_DPRINTF("idx         = Index\n");
        IEEE80211_AOW_DPRINTF("USB_diff    = Time drift between host and Rx in micro seconds\n");
        IEEE80211_AOW_DPRINTF("samp_con    = Sample consumed in 1sec\n");
        IEEE80211_AOW_DPRINTF("samp_in_buf = Sample to adjust i.e drop(+)/add(-))\n");
        IEEE80211_AOW_DPRINTF("samp_adj    = Samples in buffer (Least Count 6)\n");
        IEEE80211_AOW_DPRINTF("interval    = Audio sync window\n");
        IEEE80211_AOW_DPRINTF("\tidx\tUSB_Diff\tsamp_con\tsamp_in_buf\tsamp_adj\tinterval\n");
        IEEE80211_AOW_DPRINTF("-----------------------------------------------------------------------------------------------------------\n");
    } else {
        IEEE80211_AOW_DPRINTF("-----------------------------------------------------------------------------------------------------------\n");
        IEEE80211_AOW_DPRINTF("idx        = Index\n");
        IEEE80211_AOW_DPRINTF("timeDiff  = Time difference\n");
        IEEE80211_AOW_DPRINTF("total Time   = Total time\n");
        IEEE80211_AOW_DPRINTF("rd_before  = Read index (pre)\n");
        IEEE80211_AOW_DPRINTF("samp_adj   = Sample to adjust i.e drop/add\n");
        IEEE80211_AOW_DPRINTF("interval   = Audio sync window\n");
        IEEE80211_AOW_DPRINTF("idx\twr_before\tsamp_con\trd_before\tsamp_adj\tinterval\n");
        IEEE80211_AOW_DPRINTF("-----------------------------------------------------------------------------------------------------------\n");
    }    
    for (cnt = 0; cnt < ic->ic_aow.sync_stats.wlan_data_buf_idx; cnt++) {
        IEEE80211_AOW_DPRINTF("   %4d\t\t%6d\t\t%6d\t\t%6d\t\t%4d\t\t%4d\n",
                              cnt, 
                              ic->ic_aow.sync_stats.wlan_wr_ptr_before[cnt],
                              ic->ic_aow.sync_stats.wlan_wr_ptr_after[cnt],
                              ic->ic_aow.sync_stats.wlan_rd_ptr_before[cnt],
                              //ic->ic_aow.sync_stats.wlan_rd_ptr_after[cnt],
                              ic->ic_aow.sync_stats.i2s_disCnt[cnt],
                              ic->ic_aow.sync_stats.i2s_timeDiff[cnt]
                             );
    }
    IEEE80211_AOW_DPRINTF("\nTotal Sample Adj count = %d\n", ic->ic_aow.sync_stats.tot_samp_adj);
    IEEE80211_AOW_DPRINTF("-----------------------------------------------------------------------------------------------------------\n");
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  wlan_aow_register_calls_to_usb
 *  Description:  Registers WLAN calls to USB dev
 * =====================================================================================
 */

int wlan_aow_register_calls_to_usb(aow_dev_t* dev)
{
    dev->tx.set_audioparams = wlan_aow_set_audioparams;    
    dev->tx.send_data = wlan_aow_tx;
#if ATH_SUPPORT_AOW_TXSCHED
    dev->tx.dispatch_data = wlan_aow_dispatch_data;
#endif 
    dev->tx.send_ctrl = ieee80211_aow_tx_ctrl;
    dev->tx.get_tsf   = wlan_get_tsf;
    return 0;
}EXPORT_SYMBOL(wlan_aow_register_calls_to_usb);


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  aow_register_usb_calls_to_wlan
 *  Description:  Part of USB API, wlan will use this api to register USB functions to
 *                WLAN device
 *              
 * =====================================================================================
 */

int aow_register_usb_calls_to_wlan(void* recv_data,
                                   void* recv_ctrl,
                                   void* set_frame_size,
                                   void* set_alt_setting)
{
    struct ieee80211com *ic = aowinfo.ic;

    wlan_aow_dev.rx.recv_data = recv_data;
    wlan_aow_dev.rx.recv_ctrl = recv_ctrl;
    wlan_aow_dev.rx.set_frame_size = set_frame_size;
    wlan_aow_dev.rx.set_alt_setting = set_alt_setting;

    /* Set frame size immediately. */ 
    ieee80211_aow_setframesize_to_usb(ic->ic_aow.frame_size);
    ic->ic_aow.frame_size_set = 1;

    /* Set alt setting immediately. */
    ieee80211_aow_setaltsetting_to_usb(ic->ic_aow.alt_setting);
    ic->ic_aow.alt_setting_set = 1;

    return 0;
}EXPORT_SYMBOL(aow_register_usb_calls_to_wlan);


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  is_aow_usb_calls_registered
 *  Description:  Checks if USB APIs are registered with WLAN or not
 * =====================================================================================
 */
int is_aow_usb_calls_registered(void)
{
    int is_registered = 0; 

    if (wlan_aow_dev.rx.recv_ctrl != NULL) { 
         is_registered = 1; 
    }

    return is_registered;
}    

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ieee80211_aow_sync_stats
 *  Description:  Prints the Audio Sync related stats
 * =====================================================================================
 */

void ieee80211_aow_sync_stats(struct ieee80211com *ic)
{
    IEEE80211_AOW_DPRINTF("\nAudio Sync Stats\n");
    IEEE80211_AOW_DPRINTF("--------------------------------------------\n");
    IEEE80211_AOW_DPRINTF("SYNC Overflow           = %d\n", ic->ic_aow.sync_stats.overflow);
    IEEE80211_AOW_DPRINTF("SYNC Underflow          = %d\n", ic->ic_aow.sync_stats.i2s_sample_repeat_count);
    IEEE80211_AOW_DPRINTF("SYNC Timing error       = %d\n", ic->ic_aow.sync_stats.underflow);

    /* Call ieee80211com_aow_detail_sync_stats() here for more detailed sync stats */
//    if (AOW_AS_ENAB(ic))
        ieee80211com_aow_detail_sync_stats(ic);
}


/**
 * @brief       init routine for CI function
 * @param[in]   ic   : handle ic data structure
 * @return           : Zero for success and non zero for failure
 */
int aow_init_ci(struct ieee80211com *ic)
{
    if (ic->ic_aow.aow_ci_sock == NULL) {
        ic->ic_aow.aow_ci_sock = 
            (struct sock *) OS_NETLINK_CREATE(NETLINK_ATHEROS_AOW_CM,
                                                1,
                                                NULL,
                                                THIS_MODULE);
        if (ic->ic_aow.aow_ci_sock == NULL) {
            IEEE80211_AOW_DPRINTF("OS_NETLINK_CREATE() failed\n");
            return -ENODEV;
        }
    }        
    return 0;
}    

/**
 * @brief       Deinit routine for CI function
 * @param[in]   ic   : handle ic data structure
 */
void aow_deinit_ci(struct ieee80211com* ic)
{
    if (ic->ic_aow.aow_ci_sock) {
        OS_SOCKET_RELEASE(ic->ic_aow.aow_ci_sock);
        ic->ic_aow.aow_ci_sock = NULL;
    }
}    

/**
 * @brief       Send the AoW command message to CI app
 * @param[in]   ic   : handle ic data structure
 * @param[in]   data : handle the message 
 * @param[in]   size : size of the message
 * @return           : zero for success and non zero value for failure
 */
                 
int aow_ci_send(struct ieee80211com *ic, char *data, int size)
{
    wbuf_t  nlwbuf;
    void    *nlh;

    if (ic->ic_aow.aow_ci_sock == NULL) {
        IEEE80211_AOW_DPRINTF("%s : Error, invalid socket\n", __func__);
        return -1;
    }
   
    nlwbuf = wbuf_alloc(ic->ic_osdev,
                        WBUF_MAX_TYPE,
                        OS_NLMSG_SPACE(AOW_CTRL_MSG_MAX_LENGTH));

    if (nlwbuf != NULL) {        
        wbuf_append(nlwbuf, OS_NLMSG_SPACE(AOW_CTRL_MSG_MAX_LENGTH));
    } else {
        IEEE80211_AOW_DPRINTF("%s: wbuf_alloc() failed\n", __func__);
        return -1;
    }
    
    nlh = wbuf_raw_data(nlwbuf);

    OS_SET_NETLINK_HEADER(nlh, NLMSG_LENGTH(size), 0, 0, 0, 0);

    /* sender is in group 1<<0 */
    wbuf_set_netlink_pid(nlwbuf, 0);      /* from kernel */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
    wbuf_set_netlink_dst_pid(nlwbuf, 0);  /* multicast */
#endif 

    /* to mcast group 1 << 0, CI uses 3 */
    wbuf_set_netlink_dst_group(nlwbuf, 3);
    
    /* copy message to buffer */
    OS_MEMCPY(OS_NLMSG_DATA(nlh), data, OS_NLMSG_SPACE(size));

    /* send the message */
    OS_NETLINK_BCAST(ic->ic_aow.aow_ci_sock, nlwbuf, 0, 1, GFP_ATOMIC);

    return 0;

}

/**
 * @brief       : Init VAP related AoW info
 * @param[in]   : vap : Pointer to VAP
 * @return      : TRUE for Success, FALSE for Failure 
 */

int ieee80211_aow_vattach(struct ieee80211vap *vap)
{
    struct wlan_mlme_app_ie* ie = NULL;
    int ret = TRUE; 

    IEEE80211_AOW_DPRINTF("AoW : vap attach\n");

    ie = wlan_mlme_app_ie_create(vap);
    if (ie == NULL) {
        IEEE80211_AOW_DPRINTF("%s : Error in allocating application IE\n",
            __func__);
        ret = FALSE;
    } else {
        vap->iv_aow.app_ie_handle = ie;
    }
    
    /* Set up rates */
    ieee80211_aow_setup_rates(vap);
    
    /* Set the management frame rate to 6M including beacon rate */
    wlan_set_param(vap, IEEE80211_MGMT_RATE, IEEE80211_AOW_MIN_RATE);

    /* Set the multicast frame rate to 6M (convert ieee rate to Kbps, the param takes kbps) */
    /* Though AoW won't be using Multicast, we take this measure for completeness. */
    wlan_set_param(vap, IEEE80211_MCAST_RATE, (IEEE80211_AOW_MIN_RATE * 1000) / 2);

    return ret;
}    

/**
 * @brief       : Deinit VAP related AoW info
 * @param[in]   : Pointer to VAP
 * @return      : Success/Failure 
 */

int ieee80211_aow_vdetach(struct ieee80211vap *vap)
{
    int ret = TRUE;
    IEEE80211_AOW_DPRINTF("AoW : vap detach\n");

    if (vap->iv_aow.app_ie_handle != NULL) {
        wlan_mlme_app_ie_delete(vap->iv_aow.app_ie_handle);
    }
    return ret;
}    


/**
 * @brief       : Update the AoW related information element
 * @param[in]   : Pointer to VAP
 * @return      : Success/Failure 
 */

int ieee80211_update_aow_ie(struct ieee80211com* ic, u_int8_t* ie, int len)
{
    struct ieee80211vap *vap;
    spin_lock(ic->ic_lock);
    OS_MEMCPY(&ic->ic_aow.ie, ie,  len);

    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
        if (vap) {
            IEEE80211_VAP_APPIE_UPDATE_DISABLE(vap);
            ieee80211_aow_ie_attach(vap);
            IEEE80211_VAP_APPIE_UPDATE_ENABLE(vap);
        }
    }        
    spin_unlock(ic->ic_lock);
    return 0;
}

/**
 * @brief       : Delete the AoW related information element
 * @param[in]   : Pointer to VAP
 * @return      : Success/Failure 
 */

int ieee80211_delete_aow_ie(struct ieee80211com* ic)
{
    struct ieee80211vap *vap;
    spin_lock(ic->ic_lock);
    OS_MEMSET(&ic->ic_aow.ie, 0, sizeof(aow_ie_t));

    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
        if (vap) {
            IEEE80211_VAP_APPIE_UPDATE_DISABLE(vap);
            ieee80211_aow_ie_detach(vap);
            IEEE80211_VAP_APPIE_UPDATE_ENABLE(vap);
        }
    }        
    spin_unlock(ic->ic_lock);
    return 0;
}

/**
 * @brief       : Adds AoW IE specific into the APP IE buffer
 * @param[in]   : Pointer to VAP
 * @return      : Success/Failure 
 */

int ieee80211_aow_ie_attach(struct ieee80211vap* vap)
{
    struct ieee80211com* ic = vap->iv_ic;
    aow_ie_t *ie;
    u_int32_t len = sizeof(aow_ie_t);

    AOW_LOCK(ic);
    ie = &ic->ic_aow.ie;
    wlan_mlme_app_ie_set(vap->iv_aow.app_ie_handle, IEEE80211_FRAME_TYPE_BEACON, (u_int8_t*)ie, len);
    wlan_mlme_app_ie_set(vap->iv_aow.app_ie_handle, IEEE80211_FRAME_TYPE_PROBEREQ, (u_int8_t*)ie, len);
    wlan_mlme_app_ie_set(vap->iv_aow.app_ie_handle, IEEE80211_FRAME_TYPE_PROBERESP, (u_int8_t*)ie, len);
    wlan_mlme_app_ie_set(vap->iv_aow.app_ie_handle, IEEE80211_FRAME_TYPE_ASSOCREQ, (u_int8_t*)ie, len);
    wlan_mlme_app_ie_set(vap->iv_aow.app_ie_handle, IEEE80211_FRAME_TYPE_ASSOCRESP, (u_int8_t*)ie, len);
    wlan_mlme_app_ie_set(vap->iv_aow.app_ie_handle, IEEE80211_FRAME_TYPE_AUTH, (u_int8_t*)ie, len);
    AOW_UNLOCK(ic);
    
    return 0;
}

/**
 * @brief       : Removes AoW IE specific into the APP IE buffer
 * @param[in]   : Pointer to VAP
 * @return      : Success/Failure 
 */

int ieee80211_aow_ie_detach(struct ieee80211vap* vap)
{
    struct ieee80211com* ic = vap->iv_ic;
    aow_ie_t *ie = NULL;
    u_int32_t len = 0;

    AOW_LOCK(ic);
    wlan_mlme_app_ie_set(vap->iv_aow.app_ie_handle, IEEE80211_FRAME_TYPE_BEACON, (u_int8_t*)ie, len);
    wlan_mlme_app_ie_set(vap->iv_aow.app_ie_handle, IEEE80211_FRAME_TYPE_PROBEREQ, (u_int8_t*)ie, len);
    wlan_mlme_app_ie_set(vap->iv_aow.app_ie_handle, IEEE80211_FRAME_TYPE_PROBERESP, (u_int8_t*)ie, len);
    wlan_mlme_app_ie_set(vap->iv_aow.app_ie_handle, IEEE80211_FRAME_TYPE_ASSOCREQ, (u_int8_t*)ie, len);
    wlan_mlme_app_ie_set(vap->iv_aow.app_ie_handle, IEEE80211_FRAME_TYPE_ASSOCRESP, (u_int8_t*)ie, len);
    wlan_mlme_app_ie_set(vap->iv_aow.app_ie_handle, IEEE80211_FRAME_TYPE_AUTH, (u_int8_t*)ie, len);
    AOW_UNLOCK(ic);
    
    return 0;
}

/**
 * @brief       : Checks if the remote node is AoW device and checks assoc policy.
 *              : The info is updated by Beacon/Probe request processing
 * @param[in]   : Pointer to VAP
 * @return      : Success/Failure 
 */

 int is_aow_assoc_only_set(struct ieee80211com* ic, struct ieee80211_node* ni)
 {
    int allow  = AH_TRUE;

    /* ignore the self referential node */
    if ((ni) == ni->ni_vap->iv_bss) {
        return allow;
    }        

    if (!IS_AOW_ASSOC_SET(ic)) {
        return allow;
    } else {
        if ((ni->ni_aow_ie != NULL) && IS_NI_AOW_CAPABLE(ni)) {
            allow = AH_TRUE;
        } else {
            allow = AH_FALSE;
        }            
    }

    return allow;
 }

/**
 * @brief       : Sets the AoW capabilty information for the given node
 *              : The info is updated by Beacon/Probe request processing
 * @param[in]   : Pointer to VAP
 * @return      : Success/Failure 
 */

int update_aow_capability_for_node(struct ieee80211_node* ni)
{
   SET_AOW_DEV_CAPABILITY(ni);
   return 0;
}   

/**
 * @brief       : Initialize  version info
 * @param[in]   : Pointer to IC
 */
int init_aow_verinfo(struct ieee80211com* ic)
{
    ic->ic_aow.version = AOW_RELEASE_VERSION << AOW_RELEASE_VERSION_OFFSET;
    ic->ic_aow.version |= AOW_VERSION_MAJOR_NUM << AOW_VERSION_MAJOR_NUM_OFFSET;
    ic->ic_aow.version |= AOW_VERSION_MINOR_NUM; 
    return 0;
}

/**
 * @brief       : Initialize the AoW IE
 *              : The info is updated by Beacon/Probe request processing
 * @param[in]   : Pointer to VAP
 * @return      : Success/Failure 
 */
int init_aow_ie(struct ieee80211com* ic)
{
    aow_ie_t *ie = &ic->ic_aow.ie;

    ie->elemid = AOW_IE_ELEMID;
    ie->len    = AOW_IE_LENGTH;
    ie->oui[0] = (AOW_OUI & 0xff);
    ie->oui[1] = (AOW_OUI >> 8) & 0xff;
    ie->oui[2] = (AOW_OUI >> 16) & 0xff;
    ie->oui_type = AOW_OUI_TYPE;
    ie->version = AOW_OUI_VERSION;
    ie->id = 1;
    ie->dev_type = 1;
    ie->audio_type = 0;
    ie->party_mode = 0;

    return 0;
}

/**
 * @brief       : Print AoW OUI
 * @param[in]   : Pointer to IC
 * @return      : Success/Failure 
 */
void print_aow_ie(struct ieee80211com* ic)
{
    aow_ie_t *ie = &ic->ic_aow.ie;
    IEEE80211_AOW_DPRINTF("Element ID = 0x%x\n", ie->elemid);
    IEEE80211_AOW_DPRINTF("Length     = %d\n", ie->len);
    IEEE80211_AOW_DPRINTF("OUI[0]     = 0x%x\n", ie->oui[0]);
    IEEE80211_AOW_DPRINTF("OUI[1]     = 0x%x\n", ie->oui[1]);
    IEEE80211_AOW_DPRINTF("OUI[2]     = 0x%x\n", ie->oui[2]);
    IEEE80211_AOW_DPRINTF("OUI Type   = 0x%x\n", ie->oui_type);
    IEEE80211_AOW_DPRINTF("Version    = 0x%x\n", ie->version);
    IEEE80211_AOW_DPRINTF("AOW ID     = 0x%x\n", ie->id);
    IEEE80211_AOW_DPRINTF("Device     = 0x%x\n", ie->dev_type);
}    

/**
 * @brief       : Update AOW related info in to node info
 * @param[in]   : Pointer to node, pointer to aow ie
 * @return      : void
 */
void ieee80211_update_node_aow_info(struct ieee80211_node* ni, aow_ie_t *ie)
{
    KASSERT((ie != NULL), ("IE is NULL"));
    KASSERT((ni != NULL), ("Node is NULL"));

    /* XXX : Should have the correct endianess */

    ni->ni_aow.capable = AH_TRUE;
    ni->ni_aow.dev_type = ie->dev_type;
    ni->ni_aow.id = ie->id;
    ni->ni_aow.audio_type = ie->audio_type;
    ni->ni_aow.party_mode = ie->party_mode;
    SET_AOW_DEV_CAPABILITY(ni);

}

/**
 * @brief       : Setup AOW Node related info
 * @param[in]   : Pointer to node, pointer to aow ie
 * @return      : void
 */
int ieee80211_setup_node_aow(struct ieee80211_node *ni, ieee80211_scan_entry_t scan_entry)
{
    aow_ie_t *ie = (aow_ie_t*)ieee80211_scan_entry_aow(scan_entry);

    if (ie) {
        ieee80211_update_node_aow_info(ni, ie);
    } else {
        CLR_AOW_DEV_CAPABILITY(ni);
        ni->ni_aow.capable = AH_FALSE;
    }
    return 0;
}

/**
 * @brief       : Check if the AoW IE ID match
 * @param[in]   : Receiver AoW IE
 * @return      : True/False
 * 
 ni*/
int is_aow_id_match(struct ieee80211com* ic, aow_ie_t* ie)
{
    int id_match = FALSE;
    if (ic->ic_aow.ie.id == ie->id) {
        id_match = TRUE;
    }        
    return id_match;
}

int ieee80211_print_tx_info_all_nodes(struct ieee80211com* ic)
{
    struct ieee80211_node_table *nt = &ic->ic_sta;
    struct ieee80211_node *ni = NULL, *next = NULL;
    rwlock_state_t lock_state;
    OS_RWLOCK_READ_LOCK(&nt->nt_nodelock, &lock_state);

    TAILQ_FOREACH_SAFE(ni, &nt->nt_node, ni_list, next) {
        if ((ni) && (ni->ni_associd) &&
             (ni != ni->ni_vap->iv_bss)) {
                ieee80211_ref_node(ni);
                ic->ic_get_aow_tx_rate_info(ni);
                ieee80211_free_node(ni);
        }             
    }
    OS_RWLOCK_READ_UNLOCK(&nt->nt_nodelock, &lock_state);
    return 0;
}    

int32_t ieee80211_aow_request_version(struct ieee80211com* ic) 
{
    int version = ic->ic_aow.version;
    u_int8_t event_subtype = CM_REQUEST_VERSION_PASS;
    IEEE80211_AOW_DPRINTF("Release Type : %d\n", (version & 0xf00) >> AOW_RELEASE_VERSION_OFFSET);
    IEEE80211_AOW_DPRINTF("Major Number : %d\n", (version & 0xf0) >> AOW_VERSION_MAJOR_NUM_OFFSET);
    IEEE80211_AOW_DPRINTF("Minor Number : %d\n", (version & 0xf));
    ieee80211_aow_send_to_host(ic, (u_int8_t*)&version, sizeof(version), AOW_HOST_PKT_EVENT, event_subtype, NULL);
    return 0;
}

int ieee80211_aow_update_volume(struct ieee80211com* ic, u_int8_t* data, u_int32_t len)
{
    int i = 0;
    u_int8_t event_subtype = CM_UPDATE_VOLUME_PASS;
    ch_volume_data_t* p = (ch_volume_data_t*)data;

    AOW_LOCK(ic);

    for (i = 0; i < AOW_MAX_AUDIO_CHANNELS; i++) {
        OS_MEMCPY(&ic->ic_aow.volume_info.ch[i].info[0], p->ch[i].info, 8);
    }        

    /* toggle flag */
    ic->ic_aow.volume_info.volume_flag = ~ic->ic_aow.volume_info.volume_flag;

    AOW_UNLOCK(ic);
    ieee80211_aow_send_to_host(ic, &event_subtype, sizeof(event_subtype), AOW_HOST_PKT_EVENT, event_subtype, NULL);
    return 0;
}

int ieee80211_aow_join_indicate(struct ieee80211com* ic, STA_JOIN_STATE_T state, struct ieee80211_node *ni)
{
    int ret = 0;
    aow_eventdata_t d;
    struct CM_CONNECTED_IND* pc;
    struct CM_DISCONNECTED_IND*  pdc;

    pc = (struct CM_CONNECTED_IND*)&d.u;
    pdc = (struct CM_DISCONNECTED_IND*)&d.u;

    if (state == AOW_STA_CONNECTED) {
        d.event = CM_STA_CONNECTED;
        
        KASSERT((ni != NULL), ("Node is Null"));

        pc->device_index = ni->ni_associd;
        OS_MEMCPY(pc->addr, ni->ni_macaddr, IEEE80211_ADDR_LEN);
        OS_MEMCPY(ic->ic_aow.aow_bssid, ni->ni_macaddr, IEEE80211_ADDR_LEN);

        ret = ieee80211_aow_send_to_host(ic, (u_int8_t*)&d, sizeof(aow_eventdata_t), AOW_HOST_PKT_EVENT, CM_STA_CONNECTED, NULL);
    } else if (state == AOW_STA_DISCONNECTED) {
        d.event = CM_STA_DISCONNECTED;
        if (ni) {
            pdc->device_index = ni->ni_associd;
            OS_MEMCPY(pdc->addr, ni->ni_macaddr, IEEE80211_ADDR_LEN);
        } else {
            OS_MEMCPY(pdc->addr, ic->ic_aow.aow_bssid, IEEE80211_ADDR_LEN);
            OS_MEMSET(ic->ic_aow.aow_bssid, 0, IEEE80211_ADDR_LEN);
        }            
        ret = ieee80211_aow_send_to_host(ic, (u_int8_t*)&d, sizeof(aow_eventdata_t), AOW_HOST_PKT_EVENT, CM_STA_DISCONNECTED, NULL);
    }
    return ret;
}

/*
 * Setup operational rates.
 * For performance optimization, we want to avoid using
 * CCK rates.
 * We emulate P2P rate settings in this respect.
 */
void ieee80211_aow_setup_rates(wlan_if_t vaphandle) 
{
    int i;
    enum ieee80211_phymode mode;
    for (i = 0; i < IEEE80211_MODE_MAX; i++) {
        mode = (enum ieee80211_phymode)i;
        switch(mode) {
        case IEEE80211_MODE_11A:
        case IEEE80211_MODE_11G:
        case IEEE80211_MODE_11NA_HT20:
        case IEEE80211_MODE_11NG_HT20:
        case IEEE80211_MODE_11NA_HT40PLUS:
        case IEEE80211_MODE_11NA_HT40MINUS:
        case IEEE80211_MODE_11NG_HT40PLUS:
        case IEEE80211_MODE_11NG_HT40MINUS:
            wlan_set_operational_rates(vaphandle, mode, aow_rates, sizeof(aow_rates));
            break;
        default:
            break;
        }
    }
}


/********************* CODE SNIPPET USED FOR DEBUGGING *********************************

{
     static u_int32_t i = 0;
     static u_int64_t prev_tsf = 0;
     u_int64_t cur_tsf = ic->ic_get_aow_tsf_64(ic);
     u_int32_t k = i++ &0xff; 
     if ((k==0))
         printk("P : %d\n", (u_int32_t)(cur_tsf - prev_tsf));

     prev_tsf = cur_tsf;
}

********************* CODE SNIPPET USED FOR DEBUGGING *********************************/
#endif  /* ATH_SUPPORT_AOW */
