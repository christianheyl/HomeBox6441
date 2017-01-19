/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef __WLAN_TGT_DEF_CONFIG_H__
#define __WLAN_TGT_DEF_CONFIG_H__

/*
 * set of default target config , that can be over written by platform
 */

/*
 * default limit of 8 VAPs per device.
 */
#define CFG_TGT_NUM_VDEV                16 

/*
 * We would need 1 AST entry per peer. Scale it by a factor of 2 to minimize hash collisions.
 * TODO: This scaling factor would be taken care inside the WAL in the future.
 */
#define CFG_TGT_NUM_PEER_AST            2

/* # of WDS entries to support.
 */
#define CFG_TGT_WDS_ENTRIES             32

/* MAC DMA burst size. 0: 128B - default, 1: 256B, 2: 64B
 */
#define CFG_TGT_DEFAULT_DMA_BURST_SIZE   0

/* Fixed delimiters to be inserted after every MPDU
 */
#define CFG_TGT_DEFAULT_MAC_AGGR_DELIM   0

/*
 * This value may need to be fine tuned, but a constant value will
 * probably always be appropriate; it is probably not necessary to
 * determine this value dynamically.
 */
#define CFG_TGT_AST_SKID_LIMIT          16 

/*
 * default number of peers per device.
 */
#define CFG_TGT_NUM_PEERS               32
/*
 * max number of peers per device.
 */
#define CFG_TGT_NUM_PEERS_MAX           128

/*
 * keys per peer node
 */
#define CFG_TGT_NUM_PEER_KEYS           2
/*
 * total number of data TX and RX TIDs 
 */
#define CFG_TGT_NUM_TIDS                (2 * CFG_TGT_NUM_PEERS)
/*
 * max number of Tx TIDS
 */
#define CFG_TGT_NUM_TIDS_MAX            (2 * CFG_TGT_NUM_PEERS_MAX)
/*
 * set this to 0x7 (Peregrine = 3 chains).
 * need to be set dynamically based on the HW capability.
 */
#define CFG_TGT_DEFAULT_TX_CHAIN_MASK   0x7
/*
 * set this to 0x7 (Peregrine = 3 chains).
 * need to be set dynamically based on the HW capability.
 */
#define CFG_TGT_DEFAULT_RX_CHAIN_MASK   0x7
/* 100 ms for video, best-effort, and background */
#define CFG_TGT_RX_TIMEOUT_LO_PRI       100
/* 40 ms for voice*/
#define CFG_TGT_RX_TIMEOUT_HI_PRI       40

/* AR9888 unified is default in ethernet mode */
#define CFG_TGT_RX_DECAP_MODE (0x2)
/* Decap to native Wifi header */
#define CFG_TGT_RX_DECAP_MODE_NWIFI (0x1)

/* maximum number of pending scan requests */
#define CFG_TGT_DEFAULT_SCAN_MAX_REQS   0x4

/* maximum number of VDEV that could use BMISS offload */
#define CFG_TGT_DEFAULT_BMISS_OFFLOAD_MAX_VDEV   0x2

/* maximum number of VDEV offload Roaming to support */
#define CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_VDEV   0x2

/* maximum number of AP profiles pushed to offload Roaming */
#define CFG_TGT_DEFAULT_ROAM_OFFLOAD_MAX_PROFILES   0x8

/* default: mcast->ucast disabled if ATH_SUPPORT_MCAST2UCAST not defined */
#ifndef ATH_SUPPORT_MCAST2UCAST
#define CFG_TGT_DEFAULT_NUM_MCAST_GROUPS 0
#define CFG_TGT_DEFAULT_NUM_MCAST_TABLE_ELEMS 0
#define CFG_TGT_DEFAULT_MCAST2UCAST_MODE 0 /* disabled */
#else
/* (for testing) small multicast group membership table enabled */
#define CFG_TGT_DEFAULT_NUM_MCAST_GROUPS 4
#define CFG_TGT_DEFAULT_NUM_MCAST_TABLE_ELEMS 16
#define CFG_TGT_DEFAULT_MCAST2UCAST_MODE 2
#endif

/*
 * Specify how much memory the target should allocate for a debug log of
 * tx PPDU meta-information (how large the PPDU was, when it was sent,
 * whether it was successful, etc.)
 * The size of the log records is configurable, from a minimum of 28 bytes
 * to a maximum of about 300 bytes.  A typical configuration would result
 * in each log record being about 124 bytes.
 * Thus, 1KB of log space can hold about 30 small records, 3 large records,
 * or about 8 typical-sized records.
 */
#define CFG_TGT_DEFAULT_TX_DBG_LOG_SIZE 1024 /* bytes */

/* target based fragment timeout and MPDU duplicate detection */
#define CFG_TGT_DEFAULT_RX_SKIP_DEFRAG_TIMEOUT_DUP_DETECTION_CHECK 1

/* Configuration for VoW
 * VoW requires dedicated resources reserved for VI links. As resources 
 * are limited in the target, it requires to relook into the other resources
 * and optimize the allocation of them in the system. Below configuration 
 * tries to adjust the system resources allocation based on the current 
 * customer reqirements for Video soultions (Carrier requirements)
 */

/*  Default VoW configuration (No of VI Nodes, Descs per VI Node)
 */
#define CFG_TGT_DEFAULT_VOW_CONFIG   0

/*
 * No of VAPs per devices
 */
#define CFG_TGT_NUM_VDEV_VOW        16

/* No of WDS entries to support
    */
#define CFG_TGT_WDS_ENTRIES_VOW     16

/*
 * total number of peers per device
 */
#define CFG_TGT_NUM_PEERS_VOW       16

/*
 * total number of data TX and RX TIDs
 */
#define CFG_TGT_NUM_TIDS_VOW        (2 * (CFG_TGT_NUM_PEERS_VOW + CFG_TGT_NUM_VDEV_VOW))


/*
 * total number of descriptors to use in the target
 */
#define CFG_TGT_NUM_MSDU_DESC    (1024 + 400)

#endif  /*__WLAN_TGT_DEF_CONFIG_H__ */
