#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>



#ifdef __APPLE__
#include "opt_ah.h"
#include "osdep.h"
#endif

#include "wlantype.h"
//#include "default9300.h"
#include "Sticky.h"

//
// hal header files
//
#include "ah.h"
#include "ah_internal.h"
#include "Field.h"
#include "ParameterSelect.h"
#include "Card.h"

#include "Ar9287EepromStructGet.h"
#include "Ar9287EepromStructSet.h"
#include "rate_constants.h"
#include "NartError.h"
#include "smatch.h"
#include "ConfigurationStatus.h"
#include "mEepStruct9287.h"
#include "ParameterConfigDef.h"
#include "NewArt.h"
#include "UserPrint.h"

extern struct ath_hal *AH;
//extern int32_t ar5416_calibration_data_get(struct ath_hal *ah);

#define MBUFFER 1024

#define TRUE  1
#define FALSE 0
#define FBIN2FREQ(x,y) ((y) ? (2300 + x) : (4800 + 5 * x))
static int setFBIN2FREQ(int bin, int iBand)
{
	int freq;
	if (bin==0)
		return 0;
	if (iBand==band_BG)
		freq = FBIN2FREQ(bin,1);
	else
		freq = FBIN2FREQ(bin,0);
	return freq;
}
A_INT16 Ar9287_LengthGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    A_INT32  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.baseEepHeader.length;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT16 Ar9287_ChecksumGet()
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    A_UINT16  value;
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        value = ahp->ah_eeprom.map.mapAr9287.baseEepHeader.checksum;
		return value; 
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT16 Ar9287_EepromVersionGet()
{
	A_INT16  value;
	value = Ar9287EepromStructGet()->baseEepHeader.version;
    return value; 
}
int Ar9287_OpFlagsGet()
{
	A_UINT8  value;
	value = (u_int8_t) Ar9287EepromStructGet()->baseEepHeader.opCapFlags;
    return value; 
}
int Ar9287_eepMiscGet()
{
	A_UINT8  value;
	value = (u_int8_t) Ar9287EepromStructGet()->baseEepHeader.eepMisc;
    return value; 
}
int Ar9287_RegDmnGet(int *value, int ix, int *num)
{
	int i, iv=0;
	if (ix<0 || ix>2) {
		value[0] = Ar9287EepromStructGet()->baseEepHeader.regDmn[0];
		value[1] = Ar9287EepromStructGet()->baseEepHeader.regDmn[1];
		*num = 2;
	} else {
		value[0] = Ar9287EepromStructGet()->baseEepHeader.regDmn[0];
		*num = 1;
	}
    return VALUE_OK; 
}
A_INT32 Ar9287_MacAdressGet(A_UINT8 *mac)
{
    A_INT16 i;

    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

    for(i=0; i<6; i++)
        mac[i] = peep9287->baseEepHeader.macAddr[i];
    return 0;
}
int Ar9287_TxRxMaskGet()
{
	A_UINT8  value;
	value = (u_int8_t)((Ar9287EepromStructGet()->baseEepHeader.txMask)<<4 | 
						(Ar9287EepromStructGet()->baseEepHeader.rxMask));
    return value; 
}
int Ar9287_RxMaskGet()
{
	A_UINT8  value;
	value = (u_int8_t) Ar9287EepromStructGet()->baseEepHeader.rxMask;
    return value; 
}
int Ar9287_TxMaskGet()
{
	A_UINT8  value;
	value = (u_int8_t) Ar9287EepromStructGet()->baseEepHeader.txMask;
    return value; 
}
int Ar9287_RFSilentGet()
{
	A_UINT8  value;
	value = (u_int8_t) Ar9287EepromStructGet()->baseEepHeader.rfSilent;
    return value; 
}
int Ar9287_BlueToothOptionsGet()
{
	A_UINT8  value;
	value = (u_int8_t) Ar9287EepromStructGet()->baseEepHeader.blueToothOptions;
    return value; 
}
int Ar9287_DeviceCapGet()
{
	A_UINT8  value;
	value = (u_int8_t) Ar9287EepromStructGet()->baseEepHeader.deviceCap;
    return value; 
}
int Ar9287_CalibrationBinaryVersionGet()
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    A_UINT32  value;
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        value = ahp->ah_eeprom.map.mapAr9287.baseEepHeader.binBuildNumber;
		return value; 
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_DeviceTypeGet()
{
	A_UINT8  value;
	value = (u_int8_t) Ar9287EepromStructGet()->baseEepHeader.deviceType;
    return value; 
}
int Ar9287_OpenLoopPwrCntlGet()
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    A_UINT8  value;
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        value = ahp->ah_eeprom.map.mapAr9287.baseEepHeader.openLoopPwrCntl;
		return value; 
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_PwrTableOffsetSetGet()
{
	int8_t  value;
	value = Ar9287EepromStructGet()->baseEepHeader.pwrTableOffset;
    return value; 
}
int Ar9287_TempSensSlopeGet()       
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    int8_t  value;
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        value = ahp->ah_eeprom.map.mapAr9287.baseEepHeader.tempSensSlope;
		return value; 
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_TempSensSlopePalOnGet()                       
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    int8_t  value;
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        value = ahp->ah_eeprom.map.mapAr9287.baseEepHeader.tempSensSlopePalOn;
		return value; 
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_FutureBaseGet(int *value, int ix, int *num)
{
	int i, j,iv=0;
	if (ix<0 || ix>29) {
		for(j=0;j<29;j++)
		{
			value[j] = Ar9287EepromStructGet()->baseEepHeader.futureBase[j];
		}
		*num = 29;
	} else {
		value[0] = Ar9287EepromStructGet()->baseEepHeader.futureBase[0];
		*num = 1;
	}
    return VALUE_OK; 
}
A_INT32 Ar9287_CustomerDataGet(A_UCHAR *data, A_INT32 len)
{
    A_INT16 i;

    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

    if(len>AR9287_DATA_SZ) {
        UserPrint("Error:: Can't get %d char in the customer data array\n", len);
        return -1;
    }

    for(i=0; i<len; i++)
        data[i] = peep9287->custData[i];

    return 0;
}
int Ar9287_TemplateVersionGet()
{
    return VALUE_OK;
}

int Ar9287_AntCtrlChainGet(int *value, int ix, int *num)
{
    struct ath_hal_5416 *ahp = AH5416(AH);    
	int i, iv=0;

    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
		if (ix<0 || ix>2) {
			value[0] = ahp->ah_eeprom.map.mapAr9287.modalHeader.antCtrlChain[0];
			value[1] = ahp->ah_eeprom.map.mapAr9287.modalHeader.antCtrlChain[1];
			*num = 2;
		} else {
			value[0] = ahp->ah_eeprom.map.mapAr9287.modalHeader.antCtrlChain[0];
			*num = 1;
		}
    }
    else
    {
        return ERR_RETURN;
    }
    
    return VALUE_OK; 
}
int Ar9287_AntCtrlCommonGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    A_UINT32  value;
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            //2G
            value = ahp->ah_eeprom.map.mapAr9287.modalHeader.antCtrlCommon;
			return value; 
        }
        else
        {
            //5G
            value = 0xbadbeef;
			return value; 
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK; 
}
int Ar9287_AntennaGainGet(int *value, int ix, int *num)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, iv=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
		if (ix<0 || ix>2) {
			value[0] = ahp->ah_eeprom.map.mapAr9287.modalHeader.antennaGainCh[0];
			value[1] = ahp->ah_eeprom.map.mapAr9287.modalHeader.antennaGainCh[1];
			*num = 2;
		} else {
			value[0] = ahp->ah_eeprom.map.mapAr9287.modalHeader.antennaGainCh[0];
			*num = 1;
		}        
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_SwitchSettlingGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.switchSettling;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT32 Ar9287_TxRxAttenChGet(int *value, int ix, int *num)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, iv=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
		if (ix<0 || ix>2) {
			value[0] = ahp->ah_eeprom.map.mapAr9287.modalHeader.txRxAttenCh[0];
			value[1] = ahp->ah_eeprom.map.mapAr9287.modalHeader.txRxAttenCh[1];
			*num = 2;
		} else {
			value[0] = ahp->ah_eeprom.map.mapAr9287.modalHeader.txRxAttenCh[0];
			*num = 1;
		}        
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT32 Ar9287_RxTxMarginChGet(int *value, int ix, int *num)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, iv=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
		if (ix<0 || ix>2) {
			value[0] = ahp->ah_eeprom.map.mapAr9287.modalHeader.rxTxMarginCh[0];
			value[1] = ahp->ah_eeprom.map.mapAr9287.modalHeader.rxTxMarginCh[1];
			*num = 2;
		} else {
			value[0] = ahp->ah_eeprom.map.mapAr9287.modalHeader.rxTxMarginCh[0];
			*num = 1;
		}        
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_AdcDesiredSizeGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.adcDesiredSize;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_TxEndToXpaOffGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.txEndToXpaOff;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_TxEndToRxOnGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.txEndToRxOn;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_TxFrameToXpaOnGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.txFrameToXpaOn;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_Thresh62Get(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.thresh62;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_NoiseFloorThreshChGet(int *value, int ix, int *num)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, iv=0;

    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
		if (ix<0 || ix>2) {
			value[0] = ahp->ah_eeprom.map.mapAr9287.modalHeader.noiseFloorThreshCh[0];
			value[1] = ahp->ah_eeprom.map.mapAr9287.modalHeader.noiseFloorThreshCh[1];
			*num = 2;
		} else {
			value[0] = ahp->ah_eeprom.map.mapAr9287.modalHeader.noiseFloorThreshCh[0];
			*num = 1;
		}        
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_XpdGainGet(int iBand)  
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.xpdGain;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}   
int Ar9287_XpdGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.xpd;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_IQCalIChGet(int *value, int ix, int *num)
{
	int i, iv=0;
	if (ix<0 || ix>2) {
		value[0] = Ar9287EepromStructGet()->modalHeader.iqCalICh[0];
		value[1] = Ar9287EepromStructGet()->modalHeader.iqCalICh[1];
		*num = 2;
	} else {
		value[0] = Ar9287EepromStructGet()->modalHeader.iqCalICh[0];
		*num = 1;
	}
    return VALUE_OK; 
}
int Ar9287_IQCalQChGet(int *value, int ix, int *num)
{
	int i, iv=0;
	if (ix<0 || ix>2) {
		value[0] = Ar9287EepromStructGet()->modalHeader.iqCalQCh[0];
		value[1] = Ar9287EepromStructGet()->modalHeader.iqCalQCh[1];
		*num = 2;
	} else {
		value[0] = Ar9287EepromStructGet()->modalHeader.iqCalQCh[0];
		*num = 1;
	}
    return VALUE_OK; 
}
A_INT32 Ar9287_PdGainOverlapGet(int iBand)    
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.pdGainOverlap;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_XpaBiasLvlGet(int iBand)
{    
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.xpaBiasLvl;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_TxFrameToDataStartGet(int iBand)
{    
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.txFrameToDataStart;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_TxFrameToPaOnGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.txFrameToPaOn;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT32 Ar9287_HT40PowerIncForPdadcGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.ht40PowerIncForPdadc;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT32 Ar9287_BswAttenGet(int *value, int ix, int *num)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, iv=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
		if (ix<0 || ix>2) {
			value[0] = ahp->ah_eeprom.map.mapAr9287.modalHeader.bswAtten[0];
			value[1] = ahp->ah_eeprom.map.mapAr9287.modalHeader.bswAtten[1];
			*num = 2;
		} else {
			value[0] = ahp->ah_eeprom.map.mapAr9287.modalHeader.bswAtten[0];
			*num = 1;
		}		 
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT32 Ar9287_BswMarginGet(int *value, int ix, int *num)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, iv=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
		if (ix<0 || ix>2) {
			value[0] = ahp->ah_eeprom.map.mapAr9287.modalHeader.bswMargin[0];
			value[1] = ahp->ah_eeprom.map.mapAr9287.modalHeader.bswMargin[1];
			*num = 2;
		} else {
			value[0] = ahp->ah_eeprom.map.mapAr9287.modalHeader.bswMargin[0];
			*num = 1;
		}		 
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT32 Ar9287_SwSettleHT40Get(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.swSettleHt40;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT32 Ar9287_ModalHeaderVersionGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.version;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT32 Ar9287_db1Get(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.db1;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT32 Ar9287_db2Get(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.db2;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT32 Ar9287_ob_cckGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.ob_cck;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT32 Ar9287_ob_pskGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.ob_psk;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT32 Ar9287_ob_qamGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.ob_qam;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT32 Ar9287_ob_pal_offGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.modalHeader.ob_pal_off;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}          
int Ar9287_FutureModalGet(int *value, int ix, int *num)
{
	int i, j,iv=0;
	if (ix<0 || ix>30) {
		for(j=0;j<30;j++)
		{
			value[j] = Ar9287EepromStructGet()->modalHeader.futureModal[j];
		}
		*num = 30;
	} else {
		value[0] = Ar9287EepromStructGet()->modalHeader.futureModal[0];
		*num = 1;
	}
    return VALUE_OK; 
}
int Ar9287_PaddingGet(int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    u_int8_t  value=0;
    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            value= ahp->ah_eeprom.map.mapAr9287.padding;
			return value;
        }
        else
        {
            return ERR_VALUE_BAD;
        }
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}

A_INT32 Ar9287_SpurChansGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	struct ath_hal_5416 *ahp = AH5416(AH);
	int i;
	int freq;
		
	for (i=0; i<AR9287_EEPROM_MODAL_SPURS; i++) 
	{
		if (iBand==band_BG) 
		{
		    value[i] = ahp->ah_eeprom.map.mapAr9287.modalHeader.spurChans[i].spurChan;
		} else {
            return ERR_VALUE_BAD;
		}
		value[i] = setFBIN2FREQ(value[i], iBand);
	}
	*num=AR9287_EEPROM_MODAL_SPURS;
    return VALUE_OK; 
}
A_INT32 Ar9287_SpurRangeLowGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	struct ath_hal_5416 *ahp = AH5416(AH);
	int i;
	int freq;
	
	for (i=0; i<AR9287_EEPROM_MODAL_SPURS; i++) 
	{
		if (iBand==band_BG) 
		{
		    value[i] = ahp->ah_eeprom.map.mapAr9287.modalHeader.spurChans[i].spurRangeLow;
		} else {
            return ERR_VALUE_BAD;
		}
	}	
	*num=AR9287_EEPROM_MODAL_SPURS;
    return VALUE_OK;    
}
A_INT32 Ar9287_SpurRangeHighGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	struct ath_hal_5416 *ahp = AH5416(AH);
	int i;
	int freq;
	
	for (i=0; i<AR9287_EEPROM_MODAL_SPURS; i++) 
	{
		if (iBand==band_BG) 
		{
		    value[i] = ahp->ah_eeprom.map.mapAr9287.modalHeader.spurChans[i].spurRangeHigh;
		} else {
            return ERR_VALUE_BAD;
		}
	}	
	*num=AR9287_EEPROM_MODAL_SPURS;
    return VALUE_OK;    
}
A_INT32 Ar9287_CalPierFreqGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, iv=0, iMaxPier=NUM_2G_CAL_PIERS;
	int val;

	if (ix<0 || ix>iMaxPier) {
		for (i=0; i<iMaxPier; i++) {
			if (iBand==band_BG) {
				val = ahp->ah_eeprom.map.mapAr9287.calFreqPier2G[i];
			} else {
				return ERR_VALUE_BAD;
			}
			value[i] = setFBIN2FREQ(val, iBand);
		}
		*num = OSPREY_NUM_2G_CAL_PIERS;
	} else {
		if (iBand==band_BG) {
			val = ahp->ah_eeprom.map.mapAr9287.calFreqPier2G[ix];
		} else {
            return ERR_VALUE_BAD;
		}
		value[0] = setFBIN2FREQ(val, iBand);
		*num = 1;
	}
	return VALUE_OK;
}
A_INT32 Ar9287_CalTGTpwrcckGet(double *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, j, iv=0, iMaxPier, jMaxRate=4;
	iMaxPier=AR9287_NUM_2G_CCK_TARGET_POWERS;
	if (iy<0 || iy>=jMaxRate) {
		if (ix<0 || ix>=iMaxPier) {
			// get all i, all j
			for (i=0; i<iMaxPier; i++) {
				for (j=0; j<jMaxRate; j++) {
					value[iv++] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPowerCck[i].tPow2x[j])/2.0;
				}
			}
			*num = jMaxRate*iMaxPier;
		} else { // get all j for ix chain
			for (j=0; j<jMaxRate; j++) {
				value[iv++] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPowerCck[ix].tPow2x[j])/2.0;
			}
			*num = jMaxRate;
		}
	} else {
		if (ix<0 || ix>=iMaxPier) {
			for (i=0; i<iMaxPier; i++) {
				value[iv++] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPowerCck[i].tPow2x[iy])/2.0;
				*num = iMaxPier;
			}
		} else {
			value[0] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPowerCck[ix].tPow2x[iy])/2.0;
			*num = 1;
		}
	}	
	return VALUE_OK;     
}
A_INT32 Ar9287_CalTGTpwrGet(double *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, j, iv=0, iMaxPier, jMaxRate=4;
	iMaxPier=AR9287_NUM_2G_20_TARGET_POWERS;
	if (iy<0 || iy>=jMaxRate) {
		if (ix<0 || ix>=iMaxPier) {
			// get all i, all j
			for (i=0; i<iMaxPier; i++) {
				for (j=0; j<jMaxRate; j++) {
					value[iv++] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPower2G[i].tPow2x[j])/2.0;
				}
			}
			*num = jMaxRate*iMaxPier;
		} else { // get all j for ix chain
			for (j=0; j<jMaxRate; j++) {
				value[iv++] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPower2G[ix].tPow2x[j])/2.0;
			}
			*num = jMaxRate;
		}
	} else {
		if (ix<0 || ix>=iMaxPier) {
			for (i=0; i<iMaxPier; i++) {
				value[iv++] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPower2G[i].tPow2x[iy])/2.0;
				*num = iMaxPier;
			}
		} else {
			value[0] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPower2G[ix].tPow2x[iy])/2.0;
			*num = 1;
		}
	}	
	return VALUE_OK;     
}
A_INT32 Ar9287_CalTGTpwrht20Get(double *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, j, iv=0, iMaxPier, jMaxRate=8;
	iMaxPier=AR9287_NUM_2G_20_TARGET_POWERS;
	if (iy<0 || iy>=jMaxRate) {
		if (ix<0 || ix>=iMaxPier) {
			// get all i, all j
			for (i=0; i<iMaxPier; i++) {
				for (j=0; j<jMaxRate; j++) {
					value[iv++] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT20[i].tPow2x[j])/2.0;
				}
			}
			*num = jMaxRate*iMaxPier;
		} else { // get all j for ix chain
			for (j=0; j<jMaxRate; j++) {
				value[iv++] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT20[ix].tPow2x[j])/2.0;
			}
			*num = jMaxRate;
		}
	} else {
		if (ix<0 || ix>=iMaxPier) {
			for (i=0; i<iMaxPier; i++) {
				value[iv++] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT20[i].tPow2x[iy])/2.0;
				*num = iMaxPier;
			}
		} else {
			value[0] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT20[ix].tPow2x[iy])/2.0;
			*num = 1;
		}
	}	
	return VALUE_OK;     
}
A_INT32 Ar9287_CalTGTpwrht40Get(double *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, j, iv=0, iMaxPier, jMaxRate=8;
	iMaxPier=AR9287_NUM_2G_40_TARGET_POWERS;
	if (iy<0 || iy>=jMaxRate) {
		if (ix<0 || ix>=iMaxPier) {
			// get all i, all j
			for (i=0; i<iMaxPier; i++) {
				for (j=0; j<jMaxRate; j++) {
					value[iv++] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT40[i].tPow2x[j])/2.0;
				}
			}
			*num = jMaxRate*iMaxPier;
		} else { // get all j for ix chain
			for (j=0; j<jMaxRate; j++) {
				value[iv++] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT40[ix].tPow2x[j])/2.0;
			}
			*num = jMaxRate;
		}
	} else {
		if (ix<0 || ix>=iMaxPier) {
			for (i=0; i<iMaxPier; i++) {
				value[iv++] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT40[i].tPow2x[iy])/2.0;
				*num = iMaxPier;
			}
		} else {
			value[0] = ((double)ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT40[ix].tPow2x[iy])/2.0;
			*num = 1;
		}
	}
	return VALUE_OK;
}
A_INT32 Ar9287_CalTGTpwrcckChannelGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, iv=0, iMaxPier;
	int val;
	iMaxPier=AR9287_NUM_2G_CCK_TARGET_POWERS;
	if (ix<0 || ix>iMaxPier) {
		for (i=0; i<iMaxPier; i++) {
			value[i]=0;
			val = ahp->ah_eeprom.map.mapAr9287.calTargetPowerCck[i].bChannel;
			if(val==0 || val==255)
				break;
			value[i] = setFBIN2FREQ(val, iBand);
		}
		*num = i;
	} else {
		val = ahp->ah_eeprom.map.mapAr9287.calTargetPowerCck[ix].bChannel;
		value[0] = setFBIN2FREQ(val, iBand);
		*num = 1;
	}
	return VALUE_OK;
}
A_INT32 Ar9287_CalTGTpwrChannelGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, iv=0, iMaxPier;
	int val;
	iMaxPier=AR9287_NUM_2G_20_TARGET_POWERS;
	if (ix<0 || ix>iMaxPier) {
		for (i=0; i<iMaxPier; i++) {
			val = ahp->ah_eeprom.map.mapAr9287.calTargetPower2G[i].bChannel;
			value[i] = setFBIN2FREQ(val, iBand);
		}
		*num = iMaxPier;
	} else {
		val = ahp->ah_eeprom.map.mapAr9287.calTargetPower2G[ix].bChannel;
		value[0] = setFBIN2FREQ(val, iBand);
		*num = 1;
	}
	return VALUE_OK;
}
A_INT32 Ar9287_CalTGTpwrht20ChannelGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, iv=0, iMaxPier;
	int val;
	iMaxPier=AR9287_NUM_2G_20_TARGET_POWERS;
	if (ix<0 || ix>iMaxPier) {
		for (i=0; i<iMaxPier; i++) {
			val = ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT20[i].bChannel;
			value[i] = setFBIN2FREQ(val, iBand);
		}
		*num = iMaxPier;
	} else {
		val = ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT20[ix].bChannel;
		value[0] = setFBIN2FREQ(val, iBand);
		*num = 1;
	}
	return VALUE_OK;
}
A_INT32 Ar9287_CalTGTpwrht40ChannelGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, iv=0, iMaxPier;
	int val;
	iMaxPier=AR9287_NUM_2G_40_TARGET_POWERS;
	if (ix<0 || ix>iMaxPier) {
		for (i=0; i<iMaxPier; i++) {
			val = ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT40[i].bChannel;
			value[i] = setFBIN2FREQ(val, iBand);
		}
		*num = iMaxPier;
	} else {
		val = ahp->ah_eeprom.map.mapAr9287.calTargetPower2GHT40[ix].bChannel;
		value[0] = setFBIN2FREQ(val, iBand);
		*num = 1;
	}
	return VALUE_OK;
}
A_INT32 Ar9287_CtlIndexGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, iv=0, iMaxCtl=AR9287_NUM_CTLS;
	int val;

	if (ix<0 || ix>iMaxCtl) {
		for (i=0; i<iMaxCtl; i++) {
			if (iBand==band_BG) {
				val = ahp->ah_eeprom.map.mapAr9287.ctlIndex[i];
			} else {
				 return ERR_VALUE_BAD;
			}
			value[i] = val;
		}
		*num = iMaxCtl;
	} else {
		if (iBand==band_BG) {
			val = ahp->ah_eeprom.map.mapAr9287.ctlIndex[ix];
		} else {
			 return ERR_VALUE_BAD;
		}
		value[0] = val;
		*num = 1;
	}	
    return VALUE_OK; 
}
A_INT32 Ar9287_CtlChannelGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, j, k, iv=0, iMaxCtl=AR9287_NUM_CTLS, jMaxChain=AR9287_MAX_CHAINS, kMaxEdge=AR9287_NUM_BAND_EDGES;
	int val=0;

	if (iy<0 || iy>=kMaxEdge) {
		if (ix<0 || ix>=iMaxCtl) {
			// get all i, all j
			for (i=0; i<iMaxCtl; i++) {
				for (k=0; k<kMaxEdge; k++) {
					if (iBand==band_BG) {
						val = ahp->ah_eeprom.map.mapAr9287.ctlData[i].ctlEdges[0][k].bChannel;
					} else {
						return ERR_VALUE_BAD;
					}
					value[iv++]= setFBIN2FREQ(val, iBand);
				}
			}
			*num = kMaxEdge*iMaxCtl;
		} else { // get all j for ix chain
			for (k=0; k<kMaxEdge; k++) {
				if (iBand==band_BG) {
					val = ahp->ah_eeprom.map.mapAr9287.ctlData[ix].ctlEdges[0][k].bChannel;
				} else {
					return ERR_VALUE_BAD;
				}
				value[iv++]= setFBIN2FREQ(val, iBand);
			}
			*num = kMaxEdge;
		}
	} else {
		if (ix<0 || ix>=iMaxCtl) {
			for (i=0; i<iMaxCtl; i++) {
				if (iBand==band_BG) {
					val = ahp->ah_eeprom.map.mapAr9287.ctlData[i].ctlEdges[0][iy].bChannel;
				} else {
					return ERR_VALUE_BAD;
				}
				value[iv++]= setFBIN2FREQ(val, iBand);
				*num = iMaxCtl;
			}
		} else {
			if (iBand==band_BG) {
				val = ahp->ah_eeprom.map.mapAr9287.ctlData[ix].ctlEdges[0][iy].bChannel;
			} else {
				return ERR_VALUE_BAD;
			}
			value[iv++]= setFBIN2FREQ(val, iBand);
			*num = 1;
		}
	}
    return VALUE_OK; 
}
A_INT32 Ar9287_CtlPowerGet(double *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, j, k, iv=0, iMaxCtl=AR9287_NUM_CTLS, jMaxChain=AR9287_MAX_CHAINS, kMaxEdge=AR9287_NUM_BAND_EDGES;
	double val=0;

	if (iy<0 || iy>=kMaxEdge) {
		if (ix<0 || ix>=iMaxCtl) {
			// get all i, all j
			for (i=0; i<iMaxCtl; i++) {
				for (k=0; k<kMaxEdge; k++) {
					if (iBand==band_BG) {
						val = ((double)ahp->ah_eeprom.map.mapAr9287.ctlData[i].ctlEdges[0][k].tPower)/2.0;
					} else {
						return ERR_VALUE_BAD;
					}
					value[iv++]=val;
				}
			}
			*num = kMaxEdge*iMaxCtl;
		} else { // get all j for ix chain
			for (k=0; k<kMaxEdge; k++) {
				if (iBand==band_BG) {
					val = ((double)ahp->ah_eeprom.map.mapAr9287.ctlData[ix].ctlEdges[0][k].tPower)/2.0;
				} else {
					return ERR_VALUE_BAD;
				}
				value[iv++]=val;
			}
			*num = kMaxEdge;
		}
	} else {
		if (ix<0 || ix>=iMaxCtl) {
			for (i=0; i<iMaxCtl; i++) {
				if (iBand==band_BG) {
					val = ((double)ahp->ah_eeprom.map.mapAr9287.ctlData[i].ctlEdges[0][iy].tPower)/2.0;
				} else {
					return ERR_VALUE_BAD;
				}
				value[iv++]=val;
				*num = iMaxCtl;
			}
		} else {
			if (iBand==band_BG) {
				val = ((double)ahp->ah_eeprom.map.mapAr9287.ctlData[ix].ctlEdges[0][iy].tPower)/2.0;
			} else {
				return ERR_VALUE_BAD;
			}
			value[0]=val;
			*num = 1;
		}
	}
    return VALUE_OK; 
}
A_INT32 Ar9287_CtlFlagGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, j, k, iv=0, iMaxCtl=AR9287_NUM_CTLS, jMaxChain=AR9287_MAX_CHAINS, kMaxEdge=AR9287_NUM_BAND_EDGES;
	int val=0;

	if (iy<0 || iy>=kMaxEdge) {
		if (ix<0 || ix>=iMaxCtl) {
			// get all i, all j
			for (i=0; i<iMaxCtl; i++) {
				for (j=0; j<jMaxChain; j++) {
					for (k=0; k<kMaxEdge; k++) {
						if (iBand==band_BG) {
							val = ahp->ah_eeprom.map.mapAr9287.ctlData[i].ctlEdges[j][k].flag;
						} else {
							return ERR_VALUE_BAD;
						}
						value[iv++]=val;
					}
				}
			}
			*num = kMaxEdge*iMaxCtl;
		} else { // get all j for ix chain
			for (j=0; j<jMaxChain; j++) {
				for (k=0; k<kMaxEdge; k++) {
					if (iBand==band_BG) {
						val = ahp->ah_eeprom.map.mapAr9287.ctlData[ix].ctlEdges[j][k].flag;
					} else {
						return ERR_VALUE_BAD;
					}
					value[iv++]=val;
				}
				*num = kMaxEdge;
			}
		}
	} else {
		if (ix<0 || ix>=iMaxCtl) {
			for (i=0; i<iMaxCtl; i++) {
				for (j=0; j<jMaxChain; j++) {
					if (iBand==band_BG) {
						val = ahp->ah_eeprom.map.mapAr9287.ctlData[i].ctlEdges[j][iy].flag;
					} else {
						return ERR_VALUE_BAD;
					}
					value[iv++]=val;
					*num = iMaxCtl;
				}
			}
		} else {
			if (iBand==band_BG) {
				for (j=0; j<jMaxChain; j++) {
					val = ahp->ah_eeprom.map.mapAr9287.ctlData[ix].ctlEdges[j][iy].flag;
				}
			} else {
				return ERR_VALUE_BAD;
			}
			value[0]=val;
			*num = 1;
		}
	}
    return VALUE_OK; 
}

/**
 * - Function Name: ar9287_eepromDump
 * - Description  : provide a formatted output of the eeprom contents
 * - Arguments
 *     -
 * - Returns      :
 *******************************************************************************/
void ar9287_eepromDump(int client, ar9287_eeprom_t *pEepStruct) 
{
    A_UINT16          i, j, k, c, r;
    BASE_EEPAR9287_HEADER   *pBase = &(pEepStruct->baseEepHeader);
    MODAL_EEPAR9287_HEADER  *pModal;
    CAL_TARGET_POWER_HT  *pPowerInfo = NULL;
    CAL_TARGET_POWER_LEG  *pPowerInfoLeg = NULL;
    A_BOOL  legacyPowers = 0;
    CAL_DATA_PER_FREQ_U_AR9287*  pDataPerChannel=NULL;
    A_BOOL    is2Ghz = 0;
    A_UINT8   noMoreSpurs;
    A_UINT8*  pChannel=NULL;
    A_UINT16  channelCount, channelRowCnt, vpdCount, rowEndsAtChan;
    A_UINT16  xpdGainMapping[] = {0, 1, 2, 4};
    A_UINT16  xpdGainValues[AR9287_NUM_PD_GAINS], numXpdGain = 0, xpdMask;
    A_UINT16  numPowers = 0, numRates = 0, ratePrint = 0, numChannels=0, tempFreq;
    A_UINT16  numTxChains = (A_UINT16)( ((pBase->txMask >> 2) & 1) +
                                                ((pBase->txMask >> 1) & 1) + (pBase->txMask & 1) );
    A_UINT16  temp;
    A_UINT8   targetPower;
    //A_INT8    tempPower;
    A_UINT16  channelValue;
    A_UINT32  Freqs[6];

    A_UINT8 buffer[MBUFFER],buffer2[MBUFFER];

    const static char *sRatePrint[3][8] = {
        {"     6-24     ", "      36      ", "      48      ", "      54      ", "", "", "", ""},
        {"       1      ", "       2      ", "     5.5      ", "      11      ", "", "", "", ""},
        {"   HT MCS0    ", "   HT MCS1    ", "   HT MCS2    ", "   HT MCS3    ",
            "   HT MCS4    ", "   HT MCS5    ", "   HT MCS6    ", "   HT MCS7    "},
    };

    const static char *sTargetPowerMode[7] = {
        "5G OFDM ", "5G HT20 ", "5G HT40 ", "2G CCK  ", "2G OFDM ", "2G HT20 ", "2G HT40 ",
    };

    const static char *sDeviceType[] = {
        "UNKNOWN [0] ",
        "Cardbus     ",
        "PCI         ",
        "MiniPCI     ",
        "Access Point",
        "PCIExpress  ",
        "UNKNOWN [6] ",
        "UNKNOWN [7] ",
		"USB         ",
    };

    const static char *sCtlType[] = {
        "[ 11A base mode ]",
        "[ 11B base mode ]",
        "[ 11G base mode ]",
        "[ INVALID       ]",
        "[ INVALID       ]",
        "[ 2G HT20 mode  ]",
        "[ 5G HT20 mode  ]",
        "[ 2G HT40 mode  ]",
        "[ 5G HT40 mode  ]",
    };

    //assert(pEepStruct);


    /* Print Header info */
    pBase = &(pEepStruct->baseEepHeader);
    //targetEndian = 0;//art_getEndianMode(devNum);
    //clientEndian = targetEndian>>1;
    //targetEndian &= 1;

// Perform Structure specific swaps
    //pBase->length = client2host16(pBase->length);
    //pBase->checksum = client2host16(pBase->checksum);
    //pBase->version = client2host16(pBase->version);
    //pBase->regDmn[0] = client2host16(pBase->regDmn[0]);
    //pBase->regDmn[1] = client2host16(pBase->regDmn[1]);
    //pBase->binBuildNumber = client2host32(pBase->binBuildNumber);

//printf("SNOOP: target endian = %d, %d\n", targetEndian, (pBase->opCapFlags.eepMisc & AR9287_EEPMISC_BIG_ENDIAN) || 0);
    //SformatOutput(buffer, MBUFFER-1,"");
    //SendIt(client,buffer);
    SformatOutput(buffer, MBUFFER-1," =======================Header Information======================");
    //SendIt(client,buffer);
    SformatOutput(buffer, MBUFFER-1," |  Major Version           %2d  |  Minor Version           %2d  |",
             (pBase->version>>12)&0xF, (pBase->version & AR9287_EEP_VER_MINOR_MASK));
    //SendIt(client,buffer);
    SformatOutput(buffer, MBUFFER-1," |-------------------------------------------------------------|");
    //SendIt(client,buffer);
    SformatOutput(buffer, MBUFFER-1," |  Checksum           0x%04X   |  Length             0x%04X   |",
             pBase->checksum, pBase->length);
    //SendIt(client,buffer);
    SformatOutput(buffer, MBUFFER-1," |  RegDomain 1        0x%04X   |  RegDomain 2        0x%04X   |",
             pBase->regDmn[0], pBase->regDmn[1]);
    //SendIt(client,buffer);
    SformatOutput(buffer, MBUFFER-1," |  MacAddress: 0x%02X:%02X:%02X:%02X:%02X:%02X                            |",
             pBase->macAddr[0], pBase->macAddr[1], pBase->macAddr[2],
             pBase->macAddr[3], pBase->macAddr[4], pBase->macAddr[5]);
    //SendIt(client,buffer);
    SformatOutput(buffer, MBUFFER-1," |  TX Mask            0x%04X   |  RX Mask            0x%04X   |",
             pBase->txMask, pBase->rxMask);
    //SendIt(client,buffer);
    SformatOutput(buffer, MBUFFER-1," |  OpFlags: 5GHz %d, 2GHz %d, Disable 5HT40 %d, Disable 2HT40 %d  |",
             (pBase->opCapFlags & AR9287_OPFLAGS_11A) || 0, (pBase->opCapFlags & AR9287_OPFLAGS_11G) || 0,
             (pBase->opCapFlags & AR9287_OPFLAGS_5G_HT40) || 0, (pBase->opCapFlags & AR9287_OPFLAGS_2G_HT40) || 0);
    //SendIt(client,buffer);
    SformatOutput(buffer, MBUFFER-1," |  OpFlags: Disable 5HT20 %d, Disable 2HT20 %d                  |",
             (pBase->opCapFlags & AR9287_OPFLAGS_5G_HT20) || 0, (pBase->opCapFlags & AR9287_OPFLAGS_2G_HT20) || 0);
    //SendIt(client,buffer);
    SformatOutput(buffer, MBUFFER-1," |  Misc: Big Endian %d, Wake On Wireless %d                     |",
             (pBase->eepMisc & AR9287_EEPMISC_BIG_ENDIAN) || 0, (pBase->eepMisc & AR9287_EEPMISC_WOW) || 0);
    //SendIt(client,buffer);
    SformatOutput(buffer, MBUFFER-1," |  Cal Bin Maj Ver %3d Cal Bin Min Ver %3d Cal Bin Build %3d  |",
             (pBase->binBuildNumber >> 24) & 0xFF,
             (pBase->binBuildNumber >> 16) & 0xFF,
             (pBase->binBuildNumber >> 8) & 0xFF);
    //SendIt(client,buffer);
    if( (pBase->version & AR9287_EEP_VER_MINOR_MASK) >= AR9287_EEP_MINOR_VER_1 ) 
    {
        SformatOutput(buffer, MBUFFER-1," |  Device Type: %s                                  |",
                 sDeviceType[(pBase->deviceType & 0xf)]);
		//SendIt(client,buffer);
    }
    SformatOutput(buffer, MBUFFER-1," |  openLoopPwrCntl :  %d                                       |",
             pBase->openLoopPwrCntl);
    //SendIt(client,buffer);
    SformatOutput(buffer, MBUFFER-1," |  pwrTableOffset  : %d                                       |",
             pBase->pwrTableOffset);
    //SendIt(client,buffer);
    SformatOutput(buffer, MBUFFER-1," |  tempSensSlope   : %d                                       |",
             pBase->tempSensSlope);
    //SendIt(client,buffer);
    SformatOutput(buffer, MBUFFER-1," |  tempSensSlopePalOn : %d                                    |",
             pBase->tempSensSlopePalOn);
    //SendIt(client,buffer);
#if 0
    //if( isKiwi1_1(macRev) ) 
    //{
        SformatOutput(buffer, MBUFFER-1," |  palPwrThresh : %d                                    |",
                 pBase->palPwrThresh);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  pal_2ch_HT40_QAM : %d                                    |",
                 pBase->palEnableModes & 0x01);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  pal_2ch_HT20_QAM : %d                                    |",
                 pBase->palEnableModes & 0x02);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  pal_1ch_HT40_QAM : %d                                    |",
                 pBase->palEnableModes & 0x04);
        //SendIt(client,buffer);    
    //}
#endif
    SformatOutput(buffer, MBUFFER-1," |  Customer Data in hex                                       |");
    //SendIt(client,buffer);

	memset(buffer,0x0, sizeof(buffer));
    for( i = 0; i < AR9287_DATA_SZ; i++ ) 
    {
        if( (i % 16) == 0 ) 
        {
            SformatOutput(buffer2, MBUFFER-1," |  = ");
            strcat(buffer,buffer2);
        }
        SformatOutput(buffer2, MBUFFER-1,"%02X ", pEepStruct->custData[i]);
        strcat(buffer,buffer2);
        if( (i % 16) == 15 ) 
        {
            SformatOutput(buffer2, MBUFFER-1,"        =|");
            strcat(buffer,buffer2);
            //SendIt(client,buffer);
			memset(buffer,0x0, sizeof(buffer));
        }
    } 

    /* Print Modal Header info */
    //for( i = 0; i < 1; i++ ) 
    //{
        pModal = &(pEepStruct->modalHeader);
        //pModal->antCtrlChain[0] = target2host32(pModal->antCtrlChain[0]);
        //pModal->antCtrlChain[1] = target2host32(pModal->antCtrlChain[1]);
        //pModal->antCtrlChain[2] = target2host32(pModal->antCtrlChain[2]);
        //pModal->antCtrlCommon   = target2host32(pModal->antCtrlCommon);

        SformatOutput(buffer, MBUFFER-1," |======================2GHz Modal Header======================|");
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  Ant Chain 0  0x%08lX     |  Ant Chain 1  0x%08lX     |",
                 pModal->antCtrlChain[0], pModal->antCtrlChain[1]);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  Antenna Common  0x%08lX  |  Antenna Gain Chain 0   %3d  |",
                 pModal->antCtrlCommon, pModal->antennaGainCh[0]);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  Antenna Gain Chain 1   %3d  |  Switch Settling        %3d  |",
                 pModal->antennaGainCh[1], pModal->switchSettling);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  TxRxAttenuation Ch 0   %3d  |  TxRxAttenuation Ch 1   %3d  |",
                 pModal->txRxAttenCh[0], pModal->txRxAttenCh[1]);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  RxTxMargin Chain 0     %3d  |  RxTxMargin Chain 1     %3d  |",
                 pModal->rxTxMarginCh[0], pModal->rxTxMarginCh[1]);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  adc desired size       %3d  |  tx end to rx on        %3d  |",
                 pModal->adcDesiredSize, pModal->txEndToRxOn);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  tx end to xpa off      %3d  |  ht40PowerIncForPdadc   %3d  |",
                 pModal->txEndToXpaOff, pModal->ht40PowerIncForPdadc);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  tx frame to xpa on     %3d  |  thresh62               %3d  |",
                 pModal->txFrameToXpaOn, pModal->thresh62);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  Xpd Gain Mask 0x%X           |  Xpd                     %2d  |",
                 pModal->xpdGain, pModal->xpd);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  noise floor thres 0    %3d  |  noise floor thres 1    %3d  |",
                 pModal->noiseFloorThreshCh[0], pModal->noiseFloorThreshCh[1]);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  IQ Cal I Chain 0       %3d  |  IQ Cal Q Chain 0       %3d  |",
                 pModal->iqCalICh[0], pModal->iqCalQCh[0]);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  IQ Cal I Chain 1       %3d  |  IQ Cal Q Chain 1       %3d  |",
                 pModal->iqCalICh[1], pModal->iqCalQCh[1]);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  pdGain Overlap     %2d.%d dB  |  xpa bias level         %3d  |",
                 pModal->pdGainOverlap / 2, (pModal->pdGainOverlap % 2) * 5, pModal->xpaBiasLvl);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  txFrameToDataStart     %3d  |  txFrameToPaOn          %3d  |",
                 pModal->txFrameToDataStart, pModal->txFrameToPaOn);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  bswAtten Chain 0       %3d  |  bswAtten Chain 1       %3d  |",
                 pModal->bswAtten[0], pModal->bswAtten[1]);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  bswMargin Chain 0      %3d  |  bswMargin Chain 1      %3d  |",
                 pModal->bswMargin[0], pModal->bswMargin[1]);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  switch settling HT40   %3d  |                              |",
                 pModal->swSettleHt40);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  Anlg Out Bias (ob_cck) %3d  |  Anlg Out Bias (ob_psk) %3d  |",
               pModal->ob_cck, pModal->ob_psk);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  Anlg Out Bias (ob_qam) %3d  |  Anlg Out Bias (pal_off)%3d  |",
               pModal->ob_qam, pModal->ob_pal_off);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1," |  Anlg Drv Bias (db1)    %3d  |  Anlg Drv Bias (db2)    %3d  |",
               pModal->db1, pModal->db2);
        //SendIt(client,buffer);    
    //}
    SformatOutput(buffer, MBUFFER-1," |=============================================================|");
    //SendIt(client,buffer); 

#define SPUR_TO_KHZ(is2GHz, spur)    ( ((spur) + ((is2GHz) ? 23000 : 49000)) * 100 )
#define NO_SPUR                      ( 0x8000 )

    /* Print spur data */
    SformatOutput(buffer, MBUFFER-1,"============================Spur Information===========================");
    //SendIt(client,buffer);
    for( i = 0; i < 1; i++ ) 
    {
        if( i == 0 ) 
         {
            SformatOutput(buffer, MBUFFER-1,"| 11G Spurs in MHz (Range of 0 defaults to channel width)             |");
            //SendIt(client,buffer);
        }
        pModal = &(pEepStruct->modalHeader);
        noMoreSpurs = 0;
        memset(buffer, 0x0, sizeof(buffer));
        for( j = 0; j < AR9287_EEPROM_MODAL_SPURS; j++ ) 
        {
            //pModal->spurChans[j].spurChan = target2host16(pModal->spurChans[j].spurChan);
            if( (pModal->spurChans[j].spurChan == NO_SPUR) || noMoreSpurs ) 
            {
                noMoreSpurs = 1;
                SformatOutput(buffer2, MBUFFER-1,"|   NO SPUR   ");
                strcat(buffer,buffer2);
                //SendIt(client,buffer);
            }
            else 
            {
                SformatOutput(buffer2, MBUFFER-1,"|   %4d.%1d   ", SPUR_TO_KHZ(i, pModal->spurChans[j].spurChan)/1000,
                         (SPUR_TO_KHZ(i, pModal->spurChans[j].spurChan)/100) % 10);
                strcat(buffer,buffer2);         
                //SendIt(client,buffer);
            }
        }
        SformatOutput(buffer2, MBUFFER-1,"|");
        strcat(buffer,buffer2); 
        //SendIt(client,buffer);
        
        memset(buffer, 0x0, sizeof(buffer));
        for( j = 0; j < AR9287_EEPROM_MODAL_SPURS; j++ ) 
        {
            if( (pModal->spurChans[j].spurChan == NO_SPUR) || noMoreSpurs ) 
            {
                noMoreSpurs = 1;
                SformatOutput(buffer2, MBUFFER-1,"|             ");
                strcat(buffer,buffer2); 
                //SendIt(client,buffer);
            }
            else 
            {
                SformatOutput(buffer2, MBUFFER-1,"|<%2d.%1d-=-%2d.%1d>",
                         pModal->spurChans[j].spurRangeLow / 10, pModal->spurChans[j].spurRangeLow % 10,
                         pModal->spurChans[j].spurRangeHigh / 10, pModal->spurChans[j].spurRangeHigh % 10);
                strcat(buffer,buffer2); 
                //SendIt(client,buffer);
            }
        }
        SformatOutput(buffer2, MBUFFER-1,"|");
        strcat(buffer,buffer2);
        //SendIt(client,buffer);
    }
    SformatOutput(buffer, MBUFFER-1,"|=====================================================================|");
    //SendIt(client,buffer);

#if 0
    /* Print Noise Floor Calibration Data (stored in futureModal[]) */
    SformatOutput(buffer, MBUFFER-1,"============================Noise Floor Info===========================");
    //SendIt(client,buffer);
    for( i = 0; i < 1; i++ ) 
    {

        if( i == 0 ) 
        {
            Freqs[0] = 2437;
            Freqs[1] = 0;
            Freqs[2] = 0;
            Freqs[3] = 2437;
            Freqs[4] = 0;
            Freqs[5] = 0;
        }

        if( i == 0 ) 
        {
            SformatOutput(buffer, MBUFFER-1,"| 11G Noise Floor Calibration Data                                    |");
            //SendIt(client,buffer);
            SformatOutput(buffer, MBUFFER-1,"|==================================|==================================|");
            //SendIt(client,buffer);
            SformatOutput(buffer, MBUFFER-1,"| Chain 0                          | Chain 1                          |");
            //SendIt(client,buffer);
            SformatOutput(buffer, MBUFFER-1,"|==================================|==================================|");
            //SendIt(client,buffer);
            
            memset(buffer, 0x00, sizeof(buffer));
            for( j = 0; j < 6; j++ ) 
            {
                SformatOutput(buffer2, MBUFFER-1,"|   %04d   ", Freqs[j]);
                strcat(buffer, buffer2);
                //SendIt(client,buffer);
            }
            SformatOutput(buffer2, MBUFFER-1,"|");
            strcat(buffer, buffer2);
            //SendIt(client,buffer);
        }
        SformatOutput(buffer, MBUFFER-1,"|=====================================================================|");
        //SendIt(client,buffer);
    }
#endif
#undef SPUR_TO_KHZ
#undef NO_SPUR

    /* Print calibration info */
    for( i = 0; i < 1; i++ ) 
    {
        for( c = 0; c < AR9287_MAX_CHAINS; c++ ) 
        {
            if( !((pBase->txMask >> c) & 1) ) 
            {
                continue;
            }
            if( i == 0 ) 
            {
                pDataPerChannel = (CAL_DATA_PER_FREQ_U_AR9287*)pEepStruct->calPierData2G[c];
                numChannels = AR9287_NUM_2G_CAL_PIERS;
                pChannel = pEepStruct->calFreqPier2G;
            }
            xpdMask = pEepStruct->modalHeader.xpdGain;

            numXpdGain = 0;
            /* Calculate the value of xpdgains from the xpdGain Mask */
            for( j = 1; j <= AR9287_PD_GAINS_IN_MASK; j++ ) 
            {
                if( (xpdMask >> (AR9287_PD_GAINS_IN_MASK - j)) & 1 ) 
                {
                    if( numXpdGain >= AR9287_NUM_PD_GAINS ) 
                    {
                        //assert(0);
                        break;
                    }
                    xpdGainValues[numXpdGain++] = (A_UINT16)(AR9287_PD_GAINS_IN_MASK - j);
                }
            }

            SformatOutput(buffer, MBUFFER-1,"===============Power Calibration Information Chain %d =======================", c);
            //SendIt(client,buffer);
            for( channelRowCnt = 0; channelRowCnt < numChannels; channelRowCnt += 5 ) 
            {
                //adding a temp variable, having the expression (channelRowCnt + 5) in
                //A_MIN, causes incorrect minimum value to be returned.
                temp = channelRowCnt + 5;
                rowEndsAtChan = (A_UINT16)A_MIN(numChannels, temp);
                memset(buffer, 0x0, sizeof(buffer));
                for( channelCount = channelRowCnt; (channelCount < rowEndsAtChan) && (pChannel[channelCount] != 0xff) ; channelCount++ ) 
                {
                    SformatOutput(buffer2, MBUFFER-1,"|     %04d     ", FBIN2FREQ(pChannel[channelCount], 1));
                    strcat(buffer, buffer2);
                    //SendIt(client,buffer);
                }
                SformatOutput(buffer2, MBUFFER-1,"|");
                strcat(buffer, buffer2);
                //SendIt(client,buffer);
                
                SformatOutput(buffer, MBUFFER-1,"|==============|==============|==============|==============|==============|");
                //SendIt(client,buffer);
                memset(buffer, 0x0, sizeof(buffer));
                for( channelCount = channelRowCnt; (channelCount < rowEndsAtChan) && (pChannel[channelCount] != 0xff); channelCount++ ) 
                {
                    SformatOutput(buffer2, MBUFFER-1,"|temp   ref    ");
                    strcat(buffer, buffer2);
                    //SendIt(client,buffer);
                }
                SformatOutput(buffer2, MBUFFER-1,"|");
                strcat(buffer, buffer2);
                //SendIt(client,buffer);
                memset(buffer, 0x0, sizeof(buffer));
                for( channelCount = channelRowCnt; (channelCount < rowEndsAtChan) && (pChannel[channelCount] != 0xff); channelCount++ ) 
                {
                    SformatOutput(buffer2, MBUFFER-1,"|sens   pwr    ");
                    strcat(buffer, buffer2);
                    //SendIt(client,buffer);
                }
                SformatOutput(buffer2, MBUFFER-1,"|");
                strcat(buffer, buffer2);
                //SendIt(client,buffer);

                for( k = 0; k < numXpdGain; k++ ) 
                {
                    //if( CalSetup.openLoopPwrCntl ) 
                    //{
                        SformatOutput(buffer, MBUFFER-1,"|              |              |              |              |              |");
                        //SendIt(client,buffer);
                    //}
                    //else 
                    //{
                    //    SformatOutput(buffer, MBUFFER-1,"|              |              |              |              |              |\n");
                    //    SendIt(client,buffer);
                    //    SformatOutput(buffer, MBUFFER-1,"|              |              |              |              |              |\n");
                    //    SendIt(client,buffer);
                    //}

                    for( vpdCount = 0; vpdCount < AR9287_PD_GAIN_ICEPTS; vpdCount++ ) 
                    {
                        memset(buffer, 0x0, sizeof(buffer));
                        for( channelCount = channelRowCnt; (channelCount < rowEndsAtChan) && (pChannel[channelCount] != 0xff); channelCount++ ) 
                        {
                            /* open loop power control */
                            SformatOutput(buffer2, MBUFFER-1,"|%4d  %3d     ",
                                     (A_INT8)pDataPerChannel[channelCount].calDataOpen.vpdPdg[k][vpdCount],
                                     (A_INT8)pDataPerChannel[channelCount].calDataOpen.pwrPdg[k][vpdCount]);
                            //SendIt(client,buffer);
                            strcat(buffer, buffer2);
                        }
                        SformatOutput(buffer2, MBUFFER-1,"|");
                        strcat(buffer, buffer2);
                        //SendIt(client,buffer);
                    }
                }
                SformatOutput(buffer, MBUFFER-1,"|              |              |              |              |              |");
                //SendIt(client,buffer);
                //for( channelCount = channelRowCnt; (channelCount < rowEndsAtChan) && (pChannel[channelCount] != 0xff); channelCount++ ) 
                //{
                //    SformatOutput(buffer, MBUFFER-1,"|@pcdac  %2d    ",CalSetup.pcdacBase);
                //    SendIt(client,buffer);
                //}
                //SformatOutput(buffer, MBUFFER-1,"|\n");
                //SendIt(client,buffer);

                SformatOutput(buffer, MBUFFER-1,"|              |              |              |              |              |");
                //SendIt(client,buffer);
                SformatOutput(buffer, MBUFFER-1,"|==============|==============|==============|==============|==============|");
                //SendIt(client,buffer);
            }
            SformatOutput(buffer, MBUFFER-1,"|");
            //SendIt(client,buffer);
#if 0
            //if(isKiwi(swDeviceID)) /* print PAL ON data */
            //{ 
                SformatOutput(buffer, MBUFFER-1,"===============Power Calibration Data (PAL ON) Chain %d =======================", c);
                //SendIt(client,buffer);
                for( channelRowCnt = 0; channelRowCnt < numChannels; channelRowCnt += 5 ) 
                {
                    //adding a temp variable, having the expression (channelRowCnt + 5) in
                    //A_MIN, causes incorrect minimum value to be returned.
                    temp = channelRowCnt + 5;
                    rowEndsAtChan = (A_UINT16)A_MIN(numChannels, temp);
                    memset(buffer, 0x0, sizeof(buffer));
                    for( channelCount = channelRowCnt; (channelCount < rowEndsAtChan) && (pChannel[channelCount] != 0xff) ; channelCount++ ) 
                    {
                        SformatOutput(buffer2, MBUFFER-1,"|     %04d     ", FBIN2FREQ(pChannel[channelCount], 1));
                        //SendIt(client,buffer);
                        strcat(buffer, buffer2);
                    }
                    SformatOutput(buffer2, MBUFFER-1,"|");
                    strcat(buffer, buffer2);
                    //SendIt(client,buffer);
                    SformatOutput(buffer, MBUFFER-1,"|==============|==============|==============|==============|==============|");
                    //SendIt(client,buffer);
                    memset(buffer, 0x0, sizeof(buffer));
                    for( channelCount = channelRowCnt; (channelCount < rowEndsAtChan) && (pChannel[channelCount] != 0xff); channelCount++ ) 
                    {
                        SformatOutput(buffer2, MBUFFER-1,"|temp   ref    ");
                        //SendIt(client,buffer);
                        strcat(buffer, buffer2);
                    }
                    SformatOutput(buffer2, MBUFFER-1,"|");
                    strcat(buffer, buffer2);
                    //SendIt(client,buffer);
                    memset(buffer, 0x0, sizeof(buffer));
                    for( channelCount = channelRowCnt; (channelCount < rowEndsAtChan) && (pChannel[channelCount] != 0xff); channelCount++ ) 
                    {
                        SformatOutput(buffer2, MBUFFER-1,"|sens   pwr    ");
                        //SendIt(client,buffer);
                        strcat(buffer, buffer2);
                    }
                    SformatOutput(buffer2, MBUFFER-1,"|");
                    strcat(buffer, buffer2);
                    //SendIt(client,buffer);

                    for( k = 0; k < numXpdGain; k++ ) 
                    {
                        //if( CalSetup.openLoopPwrCntl ) 
                        //{
                            SformatOutput(buffer, MBUFFER-1,"|              |              |              |              |              |");
                            //SendIt(client,buffer);
                        //}
                        //else 
                        //{
                        //    SformatOutput(buffer, MBUFFER-1,"|              |              |              |              |              |\n");
                        //    SendIt(client,buffer);
                        //    SformatOutput(buffer, MBUFFER-1,"|              |              |              |              |              |\n");
                        //    SendIt(client,buffer);
                        //}

                        for( vpdCount = 0; vpdCount < AR9287_PD_GAIN_ICEPTS; vpdCount++ ) 
                        {
                            memset(buffer, 0x0, sizeof(buffer));
                            for( channelCount = channelRowCnt; (channelCount < rowEndsAtChan) && (pChannel[channelCount] != 0xff); channelCount++ ) 
                            {
                                /* open loop power control */
                                SformatOutput(buffer2, MBUFFER-1,"|%4d  %3d     ",
                                         (A_INT8)pDataPerChannel[channelCount].calDataOpen.pcdac[k][vpdCount], /* stores temp sens */
                                         (A_INT8)pDataPerChannel[channelCount].calDataOpen.empty[k][vpdCount]); /* stores ref power */
                                //SendIt(client,buffer);
                                strcat(buffer, buffer2);
                            }
                            SformatOutput(buffer2, MBUFFER-1,"|");
                            strcat(buffer, buffer2);
                            //SendIt(client,buffer);
                        }
                    }
                    SformatOutput(buffer, MBUFFER-1,"|              |              |              |              |              |");
                    //SendIt(client,buffer);
                    //for( channelCount = channelRowCnt; (channelCount < rowEndsAtChan) && (pChannel[channelCount] != 0xff); channelCount++ ) 
                    //{
                    //    SformatOutput(buffer, MBUFFER-1,"|@pcdac  %2d    ",CalSetup.pcdacPalOn);
                    //    SendIt(client,buffer);
                    //}
                    //SformatOutput(buffer, MBUFFER-1,"|\n");
                    //SendIt(client,buffer);

                    SformatOutput(buffer, MBUFFER-1,"|==============|==============|==============|==============|==============|");
                    //SendIt(client,buffer);
                }
                SformatOutput(buffer, MBUFFER-1,"|");
                //SendIt(client,buffer);
            //}
#endif
        }//for( c = 0; c < AR9287_MAX_CHAINS; c++ ) 
    }//for( i = 0; i < 1; i++ )

    /* Print Target Powers */
    for( i = 0; i < 7; i++ ) 
    {
        if( (i >=0 && i <=2) && !(pBase->opCapFlags & AR9287_OPFLAGS_11A) ) 
        {
            continue;
        }
        if( (i >= 3 && i <=6) && !(pBase->opCapFlags & AR9287_OPFLAGS_11G) ) 
        {
            continue;
        }
        switch( i ) 
        {
            case 0:
            case 1:
            case 2:
                break;
            case 3:
                pPowerInfo = (CAL_TARGET_POWER_HT *)pEepStruct->calTargetPowerCck;
                pPowerInfoLeg = pEepStruct->calTargetPowerCck;
                numPowers = AR9287_NUM_2G_CCK_TARGET_POWERS;
                ratePrint = 1;
                numRates = 4;
                is2Ghz = TRUE;
                legacyPowers = TRUE;
                break;
            case 4:
                pPowerInfo = (CAL_TARGET_POWER_HT *)pEepStruct->calTargetPower2G;
                pPowerInfoLeg = pEepStruct->calTargetPower2G;
                numPowers = AR9287_NUM_2G_20_TARGET_POWERS;
                ratePrint = 0;
                numRates = 4;
                is2Ghz = TRUE;
                legacyPowers = TRUE;
                break;
            case 5:
                pPowerInfo = pEepStruct->calTargetPower2GHT20;
                numPowers = AR9287_NUM_2G_20_TARGET_POWERS;
                ratePrint = 2;
                numRates = 8;
                is2Ghz = TRUE;
                legacyPowers = FALSE;
                break;
            case 6:
                pPowerInfo = pEepStruct->calTargetPower2GHT40;
                numPowers = AR9287_NUM_2G_40_TARGET_POWERS;
                ratePrint = 2;
                numRates = 8;
                is2Ghz = TRUE;
                legacyPowers = FALSE;
                break;
        }
        SformatOutput(buffer, MBUFFER-1,"============================Target Power Info===============================");
        //SendIt(client,buffer);
        for( j = 0; j < numPowers; j+=4 ) 
        {
            memset(buffer, 0x0, sizeof(buffer));
            SformatOutput(buffer2, MBUFFER-1,"|   %s   ", sTargetPowerMode[i]);
            //SendIt(client,buffer);
            strcat(buffer, buffer2);
            for( k = j; k < A_MIN(j + 4, numPowers); k++ ) 
            {
                if( legacyPowers ) 
                {
                    channelValue = FBIN2FREQ(pPowerInfoLeg[k].bChannel, (A_BOOL)is2Ghz);
                }
                else 
                {
                    channelValue = FBIN2FREQ(pPowerInfo[k].bChannel, (A_BOOL)is2Ghz);
                }
                if( channelValue != 0xff ) 
                {
                    SformatOutput(buffer2, MBUFFER-1,"|     %04d     ",channelValue);
                    strcat(buffer, buffer2);
                }
                else 
                {
                    numPowers = k;
                    continue;
                }
            }
            SformatOutput(buffer2, MBUFFER-1,"|");
            strcat(buffer, buffer2);
            //SendIt(client,buffer);
            
            SformatOutput(buffer, MBUFFER-1,"|==============|==============|==============|==============|==============|");
            //SendIt(client,buffer);
            //memset(buffer, 0x0, sizeof(buffer));
            for( r = 0; r < numRates; r++ ) 
            {
				memset(buffer, 0x0, sizeof(buffer));
                SformatOutput(buffer2, MBUFFER-1,"|%s", sRatePrint[ratePrint][r]);
                //SendIt(client,buffer);
                strcat(buffer, buffer2);
                for( k = j; k < A_MIN(j + 4, numPowers); k++ ) 
                {
                    if( legacyPowers ) 
                    {
                        targetPower = pPowerInfoLeg[k].tPow2x[r] / 2;
                        SformatOutput(buffer2, MBUFFER-1,"|     %2d.%d     ", targetPower,
                                 (pPowerInfoLeg[k].tPow2x[r] % 2) * 5);
                        //SendIt(client,buffer);
                        strcat(buffer, buffer2);
                    }
                    else 
                    {
                        targetPower = pPowerInfo[k].tPow2x[r] / 2;
                        SformatOutput(buffer2, MBUFFER-1,"|     %2d.%d     ", targetPower,
                                 (pPowerInfo[k].tPow2x[r] % 2) * 5);
                        //SendIt(client,buffer);
                        strcat(buffer, buffer2);
                    }
                }
                SformatOutput(buffer2, MBUFFER-1,"|");
                strcat(buffer, buffer2);
                //SendIt(client,buffer);
            }
            SformatOutput(buffer, MBUFFER-1,"|==============|==============|==============|==============|==============|");
            //SendIt(client,buffer);
        }
    }
    SformatOutput(buffer, MBUFFER-1,"");
    //SendIt(client,buffer);

    /* Print Band Edge Powers */

    SformatOutput(buffer, MBUFFER-1,"=======================Test Group Band Edge Power========================");
    //SendIt(client,buffer);
    for( i = 0; (pEepStruct->ctlIndex[i] != 0) && (i < AR9287_NUM_CTLS); i++ ) 
    {
        if( ((pEepStruct->ctlIndex[i] & CTL_MODE_M) == CTL_11A) || ((pEepStruct->ctlIndex[i] & CTL_MODE_M) == CTL_5GHT20) ||
            ((pEepStruct->ctlIndex[i] & CTL_MODE_M) == CTL_5GHT40) ) 
        {
            is2Ghz = FALSE;
        }
        else 
        {
            is2Ghz = TRUE;
        }
        SformatOutput(buffer, MBUFFER-1,"|                                                                       |");
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1,"| CTL: 0x%02x %s                                           |",
                 pEepStruct->ctlIndex[i], sCtlType[pEepStruct->ctlIndex[i] & CTL_MODE_M]);
        //SendIt(client,buffer);
        SformatOutput(buffer, MBUFFER-1,"|=======|=======|=======|=======|=======|=======|=======|=======|=======|");
        //SendIt(client,buffer);
        for( c = 0; c < numTxChains; c++ ) 
        {
            SformatOutput(buffer, MBUFFER-1,"==================== Edges for %d chain operation ========================", (c+1));
            //SendIt(client,buffer);
            memset(buffer, 0x0, sizeof(buffer));
            SformatOutput(buffer2, MBUFFER-1,"| edge  ");
            //SendIt(client,buffer);
            strcat(buffer, buffer2);
            for( j = 0; j < AR9287_NUM_BAND_EDGES; j++ ) 
            {
                if( pEepStruct->ctlData[i].ctlEdges[c][j].bChannel == AR9287_BCHAN_UNUSED ) 
                {
                    SformatOutput(buffer2, MBUFFER-1,"|  --   ");
                    strcat(buffer, buffer2);
                    //SendIt(client,buffer);
                }
                else 
                {
                    tempFreq = FBIN2FREQ(pEepStruct->ctlData[i].ctlEdges[c][j].bChannel, is2Ghz);
                    SformatOutput(buffer2, MBUFFER-1,"| %04d  ", tempFreq);
                    strcat(buffer, buffer2);
                    //SendIt(client,buffer);
                }
            }
            SformatOutput(buffer2, MBUFFER-1,"|");
            strcat(buffer, buffer2);
            //SendIt(client,buffer);
            SformatOutput(buffer, MBUFFER-1,"|=======|=======|=======|=======|=======|=======|=======|=======|=======|");
            //SendIt(client,buffer);

            memset(buffer, 0x0, sizeof(buffer));
            SformatOutput(buffer2, MBUFFER-1,"| power ");
            strcat(buffer, buffer2);
            //SendIt(client,buffer);
            for( j = 0; j < AR9287_NUM_BAND_EDGES; j++ ) 
            {
                if( pEepStruct->ctlData[i].ctlEdges[c][j].bChannel == AR9287_BCHAN_UNUSED ) 
                {
                    SformatOutput(buffer2, MBUFFER-1,"|  --   ");
                    strcat(buffer, buffer2);
                    //SendIt(client,buffer);
                }
                else 
                {
                    SformatOutput(buffer2, MBUFFER-1,"| %2d.%d  ", pEepStruct->ctlData[i].ctlEdges[c][j].tPower / 2,
                             (pEepStruct->ctlData[i].ctlEdges[c][j].tPower % 2) * 5);
                    strcat(buffer, buffer2);
                    //SendIt(client,buffer);
                }
            }
            SformatOutput(buffer2, MBUFFER-1,"|");
            strcat(buffer, buffer2);
            //SendIt(client,buffer);
            SformatOutput(buffer, MBUFFER-1,"|=======|=======|=======|=======|=======|=======|=======|=======|=======|");
            //SendIt(client,buffer);
            
            memset(buffer, 0x0, sizeof(buffer));
            SformatOutput(buffer2, MBUFFER-1,"| flag  ");
            strcat(buffer, buffer2);
            //SendIt(client,buffer);
            for( j = 0; j < AR9287_NUM_BAND_EDGES; j++ ) 
            {
                if( pEepStruct->ctlData[i].ctlEdges[c][j].bChannel == AR9287_BCHAN_UNUSED ) 
                {
                    SformatOutput(buffer2, MBUFFER-1,"|  --   ");
                    strcat(buffer, buffer2);
                    //SendIt(client,buffer);
                }
                else 
                {
                    SformatOutput(buffer2, MBUFFER-1,"|   %1d   ", pEepStruct->ctlData[i].ctlEdges[c][j].flag);
                    strcat(buffer, buffer2);
                    //SendIt(client,buffer);
                }
            }
            SformatOutput(buffer2, MBUFFER-1,"|");
            strcat(buffer, buffer2);
            //SendIt(client,buffer);
            SformatOutput(buffer, MBUFFER-1,"=========================================================================");
            //SendIt(client,buffer);
        }//for( c = 0; c < numTxChains; c++ )
    }//for( i = 0; (pEepStruct->ctlIndex[i] != 0) && (i < AR9287_NUM_CTLS); i++ )
}
int Ar9287_PrintEepromStruct(int client)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    ar9287_eeprom_t * pEepStruct = &(ahp->ah_eeprom.map.mapAr9287);
    ar9287_eepromDump(client, pEepStruct);
    return VALUE_OK; 
}
A_INT32 Ar9287_CalPierOpenPowerGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, j, iMaxPier=NUM_2G_CAL_PIERS, iv=0;

	if (iy<0 || iy>=iMaxPier) {
		if (ix<0 || ix>=AR9287_MAX_CHAINS) {
			// get all i, all j
			for (i=0; i<AR9287_MAX_CHAINS; i++) {
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = ahp->ah_eeprom.map.mapAr9287.calPierData2G[i][j].calDataOpen.pwrPdg[0][0];
					} else {
						return ERR_VALUE_BAD;
					}
				}
				*num = AR9287_MAX_CHAINS*iMaxPier;
			}
		} else { // get all j for ix chain
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = ahp->ah_eeprom.map.mapAr9287.calPierData2G[ix][j].calDataOpen.pwrPdg[0][0];
					} else {
						return ERR_VALUE_BAD;
					}
				}
				*num = iMaxPier;
		}
	} else {
		if (iBand==band_BG) {
			value[0] = ahp->ah_eeprom.map.mapAr9287.calPierData2G[ix][iy].calDataOpen.pwrPdg[0][0];
		} else {
			return ERR_VALUE_BAD;
		}
		*num = 1;
	}
    return VALUE_OK; 
}
A_INT32 Ar9287_CalPierOpenVoltMeasGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, j, iMaxPier=NUM_2G_CAL_PIERS, iv=0;

	if (iy<0 || iy>=iMaxPier) {
		if (ix<0 || ix>=AR9287_MAX_CHAINS) {
			// get all i, all j
			for (i=0; i<AR9287_MAX_CHAINS; i++) {
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = ahp->ah_eeprom.map.mapAr9287.calPierData2G[i][j].calDataOpen.vpdPdg[0][0];
					} else {
						return ERR_VALUE_BAD;
					}
				}
				*num = AR9287_MAX_CHAINS*iMaxPier;
			}
		} else { // get all j for ix chain
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = ahp->ah_eeprom.map.mapAr9287.calPierData2G[ix][j].calDataOpen.vpdPdg[0][0];
					} else {
						return ERR_VALUE_BAD;
					}
				}
				*num = iMaxPier;
		}
	} else {
		if (iBand==band_BG) {
			value[0] = ahp->ah_eeprom.map.mapAr9287.calPierData2G[ix][iy].calDataOpen.vpdPdg[0][0];
		} else {
			return ERR_VALUE_BAD;
		}
		*num = 1;
	}
    return VALUE_OK; 
}
A_INT32 Ar9287_CalPierOpenPcdacGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	int i, j, iMaxPier=NUM_2G_CAL_PIERS, iv=0;

	if (iy<0 || iy>=iMaxPier) {
		if (ix<0 || ix>=AR9287_MAX_CHAINS) {
			// get all i, all j
			for (i=0; i<AR9287_MAX_CHAINS; i++) {
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = ahp->ah_eeprom.map.mapAr9287.calPierData2G[i][j].calDataOpen.pcdac[0][0];
					} else {
						return ERR_VALUE_BAD;
					}
				}
				*num = AR9287_MAX_CHAINS*iMaxPier;
			}
		} else { // get all j for ix chain
			for (j=0; j<iMaxPier; j++) {
				if (iBand==band_BG) {
					value[iv++] = ahp->ah_eeprom.map.mapAr9287.calPierData2G[ix][j].calDataOpen.pcdac[0][0];
				} else {
					return ERR_VALUE_BAD;
				}
			}
			*num = iMaxPier;
		}
	} else {
		if (iBand==band_BG) {
			value[0] = ahp->ah_eeprom.map.mapAr9287.calPierData2G[ix][iy].calDataOpen.pcdac[0][0];
		} else {
			return ERR_VALUE_BAD;
		}
		*num = 1;
	}
    return VALUE_OK; 
}

A_INT32 Ar9287_CaldataMemoryTypeGet(A_UCHAR *memType, A_INT32 maxlength)
{
    switch(ar5416_calibration_data_get(AH))
    {
        case calibration_data_flash:
            strcpy(memType, "flash");
            return 0;
        case calibration_data_eeprom:
            strcpy(memType, "eeprom");
            return 0;
        case calibration_data_otp:
            strcpy(memType, "otp");
            return 0;
    }
    return -1;
}
