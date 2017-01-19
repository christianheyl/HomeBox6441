

#define Ar9287EepromALL "ALL"
#define Ar9287EepromConfig "Config"
#define Ar9287EepromConfigPCIe "ConfigPCIe"
#define Ar9287EepromDeviceId "DeviceID"
#define Ar9287EepromSSID "SSID"
#define Ar9287EepromVID "VID"
#define Ar9287EepromSVID "SVID"



enum
{
    Ar9287SetEepromVersion=10000,
	Ar9287SetEepromChecksum,
    Ar9287SetEepromTemplateVersion,
	Ar9287SetEepromALL,
	Ar9287SetEepromConfig,
	Ar9287SetEepromConfigPCIe,
	Ar9287SetEepromDeviceId,
	Ar9287SetEepromSSID,
	Ar9287SetEepromVID,
	Ar9287SetEepromSVID,
    
	Ar9287SetEepromMacAddress,
    Ar9287SetEepromCustomerData,
    Ar9287SetEepromRegulatoryDomain,
    Ar9287SetEepromTxRxMask,
	    Ar9287SetEepromTxRxMaskTx,
	    Ar9287SetEepromTxRxMaskRx,
    Ar9287SetEepromOpFlags,
    Ar9287SetEepromEepMisc,
    Ar9287SetEepromRfSilent,
		Ar9287SetEepromRfSilentB0,
		Ar9287SetEepromRfSilentB1,
		Ar9287SetEepromRfSilentGpio,
    Ar9287SetEepromBlueToothOptions,
    Ar9287SetEepromDeviceCapability,
	Ar9287SetEepromBinBuildNumber,
    Ar9287SetEepromDeviceType,
    Ar9287SetEepromPowerTableOffset,
    Ar9287SetEepromTuningCaps,
    Ar9287SetEepromFeatureEnable,
	    Ar9287SetEepromFeatureEnableTemperatureCompensation,
	    Ar9287SetEepromFeatureEnableVoltageCompensation,
	    Ar9287SetEepromFeatureEnableFastClock,
	    Ar9287SetEepromFeatureEnableDoubling,
	    Ar9287SetEepromFeatureEnableInternalSwitchingRegulator,
	    Ar9287SetEepromFeatureEnablePaPredistortion,
	    Ar9287SetEepromFeatureEnableTuningCaps,
	    Ar9287SetEepromFeatureEnableTxFrameToXpaOn,
    Ar9287SetEepromMiscellaneous,
	    Ar9287SetEepromMiscellaneousDriveStrength,
	    Ar9287SetEepromMiscellaneousThermometer,
	    Ar9287SetEepromMiscellaneousChainMaskReduce,
		Ar9287SetEepromMiscellaneousQuickDropEnable,
    	Ar9287SetEepromMiscellaneousTempSlopExtensionEnable,
		Ar9287SetEepromMiscellaneousXLNABiasStrengthEnable,
		Ar9287SetEepromMiscellaneousRFGainCAPEnable,
    Ar9287SetEepromEepromWriteEnableGpio,
    Ar9287SetEepromWlanDisableGpio,
    Ar9287SetEepromWlanLedGpio,
    Ar9287SetEepromRxBandSelectGpio,
    Ar9287SetEepromTxRxGain,
	    Ar9287SetEepromTxRxGainTx,
	    Ar9287SetEepromTxRxGainRx,
    Ar9287SetEepromSwitchingRegulator,
		Ar9287SetEepromSWREGProgram,
    Ar9287SetEeprom2GHzAntennaControlCommon,
    Ar9287SetEeprom2GHzAntennaControlCommon2,
    Ar9287SetEeprom2GHzAntennaControlChain,
    Ar9287SetEeprom2GHzAttenuationDb,
    Ar9287SetEeprom2GHzAttenuationMargin,
    Ar9287SetEeprom2GHzTemperatureSlope,
    Ar9287SetEeprom2GHzTemperatureSlopePalOn,
	Ar9287SetEeprom2GHzFutureBase,
    Ar9287SetEeprom2GHzVoltageSlope,
    Ar9287SetEeprom2GHzNoiseFloorThreshold,
	Ar9287SetEeprom2GHzXpdGain,
	Ar9287SetEeprom2GHzXpd,
	Ar9287SetEeprom2GHzIqCalICh,
	Ar9287SetEeprom2GHzIqCalQCh,
	Ar9287SetEeprom2GHzPdGainOverLap,
	Ar9287SetEeprom2GHzXpaBiasLevel,	 
	Ar9287SetEeprom2GHzTxFrameToDataStart,		 
	Ar9287SetEeprom2GHzTxFrameToPaOn,  
	Ar9287SetEeprom2GHzHt40PowerIncForPdadc,
	Ar9287SetEeprom2GHzBswAtten,
	Ar9287SetEeprom2GHzBswMargin,
	Ar9287SetEeprom2GHzSwSettleHt40,
	Ar9287SetEeprom2GHzModalHeaderversion,
	Ar9287SetEeprom2GHzDb1,
	Ar9287SetEeprom2GHzDb2,
	Ar9287SetEeprom2GHzObCck,
	Ar9287SetEeprom2GHzObPsk,
	Ar9287SetEeprom2GHzObQam,
	Ar9287SetEeprom2GHzObPalOff,
	Ar9287SetEeprom2GHzFutureModal,	
    Ar9287SetEeprom2GHzSpur,
    Ar9287SetEeprom2GHzSpurRangeLow,
    Ar9287SetEeprom2GHzSpurRangeHigh,

	Ar9287SetEeprom2GHzReserved,
	Ar9287SetEeprom2GHzQuickDrop,
    Ar9287SetEeprom2GHzTxClip,
	Ar9287SetEeprom2GHzDacScaleCCK,
    Ar9287SetEeprom2GHzAntennaGain,
    Ar9287SetEeprom2GHzSwitchSettling,
    Ar9287SetEeprom2GHzAdcSize,
    Ar9287SetEeprom2GHzTxEndToXpaOff,		
    Ar9287SetEeprom2GHzTxEndToRxOn,			
    Ar9287SetEeprom2GHzTxFrameToXpaOn,      
    Ar9287SetEeprom2GHzThresh62,
    Ar9287SetEeprom2GHzPaPredistortionHt20,
    Ar9287SetEeprom2GHzPaPredistortionHt40,
    Ar9287SetEeprom2GHzWlanSpdtSwitchGlobalControl,
    Ar9287SetEeprom2GHzXLNABiasStrength,
    Ar9287SetEeprom2GHzRFGainCAP,
    Ar9287SetEeprom2GHzFuture,
    Ar9287SetEepromAntennaDiversityControl,
	Ar9287SetEeprom5GHzQuickDropLow,
	Ar9287SetEeprom5GHzQuickDropHigh,
    Ar9287SetEepromFuture,										
    Ar9287SetEeprom2GHzCalibrationFrequency,
    Ar9287SetEeprom2GHzCalibrationData,
    Ar9287SetEeprom2GHzPowerCorrection,
    Ar9287SetEeprom2GHzCalibrationVoltage,
    Ar9287SetEeprom2GHzCalibrationTemperature,
	Ar9287SetEeprom2GHzCalibrationPcdac,
    Ar9287SetEeprom2GHzNoiseFloor,
    Ar9287SetEeprom2GHzNoiseFloorPower,
    Ar9287SetEeprom2GHzNoiseFloorTemperature,
	Ar9287SetEeprom2GHzTargetDataCck,
    Ar9287SetEeprom2GHzTargetFrequencyCck,
	Ar9287SetEeprom2GHzTargetPowerCck,
    Ar9287SetEeprom2GHzTargetData,
    Ar9287SetEeprom2GHzTargetFrequency,
	Ar9287SetEeprom2GHzTargetPower,
    Ar9287SetEeprom2GHzTargetDataHt20,
    Ar9287SetEeprom2GHzTargetFrequencyHt20,
	Ar9287SetEeprom2GHzTargetPowerHt20,
    Ar9287SetEeprom2GHzTargetDataHt40,
    Ar9287SetEeprom2GHzTargetFrequencyHt40,
    Ar9287SetEeprom2GHzTargetPowerHt40,
    Ar9287SetEeprom2GHzCtlIndex,
    Ar9287SetEeprom2GHzCtlData,
    Ar9287SetEeprom2GHzPadding,
    Ar9287SetEeprom2GHzCtlFrequency,
    Ar9287SetEeprom2GHzCtlPower,
	Ar9287SetEeprom2GHzCtlBandEdge,
	Ar9287SetEeprom2GHzCaldataMemoryType,
	Ar9287SetEeprom2GHzLengthEeprom,
	Ar9287SetEeprom2GHzOpenLoopPowerControl,
	Ar9287SetEeprom2GHzTxRxAtten,
	Ar9287SetEeprom2GHzRxTxMargin,
	Ar9287SetEeprom2GHzCalFreqPierSet,

    Ar9287SetEeprom5GHzAntennaControlCommon,
    Ar9287SetEeprom5GHzAntennaControlCommon2,
    Ar9287SetEeprom5GHzAntennaControlChain,
    Ar9287SetEeprom5GHzAttenuationDb,
    Ar9287SetEeprom5GHzAttenuationMargin,
    Ar9287SetEeprom5GHzTemperatureSlope,
    Ar9287SetEeprom5GHzVoltageSlope,
    Ar9287SetEeprom5GHzSpur,
    Ar9287SetEeprom5GHzNoiseFloorThreshold,
	Ar9287SetEeprom5GHzReserved,
	Ar9287SetEeprom5GHzQuickDrop,
    Ar9287SetEeprom5GHzXpaBiasLevel,    
    Ar9287SetEeprom5GHzTxFrameToDataStart,
    Ar9287SetEeprom5GHzTxFrameToPaOn,
    Ar9287SetEeprom5GHzTxClip,
    Ar9287SetEeprom5GHzAntennaGain,
    Ar9287SetEeprom5GHzSwitchSettling,
    Ar9287SetEeprom5GHzAdcSize,
    Ar9287SetEeprom5GHzTxEndToXpaOff,		
    Ar9287SetEeprom5GHzTxEndToRxOn,	
    Ar9287SetEeprom5GHzTxFrameToXpaOn,
    Ar9287SetEeprom5GHzThresh62,
    Ar9287SetEeprom5GHzPaPredistortionHt20,
    Ar9287SetEeprom5GHzPaPredistortionHt40,
    Ar9287SetEeprom5GHzWlanSpdtSwitchGlobalControl,
    Ar9287SetEeprom5GHzXLNABiasStrength,
    Ar9287SetEeprom5GHzRFGainCAP,
    Ar9287SetEeprom5GHzFuture,				
    Ar9287SetEeprom5GHzTemperatureSlopeLow,
    Ar9287SetEeprom5GHzTemperatureSlopeHigh,
    Ar9287SetEeprom5GHzTemperatureSlopeExtension,
    Ar9287SetEeprom5GHzAttenuationDbLow,
    Ar9287SetEeprom5GHzAttenuationDbHigh,
    Ar9287SetEeprom5GHzAttenuationMarginLow,
    Ar9287SetEeprom5GHzAttenuationMarginHigh,
    Ar9287SetEeprom5GHzCalibrationFrequency,
    Ar9287SetEeprom5GHzCalibrationData,
	Ar9287SetEeprom5GHzPowerCorrection,
	Ar9287SetEeprom5GHzCalibrationVoltage,
	Ar9287SetEeprom5GHzCalibrationTemperature,
	Ar9287SetEeprom5GHzNoiseFloor,
	Ar9287SetEeprom5GHzNoiseFloorPower,
	Ar9287SetEeprom5GHzNoiseFloorTemperature,
    Ar9287SetEeprom5GHzTargetFrequency,
    Ar9287SetEeprom5GHzTargetFrequencyHt20,
    Ar9287SetEeprom5GHzTargetFrequencyHt40,
    Ar9287SetEeprom5GHzTargetPower,
    Ar9287SetEeprom5GHzTargetPowerHt20,
    Ar9287SetEeprom5GHzTargetPowerHt40,
    Ar9287SetEeprom5GHzCtlIndex,
    Ar9287SetEeprom5GHzCtlFrequency,
    Ar9287SetEeprom5GHzCtlPower,
	Ar9287SetEeprom5GHzCtlBandEdge
};

static int ThermometerMinimum=-1;
static int ThermometerMaximum=2;
static int ThermometerDefault=1;

static unsigned int TwoBitsMinimum=0;
static unsigned int TwoBitsMaximum=0x3;
static unsigned int TwoBitsDefault=0;

static unsigned int SixBitsMinimum=0;
static unsigned int SixBitsMaximum=0x3f;
static unsigned int SixBitsDefault=0;

static unsigned int HalfByteMinimum=0;
static unsigned int HalfByteMaximum=0xf;
static unsigned int HalfByteDefault=0;

static unsigned int UnsignedByteMinimum=0;
static unsigned int UnsignedByteMaximum=0xff;
static unsigned int UnsignedByteDefault=0;

static int SignedByteMinimum=-127;
static int SignedByteMaximum=127;
static int SignedByteDefault=0;

static unsigned int UnsignedShortMinimum=0;
static unsigned int UnsignedShortMaximum=0xffff;
static unsigned int UnsignedShortDefault=0;

static int SignedShortMinimum=-32767;
static int SignedShortMaximum=32767;
static int SignedShortDefault=0;

static unsigned int UnsignedIntMinimum=0;
static unsigned int UnsignedIntMaximum=0xffffffff;
static unsigned int UnsignedIntDefault=0;

static int SignedIntMinimum= -1000000;
static int SignedIntMaximum=0x3fffffff;
static int SignedIntDefault=0;

static int FreqZeroMinimum=0;
static int FrequencyMinimum2GHz=2300;
static int FrequencyMaximum2GHz=2600;
static int FrequencyDefault2GHz=2412;

static int FrequencyMinimum5GHz=4000;
static int FrequencyMaximum5GHz=7000;
static int FrequencyDefault5GHz=5180;

static double PowerMinimum=0.0;
static double PowerMaximum=35.0;
static double PowerDefault=10.0;

static int PowerOffsetMinimum=-10;
static int PowerOffsetMaximum=35;
static int PowerOffsetDefault=0;


static int Ar9287EepromVersionMinimum=2;		// check this
static int Ar9287EepromVersionDefault=2;

static int Ar9287TemplateVersionMinimum=0;		// get from eep.h
static int Ar9287TemplateVersionDefault=0;
//want template list

static struct _ParameterList LogicalParameter[]=
{
	{0,{"no",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{1,{"yes",0,0},0,0,0,0,0,0,0,0,0,0,0},
};

static int ChainMaskMinimum=1;
static int ChainMaskMaximum=7;
static int ChainMaskDefault=7;



#define AR9287_SET_EEPROM_ALL {Ar9287SetEepromALL,{Ar9287EepromALL,0,0},"formatted display of all configuration and calibration data",'t',0,1,1,1,\
	0,0,0,0,0}

#define AR9287_SET_CONFIG {Ar9287SetEepromConfig,{Ar9287EepromConfig,0,0},"",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9287_SET_CONFIG_PCIE {Ar9287SetEepromConfigPCIe,{Ar9287EepromConfigPCIe,0,0},"",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9287_SET_EEPROM_DEVICEID {Ar9287SetEepromDeviceId,{Ar9287EepromDeviceId,"devid", 0},"the device id",'x',0,1,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define AR9287_SET_EEPROM_SSID {Ar9287SetEepromSSID,{Ar9287EepromSSID,"subSystemId",0},"the subsystem id",'x',0,1,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define AR9287_SET_EEPROM_VID {Ar9287SetEepromVID,{Ar9287EepromVID,"vendorId",0},"the vendor id",'x',0,1,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define AR9287_SET_EEPROM_SVID {Ar9287SetEepromSVID,{Ar9287EepromSVID,"subVendorId",0},"the subvendor id",'x',0,1,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define AR9287_SET_EEPROM_VERSION {Ar9287SetEepromVersion,{Ar9287EepromVersion,"eepversion","version"},"the calibration structure version number",'u',0,1,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define AR9287_SET_TEMPLATE_VERSION {Ar9287SetEepromTemplateVersion,{Ar9287EepromTemplateVersion,"eepversion",0},"the template number",'u',0,1,1,1,\
	&Ar9287TemplateVersionMinimum,&UnsignedByteMaximum,&Ar9287TemplateVersionDefault,0,0}

#define AR9287_SET_EEPROM_CHECKSUM {Ar9287SetEepromChecksum,{Ar9287EepromChecksum,"checksum",0},"checksum of the calibration structure",'u',0,1,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define AR9287_SET_EEPROM_BINBUILDNUMBER {Ar9287SetEepromBinBuildNumber,{Ar9287EepromBinBuildNumber,0,0},"the bin build number",'u',0,1,1,1,\
	&SignedIntMinimum,&SignedIntMaximum,&SignedIntDefault,0,0}

#define AR9287_SET_MAC_ADDRESS {Ar9287SetEepromMacAddress,{Ar9287EepromMacAddress,"macAddr",0},"the mac address of the device",'m',0,1,1,1,\
	0,0,0,0,0}

#define AR9287_SET_CUSTOMER_DATA {Ar9287SetEepromCustomerData,{Ar9287EepromCustomerData,"custData",0},"any text, usually used for device serial number",'t',0,1,1,1,\
	0,0,0,0,0}

#define AR9287_SET_REGULATORY_DOMAIN {Ar9287SetEepromRegulatoryDomain,{Ar9287EepromRegulatoryDomain,"regDmn",0},"the regulatory domain",'x',0,2,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}			// want list of allowed values

#define AR9287_SET_TX_RX_MASK {Ar9287SetEepromTxRxMask,{Ar9287EepromTxRxMask,"txrxMask",0},"the transmit and receive chain masks",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_TX_RX_MASK_TX {Ar9287SetEepromTxRxMaskTx,{Ar9287EepromTxRxMaskTx,"TxMask",0},"the maximum chain mask used for transmit",'x',0,1,1,1,\
	&ChainMaskMinimum,&ChainMaskMaximum,&ChainMaskDefault,0,0}

#define AR9287_SET_TX_RX_MASK_RX {Ar9287SetEepromTxRxMaskRx,{Ar9287EepromTxRxMaskRx,"RxMask",0},"the maximum chain mask used for receive",'x',0,1,1,1,\
	&ChainMaskMinimum,&ChainMaskMaximum,&ChainMaskDefault,0,0}

#define AR9287_SET_OP_FLAGS {Ar9287SetEepromOpFlags,{Ar9287EepromOpFlags,"opFlags",0},"flags that control operating modes",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_EEP_MISC {Ar9287SetEepromEepMisc,{Ar9287EepromEepMisc,"eepMisc",0},"some miscellaneous control flags",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_RF_SILENT {Ar9287SetEepromRfSilent,{Ar9287EepromRfSilent,0,0},"rf silent mode control word",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_RF_SILENT_B0 {Ar9287SetEepromRfSilentB0,{Ar9287EepromRfSilentB0,"rfSilentB0",0},"implement rf silent mode in hardware",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_RF_SILENT_B1 {Ar9287SetEepromRfSilentB1,{Ar9287EepromRfSilentB1,"rfSilentB1",0},"polarity of the rf silent control line",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_RF_SILENT_GPIO {Ar9287SetEepromRfSilentGpio,{Ar9287EepromRfSilentGpio,"rfSilentGpio",0},"the chip gpio line used for rf silent",'x',0,1,1,1,\
	&SixBitsMinimum,&SixBitsMaximum,&SixBitsDefault,0,0}

#define AR9287_SET_BLUETOOTH_OPTIONS {Ar9287SetEepromBlueToothOptions,{Ar9287EepromBlueToothOptions,0,0},"bluetooth options",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_DEVICE_CAPABILITY {Ar9287SetEepromDeviceCapability,{Ar9287EepromDeviceCapability,"DeviceCap",0},"device capabilities",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_DEVICE_TYPE {Ar9287SetEepromDeviceType,{Ar9287EepromDeviceType,"DeviceType",0},"devicetype",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_POWER_TABLE_OFFSET {Ar9287SetEepromPowerTableOffset,{Ar9287EepromPowerTableOffset,"PwrTableOffset",0},"power level of the first entry in the power table",'d',"dBm",1,1,1,\
	&PowerOffsetMinimum,&PowerOffsetMaximum,&PowerOffsetDefault,0,0}

#define AR9287_SET_TUNING {Ar9287SetEepromTuningCaps,{Ar9287EepromTuningCaps,0,0},"capacitors for tuning frequency accuracy",'x',0,2,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_FEATURE_ENABLE {Ar9287SetEepromFeatureEnable,{Ar9287EepromFeatureEnable,"featureEnable",0},"feature enable control word",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_FEATURE_ENABLE_TEMPERATURE_COMPENSATION {Ar9287SetEepromFeatureEnableTemperatureCompensation,{Ar9287EepromFeatureEnableTemperatureCompensation,"TemperatureCompensationEnable","TempCompEnable"},"enables temperature compensation on transmit power control",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_FEATURE_ENABLE_VOLTAGE_COMPENSATION {Ar9287SetEepromFeatureEnableVoltageCompensation,{Ar9287EepromFeatureEnableVoltageCompensation,"VoltageCompensationEnable","VoltCompEnable"},"enables voltage compensation on transmit power control",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_FEATURE_ENABLE_FAST_CLOCK {Ar9287SetEepromFeatureEnableFastClock,{Ar9287EepromFeatureEnableFastClock,"FastClockEnable",0},"enables fast clock mode",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_FEATURE_ENABLE_DOUBLING {Ar9287SetEepromFeatureEnableDoubling,{Ar9287EepromFeatureEnableDoubling,"DoublingEnable",0},"enables doubling mode",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_FEATURE_ENABLE_INTERNAL_SWITCHING_REGULATOR {Ar9287SetEepromFeatureEnableInternalSwitchingRegulator,{Ar9287EepromFeatureEnableInternalSwitchingRegulator,"swregenable",0},"enables the internal switching regulator",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_FEATURE_ENABLE_PA_PREDISTORTION {Ar9287SetEepromFeatureEnablePaPredistortion,{Ar9287EepromFeatureEnablePaPredistortion,"papdenable",0},"enables pa predistortion",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_FEATURE_ENABLE_TUNING_CAPS {Ar9287SetEepromFeatureEnableTuningCaps,{Ar9287EepromFeatureEnableTuningCaps,"TuningCapsEnable",0},"enables use of tuning capacitors",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_FEATURE_ENABLE_TX_FRAME_TO_XPA_ON {Ar9287SetEepromFeatureEnableTxFrameToXpaOn,{Ar9287EepromFeatureEnableTxFrameToXpaOn,"TxFrameToXpaOnEnable",0},"enables use of TxFrameToXpaOn",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_MISCELLANEOUS {Ar9287SetEepromMiscellaneous,{Ar9287EepromMiscellaneous,0,0},"miscellaneous parameters",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_MISCELLANEOUS_DRIVERS_STRENGTH {Ar9287SetEepromMiscellaneousDriveStrength,{Ar9287EepromMiscellaneousDriveStrength,"DriveStrengthReconfigure","DriveStrength"},"enables drive strength reconfiguration",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_MISCELLANEOUS_THERMOMETER {Ar9287SetEepromMiscellaneousThermometer,{Ar9287EepromMiscellaneousThermometer,"Thermometer",0},"forces use of the specified chip thermometer",'d',0,1,1,1,\
	&ThermometerMinimum,&ThermometerMaximum,&ThermometerDefault,0,0}

#define AR9287_SET_MISCELLANEOUS_CHAIN_MASK_REDUCE {Ar9287SetEepromMiscellaneousChainMaskReduce,{Ar9287EepromMiscellaneousChainMaskReduce,"ChainMaskReduce",0},"enables dynamic 2x3 mode to reduce power draw",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_MISCELLANEOUS_QUICK_DROP_ENABLE {Ar9287SetEepromMiscellaneousQuickDropEnable,{Ar9287EepromMiscellaneousQuickDropEnable,"quickDrop",0},"enables quick drop mode for improved strong signal response",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_MISCELLANEOUS_TEMP_SLOP_EXTENSION_ENABLE {Ar9287SetEepromMiscellaneousTempSlopExtensionEnable,{Ar9287EepromMiscellaneousTempSlopExtensionEnable,"tempslopextension",0},"enables temp slop extension",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_MISCELLANEOUS_XLNA_BIAS_STRENGTH_ENABLE {Ar9287SetEepromMiscellaneousXLNABiasStrengthEnable,{Ar9287EepromMiscellaneousXLNABiasStrengthEnable,"xLNABiasStrengthEnable",0},"enables XLNA bias strength set",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_MISCELLANEOUS_RF_GAIN_CAP_ENABLE {Ar9287SetEepromMiscellaneousRFGainCAPEnable,{Ar9287EepromMiscellaneousRFGainCAPEnable,"rfGainCAPEnable",0},"enables rf gain cap set",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9287_SET_EEPROM_WRITE_ENABLE_GPIO {Ar9287SetEepromEepromWriteEnableGpio,{Ar9287EepromEepromWriteEnableGpio,0,0},"gpio line used to enable the eeprom",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_WLAN_DISABLE_GPIO {Ar9287SetEepromWlanDisableGpio,{Ar9287EepromWlanDisableGpio,0,0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_WLAN_LED_GPIO {Ar9287SetEepromWlanLedGpio,{Ar9287EepromWlanLedGpio,0,0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_RX_BAND_SELECT_GPIO {Ar9287SetEepromRxBandSelectGpio,{Ar9287EepromRxBandSelectGpio,0,0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_TX_RX_GAIN {Ar9287SetEepromTxRxGain,{Ar9287EepromTxRxGain,0,0},"transmit and receive gain table control word",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_TX_RX_GAIN_TX {Ar9287SetEepromTxRxGainTx,{Ar9287EepromTxRxGainTx,"TxGain","TxGainTable"},"transmit gain table used",'x',0,1,1,1,\
	&HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define AR9287_SET_TX_RX_GAIN_RX {Ar9287SetEepromTxRxGainRx,{Ar9287EepromTxRxGainRx,"RxGain","RxGainTable"},"receive gain table used",'x',0,1,1,1,\
	&HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define AR9287_SET_SWITCHING_REGULATOR {Ar9287SetEepromSwitchingRegulator,{Ar9287EepromSwitchingRegulator,"SWREG","internalregulator"},"the internal switching regulator control word",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9287_SET_2GHZ_ANTENNA_CONTROL_CHAIN {Ar9287SetEeprom2GHzAntennaControlChain,{Ar9287Eeprom2GHzAntennaControlChain,"antCtrlChain2g",0},"per chain antenna switch control word",'x',0,AR9287_MAX_CHAINS,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define AR9287_SET_2GHZ_ANTENNA_CONTROL_COMMON {Ar9287SetEeprom2GHzAntennaControlCommon,{Ar9287Eeprom2GHzAntennaControlCommon,"AntCtrlCommon2g",0},"antenna switch control word 1",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9287_SET_2GHZ_ANTENNA_CONTROL_COMMON_2 {Ar9287SetEeprom2GHzAntennaControlCommon2,{Ar9287Eeprom2GHzAntennaControlCommon2,"AntCtrlCommon22g",0},"antenna switch control word 2",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9287_SET_2GHZ_ATTENUATION_DB {Ar9287SetEeprom2GHzAttenuationDb,{Ar9287Eeprom2GHzAttenuationDb,"xatten1DB2g",0},"attenuation value",'x',0,AR9287_MAX_CHAINS,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_ATTENUATION_MARGIN {Ar9287SetEeprom2GHzAttenuationMargin,{Ar9287Eeprom2GHzAttenuationMargin,"xatten1margin2g",0},"attenuation margin",'x',0,AR9287_MAX_CHAINS,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_TEMPERATURE_SLOPE {Ar9287SetEeprom2GHzTemperatureSlope,{Ar9287Eeprom2GHzTemperatureSlope,"tempSlope2g","TemperatureSlope2g"},"slope used in temperature compensation algorithm",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_TEMPERATURE_SLOPE_PAL_ON {Ar9287SetEeprom2GHzTemperatureSlopePalOn,{Ar9287Eeprom2GHzTemperatureSlopePalOn,"tempSlope2gPalOn","TemperatureSlopePalOn2g"},"slope pal on used in temperature compensation algorithm",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_FUTUREBASE {Ar9287SetEeprom2GHzFutureBase,{Ar9287Eeprom2GHzFutureBase,0,0},"future base",'d',0,29,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_VOLTAGE_SLOPE {Ar9287SetEeprom2GHzVoltageSlope,{Ar9287Eeprom2GHzVoltageSlope,"voltSlope2g","VoltageSlope2g"},"slope used in voltage compensation algorithm",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_SPUR {Ar9287SetEeprom2GHzSpur,{Ar9287Eeprom2GHzSpur,"SpurChans2g",0},"spur frequencies",'u',"MHz",AR9287_EEPROM_MODAL_SPURS,1,1,\
	&FreqZeroMinimum,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define AR9287_SET_2GHZ_NOISE_FLOOR_THRESHOLD {Ar9287SetEeprom2GHzNoiseFloorThreshold,{Ar9287Eeprom2GHzNoiseFloorThreshold,"NoiseFloorThreshCh2g",0},"noise floor threshold",'d',0,AR9287_MAX_CHAINS,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_XPDGAIN {Ar9287SetEeprom2GHzXpdGain,{Ar9287Eeprom2GHzXpdGain,0,0},"Xpdgain",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}
	
#define AR9287_SET_2GHZ_XPD {Ar9287SetEeprom2GHzXpd,{Ar9287Eeprom2GHzXpd,0,0},"Xpd",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_IQCALICH {Ar9287SetEeprom2GHzIqCalICh,{Ar9287Eeprom2GHzIqCalICh,0,0},"IqCalICh",'d',0,AR9287_MAX_CHAINS,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_IQCALQCH {Ar9287SetEeprom2GHzIqCalQCh,{Ar9287Eeprom2GHzIqCalQCh,0,0},"IqCalQCh",'d',0,AR9287_MAX_CHAINS,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_BSWATTEN {Ar9287SetEeprom2GHzBswAtten,{Ar9287Eeprom2GHzBswAtten,0,0},"BswAtten",'d',0,AR9287_MAX_CHAINS,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_BSWMARGIN {Ar9287SetEeprom2GHzBswMargin,{Ar9287Eeprom2GHzBswMargin,0,0},"BswMargin",'d',0,AR9287_MAX_CHAINS,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_SWSETTLEHT40 {Ar9287SetEeprom2GHzSwSettleHt40,{Ar9287Eeprom2GHzSwSettleHt40,0,0},"SwSettleHt40",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_MODALHEADERVERSION {Ar9287SetEeprom2GHzModalHeaderversion,{Ar9287Eeprom2GHzModalHeaderVersion,0,0},"SwSettleHt40",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_DB1 {Ar9287SetEeprom2GHzDb1,{Ar9287Eeprom2GHzDb1,0,0},"db1",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_DB2 {Ar9287SetEeprom2GHzDb2,{Ar9287Eeprom2GHzDb2,0,0},"db2",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_OBCCK {Ar9287SetEeprom2GHzObCck,{Ar9287Eeprom2GHzObcck,0,0},"ob_cck",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_OBPSK {Ar9287SetEeprom2GHzObPsk,{Ar9287Eeprom2GHzObPsk,0,0},"ob_psk",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_OBQAM {Ar9287SetEeprom2GHzObQam,{Ar9287Eeprom2GHzObQam,0,0},"ob_qam",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_OBPALOFF {Ar9287SetEeprom2GHzObPalOff,{Ar9287Eeprom2GHzObPalOff,0,0},"ob_pal_off",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_FUTUREMODAL {Ar9287SetEeprom2GHzFutureModal,{Ar9287Eeprom2GHzFutureModal,0,0},"future modal",'d',0,30,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_SPURRANGELOW {Ar9287SetEeprom2GHzSpurRangeLow,{Ar9287Eeprom2GHzSpurRangeLow,0,0},"SpurRangeLow",'d',0,AR9287_EEPROM_MODAL_SPURS,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_SPURRANGEHIGH {Ar9287SetEeprom2GHzSpurRangeHigh,{Ar9287Eeprom2GHzSpurRangeHigh,0,0},"SpurRangeHigh",'d',0,AR9287_EEPROM_MODAL_SPURS,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_CTLDATA {Ar9287SetEeprom2GHzCtlData,{Ar9287Eeprom2GHzCtlData,0,0},"CTL data",'d',0,AR9287_NUM_CTLS,16,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_PADDING {Ar9287SetEeprom2GHzPadding,{Ar9287Eeprom2GHzPadding,0,0},"padding",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_QUICK_DROP {Ar9287SetEeprom2GHzQuickDrop,{Ar9287Eeprom2GHzQuickDrop,"quickDrop2g",0},"quick drop value",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_XPA_BIAS_LEVEL {Ar9287SetEeprom2GHzXpaBiasLevel,{Ar9287Eeprom2GHzXpaBiasLevel,"XpaBiasLvl2g",0},"external pa bias level",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_TX_FRAME_TO_DATA_START {Ar9287SetEeprom2GHzTxFrameToDataStart,{Ar9287Eeprom2GHzTxFrameToDataStart,"TxFrameToDataStart2g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_TX_FRAME_TO_PA_ON {Ar9287SetEeprom2GHzTxFrameToPaOn,{Ar9287Eeprom2GHzTxFrameToPaOn,"TxFrameToPaOn2g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_TX_CLIP {Ar9287SetEeprom2GHzTxClip,{Ar9287Eeprom2GHzTxClip,"TxClip2g",0},"",'x',0,1,1,1,\
	&HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define AR9287_SET_2GHZ_DAC_SCALE_CCK {Ar9287SetEeprom2GHzDacScaleCCK,{Ar9287Eeprom2GHzDacScaleCCK,"DacScaleCCK",0},"",'x',0,1,1,1,\
	&HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define AR9287_SET_2GHZ_ANTENNA_GAIN {Ar9287SetEeprom2GHzAntennaGain,{Ar9287Eeprom2GHzAntennaGain,"AntennaGain2g","AntGain2g"},"",'d',0,AR9287_MAX_CHAINS,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_SWITCH_SETTLING {Ar9287SetEeprom2GHzSwitchSettling,{Ar9287Eeprom2GHzSwitchSettling,"SwitchSettling2g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_ADC_DESIRED_SIZE {Ar9287SetEeprom2GHzAdcSize,{Ar9287Eeprom2GHzAdcSize,"AdcDesiredSize2g",0},"",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_TX_END_TO_XPA_OFF {Ar9287SetEeprom2GHzTxEndToXpaOff,{Ar9287Eeprom2GHzTxEndToXpaOff,"TxEndToXpaOff2g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_TX_END_TO_RX_ON {Ar9287SetEeprom2GHzTxEndToRxOn,{Ar9287Eeprom2GHzTxEndToRxOn,"TxEndToRxOn2g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_TX_FRAME_TO_XPA_ON {Ar9287SetEeprom2GHzTxFrameToXpaOn,{Ar9287Eeprom2GHzTxFrameToXpaOn,"TxFrameToXpaOn2g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_THRESH62 {Ar9287SetEeprom2GHzThresh62,{Ar9287Eeprom2GHzThresh62,"Thresh622g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_PAPD_HT20 {Ar9287SetEeprom2GHzPaPredistortionHt20,{Ar9287Eeprom2GHzPaPredistortionHt20,"papd2gRateMaskHt20",0},"pa predistortion mask for HT20 rates",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9287_SET_2GHZ_PAPD_HT40 {Ar9287SetEeprom2GHzPaPredistortionHt40,{Ar9287Eeprom2GHzPaPredistortionHt40,"papd2gRateMaskHt40",0},"pa predistortion mask for HT40 rates",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9287_SET_2GHZ_WLAN_SPDT_SWITCH_GLOBAL_CONTROL {Ar9287SetEeprom2GHzWlanSpdtSwitchGlobalControl,{Ar9287Eeprom2GHzWlanSpdtSwitchGlobalControl,"switchcomspdt2g",0},"spdt switch setting for wlan 2G in WLAN/BT global register",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9287_SET_2GHZ_XLNA_BIAS_STRENGTH {Ar9287SetEeprom2GHzXLNABiasStrength,{Ar9287Eeprom2GHzXLNABiasStrength,"XLNABiasStrength2g",0},"set XLNA bias strength",'x',0,AR9287_MAX_CHAINS,1,1,\
	&TwoBitsMinimum,&TwoBitsMaximum,&TwoBitsDefault,0,0}

#define AR9287_SET_2GHZ_RF_GAIN_CAP {Ar9287SetEeprom2GHzRFGainCAP,{Ar9287Eeprom2GHzRFGainCAP,"rfGainCAP2g",0},"set rf Gain CAP",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9287_SET_ANTA_DIV_CTL {Ar9287SetEepromAntennaDiversityControl,{Ar9287EepromAntennaDiversityControl,"antDivCtrl",0},"antenna diversity control",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_FUTURE {Ar9287SetEepromFuture,{Ar9287EepromFuture,0,0},"reserved words, should be set to 0",'x',0,MAX_BASE_EXTENSION_FUTURE,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_CALIBRATION_FREQUENCY {Ar9287SetEeprom2GHzCalibrationFrequency,{Ar9287Eeprom2GHzCalibrationFrequency,"calPierFreq2g",0},"frequencies at which calibration is performed",'u',"MHz",AR9287_NUM_2G_CAL_PIERS,1,1,\
	&FrequencyMinimum2GHz,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define AR9287_SET_2GHZ_CALIBRATION_DATA {Ar9287SetEeprom2GHzCalibrationData,{Ar9287Eeprom2GHzCalibrationData,0,0},"",'x',0,AR9287_MAX_CHAINS,AR9287_NUM_2G_CAL_PIERS,40,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_POWER_CORRECTION {Ar9287SetEeprom2GHzPowerCorrection,{Ar9287Eeprom2GHzPowerCorrection,"CalPierRefPower2g",0},"transmit power calibration correction values",'d',0,AR9287_MAX_CHAINS,AR9287_NUM_2G_CAL_PIERS,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_CALIBRATION_VOLTAGE {Ar9287SetEeprom2GHzCalibrationVoltage,{Ar9287Eeprom2GHzCalibrationVoltage,"CalPierVoltMeas2g",0},"voltage measured during transmit power calibration",'u',0,AR9287_MAX_CHAINS,AR9287_NUM_2G_CAL_PIERS,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_CALIBRATION_TEMPERATURE {Ar9287SetEeprom2GHzCalibrationTemperature,{Ar9287Eeprom2GHzCalibrationTemperature,"CalPierTempMeas2g",0},"temperature measured during transmit power calibration",'u',0,AR9287_MAX_CHAINS,AR9287_NUM_2G_CAL_PIERS,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_CALIBRATION_PCDAC {Ar9287SetEeprom2GHzCalibrationPcdac,{Ar9287Eeprom2GHzCalibrationPcdac,0,0},"pcdac measured during transmit power calibration",'u',0,AR9287_MAX_CHAINS,AR9287_NUM_2G_CAL_PIERS,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_NOISE_FLOOR {Ar9287SetEeprom2GHzNoiseFloor,{Ar9287Eeprom2GHzNoiseFloor,"CalPierRxNoisefloorCal2g",0},"noise floor measured during receive calibration",'d',0,AR9287_MAX_CHAINS,AR9287_NUM_2G_CAL_PIERS,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_NOISE_FLOOR_POWER {Ar9287SetEeprom2GHzNoiseFloorPower,{Ar9287Eeprom2GHzNoiseFloorPower,"CalPierRxNoisefloorPower2g",0},"power measured during receive calibration",'d',0,AR9287_MAX_CHAINS,AR9287_NUM_2G_CAL_PIERS,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9287_SET_2GHZ_NOISE_FLOOR_TEMPERATURE {Ar9287SetEeprom2GHzNoiseFloorTemperature,{Ar9287Eeprom2GHzNoiseFloorTemperature,"CalPierRxTempMeas2g",0},"temperature measured during receive calibration",'u',0,AR9287_MAX_CHAINS,AR9287_NUM_2G_CAL_PIERS,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_TARGET_DATA_CCK {Ar9287SetEeprom2GHzTargetDataCck,{Ar9287Eeprom2GHzTargetDataCck,0,0},"cal data for cck rates are specified",'u',"MHz",AR9287_NUM_2G_CCK_TARGET_POWERS,5,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_TARGET_FREQUENCY_CCK {Ar9287SetEeprom2GHzTargetFrequencyCck,{Ar9287Eeprom2GHzTargetFrequencyCck,"calTGTFreqcck",0},"frequencies at which target powers for cck rates are specified",'u',"MHz",AR9287_NUM_2G_CCK_TARGET_POWERS,1,1,\
	&FrequencyMinimum2GHz,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define AR9287_SET_2GHZ_TARGET_POWER_CCK {Ar9287SetEeprom2GHzTargetPowerCck,{Ar9287Eeprom2GHzTargetPowerCck,"calTGTpwrCCK",0},"target powers for cck rates",'f',"dBm",AR9287_NUM_2G_CCK_TARGET_POWERS,4,1,\
	&PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9287_SET_2GHZ_TARGET_DATA {Ar9287SetEeprom2GHzTargetData,{Ar9287Eeprom2GHzTargetData,0,0},"cal data for legacy rates are specified",'u',"MHz",AR9287_NUM_2G_20_TARGET_POWERS,5,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_TARGET_FREQUENCY {Ar9287SetEeprom2GHzTargetFrequency,{Ar9287Eeprom2GHzTargetFrequency,"calTGTFreq2g",0},"frequencies at which target powers for legacy rates are specified",'u',"MHz",AR9287_NUM_2G_20_TARGET_POWERS,1,1,\
	&FrequencyMinimum2GHz,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define AR9287_SET_2GHZ_TARGET_POWER {Ar9287SetEeprom2GHzTargetPower,{Ar9287Eeprom2GHzTargetPower,"calTGTpwr2g",0},"target powers for legacy rates",'f',"dBm",AR9287_NUM_2G_20_TARGET_POWERS,4,1,\
	&PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9287_SET_2GHZ_TARGET_DATA_HT20 {Ar9287SetEeprom2GHzTargetDataHt20,{Ar9287Eeprom2GHzTargetDataHt20,0,0},"cal data for ht20 rates are specified",'u',"MHz",AR9287_NUM_2G_20_TARGET_POWERS,9,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_TARGET_FREQUENCY_HT20 {Ar9287SetEeprom2GHzTargetFrequencyHt20,{Ar9287Eeprom2GHzTargetFrequencyHt20,"calTGTFreqht202g",0},"frequencies at which target powers for ht20 rates are specified",'u',"MHz",AR9287_NUM_2G_20_TARGET_POWERS,1,1,\
	&FrequencyMinimum2GHz,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define AR9287_SET_2GHZ_TARGET_POWER_HT20 {Ar9287SetEeprom2GHzTargetPowerHt20,{Ar9287Eeprom2GHzTargetPowerHt20,"calTGTpwrht202g",0},"target powers for ht20 rates",'f',"dBm",AR9287_NUM_2G_20_TARGET_POWERS,14,1,\
	&PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9287_SET_2GHZ_TARGET_DATA_HT40 {Ar9287SetEeprom2GHzTargetDataHt40,{Ar9287Eeprom2GHzTargetDataHt40,0,0},"cal data for ht40 rates are specified",'u',"MHz",AR9287_NUM_2G_40_TARGET_POWERS,9,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_TARGET_FREQUENCY_HT40 {Ar9287SetEeprom2GHzTargetFrequencyHt40,{Ar9287Eeprom2GHzTargetFrequencyHt40,"calTGTFreqht402g",0},"frequencies at which target powers for ht40 rates are specified",'u',"MHz",AR9287_NUM_2G_40_TARGET_POWERS,1,1,\
	&FrequencyMinimum2GHz,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define AR9287_SET_2GHZ_TARGET_POWER_HT40 {Ar9287SetEeprom2GHzTargetPowerHt40,{Ar9287Eeprom2GHzTargetPowerHt40,"calTGTpwrht402g",0},"target powers for ht40 rates",'f',"dBm",AR9287_NUM_2G_40_TARGET_POWERS,14,1,\
	&PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9287_SET_2GHZ_CTL_INDEX {Ar9287SetEeprom2GHzCtlIndex,{Ar9287Eeprom2GHzCtlIndex,"CtlIndex2g",0},"ctl indexes, see eeprom guide for explanation",'x',0,AR9287_NUM_CTLS_2G,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9287_SET_2GHZ_CTL_FREQUENCY {Ar9287SetEeprom2GHzCtlFrequency,{Ar9287Eeprom2GHzCtlFrequency,"CtlFreq2g",0},"frequencies at which maximum transmit powers are specified",'u',"MHz",AR9287_NUM_CTLS_2G,AR9287_NUM_BAND_EDGES_2G,1,\
	&FreqZeroMinimum,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define AR9287_SET_2GHZ_CTL_POWER {Ar9287SetEeprom2GHzCtlPower,{Ar9287Eeprom2GHzCtlPower,"CtlPower2g","CtlPwr2g"},"maximum allowed transmit powers",'f',"dBm",AR9287_NUM_CTLS_2G,AR9287_NUM_BAND_EDGES_2G,1,\
	&PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9287_SET_2GHZ_CTL_BANDEDGE {Ar9287SetEeprom2GHzCtlBandEdge,{Ar9287Eeprom2GHzCtlBandEdge,"CtlBandEdge2g","ctlflag2g"},"band edge flag",'x',0,AR9287_NUM_CTLS_2G,AR9287_NUM_BAND_EDGES_2G,1,\
	&TwoBitsMinimum,&TwoBitsMaximum,&TwoBitsDefault,0,0}

#define AR9287_SET_CALDATA_MEMORY_TYPE {Ar9287SetEeprom2GHzCaldataMemoryType,{Ar9287CaldataMemoryType,"memory","caldata"},"memory type for calibration data, eeprom or flash or otp",'t',0,1,1,1,\
	0,0,0,0,0}

#define AR9287_LENGTH_EEPROM {Ar9287SetEeprom2GHzLengthEeprom,{"length",0,0},"eep length",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9287_OPEN_LOOP_POWER_CONTROL {Ar9287SetEeprom2GHzOpenLoopPowerControl,{"openLoopPwrCntl",0,0},"openLoopPwrCntl",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define Ar9287_Tx_Rx_Atten {Ar9287SetEeprom2GHzTxRxAtten,{Ar9287Eeprom2GHztxRxAttenCh,"txRxAttenCh",0},"txRxAttenCh",'x',0,AR9287_NUM_CTLS_2G,AR9287_NUM_BAND_EDGES_2G,1,\
	&UnsignedIntMinimum,&SignedIntMaximum,&SignedIntDefault,0,0}

#define AR9287_RX_TX_MARGIN {Ar9287SetEeprom2GHzRxTxMargin,{Ar9287Eeprom2GHztxrxTxMarginCh,"rxTxMarginCh",0},"rxTxMarginCh",'x',0,AR9287_NUM_CTLS_2G,AR9287_NUM_BAND_EDGES_2G,1,\
	&UnsignedIntMinimum,&SignedIntMaximum,&SignedIntDefault,0,0}

#define AR9287_HT40_POWER_INC_FOR_PDADC {Ar9287SetEeprom2GHzHt40PowerIncForPdadc,{Ar9287Eeprom2GHzHt40PowerIncForPdadc,"ht40PowerIncForPdadc",0},"ht40PowerIncForPdadc",'x',0,1,1,1,\
	&HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define AR9287_CAL_FREQ_PIER_SET {Ar9287SetEeprom2GHzCalFreqPierSet,{"ArCalFreqPier",0,0},"ArCalFreqPier",'x',0,3,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9287_PDGAIN_OVERLAP {Ar9287SetEeprom2GHzPdGainOverLap,{Ar9287Eeprom2GHzPdGainOverlap,"ArPdGainOverLap",0},"pdGainOverLap",'x',0,1,1,1,\
	&HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

