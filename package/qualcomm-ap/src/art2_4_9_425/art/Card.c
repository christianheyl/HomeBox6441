

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "wlantype.h"


//#include "AnwiDriverInterface.h"

#include "ParameterSelect.h"
#include "Card.h"
#include "ResetForce.h"
#include "smatch.h"
#include "UserPrint.h"
#include "TimeMillisecond.h"
#include "CommandParse.h"
#include "ParameterParse.h"
#include "NewArt.h"
#include "Sticky.h"

#include "ErrorPrint.h"
#include "ParseError.h"
#include "CardError.h"
#include "NartError.h"

//#include "LinkTx.h"
//#include "LinkRx.h"
#include "rate_constants.h"
#include "vrate_constants.h"
#include "LinkList.h"
#include "NoiseFloor.h"

#include "Device.h"

#ifdef DYNAMIC_DEVICE_DLL
#include "DeviceLoad.h"
#include "LinkLoad.h"
#endif

//#include "ConfigurationGet.h"
#include "Calibrate.h"

#include "NoiseFloor.h"


//#include "ConfigurationSet.h"
#include "ChipIdentify.h"
#include "Channel.h"

#include "templatelist.h"
#include "SetConfig.h"

#define MBUFFER 1024
#define MLABEL 30


//
// index for the ANWI driver stuff
//
static int DeviceIdUserInput = -1;

//
// keep track of whether we've done a reset. 
// there should be a way to ask the chip, but right now some things don't work (e.g. EepromWrite())
// unless a reset has been performed at least once since the last power down.
//	
static int ResetDone=0;

static int _DeviceValid=0;

//static int PapdEnable=0;
//static HAL_CHANNEL channel;
static int ChannelLast=0;
static int TimeLast=0;
static int BandwidthLast=0;
static unsigned int TxChainLast=0;
static unsigned int RxChainLast=0;
//static unsigned int PapdEnableLast=0;
static int preference,npreference;

#define MBUFFER 1024

void FreeMemoryPrint(void)
{
    int calibration;
	int pcie;

    calibration=DeviceEepromReport(0,0);
    if(calibration<0)
    {
	    calibration=DeviceEepromSize();
    }
    pcie=DeviceInitializationUsed();
    ErrorPrint(CardFreeMemory,calibration-pcie,calibration,pcie);
}


//
// return the channel frequency used in the last chip reset
//
int CardFrequency(void)
{
	return ChannelLast;
}

int CardRemoveDo(void)
{
    ResetDone=0;
    _DeviceValid=0;
    DeviceIdUserInput = -1;
	StickyClear(ARN_LINKLIST_IDX, 0);
    StickyClear(ARD_LINKLIST_IDX, 0);
    StickyClear(MRN_LINKLIST_IDX, 0);
    StickyClear(ARD_LINKLIST_IDX, 0);
	CalibrateClear();
    DeviceDetach();
    return 0;
}


void CardUnloadDataSend(int client)
{
	ErrorPrint(NartData,"|set|devid||");
	ErrorPrint(NartData,"|set|mac||");
	ErrorPrint(NartData,"|set|txChains||");
	ErrorPrint(NartData,"|set|rxChains||");
	ErrorPrint(NartData,"|set|2GHz||");
	ErrorPrint(NartData,"|set|5GHz||");
	ErrorPrint(NartData,"|set|4p9GHz||");
	ErrorPrint(NartData,"|set|HalfRate||");
	ErrorPrint(NartData,"|set|QuarterRate||");
}


int CardRemove(int client)
{	
	if(!CardValid())
	{
		ErrorPrint(CardNoneLoaded);
	}
	//
	// do it anyway
	//
	CardRemoveDo();
	//
	// send data to cart
	//
	CardUnloadDataSend(client);
	//
	// announce success
	//
	ErrorPrint(CardUnloadGood);
	//
	// say we're done
	//
	SendDone(client);
	
	return 0;
}
int CardPllScreen(int client)
{	
	int error;
	
	if(!CardValid())
	{
		ErrorPrint(CardNoneLoaded);
	}

printf("\n => CardPllScreen \n");

	error=DevicePllScreen();

	//
	// say we're done
	//
	SendDone(client);
	
printf("\n <= CardPllScreen \n");
	return error;
}


#ifdef DOLATERRXIQCAL
int CardRxIqCalComplete()
{
	HAL_BOOL isCalDone=0;
	int status;
	u_int32_t sched_cals=IQ_MISMATCH_CAL;
	u_int8_t rxchainmask=RxChainLast;
	
   	if(DeviceIdGet()==0x2e)
   	{
		status=ar5416Calibration(AH, &channel, rxchainmask,
					  0, &isCalDone, 0, &sched_cals);
	}
	else
    {
	status=ar9300_calibration(AH, &channel, rxchainmask,
                  0, &isCalDone, 0, &sched_cals);
	}

    return isCalDone;
}
#endif


int CardResetDo(int frequency, int txchain, int rxchain, int bandwidth)
{	
	int error;


	StickyExecute(MRD_LINKLIST_IDX);
	StickyExecute(MRN_LINKLIST_IDX);
	error=DeviceReset(frequency, txchain, rxchain, bandwidth);
	if(error==0)
	{
		StickyExecute(ARD_LINKLIST_IDX);
		StickyExecute(ARN_LINKLIST_IDX);
		ResetDone=1;
	}
	else
	{
		ResetDone=0;
	}

	return error;
}


#define MCHAIN 3
int NoiseFloorBad(int rxchain, int bandwidth)
{
	int it, numOfChains;
	int nfc[MCHAIN],nfe[MCHAIN];

    numOfChains = DeviceChainMany();

	if(NoiseFloorFetchWait(nfc,nfe,numOfChains,1000)!=0)
	{
		//DOLATERNOISEFLOOR need error message here
		return 1;
	}
    for(it=0; it<numOfChains; it++)
	{
		if(rxchain&(1<<it))
		{
			if(nfc[it]< -130)
			{
				ErrorPrint(CardNoiseFloorBad,nfc[0],nfe[0],nfc[1],nfe[1],nfc[2],nfe[2]);
				return 1;
			}
			if((bandwidth==BW_AUTOMATIC||bandwidth==BW_HT40_PLUS||bandwidth==BW_HT40_MINUS) && (nfe[it]< -130)) 
			{
 				ErrorPrint(CardNoiseFloorBad,nfc[0],nfe[0],nfc[1],nfe[1],nfc[2],nfe[2]);
 				return 1;
 			}
		}
	}
	return 0;
}


//
// ht40 = 0, dynamic ht40 mode is off
// ht40 = -1, extension is at lower frequency
// ht40 = 1, extension is higher frequency
// ht40 = other, try higher frequency for extension first, then lower
//
int CardResetIfNeeded(int frequency, int txchain, int rxchain, int force, int bandwidth)
{
   	int start;
   	int error;
   	int bandwidthUse,bandwidthTemp;
   	int rxchainuse;
   	int tfrequency;
   	int nchain;
   
   	error=0;
    if(!CardValid())
   	{
   		return -1;
   	}
   
   	if(bandwidth == BW_AUTOMATIC)
   	{
   		if(ChannelFind(frequency,frequency,bandwidth,&tfrequency,&bandwidthUse))
		{
			//
			// if we can't find the channel, presume HT20 and let reset worry about it.
			//
			bandwidthUse=BW_HT20;
		}
		else
		{
			frequency=tfrequency;
		}
   	}
   	else
   	{
   		bandwidthUse=bandwidth;
   	}
   	//
   	// if something in the code has signalled that the current configuration is invalid
   	// then we force a reset even if the current command doesn't want one.
   	//
   	if(ResetForceGet())
   	{
   		force=1;
   	}
   
   	//
   	// get maximum chain mask
   	//
   	nchain=DeviceChainMany();
    txchain &= DeviceTxChainMask();

   	nchain=DeviceRxChainMany();
   	//}*/
    rxchainuse = DeviceRxChainMask();
    
   	start=TimeMillisecond();
   	if(!ResetDone || force>0 || 
   		(force<0 && 
   		    (
   		    ChannelLast!=frequency || 
   			bandwidth!=BandwidthLast ||
   			TxChainLast!=txchain ||
   			RxChainLast!=rxchain ||
   		    start-TimeLast>=600000 
   			)
   		)
   	)
   	{	
   		//
   		// reset the device using HAL, always reset with all chains on.
   		//
        error=CardResetDo(frequency, txchain, rxchainuse, bandwidthUse);
   		if(error!=0)
   		{
   			ErrorPrint(CardResetBad,error,frequency,bandwidthUse,txchain,rxchain);
   		}      
 		if(error!=0 || NoiseFloorBad(rxchainuse, bandwidthUse))		// ####need to get correct chain mask
   		{
   			//
   			// sometimes reset fails, we don't know why, but it appears that resetting
   			// to another channel may fix it. so we're going to try this experiment.
   			//
        	if(ChannelFind(frequency+1,10000,bandwidth,&tfrequency,&bandwidthTemp))
   			{
   				ChannelFind(0,frequency-1,bandwidth,&tfrequency,&bandwidthTemp);	
   			}
  
   			//
   			// reset the device using HAL, always reset with all chains on.
   			//
   			error=CardResetDo(tfrequency, txchain, rxchainuse, bandwidthTemp);
   			//
   			// Now we have done the reset to alternative channel now try the original channel
   			//
   			error=CardResetDo(frequency, txchain, rxchainuse, bandwidthUse);
 			if(error!=0 || NoiseFloorBad(rxchainuse, bandwidthUse))		// ####need to get correct chain mask
 			{
 				error=1;
 			}
   		}
   		//
   		// record the parameters we used
   		//
   		if(error==0)
   		{
   			TimeLast=start;
   			ChannelLast=frequency;
   			BandwidthLast=bandwidthUse;
   			TxChainLast=txchain;
   			RxChainLast=rxchain;
   			ResetForceClear();
   			//
   			// reset is done, now set the rx chain mask registers.
   			//
   			DeviceRxChainSet(rxchain);
   
   			ErrorPrint(CardResetSuccess,frequency,bandwidthUse,txchain,rxchainuse);
   		}
   		else
   		{
   			TimeLast=0;
   			ChannelLast=0;
   			BandwidthLast=BW_AUTOMATIC;
   			TxChainLast=0;
   			RxChainLast=0;
   			ResetForce();
   			ErrorPrint(CardResetFail,error,frequency,bandwidthUse,txchain,rxchainuse);
   		}
   	}
   	return error;
}




static int FrequencyMinimum=2400;
static int FrequencyMaximum=6000;
static int FrequencyDefault=2412;

static int ChainMinimum=1;
static int ChainMaximum=7;
static int ChainDefault=7;

static int Ht40Default=2;

static int ResetForceDefault=1;

static struct _ParameterList Ht40Parameter[]=
{
	{0,{"no",0,0},0,0,0,0,0,0,0,0,0},
	{1,{"high",0,0},0,0,0,0,0,0,0,0,0},
	{-1,{"low",0,0},0,0,0,0,0,0,0,0,0},
	{2,{"automatic",0,0},0,0,0,0,0,0,0,0,0},
};

static struct _ParameterList ResetForceParameter[]=
{
	{0,{"no",0,0},0,0,0,0,0,0,0,0,0},
	{1,{"yes",0,0},0,0,0,0,0,0,0,0,0},
	{-1,{"automatic",0,0},0,0,0,0,0,0,0,0,0},
};

static struct _ParameterList ResetParameter[]=
{
	{LinkParameterFrequency,{"frequency",0,0},"channel carrier frequency",'u',"MHz",1,1,1,&FrequencyMinimum,&FrequencyMaximum,&FrequencyDefault,0,0},
	{LinkParameterChain,{"chain",0,0},"chain mask",'x',0,1,1,1,&ChainMinimum,&ChainMaximum,&ChainDefault,0,0},
	{LinkParameterTxChain,{"txChain",0,0},"transmit chain mask",'x',0,1,1,1,&ChainMinimum,&ChainMaximum,&ChainDefault,0,0},
	{LinkParameterRxChain,{"rxChain",0,0},"receive chain mask",'x',0,1,1,1,&ChainMinimum,&ChainMaximum,&ChainDefault,0,0},
	{LinkParameterHt40,{"ht40",0,0},"use ht40 mode",'z',0,1,1,1,0,0,&Ht40Default,
		sizeof(Ht40Parameter)/sizeof(Ht40Parameter[0]),Ht40Parameter},
	{LinkParameterReset,{"reset",0,0},"force reset",'z',0,1,1,1,0,0,&ResetForceDefault,
		sizeof(ResetForceParameter)/sizeof(ResetForceParameter[0]),ResetForceParameter},
};

static struct _ParameterList MCIResetParameter[]=
{
 	{LinkParameterFrequency,{"frequency",0,0},"channel carrier frequency",'u',"MHz",1,1,1,&FrequencyMinimum,&FrequencyMaximum,&FrequencyDefault,0,0},
}; 

static struct _ParameterList PllScreenParameter[]=
{
 	{LinkParameterAverage,{"average","avg",0},"number of measurements taken and averaged",'d',0,1,1,1,&LinkAverageMinimum,&LinkAverageMaximum,&LinkAverageDefault,sizeof(LinkAverageParameter)/sizeof(LinkAverageParameter[0]),LinkAverageParameter},
}; 


void CardResetParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(ResetParameter)/sizeof(ResetParameter[0]);
    list->special=ResetParameter;
}

void CardResetMCIParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(MCIResetParameter)/sizeof(MCIResetParameter[0]);
    list->special=MCIResetParameter;
}

int CardReset(int client)
{
    int frequency;
	int ht40;
	unsigned int txchain;
	int rxchain;
	int reset;
	int error;
	int ip, np;
	int ngot;
	char *name;
	int code;
	int index;
	int bandwidth;
	//
	// if there's no card loaded, return error
	//
    if(CardCheckAndLoad(client)!=0)
    {
		ErrorPrint(CardNoneLoaded);
		error= -1;
    }
	else
	{
		error=0;
		frequency=FrequencyDefault;
		txchain=ChainDefault;
		rxchain=ChainDefault;
		ht40=Ht40Default;			// ht40 is on, either high or low as appropriate
		reset= ResetForceDefault;		// default is force reset
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
			index=ParameterSelectIndex(name,ResetParameter,sizeof(ResetParameter)/sizeof(ResetParameter[0]));
			if(index>=0)
			{
				code=ResetParameter[index].code;
				switch(code) 
				{
					case LinkParameterFrequency:
						ngot=ParseIntegerList(ip,name,&frequency,&ResetParameter[index]);
						if(ngot<=0)
						{
							error++;
						}
						break;
					case LinkParameterHt40:
						ngot=ParseIntegerList(ip,name,&ht40,&ResetParameter[index]);
						if(ngot<=0)
						{
							error++;
						}
						break;
					case LinkParameterChain:
						ngot=ParseHexList(ip,name,&txchain,&ResetParameter[index]);
						if(ngot<=0)
						{
							error++;
						}
						else
						{
							rxchain=txchain;
						}
						break;
					case LinkParameterTxChain:
						ngot=ParseHexList(ip,name,&txchain,&ResetParameter[index]);
						if(ngot<=0)
						{
							error++;
						}
						break;
					case LinkParameterRxChain:
						ngot=ParseHexList(ip,name,&txchain,&ResetParameter[index]);
						if(ngot<=0)
						{
							error++;
						}
						break;
					case LinkParameterReset:
						ngot=ParseIntegerList(ip,name,&reset,&ResetParameter[index]);
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

		if(error==0)
		{
			SendOn(client);
			switch(ht40)
			{
				case 0:
					bandwidth = BW_HT20;
					break;
				case 1:
					bandwidth = BW_HT40_PLUS;
					break;
				case -1:
					bandwidth = BW_HT40_MINUS;
					break;
				case 2:
				default:
					bandwidth = BW_AUTOMATIC;
					break;
			}
            error=CardResetIfNeeded(frequency,txchain,rxchain,reset,bandwidth);

			SendOff(client);
		}
	}
	//
	// say we're done
	//
    SendDone(client);

	return error;
}

int MCIReset(int client)
{
    int frequency;
	int error;
	int ip, np;
	int ngot;
	char *name;
	int code;
	int index;
	//
	// if there's no card loaded, return error
	//
    if(CardCheckAndLoad(client)!=0)
    {
		ErrorPrint(CardNoneLoaded);
		error= -1;
    }
	else
	{
		error=0;
		frequency=FrequencyDefault;
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
			index=ParameterSelectIndex(name,MCIResetParameter,sizeof(MCIResetParameter)/sizeof(MCIResetParameter[0]));
			if(index>=0)
			{
				code=ResetParameter[index].code;
				switch(code) 
				{
					case LinkParameterFrequency:
						ngot=ParseIntegerList(ip,name,&frequency,&ResetParameter[index]);
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
#if ATH_SUPPORT_MCI

		/// DOLATERMCI

		if(error==0)
		{
			SendOn(client);
			if (frequency<2500)		// why 2500?
			{
				DeviceMCIReset(1);
			}
			else
			{
				DeviceMCIReset(0);
			}
			SendOff(client);
		}
#endif
	}
	//
	// say we're done
	//
    SendDone(client);

	return error;
}


#define MLABEL 30


static int CardLoadDo(int client, int devid, int calmem, char *chip)
{
	int error;
	char *dllname;

	_DeviceValid=0;

	DeviceIdUserInput=devid;

// Following block will be removed after LInux dynamic library is implemented

#ifdef DOLATER
    if(devid==AR9300_DEVID_EMU_PCIE)
    { 
        devid=AR9300_DEVID_AR9380_PCIE;
    }
#endif

#ifdef DYNAMIC_DEVICE_DLL
	//
	// try to find the appropriate HAL dll and load it.
	//
	dllname=0;
	if(chip==0 || Smatch(chip,""))
	{
		if (devid>0)	
		// load with specified devid, and the devid is nart hardcoded to match with a HAL dll
			dllname=DevidToLibrary(devid);
		else
		// search through a nart hardcoded HAL dll list to find the HAL that support chip
			dllname=SearchLibrary(&devid);
	}
	else
	{
		dllname=LoadDLL(chip,&devid);
	}
	if(Smatch(dllname,""))
	{
		ErrorPrint(CardChipUnknown);
		return -CardChipUnknown;
	}
#else
	error=ChipSelect(devid);
	if(error!=0)
	{
		return error;
	}
#endif
	DeviceDataSend();

	/*
	* For McKinley project, save current calmem into SetConfigHashTable.
	* Since ART GUI use "load calmem=auto" as default, McKinley can't use calmem=auto.
	* If calmem=auto, McKinley uses setconfig.cal_mem instead of calmem=auto. 
	*/
    if (((devid == AR6004_DEVID)||(devid == AR6006_DEVID)) && (calmem != 0))
	{
		char buffer[4];
		char parameter[16];
		memset(buffer,0, sizeof(buffer));
		memset(parameter,0, sizeof(parameter));
		sprintf(buffer, "%d", calmem);
		sprintf(parameter, "%s", "CAL_MEMORY ");
	    SetConfigSet(parameter, strlen(parameter), buffer, strlen(buffer));
    }
	// set template preference here
	if(npreference==1)
    {
		DeviceEepromTemplatePreference(preference);
	}

	//
	// attach to the device
	//
	error=DeviceAttach(devid,calmem);
	if(error==0)
	{	
		char buffer[MBUFFER];
    		SformatOutput(buffer,MBUFFER-1,"|set|IniVersion|%s|",DeviceIniVersion(devid));
		ErrorPrint(NartData,buffer);
        	ChannelCalculate();
		CalibrateClear();
		_DeviceValid=1;
	}

	return error;
}
 

static void CardLoadDataSend(int client)
{
    int devid;
	char buffer[MBUFFER];
	unsigned char macaddr[6];
    char label[MLABEL];
	int txChainNum, rxChainNum;
	int mode;
    //
    // Send any other parameter that we want cart to know about.
    // |set| ... data responses are intercepted in cart and converted to $variables which are 
    // user accessible for reports and limits and other fun stuff.
    //
    if(DeviceIdUserInput > 0)
    {
        devid = DeviceIdUserInput;
    }
    else
    {
        devid=DeviceIdGet();
    }
	SformatOutput(buffer,MBUFFER-1,"|set|devid|%04x|",devid);
    ErrorPrint(NartData,buffer);

    DeviceMacAddressGet(macaddr);
    if(macaddr[0]!=0 || macaddr[1]!=0 || macaddr[2]!=0 || macaddr[3]!=0 || macaddr[4]!=0 || macaddr[5]!=0)
    {
	    SformatOutput(buffer,MBUFFER-1,"|set|mac|%02x:%02x:%02x:%02x:%02x:%02x|",
		    macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
        buffer[MBUFFER-1]=0;
        ErrorPrint(NartData,buffer);
    }
    else
    {
	    SformatOutput(buffer,MBUFFER-1,"|set|mac||");
        buffer[MBUFFER-1]=0;
        ErrorPrint(NartData,buffer);
    }

    DeviceCustomerDataGet((unsigned char *)label, MLABEL);
    label[MLABEL-1]=0;
    if(label[0]!=0)
    {
	    SformatOutput(buffer,MBUFFER-1,"|set|customer|%s|",label);
        buffer[MBUFFER-1]=0;
        ErrorPrint(NartData,buffer);
    }
    else
    {
	    SformatOutput(buffer,MBUFFER-1,"|set|customer||");
        buffer[MBUFFER-1]=0;
        ErrorPrint(NartData,buffer);
    }

	txChainNum = DeviceTxChainMany();
    if(txChainNum!=0)
    {
	    SformatOutput(buffer,MBUFFER-1,"|set|txChains|%d|",txChainNum);
        buffer[MBUFFER-1]=0;
        ErrorPrint(NartData,buffer);
    }
    else
    {
	    SformatOutput(buffer,MBUFFER-1,"|set|txChains||");
        buffer[MBUFFER-1]=0;
        ErrorPrint(NartData,buffer);
    }

	rxChainNum = DeviceRxChainMany();
    if(rxChainNum!=0)
    {
	    SformatOutput(buffer,MBUFFER-1,"|set|rxChains|%d|",rxChainNum);
        buffer[MBUFFER-1]=0;
        ErrorPrint(NartData,buffer);
    }
    else
    {
	    SformatOutput(buffer,MBUFFER-1,"|set|rxChains||");
        buffer[MBUFFER-1]=0;
        ErrorPrint(NartData,buffer);
    }

	mode = DeviceIs2GHz();
    SformatOutput(buffer,MBUFFER-1,"|set|2GHz|%d|",mode);
    buffer[MBUFFER-1]=0;
    ErrorPrint(NartData,buffer);

	if (mode == 0)
	{
		// set frequency to 5GHz if 2GHz is not supported
		FrequencyDefault = 5180;
	}

	mode = DeviceIs5GHz();
    SformatOutput(buffer,MBUFFER-1,"|set|5GHz|%d|",mode);
    buffer[MBUFFER-1]=0;
    ErrorPrint(NartData,buffer);

	mode = DeviceIs4p9GHz();
    SformatOutput(buffer,MBUFFER-1,"|set|4p9GHz|%d|",mode);
    buffer[MBUFFER-1]=0;
    ErrorPrint(NartData,buffer);

	mode = DeviceIsHalfRate();
    SformatOutput(buffer,MBUFFER-1,"|set|HalfRate|%d|",mode);
    buffer[MBUFFER-1]=0;
    ErrorPrint(NartData,buffer);

	mode = DeviceIsQuarterRate();
    SformatOutput(buffer,MBUFFER-1,"|set|QuarterRate|%d|",mode);
    buffer[MBUFFER-1]=0;
    ErrorPrint(NartData,buffer);
}

enum
{
	LoadHelp=0,
	LoadDevid,
	LoadSsid,
	LoadBus,
	LoadChip,
};

static int DeviceIdDefault= ChipUnknown;
//static int CardDevidDefault= ChipUnknown;


static struct _ParameterList LoadParameter[]=
{
	{LoadDevid,{"devid",0,0},"device type",'x',0,0,0,0,0,0,&DeviceIdDefault,0,0},
	{LoadChip,{"chip","dll",0},"chip type",'x',0,0,0,0,0,0,0,0,0},
	TEMPLATE_PREFERENCE,
	TEMPLATE_MEMORY_READ,
	TEMPLATE_SIZE_READ,
};

void CardLoadParameterSplice(struct _ParameterList *list)
{
    list->nspecial=sizeof(LoadParameter)/sizeof(LoadParameter[0]);
    list->special=LoadParameter;
//	ChipDevidParameterSplice(&LoadParameter[0]);
}

int CardLoad(int client)
{
    unsigned int devid;
	int ndevid;
	int error;
	int lc;
	int ip, np;
	char *name;
	int code;
    int calmem,ncalmem;
	int address,naddress;
	int index;
	char *chip;
	//
	// prepare beginning of error message in case we need to use it
	//
	lc=0;
    error=0;
	//
	// install default values
	//
	ndevid= -1;
	ncalmem= -1;
	naddress= -1;
	npreference= -1;
	devid=DeviceIdDefault;
    calmem=TemplateMemoryDefaultRead;
	address=TemplateSizeDefaultRead;
	preference=TemplatePreferenceDefault;
	chip=0;
	//
	// parse parameters and values
	//
	np=CommandParameterMany();
	for(ip=0; ip<np; ip++)
	{
		name=CommandParameterName(ip);
		index=ParameterSelectIndex(name,LoadParameter,sizeof(LoadParameter)/sizeof(LoadParameter[0]));
		if(index>=0)
		{
			code=LoadParameter[index].code;
			switch(code) 
			{

				case LoadDevid:
					ndevid=ParseHexList(ip,name,&devid,&LoadParameter[index]);
					if(ndevid<=0)
					{
						error++;
					}
					break;
				case LoadChip:
					chip=CommandParameterValue(ip,0);
					break;
				case TemplateMemory:
					ncalmem=ParseIntegerList(ip,name,&calmem,&LoadParameter[index]);
					if(ncalmem<=0)
					{
						error++;
					}
					break;
				case TemplateSize:
					naddress=ParseIntegerList(ip,name,&address,&LoadParameter[index]);
					if(naddress<=0)
					{
						error++;
					}
					break;
				case TemplatePreference:
					npreference=ParseIntegerList(ip,name,&preference,&LoadParameter[index]);
					if(npreference<=0)
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

#ifdef DOLATERK31
    if(DeviceIdUserInput == AR9300_DEVID_AR9340)
    {
        ar9300_eeprom_template_preference(ar9300_eeprom_template_wasp_k31);
    }
#endif
   

	if(error==0)
	{
	    if(_DeviceValid==0)
	    {
		    SendOn(client);

			// Why are the Devicexxx functions are called here while the device funtion table is not set?
			// They don't do anything at all
			if(npreference==1)
			{
				DeviceEepromTemplatePreference(preference);
			}
			if(naddress==1)
			{
				if(address>0)
				{
					address--;
				}
				DeviceCalibrationDataAddressSet(address);
			}
			if(ncalmem==1)
			{
				DeviceCalibrationDataSet(calmem);
			}
		    error=CardLoadDo(client,devid,calmem,chip);

		    SendOff(client);
	    }
	    else
	    {
		    //
		    // we do this because the perl parsing expects these messages
		    // when that is fixed, we can remove these two responses
		    //
		    SendOn(client);
            error=0;
		    SendOff(client);
	    }

        if(error==0)
        {
#ifdef UNUSED
            DeviceMacAddressGet(macaddr);
            if(DeviceIdUserInput > 0)
            {
                devid=DeviceIdUserInput;
            }
            else
            {
	            devid=DeviceIdGet();
            }
            //
	        // return information to cart
	        //
	        ErrorPrint(NartDataHeader,"|load|devid|memaddr|memsize|mac|");
			SformatOutput(buffer,MBUFFER-1,"|load|%x|%lx|%x|%02x:%02x:%02x:%02x:%02x:%02x|",
		        devid,
                AnwiDriverMemoryMap(),
		        AnwiDriverMemorySize(),		
		        macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
            ErrorPrint(NartData,buffer);
#endif
            //
            // Send normal card data back to cart
            //
            CardLoadDataSend(client);
        }
        else
        {
            //
            // Send bad card data back to cart
            //
            CardUnloadDataSend(client);
        }
    }
	//
	// announce success
	//
	if(error==0)
	{
	    FreeMemoryPrint();
		ErrorPrint(CardLoadGood);
	}
	//
	// say we're done
	//
	SendDone(client);

    return error;
}

//
// Returns 1 is there is a valid card loaded and ready for operation.
//
int CardValid(void)
{
    return (_DeviceValid && DeviceValid());
}


//
// Returns 1 is there is a valid card loaded and ready for operation.
//
int CardValidReset(void)
{
    return CardValid() & ResetDone;
}


int CardDataSend(int client)
{
    if(CardValid())
    {
        //
        // Send normal card data back to cart
        //
        CardLoadDataSend(client);
	    return 0;
    }
    else
    {
        //
        // Send normal card data back to cart
        //
        CardUnloadDataSend(client);
        return -1;
    }
}

void DeviceDataSend(void)
{
    char buffer[MBUFFER];

    SformatOutput(buffer,MBUFFER-1,"|set|DeviceName|%s|",DeviceName());
    ErrorPrint(NartData,buffer);
	SformatOutput(buffer,MBUFFER-1,"|set|DeviceVersion|%s|",DeviceVersion());
    ErrorPrint(NartData,buffer);
    SformatOutput(buffer,MBUFFER-1,"|set|DeviceBuildDate|%s|",DeviceBuildDate());
    ErrorPrint(NartData,buffer);

#ifdef DYNAMIC_DEVICE_DLL
    SformatOutput(buffer,MBUFFER-1,"|set|DeviceLibrary|%s|",DeviceFullName());
#else
    SformatOutput(buffer,MBUFFER-1,"|set|DeviceLibrary|%s|","");
#endif    
    ErrorPrint(NartData,buffer);
}


int CardCheckAndLoad(int client)
{
	//
	// if there's no card loaded, try to load it
    // if unsuccessful, return error
	//
	if(!CardValid())
	{
        CardLoadDo(client,ChipUnknown,0,0);
        if(CardValid())
        {
            //
            // Send normal card data back to cart
            //
            CardLoadDataSend(client);
		    return 0;
        }
        else
        {
            //
            // Send normal card data back to cart
            //
            CardUnloadDataSend(client);
            return -1;
        }
	}
    return 0;
}

