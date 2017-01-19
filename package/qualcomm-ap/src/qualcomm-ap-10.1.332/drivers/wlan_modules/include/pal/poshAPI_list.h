//----------------------------------------------------------------------------
//
//  Project:   BT3_CORE
//
//  Filename:  poshAPI.h
//
//  Author:    Mike Tsai
//
//  Created:   08/17/2009
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

//Global Commands here, not per link
POSH_API_TABLE(POSH_EXIT,                    AMP_DEV*, void*,                Exit)
POSH_API_TABLE(POSH_CREATE_MAC,              AMP_DEV*, void*,                CreateMac)
POSH_API_TABLE(POSH_DELETE_MAC,              AMP_DEV*, void*,                DeleteMac)
POSH_API_TABLE(POSH_GET_SUPPORTED_PHY_TYPE,  AMP_DEV*, void*,                GetSupportedPhyType)
POSH_API_TABLE(POSH_GET_MAX_DATA_RATE,       AMP_DEV*, void*,                GetMaxDataRate)
POSH_API_TABLE(POSH_RESET,                   AMP_DEV*, void*,                Reset)
POSH_API_TABLE(POSH_QUERYCHANNELINFO,        AMP_DEV*, void*,                QueryChannelInfo)
POSH_API_TABLE(POSH_SETSHORTRANGEMODE,       AMP_DEV*, AMP_ASSOC_INFO*,      SetSRM)
POSH_API_TABLE(POSH_STARTBEACON,             AMP_DEV*, AMP_ASSOC_INFO*,      StartBeacon)
POSH_API_TABLE(POSH_STOPBEACON,              AMP_DEV*, void*,                StopBeacon)
POSH_API_TABLE(POSH_QUERYCTL,                AMP_ASSOC_INFO*, A_UINT8*,      QueryCTL)
POSH_API_TABLE(POSH_QUERYIE,                 AMP_ASSOC_INFO*, A_UINT8*,      QueryIE)
//Link specific commands, per link
POSH_API_TABLE(POSH_SETCHANNEL,              AMP_DEV*, AMP_ASSOC_INFO*,				SetChannel)
POSH_API_TABLE(POSH_CONNECT,                 AMP_DEV*, AMP_ASSOC_INFO*,				AmpConnect)
POSH_API_TABLE(POSH_DISCONNECT,              AMP_DEV*, AMP_ASSOC_INFO*,				AmpDisconnect)
POSH_API_TABLE(POSH_ALLOCATEPACKET,          AMP_DEV*, POSH_SEND_PACKET_CONTEXT*,   AllocatePacket)
POSH_API_TABLE(POSH_FREEPACKET,              AMP_DEV*, POSH_SEND_PACKET_CONTEXT*,   FreePacket)
POSH_API_TABLE(POSH_POPULATEPACKETCONTEXT,   AMP_DEV*, POSH_SEND_PACKET_CONTEXT*,   PopulatePacketContext)
POSH_API_TABLE(POSH_SENDPACKET,              AMP_DEV*, POSH_SEND_PACKET_CONTEXT*,   SendPacket)
POSH_API_TABLE(POSH_FLUSHPACKET,             AMP_DEV*, AMP_ASSOC_INFO*,				FlushPacket)
POSH_API_TABLE(POSH_SETRTS,                  AMP_DEV*, A_UINT8,	  			        SetRTS)
POSH_API_TABLE(POSH_SETENCRYPTIONKEY,        AMP_DEV*, AMP_ASSOC_INFO*,				SetEncryptionKey)
//Callback function, Received data dispatch and Tx complete callback
POSH_API_TABLE(POSH_REGISTERDATADISPATCHER,  AMP_DEV*, POSH_DATA_DISPATCHER , RegisterDataDispatcher)
POSH_API_TABLE(POSH_REGISTERTXCOMPLETE_CallBack,  AMP_DEV*, POSH_DATASENDCOMPLETECB , RegisterTxCallBack)
