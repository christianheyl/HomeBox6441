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
#include "ParameterConfigDef.h"
#include "Card.h"

#include "Ar9287EepromStructSet.h"
#include "instance.h"
#include "ConfigurationStatus.h"
#include "mEepStruct9287.h"

extern struct ath_hal *AH;

int setFREQ2FBIN(int freq, int iBand)
{
	int bin;
	if (freq==0)
		return 0;
	if (iBand==band_BG)
		bin = FREQ2FBIN(freq,1);
	else
		bin = FREQ2FBIN(freq,0);
	return bin;
}
ar9287_eeprom_t *Ar9287EepromStructGet(void)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    return (ar9287_eeprom_t *)&ahp->ah_eeprom;
}
int Ar9287_LengthSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->baseEepHeader.length=(u_int16_t)*value;
    return 0;
}
int Ar9287_ChecksumSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	ar9287_eeprom_t *peep9287;
	peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();	 // prints the Current EEPROM structure
	peep9287->baseEepHeader.checksum=(u_int16_t)*value;
	return 0;
}
int Ar9287_ChecksumCalculate(void)
{
    ar9287_eeprom_t *peep9287;  
    A_UINT16 sum = 0, *pHalf, i;
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet(); 
    peep9287->baseEepHeader.checksum=0;
	UserPrint("Ar9287_ChecksumSet: checksum init=0x%x\n", peep9287->baseEepHeader.checksum);
    // prints the Current EEPROM structure
    //	printf("target endian %d\ client endian %d\n", targetEndian, clientEndian);
    //	printf("length %x\n", pEepStruct->baseEepHeader.length);
    // change endianness of 16 and 32 bit elements
    pHalf = (A_UINT16 *)peep9287;
    //apply the endian macro to any fields that we were using
    //	pEepStruct->baseEepHeader.length = host2target16(pEepStruct->baseEepHeader.length);
    //    for (i = 0; i < lengthStruct/2; i++) {
    for (i = 0; i < sizeof(ar9287_eeprom_t)/2; i++) {
        sum ^= *pHalf++;
    }

//    pEepStruct->baseEepHeader.checksum = host2target16(0xFFFF ^ sum);
	// Don't need to swap checksum.

#if 1
    //peep9287->baseEepHeader.checksum = sum;
	//UserPrint("Ar9287_ChecksumSet: checksum calculated before ^0xffff =0x%x\n", peep9287->baseEepHeader.checksum);
    peep9287->baseEepHeader.checksum = 0xFFFF ^ sum;
	UserPrint("Ar9287_ChecksumSet: checksum calculated after ^0xffff =0x%x\n", peep9287->baseEepHeader.checksum);
#else
	UserPrint("Ar9287CalDataCheckSumSet: byte count=0x%x, checksum (calculated)=0x%x\n", i, sum);
#endif
    return 0;
}
int Ar9287_eepromVersionSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->baseEepHeader.version=(u_int16_t)*value;
    return 0;
}
A_INT32 Ar9287templateVersion(int value)
{
    return 0;
}
int Ar9287_OpFlagsSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->baseEepHeader.opCapFlags=value;
    return 0;
}
int Ar9287_eepMiscSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->baseEepHeader.eepMisc=value;
    return 0;
}
int Ar9287_RegDmnSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	u_int16_t val=0;

	for (i=ix; i<2; i++) {
		if (iv>=num)
			break;
		Ar9287EepromStructGet()->baseEepHeader.regDmn[i] = (u_int16_t)value[iv++];
	}
	return 0;
}
A_INT32 Ar9287_MacAdressSet(A_UINT8 *mac)
{
    A_INT16 i;

    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

    for(i=0; i<6; i++)
        peep9287->baseEepHeader.macAddr[i]= mac[i];

    return 0;
}
int Ar9287_RxMaskSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->baseEepHeader.rxMask=value;
    return 0;
}
int Ar9287_TxMaskSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->baseEepHeader.txMask=value;
    return 0;
}
int Ar9287_RFSilentSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->baseEepHeader.rfSilent=value;
    return 0;
}
int Ar9287_BlueToothOptionsSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->baseEepHeader.blueToothOptions=value;
    return 0;
}
int Ar9287_DeviceCapSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->baseEepHeader.deviceCap = value;
    return 0;
}
A_INT32 Ar9287_CalibrationBinaryVersionSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);

    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        ahp->ah_eeprom.map.mapAr9287.baseEepHeader.binBuildNumber = (u_int32_t) *value;
    }
    else
    {
        return ERR_RETURN;
    }
    
    return VALUE_OK;
}
int Ar9287_DeviceTypeSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->baseEepHeader.deviceType=value;
    return 0;
}
int Ar9287_OpenLoopPwrCntlSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->baseEepHeader.openLoopPwrCntl=(u_int8_t)value;
    return 0;
}
int Ar9287_PwrTableOffsetSetSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->baseEepHeader.pwrTableOffset=(int8_t)value;
    return 0;
}
int Ar9287_TempSensSlopeSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->baseEepHeader.tempSensSlope=(int8_t)value;
    return 0;
}
int Ar9287_TempSensSlopePalOnSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->baseEepHeader.tempSensSlopePalOn=(int8_t)value;
    return 0;
}
int Ar9287_FutureBaseSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	for (i=ix; i<29; i++) {
		if (iv>=num)
			break;
		Ar9287EepromStructGet()->baseEepHeader.futureBase[ix] = value[iv++];
	}
	return 0;
}
A_INT32 Ar9287_CustomerDataSet(A_UCHAR *data, A_INT32 len)
{
    A_INT16 i;

    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

    for(i=0; i<len; i++)
        peep9287->custData[i]=data[i];

    return 0;
}
int Ar9287_AntCtrlChainSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    int i, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

    for (i=ix; i<2; i++) 
    {
        if (iv>=num)
        break;

        peep9287->modalHeader.antCtrlChain[i]=value[iv++];
    }
    return 0;
}
int Ar9287_AntCtrlCommonSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->modalHeader.antCtrlCommon=(u_int32_t)value;
    return 0;
}
int Ar9287_AntennaGainSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    int i, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

    for (i=ix; i<2; i++) 
    {
        if (iv>=num)
        break;

        peep9287->modalHeader.antennaGainCh[i]=value[iv++];
    }
    return 0;
}
int Ar9287_SwitchSettlingSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

    peep9287->modalHeader.switchSettling=(u_int8_t)value;
	return 0;
}
int Ar9287_TxRxAttenChSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    int i, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

    for (i=ix; i<2; i++) 
    {
        if (iv>=num)
        break;

        peep9287->modalHeader.txRxAttenCh[i]=value[iv++];
    }
    return 0;
}
int Ar9287_RxTxMarginChSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    int i, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

    for (i=ix; i<2; i++) 
    {
        if (iv>=num)
        break;

        peep9287->modalHeader.rxTxMarginCh[i]=value[iv++];
    }
    return 0;
}
int Ar9287_AdcDesiredSizeSet(int value)
{
	Ar9287EepromStructGet()->modalHeader.adcDesiredSize = (int8_t)value;
    return 0;
}
int Ar9287_TxEndToXpaOffSet(int value)
{
	Ar9287EepromStructGet()->modalHeader.txEndToXpaOff = (u_int8_t)value;
    return 0;
}
int Ar9287_TxEndToRxOnSet(int value)
{
	Ar9287EepromStructGet()->modalHeader.txEndToRxOn = (u_int8_t)value;
    return 0;
}
int Ar9287_TxFrameToXpaOnSet(int value)
{
	Ar9287EepromStructGet()->modalHeader.txFrameToXpaOn = (u_int8_t)value;
    return 0;
}
int Ar9287_Thresh62Set(int value)
{
	Ar9287EepromStructGet()->modalHeader.thresh62 = (u_int8_t)value;
    return 0;
}
int Ar9287_NoiseFloorThreshChSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	for (i=ix; i<AR9287_MAX_CHAINS; i++) {
		if (iv>=num)
			break;
		Ar9287EepromStructGet()->modalHeader.noiseFloorThreshCh[i] = (int8_t)value[iv++];
	}
    return 0;
}
int Ar9287_XpdGainSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	//int status = checkCmdSet(&value, tValue, UINT8BITS, 1, IsHEX);
	//if (status!=VALUE_OK)
	//	return status;
		    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            ahp->ah_eeprom.map.mapAr9287.modalHeader.xpdGain =  (u_int8_t)*value;
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
int Ar9287_XpdSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
	//int status = checkCmdSet(&value, tValue, UINT8BITS, 1, IsHEX);
	//if (status!=VALUE_OK)
	//	return status;
		    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            ahp->ah_eeprom.map.mapAr9287.modalHeader.xpd =  (u_int8_t)*value;
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
int Ar9287_IQCalIChSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    int i, iv=0;
	//int status = checkCmdSet(value, tValue, UINT8BITS, 1, NotHEX);
	//if (status!=VALUE_OK)
	//	return status;
		
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
		for (i=ix; i<AR9287_MAX_CHAINS; i++) 
		{
			if (iv>=num)
				break;
			ahp->ah_eeprom.map.mapAr9287.modalHeader.iqCalICh[i] = (u_int8_t)value[iv++];
		}
		return 0;
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_IQCalQChSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    int i, iv=0;
	//int status = checkCmdSet(value, tValue, UINT8BITS, 1, NotHEX);
	//if (status!=VALUE_OK)
	//	return status;
		
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
		for (i=ix; i<AR9287_MAX_CHAINS; i++) 
		{
			if (iv>=num)
				break;
			ahp->ah_eeprom.map.mapAr9287.modalHeader.iqCalQCh[i] = (u_int8_t)value[iv++];
		}
		return 0;
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
int Ar9287_PdGainOverlapSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->modalHeader.pdGainOverlap=(u_int8_t)value;
    return 0;
}
int Ar9287_XpaBiasLvlSet(int value)
{
	Ar9287EepromStructGet()->modalHeader.xpaBiasLvl = (u_int8_t)value;
    return 0;
}
int Ar9287_TxFrameToDataStartSet(int value)
{
	Ar9287EepromStructGet()->modalHeader.txFrameToDataStart = (u_int8_t)value;
    return 0;
}
int Ar9287_TxFrameToPaOnSet(int value)
{
	Ar9287EepromStructGet()->modalHeader.txFrameToPaOn = (u_int8_t)value;
    return 0;
}
int Ar9287_HT40PowerIncForPdadcSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->modalHeader.ht40PowerIncForPdadc=(u_int8_t)value;
    return 0;
}
A_INT32 Ar9287_BswAttenSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    int i, iv=0;
		
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
		for (i=ix; i<AR9287_MAX_CHAINS; i++) 
		{
			if (iv>=num)
				break;
			ahp->ah_eeprom.map.mapAr9287.modalHeader.bswAtten[i] = (u_int8_t)value[iv++];
		}
		return 0;
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT32 Ar9287_BswMarginSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
    int i, iv=0;
		
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
		for (i=ix; i<AR9287_MAX_CHAINS; i++) 
		{
			if (iv>=num)
				break;
			ahp->ah_eeprom.map.mapAr9287.modalHeader.bswMargin[i] = (u_int8_t)value[iv++];
		}
		return 0;
    }
    else
    {
        return ERR_RETURN;
    }
    return VALUE_OK;
}
A_INT32 Ar9287_SwSettleHT40Set(int *value, int ix, int iy, int iz, int num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
		    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            ahp->ah_eeprom.map.mapAr9287.modalHeader.swSettleHt40 =  (u_int8_t)*value;
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
A_INT32 Ar9287_ModalHeaderVersionSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
		    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            ahp->ah_eeprom.map.mapAr9287.modalHeader.version =  (u_int8_t)*value;
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
A_INT32 Ar9287_db1Set(int *value, int ix, int iy, int iz, int num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
		    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            ahp->ah_eeprom.map.mapAr9287.modalHeader.db1 =  (u_int8_t)*value;
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
A_INT32 Ar9287_db2Set(int *value, int ix, int iy, int iz, int num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
		    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            ahp->ah_eeprom.map.mapAr9287.modalHeader.db2 =  (u_int8_t)*value;
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
A_INT32 Ar9287_ob_cckSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
		    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            ahp->ah_eeprom.map.mapAr9287.modalHeader.ob_cck =  (u_int8_t)*value;
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
A_INT32 Ar9287_ob_pskSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
		    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            ahp->ah_eeprom.map.mapAr9287.modalHeader.ob_psk =  (u_int8_t)*value;
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
A_INT32 Ar9287_ob_qamSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
		    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            ahp->ah_eeprom.map.mapAr9287.modalHeader.ob_qam =  (u_int8_t)*value;
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
A_INT32 Ar9287_ob_pal_offSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    struct ath_hal_5416 *ahp = AH5416(AH);
		    
    if ((AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCI)  || (AH_PRIVATE(AH)->ah_devid == AR5416_DEVID_AR9287_PCIE))
    {
        if (iBand == band_BG)
        {
            ahp->ah_eeprom.map.mapAr9287.modalHeader.ob_pal_off =  (u_int8_t)*value;
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
int Ar9287_FutureModalSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	for (i=ix; i<30; i++) {
		if (iv>=num)
			break;
		Ar9287EepromStructGet()->modalHeader.futureModal[i] = (u_int8_t)value[iv++];
	}
	return 0;
}
int Ar9287_SpurChansSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    int i, val=0, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

    for (i=ix; i<AR9287_EEPROM_MODAL_SPURS; i++) 
    {
        if (iv>=num)
        break;

		if (FBIN2FREQ(value[iv],1) > 2300 && FBIN2FREQ(value[iv],1) < 2500)
        	peep9287->modalHeader.spurChans[i].spurChan=(u_int16_t)value[iv++];
    }
    return 0;
}
int Ar9287_SpurRangeLowSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    int i, val=0, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

    for (i=ix; i<AR9287_EEPROM_MODAL_SPURS; i++) 
    {
        if (iv>=num)
        break;
        peep9287->modalHeader.spurChans[i].spurRangeLow=(u_int8_t)value[iv++];
    }
    return 0;
}
int Ar9287_SpurRangeHighSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    int i, val=0, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

    for (i=ix; i<AR9287_EEPROM_MODAL_SPURS; i++) 
    {
        if (iv>=num)
        break;
        peep9287->modalHeader.spurChans[i].spurRangeHigh=(u_int8_t)value[iv++];
    }
    return 0;
}
int Ar9287_CalPierFreqSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    int i, iv=0;
    A_UINT8 bin;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

    for (i=ix; i<AR9287_NUM_2G_CAL_PIERS; i++) 
    {
        if (iv>=num)
        break;

        bin = setFREQ2FBIN(value[iv++], 0);		
        peep9287->calFreqPier2G[i]=bin;
	}
    return 0;
}
int Ar9287_CalPierDataSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    int i, j, j0,idx=0,iv=0;
    A_UINT8 bin;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

	for (i=ix; i<AR9287_MAX_CHAINS; i++) {
		if (i==ix)
			j0=iy;
		else
			j0=0;	
		
		for (j=j0; j<AR9287_NUM_2G_CAL_PIERS; j++) {
			if (iv>=num)
				break;
			if(iz%10)
				break;
			idx = iz/10;
			switch(idx)
			{
				case 0:	/* power correction */
					peep9287->calPierData2G[ix][iy].calDataOpen.pwrPdg[0][0]=(u_int8_t)value[iv++];
					break;
				case 1:	/* temperature */
					peep9287->calPierData2G[ix][iy].calDataOpen.vpdPdg[0][0]=(u_int8_t)value[iv++];
					break;
				case 2:	/* pcdac */
					peep9287->calPierData2G[ix][iy].calDataOpen.pcdac[0][0]=(u_int8_t)value[iv++];
					break;
				case 3:	/* future use */
					peep9287->calPierData2G[ix][iy].calDataOpen.empty[0][0]=(u_int8_t)value[iv++];
					break;
				default:
					break;
			}
		}
	}
    return 0;
}
int Ar9287_CalTgtDatacckSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    int i, j, j0,idx=0,iv=0;
    A_UINT8 bin;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

	for (i=ix; i<AR9287_NUM_2G_CCK_TARGET_POWERS; i++) {
		if (iv>=num)
			break;

		if(iy==0)
		{
			peep9287->calTargetPowerCck[i].bChannel=value[iv++];
		}
		else if (iy>=1 && iy<=4)
			peep9287->calTargetPowerCck[i].tPow2x[iy-1]=(u_int8_t)value[iv++];
	}
    return 0;
}
int Ar9287_CalTgtDataSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    int i, j, j0,idx=0,iv=0;
    A_UINT8 bin;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

	for (i=ix; i<AR9287_NUM_2G_CCK_TARGET_POWERS; i++) {
		if (iv>=num)
			break;

		if(iy==0)
		{		
			peep9287->calTargetPower2G[i].bChannel=value[iv++];
		}
		else if (iy>=1 && iy<=4)
			peep9287->calTargetPower2G[i].tPow2x[iy-1]=(u_int8_t)value[iv++];
	}
    return 0;
}
int Ar9287_CalTgtDataHt20Set(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, j, j0,idx=0,iv=0;
	A_UINT8 bin;
	ar9287_eeprom_t *peep9287;	
	peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();	 // prints the Current EEPROM structure

	for (i=ix; i<AR9287_NUM_2G_20_TARGET_POWERS; i++) {
		if (iv>=num)
			break;

		if(iy==0)
		{
			peep9287->calTargetPower2GHT20[i].bChannel=value[iv++];
		}
		else if (iy>=1 && iy<=8)
			peep9287->calTargetPower2GHT20[i].tPow2x[iy-1]=(u_int8_t)value[iv++];
	}
	return 0;
}
int Ar9287_CalTgtDataHt40Set(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, j, j0,idx=0,iv=0;
	A_UINT8 bin;
	ar9287_eeprom_t *peep9287;	
	peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();	 // prints the Current EEPROM structure

	for (i=ix; i<AR9287_NUM_2G_40_TARGET_POWERS; i++) {
		if (iv>=num)
			break;
		if(iy==0)
		{
			peep9287->calTargetPower2GHT40[i].bChannel=value[iv++];
		}
		else if (iy>=1 && iy<=8)
			peep9287->calTargetPower2GHT40[i].tPow2x[iy-1]=(u_int8_t)value[iv++];
	}
	return 0;
}
A_INT32 Ar9287_CalTGTpwrcckSet(double *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, j, j0, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

	for (i=ix; i<AR9287_NUM_2G_20_TARGET_POWERS; i++) {
		if (iv>=num)
			break;
		
		if (i==ix)
			j0=iy;
		else
			j0=0;	
		
		for (j=j0; j<4; j++) {
			peep9287->calTargetPowerCck[i].tPow2x[j] = (u_int8_t)(value[iv++]*2);	// eep value is double of entered value
			if (iv>=num)
				break;
		}
	}
    return 0;
}
A_INT32 Ar9287_CalTGTPwrLegacyOFDMSet(double *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, j, j0, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    
	for (i=ix; i<AR9287_NUM_2G_20_TARGET_POWERS; i++) {
		if (iv>=num)
			break;
		
		if (i==ix)
			j0=iy;
		else
			j0=0;	
		
		for (j=j0; j<4; j++) {
				peep9287->calTargetPower2G[i].tPow2x[j] = (u_int8_t)(value[iv++]*2);	// eep value is double of entered value
			if (iv>=num)
				break;
		}
	}
    return 0;
}
A_INT32 Ar9287_CalTGTpwrht20Set(double *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, j, j0, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    
	for (i=ix; i<AR9287_NUM_2G_20_TARGET_POWERS; i++) {
		if (iv>=num)
			break;
		
		if (i==ix)
			j0=iy;
		else
			j0=0;	
		
		for (j=j0; j<8; j++) {
				peep9287->calTargetPower2GHT20[i].tPow2x[j] = (u_int8_t)(value[iv++]*2);	// eep value is double of entered value
			if (iv>=num)
				break;
		}
	}
    return 0;
}
A_INT32 Ar9287_CalTGTpwrht40Set(double *value, int ix, int iy, int iz, int num, int iBand)
{
    int i, j, j0, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

	for (i=ix; i<AR9287_NUM_2G_20_TARGET_POWERS; i++) {
		if (iv>=num)
			break;
		
		if (i==ix)
			j0=iy;
		else
			j0=0;
		
		for (j=j0; j<8; j++) {
				peep9287->calTargetPower2GHT40[i].tPow2x[j] = (u_int8_t)(value[iv++]*2);	// eep value is double of entered value
			if (iv>=num)
				break;
		}
       //peep9287->calTargetPower2GHT40[i].bChannel
	}
    return 0;
}
A_INT32 Ar9287_CalTGTpwrcckChannelSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8 bin;
	int i, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    
	for (i=ix; i<AR9287_NUM_2G_20_TARGET_POWERS; i++) {
		if (iv>=num)
			break;
		bin = setFREQ2FBIN(value[iv++], 0);
		peep9287->calTargetPowerCck[i].bChannel= bin;
	}
	
    if(num==2)peep9287->calTargetPowerCck[2].bChannel= 0xff;
	
	return 0;
}
A_INT32 Ar9287_CalTGTPwrChannelSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8 bin,i,iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    
    for (i=ix; i<AR9287_NUM_2G_20_TARGET_POWERS; i++) {
		if (iv>=num)
			break;
		bin = setFREQ2FBIN(value[iv++], 0);
		peep9287->calTargetPower2G[i].bChannel= bin;
	}
	return 0;
}
A_INT32 Ar9287_CalTGTpwrht20ChannelSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8 bin;
	int i, maxnum, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

	for (i=ix; i<AR9287_NUM_2G_20_TARGET_POWERS; i++) {
		if (iv>=num)
			break;
		bin = setFREQ2FBIN(value[iv++], 0);
		peep9287->calTargetPower2GHT20[i].bChannel= bin;
	}
	return 0;
}
A_INT32 Ar9287_CalTGTpwrht40ChannelSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8 bin;
	int i, maxnum, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    
	for (i=ix; i<AR9287_NUM_2G_20_TARGET_POWERS; i++) {
		if (iv>=num)
			break;
		bin = setFREQ2FBIN(value[iv++], 0);
		peep9287->calTargetPower2GHT40[i].bChannel= bin;
	}
	return 0;
}
A_INT32 Ar9287_CtlIndexSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int maxnum, i, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

	for (i=ix; i<AR9287_NUM_CTLS_2G; i++) {
		if (iv>=num)
			break;
		peep9287->ctlIndex[i] = (u_int8_t)(value[iv++]);
	}
    return 0;
}
int Ar9287_CtlDataSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
    int i, j, j0, k, idx=0,iv=0;
    A_UINT8 bin;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

	for (i=ix; i<AR9287_NUM_CTLS; i++) {
		if (i==ix)
			j0=iy;
		else
			j0=0;	
		
		for (j=j0; j<AR9287_MAX_CHAINS; j++) {
			if (iv>=num)
				break;
			idx = iy%2;
			switch(idx)
			{
				case 0:	/* channel */
					if(iy>=8)
					{
						/*chain1*/
						peep9287->ctlData[ix].ctlEdges[iy/8][(iy-8)/2].bChannel=value[iv++];
					}
					else
					{	
						/*chain0*/
						peep9287->ctlData[ix].ctlEdges[iy/8][iy/2].bChannel=value[iv++];
					}
					break;
				case 1:	/* power & flag */
					if(iy>=8)
					{
						/*chain1*/
						peep9287->ctlData[ix].ctlEdges[iy/8][(iy-8)/2].tPower = (u_int8_t)(value[iv] & 0x3F);
						peep9287->ctlData[ix].ctlEdges[iy/8][(iy-8)/2].flag = (u_int8_t)(value[iv++]>>6);
					}
					else
					{	
						/*chain0*/
						peep9287->ctlData[ix].ctlEdges[iy/8][iy/2].tPower = (u_int8_t)(value[iv] & 0x3F);
						peep9287->ctlData[ix].ctlEdges[iy/8][iy/2].flag = (u_int8_t)(value[iv++]>>6);
					}
					break;
				default:
					break;
			}
		}
	}
    return 0;
}
int Ar9287_PaddingSet(int value)
{
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    peep9287->padding=value;
    return 0;
}
A_INT32 Ar9287_CtlPowerSet(double *value, int ix, int iy, int iz, int num, int iBand)
{
	u_int8_t  value6;
	A_UINT8 bin;
	int i, j, j0, iEdge, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

	for (i=ix; i<AR9287_NUM_CTLS_2G; i++) {
		if (iv>=num)
			break;
		
		if (i==ix)
			j0=iy;
		else
			j0=0;	
		
		for (j=j0; j<AR9287_NUM_BAND_EDGES_2G; j++) {
			value6 = ((u_int8_t)(value[iv++]*2.0)) & 0x3f;
			peep9287->ctlData[i].ctlEdges[0][j].tPower = value6;
			peep9287->ctlData[i].ctlEdges[1][j].tPower = value6;

          	if(num==3)
    		{
				peep9287->ctlData[i].ctlEdges[0][3].tPower = 0;
				peep9287->ctlData[i].ctlEdges[1][3].tPower = 0;
    		}

			if (iv>=num)
				break;
		}
	}
    return 0;
}
A_INT32 Ar9287_CtlFlagSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	u_int8_t  value2;
	A_UINT8 bin;
	int i, j, j0, iCtl, iEdge, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    
	for (i=ix; i<AR9287_NUM_CTLS_2G; i++) {
		if (iv>=num)
			break;
		
		if (i==ix)
			j0=iy;
		else
			j0=0;	
		
		for (j=j0; j<AR9287_NUM_BAND_EDGES_2G; j++) {
			value2 = ((u_int8_t)value[iv++]) & 0x3;
			peep9287->ctlData[i].ctlEdges[0][j].flag = value2;
            peep9287->ctlData[i].ctlEdges[1][j].flag = value2;

          	if(num==3)
			{
				peep9287->ctlData[i].ctlEdges[0][3].flag = 0;
				peep9287->ctlData[i].ctlEdges[1][3].flag = 0;
			}

			if (iv>=num)
				break;
		}
	}
    return 0;
}
A_INT32 Ar9287_CtlChannelSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8 bin;
	int i, j, j0, iCtl, iEdge, iv=0;
    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure
    
	for (i=ix; i<AR9287_NUM_CTLS_2G; i++) {
		if (iv>=num)
			break;
		
		if (i==ix)
			j0=iy;
		else
			j0=0;
		
		for (j=j0; j<AR9287_NUM_BAND_EDGES_2G; j++) {
			bin = setFREQ2FBIN(value[iv++], 0);
			peep9287->ctlData[i].ctlEdges[0][j].bChannel = bin;
			peep9287->ctlData[i].ctlEdges[1][j].bChannel = bin;
          	if(num==3)
            {
			    peep9287->ctlData[i].ctlEdges[0][3].bChannel = 0;
			    peep9287->ctlData[i].ctlEdges[1][3].bChannel = 0;
            }
			if (iv>=num)
				break;
		}
	}
    return 0;
}
//given the rate index as per rate_constant.c find and return the target power
int Ar9287_RefPwrSet(int freq) 
{
    A_UINT8 powerT2 = 0;
    //int index;
    int chainpower[2];

    unsigned int tmpVal;
    unsigned int a;

    ar9287_eeprom_t *peep9287;  
    peep9287 = (ar9287_eeprom_t *)Ar9287EepromStructGet();   // prints the Current EEPROM structure

    if(freq==FBIN2FREQ(peep9287->calFreqPier2G[0], 1))
    {
        chainpower[0]=peep9287->calPierData2G[0][0].calDataOpen.pwrPdg[0][0];
        chainpower[1]=peep9287->calPierData2G[1][0].calDataOpen.pwrPdg[0][0];    
    
    }
    else if(freq==FBIN2FREQ(peep9287->calFreqPier2G[1], 1))
    {   
        chainpower[0]=peep9287->calPierData2G[0][1].calDataOpen.pwrPdg[0][0];
        chainpower[1]=peep9287->calPierData2G[1][1].calDataOpen.pwrPdg[0][0];    
    
    }
    else if(freq>=FBIN2FREQ(peep9287->calFreqPier2G[2], 1))
    {  
        chainpower[0]=peep9287->calPierData2G[0][2].calDataOpen.pwrPdg[0][0];
        chainpower[1]=peep9287->calPierData2G[1][2].calDataOpen.pwrPdg[0][0];    
    
    }
    else if(freq > FBIN2FREQ(peep9287->calFreqPier2G[0], 1) &&
			freq < FBIN2FREQ(peep9287->calFreqPier2G[1], 1))
    {    
        chainpower[0]=(peep9287->calPierData2G[0][0].calDataOpen.pwrPdg[0][0]
                            +peep9287->calPierData2G[0][1].calDataOpen.pwrPdg[0][0])/2;
        chainpower[1]=(peep9287->calPierData2G[1][0].calDataOpen.pwrPdg[0][0]   
                            +peep9287->calPierData2G[1][1].calDataOpen.pwrPdg[0][0])/2;
    }
    else if(freq > FBIN2FREQ(peep9287->calFreqPier2G[1], 1) &&
			freq < FBIN2FREQ(peep9287->calFreqPier2G[2], 1))
    {   
        chainpower[0]=(peep9287->calPierData2G[0][1].calDataOpen.pwrPdg[0][0]
                            +peep9287->calPierData2G[0][2].calDataOpen.pwrPdg[0][0])/2;
        chainpower[1]=(peep9287->calPierData2G[1][1].calDataOpen.pwrPdg[0][0]   
                            +peep9287->calPierData2G[1][2].calDataOpen.pwrPdg[0][0])/2;
    }

	// To enable open-loop power control
	MyRegisterRead(0xa270,&tmpVal); // Chain 0
	tmpVal = tmpVal & 0xFCFFFFFF;
	tmpVal = tmpVal | (0x3 << 24); //bb_error_est_mode
	MyRegisterWrite(0xa270, tmpVal);
//    printf("reg 0xa270 is 0x%x\n", REGR(devNum, 0xa270));
	tmpVal = MyRegisterRead(0xb270,&tmpVal); // Chain 1
	tmpVal = tmpVal & 0xFCFFFFFF;
	tmpVal = tmpVal | (0x3 << 24); //bb_error_est_mode
	MyRegisterWrite(0xb270, tmpVal);
//    printf("reg 0xb270 is 0x%x\n", REGR(devNum, 0xb270));

    /* write the olpc ref power for each chain */
   // Chain 0
        MyRegisterRead(0xa398,&tmpVal); 
        tmpVal = tmpVal & 0xff00ffff;
//        printf("Chain 0 txPower before %d\n", txPower);
        a = (chainpower[0])&0xff; /* removed the signed part */
        //printf("a 0x%x\n", a);
        tmpVal = tmpVal | (a << 16);
        //printf("tmpVal 0x%x \n", tmpVal);
        MyRegisterWrite(0xa398, tmpVal);
    

    // Chain 1
        MyRegisterRead(0xb398,&tmpVal); 
        tmpVal = tmpVal & 0xff00ffff;
//        printf("Chain 1 txPower before %d\n", txPower);
        a = (chainpower[1])&0xff; /* removed the signed part */
        //printf("Ar9287_RefPwrSet: a 0x%x\n", a);
        tmpVal = tmpVal | (a << 16);
        //printf("Ar9287_RefPwrSet:tmpVal 0x%x \n", tmpVal);
        MyRegisterWrite(0xb398, tmpVal);

    return 0;
}

extern void ar5416_calibration_data_set(struct ath_hal *ah, int32_t source);
A_INT32 Ar9287_CaldataMemoryTypeSet(A_UCHAR *memType)
{
    if(!strcmp(memType, "eeprom"))
        ar5416_calibration_data_set(AH, calibration_data_eeprom);
    else if(!strcmp(memType, "flash"))
        ar5416_calibration_data_set(AH, calibration_data_flash);
    else if(!strcmp(memType, "otp"))
        ar5416_calibration_data_set(AH, calibration_data_otp);
    else
        return -1;
    return 0;
}

#if AH_BYTE_ORDER == AH_BIG_ENDIAN
int swaploop=0;
void 
ar9287SwapEeprom(ar9287_eeprom_t *eep)
{
    u_int32_t dword;
    u_int16_t word;
    int	      i;
	int addr;
    u_int16_t *eep_data;
	eep_data = (u_int16_t *)eep;

	if(swaploop==0)
		UserPrint("Swap EEP\n");
	else
	{
		UserPrint("Swap EEP Back\n");
		swaploop=0;
	}
		
    word = SWAP16(eep->baseEepHeader.length);
    eep->baseEepHeader.length = word;

    word = SWAP16(eep->baseEepHeader.checksum);
    eep->baseEepHeader.checksum = word;

    word = SWAP16(eep->baseEepHeader.version);
    eep->baseEepHeader.version = word;

    word = SWAP16(eep->baseEepHeader.regDmn[0]);
    eep->baseEepHeader.regDmn[0] = word;

    word = SWAP16(eep->baseEepHeader.regDmn[1]);
    eep->baseEepHeader.regDmn[1] = word;

    for (i=0;i<AR9287_MAX_CHAINS;i++)
    {
        dword = SWAP32(eep->modalHeader.antCtrlChain[i]);
		eep->modalHeader.antCtrlChain[i] = dword;
	}

    dword = SWAP32(eep->modalHeader.antCtrlCommon);
    eep->modalHeader.antCtrlCommon = dword;

    for (i=0;i<AR9287_EEPROM_MODAL_SPURS;i++)
    {
        word = SWAP16(eep->modalHeader.spurChans[i].spurChan);
		eep->modalHeader.spurChans[i].spurChan = word;
	}

	if(AH_PRIVATE(AH)->ah_flags == AH_USE_EEPROM)
	{
	    for (addr = 0; addr < sizeof(ar9287_eeprom_t) / sizeof(u_int16_t); 
	        addr++)
	    {
	   		eep_data[addr] = SWAP16(eep_data[addr]);
			//UserPrint("offset=0x%x, eepromValue=0x%x\n", (addr+0x80), eep_data[addr]);
	    }
	}
}
#endif
