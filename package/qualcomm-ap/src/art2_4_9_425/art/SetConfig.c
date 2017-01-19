#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "smatch.h"
#include "CommandParse.h"
#include "ParameterSelect.h"
#include "ConfigurationStatus.h"
#include "NewArt.h"
#include "Device.h"
#include "SetConfig.h"
#include "UserPrint.h"

#define MBUFFER             1024

static SETCONFIG_HASH SetConfigHashTable[MAX_NUM_SET_CONFIG] = {0};
static int NumSetConfig = 0;

extern void ConfigCmdStatus(int status, int *error, char *cmd, char *name, char *tValue, int client);

void SetConfigParameterSplice(struct _ParameterList *list)
{
    DeviceSetConfigParameterSplice(list);
}

SETCONFIG_HASH *SetConfigFind (char *Item)
{
	int i;

	for (i = 0; i < NumSetConfig; ++i)
	{
		if (strcmp(Item, SetConfigHashTable[i].pKey) == 0)
		{
			return &SetConfigHashTable[i];
		}
	}
	return NULL;
}

SETCONFIG_HASH *SetConfigAlloc (int ItemLen, int ValueLen)
{
	if ((SetConfigHashTable[NumSetConfig].pKey = malloc(ItemLen+1)) == NULL)
	{
		return NULL;
	}
	if ((SetConfigHashTable[NumSetConfig].pVal = malloc(ValueLen+1)) == NULL)
	{
		free(SetConfigHashTable[NumSetConfig].pKey);
		return NULL;
	}
	NumSetConfig++;
	return (&SetConfigHashTable[NumSetConfig-1]);
}

int SetConfigSet (char *Item, int ItemLen, char *Value, int ValueLen)
{
	char *pTemp;
	char *pParam;
	int paramLen;
	SETCONFIG_HASH *ptr;

	// Remove any space in Item
	pTemp = Item;
	while (*pTemp == ' ' || *pTemp == '\t') {pTemp++;}
	pParam = pTemp;
	pTemp = &Item[ItemLen-1];
	while (*pTemp == ' ' || *pTemp == '\t') {pTemp--;}
	*++pTemp = '\0';
	paramLen = pTemp - pParam;

	if ((ptr = SetConfigFind(pParam)) == NULL)
	{
		if ((ptr = SetConfigAlloc (paramLen, ValueLen)) == NULL)
		{
			UserPrint("Error - SetConfigSet coulld not allocate a setConfig entry\n");
			return 0;
		}
	}
	strcpy (ptr->pKey, pParam);
	strcpy (ptr->pVal, Value);
	return 1;
}

void SetConfigCommand(int client)
{
    int status;
    int error;
    char *name;
    char *tValue;
	int np, ip;
	//char delimiter;	
	char buffer[MBUFFER];
	int lc, nc;
    int nvalue, iv;

    strcpy(buffer, "");

    //
	// prepare beginning of error message in case we need to use it
	//
	lc=0;
	error=0;
    status = VALUE_OK;
	//
	//parse arguments and do it
	//
	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		if (strlen(name)==0) continue;	// sometime there are tab or space at the end count as np with name as "", skip it

        //put command back into buffer
	    nvalue=CommandParameterValueMany(ip);
	    //put each value of the command arg back into buffer
	    for(iv=0; iv<nvalue; iv++) 
        {
		    tValue=CommandParameterValue(ip,iv);
			if (iv == 0)
			{
				nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"%s",tValue);
			}
			else
			{
				nc=SformatOutput(&buffer[lc],MBUFFER-lc-1," %s",tValue);
			}
		    if(nc>0)
		    {
			    lc+=nc;
		    }
	    }
	    if(nc>0)
	    {
		    lc+=nc;
	    }
        if (NumSetConfig == MAX_NUM_SET_CONFIG)
        {
            status = ERR_MAX_REACHED;
            error++;
        }
		if (error == 0)
		{
			if (SetConfigSet (name, strlen(name), buffer, strlen(buffer)) == 0)
			{
				status = ERR_RETURN;
				error++;
			}
        }
	}
    ConfigCmdStatus(status, &error, "setconfig", name, tValue, client);
	SendDone(client);
}

void SetConfigProcess()
{
    int i;

    for (i = 0; i < NumSetConfig; i++)
    {
        if (SetConfigHashTable[i].pKey)
        {
    	    DeviceSetConfigCommand(&SetConfigHashTable[i]);
        }
    }
}

