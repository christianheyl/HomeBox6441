// cal-1p.cpp : Defines the entry point for the DLL application.
//

//#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <ctype.h>
#include "smatch.h"
#include "ParameterSelect.h"
#include "CommandParse.h"
#include "ParseError.h"
#include "NartError.h"
#include "ErrorPrint.h"
#include "ParameterParse.h"

#include "calDLL.h"
#include "calibration.h"
#include "calibration_setup.h"
#include "UserPrint.h"

#define MBUFFER 1024
#define MAXVALUE 1000


struct _ParameterList CalSetParameter[]=
{
	CALIBRATION_SET_SCHEME,
	CALIBRATION_SET_POWER_GOAL,
	CALIBRATION_SET_GAIN_INDEX_SCHEME,
	CALIBRATION_SET_CALIBRATION_ATTEMPT,
	CALIBRATION_SET_TXGAIN_SLOPE,
	CALIBRATION_SET_POWER_DEVIATION,
	CALIBRATION_SET_2G_FREQ,
	CALIBRATION_SET_2G_GAIN_INDEX1_CH0,
	CALIBRATION_SET_2G_GAIN_INDEX1_CH1,
	CALIBRATION_SET_2G_GAIN_INDEX1_CH2,
	CALIBRATION_SET_2G_DAC_GAIN1,
	CALIBRATION_SET_2G_POWER_GOAL1,
	CALIBRATION_SET_2G_GAIN_INDEX2_CH0,
	CALIBRATION_SET_2G_GAIN_INDEX2_CH1,
	CALIBRATION_SET_2G_GAIN_INDEX2_CH2,
	CALIBRATION_SET_2G_DAC_GAIN2,
	CALIBRATION_SET_2G_POWER_GOAL2,
	CALIBRATION_SET_5G_FREQ,
	CALIBRATION_SET_5G_GAIN_INDEX1_CH0,
	CALIBRATION_SET_5G_GAIN_INDEX1_CH1,
	CALIBRATION_SET_5G_GAIN_INDEX1_CH2,
	CALIBRATION_SET_5G_DAC_GAIN1,
	CALIBRATION_SET_5G_POWER_GOAL1,
	CALIBRATION_SET_5G_GAIN_INDEX2_CH0,
	CALIBRATION_SET_5G_GAIN_INDEX2_CH1,
	CALIBRATION_SET_5G_GAIN_INDEX2_CH2,
	CALIBRATION_SET_5G_DAC_GAIN2,
	CALIBRATION_SET_5G_POWER_GOAL2,
	CALIBRATION_SET_RESET_UNUSED_CAL_PIERS,
};

// Parse the calibration setup file and get the initial gain settings in setup  file.
// Cart command line init value could overwrite the first calibration point init value in setup file.
CAL2P_API int Calibration_Scheme(void)
{
	int i;
	//UserPrint("Calibration_Scheme \n");
	// set to default value in case of calibration setup file is missing.
	cal.scheme=0;
	cal.PowerGoalMode = 0;
	cal.gainIndexScheme = 0;

	cal.txgainSlope = 2.0;
	cal.powerDeviation = 0.5;
	cal.txgainSlope2 = 2.0;
	cal.powerDeviation2 = 0.5;
	cal.attempt = 2;
	cal.attempt2 = 1;
	cal.txgainMin = 0;
	cal.txgainMax = 0x682;
	cal.txgain = 40;

	cal.PowerGoal = 16.0;
	cal.PowerGoal2 = 16.0;
	cal.resetUnusedCalPiers = 0;

	for(i=0; i<GAIN_CHANN_MAX_2G; i++)
		cal.gainIndex2Delta_2g[i] = 8;

	for(i=0; i<GAIN_CHANN_MAX_5G; i++)
		cal.gainIndex2Delta_5g[i] = 8;

#ifndef QDART_BUILD
	// read settings from calibration setup file 
	if (setup_file("calibration_setup.txt")<0) {
		return 0;
	}
#endif
	currentGain.txgain = cal.txgain;
	currentGain.gainIndex = cal.gainIndex;
	currentGain.dacGain = cal.dacGain;

	return cal.scheme;
}

CAL2P_API int Calibration_SetIniTxGain(int *totalGain, int maxGainEntry)
{

	int i;

	//UserPrint("Calibration_SetIniTxGain maxGainEntry %d\n", maxGainEntry);

	if (maxGainEntry>=MAX_INITXGAIN_ENTRY)
		return -1;
	cal.maxIniTxGainEntry = maxGainEntry;
	for(i=0; i<maxGainEntry; i++) {
		cal.GainTbl_totalGain[i] = totalGain[i];
	}
	return 0;
}

CAL2P_API int Calibration_SetMode(int freq, int rate, int chain)
{
	//  UserPrint("Calibration_SetMode freq %d, rate %d, chain %d \n", freq, rate, chain);

	cal.freq = freq;
	cal.rate = rate;
	cal.tMask = chain;

	return calibration_reset(freq);
}

CAL2P_API int Calibration_SetCalibrationPowerGoal(double PowerGoal, int iPoint)
{
	//  UserPrint("Calibration_SetCalibrationPowerGoal PowerGoal %f, iPoint %d \n", PowerGoal, iPoint);
	if (cal.PowerGoalMode==0) {
		if (iPoint==0)
			cal.PowerGoal = PowerGoal;
		else if (iPoint==1)
			cal.PowerGoal2 = PowerGoal;
	} 
	return 0;
}

// add for QDART_BUILD
CAL2P_API int Calibration_SetCalibrationCalculationValue(double *deltaTxPwrGoal, double *slope, int *iMaxIteration, int iPoint)
{
	if (iPoint==0) {
		cal.powerDeviation = deltaTxPwrGoal[0];
		cal.txgainSlope = slope[0];
		cal.attempt = iMaxIteration[0];
	} else if (iPoint==1) {
		cal.powerDeviation2 = deltaTxPwrGoal[1];
		cal.txgainSlope2 = slope[1];
		cal.attempt2 = iMaxIteration[1];
	}
	return 0;
}

// the tx command setting will over write what in calibration setup file
CAL2P_API int Calibration_SetTxGainInit(int txgain)
{
	//  UserPrint("Calibration_SetTxGainInit txgain %d\n", txgain);
	cal.txgain = txgain;
	currentGain.txgain = txgain;
	return 0;
}

// the tx command setting will over write what in calibration setup file
CAL2P_API int Calibration_SetTxGainMin(int txgainMin)
{
	//  UserPrint("Calibration_SetTxGainMin txgainMin %d\n", txgainMin);
	cal.txgainMin = txgainMin;
	return 0;
}

// the tx command setting will over write what in calibration setup file
CAL2P_API int Calibration_SetTxGainMax(int txgainMax)
{
	//  UserPrint("Calibration_SetTxGainMax txgainMax %d\n", txgainMax);
	cal.txgainMax = txgainMax;
	return 0;
}

// the tx command setting will over write what in calibration setup file
CAL2P_API int Calibration_SetGainIndexInit(int gainIndex)
{
	//  UserPrint("Calibration_SetGainIndexInit gainIndex %d\n", gainIndex);
	cal.gainIndex = gainIndex;
	currentGain.gainIndex = cal.gainIndex;
	return 0;
}

// the tx command setting will over write what in calibration setup file
CAL2P_API int Calibration_SetDacGainInit(int dacGain)
{
	//  UserPrint("Calibration_SetDacGainInit dacGain %d\n", dacGain);
	cal.dacGain = dacGain;
	currentGain.dacGain = cal.dacGain;
	return 0;
}

CAL2P_API int Calibration_TxGainReset(int *txGain, int *gainIndex, int *dacGain)
{
	int ip;
	//  UserPrint("Calibration_TxGainReset \n");

	if (cal.attempt==1) {
		updateGain(&ip, -1);
	} else {
		updateGainM(&ip, -1);
	}
	txGain[0] = currentGain.txgain;
	gainIndex[0] = currentGain.gainIndex;
	dacGain[0] = currentGain.dacGain;
	return 0;
}

// used when all init gain was setting from QSPR, not from calibration_setup.txt file
CAL2P_API int Calibration_SetGainInit(int *txGain, int *gainIndex, int *dacGain, int iPoint)
{
	int ip;
	//UserPrint("Calibration_SetGainInit \n");

	cal.txgain = txGain[0];
	cal.dacGain = dacGain[0];
	cal.gainIndex = gainIndex[0];
	if (iPoint==2) {
		cal.dacGain2 = dacGain[1];
		cal.gainIndex2 = gainIndex[1];
	}
	cal.txgainMin = 0;
	cal.txgainMax = 0x1a0;
	cal.resetUnusedCalPiers = 0;
	currentGain.txgain = cal.txgain;
	currentGain.gainIndex = cal.gainIndex;
	currentGain.dacGain = cal.dacGain;

	if (cal.attempt==1) {
		updateGain(&ip, -1);
	} else {
		updateGainM(&ip, -1);
	}
	return 0;
}

CAL2P_API int Calibration_GetTxGain(int *txgain, int *gainIndex, int *dacGain, int *calPoint, int ichain)
{
	//  UserPrint("Calibration_GetTxGain ichain %d \n", ichain);

	if (cal.attempt==1) {
		updateGain(calPoint, ichain);
	} else {
		updateGainM(calPoint, ichain);
	}
	txgain[0] = currentGain.txgain;
	gainIndex[0] = currentGain.gainIndex;
	dacGain[0] = currentGain.dacGain;
	return 0;
}

CAL2P_API int Calibration_Chain(int iChain)
{
	//  UserPrint("Calibration_Chain %d\n", iChain);
	return getChainMask(iChain);
}



CAL2P_API int Calibration_Calculation(double pwr_dBm, int iChain)
{
	//  UserPrint("Calibration_Calculation pwr_dBm %f, iChain %d\n", pwr_dBm, iChain);
	if (cal.attempt==1) {
		calibration_one(pwr_dBm, iChain);
	} else {
		calibration_next(pwr_dBm, iChain);
	}
	return 0;
}

CAL2P_API int Calibration_Status(int iChain)
{
	//  UserPrint("Calibration_Status iChain %d\n", iChain);
	return getStatus(iChain);
}

CAL2P_API int SetCalScheme(int *x)
{
	cal.scheme= x[0];
	return 0;
}

CAL2P_API int GetCalScheme(int *x)
{
	x[0]=cal.scheme;
	return 0;
}

static int SetCalPowerGoalMode(int *x)
{
		cal.PowerGoalMode = x[0];
		return 0;
}

static int GetCalPowerGoalMode(int *x)
{
		x[0] = cal.PowerGoalMode;
		return 0;
}

static int SetGainIndexScheme(int *x)
{
	cal.gainIndexScheme = x[0];	
	return 0;
}

static int GetGainIndexScheme(int *x)
{
	x[0] = cal.gainIndexScheme;	
	return 0;
}

static int SetCalibrationAttempt(int *x)
{
	cal.attempt = x[0];
	return 0;
}

static int GetCalibrationAttempt(int *x)
{
	x[0] = cal.attempt;
	return 0;
}
static int SetTxGainSlope(double *x)
{
	cal.txgainSlope = x[0];
	return 0;
}

static int GetTxGainSlope(double *x)
{
	x[0] = cal.txgainSlope ;
	return 0;
}

static int SetPowerDeviation(double *x)
{
	cal.powerDeviation = x[0];
	return 0;
}

static int GetPowerDeviation(double *x)
{
	x[0] = cal.powerDeviation;
	return 0;
}

static int Set2gFreq(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.gainChann_2g[i] = x[i];
	return 0;
}


static int Get2gFreq(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_2G; i++)
		x[i] = cal.gainChann_2g[i];
	return 0;
}

static int Set2gGainIndex1Ch0(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.gainIndex_2g_ch0[i] = x[i];
	return 0;
}

static int Get2gGainIndex1Ch0(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_2G; i++)
		x[i] = cal.gainIndex_2g_ch0[i];
	return 0;
}

static int Set2gGainIndex1Ch1(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.gainIndex_2g_ch1[i] = x[i];
	return 0;
}

static int Get2gGainIndex1Ch1(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_2G; i++)
		x[i] = cal.gainIndex_2g_ch1[i];
	return 0;
}

static int Set2gGainIndex1Ch2(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.gainIndex_2g_ch2[i] = x[i];
	return 0;
}

static int Get2gGainIndex1Ch2(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_2G; i++)
		x[i] = cal.gainIndex_2g_ch2[i];
	return 0;
}

static int Set2gDacGain1(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.dacGain_2g[i] = x[i];
	return 0;
}

static int Get2gDacGain1(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_2G; i++)
		x[i] = cal.dacGain_2g[i];
	return 0;
}

static int Set2gPowerGoal1(double *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.PowerGoal_2g[i] = x[i];
	return 0;
}

static int Get2gPowerGoal1(double *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_2G; i++)
		x[i] = cal.PowerGoal_2g[i];
	return 0;
}

static int Set2gGainIndex2Ch0(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.gainIndex2_2g_ch0[i] = x[i];
	return 0;
}

static int Get2gGainIndex2Ch0(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_2G; i++)
		 x[i] = cal.gainIndex2_2g_ch0[i];
	return 0;
}

static int Set2gGainIndex2Ch1(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.gainIndex2_2g_ch1[i] = x[i];
	return 0;
}

static int Get2gGainIndex2Ch1(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_2G; i++)
		x[i] = cal.gainIndex2_2g_ch1[i];
	return 0;
}

static int Set2gGainIndex2Ch2(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.gainIndex2_2g_ch2[i] = x[i];
	return 0;
}

static int Get2gGainIndex2Ch2(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_2G; i++)
		x[i] = cal.gainIndex2_2g_ch2[i];
	return 0;
}

static int Set2gDacGain2(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.dacGain2_2g[i] = x[i];
	return 0;
}

static int Get2gDacGain2(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_2G; i++)
		x[i] = cal.dacGain2_2g[i];
	return 0;
}

static int Set2gPowerGoal2(double *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.PowerGoal2_2g[i] = x[i];
	return 0;
}

static int Get2gPowerGoal2(double *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_2G; i++)
		x[i] = cal.PowerGoal2_2g[i];
	return 0;
}

static int Set5gFreq(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.gainChann_5g[i] = x[i];
	return 0;
}

static int Get5gFreq(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_5G; i++)
		x[i] = cal.gainChann_5g[i];
	return 0;
}

static int Set5gGainIndex1Ch0(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.gainIndex_5g_ch0[i] = x[i];
	return 0;
}

static int Get5gGainIndex1Ch0(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_5G; i++)
		x[i] = cal.gainIndex_5g_ch0[i];
	return 0;
}

static int Set5gGainIndex1Ch1(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.gainIndex_5g_ch1[i] = x[i];
	return 0;
}

static int Get5gGainIndex1Ch1(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_5G; i++)
		 x[i] = cal.gainIndex_5g_ch1[i];
	return 0;
}

static int Set5gGainIndex1Ch2(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.gainIndex_5g_ch2[i] = x[i];
	return 0;
}

static int Get5gGainIndex1Ch2(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_5G; i++)
		x[i] = cal.gainIndex_5g_ch2[i];
	return 0;
}

static int Set5gDacGain1(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.dacGain_5g[i] = x[i];
	return 0;
}

static int Get5gDacGain1(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_5G; i++)
		x[i] = cal.dacGain_5g[i];
	return 0;
}

static int Set5gPowerGoal1(double *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.PowerGoal_5g[i] = x[i];
	return 0;
}

static int Get5gPowerGoal1(double *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_5G; i++)
		x[i] = cal.PowerGoal_5g[i];
	return 0;
}

static int Set5gGainIndex2Ch0(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.gainIndex2_5g_ch0[i] = x[i];
	return 0;
}

static int Get5gGainIndex2Ch0(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_5G; i++)
		x[i] = cal.gainIndex2_5g_ch0[i];
	return 0;
}

static int Set5gGainIndex2Ch1(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.gainIndex2_5g_ch1[i] = x[i];
	return 0;
}

static int Get5gGainIndex2Ch1(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_5G; i++)
		x[i] = cal.gainIndex2_5g_ch1[i];
	return 0;
}

static int Set5gGainIndex2Ch2(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.gainIndex2_5g_ch2[i] = x[i];
	return 0;
}

static int Get5gGainIndex2Ch2(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_5G; i++)
		x[i] = cal.gainIndex2_5g_ch2[i];
	return 0;
}

static int Set5gDacGain2(int *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.dacGain2_5g[i] = x[i];
	return 0;
}

static int Get5gDacGain2(int *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_5G; i++)
		x[i] = cal.dacGain2_5g[i];
	return 0;
}

static int Set5gPowerGoal2(double *x, int count)
{
	int i;
	for(i=0; i<count; i++)
		cal.PowerGoal2_5g[i] = x[i];
	return 0;
}

static int Get5gPowerGoal2(double *x)
{
	int i;
	for(i=0; i<GAIN_CHANN_MAX_5G; i++)
		x[i] = cal.PowerGoal2_5g[i];
	return 0;
}

static int SetResetUnusedCalPiers(int *x)
{
	cal.resetUnusedCalPiers = x[0];
	return 0;
}

CAL2P_API int GetResetUnusedCalPiers(int *x)
{
	x[0] = cal.resetUnusedCalPiers;
	return 0;
}

#ifdef NOT_USED
static void ReturnUnsigned(char *command, char *name, char *atext, int *value, int nvalue)
{
	char buffer[MBUFFER];
	int lc, nc;
	int it;

	lc=0;
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%s|%s%s|%u",command,name,atext,value[0]);
	if(nc>0)
	{
		lc+=nc;
	}
	for(it=1; it<nvalue; it++)
	{
		nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,",%u",value[it]);
		if(nc>0)
		{
			lc+=nc;
		}
	}
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|");
    buffer[MBUFFER-1]=0;
	ErrorPrint(NartData,buffer);
}
#endif

static void ReturnSigned(char *command, char *name, int *value, int nvalue)
{
	char buffer[MBUFFER];
	int lc, nc;
	int it;

	lc=0;
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%s|%s|%d",command,name,value[0]);
	if(nc>0)
	{
		lc+=nc;
	}
	for(it=1; it<nvalue; it++)
	{
		nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,",%d",value[it]);
		if(nc>0)
		{
			lc+=nc;
		}
	}
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|");
    buffer[MBUFFER-1]=0;
	ErrorPrint(NartData,buffer);
}

static void ReturnDouble(char *command, char *name, double *value, int nvalue)
{
	char buffer[MBUFFER];
	int lc, nc;
	int it;

	lc=0;
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|%s|%s|%.1lf",command,name,value[0]);
	if(nc>0)
	{
		lc+=nc;
	}
	for(it=1; it<nvalue; it++)
	{
		nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,",%.1lf",value[it]);
		if(nc>0)
		{
			lc+=nc;
		}
	}
	nc=SformatOutput(&buffer[lc],MBUFFER-lc-1,"|");
    buffer[MBUFFER-1]=0;
	ErrorPrint(NartData,buffer);
}

static void CalfSet(int *done, int *error, int ip, int index, int (*_pSetCal)(double *value))
{
	int status;
	int ngot=0;
	char *name;	
    double value[MAXVALUE]; 

	name=CommandParameterName(ip);
	ngot=ParseDoubleList(ip,name,value,&CalSetParameter[index]);
	if(ngot>0) {
		status=_pSetCal(value);	
		ReturnDouble("set",CalSetParameter[index].word[0],value,ngot);
	}
}

static void CalSet(int *done, int *error, int ip, int index, int (*_pSetCal)(int *value))
{
	int status;
	int ngot=0;
	char *name;	
	unsigned int uvalue[MAXVALUE]; 

	status=0;
	name=CommandParameterName(ip);
	ngot=ParseIntegerList(ip,name,(int *)uvalue,&CalSetParameter[index]);
	if(ngot>0) {
		_pSetCal((int *)uvalue);
		ReturnSigned("SetCal",CalSetParameter[index].word[0],(int *)uvalue,ngot);
	}

	if (ngot<=0) {
		(*error)+=1; 
	} else {
		(*done)++;
	}
}


static void CalfSets(int *done, int *error, int ip, int index, int (*_pSetCal)(double *value, int ix))
{
	int status;
	int ngot=0;
	char *name;	
    double value[MAXVALUE]; 

	name=CommandParameterName(ip);
	ngot=ParseDoubleList(ip,name,value,&CalSetParameter[index]);
	if(ngot>0) {
		status=_pSetCal(value, ngot);	
		ReturnDouble("set",CalSetParameter[index].word[0],value,ngot);
	}
}


static void CalSets(int *done, int *error, int ip, int index, int (*_pSetCal)(int *value, int ix))
{
	int status;
	int ngot=0;
	char *name;	
	unsigned int uvalue[MAXVALUE]; 

	status=0;
	name=CommandParameterName(ip);
	ngot=ParseIntegerList(ip,name,(int *)uvalue,&CalSetParameter[index]);
	if(ngot>0) {
		_pSetCal((int *)uvalue, ngot);
		ReturnSigned("SetCal",CalSetParameter[index].word[0],(int *)uvalue,ngot);
	}

	if (ngot<=0) {
		(*error)+=1; 
	} else {
		(*done)++;
	}
}
CAL2P_API int Calibration_SetCommand(int client)
{
	int np, ip;
	char *name;	
	int error;
	int done;
	int index;
    int code;
	int ix, iy, iz;

	error=0;
	done=0;

	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		index=ParameterSelectIndexArray(name,&CalSetParameter[0],sizeof(CalSetParameter)/sizeof(CalSetParameter[0]), &ix, &iy, &iz);
		if(index>=0)
		{
			code=CalSetParameter[index].code;
			switch(code) 
			{
				case CalSetScheme:
					CalSet(&done, &error, ip, index, SetCalScheme);
					break;
				case CalSetPowerGoalMode:
					CalSet(&done, &error, ip, index, SetCalPowerGoalMode);
					break;
				case CalSetGainIndexScheme:
					CalSet(&done, &error, ip, index, SetGainIndexScheme);
					break;
				case CalSetCalibrationAttempt:
					CalSet(&done, &error, ip, index, SetCalibrationAttempt);
					break;
				case CalSetTxGainSlope:
					CalfSet(&done, &error, ip, index, SetTxGainSlope);
					break;
				case CalSetPowerDeviation:
					CalfSet(&done, &error, ip, index, SetPowerDeviation);
					break;
				case CalSet2gFreq:
					CalSets(&done, &error, ip, index, Set2gFreq);
					break;
				case CalSet2gGainIndex1Ch0:
					CalSets(&done, &error, ip, index, Set2gGainIndex1Ch0);
					break;
				case CalSet2gGainIndex1Ch1:
					CalSets(&done, &error, ip, index, Set2gGainIndex1Ch1);
					break;
				case CalSet2gGainIndex1Ch2:
					CalSets(&done, &error, ip, index, Set2gGainIndex1Ch2);
					break;
				case CalSet2gDacGain1:
					CalSets(&done, &error, ip, index, Set2gDacGain1);
					break;
				case CalSet2gPowerGoal1:
					CalfSets(&done, &error, ip, index, Set2gPowerGoal1);
					break;
				case CalSet2gGainIndex2Ch0:
					CalSets(&done, &error, ip, index, Set2gGainIndex2Ch0);
					break;
				case CalSet2gGainIndex2Ch1:
					CalSets(&done, &error, ip, index, Set2gGainIndex2Ch1);
					break;
				case CalSet2gGainIndex2Ch2:
					CalSets(&done, &error, ip, index, Set2gGainIndex2Ch2);
					break;
				case CalSet2gDacGain2:
					CalSets(&done, &error, ip, index, Set2gDacGain2);
					break;
				case CalSet2gPowerGoal2:
					CalfSets(&done, &error, ip, index, Set2gPowerGoal2);
					break;
				case CalSet5gFreq:
					CalSets(&done, &error, ip, index, Set5gFreq);
					break;
				case CalSet5gGainIndex1Ch0:
					CalSets(&done, &error, ip, index, Set5gGainIndex1Ch0);
					break;
				case CalSet5gGainIndex1Ch1:
					CalSets(&done, &error, ip, index, Set5gGainIndex1Ch1);
					break;
				case CalSet5gGainIndex1Ch2:
					CalSets(&done, &error, ip, index, Set5gGainIndex1Ch2);
					break;
				case CalSet5gDacGain1:
					CalSets(&done, &error, ip, index, Set5gDacGain1);
					break;
				case CalSet5gPowerGoal1:
					CalfSets(&done, &error, ip, index, Set5gPowerGoal1);
					break;
				case CalSet5gGainIndex2Ch0:
					CalSets(&done, &error, ip, index, Set5gGainIndex2Ch0);
					break;
				case CalSet5gGainIndex2Ch1:
					CalSets(&done, &error, ip, index, Set5gGainIndex2Ch1);
					break;
				case CalSet5gGainIndex2Ch2:
					CalSets(&done, &error, ip, index, Set5gGainIndex2Ch2);
					break;
				case CalSet5gDacGain2:
					CalSets(&done, &error, ip, index, Set5gDacGain2);
					break;
				case CalSet5gPowerGoal2:
					CalfSets(&done, &error, ip, index, Set5gPowerGoal2);
					break;
				case CalSetResetUnusedCalPiers:
					CalSet(&done, &error, ip, index, SetResetUnusedCalPiers);
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
	return 0;
}


CAL2P_API int Calibration_GetCommand(int client)
{
	int np, ip;
	char *name;	
	int error;
	int status;
	int index;
    int code;
	int ix, iy, iz;
    int value[MAXVALUE]; 
    double dvalue[MAXVALUE]; 


	error=0;

	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		index=ParameterSelectIndexArray(name,&CalSetParameter[0],sizeof(CalSetParameter)/sizeof(CalSetParameter[0]), &ix, &iy, &iz);
		if(index>=0)
		{
			code=CalSetParameter[index].code;
			switch(code) 
			{
				case CalSetScheme:
					status = GetCalScheme(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,1);
					break;
				case CalSetPowerGoalMode:
					status = GetCalPowerGoalMode(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,1);
					break;
				case CalSetGainIndexScheme:
					status = GetGainIndexScheme(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,1);
					break;
				case CalSetCalibrationAttempt:
					status = GetCalibrationAttempt(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,1);
					break;
				case CalSetTxGainSlope:
					status = GetTxGainSlope(dvalue);
					ReturnDouble("get",CalSetParameter[index].word[0],dvalue,1);
					break;
				case CalSetPowerDeviation:
					status = GetPowerDeviation(dvalue);
					ReturnDouble("get",CalSetParameter[index].word[0],dvalue,1);
					break;
				case CalSet2gFreq:
					status = Get2gFreq(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_2G);
					break;
				case CalSet2gGainIndex1Ch0:
					status = Get2gGainIndex1Ch0(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_2G);
					break;
				case CalSet2gGainIndex1Ch1:
					status = Get2gGainIndex1Ch1(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_2G);
					break;
				case CalSet2gGainIndex1Ch2:
					status = Get2gGainIndex1Ch2(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_2G);
					break;
				case CalSet2gDacGain1:
					status = Get2gDacGain1(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_2G);
					break;
				case CalSet2gPowerGoal1:
					status = Get2gPowerGoal1(dvalue);
					ReturnDouble("get",CalSetParameter[index].word[0],dvalue,GAIN_CHANN_MAX_2G);
					break;
				case CalSet2gGainIndex2Ch0:
					status = Get2gGainIndex2Ch0(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_2G);
					break;
				case CalSet2gGainIndex2Ch1:
					status = Get2gGainIndex2Ch1(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_2G);
					break;
				case CalSet2gGainIndex2Ch2:
					status = Get2gGainIndex2Ch2(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_2G);
					break;
				case CalSet2gDacGain2:
					status = Get2gDacGain2(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_2G);
					break;
				case CalSet2gPowerGoal2:
					status = Get2gPowerGoal2(dvalue);
					ReturnDouble("get",CalSetParameter[index].word[0],dvalue,GAIN_CHANN_MAX_2G);
					break;
				case CalSet5gFreq:
					status = Get5gFreq(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_5G);
					break;
				case CalSet5gGainIndex1Ch0:
					status = Get5gGainIndex1Ch0(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_5G);
					break;
				case CalSet5gGainIndex1Ch1:
					status = Get5gGainIndex1Ch1(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_5G);
					break;
				case CalSet5gGainIndex1Ch2:
					status = Get5gGainIndex1Ch2(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_5G);
					break;
				case CalSet5gDacGain1:
					status = Get5gDacGain1(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_5G);
					break;
				case CalSet5gPowerGoal1:
					status = Get5gPowerGoal1(dvalue);
					ReturnDouble("get",CalSetParameter[index].word[0],dvalue,GAIN_CHANN_MAX_5G);
					break;
				case CalSet5gGainIndex2Ch0:
					status = Get5gGainIndex2Ch0(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_5G);
					break;
				case CalSet5gGainIndex2Ch1:
					status = Get5gGainIndex2Ch1(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_5G);
					break;
				case CalSet5gGainIndex2Ch2:
					status = Get5gGainIndex2Ch2(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_5G);
					break;
				case CalSet5gDacGain2:
					status = Get5gDacGain2(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,GAIN_CHANN_MAX_5G);
					break;
				case CalSet5gPowerGoal2:
					status = Get5gPowerGoal2(dvalue);
					ReturnDouble("get",CalSetParameter[index].word[0],dvalue,GAIN_CHANN_MAX_5G);
					break;
				case CalSetResetUnusedCalPiers:
					status = GetResetUnusedCalPiers(value);
					ReturnSigned("get",CalSetParameter[index].word[0],value,1);
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
			return -1;
		}
	}
	return 0;
}

CAL2P_API int Calibration_ParameterSplice(struct _ParameterList *list)
{
    list->nspecial=(sizeof(CalSetParameter)/sizeof(CalSetParameter[0]));
    list->special=&CalSetParameter[0];
	return 0;
}
