#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "wlantype.h"

#include "Sticky.h"
#include "Field.h"
#include "DevConfigDiff.h"
#include "art_utf_common.h"
#include "ResetForce.h"

//
// hal header files
//
#include "Qc9KEeprom.h"
#include "MyAccess.h"
#include "Qc9KDevice.h"

#include "qc98xx_eeprom.h"
#include "Qc98xxEepromStructSet.h"
#include "Qc98xxEepromStruct.h"
#include "Qc98xxmEep.h"

#include "UserPrint.h"

#define TRUE 1
#define FALSE 0

extern QC98XX_EEPROM *Qc98xxEepromStructGet(void);

#define EEPROM_BASE_POINTER     ((QC98XX_EEPROM *)(pQc9kEepromArea))
#define EEPROM_ITEM_DATA_POINTER(v) ((A_UINT8 *)&(EEPROM_BASE_POINTER->v))
#define EEPROM_ITEM_OFFSET(v)   ((A_UINT16)(EEPROM_ITEM_DATA_POINTER(v) - pQc9kEepromArea))
#define EEPROM_ITEM_SIZE(v) ((A_UINT8)(sizeof(EEPROM_BASE_POINTER->v)))
#define EEPROM_ITEM_ENTRY(v) (EEPROM_ITEM_OFFSET(v), EEPROM_ITEM_SIZE(v), EEPROM_ITEM_DATA_POINTER(v))
#define EEPROM_CONFIG_DIFF_CHANGE(v) (ConfigDiffChange(EEPROM_ITEM_OFFSET(v), EEPROM_ITEM_SIZE(v), EEPROM_ITEM_DATA_POINTER(v)))
#define EEPROM_CONFIG_DIFF_CHANGE_ARRAY(v, c) (ConfigDiffChange(EEPROM_ITEM_OFFSET(v), EEPROM_ITEM_SIZE(v)*(c), EEPROM_ITEM_DATA_POINTER(v)))

#define EEPROM_CAL_INFO_ADD(v) (CalInfoChange(EEPROM_ITEM_OFFSET(v), EEPROM_ITEM_SIZE(v), EEPROM_ITEM_DATA_POINTER(v)))
#define EEPROM_CAL_INFO_ADD_ARRAY(v, c) (CalInfoChange(EEPROM_ITEM_OFFSET(v), EEPROM_ITEM_SIZE(v)*(c), EEPROM_ITEM_DATA_POINTER(v)))

int setFREQ2FBIN(int freq, int iBand)
{
    int bin;
    if (freq==0)
        return 0;
    if (iBand==band_BG)
        bin = WHAL_FREQ2FBIN(freq,1);
    else
        bin = WHAL_FREQ2FBIN(freq,0);
    return bin;
}

A_INT32 Qc98xxEepromVersion(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.eeprom_version = (A_UINT8) value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.eeprom_version);
    return 0;
}

A_INT32 Qc98xxTemplateVersion(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.template_version = (A_UINT8) value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.template_version);
    return 0;
}
/*
 *Function Name:Qc98xxAntCtrlCommonSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set AntCtrlCommon flag in field of eeprom struct (A_UINT32)
 *Returns: zero
 */
A_INT32 Qc98xxAntCtrlCommonSet(int value, int iBand)
{
	if (iBand==band_BG) {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].antCtrlCommon = (A_UINT32)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].antCtrlCommon);
	} else {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].antCtrlCommon = (A_UINT32)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].antCtrlCommon);
	}
    return 0;
}
/*
 *Function Name:Qc98xxAntCtrlCommon2Set
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set AntCtrlCommon2 flag in field of eeprom struct (A_UINT32)
 *Returns: zero
 */
A_INT32 Qc98xxAntCtrlCommon2Set(int value, int iBand)
{
	if (iBand==band_BG) {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].antCtrlCommon2 = (A_UINT32)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].antCtrlCommon2);
	} else {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].antCtrlCommon2 = (A_UINT32)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].antCtrlCommon2);
	}
    return 0;
}

A_INT32 Qc98xxRxFilterCapSet(int value, int iBand)
{
	if (iBand==band_BG) {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].rxFilterCap = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].rxFilterCap);
	} else {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].rxFilterCap = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].rxFilterCap);
	}
    return 0;
}

A_INT32 Qc98xxRxGainCapSet(int value, int iBand)
{
	if (iBand==band_BG) {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].rxGainCap = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].rxGainCap);
	} else {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].rxGainCap = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].rxGainCap);
	}
    return 0;
}

#if 0
/*
 *Function Name:Qc98xxTempSlopeSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set TempSlope flag in field of eeprom struct (A_INT8)
 *Returns: zero
 */
A_INT32 Qc98xxTempSlopeSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	for (i=ix; i<WHAL_NUM_CHAINS; i++) {
		if (iv>=num)
			break;
		if (iBand==band_BG) {
			EEPROM_BASE_POINTER->freqModalHeader.tempSlope[i] = (A_UINT8)value[iv++];
		} else {
			EEPROM_BASE_POINTER->freqModalHeader.tempSlope[i] = (A_UINT8)value[iv++];
		}
	}
    if (iBand==band_BG) {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.tempSlope[ix], iv);
    } else {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.tempSlope[ix], iv);
    }
    return 0;
}
#endif //0
/*
 *Function Name:Qc98xxVoltSlopeSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set voltSlope flag in field of eeprom struct (A_INT8)
 *Returns: zero
 */
A_INT32 Qc98xxVoltSlopeSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	for (i=ix; i<WHAL_NUM_CHAINS; i++) {
		if (iv>=num)
			break;
		if (iBand==band_BG) {
			EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].voltSlope[i] = (A_INT8)value[iv++];
		} else {
			EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].voltSlope[i] = (A_INT8)value[iv++];
		}
	}
    if (iBand==band_BG) {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(biModalHeader[BIMODAL_2G_INDX].voltSlope[ix], iv);
    } else {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(biModalHeader[BIMODAL_5G_INDX].voltSlope[ix], iv);
    }
    return 0;
}

A_INT32 Qc98xxMiscConfigurationSet(int value)
{
    EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.miscConfiguration = (unsigned char)value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.miscConfiguration);
	return 0;
}
/*
 *Function Name:Qc98xxReconfigDriveStrengthSet
 *
 *Parameters: value
 *
 *Description: set reconfigDriveStrength flag in miscConfiguration 
 *             field of eeprom struct (bit 0)
 *
 *Returns: zero
 *
 */
A_INT32 Qc98xxReconfigDriveStrengthSet(int value)
{
    if (value) {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.miscConfiguration |= WHAL_MISCCONFIG_DRIVE_STRENGTH;
    }
    else {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.miscConfiguration &= 0xfe;
    }
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.miscConfiguration);
    return 0;
}

A_INT32 Qc98xxThermometerSet(int value)
{
	A_UINT8 misc;
	value++;
    misc=EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.miscConfiguration;
	misc&=(~WHAL_MISCCONFIG_FORCE_THERM_MASK);
	misc|=((value&0x3)<<1);
    EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.miscConfiguration=misc;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.miscConfiguration);
    return 0;
}

A_INT32 Qc98xxChainMaskReduceSet(int value)
{
	unsigned int misc;
    misc = EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.miscConfiguration;
	misc &= (~WHAL_MISCCONFIG_CHAIN_MASK_REDUCE);
	misc |= ((value&0x1)<<3);
    EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.miscConfiguration = misc;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.miscConfiguration);
    return 0;
}

// bit 4 - enable quick drop
A_INT32 Qc98xxReconfigQuickDropSet(int value)		
{
    if (value)
    {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.miscConfiguration |= WHAL_MISCCONFIG_QUICK_DROP;
    }
    else
    {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.miscConfiguration &= (~0x10);
    }
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.miscConfiguration);
    return 0;
}

A_INT32 Qc98xxWlanLedGpioSet(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.wlanLedGpio = (A_UINT8)value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.wlanLedGpio);
    return 0;
}

A_INT32 Qc98xxSpurBaseASet(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.spurBaseA = (A_UINT8)value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.spurBaseA);
    return 0;
}
A_INT32 Qc98xxSpurBaseBSet(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.spurBaseB = (A_UINT8)value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.spurBaseB);
    return 0;
}

A_INT32 Qc98xxSpurRssiThreshSet(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.spurRssiThresh = (A_UINT8)value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.spurRssiThresh);
    return 0;
}

A_INT32 Qc98xxSpurRssiThreshCckSet(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.spurRssiThreshCck = (A_UINT8)value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.spurRssiThreshCck);
    return 0;
}

A_INT32 Qc98xxSpurMitFlagSet(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.spurMitFlag = (A_UINT8)value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.spurMitFlag);
    return 0;
}

A_INT32 Qc98xxTxRxGainSet(int value, int iBand)
{
    if (EEPROM_BASE_POINTER->baseEepHeader.eeprom_version == QC98XX_EEP_VER1)
    {
	    EEPROM_BASE_POINTER->baseEepHeader.txrxgain = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.txrxgain);
    }
    else
    {
        if (iBand == band_both || iBand == band_BG)
        {
            EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].txrxgain = (A_UINT8)value;
            EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].txrxgain);
        }
        if (iBand == band_both || iBand == band_A)
        {
            EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].txrxgain = (A_UINT8)value;
            EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].txrxgain);
        }
    }
	ResetForce();
    return 0;
}

/*
 *Function Name:Qc98xxTxGainSet
 *Parameters: value
 *Description: set TxGain flag in txrxgain field of eeprom struct's upper 4bits
 *Returns: zero
 */
A_INT32 Qc98xxTxGainSet(int value, int iBand)
{
	A_UINT8  value4;
	value4 = (A_UINT8)((value & 0x0f) << 4);

    if (EEPROM_BASE_POINTER->baseEepHeader.eeprom_version == QC98XX_EEP_VER1)
    {
	    EEPROM_BASE_POINTER->baseEepHeader.txrxgain &= (0x0f);
	    EEPROM_BASE_POINTER->baseEepHeader.txrxgain |= value4;
        EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.txrxgain);
    }
    else 
    {
        if (iBand == band_both || iBand == band_BG)
        {
            EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].txrxgain &= (0x0f);
            EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].txrxgain |= value4;
            EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].txrxgain);
        }
        if (iBand == band_both || iBand == band_A)
        {
            EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].txrxgain &= (0x0f);
            EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].txrxgain |= value4;
            EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].txrxgain);
        }
    }
	ResetForce();
    return 0;
}
/*
 *Function Name:Qc98xxRxGainSet
 *Parameters: value
 *Description: set RxGain flag in txrxgain field of eeprom struct's lower 4bits
 *Returns: zero
 */
A_INT32 Qc98xxRxGainSet(int value, int iBand)
{
	A_UINT8  value4;
	value4 = (A_UINT8)(value & 0x0f);

    if (EEPROM_BASE_POINTER->baseEepHeader.eeprom_version == QC98XX_EEP_VER1)
    {
        EEPROM_BASE_POINTER->baseEepHeader.txrxgain &= (0xf0);
        EEPROM_BASE_POINTER->baseEepHeader.txrxgain |= value4;
        EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.txrxgain);
    }
    else // v2
    {
        if (iBand == band_both || iBand == band_BG)
        {
            EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].txrxgain &= (0xf0);
            EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].txrxgain |= value4;
            EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].txrxgain);
        }
        if (iBand == band_both || iBand == band_A)
        {
            EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].txrxgain &= (0xf0);
            EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].txrxgain |= value4;
            EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].txrxgain);
        }
    }
	ResetForce();
    return 0;
}

A_INT32 Qc98xxEnableFeatureSet(int value)
{
    EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.featureEnable = (unsigned char)value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.featureEnable);
	return 0;
}

/*
 *Function Name:Qc98xxEnableTempCompensationSet
 *
 *Parameters: value
 *
 *Description: set reconfigDriveStrength flag in featureEnable 
 *             field of eeprom struct (bit 0)
 *
 *Returns: zero
 *
 */
A_INT32 Qc98xxEnableTempCompensationSet(int value)
{
    if (value) {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.featureEnable |= WHAL_FEATUREENABLE_TEMP_COMP_MASK;
    }
    else {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.featureEnable &= (~WHAL_FEATUREENABLE_TEMP_COMP_MASK);
    }
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.featureEnable);
    return 0;
}

/*
 *Function Name:Qc98xxEnableVoltCompensationSet
 *
 *Parameters: value
 *
 *Description: set reconfigDriveStrength flag in featureEnable 
 *             field of eeprom struct (bit 1)
 *
 *Returns: zero
 *
 */
A_INT32 Qc98xxEnableVoltCompensationSet(int value)
{
    if (value) {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.featureEnable |= WHAL_FEATUREENABLE_VOLT_COMP_MASK;
    }
    else {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.featureEnable &= (~WHAL_FEATUREENABLE_VOLT_COMP_MASK);
    }
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.featureEnable);
    return 0;
}

/*
 *Function Name:Qc98xxMacAddressSet
 *
 *Parameters: mac -- pointer to input mac address. 
 *
 *Description: Saves mac address in the eeprom structure.
 *
 *Returns: zero.
 *
 */

A_INT32 Qc98xxMacAddressSet(A_UINT8 *mac)
{
    A_INT16 i;
    for(i=0; i<6; i++)
    {
		EEPROM_BASE_POINTER->baseEepHeader.macAddr[i] = mac[i];
    }
    EEPROM_CONFIG_DIFF_CHANGE_ARRAY(baseEepHeader.macAddr[0], 6);
    return 0;
}

A_INT32 Qc98xxDeltaCck20Set(int value)
{
    EEPROM_BASE_POINTER->baseEepHeader.deltaCck20_t10 = value * 10;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.deltaCck20_t10);
    return 0;
}

A_INT32 Qc98xxDelta4020Set(int value)
{
    EEPROM_BASE_POINTER->baseEepHeader.delta4020_t10 = value * 10;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.delta4020_t10);
    return 0;
}

A_INT32 Qc98xxDelta8020Set(int value)
{
    EEPROM_BASE_POINTER->baseEepHeader.delta8020_t10 = value * 10;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.delta8020_t10);
    return 0;
}

/*
 *Function Name:Qc98xxCustomerDataSet
 *
 *Parameters: data -- Pointer to input array. 
 *            len -- length of the array. 
 *
 *Description: Saves input array in the customer data array of eeprom structure.
 *
 *Returns: -1 on error condition
 *          0 on success.
 */

A_INT32 Qc98xxCustomerDataSet(unsigned char *data, int len)
{
    A_INT16 i;

    if(len>CUSTOMER_DATA_SIZE) {
        len=CUSTOMER_DATA_SIZE;
    }

    for(i=0; i<len; i++)
    {
        EEPROM_BASE_POINTER->baseEepHeader.custData[i] = data[i];
    }
	// fill the rest with blank
	for (i = len; i < CUSTOMER_DATA_SIZE; ++i)
	{
        EEPROM_BASE_POINTER->baseEepHeader.custData[i] = 0;
	}

    EEPROM_CONFIG_DIFF_CHANGE_ARRAY(baseEepHeader.custData[0], len);
    return 0;
}

/*
 *Function Name:Qc98xxPwrTuningCapsParamsSet
 *Parameters: value0, value1
 *Description: Set TuningCapsParams values of field of eeprom struct 2 uint8
 *Returns: zero
 */
A_INT32 Qc98xxPwrTuningCapsParamsSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	
    for (i=ix; i<2; i++) 
    {
		if (iv>=num)
			break;
        if (i == 0)
        {
	        EEPROM_BASE_POINTER->baseEepHeader.param_for_tuning_caps = (A_UINT8)value[iv++];
            EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.param_for_tuning_caps);
        }
        else //(i==1)
        {
	        EEPROM_BASE_POINTER->baseEepHeader.param_for_tuning_caps_1 = (A_UINT8)value[iv++];
            EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.param_for_tuning_caps_1);
        }
	}
	ResetForce();
	return 0;
}

/*
 *Function Name:Qc98xxRegDmnSet
 *Parameters: value
 *Description: set regDmn field of eeprom struct (A_UINT16 *2) 
 *Returns: zero
 */
A_INT32 Qc98xxRegDmnSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	for (i=ix; i<2; i++) {
		if (iv>=num)
			break;
		EEPROM_BASE_POINTER->baseEepHeader.regDmn[i] = (A_UINT16)value[iv++];
	}
    EEPROM_CONFIG_DIFF_CHANGE_ARRAY(baseEepHeader.regDmn[ix], iv);
    return 0;
}

/*
 *Function Name:Qc98xxTxMaskSet
 *Parameters: value
 *Description: set txrxMask field of eeprom struct (A_UINT8) 4bit tx (upper 4)
 *Returns: zero
 */
A_INT32 Qc98xxTxMaskSet(int value)
{
	A_UINT8  value4;
	value4 = (A_UINT8)(value << 4);
	EEPROM_BASE_POINTER->baseEepHeader.txrxMask &= (0x0f);
	EEPROM_BASE_POINTER->baseEepHeader.txrxMask += value4;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.txrxMask);
    return 0;
}
/*
 *Function Name:Qc98xxRxMaskSet
 *Parameters: value
*Description: set txrxMask field of eeprom struct (A_UINT8) 4bit tx (lower 4)
 *Returns: zero
 */
A_INT32 Qc98xxRxMaskSet(int value)
{
	A_UINT8  value4;

	value4 = (A_UINT8)(value & 0x0f);
	EEPROM_BASE_POINTER->baseEepHeader.txrxMask &= (0xf0);
	EEPROM_BASE_POINTER->baseEepHeader.txrxMask += value4;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.txrxMask);
    return 0;
}
/*
 *Function Name:Qc98xxOpFlagsSet
 *Parameters: value
 *Description: set opFlags field of eeprom struct (A_UINT8)
 *Returns: zero
 */
A_INT32 Qc98xxOpFlagsSet(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.opFlags = value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.opFlags);
    return 0;
}

/*
 *Function Name:Qc98xxOpFlags2Set
 *Parameters: value
 *Description: set opFlags field of eeprom struct (A_UINT8)
 *Returns: zero
 */
A_INT32 Qc98xxOpFlags2Set(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.opFlags2 = value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.opFlags2);
    return 0;
}

/*
 *Function Name:Qc98xxEepMiscSet
 *Parameters: value
 *Description: set eepMisc field of eeprom struct (A_UINT8)
 *Returns: zero
 */
A_INT32 Qc98xxEepMiscSet(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.miscConfiguration = (A_UINT8)((value << WHAL_MISCCONFIG_EEPMISC_SHIFT) & WHAL_MISCCONFIG_EEPMISC_MASK);
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.miscConfiguration);
    return 0;
}
/*
 *Function Name:Qc98xxRfSilentSet
 *Parameters: value
 *Description: set rfSilent field of eeprom struct (A_UINT8)
 *Returns: zero
 */
A_INT32 Qc98xxRfSilentSet(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.rfSilent = (A_UINT8) value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.rfSilent);
    return 0;
}

A_INT32 Qc98xxRfSilentB0Set(int value)
{
	if (value)
    {
		EEPROM_BASE_POINTER->baseEepHeader.rfSilent |= 0x1;
    }
    else
    {
        EEPROM_BASE_POINTER->baseEepHeader.rfSilent &= (~0x1);
    }
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.rfSilent);
    return 0;
}

A_INT32 Qc98xxRfSilentB1Set(int value)
{
	if (value)
    {
		EEPROM_BASE_POINTER->baseEepHeader.rfSilent |= 0x2;
    }
    else
    {
        EEPROM_BASE_POINTER->baseEepHeader.rfSilent &= (~0x2);
    }
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.rfSilent);
    return 0;
}

A_INT32 Qc98xxRfSilentGPIOSet(int value)
{
	//clear out the field before setting
	EEPROM_BASE_POINTER->baseEepHeader.rfSilent &= (~0xfc);
	if (value)
    {
		//set the field
		EEPROM_BASE_POINTER->baseEepHeader.rfSilent |= (value << 2);
    }
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.rfSilent);
    return 0;
}

/*
 *Function Name:Qc98xxDeviceCapSet
 *Parameters: value
 *Description: set deviceCap field of eeprom struct (A_UINT8)
 *Returns: zero
 */
A_INT32 Qc98xxDeviceCapSet(int value)
{
	//EEPROM_BASE_POINTER->baseEepHeader.deviceCap = (A_UINT8) value;
    //EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.deviceCap);
    return 0;
}

/*
 *Function Name:Qc98xxBlueToothOptionsSet
 *Parameters: value
 *Description: set blueToothOptions field of eeprom struct (A_UINT8)
 *Returns: zero
 */
A_INT32 Qc98xxBlueToothOptionsSet(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.blueToothOptions = (A_UINT8) value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.blueToothOptions);
    return 0;
}

#if 0
/*
 *Function Name:Qc98xxDeviceTypetSet
 *Parameters: value
 *Description: set deviceType field of eeprom struct (A_UINT8) (lower byte in EEP)
 *Returns: zero
 */
A_INT32 Qc98xxDeviceTypetSet(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.deviceType = (A_UINT8) value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.deviceType);
    return 0;
}
#endif //0

/*
 *Function Name:Qc98xxPwrTableOffsetSet
 *Parameters: value
 *Description: set pwrTableOffset field of eeprom struct (A_INT8)
 *Returns: zero
 */
A_INT32 Qc98xxPwrTableOffsetSet(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.pwrTableOffset = (A_INT8) value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.pwrTableOffset);
    return 0;
}

#if 0
/*
 *Function Name:Qc98xxEnableFastClockSet
 *
 *Parameters: value
 *
 *Description: set Fast Clock flag in featureEnable 
 *             field of eeprom struct (bit 2)
 *
 *Returns: zero
 *
 */
A_INT32 Qc98xxEnableFastClockSet(int value)
{
    if (value) {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.featureEnable |= 0x04;
    }
    else {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.featureEnable &= 0xfb;
    }
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.featureEnable);
    return 0;
}
#endif //0

/*
 *Function Name:Qc98xxEnableDoublingSet
 *
 *Parameters: value
 *
 *Description: set reconfigDriveStrength flag in featureEnable 
 *             field of eeprom struct (bit 3)
 *
 *Returns: zero
 *
 */
A_INT32 Qc98xxEnableDoublingSet(int value)
{
    if (value) {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.featureEnable |= WHAL_FEATUREENABLE_DOUBLING_MASK;
    }
    else {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.featureEnable &= (~WHAL_FEATUREENABLE_DOUBLING_MASK);
    }
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.featureEnable);
    return 0;
}

/*
 *Function Name:Qc98xxEnableTuningCapsSet
 *Parameters: value
 *Description: set EnableTuningCaps flag in featureEnable 
 *             field of eeprom struct (bit 3)
 *Returns: zero
 */
A_INT32 Qc98xxEnableTuningCapsSet(int value)
{
    if (value)
    {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.featureEnable |= WHAL_FEATUREENABLE_TUNING_CAPS_MASK;
    }
    else
    {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.featureEnable &= (~WHAL_FEATUREENABLE_TUNING_CAPS_MASK);
    }
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.featureEnable);
    return 0;
}

/*
 *Function Name:Qc98xxInternalRegulatorSet
 *
 *Parameters: value
 *
 *Description: set internal regulator flag in featureEnable 
 *             field of eeprom struct (bit 4).
 *             Add an entry in PCIE config space
 *
 *Returns: zero
 *
 */
A_INT32 Qc98xxInternalRegulatorSet(int value)
{
	if (value) {
		// internal regulator is ON. This default setting.
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.featureEnable |= WHAL_FEATUREENABLE_INTERNAL_REGULATOR_MASK;  // set the bit
    } else {
		// Internal regulator is OFF. We should write 4 to 0x7048. This write is necessary for non-calibrated board.
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.featureEnable &= (~WHAL_FEATUREENABLE_INTERNAL_REGULATOR_MASK); // clear the bit
    }
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.featureEnable);
    return 0;
}

A_INT32 Qc98xxBoardFlagsSet(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.boardFlags = value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.boardFlags);
    return 0;
}

/*
 *Function Name:Qc98xxPapdSet
 *
 *Parameters: value
 *
 *Description: set internal regulator flag in featureEnable 
 *             field of eeprom struct (bit 4).
 *             Add an entry in PCIE config space
 *
 *Returns: zero
 *
 */
#ifndef WHAL_BOARD_PAPRD_2G_DISABLE
#define WHAL_BOARD_USE_OTP_XTALCAPS	0x040
#define WHAL_BOARD_PAPRD_2G_DISABLE 0x2000
#define WHAL_BOARD_PAPRD_5G_DISABLE 0x4000
#endif  

A_INT32 Qc98xxRbiasSet(int value)
{
    if (value) {
        //enable RBias 
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.boardFlags |= WHAL_BOARD_USE_OTP_XTALCAPS;
    } else {
        //disable RBias
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.boardFlags &= (~WHAL_BOARD_USE_OTP_XTALCAPS);
    }
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.boardFlags);
    return 0;
}

A_INT32 Qc98xxBibxosc0Set(int value)
{
    if (value) {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.flag1 |= WHAL_BOARD_FLAG1_BIBXOSC0_MASK;
    } else {
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.flag1 &= (~WHAL_BOARD_FLAG1_BIBXOSC0_MASK);
    }
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.flag1);
    return 0;
}

A_INT32 Qc98xxFlag1NoiseFlrThrSet(int value)
{
	if (Qc98xxEepromStructGet()->baseEepHeader.eeprom_version >= QC98XX_EEP_VER3)
	{
	    if (value) {
		    EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.flag1 |= WHAL_BOARD_FLAG1_NOISE_FLOOR_THR_MASK;
		} else {
			EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.flag1 &= (~WHAL_BOARD_FLAG1_NOISE_FLOOR_THR_MASK);
		}
		EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.flag1);
		return VALUE_OK;
	}
	return ERR_NOT_FOUND;
}

A_INT32 Qc98xxPapdSet(int value)
{
    if (value) {
        //enable PAPRD 
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.boardFlags &= 
            (~(WHAL_BOARD_PAPRD_2G_DISABLE | WHAL_BOARD_PAPRD_5G_DISABLE)); 
    } else {
        //disable PAPRD
        EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.boardFlags |= 
            (WHAL_BOARD_PAPRD_2G_DISABLE | WHAL_BOARD_PAPRD_5G_DISABLE); 
    }
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.boardFlags);
    return 0;
}

#if 0
A_INT32 Qc98xxTempSlopeLowSet(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.temp_slope_low = (A_INT8)value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.temp_slope_low);
    return 0;
}

A_INT32 Qc98xxTempSlopeHighSet(int value)
{
	EEPROM_BASE_POINTER->baseEepHeader.temp_slope_high = (A_INT8)value;
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.temp_slope_high);
    return 0;
}
#endif //0

A_INT32 Qc98xxSWREGSet(int value)
{
    EEPROM_BASE_POINTER->baseEepHeader.swreg = (A_UINT8)value; 
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.swreg);
    return 0;
}

A_INT32 Qc98xxSWREGProgramSet(int value)
{
	int status=VALUE_OK;
	unsigned int address, swregAddr;
	int low, high;
	int ngot;
	char regName[100];
	A_UINT32 reg;

	if (value==1) {
		ngot=FieldFind("REG_CONTROL0.SWREG_BITS",&swregAddr,&low,&high);
		if (ngot<1)
			return ERR_VALUE_BAD;

		sprintf(regName, "REG_CONTROL1.SWREG_PROGRAM"); 
		ngot=FieldFind(regName,&address,&low,&high);
		if (ngot==1) {
			// disable internal regulator program write.
			MyFieldWrite(address,low,high,0);	
			// set swreg from eep structure to HW
			reg = EEPROM_BASE_POINTER->baseEepHeader.swreg; 
			Qc9KRegisterWrite(swregAddr, reg);
			// apply internal regulator program write.
			MyFieldWrite(address,low,high,1);	
		} else 
			status = ERR_VALUE_BAD;
	}
    return status;
}

A_INT32 Qc98xxAntCtrlChainSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	for (i=ix; i<WHAL_NUM_CHAINS; i++) {
		if (iv>=num)
			break;
		if (iBand==band_BG) {
			EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].antCtrlChain[i] = (A_UINT16)value[iv++];
		} else {
			EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].antCtrlChain[i] = (A_UINT16)value[iv++];
		}
	}
    if (iBand==band_BG) {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(biModalHeader[BIMODAL_2G_INDX].antCtrlChain[ix], iv);
    } else {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(biModalHeader[BIMODAL_5G_INDX].antCtrlChain[ix], iv);
    }
    return 0;
}

A_INT32 Qc98xxNoiseFlrThrSet(int value, int iBand)
{	
	if (Qc98xxEepromStructGet()->baseEepHeader.eeprom_version >= QC98XX_EEP_VER3)
	{
		if (iBand==band_BG) 
		{
			EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].noiseFlrThr = (A_UINT8)value;
			EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].noiseFlrThr);
		} 
		else 
		{
			EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].noiseFlrThr = (A_UINT8)value;
			EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].noiseFlrThr);
		}
		return VALUE_OK;
	}
	return ERR_NOT_FOUND;
}

A_INT32 Qc98xxMinCcaPwrChainSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	if (Qc98xxEepromStructGet()->baseEepHeader.eeprom_version >= QC98XX_EEP_VER3)
	{
		for (i=ix; i<WHAL_NUM_CHAINS; i++) {
			if (iv>=num)
				break;
			if (iBand==band_BG) {
				EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].minCcaPwr[i] = (A_INT8)value[iv++];
			} else {
				EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].minCcaPwr[i] = (A_INT8)value[iv++];
			}
		}
		if (iBand==band_BG) {
			EEPROM_CONFIG_DIFF_CHANGE_ARRAY(biModalHeader[BIMODAL_2G_INDX].minCcaPwr[ix], iv);
		} else {
			EEPROM_CONFIG_DIFF_CHANGE_ARRAY(biModalHeader[BIMODAL_5G_INDX].minCcaPwr[ix], iv);
		}
		return VALUE_OK;
	}

	return ERR_NOT_FOUND;
}

/*
 *Function Name:Qc98xxXatten1DBSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set xatten1DB flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_UINT8) 
 *Returns: zero
 */
A_INT32 Qc98xxXatten1DBSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	for (i=ix; i<WHAL_NUM_CHAINS; i++) {
		if (iv>=num)
			break;
		if (iBand==band_BG) {
			EEPROM_BASE_POINTER->freqModalHeader.xatten1DB[i].value2G = (A_UINT8)value[iv++];
		} else {
			EEPROM_BASE_POINTER->freqModalHeader.xatten1DB[i].value5GMid = (A_UINT8)value[iv++];
		}
	}
    if (iBand==band_BG) {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten1DB[ix].value2G, iv);
    } else {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten1DB[ix].value5GMid, iv);
    }
	ResetForce();
    return 0;
}

/*
 *Function Name:Qc98xxXatten1MarginSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set xatten1Margin flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_UINT8) 
 *Returns: zero
 */
A_INT32 Qc98xxXatten1MarginSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	for (i=ix; i<WHAL_NUM_CHAINS; i++) {
		if (iv>=num)
			break;
		if (iBand==band_BG) {
			EEPROM_BASE_POINTER->freqModalHeader.xatten1Margin[i].value2G = (A_UINT8)value[iv++];
		} else {
			EEPROM_BASE_POINTER->freqModalHeader.xatten1Margin[i].value5GMid = (A_UINT8)value[iv++];
		}
	}
    if (iBand==band_BG) {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten1Margin[ix].value2G, iv);
    } else {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten1Margin[ix].value5GMid, iv);
    }
	ResetForce();
    return 0;
}

#if 0
/*
 *Function Name:Qc98xxXatten1HystSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set xatten1Hyst flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_UINT8) 
 *Returns: zero
 */
A_INT32 Qc98xxXatten1HystSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	for (i=ix; i<WHAL_NUM_CHAINS; i++) {
		if (iv>=num)
			break;
		if (iBand==band_BG) {
			EEPROM_BASE_POINTER->freqModalHeader.xatten1Hyst[i] = (A_UINT8)value[iv++];
		} else {
			EEPROM_BASE_POINTER->freqModalHeader.xatten1Hyst[i] = (A_UINT8)value[iv++];
		}
	}
    if (iBand==band_BG) {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten1Hyst[ix], iv);
    } else {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten1Hyst[ix], iv);
    }
	ResetForce();
    return 0;
}

/*
 *Function Name:Qc98xxXatten2DBSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set xatten2DB flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_UINT8) 
 *Returns: zero
 */
A_INT32 Qc98xxXatten2DBSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	for (i=ix; i<WHAL_NUM_CHAINS; i++) {
		if (iv>=num)
			break;
		if (iBand==band_BG) {
			EEPROM_BASE_POINTER->freqModalHeader.xatten2Db[i] = (A_UINT8)value[iv++];
		} else {
			EEPROM_BASE_POINTER->freqModalHeader.xatten2Db[i] = (A_UINT8)value[iv++];
		}
	}
    if (iBand==band_BG) {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten2Db[ix], iv);
    } else {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten2Db[ix], iv);
    }
	ResetForce();
    return 0;
}

/*
 *Function Name:Qc98xxXatten2MarginSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set xatten2Margin flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_UINT8) 
 *Returns: zero
 */
A_INT32 Qc98xxXatten2MarginSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	for (i=ix; i<WHAL_NUM_CHAINS; i++) {
		if (iv>=num)
			break;
		if (iBand==band_BG) {
			EEPROM_BASE_POINTER->freqModalHeader.xatten2Margin[i] = (A_UINT8)value[iv++];
		} else {
			EEPROM_BASE_POINTER->freqModalHeader.xatten2Margin[i] = (A_UINT8)value[iv++];
		}
	}
    if (iBand==band_BG) {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten2Margin[ix], iv);
    } else {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten2Margin[ix], iv);
    }
	ResetForce();
    return 0;
}

/*
 *Function Name:Qc98xxXatten2HystSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set xatten2Hyst flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_UINT8) 
 *Returns: zero
 */
A_INT32 Qc98xxXatten2HystSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	for (i=ix; i<WHAL_NUM_CHAINS; i++) {
		if (iv>=num)
			break;
		if (iBand==band_BG) {
			EEPROM_BASE_POINTER->freqModalHeader.xatten2Hyst[i] = (A_UINT8)value[iv++];
		} else {
			EEPROM_BASE_POINTER->freqModalHeader.xatten2Hyst[i] = (A_UINT8)value[iv++];
		}
	}
    if (iBand==band_BG) {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten2Hyst[ix], iv);
    } else {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten2Hyst[ix], iv);
    }
	ResetForce();
    return 0;
}
#endif //0
/*
 *Function Name:Qc98xxXatten1DBLowSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set xatten1DBLow flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_UINT8) 
 *Returns: zero
 */
A_INT32 Qc98xxXatten1DBLowSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	if (iBand==band_BG)
		return 0;
	for (i=ix; i<WHAL_NUM_CHAINS; i++) {
		if (iv>=num)
			break;
		EEPROM_BASE_POINTER->freqModalHeader.xatten1DB[i].value5GLow = (A_UINT8)value[iv++];
	}
    EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten1DB[ix].value5GLow, iv);
	ResetForce();
    return 0;
}
/*
 *Function Name:Qc98xxXatten1DBHighSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set xatten1DBHigh flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_UINT8) 
 *Returns: zero
 */
A_INT32 Qc98xxXatten1DBHighSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	if (iBand==band_BG)
		return 0;
	for (i=ix; i<WHAL_NUM_CHAINS; i++) {
		if (iv>=num)
			break;
		EEPROM_BASE_POINTER->freqModalHeader.xatten1DB[i].value5GHigh = (A_UINT8)value[iv++];
	}
    EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten1DB[ix].value5GHigh, iv);
	ResetForce();
    return 0;
}

/*
 *Function Name:Qc98xxXatten1MarginLowSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set xatten1MarginLow flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_UINT8) 
 *Returns: zero
 */
A_INT32 Qc98xxXatten1MarginLowSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	if (iBand==band_BG)
		return 0;
	for (i=ix; i<WHAL_NUM_CHAINS; i++) {
		if (iv>=num)
			break;
		EEPROM_BASE_POINTER->freqModalHeader.xatten1Margin[i].value5GLow = (A_UINT8)value[iv++];
	}
    EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten1Margin[ix].value5GLow, iv);
	ResetForce();
    return 0;
}

/*
 *Function Name:Qc98xxXatten1MarginHighSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set xatten1MarginHigh flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_UINT8) 
 *Returns: zero
 */
A_INT32 Qc98xxXatten1MarginHighSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	if (iBand==band_BG)
		return 0;
	for (i=ix; i<WHAL_NUM_CHAINS; i++) {
		if (iv>=num)
			break;
		EEPROM_BASE_POINTER->freqModalHeader.xatten1Margin[i].value5GHigh = (A_UINT8)value[iv++];
	}
    EEPROM_CONFIG_DIFF_CHANGE_ARRAY(freqModalHeader.xatten1Margin[ix].value5GHigh, iv);
	ResetForce();
    return 0;
}


/*
 *Function Name:Qc98xxSpurChansSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set spurChans flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_UINT8) 
 *Returns: zero
 */
A_INT32 Qc98xxSpurChansSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8 bin;
	int i, iv=0;
	for (i=ix; i<QC98XX_EEPROM_MODAL_SPURS; i++) {
		if (iv>=num)
			break;
		bin = setFREQ2FBIN(value[iv++], iBand);
		if (iBand==band_BG)
        {
			EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].spurChans[i].spurChan = bin;
            EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].spurChans[i].spurChan);
        }
		else
        {
			EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].spurChans[i].spurChan = bin;
            EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].spurChans[i].spurChan);
        }
	}
	return VALUE_OK;
}

A_INT32 Qc98xxSpurAPrimSecChooseSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8 bin;
	int i, iv=0;
	for (i=ix; i<QC98XX_EEPROM_MODAL_SPURS; i++) {
		if (iv>=num)
			break;
		bin = setFREQ2FBIN(value[iv++], iBand);
		if (iBand==band_BG)
        {
			EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].spurChans[i].spurA_PrimSecChoose = bin;
            EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].spurChans[i].spurA_PrimSecChoose);
        }
		else
        {
			EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].spurChans[i].spurA_PrimSecChoose = bin;
            EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].spurChans[i].spurA_PrimSecChoose);
        }
	}
	return VALUE_OK;
}

A_INT32 Qc98xxSpurBPrimSecChooseSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8 bin;
	int i, iv=0;
	for (i=ix; i<QC98XX_EEPROM_MODAL_SPURS; i++) {
		if (iv>=num)
			break;
		bin = setFREQ2FBIN(value[iv++], iBand);
		if (iBand==band_BG)
        {
			EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].spurChans[i].spurB_PrimSecChoose = bin;
            EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].spurChans[i].spurB_PrimSecChoose);
        }
		else
        {
			EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].spurChans[i].spurB_PrimSecChoose = bin;
            EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].spurChans[i].spurB_PrimSecChoose);
        }
	}
	return VALUE_OK;
}

#if 0
/*
 *Function Name:Qc98xxNoiseFloorThreshChSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set noiseFloorThreshCh flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_INT8) 
 *Returns: zero
 */
A_INT32 Qc98xxNoiseFloorThreshChSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;
	for (i=ix; i<WHAL_NUM_CHAINS; i++) {
		if (iv>=num)
			break;
		if (iBand==band_BG) 
        {
			EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].noiseFloorThreshCh[i] = (A_INT8)value[iv++];
            EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].noiseFloorThreshCh[i]);
		}
        else 
        {
			EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].noiseFloorThreshCh[i] = (A_INT8)value[iv++];
            EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].noiseFloorThreshCh[i]);
		}
	}
    return 0;
}

/*
 *Function Name:Qc98xxObSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set ob flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_UINT8) 
 *Returns: zero
 */
A_INT32 Qc98xxObSet(char *tValue, int *value, int iBand)
{
	if (iBand==band_BG)
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].ob_db[0].ob_dbValue = (A_UINT16)value[0];
        EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].ob_db[1].ob_dbValue = (A_UINT16)value[1];
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].ob_db[0].ob_dbValue);
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].ob_db[1].ob_dbValue);
	}
    else
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].ob_db[0].ob_dbValue = (A_UINT16)value[0];
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].ob_db[1].ob_dbValue = (A_UINT16)value[1];
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].ob_db[0].ob_dbValue);
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].ob_db[1].ob_dbValue);
	}
    return 0;
}
#endif //0
/*
 *Function Name:Qc98xxXpaBiasLvlSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set xpaBiasLvl flag in field of eeprom struct (A_UINT8)
 *Returns: zero
 */

A_INT32 Qc98xxXpaBiasLvlSet(int value, int iBand)
{
    A_UINT8 newValue;
    if (iBand==band_BG) 
    {
        newValue = (~0xF)&(EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].xpaBiasLvl);
        newValue = newValue | (0xF&value);
        EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].xpaBiasLvl = newValue;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].xpaBiasLvl);
    } 
    else 
    {
        newValue = (~0xF)&(EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].xpaBiasLvl);
        newValue = newValue | (0xF&value);
        EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].xpaBiasLvl = newValue;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].xpaBiasLvl);
    }
    return 0;
}

A_INT32 Qc98xxXpaBiasBypassSet(int value, int iBand)
{
    int newValue;
    if (iBand==band_BG) 
    {
        newValue = (~0x10)&(EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].xpaBiasLvl);
        newValue = newValue |((value<<4)&0x10);
        EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].xpaBiasLvl = newValue;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].xpaBiasLvl);
    } 
    else 
    {
        newValue = (~0x10)&(EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].xpaBiasLvl);
        newValue = newValue |((value<<4)&0x10);
        EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].xpaBiasLvl = newValue;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].xpaBiasLvl);
    }
    return 0;
}

#if 0
/*
 *Function Name:Qc98xxTxFrameToDataStartSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set txFrameToDataStart flag in field of eeprom struct (A_UINT8)
 *Returns: zero
 */
A_INT32 Qc98xxTxFrameToDataStartSet(int value, int iBand)
{
    if (iBand==band_BG) 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].txFrameToDataStart = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].txFrameToDataStart);
	} 
    else 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].txFrameToDataStart = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].txFrameToDataStart);
	}
    return 0;
}
/*
 *Function Name:Qc98xxTxFrameToPaOnSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set txFrameToPaOn flag in field of eeprom struct (A_UINT8)
 *Returns: zero
 */
A_INT32 Qc98xxTxFrameToPaOnSet(int value, int iBand)
{
    if (iBand==band_BG) 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].txFrameToPaOn = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].txFrameToPaOn);
	} 
    else 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].txFrameToPaOn = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].txFrameToPaOn);
	}
    return 0;
}

/*
 *Function Name:Qc98xxTxClipSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set txClip flag in field of eeprom struct (A_UINT8) (4 bits tx_clip)
 *Returns: zero
 */
A_INT32 Qc98xxTxClipSet(int value, int iBand)
{
	int value4 = (A_UINT8)(value & 0x0f);
	
    if (iBand==band_BG) 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].txClip &= (0xf0);				// which 4 bits are for tx_clip???
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].txClip += value4;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].txClip);
	} 
    else 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].txClip &= (0xf0);
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].txClip += value4;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].txClip);
	}
    return 0;
}
/*
 *Function Name:Qc98xxDacScaleCckSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set dac_scale_cck flag in field of eeprom struct (A_UINT8) (4 bits tx_clip)
 *Returns: zero
 */
A_INT32 Qc98xxDacScaleCckSet(int value, int iBand)
{
	int value4 = (A_UINT8)(value << 4);
	
    if (iBand==band_BG) 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].txClip &= (0x0f);				// which 4 bits are for tx_clip???
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].txClip += value4;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].txClip);
	} 
    else 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].txClip &= (0x0f);
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].txClip += value4;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].txClip);
	}
    return 0;
}
#endif //0
/*
 *Function Name:Qc98xxAntennaGainSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set antennaGain flag in field of eeprom struct (A_INT8)
 *Returns: zero
 */
A_INT32 Qc98xxAntennaGainSet(int value, int iBand)
{
    if (iBand==band_BG) 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].antennaGainCh = (A_INT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].antennaGainCh);
	} 
    else 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].antennaGainCh = (A_INT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].antennaGainCh);
	}
    return 0;
}
#if 0
/*
 *Function Name:Qc98xxAdcDesiredSizeSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set adcDesiredSize flag in field of eeprom struct (A_INT8)
 *Returns: zero
 */
A_INT32 Qc98xxAdcDesiredSizeSet(int value, int iBand)
{
    if (iBand==band_BG) 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].adcDesiredSize = (A_INT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].adcDesiredSize);
	} 
    else 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].adcDesiredSize = (A_INT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].adcDesiredSize);
	}
    return 0;
}
/*
 *Function Name:Qc98xxSwitchSettlingSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set switchSettling flag in field of eeprom struct (A_UINT8)
 *Returns: zero
 */
A_INT32 Qc98xxSwitchSettlingSet(int value, int iBand)
{
    if (iBand==band_BG) 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].switchSettling = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].switchSettling);
	} 
    else 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].switchSettling = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].switchSettling);
	}
    return 0;
}

/*
 *Function Name:Qc98xxSwitchSettlingHt40Set
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set switchSettling flag in field of eeprom struct (A_UINT8)
 *Returns: zero
 */
A_INT32 Qc98xxSwitchSettlingHt40Set(int value, int iBand)
{
    if (iBand==band_BG) 
    {
		//EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].swSettleHt40 = (A_UINT8)value;
        //EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].swSettleHt40);
	} 
    else 
    {
		//EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].swSettleHt40 = (A_UINT8)value;
        //EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].swSettleHt40);
	}
    return 0;
}

/*
 *Function Name:Qc98xxTxEndToXpaOffSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set txEndToXpaOff flag in field of eeprom struct (A_UINT8)
 *Returns: zero
 */
A_INT32 Qc98xxTxEndToXpaOffSet(int value, int iBand)
{
    if (iBand==band_BG) 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].txEndToXpaOff = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].txEndToXpaOff);
	} 
    else 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].txEndToXpaOff = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].txEndToXpaOff);
	}
    return 0;
}
/*
 *Function Name:Qc98xxTxEndToRxOnSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set txEndToRxOn flag in field of eeprom struct (A_UINT8)
 *Returns: zero
 */
A_INT32 Qc98xxTxEndToRxOnSet(int value, int iBand)
{
    if (iBand==band_BG) 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].txEndToRxOn = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].txEndToRxOn);
	} 
    else 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].txEndToRxOn = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].txEndToRxOn);
	}
    return 0;
}
/*
 *Function Name:Qc98xxTxFrameToXpaOnSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set txFrameToXpaOn flag in field of eeprom struct (A_UINT8)
 *Returns: zero
 */
A_INT32 Qc98xxTxFrameToXpaOnSet(int value, int iBand)
{
    if (iBand==band_BG) 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].txFrameToXpaOn = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].txFrameToXpaOn);
	} 
    else 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].txFrameToXpaOn = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].txFrameToXpaOn);
	}
    return 0;
}
/*
 *Function Name:Qc98xxThresh62Set
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA
 *Description: set thresh62 flag in field of eeprom struct (A_UINT8)
 *Returns: zero
 */
A_INT32 Qc98xxThresh62Set(int value, int iBand)
{
    if (iBand==band_BG)
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_2G_INDX].thresh62 = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_2G_INDX].thresh62);
	} 
    else 
    {
		EEPROM_BASE_POINTER->biModalHeader[BIMODAL_5G_INDX].thresh62 = (A_UINT8)value;
        EEPROM_CONFIG_DIFF_CHANGE(biModalHeader[BIMODAL_5G_INDX].thresh62);
	}
    return 0;
}
#endif //0

A_UINT32 Qc98xxThermAdcScaledGainSet(int value)
{
	EEPROM_BASE_POINTER->chipCalData.thermAdcScaledGain = (A_INT16) value;
    EEPROM_CONFIG_DIFF_CHANGE(chipCalData.thermAdcScaledGain);
    return 0;
}

A_UINT32 Qc98xxThermAdcOffsetSet(int value)
{
	EEPROM_BASE_POINTER->chipCalData.thermAdcOffset = (A_INT8) value;
    EEPROM_CONFIG_DIFF_CHANGE(chipCalData.thermAdcOffset);
    return 0;
}

/*
 *Function Name:Qc98xxCalFreqPierSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set calFreqPier flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_UINT8) 
 *Returns: zero
 */

A_INT32 Qc98xxCalFreqPierSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8 bin;
	int i, maxnum, iv=0;

    maxnum = (iBand==band_BG) ? WHAL_NUM_11G_CAL_PIERS : WHAL_NUM_11A_CAL_PIERS;
	for (i=ix; i<maxnum; i++) 
    {
		if (iv>=num)
			break;
		bin = setFREQ2FBIN(value[iv++], iBand);
		if (iBand==band_BG)
			EEPROM_BASE_POINTER->calFreqPier2G[i] = bin;
		else
			EEPROM_BASE_POINTER->calFreqPier5G[i] = bin;
	}
    if (iBand==band_BG)
    {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(calFreqPier2G[ix], iv);
    }
    else
    {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(calFreqPier5G[ix], iv);
    }
	return VALUE_OK;
}

A_INT32 Qc98xxCalPointTxGainIdxSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, j, k, iv=0;
	int iStart, iEnd, jStart, jEnd, kStart, kEnd;
	int iMax;
	
	iMax = (iBand==band_BG) ? WHAL_NUM_11G_CAL_PIERS : WHAL_NUM_11A_CAL_PIERS;

	iStart = ix; iEnd = ix + 1;
	if (ix < 0 )
	{
		iStart = 0; 
	}
      	if (ix >= iMax)
	{
		iEnd = iMax;
	}
	jStart = iy; jEnd = iy + 1;
	if (iy < 0)
	{
		jStart = 0; 
	}
	if(iy >= WHAL_NUM_CHAINS)
	{
		jEnd = WHAL_NUM_CHAINS;
	}
	kStart = iz; kEnd = iz + 1;
	if (iz < 0)
	{
		kStart = 0; 
	}
	if(iz >= WHAL_NUM_CAL_GAINS)
	{
		kEnd = WHAL_NUM_CAL_GAINS;
	}

    for (i = iStart; i < iEnd; i++) 
	{
		if (iv>=num)
		{
			break;
		}
		for (j = jStart; j < jEnd; j++) 
		{
			if (iv>=num)
			{
				break;
			}
			for (k = kStart; k < kEnd; k++)
			{
				if (iBand==band_BG)
				{
					EEPROM_BASE_POINTER->calPierData2G[i].calPerPoint[j].txgainIdx[k] = (A_UINT8)value[iv++];
					EEPROM_CONFIG_DIFF_CHANGE(calPierData2G[i].calPerPoint[j].txgainIdx[k]);
				}
				else
				{
					EEPROM_BASE_POINTER->calPierData5G[i].calPerPoint[j].txgainIdx[k] = (A_UINT8)value[iv++];
					EEPROM_CONFIG_DIFF_CHANGE(calPierData5G[i].calPerPoint[j].txgainIdx[k]);
				}
				if (iv>=num)
				{
					break;
				}
			}
		}
	}
    return VALUE_OK;
}

A_INT32 Qc98xxCalPointDacGainSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, j, iv=0;
	int iStart, iEnd, jStart, jEnd;
	int iMax;
	
	iMax = (iBand==band_BG) ? WHAL_NUM_11G_CAL_PIERS : WHAL_NUM_11A_CAL_PIERS;

	if (ix <= 0 || ix >= iMax)
	{
		iStart = 0; iEnd = iMax;
	}
	else
	{
		iStart = ix; iEnd = ix + 1;
	}
	if (iy <= 0 || iy >= WHAL_NUM_CAL_GAINS)
	{
		jStart = 0; jEnd = WHAL_NUM_CAL_GAINS;
	}
	else
	{
		jStart = iy; jEnd = iy + 1;
	}

    for (i = iStart; i < iEnd; i++) 
	{
		if (iv>=num)
		{
			break;
		}
		for (j = jStart; j < jEnd; j++) 
		{
			if (iv>=num)
			{
				break;
			}
			if (iBand==band_BG)
			{
				EEPROM_BASE_POINTER->calPierData2G[i].dacGain[j] = (A_UINT8)value[iv++];
				EEPROM_CONFIG_DIFF_CHANGE(calPierData2G[i].dacGain[j]);
			}
			else
			{
				EEPROM_BASE_POINTER->calPierData5G[i].dacGain[j] = (A_UINT8)value[iv++];
				EEPROM_CONFIG_DIFF_CHANGE(calPierData5G[i].dacGain[j]);
			}
		}
	}
    return VALUE_OK;
}

A_INT32 Qc98xxCalPointPowerSet(double *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, j, k, iv=0;
	int iStart, iEnd, jStart, jEnd, kStart, kEnd;
	int iMax;
	
	iMax = (iBand==band_BG) ? WHAL_NUM_11G_CAL_PIERS : WHAL_NUM_11A_CAL_PIERS;

        iStart = ix; iEnd = ix + 1;
        if (ix < 0 )
        {
                iStart = 0;
        }
        if (ix >= iMax)
        {
                iEnd = iMax;
        }
        jStart = iy; jEnd = iy + 1;
        if (iy < 0)
        {
                jStart = 0;
        }
        if(iy >= WHAL_NUM_CHAINS)
        {
                jEnd = WHAL_NUM_CHAINS;
        }
        kStart = iz; kEnd = iz + 1;
        if (iz < 0)
        {
                kStart = 0;
        }
        if(iz >= WHAL_NUM_CAL_GAINS)
        {
                kEnd = WHAL_NUM_CAL_GAINS;
        }

/*
	if (ix <= 0 || ix >= iMax)
	{
		iStart = 0; iEnd = iMax;
	}
	else
	{
		iStart = ix; iEnd = ix + 1;
	}
	if (iy <= 0 || iy >= WHAL_NUM_CHAINS)
	{
		jStart = 0; jEnd = WHAL_NUM_CHAINS;
	}
	else
	{
		jStart = iy; jEnd = iy + 1;
	}
	if (iz <= 0 || iz >= WHAL_NUM_CAL_GAINS)
	{
		kStart = 0; kEnd = WHAL_NUM_CAL_GAINS;
	}
	else
	{
		kStart = iz; kEnd = iz + 1;
	}
*/
    for (i = iStart; i < iEnd; i++) 
	{
		if (iv>=num)
		{
			break;
		}
		for (j = jStart; j < jEnd; j++) 
		{
			if (iv>=num)
			{
				break;
			}
			for (k = kStart; k < kEnd; k++)
			{
				if (iBand==band_BG)
				{
					EEPROM_BASE_POINTER->calPierData2G[i].calPerPoint[j].power_t8[k] = (A_UINT8)(value[iv++] * 8);
					EEPROM_CONFIG_DIFF_CHANGE(calPierData2G[i].calPerPoint[j].power_t8[k]);
				}
				else
				{
					EEPROM_BASE_POINTER->calPierData5G[i].calPerPoint[j].power_t8[k] = (A_UINT8)(value[iv++] * 8);
					EEPROM_CONFIG_DIFF_CHANGE(calPierData5G[i].calPerPoint[j].power_t8[k]);
				}
				if (iv>=num)
				{
					break;
				}
			}
		}
	}
    return VALUE_OK;
}

/*
 *Function Name:Qc98xxCalPierDataTempMeasSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set calPierData.refPower flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_INT8) 
 *Returns: zero
 */
A_INT32 Qc98xxCalPierDataTempMeasSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iMaxPier, iv=0;

    iMaxPier = (iBand==band_BG) ? WHAL_NUM_11G_CAL_PIERS : WHAL_NUM_11A_CAL_PIERS;
	for (i = ix; i < iMaxPier; i++) 
    {
		if (iBand==band_BG) 
        {
			EEPROM_BASE_POINTER->calPierData2G[i].thermCalVal = (A_INT8)value[iv++];
            EEPROM_CONFIG_DIFF_CHANGE(calPierData2G[i].thermCalVal);
		} 
        else 
        {
			EEPROM_BASE_POINTER->calPierData5G[i].thermCalVal = (A_INT8)value[iv++];
            EEPROM_CONFIG_DIFF_CHANGE(calPierData5G[i].thermCalVal);
		}
		if (iv>=num)
		{
			break;
		}
	}
    return VALUE_OK;
}

/*
 *Function Name:Qc98xxCalPierDataVoltMeasSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		iChain: 0,1,2
 *Description: set calPierData.refPower flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_INT8) 
 *Returns: zero
 */
A_INT32 Qc98xxCalPierDataVoltMeasSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iMaxPier, iv=0;

    iMaxPier = (iBand==band_BG) ? WHAL_NUM_11G_CAL_PIERS : WHAL_NUM_11A_CAL_PIERS;

	for (i = ix; i < iMaxPier; i++) 
    {
		if (iBand==band_BG) 
        {
			EEPROM_BASE_POINTER->calPierData2G[i].voltCalVal = (A_INT8)value[iv++];
            EEPROM_CONFIG_DIFF_CHANGE(calPierData2G[i].voltCalVal);
		} 
        else 
        {
			EEPROM_BASE_POINTER->calPierData5G[i].voltCalVal = (A_INT8)value[iv++];
            EEPROM_CONFIG_DIFF_CHANGE(calPierData5G[i].voltCalVal);
		}
		if (iv>=num)
		{
			break;
		}
	}
    return VALUE_OK;
}

A_INT32 Qc98xxCalFreqTGTcckSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8 bin;
	int i, iv=0;
	for (i=ix; i<WHAL_NUM_11B_TARGET_POWER_CHANS; i++) {
		if (iv>=num)
			break;
		bin = setFREQ2FBIN(value[iv++], iBand);
		EEPROM_BASE_POINTER->targetFreqbinCck[i] = bin;
	}
    EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetFreqbinCck[ix], iv);
	return VALUE_OK;
}

A_INT32 Qc98xxCalFreqTGTLegacyOFDMSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8 bin;
	int i, maxnum, iv=0;

    maxnum = (iBand==band_BG) ? WHAL_NUM_11G_20_TARGET_POWER_CHANS : WHAL_NUM_11A_20_TARGET_POWER_CHANS;

    for (i=ix; i<maxnum; i++) 
    {
		if (iv>=num)
			break;
		bin = setFREQ2FBIN(value[iv++], iBand);
		if (iBand==band_BG)
			EEPROM_BASE_POINTER->targetFreqbin2G[i] = bin;
		else
			EEPROM_BASE_POINTER->targetFreqbin5G[i] = bin;
	}
	if (iBand==band_BG)
    {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetFreqbin2G[ix], iv);
    }
    else
    {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetFreqbin5G[ix], iv);
    }
	return VALUE_OK;
}

A_INT32 Qc98xxCalFreqTGTHT20Set(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8 bin;
	int i, maxnum, iv=0;

    maxnum = (iBand==band_BG) ? WHAL_NUM_11G_20_TARGET_POWER_CHANS : WHAL_NUM_11A_20_TARGET_POWER_CHANS;

    for (i=ix; i<maxnum; i++) 
    {
		if (iv>=num)
			break;
		bin = setFREQ2FBIN(value[iv++], iBand);
		if (iBand==band_BG)
			EEPROM_BASE_POINTER->targetFreqbin2GVHT20[i] = bin;
		else
			EEPROM_BASE_POINTER->targetFreqbin5GVHT20[i] = bin;
	}
	if (iBand==band_BG)
    {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetFreqbin2GVHT20[ix], iv);
    }
    else
    {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetFreqbin5GVHT20[ix], iv);
    }
	return VALUE_OK;
}

A_INT32 Qc98xxCalFreqTGTHT40Set(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8 bin;
	int i, maxnum, iv=0;

    maxnum = (iBand==band_BG) ? WHAL_NUM_11G_40_TARGET_POWER_CHANS : WHAL_NUM_11A_40_TARGET_POWER_CHANS;

    for (i=ix; i<maxnum; i++) 
    {
		if (iv>=num)
			break;
		bin = setFREQ2FBIN(value[iv++], iBand);
		if (iBand==band_BG)
			EEPROM_BASE_POINTER->targetFreqbin2GVHT40[i] = bin;
		else
			EEPROM_BASE_POINTER->targetFreqbin5GVHT40[i] = bin;
	}
	if (iBand==band_BG)
    {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetFreqbin2GVHT40[ix], iv);
    }
    else
    {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetFreqbin5GVHT40[ix], iv);
    }
	return VALUE_OK;
}

A_INT32 Qc98xxCalFreqTGTHT80Set(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8 bin;
	int i, iv=0;

	if (iBand==band_BG)
	{
		return ERR_VALUE_BAD;
	}

    for (i=ix; i<WHAL_NUM_11A_80_TARGET_POWER_CHANS; i++) 
    {
		if (iv>=num)
			break;
		bin = setFREQ2FBIN(value[iv++], iBand);
		EEPROM_BASE_POINTER->targetFreqbin5GVHT80[i] = bin;
	}
    EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetFreqbin5GVHT80[ix], iv);
	return VALUE_OK;
}

A_INT32 Qc98xxCalTGTPwrCCKSet(double *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, j, j0, iv=0;
	for (i=ix; i<WHAL_NUM_11B_TARGET_POWER_CHANS; i++) 
    {
		if (iv>=num)
			break;
		if (i==ix)
			j0=iy;
		else
			j0=0;	
		for (j=j0; j<WHAL_NUM_LEGACY_TARGET_POWER_RATES; j++) 
        {
			if (iBand==band_BG) {
				EEPROM_BASE_POINTER->targetPowerCck[i].tPow2x[j] = (A_UINT8)(value[iv++]*2);	// eep value is double of entered value
                EEPROM_CONFIG_DIFF_CHANGE(targetPowerCck[i].tPow2x[j]);
			} else 
				return ERR_VALUE_BAD;
			if (iv>=num)
				break;
		}
	}
    return VALUE_OK;
}

/*
 *Function Name:Qc98xxCalTGTPwrLegacyOFDMSet
 *Parameters: value
 *			  iBand: 0-bandBG, 1-bandA,		
 *Description: set targetPowerxx flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (A_UINT8) 
 *Returns: zero
 */
A_INT32 Qc98xxCalTGTPwrLegacyOFDMSet(double *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, j, j0, iv=0;
	int iMaxPier;

    iMaxPier = (iBand==band_BG) ? WHAL_NUM_11G_20_TARGET_POWER_CHANS : WHAL_NUM_11A_20_TARGET_POWER_CHANS;
	for (i=ix; i<iMaxPier; i++) 
    {
		if (iv>=num)
			break;
		if (i==ix)
			j0=iy;
		else
			j0=0;	
		for (j=j0; j<WHAL_NUM_LEGACY_TARGET_POWER_RATES; j++) 
        {
			if (iBand==band_BG) {
				EEPROM_BASE_POINTER->targetPower2G[i].tPow2x[j] = (A_UINT8)(value[iv++]*2);	// eep value is double of entered value
                EEPROM_CONFIG_DIFF_CHANGE(targetPower2G[i].tPow2x[j]);
			} else {
				EEPROM_BASE_POINTER->targetPower5G[i].tPow2x[j] = (A_UINT8)(value[iv++]*2);
                EEPROM_CONFIG_DIFF_CHANGE(targetPower5G[i].tPow2x[j]);
			}
			if (iv>=num)
				break;
		}
	}
    return VALUE_OK;
}

A_BOOL IsDiffBetween2xTargetPowersTooBig (A_UINT8 *value, int size)
{
	int i;
	double max, min;

	max = min = value[0];
	for (i = 0; i < size; ++i)
	{
		if (value[i] > max)
		{
			max = value[i];
		}
		else if (value[i] < min)
		{
			min = value[i];
		}
	}
	return ((max - min) > 0xf);
}
//
// This is used when delta(0) > 0xf
// Adjust the smallest delta to 0, and its base accordingly.
// Then adjust the rest
//
void AdjustTargetPowerLocalArrays(A_UINT8 *tPow2xBaseLocal, A_INT8 *tPow2xDeltaLocal, int numOfStreams)
{
	int i, stream;
	int txPow2x, txPow2xLow, streamLow, rateLow;
	A_INT8 baseDelta;

	streamLow = 0;
	rateLow = WHAL_NUM_VHT_TARGET_POWER_RATES-1;
	txPow2xLow = tPow2xBaseLocal[streamLow] + tPow2xDeltaLocal[rateLow];

	// Look for the smallest target power
	for (i = 0; i < WHAL_NUM_VHT_TARGET_POWER_RATES; ++i)
	{
		stream = Qc98xxRateGroupIndex2Stream (i, i+3);
		txPow2x = tPow2xBaseLocal[stream] + tPow2xDeltaLocal[i];
		if (txPow2x < txPow2xLow)
		{
			txPow2xLow = txPow2x;
			streamLow = stream;
			rateLow = i;
		}
	}
	baseDelta = tPow2xBaseLocal[streamLow] - txPow2xLow;
		
	// adjust the bases
	for (i = 0; i < numOfStreams; ++i)
	{
		tPow2xBaseLocal[i] -= baseDelta;
	}
	// adjust deltas
	tPow2xDeltaLocal[rateLow] = 0; 
	for (i = 0; i < WHAL_NUM_VHT_TARGET_POWER_RATES; ++i)
	{
		if (i != rateLow)
		{
			tPow2xDeltaLocal[i] += baseDelta;
		}
	}
}

extern A_UINT16 VhtRateIndexToRateGroupTable[];
extern int Qc98xxTxChainMany(void);
extern A_UINT8 GetHttPow2xFromStreamAndRateIndex(A_UINT8 *tPow2xBase, A_UINT8 *tPow2xDelta, A_UINT8 stream, 
												 A_UINT16 rateIndex, A_BOOL byteDelta, A_UINT16 pierth, A_UINT8 vhtIdx, A_BOOL is2GHz);
extern A_UINT8 GetHttPow2x(A_UINT8 *tPow2xBase, A_UINT8 *tPow2xDelta, A_UINT16 userTargetPowerRateIndex, A_BOOL byteDelta, A_UINT16 pierth, A_UINT8 vhtIdx, A_BOOL is2GHz);


// The algorithm to set the target powers is as follow:
// Look for the smallest target power for all stream rates. -> stream
// Set tPow2xBase[stream] to the smallest
// Since MCS0, MCS10 and MCS20 share the same delta, the other base target powers should be adjusted accordingly
// Then set the other deltas accordingly
// Limitation: MCSs 1_2_11_12_21_22 share the same delta, and MCSs 3-4-13-14-23-24 share the same delta. 
//			   Since the base target powers are set based on MCSs 0_10_20, the target powers for the above rates might be adjusted to
//			   to the nearest posibble.

#define VHT_TARGET_RATE_0_10_20(x) (((x) == VHT_TARGET_RATE_0) || ((x) == VHT_TARGET_RATE_10) || ((x) == VHT_TARGET_RATE_20))
#define VHT_TARGET_RATE_1_2_11_12_21_22(x) (((x) == VHT_TARGET_RATE_1_2) || ((x) == VHT_TARGET_RATE_11_12) || ((x) == VHT_TARGET_RATE_21_22))
#define VHT_TARGET_RATE_3_4_13_14_23_24(x) (((x) == VHT_TARGET_RATE_3_4) || ((x) == VHT_TARGET_RATE_13_14) || ((x) == VHT_TARGET_RATE_23_24))
#define NOT_VHT_TARGET_RATE_LOW_GROUPS(x) (!VHT_TARGET_RATE_0_10_20(x) && !VHT_TARGET_RATE_1_2_11_12_21_22(x) && !VHT_TARGET_RATE_3_4_13_14_23_24(x))
#define FLAG_FIRST		0x1
#define FLAG_LAST		0x2
#define FLAG_MIDDLE		0x0

A_BOOL GetTgtPowerExtBitInfo (A_UINT16 pierth, A_UINT8 vhtIdx, A_UINT16 rateIndex, A_BOOL is2GHz,
                              A_UINT16 *bitthInArray, A_UINT8 *bitthInByte, A_UINT8 *bytethInArray)
{
    A_UINT8 maxPier;

    if (is2GHz)
    {
        maxPier = (vhtIdx == CAL_TARGET_POWER_VHT20_INDEX) ? WHAL_NUM_11G_20_TARGET_POWER_CHANS : WHAL_NUM_11G_40_TARGET_POWER_CHANS;
    }
    else //5GHz
    {
        maxPier = vhtIdx == CAL_TARGET_POWER_VHT20_INDEX ? WHAL_NUM_11A_20_TARGET_POWER_CHANS : 
                    (vhtIdx == CAL_TARGET_POWER_VHT40_INDEX ? WHAL_NUM_11A_40_TARGET_POWER_CHANS : WHAL_NUM_11A_80_TARGET_POWER_CHANS);
    }
    *bitthInArray = ((maxPier * vhtIdx) + pierth) * WHAL_NUM_VHT_TARGET_POWER_RATES + rateIndex;
    *bitthInByte = *bitthInArray % 8;
    *bytethInArray = *bitthInArray >> 3;
    if (*bytethInArray >= (is2GHz ? QC98XX_EXT_TARGET_POWER_SIZE_2G : QC98XX_EXT_TARGET_POWER_SIZE_5G))
    {
        UserPrint("GetTgtPowerExtBitInfo - Error\n");
        return  FALSE;
    }
    return TRUE;
}

A_UINT8 GetTgtPowerExtByteIndex (A_UINT16 pierth, A_UINT8 vhtIdx, A_UINT16 userTargetPowerRateIndex, A_BOOL is2GHz)
{
    A_UINT16 rateIndex;
    A_UINT8 bytethInArray, bitthInByte;
    A_UINT16 bitthInArray;

   	rateIndex = VhtRateIndexToRateGroupTable[userTargetPowerRateIndex];
    GetTgtPowerExtBitInfo (pierth, vhtIdx, rateIndex, is2GHz, &bitthInArray, &bitthInByte, &bytethInArray);
    return (bytethInArray);
}

A_UINT8 GetTgtPowerExtBit (A_UINT16 pierth, A_UINT8 vhtIdx, A_UINT16 rateIndex, A_BOOL is2GHz)
{
    A_UINT8 extByte;
    A_UINT8 bytethInArray, bitthInByte;
    A_UINT16 bitthInArray;
    
    GetTgtPowerExtBitInfo (pierth, vhtIdx, rateIndex, is2GHz, &bitthInArray, &bitthInByte, &bytethInArray);
    extByte = (is2GHz) ? EEPROM_BASE_POINTER->extTPow2xDelta2G[bytethInArray] : EEPROM_BASE_POINTER->extTPow2xDelta5G[bytethInArray];
    return ((extByte >> bitthInByte) & 1);
}

A_BOOL SetTgtPowerExtBit (A_INT8 tPow2xDelta, A_UINT16 pierth, A_UINT8 vhtIdx, A_UINT16 rateIndex, A_BOOL is2GHz)
{
    A_UINT8 bytethInArray, bitthInByte;
    A_UINT16 bitthInArray;
    A_UINT8 *extTPow2xDelta;
    A_UINT8 fifthBit;

    extTPow2xDelta = is2GHz ? Qc98xxEepromStructGet()->extTPow2xDelta2G : Qc98xxEepromStructGet()->extTPow2xDelta5G;
    GetTgtPowerExtBitInfo (pierth, vhtIdx, rateIndex, is2GHz, &bitthInArray, &bitthInByte, &bytethInArray);

    fifthBit = (tPow2xDelta >> 4) & 1;
    extTPow2xDelta[bytethInArray] &= ~(1 << bitthInByte);
    extTPow2xDelta[bytethInArray] |= (fifthBit << bitthInByte);
    return TRUE;
}

A_BOOL SetHttPow2x (double value, A_UINT8 *tPow2xBase, A_UINT8 *tPow2xDelta, A_UINT16 userTargetPowerRateIndex, 
				  int *baseChanged, A_UINT8 flag, int num, A_UINT16 pierth, A_UINT8 vhtIdx, A_BOOL is2GHz)
{
	A_UINT8 tPow2x, shiftBit;
	A_UINT8 tPow2xLowest;
	A_UINT16 targetPowerRateIndex;
	A_INT8 deltaVal, baseDelta;//, mcs0Delta;
	int i, stream;
	// Stored the results in these 2 arrays tPow2xBaseLocal and tPow2xDeltaLocal.
	// Only copy back to EEPROM area when everything is good at the end
	static A_UINT8 tPow2xBaseLocal[WHAL_NUM_STREAMS] = {0};
	// Store delta as signed to check for -ve later. THe deltas should not be -ve nor > 0xf
	static A_INT8  tPow2xDeltaLocal[WHAL_NUM_VHT_TARGET_POWER_RATES] = {0};
	int adjustAttempt;

	if ((stream = Qc98xxUserRateIndex2Stream(userTargetPowerRateIndex)) < 0)
	{
		UserPrint ("Error - Invalid target power rate %d\n", userTargetPowerRateIndex);
		return FALSE;
	}
	targetPowerRateIndex = VhtRateIndexToRateGroupTable[userTargetPowerRateIndex];
	shiftBit = (targetPowerRateIndex % 2) ? 4 : 0;

	// copy tPow2xBase and tPow2xDelta to local memory if this is the first value in the set command
	if (flag & FLAG_FIRST)
	{
		memcpy (tPow2xBaseLocal, tPow2xBase, sizeof(tPow2xBaseLocal));
		for (i = 0; i < sizeof(tPow2xDeltaLocal); i=i+2)
		{
			tPow2xDeltaLocal[i] = (tPow2xDelta[i>>1] & 0xf) | 
                                    GetTgtPowerExtBit(pierth, vhtIdx, i, is2GHz) << 4;
			tPow2xDeltaLocal[i+1] = ((tPow2xDelta[i>>1] >> 4) & 0xf) |
                                    GetTgtPowerExtBit(pierth, vhtIdx, i+1, is2GHz) << 4;
		}
	}
	tPow2x = (A_UINT8)(value * 2);

	tPow2xLowest = tPow2xBase[0];
	// Look for the lowest target power
	for (i = 1; i < WHAL_NUM_STREAMS; ++i)
	{
		if (tPow2xBaseLocal[i] < tPow2xLowest)
		{
			tPow2xLowest = tPow2xBaseLocal[i];
		}
	}

	// if no change in value, do nothing
	if (tPow2x == GetHttPow2xFromStreamAndRateIndex(tPow2xBaseLocal, tPow2xDeltaLocal, stream, targetPowerRateIndex, TRUE, pierth, vhtIdx, is2GHz))
	{
	}
	// if the input power is greater or equal to the bases and the rate is not in the 1st 3 rate groups
	//else if ((tPow2x >= tPow2xLowest) && NOT_VHT_TARGET_RATE_LOW_GROUPS(userTargetPowerRateIndex))
	else if ((tPow2x >= tPow2xLowest) && 
				(NOT_VHT_TARGET_RATE_LOW_GROUPS(userTargetPowerRateIndex) || 
					(flag == 0 && !VHT_TARGET_RATE_0_10_20(userTargetPowerRateIndex))))
	{
		// Just update the delta for that rate
		deltaVal = tPow2x - tPow2xBaseLocal[stream];
		//if (deltaVal > 0xf)
		//{
		//	UserPrint("Error - Reduce the target power value. Max acceptable value is %lf\n", ((tPow2xBase[stream] + 0xf) /2));
		//	UserPrint("Or set the target power for the rate having the lowest target power first.\n");
		//	return FALSE;
		//}
		tPow2xDeltaLocal[targetPowerRateIndex] = deltaVal; 
	}
	// MCS0 ... MCS4
	else if (VHT_TARGET_RATE_0_10_20(userTargetPowerRateIndex))
	{
		baseDelta = tPow2xBaseLocal[stream] + tPow2xDeltaLocal[targetPowerRateIndex] - tPow2x;
		if (flag & FLAG_FIRST)
		{
			tPow2xDeltaLocal[targetPowerRateIndex] -= baseDelta;
			for (i = 0; i < WHAL_NUM_STREAMS; ++i)
			{
				if (i != stream)
				{
					tPow2xBaseLocal[i] += baseDelta;
				}
			}
			for (i = 0; i < WHAL_NUM_VHT_TARGET_POWER_RATES; ++i)
			{
				if (!Qc98xxIsRateInStream(stream, i))
				{
					tPow2xDeltaLocal[i] -= baseDelta;
				}
			}
		}
		else
		{
			// adjust the stream base only
			tPow2xBaseLocal[stream] -= baseDelta;
			for (i = (stream == 1 ? 8 : 13); i < (stream == 1 ? 13 : 18); ++i)
			{
				//if (!Qc98xxIsRateInStream(stream, i))
				//{
					tPow2xDeltaLocal[i] += baseDelta;
				//}
			}
		}
		*baseChanged = 1;
	}
	// For the rest, we have to adjust bases and deltas
	// use tPow2xBaseLocal and tPow2xDeltaLocal, in case there is error
	else
	{
		baseDelta = tPow2xBaseLocal[stream] - tPow2x;
		
		// adjust the bases
		for (i = 0; i < WHAL_NUM_STREAMS; ++i)
		{
			tPow2xBaseLocal[i] -= baseDelta;
		}
		// adjust deltas
		tPow2xDeltaLocal[targetPowerRateIndex] = 0; 
		for (i = 0; i < WHAL_NUM_VHT_TARGET_POWER_RATES; ++i)
		{
			if (i != targetPowerRateIndex)
			{
				tPow2xDeltaLocal[i] += baseDelta;
			}
		}
		*baseChanged = 1;
	}

	if (flag & FLAG_LAST)
	{
		// Check for errors before copy to EEPROM area
		adjustAttempt = 3;
		while (adjustAttempt)
		{
			for (i = 0; i < WHAL_NUM_VHT_TARGET_POWER_RATES; i++)
			{
				if (tPow2xDeltaLocal[i] < 0)
				{
					UserPrint("The values needed to be adjusted so the difference between the values not > 31\n");
					return FALSE;
				}
				else if(tPow2xDeltaLocal[i] > 0xf)
				{
					// adjust the bases and deltas if possible
					AdjustTargetPowerLocalArrays(tPow2xBaseLocal, tPow2xDeltaLocal, WHAL_NUM_STREAMS);
					break;
				}
			}
			if (i == WHAL_NUM_VHT_TARGET_POWER_RATES)
			{
				break;
			}
			adjustAttempt--;
		}

		// Successfully set the target powers
		if (*baseChanged || num > 1)
		{
			for (i = 0; i < WHAL_NUM_STREAMS; ++i)
			{
				tPow2xBase[i] = tPow2xBaseLocal[i];
			}
			for (i = 0; i < WHAL_NUM_VHT_TARGET_POWER_RATES; i=i+2)
			{
				tPow2xDelta[i>>1] = (tPow2xDeltaLocal[i] & 0xf) | ((tPow2xDeltaLocal[i+1] << 4) & 0xf0);
                SetTgtPowerExtBit (tPow2xDeltaLocal[i], pierth, vhtIdx, i, is2GHz);
                SetTgtPowerExtBit (tPow2xDeltaLocal[i+1], pierth, vhtIdx, i+1, is2GHz);
			}
		}
		else
		{
			if (shiftBit == 0)
			{
				tPow2xDelta[targetPowerRateIndex>>1] = (tPow2xDeltaLocal[targetPowerRateIndex] & 0xf) | ((tPow2xDeltaLocal[targetPowerRateIndex+1] << 4) & 0xf0);
			}
			else
			{
				tPow2xDelta[targetPowerRateIndex>>1] = (tPow2xDeltaLocal[targetPowerRateIndex-1] & 0xf) | ((tPow2xDeltaLocal[targetPowerRateIndex] << 4) & 0xf0);
			}
            SetTgtPowerExtBit (tPow2xDeltaLocal[targetPowerRateIndex], pierth, vhtIdx, targetPowerRateIndex, is2GHz);
		}
	}
	return TRUE;
}

A_BOOL IsDiffBetweenTargetPowersTooBig (A_UINT8 *value, int size)
{
	int i;
	A_UINT8 max, min;

	max = min = value[0];
	for (i = 0; i < size; ++i)
	{
		if (value[i] > max)
		{
			max = value[i];
		}
		else if (value[i] < min)
		{
			min = value[i];
		}
	}
	return ((max - min) > 0x1f);
}

A_BOOL IsValidInputTargetPowers(double *value, A_UINT8 *tPow2xBase, A_UINT8 *tPow2xDelta, int j0, int iv, int num,
                                A_UINT16 pierth, A_UINT8 vhtIdx, A_BOOL is2GHz)
{
	int j, j1, i;
	A_UINT8 endValue[VHT_TARGET_RATE_LAST];

	i = iv;
	for (j=0; j < j0; j++)
	{
		endValue[j] = GetHttPow2x(tPow2xBase, tPow2xDelta, j, FALSE, pierth, vhtIdx, is2GHz);
	}
	
	j1 = (j0 + num < VHT_TARGET_RATE_LAST) ? (j0 + num) : VHT_TARGET_RATE_LAST;
		
	for (j=j0; j < j1; j++) 
    {
		endValue[j] = (A_UINT8)(value[i++] * 2);
	}
	for (; j < VHT_TARGET_RATE_LAST; j++)
	{
		endValue[j] = GetHttPow2x(tPow2xBase, tPow2xDelta, j, FALSE, pierth, vhtIdx, is2GHz);
	}
		
	if (IsDiffBetweenTargetPowersTooBig(endValue, VHT_TARGET_RATE_LAST))
	{
		UserPrint("Error - The difference of the target powers in a pier should not be greater than 15.5 dB\n");
		return FALSE;
	}
	return TRUE;
}

A_INT32 Qc98xxCalTGTPwrHT20Set(double *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, j, j0, iv=0;
	int iMaxPier, jMaxRate;
	int baseChanged;
	A_UINT8 *tPow2xBase, *tPow2xDelta;
	A_UINT8 flag;
	A_BOOL Ok;

    iMaxPier = (iBand==band_BG) ? WHAL_NUM_11G_20_TARGET_POWER_CHANS : WHAL_NUM_11A_20_TARGET_POWER_CHANS;
	jMaxRate = (iBand==band_BG) ? WHAL_NUM_11G_20_TARGET_POWER_RATES : WHAL_NUM_11A_20_TARGET_POWER_RATES;

	if (ix >= iMaxPier || iy >= VHT_TARGET_RATE_LAST)
	{
		return ERR_RETURN;
	}

	for (i=ix; i<iMaxPier; i++) 
    {
		if (iv>=num)
			break;
		if (i==ix)
			j0=iy;
		else
			j0=0;

		// Since the deltas stored in nibbles, the difference of the target powers in a pier should not be greater than 15x.5 steps
		// construct endValue to check
		if (iBand==band_BG)
		{
			tPow2xBase = EEPROM_BASE_POINTER->targetPower2GVHT20[i].tPow2xBase;
			tPow2xDelta = EEPROM_BASE_POINTER->targetPower2GVHT20[i].tPow2xDelta;
		}
		else
		{
			tPow2xBase = EEPROM_BASE_POINTER->targetPower5GVHT20[i].tPow2xBase;
			tPow2xDelta = EEPROM_BASE_POINTER->targetPower5GVHT20[i].tPow2xDelta;
		}
		if (!IsValidInputTargetPowers(value, tPow2xBase, tPow2xDelta, j0, iv, num, i, CAL_TARGET_POWER_VHT20_INDEX, iBand==band_BG))
		{
			return ERR_RETURN;
		}

		flag = FLAG_FIRST;
		baseChanged = 0;
		// The set command can set just one or the whole rates per channel
		for (j=j0; j<VHT_TARGET_RATE_LAST; j++) 
        {
			if ((iv + 1) == num) 
			{
				flag |= FLAG_LAST;
			}
			
			Ok = SetHttPow2x (value[iv++], tPow2xBase, tPow2xDelta, j, &baseChanged, flag, num, i, CAL_TARGET_POWER_VHT20_INDEX, iBand==band_BG);
			
			if (!Ok)
			{
				return ERR_RETURN;
			}
			//baseChangedCount += baseChanged;
			if (iv>=num)
			{
				break;
			}
			flag = 0;
		}
		if (iBand==band_BG)
		{
			if (baseChanged)
			{
				EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetPower2GVHT20[i].tPow2xBase, WHAL_NUM_STREAMS);
				EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetPower2GVHT20[i].tPow2xDelta, (jMaxRate >>1));
                EEPROM_CONFIG_DIFF_CHANGE_ARRAY(extTPow2xDelta2G, 1); 
			}
			else
			{
				EEPROM_CONFIG_DIFF_CHANGE(targetPower2GVHT20[i].tPow2xDelta[(j)>>1]);
                EEPROM_CONFIG_DIFF_CHANGE(extTPow2xDelta2G[GetTgtPowerExtByteIndex(i, CAL_TARGET_POWER_VHT20_INDEX, j, TRUE)]); 
			}
		}
		else
		{
			if (baseChanged)
			{
				EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetPower5GVHT20[i].tPow2xBase, WHAL_NUM_STREAMS);
				EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetPower5GVHT20[i].tPow2xDelta, (jMaxRate >>1));
                EEPROM_CONFIG_DIFF_CHANGE_ARRAY(extTPow2xDelta5G, 1); 
			}
			else
			{
				EEPROM_CONFIG_DIFF_CHANGE(targetPower5GVHT20[i].tPow2xDelta[(j)>>1]);
                EEPROM_CONFIG_DIFF_CHANGE(extTPow2xDelta5G[GetTgtPowerExtByteIndex(i, CAL_TARGET_POWER_VHT20_INDEX, j,FALSE)]); 
			}
		}
	}
    return VALUE_OK;
}

A_INT32 Qc98xxCalTGTPwrHT40Set(double *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, j, j0, iv=0;
	int iMaxPier, jMaxRate;
	int baseChanged;
	A_UINT8 *tPow2xBase, *tPow2xDelta;
	A_UINT8 flag;
	A_BOOL Ok;


    iMaxPier = (iBand==band_BG) ? WHAL_NUM_11G_40_TARGET_POWER_CHANS : WHAL_NUM_11A_40_TARGET_POWER_CHANS;
	jMaxRate = (iBand==band_BG) ? WHAL_NUM_11G_40_TARGET_POWER_RATES : WHAL_NUM_11A_40_TARGET_POWER_RATES;

	if (ix >= iMaxPier || iy >= VHT_TARGET_RATE_LAST)
	{
		return ERR_RETURN;
	}

	for (i=ix; i<iMaxPier; i++) 
    {
		if (iv>=num)
			break;
		if (i==ix)
			j0=iy;
		else
			j0=0;

		// Since the deltas stored in nibbles, the difference of the target powers in a pier should not be greater than 15x.5 steps
		// construct endValue to check
		if (iBand==band_BG)
		{
			tPow2xBase = EEPROM_BASE_POINTER->targetPower2GVHT40[i].tPow2xBase;
			tPow2xDelta = EEPROM_BASE_POINTER->targetPower2GVHT40[i].tPow2xDelta;
		}
		else
		{
			tPow2xBase = EEPROM_BASE_POINTER->targetPower5GVHT40[i].tPow2xBase;
			tPow2xDelta = EEPROM_BASE_POINTER->targetPower5GVHT40[i].tPow2xDelta;
		}
		if (!IsValidInputTargetPowers(value, tPow2xBase, tPow2xDelta, j0, iv, num, i, CAL_TARGET_POWER_VHT40_INDEX, iBand==band_BG))
		{
			return ERR_RETURN;
		}

		flag = FLAG_FIRST;
		baseChanged = 0;
		// The set command can set just one or the whole rates per channel
		for (j=j0; j<VHT_TARGET_RATE_LAST; j++) 
        {
			if ((iv + 1) == num)
			{
				flag |= FLAG_LAST;
			}
			
			Ok = SetHttPow2x (value[iv++], tPow2xBase, tPow2xDelta, j, &baseChanged, flag, num, i, CAL_TARGET_POWER_VHT40_INDEX, iBand==band_BG);
			
			if (!Ok)
			{
				return ERR_RETURN;
			}
			//baseChangedCount += baseChanged;
			if (iv>=num)
			{
				break;
			}
			flag = 0;
		}
		if (iBand==band_BG)
		{
			if (baseChanged)
			{
				EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetPower2GVHT40[i].tPow2xBase, WHAL_NUM_STREAMS);
				EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetPower2GVHT40[i].tPow2xDelta, (jMaxRate >>1));
                EEPROM_CONFIG_DIFF_CHANGE_ARRAY(extTPow2xDelta2G, 1); 
			}
			else
			{
				EEPROM_CONFIG_DIFF_CHANGE(targetPower2GVHT40[i].tPow2xDelta[(j)>>1]);
                EEPROM_CONFIG_DIFF_CHANGE(extTPow2xDelta2G[GetTgtPowerExtByteIndex(i, CAL_TARGET_POWER_VHT40_INDEX, j,TRUE)]); 
			}
		}
		else
		{
			if (baseChanged)
			{
				EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetPower5GVHT40[i].tPow2xBase, WHAL_NUM_STREAMS);
				EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetPower5GVHT40[i].tPow2xDelta, (jMaxRate >>1));
                EEPROM_CONFIG_DIFF_CHANGE_ARRAY(extTPow2xDelta5G, 1); 
			}
			else
			{
				EEPROM_CONFIG_DIFF_CHANGE(targetPower5GVHT40[i].tPow2xDelta[(j)>>1]);
                EEPROM_CONFIG_DIFF_CHANGE(extTPow2xDelta5G[GetTgtPowerExtByteIndex(i, CAL_TARGET_POWER_VHT40_INDEX, j,FALSE)]); 
			}
		}
	}
    return VALUE_OK;
}

A_INT32 Qc98xxCalTGTPwrHT80Set(double *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, j, j0, iv=0;
	int iMaxPier, jMaxRate;
	int baseChanged;
	A_UINT8 flag;
	A_BOOL Ok;

	if (iBand==band_BG)
	{
		return ERR_VALUE_BAD;
	}
    iMaxPier = WHAL_NUM_11A_80_TARGET_POWER_CHANS;
	jMaxRate = WHAL_NUM_11A_80_TARGET_POWER_RATES;

	if (ix >= iMaxPier || iy >= VHT_TARGET_RATE_LAST)
	{
		return ERR_RETURN;
	}

	for (i=ix; i<iMaxPier; i++) 
    {
		if (iv>=num)
			break;
		if (i==ix)
			j0=iy;
		else
			j0=0;

		// Since the deltas stored in nibbles, the difference of the target powers in a pier should not be greater than 15x.5 steps
		// construct endValue to check
		if (!IsValidInputTargetPowers(value, EEPROM_BASE_POINTER->targetPower5GVHT80[i].tPow2xBase, 
										EEPROM_BASE_POINTER->targetPower5GVHT80[i].tPow2xDelta, j0, iv, num,
                                        i, CAL_TARGET_POWER_VHT80_INDEX, FALSE))
		{
			return ERR_RETURN;
		}

		flag = FLAG_FIRST;
		baseChanged = 0;
		// The set command can set just one or the whole rates per channel
		for (j=j0; j<VHT_TARGET_RATE_LAST; j++) 
        {
			if ((iv + 1) == num)
			{
				flag |= FLAG_LAST;
			}
			
			Ok = SetHttPow2x (value[iv++], EEPROM_BASE_POINTER->targetPower5GVHT80[i].tPow2xBase, 
								EEPROM_BASE_POINTER->targetPower5GVHT80[i].tPow2xDelta, j, &baseChanged, flag, num,
                                i, CAL_TARGET_POWER_VHT80_INDEX, FALSE);
			
			if (!Ok)
			{
				return ERR_RETURN;
			}
			//baseChangedCount += baseChanged;
			if (iv>=num)
			{
				break;
			}
			flag = 0;
		}
		if (baseChanged)
		{
			EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetPower5GVHT80[i].tPow2xBase, WHAL_NUM_STREAMS);
			EEPROM_CONFIG_DIFF_CHANGE_ARRAY(targetPower5GVHT80[i].tPow2xDelta, (jMaxRate >>1));
            EEPROM_CONFIG_DIFF_CHANGE_ARRAY(extTPow2xDelta5G, 1); 
		}
		else
		{
			EEPROM_CONFIG_DIFF_CHANGE(targetPower5GVHT80[i].tPow2xDelta[(j)>>1]);
            EEPROM_CONFIG_DIFF_CHANGE(extTPow2xDelta5G[GetTgtPowerExtByteIndex(i, CAL_TARGET_POWER_VHT80_INDEX, j, FALSE)]); 
		}
	}
    return VALUE_OK;
}

A_INT32 Qc98xxCtlIndexSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int maxnum, i, iv=0;

    maxnum = (iBand==band_BG) ? WHAL_NUM_CTLS_2G : WHAL_NUM_CTLS_5G;
	for (i=ix; i<maxnum; i++) {
		if (iv>=num)
			break;
		if (iBand==band_BG) 
			EEPROM_BASE_POINTER->ctlIndex2G[i] = (A_UINT8)(value[iv++]);
		else 
			EEPROM_BASE_POINTER->ctlIndex5G[i] = (A_UINT8)(value[iv++]);
	}
	if (iBand==band_BG) 
    {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(ctlIndex2G[ix], iv);
    }
    else
    {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(ctlIndex5G[ix], iv);
    }
    return VALUE_OK;
}

A_INT32 Qc98xxCtlFreqSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8 bin;
	int i, j, j0, iCtl, iEdge, iv=0;
	
    if (iBand==band_BG) {
		iCtl = WHAL_NUM_CTLS_2G;
		iEdge = WHAL_NUM_BAND_EDGES_2G;
	} else {
		iCtl = WHAL_NUM_CTLS_5G;
		iEdge = WHAL_NUM_BAND_EDGES_5G;
	}
	
    for (i=ix; i<iCtl; i++) {
		if (iv>=num)
			break;
		if (i==ix)
			j0=iy;
		else
			j0=0;	
		for (j=j0; j<iEdge; j++) {
			bin = setFREQ2FBIN(value[iv++], iBand);
			if (iBand==band_BG)
				EEPROM_BASE_POINTER->ctlFreqbin2G[i][j] = bin;
			else
				EEPROM_BASE_POINTER->ctlFreqbin5G[i][j] = bin;
			if (iv>=num)
				break;
		}
	}
    if (iBand==band_BG)
    {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(ctlFreqbin2G[ix][iy], iv);
	} 
    else 
    {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(ctlFreqbin5G[ix][iy], iv);
	}
    return VALUE_OK;
}

A_INT32 Qc98xxCtlPowerSet(double *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8  value6;
	int i, j, j0, iCtl, iEdge, iv=0;
	if (iBand==band_BG) {
		iCtl = WHAL_NUM_CTLS_2G;
		iEdge = WHAL_NUM_BAND_EDGES_2G;
	} else {
		iCtl = WHAL_NUM_CTLS_5G;
		iEdge = WHAL_NUM_BAND_EDGES_5G;
	}
	for (i=ix; i<iCtl; i++) 
    {
		if (iv>=num)
			break;
		if (i==ix)
			j0=iy;
		else
			j0=0;	
		for (j=j0; j<iEdge; j++) 
        {
			value6 = ((A_UINT8)(value[iv++]*2.0)) & 0x3f;
			if (iBand==band_BG) {
				EEPROM_BASE_POINTER->ctlData2G[i].ctl_edges[j].u.tPower = value6;
                EEPROM_CONFIG_DIFF_CHANGE(ctlData2G[i].ctl_edges[j].u);
			} else {
				EEPROM_BASE_POINTER->ctlData5G[i].ctl_edges[j].u.tPower = value6;
                EEPROM_CONFIG_DIFF_CHANGE(ctlData5G[i].ctl_edges[j].u);
			}
			if (iv>=num)
				break;
		}
	}
    return VALUE_OK;
}

A_INT32 Qc98xxCtlFlagSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	A_UINT8  value2;
	int i, j, j0, iCtl, iEdge, iv=0;
	if (iBand==band_BG) {
		iCtl = WHAL_NUM_CTLS_2G;
		iEdge = WHAL_NUM_BAND_EDGES_2G;
	} else {
		iCtl = WHAL_NUM_CTLS_5G;
		iEdge = WHAL_NUM_BAND_EDGES_5G;
	}
	for (i=ix; i<iCtl; i++) 
    {
		if (iv>=num)
			break;
		if (i==ix)
			j0=iy;
		else
			j0=0;	
		for (j=j0; j<iEdge; j++) 
        {
			value2 = (A_UINT8)(value[iv++] & 0x3) ;
			if (iBand==band_BG) {
				EEPROM_BASE_POINTER->ctlData2G[i].ctl_edges[j].u.flag = value2;
                EEPROM_CONFIG_DIFF_CHANGE(ctlData2G[i].ctl_edges[j].u);
			} else {
				EEPROM_BASE_POINTER->ctlData5G[i].ctl_edges[j].u.flag = value2;
                EEPROM_CONFIG_DIFF_CHANGE(ctlData5G[i].ctl_edges[j].u);
			}
			if (iv>=num)
				break;
		}
	}
    return VALUE_OK;
}

/************************************************************************* 
*
* Qc98xxNoiseFloorSet()
* 
* Set noise floor. Designated as Pnf(dBr).  Value is set externally from application
* using the set command instead of internally during RSSI calibration.
*************************************************************************/ 

A_INT32 Qc98xxNoiseFloorSet ( int *value, int ix, int iy, int iz, int num, int iBand )
{
	int i, j, iv=0;
	int iStart, iEnd, jStart, jEnd;
	int iMax;
	
	iMax = (iBand==band_BG) ? WHAL_NUM_11G_CAL_PIERS : WHAL_NUM_11A_CAL_PIERS;

	if (ix <= 0 || ix >= iMax)
	{
		iStart = 0; iEnd = iMax;
	}
	else
	{
		iStart = ix; iEnd = ix + 1;
	}

	if (iy <= 0 || iy >= WHAL_NUM_CHAINS)
	{
		jStart = 0; jEnd = WHAL_NUM_CHAINS;
	}
	else
	{
		jStart = iy; jEnd = iy + 1;
	}
    for (i = iStart; i < iEnd; i++) 
	{
		if (iv>=num)
		{
			break;
		}
		for (j = jStart; j < jEnd; j++) 
		{
			if (iBand==band_BG)
			{
				EEPROM_BASE_POINTER->nfCalData2G[i].NF_Power_dBr[j] = (A_UINT8)value[iv++];
				EEPROM_CONFIG_DIFF_CHANGE(nfCalData2G[i].NF_Power_dBr[j]);
			}
			else
			{
				EEPROM_BASE_POINTER->nfCalData5G[i].NF_Power_dBr[j] = (A_UINT8)value[iv++];
				EEPROM_CONFIG_DIFF_CHANGE(nfCalData5G[i].NF_Power_dBr[j]);
			}
			if (iv>=num)
			{
				break;
			}
		}
	}
    return VALUE_OK;
}

/************************************************************************* 
*
* Qc98xxNoiseFloorPowerSet()
* 
* Set noise floor power. Designated as Pnf(dBm).  Value is set externally
* from application using the set command instead of internally during RSSI
* calibration.
*************************************************************************/ 

A_INT32 Qc98xxNoiseFloorPowerSet ( int *value, int ix, int iy, int iz, int num, int iBand )
{
	int i, j, iv=0;
	int iStart, iEnd, jStart, jEnd;
	int iMax;
	
	iMax = (iBand==band_BG) ? WHAL_NUM_11G_CAL_PIERS : WHAL_NUM_11A_CAL_PIERS;

	if (ix <= 0 || ix >= iMax)
	{
		iStart = 0; iEnd = iMax;
	}
	else
	{
		iStart = ix; iEnd = ix + 1;
	}

	if (iy <= 0 || iy >= WHAL_NUM_CHAINS)
	{
		jStart = 0; jEnd = WHAL_NUM_CHAINS;
	}
	else
	{
		jStart = iy; jEnd = iy + 1;
	}
    for (i = iStart; i < iEnd; i++) 
	{
		if (iv>=num)
		{
			break;
		}
		for (j = jStart; j < jEnd; j++) 
		{
			if (iBand==band_BG)
			{
				EEPROM_BASE_POINTER->nfCalData2G[i].NF_Power_dBm[j] = (A_UINT8)value[iv++];
				EEPROM_CONFIG_DIFF_CHANGE(nfCalData2G[i].NF_Power_dBm[j]);
			}
			else
			{
				EEPROM_BASE_POINTER->nfCalData5G[i].NF_Power_dBm[j] = (A_UINT8)value[iv++];
				EEPROM_CONFIG_DIFF_CHANGE(nfCalData5G[i].NF_Power_dBm[j]);
			}
			if (iv>=num)
			{
				break;
			}
		}
	}
    return VALUE_OK;
}


/************************************************************************* 
*
* Qc98xxNoiseFloorTemperatureSet()
* 
* Set noise floor temperature. Temperature value during RSSI calibration.
* Value is set externally using the set command from application instead
* of internally during RSSI calibration.
*************************************************************************/ 

A_INT32 Qc98xxNoiseFloorTemperatureSet ( int *value, int ix, int iy, int iz, int num, int iBand )
{
	int i, iv=0;
	int iStart, iEnd;
	int iMax;

	iMax = (iBand==band_BG) ? WHAL_NUM_11G_CAL_PIERS : WHAL_NUM_11A_CAL_PIERS;

	if (ix <= 0 || ix >= iMax)
	{
		iStart = 0; iEnd = iMax;
	}
	else
	{
		iStart = ix; iEnd = ix + 1;
	}

	for (i = iStart; i < iEnd; i++) 
	{
		if (iv>=num)
		{
			break;
		}
		if (iBand==band_BG)
		{
			EEPROM_BASE_POINTER->nfCalData2G[i].NF_thermCalVal = (A_UINT8)value[iv++];
			EEPROM_CONFIG_DIFF_CHANGE(nfCalData2G[i].NF_thermCalVal);
		}
		else
		{
			EEPROM_BASE_POINTER->nfCalData5G[i].NF_thermCalVal = (A_UINT8)value[iv++];
			EEPROM_CONFIG_DIFF_CHANGE(nfCalData5G[i].NF_thermCalVal);
		}
		if (iv>=num)
		{
			break;
		}

	}
    return VALUE_OK;
}

/************************************************************************* 
*
* Qc98xxNoiseFloorTemperatureSlopeSet()
* 
* Set noise floor temperature slope. Expresses sensitivity of the noise
* floor to temperature. Value is set externally using the set command from
* application instead of internally during RSSI calibration.
*************************************************************************/ 

A_INT32 Qc98xxNoiseFloorTemperatureSlopeSet ( int *value, int ix, int iy, int iz, int num, int iBand )
{
	int i, iv=0;
	int iStart, iEnd;
	int iMax;

	iMax = (iBand==band_BG) ? WHAL_NUM_11G_CAL_PIERS : WHAL_NUM_11A_CAL_PIERS;

	if (ix <= 0 || ix >= iMax)
	{
		iStart = 0; iEnd = iMax;
	}
	else
	{
		iStart = ix; iEnd = ix + 1;
	}

	for (i = iStart; i < iEnd; i++) 
	{
		if (iv>=num)
		{
			break;
		}
		if (iBand==band_BG)
		{
			EEPROM_BASE_POINTER->nfCalData2G[i].NF_thermCalSlope = (A_UINT8)value[iv++];
			EEPROM_CONFIG_DIFF_CHANGE(nfCalData2G[i].NF_thermCalSlope);
		}
		else
		{
			EEPROM_BASE_POINTER->nfCalData5G[i].NF_thermCalSlope = (A_UINT8)value[iv++];
			EEPROM_CONFIG_DIFF_CHANGE(nfCalData5G[i].NF_thermCalSlope);
		}
		if (iv>=num)
		{
			break;
		}
	}
    return VALUE_OK;
}





/************************************************************************* 
*
* Qc98xxRSSICalInfoNoiseFloorSet()
* 
* Noise floor.  Sum of results of noise floor estimation circuit plus 
* spectral scan RSSI with no input. Designated as Pnf(dBr).  Value
* is set based on internal computations instead of externally by application.
*************************************************************************/ 

A_INT32 Qc98xxRSSICalInfoNoiseFloorSet ( int freq, int chain, int noiseFloorPower_dBr )
{
	int ipier;
	int is2GHz;
	int fx;

	if ( chain >= WHAL_NUM_CHAINS )
	{
		UserPrint ( "Invalid chain index, must be less than %d\n", WHAL_NUM_CHAINS );
		return -1;
	}

	is2GHz = ( freq < 4000);
	if ( is2GHz )
	{
		// look for correct frequency pier
		for ( ipier = 0; ipier < WHAL_NUM_11G_CAL_PIERS; ipier++ )
		{
			fx = WHAL_FBIN2FREQ ( EEPROM_BASE_POINTER->calFreqPier2G [ ipier ], is2GHz);
			if ( fx == freq )
			{
				EEPROM_BASE_POINTER->nfCalData2G [ ipier ].NF_Power_dBr [ chain ] = noiseFloorPower_dBr;
				EEPROM_CONFIG_DIFF_CHANGE( nfCalData2G[ipier].NF_Power_dBr [ chain ] );
				return 0;
			}
		}
		return -1;
	}
	else
	{
		// look for correct frequency pier
		for ( ipier = 0; ipier < WHAL_NUM_11A_CAL_PIERS; ipier++ )
		{
			fx = WHAL_FBIN2FREQ ( EEPROM_BASE_POINTER->calFreqPier5G [ ipier ], is2GHz);
			if ( fx == freq )
			{
				EEPROM_BASE_POINTER->nfCalData5G [ ipier ].NF_Power_dBr [ chain ] = noiseFloorPower_dBr;
				EEPROM_CONFIG_DIFF_CHANGE( nfCalData5G[ipier].NF_Power_dBr [ chain ] );
				return 0;
			}
		}
		return -1;
	}
}

/************************************************************************* 
*
* Qc98xxRSSICalInfoNoiseFloorPowerSet()
* 
* Noise floor power.  Computed as power of known signal minus spectral scan
* RSSI level in dB.  That level is the amount over the noise floor power.
* Thus, the result of the subtraction is noise floor power. Designated as Pnf(dBm).
* 
* Value is set based on internal computations instead of externally by application.
*************************************************************************/ 

A_INT32 Qc98xxRSSICalInfoNoiseFloorPowerSet ( int freq, int chain, int noiseFloorPower_dBm )
{
	int ipier;
	int is2GHz;
	int fx;

	if ( chain >= WHAL_NUM_CHAINS )
	{
		UserPrint ( "Invalid chain index, must be less than %d\n", WHAL_NUM_CHAINS );
		return -1;
	}

	is2GHz = ( freq < 4000);
	if ( is2GHz )
	{
		// look for correct frequency pier
		for ( ipier = 0; ipier < WHAL_NUM_11G_CAL_PIERS; ipier++ )
		{
			fx = WHAL_FBIN2FREQ ( EEPROM_BASE_POINTER->calFreqPier2G [ ipier ], is2GHz);
			if ( fx == freq )
			{
				EEPROM_BASE_POINTER->nfCalData2G [ ipier ].NF_Power_dBm [ chain ] = noiseFloorPower_dBm;
				EEPROM_CONFIG_DIFF_CHANGE( nfCalData2G[ipier].NF_Power_dBm [ chain ] );
				return 0;
			}
		}
		return -1;
	}
	else
	{
		// look for correct frequency pier
		for ( ipier = 0; ipier < WHAL_NUM_11A_CAL_PIERS; ipier++ )
		{
			fx = WHAL_FBIN2FREQ ( EEPROM_BASE_POINTER->calFreqPier5G [ ipier ], is2GHz);
			if ( fx == freq )
			{
				EEPROM_BASE_POINTER->nfCalData5G [ ipier ].NF_Power_dBm [ chain ] = noiseFloorPower_dBm;
				EEPROM_CONFIG_DIFF_CHANGE( nfCalData5G[ipier].NF_Power_dBm [ chain ] );
				return 0;
			}
		}
		return -1;
	}
}

/************************************************************************* 
*
* Qc98xxRSSICalInfoNoiseFloorTemperatureSet()
* 
* Temperature during noise floor calibration.  Needed as noise floor
* power is a function of temperature.
* 
* Value is set based on internal computations instead of externally by application.
*************************************************************************/ 

A_INT32 Qc98xxRSSICalInfoNoiseFloorTemperatureSet ( int freq, int chain, int noiseFloorTemperature )
{
	int ipier;
	int is2GHz;
	int fx;

	if ( chain >= WHAL_NUM_CHAINS )
	{
		UserPrint ( "Invalid chain index, must be less than %d\n", WHAL_NUM_CHAINS );
		return -1;
	}

	is2GHz = ( freq < 4000);
	if ( is2GHz )
	{
		// look for correct frequency pier
		for ( ipier = 0; ipier < WHAL_NUM_11G_CAL_PIERS; ipier++ )
		{
			fx = WHAL_FBIN2FREQ ( EEPROM_BASE_POINTER->calFreqPier2G [ ipier ], is2GHz);
			if ( fx == freq )
			{
				EEPROM_BASE_POINTER->nfCalData2G [ ipier ].NF_thermCalVal  = noiseFloorTemperature;
				EEPROM_CONFIG_DIFF_CHANGE( nfCalData2G[ipier].NF_thermCalVal );
				return 0;
			}
		}
		return -1;
	}
	else
	{
		// look for correct frequency pier
		for ( ipier = 0; ipier < WHAL_NUM_11A_CAL_PIERS; ipier++ )
		{
			fx = WHAL_FBIN2FREQ ( EEPROM_BASE_POINTER->calFreqPier5G [ ipier ], is2GHz);
			if ( fx == freq )
			{
				EEPROM_BASE_POINTER->nfCalData5G [ ipier ].NF_thermCalVal = noiseFloorTemperature;
				EEPROM_CONFIG_DIFF_CHANGE( nfCalData5G[ipier].NF_thermCalVal );
				return 0;
			}
		}
		return -1;
	}

}



int Qc98xxCalInfoCalibrationPierSet(int pierIdx, int freq, int chain, int gain, int gainIndex, int dacGain, double power, int pwrCorrection, int voltMeas, int tempMeas, int calPoint)
{
    QC98XX_EEPROM *eep = Qc98xxEepromStructGet();

    if(chain >= WHAL_NUM_CHAINS)
    {
        UserPrint("Invalid chain index, must be less than %d\n", WHAL_NUM_CHAINS);
        return -1;
    }
   
    if(freq < 3000) 
    { /* 2GHz frequency pier */
        if(pierIdx >= WHAL_NUM_11G_CAL_PIERS)
        {
            UserPrint("Invalid 2GHz cal pier index, must be less than %d\n", WHAL_NUM_11G_CAL_PIERS);
            return -1;
        }
		eep->calFreqPier2G[pierIdx] = WHAL_FREQ2FBIN(freq,1);
		eep->calPierData2G[pierIdx].thermCalVal = tempMeas;
		eep->calPierData2G[pierIdx].voltCalVal = voltMeas; 
		eep->calPierData2G[pierIdx].dacGain[calPoint] = (A_INT8)dacGain; 
		eep->calPierData2G[pierIdx].calPerPoint[chain].txgainIdx[calPoint] = (A_UINT8)gainIndex; 
		eep->calPierData2G[pierIdx].calPerPoint[chain].power_t8[calPoint] = (A_UINT16)(power * 8); 
        EEPROM_CAL_INFO_ADD(calFreqPier2G[pierIdx]);
        EEPROM_CAL_INFO_ADD(calPierData2G[pierIdx].thermCalVal);
        EEPROM_CAL_INFO_ADD(calPierData2G[pierIdx].voltCalVal);
        EEPROM_CAL_INFO_ADD(calPierData2G[pierIdx].dacGain[calPoint]);
        EEPROM_CAL_INFO_ADD(calPierData2G[pierIdx].calPerPoint[chain].txgainIdx[calPoint]);
        EEPROM_CAL_INFO_ADD(calPierData2G[pierIdx].calPerPoint[chain].power_t8[calPoint]);
    }
    else 
    { /* 5GHz Freq pier */
        if(pierIdx >= WHAL_NUM_11A_CAL_PIERS)
        {
            UserPrint("Invalid 5GHz cal pier index, must be less than %d\n", WHAL_NUM_11A_CAL_PIERS);
            return -1;
        }
		eep->calFreqPier5G[pierIdx] = WHAL_FREQ2FBIN(freq,0);
		eep->calPierData5G[pierIdx].thermCalVal = tempMeas;
		eep->calPierData5G[pierIdx].voltCalVal = voltMeas; 
		eep->calPierData5G[pierIdx].dacGain[calPoint] = (A_INT8)dacGain; 
		eep->calPierData5G[pierIdx].calPerPoint[chain].txgainIdx[calPoint] = (A_UINT8)gainIndex; 
		eep->calPierData5G[pierIdx].calPerPoint[chain].power_t8[calPoint] = (A_UINT16)(power * 8); 
        EEPROM_CAL_INFO_ADD(calFreqPier5G[pierIdx]);
        EEPROM_CAL_INFO_ADD(calPierData5G[pierIdx].thermCalVal);
        EEPROM_CAL_INFO_ADD(calPierData5G[pierIdx].voltCalVal);
        EEPROM_CAL_INFO_ADD(calPierData5G[pierIdx].dacGain[calPoint]);
        EEPROM_CAL_INFO_ADD(calPierData5G[pierIdx].calPerPoint[chain].txgainIdx[calPoint]);
        EEPROM_CAL_INFO_ADD(calPierData5G[pierIdx].calPerPoint[chain].power_t8[calPoint]);
    }
    UserPrint("chan %d chain %d calPoint %d gainInx %d dacGain %d power %f pcorr %d  volt %d temp %d\n", 
               freq, chain, calPoint, gainIndex, dacGain, power, pwrCorrection, voltMeas, tempMeas);
    return 0;
}

int Qc98xxCalUnusedPierSet(int iChain, int iBand, int iIndex)
{
    int i;
    QC98XX_EEPROM *eep = Qc98xxEepromStructGet();

    //UserPrint("Qc98xxCalUnusedPierSet - iChain=%d; iBand=%d; iIndex=%d\n", iChain, iBand, iIndex);
    // mark all other channels as unused
    if (iBand == band_BG)
    {
        for (i = iIndex; i < WHAL_NUM_11G_CAL_PIERS; i++)
        {
            eep->calFreqPier2G[i] = WHAL_BCHAN_UNUSED;
            EEPROM_CAL_INFO_ADD(calFreqPier2G[i]);
        }
    }
    else
    {
        for (i = iIndex; i< WHAL_NUM_11A_CAL_PIERS; i++) 
        {
            eep->calFreqPier5G[i] = WHAL_BCHAN_UNUSED;
            EEPROM_CAL_INFO_ADD(calFreqPier5G[i]);
        }
    }
    return 0;
}

#if 0
void Qc98xxEepromPaPredistortionSet(int value)
{
	if(value)
	{
	    EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.featureEnable |= (1<<5);
	}
	else
	{
	    EEPROM_BASE_POINTER->baseEepHeader.opCapBrdFlags.featureEnable &= ~(1<<5);
	}
    EEPROM_CONFIG_DIFF_CHANGE(baseEepHeader.opCapBrdFlags.featureEnable);
}
#endif //0

int Qc98xxAlphaThermTableSet(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, j, k, j0, k0, iv=0;
	int jMax;
	
	jMax = (iBand==band_BG) ? QC98XX_NUM_ALPHATHERM_CHANS_2G : QC98XX_NUM_ALPHATHERM_CHANS_5G;

    for (i=ix; i<WHAL_NUM_CHAINS; i++) 
	{
		if (iv>=num)
		{
			break;
		}
		j0 = (i==ix) ? iy : 0;	
		
		for (j=j0; j<jMax; j++) 
		{
			if (iv>=num)
			{
				break;
			}
			k0 = (j==iy) ? iz : 0;
			
			for (k=k0; k < QC98XX_NUM_ALPHATHERM_TEMPS; k++)
			{
				if (iBand==band_BG)
				{
					EEPROM_BASE_POINTER->tempComp2G.alphaThermTbl[i][j][k] = (A_UINT8)value[iv++];
				}
				else
				{
					EEPROM_BASE_POINTER->tempComp5G.alphaThermTbl[i][j][k] = (A_UINT8)value[iv++];
				}
				if (iv>=num)
				{
					break;
				}
			}
		}
	}
    if (iBand==band_BG)
    {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(tempComp2G.alphaThermTbl[ix][iy][iz], iv);
	} 
    else 
    {
        EEPROM_CONFIG_DIFF_CHANGE_ARRAY(tempComp5G.alphaThermTbl[ix][iy][iz], iv);
	}
    return VALUE_OK;
}

int Qc98xxConfigAddrSet1(int *value, int ix, int iy, int iz, int num, int iBand)
{
	int i, iv=0;

	for (i=ix; i<QC98XX_CONFIG_ENTRIES; i++) 
	{
		if (iv>=num) break;
		EEPROM_BASE_POINTER->configAddr[i] = (A_UINT8)(value[iv++]);
	}
    EEPROM_CONFIG_DIFF_CHANGE_ARRAY(configAddr[ix], iv);
    return VALUE_OK;
}

int Qc98xxConfigAddrSet(unsigned int address, int low, int  high, unsigned int *value, int numVal, int prepost)
{
    int i, maxLen, entryLen;
    A_UINT32 mode, addressMode;
    A_UINT32 *pConfigAddr;
    A_UINT32 curEntryLen;
    QC98XX_EEPROM *eep = Qc98xxEepromStructGet();

    if (low != 0 && high != 31)
    {
        UserPrint("Qc98xxConfigAddrSet - field sticky write is not allowed in configAddr space\n");
        return -1;
    }
    if (numVal != 1 && numVal != 2 && numVal != 5)
    {
        UserPrint("Qc98xxConfigAddrSet - There must be 1 2 or 5 values\n");
        return -1;
    }

    maxLen = sizeof(eep->configAddr);
    mode = (numVal == 5) ? OTP_CONFIG_MODE_5MODAL : ((numVal == 2) ? OTP_CONFIG_MODE_2MODAL : OTP_CONFIG_MODE_COMMON);
    addressMode = address | (mode << CONFIG_ADDR_MODE_SHIFT) | (prepost << CONFIG_ADDR_CTRL_SHIFT);
    entryLen = GET_LENGTH_CONFIG_ENTRY_32B(mode);

    // Check for any duplicate of the input in the eeprom area
    // and find empty space to write this entry
    for  (i = 0, pConfigAddr = eep->configAddr; (i < maxLen) && (pConfigAddr[i] != 0); i+=curEntryLen)
    {
        curEntryLen = GET_LENGTH_CONFIG_ENTRY_32B((pConfigAddr[i] & CONFIG_ADDR_MODE_MASK) >> CONFIG_ADDR_MODE_SHIFT);
        if (pConfigAddr[i] == addressMode)
        {
            if (memcmp(&pConfigAddr[i+1], value, sizeof(A_UINT32)*numVal) == 0)
            {
                // an exact ebtry is already in eeprom area, dont need to add, return
                return 0;
            }
        }
    }

    if (i + entryLen >= maxLen )
    {
        UserPrint("Qc98xxConfigAddrSet - no more space in EEPROM configAddr\n");
        return -1;
    }
    pConfigAddr[i] = addressMode;
    pConfigAddr += (i+1);
    for (i = 0; i < (entryLen-1); ++i)
    {
        pConfigAddr[i] = value[i];
    }
    return 0;
}

