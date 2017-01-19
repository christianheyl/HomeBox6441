#include <adf_os_module.h>
#include <adf_os_stdtypes.h>
#include <adf_os_types.h>
#include <adf_os_mem.h>
#include <adf_nbuf.h>
#include <adf_os_defer.h>
#include <adf_os_lock.h>

#include <athdefs.h>
#include <a_osapi.h>
#include <hif.h>

#include "hif_gmac.h"
#include "adf_os_time.h"

#define HIF_GMAC_PIPES          1
#define HIF_GMAC_DBG            0

#define HIF_PIPE_GMAC_RX        1
#define HIF_PIPE_GMAC_TX        1

#define hdl_to_gmac_sc(_hdl)   (hif_gmac_softc_t *)(_hdl)
#define hdl_to_gmac_node(_hdl) (hif_gmac_node_t *)(_hdl)

#define seq_inc(val)    ((val + 1) & 255)

a_uint8_t G_seqno = 0;

static inline a_status_t __gmac_send_ack(athhdr_t *ath, hif_gmac_node_t *node);


void    hif_gmac_recv(adf_nbuf_t skb, hif_gmac_node_t  *node,  
        a_uint8_t proto); 

void    hif_gmac_recv_excep(adf_nbuf_t skb, hif_gmac_node_t  *node,  
        a_uint8_t proto); 

void    hif_gmac_dev_attach(hif_gmac_node_t  *node);

void    hif_gmac_dev_detach(hif_gmac_node_t  *node);
#define app_ind_pkt(_sc, _pkt)  do {    \
    adf_os_assert((_sc).pkt_cb.rxCompletionHandler);   \
    (_sc).pkt_cb.rxCompletionHandler((_sc).pkt_cb.Context, (_pkt), \
            HIF_PIPE_GMAC_RX); \
}while (0)	


/*****************************************************************************/
/*****************************  Globals  *************************************/
/*****************************************************************************/
hif_gmac_softc_t   *sc_gbl;
/*****************************************************************************/
/*******************************  APIs  **************************************/
/*****************************************************************************/

static inline void
hif_gmac_header(adf_nbuf_t  skb, gmac_hdr_t  *hdr)
{
    gmac_hdr_t  *gmac;

    gmac = (gmac_hdr_t *)adf_nbuf_push_head(skb, GMAC_HLEN); 
    
    adf_os_mem_copy(gmac, hdr, GMAC_HLEN);
}

/**
 * @brief This is exception path of receive 
 */
void
hif_gmac_recv_excep(adf_nbuf_t   skb, hif_gmac_node_t  *node, a_uint8_t  proto)
{
    athhdr_t          *gmac_ath;
    int                drop = 1;
    hif_gmac_softc_t *sc = sc_gbl;

    gmac_ath = &node->hdr.ath;

    /* adf_os_print("%s\n",__func__); */
    switch (proto) {
    case ATH_P_MAGNORM:
        /**
         * Firmware is executing this the first arrival of the normal
         * packet swap the receive functions
         */
        gmac_ath->type.proto = ATH_P_MAGNORM;

		adf_os_print("%s received TGT_READY_MSG\n", __func__);

        node->tgt_rdy_skb = skb;

        sc_gbl->attach(node);

        break;

    case ATH_P_MAGBOOT:
        drop =  sc->discv(skb, node);
        
        if(HIF_GMAC_DBG)
            adf_os_print("Inside MAGBOOT, drop = %d\n", drop);
        
        if (!drop)
           hif_gmac_dev_attach(node);

        adf_nbuf_free(skb);

        break;

    default: /* Drop frame not of known subtype */
        adf_nbuf_free(skb);

        break;
    }
} 
/**
 * @brief Send the packet to the Upper layer registered
 *        previously
 * 
 * @param skb
 * @param node
 */
void
hif_gmac_recv(adf_nbuf_t skb, hif_gmac_node_t  *node, a_uint8_t proto)
{
    hif_gmac_softc_t *sc = sc_gbl;
    athhdr_t           *p_ath;
    a_status_t status = A_OK;

    p_ath = ath_hdr(skb);

    if(p_ath->rel) { 
        
        status = __gmac_send_ack(p_ath, node);

        if(status != A_OK) {
            adf_os_print("not able to send ack for seqno %d\n",p_ath->seq_no);
            adf_os_assert(0);
        }
        
        if (seq_inc(G_seqno) == p_ath->seq_no)
            G_seqno = seq_inc(G_seqno);
        else {
            adf_os_print("recvd seq %d glb seq %d\n",p_ath->seq_no,G_seqno);
            adf_nbuf_free(skb);
            return;
        }    
    }

    
    adf_nbuf_pull_head(skb, ATH_HLEN);
   
    app_ind_pkt(sc->app_reg[node->app_cur], skb); 
}

static inline a_status_t
__gmac_send_ack(athhdr_t *ath, hif_gmac_node_t *node)
{

    adf_nbuf_t nbuf = NULL;
    gmac_hdr_t  *gmac = NULL;
    
    nbuf = adf_nbuf_alloc(node->os_hdl, 100, GMAC_HLEN, 0);

    if(nbuf == ADF_NBUF_NULL) {
        adf_os_print("not able to allocate nbuf for sending ack\n");
        adf_os_assert(0);
        return A_ERROR;
    }    
   
   gmac = (gmac_hdr_t *)adf_nbuf_push_head(nbuf, GMAC_HLEN);
   
   adf_os_mem_copy(&gmac->ath, ath, ATH_HLEN);
   adf_os_mem_copy(&gmac->eth, &node->hdr.eth, sizeof(ethhdr_t));
   
   return(os_gmac_xmit(nbuf, node->dev)); 
}

/**
 * @brief Used by HTC to transmit the packet
 * XXX: How to figure out the dev for xmit
 * @param hdl
 * @param buf
 * @param pipe
 * 
 * @return a_status_t
 */
static inline a_status_t
hif_gmac_hard_xmit(hif_gmac_node_t *node, adf_nbuf_t  buf) 
{
    a_bool_t  headroom;
    
    headroom = (adf_nbuf_headroom(buf) < GMAC_HLEN);
    
    /* adf_os_assert(!skb_shared(buf)); */

    if(HIF_GMAC_DBG)
        adf_os_print("Inside %s\n",__FUNCTION__);

    if(HIF_GMAC_DBG && headroom)
        adf_os_print("Realloc headroom\n");

    /**
     * Do we need to unshare
     */
    /* skb = adf_nbuf_unshare(buf); */

    if(headroom)
        buf = adf_nbuf_realloc_headroom(buf, GMAC_HLEN);
    
    hif_gmac_header(buf, &node->hdr);

    return os_gmac_xmit(buf, node->dev);

}

void
hif_gmac_dev_attach(hif_gmac_node_t  *node)
{
    hif_gmac_app_t    *app = &sc_gbl->app_reg[node->app_cur];

    adf_os_print("%s\n",__func__); 
    
    app->dev_cb.deviceInsertedHandler(node,node->os_hdl);
}

void
hif_gmac_dev_detach(hif_gmac_node_t  *node)
{
    hif_gmac_app_t    *app = &sc_gbl->app_reg[node->app_cur];

    app->dev_cb.deviceRemovedHandler(app->ctx);
}


void
hif_gmac_app_detach(hif_gmac_softc_t  *sc, hif_app_type_t app_type)
{
    hif_gmac_node_t   *node = sc->node_tbl; /* first entry */
    a_uint32_t         i;

    for (i = 0; i < GMAC_TBL_SIZE; i++) {

        if (!node[i].dev)
            continue;

        node[i].app_cur = app_type;
        
        if ( app_type == APP_FWD )
            hif_gmac_dev_detach(&node[i]);
        else if ( app_type == APP_HTC )
            sc_gbl->detach(&node[i]);
        else
            adf_os_print("Invalid APP type\n");

    }
}

hif_status_t 
HIF_register(struct htc_drvreg_callbacks *cb)
{
    hif_gmac_softc_t  *sc = sc_gbl;

    adf_os_print("Registering the HIF :%d\n", sc->next_app);
    
    if (sc->next_app >= NUM_APPS)
        return A_ERROR;

    sc->app_reg[sc->next_app].dev_cb = *cb;

    sc->next_app ++;

    return HIF_OK;
}

A_STATUS
HIF_deregister(void)
{
    hif_gmac_softc_t  *sc = sc_gbl;
    a_uint32_t i = 0;

    adf_os_print("De-registering from HIF\n");

    for (i = APP_FWD; i < NUM_APPS; i++)
        hif_gmac_app_detach(sc, i);
    
    return A_OK;
}

/**
 * @brief Load callbacks for packet transactions 
 *        Only arrive when app state is known
 */
void 
HIFPostInit(void *HIFHandle, void *hHTC, struct htc_callbacks *callbacks)
{
    hif_gmac_softc_t  *sc = sc_gbl;
    hif_gmac_node_t   *node = hdl_to_gmac_node(HIFHandle);

    adf_os_print("Post Init called\n");

    sc->app_reg[node->app_cur].ctx    = hHTC;
    sc->app_reg[node->app_cur].pkt_cb = *callbacks;
}

void 
HIFStart(HIF_HANDLE hHIF)
{
    hif_gmac_node_t   *node = (hif_gmac_node_t *)hHIF;
    hif_gmac_softc_t  *sc   = sc_gbl;
    adf_nbuf_t         skb;

    adf_os_assert(node->app_cur != APP_FWD);
    
    adf_os_spin_lock_bh(&sc->lock_bh);

    skb = node->tgt_rdy_skb;
    node->tgt_rdy_skb = NULL;

    adf_os_spin_unlock_bh(&sc->lock_bh);

    if (skb)
        sc->recv_fn[RECV_DEF](skb, node, ATH_P_MAGNORM);
}

hif_status_t
HIFSend(void *dev, a_uint8_t PipeID, adf_nbuf_t hdr_buf, adf_nbuf_t buf)
{
    adf_os_assert(!hdr_buf);

    return hif_gmac_hard_xmit((hif_gmac_node_t * )dev, buf);
}

void 
HIFShutDown(void *HIFHandle)
{
}

void 
HIFGetDefaultPipe(void *HIFHandle, a_uint8_t *ULPipe, a_uint8_t *DLPipe)
{
    *ULPipe = HIF_PIPE_GMAC_RX;
    *DLPipe = HIF_PIPE_GMAC_TX;
}

a_uint8_t 
HIFGetULPipeNum(void)
{
    return HIF_GMAC_PIPES;
}

a_uint8_t 
HIFGetDLPipeNum(void)
{
    return HIF_GMAC_PIPES;
}

void HIFEnableFwRcv(HIF_HANDLE hHIF)
{
    return;
}

/**
 * @brief Get the number of Free Descriptors available for TX
 * 
 * @param hHIF
 * @param PipeID
 * 
 * @return a_uint16_t
 */
a_uint16_t 
HIFGetFreeQueueNumber(HIF_HANDLE hHIF, a_uint8_t PipeID)
{
    return (PipeID <= 6) ? 1 : 0;
}

a_uint16_t 
HIFGetMaxQueueNumber(HIF_HANDLE hHIF, a_uint8_t PipeID)
{
    return (PipeID <= 6) ? 1 : 0;
}

/* @brief HIF GMAC init module */
hif_gmac_softc_t  *
hif_gmac_init(void)
{
    sc_gbl = adf_os_mem_alloc(NULL,sizeof(struct hif_gmac_softc));
    adf_os_mem_set(sc_gbl, 0, sizeof(struct hif_gmac_softc));

    adf_os_print("%s\n",__func__);

    sc_gbl->next_app = APP_FWD;

    sc_gbl->recv_fn[RECV_EXCEP] = hif_gmac_recv_excep;
    sc_gbl->recv_fn[RECV_DEF]   = hif_gmac_recv;
    
    adf_os_spinlock_init(&sc_gbl->lock_bh);

    return sc_gbl;
}

void
hif_gmac_exit(hif_gmac_softc_t *sc)
{
    HIF_deregister();

    adf_os_mem_free(sc);
    
    sc_gbl = NULL;

    adf_os_print("%s\n",__func__); 

}

a_status_t
hif_boot_start(HIF_HANDLE hdl)
{
    hif_gmac_node_t *node = hdl_to_gmac_node(hdl);

    /* adf_os_print("%s\n",__func__); */

	node->app_cur = APP_FWD;

    return A_STATUS_OK;
}

void
hif_boot_done(HIF_HANDLE hdl)
{
    hif_gmac_node_t *node = hdl_to_gmac_node(hdl);
    
    /* adf_os_print("%s\n",__func__); */

	node->app_cur = APP_HTC;
}


/* Used by HTC & FWD */
adf_os_export_symbol(HIF_deregister);
adf_os_export_symbol(HIF_register);
adf_os_export_symbol(HIFGetDefaultPipe);
adf_os_export_symbol(HIFStart);
adf_os_export_symbol(HIFPostInit);
adf_os_export_symbol(HIFSend);
adf_os_export_symbol(HIFShutDown);
adf_os_export_symbol(HIFGetULPipeNum);
adf_os_export_symbol(HIFGetDLPipeNum);
adf_os_export_symbol(HIFEnableFwRcv);
adf_os_export_symbol(HIFGetFreeQueueNumber);
adf_os_export_symbol(HIFGetMaxQueueNumber);

/* Used by FWD */
adf_os_export_symbol(hif_boot_start);
adf_os_export_symbol(hif_boot_done);

/* Used by if_ath_gmac */
adf_os_export_symbol(hif_gmac_dev_attach);
adf_os_export_symbol(hif_gmac_dev_detach);
adf_os_export_symbol(hif_gmac_init);
adf_os_export_symbol(hif_gmac_exit);
