#ifndef __HN_MB_MODULE_H
#define __HN_MB_MODULE_H

#ifdef __KERNEL__
#include <linux/cdev.h>
#endif  /* _KERNEL_ */

/**
 * Mailbox Header Descriptors
 *
 **/

/*
 *  Debug/Assert/Error Message
 */

//#define DBG_ENABLE_MASK_ERR             (1 << 0)
//#define DBG_ENABLE_MASK_DEBUG_PRINT     (1 << 1)
//#define DBG_ENABLE_MASK_ASSERT          (1 << 2)

#define MB_POLL_INTERVAL_MS 1
#define MB_POLL_ENABLE  1
#define MB_POLL_DISABLE 0
#define MB_REPLY_TIMEOUT 500

//#define MB_IN_MSG_BUF_NUM 4

#ifdef __KERNEL__
typedef struct
{
  int openCnt;
  int clientCnt;
  struct cdev   cdev;
  struct semaphore procSem;
}MBDrv_t;

typedef struct
{
  struct work_struct work;
  int mb;
}mb_irq_work_t;

typedef struct
{
  int pollEn;
    
  /* statistcs */
  unsigned int rxMsgCnt;
  unsigned int rxDropCnt;
  unsigned int rxErrCnt;
  unsigned int rxTimeOutCnt;

  unsigned int txMsgCnt;
  unsigned int txErrCnt;  
  unsigned int replyFlag;
  
  struct semaphore mbSem;
  wait_queue_head_t wq;
}MBox_t;

typedef int (*MB_MSG_HDLR_CBK)(int mbNum, void* data);
#define MB_MAX_CLIENT_NUM 4 	
typedef struct 
{
  int  devId;
  MB_MSG_HDLR_CBK rxMsgHdlr;
}MBClient_t;

typedef int (*MB_PROCFS_READ)(char *buffer, 
			      char **buffer_location,
			      off_t offset, int buffer_length, int *eof, 
			      void *data);
typedef struct {
  const char* procFsName;
  MB_PROCFS_READ procFsReadFunc;
}mbProcFs_t;

// mailbox module exported functions and variables

extern const MBMsgStreamE inMsgS;
extern const MBMsgStreamE outMsgS;
extern void SendErrReply(int mbNum,  MB_ERROR_CODES err);
extern void DeRegisterMailBox(int clientId);
extern int RegisterMailBox(MBClient_t *cp);
extern int SendMsgToMailBox(int mbNum, unsigned int id, unsigned int type, MBMsg_t* msg, int msgLen, unsigned char wait, MBMsg_t **reply);
extern void* GetMBBuffer(int mb, MBMsgStreamE s);
extern void SetMBBufState(unsigned int num, MBMsgStreamE stream,MBStreamStateE state);

#endif  /* _KERNEL_ */

#define MB_IOC_MAGIC 'M'
#define MB_IOCTL_SEND_MSG  _IOW(MB_IOC_MAGIC, 1, int)

typedef struct
{
  unsigned char mb;       /* mailbox number */
  unsigned char devId;    /* destination    */
  unsigned char type;     /* message type */
  unsigned char blocking; /* whether this is blocking operation */ 
  unsigned int  msgLen;   /* length of the message */
  unsigned int  replyLen; /* length of reply */
  void*         msg;      /* message to send */
  void*         reply;    /* placeholder for reply */
}MBMsgOpr_t;

#endif /* __HN_MB_MODULE_H */
