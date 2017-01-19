/*****************************************************************************
 * start multithreading on MIPS34Kc
 * Copyright (c) 2010, Lantiq Inc., All rights reserved
 *****************************************************************************/

#ifndef _STARTMT_H
#define _STARTMT_H

#include <asm/mipsmtregs.h>
#include <asm/types.h>

#define PARSE_ELF_AND_LOAD

void initMT(void);
void runMT(unsigned long stackAddr, unsigned long heapAddr, unsigned long globalAddr, unsigned long startAddr);
void stopMT(void);
void showMT(void);
void showGPR(u32 tcIndex);
void mainVPE1(void);
void menu (void);

int main_threadx(unsigned long * heapPtr);

#endif //_STARTMT_H
