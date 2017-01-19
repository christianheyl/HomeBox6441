/*
 * Copyright (c) 2002-2004 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/art2_main_per_cs/src/art2/common/crc.h#1 $
 */
#ifndef _CRC_H_
#define _CRC_H_

extern char computeCrc8(char *buf, char len);
extern unsigned char computeCrc8_OtpV8(unsigned char *buf, unsigned char len);
extern unsigned short computeChecksumOnly(unsigned short *pHalf, unsigned short length);

#endif  //_CRC_H_
