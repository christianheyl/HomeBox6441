
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>

#include "wlantype.h"

#include "smatch.h"
#include "UserPrint.h"
#include "TimeMillisecond.h"



//#if !defined(LINUX) && !defined(__APPLE__)
//#include "osdep.h"
//#endif


#include "Qc9KEepromPrint.h"
#include "Qc9KEepromParameter.h"

#include "ParameterConfigDef.h"

#define MBLOCK 10

#define MPRINTBUFFER 1024

//
// This fun structure contains the names, sizes, and types for all of the fields in the eeprom structure.
// It exists so that we can easily compare two structures and report the differences.
// It must match the definition of QC98XX_EEPROM in ar9300eep.h.
// Fields must be defined in the same order as they occur in the structure.
//

static _EepromPrintStruct _Qc9KEepromList[]=
{
    // baseEepHeader
//	{Qc9KEepromLength,offsetof(QC98XX_EEPROM,baseEepHeader.length),sizeof(A_UINT16),1,1,1,'u',1,-1,-1,0},
//	{Qc9KEepromChecksum,offsetof(QC98XX_EEPROM,baseEepHeader.checksum),sizeof(A_UINT16),1,1,1,'x',1,-1,-1,0},
	{Qc9KEepromVersion,offsetof(QC98XX_EEPROM,baseEepHeader.eeprom_version),sizeof(A_UINT8),1,1,1,'u',1,-1,-1,0},
//	{Qc9KEepromTemplateVersion,offsetof(QC98XX_EEPROM,baseEepHeader.template_version),sizeof(A_UINT8),1,1,1,'u',1,-1,-1,0},
	{Qc9KEepromMacAddress,offsetof(QC98XX_EEPROM,baseEepHeader.macAddr),6,sizeof(A_UINT8),1,1,'m',1,-1,-1,0},
	{Qc9KEepromRegulatoryDomain,offsetof(QC98XX_EEPROM,baseEepHeader.regDmn),sizeof(A_UINT16),2,1,1,'x',1,-1,-1,0},
	{Qc9KEepromOpFlags,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.opFlags),sizeof(A_UINT8),1,1,1,'x',1,-1,-1,0},
	{Qc9KEepromOpFlags2,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.opFlags2),sizeof(A_UINT8),1,1,1,'x',1,-1,-1,0},
	{Qc9KEepromBoardFlags,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.boardFlags),sizeof(A_UINT32),1,1,1,'x',1,-1,-1,0},
		//{Qc9KEeprom2GHzEnablePaPredistortion,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.boardFlags),sizeof(A_UINT8),1,1,1,'x',1,13,13,0},
		//{Qc9KEeprom5GHzEnablePaPredistortion,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.boardFlags),sizeof(A_UINT8),1,1,1,'x',1,14,14,0},
		//{Qc9KEepromTxGainTblEepEna,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.boardFlags),sizeof(A_UINT32),1,1,1,'x',1,16,16,0},
		//{Qc9KEepromTxGainTblEepScheme,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.boardFlags),sizeof(A_UINT32),1,1,1,'x',1,17,17,0},
		//{Qc9KEeprom2GHzEnableXpaBias,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.boardFlags),sizeof(A_UINT32),1,1,1,'x',1,18,18,0},
		//{Qc9KEeprom5GHzEnableXpaBias,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.boardFlags),sizeof(A_UINT32),1,1,1,'x',1,19,19,0},
	{Qc9KEepromBlueToothOptions,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.blueToothOptions),sizeof(A_UINT16),1,1,1,'x',1,-1,-1,0},
	{Qc9KEepromFeatureEnable,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.featureEnable),sizeof(A_UINT8),1,1,1,'x',1,-1,-1,0},
	//	{Qc9KEepromFeatureEnableTemperatureCompensation,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.featureEnable),sizeof(A_UINT8),1,1,1,'x',1,0,0,0},
	//	{Qc9KEepromFeatureEnableVoltageCompensation,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.featureEnable),sizeof(A_UINT8),1,1,1,'x',1,1,1,0},
		//{Qc9KEepromFeatureEnableFastClock,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.featureEnable),sizeof(A_UINT8),1,1,1,'x',1,2,2,0},
	//	{Qc9KEepromFeatureEnableDoubling,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.featureEnable),sizeof(A_UINT8),1,1,1,'x',1,3,3,0},
	//	{Qc9KEepromFeatureEnableInternalSwitchingRegulator,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.featureEnable),sizeof(A_UINT8),1,1,1,'x',1,4,4,0},
	//	{Qc9KEepromFeatureEnableTuningCaps,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.featureEnable),sizeof(A_UINT8),1,1,1,'x',1,6,6,0},
	{Qc9KEepromMiscellaneous,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.miscConfiguration),sizeof(A_UINT8),1,1,1,'x',1,-1,-1,0},
	//	{Qc9KEepromMiscellaneousDriveStrength,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.miscConfiguration),sizeof(A_UINT8),1,1,1,'x',1,0,0,0},
	//	{Qc9KEepromMiscellaneousThermometer,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.miscConfiguration),sizeof(A_UINT8),1,1,1,'d',1,2,1,-1},
	//	{Qc9KEepromMiscellaneousChainMaskReduce,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.miscConfiguration),sizeof(A_UINT8),1,1,1,'x',1,3,3,0},
	//	{Qc9KEepromMiscellaneousQuickDropEnable,offsetof(QC98XX_EEPROM,baseEepHeader.opCapBrdFlags.miscConfiguration),sizeof(A_UINT8),1,1,1,'x',1,4,4,0},
	//{Qc9KEepromBinBuildNumber,offsetof(QC98XX_EEPROM,baseEepHeader.binBuildNumber),sizeof(A_UINT16),1,1,1,'x',1,-1,-1,0},
	//{Qc9KEepromTxRxMask,offsetof(QC98XX_EEPROM,baseEepHeader.txrxMask),sizeof(A_UINT8),1,1,1,'x',1,-1,-1,0},
	    {Qc9KEepromTxRxMaskTx,offsetof(QC98XX_EEPROM,baseEepHeader.txrxMask),sizeof(A_UINT8),1,1,1,'x',1,7,4,0},
	    {Qc9KEepromTxRxMaskRx,offsetof(QC98XX_EEPROM,baseEepHeader.txrxMask),sizeof(A_UINT8),1,1,1,'x',1,3,0,0},
	{Qc9KEepromRfSilent,offsetof(QC98XX_EEPROM,baseEepHeader.rfSilent),sizeof(A_UINT8),1,1,1,'x',1,-1,-1,0},
	//	{Qc9KEepromRfSilentB0,offsetof(QC98XX_EEPROM,baseEepHeader.rfSilent),sizeof(A_UINT8),1,1,1,'x',1,0,0,0},
	//	{Qc9KEepromRfSilentB1,offsetof(QC98XX_EEPROM,baseEepHeader.rfSilent),sizeof(A_UINT8),1,1,1,'x',1,1,1,0},
	//	{Qc9KEepromRfSilentGpio,offsetof(QC98XX_EEPROM,baseEepHeader.rfSilent),sizeof(A_UINT8),1,1,1,'x',1,7,2,0},
	{Qc9KEepromWlanLedGpio,offsetof(QC98XX_EEPROM,baseEepHeader.wlanLedGpio),sizeof(A_UINT8),1,1,1,'x',1,-1,-1,0},
	{Qc9KEepromSpurBaseA,offsetof(QC98XX_EEPROM,baseEepHeader.spurBaseA),sizeof(A_UINT8),1,1,1,'u',1,-1,-1,0},
	{Qc9KEepromSpurBaseB,offsetof(QC98XX_EEPROM,baseEepHeader.spurBaseB),sizeof(A_UINT8),1,1,1,'u',1,-1,-1,0},
	{Qc9KEepromSpurRssiThresh,offsetof(QC98XX_EEPROM,baseEepHeader.spurRssiThresh),sizeof(A_UINT8),1,1,1,'u',1,-1,-1,0},
	{Qc9KEepromSpurRssiThreshCck,offsetof(QC98XX_EEPROM,baseEepHeader.spurRssiThreshCck),sizeof(A_UINT8),1,1,1,'u',1,-1,-1,0},
	{Qc9KEepromSpurMitFlag,offsetof(QC98XX_EEPROM,baseEepHeader.spurMitFlag),sizeof(A_UINT8),1,1,1,'u',1,-1,-1,0},

	{Qc9KEepromSwreg,offsetof(QC98XX_EEPROM,baseEepHeader.swreg),sizeof(A_UINT8),1,1,1,'x',1,-1,-1,0},
    {Qc9KEepromTxRxGain,offsetof(QC98XX_EEPROM,baseEepHeader.txrxgain),sizeof(A_UINT8),1,1,1,'x',1,-1,-1,0},
	//	{Qc9KEepromTxRxGainTx,offsetof(QC98XX_EEPROM,baseEepHeader.txrxgain),sizeof(A_UINT8),1,1,1,'x',1,7,4,0},
	//	{Qc9KEepromTxRxGainRx,offsetof(QC98XX_EEPROM,baseEepHeader.txrxgain),sizeof(A_UINT8),1,1,1,'x',1,3,0,0},
	{Qc9KEepromPowerTableOffset,offsetof(QC98XX_EEPROM,baseEepHeader.pwrTableOffset),sizeof(A_INT8),1,1,1,'d',1,-1,-1,0},
	{Qc9KEepromDeltaCck20,offsetof(QC98XX_EEPROM,baseEepHeader.deltaCck20_t10),sizeof(A_INT8),1,1,1,'d',1,-1,-1,0},
	{Qc9KEepromDelta4020,offsetof(QC98XX_EEPROM,baseEepHeader.delta4020_t10),sizeof(A_INT8),1,1,1,'d',1,-1,-1,0},
	{Qc9KEepromDelta8020,offsetof(QC98XX_EEPROM,baseEepHeader.delta8020_t10),sizeof(A_INT8),1,1,1,'d',1,-1,-1,0},
	{Qc9KEepromTuningCaps,offsetof(QC98XX_EEPROM,baseEepHeader.param_for_tuning_caps),sizeof(A_UINT8),1,1,1,'x',1,-1,-1,0},
	{Qc9KEepromCustomerData,offsetof(QC98XX_EEPROM,baseEepHeader.custData),CUSTOMER_DATA_SIZE,sizeof(A_UINT8),1,1,'t',1,-1,-1,0},
	//{Qc9KEepromBaseFuture,offsetof(QC98XX_EEPROM,baseEepHeader.baseFuture),sizeof(A_UINT8),MAX_BASE_FUTURE,1,1,'x',1,-1,-1,0},
                         
    // biModalHeader           
	//2G
	{Qc9KEeprom2GHzVoltageSlope,offsetof(QC98XX_EEPROM,biModalHeader)+sizeof(BIMODAL_EEP_HEADER)+offsetof(BIMODAL_EEP_HEADER,voltSlope),sizeof(A_INT8),WHAL_NUM_CHAINS,1,1,'d',1,-1,-1,0},
	{Qc9KEeprom2GHzSpur,offsetof(QC98XX_EEPROM,biModalHeader)+sizeof(BIMODAL_EEP_HEADER)+offsetof(BIMODAL_EEP_HEADER,spurChans)+offsetof(SPUR_CHAN,spurChan),sizeof(A_UINT8),QC98XX_EEPROM_MODAL_SPURS,1,1,'2',sizeof(SPUR_CHAN),-1,-1,0},
	{Qc9KEeprom2GHzSpurAPrimSecChoose,offsetof(QC98XX_EEPROM,biModalHeader)+sizeof(BIMODAL_EEP_HEADER)+offsetof(BIMODAL_EEP_HEADER,spurChans)+offsetof(SPUR_CHAN,spurA_PrimSecChoose),sizeof(A_UINT8),QC98XX_EEPROM_MODAL_SPURS,1,1,'2',sizeof(SPUR_CHAN),-1,-1,0},
	{Qc9KEeprom2GHzSpurBPrimSecChoose,offsetof(QC98XX_EEPROM,biModalHeader)+sizeof(BIMODAL_EEP_HEADER)+offsetof(BIMODAL_EEP_HEADER,spurChans)+offsetof(SPUR_CHAN,spurB_PrimSecChoose),sizeof(A_UINT8),QC98XX_EEPROM_MODAL_SPURS,1,1,'2',sizeof(SPUR_CHAN),-1,-1,0},
	{Qc9KEeprom2GHzXpaBiasLevel,offsetof(QC98XX_EEPROM,biModalHeader)+sizeof(BIMODAL_EEP_HEADER)+offsetof(BIMODAL_EEP_HEADER,xpaBiasLvl),sizeof(A_UINT8),1,1,1,'x',1,-1,-1,0},
	{Qc9KEeprom2GHzAntennaGain,offsetof(QC98XX_EEPROM,biModalHeader)+sizeof(BIMODAL_EEP_HEADER)+offsetof(BIMODAL_EEP_HEADER,antennaGainCh),sizeof(A_INT8),1,1,1,'d',1,-1,-1,0},

	{Qc9KEeprom2GHzAntennaControlCommon,offsetof(QC98XX_EEPROM,biModalHeader)+sizeof(BIMODAL_EEP_HEADER)+offsetof(BIMODAL_EEP_HEADER,antCtrlCommon),sizeof(A_UINT32),1,1,1,'x',1,-1,-1,0},
	{Qc9KEeprom2GHzAntennaControlCommon2,offsetof(QC98XX_EEPROM,biModalHeader)+sizeof(BIMODAL_EEP_HEADER)+offsetof(BIMODAL_EEP_HEADER,antCtrlCommon2),sizeof(A_UINT32),1,1,1,'x',1,-1,-1,0},
	{Qc9KEeprom2GHzAntennaControlChain,offsetof(QC98XX_EEPROM,biModalHeader)+sizeof(BIMODAL_EEP_HEADER)+offsetof(BIMODAL_EEP_HEADER,antCtrlChain),sizeof(A_UINT16),WHAL_NUM_CHAINS,1,1,'x',2,-1,-1,0},
	{Qc9KEeprom2GHzRxFilterCap,offsetof(QC98XX_EEPROM,biModalHeader)+sizeof(BIMODAL_EEP_HEADER)+offsetof(BIMODAL_EEP_HEADER,rxFilterCap),sizeof(A_UINT8),1,1,1,'u',1,-1,-1,0},
	{Qc9KEeprom2GHzRxGainCap,offsetof(QC98XX_EEPROM,biModalHeader)+sizeof(BIMODAL_EEP_HEADER)+offsetof(BIMODAL_EEP_HEADER,rxGainCap),sizeof(A_UINT8),1,1,1,'u',1,-1,-1,0},
	{Qc9KEeprom2GHzNoiseFlrThr,offsetof(QC98XX_EEPROM,biModalHeader)+offsetof(BIMODAL_EEP_HEADER,noiseFlrThr),sizeof(A_INT8),1,1,1,'d',1,3,0,0},
	{Qc9KEeprom2GHzMinCcaPwrChain,offsetof(QC98XX_EEPROM,biModalHeader)+offsetof(BIMODAL_EEP_HEADER,minCcaPwr),sizeof(A_UINT16),WHAL_NUM_CHAINS,1,1,'d',1,-1,-1,0},

    // 5G
	{Qc9KEeprom5GHzVoltageSlope,offsetof(QC98XX_EEPROM,biModalHeader)+offsetof(BIMODAL_EEP_HEADER,voltSlope),sizeof(A_INT8),WHAL_NUM_CHAINS,1,1,'d',1,-1,-1,0},
	{Qc9KEeprom5GHzSpur,offsetof(QC98XX_EEPROM,biModalHeader)+offsetof(BIMODAL_EEP_HEADER,spurChans)+offsetof(SPUR_CHAN,spurChan),sizeof(A_UINT8),QC98XX_EEPROM_MODAL_SPURS,1,1,'2',sizeof(SPUR_CHAN),-1,-1,0},
	{Qc9KEeprom5GHzSpurAPrimSecChoose,offsetof(QC98XX_EEPROM,biModalHeader)+offsetof(BIMODAL_EEP_HEADER,spurChans)+offsetof(SPUR_CHAN,spurA_PrimSecChoose),sizeof(A_UINT8),QC98XX_EEPROM_MODAL_SPURS,1,1,'2',sizeof(SPUR_CHAN),-1,-1,0},
	{Qc9KEeprom5GHzSpurBPrimSecChoose,offsetof(QC98XX_EEPROM,biModalHeader)+offsetof(BIMODAL_EEP_HEADER,spurChans)+offsetof(SPUR_CHAN,spurB_PrimSecChoose),sizeof(A_UINT8),QC98XX_EEPROM_MODAL_SPURS,1,1,'2',sizeof(SPUR_CHAN),-1,-1,0},
	{Qc9KEeprom5GHzXpaBiasLevel,offsetof(QC98XX_EEPROM,biModalHeader)+offsetof(BIMODAL_EEP_HEADER,xpaBiasLvl),sizeof(A_UINT8),1,1,1,'x',1,-1,-1,0},
	{Qc9KEeprom5GHzAntennaGain,offsetof(QC98XX_EEPROM,biModalHeader)+offsetof(BIMODAL_EEP_HEADER,antennaGainCh),sizeof(A_INT8),1,1,1,'d',1,-1,-1,0},
	{Qc9KEeprom5GHzAntennaControlCommon,offsetof(QC98XX_EEPROM,biModalHeader)+offsetof(BIMODAL_EEP_HEADER,antCtrlCommon),sizeof(A_UINT32),1,1,1,'x',1,-1,-1,0},
	{Qc9KEeprom5GHzAntennaControlCommon2,offsetof(QC98XX_EEPROM,biModalHeader)+offsetof(BIMODAL_EEP_HEADER,antCtrlCommon2),sizeof(A_UINT32),1,1,1,'x',1,-1,-1,0},
	{Qc9KEeprom5GHzAntennaControlChain,offsetof(QC98XX_EEPROM,biModalHeader)+offsetof(BIMODAL_EEP_HEADER,antCtrlChain),sizeof(A_UINT16),WHAL_NUM_CHAINS,1,1,'x',2,-1,-1,0},
	{Qc9KEeprom5GHzRxFilterCap,offsetof(QC98XX_EEPROM,biModalHeader)+offsetof(BIMODAL_EEP_HEADER,rxFilterCap),sizeof(A_UINT8),1,1,1,'u',1,-1,-1,0},
	{Qc9KEeprom5GHzRxGainCap,offsetof(QC98XX_EEPROM,biModalHeader)+offsetof(BIMODAL_EEP_HEADER,rxGainCap),sizeof(A_UINT8),1,1,1,'u',1,-1,-1,0},
        {Qc9KEeprom5GHzNoiseFlrThr,offsetof(QC98XX_EEPROM,biModalHeader)+sizeof(BIMODAL_EEP_HEADER)+offsetof(BIMODAL_EEP_HEADER,noiseFlrThr),sizeof(A_INT8),1,1,1,'d',1,3,0,0},
        {Qc9KEeprom5GHzMinCcaPwrChain,offsetof(QC98XX_EEPROM,biModalHeader)+sizeof(BIMODAL_EEP_HEADER)+offsetof(BIMODAL_EEP_HEADER,minCcaPwr),sizeof(A_UINT16),WHAL_NUM_CHAINS,1,1,'d',1,-1,-1,0},

	// freqModalHeader
	{Qc9KEeprom2GHzXatten1Db,offsetof(QC98XX_EEPROM,freqModalHeader)+offsetof(FREQ_MODAL_EEP_HEADER,xatten1DB)+offsetof(FREQ_MODAL_PIERS,value2G),sizeof(A_UINT8),WHAL_NUM_CHAINS,1,1,'x',4,-1,-1,0},
	{Qc9KEeprom5GHzXatten1DbLow,offsetof(QC98XX_EEPROM,freqModalHeader)+offsetof(FREQ_MODAL_EEP_HEADER,xatten1DB)+offsetof(FREQ_MODAL_PIERS,value5GLow),sizeof(A_UINT8),WHAL_NUM_CHAINS,1,1,'x',4,-1,-1,0},
	{Qc9KEeprom5GHzXatten1DbMid,offsetof(QC98XX_EEPROM,freqModalHeader)+offsetof(FREQ_MODAL_EEP_HEADER,xatten1DB)+offsetof(FREQ_MODAL_PIERS,value5GMid),sizeof(A_UINT8),WHAL_NUM_CHAINS,1,1,'x',4,-1,-1,0},
	{Qc9KEeprom5GHzXatten1DbHigh,offsetof(QC98XX_EEPROM,freqModalHeader)+offsetof(FREQ_MODAL_EEP_HEADER,xatten1DB)+offsetof(FREQ_MODAL_PIERS,value5GHigh),sizeof(A_UINT8),WHAL_NUM_CHAINS,1,1,'x',4,-1,-1,0},

	{Qc9KEeprom2GHzXatten1Margin,offsetof(QC98XX_EEPROM,freqModalHeader)+offsetof(FREQ_MODAL_EEP_HEADER,xatten1Margin)+offsetof(FREQ_MODAL_PIERS,value2G),sizeof(A_UINT8),WHAL_NUM_CHAINS,1,1,'x',4,-1,-1,0},
	{Qc9KEeprom5GHzXatten1MarginLow,offsetof(QC98XX_EEPROM,freqModalHeader)+offsetof(FREQ_MODAL_EEP_HEADER,xatten1Margin)+offsetof(FREQ_MODAL_PIERS,value5GLow),sizeof(A_UINT8),WHAL_NUM_CHAINS,1,1,'x',4,-1,-1,0},
	{Qc9KEeprom5GHzXatten1MarginMid,offsetof(QC98XX_EEPROM,freqModalHeader)+offsetof(FREQ_MODAL_EEP_HEADER,xatten1Margin)+offsetof(FREQ_MODAL_PIERS,value5GMid),sizeof(A_UINT8),WHAL_NUM_CHAINS,1,1,'x',4,-1,-1,0},
	{Qc9KEeprom5GHzXatten1MarginHigh,offsetof(QC98XX_EEPROM,freqModalHeader)+offsetof(FREQ_MODAL_EEP_HEADER,xatten1Margin)+offsetof(FREQ_MODAL_PIERS,value5GHigh),sizeof(A_UINT8),WHAL_NUM_CHAINS,1,1,'x',4,-1,-1,0},
    
	{Qc9KEepromThermAdcScaledGain,offsetof(QC98XX_EEPROM,chipCalData)+offsetof(CAL_DATA_PER_CHIP,thermAdcScaledGain),sizeof(A_INT16),1,1,1,'x',1,-1,-1,0},
	{Qc9KEepromThermAdcOffset,offsetof(QC98XX_EEPROM,chipCalData)+offsetof(CAL_DATA_PER_CHIP,thermAdcOffset),sizeof(A_INT8),1,1,1,'x',1,-1,-1,0},

	// 2G
	{Qc9KEeprom2GHzCalibrationFrequency,offsetof(QC98XX_EEPROM,calFreqPier2G),sizeof(A_UINT8),WHAL_NUM_11G_CAL_PIERS,1,1,'2',1,-1,-1,0},
	
        {Qc9KEeprom2GHzCalPointTxGainIdx,offsetof(QC98XX_EEPROM,calPierData2G)+offsetof(CAL_DATA_PER_FREQ_OLPC,calPerPoint)+offsetof(CAL_DATA_PER_POINT_OLPC,txgainIdx),sizeof(A_UINT8),WHAL_NUM_11G_CAL_PIERS,WHAL_NUM_CHAINS,WHAL_NUM_CAL_GAINS,'d',10622,-1,-1,0},
        {Qc9KEeprom2GHzCalPointPower,offsetof(QC98XX_EEPROM,calPierData2G)+offsetof(CAL_DATA_PER_FREQ_OLPC,calPerPoint)+offsetof(CAL_DATA_PER_POINT_OLPC,power_t8),sizeof(A_UINT16),WHAL_NUM_11G_CAL_PIERS,WHAL_NUM_CHAINS,WHAL_NUM_CAL_GAINS,'p',10622,-1,-1,0},
        {Qc9KEeprom2GHzCalPointDacGain,offsetof(QC98XX_EEPROM,calPierData2G)+offsetof(CAL_DATA_PER_FREQ_OLPC,dacGain),sizeof(A_INT8),WHAL_NUM_11G_CAL_PIERS,WHAL_NUM_CAL_GAINS,1,'d',122,-1,-1,0},
        {Qc9KEeprom2GHzCalibrationTemperature,offsetof(QC98XX_EEPROM,calPierData2G)+offsetof(CAL_DATA_PER_FREQ_OLPC,thermCalVal),sizeof(A_UINT8),WHAL_NUM_11G_CAL_PIERS,1,1,'d',22,-1,-1,0},
	{Qc9KEeprom2GHzCalibrationVoltage,offsetof(QC98XX_EEPROM,calPierData2G)+offsetof(CAL_DATA_PER_FREQ_OLPC,voltCalVal),sizeof(A_UINT8),WHAL_NUM_11G_CAL_PIERS,1,1,'u',sizeof(CAL_DATA_PER_FREQ_OLPC),-1,-1,0},

    {Qc9KEeprom2GHzTargetFrequencyCck,offsetof(QC98XX_EEPROM,targetFreqbinCck),sizeof(A_UINT8),WHAL_NUM_11B_TARGET_POWER_CHANS,1,1,'2',1,-1,-1,0},
	{Qc9KEeprom2GHzTargetFrequency,offsetof(QC98XX_EEPROM,targetFreqbin2G),sizeof(A_UINT8),WHAL_NUM_11G_LEG_TARGET_POWER_CHANS,1,1,'2',1,-1,-1,0},
	{Qc9KEeprom2GHzTargetFrequencyVht20,offsetof(QC98XX_EEPROM,targetFreqbin2GVHT20),sizeof(A_UINT8),WHAL_NUM_11G_20_TARGET_POWER_CHANS,1,1,'2',1,-1,-1,0},
	{Qc9KEeprom2GHzTargetFrequencyVht40,offsetof(QC98XX_EEPROM,targetFreqbin2GVHT40),sizeof(A_UINT8),WHAL_NUM_11G_40_TARGET_POWER_CHANS,1,1,'2',1,-1,-1,0},
    {Qc9KEeprom2GHzTargetPowerCck,offsetof(QC98XX_EEPROM,targetPowerCck),sizeof(A_UINT8),WHAL_NUM_11B_TARGET_POWER_CHANS,WHAL_NUM_11B_TARGET_POWER_RATES,1,'p',1*100+WHAL_NUM_11B_TARGET_POWER_RATES,-1,-1,0},
    {Qc9KEeprom2GHzTargetPower,offsetof(QC98XX_EEPROM,targetPower2G),sizeof(A_UINT8),WHAL_NUM_11G_LEG_TARGET_POWER_CHANS,WHAL_NUM_LEGACY_TARGET_POWER_RATES,1,'p',1*100+WHAL_NUM_LEGACY_TARGET_POWER_RATES,-1,-1,0},
	{Qc9KEeprom2GHzTargetPowerVht20,offsetof(QC98XX_EEPROM,extTPow2xDelta2G),sizeof(A_UINT8),QC98XX_EXT_TARGET_POWER_SIZE_2G,1,1,'8',1,-1,-1,0},
	{Qc9KEeprom2GHzTargetPowerVht20,offsetof(QC98XX_EEPROM,targetPower2GVHT20)+offsetof(CAL_TARGET_POWER_11G_20,tPow2xBase),sizeof(A_UINT8),WHAL_NUM_11G_20_TARGET_POWER_CHANS,WHAL_NUM_STREAMS,1,'9',112,-1,-1,0},
	{Qc9KEeprom2GHzTargetPowerVht20,offsetof(QC98XX_EEPROM,targetPower2GVHT20)+offsetof(CAL_TARGET_POWER_11G_20,tPow2xDelta),sizeof(A_UINT8),WHAL_NUM_11G_20_TARGET_POWER_CHANS,WHAL_NUM_11G_20_TARGET_POWER_RATES,1,'q',112,-1,-1,0},
//	{Qc9KEeprom2GHzTargetPowerVht20,offsetof(QC98XX_EEPROM,targetPower2GVHT20),sizeof(A_UINT8),WHAL_NUM_11G_20_TARGET_POWER_CHANS,WHAL_NUM_11G_20_TARGET_POWER_RATES,1,'p',WHAL_NUM_11G_20_TARGET_POWER_CHANS*100+WHAL_NUM_11G_20_TARGET_POWER_RATES,-1,-1,0},
//	{Qc9KEeprom2GHzTargetPowerVht40,offsetof(QC98XX_EEPROM,targetPower2GVHT40),sizeof(A_UINT8),WHAL_NUM_11G_40_TARGET_POWER_CHANS,WHAL_NUM_11G_40_TARGET_POWER_RATES,1,'p',WHAL_NUM_11G_40_TARGET_POWER_CHANS*100+WHAL_NUM_11G_40_TARGET_POWER_RATES,-1,-1,0},
	{Qc9KEeprom2GHzTargetPowerVht40,offsetof(QC98XX_EEPROM,targetPower2GVHT40)+offsetof(CAL_TARGET_POWER_11G_40,tPow2xBase),sizeof(A_UINT8),WHAL_NUM_11G_40_TARGET_POWER_CHANS,WHAL_NUM_STREAMS,1,'9',112,-1,-1,0},
	{Qc9KEeprom2GHzTargetPowerVht40,offsetof(QC98XX_EEPROM,targetPower2GVHT40)+offsetof(CAL_TARGET_POWER_11G_40,tPow2xDelta),sizeof(A_UINT8),WHAL_NUM_11G_40_TARGET_POWER_CHANS,WHAL_NUM_11G_40_TARGET_POWER_RATES,1,'q',112,-1,-1,10},
	
	{Qc9KEeprom2GHzCtlIndex,offsetof(QC98XX_EEPROM,ctlIndex2G),sizeof(A_UINT8),WHAL_NUM_CTLS_2G,1,1,'x',1,-1,-1,0},
	{Qc9KEeprom2GHzCtlFrequency,offsetof(QC98XX_EEPROM,ctlFreqbin2G),sizeof(A_UINT8),WHAL_NUM_CTLS_2G,WHAL_NUM_BAND_EDGES_2G,1,'2',1*100+WHAL_NUM_BAND_EDGES_2G,-1,-1,0},
 	{Qc9KEeprom2GHzCtlPower,offsetof(QC98XX_EEPROM,ctlData2G)+offsetof(CAL_CTL_DATA_2G,ctl_edges),sizeof(A_UINT8),WHAL_NUM_CTLS_2G,WHAL_NUM_BAND_EDGES_2G,1,'p',1*100+WHAL_NUM_BAND_EDGES_2G,-1,-1,0},
    {Qc9KEeprom2GHzCtlBandEdge,offsetof(QC98XX_EEPROM,ctlData2G)+offsetof(CAL_CTL_DATA_2G,ctl_edges),sizeof(A_UINT8),WHAL_NUM_CTLS_2G,WHAL_NUM_BAND_EDGES_2G,1,'x',1*100+WHAL_NUM_BAND_EDGES_2G,7,6,0},

    {Qc9KEeprom2GHzAlphaThermTable,offsetof(QC98XX_EEPROM,tempComp2G)+offsetof(TEMP_COMP_TBL_2G,alphaThermTbl),sizeof(A_UINT8),WHAL_NUM_CHAINS,QC98XX_NUM_ALPHATHERM_CHANS_2G,QC98XX_NUM_ALPHATHERM_TEMPS,'x',10416,-1,-1,0},
    
    // 5G
    {Qc9KEeprom5GHzCalibrationFrequency,offsetof(QC98XX_EEPROM,calFreqPier5G),sizeof(A_UINT8),WHAL_NUM_11A_CAL_PIERS,1,1,'5',1,-1,-1,0},
        {Qc9KEeprom5GHzCalPointTxGainIdx,offsetof(QC98XX_EEPROM,calPierData5G)+offsetof(CAL_DATA_PER_FREQ_OLPC,calPerPoint)+offsetof(CAL_DATA_PER_POINT_OLPC,txgainIdx),sizeof(A_UINT8),WHAL_NUM_11A_CAL_PIERS,WHAL_NUM_CHAINS,WHAL_NUM_CAL_GAINS,'d',10622,-1,-1,0},
        {Qc9KEeprom5GHzCalPointPower,offsetof(QC98XX_EEPROM,calPierData5G)+offsetof(CAL_DATA_PER_FREQ_OLPC,calPerPoint)+offsetof(CAL_DATA_PER_POINT_OLPC,power_t8),sizeof(A_UINT16),WHAL_NUM_11A_CAL_PIERS,WHAL_NUM_CHAINS,WHAL_NUM_CAL_GAINS,'p',10622,-1,-1,0},
        {Qc9KEeprom5GHzCalPointDacGain,offsetof(QC98XX_EEPROM,calPierData5G)+offsetof(CAL_DATA_PER_FREQ_OLPC,dacGain),sizeof(A_INT8),WHAL_NUM_11A_CAL_PIERS,WHAL_NUM_CAL_GAINS,1,'d',122,-1,-1,0},
        {Qc9KEeprom5GHzCalibrationTemperature,offsetof(QC98XX_EEPROM,calPierData5G)+offsetof(CAL_DATA_PER_FREQ_OLPC,thermCalVal),sizeof(A_UINT8),WHAL_NUM_11A_CAL_PIERS,1,1,'d',22,-1,-1,0},
	{Qc9KEeprom5GHzCalibrationVoltage,offsetof(QC98XX_EEPROM,calPierData5G)+offsetof(CAL_DATA_PER_FREQ_OLPC,voltCalVal),sizeof(A_UINT8),WHAL_NUM_11A_CAL_PIERS,1,1,'u',sizeof(CAL_DATA_PER_FREQ_OLPC),-1,-1,0},
	
    {Qc9KEeprom5GHzTargetFrequency,offsetof(QC98XX_EEPROM,targetFreqbin5G),sizeof(A_UINT8),WHAL_NUM_11A_LEG_TARGET_POWER_CHANS,1,1,'5',1,-1,-1,0},
	{Qc9KEeprom5GHzTargetFrequencyVht20,offsetof(QC98XX_EEPROM,targetFreqbin5GVHT20),sizeof(A_UINT8),WHAL_NUM_11A_20_TARGET_POWER_CHANS,1,1,'5',1,-1,-1,0},
	{Qc9KEeprom5GHzTargetFrequencyVht40,offsetof(QC98XX_EEPROM,targetFreqbin5GVHT40),sizeof(A_UINT8),WHAL_NUM_11A_40_TARGET_POWER_CHANS,1,1,'5',1,-1,-1,0},
	{Qc9KEeprom5GHzTargetFrequencyVht80,offsetof(QC98XX_EEPROM,targetFreqbin5GVHT80),sizeof(A_UINT8),WHAL_NUM_11A_80_TARGET_POWER_CHANS,1,1,'5',1,-1,-1,0},
	
    {Qc9KEeprom5GHzTargetPower,offsetof(QC98XX_EEPROM,targetPower5G),sizeof(A_UINT8),WHAL_NUM_11A_LEG_TARGET_POWER_CHANS,WHAL_NUM_LEGACY_TARGET_POWER_RATES,1,'p',104,-1,-1,0},
	{Qc9KEeprom5GHzTargetPowerVht20,offsetof(QC98XX_EEPROM,extTPow2xDelta5G),sizeof(A_UINT8),QC98XX_EXT_TARGET_POWER_SIZE_5G,1,1,'8',1,-1,-1,0},
	{Qc9KEeprom5GHzTargetPowerVht20,offsetof(QC98XX_EEPROM,targetPower5GVHT20)+offsetof(CAL_TARGET_POWER_11A_20,tPow2xBase),sizeof(A_UINT8),WHAL_NUM_11A_20_TARGET_POWER_CHANS,WHAL_NUM_STREAMS,1,'9',112,-1,-1,0},
	{Qc9KEeprom5GHzTargetPowerVht20,offsetof(QC98XX_EEPROM,targetPower5GVHT20)+offsetof(CAL_TARGET_POWER_11A_20,tPow2xDelta),sizeof(A_UINT8),WHAL_NUM_11A_20_TARGET_POWER_CHANS,WHAL_NUM_11A_20_TARGET_POWER_RATES,1,'q',112,-1,-1,0},
//	{Qc9KEeprom5GHzTargetPowerVht20,offsetof(QC98XX_EEPROM,targetPower5GVHT20),sizeof(A_UINT8),WHAL_NUM_11A_20_TARGET_POWER_CHANS,WHAL_NUM_11A_20_TARGET_POWER_RATES,1,'q',112,-1,-1,0},
	{Qc9KEeprom5GHzTargetPowerVht40,offsetof(QC98XX_EEPROM,targetPower5GVHT40)+offsetof(CAL_TARGET_POWER_11A_40,tPow2xBase),sizeof(A_UINT8),WHAL_NUM_11A_40_TARGET_POWER_CHANS,WHAL_NUM_STREAMS,1,'9',112,-1,-1,0},
	{Qc9KEeprom5GHzTargetPowerVht40,offsetof(QC98XX_EEPROM,targetPower5GVHT40)+offsetof(CAL_TARGET_POWER_11A_40,tPow2xDelta),sizeof(A_UINT8),WHAL_NUM_11A_40_TARGET_POWER_CHANS,WHAL_NUM_11A_40_TARGET_POWER_RATES,1,'q',112,-1,-1,1},
	{Qc9KEeprom5GHzTargetPowerVht80,offsetof(QC98XX_EEPROM,targetPower5GVHT80)+offsetof(CAL_TARGET_POWER_11A_80,tPow2xBase),sizeof(A_UINT8),WHAL_NUM_11A_80_TARGET_POWER_CHANS,WHAL_NUM_STREAMS,1,'9',112,-1,-1,0},
	{Qc9KEeprom5GHzTargetPowerVht80,offsetof(QC98XX_EEPROM,targetPower5GVHT80)+offsetof(CAL_TARGET_POWER_11A_80,tPow2xDelta),sizeof(A_UINT8),WHAL_NUM_11A_80_TARGET_POWER_CHANS,WHAL_NUM_11A_80_TARGET_POWER_RATES,1,'q',112,-1,-1,2},
	
    {Qc9KEeprom5GHzCtlIndex,offsetof(QC98XX_EEPROM,ctlIndex5G),sizeof(A_UINT8),WHAL_NUM_CTLS_5G,1,1,'x',1,-1,-1,0},
	{Qc9KEeprom5GHzCtlFrequency,offsetof(QC98XX_EEPROM,ctlFreqbin5G),sizeof(A_UINT8),WHAL_NUM_CTLS_5G,WHAL_NUM_BAND_EDGES_5G,1,'5',1*100+WHAL_NUM_BAND_EDGES_5G,-1,-1,0},                              
    {Qc9KEeprom5GHzCtlPower,offsetof(QC98XX_EEPROM,ctlData5G)+offsetof(CAL_CTL_DATA_5G,ctl_edges),sizeof(A_UINT8),WHAL_NUM_CTLS_5G,WHAL_NUM_BAND_EDGES_5G,1,'p',1*100+WHAL_NUM_BAND_EDGES_5G,-1,-1,0},
    {Qc9KEeprom5GHzCtlBandEdge,offsetof(QC98XX_EEPROM,ctlData5G)+offsetof(CAL_CTL_DATA_5G,ctl_edges),sizeof(A_UINT8),WHAL_NUM_CTLS_5G,WHAL_NUM_BAND_EDGES_5G,1,'x',1*100+WHAL_NUM_BAND_EDGES_5G,7,6,0},  

    {Qc9KEeprom5GHzAlphaThermTable,offsetof(QC98XX_EEPROM,tempComp5G)+offsetof(TEMP_COMP_TBL_5G,alphaThermTbl),sizeof(A_UINT8),WHAL_NUM_CHAINS,QC98XX_NUM_ALPHATHERM_CHANS_5G,QC98XX_NUM_ALPHATHERM_TEMPS,'x',10432,-1,-1,0},
                                             
    //{Qc9KEepromConfigAddr,offsetof(QC98XX_EEPROM,configAddr),sizeof(A_UINT32),QC98XX_CONFIG_ENTRIES,1,1,'x',1,-1,-1,0},
    
};

static unsigned int Mask(int low, int high)
{
	unsigned int mask;
	int ib;

	mask=0;
	if(low<=0)
	{
		low=0;
	}
	if(high>=31)
	{
		high=31;
	}
	for(ib=low; ib<=high; ib++)
	{
		mask|=(1<<ib);
	}
	return mask;
}


static int Qc9KEepromPrintIt(unsigned char *data, int type, int size, int length, int high, int low, int voff,
	char *buffer, int max)
{
	char *vc;
	short *vs;
	int *vl;
	//double *vd;
	float *vf;
	int lc, nc;
	int it;
	char text[MPRINTBUFFER];
	int vuse;
	unsigned int mask;
	int iuse;
	int rr;
	char comma[10];
	static rate_index=0;
	static int target_powers[24];
	strcpy(comma,",");
	comma[1]=0;
	lc=0;
	switch(size)
	{
		case 1:
			vc=(char *)data;
			if(type=='t')
			{
				//
				// make sure there is a null
				//
				if(length>MPRINTBUFFER)
				{
					length=MPRINTBUFFER;
				}
				strncpy(text,vc,length);
				text[length]=0;

				nc=SformatOutput(&buffer[lc],max-lc-1,"%s",text);
				if(nc>0)
				{
					lc+=nc;
				}
			}
			else
			{
				for(it=0; it<length; it++)
				{
					vuse=vc[it]&0xff;
					if(high>=0 && low>=0)
					{
						mask=Mask(low,high);
						vuse=(vuse&mask)>>low;
					}
					switch(type)
					{
						case 'p':
#ifdef UNUSED
							if(vuse&0xc0)
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%.1lf-%d",0.5*((int)(vuse&0x3f)),(int)(vuse>>6));
							}
							else
#endif
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%.1lf",0.5*(voff+(int)(vuse&0x3f)));
							}
							break;
						case 'q':
							if(rate_index==23){
								nc=0;
								for(rr=0;rr<23;rr++){
									nc=SformatOutput(&buffer[lc],max-lc-1,"%.1lf%s",0.5*target_powers[rr],comma);
									if(nc>0)
									{
										lc+=nc;
									}
								}
								nc=SformatOutput(&buffer[lc],max-lc-1,"%.1lf",0.5*(voff+(int)(vuse&0xf)));
								if(nc>0)
								{
									lc+=nc;
								}
								for( rr=0; rr<MBLOCK; rr++)
								{
									nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"|");
									if(nc>0)
									{
										lc+=nc;
									}
								}
								rate_index=0;
							}else{
								target_powers[rate_index]=(voff+(int)(vuse&0xf));
								rate_index++;
							}
							break;
						case '2':
							if(vuse>0)
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%d",2300+(voff+(int)vuse));
							}
							else
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%d",0);
							}
							break;
						case '5':
							if(vuse>0)
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%d",4800+5*(voff+(int)vuse));
							}
							else
							{
								nc=SformatOutput(&buffer[lc],max-lc-1,"%d",0);
							}
							break;
						case 'd':
							iuse=(int)vuse;
							if(vuse&0x80)
							{
								iuse=((int)vuse)|0xffffff00;
							}
							else
							{
								iuse=(int)vuse;
							}
							nc=SformatOutput(&buffer[lc],max-lc-1,"%+d",voff+iuse);
							break;
						case 'u':
							nc=SformatOutput(&buffer[lc],max-lc-1,"%u",(voff+(unsigned int)vuse)&0xff);
							break;
						case 'c':
							nc=SformatOutput(&buffer[lc],max-lc-1,"%1c",(voff+(unsigned int)vuse)&0xff);
							break;
						default:
						case 'x':
							nc=SformatOutput(&buffer[lc],max-lc-1,"0x%02x",(voff+(unsigned int)vuse)&0xff);
							break;
					}
					if(nc>0)
					{
						lc+=nc;
					}
					if(it<length-1)
					{
						nc=SformatOutput(&buffer[lc],max-lc-1,",");
						if(nc>0)
						{
							lc+=nc;
						}
					}
				}
			}
			break;
		case 2:
			vs=(short *)data;
			for(it=0; it<length; it++)
			{
				vuse=vs[it]&0xffff;
				if(high>=0 && low>=0)
				{
					mask=Mask(low,high);
					vuse=(vuse&mask)>>low;
				}
				switch(type)
				{
                                                case 'p':
                                                        {
                                                                nc=SformatOutput(&buffer[lc],max-lc-1,"%.3lf",0.125*(voff+(int)(vuse&0xffff)));
                                                        }
                                                        break;

		            case 'd':
						iuse=(int)vuse;
						if(vuse&0x8000)
						{
							iuse=((int)vuse)|0xffff0000;
						}
						else
						{
							iuse=(int)vuse;
						}
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%+d",voff+iuse);
						break;
		            case 'u':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%u",(voff+(unsigned int)vuse)&0xffff);
						break;
		            case 'c':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%2c",(voff+(unsigned int)vuse)&0xff,((voff+(unsigned int)vuse)>>8)&0xff);
						break;
					default:
		            case 'x':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"0x%04x",(voff+(unsigned int)vuse)&0xffff);
						break;
				}
				if(nc>0)
				{
					lc+=nc;
				}
				if(it<length-1)
				{
			        nc=SformatOutput(&buffer[lc],max-lc-1,",");
					if(nc>0)
					{
						lc+=nc;
					}
				}
			}
			break;
		case 4:
			vl=(int *)data;
			vf=(float *)data;
			for(it=0; it<length; it++)
			{
				vuse=vl[it]&0xffffffff;
				if(high>=0 && low>=0)
				{
					mask=Mask(low,high);
					vuse=(vuse&mask)>>low;
				}
				switch(type)
				{
		            case 'd':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%+d",voff+vuse);
						break;
		            case 'u':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%u",voff+(vuse&0xffffffff));
						break;
		            case 'f':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%g",voff+vf[it]);
						break;
		            case 'c':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"%4c",(voff+vuse)&0xff,((voff+(unsigned int)vuse)>>8)&0xff,((voff+(unsigned int)vuse)>>16)&0xff,((voff+(unsigned int)vuse)>>24)&0xff);
						break;
					default:
		            case 'x':
			            nc=SformatOutput(&buffer[lc],max-lc-1,"0x%08x",(voff+vuse)&0xffffffff);
						break;
				}
				if(nc>0)
				{
					lc+=nc;
				}
				if(it<length-1)
				{
			        nc=SformatOutput(&buffer[lc],max-lc-1,",");
					if(nc>0)
					{
						lc+=nc;
					}
				}
			}
			break;
		default:
			vc=(char *)data;
			if(type=='t')
			{
				//
				// make sure there is a null
				//
				if(size>MPRINTBUFFER)
				{
					size=MPRINTBUFFER;
				}
				strncpy(text,vc,size);
				text[size]=0;
				nc=SformatOutput(&buffer[lc],max-lc-1,"%s",text);
				if(nc>0)
				{
					lc+=nc;
				}
			}
			else if(type=='m')
			{
				nc=SformatOutput(&buffer[lc],max-lc-1,"%02x:%02x:%02x:%02x:%02x:%02x",
					vc[0]&0xff,vc[1]&0xff,vc[2]&0xff,vc[3]&0xff,vc[4]&0xff,vc[5]&0xff);
				if(nc>0)
				{
					lc+=nc;
				}
			}
			else
			{
				for(it=0; it<length; it++)
				{
					vuse=vc[it]&0xff;
					nc=SformatOutput(&buffer[lc],max-lc-1,"0x%02x",(voff+(unsigned int)vuse)&0xff);
					if(nc>0)
					{
						lc+=nc;
					}
				}
			}
			break;
    }
	return lc;
}


void Qc9KEepromPrintEntry(void (*print)(char *format, ...), 
	char *name, int offset, int size, int high, int low, int voff,
	int nx, int ny, int nz, int interleave,int interleave2, int interleave3,
	char type, QC98XX_EEPROM *mptr, int mcount, int all)
{
	int im;
	int lc, nc;
	char buffer[MPRINTBUFFER],fullname[MPRINTBUFFER];
	int it=0;//, iuse;
	int different;
	int length;
	int ix, iy, iz;
	int ii,jj,kk;
	int offset_x;
	int offset_y;
	int offset_z;
	static int TargetPowerBase2x[WHAL_NUM_11A_20_TARGET_POWER_CHANS][WHAL_NUM_STREAMS];
	static int TargetPowerBaseExt[QC98XX_EXT_TARGET_POWER_SIZE_5G];
	int streamXminus1=0;
	int extra=0;
	int mcs_0_10_20_offset;
	int mcs_0_10_20_low;
	int mcs_0_10_20_high;
	int mcs_0_10_20_extra;
	int mcs_1_2_11_12_21_22_offset;
	int mcs_1_2_11_12_21_22_low;
	int mcs_1_2_11_12_21_22_high;
	int mcs_1_2_11_12_21_22_extra;
	int mcs_3_4_13_14_23_24_offset;
	int mcs_3_4_13_14_23_24_low;
	int mcs_3_4_13_14_23_24_high;
	int mcs_3_4_13_14_23_24_extra;

	length=nx*ny*nz;
	for (ii=0;ii<nx;ii++)
	{
		if (type == 'q'){
			SformatOutput(fullname,MPRINTBUFFER-1,"%s[%d]",name,ii);
		}
		offset_x=offset+ii*interleave;
		for (jj=0;jj<ny;jj++)
		{
			offset_y=offset_x+jj*interleave2;
			for (kk=0;kk<nz;kk++)
			{
				offset_z=offset_y+kk*interleave3*size;
#ifdef WRONG
				iy=it%(nx*ny);
				iz=it/(nx*ny);
				ix=iy%nx;
				iy=iy/nx;
#else
				iz=it%nz;
				iy=it/nz;
				ix=iy/ny;
				iy=iy%ny;
#endif
				if (type == '9'){
					TargetPowerBase2x[ii][jj]=*(((unsigned char *)&mptr[0])+offset_z);
					continue;
				}	
				if (type == '8'){
					TargetPowerBaseExt[ii]=*(((unsigned char *)&mptr[0])+offset_z);
					continue;
				}
				if(nz>1)
				{
					SformatOutput(fullname,MPRINTBUFFER-1,"%s[%d][%d][%d]",name,ix,iy,iz);
				}
				else if(ny>1)
				{
					if (type == 'q'){
					//no op
					}else{
						SformatOutput(fullname,MPRINTBUFFER-1,"%s[%d][%d]",name,ix,iy);
					}
				}
				else if(nx>1)
				{
					SformatOutput(fullname,MPRINTBUFFER-1,"%s[%d]",name,ix);
				}
				else
				{
					SformatOutput(fullname,MPRINTBUFFER-1,"%s",name);
				}
				fullname[MPRINTBUFFER-1]=0;
				lc=0;
				if(type == 'q'){
					low=0+4*(jj%2);
					high=3+4*(jj%2);
					if(jj==17){
						nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"|ecv|%s|%d|%d|%d|%d|%d|%d|%d|%d|%c|",
						fullname,it,offset_z,size,high,low,nx,ny,nz,type);
					}
				}
				else{
					nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"|ecv|%s|%d|%d|%d|%d|%d|%d|%d|%d|%c|",
					fullname,it,offset_z,size,high,low,nx,ny,nz,type);
				}
				if(nc>0)
				{
					lc+=nc;
				}
				//
				// put value from mptr[0]
				//
				if(type == 'q'){
					if (jj<8)
						streamXminus1=0;
					else if (jj>12)
						streamXminus1=2;
					else 
						streamXminus1=1;
					extra=0;
					if(voff==10){//voff 10 is for 2G HT40; 
						if((TargetPowerBaseExt[(((WHAL_NUM_11G_20_TARGET_POWER_CHANS+ii)*WHAL_NUM_VHT_TARGET_POWER_RATES)+jj)/8]>>((((WHAL_NUM_11G_20_TARGET_POWER_CHANS+ii)*WHAL_NUM_VHT_TARGET_POWER_RATES)+jj)%8))&0x1){
						extra=16;
						}
					}
					else{
						if((TargetPowerBaseExt[(((voff*WHAL_NUM_11A_20_TARGET_POWER_CHANS+ii)*WHAL_NUM_VHT_TARGET_POWER_RATES)+jj)/8]>>((((voff*WHAL_NUM_11A_20_TARGET_POWER_CHANS+ii)*WHAL_NUM_VHT_TARGET_POWER_RATES)+jj)%8))&0x1){
							extra=16;
						}
					}
					nc=Qc9KEepromPrintIt(((unsigned char *)&mptr[0])+offset_z-jj+jj/2, type, size, 1, high, low, TargetPowerBase2x[ii][streamXminus1]+extra, &buffer[lc], MPRINTBUFFER-lc-1);
					if(jj==0){
						mcs_0_10_20_offset=offset_z-jj+jj/2;
						mcs_0_10_20_low=low;
						mcs_0_10_20_high=high;
						mcs_0_10_20_extra=extra;
					}
					if(jj==1){
						mcs_1_2_11_12_21_22_offset=offset_z-jj+jj/2;
						mcs_1_2_11_12_21_22_low=low;
						mcs_1_2_11_12_21_22_high=high;
						mcs_1_2_11_12_21_22_extra=extra;
					}
					if(jj==2){
						mcs_3_4_13_14_23_24_offset=offset_z-jj+jj/2;
						mcs_3_4_13_14_23_24_low=low;
						mcs_3_4_13_14_23_24_high=high;
						mcs_3_4_13_14_23_24_extra=extra;
					}
				}else{
					nc=Qc9KEepromPrintIt(((unsigned char *)&mptr[0])+offset_z, type, size, 1, high, low, voff, &buffer[lc], MPRINTBUFFER-lc-1);
				}
				if (type != 'q'){
					if(nc>0)
					{
						lc+=nc;
					}
					nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"|");
					if(nc>0)
					{
						lc+=nc;
					}
					//
					// loop over subsequent iterations
					// add value only if different than previous value
					//
					different=0;
					for(im=1; im<mcount; im++)
					{
						if(memcmp(((unsigned char *)&mptr[im-1])+offset_z,((unsigned char *)&mptr[im])+offset_z,size)!=0)
						{
							nc=Qc9KEepromPrintIt(((unsigned char *)&mptr[im])+offset_z, type, size, 1, high, low, voff, &buffer[lc], MPRINTBUFFER-lc-1);
							if(nc>0)
							{
								lc+=nc;
							}
							different++;
						}
						else
						{
							nc=SformatOutput(&buffer[lc], MPRINTBUFFER-lc-1,".");
							if(nc>0)
							{
								lc+=nc;
							}
						}
						nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"|");
						if(nc>0)
						{
							lc+=nc;
						}
					}
					//
					// fill in up to the maximum number of blocks
					//
					for( ; im<MBLOCK; im++)
					{
						nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"|");
						if(nc>0)
						{
							lc+=nc;
						}
					}
				}
				//
				// print it
				//
				
				if(type != 'q'){
					if(different>0 || all)
					{
						(*print)("%s",buffer);
					}
				}else if(jj==17){
					if(different>0 || all)
					{
						(*print)("%s",buffer);
					}
				}
				if(type == 'q'){
					if(jj==7)
					{
						//add 2 stream rates 10,11_12,13_14
				                nc=Qc9KEepromPrintIt(((unsigned char *)&mptr[0])+mcs_0_10_20_offset, type, size, 1, mcs_0_10_20_high, mcs_0_10_20_low, TargetPowerBase2x[ii][1]+mcs_0_10_20_extra, &buffer[lc], MPRINTBUFFER-lc-1);
				                nc=Qc9KEepromPrintIt(((unsigned char *)&mptr[0])+mcs_1_2_11_12_21_22_offset, type, size, 1, mcs_1_2_11_12_21_22_high, mcs_1_2_11_12_21_22_low, TargetPowerBase2x[ii][1]+mcs_1_2_11_12_21_22_extra, &buffer[lc], MPRINTBUFFER-lc-1);
				                nc=Qc9KEepromPrintIt(((unsigned char *)&mptr[0])+mcs_3_4_13_14_23_24_offset, type, size, 1, mcs_3_4_13_14_23_24_high, mcs_3_4_13_14_23_24_low, TargetPowerBase2x[ii][1]+mcs_3_4_13_14_23_24_extra, &buffer[lc], MPRINTBUFFER-lc-1);
					}
					if(jj==12)
					{	
						//add 3 stream rates 20,21_22,23_24
				                nc=Qc9KEepromPrintIt(((unsigned char *)&mptr[0])+mcs_0_10_20_offset, type, size, 1, mcs_0_10_20_high, mcs_0_10_20_low, TargetPowerBase2x[ii][2]+mcs_0_10_20_extra, &buffer[lc], MPRINTBUFFER-lc-1);
				                nc=Qc9KEepromPrintIt(((unsigned char *)&mptr[0])+mcs_1_2_11_12_21_22_offset, type, size, 1, mcs_1_2_11_12_21_22_high, mcs_1_2_11_12_21_22_low, TargetPowerBase2x[ii][2]+mcs_1_2_11_12_21_22_extra, &buffer[lc], MPRINTBUFFER-lc-1);
				                nc=Qc9KEepromPrintIt(((unsigned char *)&mptr[0])+mcs_3_4_13_14_23_24_offset, type, size, 1, mcs_3_4_13_14_23_24_high, mcs_3_4_13_14_23_24_low, TargetPowerBase2x[ii][2]+mcs_3_4_13_14_23_24_extra, &buffer[lc], MPRINTBUFFER-lc-1);
					
					}
				}
				it++;
			}
		}
	}
}

void Qc9KEepromDifferenceAnalyze_List(void (*print)(char *format, ...), QC98XX_EEPROM *mptr, int mcount, int all,
										_EepromPrintStruct *_EepromList, int nt, int checkUnknown)
{
	//int msize;
	int offset;
	//int length;
	//int lc, nc;
	//char buffer[MPRINTBUFFER];
	int it;
	int io;
        int interleave,interleave2,interleave3;

	offset=0;
	for(it=0; it<nt; it++)
	{
		//
		// first we do any bytes that are not associated with a field name
		//
		if (checkUnknown) {
			for(io=offset; io<_EepromList[it].offset; io++)
			{
				//Qc9KEepromPrintEntry(print, "unknown", io, 1, -1, -1, 0, 1, 1, 1, 1,1,1, 'x', mptr, mcount, all);
			}
		}
		//
		// do the field
		//
	        interleave=_EepromList[it].interleave%100;
	        interleave2=((int)(_EepromList[it].interleave/100))%100;
	        interleave3=(int)(_EepromList[it].interleave/10000);
	        Qc9KEepromPrintEntry(print,
			_EepromList[it].name, _EepromList[it].offset, _EepromList[it].size, _EepromList[it].high, _EepromList[it].low, _EepromList[it].voff,
			_EepromList[it].nx, _EepromList[it].ny, _EepromList[it].nz, interleave,interleave2,interleave3,
			_EepromList[it].type, 
			mptr, mcount, all);
	        if(_EepromList[it].interleave==1 || (it<nt-1 && _EepromList[it].interleave!=_EepromList[it+1].interleave))
		{
			offset=_EepromList[it].offset+
				(_EepromList[it].size*_EepromList[it].nx*_EepromList[it].ny*_EepromList[it].nz*_EepromList[it].interleave)-
				(_EepromList[it].interleave-1); 
		}
		else
		{
			offset=_EepromList[it].offset+_EepromList[it].size; 
		}
	}
	//
	// do any trailing bytes not associated with a field name
	//
	if (checkUnknown) {
		for(io=offset; io<sizeof(QC98XX_EEPROM); io++)
		{
			//Qc9KEepromPrintEntry(print, "unknown", io, 1, -1, -1, 0, 1, 1, 1, 1,1,1, 'x', mptr, mcount, all);
		}
	}
}

void Qc9KEepromDifferenceAnalyze(void (*print)(char *format, ...), QC98XX_EEPROM *mptr, int mcount, int all)
{
        int im;
        int lc, nc;
        char buffer[MPRINTBUFFER];
        int nt;

    //
        // make header
        //
        lc=SformatOutput(buffer,MPRINTBUFFER-1,"|ecv|name|index|offset|size|high|low|nx|ny|nz|type|");
        //
        // fill in up to the maximum number of blocks
        //
        for(im=0; im<MBLOCK; im++)
        {
                nc=SformatOutput(&buffer[lc],MPRINTBUFFER-lc-1,"b%d|",im);
                if(nc>0)
                {
                        lc+=nc;
                }
        }
        (*print)("%s",buffer);

        nt=sizeof(_Qc9KEepromList)/sizeof(_Qc9KEepromList[0]);
        Qc9KEepromDifferenceAnalyze_List(print, mptr, mcount, all, _Qc9KEepromList, nt, 1);

        //Qc9KEepromDifferenceAnalyze_newItems(print, mptr, mcount, all);

}

