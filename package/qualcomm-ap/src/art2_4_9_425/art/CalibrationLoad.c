/*
 *  Copyright ?2005 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "smatch.h"
#include "ErrorPrint.h"
#include "DeviceError.h"
#include "DllIfOs.h"
#include "ParameterSelect.h"
#include "CalibrationLoad.h"
#include "ParameterParse.h"
#include "CommandParse.h"
#include "NewArt.h"

#define MBUFFER 1024

static struct _ParameterList calDllLoadParameter={0,{"dll",0,0},"Calibration Dll name",'x',0,0,0,0,0,0,0,0,0};

static char CalibrationLibraryNameFullPath[MBUFFER];
static char CalibrationLibraryName[MBUFFER];
typedef int (*_CalSchemeFunction)(void);
typedef int (*_CalSetIniTxGainFunction)(int*, int);
typedef int (*_CalTxGainResetFunction)(int*, int*, int*);
typedef int (*_CalSetTxGainInitFunction)(int);
typedef int (*_CalSetTxGainMinFunction)(int);
typedef int (*_CalSetTxGainMaxFunction)(int);
typedef int (*_CalSetGainIndexInitFunction)(int);
typedef int (*_CalSetDacGainInitFunction)(int);
typedef int (*_CalSetModeFunction)(int, int, int);
typedef int (*_CalibrationPowerGoalFunction)(double, int);
typedef int (*_CalTxGainFunction)(int*, int*, int*, int*, int);
typedef int (*_CalCalculationFunction)(double, int);
typedef int (*_CalStatusFunction)(int);
typedef int (*_CalChainFunction)(int);
typedef int (*_CalSetCommandFunction)(int);
typedef int (*_CalGetCommandFunction)(int);
typedef int (*_CalParameterSpliceFunction)(struct _ParameterList *);
typedef int (*_CalGetResetUnusedCalPiersFunction)(int *);
typedef int (*_CalGetCalSchemeFunction)(int *);




static int (*_CalScheme)(void);
static int (*_CalSetIniTxGain)(int*, int);
static int (*_CalTxGainReset)(int*, int*, int*);
static int (*_CalSetTxGainInit)(int);
static int (*_CalSetTxGainMin)(int);
static int (*_CalSetTxGainMax)(int);
static int (*_CalSetGainIndexInit)(int);
static int (*_CalSetDacGainInit)(int);
static int (*_CalSetMode)(int, int, int);
static int (*_CalibrationPowerGoal)(double, int);
static int (*_CalTxGain)(int*, int*, int*, int*, int);
static int (*_CalCalculation)(double, int);
static int (*_CalStatus)(int);
static int (*_CalChain)(int);
static int (*_CalSetCommand)(int);
static int (*_CalGetCommand)(int);
static int (*_CalParameterSplice)(struct _ParameterList *list);
static int (*_CalGetResetUnusedCalPiers)(int *);
static int (*_CalGetCalScheme)(int *);



extern int CalibrationScheme(void)
{
    if(_CalScheme!=0)
    {
        return _CalScheme();
    }
    return -1;
}

extern int CalibrationSetIniTxGain(int *totalGain, int maxGainEntry)
{
    if(_CalSetIniTxGain!=0)
    {
        return _CalSetIniTxGain(totalGain, maxGainEntry);
    }
    return -1;
}

extern int CalibrationTxGainReset(int *txGain, int *gainIndex, int *dacGain)
{
    if(_CalTxGainReset!=0)
    {
        return _CalTxGainReset(txGain, gainIndex, dacGain);
    }
    return -1;
}

extern int CalibrationSetTxGainInit(int txGain)
{
    if(_CalSetTxGainInit!=0)
    {
        return _CalSetTxGainInit(txGain);
    }
    return -1;
}

extern int CalibrationSetTxGainMin(int txGainMin)
{
    if(_CalSetTxGainMin!=0)
    {
        return _CalSetTxGainMin(txGainMin);
    }
    return -1;
}

extern int CalibrationSetTxGainMax(int txGainMax)
{
    if(_CalSetTxGainMax!=0)
    {
        return _CalSetTxGainMax(txGainMax);
    }
    return -1;
}

extern int CalibrationSetGainIndexInit(int gainIndex)
{
    if(_CalSetGainIndexInit!=0)
    {
        return _CalSetGainIndexInit(gainIndex);
    }
    return -1;
}

extern int CalibrationSetDacGainInit(int dacGain)
{
    if(_CalSetDacGainInit!=0)
    {
        return _CalSetDacGainInit(dacGain);
    }
    return -1;
}

extern int CalibrationSetMode(int freq, int rate, int chain)
{
    if(_CalSetMode!=0)
    {
        return _CalSetMode(freq, rate, chain);
    }
    return -1;
}

extern int CalibrationSetPowerGoal(double PowerGoal, int iPoint)
{
    if(_CalibrationPowerGoal!=0)
    {
        return _CalibrationPowerGoal(PowerGoal, iPoint);
    }
    return -1;
}

extern int CalibrationGetTxGain(int *txgain, int *gainIndex, int *dacGain, int *calPoint, int ichain)
{
    if(_CalTxGain!=0)
    {
        return _CalTxGain(txgain, gainIndex, dacGain, calPoint, ichain);
    }
    return -1;
}

extern int CalibrationCalculation(double pwr_dBm, int iChain)
{
    if(_CalCalculation!=0)
    {
        return _CalCalculation(pwr_dBm, iChain);
    }
    return -1;
}

extern int CalibrationStatus(int iChain)
{
    if(_CalStatus!=0)
    {
        return _CalStatus(iChain);
    }
    return -1;
}

extern int CalibrationChain(int iChain)
{
    if(_CalChain!=0)
    {
        return _CalChain(iChain);
    }
    return -1;
}

extern int CalibrationGetScheme(int *x)
{
    if(_CalGetCalScheme!=0)
    {
        return _CalGetCalScheme(x);
    }
    return -1;
}

extern int CalibrationGetResetUnusedCalPiers(int *x)
{
    if(_CalGetResetUnusedCalPiers!=0)
    {
        return _CalGetResetUnusedCalPiers(x);
    }
    return -1;
}

//
// Returns a pointer to the specified function.
//
static void *CalibrationFunctionFind(char *prefix, char *function)
{
    void *f;
    char buffer[MBUFFER];

    if(osCheckLibraryHandle(CalibrationLibrary)!=0)
    {
        //
        // try adding the prefix in front of the name
        //
        SformatOutput(buffer,MBUFFER-1,"%s%s",prefix,function);
        buffer[MBUFFER-1]=0;
 
        f=osGetFunctionAddress(buffer,CalibrationLibrary);

        if(f==0)
        {
			//
			// try without the prefix in front of the name
			//

            f=osGetFunctionAddress(function,CalibrationLibrary);

        }

        return f;
    }
	ErrorPrint(CalibrationLoadBad,prefix,function);
	CalibrationUnload();

    return 0;
}


//
// unload the dll
//
void CalibrationUnload()
{
    if(osCheckLibraryHandle(CalibrationLibrary)!=0)
    {
        osDllUnload(CalibrationLibrary);
		CalibrationLibraryNameFullPath[0]=0;
		CalibrationLibraryName[0]=0;
    }
}

int CalibrationLoad(char *dllname)
{
    int error;
	
	//
	// Return immediately if the library is already loaded.
	//
    if(osCheckLibraryHandle(CalibrationLibrary)!=0)
	{
	
		if(Smatch(CalibrationLibraryName,dllname))
		{
			return 0;
		}
		//
		// otherwise, unload previous library
		//
		else
		{
			CalibrationUnload();
		}
	}

	strcpy(CalibrationLibraryName,dllname);
    // 
    // Load DLL file
    //
    error = osDllLoad(dllname,CalibrationLibraryNameFullPath,CalibrationLibrary);

    if(error!=0)
    {
		ErrorPrint(CalibrationNoLoad,dllname);
		return -2;
    }

	_CalScheme=(_CalSchemeFunction)CalibrationFunctionFind(dllname,"Calibration_Scheme");
	if (_CalScheme==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_Scheme");
		return -1;
	}
	
	_CalSetIniTxGain=(_CalSetIniTxGainFunction)CalibrationFunctionFind(dllname,"Calibration_SetIniTxGain");	
	if (_CalSetIniTxGain==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_SetIniTxGain");
		return -1;
	}

	_CalTxGainReset=(_CalTxGainResetFunction)CalibrationFunctionFind(dllname,"Calibration_TxGainReset");
	if (_CalTxGainReset==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_TxGainReset");
		return -1;
	}

	_CalSetTxGainInit=(_CalSetTxGainInitFunction)CalibrationFunctionFind(dllname,"Calibration_SetTxGainInit");
	if (_CalSetTxGainInit==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_SetTxGainInit");	
		return -1;
	}

	_CalSetTxGainMin=(_CalSetTxGainMinFunction)CalibrationFunctionFind(dllname,"Calibration_SetTxGainMin");
	if (_CalSetTxGainMin==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_SetTxGainMin");	
		return -1;
	}

	_CalSetTxGainMax=(_CalSetTxGainMaxFunction)CalibrationFunctionFind(dllname,"Calibration_SetTxGainMax");
	if (_CalSetTxGainMax==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_SetTxGainMax");	
		return -1;
	}

	_CalSetGainIndexInit=(_CalSetGainIndexInitFunction)CalibrationFunctionFind(dllname,"Calibration_SetGainIndexInit");
	if (_CalSetGainIndexInit==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_SetGainIndexInit");	
		return -1;
	}

	_CalSetDacGainInit=(_CalSetDacGainInitFunction)CalibrationFunctionFind(dllname,"Calibration_SetDacGainInit");
	if (_CalSetDacGainInit==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_SetDacGainInit");	
		return -1;
	}

	_CalibrationPowerGoal=(_CalibrationPowerGoalFunction)CalibrationFunctionFind(dllname,"Calibration_SetCalibrationPowerGoal");
	if (_CalibrationPowerGoal==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_SetCalibrationPowerGoal");	
		return -1;
	}

	_CalSetMode=(_CalSetModeFunction)CalibrationFunctionFind(dllname,"Calibration_SetMode");
	if (_CalSetMode==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_SetMode");	
		return -1;
	}

	_CalCalculation=(_CalCalculationFunction)CalibrationFunctionFind(dllname,"Calibration_Calculation");
	if (_CalCalculation==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_Calculation");	
		return -1;
	}

	_CalStatus=(_CalStatusFunction)CalibrationFunctionFind(dllname,"Calibration_Status");
	if (_CalStatus==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_Status");
		return -1;
	}
	
	_CalChain=(_CalChainFunction)CalibrationFunctionFind(dllname,"Calibration_Chain");
	if (_CalChain==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_Chain");
		return -1;
	}

	_CalTxGain=(_CalTxGainFunction)CalibrationFunctionFind(dllname,"Calibration_GetTxGain");
	if (_CalTxGain==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_GetTxGain");
		return -1;
	}

	_CalSetCommand=(_CalSetCommandFunction)CalibrationFunctionFind(dllname,"Calibration_SetCommand");
	if (_CalSetCommand==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_SetCommand");
		return -1;
	}

	_CalGetCommand=(_CalGetCommandFunction)CalibrationFunctionFind(dllname,"Calibration_GetCommand");
	if (_CalGetCommand==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_GetCommand");
		return -1;
	}

	_CalParameterSplice=(_CalParameterSpliceFunction)CalibrationFunctionFind(dllname,"Calibration_ParameterSplice");
	if (_CalParameterSplice==0)
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_ParameterSplice");
		return -1;
	}

	_CalGetResetUnusedCalPiers=(_CalGetResetUnusedCalPiersFunction)CalibrationFunctionFind(dllname,"GetResetUnusedCalPiers");
	if (_CalGetResetUnusedCalPiers==0)
	{
		ErrorPrint(CalibrationNoFunction,"GetResetUnusedCalPiers");
		return -1;
	}	

	_CalGetCalScheme=(_CalGetCalSchemeFunction)CalibrationFunctionFind(dllname,"GetCalScheme");
	if (_CalGetCalScheme==0)
	{
		ErrorPrint(CalibrationNoFunction,"GetCalScheme");
		return -1;
	}


	// Load all initialization parameters
	CalibrationScheme();
    return 0;
}

int CalibrationGetCommand(int client)
{
	if(osCheckLibraryHandle(CalibrationLibrary)==0)
	{
		ErrorPrint(CalibrationMissing);
		return -1;
	}
	if(_CalGetCommand!=0)
    {
        _CalGetCommand(client);
    }
	else
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_GetCommand");
		return -1;
	}
	return 0;


}

int CalibrationSetCommand(int client)
{
	char *name, *calDllName;
	int np, index;

	// Check if the command is dll load first.
	np=CommandParameterMany();
	name=CommandParameterName(0);
	index=ParameterSelectIndex(name,&calDllLoadParameter,1);
    if(index == 0)
	{
		calDllName=CommandParameterValue(0,0);
		if(CalibrationLoad(calDllName)!=0)
		{
			return -1;
		}
		return 1;
	}

	// Following part will handle different cal parameter set commands

	// First check if calibration dll was loaded before.
	if(osCheckLibraryHandle(CalibrationLibrary)==0)
	{
		ErrorPrint(CalibrationMissing);
		return -1;
	}

	if(_CalSetCommand!=0)
    {
        _CalSetCommand(client);
    }
	else
	{
		ErrorPrint(CalibrationNoFunction,"Calibration_SetCommand");
		return -1;
	}
	return 0;
}

int SetCalParameterSplice(struct _ParameterList *list)
{
    if(_CalParameterSplice!=0)
    {
        return _CalParameterSplice(list);
    }
    return -1;

}
