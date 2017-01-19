

#define Ar9300EepromALL "ALL"
#define Ar9300EepromConfig "Config"
#define Ar9300EepromConfigPCIe "ConfigPCIe"
#define Ar9300EepromDeviceId "DeviceID"
#define Ar9300EepromSSID "SSID"
#define Ar9300EepromVID "VID"
#define Ar9300EepromSVID "SVID"



enum
{
    Ar9300SetEepromVersion=10000,
    Ar9300SetEepromTemplateVersion,
	Ar9300SetEepromALL,
	Ar9300SetEepromConfig,
	Ar9300SetEepromConfigPCIe,
	Ar9300SetEepromDeviceId,
	Ar9300SetEepromSSID,
	Ar9300SetEepromVID,
	Ar9300SetEepromSVID,
    
	Ar9300SetEepromMacAddress,
    Ar9300SetEepromCustomerData,
    Ar9300SetEepromRegulatoryDomain,
    Ar9300SetEepromTxRxMask,
	    Ar9300SetEepromTxRxMaskTx,
	    Ar9300SetEepromTxRxMaskRx,
    Ar9300SetEepromOpFlags,
    Ar9300SetEepromEepMisc,
    Ar9300SetEepromRfSilent,
		Ar9300SetEepromRfSilentB0,
		Ar9300SetEepromRfSilentB1,
		Ar9300SetEepromRfSilentGpio,
    Ar9300SetEepromBlueToothOptions,
    Ar9300SetEepromDeviceCapability,
    Ar9300SetEepromDeviceType,
    Ar9300SetEepromPowerTableOffset,
    Ar9300SetEepromTuningCaps,
    Ar9300SetEepromFeatureEnable,
	    Ar9300SetEepromFeatureEnableTemperatureCompensation,
	    Ar9300SetEepromFeatureEnableVoltageCompensation,
	    Ar9300SetEepromFeatureEnableFastClock,
	    Ar9300SetEepromFeatureEnableDoubling,
	    Ar9300SetEepromFeatureEnableInternalSwitchingRegulator,
	    Ar9300SetEepromFeatureEnablePaPredistortion,
	    Ar9300SetEepromFeatureEnableTuningCaps,
	    Ar9300SetEepromFeatureEnableTxFrameToXpaOn,
    Ar9300SetEepromMiscellaneous,
	    Ar9300SetEepromMiscellaneousDriveStrength,
	    Ar9300SetEepromMiscellaneousThermometer,
	    Ar9300SetEepromMiscellaneousChainMaskReduce,
		Ar9300SetEepromMiscellaneousQuickDropEnable,
    	Ar9300SetEepromMiscellaneousTempSlopExtensionEnable,
		Ar9300SetEepromMiscellaneousXLNABiasStrengthEnable,
		Ar9300SetEepromMiscellaneousRFGainCAPEnable,
	Ar9300SetEepromMiscEnable,
		Ar9300SetEepromMiscEnableTXGainCAPEnable,
    		Ar9300SetEepromMiscEnableMinCCAPwrthresholdEnable,
    Ar9300SetEepromEepromWriteEnableGpio,
    Ar9300SetEepromWlanDisableGpio,
    Ar9300SetEepromWlanLedGpio,
    Ar9300SetEepromRxBandSelectGpio,
    Ar9300SetEepromTxRxGain,
	    Ar9300SetEepromTxRxGainTx,
	    Ar9300SetEepromTxRxGainRx,
    Ar9300SetEepromSwitchingRegulator,
		Ar9300SetEepromSWREGProgram,
    Ar9300SetEeprom2GHzAntennaControlCommon,
    Ar9300SetEeprom2GHzAntennaControlCommon2,
    Ar9300SetEeprom2GHzAntennaControlChain,
    Ar9300SetEeprom2GHzAttenuationDb,
    Ar9300SetEeprom2GHzAttenuationMargin,
    Ar9300SetEeprom2GHzTemperatureSlope,
    Ar9300SetEeprom2GHzVoltageSlope,
    Ar9300SetEeprom2GHzSpur,
    Ar9300SetEeprom2GHzMinCCAPwrThreshold,
	Ar9300SetEeprom2GHzReserved,
	Ar9300SetEeprom2GHzQuickDrop,
    Ar9300SetEeprom2GHzXpaBiasLevel,     
    Ar9300SetEeprom2GHzTxFrameToDataStart,       
    Ar9300SetEeprom2GHzTxFrameToPaOn,  
    Ar9300SetEeprom2GHzTxClip,
	Ar9300SetEeprom2GHzDacScaleCCK,
    Ar9300SetEeprom2GHzAntennaGain,
    Ar9300SetEeprom2GHzSwitchSettling,
    Ar9300SetEeprom2GHzAdcSize,
    Ar9300SetEeprom2GHzTxEndToXpaOff,		
    Ar9300SetEeprom2GHzTxEndToRxOn,			
    Ar9300SetEeprom2GHzTxFrameToXpaOn,      
    Ar9300SetEeprom2GHzThresh62,
    Ar9300SetEeprom2GHzPaPredistortionHt20,
    Ar9300SetEeprom2GHzPaPredistortionHt40,
    Ar9300SetEeprom2GHzWlanSpdtSwitchGlobalControl,
    Ar9300SetEeprom2GHzXLNABiasStrength,
    Ar9300SetEeprom2GHzRFGainCAP,
    Ar9300SetEeprom2GHzTXGainCAP,
    Ar9300SetEeprom2GHzFuture,
    Ar9300SetEepromAntennaDiversityControl,
	Ar9300SetEeprom5GHzQuickDropLow,
	Ar9300SetEeprom5GHzQuickDropHigh,
    Ar9300SetEepromFuture,										
    Ar9300SetEeprom2GHzCalibrationFrequency,
//    Ar9300SetEeprom2GHzCalibrationData,
    Ar9300SetEeprom2GHzPowerCorrection,
    Ar9300SetEeprom2GHzCalibrationVoltage,
    Ar9300SetEeprom2GHzCalibrationTemperature,
    Ar9300SetEeprom2GHzNoiseFloor,
    Ar9300SetEeprom2GHzNoiseFloorPower,
    Ar9300SetEeprom2GHzNoiseFloorTemperature,
    Ar9300SetEeprom2GHzTargetFrequencyCck,
    Ar9300SetEeprom2GHzTargetFrequency,
    Ar9300SetEeprom2GHzTargetFrequencyHt20,
    Ar9300SetEeprom2GHzTargetFrequencyHt40,
    Ar9300SetEeprom2GHzTargetPowerCck,
    Ar9300SetEeprom2GHzTargetPower,
    Ar9300SetEeprom2GHzTargetPowerHt20,
    Ar9300SetEeprom2GHzTargetPowerHt40,
    Ar9300SetEeprom2GHzCtlIndex,
    Ar9300SetEeprom2GHzCtlFrequency,
    Ar9300SetEeprom2GHzCtlPower,
	Ar9300SetEeprom2GHzCtlBandEdge,
    Ar9300SetEeprom5GHzAntennaControlCommon,
    Ar9300SetEeprom5GHzAntennaControlCommon2,
    Ar9300SetEeprom5GHzAntennaControlChain,
    Ar9300SetEeprom5GHzAttenuationDb,
    Ar9300SetEeprom5GHzAttenuationMargin,
    Ar9300SetEeprom5GHzTemperatureSlope,
    Ar9300SetEeprom5GHzVoltageSlope,
    Ar9300SetEeprom5GHzSpur,
    Ar9300SetEeprom5GHzMinCCAPwrThreshold,
	Ar9300SetEeprom5GHzReserved,
	Ar9300SetEeprom5GHzQuickDrop,
    Ar9300SetEeprom5GHzXpaBiasLevel,    
    Ar9300SetEeprom5GHzTxFrameToDataStart,
    Ar9300SetEeprom5GHzTxFrameToPaOn,
    Ar9300SetEeprom5GHzTxClip,
    Ar9300SetEeprom5GHzAntennaGain,
    Ar9300SetEeprom5GHzSwitchSettling,
    Ar9300SetEeprom5GHzAdcSize,
    Ar9300SetEeprom5GHzTxEndToXpaOff,		
    Ar9300SetEeprom5GHzTxEndToRxOn,	
    Ar9300SetEeprom5GHzTxFrameToXpaOn,
    Ar9300SetEeprom5GHzThresh62,
    Ar9300SetEeprom5GHzPaPredistortionHt20,
    Ar9300SetEeprom5GHzPaPredistortionHt40,
    Ar9300SetEeprom5GHzWlanSpdtSwitchGlobalControl,
    Ar9300SetEeprom5GHzXLNABiasStrength,
    Ar9300SetEeprom5GHzRFGainCAP,
    Ar9300SetEeprom5GHzTXGainCAP,
    Ar9300SetEeprom5GHzFuture,				
    Ar9300SetEeprom5GHzTemperatureSlopeLow,
    Ar9300SetEeprom5GHzTemperatureSlopeHigh,
    Ar9300SetEeprom5GHzTemperatureSlopeExtension,
    Ar9300SetEeprom5GHzAttenuationDbLow,
    Ar9300SetEeprom5GHzAttenuationDbHigh,
    Ar9300SetEeprom5GHzAttenuationMarginLow,
    Ar9300SetEeprom5GHzAttenuationMarginHigh,
    Ar9300SetEeprom5GHzCalibrationFrequency,
    Ar9300SetEeprom5GHzCalibrationData,
	Ar9300SetEeprom5GHzPowerCorrection,
	Ar9300SetEeprom5GHzCalibrationVoltage,
	Ar9300SetEeprom5GHzCalibrationTemperature,
	Ar9300SetEeprom5GHzNoiseFloor,
	Ar9300SetEeprom5GHzNoiseFloorPower,
	Ar9300SetEeprom5GHzNoiseFloorTemperature,
    Ar9300SetEeprom5GHzTargetFrequency,
    Ar9300SetEeprom5GHzTargetFrequencyHt20,
    Ar9300SetEeprom5GHzTargetFrequencyHt40,
    Ar9300SetEeprom5GHzTargetPower,
    Ar9300SetEeprom5GHzTargetPowerHt20,
    Ar9300SetEeprom5GHzTargetPowerHt40,
    Ar9300SetEeprom5GHzCtlIndex,
    Ar9300SetEeprom5GHzCtlFrequency,
    Ar9300SetEeprom5GHzCtlPower,
	Ar9300SetEeprom5GHzCtlBandEdge,
	Ar9300SetCaldataMemoryType,
	Ar9300Set2GPastPower,
	Ar9300Set5GPastPower,
	Ar9300Set2GDiff_OFDM_CW_Power,
	Ar9300Set5GDiff_OFDM_CW_Power
};

static int ThermometerMinimum=-1;
static int ThermometerMaximum=2;
static int ThermometerDefault=1;

static int MinCCAPwrMinimum=0;
static int MinCCAPwrMaximum=3;
static int MinCCAPwrDefault=0;

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


static int Ar9300EepromVersionMinimum=2;		// check this
static int Ar9300EepromVersionDefault=2;

static int Ar9300TemplateVersionMinimum=2;		// get from eep.h
static int Ar9300TemplateVersionDefault=2;
//want template list

static struct _ParameterList LogicalParameter[]=
{
	{0,{"no",0,0},0,0,0,0,0,0,0,0,0,0,0},
	{1,{"yes",0,0},0,0,0,0,0,0,0,0,0,0,0},
};

static int ChainMaskMinimum=1;
static int ChainMaskMaximum=7;
static int ChainMaskDefault=7;



#define AR9300_SET_EEPROM_ALL {Ar9300SetEepromALL,{Ar9300EepromALL,0,0},"formatted display of all configuration and calibration data",'t',0,1,1,1,\
	0,0,0,0,0}

#define AR9300_SET_CONFIG {Ar9300SetEepromConfig,{Ar9300EepromConfig,0,0},"",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9300_SET_CONFIG_PCIE {Ar9300SetEepromConfigPCIe,{Ar9300EepromConfigPCIe,0,0},"",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9300_SET_EEPROM_DEVICEID {Ar9300SetEepromDeviceId,{Ar9300EepromDeviceId,"devid", 0},"the device id",'x',0,1,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define AR9300_SET_EEPROM_SSID {Ar9300SetEepromSSID,{Ar9300EepromSSID,"subSystemId",0},"the subsystem id",'x',0,1,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define AR9300_SET_EEPROM_VID {Ar9300SetEepromVID,{Ar9300EepromVID,"vendorId",0},"the vendor id",'x',0,1,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define AR9300_SET_EEPROM_SVID {Ar9300SetEepromSVID,{Ar9300EepromSVID,"subVendorId",0},"the subvendor id",'x',0,1,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}


#define AR9300_SET_EEPROM_VERSION {Ar9300SetEepromVersion,{Ar9300EepromVersion,"eepversion","version"},"the calibration structure version number",'u',0,1,1,1,\
	&Ar9300EepromVersionMinimum,&UnsignedByteMaximum,&Ar9300EepromVersionDefault,0,0}

#define AR9300_SET_TEMPLATE_VERSION {Ar9300SetEepromTemplateVersion,{Ar9300EepromTemplateVersion,0,0},"the template number",'u',0,1,1,1,\
	&Ar9300TemplateVersionMinimum,&UnsignedByteMaximum,&Ar9300TemplateVersionDefault,0,0}


#define AR9300_SET_MAC_ADDRESS {Ar9300SetEepromMacAddress,{Ar9300EepromMacAddress,"mac",0},"the mac address of the device",'m',0,1,1,1,\
	0,0,0,0,0}

#define AR9300_SET_CUSTOMER_DATA {Ar9300SetEepromCustomerData,{Ar9300EepromCustomerData,"customer",0},"any text, usually used for device serial number",'t',0,1,1,1,\
	0,0,0,0,0}

#define AR9300_SET_REGULATORY_DOMAIN {Ar9300SetEepromRegulatoryDomain,{Ar9300EepromRegulatoryDomain,"regDmn",0},"the regulatory domain",'x',0,2,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}			// want list of allowed values

#define AR9300_SET_TX_RX_MASK {Ar9300SetEepromTxRxMask,{Ar9300EepromTxRxMask,"txrxMask",0},"the transmit and receive chain masks",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_TX_RX_MASK_TX {Ar9300SetEepromTxRxMaskTx,{Ar9300EepromTxRxMaskTx,"TxMask",0},"the maximum chain mask used for transmit",'x',0,1,1,1,\
	&ChainMaskMinimum,&ChainMaskMaximum,&ChainMaskDefault,0,0}

#define AR9300_SET_TX_RX_MASK_RX {Ar9300SetEepromTxRxMaskRx,{Ar9300EepromTxRxMaskRx,"RxMask",0},"the maximum chain mask used for receive",'x',0,1,1,1,\
	&ChainMaskMinimum,&ChainMaskMaximum,&ChainMaskDefault,0,0}

#define AR9300_SET_OP_FLAGS {Ar9300SetEepromOpFlags,{Ar9300EepromOpFlags,"opFlags",0},"flags that control operating modes",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_EEP_MISC {Ar9300SetEepromEepMisc,{Ar9300EepromEepMisc,"eepMisc",0},"some miscellaneous control flags",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_RF_SILENT {Ar9300SetEepromRfSilent,{Ar9300EepromRfSilent,0,0},"rf silent mode control word",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_RF_SILENT_B0 {Ar9300SetEepromRfSilentB0,{Ar9300EepromRfSilentB0,"rfSilentB0",0},"implement rf silent mode in hardware",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_RF_SILENT_B1 {Ar9300SetEepromRfSilentB1,{Ar9300EepromRfSilentB1,"rfSilentB1",0},"polarity of the rf silent control line",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_RF_SILENT_GPIO {Ar9300SetEepromRfSilentGpio,{Ar9300EepromRfSilentGpio,"rfSilentGpio",0},"the chip gpio line used for rf silent",'x',0,1,1,1,\
	&SixBitsMinimum,&SixBitsMaximum,&SixBitsDefault,0,0}

#define AR9300_SET_BLUETOOTH_OPTIONS {Ar9300SetEepromBlueToothOptions,{Ar9300EepromBlueToothOptions,0,0},"bluetooth options",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_DEVICE_CAPABILITY {Ar9300SetEepromDeviceCapability,{Ar9300EepromDeviceCapability,"DeviceCap",0},"device capabilities",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_DEVICE_TYPE {Ar9300SetEepromDeviceType,{Ar9300EepromDeviceType,"DeviceType",0},"devicetype",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_POWER_TABLE_OFFSET {Ar9300SetEepromPowerTableOffset,{Ar9300EepromPowerTableOffset,"PwrTableOffset",0},"power level of the first entry in the power table",'d',"dBm",1,1,1,\
	&PowerOffsetMinimum,&PowerOffsetMaximum,&PowerOffsetDefault,0,0}

#define AR9300_SET_TUNING {Ar9300SetEepromTuningCaps,{Ar9300EepromTuningCaps,0,0},"capacitors for tuning frequency accuracy",'x',0,2,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}


#define AR9300_SET_FEATURE_ENABLE {Ar9300SetEepromFeatureEnable,{Ar9300EepromFeatureEnable,"featureEnable",0},"feature enable control word",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_FEATURE_ENABLE_TEMPERATURE_COMPENSATION {Ar9300SetEepromFeatureEnableTemperatureCompensation,{Ar9300EepromFeatureEnableTemperatureCompensation,"TemperatureCompensationEnable","TempCompEnable"},"enables temperature compensation on transmit power control",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_FEATURE_ENABLE_VOLTAGE_COMPENSATION {Ar9300SetEepromFeatureEnableVoltageCompensation,{Ar9300EepromFeatureEnableVoltageCompensation,"VoltageCompensationEnable","VoltCompEnable"},"enables voltage compensation on transmit power control",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_FEATURE_ENABLE_FAST_CLOCK {Ar9300SetEepromFeatureEnableFastClock,{Ar9300EepromFeatureEnableFastClock,"FastClockEnable",0},"enables fast clock mode",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_FEATURE_ENABLE_DOUBLING {Ar9300SetEepromFeatureEnableDoubling,{Ar9300EepromFeatureEnableDoubling,"DoublingEnable",0},"enables doubling mode",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_FEATURE_ENABLE_INTERNAL_SWITCHING_REGULATOR {Ar9300SetEepromFeatureEnableInternalSwitchingRegulator,{Ar9300EepromFeatureEnableInternalSwitchingRegulator,"swregenable",0},"enables the internal switching regulator",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_FEATURE_ENABLE_PA_PREDISTORTION {Ar9300SetEepromFeatureEnablePaPredistortion,{Ar9300EepromFeatureEnablePaPredistortion,"papdenable",0},"enables pa predistortion",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_FEATURE_ENABLE_TUNING_CAPS {Ar9300SetEepromFeatureEnableTuningCaps,{Ar9300EepromFeatureEnableTuningCaps,"TuningCapsEnable",0},"enables use of tuning capacitors",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_FEATURE_ENABLE_TX_FRAME_TO_XPA_ON {Ar9300SetEepromFeatureEnableTxFrameToXpaOn,{Ar9300EepromFeatureEnableTxFrameToXpaOn,"TxFrameToXpaOnEnable",0},"enables use of TxFrameToXpaOn",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_MISCELLANEOUS {Ar9300SetEepromMiscellaneous,{Ar9300EepromMiscellaneous,0,0},"miscellaneous parameters",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_MISCELLANEOUS_DRIVERS_STRENGTH {Ar9300SetEepromMiscellaneousDriveStrength,{Ar9300EepromMiscellaneousDriveStrength,"DriveStrengthReconfigure","DriveStrength"},"enables drive strength reconfiguration",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_MISCELLANEOUS_THERMOMETER {Ar9300SetEepromMiscellaneousThermometer,{Ar9300EepromMiscellaneousThermometer,"Thermometer",0},"forces use of the specified chip thermometer",'d',0,1,1,1,\
	&ThermometerMinimum,&ThermometerMaximum,&ThermometerDefault,0,0}

#define AR9300_SET_MISCELLANEOUS_CHAIN_MASK_REDUCE {Ar9300SetEepromMiscellaneousChainMaskReduce,{Ar9300EepromMiscellaneousChainMaskReduce,"ChainMaskReduce",0},"enables dynamic 2x3 mode to reduce power draw",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_MISCELLANEOUS_QUICK_DROP_ENABLE {Ar9300SetEepromMiscellaneousQuickDropEnable,{Ar9300EepromMiscellaneousQuickDropEnable,"quickDrop",0},"enables quick drop mode for improved strong signal response",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_MISCELLANEOUS_TEMP_SLOP_EXTENSION_ENABLE {Ar9300SetEepromMiscellaneousTempSlopExtensionEnable,{Ar9300EepromMiscellaneousTempSlopExtensionEnable,"tempslopextension",0},"enables temp slop extension",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_MISCELLANEOUS_XLNA_BIAS_STRENGTH_ENABLE {Ar9300SetEepromMiscellaneousXLNABiasStrengthEnable,{Ar9300EepromMiscellaneousXLNABiasStrengthEnable,"xLNABiasStrengthEnable",0},"enables XLNA bias strength set",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_MISCELLANEOUS_RF_GAIN_CAP_ENABLE {Ar9300SetEepromMiscellaneousRFGainCAPEnable,{Ar9300EepromMiscellaneousRFGainCAPEnable,"rfGainCAPEnable",0},"enables rf gain cap set",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_MISC_ENABLE {Ar9300SetEepromMiscEnable,{Ar9300EepromMiscEnable,0,0},"misc enable parameters",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_MISC_ENABLE_TX_GAIN_CAP_ENABLE {Ar9300SetEepromMiscEnableTXGainCAPEnable,{Ar9300EepromMiscEnableTXGainCAPEnable,"txGainCAPEnable",0},"enables tx gain cap set",'z',0,1,1,1,\
	0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define AR9300_SET_MISC_ENABLE_MINCCA_PWR_THRESHOLD {Ar9300SetEepromMiscEnableMinCCAPwrthresholdEnable,{Ar9300EepromMiscEnableMinCCAPwrthresholdEnable,"MinCCAPwrthresholdEnable",0},"enables MinCCA PWR threshold values to be set",'d',0,1,1,1,\
        &MinCCAPwrMinimum,&MinCCAPwrMaximum,&MinCCAPwrDefault,0,0}

#define AR9300_SET_EEPROM_WRITE_ENABLE_GPIO {Ar9300SetEepromEepromWriteEnableGpio,{Ar9300EepromEepromWriteEnableGpio,0,0},"gpio line used to enable the eeprom",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_WLAN_DISABLE_GPIO {Ar9300SetEepromWlanDisableGpio,{Ar9300EepromWlanDisableGpio,0,0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_WLAN_LED_GPIO {Ar9300SetEepromWlanLedGpio,{Ar9300EepromWlanLedGpio,0,0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_RX_BAND_SELECT_GPIO {Ar9300SetEepromRxBandSelectGpio,{Ar9300EepromRxBandSelectGpio,0,0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_TX_RX_GAIN {Ar9300SetEepromTxRxGain,{Ar9300EepromTxRxGain,0,0},"transmit and receive gain table control word",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_TX_RX_GAIN_TX {Ar9300SetEepromTxRxGainTx,{Ar9300EepromTxRxGainTx,"TxGain","TxGainTable"},"transmit gain table used",'x',0,1,1,1,\
	&HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define AR9300_SET_TX_RX_GAIN_RX {Ar9300SetEepromTxRxGainRx,{Ar9300EepromTxRxGainRx,"RxGain","RxGainTable"},"receive gain table used",'x',0,1,1,1,\
	&HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define AR9300_SET_SWITCHING_REGULATOR {Ar9300SetEepromSwitchingRegulator,{Ar9300EepromSwitchingRegulator,"SWREG","internalregulator"},"the internal switching regulator control word",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9300_SET_2GHZ_ANTENNA_CONTROL_COMMON {Ar9300SetEeprom2GHzAntennaControlCommon,{Ar9300Eeprom2GHzAntennaControlCommon,"AntCtrlCommon2g",0},"antenna switch control word 1",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9300_SET_2GHZ_ANTENNA_CONTROL_COMMON_2 {Ar9300SetEeprom2GHzAntennaControlCommon2,{Ar9300Eeprom2GHzAntennaControlCommon2,"AntCtrlCommon22g",0},"antenna switch control word 2",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9300_SET_2GHZ_ANTENNA_CONTROL_CHAIN {Ar9300SetEeprom2GHzAntennaControlChain,{Ar9300Eeprom2GHzAntennaControlChain,"antCtrlChain2g",0},"per chain antenna switch control word",'x',0,OSPREY_MAX_CHAINS,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define AR9300_SET_2GHZ_ATTENUATION_DB {Ar9300SetEeprom2GHzAttenuationDb,{Ar9300Eeprom2GHzAttenuationDb,"xatten1DB2g",0},"attenuation value",'x',0,OSPREY_MAX_CHAINS,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_ATTENUATION_MARGIN {Ar9300SetEeprom2GHzAttenuationMargin,{Ar9300Eeprom2GHzAttenuationMargin,"xatten1margin2g",0},"attenuation margin",'x',0,OSPREY_MAX_CHAINS,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_TEMPERATURE_SLOPE {Ar9300SetEeprom2GHzTemperatureSlope,{Ar9300Eeprom2GHzTemperatureSlope,"tempSlope2g","TemperatureSlope2g"},"slope used in temperature compensation algorithm",'d',0,3,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_2GHZ_VOLTAGE_SLOPE {Ar9300SetEeprom2GHzVoltageSlope,{Ar9300Eeprom2GHzVoltageSlope,"voltSlope2g","VoltageSlope2g"},"slope used in voltage compensation algorithm",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_2GHZ_SPUR {Ar9300SetEeprom2GHzSpur,{Ar9300Eeprom2GHzSpur,"SpurChans2g",0},"spur frequencies",'u',"MHz",OSPREY_EEPROM_MODAL_SPURS,1,1,\
	&FreqZeroMinimum,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define AR9300_SET_2GHZ_MINCCA_PWR_THRESHOLD {Ar9300SetEeprom2GHzMinCCAPwrThreshold,{Ar9300Eeprom2GHzMinCCAPwrThreshold,"MinCCAPwrThreshCh2g",0},"MinCCA Power threshold",'d',0,OSPREY_MAX_CHAINS,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_2GHZ_RESERVED {Ar9300SetEeprom2GHzReserved,{Ar9300Eeprom2GHzReserved,"Reserved2g",0},"reserved words, should be set to 0",'x',0,MAX_MODAL_RESERVED,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_QUICK_DROP {Ar9300SetEeprom2GHzQuickDrop,{Ar9300Eeprom2GHzQuickDrop,"quickDrop2g",0},"quick drop value",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_2GHZ_XPA_BIAS_LEVEL {Ar9300SetEeprom2GHzXpaBiasLevel,{Ar9300Eeprom2GHzXpaBiasLevel,"XpaBiasLvl2g",0},"external pa bias level",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_TX_FRAME_TO_DATA_START {Ar9300SetEeprom2GHzTxFrameToDataStart,{Ar9300Eeprom2GHzTxFrameToDataStart,"TxFrameToDataStart2g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_TX_FRAME_TO_PA_ON {Ar9300SetEeprom2GHzTxFrameToPaOn,{Ar9300Eeprom2GHzTxFrameToPaOn,"TxFrameToPaOn2g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_TX_CLIP {Ar9300SetEeprom2GHzTxClip,{Ar9300Eeprom2GHzTxClip,"TxClip2g",0},"",'x',0,1,1,1,\
	&HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define AR9300_SET_2GHZ_DAC_SCALE_CCK {Ar9300SetEeprom2GHzDacScaleCCK,{Ar9300Eeprom2GHzDacScaleCCK,"DacScaleCCK",0},"",'x',0,1,1,1,\
	&HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define AR9300_SET_2GHZ_ANTENNA_GAIN {Ar9300SetEeprom2GHzAntennaGain,{Ar9300Eeprom2GHzAntennaGain,"AntennaGain2g","AntGain2g"},"",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_2GHZ_SWITCH_SETTLING {Ar9300SetEeprom2GHzSwitchSettling,{Ar9300Eeprom2GHzSwitchSettling,"SwitchSettling2g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_ADC_DESIRED_SIZE {Ar9300SetEeprom2GHzAdcSize,{Ar9300Eeprom2GHzAdcSize,"AdcDesiredSize2g",0},"",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_2GHZ_TX_END_TO_XPA_OFF {Ar9300SetEeprom2GHzTxEndToXpaOff,{Ar9300Eeprom2GHzTxEndToXpaOff,"TxEndToXpaOff2g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_TX_END_TO_RX_ON {Ar9300SetEeprom2GHzTxEndToRxOn,{Ar9300Eeprom2GHzTxEndToRxOn,"TxEndToRxOn2g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_TX_FRAME_TO_XPA_ON {Ar9300SetEeprom2GHzTxFrameToXpaOn,{Ar9300Eeprom2GHzTxFrameToXpaOn,"TxFrameToXpaOn2g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_THRESH62 {Ar9300SetEeprom2GHzThresh62,{Ar9300Eeprom2GHzThresh62,"Thresh622g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_PAPD_HT20 {Ar9300SetEeprom2GHzPaPredistortionHt20,{Ar9300Eeprom2GHzPaPredistortionHt20,"papd2gRateMaskHt20",0},"pa predistortion mask for HT20 rates",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9300_SET_2GHZ_PAPD_HT40 {Ar9300SetEeprom2GHzPaPredistortionHt40,{Ar9300Eeprom2GHzPaPredistortionHt40,"papd2gRateMaskHt40",0},"pa predistortion mask for HT40 rates",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9300_SET_2GHZ_WLAN_SPDT_SWITCH_GLOBAL_CONTROL {Ar9300SetEeprom2GHzWlanSpdtSwitchGlobalControl,{Ar9300Eeprom2GHzWlanSpdtSwitchGlobalControl,"switchcomspdt2g",0},"spdt switch setting for wlan 2G in WLAN/BT global register",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9300_SET_2GHZ_XLNA_BIAS_STRENGTH {Ar9300SetEeprom2GHzXLNABiasStrength,{Ar9300Eeprom2GHzXLNABiasStrength,"XLNABiasStrength2g",0},"set XLNA bias strength",'x',0,OSPREY_MAX_CHAINS,1,1,\
	&TwoBitsMinimum,&TwoBitsMaximum,&TwoBitsDefault,0,0}

#define AR9300_SET_2GHZ_RF_GAIN_CAP {Ar9300SetEeprom2GHzRFGainCAP,{Ar9300Eeprom2GHzRFGainCAP,"rfGainCAP2g",0},"set rf Gain CAP",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9300_SET_2GHZ_TX_GAIN_CAP {Ar9300SetEeprom2GHzTXGainCAP,{Ar9300Eeprom2GHzTXGainCAP,"txGainCAP2g",0},"set tx Gain CAP",'x',0,1,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define AR9300_SET_2GHZ_FUTURE {Ar9300SetEeprom2GHzFuture,{Ar9300Eeprom2GHzFuture,"future2g",0},"reserved words, should be set to 0",'x',0,MAX_MODAL_FUTURE,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_ANTA_DIV_CTL {Ar9300SetEepromAntennaDiversityControl,{Ar9300EepromAntennaDiversityControl,"antDivCtrl",0},"antenna diversity control",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_FUTURE {Ar9300SetEepromFuture,{Ar9300EepromFuture,0,0},"reserved words, should be set to 0",'x',0,MAX_BASE_EXTENSION_FUTURE,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_CALIBRATION_FREQUENCY {Ar9300SetEeprom2GHzCalibrationFrequency,{Ar9300Eeprom2GHzCalibrationFrequency,"calPierFreq2g",0},"frequencies at which calibration is performed",'u',"MHz",OSPREY_NUM_2G_CAL_PIERS,1,1,\
	&FrequencyMinimum2GHz,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

//#define AR9300_SET_2GHZ_CALIBRATION_DATA {Ar9300SetEeprom2GHzCalibrationData,{Ar9300Eeprom2GHzCalibrationData,0,0},"",'x',0,OSPREY_MAX_CHAINS,OSPREY_NUM_2G_CAL_PIERS,6,\
//	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_POWER_CORRECTION {Ar9300SetEeprom2GHzPowerCorrection,{Ar9300Eeprom2GHzPowerCorrection,"CalPierRefPower2g",0},"transmit power calibration correction values",'d',0,OSPREY_MAX_CHAINS,OSPREY_NUM_2G_CAL_PIERS,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_2GHZ_CALIBRATION_VOLTAGE {Ar9300SetEeprom2GHzCalibrationVoltage,{Ar9300Eeprom2GHzCalibrationVoltage,"CalPierVoltMeas2g",0},"voltage measured during transmit power calibration",'u',0,OSPREY_MAX_CHAINS,OSPREY_NUM_2G_CAL_PIERS,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_CALIBRATION_TEMPERATURE {Ar9300SetEeprom2GHzCalibrationTemperature,{Ar9300Eeprom2GHzCalibrationTemperature,"CalPierTempMeas2g",0},"temperature measured during transmit power calibration",'u',0,OSPREY_MAX_CHAINS,OSPREY_NUM_2G_CAL_PIERS,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_NOISE_FLOOR {Ar9300SetEeprom2GHzNoiseFloor,{Ar9300Eeprom2GHzNoiseFloor,"CalPierRxNoisefloorCal2g",0},"noise floor measured during receive calibration",'d',0,OSPREY_MAX_CHAINS,OSPREY_NUM_2G_CAL_PIERS,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_2GHZ_NOISE_FLOOR_POWER {Ar9300SetEeprom2GHzNoiseFloorPower,{Ar9300Eeprom2GHzNoiseFloorPower,"CalPierRxNoisefloorPower2g",0},"power measured during receive calibration",'d',0,OSPREY_MAX_CHAINS,OSPREY_NUM_2G_CAL_PIERS,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_2GHZ_NOISE_FLOOR_TEMPERATURE {Ar9300SetEeprom2GHzNoiseFloorTemperature,{Ar9300Eeprom2GHzNoiseFloorTemperature,"CalPierRxTempMeas2g",0},"temperature measured during receive calibration",'u',0,OSPREY_MAX_CHAINS,OSPREY_NUM_2G_CAL_PIERS,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_TARGET_FREQUENCY_CCK {Ar9300SetEeprom2GHzTargetFrequencyCck,{Ar9300Eeprom2GHzTargetFrequencyCck,"calTGTFreqcck",0},"frequencies at which target powers for cck rates are specified",'u',"MHz",OSPREY_NUM_2G_CCK_TARGET_POWERS,1,1,\
	&FrequencyMinimum2GHz,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define AR9300_SET_2GHZ_TARGET_FREQUENCY {Ar9300SetEeprom2GHzTargetFrequency,{Ar9300Eeprom2GHzTargetFrequency,"calTGTFreq2g",0},"frequencies at which target powers for legacy rates are specified",'u',"MHz",OSPREY_NUM_2G_20_TARGET_POWERS,1,1,\
	&FrequencyMinimum2GHz,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define AR9300_SET_2GHZ_TARGET_FREQUENCY_HT20 {Ar9300SetEeprom2GHzTargetFrequencyHt20,{Ar9300Eeprom2GHzTargetFrequencyHt20,"calTGTFreqht202g",0},"frequencies at which target powers for ht20 rates are specified",'u',"MHz",OSPREY_NUM_2G_20_TARGET_POWERS,1,1,\
	&FrequencyMinimum2GHz,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define AR9300_SET_2GHZ_TARGET_FREQUENCY_HT40 {Ar9300SetEeprom2GHzTargetFrequencyHt40,{Ar9300Eeprom2GHzTargetFrequencyHt40,"calTGTFreqht402g",0},"frequencies at which target powers for ht40 rates are specified",'u',"MHz",OSPREY_NUM_2G_40_TARGET_POWERS,1,1,\
	&FrequencyMinimum2GHz,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define AR9300_SET_2GHZ_TARGET_POWER_CCK {Ar9300SetEeprom2GHzTargetPowerCck,{Ar9300Eeprom2GHzTargetPowerCck,"calTGTpwrCCK",0},"target powers for cck rates",'f',"dBm",OSPREY_NUM_2G_CCK_TARGET_POWERS,4,1,\
	&PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9300_SET_2GHZ_TARGET_POWER {Ar9300SetEeprom2GHzTargetPower,{Ar9300Eeprom2GHzTargetPower,"calTGTpwr2g",0},"target powers for legacy rates",'f',"dBm",OSPREY_NUM_2G_20_TARGET_POWERS,4,1,\
	&PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9300_SET_2GHZ_TARGET_POWER_HT20 {Ar9300SetEeprom2GHzTargetPowerHt20,{Ar9300Eeprom2GHzTargetPowerHt20,"calTGTpwrht202g",0},"target powers for ht20 rates",'f',"dBm",OSPREY_NUM_2G_20_TARGET_POWERS,14,1,\
	&PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9300_SET_2GHZ_TARGET_POWER_HT40 {Ar9300SetEeprom2GHzTargetPowerHt40,{Ar9300Eeprom2GHzTargetPowerHt40,"calTGTpwrht402g",0},"target powers for ht40 rates",'f',"dBm",OSPREY_NUM_2G_40_TARGET_POWERS,14,1,\
	&PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9300_SET_2GHZ_CTL_INDEX {Ar9300SetEeprom2GHzCtlIndex,{Ar9300Eeprom2GHzCtlIndex,"CtlIndex2g",0},"ctl indexes, see eeprom guide for explanation",'x',0,OSPREY_NUM_CTLS_2G,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_2GHZ_CTL_FREQUENCY {Ar9300SetEeprom2GHzCtlFrequency,{Ar9300Eeprom2GHzCtlFrequency,"CtlFreq2g",0},"frequencies at which maximum transmit powers are specified",'u',"MHz",OSPREY_NUM_CTLS_2G,OSPREY_NUM_BAND_EDGES_2G,1,\
	&FreqZeroMinimum,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define AR9300_SET_2GHZ_CTL_POWER {Ar9300SetEeprom2GHzCtlPower,{Ar9300Eeprom2GHzCtlPower,"CtlPower2g","CtlPwr2g"},"maximum allowed transmit powers",'f',"dBm",OSPREY_NUM_CTLS_2G,OSPREY_NUM_BAND_EDGES_2G,1,\
	&PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9300_SET_2GHZ_CTL_BANDEDGE {Ar9300SetEeprom2GHzCtlBandEdge,{Ar9300Eeprom2GHzCtlBandEdge,"CtlBandEdge2g","ctlflag2g"},"band edge flag",'x',0,OSPREY_NUM_CTLS_2G,OSPREY_NUM_BAND_EDGES_2G,1,\
	&TwoBitsMinimum,&TwoBitsMaximum,&TwoBitsDefault,0,0}

/****************************/
#define AR9300_SET_5GHZ_ANTENNA_CONTROL_COMMON {Ar9300SetEeprom5GHzAntennaControlCommon,{Ar9300Eeprom5GHzAntennaControlCommon,"AntCtrlCommon5g",0},"antenna switch control word 1",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9300_SET_5GHZ_ANTENNA_CONTROL_COMMON_2 {Ar9300SetEeprom5GHzAntennaControlCommon2,{Ar9300Eeprom5GHzAntennaControlCommon2,"AntCtrlCommon25g",0},"antenna switch control word 2",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9300_SET_5GHZ_ANTENNA_CONTROL_CHAIN {Ar9300SetEeprom5GHzAntennaControlChain,{Ar9300Eeprom5GHzAntennaControlChain,"antCtrlChain5g",0},"per chain antenna switch control word",'x',0,OSPREY_MAX_CHAINS,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define AR9300_SET_5GHZ_ATTENUATION_DB {Ar9300SetEeprom5GHzAttenuationDb,{Ar9300Eeprom5GHzAttenuationDb,"xatten1DB5g",0},"attenuation value at 5500 MHz",'x',0,OSPREY_MAX_CHAINS,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_ATTENUATION_DB_LOW {Ar9300SetEeprom5GHzAttenuationDbLow,{Ar9300Eeprom5GHzAttenuationDbLow,"xatten1DBLow5g","xatten1DBLow"},"attenuation value at 5180 MHz",'x',0,OSPREY_MAX_CHAINS,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_ATTENUATION_DB_HIGH {Ar9300SetEeprom5GHzAttenuationDbHigh,{Ar9300Eeprom5GHzAttenuationDbHigh,"xatten1DBHigh5g","xatten1DBHigh"},"attenuation value at 5785 MHz",'x',0,OSPREY_MAX_CHAINS,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_ATTENUATION_MARGIN {Ar9300SetEeprom5GHzAttenuationMargin,{Ar9300Eeprom5GHzAttenuationMargin,"xatten1Margin5g",0},"attenuation margin at 5500 MHz",'x',0,OSPREY_MAX_CHAINS,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_ATTENUATION_MARGIN_LOW {Ar9300SetEeprom5GHzAttenuationMarginLow,{Ar9300Eeprom5GHzAttenuationMarginLow,"xatten1MarginLow","xatten1MarginLow5g"},"attenuation margin at 5180 MHz",'x',0,OSPREY_MAX_CHAINS,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_ATTENUATION_MARGIN_HIGH {Ar9300SetEeprom5GHzAttenuationMarginHigh,{Ar9300Eeprom5GHzAttenuationMarginHigh,"xatten1MarginHigh","xatten1MarginHigh5g"},"attenuation margin at 5785 MHz",'x',0,OSPREY_MAX_CHAINS,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}


#define AR9300_SET_5GHZ_TEMPERATURE_SLOPE {Ar9300SetEeprom5GHzTemperatureSlope,{Ar9300Eeprom5GHzTemperatureSlope,"tempSlope5g","TemperatureSlope5g"},"slope used at 5500 MHz in temperature compensation algorithm",'d',0,3,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_5GHZ_TEMPERATURE_SLOPE_LOW {Ar9300SetEeprom5GHzTemperatureSlopeLow,{Ar9300Eeprom5GHzTemperatureSlopeLow,"tempSlopeLow5g","TemperatureSlopeLow5g"},"slope used at 5180 MHz in temperature compensation algorithm",'d',0,3,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_5GHZ_TEMPERATURE_SLOPE_HIGH {Ar9300SetEeprom5GHzTemperatureSlopeHigh,{Ar9300Eeprom5GHzTemperatureSlopeHigh,"tempSlopeHigh5g","TemperatureSlopeHigh5g"},"slope used at 5785 MHz in temperature compensation algorithm",'d',0,3,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_5GHZ_VOLTAGE_SLOPE {Ar9300SetEeprom5GHzVoltageSlope,{Ar9300Eeprom5GHzVoltageSlope,"voltSlope5g","VoltageSlope5g"},"slope used in voltage compensation algorithm",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_5GHZ_TEMPERATURE_SLOPE_EXTENSION {Ar9300SetEeprom5GHzTemperatureSlopeExtension,{Ar9300Eeprom5GHzTemperatureSlopeExtension,"tempSlopeExtension","TemperatureSlopeExtension5g"},"slope used at cal frequncy in temperature compensation algorithm",'d',0,8,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_5GHZ_SPUR {Ar9300SetEeprom5GHzSpur,{Ar9300Eeprom5GHzSpur,"SpurChans5g",0},"spur frequencies",'u',"MHz",OSPREY_EEPROM_MODAL_SPURS,1,1,\
	&FreqZeroMinimum,&FrequencyMaximum5GHz,&FrequencyDefault5GHz,0,0}

#define AR9300_SET_5GHZ_MINCCA_PWR_THRESHOLD {Ar9300SetEeprom5GHzMinCCAPwrThreshold,{Ar9300Eeprom5GHzMinCCAPwrThreshold,"MinCCAPwrThreshCh5g",0},"MinCCA Power threshold",'d',0,OSPREY_MAX_CHAINS,1,1,\
        &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}
#define AR9300_SET_5GHZ_RESERVED {Ar9300SetEeprom5GHzReserved,{Ar9300Eeprom5GHzReserved,"Reserved5g",0},"reserved words, should be set to 0",'x',0,MAX_MODAL_RESERVED,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_QUICK_DROP {Ar9300SetEeprom5GHzQuickDrop,{Ar9300Eeprom5GHzQuickDrop,"quickDrop5g",0},"quick drop value used at 5500 MHz",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_5GHZ_QUICK_DROP_LOW {Ar9300SetEeprom5GHzQuickDropLow,{Ar9300Eeprom5GHzQuickDropLow,"quickDropLow5g",0},"quick drop value used at 5180 MHz",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_5GHZ_QUICK_DROP_HIGH {Ar9300SetEeprom5GHzQuickDropHigh,{Ar9300Eeprom5GHzQuickDropHigh,"quickDropHigh5g",0},"quick drop value used at 5785 MHz",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_5GHZ_XPA_BIAS_LEVEL {Ar9300SetEeprom5GHzXpaBiasLevel,{Ar9300Eeprom5GHzXpaBiasLevel,"XpaBiasLvl5g",0},"external pa bias level",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_TX_FRAME_TO_DATA_START {Ar9300SetEeprom5GHzTxFrameToDataStart,{Ar9300Eeprom5GHzTxFrameToDataStart,"TxFrameToDataStart5g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_TX_FRAME_TO_PA_ON {Ar9300SetEeprom5GHzTxFrameToPaOn,{Ar9300Eeprom5GHzTxFrameToPaOn,"TxFrameToPaOn5g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_TX_CLIP {Ar9300SetEeprom5GHzTxClip,{Ar9300Eeprom5GHzTxClip,"TxClip5g",0},"",'x',0,1,1,1,\
	&HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define AR9300_SET_5GHZ_ANTENNA_GAIN {Ar9300SetEeprom5GHzAntennaGain,{Ar9300Eeprom5GHzAntennaGain,"AntennaGain5g","AntGain5g"},"",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_5GHZ_SWITCH_SETTLING {Ar9300SetEeprom5GHzSwitchSettling,{Ar9300Eeprom5GHzSwitchSettling,"SwitchSettling5g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_ADC_DESIRED_SIZE {Ar9300SetEeprom5GHzAdcSize,{Ar9300Eeprom5GHzAdcSize,"AdcDesiredSize5g",0},"",'d',0,1,1,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_5GHZ_TX_END_TO_XPA_OFF {Ar9300SetEeprom5GHzTxEndToXpaOff,{Ar9300Eeprom5GHzTxEndToXpaOff,"TxEndToXpaOff5g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_TX_END_TO_RX_ON {Ar9300SetEeprom5GHzTxEndToRxOn,{Ar9300Eeprom5GHzTxEndToRxOn,"TxEndToRxOn5g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_TX_FRAME_TO_XPA_ON {Ar9300SetEeprom5GHzTxFrameToXpaOn,{Ar9300Eeprom5GHzTxFrameToXpaOn,"TxFrameToXpaOn5g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_THRESH62 {Ar9300SetEeprom5GHzThresh62,{Ar9300Eeprom5GHzThresh62,"Thresh625g",0},"",'x',0,1,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_PAPD_HT20 {Ar9300SetEeprom5GHzPaPredistortionHt20,{Ar9300Eeprom5GHzPaPredistortionHt20,"papd5gRateMaskHt20",0},"pa predistortion mask for HT20 rates",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9300_SET_5GHZ_PAPD_HT40 {Ar9300SetEeprom5GHzPaPredistortionHt40,{Ar9300Eeprom5GHzPaPredistortionHt40,"papd5gRateMaskHt40",0},"pa predistortion mask for HT40 rates",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9300_SET_5GHZ_WLAN_SPDT_SWITCH_GLOBAL_CONTROL {Ar9300SetEeprom5GHzWlanSpdtSwitchGlobalControl,{Ar9300Eeprom5GHzWlanSpdtSwitchGlobalControl,"switchcomspdt5g",0},"spdt switch setting for wlan 5G in WLAN/BT global register",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9300_SET_5GHZ_XLNA_BIAS_STRENGTH {Ar9300SetEeprom5GHzXLNABiasStrength,{Ar9300Eeprom5GHzXLNABiasStrength,"XLNABiasStrength5g",0},"set XLNA bias strength",'x',0,1,1,1,\
	&TwoBitsMinimum,&TwoBitsMaximum,&TwoBitsDefault,0,0}

#define AR9300_SET_5GHZ_RF_GAIN_CAP {Ar9300SetEeprom5GHzRFGainCAP,{Ar9300Eeprom5GHzRFGainCAP,"rfGainCAP5g",0},"set XLNA bias strength",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define AR9300_SET_5GHZ_TX_GAIN_CAP {Ar9300SetEeprom5GHzTXGainCAP,{Ar9300Eeprom5GHzTXGainCAP,"txGainCAP5g",0},"set tx Gain CAP",'x',0,1,1,1,\
	&UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define AR9300_SET_5GHZ_FUTURE {Ar9300SetEeprom5GHzFuture,{Ar9300Eeprom5GHzFuture,"future5g",0},"reserved words, should be set to 0",'x',0,MAX_MODAL_FUTURE,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_CALIBRATION_FREQUENCY {Ar9300SetEeprom5GHzCalibrationFrequency,{Ar9300Eeprom5GHzCalibrationFrequency,"calPierFreq5g",0},"frequencies at which calibration is performed",'u',"MHz",OSPREY_NUM_5G_CAL_PIERS,1,1,\
	&FrequencyMinimum5GHz,&FrequencyMaximum5GHz,&FrequencyDefault5GHz,0,0}

//#define AR9300_SET_5GHZ_CALIBRATION_DATA {Ar9300SetEeprom5GHzCalibrationData,{Ar9300Eeprom5GHzCalibrationData,0,0},"",'x',0,OSPREY_MAX_CHAINS,OSPREY_NUM_5G_CAL_PIERS,6,\
//	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_POWER_CORRECTION {Ar9300SetEeprom5GHzPowerCorrection,{Ar9300Eeprom5GHzPowerCorrection,"CalPierRefPower5g",0},"transmit power calibration correction values",'d',0,OSPREY_MAX_CHAINS,OSPREY_NUM_5G_CAL_PIERS,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_5GHZ_CALIBRATION_VOLTAGE {Ar9300SetEeprom5GHzCalibrationVoltage,{Ar9300Eeprom5GHzCalibrationVoltage,"CalPierVoltMeas5g",0},"voltage measured during transmit power calibration",'u',0,OSPREY_MAX_CHAINS,OSPREY_NUM_5G_CAL_PIERS,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_CALIBRATION_TEMPERATURE {Ar9300SetEeprom5GHzCalibrationTemperature,{Ar9300Eeprom5GHzCalibrationTemperature,"CalPierTempMeas5g",0},"temperature measured during transmit power calibration",'u',0,OSPREY_MAX_CHAINS,OSPREY_NUM_5G_CAL_PIERS,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_NOISE_FLOOR {Ar9300SetEeprom5GHzNoiseFloor,{Ar9300Eeprom5GHzNoiseFloor,"CalPierRxNoisefloorCal5g",0},"noise floor measured during receive calibration",'d',0,OSPREY_MAX_CHAINS,OSPREY_NUM_5G_CAL_PIERS,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_5GHZ_NOISE_FLOOR_POWER {Ar9300SetEeprom5GHzNoiseFloorPower,{Ar9300Eeprom5GHzNoiseFloorPower,"CalPierRxNoisefloorPower5g",0},"power measured during receive calibration",'d',0,OSPREY_MAX_CHAINS,OSPREY_NUM_5G_CAL_PIERS,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define AR9300_SET_5GHZ_NOISE_FLOOR_TEMPERATURE {Ar9300SetEeprom5GHzNoiseFloorTemperature,{Ar9300Eeprom5GHzNoiseFloorTemperature,"CalPierRxTempMeas5g",0},"temperature measured during receive calibration",'u',0,OSPREY_MAX_CHAINS,OSPREY_NUM_5G_CAL_PIERS,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_TARGET_FREQUENCY {Ar9300SetEeprom5GHzTargetFrequency,{Ar9300Eeprom5GHzTargetFrequency,"calTGTFreq5g",0},"frequencies at which target powers for legacy rates are specified",'u',"MHz",OSPREY_NUM_5G_20_TARGET_POWERS,1,1,\
	&FrequencyMinimum5GHz,&FrequencyMaximum5GHz,&FrequencyDefault5GHz,0,0}

#define AR9300_SET_5GHZ_TARGET_FREQUENCY_HT20 {Ar9300SetEeprom5GHzTargetFrequencyHt20,{Ar9300Eeprom5GHzTargetFrequencyHt20,"calTGTFreqht205g",0},"frequencies at which target powers for ht20 rates are specified",'u',"MHz",OSPREY_NUM_5G_20_TARGET_POWERS,1,1,\
	&FrequencyMinimum5GHz,&FrequencyMaximum5GHz,&FrequencyDefault5GHz,0,0}

#define AR9300_SET_5GHZ_TARGET_FREQUENCY_HT40 {Ar9300SetEeprom5GHzTargetFrequencyHt40,{Ar9300Eeprom5GHzTargetFrequencyHt40,"calTGTFreqht405g",0},"frequencies at which target powers for ht40 rates are specified",'u',"MHz",OSPREY_NUM_5G_40_TARGET_POWERS,1,1,\
	&FrequencyMinimum5GHz,&FrequencyMaximum5GHz,&FrequencyDefault5GHz,0,0}

#define AR9300_SET_5GHZ_TARGET_POWER {Ar9300SetEeprom5GHzTargetPower,{Ar9300Eeprom5GHzTargetPower,"calTGTpwr5g",0},"target powers for legacy rates",'f',"dBm",OSPREY_NUM_5G_20_TARGET_POWERS,4,1,\
	&PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9300_SET_5GHZ_TARGET_POWER_HT20 {Ar9300SetEeprom5GHzTargetPowerHt20,{Ar9300Eeprom5GHzTargetPowerHt20,"calTGTpwrht205g",0},"target powers for ht20 rates",'f',"dBm",OSPREY_NUM_5G_20_TARGET_POWERS,14,1,\
	&PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9300_SET_5GHZ_TARGET_POWER_HT40 {Ar9300SetEeprom5GHzTargetPowerHt40,{Ar9300Eeprom5GHzTargetPowerHt40,"calTGTpwrht405g",0},"target powers for ht40 rates",'f',"dBm",OSPREY_NUM_5G_40_TARGET_POWERS,14,1,\
	&PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9300_SET_5GHZ_CTL_INDEX {Ar9300SetEeprom5GHzCtlIndex,{Ar9300Eeprom5GHzCtlIndex,"CtlIndex5g",0},"ctl indexes, see eeprom guide for explanation",'x',0,OSPREY_NUM_CTLS_5G,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define AR9300_SET_5GHZ_CTL_FREQUENCY {Ar9300SetEeprom5GHzCtlFrequency,{Ar9300Eeprom5GHzCtlFrequency,"CtlFreq5g",0},"frequencies at which maximum transmit powers are specified",'u',"MHz",OSPREY_NUM_CTLS_5G,OSPREY_NUM_BAND_EDGES_5G,1,\
	&FreqZeroMinimum,&FrequencyMaximum5GHz,&FrequencyDefault5GHz,0,0}

#define AR9300_SET_5GHZ_CTL_POWER {Ar9300SetEeprom5GHzCtlPower,{Ar9300Eeprom5GHzCtlPower,"CtlPower5g","CtlPwr5g"},"maximum allowed transmit powers",'f',"dBm",OSPREY_NUM_CTLS_5G,OSPREY_NUM_BAND_EDGES_5G,1,\
	&PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9300_SET_5GHZ_CTL_BANDEDGE {Ar9300SetEeprom5GHzCtlBandEdge,{Ar9300Eeprom5GHzCtlBandEdge,"CtlBandEdge5g","ctlflag5g"},"band edge flag",'x',0,OSPREY_NUM_CTLS_5G,OSPREY_NUM_BAND_EDGES_5G,1,\
	&TwoBitsMinimum,&TwoBitsMaximum,&TwoBitsDefault,0,0}


#define AR9300_SET_CALDATA_MEMORY_TYPE {Ar9300SetCaldataMemoryType,{Ar9300CaldataMemoryType,"memory","caldata"},"memory type for calibration data, eeprom or flash or otp",'t',0,1,1,1,\
	0,0,0,0,0}

#define AR9300_SET_2G_PSAT_POWER {Ar9300Set2GPastPower,{Ar93002GPastPower,"2gpsatpower",0},"2G Past power",'f',"dBm",2,3,1,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9300_SET_5G_PSAT_POWER {Ar9300Set5GPastPower,{Ar93005GPastPower,"5gpsatpower",0},"5G Past power",'f',"dBm",2,8,1,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define AR9300_SET_2G_DIFF_OFDM_CW_POWER {Ar9300Set2GDiff_OFDM_CW_Power,{Ar93002GDiff_OFDM_CW_Power,"2gpsatdiff",0},"2G Psat diff OFDM and CWTone",'f',"dB",2,3,1,\
    0,0,0,0,0}

#define AR9300_SET_5G_DIFF_OFDM_CW_POWER {Ar9300Set5GDiff_OFDM_CW_Power,{Ar93005GDiff_OFDM_CW_Power,"5gpsatdiff",0},"5G Psat diff OFDM and CWTone",'f',"dB",2,8,1,\
    0,0,0,0,0}


