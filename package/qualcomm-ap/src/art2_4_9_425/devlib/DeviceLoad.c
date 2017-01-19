/*
 *  Copyright ?2005 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "smatch.h"
#include "ErrorPrint.h"
#include "DeviceError.h"
#include "Device.h"
#include "DeviceLoad.h"
#include "RxDescriptor.h"
#include "TxDescriptor.h"
#include "NartError.h"
#include "DllIfOs.h"
#include "SetConfig.h"

#define MBUFFER 1024


static char _DeviceLibraryName[MBUFFER];
static char _DeviceFullName[MBUFFER];


char *DeviceFullName(void)
{
	return _DeviceFullName;
}

//
// Returns a pointer to the specified function.
//
static void *DeviceFunctionFind(char *prefix, char *function)
{
    void *f;
    char buffer[MBUFFER];

    if(osCheckLibraryHandle(DeviceLibrary)!=0)
    {
        //
        // try adding the prefix in front of the name
        //
        SformatOutput(buffer,MBUFFER-1,"%s%s",prefix,function);
        buffer[MBUFFER-1]=0;
        f=osGetFunctionAddress(buffer,DeviceLibrary);
        if(f==0)
        {
			//
			// try without the prefix in front of the name
			//
			f=osGetFunctionAddress(function,DeviceLibrary);
        }
        return f;
    }
    return 0;
}


//
// unload the dll
//
void DeviceUnload()
{
    if(osCheckLibraryHandle(DeviceLibrary)!=0)
    {
        osDllUnload(DeviceLibrary);
		_DeviceLibraryName[0]=0;
    }
    //
    // Clear all function pointers
    //
	DeviceFunctionReset();    
}


//
// loads the device control dll 
//
int DeviceLoad(char *dllname)
{
    //char buffer[MBUFFER];
    int error;
	char * (*function)();
	char *prefix;
	char *name;
	char *version;
	char *date;

	if(osCheckLibraryHandle(DeviceLibrary)!=0)
	{
		//
		// Return immediately if the library is already loaded.
		//
		if(Smatch(_DeviceLibraryName,dllname))
		{
			return 0;
		}
		//
		// otherwise, unload previous library
		//
		else
		{
			DeviceUnload();
		}
    }
    // 
    // Load DLL file
    //
    error=osDllLoad(dllname,_DeviceFullName,DeviceLibrary);

    if(error)
    {
        ErrorPrint(DeviceNoLoad,dllname);
        return -1;
    }

	_DeviceFullName[MBUFFER-1]=0;
    //
    // Clear all function pointers
    //
	DeviceFunctionReset();
	//
	// see if the dll defines a prefix for all function names
	//
	prefix=dllname;
	function=DeviceFunctionFind(dllname,"DevicePrefix");
	if(function!=0)
	{
		prefix=(char *)function();
		if(prefix==0)
		{
			prefix=dllname;
		}
	}
	// 
    // Get function pointers for this device
	//
	function=DeviceFunctionFind(prefix,"DeviceSelect");
	if(function==0)
	{
		ErrorPrint(DeviceLoadBad,prefix,DeviceFullName);
		DeviceUnload();
		return -1;
	}
	error=(int)function();
	if(error!=0)
	{
		ErrorPrint(DeviceLoadBad,prefix,DeviceFullName);
		DeviceUnload();
		return -1;
	}

	name=DeviceName();
	if(name!=0)
	{
		prefix=name;
	}
	version=DeviceVersion();
	date=DeviceBuildDate();

	ErrorPrint(DeviceLoadGood,prefix,_DeviceFullName,version,date);
	SformatOutput(_DeviceLibraryName,MBUFFER-1,"%s",dllname);
	_DeviceLibraryName[MBUFFER-1]=0;

    // download setconfig list, if any, before attacch device
    SetConfigProcess();

    return 0;
}

