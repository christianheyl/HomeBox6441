/*
 * Copyright (c) 2011, Atheros Communications Inc.
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
 */

/*
 * LMAC offload interface functions for UMAC - for power and performance offload model
 */

#if ATH_SUPPORT_DFS

#include "ol_if_athvar.h"

#include "adf_os_mem.h"   /* adf_os_mem_alloc,free */
#include "adf_os_lock.h"  /* adf_os_spinlock_* */
#include "adf_os_types.h" /* adf_os_vprint */

#include "ath_dfs_structs.h"
#include "dfs_interface.h"

#include "ol_if_dfs.h"

#include "ol_regdomain.h"

#if ATH_PERF_PWR_OFFLOAD

/*
 * ic_dfs_attach - called by lmac/dfs/ to fetch the configuration
 * parameters.
 */
static int
ol_if_dfs_attach(struct ieee80211com *ic, void *ptr, void *radar_info)
{
	struct ath_dfs_caps *pCap = (struct ath_dfs_caps *) ptr;

	adf_os_print("%s: called; ptr=%p, radar_info=%p\n",
	    __func__,
	    ptr,
	    radar_info);

	pCap->ath_chip_is_bb_tlv = 1;

	pCap->ath_dfs_combined_rssi_ok = 0;
	pCap->ath_dfs_ext_chan_ok = 0;
	pCap->ath_dfs_use_enhancement = 0;
	pCap->ath_strong_signal_diversiry = 0; /* XXX? */
	pCap->ath_fastdiv_val = 0; /* XXX? */

	/* XXX radar_info? */

	return(0);
}

/*
 * ic_dfs_detach - called by lmac/dfs to detach (?)
 */
static int
ol_if_dfs_detach(struct ieee80211com *ic)
{

	adf_os_print("%s: called\n", __func__);
	return (0);	/* XXX error? */
}

/*
 * ic_dfs_attached
 */
static int
ol_if_dfs_attached(struct ieee80211com *ic)
{

#if	ATH_SUPPORT_DFS
	if (ic->ic_dfs) {
		return (1);
	} else {
		/* XXX why is it called here? */
		dfs_attach(ic);
		return (1);
	}
#else
	return (0);
#endif	/* ATH_SUPPORT_DFS */
}

/*
 * ic_dfs_enable - enable DFS
 *
 * For offload solutions, radar PHY errors will be enabled by the target
 * firmware when DFS is requested for the current channel.
 */
static int
ol_if_dfs_enable(struct ieee80211com *ic, int *is_fastclk, void *pe)
{

	adf_os_print("%s: called\n", __func__);

	/*
	 * XXX For peregrine, treat fastclk as the "oversampling" mode.
	 *     It's on by default.  This may change at some point, so
	 *     we should really query the firmware to find out what
	 *     the current configuration is.
	 */
	(* is_fastclk) = 1;

//	ol_ath_phyerr_enable(ic);
      //ieee80211_dfs_cac_start(ic);
	return (0);
}

/*
 * ic_dfs_disable
 */
static int
ol_if_dfs_disable(struct ieee80211com *ic)
{

	adf_os_print("%s: called\n", __func__);

	//ol_ath_phyerr_disable(ic);
	//dfs_radar_disable(ic);
	return (0);
}

/*
 * ic_dfs_get_thresholds
 */
static int
ol_if_dfs_get_thresholds(struct ieee80211com *ic,
    void *pe)
{
	/*
	 * XXX for now, the hardware has no API for fetching
	 * the radar parameters.
	 */
	struct ath_dfs_phyerr_param *param = (struct ath_dfs_phyerr_param *)pe;
	OS_MEMZERO(param, sizeof(*param));
	return(0);
}

/*
 * ic_get_ext_busy
 */
static int
ol_if_dfs_get_ext_busy(struct ieee80211com *ic)
{

	return (0);	/* XXX */
}

/*
 * ic_get_mib_cycle_counts_pct
 */
static int
ol_if_dfs_get_mib_cycle_counts_pct(struct ieee80211com *ic,
    u_int32_t *rxc_pcnt, u_int32_t *rxf_pcnt, u_int32_t *txf_pcnt)
{

	return (0);
}

/*
 * ic_dfs_control - dfs_control
 */

/*
 * ic_dfs_notify_radar - ieee80211_mark_dfs
 */

/*
 * ic_find_channel - ieee80211_find_channel
 */

/*
 * ic_dfs_unmark_radar - ieee80211_unmark_radar
 */

/*
 * (ic_get_ctl_by_country ?)
 */
#if 0
static u_int8_t
ol_if_get_ctl_by_country(struct ieee80211com *ic, u_int8_t *country,
    bool is2G)
{
	struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);

	return (scn->sc_ops->get_ctl_by_country(scn->sc_dev, country, is2G));
}
#endif

/*
 * ic_dfs_isdfsregdomain
 */
static u_int16_t
ol_if_dfs_isdfsregdomain(struct ieee80211com *ic)
{
#if	ATH_SUPPORT_DFS
	return (dfs_isdfsregdomain(ic));
#else
	return (0);
#endif	/* ATH_SUPPORT_DFS */
}

static u_int16_t
ol_if_dfs_usenol(struct ieee80211com *ic)
{
#if	ATH_SUPPORT_DFS
	return(dfs_usenol(ic));
#else
	return (0);
#endif	/* ATH_SUPPORT_DFS */
}

/*
 * XXX this doesn't belong here, but the DFS code requires that it exists.
 * Please figure out how to fix this!
 */
static u_int64_t
ol_if_get_tsf64(struct ieee80211com *ic)
{
	/* XXX TBD */
	return (0);
}


/*
 * Update the channel list with the given NOL list.
 *
 * Since the NOL can contain entries which may match multiple
 * channels, we need to walk the channel list to ensure that
 * we correctly update _all_ channel entries in question.
 */
static void
ol_if_dfs_clist_update(struct ieee80211com *ic, int cmd,
    struct dfs_nol_chan_entry *nollist, int nentries)
{
	struct ieee80211_channel *ichan;
	int i, j;
	int nol_found = 0;

	adf_os_print("%s: called, cmd=%d, nollist=%p, nentries=%d\n",
	    __func__, cmd, nollist, nentries);

	/* XXX for now, only handle DFS_NOL_CLIST_CMD_UPDATE. */
	if (cmd != DFS_NOL_CLIST_CMD_UPDATE) {
		adf_os_print("%s: cmd=%d, not handled!\n", __func__, cmd);
		return;
	}

	ieee80211_enumerate_channels(ichan, ic, i) {
		/* XXX break into a shared function */
		nol_found = 0;
		for (j = 0; j < nentries; j++) {
			if (ieee80211_check_channel_overlap(ic, ichan,
			    nollist[j].nol_chfreq, nollist[j].nol_chwidth)) {
				nol_found = 1;
				/*
				 * XXX break here for now; but later on when
				 * we're keeping a NOL timer per umac channel,
				 * we'll want to walk _all_ the NOL entries
				 * to find the maximum NOL time we need; then
				 * potentially update the NOL time for that
				 * umac channel.
				 */
				break;
			}
		}

		/*
		 * Dump out state transitions for now!
		 */
		if (nol_found && (! IEEE80211_IS_CHAN_RADAR(ichan)))
			adf_os_print("%s: radar found = ichan=%p, freq=%d, "
			    "vht freq1=%d, flags=0x%x\n",
			    __func__,
			    ichan,
			    ichan->ic_freq,
			    ieee80211_ieee2mhz(ichan->ic_vhtop_ch_freq_seg1,
			        ichan->ic_flags),
			    ichan->ic_flags);
		else if ((! nol_found) && IEEE80211_IS_CHAN_RADAR(ichan))
			adf_os_print("%s: NOL clear = ichan=%p, freq=%d, "
			    "vht freq1=%d, flags=0x%x\n",
			    __func__,
			    ichan,
			    ichan->ic_freq,
			    ieee80211_ieee2mhz(ichan->ic_vhtop_ch_freq_seg1,
			        ichan->ic_flags),
			    ichan->ic_flags);

		/*
		 * Now that we know whether there's a NOL entry overlapping
		 * this channel, let's set or clear the bits appropriately.
		 */
		if (nol_found)
			IEEE80211_CHAN_SET_RADAR(ichan);
		else
			IEEE80211_CHAN_CLR_RADAR(ichan);
	}
}

/*
 * ol_if_dfs_attach() - attach DFS methods to the umac state.
 */
void
ol_if_dfs_setup(struct ieee80211com *ic)
{

	adf_os_print("%s: called \n", __func__);

	/* DFS pattern matching hooks */
	ic->ic_dfs_attach = ol_if_dfs_attach;
	ic->ic_dfs_detach = ol_if_dfs_detach;
	ic->ic_dfs_attached = ol_if_dfs_attached;
	ic->ic_dfs_enable = ol_if_dfs_enable;
	ic->ic_dfs_disable = ol_if_dfs_disable;
	ic->ic_dfs_get_thresholds = ol_if_dfs_get_thresholds;
	ic->ic_dfs_control = dfs_control;	/* Direct ref to module */
	//ic->ic_get_ctl_by_country = ol_if_get_ctl_by_country;

	/* NOL related hooks */
	ic->ic_dfs_isdfsregdomain = ol_if_dfs_isdfsregdomain;
	ic->ic_dfs_usenol = ol_if_dfs_usenol;
	ic->ic_dfs_clist_update = ol_if_dfs_clist_update;

	/*
	 * Hooks from lmac/dfs/ back -into- the umac and shared DFS code
	 * (currently umac/if_lmac/if_ath_dfs.c)
	 */
	ic->ic_find_channel = ieee80211_find_channel;
	ic->ic_ieee2mhz = ieee80211_ieee2mhz;
	ic->ic_dfs_notify_radar = ieee80211_mark_dfs;
	ic->ic_dfs_unmark_radar = ieee80211_unmark_radar;
	ic->ic_start_csa = ieee80211_start_csa;

	/* Hardware facing hooks */
	ic->ic_get_ext_busy = ol_if_dfs_get_ext_busy;
	ic->ic_get_mib_cycle_counts_pct = ol_if_dfs_get_mib_cycle_counts_pct;
	ic->ic_get_TSF64 = ol_if_get_tsf64;

	/* Initial DFS pattern matching attach */
	dfs_attach(ic);
}

void
ol_if_dfs_teardown(struct ieee80211com *ic)
{
      adf_os_print("%s: called \n", __func__);
	dfs_detach(ic);
}

/* The following are for FCC Bin 1-4 pulses */
struct dfs_pulse dfs_fcc_radars[] = {
    // FCC TYPE 1
    // {18,  1,  325, 1930, 0,  6,  7,  0,  1, 18,  0, 3,  0}, // 518 to 3066
    {18,  1,  700, 700, 0,  6,  5,  0,  1, 18,  0, 3,  0},
    {18,  1,  350, 350, 0,  6,  5,  0,  1, 18,  0, 3,  0},

    // FCC TYPE 6
    // {9,   1, 3003, 3003, 1,  7,  5,  0,  1, 18,  0, 0,  1}, // 333 +/- 7 us
    {9,   1, 3003, 3003, 1,  7,  5,  0,  1, 18,  0, 0,  1},

    // FCC TYPE 2
    {23, 5, 4347, 6666, 0, 18, 11,  0,  7, 22,  0, 3,  2},

    // FCC TYPE 3
    {18, 10, 2000, 5000, 0, 23,  8,  6, 13, 22,  0, 3, 5},

    // FCC TYPE 4
    {16, 15, 2000, 5000, 0, 25,  7, 11, 23, 22,  0, 3, 11},
};

struct dfs_pulse dfs_mkk4_radars[] = {
    /* following two filters are specific to Japan/MKK4 */
//    {18,  1,  720,  720, 1,  6,  6,  0,  1, 18,  0, 3, 17}, // 1389 +/- 6 us
//    {18,  4,  250,  250, 1, 10,  5,  1,  6, 18,  0, 3, 18}, // 4000 +/- 6 us
//    {18,  5,  260,  260, 1, 10,  6,  1,  6, 18,  0, 3, 19}, // 3846 +/- 7 us
    {18,  1,  720,  720, 0,  6,  6,  0,  1, 18,  0, 3, 17}, // 1389 +/- 6 us
    {18,  4,  250,  250, 0, 10,  5,  1,  6, 18,  0, 3, 18}, // 4000 +/- 6 us
    {18,  5,  260,  260, 0, 10,  6,  1,  6, 18,  0, 3, 19}, // 3846 +/- 7 us

    /* following filters are common to both FCC and JAPAN */

    // FCC TYPE 1
    // {18,  1,  325, 1930, 0,  6,  7,  0,  1, 18,  0, 3,  0}, // 518 to 3066
    {18,  1,  700, 700, 0,  6,  5,  0,  1, 18,  0, 3,  0},
    {18,  1,  350, 350, 0,  6,  5,  0,  1, 18,  0, 3,  0},

    // FCC TYPE 6
    // {9,   1, 3003, 3003, 1,  7,  5,  0,  1, 18,  0, 0,  1}, // 333 +/- 7 us
    {9,   1, 3003, 3003, 1,  7,  5,  0,  1, 18,  0, 0,  1},

    // FCC TYPE 2
    {23, 5, 4347, 6666, 0, 18, 11,  0,  7, 22,  0, 3,  2},

    // FCC TYPE 3
    {18, 10, 2000, 5000, 0, 23,  8,  6, 13, 22,  0, 3, 5},

    // FCC TYPE 4
    {16, 15, 2000, 5000, 0, 25,  7, 11, 23, 22,  0, 3, 11},
};

struct dfs_bin5pulse dfs_fcc_bin5pulses[] = {
        {4, 28, 105, 12, 22, 5},
};

struct dfs_bin5pulse dfs_jpn_bin5pulses[] = {
        {5, 28, 105, 12, 22, 5},
};
struct dfs_pulse dfs_etsi_radars[] = {

    /* TYPE staggered pulse */
    /* 0.8-2us, 2-3 bursts,300-400 PRF, 10 pulses each */
    {30,  2,  300,  400, 2, 30,  3,  0,  5, 15, 0,   0, 1, 31},   /* Type 5*/
    /* 0.8-2us, 2-3 bursts, 400-1200 PRF, 15 pulses each */
    {30,  2,  400, 1200, 2, 30,  7,  0,  5, 15, 0,   0, 0, 32},   /* Type 6 */

    /* constant PRF based */
    /* 0.8-5us, 200  300 PRF, 10 pulses */
    {10, 5,   200,  400, 0, 24,  5,  0,  8, 15, 0,   0, 2, 33},   /* Type 1 */
    {10, 5,   400,  600, 0, 24,  5,  0,  8, 15, 0,   0, 2, 37},   /* Type 1 */
    {10, 5,   600,  800, 0, 24,  5,  0,  8, 15, 0,   0, 2, 38},   /* Type 1 */
    {10, 5,   800, 1000, 0, 24,  5,  0,  8, 15, 0,   0, 2, 39},   /* Type 1 */
//  {10, 5,   200, 1000, 0, 24,  5,  0,  8, 15, 0,   0, 2, 33},

    /* 0.8-15us, 200-1600 PRF, 15 pulses */
    {15, 15,  200, 1600, 0, 24, 8,  0, 18, 24, 0,   0, 0, 34},    /* Type 2 */

    /* 0.8-15us, 2300-4000 PRF, 25 pulses*/
    {25, 15, 2300, 4000,  0, 24, 10, 0, 18, 24, 0,   0, 0, 35},   /* Type 3 */

    /* 20-30us, 2000-4000 PRF, 20 pulses*/
    {20, 30, 2000, 4000, 0, 24, 6, 19, 33, 24, 0,   0, 0, 36},    /* Type 4 */
};

/*
 * This is called during a channel change or regulatory domain
 * reset; in order to fetch the new configuration information and
 * program the DFS pattern matching module.
 *
 * Eventually this should be split into "fetch config" (which can
 * happen at regdomain selection time) and "configure DFS" (which
 * can happen at channel config time) so as to minimise overheads
 * when doing channel changes.  However, this'll do for now.
 */
void
ol_if_dfs_configure(struct ieee80211com *ic)
{
	struct ath_dfs_radar_tab_info rinfo;
	int dfsdomain = DFS_FCC_DOMAIN;
	struct ol_ath_softc_net80211 *scn;
	struct ol_regdmn *ol_regdmn_handle;

	adf_os_print("%s: called\n", __func__);

	/* Fetch current radar patterns from the lmac */
	OS_MEMZERO(&rinfo, sizeof(rinfo));

	/*
	 * Look up the current DFS regulatory domain and decide
	 * which radar pulses to use.
	 */
	/* XXX TODO: turn this into a function! */
	scn = OL_ATH_SOFTC_NET80211(ic);
	ol_regdmn_handle = scn->ol_regdmn_handle;
	dfsdomain = ol_regdmn_handle->ol_regdmn_dfsDomain;


	switch (dfsdomain) {
		case DFS_FCC_DOMAIN:
			adf_os_print("%s: FCC domain\n", __func__);
			rinfo.dfsdomain = DFS_FCC_DOMAIN;
			rinfo.dfs_radars = dfs_fcc_radars;
			rinfo.numradars = ARRAY_LENGTH(dfs_fcc_radars);
			rinfo.b5pulses = dfs_fcc_bin5pulses;
			rinfo.numb5radars = ARRAY_LENGTH(dfs_fcc_bin5pulses);
			break;
		case DFS_ETSI_DOMAIN:
			adf_os_print("%s: ETSI domain\n", __func__);
			rinfo.dfsdomain = DFS_ETSI_DOMAIN;
			rinfo.dfs_radars = dfs_etsi_radars;
			rinfo.numradars = ARRAY_LENGTH(dfs_etsi_radars);
			rinfo.b5pulses = NULL;
			rinfo.numb5radars = 0;
			break;
		case DFS_MKK4_DOMAIN:
			adf_os_print("%s: MKK4 domain\n", __func__);
			rinfo.dfsdomain = DFS_MKK4_DOMAIN;
			rinfo.dfs_radars = dfs_mkk4_radars;
			rinfo.numradars = ARRAY_LENGTH(dfs_mkk4_radars);
			rinfo.b5pulses = dfs_jpn_bin5pulses;
			rinfo.numb5radars = ARRAY_LENGTH(dfs_jpn_bin5pulses);
			break;
		default:
			adf_os_print("%s: UNINIT domain\n", __func__);
			rinfo.dfsdomain = DFS_UNINIT_DOMAIN;
			rinfo.dfs_radars = NULL;
			rinfo.numradars = 0;
			rinfo.b5pulses = NULL;
			rinfo.numb5radars = 0;
			break;
	}

	/* XXX dfs_defaultparams */

	/*
	 * Set the regulatory domain, radar pulse table and enable
	 * radar events if required.
	 */
	dfs_radar_enable(ic, &rinfo);
}

#endif /* ATH_PERF_PWR_OFFLOAD */

#else /* ATH_SUPPORT_DFS */
void
ol_if_dfs_configure(struct ieee80211com *ic)
{
    return;
}

#endif /* ATH_SUPPORT_DFS */
