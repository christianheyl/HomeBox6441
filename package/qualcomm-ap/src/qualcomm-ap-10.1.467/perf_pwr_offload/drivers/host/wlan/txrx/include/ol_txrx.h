/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef _OL_TXRX__H_
#define _OL_TXRX__H_

#include <adf_nbuf.h> /* adf_nbuf_t */
#include <ol_txrx_types.h> /* ol_txrx_vdev_t, etc. */

void
ol_txrx_peer_unref_delete(struct ol_txrx_peer_t *peer);

#endif /* _OL_TXRX__H_ */
