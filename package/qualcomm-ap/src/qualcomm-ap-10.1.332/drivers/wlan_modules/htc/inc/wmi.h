/*
 * Copyright (c) 2004-2006 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * This file contains the definitions of the WMI protocol specified in the
 * Wireless Module Interface (WMI).  It includes definitions of all the
 * commands and events. Commands are messages from the host to the WM.
 * Events and Replies are messages from the WM to the host.
 *
 * Ownership of correctness in regards to WMI commands
 * belongs to the host driver and the WM is not required to validate
 * parameters for value, proper range, or any other checking.
 *
 */

#ifndef _WMI_H_
#define _WMI_H_

#if 0
#ifndef ATH_TARGET
//#include "athstartpack.h"
#endif
#endif

#include "athdefs.h"
//#include "wmix.h"
#include "wmi_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HTC_PROTOCOL_VERSION    0x0002
#define HTC_PROTOCOL_REVISION   0x0000

#define WMI_PROTOCOL_VERSION    0x0002
#define WMI_PROTOCOL_REVISION   0x0000

#define ATH_MAC_LEN             6               /* length of mac in bytes */
#define WMI_CMD_MAX_LEN         100
#define WMI_CONTROL_MSG_MAX_LEN     256
#define WMI_OPT_CONTROL_MSG_MAX_LEN 1536
#define IS_ETHERTYPE(_typeOrLen)        ((_typeOrLen) >= 0x0600)
#define RFC1042OUI      {0x00, 0x00, 0x00}

#define IP_ETHERTYPE 0x0800

#define WMI_IMPLICIT_PSTREAM 0xFF
#define WMI_MAX_THINSTREAM 15

struct host_app_area_s {
    a_uint32_t wmi_protocol_ver;
};

/*
 * Data Path
 */
typedef PREPACK struct {
    a_uint8_t     dstMac[ATH_MAC_LEN];
    a_uint8_t     srcMac[ATH_MAC_LEN];
    a_uint16_t    typeOrLen;
} POSTPACK ATH_MAC_HDR;

typedef PREPACK struct {
    a_uint8_t     dsap;
    a_uint8_t     ssap;
    a_uint8_t     cntl;
    a_uint8_t     orgCode[3];
    a_uint16_t    etherType;
} POSTPACK ATH_LLC_SNAP_HDR;

typedef enum {
    DATA_MSGTYPE = 0x0,
    CNTL_MSGTYPE,
    SYNC_MSGTYPE,
    OPT_MSGTYPE,
} WMI_MSG_TYPE;


typedef PREPACK struct {
    a_int8_t      rssi;            
    a_uint8_t     info;            /* WMI_MSG_TYPE in lower 2 bits - b1b0 */
                                 /* UP in next 3 bits - b4b3b2 */
#define WMI_DATA_HDR_MSG_TYPE_MASK  0x03
#define WMI_DATA_HDR_MSG_TYPE_SHIFT 0
#define WMI_DATA_HDR_UP_MASK        0x07
#define WMI_DATA_HDR_UP_SHIFT       2   
#define WMI_DATA_HDR_IS_MSG_TYPE(h, t)  (((h)->info & (WMI_DATA_HDR_MSG_TYPE_MASK)) == (t))
} POSTPACK WMI_DATA_HDR;


#define WMI_DATA_HDR_SET_MSG_TYPE(h, t) (h)->info = (((h)->info & ~(WMI_DATA_HDR_MSG_TYPE_MASK << WMI_DATA_HDR_MSG_TYPE_SHIFT)) | (t << WMI_DATA_HDR_MSG_TYPE_SHIFT))
#define WMI_DATA_HDR_SET_UP(h, p) (h)->info = (((h)->info & ~(WMI_DATA_HDR_UP_MASK << WMI_DATA_HDR_UP_SHIFT)) | (p << WMI_DATA_HDR_UP_SHIFT))

/*
 * Control Path
 */
typedef PREPACK struct {
    a_uint16_t    commandId;
    a_uint16_t    seqNo;
} POSTPACK WMI_CMD_HDR;        /* used for commands and events */


/*
 * Frame Types
 */
typedef enum {
    WMI_FRAME_BEACON        =   0,
    WMI_FRAME_PROBE_REQ,
    WMI_FRAME_PROBE_RESP,
    WMI_FRAME_ASSOC_REQ,
    WMI_FRAME_ASSOC_RESP,
    WMI_NUM_MGMT_FRAME 
} WMI_MGMT_FRAME_TYPE;

/*
 * Connect Command
 */
typedef enum {
    INFRA_NETWORK       = 0x01,
    ADHOC_NETWORK       = 0x02,
    ADHOC_CREATOR       = 0x04,
    OPT_NETWORK         = 0x08,
} NETWORK_TYPE;

typedef enum {
    OPEN_AUTH           = 0x01,
    SHARED_AUTH         = 0x02,
    LEAP_AUTH           = 0x04,  /* different from IEEE_AUTH_MODE definitions */
} DOT11_AUTH_MODE;

typedef enum {
    NONE_AUTH           = 0x01, 
    WPA_AUTH            = 0x02,
    WPA_PSK_AUTH        = 0x03,
    WPA2_AUTH           = 0x04,
    WPA2_PSK_AUTH       = 0x05,
    WPA_AUTH_CCKM       = 0x06,
    WPA2_AUTH_CCKM      = 0x07,
} AUTH_MODE;

typedef enum {
    NONE_CRYPT          = 0x01,
    WEP_CRYPT           = 0x02,
    TKIP_CRYPT          = 0x03,
    AES_CRYPT           = 0x04,
} CRYPTO_TYPE;

#define WMI_MIN_CRYPTO_TYPE NONE_CRYPT
#define WMI_MAX_CRYPTO_TYPE (AES_CRYPT + 1)

#define WMI_MIN_KEY_INDEX   0
#define WMI_MAX_KEY_INDEX   3

#define WMI_MAX_KEY_LEN     32

#define WMI_MAX_SSID_LEN    32

typedef enum {
    CONNECT_ASSOC_POLICY_USER = 0x0001,
    CONNECT_SEND_REASSOC = 0x0002,
    CONNECT_IGNORE_WPAx_GROUP_CIPHER = 0x0004,
    CONNECT_PROFILE_MATCH_DONE = 0x0008,
    CONNECT_IGNORE_AAC_BEACON = 0x0010,
    CONNECT_CSA_FOLLOW_BSS = 0x0020,
} WMI_CONNECT_CTRL_FLAGS_BITS;

#define DEFAULT_CONNECT_CTRL_FLAGS    (CONNECT_CSA_FOLLOW_BSS)

/*
 * WMI_ECHO_CMDID
 */
#define WMI_ECHOCMD_MSG_MAX_LEN         53//64 - HTC_HDR_LENGTH + sizeof(WMI_CMD_HDR) - 1

typedef PREPACK struct {
    a_uint8_t     msgSize;
    a_uint8_t     msgData[1];
} POSTPACK WMI_ECHO_CMD;

/*
 * WMI_ACCESS_MEMORY_CMDID
 */
#define WMI_ACCESS_MEMORY_MAX_TUPLES    8

typedef PREPACK struct {
    a_uint16_t    addressL;
    a_uint16_t    addressH;
    a_uint16_t    valueL;
    a_uint16_t    valueH;
} POSTPACK WMI_AVT;

typedef PREPACK struct {
    a_uint16_t     tupleNumL;
    a_uint16_t     tupleNumH;
    WMI_AVT      avt[1];
} POSTPACK WMI_ACCESS_MEMORY_CMD;

typedef PREPACK struct {
a_uint32_t tx_tick;
a_uint32_t tx_counter;
} POSTPACK WMI_TX_DEBUG_EVENT;

/*
 * List of Events (target to host)
 */
typedef enum {
    WMI_TGT_RDY_EVENTID           = 0x1001,
    WMI_SWBA_EVENTID,
    WMI_FATAL_EVENTID,
    WMI_TXTO_EVENTID,
    WMI_BMISS_EVENTID,
    WMI_WLAN_TXCOMP_EVENTID,
#ifdef MAGPIE_UAPSD
    WMI_UAPSD_TXCOMP_EVENTID,
    WMI_UAPSD_QOSNULL_EVENTID,
    WMI_PSCHANGE_EVENTID,
#endif    
    WMI_DELBA_EVENTID,
    WMI_TXRATE_EVENTID,
    WMI_ACTION_TXCOMP_EVENTID,  /* Event for Action frame TX Complete */
    WMI_ACTION_TXFAIL_EVENTID,  /* Event for Action frame TX Failure */
    WMI_GENTIMER_EVENTID,
    WMI_NODEKICK_EVENTID,    
} WMI_EVENT_ID;

typedef PREPACK struct {
    a_uint64_t currentTsf;
    a_uint8_t beaconPendingCount;
} POSTPACK WMI_SWBA_EVENT;


typedef PREPACK struct {
    a_uint32_t trigger_mask;
    a_uint32_t thresh_mask;
    a_uint32_t curr_tsf;
} POSTPACK WMI_GENTIMER_EVENT;

#ifndef ATH_TARGET
//#include "athendpack.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* _WMI_H_ */
