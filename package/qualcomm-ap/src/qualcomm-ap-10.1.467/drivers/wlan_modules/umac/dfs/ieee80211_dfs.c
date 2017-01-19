/*-
 * Copyright (c) 2007-2008 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: src/sys/net80211/ieee80211_dfs.c,v 1.9 2011/11/08 04:00:24 adrian Exp $");
#endif

#include <ieee80211_var.h>
#include <ieee80211_regdmn.h>


#if ATH_SUPPORT_DFS
static	int ieee80211_nol_timeout = 30*60;		/* 30 minutes */
#define	NOL_TIMEOUT	(ieee80211_nol_timeout*1000)

static	int ieee80211_cac_timeout = 60;		/* 60 seconds */

/* CAC in weather channel is 600 sec. However there are times when we boot up 12 sec faster in weather channel */

static	int ieee80211_cac_weather_timeout = 612;	/* 10 minutes plus 12 sec */

static int
null_set_quiet(struct ieee80211_node *ni, u_int8_t *quiet_elm)
{
	return -1;
}

/*
 * Override the default CAC timeout.
 *
 * A cac_timeout of -1 means "don't override the default".
 *
 * This is primarily for debugging.
 */
int
ieee80211_dfs_override_cac_timeout(struct ieee80211com *ic, int cac_timeout)
{
	struct ieee80211_dfs_state *dfs = &ic->ic_dfs_state;

	if (dfs == NULL)
		return (-EIO);

	dfs->cac_timeout_override = cac_timeout;
	adf_os_print("%s: CAC timeout is now %s (%d)\n",
	    __func__,
	    (cac_timeout == -1) ? "default" : "overridden",
	    cac_timeout);

	return (0);
}

int
ieee80211_dfs_get_override_cac_timeout(struct ieee80211com *ic,
    int *cac_timeout)
{
	struct ieee80211_dfs_state *dfs = &ic->ic_dfs_state;

	if (dfs == NULL)
		return (-EIO);

	(*cac_timeout) = dfs->cac_timeout_override;

	return (0);
}

/*
 * Return the CAC timeout for the given channel.
 */
int
ieee80211_get_cac_timeout(struct ieee80211com *ic,
    struct ieee80211_channel *chan)
{
    struct ieee80211_dfs_state *dfs = &ic->ic_dfs_state;

    if (dfs == NULL)
    	return (0);

    if (dfs->cac_timeout_override != -1)
    	return (dfs->cac_timeout_override);

    if (ic->ic_get_dfsdomain(ic) == DFS_ETSI_DOMAIN) {
        struct ieee80211_channel_list chan_info;
        int i;
        ic->ic_get_ext_chan_info (ic, &chan_info);
        for (i = 0; i < chan_info.cl_nchans; i++) {    
            if(IEEE80211_IS_CHAN_WEATHER_RADAR(chan_info.cl_channels[i])) {
                return (ieee80211_cac_weather_timeout);
            }
        }
    }
    return (ieee80211_cac_timeout);
}

void
ieee80211_dfs_detach(struct ieee80211com *ic)
{
	/* NB: we assume no locking is needed */
	ieee80211_dfs_reset(ic);
}


/*
 * This function is supposed to be called only on detach and regulatory
 * domain changes.  It isn't a generic state machine change.
 * In particular, the CAC timer should be reset via another method.
 */
void
ieee80211_dfs_reset(struct ieee80211com *ic)
{
	struct ieee80211_dfs_state *dfs = &ic->ic_dfs_state;

	if(! dfs->enable)
		return;
	/* NB: we assume no locking is needed */
	/* NB: cac_timer should be cleared by the state machine */
	dfs->enable = 0;
	OS_CANCEL_TIMER(&dfs->cac_timer);
	/* XXX clear the NOL timer? */
#if 0 /* AP: Not needed */
	for (i = 0; i < ic->ic_nchans; i++)
		ic->ic_channels[i].ic_state = 0;
#endif    
	dfs->lastchan = NULL;
}
static void
ath_vap_iter_cac(void *arg, wlan_if_t vap)
{
      //struct ieee80211vap *vap = arg;
      struct ieee80211com *ic = vap->iv_ic;
	if (vap->iv_state_info.iv_state != IEEE80211_S_DFS_WAIT)
		return;
      /* Radar Detected */
	if (IEEE80211_IS_CHAN_RADAR(ic->ic_curchan)) {
        ieee80211_state_event(vap, IEEE80211_STATE_EVENT_UP);
    } else {
        ieee80211_state_event(vap, IEEE80211_STATE_EVENT_DFS_CLEAR);
    }
}

static void
ieee80211_dfs_proc_cac(struct ieee80211com *ic)
{

    wlan_iterate_vap_list(ic, ath_vap_iter_cac, NULL);
}

static
OS_TIMER_FUNC(cac_timeout)
{
	struct ieee80211com *ic;
	struct ieee80211_dfs_state *dfs;

	OS_GET_TIMER_ARG(ic, struct ieee80211com *);
	dfs = &ic->ic_dfs_state;
      dfs->cac_timer_running = 0;

	printk("%s cac expired, chan %d curr time %d\n",
	    __func__, ic->ic_curchan->ic_freq, 
	    (adf_os_ticks_to_msecs(adf_os_ticks()) / 1000));
	/*
	 * When radar is detected during a CAC we are woken
	 * up prematurely to switch to a new channel.
	 * Check the channel to decide how to act.
	 */
	if (IEEE80211_IS_CHAN_RADAR(ic->ic_curchan)) {
		ieee80211_mark_dfs(ic, ic->ic_curchan);

		IEEE80211_DPRINTF_IC(ic,
		    IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_DFS,
		    "CAC timer on channel %u (%u MHz) stopped due to radar\n",
		    ic->ic_curchan->ic_ieee, ic->ic_curchan->ic_freq);

	} else {
		IEEE80211_DPRINTF_IC(ic,
		    IEEE80211_VERBOSE_LOUD,IEEE80211_MSG_DFS,
		    "CAC timer on channel %u (%u MHz) expired; "
		    "no radar detected\n",
		    ic->ic_curchan->ic_ieee, ic->ic_curchan->ic_freq);
	}
	/*
	 * Iterate over the nodes, processing the CAC completion event.
	 */
	ieee80211_dfs_proc_cac(ic);
}

int
ieee80211_dfs_cac_cancel(struct ieee80211com *ic)
{

    struct ieee80211vap *vap;
    int set_dfs_wait_mesg = 0;

    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
	if (vap->iv_state_info.iv_state != IEEE80211_S_DFS_WAIT) {
		continue;
        }
        ieee80211_dfs_cac_stop(vap);
    }
    return 0;
}

static int
ieee80211_dfs_send_dfs_wait(struct ieee80211com *ic)
{

    struct ieee80211vap *vap;
    int set_dfs_wait_mesg = 0;

    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
        if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
            printk("%s[%d] NOT A HOSTAP VAP, skip DFS (iv_opmode %d, iv_bsschan %d,  ic_freq %d)\n", __func__, __LINE__,
            vap->iv_opmode, vap->iv_bsschan->ic_freq, ic->ic_curchan->ic_freq);
            continue;
        }
        if (vap->iv_bsschan != ic->ic_curchan) {
            printk("%s[%d] VAP chan mismatch vap %d ic %d\n", __func__, __LINE__,
            vap->iv_bsschan->ic_freq, ic->ic_curchan->ic_freq);
            continue;
        }
	/* Don't send DFS_WAIT event to Down VAPs */
        if (vap->iv_state_info.iv_state < IEEE80211_S_JOIN) {
            printk("%s[%d] DONT send DFS_WAIT event to vap state %d \n", __func__, __LINE__, vap->iv_state_info.iv_state);
            continue;
        }
        ieee80211_state_event(vap, IEEE80211_STATE_EVENT_DFS_WAIT);
        set_dfs_wait_mesg++;
    }
    return set_dfs_wait_mesg;
}

/*
 * Initiate the CAC timer.  The driver is responsible
 * for setting up the hardware to scan for radar on the
 * channnel, we just handle timing things out.
 */
int
ieee80211_dfs_cac_start(struct ieee80211com *ic)
{
	struct ieee80211_dfs_state *dfs = &ic->ic_dfs_state;

	if (!IEEE80211_IS_CHAN_DFS(ic->ic_curchan)) {
		return 1;
	}

	if (ic->ic_dfs_state.ignore_dfs) {
		printk("%s ignore dfs by user %d \n",
			__func__, ic->ic_dfs_state.ignore_dfs);
		return 1;
	}

       if (ic->ic_dfs_state.ignore_cac) {
	   printk("%s ignore CAC by user %d \n",
	    __func__, ic->ic_dfs_state.ignore_cac);
		return 1;
       }

	OS_CANCEL_TIMER(&dfs->cac_timer);
	if (ieee80211_dfs_send_dfs_wait(ic)) {
            printk("cac_start chan %d timeout %d sec, curr time: %d sec\n",
                ic->ic_curchan->ic_freq, 
                ieee80211_get_cac_timeout(ic, ic->ic_curchan), 
                (adf_os_ticks_to_msecs(adf_os_ticks()) / 1000));
            dfs->cac_timer_running = 1;
            OS_SET_TIMER(&dfs->cac_timer, ieee80211_get_cac_timeout(ic,
            ic->ic_curchan) * 1000);
	}
	return 0;
}

/*
 * Clear the CAC timer.
 */
void
ieee80211_dfs_cac_stop(struct ieee80211vap *vap)
{
	struct ieee80211com *ic = vap->iv_ic;
	struct ieee80211_dfs_state *dfs = &ic->ic_dfs_state;
       u_int32_t phyerr;
       ic->ic_dfs_debug(ic, 0, (void *)&phyerr);
	printk("%s[%d] Stopping CAC Timer %d procphyerr 0x%08x\n",
	    __func__, __LINE__, ic->ic_curchan->ic_freq, phyerr);

	OS_CANCEL_TIMER(&dfs->cac_timer);
      dfs->cac_timer_running = 0;
}

/*
 * Unmark all channels whose frequencies match 'chan'.
 *
 * This matches the same logic used by ieee80211_mark_dfs (in if_lmac)
 * and ieee80211_notify_radar (here) which only mark channels whose
 * frequencies match the given detection channel.
 *
 * This doesn't at all reset or clear NOL handling for the channel.
 * It just updates the channel flags.
 *
 * This may need adjusting for 802.11n HT/40 channels and 802.11ac channels.
 */
void
ieee80211_unmark_radar(struct ieee80211com *ic, struct ieee80211_channel *chan)
{
	struct ieee80211_channel *c;
	int i;

	for (i = 0; i < ic->ic_nchans; i++) {
		c = &ic->ic_channels[i];
		if (chan->ic_freq == c->ic_freq) {
			/* XXX are both of these correct? */
			c->ic_flagext &= ~IEEE80211_CHAN_RADAR_FOUND;
			c->ic_flags &= ~IEEE80211_CHAN_RADAR;
		}
	}
}

static
OS_TIMER_FUNC(nol_timeout)
{
	struct ieee80211com *ic ;
	struct ieee80211_dfs_state *dfs;
	struct ieee80211_channel *c;
	int i;
	unsigned long oldest, now;

//	IEEE80211_LOCK_ASSERT(ic);
    OS_GET_TIMER_ARG(ic, struct ieee80211com *);
    dfs = &ic->ic_dfs_state;

	now = oldest = adf_os_ticks();
	for (i = 0; i < ic->ic_nchans; i++) {
		c = &ic->ic_channels[i];
		if (IEEE80211_IS_CHAN_RADAR(c)) {
			if (adf_os_time_after_eq(now, dfs->nol_event[i]+NOL_TIMEOUT)) {
				c->ic_flagext &= ~IEEE80211_CHAN_RADAR_FOUND;
				if (c->ic_flags & IEEE80211_CHAN_RADAR) {
					/*
					 * NB: do this here so we get only one
					 * msg instead of one for every channel
					 * table entry.
					 */
					IEEE80211_DPRINTF_IC(ic,
					    IEEE80211_VERBOSE_LOUD,
					    IEEE80211_MSG_DFS,
					    "radar on channel"
					    " %u (%u MHz) cleared after "
					    "timeout\n",
					    c->ic_ieee, c->ic_freq);
				}
			} else if (dfs->nol_event[i] < oldest)
				oldest = dfs->nol_event[i];
		}
	}
	if (oldest != now) {
		/* arrange to process next channel up for a status change */
		//callout_schedule(&dfs->nol_timer, oldest + NOL_TIMEOUT - now);
		OS_SET_TIMER(&dfs->nol_timer,
		    NOL_TIMEOUT - adf_os_ticks_to_msecs(adf_os_ticks()));
	}
}

static void
announce_radar(struct ieee80211com *ic, const struct ieee80211_channel *curchan,
	const struct ieee80211_channel *newchan)
{
	if (newchan == NULL)
		IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD,
		    IEEE80211_MSG_DFS,
		    "radar detected on channel %u (%u MHz)\n",
		    curchan->ic_ieee, curchan->ic_freq);
	else
		IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD,
		    IEEE80211_MSG_DFS,
		    "radar detected on channel %u (%u MHz), "
		    "moving to channel %u (%u MHz)\n",
		    curchan->ic_ieee, curchan->ic_freq,
		    newchan->ic_ieee, newchan->ic_freq);
}

void
ieee80211_dfs_attach(struct ieee80211com *ic)
{
	struct ieee80211_dfs_state *dfs = &ic->ic_dfs_state;

	dfs->enable = 1;
	dfs->cac_timeout_override = -1;
	OS_INIT_TIMER(ic->ic_osdev, &(dfs->cac_timer), cac_timeout,
	    (void *) (ic));
	OS_INIT_TIMER(ic->ic_osdev, &(dfs->nol_timer), nol_timeout,
	    (void *) (ic));
	ic->ic_set_quiet = null_set_quiet;
}

/*
 * Handle a radar detection event on a channel. The channel is
 * added to the NOL list and we record the time of the event.
 * Entries are aged out after NOL_TIMEOUT.  If radar was
 * detected while doing CAC we force a state/channel change.
 * Otherwise radar triggers a channel switch using the CSA
 * mechanism (when the channel is the bss channel).
 */
void
ieee80211_dfs_notify_radar(struct ieee80211com *ic,
    struct ieee80211_channel *chan)
{
	struct ieee80211_dfs_state *dfs = &ic->ic_dfs_state;
	int i;
	unsigned long now;

	//IEEE80211_LOCK_ASSERT(ic);

	/*
	 * Mark all entries with this frequency.  Notify user
	 * space and arrange for notification when the radar
	 * indication is cleared.  Then kick the NOL processing
	 * thread if not already running.
	 */
	now = adf_os_ticks();
	for (i = 0; i < ic->ic_nchans; i++) {
		struct ieee80211_channel *c = &ic->ic_channels[i];
		if (c->ic_freq == chan->ic_freq) {
#if 0
			c->ic_state &= ~IEEE80211_CHANSTATE_CACDONE;
			c->ic_state |= IEEE80211_CHANSTATE_RADAR;
#endif
			/* XXX should I do this for all channels at that freq? */
			c->ic_flags |= IEEE80211_CHAN_RADAR;
			c->ic_flagext |= IEEE80211_CHAN_RADAR_FOUND;
			dfs->nol_event[i] = now;
		}
	}

#if 0
	ieee80211_notify_radar(ic, chan);
	chan->ic_state |= IEEE80211_CHANSTATE_NORADAR;
	if (!callout_pending(&dfs->nol_timer))
		callout_reset(&dfs->nol_timer, NOL_TIMEOUT, dfs_timeout, ic);
#endif
	/* Immediately do a NOL check */
	OS_CANCEL_TIMER(&dfs->nol_timer);
	OS_SET_TIMER(&dfs->nol_timer, 1);

#if 0
	/*
	 * If radar is detected on the bss channel while
	 * doing CAC; force a state change by scheduling the
	 * callout to be dispatched asap.  Otherwise, if this
	 * event is for the bss channel then we must quiet
	 * traffic and schedule a channel switch.
	 *
	 * Note this allows us to receive notification about
	 * channels other than the bss channel; not sure
	 * that can/will happen but it's simple to support.
	 */
	if (chan == ic->ic_bsschan) {
		/* XXX need a way to defer to user app */
		dfs->newchan = ieee80211_dfs_pickchannel(ic);

		announce_radar(ic->ic_ifp, chan, dfs->newchan);

		if (callout_pending(&dfs->cac_timer))
			callout_schedule(&dfs->cac_timer, 0);
		else if (dfs->newchan != NULL) {
			/* XXX mode 1, switch count 2 */
			/* XXX calculate switch count based on max
			  switch time and beacon interval? */
			ieee80211_csa_startswitch(ic, dfs->newchan, 1, 2);
		} else {
			/*
			 * Spec says to stop all transmissions and
			 * wait on the current channel for an entry
			 * on the NOL to expire.
			 */
			/*XXX*/
			IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD,IEEE80211_MSG_DFS, "%s: No free channels; waiting for entry "
			    "on NOL to expire\n", __func__);
		}
	} else {
		/*
		 * Issue rate-limited console msgs.
		 */
		if (dfs->lastchan != chan) {
			dfs->lastchan = chan;
			dfs->cureps = 0;
			announce_radar(ic->ic_ifp, chan, NULL);
		} else if (ppsratecheck(&dfs->lastevent, &dfs->cureps, 1)) {
			announce_radar(ic->ic_ifp, chan, NULL);
		}
	}
#endif
}

#else
int
ieee80211_dfs_cac_cancel(struct ieee80211com *ic)
{
	return 1; /* NON DFS mode */
}

int
ieee80211_dfs_cac_start(struct ieee80211com *ic)
{

	return 1; /* NON DFS mode */
}

void
ieee80211_dfs_cac_stop(struct ieee80211vap *vap)
{
}

void
ieee80211_unmark_radar(struct ieee80211com *ic, struct ieee80211_channel *chan)
{

	/* XXX nothing to do here */
}

#endif /* ATH_SUPPORT_DFS */
