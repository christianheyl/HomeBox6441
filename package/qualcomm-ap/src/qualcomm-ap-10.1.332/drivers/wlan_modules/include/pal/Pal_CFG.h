//----------------------------------------------------------------------------
//
//  Project:   BT3_0_CORE
//
//  Filename:  Pal_Cfg.h
//
//  Author:    Mike Tsai
//
//  Created:   3/20/2009
//
//  Descriptions:
//
//    PAL Configuration setting here
//
//  Copyright (c) 2009, Atheros Communications, Inc. All right reserved.
//
//--------------------------------------------------------------------------

/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef __PAL_CFG_H__
#define __PAL_CFG_H__

#include "pal_osapi.h"

#define PALENUM_POOL_TAG (ULONG) 'hsOP'
#define PAL_PROCESS_INTERVAL_MS   50
#define MAX_80211G_BW             30000
//#define PAL_DO_NOT_STOP_BEACON
#ifndef PMK_LEN 
#define PMK_LEN 32
#endif

#if DBG
#define dump_frame(a, b) PAL_dump_frame(a, b)
#else
#define dump_frame(a, b) 
#endif

#endif  /* __PAL_STATE_H__ */


