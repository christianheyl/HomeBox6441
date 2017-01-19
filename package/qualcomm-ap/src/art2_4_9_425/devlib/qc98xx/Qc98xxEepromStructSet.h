
#ifndef _QC98XX_EEPROM_STRUCT_SET_H_
#define _QC98XX_EEPROM_STRUCT_SET_H_

#include "ParameterConfigDef.h"
#include "ConfigurationStatus.h"

extern A_INT32 Qc98xxXtalValue(int value);
extern A_INT32 Qc98xxEepromVersion(int value);
extern A_INT32 Qc98xxTemplateVersion(int value);
extern A_INT32 Qc98xxCustomerDataSet(unsigned char *data, int len);
extern A_INT32 Qc98xxMacAddressSet(unsigned char *mac);
extern A_INT32 Qc98xxMiscConfigurationSet(int value);
extern A_INT32 Qc98xxReconfigDriveStrengthSet(int value);
extern A_INT32 Qc98xxThermometerSet(int value);
extern A_INT32 Qc98xxChainMaskReduceSet(int value);
extern A_INT32 Qc98xxReconfigQuickDropSet(int value);		
extern A_INT32 Qc98xxWlanLedGpioSet(int value);
extern A_INT32 Qc98xxSpurBaseASet(int value);
extern A_INT32 Qc98xxSpurBaseBSet(int value);
extern A_INT32 Qc98xxSpurRssiThreshSet(int value);
extern A_INT32 Qc98xxSpurRssiThreshCckSet(int value);
extern A_INT32 Qc98xxSpurMitFlagSet(int value);
extern A_INT32 Qc98xxTxRxGainSet(int value, int iBand);
extern A_INT32 Qc98xxTxGainSet(int value, int iBand);
extern A_INT32 Qc98xxRxGainSet(int value, int iBand);
extern A_INT32 Qc98xxEnableFeatureSet(int value);
extern A_INT32 Qc98xxEnableTempCompensationSet(int value);
extern A_INT32 Qc98xxEnableVoltCompensationSet(int value);
//extern A_INT32 Qc98xxEnableFastClockSet(int value);
extern A_INT32 Qc98xxEnableDoublingSet(int value);
extern A_INT32 Qc98xxEnableTuningCapsSet(int value);
extern A_INT32 Qc98xxInternalRegulatorSet(int value);
extern A_INT32 Qc98xxRbiasSet(int value);
extern A_INT32 Qc98xxBibxosc0Set(int value);
extern A_INT32 Qc98xxFlag1NoiseFlrThrSet(int value);
extern A_INT32 Qc98xxPapdSet(int value);
extern A_INT32 Qc98xxPapdRateMaskHt20Set(int value, int iBand);
extern A_INT32 Qc98xxPapdRateMaskHt40Set(int value, int iBand);
extern A_INT32 Qc98xxFutureSet(int *value, int ix, int iy, int iz, int num, int iBand);
//extern A_INT32 Qc98xxAntDivCtrlSet(int value);
//extern A_INT32 Qc98xxAntDivCtrlSet(int value);
//extern A_INT32 Qc98xxTempSlopeLowSet(int value);
//extern A_INT32 Qc98xxTempSlopeHighSet(int value);

extern A_INT32 Qc98xxSWREGSet(int value);
extern A_INT32 Qc98xxSWREGProgramSet(int value);

extern A_INT32 Qc98xxAntCtrlCommonSet(int value, int iBand);
extern A_INT32 Qc98xxAntCtrlCommon2Set(int value, int iBand);
extern A_INT32 Qc98xxRxFilterCapSet(int value, int iBand);
extern A_INT32 Qc98xxRxGainCapSet(int value, int iBand);
//extern A_INT32 Qc98xxTempSlopeSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxVoltSlopeSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxXpaBiasLvlSet(int value, int iBand);
extern A_INT32 Qc98xxXpaBiasBypassSet(int value, int iBand);
//extern A_INT32 Qc98xxTxFrameToDataStartSet(int value, int iBand);
//extern A_INT32 Qc98xxTxFrameToPaOnSet(int value, int iBand);
//extern A_INT32 Qc98xxTxClipSet(int value, int iBand);
//extern A_INT32 Qc98xxDacScaleCckSet(int value, int iBand);
extern A_INT32 Qc98xxAntennaGainSet(int value, int iBand);
//extern A_INT32 Qc98xxAdcDesiredSizeSet(int value, int iBand);
//extern A_INT32 Qc98xxSwitchSettlingSet(int value, int iBand);
//extern A_INT32 Qc98xxSwitchSettlingHt40Set(int value, int iBand);
//extern A_INT32 Qc98xxTxEndToXpaOffSet(int value, int iBand);
//extern A_INT32 Qc98xxTxEndToRxOnSet(int value, int iBand);
//extern A_INT32 Qc98xxTxFrameToXpaOnSet(int value, int iBand);
//extern A_INT32 Qc98xxThresh62Set(int value, int iBand);

extern A_INT32 Qc98xxPwrTableOffsetSet(int value);
extern A_INT32 Qc98xxPwrTuningCapsParamsSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxDeltaCck20Set(int value);
extern A_INT32 Qc98xxDelta4020Set(int value);
extern A_INT32 Qc98xxDelta8020Set(int value);

extern A_UINT32 Qc98xxThermAdcScaledGainSet(int value);
extern A_UINT32 Qc98xxThermAdcOffsetSet(int value);

extern A_INT32 Qc98xxCalFreqPierSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCalPointTxGainIdxSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCalPointDacGainSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCalPointPowerSet(double *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCalPierDataVoltMeasSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCalPierDataTempMeasSet(int *value, int ix, int iy, int iz, int num, int iBand);

extern A_INT32 Qc98xxCalFreqTGTcckSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCalFreqTGTLegacyOFDMSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCalFreqTGTHT20Set(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCalFreqTGTHT40Set(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCalFreqTGTHT80Set(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCalTGTPwrLegacyOFDMSet(double *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCalTGTPwrCCKSet(double *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCalTGTPwrHT20Set(double *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCalTGTPwrHT40Set(double *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCalTGTPwrHT80Set(double *value, int ix, int iy, int iz, int num, int iBand);

extern A_INT32 Qc98xxCtlIndexSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCtlFreqSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCtlPowerSet(double *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxCtlFlagSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxNoiseFloorSet ( int *value, int ix, int iy, int iz, int num, int iBand );
extern A_INT32 Qc98xxNoiseFloorPowerSet ( int *value, int ix, int iy, int iz, int num, int iBand );
extern A_INT32 Qc98xxNoiseFloorTemperatureSet ( int *value, int ix, int iy, int iz, int num, int iBand );
extern A_INT32 Qc98xxNoiseFloorTemperatureSlopeSet ( int *value, int ix, int iy, int iz, int num, int iBand );


extern A_INT32 Qc98xxObSet(char *tValue, int *value, int iBand);

extern A_INT32 Qc98xxNoiseFloorThreshChSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxSpurChansSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxSpurAPrimSecChooseSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxSpurBPrimSecChooseSet(int *value, int ix, int iy, int iz, int num, int iBand);

//extern A_INT32 Qc98xxQuickDropSet(int value, int iBand);
//extern A_INT32 Qc98xxQuickDropLowSet(int value);
//extern A_INT32 Qc98xxQuickDropHighSet(int value);
extern A_INT32 Qc98xxAntCtrlChainSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxNoiseFlrThrSet(int value, int iBand);
extern A_INT32 Qc98xxMinCcaPwrChainSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxXatten1DBSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxXatten1MarginSet(int *value, int ix, int iy, int iz, int num, int iBand);
//extern A_INT32 Qc98xxXatten1HystSet(int *value, int ix, int iy, int iz, int num, int iBand);
//extern A_INT32 Qc98xxXatten2DBSet(int *value, int ix, int iy, int iz, int num, int iBand);
//extern A_INT32 Qc98xxXatten2MarginSet(int *value, int ix, int iy, int iz, int num, int iBand);
//extern A_INT32 Qc98xxXatten2HystSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxXatten1DBLowSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxXatten1MarginLowSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxXatten1DBHighSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxXatten1MarginHighSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxRegDmnSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Qc98xxTxMaskSet(int value);
extern A_INT32 Qc98xxRxMaskSet(int value);
extern A_INT32 Qc98xxOpFlagsSet(int value);
extern A_INT32 Qc98xxOpFlags2Set(int value);
extern A_INT32 Qc98xxBoardFlagsSet(int value);
extern A_INT32 Qc98xxEepMiscSet(int value);
extern A_INT32 Qc98xxRfSilentSet(int value);
extern A_INT32 Qc98xxRfSilentB0Set(int value);
extern A_INT32 Qc98xxRfSilentB1Set(int value);
extern A_INT32 Qc98xxRfSilentGPIOSet(int value);
extern A_INT32 Qc98xxDeviceCapSet(int value);
extern A_INT32 Qc98xxBlueToothOptionsSet(int value);
extern A_INT32 Qc98xxDeviceTypetSet(int value);
//
// returns 0 on success, negative error code on problem
//
extern int Qc98xxCalInfoCalibrationPierSet(int pierIdx, int freq, int chain, int gain, int gainIndex, int dacGain,
										   double power, int pwrCorrection, int voltMeas, int tempMeas, int calPoint);
extern int Qc98xxCalUnusedPierSet(int iChain, int iBand, int iIndex);
//extern void Qc98xxEepromPaPredistortionSet(int value);

extern A_INT32 Qc98xxRSSICalInfoNoiseFloorSet ( int freq, int chain, int noiseFloorPower_dBr );
extern A_INT32 Qc98xxRSSICalInfoNoiseFloorPowerSet ( int freq, int chain, int noiseFloorPower_dBm );
extern A_INT32 Qc98xxRSSICalInfoNoiseFloorTemperatureSet ( int freq, int chain, int noiseFloorTemperature );


extern int Qc98xxAlphaThermTableSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern int Qc98xxConfigAddrSet1(int *value, int ix, int iy, int iz, int num, int iBand);

extern int Qc98xxConfigAddrSet(unsigned int address, int low, int  high, unsigned int *value, int numVal, int prepost);

#endif //_QC98XX_EEPROM_STRUCT_SET_H_
