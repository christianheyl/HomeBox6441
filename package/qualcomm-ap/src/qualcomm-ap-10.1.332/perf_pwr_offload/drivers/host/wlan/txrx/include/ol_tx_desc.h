/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
/**
 * @file ol_tx_desc.h
 * @brief API definitions for the tx descriptor module within the data SW.
 */
#ifndef _OL_TX_DESC__H_
#define _OL_TX_DESC__H_

#include <queue.h>         /* TAILQ_HEAD */
#include <adf_nbuf.h>      /* adf_nbuf_t */
#include <ol_txrx_types.h> /* ol_tx_desc_t */
#include <ol_txrx_internal.h> /* TXRX_STATS_ADD */

/**
 * @brief Allocate and initialize a tx descriptor for a LL system.
 * @details
 *  Allocate a tx descriptor pair for a new tx frame - a SW tx descriptor
 *  for private use within the host data SW, and a HTT tx descriptor for
 *  downloading tx meta-data to the target FW/HW.
 *  Fill in the fields of this pair of tx descriptors based on the
 *  information in the netbuf.
 *  For LL, this includes filling in a fragmentation descriptor to
 *  specify to the MAC HW where to find the tx frame's fragments.
 *
 * @param pdev - the data physical device sending the data
 *      (for accessing the tx desc pool)
 * @param vdev - the virtual device sending the data
 *      (for specifying the transmitter address for multicast / broadcast data)
 * @param netbuf - the tx frame
 */
struct ol_tx_desc_t *
ol_tx_desc_ll(
    struct ol_txrx_pdev_t *pdev,
    struct ol_txrx_vdev_t *vdev,
    adf_nbuf_t netbuf);

/**
 * @brief Allocate and initialize a tx descriptor for a HL system.
 * @details
 *  Allocate a tx descriptor pair for a new tx frame - a SW tx descriptor
 *  for private use within the host data SW, and a HTT tx descriptor for
 *  downloading tx meta-data to the target FW/HW.
 *  Fill in the fields of this pair of tx descriptors based on the
 *  information in the netbuf.
 *
 * @param pdev - the data physical device sending the data
 *      (for accessing the tx desc pool)
 * @param vdev - the virtual device sending the data
 *      (for specifying the transmitter address for multicast / broadcast data)
 * @param netbuf - the tx frame
 */
struct ol_tx_desc_t *
ol_tx_desc_hl(
    struct ol_txrx_pdev_t *pdev,
    struct ol_txrx_vdev_t *vdev,
    adf_nbuf_t netbuf);

/**
 * @brief Use a tx descriptor ID to find the corresponding desriptor object.
 *
 * @param pdev - the data physical device sending the data
 * @param tx_desc_id - the ID of the descriptor in question
 * @return the descriptor object that has the specified ID
 */
struct ol_tx_desc_t *
ol_tx_desc_find(struct ol_txrx_pdev_t *pdev, u_int16_t tx_desc_id);

/**
 * @brief Free a list of tx descriptors and the tx frames they refer to.
 * @details
 *  Free a batch of "standard" tx descriptors and their tx frames.
 *  Free each tx descriptor, by returning it to the freelist.
 *  Unmap each netbuf, and free the netbufs as a batch.
 *  Irregular tx frames like TSO or managment frames that require
 *  special handling are processed by the ol_tx_desc_frame_free_nonstd
 *  function rather than this function.
 *  
 * @param pdev - the data physical device that sent the data
 * @param tx_descs - a list of SW tx descriptors for the tx frames
 * @param had_error - boolean indication of whether the transmission failed.
 *            This is provided to callback functions that get notified of
 *            the tx frame completion.
 */
void ol_tx_desc_frame_list_free(
    struct ol_txrx_pdev_t *pdev,
    ol_tx_desc_list *tx_descs,
    int had_error);

/**
 * @brief Free a non-standard tx frame and its tx descriptor.
 * @details
 *  Check the tx frame type (e.g. TSO vs. management) to determine what
 *  special steps, if any, need to be performed prior to freeing the
 *  tx frame and its tx descriptor.
 *  This function can also be used to free single standard tx frames.
 *  After performing any special steps based on tx frame type, free the
 *  tx descriptor, i.e. return it to the freelist, and unmap and
 *  free the netbuf referenced by the tx descriptor.
 *  
 * @param pdev - the data physical device that sent the data
 * @param tx_desc - the SW tx descriptor for the tx frame that was sent
 * @param had_error - boolean indication of whether the transmission failed.
 *            This is provided to callback functions that get notified of
 *            the tx frame completion.
 */
void ol_tx_desc_frame_free_nonstd(
    struct ol_txrx_pdev_t *pdev,
    struct ol_tx_desc_t *tx_desc,
    int had_error);

/*
 * @brief Determine the ID of a tx descriptor.
 *
 * @param pdev - the data physical device that is sending the data
 * @param tx_desc - the descriptor whose ID is being determined
 * @return numeric ID that uniquely identifies the tx descriptor
 */
static inline int
ol_tx_desc_id(struct ol_txrx_pdev_t *pdev, struct ol_tx_desc_t *tx_desc)
{
    return ((union ol_tx_desc_list_elem_t *) tx_desc) - pdev->tx_desc.array;
}


static inline struct ol_tx_desc_t *
ol_tx_desc_alloc(struct ol_txrx_pdev_t *pdev)
{
    struct ol_tx_desc_t *tx_desc = NULL;

    adf_os_spin_lock_bh(&pdev->tx_mutex);
    if (pdev->tx_desc.freelist) {
        tx_desc = &pdev->tx_desc.freelist->tx_desc;
        pdev->tx_desc.freelist = pdev->tx_desc.freelist->next;
        TXRX_STATS_ADD(pdev, pub.tx.desc_in_use, 1);
    }
    adf_os_spin_unlock_bh(&pdev->tx_mutex);

    return tx_desc;
}

static inline void
ol_tx_desc_free(struct ol_txrx_pdev_t *pdev, struct ol_tx_desc_t *tx_desc)
{
    adf_os_spin_lock_bh(&pdev->tx_mutex);
    ((union ol_tx_desc_list_elem_t *) tx_desc)->next = pdev->tx_desc.freelist;
    pdev->tx_desc.freelist = (union ol_tx_desc_list_elem_t *) tx_desc;
    TXRX_STATS_SUB(pdev, pub.tx.desc_in_use, 1);
    adf_os_spin_unlock_bh(&pdev->tx_mutex);
}
#endif /* _OL_TX_DESC__H_ */
