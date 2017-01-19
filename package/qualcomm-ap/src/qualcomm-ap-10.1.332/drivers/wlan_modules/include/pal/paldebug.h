/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifdef __cplusplus
extern "C"  {
#endif

#ifndef PAL_DEBUG_INC
#define PAL_DEBUG_INC

#include "pal_osapi.h"
#include "bt_debug.h"

extern ULONG GlobalDebugLevel;
extern ULONG GlobalDebugComponents;

//
// Define the tracing levels
//
#define     PAL_DBG_OFF                 100 // Never used in a call to MpTrace
#define     PAL_DBG_FORCE               1
#define     PAL_DBG_SERIOUS             10
#define     PAL_DBG_NORMAL              45
#define     PAL_DBG_LOUD                70
#define     PAL_DBG_DETAILED            80
#define     PAL_DBG_COMPLEX             85
#define     PAL_DBG_FUNCTION            95  // MpEntry/Exit
#define     PAL_DBG_TRACE               99  // Never Set GlobalDebugLevel to this

//
// Define the tracing components
//

#define PAL_DBG_COMP_PAL                    0x00200000

#define PalTrace(_comp, _level, s)                    \
{\
        if ( ((_comp & GlobalDebugComponents) == _comp) && (_level <= GlobalDebugLevel)) { \
            DbgPrint s;                              \
        }                                            \
}\

#if DBG

#define PAL_PRINT(s)  PalTrace(PAL_DBG_COMP_PAL, PAL_DBG_NORMAL, s);
#define PAL_PRINT_DATA(s)  PalTrace(PAL_DBG_COMP_PAL, PAL_DBG_DETAILED, s);

#else

#define PAL_PRINT(s)
#define PAL_PRINT_DATA(s)

#endif
/* note: 
    remove comment line to enable programmetic connection
    palce profiletemplate.xml to your C:\     
*/

#endif

#ifdef __cplusplus
}
#endif


