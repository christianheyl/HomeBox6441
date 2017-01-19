#include <dlfcn.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "smatch.h"
#include "ErrorPrint.h"
#include "NartError.h"
#include "DllIfOs.h"

#define MBUFFER 1024

#if defined(Linux)
#define SUFFIX  "so"
#elif defined(__APPLE__)
#define SUFFIX   "dylib"
#else
#define SUFFIX  "NONE"
#endif

static void *_DeviceLibrary=0;
static void *_LinkLibrary=0;
static void *_CalibrationLibrary=0;
static void *_Library=0;

int osDllLoad(char *dllname, char *FullName, enum LibraryType libraryType)
{
    char buffer[MBUFFER];
    switch(libraryType)
    {
        case DeviceLibrary:
            SformatOutput(buffer,MBUFFER-1,"lib%s.%s",dllname, SUFFIX);
            break;
        case LinkLibrary:
            SformatOutput(buffer,MBUFFER-1,"lib%s.%s",dllname, SUFFIX);
            break;
        case CalibrationLibrary:
            SformatOutput(buffer,MBUFFER-1,"lib%s.%s",dllname, SUFFIX);
            break;
        default:
            return -1;
    }
    printf("%s[%d] library name = %s\n",__func__,__LINE__,buffer);
    _Library = dlopen(buffer, RTLD_LAZY | RTLD_GLOBAL);
    if(!_Library)
    {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }
    if (getcwd(FullName,MBUFFER-1))
    {
        strcat(FullName, "/");
        strcat(FullName, buffer);
    }
    else
    {
        strcpy(FullName, buffer);
    }
    
    switch(libraryType)
    {
        case DeviceLibrary:
            _DeviceLibrary=_Library;
            break;
        case LinkLibrary:
            _LinkLibrary=_Library;
            break;
        case CalibrationLibrary:
            _CalibrationLibrary=_Library;
            break;
        default:
            return -1;
    }
    printf("%s[%d] FullName = %s\n", __func__,__LINE__,FullName);
    return 0;    
}


void osDllUnload(enum LibraryType libraryType) 
{
    switch(libraryType)
    {
        case DeviceLibrary:
            dlclose(_DeviceLibrary);
            _DeviceLibrary=0;
        break;
        case LinkLibrary:
            dlclose(_LinkLibrary);
            _LinkLibrary=0;
        break;
        case CalibrationLibrary:
            dlclose(_CalibrationLibrary);
            _CalibrationLibrary=0;
        break;
        default:
            return;
    }
}

void *osGetFunctionAddress(char *function, enum LibraryType libraryType)
{
    switch(libraryType)
    {
        case DeviceLibrary:
            return dlsym(_DeviceLibrary, function);
            break;
        case LinkLibrary:
            return dlsym(_LinkLibrary, function);
            break;
        case CalibrationLibrary:
            return dlsym(_CalibrationLibrary, function);
            break;
        default:
            return 0;
    }
}       

int osCheckLibraryHandle(enum LibraryType libraryType)
{
    switch(libraryType)
    {
        case DeviceLibrary:
            if(_DeviceLibrary==0)
                return 0;
            else
                return 1;
            break;
        case LinkLibrary:
            if(_LinkLibrary==0)
                return 0;
            else
                return 1;
        case CalibrationLibrary:
            if(_CalibrationLibrary==0)
                return 0;
            else
                return 1;
        default:
            return 0;
    }
}



