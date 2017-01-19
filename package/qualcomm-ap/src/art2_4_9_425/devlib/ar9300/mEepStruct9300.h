/*
 * Copyright (c) 2002-2006 Atheros Communications, Inc.
 * All rights reserved.
 *
 */

// "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/devlib/ar9300/mEepStruct9300.h#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/devlib/ar9300/mEepStruct9300.h#1 $"

#ifndef _OSPREY_EEPROM_STRUCT_H_
#define _OSPREY_EEPROM_STRUCT_H_

#ifdef _WINDOWS
#ifdef AR9300DLL
		#define AR9300DLLSPEC __declspec(dllexport)
	#else
		#define AR9300DLLSPEC __declspec(dllimport)
	#endif
#else
	#define AR9300DLLSPEC
#endif



extern AR9300DLLSPEC A_INT32 Ar9300MacAddressGet(unsigned char *mac);
extern AR9300DLLSPEC A_INT32 Ar9300CustomerDataGet(unsigned char *data, int len);
extern AR9300DLLSPEC int Ar9300CalibrationPierSet(int pierIdx, int freq, int chain, 
                          int pwrCorrection, int volt_meas, int temp_meas);
extern AR9300DLLSPEC int Ar9300CalInfoCalibrationPierSet(int pier, int frequency, int chain, 
							int gain, int gainIndex, int dacGain, double power, 
							int correction, int voltage, int temperature, int calPoint);
extern AR9300DLLSPEC int Ar9300CalibrationTxgainCAPSet(int *txgainmax);
extern AR9300DLLSPEC int Ar9300RegulatoryDomainGet(void);
extern AR9300DLLSPEC int Ar9300RegulatoryDomainOverride(unsigned int regdmn);
extern AR9300DLLSPEC int Ar9300NoiseFloorGet(int frequency, int ichain);
extern AR9300DLLSPEC int Ar9300NoiseFloorPowerGet(int frequency, int ichain);
extern AR9300DLLSPEC A_INT32 Ar9300CaldataMemoryTypeGet(A_UCHAR *memType, A_INT32 maxlength);


    const static char *sRatePrintHT[24] = 
	{
		"MCS0 ",
		"MCS1 ", 
		"MCS2 ",
		"MCS3 ",
		"MCS4 ",
		"MCS5 ",
		"MCS6 ",
		"MCS7 ",
		"MCS8 ",
		"MCS9 ",
		"MCS10",
		"MCS11",
		"MCS12",
		"MCS13",
		"MCS14",
		"MCS15",
		"MCS16",
		"MCS17",
		"MCS18",
		"MCS19",
		"MCS20",
		"MCS21",
		"MCS22",
		"MCS23"
	};


    const static char *sRatePrintLegacy[4] = 
	{
		"6-24",
		" 36 ", 
		" 48 ", 
		" 54 "
	};



    const static char *sRatePrintCck[4] = 
	{
		"1L-5L",
		" 5S  ", 
		" 11L ", 
		" 11S "
	};



    const static char *sDeviceType[] = {
      "UNKNOWN",
      "Cardbus",
      "PCI    ",
      "MiniPCI",
      "AP     ",
      "PCIE   ",
      "UNKNOWN",
      "UNKNOWN",
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


extern int Ar9300FutureGet(int *value, int ix, int *num, int iBand);
extern int Ar9300AntDivCtrlGet(void);
extern int Ar9300ReconfigMiscGet(void);
extern int Ar9300ReconfigDriveStrengthGet(void);
extern int Ar9300ReconfigQuickDropGet(void);
extern int Ar9300Reconfig8TempGet(void);
extern int Ar9300EnableFeatureGet(void);
extern int Ar9300EnableTempCompensationGet(void);
extern int Ar9300EnableVoltCompensationGet(void);
extern int Ar9300EnableFastClockGet(void);
extern int Ar9300EnableDoublingGet(void);
extern int Ar9300InternalRegulatorGet(void);
extern int Ar9300PapdGet(void);
extern int Ar9300EnableTuningCapsGet(void);
extern int Ar9300EnableTxFrameToXpaOnGet(void);
extern int Ar9300PapdRateMaskHt20Get(int iBand);
extern int Ar9300PapdRateMaskHt40Get(int iBand);
extern int Ar9300WlanSpdtSwitchGlobalControlGet(int iBand);
extern int Ar9300EnableXLNABiasStrengthGet(void);
extern int Ar9300XLANBiasStrengthGet(int *value, int ix, int *num, int iBand);
extern int Ar9300EnableRFGainCAPGet(void);
extern int Ar9300EnableTXGainCAPGet(void);
extern int Ar9300EnableMinCCAPwrThresholdGet(void);
extern int Ar9300RFGainCAPGet(int iBand);
extern int Ar9300TXGainCAPGet(int iBand);

extern int Ar9300_SWREG_Get(void);

extern int Ar9300eepromVersionGet(void);
extern int Ar9300templateVersionGet(void);
extern int Ar9300regDmnGet(int *value, int ix, int *num);
extern int Ar9300txrxMaskGet(void);
extern int Ar9300txMaskGet(void);
extern int Ar9300rxMaskGet(void);
extern int Ar9300opFlagsGet(void);
extern int Ar9300eepMiscGet(void);
extern int Ar9300rfSilentGet(void);
extern int Ar9300rfSilentB0Get(void);
extern int Ar9300rfSilentB1Get(void);
extern int Ar9300rfSilentGPIOGet(void);
extern int Ar9300blueToothOptionsGet(void);
extern int Ar9300deviceCapGet(void);
extern int Ar9300deviceTypeGet(void);
extern int Ar9300pwrTableOffsetGet(void);
extern int Ar9300pwrTuningCapsParamsGet(int *value, int ix, int *num);
extern int Ar9300TxGainGet(void);
extern int Ar9300RxGainGet(void);

extern int Ar9300TempSlopeGet(int *value, int iBand);
extern int Ar9300TempSlopeLowGet(int *value);
extern int Ar9300TempSlopeHighGet(int *value);
extern int Ar9300TempSlopeExtensionGet(int *value, int ix, int *num);
extern int Ar9300VoltSlopeGet(int iBand);
extern int Ar9300QuickDropGet(int iBand);
extern int Ar9300QuickDropLowGet();
extern int Ar9300QuickDropHighGet();
extern int Ar9300xpaBiasLvlGet(int iBand);
extern int Ar9300txFrameToDataStartGet(int iBand);
extern int Ar9300txFrameToPaOnGet(int iBand);
extern int Ar9300txClipGet(int iBand);
extern int Ar9300dac_scale_cckGet(int iBand);
extern int Ar9300antennaGainGet(int iBand);
extern int Ar9300adcDesiredSizeGet(int iBand);
extern int Ar9300switchSettlingGet(int iBand);
extern int Ar9300txEndToXpaOffGet(int iBand);
extern int Ar9300txEndToRxOnGet(int iBand);
extern int Ar9300txFrameToXpaOnGet(int iBand);
extern int Ar9300thresh62Get(int iBand);

//extern A_INT32 Ar9300CalTgtPwrGet(int *pwrArr, int band, int htMode, int iFreqNum);
//extern A_INT32 Ar9300CalTgtFreqGet(int *freqArr, int band, int htMode);

extern A_INT32 Ar9300calFreqTGTcckGet(int *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300calFreqTGTLegacyOFDMGet(int *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300calFreqTGTHT20Get(int *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300calFreqTGTHT40Get(int *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300calTGTPwrCCKGet(double *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300calTGTPwrLegacyOFDMGet(double *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300calTGTPwrHT20Get(double *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300calTGTPwrHT40Get(double *value, int ix, int iy, int iz, int *num, int iBand);

extern A_INT32 Ar9300calFreqPierGet(int *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300calPierDataRefPowerGet(int *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300calPierDataVoltMeasGet(int *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300calPierDataTempMeasGet(int *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300calPierDataRxNoisefloorCalGet(int *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300calPierDataRxNoisefloorPowerGet(int *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300calPierDataRxTempMeasGet(int *value, int ix, int iy, int iz, int *num, int iBand);

extern A_INT32 Ar9300ctlIndexGet(int *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300ctlFreqGet(int *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300ctlPowerGet(double *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300ctlFlagGet(int *value, int ix, int iy, int iz, int *num, int iBand);

extern A_INT32 Ar9300MinCCAPwrThreshChGet(int *value, int ix, int *num, int iBand);
extern A_INT32 Ar9300ReservedGet(int *value, int ix, int *num, int iBand);
extern A_INT32 Ar9300spurChansGet(int *value, int ix, int *num, int iBand);
extern int Ar9300AntCtrlCommonGet(int iBand);
extern int Ar9300AntCtrlCommon2Get(int iBand);
extern A_INT32 Ar9300antCtrlChainGet(int *value, int ix, int *num, int iBand);
extern A_INT32 Ar9300xatten1DBGet(int *value, int ix, int *num, int iBand);
extern A_INT32 Ar9300xatten1MarginGet(int *value, int ix, int *num, int iBand);
extern A_INT32 Ar9300xatten1DBLowGet(int *value, int ix, int *num, int iBand);
extern A_INT32 Ar9300xatten1MarginLowGet(int *value, int ix, int *num, int iBand);
extern A_INT32 Ar9300xatten1DBHighGet(int *value, int ix, int *num, int iBand);
extern A_INT32 Ar9300xatten1MarginHighGet(int *value, int ix, int *num, int iBand);

extern void Ar9300EepromPaPredistortionSet(int value);
extern int Ar9300EepromPaPredistortionGet(void);
extern int Ar9300EepromCalibrationValid(void);


#endif //_OSPREY_EEPROM_STRUCT_H_
