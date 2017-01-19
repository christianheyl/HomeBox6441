#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "wlantype.h"
#include "smatch.h"
#include "UserPrint.h"
#include "TimeMillisecond.h"
#include "CommandParse.h"
#include "NewArt.h"
#include "MyDelay.h"
#include "ParameterSelect.h"
#include "Card.h"
//#include "Field.h"


#include "Device.h"

#include "ParameterParse.h"
//#include "ParameterParseNart.h"
//#include "Link.h"
//#include "Calibrate.h"
#include "configCmd.h"
#include "ConfigurationStatus.h"
#include "rate_constants.h"
#include "vrate_constants.h"
#include "LinkList.h"

#define MBUFFER 1024
#define MRATE 10
#define MLOOP 200

void ConfigCmdHelpPrint()
{

}

void ConfigCmdStatus(int status, int *error, char *cmd, char *name, char *tValue, int client)
{
	char buffer[MBUFFER];
    if(status==VALUE_OK)
    {
		if (strlen(cmd)>0 && strlen(name)>0) 
			// ex: "|get|ssid|1234|"
			SformatOutput(buffer,MBUFFER-1,"|%s|%s|%s|",cmd, name, tValue);
		else if (strlen(name)>0)
			SformatOutput(buffer,MBUFFER-1,"|%s|%s|", name, tValue);
		else
			SformatOutput(buffer,MBUFFER-1,"|%s|", tValue);
		buffer[MBUFFER-1]=0;
        SendIt(client,buffer);
	} else if (status==ERR_VALUE_BAD) {
		if (strlen(tValue)>0)
			SformatOutput(buffer,MBUFFER-1,"%s",tValue);
//			SformatOutput(buffer,MBUFFER-1,"bad value %s for parameter %s",tValue,name);
		else 
			SformatOutput(buffer,MBUFFER-1,"bad value for %s %s",name,tValue);
		buffer[MBUFFER-1]=0;
		SendError(client,buffer);
	    (*error)++;
	} else {
        SformatOutput(buffer,MBUFFER-1,"error %s %s",cmd, name);
        buffer[MBUFFER-1]=0;
        SendError(client,buffer);
	    (*error)++;
    }
}

//
// link parameters
//
enum ConfigCmd 
{
    ConfigCmdHelp=0,
	ConfigCmdFreq,
	ConfigCmdDataRate,
};

struct _ParameterList cpl[]=
{
    {ConfigCmdHelp,"help","?",0}, 
	{ConfigCmdFreq,"frequency","f",0}, 
	//{ConfigCmdDataRate,"r","rate",0}, 
	LINK_RATE(MRATE),
};

static int parseConfigCmd(int *nFreq, unsigned int *Rate, unsigned int *nRate, double *tp, int client, char *cmd)
{
	int np, ip;//, i;
	char *name, tValue[MBUFFER];
	int code;
	int ngot=0, error=0;
	int status = VALUE_OK;
	//int foundRatesByName;
	unsigned int nrate;//, rate[MRATE];
	int rlegacy,rht20,rht40,nvalue,rerror,extra, index;

	//for (i=0; i<MRATE; i++) {
	//	rate[i]=0;
	//	nRate[i]=0;
	//}
	nrate = 0;

	strcpy(tValue, "");
	np=CommandParameterMany();
	if(np<=0 || np>2)
		ConfigCmdHelpPrint();
	if (np==1) {
		ConfigCmdStatus(ERR_VALUE_BAD, &error, cmd, "", "You need to specify freq and rate", client);
		ConfigCmdHelpPrint();
	}
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		index=ParameterSelectIndex(name,cpl,sizeof(cpl)/sizeof(cpl[0]));
		if (index < 0)
		{
			UserPrint("ERROR: invalid parameter %s\n",name);
			continue;
		}
		code=cpl[index].code;

		//code=ParameterSelect(name,cpl,sizeof(cpl)/sizeof(struct _ParameterList));
		switch(code)
		{
			case ConfigCmdHelp:
				ConfigCmdHelpPrint();
				break;
			case ConfigCmdFreq:
				strcpy(tValue, CommandParameterValue(ip,0));
				ngot=SformatInput(tValue," %d ",nFreq);
				if(ngot!=1)
					ConfigCmdStatus(ERR_VALUE_BAD, &error, cmd, name, tValue, client);
				break;
			case LinkParameterRate:
				/*foundRatesByName=ParseStringAndSetRates(ip,name,MRATE,(int*)rate);//first preference to the rates by name
				if(!foundRatesByName)
				{
					nrate=ParseHex(ip,name,MRATE,rate);
					if(nrate<1)	// 6 items in rate array: legacy,ht20,ht40,vht20,vht40,vht80
						rate[0]=1;	
				}
				for (i=0; i<MRATE; i++) {
					nRate[i]=rate[i];
				}*/

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
						nrate=RateCount(rlegacy,rht20,rht40,Rate);
						UserPrint("Note: Please use rate names as possible. Rate masks will be obsolete in the future.\n");
					}
				}
				if(rerror!=0)
				{
					nrate=ParseIntegerList(ip,name,Rate,&cpl[index]);
					if(nrate<=0)
					{
						error++;
					}
					else
					{
						nrate=RateExpand(Rate,nrate);
						nrate=vRateExpand(Rate,nrate);
					}
				}
				*nRate = nrate;
				break;
			default:
				UserPrint("ERROR: invalid parameter %s\n",name);
		}	// switch(code)
	}	// for(it=0; it<nt; it++)

	if (error>0)
		return ERR_VALUE_BAD;
	return VALUE_OK;
}


void ConfigGetTPCommand(int client)
{
	int status = VALUE_OK, error=0;
	int freq, ip;
	double tp=0;
	//int Rate[vNumRateCodes], RateMany, vRate[vNumRateCodes], vRateMany,ir;
	int RateMany, ir;
	unsigned int nRate[MRATE];
	//char sRate[20], buffer[MBUFFER];
	char buffer[MBUFFER];
	int nc=0, lc=0;

	nRate[0]=1<<7;		// 54Mbps
	for (ip=1; ip<MRATE; ip++) {
		nRate[ip]=0;
	}
	freq = 2412;

	//
	// if there's no card loaded, return error
	//
    if(CardCheckAndLoad(client)!=0)
    {
		SendError(client,"no card loaded");
    }
	else
	{
		if (parseConfigCmd(&freq, nRate, &RateMany, &tp, client, "getTP")!=VALUE_OK) {
			ConfigCmdHelpPrint();
			return;
		}
		/*if (nRate[0]>0 || nRate[1]>0 || nRate[2]>0)
			RateMany = RateCount(nRate[0], nRate[1], nRate[2], Rate);
		else
			vRateMany = vRateCount(nRate[3], nRate[4], nRate[5], vRate);
		for (ir=0; ir<RateMany; ir++) {
			strcpy(sRate,rateStrAll[Rate[ir]]);
			nc = SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%s|", sRate);
			if(nc>0) lc+=nc;
		}			
		for (ir=0; ir<vRateMany; ir++) {
			strcpy(sRate,vRateStrAll[vRate[ir]-numRateCodes]);
			nc = SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%s|", sRate);
			if(nc>0) lc+=nc;
		}*/
		for (ir=0; ir<RateMany; ir++) 
		{
			if (IS_vRate(nRate[ir]))
			{
				nc = SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%s|",vRateStrAll[nRate[ir]-numRateCodes]);
			}
			else
			{
				nc = SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%s|",rateStrAll[nRate[ir]]);
			}
			if(nc>0) lc+=nc;
		}
		ConfigCmdStatus(VALUE_OK, &error, "", "", buffer, client);
		lc=0;
		/*for (ir=0; ir<RateMany; ir++) {
			DeviceTargetPowerGet(freq,Rate[ir],&tp);
			nc = SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%.1f|", tp);
			if(nc>0) lc+=nc;
		}
		for (ir=0; ir<vRateMany; ir++) {
			DeviceTargetPowerGet(freq,vRate[ir],&tp);
			nc = SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%.1f|", tp);
			if(nc>0) lc+=nc;
		}*/
		for (ir=0; ir<RateMany; ir++) 
		{
			DeviceTargetPowerGet(freq, nRate[ir], &tp);
			nc = SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%.1f|", tp);
			if(nc>0) lc+=nc;
		}
		ConfigCmdStatus(VALUE_OK, &error, "", "", buffer, client);

		SendDone(client);
	}

}

void ConfigSetTPCommand(int client)
{
}
