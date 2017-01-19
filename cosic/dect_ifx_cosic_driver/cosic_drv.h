/******************************************************************************

                              Copyright (c) 2011
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany
	Author: Chintan Parekh
  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
#ifndef _COSIC_DRV_H
#define _COSIC_DRV_H_

#define MAX_READ_BUFFERS 12 /* Max buffers that are stored in driver*/
#define MAX_WRITE_BUFFERS 12 /* Max buffers that user can write */
#define MAX_COMMAND_SIZE  256

#define CVOIP_ONBOARD /* means CVOIP is on board else on daughter card*/

#ifdef CVOIP_ONBOARD
#define GPIO_INT 39 //GPIO Number for Interrupt line
#define GPIO_CS 13 //GPIO Number for Chip Select line
#define GPIO_RESET 22
#define DECT_DRV_SSC_HW_RESET 0x01
#else // CVOIP on daughter board
#define GPIO_INT 39 //GPIO Number for Interrupt line
#define GPIO_CS 7 //GPIO Number for Chip Select line
#endif

#define DBG_LEVEL 3 /* 0=NO DBG, 1=Error, 2=Normal, 3=High*/
#if DBG_LEVEL > 0
	#define DBGE printk
#else
	#define DBGE(...) 
#endif
#if DBG_LEVEL > 1
	#define DBGN printk
#else
	#define DBGN(...) 
#endif
#if DBG_LEVEL > 2
	#define DBGH printk
#else
	#define DBGH(...) 
#endif

typedef struct{
	  char acCommand[MAX_COMMAND_SIZE];
	  unsigned char len;
	  unsigned char index;
}x_CosicDrv_Buffer;

typedef enum {
	COSIC_DRV_MODE_IDLE =0,
	COSIC_DRV_MODE_WR, /*Write Command to SPI*/
	COSIC_DRV_MODE_RD, /*Read  Command from SPI*/
	COSIC_DRV_MODE_RDWR, /*Read and Write Mode */
	COSIC_DRV_MODE_WAIT_UNLOCK_SPI,
	COSIC_DRV_MODE_MAX 
} e_CosicDrv_Mode;
	
typedef struct {
	/*TxHdr[0] mapped to Hdr0 of transmit buffer - it is updated based on 
	 - CRC of received payload 
	 - 
	 TxHdr[1] is mapped to Hdr1 of transmit buffer
	 TxHdr[2] is RESERVED (Storing squence no of pkt(gw->cv) for which ack has to come)
	 TxHdr[3] is used for user defined flags - e_TransState
	*/
	char TxHdr[4]; 
	char RxHdr[4]; 
}x_CosicDrv_Transaction;

/* Flags for TxHdr[3]/RxHdr[3] */
typedef enum
{
	IDLE_STATE=0, /*No TX or No Rx */
	IN_PROGRESS,  /*Tx/Rx in progres */
	LAST_PACKET,  /*Last Pcaket sent/received */
	TRANS_OVER    /*Command recived/sent */
}e_TransState;


#define INC_RXBUF_HEAD RxBufHead = (RxBufHead+1) % (MAX_READ_BUFFERS); \
										   RxOverflowFlag = (RxBufHead==0)?0:RxOverflowFlag;

#define INC_RXBUF_TAIL RxBufTail = (RxBufTail+1) % (MAX_READ_BUFFERS); \
									     RxOverflowFlag = (RxBufTail == 0) ? 1:RxOverflowFlag;


#define INC_TXBUF_HEAD TxBufHead = (TxBufHead +1) % (MAX_WRITE_BUFFERS);\
    									 TxOverflowFlag = (TxBufHead==0)?0:TxOverflowFlag;

#define INC_TXBUF_TAIL TxBufTail = (TxBufTail+1) % (MAX_WRITE_BUFFERS); \
										   TxOverflowFlag = (TxBufTail == 0) ? 1:TxOverflowFlag;

#define IsRxBufEmpty ( ( (!RxOverflowFlag) && (RxBufHead == RxBufTail) ) ? 1:0)
#define IsRxBufFull  ( ( (RxOverflowFlag) && (RxBufHead == RxBufTail)) ? 1:0)

#define IsTxBufEmpty ( ( (!TxOverflowFlag) && (TxBufHead == RxBufTail) ) ? 1:0)
#define IsTxBufFull  ( ( (TxOverflowFlag) && (TxBufHead == TxBufTail) ) ? 1:0)

#define MAKE_TRANS_IDLE memset(&vxTransaction,0,sizeof(x_CosicDrv_Transaction));

#define SET_TXSTATE(STATE) 	(vxTransaction.TxHdr[3] = STATE )
#define IsTxStateSet(STATE) ((vxTransaction.TxHdr[3] == STATE) ?1:0)

#define SET_RXSTATE(STATE) 	(vxTransaction.RxHdr[3] = STATE )
#define IsRxStateSet(STATE) ((vxTransaction.RxHdr[3] == STATE) ?1:0)



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
#elif defined(CONFIG_DANUBE)
#define IFX_DECT_RST 20
#define IFX_DECT_INT 2
#define IFX_DECT_SPI_CS 10
extern const int dect_gpio_module_id;
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

