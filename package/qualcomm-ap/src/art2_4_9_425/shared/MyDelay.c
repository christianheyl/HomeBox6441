
//  "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/shared/MyDelay.c#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/shared/MyDelay.c#1 $"

#ifdef _WINDOWS
 #include <windows.h>
#endif


#if !defined(LINUX) && !defined(__APPLE__)
#include <conio.h>
#include <io.h>
#else 
#include <unistd.h>
#endif

//#define OSPREY 1

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>


#include "MyDelay.h"


//#define OSPREY 1


PARSEDLLSPEC void MyDelay(int ms)
{
//#ifdef OSPREY
	if(ms<=0) ms=1;
//#endif
#ifdef MDK_AP
	milliSleep(ms);
#elif defined(__APPLE__)
	usleep(ms);
#elif defined(LINUX)
    usleep(ms*1000);
#else
	Sleep(ms);
#endif	
}

