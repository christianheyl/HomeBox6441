//----------------------------------------------------------------------------
//
//  Project:   BT3_CORE
//
//  Filename:  poshAPI.h
//
//  Author:    Zhihua Liu
//
//  Created:   02/09/2009
//
//  Descriptions:
//
//    PAL OS SHIM EXPORT API
//
//  Copyright (c) 2009, Atheros Communications, Inc. All right reserved.
//
//----------------------------------------------------------------------------

/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef _H_POSH_API_H_
#define _H_POSH_API_H_

#include "pal_osapi.h"
#include "tque.h"

#define AMP_SSID_SIZE                21 // AMP-XX-XX-XX-XX-XX-XX
#define IEEE80211_FC0_SUBTYPE_QOS    0x80
#define LPOSH_PACKET_QUEUE_DEPTH     72 //64 + 4 links*2
#define NUMBER_OF_PACKET_AVAILABLE   (LPOSH_PACKET_QUEUE_DEPTH - 6)

typedef enum{
    POSH_OK = 0,
    POSH_FAILED,
} POSH_STATUS;

typedef enum{
    EVENT_POSH_VAP_CREATED,                // Vap created complete
    EVENT_POSH_VAP_DELETED,                // Vap Deleted complete
    EVENT_POSH_PREF_CHANNEL_READY,       // We got prefer channel list back
    EVENT_POSH_BEACON_START_SUCCESS,       // wlan_amp_connect success
    EVENT_POSH_BEACON_START_FAILED,        // wlan_amp_connect failed
    EVENT_POSH_BEACON_STOP_SUCCESS,        // 
    EVENT_POSH_BEACON_STOP_FAILED,         // 
    EVENT_POSH_CONNECTE_TIMEOUT,           //this could be connect or disconnect timeout
    EVENT_POSH_SEC_RETRY_TIMEOUT,          //M1 to M4 receiving timeout
    EVENT_POSH_SEC_RETRY_MAX,              //We have reache the max retry
    EVENT_POSH_SVTO,                       //Link Supervision Timeout
    EVENT_POSH_SV_REQ,                     //Time to send link supervision request 
    EVENT_POSH_DISCONNECTED,               //POSH indicates a disconnect from remote device
    EVENT_POSH_CONNECTED,                  //POSH indicates auth/assoc is completed
    EVENT_POSH_FLUSH_COMPLT,
    EVENT_POSH_PAUSE,                      // gernerate by mp when shutdown
    EVENT_POSH_READY,                      // posh is back to ready state
    EVENT_POSH_RESET_COMPLETE,             // wlan_amp_reset success
    EVENT_POSH_RESET_FAILED,               // wlan_amp_reset failed
    EVENT_POSH_SEND_PACKET_COMPLETE,       // packet sent complete
    //Security stuff, now route through same working queue
    EVENT_POSH_RECV_M4_ACK,                // Receive M4 ACK (reponder side
    EVENT_POSH_RECV_M1_FRAME,    
    EVENT_POSH_RECV_M2_FRAME,
    EVENT_POSH_RECV_M3_FRAME,
    EVENT_POSH_RECV_M4_FRAME,
    EVENT_POSH_RADIO_DISABLED,
    EVENT_POSH_AMP_SUSPEND,
    EVENT_POSH_AMP_RESUME,
} POSH_PAL_EVENT;

typedef enum {
    WLAN_RESET_START = 1,           /* Driver being reset due to request from Ndis */
    WLAN_REQUEST_COMPLETION,        /* Port disconnected on request */
    WLAN_PEER_DISCONNECT,           /* Peer node disconnected */
    WLAN_SCAN_START,                /* Miniport driver needed to scan */
    WLAN_ROAM_START,                /* STA on the miniport driver decided to roam to a different channel */
    WLAN_CHANNEL_CHANGE,            /* STA on the miniport was asked to change channel by root AP */
    WLAN_PHY_STATE,                 /* WLAN Radio state change - RFKill */
    WLAN_POWER_STATE,               /* WLAN Power state change - S3/S4 */
} WLAN_DISCONNECT_REASON;

typedef struct{
    A_UINT8     mac[6];
} MAC_ADDR, *PMAC_ADDR;

typedef struct {
    A_UINT32 wifiCap;
    A_UINT8 reglen;                                     // regdomain length, no include self
    A_UINT8 regdomain[128];                             // regdomain buffer. suppose 128 bytes enough
} AMP_CHANNEL_INFO;


//--------------------------------------------------------------------------------
// This is the context we pass to POSH for sending out a data packet
// When the tx is completed, POSH needs to return this context back to PAL
// so PAL can construct number of complete block event to BT host
//--------------------------------------------------------------------------------
typedef struct {
    AMP_DEV             *Dev;
    AMP_ASSOC_INFO      *r_amp;
    A_UINT32            logical_link_id;
    A_UINT8             *buf;
    A_UINT16            len;
    A_UINT8             user_priority;
    PACKET_HANDLE       poshHdl;
} POSH_SEND_PACKET_CONTEXT;

typedef struct {
    A_UINT8  Channel;
    A_UINT8  SSID[40];
} AMP_START_PARAMS, *PAMP_START_PARAMS;

typedef A_UINT32 (* POSH_DATA_DISPATCHER)(AMP_DEV *Dev, A_UINT8* pkt, A_UINT16 sz);
typedef A_UINT32 (* POSH_DATASENDCOMPLETECB)(AMP_DEV *Dev, A_UINT32 LogicLinkId, A_UINT8 status);

#define POSH_API_TABLE(POSH_CMD, P1, P2, Name)  typedef  A_UINT8(* POSH_CMD)(P1, P2);
#include "pal/poshapi_list.h"
#undef  POSH_API_TABLE

#define POSH_API_TABLE(POSH_CMD, P1, P2, Name) POSH_CMD  Name;
typedef struct {
#include "pal/poshapi_list.h"
}POSH_OPS, *PPOSH_OPS;
#undef POSH_API_TABLE

A_UINT8  LPOSH_Init(AMP_DEV *Dev, PPOSH_OPS fn_table);
A_UINT32 pal_posh_event_dispatcher(AMP_DEV *Dev, POSH_PAL_EVENT event_id, MP_EVENT_CONTEXT *Context);
A_UINT8  UPOSH_Init(AMP_DEV *pDev, PDRIVER_OBJECT pDrv, int InstanceNumber);
A_UINT8  UPOSH_Exit (AMP_DEV *pDev);
void UPOSH_Resume (AMP_DEV *pDev);
A_UINT8 UPOSH_Suspend (AMP_DEV *pDev);
void     PAL_timer_process(PAL_TIMER_LIST* PAL_TimerList);
typedef  void (*callbackfunc) (void *Context);
A_UINT32 pal_settimer(AMP_DEV *Dev, A_UINT32 ms, callbackfunc cb, void* context, PAL_TIMER* timer);
void     pal_cleartimer(AMP_DEV *Dev, PAL_TIMER* timerid);
A_UINT32 pal_sendpkt_complete(AMP_DEV *Dev, A_UINT32 LogicLinkId, A_UINT8 status);
A_UINT8  PAL_Timer_Init(AMP_DEV *Dev);

#endif


