


#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "mlibif.h"

#include "ParameterSelect.h"
#include "NartRegister.h"

#include "smatch.h"
#include "CommandParse.h"
#include "Card.h"
#include "NewArt.h"
#include "Field.h"
#include "Sticky.h"
#include "Device.h"
#include "GainTable.h"

#include "ErrorPrint.h"
#include "NartError.h"
#include "CardError.h"

#include "CalibrationLoad.h"

#define MBUFFER 1024


#define MVALUE 100

#define EX_STRING1 "BB_tx_gain_tab_"
// Exclude any unwanted entries by looking at hardcoded register name
static int ExcludeTxGainTable(char *tmpbuf)
{
	if (strncmp((tmpbuf+1),EX_STRING1,strlen(EX_STRING1)) == 0)
		if ((*(tmpbuf+strlen(EX_STRING1)+1)>='0') && (*(tmpbuf+strlen(EX_STRING1)+1)<='9'))
			return 1;
	return 0;
}

int GainTbl_totalGain[MVALUE];
int maxGainEntry=MVALUE;
int badValue=-1000;

void GainTableReadCommand(int client)
{
	int ngot;
	int np;
	int ip, it, iv;
	int address;
	int nvalue;
	int value[MVALUE];
	char *name;
	char ebuffer[MBUFFER],buffer[MBUFFER];
	int error;
	int done;
	int lc, nc;
	unsigned int reg;
	//
	// check if card is loaded
	//
	if(!CardValid())
	{
		ErrorPrint(CardNoneLoaded);
		SendDone(client);
		return;
	}
	//
	// prepare beginning of error message in case we need to use it
	//
	lc=0;
	error=0;
	done=0;
	//
	//parse arguments and do it
	//
	ErrorPrint(NartDataHeader,"|tgr|index|32regValue|total_gain|txbb1dbgain|txbb6dbgain|txmxrgain|padrvgnA|padrvgnB|padrvgnC|padrvgnD|");
	address= -1;
	np=CommandParameterMany();
	if(np>0)
	{
		for(ip=0; ip<np; ip++)
		{
			name=CommandParameterName(ip);
			if(Smatch(name,"a")||Smatch(name,"address"))
			{
				for(it=0; it<CommandParameterValueMany(ip); it++)
				{
					ngot=SformatInput(CommandParameterValue(ip,it)," %d ",&address);
					if(ngot==1)
					{
						nvalue=DeviceTransmitGainRead(address,&reg,value,MVALUE);
						lc=SformatOutput(buffer,MBUFFER-1,"|tgr|%2d|%08x|",address,reg);
						for(iv=0;iv<nvalue; iv++)
						{
							nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"%d|",value[iv]);
							if(nc>0)
							{
								lc+=nc;
							}
						}
						ErrorPrint(NartData,buffer);
					}
					else
					{
						address= -1;
						nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"bad address %s, ",CommandParameterValue(ip,it));
						if(nc>0)
						{
							lc+=nc;
						}
						error++;
					}
				}
			}
		}
	}
	else
	{
		//
		// do the whole thing
		//
				for(it=0; it<50; it++)
				{
						nvalue=DeviceTransmitGainRead(it,&reg,value,MVALUE);
						if(nvalue>0)
						{
							lc=SformatOutput(buffer,MBUFFER-1,"|tgr|%2d|%08x|",it,reg);
							for(iv=0;iv<nvalue; iv++)
							{
								nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"%d|",value[iv]);
								if(nc>0)
								{
									lc+=nc;
								}
							}
							ErrorPrint(NartData,buffer);
						}
						else
						{
							break;
						}
				}


				{
					unsigned int row;
					unsigned int col;
					unsigned int *addrPtr;

					unsigned int address;
					int nItem, i, lc;
					unsigned int val, low, high, j;
					char rName[50], fName[50];
					static char tmpbuf[MBUFFER], tmpbuf2[MBUFFER];

					DeviceTxGainTableRead_AddressGainTable(&addrPtr, &row, &col);

					for (j =0; j< row; j++)
					{
						address = addrPtr[j*col];
						if ((nItem=DeviceTxGainTableRead_AddressHeader(address,tmpbuf,tmpbuf2,MBUFFER))&&!ExcludeTxGainTable(tmpbuf))
						{
							ErrorPrint(NartDataHeader,tmpbuf);
							ErrorPrint(NartDataHeader,tmpbuf2);
							lc= strlen(strtok(&tmpbuf[1],"|"))+1;
							lc+=SformatOutput(&tmpbuf[lc],MBUFFER-lc-1,"|");
							for (i=-1; i<nItem; i++)
							{
								if (DeviceTxGainTableRead_AddressValue(address, i, rName, fName, &val, &low, &high)==-1)
									break;
								nc=SformatOutput(&tmpbuf[lc],MBUFFER-lc-1,"0x%x|",val);
								if(nc>0)
								{
									lc+=nc;
								}
							}
							ErrorPrint(NartData,tmpbuf);
						}
					}
				}
}
	//
	// send DONE or ERROR
	//
   if(error>0)
	{
	    SendError(client,ebuffer);
	}
	SendDone(client);

}

#define MVALUE 100


void GainTableWriteCommand(int client)
{
	int ngot;
	int np;
	int ip, it;
	int address;
	int nvalue;
	unsigned int value[MVALUE];
	char *name;
	char ebuffer[MBUFFER];
	int error;
	int done;
	int lc, nc;
	//
	// check if card is loaded
	//
	if(!CardValid())
	{
		ErrorPrint(CardNoneLoaded);
		SendDone(client);
		return;
	}
	//
	// prepare beginning of error message in case we need to use it
	//
	lc=0;
	error=0;
	done=0;
	//
	// parse arguments and do it
	//
	address= -1;
	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		if(Smatch(name,"a")||Smatch(name,"address"))
		{
			for(it=0; it<CommandParameterValueMany(ip); it++)
			{
				ngot=SformatInput(CommandParameterValue(ip,it)," %d ",&address);
			    if(ngot!=1)
				{
					address= -1;
		            nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"bad address %s, ",CommandParameterValue(ip,it));
					if(nc>0)
					{
						lc+=nc;
					}
					error++;
				}
			}
		}
		if(Smatch(name,"v")||Smatch(name,"value"))
		{
			nvalue=CommandParameterValueMany(ip);
			for(it=0; it<nvalue; it++)
			{
				ngot=SformatInput(CommandParameterValue(ip,it)," %x ",&value[it]);
			    if(ngot!=1)
				{
		            nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"bad value %s, ",CommandParameterValue(ip,it));
					if(nc>0)
					{
						lc+=nc;
					}
					error++;
					value[it]=0;
				}
			}
			if(error<=0 && address>=0)
			{
		        DeviceTransmitGainWrite(address,(int *)value,nvalue);
			}
		}
	}
	//
	// send DONE or ERROR
	//
	if(address<0 || nvalue<=0)
	{
		nc=SformatOutput(&ebuffer[lc],MBUFFER-lc-1,"no address or no value");
	    if(nc>0)
		{
			lc+=nc;
		}
		error++;
	}
	if(error>0)
	{
		SendError(client,ebuffer);
	}
	SendDone(client);
}

int queryTxGainTable(void)
{
	maxGainEntry=DeviceTransmitINIGainGet(GainTbl_totalGain);
	CalibrationSetIniTxGain(GainTbl_totalGain, maxGainEntry);
	return maxGainEntry;
}

