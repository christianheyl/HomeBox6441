/*
 *  Copyright ?2005 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#ifdef VXWORKS
#include "vxworks.h"
#endif

#ifdef __ATH_DJGPPDOS__
#define __int64 long long
typedef unsigned long DWORD;
#define Sleep   delay
#endif  // #ifdef __ATH_DJGPPDOS__

#include <errno.h>
#include <assert.h>


//#include "mIds.h"
#include <stdio.h>
//
// hal header files
//
#include "ah.h"
#include "ah_internal.h"
#include "ar5416.h"
#include "ar5416eep.h"
#include "ar5416desc.h"

#include "ah_osdep.h"
#include "opt_ah.h"

#ifdef __APPLE__
#include "osdep.h"
#endif

#include "ah_devid.h"
#include "ar5416reg.h"
#include "ar5416eep.h"
#include "wlantype.h"
//#include "mData.h"
#include "rate_constants.h"
#include "vrate_constants.h"

#include "MyDelay.h"
#include "ParameterSelect.h"
#include "Card.h"
#include "UserPrint.h"
#include "StructPrint.h"
#include "GetSet.h"

#include "Device.h"
#include "AnwiDriverInterface.h"

#include "mData5416.h"
#include "mCal5416.h"
#include "Ar9287Version.h"
#include "ar5416reg.h"

#include "ErrorPrint.h"
#include "CardError.h"
#include "TimeMillisecond.h"

#define MBUFFER 1000

#include "Sticky.h"

#include "LinkList.h"

#ifdef DYNAMIC_DEVICE_DLL
#include "LinkLoad.h"
#else
#include "DescriptorLink.h"
#endif

#include "Ar9287Version.h"
#include "Ar9287Device.h"
#include "Ar5416TxDescriptor.h"
#include "Ar5416RxDescriptor.h"
#include "Ar5416Field.h"

#include "AnwiDriverInterface.h"

#include "Ar9287EepromSave.h"
#include "Ar9287Field.h"
#include "Ar9287NoiseFloor.h"
#include "Ar9287PcieConfig.h"
#include "Ar9287TargetPower.h"
#include "Ar9287Temperature.h"
#include "AR9287ChipIdentify.h"
#include "mEepStruct9287.h"
#include "Ar9287EepromStructGet.h"
#include "Ar9287EepromStructSet.h"
#include "Ar9287NoiseFloor.h"
#include "ar5416/ar5416devicefunction.h"
#include "Ar9287ConfigurationCommand.h"
#include "ar9287eep.h"

#include "NartRegister.h"

typedef enum {
    CALDATA_AUTO=0,
    CALDATA_EEPROM,
    CALDATA_FLASH,
    CALDATA_OTP
} CALDATA_TYPE;

#define LinkDllName "linkAr9k"

// 
// this is the hal pointer, 
// returned by ath_hal_attach
// used as the first argument by most (all?) HAL routines
//
struct ath_hal *AH=0;
AR9287DLLSPEC unsigned long mac_version;
int calData = CALDATA_AUTO;

int Ar9287ChannelCalculate(int *frequency, int *option, int mchannel)
{
#define MCHANNEL 2000
	int nchannel;
	HAL_CHANNEL halchannel[MCHANNEL];
   	u_int32_t modeSelect;
    HAL_BOOL enableOutdoor;
   	HAL_BOOL enableExtendedChannels;
   	u_int16_t *regDmn;
    u_int8_t regclassids[MCHANNEL];
    u_int maxchans, maxregids, nregids;
    HAL_CTRY_CODE cc;
   	int error;
	int it;

   	//
   	// try calling this here
   	//
   	//Fcain need to handle this better, should not be accessing struct values directly
//   	regDmn = ar5416RegulatoryDomainGet(AH);
//   	if( regDmn[0] != 0) 
   	{
        ar5416RegulatoryDomainOverride(AH, 0);
   	}		
//   	if(regDmn[0] & 0x8000)
 //  		cc=regDmn[0] & 0x3ff;
//   	else
   		cc=CTRY_DEBUG;		//CTRY_DEFAULT;			// changed from CTRY_DEBUG since that doesn't work if regdmn is set
   	modeSelect=0xffffffff;
    enableOutdoor=0;
   	enableExtendedChannels=0;
    maxchans=sizeof(halchannel)/sizeof(halchannel[0]);
   	maxregids=sizeof(regclassids)/sizeof(regclassids[0]);
   
    error=ath_hal_init_channels(AH,
                 halchannel, maxchans, &nchannel,
                 regclassids, maxregids, &nregids,
                 cc, modeSelect,
                 enableOutdoor, enableExtendedChannels);
   
   	if(error==0)
   	{
   		ErrorPrint(CardLoadNoChannel);
   		return -5;
   	}
	if(nchannel>mchannel)
	{
		nchannel=mchannel;
	}
	for(it=0; it<nchannel; it++)
	{
		frequency[it]=halchannel[it].channel;
		option[it]=halchannel[it].channel_flags;
	}
   
   	return nchannel;
}



static int _Ar9287ReceiveFifo=0;
static int _Ar9287ReceiveDescriptorMaximum=-1;
static int _Ar9287ReceiveEnableFirst=0;
static int _Ar9287TransmitFifo=0;
static int _Ar9287TransmitDescriptorSplit=0;
static int _Ar9287TransmitAggregateStatus=1;
static int _Ar9287TransmitEnableFirst=0;


bool ar9287_calibration_data_read_flash(struct ath_hal *ah, long address,
    u_int8_t *buffer, int many)
{
#ifdef ART_BUILD
#ifdef MDK_AP          /* MDK_AP is defined only in NART AP build */

    int fd;
#ifdef K31_CALDATA
    extern int ath_k31_mode;
#endif

#ifdef K31_CALDATA
    extern ath_k31_mode;
    if (ath_k31_mode)
    {
        FILE *fp;
        if (instance == 0) {
            fp = fopen("/tmp/caldata2g", "rb");
        } else {
            fp = fopen("/tmp/caldata5g", "rb");
        }
        if (fp == NULL) {
            printf("%s[%d] Uncalibrated board.\n", __func__, __LINE__);
            return false;
        }
        fseek(fp, address, SEEK_SET);
        fread(buffer, many, 1, fp);
        fclose(fp);
        return true;
    }
#endif

    if ((fd = open("/dev/caldata", O_RDWR)) < 0) {
        perror("Could not open flash\n");
        return 1 ;
    }
    lseek(fd, address, SEEK_SET);
    if (read(fd, buffer, many) != many) {
        perror("\n_read\n");
        close(fd);
        return 1;
    }
    close(fd);                

    return true;
#endif   /* MDK_AP */
#endif
    return false;
}

AR9287DLLSPEC int Ar9287FlashRead(unsigned int address, unsigned char *buffer, int many)
{
#ifdef MDK_AP
    if(!ar9287_calibration_data_read_flash(AH, address, (unsigned char*)buffer, many ))// return 0 if eprom read is correct; ar9300EepromRead returns 1 for success
		return 1; // bad read

    return 0;
#else
	return 1;
#endif
}

AR9287DLLSPEC int Ar9287FlashWrite(unsigned int address, unsigned char *buffer, int many)
{
    unsigned int eepAddr;
    unsigned int byteAddr;
    unsigned short svalue, word;
    int i;

#ifdef MDK_AP
    int fd, it;
    if((fd = open("/dev/caldata", O_RDWR)) < 0) {
        perror("Could not open flash\n");
        return 1 ;
    }
    lseek(fd, address, SEEK_SET);
    if (write(fd, buffer, many) != many) {
            perror("\nwrite\n");
            return 1;
    }
	close(fd);
    return 0;
#else
	return 1;
#endif
}

int Ar9287ReceiveFifo(void)
{
	return _Ar9287ReceiveFifo;
}

int Ar9287ReceiveDescriptorMaximum(void)
{
	return _Ar9287ReceiveDescriptorMaximum;
}

int Ar9287ReceiveEnableFirst(void)
{
	return _Ar9287ReceiveEnableFirst;
}

int Ar9287TransmitFifo(void)
{
	return _Ar9287TransmitFifo;
}

int Ar9287TransmitDescriptorSplit(void)
{
	return _Ar9287TransmitDescriptorSplit;
}

int Ar9287TransmitAggregateStatus(void)
{
	return _Ar9287TransmitAggregateStatus;
}

int Ar9287TransmitEnableFirst(void)
{
	return _Ar9287TransmitEnableFirst;
}


int Ar9287Detach(void)
{
	return AnwiDriverDetach(); 
}

int Ar9287Valid(void)
{
	return AnwiDriverValid();
}

int Ar9287DeviceIdGet(void)
{
	return AnwiDriverDeviceIdGet();
}


#include "Ar9287EepromList.h"


int Ar9287MacAddressGet(unsigned char *mac)
{
    ar9287_eeprom_t *eeprom;
	struct ath_hal_5416 *ahp;

	ahp=AH5416(AH);
	if(AH!=0 && ahp!=0)
	{
        eeprom=(ar9287_eeprom_t *)&ahp->ah_eeprom;
		mac[0]=eeprom->baseEepHeader.macAddr[0];
		mac[1]=eeprom->baseEepHeader.macAddr[1];
		mac[2]=eeprom->baseEepHeader.macAddr[2];
		mac[3]=eeprom->baseEepHeader.macAddr[3];
		mac[4]=eeprom->baseEepHeader.macAddr[4];
		mac[5]=eeprom->baseEepHeader.macAddr[5];
	}
	return 0;
}

int Ar9287CustomerDataGet(unsigned char *data, int len)
{
    ar9287_eeprom_t *eeprom;
	struct ath_hal_5416 *ahp;
	int it;

	ahp=AH5416(AH);
	if(AH!=0 && ahp!=0)
	{
        eeprom=(ar9287_eeprom_t *)&ahp->ah_eeprom;
		for(it=0; it<len && it<AR9287_DATA_SZ; it++)
		{
		    data[it]=eeprom->custData[it];
			if(data[it]==0)
			{
				break;
			}
		}
	}
	data[len-1]=0;
	return 0;
}

int Ar9287Attach(int devid, int calmem)
{
    HAL_ADAPTER_HANDLE osdev;
	HAL_SOFTC sc;
	HAL_BUS_TAG st;
	HAL_BUS_HANDLE sh;
	HAL_BUS_TYPE bustype;
    struct hal_reg_parm hal_conf_parm;
	HAL_STATUS error;
	struct ath_hal_5416 *ahp;
	const u_int32_t *header;

	int start,end;
	int it;
    char buffer[MBUFFER];
	int caluse;
	int eepsize;
    int status;

	int npcie=0;

	//
	// connect to the ANWI driver
	//
 	status=AnwiDriverAttach(devid);  
 	if(status<0)  
 	{ 
	    ErrorPrint(CardLoadAnwi); 
	    return -2; 
 	} 
	//
    //
    // AND THEN WE CONFIGURE THE HAL CODE.
    // THIS SHOULD REALLY BE IN DEVICE SPECIFC FUNCTIONS UNDER THE NART CODE
    //
	//
	// attach to the HAL, pass it the register memory address from ANWI
	//
	osdev=0;
	sc=0;																// wmi_handle???
	st=0;																// bsd only?
	sh=(HAL_BUS_HANDLE)AnwiDriverRegisterMap();	// register map
	bustype=0;				
	// this stuff shouldn't be here. move into the hal as default values if conf_parm=0
	//
 #ifndef MDK_AP
 	hal_conf_parm.calInFlash = 0;	 //use EEPROM
 #else
 	hal_conf_parm.calInFlash = 1;	 //use FLASH
 #endif
	hal_conf_parm.forceBias=0;
    hal_conf_parm.forceBiasAuto=0;
    hal_conf_parm.halPciePowerSaveEnable=0;
    hal_conf_parm.halPcieL1SKPEnable=0;
    hal_conf_parm.halPcieClockReq=0;
    hal_conf_parm.halPciePowerReset=0x100;
    hal_conf_parm.halPcieWaen=0;
    hal_conf_parm.halPcieRestore=0;
    hal_conf_parm.htEnable=1;
//    hal_conf_parm.disableTurboG=0;
    hal_conf_parm.ofdmTrigLow=200;
    hal_conf_parm.ofdmTrigHigh=500;
    hal_conf_parm.cckTrigHigh=200;
    hal_conf_parm.cckTrigLow=100;
    hal_conf_parm.enableANI=1;
    hal_conf_parm.noiseImmunityLvl=4;
    hal_conf_parm.ofdmWeakSigDet=1;
    hal_conf_parm.cckWeakSigThr=0;
    hal_conf_parm.spurImmunityLvl=2;
    hal_conf_parm.firStepLvl=0;
    hal_conf_parm.rssiThrHigh=40;
    hal_conf_parm.rssiThrLow=7;
    hal_conf_parm.diversityControl=0;
    hal_conf_parm.antennaSwitchSwap=0;
//    for (it=0; it< AR_EEPROM_MODAL_SPURS; it++) 
//	{
//        hal_conf_parm.ath_hal_spurChans[it][0] = 0;
//        hal_conf_parm.ath_hal_spurChans[it][1] = 0;
//    }
    hal_conf_parm.serializeRegMode=0;
    hal_conf_parm.defaultAntCfg=0;
    hal_conf_parm.fastClockEnable=1;
    hal_conf_parm.hwMfpSupport=0;

//    hal_conf_parm.ath_hal_enableMSI=0;

	start=TimeMillisecond();
	error=0;
	AH=ar5416Attach((unsigned short)devid, osdev, sc, st, sh, bustype, NULL /* amem_handle */, &hal_conf_parm, &error);
	ar5416_calibration_data_set(AH,calmem);	//move here to get AH correctly
	if(error!=0)
	{
		ErrorPrint(CardLoadAttach,error);
		return error;
	}
	if(AH==0)
	{
		ErrorPrint(CardLoadHal);
		return -4;
	}

	mac_version = AH_PRIVATE(AH)->ah_mac_version;
	end=TimeMillisecond();

	//
	// figure out where the calibration memory really is
	//
    caluse=ar5416_calibration_data_get(AH);
	switch(caluse)
	{
		case calibration_data_none:
			//ErrorPrint(CardLoadCalibrationNone);
			eepsize=ar5416_eeprom_size(AH);
			if(eepsize>0)
			{
				ar5416_calibration_data_set(AH,calibration_data_eeprom);
			}
			else if(ar9287_calibration_data_read_flash(AH, 0x5100, header, 1)==AH_TRUE)
			{
				ar5416_calibration_data_set(AH,calibration_data_flash);
			}
			else
			{
				ar5416_calibration_data_set(AH,calibration_data_otp);
			}
			break;
		case calibration_data_flash:
			ErrorPrint(CardLoadCalibrationFlash);
			break;
		case calibration_data_eeprom:
			ErrorPrint(CardLoadCalibrationEeprom,ar5416_calibration_data_address_get(AH));
			break;
	}

	//
	// THIS LIST NEEDS TO BE COMPLETE AND ACCURATE
	// WOULD LIKE IT TO BE INSIDE THE HAL ATTACH (OR REPLACE WITH EQUIVALENT HAL FUNCTIONS)
	//
	_Ar9287ReceiveFifo=0;
	_Ar9287ReceiveDescriptorMaximum=-1;
	_Ar9287ReceiveEnableFirst=0;
	_Ar9287TransmitFifo=0;
	_Ar9287TransmitDescriptorSplit=0;
	_Ar9287TransmitAggregateStatus=1;
	_Ar9287TransmitEnableFirst=0;

	if(npcie==0) {
		if (Ar9287pcieDefault()<0) {
			ErrorPrint(CardLoadDevid,devid);
			return error;
		}
	}

	error=0;
    switch (devid) 
	{
		case AR5416_DEVID_AR9287_PCI:			// 0x002d
		case AR5416_DEVID_AR9287_PCIE:			// 0x002e
			ahp=AH5416(AH);
			GetSetDataSetup(Ar9287EepromList, sizeof(Ar9287EepromList)/sizeof(Ar9287EepromList[0]),
				(unsigned char *)&ahp->ah_eeprom,sizeof(ar9287_eeprom_t));
		    Ar9287_FieldSelect();
			npcie=Ar9287pcieAddressValueDataInit();
			if(npcie==0)
			{
				npcie=Ar9287pcieDefault();
			}
			break;

		default:
			ErrorPrint(CardLoadDevid,devid);
			error=-1;
			break;
	}

	if(error==0)
	{
	}

	return error;
}


int Ar9287Reset(int frequency, unsigned int txchain, unsigned int rxchain, int bandwidth)
{
	HAL_CHANNEL channel;
	int start,end;
	int error;
	HAL_OPMODE opmode;
	HAL_HT_MACMODE htmode;
    HAL_HT_EXTPROTSPACING exprotspacing;
	HAL_BOOL bChannelChange;
	int isscan;
	int tnf[1];
	//
	// do it
	//
	start=TimeMillisecond();
	//
	// station card
	// how do we do AP??
	//
	opmode=HAL_M_STA;
    //
	// channel parameters
	// are we really allowed to (have to) set all of this?
	//
    channel.channel=frequency;        /* setting in Mhz */

	if(bandwidth==BW_QUARTER || bandwidth==BW_HALF || bandwidth==BW_HT20 || bandwidth==BW_OFDM)
	{
		htmode=HAL_HT_MACMODE_20;
	}
	else
	{
		htmode=HAL_HT_MACMODE_2040;
	}
	if(frequency<4000)		// let's presume this is B/G
	{
	    if(bandwidth==BW_AUTOMATIC || bandwidth==BW_HT40_PLUS)
		{
		    channel.channel_flags=CHANNEL_2GHZ|CHANNEL_HT40PLUS;
		}
	    else if(bandwidth==BW_HT40_MINUS)
		{
		    channel.channel_flags=CHANNEL_2GHZ|CHANNEL_HT40MINUS;
		}
		else
		{
            channel.channel_flags=CHANNEL_2GHZ|CHANNEL_HT20; 
		}
	}
	else
	{
	    if(bandwidth==BW_HALF)
		{
			channel.channel_flags=CHANNEL_5GHZ|CHANNEL_HALF|CHANNEL_OFDM;
		}
		else if(bandwidth==BW_QUARTER)
		{
			channel.channel_flags=CHANNEL_5GHZ|CHANNEL_QUARTER|CHANNEL_OFDM;
		}
		else if (bandwidth==BW_OFDM) 
		{
			channel.channel_flags = CHANNEL_5GHZ|CHANNEL_OFDM;
		}
	    else if(bandwidth==BW_AUTOMATIC || bandwidth==BW_HT40_PLUS)
		{
		    channel.channel_flags=CHANNEL_5GHZ|CHANNEL_HT40PLUS;
		}
	    else if(bandwidth==BW_HT40_MINUS)
		{
		    channel.channel_flags=CHANNEL_5GHZ|CHANNEL_HT40MINUS;
		}
		else
		{
            channel.channel_flags=CHANNEL_5GHZ|CHANNEL_HT20; 
		}
	}    
	
	channel.priv_flags=CHANNEL_DFS_CLEAR;
    channel.max_reg_tx_power=27;  /* max regulatory tx power in dBm */
    channel.max_tx_power=2*27;     /* max true tx power in 0.5 dBm */
    channel.min_tx_power=0;     /* min true tx power in 0.5 dBm */
    channel.regClassId=0;     /* regulatory class id of this channel */

	exprotspacing=HAL_HT_EXTPROTSPACING_20;

	bChannelChange=0;           // fast channel change is broken in HAL/osprey

	error=0;
	isscan=0;
	if(AH!=0 && AH->ah_reset!=0)
	{
	    (*AH->ah_reset)(AH,opmode,&channel,htmode,txchain,rxchain,exprotspacing,bChannelChange,(HAL_STATUS*)&error,isscan);

		if(error==HAL_FULL_RESET)
		{
			error=0;
		}

        if(error==0)
		{
			StickyExecute(DEF_LINKLIST_IDX);
		}
	}
	else
	{
		error= -1;
	}
	end=TimeMillisecond();
	return error;
}

static int Ar9287TxChainMany(void)
{
	return 2;		
}

static int Ar9287RxChainMany(void)
{
	return 2;		
}

static int Ar9287is2GHz(void)
{
	return 1;		
}

static int Ar9287is5GHz(void)
{
	return 0;		
}

static int Ar9287is4p9GHz(void)
{
	return 0;		
}

static int Ar9287HalfRate(void)
{
	return 1;
}

static int Ar9287QuarterRate(void)
{
	return 1;
}

static int CalibrationTxGain;
static double CalibrationTxPower;

#define AR5416_PWR_TABLE_OFFSET  -5

#define SCALE_GAIN_HT20_7_REG (A_UINT32)(0xa398)
#define SCALE_GAIN_MASK       (A_UINT32)(0x1F)
#define PCDAC_PAL_OFF 44
#define PCDAC_PAL_ON    24

static A_UINT32 tempSensRef=0;

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

static int Ar9287CalibrationPierSet(int pierIdx, int freq, int chain, int pwrCorrection, int voltMeas, int tempMeas)
{
    A_INT16 i;
    double power_to_store_t4;
    ar9287_eeprom_t *peep9287;  
    int tempSensSlope;

    A_INT16  refPwr;
    A_INT16  pwrSteps;
    A_UINT32 scale;
    int8_t    pwrTableOffset;
//    int pcdac;
    ar9287_eeprom_t *eeprom;
	struct ath_hal_5416 *ahp;
	double power;

    /* that means the temp of the first one must be meaningful*/
    if(tempMeas !=0 )
    {
        tempSensRef=tempMeas;
    } 

	if(tempSensRef !=0)
	{
		tempMeas=tempSensRef;
	}

	ahp=AH5416(AH);
    peep9287=(ar9287_eeprom_t *)&ahp->ah_eeprom;

    tempSensSlope=peep9287->baseEepHeader.tempSensSlope;
       
//    power_to_store_t4 = (A_INT16)((CalibrationTxPower * 4) + 0.5);
//    power_to_store_t4 -= AR5416_PWR_TABLE_OFFSET * 4;

    power = CalibrationTxPower+(double)(((double)tempMeas - (double)tempSensRef)*(double)4.0/((double)tempSensSlope*(double)2.0));
    pwrTableOffset=peep9287->baseEepHeader.pwrTableOffset;
    pwrSteps = (A_INT16)Round((power-(double)pwrTableOffset)*2);
    /* get the desired scale gain */
    scale = (OS_REG_READ(AH, SCALE_GAIN_HT20_7_REG)>>5) & SCALE_GAIN_MASK;
    //scale = (OS_REG_READ(AH, 0xb398)>>5) & SCALE_GAIN_MASK;

    refPwr = (A_INT16)(pwrSteps - (A_INT16)PCDAC_PAL_OFF + (A_INT16)scale);

    if(pierIdx>=0 && pierIdx<AR9287_NUM_2G_CAL_PIERS && chain>=0 && chain<AR9287_MAX_CHAINS)
    {
		peep9287->calFreqPier2G[pierIdx]=freq-2300;
        peep9287->calPierData2G[chain][pierIdx].calDataOpen.pwrPdg[0][0]=refPwr;
        peep9287->calPierData2G[chain][pierIdx].calDataOpen.vpdPdg[0][0]=tempMeas;
        peep9287->calPierData2G[chain][pierIdx].calDataOpen.pcdac[0][0]=CalibrationTxGain;    
	}
    return 0;
}


static int Ar9287CalibrationPower(int txgain, double txpower)
{
	CalibrationTxGain=txgain;
	CalibrationTxPower=txpower;

    return 0;
}

static double Ar9287GainTableOffset(void)
{
	return -5;
}

static int Ar9287EepromSize(void)
{
	return sizeof(ar9287_eeprom_t);
}

int Ar9287IsEmbeddedArt(void)
{
    return 0;
}

int Ar9287_RegulatoryDomainOverride(unsigned int regdmn)
{
	ar5416RegulatoryDomainOverride(AH, (u_int16_t)regdmn);
    return AH_TRUE;
}

int Ar9287_RegulatoryDomainGet(void)
{
	ar5416RegulatoryDomainGet(AH);
    return AH_TRUE;
}

static struct _DeviceFunction _Ar9287Device=
{
	Ar9287ChipIdentify,				// need something here
	Ar9287Name,
	Ar9287Version,
	Ar9287BuildDate,

	Ar9287Attach,
	Ar9287Detach,
	Ar9287Valid,
	Ar9287DeviceIdGet,
	Ar9287Reset,
    Ar9287SetCommand,				//SetCommand,
    Ar9287SetParameterSplice,		//SetParameterSplice,
    Ar9287GetCommand,				//GetCommand,
    Ar9287GetParameterSplice,		//GetParameterSplice,
    Ar5416BssIdSet, 
	Ar5416StationIdSet,

    Ar5416ReceiveDescriptorPointer,
    Ar5416ReceiveUnicast,
    Ar5416ReceiveBroadcast,
    Ar5416ReceivePromiscuous,
    Ar5416ReceiveEnable,
    Ar5416ReceiveDisable,
    Ar9287Deaf,

    Ar9287ReceiveFifo,
    Ar9287ReceiveDescriptorMaximum,
    Ar9287ReceiveEnableFirst,

    Ar9287TransmitFifo,
    Ar9287TransmitDescriptorSplit,
    Ar9287TransmitAggregateStatus,
    Ar9287TransmitEnableFirst,

    0,										// Ar5416TransmitDescriptorStatusPointer,
    Ar5416TransmitDescriptorPointer,
    0,										// Ar5416TransmitRetryLimit,
    Ar5416TransmitQueueSetup,
    Ar5416TransmitRegularData,
    Ar5416TransmitFrameData,
    Ar5416TransmitContinuousData,
    Ar5416TransmitCarrier,
    Ar5416TransmitEnable,
    Ar5416TransmitDisable,

    Ar5416TransmitPowerSet,
    Ar5416TransmitGainSet,
    Ar5416TransmitGainRead,
    Ar5416TransmitGainWrite,

    Ar9287EepromRead,
    Ar9287EepromWrite,
    0,										// Ar5416OtpRead,
    0,										// Ar5416OtpWrite,
	MyMemoryBase,
	MyMemoryPtr,
    MyMemoryRead,
    MyMemoryWrite,
    MyRegisterRead,
    MyRegisterWrite,
    MyFieldRead,                            //FieldRead
    MyFieldWrite,                           //FieldWrite    

    0,										//Ar5416ConfigurationRestore,
    Ar9287ConfigurationSave, 
    Ar9287CalibrationPierSet,
    0, 										// Ar5416PowerControlOverride,
    0,										// Ar5416TargetPowerSet,
    Ar5416EepromGetTargetPwr,
    Ar5416TargetPowerApply,

    Ar9287TemperatureGet,
    Ar9287VoltageGet,
    Ar9287MacAddressGet,
    Ar9287CustomerDataGet,

    Ar9287TxChainMany,
    0,										//TxChainMask
	Ar9287RxChainMany,
	0,										//RxChainMask
    Ar9287RxChainSet,

    0,										// Ar5416EepromTemplatePreference,
    0,										// Ar5416EepromTemplateAllowed,
    0,										// Ar5416EepromCompress,
    0,										// Ar5416EepromOverwrite,
    Ar9287EepromSize,
    Ar9287EepromSaveMemorySet,
    GetAll,									// Ar5416EepromReport,

    Ar9287CalibrationDataAddressSet,
    Ar9287CalibrationDataAddressGet,
    Ar9287CalibrationDataSet,
    Ar9287CalibrationDataGet,
    0,										//Ar9287CalibrationFromEepromFile,
    0,										//Ar9287EepromTemplateInstall,
    0,										// Ar5416PapdSet,
    0,										// Ar5416PaPredistortionGet,
    Ar9287_RegulatoryDomainOverride,
    Ar9287_RegulatoryDomainGet,
    0,										// Ar5416NoiseFloorSet,
    0,										//Ar5416NoiseFloorGet,
    0,										// Ar5416NoiseFloorPowerSet,
    0,										// Ar5416NoiseFloorPowerGet,
	0,										// Ar5416SpectralScanEnable,
	0,										// Ar5416SpectralScanProcess,
	0,										// Ar5416SpectralScanDisable,
    0,										// Ar5416NoiseFloorTemperatureSet,
    0,										// Ar5416NoiseFloorTemperatureGet,
//#ifdef ATH_SUPPORT_MCI
	0,										// Ar5416MCISetup,
	0,										// Ar5416MCIReset,
//#endif
	0,										// Ar5416TuningCapsSet,
	0,										// Ar5416TuningCapsSave,

	Ar9287ChannelCalculate,

	Ar9287ConfigSpaceCommit,	
	Ar9287ConfigSpaceUsed,
	Ar9287SubVendorSet,	
	Ar9287SubVendorGet,	
	Ar9287vendorSet,	
	Ar9287vendorGet,	
	Ar9287SSIDSet,	
	Ar9287SSIDGet,	
	Ar9287deviceIDSet,	
	Ar9287deviceIDGet,	
	Ar9287pcieAddressValueDataSet,	
	Ar9287pcieAddressValueDataGet,	
	Ar9287pcieMany,
	Ar9287pcieAddressValueDataOfNumGet,	
	0,
	0,

	Ar9287NoiseFloorFetch,
	Ar9287NoiseFloorLoad,
	Ar9287NoiseFloorReady,
	Ar9287NoiseFloorEnable,

	Ar9287_OpFlagsGet,
	Ar9287is2GHz,
	Ar9287is5GHz,
	Ar9287is4p9GHz,
	Ar9287HalfRate,
	Ar9287QuarterRate,

    Ar9287FlashRead,
    Ar9287FlashWrite,

    Ar9287IsEmbeddedArt,
    0, //StickyWrite
    0, //StickyClear
    0, //ConfigAddrSet
    0, //RfBbTestPoint

    0, //TransmitDataDut
    0, //TransmitStatusReport
    0, //TransmitStop
    0, //ReceiveDataDut
    0, //ReceiveStatusReport
    0, //ReceiveStop
    0, //CalInfoInit
    0, //CalInfoCalibrationPierSet
    0, //CalUnusedPierSet
    0, //OtpLoad
    0, //SetConfigParameterSplice
    0, //SetConfigCommand
    0, //StbcGet
    0, //LdpcGet
    0, //PapdGet
    0, //EepromSaveSectionSet
	0, //PapdIsDone

	Ar9287CalibrationPower,
	0, //Ar9300CalibrationTxgainCAPSet,		//	calculate and save tx calibration txgain CAP tx_gain_cap
	0, //Ar9300IniVersion,
	0, //TxGainTableRead_AddressGainTable
	0, //TxGainTableRead_AddressHeader
	0, //TxGainTableRead_AddressValue
	0, //Get_corr_coeff
	0, //TransmitINIGainGet
	0, //SleepMode
	0, //DeviceHandleGet
	0, //XtalReferencePPMGet,
	0, //CmacPowerGet,
	0, //PsatCalibrationResultGet,
	0, //DiagData,
	Ar9287GainTableOffset,//GainTableOffset,
	Ar9287CalibrationSetting, //CalibrationSetting,
};


//
// clear all device control function pointers and set to default behavior
//
AR9287DLLSPEC int Ar9287DeviceSelect(void)
{
	int error;

	DeviceFunctionReset();
	error=DeviceFunctionSelect(&_Ar9287Device);
	if(error!=0)
	{
		return error;
	}

	error=Ar5416TxDescriptorFunctionSelect();
	if(error!=0)
	{
		return error;
	}

	error=Ar5416RxDescriptorFunctionSelect();
	if(error!=0)
	{
		return error;
	}

	//
	// try to load the link layer dll
	//
#ifdef DYNAMIC_DEVICE_DLL
	error=LinkLoad(LinkDllName);
#else
	error=LinkLinkSelect();
#endif
	if(error!=0)
	{
		return error;
	}

	return error;
}

#ifdef AR9287DLL
AR9287DLLSPEC char *DevicePrefix(void)
{
	return "Ar9287";
}
#endif


