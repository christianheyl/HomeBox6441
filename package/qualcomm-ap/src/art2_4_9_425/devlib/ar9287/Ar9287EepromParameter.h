
#define Ar9287EepromLength "Length"											// decimal
#define Ar9287EepromChecksum "CheckSum"										// decimal
#define Ar9287EepromRegDmn "RegDmn"

#define Ar9287EepromVersion "Version"										// decimal
#define Ar9287EepromTemplateVersion "Template"								// decimal
#define Ar9287EepromMacAddress "Mac"										// 6
#define Ar9287EepromCustomerData  "Customer"								// OSPREY_CUSTOMER_DATA_SIZE
#define Ar9287EepromRegulatoryDomain "RegulatoryDomain"						// 2 x 16
	// define subfields?
#define Ar9287EepromTxRxMask "Mask"
	#define Ar9287EepromTxRxMaskTx "Mask.Tx"
	#define Ar9287EepromTxRxMaskRx "Mask.Rx"
#define Ar9287EepromOpFlags "OpFlags"
#define Ar9287EepromEepMisc "EepMisc"
#define Ar9287EepromRfSilent "RfSilent"
	#define Ar9287EepromRfSilentB0 "RfSilent.HardwareEnable"
	#define Ar9287EepromRfSilentB1 "RfSilent.Polarity"
	#define Ar9287EepromRfSilentGpio "RfSilent.Gpio"
#define Ar9287EepromBlueToothOptions "BlueToothOptions"
#define Ar9287EepromDeviceCapability "DeviceCapability"
#define Ar9287EepromBinBuildNumber "BinBuildNumber"
#define Ar9287EepromDeviceType "DeviceType"
#define Ar9287EepromOpenLoopPwrCntl "OpenLoopPwrCntl"
#define Ar9287EepromPowerTableOffset "PowerTableOffset"
#define Ar9287EepromTuningCaps "TuningCaps"
#define Ar9287EepromFeatureEnable "FeatureEnable"
	#define Ar9287EepromFeatureEnableTemperatureCompensation "FeatureEnable.TemperatureCompensation"
	#define Ar9287EepromFeatureEnableVoltageCompensation "FeatureEnable.VoltageCompensation"
	#define Ar9287EepromFeatureEnableFastClock "FeatureEnable.FastClock"
	#define Ar9287EepromFeatureEnableDoubling "FeatureEnable.Doubling"
	#define Ar9287EepromFeatureEnableInternalSwitchingRegulator "FeatureEnable.SwitchingRegulator"
	#define Ar9287EepromFeatureEnablePaPredistortion "FeatureEnable.PaPredistortion"
	#define Ar9287EepromFeatureEnableTuningCaps "FeatureEnable.TuningCaps"
	#define Ar9287EepromFeatureEnableTxFrameToXpaOn "FeatureEnable.TxFrameToXpaOn"
#define Ar9287EepromMiscellaneous "Miscellaneous"
	#define Ar9287EepromMiscellaneousDriveStrength "Miscellaneous.DriveStrength"
	#define Ar9287EepromMiscellaneousThermometer "Miscellaneous.Thermometer"
	#define Ar9287EepromMiscellaneousChainMaskReduce "Miscellaneous.Dynamic2x3"
	#define Ar9287EepromMiscellaneousQuickDropEnable "Miscellaneous.QuickDropEnable"
	#define Ar9287EepromMiscellaneousTempSlopExtensionEnable "Miscellaneous.TempSlopExtensionEnable"			
	#define Ar9287EepromMiscellaneousXLNABiasStrengthEnable "Miscellaneous.xlnaBiasStrengthEnable"
	#define Ar9287EepromMiscellaneousRFGainCAPEnable "Miscellaneous.RFGainCAPEnable"
#define Ar9287EepromEepromWriteEnableGpio "EepromWriteEnableGpio"			// decimal
#define Ar9287EepromWlanDisableGpio "WlanDisableGpio"						// decimal
#define Ar9287EepromWlanLedGpio "WlanLedGpio"								// decimal
#define Ar9287EepromRxBandSelectGpio "RxBandSelectGpio"						// decimal
#define Ar9287EepromTxRxGain "GainTable"
	#define Ar9287EepromTxRxGainTx "GainTable.Tx"
	#define Ar9287EepromTxRxGainRx "GainTable.Rx"
#define Ar9287EepromSwitchingRegulator "SwitchingRegulator"					// 1 x 32
	// define subfields of switching regulator?

#define Ar9287Eeprom2GHzAntennaControlChain "2GHz.AntennaControlChain"		// OSPREY_MAX_CHAINS x 16    idle, t, r, rx1, rx12, b (2 bits each)
	// define subfields?
#define Ar9287Eeprom2GHzAntennaControlCommon "2GHz.AntennaControlCommon"	// 1 x 32    idle, t1, t2, b (4 bits per setting)
	// define subfields?
#define Ar9287Eeprom2GHzAntennaControlCommon2 "2GHz.AntennaControlCommon2"	// 1 x 32    ra1l1, ra2l1, ra1l2, ra2l2, ra12
	// define subfields?
#define Ar9287Eeprom2GHzAttenuationDb "2GHz.Attenuation.Db"					// OSPREY_MAX_CHAINS x 8
#define Ar9287Eeprom2GHzAttenuationMargin "2GHz.Attenuation.Margin"			// OSPREY_MAX_CHAINS x 8
#define Ar9287Eeprom2GHzTemperatureSlope "2GHz.TemperatureSlope"			// signed
#define Ar9287Eeprom2GHzTemperatureSlopePalOn "2GHz.TemperatureSlopePalOn"	// signed
#define Ar9287Eeprom2GHzFutureBase "FutureBase"
#define Ar9287Eeprom2GHzVoltageSlope "2GHz.VoltageSlope"					// signed
#define Ar9287Eeprom2GHzSpur "2GHz.Spur"									// OSPREY_MAX_CHAINS
#define Ar9287Eeprom2GHzSpurRangeLow "2GHz.SpurRangeLow"
#define Ar9287Eeprom2GHzSpurRangeHigh "2GHz.SpurRangeHigh"
#define Ar9287Eeprom2GHzNoiseFloorThreshold "2GHz.NoiseFloorThreshold"		// OSPREY_MAX_CHAINS   
#define Ar9287Eeprom2GHzXpdGain "2GHz.XpdGain"
#define Ar9287Eeprom2GHzXpd "2GHz.Xpd"
#define Ar9287Eeprom2GHzIqCalICh "2GHz.IqCalICh"
#define Ar9287Eeprom2GHzIqCalQCh "2GHz.IqCalQCh"
#define Ar9287Eeprom2GHzPdGainOverlap "2GHz.PdGainOverlap"
#define Ar9287Eeprom2GHzReserved "2GHz.Reserved"							// MAX_MODAL_RESERVED
#define Ar9287Eeprom2GHzQuickDrop "2GHz.QuickDrop"							// signed
#define Ar9287Eeprom2GHzXpaBiasLevel "2GHz.XpaBiasLevel"         
#define Ar9287Eeprom2GHzTxFrameToDataStart "2GHz.TxFrameToDataStart"         
#define Ar9287Eeprom2GHzTxFrameToPaOn "2GHz.TxFrameToPaOn"         
#define Ar9287Eeprom2GHzHt40PowerIncForPdadc "2GHz.Ht40PowerIncForPdadc"         
#define Ar9287Eeprom2GHzBswAtten "2GHz.BswAtten"         
#define Ar9287Eeprom2GHzBswMargin "2GHz.BswMargin"         
#define Ar9287Eeprom2GHzSwSettleHt40 "2GHz.SwSettleHt40"         
#define Ar9287Eeprom2GHzModalHeaderVersion "2GHz.ModalHeaderVersion"         
#define Ar9287Eeprom2GHzDb1 "2GHz.Db1"         
#define Ar9287Eeprom2GHzDb2 "2GHz.Db2"         
#define Ar9287Eeprom2GHzObcck "2GHz.ObCck"         
#define Ar9287Eeprom2GHzObPsk "2GHz.ObPsk"         
#define Ar9287Eeprom2GHzObQam "2GHz.ObQam"         
#define Ar9287Eeprom2GHzObPalOff "2GHz.ObPalOff"         
#define Ar9287Eeprom2GHzFutureModal "2GHz.FutureModal"         

#define Ar9287Eeprom2GHzTxClip "2GHz.TxClip"								// 4 bits tx_clip
#define Ar9287Eeprom2GHzDacScaleCCK "2GHz.DacScaleCCK"					//  4 bits dac_scale_cck
#define Ar9287Eeprom2GHzAntennaGain "2GHz.AntennaGain"						// signed
#define Ar9287Eeprom2GHzSwitchSettling "2GHz.SwitchSettling"	
#define Ar9287Eeprom2GHztxRxAttenCh "2GHz.txRxAttenCh"	
#define Ar9287Eeprom2GHztxrxTxMarginCh "2GHz.rxTxMarginCh"	
#define Ar9287Eeprom2GHzAdcSize "2GHz.AdcSize"								// signed
#define Ar9287Eeprom2GHzTxEndToXpaOff "2GHz.TxEndToXpaOff"				
#define Ar9287Eeprom2GHzTxEndToRxOn "2GHz.TxEndToRxOn"				
#define Ar9287Eeprom2GHzTxFrameToXpaOn "2GHz.TxFrameToXpaOn"         
#define Ar9287Eeprom2GHzThresh62 "2GHz.Thresh62"         
#define Ar9287Eeprom2GHzPaPredistortionHt20 "2GHz.PaPredistortion.Ht20"      // 1 x 32   
	// define subfields?
#define Ar9287Eeprom2GHzPaPredistortionHt40 "2GHz.PaPredistortion.Ht40"      // 1 x 32   
	// define subfields?
#define Ar9287Eeprom2GHzWlanSpdtSwitchGlobalControl "2GHz.WlanSpdtSwitchGlobalControl"
#define Ar9287Eeprom2GHzXLNABiasStrength "2GHz.XLNABiasStrength"
#define Ar9287Eeprom2GHzRFGainCAP "2GHz.RFGainCap"
#define Ar9287Eeprom2GHzFuture "2GHz.Future"								// MAX_MODAL_FUTURE 


#define Ar9287EepromAntennaDiversityControl "AntennaDiversityControl"	
#define Ar9287Eeprom5GHzQuickDropLow "5GHz.QuickDrop.Low"						// signed
#define Ar9287Eeprom5GHzQuickDropHigh "5GHz.QuickDrop.High"						// signed
#define Ar9287EepromFuture "Future"											// MAX_BASE_EXTENSION_FUTURE 


#define Ar9287Eeprom2GHzCalibrationFrequency "2GHz.PowerCalibration.Frequency"	// OSPREY_NUM_2G_CAL_PIERS
#define Ar9287Eeprom2GHzCalibrationData "2GHz.Calibration.Data"				// [OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS][6]
#define Ar9287Eeprom2GHzPowerCorrection "2GHz.TransmitCalibration.PowerCorrection"				// signed  [OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS], interleave 6
#define Ar9287Eeprom2GHzCalibrationVoltage "2GHz.TransmitCalibration.Voltage"	// [OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS], interleave 6
#define Ar9287Eeprom2GHzCalibrationTemperature "2GHz.TransmitCalibration.Temperature"	// [OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS], interleave 6
#define Ar9287Eeprom2GHzCalibrationPcdac "2GHz.TransmitCalibration.Pcdac"	// [OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS], interleave 6

#define Ar9287Eeprom2GHzNoiseFloor "2GHz.ReceiveCalibration.NoiseFloor"						// signed  [OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS], interleave 6
#define Ar9287Eeprom2GHzNoiseFloorPower "2GHz.ReceiveCalibration.Power"				// signed  [OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS], interleave 6
#define Ar9287Eeprom2GHzNoiseFloorTemperature	"2GHz.ReceiveCalibration.Temperature"// [OSPREY_MAX_CHAINS][OSPREY_NUM_2G_CAL_PIERS], interleave 6

#define Ar9287Eeprom2GHzTargetDataCck "2GHz.Target.Data.Cck"		// OSPREY_NUM_2G_CCK_TARGET_POWERS
#define Ar9287Eeprom2GHzTargetFrequencyCck "2GHz.Target.Frequency.Cck"		// OSPREY_NUM_2G_CCK_TARGET_POWERS
#define Ar9287Eeprom2GHzTargetPowerCck "2GHz.Target.Power.Cck"				// OSPREY_NUM_2G_CCK_TARGET_POWERS

#define Ar9287Eeprom2GHzTargetData "2GHz.Target.Data.Legacy"				// OSPREY_NUM_2G_20_TARGET_POWERS
#define Ar9287Eeprom2GHzTargetFrequency "2GHz.Target.Frequency.Legacy"				// OSPREY_NUM_2G_20_TARGET_POWERS
#define Ar9287Eeprom2GHzTargetPower "2GHz.Target.Power.Legacy"						// OSPREY_NUM_2G_20_TARGET_POWERS

#define Ar9287Eeprom2GHzTargetDataHt20 "2GHz.Target.Data.Ht20"		// OSPREY_NUM_2G_20_TARGET_POWERS
#define Ar9287Eeprom2GHzTargetFrequencyHt20 "2GHz.Target.Frequency.Ht20"		// OSPREY_NUM_2G_20_TARGET_POWERS
#define Ar9287Eeprom2GHzTargetPowerHt20 "2GHz.Target.Power.Ht20"				// OSPREY_NUM_2G_20_TARGET_POWERS

#define Ar9287Eeprom2GHzTargetDataHt40 "2GHz.Target.Data.Ht40"				// OSPREY_NUM_2G_40_TARGET_POWERS
#define Ar9287Eeprom2GHzTargetFrequencyHt40 "2GHz.Target.Frequency.Ht40"		// OSPREY_NUM_2G_40_TARGET_POWERS
#define Ar9287Eeprom2GHzTargetPowerHt40 "2GHz.Target.Power.Ht40"				// OSPREY_NUM_2G_40_TARGET_POWERS

#define Ar9287Eeprom2GHzCtlIndex "2GHz.Ctl.Index"							// OSPREY_NUM_CTLS_2G
#define Ar9287Eeprom2GHzCtlData "2GHz.Ctl.Data"
#define Ar9287Eeprom2GHzCtlFrequency "2GHz.Ctl.Frequency"					// [OSPREY_NUM_CTLS_2G][OSPREY_NUM_BAND_EDGES_2G]
#define Ar9287Eeprom2GHzCtlPower "2GHz.Ctl.Power"							// [OSPREY_NUM_CTLS_2G][OSPREY_NUM_BAND_EDGES_2G]
#define Ar9287Eeprom2GHzCtlBandEdge "2GHz.Ctl.BandEdge"						// [OSPREY_NUM_CTLS_2G][OSPREY_NUM_BAND_EDGES_2G]
#define Ar9287Eeprom2GHzPadding "2GHz.Padding"

#define Ar9287Eeprom5GHzAntennaControlCommon "5GHz.Antenna.Common"	// 1 x 32    idle, t1, t2, b (4 bits per setting)
#define Ar9287Eeprom5GHzAntennaControlCommon2 "5GHz.Antenna.Common2"	// 4 x 16    ra1l1, ra2l1, ra1l2, ra2l2, ra12
#define Ar9287Eeprom5GHzAntennaControlChain "5GHz.Antenna.Chain"		// OSPREY_MAX_CHAINS x 16   6   idle, t, r, rx1, rx12, b (2 bits each)
#define Ar9287Eeprom5GHzAttenuationDb "5GHz.Attenuation.Db.Middle"					// OSPREY_MAX_CHAINS
#define Ar9287Eeprom5GHzAttenuationMargin "5GHz.Attenuation.Margin.Middle"			// OSPREY_MAX_CHAINS
#define Ar9287Eeprom5GHzTemperatureSlope "5GHz.TemperatureSlope.Middle"			// signed
#define Ar9287Eeprom5GHzTemperatureSlopeExtension "5GHz.TemperatureSlope.Extension"			

#define Ar9287Eeprom5GHzVoltageSlope "5GHz.VoltageSlope.Middle"					// signed
#define Ar9287Eeprom5GHzSpur "5GHz.Spur"									// OSPREY_MAX_CHAINS
#define Ar9287Eeprom5GHzNoiseFloorThreshold "5GHz.NoiseFloorThreshold"		// OSPREY_MAX_CHAINS   
#define Ar9287Eeprom5GHzReserved "5GHz.Reserved"							// MAX_MODAL_RESERVED
#define Ar9287Eeprom5GHzQuickDrop "5GHz.QuickDrop.Middle"							// signed
#define Ar9287Eeprom5GHzXpaBiasLevel "5GHz.XpaBiasLevel"         
#define Ar9287Eeprom5GHzTxFrameToDataStart "5GHz.TxFrameToDataStart"         
#define Ar9287Eeprom5GHzTxFrameToPaOn "5GHz.TxFrameToPaOn"         
#define Ar9287Eeprom5GHzTxClip "5GHz.TxClip"								// 4 bits tx_clip, 4 bits dac_scale_cck
#define Ar9287Eeprom5GHzAntennaGain "5GHz.AntennaGain"						// signed
#define Ar9287Eeprom5GHzSwitchSettling "5GHz.SwitchSettling"	
#define Ar9287Eeprom5GHzAdcSize "5GHz.AdcSize"								// signed
#define Ar9287Eeprom5GHzTxEndToXpaOff "5GHz.TxEndToXpaOff"				
#define Ar9287Eeprom5GHzTxEndToRxOn "5GHz.TxEndToRxOn"				
#define Ar9287Eeprom5GHzTxFrameToXpaOn "5GHz.TxFrameToXpaOn"         
#define Ar9287Eeprom5GHzThresh62 "5GHz.Thresh62"         
#define Ar9287Eeprom5GHzPaPredistortionHt20 "5GHz.PaPredistortion.Ht20"      // 1 x 32   
#define Ar9287Eeprom5GHzPaPredistortionHt40 "5GHz.PaPredistortion.Ht40"      // 1 x 32   
#define Ar9287Eeprom5GHzWlanSpdtSwitchGlobalControl "5GHz.WlanSpdtSwitchGlobalControl"
#define Ar9287Eeprom5GHzXLNABiasStrength "5GHz.XLNABiasStrength"
#define Ar9287Eeprom5GHzRFGainCAP "5GHz.RFGainCap"
#define Ar9287Eeprom5GHzFuture "5GHz.Future"								// MAX_MODAL_FUTURE 
#define Ar9287Eeprom5GHzTemperatureSlopeLow "5GHz.TemperatureSlope.Low"		// signed
#define Ar9287Eeprom5GHzTemperatureSlopeHigh "5GHz.TemperatureSlope.High"	// signed
#define Ar9287Eeprom5GHzAttenuationDbLow "5GHz.Attenuation.Db.Low"			// OSPREY_MAX_CHAINS x 8
#define Ar9287Eeprom5GHzAttenuationDbHigh "5GHz.Attenuation.Db.High"			// OSPREY_MAX_CHAINS x 8
#define Ar9287Eeprom5GHzAttenuationMarginLow "5GHz.Attenuation.Margin.Low"	// OSPREY_MAX_CHAINS x 8
#define Ar9287Eeprom5GHzAttenuationMarginHigh "5GHz.Attenuation.Margin.High"	// OSPREY_MAX_CHAINS x 8

#define Ar9287Eeprom5GHzCalibrationFrequency "5GHz.TransmitCalibration.Frequency"	// OSPREY_NUM_5G_CAL_PIERS
#define Ar9287Eeprom5GHzCalibrationData "5GHz.Calibration.Data"				// [OSPREY_MAX_CHAINS][OSPREY_NUM_5G_CAL_PIERS][6]
#define Ar9287Eeprom5GHzPowerCorrection "5GHz.TransmitCalibration.PowerCorrection"				// [OSPREY_MAX_CHAINS][OSPREY_NUM_5G_CAL_PIERS]
#define Ar9287Eeprom5GHzCalibrationVoltage "5GHz.TransmitCalibration.Voltage"		// [OSPREY_MAX_CHAINS][OSPREY_NUM_5G_CAL_PIERS]
#define Ar9287Eeprom5GHzCalibrationTemperature "5GHz.TransmitCalibration.Temperature"// [OSPREY_MAX_CHAINS][OSPREY_NUM_5G_CAL_PIERS]
#define Ar9287Eeprom5GHzNoiseFloor "5GHz.ReceiveCalibration.NoiseFloor"						// [OSPREY_MAX_CHAINS][OSPREY_NUM_5G_CAL_PIERS]
#define Ar9287Eeprom5GHzNoiseFloorPower "5GHz.ReceiveCalibration.Power"				// [OSPREY_MAX_CHAINS][OSPREY_NUM_5G_CAL_PIERS]
#define Ar9287Eeprom5GHzNoiseFloorTemperature "5GHz.ReceiveCalibration.Temperature"	// [OSPREY_MAX_CHAINS][OSPREY_NUM_5G_CAL_PIERS]

#define Ar9287Eeprom5GHzTargetFrequency "5GHz.Target.Frequency.Legacy"				// OSPREY_NUM_5G_20_TARGET_POWERS
#define Ar9287Eeprom5GHzTargetFrequencyHt20 "5GHz.Target.Frequency.Ht20"		// OSPREY_NUM_5G_20_TARGET_POWERS
#define Ar9287Eeprom5GHzTargetFrequencyHt40 "5GHz.Target.Frequency.Ht40"		// OSPREY_NUM_5G_40_TARGET_POWERS

#define Ar9287Eeprom5GHzTargetPower "5GHz.Target.Power.Legacy"						// OSPREY_NUM_5G_20_TARGET_POWERS
#define Ar9287Eeprom5GHzTargetPowerHt20 "5GHz.Target.Power.Ht20"				// OSPREY_NUM_5G_20_TARGET_POWERS
#define Ar9287Eeprom5GHzTargetPowerHt40 "5GHz.Target.Power.Ht40"				// OSPREY_NUM_5G_40_TARGET_POWERS
#define Ar9287Eeprom5GHzCtlIndex "5GHz.Ctl.Index"							// OSPREY_NUM_CTLS_5G
#define Ar9287Eeprom5GHzCtlFrequency "5GHz.Ctl.Frequency"					// [OSPREY_NUM_CTLS_5G][OSPREY_NUM_BAND_EDGES_5G]
#define Ar9287Eeprom5GHzCtlPower "5GHz.Ctl.Power"							// [OSPREY_NUM_CTLS_5G][OSPREY_NUM_BAND_EDGES_5G]
#define Ar9287Eeprom5GHzCtlBandEdge "5GHz.Ctl.BandEdge"						// [OSPREY_NUM_CTLS_5G][OSPREY_NUM_BAND_EDGES_5G]
#define Ar9287CaldataMemoryType "caldata.memory"						// calibration data storage type


