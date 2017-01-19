//----------------------------------------------------------------------------
//
//  Project:   BT3_CORE
//
//  Filename:  pal.h
//
//  Author:    Zhihua Liu
//
//  Created:   03/27/2009
//
//  Descriptions:
//
//    PAL API INTERFACE
//
//  Copyright (c) 2009, Atheros Communications, Inc. All right reserved.
//
//----------------------------------------------------------------------------

/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef _PAL_H_
#define _PAL_H_

#include "pal_cfg.h"

/*-----------------Standard Interface------------------------------------------*/

typedef int (* pal_evt_dispatcher)(PVOID pDev, A_UINT8 *buf, A_UINT16 sz);
typedef int (* acl_data_dispatcher)(PVOID pDev, A_UINT8 *buf, A_UINT16 sz);
void        pal_evt_set_dispatcher(pal_evt_dispatcher fn);
void        pal_data_set_dispatcher(acl_data_dispatcher fn);
void        hci_cmd_que_exit(void);
void        PAL_hci_cmd_que_init(void);
int         hci_cmd_enque(A_UINT8 *buf, A_UINT16 sz);
int         hci_data_tx_enque(AMP_DEV *dev, A_UINT8 *buf, A_UINT16 sz);

/*-----------------Testing Interface------------------------------------------*/
#ifdef PAL_IOP_TEST 
void        PAL_SetDataMangle(BOOL bMangle);
void        pal_set_tx_pause(A_BOOL state);
A_BOOL      pal_is_tx_que_paused();
A_UINT8     PAL_Is_Tx_Thread_Running();
void        pal_set_flush_test_state(A_UINT8 state);
void        pal_set_CB_ID_test_state(A_UINT8 state);
#endif


#endif
