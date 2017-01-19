/*
 *  Copyright (c) 2010 Atheros Communications Inc.
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
#include "ath_internal.h"
#include "if_athvar.h"
#include "ath_tx99.h"
#include "ratectrl.h"
#include "ratectrl11n.h"
#include "ar9300/ar9300reg.h"
#include "ar9300/ar9300phy.h"

#if ATH_TX99_DIAG
#if defined(_MAVERICK_STA_) || defined(WIN32) || defined(WIN64)
int
tx99_rate_setup(struct ath_softc *sc, u_int32_t *pRateCode, u_int32_t rateKBPS)
{
    struct ath_tx99 *tx99_tgt = sc->sc_tx99;
    struct atheros_softc  *asc = (struct atheros_softc*)sc->sc_rc;
    u_int32_t i;
    
    if(asc == NULL)
    {
        DPRINTF(sc, ATH_DEBUG_TX99, "%s: asc NULL\n", __func__);
        return -EINVAL;
    }
    
    {
    	RATE_TABLE_11N  *pRateTable;
    
        if (tx99_tgt->txfreq < 4900)
            pRateTable = (RATE_TABLE_11N *)asc->hwRateTable[WIRELESS_MODE_11NG_HT20];
        else
            pRateTable = (RATE_TABLE_11N *)asc->hwRateTable[WIRELESS_MODE_11NA_HT20];

        if (pRateTable == NULL) {
            DPRINTF(sc, ATH_DEBUG_TX99, "%s: no 11n rate control table\n", __func__);
        return -EINVAL;
        }
    	
        if (rateKBPS != 0) {	
            for (i = 0; i < pRateTable->rateCount; i++) {
                if (rateKBPS == pRateTable->info[i].rateKbps)
                    break;
            }

            if (i == pRateTable->rateCount) {
                /*
                 * Requested rate not found.
                 */
                DPRINTF(sc, ATH_DEBUG_TX99, "%s: txrate %u not found\n", __func__, rateKBPS);
                return -EINVAL;
            }
        } else {
            return -EINVAL;
        }

        *pRateCode = pRateTable->info[i].rateCode;
    }
    return EOK;
}

int
tx99_get_rateKBPS(struct ath_softc *sc, u_int32_t RateCode, u_int32_t *rateKBPS)
{
    struct ath_tx99 *tx99_tgt = sc->sc_tx99;
    struct atheros_softc  *asc = (struct atheros_softc*)sc->sc_rc;
    u_int32_t mode20Hz, i;

    if(asc == NULL)
    {
        DPRINTF(sc, ATH_DEBUG_TX99, "%s: asc NULL\n", __func__);
        return -EINVAL;
    }

    {
        RATE_TABLE_11N  *pRateTable;
		
        if (tx99_tgt->txfreq < 4900)
            pRateTable = (RATE_TABLE_11N *)asc->hwRateTable[WIRELESS_MODE_11NG_HT20];
        else
            pRateTable = (RATE_TABLE_11N *)asc->hwRateTable[WIRELESS_MODE_11NA_HT20];

        if (pRateTable == NULL) {
            DPRINTF(sc, ATH_DEBUG_TX99, "%s: no 11n rate control table\n", __func__);
            return -EINVAL;
        }

        if (tx99_tgt->htmode == ATH_CWM_MODE20) 
            mode20Hz = 1;
        else if (tx99_tgt->htmode == ATH_CWM_MODE40)
            mode20Hz = 0;
        else 
            return -EINVAL;
		
        if (RateCode != 0) {	
            for (i = 0; i < pRateTable->rateCount; i++) {
                if (RateCode == pRateTable->info[i].rateCode)
                {
                    if (mode20Hz)
                        break;
                    else 
                        mode20Hz++;
                }
            }
			
            if (i == pRateTable->rateCount) {
                /*
                 * Requested rate code not found.
                 */
                DPRINTF(sc, ATH_DEBUG_TX99, "%s: tx ratecode 0X%X not found\n", __func__, RateCode);
                return -EINVAL;
            }
        } else {
            return -EINVAL;
        }

        *rateKBPS = (u_int32_t)pRateTable->info[i].rateKbps;
    }
    return EOK;
}

#ifndef ATH_SUPPORT_HTC
static int 
ath_tgt_tx99(struct ath_softc *sc, int chtype)	
{
    struct ath_tx99 *tx99_tgt = sc->sc_tx99;
    wbuf_t wbuf = tx99_tgt->wbuf;
    struct ath_buf *bf=NULL, *prev=NULL, *first=NULL, *last=NULL;
    struct ath_desc *ds=NULL;
    HAL_11N_RATE_SERIES series[4] = {{ 0 }};
    u_int32_t i;
    struct ath_txq *txq;
    u_int32_t totalbuffer=1;
    u_int32_t r;
    u_int8_t *tmp;
    u_int32_t dmalen =0;
    u_int32_t smartAntenna = SMARTANT_INVALID;

    if(tx99_tgt->type == TX99_M_SINGLE_CARRIER)
    {
        DPRINTF(sc, ATH_DEBUG_TX99, "%s: Single carrier mode\n", __func__);
        ah_tx99_set_single_carrier(sc->sc_ah, tx99_tgt->chanmask, chtype);
        wbuf_release(sc->sc_osdev, wbuf);	
        return EOK;
    }

    /* build output packet */
    {
        struct ieee80211_frame *hdr;
        u_int32_t tmplen;

        static u_int8_t PN9Data[] = {0xff, 0x87, 0xb8, 0x59, 0xb7, 0xa1, 0xcc, 0x24, 
                        0x57, 0x5e, 0x4b, 0x9c, 0x0e, 0xe9, 0xea, 0x50, 
                        0x2a, 0xbe, 0xb4, 0x1b, 0xb6, 0xb0, 0x5d, 0xf1, 
                        0xe6, 0x9a, 0xe3, 0x45, 0xfd, 0x2c, 0x53, 0x18, 
                        0x0c, 0xca, 0xc9, 0xfb, 0x49, 0x37, 0xe5, 0xa8, 
                        0x51, 0x3b, 0x2f, 0x61, 0xaa, 0x72, 0x18, 0x84, 
                        0x02, 0x23, 0x23, 0xab, 0x63, 0x89, 0x51, 0xb3, 
                        0xe7, 0x8b, 0x72, 0x90, 0x4c, 0xe8, 0xfb, 0xc0};
#if 0
        static a_uint8_t test_addr[6] ={ 0x0, 0x0, 0x2, 0x3, 0x0, 0x0};
#endif

        wbuf_set_type(wbuf, WBUF_TX_DATA);
        wbuf_set_len(wbuf, 1800);
        wbuf_set_pktlen(wbuf, 1800);
        tmp = wbuf_header(wbuf);
        tmplen = wbuf_get_pktlen(wbuf);

        hdr = (struct ieee80211_frame *)tmp;
#if 0
        IEEE80211_ADDR_COPY(&hdr->i_addr1, test_addr);
        IEEE80211_ADDR_COPY(&hdr->i_addr2, test_addr);
        IEEE80211_ADDR_COPY(&hdr->i_addr3, test_addr);
#else
        IEEE80211_ADDR_COPY(&hdr->i_addr1, sc->sc_myaddr);
        IEEE80211_ADDR_COPY(&hdr->i_addr2, sc->sc_myaddr);
        IEEE80211_ADDR_COPY(&hdr->i_addr3, sc->sc_myaddr);
#endif

        hdr->i_dur[0] = 0x0;
        hdr->i_dur[1] = 0x0;
        hdr->i_seq[0] = 0x5a;
        hdr->i_seq[1] = 0x5a;
        hdr->i_fc[0] = IEEE80211_FC0_TYPE_DATA;
        hdr->i_fc[1] = 0;
        dmalen += sizeof(struct ieee80211_frame);
        tmp +=dmalen;
        /* data content */
        for (r = 0; r < tmplen; r += sizeof(PN9Data)) {
            memcpy(tmp, PN9Data, sizeof(PN9Data));
            dmalen +=sizeof(PN9Data);
            tmp +=sizeof(PN9Data);
        }
        DPRINTF(sc, ATH_DEBUG_TX99, "%s: Construct test packet\n", __func__);
    }

    /* to set chainmsk */
    ah_tx99_chainmsk_setup(sc->sc_ah, tx99_tgt->chanmask);
    for (i=0; i<4; i++) {
        series[i].Tries = 0xf;
        series[i].Rate = tx99_tgt->txrc;
        series[i].ch_sel = tx99_tgt->chanmask;
        series[i].RateFlags = (tx99_tgt->htmode == ATH_CWM_MODE40) ?  HAL_RATESERIES_2040: 0;   //half GI???
    }

    txq = &sc->sc_txq[WME_AC_VO];
    for (i = 0; i < totalbuffer; i++)
    {	
        bf = TAILQ_FIRST(&sc->sc_txbuf);
        if(bf ==  NULL)
        {
            DPRINTF(sc, ATH_DEBUG_TX99, "%s: allocate ath buffer fail:%d\n", __func__, i+1);
            return -EINVAL;
        }
        TAILQ_REMOVE(&sc->sc_txbuf, bf, bf_list);

        bf->bf_frmlen = dmalen+IEEE80211_CRC_LEN;
        bf->bf_mpdu = wbuf;
        bf->bf_isdata = 1;
        bf->bf_qnum = txq->axq_qnum;
        bf->bf_buf_addr[0] = wbuf_map_single(
                        sc->sc_osdev,
                        wbuf,
                        BUS_DMA_TODEVICE,
                        OS_GET_DMA_MEM_CONTEXT(bf, bf_dmacontext));        
        bf->bf_buf_len[0] = roundup(bf->bf_frmlen, 4);
        ds = bf->bf_desc;
        bf->bf_node =  NULL;
        ath_hal_setdesclink(sc->sc_ah, ds, 0);

        ath_hal_set11n_txdesc(sc->sc_ah, ds
                              , bf->bf_frmlen               /* frame length */
                              , HAL_PKT_TYPE_NORMAL         /* Atheros packet type */
                              , tx99_tgt->txpower           /* txpower */
                              , HAL_TXKEYIX_INVALID         /* key cache index */
                              , HAL_KEY_TYPE_CLEAR          /* key type */
                              , HAL_TXDESC_CLRDMASK
                                | HAL_TXDESC_NOACK          /* flags */
                              );

        r = ath_hal_filltxdesc(sc->sc_ah, ds
                               , bf->bf_buf_addr 	        /* buffer address */
                               , bf->bf_buf_len		        /* buffer length */
                               , 0    				        /* descriptor id */
                               , bf->bf_qnum  		        /* QCU number */
                               , HAL_KEY_TYPE_CLEAR         /* key type */
                               , AH_TRUE                    /* first segment */
                               , AH_TRUE                    /* last segment */
                               , ds                         /* first descriptor */
                               );
        if (r == AH_FALSE) {
            DPRINTF(sc, ATH_DEBUG_TX99, "%s: fail fill tx desc r(%d)\n", __func__, r);
            return -EINVAL;
        }
        ath_desc_swap(ds);

#if UMAC_SUPPORT_SMARTANTENNA
        if(sc->sc_smartant_enable){
            /* same default antenna will be used for all rate series */
            smartAntenna = (sc->sc_defant) |(sc->sc_defant << 8)| (sc->sc_defant << 16) | (sc->sc_defant << 24); 
        } else {
            smartAntenna = SMARTANT_INVALID; /* if smart antenna is not enabled */
        }
#else
        smartAntenna = SMARTANT_INVALID;
#endif 

        ath_hal_set11n_ratescenario(sc->sc_ah, ds, ds, 0, 0, 0, series, 4, 0, smartAntenna);

        if (prev != NULL)
            ath_hal_setdesclink(sc->sc_ah, prev->bf_desc, bf->bf_daddr);

        /* insert the buffers in to tmp_q */
        TAILQ_INSERT_TAIL(&sc->tx99_tmpq, bf, bf_list);

        if(i == totalbuffer-1)
            last= bf;

        prev = bf;
    }
    first = TAILQ_FIRST(&sc->tx99_tmpq);
    ath_hal_setdesclink(sc->sc_ah, last->bf_desc, first->bf_daddr);

    ath_hal_intrset(sc->sc_ah, 0);    	/* disable interrupts */
    /* Force receive off */
    /* XXX re-enable RX in DIAG_SW as otherwise xmits hang below */
    ath_hal_stoppcurecv(sc->sc_ah);	/* disable PCU */
    ath_hal_setrxfilter(sc->sc_ah, 0);	/* clear recv filter */
    ath_hal_stopdmarecv(sc->sc_ah, 0); /* disable DMA engine */
    sc->sc_rxlink = NULL;		/* just in case */

    ah_tx99_start(sc->sc_ah, (u_int8_t)(txq->axq_qnum));  

    ath_hal_puttxbuf(sc->sc_ah, txq->axq_qnum, (a_uint32_t)first->bf_daddr);
    /* trigger tx dma start */
    if (!ath_hal_txstart(sc->sc_ah, txq->axq_qnum)) {
        DPRINTF(sc, ATH_DEBUG_TX99, "%s: txstart failed, disabled by dfs?\n", __func__);
        return -EINVAL;
    }
    DPRINTF(sc, ATH_DEBUG_TX99, "%s: tx99 continuous tx done\n", __func__);
    return EOK;
}

int ath_tx99_tgt_start(struct ath_softc *sc, int chtype)
{
    int ret;

    DPRINTF(sc, ATH_DEBUG_TX99, "%s: tx99 parameter dump!\n", __func__);
    DPRINTF(sc, ATH_DEBUG_TX99, "%s: txrate:%d\n", __func__, sc->sc_tx99->txrate);
    DPRINTF(sc, ATH_DEBUG_TX99, "%s: txpower:%d\n", __func__, sc->sc_tx99->txpower);
    DPRINTF(sc, ATH_DEBUG_TX99, "%s: txchain:%d\n", __func__, sc->sc_tx99->chanmask);
    DPRINTF(sc, ATH_DEBUG_TX99, "%s: htmode:%d\n", __func__, sc->sc_tx99->htmode);
    DPRINTF(sc, ATH_DEBUG_TX99, "%s: type:%d\n", __func__, sc->sc_tx99->type);
    DPRINTF(sc, ATH_DEBUG_TX99, "%s: channel type:%d\n", __func__, chtype);

    TAILQ_INIT(&sc->tx99_tmpq);
    ret = ath_tgt_tx99(sc, chtype);

    DPRINTF(sc, ATH_DEBUG_TX99, "%s: Trigger tx99 start done\n", __func__);

    return ret;
}

void ath_tx99_tgt_stop(struct ath_softc *sc)
{
    a_uint8_t j;
    struct ath_buf *bf=NULL;
    struct ath_tx99 *tx99_tgt = sc->sc_tx99;

    ah_tx99_stop(sc->sc_ah);

    if(tx99_tgt->type == TX99_M_CONT_FRAME_DATA)
    {
        for (j = 0; (bf = TAILQ_FIRST(&sc->tx99_tmpq)) != NULL; j++) {
            bf->bf_mpdu = NULL;
            TAILQ_REMOVE(&sc->tx99_tmpq, bf, bf_list);
            TAILQ_INSERT_HEAD(&sc->sc_txbuf, bf, bf_list);
        }
    }	
}
#endif /* no ATH_SUPPORT_HTC */

#else
#include <adf_os_types.h>
#include <adf_os_pci.h>
#include <adf_os_dma.h>
#include <adf_os_timer.h>
#include <adf_os_lock.h>
#include <adf_os_io.h>
#include <adf_os_mem.h>
#include <adf_os_util.h>
#include <adf_os_stdtypes.h>
#include <adf_os_defer.h>
#include <adf_os_atomic.h>
#include <adf_nbuf.h>
#include <adf_net.h>
#include <adf_net_wcmd.h>
#include <adf_os_irq.h>

#include "if_athvar.h"
#include "ath_internal.h"
#include "ath_tx99.h"
#include "ratectrl.h"
#include "ratectrl11n.h"
#include "ar9300/ar9300reg.h"
#include "ar9300/ar9300phy.h"


static dma_addr_t tx99_bus_map_single(void *hwdev, void *ptr,
			 size_t size, int direction)
{
    HAL_BUS_CONTEXT     *bc = &(devhandle->bc);

    if (!(devhandle->bdev) || bc->bc_bustype == HAL_BUS_TYPE_AHB) {
        dma_map_single(devhandle->bdev, ptr, size,
            (direction == BUS_DMA_TODEVICE)? DMA_TO_DEVICE : DMA_FROM_DEVICE);
    } else {
        pci_map_single(devhandle->bdev, ptr, size,
            (direction == BUS_DMA_TODEVICE)? PCI_DMA_TODEVICE : PCI_DMA_FROMDEVICE);
    }

    return __pa(ptr);
}

static int
tx99_rate_setup(struct ath_softc *sc, a_uint32_t *pRateCode)
{
    struct ath_tx99 *tx99_tgt = sc->sc_tx99;
    struct atheros_softc  *asc = (struct atheros_softc*)sc->sc_rc;
    a_uint32_t i;
    
    if(asc == NULL)
    {
    	adf_os_print("asc NULL\n");
    	return -EINVAL;
    }
    
    {
    	RATE_TABLE_11N  *pRateTable;
    
    	pRateTable = (RATE_TABLE_11N *)asc->hwRateTable[tx99_tgt->txmode];
    	if (pRateTable == NULL) {
    		adf_os_print("no 11n rate control table\n");
    		return -EINVAL;
    	}
    	
    	if (tx99_tgt->txrate != 0) {	
    	    for (i = 0; i < pRateTable->rateCount; i++) {
#if 0
/* ignore the supported rate lookup, since TX99 is test mode.
*/
                if ( pRateTable->info[i].valid==FALSE )
            	    continue;
                if (tx99_tgt->htmode == IEEE80211_CWM_MODE20) {
                    if( pRateTable->info[i].valid==TRUE_40 )
                        continue;
                } else {
                    //in IEEE80211_CWM_MODE40 mode
                    if( (tx99_tgt->txrate == 54000) && ( pRateTable->info[i].valid != TRUE_40 ))
                        continue;
                }
#endif            
                if (tx99_tgt->txrate == pRateTable->info[i].rateKbps)
                    break;
            }
    
            if (i == pRateTable->rateCount) {
                /*
                 * Requested rate not found; use the highest rate.
                 */
                i = pRateTable->rateCount-1;
                adf_os_print("txrate %u not found, using %u\n", tx99_tgt->txrate, pRateTable->info[i].rateKbps);
            }
    	} else {
            /*
            * Use the lowest rate for the current channel.
            */
            adf_os_print("tx99rate was set to 0, use the lowest rate\n");
            i = 0;
    	}
    
    	*pRateCode = pRateTable->info[i].rateCode;
    }
    return EOK;
}

static int 
ath_tgt_tx99(struct ath_softc *sc, int chtype)	
{
    struct ath_tx99 *tx99_tgt = sc->sc_tx99;
    adf_nbuf_t buf = tx99_tgt->skb;
    struct ath_buf *bf=NULL, *prev=NULL, *first=NULL, *last=NULL;
    struct ath_desc *ds=NULL;
    HAL_11N_RATE_SERIES series[4] = {{ 0 }};
    a_uint32_t i;
    struct ath_txq *txq;
    a_uint32_t txrate = 0;
    a_uint32_t totalbuffer=1; //20
    a_uint32_t r;
    a_uint8_t *tmp;
    a_uint32_t dmalen =0;
    uint32_t smartAntenna = SMARTANT_INVALID;
    
    if(tx99_tgt->type == TX99_M_SINGLE_CARRIER)
    {
    	adf_os_print("Single carrier mode\n");	
    	ah_tx99_set_single_carrier(sc->sc_ah, tx99_tgt->chanmask, chtype);
    	if (tx99_tgt->skb) {
    	    adf_nbuf_free(tx99_tgt->skb);
    	    tx99_tgt->skb = NULL;
    	}
    	return EOK;
    }
    
    /* build output packet */
    {
    	struct ieee80211_frame *hdr;
    	a_uint32_t tmplen;
    
    	static a_uint8_t PN9Data[] = {0xff, 0x87, 0xb8, 0x59, 0xb7, 0xa1, 0xcc, 0x24, 
    					 0x57, 0x5e, 0x4b, 0x9c, 0x0e, 0xe9, 0xea, 0x50, 
    					 0x2a, 0xbe, 0xb4, 0x1b, 0xb6, 0xb0, 0x5d, 0xf1, 
    					 0xe6, 0x9a, 0xe3, 0x45, 0xfd, 0x2c, 0x53, 0x18, 
    					 0x0c, 0xca, 0xc9, 0xfb, 0x49, 0x37, 0xe5, 0xa8, 
    					 0x51, 0x3b, 0x2f, 0x61, 0xaa, 0x72, 0x18, 0x84, 
    					 0x02, 0x23, 0x23, 0xab, 0x63, 0x89, 0x51, 0xb3, 
    					 0xe7, 0x8b, 0x72, 0x90, 0x4c, 0xe8, 0xfb, 0xc0};
    	static a_uint8_t test_addr[6] ={ 0x0, 0x0, 0x2, 0x3, 0x0, 0x0};
    
    	adf_nbuf_peek_header(buf, &tmp, &tmplen);
    	
    	hdr = (struct ieee80211_frame *)tmp;
    	IEEE80211_ADDR_COPY(&hdr->i_addr1, test_addr);
    	IEEE80211_ADDR_COPY(&hdr->i_addr2, test_addr);
    	IEEE80211_ADDR_COPY(&hdr->i_addr3, test_addr);
    	hdr->i_dur[0] = 0x0;
    	hdr->i_dur[1] = 0x0;
    	hdr->i_seq[0] = 0x5a;
    	hdr->i_seq[1] = 0x5a;
    	hdr->i_fc[0] = IEEE80211_FC0_TYPE_DATA;
    	hdr->i_fc[1] = 0;
    	dmalen += sizeof(struct ieee80211_frame);
    	tmp +=dmalen;
    	/* data content */
    	for (r = 0; (r + sizeof(PN9Data)) < tmplen; r += sizeof(PN9Data)) {
            adf_os_mem_copy(tmp, PN9Data, sizeof(PN9Data));
            tmp +=sizeof(PN9Data);
    	}
        adf_os_mem_copy(tmp,PN9Data,(sizeof(PN9Data) % r));
        dmalen = tmplen;
    }
    
    /* to setup tx rate */
    tx99_rate_setup(sc, &txrate);
    /* to set chainmsk */
    ah_tx99_chainmsk_setup(sc->sc_ah, tx99_tgt->chanmask);
    for (i=0; i<4; i++) {
        series[i].Tries = 0xf;
        series[i].Rate = txrate;
        series[i].ch_sel = tx99_tgt->chanmask;
        series[i].RateFlags = (tx99_tgt->htmode == IEEE80211_CWM_MODE40) ?  HAL_RATESERIES_2040: 0;   //half GI???
    }
    
    txq = &sc->sc_txq[WME_AC_VO];
    for (i = 0; i < totalbuffer; i++)
    {	
    	bf = TAILQ_FIRST(&sc->sc_txbuf);
        if(bf ==  NULL)
        {
            adf_os_print("ath_tgt_tx99: allocate ath buffer fail:%d\n",i+1);
            return -EINVAL;
        }
    	TAILQ_REMOVE(&sc->sc_txbuf, bf, bf_list);
    	
        bf->bf_frmlen = dmalen+IEEE80211_CRC_LEN;
        bf->bf_mpdu = buf;
        bf->bf_qnum = txq->axq_qnum;
        bf->bf_buf_addr[0] = tx99_bus_map_single(sc->sc_osdev, buf->data, (buf->end-buf->data), BUS_DMA_TODEVICE);
        bf->bf_buf_len[0] = roundup(bf->bf_frmlen, 4);
        ds = bf->bf_desc;
        ath_hal_setdesclink(sc->sc_ah, ds, 0);

        ath_hal_set11n_txdesc(sc->sc_ah, ds
                              , bf->bf_frmlen               /* frame length */
                              , HAL_PKT_TYPE_NORMAL         /* Atheros packet type */
                              , tx99_tgt->txpower           /* txpower */
                              , HAL_TXKEYIX_INVALID         /* key cache index */
                              , HAL_KEY_TYPE_CLEAR          /* key type */
                              , HAL_TXDESC_CLRDMASK
                                | HAL_TXDESC_NOACK          /* flags */
                              );
    	
        r = ath_hal_filltxdesc(sc->sc_ah, ds
                               , bf->bf_buf_addr 	        /* buffer address */
                               , bf->bf_buf_len		        /* buffer length */
                               , 0    				        /* descriptor id */
                               , bf->bf_qnum  		        /* QCU number */
                               , HAL_KEY_TYPE_CLEAR         /* key type */
                               , AH_TRUE                    /* first segment */
                               , AH_TRUE                    /* last segment */
                               , ds                         /* first descriptor */
                               );
    	if (r == AH_FALSE) {
            adf_os_print("%s: fail fill tx desc r(%d)\n", __func__, r);
            return -EINVAL;
    	}
        
#if UMAC_SUPPORT_SMARTANTENNA
    if(sc->sc_smartant_enable)
    {
        /* same default antenna will be used for all rate series */
        smartAntenna = (sc->sc_defant) |(sc->sc_defant << 8)| (sc->sc_defant << 16) | (sc->sc_defant << 24); 
    }
    else    
    {
        smartAntenna = SMARTANT_INVALID; /* if smart antenna is not enabled */
    }
#else
    smartAntenna = SMARTANT_INVALID;
#endif    
        ath_hal_set11n_ratescenario(sc->sc_ah, ds, ds, 0, 0, 0, series, 4, 0,smartAntenna);
            	
    	if (prev != NULL)
            ath_hal_setdesclink(sc->sc_ah, prev->bf_desc, bf->bf_daddr);

    	/* insert the buffers in to tmp_q */
    	TAILQ_INSERT_TAIL(&sc->tx99_tmpq, bf, bf_list);
    
    	if(i == totalbuffer-1)
    	  last= bf;
    
    	prev = bf;
    }
    
    first = TAILQ_FIRST(&sc->tx99_tmpq);
    ath_hal_setdesclink(sc->sc_ah, last->bf_desc, first->bf_daddr);
    
    ath_hal_intrset(sc->sc_ah, 0);    	/* disable interrupts */
    /* Force receive off */
    /* XXX re-enable RX in DIAG_SW as otherwise xmits hang below */
    ath_hal_stoppcurecv(sc->sc_ah);	/* disable PCU */
    ath_hal_setrxfilter(sc->sc_ah, 0);	/* clear recv filter */
    ath_hal_stopdmarecv(sc->sc_ah, 0); /* disable DMA engine */
    sc->sc_rxlink = NULL;		/* just in case */
        
    ah_tx99_start(sc->sc_ah, (a_uint8_t *)txq->axq_qnum);    
        
    ath_hal_puttxbuf(sc->sc_ah, txq->axq_qnum, (a_uint32_t)first->bf_daddr);
    /* trigger tx dma start */
    if (!ath_hal_txstart(sc->sc_ah, txq->axq_qnum)) {
        adf_os_print("ath_tgt_tx99: txstart failed, disabled by dfs?\n");
        return -EINVAL;
    }
    
    adf_os_print("tx99 continuous tx done\n");
    return EOK;
}

int ath_tx99_tgt_start(struct ath_softc *sc, int chtype)
{    
    int ret;
    
    adf_os_print("tx99 parameter dump\n");
    adf_os_print("txrate:%d\n",sc->sc_tx99->txrate);
    adf_os_print("txpower:%d\n",sc->sc_tx99->txpower);
    adf_os_print("txchain:%d\n",sc->sc_tx99->chanmask);
    adf_os_print("htmode:%d\n",sc->sc_tx99->htmode);
    adf_os_print("type:%d\n",sc->sc_tx99->type);
    adf_os_print("channel type:%d\n",chtype);

    TAILQ_INIT(&sc->tx99_tmpq);
    ret = ath_tgt_tx99(sc, chtype);
    
    adf_os_print("Trigger tx99 start done\n");

    return ret;
}

void ath_tx99_tgt_stop(struct ath_softc *sc)
{
    a_uint8_t j;
    struct ath_buf *bf=NULL;
    struct ath_tx99 *tx99_tgt = sc->sc_tx99;
    
    ah_tx99_stop(sc->sc_ah);

    if(tx99_tgt->type == TX99_M_CONT_FRAME_DATA)
    {
        for (j = 0; (bf = TAILQ_FIRST(&sc->tx99_tmpq)) != NULL; j++) {
            TAILQ_REMOVE(&sc->tx99_tmpq, bf, bf_list);
        	TAILQ_INSERT_HEAD(&sc->sc_txbuf, bf, bf_list);
        }
    }	
}
#endif

#endif /* ATH_TX99_DIAG */
