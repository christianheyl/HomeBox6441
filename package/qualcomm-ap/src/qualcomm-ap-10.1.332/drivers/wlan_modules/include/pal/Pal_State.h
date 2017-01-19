//----------------------------------------------------------------------------
//
//  Project:   BT3_0_CORE
//
//  Filename:  Pal_state.h
//
//  Author:    Mike Tsai
//
//  Created:   2/24/2009
//
//  Descriptions:
//
//    PAL state definitions
//
//  Copyright (c) 2009, Atheros Communications, Inc. All right reserved.
//
//--------------------------------------------------------------------------

/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef __PAL_STATE_H__
#define __PAL_STATE_H__

#include "pal_osapi.h"

typedef enum {
        PAL_WAIT_PHYSICAL_CREATION= 0,     // IDLE state waiting for Create/Accept Physical Link
		PAL_WAIT_WRITE_RMT_AMP_ASSOC,      // Received HCI Create/Accept Physical Link, is waiting for RMT Amp Assoc
		PAL_WAIT_BEACON_START,             // Processed Remote AMP Assoc and has set the channel, waiting for beacon start
        PAL_WAIT_READ_LOCAL_AMP_ASSOC,     // Sent channel select event and is waiting for read Local AMP Assoc
        PAL_WAIT_CONNECT,                  // PAL_WAIT_FOR_CONNECT (ASSOC_RSP sent/received inside Miniport driver)
        PAL_WAIT_DISCONNECT,               // PAL_WAIT_FOR_DISCONNECT (ASSOC_RSP sent/received inside Miniport driver)
		PAL_WAIT_M1,                       // Waiting for M1
    	PAL_WAIT_M2,                       // Waiting for M2
		PAL_WAIT_M3,                       // Waiting for M3
		PAL_WAIT_M4_ONAIR,                 // Waiting for M4 on air.
		PAL_WAIT_M4,                       // Waiting for M4
		PAL_PHYSICAL_LINK_ON,              // Physical Link ON
		PAL_WAIT_BEACON_STOP,              // Wait for Beacon to stop
}PAL_PHYSICAL_ACTION_STATE;		

typedef enum{
    EVENT_CREATE_PHYSICAL_LINK = 0,         // initiator, hci_write_remote_amp_assoc
    EVENT_ACCEPT_PHYSICAL_LINK,             // responder, hci_write_remote_amp_assoc
    EVENT_WRITE_REMOTE_AMP_ASSOC,
    EVENT_READ_LOCAL_AMP_ASSOC,
    EVENT_CONNECT_ASSOC_FAILED,
    EVENT_CONNECT_ACCEPT_TIMEOUT,           // timer,     timeout on given phyhdl or MP auth timeout
    EVENT_CONNECT_SUCCESS,                  // MP,        Auth/Assoc process completed
    EVENT_PREF_CHANNEL_READY,  
    EVENT_RECV_M1_FRAME,
    EVENT_RECV_M2_FRAME,
    EVENT_RECV_M3_FRAME,
    EVENT_RECV_M4_ACK,
    EVENT_RECV_M4_FRAME,
    EVENT_BEACON_START_SUCCESS,
	EVENT_BEACON_STOP_SUCCESS,
    EVENT_BEACON_START_FAILED,
    EVENT_DISCONECT_PHYSICAL_LINK,          // hci_disconnect_physcial_link, logical channel should be already free
    EVENT_MAC_MEDIA_LOSS_INDICATION,        // signal lost indication
    EVENT_MAC_CONNECTION_CANCEL_INDICATION, // beacon stopped notify from miniport
    //All timeout indication
    EVENT_SEC_RETRY_TIMEOUT,                //4-way handshaking retry timeout
    EVENT_SEC_RETRY_MAX,                    //4-way handshaking retry max
    EVENT_LINK_SVTO,                        //Link Supervision Timeout
    EVENT_LINK_SV_REQ                       //Time to send LSV request

} PHY_LINK_SM_EVENT;

A_UINT8 Pal_Physical_Link_Thread(AMP_ASSOC_INFO *amp, A_UINT16 Event, A_UINT8 *Data, A_UINT16 Len);

#ifdef PAL_USER_SPACE_CODE
//====================================================================================
// POSH simulation of Auth_req/Auth_rsp/Assoc_Req/Assoc_Rsp of miniport operation
// shall not be part of the posh operation, but not for PAL either
//====================================================================================
typedef enum {
	MP_SIM_AUTH_IDLE, 
    MP_SIM_WAIT_AUTH_REQ,
    MP_SIM_WAIT_ASSOC_REQ,	
    MP_SIM_WAIT_AUTH_RESP,	
    MP_SIM_WAIT_ASSOC_RESP
} MP_SIM_WAIT_STATE;
#endif

#endif  /* __PAL_STATE_H__ */


