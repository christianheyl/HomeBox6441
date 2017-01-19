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
 * 
 */

#include "ath_ald.h"
#include "ath_internal.h"

#include "if_athvar.h"
#include "ratectrl.h"
#include "ratectrl11n.h"

#define ALD_MAX_DEV_LOAD 300
#define ALD_MSDU_SIZE 1300
#define MAX_AGGR_LIMIT 32
#define DEFAULT_MSDU_SIZE 1000

#if ATH_SUPPORT_LINKDIAG_EXT

int ath_ald_collect_ni_data(ath_dev_t dev, ath_node_t an, ath_ald_t ald_data, ath_ni_ald_t ald_ni_data)
{
    u_int32_t ccf;
    u_int32_t myCcf = 0;
    u_int32_t aggrAverage = 0;
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);
    struct atheros_softc  *asc = (struct atheros_softc*)sc->sc_rc;
    struct ath_node *ant = ATH_NODE(an);
    struct atheros_node *pSib;
    TX_RATE_CTRL *pRc;
    const RATE_TABLE_11N  *pRateTable = (const RATE_TABLE_11N *)asc->hwRateTable[sc->sc_curmode];

    //an = ATH_NODE(ATH_NODE_NET80211(ni)->an_sta);
    pSib = ATH_NODE_ATHEROS(ant);
    pRc = (TX_RATE_CTRL *)(pSib);

    ald_ni_data->ald_lastper = pRc->state[pRc->lastRateIndex].per;

    if(pRc->TxRateCount != 0) {
        ald_ni_data->ald_avgtxrate = pRc->TxRateInMbps / pRc->TxRateCount;
        ald_ni_data->ald_avgmax4msaggr = pRc->Max4msFrameLen / (pRc->TxRateCount * ald_data->msdu_size);
    }
    
    pRc->TxRateInMbps = 0;
    pRc->Max4msFrameLen = 0;
    pRc->TxRateCount = 0;

    if (ald_ni_data->ald_avgmax4msaggr > 32)
        ald_ni_data->ald_avgmax4msaggr = 32;

    if(sc->sc_ald.sc_ald_txairtime != 0){
        //ccf = (ald_data->msdu_size/(sc->sc_ald.sc_ald_txairtime/ald_ni_data->ald_pktnum))*8;
        ccf = ald_ni_data->ald_avgtxrate;
        myCcf = sc->sc_ald.sc_ald_pktlen*8/sc->sc_ald.sc_ald_txairtime;
        aggrAverage = sc->sc_ald.sc_ald_pktnum/sc->sc_ald.sc_ald_counter;
    }
    else {
        if(ald_ni_data->ald_avgtxrate) {
            ccf = ald_ni_data->ald_avgtxrate;
            aggrAverage = ald_ni_data->ald_avgmax4msaggr/2;
        }
        else {
            ccf = pRateTable->info[INIT_RATE_MAX_40-4].rateKbps/1000;
            aggrAverage = MAX_AGGR_LIMIT/2;
        }
    }
    if (sc->sc_ald.sc_ald_counter) {
        ald_data->ald_txbuf_used = sc->sc_ald.sc_ald_bfused/sc->sc_ald.sc_ald_counter;
    } else {
        ald_data->ald_txbuf_used = 0;
    }
    ald_data->ald_curThroughput = sc->sc_ald.sc_ald_pktnum * ald_data->msdu_size * 8 / 1000000;
    if (ald_data->ald_curThroughput < 1) {
        aggrAverage = MAX_AGGR_LIMIT/2;
    }
    
    if (ccf != 0) {
        ald_ni_data->ald_capacity = ccf;
        ald_ni_data->ald_aggr = aggrAverage;
        ald_ni_data->ald_phyerr = ald_data->phyerr_rate;
        if (ald_data->msdu_size < DEFAULT_MSDU_SIZE) {
            ald_ni_data->ald_msdusize = ALD_MSDU_SIZE;
        } else {
            ald_ni_data->ald_msdusize = ald_data->msdu_size;
        }
    }
    sc->sc_ald.sc_ald_txairtime = 0;
    sc->sc_ald.sc_ald_pktnum = 0;
    sc->sc_ald.sc_ald_pktlen = 0;
    sc->sc_ald.sc_ald_bfused = 0;
    sc->sc_ald.sc_ald_counter = 0;
    ald_ni_data->ald_txrate = 0;
    ald_ni_data->ald_max4msframelen = 0;
    ald_ni_data->ald_txcount = 0;
    ald_ni_data->ald_avgtxrate = 0;
    ald_ni_data->ald_avgmax4msaggr = 0;
    return 0;
}

int ath_ald_collect_data(ath_dev_t dev, ath_ald_t ald_data)
{
    u_int32_t cu = 0; /* cu: channel utilization; cc: channel capacity */
    u_int32_t msdu_size=0, load=0;
    u_int32_t rxclear=0, rxframe=0, txframe=0;
    u_int32_t new_phyerr=0, new_ostime=0;
    u_int32_t old_phyerr=0, old_ostime=0;
    u_int32_t phyerr_rate=0;
    u_int32_t tmp = 0;
    u_int8_t good;
    struct ath_softc *sc = ATH_DEV_TO_SC(dev);

    spin_lock(ald_data->ald_lock);

    good = ath_hal_getMibCycleCountsPct(sc->sc_ah, &rxclear, &rxframe, &txframe);

    if(good) {
        cu = rxclear;
    }
    old_phyerr = ald_data->ald_phyerr;
    old_ostime = ald_data->ald_ostime;
    new_phyerr = sc->sc_phy_stats[sc->sc_curmode].ast_rx_phyerr;
    new_ostime = OS_GET_TIMESTAMP()/1000;
    if((new_ostime > old_ostime) && (old_ostime > 0))
        phyerr_rate = (new_phyerr - old_phyerr)/(new_ostime - old_ostime);
    else
        phyerr_rate = 0;

    ald_data->ald_phyerr = new_phyerr;
    ald_data->ald_ostime = new_ostime;

    if(ald_data->ald_unicast_tx_packets != 0 ){
        msdu_size = ald_data->ald_unicast_tx_bytes/ald_data->ald_unicast_tx_packets;
    }else{
        msdu_size = ALD_MSDU_SIZE;
    }

    ald_data->phyerr_rate = phyerr_rate;
    ald_data->msdu_size = msdu_size;

    /* currently not in use, the value is not accurate somehow */ 
    if((sc->sc_ald.sc_ald_tbuf != 0) && (sc->sc_ald.sc_ald_tnum != 0)){
        tmp = sc->sc_ald.sc_ald_tbuf/sc->sc_ald.sc_ald_tnum;
        if (tmp > 0) {
            load = sc->sc_txbuf_free*msdu_size/(tmp*1000);
        }
    }else
        load = ALD_MAX_DEV_LOAD;

    if (sc->sc_ald.sc_ald_cunum > 0) {
        cu = sc->sc_ald.sc_ald_cu / sc->sc_ald.sc_ald_cunum;
    }
    sc->sc_ald.sc_ald_cu = 0;
    sc->sc_ald.sc_ald_cunum = 0;
    ald_data->ald_unicast_tx_bytes = 0;
    ald_data->ald_unicast_tx_packets = 0;
   
    ald_data->ald_chan_util = cu;
    ald_data->ald_dev_load = load;
    spin_unlock(ald_data->ald_lock);

    sc->sc_ald.sc_ald_tbuf = 0;
    sc->sc_ald.sc_ald_tnum = 0;
    return 0;
}

void ath_ald_update_frame_stats(struct ath_softc *sc, struct ath_buf *bf, struct ath_tx_status *ts)
{
    struct ieee80211_frame *wh;
    int type;
    int bfUsed = 0;
    u_int32_t ptime = 0;
    u_int32_t airtime = 0;
    int i;
    void *ds = bf->bf_lastbf->bf_desc;

    wh = (struct ieee80211_frame *)wbuf_header(bf->bf_mpdu);
    type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;
    if((type == IEEE80211_FC0_TYPE_DATA) &&
        (!IEEE80211_IS_MULTICAST(wh->i_addr1)) &&
        (!IEEE80211_IS_BROADCAST(wh->i_addr1))){
        for(i=0; i < HAL_NUM_TX_QUEUES; i++) {
            if (ATH_TXQ_SETUP(sc, i)) {
                bfUsed += sc->sc_txq[i].axq_num_buf_used;
            }
        }
        ptime = sc->sc_ald.sc_ald_txairtime;
        airtime = ath_hal_txcalcairtime(sc->sc_ah, ds, ts, AH_FALSE, 1, 1);
        sc->sc_ald.sc_ald_txairtime += airtime;
        if (ptime > sc->sc_ald.sc_ald_txairtime) {
            sc->sc_ald.sc_ald_txairtime = airtime;
            sc->sc_ald.sc_ald_pktnum = 0;
        }
        if (airtime) {
            sc->sc_ald.sc_ald_pktlen += (bf->bf_isaggr ? bf->bf_al : bf->bf_frmlen);
            sc->sc_ald.sc_ald_pktnum += bf->bf_nframes;
            sc->sc_ald.sc_ald_bfused += bfUsed;
            sc->sc_ald.sc_ald_counter += 1;
        }
    }
}
/*
void ath_ald_update_rate_stats(struct ath_softc *sc, struct ath_node *an, u_int8_t rix)
{
    struct atheros_softc  *asc = (struct atheros_softc*)sc->sc_rc;
    const RATE_TABLE_11N  *pRateTable = (const RATE_TABLE_11N *)asc->hwRateTable[sc->sc_curmode];
    struct atheros_node   *asn = ATH_NODE_ATHEROS(an);
    TX_RATE_CTRL          *pRc = (TX_RATE_CTRL*)&(asn->txRateCtrl);

    pRc->TxRateInMbps += pRateTable->info[rix].rateKbps / 1000;
    pRc->Max4msFrameLen += pRateTable->info[rix].max4msframelen;
    pRc->TxRateCount += 1;
}
*/
#endif /* ATH_SUPPORT_LINKDIAG_EXT */
