/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information , see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
#ifndef DRV_DECT_C
#define DRV_DECT_C

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
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#include <asm/irq.h>

#include <linux/mm.h>
#include <linux/spinlock.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <linux/version.h>

#ifdef CONFIG_LTT
#include <linux/sched.h>
#include <asm/atomic.h>
#include <linux/marker.h>
#include <linux/inetdevice.h>
#include <net/sock.h>
#include <ltt/ltt-facility-select-default.h>
#include <ltt/ltt-facility-select-network_ip_interface.h>
#include <ltt/ltt-facility-network.h>
#include <ltt/ltt-facility-socket.h>
#include <ltt/ltt-facility-network_ip_interface.h>
#endif


#include <linux/wait.h>
#include <linux/signal.h>
#include <linux/smp_lock.h> 
/* little / big endian */
#include <asm/byteorder.h>
#include <asm/system.h>
#ifdef CONFIG_AMAZON_S

#include <asm/amazon_s/amazon_s.h>
#include <asm/amazon_s/irq.h>
#include <asm/amazon_s/amazon_s_ssc.h>

#elif defined CONFIG_DANUBE

#include <asm-mips/ifx/danube/danube.h>
#include <asm-mips/ifx/danube/irq.h>
#include <asm-mips/ifx/ifx_gpio.h>
#include <asm/ifx/ifx_ssc.h>
#include <asm-mips/ifx/ifx_led.h>
const int dect_gpio_module_id = IFX_GPIO_MODULE_DECT;


#elif defined CONFIG_AR9
#include <asm-mips/ifx/ar9/ar9.h>
#include <asm-mips/ifx/ar9/irq.h>
#include <asm-mips/ifx/ifx_gpio.h>
#include <asm/ifx/ifx_ssc.h>
const int dect_gpio_module_id = IFX_GPIO_MODULE_DECT;
#elif defined CONFIG_AR10
#include <asm-mips/ifx/ar10/ar10.h>
#include <asm-mips/ifx/ar10/irq.h>
#include <asm-mips/ifx/ifx_gpio.h>
#ifdef DECT_USE_USIF
#include <asm/ifx/ifx_usif_spi.h>
#else
#include <asm/ifx/ifx_ssc.h>
#endif
const int dect_gpio_module_id = IFX_GPIO_MODULE_DECT;
#elif defined CONFIG_VR9
#include <asm-mips/ifx/vr9/vr9.h>
#include <asm-mips/ifx/vr9/irq.h>
#include <asm-mips/ifx/ifx_gpio.h>
#ifdef DECT_USE_USIF
#include <asm/ifx/ifx_usif_spi.h>
#else
#include <asm/ifx/ifx_ssc.h>
#endif
const int dect_gpio_module_id = IFX_GPIO_MODULE_DECT;
#endif

#include "lib_bufferpool.h"
#include<asm-mips/ifx/ifx_gptu.h>
#include "drv_dect.h"        /* The definition of the DECT Driver interfaces. */
#include "fhmac.h"
#include "fmbc.h"
#include "FMAC_DEF.H"
#include "FDEF.H"
#include "fdebug.h"
#include "drv_dect_cosic_drv.h"
#include "drv_tapi_io.h"
#include "drv_tapi_kio.h"


MODULE_AUTHOR("LANTIQ");
MODULE_DESCRIPTION("DECT Driver");
MODULE_LICENSE("GPL");
/* ================================  */
/* MCEI table                                                          */
/* ================================ */
extern MCEI_TAB_STRUC Mcei_Table[MAX_MCEI];
extern void EnableGwDebugStats(void);
extern void DisableGwDebugStats(void);
extern void ResetGwDebugStats(void);
extern void GetModemDebugStats(void);
extern void ResetModemDebugStats(void);
extern int WriteDbgPkts(unsigned char debug_info_id, unsigned char rw_indicator, \
                  unsigned char CurrentInc, unsigned char *G_PTR_buf);
extern modem_dbg_stats modem_stats;
extern modem_dbg_lbn_stats lbn_stats[6];
extern dect_version version; 
extern gw_dbg_stats gw_stats;
extern unsigned int vuiDbgSendFlags;
extern atomic_t vuiGwDbgFlag;
extern x_IFX_Mcei_Buffer xMceiBuffer[MAX_MCEI];
extern void ifx_sscCosicLock(void);
extern int viSPILockedCB;
extern int viSPILocked;
extern int FirstLock;
int viCSStatus=0;
int viDisableIrq;
#define ENHANCED_DELAY
#ifdef ENHANCED_DELAY
unsigned int vuiDealy=1;
#endif
#ifdef CONFIG_AMAZON_S
#define ifx_sscAllocConnection amazon_s_sscAllocConnection
#define ifx_sscFreeConnection  amazon_s_sscFreeConnection
#define ifx_ssc_cs_low  amazon_s_ssc_cs_low
#define ifx_ssc_cs_high  amazon_s_ssc_cs_high
#endif



#define DECT_DRV_MAJOR             213
#define DECT_DRV_MINOR_0           0
#define DECT_DRV_MINOR_1           1

#define DECT_DEVICE_NAME "dect_drv"
#define DECT_VERSION "3.1.0.0"
#define MAX_SEND_BUFFER_COUNT  		80
#define MAX_RECEIVED_BUFFER_COUNT	80

#define MAX_DEBUG_INFO_COUNT	    10

static int open_dect_count = 0;
static int create_dir = 0;
int Dect_excpFlag = 0;
wait_queue_head_t Dect_WakeupList;

#ifdef TWINPASS
#define DECT_IRQ_NUM    INT_NUM_IM4_IRL30
#define DECT_CS IFX_SSC_WHBGPOSTAT_OUT0_POS
#elif defined(CONFIG_AMAZON_S)
// connect JP73 2 and 3  GPIO39 Using P2.7---> EXIN 3--> IM1_IRL0
#define DECT_IRQ_NUM    INT_NUM_IM1_IRL0
#define DECT_CS IFX_SSC_WHBGPOSTAT_OUT2_POS//FIXME CS pin, now configure as SPI_CS3
#elif defined(CONFIG_DANUBE)
#define DECT_IRQ_NUM    INT_NUM_IM1_IRL2
#define DECT_CS IFX_SSC_WHBGPOSTAT_OUT5_POS
#elif  defined CONFIG_IFX_GW188
#define DECT_IRQ_NUM    INT_NUM_IM1_IRL0
#define DECT_CS IFX_SSC_WHBGPOSTAT_OUT0_POS//Not used
#elif defined CONFIG_AR9
//connect JP73 2 and 3  GPIO39 Using P2.7---> EXIN 3--> IM1_IRL0
#define DECT_IRQ_NUM    INT_NUM_IM1_IRL0
#define DECT_CS IFX_SSC_WHBGPOSTAT_OUT2_POS//FIXME CS pin, now configure as SPI_CS3
#elif defined CONFIG_AR10
//GPIO9 Using P0.9---> EXIN 5--> INT_NUM_IM1_IRL2 , SPI_CS1
#define DECT_IRQ_NUM    INT_NUM_IM1_IRL2
#ifdef DECT_USE_USIF //we are using SSC-SPI.
#define DECT_CS IFX_SSC_WHBGPOSTAT_OUT0_POS//SPI_CS5 for USIF aswell??
#else
#define DECT_CS IFX_SSC_WHBGPOSTAT_OUT1_POS//GPIO15-SPI_CS1
#endif
#elif defined CONFIG_VR9
#if 0 //ctc
//connect JP61 1 and 2  GPIO9 Using P0.9---> EXIN 5--> IM1_IRL2
#define DECT_IRQ_NUM    INT_NUM_IM1_IRL2
#else
//GPIO1 Using P0.1---> EXIN 1--> IM3_IRL31, SPI_CS5
#define DECT_IRQ_NUM    INT_NUM_IM3_IRL31
#endif
#ifdef DECT_USE_USIF 
#define DECT_CS IFX_SSC_WHBGPOSTAT_OUT0_POS//FIXME CS pin, now configure as SPI_CS3
#else
#if 0 //ctc
#define DECT_CS IFX_SSC_WHBGPOSTAT_OUT1_POS//FIXME CS pin, now configure as SPI_CS3
#else
#define DECT_CS IFX_SSC_WHBGPOSTAT_OUT4_POS//FIXME CS pin, now configure as SPI_CS5
#endif
#endif
#endif /* Twinpass/Danube */
/*Initialise it in open*/
int	dect_gpio_intnum = -1;				/* valiable interrupt number */
extern int iCosicMode; /* Tx = 0, Rx = 1 */


/*RECEIVED BUFFER */
HMAC_QUEUES send_to_stack_buf[MAX_RECEIVED_BUFFER_COUNT];
static int send_to_stack_buf_r_count = 0;
static int send_to_stack_buf_w_count = 0;
static unsigned int uiOverflow_r;
static unsigned int uiOverflow_w;


/* SEND BUFFER */
HMAC_QUEUES send_to_lmac_buf[MAX_SEND_BUFFER_COUNT];
static int send_to_lmac_buf_r_count = 0;
static int send_to_lmac_buf_w_count = 0;

unsigned char vucDriverMode;
unsigned char vucReadBuffer[MAX_READ_BUFFFERS][COSIC_LOADER_PACKET_SIZE];
unsigned char vucWriteBuffer[MAX_WRITE_BUFFERS][COSIC_LOADER_PACKET_SIZE];
unsigned short vu16ReadBufHead;
unsigned short vu16ReadBufTail;
unsigned short vu16WriteBufHead;
unsigned short vu16WriteBufTail;

u32 ssc_dect_cs(u32 on,u32 cs_data);
u32 ssc_dect_cs_toggle(void);
/* SEND DEBUG BUFFER */
DECT_MODULE_DEBUG_INFO send_to_debug_info_buf[MAX_DEBUG_INFO_COUNT];
static int send_to_debug_info_buf_r_count = 0;
static int send_to_debug_info_buf_w_count = 0;

/* RECEIVE DEBUG BUFFER */
DECT_MODULE_DEBUG_INFO receive_to_debug_info_buf[MAX_DEBUG_INFO_COUNT];
static int receive_to_debug_info_buf_r_count = 0;
static int receive_to_debug_info_buf_w_count = 0;

IFX_SSC_HANDLE *spi_dev_handler=NULL;

extern  int ifx_ssc_cs_low(u32 pin);
extern  int ifx_ssc_cs_high(u32 pin);
BUFFERPOOL *pTapiWriteBufferPool;
#ifdef CONFIG_DANUBE
extern void *g_dect_isdn_fxs_reset_trigger;
#endif
#if CONFIG_PROC_FS
static struct proc_dir_entry *dect_proc_dir;
static struct proc_dir_entry *dect_version_file,*dect_gw_stats_file,*dect_modem_stats_file;
static struct proc_dir_entry *dect_channel_stats_file;

int dect_drv_read_proc(char *page, char **start, off_t off,
								int count, int *eof, void *data);

int dect_drv_read_version_proc(char *buf);
int dect_drv_read_gw_stats_proc(char *buf);
int dect_drv_read_modem_stats_proc(char *buf);
int dect_drv_read_channel_stats_proc(char *buf);

int dect_drv_write_proc(struct file *file,const char *buffer,unsigned long count,void *data);

#endif /* CONFIG_PROC_FS */

#ifdef CONFIG_LTT

void probe_cosic_event(const char *format, ...)
{
 va_list ap;
 unsigned long iPktCount;
 const char *mystr;
 
 va_start(ap, format);
 iPktCount = va_arg(ap,typeof(iPktCount));
 mystr = va_arg(ap, typeof(mystr));
// printk(KERN_INFO "iPktCount %lu, string %s\n", iPktCount, mystr);
 /* Call tracer */
 trace_socket_call(4, iPktCount);
}

void probe_tapi_event(const char *format, ...)
{
 va_list ap;
 unsigned long iPktCount;
 const char *mystr;
 
 va_start(ap, format);
 iPktCount = va_arg(ap,typeof(iPktCount));
 mystr = va_arg(ap, typeof(mystr));
// printk(KERN_INFO "iPktCount %lu, string %s\n", iPktCount, mystr);
 /* Call tracer */
 if(strcmp(mystr,"1")==0){
   trace_socket_call(1, iPktCount);
 }
 else if(strcmp(mystr,"2")==0){
   trace_socket_call(2, iPktCount);
 }
 else{
   trace_socket_call(3, iPktCount);
 }
}


#endif




/*******************************************************************************
Description:	HMAC to stack data
Arguments: 
Note: 
*******************************************************************************/
void Dect_SendtoStack(HMAC_QUEUES* data)
{
#ifdef CONFIG_LTT
  int i,j;
#endif
  memcpy((unsigned char*)&send_to_stack_buf[send_to_stack_buf_w_count++], data, sizeof(HMAC_QUEUES));
  if (send_to_stack_buf_w_count == MAX_RECEIVED_BUFFER_COUNT)
    send_to_stack_buf_w_count = 0;


  if (send_to_stack_buf_w_count == send_to_stack_buf_r_count)
  {
    printk("Critical Error!!! to stack buffer overflow!! Erase all data\n");

#ifdef CONFIG_LTT
    //OSGI dump packets
    for(i=0; i< MAX_RECEIVED_BUFFER_COUNT; i++){
      printk("MSG:%d R/WCount:%d",send_to_stack_buf[i].MSG,
                        send_to_stack_buf_w_count);
      for(j=0;j<20;j++){
        printk("[0x%x]",send_to_stack_buf[i].G_PTR_buf[j]);
      }
      printk("\n");
    }
#endif
    memset(&send_to_stack_buf[0], 0x00, sizeof(send_to_stack_buf));
    send_to_stack_buf_r_count = 0;
    send_to_stack_buf_w_count = 0;    
  }

  Dect_excpFlag = 1;
	wake_up_interruptible(&Dect_WakeupList);
}

/*******************************************************************************
Description:	HMAC to lmac data
Arguments: 
Note: 
*******************************************************************************/
void Dect_SendtoLMAC(HMAC_QUEUES* data)
{
  if(viDisableIrq==2){
		return;
	}
  memcpy((unsigned char*)&send_to_lmac_buf[send_to_lmac_buf_w_count++], data, sizeof(HMAC_QUEUES));
  if (send_to_lmac_buf_w_count == MAX_SEND_BUFFER_COUNT){
    send_to_lmac_buf_w_count = 0;
		uiOverflow_w++;
	}

  if ((send_to_lmac_buf_w_count == send_to_lmac_buf_r_count)&&(uiOverflow_r == uiOverflow_w-1))
  {
    printk("Critical Error!!! to lmac buffer overflow!! Erase all data\n");
    memset(&send_to_lmac_buf[0], 0x00, sizeof(send_to_lmac_buf));
    send_to_lmac_buf_r_count = 0;
    send_to_lmac_buf_w_count = 0;
		uiOverflow_w=0;
		uiOverflow_r=0;
  }
}

/*******************************************************************************
Description:	Get data for Cosic drvier
Arguments: 
Note: 
*******************************************************************************/
HMAC_LMAC_RETURN_VALUE Dect_Drv_Get_Data(HMAC_QUEUES* data)
{
  HMAC_LMAC_RETURN_VALUE nRet = DECT_DRV_HMAC_BUF_EMPTY;
  
  if (send_to_lmac_buf_r_count != send_to_lmac_buf_w_count)
  {
    memcpy(data, (unsigned char*)&send_to_lmac_buf[send_to_lmac_buf_r_count], sizeof(HMAC_QUEUES));
    memset((unsigned char*)&send_to_lmac_buf[send_to_lmac_buf_r_count], 0x00, sizeof(HMAC_QUEUES));
    send_to_lmac_buf_r_count++;

    if (send_to_lmac_buf_r_count == MAX_SEND_BUFFER_COUNT){
      send_to_lmac_buf_r_count = 0;
			uiOverflow_r++;
		}
    
    if (send_to_lmac_buf_r_count != send_to_lmac_buf_w_count)
      nRet = DECT_DRV_HMAC_BUF_MORE; //have more data
    else
      nRet = DECT_DRV_HMAC_BUF_LAST; //this data is last
  }
  
  return nRet;
}
EXPORT_SYMBOL(Dect_Drv_Get_Data);



/*******************************************************************************
Description:	HMAC to stack data
Arguments: 
Note: 
*******************************************************************************/
void Dect_DebugSendtoApplication(DECT_MODULE_DEBUG_INFO* data)
{
  memcpy((unsigned char*)&receive_to_debug_info_buf[receive_to_debug_info_buf_w_count++], data, sizeof(DECT_MODULE_DEBUG_INFO));
  if (receive_to_debug_info_buf_w_count == MAX_DEBUG_INFO_COUNT)
    receive_to_debug_info_buf_w_count = 0;


  if (receive_to_debug_info_buf_w_count == receive_to_debug_info_buf_r_count)
  {
    printk("Critical Error!!! to application buffer overflow!! Erase all data\n");
    memset(&receive_to_debug_info_buf[0], 0x00, sizeof(receive_to_debug_info_buf));
    receive_to_debug_info_buf_r_count = 0;
    receive_to_debug_info_buf_w_count = 0;    
  }

  Dect_excpFlag = 1;
	wake_up_interruptible(&Dect_WakeupList);
}


/*******************************************************************************
Description:	HMAC to lmac data
Arguments: 
Note: 
*******************************************************************************/
void Dect_DebugSendtoModule(DECT_MODULE_DEBUG_INFO* data)
{
  if(viDisableIrq==2){
		return;
	}
  memcpy((unsigned char*)&send_to_debug_info_buf[send_to_debug_info_buf_w_count++], data, sizeof(DECT_MODULE_DEBUG_INFO));
  if (send_to_debug_info_buf_w_count == MAX_DEBUG_INFO_COUNT)
    send_to_debug_info_buf_w_count = 0;

  if (send_to_debug_info_buf_w_count == send_to_debug_info_buf_r_count)
  {
    printk("Critical Error!!! to debug buffer overflow!! Erase all data\n");
    memset(&send_to_debug_info_buf[0], 0x00, sizeof(send_to_debug_info_buf));
    send_to_debug_info_buf_r_count = 0;
    send_to_debug_info_buf_w_count = 0;    
  }
}


/*******************************************************************************
Description:	Get data for Cosic drvier
Arguments: 
Note: 
*******************************************************************************/
HMAC_LMAC_RETURN_VALUE Dect_Debug_Get_Data(DECT_MODULE_DEBUG_INFO* data)
{
  HMAC_LMAC_RETURN_VALUE nRet = DECT_DRV_HMAC_BUF_EMPTY;
  
  if (send_to_debug_info_buf_r_count != send_to_debug_info_buf_w_count)
  {
    memcpy(data, (unsigned char*)&send_to_debug_info_buf[send_to_debug_info_buf_r_count], sizeof(DECT_MODULE_DEBUG_INFO));
    memset((unsigned char*)&send_to_debug_info_buf[send_to_debug_info_buf_r_count], 0x00, sizeof(DECT_MODULE_DEBUG_INFO));
    send_to_debug_info_buf_r_count++;

    if (send_to_debug_info_buf_r_count == MAX_DEBUG_INFO_COUNT)
      send_to_debug_info_buf_r_count = 0;
    
    if (send_to_debug_info_buf_r_count != send_to_debug_info_buf_w_count)
      nRet = DECT_DRV_HMAC_BUF_MORE; //have more data
    else
      nRet = DECT_DRV_HMAC_BUF_LAST; //this data is last
  }
  
  return nRet;
}
EXPORT_SYMBOL(Dect_Debug_Get_Data);

void Reset_Hmac_Debug_buffer(void)
{
	memset(&send_to_lmac_buf, 0x00, sizeof(send_to_lmac_buf));
	send_to_lmac_buf_r_count = 0;
	send_to_lmac_buf_w_count = 0;
	
	
	memset(&send_to_stack_buf, 0x00, sizeof(send_to_stack_buf));
	send_to_stack_buf_r_count = 0;
	send_to_stack_buf_w_count = 0;

	memset(&send_to_debug_info_buf, 0x00, sizeof(send_to_debug_info_buf));
	send_to_debug_info_buf_r_count = 0;
	send_to_debug_info_buf_w_count = 0;

	memset(&receive_to_debug_info_buf, 0x00, sizeof(receive_to_debug_info_buf));
	receive_to_debug_info_buf_r_count = 0;
	receive_to_debug_info_buf_w_count = 0;
}

/*
 * Forwards for our methods.
 */
int dect_drv_open   (struct inode *inode, struct file *filp);
int dect_drv_release(struct inode *inode, struct file *filp);
int dect_drv_ioctl  (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
int dect_drv_poll   (struct file *file, poll_table *wait);
int dect_drv_write  (struct file *file_p, char *buf, size_t count, loff_t *ppos);
int dect_drv_read   (struct file *file_p, char *buf, size_t count, loff_t *ppos);

/* 
 * The operations for device node handled.
 */
struct file_operations dect_drv_fops =
{
    owner:
        THIS_MODULE,
    read:
        dect_drv_read,
    write:
        (void*)dect_drv_write,
    poll: 
        (void*)dect_drv_poll,
    ioctl:
        dect_drv_ioctl,
    open:
        dect_drv_open,
    release:
        dect_drv_release
};

/*******************************************************************************
Description:
Arguments:
Note:
*******************************************************************************/
int dect_drv_read(struct file *file_p, char *buf, size_t count, loff_t *ppos)
{
  return 0;
}


/*******************************************************************************
Description:
Arguments:
Note:
*******************************************************************************/
int dect_drv_write(struct file *file_p, char *buf, size_t count, loff_t *ppos)
{
  return 0;
}

/*******************************************************************************
Description:
   The function for the system call "select".
Arguments:
Note:
*******************************************************************************/
int dect_drv_poll(struct file *file_p, poll_table *wait)
{
  int ret = 0;

  /* install the poll queues of events to poll on */
  poll_wait(file_p, &Dect_WakeupList, wait);
  if ((Dect_excpFlag)&&(vucDriverMode == DECT_DRV_SET_LOADER_MODE)){
    if ( vu16ReadBufHead != vu16ReadBufTail )
      {
          ret |= (POLLIN | POLLRDNORM);
      }
    Dect_excpFlag = 0;
//if (ret)
//printk("%s1: ret=%x\n", __func__, ret);
    return ret;
  }

  if (send_to_stack_buf_w_count != send_to_stack_buf_r_count)
  {
    ret |= (POLLIN | POLLRDNORM);
    Dect_excpFlag = 0;
  }

  if (receive_to_debug_info_buf_w_count != receive_to_debug_info_buf_r_count)
  {
    ret |= (POLLOUT | POLLRDNORM);
    Dect_excpFlag = 0;
  }
//if (ret)
//printk("%s2: ret=%x\n", __func__, ret);
  return ret;
}

/*******************************************************************************
Description:
   Handle IOCTL commands.
Arguments:
Note:
*******************************************************************************/
int dect_drv_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int res;
    unsigned short u16Len;
    unsigned char* pucBuf;

  //printk("%s: %x\n", __func__, cmd);
  switch(cmd)
  {
    case DECT_DRV_SHUTDOWN:
		{
			   dect_drv_release(inode,filp);
         break;  
		}
    case DECT_DRV_TRIGGER_COSIC_DRV:
    {
         unsigned char  data[ 2 ];

         copy_from_user ( data, (unsigned char *)arg, sizeof( data ) );
         if( (data[ 0 ] == 1) && (data[ 1 ] == 1) )
         {
            gp_onu_disable_external_interrupt( );
         }
         trigger_cosic_driver( );
         break;  
    }

    case DECT_DRV_TO_HMAC_MSG :
      {
        HMAC_QUEUES value;
        copy_from_user ((HMAC_QUEUES *)&value, (HMAC_QUEUES *)arg, sizeof (HMAC_QUEUES));
        if(vucDriverMode ==  DECT_DRV_SET_APP_MODE_CoC_RQ)
				{
					/*No need to respond with CoC dummy pkt as we have valid data and don't want to enter CoC. 
						So change state and no need to toggle CS as not yet entered CoC confirm state*/
        	vucDriverMode = DECT_DRV_SET_APP_MODE;
				}
        else if(vucDriverMode ==  DECT_DRV_SET_APP_MODE_CoC_CNF)
        {
        	/*Toggle CS to indicate MODEM to come out of CoC*/
        	ssc_dect_cs_toggle();
        	ssc_dect_cs_toggle();
          //printk("\nToggling CS to come out of CoC\n");
				}

       	DECODE_HMAC(&value);
      }
      break;

    case DECT_DRV_FROM_HMAC_MSG :
		res = 0;

		while(1)
		{
		  if (send_to_stack_buf_r_count != send_to_stack_buf_w_count)
		  {
			  copy_to_user ((void *)arg, (unsigned char*)&send_to_stack_buf[send_to_stack_buf_r_count], sizeof(HMAC_QUEUES));
			  memset((unsigned char*)&send_to_stack_buf[send_to_stack_buf_r_count], 0x00, sizeof(HMAC_QUEUES));
			  send_to_stack_buf_r_count++;
		  
			if (send_to_stack_buf_r_count == MAX_RECEIVED_BUFFER_COUNT)
			  send_to_stack_buf_r_count = 0;
				arg += sizeof(HMAC_QUEUES);
				res++;
		  }
		  else
		  {
			if(res == 0)
				return -1;
			else
				return res;
		  }
		}
      break;

    case DECT_DRV_GET_PMID :
      {
        unsigned char data[4];
        copy_from_user (data, (unsigned char *)arg, 4);

		Get_Used_Pmid(data[0], &data[1]);

        copy_to_user ((void *)arg, data, 4);
      }
      break;

    case DECT_DRV_GET_ENC_STATE :
      {
        unsigned char data[2];
        copy_from_user (data, (unsigned char *)arg, sizeof (data));
        data[1] = Get_Enc_State(data[0]);
        copy_to_user ((void *)arg, data, sizeof (data));        
      }
      break;
      
    case DECT_DRV_SET_KNL_STATE :
      {
        unsigned char data[2];
        copy_from_user (data, (unsigned char *)arg, sizeof (data));
        Set_KNL_State(data[0], data[1]);
      }
      break;





	case DECT_DRV_DEBUG_INFO:
		{
        DECT_MODULE_DEBUG_INFO value;
        copy_from_user ((DECT_MODULE_DEBUG_INFO *)&value, (DECT_MODULE_DEBUG_INFO *)arg, sizeof (DECT_MODULE_DEBUG_INFO));
		
		DECODE_DEBUG_TO_MODULE(&value);
		}
		break;


	case DECT_DRV_DEBUG_INFO_FROM_MODULE:
		res = 0;

		while(1)
		{
		  if (receive_to_debug_info_buf_r_count != receive_to_debug_info_buf_w_count)
		  {
			  copy_to_user ((void *)arg, (unsigned char*)&receive_to_debug_info_buf[receive_to_debug_info_buf_r_count], sizeof(DECT_MODULE_DEBUG_INFO));
			  memset((unsigned char*)&receive_to_debug_info_buf[receive_to_debug_info_buf_r_count], 0x00, sizeof(DECT_MODULE_DEBUG_INFO));
			  receive_to_debug_info_buf_r_count++;
		  
			if (receive_to_debug_info_buf_r_count == MAX_DEBUG_INFO_COUNT)
			  receive_to_debug_info_buf_r_count = 0;

			arg += sizeof(DECT_MODULE_DEBUG_INFO);
			res++;
		  }
		  else
		  {

			if(res == 0)
				return -1;
			else
				return res;
		  }
		}

		break;

    case DECT_DRV_SET_LOADER_MODE:
          vucDriverMode = DECT_DRV_SET_LOADER_MODE;
          /*printk("Moving to LOADER MODE\n");*/
          break;
    case DECT_DRV_SET_APP_MODE:
         vucDriverMode = DECT_DRV_SET_APP_MODE;
         /*printk("Moving to APP MODE\n");*/
         break;
   case DECT_DRV_FW_WRITE:
				{
					copy_from_user((void*)(&u16Len),(void*)arg,2); //read len
					//printk(": Write buffer len=%d\n",u16Len);
					if( u16Len < COSIC_LOADER_PACKET_SIZE ) {
						copy_from_user((void*)vucWriteBuffer[vu16WriteBufHead],
													(void*)arg, u16Len+2);

						++vu16WriteBufHead;
						if(vu16WriteBufHead == MAX_READ_BUFFFERS)
							vu16WriteBufHead=0;
						if( vu16WriteBufHead == vu16WriteBufTail )
						{
							printk(": Write buffer full\n");
							++vu16WriteBufTail;
							if(vu16WriteBufTail == MAX_READ_BUFFFERS)
								vu16WriteBufTail = 0;
						}
					}// packet size check
				}
        break;
   case DECT_DRV_FW_READ:
			{
				//If data available, copy to user buffer
				if( vu16ReadBufHead != vu16ReadBufTail )
				{
					pucBuf = vucReadBuffer[vu16ReadBufTail];
					u16Len = ((*pucBuf) << 8) + (*(pucBuf+1));
					/*DEBUG_MSG(COSIC_LOADER_NAME ": ReadBufer(Head=%d Tail=%d) Tail Buf Len=%d\n",
							vu16ReadBufHead,vu16ReadBufTail,u16Len);*/
					copy_to_user((void*)arg, pucBuf,u16Len+2); //+2 including len field
					++vu16ReadBufTail;
					if(vu16ReadBufTail == MAX_READ_BUFFFERS)
						vu16ReadBufTail = 0;
				}
				break;
			}
		case DECT_DRV_IRQ_CTRL:
		{
     unsigned char  data;
     copy_from_user ( &data, (unsigned char *)arg, 1);
#undef printk
     printk("Entering inside DECT_DRV_IRQ_CTRL with value %d\n",data);
		 if(data==1)//Enable IRQ
			{
				viDisableIrq = 0;
				gp_onu_enable_external_interrupt();	  
			}else{//Disable IRQ
					viDisableIrq=1;	
			}
			break;
		}
		case DECT_DRV_ENABLE:
		{
     unsigned char  data;
     copy_from_user ( &data, (unsigned char *)arg, 1);
     printk("Entering inside DECT_DRV_ENABLE with value %d\n",data);
		 	if(data==1){
				vu16ReadBufHead = vu16ReadBufTail = vu16WriteBufHead = vu16WriteBufTail = 0;
        memset(vucReadBuffer,0,sizeof(vucReadBuffer));
        memset(vucWriteBuffer,0,sizeof(vucWriteBuffer));
					//ssc_dect_haredware_reset(1);
  				ifx_sscCosicLock();
			}else{
					if(spi_dev_handler != NULL)
					{
    				if(viSPILocked)
    				{
      				ifx_sscAsyncUnLock(spi_dev_handler);
      				viSPILocked=0;
    				}
					}
					vucDriverMode=0;
					FirstLock=1;
					ssc_dect_haredware_reset(0);
			}

		}
		break;
	  
    default :
      break;
  }
  
  return 0;
}

/*function to toggle Chip Select PIN required to come out of CoC*/
u32 ssc_dect_cs_toggle()
{
  if(viCSStatus)
	{
#if defined(CONFIG_IFX_GW188)|| defined(CONFIG_AR10)
      ifx_gpio_output_clear(IFX_DECT_SPI_CS, dect_gpio_module_id);
#else
			ifx_ssc_cs_low(DECT_CS);
#endif
			viCSStatus=IFX_DECT_OFF;
	}
	else
	{
#if defined(CONFIG_IFX_GW188)|| defined(CONFIG_AR10)
      ifx_gpio_output_set(IFX_DECT_SPI_CS, dect_gpio_module_id);
#else
			ifx_ssc_cs_high(DECT_CS);
#endif
			viCSStatus=IFX_DECT_ON;
	}
	return 0;
}

u32 ssc_dect_cs(u32 on,u32 cs_data)
{
  //printk("\nCSCB\n");
  if(viSPILockedCB)
	{
		if (iCosicMode == 0) /* Tx */
		{
  		//printk("\nCS->low\n");
#ifdef ENHANCED_DELAY
			udelay(vuiDealy);		 /*Testing delay before toggle */
#endif
#if defined(CONFIG_IFX_GW188)|| defined(CONFIG_AR10)
  	//	printk("\n-Bala -CS->low CS number %d \n",IFX_DECT_SPI_CS);
      ifx_gpio_output_clear(IFX_DECT_SPI_CS, dect_gpio_module_id);
#else
			ifx_ssc_cs_low(DECT_CS);
#endif
			viCSStatus=IFX_DECT_OFF;/*store the CS state; required for ssc_dect_cs_toggle*/
			udelay(20);		 /*delay change 8 -> 10 */
		}
		else /* Rx */
		{
  		//printk("\nCS->High\n");
#ifdef ENHANCED_DELAY
			udelay(vuiDealy);		 /*Testing delay before toggle */
#endif
#if defined(CONFIG_IFX_GW188)|| defined(CONFIG_AR10)
  		//printk("\n-Bala -CS->High CS number %d \n",IFX_DECT_SPI_CS);
      ifx_gpio_output_set(IFX_DECT_SPI_CS, dect_gpio_module_id);
#else
			ifx_ssc_cs_high(DECT_CS);
#endif
			viCSStatus=IFX_DECT_ON;/*store the CS state; required for ssc_dect_cs_toggle*/
			udelay(20);		 /*delay change 8 -> 10 */
		}
    viSPILockedCB=0;
	}
   return 0;
}



/* dect hard ware reset pin control function */
int ssc_dect_haredware_reset(int on)
{
   if(on)
   {
#ifdef CONFIG_AMAZON_S
#ifdef UTA
	  *(AMAZON_S_GPIO_P3_OUT) = (*AMAZON_S_GPIO_P3_OUT) | (1<<5);
#else
	  *(AMAZON_S_GPIO_P1_OUT) = (*AMAZON_S_GPIO_P1_OUT) | (1<<6);
#endif	  
#elif defined (CONFIG_AR9) || defined (CONFIG_VR9) || defined (CONFIG_AR10)
    ifx_gpio_output_set(IFX_DECT_RST, dect_gpio_module_id);
#endif   
   }
   else
   {
#ifdef CONFIG_AMAZON_S
#ifdef UTA
	  *(AMAZON_S_GPIO_P3_OUT) = (*AMAZON_S_GPIO_P3_OUT) & (~(1<<5));
#else
	  *(AMAZON_S_GPIO_P1_OUT) = (*AMAZON_S_GPIO_P1_OUT) & (~(1<<6));
#endif	  
#elif defined (CONFIG_AR9) || defined (CONFIG_VR9) || defined (CONFIG_AR10)
    ifx_gpio_output_clear(IFX_DECT_RST, dect_gpio_module_id);
#endif
   }
   return 0;
}
EXPORT_SYMBOL(ssc_dect_haredware_reset);


/*******************************************************************************
Description:
   Open the Dect driver.
Arguments:
Note:
*******************************************************************************/
int dect_drv_open(struct inode *inode, struct file *filp)
{

  unsigned int num;
  if (open_dect_count)
      return -1;
  dect_gpio_intnum = DECT_IRQ_NUM;
  num = MINOR(inode->i_rdev);
  if (num > DECT_DRV_MINOR_1)
  {
    printk("DECT Driver: Minor number error!\n");
    return -1;
  }
	FirstLock =1;   
  //printk(DECT_DEVICE_NAME " : Device open (%d, %d)\n\n", MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
  
  open_dect_count++;


/* Use filp->private_data to point to the device data. */
  filp->private_data = (void *)num;
  
  /* Initialize a wait_queue list for the system poll function. */ 
  init_waitqueue_head(&Dect_WakeupList);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
  /* Increment module use counter */
  MOD_INC_USE_COUNT;
#endif
  HMAC_INIT();
	vucDriverMode=0;

  Reset_Hmac_Debug_buffer();

	atomic_set(&vuiGwDbgFlag,1);
  Dect_int_GPIO_init();

  {
    IFX_SSC_CONFIGURE_t ssc_cfg = {0};
	
    ssc_cfg.baudrate     = 4000000;			// TODO: baudrate is int valiable????
    ssc_cfg.csset_cb     = (int (*)(u32,IFX_CS_DATA))ssc_dect_cs;
    ssc_cfg.cs_data      = DECT_CS;
		ssc_cfg.maxFIFOSize  = 512;
		ssc_cfg.fragSize	 = 512;
    ssc_cfg.ssc_mode     = IFX_SSC_MODE_3;
    ssc_cfg.ssc_prio = IFX_SSC_PRIO_ASYNC;
#if defined(CONFIG_AR10) && defined(DECT_USE_USIF)
		ssc_cfg.maxFIFOSize  = 512;
    ssc_cfg.duplex_mode  = IFX_USIF_SPI_HALF_DUPLEX;
#endif
    spi_dev_handler = ifx_sscAllocConnection(DECT_DEVICE_NAME, &ssc_cfg);

    if (spi_dev_handler == NULL)
    {
       printk("failed to register spi device \n");
       return -1;
    }
  }
  ifx_sscCosicLock();
	/* Initialize the buffer pools */
#ifndef USE_VOICE_BUFFERPOOL
	pTapiWriteBufferPool = bufferPoolInit(sizeof(TAPI_VOICE_WRITE_BUFFER),
									MAX_TAPI_BUFFER_CNT,5);

	if(pTapiWriteBufferPool == NULL)
	{
		printk("Error initilizing the write buffer pool\n");
	}
#endif
	if(!create_dir){

#if CONFIG_PROC_FS

	dect_proc_dir = proc_mkdir("driver/dect", NULL);
  create_dir++;

	if (dect_proc_dir != NULL)
	{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
		dect_proc_dir->owner = THIS_MODULE;
#endif
		dect_version_file = create_proc_read_entry("version", S_IFREG|S_IRUGO, dect_proc_dir,
										dect_drv_read_proc, dect_drv_read_version_proc);
		if(dect_version_file != NULL)
    {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
       dect_version_file->owner = THIS_MODULE;
#endif
    }

    dect_gw_stats_file = create_proc_entry("gw-stats", 0644, dect_proc_dir);
		if(dect_gw_stats_file != NULL)
    {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
       dect_gw_stats_file->owner = THIS_MODULE;
#endif
       dect_gw_stats_file->data = dect_drv_read_gw_stats_proc;
       dect_gw_stats_file->read_proc = dect_drv_read_proc;
       dect_gw_stats_file->write_proc = dect_drv_write_proc;
    }

    dect_modem_stats_file = create_proc_entry("modem-stats", 0644, dect_proc_dir);
		if(dect_modem_stats_file != NULL)
    {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
       dect_modem_stats_file->owner = THIS_MODULE;
#endif
       dect_modem_stats_file->data = dect_drv_read_modem_stats_proc;
       dect_modem_stats_file->read_proc = dect_drv_read_proc;
       dect_modem_stats_file->write_proc = dect_drv_write_proc;
    }

    dect_channel_stats_file = create_proc_entry("channel-stats", 0644, dect_proc_dir);
		if(dect_channel_stats_file != NULL)
    {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
       dect_channel_stats_file->owner = THIS_MODULE;
#endif
       dect_channel_stats_file->data = dect_drv_read_channel_stats_proc;
       dect_channel_stats_file->read_proc = dect_drv_read_proc;
       dect_channel_stats_file->write_proc = dect_drv_write_proc;
    }
	}

#endif /* CONFIG_PROC_FS */
	}

 vu16ReadBufHead = vu16ReadBufTail = vu16WriteBufHead = vu16WriteBufTail = 0;
 memset(vucReadBuffer,0,sizeof(vucReadBuffer));
 memset(vucWriteBuffer,0,sizeof(vucWriteBuffer));

  /* COSIC TAPI read THREAD */
  start_tapiThread();

  /* COSIC DRIVER THREAD */
	start_cosicThread();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
  if( !request_irq( dect_gpio_intnum , Dect_GPIO_interrupt, IRQF_DISABLED, "dect", NULL))
#else
  if( !request_irq( dect_gpio_intnum , Dect_GPIO_interrupt, SA_INTERRUPT, "dect", NULL))
#endif 
  { 
		gp_onu_enable_external_interrupt();	  
  } 
  else 
  { 
	  printk("can't get assigned irq %d\n",dect_gpio_intnum);
	  printk(" prnioport can't get assigned irq n");
	  dect_gpio_intnum = -1;
	  
  }
  return 0;
}


/*******************************************************************************
Description:
   Close the Dect driver.
Arguments:
Note:
*******************************************************************************/
int dect_drv_release(struct inode *inode, struct file *filp)
{
#if 0
	/*printk("\n"DECT_DEVICE_NAME " : Device release (%d, %d)\n\n", MAJOR(inode->i_rdev), MINOR(inode->i_rdev));*/
	if (--open_dect_count == 0)
	{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    MOD_DEC_USE_COUNT;
#endif
	}
#endif
	gp_onu_disable_external_interrupt();

	if(open_dect_count){
  FirstLock =1;
	ssc_dect_haredware_reset(0);
	if(dect_gpio_intnum != -1 ) 
	{
			disable_irq(dect_gpio_intnum);
			free_irq(dect_gpio_intnum , NULL); 
			dect_gpio_intnum = -1;
		#ifdef CONFIG_DANUBE
			ifx_gptu_timer_free(TIMER2A);
		#endif
	}
	if(spi_dev_handler != NULL)
	{
    if(viSPILocked)
    {
      ifx_sscAsyncUnLock(spi_dev_handler);
      viSPILocked=0;
    }
		ifx_sscFreeConnection(spi_dev_handler);
		spi_dev_handler = NULL;
	}
  ifx_gpio_deregister(IFX_GPIO_MODULE_DECT);

#if CONFIG_PROC_FS
	if (create_dir)
	{
		remove_proc_entry("version",dect_proc_dir);
		remove_proc_entry("gw-stats",dect_proc_dir);
		remove_proc_entry("modem-stats",dect_proc_dir);
		remove_proc_entry("channel-stats",dect_proc_dir);
		remove_proc_entry("driver/dect",NULL);
		create_dir=0;
	}
#endif /*CONFIG_PROC_FS*/
	vucDriverMode=0;
	stop_cosicThread();
	stop_tapiThread();
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    MOD_DEC_USE_COUNT;
#endif
	open_dect_count = 0;
	}/*if (open_dect_count)*/
  return 0;
}


/*******************************************************************************
Description:
   register Dect driver to kernel
Arguments:
Note:
*******************************************************************************/
int dect_drv_init(void)
{
  int result;

  result = register_chrdev(DECT_DRV_MAJOR, DECT_DEVICE_NAME, &dect_drv_fops);
  
  if (result < 0)
  {
    printk(KERN_INFO "DECT Driver: Unable to get major %d\n", DECT_DRV_MAJOR);
    return result;
  }
#ifdef CONFIG_LTT
  result = marker_set_probe("cosic_event",
                            "long %lu string %s",
                            probe_cosic_event);
  if (!result){
    printk("Unable to set cosic marker\n");
    goto cleanup;
  }

  result = marker_set_probe("tapi_event",
                            "long %lu string %s",
                            probe_tapi_event);
  if (!result){
    printk("Unable to set tapi marker\n");
    goto cleanup;
  }

  return 0;
cleanup:
  marker_remove_probe(probe_cosic_event);
  marker_remove_probe(probe_tapi_event);
#endif
  return 0;
}


/*******************************************************************************
Description:
   clean up the DECT driver.
Arguments:
Note:
*******************************************************************************/
void dect_drv_cleanup(void)
{

#ifdef CONFIG_LTT
  marker_remove_probe(probe_cosic_event);
  marker_remove_probe(probe_tapi_event);
#endif
	unregister_chrdev(DECT_DRV_MAJOR, DECT_DEVICE_NAME);

#if 0 /* Commented for having sync with open */
  stop_cosicThread();
	stop_tapiThread();
	if(dect_gpio_intnum != -1 ) {
	free_irq(dect_gpio_intnum , NULL);
	dect_gpio_intnum = -1;
#ifdef CONFIG_DANUBE
	ifx_gptu_timer_free(TIMER2A);
#endif
	} 
#endif
#if 0 /*CONFIG_PROC_FS*/
	remove_proc_entry("version",dect_proc_dir);
	remove_proc_entry("gw-stats",dect_proc_dir);
	remove_proc_entry("modem-stats",dect_proc_dir);
	remove_proc_entry("channel-stats",dect_proc_dir);

	remove_proc_entry("driver/dect",NULL);
	create_dir--;
#endif /*CONFIG_PROC_FS*/

}


module_init(dect_drv_init);
module_exit(dect_drv_cleanup);

#if CONFIG_PROC_FS
			
int dect_drv_read_proc(char *page, char **start, off_t off,
								int count, int *eof, void *data)
{
	int len;
	int (*fn)(char *buf);
	if (data != NULL)
	{
		fn = data;
		len = fn(page);
	}
	else
		return 0;

	if (len <= off+count)
		*eof = 1;

	*start = page + off;

	len -= off;

	if (len > count)
		len = count;

	if (len < 0)
		len = 0;

	return len;

}
					
int dect_drv_read_version_proc(char *buf)
{
	int len=0,len1=0,len2=0;
	char buf1[20],buf2[20];
	strcpy(version.ucDrvVersion,DECT_VERSION);
	len1=sprintf(buf1,"%x.%x.%x",(version.ucCosicSwVersion[0] >> 4),\
							(version.ucCosicSwVersion[0] & 0x0f),version.ucCosicSwVersion[1]);
	len2=sprintf(buf2,"%x.%x.%x",(version.ucBmcFwVersion[0] >> 4),\
							(version.ucBmcFwVersion[0] & 0x0f),version.ucBmcFwVersion[1]);
	len = sprintf(buf, "\nModules          Version \
											\nCOSIC s/w         %s \
											\nBMC f/w           %s \
											\nGW COSIC driver   %s \n", \
											buf1,buf2,version.ucDrvVersion);
	gw_stats.uiLastOprn = DBG_READ_GW_VERSION;
	gw_stats.uiLastOprnStatus = DBG_SUCCESS;
	return len;
}

int dect_drv_read_gw_stats_proc(char *buf)
{
	int len=0,len1=0,len2=0,i=0,iNumCalls=0;
	char oprn[20],mode[15],status[10],buf1[200],buf2[200],gw_dbg_status[10];
	char *b1=buf1,*b2=buf2;
	if(atomic_read(&vuiGwDbgFlag)==1)
	{
		strcpy(gw_dbg_status,"Enabled");
	}
	else
	{
		strcpy(gw_dbg_status,"Disabled");
	}
  switch(gw_stats.uiLastOprn)
	{
		case DBG_READ_GW_STATS :
			strcpy(oprn,"READ gw-stats");
    break;
  	case DBG_READ_MODEM_STATS :
			strcpy(oprn,"READ modem-stats");
    break;
  	case DBG_READ_GW_VERSION :
			strcpy(oprn,"READ version");
    break;
  	case DBG_RESET_GW_STATS :
			strcpy(oprn,"RESET gw-stats");
    break;
  	case DBG_RESET_MODEM_STATS :
			strcpy(oprn,"RESET modem-stats");
    break;
  	case DBG_REQ_MODEM_STATS:
			strcpy(oprn,"GET modem-stats");
    break;
  	case DBG_GW_STATS_EN:
			strcpy(oprn,"ENABLE gw-stats");
    break;
  	case DBG_GW_STATS_DIS:
			strcpy(oprn,"DISABLE gw-stats");
    break;
  	default:
			strcpy(oprn,"NONE");
	}
  switch(gw_stats.uiLastOprnStatus)
	{
		case DBG_SUCCESS :
			strcpy(status,"SUCCESS");
    break;
		case DBG_FAIL :
			strcpy(status,"FAIL");
    break;
		case DBG_PENDING :
			strcpy(status,"PENDING");
    break;
	}
	switch(vucDriverMode)
	{
		case DECT_DRV_SET_APP_MODE :
			strcpy(mode,"App");
    break;
		case DECT_DRV_SET_LOADER_MODE :
			strcpy(mode,"Boot");
    break;
		case DECT_DRV_SET_APP_MODE_CoC_RQ :
			strcpy(mode,"App-CoC-RQ");
    break;
		case DECT_DRV_SET_APP_MODE_CoC_CNF :
			strcpy(mode,"App-CoC");
    break;
	}
	for(i=0;i<10;i++)
	{
		b1+=len1;
		if(i == 9)
			len1=sprintf(b1,"\t%d+",i+1);
		else
			len1=sprintf(b1,"\t%d",i+1);
	}
  b1[len1]='\0';
	for(i=0;i<10;i++)
	{
		b2+=len2;
		len2=sprintf(b2,"\t%d",gw_stats.auiIntLossSeq[i]);
	}
  b2[len2]='\0';
  for(i=0;i<MAX_MCEI;i++)
  {
    if(xMceiBuffer[i].iKpiChan != 0xFF){
      iNumCalls++;
    }
  }

	len = sprintf(buf, "\nGW debug : %s \
								\nLast Debug operation : %s status : %s \
								\nTime stamp of this stats : %lu \
								\nTotal interrupts received by gw : %u \
								\nRising Interrupts count before RxCB : %u \
								\nRising Interrupts count before TxCB : %u \
								\nUnlock at falling edge count : %u \
								\nKPI write fail count : %u \
								\nCorrupted SPI packet receive count : %u \
								\nGW COSIC Driver Mode : %s \
								\nNum Active Calls : %u  \
								\nInterrupt loss sequence stats: \
								\n%s\n%s\n", \
								gw_dbg_status,oprn,status,jiffies,gw_stats.uiInt,gw_stats.uiLossRx, \
								gw_stats.uiLossTx,gw_stats.uiUnLockFallEdge, \
							  gw_stats.uiKpi,gw_stats.uiInvSpi,mode,iNumCalls,buf1,buf2);
	gw_stats.uiLastOprn = DBG_READ_GW_STATS;
	gw_stats.uiLastOprnStatus = DBG_SUCCESS;
	return len;
}

int dect_drv_read_channel_stats_proc(char *buf)
{
  int len =0;
  IFX_TAPI_KIO_HANDLE kHandle1, kHandle2, kHandle3, kHandle4, kHandle5, kHandle6;
  IFX_TAPI_DECT_STATISTICS_t xStat1, xStat2, xStat3, xStat4, xStat5, xStat6;
	kHandle1 = ifx_tapi_kopen("/dev/vmmc11");
	kHandle2 = ifx_tapi_kopen("/dev/vmmc12");
	kHandle3 = ifx_tapi_kopen("/dev/vmmc13");
	kHandle4 = ifx_tapi_kopen("/dev/vmmc14");
	kHandle5 = ifx_tapi_kopen("/dev/vmmc15");
	kHandle6 = ifx_tapi_kopen("/dev/vmmc16");
  memset(&xStat1, 0, sizeof(IFX_TAPI_DECT_STATISTICS_t));
  memset(&xStat2, 0, sizeof(IFX_TAPI_DECT_STATISTICS_t));
  memset(&xStat3, 0, sizeof(IFX_TAPI_DECT_STATISTICS_t));
  memset(&xStat4, 0, sizeof(IFX_TAPI_DECT_STATISTICS_t));
  memset(&xStat5, 0, sizeof(IFX_TAPI_DECT_STATISTICS_t));
  memset(&xStat6, 0, sizeof(IFX_TAPI_DECT_STATISTICS_t));
  ifx_tapi_kioctl(kHandle1, IFX_TAPI_DECT_STATISTICS_GET, (int)&xStat1);
  ifx_tapi_kclose(kHandle1);
  ifx_tapi_kioctl(kHandle2, IFX_TAPI_DECT_STATISTICS_GET, (int)&xStat2);
  ifx_tapi_kclose(kHandle2);
  ifx_tapi_kioctl(kHandle3, IFX_TAPI_DECT_STATISTICS_GET, (int)&xStat3);
  ifx_tapi_kclose(kHandle3);
  ifx_tapi_kioctl(kHandle4, IFX_TAPI_DECT_STATISTICS_GET, (int)&xStat4);
  ifx_tapi_kclose(kHandle4);
  ifx_tapi_kioctl(kHandle5, IFX_TAPI_DECT_STATISTICS_GET, (int)&xStat5);
  ifx_tapi_kclose(kHandle5);
  ifx_tapi_kioctl(kHandle6, IFX_TAPI_DECT_STATISTICS_GET, (int)&xStat6);
  ifx_tapi_kclose(kHandle6);
	len = sprintf(buf, "\nChannel               : \t1\t2\t3\t4\t5\t6 \
								\nNumber Of Upstream Packet   : \t%d\t%d\t%d\t%d\t%d\t%d  \
								\nNumber Of Downstream Packet : \t%d\t%d\t%d\t%d\t%d\t%d  \
								\nNumber Of SID packets : \t%d\t%d\t%d\t%d\t%d\t%d  \
								\nNumber Of PLC packets : \t%d\t%d\t%d\t%d\t%d\t%d  \
								\nOverRun               : \t%d\t%d\t%d\t%d\t%d\t%d  \
								\nUnderRun              : \t%d\t%d\t%d\t%d\t%d\t%d  \
								\nNumber Of Invalid packets : \t%d\t%d\t%d\t%d\t%d\t%d  \
								\n", \
							  xStat1.nPktUp,xStat2.nPktUp,xStat3.nPktUp,xStat4.nPktUp,xStat5.nPktUp,xStat6.nPktUp, \
							  xStat1.nPktDown,xStat2.nPktDown,xStat3.nPktDown,xStat4.nPktDown,xStat5.nPktDown,xStat6.nPktDown, \
							  xStat1.nSid,xStat2.nSid,xStat3.nSid,xStat4.nSid,xStat5.nSid,xStat6.nSid, \
							  xStat1.nPlc,xStat2.nPlc,xStat3.nPlc,xStat4.nPlc,xStat5.nPlc,xStat6.nPlc, \
							  xStat1.nOverflows,xStat2.nOverflows,xStat3.nOverflows,xStat4.nOverflows,xStat5.nOverflows,xStat6.nOverflows, \
                xStat1.nUnderflows, xStat2.nUnderflows, xStat3.nUnderflows,xStat4.nUnderflows,xStat5.nUnderflows,xStat6.nUnderflows, \
                xStat1.nInvalid, xStat2.nInvalid, xStat3.nInvalid,xStat4.nInvalid,xStat5.nInvalid,xStat6.nInvalid);
	return len;
}


int dect_drv_read_modem_stats_proc(char *buf)
{
	int len=0,len1=0,len2=0,len3=0,len4=0,len5=0,/*len6=0,*/len7=0,i=0,tmp[MAX_MCEI]={0};
	char buf1[200],status[20],BadAX[100],Q1Q2BadA[100],BadZ[100],FHO[100],SHO[100],/*RSSI[100],*/SyncErr[100];
	char *b1=buf1,*b2=status,*b3=BadAX,*b4=Q1Q2BadA,*b5=BadZ,*b7=SHO,*b6=FHO,/**b8=RSSI,*/*b9=SyncErr;
#if 0
	for(i=0;i<MAX_MCEI;i++)
	{
		printk("\nLBN%d = %d   %d\n",i,Mcei_Table[i].lbn_1,Mcei_Table[i].lbn_2);
	}
#endif
	for(i=0;i<MAX_MCEI;i++)
	{
		b1+=len1;
		len1=sprintf(b1,"\t%d",i);
	}
  b1[len1]='\0';
	len1=0;
	/*Check for MCEI active or not & store the status in tmp */
	for(i=0;i<MAX_MCEI;i++)
	{
		if (Mcei_Table[i].lbn_1 != NO_LBN)
		{
			tmp[Mcei_Table[i].lbn_1]=1;
		}	
		if (Mcei_Table[i].lbn_2 != NO_LBN)
		{
			tmp[Mcei_Table[i].lbn_2]=1;
		}	
	}
	for(i=0;i<MAX_MCEI;i++)
	{
		b2+=len1;
		/*Check for MCEI active or not*/
		if(tmp[i])
		{
			len1=sprintf(b2,"\t%s","A");
		}
		else
		{	
			len1=sprintf(b2,"\t%s","NA");
		}
	}
  b2[len1]='\0';
	len1=0;
	for(i=0;i<6;i++)
	{
		b3+=len1;
		len1=sprintf(b3,"\t%d",lbn_stats[i].uiBadAXCRC);
		b4+=len2;
		len2=sprintf(b4,"\t%d",lbn_stats[i].uiQ1Q2BadACRC);
		b5+=len3;
		len3=sprintf(b5,"\t%d",lbn_stats[i].uiBadZCRC);
		b6+=len4;
		len4=sprintf(b6,"\t%d",lbn_stats[i].uiFailedHO);
		b7+=len5;
		len5=sprintf(b7,"\t%d",lbn_stats[i].uiSuccessHO);
		/*
		b8+=len6;
		len6=sprintf(b8,"\t%d",lbn_stats[i].uiRSSI);
		*/
		b9+=len7;
		len7=sprintf(b9,"\t%d",lbn_stats[i].uiSyncErr);
	}
  b3[len1]='\0';
  b4[len2]='\0';
  b5[len3]='\0';
  b6[len4]='\0';
  b7[len5]='\0';
  //b8[len6]='\0';
  b9[len7]='\0';
	len = sprintf(buf, "\nSPI Timeout Counter for Tx                        : %u/%u  \
											\nSPI Timeout Counter for Rx                        : %u/%u  \
											\nStack Queue Overflow Counter                      : %u/%u  \
											\nNot Serviced SPI Interrupt Counter                : %u/%u  \
											\nDummy Bearer Change Counter                       : %u/%u	 \
											\n\nLBN stats   :        \
											\n\n\t\t\t%s \
											\n status                       %s \
											\n Bad A field CRC counter      %s \
											\n Q2=0 counter                 %s \
											\n Bad Z field CRC counter      %s \
											\n Failed Handovers             %s \
											\n Successful handovers         %s \
											\n Number of sync errors        %s \n ",    																						 \
											modem_stats.uiSpiTimeOutTxCC,modem_stats.uiSpiTimeOutTxTC,   \
											modem_stats.uiSpiTimeOutRxCC,modem_stats.uiSpiTimeOutRxTC,   \
											modem_stats.uiStackOverFlowCC,modem_stats.uiStackOverFlowTC, \
											modem_stats.uiNonSrvcSpiIntCC,modem_stats.uiNonSrvcSpiIntTC, \
											modem_stats.uiDummyBearerChangeCC,modem_stats.uiDummyBearerChangeTC, \
											buf1,status,BadAX,Q1Q2BadA,BadZ,FHO,SHO,/*RSSI,*/SyncErr);
	gw_stats.uiLastOprn = DBG_READ_MODEM_STATS;
	gw_stats.uiLastOprnStatus = DBG_SUCCESS;
	return len;
}

int dect_drv_write_proc(struct file *file,const char *buffer,unsigned long count,void *data)
{
#ifdef ENHANCED_DELAY
  int len=count;
#else
  int len=1;
#endif
  char kdata[10]= {0};
  if(copy_from_user(&kdata, buffer, len)) 
  {
    printk("\ncopy from user failed in proc system\n");
    return -EFAULT;
  }
	switch(kdata[0] - '0')
  {
    case 0:
    {
      if(data == dect_drv_read_gw_stats_proc)
      {
				DisableGwDebugStats();
      }
      break;
    }
    case 1:
    {
      if(data == dect_drv_read_gw_stats_proc)
      {
				EnableGwDebugStats();
      }
      else if(data == dect_drv_read_modem_stats_proc)
      {
				GetModemDebugStats();
				/*Since info id 6,7,8 debug pkts are sent by above fn*/
				vuiDbgSendFlags |= PROC_MODEM_DBG6_SEND;
				vuiDbgSendFlags |= PROC_MODEM_DBG7_SEND;
				vuiDbgSendFlags |= PROC_MODEM_DBG8_SEND;
      }
      break;
    }
    case 2:
    {
      if(data == dect_drv_read_gw_stats_proc)
      {
				ResetGwDebugStats();
      }
      else if(data == dect_drv_read_modem_stats_proc)
      {
				ResetModemDebugStats();
				/*Since info id 6debug pkt is sent by above fn*/
				vuiDbgSendFlags |= PROC_MODEM_DBG6_SEND;
      }
      break;
    }
    default:
    {
#ifdef ENHANCED_DELAY
       if(len < 4){
         printk("\nWRONG input from proc file system = %d\n",kdata);
       }
       else{
         vuiDealy = (kdata[1]-'0')*100+(kdata[2]-'0')*10+(kdata[3]-'0');
         printk("Dealy is %d\n", vuiDealy);
       }
#else
       printk("\nWRONG input from proc file system = %d\n",kdata);
#endif
    }
  }
  return count;
}

#endif /* CONFIG_PROC_FS */

#endif /* DRV_DECT_C */

