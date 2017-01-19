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
#include "LinkError.h"
#include "LinkTxRx.h"
#include "LinkLoad.h"
#include "RxDescriptor.h"
#include "TxDescriptor.h"
#include "DeviceError.h"
#include "DllIfOs.h"

#define MBUFFER 1024

static char LinkLibraryName[MBUFFER];

//
// Returns a pointer to the specified function.
//
static void *LinkFunctionFind(char *prefix, char *function)
{
    void *f;
    char buffer[MBUFFER];

    if(osCheckLibraryHandle(LinkLibrary)!=0)
    {
        //
        // try adding the prefix in front of the name
        //
        SformatOutput(buffer,MBUFFER-1,"%s%s",prefix,function);
        buffer[MBUFFER-1]=0;
 
        f=osGetFunctionAddress(buffer,LinkLibrary);

        if(f==0)
        {
			//
			// try without the prefix in front of the name
			//

            f=osGetFunctionAddress(function,LinkLibrary);

        }

        return f;
    }
    return 0;
}


//
// unload the dll
//
void LinkUnload()
{
    if(osCheckLibraryHandle(LinkLibrary)!=0)
    {
        osDllUnload(LinkLibrary);
		LinkLibraryName[0]=0;
    }
}


//
// loads the Link control dll 
//
int LinkLoad(char *dllname)
{
    char fullname[MBUFFER];
    int error;
	char * (*function)();
	char *prefix;

    if(osCheckLibraryHandle(LinkLibrary)!=0)
	{
		//
		// Return immediately if the library is already loaded.
		//
		if(Smatch(LinkLibraryName,dllname))
		{
			return 0;
		}
		//
		// otherwise, unload previous library
		//
		else
		{
			LinkUnload();
		}
    }
    // 
    // Load DLL file
    //
    error = osDllLoad(dllname,fullname,LinkLibrary);
    if(error!=0)
    {
        ErrorPrint(LinkNoLoad,dllname);
		return -1;
    }
	fullname[MBUFFER-1]=0;
    //
    // Clear all function pointers
    //
	LinkFunctionReset();
	//
	// see if the dll defines a prefix for all function names
	//
	prefix=dllname;
	function=LinkFunctionFind(dllname,"LinkPrefix");
	if(function!=0)
	{
		prefix=(char *)function();
		if(prefix==0)
		{
			prefix=dllname;
		}
	}
	// 
    // Get function pointers for this Link
	//
	function=LinkFunctionFind(prefix,"LinkSelect");
	if(function==0)
	{
		ErrorPrint(LinkLoadBad,prefix,fullname);
		LinkUnload();
		return -1;
	}
	error=(int)function();
	if(error!=0)
	{
		ErrorPrint(LinkLoadBad,prefix,fullname);
		LinkUnload();
		return -1;
	}

	ErrorPrint(LinkLoadGood,prefix,fullname);
	SformatOutput(LinkLibraryName,MBUFFER-1,"%s",dllname);
	LinkLibraryName[MBUFFER-1]=0;

    return 0;
}

