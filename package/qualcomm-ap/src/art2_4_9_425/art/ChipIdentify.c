#include <stdlib.h>
#include <string.h>
#include "CommandParse.h"
#include "ParameterParse.h"
#include "ParseError.h"
#include "ParameterSelect.h"

#include "smatch.h"
#include "ErrorPrint.h"
#include "UserPrint.h"

#include "Device.h"
#include "ChipIdentify.h"
#ifdef DYNAMIC_DEVICE_DLL
#include "DeviceLoad.h"
#include "LinkLoad.h"
#endif

#define MBUFFER 1024
#define MAX_DLL	5
#define CHIP_MAC_ID (0x4020)


struct _DevidToName DevidToNameA[MAX_DLL];
int numDLL_addition=0;

char *LoadDLL(char *dllName, int *devid)
{
	int error;
	error=DeviceLoad(dllName);
	if(error==0) {	// this hal dll is loaded
		*devid=DeviceChipIdentify();	
		if (*devid>0)
			return dllName;
		else	// the loaded HAL dll doesn't support this chip
			return 0;
	} else
		return 0;
}

char *DevidToLibrary(int devid)
{
	int error;
	int halSize=0, i=0;

	halSize = sizeof(DevidToName)/sizeof(DevidToName[0]);
	// loop through all nart hardcoded devid,hal pair
	for (i=0; i<halSize; i++) 
	{
		if(devid==DevidToName[i].devid)
		{
			error=DeviceLoad(DevidToName[i].name);
			if(error==0)	// this hal dll is loaded
				return DevidToName[i].name;
			else	// this hal dll is failed to load
				return 0;

		}
	}
	// the devid is not in nart hardcoded devid,hal pair list
	// check if user has been add any devid,hal pair from cart command (HAL name=halName;devid=xxxx)
	for (i=0; i<numDLL_addition; i++) 
	{
		if (DevidToNameA[i].name==0)
			continue;
		if(devid==DevidToNameA[i].devid)
		{
			error=DeviceLoad(DevidToNameA[i].name);
			if(error==0)	// this hal dll is loaded
				return DevidToNameA[i].name;
			else	// this hal dll is failed to load
				return 0;
		}
	}
	return 0;
}


char *SearchLibrary(int *devid)
{
	int error;
	int halSize=0, i=0;
	*devid=0;

	halSize = sizeof(HAL_Dll)/sizeof(HAL_Dll[0]);
	// loop through all nart hardcoded hal dll list
	for (i=0; i<halSize; i++) {
		error=DeviceLoad(HAL_Dll[i]);
		if(error==0)	// this hal dll is loaded 
		{
			*devid=DeviceChipIdentify();	
			if (*devid>0)
				break;
			else {	// the loaded HAL dll doesn't support this chip
				DeviceUnload();
			}
		}
	}
	if (i<halSize)
	{
	    return HAL_Dll[i];
	} else {
		// not found in nart hard coded hal dll list
		// check if user has been add any hal from cart command (HAL name=halName;devid=xxxx)
		for (i=0; i<numDLL_addition; i++) 
		{
			if (DevidToNameA[i].name==0)
				continue;
			error=DeviceLoad(DevidToNameA[i].name);
			if(error==0)	// this hal dll is loaded 
			{
				*devid=DeviceChipIdentify();	
				if (*devid>0)
					break;
				else {	// the loaded HAL dll doesn't support this chip
					DeviceUnload();
				}
			}
		}
		if (i<numDLL_addition)
			return DevidToNameA[i].name;
	}
    return 0;
}

enum
{
    _HALCommandName=0,
	_HALCommandDevid,
};

static struct _ParameterList HALParameter[]=
{
	{_HALCommandName,{"name","dll",0},"HAL dll name",'t',0,1,1,1,0,0,"ar9300_9-3-1",0,0},
	{_HALCommandDevid,{"devid",0,0},"devid",'d',0,1,1,1,0,0,"0",0,0},
}; 

void HALParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(HALParameter)/sizeof(HALParameter[0]);
    list->special=HALParameter;
} 

void HALCommand()
{
	char str[MBUFFER];
	int error;
	int ip, nparam;
	int code, index;
	char *name;
	//
	// default values
	//
	SformatOutput(str,MBUFFER-1,"");
	error=0;
	//
	// add the parameters
	//
	nparam=CommandParameterMany();
	for(ip=0; ip<nparam; ip++)
	{
		name=CommandParameterName(ip);
        code=ParameterSelect(name,HALParameter,sizeof(HALParameter)/sizeof(struct _ParameterList));
		switch (code) 
		{
			case _HALCommandName: 
				index=numDLL_addition;
				DevidToNameA[index].name = (char *) malloc (20);
				numDLL_addition++;
				SformatOutput(str,MBUFFER-1,"%s",CommandParameterValue(ip,0));
				str[MBUFFER-1]=0;
				sprintf(DevidToNameA[index].name, "%s", str);
				break;
			case _HALCommandDevid: 
				SformatOutput(str,MBUFFER-1,"%s",CommandParameterValue(ip,0));
				str[MBUFFER-1]=0;
				DevidToNameA[index].devid=atoi(str);
				break;
			default:
				ErrorPrint(ParseBadParameter,name);
				error++;
				break;
		}
	}
}

