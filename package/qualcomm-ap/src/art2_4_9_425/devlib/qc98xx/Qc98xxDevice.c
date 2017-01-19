/*
 *  Copyright ?2005 Atheros Communications, Inc.,  All Rights Reserved.
 *
 */


#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "wlantype.h"
#include "wlanproto.h"
#include "mlibif.h"
#include "art_utf_common.h"
#include "dk_cmds.h"
#include "tlvCmd_if.h"
#include "qc98xx_eeprom.h"
#include "Qc98xxmEep.h"

#include "LinkLoad.h"
#include "Device.h"
#include "ParameterSelect.h"
#include "Sticky.h"

#include "Qc9KEeprom.h"
#include "Qc9KDevice.h"
#include "Qc9KRegDomain.h"
#include "DevSetConfig.h"
#include "Qc9KSetConfig.h"

#include "mCal98xx.h"
#include "Qc98xxDevice.h"
#include "Qc98xxEepromStructSet.h"
#include "Qc98xxEepromStructGet.h"
#include "Qc98xxEepromSave.h"
#include "Qc98xxField.h"
#include "Qc98xxVersion.h"
#include "Qc98xxNoiseFloor.h"
#include "Qc98xxPcieConfig.h"

#include "Field.h"
#include "MyAccess.h"
#include "rate_constants.h"
#include "vrate_constants.h"
#include "Qc98xxEepromStruct.h"
#include "DevConfigDiff.h"
#include "DevNonEepromParameter.h"
#include "Qc9KRegDomain.h"
#include "DevDeviceFunction.h"
#include "Qc9KEeprom.h"
#include "qc98xxtemplate.h"

#include "TimeMillisecond.h"
#include "UserPrint.h"
#include "ErrorPrint.h"
#include "CardError.h"
#include "MyDelay.h"
#include "sw_version.h"

#ifdef QDART_BUILD
#include "qmslCmd.h"
#define TEST_WITHOUT_CHIP 1
#endif

#define MDCU 10			// should we only set the first 8??
#define MQCU 10
        
#define BW_AUTOMATIC (0)
#define BW_HT40_PLUS (40)
#define BW_HT40_MINUS (-40)
#define BW_HT20 (20)
#define BW_OFDM (19)
#define BW_HALF (10)
#define BW_QUARTER (5)

#define HAL_HT_MACMODE_20       0           /* 20 MHz operation */
#define HAL_HT_MACMODE_2040     1           /* 20/40 MHz operation */

#define QC98XX_DEFAULT_NAME		"QCA ART2 for Peregrine(AR9888)"

extern PIPE_CMD GlobalCmd;

static A_UCHAR  bssID[6]     = {0x50, 0x55, 0x55, 0x55, 0x55, 0x05};
static A_UCHAR  txStation[6] = {0x20, 0x22, 0x24, 0x26, 0x00, 0x00};	// Golden

static int _Qc98xxValid = 0;

#if AP_BUILD
extern void Qc98xx_swap_eeprom(QC98XX_EEPROM *eep);
#endif

#define MCHANNEL 2000

// for debugging to reduce calibration time only!!
// 

#define TRACE_START()	
//UserPrint("%s\n", __FUNCTION__)
//#define TRACE_START()	UserPrint("File %s, Line %d\n", __FILE__, __LINE__)
#define TRACE_END   TRACE_START

int Qc98xxChannelCalculate(int *frequency, int *option, int mchannel)
{
	int nchannel;
	int channels[MCHANNEL];
    int flags[MCHANNEL];
   	A_UINT32 modeSelect;
   	A_UINT32 regDmn[2];
    A_UINT8 regclassids[MCHANNEL];
    A_UINT32 maxchans, maxregids, nregids;
    A_UINT16 cc;
   	int error, num;
	int it;
   	//
   	// try calling this here
   	//
   	//Fcain need to handle this better, should not be accessing struct values directly
	TRACE_START();

    Qc98xxRegDmnGet((int *)regDmn, -1, &num);
   	if( regDmn[0] != 0) 
   	{
        Qc9KRegulatoryDomainOverride(0);
   	}		
   	if(regDmn[0] & 0x8000)
    {
   		cc=(A_UINT16)(regDmn[0] & 0x3ff);
    }
    else
    {
        cc=CTRY_DEBUG;		//CTRY_DEFAULT;			// changed from CTRY_DEBUG since that doesn't work if regdmn is set
    }  	
    modeSelect=0xffffffff;
    maxchans=sizeof(channels)/sizeof(channels[0]);
   	maxregids=sizeof(regclassids)/sizeof(regclassids[0]);
   
    memset (channels, 0, sizeof(channels));
    memset (flags, 0, sizeof(flags));
    error = Qc9KInitChannels(channels, flags, maxchans, &nchannel,
                            regclassids, maxregids, &nregids,
                            cc, modeSelect, 0, 0);
   
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
		frequency[it] = channels[it];
		option[it] = flags[it];
	}
   	return nchannel;
}
 
int Qc98xxOtpRead(unsigned int address, unsigned char *buffer, int many, int is_wifi)
{
	TRACE_START();
    if (art_efuseRead(buffer, (A_UINT32 *)&many, address) == A_OK)
    {
        return 0;
    }
    return (-1);
}

int Qc98xxOtpWrite(unsigned int address, unsigned char *buffer, int many, int is_wifi)
{
	TRACE_START();
    if (art_efuseWrite(buffer, many, address) == A_OK)
    {
        return 0;
    }
    return (-1);
}

int Qc98xxOtpLoad()
{
	TRACE_START();
	if (Qc98xxLoadOtp())
	{
		// update Qc98xxEepromBoardArea with OTP data
		memcpy (Qc98xxEepromBoardArea, Qc98xxEepromArea, sizeof(QC98XX_EEPROM));
		return 0;
	}
	else return (-1);
}

QC98XXDLLSPEC int Qc98xxBssIdSet(unsigned char *bssid)
{
	unsigned int reg;
    unsigned int address1, address2;
    int low1, low2, high1, high2;

	TRACE_START();
    if (FieldFind("MAC_PCU_BSSID_L32", &address1, &low1, &high1) &&
        FieldFind("MAC_PCU_BSSID_U16", &address2, &low2, &high2))
    {
        reg=bssid[3]<<24|bssid[2]<<16|bssid[1]<<8|bssid[0];
        MyRegisterWrite(address1,reg);

	    reg = (bssid[5]<<8|bssid[4]);
        MyFieldWrite(address2, low2, high2, reg);
    
	    return 0;
    }
    return (-1);
}



static int HeavyClipValueRead = 0;
static int HeavyClipCurVal = 0;
static const char const *heavy_clip_reg_name = "BB_heavy_clip_1";
static const char const *heavy_clip_field_name = "heavy_clip_enable";

// if disable is true, then we write the register to disable
// else we restore the previous value which was read
QC98XXDLLSPEC int Qc98xxHeavyClipEnableSet(unsigned int enable)
{
	if (FieldWrite("heavy_clip_enable", enable) != 0)
	{
		UserPrint("Error: Could not write to heavy clip\n");
		return -1;
	}
	return 0;
}

QC98XXDLLSPEC int Qc98xxHeavyClipEnableGet(unsigned int *enabled)
{
	if (FieldRead("heavy_clip_enable", (unsigned int *)enabled) != 0)
	{
		UserPrint("Error: Unable to read heavy clip value\n");
		return -1;
	}

	return 0;
}

int Qc98xxStationIdSet(unsigned char *mac)
{
	unsigned int reg;
    unsigned int address1, address2;
    int low1, low2, high1, high2;

	TRACE_START();

    if (FieldFind("MAC_PCU_STA_ADDR_L32", &address1, &low1, &high1) &&
        FieldFind("MAC_PCU_STA_ADDR_U16.ADDR_47_32", &address2, &low2, &high2))
    {
	    reg=mac[3]<<24|mac[2]<<16|mac[1]<<8|mac[0];
        MyRegisterWrite(address1,reg);

	    reg = (mac[5]<<8|mac[4]);
        MyFieldWrite(address2, low2, high2, reg);
	    return 0;
    }
   return -1;
}

QC98XXDLLSPEC int Qc98xxTargetPowerGet(int frequency, int rate, double *power)
{
    A_BOOL  is2GHz = 0;
    A_UINT8 powerT2 = 0;

	TRACE_START();
    if(frequency < 4000) {
        is2GHz = 1;
    }
    //call the relevant get target power file based on rateIndex.  
    //Rate index will be used to find the appropriate target power index
    switch (rate) {
        //Legacy
        case RATE_INDEX_6:
        case RATE_INDEX_9:
        case RATE_INDEX_12:
        case RATE_INDEX_18:
        case RATE_INDEX_24:
            powerT2 = Qc98xxEepromGetLegacyTrgtPwr(LEGACY_TARGET_RATE_6_24, frequency, is2GHz);            
            break;
        case RATE_INDEX_36:
            powerT2 = Qc98xxEepromGetLegacyTrgtPwr(LEGACY_TARGET_RATE_36, frequency, is2GHz);            
            break;
        case RATE_INDEX_48:
            powerT2 = Qc98xxEepromGetLegacyTrgtPwr(LEGACY_TARGET_RATE_48, frequency, is2GHz);            
            break;
        case RATE_INDEX_54:
            powerT2 = Qc98xxEepromGetLegacyTrgtPwr(LEGACY_TARGET_RATE_54, frequency, is2GHz);            
            break;

        //Legacy CCK
        case RATE_INDEX_1L:
        case RATE_INDEX_2L:
        case RATE_INDEX_2S:
        case RATE_INDEX_5L:
            powerT2 = Qc98xxEepromGetCckTrgtPwr(LEGACY_TARGET_RATE_1L_5L, frequency);
            break;
        case RATE_INDEX_5S:
            powerT2 = Qc98xxEepromGetCckTrgtPwr(LEGACY_TARGET_RATE_5S, frequency);
            break;
        case RATE_INDEX_11L:
            powerT2 = Qc98xxEepromGetCckTrgtPwr(LEGACY_TARGET_RATE_11L, frequency);
            break;
        case RATE_INDEX_11S:
            powerT2 = Qc98xxEepromGetCckTrgtPwr(LEGACY_TARGET_RATE_11S, frequency);
            break;
        
        //HT20
        case RATE_INDEX_HT20_MCS0:
        case RATE_INDEX_HT20_MCS8:
        case RATE_INDEX_HT20_MCS16:
		case vRATE_INDEX_HT20_MCS0:
		case vRATE_INDEX_HT20_MCS10:
		case vRATE_INDEX_HT20_MCS20:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS0_10_20, frequency, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS1:
        case RATE_INDEX_HT20_MCS2:
        case RATE_INDEX_HT20_MCS9:
        case RATE_INDEX_HT20_MCS10:
        case RATE_INDEX_HT20_MCS17:
        case RATE_INDEX_HT20_MCS18:
		case vRATE_INDEX_HT20_MCS1:
		case vRATE_INDEX_HT20_MCS2:
		case vRATE_INDEX_HT20_MCS11:
		case vRATE_INDEX_HT20_MCS12:
		case vRATE_INDEX_HT20_MCS21:
		case vRATE_INDEX_HT20_MCS22:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS1_2_11_12_21_22, frequency, is2GHz);     
            break;
        case RATE_INDEX_HT20_MCS3:
        case RATE_INDEX_HT20_MCS4:
        case RATE_INDEX_HT20_MCS11:
        case RATE_INDEX_HT20_MCS12:
        case RATE_INDEX_HT20_MCS19:
        case RATE_INDEX_HT20_MCS20:
		case vRATE_INDEX_HT20_MCS3:
		case vRATE_INDEX_HT20_MCS4:
		case vRATE_INDEX_HT20_MCS13:
		case vRATE_INDEX_HT20_MCS14:
		case vRATE_INDEX_HT20_MCS23:
		case vRATE_INDEX_HT20_MCS24:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS3_4_13_14_23_24, frequency, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS5:
		case vRATE_INDEX_HT20_MCS5:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS5, frequency, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS6:
		case vRATE_INDEX_HT20_MCS6:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS6, frequency, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS7:
		case vRATE_INDEX_HT20_MCS7:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS7, frequency, is2GHz);
            break;
        case vRATE_INDEX_HT20_MCS8:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS8, frequency, is2GHz);
            break;
        case vRATE_INDEX_HT20_MCS9:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS9, frequency, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS13:
		case vRATE_INDEX_HT20_MCS15:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS15, frequency, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS14:
 		case vRATE_INDEX_HT20_MCS16:
           powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS16, frequency, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS15:
		case vRATE_INDEX_HT20_MCS17:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS17, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT20_MCS18:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS18, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT20_MCS19:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS19, frequency, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS21:
		case vRATE_INDEX_HT20_MCS25:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS25, frequency, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS22:
 		case vRATE_INDEX_HT20_MCS26:
           powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS26, frequency, is2GHz);
            break;
        case RATE_INDEX_HT20_MCS23:
		case vRATE_INDEX_HT20_MCS27:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS27, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT20_MCS28:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS28, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT20_MCS29:
            powerT2 = Qc98xxEepromGetHT20TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS29, frequency, is2GHz);
            break;
        
        //HT40
        case RATE_INDEX_HT40_MCS0:
        case RATE_INDEX_HT40_MCS8:
        case RATE_INDEX_HT40_MCS16:
		case vRATE_INDEX_HT40_MCS0:
		case vRATE_INDEX_HT40_MCS10:
		case vRATE_INDEX_HT40_MCS20:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS0_10_20, frequency, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS1:
        case RATE_INDEX_HT40_MCS2:
        case RATE_INDEX_HT40_MCS9:
        case RATE_INDEX_HT40_MCS10:
        case RATE_INDEX_HT40_MCS17:
        case RATE_INDEX_HT40_MCS18:
		case vRATE_INDEX_HT40_MCS1:
		case vRATE_INDEX_HT40_MCS2:
		case vRATE_INDEX_HT40_MCS11:
		case vRATE_INDEX_HT40_MCS12:
		case vRATE_INDEX_HT40_MCS21:
		case vRATE_INDEX_HT40_MCS22:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS1_2_11_12_21_22, frequency, is2GHz);     
            break;
        case RATE_INDEX_HT40_MCS3:
        case RATE_INDEX_HT40_MCS4:
        case RATE_INDEX_HT40_MCS11:
        case RATE_INDEX_HT40_MCS12:
        case RATE_INDEX_HT40_MCS19:
        case RATE_INDEX_HT40_MCS20:
		case vRATE_INDEX_HT40_MCS3:
		case vRATE_INDEX_HT40_MCS4:
		case vRATE_INDEX_HT40_MCS13:
		case vRATE_INDEX_HT40_MCS14:
		case vRATE_INDEX_HT40_MCS23:
		case vRATE_INDEX_HT40_MCS24:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS3_4_13_14_23_24, frequency, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS5:
		case vRATE_INDEX_HT40_MCS5:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS5, frequency, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS6:
		case vRATE_INDEX_HT40_MCS6:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS6, frequency, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS7:
		case vRATE_INDEX_HT40_MCS7:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS7, frequency, is2GHz);
            break;
        case vRATE_INDEX_HT40_MCS8:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS8, frequency, is2GHz);
            break;
        case vRATE_INDEX_HT40_MCS9:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS9, frequency, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS13:
		case vRATE_INDEX_HT40_MCS15:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS15, frequency, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS14:
 		case vRATE_INDEX_HT40_MCS16:
           powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS16, frequency, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS15:
		case vRATE_INDEX_HT40_MCS17:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS17, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT40_MCS18:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS18, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT40_MCS19:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS19, frequency, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS21:
		case vRATE_INDEX_HT40_MCS25:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS25, frequency, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS22:
 		case vRATE_INDEX_HT40_MCS26:
           powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS26, frequency, is2GHz);
            break;
        case RATE_INDEX_HT40_MCS23:
		case vRATE_INDEX_HT40_MCS27:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS27, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT40_MCS28:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS28, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT40_MCS29:
            powerT2 = Qc98xxEepromGetHT40TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS29, frequency, is2GHz);
            break;

		//HT80
		case vRATE_INDEX_HT80_MCS0:
		case vRATE_INDEX_HT80_MCS10:
		case vRATE_INDEX_HT80_MCS20:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS0_10_20, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT80_MCS1:
		case vRATE_INDEX_HT80_MCS2:
		case vRATE_INDEX_HT80_MCS11:
		case vRATE_INDEX_HT80_MCS12:
		case vRATE_INDEX_HT80_MCS21:
		case vRATE_INDEX_HT80_MCS22:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS1_2_11_12_21_22, frequency, is2GHz);     
            break;
		case vRATE_INDEX_HT80_MCS3:
		case vRATE_INDEX_HT80_MCS4:
		case vRATE_INDEX_HT80_MCS13:
		case vRATE_INDEX_HT80_MCS14:
		case vRATE_INDEX_HT80_MCS23:
		case vRATE_INDEX_HT80_MCS24:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS3_4_13_14_23_24, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT80_MCS5:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS5, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT80_MCS6:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS6, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT80_MCS7:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS7, frequency, is2GHz);
            break;
        case vRATE_INDEX_HT80_MCS8:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS8, frequency, is2GHz);
            break;
        case vRATE_INDEX_HT80_MCS9:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS9, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT80_MCS15:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS15, frequency, is2GHz);
            break;
 		case vRATE_INDEX_HT80_MCS16:
           powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS16, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT80_MCS17:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS17, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT80_MCS18:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS18, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT80_MCS19:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS19, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT80_MCS25:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS25, frequency, is2GHz);
            break;
 		case vRATE_INDEX_HT80_MCS26:
           powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS26, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT80_MCS27:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS27, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT80_MCS28:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS28, frequency, is2GHz);
            break;
		case vRATE_INDEX_HT80_MCS29:
            powerT2 = Qc98xxEepromGetHT80TrgtPwr(rate, WHAL_VHT_TARGET_POWER_RATES_MCS29, frequency, is2GHz);
            break;
		
		default:
			UserPrint("Qc98xxTargetPowerGet - Invalid rate index %d\n", rate);
			return -1;
    }
    *power = ((double)powerT2)/2;
    return 0;
}

//int Qc98xxTargetPowerSetFunction(/* arguments????*/)
#define NUM_TEMP_READINGS 30

int Qc98xxTemperatureGet(int forceTempRead)
{
    int therm_value = 0;

	TRACE_START();
	if (forceTempRead == 1) {
        // McKinley 1.0 bug
//        FieldWrite("rxtx_b1_RXTX4.thermOn", 0);
//        FieldWrite("rxtx_b1_RXTX4.thermOn_ovr", 1);

        FieldRead("BB_therm_adc_4.latest_therm_value", &therm_value);
	}
    return therm_value;

}

int Qc98xxVoltageGet(void)
{
    return 0;
    //return (PHY_BB_THERM_ADC_4_LATEST_VOLT_VALUE_GET(REGR(devNum, PHY_BB_THERM_ADC_4_ADDRESS)));
}

int Qc98xxRxChainSet(int rxchain)
{
    return 0;
}

int Qc98xxTransmitDataDut(void *params)
{
	TRACE_START();
    art_txDataStart ((TX_DATA_START_PARAMS *)params);
    return 0;
}

int Qc98xxTransmitStatusReport(void **txStatus, int stop)
{
	TRACE_START();
    return art_txStatusReport(txStatus, stop);
}

int Qc98xxTransmitStop(void **txStatus, int calibrate)
{
	TRACE_START();
    return art_txDataStop(txStatus, calibrate);
}

int Qc98xxReceiveDataDut(void *params)
{
	TRACE_START();
    art_rxDataStart ((RX_DATA_START_PARAMS *)params);
    return 0;
}

int Qc98xxReceiveStatusReport(void **rxStatus, int stop)
{
	TRACE_START();
    return art_rxStatusReport(rxStatus, stop);
}

int Qc98xxReceiveStop(void **rxStatus)
{
	TRACE_START();
    return art_rxDataStop(rxStatus);
}

int Qc98xxNumEepromModalSpursGet()
{
	TRACE_START();
    return QC98XX_EEPROM_MODAL_SPURS;
}

int Qc98xxNum2gCckTargetPowersGet(void)
{
	TRACE_START();
    return WHAL_NUM_11B_TARGET_POWER_CHANS;//2
}
int Qc98xxNum2g20TargetPowersGet(void)
{
	TRACE_START();
    return WHAL_NUM_11G_20_TARGET_POWER_CHANS;//3
}
int Qc98xxNum5g20TargetPowersGet(void)
{
	TRACE_START();
    return WHAL_NUM_11A_20_TARGET_POWER_CHANS;//8
}
int Qc98xxNum2g40TargetPowersGet(void)
{
	TRACE_START();
    return WHAL_NUM_11G_40_TARGET_POWER_CHANS;//3
}
int Qc98xxNum5g40TargetPowersGet(void)
{
	TRACE_START();
    return WHAL_NUM_11A_40_TARGET_POWER_CHANS;//8
}

int Qc98xxNumCtls2gGet()
{
	TRACE_START();
    return WHAL_NUM_CTLS_2G;
}

int Qc98xxNumCtls5gGet()
{
	TRACE_START();
    return WHAL_NUM_CTLS_5G;
}

int Qc98xxNumBandEdges2gGet()
{
	TRACE_START();
    return WHAL_NUM_BAND_EDGES_2G;
}

int Qc98xxNumBandEdges5gGet()
{
	TRACE_START();
    return WHAL_NUM_BAND_EDGES_5G;
}

A_BOOL readEepromFile()
{ 
    A_UINT32 numBytes;

    numBytes = 0;

	TRACE_START();
	if(configSetup.eepromFile[0] == '\0')
	{
        UserPrint("Error no EEPROM file defined. Use \"setconfig EEPROM_FILE\" to define\n");
		return FALSE;
	}
    else
    {
        if (!readCalDataFromFile(configSetup.eepromFile, (QC98XX_EEPROM *)pQc9kEepromArea, &numBytes))
        {
            UserPrint("Error reading EEPROM file %s \n", configSetup.eepromFile);
            return FALSE;
        }
    }
    return TRUE;
}

int Qc98xxCalibrationFromEepromFile(void)
{ 
	TRACE_START();
    pQc9kEepromArea = Qc98xxEepromArea;
    pQc9kEepromBoardArea = Qc98xxEepromBoardArea;
    if (readEepromFile() == FALSE)
    {
        return -1;
    }
    
	if (qc98xxEepromAttach() == FALSE)
    {
        return -1;
    }

	memcpy (Qc98xxEepromBoardArea, Qc98xxEepromArea, sizeof(QC98XX_EEPROM));
    return 0;
}

#ifdef AP_BUILD
int Qc98xxCalibrationFromFlash(void)
{ 
    int fd;
    int offset;
    int numBytes = sizeof(QC98XX_EEPROM);
    QC98XX_EEPROM *pQc98xxData;
    pQc9kEepromArea = Qc98xxEepromArea;
    pQc9kEepromBoardArea = Qc98xxEepromBoardArea;
    if((fd = open("/dev/caldata", O_RDWR)) < 0) {
            perror("Could not open flash\n");
            return -1;
    }

    // First 0x1000 are reserved for ethernet mac address and
    // other board config data.
    offset = MAX_EEPROM_SIZE + FLASH_BASE_CALDATA_OFFSET;
    lseek(fd, offset, SEEK_SET);
    if (read(fd, (void *)pQc9kEepromArea, numBytes) != numBytes ) {
        perror("\n_read\n");
        close(fd);
        return -1;
    }
    close(fd);

    pQc98xxData = (QC98XX_EEPROM *)pQc9kEepromArea;
    Qc98xx_swap_eeprom(pQc98xxData);

    if (numBytes != pQc98xxData->baseEepHeader.length)
    {
        UserPrint("ERROR - File size of %d NOT match eepromData->baseEepHeader.length %d\n", numBytes, pQc98xxData->baseEepHeader.length);
        return -1;
    }
    
	if (qc98xxEepromAttach() == FALSE)
    {
        return -1;
    }

	memcpy (Qc98xxEepromBoardArea, Qc98xxEepromArea, sizeof(QC98XX_EEPROM));
    return 0;
}

#endif

int Qc98xxCalibrationFromTemplate(void)
{
	QC98XX_EEPROM *pTemplate;
	

	int templateId = Qc98xxGetEepromTemplatePreference();
	
	TRACE_START();

	pTemplate =	Qc98xxEepromStructDefaultFindById(templateId);
	if (pTemplate == NULL)
	{
		UserPrint("Qc98xxCalibrationFromTemplate - Could not find template with ID %d, use the generic one\n", templateId);
		pTemplate = Qc98xxEepromStructDefaultFindById(qc98xx_eeprom_template_base);
	}
	UserPrint("Qc98xxCalibrationFromTemplate - load template %s\n", Qc98xxGetTemplateNameGivenVersion(pTemplate->baseEepHeader.template_version));
	memcpy (Qc98xxEepromArea, pTemplate, sizeof(QC98XX_EEPROM));

	pQc9kEepromArea = Qc98xxEepromArea;
    pQc9kEepromBoardArea = Qc98xxEepromBoardArea;

	if (qc98xxEepromAttach() == FALSE)
    {
        return -1;
    }

	memcpy (Qc98xxEepromBoardArea, Qc98xxEepromArea, sizeof(QC98XX_EEPROM));
    return 0;
}

int Qc98xxCalibrationFromOtp(void)
{
	TRACE_START();
    pQc9kEepromArea = Qc98xxEepromArea;
    pQc9kEepromBoardArea = Qc98xxEepromBoardArea;
    if (Qc98xxLoadOtp() == FALSE)
    {
        return -1;
    }
	if (qc98xxEepromAttach() == FALSE)
    {
        return -1;
    }
	memcpy (Qc98xxEepromBoardArea, Qc98xxEepromArea, sizeof(QC98XX_EEPROM));
	return 0;
}

int Qc98xxValid(void)
{
	TRACE_START();
    return _Qc98xxValid;
}

QC98XXDLLSPEC int Qc98xxIsEmbeddedArt(void)
{
	TRACE_START();
    return 1;
}

QC98XXDLLSPEC int Qc98xxIs11ACDevice(void)
{
	TRACE_START();
    return 1;
}

int Qc98xxReset(int frequency, unsigned int txchain, unsigned int rxchain, int bandwidth)
{
	int start,end;
	int error;
	//HAL_OPMODE opmode;
	//int htmode;
    //HAL_HT_EXTPROTSPACING exprotspacing;
	//HAL_BOOL bChannelChange;
	//int isscan;
    //A_UINT32 turbo;

	TRACE_START();
    StickyExecuteDut(0); // For embedded ART, send sticky list to DUT before reset device
    //
	// do it
	//
	start=TimeMillisecond();
        
#ifdef UNUSED
    htmode = HAL_HT_MACMODE_20;
    if (bandwidth == BW_HT40_PLUS || bandwidth == BW_HT40_MINUS)
    {
        htmode = HAL_HT_MACMODE_2040;
    }
    
    turbo = 0;
    if (bandwidth == BW_QUARTER)
    {
        turbo = QUARTER_SPEED_MODE;
    }
    else if (bandwidth == BW_HALF)
    {
        turbo = HALF_SPEED_MODE;
    }
    
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

    if(frequency<4000)		// let's presume this is B/G
	{
	    if(ht40>0)
		{
		    channel.channelFlags=CHANNEL_2GHZ|CHANNEL_HT40PLUS;
		}
	    else if(ht40<0)
		{
		    channel.channelFlags=CHANNEL_2GHZ|CHANNEL_HT40MINUS;
		}
		else
		{
            channel.channelFlags=CHANNEL_2GHZ|CHANNEL_HT20; 
		}
	}
	else
	{
	    if(ht40>0)
		{
		    channel.channelFlags=CHANNEL_5GHZ|CHANNEL_HT40PLUS;
		}
	    else if(ht40<0)
		{
		    channel.channelFlags=CHANNEL_5GHZ|CHANNEL_HT40MINUS;
		}
		else
		{
            channel.channelFlags=CHANNEL_5GHZ|CHANNEL_HT20; 
		}
	}
	if(ht40)
	{
		htmode=HAL_HT_MACMODE_2040;
	}
	else
	{
		htmode=HAL_HT_MACMODE_20;
	}
    exprotspacing=HAL_HT_EXTPROTSPACING_20;

	bChannelChange=0;           // fast channel change is broken in HAL/osprey
	if(ResetForce)
	{
		bChannelChange=0;
	}
#endif //UNUSED
	//error=0;
	//isscan=0;
	//if(AH!=0 && AH->ah_reset!=0)
	//{

	    //qc98xxReset(opmode,&channel,htmode,txchain,rxchain,exprotspacing,bChannelChange,(HAL_STATUS*)&error,isscan);
        error = art_whalResetDevice(txStation, bssID, frequency, bandwidth, rxchain, txchain);
        //if(error==0)
		//{
		//	ResetDone=1;
		//}
	//}
	//else
	//{
	//	error= -1;
	//}
	end=TimeMillisecond();
	
	return error;
}

int qc98xxChainMany (A_BOOL txChain)
{
	int i, chainMask, numChain;

	TRACE_START();
    if (txChain)
	{
		chainMask = Qc98xxTxMaskGet();
	}
	else
	{
		chainMask = Qc98xxRxMaskGet();
	}
	numChain = 0;
	for (i = 0; i < 4; ++i)
	{
		if (((chainMask >> i) & 1) == 1)
		{
			numChain++;
		}
	}
	return numChain;
}

int Qc98xxTxChainMany(void)
{
	TRACE_START();
    return qc98xxChainMany(TRUE);
}

int Qc98xxRxChainMany(void)
{
	TRACE_START();
    return qc98xxChainMany(FALSE);
}

int Qc98xxCalInfoInit()
{
	TRACE_START();
    CalInfoInit();
    return 0;
}

int Qc98xxXtalValue(int caps)
{
  FieldWrite("top_wlan_XTALWLAN.xtal_capindac", caps);
  FieldWrite("top_wlan_XTALWLAN.xtal_capoutdac", caps);
  return 0;
}

int Qc98xxTuningCapsSet(int caps)
{
	Qc98xxXtalValue(caps);
	return 0;
}

int Qc98xxTuningCapsSave(int caps)
{
	Qc98xxXtalValue(caps);
	Qc98xxPwrTuningCapsParamsSet(&caps, 0, 0, 0, 1, 0);
	return 0;
}

int Qc98xxEepromSize(void)
{
	TRACE_START();
    return 0;
}

int Qc98xxEepromStructSize(void)
{
	TRACE_START();
    return sizeof(QC98XX_EEPROM);
}

int Qc98xxIs2GHz (void)
{	
	return ((Qc98xxEepromStructGet()->baseEepHeader.opCapBrdFlags.opFlags & WHAL_OPFLAGS_11G) > 0 ? 1 : 0);
}

int Qc98xxIs5GHz (void)
{
    unsigned int value;
    TRACE_START();

	if (Qc98xxEepromStructGet()->baseEepHeader.opCapBrdFlags.opFlags & WHAL_OPFLAGS_11A)
	{
		FieldRead("ENTERPRISE_CONFIG_OVRD.DUAL_BAND_DISABLE", &value);
		return (value ? 0 : 1);
	}
	else
	{
		return 0;
	}
}

int Qc98xxIs4p9GHz(void)    
{
	TRACE_START();
    return 1;
}

int Qc98xxHalfRate(void)    
{
    unsigned int value;
	TRACE_START();

    FieldRead("ENTERPRISE_CONFIG_OVRD.CH_5MHZ_DISABLE", &value);
    return (value ? 0 : 1);
}

int Qc98xxQuarterRate(void)     
{
    unsigned int value;
	TRACE_START();
    
	FieldRead("ENTERPRISE_CONFIG_OVRD.CH_10MHZ_DISABLE", &value);
    return (value ? 0 : 1);
}

int Qc98xxCustomNameGet(char *name)     
{
	TRACE_START();

	if(!strcmp(configSetup.customName,""))
	{
		strcpy(name, QC98XX_DEFAULT_NAME);
	}
	else
	{
		strcpy(name, configSetup.customName);
	}
	return 0;
}

int Qc98xxPapdIsDone(int txPwr)
{
    unsigned int ch0_is_done=0, ch1_is_done=0;
    int count = 0;
	TRACE_START();
    
    if (txPwr > 12)
    {
        do {
            if (count)
                MyDelay(1);
            if (!ch0_is_done){
                FieldRead("BB_paprd_ctrl0_b0.paprd_enable_0", &ch0_is_done);
            }
            if (!ch1_is_done){
                FieldRead("BB_paprd_ctrl0_b1.paprd_enable_1", &ch1_is_done);
            }
            count++;
            UserPrint("count = %d ch0_done=%d ch1_done=%d\n", count, ch0_is_done, ch1_is_done);
            if ((ch0_is_done == 1) && (ch1_is_done == 1))
                MyDelay(600);
        } while (((ch0_is_done == 0) || (ch1_is_done == 0)) && (count <= 500));
    }
    if (count > 500)
        return 0;
    else
        return 1;
}

#ifdef AP_BUILD
int Qc98xxSwapCalStruct(void)
{
    QC98XX_EEPROM *mptr;
    mptr = Qc98xxEepromStructGet();
    Qc98xx_swap_eeprom(mptr);
    return 0;
}
#endif

int Qc98xxAttach(int devid, int calmem)
{
    //HAL_ADAPTER_HANDLE osdev;
	//HAL_SOFTC sc;
	//HAL_BUS_TAG st;
	//HAL_BUS_HANDLE sh;
	//HAL_BUS_TYPE bustype;
    //struct hal_reg_parm hal_conf_parm;
	//HAL_STATUS error;
    int error;
	int start,end;
	//int it;
    //char buffer[MBUFFER];
	int caluse;
	int eepsize;
	TRACE_START();

//#ifdef _WINDOWS
//	if(Qc9KReEnableDevice())
//	{
//		UserPrint("Note: Re-enable device failed.\r\n");
//	}
//#endif
    // Setup the device functions and call the functions that are required before loading the device
    //DevicePreInitialize(devid);
	//DeviceUserPrintConsoleSet(_UserPrintConsole);

	//
	// connect to the ANWI driver
	//
    //devid=AnwiDriverAttach(devid); 
	qc98xxInitTemplateTbl();
    Qc98xxCalibrationDataSet(calmem);   // calmem is needed in Qc9KCardLoad
#ifdef TEST_WITHOUT_CHIP
	error = 0;
#else
    error=Qc9KCardLoad();
#endif
    if(error!=0) 
    {
		ErrorPrint(CardLoadAnwi);
        return -2;
    }

	ConfigDiffInit();
    CalInfoInit();

	start=TimeMillisecond();
	error=0;

    //AH = qc98xxAttach(devid, osdev, sc, st, sh, bustype, amem_handle, &hal_conf_parm, &error);

	//if(error!=0)
	//{
	//	ErrorPrint(CardLoadAttach,error);
	//	return error;
	//}
	//if(AH==0)
	//{
	//    ErrorPrint(CardLoadHal);
	//	return -4;
	//}
	end=TimeMillisecond();
	UserPrint("ath_hal_attach duration: %d=%d-%d ms\n",end-start,end,start);
    
    // Setup device
	//UserPrintConsole(1);
    Qc98xxFieldSelect();

    //caluse = calmem;
    
	switch(calmem)
	{
		case DeviceCalibrationDataNone:
			ErrorPrint(CardLoadCalibrationNone);
			eepsize=DeviceEepromSize();
			if(eepsize>0)
			{
				Qc98xxCalibrationDataSet(DeviceCalibrationDataEeprom);
			}
			else
			{
				Qc98xxCalibrationDataSet(DeviceCalibrationDataOtp);
			}
			break;
		case DeviceCalibrationDataFlash:
			ErrorPrint(CardLoadCalibrationFlash);
			break;
		case DeviceCalibrationDataEeprom:
			ErrorPrint(CardLoadCalibrationEeprom,DeviceCalibrationDataAddressGet());
			break;
		case DeviceCalibrationDataOtp:
			ErrorPrint(CardLoadCalibrationOtp,DeviceCalibrationDataAddressGet());
			break;
        case DeviceCalibrationDataFile:
			ErrorPrint(CardLoadCalibrationFile);
			break;
	}

    Qc98xxPcieAddressValueDataInit();

	caluse = Qc98xxCalibrationDataGet();
	if (caluse == DeviceCalibrationDataFile)
	{
 		if (Qc98xxCalibrationFromEepromFile() != 0)
		{
			ErrorPrint(CardLoadCalibrationFile);
			Qc9KCardRemove();
			return -5;
		}
	}
	else if (caluse == DeviceCalibrationDataOtp)
	{
		if (Qc98xxCalibrationFromOtp() != 0)
		{
			ErrorPrint(CardLoadCalibrationOtp);
			// if nothing from otp, get board data from template
			if (Qc98xxCalibrationFromTemplate() != 0)
			{
				ErrorPrint(CardLoadCalibrationNone);
				Qc9KCardRemove();
				return -5;
			}
		}
	}
	else if (caluse == DeviceCalibrationDataFlash)
	{
#ifdef AP_BUILD
        if (Qc98xxCalibrationFromFlash() != 0)
        {
            ErrorPrint(CardLoadCalibrationFlash);
            Qc9KCardRemove();
            return -5;
        }
#else
       // Read from flash for STA card is not valid  
       ErrorPrint(CardLoadCalibrationFlash);
       Qc9KCardRemove();
       return -5;
#endif

	}
	else
	{
		if (Qc98xxCalibrationFromTemplate() != 0)
		{
			ErrorPrint(CardLoadCalibrationNone);
			Qc9KCardRemove();
			return -5;
		}
	}

	/*if (ConfigurationInit()!=0) 
	{
		ErrorPrint(CardLoadPcie);
		return -3;
    }
	//
    //
    // FROM HERE ON WE CONFIGURE THE NART CODE
    //
	CalibrateClear();*/

    Qc9KRegDomainInit();
    
    _Qc98xxValid = 1;
	Qc98xxCalibrationDataSet(caluse);

    return 0;
}
 
int Qc98xxDetach(void)
{
    int status;
	TRACE_START();

    ConfigDiffClearAll();
    CalInfoClearAll();
    status = Qc9KCardRemove();
    _Qc98xxValid = 0;
	Qc9KDisableDevice ();
    return status;
    //return AnwiDriverDetach(); 
}

int Qc98xxTxGainTableRead_AddressGainTable(unsigned int **address, unsigned int *row, unsigned int *col)
{
	return 0;
}

int Qc98xxSleepMode (int mode)
{
	TRACE_START();

	return art_sleepMode(mode);
}

int Qc98xxDeviceHandleGet (unsigned int *handle)
{
	TRACE_START();

	return art_getDeviceHandle(handle);
}

int Qc98xxIsVersion1()
{
	TRACE_START();

    return ((configSetup.SwVersion & VER_MINOR_BIT_MASK) == 0 ? TRUE : FALSE);
}

int Qc98xxReadPciConfigSpace(unsigned int offset, unsigned int *value)
{
	TRACE_START();

    *value = art_readPciConfigSpace(offset);
    return 0;
}

int Qc98xxWritePciConfigSpace(unsigned int offset, unsigned int value)
{
	TRACE_START();

    // if v1, return since there is no effect in wrting to PCIE config space since the patchs prevent it
    if (Qc98xxIsVersion1())
    {
        UserPrint("Write to PCIE config space is not supported.\n");
        return 0;
    }

    if (offset < 4) //devid/vid
    {
        UserPrint("Not allowed to write to %d offset.\n", offset);
        return -1;
    }
    return art_writePciConfigSpace(offset, value);
}

int Qc98xxDiagData( void *buf, unsigned int len, unsigned char *returnBuf, unsigned int *returnBufSize )
{
	TRACE_START();

#ifdef _WINDOWS
	return art_tlvSend2( buf, len, returnBuf, returnBufSize );
#else
	// Other OS not supported at this time
	return A_ERROR;
#endif
}

/********************************************************************** 
* A_INT32 Qc98xxDeviceNoiseFloorGet () 
*  
* Get the noise floor value from NVRAM.  This is Pnf(dBr). 
* It represents the sum of the noise floor as estimated by Pnf,cal(dBr) 
* plus the RX_noise_cal_offset(dB) which is an RSSI measurement from 
* the spectral scan HW.  In both cases, there is no signal input. 
* 
* params: int frequency, int ichain 
*  
* return in value: noise floor 
***********************************************************************/

A_INT32 Qc98xxDeviceNoiseFloorGet (int frequency, int ichain)
{
	A_INT32 nf;
    nf = Qc98xxRSSICalInfoNoiseFloorGet ( frequency, ichain );
	return nf;
}

/********************************************************************** 
* A_INT32 Qc98xxDeviceNoiseFloorPowerGet (int frequency, int ichain, int nf)
*  
* Get the noise floor power value in NVRAM.  This is Pnf(dBm). 
* RSSIsig,corr(dB) is computed with a signal of known input power in dBm. 
* RSSIsig,corr(dB) represents signal power over and above the noise floor 
* Pnf(dBr). RSSIsig,corr(dB) is subtraced from the input signal strength value 
* in order to obtain Pnf(dBm) which is the noise power in real world units. 
*  
* params: int frequency, int ichain, 
*  
* return in value: noise floor power 
* 
***********************************************************************/

A_INT32 Qc98xxDeviceNoiseFloorPowerGet ( int frequency, int ichain )
{
	A_INT32 nf_power;
    nf_power = Qc98xxRSSICalInfoNoiseFloorPowerGet ( frequency, ichain );
	return nf_power;
}

/********************************************************************** 
* 
* A_INT32 Qc98xxDeviceNoiseFloorTemperatureGet ( int frequency, int ichain ) 
* 
* Get the noise floor temperature from NVRAM. This is the temperature 
* at which the noise floor was calibrated. 
* 
***********************************************************************/
 
A_INT32 Qc98xxDeviceNoiseFloorTemperatureGet ( int frequency, int ichain )
{
	A_INT32 nf_temperature;
	nf_temperature = Qc98xxRSSICalInfoNoiseFloorTemperatureGet ( frequency, ichain );
	return nf_temperature;
}



/********************************************************************** 
*  A_INT32 Qc98xxDeviceNoiseFloorSet (  int frequency, int ichain, int nf  )
*  
* Set the noise floor value in NVRAM.  This is Pnf(dBr). 
* It represents the sum of the noise floor as estimated by Pnf,cal(dBr) 
* plus the RX_noise_cal_offset(dB) which is an RSSI measurement from 
* the spectral scan HW.  In both cases, there is no signal input. 
* 
* params : int frequency, int ichain, int nf 
* 
***********************************************************************/
A_INT32 Qc98xxDeviceNoiseFloorSet (  int frequency, int ichain, int nf )
{
    Qc98xxRSSICalInfoNoiseFloorSet ( frequency, ichain, nf );
	return 0;
}

/********************************************************************** 
* A_INT32 Qc98xxDeviceNoiseFloorPowerSet  (  int frequency, int ichain, int nf  ) 
*  
* Set the noise floor power value in NVRAM.  This is Pnf(dBm). 
* RSSIsig,corr(dB) is computed with a signal of known input power in dBm. 
* RSSIsig,corr(dB) represents signal power over and above the noise floor 
* Pnf(dBr). RSSIsig,corr(dB) is subtraced from the input signal strength value 
* in order to obtain Pnf(dBm) which is the noise power in real world units. 
*  
* params for value: int frequency, int ichain, int nf 
* 
***********************************************************************/

A_INT32 Qc98xxDeviceNoiseFloorPowerSet ( int frequency, int ichain, int nfPower )
{
    Qc98xxRSSICalInfoNoiseFloorPowerSet ( frequency, ichain, nfPower );
	return 0;
}

/********************************************************************** 
* A_INT32 Qc98xxDeviceNoiseFloorTemperatureSet ( int frequency, int ichain, int nf  ) 
*  
* Set the noise floor temperature value in NVRAM.
* params for value: int frequency, int ichain, int nf 
* 
***********************************************************************/

A_INT32 Qc98xxDeviceNoiseFloorTemperatureSet ( int frequency, int ichain, int nfTemperature )
{
    Qc98xxRSSICalInfoNoiseFloorTemperatureSet ( frequency, ichain, nfTemperature );
	return 0;
}

//=======================================================================

static struct _DeviceFunction _Qc98xxDevice =
{
    Qc9KChipIdentify,                       //ChipIdentify
	Qc98xxName,                             //Name
	Qc98xxVersion,                          //Version
	Qc98xxBuildDate,                        //BuildDate

	Qc98xxAttach,                           //Attach
	Qc98xxDetach,                             //Detach
	Qc98xxValid,                              //Valid
	Qc9KDeviceIdGet,                        //IdGet
	Qc98xxReset,                            //Reset
    Qc98xxSetCommand,                       //SetCommand
    Qc98xxSetParameterSplice,               //SetParameterSplice
    Qc98xxGetCommand,                       //GetCommand
    Qc98xxGetParameterSplice,               //GetParameterSplice
    Qc98xxBssIdSet,                         //BssIdSet
	Qc98xxStationIdSet,                     //StationIdSet
    0,//Qc98xxReceiveDescriptorPointer,     //ReceiveDescriptorPointer
    0,//Qc98xxReceiveUnicast,               //ReceiveUnicast
    0,//Qc98xxReceiveBroadcast,             //ReceiveBroadcast
    0,//Qc98xxReceivePromiscuous,           //ReceivePromiscuous
    0,//Qc98xxReceiveEnable,                //ReceiveEnable
    0,//Qc98xxReceiveDisable,               //ReceiveDisable
    0,//Qc98xxReceiveDeafMode,              //ReceiveDeafMode
    0,//Qc98xxReceiveFifo,                  //ReceiveFifo
    0,//Qc98xxReceiveDescriptorMaximum,     //ReceiveDescriptorMaximum     
    0,//Qc98xxReceiveEnableFirst,           //ReceiveEnableFirst     
    0,//Qc98xxTransmitFifo,                 //TransmitFifo     
    0,//Qc98xxTransmitDescriptorSplit,      //TransmitDescriptorSplit     
    0,//Qc98xxTransmitAggregateStatus,      //TransmitAggregateStatus     
    0,//Qc98xxTransmitEnableFirst,          //TransmitEnableFirst     
    0,//Qc98xxTransmitDescriptorStatusPointer,//TransmitDescriptorStatusPointer     
    0,//Qc98xxTransmitDescriptorPointer,    //TransmitDescriptorPointer     
    0,										// TransmitRetryLimit,
    0,//Qc98xxTransmitQueueSetup,           //TransmitQueueSetup
    0,//Qc98xxTransmitRegularData,          //TransmitRegularData
    0,//Qc98xxTransmitFrameData,            //TransmitFrameData
    0,//Qc98xxTransmitContinuousData,       //TransmitContinuousData 
    0,                                      //TransmitCarrier     
    0,//Qc98xxTransmitEnable,               //TransmitEnable     
    0,//Qc98xxTransmitDisable,              //TransmitDisable     
    0,//Qc98xxTransmitPowerSet,             //TransmitPowerSet     
    0,//Qc98xxTransmitGainSet,              //TransmitGainSet     
    0,//Qc98xxTransmitGainRead,             //TransmitGainRead     
    0,//Qc98xxTransmitGainWrite,            //TransmitGainWrite     
    0,										//EepromRead     
    0,										//EepromWrite     
    Qc98xxOtpRead,                          //OtpRead     
    Qc98xxOtpWrite,                         //OtpWrite     
	0,//MyMemoryBase,                       //MemoryBase     
	0,//MyMemoryPtr,                        //MemoryPtr
    Qc9KMemoryRead,                           //MemoryRead     
    Qc9KMemoryWrite,                          //MemoryWrite     
    Qc9KRegisterRead,                         //RegisterRead     
    Qc9KRegisterWrite,                        //RegisterWrite
    MyFieldRead,                            //FieldRead
    MyFieldWrite,                           //FieldWrite    
    0,                                      //ConfigurationRestore     
    Qc98xxEepromSave,                       //ConfigurationSave     
    0,                                      //CalibrationPierSet     
    0,                                      //PowerControlOverride     
    0,// Qc98xxTargetPowerSet,              //TargetPowerSet     
    Qc98xxTargetPowerGet,                   //TargetPowerGet
    0,                                      //TargetPowerApply     
    Qc98xxTemperatureGet,                   //TemperatureGet     
    Qc98xxVoltageGet,                       //VoltageGet     
    Qc98xxMacAddressGet,                    //MacAddressGet     
    Qc98xxCustomerDataGet,                  //CustomerDataGet     

	Qc98xxTxChainMany,                      //TxChainMany
	Qc98xxTxMaskGet,						//TxChainMask
	Qc98xxRxChainMany,                      //RxChainMany
	Qc98xxRxMaskGet,						//RxChainMask
    Qc98xxRxChainSet,                       //RxChainSet     
    Qc98xxEepromTemplatePreference,         //EepromTemplatePreference     
    Qc98xxEepromTemplateAllowed,            //EepromTemplateAllowed     
    Qc98xxEepromCompress,                   //EepromCompress     
    0,										//EepromOverwrite     
    Qc98xxEepromSize,                       //EepromSize     
    Qc98xxEepromSaveMemorySet,              //EepromSaveMemorySet     
    Qc98xxEepromReport,                     //EepromReport     
    Qc98xxCalibrationDataAddressSet,        //CalibrationDataAddressSet     
    Qc98xxCalibrationDataAddressGet,        //CalibrationDataAddressGet     
    Qc98xxCalibrationDataSet,               //CalibrationDataSet     
    Qc98xxCalibrationDataGet,               //CalibrationDataGet     
    Qc98xxCalibrationFromEepromFile,	    //CalibrationFromEepromFile,
    0,                                      //EepromTemplateInstall     
    
    Qc98xxPapdSet,                          //PaPredistortionSet     
    Qc98xxPapdGet,                          //PaPredistortionGet     
    Qc98xxRegulatoryDomainOverride,         //RegulatoryDomainOverride     
    Qc98xxRegulatoryDomainGet,              //RegulatoryDomainGet     
    Qc98xxDeviceNoiseFloorSet,              //NoiseFloorSet     
    Qc98xxDeviceNoiseFloorGet,              //NoiseFloorGet
    Qc98xxDeviceNoiseFloorPowerSet,         //NoiseFloorPowerSet      
    Qc98xxDeviceNoiseFloorPowerGet,         //NoiseFloorPowerGet       
	0,                                      //SpectralScanEnable     
	0                             ,         //SpectralScanProcess     
	0,                                      //SpectralScanDisable     
    Qc98xxDeviceNoiseFloorTemperatureSet,   //NoiseFloorTemperatureSet     
    Qc98xxDeviceNoiseFloorTemperatureGet,   //NoiseFloorTemperatureGet     
//#ifdef ATH_SUPPORT_MCI
	0,		// Qc98xxMCISetup,              //MCISetup       
	0,		// Qc98xxMCIReset,              //MCIReset     
//#endif
	Qc98xxTuningCapsSet,                    //TuningCapsSet     
	Qc98xxTuningCapsSave,                   //TuningCapsSave     
    Qc98xxChannelCalculate,                 //ChannelCalculate     

	Qc98xxConfigSpaceCommit,                //InitializationCommit     
	Qc98xxConfigSpaceUsed,                  //InitializationUsed        
	Qc98xxSubVendorSet,                     //InitializationSubVendorSet     
	Qc98xxSubVendorGet,                     //InitializationSubVendorGet     
	Qc98xxVendorSet,                        //InitializationVendorSet     
	Qc98xxVendorGet,                        //InitializationVendorGet     
	Qc98xxSSIDSet,                          //InitializationSsidSet     
	Qc98xxSSIDGet,                          //InitializationSsidVendorGet     
	Qc98xxDeviceIDSet,                      //InitializationDevidSet     
	Qc98xxDeviceIDGet,                      //InitializationDevidGet     
	Qc98xxPcieAddressValueDataSet,          //InitializationSet     
	Qc98xxPcieAddressValueDataGet,          //InitializationGet     
	Qc98xxPcieMany,                         //InitializationMany     
	Qc98xxPcieAddressValueDataOfNumGet,     //InitializationGetByIndex     
	Qc98xxPcieAddressValueDataRemove,       //InitializationRemove     
	0,                                      //InitializationRestore     

	Qc98xxNoiseFloorFetch,                  //NoiseFloorFetch     
	Qc98xxNoiseFloorLoad,                   //NoiseFloorLoad     
	Qc98xxNoiseFloorReady,                  //NoiseFloorReady     
	Qc98xxNoiseFloorEnable,                 //NoiseFloorEnable     

	Qc98xxOpFlagsGet,                       //OpflagsGet       
	Qc98xxIs2GHz,                           //Is2GHz     
	Qc98xxIs5GHz,                           //Is5GHz     
	Qc98xxIs4p9GHz,                         //Is4p9GHz     
	Qc98xxHalfRate,                         //IsHalfRate     
	Qc98xxQuarterRate,                      //IsQuarterRate     

    0,                                      //FlashRead     
    0,                                      //FlashWrite  
    Qc98xxIsEmbeddedArt,                    //IsEmbeddedArt
    Qc9KStickyWrite,                        //StickyWrite
    Qc9KStickyClear,                        //StickyClear
    Qc98xxConfigAddrSet,                    //ConfigAddrSet
    Qc98xxRfBbTestPoint,                    //RfBbTestPoint

    Qc98xxTransmitDataDut,                  //TransmitDataDut
    Qc98xxTransmitStatusReport,             //TransmitStatusReport
    Qc98xxTransmitStop,                     //TransmitStop
    Qc98xxReceiveDataDut,                   //ReceiveDataDut
    Qc98xxReceiveStatusReport,              //ReceiveStatusReport
    Qc98xxReceiveStop,                      //ReceiveStop

    Qc98xxCalInfoInit,                      //CalInfoInit
    Qc98xxCalInfoCalibrationPierSet,        //CalInfoCalibrationPierSet
    Qc98xxCalUnusedPierSet,                 //CalUnusedPierSet
    Qc98xxOtpLoad,                          //OtpLoad
    DevSetConfigParameterSplice,           //SetConfigParameterSplice
    Qc9KSetConfigCommand,                   //SetConfigCommand

    DevStbcGet,                            //StbcGet
    DevLdpcGet,                            //LdpcGet
    Qc98xxPapdGet,                          //PapdGet

    0,										//EepromSaveSectionSet
    Qc98xxPapdIsDone,                         //PapdIsDone

	0,										 //CalibrationPower
	0,										//CalibrationTxgainCAPSet
	0,										//IniVersion
	Qc98xxTxGainTableRead_AddressGainTable,	//TxGainTableRead_AddressGainTable
	Qc9KTxGainTableRead_AddressHeader,		//TxGainTableRead_AddressHeader
	Qc9KTxGainTableRead_AddressValue,		//TxGainTableRead_AddressValue
	0,										//Get_corr_coeff
	Qc98xxTransmitINIGainGet,				//TransmitINIGainGet
	Qc98xxSleepMode,						//SleepMode
	Qc98xxDeviceHandleGet,					//DeviceHandleGet
	0,										//XtalReferencePPMGet
	0,										//CmacPowerGet
	0,										//PsatCalibrationResultGet
	Qc98xxDiagData,							//DiagData
	0,                                      //GainTableOffset
	0,                                      //CalibrationSetting
	0,                                      //pll_screen
    Qc98xxReadPciConfigSpace,               //ReadPciConfigSpace
    Qc98xxWritePciConfigSpace,              //WritePciConfigSpace
    Qc98xxIs11ACDevice,                     //Is11ACDevice
	Qc98xxSetCommandLine,					//SetCommandLine
    DevNonCenterFreqAllowedGet,             //NonCenterFreqAllowedGet
	Qc98xxHeavyClipEnableSet,				//HeavyClipEnableSet
	Qc98xxHeavyClipEnableGet,				// HeavyClipStatus
	Qc98xxMacAddressSet,					//Qc98xxMacAddressSet
};

//=======================================================================
// for Qdart
//=======================================================================
int Qc98xxtlvCallbackSet(_tlvCallback ptlvCallbackFunc)
{
#ifdef QDART_BUILD
	return DevtlvCallbackSet(ptlvCallbackFunc);
#endif
	return 0;
}

int Qc98xxtlvCreate(unsigned char tlvOpcode)
{
#ifdef QDART_BUILD
	return DevtlvCreate(tlvOpcode);
#endif
	return 0;
}

int Qc98xxtlvAddParam(char *pKey, char *pData)
{
#ifdef QDART_BUILD
	return DevtlvAddParam(pKey, pData);
#endif
	return 0;
}

int Qc98xxtlvComplete(void)
{
#ifdef QDART_BUILD
	return DevtlvComplete();
#endif
	return 0;
}

int Qc98xxtlvGetRspParam(char *pKey, char *pData)
{
#ifdef QDART_BUILD
	return DevtlvGetRspParam(pKey, pData);
#endif
	return 0;
}

int Qc98xxtlvCalibration(double pwr)
{
#ifdef QDART_BUILD
	return DevtlvCalibration(pwr);
#endif
	return 0;
}

int Qc98xxtlvCalibrationInit(int mode)
{
#ifdef QDART_BUILD
	return DevtlvCalibrationInit(mode);
#endif
	return 0;
}

//=======================================================================
static struct _TlvDeviceFunction _Qc98xx_TlvDevice =
{
	Qc98xxtlvCallbackSet,					//tlvCallbackSet
	Qc98xxtlvCreate,						//tlvCreate
	Qc98xxtlvAddParam,						//tlvAddParam
	Qc98xxtlvComplete,						//tlvComplete
	Qc98xxtlvGetRspParam,					//tlvGetRspParam
	Qc98xxtlvCalibrationInit,				//tlvCalibrationInit
	Qc98xxtlvCalibration,					//tlvCalibration
};

static struct _DevDeviceFunction _Qc98xx_DevDevice =
{
    Qc98xxRegulatoryDomainGet,						//RegulatoryDomainGet
    Qc98xxRegulatoryDomain1Get,						//RegulatoryDomain1Get
    Qc98xxOpFlagsGet,								//OpFlagsGet
    Qc98xxOpFlags2Get,								//OpFlags2Get
    Qc98xxIs4p9GHz,									//Is4p9GHz
	Qc98xxHalfRate,									//HalfRate
	Qc98xxQuarterRate,								//QuarterRate
	Qc98xxCustomNameGet,							//CustomNameGet
#ifdef AP_BUILD
    Qc98xxSwapCalStruct,                             //SwapCalStruct
#else
	0,
#endif
};

#define LinkDllName "LinkQc9K"

//
// clear all device control function pointers and set to default behavior
//
//
// clear all device control function pointers and set to default behavior
//

QC98XXDLLSPEC int Qc98xxDeviceSelect()
{
	int error;

	DeviceFunctionReset();
	error=DeviceFunctionSelect(&_Qc98xxDevice);
	if(error!=0)
	{
		return error;
	}
	TlvDeviceFunctionReset();
	error=TlvDeviceFunctionSelect(&_Qc98xx_TlvDevice);
	if(error!=0)
	{
		return error;
	}
	DevDeviceFunctionReset();
	error = DevDeviceFunctionSelect(&_Qc98xx_DevDevice);
	if (error != 0)
	{
	    return error;
	}
	//
	// try to load the link layer dll
	//
#ifdef DYNAMIC_DEVICE_DLL
    error=LinkLoad(LinkDllName);
#else
	error=LinkLinkSelectExternal(Qc98xxLinkFunction);
#endif
	if(error!=0)
	{
		return error;
	}
	return error;
}

#ifdef QC98XXDLL
QC98XXDLLSPEC char *DevicePrefix(void)
{
	return "Qc98xx";
}
#endif

