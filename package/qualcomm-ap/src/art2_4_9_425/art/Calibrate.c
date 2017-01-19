

#include <stdio.h>
#include <stdlib.h>


#include "wlantype.h"
#include "smatch.h"
#include "TimeMillisecond.h"
#include "CommandParse.h"
#include "NewArt.h"
#include "MyDelay.h"
#include "ParameterSelect.h"
#include "Card.h"
#include "Field.h"

#include "Device.h"
#include "LinkTxRx.h"
#include "Calibrate.h"
#include "GainTable.h"
#include "rate_constants.h"
#include "vrate_constants.h"
#include "LinkList.h"

#include "ErrorPrint.h"
#include "NartError.h"


#define MBUFFER 1024

#define MMEASUREMENT 200

#define MCHAIN 3

struct _CalibrateGain
{
    int txgain;
    int gainIndex;
    int dacGain;
};

struct _CalibrateMeasurement
{
    int frequency;
    int txchain;
	struct _CalibrateGain gain[2];
    double power[2];
    int correction;
    int p1;
    int p2;
    int temperature;
    int voltage;
};

static int maxTxgain2G=0;	
static int maxTxgain5G=0;	

extern int temperature_before_reading;

static struct _CalibrateMeasurement CalibrateMeasurement[MMEASUREMENT];

static int CalibrateMeasurementMany=0;

static int Round(double value)
{
	int ivalue;

	ivalue=(int)value;
	if(value-ivalue>=0.5)
	{
		ivalue++;
	}
	if(value-ivalue<=-0.5)
	{
		ivalue--;
	}
	return ivalue;
}

static int MeasurementFind(int frequency, int txchain)
{
    int it;

    for(it=0; it<CalibrateMeasurementMany; it++)
    {
        if(frequency==CalibrateMeasurement[it].frequency &&
            txchain==CalibrateMeasurement[it].txchain)
        {
            return it;
        }
    }
    return -1;
}


static double GainTableOffset=0.0;

void CalibrateStatsHeader(int client)
{
	ErrorPrint(NartDataHeader,"|cal|frequency|txchain|txgain|gainIndex|dacGain|calPoint||power|pcorr|voltage|temp|");
}

static void CalibrateStatsReturn(int client, int frequency, int txchain, int txgain, int gainIndx, int dacGain, double power, int pcorr, int voltage, int temp, int ip)
{
    char buffer[MBUFFER];

    SformatOutput(buffer,MBUFFER-1,"|cal|%d|%d|%d|%d|%d|%d||%.1lf|%d|%d|%d|",
        frequency,txchain,txgain,gainIndx,dacGain,ip,power,pcorr,voltage,temp);
    ErrorPrint(NartData,buffer);
}

int CalibrateInformationRecord(int frequency,
                               int txchain,
                               int txgain,
                               double power,
                               int correction,
                               int p1,
                               int p2,
                               int temperature,
                               int voltage)
{
    int it;
    it=MeasurementFind(frequency,txchain);
    if (it < 0)
    {
        if(CalibrateMeasurementMany<MMEASUREMENT)
        {
            it=CalibrateMeasurementMany;
            CalibrateMeasurementMany++;
        }
    }
    if (it >= 0)
    {
        CalibrateMeasurement[it].frequency = frequency;
        CalibrateMeasurement[it].txchain=txchain;
        CalibrateMeasurement[it].gain[0].txgain=txgain;
        CalibrateMeasurement[it].power[0]=power;
        CalibrateMeasurement[it].correction=correction;
        CalibrateMeasurement[it].temperature = temperature;
        CalibrateMeasurement[it].voltage = voltage;

        return 0;
    }

    return -1;
}
//
// record any information required to support calibration
//
int CalibrateRecord(int client, int frequency, int txchain, int txgain, int gainIndx, int dacGain, double power, int ip)
{
    int it;
    int correction = 0;
	const int defaultTemp = 128;
    int temperature;
    int voltage;
    //
	// use temperature from after the reading as being most accurate
	//
    temperature=DeviceTemperatureGet(1);

	if (temperature <= 0)
	{
		temperature = defaultTemp;
		UserPrint("Error reading temperature!! Using default temp for calibration %d \n", defaultTemp);
	}

    voltage=DeviceVoltageGet();
    it=MeasurementFind(frequency,txchain);
    if(it<0)
    {
        if(CalibrateMeasurementMany<MMEASUREMENT)
        {
            it=CalibrateMeasurementMany;
            CalibrateMeasurementMany++;
        }
    }
    if(it>=0)
    {
        CalibrateMeasurement[it].frequency=frequency;
        CalibrateMeasurement[it].txchain=txchain;
		CalibrateMeasurement[it].gain[ip].txgain=txgain;
		CalibrateMeasurement[it].gain[ip].gainIndex=gainIndx;
		CalibrateMeasurement[it].gain[ip].dacGain=dacGain;
        CalibrateMeasurement[it].power[ip]=power;
        GainTableOffset=DeviceGainTableOffset();
		correction=(int)Round(2.0*(power-GainTableOffset)-txgain+14.0);
        CalibrateMeasurement[it].correction=correction;
        CalibrateMeasurement[it].temperature = temperature;
        CalibrateMeasurement[it].voltage = voltage;
        CalibrateStatsReturn(client, frequency, txchain, txgain, gainIndx, dacGain, power, correction, voltage, temperature, ip);

        return 0;
    }

    return -1;
}

void CalibrateTemperatureSetFromDut()
{
    CalibrateMeasurement[CalibrateMeasurementMany-1].temperature = LinkTxStatTemperatureGet();
}

int txGainCAP_Save()
{
    int ic,it;
    int chmask;
    int txgainmax[2];
	int mode;
    txgainmax[0]=-100;
	txgainmax[1]=-100;
    for(ic=0; ic<MCHAIN; ic++)
    {
        chmask=(1<<ic);
        for(it=0; it<CalibrateMeasurementMany; it++)
        {
            if(CalibrateMeasurement[it].txchain==chmask)
            {
                // NEED TO SORT BY FREQUENCY
				mode=(CalibrateMeasurement[it].frequency>4000);
				if (CalibrateMeasurement[it].gain[0].txgain > txgainmax[mode])
					txgainmax[mode] = CalibrateMeasurement[it].gain[0].txgain;
				if (CalibrateMeasurement[it].gain[1].txgain > txgainmax[mode])
					txgainmax[mode] = CalibrateMeasurement[it].gain[1].txgain;
			}
        }
    }
	for (ic=0;ic<2;ic++) {
		if (txgainmax[ic]>-100) {
			txgainmax[ic] += 20;	// cap txgain to max txgain + 10dB=>20txgain step
		}
	}
	DeviceCalibrationTxgainCAPSet(txgainmax);
   return 0;
}

//
// Save the calibration data in the internal configuration data structure
//
int CalibrateSave(int calPoints)
{
    int ic,it, ip;
    int chmask, chains;
	int altChMask = 0;
    int pier[2];
	int mode;
	int curChain = 0;	
	int	txMask = 0;

	chains = DeviceChainMany();
	txMask = DeviceTxChainMask();

    DeviceCalInfoInit();
    //
    // first we update the calibration data in the internal memory structure
    //
    // do we need to sort it?
    // NEED STUFF FROM FIONA HERE
    for(ic=0; ic<chains; ic++)
    {
		int i=0;
		if (!(txMask & (1<<ic)))
			continue;

		curChain = 0;

		for (i=0; i<ic; i++)
			if (txMask & (1 << ic))
				curChain++;

		chmask = (txMask & (1 << ic));
		//from art2_main
		if (DeviceChainMany() == 1){
			// get tx masks directly
			chmask = DeviceTxChainMask();
			curChain = ic = chmask - 1;
		}
        UserPrint("CalibrateSave chains=%d chmask=0x%x curChain=%d CalibrateMeasurementMany=%d\n", 
			chains, chmask, curChain, CalibrateMeasurementMany);
		pier[0]=0;
		pier[1]=0;
        for(it=0; it<CalibrateMeasurementMany; it++)
        {
            if(CalibrateMeasurement[it].txchain==chmask)
            {
                // NEED TO SORT BY FREQUENCY
				for (ip=0; ip<calPoints;ip++) {
					mode=(CalibrateMeasurement[it].frequency>4000);
					DeviceCalInfoCalibrationPierSet(pier[mode],CalibrateMeasurement[it].frequency, curChain,
						CalibrateMeasurement[it].gain[ip].txgain, CalibrateMeasurement[it].gain[ip].gainIndex, CalibrateMeasurement[it].gain[ip].dacGain,
						CalibrateMeasurement[it].power[ip], CalibrateMeasurement[it].correction,
						CalibrateMeasurement[it].voltage,CalibrateMeasurement[it].temperature, ip);

					DeviceCalibrationPower(CalibrateMeasurement[it].gain[ip].txgain,CalibrateMeasurement[it].power[ip]);
                	DeviceCalibrationPierSet(pier[mode],CalibrateMeasurement[it].frequency,ic,CalibrateMeasurement[it].correction,CalibrateMeasurement[it].voltage,CalibrateMeasurement[it].temperature);
				}
                pier[mode]++;
            }
        }
		if (DeviceIsEmbeddedArt()) {
			int ResetUnusedCalPiers;
			CalibrationGetResetUnusedCalPiers(&ResetUnusedCalPiers);
			if(ResetUnusedCalPiers!=0)
			{
				DeviceCalUnusedPierSet(curChain, 0, pier[0]);
				DeviceCalUnusedPierSet(curChain, 1, pier[1]);
			}
		}
    }
	if (!DeviceIsEmbeddedArt()) {
		txGainCAP_Save();
	}
    return 0;
}

//
// Clear all of the saved calibration data
//
int CalibrateClear()
{
    CalibrateMeasurementMany=0;

    return 0;
}


static int ChainIdentify(unsigned int chmask)
{
    int it;
    int good;

    good= -1;
    for(it=0; it<MCHAIN; it++)
    {
        if((chmask>>it)&0x1)
        {
            if(good>=0)
            {
                return -1;
            }
            good=it;
        }
    }
    return good;
}
