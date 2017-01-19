/*
 *  Copyright © 2005 Atheros Communications, Inc.,  All Rights Reserved.
 */
/* mdata5416.h - Type definitions needed for data transfer functions */

#ifndef	_MDATA5416_H
#define	_MDATA5416_H

//#ident  "ACI $Id: //depot/sw/branches/art2_main_per_cs/src/art2/devlib/ar5416/mData5416.h#1 $, $Header: //depot/sw/branches/art2_main_per_cs/src/art2/devlib/ar5416/mData5416.h#1 $"

#ifdef UNUSED
#ifdef Linux
#include "../mdata.h"
#else
#include "..\mdata.h"
#endif

void macAPIInitAr5416
(
 A_UINT32 devNum
);

A_UINT32 setupAntennaAr5416
(
 A_UINT32 devNum, 
 A_UINT32 antenna, 
 A_UINT32* antModePtr 
);

A_UINT32 sendTxEndPacketAr5416
(
 A_UINT32 devNum, 
 A_UINT16 queueIndex
);

void setDescriptorAr5416
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UINT32 descNum,
 A_UINT32 rateIndex,
 A_UINT32 broadcast
);

void setStatsPktDescAr5416
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 rateValue
);

void setContDescriptorAr5416
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UCHAR  dataRate
);

A_UINT32 txGetDescRateAr5416
(
 A_UINT32 devNum,
 A_UINT32 descAddr,
 A_BOOL   *ht40,
 A_BOOL   *shortGi
);

void txBeginConfigAr5416
(
 A_UINT32 devNum,
 A_BOOL	  enableInterrupt
);

void txBeginContDataAr5416
(
 A_UINT32 devNum,
 A_UINT16 queueIndex
);

void txBeginContFramedDataAr5416
(
 A_UINT32 devNum,
 A_UINT16 queueIndex,
 A_UINT32 ifswait,
 A_BOOL	  retries
);

void beginSendStatsPktAr5416
(
 A_UINT32 devNum, 
 A_UINT32 DescAddress
);

void writeRxDescriptorAr5416
( 
 A_UINT32 devNum, 
 A_UINT32 rxDescAddress
);

void rxBeginConfigAr5416
(
 A_UINT32 devNum
);

void rxCleanupConfigAr5416
(
 A_UINT32 devNum
);

void setPPM5416
(
 A_UINT32 devNum,
 A_UINT32 enablePPM
);

void setDescriptorEndPacketAr5416
(
 A_UINT32 devNum,
 MDK_ATHEROS_DESC*  localDescPtr,
 A_UINT32 pktSize,
 A_UINT32 antMode,
 A_UINT32 descNum,
 A_UINT32 rateIndex,
 A_UINT32 broadcast
);
#endif

extern int Ar5416ReceiveDescriptorPointer(unsigned int descriptor);
extern int Ar5416ReceiveUnicast(int on);
extern int Ar5416ReceiveBroadcast(int on);
extern int Ar5416ReceivePromiscuous(int on);
extern int Ar5416ReceiveEnable(void);
extern int Ar5416ReceiveDisable(void);

extern int Ar5416TransmitDescriptorPointer(int queue, unsigned int descriptor);
extern int Ar5416TransmitRetryLimit(int retry);
extern int Ar5416TransmitQueueSetup(int qcu, int dcu);
extern int Ar5416Deaf(int deaf);
extern int Ar5416TransmitRegularData(void);			// normal
extern int Ar5416TransmitFrameData(int ifs);	// tx99
extern int Ar5416TransmitContinuousData(void);		// tx100
extern int Ar5416TransmitCarrier(unsigned int tx_chain_mask);					// carrier only
extern int Ar5416TransmitEnable(unsigned int qmask);
extern int Ar5416TransmitDisable(unsigned int qmask);

extern int Ar5416DeviceSelect();

extern int    Ar5416BssIdSet(unsigned char *mac); 
extern int	  Ar5416StationIdSet(unsigned char *mac);
extern int    Ar5416ReceiveFifo(void);
extern int    Ar5416ReceiveDescriptorMaximum(void);
extern int    Ar5416ReceiveEnableFirst(void);
extern int    Ar5416TransmitFifo(void);
extern int    Ar5416TransmitDescriptorSplit(void);
extern int    Ar5416TransmitAggregateStatus(void);
extern int    Ar5416TransmitEnableFirst(void);

#endif	/* _MDATA5416_H */
