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
#include <ieee80211_var.h>
#include "ieee80211_ioctl.h"
#include "ald_netlink.h"

#if ATH_SUPPORT_LINKDIAG_EXT
#include "ieee80211_ald_priv.h"    /* Private to ALD module */
#include "ath_dev.h"
#include "if_athvar.h"
int ieee80211_ald_vattach(wlan_if_t vap)
{
    struct ieee80211com     *ic;
    struct ath_linkdiag     *ald_priv;
    int  ret = 0;

    ic  = vap->iv_ic;

    if (vap->iv_ald) {
        ASSERT(vap->iv_ald == 0);
        return -1; /* already attached ? */
    }

    ald_priv = (struct ath_linkdiag *) OS_MALLOC(ic->ic_osdev, (sizeof(struct ath_linkdiag)), GFP_ATOMIC);
    vap->iv_ald = ald_priv;

    if (ald_priv == NULL) {
       return -ENOMEM;
    } else {
        OS_MEMZERO(ald_priv, sizeof(struct ath_linkdiag));
        spin_lock_init(&ald_priv->ald_lock);
        
        vap->iv_ald->ald_maxcu = WLAN_MAX_MEDIUM_UTILIZATION;
        
        return ret;
    }
}

int ieee80211_ald_vdetach(wlan_if_t vap)
{
    if(vap->iv_ald){
        spin_lock_destroy(&ald_priv->ald_lock);
        OS_FREE(vap->iv_ald);
    }
    return 0;
}

int wlan_ald_get_statistics(wlan_if_t vap, void *para)
{
    struct ieee80211com     *ic;
    struct ald_stat_info *param = (struct ald_stat_info *)para;
    ic  = vap->iv_ic;
    vap->iv_ald->staticp = param;
    switch (param->cmd)
    {
    case IEEE80211_ALD_MAXCU:
        vap->iv_ald->ald_maxcu = param->maxcu;
        break;
    default:
        break;
    }
    ath_net80211_ald_get_statistics(vap);
    return 0;
}

void ath_net80211_ald_get_statistics(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(ic);
    struct ieee80211_node_table *nt = &ic->ic_sta;
    struct ieee80211_node *ni = NULL, *next=NULL;
    rwlock_state_t lock_state;
    int index = 0;
    struct ieee80211_chanutil_info *cu = &vap->chanutil_info;
    ath_node_t an;
               
    scn->sc_ops->ald_collect_data(scn->sc_dev, vap->iv_ald);
    vap->iv_ald->staticp->utility = cu->value * 100 / 256;
    vap->iv_ald->staticp->load = vap->iv_ald->ald_dev_load;
    vap->iv_ald->staticp->txbuf = vap->iv_ald->ald_txbuf_used;
    vap->iv_ald->staticp->curThroughput = vap->iv_ald->ald_curThroughput;

    OS_RWLOCK_READ_LOCK(&nt->nt_nodelock, &lock_state);
    TAILQ_FOREACH_SAFE(ni, &nt->nt_node, ni_list, next) {
        /* ieee80211_sta_leave may be called or RWLOCK_WRITE_LOCK may be acquired */
        /* TBD: this is not multi-thread safe. Should use wlan_iterate_station_list */ 
        if(IEEE80211_ADDR_EQ(vap->iv_myaddr, ni->ni_macaddr))
            continue;
        ieee80211_ref_node(ni);
        an = ATH_NODE_NET80211(ni)->an_sta;

        memcpy(vap->iv_ald->staticp->lkcapacity[index].da, ni->ni_macaddr, 6);
        scn->sc_ops->ald_collect_ni_data(scn->sc_dev, an, vap->iv_ald, &ni->ni_ald);
        vap->iv_ald->staticp->lkcapacity[index].capacity = ni->ni_ald.ald_capacity;
        vap->iv_ald->staticp->lkcapacity[index].aggr = ni->ni_ald.ald_aggr;
        vap->iv_ald->staticp->lkcapacity[index].phyerr = ni->ni_ald.ald_phyerr;
        vap->iv_ald->staticp->lkcapacity[index].lastper = ni->ni_ald.ald_lastper;
        vap->iv_ald->staticp->lkcapacity[index].msdusize = ni->ni_ald.ald_msdusize;
        index++;
        ieee80211_free_node(ni);
        if(index >= MAX_NODES_NETWORK)
            goto staticout;
    }
staticout:    
    OS_RWLOCK_READ_UNLOCK(&nt->nt_nodelock, &lock_state);
    vap->iv_ald->staticp->nientry = index;
    vap->iv_ald->staticp->vapstatus = (vap->iv_state_info.iv_state == IEEE80211_S_RUN ? 1 : 0);
}

void ieee80211_ald_record_tx(struct ieee80211vap *vap, wbuf_t wbuf, int datalen)
{
    struct ieee80211_frame *wh;
    int type;

    wh = (struct ieee80211_frame *)wbuf_header(wbuf);
    type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;

    if((type == IEEE80211_FC0_TYPE_DATA) &&
          (!IEEE80211_IS_MULTICAST(wh->i_addr1)) &&
          (!IEEE80211_IS_BROADCAST(wh->i_addr1))){
        int32_t tmp;
        tmp = vap->iv_ald->ald_unicast_tx_bytes;
        spin_lock(vap->iv_ald->ald_lock);
        vap->iv_ald->ald_unicast_tx_bytes += datalen;
        if(tmp > vap->iv_ald->ald_unicast_tx_bytes){
            vap->iv_ald->ald_unicast_tx_bytes = datalen;
            vap->iv_ald->ald_unicast_tx_packets = 0;
        }
        vap->iv_ald->ald_unicast_tx_packets++;
        spin_unlock(vap->iv_ald->ald_lock);
    }
}
#endif


