


#define Ar9300EepromVersion "Version"										// decimal
#define Ar9300EepromTemplateVersion "Template"								// decimal
#define Ar9300EepromMacAddress "Mac"										// 6
#define Ar9300EepromCustomerData  "Customer"								// OSPREY_CUSTOMER_DATA_SIZE
#define Ar9300EepromRegulatoryDomain "RegulatoryDomain"						// 2 x 16
	// define subfields?
#define Ar9300EepromTxRxMask "Mask"
	#define Ar9300EepromTxRxMaskTx "Mask.Tx"
	#define Ar9300EepromTxRxMaskRx "Mask.Rx"
#define Ar9300EepromOpFlags "OpFlags"
#define Ar9300EepromEepMisc "EepMisc"
#define Ar9300EepromRfSilent "RfSilent"
	#define Ar9300EepromRfSilentB0 "RfSilent.HardwareEnable"
	#define Ar9300EepromRfSilentB1 "RfSilent.Polarity"
	#define Ar9300EepromRfSilentGpio "RfSilent.Gpio"
#define Ar9300EepromBlueToothOptions "BlueToothOptions"
#define Ar9300EepromDeviceCapability "DeviceCapability"
#define Ar9300EepromDeviceType "DeviceType"
#define Ar9300EepromPowerTableOffset "PowerTableOffset"
#define Ar9300EepromTuningCaps "TuningCaps"
#define Ar9300EepromFeatureEnable "FeatureEnable"
	#define Ar9300EepromFeatureEnableTemperatureCompensation "FeatureEnable.TemperatureCompensation"
	#define Ar9300EepromFeatureEnableVoltageCompensation "FeatureEnable.VoltageCompensation"
	#define Ar9300EepromFeatureEnableFastClock "FeatureEnable.FastClock"
	#define Ar9300EepromFeatureEnableDoubling "FeatureEnable.Doubling"
	#define Ar9300EepromFeatureEnableInternalSwitchingRegulator "FeatureEnable.SwitchingRegulator"
	#define Ar9300EepromFeatureEnablePaPredistortion "FeatureEnable.PaPredistortion"
	#define Ar9300EepromFeatureEnableTuningCaps "FeatureEnable.TuningCaps"
	#define Ar9300EepromFeatureEnableTxFrameToXpaOn "FeatureEnable.TxFrameToXpaOn"
#define Ar9300EepromMiscellaneous "Miscellaneous"
	#define Ar9300EepromMiscellaneousDriveStrength "Miscellaneous.DriveStrength"
	#define Ar9300EepromMiscellaneousThermometer "Miscellaneous.Thermometer"
	#define Ar9300EepromMiscellaneousChainMaskReduce "Miscellaneous.Dynamic2x3"
	#define Ar9300EepromMiscellaneousQuickDropEnable "Miscellaneous.QuickDropEnable"
	#define Ar9300EepromMiscellaneousTempSlopExtensionEnable "Miscellaneous.TempSlopExtensionEnable"			
	#define Ar9300EepromMiscellaneousXLNABiasStrengthEnable "Miscellaneous.xlnaBiasStrengthEnable"
	#define Ar9300EepromMiscellaneousRFGainCAPEnable "Miscellaneous.RFGainCAPEnable"
#define Ar9300EepromEepromWriteEnableGpio "EepromWriteEnableGpio"			// decimal
#define Ar9300EepromWlanDisableGpio "WlanDisableGpio"						// decimal
#define Ar9300EepromWlanLedGpio "WlanLedGpio"								// decimal
#define Ar9300EepromRxBandSelectGpio "RxBandSelectGpio"						// decimal
#define Ar9300EepromTxRxGain "GainTable"
	#define Ar9300EepromTxRxGainTx "GainTable.Tx"
	#define Ar9300EepromTxRxGainRx "GainTable.Rx"
#define Ar9300EepromSwitchingRegulator "SwitchingRegulator"					// 1 x 32
	// define subfields of switching regulator?

#define Ar9300Eeprom2GHzAntennaControlCommon "2GHz.AntennaControlCommon"	// 1 x 32    idle, t1, t2, b (4 bits per setting)
	// define subfields?
#define Ar9300Eeprom2GHzAntennaControlCommon2 "2GHz.AntennaControlCommon2"	// 1 x 32    ra1l1, ra2l1, ra1l2, ra2l2, ra12
	// define subfields?
#define Ar9300Eeprom2GHzAntennaControlChain "2GHz.AntennaControlChain"		// OSPREY_MAX_CHAINS x 16    idle, t, r, rx1, rx12, b (2 bits each)
	// define subfields?
#define Ar9300Eeprom2GHzAttenuationDb "2GHz.Attenuation.Db"					// OSPREY_MAX_CHAINS x 8
#define Ar9300Eeprom2GHzAttenuationMargin "2GHz.Attenuation.Margin"			// OSPREY_MAX_CHAINS x 8
#define Ar9300Eeprom2GHzTemperatureSlope "2GHz.TemperatureSlope"			// signed
#define Ar9300Eeprom2GHzVoltageSlope "2GHz.VoltageSlope"					// signed
#define Ar9300Eeprom2GHzSpur "2GHz.Spur"									// OSPREY_MAX_CHAINS
#define Ar9300Eeprom2GHzMinCCAPwrThreshold "2GHz.MinCCAPwrThreshold"		// OSPREY_MAX_CHAINS   
#define Ar9300Eeprom2GHzReserved "2GHz.Reserved"							// MAX_MODAL_RESERVED
#define Ar9300Eeprom2GHzQuickDrop "2GHz.QuickDrop"							// signed
#define Ar9300Eeprom2GHzXpaBiasLevel "2GHz.XpaBiasLevel"         
#define Ar9300Eeprom2GHzTxFrameToDataStart "2GHz.TxFrameToDataStart"         
#define Ar9300Eeprom2GHzTxFrameToPaOn "2GHz.TxFrameToPaOn"         
#define Ar9300Eeprom2GHzTxClip "2GHz.TxClip"								// 4 bits tx_clip
#define Ar9300Eeprom2GHzDacScaleCCK "2GHz.DacScaleCCK"					//  4 bits dac_scale_cck
#define Ar9300Eeprom2GHzAntennaGain "2GHz.AntennaGain"						// signed
#define Ar9300Eeprom2GHzSwitchSettling "2GHz.SwitchSettling"	
#define Ar9300Eeprom2GHzAdcSize "2GHz.AdcSize"								// signed
#define Ar9300Eeprom2GHzTxEndToXpaOff "2GHz.TxEndToXpaOff"				
#define Ar9300Eeprom2GHzTxEndToRxOn "2GHz.TxEndToRxOn"				
#define Ar9300Eeprom2GHzTxFrameToXpaOn "2GHz.TxFrameToXpaOn"         
#define Ar9300Eeprom2GHzThresh62 "2GHz.Thresh62"         
#define Ar9300Eeprom2GHzPaPredistortionHt20 "2GHz.PaPredistortion.Ht20"      // 1 x 32   
	// define subfields?
#define Ar9300Eeprom2GHzPaPredistortionHt40 "2GHz.PaPredistortion.Ht40"      // 1 x 32   
	// define subfields?
#define Ar9300Eeprom2GHzWlanSpdtSwitchGlobalControl "2GHz.WlanSpdtSwitchGlobalControl"
#define Ar9300Eeprom2GHzXLNABiasStrength "2GHz.XLNABiasStrength"
#define Ar9300Eeprom2GHzRFGainCAP "2GHz.RFGainCap"
#define Ar9300Eeprom2GHzTXGainCAP "2GHz.TXGainCap"
#define Ar9300Eeprom2GHzFuture "2GHz.Future"								// MAX_MODAL_FUTURE 


#define Ar9300EepromAntennaDiversityControl "AntennaDiversityControl"	
#define Ar9300EepromMiscEnable "MiscEnable"
	#define Ar9300EepromMiscEnableTXGainCAPEnable "MiscEnable.TXGainCAPEnable"
#define Ar9300EepromMiscEnableMinCCAPwrthresholdEnable "MiscEnable.MinCCAPwrThresholdEnable"
#define Ar9300Eeprom5GHzQuickDropLow "5GHz.QuickDrop.Low"						// signed
#define Ar9300Eeprom5GHzQuickDropHigh "5GHz.QuickDrop.High"						// signed
#define Ar9300EepromFuture "Future"											// MAX_BASE_EXTENSION_FUTURE 


#define Ar9300Eeprom2GHzCalibrationFrequency "2GHz.PowerCalibration.Frequency"	// OSPREY_NUM_2G_CAL_PIERS
#define Ar9300Eeprom2GHzCalibrationData "2GHz.Calibration.Data"				// [OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS][6]
#define Ar9300Eeprom2GHzPowerCorrection "2GHz.TransmitCalibration.PowerCorrection"				// signed  [OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS], interleave 6
#define Ar9300Eeprom2GHzCalibrationVoltage "2GHz.TransmitCalibration.Voltage"	// [OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS], interleave 6
#define Ar9300Eeprom2GHzCalibrationTemperature "2GHz.TransmitCalibration.Temperature"	// [OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS], interleave 6
#define Ar9300Eeprom2GHzNoiseFloor "2GHz.ReceiveCalibration.NoiseFloor"						// signed  [OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS], interleave 6
#define Ar9300Eeprom2GHzNoiseFloorPower "2GHz.ReceiveCalibration.Power"				// signed  [OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS], interleave 6
#define Ar9300Eeprom2GHzNoiseFloorTemperature	"2GHz.ReceiveCalibration.Temperature"// [OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS], interleave 6
#define Ar9300Eeprom2GHzTargetFrequencyCck "2GHz.Target.Frequency.Cck"		// OSPREY_NUM_2G_CCK_TARGET_POWERS
#define Ar9300Eeprom2GHzTargetFrequency "2GHz.Target.Frequency.Legacy"				// OSPREY_NUM_2G_20_TARGET_POWERS
#define Ar9300Eeprom2GHzTargetFrequencyHt20 "2GHz.Target.Frequency.Ht20"		// OSPREY_NUM_2G_20_TARGET_POWERS
#define Ar9300Eeprom2GHzTargetFrequencyHt40 "2GHz.Target.Frequency.Ht40"		// OSPREY_NUM_2G_40_TARGET_POWERS
#define Ar9300Eeprom2GHzTargetPowerCck "2GHz.Target.Power.Cck"				// OSPREY_NUM_2G_CCK_TARGET_POWERS
#define Ar9300Eeprom2GHzTargetPower "2GHz.Target.Power.Legacy"						// OSPREY_NUM_2G_20_TARGET_POWERS
#define Ar9300Eeprom2GHzTargetPowerHt20 "2GHz.Target.Power.Ht20"				// OSPREY_NUM_2G_20_TARGET_POWERS
#define Ar9300Eeprom2GHzTargetPowerHt40 "2GHz.Target.Power.Ht40"				// OSPREY_NUM_2G_40_TARGET_POWERS
#define Ar9300Eeprom2GHzCtlIndex "2GHz.Ctl.Index"							// OSPREY_NUM_CTLS_2G
#define Ar9300Eeprom2GHzCtlFrequency "2GHz.Ctl.Frequency"					// [OSPREY_NUM_CTLS_2G][OSPREY_NUM_BAND_EDGES_2G]
#define Ar9300Eeprom2GHzCtlPower "2GHz.Ctl.Power"							// [OSPREY_NUM_CTLS_2G][OSPREY_NUM_BAND_EDGES_2G]
#define Ar9300Eeprom2GHzCtlBandEdge "2GHz.Ctl.BandEdge"						// [OSPREY_NUM_CTLS_2G][OSPREY_NUM_BAND_EDGES_2G]


#define Ar9300Eeprom5GHzAntennaControlCommon "5GHz.Antenna.Common"	// 1 x 32    idle, t1, t2, b (4 bits per setting)
#define Ar9300Eeprom5GHzAntennaControlCommon2 "5GHz.Antenna.Common2"	// 4 x 16    ra1l1, ra2l1, ra1l2, ra2l2, ra12
#define Ar9300Eeprom5GHzAntennaControlChain "5GHz.Antenna.Chain"		// OSPREY_MAX_CHAINS x 16   6   idle, t, r, rx1, rx12, b (2 bits each)
#define Ar9300Eeprom5GHzAttenuationDb "5GHz.Attenuation.Db.Middle"					// OSPREY_MAX_CHAINS
#define Ar9300Eeprom5GHzAttenuationMargin "5GHz.Attenuation.Margin.Middle"			// OSPREY_MAX_CHAINS
#define Ar9300Eeprom5GHzTemperatureSlope "5GHz.TemperatureSlope.Middle"			// signed
#define Ar9300Eeprom5GHzTemperatureSlopeExtension "5GHz.TemperatureSlope.Extension"			

#define Ar9300Eeprom5GHzVoltageSlope "5GHz.VoltageSlope.Middle"					// signed
#define Ar9300Eeprom5GHzSpur "5GHz.Spur"									// OSPREY_MAX_CHAINS
#define Ar9300Eeprom5GHzMinCCAPwrThreshold "5GHz.MinCCAPwrThreshold"		// OSPREY_MAX_CHAINS   
#define Ar9300Eeprom5GHzReserved "5GHz.Reserved"							// MAX_MODAL_RESERVED
#define Ar9300Eeprom5GHzQuickDrop "5GHz.QuickDrop.Middle"							// signed
#define Ar9300Eeprom5GHzXpaBiasLevel "5GHz.XpaBiasLevel"         
#define Ar9300Eeprom5GHzTxFrameToDataStart "5GHz.TxFrameToDataStart"         
#define Ar9300Eeprom5GHzTxFrameToPaOn "5GHz.TxFrameToPaOn"         
#define Ar9300Eeprom5GHzTxClip "5GHz.TxClip"								// 4 bits tx_clip, 4 bits dac_scale_cck
#define Ar9300Eeprom5GHzAntennaGain "5GHz.AntennaGain"						// signed
#define Ar9300Eeprom5GHzSwitchSettling "5GHz.SwitchSettling"	
#define Ar9300Eeprom5GHzAdcSize "5GHz.AdcSize"								// signed
#define Ar9300Eeprom5GHzTxEndToXpaOff "5GHz.TxEndToXpaOff"				
#define Ar9300Eeprom5GHzTxEndToRxOn "5GHz.TxEndToRxOn"				
#define Ar9300Eeprom5GHzTxFrameToXpaOn "5GHz.TxFrameToXpaOn"         
#define Ar9300Eeprom5GHzThresh62 "5GHz.Thresh62"         
#define Ar9300Eeprom5GHzPaPredistortionHt20 "5GHz.PaPredistortion.Ht20"      // 1 x 32   
#define Ar9300Eeprom5GHzPaPredistortionHt40 "5GHz.PaPredistortion.Ht40"      // 1 x 32   
#define Ar9300Eeprom5GHzWlanSpdtSwitchGlobalControl "5GHz.WlanSpdtSwitchGlobalControl"
#define Ar9300Eeprom5GHzXLNABiasStrength "5GHz.XLNABiasStrength"
#define Ar9300Eeprom5GHzRFGainCAP "5GHz.RFGainCap"
#define Ar9300Eeprom5GHzTXGainCAP "5GHz.TXGainCap"
#define Ar9300Eeprom5GHzFuture "5GHz.Future"								// MAX_MODAL_FUTURE 
#define Ar9300Eeprom5GHzTemperatureSlopeLow "5GHz.TemperatureSlope.Low"		// signed
#define Ar9300Eeprom5GHzTemperatureSlopeHigh "5GHz.TemperatureSlope.High"	// signed
#define Ar9300Eeprom5GHzAttenuationDbLow "5GHz.Attenuation.Db.Low"			// OSPREY_MAX_CHAINS x 8
#define Ar9300Eeprom5GHzAttenuationDbHigh "5GHz.Attenuation.Db.High"			// OSPREY_MAX_CHAINS x 8
#define Ar9300Eeprom5GHzAttenuationMarginLow "5GHz.Attenuation.Margin.Low"	// OSPREY_MAX_CHAINS x 8
#define Ar9300Eeprom5GHzAttenuationMarginHigh "5GHz.Attenuation.Margin.High"	// OSPREY_MAX_CHAINS x 8

#define Ar9300Eeprom5GHzCalibrationFrequency "5GHz.TransmitCalibration.Frequency"	// OSPREY_NUM_5G_CAL_PIERS
#define Ar9300Eeprom5GHzCalibrationData "5GHz.Calibration.Data"				// [OSPREY_MAX_CHAINS][OSPREY_NUM_5G_CAL_PIERS][6]
#define Ar9300Eeprom5GHzPowerCorrection "5GHz.TransmitCalibration.PowerCorrection"				// [OSPREY_MAX_CHAINS][OSPREY_NUM_5G_CAL_PIERS]
#define Ar9300Eeprom5GHzCalibrationVoltage "5GHz.TransmitCalibration.Voltage"		// [OSPREY_MAX_CHAINS][OSPREY_NUM_5G_CAL_PIERS]
#define Ar9300Eeprom5GHzCalibrationTemperature "5GHz.TransmitCalibration.Temperature"// [OSPREY_MAX_CHAINS][OSPREY_NUM_5G_CAL_PIERS]
#define Ar9300Eeprom5GHzNoiseFloor "5GHz.ReceiveCalibration.NoiseFloor"						// [OSPREY_MAX_CHAINS][OSPREY_NUM_5G_CAL_PIERS]
#define Ar9300Eeprom5GHzNoiseFloorPower "5GHz.ReceiveCalibration.Power"				// [OSPREY_MAX_CHAINS][OSPREY_NUM_5G_CAL_PIERS]
#define Ar9300Eeprom5GHzNoiseFloorTemperature "5GHz.ReceiveCalibration.Temperature"	// [OSPREY_MAX_CHAINS][OSPREY_NUM_5G_CAL_PIERS]

#define Ar9300Eeprom5GHzTargetFrequency "5GHz.Target.Frequency.Legacy"				// OSPREY_NUM_5G_20_TARGET_POWERS
#define Ar9300Eeprom5GHzTargetFrequencyHt20 "5GHz.Target.Frequency.Ht20"		// OSPREY_NUM_5G_20_TARGET_POWERS
#define Ar9300Eeprom5GHzTargetFrequencyHt40 "5GHz.Target.Frequency.Ht40"		// OSPREY_NUM_5G_40_TARGET_POWERS

#define Ar9300Eeprom5GHzTargetPower "5GHz.Target.Power.Legacy"						// OSPREY_NUM_5G_20_TARGET_POWERS
#define Ar9300Eeprom5GHzTargetPowerHt20 "5GHz.Target.Power.Ht20"				// OSPREY_NUM_5G_20_TARGET_POWERS
#define Ar9300Eeprom5GHzTargetPowerHt40 "5GHz.Target.Power.Ht40"				// OSPREY_NUM_5G_40_TARGET_POWERS
#define Ar9300Eeprom5GHzCtlIndex "5GHz.Ctl.Index"							// OSPREY_NUM_CTLS_5G
#define Ar9300Eeprom5GHzCtlFrequency "5GHz.Ctl.Frequency"					// [OSPREY_NUM_CTLS_5G][OSPREY_NUM_BAND_EDGES_5G]
#define Ar9300Eeprom5GHzCtlPower "5GHz.Ctl.Power"							// [OSPREY_NUM_CTLS_5G][OSPREY_NUM_BAND_EDGES_5G]
#define Ar9300Eeprom5GHzCtlBandEdge "5GHz.Ctl.BandEdge"						// [OSPREY_NUM_CTLS_5G][OSPREY_NUM_BAND_EDGES_5G]
#define Ar9300CaldataMemoryType "caldata.memory"						// calibration data storage type
#define Ar93002GPastPower "psat.2g.power"
#define Ar93005GPastPower "psat.5g.power"
#define Ar93002GDiff_OFDM_CW_Power "psat.2g.diff"
#define Ar93005GDiff_OFDM_CW_Power "psat.5g.diff"


