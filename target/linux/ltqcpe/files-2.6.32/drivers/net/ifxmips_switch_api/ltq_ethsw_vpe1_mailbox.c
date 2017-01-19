/****************************************************************************
                              Copyright (c) 2010
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************
   \file ltq_ethsw_flow_api.c
   \9/6/2011 Yinglei Huang: Initial version separated from ifx_ethsw_flow_api.c
 *****************************************************************************/
#include <linux/kernel.h>                                   
#include <linux/module.h>                                   
#include <linux/errno.h>                                    
#include <linux/proc_fs.h>                                  
#include <linux/delay.h>                                    
#include <linux/kthread.h>                                  
#include <linux/mm.h>                                       

#include <ifx_ethsw_flow_core.h>
#include <ifx_ethsw_pce.h>
#include <ifx_ethsw_flow_ll.h>
#include <ifx_ethsw_core_platform.h>
#include <ltq_hn_mbDefines.h>
#include <ltq_hn_mbMod.h>

/************************************************************************
 * The following functions replace all of the switch driver core API's
 * defined above on the VPE0 side, as mail box is used to send ioctl
 * request to VPE1. Only one API is needed to do this which is now
 * defined by sw_ioctl_mb(). Mailbox receive handler and registration
 * are needed and defined here too
 *
 *************************************************************************/
#define SWDRV_DEBUG  1

#define SW_DEBUG(format...)                     \
   if(SWDRV_DEBUG) printk(KERN_DEBUG format);

#define MB_MSG_PAYLOAD_BYTES    (MB_MSG_PAYLOAD_MAX_SIZE -4)  //take away 4-bytes of command

static MBClient_t swMBClient;
static int swMailBox = 0; // using mail box 1 for Tx/Rx. It is not specified when register with mailbox
// this really should be mail priority, for client should not know which mail box
// to use
static int replyFlag;  // for signaling response msg from vpe1
static wait_queue_head_t swMbWq;

void IFX_SW_DeAttachMailBox(void);
int IFX_SW_AttachMailBox(void);

static int SW_RxMsgHandler(int mbNum, void* data)
{
    int status = 0;

    MBMsgHdr_t* hdr = ( MBMsgHdr_t*)data;

    if (NULL!= hdr) {
        if( 0 != MSG_ACKBIT(hdr) ) {
            /* wake up the wait queue */

            replyFlag = hdr->seqNo;
            wake_up_interruptible(&swMbWq);
        }
        else {
            SW_DEBUG("%s: unknow message type <%d>.\n", __func__, hdr->type);
        }
    }

    /* We are done. Set the mailbox flag so the other end can
       send us another message.
    */

    return status;
}

void IFX_SW_DeAttachMailBox()
{
    DeRegisterMailBox(swMBClient.devId);
}

int IFX_SW_AttachMailBox()
{
    int status = IFX_SUCCESS;

    if (swMBClient.devId == MB_MSG_DEV_ETH) {
        /* This should not happen. But just in case, we de-attach
         * the mailbox and restart again
         */
        DeRegisterMailBox(swMBClient.devId);
        swMBClient.devId = 0;
    }

    swMBClient.devId = MB_MSG_DEV_ETH;
    swMBClient.rxMsgHdlr = SW_RxMsgHandler;

    status = RegisterMailBox(&swMBClient);
    if (IFX_SUCCESS !=  status) {
        SW_DEBUG("%s: Failed to register mailbox\n", __func__);
        status = IFX_ERROR;
    }


    return status;
}

/*
 * ifx_sw_ioctl_comm() : generic function to highjack user ioctl calls and send it to vpe1 as mailbox message,
 *      and return back date for the get operations.
 * Arguments:
 *      command: passed from above, 4 bytes, encoded as ((RW << 30) | (size << 16) | (nType << 8) | funcIndex),
 *          RW = (None(0), Write(1), Read(2)), size is 12 bits (bit 2-3 of command are not used), nType=('E', 'F') for
 *          the ETHSW table (main SW function table "ifx_flow_fkt_ptr_tbl" with 97 routines) and for the
 *          FLOW table (switch debug function table "ifx_flow_fkt_tbl" with 20 routines, respectively
 *      pPar : pointer to the user parameter buffer
 */
IFX_return_t ifx_sw_ioctl_comm(void *handle, int command, IFX_uint32_t pPar)
{
    int status;
    MBMsg_t *rep = NULL;
    SW_MBMsg_t* reply = NULL;
    MBMsgHdr_t *pHdr;
    SW_MBMsg_t  *pMsg;
    unsigned int wait = 1; //wait for mailbox to ack
    int payloadLen;

    IFX_uint8_t  RW = _IOC_DIR(command);
    IFX_uint16_t size = _IOC_SIZE(command);

    if (((RW != _IOC_WRITE)&&(RW != _IOC_READ)&&(RW != (_IOC_READ | _IOC_WRITE))) || (size > MB_MSG_PAYLOAD_BYTES)) {
        SW_DEBUG( "%s: wrong param RW=%d size=%d\n", __FUNCTION__, (int)RW, (int)size);
        return IFX_ERROR;
    }

    pMsg = (SW_MBMsg_t  *)kmalloc(sizeof(SW_MBMsg_t), GFP_KERNEL);
    if (pMsg == NULL) {
        SW_DEBUG("%s: malloc failed!!!\n", __FUNCTION__);
        return IFX_ERROR;
    }

    pMsg->command = command;
    memcpy(pMsg->payLoad,(void *)pPar, size);

    payloadLen = 4 + size;   //command is 4-bytes

    /* Send the message and wait for reply */

    status = SendMsgToMailBox(swMailBox, MB_MSG_DEV_ETH, MB_MSG_TYPE_REQUEST, (MBMsg_t *)pMsg, payloadLen, wait, &rep);

    if (IFX_SUCCESS == status) {
        /* we received a reply, return reply content to user */
        reply = (SW_MBMsg_t*)rep;
        if (reply) {
            pHdr = (MBMsgHdr_t *)&(reply->mbHdr);

            // validate msg is for me, and no error from vpe1 side swdrv execution
            if ((MSG_DEVID(pHdr) == MB_MSG_DEV_ETH) && (!MSG_NACKBIT(pHdr)) && (command == reply->command))
            {
                //The response msg length is total size of msg
                size = reply->mbHdr.length - (sizeof(MBMsgHdr_t) + 4); // take away msg header and 4-bytes command
		// For read, copy data back
                if (RW & _IOC_READ) {
                    memcpy((void *)pPar, reply->payLoad, size);
                }
            }
            else {
                SW_DEBUG("%s  ACK ERROR: devId=%d , command=0x%08x, ack_devId=%d, ack_command=0x%08x, nack=%d\n",
                         __FUNCTION__, MB_MSG_DEV_ETH, command, MSG_DEVID(pHdr), reply->command, MSG_NACKBIT(pHdr));
                status = IFX_ERROR;
            }
           SetMBBufState(swMailBox, inMsgS, MB_EMPTY);
        }
        else
            if (wait)
            {
               SW_DEBUG("%s ERROR: NO response msg devId=%d command=0x%08x\n", __FUNCTION__, MB_MSG_DEV_ETH, command);
               status = IFX_ERROR;
            }
    }



    kfree(pMsg);

    return status;
}


