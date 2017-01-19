
//  "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/shared/ConnectError.c#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/shared/ConnectError.c#1 $"


#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "smatch.h"
#include "ErrorPrint.h"
#include "ConnectError.h"



static struct _Error _ConnectError[]=
{
    {ConnectNartTrying,ErrorInformation,ConnectNartTryingFormat},
    {ConnectNart,ErrorInformation,ConnectNartFormat},
    {ConnectNartBad,ErrorFatal,ConnectNartBadFormat},
    {ConnectNartTimeout,ErrorFatal,ConnectNartTimeoutFormat},
    {ConnectNartTest,ErrorInformation,ConnectNartTestFormat},
    {ConnectNartRead,ErrorFatal,ConnectNartReadFormat},
    {ConnectNartWrite,ErrorFatal,ConnectNartWriteFormat},
    {ConnectNartClose,ErrorFatal,ConnectNartCloseFormat},
    {ConnectNartCommand,ErrorControl,ConnectNartCommandFormat},
    {ConnectNartResponse,ErrorControl,ConnectNartResponseFormat},
    {ConnectNartError,ErrorFatal,ConnectNartErrorFormat},
    {ConnectNartDone,ErrorControl,ConnectNartDoneFormat},
    {ConnectNartData,ErrorControl,ConnectNartDataFormat},
    {ConnectNartIndexBad,ErrorFatal,ConnectNartIndexBadFormat},
    {ConnectNartNoConnection,ErrorFatal,ConnectNartNoConnectionFormat},
    {ConnectGuiListen,ErrorInformation,ConnectGuiListenFormat},
    {ConnectGuiListenBad,ErrorInformation,ConnectGuiListenBadFormat},
    {ConnectGuiTrying,ErrorInformation,ConnectGuiTryingFormat},
    {ConnectGui,ErrorInformation,ConnectGuiFormat},
    {ConnectGuiBad,ErrorFatal,ConnectGuiBadFormat},
    {ConnectGuiRead,ErrorFatal,ConnectGuiReadFormat},
    {ConnectGuiWrite,ErrorFatal,ConnectGuiWriteFormat},
    {ConnectGuiClose,ErrorFatal,ConnectGuiCloseFormat},
    {ConnectGuiWait,ErrorInformation,ConnectGuiWaitFormat},
	{ConnectGuiAccept,ErrorInformation,ConnectGuiAcceptFormat},
};

static int _ErrorFirst=1;

PARSEDLLSPEC void ConnectErrorInit(void)
{
    if(_ErrorFirst)
    {
        ErrorHashCreate(_ConnectError,sizeof(_ConnectError)/sizeof(_ConnectError[0]));
    }
    _ErrorFirst=0;
}


