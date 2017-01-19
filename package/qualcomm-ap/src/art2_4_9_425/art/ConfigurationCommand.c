#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "wlantype.h"
#include "smatch.h"
#include "UserPrint.h"
#include "TimeMillisecond.h"
#include "CommandParse.h"
#include "NewArt.h"
#include "MyDelay.h"
#include "ParameterSelect.h"
#include "Card.h"
#include "Field.h"

#include "Device.h"

#include "ParameterParse.h"
#include "ParameterParseNart.h"
#include "Link.h"
#include "Calibrate.h"
#include "ConfigurationCommand.h"
//#include "ConfigurationSet.h"
//#include "ConfigurationGet.h"
#include "ConfigurationStatus.h"

#include "ErrorPrint.h"
#include "CardError.h"
#include "ParseError.h"
#include "NartError.h"

#ifdef UNUSED
#include "Ar9300EepromParameter.h"
#include "Ar9300SetList.h"


//
// hal header files
//
#include "ah.h"
#include "ah_internal.h"
#include "ar9300.h"
#include "ar9300eep.h"

#include "Ar9300EepromStructSet.h"
#include "mEepStruct9300.h"
#include "ar9300_EEPROM_print.h"
#include "Ar9300EepromSave.h"
#include "Ar9300PcieConfig.h"
#endif

#include "templatelist.h"

#define MBUFFER 1024

#ifndef max
#define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif

#ifdef UNUSED
extern int Ar9300FlashCal(int value);
#endif

#define MALLOWED 100

enum
{
	CheckEepromAll=1,
};

static int CheckAllDefault=0;
struct _ParameterList ConfigurationCheckParameter[]=
{
	TEMPLATE_MEMORY_READ,
	TEMPLATE_SIZE_READ,
	{CheckEepromAll,{"all",0,0},"include unchanged fields?",'z',0,1,1,1,0,0,&CheckAllDefault,
		sizeof(TemplateLogicalParameter)/sizeof(TemplateLogicalParameter[0]),TemplateLogicalParameter},
};

struct _ParameterList ConfigurationRestoreParameter[]=
{
	TEMPLATE_PREFERENCE,
	TEMPLATE_MEMORY_READ,
	TEMPLATE_SIZE_READ,
};

static struct _ParameterList ConfigurationSaveParameter[]=
{
	TEMPLATE_ALLOWED,
	TEMPLATE_MEMORY,
	TEMPLATE_SIZE,
	TEMPLATE_COMPRESS,
	TEMPLATE_OVERWRITE,
    TEMPLATE_SECTION(MALLOWED),
};

enum 
{
	PcieAction=1000,
	PcieAddress,
	PcieValue,
	PcieAdd,
	PcieModify,
	PcieDelete,
	PcieCommit,
	PcieRead,
	PcieList,
    PcieSize,
};

static int PcieActionDefault=PcieCommit;

static struct _ParameterList PcieActionParameter[]=
{
	{PcieCommit,{"commit","save",0},"save the (register, value) pairs in the power on initialization space",'z',0,1,1,1,0,0,0,0,0},
	{PcieRead,{"read",0,0},"read the (register, value) pairs in the power on initialization space",'z',0,1,1,1,0,0,0,0,0},
	{PcieList,{"list","print",0},"list the (register, value) pair to the power on initialization space",'z',0,1,1,1,0,0,0,0,0},
	{PcieAdd,{"add",0,0},"add a (register, value) pair to the power on initialization space",'z',0,1,1,1,0,0,0,0,0},
//	{PcieModify,{"modify",0,0},"modify a (register, value) pair in the power on initialization space",'z',0,1,1,1,0,0,0,0,0},
	{PcieDelete,{"delete",0,0},"delete a (register, value) pair from the power on initialization space",'z',0,1,1,1,0,0,0,0,0},
};

static int ValueSizeMinimum=1;
static int ValueSizeMaximum=4;

static struct _ParameterList PcieSaveParameter[]=
{
	{TemplateMemory,{"memory","caldata",0},"memory type used for pcie initilization data",'z',0,1,1,1,0,0,&TemplateMemoryDefault,
	    sizeof(TemplateMemoryParameter)/sizeof(TemplateMemoryParameter[0]),TemplateMemoryParameter},
	{PcieAction,{"action",0,0},"action to be performed",'z',0,1,1,1,0,0,&PcieActionDefault,
	    sizeof(PcieActionParameter)/sizeof(PcieActionParameter[0]),PcieActionParameter},
	{PcieAddress,{"register","address","a"},"register address",'x',0,1,1,1,0,0,0,0,0},
	{PcieValue,{"value","v",0},"register value",'x',0,1,1,1,0,0,0,0,0},
    {PcieSize,{"size",0,0},"1, 2 or 4-byte value",'u',0,1,1,1,&ValueSizeMinimum,&ValueSizeMaximum,&ValueSizeMaximum,0,0},
};

void ConfigurationCheckParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(ConfigurationCheckParameter)/sizeof(ConfigurationCheckParameter[0]);
    list->special=ConfigurationCheckParameter;
}

void ConfigurationRestoreParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(ConfigurationRestoreParameter)/sizeof(ConfigurationRestoreParameter[0]);
    list->special=ConfigurationRestoreParameter;
}

void ConfigurationSaveParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(ConfigurationSaveParameter)/sizeof(ConfigurationSaveParameter[0]);
    list->special=ConfigurationSaveParameter;
}

void ConfigurationPcieSaveParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(PcieSaveParameter)/sizeof(PcieSaveParameter[0]);
    list->special=PcieSaveParameter;
}


static int PrintMacAddress(char *buffer, int max, unsigned char mac[6])
{
    int nc;
    //
	// try with colons
	//
	nc=SformatOutput(buffer,max-1,"%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    buffer[max-1]=0;
	if (nc!=17) 
		return ERR_VALUE_BAD;
	return VALUE_OK;
}

static int PrintClient;

static void Print(char *format, ...)
{
    va_list ap;
    char buffer[MBUFFER];

    va_start(ap, format);
#if defined LINUX || defined __APPLE__ 
    vsnprintf(buffer,MBUFFER,format,ap);
#else
    _vsnprintf(buffer,MBUFFER,format,ap);
#endif
    va_end(ap);
	ErrorPrint(NartData,buffer);
}

//
// Commit the internal structure to the card.
// If OTP is in use, this command consumes additional card memory.
// If Eeprom or FLASH is in use, the memory is overwritten.
//
void ConfigurationCheckCommand(int client)
{
//    char buffer[MBUFFER];
	int all,nall;
	int memory,nmemory;
	int address,naddress;
	int ip, np;
	int error;
	char *name;
	int code;
	int index;
	//
	// install default parameter values
	//
	error=0;
	nmemory= -1;
	memory=TemplateMemoryDefaultRead;
	naddress= -1;
	address=TemplateSizeDefaultRead;
	nall= -1;
	all=0;
	//
	// parse arguments and do it
	//
	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		index=ParameterSelectIndex(name,ConfigurationCheckParameter,sizeof(ConfigurationCheckParameter)/sizeof(ConfigurationCheckParameter[0]));
		if(index>=0)
		{
			code=ConfigurationCheckParameter[index].code;
			switch(code) 
			{
				case CheckEepromAll:
					nall=ParseIntegerList(ip,name,&all,&ConfigurationCheckParameter[index]);
					if(nall<=0)
					{
						error++;
					}
					break;
				case TemplateMemory:
					nmemory=ParseIntegerList(ip,name,&memory,&ConfigurationCheckParameter[index]);
					if(nmemory<=0)
					{
						error++;
					}
					break;
				case TemplateSize:
					naddress=ParseIntegerList(ip,name,&address,&ConfigurationCheckParameter[index]);
					if(naddress<=0)
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
		//
		// if there's no card loaded, return error
		//
		if(CardCheckAndLoad(client)!=0)
		{
			ErrorPrint(CardNoneLoaded);
			error= -1;
		}
		else
		{
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

			PrintClient=client;
			DeviceEepromReport(Print,all);
			FreeMemoryPrint();
		}
	}

	SendDone(client);
}


//
// Commit the internal structure to the card.
// If OTP is in use, this command consumes additional card memory.
// If Eeprom or FLASH is in use, the memory is overwritten.
//
void ConfigurationSaveCommand(int client)
{
    int error;
    char buffer[MBUFFER];
	int np, ip;
	char *name;	
	int index;
    int code;
	unsigned int preference;
	int npreference;
	unsigned int allowed[MALLOWED];
	int nallowed;
	int overwrite=0,noverwrite;
	int compress=0,ncompress;
	int memory=0,nmemory;
	int address=0,naddress;
    int section[10], nsection, i;
    unsigned int sectionMask;
	//
	// install default parameter values
	//
	error=0;
	preference= -1;
	npreference= -1;
	allowed[0]= -1;
	nallowed= -1;
	overwrite= -1;
	noverwrite= -1;
	compress= -1;
	ncompress= -1;
	memory= -1;
	nmemory= -1;
	address= -1;
	naddress= -1;
    section[0]= -1 ;
    nsection = -1;
    sectionMask = 0;      // all sections
	//
	// parse arguments and do it
	//
	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		index=ParameterSelectIndex(name,ConfigurationSaveParameter,sizeof(ConfigurationSaveParameter)/sizeof(ConfigurationSaveParameter[0]));
		if(index>=0)
		{
			code=ConfigurationSaveParameter[index].code;
			switch(code) 
			{
				case TemplatePreference:
					npreference=ParseIntegerList(ip,name,(int *)&preference,&ConfigurationSaveParameter[index]);
					if(npreference<=0)
					{
						error++;
					}
					break;
				case TemplateAllowed:
					nallowed=ParseIntegerList(ip,name,(int *)allowed,&ConfigurationSaveParameter[index]);		
					if(nallowed<=0)
					{
						error++;
					}
					break;
				case TemplateCompress:
					ncompress=ParseIntegerList(ip,name,&compress,&ConfigurationSaveParameter[index]);		
					if(ncompress<=0)
					{
						error++;
					}
					break;
				case TemplateOverwrite:
					noverwrite=ParseIntegerList(ip,name,&overwrite,&ConfigurationSaveParameter[index]);		
					if(noverwrite<=0)
					{
						error++;
					}
					break;
				case TemplateMemory:
					nmemory=ParseIntegerList(ip,name,&memory,&ConfigurationSaveParameter[index]);
					if(nmemory<=0)
					{
						error++;
					}
					break;
				case TemplateSize:
					naddress=ParseIntegerList(ip,name,&address,&ConfigurationSaveParameter[index]);
					if(naddress<=0)
					{
						error++;
					}
					break;
                case TemplateSection:
                    nsection=ParseIntegerList(ip,name,section,&ConfigurationSaveParameter[index]);
					if(nsection<=0)
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
		//
		// check if card is loaded
		//
		if(!CardValidReset())
		{
			ErrorPrint(CardNoLoadOrReset);
		}
		else
		{
			if(npreference==1)
			{
				DeviceEepromTemplatePreference(preference);
			}
			if(nallowed>0)
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
				DeviceEepromSaveMemorySet(memory);
			}
            if(nsection > 0)
            {
                for (i = 0; i < nsection; ++i)
                {
                    sectionMask |= (1 << section[i]);
                }
            }
            DeviceEepromSaveSectionSet(sectionMask);
			//
			// write the chip memory
			//
			error=DeviceConfigurationSave();
			if(error<0)
			{
				if(error== -1)
				{
					SformatOutput(buffer,MBUFFER-1,"Can't write eeprom.");
					buffer[MBUFFER-1]=0;
					SendError(client,buffer);
				}
				else if(error== -2)
				{
					SformatOutput(buffer,MBUFFER-1,"Too many eeprom write errors.");
					buffer[MBUFFER-1]=0;
					SendError(client,buffer);
				}
				else if(error== -3)
				{
					SformatOutput(buffer,MBUFFER-1,"Fatal eeprom error. Bad chip.");
					buffer[MBUFFER-1]=0;
					SendError(client,buffer);
				}
				else if(error<0)
				{
					SformatOutput(buffer,MBUFFER-1,"Can't write eeprom structure %d",error);
					buffer[MBUFFER-1]=0;
					SendError(client,buffer);
				}
				ErrorPrint(CardCalibrationSaveError,error);
			}
			else
			{
				ErrorPrint(CardCalibrationSave,error);
				FreeMemoryPrint();
			}
		}
	}
    SendDone(client);
}


//
// Commit the internal structure to the card.
// If OTP is in use, this command consumes additional card memory.
// If Eeprom or FLASH is in use, the memory is overwritten.
//
void ConfigurationPcieSaveCommand(int client)
{
	int np, ip;
	char *name;	
	int error;
	int index;
    int code;
    char buffer[MBUFFER];
	int naction,action;
	int naddress,address=0;
	int nvalue,value=0;
	int it;
    int size;

	naction= -1;
	action= -1;
	naddress= -1;
	address= -1;
	nvalue= -1;
	value= -1;
    size = ValueSizeMaximum;
	//
	// check if card is loaded
	//
	if(!CardValidReset())
	{
		ErrorPrint(CardNoLoadOrReset);
	}
	else
	{
		//
		// install default parameter values
		//
		error=0;
		action= PcieCommit;
		naddress= -1;
		nvalue= -1;
		//
		// parse arguments and do it
		//
		np=CommandParameterMany();
		for(ip=0; ip<np; ip++)
		{
			name=CommandParameterName(ip);
			index=ParameterSelectIndex(name,PcieSaveParameter,sizeof(PcieSaveParameter)/sizeof(PcieSaveParameter[0]));
			if(index>=0)
			{
				code=PcieSaveParameter[index].code;
				switch(code) 
				{
					case PcieAction:
						naction=ParseIntegerList(ip,name,&action,&PcieSaveParameter[index]);
						if(naction<=0)
						{
							error++;
						}
						break;
					case PcieAddress:
						naddress=ParseHexList(ip,name,(unsigned int *)&address,&PcieSaveParameter[index]);
						if(naddress<=0)
						{
							error++;
						}
						break;
					case PcieValue:
						nvalue=ParseHexList(ip,name,(unsigned int *)&value,&PcieSaveParameter[index]);
						if(nvalue<=0)
						{
							error++;
						}
						break;
                    case PcieSize:
                        nvalue=ParseIntegerList(ip,name,(unsigned int *)&size,&PcieSaveParameter[index]);
						if(nvalue<=0)
						{
							error++;
						}
                        break;
                    default:
                        break;
				}
			}
		}

		if(error<=0)
		{
			switch(action)
			{
				case PcieCommit:
                    if(naddress<=0 && nvalue<=0)
					{
						error=DeviceInitializationCommit();
						if(error>=0)
						{
							ErrorPrint(CardPcieSave,error);
 							FreeMemoryPrint();
						}
						else
						{
							ErrorPrint(CardPcieSaveError,error);
						}
					}
					else
					{
						UserPrint("address and value not allowed\n");
					}
					break;
				case PcieRead:
                    if(naddress<=0 && nvalue<=0)
					{
						DeviceInitializationRestore();
					}
					else
					{
						UserPrint("address and value not allowed\n");
					}
					break;
				case PcieList:
                    if(naddress<=0 && nvalue<=0)
					{
						ErrorPrint(NartDataHeader,"|boot|register|value|");
						for(it=0; it<100; it++)
						{
							unsigned int raddress;
							unsigned int rvalue;
							if(DeviceInitializationGetByIndex(it, &raddress, &rvalue)==0)
							{
								SformatOutput(buffer,MBUFFER-1,"|boot|0x%x|0x%x|",raddress,rvalue);
								buffer[MBUFFER-1]=0;
								ErrorPrint(NartData,buffer);
							}
							else
							{
								break;
							}
						}
					}
					else
					{
						UserPrint("address and value not allowed\n");
					}
					break;
				case PcieAdd:
                    if(naddress==1 && nvalue==1)
					{
                        // check if address and size matching
                        if ((size == 3) || (size == 4 && (address & 3) != 0) || (size == 2 && (address & 1) != 0))
                        {
                            UserPrint("address and/or size are invalid\n");
                        }
                        else 
                        {
						    DeviceInitializationSet(address, value, size);
                        }
					}
					else
					{
						UserPrint("address and value required\n");
					}
					break;
				case PcieDelete:
                    if(naddress==1)
					{
                        // check if address and size agreed
                        if ((size == 4 && (address & 3) != 0) || (size == 2 && (address & 1) != 0))
                        {
                            UserPrint("address and/or size are invalid\n");
                        }
                        else
                        {
						    DeviceInitializationRemove(address, size);
                        }
					}
					else
					{
						UserPrint("address required.\n");
					}
					break;
                default:
                    break;
			}
		}
	}

    SendDone(client);
}


//
// Restore the internal structure to the state on the card.
// That is, forget all changes that we have made since the last ConfigureSave() command.
//
void ConfigurationRestoreCommand(int client)
{
	char buffer[MBUFFER];
 	int memory,nmemory;
	int address,naddress;
	int preference,npreference;
	int ip, np;
	int error;
	char *name;
	int code;
	int index;
	//
	// install default parameter values
	//
	error=0;
	npreference= -1;
	preference=TemplatePreferenceDefault;
	nmemory= -1;
	memory=TemplateMemoryDefaultRead;
	naddress= -1;
	address=TemplateSizeDefaultRead;
	//
	// parse arguments and do it
	//
	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		index=ParameterSelectIndex(name,ConfigurationRestoreParameter,sizeof(ConfigurationRestoreParameter)/sizeof(ConfigurationRestoreParameter[0]));
		if(index>=0)
		{
			code=ConfigurationRestoreParameter[index].code;
			switch(code) 
			{
				case TemplatePreference:
					npreference=ParseIntegerList(ip,name,&preference,&ConfigurationRestoreParameter[index]);
					if(npreference<=0)
					{
						error++;
					}
					break;
				case TemplateMemory:
					nmemory=ParseIntegerList(ip,name,&memory,&ConfigurationRestoreParameter[index]);
					if(nmemory<=0)
					{
						error++;
					}
					break;
				case TemplateSize:
					naddress=ParseIntegerList(ip,name,&address,&ConfigurationRestoreParameter[index]);
					if(naddress<=0)
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
		//
		// if there's no card loaded, return error
		//
		if(CardCheckAndLoad(client)!=0)
		{
			ErrorPrint(CardNoneLoaded);
			error= -1;
		}
		else
		{
			if(npreference==1)
			{
				DeviceEepromTemplatePreference(preference);
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
			error=DeviceConfigurationRestore();
			if(error!=0)
			{
				SformatOutput(buffer,MBUFFER-1,"Can't restore startup configuration data");
				buffer[MBUFFER-1]=0;
				SendError(client,buffer);
			}
		}
	}
    
    SendDone(client); 
}

void ConfigurationSetParameterSplice(struct _ParameterList *list)
{
    DeviceSetParameterSplice(list);;
}


void ConfigurationGetParameterSplice(struct _ParameterList *list)
{
    DeviceGetParameterSplice(list);;
}


//
// parse and then set a configuration parameter in the internal structure
//
void ConfigurationSetCommand(int client)
{
	//
	// check if card is loaded
	//
	if(!CardValid())
	{
		ErrorPrint(CardNoneLoaded);
	}
	else
	{
		DeviceSetCommand(client);
	}
	SendDone(client);
}


//
// parse and then get a configuration parameter in the internal structure
//
void ConfigurationGetCommand(int client)
{
	DeviceGetCommand(client);
	SendDone(client);
}	

