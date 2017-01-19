//----------------------------------------------------------------------------
//
//  Project:   BT3_CORE
//
//  Filename:  bt_inp.h
//
//  Author:    Zhihua Liu
//
//  Created:   03/25/2009
//
//  Descriptions:
//
//    PAL HCI CMD & Data Processing
//
//  Copyright (c) 2009, Atheros Communications, Inc. All right reserved.
//
//----------------------------------------------------------------------------

/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef _H_BT_INP_
#define _H_BT_INP_

void      acl_data_tx_complete(AMP_DEV* Dev, A_UINT32 LogicLinkId, A_BOOL status);
void      Update_local_amp_assoc(AMP_DEV *Dev, AMP_ASSOC_INFO *Local_Amp, A_UINT8 *Chanlist, A_UINT16 Len);
void      pal_evt_set_evt_mask(AMP_DEV *Dev, A_UINT8 index, A_UINT64 mask);
void      init_local_amp_assoc(AMP_DEV *dev, AMP_ASSOC_INFO *amp);
A_UINT32  pal_data_dispatcher(AMP_DEV *Dev, A_UINT8 *pkt, A_UINT16 sz);
int       hci_data_tx_enque(AMP_DEV *dev, A_UINT8 *buf, A_UINT16 sz);
A_UINT16  PAL_SetFakePhyLink(AMP_DEV *dev, A_UINT8 PhyHdl);
void      setup_local_amp_assco_by_country(AMP_ASSOC_INFO *Local_Amp, A_UINT8 *Chanlist, A_UINT16 *plen);
void      setup_default_local_amp_assco(A_UINT8 *Chanlist, A_UINT16 *plen);

#endif

