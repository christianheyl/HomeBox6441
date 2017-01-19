//
// File: MyAccess.c
//
// Description: Memory and register access functions
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "wlantype.h"

#include "Device.h"
#include "UserPrint.h"
#include "MyAccess.h"

AR6KDLLSPEC int MyRegisterRead(unsigned int address, unsigned int *value)
{
    return (DeviceRegisterRead(address,value));
}

AR6KDLLSPEC int MyRegisterWrite(unsigned int address, unsigned int value)
{
    return (DeviceRegisterWrite(address, value));	
}

unsigned int MaskCreate(int low, int high)
{
	unsigned int mask;
	int ib;

	mask=0;
	if(low<=0)
	{
		low=0;
	}
	if(high>=31)
	{
		high=31;
	}
	for(ib=low; ib<=high; ib++)
	{
		mask|=(1<<ib);
	}
	return mask;
}

AR6KDLLSPEC int MyFieldRead(unsigned int address, int low, int high, unsigned int *value)
{
	unsigned int mask;
	unsigned int reg;
	int error;

    error=DeviceRegisterRead(address,&reg);
	if(error==0)
	{
		mask=MaskCreate(low,high);
		*value=(reg&mask)>>low;
	}
	
	return error;
}

AR6KDLLSPEC int MyFieldWrite(unsigned int address, int low, int high, unsigned int value)
{
	unsigned int mask;
	unsigned int reg;
	int error;

    error=DeviceRegisterRead(address,&reg);
	if(error==0)
	{
		mask=MaskCreate(low,high);
		reg &= ~(mask);							// clear bits
		reg |= ((value<<low)&mask);				// set new value
        error=DeviceRegisterWrite(address,reg);
	}
	return error;
}

AR6KDLLSPEC int MyMemoryRead(unsigned int address, unsigned int *buffer, int many)
{
    return (DeviceMemoryRead(address, buffer, many));	
}

AR6KDLLSPEC int MyMemoryWrite(unsigned int address, unsigned int *buffer, int many)
{
    return (DeviceMemoryWrite(address, buffer, many));
}

