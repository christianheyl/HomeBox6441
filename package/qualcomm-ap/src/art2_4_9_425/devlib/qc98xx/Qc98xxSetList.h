
#ifndef _QC98XX_SET_LIST_H_
#define _QC98XX_SET_LIST_H_

#include "DevSetList.h"
#include "Qc9KEepromParameter.h"
#include "Qc98xxEepromStruct.h"

static int Qc98xxEepromVersionMinimum=2;		// check this
static int Qc98xxEepromVersionDefault=2;

static int Qc98xxTemplateVersionMinimum=2;		// get from eep.h
static int Qc98xxTemplateVersionDefault=2;
//want template list

static int Qc98xxChainMaskMinimum=1;
static int Qc98xxChainMaskMaximum=7;
static int Qc98xxChainMaskDefault=7;

#define QC98XX_SET_EEPROM_ALL {Qc9KSetEepromALL,{Qc9KEepromALL,0,0},"formatted display of all configuration and calibration data",'t',0,1,1,1,\
    0,0,0,0,0}

#define QC98XX_SET_EEPROM_ALL_FIRMWARE {Qc9KSetEepromAllFirmware,{Qc9KEepromAllFirmware,0,0},"formatted display of all configuration and calibration data from Firmware/DUT",'t',0,1,1,1,\
    0,0,0,0,0}

// PCIE config
#define QC98XX_SET_CONFIG {Qc9KSetEepromConfig,{Qc9KEepromConfig,0,0},"pcie configuration space",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define QC98XX_SET_CONFIG_PCIE {Qc9KSetEepromConfigPCIe,{Qc9KEepromConfigPCIe,0,0},"pcie configuration space",'x',0,1,1,1,\
	&UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define QC98XX_SET_EEPROM_DEVICEID {Qc9KSetEepromDeviceId,{Qc9KEepromDeviceId,"devid", 0},"the device id",'x',0,1,1,1,\
    &UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define QC98XX_SET_EEPROM_SSID {Qc9KSetEepromSSID,{Qc9KEepromSubSystemId,"SubSystemId",0},"the subsystem id",'x',0,1,1,1,\
    &UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define QC98XX_SET_EEPROM_VID {Qc9KSetEepromVID,{Qc9KEepromVID,"vendorId",0},"the vendor id",'x',0,1,1,1,\
    &UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define QC98XX_SET_EEPROM_SVID {Qc9KSetEepromSVID,{Qc9KEepromSVID,"subVendorId",0},"the subvendor id",'x',0,1,1,1,\
    &UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

// Base Header
#define QC98XX_SET_EEPROM_VERSION {Qc9KSetEepromVersion,{Qc9KEepromVersion,"eepversion",0},"the calibration structure version number",'u',0,1,1,1,\
    &Qc98xxEepromVersionMinimum,&UnsignedByteMaximum,&Qc98xxEepromVersionDefault,0,0}

#define QC98XX_SET_TEMPLATE_VERSION {Qc9KSetEepromTemplateVersion,{Qc9KEepromTemplateVersion,0,0},"the template number",'u',0,1,1,1,\
    &Qc98xxTemplateVersionMinimum,&UnsignedByteMaximum,&Qc98xxTemplateVersionDefault,0,0}

#define QC98XX_SET_MAC_ADDRESS {Qc9KSetEepromMacAddress,{Qc9KEepromMacAddress,"mac",0},"the mac address of the device",'m',0,1,1,1,\
    0,0,0,0,0}

#define QC98XX_SET_REGULATORY_DOMAIN {Qc9KSetEepromRegulatoryDomain,{Qc9KEepromRegulatoryDomain,"regDmn",0},"the regulatory domain",'x',0,2,1,1,\
    &UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}			// want list of allowed values

#define QC98XX_SET_OP_FLAGS {Qc9KSetEepromOpFlags,{Qc9KEepromOpFlags,"opFlags",0},"flags that control operating modes",'x',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_OP_FLAGS2 {Qc9KSetEepromOpFlags2,{Qc9KEepromOpFlags2,"opFlags2",0},"flags that control VHT operating modes",'x',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_BOARD_FLAGS {Qc9KSetEepromBoardFlags, {Qc9KEepromBoardFlags,"boardFlags",0},"board flags",'x',0,1,1,1,\
    &UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define QC98XX_SET_BOARD_FLAGS_ENABLE_RBIAS {Qc9KSetEepromEnableRbias,{Qc9KEepromEnableRbias,0,0},"enables rbias",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define QC98XX_SET_FEATURE_ENABLE_PA_PREDISTORTION {Qc9KSetEepromFeatureEnablePaPredistortion,{Qc9KEeprom2GHzEnablePaPredistortion,"papdenable",0},"enables pa predistortion",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define QC98XX_SET_BLUETOOTH_OPTIONS {Qc9KSetEepromBlueToothOptions,{Qc9KEepromBlueToothOptions,0,0},"bluetooth options",'x',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_FEATURE_ENABLE {Qc9KSetEepromFeatureEnable,{Qc9KEepromFeatureEnable,"featureEnable",0},"feature enable control word",'x',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_FEATURE_ENABLE_TEMPERATURE_COMPENSATION {Qc9KSetEepromFeatureEnableTemperatureCompensation,{Qc9KEepromFeatureEnableTemperatureCompensation,"TemperatureCompensationEnable","TempCompEnable"},"enables temperature compensation on transmit power control",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define QC98XX_SET_FEATURE_ENABLE_VOLTAGE_COMPENSATION {Qc9KSetEepromFeatureEnableVoltageCompensation,{Qc9KEepromFeatureEnableVoltageCompensation,"VoltageCompensationEnable","VoltCompEnable"},"enables voltage compensation on transmit power control",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define QC98XX_SET_FEATURE_ENABLE_FAST_CLOCK {Qc9KSetEepromFeatureEnableFastClock,{Qc9KEepromFeatureEnableFastClock,"FastClockEnable",0},"enables fast clock mode",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define QC98XX_SET_FEATURE_ENABLE_DOUBLING {Qc9KSetEepromFeatureEnableDoubling,{Qc9KEepromFeatureEnableDoubling,"DoublingEnable",0},"enables doubling mode",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define QC98XX_SET_FEATURE_ENABLE_INTERNAL_SWITCHING_REGULATOR {Qc9KSetEepromFeatureEnableInternalSwitchingRegulator,{Qc9KEepromFeatureEnableInternalSwitchingRegulator,"swregenable",0},"enables the internal switching regulator",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define QC98XX_SET_FEATURE_ENABLE_TUNING_CAPS {Qc9KSetEepromFeatureEnableTuningCaps,{Qc9KEepromFeatureEnableTuningCaps,"TuningCapsEnable",0},"enables use of tuning capacitors",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define QC98XX_SET_MISCELLANEOUS {Qc9KSetEepromMiscellaneous,{Qc9KEepromMiscellaneous,0,0},"miscellaneous parameters",'x',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_MISCELLANEOUS_DRIVERS_STRENGTH {Qc9KSetEepromMiscellaneousDriveStrength,{Qc9KEepromMiscellaneousDriveStrength,"DriveStrengthReconfigure","DriveStrength"},"enables drive strength reconfiguration",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define QC98XX_SET_MISCELLANEOUS_THERMOMETER {Qc9KSetEepromMiscellaneousThermometer,{Qc9KEepromMiscellaneousThermometer,"Thermometer",0},"forces use of the specified chip thermometer",'d',0,1,1,1,\
    &ThermometerMinimum,&ThermometerMaximum,&ThermometerDefault,0,0}

#define QC98XX_SET_MISCELLANEOUS_CHAIN_MASK_REDUCE {Qc9KSetEepromMiscellaneousChainMaskReduce,{Qc9KEepromMiscellaneousChainMaskReduce,"ChainMaskReduce",0},"enables dynamic 2x3 mode to reduce power draw",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define QC98XX_SET_MISCELLANEOUS_QUICK_DROP_ENABLE {Qc9KSetEepromMiscellaneousQuickDropEnable,{Qc9KEepromMiscellaneousQuickDropEnable,"quickDrop",0},"enables quick drop mode for improved strong signal response",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define QC98XX_SET_MISCELLANEOUS_EEP {Qc9KSetEepromMiscellaneousEep,{Qc9KEepromMiscellaneousEep,"eepMisc",0},"some miscellaneous control flags",'x',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_BOARD_FLAG1_BIBXOSC0 {Qc9KSetEepromBibxosc0,{Qc9KEepromBibxosc0,0,0},"the bias current for XTALOSC driver control",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define QC98XX_SET_BOARD_FLAG1_NOISE_FLR_THR {Qc9KSetEepromFlag1NoiseFlrThr,{Qc9KEepromFlag1NoiseFlrThr,0,0},"Noise floor threshold control",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define QC98XX_SET_TX_RX_MASK {Qc9KSetEepromTxRxMask,{Qc9KEepromTxRxMask,"txrxMask",0},"the transmit and receive chain masks",'x',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_TX_RX_MASK_TX {Qc9KSetEepromTxRxMaskTx,{Qc9KEepromTxRxMaskTx,"TxMask",0},"the maximum chain mask used for transmit",'x',0,1,1,1,\
    &Qc98xxChainMaskMinimum,&Qc98xxChainMaskMaximum,&Qc98xxChainMaskDefault,0,0}

#define QC98XX_SET_TX_RX_MASK_RX {Qc9KSetEepromTxRxMaskRx,{Qc9KEepromTxRxMaskRx,"RxMask",0},"the maximum chain mask used for receive",'x',0,1,1,1,\
    &Qc98xxChainMaskMinimum,&Qc98xxChainMaskMaximum,&Qc98xxChainMaskDefault,0,0}

#define QC98XX_SET_RF_SILENT {Qc9KSetEepromRfSilent,{Qc9KEepromRfSilent,0,0},"rf silent mode control word",'x',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_RF_SILENT_B0 {Qc9KSetEepromRfSilentB0,{Qc9KEepromRfSilentB0,"rfSilentB0",0},"implement rf silent mode in hardware",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define QC98XX_SET_RF_SILENT_B1 {Qc9KSetEepromRfSilentB1,{Qc9KEepromRfSilentB1,"rfSilentB1",0},"polarity of the rf silent control line",'z',0,1,1,1,\
    0,0,0,sizeof(LogicalParameter)/sizeof(LogicalParameter[0]),LogicalParameter}

#define QC98XX_SET_RF_SILENT_GPIO {Qc9KSetEepromRfSilentGpio,{Qc9KEepromRfSilentGpio,"rfSilentGpio",0},"the chip gpio line used for rf silent",'x',0,1,1,1,\
    &SixBitsMinimum,&SixBitsMaximum,&SixBitsDefault,0,0}

#define QC98XX_SET_WLAN_LED_GPIO {Qc9KSetEepromWlanLedGpio,{Qc9KEepromWlanLedGpio,0,0},"",'x',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_DEVICE_TYPE {Qc9KSetEepromDeviceType,{Qc9KEepromDeviceType,"DeviceType",0},"devicetype",'x',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_CTL_VERSION {Qc9KSetEepromCtlVersion, {Qc9KEepromCtlVersion,"ctlVersion",0},"ctl version",'u',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_SPUR_BASE_A {Qc9KSetEepromSpurBaseA, {Qc9KEepromSpurBaseA,"SpurBaseA",0}," spur base A",'u',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_SPUR_BASE_B {Qc9KSetEepromSpurBaseB, {Qc9KEepromSpurBaseB,"SpurBaseB",0}," spur base B",'u',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_SPUR_RSSI_THRESH {Qc9KSetEepromSpurRssiThresh, {Qc9KEepromSpurRssiThresh,"SpurRssiThresh",0},"spur rssi thresh",'u',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_SPUR_RSSI_THRESH_CCK {Qc9KSetEepromSpurRssiThreshCck, {Qc9KEepromSpurRssiThreshCck,"SpurRssiThreshCck",0},"spur rssi thresh CCK",'u',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_SPUR_MIT_FLAG {Qc9KSetEepromSpurMitFlag, {Qc9KEepromSpurMitFlag,"SpurMitFlag",0},"spur mit flag",'u',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_SWITCHING_REGULATOR {Qc9KSetEepromSwreg,{Qc9KEepromSwreg,"SWREG","internalregulator"},"the internal switching regulator control word",'x',0,1,1,1,\
    &UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define QC98XX_SET_SWITCHING_REGULATOR_PROGRAM {Qc9KSetEepromSwregProgram,{Qc9KEepromSwregProgram,"SWREGProgram",0},"set the internal switching regulator control program",'u',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_TX_RX_GAIN {Qc9KSetEepromTxRxGain,{Qc9KEepromTxRxGain,0,0},"transmit and receive gain table control word",'x',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_TX_RX_GAIN_TX {Qc9KSetEepromTxRxGainTx,{Qc9KEepromTxRxGainTx,"TxGain","TxGainTable"},"transmit gain table used",'x',0,1,1,1,\
    &HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define QC98XX_SET_TX_RX_GAIN_RX {Qc9KSetEepromTxRxGainRx,{Qc9KEepromTxRxGainRx,"RxGain","RxGainTable"},"receive gain table used",'x',0,1,1,1,\
    &HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define QC98XX_SET_POWER_TABLE_OFFSET {Qc9KSetEepromPowerTableOffset,{Qc9KEepromPowerTableOffset,"PwrTableOffset",0},"power level of the first entry in the power table",'d',"dBm",1,1,1,\
    &PowerOffsetMinimum,&PowerOffsetMaximum,&PowerOffsetDefault,0,0}

#define QC98XX_SET_TUNING_CAPS {Qc9KSetEepromTuningCaps,{Qc9KEepromTuningCaps,0,0},"capacitors for tuning frequency accuracy",'x',0,2,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_DELTA_CCK_20 {Qc9KSetEepromDeltaCck20, {Qc9KEepromDeltaCck20,"DeltaCck20",0},"delta CCK-HT20",'d',0,1,1,1,\
    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_DELTA_40_20 {Qc9KSetEepromDelta4020, {Qc9KEepromDelta4020,"Delta4020",0},"delta HT40-HT20",'d',0,1,1,1,\
    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_DELTA_80_20 {Qc9KSetEepromDelta8020, {Qc9KEepromDelta8020,"Delta80200",0},"delta HT80-HT20",'d',0,1,1,1,\
    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_CUSTOMER_DATA {Qc9KSetEepromCustomerData,{Qc9KEepromCustomerData,"customerData",0},"any text, usually used for device serial number",'t',0,1,1,1,\
    0,0,0,0,0}

// 2GHz Bimodal Header

#define QC98XX_SET_2GHZ_VOLTAGE_SLOPE {Qc9KSetEeprom2GHzVoltageSlope,{Qc9KEeprom2GHzVoltageSlope,"voltSlope2g","VoltageSlope2g"},"slope used in voltage compensation algorithm",'d',0,WHAL_NUM_CHAINS,1,1,\
    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_SPUR {Qc9KSetEeprom2GHzSpur,{Qc9KEeprom2GHzSpur,"SpurChans2g",0},"spur frequencies",'u',"MHz",QC98XX_EEPROM_MODAL_SPURS,1,1,\
    &FreqZeroMinimum,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}
#define QC98XX_SET_2GHZ_SPURA {Qc9KSetEeprom2GHzSpurAPrimSecChoose,{Qc9KEeprom2GHzSpurAPrimSecChoose,"SpurAPrimSecChoose2g",0},"spurA frequencies",'u',"MHz",QC98XX_EEPROM_MODAL_SPURS,1,1,\
    &FreqZeroMinimum,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}
#define QC98XX_SET_2GHZ_SPURB {Qc9KSetEeprom2GHzSpurBPrimSecChoose,{Qc9KEeprom2GHzSpurBPrimSecChoose,"SpurBPrimSecChoose2g",0},"spurB frequencies",'u',"MHz",QC98XX_EEPROM_MODAL_SPURS,1,1,\
    &FreqZeroMinimum,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

	//#define Qc9KEeprom2GHzSpurAPrimSecChoose "2GHz.SpurAPrimSecChoose"
	//#define Qc9KEeprom2GHzSpurBPrimSecChoose "2GHz.SpurBPrimSecChoose"

//#define QC98XX_SET_2GHZ_NOISE_FLOOR_THRESHOLD {Qc9KSetEeprom2GHzNoiseFloorThreshold,{Qc9KEeprom2GHzNoiseFloorThreshold,"NoiseFloorThreshCh2g",0},"noise floor threshold",'d',0,WHAL_NUM_CHAINS,1,1,\
//    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_XPA_BIAS_LEVEL {Qc9KSetEeprom2GHzXpaBiasLevel,{Qc9KEeprom2GHzXpaBiasLevel,"XpaBiasLvl2g",0},"external pa bias level",'x',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_XPA_BIAS_BYPASS {Qc9KSetEeprom2GHzXpaBiasBypass,{Qc9KEeprom2GHzXpaBiasBypass,"XpaBiasBypass2g",0},"external pa bias bypass",'x',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_ANTENNA_GAIN {Qc9KSetEeprom2GHzAntennaGain,{Qc9KEeprom2GHzAntennaGain,"AntennaGain2g","AntGain2g"},"",'d',0,WHAL_NUM_CHAINS,1,1,\
    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_ANTENNA_CONTROL_COMMON {Qc9KSetEeprom2GHzAntennaControlCommon,{Qc9KEeprom2GHzAntennaControlCommon,"AntCtrlCommon2g",0},"antenna switch control word 1",'x',0,1,1,1,\
    &UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define QC98XX_SET_2GHZ_ANTENNA_CONTROL_COMMON_2 {Qc9KSetEeprom2GHzAntennaControlCommon2,{Qc9KEeprom2GHzAntennaControlCommon2,"AntCtrlCommon22g",0},"antenna switch control word 2",'x',0,1,1,1,\
    &UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define QC98XX_SET_2GHZ_ANTENNA_CONTROL_CHAIN {Qc9KSetEeprom2GHzAntennaControlChain,{Qc9KEeprom2GHzAntennaControlChain,"antCtrlChain2g",0},"per chain antenna switch control word",'x',0,WHAL_NUM_CHAINS,1,1,\
    &UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define QC98XX_SET_2GHZ_RX_FILTER_CAP {Qc9KSetEeprom2GHzRxFilterCap, {Qc9KEeprom2GHzRxFilterCap,"RxFilterCap2g",0},"rx filter cap",'u',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_RX_GAIN_CAP {Qc9KSetEeprom2GHzRxGainCap, {Qc9KEeprom2GHzRxGainCap,"RxGainCap2g",0},"rx gain cap",'u',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_TX_RX_GAIN_TX {Qc9KSetEeprom2GHzTxRxGainTx,{Qc9KEeprom2GHzTxRxGainTx,0,0},"transmit gain table used",'x',0,1,1,1,\
    &HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define QC98XX_SET_2GHZ_TX_RX_GAIN_RX {Qc9KSetEeprom2GHzTxRxGainRx,{Qc9KEeprom2GHzTxRxGainRx,0,0},"receive gain table used",'x',0,1,1,1,\
    &HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define QC98XX_SET_2GHZ_NOISE_FLR_THR {Qc9KSetEeprom2GHzNoiseFlrThr,{Qc9KEeprom2GHzNoiseFlrThr,"noiseFlrThr2g",0},"Noise Floor Threshold used",'d',0,1,1,1,\
    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_MIN_CCA_PWR_CHAIN {Qc9KSetEeprom2GHzMinCcaPwrChain,{Qc9KEeprom2GHzMinCcaPwrChain,"minCcaPwrChain2g",0},"per chain minimum cca power word",'d',0,WHAL_NUM_CHAINS,1,1,\
    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

// 5GHz Bimodal Header

#define QC98XX_SET_5GHZ_VOLTAGE_SLOPE {Qc9KSetEeprom5GHzVoltageSlope,{Qc9KEeprom5GHzVoltageSlope,"voltSlope5g","VoltageSlope5g"},"slope used in voltage compensation algorithm",'d',0,WHAL_NUM_CHAINS,1,1,\
    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_SPUR {Qc9KSetEeprom5GHzSpur,{Qc9KEeprom5GHzSpur,"SpurChans5g",0},"spur frequencies",'u',"MHz",QC98XX_EEPROM_MODAL_SPURS,1,1,\
    &FreqZeroMinimum,&FrequencyMaximum5GHz,&FrequencyDefault5GHz,0,0}
#define QC98XX_SET_5GHZ_SPURA {Qc9KSetEeprom5GHzSpurAPrimSecChoose,{Qc9KEeprom5GHzSpurAPrimSecChoose,"SpurAPrimSecChoose5g",0},"spurA frequencies",'u',"MHz",QC98XX_EEPROM_MODAL_SPURS,1,1,\
    &FreqZeroMinimum,&FrequencyMaximum5GHz,&FrequencyDefault5GHz,0,0}
#define QC98XX_SET_5GHZ_SPURB {Qc9KSetEeprom5GHzSpurBPrimSecChoose,{Qc9KEeprom5GHzSpurBPrimSecChoose,"SpurBPrimSecChoose5g",0},"spurB frequencies",'u',"MHz",QC98XX_EEPROM_MODAL_SPURS,1,1,\
    &FreqZeroMinimum,&FrequencyMaximum5GHz,&FrequencyDefault5GHz,0,0}

//#define QC98XX_SET_5GHZ_NOISE_FLOOR_THRESHOLD {Qc9KSetEeprom5GHzNoiseFloorThreshold,{Qc9KEeprom5GHzNoiseFloorThreshold,"NoiseFloorThreshCh5g",0},"noise floor threshold",'d',0,WHAL_NUM_CHAINS,1,1,\
//    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_XPA_BIAS_LEVEL {Qc9KSetEeprom5GHzXpaBiasLevel,{Qc9KEeprom5GHzXpaBiasLevel,"XpaBiasLvl5g",0},"external pa bias level",'x',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_XPA_BIAS_BYPASS {Qc9KSetEeprom5GHzXpaBiasBypass,{Qc9KEeprom5GHzXpaBiasBypass,"XpaBiasBypass5g",0},"external pa bias bypass",'x',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_ANTENNA_GAIN {Qc9KSetEeprom5GHzAntennaGain,{Qc9KEeprom5GHzAntennaGain,"AntennaGain5g","AntGain5g"},"",'d',0,WHAL_NUM_CHAINS,1,1,\
    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_ANTENNA_CONTROL_COMMON {Qc9KSetEeprom5GHzAntennaControlCommon,{Qc9KEeprom5GHzAntennaControlCommon,"AntCtrlCommon5g",0},"antenna switch control word 1",'x',0,1,1,1,\
    &UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define QC98XX_SET_5GHZ_ANTENNA_CONTROL_COMMON_2 {Qc9KSetEeprom5GHzAntennaControlCommon2,{Qc9KEeprom5GHzAntennaControlCommon2,"AntCtrlCommon25g",0},"antenna switch control word 2",'x',0,1,1,1,\
    &UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}

#define QC98XX_SET_5GHZ_ANTENNA_CONTROL_CHAIN {Qc9KSetEeprom5GHzAntennaControlChain,{Qc9KEeprom5GHzAntennaControlChain,"antCtrlChain5g",0},"per chain antenna switch control word",'x',0,WHAL_NUM_CHAINS,1,1,\
    &UnsignedShortMinimum,&UnsignedShortMaximum,&UnsignedShortDefault,0,0}

#define QC98XX_SET_5GHZ_RX_FILTER_CAP {Qc9KSetEeprom5GHzRxFilterCap, {Qc9KEeprom5GHzRxFilterCap,"RxFilterCap5g",0},"rx filter cap",'u',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_RX_GAIN_CAP {Qc9KSetEeprom5GHzRxGainCap, {Qc9KEeprom5GHzRxGainCap,"RxGainCap5g",0},"rx gain cap",'u',0,1,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_TX_RX_GAIN_TX {Qc9KSetEeprom5GHzTxRxGainTx,{Qc9KEeprom5GHzTxRxGainTx,0,0},"transmit gain table used",'x',0,1,1,1,\
    &HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define QC98XX_SET_5GHZ_TX_RX_GAIN_RX {Qc9KSetEeprom5GHzTxRxGainRx,{Qc9KEeprom5GHzTxRxGainRx,0,0},"receive gain table used",'x',0,1,1,1,\
    &HalfByteMinimum,&HalfByteMaximum,&HalfByteDefault,0,0}

#define QC98XX_SET_5GHZ_NOISE_FLR_THR {Qc9KSetEeprom5GHzNoiseFlrThr,{Qc9KEeprom5GHzNoiseFlrThr,"noiseFlrThr5g",0},"noise floor threshold used",'d',0,1,1,1,\
    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_MIN_CCA_PWR_CHAIN {Qc9KSetEeprom5GHzMinCcaPwrChain,{Qc9KEeprom5GHzMinCcaPwrChain,"minCcaPwrChain5g",0},"per chain minimum cca power ",'d',0,WHAL_NUM_CHAINS,1,1,\
    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

// FreqModal Header
#define QC98XX_SET_2GHZ_XATTEN1_DB {Qc9KSetEeprom2GHzXatten1Db, {Qc9KEeprom2GHzXatten1Db,"Xatten1Db2g", 0},"2g attenuation1 db",'x',0,WHAL_NUM_CHAINS,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_XATTEN1_DB_LOW {Qc9KSetEeprom5GHzXatten1DbLow, {Qc9KEeprom5GHzXatten1DbLow,"Xatten1DbLow5g", 0},"5g attenuation1 db low",'x',0,WHAL_NUM_CHAINS,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_XATTEN1_DB_MID {Qc9KSetEeprom5GHzXatten1DbMid, {Qc9KEeprom5GHzXatten1DbMid,"Xatten1DbMid5g", 0},"5g attenuation1 db mid",'x',0,WHAL_NUM_CHAINS,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_XATTEN1_DB_HIGH {Qc9KSetEeprom5GHzXatten1DbHigh, {Qc9KEeprom5GHzXatten1DbHigh,"Xatten1DbHigh5g", 0},"5g attenuation1 db high",'x',0,WHAL_NUM_CHAINS,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_XATTEN1_MARGIN {Qc9KSetEeprom2GHzXatten1Margin, {Qc9KEeprom2GHzXatten1Margin,"Xatten1Margin2g", 0},"2g attenuation1 Margin",'x',0,WHAL_NUM_CHAINS,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_XATTEN1_MARGIN_LOW {Qc9KSetEeprom5GHzXatten1MarginLow, {Qc9KEeprom5GHzXatten1MarginLow,"Xatten1MarginLow5g", 0},"5g attenuation1 Margin low",'x',0,WHAL_NUM_CHAINS,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_XATTEN1_MARGIN_MID {Qc9KSetEeprom5GHzXatten1MarginMid, {Qc9KEeprom5GHzXatten1MarginMid,"Xatten1MarginMid5g", 0},"5g attenuation1 Margin mid",'x',0,WHAL_NUM_CHAINS,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_XATTEN1_MARGIN_HIGH {Qc9KSetEeprom5GHzXatten1MarginHigh, {Qc9KEeprom5GHzXatten1MarginHigh,"Xatten1MarginHigh5g", 0},"5g attenuation1 Margin high",'x',0,WHAL_NUM_CHAINS,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

// Chip Cal Data
#define QC98XX_SET_THERM_ADC_SCALED_GAIN {Qc9KSetEepromThermAdcScaledGain, {Qc9KEepromThermAdcScaledGain,0,0},"",'d',0,1,1,1,\
    &SignedShortMinimum,&SignedShortMaximum,&SignedShortDefault,0,0}

#define QC98XX_SET_THERM_ADC_OFFSET {Qc9KSetEepromThermAdcOffset, {Qc9KEepromThermAdcOffset,0,0},"",'d',0,1,1,1,\
    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

// 2GHz Cal Info
#define QC98XX_SET_2GHZ_CALIBRATION_FREQUENCY {Qc9KSetEeprom2GHzCalibrationFrequency,{Qc9KEeprom2GHzCalibrationFrequency,"calPierFreq2g",0},"frequencies at which calibration is performed",'u',"MHz",WHAL_NUM_11G_CAL_PIERS,1,1,\
    &FrequencyMinimum2GHz,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

//#define Qc9KEeprom2GHzCalPoint "2GHz.TxCalibration.CalPoint"
#define QC98XX_SET_2GHZ_CALPOINT_TXGAIN_INDEX {Qc9KSetEeprom2GHzCalPointTxGainIdx, {Qc9KEeprom2GHzCalPointTxGainIdx,0,0},"txgain index",'u',0,WHAL_NUM_11G_CAL_PIERS,WHAL_NUM_CHAINS,WHAL_NUM_CAL_GAINS,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_CALPOINT_DAC_GAIN {Qc9KSetEeprom2GHzCalPointDacGain, {Qc9KEeprom2GHzCalPointDacGain,0,0},"dac gain",'d',0,WHAL_NUM_11G_CAL_PIERS,WHAL_NUM_CAL_GAINS,1,\
    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_CALPOINT_POWER {Qc9KSetEeprom2GHzCalPointPower, {Qc9KEeprom2GHzCalPointPower,0,0},"power",'f',"dBm",WHAL_NUM_11G_CAL_PIERS,WHAL_NUM_CHAINS,WHAL_NUM_CAL_GAINS,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define QC98XX_SET_2GHZ_CALIBRATION_TEMPERATURE {Qc9KSetEeprom2GHzCalibrationTemperature,{Qc9KEeprom2GHzCalibrationTemperature,"CalPierTempMeas2g",0},"temperature measured during transmit power calibration",'u',0,WHAL_NUM_11G_CAL_PIERS,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_CALIBRATION_VOLTAGE {Qc9KSetEeprom2GHzCalibrationVoltage,{Qc9KEeprom2GHzCalibrationVoltage,"CalPierVoltMeas2g",0},"voltage measured during transmit power calibration",'u',0,WHAL_NUM_11G_CAL_PIERS,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_TARGET_FREQUENCY_CCK {Qc9KSetEeprom2GHzTargetFrequencyCck,{Qc9KEeprom2GHzTargetFrequencyCck,"calTGTFreqcck",0},"frequencies at which target powers for cck rates are specified",'u',"MHz",WHAL_NUM_11B_TARGET_POWER_CHANS,1,1,\
    &FrequencyMinimum2GHz,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define QC98XX_SET_2GHZ_TARGET_FREQUENCY {Qc9KSetEeprom2GHzTargetFrequency,{Qc9KEeprom2GHzTargetFrequency,"calTGTFreq2g",0},"frequencies at which target powers for legacy rates are specified",'u',"MHz",WHAL_NUM_11G_20_TARGET_POWER_CHANS,1,1,\
    &FrequencyMinimum2GHz,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define QC98XX_SET_2GHZ_TARGET_FREQUENCY_VHT20 {Qc9KSetEeprom2GHzTargetFrequencyVht20,{Qc9KEeprom2GHzTargetFrequencyVht20,"calTGTFreqvht202g",0},"frequencies at which target powers for ht20 rates are specified",'u',"MHz",WHAL_NUM_11G_20_TARGET_POWER_CHANS,1,1,\
    &FrequencyMinimum2GHz,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define QC98XX_SET_2GHZ_TARGET_FREQUENCY_VHT40 {Qc9KSetEeprom2GHzTargetFrequencyVht40,{Qc9KEeprom2GHzTargetFrequencyVht40,"calTGTFreqvht402g",0},"frequencies at which target powers for ht40 rates are specified",'u',"MHz",WHAL_NUM_11G_40_TARGET_POWER_CHANS,1,1,\
    &FrequencyMinimum2GHz,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define QC98XX_SET_2GHZ_TARGET_POWER_CCK {Qc9KSetEeprom2GHzTargetPowerCck,{Qc9KEeprom2GHzTargetPowerCck,"calTGTpwrCCK",0},"target powers for cck rates",'f',"dBm",WHAL_NUM_11B_TARGET_POWER_CHANS,WHAL_NUM_11B_TARGET_POWER_RATES,1,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define QC98XX_SET_2GHZ_TARGET_POWER {Qc9KSetEeprom2GHzTargetPower,{Qc9KEeprom2GHzTargetPower,"calTGTpwr2g",0},"target powers for legacy rates",'f',"dBm",WHAL_NUM_11G_20_TARGET_POWER_CHANS,WHAL_NUM_LEGACY_TARGET_POWER_RATES,1,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define QC98XX_SET_2GHZ_TARGET_POWER_VHT20 {Qc9KSetEeprom2GHzTargetPowerVht20,{Qc9KEeprom2GHzTargetPowerVht20,"calTGTpwrvht202g",0},"target powers for ht20 rates",'f',"dBm",WHAL_NUM_11G_20_TARGET_POWER_CHANS,VHT_TARGET_RATE_LAST,1,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define QC98XX_SET_2GHZ_TARGET_POWER_VHT40 {Qc9KSetEeprom2GHzTargetPowerVht40,{Qc9KEeprom2GHzTargetPowerVht40,"calTGTpwrvht402g",0},"target powers for ht40 rates",'f',"dBm",WHAL_NUM_11G_40_TARGET_POWER_CHANS,VHT_TARGET_RATE_LAST,1,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

// 2GHz Ctl
#define QC98XX_SET_2GHZ_CTL_INDEX {Qc9KSetEeprom2GHzCtlIndex,{Qc9KEeprom2GHzCtlIndex,"CtlIndex2g",0},"ctl indexes, see eeprom guide for explanation",'x',0,WHAL_NUM_CTLS_2G,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_CTL_FREQUENCY {Qc9KSetEeprom2GHzCtlFrequency,{Qc9KEeprom2GHzCtlFrequency,"CtlFreq2g",0},"frequencies at which maximum transmit powers are specified",'u',"MHz",WHAL_NUM_CTLS_2G,WHAL_NUM_BAND_EDGES_2G,1,\
    &FreqZeroMinimum,&FrequencyMaximum2GHz,&FrequencyDefault2GHz,0,0}

#define QC98XX_SET_2GHZ_CTL_POWER {Qc9KSetEeprom2GHzCtlPower,{Qc9KEeprom2GHzCtlPower,"CtlPower2g","CtlPwr2g"},"maximum allowed transmit powers",'f',"dBm",WHAL_NUM_CTLS_2G,WHAL_NUM_BAND_EDGES_2G,1,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define QC98XX_SET_2GHZ_CTL_BANDEDGE {Qc9KSetEeprom2GHzCtlBandEdge,{Qc9KEeprom2GHzCtlBandEdge,"CtlBandEdge2g","ctlflag2g"},"band edge flag",'x',0,WHAL_NUM_CTLS_2G,WHAL_NUM_BAND_EDGES_2G,1,\
    &TwoBitsMinimum,&TwoBitsMaximum,&TwoBitsDefault,0,0}

#define QC98XX_SET_2GHZ_NOISE_FLOOR {Qc9KSetEeprom2GHzNoiseFloor,{Qc9KEeprom2GHzNoiseFloor,"CalPierRxNoisefloorCal2g",0},"noise floor measured during RX calibration",'d',"dBr",WHAL_NUM_11G_CAL_PIERS,WHAL_NUM_CHAINS,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_NOISE_FLOOR_POWER {Qc9KSetEeprom2GHzNoiseFloorPower,{Qc9KEeprom2GHzNoiseFloorPower,"CalPierRxNoisefloorPower2g",0},"noise floor power measured during RX calibration",'d',"dBm",WHAL_NUM_11G_CAL_PIERS,WHAL_NUM_CHAINS,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_NOISE_FLOOR_TEMPERATURE {Qc9KSetEeprom2GHzNoiseFloorTemperature,{Qc9KEeprom2GHzNoiseFloorTemperature,"CalPierRxTempMeas2g",0},"temperature measured during RX noise floor calibration",'u',0,WHAL_NUM_11G_CAL_PIERS,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_2GHZ_NOISE_FLOOR_TEMPERATURE_SLOPE {Qc9KSetEeprom2GHzNoiseFloorTemperatureSlope,{Qc9KEeprom2GHzNoiseFloorTemperatureSlope,"CalPierRxTempSlopeMeas2g",0},"temperature measured during RX noise floor calibration",'u',0,WHAL_NUM_11G_CAL_PIERS,1,1,\
	&UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}



// 2GHz Alpha Therm
#define QC98XX_SET_2GHZ_ALPHA_THERM_TABLE {Qc9KSetEeprom2GHzAlphaThermTable, {Qc9KEeprom2GHzAlphaThermTable,0,0},"alpha therm table",'u',0,WHAL_NUM_CHAINS,QC98XX_NUM_ALPHATHERM_CHANS_2G,QC98XX_NUM_ALPHATHERM_TEMPS,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

// 5GHz Cal Info
#define QC98XX_SET_5GHZ_CALIBRATION_FREQUENCY {Qc9KSetEeprom5GHzCalibrationFrequency,{Qc9KEeprom5GHzCalibrationFrequency,"calPierFreq5g",0},"frequencies at which calibration is performed",'u',"MHz",WHAL_NUM_11A_CAL_PIERS,1,1,\
    &FrequencyMinimum5GHz,&FrequencyMaximum5GHz,&FrequencyDefault5GHz,0,0}

//#define Qc9KEeprom5GHzCalPoint "5GHz.TxCalibration.CalPoint"
#define QC98XX_SET_5GHZ_CALPOINT_TXGAIN_INDEX {Qc9KSetEeprom5GHzCalPointTxGainIdx, {Qc9KEeprom5GHzCalPointTxGainIdx,0,0},"txgain index",'u',0,WHAL_NUM_11A_CAL_PIERS,WHAL_NUM_CHAINS,WHAL_NUM_CAL_GAINS,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_CALPOINT_DAC_GAIN {Qc9KSetEeprom5GHzCalPointDacGain, {Qc9KEeprom5GHzCalPointDacGain,0,0},"dac gain",'d',0,WHAL_NUM_11A_CAL_PIERS,WHAL_NUM_CAL_GAINS,1,\
    &SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_CALPOINT_POWER {Qc9KSetEeprom5GHzCalPointPower, {Qc9KEeprom5GHzCalPointPower,0,0},"power",'f',"dBm",WHAL_NUM_11A_CAL_PIERS,WHAL_NUM_CHAINS,WHAL_NUM_CAL_GAINS,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define QC98XX_SET_5GHZ_CALIBRATION_TEMPERATURE {Qc9KSetEeprom5GHzCalibrationTemperature,{Qc9KEeprom5GHzCalibrationTemperature,"CalPierTempMeas5g",0},"temperature measured during transmit power calibration",'u',0,WHAL_NUM_11A_CAL_PIERS,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_CALIBRATION_VOLTAGE {Qc9KSetEeprom5GHzCalibrationVoltage,{Qc9KEeprom5GHzCalibrationVoltage,"CalPierVoltMeas5g",0},"voltage measured during transmit power calibration",'u',0,WHAL_NUM_11A_CAL_PIERS,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_TARGET_FREQUENCY {Qc9KSetEeprom5GHzTargetFrequency,{Qc9KEeprom5GHzTargetFrequency,"calTGTFreq5g",0},"frequencies at which target powers for legacy rates are specified",'u',"MHz",WHAL_NUM_11A_20_TARGET_POWER_CHANS,1,1,\
    &FrequencyMinimum5GHz,&FrequencyMaximum5GHz,&FrequencyDefault5GHz,0,0}

#define QC98XX_SET_5GHZ_TARGET_FREQUENCY_VHT20 {Qc9KSetEeprom5GHzTargetFrequencyVht20,{Qc9KEeprom5GHzTargetFrequencyVht20,"calTGTFreqvht205g",0},"frequencies at which target powers for ht20 rates are specified",'u',"MHz",WHAL_NUM_11A_20_TARGET_POWER_CHANS,1,1,\
    &FrequencyMinimum5GHz,&FrequencyMaximum5GHz,&FrequencyDefault5GHz,0,0}

#define QC98XX_SET_5GHZ_TARGET_FREQUENCY_VHT40 {Qc9KSetEeprom5GHzTargetFrequencyVht40,{Qc9KEeprom5GHzTargetFrequencyVht40,"calTGTFreqvht405g",0},"frequencies at which target powers for ht40 rates are specified",'u',"MHz",WHAL_NUM_11A_40_TARGET_POWER_CHANS,1,1,\
    &FrequencyMinimum5GHz,&FrequencyMaximum5GHz,&FrequencyDefault5GHz,0,0}

#define QC98XX_SET_5GHZ_TARGET_FREQUENCY_VHT80 {Qc9KSetEeprom5GHzTargetFrequencyVht80,{Qc9KEeprom5GHzTargetFrequencyVht80,"calTGTFreqvht805g",0},"frequencies at which target powers for ht80 rates are specified",'u',"MHz",WHAL_NUM_11A_80_TARGET_POWER_CHANS,1,1,\
    &FrequencyMinimum5GHz,&FrequencyMaximum5GHz,&FrequencyDefault5GHz,0,0}

#define QC98XX_SET_5GHZ_TARGET_POWER {Qc9KSetEeprom5GHzTargetPower,{Qc9KEeprom5GHzTargetPower,"calTGTpwr5g",0},"target powers for legacy rates",'f',"dBm",WHAL_NUM_11A_20_TARGET_POWER_CHANS,WHAL_NUM_LEGACY_TARGET_POWER_RATES,1,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define QC98XX_SET_5GHZ_TARGET_POWER_VHT20 {Qc9KSetEeprom5GHzTargetPowerVht20,{Qc9KEeprom5GHzTargetPowerVht20,"calTGTpwrvht205g",0},"target powers for ht20 rates",'f',"dBm",WHAL_NUM_11A_20_TARGET_POWER_CHANS,VHT_TARGET_RATE_LAST,1,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define QC98XX_SET_5GHZ_TARGET_POWER_VHT40 {Qc9KSetEeprom5GHzTargetPowerVht40,{Qc9KEeprom5GHzTargetPowerVht40,"calTGTpwrvht405g",0},"target powers for ht40 rates",'f',"dBm",WHAL_NUM_11A_40_TARGET_POWER_CHANS,VHT_TARGET_RATE_LAST,1,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define QC98XX_SET_5GHZ_TARGET_POWER_VHT80 {Qc9KSetEeprom5GHzTargetPowerVht80,{Qc9KEeprom5GHzTargetPowerVht80,"calTGTpwrvht805g",0},"target powers for ht80 rates",'f',"dBm",WHAL_NUM_11A_80_TARGET_POWER_CHANS,VHT_TARGET_RATE_LAST,1,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

// 5GHz Ctl

#define QC98XX_SET_5GHZ_CTL_INDEX {Qc9KSetEeprom5GHzCtlIndex,{Qc9KEeprom5GHzCtlIndex,"CtlIndex5g",0},"ctl indexes, see eeprom guide for explanation",'x',0,WHAL_NUM_CTLS_5G,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_CTL_FREQUENCY {Qc9KSetEeprom5GHzCtlFrequency,{Qc9KEeprom5GHzCtlFrequency,"CtlFreq5g",0},"frequencies at which maximum transmit powers are specified",'u',"MHz",WHAL_NUM_CTLS_5G,WHAL_NUM_BAND_EDGES_5G,1,\
    &FreqZeroMinimum,&FrequencyMaximum5GHz,&FrequencyDefault5GHz,0,0}

#define QC98XX_SET_5GHZ_CTL_POWER {Qc9KSetEeprom5GHzCtlPower,{Qc9KEeprom5GHzCtlPower,"CtlPower5g","CtlPwr5g"},"maximum allowed transmit powers",'f',"dBm",WHAL_NUM_CTLS_5G,WHAL_NUM_BAND_EDGES_5G,1,\
    &PowerMinimum,&PowerMaximum,&PowerDefault,0,0}

#define QC98XX_SET_5GHZ_CTL_BANDEDGE {Qc9KSetEeprom5GHzCtlBandEdge,{Qc9KEeprom5GHzCtlBandEdge,"CtlBandEdge5g","ctlflag5g"},"band edge flag",'x',0,WHAL_NUM_CTLS_5G,WHAL_NUM_BAND_EDGES_5G,1,\
    &TwoBitsMinimum,&TwoBitsMaximum,&TwoBitsDefault,0,0}


#define QC98XX_SET_5GHZ_NOISE_FLOOR {Qc9KSetEeprom5GHzNoiseFloor,{Qc9KEeprom5GHzNoiseFloor,"CalPierRxNoisefloorCal5g",0},"noise floor measured during RX calibration",'d',"dBr",WHAL_NUM_11A_CAL_PIERS,WHAL_NUM_CHAINS,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_NOISE_FLOOR_POWER {Qc9KSetEeprom5GHzNoiseFloorPower,{Qc9KEeprom5GHzNoiseFloorPower,"CalPierRxNoisefloorPower5g",0},"noise floor power measured during RX calibration",'d',"dBm",WHAL_NUM_11A_CAL_PIERS,WHAL_NUM_CHAINS,1,\
	&SignedByteMinimum,&SignedByteMaximum,&SignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_NOISE_FLOOR_TEMPERATURE {Qc9KSetEeprom5GHzNoiseFloorTemperature,{Qc9KEeprom5GHzNoiseFloorTemperature,"CalPierRxTempMeas5g",0},"temperature measured during RX noise floor calibration",'u',0,WHAL_NUM_11A_CAL_PIERS,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

#define QC98XX_SET_5GHZ_NOISE_FLOOR_TEMPERATURE_SLOPE {Qc9KSetEeprom5GHzNoiseFloorTemperatureSlope,{Qc9KEeprom5GHzNoiseFloorTemperatureSlope,"CalPierRxTempSlopeMeas5g",0},"temperature slope measured during RX noise floor calibration",'u',0,WHAL_NUM_11A_CAL_PIERS,1,1,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}


// 5GHz Alpha Therm
#define QC98XX_SET_5GHZ_ALPHA_THERM_TABLE {Qc9KSetEeprom5GHzAlphaThermTable, {Qc9KEeprom5GHzAlphaThermTable,0,0},"alpha therm table",'u',0,WHAL_NUM_CHAINS,QC98XX_NUM_ALPHATHERM_CHANS_5G,QC98XX_NUM_ALPHATHERM_TEMPS,\
    &UnsignedByteMinimum,&UnsignedByteMaximum,&UnsignedByteDefault,0,0}

// Config
#define QC98XX_SET_CONFIG_ADDR {Qc9KSetEepromConfigAddr, {Qc9KEepromConfigAddr,0,0},"config",'x',0,QC98XX_CONFIG_ENTRIES,1,1, \
    &UnsignedIntMinimum,&UnsignedIntMaximum,&UnsignedIntDefault,0,0}


#endif //_QC98XX_SET_LIST_H_
