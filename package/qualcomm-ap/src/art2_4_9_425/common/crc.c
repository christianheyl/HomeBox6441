/* mEeprom.c - contians functions for reading eeprom and getting pcdac power settings for all channels */
/* Copyright (c) 2001 Atheros Communications, Inc., All Rights Reserved */

//#ident  "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/common/crc.c#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/common/crc.c#1 $"

/*
Revsision history
--------------------
1.0       Created.
*/


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

// copied from web, original author: T. Scott Dattalo 
static char crc2(char crc, char crc_data) 
{ 
    char i; 
    i = (crc_data ^ crc) & 0xff; 
    crc = 0; 
    if(i & 1) crc ^= 0x5e; 
    if(i & 2) crc ^= 0xbc; 
    if(i & 4) crc ^= 0x61; 
    if(i & 8) crc ^= 0xc2; 
    if(i & 0x10) crc ^= 0x9d; 
    if(i & 0x20) crc ^= 0x23; 
    if(i & 0x40) crc ^= 0x46; 
    if(i & 0x80) crc ^= 0x8c; 
    return(crc); 
}

char computeCrc8(char *buf, char len)
{
    char i, crc;

    crc=0;
    for (i=0; i<len; i++) {
        crc = crc2(crc, buf[i]);
    }
    return(crc);
}

unsigned char computeCrc8_OtpV8(unsigned char *buf, unsigned char len)
{
    unsigned char sum = 0, i;
    
    for (i = 0; i < len; i++) 
    { 
        sum ^= *buf++; 
    }
    sum = ~sum;
    return(sum);
}


int verifyChecksum_crc8(char *buf, char length)
{
    return(((computeCrc8(buf, length-1) == buf[length-1]) ? 1 : 0));
}

unsigned short computeChecksumOnly(unsigned short *pHalf, unsigned short length)
{
    unsigned short sum = 0, i;
    for (i = 0; i < length; i++) { sum ^= *pHalf++; }
    return(sum);
}
