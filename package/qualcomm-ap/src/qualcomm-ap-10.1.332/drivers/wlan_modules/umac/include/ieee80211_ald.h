/*
 *  Copyright (c) 2008 Atheros Communications Inc.
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

#ifndef _IEEE80211_ALD_H
#define _IEEE80211_ALD_H

typedef void * ald_netlink_t;

#if ATH_SUPPORT_LINKDIAG_EXT
struct ath_ni_linkdiag {          
    int32_t ald_airtime;
    int32_t ald_pktlen;
    int32_t ald_pktnum;
    int32_t ald_count;
    int32_t ald_aggr;
    int32_t ald_phyerr;
    int32_t ald_msdusize;
    u_int32_t ald_capacity;
    u_int32_t ald_lastper;
    u_int32_t ald_txrate;
    u_int32_t ald_max4msframelen;
    u_int32_t ald_txcount;
    u_int32_t ald_avgtxrate;
    u_int32_t ald_avgmax4msaggr;
    u_int32_t ald_txqdepth;
    spinlock_t ald_lock;
};

struct ath_linkdiag {
    u_int32_t ald_phyerr;
    u_int32_t ald_ostime;
    u_int32_t ald_maxcu;
    u_int32_t msdu_size;
    u_int32_t phyerr_rate;

    int32_t ald_unicast_tx_bytes;
    int32_t ald_unicast_tx_packets;
    
    u_int32_t ald_chan_util;
    u_int32_t ald_chan_capacity;
    u_int32_t ald_dev_load;
    u_int32_t ald_txbuf_used;
    u_int32_t ald_curThroughput;

    struct ald_stat_info *staticp;
    spinlock_t ald_lock;
};
#endif /* ATH_SUPPORT_LINKDIAG_EXT */

#if ATH_SUPPORT_LINKDIAG_EXT
/**
 * for upper layer to call and get WLAN statistics for each link 
 * @param      vaphandle   : vap handle
 * @param      param   : handle for the return value .    
 *
 * @return : 0  on success and -ve on failure.
 *
 */
int wlan_ald_get_statistics(wlan_if_t vaphandle, void *param);
/**
 * get WLAN statistics for each link by call lmac statistics functions
 * @param      vap   : vap handle
 */
void ath_net80211_ald_get_statistics(struct ieee80211vap *vap);
/**
 * attach ath link diag functionalities
 * @param      vap   : vap handle
 */
int ieee80211_ald_vattach(struct ieee80211vap *vap);
/**
 * deattach ath link diag functionalities
 * @param      vap   : vap handle
 */
int ieee80211_ald_vdetach(struct ieee80211vap *vap);
/**
 * collect unicast packet transmission stats and save it locally
 * @param      vap   : vap handle
 */
void ieee80211_ald_record_tx(struct ieee80211vap *vap, wbuf_t wbuf, int datalen);

#else /* ATH_SUPPORT_LINKDIAG_EXT */

#define wlan_ald_get_statistics(a, b)   do{}while(0)
#define ath_net80211_ald_get_statistics(a)  do{}while(0)
#define ieee80211_ald_vattach(a) do{}while(0)
#define ieee80211_ald_vdetach(b) do{}while(0)
#define ieee80211_ald_record_tx(a, b, c) do{}while(0)

#endif /* ATH_SUPPORT_LINKDIAG_EXT */

#endif
