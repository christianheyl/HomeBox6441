/******************************************************************************

                               Copyright (c) 2010
                            Lantiq Broadband North America
                     40 Middlesex Turnpike; Bedford MA 01730, USA

**
** FILE NAME    : hn_mbMod.c
** PROJECT      : HN
** MODULES      : MB module
**
** DATE         : 26 Jul 2010
** AUTHOR       : Leo Gray
** DESCRIPTION  : HN mail box module source file
**
** HISTORY
** $Date        $Author         $Comment
** 26 JUL 2010  Leo Gray        Init Version
** 29 SEPT 2010 Rick Han        Working version
******************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <asm/uaccess.h>

#ifndef MB_SIMULATION
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#else
#include <asm/io.h>
#endif 

#include "ltq_hn_mbDefines.h"
#include "ltq_hn_mbMod.h"
#include "ltq_hn_hnDrv.h"

#define MB_POLLING 1

#ifndef MB_SIMULATION
#define VPE0_to_VPE1_MB0_IM4_IRL14  14
#endif

#if (MB_POLLING)
#define MB_POLL_THREAD_NAME "mbPollingThread"
static struct task_struct* pollThread = NULL;
static int mb_poll_thread(void* arg);
static void StartMBPolling(int mb);
//static void StopMBPolling(int mb);
#else
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#define MB_ISR_WQ_NAME "mbISRwq"
static struct workqueue_struct *irq_wq = NULL;
static mb_irq_work_t irq_work[MB_NUM]; 
static int mb_irq = INT_NUM_IM0_IRL0;
static irqreturn_t  mb_irq_handler(int, void* regs);
static void wq_func(struct work_struct *work);
#endif 
/*
 * ####################################
 *              Version No.
 * ####################################
 */

#define MB_VER_MINOR           0

/*
 * ####################################
 *              Definition
 * ####################################
 */

#define MB_DEVICE_NAME             "mb0"
/*
 * ####################################
 *             Declaration
 * ####################################
 */
static int mb_major = 0;        /* Major Device Number */
static int mb_minor = MB_VER_MINOR;

static MBDrv_t *mbModule = NULL;
const MBMsgStreamE inMsgS = MB_STREAM_VPE1_TO_VPE0;
const MBMsgStreamE outMsgS = MB_STREAM_VPE0_TO_VPE1;

static MBPairDesc_t* mbox[MB_NUM];
static MBox_t *mailbox[MB_NUM];
static MBDescTbl_t  *mbDescTbl;
static char* mbBufP = NULL;
static unsigned int msgSeqNo = 0;

//debug_info is only needed for debugging, not normal operation
int mb_debug_info = 0;

#define MB_INFO(format...)			\
  do { printk(KERN_INFO format); } while ( 0 )

#define MB_DEBUG(format...)			\
  do { printk(KERN_DEBUG format); } while ( 0 )

#define MB_DEBUG_INFO(format...)			\
  if(mb_debug_info) printk(KERN_DEBUG format);

static MBClient_t* mbClients[MB_MAX_CLIENT_NUM];

static mbParams_t mbParams[MB_NUM] = {
  {MB_HI_PRIOR, (MB_MSG_PAYLOAD_MAX_SIZE + sizeof(MBMsgHdr_t))},  
  {MB_HI_PRIOR, ((MB_MSG_PAYLOAD_MAX_SIZE+7)/8 + sizeof(MBMsgHdr_t))},
  //{MB_MI_PRIOR, (MB_MSG_PAYLOAD_MAX_SIZE + sizeof(MBMsgHdr_t))},
  //{MB_MI_PRIOR, ((MB_MSG_PAYLOAD_MAX_SIZE+7)/8 + sizeof(MBMsgHdr_t))},
  {MB_LO_PRIOR, (MB_MSG_PAYLOAD_MAX_SIZE + sizeof(MBMsgHdr_t))},
  {MB_LO_PRIOR, ((MB_MSG_PAYLOAD_MAX_SIZE+7)/8 + sizeof(MBMsgHdr_t))}
};

/************************************************************
 * Proc files 
 ************************************************************/
#define MB_PROCFS_NAME          "mbDrv"
#define PROC_FS_MAX_SZ          1024
static struct proc_dir_entry *mbProcfsPtr = NULL;
static struct proc_dir_entry *mb1ProcfsPtr = NULL;

static int MB_RegisterProcfs(void);
static void MB_DeRegisterProcfs(void);

static int DumpMailboxBuffer(char* buffer, int bufLen, int mb);
static int MB_ProcRead(char *buffer, 
		       char **buffer_location,
		       off_t offset, int buffer_length, int *eof, 
		       void *data);
static int MB_ProcRead_MB0(char *buffer, 
			   char **buffer_location,
			   off_t offset, int buffer_length, int *eof, 
			   void *data);
static int MB_ProcRead_MB1(char *buffer, 
			   char **buffer_location,
			   off_t offset, int buffer_length, int *eof, 
			   void *data);
static int MB_ProcRead_MB2(char *buffer, 
			   char **buffer_location,
			   off_t offset, int buffer_length, int *eof, 
			   void *data);
static int MB_ProcRead_MB3(char *buffer, 
			   char **buffer_location,
			   off_t offset, int buffer_length, int *eof, 
			   void *data);

static mbProcFs_t mbProcFs[MB_NUM] = {
  {"mb0", MB_ProcRead_MB0},  
  {"mb1", MB_ProcRead_MB1},
  {"mb2", MB_ProcRead_MB2},
  {"mb3", MB_ProcRead_MB3}
};			  

static MBMsgStreamE GetMBBufState(unsigned int num, 
				  MBMsgStreamE stream);

void SetMBBufState(unsigned int num, 
		   MBMsgStreamE stream,
		   MBStreamStateE state);

/************************************************************
 * static functions 
 ************************************************************/
static int InitMBDescTable(void* mbTblAddr, void* mbBufAddr);
static int mbInit(void);
static int mb_open(struct inode *mb_node, struct file *mb_file);
static int mb_close(struct inode *hn_node, struct file *hn_file);
static int mb_ioctl(struct inode *inode, struct file *file,
		    unsigned int cmd, unsigned long arg);
static void ProcMsg(int mbNum, MBClient_t* client);
static MBClient_t* FindMsgClient(MBMsgHdr_t* hdr);


static int MailBoxNumberIsValid(unsigned int num);
static int SendMBMsg(unsigned int mbNum, MBMsgStreamE stream, MBMsg_t* mP);
static int CopyMsgHdr(unsigned int mbNum, MBMsgStreamE stream, MBMsgHdr_t* hdrP);
static unsigned char* GetMBMsgBuf(unsigned int mbNum, MBMsgStreamE stream);
static void InitMsgHeader(MBMsgHdr_t* hdr, unsigned char devId,
			  unsigned char type, 
			  unsigned int len);


void SendErrReply(int mbNum, MB_ERROR_CODES err);
int SendMsgToMailBox(int mbNum,
		     unsigned int id,
		     unsigned int type, 
		     MBMsg_t* msg, 
		     int msgLen, 
		     unsigned char blocking, 
		     MBMsg_t** reply);

#ifdef MB_SIMULATION
int mb_mmap(struct file *fileP, struct vm_area_struct* vma);
#endif 

static struct file_operations mb_fops = {
  .open    = mb_open,
  .release = mb_close,
  .ioctl   = mb_ioctl,

#ifdef MB_SIMULATION
  .mmap = mb_mmap,
#endif 
};

void* GetMBBuffer(int mb, MBMsgStreamE s);

#ifdef MB_SIMULATION
MBDescTbl_t *simDescTbl;
unsigned char* simBuf = NULL;
unsigned char* bufArea = NULL;
#endif 


/************************************************************
 * MailBoxNumberIsValid: helper function to validate mailbox number
 ************************************************************/

int MailBoxNumberIsValid(unsigned int num)
{
  return (num < MB_NUM);
}

int MailBoxStreamIsValid(MBMsgStreamE stream)
{
  return ((MB_STREAM_VPE0_TO_VPE1 <= stream) &&
	  (MB_STREAM_NUM > stream));
}

/************************************************************
 * SendMBMesg: send a message with raw data  
 * INPUT: 
 *   - mbNum:  mail box number 
 *   - stream: stream (MB_STREAM_VPE0_TO_VPE1 or MB_STREAM_VPE1_TO_VPE0) 
 *   - mP:     pointer a message buffer
 * 
 * OUTPUT: none 
 * RETURN: MB_SUCCESS/MB_FAILURE
 ************************************************************/
int SendMBMsg(unsigned int mbNum,
	      MBMsgStreamE stream,
	      MBMsg_t* mP)
{
  unsigned char* dataP;
  MBStream_t* mbS;
  int state = MB_FAILURE;

  if(!MailBoxNumberIsValid(mbNum)) 
    {
      return MB_FAILURE;
    }

  if(MB_STREAM_VPE0_TO_VPE1 == stream)
    {
      mbS = &(mbox[mbNum]->vpe0ToVpe1);
    }
  else 
    {
      mbS = &(mbox[mbNum]->vpe1ToVpe0);
    }

  if((!mbS) || (MB_EMPTY != mbS->mbBufState))
    {
      if(MB_EMPTY != mbS->mbBufState)
	{
	  printk(KERN_INFO "buffer state: 0x%08x\n", mbS->mbBufState);
	}
      goto exit;
    }
  dataP = mbS->mbBuf;
  
  if(!dataP)
    {
      goto exit;
    }

  memcpy(dataP, (void*)mP, (mP->hdr.length));
  
  /* update mail box status */
  mbS->mbBufState = MB_HAS_DATA;
  state = MB_SUCCESS;

#if !(MB_POLLING)
  // send intr to VPE1
  *IFX_ICU_VPE1_IM4_IRSR = 1 << (VPE0_to_VPE1_MB0_IM4_IRL14 + mbNum);
#endif

  state = MB_SUCCESS;

 exit:
  return state;
}

/************************************************************
 * CopyMsgHdr: Get message header information  
 * 
 * INPUT: 
 *   - mbNum:  mail box number 
 *   - stream: stream (MB_STREAM_VPE0_TO_VPE1 or MB_STREAM_VPE1_TO_VPE0)
 *  
 * OUTPUT:
 *   - hdrP:      message header 
 * RETURN:  
 *   MB_SUCCESS/MB_FAILURE
 * 
 ************************************************************/ 
int CopyMsgHdr(unsigned int mbNum,
	       MBMsgStreamE stream,
	       MBMsgHdr_t* hdrP)
{
  MBMsgHdr_t* msg;
  MBStream_t* mbS;
  int state = MB_FAILURE;

  /* validate arguments */
  if(!MailBoxNumberIsValid(mbNum))
    
    {
      goto exit;
    }

  if(MB_STREAM_VPE0_TO_VPE1 == stream)
    {
      mbS = &(mbox[mbNum]->vpe0ToVpe1);
    }
  else 
    {
      mbS = &(mbox[mbNum]->vpe1ToVpe0);
    }

  if((!mbS) || (MB_HAS_DATA != mbS->mbBufState))
    {
      goto exit;

    }

  msg = (MBMsgHdr_t*)(mbS->mbBuf);
  if(!msg)
    {
      goto exit;
    }

  /* Get the header fields */
  memcpy((unsigned char*)hdrP, (unsigned char*)msg, sizeof(MBMsgHdr_t));
  state =  MB_SUCCESS;

 exit:
  return state;
}

/************************************************************
 * GetMBMsgBuf: Copy message data from mail box to a given buffer  
 * 
 * INPUT: 
 *   - mbNum:  mail box number 
 *   - stream: stream (MB_STREAM_VPE0_TO_VPE1 or MB_STREAM_VPE1_TO_VPE0)
 *  
 * OUTPUT:
 *   - bufP:      pointer to a buffer where the data is copied to 
 * RETURN:  
 *   MB_SUCCESS/MB_FAILURE
 * 
 ************************************************************/ 
unsigned char* GetMBMsgBuf(unsigned int mbNum,
			   MBMsgStreamE stream)
{
  MBStream_t* mbS;
  unsigned char* bp = NULL;

  /* validate arguments */
  if(!MailBoxNumberIsValid(mbNum))
    
    {
      goto exit;
    }

  if(MB_STREAM_VPE0_TO_VPE1 == stream)
    {
      mbS = &(mbox[mbNum]->vpe0ToVpe1);
    }
  else 
    {
      mbS = &(mbox[mbNum]->vpe1ToVpe0);
    }

  if(!mbS) 
    {
      goto exit;
    }

  bp = (mbS->mbBuf);
  
 exit:
  return bp;
}

int InitMBDescTable(void* mbTblAddr, void* mbBufAddr)
{
  int mbNum;
  void* bufP = mbBufAddr;
  MBDescTbl_t *mbTblP = (MBDescTbl_t *)mbTblAddr;


  MB_INFO("%s: mbDescTbl: 0x%p  mb buffer starts: 0x%p\n",
	  __func__, mbTblAddr, mbBufAddr);
  
  mbTblP->mbNumOfDesc = MB_NUM;
  
  for(mbNum = 0; mbNum<MB_NUM; mbNum++)
    {
      mbTblP->mbPair[mbNum].priority = mbParams[mbNum].p;
      mbTblP->mbPair[mbNum].size = mbParams[mbNum].s;
      
      mbTblP->mbPair[mbNum].vpe0ToVpe1.mbBufState = MB_EMPTY;
      mbTblP->mbPair[mbNum].vpe0ToVpe1.mbBuf = bufP;
      mbTblP->mbPair[mbNum].vpe1ToVpe0.mbBufState = MB_EMPTY;
      mbTblP->mbPair[mbNum].vpe1ToVpe0.mbBuf = bufP + mbParams[mbNum].s;
            
      /* move the buffer pointer */
      bufP += mbParams[mbNum].s*2;
      
      mbox[mbNum] = &(mbTblP->mbPair[mbNum]);
    }
  
  if(mbNum != (mbTblP)->mbNumOfDesc)
    {
      (mbTblP)->mbVersion = -1;
      return MB_FAILURE;
    }
  else
    {
      /* declare that the MB is ready to use */
      (mbTblP)->mbVersion = (MB_MAGIC + MB_VERSION);
      return MB_SUCCESS;
    }
}

int mbInit(void)
{
  int mb;
  int cn;

#ifdef MB_SIMULATION 
  unsigned long vAddr;
  unsigned int bufSize = 4*MB_BUF_SIZE+2*PAGE_SIZE+sizeof(MBDescTbl_t);
  bufArea = (unsigned char*)kmalloc(bufSize, GFP_KERNEL);
  simDescTbl = (MBDescTbl_t*)(((unsigned long)bufArea + PAGE_SIZE -1) & PAGE_MASK);
  simBuf = (unsigned char*)((unsigned long)simDescTbl + sizeof(MBDescTbl_t));

   
  for (vAddr = (unsigned long)simDescTbl;
       vAddr < (unsigned long)bufArea + bufSize;
       vAddr+=PAGE_SIZE)
    {
      SetPageReserved(virt_to_page(vAddr));
    }

  MB_INFO("%s entering, running in simulation mode. \n", __func__);
#else
  MB_INFO("%s entering, running in operation mode. \n", __func__);
#endif 

  /* Initialize local data */
  if(NULL == mbModule)
    {
      return -ENODEV;
    }

  /* Initialize local variables */
  mbModule->openCnt = 0;
  mbModule->clientCnt = 0;
  init_MUTEX(&(mbModule->procSem));

  for (cn=0; cn<MB_MAX_CLIENT_NUM; cn++)
    {
      mbClients[cn] = NULL;
    }

  for(mb = 0; mb < MB_NUM; mb++)
    {
      mailbox[mb] = kmalloc(sizeof(MBox_t), GFP_KERNEL);
      if(NULL == mailbox[mb])
	{
	  MB_DEBUG("Cannot alloc mailbox\n");
	  return MB_FAILURE;
	}
      else
	{
	  /* Initialize the mail box */
	  mailbox[mb]->pollEn = MB_POLL_DISABLE;
	  init_MUTEX(&(mailbox[mb]->mbSem));
	  init_waitqueue_head(&(mailbox[mb]->wq));

	  mailbox[mb]->rxMsgCnt = 0;
	  mailbox[mb]->rxDropCnt = 0;
	  mailbox[mb]->rxErrCnt = 0;
	  mailbox[mb]->rxTimeOutCnt = 0;
	  mailbox[mb]->txMsgCnt = 0;
	  mailbox[mb]->txErrCnt = 0;
	}
    }

  /* Initialize mailbox descriptor table */ 
#ifdef MB_SIMULATION
  mbDescTbl = simDescTbl;
  mbBufP = simBuf;
#else
  MB_INFO("MB descriptor table physical addr: 0x%08x, buffer addr: 0x%08x\n",
	  MB_REGION_START, MB_BUF);
  
  mbDescTbl = (MBDescTbl_t*)(MB_REGION_START);
  mbBufP = (unsigned char*)(MB_BUF);
#endif 

  /* set up high priority mailboxes */
  if(MB_SUCCESS != InitMBDescTable(mbDescTbl, mbBufP))
    {
      MB_DEBUG("Cannot init MB descriptor table\n");
      return MB_FAILURE;
    }

#if (MB_POLLING) 
  /*Create a thread poll mailbox */
  pollThread = kthread_create(mb_poll_thread, 0, "MB_Poll_Thread");
  if(NULL != pollThread)
    {
      wake_up_process(pollThread);
    }
  else 
    {
      MB_DEBUG("Cannot create MB polling thread\n");
      return -EBUSY;
    }

  /* we are all set, now start polling */
  for(mb = 0; mb < MB_NUM; mb++)
    {
      StartMBPolling(mb);
    }
#else

  /* Init workqueue */
  irq_wq = create_singlethread_workqueue(MB_ISR_WQ_NAME);
  if(NULL == irq_wq)
    {
      MB_DEBUG("Cannot create MB interrupt handling workqueue\n");
      return -EBUSY;
    }

  /* register IRQs */
  for(mb = 0; mb < MB_NUM; mb++)
    {
      if(0 != request_irq(mb_irq + mb, mb_irq_handler, 
			  SA_SHIRQ, MB_DEVICE_NAME, mbModule))
	{
	  MB_DEBUG("Cannot register MB_%d interrupt IRQ\n", mb);
	  return -EBUSY;
	}
      else
	{
	  MB_DEBUG("register interrupt: irq = %d, mb = %d\n", mb_irq + mb, mb);
	}
    }
#endif   
  
  return 0;
}

/**
 * mb_open
 **/
int mb_open(struct inode *mb_node, struct file *mb_file)
{ 
  if(NULL != mbModule)
    {
      /* Update usage count */
      mbModule->openCnt ++;
      mb_file->private_data = (void*)mbModule;

      // MB_DEBUG("Number of times opened %d\n", mbModule->openCnt);
      return MB_SUCCESS;
    }
  else 
    {
      MB_DEBUG("MB module is not initialized before open\n");     
      return -ENODEV;
    }
}

/**
 * mb_close
 *
 **/
int mb_close(struct inode *hn_node, struct file *hn_file)
{
  if(NULL != mbModule)
    {
      /* Update usage count */
      mbModule->openCnt --;
    }
  
  return 0;
}

/**
 * mb_ioctl
 *
 **/
int mb_ioctl(struct inode *inode, struct file *file,
	     unsigned int cmd, unsigned long arg)
{
  int status = MB_SUCCESS;
  MBDrv_t *mb = (MBDrv_t *)file->private_data;
  MBMsgOpr_t msgOp;
  MBMsg_t msg;
  MBMsg_t* reply = NULL;
  unsigned int replyLen = 0;
  MBMsgHdr_t *hdr = 0;

  if((NULL == mb) || (mb != mbModule))
   {
      MB_DEBUG("%s: Cannot locate mb module %p\n", 
	       __func__, mb);
      return -ENODEV;
    }
  
  if (_IOC_TYPE(cmd) != MB_IOC_MAGIC)
    {
      MB_DEBUG("%s: Invalid command type\n", __func__);
      return -EINVAL;
    }
  
  /* Now process the command */
  switch(cmd)
    {
    case MB_IOCTL_SEND_MSG:
      memset((void*)&msgOp, 0, sizeof(MBMsgOpr_t));
      if(0 != copy_from_user((void*)&msgOp, (void*)arg, 
			     sizeof(MBMsgOpr_t)))
	{
	  MB_DEBUG("%s(2): Cannot copy data from user space.\n", __func__);
	  status = -EFAULT;
	}
      else if ((0 != msgOp.msg) &&
	       (MB_MSG_PAYLOAD_MAX_SIZE >= msgOp.msgLen))
	{
	  /* copy message from user space */
	  if(0 != copy_from_user((void*)(size_t)&(msg.payload),
				 (void*)(size_t)msgOp.msg,
				 msgOp.msgLen))
	    {
	      MB_DEBUG("%s (1): Cannot copy message from user space.\n", __func__);
	      status = -EFAULT;
	    }
	  else
	    {
	      
	      
	      /* send the message */

	      status = SendMsgToMailBox(msgOp.mb, 
					msgOp.devId,
					msgOp.type,
					&msg,
					msgOp.msgLen,
					msgOp.blocking,
					&reply);
	    if(MB_SUCCESS == status)
        {
	      if (NULL != reply)
		  {
		  hdr = (MBMsgHdr_t*)reply;

		  /* get payload length */
		  replyLen = MSG_LEN(hdr) - sizeof(MBMsgHdr_t);
          //printk(KERN_INFO "after SendMsgToMailBox - replyLen = %d\n", replyLen);
		  if(0 != replyLen)
		    {
		      /* copy reply to user */
#if 0
		      if(0 != COPY_TO_USER((void*)(size_t)(msgOp.reply),
					   (void*)(size_t)(reply->payload),
					   replyLen))
            
			{
			  MB_DEBUG("%s (1): Cannot copy message to user space. bytes left = %d, \n", __func__, junkx);
			  msgOp.replyLen = 0;
			  status = -EFAULT;
			}
		      else
			{
			  //successfully copied payload to user space */
			  msgOp.replyLen = replyLen;
			}
#else
            // due to transfer size in the "device", we need to break down the msg copying
            int temp_len, cur_len;
            int loop_cnt;
            void *usr_buf;
            void *msg_buf;
            int bytes_left;


            temp_len = replyLen;
            usr_buf = (void*)msgOp.reply;
            msg_buf = (void*)reply->payload;
            loop_cnt = 0;
            replyLen = 0;
            while (temp_len > 0)
            {
                cur_len = temp_len;
                if (temp_len > 1024)
                {
                    cur_len = 1024;
                }
                temp_len -= cur_len;
		        if ( (bytes_left = copy_to_user(usr_buf, msg_buf, cur_len)) != 0)
                {
                    MB_DEBUG("ERROR: %s (idx = %d): Cannot copy message to user space. bytes left = %d, \n", __func__, loop_cnt, bytes_left);
			        status = -EFAULT;
                    break;
                }
                else
                {
                    //MB_DEBUG("%s (idx = %d): usr_buf = 0x%p, msg_buf = 0x%p \n", __func__, loop_cnt, usr_buf, msg_buf);
                    replyLen += cur_len;
                }
                usr_buf += cur_len;
                msg_buf += cur_len;
                loop_cnt++;
            }

#endif
			  msgOp.replyLen = replyLen;

		      if(0 != copy_to_user((void*)(size_t)arg,
					   (void*)(size_t)&msgOp,
					   sizeof(MBMsgOpr_t)))
			{
			  MB_DEBUG("%s (2): Cannot copy message to user space.\n", __func__);
			  status = -EFAULT;
			}
		    }
		  
//		  SetMBBufState(msgOp.mb, inMsgS, MB_EMPTY);
            kfree(reply);
		  }
        }
	    }
	}
      else
	{
	  MB_DEBUG("%s: invalid message information msg(0x%p) len(%x)\n",
		   __func__, msgOp.msg, msgOp.msgLen);
	  
	  status = -EINVAL;
	}
	  
      /* copy msg operation information to user */
	  
          
      break;
	
    default:
      status = -EINVAL;
      break;
    }
  
  return status;
}

void InitMsgHeader(MBMsgHdr_t* hdr, unsigned char devId, 
		   unsigned char type, unsigned int len)
{
  memset((void*)hdr, 0, sizeof(MBMsgHdr_t));
  hdr->type = ((devId<<MB_MSG_HDR_DEV_OFFSET) | (type));
  hdr->seqNo = msgSeqNo++;
  SET_MSG_LEN(hdr, (len + sizeof(MBMsgHdr_t)));
}
 
#ifdef MB_SIMULATION
/**
 * mb_mmap
 *
 **/ 
int mb_mmap(struct file *fileP, struct vm_area_struct* vma)
{    
  vma->vm_flags |= (VM_LOCKED | VM_RESERVED); 
  vma->vm_ops = NULL;
  
  if(remap_pfn_range(vma,
                     vma->vm_start, 
		     (virt_to_phys(simDescTbl))>>PAGE_SHIFT,
		     vma->vm_end - vma->vm_start,
		     vma->vm_page_prot))
    {
      MB_DEBUG("%s: Can not map adress to user space\n", __func__);
      return -EAGAIN;
    } 

  return 0;
}
#endif

void ProcMsg(int mbNum, MBClient_t *client)
{
  MBMsg_t* msg = (MBMsg_t*)GetMBMsgBuf(mbNum, inMsgS);

  /* invoke callback if there is one */
  if(NULL != msg)
    {
      if (0 != client->rxMsgHdlr(mbNum, (void*)msg))
	{
	  /* update the error counter and then clear the mailbox statue */
	  mailbox[mbNum]->rxErrCnt++;
	}
      else
	{
	  mailbox[mbNum]->rxMsgCnt++;
	}
    }
  else
    {
#if (MB_POLLING)
      MB_DEBUG("%s: cannot get the message\n", __func__);
#endif 
      SendErrReply(mbNum, MB_ERR_BAD_MSG);
      /* update the error counter and then clear the mailbox statue */
      mailbox[mbNum]->rxErrCnt++;

    }

  SetMBBufState(mbNum, inMsgS, MB_EMPTY);  
}

void SendErrReply(int mbNum, MB_ERROR_CODES err)
{
  MBMsg_t errMsg;

  if(MB_SUCCESS == CopyMsgHdr(mbNum, MB_STREAM_VPE1_TO_VPE0, 
			      (MBMsgHdr_t*)&errMsg))
    {
      errMsg.payload[0] = err; 
      SendMBMsg(mbNum, outMsgS, &errMsg);
    }
}

#if (MB_POLLING)
void StartMBPolling(int mb)
{
  if((MailBoxNumberIsValid(mb)) &&
     (NULL != mailbox[mb]))
    {
      mailbox[mb]->pollEn = MB_POLL_ENABLE;      
      MB_DEBUG("%s: mb-%d\n", __func__, mb);
    }
}

void StopMBPolling(int mb)
{
  if((MailBoxNumberIsValid(mb)) && 
     (NULL != mailbox[mb]))
    {
      mailbox[mb]->pollEn = MB_POLL_DISABLE;
      MB_DEBUG("%s: mb-%d\n", __func__, mb);
    }
}

int mb_poll_thread(void* arg)
{
  int mb;
  MBMsgHdr_t* hdr;
  MBClient_t* client;
  int test = 1;

  for (;;)
    {
      if (kthread_should_stop())
	{
	  /* parent sends a stop signal to me */
	  break;
	}
      
      //Poll mail box 	  
      for (mb = 0; mb < MB_NUM; mb++)
	{
	  if( 0 == test)
	    {
	      MB_INFO("%s: mb-%d poll_state %u buffer_state %u \n", 
		      __func__, mb, mailbox[mb]->pollEn, 
		      GetMBBufState(mb, inMsgS));
	      test = 1;
	    }

	  if((MB_POLL_ENABLE == mailbox[mb]->pollEn) &&
	     (MB_HAS_DATA == GetMBBufState(mb, inMsgS))) 
	    {
	      /* Get message header  */
	      hdr = (MBMsgHdr_t*)GetMBMsgBuf(mb, inMsgS);
	      if(hdr)
		{
		  /* if this is an ACK to my request, wake up the sender 
		     sitting in the waiting queue.
		  */
		  if( 0 != MSG_ACKBIT(hdr) )
		    {
		      mailbox[mb]->replyFlag = hdr->seqNo;
		      wake_up_interruptible(&(mailbox[mb]->wq));
		    }
		  else
		    {
		      /* This is a message initiated by VPE1 */
		      /* find the client for the message */
		      client = FindMsgClient(hdr);
		      
		      if(NULL != client)
			{
			  /* set the mailbox state to PROCESSED 
			   * so we will not try to process it again 
			   */
			  SetMBBufState(mb, inMsgS, MB_BEING_PROCESSED);
			  ProcMsg(mb, client);
			}
		      else
			{
			  /* can not find a client */
			  mailbox[mb]->rxDropCnt++;
			  SetMBBufState(mb, inMsgS, MB_EMPTY);
			}
		    }
		}
	      else
		{
		  /* can not get the msg header*/
		  mailbox[mb]->rxErrCnt++;
		  SetMBBufState(mb, inMsgS, MB_EMPTY);
		}
	    }
	}
    
      /* pause before next poll */
#ifdef MB_SIMULATION
      msleep_interruptible(100*MB_POLL_INTERVAL_MS);      
#else
      msleep_interruptible(MB_POLL_INTERVAL_MS);
#endif 
    }

  return 0;
}  
				      
#else
irqreturn_t mb_irq_handler(int irq, void* dev)
{
  int mb = irq - mb_irq;
 
  if(MailBoxNumberIsValid(mb) && (NULL != irq_wq))
    {
      /* create a work */
      INIT_WORK((struct work_struct*)(&irq_work[mb]), wq_func);
      queue_work(irq_wq, (struct work_struct*)(&irq_work[mb])); 
    }
  else
    {
      /* should disable the interrupt forever */
    }
  
  // clear intr source
  *IFX_ICU_IM0_ISR = (1 << mb); 
  return 0;
}

void wq_func(struct work_struct *work)
{
  if(NULL != work)
    {
      int mb;
      MBMsgHdr_t* hdr;
      MBClient_t* client;
      mb_irq_work_t* irq_work = (mb_irq_work_t*)work;

      mb = irq_work->mb;
      
      /* Get message header  */
      hdr = (MBMsgHdr_t*)GetMBMsgBuf(mb, inMsgS);
      if(hdr)
	{
	  /* if this is an ACK to my request, wake up the sender 
	     sitting in the waiting queue.
	  */
	  if( 0 != MSG_ACKBIT(hdr) )
	    {
	      mailbox[mb]->replyFlag = hdr->seqNo;
	      wake_up_interruptible(&(mailbox[mb]->wq));
	    }
	  else
	    {
	      /* This is a message initiated by VPE1 */
	      /* find the client for the message */
	      client = FindMsgClient(hdr);
	      
	      if(NULL != client)
		{
		  /* set the mailbox state to PROCESSED 
		   * so we will not try to process it again 
		   */
		  SetMBBufState(mb, inMsgS, MB_BEING_PROCESSED);
		  ProcMsg(mb, client);
		}
	      else
		{
		  /* can not find a client */
		  mailbox[mb]->rxDropCnt++;
		  SetMBBufState(mb, inMsgS, MB_EMPTY);
		}
	    }
	}
      else
	{
	  /* can not get the msg header*/
	  mailbox[mb]->rxErrCnt++;
	  SetMBBufState(mb, inMsgS, MB_EMPTY);
	}
    }
  return;
}

#endif 

/**
 *
 *  Initialize MB module
 *
 **/
int __init HN_MB_ModuleInit (void)
{
  dev_t dev;
  int retVal = MB_SUCCESS;

  MB_INFO("Initialize mailbox (MB) module\n");
  
  mbModule = kmalloc(sizeof(MBDrv_t), GFP_KERNEL);
  if ( NULL == mbModule) 
    {
      MB_DEBUG("Error: Cannot malloc mb module\n");
      return -ENOMEM;
    }

  if (mb_major ) 
    {
      dev = MKDEV(mb_major, mb_minor);
      retVal = register_chrdev_region(dev, 1, MB_DEVICE_NAME);
    } 
  else 
    {
      if(0 != alloc_chrdev_region(&dev, mb_minor, 1, MB_DEVICE_NAME))
	{
	  MB_DEBUG("mailbox (MB) alloc_chrdev_region failed\n");
	  retVal = -ENODEV;
	}
      else
	{
	  mb_major = MAJOR(dev);
	  cdev_init(&mbModule->cdev, &mb_fops);
	  mbModule->cdev.owner = THIS_MODULE;
	  	  
	  if(0 != cdev_add(&mbModule->cdev, dev, 1))
	    {
	      MB_DEBUG("mailbox (MB) cdev_add failed\n");
	      retVal = -ENODEV;
	    }
	}
    }
  
  MB_INFO("mb module major number %d.\n", mb_major);
  
  retVal = MB_RegisterProcfs();
  
  if(MB_SUCCESS == retVal)
    {
      retVal =  mbInit();
    }

  return retVal;
}

/**
 *
 *  exit MB module
 *
 **/
void __exit HN_MB_ModuleExit (void)
{
  int mb;
#ifdef MB_SIMULATION
  unsigned long vAddr;
  unsigned int bufSize = 4*MB_BUF_SIZE+2*PAGE_SIZE+sizeof(MBDescTbl_t);
#endif 

  MB_INFO("Exiting HN MB Version 1.0");

  /* Remove the cdev */
  cdev_del(&(mbModule->cdev));
  
  /* Release the major number */
  unregister_chrdev_region(MKDEV(mb_major, mb_minor), 1);

  /* Remove proc entry */
  MB_DeRegisterProcfs();

#if (MB_POLLING)
  /* stop polling thread */
  kthread_stop(pollThread);
#else
  free_irq(mb_irq, NULL);

  if(NULL != irq_wq)
    {
      flush_workqueue(irq_wq);
      destroy_workqueue(irq_wq);
    }
  
#endif 

  kfree(mbModule);
  mbModule = NULL;

  for(mb = 0; mb < MB_NUM; mb++)
    {
      if(NULL != mailbox[mb])
	{
	  kfree(mailbox[mb]);
	  mailbox[mb] = NULL;
	}
    }

#ifdef MB_SIMULATION
   for (vAddr = (unsigned long)simDescTbl; 
       vAddr < (unsigned long)bufArea + bufSize;
       vAddr+=PAGE_SIZE)
    {
      ClearPageReserved(virt_to_page(vAddr));
    }
  kfree(bufArea);
  simDescTbl = NULL;
  simBuf = NULL;
#endif 
    
  MB_INFO("HN MB module is removed\n");
}
 
int MB_RegisterProcfs()
{
  int status = MB_SUCCESS;
  int mbNum;

  /* create a proc_fs for the drive*/
  mbProcfsPtr = create_proc_read_entry(MB_PROCFS_NAME, 
				       0444, 
				       0,    /* /proc */
				       MB_ProcRead, NULL );
  
  MB_INFO("%s: /proc/%s created\n", __func__, MB_PROCFS_NAME);

  
  for(mbNum = 0; mbNum<MB_NUM; mbNum++)
    {
      mb1ProcfsPtr = create_proc_read_entry(mbProcFs[mbNum].procFsName, 
					    0444, 
					    0,    /* /proc */
					    mbProcFs[mbNum].procFsReadFunc, NULL );
      MB_INFO("%s: /proc/%s created\n", __func__, mbProcFs[mbNum].procFsName);
    }

  return status;
}

void MB_DeRegisterProcfs(void)
{
  int mbNum;
  if(NULL != mbProcfsPtr)
    {
      remove_proc_entry(MB_PROCFS_NAME, 0);  
    }

  for(mbNum = 0; mbNum<MB_NUM; mbNum++)
    {
      remove_proc_entry(mbProcFs[mbNum].procFsName, 0);
    }
}

int DumpMailboxBuffer(char *buffer, int bufLen, int mb)
{
  int len = 0;
  int i = 0;
  int dumpSize = (60*4); /*60*4 bytes */ 
  unsigned int *ptr = (unsigned int *)(mbox[mb]->vpe0ToVpe1.mbBuf);

  len += sprintf(buffer+len, "mailbox-%d VPE0 to VPE1 buffer\n", mb);
  len += sprintf(buffer+len, "buffer %p len: %u\n", ptr, mbParams[mb].s);

  if(dumpSize > mbParams[mb].s)
    {
      dumpSize = mbParams[mb].s;
    }

  for (i=0; i<dumpSize/4; i++)
    {
      if(0!=i && 0==i%4)
	{
	  len += sprintf(buffer+len, "\n");
	}
      len += sprintf(buffer+len, "0x%08x ", *(ptr++));

      if(len >= (bufLen-8))
	{
	  len += sprintf(buffer+len, "\n");
	  goto exit;
	}
    }
  
  len += sprintf(buffer+len, "\n");
  
  ptr = (unsigned int *)(mbox[mb]->vpe1ToVpe0.mbBuf);
  len += sprintf(buffer+len, "mailbox-%d VPE1 to VPE0 buffer\n", mb);
  len += sprintf(buffer+len, "buffer %p len: %u\n", ptr, mbParams[mb].s);
  for (i=0; i<dumpSize/4; i++)
    {
      if(len >= (bufLen-8))
	{
	  len += sprintf(buffer+len, "\n");
	  break;
	}

      if(0!=i && 0==i%4)
	{
	  len += sprintf(buffer+len, "\n");
	}
      len += sprintf(buffer+len, "0x%08x ", *(ptr++));

      if(len >= (bufLen-8))
	{
	  len += sprintf(buffer+len, "\n");
	  goto exit;
	}
    }
  
  len += sprintf(buffer+len, "\n");

 exit:
  return len;
}

int MB_ProcRead_MB0(char *buffer, 
		    char **buffer_location,
		    off_t offset, int buffer_length, int *eof, 
		    void *data)
{
  int len = 0;
  if(mbModule)
    {
      /* get semaphore */
      down(&(mbModule->procSem));
      len =  DumpMailboxBuffer(buffer, buffer_length, 0);
      up(&(mbModule->procSem));
    }

  return len;
}

int MB_ProcRead_MB1(char *buffer, 
		    char **buffer_location,
		    off_t offset, int buffer_length, int *eof, 
		    void *data)
{
  int len = 0;
  if(mbModule)
    {
      /* get semaphore */
      down(&(mbModule->procSem));
      len =  DumpMailboxBuffer(buffer, buffer_length, 1);
      up(&(mbModule->procSem));
    }

  return len;
}

int MB_ProcRead_MB2(char *buffer, 
		    char **buffer_location,
		    off_t offset, int buffer_length, int *eof, 
		    void *data)
{
  int len = 0;
  if(mbModule)
    {
      /* get semaphore */
      down(&(mbModule->procSem));
      len =  DumpMailboxBuffer(buffer, buffer_length, 2);
      up(&(mbModule->procSem));
    }

  return len;
}

int MB_ProcRead_MB3(char *buffer, 
		    char **buffer_location,
		    off_t offset, int buffer_length, int *eof, 
		    void *data)
{
  int len = 0;
  if(mbModule)
    {
      /* get semaphore */
      down(&(mbModule->procSem));
      len =  DumpMailboxBuffer(buffer, buffer_length, 3);
      up(&(mbModule->procSem));
    }

  return len;
}

int MB_ProcRead(char *buffer, 
		char **buffer_location,
		off_t offset, int buffer_length, int *eof, 
		void *data)
{
  int len = 0;
  int i;
  char prio[3];

  if(mbModule)
    {
      /* get semaphore */
      down(&(mbModule->procSem));
      len += sprintf(buffer+len, "MB module: \n");
      len += sprintf(buffer+len, "openCnt (%d) clientCnt (%d)\n", 
		     mbModule->openCnt, mbModule->clientCnt);
      len += sprintf(buffer+len, "MB descTbl: %p version 0x%08x\n", 
		     mbDescTbl, mbDescTbl->mbVersion);

      len += sprintf(buffer+len, "=== mailboxes ===\n");
      for(i = 0; i<MB_NUM; i++)
	{
	  len += sprintf(buffer+len, "mb-%d: OUT: (%p/%u)  IN: (%p/%u)\n",
			 i, mbox[i]->vpe0ToVpe1.mbBuf, mbox[i]->vpe0ToVpe1.mbBufState,
			 mbox[i]->vpe1ToVpe0.mbBuf, mbox[i]->vpe1ToVpe0.mbBufState);
	
	  switch(mbParams[i].p)
	    {
	    case MB_HI_PRIOR:
	      strcpy(prio, "hi");
	      break;

	    case MB_MI_PRIOR:
	      strcpy(prio, "mi");
	      break;

	    case MB_LO_PRIOR:
	      strcpy(prio, "lo");
	      break;
	      
	    default:
	      strcpy(prio, "--");
	      break;
	    }
	  
	  len += sprintf(buffer+len, "mb-%i priority: %s size: %u\n",
			 i, prio, mbParams[i].s);
	}

      len += sprintf(buffer+len, "=== mailbox clients ===\n");

      for(i=0; i<MB_MAX_CLIENT_NUM; i++)
	{
	  if(NULL != mbClients[i])
	    {
	      len += sprintf(buffer+len, "client-%i: devID %u\t",
			     i, mbClients[i]->devId);
	    }
	}
      len += sprintf(buffer+len, "\n\n");
  
      len += sprintf(buffer+len, "=== mailbox stats===\n");
      for(i=0; i<MB_NUM; i++)
	{
	  len += sprintf(buffer+len, "mailbox-%d:\n", i);
	  len += sprintf(buffer+len, "\trxMsg %u   rxDrop %u  rxErr %u rxTimeOut %u\n",
			 mailbox[i]->rxMsgCnt,
			 mailbox[i]->rxDropCnt,
			 mailbox[i]->rxErrCnt,
			 mailbox[i]->rxTimeOutCnt);
	  len += sprintf(buffer+len, "\ttxMsg %u  txerr %u\n",
			 mailbox[i]->txMsgCnt, mailbox[i]->txErrCnt);
	}
    }

  /* release semaphore */
  up(&(mbModule->procSem));

  return len;
}

/************************************************************
 * EXPORTED module functions 
 ************************************************************/
int RegisterMailBox(MBClient_t* cp)
{
  int status = MB_SUCCESS;
  int cid;
  int registered = 0;

  for(cid = 0; cid < MB_MAX_CLIENT_NUM; cid++)
    {
      if(NULL == mbClients[cid])
	{
	  mbClients[cid] = cp;
	  registered = 1; 
	  mbModule->clientCnt++;
	  break;
	}
      else 
	{
	  if(mbClients[cid]->devId == cp->devId)
	    {
	      /* rduplicate client */
	      status = -EINVAL;
	    }
	}
    }

  if(1 != registered)
    {
      MB_DEBUG("%s: MB client registeration failed\n", 
	       __FUNCTION__);
      status = -EBUSY;
    }
  
  return status;
}

void DeRegisterMailBox(int devId)
{
  int cid;
  for(cid = 0; cid < MB_MAX_CLIENT_NUM; cid++)
    {
      if((NULL !=  mbClients[cid]) &&
	 (mbClients[cid]->devId == devId))
	{
	  mbClients[cid] = NULL;
	  mbModule->clientCnt--;
	}
    }
}

MBClient_t* FindMsgClient(MBMsgHdr_t* hdr)
{
  MBClient_t* cp = NULL;
  int cn;
  for(cn = 0; cn<MB_MAX_CLIENT_NUM; cn++)
    {
      if((NULL != mbClients[cn]) &&
	 ((mbClients[cn]->devId == MSG_DEVID(hdr)) ||
	  (MB_MSG_DEV_LOCAL == MSG_DEVID(hdr))))
	{
	  cp = mbClients[cn];
	  break;
	}
    }
  
  return cp;
}

#define SUPPORT_SEGMENTED_MSG

int SendMsgToMailBox(int mbNum,
		     unsigned int id,
		     unsigned int type, 
		     MBMsg_t* msg, 
		     int msgLen, 
		     unsigned char blocking, 
		     MBMsg_t** reply)
{
  int ret = -EINVAL;
  MBMsgHdr_t* hdr = (MBMsgHdr_t*)msg;

  /* Sanity check */
  if(
#ifndef SUPPORT_SEGMENTED_MSG
     (MB_MSG_PAYLOAD_MAX_SIZE >= msgLen) &&
#endif
     (NULL != msg)                       &&
     (MailBoxNumberIsValid(mbNum))
    )
    {
      InitMsgHeader(hdr, id, type, msgLen);
      
      /* aquire semaphore */
      down(&(mailbox[mbNum]->mbSem));
      ret = SendMBMsg(mbNum, outMsgS, msg);
      
      if(0 != ret)
	{
	  mailbox[mbNum]->txErrCnt++;
	}
      else
	{
	  mailbox[mbNum]->txMsgCnt++;
	
	  if(blocking)
	    {
	      mailbox[mbNum]->replyFlag = -1;
	      if(0 == wait_event_interruptible_timeout(mailbox[mbNum]->wq, 
						       mailbox[mbNum]->replyFlag == msg->hdr.seqNo,
						       MB_REPLY_TIMEOUT))
		{
		  MB_DEBUG("%s: Do not receive reply within timeout interval.\n", 
			   __func__);

		  //reset the TX mailbox so that we can use for next message
		  mailbox[mbNum]->rxTimeOutCnt++;
		  SetMBBufState(mbNum, outMsgS, MB_EMPTY);
		  ret = -ENOMSG;
          goto _SendMsgToMailBox_return;
		}
	      else
		{
#ifndef SUPPORT_SEGMENTED_MSG
		  /* received send reply back to the user */
		  *reply = (MBMsg_t*)GetMBBuffer(mbNum, inMsgS);
#else
          {
		     MBMsg_t* temp_reply;
		     MBMsg_t* segm_msg;
             HNMsg_t* temp_HnMsg;
             int segm_len, cur_segm_len;
             char* payload_ptr;
            
		     temp_reply = (MBMsg_t*)GetMBBuffer(mbNum, inMsgS);
             if ((temp_reply->hdr.length & LENGTH_SEGMENTATION_MASK) == MB_SEGMENTATION_START)           
             {
                // start of segmented msg
                temp_HnMsg = (HNMsg_t *)temp_reply->payload;
                segm_len = temp_HnMsg->data[0];
                cur_segm_len = segm_len;
                segm_msg = (MBMsg_t *)kmalloc(segm_len, GFP_KERNEL);

                if (segm_msg == NULL)
                {
		            MB_DEBUG("%s: kmalloc failed to create %d bytes.\n", 
            			   __func__, segm_len);
                    ret = -ENOMEM;
                    goto _SendMsgToMailBox_return;
                }
                //printk(KERN_INFO "SendMsgToMailBox - segm addr = 0x%p\n", segm_msg);

                // copy over the MB header and HN header
                segm_msg->hdr = temp_reply->hdr;
                ((HNMsg_t *)(segm_msg->payload))->type = temp_HnMsg->type;
                ((HNMsg_t *)(segm_msg->payload))->cmd = temp_HnMsg->cmd;
                segm_msg->hdr.length = MB_SEGMENTED_PAYLOAD | (segm_len + HN_MSG_HDR_LEN + MB_MSG_HDR_LEN);
            
                payload_ptr = (char*)(((HNMsg_t *)(segm_msg->payload))->data);

                //while ((temp_reply->hdr.length & LENGTH_SEGMENTATION_MASK) == MB_SEGMENTED_PAYLOAD)           
                do
                {
                    int len;

                    SetMBBufState(mbNum, inMsgS, MB_EMPTY);

                    mailbox[mbNum]->replyFlag = -1;
                    if(0 == wait_event_interruptible_timeout(mailbox[mbNum]->wq, 
                                           mailbox[mbNum]->replyFlag == msg->hdr.seqNo,
                                           MB_REPLY_TIMEOUT))
                    {
                        MB_DEBUG("%s: Do not receive reply within timeout interval.\n", __func__);
    
                        //reset the TX mailbox so that we can use for next message
                        mailbox[mbNum]->rxTimeOutCnt++;
//                        SetMBBufState(mbNum, outMsgS, MB_EMPTY);
                        ret = (-ENOMSG);
                        kfree(segm_msg);
                        goto _SendMsgToMailBox_return;
                    }
                    else
                    {
		                temp_reply = (MBMsg_t*)GetMBBuffer(mbNum, inMsgS);
                        len = (temp_reply->hdr.length & (~LENGTH_SEGMENTATION_MASK)) - HN_MSG_HDR_LEN - MB_MSG_HDR_LEN;
                        if ((0 < len) && (len <= cur_segm_len))
                        {
                            memcpy((void*)payload_ptr, (void*)(((HNMsg_t *)(temp_reply->payload))->data), len);
                            cur_segm_len -= len;
                            payload_ptr += len;
                        }
                        else
                        {
                            if ((temp_reply->hdr.length & LENGTH_SEGMENTATION_MASK) != MB_SEGMENTATION_END)           
                            {                    
                                ret = (-EINVAL);
                                kfree(segm_msg);
                                goto _SendMsgToMailBox_return;
                            }
                            else
                            {
                                SetMBBufState(mbNum, inMsgS, MB_EMPTY);
                                break;
                            }
                        }
                    }
                }
                while ((temp_reply->hdr.length & LENGTH_SEGMENTATION_MASK) == MB_SEGMENTED_PAYLOAD);

                
		        *reply = segm_msg;

             }
             else
             {
                int msg_len;

                // start of segmented msg
                msg_len = temp_reply->hdr.length & MB_MSG_LEN_MASK;
                *reply = (MBMsg_t *)kmalloc(msg_len, GFP_KERNEL);

                if (*reply == NULL)
                {
		            MB_DEBUG("%s: kmalloc failed to create %d bytes.\n", 
            			   __func__, msg_len);
                    ret = -ENOMEM;
                    goto _SendMsgToMailBox_return;
                }
                memcpy((void*)*reply, (void*)temp_reply, msg_len);
             }
          }
#endif
		}
	    }
	}
      
_SendMsgToMailBox_return:
      /* release the semaphore */
      SetMBBufState(mbNum, outMsgS, MB_EMPTY);
      SetMBBufState(mbNum, inMsgS, MB_EMPTY);
      up(&(mailbox[mbNum]->mbSem));
    }

  if (ret)
    MB_DEBUG("%s: status %u\n", __func__, ret);
  
  return ret;
}

void* GetMBBuffer(int mb, MBMsgStreamE s)
{
  MBStream_t* mbS;
  if(MailBoxNumberIsValid(mb))
    {
      if(MB_STREAM_VPE0_TO_VPE1 == s)
	{
	  mbS = &(mbox[mb]->vpe0ToVpe1);
	}
      else 
	{
	  mbS = &(mbox[mb]->vpe1ToVpe0);
	}
      return (void*)mbS->mbBuf;
    }
  else
    {
      return (void*)0;
    }
}

/************************************************************
 * GetMBBufState: read mail box message stream buffer state
 ************************************************************/
MBMsgStreamE GetMBBufState(unsigned int num, 
			   MBMsgStreamE stream)
{
  MBStream_t* mbS;
  if(MailBoxNumberIsValid(num))
    {
      if(MB_STREAM_VPE0_TO_VPE1 == stream)
	{
	  mbS = &(mbox[num]->vpe0ToVpe1);
	}
      else 
	{
	  mbS = &(mbox[num]->vpe1ToVpe0);
	}

      return mbS->mbBufState;
    }
  else 
       {
	 return MB_UNKNOWN;
       }
}

void SetMBBufState(unsigned int num, 
		   MBMsgStreamE stream,
		   MBStreamStateE state)
{
  MBStream_t* mbS;
  if(MailBoxNumberIsValid(num))
    {
      if(MB_STREAM_VPE0_TO_VPE1 == stream)
	{
	  mbS = &(mbox[num]->vpe0ToVpe1);
	}
      else 
	{
	  mbS = &(mbox[num]->vpe1ToVpe0);
	}

      mbS->mbBufState = state;
    }
}

EXPORT_SYMBOL(RegisterMailBox);
EXPORT_SYMBOL(DeRegisterMailBox);
EXPORT_SYMBOL(SendMsgToMailBox);
EXPORT_SYMBOL(SendErrReply);
EXPORT_SYMBOL(GetMBBuffer);
EXPORT_SYMBOL(GetMBBufState);
EXPORT_SYMBOL(SetMBBufState);
EXPORT_SYMBOL(inMsgS);
EXPORT_SYMBOL(outMsgS);

module_init (HN_MB_ModuleInit);
module_exit (HN_MB_ModuleExit);
MODULE_LICENSE ("GPL");
