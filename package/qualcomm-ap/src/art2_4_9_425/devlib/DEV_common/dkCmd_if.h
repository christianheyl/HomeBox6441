#ifndef _DKCMD_IF_H_
#define _DKCMD_IF_H_

#ifdef _WINDOWS
extern A_INT32 art_openSocket();
#endif //_WINDOWS

extern A_INT32 art_mdkErrNo;
extern A_CHAR art_mdkErrStr[SIZE_ERROR_BUFFER];

extern A_INT32  art_setupDevice(A_UINT32 whichDevice);

//extern A_INT32 art_initF2(A_UINT32 whichDevice, DK_DEV_INFO **pdkInfo);
extern A_INT32 art_initF2(A_UINT32 whichDevice);
extern A_UINT32 art_createEvent(A_UINT32 devNum, A_UINT32 type, A_UINT32 persistent, 
                         A_UINT32 param1, A_UINT32 param2, A_UINT32 param3);
extern A_UINT32 art_getISREvent(A_UINT32 devNum, EVENT_STRUCT *ppEvent);

extern A_UINT32 art_cfgRead(A_UINT32 devNum, A_UINT32 regOffset);

extern A_UINT32 art_regRead(A_UINT32 devNum, A_UINT32 regOffset);

extern A_UINT32 art_regWrite(A_UINT32 devNum, A_UINT32 regOffset, A_UINT32 regValue);

extern A_UINT32 art_mem32Read(A_UINT32 devNum, A_UINT32 regAddr);

extern A_UINT32 art_mem32Write(A_UINT32 devNum, A_UINT32 regAddr, A_UINT32 regValue);

extern A_UINT32 art_memRead(A_UINT32 devNum, A_UINT32 physAddr, A_UCHAR  *bytesRead, A_UINT32 length);

extern A_UINT32 art_memWrite(A_UINT32 devNum, A_UINT32 physAddr, A_UCHAR  *bytesWrite, A_UINT32 length);

extern A_UINT32 art_cfgWrite(A_UINT32 devNum, A_UINT32 regOffset, A_UINT32 regValue);

//extern void art_setMoreResetParams(A_UINT32 devNum, MORE_RESET_PARMS *moreResetParms);

extern void art_setResetParams(A_UINT32 devNum, A_CHAR *pFilename, A_BOOL eePromLoad, A_BOOL eePromHeaderLoad,
                        A_UCHAR mode, A_UINT16 initCodeFlag);

extern A_UINT32 art_mini_resetDevice(A_UINT32 devNum, A_UCHAR *mac, A_UCHAR *bss, A_UINT32 freq, A_UINT32 turbo, A_BOOL hwRst);

extern void art_getDeviceInfo(A_UINT32 devNum, SUB_DEV_INFO *devStruct);

extern A_UINT32 art_eepromRead(A_UINT32 devNum, A_UINT32 eepromOffset);

extern void art_eepromWrite(A_UINT32 devNum, A_UINT32 eepromOffset, A_UINT32 eepromValue);

extern void art_eepromReadBlock(A_UINT32 devNum, A_UINT32 startOffset,	A_UINT32 length, A_UINT32 *buf);

extern void art_eepromReadLocs(A_UINT32 devNum, A_UINT32 startOffset, A_UINT32 length, A_UINT8 *buf);

extern void art_eepromWriteBlock(A_UINT32 devNum, A_UINT32 startOffset, A_UINT32 length, A_UINT32 *buf);

extern void art_eepromWriteByteBasedBlock(A_UINT32 devNum,	A_UINT32 startOffset, A_UINT32 length, A_UINT8 *buf);

extern A_UINT32 art_checkRegs(A_UINT32 devNum);

extern A_UINT32 art_checkProm(A_UINT32 devNum,	A_UINT32 enablePrint);

extern void art_rereadProm(A_UINT32 devNum);

extern void art_changeChannel(A_UINT32 devNum,	A_UINT32 freq);

extern void art_ar5416ChannelChangeTx(A_UINT32 devNum, A_UINT32 freq);

extern A_UINT32 art_loadSwitchTableParams(A_UINT32 devNum,	A_UINT32 ant);

extern void art_txDataSetup(A_UINT32 devNum, A_UINT32 rateMask, A_UINT32 rateMaskMcs20, A_UINT32 rateMaskMcs40, A_UCHAR *dest,
                     A_UINT32 numDescPerRate, A_UINT32 dataBodyLength, A_UCHAR *dataPattern, A_UINT32 dataPatternLength,
                     A_UINT32 retries, A_UINT32 antenna, A_UINT32 broadcast);

extern void art_txDataAggSetup(A_UINT32 devNum, A_UINT32 rateMask,	A_UCHAR *dest, A_UINT32 numDescPerRate,
                        A_UINT32 dataBodyLength, A_UCHAR *dataPattern, A_UINT32 dataPatternLength,
                        A_UINT32 retries, A_UINT32 antenna,	A_UINT32 broadcast, A_UINT32 aggSize);

extern void art_txDataSetupNoEndPacket(A_UINT32 devNum, A_UINT32 rateMask,	A_UCHAR *dest, A_UINT32 numDescPerRate,
                                A_UINT32 dataBodyLength, A_UCHAR *dataPattern, A_UINT32 dataPatternLength, 
                                A_UINT32 retries, A_UINT32 antenna, A_UINT32 broadcast);

extern void art_txDataBegin(A_UINT32 devNum, A_UINT32 timeout,	A_UINT32 remoteStats);

extern void art_txDataStart(TX_DATA_START_PARAMS *txDataStartParams);

extern void art_rxDataSetup(A_UINT32 devNum, A_UINT32 numDesc,	A_UINT32 dataBodyLength, A_UINT32 enablePPM);

extern void art_rxDataAggSetup(A_UINT32 devNum, A_UINT32 numDesc, A_UINT32 dataBodyLength,	A_UINT32 enablePPM, A_UINT32 aggSize);

extern void art_cleanupTxRxMemory(A_UINT32 devNum, A_UINT32 flags);

extern void art_rxDataBegin(A_UINT32 devNum, A_UINT32 waitTime, A_UINT32 timeout, A_UINT32 remoteStats,
                     A_UINT32 enableCompare, A_UCHAR *dataPattern, A_UINT32 dataPatternLength);

// works with Signal Generators
extern void art_rxDataBeginSG
(
        A_UINT32 devNum,
        A_UINT32 waitTime,
        A_UINT32 timeout,
        A_UINT32 remoteStats,
        A_UINT32 enableCompare,
        A_UCHAR *dataPattern,
        A_UINT32 dataPatternLength,
        A_UINT32 sgpacketnumber
);


extern void art_rxGetData
(
 A_UINT32 devNum, 
 A_UINT32 bufferNum, 
 A_UCHAR *pReturnBuffer, 
 A_UINT32 sizeBuffer
);

extern void art_rxDataComplete
(
	A_UINT32 devNum,
	A_UINT32 waitTime,
	A_UINT32 timeout,
	A_UINT32 remoteStats,
	A_UINT32 enableCompare,
	A_UCHAR *dataPattern, 
	A_UINT32 dataPatternLength
);

void art_rxDataStart 
(
    RX_DATA_START_PARAMS *Params
);

extern void art_txrxDataBegin
(
	A_UINT32 devNum,
	A_UINT32 waitTime,
	A_UINT32 timeout,
	A_UINT32 remoteStats,
	A_UINT32 enableCompare,
	A_UCHAR *dataPattern, 
	A_UINT32 dataPatternLength
);

A_BOOL art_rxLastDescStatsSnapshot
(
 A_UINT32 devNum, 
 RX_STATS_SNAPSHOT *pRxStats
);

extern void art_setAntenna
(
	A_UINT32 devNum,
	A_UINT32 antenna
);

extern A_UINT16 art_getMaxPowerForRate
(
 A_UINT32 devNum,
 A_UINT16 freq,
 A_UINT16 rate
);

extern A_UINT32 art_getPhaseCal
(
 A_UINT32 devNum,
 A_UINT32 freq
);

extern A_UINT16 art_getPcdacForPower
(
 A_UINT32 devNum,
 A_UINT16 freq,
 A_INT16 power
);

extern void art_txContBegin
(
	A_UINT32 devNum,
	A_UINT32 type,
	A_UINT32 typeOption1,
	A_UINT32 typeOption2,
	A_UINT32 antenna
);

extern void art_txContFrameBegin
(
	A_UINT32 devNum,
	A_UINT32 length,
	A_UINT32 ifswait,
	A_UINT32 typeOption1,
	A_UINT32 typeOption2,
	A_UINT32 antenna,
	A_BOOL   performStabilizePower,
	A_UINT32 numDescriptors,
	A_UCHAR *dest
);

extern void art_txContEnd
(
	A_UINT32 devNum
);

extern void art_tempComp
(
	A_UINT32 devNum
);

extern void art_setSingleTransmitPower
(
 A_UINT32 devNum,
 A_UCHAR pcdac
);


extern void art_getField
(
	A_UINT32 devNum,
	A_CHAR   *fieldName,
	A_UINT32 *baseValue,
	A_UINT32 *turboValue
);

extern void art_changeField
(
	A_UINT32 devNum,
	A_CHAR *fieldName, 
	A_UINT32 newValue
);

extern void art_writeField
(
	A_UINT32 devNum,
	A_CHAR *fieldName, 
	A_UINT32 newValue
);

extern void art_teardownDevice
(
	A_UINT32 devNum
);

A_BOOL art_testLib
(
 A_UINT32 devNum,
 A_UINT32 timeout
);

extern void art_ForceSinglePCDACTableAnalog
(
	A_UINT32 devNum, 
	A_UINT16 pcdac,
        A_BOOL   turnOn
);

extern void art_ForceSinglePCDACTable
(
	A_UINT32 devNum, 
	A_UINT16 pcdac
);

extern void art_ForceSinglePCDACTableGriffin
(
	A_UINT32 devNum, 
	A_UINT16 pcdac,
	A_UINT16 offset
);

extern void art_ForcePCDACTable
(
	A_UINT32 devNum, 
	A_UINT16 *pcdac
);

extern void art_specifySubSystemID
(
 A_UINT32 devNum,
 A_INT16  subsystemID
);

extern void art_setupOlpcCAL(A_UINT32 devNum);

extern void art_forcePowerTxMax 
(
	A_UINT32		devNum,
	A_UINT16		*pRatesPower
);

extern void art_forceSinglePowerTxMax 
(
	A_UINT32		devNum,
	A_UINT16		powerValue
);

extern A_UINT16 art_GetEepromStruct
(
 A_UINT32 devNum,
 A_UINT16 eepStructFlag,	//which eeprom strcut
 void **ppReturnStruct		//return ptr to struct asked for
);

extern void art_writeNewProdData
(
 A_UINT32 devNum,
 A_INT32  *argList,
 A_UINT32 numArgs
);

extern void art_writeProdData
(
 A_UINT32 devNum,
 A_UCHAR wlan0Mac[6],
 A_UCHAR wlan1Mac[6],
 A_UCHAR enet0Mac[6],
 A_UCHAR enet1Mac[6]
);

A_BOOL art_ftpDownloadFile
(
 A_UINT32 devNum,
 A_CHAR *hostname,
 A_CHAR *user,
 A_CHAR *password,
 A_CHAR *remoteFile,
 A_CHAR *localFile
);


A_BOOL checkLibError
(
	 A_UINT32 devNum,
     A_BOOL	printError 
);

extern void art_getLastErrorStr
(
 A_CHAR *pStrBuffer
);

A_INT32 art_getFieldForMode
(
 A_UINT32 devNum,
 A_CHAR   *fieldName,
 A_UINT32  mode,			//desired mode 
 A_UINT32  turbo		//Flag for base or turbo value
);

A_INT32 art_getFieldForModeChecked
(
 A_UINT32 devNum,
 A_CHAR   *fieldName,
 A_UINT32  mode,			//desired mode 
 A_UINT32  turbo		//Flag for base or turbo value
);

extern void art_changeMultipleFieldsAllModes
(
 A_UINT32		  devNum,
 PARSE_MODE_INFO *pFieldsToChange,
 A_UINT32		  numFields
);

extern void art_changeMultipleFields
(
 A_UINT32		  devNum,
 PARSE_FIELD_INFO *pFieldsToChange,
 A_UINT32		  numFields
);

extern void art_changeAddacField
(
 A_UINT32		  devNum,
 PARSE_FIELD_INFO *pFieldToChange
);

extern void art_saveXpaBiasLvlFreq
( 
    A_UINT32		  devNum,
    PARSE_FIELD_INFO *pFieldToChange,
    A_CHAR *level
);

A_INT16 art_GetMacAddr
(
	A_UINT32 devNum,
	A_UINT16 wmac,
	A_UINT16 instNo,
	A_UINT8	*macAddr
);

A_BOOL selectPrimary
(
 void
);

A_BOOL selectSecondary
(
 void
);

A_BOOL activateCommsInitHandshake
(
 A_CHAR *machName
);

A_BOOL art_waitForGenericCmd
(
 void *pSock,
 A_UCHAR   *pStringVar,
 A_UINT32  *pIntVar1,
 A_UINT32  *pIntVar2,
 A_UINT32  *pIntVar3
);

A_BOOL art_sendGenericCmd
(
 A_UINT32 devNum,
 A_CHAR *stringVar,
 A_INT32 intVar1,
 A_INT32 intVar2,
 A_INT32 intVar3
);

A_BOOL waitCommsInitHandshake
(
 void
);

void closeComms
(
 void
);


extern void art_enableHwCal
(
	 A_UINT32 devNum,
     A_UINT32 calFlag
);

extern void art_supplyFalseDetectbackoff
(
	A_UINT32 devNum,
	A_UINT32 *pBackoffValues
);

extern A_UINT16 art_getXpdgainForPower
(
	A_UINT32 devNum,
	A_INT16  power
);

extern A_UINT16 art_getPowerIndex
(
	A_UINT32 devNum,
	A_INT32  power   // 2 x power in dB
);

A_BOOL	 art_getCtlPowerInfo
(
 A_UINT32 devNum,
 CTL_POWER_INFO *pReturnStruct //pointer to structure to fill
);

extern A_UINT32 art_getEndianMode
(
	A_UINT32 devNum
);

extern void art_ar5416SetGpio( 
    A_UINT32 devNum, 
    A_UINT32 uiGpio, 
    A_UINT32 uiVal );

extern A_UINT32 art_ar5416ReadGpio( 
    A_UINT32 devNum, 
    A_UINT32 uiGpio );

extern void art_generateTxGainTblFromCfg
(
    A_UINT32 devNum, 
    A_UINT32 modeMask
);

//extern void art_generateVenusTxGainTblFromCfg
//(
//    A_UINT32 devNum,
//    A_UINT32 modeMask
//);

extern void art_pushTxGainTbl(A_UINT32 devNum, A_UINT32 pcdac);
//A_UINT32 get_eeprom_size(A_UINT32 devNum,A_UINT32 *eepromSize, A_UINT32 *checkSumLength);
//A_BOOL eeprom_verify_checksum (A_UINT32 devNum);
A_UINT32 eeprom_get_checksum(A_UINT32 devNum, A_UINT16 startAddr, A_UINT32 numWords ) ;

extern A_UINT32 art_hwReset(A_UINT32 devNum, A_UINT32 rMask);
extern void art_pllProgram(A_UINT32 devNum, A_UINT32 turbo, A_UINT32 mode);
extern A_UINT32 art_calCheck (A_UINT32 devNum, A_UINT32 enableCal, A_UINT32 timeout );
extern void art_pciWrite(A_UINT32 devNum, PCI_VALUES *pPciValues, A_UINT32 length);
extern A_UINT32 art_ap_reg_read ( A_UINT32 devNum, A_UINT32 regAddr);
extern A_UINT32 art_ap_reg_write ( A_UINT32 devNum, A_UINT32 regAddr, A_UINT32 regValue);
A_UINT32 load_and_run_code( A_UINT32 devNum, A_UINT32 loadAddress, A_UINT32 totalBytes, A_UCHAR *loadBytes);
extern void art_fillTxStats ( A_UINT32 devNum, A_UINT32 descAddress, A_UINT32 numDesc, A_UINT32 dataBodyLen, A_UINT32 txTime, TX_STATS_STRUCT *txStats);
extern void art_createDescriptors(A_UINT32 devNumIndex, A_UINT32 descBaseAddress,  A_UINT32 descInfo, A_UINT32 bufAddrIncrement, A_UINT32 descOp, A_UINT32 descWords[MAX_DESC_WORDS]);
//extern A_UINT32 art_memAlloc (   A_UINT32 allocSize, A_UINT32 physAddr, A_UINT32 devNum  );
extern void art_memFree (   A_UINT32 fAddr, A_UINT32 devNum  );
extern void art_load_and_program_code( A_UINT32 devNum, A_UINT32 loadAddress, A_UINT32 totalBytes, A_UCHAR *loadBytes, A_BOOL calData);
extern void art_iq_calibration(A_UINT32 devNum, IQ_FACTOR *iq_coeff);
A_INT32 art_getRefClkSpeed(A_UINT32 devNum) ;
extern A_UINT32 art_whalResetDevice(A_UINT32 devNum, A_UCHAR *mac, A_UCHAR *bss, A_UINT32 freq, A_UINT32 turbo, A_UINT32 ht40, A_UINT16 rxchain, A_UINT16 txchain, A_UINT32 *rateMaskRow);
A_BOOL artSendCmd(PIPE_CMD *pCmdStruct,A_UINT32 cmdSize,void **returnCmdStruct);

#ifndef CUSTOMER_REL		  
extern A_UINT32 art_send_frame_and_recv(
   A_UINT32 devIndex, 
   A_UINT8 *pBuffer, 
   A_UINT32 tx_desc_ptr, 
   A_UINT32 tx_buf_ptr, 
   A_UINT32 rx_desc_ptr, 
   A_UINT32 rx_buf_ptr,
   A_UINT32 rate_code
);

extern A_UINT32 art_recv_frame_and_xmit(
   A_UINT32 devIndex, 
   A_UINT32 tx_desc_ptr, 
   A_UINT32 tx_buf_ptr, 
   A_UINT32 rx_desc_ptr, 
   A_UINT32 rx_buf_ptr,
   A_UINT32 rate_code
);
#endif

extern A_UINT32 art_otpWrite(A_UINT32 devNum, A_UCHAR *buf, A_UINT32 length);
A_BOOL art_otpRead(A_UINT32 devNum, A_UCHAR  *buf, A_UINT32 *length);
A_BOOL art_otpReset(A_UINT32 devNum, enum otpstream_op_app resetCmd);
A_BOOL art_efuseRead(A_UINT32 devNum, A_UCHAR *buf, A_UINT32 *length, A_UINT32 startPos);
A_BOOL art_efuseWrite(A_UINT32 devNum, A_UCHAR *buf, A_UINT32 length, A_UINT32 startPos);
int art_otpLoad(A_UINT32 value);

extern void art_sleepCmd ( A_UINT32 devNum, A_UINT32 sleep_enable);

extern int art_receiveDisable();
extern int art_receiveEnable();
extern int art_transmitDisable(unsigned long qmask);
extern int art_transmitEnable(unsigned long qmask);
extern int art_txDataStop(void **txStatus);
extern int art_rxDataStop(void **rxStatus);
extern int art_txStatusReport(void **txStatus, int stop);
extern int art_rxStatusReport(void **rxStatus, int stop);
extern int art_eepromWriteItems(unsigned int numOfItems, unsigned char *pBuffer, unsigned int length);
extern int art_stickyWrite(int numOfRegs, unsigned char *pBuffer, unsigned int length);
extern int art_stickyClear(int numOfRegs, unsigned char *pBuffer, unsigned int length);
extern int art_PsatCalResult(void **PsatCalResult, int chain);
int art_tlvSend(unsigned char *pBuffer, unsigned int length, unsigned char *pReturnBuffer, unsigned int *pReturnLength);

#endif //_DKCMD_IF_H_
