/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#ifndef _CFG_HOST__H_
#define _CFG_HOST__H_

#if defined(CONFIG_AR6004_SUPPORT)
#include <wmi.h> /* NUM_DEV, AP_MAX_NUM_STA */
#elif defined(CONFIG_AR6320_SUPPORT) /* TODO: use AR6004's content to compile, fix this later */
#include <wmi.h> /* NUM_DEV, AP_MAX_NUM_STA */
#endif

/**
 * @brief cfg_host_num_vdev - number of virtual devices (VAPs) to support
 */
/*
 * FOR NOW, hard code a limit of 8 VAPs per device.
 * Later, set this dynamically via bootup config.
 * This value needs to match the target's cfg_tgt_num_vdev value.
 */
#if defined(CONFIG_AR6004_SUPPORT)
    #define cfg_host_num_vdev(ar) NUM_DEV
#elif defined(CONFIG_AR6320_SUPPORT) /* TODO: use AR6004's content to compile, fix this later */
    #define cfg_host_num_vdev(ar) NUM_DEV
#elif defined(CONFIG_AR9888_SUPPORT)
    #define cfg_host_num_vdev(ar) 8
#else
    #error UNKNOWN OR UNSPECIFIED TARGET CHIP
#endif

/**
 * @brief cfg_host_ast_size - number of entries in the address search table
 */
/*
 * FOR NOW, use 4x the number of peers.
 * This 4x factor makes the table sparse enough to accomodate hash collisions,
 * and also provides enough entries to support WDS and proxy STA routing.
 * This 4x factor may be suitable indefinitely.
 * Alternatively, this 4x definition can be replaced with a independent
 * dynamic value set during bootup config.
 * This value needs to match the target's cfg_tgt_ast_size value.
 */
#if defined(CONFIG_AR6004_SUPPORT)
    //#error AST does not apply to McKinley AR6000
    #define cfg_host_ast_size() 0
#elif defined(CONFIG_AR6320_SUPPORT) /* TODO: use AR6004's content to compile, fix this later */
    //#error AST does not apply to McKinley AR6000
    #define cfg_host_ast_size() 0
#elif defined(CONFIG_AR9888_SUPPORT)
    #define cfg_host_ast_size(ar) (4 * cfg_host_num_peers())
#else
    #error UNKNOWN OR UNSPECIFIED TARGET CHIP
#endif

/**
 * @brief cfg_host_num_peers - number of peer nodes to support
 */
/*
 * FOR NOW, hard code a limit of 64 peers per device.
 * Later, set this dynamically through the host interest area.
 * Later, set this dynamically via bootup config.
 * This value needs to match the target's cfg_tgt_num_peers value.
 */
#if defined(CONFIG_AR6004_SUPPORT)
    #define cfg_host_num_peers(ar) AP_MAX_NUM_STA
#elif defined(CONFIG_AR6320_SUPPORT) /* TODO: use AR6004's content to compile, fix this later */
    #define cfg_host_num_peers(ar) AP_MAX_NUM_STA
#elif defined(CONFIG_AR9888_SUPPORT)
    #define cfg_host_num_peers(ar) 64
#else
    #error UNKNOWN OR UNSPECIFIED TARGET CHIP
#endif

/*
 * FOR NOW, hard code this to 2x the number of peers.
 * (On average, 2 TIDs used per peer node.)
 * Later, set this dynamically via bootup config.
 * This value needs to match the target's cfg_tgt_num_tids value.
 */
#if defined(CONFIG_AR6004_SUPPORT)
    //#error per TID info for PN checks is not applicable for McKinley AR6004
    #define cfg_host_num_tids() 0
#elif defined(CONFIG_AR6320_SUPPORT) /* TODO: use AR6004's content to compile, fix this later */
    //#error per TID info for PN checks is not applicable for McKinley AR6004
    #define cfg_host_num_tids() 0
#elif defined(CONFIG_AR9888_SUPPORT)
    #define cfg_host_num_tids(ar) (2 * cfg_host_num_peers())
#else
    #error UNKNOWN OR UNSPECIFIED TARGET CHIP
#endif

#endif /* _CFG_HOST__H_ */
