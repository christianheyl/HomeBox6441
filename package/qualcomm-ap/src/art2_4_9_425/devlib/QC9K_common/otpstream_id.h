/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
// Copyright (c) 2010 Atheros Communications Inc.
// All rights reserved.
// $ATH_LICENSE_TARGET_C$

#ifndef __OTPSTREAM_ID_H__
#define __OTPSTREAM_ID_H__

/* Stream IDs used in One Time Programmable memory */

#define OTPSTREAM_ID_INVALID0             0

        /* BEGIN RESERVED ID's */
/* NOTE: ID's 1..127 are reserved for manufacturing and calibration */
#define OTPSTREAM_ID_CAL                  1
#define OTPSTREAM_ID_CUSTOMER             2
#define OTPSTREAM_ID_MODAL_CFG            3
#define OTPSTREAM_ID_MAC                  4
#define OTPSTREAM_ID_NONMODAL_CFG         5
#define OTPSTREAM_ID_CAL_5G               6
#define OTPSTREAM_ID_CAL_2G               7 
#define OTPSTREAM_ID_ID                   8 
#define OTPSTREAM_ID_CTL_5G               9
#define OTPSTREAM_ID_CTL_2G               10
#define OTPSTREAM_ID_CONFIG               11 /*   <----------------\   */
                                             /*                    |   */
                                             /*      MUST  MATCH!  |   */
                                             /*                    V   */
#define OTPSTREAM_ID_IS_MFG_CAL(myid) (((myid) >= 1) && ((myid) <= 11))
#define RESERVED_ID_127                 127
        /* END RESERVED ID's */


        /* Add new OTPSTREAM ID's here */

#define OTPSTREAM_ID_PATCH3             247 /* applied just after BMI phase */
#define OTPSTREAM_ID_PATCH2             248 /* applied just before BMI phase */
#define OTPSTREAM_ID_PATCH1             249 /* early patches, to init interconnect */
#define OTPSTREAM_ID_PCIE_CONFIG        250
#define OTPSTREAM_ID_IOT_CHIP_TYPE      251
#define OTPSTREAM_ID_USB_CONFIG         252
#define OTPSTREAM_ID_NVRAM_CONFIG       253
#define OTPSTREAM_ID_EXTEND             254 /* reserved to extend stream id's */
#define OTPSTREAM_ID_INVALID1           255

#endif /* __OTPSTREAM_ID_H__ */
