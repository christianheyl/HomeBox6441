

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "wlantype.h"


#include "mlibif.h"				// for OScfgRead()

#include "NewArt.h"
#include "smatch.h"
#include "UserPrint.h"

#include "AnwiDriverInterface.h"

#include "common_hwext.h"
#define MBUFFER 1024

// index for the ANWI driver stuff
//
static short devIndex=0;		// this has to be 0


#define A_swap32(x) \
            ((A_UINT32)( \
                                         (((A_UINT32)(x) & (A_UINT32)0x000000ffUL) << 24) | \
                                         (((A_UINT32)(x) & (A_UINT32)0x0000ff00UL) <<  8) | \
                                         (((A_UINT32)(x) & (A_UINT32)0x00ff0000UL) >>  8) | \
                                         (((A_UINT32)(x) & (A_UINT32)0xff000000UL) >> 24) ))


//
// Returns 1 is there is a valid card loaded and ready for operation.
//
ANWIDLLSPEC int AnwiDriverValid()
{
    if(globDrvInfo.pDevInfoArray[devIndex]!=0)
	{
		if(globDrvInfo.pDevInfoArray[devIndex]->pdkInfo!=0)
		{
			if(globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memVirAddr!=0)
			{
				if(globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->aregVirAddr[0]!=0)
				{
					return 1;
				}
			}
		}
	}
	return 0;
}



static FILE *_RegisterDebug=0;
static int _RegisterDebugState=0;	// 1 means write file, 2 means print on screen, 4 means send to cart

//
// Set the time used in file names
//
static void MyRegisterFileName(char *buffer, int max)
{
    time_t now=0;
    struct tm *lnow;
    //
	// get time since we're probably going to use it
	//
	now=time(0);
	lnow=localtime(&now);
	SformatOutput(buffer,max-1,"%02d%02d%02d%02d%02d%02d.rw",
		lnow->tm_year-100,lnow->tm_mon+1,lnow->tm_mday,lnow->tm_hour,lnow->tm_min,lnow->tm_sec);
}


ANWIDLLSPEC void MyRegisterPrint(unsigned long address, unsigned long before, unsigned long after)
{
	char buffer[MBUFFER];
	SformatOutput(buffer,MBUFFER-1,"%04lx: %08lx -> %08lx",address,before,after);

	//
	// put in file
	//
	if(_RegisterDebugState&0x1 && _RegisterDebug)
	{
		fprintf(_RegisterDebug,"%s\n",buffer);
		fflush(_RegisterDebug);
	}
	//
	// show in window
	//
	if(_RegisterDebugState&0x2)
	{
		UserPrint("%s\n",buffer);
	}
	//
	// return to cart
	//
#ifdef DOLATER
	if(_RegisterDebugState&0x4)
	{
	    SendDebug(0,buffer);		// need to record which client
	}
#endif
}


//
// turn on/off output of all register writes to a file
//
ANWIDLLSPEC void MyRegisterDebug(int state)
{
	char buffer[MBUFFER];

	if(state)
	{
		//
		// correct state of file.
		// open it if we need it and it is not open
		// or close it if it is open and we don't need it
		//
		if(state&0x1)
		{
			if(_RegisterDebug==0)
			{
			    MyRegisterFileName(buffer,MBUFFER);
		        _RegisterDebug=fopen(buffer,"w+");
			}
		}
		else
		{
			if(_RegisterDebug!=0)
			{
			 	fclose(_RegisterDebug);
		        _RegisterDebug=0;
			}
		}
		//
		// trap register writes
		//
#ifdef DOLATER
#ifndef __APPLE__
		HalRegisterDebug(MyRegisterPrint);
#endif
#endif
		//
		// record where we want the debug output to go
		//
		_RegisterDebugState=state;
	}
	else 
	{
		//
		// close file if it is open
		//
		if(_RegisterDebug!=0)
		{
	        fclose(_RegisterDebug);
		    _RegisterDebug=0;
		}
		//
		// turn off trapping of register writes
		//
#ifdef DOLATER
#ifndef __APPLE__
		HalRegisterDebug(0);
#endif
#endif
		//
		//  turn off all debug output
		//
		_RegisterDebugState=0;
	}
}

ANWIDLLSPEC int MyRegisterRead(unsigned int address, unsigned int *value)
{
	unsigned int *rptr;
    //
	// check alignment, registers must be addressed as 0, 4, 8, ...
	//
	address=4*(address/4);
	// 
	// CHECK ADDRESS LIMITS 
    //
    if(globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->aregVirAddr[0]!=0 &&
		address>=0 &&
		address<globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->aregRange[0])
	{
	    rptr=(unsigned int *)(address+globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->aregVirAddr[0]);
		*value=(*(volatile unsigned int *)rptr);
#ifdef ENDIAN_SWAP 
        *value=A_swap32(*value);
#endif

		return 0;
	}
	else
	{
		*value=0;
		return -1;
	}
}

ANWIDLLSPEC int MyRegisterWrite(unsigned int address, unsigned int value)
{
	unsigned int *rptr;
    //
	// check alignment, registers must be addressed as 0, 4, 8, ...
	//
	address=4*(address/4);
	// 
	// CHECK ADDRESS LIMITS 
    //
    if(globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->aregVirAddr[0]!=0&&
		address>=0 &&
		address<globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->aregRange[0])
	{
	    rptr=(unsigned int *)(address+globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->aregVirAddr[0]);
		if(_RegisterDebug)
		{
			MyRegisterPrint(address,*rptr,value);
		}
#ifdef ENDIAN_SWAP 
        value=A_swap32((value));
#endif
	    *rptr=value;
	    return 0;
	}
	else
	{
		return -1;
	}
}

ANWIDLLSPEC unsigned int MaskCreate(int low, int high)
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

ANWIDLLSPEC int MyFieldRead(unsigned int address, int low, int high, unsigned int *value)
{
	unsigned int mask;
	unsigned int reg;
	int error;

    error=MyRegisterRead(address,&reg);
	if(error==0)
	{
		mask=MaskCreate(low,high);
		*value=(reg&mask)>>low;
	}
	
	return error;
}

ANWIDLLSPEC int MyFieldWrite(unsigned int address, int low, int high, unsigned int value)
{
	unsigned int mask;
	unsigned int reg;
	int error;

	error=MyRegisterRead(address,&reg);
	if(error==0)
	{
		mask=MaskCreate(low,high);
		reg &= ~(mask);							// clear bits
		reg |= ((value<<low)&mask);				// set new value
		error=MyRegisterWrite(address,reg);
	}
	return error;
}

ANWIDLLSPEC uintptr_t MyMemoryBase()
{
    if(AnwiDriverValid())
	{
        return globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memPhyAddr;
	}
	return 0;
}

ANWIDLLSPEC unsigned char *MyMemoryPtr(unsigned int address)
{
    uintptr_t localAddress = (uintptr_t)address;
	unsigned char *rptr;

    if(globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memVirAddr!=0)
	{
        if(localAddress>=globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memPhyAddr &&
		    localAddress<globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memPhyAddr+globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memSize)
		{
	        localAddress-=globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memPhyAddr;
		    localAddress+=globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memVirAddr;
	        rptr=(unsigned char *)localAddress;
			return rptr;
		}
	}
    return 0;
}

ANWIDLLSPEC int MyMemoryRead(unsigned int address, unsigned int *buffer, int many)
{
    uintptr_t localAddress = (uintptr_t)address;
	unsigned char *rptr;

    if(globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memVirAddr!=0)
	{
        if(localAddress>=globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memPhyAddr &&
		    localAddress<globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memPhyAddr+globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memSize)
		{
	        localAddress-=globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memPhyAddr;
		    localAddress+=globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memVirAddr;
	        rptr=(unsigned char *)localAddress;
	        memcpy(buffer,rptr,many);
			return 0;
		}
		else
		{
		    memset(buffer,0,many);
		    return -1;
		}
	}
	else
	{
		memset(buffer,0,many);
		return -1;
	}
}

ANWIDLLSPEC int MyMemoryWrite(unsigned int address, unsigned int *buffer, int many)
{
    uintptr_t localAddress = (uintptr_t)address;
	unsigned char *rptr;

    if(globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memVirAddr!=0)
	{
        if(localAddress>=globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memPhyAddr &&
		    localAddress<globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memPhyAddr+globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memSize)
		{
	        localAddress-=globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memPhyAddr;
		    localAddress+=globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memVirAddr;
	        rptr=(unsigned char *)localAddress;
	        memcpy(rptr,buffer,many);
	        return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}


ANWIDLLSPEC int AnwiDriverDetach()
{
	if(AnwiDriverValid())
	{
		deviceCleanup(0);
		envCleanup(1);
	}
	globDrvInfo.pDevInfoArray[devIndex]=0;

    return 0;
}


//
// Grab the device id from the pci bus
// is there anything else we should check, like an Atheros id?
// how do we get this for other bus types?
//
ANWIDLLSPEC int AnwiDriverDeviceIdGet()
{
	unsigned int address, value;
	int devid;
	unsigned int devNum;

	devNum=0;			// why do we still need this????
	address=0;
    value=OScfgRead(devNum,address);
	devid=((value>>16)&0xffff);

	return devid;
}

	

ANWIDLLSPEC int AnwiDriverAttach(int devid)
{
    DK_DEV_INFO *pdkInfo=0;
    int error;
	devIndex=0;
	//
	// connect to the ANWI driver
	//
    error=envInit(FALSE, 1); 
    if(error!=0) 
	{
	    globDrvInfo.pDevInfoArray[devIndex]=0;
        return -2;
    }

	error=deviceInit(devIndex,0,pdkInfo);
    if(error!=0) 
	{
		envCleanup(1);
	    globDrvInfo.pDevInfoArray[devIndex]=0;
        return -3;
    }
#ifdef UNUSED
	//
	// get the device id from the PCI bus
	//
	if(devid<=0)
	{
	    devid=DeviceIdGet();
	    UserPrint("devid=%x\n",devid);
	}

    return devid;
#else
	return 0;
#endif
}

ANWIDLLSPEC uintptr_t AnwiDriverMemoryMap()
{
	if(globDrvInfo.pDevInfoArray[devIndex]!=0)
	{
		if(globDrvInfo.pDevInfoArray[devIndex]->pdkInfo!=0)
		{
            return globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memPhyAddr;
        }
    }
    return 0;
}


ANWIDLLSPEC int AnwiDriverMemorySize()
{
	if(globDrvInfo.pDevInfoArray[devIndex]!=0)
	{
		if(globDrvInfo.pDevInfoArray[devIndex]->pdkInfo!=0)
		{
	        return globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->memSize;
        }
    }
    return 0;
}

ANWIDLLSPEC uintptr_t AnwiDriverRegisterMap()
{
	if(globDrvInfo.pDevInfoArray[devIndex]!=0)
	{
		if(globDrvInfo.pDevInfoArray[devIndex]->pdkInfo!=0)
		{
            return globDrvInfo.pDevInfoArray[devIndex]->pdkInfo->aregVirAddr[0];	// register map
        }
    }
    return 0;
}
