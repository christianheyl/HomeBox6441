/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
//------------------------------------------------------------------------------
// <copyright file="dbglog_id.h" company="Atheros">
//    Copyright (c) 2004-2010 Atheros Corporation.  All rights reserved.
// $ATH_LICENSE_HOSTSDK0_C$
//------------------------------------------------------------------------------
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================

#ifndef _DBGLOG_ID_H_
#define _DBGLOG_ID_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The target state machine framework will send dbglog messages on behalf on
 * other modules. We do this do avoid each target module adding identical
 * dbglog code for state transitions and event processing. We also don't want
 * to force each module to define the the same XXX_DBGID_SM_MSG with the same
 * value below. Instead we use a special ID that the host dbglog code
 * recognizes as a message sent by the SM on behalf on another module.
 */
#define DBGLOG_DBGID_SM_FRAMEWORK_PROXY_DBGLOG_MSG 1000

/*
 * The nomenclature for the debug identifiers is MODULE_DESCRIPTION.
 * Please ensure that the definition of any new debugid introduced is captured
 * between the <MODULE>_DBGID_DEFINITION_START and
 * <MODULE>_DBGID_DEFINITION_END defines. The structure is required for the
 * parser to correctly pick up the values for different debug identifiers.
 */

/*
* The target state machine framework will send dbglog messages on behalf on
* other modules. We do this do avoid each module adding identical dbglog code 
* for state transitions and event processing. We also don't want to force each
* module to define the the same XXX_DBGID_SM_MSG with the same value below.
* Instead we use a special ID that the host dbglog code recognizes as a
* message sent by the SM on behalf on another module.
*/
#define DBGLOG_DBGID_SM_FRAMEWORK_PROXY_DBGLOG_MSG 1000


/* INF debug identifier definitions */
#define INF_DBGID_DEFINITION_START                    0
#define INF_ASSERTION_FAILED                          1
#define INF_TARGET_ID                                 2
#define INF_DBGID_DEFINITION_END                      3

/* WMI debug identifier definitions */
#define WMI_DBGID_DEFINITION_START                    0
#define WMI_CMD_RX_XTND_PKT_TOO_SHORT                 1
#define WMI_EXTENDED_CMD_NOT_HANDLED                  2
#define WMI_CMD_RX_PKT_TOO_SHORT                      3
#define WMI_CALLING_WMI_EXTENSION_FN                  4
#define WMI_CMD_NOT_HANDLED                           5
#define WMI_IN_SYNC                                   6
#define WMI_TARGET_WMI_SYNC_CMD                       7
#define WMI_SET_SNR_THRESHOLD_PARAMS                  8
#define WMI_SET_RSSI_THRESHOLD_PARAMS                 9
#define WMI_SET_LQ_TRESHOLD_PARAMS                   10
#define WMI_TARGET_CREATE_PSTREAM_CMD                11
#define WMI_WI_DTM_INUSE                             12
#define WMI_TARGET_DELETE_PSTREAM_CMD                13
#define WMI_TARGET_IMPLICIT_DELETE_PSTREAM_CMD       14
#define WMI_TARGET_GET_BIT_RATE_CMD                  15
#define WMI_GET_RATE_MASK_CMD_FIX_RATE_MASK_IS       16
#define WMI_TARGET_GET_AVAILABLE_CHANNELS_CMD        17
#define WMI_TARGET_GET_TX_PWR_CMD                    18
#define WMI_FREE_EVBUF_WMIBUF                        19
#define WMI_FREE_EVBUF_DATABUF                       20
#define WMI_FREE_EVBUF_BADFLAG                       21
#define WMI_HTC_RX_ERROR_DATA_PACKET                 22
#define WMI_HTC_RX_SYNC_PAUSING_FOR_MBOX             23
#define WMI_INCORRECT_WMI_DATA_HDR_DROPPING_PKT      24
#define WMI_SENDING_READY_EVENT                      25
#define WMI_SETPOWER_MDOE_TO_MAXPERF                 26
#define WMI_SETPOWER_MDOE_TO_REC                     27
#define WMI_BSSINFO_EVENT_FROM                       28
#define WMI_TARGET_GET_STATS_CMD                     29
#define WMI_SENDING_SCAN_COMPLETE_EVENT              30
#define WMI_SENDING_RSSI_INDB_THRESHOLD_EVENT        31
#define WMI_SENDING_RSSI_INDBM_THRESHOLD_EVENT       32
#define WMI_SENDING_LINK_QUALITY_THRESHOLD_EVENT     33
#define WMI_SENDING_ERROR_REPORT_EVENT               34
#define WMI_SENDING_CAC_EVENT                        35
#define WMI_TARGET_GET_ROAM_TABLE_CMD                36
#define WMI_TARGET_GET_ROAM_DATA_CMD                 37
#define WMI_SENDING_GPIO_INTR_EVENT                  38
#define WMI_SENDING_GPIO_ACK_EVENT                   39
#define WMI_SENDING_GPIO_DATA_EVENT                  40
#define WMI_CMD_RX                                   41
#define WMI_CMD_RX_XTND                              42
#define WMI_EVENT_SEND                               43
#define WMI_EVENT_SEND_XTND                          44
#define WMI_CMD_PARAMS_DUMP_START                    45
#define WMI_CMD_PARAMS_DUMP_END                      46
#define WMI_CMD_PARAMS                               47
#define WMI_EVENT_ALLOC_FAILURE                      48
#define WMI_DBGID_DCS_PARAM_CMD                      49
#define WMI_DBGID_DEFINITION_END                     50

/*  PM Message definition*/
#define PS_STA_DEFINITION_START                     0
#define PS_STA_PM_ARB_REQUEST                       1
#define PS_STA_DELIVER_EVENT                        2


/** RESMGR OCS dbg ids */
#define RESMGR_OCS_DEFINITION_START                 0
#define RESMGR_OCS_ALLOCRAM_SIZE                    1
#define RESMGR_OCS_RESOURCES                        2
#define RESMGR_OCS_LINK_CREATE                      3
#define RESMGR_OCS_LINK_DELETE                      4
#define RESMGR_OCS_CHREQ_CREATE                     5
#define RESMGR_OCS_CHREQ_DELETE                     6
#define RESMGR_OCS_CHREQ_START                      7
#define RESMGR_OCS_CHREQ_STOP                       8
#define RESMGR_OCS_SCHEDULER_INVOKED                9
#define RESMGR_OCS_CHREQ_GRANT                      10
#define RESMGR_OCS_CHREQ_COMPLETE                   11
#define RESMGR_OCS_NEXT_TSFTIME                     12
#define RESMGR_OCS_TSF_TIMEOUT_US                   13
#define RESMGR_OCS_CURR_CAT_WINDOW                  14
#define RESMGR_OCS_CURR_CAT_WINDOW_REQ              15
#define RESMGR_OCS_CURR_CAT_WINDOW_TIMESLOT         16
#define RESMGR_OCS_CHREQ_RESTART                    17
#define RESMGR_OCS_CLEANUP_CH_ALLOCATORS            18
#define RESMGR_OCS_PURGE_CHREQ                      19
#define RESMGR_OCS_CH_ALLOCATOR_FREE                20
#define RESMGR_OCS_RECOMPUTE_SCHEDULE               21
#define RESMGR_OCS_NEW_CAT_WINDOW_REQ               22
#define RESMGR_OCS_NEW_CAT_WINDOW_TIMESLOT          23
#define RESMGR_OCS_CUR_CH_ALLOC                     24
#define RESMGR_OCS_WIN_CH_ALLOC                     25
#define RESMGR_OCS_SCHED_CH_CHANGE                  26
#define RESMGR_OCS_CONSTRUCT_CAT_WIN                27
#define RESMGR_OCS_CHREQ_PREEMPTED                  28
#define RESMGR_OCS_CH_SWITCH_REQ                    29
#define RESMGR_OCS_CHANNEL_SWITCHED                 30
#define RESMGR_OCS_CLEANUP_STALE_REQS               31
#define RESMGR_OCS_DEFINITION_END                   32

/* RESMGR CHNMGR debug ids */
#define RESMGR_CHMGR_DEFINITION_START               0
#define RESMGR_CHMGR_PAUSE_COMPLETE                 1
#define RESMGR_CHMGR_CHANNEL_CHANGE                 2
#define RESMGR_CHMGR_RESUME_COMPLETE                3
#define RESMGR_CHMGR_VDEV_PAUSE                     4
#define RESMGR_CHMGR_VDEV_UNPAUSE                   5
#define RESMGR_CHMGR_DEFINITION_END                 7

/* VDEV manager debug ids */
#define VDEV_MGR_DEFINITION_START                   0
#define VDEV_MGR_BMISS_TIMEOUT                      1
#define VDEV_MGR_BMISS_DETECTED                     2
#define VDEV_MGR_BCN_IN_SYNC                        3
#define VDEV_MGR_AP_KEEPALIVE_IDLE                  4
#define VDEV_MGR_AP_KEEPALIVE_INACTIVE              5
#define VDEV_MGR_AP_KEEPALIVE_UNRESPONSIVE          6

/* WHAL debug identifier definitions */
#define WHAL_DBGID_DEFINITION_START                 0
#define WHAL_ERROR_ANI_CONTROL                      1
#define WHAL_ERROR_CHIP_TEST1                       2
#define WHAL_ERROR_CHIP_TEST2                       3
#define WHAL_ERROR_EEPROM_CHECKSUM                  4
#define WHAL_ERROR_EEPROM_MACADDR                   5
#define WHAL_ERROR_INTERRUPT_HIU                    6
#define WHAL_ERROR_KEYCACHE_RESET                   7
#define WHAL_ERROR_KEYCACHE_SET                     8
#define WHAL_ERROR_KEYCACHE_TYPE                    9
#define WHAL_ERROR_KEYCACHE_TKIPENTRY              10
#define WHAL_ERROR_KEYCACHE_WEPLENGTH              11
#define WHAL_ERROR_PHY_INVALID_CHANNEL             12
#define WHAL_ERROR_POWER_AWAKE                     13
#define WHAL_ERROR_POWER_SET                       14
#define WHAL_ERROR_RECV_STOPDMA                    15
#define WHAL_ERROR_RECV_STOPPCU                    16
#define WHAL_ERROR_RESET_CHANNF1                   17
#define WHAL_ERROR_RESET_CHANNF2                   18
#define WHAL_ERROR_RESET_PM                        19
#define WHAL_ERROR_RESET_OFFSETCAL                 20
#define WHAL_ERROR_RESET_RFGRANT                   21
#define WHAL_ERROR_RESET_RXFRAME                   22
#define WHAL_ERROR_RESET_STOPDMA                   23
#define WHAL_ERROR_RESET_ERRID                     24
#define WHAL_ERROR_RESET_ADCDCCAL1                 25
#define WHAL_ERROR_RESET_ADCDCCAL2                 26
#define WHAL_ERROR_RESET_TXIQCAL                   27
#define WHAL_ERROR_RESET_RXIQCAL                   28
#define WHAL_ERROR_RESET_CARRIERLEAK               29
#define WHAL_ERROR_XMIT_COMPUTE                    30
#define WHAL_ERROR_XMIT_NOQUEUE                    31
#define WHAL_ERROR_XMIT_ACTIVEQUEUE                32
#define WHAL_ERROR_XMIT_BADTYPE                    33
#define WHAL_ERROR_XMIT_STOPDMA                    34
#define WHAL_ERROR_INTERRUPT_BB_PANIC              35
#define WHAL_ERROR_PAPRD_MAXGAIN_ABOVE_WINDOW      36
#define WHAL_DBGID_DEFINITION_END                  37

#define COEX_DEBUGID_START              0
#define BTCOEX_DBG_MCI_1                            1
#define BTCOEX_DBG_MCI_2                            2
#define BTCOEX_DBG_MCI_3                            3
#define BTCOEX_DBG_MCI_4                            4
#define BTCOEX_DBG_MCI_5                            5
#define BTCOEX_DBG_MCI_6                            6
#define BTCOEX_DBG_MCI_7                            7
#define BTCOEX_DBG_MCI_8                            8
#define BTCOEX_DBG_MCI_9                            9
#define BTCOEX_DBG_MCI_10                           10
#define COEX_WAL_BTCOEX_INIT                        11
#define COEX_WAL_PAUSE                              12
#define COEX_WAL_RESUME                             13
#define COEX_UPDATE_AFH                             14
#define COEX_HWQ_EMPTY_CB                           15
#define COEX_MCI_TIMER_HANDLER                      16
#define COEX_MCI_RECOVER                            17
#define ERROR_COEX_MCI_ISR                          18
#define ERROR_COEX_MCI_GPM                          19
#define COEX_ProfileType                            20
#define COEX_LinkID                                 21
#define COEX_LinkState                              22
#define COEX_LinkRole                               23
#define COEX_LinkRate                               24
#define COEX_VoiceType                              25
#define COEX_TInterval                              26
#define COEX_WRetrx                                 27
#define COEX_Attempts                               28
#define COEX_PerformanceState                       29
#define COEX_LinkType                               30
#define COEX_RX_MCI_GPM_VERSION_QUERY               31
#define COEX_RX_MCI_GPM_VERSION_RESPONSE            32
#define COEX_RX_MCI_GPM_STATUS_QUERY                33
#define COEX_STATE_WLAN_VDEV_DOWN                   34
#define COEX_STATE_WLAN_VDEV_START                  35
#define COEX_STATE_WLAN_VDEV_CONNECTED              36
#define COEX_STATE_WLAN_VDEV_SCAN_STARTED           37
#define COEX_STATE_WLAN_VDEV_SCAN_END               38
#define COEX_STATE_WLAN_DEFAULT                     39
#define COEX_CHANNEL_CHANGE                         40
#define COEX_POWER_CHANGE                           41
#define COEX_CONFIG_MGR                             42
#define COEX_TX_MCI_GPM_BT_CAL_REQ                  43
#define COEX_TX_MCI_GPM_BT_CAL_GRANT                44
#define COEX_TX_MCI_GPM_BT_CAL_DONE                 45
#define COEX_TX_MCI_GPM_WLAN_CAL_REQ                46
#define COEX_TX_MCI_GPM_WLAN_CAL_GRANT              47
#define COEX_TX_MCI_GPM_WLAN_CAL_DONE               48
#define COEX_TX_MCI_GPM_BT_DEBUG                    49
#define COEX_TX_MCI_GPM_VERSION_QUERY               50
#define COEX_TX_MCI_GPM_VERSION_RESPONSE            51
#define COEX_TX_MCI_GPM_STATUS_QUERY                52
#define COEX_TX_MCI_GPM_HALT_BT_GPM                 53
#define COEX_TX_MCI_GPM_WLAN_CHANNELS               54
#define COEX_TX_MCI_GPM_BT_PROFILE_INFO             55
#define COEX_TX_MCI_GPM_BT_STATUS_UPDATE            56
#define COEX_TX_MCI_GPM_BT_UPDATE_FLAGS             57
#define COEX_TX_MCI_GPM_UNKNOWN                     58
#define COEX_TX_MCI_SYS_WAKING                      59
#define COEX_TX_MCI_LNA_TAKE                        60
#define COEX_TX_MCI_LNA_TRANS                       61
#define COEX_TX_MCI_SYS_SLEEPING                    62
#define COEX_TX_MCI_REQ_WAKE                        63
#define COEX_TX_MCI_REMOTE_RESET                    64
#define COEX_TX_MCI_TYPE_UNKNOWN                    65
#define COEX_WHAL_MCI_RESET                         66
#define COEX_POLL_BT_CAL_DONE_TIMEOUT               67
#define COEX_WHAL_PAUSE                             68
#define COEX_RX_MCI_GPM_BT_CAL_REQ                  69
#define COEX_RX_MCI_GPM_BT_CAL_DONE                 70
#define COEX_RX_MCI_GPM_BT_CAL_GRANT                71
#define COEX_WLAN_CAL_START                         72
#define COEX_WLAN_CAL_RESULT                        73
#define COEX_BtMciState                             74
#define COEX_BtCalState                             75
#define COEX_WlanCalState                           76
#define COEX_RxReqWakeCount                         77
#define COEX_RxRemoteResetCount                     78
#define COEX_RESTART_CAL                            79
#define COEX_SENDMSG_QUEUE                          80
#define COEX_RESETSEQ_LNAINFO_TIMEOUT               81
#define COEX_MCI_ISR_IntRaw                         82
#define COEX_MCI_ISR_Int1Raw                        83
#define COEX_MCI_ISR_RxMsgRaw                       84
#define COEX_WHAL_COEX_RESET                        85
#define COEX_WAL_COEX_INIT                          86
#define COEX_TXRX_CNT_LIMIT_ISR                     87
#define COEX_CH_BUSY                                88
#define COEX_REASSESS_WLAN_STATE                    89
#define COEX_BTCOEX_WLAN_STATE_UPDATE               90
#define COEX_BT_NUM_OF_PROFILES                     91
#define COEX_BT_NUM_OF_HID_PROFILES                 92
#define COEX_BT_NUM_OF_ACL_PROFILES                 93
#define COEX_BT_NUM_OF_HI_ACL_PROFILES              94
#define COEX_BT_NUM_OF_VOICE_PROFILES               95
#define COEX_WLAN_AGGR_LIMIT                        96
#define COEX_BT_LOW_PRIO_BUDGET                     97
#define COEX_BT_HI_PRIO_BUDGET                      98
#define COEX_BT_IDLE_TIME                           99
#define COEX_SET_COEX_WEIGHT                        100
#define COEX_WLAN_WEIGHT_GROUP                      101
#define COEX_BT_WEIGHT_GROUP                        102
#define COEX_BT_INTERVAL_ALLOC                      103
#define COEX_BT_SCHEME                              104
#define COEX_BT_MGR                                 105
#define COEX_BT_SM_ERROR                            106
#define COEX_SYSTEM_UPDATE                          107
#define COEX_LOW_PRIO_LIMIT                         108
#define COEX_HI_PRIO_LIMIT                          109
#define COEX_BT_INTERVAL_START                      110
#define COEX_WLAN_INTERVAL_START                    111
#define COEX_NON_LINK_BUDGET                        112
#define COEX_CONTENTION_MSG                         113
#define COEX_SET_NSS                                114
#define COEX_SELF_GEN_MASK                          115
#define COEX_PROFILE_ERROR                          116
#define COEX_WLAN_INIT                              117
#define COEX_BEACON_MISS                            118
#define COEX_BEACON_OK                              119
#define COEX_BTCOEX_SCAN_ACTIVITY                   120
#define COEX_SCAN_ACTIVITY                          121
#define COEX_FORCE_QUIETTIME                        122
#define COEX_BT_MGR_QUIETTIME                       123
#define COEX_BT_INACTIVITY_TRIGGER                  124
#define COEX_BT_INACTIVITY_REPORTED                 125
#define COEX_TX_MCI_GPM_WLAN_PRIO                   126
#define COEX_TX_MCI_GPM_BT_PAUSE_PROFILE            127
#define COEX_TX_MCI_GPM_WLAN_SET_ACL_INACTIVITY     128
#define COEX_RX_MCI_GPM_BT_ACL_INACTIVITY_REPORT    129
#define COEX_GENERIC_ERROR                          130
#define COEX_RX_RATE_THRESHOLD                      131
#define COEX_RSSI                                   132

#define COEX_WLAN_VDEV_NOTIF_START                  133
#define COEX_WLAN_VDEV_NOTIF_UP                     134
#define COEX_WLAN_VDEV_NOTIF_DOWN                   135
#define COEX_WLAN_VDEV_NOTIF_STOP                   136
#define COEX_WLAN_VDEV_NOTIF_ADD_PEER               137
#define COEX_WLAN_VDEV_NOTIF_DELETE_PEER            138
#define COEX_WLAN_VDEV_NOTIF_CONNECTED_PEER         139
#define COEX_WLAN_VDEV_NOTIF_PAUSE                  140
#define COEX_WLAN_VDEV_NOTIF_UNPAUSED               141
#define COEX_STATE_WLAN_VDEV_PEER_ADD               142
#define COEX_STATE_WLAN_VDEV_CONNECTED_PEER         143
#define COEX_STATE_WLAN_VDEV_DELETE_PEER            144
#define COEX_STATE_WLAN_VDEV_PAUSE                  145
#define COEX_STATE_WLAN_VDEV_UNPAUSED               146
#define COEX_SCAN_CALLBACK                          147
#define COEX_RC_SET_CHAINMASK                       148

#define COEX_DEBUG_ID_END                           149

#define SCAN_START_COMMAND_FAILED                   0
#define SCAN_STOP_COMMAND_FAILED                    1
#define SCAN_EVENT_SEND_FAILED                      2

#define BEACON_EVENT_SWBA_SEND_FAILED               0

#define RATECTRL_DBGID_DEFINITION_START             0
#define RATECTRL_DBGID_ASSOC                        1
#define RATECTRL_DBGID_NSS_CHANGE                   2
#define RATECTRL_DBGID_CHAINMASK_ERR                3
#define RATECTRL_DBGID_UNEXPECTED_FRAME             4
#define RATECTRL_DBGID_WAL_RCQUERY                  5
#define RATECTRL_DBGID_WAL_RCUPDATE                 6
#define RATECTRL_DBGID_DEFINITION_END               7

#define AP_PS_DBGID_DEFINITION_START                0
#define AP_PS_DBGID_UPDATE_TIM                      1
#define AP_PS_DBGID_PEER_STATE_CHANGE               2
#define AP_PS_DBGID_PSPOLL                          3
#define AP_PS_DBGID_PEER_CREATE                     4
#define AP_PS_DBGID_PEER_DELETE                     5
#define AP_PS_DBGID_VDEV_CREATE                     6
#define AP_PS_DBGID_VDEV_DELETE                     7
#define AP_PS_DBGID_SYNC_TIM                        8
#define AP_PS_DBGID_NEXT_RESPONSE                   9
#define AP_PS_DBGID_START_SP                        10
#define AP_PS_DBGID_COMPLETED_EOSP                  11
#define AP_PS_DBGID_TRIGGER                         12
#define AP_PS_DBGID_DUPLICATE_TRIGGER               13
#define AP_PS_DBGID_UAPSD_RESPONSE                  14
#define AP_PS_DBGID_SEND_COMPLETE                   15
#define AP_PS_DBGID_SEND_N_COMPLETE                 16
#define AP_PS_DBGID_DETECT_OUT_OF_SYNC_STA          17

#define WAL_DBGID_DEFINITION_START                  0
#define WAL_DBGID_FAST_WAKE_REQUEST                 1
#define WAL_DBGID_FAST_WAKE_RELEASE                 2
#define WAL_DBGID_SET_POWER_STATE                   3
#define WAL_DBGID_CHANNEL_CHANGE_FORCE_RESET        5
#define WAL_DBGID_CHANNEL_CHANGE                    6
#define WAL_DBGID_VDEV_START                        7
#define WAL_DBGID_VDEV_STOP                         8
#define WAL_DBGID_VDEV_UP                           9
#define WAL_DBGID_VDEV_DOWN                         10
#define WAL_DBGID_SW_WDOG_RESET                     11
#define WAL_DBGID_TX_SCH_REGISTER_TIDQ              12
#define WAL_DBGID_TX_SCH_UNREGISTER_TIDQ            13
#define WAL_DBGID_TX_SCH_TICKLE_TIDQ                14
#define WAL_DBGID_XCESS_FAILURES                    15
#define WAL_DBGID_AST_ADD_WDS_ENTRY                 16
#define WAL_DBGID_AST_DEL_WDS_ENTRY                 17
#define WAL_DBGID_AST_WDS_ENTRY_PEER_CHG            18
#define WAL_DBGID_AST_WDS_SRC_LEARN_FAIL            19
#define WAL_DBGID_STA_KICKOUT                       20
#define WAL_DBGID_BAR_TX_FAIL                       21
#define WAL_DBGID_BAR_ALLOC_FAIL                    22
#define WAL_DBGID_LOCAL_DATA_TX_FAIL                23
#define WAL_DBGID_SECURITY_PM4_QUEUED               24
#define WAL_DBGID_SECURITY_GM1_QUEUED               25
#define WAL_DBGID_SECURITY_PM4_SENT                 26
#define WAL_DBGID_SECURITY_ALLOW_DATA               27
#define WAL_DBGID_SECURITY_UCAST_KEY_SET            28
#define WAL_DBGID_SECURITY_MCAST_KEY_SET            29
#define WAL_DBGID_SECURITY_ENCR_EN                  30
#define WAL_DBGID_BB_WDOG_TRIGGERED                 31
#define WAL_DBGID_RX_LOCAL_BUFS_LWM                 32
#define WAL_DBGID_RX_LOCAL_DROP_LARGE_MGMT          33
#define WAL_DBGID_VHT_ILLEGAL_RATE_PHY_ERR_DETECTED 34
#define WAL_DBGID_DEV_RESET                         35
#define WAL_DBGID_TX_BA_SETUP                       36
#define WAL_DBGID_RX_BA_SETUP                       37
#define WAL_DBGID_DEV_TX_TIMEOUT                    38
#define WAL_DBGID_DEV_RX_TIMEOUT                    39 
#define WAL_DBGID_STA_VDEV_XRETRY                   40  
#define WAL_DBGID_DCS                               41
#define WAL_DBGID_DEFINITION_END                    42 

#define ANI_DBGID_POLL                               0
#define ANI_DBGID_CONTROL                            1
#define ANI_DBGID_OFDM_PARAMS                        2
#define ANI_DBGID_CCK_PARAMS                         3
#define ANI_DBGID_RESET                              4
#define ANI_DBGID_RESTART                            5
#define ANI_DBGID_OFDM_LEVEL                         6
#define ANI_DBGID_CCK_LEVEL                          7
#define ANI_DBGID_FIRSTEP                            8
#define ANI_DBGID_CYCPWR                             9
#define ANI_DBGID_MRC_CCK                           10
#define ANI_DBGID_SELF_CORR_LOW                     11
#define ANI_DBGID_ENABLE                            12
#define ANI_DBGID_CURRENT_LEVEL                     13
#define ANI_DBGID_POLL_PERIOD                       14
#define ANI_DBGID_LISTEN_PERIOD                     15
#define ANI_DBGID_OFDM_LEVEL_CFG                    16
#define ANI_DBGID_CCK_LEVEL_CFG                     17

/* OFFLOAD Manager Debugids*/
#define OFFLOAD_MGR_DBGID_DEFINITION_START             0
#define OFFLOADMGR_REGISTER_OFFLOAD                    1
#define OFFLOADMGR_DEREGISTER_OFFLOAD                  2
#define OFFLOADMGR_NO_REG_DATA_HANDLERS                3
#define OFFLOADMGR_NO_REG_EVENT_HANDLERS               4
#define OFFLOADMGR_REG_OFFLOAD_FAILED                  5
#define OFFLOADMGR_DBGID_DEFINITION_END                6

/*Resource Debug IDs*/
#define RESOURCE_DBGID_DEFINITION_START             0
#define RESOURCE_PEER_ALLOC                         1
#define RESOURCE_PEER_FREE                          2
#define RESOURCE_PEER_ALLOC_WAL_PEER                3
#define RESOURCE_DBGID_DEFINITION_END               4
/* DCS debug IDs*/
#define WLAN_DCS_DBGID_INIT                         0
#define WLAN_DCS_DBGID_WMI_CWINT                    1
#define WLAN_DCS_DBGID_TIMER                        2
#define WLAN_DCS_DBGID_CMDG                         3
#define WLAN_DCS_DBGID_CMDS                         4
#define WLAN_DCS_DBGID_DINIT                        5
#ifdef __cplusplus
}
#endif

#endif /* _DBGLOG_ID_H_ */

