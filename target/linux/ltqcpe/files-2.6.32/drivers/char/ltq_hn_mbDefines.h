/************************************************************************
 * 
 * This files specifies Mialbox common data structures, and common defines. 
 * This file is shared by both Linux and ThreadX software. Therefore, this
 * file should avoid any OS dependency. 
 *
 ************************************************************************/

#ifndef _LTQ_HN_MBDEFINES_H_
#define _LTQ_HN_MBDEFINES_H_


//#define MB_REGION_START 0xBF280000      /* This is the start of mailbox region */

#if 0
#if (HN_16M_DRAM)
#define MB_REGION_START (0xa0000000 + 14*1024*1024)
#else
#define MB_REGION_START (0xa0000000 + 30*1024*1024)
#endif
#endif
//extern unsigned int vpe1_load_addr;
//#define MB_REGION_START  (KSEG1ADDR(vpe1_load_addr))
//For timebeing force start address to 30M instead of  taking from kernel command line
#define MB_REGION_START (0xa0000000 + 30*1024*1024)
#define MB_REGION_END   (MB_REGION_START + 8*1024 -1)
#define MB_DESC_TBL_SIZE 0x100 
#define MB_BUF (MB_REGION_START + MB_DESC_TBL_SIZE)  /* This is the mailbox buffer start location */
#define MB_MAGIC    0xabcdab00
#define NAME_LENGTH 100


typedef enum 
  {
    MB_HI_PRIOR, 
    MB_MI_PRIOR,
    MB_LO_PRIOR,
    
    MB_PRIOR_NUM
  }MBPriorityE;


typedef struct MBMsgHdr
{
  unsigned int     type;   // bit 31-28 = devId
                           // bit 7 - msg ACK flag
                           // bit 8 - msg NACK flag 
                           // used in request reply indicating request failed
                           // bit 3-0 = msg Type

  unsigned int     seqNo;  // special field used by VPE0_SW
        
  unsigned int     length; // bit 31-30 = segmentation: 
                           //     0x00(b) not segmented
                           //     0x01(b) segment start
                           //     0x10(b) segmented msg transfers
                           //     0x11(b) segmented end
                           
                           // bit 29-0 = length of (header plus payload in bytes)
}MBMsgHdr_t;

/****************************************
 * message header type field 
 ****************************************/
#define MB_MSG_HDR_DEV_OFFSET  28
#define MB_MSG_HDR_DEV_MASK (0xF << MB_MSG_HDR_DEV_OFFSET) 

typedef enum {
  MB_MSG_DEV_LOCAL,
  MB_MSG_DEV_ETH,
  MB_MSG_DEV_HN,
  MB_MSG_DEV_DRV,

  MB_MSG_DEV_MAX  
}MB_MSG_DEVID;

#define MB_MSG_HDR_TYPE_MASK    (0xF) 


typedef enum {
  
  MB_MSG_TYPE_HELLO  = 1,
  MB_MSG_TYPE_NOTIFY,
  MB_MSG_TYPE_CRITICAL_NOTIFY,
  MB_MSG_TYPE_REQUEST,
  MB_MSG_DEBUG_ACCESS,
  MB_MSG_DEBUG_RESET,
  
  /* add new type above this line */
  MB_MSG_TYPE_ERROR = 0xF
} MB_MSG_TYPE;


// ACK means a reply from a successful operation
// payload would carry intended data structure
#define MB_MSG_TYPE_ACK_OFFSET   7
#define MB_MSG_TYPE_ACK_MASK     0x1<<(MB_MSG_TYPE_ACK_OFFSET) 


// NACK mean a reply from a unsuccessful operation
// payload would carry error code(s)
#define MB_MSG_TYPE_NACK_OFFSET  8
#define MB_MSG_TYPE_NACK_MASK    0x1<<(MB_MSG_TYPE_NACK_OFFSET)

#define MB_MSG_LEN_MASK 0x3FFFFFFF      
#define LENGTH_SEGMENTATION_MASK    0xC0000000
#define MB_SEGMENTATION_START  0x40000000
#define MB_SEGMENTED_PAYLOAD   0x80000000
#define MB_SEGMENTATION_END    0xC0000000

#define SEGMENTATION_START_PAYLOAD_LENGTH       8   // 4-byte struct_size; 4-byte struct pointer
    #define SEGMENTATION_START_PAYLOAD_STRUCT_SIZE_OFFSET       0   // first payload word = struct_size
    #define SEGMENTATION_START_PAYLOAD_STRUCT_PTR_OFFSET        1   // first payload word = struct_size

/* some helpers */
#define MSG_LEN(hdr) \
  (hdr->length & MB_MSG_LEN_MASK)
#define SET_MSG_LEN(hdr, len)  \
  (hdr->length &= LENGTH_SEGMENTATION_MASK);    \
  (hdr->length |= len)
#define MSG_DEVID(hdr) \
  ((hdr->type & MB_MSG_HDR_DEV_MASK) >> MB_MSG_HDR_DEV_OFFSET)
#define MSG_TYPE(hdr) \
  (hdr->type & MB_MSG_HDR_TYPE_MASK)
#define MSG_ACKBIT(hdr) \
  ((hdr->type & MB_MSG_TYPE_ACK_MASK) >> MB_MSG_TYPE_ACK_OFFSET)
#define MSG_NACKBIT(hdr) \
  ((hdr->type & MB_MSG_TYPE_NACK_MASK) >> MB_MSG_TYPE_NACK_OFFSET)
  

#define MB_NUM                  4
#define MB_MSG_PAYLOAD_MAX_SIZE 1500
#define MB_MSG_HDR_LEN          (sizeof(MBMsgHdr_t))
#define MB_BUF_SIZE             (MB_MSG_PAYLOAD_MAX_SIZE + MB_MSG_HDR_LEN)

#ifndef VPE1_SW 
#define MB_VERSION 1
#define MB_DESCTBL_HDR_SIZE (sizeof(MBDescTbl_t) + 4*sizeof(unsigned int))

typedef struct {
  MBPriorityE p;
  unsigned int s;
  const char* procFsName;
}mbParams_t;

#endif

/************************************************************
 * MBStream: an uni-direction message stream 
 *   - mbBufState: the buffer state 
 *   - mbBufAddr:  the location of the message buffer
 ************************************************************/
typedef struct MBStream
{
  unsigned int mbBufState;
  unsigned char*mbBuf;
} MBStream_t;

/************************************************************
 * Mailbox descriptor: Each MBPairDesc descripts a pair of 
 * unidirectional mailbox, which is formed by two MBStreams.
 ************************************************************/ 

typedef struct MBPairDesc
{
  MBPriorityE priority;
  unsigned int size;
  MBStream_t vpe0ToVpe1;
  MBStream_t vpe1ToVpe0;
}MBPairDesc_t;

/************************************************************
 * Mailbox descriptor table: This table contains
 *     - mbVersion:   the version of the mailboxes
 *     - mbNumOfDesc: number of mailbox descriptor 
 *     - mbDescPair: List of descriptors
 ************************************************************/
typedef struct MBDescTbl
{
  int mbVersion;
  unsigned int mbNumOfDesc;
  MBPairDesc_t mbPair[MB_NUM];
} MBDescTbl_t;

/************************************************************
 * MBBufDesc descripts mailbox buffer format
 ************************************************************/
#ifdef VPE1_SW
    typedef struct {
        MBMsgHdr_t      t_MsgHdr;
        unsigned int    Payload[MB_MSG_PAYLOAD_MAX_SIZE/4];   // payload will employ a modified TV block structure
                                                  // Payload[0]   - T
                                                  // Payload[1]   - first location of V-block
    } MsgBuf_t;
#endif

typedef enum {
  MB_ERR_NONE = 0,
  MB_ERR_INV_MSG_TYPE,
  MB_ERR_INV_CMD,
  MB_ERR_OPR_FAILED,
  MB_ERR_INV_INDEX,
  MB_ERR_INV_LENGTH,
  MB_ERR_INV_DEVID,
  MB_ERR_BAD_MB_STATE,
  MB_ERR_NO_HANDLER,
  MB_ERR_FIFO_OVER,
  MB_ERR_BAD_MSG,
  MB_ERR_INV_DATA_TYPE,
  MB_ERR_INV_DATA_COUNT,
  MB_ERR_INV_ADDR_RANGE,
  MB_SEGMENTED_MSG,
  MB_ERR_MAX
} MB_ERROR_CODES;


/****************************************
 * HN cmdType field 
 ****************************************/
#define CMD_TYPE_BLOCK_OFFSET   0x100

#define MB_MSG_CMD_ETH_BEGIN    0x0

#define MB_MSG_CMD_HN_BEGIN     (MB_MSG_CMD_ETH_BEGIN + CMD_TYPE_BLOCK_OFFSET)

#define MB_MSG_CMD_LOCAL_BEGIN  (MB_MSG_CMD_HN_BEGIN + CMD_TYPE_BLOCK_OFFSET)

/************************************************************
 * MBMsg_t: Mail box message format
 *    - hdr:  message header specified above 
 *    - payload: message payload in fixed length 
 ************************************************************/
       
typedef struct MBMsg
{
  MBMsgHdr_t   hdr;
  unsigned int payload[MB_MSG_PAYLOAD_MAX_SIZE/4];
}MBMsg_t;

typedef struct MBErrMsg
{
  MBMsgHdr_t   hdr;
  unsigned int errCode;
}MBErrMsg_t;

typedef struct SW_MBMsg                                             
{                                                           
  	MBMsgHdr_t mbHdr;                                         
  	int command;                                              
  	int payLoad[MB_MSG_PAYLOAD_MAX_SIZE/4 - 1];               
}SW_MBMsg_t;
/************************************************************************
 * MB defines 
 ************************************************************************/
typedef enum
  {
    MB_SUCCESS = 0,
    MB_FAILURE = 1
  }MBstatus;

typedef enum
  {
    MB_STREAM_VPE0_TO_VPE1 = 0,
    MB_STREAM_VPE1_TO_VPE0 = 1,
    
    MB_STREAM_NUM
  }MBMsgStreamE;

typedef enum
  {
    MB_UNKNOWN = 0,
    MB_EMPTY,
    MB_HAS_DATA,    
    MB_BEING_PROCESSED
  }MBStreamStateE;


/********************************************************************
 * Device specific defines that VPE1 message controller needs to know 
 ********************************************************************/

/* 
 * HN message header (32-bit)
 * This field specifies HN message type. VPE1 message controller 
 * should echo this field in reply message. In a notify message, VPE1 
 * message controller should fill this field with value HN_MSG_TYPE_EVENT
 */
typedef enum
  {
    HN_MSG_TYPE_UNKNOWN = 0x0,
    HN_MSG_TYPE_EVENT  = 0x1<<0,
    HN_MSG_TYPE_CFG    = 0x1<<1,
    HN_MSG_TYPE_PMON   = 0x1<<3,
    HN_MSG_TYPE_HELLO  = 0x1<<4,
    HN_MSG_TYPE_DEBUG  = 0x1<<5,
    HN_MSG_TYPE_RESET  = 0x1<<6
  }HNMsgTypeE;

#endif /* _LTQ_HN_MBDEFINES_H_ */
