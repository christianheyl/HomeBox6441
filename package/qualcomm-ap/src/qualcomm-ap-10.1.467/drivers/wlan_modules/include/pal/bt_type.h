//----------------------------------------------------------------------------
//
//  Project:   Azure
//
//  Filename:  common.h
//
//  Author:    Joakim Linde
//
//  Created:   6/1/2005
//
//  Descriptions:
//
//    Add a description here
//
//  Copyright (c) 2005, Atheros Communications, Inc. All right reserved.
//
//----------------------------------------------------------------------------

/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#ifndef BT_TYPE_H_INCLUDED_
#define BT_TYPE_H_INCLUDED_


//----------------------------------------------------------------------------
//
//  Types
//
//----------------------------------------------------------------------------

typedef unsigned long long u64;
typedef unsigned long u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef signed long long s64;
typedef signed long s32;
typedef signed short s16;
typedef signed char s8;
#define WPA_TYPES_DEFINED

//----------------------------------------------------------------------------
//
//  Types
//
//----------------------------------------------------------------------------

typedef unsigned char  uint8;
typedef signed char    sint8;
typedef unsigned short uint16;
typedef signed short   sint16;
typedef unsigned int   uint32;
typedef signed int     sint32;
typedef unsigned __int64 uint64; 
typedef signed __int64 sint64;
//typedef unsigned int   bool;
typedef unsigned char  shortbool;

/* boolean */
//typedef unsigned char bool;
#define false     0
#define true      1

//----------------------------------------------------------------------------
//
//  Defines
//
//----------------------------------------------------------------------------
//SIM_HOST is generic to platform simulation and target 
//When it is defined, the simhost.c provides a thin layer, very simple
//host simulation from CLI.
//When it is not defined, in the simulation mode, we receive command via
//a virtual UART port (named pipe) from OINA host stack
//In the real target system, we receive command via a real UART port
//from OINA host stack
//----------------------------------------------------------------------------

#define __RPCASYNC_H__ // To avoid irritating warnings in rpcasync.h

// define inline since it is only define for C++ in Win32, not plain C.
#define inline __inline

//----------------------------------------------------------------------------
//
//  Common Macros
//
//----------------------------------------------------------------------------


// Define NULL if needed 
#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif        
#endif

#define UNUSED(x) (x = x)

//----------------------------------------------------------------------------
//
//  Common Includes
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
//  Global Variables
//
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//
//  Functions
//
//----------------------------------------------------------------------------

#endif //BT_TYPE_H_INCLUDED_

//----------------------------------------------------------------------------
// End of file: common.h
