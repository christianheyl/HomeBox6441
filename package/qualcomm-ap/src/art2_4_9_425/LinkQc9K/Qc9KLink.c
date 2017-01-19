/*
 *  Copyright � 2005 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <ctype.h>

#include "wlantype.h"
#include "art_utf_common.h"
#include "LinkTxRx.h"
#include "LinkStat.h"
#include "Qc9KLink.h"
#include "Qc9KLinkTx.h"
#include "Qc9KLinkRx.h"
#include "Ar6KLinkTxStat.h"
#include "Ar6KLinkRxStat.h"

static struct _LinkFunction Qc9KLinkFunction=
{
	0,                              //TxSetup,
	0,                              //TxStart,
	Qc9KLinkTxComplete,
    Ar6KLinkTxStatFetch,            //TxStatFetch,
	0,                              //RxSetup,
    Qc9KLinkRxStart,                //RxStart,
	Qc9KLinkRxComplete,             //RxComplete,
    Ar6KLinkRxStatFetch,            //RxStatFetch,
	0,                              //RxSpectralScan,
	0,                              //TxForPAPD,
    Qc9KLinkTxDataStartCmd,         //TxDataStartCmd
    Qc9KLinkRxDataStartCmd,         //RxDataStartCmd
    Ar6KLinkTxStatTemperatureGet,   //TxStatTemperatureGet

};

//
// clear all device control function pointers and set to default behavior
//
LINKDLLSPEC int Qc9KLinkSelect(void)
{
	int error;

	LinkFunctionReset();
	error=LinkFunctionSelect(&Qc9KLinkFunction);

	return error;
}

LINKDLLSPEC char *LinkPrefix(void)
{
	return "Qc9K";
}

  
