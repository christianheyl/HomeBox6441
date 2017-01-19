#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <ctype.h>
#include "UserPrint.h"
#include "ErrorPrint.h"
#include "LinkError.h"
#include "ParameterSelect.h"
#include "calDLL.h"
#include "calibration.h"
#include "calibration_setup.h"

#define MATTEMPT 10
static int status[MCHAIN];
static int CalibrateGain[MCHAIN][MATTEMPT];
static double CalibratePower[MCHAIN][MATTEMPT];
static int CalibrateAttempt[MCHAIN], attempt;
static double PowerGoal, txgainSlope, powerDeviation;
static int testPoint=0;
static int status_done = CALNEXT_DoneOne;
static int 	lastTxgain;

int channFind(int chann)
{
	int it = -1;
	if (chann<2500) 
	{
		for (it=0; it<GAIN_CHANN_MAX_2G; it++) {
			if (chann==cal.gainChann_2g[it]) {
				break;
			}
		}

		if (it >= GAIN_CHANN_MAX_2G) 
		{
			ErrorPrint(CalibrateChannSelectFail,
				"calibration channel %d doesn't match what in calibration_setup file\n", chann); 
			return -1;
		}
	} 
	else 
	{
		for (it=0; it<GAIN_CHANN_MAX_5G; it++) {
			if (chann==cal.gainChann_5g[it]) {
				break;
			}
		}

		if (it >= GAIN_CHANN_MAX_5G) 
		{
			ErrorPrint(CalibrateChannSelectFail,
				"calibration channel %d doesn't match what in calibration_setup file\n", chann); 
			return -1;
		}
	}

	if (chann<2500) 
	{
		if(cal.tMask == 0x1) {
			cal.gainIndex = cal.gainIndex_2g_ch0[it];
		} else if(cal.tMask == 0x2){
			cal.gainIndex = cal.gainIndex_2g_ch1[it];
		} else if(cal.tMask == 0x4){
			cal.gainIndex = cal.gainIndex_2g_ch2[it];
		}
		cal.dacGain = cal.dacGain_2g[it];
		cal.dacGain2 = cal.dacGain2_2g[it];
		if (cal.gainIndexScheme == 0) {
			if(cal.tMask == 0x1) {
				cal.gainIndex2 = cal.gainIndex2_2g_ch0[it];
			} else if(cal.tMask == 0x2) {
				cal.gainIndex2 = cal.gainIndex2_2g_ch1[it];
			}else if(cal.tMask == 0x4) {
				cal.gainIndex2 = cal.gainIndex2_2g_ch2[it];
			}
		} else {
			cal.gainIndexDelta = cal.gainIndex2Delta_2g[it];
		}

		if (cal.scheme == 2)
		{
			// 2 point calibration uses this anyways
			cal.gainIndexDelta = cal.gainIndex2Delta_2g[it];
		}

		if (cal.PowerGoalMode != 0)
		{
			cal.PowerGoal = cal.PowerGoal_2g[it];
			cal.PowerGoal2 = cal.PowerGoal2_2g[it];
		}
	} 
	else 
	{
		if(cal.tMask == 0x1) {
			cal.gainIndex = cal.gainIndex_5g_ch0[it];
		} else if(cal.tMask == 0x2) {
			cal.gainIndex = cal.gainIndex_5g_ch1[it];
		} else if(cal.tMask == 0x4) {
			cal.gainIndex = cal.gainIndex_5g_ch2[it];
		}
		cal.dacGain = cal.dacGain_5g[it];
		cal.dacGain2 = cal.dacGain2_5g[it];
		if (cal.gainIndexScheme == 0) {
			if(cal.tMask == 0x1) {
				cal.gainIndex2 = cal.gainIndex2_5g_ch0[it];
			} else if(cal.tMask == 0x2) {
				cal.gainIndex2 = cal.gainIndex2_5g_ch1[it];
			} else if(cal.tMask == 0x4) {
				cal.gainIndex2 = cal.gainIndex2_5g_ch2[it];
			}
		} else {
			cal.gainIndexDelta = cal.gainIndex2Delta_5g[it];
		}

		if (cal.scheme == 2)
		{
			// 2 point calibration uses this anyways
			cal.gainIndexDelta = cal.gainIndex2Delta_5g[it];
		}

		if (cal.PowerGoalMode != 0)
		{
			cal.PowerGoal = cal.PowerGoal_5g[it];
			cal.PowerGoal2 = cal.PowerGoal2_5g[it];
		}
	}
	return 0;
}

int getGainIndexFromTxGain(int txgain)
{
	int i=0;
	// this formular works for calculate Osprey gainIndex
	// Might not work for peregrine fix dacGain 2 point calibration algorithm
	while ( (txgain > cal.GainTbl_totalGain[i]) && (i<(cal.maxIniTxGainEntry-1)) )
	{
		i++;
	}
	return i;
}

// This algorithm is for Peregrine - 2 point calibration
//
int CalibrateGainNext2Point(int txgain, double measuredPower)
{
	double GAIN_STEP = (0.125);
	double DISTANCE_FROM_FIRST_POINT = cal.gainIndexDelta;//read from the cal setup 6.0; //first point and second points are 6dB apart
	int i=0;
	double targetPower = PowerGoal;
	//int currentGainIndex = txgain;
	int newTxGainValue = 0;
	double deltaInPower = targetPower - measuredPower;
	int closestGainIndex = 0;
	int closestGain2Index = 0;
	int changeInGainValue = (int)(deltaInPower/GAIN_STEP);
	//int curGain2Val = cal.GainTbl_totalGain[cal.gainIndex2];
	int newTxGain2Value = 0;
	int newGainIndex = 0;

	if (testPoint == 0)
	{
		// we need to reduce the power if we get higher than expected power
		// i.e., deltaInPower is negative
		// OR we need to increase the power if we get lower than expected power
		if (deltaInPower < 0)
		{
			newTxGainValue = txgain - abs(changeInGainValue);		
			//newTxGain2Value = curGain2Val - abs(changeInGainValue);
		}
		else
		{		
			newTxGainValue =  txgain + changeInGainValue;
			//newTxGain2Value = curGain2Val + changeInGainValue;	
		}


		for (i=0; i<cal.maxIniTxGainEntry; i++)
		{
			if (abs(cal.GainTbl_totalGain[i] - newTxGainValue) < abs(cal.GainTbl_totalGain[closestGainIndex] - newTxGainValue))
			{
				closestGainIndex = i;
			}			
		}
		
		newTxGain2Value = cal.GainTbl_totalGain[closestGainIndex] + 
			(DISTANCE_FROM_FIRST_POINT/GAIN_STEP); // second point is away from first point by x dbs
		
		for (i=0; i<cal.maxIniTxGainEntry; i++)
		{
			if (abs(cal.GainTbl_totalGain[i] - newTxGain2Value) < abs(cal.GainTbl_totalGain[closestGain2Index] - newTxGain2Value))
			{
				closestGain2Index = i;
			}			
		}

		cal.gainIndex2 = closestGain2Index;

//		UserPrint("CalibrateGainNext2Point() txgain %d deltaInPower %f newTxGain %d, newTxGain2Value %d cal.GainTbl_totalGain[%d] %d cal.GainTbl_totalGain[%d] %d cal.gainIndexDelta %d\n", 
//			txgain, deltaInPower, 
//			newTxGainValue, newTxGain2Value,
//			closestGainIndex, cal.GainTbl_totalGain[closestGainIndex], 
//			closestGain2Index, cal.GainTbl_totalGain[closestGain2Index],
//			cal.gainIndexDelta);	
	}

	if (testPoint == 0)
	{
		newGainIndex = cal.GainTbl_totalGain[closestGainIndex];
	}
	else 
	{
		newGainIndex = cal.GainTbl_totalGain[closestGain2Index];
	}

	return newGainIndex;
}

int getDacGainFromTxGain(int txgain)
{
	int indx;

	//UserPrint("getDacGainFromTxGain txgain %d\n", txgain);
	indx = getGainIndexFromTxGain(txgain);
	
	if (indx<0 || indx>=cal.maxIniTxGainEntry)
		return -1000;
	
	return ( cal.GainTbl_totalGain[indx] - txgain);
}

int getTxGainFromGainIndexAndDacGain(int indx, int dacGain)
{
	//UserPrint("getTxGainFromGainIndexAndDacGain indx %d, dacGain %d\n", indx, dacGain);

	if (indx<0 || indx>=cal.maxIniTxGainEntry)
		return -1000;
	
	return (cal.GainTbl_totalGain[indx]+dacGain);
}

// Will be called when freq, chain, rate changes.
int calibration_reset(int freq)
{
	int ich;

	//UserPrint("calibration_reset freq %d \n", freq);
	testPoint = 0;

#ifndef QDART_BUILD
	if (channFind(freq)<0)
		return -1;
#endif

	currentGain.txgain = cal.txgain;
	currentGain.gainIndex = cal.gainIndex;
	currentGain.dacGain = cal.dacGain;
	

	for(ich=0; ich<MCHAIN; ich++)
	{
		if(cal.tMask&(1<<ich))
		{
			status[ich] = CALNEXT_UNINIT;
			CalibrateAttempt[ich]=0;
		}
	}
	
	return 0;
}

int updateGainM(int *calPoint, int ichain)
{

	int ich=0;
	//UserPrint("updateGainM ichain %d\n", ichain);

	if (ichain<0) {
		if ( status_done == CALNEXT_DoneTwo) {
			status_done = CALNEXT_DoneOne;
		}
	}

	if ( status_done == CALNEXT_DoneTwo) 
	{
		*calPoint=1;
		testPoint = 1;
		
		//UserPrint("updateGainM calPoint %d\n", *calPoint);
		
		attempt = 1;
		currentGain.txgain = getCalibrationGain(ichain);
		if (currentGain.txgain ==-1)
			currentGain.txgain = lastTxgain;	// reach the defined attemp.
		if (status[ichain]==CALNEXT_DoneOne) {
			if (cal.gainIndexScheme==0) {
				// 2nd gainIndex defined in calibration setup file
				currentGain.gainIndex = cal.gainIndex2;
			} else {
				// get gain Index fro the init delat + 1st cal point gainIndex
				currentGain.gainIndex = getGainIndexFromTxGain(lastTxgain) + cal.gainIndexDelta;
			}
		} else {
		// get gainIndex from calculated txgain	
			currentGain.gainIndex = getGainIndexFromTxGain(currentGain.txgain);
		}
		currentGain.dacGain = cal.dacGain2;
		currentGain.txgain = getTxGainFromGainIndexAndDacGain(currentGain.gainIndex, currentGain.dacGain);
	} 
	else 
	{
		*calPoint=0;
		testPoint = 0;
		//UserPrint("UpdateGainM calPoint %d\n", *calPoint);
		if (ichain>=0) {
			currentGain.txgain = getCalibrationGain(ichain);
		} else {
			// initialize, only called when setting freq, chain
			PowerGoal = cal.PowerGoal;
			txgainSlope = cal.txgainSlope;
			powerDeviation = cal.powerDeviation;
			attempt = cal.attempt;
		} 
		if (cal.scheme==0) {
			currentGain.gainIndex = getGainIndexFromTxGain(currentGain.txgain);
			currentGain.dacGain = getDacGainFromTxGain(currentGain.txgain);
		} else if (cal.scheme==2) {
			currentGain.dacGain = cal.dacGain;
			if (ichain>=0) {
				currentGain.gainIndex = getGainIndexFromTxGain(currentGain.txgain);
			} else {
				currentGain.gainIndex = cal.gainIndex;
				currentGain.txgain = getTxGainFromGainIndexAndDacGain(cal.gainIndex, cal.dacGain);
			}
		}
	}
	
	for(ich=0; ich<MCHAIN; ich++) {
		if(cal.tMask&(1<<ich))
		{
			CalibrateGain[ich][CalibrateAttempt[ich]]= currentGain.txgain;
		}
		else
		{
			if (CalibrateAttempt[ich]==0) {
				CalibrateGain[ich][CalibrateAttempt[ich]]= -1;
			}
		}
	}

	return 0;	
}

int updateGain(int *calPoint, int ichain)
{
	int ich=0;

	*calPoint=testPoint;
	//UserPrint("updateGain ichain %d calPoint %d\n", ichain, *calPoint);

	if (cal.scheme==0) {
		currentGain.txgain = cal.txgain;
		currentGain.gainIndex = getGainIndexFromTxGain(currentGain.txgain);
		currentGain.dacGain = getDacGainFromTxGain(currentGain.txgain);
	} else if (cal.scheme==2) {
		if (testPoint ==0) {
			currentGain.dacGain = cal.dacGain;
			currentGain.gainIndex = cal.gainIndex;
			currentGain.txgain = getTxGainFromGainIndexAndDacGain(cal.gainIndex, cal.dacGain);
		} else {
			currentGain.dacGain = cal.dacGain2;
			if (cal.gainIndexScheme==0) {
				// 2nd gainIndex defined in calibration setup file
				currentGain.gainIndex = cal.gainIndex2;
			} else {
				// get gain Index fro the init delat + 1st cal point gainIndex
				currentGain.gainIndex = cal.gainIndex + cal.gainIndexDelta;
			}
			currentGain.txgain = getTxGainFromGainIndexAndDacGain(currentGain.gainIndex, currentGain.dacGain);
		}
	}

	for(ich=0; ich<MCHAIN; ich++) {
		if(cal.tMask&(1<<ich))
		{
			CalibrateGain[ich][CalibrateAttempt[ich]]= currentGain.txgain;
		}
		else
		{
			if (CalibrateAttempt[ich]==0) {
				CalibrateGain[ich][CalibrateAttempt[ich]]= -1;
			}
		}
	}

	return 0;	
}

static int CalibrateGainNext(int chain)
{
	int it;
	int high, low, high2, low2;
	double dpower, dgain;

	if(CalibrateAttempt[chain]==0)
	{
		//
		// first attempt, use the gain supplied by the user
		//
		return CalibrateGain[chain][0];
	}
	else if(CalibrateAttempt[chain]==1)
	{
		//
		// presume the power/gain slope is 2 and extrapolate
		//
		return (int)(txgainSlope*(PowerGoal-CalibratePower[chain][0])+CalibrateGain[chain][0]);
	}
	else if(CalibrateAttempt[chain]==2)
	{
		//
		// compute the actual power/gain slope and interpolate or extrapolate
		//
		dpower=CalibratePower[chain][1]-CalibratePower[chain][0];
		dgain=CalibrateGain[chain][1]-CalibrateGain[chain][0];
		if(dpower!=0)
		{
			return (int)((dgain/dpower)*(PowerGoal-CalibratePower[chain][0])+CalibrateGain[chain][0]);
		}
		else
		{
			return (int)(CalibrateGain[chain][1]+dgain);
		}
	}
	else
	{
		//
		// try to find the closest measurement below and above the target. then interpolate.
		//
		high= -1;
		low= -1;
		high2= -1;
		low2= -1;
		for(it=0; it<CalibrateAttempt[chain]; it++)
		{
			dpower=PowerGoal-CalibratePower[chain][it];
			if(dpower>=0)
			{
				if(low<0 || dpower<(PowerGoal-CalibratePower[chain][low]))
				{
					low2=low;
					low=it;
				}
				else if(low2<0 || dpower<(PowerGoal-CalibratePower[chain][low2]))
				{
					low2=it;
				}
			}
			else
			{
				if(high<0 || dpower<(PowerGoal-CalibratePower[chain][high]))
				{
					high2=high;
					high=it;
				}
				else if(high2<0 || dpower<(PowerGoal-CalibratePower[chain][high2]))
				{
					high2=it;
				}
			}
		}
		if(!(low>=0 && high>=0 && low!=high))
		{
			//
			// if we don't have a high and a low measurment, substitute the next lowest or next highest
			//
			if(low!=0)
			{
				high=low2;
			}
			else
			{
				low=high2;
			}
		}
		//
		// compute the actual power/gain slope and interpolate
		//
		dpower=CalibratePower[chain][high]-CalibratePower[chain][low];
		dgain=CalibrateGain[chain][high]-CalibrateGain[chain][low];
		if(dpower!=0)
		{
			return (int)((dgain/dpower)*(PowerGoal-CalibratePower[chain][low])+CalibrateGain[chain][low]);
		}
		else
		{
			return (int)(CalibrateGain[chain][1]+dgain);
		}
	}
}

int getStatus(int ich)
{
	
	int retStatus = 0;
	
	if (status[ich]==CALNEXT_DoneOne)
		retStatus = CALNEXT_TxStart;
	else
		retStatus = status[ich];

	//UserPrint("getStatus ich %d, retStatus %d\n", ich, retStatus);
	return retStatus;
}

int getCalibrationGain(int ichain)
{
	//UserPrint("getCalibrationGain ichain %d\n", ichain);
	return CalibrateGain[ichain][CalibrateAttempt[ichain]];
}

int getChainMask(ich)
{
	int calChain, jch;
	int cdiff;
	//UserPrint("getChainMask ich %d\n", ich);


	calChain=(1<<ich);
//	UserPrint("redo calibration for chain mask=%x at txgain=%d\n",_LinkCalibrateChain,_LinkPcdac);
	//
	// see if we can do any of the other chains at the same time
	//
    for(jch=ich+1; jch<MCHAIN; jch++)
	{						
		if(CalibrateGain[jch][CalibrateAttempt[jch]]>0)
        {
			cdiff = CalibrateGain[jch][CalibrateAttempt[jch]]- getCalibrationGain(ich);
			if(-(powerDeviation*txgainSlope) < cdiff && cdiff <= (powerDeviation*txgainSlope))
			{
				calChain|=(1<<jch);
			}
		}
	}
	cal.tMask = calChain;
	return cal.tMask;
}


int calibration_next(double power, int ichain)
{
	double dpower=0.0;

	//UserPrint("calibration_next(): power %f, ichain %d testPoint %d\n", power, ichain, testPoint);

 	if (status[ichain] == CALNEXT_DoneOne) 
	{
		if (cal.attempt<=1) 
		{
			status[ichain] = CALNEXT_Done;
			status_done = CALNEXT_DoneOne;
			return 0;
		} 
		else 
		{
			PowerGoal = cal.PowerGoal;
			txgainSlope = cal.txgainSlope;
			powerDeviation = cal.powerDeviation;
			//attempt = cal.attempt;
			attempt = 1;
			CalibrateAttempt[ichain]=0;
		}
	}
	else if (status[ichain] == CALNEXT_DoneTwo) 
	{
			status[ichain] = CALNEXT_Done;
			status_done = CALNEXT_DoneOne;
			return 0;
	}
	lastTxgain = CalibrateGain[ichain][CalibrateAttempt[ichain]];

	// record measurement for this chain
	CalibratePower[ichain][CalibrateAttempt[ichain]]=power;
    CalibrateAttempt[ichain]++;

	dpower = PowerGoal - CalibratePower[ichain][CalibrateAttempt[ichain]-1];
	if(dpower >= -powerDeviation && dpower <= powerDeviation)
	{
		//
		// measurement is good. set gain to -1 to indicate that we should stop.
		//
		CalibrateGain[ichain][CalibrateAttempt[ichain]]= -1;
		status[ichain] = status_done;
	}
	else
	{
		if(CalibrateAttempt[ichain] >= attempt)
		{
			CalibrateGain[ichain][CalibrateAttempt[ichain]]=-1;
			status[ichain] = status_done;
//			ErrorPrint(CalibrateFail,1<<ichain,
//				CalibrateGain[ichain][CalibrateAttempt[ichain]-1],
//				CalibratePower[ichain][CalibrateAttempt[ichain]-1]); 
		}
		else
		{
			// for Peregrine 2 point calibration
			if (cal.scheme == 2)
			{
			    CalibrateGain[ichain][CalibrateAttempt[ichain]] = CalibrateGainNext2Point(lastTxgain, power);
				
				if ((CalibrateAttempt[ichain] > 0) && 
					 CalibrateGain[ichain][CalibrateAttempt[ichain]] == CalibrateGain[ichain][CalibrateAttempt[ichain]-1])
				{
					status[ichain] = CALNEXT_DoneOne;
				}
				else
				{
					status[ichain] = CALNEXT_TxStart;
				}
			}
			else
			{
				CalibrateGain[ichain][CalibrateAttempt[ichain]] = CalibrateGainNext(ichain);
				status[ichain] = CALNEXT_TxStart;
			}
		
			if(CalibrateGain[ichain][CalibrateAttempt[ichain]]>cal.txgainMax)
			{
				if(CalibrateGain[ichain][CalibrateAttempt[ichain]-1] >= cal.txgainMax)
				{
					CalibrateGain[ichain][CalibrateAttempt[ichain]] = -1;
					status[ichain] = status_done;
					ErrorPrint(CalibrateFail,1<<ichain,
						CalibrateGain[ichain][CalibrateAttempt[ichain]-1],
						CalibratePower[ichain][CalibrateAttempt[ichain]-1]); 
				}
				else
				{
					CalibrateGain[ichain][CalibrateAttempt[ichain]] = cal.txgainMax;
				}
			}
			if(CalibrateGain[ichain][CalibrateAttempt[ichain]] < cal.txgainMin)
			{
				if(CalibrateGain[ichain][CalibrateAttempt[ichain]-1]<=cal.txgainMin)
				{
					CalibrateGain[ichain][CalibrateAttempt[ichain]]= -1;
					status[ichain] = status_done;
					ErrorPrint(CalibrateFail,1<<ichain,
						CalibrateGain[ichain][CalibrateAttempt[ichain]-1],
						CalibratePower[ichain][CalibrateAttempt[ichain]-1]); 
				}
				else
				{
					CalibrateGain[ichain][CalibrateAttempt[ichain]] = cal.txgainMin;
				}
			}
		}
	}

	if ((cal.scheme == 0 || cal.scheme == 1) && 
		status[ichain] == CALNEXT_DoneOne)
	{
		status[ichain] = CALNEXT_Done;
	}
	else if ( cal.scheme == 2)
	{
		// done 1st cal point, update 2nd cal point
		if (status[ichain] == CALNEXT_DoneOne)
		{	
			status_done = CALNEXT_DoneTwo;
		} 
		else if ( status[ichain] == CALNEXT_DoneTwo)
		{
			status[ichain] = CALNEXT_Done;
		}
	}
	return 0;
} 

int calibration_one(double power, int ichain)
{
	//UserPrint("calibration_one power %d, ichain %d\n", power, ichain);

	CalibratePower[ichain][CalibrateAttempt[ichain]] = power;
 	if (status[ichain] == CALNEXT_UNINIT) {
		status[ichain] = CALNEXT_DoneOne;
		testPoint=1;
	} else if (status[ichain] == CALNEXT_DoneOne){
		status[ichain] = CALNEXT_Done;
		testPoint=0;
	}
	return 0;
}
