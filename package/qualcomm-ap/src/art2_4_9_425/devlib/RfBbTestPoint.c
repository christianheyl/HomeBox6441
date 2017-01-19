//
// File RfBbTestPoint.c
//
// Process RF-BB testpoint command
//

#include <stdio.h>
#include <stdlib.h>

#include "TimeMillisecond.h"
#include "Device.h"
#include "MyDelay.h"
#include "UserPrint.h"


DEVICEDLLSPEC void RfBbTestPointStart(int frequency, int ht40, int bandwidth, int antennapair, unsigned char chainnum,
                       int mbgain, int rfgain, int coex, int sharedrx, int switchtable, unsigned char AnaOutEn,
                       int (*ison)(), int (*done)())
{	
	int it;

	DeviceRfBbTestPoint(frequency, ht40, bandwidth, antennapair, chainnum, mbgain, rfgain, coex, sharedrx, switchtable, AnaOutEn);

	if(ison!=0)
	{
		(*ison)();
	}

	for(it = 0; ; it++) 
	{
		//
		// check for message from cart telling us to stop
		// this is the normal terminating condition
		//
		if(done!=0)
		{
			if((*done)())
			{
				UserPrint("Stop message received.\n");
				break;
			}
		}
		//
		// sleep every other time, need to keep up with fast rates
		//
		if (it % 100 == 0)
		{
			UserPrint(".");
		}
	    MyDelay(100);
    } 
}
