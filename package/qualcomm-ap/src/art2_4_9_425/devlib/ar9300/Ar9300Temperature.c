/*
 *  Copyright ?2005 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#include <stdio.h>

#include "wlantype.h"
//#include "mdata.h"
#include "rate_constants.h"

#include "NewArt.h"
#include "ParameterSelect.h"
#include "Card.h"
#include "smatch.h"
#include "UserPrint.h"
#include "Field.h"
#include "AnwiDriverInterface.h"
#include "TimeMillisecond.h"

#include "Device.h"

#include "mCal9300.h"
#include "mEepStruct9300.h"

#include "Ar9300CalibrationApply.h"
//#include "Ar9300EepromSave.h"
//#include "Ar9300EepromRestore.h"
#include "ar9300_target_pwr.h"
#include "Ar9300Device.h"

#include "Ar9300Temperature.h"

//
// hal header files
//
#include "ah.h"
#include "ah_internal.h"
#include "ar9300.h"
#include "ar9300eep.h"
#include "ar9300reg.h"
#include "ar9300desc.h"

extern struct ath_hal *AH;

static unsigned int VoltageAddress=0;
static int VoltageLow=0;
static int VoltageHigh=0;

static unsigned int TemperatureAddress=0;
static int TemperatureLow=0;
static int TemperatureHigh=0;

static unsigned int AnalogTempAddress=0;
static int AnalogTempLow=0;
static int AnalogTempHigh=0;

static unsigned int TemperatureDoneAddress=0;
static int TemperatureDoneLow=0;
static int TemperatureDoneHigh=0;

static unsigned int TemperatureAdc2Address=0;
static int TemperatureAdc2Low=0;
static int TemperatureAdc2High=0;

static unsigned int TemperatureLocalAddress=0;
static int TemperatureLocalLow=0;
static int TemperatureLocalHigh=0;

static unsigned int TemperatureStartAddress=0;
static int TemperatureStartLow=0;
static int TemperatureStartHigh=0;

static unsigned int TemperatureForceAddress=0;
static int TemperatureForceLow=0;
static int TemperatureForceHigh=0;

static unsigned int savedTime=0;
static unsigned int savedTemp=135;  //rough estimate of osprey temp sensor at room

#define MSTUCK 30
#define NUM_TX_FRAME_READS 50
static int TemperatureStuckCount=0;

static void Ar9300TemperatureFieldLookup()
{
	FieldFind("bb_therm_adc_4.latest_volt_value", &VoltageAddress, &VoltageLow, &VoltageHigh);
	FieldFind("bb_therm_adc_4.latest_therm_value", &TemperatureAddress, &TemperatureLow, &TemperatureHigh);
	FieldFind("ch0_THERM.sar_adc_out", &AnalogTempAddress, &AnalogTempLow, &AnalogTempHigh);
	FieldFind("ch0_therm.sar_adc_done", &TemperatureDoneAddress, &TemperatureDoneLow, &TemperatureDoneHigh);
	FieldFind("bb_therm_adc_2.measure_therm_freq", &TemperatureAdc2Address, &TemperatureAdc2Low, &TemperatureAdc2High);
	FieldFind("ch0_therm.local_therm", &TemperatureLocalAddress, &TemperatureLocalLow, &TemperatureLocalHigh);
	FieldFind("ch0_therm.thermstart", &TemperatureStartAddress, &TemperatureStartLow, &TemperatureStartHigh);
    FieldFind("BB_therm_adc_1.init_therm_setting", &TemperatureForceAddress, &TemperatureForceLow, &TemperatureForceHigh);
//	UserPrint("bb_therm_adc_4.latest_therm_value %x[%d,%d]\n",TemperatureAddress, TemperatureLow, TemperatureHigh);
//	UserPrint("ch0_therm.sar_adc_done %x[%d,%d]\n", TemperatureDoneAddress, TemperatureDoneLow, TemperatureDoneHigh);
//	UserPrint("bb_therm_adc_2.measure_therm_freq %x[%d,%d]\n", TemperatureAdc2Address, TemperatureAdc2Low, TemperatureAdc2High);
//	UserPrint("ch0_therm.local_therm %x[%d,%d]\n", TemperatureLocalAddress, TemperatureLocalLow, TemperatureLocalHigh);
//	UserPrint("ch0_therm.thermstart %x[%d,%d]\n", TemperatureStartAddress, TemperatureStartLow, TemperatureStartHigh);
}


static int TemperatureRestart()
{
	unsigned int save;
	unsigned int reg;
    unsigned int txReg;
    unsigned int readbackReg=0;
    int i;
    int pass = 1;

    //
    //Look for interframe spacing
    //
/*    for(i = 0; i<NUM_TX_FRAME_READS;i++) {  
        MyRegisterRead(0x806c, &txReg);
        if(txReg & 0x200) {
            break;
        }
//        UserPrint("txReg = %d\n", txReg);
    }
*/    //read again to cause delay
 //   MyRegisterRead(0x806c, &txReg);
	//
	// read and remember value
	//
///////	MyRegisterRead(TemperatureAdc2Address,&save);
//	UserPrint("save=%x  ",save);
	//
	// set these two bits
	//
	MyRegisterRead(TemperatureLocalAddress,&reg);
    reg |= (1<<TemperatureLocalLow);
    reg |= (1<<TemperatureStartLow);
//	UserPrint("writing to set bits reg=%x ",reg);
	MyRegisterWrite(TemperatureLocalAddress, reg);

    for (i=0;i<20;i++) {
//        MyDelay(1);
        MyRegisterRead(TemperatureLocalAddress, &readbackReg);
//        UserPrint("in local set: resetReg = %x\n", readbackReg);
        if(readbackReg & ((1<<TemperatureLocalLow) | (1<<TemperatureStartLow)) )
        {
            break;
        }
    }

    if(i == 20) {
        UserPrint("!!!!!!!local_therm and/or thermStartbit not set, continue anyway reg = %x\n", readbackReg);
        pass = 0;
    }

/*
	MyRegisterWrite(TemperatureLocalAddress,(reg | (1<<TemperatureStartLow)));

    for (i=0;i<10;i++) {
//        MyDelay(1);
        MyRegisterRead(TemperatureLocalAddress, &readbackReg);
/////        UserPrint("in start set: resetReg = %x\n", readbackReg);
        if((readbackReg & (1<<TemperatureStartLow)))
        {
            break;
        }
    }

    if(i == 10) {
        UserPrint("!!!!!!!thermstart bit not set, continue anyway reg = %x\n", readbackReg);
        pass = 0;
    }
*/
	//
	// then clear them
	//
	reg &= ~(1<<TemperatureLocalLow);
	reg &= ~(1<<TemperatureStartLow);
	//UserPrint("writing to clear bits reg-> %x ",reg);
	MyRegisterWrite(TemperatureLocalAddress,reg);

    for (i=0;i<10;i++) {
//        MyDelay(1);
        MyRegisterRead(TemperatureLocalAddress, &readbackReg);
////        UserPrint("in local clear: resetReg = %x\n", readbackReg);
        if(((readbackReg >> TemperatureLocalLow) & 0x1)==0 )
        {
            break;
        }
    }
    if(i == 10) {
        UserPrint("!!!!!!!local bits not clear, continue anyway reg = %x\n", readbackReg);
        pass = 0;
    }

	TemperatureStuckCount=0;
////	UserPrint("temperature restart\n");
    return (pass);
}



#define NUM_TEMP_READINGS 30

AR9300DLLSPEC int Ar9300TemperatureGet(int forceTempRead)
{
    unsigned int txReg;
    unsigned int txState;
	unsigned int temperature=0;
    unsigned int done=0;
    unsigned int AnalogTemp;
    unsigned int currentTime;
    unsigned int endTime;
    unsigned int tempReading[NUM_TEMP_READINGS];
    int i;
    int maxIndex = 0;

#ifdef AR9560_EMULATION
	return 20;
#endif

    if(TemperatureAddress==0)
    {
	    Ar9300TemperatureFieldLookup();
    }

    currentTime = TimeMillisecond();
    endTime = savedTime+3000;
    if(forceTempRead || ((endTime>savedTime && (currentTime>endTime || currentTime<savedTime)) || (endTime<savedTime && currentTime>endTime && currentTime<savedTime)))
    {
	/*	if(AR_SREV_OSPREY_10(AH))   // this def has taken out from newmaStaging HAL
		{
			savedTime = currentTime;

			if (TemperatureRestart()) {
				for (i=0; i<NUM_TEMP_READINGS; i++) {
					MyFieldRead(AnalogTempAddress, AnalogTempLow, AnalogTempHigh, &AnalogTemp);
					tempReading [i] = AnalogTemp;

					if(i > 0) {
						if(tempReading[i] > tempReading[maxIndex]) {
							maxIndex = i;
						}
					}
				}
				if(tempReading[maxIndex] >= 0x20) {
					savedTemp = tempReading[maxIndex];
					temperature = tempReading[maxIndex];
				} else {
					temperature = savedTemp;
				}
	   
				MyFieldWrite(TemperatureForceAddress, TemperatureForceLow, TemperatureForceHigh, temperature);
			}
			else {
				temperature = savedTemp;
			}
			UserPrint("ReturnTemp = %x\n", temperature);
		}
		else { *//* ie 2.0 temp comp */
			MyFieldRead(AnalogTempAddress, AnalogTempLow, AnalogTempHigh, &AnalogTemp);
    		MyFieldRead(TemperatureAddress,TemperatureLow, TemperatureHigh, &temperature);

			//UserPrint("\nbbtemp=%x alogTemp = %x\n", temperature,AnalogTemp);
			savedTemp = temperature;
		//}
    }
    else
    {
        temperature = savedTemp;
    }
    return(temperature);
}

int Ar9300TemperatureGetOld(void)
{
	unsigned int temperature=0;
    unsigned int AnalogTemp=0;
	unsigned int stuck=0;
    unsigned int currentTime;
    unsigned int endTime;
    int i;

    currentTime = TimeMillisecond();
    endTime = savedTime+5000;

    if((endTime>savedTime && (currentTime>endTime || currentTime<savedTime)) || (endTime<savedTime && currentTime>endTime && currentTime<savedTime))
    {
	    if(TemperatureAddress==0)
	    {
		    Ar9300TemperatureFieldLookup();
	    }
        for(i = 0; i<MSTUCK; i++) {
            MyDelay(1);
            MyFieldRead(TemperatureAddress,TemperatureLow,TemperatureHigh,&temperature);
	        MyFieldRead(TemperatureDoneAddress,TemperatureDoneLow,TemperatureDoneHigh,&stuck);
            MyFieldRead(AnalogTempAddress, AnalogTempLow, AnalogTempHigh, &AnalogTemp);
            UserPrint("Stuck = %d   ", stuck);
            if (stuck == 1){
               break;
            }
        }
        //UserPrint("\n");
        if((stuck==0)) 
	    {
		    TemperatureRestart();
	    }
	    else
	    {
            savedTemp = temperature;
	    }
    	UserPrint("\nctemp=%d stemp=%d alogTemp = %d, ctime=%d stime=%d stuck=%d\n",
                    temperature,savedTemp,AnalogTemp,currentTime, savedTime, stuck);
        savedTime = currentTime;
    }
    return (int)savedTemp;
}


AR9300DLLSPEC int Ar9300VoltageGet(void)
{
	unsigned int voltage;
	unsigned int stuck;

    return 0;  //This is currently disabled by analog overriding being used by temp comp
               //don't both reading as it will give 0 anyway.

	if(VoltageAddress==0)
	{
		Ar9300TemperatureFieldLookup();
	}
	MyFieldRead(VoltageAddress,VoltageLow,VoltageHigh,&voltage);
	MyFieldRead(TemperatureDoneAddress,TemperatureDoneLow,TemperatureDoneHigh,&stuck);
	if(stuck==0)
	{
		TemperatureStuckCount++;
		if(TemperatureStuckCount>=MSTUCK)
		{
			TemperatureRestart();
		}
	}
	else
	{
		TemperatureStuckCount=0;
	}
//	UserPrint("volt=%d %d\n",voltage,TemperatureStuckCount);
	return (int)voltage;
}
