/*
 * Copyright (c) 2011, Atheros Communications Inc.
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
 */

/* Host Debug log implementation */


#include "athdefs.h"
#include "a_types.h"
#include "a_osapi.h"
#include "a_debug.h"
#include "ol_defines.h"
#include "ol_if_ath_api.h"
#include "ol_helper.h"
#include "dbglog_host.h"
#include "wmi.h"
#include "wmi_unified_api.h"
#include <pktlog_ac_api.h>

#define DBGLOG_PRINT_PREFIX "FWLOG: "

module_dbg_print mod_print[WLAN_MODULE_ID_MAX];
dbglog_prt_path_t dbglog_prt_path;

A_STATUS 
wmi_config_debug_module_cmd(wmi_unified_t  wmi_handle, 
                            struct dbglog_config_msg_s *config);

const char *dbglog_get_module_str(A_UINT32 module_id) 
{
    switch (module_id) {
    case WLAN_MODULE_INF:
        return "INF";
    case WLAN_MODULE_WMI:
        return "WMI";
    case WLAN_MODULE_STA_PWRSAVE:
        return "STA PS";
    case WLAN_MODULE_WHAL:
        return "WHAL";
    case WLAN_MODULE_COEX:
        return "COEX";
    case WLAN_MODULE_ROAM:
        return "ROAM";
    case WLAN_MODULE_RESMGR_CHAN_MANAGER:
        return "CHANMGR";
    case WLAN_MODULE_RESMGR_OCS:
        return "OCS";
    case WLAN_MODULE_VDEV_MGR:
        return "VDEV";
    case WLAN_MODULE_SCAN:
        return "SCAN";
    case WLAN_MODULE_RATECTRL:
        return "RC";
    case WLAN_MODULE_AP_PWRSAVE:
        return "AP PS";
    case WLAN_MODULE_BLOCKACK:
        return "BA";
    case WLAN_MODULE_MGMT_TXRX:
        return "MGMT";
    case WLAN_MODULE_DATA_TXRX:
        return "DATA";
    case WLAN_MODULE_HTT:
        return "HTT";
    case WLAN_MODULE_HOST:
        return "HOST";
    case WLAN_MODULE_BEACON:
        return "BEACON";
    case WLAN_MODULE_OFFLOAD:
        return "OFFLOAD";
    case WLAN_MODULE_WAL:
        return "WAL";
    case WAL_MODULE_DE:
        return "DE";
    case WLAN_MODULE_PCIELP:
        return "PCIELP";
    case WLAN_MODULE_RTT:
        return "RTT";
    case WLAN_MODULE_DCS:
        return "DCS";
    case WLAN_MODULE_ANI:
        return "ANI";
    default:
        return "UNKNOWN";
    }
}

char * DBG_MSG_ARR[WLAN_MODULE_ID_MAX][MAX_DBG_MSGS] =
{
    {
        "INF_MSG_START",
        "INF_ASSERTION_FAILED",
        "INF_TARGET_ID",
        "INF_MSG_END"
    },
    {
        "WMI_DBGID_DEFINITION_START",
        "WMI_CMD_RX_XTND_PKT_TOO_SHORT",
        "WMI_EXTENDED_CMD_NOT_HANDLED",
        "WMI_CMD_RX_PKT_TOO_SHORT",
        "WMI_CALLING_WMI_EXTENSION_FN",
        "WMI_CMD_NOT_HANDLED",
        "WMI_IN_SYNC",
        "WMI_TARGET_WMI_SYNC_CMD",
        "WMI_SET_SNR_THRESHOLD_PARAMS",
        "WMI_SET_RSSI_THRESHOLD_PARAMS",
        "WMI_SET_LQ_TRESHOLD_PARAMS",
        "WMI_TARGET_CREATE_PSTREAM_CMD",
        "WMI_WI_DTM_INUSE",
        "WMI_TARGET_DELETE_PSTREAM_CMD",
        "WMI_TARGET_IMPLICIT_DELETE_PSTREAM_CMD",
        "WMI_TARGET_GET_BIT_RATE_CMD",
        "WMI_GET_RATE_MASK_CMD_FIX_RATE_MASK_IS",
        "WMI_TARGET_GET_AVAILABLE_CHANNELS_CMD",
        "WMI_TARGET_GET_TX_PWR_CMD",
        "WMI_FREE_EVBUF_WMIBUF",
        "WMI_FREE_EVBUF_DATABUF",
        "WMI_FREE_EVBUF_BADFLAG",
        "WMI_HTC_RX_ERROR_DATA_PACKET",
        "WMI_HTC_RX_SYNC_PAUSING_FOR_MBOX",
        "WMI_INCORRECT_WMI_DATA_HDR_DROPPING_PKT",
        "WMI_SENDING_READY_EVENT",
        "WMI_SETPOWER_MDOE_TO_MAXPERF",
        "WMI_SETPOWER_MDOE_TO_REC",
        "WMI_BSSINFO_EVENT_FROM",
        "WMI_TARGET_GET_STATS_CMD",
        "WMI_SENDING_SCAN_COMPLETE_EVENT",
        "WMI_SENDING_RSSI_INDB_THRESHOLD_EVENT ",
        "WMI_SENDING_RSSI_INDBM_THRESHOLD_EVENT",
        "WMI_SENDING_LINK_QUALITY_THRESHOLD_EVENT",
        "WMI_SENDING_ERROR_REPORT_EVENT",
        "WMI_SENDING_CAC_EVENT",
        "WMI_TARGET_GET_ROAM_TABLE_CMD",
        "WMI_TARGET_GET_ROAM_DATA_CMD",
        "WMI_SENDING_GPIO_INTR_EVENT",
        "WMI_SENDING_GPIO_ACK_EVENT",
        "WMI_SENDING_GPIO_DATA_EVENT",
        "WMI_CMD_RX",
        "WMI_CMD_RX_XTND",
        "WMI_EVENT_SEND",
        "WMI_EVENT_SEND_XTND",
        "WMI_CMD_PARAMS_DUMP_START",
        "WMI_CMD_PARAMS_DUMP_END",
        "WMI_CMD_PARAMS",
        "WMI_EVENT_ALLOC_FAILURE",
        "WMI_DBGID_DCS_PARAM_CMD",
        "WMI_DBGOD_DEFNITION_END",
    },
    {
        "PS_STA_DEFINITION_START",
        "PS_STA_PM_ARB_REQUEST",
    },
    {
        "WHAL_DBGID_DEFINITION_START",
        "WHAL_ERROR_ANI_CONTROL",
        "WHAL_ERROR_CHIP_TEST1",
        "WHAL_ERROR_CHIP_TEST2",
        "WHAL_ERROR_EEPROM_CHECKSUM",
        "WHAL_ERROR_EEPROM_MACADDR",
        "WHAL_ERROR_INTERRUPT_HIU",
        "WHAL_ERROR_KEYCACHE_RESET",
        "WHAL_ERROR_KEYCACHE_SET",
        "WHAL_ERROR_KEYCACHE_TYPE",
        "WHAL_ERROR_KEYCACHE_TKIPENTRY",
        "WHAL_ERROR_KEYCACHE_WEPLENGTH",
        "WHAL_ERROR_PHY_INVALID_CHANNEL",
        "WHAL_ERROR_POWER_AWAKE",
        "WHAL_ERROR_POWER_SET",
        "WHAL_ERROR_RECV_STOPDMA",
        "WHAL_ERROR_RECV_STOPPCU",
        "WHAL_ERROR_RESET_CHANNF1",
        "WHAL_ERROR_RESET_CHANNF2",
        "WHAL_ERROR_RESET_PM",
        "WHAL_ERROR_RESET_OFFSETCAL",
        "WHAL_ERROR_RESET_RFGRANT",
        "WHAL_ERROR_RESET_RXFRAME",
        "WHAL_ERROR_RESET_STOPDMA",
        "WHAL_ERROR_RESET_ERRID",
        "WHAL_ERROR_RESET_ADCDCCAL1",
        "WHAL_ERROR_RESET_ADCDCCAL2",
        "WHAL_ERROR_RESET_TXIQCAL",
        "WHAL_ERROR_RESET_RXIQCAL",
        "WHAL_ERROR_RESET_CARRIERLEAK",
        "WHAL_ERROR_XMIT_COMPUTE",
        "WHAL_ERROR_XMIT_NOQUEUE",
        "WHAL_ERROR_XMIT_ACTIVEQUEUE",
        "WHAL_ERROR_XMIT_BADTYPE",
        "WHAL_ERROR_XMIT_STOPDMA",
        "WHAL_ERROR_INTERRUPT_BB_PANIC",
        "WHAL_ERROR_RESET_TXIQCAL",
        "WHAL_ERROR_PAPRD_MAXGAIN_ABOVE_WINDOW",
        "WHAL_COEX_RESET",
        "WHAL_COEX_SELF_GEN_MASK",
        "WHAL_ERROR_COEX_MCI_ISR",
        "WHAL_COEX_MCI_ISR_IntRaw",
        "WHAL_COEX_MCI_ISR_Int1Raw",
        "WHAL_COEX_MCI_ISR_RxMsgRaw",
        "WHAL_COEX_SENDMSG_QUEUE",
        "WHAL_COEX_TX_MCI_REMOTE_RESET",
        "WHAL_COEX_TX_MCI_TYPE_UNKNOWN",
        "WHAL_COEX_TX_MCI_SYS_SLEEPING",
        "WHAL_COEX_TX_MCI_REQ_WAKE",
        "WHAL_COEX_TX_MCI_SYS_WAKING",
        "WHAL_COEX_TX_MCI_LNA_TAKE",
        "WHAL_COEX_TX_MCI_LNA_TRANS",
        "WHAL_COEX_TX_MCI_GPM_UNKNOWN",
        "WHAL_COEX_TX_MCI_GPM_WLAN_SET_ACL_INACTIVITY",
        "WHAL_COEX_TX_MCI_GPM_BT_PAUSE_PROFILE",
        "WHAL_COEX_TX_MCI_GPM_WLAN_PRIO",
        "WHAL_COEX_TX_MCI_GPM_BT_STATUS_UPDATE",
        "WHAL_COEX_TX_MCI_GPM_BT_UPDATE_FLAGS",
        "WHAL_COEX_TX_MCI_GPM_VERSION_QUERY",
        "WHAL_COEX_TX_MCI_GPM_VERSION_RESPONSE",
        "WHAL_COEX_TX_MCI_GPM_STATUS_QUERY",
        "WHAL_COEX_TX_MCI_GPM_HALT_BT_GPM",
        "WHAL_COEX_TX_MCI_GPM_WLAN_CHANNELS",
        "WHAL_COEX_TX_MCI_GPM_BT_PROFILE_INFO",
        "WHAL_COEX_TX_MCI_GPM_BT_CAL_REQ ",
        "WHAL_COEX_TX_MCI_GPM_BT_CAL_GRANT",
        "WHAL_COEX_TX_MCI_GPM_BT_CAL_DONE",
        "WHAL_COEX_TX_MCI_GPM_WLAN_CAL_REQ",
        "WHAL_COEX_TX_MCI_GPM_WLAN_CAL_GRANT",
        "WHAL_COEX_TX_MCI_GPM_WLAN_CAL_DONE",
        "WHAL_COEX_TX_MCI_GPM_BT_DEBUG",
        "WHAL_COEX_WHAL_MCI_RESET",
        "WHAL_COEX_POLL_BT_CAL_DONE_TIMEOUT",
        "WHAL_COEX_WHAL_PAUSE",
        "WHAL_COEX_RX_MCI_GPM_BT_CAL_REQ",
        "WHAL_COEX_RX_MCI_GPM_BT_CAL_DONE",
        "WHAL_COEX_RX_MCI_GPM_BT_CAL_GRANT",
        "WHAL_COEX_WLAN_CAL_START",
        "WHAL_COEX_WLAN_CAL_RESULT ",
        "WHAL_COEX_BtMciState",
        "WHAL_COEX_BtCalState",
        "WHAL_COEX_WlanCalState",
        "WHAL_COEX_RxReqWakeCount",
        "WHAL_COEX_RxRemoteResetCount",
        "WHAL_COEX_RESTART_CAL",
        "WHAL_COEX_WHAL_COEX_RESET",
        "WHAL_COEX_SELF_GEN_MASK",
        "WHAL_DBGID_DEFINITION_END"
    },
    {
        "COEX_DEBUGID_START",
        "BTCOEX_DBG_MCI_1",
        "BTCOEX_DBG_MCI_2",
        "BTCOEX_DBG_MCI_3",
        "BTCOEX_DBG_MCI_4",
        "BTCOEX_DBG_MCI_5",
        "BTCOEX_DBG_MCI_6",
        "BTCOEX_DBG_MCI_7",
        "BTCOEX_DBG_MCI_8",
        "BTCOEX_DBG_MCI_9",
        "BTCOEX_DBG_MCI_10",
        "COEX_WAL_BTCOEX_INIT",
        "COEX_WAL_PAUSE",
        "COEX_WAL_RESUME",
        "COEX_UPDATE_AFH",
        "COEX_HWQ_EMPTY_CB",
        "COEX_MCI_TIMER_HANDLER",
        "COEX_MCI_RECOVER",
        "ERROR_COEX_MCI_ISR",
        "ERROR_COEX_MCI_GPM",
        "COEX_ProfileType",
        "COEX_LinkID",
        "COEX_LinkState",
        "COEX_LinkRole",
        "COEX_LinkRate",
        "COEX_VoiceType",
        "COEX_TInterval",
        "COEX_WRetrx",
        "COEX_Attempts",
        "COEX_PerformanceState",
        "COEX_LinkType",
        "COEX_RX_MCI_GPM_VERSION_QUERY",
        "COEX_RX_MCI_GPM_VERSION_RESPONSE",
        "COEX_RX_MCI_GPM_STATUS_QUERY",
        "COEX_STATE_WLAN_VDEV_DOWN",
        "COEX_STATE_WLAN_VDEV_START",
        "COEX_STATE_WLAN_VDEV_CONNECTED",
        "COEX_STATE_WLAN_VDEV_SCAN_STARTED",
        "COEX_STATE_WLAN_VDEV_SCAN_END",
        "COEX_STATE_WLAN_DEFAULT",
        "COEX_CHANNEL_CHANGE",
        "COEX_POWER_CHANGE",
        "COEX_CONFIG_MGR",
        "COEX_TX_MCI_GPM_BT_CAL_REQ",
        "COEX_TX_MCI_GPM_BT_CAL_GRANT",
        "COEX_TX_MCI_GPM_BT_CAL_DONE",
        "COEX_TX_MCI_GPM_WLAN_CAL_REQ",
        "COEX_TX_MCI_GPM_WLAN_CAL_GRANT",
        "COEX_TX_MCI_GPM_WLAN_CAL_DONE",
        "COEX_TX_MCI_GPM_BT_DEBUG",
        "COEX_TX_MCI_GPM_VERSION_QUERY",
        "COEX_TX_MCI_GPM_VERSION_RESPONSE",
        "COEX_TX_MCI_GPM_STATUS_QUERY",
        "COEX_TX_MCI_GPM_HALT_BT_GPM",
        "COEX_TX_MCI_GPM_WLAN_CHANNELS",
        "COEX_TX_MCI_GPM_BT_PROFILE_INFO",
        "COEX_TX_MCI_GPM_BT_STATUS_UPDATE",
        "COEX_TX_MCI_GPM_BT_UPDATE_FLAGS",
        "COEX_TX_MCI_GPM_UNKNOWN",
        "COEX_TX_MCI_SYS_WAKING",
        "COEX_TX_MCI_LNA_TAKE",
        "COEX_TX_MCI_LNA_TRANS",
        "COEX_TX_MCI_SYS_SLEEPING",
        "COEX_TX_MCI_REQ_WAKE",
        "COEX_TX_MCI_REMOTE_RESET",
        "COEX_TX_MCI_TYPE_UNKNOWN",
        "COEX_WHAL_MCI_RESET",
        "COEX_POLL_BT_CAL_DONE_TIMEOUT",
        "COEX_WHAL_PAUSE",
        "COEX_RX_MCI_GPM_BT_CAL_REQ",
        "COEX_RX_MCI_GPM_BT_CAL_DONE",
        "COEX_RX_MCI_GPM_BT_CAL_GRANT",
        "COEX_WLAN_CAL_START",
        "COEX_WLAN_CAL_RESULT",
        "COEX_BtMciState",
        "COEX_BtCalState",
        "COEX_WlanCalState",
        "COEX_RxReqWakeCount",
        "COEX_RxRemoteResetCount",
        "COEX_RESTART_CAL",
        "COEX_SENDMSG_QUEUE",
        "COEX_RESETSEQ_LNAINFO_TIMEOUT",
        "COEX_MCI_ISR_IntRaw",
        "COEX_MCI_ISR_Int1Raw",
        "COEX_MCI_ISR_RxMsgRaw",
        "COEX_WHAL_COEX_RESET",
        "COEX_WAL_COEX_INIT",
        "COEX_TXRX_CNT_LIMIT_ISR",
        "COEX_CH_BUSY",
        "COEX_REASSESS_WLAN_STATE",
        "COEX_BTCOEX_WLAN_STATE_UPDATE",
        "COEX_BT_NUM_OF_PROFILES",
        "COEX_BT_NUM_OF_HID_PROFILES",
        "COEX_BT_NUM_OF_ACL_PROFILES",
        "COEX_BT_NUM_OF_HI_ACL_PROFILES",
        "COEX_BT_NUM_OF_VOICE_PROFILES",
        "COEX_WLAN_AGGR_LIMIT",
        "COEX_BT_LOW_PRIO_BUDGET",
        "COEX_BT_HI_PRIO_BUDGET",
        "COEX_BT_IDLE_TIME",
        "COEX_SET_COEX_WEIGHT",
        "COEX_WLAN_WEIGHT_GROUP",
        "COEX_BT_WEIGHT_GROUP",
        "COEX_BT_INTERVAL_ALLOC",
        "COEX_BT_SCHEME",
        "COEX_BT_MGR",
        "COEX_BT_SM_ERROR",
        "COEX_SYSTEM_UPDATE",
        "COEX_LOW_PRIO_LIMIT",
        "COEX_HI_PRIO_LIMIT",
        "COEX_BT_INTERVAL_START",
        "COEX_WLAN_INTERVAL_START",
        "COEX_NON_LINK_BUDGET",
        "COEX_CONTENTION_MSG",
        "COEX_SET_NSS",
        "COEX_SELF_GEN_MASK",
        "COEX_PROFILE_ERROR",
        "COEX_WLAN_INIT",
        "COEX_BEACON_MISS",
        "COEX_BEACON_OK",
        "COEX_BTCOEX_SCAN_ACTIVITY",
        "COEX_SCAN_ACTIVITY",
        "COEX_FORCE_QUIETTIME",
        "COEX_BT_MGR_QUIETTIME",
        "COEX_BT_INACTIVITY_TRIGGER",
        "COEX_BT_INACTIVITY_REPORTED",
        "COEX_TX_MCI_GPM_WLAN_PRIO",
        "COEX_TX_MCI_GPM_BT_PAUSE_PROFILE",
        "COEX_TX_MCI_GPM_WLAN_SET_ACL_INACTIVITY",
        "COEX_RX_MCI_GPM_BT_ACL_INACTIVITY_REPORT",
        "COEX_GENERIC_ERROR",
        "COEX_RX_RATE_THRESHOLD",
        "COEX_RSSI",
        "COEX_WLAN_VDEV_NOTIF_START", //                 133
        "COEX_WLAN_VDEV_NOTIF_UP", //                    134
        "COEX_WLAN_VDEV_NOTIF_DOWN",   //                135
        "COEX_WLAN_VDEV_NOTIF_STOP",    //               136
        "COEX_WLAN_VDEV_NOTIF_ADD_PEER",    //           137
        "COEX_WLAN_VDEV_NOTIF_DELETE_PEER",  //          138
        "COEX_WLAN_VDEV_NOTIF_CONNECTED_PEER",  //       139
        "COEX_WLAN_VDEV_NOTIF_PAUSE",   //        140
        "COEX_WLAN_VDEV_NOTIF_UNPAUSED",    //        141   
        "COEX_STATE_WLAN_VDEV_PEER_ADD",    //           142
        "COEX_STATE_WLAN_VDEV_CONNECTED_PEER",    //     143
        "COEX_STATE_WLAN_VDEV_DELETE_PEER",  //          144
        "COEX_STATE_WLAN_VDEV_PAUSE",  //         145
        "COEX_STATE_WLAN_VDEV_UNPAUSED",    //        146
        "COEX_SCAN_CALLBACK",           //    147
        "COEX_DEBUG_MESSAGE_END"
    },
    {
        "RO_DBGID_DEFINITION_START",
        "RO_REFRESH_ROAM_TABLE",
        "RO_UPDATE_ROAM_CANDIDATE",
        "RO_UPDATE_ROAM_CANDIDATE_CB",
        "RO_UPDATE_ROAM_CANDIDATE_FINISH",
        "RO_REFRESH_ROAM_TABLE_DONE",
        "RO_PERIODIC_SEARCH_CB",
        "RO_PERIODIC_SEARCH_TIMEOUT",
        "RO_INIT",
        "RO_BMISS_STATE1",
        "RO_BMISS_STATE2",
        "RO_SET_PERIODIC_SEARCH_ENABLE",
        "RO_SET_PERIODIC_SEARCH_DISABLE",
        "RO_ENABLE_SQ_THRESHOLD",
        "RO_DISABLE_SQ_THRESHOLD",
        "RO_ADD_BSS_TO_ROAM_TABLE",
        "RO_SET_PERIODIC_SEARCH_MODE",
        "RO_CONFIGURE_SQ_THRESHOLD1",
        "RO_CONFIGURE_SQ_THRESHOLD2",
        "RO_CONFIGURE_SQ_PARAMS",
        "RO_LOW_SIGNAL_QUALITY_EVENT",
        "RO_HIGH_SIGNAL_QUALITY_EVENT",
        "RO_REMOVE_BSS_FROM_ROAM_TABLE",
        "RO_UPDATE_CONNECTION_STATE_METRIC",
        "RO_LOWRSSI_SCAN_PARAMS",
        "RO_LOWRSSI_SCAN_START",
        "RO_LOWRSSI_SCAN_END",
        "RO_LOWRSSI_SCAN_CANCEL",
        "RO_LOWRSSI_ROAM_CANCEL",
        "RO_REFRESH_ROAM_CANDIDATE",
        "RO_DBGID_DEFINITION_END"
    },
    {
        "RESMGR_CHMGR_DEFINITION_START",
        "RESMGR_CHMGR_PAUSE_COMPLETE",
        "RESMGR_CHMGR_CHANNEL_CHANGE",
        "RESMGR_CHMGR_RESUME_COMPLETE",
        "RESMGR_CHMGR_VDEV_PAUSE",
        "RESMGR_CHMGR_VDEV_UNPAUSE",
        "RESMGR_CHMGR_DEFINITION_END"
    },
    {
        "RESMGR_OCS_DEFINITION_START",
        "RESMGR_OCS_ALLOCRAM_SIZE",
        "RESMGR_OCS_RESOURCES",
        "RESMGR_OCS_LINK_CREATE",
        "RESMGR_OCS_LINK_DELETE",
        "RESMGR_OCS_CHREQ_CREATE",
        "RESMGR_OCS_CHREQ_DELETE",
        "RESMGR_OCS_CHREQ_START",
        "RESMGR_OCS_CHREQ_STOP",
        "RESMGR_OCS_SCHEDULER_INVOKED",
        "RESMGR_OCS_CHREQ_GRANT",
        "RESMGR_OCS_CHREQ_COMPLETE",
        "RESMGR_OCS_NEXT_TSFTIME",
        "RESMGR_OCS_TSF_TIMEOUT_US",
        "RESMGR_OCS_CURR_CAT_WINDOW",
        "RESMGR_OCS_CURR_CAT_WINDOW_REQ",
        "RESMGR_OCS_CURR_CAT_WINDOW_TIMESLOT",
        "RESMGR_OCS_CHREQ_RESTART",
        "RESMGR_OCS_CLEANUP_CH_ALLOCATORS",
        "RESMGR_OCS_PURGE_CHREQ",
        "RESMGR_OCS_CH_ALLOCATOR_FREE",
        "RESMGR_OCS_RECOMPUTE_SCHEDULE",
        "RESMGR_OCS_NEW_CAT_WINDOW_REQ",
        "RESMGR_OCS_NEW_CAT_WINDOW_TIMESLOT",
        "RESMGR_OCS_CUR_CH_ALLOC",
        "RESMGR_OCS_WIN_CH_ALLOC",
        "RESMGR_OCS_SCHED_CH_CHANGE",
        "RESMGR_OCS_CONSTRUCT_CAT_WIN",
        "RESMGR_OCS_CHREQ_PREEMPTED",
        "RESMGR_OCS_CH_SWITCH_REQ",
        "RESMGR_OCS_CHANNEL_SWITCHED",
        "RESMGR_OCS_CLEANUP_STALE_REQS",
        "RESMGR_OCS_DEFINITION_END"
    },
    {   
        "VDEV_MGR_DEBID_DEFINITION_START", /* vdev Mgr */
        "VDEV_MGR_BEACON_MISS_TIMER_TIMEOUT",
        "VDEV_MGR_BEACON_MISS_DETECTED",
        "VDEV_MGR_BEACON_IN_SYNC",
        "VDEV_MGR_AP_KEEPALIVE_IDLE",
        "VDEV_MGR_AP_KEEPALIVE_INACTIVE",
        "VDEV_MGR_AP_KEEPALIVE_UNRESPONSIVE",
    },
    { 
      "SCAN_START_COMMAND_FAILED", /* scan */
      "SCAN_STOP_COMMAND_FAILED", 
      "SCAN_EVENT_SEND_FAILED",
    },
    { "" /* Rate ctrl*/
    },
    { 
        "AP_PS_DBGID_DEFINITION_START",
        "AP_PS_DBGID_UPDATE_TIM",
        "AP_PS_DBGID_PEER_STATE_CHANGE",
        "AP_PS_DBGID_PSPOLL",
        "AP_PS_DBGID_PEER_CREATE",
        "AP_PS_DBGID_PEER_DELETE",
        "AP_PS_DBGID_VDEV_CREATE",
        "AP_PS_DBGID_VDEV_DELETE",
        "AP_PS_DBGID_SYNC_TIM",
        "AP_PS_DBGID_NEXT_RESPONSE",
        "AP_PS_DBGID_START_SP",
        "AP_PS_DBGID_COMPLETED_EOSP",
        "AP_PS_DBGID_TRIGGER",
        "AP_PS_DBGID_DUPLICATE_TRIGGER",
        "AP_PS_DBGID_UAPSD_RESPONSE",
        "AP_PS_DBGID_SEND_COMPLETE",
        "AP_PS_DBGID_SEND_N_COMPLETE",
    },
    {
        "" /* Block Ack */
    },
    {
        "" /* Mgmt TxRx */
    },
    { "" /* Data TxRx */
    },
    { "" /* HTT */
    },
    { "" /* HOST */
    },
    { "" /* BEACON */
      "BEACON_EVENT_SWBA_SEND_FAILED",
    },
    { /* Offload Mgr */
        "OFFLOAD_MGR_DBGID_DEFINITION_START",
        "OFFLOADMGR_REGISTER_OFFLOAD",
        "OFFLOADMGR_DEREGISTER_OFFLOAD",
        "OFFLOADMGR_NO_REG_DATA_HANDLERS",
        "OFFLOADMGR_NO_REG_EVENT_HANDLERS",
        "OFFLOADMGR_REG_OFFLOAD_FAILED",
        "OFFLOADMGR_DBGID_DEFINITION_END",
    },
    { 
        "WAL_DBGID_DEFINITION_START",
        "WAL_DBGID_FAST_WAKE_REQUEST",
        "WAL_DBGID_FAST_WAKE_RELEASE",
        "WAL_DBGID_SET_POWER_STATE",
        "WAL_DBGID_MISSING",
        "WAL_DBGID_CHANNEL_CHANGE_FORCE_RESET",
        "WAL_DBGID_CHANNEL_CHANGE",
        "WAL_DBGID_VDEV_START",
        "WAL_DBGID_VDEV_STOP",
        "WAL_DBGID_VDEV_UP",
        "WAL_DBGID_VDEV_DOWN",
        "WAL_DBGID_SW_WDOG_RESET",
        "WAL_DBGID_TX_SCH_REGISTER_TIDQ",
        "WAL_DBGID_TX_SCH_UNREGISTER_TIDQ",
        "WAL_DBGID_TX_SCH_TICKLE_TIDQ",
        "WAL_DBGID_XCESS_FAILURES",
        "WAL_DBGID_AST_ADD_WDS_ENTRY",
        "WAL_DBGID_AST_DEL_WDS_ENTRY",
        "WAL_DBGID_AST_WDS_ENTRY_PEER_CHG",
        "WAL_DBGID_AST_WDS_SRC_LEARN_FAIL",
        "WAL_DBGID_STA_KICKOUT",
        "WAL_DBGID_BAR_TX_FAIL",
        "WAL_DBGID_BAR_ALLOC_FAIL",
        "WAL_DBGID_LOCAL_DATA_TX_FAIL",
        "WAL_DBGID_SECURITY_PM4_QUEUED",
        "WAL_DBGID_SECURITY_GM1_QUEUED",
        "WAL_DBGID_SECURITY_PM4_SENT",
        "WAL_DBGID_SECURITY_ALLOW_DATA",
        "WAL_DBGID_SECURITY_UCAST_KEY_SET",
        "WAL_DBGID_SECURITY_MCAST_KEY_SET",
        "WAL_DBGID_SECURITY_ENCR_EN",
        "WAL_DBGID_BB_WDOG_TRIGGERED",
        "WAL_DBGID_RX_LOCAL_BUFS_LWM",
        "WAL_DBGID_RX_LOCAL_DROP_LARGE_MGMT",
        "WAL_DBGID_VHT_ILLEGAL_RATE_PHY_ERR_DETECTED",
        "WAL_DBGID_DEV_RESET",
        "WAL_DBGID_TX_BA_SETUP",
        "WAL_DBGID_RX_BA_SETUP",
        "WAL_DBGID_DEV_TX_TIMEOUT",                    
        "WAL_DBGID_DEV_RX_TIMEOUT",                    
        "WAL_DBGID_STA_VDEV_XRETRY",                   
        "WAL_DBGID_DCS",
        "WAL_DBGID_DEFINITION_END",
    },
    {
        "" /* DE */
    },
    {
        "" /* pcie lp */
    },
    {
        "" /* RTT */
    },
    {      /* RESOURCE */
        "RESOURCE_DBGID_DEFINITION_START",
        "RESOURCE_PEER_ALLOC",
        "RESOURCE_PEER_FREE",
        "RESOURCE_PEER_ALLOC_WAL_PEER",
        "RESOURCE_DBGID_DEFINITION_END",
    }, 
    { /* DCS */
        "WLAN_DCS_DBGID_INIT",
        "WLAN_DCS_DBGID_WMI_CWINT",
        "WLAN_DCS_DBGID_TIMER",
        "WLAN_DCS_DBGID_CMDG",
        "WLAN_DCS_DBGID_CMDS",
        "WLAN_DCS_DBGID_DINIT"
    },
    {   /* ANI  */
        ""
    }
};

void dbglog_module_log_enable(wmi_unified_t  wmi_handle, A_UINT32 mod_id, 
                  bool isenable)
{
    struct dbglog_config_msg_s configmsg;

    if (mod_id > WLAN_MODULE_ID_MAX) {
        printk("dbglog_module_log_enable: Invalid module id %d\n",
                mod_id);
        return;
    }
    
    OS_MEMSET(&configmsg, 0, sizeof(struct dbglog_config_msg_s));

    if (isenable)
        DBGLOG_MODULE_ENABLE(configmsg.config.mod_id, mod_id);

    configmsg.cfgvalid[mod_id/32] = (1 << (mod_id % 32)); 
    printk("cfg valid value inside module enable %0x",configmsg.cfgvalid[0]); 
    wmi_config_debug_module_cmd(wmi_handle, &configmsg);
}

void dbglog_vap_log_enable(wmi_unified_t  wmi_handle, A_UINT16 vap_id,
               bool isenable)
{
    struct dbglog_config_msg_s configmsg;
    
    if (vap_id > DBGLOG_MAX_VAPID) {
        printk("dbglog_vap_log_enable:Invalid vap_id %d\n",
        vap_id);
        return;
    }

    OS_MEMSET(&configmsg, 0, sizeof(struct dbglog_config_msg_s));
    
    if (isenable)
        DBGLOG_VAP_ENABLE(configmsg.config.dbg_config, vap_id);
    
    configmsg.cfgvalid[DBGLOG_MODULE_BITMAP_SIZE] = (1 << (vap_id + 
                             DBGLOG_VAP_LOG_ENABLE_OFFSET));
    wmi_config_debug_module_cmd(wmi_handle, &configmsg);

}

void dbglog_set_log_lvl(wmi_unified_t  wmi_handle, DBGLOG_LOG_LVL log_lvl)
{
    struct dbglog_config_msg_s configmsg;
    
    OS_MEMSET(&configmsg, 0, sizeof(struct dbglog_config_msg_s));
    DBGLOG_LOG_LVL_ENABLE(configmsg.config.dbg_config, log_lvl);
    configmsg.cfgvalid[DBGLOG_MODULE_BITMAP_SIZE] = DBGLOG_LOG_LVL_ENABLE_MASK;
    wmi_config_debug_module_cmd(wmi_handle, &configmsg);
}

void dbglog_reporting_enable(wmi_unified_t  wmi_handle, bool isenable)
{
    struct dbglog_config_msg_s configmsg;
    
    OS_MEMSET(&configmsg, 0, sizeof(struct dbglog_config_msg_s));
     
     if (isenable)
        DBGLOG_REPORTING_ENABLE(configmsg.config.dbg_config);

    configmsg.cfgvalid[DBGLOG_MODULE_BITMAP_SIZE] = DBGLOG_REPORTING_ENABLE_MASK;
    wmi_config_debug_module_cmd(wmi_handle, &configmsg);
}

void dbglog_set_timestamp_resolution(wmi_unified_t  wmi_handle, A_UINT16 tsr)
{
    struct dbglog_config_msg_s configmsg;

    OS_MEMSET(&configmsg, 0, sizeof(struct dbglog_config_msg_s));
    DBGLOG_TIMESTAMP_RES_SET(configmsg.config.dbg_config, tsr);
    configmsg.cfgvalid[DBGLOG_MODULE_BITMAP_SIZE] = DBGLOG_TIMESTAMP_RESOLUTION_MASK;
    wmi_config_debug_module_cmd(wmi_handle, &configmsg);
}

void dbglog_set_report_size(wmi_unified_t  wmi_handle, A_UINT16 size)
{
    struct dbglog_config_msg_s configmsg;
    
    OS_MEMSET(&configmsg, 0, sizeof(struct dbglog_config_msg_s));
    DBGLOG_REPORT_SIZE_SET(configmsg.config.dbg_config, size);
    configmsg.cfgvalid[DBGLOG_MODULE_BITMAP_SIZE] = DBGLOG_REPORT_SIZE_MASK;
    wmi_config_debug_module_cmd(wmi_handle, &configmsg);
}

A_STATUS 
wmi_config_debug_module_cmd(wmi_unified_t  wmi_handle, struct dbglog_config_msg_s *configmsg)
{
    wmi_buf_t osbuf;
    WMI_DBGLOG_CFG_CMD *cmd;
    A_STATUS status;

    osbuf = wmi_buf_alloc(wmi_handle, sizeof(*cmd));
    if (osbuf == NULL) {
        return A_NO_MEMORY;
    }

    wbuf_append(osbuf, sizeof(*cmd));

    cmd = (WMI_DBGLOG_CFG_CMD *)(wbuf_header(osbuf));
   
    printk("wmi_dbg_cfg_send: mod[0]%08x dbgcfg%08x cfgvalid[0] %08x"
            " cfgvalid[1] %08x\n",configmsg->config.mod_id[0], 
        configmsg->config.dbg_config, configmsg->cfgvalid[0], 
        configmsg->cfgvalid[1]);

    OS_MEMCPY(&cmd->config, configmsg, sizeof(struct dbglog_config_msg_s));

    status = wmi_unified_cmd_send(wmi_handle, osbuf, 
                   sizeof(WMI_DBGLOG_CFG_CMD),
                   WMI_DBGLOG_CFG_CMDID);

    if (status != A_OK) {
        wbuf_free(osbuf);
    }

    return status;
}

static char *
dbglog_get_msg(A_UINT32 moduleid, A_UINT32 debugid) 
{
    static char unknown_str[64];

    if (moduleid < WLAN_MODULE_ID_MAX && debugid < MAX_DBG_MSGS) {
        char *str = DBG_MSG_ARR[moduleid][debugid];
        if (str && str[0] != '\0') {
            return str;
        }
    }

    snprintf(unknown_str, sizeof(unknown_str), 
            "UNKNOWN %u:%u", 
            moduleid, debugid);

    return unknown_str;
}

void
dbglog_printf(
        A_UINT32 timestamp, 
        A_UINT16 vap_id, 
        const char *fmt, ...)
{
    char buf[512];
    va_list ap;

    if (vap_id < DBGLOG_VAPID_NUM_MAX) {
        if (dbglog_prt_path == DBGLOG_PRT_WMI) {
            printk(DBGLOG_PRINT_PREFIX "[%u] vap-%u ", timestamp, vap_id);
        } else {
            sprintf(buf, DBGLOG_PRINT_PREFIX "[%u] vap-%u ", timestamp, vap_id);
#ifndef REMOVE_PKT_LOG
            strcat(dbglog_print_buffer, buf);
#endif
        }
    } else {
        if (dbglog_prt_path == DBGLOG_PRT_WMI) {
            printk(DBGLOG_PRINT_PREFIX "[%u] ", timestamp);
        } else {
            sprintf(buf, DBGLOG_PRINT_PREFIX "[%u] ", timestamp);
#ifndef REMOVE_PKT_LOG
            strcat(dbglog_print_buffer, buf);
#endif
        }
    }

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    printk("%s\n", buf);
    if (dbglog_prt_path == DBGLOG_PRT_PKTLOG) {
#ifndef REMOVE_PKT_LOG
        strcat(dbglog_print_buffer, buf);
#endif
    }
}

#define USE_NUMERIC 0

A_BOOL
dbglog_default_print_handler(A_UINT32 mod_id, A_UINT16 vap_id,
                            A_UINT32 dbg_id, A_UINT32 timestamp,
                            A_UINT16 numargs, A_UINT32 *args)
{
    int i;
    char buf[512];

    if (vap_id < DBGLOG_VAPID_NUM_MAX) {
        if (dbglog_prt_path == DBGLOG_PRT_WMI) {
            printk(DBGLOG_PRINT_PREFIX "[%u] vap-%u %s ( ", timestamp, vap_id, dbglog_get_msg(mod_id, dbg_id));
        } else {
            sprintf(buf, DBGLOG_PRINT_PREFIX "[%u] vap-%u %s ( ", timestamp, vap_id, dbglog_get_msg(mod_id, dbg_id));
#ifndef REMOVE_PKT_LOG
            strcat(dbglog_print_buffer, buf);
#endif
        }
    } else {
        if (dbglog_prt_path == DBGLOG_PRT_WMI) {
            printk(DBGLOG_PRINT_PREFIX "[%u] %s ( ", timestamp, dbglog_get_msg(mod_id, dbg_id));
        } else {
            sprintf(buf, DBGLOG_PRINT_PREFIX "[%u] %s ( ", timestamp, dbglog_get_msg(mod_id, dbg_id));
#ifndef REMOVE_PKT_LOG
            strcat(dbglog_print_buffer, buf);
#endif
        }
    }

    for (i = 0; i < numargs; i++) {
#if USE_NUMERIC
        if (dbglog_prt_path == DBGLOG_PRT_WMI) {
            printk("%u", args[i]);
        } else {
            sprintf(buf, "%u", args[i]);
#ifndef REMOVE_PKT_LOG
            strcat(dbglog_print_buffer, buf);
#endif
        }
#else
        if (dbglog_prt_path == DBGLOG_PRT_WMI) {
            printk("%#x", args[i]);
        } else {
            sprintf(buf, "%#x", args[i]);
#ifndef REMOVE_PKT_LOG
            strcat(dbglog_print_buffer, buf);
#endif
        }
#endif
        if ((i + 1) < numargs) {
            if (dbglog_prt_path == DBGLOG_PRT_WMI) {
                printk(", ");
            } else {
                sprintf(buf, ", ");
#ifndef REMOVE_PKT_LOG
                strcat(dbglog_print_buffer, buf);
#endif
            }
        }
    }

    if (dbglog_prt_path == DBGLOG_PRT_WMI) {
        printk(" )\n");
    } else {
        sprintf(buf, " )\n");
#ifndef REMOVE_PKT_LOG
        strcat(dbglog_print_buffer, buf);
#endif
    }

    return TRUE;
}

int
dbglog_parse_debug_logs(ol_scn_t scn, u_int8_t *datap, u_int16_t len, void *context)
{
    A_UINT32 count;
    A_UINT32 *buffer;
    A_UINT32 timestamp;
    A_UINT32 debugid;
    A_UINT32 moduleid;
    A_UINT16 vapid;
    A_UINT16 numargs;
    A_UINT16 length;
    A_UINT32 dropped;
    char buf[512];

    dbglog_prt_path = (dbglog_prt_path_t) context;
    dropped = *((A_UINT32 *)datap);
    datap += sizeof(dropped);
    len -= sizeof(dropped);
    if (dropped > 0) {
        if (dbglog_prt_path == DBGLOG_PRT_WMI) {
            printk(DBGLOG_PRINT_PREFIX "%d log buffers are dropped \n",
                                                                dropped);
        } else {
            sprintf(buf, DBGLOG_PRINT_PREFIX "%d log buffers are dropped \n",
                                                                    dropped);
#ifndef REMOVE_PKT_LOG
            strcat(dbglog_print_buffer, buf);
#endif
        }
    }

    count = 0;
    buffer = (A_UINT32 *)datap;
    length = (len >> 2);
    while (count < length) {
        debugid = DBGLOG_GET_DBGID(buffer[count + 1]);
        moduleid = DBGLOG_GET_MODULEID(buffer[count + 1]);
        vapid = DBGLOG_GET_VAPID(buffer[count + 1]);
        numargs = DBGLOG_GET_NUMARGS(buffer[count + 1]);
        timestamp = DBGLOG_GET_TIME_STAMP(buffer[count]);

        if (moduleid >= WLAN_MODULE_ID_MAX)
            return 0; 
        
        if (mod_print[moduleid] == NULL) {
            /* No module specific log registered use the default handler*/
            dbglog_default_print_handler(moduleid, vapid, debugid, timestamp, 
                                         numargs, 
                                         (((A_UINT32 *)buffer) + 2 + count));
        } else {
            if(!(mod_print[moduleid](moduleid, vapid, debugid, timestamp, 
                            numargs, 
                            (((A_UINT32 *)buffer) + 2 + count)))) {
                /* The message is not handled by the module specific handler*/
                dbglog_default_print_handler(moduleid, vapid, debugid, timestamp, 
                        numargs, 
                        (((A_UINT32 *)buffer) + 2 + count));

            }
        }

        count += numargs + 2; /* 32 bit Time stamp + 32 bit Dbg header*/
    }
    /* Always returns zero */
    return (0);
}

void
dbglog_reg_modprint(A_UINT32 mod_id, module_dbg_print printfn)
{
    if (!mod_print[mod_id]) {
        mod_print[mod_id] = printfn;
    } else {
        printk("module print is already registered for thsi module %d\n",
               mod_id);
    }
}

void 
dbglog_sm_print(
        A_UINT32 timestamp, 
        A_UINT16 vap_id, 
        A_UINT16 numargs, 
        A_UINT32 *args, 
        const char *module_prefix,
        const char *states[], A_UINT32 num_states,
        const char *events[], A_UINT32 num_events)
{
    A_UINT8 type, arg1, arg2, arg3;
    A_UINT32 extra;

    if (numargs != 2) {
        return;
    }

    type = (args[0] >> 24) & 0xff;
    arg1 = (args[0] >> 16) & 0xff;
    arg2 = (args[0] >>  8) & 0xff;
    arg3 = (args[0] >>  0) & 0xff;

    extra = args[1];

    switch (type) {
    case 0: /* state transition */
        if (arg1 < num_states && arg2 < num_states) {
            dbglog_printf(timestamp, vap_id, "%s: %s => %s (%#x)",
                    module_prefix, states[arg1], states[arg2], extra);
        } else {
            dbglog_printf(timestamp, vap_id, "%s: %u => %u (%#x)", 
                    module_prefix, arg1, arg2, extra);
        }
        break;
    case 1: /* dispatch event */
        if (arg1 < num_states && arg2 < num_events) {
            dbglog_printf(timestamp, vap_id, "%s: %s < %s (%#x)",
                    module_prefix, states[arg1], events[arg2], extra);
        } else {
            dbglog_printf(timestamp, vap_id, "%s: %u < %u (%#x)",
                    module_prefix, arg1, arg2, extra);
        }
        break;
    case 2: /* warning */
        switch (arg1) {
        case 0: /* unhandled event */
            if (arg2 < num_states && arg3 < num_events) {
                dbglog_printf(timestamp, vap_id, "%s: unhandled event %s in state %s (%#x)",
                        module_prefix, events[arg3], states[arg2], extra);
            } else {
                dbglog_printf(timestamp, vap_id, "%s: unhandled event %u in state %u (%#x)",
                        module_prefix, arg3, arg2, extra);
            }
            break;
        default:
            break;

        }
        break;
    }
}

A_BOOL
dbglog_sta_powersave_print_handler(
        A_UINT32 mod_id, 
        A_UINT16 vap_id, 
        A_UINT32 dbg_id,
        A_UINT32 timestamp, 
        A_UINT16 numargs, 
        A_UINT32 *args)
{
    static const char *states[] = {
        "IDLE",
        "ACTIVE",
        "SLEEP_TXQ_FLUSH",
        "SLEEP_TX_SENT",
        "PAUSE",
        "SLEEP_DOZE",
        "SLEEP_AWAKE",
        "ACTIVE_TXQ_FLUSH",
        "ACTIVE_TX_SENT",
        "PAUSE_TXQ_FLUSH",
        "PAUSE_TX_SENT",
        "IDLE_TXQ_FLUSH",
        "IDLE_TX_SENT",
    };

    static const char *events[] = {
        "START",
        "STOP",
        "PAUSE",
        "UNPAUSE",
        "TIM",
        "DTIM",
        "SEND_COMPLETE",
        "PRE_SEND",
        "RX",
        "HWQ_EMPTY",
        "PAUSE_TIMEOUT",
        "TXRX_INACTIVITY_TIMEOUT",
        "PSPOLL_TIMEOUT",
        "UAPSD_TIMEOUT",
        "DELAYED_SLEEP_TIMEOUT",
        "SEND_N_COMPLETE",
        "TIDQ_PAUSE_COMPLETE",
    };

    switch (dbg_id) {
    case DBGLOG_DBGID_SM_FRAMEWORK_PROXY_DBGLOG_MSG:
        dbglog_sm_print(timestamp, vap_id, numargs, args, "STA PS",
                states, ARRAY_LENGTH(states), events, ARRAY_LENGTH(events));
        break;
    case PS_STA_PM_ARB_REQUEST:
        if (numargs == 4) {
            dbglog_printf(timestamp, vap_id, "PM ARB request flags=%x, last_time=%x %s: %s", 
                    args[1], args[2], dbglog_get_module_str(args[0]), args[3] ? "SLEEP" : "WAKE");
        }
        break;
    case PS_STA_DELIVER_EVENT:
        if (numargs == 2) {
            dbglog_printf(timestamp, vap_id, "STA PS: %s %u", 
                    (args[0] == 0 ? "PAUSE_COMPLETE" :  
                    (args[0] == 1 ? "UNPAUSE_COMPLETE" :  
                    (args[0] == 2 ? "SLEEP" :  
                    (args[0] == 3 ? "AWAKE" : "UNKNOWN")))),
                    args[1]);
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

A_BOOL dbglog_ratectrl_print_handler(
        A_UINT32 mod_id, 
        A_UINT16 vap_id, 
        A_UINT32 dbg_id,
        A_UINT32 timestamp, 
        A_UINT16 numargs, 
        A_UINT32 *args)
{
    switch (dbg_id) {
        case RATECTRL_DBGID_ASSOC:
           dbglog_printf(timestamp, vap_id, "RATE: ChainMask %d, phymode %d, ni_flags 0x%08x, vht_mcs_set 0x%04x, ht_mcs_set 0x%04x",
              args[0], args[1], args[2], args[3], args[4]);
           break;
        case RATECTRL_DBGID_NSS_CHANGE:
           dbglog_printf(timestamp, vap_id, "RATE: NEW NSS %d\n", args[0]);
           break;
        case RATECTRL_DBGID_CHAINMASK_ERR:
           dbglog_printf(timestamp, vap_id, "RATE: Chainmask ERR %d %d %d\n",
               args[0], args[1], args[2]);
           break;
        case RATECTRL_DBGID_UNEXPECTED_FRAME:
           dbglog_printf(timestamp, vap_id, "RATE: WARN1: rate %d flags 0x%08x\n", args[0], args[1]);
           break;
        case RATECTRL_DBGID_WAL_RCQUERY:
            dbglog_printf(timestamp, vap_id, "ratectrl_dbgid_wal_rcquery [rix1 %d rix2 %d rix3 %d proberix %d ppduflag 0x%x] ",
                    args[0], args[1], args[2], args[3], args[4]);
            break;
        case RATECTRL_DBGID_WAL_RCUPDATE:
            dbglog_printf(timestamp, vap_id, "ratectrl_dbgid_wal_rcupdate [numelems %d ppduflag 0x%x] ",
                    args[0], args[1]);
    }
    return TRUE;
}

A_BOOL dbglog_dcs_print_handler(
        A_UINT32 mod_id,
        A_UINT16 vap_id,
        A_UINT32 dbg_id,
        A_UINT32 timestamp,
        A_UINT16 numargs,
        A_UINT32 *args)
{
  switch (dbg_id) {
   case WLAN_DCS_DBGID_INIT:
        dbglog_printf(timestamp, vap_id,
        "DCS init:  %d", args[0]);
        break;
   case WLAN_DCS_DBGID_WMI_CWINT:
        dbglog_printf(timestamp, vap_id,
        "DCS wmi cwinit:  %d", args[0]);
        break;
   case WLAN_DCS_DBGID_TIMER:
        dbglog_printf(timestamp, vap_id,
        "DCS timer:  %d", args[0]);
        break;
   case WLAN_DCS_DBGID_CMDG:
        dbglog_printf(timestamp, vap_id,
        "DCS cmdg:  %d", args[0]);
        break;
   case WLAN_DCS_DBGID_CMDS:
        dbglog_printf(timestamp, vap_id,
        "DCS cmds:  %d", args[0]);
        break;
   case WLAN_DCS_DBGID_DINIT:
        dbglog_printf(timestamp, vap_id,
        "DCS dinit:  %d", args[0]);
        break;
   default:
        dbglog_printf(timestamp, vap_id,
        "DCS arg %d", args[0]);
        break;
  }
    return TRUE;
}

A_BOOL dbglog_ani_print_handler(
        A_UINT32 mod_id,
        A_UINT16 vap_id,
        A_UINT32 dbg_id,
        A_UINT32 timestamp,
        A_UINT16 numargs,
        A_UINT32 *args)
{
  switch (dbg_id) {
   case ANI_DBGID_ENABLE:
        dbglog_printf(timestamp, vap_id,
        "ANI Enable:  %d", args[0]);
        break;
   case ANI_DBGID_POLL:
        dbglog_printf(timestamp, vap_id,
        "ANI POLLING: AccumListenTime %d ListenTime %d ofdmphyerr %d cckphyerr %d",
                args[0], args[1], args[2],args[3]);
        break;
   case ANI_DBGID_CURRENT_LEVEL:
        dbglog_printf(timestamp, vap_id,
        "ANI CURRENT LEVEL: ofdm level %d cck level %d",
                args[0], args[1]);
        break;

   case ANI_DBGID_RESTART:
        dbglog_printf(timestamp, vap_id,
        "ANI RESTART: AccumListenTime %d ListenTime %d ofdmphyerr %d cckphyerr %d",
                args[0], args[1], args[2],args[3]);
        break;
   case ANI_DBGID_OFDM_LEVEL:
        dbglog_printf(timestamp, vap_id,
        "ANI UPDATE ofdm level %d firstep %d firstep_low %d cycpwr_thr %d self_corr_low %d",
        args[0], args[1],args[2],args[3],args[4]);
        break;
   case ANI_DBGID_CCK_LEVEL:
        dbglog_printf(timestamp, vap_id,
                "ANI UPDATE  cck level %d firstep %d firstep_low %d mrc_cck %d",
                args[0],args[1],args[2],args[3]);
        break;
   case ANI_DBGID_CONTROL:
        dbglog_printf(timestamp, vap_id,
                "ANI CONTROL ofdmlevel %d ccklevel %d\n",
                args[0]);
        break;
   case ANI_DBGID_OFDM_PARAMS:
        dbglog_printf(timestamp, vap_id,
                "ANI ofdm_control firstep %d cycpwr %d\n",
                args[0],args[1]);
        break;
   case ANI_DBGID_CCK_PARAMS:
        dbglog_printf(timestamp, vap_id,
                "ANI cck_control mrc_cck %d barker_threshold %d\n",
                args[0],args[1]);
        break;
   case ANI_DBGID_RESET:
        dbglog_printf(timestamp, vap_id,
                "ANI resetting resetflag %d resetCause %8x channel index %d",
                args[0],args[1],args[2]);
        break;
   case ANI_DBGID_SELF_CORR_LOW:
        dbglog_printf(timestamp, vap_id,"ANI self_corr_low %d",args[0]);
        break;
   case ANI_DBGID_FIRSTEP:
        dbglog_printf(timestamp, vap_id,"ANI firstep %d firstep_low %d",
            args[0],args[1]);
        break;
   case ANI_DBGID_MRC_CCK:
        dbglog_printf(timestamp, vap_id,"ANI mrc_cck %d",args[0]);
        break;
   case ANI_DBGID_CYCPWR:
        dbglog_printf(timestamp, vap_id,"ANI cypwr_thresh %d",args[0]);
        break;
   case ANI_DBGID_POLL_PERIOD:
        dbglog_printf(timestamp, vap_id,"ANI Configure poll period to %d",args[0]);
        break;
   case ANI_DBGID_LISTEN_PERIOD:
        dbglog_printf(timestamp, vap_id,"ANI Configure listen period to %d",args[0]);
        break;

   case ANI_DBGID_OFDM_LEVEL_CFG:
        dbglog_printf(timestamp, vap_id,"ANI Configure ofdm level to %d",args[0]);
        break;

   case ANI_DBGID_CCK_LEVEL_CFG:
        dbglog_printf(timestamp, vap_id,"ANI Configure cck level to %d",args[0]);
        break;

   default:
        dbglog_printf(timestamp, vap_id,"ANI arg1 %d arg2 %d arg3 %d",
              args[0],args[1],args[2]);
        break;
  }
    return TRUE;
}

A_BOOL
dbglog_ap_powersave_print_handler(
        A_UINT32 mod_id, 
        A_UINT16 vap_id, 
        A_UINT32 dbg_id,
        A_UINT32 timestamp, 
        A_UINT16 numargs, 
        A_UINT32 *args)
{
    switch (dbg_id) {
    case AP_PS_DBGID_UPDATE_TIM:
        if (numargs == 2) {
            dbglog_printf(timestamp, vap_id, 
                    "AP PS: TIM update AID=%u %s", 
                    args[0], args[1] ? "set" : "clear");
        }
        break;
    case AP_PS_DBGID_PEER_STATE_CHANGE:
        if (numargs == 2) {
            dbglog_printf(timestamp, vap_id, 
                    "AP PS: AID=%u power save %s", 
                    args[0], args[1] ? "enabled" : "disabled");
        }
        break;
    case AP_PS_DBGID_PSPOLL:
        if (numargs == 3) {
            dbglog_printf(timestamp, vap_id, 
                    "AP PS: AID=%u pspoll response tid=%u flags=%x", 
                    args[0], args[2], args[3]);
        }
        break;
    case AP_PS_DBGID_PEER_CREATE:
        if (numargs == 1) {
            dbglog_printf(timestamp, vap_id, 
                    "AP PS: create peer AID=%u", args[0]);
        }
        break;
    case AP_PS_DBGID_PEER_DELETE:
        if (numargs == 1) {
            dbglog_printf(timestamp, vap_id, 
                    "AP PS: delete peer AID=%u", args[0]);
        }
        break;
    case AP_PS_DBGID_VDEV_CREATE:
        dbglog_printf(timestamp, vap_id, "AP PS: vdev create");
        break;
    case AP_PS_DBGID_VDEV_DELETE:
        dbglog_printf(timestamp, vap_id, "AP PS: vdev delete");
        break;
    case AP_PS_DBGID_SYNC_TIM:
        if (numargs == 3) {
            dbglog_printf(timestamp, vap_id, 
                    "AP PS: AID=%u advertised=%#x buffered=%#x", args[0], args[1], args[2]);
        }
        break;
    case AP_PS_DBGID_NEXT_RESPONSE:
        if (numargs == 4) {
            dbglog_printf(timestamp, vap_id, 
                    "AP PS: AID=%u select next response %s%s%s", args[0],
                    args[1] ? "(usp active) " : "",
                    args[2] ? "(pending usp) " : "",
                    args[3] ? "(pending poll response)" : "");
        }
        break;
    case AP_PS_DBGID_START_SP:
        if (numargs == 3) {
            dbglog_printf(timestamp, vap_id, 
                    "AP PS: AID=%u START SP tsf=%#x (%u)", 
                    args[0], args[1], args[2]);
        }
        break;
    case AP_PS_DBGID_COMPLETED_EOSP:
        if (numargs == 3) {
            dbglog_printf(timestamp, vap_id, 
                    "AP PS: AID=%u EOSP eosp_tsf=%#x trigger_tsf=%#x", 
                    args[0], args[1], args[2]);
        }
        break;
    case AP_PS_DBGID_TRIGGER:
        if (numargs == 4) {
            dbglog_printf(timestamp, vap_id, 
                    "AP PS: AID=%u TRIGGER tsf=%#x %s%s", args[0], args[1],
                    args[2] ? "(usp active) " : "",
                    args[3] ? "(send_n in progress)" : "");
        }
        break;
    case AP_PS_DBGID_DUPLICATE_TRIGGER:
        if (numargs == 4) {
            dbglog_printf(timestamp, vap_id, 
                    "AP PS: AID=%u DUP TRIGGER tsf=%#x seq=%u ac=%u", 
                    args[0], args[1], args[2], args[3]);
        }
        break;
    case AP_PS_DBGID_UAPSD_RESPONSE:
        if (numargs == 5) {
            dbglog_printf(timestamp, vap_id, 
                    "AP PS: AID=%u UAPSD response tid=%u, n_mpdu=%u flags=%#x max_sp=%u current_sp=%u", 
                    args[0], args[1], args[2], args[3], (args[4] >> 16) & 0xffff, args[4] & 0xffff);
        } 
        break;
    case AP_PS_DBGID_SEND_COMPLETE:
        if (numargs == 5) {
            dbglog_printf(timestamp, vap_id, 
                    "AP PS: AID=%u SEND_COMPLETE fc=%#x qos=%#x %s%s", 
                    args[0], args[1], args[2],
                    args[3] ? "(usp active) " : "",
                    args[4] ? "(pending poll response)" : "");
        }
        break;
    case AP_PS_DBGID_SEND_N_COMPLETE:
        if (numargs == 3) {
            dbglog_printf(timestamp, vap_id, 
                    "AP PS: AID=%u SEND_N_COMPLETE %s%s", 
                    args[0], 
                    args[1] ? "(usp active) " : "",
                    args[2] ? "(pending poll response)" : "");
        }
        break;
    case AP_PS_DBGID_DETECT_OUT_OF_SYNC_STA:
        if (numargs == 4) {
            dbglog_printf(timestamp, vap_id, 
                    "AP PS: AID=%u detected out-of-sync now=%u tx_waiting=%u txq_depth=%u",
                   args[0], args[1], args[2], args[3]);
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

A_BOOL
dbglog_wal_print_handler(
        A_UINT32 mod_id, 
        A_UINT16 vap_id, 
        A_UINT32 dbg_id,
        A_UINT32 timestamp, 
        A_UINT16 numargs, 
        A_UINT32 *args)
{
    static const char *states[] = {
        "ACTIVE",
        "WAIT",
        "WAIT_FILTER",
        "PAUSE",
        "PAUSE_SEND_N",
        "BLOCK",
    };

    static const char *events[] = {
        "PAUSE",
        "PAUSE_FILTER",
        "UNPAUSE",

        "BLOCK",
        "BLOCK_FILTER",
        "UNBLOCK",

        "HWQ_EMPTY",
        "ALLOW_N",
    };

#define WAL_VDEV_TYPE(type)     \
    (type == 0 ? "AP" :       \
    (type == 1 ? "STA" :        \
    (type == 2 ? "IBSS" :         \
    (type == 2 ? "MONITOR" :    \
     "UNKNOWN"))))

#define WAL_SLEEP_STATE(state)      \
    (state == 1 ? "NETWORK SLEEP" : \
    (state == 2 ? "AWAKE" :         \
    (state == 3 ? "SYSTEM SLEEP" :  \
    "UNKNOWN")))

    switch (dbg_id) {
    case DBGLOG_DBGID_SM_FRAMEWORK_PROXY_DBGLOG_MSG:
        dbglog_sm_print(timestamp, vap_id, numargs, args, "TID PAUSE",
                states, ARRAY_LENGTH(states), events, ARRAY_LENGTH(events));
        break;
    case WAL_DBGID_SET_POWER_STATE:
        if (numargs == 3) {
            dbglog_printf(timestamp, vap_id, 
                    "WAL %s => %s, req_count=%u",
                    WAL_SLEEP_STATE(args[0]), WAL_SLEEP_STATE(args[1]),
                    args[2]);
        }
        break;
    case WAL_DBGID_CHANNEL_CHANGE_FORCE_RESET:
        if (numargs == 4) {
            dbglog_printf(timestamp, vap_id, 
                    "WAL channel change (force reset) freq=%u, flags=%u mode=%u rx_ok=%u tx_ok=%u",
                    args[0] & 0x0000ffff, (args[0] & 0xffff0000) >> 16, args[1],
                    args[2], args[3]);
        }
        break;
    case WAL_DBGID_CHANNEL_CHANGE:
        if (numargs == 2) {
            dbglog_printf(timestamp, vap_id, 
                    "WAL channel change freq=%u, mode=%u flags=%u rx_ok=1 tx_ok=1",
                    args[0] & 0x0000ffff, (args[0] & 0xffff0000) >> 16, args[1]);
        }
        break;
    case WAL_DBGID_VDEV_START:
        if (numargs == 1) {
            dbglog_printf(timestamp, vap_id, "WAL %s vdev started",
                    WAL_VDEV_TYPE(args[0]));
        }
        break;
    case WAL_DBGID_VDEV_STOP:
        dbglog_printf(timestamp, vap_id, "WAL %s vdev stopped",
                WAL_VDEV_TYPE(args[0]));
        break;
    case WAL_DBGID_VDEV_UP:
        dbglog_printf(timestamp, vap_id, "WAL %s vdev up, count=%u",
                WAL_VDEV_TYPE(args[0]), args[1]);
        break;
    case WAL_DBGID_VDEV_DOWN:
        dbglog_printf(timestamp, vap_id, "WAL %s vdev down, count=%u",
                WAL_VDEV_TYPE(args[0]), args[1]);
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

A_BOOL
dbglog_scan_print_handler(
        A_UINT32 mod_id, 
        A_UINT16 vap_id, 
        A_UINT32 dbg_id,
        A_UINT32 timestamp, 
        A_UINT16 numargs, 
        A_UINT32 *args)
{
    static const char *states[] = {
        "IDLE",
        "BSSCHAN",
        "WAIT_FOREIGN_CHAN",
        "FOREIGN_CHANNEL",
        "TERMINATING"
    };

    static const char *events[] = {
        "REQ",
        "STOP",
        "BSSCHAN",
        "FOREIGN_CHAN",
        "CHECK_ACTIVITY",
        "REST_TIME_EXPIRE",
        "DWELL_TIME_EXPIRE",
        "PROBE_TIME_EXPIRE",
    };

    switch (dbg_id) {
    case DBGLOG_DBGID_SM_FRAMEWORK_PROXY_DBGLOG_MSG:
        dbglog_sm_print(timestamp, vap_id, numargs, args, "SCAN",
                states, ARRAY_LENGTH(states), events, ARRAY_LENGTH(events));
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

void
dbglog_init(wmi_unified_t wmi_handle)
{
    OS_MEMSET(mod_print, 0, sizeof(mod_print));

    dbglog_reg_modprint(WLAN_MODULE_STA_PWRSAVE, dbglog_sta_powersave_print_handler);
    dbglog_reg_modprint(WLAN_MODULE_AP_PWRSAVE, dbglog_ap_powersave_print_handler);
    dbglog_reg_modprint(WLAN_MODULE_WAL, dbglog_wal_print_handler);
    dbglog_reg_modprint(WLAN_MODULE_SCAN, dbglog_scan_print_handler);
    dbglog_reg_modprint(WLAN_MODULE_RATECTRL, dbglog_ratectrl_print_handler);
    dbglog_reg_modprint(WLAN_MODULE_ANI,dbglog_ani_print_handler);
    dbglog_reg_modprint(WLAN_MODULE_DCS,dbglog_dcs_print_handler);

    wmi_unified_register_event_handler(wmi_handle, WMI_DEBUG_MESG_EVENTID,
                                       dbglog_parse_debug_logs, DBGLOG_PRT_WMI);

}

#ifdef unittest_dbglogs

void
test_dbg_config(wmi_unified_t  wmi_handle) 
{
    dbglog_module_log_enable(wmi_handle, 4, 0);
    dbglog_vap_log_enable(wmi_handle, 15, 0);
    dbglog_set_log_lvl(wmi_handle, 1);
}
#endif /* unittest_dbglogs */

