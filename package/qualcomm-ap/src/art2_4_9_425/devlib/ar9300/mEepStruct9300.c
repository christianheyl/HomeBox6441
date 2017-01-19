#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "AquilaNewmaMapping.h"

#include "wlantype.h"
#include "Interpolate.h"
#include "mCal9300.h"
#include "Ar9300Device.h"

#include "UserPrint.h"
#include "Field.h"

#ifdef UNUSED

#include "default9300.h"

//static OSPREY_EEPROM default9300;
OSPREY_EEPROM currentCard9300;

#endif

//
// hal header files
//
#include "ah.h"
#include "ah_internal.h"
#include "ar9300eep.h"
#include "Ar9300EepromStructSet.h"
#include "ConfigurationStatus.h"
#include "ar9300reg.h"
#include "ah_devid.h"

#include "ar9300Eeprom_switchcomspdt.h"
#include "ar9300Eeprom_xLNA.h"
#include "ar9300Eeprom_tempslopextension.h"
#include "ar9300Eeprom_rfGainCap.h"
#include "ar9300Eeprom_txGainCap.h"


AR9300DLLSPEC int Ar9300CalibrationPierSet(int pierIdx, int freq, int chain, 
                          int pwrCorrection, int volt_meas, int temp_meas)
{
    A_UINT8 *pCalPier;
    OSP_CAL_DATA_PER_FREQ_OP_LOOP *pCalPierStruct;
	int is2G;

    if(chain >= OSPREY_MAX_CHAINS) {
        UserPrint("Invalid chain index, must be less than %d\n", OSPREY_MAX_CHAINS);
        return -1;
    }

    if(freq < 3000) { /* 2GHz frequency pier */
        if(pierIdx >= OSPREY_NUM_2G_CAL_PIERS){
            UserPrint("Invalid 2GHz cal pier index, must be less than %d\n", OSPREY_NUM_2G_CAL_PIERS);
            return -1;
        }
		is2G=1;
        pCalPier = &(Ar9300EepromStructGet()->cal_freq_pier_2g[pierIdx]);
        pCalPierStruct = &(Ar9300EepromStructGet()->cal_pier_data_2g[chain][pierIdx]);
    }
    else { /* 5GHz Freq pier */
        if(pierIdx >= OSPREY_NUM_5G_CAL_PIERS){
            UserPrint("Invalid 5GHz cal pier index, must be less than %d\n", OSPREY_NUM_5G_CAL_PIERS);
            return -1;
        }
		is2G=0;
        pCalPier = &(Ar9300EepromStructGet()->cal_freq_pier_5g[pierIdx]);
        pCalPierStruct = &(Ar9300EepromStructGet()->cal_pier_data_5g[chain][pierIdx]);
    }

    *pCalPier = FREQ2FBIN(freq,is2G);
    pCalPierStruct->ref_power = pwrCorrection;
    pCalPierStruct->temp_meas = temp_meas;
    pCalPierStruct->volt_meas = volt_meas; 
    return 0;
}

AR9300DLLSPEC int Ar9300CalInfoCalibrationPierSet(int pier, int frequency, int chain, 
				int gain, int gainIndex, int dacGain, double power, 
				int correction, int voltage, int temperature, int calPoint)
{
	Ar9300CalibrationPierSet(pier, frequency, chain, 
                          correction, voltage, temperature);
	return 0;
}

int Ar9300eepromVersionGet()
{
	A_UINT8  value;
	value = (u_int8_t) Ar9300EepromStructGet()->eeprom_version;
    return value; 
}
int Ar9300templateVersionGet()
{
	A_UINT8  value;
	value = (u_int8_t) Ar9300EepromStructGet()->template_version;
    return value; 
}
int Ar9300FutureGet(int *value, int ix, int *num, int iBand)
{
	int i, iv=0, max=MAX_MODAL_FUTURE;
	if (iBand==-1)
		max=MAX_BASE_EXTENSION_FUTURE;
	if (ix<0 || ix>=max) {
		for (i=0; i<max; i++) {
			if (iBand==band_BG)
				value[iv++] =  (A_UINT8)Ar9300EepromStructGet()->modal_header_2g.futureModal[i];
			else if (iBand==band_A)
				value[iv++] =  (A_UINT8)Ar9300EepromStructGet()->modal_header_5g.futureModal[i];
			else
				value[iv++] =  (A_UINT8)Ar9300EepromStructGet()->base_ext1.future[i];
			*num=max;
		}
	} else {
			if (iBand==band_BG)
				value[0] =  (A_UINT8)Ar9300EepromStructGet()->modal_header_2g.futureModal[ix];
			else if (iBand==band_A)
				value[0] =  (A_UINT8)Ar9300EepromStructGet()->modal_header_5g.futureModal[ix];
			else
				value[0] =  (A_UINT8)Ar9300EepromStructGet()->base_ext1.future[ix];
		*num=1;
	}
    return 0; 
}

int Ar9300AntDivCtrlGet(void)
{
	A_UINT8  value;
	value = Ar9300EepromStructGet()->base_ext1.ant_div_control;
    return value; 
}

/*
 *Function Name:Ar9300pwrTuningCapsParamsGet
 *Parameters: returned string value for get 2 uint8 value delim with ,
 *Description: Get TuningCapsParams values from field of eeprom struct 2 uint8
 *Returns: zero
 */
int Ar9300pwrTuningCapsParamsGet(int *value, int ix, int *num)
{
	int i, iv=0;
	if (ix<0 || ix>2) {
		value[0] = Ar9300EepromStructGet()->base_eep_header.params_for_tuning_caps[0];
		value[1] = Ar9300EepromStructGet()->base_eep_header.params_for_tuning_caps[1];
		*num = 2;
	} else {
		value[0] = Ar9300EepromStructGet()->base_eep_header.params_for_tuning_caps[ix];
		*num = 1;
	}
    return VALUE_OK; 
}
/*
 *Function Name:Ar9300regDmnGet
 *Parameters: returned string value for get value
 *Description: Get reg_dmn value from field of eeprom struct uint16*2 
 *Returns: zero
 */
int Ar9300regDmnGet(int *value, int ix, int *num)
{
	int i, iv=0;
	if (ix<0 || ix>2) {
		value[0] = Ar9300EepromStructGet()->base_eep_header.reg_dmn[0];
		value[1] = Ar9300EepromStructGet()->base_eep_header.reg_dmn[1];
		*num = 2;
	} else {
		value[0] = Ar9300EepromStructGet()->base_eep_header.reg_dmn[ix];
		*num = 1;
	}
    return VALUE_OK; 
}


AR9300DLLSPEC int Ar9300RegulatoryDomainGet(void)
{
	return Ar9300EepromStructGet()->base_eep_header.reg_dmn[0];
}


AR9300DLLSPEC int Ar9300RegulatoryDomainOverride(unsigned int regdmn)
{
	Ar9300EepromStructGet()->base_eep_header.reg_dmn[0]=regdmn;
	return 0;
}

AR9300DLLSPEC int Ar9300NoiseFloorGet(int frequency, int ichain)
{
	return ar9300NoiseFloorGet(AH, frequency, ichain);
}

AR9300DLLSPEC int Ar9300NoiseFloorPowerGet(int frequency, int ichain)
{
	return ar9300NoiseFloorPowerGet(AH, frequency, ichain);
}

/*
 *Function Name:Ar9300txMaskGet
 *Parameters: returned string value for get value
 *Description: Get txMask value from field of eeprom struct uint8 (up 4 bit???)
 *Returns: zero
 */
int Ar9300txrxMaskGet(void)
{
	A_UINT8  value;
	value = Ar9300EepromStructGet()->base_eep_header.txrx_mask;
    return value; 
}

/*
 *Function Name:Ar9300txMaskGet
 *Parameters: returned string value for get value
 *Description: Get txMask value from field of eeprom struct uint8 (up 4 bit???)
 *Returns: zero
 */
int Ar9300txMaskGet(void)
{
	A_UINT8  value;
	value = (Ar9300EepromStructGet()->base_eep_header.txrx_mask & 0xf0) >> 4;
    return value; 
}
/*
 *Function Name:Ar9300rxMaskGet
 *Parameters: returned string value for get value
 *Description: Get rxMask value from field of eeprom struct uint8 (low 4bits??)
 *Returns: zero
 */
int Ar9300rxMaskGet(void)
{
	A_UINT8  value;
	value = (Ar9300EepromStructGet()->base_eep_header.txrx_mask & 0x0f);
    return value; 
}
/*
 *Function Name:Ar9300opFlagsGet
 *Parameters: returned string value for get value
 *Description: Get op_flags value from field of eeprom struct uint8
 *Returns: zero
 */
int Ar9300opFlagsGet(void)
{
	A_UINT8  value;
	value = Ar9300EepromStructGet()->base_eep_header.op_cap_flags.op_flags;
    return value; 
}/*
 *Function Name:Ar9300eepMiscGet
 *Parameters: returned string value for get value
 *Description: Get eepMisc value from field of eeprom struct uint8
 *Returns: zero
 */
int Ar9300eepMiscGet(void)
{
	A_UINT8  value;
	value = Ar9300EepromStructGet()->base_eep_header.op_cap_flags.eepMisc;
    return value; 
}/*
 *Function Name:Ar9300rf_silentGet
 *Parameters: returned string value for get value
 *Description: Get rf_silent value from field of eeprom struct uint8
 *Returns: zero
 */
int Ar9300rfSilentGet(void)
{
	A_UINT8  value;
	value = Ar9300EepromStructGet()->base_eep_header.rf_silent;
    return value; 
}
int Ar9300rfSilentB0Get(void)
{
	A_UINT8  value;
	value = Ar9300EepromStructGet()->base_eep_header.rf_silent & 0x01;
    return value; 
}
int Ar9300rfSilentB1Get(void)
{
	A_UINT8  value;
	value = Ar9300EepromStructGet()->base_eep_header.rf_silent >>1;
    return value; 
}
int Ar9300rfSilentGPIOGet(void)
{
	A_UINT8  value;
	value = Ar9300EepromStructGet()->base_eep_header.rf_silent>>2;
    return value; 
}
/*
 *Function Name:Ar9300blueToothOptionsGet
 *Parameters: returned string value for get value
 *Description: Get blue_tooth_options value from field of eeprom struct uint8
 *Returns: zero
 */
int Ar9300blueToothOptionsGet(void)
{
	A_UINT8  value;
	value = Ar9300EepromStructGet()->base_eep_header.blue_tooth_options;
    return value; 
}/*
 *Function Name:Ar9300deviceCapGet
 *Parameters: returned string value for get value
 *Description: Get device_cap value from field of eeprom struct uint8
 *Returns: zero
 */
int Ar9300deviceCapGet(void)
{
	A_UINT8  value;
	value = Ar9300EepromStructGet()->base_eep_header.device_cap;
    return value; 
}/*
 *Function Name:Ar9300deviceTypeGet
 *Parameters: returned string value for get value
 *Description: Get device_type value from field of eeprom struct uint8
 *Returns: zero
 */
int Ar9300deviceTypeGet(void)
{
	A_UINT8  value;
	value = Ar9300EepromStructGet()->base_eep_header.device_type;
    return value; 
}
/*
 *Function Name:Ar9300pwrTableOffsetGet
 *Parameters: returned string value for get value
 *Description: Get pwrTableOffset value from field of eeprom struct int8
 *Returns: zero
 */
int Ar9300pwrTableOffsetGet(void)
{
	A_INT8  value;
	value = Ar9300EepromStructGet()->base_eep_header.pwrTableOffset;
    return value; 
}

/*
 *Function Name:Ar9300TempSlopeGet
 *Parameters: returned string value for get value
 *Description: Get TempSlope value from field of eeprom struct int8
 *Returns: zero
 */
int Ar9300TempSlopeGet(int *value, int iBand)
{
    	if (iBand==band_BG) {

		if(!AR_SREV_SCORPION(AH)) {
			value[0] = Ar9300EepromStructGet()->modal_header_2g.temp_slope;
		} else {
			/*Scorpion has per chain tempslope registers*/
			 value[0] = Ar9300EepromStructGet()->base_ext2.temp_slope_low;
			 value[1] = Ar9300EepromStructGet()->modal_header_2g.temp_slope;
			 value[2] = Ar9300EepromStructGet()->base_ext2.temp_slope_high;
			 
		}
	} else {

		if(!AR_SREV_SCORPION(AH)) {
			value[0] = Ar9300EepromStructGet()->modal_header_5g.temp_slope;
	        } else {
			/*Scorpion has per chain tempslope registers*/
			value[0] = Ar9300EepromStructGet()->modal_header_5g.temp_slope;
			value[1] = Ar9300EepromStructGet()->base_ext1.tempslopextension[0];	
			value[2] = Ar9300EepromStructGet()->base_ext1.tempslopextension[1];
		}
	}
	return VALUE_OK; 
}
int Ar9300TempSlopeLowGet(int *value)
{
	if(!AR_SREV_SCORPION(AH)) {
		value[0] = Ar9300EepromStructGet()->base_ext2.temp_slope_low;
	} else {
		/*Scorpion has per chain tempslope registers*/
		value[0] = Ar9300EepromStructGet()->base_ext1.tempslopextension[2];
		value[1] = Ar9300EepromStructGet()->base_ext1.tempslopextension[3];
		value[2] = Ar9300EepromStructGet()->base_ext1.tempslopextension[4];
	}
    return VALUE_OK; 
}
int Ar9300TempSlopeHighGet(int *value)
{
	if(!AR_SREV_SCORPION(AH)) {
		value[0] = Ar9300EepromStructGet()->base_ext2.temp_slope_high;
 	} else {
		/*Scorpion has per chain tempslope registers*/
		value[0] = Ar9300EepromStructGet()->base_ext1.tempslopextension[5];
		value[1] = Ar9300EepromStructGet()->base_ext1.tempslopextension[6];
		value[2] = Ar9300EepromStructGet()->base_ext1.tempslopextension[7];
	}
	return VALUE_OK; 
}

/*
 *Function Name:Ar9300TempSlopeExtensionGet
 *Description: Get extension set of TempSlope value from field of eeprom struct int8
 *Returns: zero
 */
int Ar9300TempSlopeExtensionGet(int *value, int ix, int *num)
{
	ar9300_eeprom_t *ahp_Eeprom;
	ahp_Eeprom = Ar9300EepromStructGet();

	Ar9300Eeprom_tempslopeextensionGet(ahp_Eeprom, value, ix, num);
    return VALUE_OK; 
}

/*
 *Function Name:Ar9300VoltSlopeGet
 *Parameters: returned string value for get value
 *Description: Get VoltSlope value from field of eeprom struct int8
 *Returns: zero
 */
int Ar9300VoltSlopeGet(int iBand)
{
	A_INT8  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.voltSlope;
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.voltSlope;
	}	
    return value; 
}

/*
 *Function Name:Ar9300ReconfigMiscGet
 *Description: Get miscConfiguration all bits
 */
int Ar9300ReconfigMiscGet(void)
{
	A_INT32  bit;
	bit = Ar9300EepromStructGet()->base_eep_header.misc_configuration;
    return bit; 
}
/*
 *Function Name:Ar9300reconfigDriveStrengthGet
 *Description: Get reconfigDriveStrength flag from misc_configuration 
 *             field of eeprom struct (bit 0)
 */
int Ar9300ReconfigDriveStrengthGet(void)
{
	A_INT32  bit;
	bit = Ar9300EepromStructGet()->base_eep_header.misc_configuration & 0x01;
    return bit; 
}
// bit 4 - enable quick drop
int Ar9300ReconfigQuickDropGet(void)
{
	A_INT32  bit;
	bit = Ar9300EepromStructGet()->base_eep_header.misc_configuration >>4;
    return bit; 
}
// bit 5 - enable 8 temp
int Ar9300Reconfig8TempGet(void)
{
	A_INT32  bit;
	bit = Ar9300EepromStructGet()->base_eep_header.misc_configuration >>5;
    return bit; 
}
/*
 *Function Name:Ar9300TxGainGet
 *Parameters: returned string value for get bit
 *Description: Get TxGain flag in txrxgain field of eeprom struct's upper 4bits
 *Returns: zero
 */
int Ar9300TxGainGet(void)
{
	A_UINT8  value;
	value = (Ar9300EepromStructGet()->base_eep_header.txrxgain  & 0xf0) >> 4;
    return value; 
}
/*
 *Function Name:Ar9300RxGainGet
 *Parameters: returned string value for get bit
 *Description: Get RxGain flag in txrxgain field of eeprom struct's upper 4bits
 *Returns: zero
 */
int Ar9300RxGainGet(void)
{
	A_UINT8  value;
	value = Ar9300EepromStructGet()->base_eep_header.txrxgain  & 0x0f;
    return value; 
}

/*
 *Function Name:Ar9300EnableFeatureGet
 *Description: get all featureEnable 8bits
 *             field of eeprom struct (bit 0)
 * enable tx temp comp 
 */
int Ar9300EnableFeatureGet(void)
{
	int value;
    value = Ar9300EepromStructGet()->base_eep_header.feature_enable;
    return value; 
}
/*
 *Function Name:Ar9300EnableTempCompensationGet
 *Description: get EnableTempCompensation flag from featureEnable 
 *             field of eeprom struct (bit 0)
 * enable tx temp comp 
 */
int Ar9300EnableTempCompensationGet(void)
{
	int value;
    value = Ar9300EepromStructGet()->base_eep_header.feature_enable & 0x01;
    return value; 
}

/*
 *Function Name:Ar9300EnableVoltCompensationGet
 *Description: get rEnableVoltCompensation flag from featureEnable 
 *             field of eeprom struct (bit 1)
 * enable tx volt comp
 */
int Ar9300EnableVoltCompensationGet(void)
{
	int value;
    value = (Ar9300EepromStructGet()->base_eep_header.feature_enable & 0x02) >> 1;
    return value; 
}

/*
 *Function Name:Ar9300EnableFastClockGet
 *Description: Get reconfigDriveStrength flag from feature_enable 
 *             field of eeprom struct (bit 2)
 * enable fastClock - default to 1
 */
int Ar9300EnableFastClockGet(void)
{
	int value;
    value = (Ar9300EepromStructGet()->base_eep_header.feature_enable & 0x04) >> 2;
    return value; 
}

/*
 *Function Name:Ar9300EnableDoublingGet
 *Description: get EnableDoubling flag from featureEnable 
 *             field of eeprom struct (bit 3)
 * enable doubling - default to 1
 */
int Ar9300EnableDoublingGet(void)
{
	int value;
    value = (Ar9300EepromStructGet()->base_eep_header.feature_enable & 0x08) >> 3;
    return value; 
}

/*
 *Function Name:Ar9300EnableTuningCapsGet
 *Description: get EnableTuningCaps flag from featureEnable 
 *             field of eeprom struct (bit 6)
 * enable TuningCaps - default to 0
 */
int Ar9300EnableTuningCapsGet(void)
{
	int value;
    value = (Ar9300EepromStructGet()->base_eep_header.feature_enable & 0x40) >> 6;
    return value; 
}

/*
 *Function Name:Ar9300EnableTxFrameToXpaOnGet
 *Description: get EnableTxFrameToXpaOn flag from featureEnable 
 *             field of eeprom struct (bit 7)
 * enable TxFrameToXpaOn - default to 0
 */
int Ar9300EnableTxFrameToXpaOnGet(void)
{
	int value;
    value = (Ar9300EepromStructGet()->base_eep_header.feature_enable & 0x80) >> 7;
    return value; 
}

/*
 *Function Name:Ar9300EnableXLNABiasStrengthGet
 *Description: get EnableXLNABiasStrength flag from misc_configuration 
 *             field of eeprom struct (bit 6)
 * enable XLNABiasStrength - default to 0
 */
int Ar9300EnableXLNABiasStrengthGet(void)
{
	int value;
	value = (Ar9300EepromStructGet()->base_eep_header.misc_configuration & 0x40) >> 6;
    return value; 
}

/*
 *Function Name:Ar9300EnableRFGainCAPGet
 *Description: get EnableRFGainCAP flag from misc_configuration 
 *             field of eeprom struct (bit 7)
 * enable RFGainCAP - default to 0
 */
int Ar9300EnableRFGainCAPGet(void)
{
	int value;
	value = (Ar9300EepromStructGet()->base_eep_header.misc_configuration & 0x80) >> 7;
    return value; 
}


/*
 *Function Name:Ar9300EnableMinCCAPwrThresholdGet
 *Description: get EnableMinCCAPwr flag from misc_enable 
 *             field of eeprom struct (bit 2 & 3)
 * enable EnableMinCCAPwr (bit 2 for 2G and bit 3 for 5G)
 */
int Ar9300EnableMinCCAPwrThresholdGet(void)
{
	int value;
	value = ((Ar9300EepromStructGet()->base_ext1.misc_enable & 0xc) >> 2) & 0x3;
    return value; 
}

/*
 *Function Name:Ar9300EnableTXGainCAPGet
 *Description: get EnableTXGainCAP flag from misc_enable 
 *             field of eeprom struct (bit 0)
 * enable TXGainCAP - default to 0
 */
int Ar9300EnableTXGainCAPGet(void)
{
	A_UINT32  value=0;
	ar9300_eeprom_t *ahp_Eeprom;
		ahp_Eeprom = Ar9300EepromStructGet();

	value=Ar9300Eeprom_txGainCapEnableGet(ahp_Eeprom);
    return value; 
}

/*
 *Function Name:Ar9300InternalRegulatorGet
 *Description: get internal regulator flag from feature_enable 
 *             field of eeprom struct (bit 4)
 * enable internal regulator - default to 1
 */
int Ar9300InternalRegulatorGet()
{
	int value;
    value = (Ar9300EepromStructGet()->base_eep_header.feature_enable & 0x10) >> 4;
    return value; 
}

/*
 *Function Name:Ar9300PapdGet
 *Description: get PA predistortion enable flag from feature_enable 
 *             field of eeprom struct (bit 5)
 * enable paprd - default to 0 
 */
int Ar9300PapdGet(void)
{
	int value;
    value = (Ar9300EepromStructGet()->base_eep_header.feature_enable & 0x20) >> 5;
    return value; 
}

int Ar9300PapdRateMaskHt20Get(int iBand)
{
	u_int32_t  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.paprd_rate_mask_ht20;
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.paprd_rate_mask_ht20;
	}
    return value; 
}

int Ar9300PapdRateMaskHt40Get(int iBand)
{
	u_int32_t  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.paprd_rate_mask_ht40;
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.paprd_rate_mask_ht40;
	}
    return value; 
}

/*
 *Function Name:Ar9300WlanSpdtSwitchGlobalControlGet
 *Parameters: returned string value for get value
 *Description: Get switchcomspdt value from field of eeprom struct u_int32
 *Returns: zero
 */
int Ar9300WlanSpdtSwitchGlobalControlGet(int iBand)
{
	A_UINT32  value=0;
	ar9300_eeprom_t *ahp_Eeprom;
		ahp_Eeprom = Ar9300EepromStructGet();

	value=Ar9300Eeprom_switchcomspdtGet(ahp_Eeprom, iBand);
    return value; 
}

int Ar9300XLANBiasStrengthGet(int *value, int ix, int *num, int iBand)
{
	int i, iv=0, iCMaxChain=OSPREY_MAX_CHAINS;
	int val;
	ar9300_eeprom_t *ahp_Eeprom;
		ahp_Eeprom = Ar9300EepromStructGet();
	// bit0,1 for chain0, bit2,3 for chain1, bit4,5 for chain2
	val = Ar9300Eeprom_xLNABiasStrengthGet(ahp_Eeprom, iBand);

	if (ix<0 || ix>=iCMaxChain) {
		value[0] = val&(0x3);
		value[1] = (val&(3<<2))>>2;
		value[2] = (val&(3<<4))>>4;
		*num = iCMaxChain;
	} else {
		iv = (ix*2);
		value[0] = (val&(3<<iv)) >> iv;
		*num = 1;
	}
    return VALUE_OK; 
}

int Ar9300RFGainCAPGet(int iBand)
{
	A_UINT32  value=0;
	ar9300_eeprom_t *ahp_Eeprom;
		ahp_Eeprom = Ar9300EepromStructGet();

	value=Ar9300Eeprom_rfGainCapGet(ahp_Eeprom, iBand);
    return value; 
}

int Ar9300TXGainCAPGet(int iBand)
{
	A_UINT32  value=0;
	ar9300_eeprom_t *ahp_Eeprom;
		ahp_Eeprom = Ar9300EepromStructGet();

	value=Ar9300Eeprom_txGainCapGet(ahp_Eeprom, iBand);
    return value; 
}

int Ar9300_SWREG_Get(void)
{
	return Ar9300EepromStructGet()->base_eep_header.swreg;
/*
	int ngot;
	unsigned int address;
	int low, high;
	int status=VALUE_OK;
	char regName[100];
	A_UINT32 mask, reg;

	sprintf(regName, "REG_CONTROL0.%s",sValue); 
	if (strcmp(sValue, "swreg")==0) 
		sprintf(regName, "REG_CONTROL0.swreg_pwd"); 
	ngot=FieldFind(regName,&address,&low,&high);
	if (ngot==1) {
		mask = MaskCreate(low, high);
		reg = Ar9300EepromStructGet()->base_eep_header.swreg; 
		if (strcmp(sValue, "swreg")!=0) { 
			reg &= mask;
			reg = reg>>low;
		}
		sprintf(sValue, "0x%x", reg);
	} else {
		status = ERR_VALUE_BAD;
		sprintf(sValue, "Can't find reg name: %s", regName);
	}

    return status; */
}

/*
 *Function Name:Ar9300MacAdressGet
 *
 *Parameters: mac -- pointer to output pointer
 *
 *Description: Returns MAC address from eeprom structure.
 *
 *Returns: zero
 *
 */



AR9300DLLSPEC A_INT32 Ar9300MacAddressGet(A_UCHAR *data)
{
    A_INT16 i;

	for(i=0; i<6; i++)
        data[i] = Ar9300EepromStructGet()->mac_addr[i];

    return 0;
}

AR9300DLLSPEC A_INT32 Ar9300CustomerDataGet(A_UCHAR *data, A_INT32 maxlength)
{
    int i;
	int length;

	if(maxlength>OSPREY_CUSTOMER_DATA_SIZE) 
	{
        length=OSPREY_CUSTOMER_DATA_SIZE;
    }
	else
	{
		length=maxlength;
	}

    for(i=0; i<length; i++)
	{
        data[i] = Ar9300EepromStructGet()->custData[i];
	}
	for(i=length; i<maxlength; i++)
	{
		data[i]=0;
	}

    return 0;
}
AR9300DLLSPEC A_INT32 Ar9300CaldataMemoryTypeGet(A_UCHAR *memType, A_INT32 maxlength)
{
    switch(ar9300_calibration_data_get(AH))
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
/*
A_INT32 Ar9300CalTgtPwrGet(int *pwrArr, int band, int htMode, int iFreqNum)
{
	int i, j;
	if (band==band_BG) {
		if (htMode==legacy_CCK) {
			for (i=0; i<NUM_CALTGT_FREQ_CCK;i++) {
				if (iFreqNum ==i) {
					for (j=0; j<NUM_TGT_DATARATE_LEGACY; j++) 
						pwrArr[j] = (int)(Ar9300EepromStructGet()->cal_target_power_cck[i].t_pow2x[j]/2);
					break;
				}
			}
		} else {
			for (i=0; i<NUM_CALTGT_FREQ_2G;i++) {
				if (iFreqNum ==i) {
					if (htMode==legacy_OFDM) {
						for (j=0; j<NUM_TGT_DATARATE_LEGACY; j++) {
							pwrArr[j] = (int)(Ar9300EepromStructGet()->cal_target_power_2g[i].t_pow2x[j]/2);
						}
					} else if (htMode==HT20) {
						for (j=0; j<NUM_TGT_DATARATE_HT; j++)  {
							pwrArr[j] = (int)(Ar9300EepromStructGet()->cal_target_power_2g_ht20[i].t_pow2x[j]/2);
						}
					} else if (htMode==HT40) {
						for (j=0; j<NUM_TGT_DATARATE_HT; j++)  {
							pwrArr[j] = (int)(Ar9300EepromStructGet()->cal_target_power_2g_ht40[i].t_pow2x[j]/2);
						}
					}
					break;
				}
			}
		}
	} 

	if (band==band_A) {
		for (i=0; i<NUM_CALTGT_FREQ_5G;i++) {
			if (iFreqNum ==i) {
				if (htMode==legacy_OFDM) {
					for (j=0; j<NUM_TGT_DATARATE_LEGACY; j++) {
//						tmp = (int)Ar9300EepromStructGet()->cal_target_power_5g[i].t_pow2x[j];
						pwrArr[j] = (int)(Ar9300EepromStructGet()->cal_target_power_5g[i].t_pow2x[j]/2);
					}
				} else if (htMode==HT20) {
					for (j=0; j<NUM_TGT_DATARATE_HT; j++) {
						pwrArr[j] = (int)(Ar9300EepromStructGet()->cal_target_power_5g_ht20[i].t_pow2x[j]/2);
					}
				} else if (htMode==HT40) {
					for (j=0; j<NUM_TGT_DATARATE_HT; j++) {
						pwrArr[j] = (int)(Ar9300EepromStructGet()->cal_target_power_5g_ht40[i].t_pow2x[j]/2);
					}
				}
				break;
			}
		}
	}
	return 0;
}
*/
int setFBIN2FREQ(int bin, int iBand)
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

A_INT32 Ar9300antCtrlChainGet(int *value, int ix, int *num, int iBand)
{
	int i, iv=0, iCMaxChain=OSPREY_MAX_CHAINS;
	int val;
	if (ix<0 || ix>iCMaxChain) {
		for (i=0; i<iCMaxChain; i++) {
			if (iBand==band_BG) {
				val = Ar9300EepromStructGet()->modal_header_2g.ant_ctrl_chain[i];
			} else {
				val = Ar9300EepromStructGet()->modal_header_5g.ant_ctrl_chain[i];
			}
			value[i] = val;
			

		}
		*num = iCMaxChain;
	} else {
		if (iBand==band_BG) {
			val = Ar9300EepromStructGet()->modal_header_2g.ant_ctrl_chain[ix];
		} else {
			val = Ar9300EepromStructGet()->modal_header_5g.ant_ctrl_chain[ix];
		}
		value[0] = val;
		*num = 1;
	}
    return VALUE_OK; 
}
/*
 *Function Name:Ar9300AntCtrlCommonGet
 *Parameters: returned string value for get value
 *Description: Get AntCtrlCommon value from field of eeprom struct u_int32
 *Returns: zero
 */
int Ar9300AntCtrlCommonGet(int iBand)
{
	A_UINT32  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.ant_ctrl_common;
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.ant_ctrl_common;
	}	
    return value; 
}
/*
 *Function Name:Ar9300AntCtrlCommon2Get
 *Parameters: returned string value for get value
 *Description: Get AntCtrlCommon2 value from field of eeprom struct u_int32
 *Returns: zero
 */
int Ar9300AntCtrlCommon2Get(int iBand)
{
	A_UINT32  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.ant_ctrl_common2;
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.ant_ctrl_common2;
	}	
    return value; 
}


/*
 *Function Name:Ar9300xatten1DBGet
 *Parameters: returned string value for get value
 *Description: Get xatten1DB flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (u_int8_t) 
 *Returns: zero
 */
A_INT32 Ar9300xatten1DBGet(int *value, int ix, int *num, int iBand)
{
	int i, iv=0, iCMaxChain=OSPREY_MAX_CHAINS;
	int val;
	if (ix<0 || ix>iCMaxChain) {
		for (i=0; i<iCMaxChain; i++) {
			if (iBand==band_BG) {
				val = Ar9300EepromStructGet()->modal_header_2g.xatten1_db[i];
			} else {
				val = Ar9300EepromStructGet()->modal_header_5g.xatten1_db[i];
			}
			value[i] = val;
		}
		*num = iCMaxChain;
	} else {
		if (iBand==band_BG) {
			val = Ar9300EepromStructGet()->modal_header_2g.xatten1_db[ix];
		} else {
			val = Ar9300EepromStructGet()->modal_header_5g.xatten1_db[ix];
		}
		value[0] = val;
		*num = 1;
	}
    return VALUE_OK; 
} 

A_INT32 Ar9300xatten1DBLowGet(int *value, int ix, int *num, int iBand)
{
	int i, iv=0, iCMaxChain=OSPREY_MAX_CHAINS;
	int val;
	if (iBand==band_BG) {
		*num=0;
		return 0;
	}
	if (ix<0 || ix>iCMaxChain) {
		for (i=0; i<iCMaxChain; i++) {
			value[i] = Ar9300EepromStructGet()->base_ext2.xatten1_db_low[i];
		}
		*num = iCMaxChain;
	} else {
		value[0] = Ar9300EepromStructGet()->base_ext2.xatten1_db_low[ix];
		*num = 1;
	}
    return VALUE_OK; 
}

A_INT32 Ar9300xatten1DBHighGet(int *value, int ix, int *num, int iBand)
{
	int i, iv=0, iCMaxChain=OSPREY_MAX_CHAINS;
	int val;
	if (iBand==band_BG) {
		*num=0;
		return 0;
	}
	if (ix<0 || ix>iCMaxChain) {
		for (i=0; i<iCMaxChain; i++) {
			value[i] = Ar9300EepromStructGet()->base_ext2.xatten1_db_high[i];
		}
		*num = iCMaxChain;
	} else {
		value[0] = Ar9300EepromStructGet()->base_ext2.xatten1_db_high[ix];
		*num = 1;
	}
    return VALUE_OK; 
}


/*
 *Function Name:Ar9300xatten1MarginGet
 *Parameters: returned string value for get value
 *Description: Get xatten1Margin flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (u_int8_t) 
 *Returns: zero
 */
A_INT32 Ar9300xatten1MarginGet(int *value, int ix, int *num, int iBand)
{
	int i, iv=0, iCMaxChain=OSPREY_MAX_CHAINS;
	int val;
	if (ix<0 || ix>iCMaxChain) {
		for (i=0; i<iCMaxChain; i++) {
			if (iBand==band_BG) {
				val = Ar9300EepromStructGet()->modal_header_2g.xatten1_margin[i];
			} else {
				val = Ar9300EepromStructGet()->modal_header_5g.xatten1_margin[i];
			}
			value[i] = val;
		}
		*num = iCMaxChain;
	} else {
		if (iBand==band_BG) {
			val = Ar9300EepromStructGet()->modal_header_2g.xatten1_margin[ix];
		} else {
			val = Ar9300EepromStructGet()->modal_header_5g.xatten1_margin[ix];
		}
		value[0] = val;
		*num = 1;
	}
    return VALUE_OK; 
}

A_INT32 Ar9300xatten1MarginLowGet(int *value, int ix, int *num, int iBand)
{
	int i, iv=0, iCMaxChain=OSPREY_MAX_CHAINS;
	int val;
	if (ix<0 || ix>iCMaxChain) {
		for (i=0; i<iCMaxChain; i++) {
			value[i] = Ar9300EepromStructGet()->base_ext2.xatten1_margin_low[i];
		}
		*num = iCMaxChain;
	} else {
		value[0] = Ar9300EepromStructGet()->base_ext2.xatten1_margin_low[ix];
		*num = 1;
	}
    return VALUE_OK; 
}

A_INT32 Ar9300xatten1MarginHighGet(int *value, int ix, int *num, int iBand)
{
	int i, iv=0, iCMaxChain=OSPREY_MAX_CHAINS;
	int val;
	if (ix<0 || ix>iCMaxChain) {
		for (i=0; i<iCMaxChain; i++) {
			value[i] = Ar9300EepromStructGet()->base_ext2.xatten1_margin_high[i];
		}
		*num = iCMaxChain;
	} else {
		value[0] = Ar9300EepromStructGet()->base_ext2.xatten1_margin_high[ix];
		*num = 1;
	}
    return VALUE_OK; 
}

/*
 *Function Name:Ar9300spurChansGet
 *Parameters: returned string value for get value
 *Description: Get spurChans flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (u_int8_t) 
 *Returns: zero
 */
A_INT32 Ar9300spurChansGet(int *value, int ix, int *num, int iBand)
{
	int i, iv=0;
	int val;
	if (ix<0 || ix>OSPREY_EEPROM_MODAL_SPURS) {
		for (i=0; i<OSPREY_EEPROM_MODAL_SPURS; i++) {
			if (iBand==band_BG) {
				val = Ar9300EepromStructGet()->modal_header_2g.spur_chans[i];
			} else {
				val = Ar9300EepromStructGet()->modal_header_5g.spur_chans[i];
			}
			value[i] = setFBIN2FREQ(val, iBand);
		}
		*num = OSPREY_EEPROM_MODAL_SPURS;
	} else {
		if (iBand==band_BG) {
			val = Ar9300EepromStructGet()->modal_header_2g.spur_chans[ix];
		} else {
			val = Ar9300EepromStructGet()->modal_header_5g.spur_chans[ix];
		}
		value[0] = setFBIN2FREQ(val, iBand);
		*num = 1;
	}
    return VALUE_OK; 
}

/*
 *Function Name:Ar9300MinCCAPwrThreshChGet
 *Parameters: returned string value for get value
 *Description: Get noise_floor_thresh_ch values in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (int8_t) 
 *Returns: zero
 */
A_INT32 Ar9300MinCCAPwrThreshChGet(int *value, int ix, int *num, int iBand)
{
	int i, iv=0, iCMaxChain=OSPREY_MAX_CHAINS;
	int val;
	if (ix<0 || ix>iCMaxChain) {
		for (i=0; i<iCMaxChain; i++) {
			if (iBand==band_BG) {
				val = Ar9300EepromStructGet()->modal_header_2g.noise_floor_thresh_ch[i];
			} else {
				val = Ar9300EepromStructGet()->modal_header_5g.noise_floor_thresh_ch[i];
			}
			value[i] = val;
		}
		*num = iCMaxChain;
	} else {
		if (iBand==band_BG) {
			val = Ar9300EepromStructGet()->modal_header_2g.noise_floor_thresh_ch[ix];
		} else {
			val = Ar9300EepromStructGet()->modal_header_5g.noise_floor_thresh_ch[ix];
		}
		value[0] = val;
		*num = 1;
	}
    return VALUE_OK; 
}

A_INT32 Ar9300ReservedGet(int *value, int ix, int *num, int iBand)
{
	int i, iv=0, iMax=MAX_MODAL_RESERVED;
	int val;
	if (ix<0 || ix>iMax) {
		for (i=0; i<iMax; i++) {
			if (iBand==band_BG) {
				val = Ar9300EepromStructGet()->modal_header_2g.reserved[i];
			} else {
				val = Ar9300EepromStructGet()->modal_header_5g.reserved[i];
			}
			value[i] = val;
		}
		*num = iMax;
	} else {
		if (iBand==band_BG) {
			val = Ar9300EepromStructGet()->modal_header_2g.reserved[ix];
		} else {
			val = Ar9300EepromStructGet()->modal_header_5g.reserved[ix];
		}
		value[0] = val;
		*num = 1;
	}
    return VALUE_OK; 
}


int Ar9300QuickDropGet(int iBand)
{
	int8_t  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.quick_drop;
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.quick_drop;
	}	
    return value; 
}

int Ar9300QuickDropLowGet()
{
	int8_t  value;
	value = Ar9300EepromStructGet()->base_ext1.quick_drop_low;
    return value; 
}

int Ar9300QuickDropHighGet()
{
	int8_t  value;
	value = Ar9300EepromStructGet()->base_ext1.quick_drop_high;
    return value; 
}

/*
 *Function Name:Ar9300xpaBiasLvlGet
 *Parameters: returned string value for get value
 *Description: Get xpa_bias_lvl value from field of eeprom struct u_int8_t
 *Returns: zero
 */
int Ar9300xpaBiasLvlGet(int iBand)
{
	u_int8_t  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.xpa_bias_lvl;
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.xpa_bias_lvl;
	}	
    return value; 
}

/*
 *Function Name:Ar9300txFrameToDataStartGet
 *Parameters: returned string value for get value
 *Description: Get tx_frame_to_data_start value from field of eeprom struct u_int8_t
 *Returns: zero
 */
int Ar9300txFrameToDataStartGet( int iBand)
{
	A_UINT8  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.tx_frame_to_data_start;
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.tx_frame_to_data_start;
	}	
    return value; 
}

/*
 *Function Name:Ar9300txFrameToPaOnGet
 *Parameters: returned string value for get value
 *Description: Get tx_frame_to_pa_on value from field of eeprom struct u_int8_t
 *Returns: zero
 */
int Ar9300txFrameToPaOnGet( int iBand)
{
	A_UINT8  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.tx_frame_to_pa_on;
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.tx_frame_to_pa_on;
	}	
    return value; 
}
/*
 *Function Name:Ar9300txClipGet
 *Parameters: returned string value for get value
 *Description: Get txClip value from field of eeprom struct u_int8_t (4 bits tx_clip)
 *Returns: zero
 */
int Ar9300txClipGet( int iBand)
{
	A_UINT8  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.txClip & 0x0f;		// which 4 bits are for tx_clip???
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.txClip & 0x0f;
	}	
    return value; 
}
/*
 *Function Name:Ar9300dac_scale_cckGet
 *Parameters: returned string value for get value
 *Description: Get txClip(dac_scale_cck) value from field of eeprom struct u_int8_t (4 bits tx_clip)
 *Returns: zero
 */
int Ar9300dac_scale_cckGet( int iBand)
{
	A_UINT8  value;
	if (iBand==band_BG) {
		value = (Ar9300EepromStructGet()->modal_header_2g.txClip & 0xf0) >> 4;		// which 4 bits are for dac_scale_cck???
	} else {
		value = (Ar9300EepromStructGet()->modal_header_5g.txClip & 0xf0) >> 4;
	}	
    return value; 
}
/*
 *Function Name:Ar9300antennaGainGet
 *Parameters: returned string value for get value
 *Description: Get antenna_gain value from field of eeprom struct int8_t 
 *Returns: zero
 */
int Ar9300antennaGainGet( int iBand)
{
	A_INT8  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.antenna_gain;		
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.antenna_gain;
	}	
    return value; 
}
/*
 *Function Name:Ar9300adcDesiredSizeGet
 *Parameters: returned string value for get value
 *Description: Get adcDesiredSize value from field of eeprom struct int8_t 
 *Returns: zero
 */
int Ar9300adcDesiredSizeGet( int iBand)
{
	A_INT8  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.adcDesiredSize;		
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.adcDesiredSize;
	}	
    return value; 
}
/*
 *Function Name:Ar9300switchSettlingGet
 *Parameters: returned string value for get value
 *Description: Get switchSettling value from field of eeprom struct u_int8_t 
 *Returns: zero
 */
int Ar9300switchSettlingGet( int iBand)
{
	A_UINT8  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.switchSettling;		
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.switchSettling;
	}	
    return value; 
}
/*
 *Function Name:Ar9300txEndToXpaOffGet
 *Parameters: returned string value for get value
 *Description: Get txEndToXpaOff value from field of eeprom struct u_int8_t 
 *Returns: zero
 */
int Ar9300txEndToXpaOffGet( int iBand)
{
	A_UINT8  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.tx_end_to_xpa_off;		
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.tx_end_to_xpa_off;
	}	
    return value; 
}
/*
 *Function Name:Ar9300txEndToRxOnGet
 *Parameters: returned string value for get value
 *Description: Get txEndToRxOn value from field of eeprom struct u_int8_t 
 *Returns: zero
 */
int Ar9300txEndToRxOnGet( int iBand)
{
	A_UINT8  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.txEndToRxOn;		
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.txEndToRxOn;
	}	
    return value; 
}
/*
 *Function Name:Ar9300txFrameToXpaOnGet
 *Parameters: returned string value for get value
 *Description: Get tx_frame_to_xpa_on value from field of eeprom struct u_int8_t 
 *Returns: zero
 */
int Ar9300txFrameToXpaOnGet( int iBand)
{
	A_UINT8  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.tx_frame_to_xpa_on;		
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.tx_frame_to_xpa_on;
	}	
    return value; 
}
/*
 *Function Name:Ar9300thresh62Get
 *Parameters: returned string value for get value
 *Description: Get thresh62 value from field of eeprom struct u_int8_t 
 *Returns: zero
 */
int Ar9300thresh62Get( int iBand)
{
	A_UINT8  value;
	if (iBand==band_BG) {
		value = Ar9300EepromStructGet()->modal_header_2g.thresh62;		
	} else {
		value = Ar9300EepromStructGet()->modal_header_5g.thresh62;
	}	
    return value; 
}

/*
 *Function Name:Ar9300calFreqPierGet
 *Parameters: returned string value for get value
 *Description: Get calFreqPier flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (u_int8_t) 
 *Returns: zero
 */
A_INT32 Ar9300calFreqPierGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, iv=0, iMaxPier;
	int val;
	if (iBand==band_BG) 
		iMaxPier=OSPREY_NUM_2G_CAL_PIERS;
	else
		iMaxPier=OSPREY_NUM_5G_CAL_PIERS;
	if (ix<0 || ix>iMaxPier) {
		for (i=0; i<iMaxPier; i++) {
			if (iBand==band_BG) {
				val = Ar9300EepromStructGet()->cal_freq_pier_2g[i];
			} else {
				val = Ar9300EepromStructGet()->cal_freq_pier_5g[i];
			}
			value[i] = setFBIN2FREQ(val, iBand);
		}
		*num = iMaxPier;
	} else {
		if (iBand==band_BG) {
			val = Ar9300EepromStructGet()->cal_freq_pier_2g[ix];
		} else {
			val = Ar9300EepromStructGet()->cal_freq_pier_5g[ix];
		}
		value[0] = setFBIN2FREQ(val, iBand);
		*num = 1;
	}
	return VALUE_OK;
}
/*
 *Function Name:Ar9300calPierDataRefPowerGet
 *Parameters: returned string value for get value
 *Description: Get calPierData.RefPower flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (int8_t) 
 *Returns: zero
 */
A_INT32 Ar9300calPierDataRefPowerGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, j, iMaxPier, iv=0;
	if (iBand==band_BG) 
		iMaxPier=OSPREY_NUM_2G_CAL_PIERS;
	else
		iMaxPier=OSPREY_NUM_5G_CAL_PIERS;
	if (iy<0 || iy>=iMaxPier) {
		if (ix<0 || ix>=OSPREY_MAX_CHAINS) {
			// get all i, all j
			for (i=0; i<OSPREY_MAX_CHAINS; i++) {
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_2g[i][j].ref_power;
					} else {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_5g[i][j].ref_power;
					}
				}
				*num = OSPREY_MAX_CHAINS*iMaxPier;
			}
		} else { // get all j for ix chain
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_2g[ix][j].ref_power;
					} else {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_5g[ix][j].ref_power;
					}
				}
				*num = iMaxPier;
		}
	} else {
		if (iBand==band_BG) {
			value[0] = Ar9300EepromStructGet()->cal_pier_data_2g[ix][iy].ref_power;
		} else {
			value[0] = Ar9300EepromStructGet()->cal_pier_data_5g[ix][iy].ref_power;
		}
		*num = 1;
	}
    return VALUE_OK; 
}
/*
 *Function Name:Ar9300calPierDataVoltMeasGet
 *Parameters: returned string value for get value
 *Description: Get calPierData.volt_meas flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (u_int8_t) 
 *Returns: zero
 */
A_INT32 Ar9300calPierDataVoltMeasGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, j, iMaxPier, iv=0;
	if (iBand==band_BG) 
		iMaxPier=OSPREY_NUM_2G_CAL_PIERS;
	else
		iMaxPier=OSPREY_NUM_5G_CAL_PIERS;
	if (iy<0 || iy>=iMaxPier) {
		if (ix<0 || ix>=OSPREY_MAX_CHAINS) {
			// get all i, all j
			for (i=0; i<OSPREY_MAX_CHAINS; i++) {
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_2g[i][j].volt_meas;
					} else {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_5g[i][j].volt_meas;
					}
				}
				*num = OSPREY_MAX_CHAINS*iMaxPier;
			}
		} else { // get all j for ix chain
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_2g[ix][j].volt_meas;
					} else {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_5g[ix][j].volt_meas;
					}
				}
				*num = iMaxPier;
		}
	} else {
		if (iBand==band_BG) {
			value[0] = Ar9300EepromStructGet()->cal_pier_data_2g[ix][iy].volt_meas;
		} else {
			value[0] = Ar9300EepromStructGet()->cal_pier_data_5g[ix][iy].volt_meas;
		}
		*num = 1;
	}
    return VALUE_OK; 
}
/*
 *Function Name:Ar9300calPierDataTempMeasGet
 *Parameters: returned string value for get value
 *Description: Get calPierData.temp_meas flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (u_int8_t) 
 *Returns: zero
 */
A_INT32 Ar9300calPierDataTempMeasGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, j, iMaxPier, iv=0;
	if (iBand==band_BG) 
		iMaxPier=OSPREY_NUM_2G_CAL_PIERS;
	else
		iMaxPier=OSPREY_NUM_5G_CAL_PIERS;
	if (iy<0 || iy>=iMaxPier) {
		if (ix<0 || ix>=OSPREY_MAX_CHAINS) {
			// get all i, all j
			for (i=0; i<OSPREY_MAX_CHAINS; i++) {
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_2g[i][j].temp_meas;
					} else {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_5g[i][j].temp_meas;
					}
				}
				*num = OSPREY_MAX_CHAINS*iMaxPier;
			}
		} else { // get all j for ix chain
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_2g[ix][j].temp_meas;
					} else {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_5g[ix][j].temp_meas;
					}
				}
				*num = iMaxPier;
		}
	} else {
		if (iBand==band_BG) {
			value[0] = Ar9300EepromStructGet()->cal_pier_data_2g[ix][iy].temp_meas;
		} else {
			value[0] = Ar9300EepromStructGet()->cal_pier_data_5g[ix][iy].temp_meas;
		}
		*num = 1;
	}
    return VALUE_OK; 
}
/*
 *Function Name:Ar9300calPierDataRxNoisefloorCalGet
 *Parameters: returned string value for get value
 *Description: Get calPierData.rx_noisefloor_cal flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (u_int8_t) 
 *Returns: zero
 */
A_INT32 Ar9300calPierDataRxNoisefloorCalGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, j, iMaxPier, iv=0;
	if (iBand==band_BG) 
		iMaxPier=OSPREY_NUM_2G_CAL_PIERS;
	else
		iMaxPier=OSPREY_NUM_5G_CAL_PIERS;
	if (iy<0 || iy>=iMaxPier) {
		if (ix<0 || ix>=OSPREY_MAX_CHAINS) {
			// get all i, all j
			for (i=0; i<OSPREY_MAX_CHAINS; i++) {
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_2g[i][j].rx_noisefloor_cal;
					} else {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_5g[i][j].rx_noisefloor_cal;
					}
				}
				*num = OSPREY_MAX_CHAINS*iMaxPier;
			}
		} else { // get all j for ix chain
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_2g[ix][j].rx_noisefloor_cal;
					} else {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_5g[ix][j].rx_noisefloor_cal;
					}
				}
				*num = iMaxPier;
		}
	} else {
		if (iBand==band_BG) {
			value[0] = Ar9300EepromStructGet()->cal_pier_data_2g[ix][iy].rx_noisefloor_cal;
		} else {
			value[0] = Ar9300EepromStructGet()->cal_pier_data_5g[ix][iy].rx_noisefloor_cal;
		}
		*num = 1;
	}
    return VALUE_OK; 
}
/*
 *Function Name:Ar9300calPierDataRxNoisefloorPowerGet
 *Parameters: returned string value for get value
 *Description: Get calPierData.rx_noisefloor_power flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (u_int8_t) 
 *Returns: zero
 */
A_INT32 Ar9300calPierDataRxNoisefloorPowerGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, j, iMaxPier, iv=0;
	if (iBand==band_BG) 
		iMaxPier=OSPREY_NUM_2G_CAL_PIERS;
	else
		iMaxPier=OSPREY_NUM_5G_CAL_PIERS;
	if (iy<0 || iy>=iMaxPier) {
		if (ix<0 || ix>=OSPREY_MAX_CHAINS) {
			// get all i, all j
			for (i=0; i<OSPREY_MAX_CHAINS; i++) {
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_2g[i][j].rx_noisefloor_power;
					} else {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_5g[i][j].rx_noisefloor_power;
					}
				}
				*num = OSPREY_MAX_CHAINS*iMaxPier;
			}
		} else { // get all j for ix chain
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_2g[ix][j].rx_noisefloor_power;
					} else {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_5g[ix][j].rx_noisefloor_power;
					}
				}
				*num = iMaxPier;
		}
	} else {
		if (iBand==band_BG) {
			value[0] = Ar9300EepromStructGet()->cal_pier_data_2g[ix][iy].rx_noisefloor_power;
		} else {
			value[0] = Ar9300EepromStructGet()->cal_pier_data_5g[ix][iy].rx_noisefloor_power;
		}
		*num = 1;
	}
    return VALUE_OK; 
}
/*
 *Function Name:Ar9300calPierDataRxTempMeasGet
 *Parameters: returned string value for get value
 *Description: Get calPierData.rxTempMeas flag in field of eeprom struct in OSPREY_MODAL_EEP_HEADER (u_int8_t) 
 *Returns: zero
 */
A_INT32 Ar9300calPierDataRxTempMeasGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, j, iMaxPier, iv=0;
	if (iBand==band_BG) 
		iMaxPier=OSPREY_NUM_2G_CAL_PIERS;
	else
		iMaxPier=OSPREY_NUM_5G_CAL_PIERS;
	if (iy<0 || iy>=iMaxPier) {
		if (ix<0 || ix>=OSPREY_MAX_CHAINS) {
			// get all i, all j
			for (i=0; i<OSPREY_MAX_CHAINS; i++) {
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_2g[i][j].rxTempMeas;
					} else {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_5g[i][j].rxTempMeas;
					}
				}
				*num = OSPREY_MAX_CHAINS*iMaxPier;
			}
		} else { // get all j for ix chain
				for (j=0; j<iMaxPier; j++) {
					if (iBand==band_BG) {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_2g[ix][j].rxTempMeas;
					} else {
						value[iv++] = Ar9300EepromStructGet()->cal_pier_data_5g[ix][j].rxTempMeas;
					}
				}
				*num = iMaxPier;
		}
	} else {
		if (iBand==band_BG) {
			value[0] = Ar9300EepromStructGet()->cal_pier_data_2g[ix][iy].rxTempMeas;
		} else {
			value[0] = Ar9300EepromStructGet()->cal_pier_data_5g[ix][iy].rxTempMeas;
		}
		*num = 1;
	}
    return VALUE_OK; 
}

A_INT32 Ar9300calFreqTGTcckGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, iv=0, iMaxPier;
	int val;
	iMaxPier=OSPREY_NUM_2G_CCK_TARGET_POWERS;
	if (ix<0 || ix>iMaxPier) {
		for (i=0; i<iMaxPier; i++) {
			val = Ar9300EepromStructGet()->cal_target_freqbin_cck[i];
			value[i] = setFBIN2FREQ(val, iBand);
		}
		*num = iMaxPier;
	} else {
		val = Ar9300EepromStructGet()->cal_target_freqbin_cck[ix];
		value[0] = setFBIN2FREQ(val, iBand);
		*num = 1;
	}
	return VALUE_OK;
}

A_INT32 Ar9300calFreqTGTLegacyOFDMGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, iv=0, iMaxPier;
	int val;
	if (iBand==band_BG) 
		iMaxPier=OSPREY_NUM_2G_20_TARGET_POWERS;
	else
		iMaxPier=OSPREY_NUM_5G_20_TARGET_POWERS;
	if (ix<0 || ix>iMaxPier) {
		for (i=0; i<iMaxPier; i++) {
			if (iBand==band_BG) {
				val = Ar9300EepromStructGet()->cal_target_freqbin_2g[i];
			} else {
				val = Ar9300EepromStructGet()->cal_target_freqbin_5g[i];
			}
			value[i] = setFBIN2FREQ(val, iBand);
		}
		*num = iMaxPier;
	} else {
		if (iBand==band_BG) {
			val = Ar9300EepromStructGet()->cal_target_freqbin_2g[ix];
		} else {
			val = Ar9300EepromStructGet()->cal_target_freqbin_5g[ix];
		}
		value[0] = setFBIN2FREQ(val, iBand);
		*num = 1;
	}
	return VALUE_OK;
}

A_INT32 Ar9300calFreqTGTHT20Get(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, iv=0, iMaxPier;
	int val;
	if (iBand==band_BG) 
		iMaxPier=OSPREY_NUM_2G_20_TARGET_POWERS;
	else
		iMaxPier=OSPREY_NUM_5G_20_TARGET_POWERS;
	if (ix<0 || ix>iMaxPier) {
		for (i=0; i<iMaxPier; i++) {
			if (iBand==band_BG) {
				val = Ar9300EepromStructGet()->cal_target_freqbin_2g_ht20[i];
			} else {
				val = Ar9300EepromStructGet()->cal_target_freqbin_5g_ht20[i];
			}
			value[i] = setFBIN2FREQ(val, iBand);
		}
		*num = iMaxPier;
	} else {
		if (iBand==band_BG) {
			val = Ar9300EepromStructGet()->cal_target_freqbin_2g_ht20[ix];
		} else {
			val = Ar9300EepromStructGet()->cal_target_freqbin_5g_ht20[ix];
		}
		value[0] = setFBIN2FREQ(val, iBand);
		*num = 1;
	}
	return VALUE_OK;
}

A_INT32 Ar9300calFreqTGTHT40Get(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, iv=0, iMaxPier;
	int val;
	if (iBand==band_BG) 
		iMaxPier=OSPREY_NUM_2G_40_TARGET_POWERS;
	else
		iMaxPier=OSPREY_NUM_5G_40_TARGET_POWERS;
	if (ix<0 || ix>iMaxPier) {
		for (i=0; i<iMaxPier; i++) {
			if (iBand==band_BG) {
				val = Ar9300EepromStructGet()->cal_target_freqbin_2g_ht40[i];
			} else {
				val = Ar9300EepromStructGet()->cal_target_freqbin_5g_ht40[i];
			}
			value[i] = setFBIN2FREQ(val, iBand);
		}
		*num = iMaxPier;
	} else {
		if (iBand==band_BG) {
			val = Ar9300EepromStructGet()->cal_target_freqbin_2g_ht40[ix];
		} else {
			val = Ar9300EepromStructGet()->cal_target_freqbin_5g_ht40[ix];
		}
		value[0] = setFBIN2FREQ(val, iBand);
		*num = 1;
	}
	return VALUE_OK;
}

A_INT32 Ar9300calTGTPwrCCKGet(double *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, j, iv=0, iMaxPier, jMaxRate=4;
	int val;
	iMaxPier=OSPREY_NUM_2G_CCK_TARGET_POWERS;
	if (iy<0 || iy>=jMaxRate) {
		if (ix<0 || ix>=iMaxPier) {
			// get all i, all j
			for (i=0; i<iMaxPier; i++) {
				for (j=0; j<jMaxRate; j++) {
					value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_cck[i].t_pow2x[j])/2.0;
				}
			}
			*num = jMaxRate*iMaxPier;
		} else { // get all j for ix chain
				for (j=0; j<jMaxRate; j++) {
					value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_cck[ix].t_pow2x[j])/2.0;
				}
				*num = jMaxRate;
		}
	} else {
		if (ix<0 || ix>=iMaxPier) {
			for (i=0; i<iMaxPier; i++) {
				value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_cck[i].t_pow2x[iy])/2.0;
				*num = iMaxPier;
			}
		} else {
			value[0] = ((double)Ar9300EepromStructGet()->cal_target_power_cck[ix].t_pow2x[iy])/2.0;
			*num = 1;
		}
	}	
	return VALUE_OK; 
}
A_INT32 Ar9300calTGTPwrLegacyOFDMGet(double *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, j, iv=0, iMaxPier, jMaxRate=4;
	int val;
	if (iBand==band_BG) 
		iMaxPier=OSPREY_NUM_2G_20_TARGET_POWERS;
	else
		iMaxPier=OSPREY_NUM_5G_20_TARGET_POWERS;
	if (iy<0 || iy>=jMaxRate) {
		if (ix<0 || ix>=iMaxPier) {
			// get all i, all j
			for (i=0; i<iMaxPier; i++) {
				for (j=0; j<jMaxRate; j++) {
					if (iBand==band_BG) {
						value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_2g[i].t_pow2x[j])/2.0;
					} else {
						value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_5g[i].t_pow2x[j])/2.0;
					}
				}
			}
			*num = jMaxRate*iMaxPier;
		} else { // get all j for ix chain
				for (j=0; j<jMaxRate; j++) {
					if (iBand==band_BG) {
						value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_2g[ix].t_pow2x[j])/2.0;
					} else {
						value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_5g[ix].t_pow2x[j])/2.0;
					}
				}
				*num = jMaxRate;
		}
	} else {
		if (ix<0 || ix>=iMaxPier) {
			for (i=0; i<iMaxPier; i++) {
				if (iBand==band_BG) {
					value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_2g[i].t_pow2x[iy])/2.0;
				} else {
					value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_5g[i].t_pow2x[iy])/2.0;
				}
				*num = iMaxPier;
			}
		} else {
			if (iBand==band_BG) {
				value[0] =((double) Ar9300EepromStructGet()->cal_target_power_2g[ix].t_pow2x[iy])/2.0;
			} else {
				value[0] = ((double)Ar9300EepromStructGet()->cal_target_power_5g[ix].t_pow2x[iy])/2.0;
			}
			*num = 1;
		}
	}
    return VALUE_OK; 
}
A_INT32 Ar9300calTGTPwrHT20Get(double *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, j, iv=0, iMaxPier, jMaxRate=14;
	int val;
	if (iBand==band_BG) 
		iMaxPier=OSPREY_NUM_2G_20_TARGET_POWERS;
	else
		iMaxPier=OSPREY_NUM_5G_20_TARGET_POWERS;
	if (iy<0 || iy>=jMaxRate) {
		if (ix<0 || ix>=iMaxPier) {
			// get all i, all j
			for (i=0; i<iMaxPier; i++) {
				for (j=0; j<jMaxRate; j++) {
					if (iBand==band_BG) {
						value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_2g_ht20[i].t_pow2x[j])/2.0;
					} else {
						value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_5g_ht20[i].t_pow2x[j])/2.0;
					}
				}
			}
			*num = jMaxRate*iMaxPier;
		} else { // get all j for ix chain
				for (j=0; j<jMaxRate; j++) {
					if (iBand==band_BG) {
						value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_2g_ht20[ix].t_pow2x[j])/2.0;
					} else {
						value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_5g_ht20[ix].t_pow2x[j])/2.0;
					}
				}
				*num = jMaxRate;
		}
	} else {
		if (ix<0 || ix>=iMaxPier) {
			for (i=0; i<iMaxPier; i++) {
				if (iBand==band_BG) {
					value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_2g_ht20[i].t_pow2x[iy])/2.0;
				} else {
					value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_5g_ht20[i].t_pow2x[iy])/2.0;
				}
				*num = iMaxPier;
			}
		} else {
			if (iBand==band_BG) {
				value[0] = ((double)Ar9300EepromStructGet()->cal_target_power_2g_ht20[ix].t_pow2x[iy])/2.0;
			} else {
				value[0] = ((double)Ar9300EepromStructGet()->cal_target_power_5g_ht20[ix].t_pow2x[iy])/2.0;
			}
			*num = 1;
		}
	}
    return VALUE_OK; 
}
A_INT32 Ar9300calTGTPwrHT40Get(double *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, j, iv=0, iMaxPier, jMaxRate=14;
	int val;
	if (iBand==band_BG) 
		iMaxPier=OSPREY_NUM_2G_40_TARGET_POWERS;
	else
		iMaxPier=OSPREY_NUM_5G_40_TARGET_POWERS;
	if (iy<0 || iy>=jMaxRate) {
		if (ix<0 || ix>=iMaxPier) {
			// get all i, all j
			for (i=0; i<iMaxPier; i++) {
				for (j=0; j<jMaxRate; j++) {
					if (iBand==band_BG) {
						value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_2g_ht40[i].t_pow2x[j])/2.0;
					} else {
						value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_5g_ht40[i].t_pow2x[j])/2.0;
					}
				}
			}
			*num = jMaxRate*iMaxPier;
		} else { // get all j for ix chain
				for (j=0; j<jMaxRate; j++) {
					if (iBand==band_BG) {
						value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_2g_ht40[ix].t_pow2x[j])/2.0;
					} else {
						value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_5g_ht40[ix].t_pow2x[j])/2.0;
					}
				}
				*num = jMaxRate;
		}
	} else {
		if (ix<0 || ix>=iMaxPier) {
			for (i=0; i<iMaxPier; i++) {
				if (iBand==band_BG) {
					value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_2g_ht40[i].t_pow2x[iy])/2.0;
				} else {
					value[iv++] = ((double)Ar9300EepromStructGet()->cal_target_power_5g_ht40[i].t_pow2x[iy])/2.0;
				}
				*num = iMaxPier;
			}
		} else {
			if (iBand==band_BG) {
				value[0] = ((double)Ar9300EepromStructGet()->cal_target_power_2g_ht40[ix].t_pow2x[iy])/2.0;
			} else {
				value[0] = ((double)Ar9300EepromStructGet()->cal_target_power_5g_ht40[ix].t_pow2x[iy])/2.0;
			}
			*num = 1;
		}
	}
    return VALUE_OK; 
}

A_INT32 Ar9300ctlIndexGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, iv=0, iMaxCtl;
	int val;
	if (iBand==band_BG) 
		iMaxCtl=OSPREY_NUM_CTLS_2G;
	else
		iMaxCtl=OSPREY_NUM_CTLS_5G;
	if (ix<0 || ix>iMaxCtl) {
		for (i=0; i<iMaxCtl; i++) {
			if (iBand==band_BG) {
				val = Ar9300EepromStructGet()->ctl_index_2g[i];
			} else {
				val = Ar9300EepromStructGet()->ctl_index_5g[i];
			}
			value[i] = val;
		}
		*num = iMaxCtl;
	} else {
		if (iBand==band_BG) {
			val = Ar9300EepromStructGet()->ctl_index_2g[ix];
		} else {
			val = Ar9300EepromStructGet()->ctl_index_5g[ix];
		}
		value[0] = val;
		*num = 1;
	}	
    return VALUE_OK; 
}

A_INT32 Ar9300ctlFreqGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, j, iv=0, iMaxCtl, jMaxEdge;
	int val;
	if (iBand==band_BG) {
		jMaxEdge=OSPREY_NUM_BAND_EDGES_2G;
		iMaxCtl=OSPREY_NUM_CTLS_2G;
	} else {
		jMaxEdge=OSPREY_NUM_BAND_EDGES_5G;
		iMaxCtl=OSPREY_NUM_CTLS_5G;
	}
	if (iy<0 || iy>=jMaxEdge) {
		if (ix<0 || ix>=iMaxCtl) {
			// get all i, all j
			for (i=0; i<iMaxCtl; i++) {
				for (j=0; j<jMaxEdge; j++) {
					if (iBand==band_BG) {
						val = Ar9300EepromStructGet()->ctl_freqbin_2G[i][j];
					} else {
						val = Ar9300EepromStructGet()->ctl_freqbin_5G[i][j];
					}
					value[iv++]=setFBIN2FREQ(val, iBand);
				}
			}
			*num = jMaxEdge*iMaxCtl;
		} else { // get all j for ix chain
				for (j=0; j<jMaxEdge; j++) {
					if (iBand==band_BG) {
						val = Ar9300EepromStructGet()->ctl_freqbin_2G[ix][j];
					} else {
						val = Ar9300EepromStructGet()->ctl_freqbin_5G[ix][j];
					}
					value[iv++]=setFBIN2FREQ(val, iBand);
				}
				*num = jMaxEdge;
		}
	} else {
		if (ix<0 || ix>=iMaxCtl) {
			for (i=0; i<iMaxCtl; i++) {
				if (iBand==band_BG) {
					val = Ar9300EepromStructGet()->ctl_freqbin_2G[i][iy];
				} else {
					val = Ar9300EepromStructGet()->ctl_freqbin_5G[i][iy];
				}
				value[iv++]=setFBIN2FREQ(val, iBand);
				*num = iMaxCtl;
			}
		} else {
			if (iBand==band_BG) {
				val = Ar9300EepromStructGet()->ctl_freqbin_2G[ix][iy];
			} else {
				val = Ar9300EepromStructGet()->ctl_freqbin_5G[ix][iy];
			}
			value[0]=setFBIN2FREQ(val, iBand);
			*num = 1;
		}
	}
    return VALUE_OK; 
}
A_INT32 Ar9300ctlPowerGet(double *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, j, iv=0, iMaxCtl, jMaxEdge;
	int val;
	if (iBand==band_BG) {
		jMaxEdge=OSPREY_NUM_BAND_EDGES_2G;
		iMaxCtl=OSPREY_NUM_CTLS_2G;
	} else {
		jMaxEdge=OSPREY_NUM_BAND_EDGES_5G;
		iMaxCtl=OSPREY_NUM_CTLS_5G;
	}
	if (iy<0 || iy>=jMaxEdge) {
		if (ix<0 || ix>=iMaxCtl) {
			// get all i, all j
			for (i=0; i<iMaxCtl; i++) {
				for (j=0; j<jMaxEdge; j++) {
					if (iBand==band_BG) {
						val = Ar9300EepromStructGet()->ctl_power_data_2g[i].ctl_edges[j].t_power;
					} else {
						val = Ar9300EepromStructGet()->ctl_power_data_5g[i].ctl_edges[j].t_power;
					}
					value[iv++]=val/2.0;
				}
			}
			*num = jMaxEdge*iMaxCtl;
		} else { // get all j for ix chain
				for (j=0; j<jMaxEdge; j++) {
					if (iBand==band_BG) {
						val = Ar9300EepromStructGet()->ctl_power_data_2g[ix].ctl_edges[j].t_power;
					} else {
						val = Ar9300EepromStructGet()->ctl_power_data_5g[ix].ctl_edges[j].t_power;
					}
					value[iv++]=val/2.0;
				}
				*num = jMaxEdge;
		}
	} else {
		if (ix<0 || ix>=iMaxCtl) {
			for (i=0; i<iMaxCtl; i++) {
				if (iBand==band_BG) {
					val = Ar9300EepromStructGet()->ctl_power_data_2g[i].ctl_edges[iy].t_power;
				} else {
					val = Ar9300EepromStructGet()->ctl_power_data_5g[i].ctl_edges[iy].t_power;
				}
				value[iv++]=val/2.0;
				*num = iMaxCtl;
			}
		} else {
			if (iBand==band_BG) {
				val = Ar9300EepromStructGet()->ctl_power_data_2g[ix].ctl_edges[iy].t_power;
			} else {
				val = Ar9300EepromStructGet()->ctl_power_data_5g[ix].ctl_edges[iy].t_power;
			}
			value[0]=val/2.0;
			*num = 1;
		}
	}
    return VALUE_OK; 
}
A_INT32 Ar9300ctlFlagGet(int *value, int ix, int iy, int iz, int *num, int iBand)
{
	int i, j, iv=0, iMaxCtl, jMaxEdge;
	int val;
	if (iBand==band_BG) {
		jMaxEdge=OSPREY_NUM_BAND_EDGES_2G;
		iMaxCtl=OSPREY_NUM_CTLS_2G;
	} else {
		jMaxEdge=OSPREY_NUM_BAND_EDGES_5G;
		iMaxCtl=OSPREY_NUM_CTLS_5G;
	}
	if (iy<0 || iy>=jMaxEdge) {
		if (ix<0 || ix>=iMaxCtl) {
			// get all i, all j
			for (i=0; i<iMaxCtl; i++) {
				for (j=0; j<jMaxEdge; j++) {
					if (iBand==band_BG) {
						val = Ar9300EepromStructGet()->ctl_power_data_2g[i].ctl_edges[j].flag;
					} else {
						val = Ar9300EepromStructGet()->ctl_power_data_5g[i].ctl_edges[j].flag;
					}
					value[iv++]=val;
				}
			}
			*num = jMaxEdge*iMaxCtl;
		} else { // get all j for ix chain
				for (j=0; j<jMaxEdge; j++) {
					if (iBand==band_BG) {
						val = Ar9300EepromStructGet()->ctl_power_data_2g[ix].ctl_edges[j].flag;
					} else {
						val = Ar9300EepromStructGet()->ctl_power_data_5g[ix].ctl_edges[j].flag;
					}
					value[iv++]=val;
				}
				*num = jMaxEdge;
		}
	} else {
		if (ix<0 || ix>=iMaxCtl) {
			for (i=0; i<iMaxCtl; i++) {
				if (iBand==band_BG) {
					val = Ar9300EepromStructGet()->ctl_power_data_2g[i].ctl_edges[iy].flag;
				} else {
					val = Ar9300EepromStructGet()->ctl_power_data_5g[i].ctl_edges[iy].flag;
				}
				value[iv++]=val;
				*num = iMaxCtl;
			}
		} else {
			if (iBand==band_BG) {
				val = Ar9300EepromStructGet()->ctl_power_data_2g[ix].ctl_edges[iy].flag;
			} else {
				val = Ar9300EepromStructGet()->ctl_power_data_5g[ix].ctl_edges[iy].flag;
			}
			value[0]=val;
			*num = 1;
		}
	}	
    return VALUE_OK; 
}

AR9300DLLSPEC void Ar9300EepromPaPredistortionSet(int value)
{
	if(value)
	{
	    Ar9300EepromStructGet()->base_eep_header.feature_enable |= (1<<5);
	}
	else
	{
	    Ar9300EepromStructGet()->base_eep_header.feature_enable &= ~(1<<5);
	}
}

AR9300DLLSPEC int Ar9300EepromPaPredistortionGet(void)
{
	return ((Ar9300EepromStructGet()->base_eep_header.feature_enable>>5)&1);
}

int Ar9300EepromCalibrationValid(void)
{
	int ic, ip;

	for(ic=0; ic<OSPREY_MAX_CHAINS; ic++)
	{
		for(ip=0; ip<OSPREY_NUM_2G_CAL_PIERS; ip++)
		{
			if(Ar9300EepromStructGet()->cal_pier_data_2g[ic][ip].ref_power!=0 && Ar9300EepromStructGet()->cal_pier_data_2g[ic][ip].temp_meas!=0)
			{
				return 1;
			}
		}
	}
	for(ic=0; ic<OSPREY_MAX_CHAINS; ic++)
	{
		for(ip=0; ip<OSPREY_NUM_5G_CAL_PIERS; ip++)
		{
			if(Ar9300EepromStructGet()->cal_pier_data_5g[ic][ip].ref_power!=0 && Ar9300EepromStructGet()->cal_pier_data_5g[ic][ip].temp_meas!=0)
			{
				return 1;
			}
		}
	}
    return 0;
}


