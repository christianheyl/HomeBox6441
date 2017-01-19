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


#ifndef	__OL_IF_STATS_H
#define	__OL_IF_STATS_H




int
wmi_unified_wlan_profile_data_event_handler (ol_scn_t scn, u_int8_t *data, u_int16_t datalen, void *context);

void
ol_ath_chan_info_attach(struct ieee80211com *ic);

void
ol_ath_chan_info_detach(struct ieee80211com *ic);

void
ol_ath_stats_attach(struct ieee80211com *ic);

void
ol_ath_stats_detach(struct ieee80211com *ic);

void 
ol_get_wal_dbg_stats(struct ol_ath_softc_net80211 *scn,
                            struct wal_dbg_stats *dbg_stats);

#endif /* OL_IF_STATS_H */

