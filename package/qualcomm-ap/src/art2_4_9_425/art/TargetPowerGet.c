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

#include "rate_constants.h"
#include "vrate_constants.h"

#include "TargetPowerGet.h"

#include "ErrorPrint.h"
#include "ParseError.h"
#include "CardError.h"
#include "NartError.h"



#include "LinkList.h"

#define MBUFFER 1024

#define MLOOP 100
#define MRATE 200

//
// exported to test.c to complete menu structure
//
static struct _ParameterList TargetPowerParameter[]=
{	
	LINK_FREQUENCY(MLOOP),
	LINK_RATE(MRATE),
};

void TargetPowerParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(TargetPowerParameter)/sizeof(TargetPowerParameter[0]);
    list->special=TargetPowerParameter;
}


//
// interpolates into the stored data structure and returns the value that will be used
//
void TargetPowerGetCommand(int client)
{
	int np, ip;
	char *name;	
	char buffer[MBUFFER];
	int error;
    int parseStatus=0, nc=0, lc=0;
	int index;
    int code;
	int it, nfrequency, frequency[MLOOP];
	int ir, nrate, rate[MRATE], nvrate, vrate[MRATE];
	double tp;
	int rlegacy,rht20,rht40,rerror,ngot,extra;
	int nvalue;
	//
	// install default parameter values
	//
	error=0;
	//
	// a bunch of frequencies
	//
	nfrequency=1;
	frequency[0]= 2412;
    //
	// all rates
	//
	nrate=1;
	rate[0]=RATE_INDEX_6;
	//
	// parse arguments and do it
	//
	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		index=ParameterSelectIndex(name,TargetPowerParameter,sizeof(TargetPowerParameter)/sizeof(TargetPowerParameter[0]));
		if(index>=0)
		{
			code=TargetPowerParameter[index].code;
			switch(code) 
			{
				case LinkParameterFrequency:
					nfrequency=ParseIntegerList(ip,name,frequency,&TargetPowerParameter[index]);
					if(nfrequency<=0)
					{
						error++;
					}
					break;
				case LinkParameterRate:
					nvalue=CommandParameterValueMany(ip);
					//
					// check if it might be the old mask codes
					//
					rerror=1;
					if(nvalue==3)
					{
						rerror=0;
						rlegacy=0;
						rht20=0;
						rht40=0;
						ngot=SformatInput(CommandParameterValue(ip,0)," %x %1c",&rlegacy,&extra);
						if(ngot!=1)
						{
							rerror++;
						}
						if(nvalue>=2)
						{
							ngot=SformatInput(CommandParameterValue(ip,1)," %x %1c",&rht20,&extra);
							if(ngot!=1)
							{
								rerror++;
							}
						}
						if(nvalue>=3)
						{
							ngot=SformatInput(CommandParameterValue(ip,2)," %x %1c",&rht40,&extra);
							if(ngot!=1)
							{
								rerror++;
							}
						}
						if(rerror<=0)
						{
							nrate=RateCount(rlegacy,rht20,rht40,rate);
							UserPrint("Note: Please use rate names as possible. Rate masks will be obsolete in the future.\n");
						}
					}
					if(rerror!=0)
					{
						nrate=ParseIntegerList(ip,name,rate,&TargetPowerParameter[index]);
						if(nrate<=0)
						{
							error++;
						}
						else
						{
							nrate=RateExpand(rate,nrate);
							nrate=vRateExpand(rate,nrate);
						}
					}
					break;
				default:
					ErrorPrint(ParseBadParameter,name);
					error++;
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
		if(!CardValid())
		{
			ErrorPrint(CardNoneLoaded);
		}
		else
		{
			ErrorPrint(NartDataHeader,"|tp|frequency|rate||target|");

			for(it=0; it<nfrequency; it++)
			{
				for(ir=0; ir<nrate; ir++)
				{
					DeviceTargetPowerGet(frequency[it],rate[ir],&tp);
					if (IS_vRate(rate[ir]))
						SformatOutput(buffer,MBUFFER-1,"|tp|%d|%s||%.1lf|",frequency[it],vRateStrAll[rate[ir]-numRateCodes],tp);
					else
						SformatOutput(buffer,MBUFFER-1,"|tp|%d|%s||%.1lf|",frequency[it],rateStrAll[rate[ir]],tp);
					buffer[MBUFFER-1]=0;
					ErrorPrint(NartData,buffer);
				}
			}
		}
	}
	
	SendDone(client);
}

//
// exported to test.c to complete menu structure
//
static struct _ParameterList NoiseFloorGetParameter[]=
{	
	LINK_FREQUENCY(MLOOP),
	LINK_CHAIN(1),
};

void NoiseFloorGetParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(NoiseFloorGetParameter)/sizeof(NoiseFloorGetParameter[0]);
    list->special=NoiseFloorGetParameter;
}


//
// interpolates into the stored data structure and returns the value that will be used
//
void NoiseFloorGetCommand(int client)
{
	int np, ip;
	char *name;	
	char buffer[MBUFFER];
	int error;
	int index;
    int code;
	int it, nfrequency, frequency[MLOOP];
	int nchain, chain;
	int nf, nfp;
	int ich;
	//
	// prepare beginning of error message in case we need to use it
	//
	error=0;
	nfrequency=1;
	frequency[0]= 2412;
	nchain=1;
	chain= 0x7;
	//
	// parse arguments and do it
	//
	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		index=ParameterSelectIndex(name,NoiseFloorGetParameter,sizeof(NoiseFloorGetParameter)/sizeof(NoiseFloorGetParameter[0]));
		if(index>=0)
		{
			code=NoiseFloorGetParameter[index].code;
			switch(code) 
			{
				case LinkParameterFrequency:
					nfrequency=ParseIntegerList(ip,name,frequency,&NoiseFloorGetParameter[index]);
					if(nfrequency<=0)
					{
						error++;
					}
					break;
				case LinkParameterChain:
					nchain=ParseHexList(ip,name,&chain,&NoiseFloorGetParameter[index]);		
					if(nchain<=0)
					{
						error++;
					}
					break;
				default:
					ErrorPrint(ParseBadParameter,name);
					error++;
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
		if(!CardValid())
		{
			ErrorPrint(CardNoneLoaded);
		}
		else
		{
			ErrorPrint(NartDataHeader,"|nfg|frequency|chain||nf|nfp|");

			for(it=0; it<nfrequency; it++)
			{
				for(ich=0; ich<3; ich++)
				{
					if((1<<ich)&chain)
					{
						nf=DeviceNoiseFloorGet(frequency[it], ich);
						nfp=DeviceNoiseFloorPowerGet(frequency[it], ich);
						SformatOutput(buffer,MBUFFER-1,"|nfg|%d|%x||%d|%d|",frequency[it],1<<ich,nf,nfp);
						buffer[MBUFFER-1]=0;
						ErrorPrint(NartData,buffer);
					}
				}
			}
		}
	}
	
	SendDone(client);
}



