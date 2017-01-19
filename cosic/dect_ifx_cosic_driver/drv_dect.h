/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#ifndef _DECT_DRV_H_
#define _DECT_DRV_H_

#define G_PTR_MAX_COUNT_MAC   10

#ifdef CATIQ_UPLANE
#define G_PTR_MAX_COUNT   64
#else
#define G_PTR_MAX_COUNT   10    //
#endif
#define G_PTR_MAX_DEBUG_COUNT	43		// 'ram indicator 1byte' + 'address 2byte' + 'data 40byte'
typedef struct hmac_queues
{
  unsigned char PROCID;
  unsigned char MSG;
  unsigned char Parameter1;
  unsigned char Parameter2;
  unsigned char Parameter3;
  unsigned char Parameter4;
  unsigned char G_PTR_buf[G_PTR_MAX_COUNT];
  unsigned char CurrentInc;
} HMAC_QUEUES;

typedef struct dect_module_debug_info
{
  unsigned char debug_info_id;
  unsigned char rw_indicator;		// read write indicator
  unsigned char CurrentInc;			
  unsigned char G_PTR_buf[G_PTR_MAX_DEBUG_COUNT];
} DECT_MODULE_DEBUG_INFO;


typedef enum
{
  DECT_DRV_HMAC_BUF_EMPTY = 0x00,
  DECT_DRV_HMAC_BUF_LAST,
  DECT_DRV_HMAC_BUF_MORE
}HMAC_LMAC_RETURN_VALUE;

#define MAX_READ_BUFFFERS 5 /* Max buffers that are stored in driver*/
#define MAX_WRITE_BUFFERS 5 /* Max buffers that user can write */
#define COSIC_LOADER_PACKET_SIZE 508 
extern unsigned char vucDriverMode;
extern unsigned char vucReadBuffer[MAX_READ_BUFFFERS][COSIC_LOADER_PACKET_SIZE];
extern unsigned char vucWriteBuffer[MAX_WRITE_BUFFERS][COSIC_LOADER_PACKET_SIZE];
extern unsigned short vu16ReadBufHead;
extern unsigned short vu16ReadBufTail;
extern unsigned short vu16WriteBufHead;
extern unsigned short vu16WriteBufTail;

#define IFX_DECT_ON                0x01
#define IFX_DECT_OFF               0x00

#define DECT_DRV_SHUTDOWN                		 0x11  // to HMBC
#define DECT_DRV_TO_HMAC_MSG                 0x01  // to HMBC
#define DECT_DRV_FROM_HMAC_MSG               0x02  // from HMBC
#define DECT_DRV_GET_PMID                    0x03  // from HMBC
#define DECT_DRV_GET_ENC_STATE               0x04  // from HMBC
#define DECT_DRV_SET_KNL_STATE               0x05  // to HMBC
#define DECT_DRV_DEBUG_INFO                  0x06  // To Cosic Modem debug infomation
#define DECT_DRV_DEBUG_INFO_FROM_MODULE      0x07  // To Cosic Modem debug infomation
#define DECT_DRV_TRIGGER_COSIC_DRV           0x08  // Activate cosic driver

#define DECT_DRV_SET_LOADER_MODE 0x09 // To Loader Mode

#define DECT_DRV_SET_APP_MODE 0x0A // To Normal working mode
#define DECT_DRV_SET_APP_MODE_CoC_RQ 0x0D // To Normal working CoC requeted by Modem mode
#define DECT_DRV_SET_APP_MODE_CoC_CNF 0x0E // To Normal working CoC confirmed by GW mode

#define DECT_DRV_FW_WRITE 0x0B // To write firmware

#define DECT_DRV_FW_READ 0x0C // To Read write confirmation
#define DECT_DRV_IRQ_CTRL 0x0F // To control IRQ
#define DECT_DRV_ENABLE 0x10

extern int spi_dect_dev_id;

void Dect_SendtoStack(HMAC_QUEUES * data);
void Dect_SendtoLMAC(HMAC_QUEUES * data);
HMAC_LMAC_RETURN_VALUE Dect_Drv_Get_Data(HMAC_QUEUES* data);

void Dect_DebugSendtoApplication(DECT_MODULE_DEBUG_INFO* data);
void Dect_DebugSendtoModule(DECT_MODULE_DEBUG_INFO* data);
HMAC_LMAC_RETURN_VALUE Dect_Debug_Get_Data(DECT_MODULE_DEBUG_INFO* data);

int ssc_dect_haredware_reset(int on);

void Reset_Hmac_Debug_buffer(void);

/* Datastructure for maintaining the incoming buffers for voice and data */
typedef struct x_IFX_Mcei_Buffer
{
  int iKpiChan;
  int iIn;
  int iOut;
  char *pcVoiceDataBuffer;
  char *pcVoiceDataBuffer1;
  char acMacBuffer[50];
}x_IFX_Mcei_Buffer;

#ifdef CONFIG_VR9
#if 0 //ctc
#define IFX_DECT_RST 14
#define IFX_DECT_INT 9
#define IFX_DECT_SPI_CS 22
#else
#define IFX_DECT_RST 12
#define IFX_DECT_INT 1
#define IFX_DECT_SPI_CS 9
#endif
extern const int dect_gpio_module_id;
#endif


#ifdef CONFIG_AR9 
	#ifdef CONFIG_IFX_GW188
		#define IFX_DECT_RST 46
		#define IFX_DECT_SPI_CS 33
	#else
		#ifdef COSIC_BMC_FW_ON_RAM
			#define IFX_DECT_RST 33
		#else
			#define IFX_DECT_RST 22
		#endif
		#define IFX_DECT_SPI_CS 13
	#endif
#define IFX_DECT_INT 39
extern const int dect_gpio_module_id;
#elif defined(CONFIG_DANUBE)
#define IFX_DECT_RST 20
#define IFX_DECT_INT 2
#define IFX_DECT_SPI_CS 10
extern const int dect_gpio_module_id;
#elif defined(CONFIG_AR10)
#define IFX_DECT_RST 19
#define IFX_DECT_INT 9		/* EXIN5 */
#define IFX_DECT_SPI_CS 15	/* SPI_CS1 */
#endif

#ifdef DECT_USE_USIF
#define ifx_ssc_cs_low ifx_usif_spi_cs_low
#define ifx_ssc_cs_high ifx_usif_spi_cs_high
#define ifx_sscLock ifx_usif_spiLock
#define ifx_sscUnlock ifx_usif_spiLock
#define ifx_sscSetBaud ifx_usif_spiSetBaud
#define ifx_sscTxRx ifx_usif_spiTxRx
#define ifx_sscRx ifx_usif_spiRx
#define ifx_sscTx ifx_usif_spiTx
#define IFX_SSC_HANDLE IFX_USIF_SPI_HANDLE_t
#define ifx_sscAllocConnection ifx_usif_spiAllocConnection
#define ifx_sscFreeConnection ifx_usif_spiFreeConnection
#define ifx_sscAsyncTxRx ifx_usif_spiAsyncTxRx
#define ifx_sscAsyncTx ifx_usif_spiAsyncTx
#define ifx_sscAsyncRx ifx_usif_spiAsyncRx
#define ifx_sscAsyncLock ifx_usif_spiAsyncLock
#define ifx_sscAsyncUnLock ifx_usif_spiAsyncUnLock
#define IFX_SSC_PRIO_LOW IFX_USIF_SPI_PRIO_LOW
#define IFX_SSC_PRIO_MID IFX_USIF_SPI_PRIO_MID
#define IFX_SSC_PRIO_HIGH IFX_USIF_SPI_PRIO_HIGH
#define IFX_SSC_PRIO_ASYNC IFX_USIF_SPI_PRIO_ASYNC 
#define IFX_SSC_MODE_0 IFX_USIF_SPI_MODE_0
#define IFX_SSC_MODE_1 IFX_USIF_SPI_MODE_1
#define IFX_SSC_MODE_2 IFX_USIF_SPI_MODE_2
#define IFX_SSC_MODE_3 IFX_USIF_SPI_MODE_3
#define IFX_SSC_MODE_UNKNOWN IFX_USIF_SPI_MODE_UNKNOWN 
#define IFX_SSC_HANDL_TYPE_SYNC IFX_USIF_SPI_HANDL_TYPE_SYNC
#define IFX_SSC_HANDL_TYPE_ASYNC IFX_USIF_SPI_HANDL_TYPE_ASYNC
#define IFX_CS_DATA IFX_USIF_SPI_CS_DATA_t
#define IFX_SSC_CS_ON IFX_USIF_SPI_CS_ON
#define IFX_SSC_CS_OFF IFX_USIF_SPI_CS_OFF
#define IFX_SSC_WHBGPOSTAT_OUT0_POS IFX_USIF_SPI_CS0
#define IFX_SSC_WHBGPOSTAT_OUT1_POS IFX_USIF_SPI_CS1
#define IFX_SSC_WHBGPOSTAT_OUT2_POS IFX_USIF_SPI_CS2
#define IFX_SSC_WHBGPOSTAT_OUT3_POS IFX_USIF_SPI_CS3
#define IFX_SSC_WHBGPOSTAT_OUT4_POS IFX_USIF_SPI_CS4
#define IFX_SSC_WHBGPOSTAT_OUT5_POS IFX_USIF_SPI_CS5
#define IFX_SSC_WHBGPOSTAT_OUT6_POS IFX_USIF_SPI_CS6
#define IFX_SSC_WHBGPOSTAT_OUT7_POS IFX_USIF_SPI_CS7
#define IFX_SSC_CS_CB_t IFX_USIF_SPI_CS_CB_t
#define ifx_ssc_async_fkt_cb_t ifx_usif_spi_async_fkt_cb_t
#define IFX_SSC_ASYNC_CALLBACK_t IFX_USIF_SPI_ASYNC_CALLBACK_t
#define IFX_SSC_CONFIGURE_t IFX_USIF_SPI_CONFIGURE_t
#define ssc_mode spi_mode
#define ssc_prio spi_prio
#endif
#endif /* _DECT_DRV_H_ */

