//----------------------------------------------------------------------------
//
//  Project:   BT3_0_CORE
//
//  Filename:  Pal_Cfg.h
//
//  Author:    Mike Tsai
//
//  Created:   3/20/2009
//
//  Descriptions:
//
//    PAL Configuration setting here
//
//  Copyright (c) 2009, Atheros Communications, Inc. All right reserved.
//
//--------------------------------------------------------------------------

/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef __PAL_TXQ_H__
#define __PAL_TXQ_H__

#include "amp_db.h"
#include "poshApi.h"

int pal_tx_queue_attach(AMP_DEV *amp_dev, osdev_t oshandle, pal_tx_queue_data_t *pal_tx_queue_data);

int pal_tx_queue_detach(pal_tx_queue_data_t *pal_tx_queue_data);

int pal_tx_queue_start(pal_tx_queue_data_t *pal_tx_queue_data);

int pal_tx_queue_stop(pal_tx_queue_data_t *pal_tx_queue_data);

int pal_tx_queue_add_packet(pal_tx_queue_data_t *pal_queue_data, POSH_SEND_PACKET_CONTEXT *context);

int pal_tx_queue_enhanced_flush(pal_tx_queue_data_t *pal_tx_queue_data);

#endif  /* __PAL_TXQ_H__ */


