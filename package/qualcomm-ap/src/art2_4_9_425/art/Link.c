

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wlantype.h"
#include "rate_constants.h"
#include "vrate_constants.h"
#include "smatch.h"
#include "UserPrint.h"
#include "ErrorPrint.h"
#include "TimeMillisecond.h"

#include "Device.h"
#include "LinkStat.h"
#include "LinkTxRx.h"


#include "CommandParse.h"
#include "ParameterSelect.h"	
#include "ParameterParse.h"		
#include "NewArt.h"
#include "MyDelay.h"
#include "Card.h"
#include "ResetForce.h"
#include "Link.h"

#include "LinkList.h"

#include "Field.h"
#include "Sticky.h"
#include "RfBbTestPoint.h"

#include "Calibrate.h"
#include "NoiseFloor.h"
#include "Channel.h"

#include "ParseError.h"
#include "CardError.h"
#include "NartError.h"
#include "LinkError.h"

#include "GainTable.h"
#include "calDLL.h"
#include "CalibrationLoad.h"
#include "SleepMode.h"
#include "ChipIdentify.h"

#define MBUFFER 1024

static int _LinkClient;

static int _LinkCalibrateCount=0;

static int CalibratePowerMean=11;
static int CalibrateTxGainMinimum=0;
static int CalibrateTxGainMaximum=120;

static char *calDLL="cal-2p";

//
// control parameters
//
static int _LinkFrequency=2412;
static int _LinkCenterFrequencyUsed=0;
static int _LinkTxChain=0x7;
static int _LinkRxChain=0x7;
static int _LinkHt40=0;
static int _LinkShortGI=0;
static int _LinkStat=3;
static int _LinkIss=0;
static int _LinkAtt=0;
static int _LinkIfs= -1;				// -1 means regular mode, 0 means tx100, >0 means tx99
static int _LinkDeafMode=0;	
static int _LinkCWmin= -1;
static int _LinkCWmax= -1;
static int _LinkPromiscuous=0;
static int _LinkNdump=0;
static int _LinkRetryMax=15;
static int _LinkPacketLength[MRATE];
static int _LinkPacketLengthMany;
static int _LinkPacketMany=100;
static int _LinkDuration= -1;
static int _LinkBc=1;
static int _LinkIr=0;
static int _LinkAgg=0;
static int _LinkReset;					// 1=reset before op, 0=no reset before op, -1=auto

static int _LinkStopCalibration = 0;
static int _CurrentCalPoint = 0;

#ifdef UNUSED
static int _Papd_reset=0;
#endif
static int _LinkCarrier=0;				// if set, output carrier only

#define MPATTERN 100
static int _LinkPatternLength=0;
static unsigned char _LinkPattern[MPATTERN];	

static int _LinkAntenna=0;					// nothing sets this

static int _LinkTpcm=TpcmTargetPower;
static double _LinkTransmitPower[MRATE];		// -1 means use target power, otherwise tx power in dBm, only if _LinkTpcm=TpcmClosed
static int _LinkTransmitPowerMany;
static int _LinkPcdac=30;
static int _LinkPdgain=3;

static int _CalibrationScheme=-1;
static int _LinkTxGainIndex= -1;
static int _LinkTxDacGain= 0;

static int _LinkCalibrate=0;		// 0=none, 1=combined; 2=individual chains
static int _LinkCalibrateChain=0;
static int _LinkCalibrateLast=0;

static int _LinkNoiseFloor;
static int _LinkNoiseFloorControl[MCHAIN],_LinkNoiseFloorExtension[MCHAIN];
static int _LinkRssiCalibrate=0;

static int _LinkRxIqCal=0;

static int _LinkSpectralScan=0;
static int _LinkSpectralScanMany=0;
static double _LinkSpectralScanRssi;
static double _LinkSpectralScanRssiControl[MCHAIN];
static double _LinkSpectralScanRssiExtension[MCHAIN];
static int _LinkNoiseFloorTemperature;

static int _LinkXtalCal=0;
static int _LinkDataCheck=0;
static double _LinkFrequencyActual=0;

static int _LinkFindSpur=0;

static void LinkSpectralScanProcess(int rssi, int *rssic, int *rssie, int nchain, int *spectrum, int nspectrum); 
static int _LinkChipTemperature=0;
static int _NoiseFloorCal[MCHAIN];


static int _LinkRate[MRATE];
static int _LinkRateMany=0;
static int _LinkBandwidth=BW_AUTOMATIC;
// Psat calibration 
static double _LinkCmacPMPower = 0.0;
static int _LinkPsatCal = 0;
static int _LinkCmacPower = 0;
static int _LinkDutyCycle = 0;

// optimizing tx-gain table reads
static int _LinkFrequencyBand = 0;
static int _PrevLinkFrequencyBand = 0;

//int temperature_before_reading=0;


//
// these are the default values
//
static unsigned char bssID[6]={0x50,0x55,0x55,0x55,0x55,0x05};
static unsigned char rxStation[6]={0x10,0x11,0x11,0x11,0x11,0x01};   // DUT
static unsigned char txStation[6]={0x20,0x22,0x22,0x22,0x22,0x02};    // Golden

static unsigned char _LinkBssId[6]     ;
static unsigned char _LinkRxStation[6] ;   // DUT
static unsigned char _LinkTxStation[6];    // Golden
//static unsigned short _LinkAssocId=0;		// i don't know what this is???

static int _LinkRxGain[2];
static unsigned int _LinkCoex;
static unsigned int _LinkSharedRx;
static unsigned int _LinkSwitchTable;
static unsigned int _LinkAntennaPair;
static unsigned int _LinkChainNumber;
static unsigned int _LinkForceTarget;


// for calibration time improvements
static unsigned int ChainsToCalibrate = 0;
static unsigned int StartAlreadyReceived = 0;

    static enum {NO_MODE, TX_MODE, RX_MODE, WAIT_MODE, LOOP_MODE, EXIT_MODE, CARRIER_MODE} LinkMode;




unsigned int GetNumOfChainsInCalCommand(unsigned int x)
{
	unsigned int b;

	for (b = 0; x != 0; x >>= 1)
		if (x & 01)
			b++;

	return b;
}

unsigned int GetCurrentChainForCal(unsigned int chmask, unsigned int curCount)
{
	unsigned int count;

    for (count=0; count < curCount-1; count++)
    {
        chmask &= chmask-1;
    }

    return chmask & ~(chmask-1);	
}


int ithRateName(char *ratename, int i)
{
	if (IS_vRate(i))
		sprintf(ratename, "%s", vRateStrAll[i-numRateCodes]);  
	else
		sprintf(ratename, "%s", rateStrAll[i]);  
	return 0;
}

static int MessageWait(int timeout)
{
	int nread;
	char buffer[MBUFFER];
	int start,end;
	int client;

	start=TimeMillisecond();
//	UserPrint("MessageWait start\n");

	while(1)
	{
		nread=CommandNext(buffer,MBUFFER-1,&client);
	    //
	    // Got data. Process it.
		//
		if(nread>0)
		{
			if(Smatch(buffer,"START"))
			{
				return 0;
			}
			else
			{
				return -1;
			}
		}
		else if(nread<0)
		{
			return -1;
		}
		//
		// dont wait forever
		//
		end=TimeMillisecond();
		if(end-start>timeout)
		{
			UserPrint("%9d message wait timeout\n",end);
			break;
		}
		UserPrint("+");
		MyDelay(1);
	}
	return -1;
}


static int LinkPowerRequest(int client, int txchain, int frequency, int dataRate, double targetPower)
{
	char buffer[MBUFFER];
	char sRate[20] = {0};

	ithRateName(sRate, dataRate);
	SformatOutput(buffer,MBUFFER-1,"cal txch=%x;freq=%d;r=%s;calpoint=%d;tgtpwr=%f;power=?",
			      txchain, frequency, sRate, _CurrentCalPoint, targetPower);
	
	buffer[MBUFFER-1]=0;
	ErrorPrint(NartRequest,buffer);
	
	return 0;
}


static int LinkPowerRequestAll(int client, int isolated, int txchain, int frequency, int dataRate, double targetPower)
{
	int ich;

	if(isolated)
	{
		for(ich=0; ich<MCHAIN; ich++)
		{
			if(txchain&(1<<ich))
			{
                LinkPowerRequest(client,(1<<ich), frequency, dataRate, targetPower);
			}
		}
	}
	else
	{
        LinkPowerRequest(client,txchain, frequency, dataRate, targetPower);
	}
    return 0;
}

static void XtalCalibrateStatsHeader()
{
	ErrorPrint(NartDataHeader,"|xtal|frequency|txchain|txgain|txpower||factual|ppm|cap|");
}
 
static void XtalCalibrateStatsReturn(int frequency, int txchain, int txgain, double txpower, double factual, int ppm, int cap)
{
    char buffer[MBUFFER];

    SformatOutput(buffer,MBUFFER-1,"|xtal|%d|%d|%d|%.1lf||%.6lf|%d|%d|",
        frequency,txchain,txgain,txpower,factual,ppm,cap);
    ErrorPrint(NartData,buffer);
}

static int CarrierFrequencyRequest(int client, int txchain)
{
   	char buffer[MBUFFER];
   
	SformatOutput(buffer,MBUFFER-1,"xtal txchain=%x; frequency=?",txchain);
   	buffer[MBUFFER-1]=0;
   	ErrorPrint(NartRequest,buffer);
   
   	return 0;
}
   
static int CarrierFrequencyRequestAll(int client, int isolated, int txchain)
{
   	int ich;
   
   	if(isolated)
   	{
   		for(ich=0; ich<MCHAIN; ich++)
   		{
   			if(txchain&(1<<ich))
   			{
   				CarrierFrequencyRequest(client,(1<<ich));
   			}
   		}
   	}
   	else
   	{
   		CarrierFrequencyRequest(client,txchain);
   	}
    return 0;
}

static void CmacPowerStatsHeader()
{
    ErrorPrint(NartDataHeader,"|cmac|frequency|txchain|txgain||power|cmac_power|");
}

static void CmacPowerStatsReturn(int frequency, int txchain, int txgain, double power, double cmac_power)
{
    char buffer[MBUFFER];
    SformatOutput(buffer,MBUFFER-1,"|cmac|%d|%d|%d||%.1lf|%.1lf|",
        frequency,txchain,txgain,power,cmac_power);
    ErrorPrint(NartData,buffer);
}

static int CmacPowerRequest(int client, int txchain)
{
    char buffer[MBUFFER];
    SformatOutput(buffer,MBUFFER-1,"cmac txchain=%x; power=?",txchain);
    buffer[MBUFFER-1]=0;
    ErrorPrint(NartRequest,buffer);

    return 0;
}

static int CmacPowerRequestAll(int client, int txchain)
{
    CmacPowerRequest(client,txchain);
    return 0;
}

static void PsatCalibrationStatsHeader()
{
    ErrorPrint(NartDataHeader,"|psat|frequency|txchain|txgain||olpc_delta|thermal|cmac_olpc|cmac_psat|cmac_olpc_pcdac|cmac_psat_pcdac|");
}

static void  PsatCalibrationStatsReturn(int frequency, int txchain, int txgain)
{
    char buffer[MBUFFER];
    int olpc_dalta=0, thermal=0; 
    double cmac_power_olpc=0.0, cmac_power_psat=0.0;
    unsigned int olcp_pcdac=0, psat_pcdac=0;
    
    DevicePsatCalibrationResultGet(frequency, txchain, &olpc_dalta, &thermal, &cmac_power_olpc, &cmac_power_psat, &olcp_pcdac, &psat_pcdac);
    // store calibraton information to global structure
    CalibrateInformationRecord(frequency, txchain, txgain, 0.0/*power*/, olpc_dalta, 0, 0, thermal, 0);

    // show message in cart
    SformatOutput(buffer,MBUFFER-1,"|psat|%d|%d|%d||%d|%d|%.1lf|%.1lf|%d|%d|",
        frequency, txchain, txgain,
        olpc_dalta, thermal, cmac_power_olpc, cmac_power_psat, olcp_pcdac, psat_pcdac);
    ErrorPrint(NartData,buffer);
}


   
static int LinkTxIsOn()
{
    SendOn(_LinkClient);

	//temperature_before_reading=DeviceTemperatureGet(1);
	//UserPrint("temperature_before_reading %d\n",temperature_before_reading);

	//
	// if we're doing calibration, request power readings
	// answers come back in cal messages which are handled by LinkCalibrate()
	//
	if(_LinkCalibrate)
	{
		double tgtpwr = 0;

		DeviceTargetPowerGet(_LinkFrequency, _LinkRate[0], &tgtpwr);

        LinkPowerRequestAll(_LinkClient,_LinkCalibrate,_LinkTxChain, _LinkFrequency, _LinkRate[0], tgtpwr);
	}
	if(_LinkXtalCal)
	{
        CarrierFrequencyRequestAll(_LinkClient,0,_LinkTxChain);
	}
    if(_LinkCmacPower)
    {
        CmacPowerRequestAll(_LinkClient,_LinkTxChain);
    }
	return 0;
}


static int ChainIdentify(A_UINT32 chmask)
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

int LinkCalibrate()
{
    int ip, np;
    double power;
    char *name;
    int lc;
    int ngot;
    int error;
	unsigned int txchain;
	int ichain;
	int txGain, gainIdx, dacGain;
	//
	// parse parameters and values
	//
    error=0;
    lc=0;
	np=CommandParameterMany();
	txchain=_LinkTxChain;
	power=-1000;
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		if(Smatch(name,"power") || Smatch(name,"pow") || Smatch(name,"p"))
		{
			ngot=SformatInput(CommandParameterValue(ip,0)," %lg ",&power);
			if(ngot!=1)
			{	
	            ErrorPrint(ParseBadValue,CommandParameterValue(ip,0),name);
			    error++;
			}
		}
		else if(Smatch(name,"txch") || Smatch(name,"ch") || Smatch(name,"chain") || Smatch(name,"txchain"))
		{
			ngot=SformatInput(CommandParameterValue(ip,0)," %x ",&txchain);
			if(ngot!=1)
			{	
	            ErrorPrint(ParseBadValue,CommandParameterValue(ip,0),name);
			    error++;
			}
		}
        else if(Smatch(name,"last"))
		{
			ngot=SformatInput(CommandParameterValue(ip,0)," %x ",&_LinkCalibrateLast);
				if(ngot!=1)
			{	
	            ErrorPrint(ParseBadValue,CommandParameterValue(ip,0),name);
			    error++;
			}
    }
    }
    //
    // send header
    //
    if(_LinkCalibrateCount==0)
    {
        CalibrateStatsHeader(_LinkClient);
    }
    //
    // go off and get all the other data we need
    //
    if(error==0)
	{
		//
		// one shot calibration
		//
		if(_LinkCalibrate)
		{
			//
			// record measurement for this chain
			//
			ichain=ChainIdentify(txchain);
			CalibrationGetTxGain(&txGain, &gainIdx, &dacGain, &ip, ichain);
			CalibrateRecord(_LinkClient,_LinkFrequency,(1<<ichain),txGain,gainIdx,dacGain,power,ip);
			CalibrationCalculation(power, ichain);
		}
		//
		// check off this chain as being done. if there are no more to do
		// then abort transmission
		//
		_LinkCalibrateChain &= ~(txchain);
		if(_LinkCalibrateChain==0)
		{
			return 1;			// signal to stop transmission
		}
	}

    return 0;
}

int LinkXtalCal()
{
	int ip, np;
	double ff;
	char *name;
	int lc;
	int ngot;
	int error;
	unsigned int txchain;
   	//
   	// parse parameters and values
   	//
    error=0;
    lc=0;
   	np=CommandParameterMany();
   	txchain=_LinkTxChain;
   	ff=0;
   	for(ip=0; ip<np; ip++)
   	{
   		name=CommandParameterName(ip);
   		if(Smatch(name,"frequency"))
   		{
   			ngot=SformatInput(CommandParameterValue(ip,0)," %lg ",&ff);
   			if(ngot!=1)
   			{	
   	            ErrorPrint(ParseBadValue,CommandParameterValue(ip,0),name);
   			    error++;
   			}
   		}
   		else if(Smatch(name,"txch") || Smatch(name,"ch") || Smatch(name,"chain") || Smatch(name,"txchain"))
   		{
   			ngot=SformatInput(CommandParameterValue(ip,0)," %x ",&txchain);
   			if(ngot!=1)
   			{	
   	            ErrorPrint(ParseBadValue,CommandParameterValue(ip,0),name);
   			    error++;
   			}
   		}
    }
    //
    // go off and get all the other data we need
    //
    if(error==0)
   	{
		_LinkFrequencyActual = ff;
   	}
   
    return 1;			// signal to stop transmission
}

static int LinkCmacPower()
{
    int ip, np;
    double power;
    char *name;
    int lc;
    int ngot;
    int error;
    unsigned int txchain;
   	//
   	// parse parameters and values
   	//
    error=0;
    lc=0;
   	np=CommandParameterMany();
   	txchain=_LinkTxChain;
   	for(ip=0; ip<np; ip++)
   	{
   		name=CommandParameterName(ip);
   		if(Smatch(name,"power") || Smatch(name,"pow") || Smatch(name,"p"))
   		{
   			ngot=SformatInput(CommandParameterValue(ip,0)," %lg ",&power);
   			if(ngot!=1)
   			{	
   	            ErrorPrint(ParseBadValue,CommandParameterValue(ip,0),name);
   			    error++;
   			}
   		}
   		else if(Smatch(name,"txch") || Smatch(name,"ch") || Smatch(name,"chain") || Smatch(name,"txchain"))
   		{
   			ngot=SformatInput(CommandParameterValue(ip,0)," %x ",&txchain);
   			if(ngot!=1)
   			{	
   	            ErrorPrint(ParseBadValue,CommandParameterValue(ip,0),name);
   			    error++;
   			}
   		}
    }
    //
    // go off and get all the other data we need
    //
    if(error==0)
   	{
		_LinkCmacPMPower = power;
   	}
   
    return 1;			// signal to stop transmission
}

//
// Called from inside LinkRxComplete.
// Checks for messages from cart.
// Returns 0 to indicate that the operation should continue.
//
static int LinkStopIt()
{
	int nread;
	char buffer[MBUFFER];
	int client;
    char *word;
//	int caldone;

	if(_LinkRxIqCal)
	{
#ifdef DOLATERRXIQCAL
		caldone=CardRxIqCalComplete();
		if(caldone)
#endif
		{
			return 2;		// immediate stop
		}
	}

	nread=CommandNext(buffer,MBUFFER-1,&client);
	//
	// Got data. Process it.
	//
	if(nread>0)
	{
		//
		// Dispatch
		//
		CommandParse(buffer);
		word=CommandWord();

		if(Smatch(word,"STOP"))
		{
			return 1;	// read remaining packets
		}
		else if(Smatch(word,"cal"))
		{
            return LinkCalibrate();
		}
		else if(Smatch(word,"xtal") || Smatch(word,"xtalcal"))
   		{
            return LinkXtalCal();
   		}
        else if(Smatch(word,"cmac"))
        {
            return LinkCmacPower();
        }
		else if(Smatch(word,"stopcal"))
		{
			_LinkStopCalibration = 1;
			return 1;
		}
		else if(Smatch(word,"unload") || 
				Smatch(word,"!unload"))
		{
			// This is abnormal.. some error happened 
			// and still we want to cleanly exit
			_LinkCalibrateCount = 0;
			_LinkCalibrate = 0;
			return CardRemove(client);
		}
 	}
    return 0;
}


static void LinkRateCheck()
{
	int ir, im, ErrorRate, *pCur;
	int rfrequency,rbandwidth;
	char sRate[20];
    int streamMany, tbandwidth;
    int is40MHzCFreq, is80MHzCFreq;

	//
	// if frequency is 5GHz, eliminate any legacy rates 
	//
	if(_LinkFrequency>4000)
	{
		for(ir=0; ir<_LinkRateMany; ir++)
		{
			if(IS_11B_RATE_INDEX(_LinkRate[ir]))
			{
				ithRateName(sRate, _LinkRate[ir]);
				ErrorPrint(LinkRateRemove11B,sRate);
				_LinkRateMany--;
				for(im=ir; im<_LinkRateMany; im++)
				{
					_LinkRate[im]=_LinkRate[im+1];
				}
				ir--;
			}
		}
	}
	//
	// Figure out bandwidth from frequency
	// Note: For Peregrine, don't need to find the bandwidth from the channel table. 
    //       The wlanMode will be sent to UTF based on the rate masks if _LinkBandwidth==BW_AUTOMATIC in LinkQc9K
	if (_LinkBandwidth==BW_AUTOMATIC && !DeviceIs11ACDevice())
	{
		ChannelFind(_LinkFrequency,_LinkFrequency,_LinkBandwidth,&rfrequency,&rbandwidth);
	}
	else 
	{
		rbandwidth=_LinkBandwidth;
	}

	// Eliminate improper rate vs bandwidth
	for (ir=0, ErrorRate=0, pCur = (int *)_LinkRate; ir<_LinkRateMany; ir++, pCur++)
	{
		switch (rbandwidth) {
			case BW_QUARTER:
			case BW_HALF:
			case BW_HT20: // Eliminate both VHT40 and VHT80
				if (IS_HT40_RATE_INDEX(*pCur) || IS_HT40_vRATE_INDEX(*pCur))
				{
					ErrorRate = LinkRateRemoveHt40;
					break;
				}
			case BW_HT40_PLUS: 
			case BW_HT40_MINUS:  // Eliminate VHT 80
				if (IS_HT80_vRATE_INDEX(*pCur))
					ErrorRate = LinkRateRemoveHt80;
				break;
			case BW_VHT80_0:
			case BW_VHT80_1:
			case BW_VHT80_2:
			case BW_VHT80_3: // No need to eliminate any rate
			default:  // Do not alter any setting such as BW_AUTOMATIC
				break;
		}
		if (ErrorRate)
		{
		    ithRateName(sRate, *pCur);
			ErrorPrint(ErrorRate,sRate);
			--_LinkRateMany;
	  		memmove(pCur, (int *)(pCur+1), (_LinkRateMany-ir--)*sizeof(int));  //Shift buffer to left by 1 and adjust index/pointer
            pCur--;
			ErrorRate = 0;
		}
	}

	// CHECK 2 STREAM and 3 STREAM RATES AGAINST CHAIN MASK HERE.
    streamMany = DeviceTxChainMany();
	for (ir=0, ErrorRate=0, pCur = (int *)_LinkRate; ir<_LinkRateMany; ir++, pCur++)
	{
        if (streamMany == 1 && (IS_2STREAM_RATE_INDEX(*pCur) || IS_2STREAM_vRATE_INDEX(*pCur)))
        {
            ErrorRate = LinkRateRemove2StreamRate;
        }
        if ((streamMany == 1 || streamMany == 2) && (IS_3STREAM_RATE_INDEX(*pCur) || IS_3STREAM_vRATE_INDEX(*pCur)))
        {
            ErrorRate = LinkRateRemove3StreamRate;
        }
		if (ErrorRate)
		{
		    ithRateName(sRate, *pCur);
			ErrorPrint(ErrorRate,sRate);
			--_LinkRateMany;
	  		memmove(pCur, (int *)(pCur+1), (_LinkRateMany-ir--)*sizeof(int));  //Shift buffer to left by 1 and adjust index/pointer
            pCur--;
			ErrorRate = 0;
		}
	}
	//
	// if autoconfiguring ht40, see if any ht40 rates are requested
	//
    tbandwidth = BW_AUTOMATIC;
	if(_LinkBandwidth==BW_AUTOMATIC)
	{
		for(ir=0; ir<_LinkRateMany; ir++)
		{
			if(IS_HT20_RATE_INDEX(_LinkRate[ir]) || IS_LEGACY_RATE_INDEX(_LinkRate[ir]) || IS_11B_RATE_INDEX(_LinkRate[ir]) || IS_11G_RATE_INDEX(_LinkRate[ir]))
			{
				tbandwidth = BW_HT20;
			}

			if (IS_HT40_RATE_INDEX(_LinkRate[ir]))
			{
				tbandwidth = BW_HT40_PLUS;
			}

			if (rbandwidth==BW_HT40_PLUS || rbandwidth==BW_HT40_MINUS)
            {
                tbandwidth = BW_HT20;
			    if(IS_HT40_RATE_INDEX(_LinkRate[ir]) || IS_HT40_vRATE_INDEX(_LinkRate[ir]))
			    {
			    	break;
			    }
            }
            else if (rbandwidth==BW_VHT80_0 || rbandwidth==BW_VHT80_1 || rbandwidth==BW_VHT80_2 || rbandwidth==BW_VHT80_3)
            {
                tbandwidth = BW_HT40_PLUS;
			    if(IS_HT80_vRATE_INDEX(_LinkRate[ir]))
			    {
			    	break;
			    }
            }
		}
		if(ir>=_LinkRateMany)
		{
			_LinkBandwidth=tbandwidth;
		}
		else 
		{
			_LinkBandwidth=rbandwidth;
		}
	}
	else 
	{
		_LinkBandwidth=rbandwidth;
	}

    // Check for valid center frequency if applied
    if ((_LinkRateMany > 0) && (_LinkCenterFrequencyUsed) && (_LinkFrequency > 4000) && (DeviceNonCenterFreqAllowedGet() == 0))
    {
        is40MHzCFreq = Is40MHzCenterFrequency(_LinkFrequency); 
        is80MHzCFreq = Is80MHzCenterFrequency(_LinkFrequency); 

	    for (ir=0, ErrorRate=0, pCur = (int *)_LinkRate; ir<_LinkRateMany; ir++, pCur++)
		{
	        if ((IS_HT40_RATE_INDEX(*pCur) || IS_HT40_vRATE_INDEX(*pCur)) && !is40MHzCFreq) 
			{
                ErrorRate = LinkRateRemoveHt40CenterFrequency;
            }
    	    if(IS_HT80_vRATE_INDEX(*pCur) && !is80MHzCFreq)
			{
                ErrorRate = LinkRateRemoveHt80CenterFrequency;
            }
		    if (ErrorRate)
		    {
		        ithRateName(sRate, *pCur);
			    ErrorPrint(ErrorRate,sRate,_LinkFrequency);
			    --_LinkRateMany;
	  		    memmove(pCur, (int *)(pCur+1), (_LinkRateMany-ir--)*sizeof(int));  //Shift buffer to left by 1 and adjust index/pointer
                pCur--;
			    ErrorRate = 0;
		    }
		}
    }
}


static int MacCompare(unsigned char *m1, unsigned char *m2)
{
	int it;

	for(it=0; it<6; it++)
	{
		if(m1[it]!=m2[it])
		{
			return 0;
		}
	}
	return 1;
}


static void MacSave(unsigned char *dest, unsigned char *source)
{
	int it;

	for(it=0; it<6; it++)
	{
		dest[it]=source[it];
	}
}


static double HistogramMean(int *data, int ndata)
{
	int it;
	int count;
	double mean;
	//
	// compute mean
	//
	count=0;
	mean=0;
	for(it=0; it<ndata; it++)
	{
		if(data[it]>0)
		{
			//
			// accumulate data for mean
			//
			count+=data[it];
			mean+=(it*data[it]);
		}
	}
	//
	// finish computing mean value
	//
	if(count>0)
	{
		mean/=count;
	}
	return mean;
}


static int HistogramPrint(char *buffer, int max, int datamin, int *data, int ndata)
{
	int it, is;
	int dmin,dmax;
	int count;
	double mean;
	int lc, nc;
    int ndup;
	//
	// find minimum and maximum nonzero data
	// also compute mean
	//
	dmin= -1;
	dmax= -1;
	count=0;
	mean=0;
	for(it=0; it<ndata; it++)
	{
		if(data[it]>0)
		{
			if(dmin<0)
			{
				dmin=it;
			}
			dmax=it;
			//
			// accumulate data for mean
			//
			count+=data[it];
			mean+=(it*data[it]);
		}
	}
	//
	// finish computing mean value
	//
	if(count>0)
	{
		mean/=count;
	}
	mean+=datamin;
	lc=SformatOutput(buffer,max-1,"%.1lf",mean);
	if(count>0)	//dmin>=0 && dmin!=dmax)
	{
		nc=SformatOutput(&buffer[lc],max-lc-1,":%d",data[dmin]);
		if(nc>0)
		{
			lc+=nc;
		}
	    for(it=dmin+1; it<=dmax; it++)
		{
            //
            // count number of duplicate values
            //
            for(is=it+1; is<=dmax; is++)
            {
                if(data[it]!=data[is])
                {
                    break;
                }
            }
            ndup=is-it;
            if(ndup<=1)
            {
		        nc=SformatOutput(&buffer[lc],max-lc-1,",%d",data[it]);
		        if(nc>0)
			    {
			        lc+=nc;
			    }
            }
            else
            {
		        nc=SformatOutput(&buffer[lc],max-lc-1,",%d*%d",data[it],ndup);
		        if(nc>0)
			    {
			        lc+=nc;
			    }
                it=is-1;
            }
		}
	}
	buffer[max-1]=0;
	return lc;
}


static int HistogramPrintWithBar(char *buffer, int max, int datamin, int *data, int ndata)
{
	int lc, nc;

	lc=HistogramPrint(buffer,max,datamin,data,ndata);
	nc=SformatOutput(&buffer[lc],max-lc-1,"|");
	if(nc>0)
	{
		lc+=nc;
	}
	return lc;
}

static int HistogramMedian(int *data, int ndata)
{
	int it;
   	int count, halfcount;

	count=0;
	for(it=0; it<ndata; it++)
	{
		if(data[it]>0)
		{
			//
			// accumulate data for mean
			//
			count+=data[it];
		}
	}
    halfcount = count/2;
    count = 0;
	for(it=0; it<ndata; it++)
	{
	    count+=data[it];
        if(count > halfcount) 
            break;
    }
	return it;
}

// load calibration DLL, and read calibration setup file
// only run at the first time tx command with cal parameter set up after nart start. 
int Calibration_INIT()
{
	if (Slength(calDLL)>1) {
		CalibrationLoad(calDLL);
	} else {  
		CalibrationLoad("cal-2p");
	} 
	return 0;
}

//
// exported to test.c to complete menu structure
//
static struct _ParameterList LinkParameter[]=
{	
	LINK_FREQUENCY(1),
	LINK_CENTER_FREQUENCY(1),
	LINK_RATE(MRATE), 
	LINK_INTERLEAVE,
	LINK_HT40,
	LINK_PACKET_COUNT(1), 
	LINK_AGGREGATE(1),
	LINK_DURATION,
	LINK_PACKET_LENGTH(MRATE), 
	LINK_TRANSMIT_POWER(MRATE),
	LINK_TXGAIN(1),
	LINK_TXGAIN_INDEX(1),
	LINK_DACGAIN(1),
	LINK_BROADCAST,
	LINK_RETRY,
	LINK_TX99,
	LINK_TX100,
	LINK_CARRIER,
	LINK_CHAIN(1),
	LINK_TX_CHAIN(1), 
	LINK_RX_CHAIN,
	LINK_DUMP,
	LINK_PROMISCUOUS,
	LINK_BSSID,
	LINK_MAC_TX,
	LINK_MAC_RX,
	LINK_ATTENUATION(1),		// #### shouldn't be passed to nart
	LINK_ISS(1),				// #### shouldn't be passed to nart
	LINK_CALIBRATE,
	LINK_CALIBRATE_GOAL,
	LINK_CALIBRATE_TX_GAIN_MINIMUM,
	LINK_CALIBRATE_TX_GAIN_MAXIMUM,
	LINK_NOISE_FLOOR,
	LINK_RSSI_CALIBRATE,
	LINK_RX_IQ_CAL,
	LINK_AVERAGE,
	LINK_RESET,
	LINK_PDGAIN,
	LINK_STATISTIC,
	LINK_GUARD_INTERVAL,
	LINK_INTERFRAME_SPACING,
	LINK_DEAF_MODE,
	LINK_PATTERN(MPATTERN),
	LINK_CHIP_TEMPERATURE,
	LINK_SPECTRAL_SCAN,
	LINK_BANDWIDTH,
	LINK_XTAL_CAL,
	LINK_RX_GAIN,
	LINK_COEX,
    LINK_SHARED_RX,
	LINK_SWITCH_TABLE,
	LINK_ANTENNA_PAIR,
	LINK_CHAIN_NUMBER,
	LINK_DATA_CHECK,
	LINK_PSAT_CAL,	

	LINK_CMAC_POWER,	
	LINK_FIND_SPUR,
	LINK_FORCE_TARGET_POWER,
	LINK_DUTY_CYCLE,
};


void LinkParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(LinkParameter)/sizeof(LinkParameter[0]);
    list->special=LinkParameter;
}


static int ParseInput(int client)
{
 	int ip,np;
	char *name;
	int error;
	int it;
	int ngot;
	int index;
	int code;			
	int pattern[MPATTERN];
	int rlegacy,rht20,rht40,/*rvht20,rvht40,rvht80,*/extra;
	int nvalue;
	int rerror;
	double dpcdac;
	int tx99;
	int tx100;
	int txchainip;
	int rxchainip;
	//int LinkVRate[MRATE];
	//int LinkVRateMany=0;
    int ht40Input = 0;
    int isCalPcdacSet = 0;

    _LinkCalibrateCount=0;
	_LinkChipTemperature=0;
	_LinkNoiseFloor=0;
	_LinkRssiCalibrate=0;
	_LinkPatternLength=0;
	_LinkReset= -1;
	_LinkDuration= -1;
	_LinkPacketLength[0]=1000;
	_LinkPacketLengthMany = 1;
	_LinkPacketMany=100;
	_LinkFrequency= DeviceIs2GHz() ? 2412 : 5180;
    _LinkCenterFrequencyUsed=0;
	_LinkTxChain=0x7;
	_LinkRxChain=0x7;
	_LinkRateMany=1;
	_LinkRate[0]=RATE_INDEX_6;
	_LinkTpcm=TpcmTargetPower;
	_LinkPcdac=30;
	_LinkPdgain=3;
	_LinkTransmitPower[0]= -1;
    _LinkTransmitPowerMany = 1;
	_LinkAtt= -1;
	_LinkIss= -1;
	_LinkBc=1;
	_LinkRetryMax=0;
	_LinkStat=3;	
	_LinkIr=0;
	_LinkHt40=2;
	_LinkAgg=1;
	_LinkIfs= -1;
	_LinkDeafMode=0;
	_LinkCWmin= -1;
	_LinkCWmax= -1;
	_LinkNdump=0;
	_LinkPromiscuous=0;
    _LinkShortGI=0;
	_LinkCalibrate=0;
	_LinkCarrier=0;
	_LinkBandwidth=BW_AUTOMATIC;
	_LinkXtalCal=0;
    _LinkRxGain[0] = LinkRxGainMaximum;    //mb_gain for rfbbtp command
    _LinkRxGain[1] = LinkRxGainMaximum;    //rf_gain for rfbbtp command
    _LinkCoex = 0;
    _LinkSharedRx = 0;
    _LinkSwitchTable = 0;
    _LinkAntennaPair = 1;
    _LinkChainNumber = 0;
	_LinkForceTarget = 0;
	_LinkDutyCycle = 0;

	CalibratePowerMean= -100;
	CalibrateTxGainMinimum=0;
	CalibrateTxGainMaximum=120;
	for(it=0; it<6; it++)
	{
		_LinkBssId[it]=bssID[it];
		_LinkTxStation[it]=txStation[it];
		_LinkRxStation[it]=rxStation[it];
	}
	_LinkRxIqCal=0;
	tx99=0;
	tx100=0;
	_LinkSpectralScan=0;
	_LinkSpectralScanMany=0;
    _LinkPsatCal = 0;
    _LinkCmacPower = 0;
	//
	// prepare beginning of error message in case we need to use it
	//
	error=0;
	//
	// parse parameters and values
	//
	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		index=ParameterSelectIndex(name,LinkParameter,sizeof(LinkParameter)/sizeof(LinkParameter[0]));
		if(index>=0)
		{
			code=LinkParameter[index].code;
			switch(code) 
			{
				case LinkParameterFrequency:
                    if (_LinkCenterFrequencyUsed)
					{
						ErrorPrint(ParseCenterFrequencyUsed,CommandParameterValue(ip,0),_LinkFrequency);
						error++;
					}
					ngot=ParseIntegerList(ip,name,&_LinkFrequency,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					if ((!DeviceIs2GHz() && _LinkFrequency < 4000) || (!DeviceIs5GHz() && _LinkFrequency > 4000))
					{
						ErrorPrint(ParseBadValue,CommandParameterValue(ip,0),name);
						error++;
					}
					break;
				case LinkParameterCenterFrequency:
					ngot=ParseIntegerList(ip,name,&_LinkFrequency,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					if ((!DeviceIs2GHz() && _LinkFrequency < 4000) || (!DeviceIs5GHz() && _LinkFrequency > 4000))
					{
						ErrorPrint(ParseBadValue,CommandParameterValue(ip,0),name);
						error++;
					}
                    else if (ngot > 0)
                    {
                        _LinkCenterFrequencyUsed=1;
                    }
					break;
				case LinkParameterChipTemperature:
					ngot=ParseIntegerList(ip,name,&_LinkChipTemperature,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterTransmitPower:
                    // Multiple tx powers can be entered for multiple rates.
                    // If there is only 1 entry, it will be applied for all rates.
                    // If the first entry is <=-100, target power will be applied.
                    // If there is more than 1 entries, each entry will be corresponding to tx power of each rate.
                    // If number of entries is less than number of rates, the last entry will be applied for the rest of the rates.
                    // If number of entries is more than number of rates, the excess will be ignored.
					_LinkTransmitPowerMany=ParseDoubleList(ip,name,_LinkTransmitPower,&LinkParameter[index]);
					if(_LinkTransmitPowerMany<=0)
					{
						error++;
					}
					else if(_LinkTransmitPower[0]<= -100)
					{
						_LinkTpcm=TpcmTargetPower;
					}
					else if (_LinkTpcm != TpcmForcedTargetPower)
					{
						_LinkTpcm=TpcmTxPower;
					}
					break;
				case LinkParameterPacketLength:
					_LinkPacketLengthMany = ParseIntegerList(ip,name,_LinkPacketLength,&LinkParameter[index]);
					if(_LinkPacketLengthMany<=0)
					{
						error++;
					}
					break;
				case LinkParameterDuration:
					ngot=ParseIntegerList(ip,name,&_LinkDuration,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterNoiseFloor:
					ngot=ParseIntegerList(ip,name,&_LinkNoiseFloor,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterRssiCalibrate:
					ngot=ParseIntegerList(ip,name,&_LinkRssiCalibrate,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterRxIqCal:
					ngot=ParseIntegerList(ip,name,&_LinkRxIqCal,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterSpectralScan:
					ngot=ParseIntegerList(ip,name,&_LinkSpectralScan,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterCalibrate:
					_LinkCalibrate=1;
					_LinkStopCalibration = 0;
					calDLL = CommandParameterValue(ip,0);
					Calibration_INIT();
					CalibrationGetScheme(&_CalibrationScheme);
                    if(isCalPcdacSet)CalibrationSetTxGainInit(_LinkPcdac);
					break;
				case LinkParameterCalibrateGoal:
					ngot=ParseIntegerList(ip,name,&CalibratePowerMean,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterCalibrateTxGainMinimum:
					ngot=ParseIntegerList(ip,name,&CalibrateTxGainMinimum,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					CalibrationSetTxGainMin(CalibrateTxGainMinimum);
					break;
				case LinkParameterCalibrateTxGainMaximum:
					ngot=ParseIntegerList(ip,name,&CalibrateTxGainMaximum,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					CalibrationSetTxGainMax(CalibrateTxGainMaximum);
					break;
				case LinkParameterInputSignalStrength:
					ngot=ParseIntegerList(ip,name,&_LinkIss,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterAttenuation:
					ngot=ParseIntegerList(ip,name,&_LinkAtt,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterPdgain:
					ngot=ParseIntegerList(ip,name,&_LinkPdgain,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterPcdac:
					//
					// check for double precision because that is what litepoint sends, 8/3/2010
					//
					ngot=sscanf(CommandParameterValue(ip,0)," %lg %1c",&dpcdac,&extra);
					if(ngot==1)
					{
						_LinkPcdac=(int)dpcdac;
						_LinkTpcm=TpcmTxGain;
					}
					else
					{
						ngot=ParseIntegerList(ip,name,&_LinkPcdac,&LinkParameter[index]);
						if(ngot<=0)
						{
							error++;
						}
						else
						{	
							_LinkTpcm=TpcmTxGain;
						}
					}
					if (ngot>0) {
						CalibrationSetTxGainInit(_LinkPcdac);
                        isCalPcdacSet = 1;
					}
					break;
				case LinkParameterTxGainIndex:
					ngot=ParseIntegerList(ip,name,&_LinkTxGainIndex,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					else
					{	
						_LinkTpcm=TpcmTxGainIndex;
					}
 
					if (ngot>0) {
						CalibrationSetGainIndexInit(_LinkTxGainIndex);
					}
					break;
				case LinkParameterDACGain:
					ngot=ParseIntegerList(ip,name,&_LinkTxDacGain,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					else
					{	
						_LinkTpcm=TpcmDACGain;
					}

					if (ngot>0) {
						CalibrationSetDacGainInit(_LinkTxDacGain);
					}
					break;
				case LinkParameterForceTargetPower:
					ngot=ParseIntegerList(ip,name,(int *)&_LinkForceTarget,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					else
					{
						_LinkTpcm = TpcmForcedTargetPower;
					}
					break;
				case LinkParameterHt40:
					ngot=ParseIntegerList(ip,name,&_LinkHt40,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					else if (_LinkBandwidth == BW_AUTOMATIC)
					{
						switch(_LinkHt40){
						case 0:
							_LinkBandwidth=BW_HT20;
							break;
						case 1:
							_LinkBandwidth=BW_HT40_PLUS;
							break;
						case -1:
							_LinkBandwidth=BW_HT40_MINUS;
							break;
						case -4:
							_LinkBandwidth=BW_VHT80_0;
							break;
						case -3:
							_LinkBandwidth=BW_VHT80_1;
							break;
						case 3:
							_LinkBandwidth=BW_VHT80_2;
							break;
						case 4:
							_LinkBandwidth=BW_VHT80_3;
							break;
						case 2:
							_LinkBandwidth=BW_AUTOMATIC;
							break;
						default:
							error++;
							break;
						}
					}
                    ht40Input = 1;
					break;
				case LinkParameterBandwidth:
					ngot=ParseIntegerList(ip,name,&_LinkBandwidth,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterInterleaveRates:
					ngot=ParseIntegerList(ip,name,&_LinkIr,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterBroadcast:
					ngot=ParseIntegerList(ip,name,&_LinkBc,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterRetry:
					ngot=ParseIntegerList(ip,name,&_LinkRetryMax,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterAggregate:
					ngot=ParseIntegerList(ip,name,&_LinkAgg,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterGuardInterval:
					ngot=ParseIntegerList(ip,name,&_LinkShortGI,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterChain:
					ngot=ParseIntegerList(ip,name,&_LinkTxChain,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					else
					{
						_LinkRxChain=_LinkTxChain;
						rxchainip = txchainip = ip;
					}
					break;
				case LinkParameterTxChain:
					ngot=ParseIntegerList(ip,name,&_LinkTxChain,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					txchainip = ip;
					break;
				case LinkParameterRxChain:
					ngot=ParseIntegerList(ip,name,&_LinkRxChain,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					rxchainip = ip;
					break;
				case LinkParameterStatistic:
					ngot=ParseIntegerList(ip,name,&_LinkStat,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterReset:
					ngot=ParseIntegerList(ip,name,&_LinkReset,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterPromiscuous:
					ngot=ParseIntegerList(ip,name,&_LinkPromiscuous,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterDump:
					ngot=ParseIntegerList(ip,name,&_LinkNdump,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterDeafMode:
					ngot=ParseIntegerList(ip,name,&_LinkDeafMode,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterInterframeSpacing:
					ngot=ParseIntegerList(ip,name,&_LinkIfs,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterCarrier:
					ngot=ParseIntegerList(ip,name,&_LinkCarrier,&LinkParameter[index]);		
					if(ngot<=0)
					{
						error++;
					}
					else
					{
						if(_LinkCarrier!=0)
						{
							tx100=1;
							tx99=0;
							_LinkIfs=0;
						}
					}
					break;
				case LinkParameterTx99:
					ngot=ParseIntegerList(ip,name,&tx99,&LinkParameter[index]);		
					if(ngot<=0)
					{
						error++;
					}
					else
					{
						if(tx99!=0)
						{
							tx100=0;
							_LinkIfs=1;
						}
						else
						{
							_LinkIfs= -1;
						}
						}
					break;
				case LinkParameterTx100:
					ngot=ParseIntegerList(ip,name,&tx100,&LinkParameter[index]);		
					if(ngot<=0)
					{
						error++;
					}
					else
					{
						if(tx100!=0)
						{
							tx99=0;
							_LinkIfs=0;
						}
						else
						{
							_LinkIfs= -1;
						}
					}
					break;
				case LinkParameterPacketCount:
					ngot=ParseIntegerList(ip,name,&_LinkPacketMany,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
				case LinkParameterPattern:
					_LinkPatternLength=ParseHexList(ip,name,(unsigned int *)pattern,&LinkParameter[index]);
					if(_LinkPatternLength<=0)
					{
						error++;
					}
					else
					{
						for(it=0; it<_LinkPatternLength; it++)
						{
							_LinkPattern[it]=pattern[it];
						}
					}
					break;
				case LinkParameterRate:
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
							_LinkRateMany=RateCount(rlegacy,rht20,rht40,_LinkRate);
						}
					}
					if(rerror!=0)
					{
						_LinkRateMany=ParseIntegerList(ip,name,_LinkRate,&LinkParameter[index]);
						if(_LinkRateMany<=0)
						{
							error++;
						}
						else
						{
							_LinkRateMany=RateExpand(_LinkRate,_LinkRateMany);
							_LinkRateMany=vRateExpand(_LinkRate,_LinkRateMany);
						}
					}
					break;
				case LinkParameterBssId:
					if(ParseMacAddress(CommandParameterValue(ip,0),_LinkBssId))
					{
						ErrorPrint(ParseBadValue,CommandParameterValue(ip,0),name);
						error++;
					}
					break;
				case LinkParameterMacTx:
					if(ParseMacAddress(CommandParameterValue(ip,0),_LinkTxStation))
					{
						ErrorPrint(ParseBadValue,CommandParameterValue(ip,0),name);
						error++;
					}
					break;
				case LinkParameterMacRx:
					if(ParseMacAddress(CommandParameterValue(ip,0),_LinkRxStation))
					{
						ErrorPrint(ParseBadValue,CommandParameterValue(ip,0),name);
						error++;
					}
					break;
  				case LinkParameterXtalCal:
   					ngot=ParseIntegerList(ip,name,&_LinkXtalCal,&LinkParameter[index]);
   					if(ngot<=0)
   					{
   						error++;
   					}
					else
					{
						if(_LinkXtalCal)
						{
							_LinkCarrier=1;
						}
					}
   					break;
                case LinkParameterRxGain:
					ngot=ParseIntegerList(ip,name,_LinkRxGain,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
					break;
                case LinkParameterCoexMode:
					ngot=ParseIntegerList(ip,name,(int *)&_LinkCoex,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
                    break;
                case LinkParameterSharedRx:
					ngot=ParseIntegerList(ip,name,(int *)&_LinkSharedRx,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
                    break;
                case LinkParameterSwitchTable:
					ngot=ParseIntegerList(ip,name,(int *)&_LinkSwitchTable,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
                    break;
                case LinkParameterAntennaPair:
					ngot=ParseIntegerList(ip,name,(int *)&_LinkAntennaPair,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
                    break;
                case LinkParameterChainNumber:
					ngot=ParseIntegerList(ip,name,(int *)&_LinkChainNumber,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
                    break;
  				case LinkParameterDataCheck:
   					ngot=ParseIntegerList(ip,name,&_LinkDataCheck,&LinkParameter[index]);
   					if(ngot<=0)
   					{
   						error++;
   					}
   					break;
				case LinkParameterFindSpur:
					ngot=ParseIntegerList(ip,name,&_LinkFindSpur,&LinkParameter[index]);
   					if(ngot<=0)
   					{
   						error++;
   					}
   					break;
                case LinkParameterPsatCal:
					ngot=ParseIntegerList(ip,name,&_LinkPsatCal,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
                    break;
                case LinkParameterCmacPower:
					ngot=ParseIntegerList(ip,name,&_LinkCmacPower,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
                    break;
				case LinkParameterDutyCycle:
					ngot=ParseIntegerList(ip,name,&_LinkDutyCycle,&LinkParameter[index]);
					if(ngot<=0)
					{
						error++;
					}
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

	if(error!=0)
	{
		return error;
	}

	//
	// if there's no card loaded, return error
	//
	if(CardCheckAndLoad(client)!=0)
	{
		ErrorPrint(CardNoneLoaded);
		return -CardNoneLoaded;
	}

	//
	//  restrict chain mask to maximum supported by chip
	//
	_LinkRxChain&=DeviceRxChainMask();
	_LinkTxChain&=DeviceTxChainMask();
	if (_LinkRxChain == 0)
	{
		ErrorPrint(ParseBadValue, CommandParameterValue(rxchainip, 0),"rxchain or chain");
		return -1;
	}
	if (_LinkTxChain == 0)
	{
		ErrorPrint(ParseBadValue, CommandParameterValue(txchainip, 0),"txchain or chain");
		return -1;
	}

	LinkRateCheck();
	if(_LinkRateMany<=0 && !_LinkCarrier)
	{
		return -1;
	}
    //TRANG - workaround for old IqFact+ when "ht40" or "bw" is not specified for VHT40 rate
    if ((ht40Input == 0) && (_LinkRateMany == 1) && IS_HT40_vRATE_INDEX(_LinkRate[0]) && (_LinkCenterFrequencyUsed == 0))
    {
        _LinkFrequency -= 20;
    }

#ifdef UNUSED
	if(_Papd_reset)
	{
		_LinkReset = 1;
		_Papd_reset = 0;
	}
#endif

    //Adjust _LinkTransmitPower and _LinkPacketLength arrays according to _LinkRateMany
    if (_LinkTpcm == TpcmTxPower)
    {
        for (ip = 0; ip < _LinkRateMany; ++ip)
        {
            if (ip >= _LinkTransmitPowerMany)
            {
                _LinkTransmitPower[ip] = _LinkTransmitPower[_LinkTransmitPowerMany-1];
            }
            if (ip >= _LinkPacketLengthMany)
            {
                _LinkPacketLength[ip] = _LinkPacketLength[_LinkPacketLengthMany-1];
            }
        }
    }
    for (ip = 0; ip < _LinkRateMany; ++ip)
    {
        if (ip >= _LinkPacketLengthMany)
        {
            _LinkPacketLength[ip] = _LinkPacketLength[_LinkPacketLengthMany-1];
        }
    }

	return 0;
}

//
// code for transmit side of link test
//

static void LinkTransmitStatsHeader()
{
	ErrorPrint(NartDataHeader,"|tx|frequency|tp|txchain|iss|att|pdgain|txgain|rate|pl|pc|agg||correct|throughput|error|fifo|excess|retry|dretry|rssi|rssi00|rssi01|rssi02|rssi10|rssi11|rssi12|txgi|dacg|byte|duration|temp|volt|");
}


static void LinkTransmitStatsReturn(int channel, 
	double pout, int txchain,
    int iss, int att, int pdgain, int pcdac, 
	char *rate, int plength, int expected, int agg,
	int correct, 
	int tput, 
	int other, int under, 
	int excess, 
	int *retry, int mretry,
	int *dretry, int mdretry,
	int *rssi, int mrssi,
	int *rssi00, int mrssi00,
	int *rssi01, int mrssi01,
	int *rssi02, int mrssi02,
    int *rssi10, int mrssi10,
    int *rssi11, int mrssi11,
    int *rssi12, int mrssi12,
	int txgi, int dacg,
    int byte, int duration,
	int temp, int volt,
	int rssimin)
{
	char buffer[MBUFFER];
	int lc, nc;
    //
	// format message
	//
	lc=SformatOutput(buffer,MBUFFER-1,"|tx|%d|%.1f|%d|%d|%d|%d|%d|%s|%d|%d|%d||%d|%d|%d|%d|%d|",
        channel,
		pout,
        txchain,
		iss,
		att,
		pdgain,
		pcdac,
        rate,
		plength,
        expected,
		agg,
        correct,
        tput,
		other,
		under,
		excess);
	//
	// add retry histogram
	//
    nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,0,retry,mretry);
	if(nc>0)
	{
		lc+=nc;
	}
	//
	// add dretry histogram
	//
    nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,0,dretry,mdretry);
	if(nc>0)
	{
		lc+=nc;
	}
	//
	// add rssi histogram
	//
    nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,rssimin,rssi,mrssi);
	if(nc>0)
	{
		lc+=nc;
	}
	//
	// add rssi histogram
	//
    nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,rssimin,rssi00,mrssi00);
	if(nc>0)
	{
		lc+=nc;
	}
	//
	// add rssi histogram
	//
    nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,rssimin,rssi01,mrssi01);
	if(nc>0)
	{
		lc+=nc;
	}
	//
	// add rssi histogram
	//
    nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,rssimin,rssi02,mrssi02);
	if(nc>0)
	{
		lc+=nc;
	}
	//
	// add rssi histogram
	//
    nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,rssimin,rssi10,mrssi10);
	if(nc>0)
	{
		lc+=nc;
	}
	//
	// add rssi histogram
	//
    nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,rssimin,rssi11,mrssi11);
	if(nc>0)
	{
		lc+=nc;
	}
	//
	// add rssi histogram
	//
    nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,rssimin,rssi12,mrssi12);
	if(nc>0)
	{
		lc+=nc;
	}
    nc=SformatOutput(&buffer[lc],MBUFFER-lc,"%d|%d|",txgi,dacg);
	if(nc>0)
	{
		lc+=nc;
	}
    nc=SformatOutput(&buffer[lc],MBUFFER-lc,"%d|%d|",byte,duration);
	if(nc>0)
	{
		lc+=nc;
	}
    nc=SformatOutput(&buffer[lc],MBUFFER-lc,"%d|%d|",temp,volt);
	if(nc>0)
	{
		lc+=nc;
	}
	//
	// send it
	//
	ErrorPrint(NartData,buffer);
}


static void LinkTransmitStatPrint()
{
    int j;
    struct txStats *tStats;
    double tp;
	int temp, volt;
 	int nf[MCHAIN],nfc[MCHAIN],nfe[MCHAIN],nfcal[MCHAIN],nfpower[MCHAIN],nfTemperature[MCHAIN];
 	int it;
	int txMask;
	char sRate[20];

    if (!DeviceIsEmbeddedArt())
    {
        DeviceNoiseFloorFetch(nfc,nfe,MCHAIN);
		txMask = DeviceTxChainMask();
 	    for(it=0; it < MCHAIN; it++)
 		{
			if (txMask & (1 << it))
			{
				nf[it]=(nfc[it]+nfe[it])/2;
				nfcal[it]=DeviceNoiseFloorGet(_LinkFrequency, it);
				nfpower[it]=DeviceNoiseFloorPowerGet(_LinkFrequency, it);
				nfTemperature[it] = DeviceNoiseFloorTemperatureGet(_LinkFrequency, it);
				UserPrint("chain: %4d nfc: %5d nfcal: %5d nfpower: %5d nfTemp: %5d\n",it,nfc[it],nfcal[it],nfpower[it],nfTemperature[it]);
			}
 		}
    }

   	if(_LinkStat&2)
   	{      

        if (!DeviceIsEmbeddedArt())
        {
		temp=DeviceTemperatureGet(1);
		UserPrint("LinkTransmitStatPrint(): temp %d _LinkChipTemperature %d\n", temp, _LinkChipTemperature);
        }

	volt=DeviceVoltageGet();
    	LinkTransmitStatsHeader();
   		//
   		// these stats aren't accurate
   		// it looks like the library does not compute and store startTime or endTime
   		// so we made up our own. not accurate if more than 1 rate used
   		//
        for (j = 0; j < _LinkRateMany; j++) 
   		{
            tStats=LinkTxStatFetch(_LinkRate[j]);
            if (DeviceIsEmbeddedArt())
            {
                temp = tStats->temperature;
            }
            if(_LinkTpcm==TpcmTargetPower)
            {
                DeviceTargetPowerGet(_LinkFrequency,_LinkRate[j],&tp);
            }
            else
            {
                tp=_LinkTransmitPower[j];
            }
			ithRateName(sRate, _LinkRate[j]);
            LinkTransmitStatsReturn(_LinkFrequency,
   				tp,_LinkTxChain,
                _LinkIss,_LinkAtt,_LinkPdgain,_LinkPcdac,
				sRate,		//rateStrAll[_LinkRate[j]],
				_LinkPacketLength[j],_LinkPacketMany,_LinkAgg,
				tStats->goodPackets,tStats->newThroughput,
				tStats->otherError,tStats->underruns,tStats->excessiveRetries,
				tStats->shortRetry, MRETRY,
				tStats->longRetry, MRETRY,
				tStats->rssi, MRSSI,
				tStats->rssic[0], MRSSI,
				tStats->rssic[1], MRSSI,
				tStats->rssic[2], MRSSI,
				tStats->rssie[0], MRSSI,
				tStats->rssie[1], MRSSI,
				tStats->rssie[2], MRSSI,
				_LinkTxGainIndex,_LinkTxDacGain,
                tStats->byteCount,tStats->endTime-tStats->startTime,
				temp,volt,
				tStats->rssimin);
		}
#ifdef DOALL
        tStats=LinkTxStatTotalFetch();
        LinkTransmitStatsReturn(_LinkFrequency,
			_LinkTransmitPower[0],_LinkTxChain,
            _LinkIss,_LinkAtt,_LinkPdgain,_LinkPcdac,
			"ALL",
			_LinkPacketLength[0],_LinkPacketMany,_LinkAgg,
			tStats->goodPackets,tStats->newThroughput,
			tStats->otherError,tStats->underruns,tStats->excessiveRetries,
			tStats->shortRetry, MRETRY,
			tStats->longRetry, MRETRY,
			tStats->rssi, MRSSI,
			tStats->rssic[0], MRSSI,
			tStats->rssic[1], MRSSI,
			tStats->rssic[2], MRSSI,
			tStats->rssie[0], MRSSI,
			tStats->rssie[1], MRSSI,
			tStats->rssie[2], MRSSI,
			_LinkTxGainIndex,_LinkTxDacGain,
            tStats->byteCount,tStats->endTime-tStats->startTime);
#endif
	}
}

int LinkTxCarrierCompleteOldChip(int timeout, int (*ison)(), int (*done)(), int chipTemperature)
{
	int it=0;
    unsigned int startTime, endTime, ctime, failTime;
	int failRetry;
    int pass;
    int batch, lbatch;
	int temperature;
	int doTemp;

//    LinkTxStatClear();
    lbatch=0;
    pass=0;
	doTemp=0;
    //
	// Loop timeout condition.
	// This number can be large since it is only used for
	// catastrophic failure of cart. The normal terminating
	// condition is a message from cart saying STOP.
	//
    startTime=TimeMillisecond();
	ctime=startTime;
//	if(timeout<=0)
//	{
//		timeout=60*60*1000;         // one hour
//	}
	endTime=startTime+timeout; 
	failTime=endTime;
	failRetry=0;

	if(ison!=0)
	{
		(*ison)();
	}

	while (1)
	{
		//
		// check temperature
		//
		if(doTemp>10)
		{
			temperature=DeviceTemperatureGet(0);
			//
			// are we supposed to terminate on reaching a certain chip temperature?
			//
			if(chipTemperature>0)
			{
				if(temperature<=chipTemperature)
				{
					UserPrint(" %d Temperature %d <= %d",it,temperature,chipTemperature);
					break;
				}
			}
			doTemp=0;
		}
		doTemp++;
		//
		// check for message from cart telling us to stop
		// this is the normal terminating condition
		//
		pass++;
		if(pass>100)
		{
			if(done!=0)
			{
				if((*done)())
				{
					UserPrint(" %d Stop",it);
    				break;
				}
			}
			pass=0;
		}
    } 

	UserPrint(" %d Done\n",it);
    return 0;
}


static void LinkTransmitDoIt()
{
    LinkTxStart();

   	if(_LinkCarrier&&(DeviceIdGet()==0x2e))
   		LinkTxCarrierCompleteOldChip(_LinkDuration, LinkTxIsOn, LinkStopIt, _LinkChipTemperature);	
	else if (DeviceIdGet()==0x2e)
	{
		if((_LinkTpcm == 2)||(_LinkTpcm == 1))
		{
			DeviceCalibrationSetting();
		}		
        LinkTxComplete(_LinkDuration, LinkTxIsOn, LinkStopIt, _LinkChipTemperature, _LinkCalibrate);	
	}
	else{
            /* If doing Psat calibration, don't wait for Tx complete */
            if (!_LinkPsatCal)
            LinkTxComplete(_LinkDuration, LinkTxIsOn, LinkStopIt, _LinkChipTemperature, _LinkCalibrate);	
	}

	SendOff(_LinkClient);
}


static void LinkTransmitDoItDut(int caps)
{
    LinkTxDataStartCmd(_LinkFrequency, _LinkCenterFrequencyUsed, _LinkTpcm, _LinkPcdac, _LinkTransmitPower, _LinkTxGainIndex, _LinkTxDacGain,
                               _LinkHt40, _LinkRate, _LinkRateMany, _LinkIr,
                               _LinkBssId, _LinkTxStation, _LinkRxStation,
                               _LinkPacketMany, _LinkPacketLength,
                               _LinkRetryMax, _LinkAntenna, _LinkBc, _LinkIfs,
	                           _LinkShortGI, _LinkTxChain, _LinkAgg,
	                           _LinkPattern, _LinkPatternLength, _LinkCarrier, _LinkBandwidth,
                               _LinkPsatCal,
							   _LinkDutyCycle);
	if(_LinkXtalCal)
	{
		DeviceTuningCapsSet(caps);
	}
	
   /* If doing Psat calibration, don't wait for Tx complete */
    if (!_LinkPsatCal)
    {
	//UserPrint("LinkTxDataStartCmd( ) _LinkTpcm %d, _LinkPcdac %d, _LinkTransmitPower %d, LinkTxGainIndex %d, _LinkTxDacGain \n",
    //           _LinkTpcm, _LinkPcdac, _LinkTransmitPower, _LinkTxGainIndex, _LinkTxDacGain);

		// if we are doing continuous transmission..
		if (_LinkPacketMany <= 0)
			_LinkDuration = -1;

        LinkTxComplete(_LinkDuration, LinkTxIsOn, LinkStopIt, _LinkChipTemperature, _LinkCalibrate);	
     }

     SendOff(_LinkClient);
     SleepModeWakeupSet();
}

static int doReset()
{
	int error;

	error=CardResetIfNeeded(_LinkFrequency,_LinkTxChain,_LinkRxChain,_LinkReset,_LinkBandwidth);
	if(error!=0)
	{
		return error;
	}

	return 0;
}

static int LinkTransmitFirstTime()
{
	int error;
	unsigned int devid;
	//
	// should we do a reset?
	// yes, if the user explicitly asked for it (_LinkReset>0) or if
	// the user asked for auto mode (_LinkReset<0) and any important parameters have changed
	//
/*    error=doReset();
	if(error!=0)
	{
		return error;
	}
*/
    //
    // set power control mode
    //
    _LinkTxGainIndex= -1;


 		UserPrint("_LinkTpcm %d\n",_LinkTpcm);
	if(_LinkTpcm==TpcmTxGain)
	{
		_LinkTxGainIndex=DeviceTransmitGainSet(_LinkFrequency>=4000,_LinkPcdac);

		devid=DeviceIdGet();
		if(devid==0x2e)
			FieldRead("bb_forced_dac_gain", (unsigned int *)&_LinkTxDacGain);
		else
		FieldRead("forced_dac_gain", (unsigned int *)&_LinkTxDacGain);	
//		TpcmLast=TpcmTxGain;
		UserPrint("TxGainIndex=%d TxDacGain=%d\n",_LinkTxGainIndex,_LinkTxDacGain);
	}
	else if(_LinkTpcm==TpcmTxGainIndex)
	{
 		UserPrint("Direclty applying gain index\n");
		_LinkTxGainIndex=DeviceTransmitGainSet(TpcmTxGainIndex,_LinkTxGainIndex);
		FieldRead("forced_dac_gain", (unsigned int *)&_LinkTxDacGain);	
	}
	else if(_LinkTpcm==TpcmDACGain)
	{
 		UserPrint("Direclty applying gain index and fixed dacGain\n");
		_LinkTxGainIndex=DeviceTransmitGainSet(TpcmDACGain,_LinkPcdac);
		UserPrint("TxGainIndex=%d TxDacGain=%d PCdac=%d\n",_LinkTxGainIndex,_LinkTxDacGain,_LinkPcdac);
	}
	else if(_LinkTpcm==TpcmTxPower)
	{
		DeviceTransmitPowerSet(_LinkFrequency>=4000,_LinkTransmitPower[0]);
		UserPrint("tx power=%lg\n",_LinkTransmitPower[0]);
	}
    else
    {
        DeviceTargetPowerApply(_LinkFrequency);
 		UserPrint("TARGET POWER\n");
   }
	//
	// get all of the packets descriptors ready to run
	//
    LinkTxSetup(_LinkRate, _LinkRateMany, _LinkIr,
		_LinkBssId, _LinkTxStation, _LinkRxStation, 
		_LinkPacketMany, _LinkPacketLength[0], _LinkRetryMax, _LinkAntenna, _LinkBc, _LinkIfs,
		_LinkShortGI, _LinkTxChain, _LinkAgg,
		_LinkPattern, _LinkPatternLength);

	return 0;
}

// This array contains the rates used to get the target power from in case calibration rate is legacy
static int CalTargetPowerRate[] = {RATE_INDEX_6, RATE_INDEX_54, RATE_INDEX_HT20_MCS7, RATE_INDEX_HT20_MCS15};

static void TargetPowerGoal()
{
	int ir;
	double tp,tptotal;
	int count;
	int nchain;
	int maxrate;
	int CalTargetPowerRateSize;

	tptotal=0;
	count=0;
	CalTargetPowerRateSize = sizeof(CalTargetPowerRate)/sizeof(int);
	//
	// default target power goal is average over ht20 mcs rates
	//
	nchain=DeviceTxChainMany();
	if(nchain==1)
	{
		maxrate=RATE_INDEX_HT20_MCS7;
		CalTargetPowerRate[CalTargetPowerRateSize-1] = RATE_INDEX_HT20_MCS7;
	}
	else if(nchain==2)
	{
		maxrate=RATE_INDEX_HT20_MCS15;
		CalTargetPowerRate[CalTargetPowerRateSize-1] = RATE_INDEX_HT20_MCS15;
	}
	else
	{
		maxrate=RATE_INDEX_HT20_MCS23;
		CalTargetPowerRate[CalTargetPowerRateSize-1] = RATE_INDEX_HT20_MCS23;
	}

	// if cal rate is legacy rate, average over taget powers of 6M, 54M, HT20 MCS7 and maxrate
	if (IS_LEGACY_RATE_INDEX(_LinkRate[0]))
	{
		for(ir=0; ir < CalTargetPowerRateSize; ir++)
		{
			if(DeviceTargetPowerGet(_LinkFrequency,CalTargetPowerRate[ir],&tp)==0)
			{
				tptotal+=tp;
				count++;
			}
		}
	}
	else //average over ht20 mcs rates
	{
		for(ir=RATE_INDEX_HT20_MCS0; ir<=maxrate; ir++)
		{
			if(DeviceTargetPowerGet(_LinkFrequency,ir,&tp)==0)
			{
				tptotal+=tp;
				count++;
			}
		}
	}
	if(count>0)
	{
		CalibratePowerMean=(int)(0.5+(tptotal/((double)count)));
	}
	else
	{
		CalibratePowerMean=(int)10.0;
	}
}

void LinkTransmitHost(int client)
{
	int pap;
	int ich;
 	int caps=0x40;
	int step;
 	int ppm;
	double cmac_power = 0.0;
	int calPoint;
	//
	// parse the input and if it is good
	// get ready to do the receive
	//
	if(ParseInput(client)==0)
	{
		pap=0;
		//
		// if there's no card loaded, return error
		//
		if(CardCheckAndLoad(client)!=0)
		{
			ErrorPrint(CardNoneLoaded);
		}
		else
		{
            _LinkCalibrateLast = 0;
			// Save client handle so we can send messages back to the right client
			//
			_LinkClient=client;
			if(_LinkCalibrate!=0) {
				if(CalibratePowerMean<=-100)
				{
					TargetPowerGoal();
				}
				_LinkCalibrateChain=_LinkTxChain;

				if (CalibrationSetMode(_LinkFrequency, _LinkRate[0], _LinkCalibrateChain)<0)
					return;
				CalibrationSetPowerGoal(CalibratePowerMean, 0);

				pap=DevicePaPredistortionGet();
				DevicePaPredistortionSet(0);
			}
			ResetForce();
			doReset();           
			//
			// Acknowledge command from client
			//
			SendOk(client);

			if(_LinkCalibrate!=0) {
				queryTxGainTable();
				CalibrationTxGainReset(&_LinkPcdac, &_LinkTxGainIndex, &_LinkTxDacGain);
			}	
			if (_LinkCarrier!=0)
			{
				pap=DevicePaPredistortionGet();
				DevicePaPredistortionSet(0);
				ResetForce();
			}
			if(_LinkCmacPower)
			{
				CmacPowerStatsHeader();
			}
 			if(_LinkXtalCal!=0)
 			{
 				caps=0x40;
 				step=0x20;
 				XtalCalibrateStatsHeader();
 			}
 			if(_LinkPsatCal)
			{
				PsatCalibrationStatsHeader();
			}
			LinkMode = TX_MODE;

			//
			// Setup the descriptors for receive
			//
			if(LinkTransmitFirstTime()==0)
			{
				// Acknowledge command from client
				//
//				SendOk(client);
				//
				// wait for "START" message
				//
				if(MessageWait(60000)==0)
				{
					//
					// this is where we really do it.
					// turn on the receiver and get the packets
					//
CalibrateAgain:
					if(_LinkCarrier)
					{
						DeviceTransmitCarrier(_LinkTxChain);
					}
  					if(_LinkXtalCal)
   					{
					  UserPrint("caps=%d\n", caps);
					  FieldWrite("top_wlan_XTALWLAN.xtal_capindac", caps);
   					}
  					
  					if(_LinkPsatCal)
					{
					    DevicePsatCalibration(_LinkFrequency, _LinkTxChain, _LinkRxChain, _LinkHt40);
					}
					
					LinkTransmitDoIt();	

					if(_LinkCarrier)
					{
						DeviceTransmitRegularData();
					}
					//
					// need to reset to turn off tx100 mode
					//
					if(_LinkIfs==0)
					{
						_LinkReset=1;
						doReset();
					}
					else
					{
						//
						// send statistics back to client
						//
						LinkTransmitStatPrint();
					}
				}
				else
				{
					//
					// need to reset to turn off tx100 mode
					//
					if(_LinkIfs==0)
					{
						_LinkReset=1;
						doReset();
					}
					ErrorPrint(TransmitCancel);
				}
			}
			else 
			{
			    ErrorPrint(TransmitSetup);
			}

			if(_LinkCalibrate!=0)
			{
				DevicePaPredistortionSet(pap);
				//
				// did we get good values for all chains?
				//
                for(ich=0; ich<MCHAIN; ich++)
				{
					if (CalibrationStatus(ich)!=CALNEXT_Done) 
					{
						if(DeviceIdGet()==AR5416_DEVID_AR9287_PCIE)
							continue;

						CalibrationGetTxGain(&_LinkPcdac, &_LinkTxGainIndex, &_LinkTxDacGain, &calPoint, ich);
						_CurrentCalPoint = calPoint;
						_LinkCalibrateChain = CalibrationChain(ich);
						LinkTransmitFirstTime();
						goto CalibrateAgain;
					}
				}
				//
				// yes. save it.
				//
				//CalibrateSave();
			}
   			if(_LinkXtalCal)
   			{
				//
 				// did we get a good value?
 				//
 				ppm=(int)(((_LinkFrequencyActual/_LinkFrequency)-1)*1e6);
 				XtalCalibrateStatsReturn(_LinkFrequency,_LinkTxChain,_LinkPcdac,_LinkTransmitPower[0],_LinkFrequencyActual,ppm,caps);
 				if(step>0 && (ppm<-2 || ppm>2))
   				{
   					if(ppm<0)
   					{
 						caps-=step;
   					}
 					else if(ppm>0)
   					{
 						caps+=step;
   					}
   					step/=2;
   					LinkTransmitFirstTime();
   					goto CalibrateAgain;
   				}
 				//
 				// yes. save it.
 				//
 				DeviceTuningCapsSave(caps);
   			}
 			if (_LinkCarrier!=0)
			{
				DevicePaPredistortionSet(pap);
			}
                    // Cmac Power
                    if (_LinkCmacPower)
                    {
                        cmac_power = DeviceCmacPowerGet(_LinkTxChain);
                        CmacPowerStatsReturn(_LinkFrequency,_LinkTxChain,_LinkPcdac, _LinkCmacPMPower, cmac_power);
                    }
                    if (_LinkPsatCal)
                    {
                        PsatCalibrationStatsReturn(_LinkFrequency,_LinkTxChain,_LinkPcdac);
                    }
		}
	}
	//
	// tell the client we are done
	//
    SendDone(client);
    if (_LinkCalibrateLast)
    {
		if (_CalibrationScheme==CALIBRATION_SCHEME_GAININDEX_DACGAIN)
			CalibrateSave(2);
		else
			CalibrateSave(1);
        // Force reset after calibration
        _LinkReset=1;
        doReset();
    }
}

static void LinkTransmitDut(int client)
{
	int pap;
	int ich;
 	int caps=0x40;
	int step;
 	int ppm;
	//int jch;
	//int cdiff;
	int calPoint;
    double cmac_power = 0.0;
	unsigned int hvyClipVal = 0;
	unsigned int numChains = 0;
	unsigned int chCount = 0;
	unsigned int numOfChains = 0;
	//
	// parse the input and if it is good
	// get ready to do the receive
	//
	if(ParseInput(client)==0)
	{
		pap=0;
		//
		// if there's no card loaded, return error
		//
		if(CardCheckAndLoad(client)!=0)
		{
			ErrorPrint(CardNoneLoaded);
		}
		else
		{
            _LinkCalibrateLast = 0;
			// Save client handle so we can send messages back to the right client
			//
			_LinkClient=client;
			if(_LinkCalibrate!=0)
			{
				if (_CalibrationScheme==CALIBRATION_SCHEME_GAININDEX_DACGAIN)
				{
					// peregrine calibration scheme
					_LinkTpcm=TpcmTxGainIndex;
				}
				if(CalibratePowerMean<=-100)
				{
					TargetPowerGoal();
				}
				
				DeviceHeavyClipEnableGet(&hvyClipVal);

				//UserPrint("heavyClip value 0x%x\n",hvyClipVal);
				DeviceHeavyClipEnableSet(1); // disable heavy clip

#if 0
				{// debug only
					int temp;
					DeviceHeavyClipEnableGet(&temp);
					UserPrint("heavyClip value 0x%x\n", temp);
				}
#endif
				//if (CalibrationSetMode(_LinkFrequency, _LinkRate[0], _LinkCalibrateChain)<0)
				//	return;
				//CalibrationSetPowerGoal(CalibratePowerMean, 0);

				//pap=DevicePaPredistortionGet();
				//DevicePaPredistortionSet(0);
				//ResetForce();
			}
			if (StickyExecuteDut(DEF_LINKLIST_IDX) | StickyExecuteDut(ARD_LINKLIST_IDX)) // For embedded ART, send sticky list if any to DUT before reset device
			{
                ResetForce();
                doReset();           
            } 
			//
			// Acknowledge command from client
			//
			SendOk(client);

			if(_LinkCalibrate!=0) {
				
				if (_LinkFrequency <= 4000)
				{
					_LinkFrequencyBand = 2;
				}
				else
				{
					_LinkFrequencyBand = 5;
				}

				if ( _LinkFrequencyBand != _PrevLinkFrequencyBand)
				{
					_PrevLinkFrequencyBand = _LinkFrequencyBand;
					doReset();
					queryTxGainTable();
				}

				
			}	
			if (_LinkCarrier!=0)
			{
				
				ResetForce();
            }
			if(_LinkCmacPower)
			{
				CmacPowerStatsHeader();
			}
 			if(_LinkXtalCal!=0)
 			{
 				caps=0x40;
 				step=0x20;
 				XtalCalibrateStatsHeader();
 			}
 			if(_LinkPsatCal)
			{
				PsatCalibrationStatsHeader();
			}
			LinkMode = TX_MODE;

			numOfChains = 1;

			if (_LinkCalibrate!=0)
			{
				//save for later use
				ChainsToCalibrate = _LinkTxChain;				
				numOfChains = GetNumOfChainsInCalCommand(_LinkTxChain);
			}

			
			for (chCount=0; chCount<numOfChains; chCount++)
			{
				if (_LinkCalibrate!=0)
				{
					if(numOfChains != 1)
					{
						_LinkTxChain = GetCurrentChainForCal(ChainsToCalibrate, chCount+1);
						_LinkCalibrateChain = _LinkTxChain;
					}

					if (CalibrationSetMode(_LinkFrequency, _LinkRate[0], _LinkTxChain)<0)
						return;
					CalibrationSetPowerGoal(CalibratePowerMean, 0);
			
					CalibrationTxGainReset(&_LinkPcdac, &_LinkTxGainIndex, &_LinkTxDacGain);
				}

				//
				// wait for "START" message
				//
				if(StartAlreadyReceived || MessageWait(60000)==0)
				{
					if (!StartAlreadyReceived)
					{
						StartAlreadyReceived = 1;
					}
					//
					// this is where we really do it.
					// turn on the receiver and get the packets
					//
	CalibrateAgain:
					//if(_LinkCarrier)
					//{
					//	DeviceTransmitCarrier(_LinkTxChain);
					//}

					if(_LinkXtalCal)
   					{
 						DeviceTuningCapsSet(caps);
   					}

					LinkTransmitDoItDut(caps);

					//
					// need to reset to turn off tx100 mode
					//
					if(_LinkIfs==0)
					//if(_LinkIfs==0 && !_LinkXtalCal)
					{
						_LinkReset=1;
						doReset();
					}
					else
					{
						//
						// send statistics back to client
						//
						LinkTransmitStatPrint();
					}
				}
				else
				{
					//
					// need to reset to turn off tx100 mode
					//
					if(_LinkIfs==0)
					{
						_LinkReset=1;
						doReset();
					}
					ErrorPrint(TransmitCancel);
				}

				if(_LinkCalibrate!=0)
				{
					//DeviceEepromPaPredistortionSet(pap);
					//
					// did we get good values for all chains?
					//
					for(ich=0; ich<MCHAIN; ich++)
					{
						if (CalibrationStatus(ich)!=CALNEXT_Done) 
						{
							CalibrationGetTxGain(&_LinkPcdac, &_LinkTxGainIndex, &_LinkTxDacGain, &calPoint, ich);
							_CurrentCalPoint = calPoint;
							_LinkCalibrateChain = CalibrationChain(ich);

							if (!_LinkStopCalibration)
								goto CalibrateAgain;
						}
					}
				}
   				if(_LinkXtalCal)
   				{
					//
					// did we get a good value?
					//
					ppm=(int)(((_LinkFrequencyActual/_LinkFrequency)-1)*1e6);
					XtalCalibrateStatsReturn(_LinkFrequency,_LinkTxChain,_LinkPcdac,_LinkTransmitPower[0],_LinkFrequencyActual,ppm,caps);
	                
					if(step>0 && (ppm<(-5 + DeviceXtalReferencePPMGet()) || ppm>(5 + DeviceXtalReferencePPMGet())))
					{
	                    
						if(ppm<(0 + DeviceXtalReferencePPMGet()))
						{
							caps-=step;
						}
	                    
						else if(ppm>(0 + DeviceXtalReferencePPMGet()))
						{
							caps+=step;
						}
						step/=2;
	                    
						goto CalibrateAgain;
					}
					
					else if(step==0 && (ppm<(-5 + DeviceXtalReferencePPMGet()) || ppm>(5 + DeviceXtalReferencePPMGet())))
					{
						caps = 0xffff;
					}
 					//
 					// yes. save it.
 					//
 					DeviceTuningCapsSave(caps);
				}
 				//if (_LinkCarrier!=0)
				//{
				//	DevicePaPredistortionSet(pap);
				//}

				// Cmac Power
				if (_LinkCmacPower)
				{
					cmac_power = DeviceCmacPowerGet(_LinkTxChain);
					CmacPowerStatsReturn(_LinkFrequency,_LinkTxChain,_LinkPcdac, _LinkCmacPMPower, cmac_power);
				}
	    		if (_LinkPsatCal)
				{
    				PsatCalibrationStatsReturn(_LinkFrequency,_LinkTxChain,_LinkPcdac);
    			}
			} // loop for calibrating for all the chains
		}
	} // ParseInput()
	//
	// tell the client we are done
	//
    SendDone(client);

	StartAlreadyReceived = 0;

    if (_LinkCalibrateLast || _LinkPsatCal)
    {
		if (_CalibrationScheme==CALIBRATION_SCHEME_GAININDEX_DACGAIN)
		{
			CalibrateSave(2);
			DeviceHeavyClipEnableSet(hvyClipVal); // write original heavy clip
		}
		else
			CalibrateSave(1);

		if (_CalibrationScheme==CALIBRATION_SCHEME_GAININDEX_DACGAIN)
		{
			// Applies to peregrine calibration scheme only
			// Force reset after calibration
			_LinkTpcm=TpcmTargetPower; // restore to TPC
		}
        _LinkReset=1;
        doReset();
    }
    if(_LinkXtalCal)
    {
        // save it, and send to UTF.
        DeviceTuningCapsSave(caps);
        _LinkReset=1;
        doReset();
    }
}

void LinkTransmit(int client)
{
    if (DeviceIsEmbeddedArt() == 1)
    {
        LinkTransmitDut(client);
    }
    else
    {
        LinkTransmitHost(client);
    }
}

//
// code to run the receiver in a link test
//



static void LinkReceiveStatsHeader()
{
	ErrorPrint(NartDataHeader,"|rx|frequency|tp|iss|att|pdgain|txgain|rate|pl|pc|agg||correct|throughput|error|crc|psr|rssi|rssi00|rssi01|rssi02|rssi10|rssi11|rssi12|evm0|evm1|evm2|byte|dontcountbyte|duration|rxchain|bitErrorCompares|");
  
}


static void LinkReceiveStatsReturn(int channel, 
	double pout, int iss, int att, int pdgain, int pcdac, 
	char *rate, int plength, int expected, int agg,
	int error, int crc, int correct, int tput,
	int *rssi, int mrssi,
	int *rssi1, int mrssi1, 
	int *rssi2, int mrssi2, 
	int *rssi3, int mrssi3,
	int *rssi11, int mrssi11, 
	int *rssi12, int mrssi12, 
	int *rssi13, int mrssi13,
	int *evm1, int mevm1,
	int *evm2, int mevm2,
	int *evm3, int mevm3,
    int byte, int dontCount,int duration, int rxchain,
	int rssimin,int bitErrorCompares)
 {
	char buffer[MBUFFER];
	double psr;
	int lc, nc;
	int denom;
    //
	// compute PSR
	//
	if(agg>1)
	{
		denom=agg*(expected/agg);
	}
	else
	{
		denom=expected;
	}
	if(denom>0)
	{
		psr=((100.0*correct)/((double)denom));
	}
	else
	{
		psr=0;
	}
    //
	// format message
	//
	lc=SformatOutput(buffer,MBUFFER-1,"|rx|%d|%.1f|%d|%d|%d|%d|%s|%d|%d|%d||%d|%d|%d|%d|%.3lf|",
        channel,
		pout,
		iss,
		att,
		pdgain,
		pcdac,
        rate,
		plength,
        denom,
		agg,
        correct,
		tput,
        error,
		crc,
		psr);
	
        nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,rssimin,rssi,mrssi);
	    if(nc>0)
		{
			lc+=nc;
		}
        nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,rssimin,rssi1,mrssi1);
	    if(nc>0)
		{
			lc+=nc;
		}
        nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,rssimin,rssi2,mrssi2);
	    if(nc>0)
		{
			lc+=nc;
		}
        nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,rssimin,rssi3,mrssi3);
	    if(nc>0)
		{
			lc+=nc;
		}
        nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,rssimin,rssi11,mrssi11);
	    if(nc>0)
		{
			lc+=nc;
		}
        nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,rssimin,rssi12,mrssi12);
	    if(nc>0)
		{
			lc+=nc;
		}
        nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,rssimin,rssi13,mrssi13);
	    if(nc>0)
		{
			lc+=nc;
		}
         nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,0,evm1,mevm1);
	    if(nc>0)
		{
			lc+=nc;
		}
        nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,0,evm2,mevm2);
	    if(nc>0)
		{
			lc+=nc;
		}
        nc=HistogramPrintWithBar(&buffer[lc],MBUFFER-lc,0,evm3,mevm3);
	    if(nc>0)
		{
			lc+=nc;
		}
    
	nc=SformatOutput(&buffer[lc],MBUFFER-lc,"%d|%d|%d|%d|%d|",byte, dontCount, duration, rxchain, bitErrorCompares);
	if(nc>0)
	{
		lc+=nc;
	}
	//
	// send it
	//
	ErrorPrint(NartData,buffer);
}


static void LinkReceiveStatPrint()
{
    int j;
    struct rxStats *rStats;
	int nf[MCHAIN],nfc[MCHAIN],nfe[MCHAIN],nfcal[MCHAIN],nfpower [ MCHAIN ],nfTemperature [ MCHAIN ];
	int it;
	int rxMask;
	char sRate[20];

	rxMask = DeviceRxChainMask();

    DeviceNoiseFloorFetch(nfc,nfe,MCHAIN);
	
	for(it=0; it<MCHAIN; it++)
	{
		if (rxMask & (1 << it))
		{
			nf[it]=(nfc[it]+nfe[it])/2;
			nfcal[it]=DeviceNoiseFloorGet(_LinkFrequency, it);
			nfpower[it]=DeviceNoiseFloorPowerGet(_LinkFrequency, it);
			nfTemperature[it] = DeviceNoiseFloorTemperatureGet(_LinkFrequency, it);
			UserPrint("chain: %4d nfc: %5d nfcal: %5d nfpower: %5d nfTemp: %5d\n",it,nfc[it],nfcal[it],nfpower[it],nfTemperature[it]);
		}
	}
    //
    // make return messages
	// received vs correct, chain1rssi, chain2rssi, chain3rssi???????
	//
	it=0;
	if(_LinkStat&1)
	{
		LinkReceiveStatsHeader();
        for (j = 0; j < _LinkRateMany; j++) 
		{
			ithRateName(sRate, _LinkRate[j]);
            rStats=LinkRxStatFetch(_LinkRate[j]);
            LinkReceiveStatsReturn(_LinkFrequency,
				_LinkTransmitPower[j],
				_LinkIss,
				_LinkAtt,
				_LinkPdgain,
				_LinkPcdac,
				sRate,	//rateStrAll[_LinkRate[j]],
				_LinkPacketLength[j],
				_LinkPacketMany,
				_LinkAgg,
				rStats->otherError,
				rStats->crcPackets,
				rStats->goodPackets,
				rStats->rxThroughPut,
				rStats->rssi,MRSSI,
				rStats->rssic[0],MRSSI,
				rStats->rssic[1],MRSSI,
				rStats->rssic[2],MRSSI,
				rStats->rssie[0],MRSSI,
				rStats->rssie[1],MRSSI,
				rStats->rssie[2],MRSSI,
				rStats->evm[0],MEVM,
				rStats->evm[1],MEVM,
				rStats->evm[2],MEVM,
                rStats->byteCount,rStats->dontCount,rStats->endTime-rStats->startTime, _LinkRxChain,
				rStats->rssimin,
				rStats->bitErrorCompares);
		}
#ifdef DOALL
        rStats=LinkRxStatTotalFetch();
         LinkReceiveStatsReturn(_LinkFrequency,
			_LinkTransmitPower[0],
			_LinkIss,
			_LinkAtt,
			_LinkPdgain,
			_LinkPcdac,
			"ALL",
			_LinkPacketLength[0],
			_LinkPacketMany,
			_LinkAgg,
			rStats->otherError,
			rStats->crcPackets,
			rStats->goodPackets,
			rStats->rxThroughPut,
			rStats->rssi,MRSSI,
			rStats->rssic[0],MRSSI,
			rStats->rssic[1],MRSSI,
			rStats->rssic[2],MRSSI,
			rStats->rssie[0],MRSSI,
			rStats->rssie[1],MRSSI,
			rStats->rssie[2],MRSSI,
			rStats->evm[0],MEVM,
			rStats->evm[1],MEVM,
			rStats->evm[2],MEVM,
            rStats->byteCount,rStats->endTime-rStats->startTime);
#endif
	}
}

static void LinkReceiveSpurPrint(){

    if(_LinkStat&1){
        int i;
        RX_SPUR_STATS_STRUCT* rSpurStats;
        rSpurStats = (RX_SPUR_STATS_STRUCT *)LinkRxStatFetch(0);

        ErrorPrint(NartDataHeader,"|rx|frequency|spurLevel|");
        for(i=0;i<56;i++){
            char buffer[MBUFFER];
            SformatOutput(buffer,MBUFFER,"|rx|%.2f|%.2f|",rSpurStats->freq[i],rSpurStats->spurLevel[i]);
            ErrorPrint(NartData,buffer);
        }
    }
    /* Turn off find spur after finished.*/
    _LinkFindSpur = 0;
    LinkRxFindSpur(_LinkFindSpur);
    DeviceSpectralScanDisable();

}

static void LinkReceiveDoItDut(int duration)
{
	struct rxStats *rStats;
	int many, rate;
	int timeout;
	char buffer[MBUFFER]; 
	int nc; 
	int rxMask;
	//
	// make the timeout a little bit longer, just in case
	//
	//int nf[MCHAIN],nfc[MCHAIN],nfe[MCHAIN],nfcal[MCHAIN],nfpower[MCHAIN];
	int nfc[MCHAIN],nfe[MCHAIN];
	int it;

	timeout=duration;
	if(timeout>0)
	{
		timeout+=100;
	}

	if(_LinkRssiCalibrate==0)
	{
		LinkRxFindSpur(_LinkFindSpur);

		//
		// setup and start the receiver
		//
		LinkRxDataStartCmd(_LinkFrequency, _LinkCenterFrequencyUsed, _LinkAntenna, _LinkRxChain, _LinkPromiscuous, _LinkBc,
		                    _LinkBssId, _LinkRxStation, _LinkPacketMany, _LinkRate, _LinkRateMany, _LinkBandwidth, _LinkDataCheck);

		//
		// tell the client the receiver is on
		//
		SendOn(_LinkClient);
		//
		// track the descriptors as they arrive
		//
		_LinkSpectralScanMany = 0;
		LinkRxComplete(timeout, _LinkNdump, _LinkPattern, 2, LinkStopIt);
	} else {
		UserPrint("Adjust noise floor numbers\n");

		DeviceNoiseFloorFetch(nfc,nfe,MCHAIN);
		for(it=0; it<MCHAIN; it++)
		{
			//UserPrint("[before]nf[%d] = %d, %d\n", it, nfc[it], nfe[it]);
			nfc[it] = nfc[it] + (int)(_LinkSpectralScanRssiControl[it] - 0.5); 
			nfe[it] = nfe[it] + (int)(_LinkSpectralScanRssiExtension[it] - 0.5);
			_NoiseFloorCal[it] = nfc[it];
			//UserPrint("[After]nf[%d] = %d, %d\n", it, nfc[it], nfe[it]);
		}


		// Apply noise floor numbers.
		NoiseFloorLoadWait(nfc, nfe, MCHAIN, 100);
		for(it=0; it<MCHAIN; it++)
		{
			nfc[it] = -1;
			nfe[it] = -1;
		}
	    
		DeviceNoiseFloorFetch(nfc,nfe,MCHAIN);
		for(it=0; it<MCHAIN; it++)
		{
			UserPrint("NF %d: %5d %5d\n",it,nfc[it],nfe[it]);
		}
		//_LinkNoiseFloor=-100;
		//LinkReceiveNoiseFloor();

#if 0 /* Currently use normal packets instead of SS packets */
		UserPrint(">>>>>>>>>>>>Perform SS with signal\n");
		// Start Spectral scan here
		LinkRxSpectralScan(1,LinkSpectralScanProcess);
#endif
		many=_LinkRateMany;
		rate=_LinkRate[0];
		_LinkRateMany=1;
		_LinkRate[0]=0x20;/* always use HT20 MCS0 as same as LP script file */
		LinkRxDataStartCmd(_LinkFrequency, _LinkCenterFrequencyUsed, _LinkAntenna, _LinkRxChain, _LinkPromiscuous, _LinkBc,
                               _LinkBssId, _LinkRxStation, _LinkPacketMany, _LinkRate, _LinkRateMany, _LinkBandwidth, _LinkDataCheck);
		SendOn(_LinkClient);
		_LinkSpectralScanMany = 0;
		// This will allow Litepoint to command a stop 
		LinkRxComplete(1000, _LinkNdump, _LinkPattern, 2, LinkStopIt);  
		_LinkRateMany=many;
		_LinkRate[0]=rate;
		rStats=LinkRxStatFetch(0x20);
		_LinkSpectralScanRssi=HistogramMedian(rStats->rssi,MRSSI)+rStats->rssimin;
		rxMask = DeviceRxChainMask();
		for(it=0; it<MCHAIN; it++)
		{
			if (rxMask & (1 << it))
			{
				_LinkSpectralScanRssiControl[it]=HistogramMedian(rStats->rssic[it],MRSSI)+rStats->rssimin;
				_LinkSpectralScanRssiExtension[it]=HistogramMedian(rStats->rssie[it],MRSSI)+rStats->rssimin;
			}
		}

		nc=SformatOutput(buffer,MBUFFER-1,"spectral scan rssi=%lg   c=%lg %lg %lg   e=%lg %lg %lg\n",
		                 _LinkSpectralScanRssi,
		                 _LinkSpectralScanRssiControl[0],
		                 _LinkSpectralScanRssiControl[1],
		                 _LinkSpectralScanRssiControl[2],
		                 _LinkSpectralScanRssiExtension[0],
		                 _LinkSpectralScanRssiExtension[1],
		                 _LinkSpectralScanRssiExtension[2]);
		ErrorPrint(NartData,buffer); 
	}
	//
	// tell the client the receiver is off
	//
	SendOff(_LinkClient);
}

static void LinkReceiveDoIt(int duration)
{
    struct rxStats *rStats;
    int many, rate;
	int timeout;
    char buffer[MBUFFER]; 
    int nc; 
    //
	// make the timeout a little bit longer, just in case
	//
	//int nf[MCHAIN],nfc[MCHAIN],nfe[MCHAIN],nfcal[MCHAIN],nfpower[MCHAIN];
	int nfc[MCHAIN],nfe[MCHAIN];
	int it;

	timeout=duration;
	if(timeout>0)
	{
		timeout+=100;
	}

    if(_LinkRssiCalibrate==0)
    {
        //
	    // start the receiver
	    //
	    LinkRxStart(_LinkPromiscuous);
	    //
	    // tell the client the receiver is on
	    //
        SendOn(_LinkClient);
	    //
	    // track the descriptors as they arrive
	    //
        _LinkSpectralScanMany = 0;
	    LinkRxComplete(timeout, _LinkNdump, _LinkPattern, 2, LinkStopIt);
     }
     else
     {

		UserPrint("Adjust noise floor numbers\n");

		DeviceNoiseFloorFetch(nfc,nfe,MCHAIN);
		for(it=0; it<MCHAIN; it++)
		{
			nfc[it] = nfc[it] + (int)(_LinkSpectralScanRssiControl[it] - 0.5); 
			nfe[it] = nfe[it] + (int)(_LinkSpectralScanRssiExtension[it] - 0.5);
			_NoiseFloorCal[it] = nfc[it];
		}


		// Apply noise floor numbers.
		NoiseFloorLoadWait(nfc, nfe, MCHAIN, 100);

		for(it=0; it<MCHAIN; it++)
		{
			nfc[it] = -1;
			nfe[it] = -1;
		}
	    
		DeviceNoiseFloorFetch(nfc,nfe,MCHAIN);
		for(it=0; it<MCHAIN; it++)
		{
			UserPrint("NF %d: %5d %5d\n",it,nfc[it],nfe[it]);
		}
	//	_LinkNoiseFloor=-100;
	//    LinkReceiveNoiseFloor();

        UserPrint(">>>>>>>>>>>>Perform SS with signal\n");
        // Start Spectral scan here
        LinkRxSetup(_LinkBssId,_LinkRxStation);
        LinkRxSpectralScan(1,LinkSpectralScanProcess);

        many=_LinkRateMany;
        rate=_LinkRate[0];
        _LinkRateMany=1;
        _LinkRate[0]=20;

        LinkRxStart(_LinkPromiscuous);
        SendOn(_LinkClient);
        //MyDelay(1000);
        _LinkSpectralScanMany = 0;
        LinkRxComplete(100, _LinkNdump, _LinkPattern, 2, 0);
        _LinkRateMany=many;
        _LinkRate[0]=rate;
        rStats=LinkRxStatFetch(20);
        _LinkSpectralScanRssi=HistogramMedian(rStats->rssi,MRSSI)+rStats->rssimin;
        for(it=0; it<MCHAIN; it++)
        {
            _LinkSpectralScanRssiControl[it]=HistogramMedian(rStats->rssic[it],MRSSI)+rStats->rssimin;
            _LinkSpectralScanRssiExtension[it]=HistogramMedian(rStats->rssie[it],MRSSI)+rStats->rssimin;
        }

       // UserPrint("spectral scan rssi=%lg   c=%lg %lg %lg   e=%lg %lg %lg\n", _LinkSpectralScanRssi, _LinkSpectralScanRssiControl[0],_LinkSpectralScanRssiControl[1],  _LinkSpectralScanRssiControl[2], _LinkSpectralScanRssiExtension[0], _LinkSpectralScanRssiExtension[1], _LinkSpectralScanRssiExtension[2]);

        nc=SformatOutput(buffer,MBUFFER-1,"spectral scan rssi=%lg   c=%lg %lg %lg   e=%lg %lg %lg\n",
            _LinkSpectralScanRssi,
            _LinkSpectralScanRssiControl[0],
            _LinkSpectralScanRssiControl[1],
            _LinkSpectralScanRssiControl[2],
            _LinkSpectralScanRssiExtension[0],
            _LinkSpectralScanRssiExtension[1],
            _LinkSpectralScanRssiExtension[2]);
        ErrorPrint(NartData,buffer); 
     }
    //
	// tell the client the receiver is off
	//
	SendOff(_LinkClient);

}

static void LinkSpectralScanHeader()
{
	ErrorPrint(NartDataHeader,"|ss|count|rssi|rssi00|rssi01|rssi02|rssi10|rssi11|rssi12|spectrum|");
}

static void LinkSpectralScanData(int rssi, int *rssic, int *rssie, int nchain, int *spectrum, int nspectrum)
{
	char buffer[MBUFFER];
	int nc;

	nc=SformatOutput(buffer,MBUFFER-1,"|ss|%d|%d|%d|%d|%d|%d|%d|%d|",
		_LinkSpectralScanMany,
		rssi,
		rssic[0],rssic[1],rssic[2],
		rssie[0],rssie[1],rssie[2]);
	HistogramPrintWithBar(&buffer[nc],MBUFFER-nc-1,0,spectrum,nspectrum);
	ErrorPrint(NartData,buffer);
}

static void LinkSpectralScanProcess(int rssi, int *rssic, int *rssie, int nchain, int *spectrum, int nspectrum)
{
	if(_LinkSpectralScanMany<=0)
	{
		_LinkSpectralScanMany=0;
		LinkSpectralScanHeader();
	}
	LinkSpectralScanData(rssi,rssic,rssie,nchain,spectrum,nspectrum);
	_LinkSpectralScanMany++;
}

static void LinkRxIqCalibrationSave()
{
	ErrorPrint(NartDataHeader,"|rxiqcal|frequency|chain|ce|iss||rssi|nf|offset|");
	//
	// get data and return to client
	//
}

static void LinkRssiCalibrationSave()
{
    int it;
    int nfpower;
    char buffer[MBUFFER];
	int rxMask;

    ErrorPrint(NartDataHeader,"|rssi|frequency|chain|ce|iss||rssi|nfcal|nfpower|temperature|");
	rxMask = DeviceRxChainMask();
    for(it=0; it<MCHAIN; it++)
    {
		if (rxMask & (1 << it))
		{
			nfpower=(int)(_LinkIss-_LinkSpectralScanRssiControl[it]);
			SformatOutput(buffer,MBUFFER-1,"|rssi|%d|%d|c|%d||%.11f|%d|%d|%d",
				_LinkFrequency,it,_LinkIss,_LinkSpectralScanRssiControl[it],_NoiseFloorCal[it],nfpower,_LinkNoiseFloorTemperature);

			buffer[MBUFFER-1]=0;
			ErrorPrint(NartData,buffer);

			// Check for range of -58 to -122

			DeviceNoiseFloorSet(_LinkFrequency, it, _NoiseFloorCal[it]);
			DeviceNoiseFloorPowerSet(_LinkFrequency, it, nfpower);
			DeviceNoiseFloorTemperatureSet(_LinkFrequency, it, _LinkNoiseFloorTemperature);
		}
    }
}


static void LinkReceiveNoiseFloor()
{
	int nf;
	//
	// install noise floor values
	//
	if(_LinkNoiseFloor<0)
	{
        NoiseFloorDo(_LinkFrequency, &_LinkNoiseFloor, 1, 0, 0, 0, 0, _LinkNoiseFloorControl, _LinkNoiseFloorExtension, MCHAIN);
	}
	//
	// or compute noise floor values
	//
	else if(_LinkNoiseFloor>0)
	{
		nf= -_LinkNoiseFloor;
        NoiseFloorDo(_LinkFrequency, &nf, 1, 20, 11, 100, 0, _LinkNoiseFloorControl, _LinkNoiseFloorExtension, MCHAIN);
	}

    _LinkNoiseFloorTemperature=DeviceTemperatureGet(1);
}

static int LinkReceiveSpectralScanDut()
{
    struct rxStats *rStats;
	int many, rate;
	int it;
	int rxMask;
	//
	// if there's no card loaded, return error
	//
	if(CardCheckAndLoad(_LinkClient)!=0)
	{
		return -ErrorPrint(CardNoneLoaded);
	}
	else
	{
#ifdef SPECTRAL_SCAN_IMPLEMENTED
		LinkMode = RX_MODE;
#if 0 /* Currently use normal packets instead of SS packets */
		LinkRxSpectralScan(1,LinkSpectralScanProcess);
#endif
		//
		// turn on the receiver and get the packets
		// ######### need to save these and then restore after
		//
		many=_LinkRateMany;
		rate=_LinkRate[0];
		_LinkRateMany=1;
		_LinkRate[0]=0x20; /* HT20 MCS0 sa same sa LP script file */
		//
		// start the receiver
		//
		LinkRxDataStartCmd(_LinkFrequency, _LinkCenterFrequencyUsed, _LinkAntenna, _LinkRxChain, _LinkPromiscuous, _LinkBc,
                               _LinkBssId, _LinkRxStation, _LinkPacketMany, _LinkRate, _LinkRateMany, _LinkBandwidth, _LinkDataCheck);
        
		//
		// track the descriptors as they arrive
		//
		LinkRxComplete(100, _LinkNdump, _LinkPattern, _LinkPatternLength, 0);

 		_LinkRateMany=many;;
		_LinkRate[0]=rate;
		//
		// ###########analyze statistics 
		// i wonder what rate the fft data is marked with
		//
		rStats=LinkRxStatFetch(0x20);
		_LinkSpectralScanRssi=HistogramMean(rStats->rssi,MRSSI)+rStats->rssimin;
		rxMask = DeviceRxChainMask();
		for(it=0; it<MCHAIN; it++)
		{
			if (rxMask & (1 << it))
			{
				_LinkSpectralScanRssiControl[it]=HistogramMean(rStats->rssic[it],MRSSI)+rStats->rssimin;
				_LinkSpectralScanRssiExtension[it]=HistogramMean(rStats->rssie[it],MRSSI)+rStats->rssimin;
			}
		}
#else
        // Assume 0 for now until spectral scan is implemented in UTF.
		for(it=0; it<MCHAIN; it++)
		{
		    _LinkSpectralScanRssiControl[it] = 0;
			_LinkSpectralScanRssiExtension[it] = 0;
		}
#endif

		UserPrint("spectral scan rssi=%lg   c=%lg %lg %lg   e=%lg %lg %lg\n",
			_LinkSpectralScanRssi,
			_LinkSpectralScanRssiControl[0],
			_LinkSpectralScanRssiControl[1],
			_LinkSpectralScanRssiControl[2],
			_LinkSpectralScanRssiExtension[0],
			_LinkSpectralScanRssiExtension[1],
			_LinkSpectralScanRssiExtension[2]);


		return (int)_LinkSpectralScanRssi;
	}
	return 0;
}

static int LinkReceiveSpectralScan()
{
    struct rxStats *rStats;
	int many, rate;
	int it;
	int rxMask;
	//
	// if there's no card loaded, return error
	//
	if(CardCheckAndLoad(_LinkClient)!=0)
	{
		return -ErrorPrint(CardNoneLoaded);
	}
	else
	{
		LinkMode = RX_MODE;
		//
		// Setup the descriptors for receive
		//
		LinkRxSetup(_LinkBssId,_LinkRxStation);

		LinkRxSpectralScan(1,LinkSpectralScanProcess);
		//
		// turn on the receiver and get the packets
		// ######### need to save these and then restore after
		//
		many=_LinkRateMany;
		rate=_LinkRate[0];
		_LinkRateMany=1;
		_LinkRate[0]=20;
		//
		// start the receiver
		//
		LinkRxStart(_LinkPromiscuous);
		//
		// track the descriptors as they arrive
		//
		LinkRxComplete(100, _LinkNdump, _LinkPattern, _LinkPatternLength, 0);

 		_LinkRateMany=many;;
		_LinkRate[0]=rate;
		//
		// ###########analyze statistics 
		// i wonder what rate the fft data is marked with
		//
        rStats=LinkRxStatFetch(20);
		_LinkSpectralScanRssi=HistogramMean(rStats->rssi,MRSSI)+rStats->rssimin;
		rxMask = DeviceRxChainMask();
		for(it=0; it<MCHAIN; it++)
		{
			if (rxMask & (1 << it))
			{
				_LinkSpectralScanRssiControl[it]=HistogramMean(rStats->rssic[it],MRSSI)+rStats->rssimin;
				_LinkSpectralScanRssiExtension[it]=HistogramMean(rStats->rssie[it],MRSSI)+rStats->rssimin;
			}
		}
		UserPrint("spectral scan rssi=%lg   c=%lg %lg %lg   e=%lg %lg %lg\n",
			_LinkSpectralScanRssi,
			_LinkSpectralScanRssiControl[0],
			_LinkSpectralScanRssiControl[1],
			_LinkSpectralScanRssiControl[2],
			_LinkSpectralScanRssiExtension[0],
			_LinkSpectralScanRssiExtension[1],
			_LinkSpectralScanRssiExtension[2]);


		return (int)_LinkSpectralScanRssi;
	}
	return 0;
}

static int LinkReceiveFirstTime()
{
	int error;
	//
	// should we do a reset?
	// yes, if the user explicitly asked for it (_LinkReset>0) or if
	// the user asked for auto mode (_LinkReset<0) and any important parameters have changed
	//
    error=doReset();
	if(error!=0)
	{
		return error;
	}
	//
	// if we are doing rssi calibration, force noise floor
	//
	if(_LinkRssiCalibrate!=0)
	{
		if(_LinkNoiseFloor==0)
		{
			_LinkNoiseFloor= 110;
		}
	}
	//
	// if we are supposed to do noise floor calibration, do it here
	// after reset and before setting up descriptors
	//
	if(_LinkNoiseFloor!=0)
	{
		LinkReceiveNoiseFloor();
		if(_LinkSpectralScan==0)
		{
			LinkReceiveSpectralScan();
			//
			// need to reset to turn off carrier mode
			//
			_LinkReset=1;
			doReset();
		}
	}
    //
	// get all of the packets descriptors ready to run
	//
    LinkRxSetup(_LinkBssId,_LinkRxStation);

	LinkRxSpectralScan(_LinkSpectralScan,LinkSpectralScanProcess);

	return 0;
}


void LinkReceiveHost(int client)
{
	//
	// parse the input and if it is good
	// get ready to do the receive
	//
	if(ParseInput(client)==0)
	{
		//
		// if there's no card loaded, return error
		//
		if(CardCheckAndLoad(client)!=0)
		{
			ErrorPrint(CardNoneLoaded);
		}
		else
		{
			_LinkClient=client;

			LinkMode = RX_MODE;
			//
			// Setup the descriptors for receive
			//
			if(LinkReceiveFirstTime()==0)
			{
				//
				// Acknowledge command from client
				//
				SendOk(client);


				//	LinkReceiveStatPrint();
				//
				// wait for "START" message
				//
				if(MessageWait(60000)==0)
				{
					if(_LinkSpectralScan)
					{
						_LinkRateMany=1;
						_LinkRate[0]=20;
					}
					//
					// this is where we really do it.
					// turn on the receiver and get the packets
					//
					LinkReceiveDoIt(_LinkDuration);	
					//
					// if we are doing rssi calibration, save the data here
					//
					if(_LinkRssiCalibrate)
					{
						LinkRssiCalibrationSave();
					}
					//
					// if we are doing rx iq calibration, save the data here
					//
					if(_LinkRxIqCal)
					{
						LinkRxIqCalibrationSave();
					}
					//
					// send statistics back to client
					//
					LinkReceiveStatPrint();
				}
				else
				{
					ErrorPrint(ReceiveCancel);
				}
			} 
			else 
			{
			    ErrorPrint(ReceiveSetup);
			}
		}
	}
	SendDone(client);
}

void LinkReceiveDut(int client)
{
	//
	// parse the input and if it is good
	// get ready to do the receive
	//
	if(ParseInput(client)==0)
	{
		//
		// if there's no card loaded, return error
		//
		if(CardCheckAndLoad(client)!=0)
		{
			ErrorPrint(CardNoneLoaded);
		}
		else
		{
			_LinkClient=client;
         
			if (StickyExecuteDut(DEF_LINKLIST_IDX)) // For embedded ART, send sticky list if any to DUT before reset device
			{
				ResetForce();
				doReset();
			}          

			/* copy from LinkReceiveFirstTime() */
			//
			// we are doing rssi calibration, force noise floor
			//
			if(_LinkRssiCalibrate!=0)
			{
				if(_LinkNoiseFloor==0) 
				{
					_LinkNoiseFloor= 110;
				}
			}
			//
			// if we are supposed to do noise floor calibration, do it here
			// after reset and before setting up descriptors
			//
			if(_LinkNoiseFloor!=0)
			{
				LinkReceiveNoiseFloor();
				// This condition should probably be removed as spectral scan needs to be performed regardless
				// for NF estimation.

				if(_LinkSpectralScan==0)
				{
					LinkReceiveSpectralScanDut();
					//
					// need to reset to turn off carrier mode
					//
					_LinkReset=1;
					doReset();
				}
 			}

			LinkRxSpectralScan(_LinkSpectralScan,LinkSpectralScanProcess);
			/* end of copy from LinkReceiveFirstTime() */

			//
			// Acknowledge command from client
			//
			SendOk(client);

			LinkMode = RX_MODE;
				
			//
			// wait for "START" message
			//
			if(MessageWait(60000)==0)
			{
				if(_LinkSpectralScan)
				{
					_LinkRateMany=1;
					_LinkRate[0]=0x20;
				}
				//
				// this is where we really do it.
				// turn on the receiver and get the packets
				//
				LinkReceiveDoItDut(_LinkDuration);  
				//
				// if we are doing rssi calibration, save the data here
				//
				if(_LinkRssiCalibrate)
				{
					LinkRssiCalibrationSave();
				}
				//
				// if we are doing rx iq calibration, save the data here
				//
				if(_LinkRxIqCal)
				{
					LinkRxIqCalibrationSave();
				}
				//
				// send statistics back to client
				//
				if(_LinkFindSpur){
					LinkReceiveSpurPrint();
				}else{
					LinkReceiveStatPrint();
				}
            }
		    else
			{
				ErrorPrint(ReceiveCancel);
			}
		}
	}
	SendDone(client);
	SleepModeWakeupSet();
}

void LinkReceive(int client)
{
    if (DeviceIsEmbeddedArt())
    {
        LinkReceiveDut(client);
    }
    else
    {
        LinkReceiveHost(client);
    }
}

void RfBbTestPointParameterSplice(struct _ParameterList *list)
{
	// Already displayed in cart
    //list->nspecial=sizeof(RfBbTestPointParameter)/sizeof(RfBbTestPointParameter[0]);
    //list->special=RfBbTestPointParameter;
}

void RfBbTestPoint(int client)
{
	unsigned char AnaOutEn = 1;

	if(CardCheckAndLoad(client)!=0)
	{
		ErrorPrint(CardNoneLoaded);
		return;
	}
	if(ParseInput(client)==0)
	{
    	_LinkClient=client;

		SendOk(client);
	 
		if(MessageWait(60000)==0)
		{
			//
			// start continuous receive mode
			//
       		RfBbTestPointStart(_LinkFrequency, _LinkHt40, _LinkBandwidth, _LinkAntennaPair, _LinkChainNumber, 
                               _LinkRxGain[0], _LinkRxGain[1], _LinkCoex, _LinkSharedRx, _LinkSwitchTable, AnaOutEn,
                               LinkTxIsOn, LinkStopIt);
			SendOff(client);
			//
			// need to reset to turn off cont. rx mode
			//
			_LinkReset=1;
			doReset();
		}
		else
		{
			ErrorPrint(ReceiveCancel);
		}
	}
	SendDone(client);
}


