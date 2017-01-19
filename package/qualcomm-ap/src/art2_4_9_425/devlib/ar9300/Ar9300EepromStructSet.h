


#include "ParameterConfigDef.h"
#include "ConfigurationStatus.h"

extern ar9300_eeprom_t *Ar9300EepromStructGet(void);
extern A_INT32 Ar9300eepromVersion(int value);
extern A_INT32 Ar9300templateVersion(int value);
extern A_INT32 Ar9300CustomerDataSet(unsigned char *data, A_INT32 len);
extern A_INT32 Ar9300MacAddressSet(unsigned char *mac);
extern A_INT32 Ar9300FutureSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300AntDivCtrlSet(int value);
extern A_INT32 Ar9300ReconfigDriveStrengthSet(int value);
extern A_INT32 Ar9300ReconfigQuickDropSet(int value);
extern A_INT32 Ar9300ReconfigTempSlopExtensionSet(int value);
extern A_INT32 Ar9300ThermometerGet(void);
extern A_INT32 Ar9300ThermometerSet(int value);
extern A_INT32 Ar9300ChainMaskReduceGet(void);
extern A_INT32 Ar9300ChainMaskReduceSet(int value);
extern A_INT32 Ar9300TxGainSet(int value);
extern A_INT32 Ar9300RxGainSet(int value);
extern A_INT32 Ar9300EnableTempCompensationSet(int value);
extern A_INT32 Ar9300EnableVoltCompensationSet(int value);
extern A_INT32 Ar9300EnableFastClockSet(int value);
extern A_INT32 Ar9300EnableDoublingSet(int value);
extern A_INT32 Ar9300InternalRegulatorSet(int value);
extern A_INT32 Ar9300PapdSet(int value);
extern A_INT32 Ar9300EnableTuningCapsSet(int value);
extern A_INT32 Ar9300EnableTxFrameToXpaOnSet(int value);
extern A_INT32 Ar9300PapdRateMaskHt20Set(int value, int iBand);
extern A_INT32 Ar9300PapdRateMaskHt40Set(int value, int iBand);
extern A_INT32 Ar9300WlanSpdtSwitchGlobalControlSet(int value, int iBand);
extern A_INT32 Ar9300EnableXLNABiasStrengthSet(int value);
extern A_INT32 Ar9300XLANBiasStrengthSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300EnableRFGainCAPSet(int value);
extern A_INT32 Ar9300RFGainCAPSet(int value, int iBand);
extern A_INT32 Ar9300EnableTXGainCAPSet(int value);
extern A_INT32 Ar9300TXGainCAPSet(int value, int iBand);
extern A_INT32 Ar9300EnableMinCCAPwrThresholdSet(int value);
extern A_INT32 Ar9300_SWREG_Set(int value);
extern A_INT32 Ar9300_SWREG_PROGRAM_Set(int value);

extern A_INT32 Ar9300AntCtrlCommonSet(int value, int iBand);
extern A_INT32 Ar9300AntCtrlCommon2Set(int value, int iBand);
extern A_INT32 Ar9300TempSlopeSet(int *value, int ix, int iy, int iz,int num, int iBand);
extern A_INT32 Ar9300VoltSlopeSet(int value, int iBand);
extern A_INT32 Ar9300TempSlopeLowSet(int *value, int ix, int iy, int iz);
extern A_INT32 Ar9300TempSlopeHighSet(int *value, int ix, int iy, int iz );
extern A_INT32 Ar9300TempSlopeExtensionSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300QuickDropSet(int value, int iBand);
extern A_INT32 Ar9300QuickDropLowSet(int value);
extern A_INT32 Ar9300QuickDropHighSet(int value);
extern A_INT32 Ar9300xpaBiasLvlSet(int value, int iBand);
extern A_INT32 Ar9300txFrameToDataStartSet(int value, int iBand);
extern A_INT32 Ar9300txFrameToPaOnSet(int value, int iBand);
extern A_INT32 Ar9300txClipSet(int value, int iBand);
extern A_INT32 Ar9300dac_scale_cckSet(int value, int iBand);
extern A_INT32 Ar9300antennaGainSet(int value, int iBand);
extern A_INT32 Ar9300adcDesiredSizeSet(int value, int iBand);
extern A_INT32 Ar9300switchSettlingSet(int value, int iBand);
extern A_INT32 Ar9300txEndToXpaOffSet(int value, int iBand);
extern A_INT32 Ar9300txEndToRxOnSet(int value, int iBand);
extern A_INT32 Ar9300txFrameToXpaOnSet(int value, int iBand);
extern A_INT32 Ar9300thresh62Set(int value, int iBand);

extern A_INT32 Ar9300pwrTableOffsetSet(int value);
extern A_INT32 Ar9300pwrTuningCapsParamsSet(int *value, int ix, int iy, int iz, int num, int iBand);

extern A_INT32 Ar9300calFreqTGTcckSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300calFreqTGTLegacyOFDMSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300calFreqTGTHT20Set(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300calFreqTGTHT40Set(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300calTGTPwrLegacyOFDMSet(double *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300calTGTPwrCCKSet(double *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300calTGTPwrHT20Set(double *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300calTGTPwrHT40Set(double *value, int ix, int iy, int iz, int num, int iBand);

extern A_INT32 Ar9300calFreqPierSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300calPierDataRefPowerSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300calPierDataVoltMeasSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300calPierDataTempMeasSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300calPierDataRxNoisefloorCalSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300calPierDataRxNoisefloorPowerSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300calPierDataRxTempMeaSet(int *value, int ix, int iy, int iz, int num, int iBand);

extern A_INT32 Ar9300ctlIndexSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300ctlFreqSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300ctlFlagSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300ctlPowerSet(double *value, int ix, int iy, int iz, int num, int iBand);

extern int Ar9300EepromWriteEnableGpioSet(int value);
extern int Ar9300EepromWriteEnableGpioGet(void);

extern int Ar9300WlanDisableGpioSet(int value);
extern int Ar9300WlanDisableGpioGet(void);

extern int Ar9300WlanLedGpioSet(int value);
extern int Ar9300WlanLedGpioGet(void);

extern int Ar9300RxBandSelectGpioSet(int value);
extern int Ar9300RxBandSelectGpioGet(void);
extern A_INT32 Ar9300MinCCAPwrThreshChSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300ReservedSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300spurChansSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300antCtrlChainSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300xatten1DBSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300xatten1MarginSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300xatten1DBLowSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300xatten1MarginLowSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300xatten1DBHighSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300xatten1MarginHighSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300regDmnSet(int *value, int ix, int iy, int iz, int num, int iBand);
extern A_INT32 Ar9300txMaskSet(int value);
extern A_INT32 Ar9300rxMaskSet(int value);
extern A_INT32 Ar9300opFlagsSet(int value);
extern A_INT32 Ar9300eepMiscSet(int value);
extern A_INT32 Ar9300rfSilentSet(int value);
extern A_INT32 Ar9300rfSilentB0Set(int value);
extern A_INT32 Ar9300rfSilentB1Set(int value);
extern A_INT32 Ar9300rfSilentGPIOSet(int value);
extern A_INT32 Ar9300deviceCapSet(int value);
extern A_INT32 Ar9300blueToothOptionsSet(int value);
extern A_INT32 Ar9300deviceTypetSet(int value);
//
// returns 0 on success, negative error code on problem
//
extern AR9300DLLSPEC int Ar9300NoiseFloorSet(int frequency, int ichain, int nf);
extern AR9300DLLSPEC int Ar9300NoiseFloorPowerSet(int frequency, int ichain, int nfpower);
extern int Ar9300NoiseFloorTemperatureSet(int frequency, int ichain, int temperature);

extern A_INT32 Ar9300EnableFeatureSet(int value);
extern A_INT32 Ar9300MiscConfigurationSet(int value);
extern A_INT32 Ar9300CaldataMemoryTypeSet(A_UCHAR *memType);

extern A_INT32 Ar9300PsatPowerSet(double *value, int ix, int iy, int iz, int num, int iBand);
extern int Ar9300PsatPowerGet(double *value, int ix, int iy, int iz, int *num, int iBand);
extern int Ar9300PsatDiffGet(double *value, int ix, int iy, int iz, int *num, int iBand);
extern A_INT32 Ar9300PsatDiffSet(double *value, int ix, int iy, int iz, int num, int iBand);

