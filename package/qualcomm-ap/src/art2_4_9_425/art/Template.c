#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "CommandParse.h"
#include "NewArt.h"
#include "ParameterSelect.h"
#include "ParameterParse.h"
#include "Template.h"
#include "ErrorPrint.h"
#include "ParseError.h"
#include "Card.h"
#include "Channel.h"


#ifdef UNUSED
#include "Ar9300EepromSave.h"

//
// hal header files
//
#include "ah.h"
#include "ah_internal.h"
#include "ar9300.h"
#include "ar9300eep.h"

// 
// this is the hal pointer, 
// returned by ath_hal_attach
// used as the first argument by most (all?) HAL routines
//
struct ath_hal *AH;
#endif


#define MBUFFER 1024

#define MLOOP 100

#include "Device.h"
#include "templatelist.h"

static struct _ParameterList TemplateParameter[]=
{
	TEMPLATE_PREFERENCE,
	TEMPLATE_ALLOWED,
	TEMPLATE_MEMORY,
	TEMPLATE_SIZE,
	TEMPLATE_COMPRESS,
	TEMPLATE_OVERWRITE,
	TEMPLATE_INSTALL,
};


void TemplateParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(TemplateParameter)/sizeof(TemplateParameter[0]);
    list->special=TemplateParameter;
}


//
// interpolates into the stored data structure and returns the value that will be used
//
void TemplateCommand(int client)
{
	int np, ip;
	char *name;	
	int error;
	int index;
    int code;
	int preference,npreference;
	int allowed[MALLOWED],nallowed;
	int overwrite,noverwrite;
	int compress,ncompress;
	int memory,nmemory;
	int address,naddress;
	int install,ninstall;
	//
	// install default parameter values
	//
	error=0;
	preference= -1;
	npreference= -1;
	overwrite= -1;
	noverwrite= -1;
	compress= -1;
	ncompress= -1;
	memory= -1;
	nmemory= -1;
	address=-1;
	naddress= -1;
	//
	// parse arguments and do it
	//
	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		index=ParameterSelectIndex(name,TemplateParameter,sizeof(TemplateParameter)/sizeof(TemplateParameter[0]));
		if(index>=0)
		{
			code=TemplateParameter[index].code;
			switch(code) 
			{
				case TemplatePreference:
					npreference=ParseIntegerList(ip,name,&preference,&TemplateParameter[index]);
					if(npreference<=0)
					{
						error++;
					}
					break;
				case TemplateAllowed:
					nallowed=ParseIntegerList(ip,name,allowed,&TemplateParameter[index]);		
					if(nallowed<=0)
					{
						error++;
					}
					break;
				case TemplateCompress:
					ncompress=ParseIntegerList(ip,name,&compress,&TemplateParameter[index]);		
					if(ncompress<=0)
					{
						error++;
					}
					break;
				case TemplateOverwrite:
					noverwrite=ParseIntegerList(ip,name,&overwrite,&TemplateParameter[index]);		
					if(noverwrite<=0)
					{
						error++;
					}
					break;
				case TemplateMemory:
					nmemory=ParseIntegerList(ip,name,&memory,&TemplateParameter[index]);
					if(nmemory<=0)
					{
						error++;
					}
					break;
				case TemplateSize:
					naddress=ParseIntegerList(ip,name,&address,&TemplateParameter[index]);
					if(naddress<=0)
					{
						error++;
					}
					break;
				case TemplateInstall:
					ninstall=ParseIntegerList(ip,name,&install,&TemplateParameter[index]);
					if(ninstall<=0)
					{
						error++;
					}
					break;
				default:
					error++;
					ErrorPrint(ParseBadParameter,name);
					break;
			}
		}
		else
		{
			error++;
			ErrorPrint(ParseBadParameter,name);
		}
	}

	if(error<=0)
	{
		if(npreference==1)
		{
			DeviceEepromTemplatePreference(preference);
		}
		if(nallowed>=0)
		{
			DeviceEepromTemplateAllowed((unsigned int*)allowed,(unsigned int)nallowed);
		}
		if(ncompress==1)
		{
			DeviceEepromCompress(compress);
		}
		if(noverwrite==1)
		{
			DeviceEepromOverwrite(overwrite);
		}
		if(naddress==1)
		{
			if(address>0)
			{
				address--;
			}
			DeviceCalibrationDataAddressSet(address);
		}
		if(nmemory==1)
		{
			DeviceCalibrationDataSet(memory);
		}
		if(ninstall==1 && npreference==1)
		{
			if(install==1 || (install==2 && DeviceCalibrationDataGet()==DeviceCalibrationDataNone))
			{
				DeviceEepromTemplateInstall(preference);
				ChannelCalculate();
			}
		}
	}
	
	SendDone(client);
}





