/*
 *  Copyright ï¿?2005 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */

#include <stdio.h>

#include "AquilaNewmaMapping.h"

//
// hal header files
//
#include "ah.h"
#include "ah_internal.h"
#include "ar9300.h"
#include "ar9300eep.h"
#include "ar9300desc.h"
#include "Ar9300_ChipInfo.h"
#include "ar9340.ini"
//#include "ar953x.ini" Need to include this once all the HAL have this INI

#ifdef NART_SCORPION_SUPPORT // Need to remnove this flag later once all branches have scorpion INI file
#include "ar955x.ini"
#endif

#include "ah_osdep.h"
#include "opt_ah.h"

#ifdef __APPLE__
#include "osdep.h"
#endif

//#include "ah_devid.h"
#include "ChipIdentify.h"
#include "ah_internal.h"
#include "ar9300reg.h"
#include "ar9300eep.h"

#include "papredistortion.h"
//#include "psat_cal.h"

#include "Ar9300Field.h"
#include "Ar9300SpectralScan.h"

#include "wlantype.h"
//#include "mdata.h"
#include "rate_constants.h"
#include "vrate_constants.h"

#include "NewArt.h"
#include "ParameterSelect.h"

#include "AnwiDriverInterface.h"
//#include "Card.h"
#include "smatch.h"
#include "Field.h"

#include "Device.h"
#include "Ar9300Device.h"
#include "Ar9300NoiseFloor.h"

//#include "LinkRx.h"
#include "mCal9300.h"
#include "mEepStruct9300.h"
#include "Ar9300EepromStructSet.h"

#include "Ar9300CalibrationApply.h"
#include "Ar9300EepromSave.h"
#include "ar9300_target_pwr.h"

#include "Ar9300Temperature.h"

#include "AnwiDriverInterface.h"

//#include "ParameterConfigDef.h"
//#include "ConfigurationStatus.h"

#include "UserPrint.h"
#include "ErrorPrint.h"
#include "CardError.h"

#include "TimeMillisecond.h"

#include "Ar9300TxDescriptor.h"
#include "Ar9300RxDescriptor.h"

#include "Ar9300PcieConfig.h"
#include "Sticky.h"

#include "LinkList.h"

#ifdef DYNAMIC_DEVICE_DLL
#include "LinkLoad.h"
#else
#include "DescriptorLink.h"
#endif

#include "Ar9300Version.h"
#include "AR9300ChipIdentify.h"
#include "ar9300Eeprom_txGainCap.h"

#include "NartRegister.h"

#ifndef NART_SCORPION_SUPPORT // To Do:: This 'ifndef endif' block needs to be removed when wlan/hal/ah_devid.h is updated with scorpion device ID. Only 'scorpion_dev' HAL branch defines this device ID now.
#define AR9300_DEVID_AR955X       0x0039        /* Scorpion */
#endif

#define LinkDllName "linkAr9k"

// 
// this is the hal pointer, 
// returned by ath_hal_attach
// used as the first argument by most (all?) HAL routines
//
struct ath_hal *AH=0;


#define MDCU 10			// should we only set the first 8??
#define MQCU 10

#define MBUFFER 1024

#define MAC_PCU_STA_ADDR_L32 0x00008000
#define MAC_PCU_STA_ADDR_U16 0x00008004
#define MAC_PCU_BSSID_L32 0x00008008
#define MAC_PCU_BSSID_U16 0x0000800c

static int deafMode = 0;
static int undeafThresh62 = 0;
static int undeafThresh62Ext = 0;
static int undeafForceAgcClear = 0;
static int undeafCycpwrThr1 = 0;
static int undeafCycpwrThr1Ext = 0;
static int undeafRssiThr1a = 0;

#ifdef UNUSED    // I DONT UNDERSTAND WHAT THIS IS USED FOR.
int calData = CALDATA_AUTO;
#endif


int Ar9300ChannelCalculate(int *frequency, int *option, int mchannel)
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
   	regDmn = ar9300_regulatory_domain_get(AH);
   	if( regDmn[0] != 0)
   	{
        ar9300_regulatory_domain_override(AH, 0);
   	}
   	if(regDmn[0] & 0x8000)
// 		cc=regDmn[0] & 0x3ff;
		cc=regDmn[0] & 0xfff;
   	else
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


int Ar9300TuningCapsSet(int caps)
{
    //FieldWrite("ch0_XTAL.xtal_capindac", caps);
    //FieldWrite("ch0_XTAL.xtal_capoutdac", caps);
	//return 0;
	return Ar9300_TuningCaps(caps);
}

int Ar9300TuningCapsSave(int caps)
{
	Ar9300pwrTuningCapsParamsSet(&caps, 0, 0, 0, 1, 0);
	return 0;
}


int Ar9300TransmitCarrier(unsigned int txchain)
{
	if(txchain&1)
	{
		FieldWrite("ch0_rxtx3.dacfullscale",1);
	}
	if(txchain&2)
	{
		FieldWrite("ch1_rxtx3.dacfullscale",1);
	}
	if(txchain&4)
	{
		FieldWrite("ch2_rxtx3.dacfullscale",1);
	}

	return 0;
}

static void Ar9300CarrierOnlyClear(void)
{
	FieldWrite("ch0_rxtx3.dacfullscale",0);
	FieldWrite("ch1_rxtx3.dacfullscale",0);
	FieldWrite("ch2_rxtx3.dacfullscale",0);
}

AR9300DLLSPEC int Ar9300TxChainMany(void)
{

//	unsigned int ah_enterprise_mode;
	int regChains=1;
	int txMask = Ar9300txMaskGet();

	regChains=Ar9300_TxChainMany(txMask);

/*
    if (AR_SREV_HORNET(AH) || AR_SREV_POSEIDON(AH) || AR_SREV_APHRODITE(AH))
	{
       regChains = 1;
    }
	else if (AR_SREV_WASP(AH) || AR_SREV_JUPITER(AH))
	{
       regChains = 2;
    }
	else
	{
        // Osprey needs to be configured for 3-chain mode before running AGC/TxIQ cals
		int ah_enterprise_mode;

		MyRegisterRead(AR_ENT_OTP,&ah_enterprise_mode);
        if(ah_enterprise_mode&AR_ENT_OTP_CHAIN2_DISABLE)
		{
			regChains = 2;
		}
		else
		{
			regChains = 3;
		}
    }

	if (txMask==7) {
		return regChains;
	} else if (txMask==5 || txMask==3) {
		if (regChains>=2)
			return 2;
		else
			return 1;
	} else {
		return 1;
	} */
	return regChains;
}

AR9300DLLSPEC int Ar9300RxChainMany(void)
{
//	unsigned int ah_enterprise_mode;
	int regChains=1;
	int rxMask = Ar9300rxMaskGet();

	regChains=Ar9300_RxChainMany(rxMask);

/*
    if (AR_SREV_HORNET(AH) || AR_SREV_POSEIDON(AH) || AR_SREV_APHRODITE(AH))

	{
       regChains = 1;
    }
	else if (AR_SREV_WASP(AH) || AR_SREV_JUPITER(AH))
	{
       regChains = 2;
    }
	else
	{
        // Osprey needs to be configured for 3-chain mode before running AGC/TxIQ cals
		int ah_enterprise_mode;

		MyRegisterRead(AR_ENT_OTP,&ah_enterprise_mode);
        if(ah_enterprise_mode&AR_ENT_OTP_CHAIN2_DISABLE)
		{
			regChains = 2;
		}
		else
		{
			regChains = 3;
		}
    }

	if (rxMask==7) {
		return regChains;
	} else if (rxMask==5 || rxMask==3) {
		if (regChains>=2)
			return 2;
		else
			return 1;
	} else {
		return 1;
	} */
	return regChains;
}

AR9300DLLSPEC int Ar9300is2GHz(void)
{
//	unsigned int ah_mode;
	int opflag = DeviceOpflagsGet();
	int regFlag=1;
	regFlag = Ar9300_is2GHz(opflag);
/*
    if (AR_SREV_HORNET(AH) || AR_SREV_POSEIDON(AH))
	{
       regFlag=1;
    }
	else if (AR_SREV_WASP(AH) || AR_SREV_JUPITER(AH))
	{
       regFlag=1;
    }
	else
	{
		regFlag=1;
    }
	if (regFlag==1) {
		if (opflag&(0x2))
			return 1;
		else
			return 0;
	} else {
		return 0;
	}  */
	return regFlag;
}

AR9300DLLSPEC int Ar9300is5GHz(void)
{
//	unsigned int ah_mode;
	int opflag = DeviceOpflagsGet();
	int regFlag=1;
	regFlag = Ar9300_is5GHz(opflag);

/*
    if (AR_SREV_HORNET(AH) || AR_SREV_POSEIDON(AH))
	{
        regFlag=1;
    }
	else if (AR_SREV_WASP(AH) || AR_SREV_JUPITER(AH) || AR_SREV_APHRODITE(AH))
	{
        regFlag=1;
    }
	else
	{
		MyRegisterRead(AR_ENT_OTP,&ah_mode);
        if(ah_mode&AR_ENT_OTP_DUAL_BAND_DISABLE)
		{
			 regFlag=0;
		}
    }
	if (regFlag==1) {
		if ( opflag&(0x1) )
			return 1;
		else
			return 0;
	} else {
		return 0;
    }  */
	return regFlag;
}

AR9300DLLSPEC int Ar9300is4p9GHz(void)
{
/*	unsigned int ah_mode;

    if (AR_SREV_HORNET(AH) || AR_SREV_POSEIDON(AH))
	{
       return 0;
    }
	else if (AR_SREV_WASP(AH) || AR_SREV_JUPITER(AH) || AR_SREV_APHRODITE(AH))
	{
       return 0;
    }
	else
	{
		MyRegisterRead(AR_ENT_OTP,&ah_mode);
        if(ah_mode&AR_ENT_OTP_49GHZ_DISABLE)
		{
			return 0;
		}
		else
		{
			return 1;
		}
    } */
	return Ar9300_is4p9GHz();
}

AR9300DLLSPEC int Ar9300HalfRate(void)
{
/*	unsigned int ah_mode;

    if (AR_SREV_HORNET(AH) || AR_SREV_POSEIDON(AH))
	{
       return 0;
    }
	else if (AR_SREV_WASP(AH) || AR_SREV_JUPITER(AH) || AR_SREV_APHRODITE(AH))
	{
       return 0;
    }
	else
	{
		MyRegisterRead(AR_ENT_OTP,&ah_mode);
        if(ah_mode&AR_ENT_OTP_10MHZ_DISABLE)
		{
			return 0;
		}
		else
		{
			return 1;
		}
    } */
	return Ar9300_HalfRate();
}

AR9300DLLSPEC int Ar9300QuarterRate(void)
{
/*	unsigned int ah_mode;

    if (AR_SREV_HORNET(AH) || AR_SREV_POSEIDON(AH))
	{
       return 0;
    }
	else if (AR_SREV_WASP(AH) || AR_SREV_JUPITER(AH) || AR_SREV_APHRODITE(AH))
	{
       return 0;
    }
	else
	{
		MyRegisterRead(AR_ENT_OTP,&ah_mode);
        if(ah_mode&AR_ENT_OTP_5MHZ_DISABLE)
		{
			return 0;
		}
		else
		{
			return 1;
		}
    }	*/
	return Ar9300_QuarterRate();
}

#ifdef UNUSED
int Ar9300FlashCal(int value)
{
#ifdef MDK_AP
    calData = CALDATA_FLASH ;
    UserPrint("FlashCal = %d\n", calData);
	return 0;
#else
	printf("Error: Flash access for windows build is not supported.\n");
	return -1;
#endif
}
#endif

AR9300DLLSPEC int Ar9300BssIdSet(unsigned char *bssid)
{
	unsigned int reg;

	reg=bssid[3]<<24|bssid[2]<<16|bssid[1]<<8|bssid[0];
    MyRegisterWrite(MAC_PCU_BSSID_L32,reg);

//    reg=0;
	MyRegisterRead(MAC_PCU_BSSID_U16,&reg);
	reg &= ~(0xffff);
	reg |= (bssid[5]<<8|bssid[4]);
    MyRegisterWrite(MAC_PCU_BSSID_U16,reg);

	return 0;
}


AR9300DLLSPEC int Ar9300StationIdSet(unsigned char *mac)
{
	unsigned int reg;

	reg=mac[3]<<24|mac[2]<<16|mac[1]<<8|mac[0];
    MyRegisterWrite(MAC_PCU_STA_ADDR_L32,reg);

//    reg=0;
	MyRegisterRead(MAC_PCU_STA_ADDR_U16,&reg);
	reg &= ~(0xffff);
	reg |= (mac[5]<<8|mac[4]);
    MyRegisterWrite(MAC_PCU_STA_ADDR_U16,reg);

	return 0;
}

#ifdef UNUSED
AR9300DLLSPEC int Ar9300RegisterRead(unsigned int address, unsigned int *value)
{
	return MyRegisterRead(address, value);
}


AR9300DLLSPEC int Ar9300RegisterWrite(unsigned int address, unsigned int value)
{
	return MyRegisterWrite(address, value);
}


AR9300DLLSPEC int Ar9300MemoryRead(unsigned int address, unsigned int *value, int nvalue)
{
	return MyMemoryRead(address, value, nvalue);
}


AR9300DLLSPEC int Ar9300MemoryWrite(unsigned int address, unsigned int *value, int nvalue)
{
	return MyMemoryWrite(address, value, nvalue);
}
#endif


#define AR9300_EEPROM_SIZE 16*1024  // byte addressable


AR9300DLLSPEC int Ar9300EepromRead(unsigned int address, unsigned char *buffer, int many)
{
    if(!ar9300_calibration_data_read_eeprom(AH, address, (unsigned char*)buffer, many ))// return 0 if eprom read is correct; ar9300EepromRead returns 1 for success
		return 1; // bad read

    return 0;
}


AR9300DLLSPEC int Ar9300EepromWrite(unsigned int address, unsigned char *buffer, int many)
{
    unsigned int eepAddr;
    unsigned int byteAddr;
    unsigned short svalue, word;
    int i;

	if(((address)<0)||((address+many)>AR9300_EEPROM_SIZE-1)){
		return 1; // bad address
	}

    for(i=0;i<many;i++){
        eepAddr = (unsigned short)(address+i)/2;
       	byteAddr = (unsigned short) (address+i)%2;
		if(!ar9300_eeprom_read_word(AH, eepAddr, &svalue ))// return 0 if eprom read is correct; ar9300EepromRead returns 1 for success
			return 1; // bad read
        word = buffer[i]<<(byteAddr*8);
        svalue = svalue & ~(0xff<<(byteAddr*8));
        svalue = svalue | word;
		if(!ar9300_eeprom_write(AH, eepAddr,  svalue ))// return 0 if eprom write is correct; ar9300_eeprom_write returns 1 for success
			return 1; // bad write
	}
    return 0;
}


AR9300DLLSPEC int Ar9300FlashRead(unsigned int address, unsigned char *buffer, int many)
{
#ifdef MDK_AP
    if(!ar9300_calibration_data_read_flash(AH, address, (unsigned char*)buffer, many ))// return 0 if eprom read is correct; ar9300EepromRead returns 1 for success
		return 1; // bad read

    return 0;
#else
	return 1;
#endif
}


AR9300DLLSPEC int Ar9300FlashWrite(unsigned int address, unsigned char *buffer, int many)
{
    unsigned int eepAddr;
    unsigned int byteAddr;
    unsigned short svalue, word;
    int i;

#ifdef MDK_AP
	if(((address)<0)||((address+many)>AR9300_EEPROM_SIZE-1)){
		return 1; // bad address
	}

//    if(calData == CALDATA_FLASH ){
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
 //   }

    return 1;
#else
	return 1;
#endif
}


AR9300DLLSPEC int Ar9300OtpRead(unsigned int address, unsigned char *buffer, int many, int is_wifi)
{
    if(!ar9300_calibration_data_read_otp(AH, address, (unsigned char*)buffer, many, is_wifi ))// return 0 if eprom read is correct; ar9300EepromRead returns 1 for success
		return 1; // bad read

    return 0;
}

AR9300DLLSPEC int Ar9300OtpWrite(unsigned int address, unsigned char *buffer, int many, int is_wifi)
{
    unsigned int eepAddr;
    unsigned int byteAddr;
    unsigned int word;
    unsigned int svalue;
    int i;
	int osize;

#ifdef USE_AQUILA_HAL
	osize=1024;
#else
//	osize=ar9300_otp_size(AH);
	osize=1024;
#endif

	if(((address)<0)||((address+many)>osize)){
		return 1; // bad address
	}

     for(i=0;i<many;i++){
        eepAddr = (unsigned short)(address+i)/4;
       	byteAddr = (unsigned short) (address+i)%4;
		if(!ar9300_otp_read(AH, eepAddr, &svalue, is_wifi))// return 0 if otp read is correct; ar9300_otp_read returns 1 for success
			return 1; // bad read
        word = buffer[i]<<(byteAddr*8);
        svalue = svalue & ~(0xff<<(byteAddr*8));
        svalue = svalue | word;
		if(!ar9300_otp_write(AH, eepAddr,  svalue, is_wifi))// return 0 if otp write is correct; ar9300_otp_write returns 1 for success
			return 1; // bad write
	}
    return 0;
}


#undef REGR

static unsigned int REGR(unsigned int devNum, unsigned int address)
{
	unsigned int value;

	devNum=0;

	MyRegisterRead(address,&value);

	return value;
}

#define MAC_PCU_RX_FILTER (0x803c)
#define BROADCAST (0x4)
#define UNICAST (0x1)
#define MULTICAST (0x2)
#define PROMISCUOUS (0x20)

#define MAC_PCU_DIAG_SW (0x8048)

#define MAC_DMA_CR (0x0008)
#define RXE_LP (0x4)

#define MAC_DMA_RX_QUEUE_LP_RXDP (0x0078)

#define MAC_QCU_TXDP (0x0800)

#define MAC_QCU_STATUS_RING_START (0x0830)
#define MAC_QCU_STATUS_RING_STOP  (0x0834)

#define MAC_DCU_QCUMASK (0x1000)
//
// disable receive
//
AR9300DLLSPEC int Ar9300ReceiveDisable(void)
{
    FieldWrite("MAC_DMA_CR.RXD",1);
	return 0;
}


//
// enable receive
//
AR9300DLLSPEC int Ar9300ReceiveEnable(void)
{
	MyRegisterWrite(MAC_PCU_DIAG_SW,0);
    FieldWrite("MAC_DMA_CR.RXD",0);
    FieldWrite("MAC_DMA_CR.RXE_LP",1);
	return 0;
}


//
// set pointer to rx descriptor in shared memory
//
AR9300DLLSPEC int Ar9300ReceiveDescriptorPointer(unsigned int descriptor)
{
    MyRegisterWrite(MAC_DMA_RX_QUEUE_LP_RXDP, descriptor);

	return 0;
}


//
// set or clear receive filter bit
//
AR9300DLLSPEC int Ar9300ReceiveFilter(int on, unsigned int mask)
{
	unsigned int reg;

		reg=REGR(0, MAC_PCU_RX_FILTER);
//	    UserPrint("Ar9300ReceiveFilter(on,%x): MAC_PCU_RX_FILTER %x ->",mask,reg);
	if(on)
	{
		reg |= mask;
	}
	else
	{
		reg &= (~mask);
 	}
//    	UserPrint(" %x\n",reg);
        MyRegisterWrite(MAC_PCU_RX_FILTER, reg);

	return 0;
}


//
// set or clear receive of unicast packets
//
AR9300DLLSPEC int Ar9300ReceiveUnicast(int on)
{
	return Ar9300ReceiveFilter(on,UNICAST);
}


//
// set or clear receive of broadcast packets
//
AR9300DLLSPEC int Ar9300ReceiveBroadcast(int on)
{
	return Ar9300ReceiveFilter(on,BROADCAST);
}


//
// set or clear promiscuous mode
//
AR9300DLLSPEC int Ar9300ReceivePromiscuous(int on)
{
	return Ar9300ReceiveFilter(on,PROMISCUOUS);
}


//
// Set contention window.
// cwmin and cwmax are limited to values that are powers of two minus 1: 0, 1, 3, 7, ....
//
static int Ar9300TransmitContentionWindow(int dcu, int cwmin, int cwmax)
{
	static int allowed[]={0x0,0x1,0x3,0x7,0xf,0x1f,0x3f,0x7f,0xff,0x1ff,0x3ff};
	int nallowed;
	int it;
	char buffer[MBUFFER];
	//
	// default values
	//
//	UserPrint("Ar9300ContentionWindow(%d,%d,%d): ",dcu,cwmin,cwmax);
	nallowed=sizeof(allowed)/sizeof(int);

	if(cwmin<0)
	{
		cwmin=0xf;
	}
	else if(cwmin>=allowed[nallowed-1])
	{
		cwmin=allowed[nallowed-1];
	}
	else
	{
		for(it=0; it<sizeof(allowed)/sizeof(int); it++)
		{
			if(cwmin>allowed[it])
			{
				cwmin=allowed[it];
				break;
			}
		}
	}

	if(cwmax<0)
	{
		cwmax=0x3ff;
	}
	else if(cwmax>=allowed[nallowed-1])
	{
		cwmax=allowed[nallowed-1];
	}
	else
	{
		for(it=0; it<sizeof(allowed)/sizeof(int); it++)
		{
			if(cwmax>allowed[it])
			{
				cwmax=allowed[it];
				break;
			}
		}
	}

	if(cwmin>cwmax)
	{
		cwmin=cwmax;
	}

	SformatOutput(buffer,MBUFFER-1,"MAC_DCU_LCL_IFS[%d].CW_MIN",dcu);
    FieldWrite(buffer,cwmin);

	SformatOutput(buffer,MBUFFER-1,"MAC_DCU_LCL_IFS[%d].CW_MAX",dcu);
    FieldWrite(buffer,cwmax);

	return 0;
}


static int Ar9300TransmitOtherFastStuff(int on)
{
	if(on)
	{
		FieldWrite("MAC_DCU_GBL_IFS_SIFS.DURATION", 0x400);
	}
	return 0;
}

AR9300DLLSPEC int Ar9300ContinuousDataMode(int on)
{
	unsigned int reg;

	if(on)
	{
        // Put PCU and DMA in continuous data mode
        reg=REGR(0, 0x8054);
//		UserPrint("Ar5416ContinuousDataMode(%d) 0x8054: %x ->",on,reg);
	    reg |= 1;
//		UserPrint(" %x\n",reg);
		MyRegisterWrite(0x8054, reg);

        //disable encryption since packet has no header
		reg=REGR(0, 0x8048);
//		UserPrint("Ar5416ContinuousDataMode(%d) F2_DIAG_SW: %x ->",on,reg);
		reg |= 0x8;
//		UserPrint(" %x\n",reg);
        MyRegisterWrite(0x8048,  reg);
	}
	else
	{
        // Put PCU and DMA in continuous data mode
        reg=REGR(0, 0x8054);
//		UserPrint("Ar5416ContinuousDataMode(%d) 0x8054: %x ->",on,reg);
	    reg &= (~1);
//		UserPrint(" %x\n",reg);
		MyRegisterWrite(0x8054, reg);
	}
	return 0;
}

AR9300DLLSPEC int Ar9300TransmitRegularData(void)			// normal
{
	int dcu;

	for(dcu=0; dcu<MDCU; dcu++)
	{
        Ar9300TransmitContentionWindow(dcu, -1, -1);
	}
	Ar9300TransmitOtherFastStuff(1);
	Ar9300ContinuousDataMode(0);
	return 0;
}


AR9300DLLSPEC int Ar9300TransmitFrameData(int ifs)	// tx99
{
	int dcu;

	for(dcu=0; dcu<MDCU; dcu++)
	{
        Ar9300TransmitContentionWindow(dcu, 0, 0);
	}
	Ar9300TransmitOtherFastStuff(1);
	Ar9300ContinuousDataMode(0);
	return 0;
}


AR9300DLLSPEC int Ar9300TransmitContinuousData(void)		// tx100
{
	int dcu;

	for(dcu=0; dcu<MDCU; dcu++)
	{
        Ar9300TransmitContentionWindow(dcu, -1, -1);
	}
	Ar9300TransmitOtherFastStuff(1);
	Ar9300ContinuousDataMode(1);

    return 0;
}


//
// set pointer to tx descriptor in shared memory
//
AR9300DLLSPEC int Ar9300TransmitDescriptorPointer(int queue, unsigned int descriptor)
{
    MyRegisterWrite(MAC_QCU_TXDP + (4 * queue), descriptor);
	return 0;
}

//
// set pointer to tx descriptor in shared memory
//
AR9300DLLSPEC int Ar9300TransmitDescriptorStatusPointer(unsigned int start, unsigned int stop)
{
    MyRegisterWrite(MAC_QCU_STATUS_RING_START, start);
    MyRegisterWrite(MAC_QCU_STATUS_RING_STOP, stop);
	return 0;
}

//
// map the qcu to the dcu and enable the clocks for both
//
AR9300DLLSPEC int Ar9300TransmitQueueSetup(int qcu, int dcu)
{
	unsigned int reg;

	MyRegisterWrite(MAC_PCU_DIAG_SW,0);
    //
	// program the queue
	//
    reg=REGR(0,MAC_DCU_QCUMASK + (4 * dcu ));
//	UserPrint("Ar9300QueueSetup(%d,%d): F2_D0_QCUMASK %x ->",qcu,dcu,reg);
	reg|=(1<<qcu);
//	UserPrint(" %x\n",reg);
    MyRegisterWrite(MAC_DCU_QCUMASK +  ( 4 * dcu ), reg);

	return 0;
}

AR9300DLLSPEC int Ar9300TransmitDisable(unsigned int mask)
{
    ar9300_abort_tx_dma(AH);
    return 0;
}

AR9300DLLSPEC int Ar9300TargetPowerApply(int frequency)
{
    A_UINT8 targetPowerValT2[ar9300_rate_size];

	/* make sure forced gain is not set - HAL function will not do this */
    FieldWrite("force_dac_gain", 0);
	FieldWrite("force_tx_gain", 0);

	ar9300_set_target_power_from_eeprom(AH, (short)frequency, targetPowerValT2);
	//
	// Write target power array to registers
	//
	ar9300_transmit_power_reg_write(AH, targetPowerValT2);

   return 0;
}
/*
*   returns:
*        0: success
*        1: error
*/
AR9300DLLSPEC int Ar9300PllSceen(void)
{
    HAL_CHANNEL channel;
    int average = 50;
    int i = 0;
    int shutdownState = 0;
    unsigned int rtcState = 0, pllState = 0, vc_meas0 = 0;
    int min = 0, max = 0;
    int start,end;

#define RTC_SYNC_FORCE_WAKE (0x0000704c)
#define RTC_SYNC_RESET (0x00007040)
#define RTC_SYNC_STATUS (0x00007044)
#define PLL_CONTROL (0x00007014)

printf("\n =>  Ar9300PllSceen  \n");
    start=TimeMillisecond();

    channel.channel=2412;        /* setting in Mhz */
    channel.channel_flags=CHANNEL_2GHZ|CHANNEL_HT20;
    channel.priv_flags=CHANNEL_DFS_CLEAR;
    channel.max_reg_tx_power=27;  /* max regulatory tx power in dBm */
    channel.max_tx_power=2*27;     /* max true tx power in 0.5 dBm */
    channel.min_tx_power=0;     /* min true tx power in 0.5 dBm */
    channel.regClassId=0;     /* regulatory class id of this channel */

    for(i = 0; i < average; i++){
        //power_mode_full_sleep
        #if 1
        ar9300_set_power_mode(AH, HAL_PM_FULL_SLEEP, 1);
        #else
        FieldWrite("MAC_PCU_STA_ADDR_U16.PW_SAVE", 0x1);
        OS_DELAY(50);
        FieldWrite("RTC_SYNC_FORCE_WAKE.ENABLE", 0x0);
        OS_DELAY(50);
        FieldWrite("RTC_SYNC_RESET.RESET_L", 0x0);
        OS_DELAY(50);
        #endif

        //power_mode_awake
        #if 1
        ar9300_set_power_mode_awake(AH, 1);
        #else
        OS_DELAY(50);
        FieldRead("RTC_SYNC_STATUS.SHUTDOWN_STATE", (unsigned int *)&shutdownState);
        if(shutdownState == 1){
            MyRegisterWrite(RTC_SYNC_FORCE_WAKE, 0x3);
            OS_DELAY(50);
            MyRegisterWrite(RTC_SYNC_RESET, 0x0);
            OS_DELAY(50);
            MyRegisterWrite(RTC_SYNC_RESET, 0x1);
            OS_DELAY(50);

            do{
                MyRegisterRead(RTC_SYNC_STATUS, &rtcState);
                OS_DELAY(50);
            }while(rtcState != 2);

            do{
                FieldRead("PLL_CONTROL.UPDATING", (unsigned int *)&pllState);
                OS_DELAY(50);
            }while(pllState == 1);

        }
        #endif

        //init_pll
        ar9300_init_pll(AH, &channel);

        /*****************************************************
          * toggle "ch0_DPLL3.do_meas" and read "ch0_DPLL4.vc_meas0" back.
          * record the highest and the lowest vc_meas0 value
          * if the delta of the highest and the lowest value is smaller than 0x2000, we treat it as good chip.
          *****************************************************/
        FieldWrite("ch0_DPLL3.do_meas", 0x0);
        OS_DELAY(50);
        FieldWrite("ch0_DPLL3.do_meas", 0x1);
        OS_DELAY(50);

        FieldRead("ch0_DPLL4.vc_meas0", (unsigned int *)&vc_meas0);
        if(vc_meas0 == 0)
            continue;

        printf("\n vc_meas0 = %d", vc_meas0);
        if(min == 0 && max == 0){
            /* first loop */
            min = vc_meas0;
            max = vc_meas0;
        }else{
            if(vc_meas0 < min)
                min = vc_meas0;

            if(vc_meas0 > max)
                max = vc_meas0;

       }
        printf("\n min = [%d], max = [%d]", min, max);
    }

printf("\n DELTA = %d", max - min);
printf("\n <=  Ar9300PllSceen  \n");
    end=TimeMillisecond();
    printf("Ar9300PllSceen duration: %d=%d-%d ms\n",end-start,end,start);
#undef RTC_SYNC_FORCE_WAKE
#undef RTC_SYNC_RESET
#undef RTC_SYNC_STATUS
#undef PLL_CONTROL
    if((max - min) < 0x2000)
		return 0;
    else
		return 1;

}



//set the registers for the selected rx chain mask
AR9300DLLSPEC int Ar9300RxChainSet(int rxChain)
{
    if(rxChain == 0x5) {
        FieldWrite("BB_analog_swap.swap_alt_chn", 0x1);
    }

    FieldWrite("BB_multichain_enable.rx_chain_mask", rxChain & 0x7);
    FieldWrite("BB_cal_chain_mask.cal_chain_mask", rxChain & 0x7);
    return 0;
}

//
//enable deaf mode
//
AR9300DLLSPEC int Ar9300ReceiveDeafMode(int deaf)
{
    //
    //If not currently in deaf mode Store off the existing field values so that can go back to undeaf mode
    //
    if (deafMode == 0) {
        FieldRead("BB_cca_b0.cf_thresh62", (unsigned int *)&undeafThresh62);
        FieldRead("BB_ext_chan_pwr_thr_1.thresh62_ext", (unsigned int *) &undeafThresh62Ext);
        FieldRead("BB_test_controls.force_agc_clear", (unsigned int *) &undeafForceAgcClear);
        FieldRead("BB_timing_control_5.cycpwr_thr1", (unsigned int *) &undeafCycpwrThr1);
        FieldRead("BB_ext_chan_pwr_thr_2_b0.cycpwr_thr1_ext", (unsigned int *) &undeafCycpwrThr1Ext);
        FieldRead("BB_timing_control_5.rssi_thr1a", (unsigned int *) &undeafRssiThr1a);
    }

    if(deaf) {
        FieldWrite("BB_cca_b0.cf_thresh62", 0x7f);
        FieldWrite("BB_ext_chan_pwr_thr_1.thresh62_ext", 0x7f);
        FieldWrite("BB_test_controls.force_agc_clear", 1);
        FieldWrite("BB_timing_control_5.cycpwr_thr1", 0x7f);
        FieldWrite("BB_ext_chan_pwr_thr_2_b0.cycpwr_thr1_ext", 0x7f);
        FieldWrite("BB_timing_control_5.rssi_thr1a", 0x7f);
        deafMode=1;
    } else {
        FieldWrite("BB_cca_b0.cf_thresh62", undeafThresh62);
        FieldWrite("BB_ext_chan_pwr_thr_1.thresh62_ext", undeafThresh62Ext);
        FieldWrite("BB_test_controls.force_agc_clear", undeafForceAgcClear);
        FieldWrite("BB_timing_control_5.cycpwr_thr1", undeafCycpwrThr1);
        FieldWrite("BB_ext_chan_pwr_thr_2_b0.cycpwr_thr1_ext", undeafCycpwrThr1Ext);
        FieldWrite("BB_timing_control_5.rssi_thr1a", undeafRssiThr1a);
        deafMode=0;
    }
    return 0;
}

#ifdef UNUSED
#define MCHANNEL 2000
static HAL_CHANNEL Channel[MCHANNEL];
static unsigned int ChannelMany;

static int CardChannelCalculate(void)
{
	u_int32_t modeSelect;
    HAL_BOOL enableOutdoor;
	HAL_BOOL enableExtendedChannels;
	u_int16_t *regDmn;
    u_int8_t regclassids[MCHANNEL];
    u_int maxchans, maxregids, nregids;
    HAL_CTRY_CODE cc;
	int error;
	//
	// try calling this here
	//
	//Fcain need to handle this better, should not be accessing struct values directly
	regDmn = ar9300_regulatory_domain_get(AH);
	if(regDmn != 0)
	{
		UserPrint("RegulatoryDomainOverride: 0x%x -> 0\n",regDmn[0]);
        ar9300_regulatory_domain_override(AH,0);
	}
	cc=CTRY_DEBUG;		//CTRY_DEFAULT;			// changed from CTRY_DEBUG since that doesn't work if regdmn is set
	modeSelect=0xffffffff;
    enableOutdoor=0;
	enableExtendedChannels=0;
    maxchans=sizeof(Channel)/sizeof(Channel[0]);
	maxregids=sizeof(regclassids)/sizeof(regclassids[0]);

    error=ath_hal_init_channels(AH,
              Channel, maxchans, &ChannelMany,
              regclassids, maxregids, &nregids,
              cc, modeSelect,
              enableOutdoor, enableExtendedChannels);

	if(error==0)
	{
//		AH=0;
		ErrorPrint(CardLoadNoChannel);
//		return -5;
	}

	return ChannelMany;
}
#endif

static int _Ar9300ReceiveFifo;
static int _Ar9300ReceiveDescriptorMaximum;
static int _Ar9300ReceiveEnableFirst;
static int _Ar9300TransmitFifo;
static int _Ar9300TransmitDescriptorSplit;
static int _Ar9300TransmitAggregateStatus;
static int _Ar9300TransmitEnableFirst;


AR9300DLLSPEC int Ar9300ReceiveFifo(void)
{
	return _Ar9300ReceiveFifo;
}

AR9300DLLSPEC int Ar9300ReceiveDescriptorMaximum(void)
{
	return _Ar9300ReceiveDescriptorMaximum;
}

AR9300DLLSPEC int Ar9300ReceiveEnableFirst(void)
{
	return _Ar9300ReceiveEnableFirst;
}

AR9300DLLSPEC int Ar9300TransmitFifo(void)
{
	return _Ar9300TransmitFifo;
}

AR9300DLLSPEC int Ar9300TransmitDescriptorSplit(void)
{
	return _Ar9300TransmitDescriptorSplit;
}

AR9300DLLSPEC int Ar9300TransmitAggregateStatus(void)
{
	return _Ar9300TransmitAggregateStatus;
}

AR9300DLLSPEC int Ar9300TransmitEnableFirst(void)
{
	return _Ar9300TransmitEnableFirst;
}


int Ar9300Detach(void)
{
	return AnwiDriverDetach();
}

int Ar9300Valid(void)
{
	return AnwiDriverValid();
}

int Ar9300DeviceIdGet(void)
{
	return AnwiDriverDeviceIdGet();
}

int Ar9300Attach(int devid, int calmem)
{
    HAL_ADAPTER_HANDLE osdev;
	HAL_SOFTC sc;
	HAL_BUS_TAG st;
	HAL_BUS_HANDLE sh;
	HAL_BUS_TYPE bustype;
    struct hal_reg_parm hal_conf_parm;
	HAL_STATUS error;
	unsigned char header[compression_header_length];

	int start,end;
	int it;
    char buffer[MBUFFER];
	int caluse;
	int eepsize;
    int status;

	int npcie;

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
 #ifndef MD_AP
 	hal_conf_parm.calInFlash = 0;		// for Jupiter load
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

	ar9300_calibration_data_set(0,calmem);

	start=TimeMillisecond();
	error=0;
	AH=ar9300_attach((unsigned short)devid, osdev, sc, st, sh, bustype, NULL /* amem_handle */, &hal_conf_parm, &error);
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
	end=TimeMillisecond();
	UserPrint("ath_hal_attach duration: %d=%d-%d ms\n",end-start,end,start);
	//
	// figure out where the calibration memory really is
	//
    caluse=ar9300_calibration_data_get(AH);
	switch(caluse)
	{
		case calibration_data_none:
			ErrorPrint(CardLoadCalibrationNone);
			eepsize=ar9300_eeprom_size(AH);
			if(eepsize>0)
			{
				ar9300_calibration_data_set(AH,calibration_data_eeprom);
			}
			else if(ar9300_calibration_data_read_flash(AH, 0x1000, header, 1)==AH_TRUE)
			{
				ar9300_calibration_data_set(AH,calibration_data_flash);
			}
			else
			{
				ar9300_calibration_data_set(AH,calibration_data_otp);
			}
			break;
		case calibration_data_flash:
			ErrorPrint(CardLoadCalibrationFlash);
			break;
		case calibration_data_eeprom:
			ErrorPrint(CardLoadCalibrationEeprom,ar9300_calibration_data_address_get(AH));
			break;
		case calibration_data_otp:
			ErrorPrint(CardLoadCalibrationOtp,ar9300_calibration_data_address_get(AH));
			break;
	}
	//
	// read the pcie data initialization space
	//
	npcie=Ar9300pcieAddressValueDataInit();
	//
	// THIS LIST NEEDS TO BE COMPLETE AND ACCURATE
	// WOULD LIKE IT TO BE INSIDE THE HAL ATTACH (OR REPLACE WITH EQUIVALENT HAL FUNCTIONS)
	//
	_Ar9300ReceiveFifo=1;
	_Ar9300ReceiveDescriptorMaximum=128;
	_Ar9300ReceiveEnableFirst=1;
	_Ar9300TransmitFifo=1;
	_Ar9300TransmitDescriptorSplit=1;
	_Ar9300TransmitAggregateStatus=0;
	_Ar9300TransmitEnableFirst=1;

	error=0;
	if (Ar9300_FieldSelect(devid)<0) {
		ErrorPrint(CardLoadDevid,devid);
		return error;
	}

	if(npcie==0) {
		if (Ar9300pcieDefault(devid)<0) {
			ErrorPrint(CardLoadDevid,devid);
			return error;
		}
	}

/*
    switch (devid)
	{
		case AR9300_DEVID_AR9380_PCIE:			// osprey
		case AR9300_DEVID_EMU_PCIE:
			Ar9300_2_0_FieldSelect();
			if(npcie==0)
			{
				npcie=Ar9380pcieDefault();
			}
			break;

	    case AR9300_DEVID_AR9580_PCIE:			// peacock
			Ar9300_2_0_FieldSelect();
			if(npcie==0)
			{
				npcie=Ar9580pcieDefault();
			}
			break;

#ifndef USE_AQUILA_HAL
		case AR9300_DEVID_AR946X_PCIE:			// jupiter
			Ar9300_FieldSelect_Jupiter();
#ifdef ATH_SUPPORT_MCI
//DOLATERJUPITER			Ar9300MCISetup();
#endif
			if(npcie==0)
			{
				npcie=Ar946XpcieDefault();
			}
			break;
#endif
#ifndef USE_AQUILA_HAL
		case AR9300_DEVID_AR956X_PCIE:			// aphrodite
			Ar9300_FieldSelect_Aphrodite();
#ifdef ATH_SUPPORT_MCI
//DOLATERJUPITER			Ar9300MCISetup();
#endif
			if(npcie==0)
			{
				npcie=Ar956XpcieDefault();
			}
			break;
#endif
		case AR9300_DEVID_AR9485_PCIE:			// poseidon
			Ar9485FieldSelect();
			if(npcie==0)
			{
				npcie=Ar9485pcieDefault();
			}
			break;

		case AR9300_DEVID_AR9330:				// hornet
			Ar9330_FieldSelect();
			if(npcie==0)
			{
				npcie=Ar9330pcieDefault();
			}
			break;

		case AR9300_DEVID_AR9340:				// wasp
			Ar9340FieldSelect();
			if(npcie==0)
			{
				Ar934XpcieDefault();
			}
			break;
		default:
			ErrorPrint(CardLoadDevid,devid);
			error=-1;
			break;
	}
*/
	if(error==0)
	{
	}

	return error;
}


AR9300DLLSPEC int Ar9300Reset(int frequency, unsigned int txchain, unsigned int rxchain, int bandwidth)
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

#ifdef ATH_SUPPORT_PAPRD
			// Call PAPD routine
			if(Ar9300EepromPaPredistortionGet() && Ar9300EepromCalibrationValid())
			{
				struct ath_hal_9300 *ahp = AH9300(AH);
				u_int8_t   txrxMask;

				txrxMask = ahp->ah_eeprom.base_eep_header.txrx_mask;

				papredistortionSingleTable(AH, &channel, ((txrxMask&0xF0)>>4) & txchain);
			}
#endif
		}
	}
	else
	{
		error= -1;
	}
	end=TimeMillisecond();


	return error;
}

AR9300DLLSPEC int Ar9300CalibrationTxgainCAPSet(int *txgainmax)
{
	return Ar9300Eeprom_CalibrationTxgainCAPSet(AH, txgainmax);
}

static int Ar9300PaPredistortionGet(void)
{
    struct ath_hal_9300 *ahp = AH9300(AH);
    return ar9300_eeprom_get(ahp, EEP_PAPRD_ENABLED);
}
/*
double Ar9300CmacPowerGet(int chain)
{
   A_UINT32 cmac_i;
   A_INT16 cmac_power_t10;
   if (chain == 1){
       FieldRead("ch0_cmac_results_i.ate_cmac_results",(unsigned int *)&cmac_i);
   } else {
       FieldRead("ch1_cmac_results_i.ate_cmac_results",(unsigned int *)&cmac_i);
   }
   cmac_power_t10 = cmac2Pwr_t10(cmac_i);
   //printf("cmac_i = %d, cmac_power=%d\n", (unsigned int)cmac_i, cmac_power_t10);

   return (double)(cmac_power_t10 / 10.0);
}	
	
int Ar9300PsatCalibration(int frequency, unsigned int txchain, unsigned int rxchain, int bandwidth)
{
    PSAT_CAL(AH, frequency, txchain, rxchain, bandwidth);
    return 0;
}
	
int Ar9300PsatCalibrationResultGet(int frequency, int chain, int *olpc_dalta, int *thermal, double *cmac_power_olpc, double *cmac_power_psat, unsigned int *olcp_pcdac, unsigned int *psat_pcdac)
{
    psat_cal_channel_result(frequency, chain, olpc_dalta, thermal, cmac_power_olpc, cmac_power_psat,  olcp_pcdac, psat_pcdac);
        
#if 0
    printf("%d, %d, %.1lf, %.1lf, %d, %d\n",
           *olpc_dalta,
           *thermal,
           *cmac_power_olpc,
           *cmac_power_psat,
           *olcp_pcdac,
           *psat_pcdac);
#endif          
    return 1;
}
*/
int Ar9300IsEmbeddedArt(void)
{
    return 0;
}

char *Ar9300IniVersion(int devid)
{
    char buffer[MBUFFER];
    switch (devid)
        {
                case AR9300_DEVID_AR9340:
			{
#ifdef INI_VERSION_AR9340
				strcpy(buffer,INI_VERSION_AR9340);
				return &buffer[4]; // remove '$Id:' from the begnning
#endif
				return "Undefined";
			}
                        break;
                case AR9300_DEVID_AR955X:
			{
#ifdef INI_VERSION_AR955X
				strcpy(buffer,INI_VERSION_AR955X);
				return &buffer[4]; // remove '$Id:' from the begnning
#endif
				return "Undefined";
			}
                        break;
				case AR9300_DEVID_AR956X:
			{
#ifdef INI_VERSION_AR956X
				strcpy(buffer,INI_VERSION_AR956X);
				return &buffer[4]; // remove '$Id:' from the begnning
#endif
				return "Undefined";
			}
						break;
                case AR9300_DEVID_AR953X:
			{
#ifdef INI_VERSION_AR953X
				strcpy(buffer,INI_VERSION_AR953X);
				return &buffer[4]; // remove '$Id:' from the begnning
#endif
				return "Undefined";
			}
                        break;
		default: return "Undefined";
	}

}
static struct _DeviceFunction _Ar9300Device=
{
	Ar9300ChipIdentify,
	Ar9300Name,
	Ar9300Version,
	Ar9300BuildDate,

	Ar9300Attach,
	Ar9300Detach,
	Ar9300Valid,
	Ar9300DeviceIdGet,
	Ar9300Reset,
    Ar9300SetCommand,
    Ar9300SetParameterSplice,
    Ar9300GetCommand,
    Ar9300GetParameterSplice,
    Ar9300BssIdSet,
	Ar9300StationIdSet,

    Ar9300ReceiveDescriptorPointer,
    Ar9300ReceiveUnicast,
    Ar9300ReceiveBroadcast,
    Ar9300ReceivePromiscuous,
    Ar9300ReceiveEnable,
    Ar9300ReceiveDisable,
    Ar9300ReceiveDeafMode,

    Ar9300ReceiveFifo,
    Ar9300ReceiveDescriptorMaximum,
    Ar9300ReceiveEnableFirst,

    Ar9300TransmitFifo,
    Ar9300TransmitDescriptorSplit,
    Ar9300TransmitAggregateStatus,
    Ar9300TransmitEnableFirst,

    Ar9300TransmitDescriptorStatusPointer,
    Ar9300TransmitDescriptorPointer,
    0,										// Ar9300TransmitRetryLimit,
    Ar9300TransmitQueueSetup,
    Ar9300TransmitRegularData,
    Ar9300TransmitFrameData,
    Ar9300TransmitContinuousData,
    Ar9300TransmitCarrier,
    0,										// Ar9300TransmitEnable,
    Ar9300TransmitDisable,

    Ar9300TransmitPowerSet,
    Ar9300TransmitGainSet,
    Ar9300TransmitGainRead,
    Ar9300TransmitGainWrite,

    Ar9300EepromRead,
    Ar9300EepromWrite,
    Ar9300OtpRead,
    Ar9300OtpWrite,
	MyMemoryBase,
	MyMemoryPtr,
    MyMemoryRead,
    MyMemoryWrite,
    MyRegisterRead,
    MyRegisterWrite,
    MyFieldRead,                            //FieldRead
    MyFieldWrite,                           //FieldWrite

    Ar9300ConfigurationRestore,
    Ar9300ConfigurationSave,
    Ar9300CalibrationPierSet,
    Ar9300PowerControlOverride,
    0,										// Ar9300TargetPowerSet,
    Ar9300TargetPowerGet,
    Ar9300TargetPowerApply,

    Ar9300TemperatureGet,
    Ar9300VoltageGet,
    Ar9300MacAddressGet,
    Ar9300CustomerDataGet,

    Ar9300TxChainMany,
	0,										//TxChainMask
    Ar9300RxChainMany,
	0,										//RxChainMask
    Ar9300RxChainSet,

    Ar9300EepromTemplatePreference,
    Ar9300EepromTemplateAllowed,
    Ar9300EepromCompress,
    Ar9300EepromOverwrite,
    Ar9300EepromSize,
    Ar9300EepromSaveMemorySet,
    Ar9300EepromReport,

    Ar9300CalibrationDataAddressSet,
    Ar9300CalibrationDataAddressGet,
    Ar9300CalibrationDataSet,
    Ar9300CalibrationDataGet,
    0,										// Ar9300CalibrationFromEepromFile,
    Ar9300EepromTemplateInstall,
    Ar9300PapdSet,
    Ar9300PaPredistortionGet,
    Ar9300RegulatoryDomainOverride,
    Ar9300RegulatoryDomainGet,
    Ar9300NoiseFloorSet,
    Ar9300NoiseFloorGet,
    Ar9300NoiseFloorPowerSet,
    Ar9300NoiseFloorPowerGet,
	Ar9300SpectralScanEnable,
	Ar9300SpectralScanProcess,
	Ar9300SpectralScanDisable,
    Ar9300NoiseFloorTemperatureSet,
    0,										// Ar9300NoiseFloorTemperatureGet,
//#ifdef ATH_SUPPORT_MCI
	0,		// Ar9300MCISetup,
	0,		// Ar9300MCIReset,
//#endif
	Ar9300TuningCapsSet,
	Ar9300TuningCapsSave,

	Ar9300ChannelCalculate,

	Ar9300ConfigSpaceCommit,
	Ar9300ConfigSpaceUsed,
	Ar9300SubVendorSet,
	Ar9300SubVendorGet,
	Ar9300vendorSet,
	Ar9300vendorGet,
	Ar9300SSIDSet,
	Ar9300SSIDGet,
	Ar9300deviceIDSet,
	Ar9300deviceIDGet,
	Ar9300pcieAddressValueDataSet,
	Ar9300pcieAddressValueDataGet,
	Ar9300pcieMany,
	Ar9300pcieAddressValueDataOfNumGet,
	0,
	0,

	Ar9300NoiseFloorFetch,
	Ar9300NoiseFloorLoad,
	Ar9300NoiseFloorReady,
	Ar9300NoiseFloorEnable,

	Ar9300opFlagsGet,
	Ar9300is2GHz,
	Ar9300is5GHz,
	Ar9300is4p9GHz,
	Ar9300HalfRate,
	Ar9300QuarterRate,

    Ar9300FlashRead,
    Ar9300FlashWrite,

    Ar9300IsEmbeddedArt,
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
    Ar9300CalInfoCalibrationPierSet, //CalInfoCalibrationPierSet
    0, //CalUnusedPierSet
    0, //OtpLoad
    0, //SetConfigParameterSplice
    0, //SetConfigCommand
    0, //StbcGet
    0, //LdpcGet
    0, //PapdGet
    0, //EepromSaveSectionSet
	0, //PapdIsDone

	0, //CalibrationPower
	Ar9300CalibrationTxgainCAPSet,		//	calculate and save tx calibration txgain CAP tx_gain_cap
    Ar9300IniVersion,

	Ar9300TxGainTableRead_AddressGainTable,// used together with Ar9300TransmitGainRead
	Ar9300TxGainTableRead_AddressHeader,   // used together with Ar9300TransmitGainRead
    Ar9300TxGainTableRead_AddressValue,    // used together with Ar9300TransmitGainRead
	Ar9300_get_corr_coeff,
	Ar9300TransmitINIGainGet,				// get the array of ini tx gain table total_gain byte.

	0,										 //SleepMode
	0,										 //DeviceHandleGet

	0,										//XtalReferencePPMGet
    0, //Ar9300CmacPowerGet,                     //CmacPowerGet
    0, //Ar9300PsatCalibrationResultGet,         //PsatCalibrationResultGet
	0,										//DiagData
	0, //GainTableOffset
	0, //CalibrationSetting
    Ar9300PllSceen,
    0 //Ar9300PsatCalibration //PsatCalibration
};


//
// clear all device control function pointers and set to default behavior
//
AR9300DLLSPEC int Ar9300DeviceSelect()
{
	int error;

	DeviceFunctionReset();
	error=DeviceFunctionSelect(&_Ar9300Device);
	if(error!=0)
	{
		return error;
	}
	error=Ar9300TxDescriptorFunctionSelect();
	if(error!=0)
	{
		return error;
	}
	error=Ar9300RxDescriptorFunctionSelect();
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

#ifdef AR9300DLL
AR9300DLLSPEC char *DevicePrefix(void)
{
	return "Ar9300";
}
#endif


