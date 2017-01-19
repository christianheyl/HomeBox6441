/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
#ifndef FDEBUG_H
#define FDEBUG_H

#define GW_DBG_ENABLE 0x01
#define GW_DBG_DISABLE 0x02
#define MODEM_DBG_RESET 0x04
#define MODEM_DBG_SEND 0x08


/*Flags to indicate debug pkt sent by proc*/
#define PROC_MODEM_DBG6_SEND 0x20
#define PROC_MODEM_DBG7_SEND 0x40
#define PROC_MODEM_DBG8_SEND 0x80

typedef enum
{
  DBG_SUCCESS,
  DBG_FAIL,
  DBG_PENDING
}dbg_oprn_status;
typedef enum
{
  DBG_READ_GW_STATS=1,
  DBG_READ_MODEM_STATS,
  DBG_READ_GW_VERSION,
  DBG_RESET_GW_STATS,
  DBG_RESET_MODEM_STATS,
  DBG_REQ_MODEM_STATS,
  DBG_GW_STATS_EN,
  DBG_GW_STATS_DIS
}dbg_oprns;
typedef struct
{
 unsigned char ucDrvMode;
 unsigned int uiInt;
 unsigned int uiTmpInt;
 unsigned int uiCdc;
 unsigned int uiTmpCdc;
 unsigned int uiLossRx;
 unsigned int uiLossTx;
 unsigned int uiUnLockFallEdge;
 unsigned int uiUnLockAtRxCB;
 unsigned int uiUnLockAtTxCB;
 unsigned int uiKpi;
 unsigned int uiInvSpi;
 unsigned int uiLastOprn;
 unsigned int uiLastOprnStatus;
 unsigned int auiIntLossSeq[10];
}gw_dbg_stats;
typedef struct
{
 unsigned int uiSpiTimeOutTxCC;/*Current counter for SPI Timeout Counter for Tx*/
 unsigned int uiSpiTimeOutRxCC;/*Current counter for SPI Timeout Counter for Rx*/
 unsigned int uiSpiTimeOutTxTC;/*Total counter for SPI Timeout Counter for Tx*/
 unsigned int uiSpiTimeOutRxTC;/*Total counter for SPI Timeout Counter for Rx*/
 unsigned int uiStackOverFlowCC;/*Current counter for stack overflow*/
 unsigned int uiStackOverFlowTC;/*Total counter for stack overflow*/
 unsigned int uiNonSrvcSpiIntCC;/*Current counter for Not Serviced SPI Interrupt */
 unsigned int uiNonSrvcSpiIntTC;/* Total counter for Not Serviced SPI Interrupt */
 unsigned int uiDummyBearerChangeCC;/*Current counter for Dummy bearer change */
 unsigned int uiDummyBearerChangeTC;/*Total counter for Dummy bearer change */
 unsigned int uiMaxTimeSscTimerA;/*Max Time betn SSC Services in Timer A increments */
}modem_dbg_stats;

typedef struct
{
	char cstatus;
	unsigned int uiBadAXCRC;/*Bad field A+X CRC counter */ 
	unsigned int uiQ1Q2BadACRC; /*Q1/Q2 Bad A field CRCcounter*/
	unsigned int uiBadZCRC; /*Bad Z field CRC counter  */
	unsigned int uiFailedHO; /*Failed Handovers */
	unsigned int uiSuccessHO; /*Successful Handovers */
	unsigned int uiRSSI; /*RSSI under threshold counter*/
	unsigned int uiSyncErr; /*Number of sync errors*/
}modem_dbg_lbn_stats;

typedef struct
{
	unsigned char ucDrvVersion[20];
	unsigned char ucCosicSwVersion[20];
	unsigned char ucBmcFwVersion[20];
}dect_version;
void DECODE_DEBUG_TO_MODULE(DECT_MODULE_DEBUG_INFO* data);
void DECODE_DEBUG_FROM_MODULE(DECT_MODULE_DEBUG_INFO *data);

#endif

