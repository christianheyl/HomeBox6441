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
 */

/*
 * Public Interface for ALD control module
 */

#ifndef _DEV_ATH_ALD_H
#define _DEV_ATH_ALD_H

#include <ath_dev.h>

#include "ath_desc.h"
#include "ah_desc.h"

/* defines */
/* structure in ath_softc to hold statistics */
struct ath_ald_record {
    u_int                   sc_ald_tbuf;
    u_int                   sc_ald_tnum;
    u_int32_t               sc_ald_cu;
    u_int32_t               sc_ald_cunum;
    
    u_int32_t               sc_ald_txairtime;
    u_int32_t               sc_ald_pktlen;
    u_int32_t               sc_ald_pktnum;
    u_int32_t               sc_ald_bfused;
    u_int32_t               sc_ald_counter;
};

#if ATH_SUPPORT_LINKDIAG_EXT

/* Collect per link stats */
int ath_ald_collect_ni_data(ath_dev_t dev, ath_node_t an, ath_ald_t ald_data, ath_ni_ald_t ald_ni_data); 
/* collect over all WLAN stats */
int ath_ald_collect_data(ath_dev_t dev, ath_ald_t ald_data);
/* update local stats after each unicast frame transmission */
void ath_ald_update_frame_stats(struct ath_softc *sc, struct ath_buf *bf, struct ath_tx_status *ts);
//void ath_ald_update_rate_stats(struct ath_softc *sc, struct ath_node *an, u_int8_t rix);

#else

#define ath_ald_collect_ni_data(a, b, c, d) do{}while(0)
#define ath_ald_collect_data(a, b) do{}while(0)
#define ath_ald_update_frame_stats(a, b, c)  do{}while(0)
//#define ath_ald_update_rate_stats(struct ath_softc *sc, struct ath_node *an, u_int8_t rix)

#endif  /* ATH_SUPPORT_LINKDIAG_EXT */

#endif  /* _DEV_ATH_ALD_H */
