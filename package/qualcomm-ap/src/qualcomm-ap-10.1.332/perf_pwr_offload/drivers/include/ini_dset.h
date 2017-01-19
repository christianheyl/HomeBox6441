/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
//------------------------------------------------------------------------------
// Copyright (c) 2004-2010 Atheros Corporation.  All rights reserved.
// $ATH_LICENSE_HOSTSDK0_C$
//------------------------------------------------------------------------------
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================

#ifndef _INI_DSET_H_
#define _INI_DSET_H_

/*
 * Each of these represents a WHAL INI table, which consists
 * of an "address column" followed by 1 or more "value columns".
 *
 * Software uses the base WHAL_INI_DATA_ID+column to access a
 * DataSet that holds a particular column of data.
 */

#if defined(AR6002_REV4) || defined(AR6003)
typedef enum {
    WHAL_INI_DATA_ID_NULL               =0,
    WHAL_INI_DATA_ID_MODE_SPECIFIC      =1,  /* 2,3,4,5 */
    WHAL_INI_DATA_ID_COMMON             =6,  /* 7 */
    WHAL_INI_DATA_ID_BB_RFGAIN          =8,  /* 9,10 */
#ifdef FPGA
    WHAL_INI_DATA_ID_ANALOG_BANK0       =11, /* 12 */
    WHAL_INI_DATA_ID_ANALOG_BANK1       =13, /* 14 */
    WHAL_INI_DATA_ID_ANALOG_BANK2       =15, /* 16 */
    WHAL_INI_DATA_ID_ANALOG_BANK3       =17, /* 18, 19 */
    WHAL_INI_DATA_ID_ANALOG_BANK6       =20, /* 21,22 */
    WHAL_INI_DATA_ID_ANALOG_BANK7       =23, /* 24 */
    WHAL_INI_DATA_ID_ADDAC              =25, /* 26 */
#else
    WHAL_INI_DATA_ID_ANALOG_COMMON      =11, /* 12 */ 
    WHAL_INI_DATA_ID_ANALOG_MODE_SPECIFIC=13, /* 14,15 */ 
    WHAL_INI_DATA_ID_ANALOG_BANK6       =16, /* 17,18 */
    WHAL_INI_DATA_ID_MODE_OVERRIDES     =19, /* 20,21,22,23 */
    WHAL_INI_DATA_ID_COMMON_OVERRIDES   =24, /* 25 */
    WHAL_INI_DATA_ID_ANALOG_OVERRIDES   =26, /* 27,28 */
#endif /* FPGA */
    WHAL_INI_DATA_ID_MAX                =31
} WHAL_INI_DATA_ID;
#endif /* AR6002_REV4 || AR6003 */

#if defined(AR6002_REV6) || defined(AR6004)
typedef enum {
#ifdef FPGA
    WHAL_INI_DATA_ID_NULL               =0,
    WHAL_INI_DATA_ID_MODE_SPECIFIC      =1,  /* 2,3,4,5 */
    WHAL_INI_DATA_ID_COMMON             =6,  /* 7 */
    WHAL_INI_DATA_ID_BB_RFGAIN          =8,  /* 9,10 */
    WHAL_INI_DATA_ID_ANALOG_BANK0       =11, /* 12 */
    WHAL_INI_DATA_ID_ANALOG_BANK1       =13, /* 14 */
    WHAL_INI_DATA_ID_ANALOG_BANK2       =15, /* 16 */
    WHAL_INI_DATA_ID_ANALOG_BANK3       =17, /* 18, 19 */
    WHAL_INI_DATA_ID_ANALOG_BANK6       =20, /* 21,22 */
    WHAL_INI_DATA_ID_ANALOG_BANK7       =23, /* 24 */
    WHAL_INI_DATA_ID_ADDAC              =25, /* 26 */
    WHAL_INI_DATA_ID_MAX                =31
#else
    WHAL_INI_DATA_ID_NULL               =0,
    WHAL_INI_DATA_ID_SOC_COMMON         =1,  /* 2 */
    WHAL_INI_DATA_ID_SOC_MODE_SPECIFIC  =3,  /* 4,5,6,7 */
    WHAL_INI_DATA_ID_MAC_COMMON         =8,  /* 9 */
    WHAL_INI_DATA_ID_MAC_MODE_SPECIFIC  =10, /* 11,12,13,14 */
    WHAL_INI_DATA_ID_BB_COMMON          =15, /* 16 */
    WHAL_INI_DATA_ID_BB_MODE_SPECIFIC   =17, /* 18,19,20,21 */
    WHAL_INI_DATA_ID_ANALOG_COMMON      =22, /* 23 */
    WHAL_INI_DATA_ID_ANALOG_MODE_SPECIFIC=24,/* 25,26,27,28 */ 
    WHAL_INI_DATA_ID_BB_RXGAIN_TABLE    =29, /* 30 */      /* without XLNA */
    WHAL_INI_DATA_ID_BB_RXGAIN_TABLE_XLNA=31, /* 32 */
    WHAL_INI_DATA_ID_BB_TXGAIN_TABLE_HI =33, /* 34,35,36,37 */
    WHAL_INI_DATA_ID_BB_TXGAIN_TABLE_LO =38, /* 39,40,41,42 */
    WHAL_INI_DATA_ID_ANALOG_XTAL        =43, /* 44,45,46,47,48,49 */
    WHAL_INI_DATA_ID_BB_TIERED_MODE_SPECIFIC = 50, /* 51,52,53,54 */
    WHAL_INI_DATA_ID_COMMON_OVERRIDES   =55, /* 56 */
    WHAL_INI_DATA_ID_MODE_OVERRIDES     =57, /* 58,59,50,61 */
    WHAL_INI_DATA_ID_ANALOG_OVERRIDES   =62, /* 63,64,65,66 */
    WHAL_INI_DATA_ID_MAX                =70  /* reserved for future ids */
#endif /* FPGA */
} WHAL_INI_DATA_ID;
#endif /* AR6002_REV6 || AR6004 */ 

#if defined(AR600P)
typedef enum {
/* Add these definitions for compatability  */
#define WHAL_INI_DATA_ID_BB_RFGAIN_LNA1 WHAL_INI_DATA_ID_BB_RFGAIN
#define WHAL_INI_DATA_ID_BB_RFGAIN_LNA2 WHAL_INI_DATA_ID_BB_RFGAIN
    WHAL_INI_DATA_ID_NULL               =0,
    WHAL_INI_DATA_ID_MODE_SPECIFIC      =1,  /* 2,3,4,5 */
    WHAL_INI_DATA_ID_COMMON             =6,  /* 7 */
    WHAL_INI_DATA_ID_BB_RFGAIN          =8,  /* 9,10 */
#ifdef FPGA
    WHAL_INI_DATA_ID_ANALOG_BANK0       =11, /* 12 */
    WHAL_INI_DATA_ID_ANALOG_BANK1       =13, /* 14 */
    WHAL_INI_DATA_ID_ANALOG_BANK2       =15, /* 16 */
    WHAL_INI_DATA_ID_ANALOG_BANK3       =17, /* 18, 19 */
    WHAL_INI_DATA_ID_ANALOG_BANK6       =20, /* 21,22 */
    WHAL_INI_DATA_ID_ANALOG_BANK7       =23, /* 24 */
    WHAL_INI_DATA_ID_ADDAC              =25, /* 26 */
#else
    WHAL_INI_DATA_ID_ANALOG_COMMON      =11, /* 12 */ 
    WHAL_INI_DATA_ID_ANALOG_MODE_SPECIFIC=13, /* 14,15 */ 
    WHAL_INI_DATA_ID_ANALOG_BANK6       =16, /* 17,18 */
    WHAL_INI_DATA_ID_MODE_OVERRIDES     =19, /* 20,21,22,23 */
    WHAL_INI_DATA_ID_COMMON_OVERRIDES   =24, /* 25 */
    WHAL_INI_DATA_ID_ANALOG_OVERRIDES   =26, /* 27,28 */
#endif /* FPGA */
    WHAL_INI_DATA_ID_MAX                =31
} WHAL_INI_DATA_ID;
#endif /* AR600P */

typedef PREPACK struct {
    A_UINT16 freqIndex; // 1 - A mode 2 - B or G mode 0 - common
    A_UINT16 offset;
    A_UINT32 newValue;
} POSTPACK INI_DSET_REG_OVERRIDE;

#endif
