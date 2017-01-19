#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "smatch.h"
#include "CommandParse.h"
#include "ParameterSelect.h"
#include "ParseError.h"
#include "NewArt.h"
#include "Device.h"
#include "SleepMode.h"
#include "UserPrint.h"
#include "ErrorPrint.h"

enum
{
	SleepMode = 0,
};

static int SleepModeMin = SLEEP_MODE_SLEEP;
static int SleepModeMax = SLEEP_MODE_DEEP_SLEEP;
static int SleepModeDefault = SLEEP_MODE_SLEEP;

//
// Sleep mode parameters
//

static struct _ParameterList SleepModeParameter[]=
{
	{SleepMode,{"mode",0,0},"the sleep mode (0:sleep; 1:wakeup; 2:deep sleep)",'u',0,1,1,1,&SleepModeMin,&SleepModeMax,&SleepModeDefault,0,0},
};

static int CurrentSleepMode = SLEEP_MODE_WAKEUP;

void SleepModeWakeupSet()
{
	// This is call when tx or rx is issued since the DUT will be waken up if tx/rx is received during sleep mode
	CurrentSleepMode = SLEEP_MODE_WAKEUP;
}

void SleepModeParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(SleepModeParameter)/sizeof(SleepModeParameter[0]);
    list->special=SleepModeParameter;
}

void SleepModeCommand(int client)
{
    int error;
    char *name;
	int np, ip, ngot;
    int value, selection;

    //
	// prepare beginning of error message in case we need to use it
	//
	error=0;
	//
	//parse arguments and do it
	//
	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		if (strlen(name)==0) continue;	// sometime there are tab or space at the end count as np with name as "", skip it

		selection=ParameterSelect(name,SleepModeParameter,sizeof(SleepModeParameter)/sizeof(SleepModeParameter[0]));
		switch(selection)
		{
			case SleepMode:
				ngot=SformatInput(CommandParameterValue(ip,0)," %d ",&value);
				if(ngot != 1 || (value < SleepModeMin || value > SleepModeMax))
				{
					ErrorPrint(ParseBadValue, CommandParameterValue(value, 0),"mode");
					error++;
				}
				break;
			default:
				break;
		}
	}
	if (error == 0)
	{
		if (CurrentSleepMode == value)
		{
			UserPrint("The device is currently in %s mode.\n", (value == SLEEP_MODE_SLEEP ? "sleep" :
				(value == SLEEP_MODE_WAKEUP ? "wakeup" : "deep sleep")));
		}
		else
		{
			SendOn(client);
			DeviceSleepMode(value);
			SendOff(client);
			CurrentSleepMode = value;
		}
	}
	SendDone(client);
}

