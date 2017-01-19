/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
/**
 * @file ol_txrx_api.h
 * @brief Definitions used in multiple external interfaces to the txrx SW.
 */
#ifndef _OL_TXRX_API__H_
#define _OL_TXRX_API__H_

/**
 * @typedef ol_txrx_pdev_handle
 * @brief opaque handle for txrx physical device object
 */
struct ol_txrx_pdev_t;
typedef struct ol_txrx_pdev_t* ol_txrx_pdev_handle;

/**
 * @typedef ol_txrx_vdev_handle
 * @brief opaque handle for txrx virtual device object
 */
struct ol_txrx_vdev_t;
typedef struct ol_txrx_vdev_t* ol_txrx_vdev_handle;

/**
 * @typedef ol_txrx_peer_handle
 * @brief opaque handle for txrx peer object
 */
struct ol_txrx_peer_t;
typedef struct ol_txrx_peer_t* ol_txrx_peer_handle;

#if QCA_OL_11AC_FAST_PATH
extern uint32_t
ol_tx_ll_fast(ol_txrx_vdev_handle vdev,
              adf_nbuf_t *nbuf_arr,
              uint32_t num_msdus);
#endif /* QCA_OL_11AC_FAST_PATH */

#endif /* _OL_TXRX_API__H_ */
