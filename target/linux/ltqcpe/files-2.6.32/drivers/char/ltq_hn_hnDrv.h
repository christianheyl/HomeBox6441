#ifndef _LTQ_HN_HNDRV_H_
#define _LTQ_HN_HNDRV_H_

#ifdef __KERNEL__
#include <linux/cdev.h>
#endif /*__KERNEL__ */

#ifndef _WINDOWS 
#define HN_IOC_MAGIC 'H'
#define HN_IOCTL_SET_DEV  _IOW(HN_IOC_MAGIC, 0, int)
#define HN_IOCTL_GET_DEV  _IOR(HN_IOC_MAGIC, 1, int)
#define HN_IOCTL_GET_MSG  _IOR(HN_IOC_MAGIC, 2, int)
#define HN_IOCTL_SEND_MSG _IOR(HN_IOC_MAGIC, 3, int)
#define HN_IOCTL_DEBUG    _IOWR(HN_IOC_MAGIC, 4, int)
#define HN_IOCTL_RESET    _IOWR(HN_IOC_MAGIC, 5, int)
#else
#define HN_IOCTL_SET_DEV  0
#define HN_IOCTL_GET_DEV  1
#define HN_IOCTL_GET_MSG  2
#define HN_IOCTL_SEND_MSG 3
#define HN_IOCTL_DEBUG    4
#define HN_IOCTL_RESET    5
#endif 

#define HN_SUCCESS 0
#define HN_PROC_NAME "hnDev"
#define HN_DEVICE_NAME  "hnDriver"

//#if defined(__KERNEL__) || defined(_WINDOWS) 
#include "ltq_hn_mbDefines.h"

//#endif 

#define HN_MSG_LEN (MB_MSG_PAYLOAD_MAX_SIZE - 8)
typedef struct 
{
  HNMsgTypeE type;     /* defined in ltq_hn_mbDefines.h */
  unsigned int cmd;    /* defined in ltq_hn_mbDefines.h */
  unsigned int data[HN_MSG_LEN]; 
}HNMsg_t;

// this is used by both VPE0 and PC code
#define HN_MSG_HDR_LEN 8

#if defined(__KERNEL__)
#define HN_FIFO_LEN 8 /* numbe rof messages allowed to be put in temproray placeholder */  

#define HN_INFO(format...)        \
  do { printk(KERN_INFO format); } while ( 0 ) 
#define HN_DEBUG(format...)	  \
  do { printk(KERN_INFO format); } while ( 0 )  

typedef struct HNMsgBuffer 
{
  /* list */
  struct HNMsgBuffer *prev;
  struct HNMsgBuffer *next;

  int data;
  HNMsgTypeE type;
  int len; 
  unsigned char buf[MB_BUF_SIZE];
}HNMsgBuffer_t;

typedef struct HNMBSubscriber  
{
  struct HNMBSubscriber *prev;
  struct HNMBSubscriber *next;

  struct task_struct* task;
  int sigNo;  
  HNMsgTypeE type;
  HNMsgBuffer_t* fifo;
}HNMBSubscriber_t;

typedef struct 
{
  int initialized;
  int started;
  struct cdev cdev;
  struct semaphore hnSem;   /* HN driver IOCTL semaphore */
  int users;
}HNDriver_t;

/* Private data to track each user space open instance. */

/*
 *  Debug/Assert/Error Message
 */
#define DBG_ENABLE_MASK_ERR             (1 << 0)
#define DBG_ENABLE_MASK_DEBUG           (1 << 1)

#define HN_STR_CFG_MAXLEN 64

typedef enum {
  HNInitMode_Cold = 0,
  HNInitMode_warm, 
  
  HNInitMode_max
}HNInitModeE;

typedef struct
{
  unsigned int size;
  unsigned int buf;
}HNVpeBinBuffer_t;

#endif   /* __ kernel __ */

typedef struct
{
  int sigNo; 
  int type;
}HNSubscriber_t;

typedef struct 
{
  unsigned int mb;
  unsigned int blocking;
  unsigned int msg;
  unsigned int msgLen;
  unsigned int reply;
  int          replyLen;
}HNSendMsgOpParam_t;

typedef struct 
{
  unsigned int type;
  unsigned int msg;
}HNGetMsgOpParam_t;

#define HN_CMD_START 0x100
typedef enum
  {
    HNIoctl_Unknown = HN_CMD_START + 0xD0,
    
    /* mail box operations */
    HNIoctl_MBSendHello,
    HNIoctl_MBSubscribe,
    HNIoctl_MBUnSubscribe,
    HNIoctl_MBSetRxFlag,
    
    /* HN device configuration and minitoring */
    
    HNIoctl_DebugRead,
    HNIoctl_DebugWrite,

    /* vpe management command */
    //HNIoctl_LoadVpeBinary,

    /* vpe 1 software reset command */
    HNIoctl_SwReset,

    HNIoctl_Max
  }HNIoctlSubCmdE;

typedef struct 
{
  HNIoctlSubCmdE subCmd;   //HN operation command
  int length;          //length of "param" if it is a pointer to a data structure
  unsigned int param;  // this field can be used as a pointer to a data structure
}HNDrvIoctlCmd_t;

#ifdef _WINDOWS
int HN_ModuleIoctl(struct inode *, struct file *, 
			  unsigned int, unsigned long);
#endif

#endif /* _LTQ_HN_HNDRV_H_ */
