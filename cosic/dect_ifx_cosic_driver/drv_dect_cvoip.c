/******************************************************************************
                              Copyright (c) 2011
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information , see the file 'LICENSE' in the root folder of
  this software module.
	Author: Chintan Parekh (LQIN CPE SDC SWVD)

******************************************************************************/
#ifndef COSIC_DRV_C
#define COSIC_DRV_C

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


#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#include <linux/wait.h>
#include <linux/signal.h>
#include <linux/smp_lock.h> 

#include <asm/irq.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <asm/byteorder.h>
#include <asm/system.h>

#include "cosic_drv.h"

#ifdef CONFIG_AR9
	#include <asm-mips/ifx/ar9/ar9.h>
	#include <asm-mips/ifx/ar9/irq.h>
	#include <asm-mips/ifx/ifx_gpio.h>
	#include <asm/ifx/ifx_ssc.h>
	const int dect_gpio_module_id = IFX_GPIO_MODULE_DECT;
#endif

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

#ifdef  CONFIG_AR9
//connect JP73 2 and 3  GPIO39 Using P2.7---> EXIN 3--> IM1_IRL0
	#define DECT_IRQ_NUM    INT_NUM_IM1_IRL0
	//#define CVoIP_CS IFX_SSC_WHBGPOSTAT_OUT2_POS  //FIXME CS pin, now configure as SPI_CS3
	//#define CVoIP_CS 7  //P0.7
#endif 

#define COSIC_DRV_MAJOR             213
#define COISC_DRV_MINOR_0           0
#define COSIC_DRV_MINOR_1           1
#define COSIC_DEVICE_NAME "dect_drv"
#define DECT_VERSION "3.1.0.0"

#ifdef UNITTEST
char vcCorruption;
char vcMode;
char vcRxData;
#define CONFIG_PROC_FS_1
extern void tasklet_FallingEdge(unsigned long iDummy);
extern void tasklet_RisingEdge(unsigned long iDummy);
#endif
#ifdef CONFIG_PROC_FS_1
static void drv_dect_procfs_init(void);
static struct proc_dir_entry *dect_proc_dir;
static struct proc_dir_entry *dect_stub_file;
static int create_dir = 0;
#endif

MODULE_AUTHOR("LANTIQ");
MODULE_DESCRIPTION("Coisc Driver");
MODULE_LICENSE("GPL");

/******** SPI ***********/

#ifdef DRV_STATS
unsigned int RxBuffFull;
unsigned int RxBuffEmpty;
unsigned int RxRollOver;

unsigned int TxBuffFull;
unsigned int TxBuffEmpty;
unsigned int TxRollOver;

unsigned int CmdSent;
unsigned int CmdRcvd;


unsigned int HFXSent;
unsigned int HFXRcvd;
unsigned int FDX;

#endif

/*Global data*/
e_CosicDrv_Mode eDriverMode;
x_CosicDrv_Transaction vxTransaction; 
wait_queue_head_t Dect_WakeupList;

x_CosicDrv_Buffer xRxBuffer[MAX_READ_BUFFERS];
unsigned int RxOverflowFlag;
unsigned short RxBufHead;
unsigned short RxBufTail;

x_CosicDrv_Buffer xTxBuffer[MAX_WRITE_BUFFERS];
unsigned int TxOverflowFlag;
unsigned short TxBufHead;
unsigned short TxBufTail;

int viSPILocked;
int iCosicDrv_Opencount = 0;
int Dect_excpFlag = 0;
int	dect_gpio_intnum = -1;				/* interrupt number */
unsigned long iDummyDevId;

#ifdef CVOIP_ONBOARD
int ssc_dect_hardware_reset(int on);
#endif
volatile unsigned int IntEdgFlg;


extern int dect_gpio_intnum;
/*Global data ends*/


u32 ssc_dect_cs(u32 on,u32 cs_data);
u32 ssc_dect_cs_toggle(void);
IFX_SSC_HANDLE *spi_dev_handler=NULL;
extern  int ifx_ssc_cs_low(u32 pin);
extern  int ifx_ssc_cs_high(u32 pin);

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
#endif

/*******************************************************************************
Description: Reset all global variables, structures and buffers.
Arguments:
Note:
*******************************************************************************/
void CosicDrv_ResetGlobal(void)
{
  eDriverMode = COSIC_DRV_MODE_IDLE;
  viSPILocked = 0;
  MAKE_TRANS_IDLE;

  TxBufTail = TxBufHead = TxOverflowFlag = 0;
  RxBufTail = RxBufHead = RxOverflowFlag = 0;
  memset(xTxBuffer,0,sizeof(xTxBuffer));
  memset(xRxBuffer,0,sizeof(xRxBuffer));
}
/*******************************************************************************
Description: A callback registered with SPI driver. It is inovked whent SPI bus
						 is locked/unlocked successfully.
						 
						 Chip select (CS) signal to CVoIP should be toggled.

Modifies:    viSPILocked Variable.All other function should only access this variable.
						 Change in CS - Indicates Beginning / End of Transaction.
Arguments:
Note:
*******************************************************************************/
u32 CosicDrv_SPICallback(u32 on,u32 cs_data)
{
  static int i;
  
	DBGN("CosicDrv_SPICallback Entry\n");
  if(i==0){
    DBGN("<CosicDrv_SPICallback> Ignore first CB/setting out to high\n");
    ifx_gpio_output_set(GPIO_CS, dect_gpio_module_id);
    i=1;
    return 0;
  }
	if(0 == viSPILocked) /*Chip select is High*/
	{
      DBGN("CosicDrv_SPICallback, Pulling CS to Low\n");
      ifx_gpio_output_clear(GPIO_CS, dect_gpio_module_id);
			//ifx_ssc_cs_low(GPIO_CS);
			udelay(300000);
			viSPILocked = 1 ;
			MAKE_TRANS_IDLE; //At begnning of transaction;
	}
	else /*Chip select is Low*/
	{
      DBGN("CosicDrv_SPICallback, Pulling CS to High\n");
      ifx_gpio_output_set(GPIO_CS, dect_gpio_module_id);
			//ifx_ssc_cs_high(GPIO_CS);
			viSPILocked = 0;
	}
  DBGN("CosicDrv_SPICallback Exit\n");
	return 0;
}
#ifdef CVOIP_ONBOARD
/* dect hard ware reset pin control function */
int ssc_dect_hardware_reset(int on)
{
   if(on)
   {
    ifx_gpio_output_set(GPIO_RESET, dect_gpio_module_id);
   }
   else
   {
    ifx_gpio_output_clear(GPIO_RESET, dect_gpio_module_id);
   }
   return 0;
}

EXPORT_SYMBOL(ssc_dect_hardware_reset);
#endif



int cosic_drv_open   (struct inode *inode, struct file *filp);
int cosic_drv_release(struct inode *inode, struct file *filp);
int cosic_drv_ioctl  (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
int cosic_drv_poll   (struct file *file, poll_table *wait);
int cosic_drv_write  (struct file *file_p, char *buf, size_t count, loff_t *ppos);
int cosic_drv_read   (struct file *file_p, char *buf, size_t count, loff_t *ppos);

/* 
 * The operations for device node handled.
 */
struct file_operations cosic_drv_fops =
{
    owner:
        THIS_MODULE,
    read:
        cosic_drv_read,
    write:
        (void*)cosic_drv_write,
    poll: 
        (void*)cosic_drv_poll,
    ioctl:
        cosic_drv_ioctl,
    open:
        cosic_drv_open,
    release:
        cosic_drv_release
};

/*******************************************************************************
Description: read system call mapped from Application
						 1. See If there is any data to read
						 2. Lock the read index
						 3. copy data to user data structure
						 4. If no data is read then put the appplication to sleep ?
						 5. Head is updated from Application - read system call.	
						 6. Tail is upadted when data is recevied from CVoIP
Arguments:
Note:
*******************************************************************************/
int cosic_drv_read(struct file *file_p, char *buf, size_t count, loff_t *ppos)
{
	DBGN("cosic_drv_read Entry\n");
  DBGH("Overflow %d RxBufHead %d, RxBufTail %d\n",RxOverflowFlag,RxBufHead,RxBufTail);
	if(IsRxBufEmpty){
#ifdef DRV_STATS
			RxBuffEmpty++;
#endif 		
		DBGE("RxBuff is Empty\n");
		return 0;
	}
  DBGH("Str: [%s] Len: [%d]\n",xRxBuffer[RxBufHead].acCommand,xRxBuffer[RxBufHead].len);
	if( copy_to_user(buf, xRxBuffer[RxBufHead].acCommand, 
									xRxBuffer[RxBufHead].len)) {
		return -EFAULT;
	}
  count = xRxBuffer[RxBufHead].len;

	/*Reset Buf cotents*/
	memset(&xRxBuffer[RxBufHead],0,sizeof(x_CosicDrv_Buffer));
	INC_RXBUF_HEAD;

	buf[count-1] = '\0';
	DBGN("cosic_drv_read Exit\n");
	return (count-1);
}

void CosicDrv_InitDataTransfer (int iHandle, int iRetVal);
extern IFX_SSC_HANDLE *SPI_CB_Dummy_handler;
/*******************************************************************************
Description: write system call mapped from the Application
1. Application invokes the write system call to send command to CVoIP
2. If Write buffers are full - return fail
3. Lock the Write Index
4. Update write buffer
5. Free write index
6. See Driver mode 
if Driver is in Idle mode - 
  pull down CS to notify CVoIP of write
  return
if Driver is in read mode
  start sending data in next read cycle.
if Driver is in write mode
  schedule process for next write cycle.
Arguments:
Note:
*******************************************************************************/
int cosic_drv_write(struct file *file_p, char *buf, size_t count, loff_t *ppos)
{
  int i, Diff;
	x_CosicDrv_Buffer xTxBufferLocal;
  IFX_SSC_ASYNC_CALLBACK_t xSscTaskletcb;
  //DBGN("<cosic_drv_write>Entry\n");
  /* Check if the buffer is full */
  if(IsTxBufFull){
#ifdef DRV_STATS
    TxBuffFull++;
#endif		
    DBGE("<cosic_drv_write>Transmit Buffer full\n");
    return -1;
  }
  DBGH("<cosic_drv_write> TxBufTail is %d, TxBufHead is %d\n",TxBufTail,TxBufHead);
	memset(&xTxBufferLocal,0,sizeof(x_CosicDrv_Buffer));
  if(copy_from_user(xTxBufferLocal.acCommand,buf,count)){
    DBGE("<cosic_drv_write>Failed to copy from user\n");
    return -EFAULT;
  }
	xTxBufferLocal.acCommand[count]='\r';
  xTxBufferLocal.len = count+1;
//#if DBGH == printk
	printk("Data written by app on Tail %d is \n+++++[",TxBufTail);
	for (i=0; i<count; i++){
		//DBGH("%c",xTxBuffer[TxBufTail].acCommand[i]);
		printk("%c",xTxBufferLocal.acCommand[i]);
	}
	printk("]+++++\n");
//#endif
	Diff = TxBufTail-TxBufHead;
	if(Diff < 0)
		Diff += MAX_WRITE_BUFFERS;

	printk("write, Diff is %d\n",Diff);
	if((Diff == 0) || (Diff == 1) || 
			((xTxBuffer[TxBufTail].len + xTxBufferLocal.len) > MAX_COMMAND_SIZE)){
  	memset(&xTxBuffer[TxBufTail],0,sizeof(x_CosicDrv_Buffer));
		memcpy(xTxBuffer[TxBufTail].acCommand,xTxBufferLocal.acCommand,xTxBufferLocal.len);
		xTxBuffer[TxBufTail].len = xTxBufferLocal.len;
		printk("Written in new buffe %d\n",TxBufTail);
  	INC_TXBUF_TAIL;
	}else{
		if (TxBufTail == 0)
			Diff = MAX_WRITE_BUFFERS;
		else
			Diff = TxBufTail;
		memcpy(&xTxBuffer[Diff-1].acCommand[xTxBuffer[Diff-1].len],xTxBufferLocal.acCommand,xTxBufferLocal.len);
		xTxBuffer[Diff-1].len += xTxBufferLocal.len;
		printk("appended in same buffe %d\n",Diff);
	}
#ifdef DRV_STATS	
  if(TxOverflowFlag){
    TxRollOver++;
  }
#endif
  /* Print the received buffers */
/*
  for(i=0;i<TxBufTail;i++){
    printk("Str:[%s],Len[%d]\n",xTxBuffer[i].acCommand,xTxBuffer[i].len);
  }
*/
  if(COSIC_DRV_MODE_IDLE == eDriverMode){
    DBGN("<cosic_drv_write>Driver is in Idle mode\n");
    xSscTaskletcb.pFunction = CosicDrv_InitDataTransfer;
    xSscTaskletcb.functionHandle = (int)SPI_CB_Dummy_handler;
    eDriverMode = COSIC_DRV_MODE_WR;
    DBGN("<cosic_drv_write>Calling Async Lock\n");
    if(ifx_sscAsyncLock(spi_dev_handler,&xSscTaskletcb) < 0) {
      DBGE("<cosic_drv_write>SPI Lock Failed\n");
      return -1;
    }
  }else{
		DBGN("write, Cosic driver is not in idle mode..\n");
	}
  //DBGN("<cosic_drv_write>Exit\n");
  return count;
}

/*******************************************************************************
Description:
   The function for the system call "select".
Arguments:
Note:
*******************************************************************************/
int cosic_drv_poll(struct file *file_p, poll_table *wait)
{
  int ret = 0;
	DBGN("cosic_drv_poll, Entry\n");
  /* install the poll queues of events to poll on */
  poll_wait(file_p, &Dect_WakeupList, wait);
	DBGH("cosic_drv_poll, Dect_excpFlag:%d, IsRx1BufEmpty:%d\n",Dect_excpFlag,IsRxBufEmpty);
  if (Dect_excpFlag){
   if (!(IsRxBufEmpty)) 
          ret |= (POLLIN | POLLRDNORM);
    Dect_excpFlag = 0;
	}

	DBGN("cosic_drv_poll, Exit\n");
  return ret;
}

/*******************************************************************************
Description:
   Handle IOCTL commands.
Arguments:
Note:
*******************************************************************************/
int cosic_drv_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
  switch(cmd)
  {
#ifdef CVOIP_ONBOARD
		case DECT_DRV_SSC_HW_RESET:
    {
    	unsigned char  data;

      copy_from_user(&data, (unsigned char *)arg, sizeof(unsigned char));
			ssc_dect_hardware_reset(1);
			printk("ioctl data is ===%c===,===%x===\n",data,data);
      break;
    }
#endif
	  default :
      break;
  }
  return 0;
}

void CosicDrv_GPIOInit(void);
irqreturn_t CosicDrv_ISR(int irq, void *dev_id);
void gp_onu_enable_external_interrupt(void);
/*******************************************************************************
Description:  Open the Dect driver.
Arguments:
Note:
*******************************************************************************/
int cosic_drv_open(struct inode *inode, struct file *filp)
{
  IFX_SSC_CONFIGURE_t ssc_cfg = {0};
  unsigned int num = MINOR(inode->i_rdev);
  DBGN("<cosic_drv_open>Entry\n");
  dect_gpio_intnum = DECT_IRQ_NUM;
  if (num > COSIC_DRV_MINOR_1)  {
    DBGE("DECT Driver: Minor number error!\n");
    return -1;
  }
  /* Return error in case multiple devices open this */
  if(iCosicDrv_Opencount){
      DBGE("<cosic_drv_open>Multiple opens\n");
      return -1;
  }
  iCosicDrv_Opencount++;
  filp->private_data = (void *)num;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
  MOD_INC_USE_COUNT;
#endif
  //atomic_set(&vuiGwDbgFlag,1);
  /* GPIO initialization */
  CosicDrv_GPIOInit();
  /* Alloc a SPI connection */	
  ssc_cfg.baudrate=4000000; // TODO: baudrate is int valiable????
  ssc_cfg.csset_cb=(int (*)(u32,IFX_CS_DATA))CosicDrv_SPICallback;
  ssc_cfg.cs_data=GPIO_CS;
  ssc_cfg.fragSize=32; //TODO: Changed from 16 to 32 as its min;
  ssc_cfg.maxFIFOSize=32; //TODO: Changed from 16 to 32 as its min;
  ssc_cfg.ssc_mode=IFX_SSC_MODE_3;
  ssc_cfg.ssc_prio=IFX_SSC_PRIO_ASYNC;
#ifdef HDX_CONFIG
  ssc_cfg.duplex_mode=IFX_SSC_HALF_DUPLEX;
#else
  ssc_cfg.duplex_mode=IFX_SSC_FULL_DUPLEX;
#endif
  DBGH("<cosic_drv_open>Calling SSC Alloc connection\n");
  spi_dev_handler=ifx_sscAllocConnection(COSIC_DEVICE_NAME, &ssc_cfg);
  if(spi_dev_handler == NULL){
    DBGE("failed to register with spi device \n");
    return -1;
  }
  init_waitqueue_head(&Dect_WakeupList);
#ifdef CONFIG_PROC_FS_1
  drv_dect_procfs_init();
  DGBH("procfs initialized\n");
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
  if( !request_irq( dect_gpio_intnum,CosicDrv_ISR,IRQF_DISABLED, "Cosic", NULL))
#else
  if( !request_irq( dect_gpio_intnum,CosicDrv_ISR, SA_INTERRUPT, "Cosic", NULL))
#endif 
  { 
		DBGN("enabling external interupts..\n");
    gp_onu_enable_external_interrupt();	  
  } 
  else 
  { 
    DBGE("can't get assigned irq %d\n",dect_gpio_intnum);
    dect_gpio_intnum = -1;	  
  }
  /*Reset all the global data here*/
  CosicDrv_ResetGlobal();
  DBGN("<cosic_drv_open>Exit\n");
  return 0;
}


/*******************************************************************************
Description: Close the Dect driver.
Arguments:
Note:
*******************************************************************************/
int cosic_drv_release(struct inode *inode, struct file *filp)
{
	--iCosicDrv_Opencount;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    MOD_DEC_USE_COUNT;
#endif
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
    }
		ifx_sscFreeConnection(spi_dev_handler);
		spi_dev_handler = NULL;
	}
  ifx_gpio_deregister(IFX_GPIO_MODULE_DECT);

  return 0;
}


/*******************************************************************************
Description: register Dect driver to kernel
Arguments:
Note:
*******************************************************************************/
int cosic_drv_init(void)
{
  int result;
  result = register_chrdev(COSIC_DRV_MAJOR, COSIC_DEVICE_NAME, &cosic_drv_fops);
  
  if (result < 0)
  {
    DBGE(KERN_INFO "DECT Driver: Unable to get major %d\n", COSIC_DRV_MAJOR);
    return result;
  }
#ifdef CONFIG_LTT
  result = marker_set_probe("cosic_event",
                            "long %lu string %s",
                            probe_cosic_event);
  if (!result){
    DBGE("Unable to set cosic marker\n");
  }
#endif
	 DBGN("Cosic module is inserted successfully...\n");
   return 0;
}


/*******************************************************************************
Description:   clean up the DECT driver.
Arguments:
Note:
*******************************************************************************/
void cosic_drv_cleanup(void)
{

#ifdef CONFIG_LTT
  marker_remove_probe(probe_cosic_event);
#endif
	unregister_chrdev(COSIC_DRV_MAJOR, COSIC_DEVICE_NAME);
#ifdef CONFIG_PROC_FS_1
#ifndef UNITTEST
	remove_proc_entry("version",dect_proc_dir);
	remove_proc_entry("gw-stats",dect_proc_dir);
	remove_proc_entry("modem-stats",dect_proc_dir);
	remove_proc_entry("channel-stats",dect_proc_dir);
#else
	remove_proc_entry("stub",dect_proc_dir);
	remove_proc_entry("driver/dect",NULL);
	create_dir--;
#endif
#endif /*CONFIG_PROC_FS*/
}
module_init(cosic_drv_init);
module_exit(cosic_drv_cleanup);

#ifdef CONFIG_PROC_FS_1
#ifdef UNITTEST
int dect_drv_write_proc(struct file *file,const char *buffer,unsigned long count,void *data)
{
  int len=4;
  char kdata[10]= {0};
	printk("Entry in dect_drv_write_proc\n");
  if(copy_from_user(&kdata, buffer, len)) 
  {
    printk("\ncopy from user failed in proc system\n");
    return -EFAULT;
  }
	vcCorruption = kdata[3] - '0';
	vcMode = kdata[0] - '0';
	vcRxData = kdata[2] - '0';
	if (vcRxData == 0)
		vcRxData = 10;
	else if (vcRxData == 1)
		vcRxData = 14;
	else if (vcRxData == 2)
		vcRxData = 27;
	if (vcMode == 0)
		printk("GW -->> CVoIP..............\n");
	else if (vcMode == 1)
		printk("CVoIP -->> GW..............\n");
	else if (vcMode == 2)
		printk("Fullduplex..............\n");

	switch(kdata[1] - '0')
  {
    case 0:
    {
			printk("0, pkt len < 13 ...., TxBufTail is %d\n",TxBufTail);
  		memset(&xTxBuffer[0],0,sizeof(x_CosicDrv_Buffer));
  		memset(&xTxBuffer[0],'1',9);
			xTxBuffer[0].len = 9;
			xTxBuffer[0].index = 0;
    	tasklet_FallingEdge(0);
      break;
    }
    case 1:
    {
			printk("1 pkt len = 13 ....fallingedge\n");
  		memset(&xTxBuffer[0],0,sizeof(x_CosicDrv_Buffer));
  		memset(&xTxBuffer[0],'1',13);
			xTxBuffer[0].len = 13;
			xTxBuffer[0].index = 0;
    	tasklet_FallingEdge(0);
      break;
    }
    case 2:
    {
			printk("2 pkt len = 14 ....fallingedge\n");
  		memset(&xTxBuffer[0],0,sizeof(x_CosicDrv_Buffer));
  		memset(&xTxBuffer[0],'1',14);
			xTxBuffer[0].len = 14;
			xTxBuffer[0].index = 0;
    	tasklet_FallingEdge(0);
      break;
    }
    case 3:
    {
			printk("2 pkt len = 27 ....fallingedge\n");
  		memset(&xTxBuffer[0],0,sizeof(x_CosicDrv_Buffer));
  		memset(&xTxBuffer[0],'1',27);
			xTxBuffer[0].len = 27;
			xTxBuffer[0].index = 0;
    	tasklet_FallingEdge(0);
      break;
    }
    case 4:
    {
			printk("Full duplex......\n");
  		memset(&xTxBuffer[0],0,sizeof(x_CosicDrv_Buffer));
  		memset(&xTxBuffer[0],'1',10);
			xTxBuffer[0].len = 10;
			xTxBuffer[0].index = 0;
    	tasklet_FallingEdge(0);
      break;
    }
    case 9:
    {
			printk("COSIC VoIP initiated data.....\n");
  		memset(&xTxBuffer[0],0,sizeof(x_CosicDrv_Buffer));
			//vcCorruption = 9;
    	tasklet_FallingEdge(0);
      break;
    }
    default:
    {
       printk("\nWRONG input from proc file system = %s\n",kdata);
    }
  }
	printk("\n2nd byte input from proc file system = %d\n",vcCorruption);
  return count;
}
#endif
			
int dect_drv_read_proc(char *page, char **start, off_t off,
								int count, int *eof, void *data)
{
	int len;
	int (*fn)(char *buf);
	printk("Entry in dect_drv_read_proc\n");
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

int dect_drv_stub_proc(char *buf)
{
	return 1;
}
static void drv_dect_procfs_init(void)
{

  printk("Initializing procfs flag is %d\n", create_dir);
	if(!create_dir){
	dect_proc_dir = proc_mkdir("driver/dect", NULL);
  create_dir++;
	if (dect_proc_dir != NULL) {
#ifndef UNITTEST
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
		dect_proc_dir->owner = THIS_MODULE;
#endif
		dect_version_file = create_proc_read_entry("version", S_IFREG|S_IRUGO, dect_proc_dir,
										dect_drv_read_proc, dect_drv_read_version_proc);
		if(dect_version_file != NULL) {
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
#endif
#ifdef UNITTEST
  	printk("Creating stub under /proc/driver/dect/\n");
    dect_stub_file = create_proc_entry("stub", 0644, dect_proc_dir);
		if(dect_stub_file != NULL)
    {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
       dect_stub_file->owner = THIS_MODULE;
#endif
       dect_stub_file->data = dect_drv_stub_proc;
       dect_stub_file->read_proc = dect_drv_read_proc;
       dect_stub_file->write_proc = dect_drv_write_proc;
    }else{
  	printk("Initializing procfs failed2\n");
	}

#endif
	}else{
  	printk("Initializing procfs failed\n");
	}

	}
}
#endif /* DRV_DECT_C */

#endif
