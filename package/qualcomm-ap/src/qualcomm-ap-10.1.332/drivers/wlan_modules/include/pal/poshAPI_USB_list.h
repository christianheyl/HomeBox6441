//----------------------------------------------------------------------------
//
//  Project:   BT3_CORE
//
//  Filename:  poshAPI_USB_list.h
//
//  Author:    Sharo Chen
//
//  Created:   03/15/2010
//
//  Descriptions:
//
//    Table for creating all POSH commands, you only have to enter the new POSH API
//    in this table once and compiler will automatically generate the type and
//    function table. This way, we won't end up adding API in the table, but
//    forgot to fill in the function or register the table with function pointer
//
//  Copyright (c) 2009, Atheros Communications, Inc. All right reserved.
//
//----------------------------------------------------------------------------

/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#ifdef ATH_USB
//Global Commands here, not per link
POSH_API_TABLE(POSH_SETSHORTRANGEMODE,       AMP_DEV*, AMP_ASSOC_INFO*,      SetSRM)
POSH_API_TABLE(POSH_STARTBEACON,             AMP_DEV*, AMP_ASSOC_INFO*,      StartBeacon)
POSH_API_TABLE(POSH_STOPBEACON,              AMP_DEV*, void*,                StopBeacon)
//Link specific commands, per link
POSH_API_TABLE(POSH_CONNECT,                 AMP_DEV*, AMP_ASSOC_INFO*,				AmpConnect)
POSH_API_TABLE(POSH_DISCONNECT,              AMP_DEV*, AMP_ASSOC_INFO*,				AmpDisconnect)
POSH_API_TABLE(POSH_SETRTS,                  AMP_DEV*, A_UINT8,	  			        SetRTS)
POSH_API_TABLE(POSH_SETENCRYPTIONKEY,        AMP_DEV*, AMP_ASSOC_INFO*,				SetEncryptionKey)
#endif


