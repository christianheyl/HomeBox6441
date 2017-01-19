/******************************************************************************

                               Copyright (c) 2010
                            Lantiq Broadband North America
                     40 Middlesex Turnpike; Bedford MA 01730, USA

**
** FILE NAME    : hnDrv.c
** PROJECT      : HN
** MODULES      : HN module
**
** DATE         : 5 Oct 2010
** AUTHOR       : Rick Han
** DESCRIPTION  : HN device driver source file. This file is also used 
**                by WINDOWS application to build HN remote control messages.
**                The code shared by WINDOWS application is protected by _WINDOWS
**                define. 
**
** HISTORY
** $Date        $Author         $Comment
** 29 SEPT 2010 Rick Han        Working version
******************************************************************************/

#ifndef _WINDOWS

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/signal.h>
#include <linux/mm.h>
#include <linux/string.h>


#else
#include <string.h>
#include "HNCtrlClient.h"

#define EFAULT   1
#define EINVAL   2
#define ENOTSUPP 3
#define ENOMSG   4

#endif /* _WINDOWS */



#include "ltq_hn_hnDrv.h"
#include "ltq_hn_mbMod.h"
#include "localdevice_mb.h"

#define HN_PRINTMSG 0

/* define data copy macro for both Linux kernel and windows */
static int COPY_FROM_USER(void* dest, void* src, int len)
{
#ifndef _WINDOWS
  return copy_from_user(dest, src, len);
#else
  memcpy(dest, src, len);
  return 0;
#endif 
}

static int COPY_TO_USER(void* dest, void* src, int len)
{
#ifndef _WINDOWS
  return copy_to_user(dest, src, len);
#else
  memcpy(dest, src, len);
  return 0;
#endif 
}

static unsigned int HTONL(unsigned int val)
{
#if defined(SIMULATION) || defined(_WINDOWS)
  return htonl(val);
#else
  return val;
#endif
}

static unsigned short HTONS(unsigned short val)
{
#if defined(SIMULATION) || defined(_WINDOWS)
  return htons(val);
#else
  return val;
#endif
}

#ifndef _WINDOWS
#if 0
/************************************************************
 * extern functions (defined in hn_mbMod.c)
 ************************************************************/
int RegisterMailBox(MBClient_t *cp);
void DeRegisterMailBox(int clientId);
int SendMsgToMailBox(int mbNum,
		     unsigned int id,
		     unsigned int type, 
		     MBMsg_t* msg, 
		     int msgLen, 
		     unsigned char blocking, 
		     MBMsg_t** reply);
void* GetMBBuffer(int num, MBMsgStreamE s);
void SendErrReply(int mbNum, MB_ERROR_CODES err);
MBMsgStreamE GetMBBufState(unsigned int num, 
			   MBMsgStreamE stream);
void SetMBBufState(unsigned int num, 
		   MBMsgStreamE stream,
		   MBStreamStateE state);
#endif
/************************************************************
 * proc files 
 ************************************************************/
static struct proc_dir_entry *hnProcfsPtr = 0;

static int HN_RegisterProcfs(void);
static void HN_DeRegisterProcfs(void);
static int HN_ProcRead(char *buffer, 
		       char **buffer_location,
		       off_t offset, int buffer_length, int *eof, 
		       void *data);

/************************************************************
 * device operations
 ************************************************************/
static int HN_RegisterDevice(void);
static int HN_ModuleOpen(struct inode *, struct file *);
static int HN_ModuleClose(struct inode *, struct file *);
int HN_ModuleIoctl(struct inode *, struct file *, 
			  unsigned int, unsigned long);

/* mmap is for copying VPE1 binary from the user space */
//static int HN_ModuleMMap(struct file *fileP, struct vm_area_struct *vma);
//static void HN_VMOpen(struct vm_area_struct *vma); 
//static void HN_VMClose(struct vm_area_struct *vma);
//static HNVpeBinBuffer_t vpeBinBuf;
//static int HN_SetVpe(unsigned long);
//static int HN_LoadVpeBinary(void);

static int HN_SendHelloToVpe1(unsigned int *payload);
static int HN_ValidateRequestReply(MBMsgHdr_t* hdr, HNMsg_t* hnMsg, 
				   HNMsgTypeE hnMsgType);
static int HN_SendMsg(unsigned long arg);
static int HN_GetMsg(unsigned long arg);
#endif /* !_WINDOWS */

/**** used by both Linux kernel and Windows application */
static int HNIOCTL_SetHW(HNDrvIoctlCmd_t* op);
static int HNIOCTL_GetHW(HNDrvIoctlCmd_t* op);
static int HN_BuildRequestMsg(HNMsg_t* hnMsg,
			     HNMsgTypeE hnMsgType,
			     HNDrvIoctlCmd_t* op);
static int HN_SetDeviceParam (unsigned long arg);
static int HN_GetDeviceParam (unsigned long arg);
static int HN_SendDebugMsg(unsigned long arg);
static int HN_SendDebugMemOperMsg(LocalDeviceDebugAccess_t* msmOp);
int ProcVPE1Msg(MBMsg_t* msg);

#ifdef _WINDOWS
static int HN_SendResetMsg(unsigned long arg);
#endif

#ifndef _WINDOWS 
static int hn_major = 0;        /* Major Device Number */
static int hn_minor = 0;
static HNDriver_t *hnDrv;
static struct file_operations hn_fops = {
  .open    = HN_ModuleOpen,
  .release = HN_ModuleClose,
  .ioctl   = HN_ModuleIoctl,
  //.mmap    = HN_ModuleMMap,
};

/************************************************************
 * Mailbox message handler 
 ************************************************************/
static int hello_mb = 1;
static int cfg_mb = 0;
static int debug_mb = 2;

static int replyFlag[MB_NUM];
static MBClient_t* hnMBClient = 0;
static HNMBSubscriber_t* hnMsgSubscribers = 0;
static HNMsgBuffer_t hnMsgPlaceholder[HN_FIFO_LEN];

static int HN_RxMsgHandler(int mbNum, void* data);
static int HN_AttachMailBox(void);
static void HN_DeAttachMailBox(void);
//static int HN_SendToMB(int mbNum, MBMsg_t* msg);
static int HN_SubscribeMB(int sigNo, HNMsgTypeE type);
static void HN_AddSubscriber(HNMBSubscriber_t* sub);
static void HN_RemoveSubscriber(HNMsgTypeE type);

static HNMBSubscriber_t* HN_FindMsgSubscriber(HNMsgTypeE type);
static HNMsgBuffer_t* HN_AddToHNMsgPool(int len, HNMsgTypeE type, void* data);
static void HN_AddToSubscriberFifo(HNMBSubscriber_t* subscriber, 
				   HNMsgBuffer_t* buf);
static HNMsgBuffer_t* HN_PopSubscriberFifo(HNMBSubscriber_t* subscriber);
static void HN_FlushSubscriberFifo(HNMBSubscriber_t* subscriber);
/*static void HN_FreeHNMsgBuffer(HNMsgBuffer_t* buf); */

static void HN_PrintMsg(MBMsg_t* msg);

/************************************************************
 * Local static functions 
 ************************************************************/
static int HN_Init(HNInitModeE mode);

/* No action when start and close. Temporary commenting out */
#if 0
static int HN_Start(void);
static int HN_Stop(void);
#endif 
/************************************************************
 * Function defines 
 ************************************************************/

int HN_ProcRead(char *buffer, 
		char **buffer_location,
		off_t offset, int buffer_length, int *eof, 
		void *data)
{
  int len = 0;
  
  if(hnDrv)
    {
        /* get semaphore */
      down(&(hnDrv->hnSem));
      len += sprintf(buffer+len, "HN driver module: \n");
      len += sprintf(buffer+len, "initialized (%d) started (%d)\n", 
		     hnDrv->initialized, hnDrv->started);
	 
      /* release semaphore */
      up(&(hnDrv->hnSem));
    }

  return len;
}

int HN_RegisterProcfs()
{
  int status = HN_SUCCESS;

  /* create a proc_fs for the drive*/
  hnProcfsPtr = create_proc_read_entry(HN_PROC_NAME, 
				       0444, 
				       0,    /* /proc */
				       HN_ProcRead, 0 );
  
  HN_INFO("%s: /proc/%s created\n", __func__, HN_PROC_NAME);

  return status;
}


void HN_DeRegisterProcfs(void)
{
  if(0 != hnProcfsPtr)
    {
      remove_proc_entry(HN_PROC_NAME, 0);  
    }
}


int HN_RegisterDevice(void)
{

  int status = HN_SUCCESS;
  dev_t devNo;

  status = alloc_chrdev_region(&devNo, hn_minor, 1, HN_DEVICE_NAME);

  if(HN_SUCCESS != status)
  {
    HN_DEBUG("%s Failed to register device (status %d)\n", 
	     __func__, status);  
  }
  else
    {
      hn_major = MAJOR(devNo);
      
      /* connect the file operations with the cdev*/
      cdev_init(&(hnDrv->cdev), &hn_fops);
      hnDrv->cdev.owner = THIS_MODULE;
      status = cdev_add(&(hnDrv->cdev), devNo, 1); 
      
      if (HN_SUCCESS == status)
        {
	  HN_INFO("%s: Added device /dev/%s as %u:%u\n",
		  __func__, HN_DEVICE_NAME, hn_major, 0);
        }
        else
        {
	  HN_DEBUG("%s: Failed to add device /dev/%s \n",
                   __func__, HN_DEVICE_NAME);
        }  
    }

    return status;
}


int HN_ModuleOpen(struct inode *inode, struct file *file)
{
  int status = HN_SUCCESS;

  if(0 != hnDrv)
    {
      /* Start HN device if we have not done so */
      hnDrv->users++;
      file->private_data = (void*)hnDrv;
      
      if(0 == hnDrv->started)
	{
	  //status = HN_Start();
	}
    }
  else
    {
      HN_INFO("%s: Failed to open HN driver module.\n", __func__); 
      status = -ENODEV;
    }
  
  return status;
}

int HN_ModuleClose(struct inode *inode, struct file *file)
{
  int status = HN_SUCCESS;
  if(0 != hnDrv) 
    {
      hnDrv->users --;
      
      if(0 == hnDrv->users)
	{
	  //status = HN_Stop();
	}
    } 
  return status;
}

#if 0 
int HN_ModuleMMap(struct file *fileP, struct vm_area_struct* vma)
{    
  unsigned long vAddr;
  int size = vma->vm_end - vma->vm_start; 
  void *bp;
  unsigned char* bufArea; 

  HN_INFO("%s request size %u\n", __func__, size);


  bp = (void*)kmalloc(size, GFP_KERNEL);
  bufArea = (unsigned char*)(((unsigned long)bp + 
			      PAGE_SIZE -1) & PAGE_MASK);
  
  for (vAddr = (unsigned long)bufArea; 
       vAddr < (unsigned long)bp + MB_BUF_SIZE;
       vAddr+=PAGE_SIZE)
    {
      SetPageReserved(virt_to_page(vAddr));
    }

  HN_INFO("%s reserved pages\n", __func__);


  vma->vm_flags |= (VM_LOCKED | VM_RESERVED); 
  vma->vm_ops = 0;
  
  if(remap_pfn_range(vma, vma->vm_start, 
		     (virt_to_phys(bufArea))>>PAGE_SHIFT,
		     vma->vm_end - vma->vm_start,
		     vma->vm_page_prot))
    {
      HN_DEBUG("%s: Can not map adress to user space\n", __func__);
      return -EAGAIN;
    } 

  /* take a note so we will be able to free the buffer 
     when is is done with VPE */
  if(0 != vpeBinBuf.buf)
    {
      kfree(bp);
    }

  vpeBinBuf.size = size;
  vpeBinBuf.buf = bp; 
  HN_INFO("%s mapped mem %p size %u\n", __func__, vpeBinBuf.buf, vpeBinBuf.size);
  
  return 0;
}

void HN_VMOpen(struct vm_area_struct *vma)
{
  HN_DEBUG("%s: virt: 0x%lx phys: 0x%lx\n", 
	   __func__, vma->vm_start, vma->vm_pgoff<<PAGE_SHIFT);
}

void HN_VMClose(struct vm_area_struct *vma)
{
  HN_DEBUG("%s \n", __func__); 
}
#endif 

/**
 *  Initialize HN driver module
 **/
int __init HN_ModuleInit (void)
{
  int retVal;

#ifdef NEW_CHAR_DRIVER
  dev_t dev;
#endif

  HN_INFO("%s: Initialize HN driver module\n", __func__);
  
  hnDrv = kmalloc(sizeof(HNDriver_t), GFP_KERNEL);
  if ( 0 == hnDrv) 
    {
      HN_DEBUG("%s: Error: Cannot malloc HN driver module\n", __func__);
      return -ENOMEM;
    }
  
  /* Initialize driver module local data */
  hnDrv->initialized = 0;
  hnDrv->started = 0;
  init_MUTEX(&(hnDrv->hnSem));
   
  /* Register procfs */
  retVal = HN_RegisterProcfs();
  if(HN_SUCCESS == retVal)
    {
      /* Register device */
      
      retVal = HN_RegisterDevice();
      
      if(HN_SUCCESS == retVal)
	{
	  retVal = HN_Init(HNInitMode_Cold);
	}
    }

  if(HN_SUCCESS == retVal)
    {
      HN_INFO("%s: HN driver module initialized\n", __func__);
      hnDrv->initialized = 1;
    }
  else
    {
      HN_INFO("%s: HN driver module initialization failed\n", __func__);
      hnDrv->initialized = 0;
    }

  return retVal;
}

/**
 *  exit MB module
 **/
void __exit HN_ModuleExit (void)
{
  HNMBSubscriber_t *subP, *curP;

  HN_INFO("%s: Exiting HN driver module \n", __func__);

  /* Remove the cdev */
  cdev_del(&hnDrv->cdev);
  
  /* de-attach mailbox */
  if(0 != hnMBClient)
    {
      HN_DeAttachMailBox();
      kfree(hnMBClient);
      hnMBClient = 0;
    }

  /* Release the major number */
  unregister_chrdev_region(MKDEV(hn_major, hn_minor), 1);
  
  /* Remove proc entry */
  HN_DeRegisterProcfs();
  
  /* free resources */
  kfree(hnDrv);
  hnDrv = 0;

  if(0 != hnMBClient)
    {
      kfree(hnMBClient);
      hnMBClient = 0;
    }

  subP = hnMsgSubscribers;
  while (0 != subP)
    {
      curP = subP;
      subP= curP->next;
      kfree(curP);
    }

  HN_INFO("%s: HN driver module is removed.\n", __func__);
}


/************************************************************
 * Static functions 
 ************************************************************/
int HN_Init(HNInitModeE mode)
{
  int status = HN_SUCCESS;
  int mb;

  /* allocate resources for mailbox operation */
  for (mb = 0; mb < MB_NUM; mb++)
    {
      replyFlag[mb] = -1;
    }
  
  memset((void*)hnMsgPlaceholder, 0, sizeof(HNMsgBuffer_t)*HN_FIFO_LEN);
  //memset((void*)&vpeBinBuf, 0, sizeof(HNVpeBinBuffer_t));

  /* attach to mailbox */
  HN_AttachMailBox();

  switch(mode)
    {
    case HNInitMode_Cold:
      break;

    case HNInitMode_warm:
      break;
    default:
      HN_DEBUG("%s: Invalid initialization mode (%i).\n", 
	       __func__, mode);
      status = -EINVAL;
      break;
    }

  return status;
}

#if 0 /* Not clear what do we need to do to the firmware when start */
int HN_Start(void)
{
  int status = HN_SUCCESS;

  HN_DEBUG("%s \n", __func__);
  hnDrv->started = 1;
  return status;
}

int HN_Stop(void)
{
  /* stop mailbox polling */
  HN_DEBUG("%s \n", __func__);
  hnDrv->started = 0;
  return HN_SUCCESS;
}
#endif 

/************************************************************
 * mail box I/O 
 ************************************************************/
int HN_AttachMailBox()
{
  int status = HN_SUCCESS;
  
  if(0 != hnMBClient)
    {
      /* This should not happen. But just in case, we de-attach 
       * the mailbox and restart again 
       */
      DeRegisterMailBox(hnMBClient->devId);
      kfree(hnMBClient);
    }

  hnMBClient = (MBClient_t*)kmalloc(sizeof(MBClient_t), GFP_KERNEL);
  if(0 == hnMBClient)
    {
      HN_DEBUG("%s: Failed to create a mailbox client.\n", __func__);
      status = -ENOMEM;
    }
  else
    {
      hnMBClient->devId = MB_MSG_DEV_HN;
      hnMBClient->rxMsgHdlr = HN_RxMsgHandler;
      
      status = RegisterMailBox(hnMBClient);
      if(HN_SUCCESS !=  status)
	{
	  HN_DEBUG("%s: Failed to register mailbox\n", __func__);
	  kfree(hnMBClient);
	  hnMBClient = 0;
	  status = -EIO;
	}
    }
  
  return status;
}

void HN_DeAttachMailBox()
{
  DeRegisterMailBox(hnMBClient->devId);
  kfree(hnMBClient);
  hnMBClient = 0;
}

int HN_SubscribeMB(int sigNo, HNMsgTypeE type)
{
  HNMBSubscriber_t *subscriber = 0; 			
  HNMBSubscriber_t *newSubP = 0;
  
  int status = HN_SUCCESS;
  
  //subscriber = hnMsgSubscribers;
  subscriber = HN_FindMsgSubscriber(type);
  if(0 != subscriber)
    {
      /* someone already subscribed this message type, over write 
	 the exsting subscriber
      */
      if(type == subscriber->type)
	{
	  /* the existing subscriber is only responsible 
	     for this message type. Replace it with a new one. 
	  */
	  HN_FlushSubscriberFifo(subscriber);
	  HN_RemoveSubscriber(subscriber->type);
	  kfree(subscriber);
	}
      else
	{
	  /* reset this type bit in the existing subscriber */
	  subscriber->type &= ~(type);
	  HN_FlushSubscriberFifo(subscriber);
	}
    }
  
  /* Create a new subscriber and insert into out list */
  newSubP = kmalloc(sizeof(HNMBSubscriber_t), GFP_KERNEL);
  if(0 == newSubP)
    {
      status = -ENOMEM;
      goto exit;
    }

  /* Init subscriber object */
  newSubP->task = current;
  newSubP->sigNo = sigNo;
  newSubP->type = type;
  newSubP->fifo = 0;

  HN_AddSubscriber(newSubP);

 exit:
  return status; 
}

void HN_AddSubscriber(HNMBSubscriber_t* sub)
{
  
  sub->prev = 0;
  sub->next = hnMsgSubscribers;
  
  if(0 != hnMsgSubscribers)
    {
      hnMsgSubscribers->prev  = sub;
    }

  hnMsgSubscribers = sub;

  return;
}

void HN_RemoveSubscriber(HNMsgTypeE type)
{
  HNMBSubscriber_t *sub = 0;
  sub = HN_FindMsgSubscriber(type);
  if(0 != sub)
    {
      if (0 == sub->prev)
	{
	  /* I am the first in the list */
	  hnMsgSubscribers = sub->next;
	}
      
      else if(0 == sub->next)
	{
	  /* I am the last in the list and there is one in front of me*/
	  sub->prev->next = 0;
	}
      else 
	{
	  /* I am in the middle of the list */
	  sub->prev->next = sub->next;
	  sub->next->prev = sub->prev;
	}
      kfree(sub);
    }

  return;
}

int HN_RxMsgHandler(int mbNum, void* data)
{
  int status = 0;

  MBMsgHdr_t* hdr = ( MBMsgHdr_t*)data;
  MBMsg_t* msg = (MBMsg_t*)data;
  HNMsg_t* hnMsg;
  HNMBSubscriber_t *subscriber;
  HNMsgBuffer_t *buf;
  
  if(0!= hdr)
    {
      /* This is a messages initiated by VPE1. 
	 Notify the subscriber if we can find one, otherwise
	 drop the message  
      */
      hnMsg = (HNMsg_t*)(msg->payload);
      subscriber = HN_FindMsgSubscriber(hnMsg->type);
      if(0 != subscriber)
	{
	  buf = HN_AddToHNMsgPool((msg->hdr.length),
				  hnMsg->type, data);
	  
	  if(0 == buf)
	    {
	      SendErrReply(mbNum, MB_ERR_FIFO_OVER);
	      status = -EBUSY;
	    }
	  else 
	    {
	      HN_AddToSubscriberFifo(subscriber, buf);
	      
	      if(0 != send_sig(subscriber->sigNo, subscriber->task, 0))
		{
		  HN_FlushSubscriberFifo(subscriber);
		  SendErrReply(mbNum, MB_ERR_NO_HANDLER);
		  status = -ENOTSUPP;
		}
	    }
	}
      else 
	{
	  //We can not find a user space application
	  //to handle this message. Dispatch to driver
	  //ProcVPE1Msg()
	  status = ProcVPE1Msg(msg);
	}
    }

  SetMBBufState(mbNum, inMsgS, MB_EMPTY);
  return status;
}

int ProcVPE1Msg(MBMsg_t* msg)
{
  MBMsgHdr_t* hdr = (MBMsgHdr_t*)msg;
  char* msgType;
  
  switch(MSG_TYPE(hdr))
    {
    case MB_MSG_TYPE_HELLO:
      msgType = "HELLO";
      break;

    case MB_MSG_TYPE_NOTIFY:
      msgType = "NOTIFY";
      break;

    case MB_MSG_TYPE_CRITICAL_NOTIFY:
      msgType = "CRITICAL";
      break;

    case MB_MSG_TYPE_REQUEST:
      msgType = "REQUEST";
      break;

    case MB_MSG_DEBUG_ACCESS:
      msgType = "ACCESS";
      break;
      
    default:
      msgType = "UNKNOWN";
      break;
    }

  HN_INFO("RECV VPE1 MSG: type %s \n", msgType);
  return HN_SUCCESS;
}


HNMBSubscriber_t* HN_FindMsgSubscriber(HNMsgTypeE type)
{
  HNMBSubscriber_t* subP = 0;
    
  subP = hnMsgSubscribers;
  while(0 != subP)
    {
      if(0 != (type & subP->type))
	{
	  break;
	}
      
      subP = subP->next;
    }

  return subP;
}

HNMsgBuffer_t* HN_AddToHNMsgPool(int len, HNMsgTypeE type, 
				 void* data)
{
  HNMsgBuffer_t* bufP = 0;
  int i;

  if(len > MB_BUF_SIZE)
    {
      return 0;
    }

  for (i = 0; i<HN_FIFO_LEN; i++)
    {
      if(0 == hnMsgPlaceholder[i].data)
	{
	  down(&(hnDrv->hnSem));

	  memcpy((void*)(hnMsgPlaceholder[i].buf), 
		 data, len);
	  hnMsgPlaceholder[i].len = len;
	  hnMsgPlaceholder[i].type = type;
	  hnMsgPlaceholder[i].data = 1;

	  up(&(hnDrv->hnSem));
	  break;
	}
    }
  
  if(HN_FIFO_LEN != i)
    {
      bufP = &hnMsgPlaceholder[i];
    }

  return bufP;
}

void HN_AddToSubscriberFifo(HNMBSubscriber_t* subscriber, 
			    HNMsgBuffer_t* buf)
{
  HNMsgBuffer_t* bufP = subscriber->fifo;
  HNMsgBuffer_t* curP = 0;

  if(0 == bufP)
    {
      buf->prev = 0;
      buf->next = 0;
      subscriber->fifo = buf;
    }
  else 
    {
      while(0 != bufP)
	{
	  curP = bufP;
	  bufP = curP->next;
	}
      curP->next = buf;
      buf->next = 0;
      buf->prev = curP;
    }
}

HNMsgBuffer_t* HN_PopSubscriberFifo(HNMBSubscriber_t* subscriber)
{
  HNMsgBuffer_t* top = 0;
  HNMsgBuffer_t* next = 0;
  
  if(0 != subscriber->fifo)
    {
      top = subscriber->fifo;
      next = subscriber->fifo->next;
      subscriber->fifo = next;
    }

  return top;
}

void HN_FlushSubscriberFifo(HNMBSubscriber_t* subscriber)
{
  HNMsgBuffer_t* bufP;

  while(0!= (bufP = HN_PopSubscriberFifo(subscriber)))
    {
      if (0 == bufP)
	{
	  break;
	}
      
      bufP->data = 0;
    }
	
  return;
}

#if 0 
void HN_FreeHNMsgBuffer(HNMsgBuffer_t* buf)
{
  /* make the buffer available */
  buf->data = 0;
}
#endif 

int HN_GetMsg(unsigned long arg)
{
  int status = HN_SUCCESS;
  HNDrvIoctlCmd_t op;
  HNGetMsgOpParam_t cmdParam;
  HNMBSubscriber_t*subP = 0;
  HNMsgBuffer_t* bufP = 0;

  memset((void*)&op, 0, sizeof(HNDrvIoctlCmd_t));
  if(0 != COPY_FROM_USER((void*)&op, (void*)arg, 
			 sizeof(HNDrvIoctlCmd_t)))
    {
 
      HN_DEBUG("%s: Cannot copy data from user space.\n", __func__);
      status = -EFAULT;
    }
  else
    {
      if(0 != COPY_FROM_USER((void*)(size_t)&cmdParam,
			     (void*)(size_t)op.param,
			     op.length))
	{
	  HN_DEBUG("%s: Cannot copy param from user space.\n", __func__);
	  status = -EFAULT;
	}     
      
      subP = HN_FindMsgSubscriber(cmdParam.type);
      if(0 != subP)
      	{
	  bufP = HN_PopSubscriberFifo(subP);
	  
	  if(0 != bufP)
      	    {
	      if(0 != COPY_TO_USER((void*)(size_t)(cmdParam.msg), 
				   bufP->buf, 
				   bufP->len))
	      	{
	      	  HN_DEBUG("%s: Cannot copy msg to user space.\n", __func__);
	      	  status = -EFAULT;
	      	}
	    }
	}
      else
      	{
	  status = -EINVAL;
	}
    }
  
  if(0 != bufP)
    {
      bufP->data = 0;
    }

  return status;
}

int HN_SendMsg(unsigned long arg)
{
  int status = HN_SUCCESS;
  HNDrvIoctlCmd_t op;
  MBMsg_t msg;
  MBMsg_t *reply = 0;
  MBMsgHdr_t *hdr;
  HNSendMsgOpParam_t cmdParam;
  int payloadLen = 0;

  memset((void*)&op, 0, sizeof(HNDrvIoctlCmd_t));
  if(0 != COPY_FROM_USER((void*)&op, (void*)arg, 
			 sizeof(HNDrvIoctlCmd_t)))
    {
       HN_DEBUG("%s: Cannot copy data from user space.\n", __func__);
       status = -EFAULT;
    }
  else
    {
      /* Copy message form user space */
      if(0 != COPY_FROM_USER((void*)(size_t)&cmdParam,
			     (void*)(size_t)op.param,
			     op.length))
	{
	  HN_DEBUG("%s: Cannot copy param from user space.\n", __func__);
	  status = -EFAULT;
	}     
      else if(0 != cmdParam.msgLen)
	{
	  //Build mailbox message 
	  if(0 != COPY_FROM_USER((void*)(size_t)&(msg.payload),
				 (void*)(size_t)cmdParam.msg,
				 cmdParam.msgLen))
	    {
	      HN_DEBUG("%s: Cannot copy message from user space.\n", __func__);
	      status = -EFAULT;
	    }
	  else
	    {
	      /* send the message */
	      status = SendMsgToMailBox(cmdParam.mb, 
					MB_MSG_DEV_HN,
					MB_MSG_TYPE_REQUEST,
					&msg,
					cmdParam.msgLen,
					cmdParam.blocking,
					&reply);
	      if((MB_SUCCESS == status) && (0 != reply))
		{
		  /* send data back to user */
		  hdr = (MBMsgHdr_t*)reply;

		  /* get payload length */
		  payloadLen = MSG_LEN(hdr) - sizeof(MBMsgHdr_t);
		  if((0 != payloadLen) && 
		     (0 != COPY_TO_USER((void*)(size_t)(cmdParam.reply),
					(void*)(size_t)(reply->payload),
					payloadLen)))
		    {
		      HN_DEBUG("%s: Cannot copy message from user space.\n", __func__);
		      cmdParam.replyLen = 0;
		      status = -EFAULT;
		    }
		  else
		    {
		      //successfully copied payload to user space */
		      cmdParam.replyLen = payloadLen;
		    }
		  
		  if(0 != COPY_TO_USER((void*)(size_t)op.param,
				       (void*)(size_t)&cmdParam,
				       op.length))
		    {
		      status = -EFAULT;
		    }
		
		  SetMBBufState(cmdParam.mb, inMsgS, MB_EMPTY);
                  kfree(reply);
		}
	    }
	}
    }

  return status;
}


int HN_SendHelloToVpe1(unsigned int *payload)
{
  int status = HN_SUCCESS;
  
  unsigned char wait;
  MBMsg_t msg;
  MBMsg_t* reply = 0;
  MBMsgHdr_t* hdr;

  msg.payload[0] = HN_MSG_TYPE_HELLO;
  msg.payload[1] = *payload;
  
  wait = 1;
  status = SendMsgToMailBox(hello_mb, 
			    MB_MSG_DEV_LOCAL,
			    MB_MSG_TYPE_HELLO,
			    &msg,
			    8,
			    1,
			    &reply);
  if((HN_SUCCESS != status) || (0 == reply))
    {
      goto exit;
    }

  hdr = (MBMsgHdr_t*)reply;
  if((MB_MSG_DEV_HN != MSG_DEVID(hdr)) || 
     (MB_MSG_TYPE_HELLO != MSG_TYPE(hdr)))
    {
      status = -ENOMSG;
    }
  else
    {
      *payload = reply->payload[1];
    }

  SetMBBufState(hello_mb, inMsgS, MB_EMPTY);
  kfree(reply);
  
 exit:
  return status;
}

int HN_ValidateRequestReply(MBMsgHdr_t* hdr, HNMsg_t* hnMsg, 
			    HNMsgTypeE hnMsgType)
{
  if((MB_MSG_DEV_HN != MSG_DEVID(hdr)) || 
     (hnMsgType != hnMsg->type))
    {
      return -ENOMSG;
    }
  else
    {
      return HN_SUCCESS;
    }
}

void HN_PrintMsg(MBMsg_t* msg)
{
  HNMsg_t* hnMsg;
  int i;
  //char s[MB_BUF_SIZE];

  HN_DEBUG(" ===== MB message header === \n");
  HN_DEBUG(" 0x%08x  0x%08x  0x%08x \n", 
	   msg->hdr.type, msg->hdr.seqNo, msg->hdr.length);

  hnMsg = (HNMsg_t*)(msg->payload);
  HN_DEBUG(" ===== HN message header === \n");
  HN_DEBUG(" 0x%08x \n", hnMsg->type); 
  
  HN_DEBUG(" ===== HN message payload === \n");
  HN_DEBUG("  ioctl: 0x%08x\n", hnMsg->cmd);
  for(i = 0; i< ((msg->hdr.length-8)/4); i++)
    {
      HN_DEBUG("  0x%08x ", hnMsg->data[i]);
      if((0 != i) && (i%4 == 0))
	{
	  HN_DEBUG("\n");
	}
    }
#if 0 
  /* special case for character strings */
  if((MB_MSG_CMD_HNDomainName == GetMBMsgCmdType(&(msg->hdr))) ||
     (MB_MSG_CMD_HNSecurityKey == GetMBMsgCmdType(&(msg->hdr))))
    {
      memcpy(s, (void*)(size_t)&(hnMsg->payload[1]), (msg->hdr.length-8));
      HN_DEBUG("string: %s\n", s);
    }
#endif 
}

module_init (HN_ModuleInit);
module_exit (HN_ModuleExit);
MODULE_LICENSE ("GPL");

#endif  /* !_WINDOWS */

 

/**** 
 ***   The code used by both Linux kernel and WINDOWS remote control DLL 
 ****/

int HN_ModuleIoctl(struct inode *inode, struct file *file,
		   unsigned int cmd, unsigned long arg)
{
  int status = HN_SUCCESS;
#ifndef _WINDOWS 
  HNDriver_t * drvP = (HNDriver_t *)file->private_data;

  if((0 == drvP) || (drvP != hnDrv))
    {
      HN_DEBUG("%s: Cannot locate driver module drv 0x%p, hdDrv 0x%p\n", 
	       __func__, drvP, hnDrv);
      return -ENODEV;
    }
  
  if (_IOC_TYPE(cmd) != HN_IOC_MAGIC)
    {
      HN_DEBUG("%s: Invalid command type\n", __func__);
      return -EINVAL;
    }
#endif 

  /* Now process the command */
  switch(cmd)
    {
    case HN_IOCTL_SET_DEV:
      status = HN_SetDeviceParam(arg);
      break;

    case HN_IOCTL_GET_DEV:
      status = HN_GetDeviceParam(arg);
      break;

#ifndef _WINDOWS       
    case HN_IOCTL_SEND_MSG:
      status = HN_SendMsg(arg);
      break;
 
    case HN_IOCTL_GET_MSG:
      status = HN_GetMsg(arg);
      break;
#else
    case HN_IOCTL_RESET:
      status = HN_SendResetMsg(arg);
      break;
#endif 

    case HN_IOCTL_DEBUG:
      status = HN_SendDebugMsg(arg);
      break;

    default:
      status = -EINVAL;
      break;
    }

  return status;
}

/************************************************************
 * IOCTL functions 
 ************************************************************/
int HN_SetDeviceParam(unsigned long arg)
{
  int status = HN_SUCCESS;
  HNDrvIoctlCmd_t op;

#ifdef _WINDOWS 
  op = *(HNDrvIoctlCmd_t *)arg;
#else
//  HN_DEBUG("%s: entering .... \n", __func__);

  memset((void*)&op, 0, sizeof(HNDrvIoctlCmd_t));
  
  if(0 != COPY_FROM_USER((void*)&op, (void*)arg, 
			 sizeof(HNDrvIoctlCmd_t)))
    {
      HN_DEBUG("%s: Cannot copy data from user space.\n", __func__); 
      status = -EFAULT;
    }
  else
#endif 
    {
      switch(op.subCmd)
	{
#ifndef _WINDOWS 
	case HNIoctl_MBSubscribe:
	  {
	    HNSubscriber_t sub;
	    if(0 != COPY_FROM_USER((void*)&sub, (void*)(size_t)op.param,
				   op.length))
	      {
		HN_DEBUG("%s: can not copy subscriber info from user\n", 
			 __func__);
		status = -EFAULT;
	      }
	    else
	      {
		status = HN_SubscribeMB(sub.sigNo, sub.type);
	      }
	  }
	  break;

	case HNIoctl_MBUnSubscribe:
	  { 
	    HNSubscriber_t sub;
	    if(0 != COPY_FROM_USER((void*)&sub, (void*)(size_t)op.param,
				   op.length))
	      {
		HN_DEBUG("%s: can not copy subscriber info from user\n", 
			 __func__);
		status = -EFAULT;
	      }
	    else
	      {
		HN_RemoveSubscriber(sub.type);
		status = HN_SUCCESS;
	      }
	  }
	  break;
#endif 
	default:
	  status = HNIOCTL_SetHW(&op);
	  break;
	}
    }
  
  return status;
}

int HN_GetDeviceParam(unsigned long arg)
{
  int status = HN_SUCCESS;
  HNDrvIoctlCmd_t op;

#ifdef _WINDOWS 
  op = *(HNDrvIoctlCmd_t *)arg;
#else
//  HN_DEBUG("%s: entering .... \n", __func__);

  memset((void*)(&op), 0, sizeof(HNDrvIoctlCmd_t));
  if(0 != COPY_FROM_USER((void*)&op, (void*)arg, 
			 sizeof(HNDrvIoctlCmd_t)))
    {
      HN_DEBUG("%s: Cannot copy data from user space.\n", __func__); 
      status = -EFAULT;
    }
  else
#endif
    {
      switch(op.subCmd)
	{
#ifndef _WINDOWS 
	case HNIoctl_MBSendHello:
	  status = HN_SendHelloToVpe1(&(op.param));
	  
	  /* return the payload in the reply to the user */
	  if(HN_SUCCESS == status)
	    {
	      if (0 != COPY_TO_USER((void*)arg, &op, 
				    (sizeof(HNDrvIoctlCmd_t))))
		{
		  status = -EFAULT;
		}  
	    }

	  break;
#endif 
	default:
	  status = HNIOCTL_GetHW(&op);

	  if(HN_SUCCESS == status)
	    {
	      /* we need to notify the user application about the 
		 length of the data replied by the HW. 
		 NOTE: we only need to copy up to the length field. 
	      */
	      if (0 != COPY_TO_USER((void*)arg, &op, 
				    (sizeof(HNIoctlSubCmdE) + sizeof(int))))
		{
		  status = -EFAULT;
		}  
	    }
	  
	  break;
	}
    }
  
  return status; 
}

int HN_SendDebugMsg(unsigned long arg)
{
  int status = HN_SUCCESS;
  HNDrvIoctlCmd_t op;
  LocalDeviceDebugAccess_t memOp;

#ifdef _WINDOWS 
  op = (HNDrvIoctlCmd_t *)arg;
#else

//  HN_DEBUG("%s: entering .... \n", __func__);

  memset((void*)&op, 0, sizeof(HNDrvIoctlCmd_t));
  if(0 != COPY_FROM_USER((void*)&op, (void*)arg, 
			 sizeof(HNDrvIoctlCmd_t)))
    {
      HN_DEBUG("%s: Cannot copy data from user space.\n", __func__); 
      status = -EFAULT;
    }
  else
#endif  
    {
      switch(op.subCmd)
	{
	case HNIoctl_DebugRead:
	case HNIoctl_DebugWrite:
	  /* copy debug read construction from user */
	  memset((void*)&memOp, 0, sizeof(LocalDeviceDebugAccess_t));
	  if(0 != COPY_FROM_USER((void*)(size_t)&memOp,
				 (void*)(size_t)op.param,
				 op.length))
	     {
#ifndef _WINDOWS
	       HN_DEBUG("%s: Cannot copy param from user space.\n", 
			__func__);
#endif
	       status = -EFAULT;
	     } 
	  status = HN_SendDebugMemOperMsg(&memOp);
	  break;

	default:
	  status = -ENOTSUPP;

	}
    }

  return status;
}


int HN_SendDebugMemOperMsg(LocalDeviceDebugAccess_t* memOp)
{
  int status = HN_SUCCESS;
  MBMsg_t msg;
  MBMsg_t* reply = 0;
  MBMsgHdr_t *hdr;
  char* ptr;
  LocalDeviceDebugAccess_t* rPayload;
  int len = 0;

  
  /* preapre message header */
  
  ptr = (char*)(msg.payload);
 
  /* encoding the operation command to the payload */
  memcpy(ptr, (void*)(size_t)memOp, sizeof(DebugAccessCommand_t));
  len += sizeof(DebugAccessCommand_t);

  /*If this is write request, copy the data to be written */
  if(MB_DEBUG_ACCESS_WRITE == memOp->t_DebugAccessCommand.uc_command)
  {
      ptr += sizeof(DebugAccessCommand_t);
      if(0 != COPY_FROM_USER(ptr, (void*)(size_t)(memOp->data), 
			     (memOp->t_DebugAccessCommand.uc_dataType)))
	  {
#ifndef _WINDOWS
	      HN_DEBUG("%s: Cannot copy data from user space.\n", __func__);
#endif
	      status = -EFAULT;
	      goto exit;
	  }

      len += memOp->t_DebugAccessCommand.uc_dataType;
  }

  /* send the message */

#ifndef _WINDOWS 
  status = SendMsgToMailBox(0, 
			    MB_MSG_DEV_LOCAL,
			    MB_MSG_DEBUG_ACCESS,
			    &msg,
			    len,
			    1,
			    &reply);
#else
  
  {
      int temp_len = 0;
      LocalDeviceDebugAccess_t *reply = NULL;
      status = Win32SendMsg((void*)(msg.payload), len, (void**)&reply, &temp_len,
                  MB_MSG_DEV_LOCAL, MB_MSG_DEBUG_ACCESS);
      
      if ((status == HN_SUCCESS) && (reply != NULL) && (temp_len - sizeof(DebugAccessCommand_t) >= 0))
      {
          memcpy((void*)memOp->data, (void*)(&reply->data), temp_len - sizeof(DebugAccessCommand_t));
      }
  }
  return(status);

  // _WINDOWS code terminates here
#endif 
  
  /* process response */
  if(HN_SUCCESS == status)
    {
      if(0 == reply)
	{
	  status = -ENOMSG;
	  goto exit;
	}
      
      rPayload = (LocalDeviceDebugAccess_t*)(reply->payload); 

      if((MB_DEBUG_ACCESS_READ == memOp->t_DebugAccessCommand.uc_command) ||
	 (MB_DEBUG_ACCESS_WRITE == memOp->t_DebugAccessCommand.uc_command))
	{
	  /* copy reply to the user */
	  if((rPayload->t_DebugAccessCommand.uc_command != memOp->t_DebugAccessCommand.uc_command) ||
	     (rPayload->t_DebugAccessCommand.uc_dataType != memOp->t_DebugAccessCommand.uc_dataType) || 
	     (rPayload->t_DebugAccessCommand.us_count != memOp->t_DebugAccessCommand.us_count))
	    {
	      status = -EFAULT;
	    }
	  else
	    {
	      hdr = (MBMsgHdr_t*)reply;
	      len = MSG_LEN(hdr);
	      len -= sizeof(MBMsgHdr_t);
	      len -= sizeof(sizeof(DebugAccessCommand_t));

	      if(0 < len)
		{
		  if(0 != COPY_TO_USER((void*)(size_t)(memOp->data), 
				       (void*)(size_t)&(rPayload->data),
				       len))
		    {
#ifndef _WINDOWS
		      HN_DEBUG("%s: Cannot copy message to user space.\n", __func__);
#endif
		      
		      status = -EFAULT;
		    }
		}
	    }
	}
      else
	{
#ifndef _WINDOWS
	  HN_DEBUG("%s: Invalid header in reply.\n", __func__);
#endif 
	  status = -EFAULT;
	}
#ifndef _WINDOWS
      SetMBBufState(debug_mb, inMsgS, MB_EMPTY);
      kfree(reply);
#endif
    }

 exit:
  return status;
}

#ifdef _WINDOWS
int HN_SendResetMsg(unsigned long arg)
{
  int status = HN_SUCCESS;
  HNDrvIoctlCmd_t *op;
  MBMsg_t  msg;
  int dataLen;
  HNMsg_t* hnMsg;

  op = (HNDrvIoctlCmd_t *)arg;
  /* enclose HN message in the MB message payload */
  hnMsg = (HNMsg_t*)msg.payload;
  dataLen = HN_BuildRequestMsg(hnMsg, HN_MSG_TYPE_RESET, op);
  
  if(0 > dataLen)
    {
      //Error:
      status = -EFAULT;
    }
  else
    {
      int temp_len = 0;
      HNMsg_t *reply = NULL;
      /* call DLL send message */
      status = Win32SendMsg((void*)(msg.payload), dataLen, (void**)&reply, &temp_len,
                  MB_MSG_DEV_DRV, MB_MSG_DEBUG_RESET);
    }
  return status;
}
#endif

int HNIOCTL_SetHW(HNDrvIoctlCmd_t* op)
{
  int status = HN_SUCCESS;
  MBMsg_t  msg;
  MBMsg_t* reply = 0;
  int dataLen;
  HNMsg_t* hnMsg;

#ifndef _WINDOWS 
  unsigned int block; 
  MBMsgHdr_t* hdr;
#endif

  /* enclose HN message in the MB message payload */
  hnMsg = (HNMsg_t*)msg.payload;
  dataLen = HN_BuildRequestMsg(hnMsg, HN_MSG_TYPE_CFG, op);
  
  if(0 > dataLen)
    {
      //Error:
      status = -EFAULT;
    }
  else
    {
#ifndef _WINDOWS 
      /* Send the message and wait for reply */
      block = 1;
      status = SendMsgToMailBox(cfg_mb,
				MB_MSG_DEV_HN,
				MB_MSG_TYPE_REQUEST,
				&msg, 
				dataLen, 
				block, 
				&reply);      
      if(HN_SUCCESS == status)
	{
	  /* we received a reply, return reply content to user */
	  if(0 != reply)
	    {
	      hdr = (MBMsgHdr_t*)(reply);
	      hnMsg = (HNMsg_t*)(reply->payload);
	      
	      /* Make sure the reply is for me */
	      status = HN_ValidateRequestReply(hdr, hnMsg,
					       HN_MSG_TYPE_CFG);
					       
	      if((HN_SUCCESS == status) && 
		 (hnMsg->cmd == op->subCmd))
		{
		  //Check if the firmware report error
		  if(0 != MSG_NACKBIT(hdr))
		    {
		      status = hnMsg->data[1];
		    }
		}
              kfree(reply);
	    }
	  else
	    {
	      status = -ENOMSG;
	    }

	 SetMBBufState(cfg_mb, inMsgS, MB_EMPTY); 
	}
  
#else
      /* call DLL send message */
  //    return Win32SendMsg((void*)(msg.payload), dataLen, (void*)op->param, &(op->length),
  //			  MB_MSG_DEV_HN, MB_MSG_TYPE_REQUEST);

  {
      int temp_len = 0;
      HNMsg_t *reply = NULL;
      status = Win32SendMsg((void*)(msg.payload), dataLen, (void**)&reply, &temp_len,
                  MB_MSG_DEV_HN, MB_MSG_TYPE_REQUEST);
      
      if ((status == HN_SUCCESS) && (reply != NULL) && ((temp_len - HN_MSG_HDR_LEN) >= 0))
      {
          op->length = temp_len - HN_MSG_HDR_LEN;
          memcpy((void*)op->param, (void*)reply->data, temp_len - HN_MSG_HDR_LEN);
      }
      else
      {
          // over-ride status
          status = -1;
      }
  }
  return(status);

  // _WINDOWS code terminates here
#endif 
    }
  return status;
}

int HNIOCTL_GetHW(HNDrvIoctlCmd_t* op)
{
  int status = HN_SUCCESS;
  MBMsg_t  msg;
  MBMsg_t* reply = 0;
  HNMsg_t* hnMsg;
  int dataLen;

#ifndef _WINDOWS 
  unsigned int block; 
  MBMsgHdr_t* hdr;
#endif

  /* enclose HN message in the MB message payload */
  hnMsg = (HNMsg_t*)msg.payload;
  dataLen = HN_BuildRequestMsg(hnMsg, HN_MSG_TYPE_CFG, op);
  
  if(0 > dataLen)
    {
      //Error:
      status = -EFAULT;
    }
  else
    {
      /* Send the message and wait for reply */
#ifndef _WINDOWS 
      block = 1;
      status = SendMsgToMailBox(cfg_mb,
				MB_MSG_DEV_HN,
				MB_MSG_TYPE_REQUEST,
				&msg, 
				dataLen, 
				block, 
				&reply);      
      
      if(HN_SUCCESS == status)
	{
	  /* we received a reply, return reply content to user */
	  if(0 != reply)
	    {
	      hdr = (MBMsgHdr_t*)(reply);
	      hnMsg = (HNMsg_t*)(reply->payload);
	      
	      /* Make sure the reply is for me */
	      status = HN_ValidateRequestReply(hdr, hnMsg, 
					       HN_MSG_TYPE_CFG);
				
		  if((HN_SUCCESS == status) &&
		   (hnMsg->cmd == op->subCmd))
		  {
		  //Check if the firmware report error
		    if(0 != MSG_NACKBIT(hdr))
		    {
		      status = hnMsg->data[1];
		    }
		    else
		    {
		      op->length = (MSG_LEN(hdr) - sizeof(MBMsgHdr_t) - HN_MSG_HDR_LEN);

		      if(0 != op->length)
			  {
			    if(0 != COPY_TO_USER((void*)(size_t)(op->param), 
					       (void*)(hnMsg->data), 
					       op->length))
			    {
			      HN_DEBUG("%s: copy parameters to user failed\n", __func__);
			      status = -EFAULT;
			    }
			  }
		    }
		  }
		  else
		  {
		    status = -ENOMSG;
		  }

            kfree(reply);
#if 0 // Reply has to be freed for all messages, but not only for segmented message
          if ((hdr->length & LENGTH_SEGMENTATION_MASK) == MB_SEGMENTED_PAYLOAD)
          {
            // have to free the msg buffer kmalloc'ed by kernel
            kfree(reply);
          }
#endif
	    }
	  
	  SetMBBufState(cfg_mb, inMsgS, MB_EMPTY);
	}
#else
    // within Win32SendMsg, the HN_MSG_HDR will be taken off from the replied payload
    //  return Win32SendMsg((void*)(msg.payload), dataLen, (void*)op->param, &(op->length),
    //			  MB_MSG_DEV_HN, MB_MSG_TYPE_REQUEST);

    {
        int temp_len = 0;
        HNMsg_t *reply = NULL;
        status = Win32SendMsg((void*)(msg.payload), dataLen, (void**)&reply, &temp_len,
                    MB_MSG_DEV_HN, MB_MSG_TYPE_REQUEST);
        
        if ((status == HN_SUCCESS) && (reply != NULL) && ((temp_len - HN_MSG_HDR_LEN) >= 0))
        {
            op->length = temp_len - HN_MSG_HDR_LEN;
            memcpy((void*)op->param, (void*)reply->data, temp_len - HN_MSG_HDR_LEN);
        }
        else
        {
            // over-ride status
            status = -1;
        }
    }
  return(status);
#endif 

    }

  return status;
}

int HN_BuildRequestMsg(HNMsg_t* hnMsg,
		       HNMsgTypeE hnMsgType,
		       HNDrvIoctlCmd_t* op)
{
  int len = 0;
  
  if (sizeof(HNMsgTypeE) == 2)
  {
    hnMsg->type = HTONS(hnMsgType);
  }
  else if (sizeof(HNMsgTypeE) == 4)
  {
    hnMsg->type = HTONL(hnMsgType);
  }
  else
  {
    hnMsg->type = hnMsgType;
  }

  len += sizeof(HNMsgTypeE);

  //printf("type = 0x%x, len = 0x%x\n", hnMsgType, len);
  hnMsg->cmd = HTONL(op->subCmd);

  len += sizeof(HNIoctlSubCmdE);
  //printf("subCmd = 0x%x, len = 0x%x, op_length = 0x%x\n", op->subCmd, len, op->length);
  
  if(0 != op->length)
    {
      if(0 != COPY_FROM_USER((void*)&(hnMsg->data), 
			     (void*)(size_t)(op->param), 
			     op->length))
	{
#ifndef _WINDOWS 
	  HN_DEBUG("%s: Cannot copy data from user space.\n", __func__);
#endif 
	  len = -1;
	}
      else
	{
	  len += op->length;
	}
    }
  return len;
}

