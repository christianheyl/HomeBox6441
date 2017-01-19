#include <ieee80211_var.h>
#include <ieee80211_regdmn.h>

/*
 * Start the process of sending a CSA and doing a channel switch.
 *
 * By setting IEEE80211_F_CHANSWITCH, the beacon update code
 * will add a CSA IE.  Once the count reaches 0, the channel
 * switch will occur.
 */
void
ieee80211_start_csa(struct ieee80211com *ic, u_int8_t ieeeChan)
{
#ifdef MAGPIE_HIF_GMAC
	struct ieee80211vap *tmp_vap = NULL;
#endif

	IEEE80211_DPRINTF_IC(ic, IEEE80211_VERBOSE_LOUD, IEEE80211_MSG_DEBUG,
	    "Beginning CSA to channel %d\n", ieeeChan);

	ic->ic_chanchange_chan = ieeeChan;
	ic->ic_chanchange_tbtt = IEEE80211_RADAR_11HCOUNT;
#ifdef MAGPIE_HIF_GMAC
	TAILQ_FOREACH(tmp_vap, &ic->ic_vaps, iv_next) {
		ic->ic_chanchange_cnt += ic->ic_chanchange_tbtt;
	}
#endif
	ic->ic_flags |= IEEE80211_F_CHANSWITCH;
}
