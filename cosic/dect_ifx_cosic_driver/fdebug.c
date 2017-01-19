/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
#ifndef FDEBUG_C
#define FDEBUG_C

#include <linux/interrupt.h>


#include "drv_dect.h"
#include "fdebug.h"
#include "MESSAGE_DEF.H"

void EnableGwDebugStats(void);
void DisableGwDebugStats(void);
void ResetGwDebugStats(void);
void GetModemDebugStats(void);
void ResetModemDebugStats(void);
void DecodeDbg8Pkt(unsigned char *buf);
void DecodeDbg7Pkt(unsigned char *buf);
void DecodeDbg6Pkt(unsigned char *buf);
unsigned int GetTwoBytes(unsigned char *buf);
void WriteDbgPkts(unsigned char debug_info_id, unsigned char rw_indicator, \
									unsigned char CurrentInc, unsigned char *G_PTR_buf);


extern unsigned int Max_spi_data_len_val;
unsigned int vuiDbgFlags;
/*Flag to remember who has sent this: proc file sytem or App. If a particular bit is set then 
that info id debug pkt is sent by proc input else from App*/
unsigned int vuiDbgSendFlags; 
atomic_t vuiGwDbgFlag;
dect_version version;
gw_dbg_stats gw_stats;
modem_dbg_stats modem_stats;
modem_dbg_lbn_stats lbn_stats[6];

void GetModemDebugStats(void)
{
	WriteDbgPkts(6,0,0,NULL);
	WriteDbgPkts(7,0,0,NULL);
	WriteDbgPkts(8,0,0,NULL);
	vuiDbgFlags |= MODEM_DBG_SEND;
	gw_stats.uiLastOprn = DBG_REQ_MODEM_STATS;
	gw_stats.uiLastOprnStatus = DBG_PENDING;
}
void ResetModemDebugStats(void)
{
	/*do dummy read from COSIC and ignore the status from modem; reset local status to zero*/
	memset(&modem_stats,0,sizeof(modem_stats));
	vuiDbgFlags |= MODEM_DBG_RESET;
	WriteDbgPkts(6,1,0,NULL);
	gw_stats.uiLastOprn = DBG_RESET_MODEM_STATS;
	gw_stats.uiLastOprnStatus = DBG_PENDING;
}
void ResetGwDebugStats(void)
{
	int flag=0;
	if(atomic_read(&vuiGwDbgFlag)==1)
	{
		atomic_set(&vuiGwDbgFlag,0);
		flag=1;
	}
	memset(&gw_stats,0,sizeof(gw_stats));
	if(flag)
	{
		atomic_set(&vuiGwDbgFlag,1);
	}
	gw_stats.uiLastOprn = DBG_RESET_GW_STATS;
	gw_stats.uiLastOprnStatus = DBG_SUCCESS;
}
void EnableGwDebugStats(void)
{
	memset(&gw_stats,0,sizeof(gw_stats));
	atomic_set(&vuiGwDbgFlag,1);
	gw_stats.uiLastOprn = DBG_GW_STATS_EN;
	gw_stats.uiLastOprnStatus = DBG_SUCCESS;
}
void DisableGwDebugStats(void)
{
	atomic_set(&vuiGwDbgFlag,0);
	memset(&gw_stats,0,sizeof(gw_stats));
	gw_stats.uiLastOprn = DBG_GW_STATS_DIS;
	gw_stats.uiLastOprnStatus = DBG_SUCCESS;
}
void DECODE_DEBUG_TO_MODULE(DECT_MODULE_DEBUG_INFO* data)
{
	unsigned char index = 0;


	switch(data->debug_info_id)
	{
		case BMC_REGISTER_READ_REQ:
			if(data->debug_info_id == BMC_REGISTER_READ_REQ && data->rw_indicator)
			{

				while( index < 42)
				{
					switch(data->G_PTR_buf[index])
					{
						case BMC_REGISTER:
							index += 13;
							break;

						case OSC_TRIMMING_VAL:
							index += 3;
							break;

						case GFSK_VALUE:
							index += 3;
							break;

						case RF_TEST_MODE_SET:
							index += 4;
							break;

						case SPI_SETUP_PACKET:
							{
							unsigned int temp_data_len = 0;
							unsigned long ilockflags;
							temp_data_len =data->G_PTR_buf[index+2];
							temp_data_len = temp_data_len << 8;
							temp_data_len &= 0xFF00;
							temp_data_len |= data->G_PTR_buf[index+3];

							local_irq_save(ilockflags);	 
							Max_spi_data_len_val = temp_data_len;		 // setting max spi length
							local_irq_restore(ilockflags);
							}
							index += 4;
							break;
						default:
							index++;
							break;
					}
				}

			}
			break;

		case BMC_BEARER_READ_REQ:
			break;
		case MEMORY_INFOMATION:
			break;
		case PATCH_RAM_INFORMATION:
			break;

	}

	Dect_DebugSendtoModule(data);
}




void DECODE_DEBUG_FROM_MODULE(DECT_MODULE_DEBUG_INFO *data)
{
  int i=0,procflg=0;
	switch(data->debug_info_id)
	{
		case 6:
					if(vuiDbgSendFlags & PROC_MODEM_DBG6_SEND)
					{
						procflg=1;
						vuiDbgSendFlags &= ~PROC_MODEM_DBG6_SEND;
					}
					if(vuiDbgFlags & MODEM_DBG_RESET)
					{
						/*Dont update since this was for RESET option*/
						vuiDbgFlags &= ~MODEM_DBG_RESET;
						memset(&modem_stats,0,sizeof(modem_stats));
						if(gw_stats.uiLastOprn == DBG_RESET_MODEM_STATS)
						{
							gw_stats.uiLastOprnStatus = DBG_SUCCESS;
						}
					}
					else if(vuiDbgFlags & MODEM_DBG_SEND)
					{
						vuiDbgFlags &= ~MODEM_DBG_SEND;
						DecodeDbg6Pkt(data->G_PTR_buf);
						if(gw_stats.uiLastOprn == DBG_REQ_MODEM_STATS)
						{
							gw_stats.uiLastOprnStatus = DBG_SUCCESS;
						}
					}
					break;
		case 7:
					printk("\n Debug Pkt content Start\n");
          for(i=0;i<15;i++)
					{
						printk("  %x ",data->G_PTR_buf[i]);
					}
					printk("\n Debug Pkt content End\n");
					if(vuiDbgSendFlags & PROC_MODEM_DBG7_SEND)
					{
						procflg=1;
						vuiDbgSendFlags &= ~PROC_MODEM_DBG7_SEND;
					}
					DecodeDbg7Pkt(data->G_PTR_buf);
					break;
		case 8:
					printk("\n Debug Pkt content Start\n");
          for(i=0;i<15;i++)
					{
						printk("  %x ",data->G_PTR_buf[i]);
					}
					printk("\n Debug Pkt content End\n");
					if(vuiDbgSendFlags & PROC_MODEM_DBG8_SEND)
					{
						procflg=1;
						vuiDbgSendFlags &= ~PROC_MODEM_DBG8_SEND;
					}
					DecodeDbg8Pkt(data->G_PTR_buf);
					break;
	}
	if(!procflg)
	{
	/*indicate to app only if the request is not from proc file system*/
		Dect_DebugSendtoApplication(data);
	}
}

unsigned int GetTwoBytes(unsigned char *buf)
{
	unsigned int num=0;
	num = (((*buf) << 8) | *(buf+1));
	return num;
}
void DecodeDbg6Pkt(unsigned char *buf)
{
	int i=0,j=0;
	unsigned int uiTemp=0;
	version.ucCosicSwVersion[0]=buf[0];
	version.ucCosicSwVersion[1]=buf[1];
	version.ucBmcFwVersion[0]=buf[2];
	version.ucBmcFwVersion[1]=buf[3];

	i= 4;
	uiTemp = modem_stats.uiSpiTimeOutTxTC; 
  modem_stats.uiSpiTimeOutTxTC =GetTwoBytes(buf+i);
	modem_stats.uiSpiTimeOutTxCC = modem_stats.uiSpiTimeOutTxTC - uiTemp;
	i += 2;
	uiTemp = modem_stats.uiSpiTimeOutRxTC;
  modem_stats.uiSpiTimeOutRxTC =GetTwoBytes(buf+i);
	modem_stats.uiSpiTimeOutRxCC = modem_stats.uiSpiTimeOutRxTC - uiTemp;
	i += 2;
	uiTemp = modem_stats.uiStackOverFlowTC;
  modem_stats.uiStackOverFlowTC =GetTwoBytes(buf+i);
	modem_stats.uiStackOverFlowCC = modem_stats.uiStackOverFlowTC - uiTemp;
	i += 2;
	uiTemp = modem_stats.uiNonSrvcSpiIntTC;
  modem_stats.uiNonSrvcSpiIntTC =GetTwoBytes(buf+i);
	modem_stats.uiNonSrvcSpiIntCC = modem_stats.uiNonSrvcSpiIntTC - uiTemp;
	i += 2;

	while(i < 24)
	{
		lbn_stats[j++].uiBadAXCRC = GetTwoBytes(buf+i);
		i+=2;
	}
	j=0;
	while(i < 36)
	{
		lbn_stats[j++].uiQ1Q2BadACRC = GetTwoBytes(buf+i);
		i+=2;
	}
	uiTemp = modem_stats.uiDummyBearerChangeTC;
	modem_stats.uiDummyBearerChangeTC = GetTwoBytes(buf+i);
	modem_stats.uiDummyBearerChangeCC = modem_stats.uiDummyBearerChangeTC - uiTemp;
	//modem_stats.uiMaxTimeSscTimerA = buf[36];
}
void DecodeDbg7Pkt(unsigned char *buf)
{
	int i=0,j=0;
	while(i < 12)
	{
		lbn_stats[j++].uiFailedHO = GetTwoBytes(buf+i);
		i+=2;
	}
	j=0;
	while(i < 24)
	{
		lbn_stats[j++].uiSuccessHO = GetTwoBytes(buf+i);
		i+=2;
	}
	j=0;
	while(i < 36)
	{
		lbn_stats[j++].uiBadZCRC = GetTwoBytes(buf+i);
		i+=2;
	}
}
void DecodeDbg8Pkt(unsigned char *buf)
{
	int i=0,j=0;
	while(i < 12)
	{
		lbn_stats[j++].uiRSSI = GetTwoBytes(buf+i);
		i+=2;
	}
	j=0;
	while(i < 24)
	{
		lbn_stats[j++].uiSyncErr = GetTwoBytes(buf+i);
		i+=2;
	}
}
/******************************************************************  
 *  Function Name  : WriteDbgPkts
 *  Description    : This forms DBG pkts and sends it to LMAC
 *  Input Values   : Debug Info Id, 
									 : read/write indicator, read=0 write=1
									 : Incarnation(0), 
									 : Buffer: used incase of write 
 *  Output Values  : None
 *  Return Value   : void 
 *  Notes          : 
 * ****************************************************************/
void WriteDbgPkts(unsigned char debug_info_id, unsigned char rw_indicator, \
                  unsigned char CurrentInc, unsigned char *G_PTR_buf)
{
	DECT_MODULE_DEBUG_INFO value;
	memset(&value,0,sizeof(value));
	value.debug_info_id =debug_info_id; 
	value.rw_indicator =rw_indicator; 
	value.CurrentInc =CurrentInc; 
	DECODE_DEBUG_TO_MODULE(&value);
	vuiDbgFlags |= MODEM_DBG_SEND;
	return;
}

#endif

