

//  "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/shared/NartError.c#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/shared/NartError.c#1 $"


#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "ErrorPrint.h"
#include "NartError.h"

static struct _Error _NartError[]=
{
    {NartOk,ErrorControl,NartOkFormat},
    {NartOn,ErrorControl,NartOnFormat},
    {NartOff,ErrorControl,NartOffFormat},
    {NartDataHeader,ErrorInformation,NartDataHeaderFormat},
    {NartData,ErrorInformation,NartDataFormat},
    {NartError,ErrorFatal,NartErrorFormat},
    {NartDone,ErrorControl,NartDoneFormat},
	{NartDebug,ErrorDebug,NartDebugFormat},
	{NartProcess,ErrorControl,NartProcessFormat},
	{NartActive,ErrorInformation,NartActiveFormat},
    {NartOther,ErrorInformation,NartOtherFormat},
    {NartRequest,ErrorInformation,NartRequestFormat},
};

static int _ErrorFirst=1;

PARSEDLLSPEC void NartErrorInit(void)
{
    if(_ErrorFirst)
    {
        ErrorHashCreate(_NartError,sizeof(_NartError)/sizeof(_NartError[0]));
    }
    _ErrorFirst=0;
}

