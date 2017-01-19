/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
#ifndef DRV_DECT_COSIC_DRV_C
#define DRV_DECT_COSIC_DRV_C

#include <linux/autoconf.h>
#include <linux/module.h>

#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <linux/ioport.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/param.h>
#include <linux/timer.h>
#include <linux/ctype.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
#include <asm/semaphore.h>
#endif
#include <asm/irq.h>
#include <linux/unistd.h>
#ifdef CONFIG_LTT
#include <linux/marker.h>
#endif
#ifdef CONFIG_AMAZON_S

#include <asm/amazon_s/amazon_s.h>
#include <asm/amazon_s/irq.h>

#elif defined(CONFIG_DANUBE)


#include <asm-mips/ifx/danube/danube.h>
#include <asm-mips/ifx/danube/irq.h>
#include <asm-mips/ifx/ifx_gpio.h>
#include <asm-mips/ifx/ifx_led.h>
#include<asm-mips/ifx/ifx_gptu.h>
static const int dect_gpio_module_id = IFX_GPIO_MODULE_DECT;
#elif defined CONFIG_AR10

#include <asm-mips/ifx/ar10/ar10.h>
#include <asm-mips/ifx/ar10/irq.h>
#include <asm-mips/ifx/ifx_gpio.h>
static const int dect_gpio_module_id = IFX_GPIO_MODULE_DECT;


#elif defined CONFIG_AR9

#include <asm-mips/ifx/ar9/ar9.h>
#include <asm-mips/ifx/ar9/irq.h>
#include <asm-mips/ifx/ifx_gpio.h>
static const int dect_gpio_module_id = IFX_GPIO_MODULE_DECT;
#elif defined CONFIG_VR9
#include <asm-mips/ifx/vr9/vr9.h>
#include <asm-mips/ifx/vr9/irq.h>
#include <asm-mips/ifx/ifx_gpio.h>
static const int dect_gpio_module_id = IFX_GPIO_MODULE_DECT;
#endif	
#include "ifx_types.h"
#ifdef DECT_USE_USIF
#include <asm/ifx/ifx_usif_spi.h>
#else
#include <asm/ifx/ifx_ssc.h>
#endif
#include "drv_dect.h"
#include "drv_dect_cosic_drv.h"
#include "fhmac.h"
#include "FDEF.H"
#include "CONF_CP.H"
#include "DECT.H"
#include "lib_bufferpool.h"
#include "drv_tapi_kpi_io.h"
#include "drv_timer.h"
#include "fdebug.h"

/*enable the debug in Makefile by passing IFX_COSIC_DEBUG flag*/	
#ifdef IFX_COSIC_DEBUG
  #define IFX_COSIC_DMSG(fmt, args...) printk(KERN_INFO "[%s %d]: "fmt,__func__, __LINE__, ##args)
#else
  #define IFX_COSIC_DMSG(fmt, args...)
#endif

#ifdef CONFIG_DANUBE
/*GPT register values*/
#define GPT_INT_NODE_EN_REG 0xBE100aF4
#define GPT_INT_NODE_CTRL_REG 0xBE100aFC
#define GPT_CTRL_REG_COUNTER_2A 0xBE100a30
#define GPT_RELOAD_REG_2A 0xBE100a40
#define GPT_RUN_REG_2A 0xBE100a38


#define ACKNOWLEDGE_IRQ                         \
do {                                                     \
   /* ack GPTC 2A IRQ */                                 \
   *(unsigned long *) GPT_INT_NODE_CTRL_REG = 0x00000004;           \
   /* reload GPTC 2A with initial value. in our case with 1*/                                  \
   *(unsigned long *) GPT_RUN_REG_2A = 0x00000005;           \
} while (0)

#define CONFIGURE_FALLING_EDGE_INT *(unsigned long *) GPT_CTRL_REG_COUNTER_2A = 0x0000008C
#define CONFIGURE_RISING_EDGE_INT *(unsigned long *) GPT_CTRL_REG_COUNTER_2A = 0x0000004C

#elif defined CONFIG_AR9
#define ACKNOWLEDGE_IRQ  gp_onu_gpio_write_with_bit_mask((unsigned long *)IFX_ICU_EIU_INC, (8),(8))

#define CONFIGURE_FALLING_EDGE_INT  \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C &= 0xFFFF0FFF; \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C |= 0x2000

#define CONFIGURE_RISING_EDGE_INT  \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C &= 0xFFFF0FFF; \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C |= 0x1000

#elif defined(CONFIG_VR9)
 /*ctc change from EXIN5 --> EXIN1*/
#if 0
#define ACKNOWLEDGE_IRQ  gp_onu_gpio_write_with_bit_mask((unsigned long *)IFX_ICU_EIU_INC, (0x32),(0x32))

#define CONFIGURE_FALLING_EDGE_INT  \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C &= 0xFF0FFFFF; \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C |= 0x200000

#define CONFIGURE_RISING_EDGE_INT  \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C &= 0xFF0FFFFF;  \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C |= 0x100000
#else
#define ACKNOWLEDGE_IRQ  gp_onu_gpio_write_with_bit_mask((unsigned long *)IFX_ICU_EIU_INC, (0x02),(0x02))

#define CONFIGURE_FALLING_EDGE_INT  \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C &= 0xFFFFFF0F; \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C |= 0x000020

#define CONFIGURE_RISING_EDGE_INT  \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C &= 0xFFFFFF0F; \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C |= 0x000010
#endif
#elif defined(CONFIG_AR10)
#define ACKNOWLEDGE_IRQ  gp_onu_gpio_write_with_bit_mask((unsigned long *)IFX_ICU_EIU_INC, (0x32),(0x32))

#define CONFIGURE_FALLING_EDGE_INT  \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C &= 0xFF0FFFFF; \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C |= 0x200000

#define CONFIGURE_RISING_EDGE_INT  \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C &= 0xFF0FFFFF;  \
				*(unsigned long *) IFX_ICU_EIU_EXIN_C |= 0x100000
#elif defined CONFIG_AMAZON_S
#define ACKNOWLEDGE_IRQ gp_onu_gpio_write_with_bit_mask((unsigned long *)AMAZON_S_ICU_EIU_INC, (8),(8))
#endif
	
#define COSIC_RX_HEADER_LEN 8

#define WIDEBAND_PKT_LENGTH  80
#define NARROWBAND_PKT_LENGTH  40

#define WIDEBAND_PKT  3
#define NARROWBAND_PKT  0

#define VOICE_HEADER_LENGTH 6

#define FIRST_BYTE_FROM_MODEM_LOADER_MODE 0x55
#define FIRST_BYTE_FROM_MODEM_APP_MODE 0xAA

volatile unsigned int IntEdgFlg;
extern u32 ssc_dect_cs(u32 on,u32 cs_data);
extern modem_dbg_stats modem_stats;
extern gw_dbg_stats gw_stats;
extern dect_version version;
extern modem_dbg_lbn_stats lbn_stats[6];
extern atomic_t vuiGwDbgFlag;
extern void WriteDbgPkts(unsigned char debug_info_id, unsigned char rw_indicator, \
									unsigned char CurrentInc, unsigned char *G_PTR_buf);
struct tasklet_struct x_IFX_Cosic_Tasklet;
struct tasklet_struct x_IFX_Cosic_Tasklet1;
struct tasklet_struct x_IFX_Tapi_Tasklet;
volatile unsigned long i_CosicData;
volatile unsigned long i_CosicData1;
atomic_t vuiIgnoreint;
extern int viDisableIrq;
//int viFreeIrq;

x_IFX_Mcei_Buffer xMceiBuffer[MAX_MCEI];

typedef struct{
  int pid;
  unsigned long jiffies;
}my_sched_info_t;

unsigned int	transmit_len;
unsigned int	rx_len;


/* Interrupt number on which the dect notifies */
extern int dect_gpio_intnum;
extern int Dect_excpFlag;
extern wait_queue_head_t Dect_WakeupList;
int valid_voice_pkt[MAX_MCEI];
extern BUFFERPOOL *pTapiWriteBufferPool;

unsigned char Dect_mutex_free = 0;

/* SSC TX and RX buffers */
#ifdef COSIC_DMA
unsigned char *ssc_cosic_rx_buf_temp,*ssc_cosic_tx_buf_temp;
unsigned char *ssc_cosic_rx_buf;	/* cosic rx  buffer */
unsigned char *ssc_cosic_tx_buf;	/* cosic tx buffer */
#else
unsigned char ssc_cosic_rx_buf[MAX_SPI_DATA_LEN + 128];	/* cosic rx  buffer */
unsigned char ssc_cosic_tx_buf[MAX_SPI_DATA_LEN+128];	/* cosic tx buffer */
#endif
int32 iNeedVersion=1;

/* tapi voice wrtie data */
TAPI_VOICE_WRITE_BUFFER Tapi_voice_write_buffer[MAX_TAPI_BUFFER_CNT];
unsigned char           Tapi_voice_write_buffer_r_cnt;
unsigned char           Tapi_voice_write_buffer_w_cnt;
/* COSIC data packat */
COSIC_PACKET cosic_data_packet;

/* Max SPI data length */
unsigned int Max_spi_data_len_val = MAX_SPI_DATA_LEN;

#ifdef  VOICE_CYCLIC_INDEX_CHECK
unsigned char Tback_cyclic_index[6];
unsigned char Tcheck_cyclic_index[6];
#endif

/* cosic thread/tasklet status variable, It alternates between Tx and Rx */
COSIC_THREAD_STATUS cosic_status = COSIC_THREAD_TRANSFER_STATUS;		
/* Temporary variable used for looping till DECT application comes up */
unsigned int starting_cnt = 0;
/* SPI device handler for COSIC */
extern IFX_SSC_HANDLE *spi_dev_handler;
/* Cosic mode indicating Tx/Rx Tx=0, Rx=1 */
int iCosicMode=1;
/* Voice mute */
#ifdef VOICE_MUTE_SERVICE_CHANGE
unsigned char voice_mute_cnt[MAX_MCEI];
#endif
uint32 uiReq;
uint32 uiResp;
int viSPILocked =0;
int viSPILockedCB =0;
int iDriverStatus = 0;
int FirstLock =1;
int viSPILocked1 =0;
int viSPIUnLockSchedule =0;
/* Function prototypes */
int ssc_dect_debug(int on);
void ifx_sscCosicRx(void);
int ifx_sscCosicLock(void);
void ifx_sscCosicUnLock(void);
void ifx_sscCosicTx(void);
void ifx_cosic_driver(unsigned long ulDummy);
void ifx_cosic_driver1(unsigned long ulDummy);
void IFX_COSIC_TapiRead(unsigned long ulDummy);
void init_tapi_read_write_buffer(void);
/* EXPORT SYMBOL from TAPI drvier module */
extern IFX_int32_t IFX_TAPI_KPI_ReadData( IFX_TAPI_KPI_CH_t nKpiGroup,
                                   IFX_TAPI_KPI_CH_t *nKpiChannel,
                                   void **pPacket,
                                   IFX_uint32_t *nPacketLength,
                                   IFX_uint8_t *nMore);
extern IFX_int32_t IFX_TAPI_KPI_WriteData( IFX_TAPI_KPI_CH_t nKpiChannel,
                                    void *pPacket,
                                    IFX_uint32_t nPacketLength);
void gp_onu_enable_external_interrupt(void);


/* Structure maintaining  Mapping between LMAC and HMAC */
static unsigned char   Message_Map_Table[ ][ 2 ] = 
{
  {LMACCmd_BOOT_RQ_HMAC, MAC_BOOT_RQ_HMAC},/*MAC_BOOT_RQ_HMAC (HMAC)*/
  {HMACCmd_BOOT_IND_LMAC, MAC_BOOT_IND_LMAC},
  {LMACCmd_PARAMETER_PRELOAD_RQ_HMAC, MAC_PARAMETER_PRELOAD_RQ_HMAC },
  {LMACCmd_GFSK_VALUE_READ_RQ_HMAC, MAC_GFSK_GET_RQ_HMAC},
  {HMACCmd_GFSK_VALUE_READ_IND_LMAC, MAC_GFSK_IND_LMAC},/*MAC_GFSK_IND_LMAC*/
  {LMACCmd_GFSK_VALUE_WRITE_RQ_HMAC, MAC_GFSK_SET_RQ_HMAC},
  {LMACCmd_OSC_VALUE_READ_RQ_HMAC, MAC_OSC_GET_RQ_HMAC},
  {HMACCmd_OSC_VALUE_READ_IND_LMAC, MAC_OSC_IND_LMAC},
  {LMACCmd_OSC_VALUE_WRITE_RQ_HMAC, MAC_OSC_SET_RQ_HMAC},
  {HMACCmd_SLOT_FRAME_IND_LMAC, 0xFF},
  {LMACCmd_SEND_DUMMY_RQ_HMAC, MAC_SEND_DUMMY_RQ_HMAC},	
  {LMACCmd_A44_SET_RQ_HMAC, MAC_A44_SET_RQ_HMAC},
  {LMACCmd_TBR6_TEST_MODE_RQ_HMAC, MAC_TBR6_MODE_RQ_HMAC},
  {LMACCmd_Qt_MESSAGE_SET_RQ_HMAC,  MAC_QT_SET_RQ_HMAC},
  {LMACCmd_SWITCH_HO_TO_TB_RQ_HMAC, MAC_SWITCH_HO_TO_TB_RQ_HMAC},
  {LMACCmd_PAGE_RQ_HMAC, MAC_PAGE_RQ_HMAC},/*MAC_PAGE_RQ_HMAC (HMAC)*/
  {LMACCmd_PAGE_CANCEL_RQ_HMAC, MAC_PAGE_CANCEL_HMAC},/*MAC_PAGE_CNCL(HMAC)*/
  {HMACCmd_BS_INFO_SENT_LMAC, MAC_BS_INFO_SENT_LMAC},/*MAC_BS_INFO_SENT(LMAC)*/
  {HMACCmd_ACCESS_RQ_LMAC, MAC_MCEI_REQUEST_LMAC},/*MAC_MCEI_REQUEST(LMAC)*/
  {LMACCmd_ACCESS_CFM_HMAC, MAC_MCEI_CONFIRM_HMAC},/*MAC_MCEI_CONFIRM(HMAC)*/
  {HMACCmd_ESTABLISHED_IN_LMAC, MAC_ESTABLISHED_TB_LMAC},
  {LMACCmd_RELEASE_TBC_RQ_HMAC, MAC_RELEASE_TB_RQ_HMAC},
  {HMACCmd_RELEASE_TBC_IN_LMAC, MAC_RELEASED_TB_LMAC},/*MAC_RELEASED_TB(LMAC)*/
  {HMACCmd_CO_DATA_DTR_LMAC, MAC_CO_DATA_DTR_LMAC},/*MAC_CO_DATA_DTR(LMAC)*/
  {LMACCmd_CO_DATA_RQ_HMAC, MAC_CO_DATA_RQ_HMAC},/*MAC_CO_DATA_RQ_HMAC(HMAC)*/
  {HMACCmd_CO_DATA_IN_LMAC, MAC_CO_DATA_IN_LMAC},/*MAC_CO_DATA_IN_LMAC(LMAC)*/
  {LMACCmd_ENC_KEY_RQ_HMAC, MAC_ENC_KEY_RQ_HMAC},/*MAC_ENC_KEY_RQ_HMAC(HMAC)*/
  {HMACCmd_ENC_EKS_IND_LMAC, MAC_ENC_EKS_IN_LMAC},/*MAC_ENC_EKS_IN_LMAC(LMAC)*/
  {HMACCmd_ENC_EKS_FAIL_LMAC, MAC_ENC_EKS_FAIL_LMAC},/*MAC_ENC_EKS_FAIL(LMAC)*/
  {LMACCmd_VOICE_EXTERNAL_HMAC, MAC_VOICE_EXTERNAL_HMAC},
  {LMACCmd_VOICE_INTERNAL_HMAC, MAC_VOICE_INTERNAL_HMAC},
  {LMACCmd_VOICE_CONFERENCE_HMAC, MAC_VOICE_CONFERENCE_HMAC},/*MAC_CONF(HMAC)*/
  {LMACCmd_SLOTTYPE_MOD_RQ_HMAC, MAC_SLOTTYPE_MOD_RQ_HMAC},
  {HMACCmd_SLOTTYPE_MOD_CFM_LMAC, MAC_SLOTTYPE_MOD_CFM_LMAC},
  {HMACCmd_SLOTTYPE_MOD_TRIGGERED_LMAC, MAC_SLOTTYPE_MOD_TRIGGERED_PP_LMAC },
  {LMACCmd_MODULE_RESET_RQ_HMAC, MAC_MODULE_RESET_RQ_HMAC},
  {HMACCmd_DEBUG_MESSAGE_IND_LMAC, MAC_DEBUG_MESSAGE_IND_LMAC},
  {LMACCmd_NOEMO_RQ_HMAC, MAC_NOEMO_RQ_HMAC},/* "No-emission" mode procedures*/
  {HMACCmd_NOEMO_IND_LMAC, MAC_NOEMO_IND_LMAC},   
  {HMACCmd_DEBUGMSG_IN_LMAC, MAC_DEBUGMSG_IN_LMAC},   
  {0xFF,0xFF} 
};
#ifdef USE_VOICE_BUFFERPOOL
	#define BUFFER_POOL_PUT IFX_TAPI_VoiceBufferPut
#else
	#define BUFFER_POOL_PUT  bufferPoolPut
#endif

  unsigned short FwLen =0;
  unsigned char* pvucWriteBuffer=NULL;

/******************************************************************  
 *  Function Name  : start_tapiThread 
 *  Description    : This function registers a function to execute
 *                   when tapi tasklet is scheduled. In cases where
 *                   tasklets are not supported it starts a kernel 
 *                   thread
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void start_tapiThread(void)
{
  int iRet;
  init_tapi_read_write_buffer();
  tasklet_init(&x_IFX_Tapi_Tasklet,&IFX_COSIC_TapiRead,
               0);
  iRet=IFX_TAPI_KPI_EgressTaskletRegister(IFX_TAPI_KPI_GROUP2,
                                          (void*)&x_IFX_Tapi_Tasklet);
  if(iRet<0){
    printk("TAPI KPI registration failed\n");
  }
}

/******************************************************************
 *  Function Name  : stop_tapiThread
 *  Description    : This function unregisters the function with
 *                   TAPI tasklet mode/stops the kernel thread
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void stop_tapiThread(void)
{
  int iRet;
  /* TODO: No Unregister function?? */
  iRet=IFX_TAPI_KPI_EgressTaskletRegister(IFX_TAPI_KPI_GROUP2,
                                          NULL);
  if(iRet<0){
    printk("TAPI KPI Unregistration failed\n");
  }
  tasklet_kill(&x_IFX_Tapi_Tasklet);
}

/******************************************************************
 *  Function Name  : start_cosicThread
 *  Description    : This function initializes the COSIC tasklet or
 *                   starts cosic kernel thread
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void start_cosicThread(void)
{
#ifdef COSIC_DMA
  {
    ssc_cosic_tx_buf_temp = kmalloc(519,GFP_KERNEL);
    if(ssc_cosic_tx_buf_temp == NULL){
      printk("Error allocating Tx Buffer\n");
    }
		/*check for byte alignment*/
    if((int)ssc_cosic_tx_buf_temp%8){
      ssc_cosic_tx_buf = ssc_cosic_tx_buf_temp + (8 - ((int)ssc_cosic_tx_buf_temp%8));
    }
    else{
      ssc_cosic_tx_buf = ssc_cosic_tx_buf_temp;
    }
    ssc_cosic_rx_buf_temp = kmalloc(519,GFP_KERNEL);
    if(ssc_cosic_rx_buf_temp == NULL){
      printk("Error allocating Rx Buffer\n");
    }
		/*check for byte alignment*/
    if((int)ssc_cosic_rx_buf_temp%8){
     ssc_cosic_rx_buf = ssc_cosic_rx_buf_temp + (8 - ((int)ssc_cosic_rx_buf_temp%8));
    }
    else{
      ssc_cosic_rx_buf = ssc_cosic_rx_buf_temp;
    }
  }
#endif

  tasklet_init(&x_IFX_Cosic_Tasklet,(void*)&ifx_cosic_driver,
               (unsigned long)&i_CosicData);
  tasklet_init(&x_IFX_Cosic_Tasklet1,(void*)&ifx_cosic_driver1,
               (unsigned long)&i_CosicData1);
}

/******************************************************************
 *  Function Name  : stop_cosicThread 
 *  Description    : This function kilss the cosic drivers'
 *                   tasklet/thread
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void stop_cosicThread(void)
{
#ifdef COSIC_DMA
  if(ssc_cosic_rx_buf_temp != NULL){
    kfree(ssc_cosic_rx_buf_temp);
    ssc_cosic_rx_buf_temp = NULL;
  }
  if(ssc_cosic_tx_buf_temp != NULL){
    kfree(ssc_cosic_tx_buf_temp);
    ssc_cosic_tx_buf_temp = NULL;
  }
#endif
  if(dect_gpio_intnum != -1 ){
    disable_irq(dect_gpio_intnum);
    free_irq(dect_gpio_intnum , NULL);
		if(spi_dev_handler != NULL)
		{
    	ifx_sscFreeConnection(spi_dev_handler);
			spi_dev_handler = NULL;
		}
    dect_gpio_intnum = -1;
  }
  tasklet_kill(&x_IFX_Cosic_Tasklet);
  tasklet_kill(&x_IFX_Cosic_Tasklet1);
}

/******************************************************************
 *  Function Name  : 
 *  Description    : This function parses the data from danube to
 *                   LMAC 
 *  Input Values   : data buffer
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 *        lmac data parsing
 *	+---------+----+----+----+------------
 *	|         |    |    |    |                        
 *	+---------+----+----+----+------------
 *	 Total Len Type Len  Slot 
 *  Type:  0x01-COSIC_MAC_PACKET, 0x17-COSIC_VOICE_PACKET
 *         0x02-COSIC_DECT_SYNC_PACKET, 0x03-COSIC_DECT_SETUP_PACKET    
 * ****************************************************************/
void 
From_LMAC_Data(unsigned char *data)
{
  unsigned int  total_len;
  unsigned int  i;
  unsigned int  data_index;
  unsigned int  data_len;
  unsigned int  voice_channel;
  int ret;
  TAPI_VOICE_WRITE_BUFFER *pTapiData;
#ifdef VOICE_DATA_NIBBLE_SWAP
  unsigned char voice_nibble;
#endif
  unsigned char valid_channel;
  //static int counter=0;
  /* First 2 bytes constitute the total length */
  total_len = data[COSIC_DATA_HIGH_LENGTH];
  total_len = total_len << 8 | data[COSIC_DATA_LOW_LENGTH];
  /* Remove the bytes reqd for storing total len */
  total_len -= 2;
  data_index = COSIC_DATA_MSG_TYPE;
  /* if the length recvd is lesser than two bytes, it's an erroneous condn */
  if((total_len <= 2) || (total_len > Max_spi_data_len_val)){
    return;
  }
      if((vucDriverMode == DECT_DRV_SET_LOADER_MODE)&&(data[COSIC_DATA_MSG_TYPE] == COSIC_DECT_SYNC_PACKET)){
    		memcpy(vucReadBuffer[vu16ReadBufHead],data, total_len+2);
		  	++vu16ReadBufHead;
			 if(vu16ReadBufHead == MAX_READ_BUFFFERS)
			  	vu16ReadBufHead = 0;
			  if(vu16ReadBufHead == vu16ReadBufTail) {
				   printk(": Read buffer full\n");
				   vu16ReadBufTail++;
				   if(vu16ReadBufTail == MAX_READ_BUFFFERS)
					   vu16ReadBufTail = 0;
        }
        Dect_excpFlag = 1;
        wake_up_interruptible(&Dect_WakeupList);
        return;
      }

  /* if totallen is lt HMAC Q size or gt SPI data len mark invalid */
  /*We allow different sized packets => Merged from Intnix Rel 140409*/
  /*if((total_len < sizeof(HMAC_QUEUES)+2)||total_len > Max_spi_data_len_val){*/
	if((total_len < (17+2)) || total_len > Max_spi_data_len_val){
    IFX_COSIC_DMSG("invalid packet size total_len=%d\n", total_len);
    return;
  }
  while(data[data_index]){
    if(total_len <= data_index) break;
		/* check message type 
		0x01 : COSIC_MAC_PACKET , 			0x17 : COSIC_VOICE_PACKET
		0x02 : COSIC_DECT_SYNC_PACKET,	0x03 : COSIC_DECT_SETUP_PACKET    
      0x05 : COSIC_DECT_FU10_PACKET
		*/
    switch(data[data_index++]){
      case COSIC_VOICE_PACKET:	
      {
        /* 4 byte header + payload	
           Type (1 byte) - 0x17
	         Length (1 byte) - 40 t0 80
	         MCEI Number(1 byte) - MCEI number / voice channel / DECT channel
	         Sub Info (nibble),     - don't care
	         Cyclic Index (nible),  - 4 bits cyclic counter set to help TAPI FW 
                                    in case of lost packets (missed interrupts),
           Voice Data (40 or 80 bytes)*/
        /*IFX_COSIC_DMSG("cosic voice packet rcvd \n");*/
#ifdef USE_VOICE_BUFFERPOOL
        pTapiData=(TAPI_VOICE_WRITE_BUFFER*)IFX_TAPI_VoiceBufferGet();
#else
	      pTapiData=(TAPI_VOICE_WRITE_BUFFER*)bufferPoolGet(pTapiWriteBufferPool);
#endif
        if(pTapiData == NULL){
          /* TODO: error handling?? */
          printk(" buffer pool get failed\n");
		  return;
        }
        /* set packet type */
        pTapiData->type_slot = 0;
        pTapiData->type_slot = TAPI_B_VOICE_PACKET << 4;
        /* populate the data len Minus MCEI_num(1byte), sub_Info+seq(1byte) */
        data_len = data[data_index++] - 2;
        if(data_len > MAX_TAPI_VOICE_BUFFER_SIZE){
          data_len = MAX_TAPI_VOICE_BUFFER_SIZE;
	      }
        data_len += 4;
        pTapiData->voice_len_high = (data_len >> 8); /* should be zero */
	      pTapiData->voice_len_low  = data_len;
	      data_len -= 4;
        /* mcei number */
        voice_channel = data[data_index++]; 
        if(! valid_voice_pkt[voice_channel])
		{
		/*Check if there is a valid voice packet on this channel*/
	  /* 0x10-No error & G.726, 0x40-No error & G.722*/	
			if((data[data_index] == 0x10) || (data[data_index] == 0x40)){
        IFX_COSIC_DMSG("Valid voice pkt\n");
				valid_voice_pkt[voice_channel]=1;
      }
		};
        /* set dect channel number */
        pTapiData->type_slot |= (xMceiBuffer[voice_channel].iKpiChan & 0x0F);
        /* set sub_type & cycle_index */
#ifndef USE_PLC_BIT
        if(data_len == MAX_TAPI_VOICE_BUFFER_SIZE){
          pTapiData->info_cyclic_index =  0x30;/* Wideband */
        }
        else{
          pTapiData->info_cyclic_index =  0x00;	/* Narrow band */
        }
#else
        /* 0x10-No error & G.726, 0x90-PLC set & G.726
	         0x40-No error & G.722, 0xC0-PLC Set & G.722
	          TODO: need one element(plc_bit) */
		
       if(data_len == MAX_TAPI_VOICE_BUFFER_SIZE){
          	pTapiData->info_cyclic_index =  0x30;/* Wideband */
       	}
       	else{
          	pTapiData->info_cyclic_index =  0x00;	/* Narrow band */
        }

		
		{
	    unsigned char plc_bit = data[data_index] & 0xF0;
			if(valid_voice_pkt[voice_channel] != 0)
        	{
        		if((plc_bit == 0x90) || (plc_bit == 0xC0)){
              IFX_COSIC_DMSG("PLC set\n"); 
	         		pTapiData->info_cyclic_index = 0x80;/* wide band */
        		}
			}	
		}
#endif
        pTapiData->info_cyclic_index |=  data[data_index++] & 0x0F;
        /* burst error info is not used currently, set it to zero */
	      memset(pTapiData->burst_error_info, 0, 4);
#ifdef VOICE_DATA_NIBBLE_SWAP
        if(data_len < MAX_TAPI_VOICE_BUFFER_SIZE){
	  /* voice data nibble swap  ralph_20071222 */
	        for(i=0; i< data_len; i++){
            voice_nibble = 0;
            voice_nibble = (data[data_index+i] << 4) & 0xF0;
            voice_nibble |= (data[data_index+i] >> 4) & 0x0F;
            data[data_index+i] = voice_nibble;
	        }
        } 
#endif
        /* copy the data into voice buffer */
        memcpy(pTapiData->voice_buffer, &data[data_index], data_len);
        valid_channel = 0;
        if((voice_channel < MAX_MCEI) && 
           (xMceiBuffer[voice_channel].iKpiChan != 0xFF)){
	         valid_channel = 1;
        }
        if(valid_channel == 0){
	        IFX_COSIC_DMSG("@%d %d\n", voice_channel, data_len);
          BUFFER_POOL_PUT(pTapiData);
          data_index += data_len;
          break;
        }
#ifdef VOICE_MUTE_SERVICE_CHANGE
        if(voice_mute_cnt){
          BUFFER_POOL_PUT(pTapiData);
          data_index += data_len;
          break;
	}
#endif
        /* write to TAPI KPI */
        /* TODO: increase plc_bit lentgh*/
        ret = IFX_TAPI_KPI_WriteData(IFX_TAPI_KPI_GROUP2 | 
                                     (xMceiBuffer[voice_channel].iKpiChan&0x0F), 
                                     pTapiData, data_len+8);

       if(ret != data_len+8 || ret == IFX_ERROR)
       {
          BUFFER_POOL_PUT(pTapiData);
	       IFX_COSIC_DMSG("IFX_TAPI_KPI_WriteData Error Return ret=%d tapi_CH_arry=%d voice_channel=%d\n", ret,xMceiBuffer[voice_channel].iKpiChan, voice_channel);
         data_index += data_len;
         break;
       }
       data_index += data_len;
    }
    break;
    /* COSIC_VOICE_PACKET ]] */
#ifdef CATIQ_UPLANE
    case COSIC_DECT_FU10_PACKET:  
    {
      /* clear union buffer */
      memset(&cosic_data_packet, 0, sizeof(cosic_data_packet));

      /* Fill buffer with recieved contents */
      data_len=data[data_index++];
      cosic_data_packet.uni.DATA_PACKET.PROCID=data[data_index++];
      cosic_data_packet.uni.DATA_PACKET.MSG= data[data_index++];
      cosic_data_packet.uni.DATA_PACKET.LengthHi=data[data_index++];
      cosic_data_packet.uni.DATA_PACKET.LengthLo=data[data_index++];
      cosic_data_packet.uni.DATA_PACKET.Status=data[data_index++];
      cosic_data_packet.uni.DATA_PACKET.Parameter4=data[data_index++];
#if 1
      data_len= cosic_data_packet.uni.DATA_PACKET.LengthLo;
			 /* Invalid packet length */
      if(data_len > G_PTR_MAX_COUNT){
        printk("<RX-FU10 Error> data len %d\n",data_len);
        return;
      }
#else
      data_len= (cosic_data_packet.uni.DATA_PACKET.LengthHi<<8)+
                 cosic_data_packet.uni.DATA_PACKET.LengthLo;
			 /* Invalid packet length */
      if(data_len > MAX_SPI_DATA_LEN){
        printk("<RX-FU10 Error> data len %d\n",data_len);
        return;
      }
#endif

      for(i=0;i<data_len; i++){
        cosic_data_packet.uni.DATA_PACKET.Uplan[i] = data[data_index++];
      }
      cosic_data_packet.uni.DATA_PACKET.CurrentInc=data[data_index++];
      /*process data packet*/ 
      /*printk("<RX-FU10>");*/
      DECODE_HMAC((HMAC_QUEUES*)&cosic_data_packet.uni.temp_hmac);
    }
    break;
#endif      
    case COSIC_MAC_PACKET: /* to HMAC packet data */
    {
      /* 2 byte header + MAC stream	
	       Type (1 byte), Length (1 byte), MAC Command Stream(MAX 25byte)
         2 byte header
         Type=0x01		8bits, 
         Length=??		8bits,
         MAC command Stream 
      */
      /* clear union buffer */
      memset(&cosic_data_packet, 0, sizeof(cosic_data_packet));
      data_len = data[data_index++];
      /* copy hmac packet */
      for(i=0;i<data_len; i++){
	      cosic_data_packet.uni.temp_hmac[i] = data[data_index++];
      }
#ifdef CATIQ_UPLANE
      /*Copy the last byte to CurrentInc to allow the bigger size of G_PTR_buf */
      cosic_data_packet.uni.temp_hmac_struct.CurrentInc=
                      cosic_data_packet.uni.temp_hmac[data_len-1];
#endif
      i = 0;
      /* command matching */
      while(1){
        if(Message_Map_Table[i][0] == 
           cosic_data_packet.uni.temp_hmac[COSIC_MAC_MSG]){
	         cosic_data_packet.uni.temp_hmac[COSIC_MAC_MSG] = 
                                 Message_Map_Table[ i ][ 1 ];
	         if(cosic_data_packet.uni.temp_hmac[COSIC_MAC_MSG] == MAC_BOOT_IND_LMAC)
					 {
							/*Send Version query debug pkt*/
              if(iNeedVersion == 1){
							  WriteDbgPkts(6,0,0,NULL);
                iNeedVersion=0;
              }
					 } 
          break;
        }
        if(Message_Map_Table[i][0] == 0xFF){
          IFX_COSIC_DMSG("<COSIC>Message %d Not Found In Message_Map_Table\n",cosic_data_packet.uni.temp_hmac[COSIC_MAC_MSG]);
          cosic_data_packet.uni.temp_hmac[COSIC_MAC_MSG] = 0xFF;
          break;
        }
        i++;
      }
      if(cosic_data_packet.uni.temp_hmac[COSIC_MAC_MSG] == 0x80){
        for(i=0; i< 17;i++){
          IFX_COSIC_DMSG("0x%x ",cosic_data_packet.uni.temp_hmac[i]);
        }
        IFX_COSIC_DMSG("\n");
      }
			DECODE_HMAC((HMAC_QUEUES*)&cosic_data_packet.uni.temp_hmac);
    }
    break;
    /* COSIC_MAC_PACKET ]] */
    case COSIC_DECT_SYNC_PACKET:
      IFX_COSIC_DMSG("rcvd sync packet\n");
      /* clear union buffer */
      memset(&cosic_data_packet, 0, sizeof(cosic_data_packet));
      cosic_data_packet.uni.SYNC_PACKET.Sync_Packet_RxTxstatus = 
                                                     data[data_index++];
      cosic_data_packet.uni.SYNC_PACKET.Sync_Packet_TimeStamp = 
                                                     data[data_index++];
      cosic_data_packet.uni.SYNC_PACKET.Sync_Packet_Reserved = 
                                                     data[data_index++];
    break;
    case COSIC_DECT_SETUP_PACKET:
      IFX_COSIC_DMSG("set up packet \n");
      /* clear union buffer */
      memset(&cosic_data_packet, 0, sizeof(cosic_data_packet));
      data_len = data[data_index++];
      cosic_data_packet.uni.SETUP_PACKET.Setup_Packet_SPIMode = 
                                                     data[data_index++];
      for(i=0; i<12;i++){
        cosic_data_packet.uni.SETUP_PACKET.Setup_RXTX_status[i] = 
                                                     data[data_index++];
      } 
      break;
    case COSIC_DECT_DEBUG_PACKET:
      /* clear union buffer */
      memset(&cosic_data_packet, 0, sizeof(cosic_data_packet));
      data_len = data[data_index++];
      /* Message */
      cosic_data_packet.uni.DEBUG_PACKET.debug_info_id = data[data_index++];
      cosic_data_packet.uni.DEBUG_PACKET.rw_indicator = 0;
      data_index++;
      /* Current inc */
      cosic_data_packet.uni.DEBUG_PACKET.CurrentInc = data[data_index++];
      if(data_len > 1){
        for(i=0; i<data_len-3;i++){
          cosic_data_packet.uni.DEBUG_PACKET.G_PTR_buf[i] = data[data_index++];
	        IFX_COSIC_DMSG(" %x ",data[data_index-1]);
        }
      }
			DECODE_DEBUG_FROM_MODULE((DECT_MODULE_DEBUG_INFO*)&cosic_data_packet.uni.DEBUG_PACKET);
      break;
    default:
      IFX_COSIC_DMSG("Default %x %x %x %x %x %x %x %x %x %x\n", data[0],data[1],
                     data[2],data[3],data[4],data[5],data[6],data[7],
                     data[8],data[9]);
      IFX_COSIC_DMSG("data_index = %d %x %x %x %x %x %x %x %x %x %x\n", 
                      data_index, data[data_index-1], data[data_index],
                      data[data_index+1], data[data_index+2],
                      data[data_index+3],data[data_index+4],
                      data[data_index+5],data[data_index+6],
                      data[data_index+7],data[data_index+8]);
      return;
    }
  }
  return;
}


/******************************************************************
 *  Function Name  : To_LMAC_Data 
 *  Description    : This function packs the data from Danube to
 *                   LMAC
 *  Input Values   : data buffer
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void 
To_LMAC_Data(unsigned char *data)
{
	unsigned char	temp_hmac[sizeof(HMAC_QUEUES)/*43*/] = {0};
  unsigned int	total_len;
  unsigned int	i;
  unsigned int   sendlen;
  unsigned char	voice_packet_cnt = 0;
  unsigned char	lmac_packet_cnt = 0;
  unsigned char	debug_packet_cnt = 0;
  unsigned int data_index;
  HMAC_QUEUES *hmac_ptr;
  /*unsigned char   voice_nibble;*/

  data_index = COSIC_DATA_MSG_TYPE;
  total_len = 0;

  /* GET VOICE DATA */
  for(i=0;i<MAX_MCEI;i++){
    int iVoiceLen=0,j=0, iVoiceInfo;
#ifdef VOICE_MUTE_SERVICE_CHANGE
    if(voice_mute_cnt[i]){
      voice_mute_cnt[i]--;
    }
    if(voice_mute_cnt[i]){
      for(j=0;j<MAX_MCEI;j++){
        if(xMceiBuffer[j].pcVoiceDataBuffer != NULL){
          BUFFER_POOL_PUT(xMceiBuffer[j].pcVoiceDataBuffer);
          /* increase read buffer counter */
          xMceiBuffer[j].pcVoiceDataBuffer = xMceiBuffer[j].pcVoiceDataBuffer1;
          xMceiBuffer[j].pcVoiceDataBuffer1 = NULL;
        }
      }
      if(j==MAX_MCEI){
        break;
      }
    }
#endif
    if(xMceiBuffer[i].pcVoiceDataBuffer != NULL){
#ifdef CONFIG_LTT
      MARK(cosic_event,"long %lu string %s",xMceiBuffer[i].iVBCnt,"4");
#endif
      iVoiceInfo = TAPI_VOICE_PACKET_SUB_TYPE(xMceiBuffer[i].\
                   pcVoiceDataBuffer[TAPI_VOICE_PACKET_SUB_TYPE_INDEX]);
      if(iVoiceInfo==NARROWBAND_PKT){
        if(total_len > (Max_spi_data_len_val - (NARROWBAND_PKT_LENGTH+VOICE_HEADER_LENGTH))){ 
          break;
        }
      }
      else if(iVoiceInfo == WIDEBAND_PKT){
        if(total_len > (Max_spi_data_len_val - (WIDEBAND_PKT_LENGTH+VOICE_HEADER_LENGTH))){ 
          break;
        }
      }
      /* cmd */
      data[data_index++] = COSIC_VOICE_PACKET;
      /* length */
      if(iVoiceInfo == NARROWBAND_PKT){
        iVoiceLen = NARROWBAND_PKT_LENGTH;	/* force setting */
      }
      else if(iVoiceInfo == WIDEBAND_PKT){
        iVoiceLen = WIDEBAND_PKT_LENGTH;	/* force setting */
      }
      /* voice len +  mcei num + voice infor & cyclic */
      data[data_index++] = iVoiceLen + 2;
      /* MCEI num */
      data[data_index++] = i;
      /* voice info + cyclic_index */
      data[data_index] = iVoiceInfo<< 4;
      data[data_index++] |= (TAPI_VOICE_PACKET_SEQ(xMceiBuffer[i].
                       pcVoiceDataBuffer[TAPI_VOICE_PACKET_SEQ_INDEX])& 0x0F);
      /* voice stream */
      for(j =0; j<iVoiceLen; j++){
#ifdef VOICE_DATA_NIBBLE_SWAP
        if(iVoiceLen < MAX_TAPI_VOICE_BUFFER_SIZE){
          data[data_index++]  = ((xMceiBuffer[i].\
             pcVoiceDataBuffer[TAPI_VOICE_PACKET_HEADER_LEN+j]<<4)&0xF0)|
             ((xMceiBuffer[i].\
             pcVoiceDataBuffer[TAPI_VOICE_PACKET_HEADER_LEN+j]>>4)&0x0F);
        }
        else{
          data[data_index++] = xMceiBuffer[i].\
                      pcVoiceDataBuffer[TAPI_VOICE_PACKET_HEADER_LEN + j];
        }
#else
        data[data_index++] = xMceiBuffer[i].
                 pcVoiceDataBuffer[TAPI_VOICE_PACKET_HEADER_LEN + j];
#endif
      }
      BUFFER_POOL_PUT(xMceiBuffer[i].pcVoiceDataBuffer);
      /* increase total length - cmd + length + mcei num + 
       * voice info & cyclic + voice pkt len	*/
      total_len += (iVoiceLen + 4);
#if 1 /* For Min 2 packet, prioritization*/
      /* Changed to if 1 to avoid delay in internal calls */
      xMceiBuffer[i].pcVoiceDataBuffer = xMceiBuffer[i].pcVoiceDataBuffer1;
      xMceiBuffer[i].pcVoiceDataBuffer1 = NULL;
#else
      xMceiBuffer[i].pcVoiceDataBuffer = NULL;
#endif
      voice_packet_cnt++;
      /* transfer 2 voice packet  1 times */
      if(voice_packet_cnt >= 2) 
        break;
    }
  }


  /*  GET MAC data */
  /* just one packet data system have many problem never chane please */
	while(Dect_Drv_Get_Data((HMAC_QUEUES*)&temp_hmac) != DECT_DRV_HMAC_BUF_EMPTY){
 		hmac_ptr = (HMAC_QUEUES*)&temp_hmac;
#ifdef CATIQ_UPLANE
    if(hmac_ptr->MSG==MAC_FU10_DATA_RQ){
      /*printk("<TX FU10>");*/
      data[data_index++] = COSIC_DECT_FU10_PACKET;
      sendlen=data[data_index++] = sizeof(HMAC_QUEUES);
    }
    else{
      data[data_index++] = COSIC_MAC_PACKET;
      /*Copy current inc to correct position*/
      hmac_ptr->G_PTR_buf[11]=hmac_ptr->CurrentInc;
      sendlen=data[data_index++] = 17; /*proc,msg,p1,p2 p3 p4 data[10],currentinc*/
    }
    total_len += (sendlen+2);		/* Data + cmd + length */
#else
    data[data_index++] = COSIC_MAC_PACKET;
    sendlen=data[data_index++] = sizeof(HMAC_QUEUES);
    total_len += ((sizeof(HMAC_QUEUES))+2);		/* HMAC_QUEUES size + cmd + length */
#endif

#ifdef CATIQ_UPLANE
    /*Do message mapping only for non FU10 data*/
    if(hmac_ptr->MSG!=MAC_FU10_DATA_RQ)
    {    
#endif
    i = 0;
    while(1){
      if(Message_Map_Table[i][1]==hmac_ptr->MSG){
        hmac_ptr->MSG = Message_Map_Table[i][0];
#ifdef VOICE_MUTE_SERVICE_CHANGE
        if(hmac_ptr->MSG == LMACCmd_SLOTTYPE_MOD_RQ_HMAC){
					voice_mute_cnt[hmac_ptr->Parameter1] = 100;/*Took from intinix rel 140409*/
          /*voice_mute_cnt = 100;*/
        }
#endif
        break;
      }
      if(Message_Map_Table[i][0]==0xFF){
        IFX_COSIC_DMSG(" 0xFF packet???  hmac_ptr->MSG=0x%x\n",hmac_ptr->MSG);
        hmac_ptr->MSG = 0xFF;
        break;
      }
      i++;
    }
    /* message matching ]] */
#ifdef CATIQ_UPLANE
    }
#endif

    if(hmac_ptr->MSG != 0xFF){
      for(i =0; i<sendlen; i++){
        data[data_index++] = temp_hmac[i];
      }
      lmac_packet_cnt++;
      if(lmac_packet_cnt > 6){
        //printk("over packet, total_len=%d, data_index=%d???\n",total_len,data_index);
      }
    }
    if(total_len < (Max_spi_data_len_val - (2+sizeof(HMAC_QUEUES) ))){
      continue;
    }
    else{
      break;
    }
  }
  if((total_len+2) != data_index){
    printk("MAC/FU10: total_len %d. data_index %d\xd\xa",total_len,data_index);
  }
#if 0 
  /* GET VOICE DATA */
  for(i=0;i<MAX_MCEI;i++){
    int iVoiceLen=0,j=0, iVoiceInfo;
#ifdef VOICE_MUTE_SERVICE_CHANGE
    if(voice_mute_cnt[i]){
      voice_mute_cnt[i]--;
    }
    if(voice_mute_cnt[i]){
      for(j=0;j<MAX_MCEI;j++){
        if(xMceiBuffer[j].pcVoiceDataBuffer != NULL){
          BUFFER_POOL_PUT(xMceiBuffer[j].pcVoiceDataBuffer);
          /* increase read buffer counter */
          xMceiBuffer[j].pcVoiceDataBuffer = xMceiBuffer[j].pcVoiceDataBuffer1;
          xMceiBuffer[j].pcVoiceDataBuffer1 = NULL;
        }
      }
      if(j==MAX_MCEI){
        break;
      }
    }
#endif
    if(xMceiBuffer[i].pcVoiceDataBuffer != NULL){
#ifdef CONFIG_LTT
      MARK(cosic_event,"long %lu string %s",xMceiBuffer[i].iVBCnt,"4");
#endif
      iVoiceInfo = TAPI_VOICE_PACKET_SUB_TYPE(xMceiBuffer[i].\
                   pcVoiceDataBuffer[TAPI_VOICE_PACKET_SUB_TYPE_INDEX]);
      if(iVoiceInfo==NARROWBAND_PKT){
        if(total_len > (Max_spi_data_len_val - (NARROWBAND_PKT_LENGTH+VOICE_HEADER_LENGTH))){ 
          break;
        }
      }
      else if(iVoiceInfo == WIDEBAND_PKT){
        if(total_len > (Max_spi_data_len_val - (WIDEBAND_PKT_LENGTH+VOICE_HEADER_LENGTH))){ 
          break;
        }
      }
      /* cmd */
      data[data_index++] = COSIC_VOICE_PACKET;
      /* length */
      if(iVoiceInfo == NARROWBAND_PKT){
        iVoiceLen = NARROWBAND_PKT_LENGTH;	/* force setting */
      }
      else if(iVoiceInfo == WIDEBAND_PKT){
        iVoiceLen = WIDEBAND_PKT_LENGTH;	/* force setting */
      }
      /* voice len +  mcei num + voice infor & cyclic */
      data[data_index++] = iVoiceLen + 2;
      /* MCEI num */
      data[data_index++] = i;
      /* voice info + cyclic_index */
      data[data_index] = iVoiceInfo<< 4;
      data[data_index++] |= (TAPI_VOICE_PACKET_SEQ(xMceiBuffer[i].
                       pcVoiceDataBuffer[TAPI_VOICE_PACKET_SEQ_INDEX])& 0x0F);
      /* voice stream */
      for(j =0; j<iVoiceLen; j++){
#ifdef VOICE_DATA_NIBBLE_SWAP
        if(iVoiceLen < MAX_TAPI_VOICE_BUFFER_SIZE){
          data[data_index++]  = ((xMceiBuffer[i].\
             pcVoiceDataBuffer[TAPI_VOICE_PACKET_HEADER_LEN+j]<<4)&0xF0)|
             ((xMceiBuffer[i].\
             pcVoiceDataBuffer[TAPI_VOICE_PACKET_HEADER_LEN+j]>>4)&0x0F);
        }
        else{
          data[data_index++] = xMceiBuffer[i].\
                      pcVoiceDataBuffer[TAPI_VOICE_PACKET_HEADER_LEN + j];
        }
#else
        data[data_index++] = xMceiBuffer[i].
                 pcVoiceDataBuffer[TAPI_VOICE_PACKET_HEADER_LEN + j];
#endif
      }
      BUFFER_POOL_PUT(xMceiBuffer[i].pcVoiceDataBuffer);
      /* increase total length - cmd + length + mcei num + 
       * voice info & cyclic + voice pkt len	*/
      total_len += (iVoiceLen + 4);
#if 1 /* For Min 2 packet, prioritization*/
      /* Changed to if 1 to avoid delay in internal calls */
      xMceiBuffer[i].pcVoiceDataBuffer = xMceiBuffer[i].pcVoiceDataBuffer1;
      xMceiBuffer[i].pcVoiceDataBuffer1 = NULL;
#else
      xMceiBuffer[i].pcVoiceDataBuffer = NULL;
#endif
      voice_packet_cnt++;
      /* transfer 2 voice packet  1 times */
#if (MAX_SPI_DATA_LEN > 256)
      if(voice_packet_cnt >= 4) 
#else
      if(voice_packet_cnt >= 3) 
#endif
        break;
    }
  }
#endif

  if(total_len < (Max_spi_data_len_val - 50)) {
    /* GET Debug data */
	  while(Dect_Debug_Get_Data((DECT_MODULE_DEBUG_INFO*)&temp_hmac) != DECT_DRV_HMAC_BUF_EMPTY){
    /*while(Dect_Debug_Get_Data(temp_hmac) != DECT_DRV_HMAC_BUF_EMPTY){*/
      IFX_COSIC_DMSG("GET DEBUG INFOMATION\n");
      /* HMAC_QUEUES size + cmd + length */
      total_len += ((sizeof(DECT_MODULE_DEBUG_INFO))+2);
      data[data_index] = COSIC_DECT_DEBUG_PACKET;
      data_index++;
      data[data_index] = sizeof(DECT_MODULE_DEBUG_INFO);
      data_index++;
      /* message matching [[ */
		  hmac_ptr = (HMAC_QUEUES*)&temp_hmac;
      /*hmac_ptr = temp_hmac;*/
      for(i =0; i<sizeof(DECT_MODULE_DEBUG_INFO); i++){
        data[data_index++] = temp_hmac[i];
      }
      debug_packet_cnt++;
      if(total_len > (Max_spi_data_len_val - 50)) break;
      if(debug_packet_cnt > 6){
        IFX_COSIC_DMSG("over packet???\n");
        break;
      }
    }
  }
  /* TX dummy data for rx tx interrupt sync */
  if(total_len == 0){
    total_len = 0x02;
    data[data_index++] = COSIC_MAC_PACKET;
    data[data_index++] = 0x00;
  }
    if(vucDriverMode == DECT_DRV_SET_LOADER_MODE){
		  data[data_index++] = 0xFF;
    }

  data[COSIC_DATA_HIGH_LENGTH] = ((total_len & 0xFF00) >> 8);
  data[COSIC_DATA_LOW_LENGTH] = total_len & 0x00FF;
}
#ifndef CONFIG_DANUBE
/******************************************************************
 *  Function Name  : gp_onu_gpio_write_with_bit_mask 
 *  Description    : This function OR's the GPIO register with
 *                   updated value after masking with bitmask
 *  Input Values   : addr - Register address
 *                   value - Value to be ORed ro existing content
 *                   bit_mask - Bit mask to be applied on given register
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
static void 
gp_onu_gpio_write_with_bit_mask(volatile unsigned long *addr, 
                                volatile unsigned long value,
                                volatile unsigned long bit_mask)
{
  volatile unsigned long reg_value;
  reg_value = *addr;
  reg_value &= (~bit_mask);
  reg_value |= value;
  *addr = reg_value;
  return;
}
#endif

/******************************************************************
 *  Function Name  : Dect_GPIO_interrupt
 *  Description    : LMAC GPIO interrupt routine 
 *  Input Values   : irq - Interrupt req number
 *                   dev_id device id
 *  Output Values  : None
 *  Return Value   : Status of IRQ handling
 *  Notes          : 1. occur LMAC GPIO interrupt
 *                   2. clear GPIO interrupt
 *                   3. schedule tasklet/wakeup cosic thread
 * ****************************************************************/
irqreturn_t  Dect_GPIO_interrupt(int irq, void *dev_id) 
{ 
  /* Acknowledge the interrupt  */
	ACKNOWLEDGE_IRQ;
	gw_stats.uiInt++;/*Total number of interrupts got from cosic modem*/
  /* for waiting dect application active */
#if 0
  if(starting_cnt < 2000){		
    starting_cnt++;
    return IRQ_HANDLED;
  }
#endif
	if(IntEdgFlg)
	{
		/*Rising edge so next should be for falling edge*/
		CONFIGURE_FALLING_EDGE_INT;
		IntEdgFlg =0;/*Flag set for next int i,e falling edge*/

	  if(atomic_read(&vuiIgnoreint)== 1){
			if(atomic_read(&vuiGwDbgFlag)==1){
    				if(iCosicMode){
       				gw_stats.uiLossRx++; /*Total number of rising interrupt ignored due to Rx operation @ GW means Rising int comes before RxCB*/
	 					}
	 				else{
       			gw_stats.uiLossTx++;/*Total number of rising interrupts ignored due to Tx operation @ GW means Rising int comes before TxCB*/
	 				}
				}
			return IRQ_HANDLED;
		}
    if((iDriverStatus  == MAC_BOOT_IND_LMAC) && (viSPILocked))
		{
  		atomic_set(&vuiIgnoreint,1);
			i_CosicData1 =1;
  		tasklet_hi_schedule(&x_IFX_Cosic_Tasklet1);/*schedule tasklet for rising edge unlock*/
			viSPIUnLockSchedule =0;
		}
		return IRQ_HANDLED;
	}
	else
	{
		/*falling edge so next should be for rising edge*/
		CONFIGURE_RISING_EDGE_INT;
		IntEdgFlg =1;/*Flag set for next int i,e Rising edge*/
		gw_stats.uiTmpInt++;/*Total number of falling interrupts used to calculate interrupts interval missed*/

	 if(atomic_read(&vuiIgnoreint)==1){
    return IRQ_HANDLED;
  	}
	}
	/* wake up cosic thread */
  Dect_mutex_free = 1;

	/*No Tx or Rx is going on; but still SPI locked for falling edge;So unlock and return 
		so that other should get a chance to access SPI*/
  if((iDriverStatus  == MAC_BOOT_IND_LMAC) && (viSPILocked))
	{
  	atomic_set(&vuiIgnoreint,1);
		i_CosicData1 =0;
  	tasklet_hi_schedule(&x_IFX_Cosic_Tasklet1);/*schedule tasklet for falling edge unlock*/
		return IRQ_HANDLED;
	}

  atomic_set(&vuiIgnoreint,1);
  tasklet_hi_schedule(&x_IFX_Cosic_Tasklet);/*schedule tasklet for falling edge Tx/Rx*/
	viSPIUnLockSchedule =1;
  return IRQ_HANDLED;
} 

/******************************************************************
 *  Function Name  : trigger_cosic_driver
 *  Description    : This function triggers the cosic driver
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          :
 * ****************************************************************/
void trigger_cosic_driver( void )
{
  Dect_mutex_free = 1;
  tasklet_hi_schedule(&x_IFX_Cosic_Tasklet);
}


/******************************************************************
 *  Function Name  : gp_onu_enable_external_interrupt 
 *  Description    : This function enables the GPIO interrupt
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void gp_onu_enable_external_interrupt(void)
{
#ifdef CONFIG_AMAZON_S
	/* setting falling edge setting */
	gp_onu_gpio_write_with_bit_mask((unsigned long*)AMAZON_S_ICU_EIU_EXIN_C, (0x02000),(0x07000));
                                                                                
	/* external 5 interrupt enable */
	gp_onu_gpio_write_with_bit_mask((unsigned long*)AMAZON_S_ICU_EIU_INEN, (8),(8));
#elif defined CONFIG_AR9
  /* setting falling edge setting */
  gp_onu_gpio_write_with_bit_mask((unsigned long*)IFX_ICU_EIU_EXIN_C, (0x02000),(0x07000));
	IntEdgFlg = 0; /*Indicates falling edge*/
  /* external 5 interrupt enable */
  gp_onu_gpio_write_with_bit_mask((unsigned long*)IFX_ICU_EIU_INEN, (8),(8));
#elif defined(CONFIG_VR9)
  /* setting falling edge setting */
  /* ctc changed to EXIN1 from EXIN5 */
  /* gp_onu_gpio_write_with_bit_mask((unsigned long*)IFX_ICU_EIU_EXIN_C, (0x0200000),(0x0700000)); */
  gp_onu_gpio_write_with_bit_mask((unsigned long*)IFX_ICU_EIU_EXIN_C, (0x0000020),(0x0000070));
	IntEdgFlg = 0; /*Indicates falling edge*/
  /* external 5 interrupt enable */
	/* ctc changed to EXIN1 */
  /* gp_onu_gpio_write_with_bit_mask((unsigned long*)IFX_ICU_EIU_INEN, (0x32),(0x32)); */
  gp_onu_gpio_write_with_bit_mask((unsigned long*)IFX_ICU_EIU_INEN, (0x02),(0x02));
#elif defined(CONFIG_AR10)
  /* setting falling edge setting */
  gp_onu_gpio_write_with_bit_mask((unsigned long*)IFX_ICU_EIU_EXIN_C, (0x0200000),(0x0700000));
	IntEdgFlg = 0; /*Indicates falling edge*/
  /* external 5 interrupt enable */
  gp_onu_gpio_write_with_bit_mask((unsigned long*)IFX_ICU_EIU_INEN, (0x32),(0x32));
#endif	
}
/******************************************************************
 *  Function Name  : gp_onu_disable_external_interrupt 
 *  Description    : This function disables the GPIO interrupt
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void gp_onu_disable_external_interrupt(void)
{
	/* Disable interrupt */
#ifdef CONFIG_AMAZON_S
	gp_onu_gpio_write_with_bit_mask((unsigned
	long*)AMAZON_S_ICU_EIU_INEN, (0),(8));
#elif defined CONFIG_AR9
	gp_onu_gpio_write_with_bit_mask((unsigned long*)IFX_ICU_EIU_INEN, (0),(8));
#elif defined(CONFIG_VR9)
	/* ctc changed to EXIN1 */
	/* gp_onu_gpio_write_with_bit_mask((unsigned long*)IFX_ICU_EIU_INEN, (0),(0x32)); */
	gp_onu_gpio_write_with_bit_mask((unsigned long*)IFX_ICU_EIU_INEN, (0),(0x02));
#elif defined(CONFIG_AR10)
	gp_onu_gpio_write_with_bit_mask((unsigned long*)IFX_ICU_EIU_INEN, (0),(0x32));
#endif
}

/******************************************************************
 *  Function Name  : Dect_int_GPIO_init
 *  Description    : GPIO interrupt setting for LMAC
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void Dect_int_GPIO_init(void)
{

#if defined (CONFIG_AR9) || defined (CONFIG_VR9) || defined (CONFIG_DANUBE) || defined (CONFIG_AR10)
    ifx_gpio_register(IFX_GPIO_MODULE_DECT);
#endif	
#ifdef CONFIG_DANUBE
    ifx_gpio_pin_reserve(21,IFX_GPIO_MODULE_DECT);
		ifx_gpio_altsel0_set(21,IFX_GPIO_MODULE_DECT);
		ifx_gpio_altsel1_set(21,IFX_GPIO_MODULE_DECT);
		ifx_gpio_dir_in_set(21,IFX_GPIO_MODULE_DECT);
		ifx_gpio_open_drain_clear(21,IFX_GPIO_MODULE_DECT);
		ifx_gpio_pudsel_set(21,IFX_GPIO_MODULE_DECT);
		ifx_gpio_puden_set(21,IFX_GPIO_MODULE_DECT);
		ifx_gptu_timer_request(TIMER2A, TIMER_FLAG_16BIT|TIMER_FLAG_COUNTER|TIMER_FLAG_ONCE|\
                           TIMER_FLAG_DOWN|0x40|\
                           TIMER_FLAG_UNSYNC|\
                           TIMER_FLAG_NO_HANDLE, 1, 0, 0);
		/*configure 2A counter for Falling Edge*/
    *(unsigned long *) GPT_CTRL_REG_COUNTER_2A = 0x0000008C;
		/*load the counter value 1 ro 2A counter of GPT*/
    *(unsigned long *) GPT_RELOAD_REG_2A = 0x00000001;
		/*Enable 2A GPT counter*/
    *(unsigned long *) GPT_INT_NODE_EN_REG|= 0x04;
		/*Reloads the counter value when starts + Enable bit of CON will be set for GPT 2A counter*/
    *(unsigned long *) GPT_RUN_REG_2A = 0x00000005;

#endif	
#if 1  /* Reset the modem */
  if(vucDriverMode != DECT_DRV_SET_APP_MODE){ 
    /* 2. reset pin setting low */
    ssc_dect_haredware_reset(0);
    //ssc_dect_debug(0);

    /* 3. delay 10ms -> 50ms*/
    udelay(50000);	
	
    /* 4. reset pin setting high */
    ssc_dect_haredware_reset(1);
  }
#endif
}

/******************************************************************
 *  Function Name  : ssc_dect_debug 
 *  Description    : This function enables the debugging of SSC for
 *                   DEC
 *  Input Values   : Flag indicating Enable/Disable
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
int 
ssc_dect_debug(int on)
{
   if(on){
#ifdef CONFIG_AMAZON_S
	  *(AMAZON_S_GPIO_P1_OUT) = (*AMAZON_S_GPIO_P1_OUT)| (0x00001000);
#elif defined(CONFIG_DANUBE)
     *(IFX_GPIO_P1_OUT) = (*IFX_GPIO_P1_OUT)| (0x00001000);
#elif defined CONFIG_AR9
	  *(IFX_GPIO_P1_OUT) = (*IFX_GPIO_P1_OUT)| (0x00001000);
#endif
   }
   else{
#ifdef CONFIG_AMAZON_S
	  *(AMAZON_S_GPIO_P1_OUT) = (*AMAZON_S_GPIO_P1_OUT) & (0x0000efff);
#elif defined(CONFIG_DANUBE)
     *(IFX_GPIO_P1_OUT) = (*IFX_GPIO_P1_OUT) & (0x0000efff);
#elif defined CONFIG_AR9
	  *(IFX_GPIO_P1_OUT) = (*IFX_GPIO_P1_OUT) & (0x0000efff);
#endif
   }
   return 0;
}

/******************************************************************
 *  Function Name  : ifx_cosic_AsyncLockCB
 *  Description    : Callback invoked on successfully locking the 
 *                   SPI bus
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : SPI bus is locked for our usage, So start receiving
 *                   cosic messages
 * ****************************************************************/
void 
ifx_cosic_AsyncLockCB(int iHandle, int iRetVal)
{
  if( FirstLock == 1)
  {
     FirstLock=2;
     if(vucDriverMode != DECT_DRV_SET_APP_MODE){ 
		 //printk("\n AsyncLockCB: First Lock\n");
    /* 4. reset pin setting high */
    ssc_dect_haredware_reset(1);
  }
     return;
  }
  //printk("\nAsyncLockCB\n");
  if(iCosicMode == 0){
    uiReq = 1;
    ifx_sscCosicTx();
  }
  else{ 
    uiReq = 2;
    ifx_sscCosicRx();
  }
  return;
}

/******************************************************************
 *  Function Name  : ifx_cosic_AsyncTxCB
 *  Description    : Callback invoked on successfully locking the 
 *                   Tx
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void
ifx_cosic_AsyncTxCB(int iHandle, int iRetVal)
{
	unsigned long flags;
  if(uiReq == 1){
    uiResp = 1;
  }
  atomic_set(&vuiIgnoreint,0);
	local_irq_save(flags);
	if(IntEdgFlg == 0)/*means rising edge int got*/
	{
  	ifx_sscCosicUnLock();
		//gw_stats.uiUnLockAtTxCB++;
	}
	local_irq_restore(flags);
  atomic_set(&vuiIgnoreint,0);
  return;
}

void 
ifx_cosic_driver1(unsigned long ulIntEdge)
{
	if(i_CosicData1 == 0)
	{
		gw_stats.uiUnLockFallEdge++;/*Total number of falling interrupts  @ GW used to unlock the SPI*/
	}
	ifx_sscCosicUnLock();
	atomic_set(&vuiIgnoreint,0);
}

/******************************************************************
 *  Function Name  : ifx_cosic_driver
 *  Description    : Cosic driver functionality executed in tasklet
 *                   or thread mode
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void 
ifx_cosic_driver(unsigned long ulIntEdge)
{
	if(atomic_read(&vuiGwDbgFlag)==1)
  {
		unsigned int uiTmp=0;
    gw_stats.uiCdc++;
    gw_stats.uiTmpCdc++;
		uiTmp=gw_stats.uiTmpInt - gw_stats.uiTmpCdc;
    if(uiTmp >= 10)
    {
      gw_stats.auiIntLossSeq[9]++;
    }
    else if(uiTmp > 0)
    {
      gw_stats.auiIntLossSeq[uiTmp - 1]++;
    }
		gw_stats.uiTmpInt=gw_stats.uiTmpCdc=0;/*Reset temp int count.*/
  }

  switch(cosic_status){
    case COSIC_THREAD_INIT_STATUS:
    break;
				
    /* COSIC THREAD TRANSFER STATUS  [[ */
    case COSIC_THREAD_TRANSFER_STATUS:
      cosic_status = COSIC_THREAD_RECEIVE_STATUS; /*status change*/
			iCosicMode = 0;
      if(ifx_sscCosicLock()<0){
        cosic_status = COSIC_THREAD_TRANSFER_STATUS;/* status change */
        iCosicMode = 1;
      } 
      break;
      /* COSIC THREAD TRANSFER STATUS  ]] */

      /* COSIC THREAD RECEIVE STATUS [[*/
      case COSIC_THREAD_RECEIVE_STATUS:
      {
        unsigned long before_ssc_op = jiffies;	
        cosic_status = COSIC_THREAD_TRANSFER_STATUS;/* status change */
        iCosicMode = 1;
       if(ifx_sscCosicLock()<0){
          cosic_status = COSIC_THREAD_RECEIVE_STATUS; /*status change*/
	  iCosicMode = 0;
        }
        if(jiffies - before_ssc_op > 1){
          //printk("ssc op is screwed \n");
        }
      }
      break;
      /* COSIC THREAD RECEIE STATUS ]]*/
      
      default:
        printk("Invalid cosic status\n");
        break;
  }
}
/******************************************************************
 *  Function Name  : init_tapi_read_write_buffer 
 *  Description    : This function initializes the TAPI read and write
 *                   buffers
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void 
init_tapi_read_write_buffer(void)
{
  unsigned char i;
  for(i=0;i<MAX_MCEI;i++){
    xMceiBuffer[i].pcVoiceDataBuffer = NULL;
    xMceiBuffer[i].pcVoiceDataBuffer1 = NULL;
    xMceiBuffer[i].iKpiChan = -1; /* Fill some invalid number as of now */
      /* xMceiBuffer[i].acMacBuffer = */
  }
  /* tapi voice wrtie data initial */
  for(i=0; i<MAX_TAPI_BUFFER_CNT; i++){
    memset(&Tapi_voice_write_buffer[i], 0, sizeof(TAPI_VOICE_WRITE_BUFFER));
  }
  Tapi_voice_write_buffer_r_cnt = 0;
  Tapi_voice_write_buffer_w_cnt = 0;
}

/******************************************************************
 *  Function Name  : IFX_COSIC_TapiRead 
 *  Description    : This function is registered with TAPI. TAPI calls 
 *                   this whenever there is some data to be sent 
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void IFX_COSIC_TapiRead(unsigned long ulDummy)
{
  IFX_uint32_t packet_len;
  long ret;
  static unsigned long  iPktCnt;
  unsigned short nKpiChannel_number;
  unsigned char  *voice_buffer;
  unsigned char  more_packet;
  unsigned char  i;
  /* IFX_TAPI_KPI_WaitForData, IFX_TAPI_KPI_ReadData is not EXPORT_SYMBOL *
   * change 4 byte header
   * Type=0	4 bits,  
   * Slot=??	4 bits according to recieved slot number 
   *            (just use channel number directly)
   * Subtype=??	4 bits according to def e.g. G.726=1, G.722=4, PLC=8 
   *            (received packet on Cosic was defined to need BEC)
   * Seq= 	4 bits cyclic counter set to help TAPI FW in case of lost 
   *            packets (missed interrupts), we will have to do this here. 
   * Length=??	2 bytes, 40 for G.726, 80 for G.722 and G.711 to 5 byte 
   *            header	+ payload Type (1 byte), Length (1 byte), 
   *            Channel (1 byte), Voice Info (1 byte), Cyclic Index (1 byte), 
   *            Voice Data (40 or 80 bytes)*/
   do{
     ret = IFX_TAPI_KPI_ReadData (IFX_TAPI_KPI_GROUP2,
                          &nKpiChannel_number,(void**)&voice_buffer,
                          &packet_len, &more_packet);
     if(ret<0){
       //printk(" KPI_ReadData Failed\n");
       return; 
     }
     /* Find MCEI from TAPI KPI channle number */
     for(i=0;i<MAX_MCEI;i++){
       if(xMceiBuffer[i].iKpiChan == (nKpiChannel_number & 0x00FF)){
         break;
       }
     }
     if((i>=MAX_MCEI)||
        (TAPI_VOICE_PACKET_TYPE(voice_buffer[TAPI_VOICE_PACKET_TYPE_INDEX]) 
          != TAPI_B_VOICE_PACKET)){
       BUFFER_POOL_PUT(&voice_buffer[0]);
       if(more_packet){
         continue;
       } 
       else{
         return;
       }
     }
     if((xMceiBuffer[i].pcVoiceDataBuffer != NULL) && 
        (xMceiBuffer[i].pcVoiceDataBuffer1 != NULL)){
#ifdef CONFIG_LTT
      iPktCnt++;
      xMceiBuffer[i].iVBCnt = xMceiBuffer[i].iVB1Cnt;
      xMceiBuffer[i].iVB1Cnt = iPktCnt;    
      MARK(tapi_event, "long %lu string %s",iPktCnt,"1");
#endif
       /* Overwite the first packet and return that to pool */
       BUFFER_POOL_PUT(xMceiBuffer[i].pcVoiceDataBuffer);
       xMceiBuffer[i].pcVoiceDataBuffer = xMceiBuffer[i].pcVoiceDataBuffer1;
       xMceiBuffer[i].pcVoiceDataBuffer1 = &voice_buffer[0];
     }
#if 1
     else if(xMceiBuffer[i].pcVoiceDataBuffer == NULL){
#ifdef CONFIG_LTT
      iPktCnt++;
      xMceiBuffer[i].iVBCnt = iPktCnt;    
      MARK(tapi_event, "long %lu string %s",iPktCnt,"2");
#endif
       xMceiBuffer[i].pcVoiceDataBuffer = &voice_buffer[0];
     }
     else if(xMceiBuffer[i].pcVoiceDataBuffer1 == NULL){
#ifdef CONFIG_LTT
      iPktCnt++;
      xMceiBuffer[i].iVB1Cnt = iPktCnt;    
      MARK(tapi_event, "long %lu string %s",iPktCnt,"3");
#endif
       xMceiBuffer[i].pcVoiceDataBuffer1 = &voice_buffer[0];
     }
#else
     else if(xMceiBuffer[i].pcVoiceDataBuffer1 == NULL){
#ifdef CONFIG_LTT
      iPktCnt++;
      xMceiBuffer[i].iVB1Cnt = iPktCnt;    
      MARK(tapi_event, "long %lu string %s",iPktCnt,"2");
#endif
       xMceiBuffer[i].pcVoiceDataBuffer1 = &voice_buffer[0];
     }
     else if(xMceiBuffer[i].pcVoiceDataBuffer == NULL){
#ifdef CONFIG_LTT
      iPktCnt++;
      xMceiBuffer[i].iVBCnt = xMceiBuffer[i].iVB1Cnt;
      xMceiBuffer[i].iVB1Cnt = iPktCnt;    
      MARK(tapi_event, "long %lu string %s",iPktCnt,"3");
#endif
       xMceiBuffer[i].pcVoiceDataBuffer = xMceiBuffer[i].pcVoiceDataBuffer1;
       xMceiBuffer[i].pcVoiceDataBuffer1 = &voice_buffer[0];
     }
#endif
   }while(more_packet);
}

/******************************************************************
 *  Function Name  : ifx_cosic_AsyncRxCB2 
 *  Description    : This function is called when the complete msg
 *                   is read from the SPI bus. SPI bus has to be 
 *                   locked for certain duration before releasing it 
 *                   for other modules to use.
 *  Input Values   : Handle passed in Async RX call back, Number of 
 *                   bytes recvd
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void
ifx_cosic_AsyncRxCB2(int iHandle, int iRetVal)
{
	unsigned long flags;
  uiResp = 3;
  /* place holder for avoding delay in tasklet mode */
  if(iRetVal == 0){
    printk("ssc receive failed \n");
  	atomic_set(&vuiIgnoreint,0);
		local_irq_save(flags);
		if(IntEdgFlg == 0)/*means rising edge int got*/
  	{
			ifx_sscCosicUnLock();
		}
		local_irq_restore(flags);
  atomic_set(&vuiIgnoreint,0);
  	return;
  }
  if((iRetVal > 0) && (iRetVal < Max_spi_data_len_val)){ 
	if((ssc_cosic_rx_buf[0]==FIRST_BYTE_FROM_MODEM_LOADER_MODE) || (ssc_cosic_rx_buf[0]==FIRST_BYTE_FROM_MODEM_APP_MODE)){
		From_LMAC_Data(&ssc_cosic_rx_buf[1]);
	}
    Cosic_modem_monitoring_timer_ctrl(IFX_DECT_ON);
  }
  else{
    int iTemp;
    for(iTemp=0;iTemp<15;iTemp++){
      printk("Buff %x\t",ssc_cosic_rx_buf[iTemp]);
    }
    printk("\nLen = %d\n",iRetVal); 
  }
  atomic_set(&vuiIgnoreint,0);
	local_irq_save(flags);
	if(IntEdgFlg == 0)/*means rising edge int got*/
  {
		ifx_sscCosicUnLock();
		//gw_stats.uiUnLockAtRxCB++;
	}
	local_irq_restore(flags);
  atomic_set(&vuiIgnoreint,0);

  return;
}
/******************************************************************
 *  Function Name  : ifx_cosic_AsyncRxCB1 
 *  Description    : This function Reads the remaining message
 *                   available on the SPI bus
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : As a first step we read the first 2 bytes to get 
 *                   the message lenght. Once the message size is known 
 *                   remaining information is read
 * ****	************************************************************/
void
ifx_cosic_AsyncRxCB1(int iHandle, int iRetVal)
{
	unsigned long flags;
  int left_len = 0;
  IFX_SSC_ASYNC_CALLBACK_t xSscTaskletcb;
  uiResp = 2;
  xSscTaskletcb.pFunction = ifx_cosic_AsyncRxCB2;
  xSscTaskletcb.functionHandle = (int)spi_dev_handler;
  if(iRetVal == 0){
    printk("ssc receive failed \n");
    atomic_set(&vuiIgnoreint,0);
		local_irq_save(flags);
		if(IntEdgFlg == 0)/*means rising edge int got*/
		{
    	ifx_sscCosicUnLock();
		}
		local_irq_restore(flags);
  atomic_set(&vuiIgnoreint,0);
    return;
  }
  rx_len = (ssc_cosic_rx_buf[1] & 0xff) << 8;
  rx_len |= (ssc_cosic_rx_buf[2] & 0xff);

		/*Invalid pkt*/
  if((rx_len > MAX_SPI_DATA_LEN)){
   #if 0 //ctc
    printk("!");
   #endif
  if(atomic_read(&vuiGwDbgFlag)==1){
    gw_stats.uiInvSpi++;
	}
    atomic_set(&vuiIgnoreint,0);
		local_irq_save(flags);
		if(IntEdgFlg == 0)/*means rising edge int got*/
		{
    	ifx_sscCosicUnLock(); 
		}
		local_irq_restore(flags);
  atomic_set(&vuiIgnoreint,0);
    return;
  }
  if(rx_len > (COSIC_RX_HEADER_LEN -3)){
    left_len = rx_len - (COSIC_RX_HEADER_LEN - 3);
  }
  if(left_len > 0){
    uiReq = 3;
    if(ifx_sscAsyncRx(spi_dev_handler, &xSscTaskletcb, 
       ssc_cosic_rx_buf + COSIC_RX_HEADER_LEN, left_len) < 0){
      printk("ssc receive failed \n");
      atomic_set(&vuiIgnoreint,0);
		local_irq_save(flags);
		if(IntEdgFlg == 0)/*means rising edge int got*/
		{
      ifx_sscCosicUnLock();
		}
		local_irq_restore(flags);
  atomic_set(&vuiIgnoreint,0);
      return;
    }
  }
  else{
			/*Dummy packet section*/
			/*Check for CoC dummy packet from COSIC MODEM; CoC will be always in App mode*/
			/*CoC Dummy packet structure :       0xAA,0x00,0x02,0x01,0x01*/
      /*byte position                       0  ,  1 ,  2 ,  3 ,  4 */
    	if ((ssc_cosic_rx_buf[4] == 1) && (ssc_cosic_rx_buf[0] == 0xAA) && (vucDriverMode ==  DECT_DRV_SET_APP_MODE))
      {
				//printk("\nCoC dummy pkt from Modem\n");
        /*Request for CoC from COSIC modem*/
				vucDriverMode = DECT_DRV_SET_APP_MODE_CoC_RQ;
      }
			/*check for normal dummy pkt from modem to clear CoC*/ 
			/*Normal Dummy packet structure :    0xAA,0x00,0x02,0x01,0x00*/
      /*byte position                       0  ,  1 ,  2 ,  3 ,  4 */
	    else if ((ssc_cosic_rx_buf[4] == 0) && ((vucDriverMode ==  DECT_DRV_SET_APP_MODE_CoC_RQ)|| (vucDriverMode ==  DECT_DRV_SET_APP_MODE_CoC_CNF)))
      {
				//printk("\nNormal dummy pkt from Modem to clear CoC\n");
				/*Change the driver state to Normal AppMode*/
				vucDriverMode = DECT_DRV_SET_APP_MODE;
				/*Activate the COSIC reset timer as we are not in CoC and idealy we should receive dummy pkt
					from modem on a periodic basis. If we dont recv any pkt from modem for a particular time out
					then COSIC modem will be soft reset by Gw. The below function activates that timer */
				Cosic_modem_monitoring_timer_ctrl(IFX_DECT_ON);
      }	
    	/* No more data to be read so unlock the SPI bus */
    	atomic_set(&vuiIgnoreint,0);
			local_irq_save(flags);
			if(IntEdgFlg == 0)/*means rising edge int got*/
			{
    		ifx_sscCosicUnLock();
				//gw_stats.uiUnLockAtRxCB++;
			}
			local_irq_restore(flags);
    	Cosic_modem_monitoring_timer_ctrl(IFX_DECT_ON);
  		atomic_set(&vuiIgnoreint,0);
  }
  return;
}

/******************************************************************
 *  Function Name  : ifx_sscCosicRx 
 *  Description    : Receives Cosic messages on getting and intterupt 
 *                   from GPIO, cosic is in RX mode
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void 
ifx_sscCosicRx(void)
{
	unsigned long flags;
  IFX_SSC_ASYNC_CALLBACK_t xSscTaskletcb;
  xSscTaskletcb.pFunction = ifx_cosic_AsyncRxCB1;
  xSscTaskletcb.functionHandle = (int)spi_dev_handler;
  
  /* First, receive the total packet length */
  if(ifx_sscAsyncRx(spi_dev_handler, &xSscTaskletcb, ssc_cosic_rx_buf, 
     COSIC_RX_HEADER_LEN) < 0){
    printk("ssc receive failed \n");
		local_irq_save(flags);
		if(IntEdgFlg == 0)/*means rising edge int got*/
		{
    	ifx_sscCosicUnLock(); 
		}
		local_irq_restore(flags);
    atomic_set(&vuiIgnoreint,0);
    return;
  }
  /* In case of Async mode the call back gets called in the tasklet context.
   * If Cosic is running in thread mode explicitly call that function */
}
/******************************************************************
 *  Function Name  : ifx_sscCosicTx
 *  Description    : This function sends the messages to cosic modem
 *                   using the SPI. Async/Sync is used based on tasklet
 *                   support
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void 
ifx_sscCosicTx()
{
	unsigned long flags;
	unsigned int  dummy_len,ret;
  unsigned short u16Len =0;
  unsigned char* pucBuf=NULL;
  IFX_SSC_ASYNC_CALLBACK_t xSscTaskletcb;
        if(vucDriverMode == DECT_DRV_SET_LOADER_MODE){
          char ucOpcode1=0,ucOpcode2=0;
		      u16Len = 0;
      	  if( vu16WriteBufTail != vu16WriteBufHead ) {
			/*send user data*/
       		u16Len = (vucWriteBuffer[vu16WriteBufTail][0]<<8) + 
										vucWriteBuffer[vu16WriteBufTail][1];
			u16Len += 2 ; //for 'total size' field
			pucBuf =  vucWriteBuffer[vu16WriteBufTail];
            ucOpcode1 = vucWriteBuffer[vu16WriteBufTail][4];
            ucOpcode2 = vucWriteBuffer[vu16WriteBufTail][5];
		    ++vu16WriteBufTail;
		    if(vu16WriteBufTail == MAX_WRITE_BUFFERS)
			  vu16WriteBufTail = 0;
			pucBuf[u16Len] = 0xFF; //Extra byte... //TODO: Why it is required? Discuss with Hubert
			++u16Len;
			iCosicMode = 0;
      xSscTaskletcb.pFunction = ifx_cosic_AsyncTxCB;
      xSscTaskletcb.functionHandle = (int)spi_dev_handler;
      if(ifx_sscAsyncTx(spi_dev_handler, &xSscTaskletcb, pucBuf,
                 u16Len) < 0){
        atomic_set(&vuiIgnoreint,0);
				ifx_sscCosicUnLock();
        printk("ssc transmit1 failed %d\n", u16Len);
				//return;
      }
              
            if((ucOpcode1 == 0x10) && (ucOpcode2 == 0x08)){
			  //printk("\n: Cosic Firmware Download complete\n");
			  udelay(10000); 
        vucDriverMode = DECT_DRV_SET_APP_MODE;
			  //printk("\n: Cosic Firmware Download complete11111\n");
            }
            return;
		  }
        }
      transmit_len = 0;
      dummy_len = 0;
      ret = 0;

      /* send the data to LMAC */
      if((vucDriverMode == DECT_DRV_SET_APP_MODE_CoC_RQ)||(vucDriverMode == DECT_DRV_SET_APP_MODE_CoC_CNF))
			{
				/*Since under CoC state, send only CoC dummy packet till the CoC state is cleared by Modem by Normal dummy pkt*/
				/*CoC Dummy packet structure :       0x00,0x02,0x01,0x01*/
      	/*byte position                       0  ,  1 ,  2 ,  3 */
				ssc_cosic_tx_buf[0]= 0;
				ssc_cosic_tx_buf[1]= 2;
				ssc_cosic_tx_buf[2]= 1;
				ssc_cosic_tx_buf[3]= 1;
        //printk("\nSending CoC dummy pkt from GW\n");
        if(vucDriverMode == DECT_DRV_SET_APP_MODE_CoC_RQ)
        {
					/*move driver state from Coc-request to CoC-confirm, as we already ack the CoC req from modem*/
        	vucDriverMode = DECT_DRV_SET_APP_MODE_CoC_CNF;
					/*Deactivate the COSIC reset timer as we are in CoC and we don't receive any pkt
					from modem on a periodic basis. If we dont recv any pkt from modem for a particular time out
					then COSIC modem will be soft reset by Gw. The below function deactivates that timer */
					Cosic_modem_monitoring_timer_ctrl(IFX_DECT_OFF);
          //printk("\nGW into CoC CNF mode\n");
				}
			}
			else
			{
      	To_LMAC_Data(ssc_cosic_tx_buf);
			}

      transmit_len = (ssc_cosic_tx_buf[COSIC_DATA_HIGH_LENGTH]<<8);
      transmit_len |= ssc_cosic_tx_buf[COSIC_DATA_LOW_LENGTH];

      if(transmit_len){
	/* ralph_080224 for size 4 unit */
        transmit_len += 2;

        if(vucDriverMode != DECT_DRV_SET_LOADER_MODE){
          dummy_len = transmit_len % 4;
          if(dummy_len){
            transmit_len = transmit_len + (4 - dummy_len);/*unit 4byte*/
          }
        }
        if(vucDriverMode == DECT_DRV_SET_LOADER_MODE){
            if(transmit_len == 4){
                transmit_len++;
            }else{
                printk("In load mode should not transmit anything else\n");
                return;
            }
        }
  xSscTaskletcb.pFunction = ifx_cosic_AsyncTxCB;
  xSscTaskletcb.functionHandle = (int)spi_dev_handler;
  if(ifx_sscAsyncTx(spi_dev_handler, &xSscTaskletcb, ssc_cosic_tx_buf, 
     transmit_len) < 0){
		local_irq_save(flags);
		if(IntEdgFlg == 0)/*means rising edge int got*/
		{
    	ifx_sscCosicUnLock();
		}
		local_irq_restore(flags);
    atomic_set(&vuiIgnoreint,0);
    printk("ssc transmit2 failed %d\n", transmit_len);
  }
	}	
 	if(viDisableIrq==1){
		viDisableIrq=2;
	}

  return;
}
/******************************************************************
 *  Function Name  : ifx_sscCosicUnLock
 *  Description    : This function Unlocks the SPI bus
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
void 
ifx_sscCosicUnLock()
{
  /* Ensure that the callback is called before the unlock */
  if(uiReq != uiResp){
    return;
  }
  if((iDriverStatus  == MAC_BOOT_IND_LMAC) && (viSPILocked))
	{
		ifx_sscAsyncUnLock(spi_dev_handler);
		viSPILocked=0;
		if(viDisableIrq==2){
			gp_onu_disable_external_interrupt();
		}
	}

 viSPILocked1=0;
	return;
}
/******************************************************************
 *  Function Name  : ifx_sscCosicLock
 *  Description    : This function locks the SPI bus
 *  Input Values   : None
 *  Output Values  : None
 *  Return Value   : None
 *  Notes          : 
 * ****************************************************************/
int 
ifx_sscCosicLock()
{
  IFX_SSC_ASYNC_CALLBACK_t xSscTaskletcb;
	viSPILocked1=1;
  viSPILockedCB =1;
  if((iDriverStatus  != MAC_BOOT_IND_LMAC) && (viSPILocked))
  {
    //printk("\ncalling LOCAL-Lock\n");
    ssc_dect_cs(0,0);
    //printk("\nlock: LOCAL\n");
    ifx_cosic_AsyncLockCB(((int)spi_dev_handler),0);
    return 0;
  }
  //printk("\nCalling AsyncLock\n");
	viSPILocked=1;
  xSscTaskletcb.pFunction = ifx_cosic_AsyncLockCB;
  xSscTaskletcb.functionHandle = (int)spi_dev_handler;
  if(ifx_sscAsyncLock(spi_dev_handler,&xSscTaskletcb) < 0){
    //viSPILocked=0;
    viSPILockedCB =0;
    atomic_set(&vuiIgnoreint,0);
    return -1;
  }
  return 0;
}

#endif /* DRV_DECT_COSIC_DRV_C */



			/* Invalid packet length */
