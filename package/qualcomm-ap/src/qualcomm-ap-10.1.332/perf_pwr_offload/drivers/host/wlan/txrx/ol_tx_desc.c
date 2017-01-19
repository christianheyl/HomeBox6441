/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */


#include <adf_nbuf.h>      /* adf_nbuf_t, etc. */
#include <adf_os_lock.h>   /* adf_os_spinlock */
#include <queue.h>         /* TAILQ */

#include <ol_htt_tx_api.h> /* htt_tx_desc_id */

#include <ol_txrx_types.h> /* ol_txrx_pdev_t */
#include <ol_tx_desc.h>
#include <ol_txrx_internal.h>


/* TBD: make this inline in the .h file? */
struct ol_tx_desc_t *
ol_tx_desc_find(struct ol_txrx_pdev_t *pdev, u_int16_t tx_desc_id)
{
    return &pdev->tx_desc.array[tx_desc_id].tx_desc;
}


#define ol_tx_desc_base_inline(pdev, vdev, netbuf, tx_desc) \
do {                                                        \
    /* allocate the descriptor */                           \
    tx_desc = ol_tx_desc_alloc(pdev);                       \
    if (!tx_desc) return NULL;                              \
                                                            \
    /* initialize the SW tx descriptor */                   \
    tx_desc->netbuf = netbuf;                               \
    tx_desc->pkt_type = ol_tx_frm_std;                      \
                                                            \
    /* initialize the HW tx descriptor */                   \
    htt_tx_desc_init(                                       \
        pdev->htt_pdev, tx_desc->htt_tx_desc,               \
        ol_tx_desc_id(pdev, tx_desc),                       \
        adf_nbuf_len(netbuf),                               \
        vdev->vdev_id,                                      \
        pdev->htt_pkt_type,                                 \
        adf_nbuf_get_tx_cksum(netbuf),                      \
        adf_nbuf_get_tid(netbuf));                          \
} while (0)


struct ol_tx_desc_t *
ol_tx_desc_ll(
    struct ol_txrx_pdev_t *pdev,
    struct ol_txrx_vdev_t *vdev,
    adf_nbuf_t netbuf)
{
    struct ol_tx_desc_t *tx_desc;
    int i, num_frags;

    ol_tx_desc_base_inline(pdev, vdev, netbuf, tx_desc);

    /* initialize the fragmentation descriptor */
    num_frags = adf_nbuf_get_num_frags(netbuf);
    htt_tx_desc_num_frags(pdev->htt_pdev, tx_desc->htt_tx_desc, num_frags);
    for (i = 0; i < num_frags; i++) {
        int frag_len;
        u_int32_t frag_paddr;

        frag_len = adf_nbuf_get_frag_len(netbuf, i);
        frag_paddr = adf_nbuf_get_frag_paddr_lo(netbuf, i);
        htt_tx_desc_frag(
            pdev->htt_pdev, tx_desc->htt_tx_desc, i, frag_paddr, frag_len);
    }
    return tx_desc;
}

struct ol_tx_desc_t *
ol_tx_desc_hl(
    struct ol_txrx_pdev_t *pdev,
    struct ol_txrx_vdev_t *vdev,
    adf_nbuf_t netbuf)
{
    struct ol_tx_desc_t *tx_desc;

    ol_tx_desc_base_inline(pdev, vdev, netbuf, tx_desc);

    return tx_desc;
}

void ol_tx_desc_frame_list_free(
    struct ol_txrx_pdev_t *pdev,
    ol_tx_desc_list *tx_descs,
    int had_error)
{
    struct ol_tx_desc_t *tx_desc, *tmp;
    adf_nbuf_t msdus = NULL;

    TAILQ_FOREACH_SAFE(tx_desc, tx_descs, tx_desc_list_elem, tmp) {
        adf_nbuf_t msdu = tx_desc->netbuf;

        adf_os_atomic_init(&tx_desc->ref_cnt); /* clear the ref cnt */
        adf_nbuf_unmap(pdev->osdev, msdu, ADF_OS_DMA_TO_DEVICE);
        /* free the tx desc */
        ol_tx_desc_free(pdev, tx_desc);
        /* link the netbuf into a list to free as a batch */
        adf_nbuf_set_next(msdu, msdus);
        msdus = msdu;
    }
    /* free the netbufs as a batch */
    adf_nbuf_tx_free(msdus, had_error);
}

void ol_tx_desc_frame_free_nonstd(
    struct ol_txrx_pdev_t *pdev,
    struct ol_tx_desc_t *tx_desc,
    int had_error)
{
    int mgmt_type;
    ol_txrx_mgmt_tx_cb cb;
	
    adf_os_atomic_init(&tx_desc->ref_cnt); /* clear the ref cnt */

    adf_nbuf_unmap(pdev->osdev, tx_desc->netbuf, ADF_OS_DMA_TO_DEVICE);
    /* check the frame type to see what kind of special steps are needed */
    if (tx_desc->pkt_type == ol_tx_frm_tso) {
#if 0
        /*
         * Free the segment's customized ethernet+IP+TCP header.
         * Fragment 0 added by the WLAN driver is the HTT+HTC tx descriptor.
         * Fragment 1 added by the WLAN driver is the Ethernet+IP+TCP header
         * added for this TSO segment.
         */
        tso_tcp_hdr = adf_nbuf_get_frag_vaddr(tx_desc->netbuf, 1);
        ol_tx_tso_hdr_free(pdev, tso_tcp_hdr);
#endif
        /* free the netbuf */
        adf_nbuf_set_next(tx_desc->netbuf, NULL);
        adf_nbuf_tx_free(tx_desc->netbuf, had_error);
    } else if (tx_desc->pkt_type >= OL_TXRX_MGMT_TYPE_BASE) {
        mgmt_type = tx_desc->pkt_type - OL_TXRX_MGMT_TYPE_BASE;
        cb = pdev->tx_mgmt.callbacks[mgmt_type].cb;
        if (cb) {
            void *ctxt;
            ctxt = pdev->tx_mgmt.callbacks[mgmt_type].ctxt;
            cb(ctxt, tx_desc->netbuf, had_error);
        }
        /* free the netbuf */
        adf_nbuf_free(tx_desc->netbuf);
    } else {
        /* single regular frame */
        adf_nbuf_set_next(tx_desc->netbuf, NULL);
        adf_nbuf_tx_free(tx_desc->netbuf, had_error);
    }
    /* free the tx desc */
    ol_tx_desc_free(pdev, tx_desc);
}
