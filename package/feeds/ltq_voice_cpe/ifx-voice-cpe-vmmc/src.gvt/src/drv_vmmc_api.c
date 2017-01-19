/******************************************************************************

                              Copyright (c) 2013
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_vmmc_api.c
   Implements the interface towards the chip.
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_api.h"
#include "drv_vmmc_api.h"
#include "drv_vmmc_access.h"

#ifdef PMC_SUPPORTED
#include "drv_vmmc_pmc.h"
#endif /* PMC_SUPPORTED */


/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/*lint -e 754 */
struct msgHead_t {
#if IFXOS_BYTE_ORDER == IFXOS_BIG_ENDIAN
   CMD_HEAD_BE;
#else
   #error LITTLE ENDIAN is currently not supported.
#endif
};

/* ============================= */
/* Local function declaration    */
/* ============================= */
extern IFX_int32_t VMMC_WaitForCmdMbxData(VMMC_DEVICE *pDev);
static IFX_int32_t cmdWrite(VMMC_DEVICE *pDev,
                            IFX_uint32_t *pCmd,
                            IFX_uint16_t nCount);

/* ============================= */
/* Local function definitions    */
/* ============================= */

/* ============================= */
/* Global function definitions   */
/* ============================= */

/** @defgroup VMMC_MBX_INTERFACE Driver Mailbox Interface
    @{ */

/**
   Write command

   The length information is written into the header and the RW field is set
   to CMDWRITE. For security purposes the reserved bits in the command header
   are set to 0. All other fields in the command header are left untouched.

   \param  pDev         Pointer to the device structure.
   \param  pCmd         Pointer to the buffer with the command. It contains the
                        command header and the data to be written.
   \param  nCount       Number of data BYTES to write, without command header.
                        Must not be larger than the maximum mailbox size.

   \return
   - VMMC_statusOk      on success.
   - IFX_ERROR on error. Error can occur if command-inbox space is not
                        sufficient or if low level function/macros fail.
   - VMMC_ERR_NO_FIBXMS Not enough inbox space for writing command.

   \remarks
   Caution: nCount is in bytes now, not in words.
*/
IFX_int32_t CmdWrite(VMMC_DEVICE *pDev, IFX_uint32_t *pCmd, IFX_uint16_t nCount)
{
   struct msgHead_t *pMsgHead = (struct msgHead_t *) pCmd;

   /* This wrapper just makes sure that the RW bit is set to "write". */

   /* Set fields in command header to defined values. */
   pMsgHead->RW = CMDWRITE;
   pMsgHead->LENGTH = nCount;
   pMsgHead->Res00 = 0;

   return cmdWrite(pDev, pCmd, nCount);
}


/**
   Write command without blocking, intended for use within interrupt context.

   The length information is written into the header and the RW field is set
   to CMDWRITE. For security purposes the reserved bits in the command header
   are set to 0. All other fields in the command header are left untouched.

   This function can actually be called from interupt or task context. If called
   from task context it will protect itself against interrupts and tasks.

   \param  pDev         Pointer to the device structure.
   \param  pCmd         Pointer to the buffer with the command. It contains the
                        command header and the data to be written.
   \param  nCount       Number of data BYTES to write, without command header.
                        Must not be larger than the maximum mailbox size.

   \return
   - VMMC_statusOk      on success.
   - IFX_ERROR on error. Error can occur if command-inbox space is not
                         sufficient or if low level function/macros fail.
   - VMMC_ERR_NO_FIBXMS Not enough inbox space for writing command.

   \remarks
   Caution: nCount is in bytes now, not in words.
*/
IFX_int32_t CmdWriteIsr (VMMC_DEVICE *pDev,
                         IFX_uint32_t* pCmd, IFX_uint16_t nCount)
{
   struct msgHead_t *pMsgHead = (struct msgHead_t *) pCmd;

   /* This wrapper just makes sure that the RW bit is set to "write". */

   /* Set fields in command header to defined values. */
   pMsgHead->RW = CMDWRITE;
   pMsgHead->LENGTH = nCount;
   pMsgHead->Res00 = 0;

   return cmdWrite(pDev, pCmd, nCount);
}

/** @} */


/**
   Internal: write command to the mailbox

   \param  pDev         Pointer to the device structure.
   \param  pCmd         Pointer to the buffer with the command. It contains the
                        command header and the data to be written.
   \param  nCount       Number of data BYTES to write, without command header.
                        Must not be larger than the maximum mailbox size.

   \return
   - VMMC_statusOk      on success.
   - IFX_ERROR on error. Error can occur if command-inbox space is not
                         sufficient or if low level function/macros fail.
   - VMMC_ERR_NO_FIBXMS  Not enough inbox space for writing the command.
*/
static IFX_int32_t cmdWrite(VMMC_DEVICE *pDev,
                            IFX_uint32_t *pCmd,
                            IFX_uint16_t nCount)
{
   IFX_int32_t err = VMMC_statusOk;
   mps_message msg;
   struct msgHead_t *pMsgHead = (struct msgHead_t *) pCmd;

   /* The command header must not be all zero. */
   VMMC_ASSERT ((*pCmd & 0xffff0000) != 0);
   /* The command must be aligned to an even 32 bit address */
   VMMC_ASSERT (((IFX_uint32_t)pCmd & 0x3) == 0);

   /* increase count by the length of the command header */
   nCount += CMD_HDR_CNT;
   /* force the number of bytes to be a multiple of 4 - this is required
      for example in case of COP CRAM download, the length tag in the
      message can be a multiple of 2, while the msg.nDataBytes field must
      be a multiple of 4, the calling function should add some dummy
      data (padding) at the end of the pCmd buffer. */
   if (nCount % 4 != 0)
   {
      nCount += 4 - (nCount%4);
   }

   msg.pData          = (IFX_uint8_t *)pCmd;
   msg.nDataBytes     = nCount;
   msg.RTP_PaylOffset = 0;
   msg.cmd_type       = pMsgHead->CMD;

#ifndef PMC_SUPPORTED
   /* concurrent access protect by driver, but it should move to here.
       To cease interrupt and to use semaphore here is a good idea.*/
   VMMC_HOST_PROTECT(pDev);
   err = ifx_mps_write_mailbox(command, &msg);
   VMMC_HOST_RELEASE(pDev);
#else
   /* This does exactly the same as the code above. Additionally it records
      the enable and disable of selected FW features and reports the status
      to the power management control unit. */
   err = VMMC_PMC_Write(pDev, &msg);
#endif /* PMC_SUPPORTED */


   /* The following log macro should only log write commands.
    * Read commands are always assumed to be written through function CmdRead(),
    * and they are logged in that function. */
   if (pMsgHead->RW == CMDWRITE)
   {
      LOG_WR_CMD(pDev->nDevNr, pDev->nChannel, pCmd, nCount>>1,
                 !err ? err : pDev->err);
   }

   /* MPS returns -1 if error */
   if (err != VMMC_statusOk)
   {
      SET_DEV_ERROR (VMMC_ERR_MBXWRITE);
      RETURN_INFO_DEVSTATUS (VMMC_statusCmdWr,pCmd, nCount);
   }
   return err;
}


/** \addtogroup VMMC_MBX_INTERFACE */
/** @{ */

/**
   Read Command

   Read data from the device in a synchronous way. A read command is written
   using the command write routine. Then this code waits for response data
   to arrive in the cmd out mailbox, reads and returns it.

   The command write routine is used to write the read command. In this case,
   the protection against interupts and concurent tasks is done by Command Read
   routine itself. Refer to Command Write routine for more details about
   writing commands.

   \param  pDev         Pointer to the device structure.
   \param  pCmd         Pointer to the buffer with the command. It contains the
                        command header to be written.
   \param  pData        Pointer to a buffer for the result. It must be large
                        enough for the command header and the number of data
                        bytes to  read given in parameter nCount.
   \param  nCount       Number of data BYTES to read, without command header.
                        Must not be larger than the maximum mailbox size.

   \return
   - IFX_SUCCESS         on success.
   - IFX_ERROR on error. Error can occur if command-inbox space is not
                         sufficient or if low level function/macros fail.
   - VMMC_ERR_NO_FIBXMS  Not enough inbox space for writing command.
   - VMMC_ERR_OBXML_ZERO No data in the mailbox.

   \remarks
   Caution: nCount is in bytes now, not in words.
   \remarks
   Protection mechanism is done when the command is actually being written.
*/
IFX_int32_t CmdRead (VMMC_DEVICE *pDev, IFX_uint32_t *pCmd,
                     IFX_uint32_t *pData, IFX_uint16_t nCount)
{
   IFX_int32_t  ret, err;
   mps_message msg;
   struct msgHead_t *pMsgHead = (struct msgHead_t *) pCmd;

   VMMC_OS_MutexGet (&pDev->mtxCmdReadAcc);

   /* Set fields in command header to defined values. */
   pMsgHead->RW = CMDREAD;
   pMsgHead->BC = 0;
   pMsgHead->LENGTH = nCount;
   pMsgHead->Res00 = 0;

   /* increase count by length of the command header */
   nCount += CMD_HDR_CNT;

   pDev->bCmdOutBoxData = IFX_FALSE;

   /* Write read command : length 0 because only command header is written. */
   err = cmdWrite(pDev, pCmd, 0);

   if (err == IFX_SUCCESS)
   {
      /* Read command was written successfully. Now wait for data and read it.
         Even when waiting for the data times-out we try to read the mailbox
         to find if there is something in the mailbox and just the wakeup did
         not work. If there was something in the mailbox this would indicate
         a problem in the OS with blocking or waking up. If there is nothing
         in the mailbox this would indicate that the FW did not answer.
         The following cases are distinguished:
            Wait    Read       Comment
            Ok      Ok         Read successful
            Ok      No data    Error: woke-up but no data?
            Timeout Ok         OS problem with blocking or waking up
            Timeout No data    Timeout - FW did not respond
            -       Error      Read error
      */

      /* Wait for data in the mailbox. */
      ret = VMMC_WaitForCmdMbxData(pDev);

      /* Check for CmdReadError.
         The flag is set in IRQ context if CERR irq occurs. */
      if (pDev->bCmdReadError == IFX_TRUE)
      {
         TRACE (VMMC, DBG_LEVEL_HIGH, ("Read Cmd MBX cmderr.\n"));
         err = IFX_ERROR;
      }
      else
      {
         /* Read data if any is available */
         err = ifx_mps_read_mailbox(command, (mps_message*) &msg);

         if (err == IFX_SUCCESS)
         {
            /* Read successful */

            /* FW provides the data we read in a buffer that we now need to
               return back. So we copy the data into the buffer provided by
               the caller and release the one given to us by FW.*/
            memcpy((IFX_uint8_t *)pData,
                   (IFX_uint8_t *)msg.pData, msg.nDataBytes);
            /* Protection is already done in this function. */
            IFX_TAPI_VoiceBufferPut(msg.pData);

            if (ret == 0)
            {
               /* OS problem with blocking or waking up. This is not an
                  immediate error but needs investigation. */
               TRACE (VMMC, DBG_LEVEL_HIGH, ("Read Cmd MBX overslept.\n"));
            }
         }
         else
         if ((err == -ENODATA) && (ret == 0))
         {
            /* Timeout - FW did not respond */
            TRACE (VMMC, DBG_LEVEL_HIGH, ("Read Cmd MBX timeout.\n"));
            VMMC_DevErrorEvent (pDev, VMMC_statusCmdRdTimeout,
                                __LINE__, __FILE__, pCmd, nCount);
            err = VMMC_statusCmdRdTimeout;
         }
         else
         if ((err == -ENODATA) && (ret > 0))
         {
            /* Error: woke-up but no data? */
            TRACE (VMMC, DBG_LEVEL_HIGH,
                   ("Read Cmd MBX spurious wakup - no data.\n"));
            err = IFX_ERROR;
         }
         else
         {
            /* Read error */
            TRACE (VMMC, DBG_LEVEL_HIGH, ("Read Cmd MBX error.\n"));
         }
      }
   }

   VMMC_OS_MutexRelease (&pDev->mtxCmdReadAcc);

   LOG_RD_CMD(pDev->nDevNr, pDev->nChannel, pCmd, pData, nCount>>1,
              !err ? err : (pDev->err ? pDev->err : err));

   return err;
}

/** @} */


#if 0
/*******************************************************************************
Description:
   copy a byte buffer into a dword buffer respecting the endianess.
Argiments
   pDWbuf : DWord buffer
   pBbuf  : Byte buffer
   nB     : size of Bytes to be copied (must be a multiple of 4)
Return:
Remarks:
   No matter if uC supports big endian or little endian, this macro will still
   copy the data in the apropriate way for a big endian target chip like Vinetic
   The pointer pBbuf might be not aligned to 32bit - therefore the dword is
   first copied byte wise into a 32bit variable (tmp) before swapping.
*******************************************************************************/
void cpb2dw(IFX_uint32_t* pDWbuf, IFX_uint8_t* pBbuf, IFX_uint32_t nB)
{
   IFX_uint32_t i = 0, nDW = nB >> 2;
   IFX_uint32_t tmp;
   IFX_uint8_t *pBtmp = (IFX_uint8_t *)pBbuf;
   VMMC_ASSERT (!(nB % 4));
   for ( i = 0; i < nDW; i++ )
   {
      tmp = ((pBtmp[4*i+0] << 24) |
             (pBtmp[4*i+1] << 16) |
             (pBtmp[4*i+2] <<  8) |
             (pBtmp[4*i+3] <<  0));
             (pDWbuf)[i] = (tmp);
   }
}
#endif


/*******************************************************************************
Description:
   copy a byte buffer into a word buffer respecting the endianess.
Argiments
   pWbuf  : Word buffer
   pBbuf  : Byte buffer
   nB     : size of Bytes to be copied (must be a multiple of 2)
Return:
Remarks:
   No matter if uC supports big endian or little endian, this macro will still
   copy the data in the apropriate way for a big endian target chip like Vinetic
   The pointer pBbuf might be not aligned to 32bit - therefore the word is
   first copied byte wise into a 16bit variable (tmp) before swapping.
*******************************************************************************/
void cpb2w (IFX_uint16_t* pWbuf,IFX_uint8_t *pBbuf, IFX_uint32_t nB)
{
   IFX_uint32_t i = 0, nW = nB >> 1;
   IFX_uint16_t tmp;
   IFX_uint8_t *pBtmp = (IFX_uint8_t *) pBbuf;
   VMMC_ASSERT (!(nB % 2));
   for ( i = 0; i < nW; i++ )
   {
      tmp = ((pBtmp[2*i+0] << 8) |
             (pBtmp[2*i+1] << 0));
      (pWbuf)[i] = /*lint --e(661, 662)*/ (tmp);
   }
}


/**
   Debug only - handle a CmdError and read out the reason.

   \param  pLLDev       Pointer to the device structure.
   \param  pData        Pointer to cause and header of the the command.

   \return
   - VMMC_statusCmdWr Writing the command failed
   - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_CmdErr_Handler(IFX_TAPI_LL_DEV_t *pLLDev,
                                IFX_TAPI_DBG_CERR_t *pData)
{
   VMMC_DEVICE    *pDev       = (VMMC_DEVICE *) pLLDev;
   SYS_CERR_GET_t  cmdErrGet;
   int err;

   memset (&cmdErrGet, 0, sizeof(SYS_CERR_GET_t));
   cmdErrGet.MOD  = MOD_SYSTEM;
   cmdErrGet.CMD  = CMD_EOP;
   cmdErrGet.ECMD = SYS_CERR_GET_ECMD;

   /* reset internal state for CmdRead */
   pDev->bCmdReadError = IFX_FALSE;

   err = CmdRead(pDev, (IFX_uint32_t*) &cmdErrGet, (IFX_uint32_t*) &cmdErrGet,
                 sizeof(SYS_CERR_GET_t) - CMD_HDR_CNT);

   pData->cause = cmdErrGet.cause;
   pData->cmd   = cmdErrGet.cmd;

   return err;
}
