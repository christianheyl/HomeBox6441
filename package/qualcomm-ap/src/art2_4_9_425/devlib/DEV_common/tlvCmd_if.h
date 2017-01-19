#ifndef _TLVCMD_IF_H_
#define _TLVCMD_IF_H_

extern int art_initF2();
extern void art_teardownDevice();
extern int art_whalResetDevice(A_UCHAR *mac, A_UCHAR *bss, A_UINT32 freq, A_INT32 bandwidth, A_UINT16 rxchain, A_UINT16 txchain);

extern A_UINT32 art_regRead(A_UINT32 regOffset);
extern int art_regWrite(A_UINT32 regOffset, A_UINT32 regValue);
extern A_UINT32 art_mem32Read(A_UINT32 regAddr);
extern A_UINT32 art_mem32Write(A_UINT32 regAddr, A_UINT32 regValue);
extern int art_memRead(A_UINT32 physAddr, A_UCHAR  *bytesRead, A_UINT32 length);
extern int art_memWrite (A_UINT32 physAddr, A_UCHAR  *buf, A_UINT32 length);
//extern A_UINT32 art_memWrite(A_UINT32 physAddr, A_UCHAR  *bytesWrite, A_UINT32 length);

extern A_UINT32 art_cfgRead(A_UINT32 regOffset);
extern int art_cfgWrite(A_UINT32 regOffset, A_UINT32 regValue);

extern int art_eepromWriteItems(unsigned int numOfItems, unsigned char *pBuffer, unsigned int length);
extern int art_stickyWrite(int numOfRegs, unsigned char *pBuffer, unsigned int length);
extern int art_stickyClear(int numOfRegs, unsigned char *pBuffer, unsigned int length);

extern int art_otpWrite(A_UCHAR *buf, A_UINT32 length);
extern int art_otpRead(A_UCHAR  *buf, A_UINT32 *length);
extern int art_otpReset(enum otpstream_op_app resetCmd);
extern int art_efuseRead(A_UCHAR *buf, A_UINT32 *length, A_UINT32 startPos);
extern int art_efuseWrite(A_UCHAR *buf, A_UINT32 length, A_UINT32 startPos);
extern int art_otpLoad(A_UINT32 value);

extern int art_tlvSend2( void* tlvStr, int tlvStrLen, unsigned char *respdata, unsigned int *nrespdata );

extern int art_txDataStart(TX_DATA_START_PARAMS *txDataStartParams);
extern int art_txDataStop(void **txStatus, int calibrate);
extern int art_txStatusReport(void **txStatus, int stop);

extern int art_rxDataStart (RX_DATA_START_PARAMS *Params);
extern int art_rxDataStop(void **rxStatus);
extern int art_rxStatusReport(void **rxStatus, int stop);

extern int art_sleepMode (int mode);
extern int art_getDeviceHandle (unsigned int *handle);

extern A_UINT32 art_readPciConfigSpace(A_UINT32 offset);
extern int art_writePciConfigSpace(A_UINT32 offset, A_UINT32 value);

#endif //_TLVCMD_IF_H_
