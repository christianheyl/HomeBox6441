//----------------------------------------------------------------------------
//
//  Project:   Seattle Release
//
//  Filename:  bt_debug.h
//
//  Author:    Mike Tsai
//
//  Created:   06/19/2008
//
//  Descriptions:
//
//    Add a description here
//
//  Copyright (c) 2009, Atheros Communications, Inc. All right reserved.
//
//
//
//----------------------------------------------------------------------------

/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#ifndef _BT_DEBUG__H
#define _BT_DEBUG__H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "pal_osapi.h"
//
// Message verbosity: lower values indicate higher urgency
//
#define DL_EXTRA_LOUD       20
#define DL_VERY_LOUD        10
#define DL_LOUD             8
#define DL_INFO             6
#define DL_WARN             4
#define DL_ERROR            2
#define DL_FATAL            0

//Control Block Definition here
#define DBG_GLOBAL          0
#define DBG_ERTM            1
#define DBG_AMP             2
#define DBG_PAL             3
#define DBG_LAST            4 //always last+1

typedef struct _btDbgSetting {

 int    ndisprotDebugLevel;
 uint8  CtrlBlkEnableDbg[DBG_LAST];
}btDbgSetting;

extern const char *szDbgList[DBG_LAST];
extern btDbgSetting BT_DbgSetting;
extern void DbgPrintHexDump(uint8 *pBuffer, uint32	Length);
 
#define DEBUGP(ctrlblk, lev, stmt)	                        \
{                                                           \
     if ((lev <= BT_DbgSetting.ndisprotDebugLevel) &&  (BT_DbgSetting.CtrlBlkEnableDbg[ctrlblk] != 0))   \
	 {                                                      \
            Report((szDbgList[ctrlblk])); Report(stmt);     \
     }                                                      \
}\

inline void DEBUGPDUMP(uint8 ctrlblk, uint8 lev, uint8 *pBuf, uint16 Len)                                    
{                                                              
     if ( (lev <= BT_DbgSetting.ndisprotDebugLevel) &&
		  (BT_DbgSetting.CtrlBlkEnableDbg[ctrlblk] != 0))
	 {
                DbgPrintHexDump(pBuf, Len);         
     }
}


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // _BT_DEBUG__H

