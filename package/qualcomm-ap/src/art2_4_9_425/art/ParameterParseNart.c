
//  "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/art/ParameterParseNart.c#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/art/ParameterParseNart.c#1 $"


#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "smatch.h"
#include "NewArt.h"

#include "CommandParse.h"
#include "ParameterSelect.h"
#include "ParameterParse.h"
#include "ParameterParseNart.h"
#include "ConfigurationStatus.h"



void Print_RefSetCalTGTPwrDataRate(_PARAM_ITEM_STRUCT *paramS, int client)
{
	char title[MBUFFER], buff[MBUFFER];
	calTGTpwrDataRateTitle(paramS->iMode, title);

	sprintf(buff,"	dataRate: %s\n", title);	
	buff[MBUFFER-1]=0;
    SendIt(client,buff);
}

void Print_RefSet(char *name, _PARAM_ITEM_STRUCT *paramS, int client)
{
	char buff[MBUFFER];
	int lc=0, nc=0;
	int i,j;
	nc=SformatOutput(&buff[lc],MBUFFER-lc-1, "Reference format: ");
	if(nc>0) lc+=nc;
	if (!strcmp(paramS->paramName, "config")) {	// set config=32bitHexValue; address=16bitHexAddress;
		nc=SformatOutput(&buff[lc],MBUFFER-lc-1, "\n	config=32bitHexValue; address=16bitHexAddress;");
		if(nc>0) lc+=nc;	
		buff[MBUFFER-1]=0;
		SendIt(client,buff);
		return;
	}

	// ex format of setting 1 value for all selected items
	if (paramS->numItems<=0 || paramS->numItems>3) {	// error
		nc=SformatOutput(&buff[lc],MBUFFER-lc-1, "\n	%s error", name);	
		buff[MBUFFER-1]=0;
		SendIt(client,buff);
		return;
	} else {
		nc=SformatOutput(&buff[lc],MBUFFER-lc-1, "\n	%s=v.v0", name);	
		if(nc>0) lc+=nc;
		nc=SformatOutput(&buff[lc],MBUFFER-lc-1, ",%s.I,%s.J;", paramS->item[1].itemName, paramS->item[2].itemName);
		if(nc>0) lc+=nc;
	}
	// ex format of setting array values
	for (j=1; j<paramS->numItems; j++) {
		nc=SformatOutput(&buff[lc],MBUFFER-lc-1, "\n	%s=v.(", name);
		if(nc>0) lc+=nc;
		for (i=0; i<paramS->item[j].itemMax-paramS->item[j].itemMin+1; i++) {
			nc=SformatOutput(&buff[lc],MBUFFER-lc-1, "v%d", i);
			if(nc>0) lc+=nc;
			if (i<paramS->item[j].itemMax-paramS->item[j].itemMin) {
				nc=SformatOutput(&buff[lc],MBUFFER-lc-1, ",");
				if(nc>0) lc+=nc;
			} else {
				nc=SformatOutput(&buff[lc],MBUFFER-lc-1, ")");
				if(nc>0) lc+=nc;
			}
		}
		if (j==1) {
			nc=SformatOutput(&buff[lc],MBUFFER-lc-1, ",%s.I;", paramS->item[2].itemName);
			if(nc>0) lc+=nc;
		} else {
			nc=SformatOutput(&buff[lc],MBUFFER-lc-1, ",%s.J;", paramS->item[1].itemName);
			if(nc>0) lc+=nc;
		}
	}
	buff[MBUFFER-1]=0;
	SendIt(client,buff);
}

void Print_RefGet(char *name, _PARAM_ITEM_STRUCT *paramS, int client)
{
	char buff[MBUFFER];
	int lc=0, nc=0;
	nc=SformatOutput(&buff[lc],MBUFFER-lc-1, "Reference format: ");
	if(nc>0) lc+=nc;
	// ex format of setting 1 value for all selected items
	if (paramS->numItems<=0 || paramS->numItems>2) {	// error
		nc=SformatOutput(&buff[lc],MBUFFER-lc-1, "\n	%s error", name);	
		buff[MBUFFER-1]=0;
		SendIt(client,buff);
		return;
	} 
	// no paramter, get all
	if (!strcmp(paramS->paramName, "config")) {	// config, need to provide address a.addr, no get all
		nc=SformatOutput(&buff[lc],MBUFFER-lc-1, "\n	config=a.HexAddress16;");
		if(nc>0) lc+=nc;	
	} else {
		nc=SformatOutput(&buff[lc],MBUFFER-lc-1, "\n	%s;		-- get all;", name);
		if(nc>0) lc+=nc;
		if (paramS->numItems==1) {
			nc=SformatOutput(&buff[lc],MBUFFER-lc-1, "\n	%s=%s.I;", name, paramS->item[0].itemName);
			if(nc>0) lc+=nc;
		} else if (paramS->numItems==2) {
			nc=SformatOutput(&buff[lc],MBUFFER-lc-1, "%s=%s.I;", name, paramS->item[0].itemName);
			if(nc>0) lc+=nc;
			nc=SformatOutput(&buff[lc],MBUFFER-lc-1, "\n	%s=%s.J;", name, paramS->item[1].itemName);
			if(nc>0) lc+=nc;
			nc=SformatOutput(&buff[lc],MBUFFER-lc-1, "\n	%s=%s.I,%s.J;", name, paramS->item[0].itemName, paramS->item[1].itemName);
			if(nc>0) lc+=nc;
		}
	}
	buff[MBUFFER-1]=0;
	SendIt(client,buff);
}


int ParseIntegerWithParamName(int input, char *name, _PARAM_ITEM_STRUCT *params, int client)
// this function will find the FIRST parameter with parameter name defined in params->paramName in cmd's input th parameter's
// if there are identical paramName.value, program will take the first one and ignore the rest in the input the cmd parameters
// numInGroup is the number of the parameters under this parameter name. 
//		ex. set CalPier=f0.(4,5,6,7)		params[0]->name="f0", numInGroup=4, 
// if numInGroup=0, cmd format is cmd param0; param1;
// if numInGroup=1, cmd format is cmd param0=paramValue0; param1=paramValue1;
// if numInGroup>1, cmd format is cmd param0=f.(paramValue00,paramValue01,paramValue02); param1=f0.(paramValue10,paramValue11,paramValue12);
//						numInGroup=3 for input=0, param[0]->paramName="f"; numInGroup=3 for input=1, param[0]->paramName="f0"
{
	char buff[MBUFFER];
	int ip, np;
	int newvalue;
	char str[MBUFFER], *strPara, *tmp;
	int i, j, iParam, ngot, num=0;
	int status=VALUE_OK;

//	params->numInGroup = -1;
	
	np=CommandParameterValueMany(input);
	if (np==0 && params->numItems>0) {
		if (!strcmp(params->item[0].itemName, "v"))	// it's set, need to have values np>0
			return ERR_VALUE_BAD;
		if (!strcmp(params->paramName, "config"))	// config, need to provide address a.addr
			return ERR_VALUE_BAD;
	}
	if(np>params->numItems)
	{
		sprintf(buff, "too many values for %s.\n",name);
		buff[MBUFFER-1]=0;
		SendIt(client,buff);
		return ERR_VALUE_BAD;
	}
	for(ip=0; ip<np; ip++)
	{
		iParam=-1;
		iParam=-1;
	// ex: in this cmd  set_calPier data_2G_chain0=f0.(1,2,3,4,5,6),f1.(4,5,6,7,8,9); data_2G_chain1=f.(1,2,3,4,5,6),f2.(4,5,6,7,8,9)
	// input 1th param is data_2G_chain1=f.(1,2,3,4,5,6),f2.(4,5,6,7,8,9)
	// param is f1.(4,5,6,7,8,9) for input=0, ip=1, parse out interger array will be 4,5,6,7,8,9
		strcpy(str, CommandParameterValue(input,ip));	
		if (str==0) {
			status = ERR_VALUE_BAD;
			break;
		}
		strPara = strstr(str, ".");
		if (strPara==0) {
			sprintf(buff, "No '.' in Parameter Item %s.\n",str);
			buff[MBUFFER-1]=0;
			SendIt(client,buff);
			status = ERR_VALUE_BAD;
			break;
		}
		*strPara = 0;
		for (i=0; i<params->numItems; i++) {
			if (strcmp(str, params->item[i].itemName)==0) {	// paramName could be f, f0, ....f7 for set_calPier
				iParam = i;
				break;
			}
		}
		if (iParam==-1){
			sprintf(buff, "No item name match with %s.\n",str);
			buff[MBUFFER-1]=0;
			SendIt(client,buff);
			status = ERR_VALUE_BAD;
			break;
		}
		strPara++;						// take out "."
		if (strPara==0) {
			status = ERR_VALUE_BAD;
			break;
		}
		if (*strPara=='(')	strPara++;	// take out "(" for a group of value with (a,b,c)
		
		tmp = strstr(strPara, ")");
		if (tmp) 
			*tmp=',';

		ngot = 0;
		num = 0;
		for (i=0; i<MAX_NUM_PARAM_VALUE; i++) {
			if (strPara==0) 
				break;
			if (iParam==0 && params->isHex==1) {
				ngot=SformatInput(strPara," 0x%x ",&newvalue);
				if(ngot<1)
					ngot=SformatInput(strPara," %x ",&newvalue);
			} else // for "v" is always params->item[0], other items are index, need read in as decimal
				ngot=SformatInput(strPara, "%d,", &newvalue);
			if (ngot==1) {	
				params->item[iParam].values[num++] = newvalue;
				strPara = strstr(strPara, ",");
				if (strPara) strPara++;
			} else {
				break;
			} 
		}
		if (iParam==0 && strcmp(params->item[iParam].itemName,"v")==0) {
			// num of values depends the number of selected items, could be 1 or the number of array
			if (num < params->item[iParam].numInGroup) {
				for (j=num; j<params->item[iParam].numInGroup; j++)
					params->item[iParam].values[j]=0;
			}
			params->item[iParam].numInGroup = num;
			params->item[iParam].itemValueIsSet = 1;
		} else {
			if (params->item[iParam].numInGroup != num) {
				sprintf(buff, "You set one value for %s, need %d\n", params->item[iParam].itemName,  params->item[iParam].numInGroup);
				buff[MBUFFER-1]=0;
				SendIt(client,buff);
				status = ERR_VALUE_BAD;
				break;
			} else
				params->item[iParam].itemValueIsSet = 1;
		}
	}

	if (status==VALUE_OK) {
	// got all value, check if the values are all within limit
		for (i=0; i<params->numItems; i++) {
			if (params->item[i].itemValueIsSet) {
				for (j=0; j<params->item[i].numInGroup; j++) {
					if (params->item[i].values[j]<params->item[i].itemMin || params->item[i].values[j]>params->item[i].itemMax) { 
						sprintf(buff, "item %s.%d value %d is out of limit %d~%d\n", params->item[i].itemName, params->item[i].values[j],
							params->item[i].values[j], params->item[i].itemMin, params->item[i].itemMax);						
						buff[MBUFFER-1]=0;
						SendIt(client,buff);
						status= ERR_VALUE_BAD;
					}
				}	
			}
		}
	}
	return status;
}

void paramItemSet(_PARAM_ITEM_STRUCT *param, char *itemName, int min, int max, int numValues)
{
	param->item[param->numItems].itemValueIsSet = 0;
	param->item[param->numItems].itemMin = min;
	param->item[param->numItems].itemMax = max;
	param->item[param->numItems].numInGroup = numValues;
	strcpy(param->item[param->numItems].itemName, itemName);
	param->numItems++;
}

void paramItemReSet(_PARAM_ITEM_STRUCT *param, char *itemName, int min, int max, int numValues)
{
	int found = 0;
	int i;
	for (i=0; i<param->numItems; i++) {
		if (strcmp(param->item[i].itemName, itemName)==0) {
			param->item[i].itemValueIsSet = 0;
			param->item[i].itemMin = min;
			param->item[i].itemMax = max;
			param->item[i].numInGroup = numValues;
			found = 1;
			break;
		}
	}
	if (!found)
		paramItemSet(param, itemName, min, max, numValues);
}

int paramItemIsSet(_PARAM_ITEM_STRUCT *param, char *itemName, int *itemNum)
// check if itemName is in param and itemValueIsSet is set
// return 1 if itemName is in param and itemValueIsSet is set, itemNum is the itemNum in param for itemName
// return 0 if itemName is in param and itemValueIsSet is not set, itemNum is the itemNum in param for itemName
// return -1 if itemName is not in param
{
	int i;
	for (i=0; i<param->numItems; i++) {
		if (strcmp(param->item[i].itemName, itemName)==0) {
			*itemNum = i;
			return param->item[i].itemValueIsSet;
		}
	}
	return -1;
}

void calTGTpwrDataRateTitle(int iMode, char *title)
{
	if (iMode==legacy_CCK) 
		strcpy(title, "r1L_5L,r5S,r11L,r11S:");
	else if (iMode==HT20 || iMode==HT40) 
		strcpy(title, "MCS0_8_16,MCS1_3_9_11_17_19,MCS4,MCS5,MCS6,MCS7,MCS12,MCS13,MCS14,MCS15,MCS20,MCS21,MCS22,MCS23:");
	else
		strcpy(title, "r6_24,r36,r48,r54:");

}

void calTGTpwrDataRateSelTitle(int iMode, int iSel, char *title)
{
	if (iMode==legacy_CCK)
		sprintf(title, "\nDataRate:%s", cckDataRate_str[iSel]);
	else if (iMode==HT20 || iMode==HT40) 
		sprintf(title, "\nDataRate:%s", htDataRate_str[iSel]);
	else
		sprintf(title, "\nDataRate:%s", legacyDataRate_str[iSel]);
}


int getCmdSetValue(char *name, int ip, int num, int *value, char *tValue, int isHex)
{
	int i;
	int nc=0, lc=0;
	int status=VALUE_OK, parseStatus;
	if (num<=0) {
		nc=SformatOutput(&tValue[lc],MBUFFER-lc-1,"Need have at least one value, you have %d\n", num);
		if(nc>0) lc+=nc;
		status = ERR_VALUE_BAD;
	}
	if (status == VALUE_OK) {
		if (isHex==1)
			parseStatus = ParseHex(ip, name, num, (unsigned int *)value);
		else
			parseStatus=ParseInteger(ip, name, num, value);
//		if(parseStatus<=num && parseStatus>0) {
		if(parseStatus>0) {
			if (isHex==1) 
				nc=SformatOutput(&tValue[lc],MBUFFER-lc-1,"0x%x", *value);
			else 
				nc=SformatOutput(&tValue[lc],MBUFFER-lc-1,"%s", CommandParameterValue(ip,0));
			if(nc>0) lc+=nc;
			status=VALUE_OK;
			if (parseStatus<num) {
				for (i=parseStatus; i<num; i++)
					value[i]=0;
			}
		} else {
			if (isHex==1)
				nc=SformatOutput(&tValue[lc],MBUFFER-lc-1,"Need %d HEX set values\n", num);
			else
				nc=SformatOutput(&tValue[lc],MBUFFER-lc-1,"Need %d set values\n", num);
			if(nc>0) lc+=nc;
			status=ERR_VALUE_BAD;
		}
	}

	if (status==ERR_VALUE_BAD) {
		nc=SformatOutput(&tValue[lc],MBUFFER-lc-1,"Reference format: %s=", name);
		if(nc>0) lc+=nc;
		for (i=0; i<num; i++) {
			if (isHex==1)
				nc=SformatOutput(&tValue[lc],MBUFFER-lc-1,"HEXv%d", i);
			else
				nc=SformatOutput(&tValue[lc],MBUFFER-lc-1,"v%d", i);
			if(nc>0) lc+=nc;
			if (i<num-1)
				nc=SformatOutput(&tValue[lc],MBUFFER-lc-1,",");
			else
				nc=SformatOutput(&tValue[lc],MBUFFER-lc-1,";\n");
			if(nc>0) lc+=nc;
		}
		return status;
	}
	return parseStatus;
}
