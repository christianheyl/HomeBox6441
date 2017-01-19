/*
 * Copyright (c) 2010, Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */

#include <ieee80211_channel.h>
#include <ieee80211_var.h>
#include <ieee80211_scan.h>
#include <ieee80211_resmgr.h>

#include "ieee80211_sme_api.h"
#include "ieee80211_sm.h"
#include "if_athvar.h"

#if UMAC_SUPPORT_DFS
//MERGE_TODO Remove this line #define IS_CHANNEL_WEATHER_RADAR(chan) ((chan->ic_freq >= 5600) && (chan->ic_freq <= 5650))
/*
 * Print a console message with the device name prepended.
 */
void
if_printf( osdev_t dev, const char *fmt, ...)
{
    va_list ap;
    char buf[512];              /* XXX */

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    
    printk("\n %s\n", buf);     /* TODO: print device name also */
}

static void
change_channel(struct ieee80211com *ic,
	       struct ieee80211_channel *chan)
{
    ic->ic_curchan = chan;
    ic->ic_set_channel(ic);
}

static int ieee80211_random_channel(struct ieee80211com *ic)
{
    int chanStart,n=0;
    u_int32_t curChanFlags, chan_flags, chan_flagext=0;

    int numGChannels=0;
    int numAChannels=0;
    int j;
    u_int8_t available_chan_idx[IEEE80211_CHAN_MAX+1] = {0}; /* 256 bytes on stack, should be ok */
    int available_chan_count = 0;
    
    /* Pick a random channel */

    /* Find how many G channels are present in the channel list
    * Assuming all G channels are present at the begining of
    * the list, followed by all A channels
     */

    for (j = 0; j < ic->ic_nchans; j++)
    {
        chan_flags = ic->ic_channels[j].ic_flags;
        if(chan_flags & IEEE80211_CHAN_2GHZ)
        {
            numGChannels++;
            continue;
        }
        else
            break;
    }

    numAChannels = (ic->ic_nchans - numGChannels);
    chanStart = numGChannels ; //+ ( OS_GET_TICKS() % numAChannels);

    curChanFlags = (ic->ic_curchan->ic_flags) & IEEE80211_CHAN_ALL;
    if ( ic->ic_flags_ext & IEEE80211_FEXT_BLKDFSCHAN )
	curChanFlags &= ~IEEE80211_CHAN_DFS;
    
    for (n = 0; n < ic->ic_nchans; chanStart++, n++)
	{
	    if (chanStart == ic->ic_nchans)
		chanStart = 0;
	    chan_flags = ic->ic_channels[chanStart].ic_flags;
	    chan_flagext = ic->ic_channels[chanStart].ic_flagext;
        /* these channels have CAC of 10 minutes so skipping these */
#if 0 //MERGETODO ATH_SUPPORT_DFS
        if(ic->ic_no_weather_radar_chan) {
        /*we should also avoid this channel in 
          HT40 mode as extension channel will be on 5600 */
#define ADJACENT_WEATHER_RADAR_CHANNEL   5580 
            u_int32_t freq = ieee80211_chan2freq(ic,&ic->ic_channels[chanStart]);

            if(((IS_CHANNEL_WEATHER_RADAR(freq)) 
                        || ((IEEE80211_CHAN_11NA_HT40PLUS & chan_flags)
                            && (ADJACENT_WEATHER_RADAR_CHANNEL == freq)))
                    && (DFS_ETSI_DOMAIN == ic->ic_get_dfsdomain(ic))) 
            {
				continue;
			}
#undef ADJACENT_WEATHER_RADAR_CHANNEL         
		}
#endif        
	    /*
	     * (1) skip static turbo channel as it will require STA to be in
	     * static turbo to work.
	     * (2) skip channel which's marked with radar detction
	     * (3) WAR: we allow user to config not to use any DFS channel
	     */
	    /* When we pick a channel, skip excluded 11D channels. See bug 3124 */
	    
	    if ((chan_flags & IEEE80211_CHAN_STURBO) ||
		(chan_flags & IEEE80211_CHAN_RADAR)  ||
		(chan_flagext & IEEE80211_CHAN_11D_EXCLUDED) ||
		(chan_flags & IEEE80211_CHAN_DFS &&
		 ic->ic_flags_ext & IEEE80211_FEXT_BLKDFSCHAN ))
		continue;
	    if ((chan_flags & IEEE80211_CHAN_ALL) == curChanFlags) {
		//break;
             available_chan_idx[available_chan_count++] = chanStart;
             if (available_chan_count >= IEEE80211_CHAN_MAX + 1) 
                break;
           }
	}
    if(available_chan_count)
    {
        u_int32_t   random_byte = 0;
        OS_GET_RANDOM_BYTES(&random_byte,1);
        j = (random_byte + OS_GET_TICKS() ) % available_chan_count;
        chanStart = (available_chan_idx[j]);
#if 0 //DEBUG code  
 
        for (n = 0; n < available_chan_count; n++) {
        printk("%s[%d] idx %d chan %d flags 0x%08x\n", __func__, __LINE__, 
                available_chan_idx[n], ic->ic_channels[available_chan_idx[n]].ic_ieee, ic->ic_channels[available_chan_idx[n]].ic_flags);
        }
        printk("%s[%d] random %d avail_count %d rand_idx %d\n", __func__, __LINE__, random_byte, available_chan_count, j);
        printk("%s[%d] Selected chan %d flags 0x%08x chanStart %d \n", __func__, __LINE__, 
                ic->ic_channels[chanStart].ic_ieee, ic->ic_channels[chanStart].ic_flags, chanStart);
 #endif
        return chanStart;
    }
    return -1;

}
/*update the bss channel info on all vaps */
static void ieee80211_vap_iter_update_bss_chan(void *arg, struct ieee80211vap *vap)
{
      struct ieee80211_channel *bsschan = (struct ieee80211_channel *) arg;
 
      vap->iv_bsschan = bsschan;
      if(vap->iv_bss)
      {
          vap->iv_bss->ni_chan = bsschan;
      }
}

#if ATH_SUPPORT_IBSS_DFS

/*
 *This function is also defined at ieee80211_wireless.c. However, it is not active yet with compilation flag.
 *Once it is actived, maybe we should remove one of it.
 */
#define IEEE80211_MODE_TURBO_STATIC_A   IEEE80211_MODE_MAX
static int
ieee80211_check_mode_consistency(struct ieee80211com *ic,int mode,struct ieee80211_channel *c)
{
    if (c == IEEE80211_CHAN_ANYC) return 0;
    switch (mode)
    {
    case IEEE80211_MODE_11B:
        if(IEEE80211_IS_CHAN_B(c))
            return 0;
        else
            return 1;
        break;

    case IEEE80211_MODE_11G:
        if(IEEE80211_IS_CHAN_ANYG(c))
            return 0;
        else
            return 1;
        break;

    case IEEE80211_MODE_11A:
        if(IEEE80211_IS_CHAN_A(c))
            return 0;
        else
            return 1;
        break;

    case IEEE80211_MODE_TURBO_STATIC_A:
        if(IEEE80211_IS_CHAN_A(c) && IEEE80211_IS_CHAN_STURBO(c) )
            return 0;
        else
            return 1;
        break;

    case IEEE80211_MODE_AUTO:
        return 0;
        break;

    case IEEE80211_MODE_11NG_HT20:
        if(IEEE80211_IS_CHAN_11NG_HT20(c))
            return 0;
        else
            return 1;
        break;

    case IEEE80211_MODE_11NG_HT40PLUS:
        if(IEEE80211_IS_CHAN_11NG_HT40PLUS(c))
            return 0;
        else
            return 1;
        break;

    case IEEE80211_MODE_11NG_HT40MINUS:
        if(IEEE80211_IS_CHAN_11NG_HT40MINUS(c))
            return 0;
        else
            return 1;
        break;

    case IEEE80211_MODE_11NG_HT40:
        if(IEEE80211_IS_CHAN_11NG_HT40MINUS(c) || IEEE80211_IS_CHAN_11NG_HT40PLUS(c))
            return 0;
        else
            return 1;
        break;

    case IEEE80211_MODE_11NA_HT20:
        if(IEEE80211_IS_CHAN_11NA_HT20(c))
            return 0;
        else
            return 1;
        break;

    case IEEE80211_MODE_11NA_HT40PLUS:
        if(IEEE80211_IS_CHAN_11NA_HT40PLUS(c))
            return 0;
        else
            return 1;
        break;

    case IEEE80211_MODE_11NA_HT40MINUS:
        if(IEEE80211_IS_CHAN_11NA_HT40MINUS(c))
            return 0;
        else
            return 1;
        break;

    case IEEE80211_MODE_11NA_HT40:
        if(IEEE80211_IS_CHAN_11NA_HT40MINUS(c) || IEEE80211_IS_CHAN_11NA_HT40PLUS(c))
            return 0;
        else
            return 1;
        break;
    }
    return 1;

}
#undef  IEEE80211_MODE_TURBO_STATIC_A

/*
 * Check with our own IBSS DFS list and see if this channel is radar free.
 * return 1 if it is radar free.
 */
static int ieee80211_checkDFS_free(struct ieee80211vap *vap, struct ieee80211_channel *pchannel)
{
    int isradarfree = 1;
    u_int   i = 0;

    for (i = (vap->iv_ibssdfs_ie_data.len - IBSS_DFS_ZERO_MAP_SIZE)/sizeof(struct channel_map_field); i > 0; i--) {
        if (vap->iv_ibssdfs_ie_data.ch_map_list[i-1].ch_num == pchannel->ic_ieee) {
            if (vap->iv_ibssdfs_ie_data.ch_map_list[i-1].ch_map.radar) {
                isradarfree = 0;
            }
            break;
        }
    }    
    
    return isradarfree; 
}

/*
 * Found next DFS free channel for ibss. Noted that this function currently only used for ibss dfs.
 */
static void * 
ieee80211_next_channel(struct ieee80211vap *vap, struct ieee80211_channel *pchannel)
{
    int target_channel = 0;
    struct ieee80211_channel *ptarget_channel = pchannel;
    u_int i;
    u_int8_t startfromhead = 0;
    u_int8_t foundfitchan = 0;
    struct ieee80211com *ic = vap->iv_ic;

    for (i = 0; i < ic->ic_nchans; i++) {
        if(!ieee80211_check_mode_consistency(ic, ic->ic_curmode, &ic->ic_channels[i])) {
           if(ic->ic_channels[i].ic_ieee == pchannel->ic_ieee) {
               break;
           }
        }
    }   

    target_channel = i + 1;
    if (target_channel >= ic->ic_nchans) {
        target_channel = 0;
        startfromhead = 1;
    }
    
    for (; target_channel < ic->ic_nchans; target_channel++) {
        if(!ieee80211_check_mode_consistency(ic, ic->ic_curmode, &ic->ic_channels[target_channel]) &&
           ieee80211_checkDFS_free(vap, &ic->ic_channels[target_channel])){
            foundfitchan = 1;
            ptarget_channel = &ic->ic_channels[target_channel];
            break;
        } else if ((target_channel >= ic->ic_nchans - 1) && !startfromhead) { 
            /* if we could not find next in the trail. restart from head once */
            target_channel = 0;
            startfromhead = 1;
        } else if (ic->ic_channels[target_channel].ic_ieee == pchannel->ic_ieee &&
                   ic->ic_channels[target_channel].ic_flags == pchannel->ic_flags &&
                   startfromhead) { 
            /* we already restart to find channel from head but could not find a proper one , just jump to a random one  */
            target_channel = ieee80211_random_channel(ic);
            if ( target_channel != -1) {
                ptarget_channel = &ic->ic_channels[target_channel];
            } else {
                ptarget_channel = pchannel;
            } 
            break;
        }
    }

	return ptarget_channel;
}
#endif /* ATH_SUPPORT_IBSS_DFS */

/*
 * Execute radar channel change. This is called when a radar/dfs
 * signal is detected.  AP mode only.  Return 1 on success, 0 on
 * failure
 */
int
ieee80211_dfs_action(struct ieee80211vap *vap, struct ieee80211_channelswitch_ie *pcsaie)
{
    struct ieee80211com *ic = vap->iv_ic;
    osdev_t dev = ic->ic_osdev;
    int target_channel;
    struct ieee80211_channel *ptarget_channel = NULL;
#ifdef MAGPIE_HIF_GMAC
    struct ieee80211vap *tmp_vap = NULL;
#endif    
    if (vap->iv_opmode != IEEE80211_M_HOSTAP &&
        vap->iv_opmode != IEEE80211_M_IBSS) {
        return 0;
    }
    
#if ATH_SUPPORT_IBSS_DFS    
    if (vap->iv_opmode == IEEE80211_M_IBSS) {

        if (pcsaie) {
            ptarget_channel = ieee80211_doth_findchan(vap, pcsaie->newchannel);
        } else if(ic->ic_flags & IEEE80211_F_CHANSWITCH) {        
            ptarget_channel = ieee80211_doth_findchan(vap, ic->ic_chanchange_chan);
        } else { 
            ptarget_channel = (struct ieee80211_channel *)ieee80211_next_channel(vap, ic->ic_curchan);
        } 
    }
#endif /* ATH_SUPPORT_IBSS_DFS */

    if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
        target_channel = ieee80211_random_channel(ic);
        if ( target_channel != -1) {
            ptarget_channel = &ic->ic_channels[target_channel];
        } else {
            ptarget_channel = NULL;
        }
    }

    /* If we do have a scan entry, make sure its not an excluded 11D channel.
       See bug 31246 */
    /* No channel was found via scan module, means no good scanlist
       was found */

        if (ptarget_channel)
		{
		    if (vap->iv_state_info.iv_state == IEEE80211_S_RUN)
            {
                if_printf(dev,"Changing to channel %d (%d MHz) chanStart %d\n",
                        ptarget_channel->ic_ieee,
                        ptarget_channel->ic_freq);
                if (pcsaie) {
                    ic->ic_chanchange_chan = pcsaie->newchannel;
                    ic->ic_chanchange_tbtt = pcsaie->tbttcount;                
                } else {
                    ic->ic_chanchange_chan = ptarget_channel->ic_ieee;
                    ic->ic_chanchange_tbtt = IEEE80211_RADAR_11HCOUNT;
                }
#ifdef MAGPIE_HIF_GMAC
                TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
                    ic->ic_chanchange_cnt += ic->ic_chanchange_tbtt;
                }    
#endif
                ic->ic_flags |= IEEE80211_F_CHANSWITCH;

#if ATH_SUPPORT_IBSS_DFS
            if (vap->iv_opmode == IEEE80211_M_IBSS) {
                struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
                struct ieee80211_action_mgt_args *actionargs; 
                /* overwirte with our own value if we are DFS owner */
                if ( pcsaie == NULL &&
                    vap->iv_ibssdfs_state == IEEE80211_IBSSDFS_OWNER) {
                    ic->ic_chanchange_tbtt = vap->iv_ibss_dfs_csa_threshold;
                } 
                
                vap->iv_ibssdfs_state = IEEE80211_IBSSDFS_CHANNEL_SWITCH;
                
                if(vap->iv_csa_action_count_per_tbtt > vap->iv_ibss_dfs_csa_measrep_limit) {
                    return 1;
                }
                vap->iv_csa_action_count_per_tbtt++;

                actionargs = OS_MALLOC(vap->iv_ic->ic_osdev, sizeof(struct ieee80211_action_mgt_args) , GFP_KERNEL);
                if (actionargs == NULL) {
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s: Unable to alloc arg buf. Size=%d\n",
                                                __func__, sizeof(struct ieee80211_action_mgt_args));
                    return 0;
                }
                OS_MEMZERO(actionargs, sizeof(struct ieee80211_action_mgt_args));
                ieee80211_ic_doth_set(ic);
                /* use beacon_update function to do real channel switch */
                scn->sc_ops->ath_ibss_beacon_update_start(scn->sc_dev);

                actionargs->category = IEEE80211_ACTION_CAT_SPECTRUM;
                actionargs->action   = IEEE80211_ACTION_CHAN_SWITCH;
                actionargs->arg1     = 1;   /* mode? no use for now */
                actionargs->arg2     = ic->ic_chanchange_chan;
                if (vap->iv_ibssdfs_state == IEEE80211_IBSSDFS_CHANNEL_SWITCH) {
                    actionargs->arg3 = ic->ic_chanchange_tbtt - vap->iv_chanchange_count;
                } else {
                    actionargs->arg3 = ic->ic_chanchange_tbtt;
                }
                ieee80211_send_action(vap->iv_bss, actionargs, NULL);
                OS_FREE(actionargs);
                
            }
#endif /* ATH_SUPPORT_IBSS_DFS */
            }
            else
			{
			    /*
			     * vap is not in run  state yet. so
			     * change the channel here.
			     */
#ifdef MAGPIE_HIF_GMAC
                ic->ic_chanchange_chan = ptarget_channel->ic_ieee;
#endif                
                change_channel(ic,ptarget_channel);
 
                            /* update the bss channel of all the vaps */
                            wlan_iterate_vap_list(ic, ieee80211_vap_iter_update_bss_chan, ptarget_channel);
 
			}
		}
	    else
		{
		    /* should never come here? */
		    if_printf(dev,"Cannot change to any channel\n");
		    return 0;
		}
    return 1;
}

/*
 * Fetch a mute test channel which matches the operational mode/flags of the
 * current channel.
 *
 * Simply returning '36' when the AP is in HT40D mode will fail; the channel
 * lookup will be done with the channel flags requiring HT40D and said lookup
 * won't find a channel.
 *
 * XXX TODO: figure out the correct mute channel to return for VHT operation.
 *   It may be that we instead have to return the actual full VHT channel
 *   configuration (freq1, freq2, legacy ctl/ext info) when it's time to do
 *   this.
 */
static int
ieee80211_get_test_mute_chan(struct ieee80211com *ic,
  const struct ieee80211_channel *chan)
{
    if (chan == NULL)
        return IEEE80211_RADAR_TEST_MUTE_CHAN_11A;

    if (IEEE80211_IS_CHAN_VHT(chan))
        adf_os_print("%s: VHT not yet supported here (please fix); "
          "freq=%d, flags=0x%08x, "
          "falling through\n",
          __func__, chan->ic_freq, chan->ic_flags);

    if (IEEE80211_IS_CHAN_11N_HT40MINUS(chan))
        return IEEE80211_RADAR_TEST_MUTE_CHAN_11NHT40D;
    else if (IEEE80211_IS_CHAN_11N_HT40PLUS(chan))
        return IEEE80211_RADAR_TEST_MUTE_CHAN_11NHT40U;
    else if (IEEE80211_IS_CHAN_11N_HT20(chan))
        return IEEE80211_RADAR_TEST_MUTE_CHAN_11NHT20;
    else if (IEEE80211_IS_CHAN_A(chan))
        return IEEE80211_RADAR_TEST_MUTE_CHAN_11A;
    else {
        adf_os_print("%s: unknown channel mode, freq=%d, flags=0x%08x\n",
          __func__, chan->ic_freq, chan->ic_flags);
        return IEEE80211_RADAR_TEST_MUTE_CHAN_11A;
    }
}

void
ieee80211_mark_dfs(struct ieee80211com *ic, struct ieee80211_channel *ichan)
  {
     struct ieee80211_channel *c=NULL;
     struct ieee80211vap *vap;
     int i;
#ifdef MAGPIE_HIF_GMAC
     struct ieee80211vap* tmp_vap = NULL;
#endif     

     if (ic->ic_opmode == IEEE80211_M_HOSTAP ||
         ic->ic_opmode == IEEE80211_M_IBSS)
     {
         ieee80211_dfs_cac_cancel(ic);
         /* Mark the channel in the ic_chan list */
         /*
          * XXX TODO: this isn't exactly correct.
          * Specifically - it only marks the channels that match
          * the given centre frequency as having DFS, rather than
          * actually checking for channel overlap.  So it's not
          * entirely correct behaviour for VHT or HT40 operation.
          *
          * In any case, it should eventually be taught to just
          * use the channel centre and channel overlap code
          * that now exists in umac/base/ieee80211_channel.c
          * and flag them all.
          *
          * In actual reality, this also gets set correctly by
          * the NOL dfs channel list update method.
          */
         if ((ic->ic_flags_ext & IEEE80211_FEXT_MARKDFS) &&
             (ic->ic_dfs_usenol(ic) == 1))
         {
             for (i=0;i<ic->ic_nchans; i++)
             {
                 c = &ic->ic_channels[i];
                 if (c->ic_freq != ichan->ic_freq)
                     continue;
                 c->ic_flags |= IEEE80211_CHAN_RADAR;
             }

             c = ieee80211_find_channel(ic, ichan->ic_freq, ichan->ic_flags);

             if (c == NULL)
             {
                 return;
             }

             /*
              * If the reported event is on the current channel centre
              * frequency, begin the process of moving to another
              * channel.
              *
              * Unfortunately for now, this is not entirely correct -
              * If we report a radar event on a subchannel of the current
              * channel, this test will fail and we'll end up not
              * starting a CSA.
              *
              * XXX TODO: this API needs to change to take a radar event
              * frequency/width, instead of a channel.  It's then up
              * to the alternative umac implementations to write glue
              * to correctly handle things.
              */
             if  (ic->ic_curchan->ic_freq == c->ic_freq)
             {
                 /* get an AP vap */
                 vap = TAILQ_FIRST(&ic->ic_vaps);
                 while ((vap != NULL) && (vap->iv_state_info.iv_state != IEEE80211_S_RUN)  &&
                     (vap->iv_ic != ic))
                 {
                     vap = TAILQ_NEXT(vap, iv_next);
                 }
                 if (vap == NULL)
                 {
                     /*
                      * No running VAP was found, check
                      * any one is scanning.
                      */
                     vap = TAILQ_FIRST(&ic->ic_vaps);
                     while ((vap != NULL) && (vap->iv_ic != ic) && 
                            (vap->iv_state_info.iv_state != IEEE80211_S_SCAN))
                     {
                         vap = TAILQ_NEXT(vap, iv_next);
                     }
                     /*
                     * No running/scanning VAP was found, so they're all in
                     * INIT state, no channel change needed
                     */
                     if(!vap) return;
                     /* is it really Scanning */
                     /* XXX race condition ?? */
                     if(ic->ic_flags & IEEE80211_F_SCAN) return;
                     /* it is not scanning , but waiting for ath driver to move he vap to RUN */
                 }
#if ATH_SUPPORT_IBSS_DFS
               /* call the dfs action */
               if (vap->iv_opmode == IEEE80211_M_IBSS) {
                    u_int8_t index;
                    /* mark ibss dfs element, only support radar for now */
                    for (index = (vap->iv_ibssdfs_ie_data.len - IBSS_DFS_ZERO_MAP_SIZE)/sizeof(struct channel_map_field); index >0; index--) {
                        if (vap->iv_ibssdfs_ie_data.ch_map_list[index-1].ch_num == ichan->ic_ieee) {
                            vap->iv_ibssdfs_ie_data.ch_map_list[index-1].ch_map.radar = 1;
                            vap->iv_ibssdfs_ie_data.ch_map_list[index-1].ch_map.unmeasured = 0;
                            break;
                        }
                    }
                    
                   if(IEEE80211_ADDR_EQ(vap->iv_myaddr, vap->iv_ibssdfs_ie_data.owner)) {
                        ieee80211_dfs_action(vap, NULL);
                   } else {
                        /* send out measurement report in this case */
                        ieee80211_measurement_report_action(vap, NULL);
                        if (vap->iv_ibssdfs_state == IEEE80211_IBSSDFS_JOINER) {
                            vap->iv_ibssdfs_state = IEEE80211_IBSSDFS_WAIT_RECOVERY;
                        }
                   } 
               }
#endif /* ATH_SUPPORT_IBSS_DFS */
                if (vap->iv_opmode == IEEE80211_M_HOSTAP)
                    ieee80211_dfs_action(vap, NULL);
             }
           else
           {
           }
         }
         else
         {
             /* Change to a radar free 11a channel for dfstesttime seconds */
             ic->ic_chanchange_chan = ieee80211_get_test_mute_chan(ic,
               ic->ic_curchan);
             ic->ic_chanchange_tbtt = IEEE80211_RADAR_11HCOUNT;
#ifdef MAGPIE_HIF_GMAC             
             TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
                 ic->ic_chanchange_cnt += ic->ic_chanchange_tbtt;
             }
#endif             
             ic->ic_flags |= IEEE80211_F_CHANSWITCH;
             /* A timer is setup in the radar task if markdfs is not set and
              * we are in hostap mode.
              */
         }
     }
     else
     {
         /* Are we in sta mode? If so, send an action msg to ap saying we found  radar? */
     }
 }
#if ATH_SUPPORT_IBSS_DFS
void ieee80211_ibss_beacon_update_start(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    scn->sc_ops->ath_ibss_beacon_update_start(scn->sc_dev);
}

void ieee80211_ibss_beacon_update_stop(struct ieee80211com *ic)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    scn->sc_ops->ath_ibss_beacon_update_stop(scn->sc_dev);
}

/*
 * Build the ibss DFS element.
 */
u_int
ieee80211_create_dfs_channel_list(struct ieee80211vap *vap, struct channel_map_field *ch_map_list)
{
    u_int i, dfs_ch_count = 0;
    struct ieee80211com *ic = vap->iv_ic;

    for (i = 0; i < ic->ic_nchans; i++) {
        if((ic->ic_channels[i].ic_flagext & IEEE80211_CHAN_DFS) &&
            !ieee80211_check_mode_consistency(ic, ic->ic_curmode, &ic->ic_channels[i]))
        {
            ch_map_list[dfs_ch_count].ch_num = ic->ic_channels[i].ic_ieee;
            ch_map_list[dfs_ch_count].ch_map.bss = 0;
            ch_map_list[dfs_ch_count].ch_map.ofdem_preamble = 0;
            ch_map_list[dfs_ch_count].ch_map.und_signal = 0;
            ch_map_list[dfs_ch_count].ch_map.radar = 0;
            ch_map_list[dfs_ch_count].ch_map.unmeasured = 1;
            ch_map_list[dfs_ch_count].ch_map.reserved = 0;
            if (ic->ic_channels[i].ic_flags & IEEE80211_CHAN_RADAR) {
                ch_map_list[dfs_ch_count].ch_map.unmeasured = 0;
                ch_map_list[dfs_ch_count].ch_map.radar = 1;
            } else if (ic->ic_channels[i].ic_flagext & IEEE80211_CHAN_DFS_CLEAR) {
                ch_map_list[dfs_ch_count].ch_map.unmeasured = 0;
            }
            dfs_ch_count ++;
        }
    }   
    return dfs_ch_count;
}

/*
* initialize a IBSS dfs ie
*/
void
ieee80211_build_ibss_dfs_ie(struct ieee80211vap *vap)
{
    u_int ch_count;
    if (vap->iv_ibss_dfs_enter_recovery_threshold_in_tbtt == 0) {
        vap->iv_ibss_dfs_enter_recovery_threshold_in_tbtt = INIT_IBSS_DFS_OWNER_RECOVERY_TIME_IN_TBTT;
        vap->iv_ibss_dfs_csa_measrep_limit = DEFAULT_MAX_CSA_MEASREP_ACTION_PER_TBTT;
        vap->iv_ibss_dfs_csa_threshold     = IEEE80211_RADAR_11HCOUNT;
    }
    OS_MEMZERO(vap->iv_ibssdfs_ie_data.ch_map_list, sizeof(struct channel_map_field) * (IEEE80211_CHAN_MAX + 1));  
    vap->iv_ibssdfs_ie_data.ie = IEEE80211_ELEMID_IBSSDFS;
    vap->iv_ibssdfs_ie_data.rec_interval = vap->iv_ibss_dfs_enter_recovery_threshold_in_tbtt;
    ch_count = ieee80211_create_dfs_channel_list(vap, vap->iv_ibssdfs_ie_data.ch_map_list);
    vap->iv_ibssdfs_ie_data.len = IBSS_DFS_ZERO_MAP_SIZE + (sizeof(struct channel_map_field) * ch_count);
    vap->iv_measrep_action_count_per_tbtt = 0;
    vap->iv_csa_action_count_per_tbtt = 0;
    vap->iv_ibssdfs_recovery_count = vap->iv_ibss_dfs_enter_recovery_threshold_in_tbtt;
}

#endif /* ATH_SUPPORT_IBSS_DFS */
#else

void
ieee80211_mark_dfs(struct ieee80211com *ic, struct ieee80211_channel *ichan)
{
    return;
}

#if ATH_SUPPORT_IBSS_DFS
void ieee80211_ibss_beacon_update_start(struct ieee80211com *ic)
{
    return;
}

void ieee80211_ibss_beacon_update_stop(struct ieee80211com *ic)
{
    return;
}
#endif /* ATH_SUPPORT_IBSS_DFS */

#endif    // UMAC_SUPPORT_DFS
