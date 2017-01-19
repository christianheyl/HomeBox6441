

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <ctype.h>

#include "LinkTxRx.h"
#include "LinkStat.h"

#include "DescriptorLink.h"
#include "DescriptorLinkRx.h"
#include "DescriptorLinkRxStat.h"
#include "DescriptorLinkTx.h"
#include "DescriptorLinkTxStat.h"


static struct _LinkFunction _DescriptorLink=
{
	DescriptorLinkTxSetup,
	DescriptorLinkTxStart,
	DescriptorLinkTxComplete,
	DescriptorLinkTxStatFetch,
	DescriptorLinkRxSetup,
	DescriptorLinkRxStart,
	DescriptorLinkRxComplete,
	DescriptorLinkRxStatFetch,
	DescriptorLinkRxSpectralScan,
	DescriptorLinkTxForPAPD,
    0,      //TxDataStartCmd
    0,      //RxDataStartCmd
    0,      //TxStatTemperatureGet
};


//
// clear all device control function pointers and set to default behavior
//
int LINKDLLSPEC LinkLinkSelect(void)
{
	int error;

	LinkFunctionReset();
	error=LinkFunctionSelect(&_DescriptorLink);

	return error;
}

LINKDLLSPEC char *LinkPrefix(void)
{
	return "Link";
}

