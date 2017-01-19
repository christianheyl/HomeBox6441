/*
 *  Copyright ?2005 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>

#include "smatch.h"
#include "ErrorPrint.h"
#include "LinkError.h"
#include "LinkTxRx.h"
#include "LinkLoad.h"
#include "RxDescriptor.h"
#include "TxDescriptor.h"
#include "DeviceError.h"


#define MBUFFER 1024


static void *LinkLibrary;
static char LinkLibraryName[MBUFFER];

#if defined(Linux)
#define SUFFIX  "so"
#elif defined(__APPLE__)
#define SUFFIX   "dylib"
#else
#define SUFFIX  "NONE"
#endif

//
// Returns a pointer to the specified function.
//
static void *LinkFunctionFind(char *prefix, char *function)
{
    void *f;
    char buffer[MBUFFER];

    if(LinkLibrary!=0)
    {
        //
        // try adding the prefix in front of the name
        //
        SformatOutput(buffer,MBUFFER-1,"%s%s",prefix,function);
        buffer[MBUFFER-1]=0;
        dlerror(); // clear any error
        f=dlsym(LinkLibrary, buffer);
        if(f==0)
        {
			//
			// try without the prefix in front of the name
			//
            f=dlsym(LinkLibrary, function);
            if (f == 0)
            {
                ErrorPrint (LinkNoFunction, function);
                return 0;
            }
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
    if(LinkLibrary!=0)
    {
	    dlclose(LinkLibrary);
        LinkLibrary=0;
		LinkLibraryName[0]=0;
    }
}


//
// loads the Link control dll 
//
int LinkLoad(char *dllname)
{
    char buffer[MBUFFER],fullname[MBUFFER];
    int error;
	char * (*function)();
	char *prefix;

    SformatOutput(buffer,MBUFFER-1,"lib%s.%s",dllname, SUFFIX);
    if(LinkLibrary!=0)
	{
		//
		// Return immediately if the library is already loaded.
		//
        if(Smatch(LinkLibraryName,buffer))
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
    LinkLibrary = dlopen(buffer, RTLD_LAZY | RTLD_GLOBAL);
    if(LinkLibrary==0) 
    {
        UserPrint ("%s\n", dlerror());
        ErrorPrint(LinkNoLoad,buffer);
		return -1;
	}
	//GetModuleFileName(LinkLibrary,fullname,MBUFFER-1);
    if (getcwd(fullname,MBUFFER-1))
    {
        strcat(fullname, "/");
        strcat(fullname, buffer);
    }
    else
    {
        strcpy(fullname, buffer);
    }
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
    SformatOutput(LinkLibraryName,MBUFFER-1,"%s",buffer);
	LinkLibraryName[MBUFFER-1]=0;

    return 0;
}

