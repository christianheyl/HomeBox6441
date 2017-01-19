#ifndef _QC9K_EEPROM_PARAMETER_H_
#define _QC9K_EEPROM_PARAMETER_H_

// Base Header
#define Qc9KEepromALL "ALL"
#define Qc9KEepromAllFirmware "ALLFw"
#define Qc9KEepromLength "Length"
#define Qc9KEepromChecksum "Checksum"
#define Qc9KEepromVersion "Version"										// decimal
#define Qc9KEepromTemplateVersion "Template"								// decimal
#define Qc9KEepromConfig "Config"
#define Qc9KEepromConfigPCIe "ConfigPCIe"
#define Qc9KEepromDeviceId "DeviceId"
#define Qc9KEepromSubSystemId "SSID"
#define Qc9KEepromVID "VID"
#define Qc9KEepromSVID "SVID"
#define Qc9KEepromMacAddress "Mac"										// 6
#define Qc9KEepromRegulatoryDomain "RegulatoryDomain"						// 2 x 16
#define Qc9KEepromOpFlags "OpFlags"
#define Qc9KEepromOpFlags2 "OpFlags2"
#define Qc9KEepromBoardFlags "BoardFlags"
    #define Qc9KEepromEnableRbias "Rbias"
	#define Qc9KEeprom2GHzEnablePaPredistortion "2GHz.PaPredistortion"
	#define Qc9KEeprom5GHzEnablePaPredistortion "5GHz.PaPredistortion"
	#define Qc9KEepromTxGainTblEepEna "TxgainTblEepEna"
	#define Qc9KEepromTxGainTblEepScheme "TxgainTblEepScheme"
	#define Qc9KEeprom2GHzEnableXpaBias "2GHz.EnableXpaBias"
	#define Qc9KEeprom5GHzEnableXpaBias "5GHz.EnableXpaBias"
#define Qc9KEepromBlueToothOptions "BlueToothOptions"
#define Qc9KEepromFeatureEnable "FeatureEnable"
	#define Qc9KEepromFeatureEnableTemperatureCompensation "FeatureEnable.TemperatureCompensation"
	#define Qc9KEepromFeatureEnableVoltageCompensation "FeatureEnable.VoltageCompensation"
	#define Qc9KEepromFeatureEnableFastClock "FeatureEnable.FastClock"
	#define Qc9KEepromFeatureEnableDoubling "FeatureEnable.Doubling"
	#define Qc9KEepromFeatureEnableInternalSwitchingRegulator "FeatureEnable.SwitchingRegulator"
	#define Qc9KEepromFeatureEnableTuningCaps "FeatureEnable.TuningCaps"
#define Qc9KEepromMiscellaneous "Miscellaneous"
	#define Qc9KEepromMiscellaneousDriveStrength "Miscellaneous.DriveStrength"
	#define Qc9KEepromMiscellaneousThermometer "Miscellaneous.Thermometer"
	#define Qc9KEepromMiscellaneousChainMaskReduce "Miscellaneous.Dynamic2x3"
	#define Qc9KEepromMiscellaneousQuickDropEnable "Miscellaneous.QuickDropEnable"
	#define Qc9KEepromMiscellaneousEep "Miscellaneous.Eep"
#define Qc9KEepromFlag1 "BoardFlag1"
    #define Qc9KEepromBibxosc0 "BiasCurrentXosc0"
#define Qc9KEepromFlag1NoiseFlrThr "NoiseFlrThrFlag1"
#define Qc9KEepromBinBuildNumber "BinBuildNumber"
#define Qc9KEepromTxRxMask "Mask"
	#define Qc9KEepromTxRxMaskTx "Mask.Tx"
	#define Qc9KEepromTxRxMaskRx "Mask.Rx"
#define Qc9KEepromRfSilent "RfSilent"
	#define Qc9KEepromRfSilentB0 "RfSilent.HardwareEnable"
	#define Qc9KEepromRfSilentB1 "RfSilent.Polarity"
	#define Qc9KEepromRfSilentGpio "RfSilent.Gpio"
#define Qc9KEepromWlanLedGpio "WlanLedGpio"								// decimal
#define Qc9KEepromDeviceType "DeviceType"
#define Qc9KEepromCtlVersion "CtlVersion"
#define Qc9KEepromSpurBaseA "SpurBaseA"
#define Qc9KEepromSpurBaseB "SpurBaseB"
#define Qc9KEepromSpurRssiThresh "SpurRssiThresh"
#define Qc9KEepromSpurRssiThreshCck "SpurRssiThreshCck"
#define Qc9KEepromSpurMitFlag "SpurMitFlag"
#define Qc9KEepromSwreg "Swreg"
#define Qc9KEepromSwregProgram "SwregProgram"
// define subfields of switching regulator?
#define Qc9KEepromTxRxGain "GainTable"
	#define Qc9KEepromTxRxGainTx "GainTable.Tx"
	#define Qc9KEepromTxRxGainRx "GainTable.Rx"
#define Qc9KEepromPowerTableOffset "PowerTableOffset"
#define Qc9KEepromTuningCaps "TuningCaps"
#define Qc9KEepromDeltaCck20 "DeltaCck20"
#define Qc9KEepromDelta4020 "Delta4020"
#define Qc9KEepromDelta8020 "Delta8020"
#define Qc9KEepromCustomerData "Customer"
#define Qc9KEepromBaseFuture "BaseFuture"

// 2GHz BiModal Header
#define Qc9KEeprom2GHzVoltageSlope "2GHz.VoltageSlope"					// signed
#define Qc9KEeprom2GHzSpur "2GHz.Spur"									// OSPREY_MAX_CHAINS
	#define Qc9KEeprom2GHzSpurAPrimSecChoose "2GHz.SpurAPrimSecChoose"
	#define Qc9KEeprom2GHzSpurBPrimSecChoose "2GHz.SpurBPrimSecChoose"
//#define Qc9KEeprom2GHzNoiseFloorThreshold "2GHz.NoiseFloorThreshold"		// OSPREY_MAX_CHAINS   
#define Qc9KEeprom2GHzXpaBiasLevel "2GHz.XpaBiasLevel"         
#define Qc9KEeprom2GHzXpaBiasBypass "2GHz.XpaBiasBypass"
#define Qc9KEeprom2GHzAntennaGain "2GHz.AntennaGain"						// signed
#define Qc9KEeprom2GHzAntennaControlCommon "2GHz.AntennaControlCommon"	// 1 x 32    idle, t1, t2, b (4 bits per setting)
	// define subfields?
#define Qc9KEeprom2GHzAntennaControlCommon2 "2GHz.AntennaControlCommon2"	// 1 x 32    ra1l1, ra2l1, ra1l2, ra2l2, ra12
	// define subfields?
#define Qc9KEeprom2GHzAntennaControlChain "2GHz.AntennaControlChain"		// OSPREY_MAX_CHAINS x 16    idle, t, r, rx1, rx12, b (2 bits each)
	// define subfields?
#define Qc9KEeprom2GHzRxFilterCap "2GHz.RxFilterCap"
#define Qc9KEeprom2GHzRxGainCap "2GHz.RxGainCap"
#define Qc9KEeprom2GHzTxRxGainTx "2GHz.GainTable.Tx"
#define Qc9KEeprom2GHzTxRxGainRx "2GHz.GainTable.Rx"
#define Qc9KEeprom2GHzNoiseFlrThr "2GHz.noiseFlrThr"
#define Qc9KEeprom2GHzMinCcaPwrChain   "2GHz.minCcaPwr"
#define Qc9KEeprom2GHzFuture "2GHz.Future"								// MAX_MODAL_FUTURE 

// 5GHz BiModal Header
#define Qc9KEeprom5GHzVoltageSlope "5GHz.VoltageSlope"					// signed
#define Qc9KEeprom5GHzSpur "5GHz.Spur"									// OSPREY_MAX_CHAINS
	#define Qc9KEeprom5GHzSpurAPrimSecChoose "5GHz.SpurAPrimSecChoose"
	#define Qc9KEeprom5GHzSpurBPrimSecChoose "5GHz.SpurBPrimSecChoose"
//#define Qc9KEeprom5GHzNoiseFloorThreshold "5GHz.NoiseFloorThreshold"		// OSPREY_MAX_CHAINS   
#define Qc9KEeprom5GHzXpaBiasLevel "5GHz.XpaBiasLevel"         
#define Qc9KEeprom5GHzXpaBiasBypass "5GHz.XpaBiasBypass"
#define Qc9KEeprom5GHzAntennaGain "5GHz.AntennaGain"						// signed
#define Qc9KEeprom5GHzAntennaControlCommon "5GHz.AntennaControlCommon"	// 1 x 32    idle, t1, t2, b (4 bits per setting)
	// define subfields?
#define Qc9KEeprom5GHzAntennaControlCommon2 "5GHz.AntennaControlCommon2"	// 1 x 32    ra1l1, ra2l1, ra1l2, ra2l2, ra12
	// define subfields?
#define Qc9KEeprom5GHzAntennaControlChain "5GHz.AntennaControlChain"		// OSPREY_MAX_CHAINS x 16    idle, t, r, rx1, rx12, b (2 bits each)
	// define subfields?
#define Qc9KEeprom5GHzRxFilterCap "5GHz.RxFilterCap"
#define Qc9KEeprom5GHzRxGainCap "5GHz.RxGainCap"
#define Qc9KEeprom5GHzTxRxGainTx "5GHz.GainTable.Tx"
#define Qc9KEeprom5GHzTxRxGainRx "5GHz.GainTable.Rx"
#define Qc9KEeprom5GHzNoiseFlrThr "5GHz.noiseFlrThr"
#define Qc9KEeprom5GHzMinCcaPwrChain "5GHz.minCcaPwr"
#define Qc9KEeprom5GHzFuture "5GHz.Future"								// MAX_MODAL_FUTURE 

// FreqModal Header
#define Qc9KEeprom2GHzXatten1Db "2GHz.Xatten1Db"
#define Qc9KEeprom5GHzXatten1DbLow "5GHz.Xatten1Db.Low"
#define Qc9KEeprom5GHzXatten1DbMid "5GHz.Xatten1Db.Mid"
#define Qc9KEeprom5GHzXatten1DbHigh "5GHz.Xatten1Db.High"

#define Qc9KEeprom2GHzXatten1Margin "2GHz.Xatten1Margin"
#define Qc9KEeprom5GHzXatten1MarginLow "5GHz.Xatten1Margin.Low"
#define Qc9KEeprom5GHzXatten1MarginMid "5GHz.Xatten1Margin.Mid"
#define Qc9KEeprom5GHzXatten1MarginHigh "5GHz.Xatten1Margin.High"

// Chip Cal Data
#define Qc9KEepromThermAdcScaledGain "ThermAdcScaledGain"   // 2B, "therm_adc_scaled_gain"
#define Qc9KEepromThermAdcOffset "ThermAdcOffset"       // 1B, "therm_adc_offset"

// 2GHz Cal Info
#define Qc9KEeprom2GHzCalibrationFrequency "2GHz.TxCalibration.Frequency"
#define Qc9KEeprom2GHzCalPoint "2GHz.TxCalibration.CalPoint"
	#define Qc9KEeprom2GHzCalPointTxGainIdx "2GHz.TxCalibration.CalPoint.TxgainIdx"
	#define Qc9KEeprom2GHzCalPointDacGain "2GHz.TxCalibration.CalPoint.DacGain"
	#define Qc9KEeprom2GHzCalPointPower "2GHz.TxCalibration.CalPoint.Power"
#define Qc9KEeprom2GHzCalibrationTemperature "2GHz.TxCalibration.Temperature"
#define Qc9KEeprom2GHzCalibrationVoltage "2GHz.TxCalibration.Voltage"	

#define Qc9KEeprom2GHzTargetFrequencyCck "2GHz.Target.Frequency.Cck"		// OSPREY_NUM_2G_CCK_TARGET_POWERS
#define Qc9KEeprom2GHzTargetFrequency "2GHz.Target.Frequency.Legacy"				// OSPREY_NUM_2G_20_TARGET_POWERS
#define Qc9KEeprom2GHzTargetFrequencyVht20 "2GHz.Target.Frequency.Vht20"		// OSPREY_NUM_2G_20_TARGET_POWERS
#define Qc9KEeprom2GHzTargetFrequencyVht40 "2GHz.Target.Frequency.Vht40"		// OSPREY_NUM_2G_40_TARGET_POWERS

#define Qc9KEeprom2GHzTargetPowerCck "2GHz.Target.Power.Cck"				// OSPREY_NUM_2G_CCK_TARGET_POWERS
#define Qc9KEeprom2GHzTargetPower "2GHz.Target.Power.Legacy"						// OSPREY_NUM_2G_20_TARGET_POWERS
#define Qc9KEeprom2GHzTargetPowerVht20 "2GHz.Target.Power.Vht20"				// OSPREY_NUM_2G_20_TARGET_POWERS
#define Qc9KEeprom2GHzTargetPowerVht40 "2GHz.Target.Power.Vht40"				// OSPREY_NUM_2G_40_TARGET_POWERS

// 2GHz Ctl
#define Qc9KEeprom2GHzCtlIndex "2GHz.Ctl.Index"							// OSPREY_NUM_CTLS_2G
#define Qc9KEeprom2GHzCtlFrequency "2GHz.Ctl.Frequency"					// [OSPREY_NUM_CTLS_2G][OSPREY_NUM_BAND_EDGES_2G]
#define Qc9KEeprom2GHzCtlPower "2GHz.Ctl.Power"							// [OSPREY_NUM_CTLS_2G][OSPREY_NUM_BAND_EDGES_2G]
#define Qc9KEeprom2GHzCtlBandEdge "2GHz.Ctl.BandEdge"						// [OSPREY_NUM_CTLS_2G][OSPREY_NUM_BAND_EDGES_2G]

#define Qc9KEeprom2GHzNoiseFloor "2GHz.RxCalibration.NoiseFloor"
#define Qc9KEeprom2GHzNoiseFloorPower "2GHz.RxCalibration.NoiseFloorPower"
#define Qc9KEeprom2GHzNoiseFloorTemperature "2GHz.RxCalibration.NoiseFloorTemperature"
#define Qc9KEeprom2GHzNoiseFloorTemperatureSlope "2GHz.RxCalibration.NoiseFloorTemperatureSlope"


// 2GHz Alpha Therm
//#define Qc9KEeprom2GHzAlphaThermChannel "2GHz.AlphaThermChannel"
#define Qc9KEeprom2GHzAlphaThermTable "2GHz.AlphaThermTable"

// 5GHz Cal Info
#define Qc9KEeprom5GHzCalibrationFrequency "5GHz.TxCalibration.Frequency"
#define Qc9KEeprom5GHzCalPoint "5GHz.TxCalibration.CalPoint"
	#define Qc9KEeprom5GHzCalPointTxGainIdx "5GHz.TxCalibration.CalPoint.TxGainIdx"
	#define Qc9KEeprom5GHzCalPointDacGain "5GHz.TxCalibration.CalPoint.DacGain"
	#define Qc9KEeprom5GHzCalPointPower "5GHz.TxCalibration.CalPoint.Power"
#define Qc9KEeprom5GHzCalibrationTemperature "5GHz.TxCalibration.Temperature"
#define Qc9KEeprom5GHzCalibrationVoltage "5GHz.TxCalibration.Voltage"	

#define Qc9KEeprom5GHzTargetFrequency "5GHz.Target.Frequency.Legacy"				// OSPREY_NUM_5G_20_TARGET_POWERS
#define Qc9KEeprom5GHzTargetFrequencyVht20 "5GHz.Target.Frequency.Vht20"		// OSPREY_NUM_5G_20_TARGET_POWERS
#define Qc9KEeprom5GHzTargetFrequencyVht40 "5GHz.Target.Frequency.Vht40"		// OSPREY_NUM_5G_40_TARGET_POWERS
#define Qc9KEeprom5GHzTargetFrequencyVht80 "5GHz.Target.Frequency.Vht80"		// OSPREY_NUM_5G_40_TARGET_POWERS

#define Qc9KEeprom5GHzTargetPower "5GHz.Target.Power.Legacy"						// OSPREY_NUM_5G_20_TARGET_POWERS
#define Qc9KEeprom5GHzTargetPowerVht20 "5GHz.Target.Power.Vht20"				// OSPREY_NUM_5G_20_TARGET_POWERS
#define Qc9KEeprom5GHzTargetPowerVht40 "5GHz.Target.Power.Vht40"				// OSPREY_NUM_5G_40_TARGET_POWERS
#define Qc9KEeprom5GHzTargetPowerVht80 "5GHz.Target.Power.Vht80"				// OSPREY_NUM_5G_40_TARGET_POWERS

// 5GHz Ctl
#define Qc9KEeprom5GHzCtlIndex "5GHz.Ctl.Index"							// OSPREY_NUM_CTLS_5G
#define Qc9KEeprom5GHzCtlFrequency "5GHz.Ctl.Frequency"					// [OSPREY_NUM_CTLS_5G][OSPREY_NUM_BAND_EDGES_5G]
#define Qc9KEeprom5GHzCtlPower "5GHz.Ctl.Power"							// [OSPREY_NUM_CTLS_5G][OSPREY_NUM_BAND_EDGES_5G]
#define Qc9KEeprom5GHzCtlBandEdge "5GHz.Ctl.BandEdge"						// [OSPREY_NUM_CTLS_5G][OSPREY_NUM_BAND_EDGES_5G]

#define Qc9KEeprom5GHzNoiseFloor "5GHz.RxCalibration.NoiseFloor"
#define Qc9KEeprom5GHzNoiseFloorPower "5GHz.RxCalibration.NoiseFloorPower"
#define Qc9KEeprom5GHzNoiseFloorTemperature "5GHz.RxCalibration.NoiseFloorTemperature"
#define Qc9KEeprom5GHzNoiseFloorTemperatureSlope "5GHz.RxCalibration.NoiseFloorTemperatureSlope"



// 5GHz Alpha Therm
//#define Qc9KEeprom5GHzAlphaThermChannel "5GHz.AlphaThermChannel"
#define Qc9KEeprom5GHzAlphaThermTable "5GHz.AlphaThermTable"

// Config Area
#define Qc9KEepromConfigAddr "ConfigAddr"


enum
{
	// Base header
    Qc9KSetEepromVersion=10000,
    Qc9KSetEepromTemplateVersion,
    Qc9KSetEepromALL,
    Qc9KSetEepromAllFirmware,
	Qc9KSetEepromConfig,
    Qc9KSetEepromConfigPCIe,
    Qc9KSetEepromDeviceId,
    Qc9KSetEepromSSID,
    Qc9KSetEepromVID,
    Qc9KSetEepromSVID,
    
    Qc9KSetEepromMacAddress,
    Qc9KSetEepromRegulatoryDomain,
    Qc9KSetEepromOpFlags,
    Qc9KSetEepromOpFlags2,
    Qc9KSetEepromBoardFlags,
	Qc9KSetEepromTxGainTblEepEna,
	Qc9KSetEepromTxGainTblEepScheme,
	Qc9KSetEeprom2GHzEnableXpaBias,
	Qc9KSetEeprom5GHzEnableXpaBias,
    Qc9KSetEepromBlueToothOptions,
    Qc9KSetEepromFeatureEnable,
    Qc9KSetEepromFeatureEnableTemperatureCompensation,
    Qc9KSetEepromFeatureEnableVoltageCompensation,
    Qc9KSetEepromFeatureEnableFastClock,
    Qc9KSetEepromFeatureEnableDoubling,
    Qc9KSetEepromFeatureEnableInternalSwitchingRegulator,
    Qc9KSetEepromEnableRbias,
    Qc9KSetEepromFeatureEnablePaPredistortion,
    Qc9KSetEepromFeatureEnableTuningCaps,
    Qc9KSetEepromMiscellaneous,
    Qc9KSetEepromMiscellaneousDriveStrength,
    Qc9KSetEepromMiscellaneousThermometer,
    Qc9KSetEepromMiscellaneousChainMaskReduce,
    Qc9KSetEepromMiscellaneousQuickDropEnable,
    Qc9KSetEepromMiscellaneousEep,
    Qc9KSetEepromBibxosc0,
    Qc9KSetEepromFlag1NoiseFlrThr,
    Qc9KSetEepromBinBuildNumber,
    Qc9KSetEepromTxRxMask,
    Qc9KSetEepromTxRxMaskTx,
    Qc9KSetEepromTxRxMaskRx,
    Qc9KSetEepromRfSilent,
    Qc9KSetEepromRfSilentB0,
    Qc9KSetEepromRfSilentB1,
    Qc9KSetEepromRfSilentGpio,
    Qc9KSetEepromWlanLedGpio,
    Qc9KSetEepromDeviceType,
    Qc9KSetEepromCtlVersion,
    Qc9KSetEepromSpurBaseA,
    Qc9KSetEepromSpurBaseB,
    Qc9KSetEepromSpurRssiThresh,
    Qc9KSetEepromSpurRssiThreshCck,
    Qc9KSetEepromSpurMitFlag,
    Qc9KSetEepromSwreg,
	Qc9KSetEepromSwregProgram,
	Qc9KSetEepromTxRxGain,
    Qc9KSetEepromTxRxGainTx,
    Qc9KSetEepromTxRxGainRx,
    Qc9KSetEepromPowerTableOffset,
    Qc9KSetEepromTuningCaps,
    Qc9KSetEepromDeltaCck20,
    Qc9KSetEepromDelta4020,
    Qc9KSetEepromDelta8020,
    Qc9KSetEepromCustomerData,
    Qc9KSetEepromBaseFuture,

	// 2GHz Bimodal Header
    Qc9KSetEeprom2GHzVoltageSlope,
    Qc9KSetEeprom2GHzSpur,
    Qc9KSetEeprom2GHzSpurAPrimSecChoose,
    Qc9KSetEeprom2GHzSpurBPrimSecChoose,
    //Qc9KSetEeprom2GHzNoiseFloorThreshold,
    Qc9KSetEeprom2GHzXpaBiasLevel,     
    Qc9KSetEeprom2GHzXpaBiasBypass,
    Qc9KSetEeprom2GHzAntennaGain,
    Qc9KSetEeprom2GHzAntennaControlCommon,
    Qc9KSetEeprom2GHzAntennaControlCommon2,
    Qc9KSetEeprom2GHzAntennaControlChain,
    Qc9KSetEeprom2GHzRxFilterCap,
    Qc9KSetEeprom2GHzRxGainCap,
    Qc9KSetEeprom2GHzTxRxGainTx,
    Qc9KSetEeprom2GHzTxRxGainRx,
	Qc9KSetEeprom2GHzNoiseFlrThr,
	Qc9KSetEeprom2GHzMinCcaPwrChain,
    Qc9KSetEeprom2GHzFuture,

	// 5GHz Bimodal Header
    Qc9KSetEeprom5GHzVoltageSlope,
    Qc9KSetEeprom5GHzSpur,
    Qc9KSetEeprom5GHzSpurAPrimSecChoose,
    Qc9KSetEeprom5GHzSpurBPrimSecChoose,
    //Qc9KSetEeprom5GHzNoiseFloorThreshold,
    Qc9KSetEeprom5GHzXpaBiasLevel,     
    Qc9KSetEeprom5GHzXpaBiasBypass, 
    Qc9KSetEeprom5GHzAntennaGain,
    Qc9KSetEeprom5GHzAntennaControlCommon,
    Qc9KSetEeprom5GHzAntennaControlCommon2,
    Qc9KSetEeprom5GHzAntennaControlChain,
    Qc9KSetEeprom5GHzRxFilterCap,
    Qc9KSetEeprom5GHzRxGainCap,
    Qc9KSetEeprom5GHzTxRxGainTx,
    Qc9KSetEeprom5GHzTxRxGainRx,
	Qc9KSetEeprom5GHzNoiseFlrThr,
	Qc9KSetEeprom5GHzMinCcaPwrChain,
    Qc9KSetEeprom5GHzFuture,


	// FreqModal Header
    Qc9KSetEeprom2GHzXatten1Db,
    Qc9KSetEeprom5GHzXatten1DbLow,
    Qc9KSetEeprom5GHzXatten1DbMid,
    Qc9KSetEeprom5GHzXatten1DbHigh,

    Qc9KSetEeprom2GHzXatten1Margin ,
    Qc9KSetEeprom5GHzXatten1MarginLow,
    Qc9KSetEeprom5GHzXatten1MarginMid,
    Qc9KSetEeprom5GHzXatten1MarginHigh,

	// Chip Cal Data
    Qc9KSetEepromThermAdcScaledGain,
    Qc9KSetEepromThermAdcOffset,

	// 2GHz Cal Info
	Qc9KSetEeprom2GHzCalibrationFrequency,
	Qc9KSetEeprom2GHzCalPoint,
	Qc9KSetEeprom2GHzCalPointTxGainIdx,
	Qc9KSetEeprom2GHzCalPointDacGain,
	Qc9KSetEeprom2GHzCalPointPower,
    Qc9KSetEeprom2GHzCalibrationTemperature,
    Qc9KSetEeprom2GHzCalibrationVoltage,

    Qc9KSetEeprom2GHzTargetFrequencyCck,
    Qc9KSetEeprom2GHzTargetFrequency,
    Qc9KSetEeprom2GHzTargetFrequencyVht20,
    Qc9KSetEeprom2GHzTargetFrequencyVht40,
    Qc9KSetEeprom2GHzTargetPowerCck,
    Qc9KSetEeprom2GHzTargetPower,
    Qc9KSetEeprom2GHzTargetPowerVht20,
    Qc9KSetEeprom2GHzTargetPowerVht40,

	// 2GHz Ctl
    Qc9KSetEeprom2GHzCtlIndex,
    Qc9KSetEeprom2GHzCtlFrequency,
    Qc9KSetEeprom2GHzCtlPower,
    Qc9KSetEeprom2GHzCtlBandEdge,

	Qc9KSetEeprom2GHzNoiseFloor,
	Qc9KSetEeprom2GHzNoiseFloorPower,
	Qc9KSetEeprom2GHzNoiseFloorTemperature,
	Qc9KSetEeprom2GHzNoiseFloorTemperatureSlope,


	// 2GHz Alpha Therm
    //Qc9KSetEeprom2GHzAlphaThermChannel,
    Qc9KSetEeprom2GHzAlphaThermTable,

	// 5GHz Cal Info
	Qc9KSetEeprom5GHzCalibrationFrequency,
	Qc9KSetEeprom5GHzCalPoint,
	Qc9KSetEeprom5GHzCalPointTxGainIdx,
	Qc9KSetEeprom5GHzCalPointDacGain,
	Qc9KSetEeprom5GHzCalPointPower,
    Qc9KSetEeprom5GHzCalibrationTemperature,
    Qc9KSetEeprom5GHzCalibrationVoltage,


    Qc9KSetEeprom5GHzTargetFrequency,
    Qc9KSetEeprom5GHzTargetFrequencyVht20,
    Qc9KSetEeprom5GHzTargetFrequencyVht40,
    Qc9KSetEeprom5GHzTargetFrequencyVht80,
	Qc9KSetEeprom5GHzTargetPower,
    Qc9KSetEeprom5GHzTargetPowerVht20,
    Qc9KSetEeprom5GHzTargetPowerVht40,
    Qc9KSetEeprom5GHzTargetPowerVht80,

	// 5GHz Ctl
	Qc9KSetEeprom5GHzCtlIndex,
    Qc9KSetEeprom5GHzCtlFrequency,
    Qc9KSetEeprom5GHzCtlPower,
    Qc9KSetEeprom5GHzCtlBandEdge,
	Qc9KSetEeprom5GHzNoiseFloor,
	Qc9KSetEeprom5GHzNoiseFloorPower,
	Qc9KSetEeprom5GHzNoiseFloorTemperature,
    Qc9KSetEeprom5GHzNoiseFloorTemperatureSlope,
	// 5GHz Alpha Therm
    //Qc9KSetEeprom5GHzAlphaThermChannel,
    Qc9KSetEeprom5GHzAlphaThermTable,

	// Config
	Qc9KSetEepromConfigAddr,
};


#endif //_QC9K_EEPROM_PARAMETER_H_
