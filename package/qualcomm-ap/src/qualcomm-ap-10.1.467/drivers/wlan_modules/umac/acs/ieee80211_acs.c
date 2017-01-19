/*
 *  Copyright (c) 2008 Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */

#include <osdep.h>

#include <ieee80211_var.h>
#include <ieee80211_scan.h>
#include <ieee80211_channel.h>

#if UMAC_SUPPORT_ACS

#define IEEE80211_MAX_ACS_EVENT_HANDLERS 10
#define DEBUG_EACS 0

#define NF_WEIGHT_FACTOR (2)
#define CHANLOAD_WEIGHT_FACTOR (4)
#define CHANLOAD_INCREASE_AVERAGE_RSSI (40)
#define NOISE_FLOOR_THRESH -85 /* noise floor threshold to detect presence of video bridge */

/* Parameters to derive secondary channels */
#define UPPER_FREQ_SLOT 1
#define LOWER_FREQ_SLOT -1
#define SEC_40_LOWER -6
#define SEC_40_UPPER -2
#define SEC20_OFF_2 2
#define SEC20_OFF_6 6
/* Use a RSSI threshold of 10dB(?) above the noise floor*/
#define SPECTRAL_EACS_RSSI_THRESH  30

/* Added to avoid Static overrun Coverity issues */
#define IEEE80211_ACS_CHAN_MAX IEEE80211_CHAN_MAX+1
struct ieee80211_acs {
    /* Driver-wide data structures */
    wlan_dev_t                          acs_ic;
    wlan_if_t                           acs_vap;
    osdev_t                             acs_osdev;

    spinlock_t                          acs_lock;                /* acs lock */

    /* List of clients to be notified about scan events */
    u_int16_t                           acs_num_handlers;
    ieee80211_acs_event_handler         acs_event_handlers[IEEE80211_MAX_ACS_EVENT_HANDLERS];
    void                                *acs_event_handler_arg[IEEE80211_MAX_ACS_EVENT_HANDLERS];

    IEEE80211_SCAN_REQUESTOR            acs_scan_requestor;    /* requestor id assigned by scan module */
    IEEE80211_SCAN_ID                   acs_scan_id;           /* scan id assigned by scan scheduler */
    u_int8_t                            acs_in_progress:1,  /* flag for ACS in progress */
                                        acs_scan_2ghz_only:1; /* flag for scan 2.4 GHz channels only */
    struct ieee80211_channel            *acs_channel;

    u_int16_t                           acs_nchans;         /* # of all available chans */
    struct ieee80211_channel            *acs_chans[IEEE80211_ACS_CHAN_MAX];
    u_int8_t                            acs_chan_maps[IEEE80211_ACS_CHAN_MAX];       /* channel mapping array */
    int32_t                             acs_chan_maxrssi[IEEE80211_ACS_CHAN_MAX];    /* max rssi of these channels */
    int32_t                             acs_chan_minrssi[IEEE80211_ACS_CHAN_MAX];    /* Min rssi of the channel [debugging] */
    int16_t                             acs_noisefloor[IEEE80211_ACS_CHAN_MAX];      /* Noise floor value read current channel */
    int16_t                             acs_channel_loading[IEEE80211_ACS_CHAN_MAX];      /* Noise floor value read current channel */
    u_int32_t                           acs_chan_load[IEEE80211_ACS_CHAN_MAX];
    u_int32_t                           acs_cycle_count[IEEE80211_ACS_CHAN_MAX];
#if ATH_SUPPORT_VOW_DCS
    u_int32_t                           acs_intr_ts[IEEE80211_ACS_CHAN_MAX];
    u_int8_t                            acs_intr_status[IEEE80211_ACS_CHAN_MAX];
#endif
    int32_t                             acs_minrssi_11na;    /* min rssi in 5 GHz band selected channel */
    int32_t                             acs_avgrssi_11ng;    /* average rssi in 2.4 GHz band selected channel */
    bool                                acs_sec_chan[IEEE80211_ACS_CHAN_MAX];       /*secondary channel flag */
    u_int8_t                            acs_chan_nbss[IEEE80211_ACS_CHAN_MAX];      /* No. of BSS of the channel */
    u_int16_t                           acs_nchans_scan;         /* # of all available chans */
    u_int8_t                            acs_ieee_chan[IEEE80211_ACS_CHAN_MAX];       /* channel mapping array */
 
#if ATH_SUPPORT_SPECTRAL
    int8_t                              ctl_eacs_rssi_thresh;          /* eacs spectral control channel rssi threshold */
    int8_t                              ext_eacs_rssi_thresh;          /* eacs spectral extension channel rssi threshold */
    int8_t                              ctl_eacs_avg_rssi;             /* eacs spectral control channel avg rssi */
    int8_t                              ext_eacs_avg_rssi;             /* eacs spectral extension channel avg rssi */

    int32_t                             ctl_chan_loading;              /* eacs spectral control channel spectral load*/
    int32_t                             ctl_chan_frequency;            /* eacs spectral control channel frequency (in Mhz) */
    int32_t                             ctl_chan_noise_floor;          /* eacs spectral control channel noise floor*/
    int32_t                             ext_chan_loading;              /* eacs spectral extension channel spectral load*/      
    int32_t                             ext_chan_frequency;            /* eacs spectral extension channel frequency (in Mhz) */
    int32_t                             ext_chan_noise_floor;          /* eacs spectral externsion channel noise floor*/        
    int32_t                             ctl_eacs_spectral_reports;     /* no of spectral reports received for control channel*/
    int32_t                             ext_eacs_spectral_reports;     /* no of spectral reports received for extension channel*/
    int32_t                             ctl_eacs_interf_count;         /* no of times interferece detected on control channel */
    int32_t                             ext_eacs_interf_count;         /* no of times interferece detected on extension*/
    int32_t                             ctl_eacs_duty_cycle;           /* duty cycles on control channel*/
    int32_t                             ext_eacs_duty_cycle;           /* duty cycles on extension channel*/
    int32_t                             eacs_this_scan_spectral_data;  /* no. of spectral sample to calculate duty cycles */
#endif

};

struct ieee80211_acs_adj_chan_stats {
    u_int32_t                           adj_chan_load;
    u_int32_t                           adj_chan_rssi;
    u_int8_t                            if_valid_stats;    
    u_int8_t                            adj_chan_idx;
};
#define UNII_II_EXT_BAND(freq)  (freq >= 5500) && (freq <= 5700)
/* Forward declarations */
static void ieee80211_acs_free_scan_resource(ieee80211_acs_t acs);
static void ieee80211_free_ht40intol_scan_resource(ieee80211_acs_t acs);
#if ATH_SUPPORT_SPECTRAL
int get_eacs_control_duty_cycle(ieee80211_acs_t acs);
int get_eacs_extension_duty_cycle(ieee80211_acs_t acs);
#endif

static INLINE u_int8_t ieee80211_acs_in_progress(ieee80211_acs_t acs) 
{
    return (acs->acs_in_progress);
}

/*
 * Check for channel interference.
 */
static int
ieee80211_acs_channel_is_set(struct ieee80211vap *vap)
{
    struct ieee80211_channel    *chan = NULL;

    chan =  vap->iv_des_chan[vap->iv_des_mode];

    if ((chan == NULL) || (chan == IEEE80211_CHAN_ANYC)) {
        return (0);
    } else {
        return (1);
    }
}

/*
 * Check for channel interference.
 */
static int
ieee80211_acs_check_interference(struct ieee80211_channel *chan, struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;

    /*
   * (1) skip static turbo channel as it will require STA to be in
   * static turbo to work.
   * (2) skip channel which's marked with radar detection
   * (3) WAR: we allow user to config not to use any DFS channel
   * (4) skip excluded 11D channels. See bug 31246 
   */
    if ( IEEE80211_IS_CHAN_STURBO(chan) || 
         IEEE80211_IS_CHAN_RADAR(chan) ||
         (IEEE80211_IS_CHAN_DFSFLAG(chan) && ieee80211_ic_block_dfschan_is_set(ic)) ||
         IEEE80211_IS_CHAN_11D_EXCLUDED(chan) ) {
        return (1);
    } else {
        return (0);
    }
}

static void ieee80211_acs_get_adj_ch_stats(ieee80211_acs_t acs, struct ieee80211_channel *channel,
        struct ieee80211_acs_adj_chan_stats *adj_chan_stats )
{
#define ADJ_CHANS 8
    u_int8_t ieee_chan = ieee80211_chan2ieee(acs->acs_ic, channel);
    u_int8_t k; 
    u_int32_t max_adj_chan_load = 0;
    int sec_chan, first_adj_chan, last_adj_chan;
    int sec_chan_40_1 = 0;
    int sec_chan_40_2 = 0;
    int8_t pri_center_ch_diff;
    int8_t sec_level;

    int32_t mode_mask = (IEEE80211_CHAN_11NA_HT20 |
                         IEEE80211_CHAN_11NA_HT40PLUS |
                         IEEE80211_CHAN_11NA_HT40MINUS |
                         IEEE80211_CHAN_11AC_VHT20 |
                         IEEE80211_CHAN_11AC_VHT40PLUS |
                         IEEE80211_CHAN_11AC_VHT40MINUS |
                         IEEE80211_CHAN_11AC_VHT80);

    adj_chan_stats->if_valid_stats = 1;
    adj_chan_stats->adj_chan_load = 0;
    adj_chan_stats->adj_chan_rssi = 0;
    adj_chan_stats->adj_chan_idx = 0;

    switch (channel->ic_flags & mode_mask)
    {
        case IEEE80211_CHAN_11NA_HT40PLUS:
        case IEEE80211_CHAN_11AC_VHT40PLUS:
            sec_chan = ieee_chan+4;
            first_adj_chan = ieee_chan - ADJ_CHANS;
            last_adj_chan = sec_chan + ADJ_CHANS;
            break;
        case IEEE80211_CHAN_11NA_HT40MINUS:
        case IEEE80211_CHAN_11AC_VHT40MINUS:
            sec_chan = ieee_chan-4;
            first_adj_chan = sec_chan - ADJ_CHANS;
            last_adj_chan = ieee_chan + ADJ_CHANS;
            break;
        case IEEE80211_CHAN_11AC_VHT80:
            
            pri_center_ch_diff = ieee_chan - channel->ic_vhtop_ch_freq_seg1;
 
            /* Secondary 20 channel would be less(2 or 6) or more (2 or 6) 
             * than center frequency based on primary channel 
             */
            if(pri_center_ch_diff > 0) {
                sec_level = LOWER_FREQ_SLOT;
                sec_chan_40_1 = channel->ic_vhtop_ch_freq_seg1 + SEC_40_LOWER; 
                sec_chan_40_2 = channel->ic_vhtop_ch_freq_seg1 + SEC_40_UPPER; 
            }
            else {
                sec_level = UPPER_FREQ_SLOT;
                sec_chan_40_1 = channel->ic_vhtop_ch_freq_seg1 - SEC_40_UPPER; 
                sec_chan_40_2 = channel->ic_vhtop_ch_freq_seg1 - SEC_40_LOWER; 
            }

            if((sec_level*pri_center_ch_diff) < -2)
                sec_chan = channel->ic_vhtop_ch_freq_seg1 - (sec_level* SEC20_OFF_2);
            else 
                sec_chan = channel->ic_vhtop_ch_freq_seg1 - (sec_level* SEC20_OFF_6);
              /* Adjacnet channels are 4 channels before the band and 4 channels are 
                 after the band */
            first_adj_chan = (channel->ic_vhtop_ch_freq_seg1 - 6) - 2*ADJ_CHANS;
            last_adj_chan =  (channel->ic_vhtop_ch_freq_seg1 + 6) + 2*ADJ_CHANS;
            break;
        case IEEE80211_CHAN_11NA_HT20:
        case IEEE80211_CHAN_11AC_VHT20:
        default: /* neither HT40+ nor HT40-, finish this call */
            sec_chan = ieee_chan;
            first_adj_chan = sec_chan - ADJ_CHANS;
            last_adj_chan = ieee_chan + ADJ_CHANS;
            break;
    }

	/* Added as a fix for Coverity ID 12130, static overrun */
    if(sec_chan > (u_int8_t)IEEE80211_CHAN_ANY) {
        sec_chan = (u_int8_t)IEEE80211_CHAN_ANY;
    }
    if(sec_chan_40_1 > (u_int8_t)IEEE80211_CHAN_ANY) {
        sec_chan_40_1 = (u_int8_t)IEEE80211_CHAN_ANY;
    }
    if(sec_chan_40_2 > (u_int8_t)IEEE80211_CHAN_ANY) {
        sec_chan_40_2 = (u_int8_t)IEEE80211_CHAN_ANY;
    }
    if ( (sec_chan != ieee_chan) ){
        /* check sec chan noise floor. If it is above threshold, then this means
         * that video bridge or other interfernce source is present. Ignore this ch.
         */
        if (acs->acs_noisefloor[sec_chan] >= NOISE_FLOOR_THRESH ){              
#if DEBUG_EACS
            printk( "%s chan: %d maxrssi: %d, nf: %d\n", __func__,
		ieee80211_chan2ieee(acs->acs_ic, channel), acs->acs_chan_maxrssi[sec_chan], acs->acs_noisefloor[sec_chan] );
#endif
            adj_chan_stats->adj_chan_load = 100; /* increase Ch load and RSSI to reject this channel */
            adj_chan_stats->adj_chan_rssi = MAX( 50, acs->acs_chan_maxrssi[sec_chan]);
            adj_chan_stats->adj_chan_idx = sec_chan;
            return;
	}
         /* in VHT80, It has secondary 40 channel. It considers those channels */
        if ((sec_chan_40_1) &&(acs->acs_noisefloor[sec_chan_40_1] >= NOISE_FLOOR_THRESH )){              
#if DEBUG_EACS
            printk( "%s chan: %d maxrssi: %d, nf: %d\n", __func__,
                ieee80211_chan2ieee(acs->acs_ic, channel), acs->acs_chan_maxrssi[sec_chan_40_1], acs->acs_noisefloor[sec_chan_40_1] );
#endif
            adj_chan_stats->adj_chan_load = 100; /* increase Ch load and RSSI to reject this channel */
            adj_chan_stats->adj_chan_rssi = MAX( 50, acs->acs_chan_maxrssi[sec_chan_40_1]);
            adj_chan_stats->adj_chan_idx = sec_chan_40_1;
            return;
	}       
 
         /* in VHT80, It has secondary 40 channel. It considers those channels */
       if ((sec_chan_40_2) &&(acs->acs_noisefloor[sec_chan_40_2] >= NOISE_FLOOR_THRESH )){              
#if DEBUG_EACS
            printk( "%s chan: %d maxrssi: %d, nf: %d\n", __func__,
                  ieee80211_chan2ieee(acs->acs_ic, channel), acs->acs_chan_maxrssi[sec_chan_40_2], acs->acs_noisefloor[sec_chan_40_2] );
#endif
            adj_chan_stats->adj_chan_load = 100; /* increase Ch load and RSSI to reject this channel */
            adj_chan_stats->adj_chan_rssi = MAX( 50, acs->acs_chan_maxrssi[sec_chan_40_2]);
            adj_chan_stats->adj_chan_idx = sec_chan_40_2;
            return;
	}       

        if (( acs->acs_chan_maxrssi[sec_chan] != 0 ) ||
             ((sec_chan_40_1) && ( acs->acs_chan_maxrssi[sec_chan_40_1] != 0 )) ||
             ((sec_chan_40_2) && ( acs->acs_chan_maxrssi[sec_chan_40_2] != 0 ))) {
#if DEBUG_EACS
		printk ("%s: Pri ch: %d Sec ch: %d Sec ch load %d rssi: %d\n ", __func__, ieee_chan, sec_chan,
			acs->acs_chan_load[sec_chan], acs->acs_chan_maxrssi[sec_chan]);
#endif

            if ( acs->acs_chan_maxrssi[ieee_chan] != 0 ){
                adj_chan_stats->adj_chan_load = acs->acs_chan_load[sec_chan];
                adj_chan_stats->adj_chan_rssi = acs->acs_chan_maxrssi[sec_chan];
                adj_chan_stats->adj_chan_idx = sec_chan;
                max_adj_chan_load = acs->acs_chan_load[sec_chan]; 
                /* If channel is VHT80, It needs to check the channel load and
                    RSSI with secondary 40 channel also */
                if(sec_chan_40_1 && sec_chan_40_2){

                    if(adj_chan_stats->adj_chan_load < MAX(acs->acs_chan_load[sec_chan_40_1],acs->acs_chan_load[sec_chan_40_2])) {
                        adj_chan_stats->adj_chan_load = MAX(acs->acs_chan_load[sec_chan_40_1],acs->acs_chan_load[sec_chan_40_2]);
                        max_adj_chan_load = adj_chan_stats->adj_chan_load; 
                    }

                    if(adj_chan_stats->adj_chan_rssi < MAX(acs->acs_chan_maxrssi[sec_chan_40_1],acs->acs_chan_maxrssi[sec_chan_40_2])) {
                        adj_chan_stats->adj_chan_rssi = MAX(acs->acs_chan_maxrssi[sec_chan_40_1],acs->acs_chan_maxrssi[sec_chan_40_2]);
                        if(acs->acs_chan_maxrssi[sec_chan_40_1] > acs->acs_chan_maxrssi[sec_chan_40_2]) {
                           adj_chan_stats->adj_chan_idx = sec_chan_40_1;
                        }
                        else {
                           adj_chan_stats->adj_chan_idx = sec_chan_40_2;
                        }
                    }
                }
            }
            else{
                /* As per the standard, if AP detects a beacon in the sec ch, 
                 * then it should switch to 20 MHz mode. So, reject this channel */
                adj_chan_stats->adj_chan_load = 100;
                adj_chan_stats->adj_chan_rssi = acs->acs_chan_maxrssi[sec_chan];
                adj_chan_stats->adj_chan_idx = sec_chan;
                /* If channel is VHT80, It needs to check the channel load and
                    RSSI with secondary 40 channel also */
                if(sec_chan_40_1 && sec_chan_40_2){

                    if(adj_chan_stats->adj_chan_rssi < MAX(acs->acs_chan_maxrssi[sec_chan_40_1],acs->acs_chan_maxrssi[sec_chan_40_2])) {
                        adj_chan_stats->adj_chan_rssi = MAX(acs->acs_chan_maxrssi[sec_chan_40_1],acs->acs_chan_maxrssi[sec_chan_40_2]);
                        if(acs->acs_chan_maxrssi[sec_chan_40_1] > acs->acs_chan_maxrssi[sec_chan_40_2]) {
                           adj_chan_stats->adj_chan_idx = sec_chan_40_1;
                        }
                        else {
                           adj_chan_stats->adj_chan_idx = sec_chan_40_2;
                        }
                    }
                }

                return; 
            }
        }
    }

    /* adjacent channel = [primary ch -ADJ_CHANS, secondary ch + ADJ_CHANS] */
    for (k = first_adj_chan ; (k >= first_adj_chan && k <= last_adj_chan); k += 2) {

        if ( (k == ieee_chan) || (k == sec_chan) ) continue; 

        /* sum of adjacent channel load */
        if(acs->acs_chan_maxrssi[k]){
            adj_chan_stats->adj_chan_load += acs->acs_chan_load[k];
#if DEBUG_EACS
	    printk ("%s: Adj ch: %d ch load: %d rssi: %d\n",
                __func__, k, acs->acs_chan_load[k], acs->acs_chan_maxrssi[k] );
#endif
        }
        /* max adjacent channel rssi */
        if(acs->acs_chan_maxrssi[k] > adj_chan_stats->adj_chan_rssi){
            adj_chan_stats->adj_chan_rssi = acs->acs_chan_maxrssi[k];
            adj_chan_stats->adj_chan_idx = k;
        }
        /* max adj channel load */
        if (acs->acs_chan_load[k] > max_adj_chan_load ) {
            max_adj_chan_load = acs->acs_chan_load[k];
        }
        
    }
    adj_chan_stats->adj_chan_load = MAX(adj_chan_stats->adj_chan_load, max_adj_chan_load ); 
#if DEBUG_EACS
    printk ("%s: Adj ch stats valid: %d ind: %d rssi: %d load: %d\n",__func__, 
            adj_chan_stats->if_valid_stats, adj_chan_stats->adj_chan_idx, 
            adj_chan_stats->adj_chan_rssi, adj_chan_stats->adj_chan_load );
#endif

#undef ADJ_CHANS
}


/*
 * In 5 GHz, if the channel is unoccupied the max rssi
 * should be zero; just take it.Otherwise
 * track the channel with the lowest rssi and
 * use that when all channels appear occupied.
 */
static INLINE struct ieee80211_channel *
ieee80211_acs_find_best_11na_centerchan(ieee80211_acs_t acs)
{
    struct ieee80211_channel *channel;
    int best_center_chanix = -1, i;
    u_int8_t cur_chan;
#define MIN_ADJ_CH_RSSI_THRESH 8
#define CH_LOAD_INACCURACY 0

    u_int32_t min_max_chan_load = 0xffffffff;
    u_int32_t min_sum_chan_load = 0xffffffff, sum;
    u_int32_t min_cur_chan_load = 0xffffffff;
    int min_sum_chan_load_idx = -1;
    int8_t last_maxregpower = 0, adjusted_maxregpower;

    int cur_ch_load_thresh = 0;
    u_int16_t nchans = 0;
    u_int8_t chan_ind[IEEE80211_CHAN_MAX];
    u_int8_t random;
    int j;
    struct ieee80211_acs_adj_chan_stats *adj_chan_stats;
    struct ieee80211com *ic = acs->acs_ic;
#if ATH_SUPPORT_VOW_DCS 
    u_int32_t nowms =  (u_int32_t) CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP());
    u_int8_t ieee_chan, intr_chan_cnt = 0;

#define DCS_AGING_TIME 300000 /* 5 mins */
#define DCS_PENALTY    30     /* channel utilization in % */
#define DCS_DISABLE_THRESH 3  /* disable dcs, after these many channel change triggers */
    for (i = 0; i < acs->acs_nchans; i++) {
        channel = acs->acs_chans[i];
        if (ieee80211_acs_check_interference(channel, acs->acs_vap)) {
            continue;
        }
        ieee_chan = ieee80211_chan2ieee(acs->acs_ic, channel);
        if ( acs->acs_intr_status[ieee_chan] ){ 
            if ((nowms >= acs->acs_intr_ts[ieee_chan]) && 
                    ((nowms - acs->acs_intr_ts[ieee_chan]) <= DCS_AGING_TIME)){
                acs->acs_chan_load[ieee_chan] += DCS_PENALTY;
                intr_chan_cnt += 1;
            }
            else{
                acs->acs_intr_status[ieee_chan] = 0;
            }
        }
    }
    if ( intr_chan_cnt >= DCS_DISABLE_THRESH ){
        /* To avoid frequent channel change,if channel change is 
         * triggered three times in last 5 mins, disable dcs. 
         */ 
        ic->ic_disable_dcsim(ic);
    }
#undef DCS_AGING_TIME
#undef DCS_PENALTY
#undef DCS_DISABLE_THRESH
#endif

    adj_chan_stats = (struct ieee80211_acs_adj_chan_stats *) OS_MALLOC(acs->acs_osdev, 
            IEEE80211_CHAN_MAX * sizeof(struct ieee80211_acs_adj_chan_stats), 0);

    if (adj_chan_stats) {
        OS_MEMZERO(adj_chan_stats, sizeof(struct ieee80211_acs_adj_chan_stats)*IEEE80211_CHAN_MAX);
    }
    else {
        adf_os_print("%s: malloc failed \n",__func__);
        return NULL;//ENOMEM;
    }

    
    acs->acs_minrssi_11na = 0xffffffff; /* Some large value */


    /* Scan through the channel list to find the best channel */
    do {
        for (i = 0; i < acs->acs_nchans; i++) {
            channel = acs->acs_chans[i];
            cur_chan = ieee80211_chan2ieee(acs->acs_ic, channel);
            /* Check if it is 5GHz channel  */
            if (IEEE80211_IS_CHAN_5GHZ(channel)) {
                 /* Best Channel for VHT BSS shouldn't be the secondary channel of other BSS 
                  * Do not consider this channel for best channel selection
                  */
                if((acs->acs_vap->iv_des_mode == IEEE80211_MODE_AUTO) ||
                    (acs->acs_vap->iv_des_mode >= IEEE80211_MODE_11AC_VHT20)) {
                     if(acs->acs_sec_chan[cur_chan] == true) {
#if DEBUG_EACS
                        printk("Skipping the chan %d for best channel selection If desired mode is AUTO/VHT\n",cur_chan);
#endif
                        continue;
                     }
                }
                if(acs->acs_ic->ic_no_weather_radar_chan) {
                    if(IEEE80211_IS_CHAN_WEATHER_RADAR(channel)
                            && (acs->acs_ic->ic_get_dfsdomain(acs->acs_ic) == DFS_ETSI_DOMAIN)) {
                        continue;
                    }
                }
                /* Check of DFS and other modes where we do not want to use the 
                 * channel  
                */
                if (ieee80211_acs_check_interference(channel, acs->acs_vap)) {
                    continue;
                }
                
                /* Check if the noise floor value is very high. If so, it indicates
                 * presence of CW interference (Video Bridge etc). This channel 
                 * should not be used 
                 */
                if (acs->acs_noisefloor[cur_chan] >= NOISE_FLOOR_THRESH ) {
#if DEBUG_EACS 
                    printk( "%s chan: %d maxrssi: %d, nf: %d\n", __func__,
       		        cur_chan, acs->acs_chan_maxrssi[cur_chan], acs->acs_noisefloor[cur_chan] );
#endif
                    continue;
                }
    
                /*
                 * ETSI UNII-II Ext band has different limits for STA and AP.
                 * The software reg domain table contains the STA limits(23dBm).
                 * For AP we adjust the max power(30dBm) dynamically. 
                 */
                if (UNII_II_EXT_BAND(ieee80211_chan2freq(acs->acs_ic, channel)) 
                        && (acs->acs_ic->ic_get_dfsdomain(acs->acs_ic) == DFS_ETSI_DOMAIN)){
                    adjusted_maxregpower = MIN( 30, channel->ic_maxregpower+7 );
                }
                else {
                    adjusted_maxregpower = channel->ic_maxregpower;
                }
              
                /* check neighboring channel load 
                 * pending - first check the operating mode from beacon( 20MHz/40 MHz) and 
                 * based on that find the interfering channel 
                 */
                if( !adj_chan_stats[i].if_valid_stats){ 
                    ieee80211_acs_get_adj_ch_stats(acs, channel, &adj_chan_stats[i]);
                    /* find minimum of MAX( adj_chan_load, cur channel load)*/
                    if ( min_max_chan_load > MAX (adj_chan_stats[i].adj_chan_load , 
                                acs->acs_chan_load[cur_chan])){
                        min_max_chan_load = MAX (adj_chan_stats[i].adj_chan_load , 
                                acs->acs_chan_load[cur_chan]) ;
                    } 
#if DEBUG_EACS
                    printk( "%s chan: %d maxrssi: %d, cl: %d Adj cl: %d ind: %d cl thresh: %d %d\n", __func__,
                        cur_chan, acs->acs_chan_maxrssi[cur_chan], acs->acs_chan_load[cur_chan], 
                        adj_chan_stats[i].adj_chan_load, i, cur_ch_load_thresh, adjusted_maxregpower);
#endif      
               }
    
               if( acs->acs_chan_load[ieee80211_chan2ieee(acs->acs_ic, channel)] > cur_ch_load_thresh) {
                   continue;
               }
    
               /* look for max rssi in beacons found in this channel */
               if ( (acs->acs_chan_maxrssi[cur_chan] == 0) && 
                       (adj_chan_stats[i].adj_chan_rssi == 0)) {
                    
                    /* minimize current ch load */ 
                    if (acs->acs_chan_load[cur_chan ] < min_cur_chan_load) {
                        min_cur_chan_load = acs->acs_chan_load[cur_chan];
                        last_maxregpower = adjusted_maxregpower;
                        nchans = 0;
                        chan_ind[nchans++] = i;
#if DEBUG_EACS
                        printk(" nchans: %d, %d\n",nchans, i );
#endif
                    } else if (acs->acs_chan_load[cur_chan] == min_cur_chan_load) {
                        /* when chan load is same, maximize transmit power */
		                if (adjusted_maxregpower > last_maxregpower ) {
		                    last_maxregpower = adjusted_maxregpower;
			                nchans = 0;
			            }
			            if( adjusted_maxregpower >= last_maxregpower ){
		                      chan_ind[nchans++] = i;
#if DEBUG_EACS		
                              printk(" nchans: %d, %d\n",nchans, i );
#endif
			            }
		            }
                } else {
                    if ( adj_chan_stats[i].adj_chan_rssi < MIN_ADJ_CH_RSSI_THRESH ){
                        sum = acs->acs_chan_load[ieee80211_chan2ieee(acs->acs_ic, channel)];
                    } else {
                        sum = adj_chan_stats[i].adj_chan_load + 
                        acs->acs_chan_load[ieee80211_chan2ieee(acs->acs_ic, channel)];
                    }
                    /* minimize sum of ch load */
                    if (sum < min_sum_chan_load) {
                        min_sum_chan_load = sum;
                        min_sum_chan_load_idx = i;
                    }
#if DEBUG_EACS
                    printk(" sum: %d, %d\n",sum, i );
#endif
                }
            } else {
                /* Skip non-5GHZ channel */
                continue;
            }
        }
    
        if (nchans != 0){
            OS_GET_RANDOM_BYTES(&random, sizeof(random));
            j = random % nchans;
            best_center_chanix = chan_ind[j];
        } else if (min_sum_chan_load_idx != -1){
            /* minimize sum of adj chan load and curr chan load, 
             * when max( adj chan load, curr chan load) is less than 
             * min_max_chan_load + CH_LOAD_INACCURACY
             */
            best_center_chanix = min_sum_chan_load_idx;
#if ATH_SUPPORT_VOW_DCS
#define DCS_CHLOAD_THRESH 30
            if( min_sum_chan_load >= DCS_CHLOAD_THRESH ){
                /* If interference in the chosen channel is above the minimum
                 * interference level at which DCS triggers channel change,
                 * then disable dcs. 
                 */ 
                ic->ic_disable_dcsim(ic);
            }
#undef DCS_CHLOAD_THRESH
#endif
        } else if (cur_ch_load_thresh){
            break;
        }
        cur_ch_load_thresh += min_max_chan_load + CH_LOAD_INACCURACY;
    } while(best_center_chanix == -1 );
    OS_FREE(adj_chan_stats);
#undef MIN_ADJ_CH_RSSI_THRESH
#undef CH_LOAD_INACCURACY
    
    if (best_center_chanix != -1) {
        ic->ic_eacs_done = 1;
        channel = acs->acs_chans[best_center_chanix];
#if DEBUG_EACS
        printk( "%s found best 11na center chan: %d\n", 
            __func__, ieee80211_chan2ieee(acs->acs_ic, channel));
#endif
        printk("Function %s best 5G channel is %d\n", __func__,ieee80211_chan2ieee(acs->acs_ic, channel));
    } else {
        channel = ieee80211_find_dot11_channel(acs->acs_ic, 0, acs->acs_vap->iv_des_mode);
        /* no suitable channel, should not happen */
#if DEBUG_EACS      
        printk( "%s: no suitable channel! (should not happen)\n", __func__);
#endif
    }

    return channel;
}


static int32_t ieee80211_acs_find_average_rssi(ieee80211_acs_t acs, const int *chanlist, int chancount, u_int8_t centChan)
{
    u_int8_t chan;
    int i;
    int32_t average = 0;
    u_int32_t totalrssi = 0; /* total rssi for all channels so far */

    if (chancount == 0) {
        /* return a large enough number for not to choose this channel */
        return 0xffffffff;
    }

    for (i = 0; i < chancount; i++) {
        chan = chanlist[i]; 
        totalrssi += acs->acs_chan_maxrssi[chan];
#if DEBUG_EACS
        printk( "%s chan: %d maxrssi: %d\n", __func__, chan, acs->acs_chan_maxrssi[chan]);
#endif
    }
    
    average = totalrssi/chancount;
#if DEBUG_EACS   
    printk( "Channel %d average beacon RSSI %d noisefloor %d\n", 
       centChan, average, acs->acs_noisefloor[centChan]);
#endif
    printk("Channel %d average beacon RSSI %d noisefloor %d ", 
                  centChan, average, acs->acs_noisefloor[centChan]);
    /* add the weighted noise floor */
    average += acs->acs_noisefloor[centChan] * NF_WEIGHT_FACTOR;
#if ATH_SUPPORT_SPECTRAL
    /* If channel loading is greater, add RSSI factor to the average_rssi for that channel */
    average +=  ((acs->acs_channel_loading[centChan] * CHANLOAD_INCREASE_AVERAGE_RSSI) / 100);
    printk("SS Chan Load %d",
           ((acs->acs_channel_loading[centChan] * CHANLOAD_INCREASE_AVERAGE_RSSI) / 100));
#endif    
    return (average);
}

static INLINE struct ieee80211_channel *
ieee80211_acs_find_best_11ng_centerchan(ieee80211_acs_t acs)
{
    struct ieee80211_channel *channel;
    int best_center_chanix = -1, i;
    u_int8_t chan, band;
    int32_t avg_rssi;

    /*
   * The following center chan data structures are invented to allow calculating
   * the average rssi in 20Mhz band for a certain center chan. 
   *
   * We would then pick the band which has the minimum rsi of all the 20Mhz bands.
   */

    /* For 20Mhz band with center chan 1, we would see beacons only on channels 1,2 & 3 */
    static const u_int center1[] = { 1, 2, 3 };

    /* For 20Mhz band with center chan 6, we would see beacons on channels 4,5,6, 7 & 8. */
    static const u_int center6[] = { 4, 5, 6, 7, 8 };

    /* For 20Mhz band with center chan 11, we would see beacons on channels 9,10 & 11. */
    static const u_int center11[] = { 9, 10, 11 };

    struct centerchan {
        int count;               /* number of chans to average the rssi */
        const u_int *chanlist;   /* the possible beacon channels around center chan */
    };

#define X(a)    sizeof(a)/sizeof(a[0]), a

    struct centerchan centerchans[] = {
        { X(center1) },
        { X(center6) },
        { X(center11) }
    };

    acs->acs_avgrssi_11ng = 1000;
   
    for (i = 0; i < acs->acs_nchans; i++) {
        channel = acs->acs_chans[i];
        chan = ieee80211_chan2ieee(acs->acs_ic, channel);

        if ((chan != 1) && (chan != 6) && (chan != 11)) {
            /* Don't bother with other center channels except for 1, 6 & 11 */
            continue;
        }

        switch (chan) {
            case 1:
                band = 0;
                break;
            case 6:
                band = 1;
                break;
            case 11:
                band = 2;
                break;
            default: 
                band = 0;
                break;
        }

        /* find the average rssi for this 20Mhz band */
        avg_rssi = ieee80211_acs_find_average_rssi(acs, centerchans[band].chanlist, centerchans[band].count, chan);
#if DEBUG_EACS
        printk( "%s chan: %d beacon RSSI + weighted noisefloor: %d\n", __func__, chan, avg_rssi);
#endif
        printk(" %s chan: %d beacon RSSI + weighted noisefloor: %d\n", __func__, chan, avg_rssi);

        if (avg_rssi < acs->acs_avgrssi_11ng) {
            acs->acs_avgrssi_11ng = avg_rssi;
            best_center_chanix = i;
        }
    }

    if (best_center_chanix != -1) {
        channel = acs->acs_chans[best_center_chanix];
#if DEBUG_EACS
        printk( "%s found best 11ng center chan: %d rssi: %d\n", 
                __func__, ieee80211_chan2ieee(acs->acs_ic, channel), acs->acs_avgrssi_11ng);
#endif
        printk("%s found best 11ng center chan: %d rssi: %d\n", 
               __func__, ieee80211_chan2ieee(acs->acs_ic, channel), acs->acs_avgrssi_11ng);
    } else {
        channel = ieee80211_find_dot11_channel(acs->acs_ic, 0, acs->acs_vap->iv_des_mode);
        /* no suitable channel, should not happen */
#if DEBUG_EACS    
        printk( "%s: no suitable channel! (should not happen)\n", __func__);
#endif
    }

    return channel;
}

static INLINE struct ieee80211_channel *
ieee80211_acs_find_best_auto_centerchan(ieee80211_acs_t acs)
{
    struct ieee80211_channel *channel_11na, *channel_11ng;

    u_int8_t ieee_chan_11na, ieee_chan_11ng;
    channel_11na = ieee80211_acs_find_best_11na_centerchan(acs);
    channel_11ng = ieee80211_acs_find_best_11ng_centerchan(acs);

    ieee_chan_11ng = ieee80211_chan2ieee(acs->acs_ic, channel_11ng);
    ieee_chan_11na = ieee80211_chan2ieee(acs->acs_ic, channel_11na);

#if DEBUG_EACS
    printk( "%s 11na chan: %d chan_load: %d, 11ng chan: %d chan_load: %d\n", 
            __func__, ieee_chan_11na, acs->acs_chan_load[ieee_chan_11na],
            ieee_chan_11ng, acs->acs_chan_load[ieee_chan_11ng]);
#endif
    /* Check which of them have the minimum channel load. If both have the same,
     * choose the 5GHz channel 
     */
    if (acs->acs_chan_load[ieee_chan_11ng] < acs->acs_chan_load[ieee_chan_11na]) {
        return channel_11ng;
    } else {
        return channel_11na;
    }
}

static INLINE struct ieee80211_channel *
ieee80211_acs_find_best_centerchan(ieee80211_acs_t acs)
{
    struct ieee80211_channel *channel;

    switch (acs->acs_vap->iv_des_mode)
    {
    case IEEE80211_MODE_11A:
    case IEEE80211_MODE_TURBO_A:
    case IEEE80211_MODE_11NA_HT20:
    case IEEE80211_MODE_11NA_HT40PLUS:
    case IEEE80211_MODE_11NA_HT40MINUS:
    case IEEE80211_MODE_11NA_HT40:
    case IEEE80211_MODE_11AC_VHT20: 
    case IEEE80211_MODE_11AC_VHT40PLUS: 
    case IEEE80211_MODE_11AC_VHT40MINUS:
    case IEEE80211_MODE_11AC_VHT40:
    case IEEE80211_MODE_11AC_VHT80:     
        channel = ieee80211_acs_find_best_11na_centerchan(acs);
        break;

    case IEEE80211_MODE_11B:
    case IEEE80211_MODE_11G:
    case IEEE80211_MODE_TURBO_G:
    case IEEE80211_MODE_11NG_HT20:
    case IEEE80211_MODE_11NG_HT40PLUS:
    case IEEE80211_MODE_11NG_HT40MINUS:
    case IEEE80211_MODE_11NG_HT40:
        channel = ieee80211_acs_find_best_11ng_centerchan(acs);
        break;

    default:
        if (acs->acs_scan_2ghz_only) {
            channel = ieee80211_acs_find_best_11ng_centerchan(acs);
        } else {
            channel = ieee80211_acs_find_best_auto_centerchan(acs);
        }
        break;
    }
#if ATH_SUPPORT_SPECTRAL
    acs->acs_ic->ic_stop_spectral_scan(acs->acs_ic);
#endif

    return channel;
}

static struct ieee80211_channel *
ieee80211_acs_pickup_random_channel(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_channel *channel;
    u_int16_t nchans = 0;
    struct ieee80211_channel **chans;
    u_int8_t random;
    int i;
    int index =0;

    chans = (struct ieee80211_channel **)
	    OS_MALLOC(ic->ic_osdev,
		      IEEE80211_CHAN_MAX * sizeof(struct ieee80211_channel *),
		      0);
    if (chans == NULL)
	    return NULL;

    ieee80211_enumerate_channels(channel, ic, i) {
        /* skip if found channel interference */
        if (ieee80211_acs_check_interference(channel, vap)) {
#if DEBUG_EACS      
      printk( "%s: Skip channel %d due to channel interference, flags=0x%x, flagext=0x%d\n",
                __func__, ieee80211_chan2ieee(ic, channel), ieee80211_chan_flags(channel), ieee80211_chan_flagext(channel));
#endif
            continue;
        }

        if (vap->iv_des_mode == ieee80211_chan2mode(channel)) {
#if DEBUG_EACS      
      printk( "%s: Put channel %d into channel list for pick up later\n",
                __func__, ieee80211_chan2ieee(ic, channel));
#endif
            chans[nchans++] = channel;
        }
    }

    if (nchans == 0) {
#if DEBUG_EACS
        printk( "%s: no any available channel for pick up\n",
            __func__);
#endif
	OS_FREE(chans);
        return NULL;
    }

    /* Pick up a random channel to start */
    OS_GET_RANDOM_BYTES(&random, sizeof(random));
    i = random % nchans;
    channel = chans[i];
pickchannel:
    OS_GET_RANDOM_BYTES(&random, sizeof(random));
    i = random % nchans;
    channel = chans[i];
    /* CAC for weather radar channel is 10 minutes so we are avoiding these channels */
    if (IEEE80211_IS_CHAN_5GHZ(channel)) {
        if((ic->ic_no_weather_radar_chan) 
                && (IEEE80211_IS_CHAN_WEATHER_RADAR(channel))
                && (DFS_ETSI_DOMAIN == ic->ic_get_dfsdomain(ic))) {
#define MAX_RETRY 5
#define BYPASS_WRADAR_CHANNEL_CNT 5 
                if(index >= MAX_RETRY) {
#if DEBUG_EACS
                        printk( "%s: no any available channel for pick up\n",
                                __func__);
#endif
                        channel = chans[(i + BYPASS_WRADAR_CHANNEL_CNT) % nchans];
                        /* there can be three channels in 
                           this range so adding 3 */
                        OS_FREE(chans);
                        return channel;
                }
#undef MAX_RETRY 
#undef BYPASS_WRADAR_CHANNEL_CNT 
                index++;
                goto pickchannel;
            }
        }
    
    OS_FREE(chans);
#if DEBUG_EACS
    printk( "%s: Pick up a random channel=%d to start\n",
        __func__, ieee80211_chan2ieee(ic, channel));
#endif
    return channel;
}

static void
ieee80211_find_ht40intol_overlap(ieee80211_acs_t acs,
                                     struct ieee80211_channel *channel)
{
#define CEILING 12
#define FLOOR    1
#define HT40_NUM_CHANS 5
    struct ieee80211_channel *iter_channel;
    u_int8_t ieee_chan = ieee80211_chan2ieee(acs->acs_ic, channel);
    int i, j, k; 


    int mean_chan; 

    int32_t mode_mask = (IEEE80211_CHAN_11NG_HT40PLUS |
                         IEEE80211_CHAN_11NG_HT40MINUS);


    if (!channel)
        return;

    switch (channel->ic_flags & mode_mask)
    {
        case IEEE80211_CHAN_11NG_HT40PLUS:
            mean_chan = ieee_chan+2;
            break;
        case IEEE80211_CHAN_11NG_HT40MINUS:
            mean_chan = ieee_chan-2;
            break;
        default: /* neither HT40+ nor HT40-, finish this call */
            return;
    }


    /* We should mark the intended channel as IEEE80211_CHAN_HTINTOL 
       if the intended frequency overlaps the iterated channel partially */

    /* According to 802.11n 2009, affected channel = [mean_chan-5, mean_chan+5] */
    for (j=MAX(mean_chan-HT40_NUM_CHANS, FLOOR); j<=MIN(CEILING,mean_chan+HT40_NUM_CHANS); j++) {
          for (i = 0; i < acs->acs_nchans; i++) {
              iter_channel = acs->acs_chans[i];
              k = ieee80211_chan2ieee(acs->acs_ic, iter_channel);

              /* exactly overlapping is allowed. Continue */
              if (k == ieee_chan) continue; 

              if (k  == j) {
                  if (iter_channel->ic_flags & IEEE80211_CHAN_HT40INTOL) {
#if DEBUG_EACS
                      printk( "%s Found on channel %d\n", __func__, j);
#endif
                      channel->ic_flags |= IEEE80211_CHAN_HT40INTOL;
                   }
              }
          }
    }



#undef CEILING
#undef FLOOR
#undef HT40_NUM_CHANS
}

/* 
 * Find all channels with active AP's.
 */
static int
ieee80211_get_ht40intol(void *arg, wlan_scan_entry_t se)
{
    struct ieee80211_channel *scan_chan = ieee80211_scan_entry_channel(se),
                             *iter_chan;
    ieee80211_acs_t acs = (ieee80211_acs_t) arg;
    int i;

    for (i = 0; i < acs->acs_nchans; i++) {
        iter_chan = acs->acs_chans[i];
        if ((ieee80211_chan2ieee(acs->acs_ic, scan_chan)) ==
            ieee80211_chan2ieee(acs->acs_ic, iter_chan)) {
            if (!(iter_chan->ic_flags & IEEE80211_CHAN_HT40INTOL)) {
#if DEBUG_EACS
                printk( "%s Marking channel %d\n", __func__,
                                  ieee80211_chan2ieee(acs->acs_ic, iter_chan));
#endif
            }
            iter_chan->ic_flags |= IEEE80211_CHAN_HT40INTOL;
        }
    }
    return EOK;

}
/* 
 * Derive secondary channels based on the phy mode of the scan entry
 */
static int ieee80211_acs_derive_sec_chan(wlan_scan_entry_t se, 
                       u_int8_t channel_ieee_se, u_int8_t *sec_chan_20,
                                                  u_int8_t *sec_chan_40)
{
    u_int8_t vht_center_freq;
    int8_t pri_center_ch_diff;
    int8_t sec_level;
    u_int8_t phymode_se;
    struct ieee80211_ie_vhtop *vhtop = NULL;


    phymode_se = wlan_scan_entry_phymode(se);
    switch (phymode_se)
    {
        /*secondary channel for VHT40MINUS and NAHT40MINUS is same */
        case IEEE80211_MODE_11NA_HT40MINUS:
        case IEEE80211_MODE_11AC_VHT40MINUS:
            *sec_chan_20 = channel_ieee_se - 4;
            break;
        /*secondary channel for VHT40PLUS and NAHT40PLUS is same */
        case IEEE80211_MODE_11NA_HT40PLUS:
        case IEEE80211_MODE_11AC_VHT40PLUS:
            *sec_chan_20 = channel_ieee_se + 4;
            break;
        case IEEE80211_MODE_11AC_VHT80:

            vhtop = (struct ieee80211_ie_vhtop *)ieee80211_scan_entry_vhtop(se);
            vht_center_freq = vhtop->vht_op_ch_freq_seg1;
            
            /* For VHT mode, The center frequency is given in VHT OP IE
             * For example: 42 is center freq and 36 is primary channel
             * then secondary 20 channel would be 40 and secondary 40 channel
             * would be 44 
             */
            pri_center_ch_diff = channel_ieee_se - vht_center_freq;
 
            /* Secondary 20 channel would be less(2 or 6) or more (2 or 6) 
             * than center frequency based on primary channel 
             */
            if(pri_center_ch_diff > 0) {
                sec_level = LOWER_FREQ_SLOT;
            }
            else {
                sec_level = UPPER_FREQ_SLOT;
            }
            if((sec_level*pri_center_ch_diff) < -2)
                *sec_chan_20 = vht_center_freq - (sec_level* SEC20_OFF_2);
            else 
                *sec_chan_20 = vht_center_freq - (sec_level* SEC20_OFF_6);

            break;
    }
    return EOK;
}

/* 
 * Trace all entries to record the max rssi found for every channel.
 */
static int
ieee80211_acs_get_channel_maxrssi_n_secondary_ch(void *arg, wlan_scan_entry_t se)
{
    ieee80211_acs_t acs = (ieee80211_acs_t) arg;
    struct ieee80211_channel *channel_se = ieee80211_scan_entry_channel(se);
    u_int8_t rssi_se = ieee80211_scan_entry_rssi(se);
    u_int8_t channel_ieee_se = ieee80211_chan2ieee(acs->acs_ic, channel_se);
    u_int8_t sec_chan_20 = 0;
    u_int8_t sec_chan_40 = 0;
#if DEBUG_EACS
    printk( "%s chan %d rssi %d noise: %d \n", 
                      __func__, channel_ieee_se, rssi_se, acs->acs_noisefloor[channel_ieee_se]);
#endif
    if (rssi_se > acs->acs_chan_maxrssi[channel_ieee_se]) {
        acs->acs_chan_maxrssi[channel_ieee_se] = rssi_se;
    }
    /* This support is for stats */
    if ((acs->acs_chan_minrssi[channel_ieee_se] == 0) || 
                  (rssi_se < acs->acs_chan_maxrssi[channel_ieee_se])) {
        acs->acs_chan_minrssi[channel_ieee_se] = rssi_se;
    }
    acs->acs_chan_nbss[channel_ieee_se] += 1;

    if(!ieee80211_acs_derive_sec_chan(se, channel_ieee_se, &sec_chan_20, &sec_chan_40)) {
        acs->acs_sec_chan[sec_chan_20] = true;
#if DEBUG_EACS
        printk("secondary 20 channel is %d and secondary 40 channel is %d\n",sec_chan_20,sec_chan_40);
#endif
        /* Secondary 40 would be enabled for 80+80 Mhz channel or 
         * 160 Mhz channel 
         */
        if(sec_chan_40 != 0) {
           acs->acs_sec_chan[sec_chan_40] = true;
           acs->acs_sec_chan[sec_chan_40 + 4] = true;
        }
    }

    return EOK;
}

static INLINE void ieee80211_acs_check_2ghz_only(ieee80211_acs_t acs, u_int16_t nchans_2ghz)
{
    /* No 5 GHz channels available, skip 5 GHz scan */
    if ((nchans_2ghz) && (acs->acs_nchans == nchans_2ghz)) {
        acs->acs_scan_2ghz_only = 1;
#if DEBUG_EACS
        printk( "%s: No 5 GHz channels available, skip 5 GHz scan\n", __func__);
#endif
    }
}

static INLINE void ieee80211_acs_get_phymode_channels(ieee80211_acs_t acs, enum ieee80211_phymode mode)
{
    struct ieee80211_channel *channel;
    int i;
#if DEBUG_EACS
    printk( "%s: get channels with phy mode=%d\n", __func__, mode);
#endif
    ieee80211_enumerate_channels(channel, acs->acs_ic, i) {
#if DEBUG_EACS
    printk("%s ic_freq: %d ic_ieee: %d ic_flags: %08x \n", __func__, channel->ic_freq, channel->ic_ieee, channel->ic_flags);
#endif

        if (mode == ieee80211_chan2mode(channel)) {
            u_int8_t ieee_chan_num = ieee80211_chan2ieee(acs->acs_ic, channel);

#if ATH_SUPPORT_IBSS_ACS
            /*
             * ACS : filter out DFS channels for IBSS mode
             */
            if((wlan_vap_get_opmode(acs->acs_vap) == IEEE80211_M_IBSS) && IEEE80211_IS_CHAN_DISALLOW_ADHOC(channel)) {
#if DEBUG_EACS
                printk( "%s skip DFS-check channel %d\n", __func__, channel->ic_freq);
#endif
                continue;
            }
#endif     	
            if ((wlan_vap_get_opmode(acs->acs_vap) == IEEE80211_M_HOSTAP) && IEEE80211_IS_CHAN_DISALLOW_HOSTAP(channel)) {
#if DEBUG_EACS
                printk( "%s skip Station only channel %d\n", __func__, channel->ic_freq);
#endif
                continue;
            }
#if DEBUG_EACS
            printk("%s adding channel %d %08x %d to list\n", __func__, 
                    channel->ic_freq, channel->ic_flags, channel->ic_ieee);
#endif
            
            if (ieee_chan_num != (u_int8_t)IEEE80211_CHAN_ANY) {
                acs->acs_chan_maps[ieee_chan_num] = acs->acs_nchans;
            }
            acs->acs_chans[acs->acs_nchans++] = channel;
        }
    }
}

/*
 * construct the available channel list
 */
static void ieee80211_acs_construct_chan_list(ieee80211_acs_t acs, enum ieee80211_phymode mode)
{
    u_int16_t nchans_2ghz = 0;

    /* reset channel mapping array */
    OS_MEMSET(&acs->acs_chan_maps, 0xff, sizeof(acs->acs_chan_maps));
    acs->acs_nchans = 0;

    if (mode == IEEE80211_MODE_AUTO) {
        /* HT20 only if IEEE80211_MODE_AUTO */
        ieee80211_acs_get_phymode_channels(acs, IEEE80211_MODE_11NG_HT20);
        nchans_2ghz = acs->acs_nchans;
        ieee80211_acs_get_phymode_channels(acs, IEEE80211_MODE_11NA_HT20);
        ieee80211_acs_check_2ghz_only(acs, nchans_2ghz);

        /* If no any HT channel available */
        if (acs->acs_nchans == 0) {
            ieee80211_acs_get_phymode_channels(acs, IEEE80211_MODE_11G);
            nchans_2ghz = acs->acs_nchans;
            ieee80211_acs_get_phymode_channels(acs, IEEE80211_MODE_11A);
            ieee80211_acs_check_2ghz_only(acs, nchans_2ghz);
        }

        /* If still no channel available */
        if (acs->acs_nchans == 0) {
            ieee80211_acs_get_phymode_channels(acs, IEEE80211_MODE_11B);
            acs->acs_scan_2ghz_only = 1;
        }
    } else if (mode == IEEE80211_MODE_11NG_HT40){
        /* if PHY mode is not AUTO, get channel list by PHY mode directly */
        ieee80211_acs_get_phymode_channels(acs, IEEE80211_MODE_11NG_HT40PLUS);
        ieee80211_acs_get_phymode_channels(acs, IEEE80211_MODE_11NG_HT40MINUS);
        acs->acs_scan_2ghz_only = 1;

    } else if (mode == IEEE80211_MODE_11NA_HT40){
        /* if PHY mode is not AUTO, get channel list by PHY mode directly */
        ieee80211_acs_get_phymode_channels(acs, IEEE80211_MODE_11NA_HT40PLUS);
        ieee80211_acs_get_phymode_channels(acs, IEEE80211_MODE_11NA_HT40MINUS);
    } else if (mode == IEEE80211_MODE_11AC_VHT40) {
        /* if PHY mode is not AUTO, get channel list by PHY mode directly */
        ieee80211_acs_get_phymode_channels(acs, IEEE80211_MODE_11AC_VHT40PLUS);
        ieee80211_acs_get_phymode_channels(acs, IEEE80211_MODE_11AC_VHT40MINUS);
    } else if (mode == IEEE80211_MODE_11AC_VHT80) {
        /* if PHY mode is not AUTO, get channel list by PHY mode directly */
        ieee80211_acs_get_phymode_channels(acs, IEEE80211_MODE_11AC_VHT80);
    } else {
        /* if PHY mode is not AUTO, get channel list by PHY mode directly */
        ieee80211_acs_get_phymode_channels(acs, mode);
    }
}

int ieee80211_acs_attach(ieee80211_acs_t *acs, 
                          wlan_dev_t          devhandle, 
                          osdev_t             osdev)
{
    if (*acs) 
        return EINPROGRESS; /* already attached ? */

    *acs = (ieee80211_acs_t) OS_MALLOC(osdev, sizeof(struct ieee80211_acs), 0);
    if (*acs) {
        OS_MEMZERO(*acs, sizeof(struct ieee80211_acs));

        /* Save handles to required objects.*/
        (*acs)->acs_ic     = devhandle; 
        (*acs)->acs_osdev  = osdev; 

        spin_lock_init(&((*acs)->acs_lock));

        return EOK;
    }

    return ENOMEM;
}

int ieee80211_acs_detach(ieee80211_acs_t *acs)
{
    if (*acs == NULL) 
        return EINPROGRESS; /* already detached ? */

    /*
   * Free synchronization objects
   */
    spin_lock_destroy(&((*acs)->acs_lock));

    OS_FREE(*acs);

    *acs = NULL;

    return EOK;
}

static void 
ieee80211_acs_post_event(ieee80211_acs_t                 acs,     
                         struct ieee80211_channel       *channel) 
{
    int                                 i,num_handlers;
    ieee80211_acs_event_handler         acs_event_handlers[IEEE80211_MAX_ACS_EVENT_HANDLERS];
    void                                *acs_event_handler_arg[IEEE80211_MAX_ACS_EVENT_HANDLERS];

    /*
     * make a local copy of event handlers list to avoid 
     * the call back modifying the list while we are traversing it.
     */ 
    spin_lock(&acs->acs_lock);
    num_handlers=acs->acs_num_handlers;
    for (i=0; i < num_handlers; ++i) {
        acs_event_handlers[i] = acs->acs_event_handlers[i];
        acs_event_handler_arg[i] = acs->acs_event_handler_arg[i];
    }
    spin_unlock(&acs->acs_lock);
    for (i = 0; i < num_handlers; ++i) {
        (acs_event_handlers[i]) (acs_event_handler_arg[i], channel);
    }
} 

/*
 * scan handler used for scan complete
 */
static void ieee80211_ht40intol_evhandler(struct ieee80211vap *originator,
                                              ieee80211_scan_event *event,
                                              void *arg)
{
    ieee80211_acs_t acs = (ieee80211_acs_t) arg;
    struct ieee80211vap *vap = acs->acs_vap;

    /* 
     * we don't need lock in evhandler since 
     * 1. scan module would guarantee that event handlers won't get called simultaneously
     * 2. acs_in_progress prevent furher access to ACS module
     */
#if 0
#if DEBUG_EACS
    printk( "%s scan_id %08X event %d reason %d \n", __func__, event->scan_id, event->type, event->reason);
#endif
#endif

#if ATH_SUPPORT_MULTIPLE_SCANS
    /*
     * Ignore notifications received due to scans requested by other modules
     * and handle new event IEEE80211_SCAN_DEQUEUED.
     */
    ASSERT(0);

    /* Ignore events reported by scans requested by other modules */
    if (acs->acs_scan_id != event->scan_id) {
        return;
    }
#endif    /* ATH_SUPPORT_MULTIPLE_SCANS */

    if ((event->type != IEEE80211_SCAN_COMPLETED) &&
        (event->type != IEEE80211_SCAN_DEQUEUED)) {
        return;
    }

    if (event->reason != IEEE80211_REASON_COMPLETED) {
#if DEBUG_EACS
        printk( "%s: Scan not totally complete. "
                          "Should not occur normally! Investigate.\n",
                         __func__);
#endif
        goto scan_done;
    }
    
    wlan_scan_table_iterate(vap, ieee80211_get_ht40intol, acs);
    ieee80211_find_ht40intol_overlap(acs, vap->iv_des_chan[vap->iv_des_mode]);

scan_done:
    ieee80211_free_ht40intol_scan_resource(acs);
    ieee80211_acs_post_event(acs, vap->iv_des_chan[vap->iv_des_mode]);

    acs->acs_in_progress = false;

    return;
}

/*
 * scan handler used for scan complete
 */
static void ieee80211_acs_scan_evhandler(struct ieee80211vap *originator, ieee80211_scan_event *event, void *arg)
{
    struct ieee80211_channel *channel;
    u_int8_t flags;
    struct ieee80211com *ic;
    ieee80211_acs_t acs;

    ic = originator->iv_ic;
    acs = (ieee80211_acs_t) arg;
    /* 
     * we don't need lock in evhandler since 
     * 1. scan module would guarantee that event handlers won't get called simultaneously
     * 2. acs_in_progress prevent furher access to ACS module
     */
#if DEBUG_EACS
    printk( "%s scan_id %08X event %d reason %d \n", 
                      __func__, event->scan_id, event->type, event->reason);
#endif
    
#if ATH_SUPPORT_MULTIPLE_SCANS
    /*
     * Ignore notifications received due to scans requested by other modules
     * and handle new event IEEE80211_SCAN_DEQUEUED.
     */
    ASSERT(0);

    /* Ignore events reported by scans requested by other modules */
    if (acs->acs_scan_id != event->scan_id) {
        return;
    }
#endif    /* ATH_SUPPORT_MULTIPLE_SCANS */

    /* 
     * Retrieve the Noise floor information and channel load 
     * in case of channel change and restart the noise floor 
     * computation for the next channel 
     */
    if( event->type == IEEE80211_SCAN_FOREIGN_CHANNEL_GET_NF ) {
        flags = ACS_CHAN_STATS_NF;
#if DEBUG_EACS
        u_int32_t now = (u_int32_t) CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP());
        printk("Requesting for CHAN STATS and NF from Target \n");
        printk("%d.%03d | %s:request stats and nf \n",  now / 1000, now % 1000, __func__);
#endif    
        ic->ic_hal_get_chan_info(ic, flags);
     }
    if ( event->type == IEEE80211_SCAN_FOREIGN_CHANNEL ) {
        /* get initial chan stats for the current channel */
        flags = ACS_CHAN_STATS;
        ic->ic_hal_get_chan_info(ic, flags);
    }
    if (event->type != IEEE80211_SCAN_COMPLETED) {
        return;
    }

    if (event->reason != IEEE80211_REASON_COMPLETED) {
#if DEBUG_EACS
        printk( "%s: Scan not totally complete. Should not occur normally! Investigate.\n",
            __func__);
#endif
        /* If scan is cancelled, ACS should invoke the scan again*/
        channel = IEEE80211_CHAN_ANYC;
        goto scan_done;
    }

    wlan_scan_table_iterate(acs->acs_vap, ieee80211_acs_get_channel_maxrssi_n_secondary_ch, acs);

    channel = ieee80211_acs_find_best_centerchan(acs);

    switch (acs->acs_vap->iv_des_mode) {
    case IEEE80211_MODE_11NG_HT40PLUS:
    case IEEE80211_MODE_11NG_HT40MINUS:
    case IEEE80211_MODE_11NG_HT40:
        wlan_scan_table_iterate(acs->acs_vap, ieee80211_get_ht40intol, acs);
        ieee80211_find_ht40intol_overlap(acs, channel);
    default:
    break;
    }

scan_done:
    ieee80211_acs_free_scan_resource(acs);
    ieee80211_acs_post_event(acs, channel);
    acs->acs_in_progress = false;

    return;
}

static void ieee80211_acs_free_scan_resource(ieee80211_acs_t acs)
{
    int    rc;
    
    /* unregister scan event handler */
    rc = wlan_scan_unregister_event_handler(acs->acs_vap, 
                                            ieee80211_acs_scan_evhandler, 
                                            (void *) acs);
    if (rc != EOK) {
        IEEE80211_DPRINTF(acs->acs_vap, IEEE80211_MSG_ACS,
                          "%s: wlan_scan_unregister_event_handler() failed handler=%08p,%08p rc=%08X\n", 
                          __func__, ieee80211_acs_scan_evhandler, acs, rc);
    }
    wlan_scan_clear_requestor_id(acs->acs_vap, acs->acs_scan_requestor);
}

static void ieee80211_free_ht40intol_scan_resource(ieee80211_acs_t acs)
{
    int    rc;
    
    /* unregister scan event handler */
    rc = wlan_scan_unregister_event_handler(acs->acs_vap,
                                            ieee80211_ht40intol_evhandler,
                                            (void *) acs);
    if (rc != EOK) {
        IEEE80211_DPRINTF(acs->acs_vap, IEEE80211_MSG_ACS,
                          "%s: wlan_scan_unregister_event_handler() failed handler=%08p,%08p rc=%08X\n", 
                          __func__, ieee80211_ht40intol_evhandler, acs, rc);
    }
    wlan_scan_clear_requestor_id(acs->acs_vap, acs->acs_scan_requestor);
}

static INLINE void
ieee80211_acs_iter_vap_channel(void *arg, struct ieee80211vap *vap, bool is_last_vap)
{
    struct ieee80211vap *current_vap = (struct ieee80211vap *) arg; 
    struct ieee80211_channel *channel;

    if (wlan_vap_get_opmode(vap) != IEEE80211_M_HOSTAP) {
        return;
    }
    if (ieee80211_acs_channel_is_set(current_vap)) {
        return;
    }
    if (vap == current_vap) {
        return;
    }

    if (ieee80211_acs_channel_is_set(vap)) {
        channel =  vap->iv_des_chan[vap->iv_des_mode];
        current_vap->iv_ic->ic_acs->acs_channel = channel;
    }

}

static int ieee80211_find_ht40intol_bss(struct ieee80211vap *vap)
{
/* XXX tunables */
#define MIN_DWELL_TIME        200  /* 200 ms */
#define MAX_DWELL_TIME        300  /* 300 ms */
    struct ieee80211com *ic = vap->iv_ic;
    ieee80211_acs_t acs = ic->ic_acs;
    ieee80211_scan_params scan_params; 
    struct ieee80211_channel *chan;
    int rc;
    u_int8_t chan_list_allocated = false;
    u_int8_t i;

    spin_lock(&acs->acs_lock);
    if (acs->acs_in_progress) {
        /* Just wait for acs done */
        spin_unlock(&acs->acs_lock);
        return EINPROGRESS;
    }

    acs->acs_in_progress = true;

    spin_unlock(&acs->acs_lock);

    /* acs_in_progress prevents others from reaching here so unlocking is OK */

    acs->acs_vap = vap;

    /* reset channel mapping array */
    OS_MEMSET(&acs->acs_chan_maps, 0xff, sizeof(acs->acs_chan_maps));
    acs->acs_nchans = 0;
    /* Get 11NG HT20 channels */
    ieee80211_acs_get_phymode_channels(acs, IEEE80211_MODE_11NG_HT20);

    if (acs->acs_nchans == 0) {
#if DEBUG_EACS
        printk("%s: Cannot construct the available channel list.\n",
                          __func__);
#endif
        goto err; 
    }

    /* register scan event handler */
    rc = wlan_scan_register_event_handler(vap, ieee80211_ht40intol_evhandler, (void *) acs);
    if (rc != EOK) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY,
                          "%s: wlan_scan_register_event_handler() failed handler=%08p,%08p rc=%08X\n",
                          __func__, ieee80211_ht40intol_evhandler, acs, rc);
    }
    wlan_scan_get_requestor_id(vap, (u_int8_t*)"acs", &acs->acs_scan_requestor);

    /* Fill scan parameter */
    OS_MEMZERO(&scan_params, sizeof(ieee80211_scan_params));
    wlan_set_default_scan_parameters(vap,&scan_params,IEEE80211_M_HOSTAP,
                                     false,true,false,true,0,NULL,0);

    scan_params.type = IEEE80211_SCAN_FOREGROUND;
    scan_params.flags = IEEE80211_SCAN_PASSIVE;
    scan_params.min_dwell_time_passive = MIN_DWELL_TIME;
    scan_params.max_dwell_time_passive = MAX_DWELL_TIME;
    scan_params.min_dwell_time_active  = MIN_DWELL_TIME;
    scan_params.max_dwell_time_active  = MAX_DWELL_TIME;

    scan_params.flags |= IEEE80211_SCAN_2GHZ;

    if(scan_params.chan_list == NULL) {
        scan_params.chan_list = (u_int32_t *) OS_MALLOC(acs->acs_osdev, sizeof(u_int32_t)*acs->acs_nchans, 0);
        chan_list_allocated = true;
        /* scan needs to be done on 2GHz  channels */
        scan_params.num_channels = 0;

        for (i = 0; i < acs->acs_nchans; i++) {
            chan = acs->acs_chans[i];
            if (IEEE80211_IS_CHAN_2GHZ(chan)) {
                scan_params.chan_list[scan_params.num_channels++] = chan->ic_freq;
            }
        }
    }


    /* If scan is invoked from ACS, Channel event notification should be
     * enabled  This is must for offload architecture
     */
    scan_params.flags |= IEEE80211_SCAN_CHAN_EVENT; 


    /* Try to issue a scan */
    if (wlan_scan_start(vap,
                        &scan_params,
                        acs->acs_scan_requestor,
                        IEEE80211_SCAN_PRIORITY_LOW,
                        &(acs->acs_scan_id)) != EOK) {
#if DEBUG_EACS
        printk( "%s: Issue a scan fail.\n",
                          __func__);
#endif
        goto err;
    }

    goto end;

err:
    ieee80211_free_ht40intol_scan_resource(acs);
    acs->acs_in_progress = false;
    ieee80211_acs_post_event(acs, vap->iv_des_chan[vap->iv_des_mode]);
end:
    if(chan_list_allocated == true) {
        OS_FREE(scan_params.chan_list);
    }
    return EOK;
#undef MIN_DWELL_TIME
#undef MAX_DWELL_TIME
}

static int ieee80211_autoselect_infra_bss_channel(struct ieee80211vap *vap)
{
/* XXX tunables */
#define MIN_DWELL_TIME        200  /* 200 ms */
#define MAX_DWELL_TIME        300  /* 300 ms */

    struct ieee80211com *ic = vap->iv_ic;
    ieee80211_acs_t acs = ic->ic_acs;
    struct ieee80211_channel *channel;
    ieee80211_scan_params scan_params; 
    u_int32_t num_vaps;
    int rc;
    u_int8_t chan_list_allocated = false;
#ifdef DEBUG_EACS
    printk("Invoking ACS module for Best channel selection \n");
#endif
    spin_lock(&acs->acs_lock);
    if (acs->acs_in_progress) {
        /* Just wait for acs done */
        spin_unlock(&acs->acs_lock);
        return EINPROGRESS;
    }
    /* check if any VAP already set channel */
    acs->acs_channel = NULL;
    ieee80211_iterate_vap_list_internal(ic, ieee80211_acs_iter_vap_channel,vap,num_vaps);

    acs->acs_in_progress = true;

    spin_unlock(&acs->acs_lock);

    /* acs_in_progress prevents others from reaching here so unlocking is OK */

    if (acs->acs_channel && (!ic->cw_inter_found)) {
        /* wlan scanner not yet started so acs_in_progress = true is OK */
        ieee80211_acs_post_event(acs, acs->acs_channel);
        acs->acs_in_progress = false;
        return EOK;
    }
    acs->acs_vap = vap;

    ieee80211_acs_construct_chan_list(acs,acs->acs_vap->iv_des_mode);
    if (acs->acs_nchans == 0) {
#if DEBUG_EACS
        printk( "%s: Cannot construct the available channel list.\n", __func__);
#endif
        goto err; 
    }
#if ATH_SUPPORT_VOW_DCS
/* update dcs information */
    if(ic->cw_inter_found && ic->ic_curchan){
        acs->acs_intr_status[ieee80211_chan2ieee(ic, ic->ic_curchan)] = 1;
        acs->acs_intr_ts[ieee80211_chan2ieee(ic, ic->ic_curchan)] = 
            (u_int32_t) CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP());
    }
#endif

    /* register scan event handler */
    rc = wlan_scan_register_event_handler(vap, ieee80211_acs_scan_evhandler, (void *) acs);
    if (rc != EOK) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACS,
                          "%s: wlan_scan_register_event_handler() failed handler=%08p,%08p rc=%08X\n", 
                          __func__, ieee80211_acs_scan_evhandler, (void *) acs, rc);
    }
    wlan_scan_get_requestor_id(vap,(u_int8_t*)"acs", &acs->acs_scan_requestor);
    

    /* Fill scan parameter */
    OS_MEMZERO(&scan_params,sizeof(ieee80211_scan_params));
    wlan_set_default_scan_parameters(vap,&scan_params,IEEE80211_M_HOSTAP,false,true,false,true,0,NULL,0);

    scan_params.type = IEEE80211_SCAN_FOREGROUND;
    scan_params.flags = IEEE80211_SCAN_PASSIVE;
    scan_params.min_dwell_time_passive = MIN_DWELL_TIME;
    scan_params.max_dwell_time_passive = MAX_DWELL_TIME;
    scan_params.min_dwell_time_active  = MIN_DWELL_TIME;
    scan_params.max_dwell_time_active  = MAX_DWELL_TIME;

     /* If scan is invoked from ACS, Channel event notification should be
      * enabled  This is must for offload architecture
      */
    scan_params.flags |= IEEE80211_SCAN_CHAN_EVENT; 

    switch (vap->iv_des_mode)
    {
    case IEEE80211_MODE_11A:
    case IEEE80211_MODE_TURBO_A:
    case IEEE80211_MODE_11NA_HT20:
    case IEEE80211_MODE_11NA_HT40PLUS:
    case IEEE80211_MODE_11NA_HT40MINUS:
    case IEEE80211_MODE_11NA_HT40:
    case IEEE80211_MODE_11AC_VHT20: 
    case IEEE80211_MODE_11AC_VHT40PLUS: 
    case IEEE80211_MODE_11AC_VHT40MINUS:
    case IEEE80211_MODE_11AC_VHT40:
    case IEEE80211_MODE_11AC_VHT80:     
        scan_params.flags |= IEEE80211_SCAN_5GHZ; 
        break;

    case IEEE80211_MODE_11B:
    case IEEE80211_MODE_11G:
    case IEEE80211_MODE_TURBO_G:
    case IEEE80211_MODE_11NG_HT20:
    case IEEE80211_MODE_11NG_HT40PLUS:
    case IEEE80211_MODE_11NG_HT40MINUS:
    case IEEE80211_MODE_11NG_HT40:
        scan_params.flags |= IEEE80211_SCAN_2GHZ;
        break;

    default:
        if (acs->acs_scan_2ghz_only) {
            scan_params.flags |= IEEE80211_SCAN_2GHZ; 
        } else {
            scan_params.flags |= IEEE80211_SCAN_ALLBANDS; 
        }
        break;
    }
    acs->acs_nchans_scan = 0;
    /* If scan needs to be done on specific band or specific set of channels */
    if ((scan_params.flags & (IEEE80211_SCAN_ALLBANDS)) != IEEE80211_SCAN_ALLBANDS) {
        wlan_chan_t *chans;
        u_int16_t nchans = 0;
        u_int8_t i;

        scan_params.num_channels = 0;
        
        chans = (wlan_chan_t *)OS_MALLOC(acs->acs_osdev,
                        sizeof(wlan_chan_t) * IEEE80211_CHAN_MAX, 0);
        if (chans == NULL)
            goto err2;
        /* HT20 only if IEEE80211_MODE_AUTO */
   
        if (scan_params.flags & IEEE80211_SCAN_2GHZ) {
            nchans = wlan_get_channel_list(ic, IEEE80211_MODE_11NG_HT20,
                                            chans, IEEE80211_CHAN_MAX);
            /* If no any HT channel available */
            if (nchans == 0) {
                 nchans = wlan_get_channel_list(ic, IEEE80211_MODE_11G,
                                            chans, IEEE80211_CHAN_MAX);
            }
        } else {
            nchans = wlan_get_channel_list(ic, IEEE80211_MODE_11NA_HT20,
                                            chans, IEEE80211_CHAN_MAX);
            /* If no any HT channel available */
            if (nchans == 0) {
                 nchans = wlan_get_channel_list(ic, IEEE80211_MODE_11A,
                                            chans, IEEE80211_CHAN_MAX);
            }
        }
        /* If still no channel available */
        if (nchans == 0) {
            nchans = wlan_get_channel_list(ic, IEEE80211_MODE_11B,
                                        chans, IEEE80211_CHAN_MAX);
        }


        if(scan_params.chan_list == NULL) {
            scan_params.chan_list = (u_int32_t *) OS_MALLOC(acs->acs_osdev, sizeof(u_int32_t)*nchans, 0);
            chan_list_allocated = true;
        }
        for (i = 0; i < nchans; i++)
        {
            const wlan_chan_t chan = chans[i];
            if ((scan_params.flags & IEEE80211_SCAN_2GHZ) ?
                IEEE80211_IS_CHAN_2GHZ(chan) : IEEE80211_IS_CHAN_5GHZ(chan)) {
                scan_params.chan_list[scan_params.num_channels++] = chan->ic_freq;
                acs->acs_ieee_chan[i] = ieee80211_chan2ieee(ic, chan);
            }
        }
        acs->acs_nchans_scan = nchans;
        OS_FREE(chans);
    }


    /* clear ACS RSSI table and nbss entries*/
    OS_MEMZERO(&acs->acs_chan_maxrssi,sizeof(acs->acs_chan_maxrssi));
    OS_MEMZERO(&acs->acs_chan_minrssi, sizeof(acs->acs_chan_minrssi));
    OS_MEMZERO(&acs->acs_chan_nbss, sizeof(acs->acs_chan_nbss));

    /* Try to issue a scan */
#if ATH_SUPPORT_SPECTRAL
    /* For ACS spectral sample indication priority should be
       low (0) to indicate scan entries to mgmt layer */
    ic->ic_start_spectral_scan(ic,0);
#endif

    /*Flush scan table before starting scan */
    wlan_scan_table_flush(vap);
    if (wlan_scan_start(vap,
                        &scan_params,
                        acs->acs_scan_requestor,
                        IEEE80211_SCAN_PRIORITY_LOW,
                        &(acs->acs_scan_id)) != EOK) {
#if DEBUG_EACS
        printk( "%s: Issue a scan fail.\n",
            __func__);
#endif
        goto err2; 
    }

    goto end;

err2:
    ieee80211_acs_free_scan_resource(acs);
err:
    /* select the first available channel to start */
    channel = ieee80211_find_dot11_channel(ic, 0, vap->iv_des_mode);
    ieee80211_acs_post_event(acs, channel);
#if DEBUG_EACS
    printk( "%s: Use the first available channel=%d to start\n",
        __func__, ieee80211_chan2ieee(ic, channel));
#endif

#if ATH_SUPPORT_SPECTRAL
    ic->ic_stop_spectral_scan(ic);
#endif
    acs->acs_in_progress = false;
end:
    if(chan_list_allocated == true)
        OS_FREE(scan_params.chan_list);
    return EOK;
#undef MIN_DWELL_TIME
#undef MAX_DWELL_TIME
}

static int ieee80211_acs_register_event_handler(ieee80211_acs_t          acs, 
                                         ieee80211_acs_event_handler evhandler, 
                                         void                         *arg)
{
    int    i;

    for (i = 0; i < IEEE80211_MAX_ACS_EVENT_HANDLERS; ++i) {
        if ((acs->acs_event_handlers[i] == evhandler) &&
            (acs->acs_event_handler_arg[i] == arg)) {
            return EEXIST; /* already exists */
        }
    }

    if (acs->acs_num_handlers >= IEEE80211_MAX_ACS_EVENT_HANDLERS) {
        return ENOSPC;
    }

    spin_lock(&acs->acs_lock);
    acs->acs_event_handlers[acs->acs_num_handlers] = evhandler;
    acs->acs_event_handler_arg[acs->acs_num_handlers++] = arg;
    spin_unlock(&acs->acs_lock);

    return EOK;
}

static int ieee80211_acs_unregister_event_handler(ieee80211_acs_t          acs, 
                                           ieee80211_acs_event_handler evhandler, 
                                           void                         *arg)
{
    int    i;

    spin_lock(&acs->acs_lock);
    for (i = 0; i < IEEE80211_MAX_ACS_EVENT_HANDLERS; ++i) {
        if ((acs->acs_event_handlers[i] == evhandler) &&
            (acs->acs_event_handler_arg[i] == arg)) {
            /* replace event handler being deleted with the last one in the list */
            acs->acs_event_handlers[i]    = acs->acs_event_handlers[acs->acs_num_handlers - 1];
            acs->acs_event_handler_arg[i] = acs->acs_event_handler_arg[acs->acs_num_handlers - 1];

            /* clear last event handler in the list */
            acs->acs_event_handlers[acs->acs_num_handlers - 1]    = NULL;
            acs->acs_event_handler_arg[acs->acs_num_handlers - 1] = NULL;
            acs->acs_num_handlers--;

            spin_unlock(&acs->acs_lock);

            return EOK;
        }
    }
    spin_unlock(&acs->acs_lock);

    return ENXIO;
   
}

int wlan_autoselect_register_event_handler(wlan_if_t                    vaphandle, 
                                           ieee80211_acs_event_handler evhandler, 
                                           void                         *arg)
{
    return ieee80211_acs_register_event_handler(vaphandle->iv_ic->ic_acs,
                                                 evhandler, 
                                                 arg);
}

int wlan_autoselect_unregister_event_handler(wlan_if_t                    vaphandle, 
                                             ieee80211_acs_event_handler evhandler,
                                             void                         *arg)
{
    return ieee80211_acs_unregister_event_handler(vaphandle->iv_ic->ic_acs, evhandler, arg);
}

int wlan_autoselect_in_progress(wlan_if_t vaphandle)
{
    if (!vaphandle->iv_ic->ic_acs) return 0;
    return ieee80211_acs_in_progress(vaphandle->iv_ic->ic_acs);
}


int wlan_autoselect_find_infra_bss_channel(wlan_if_t             vaphandle)
{
    return ieee80211_autoselect_infra_bss_channel(vaphandle);
}

int wlan_attempt_ht40_bss(wlan_if_t             vaphandle)
{
    return ieee80211_find_ht40intol_bss(vaphandle);
}

int wlan_acs_find_best_channel(struct ieee80211vap *vap, int *bestfreq, int num)
{
    
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_channel *best_11na = NULL;
    struct ieee80211_channel *best_11ng = NULL;
    struct ieee80211_channel *best_overall = NULL;
    //struct ieee80211_acs myacs;
    int retv = 0;
     
    ieee80211_acs_t acs = ic->ic_acs;
    ieee80211_acs_t temp_acs;
    
    
    //temp_acs = &myacs;

    //temp_acs = (ieee80211_acs_t) kmalloc(sizeof(struct ieee80211_acs), GFP_KERNEL);
    temp_acs = (ieee80211_acs_t) OS_MALLOC(acs->acs_osdev, sizeof(struct ieee80211_acs), 0);
    
    if (temp_acs) {
        OS_MEMZERO(temp_acs, sizeof(struct ieee80211_acs));
    }
    else {
       adf_os_print("%s: malloc failed \n",__func__);
       return ENOMEM;
    }
    
    temp_acs->acs_ic = ic;
    temp_acs->acs_vap = vap;
    temp_acs->acs_osdev = acs->acs_osdev;
    temp_acs->acs_num_handlers = 0;
    temp_acs->acs_in_progress = true;
    temp_acs->acs_scan_2ghz_only = 0;
    temp_acs->acs_channel = NULL;
    temp_acs->acs_nchans = 0;
    
    
    
    ieee80211_acs_construct_chan_list(temp_acs,IEEE80211_MODE_AUTO);
    if (temp_acs->acs_nchans == 0) {
#if DEBUG_EACS
        printk( "%s: Cannot construct the available channel list.\n", __func__);
#endif
        retv = -1;
        goto err; 
    }
    
        
    
    wlan_scan_table_iterate(temp_acs->acs_vap, ieee80211_acs_get_channel_maxrssi_n_secondary_ch, temp_acs);

    best_11na = ieee80211_acs_find_best_11na_centerchan(temp_acs);
    best_11ng = ieee80211_acs_find_best_11ng_centerchan(temp_acs);

    if (temp_acs->acs_minrssi_11na > temp_acs->acs_avgrssi_11ng) {
        best_overall = best_11ng;
    } else {
        best_overall = best_11na;
    }    
 
    if( best_11na==NULL || best_11ng==NULL || best_overall==NULL) {
        adf_os_print("%s: null channel \n",__func__);
        retv = -1;
        goto err;
    }
    
    bestfreq[0] = (int) best_11na->ic_freq;
    bestfreq[1] = (int) best_11ng->ic_freq;
    bestfreq[2] = (int) best_overall->ic_freq;

err:
    OS_FREE(temp_acs);
    //kfree(temp_acs);
    return retv;
    
}

/*
 * Update NF and Channel Load upon WMI event
 */
void ieee80211_acs_stats_update(ieee80211_acs_t acs,
                                    u_int8_t flags,
                                    u_int ieee_chan_num,
                                    int16_t acs_noisefloor,
                                    struct ieee80211_chan_stats *chan_stats)
{
    u_int32_t temp = 0;
#if DEBUG_EACS  
    u_int32_t now = (u_int32_t) CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP());
    printk("%d.%03d | %s: \n",  now / 1000, now % 1000, __func__);
#endif
    if(acs == NULL) {
       printk("ERROR: ACS is not initialized \n");
       return;
    }
    if(flags == ACS_CHAN_STATS_NF) {
#if DEBUG_EACS  
        printk("Updating the CHAN STATS and NF both\n");
#endif
        if (ieee_chan_num != IEEE80211_CHAN_ANY) {
            acs->acs_noisefloor[ieee_chan_num] = acs_noisefloor;
        }
#define MIN_CLEAR_CNT_DIFF 1000
         /* get final chan stats for the current channel*/
        acs->acs_chan_load[ieee_chan_num] = chan_stats->chan_clr_cnt - acs->acs_chan_load[ieee_chan_num] ;
        acs->acs_cycle_count[ieee_chan_num] = chan_stats->cycle_cnt - acs->acs_cycle_count[ieee_chan_num] ;
        /* make sure when new clr_cnt is more than old clr cnt, ch utilization is non-zero */
        if (acs->acs_chan_load[ieee_chan_num] > MIN_CLEAR_CNT_DIFF ){
            temp = (u_int32_t)(100* acs->acs_chan_load[ieee_chan_num]);
            temp = (u_int32_t)( temp/(acs->acs_cycle_count[ieee_chan_num]));
            acs->acs_chan_load[ieee_chan_num] = MAX( 1,temp);
        } else {
            acs->acs_chan_load[ieee_chan_num] = 0;
        }
#if ATH_SUPPORT_SPECTRAL
        acs->acs_channel_loading[ieee_chan_num] = get_eacs_control_duty_cycle(acs);
#endif
    } 
    else if(flags == ACS_CHAN_STATS) {
#if DEBUG_EACS  
        printk("Updating the CHAN STATS alone \n");
#endif
        acs->acs_chan_load[ieee_chan_num] = chan_stats->chan_clr_cnt;
        acs->acs_cycle_count[ieee_chan_num] = chan_stats->cycle_cnt;
#if ATH_SUPPORT_SPECTRAL
        /*If spectral ACS stats are not reset on channel change, they will be reset here */
        ieee80211_init_spectral_chan_loading(acs->acs_ic, 
                                              ieee80211_ieee2mhz(ieee_chan_num, 0),0);
#endif
    }
#if DEBUG_EACS  
       printk("%s: final_stats noise: %d ch: %d ch load: %d cycle cnt: %d \n ",__func__, 
               acs->acs_noisefloor[ieee_chan_num], ieee_chan_num,
               acs->acs_chan_load[ieee_chan_num], acs->acs_cycle_count[ieee_chan_num] ); 
#endif
#undef MIN_CLEAR_CNT_DIFF
}

int ieee80211_acs_scan_report(struct ieee80211com *ic, struct ieee80211_acs_dbg *acs_r)
{
   ieee80211_acs_t acs = ic->ic_acs;
   struct ieee80211_channel *channel = NULL;
   u_int8_t i, ieee_chan;
   u_int16_t nchans;

   if(ieee80211_acs_in_progress(acs)) {
       printk("ACS scan is in progress, Please request for the report after a while... \n");
       acs_r->nchans = 0;
       return 0;
   }
   nchans = (acs->acs_nchans_scan)?(acs->acs_nchans_scan):(acs->acs_nchans);
   i = acs_r->entry_id;
   if(i >= nchans) {
       acs_r->nchans = 0;
       return 0;

   }
   acs_r->nchans = nchans;
    /* If scan channel list is not generated by ACS, 
	acs_chans[i] have all channels */
   if(acs->acs_nchans_scan == 0) {
      channel = acs->acs_chans[i];
      ieee_chan = ieee80211_chan2ieee(acs->acs_ic, channel);
      acs_r->chan_freq = ieee80211_chan2freq(acs->acs_ic, channel);
   } else {
      ieee_chan = acs->acs_ieee_chan[i];
      acs_r->chan_freq = ieee80211_ieee2mhz(ieee_chan,0);
   }
   acs_r->ieee_chan = ieee_chan;
   acs_r->chan_nbss = acs->acs_chan_nbss[ieee_chan];
   acs_r->chan_maxrssi = acs->acs_chan_maxrssi[ieee_chan];
   acs_r->chan_minrssi = acs->acs_chan_minrssi[ieee_chan];
   acs_r->noisefloor = acs->acs_noisefloor[ieee_chan];
   acs_r->channel_loading = acs->acs_channel_loading[ieee_chan];
   acs_r->chan_load = acs->acs_chan_load[ieee_chan];
   acs_r->sec_chan = acs->acs_sec_chan[ieee_chan];
 
   return 0;
}

int wlan_acs_scan_report(wlan_if_t vaphandle,void *acs_rp)
{
   struct ieee80211_acs_dbg *acs_r = (struct ieee80211_acs_dbg *)acs_rp;
   return ieee80211_acs_scan_report(vaphandle->iv_ic,acs_r);
}
#if ATH_SUPPORT_SPECTRAL

/*
 * Function     : update_eacs_avg_rssi
 * Description  : Update average RSSI
 * Input        : Pointer to acs struct
                  nfc_ctl_rssi  - control chan rssi
                  nfc_ext_rssi  - extension chan rssi
 * Output       : Noisefloor
 *
 */
static void update_eacs_avg_rssi(ieee80211_acs_t acs, int8_t nfc_ctl_rssi, int8_t nfc_ext_rssi)
{
    int temp=0;

    if(acs->acs_ic->ic_cwm_get_width(acs->acs_ic) == IEEE80211_CWM_WIDTH40) {
        // HT40 mode
        temp = (acs->ext_eacs_avg_rssi * (acs->ext_eacs_spectral_reports));
        temp += nfc_ext_rssi;
        acs->ext_eacs_spectral_reports++;
        acs->ext_eacs_avg_rssi = (temp / acs->ext_eacs_spectral_reports);
    }


    temp = (acs->ctl_eacs_avg_rssi * (acs->ctl_eacs_spectral_reports));
    temp += nfc_ctl_rssi;

    acs->ctl_eacs_spectral_reports++;
    acs->ctl_eacs_avg_rssi = (temp / acs->ctl_eacs_spectral_reports);
}


/*
 * Function     : update EACS Thresholds
 * Description  : Updates EACS Thresholds
 * Input        : Pointer to acs struct
                  ctl_nf - control chan nf
                  ext_nf  - extension chan rs
 * Output       : void
 *
 */
void update_eacs_thresholds(ieee80211_acs_t acs,int8_t ctrl_nf,int8_t ext_nf)
{

    acs->ctl_eacs_rssi_thresh = ctrl_nf + 10;

    if (acs->acs_ic->ic_cwm_get_width(acs->acs_ic) == IEEE80211_CWM_WIDTH40) {
        acs->ext_eacs_rssi_thresh = ext_nf + 10;
    }

    acs->ctl_eacs_rssi_thresh = 32;
    acs->ext_eacs_rssi_thresh = 32;
}

/*
 * Function     : ieee80211_update_eacs_counters
 * Description  : Update EACS counters
 * Input        : Pointer to ic struct
                  nfc_ctl_rssi  - control chan rssi
                  nfc_ext_rssi  - extension chan rssi
                  ctl_nf - control chan nf
                  ext_nf  - extension chan rs
 * Output       : Noisefloor
 *
 */
void ieee80211_update_eacs_counters(struct ieee80211com *ic, int8_t nfc_ctl_rssi,
                                     int8_t nfc_ext_rssi,int8_t ctrl_nf, int8_t ext_nf)
{
    ieee80211_acs_t acs = ic->ic_acs;


    if(!ieee80211_acs_in_progress(acs)) {
        return;
    }
    acs->eacs_this_scan_spectral_data++;
    update_eacs_thresholds(acs,ctrl_nf,ext_nf);
    update_eacs_avg_rssi(acs, nfc_ctl_rssi, nfc_ext_rssi);

    if (ic->ic_cwm_get_width(ic) == IEEE80211_CWM_WIDTH40) {
        // HT40 mode
        if (nfc_ext_rssi > acs->ext_eacs_rssi_thresh){
            acs->ext_eacs_interf_count++;
        }
        acs->ext_eacs_duty_cycle=((acs->ext_eacs_interf_count * 100)/acs->eacs_this_scan_spectral_data);
    }
    if (nfc_ctl_rssi > acs->ctl_eacs_rssi_thresh){
        acs->ctl_eacs_interf_count++;
    }

    acs->ctl_eacs_duty_cycle=((acs->ctl_eacs_interf_count * 100)/acs->eacs_this_scan_spectral_data);
#if DEBUG_EACS  
    if(acs->ctl_eacs_interf_count > acs->eacs_this_scan_spectral_data) {
        printk("eacs interf count is greater than spectral samples %d:%d \n",
                      acs->ctl_eacs_interf_count,acs->eacs_this_scan_spectral_data);
    }
#endif
}

/*
 * Function     : get_eacs_control_duty_cycle
 * Description  : Get EACS control duty cycle
 * Input        : Pointer to acs Struct
 * Output       : Duty Cycle
 *
 */
int get_eacs_control_duty_cycle(ieee80211_acs_t acs)
{
    return acs->ctl_eacs_duty_cycle;
}

/* Function     : get_eacs_extension_duty_cycle
 * Description  : Get EACS extension duty cycle
 * Input        : Pointer to  acs struct
 * Output       : Duty Cycle
 *
 */
int get_eacs_extension_duty_cycle(ieee80211_acs_t acs)
{
    return acs->ext_eacs_duty_cycle;
}

/*
 * Function     : print_chan_loading_details
 * Description  : Debug function to print channel loading information
 * Input        : Pointer to ic
 * Output       : void
 *
 */
static void print_chan_loading_details(struct ieee80211com *ic)
{
    ieee80211_acs_t acs = ic->ic_acs;
    printk("ctl_chan_freq       = %d\n",acs->ctl_chan_frequency);
    printk("ctl_interf_count    = %d\n",acs->ctl_eacs_interf_count);
    printk("ctl_duty_cycle      = %d\n",acs->ctl_eacs_duty_cycle);
    printk("ctl_chan_loading    = %d\n",acs->ctl_chan_loading);
    printk("ctl_nf              = %d\n",acs->ctl_chan_noise_floor);

    if (ic->ic_cwm_get_width(ic) == IEEE80211_CWM_WIDTH40) {
        // HT40 mode
        printk("ext_chan_freq       = %d\n",acs->ext_chan_frequency);
        printk("ext_interf_count    = %d\n",acs->ext_eacs_interf_count);
        printk("ext_duty_cycle      = %d\n",acs->ext_eacs_duty_cycle);
        printk("ext_chan_loading    = %d\n",acs->ext_chan_loading);
        printk("ext_nf              = %d\n",acs->ext_chan_noise_floor);
    }

    printk("%s this_scan_spectral_data count = %d\n", __func__, acs->eacs_this_scan_spectral_data);
}



/*
 * Function     : ieee80211_init_spectral_chan_loading
 * Description  : Initializes Channel loading information
 * Input        : Pointer to ic
                  current_channel - control channel freq
                  ext_channel - extension channel freq
 * Output       : void
 *
 */
void ieee80211_init_spectral_chan_loading(struct ieee80211com *ic, 
                                           int current_channel, int ext_channel)
{

    ieee80211_acs_t acs = ic->ic_acs;

    if(!ieee80211_acs_in_progress(acs)) {
        return;
    }
    if ((current_channel == 0) && (ext_channel == 0)) {
        return;
    }
    /* Check if channel change has happened in this reset */
    if(acs->ctl_chan_frequency != current_channel) {
        /* If needed, check the channel loading details
         * print_chan_loading_details(spectral);
         */
        acs->eacs_this_scan_spectral_data = 0;
       
        if (ic->ic_cwm_get_width(ic) == IEEE80211_CWM_WIDTH40) {
            // HT40 mode
            acs->ext_eacs_interf_count = 0;
            acs->ext_eacs_duty_cycle   = 0;
            acs->ext_chan_loading      = 0;
            acs->ext_chan_noise_floor  = 0;
            acs->ext_chan_frequency    = ext_channel;
        }

        acs->ctl_eacs_interf_count = 0;
        acs->ctl_eacs_duty_cycle   = 0;
        acs->ctl_chan_loading      = 0;
        acs->ctl_chan_noise_floor  = 0;
        acs->ctl_chan_frequency    = current_channel;

        acs->ctl_eacs_rssi_thresh = SPECTRAL_EACS_RSSI_THRESH;
        acs->ext_eacs_rssi_thresh = SPECTRAL_EACS_RSSI_THRESH;

        acs->ctl_eacs_avg_rssi = SPECTRAL_EACS_RSSI_THRESH;
        acs->ext_eacs_avg_rssi = SPECTRAL_EACS_RSSI_THRESH;

        acs->ctl_eacs_spectral_reports = 0;
        acs->ext_eacs_spectral_reports = 0;

    }

}

/*
 * Function     : ieee80211_get_spectral_freq_loading
 * Description  : Get EACS control duty cycle
 * Input        : Pointer to ic
 * Output       : Duty Cycle
 *
 */

int ieee80211_get_spectral_freq_loading(struct ieee80211com *ic)
{
    ieee80211_acs_t acs = ic->ic_acs;
    int duty_cycles;
    duty_cycles = ((acs->ctl_eacs_duty_cycle > acs->ext_eacs_duty_cycle) ? 
                                                acs->ctl_eacs_duty_cycle : 
                                                acs->ext_eacs_duty_cycle);
    return duty_cycles;
}
#endif
#else
int wlan_autoselect_register_event_handler(wlan_if_t                    vaphandle, 
                                           ieee80211_acs_event_handler evhandler, 
                                           void                         *arg)
{
    return EPERM;
}

int wlan_autoselect_unregister_event_handler(wlan_if_t                    vaphandle, 
                                             ieee80211_acs_event_handler evhandler,
                                             void                         *arg)
{
    return EPERM;
}

int wlan_autoselect_in_progress(wlan_if_t vaphandle)
{
    return 0;
}

int wlan_autoselect_find_infra_bss_channel(wlan_if_t             vaphandle)
{
    return EPERM;
}

int wlan_attempt_ht40_bss(wlan_if_t             vaphandle)
{
    return EPERM;
}

int wlan_acs_find_best_channel(struct ieee80211vap *vap, int *bestfreq, int num){

        return EINVAL;
}

int wlan_acs_scan_report(wlan_if_t vaphandle, void *acs_rp)
{
   struct ieee80211_acs_dbg *acs_r = (struct ieee80211_acs_dbg *)acs_rp;
   acs_r->nchans = 0;
   printk("ACS is not enabled\n");
   return EPERM;  
}
#undef DEBUG_EACS 
#endif

