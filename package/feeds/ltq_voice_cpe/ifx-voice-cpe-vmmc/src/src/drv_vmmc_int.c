/******************************************************************************

                              Copyright (c) 2013
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

/*
   \file drv_vmmc_int.c
   This file contains the implementation of the interrupt handler.

   \remarks
   The implementation assumes that multiple instances of this interrupt handler
   cannot preempt each other, i.e. if there is more than one VMMC in your
   design all devices are expected to raise interrupts at the same priority
   level.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "drv_api.h"

#include "drv_vmmc_cod_priv.h"
#include "drv_vmmc_sig_priv.h"
#include "drv_vmmc_alm_priv.h"
#include "drv_mps_vmmc.h"
#include "drv_mps_vmmc_device.h"
#include "drv_vmmc_api.h"
#include "drv_vmmc_alm.h"
#include "drv_vmmc_cod.h"
#include "drv_vmmc_sig.h"
#include "drv_vmmc_sig_cid.h"
#include "drv_vmmc_pcm.h"
#include "drv_vmmc_int.h"
#include "drv_vmmc_int_evt.h"
#include "drv_vmmc_res.h"
#include "drv_vmmc_init.h"
#include "drv_vmmc_announcements.h"
#ifdef PMC_SUPPORTED
#include "drv_vmmc_pmc.h"
#endif /* PMC_SUPPORTED */

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */
#define MPS_CH_VALID(mpsCh)      ((mpsCh <= NUM_VOICE_CHANNEL) ? 1 : 0)

/* ============================= */
/* Global function declarations  */
/* ============================= */

/* ============================= */
/* Local function declarations   */
/* ============================= */
static IFX_void_t VMMC_Ad_Callback(MbxEventRegs_s *pEvent);
static IFX_void_t VMMC_Ad0_Callback(VMMC_DEVICE *pDev, MbxEventRegs_s *pEvent);
static IFX_void_t VMMC_EvtMbx_Callback(u32 cookie,
                                       mps_event_msg *pEvtMsg);
static IFX_void_t VMMC_Upstream_Callback(mps_devices mpsChannel);
extern IFX_uint16_t VMMC_ALM_ElapsedTimeSinceLastHook (VMMC_CHANNEL *pCh);

/* ============================= */
/* Local variable definitions    */
/* ============================= */

/* ============================= */
/* Global function definitions   */
/* ============================= */

/**
   VMMC callback routine registration

   \param  pDev         Pointer to VMMC device struct.
   \param  mpsCh        MPS channel

   \return
   IFX_SUCCESS or IFX_ERROR
*/
IFX_return_t VMMC_Register_Callback(VMMC_DEVICE *pDev)
{
   IFX_int32_t ret  = VMMC_statusOk;
   IFX_int32_t mpsCh;
   MbxEventRegs_s mask;

   /* prepare interrupt mask, these are mainly events related to mailbox handling */
   mask.MPS_Ad0Reg.val       = 0x0000CF68;
   mask.MPS_Ad1Reg.val       = 0x00000000;
   mask.MPS_VCStatReg[0].val = 0x00000000;
   mask.MPS_VCStatReg[1].val = 0x00000000;
   mask.MPS_VCStatReg[2].val = 0x00000000;
   mask.MPS_VCStatReg[3].val = 0x00000000;

   /* Register AFE/DFE interrupt handler */
   ret = ifx_mps_register_event_callback(command,
                                         &mask,
                                         VMMC_Ad_Callback);
   if (ret != IFX_SUCCESS)
   {
      TRACE(VMMC, DBG_LEVEL_HIGH,
           ("INFO: Register event callback failed %d.\n", ret));
   }

   /* Register event mailbox handler */
   if (ret == IFX_SUCCESS)
   {
      ret = ifx_mps_register_event_mbx_callback((IFX_uint32_t) pDev,
                                                VMMC_EvtMbx_Callback);
      if (ret != IFX_SUCCESS)
      {
         TRACE (VMMC, DBG_LEVEL_HIGH,
                ("INFO: Register event mailbox failed %d.\n", ret));
      }
   }

   /* Register data upstream mailbox handler */
   for (mpsCh = 1; (ret == IFX_SUCCESS) && MPS_CH_VALID(mpsCh); mpsCh++)
   {
      /** MPS driver channel numbering: 1 stands for command channel,
          2 stands for voice channel 0, 3 stands for voice channel 1, ... */

      ret = ifx_mps_register_data_callback((mps_devices)mpsCh,
                                           1, /* upstream */
                                           VMMC_Upstream_Callback);

      TRACE(VMMC, DBG_LEVEL_LOW,
            ("VMMC_Register_Callback(), registered for mpsCh=%d (%s)\n",
            mpsCh - 1, ret==0 ? "success":"error"));

#ifdef VMMC_FEAT_PACKET
      if (ret == IFX_SUCCESS)
      {
         /* call MPS driver open */
         /* We do not provide a file pointer context as we are calling
            from kernel space for which MPS does not use this. */
         ret = ifx_mps_open((void *)mpsCh, IFX_NULL);
      }
#endif
   }

   /* Enable interrupts according to the configured mask & registered callbacks */
   if (ret == IFX_SUCCESS)
   {
      /* disable DD_MBX interrupt by default */
      mask.MPS_Ad0Reg.fld.dd_mbx = 0;

      ret = ifx_mps_event_activation(command, &mask);
      if (ret != IFX_SUCCESS)
         TRACE (VMMC, DBG_LEVEL_HIGH,("INFO: Activating events failed %d.\n", ret));
   }

   return VMMC_SUCCESS(ret) ? IFX_SUCCESS : IFX_ERROR;
}


/**
   VMMC callback routine de-registration

   \param  mpsCh        MPS channel

   \return
   IFX_SUCCESS or IFX_ERROR
*/
IFX_return_t VMMC_UnRegister_Callback(void)
{
   IFX_int32_t ret  = VMMC_statusOk;
   IFX_int32_t mpsCh;
   MbxEventRegs_s mask;

   /* Unregister data upstream mailbox handler */
   for (mpsCh = 1; (ret == IFX_SUCCESS) && MPS_CH_VALID(mpsCh); mpsCh++)
   {
#ifdef VMMC_FEAT_PACKET
      /* call MPS driver close */
      ret = ifx_mps_close((void *)mpsCh, IFX_NULL);
#endif
      if (ret == IFX_SUCCESS)
      {
         ret = ifx_mps_unregister_data_callback((mps_devices) mpsCh, 1 /* upstream */);
         if (ret != IFX_SUCCESS)
         {
            TRACE (VMMC, DBG_LEVEL_HIGH,
                  ("INFO: [%d] Un-Register data callback failed %d.\n", (mpsCh -1), ret));
         }
      }
      TRACE(VMMC, DBG_LEVEL_LOW,
            ("VMMC_UnRegister_Callback(), unregistered for mpsCh=%d (%s)\n",
            mpsCh - 1, ret==IFX_SUCCESS ? "success":"error"));
      if (ret != IFX_SUCCESS)
      {
         TRACE(VMMC, DBG_LEVEL_HIGH,
               ("Closing MPS driver failed for mpsCh=%d\n", mpsCh - 1));
      }
   }

   /* Unregister AFE/DFE interrupt handler */
   ret = ifx_mps_unregister_event_callback(command);
   if (ret != IFX_SUCCESS)
   {
      TRACE (VMMC, DBG_LEVEL_HIGH,
            ("INFO: Un-Register event callback failed %d.\n", ret));
   }

   /* Unregister event mailbox handler */
   ret = ifx_mps_unregister_event_mbx_callback();
   if (ret != IFX_SUCCESS)
   {
      TRACE (VMMC, DBG_LEVEL_HIGH,
            ("INFO: Un-Register event mailbox callback failed %d.\n", ret));
   }

   /* Mask all interrupts */
   /* Disable will only happen after all callback handlers are unregistered */
   mask.MPS_Ad0Reg.val       = 0x00000000;
   mask.MPS_Ad1Reg.val       = 0x00000000;
   mask.MPS_VCStatReg[0].val = 0x00000000;
   mask.MPS_VCStatReg[1].val = 0x00000000;
   mask.MPS_VCStatReg[2].val = 0x00000000;
   mask.MPS_VCStatReg[3].val = 0x00000000;
   ret = ifx_mps_event_activation(command, &mask);
   if (ret != IFX_SUCCESS)
      TRACE (VMMC, DBG_LEVEL_HIGH,("Disable IRQs failed %d.\n", ret));

   return VMMC_SUCCESS(ret) ? IFX_SUCCESS : IFX_ERROR;
}


/**
   Handles interrupts from AFE and DFE

   \param  pEvent       Pointer to struct reporting the interupt status.
*/
static IFX_void_t VMMC_Ad_Callback(MbxEventRegs_s *pEvent)
{
   VMMC_DEVICE *pDev;

   VMMC_GetDevice (0, &pDev);

   if (pEvent->MPS_Ad0Reg.val)
      VMMC_Ad0_Callback(pDev, pEvent);
   /* Current voice-FW does not use AD1 for signalling events */
}


/**
   Handles interrupts from the AFE and DFE register 0

   \param  pEvent       Pointer to struct reporting the interupt status.
*/
static IFX_void_t VMMC_Ad0_Callback(VMMC_DEVICE *pDev, MbxEventRegs_s *pEvent)
{
   MPS_Ad0Reg_u *pAd0reg = &(pEvent->MPS_Ad0Reg);
   IFX_TAPI_EVENT_t tapiEvent;

   if(pAd0reg->fld.mips_ol)
   {
      SYS_OLOAD_ACK_t mipsOvld;
      pDev->nMipsOl++;

      SET_DEV_ERROR(VMMC_ERR_MIPS_OL);
/*lint -e{525} */
#ifdef VMMC_FEAT_FAX_T38
{
      TAPI_CHANNEL *pChannel;
      IFX_uint32_t nCh;
      VMMC_CHANNEL *pCh;

      for (nCh = 0; nCh < pDev->caps.nALI; nCh++)
      {
         pCh = &pDev->pChannel[nCh];
         pChannel = (TAPI_CHANNEL *) pCh->pTapiCh;

         /* check status and do some actions */
         if (pCh->TapiFaxStatus.nStatus &
            (IFX_TAPI_FAX_T38_DP_ON | IFX_TAPI_FAX_T38_TX_ON))
         {
            /* transmission stopped, so set error and raise exception */
            /* Fill event structure. */
            memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
            tapiEvent.id = IFX_TAPI_EVENT_T38_ERROR_OVLD;
            tapiEvent.module = IFX_TAPI_MODULE_TYPE_COD;
            IFX_TAPI_Event_Dispatch(pChannel,&tapiEvent);
            TRACE(VMMC,DBG_LEVEL_NORMAL,
                  ("VIN%d, IRQ: MIPS overload, Ch%d: FAX stopped\n",
                  pDev->nDevNr, nCh));
         }
      }
}
#endif
      memset(&mipsOvld, 0, sizeof(SYS_OLOAD_ACK_t));
      mipsOvld.CMD = CMD_EOP;
      mipsOvld.MOD = MOD_SYSTEM;
      mipsOvld.ECMD = SYS_OLOAD_ACK_ECMD;
      mipsOvld.LENGTH = SYS_OLOAD_ACK_LEN;
      mipsOvld.MIPS_OL = SYS_OLOAD_MIPS_OL_ACK;

      CmdWriteIsr (pDev, (IFX_uint32_t*)&mipsOvld, mipsOvld.LENGTH);
      VMMC_EVAL_MOL(pDev);
   }

   if(pAd0reg->fld.cmd_err)
   {
      SYS_CERR_ACK_t cmdErrAck;
      memset(&cmdErrAck, 0, sizeof(SYS_CERR_ACK_t));
      cmdErrAck.MOD  = MOD_SYSTEM;
      cmdErrAck.CMD  = CMD_EOP;
      /* all other fields are 0, i.e. above memset is sufficient */
      /* cmdErrAck.ECMD = 0; */
      CmdWriteIsr(pDev, (IFX_uint32_t *) &cmdErrAck, 0);

      SET_DEV_ERROR(VMMC_ERR_CERR);
      pDev->bCmdReadError = IFX_TRUE;
      /* wake up sleeping process for further processing of received command */
      pDev->bCmdOutBoxData = IFX_TRUE;
      VMMC_OS_EventWakeUp (&pDev->mpsCmdWakeUp);

      /* call event dispatcher */
      memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
      tapiEvent.id = IFX_TAPI_EVENT_DEBUG_CERR;
      IFX_TAPI_Event_Dispatch(pDev->pChannel[0].pTapiCh, &tapiEvent);
   }

   if(pAd0reg->fld.wd_fail)
   {
      TRACE(VMMC, DBG_LEVEL_HIGH,
            ("VMMC%d: IRQ: EDSP watchdog failure\n", pDev->nDevNr));
      memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
      tapiEvent.id = IFX_TAPI_EVENT_FAULT_FW_WATCHDOG;
      IFX_TAPI_Event_Dispatch(pDev->pChannel[0].pTapiCh, &tapiEvent);
   }

   if(pAd0reg->fld.pcm_crash)
   {
      TRACE (VMMC, DBG_LEVEL_HIGH,("INT: PCM crash!\n"));
   }

#ifdef VMMC_FEAT_HDLC
   if(pAd0reg->fld.dd_mbx)
   {
      irq_VMMC_RES_HDLC_DD_MBX_Handler (pDev);
   }
#endif /* VMMC_FEAT_HDLC */

}


/**
   Handler for upstream data packets (voice, fax and CID data)

   \param  mpsChannel   MPS driver channel number.
*/
static IFX_void_t VMMC_Upstream_Callback(mps_devices mpsChannel)
{
   IFX_int32_t ret = IFX_ERROR;
   IFX_uint16_t *pPacket = NULL;
#if defined(VMMC_FEAT_KPI) || defined(TAPI_CID)
   IFX_boolean_t dropPacket = IFX_FALSE,
                 processed = IFX_FALSE;
#endif /* VMMC_FEAT_KPI || TAPI_CID */
   static IFX_uint16_t nFailCnt = 0;
   VMMC_DEVICE *pDev;

   VMMC_GetDevice (0, &pDev);

   if (mpsChannel == command)
   {
      /* wake up sleeping process for further processing of received command */
      pDev->bCmdOutBoxData = IFX_TRUE;
      VMMC_OS_EventWakeUp (&pDev->mpsCmdWakeUp);
   }
#ifdef VMMC_FEAT_PACKET
   else
   {
      VMMC_CHANNEL *pCh;
      IFX_uint8_t nCh = 0;
      mps_message rw;

      ret = ifx_mps_read_mailbox(mpsChannel, &rw);
      pPacket = (IFX_uint16_t *)KSEG0ADDR((IFX_int32_t)rw.pData);

      /* For mps driver, voice0 = 2
                         voice1 = 3
                         voice2 = 4
                         voice3 = 5
                         voice4 = 6.
                         voice5 = 7.
         It must be changed to TAPI channel number.
         (ch0 = 0, ch1 = 1 and so on.)
      */
      nCh = (IFX_uint8_t) mpsChannel - 2;

      /*if (rw.cmd_type == DAT_PAYL_PTR_MSG_HDLC_PACKET)*/
      LOG_RD_PKT((pDev->nDevNr), nCh, pPacket, rw.nDataBytes,
                 !ret ? ret : pDev->err);

      pCh = &pDev->pChannel[nCh];
      if ((pCh->pCOD == IFX_NULL || pCh->pCOD->fw_cod_ch_speech.ENC == 0) &&
          rw.cmd_type <= DAT_PAYL_PTR_MSG_EVENT_PACKET)
      {
         /* discard voice and event packets if coder is not running */
         IFX_TAPI_VoiceBufferPut((void *)pPacket);
         IFX_TAPI_Stat_Add(pCh->pTapiCh, IFX_TAPI_STREAM_COD,
                           TAPI_STAT_COUNTER_EGRESS_DISCARDED, 1);
         return;
      }

#ifdef TAPI_CID
      /* handle CID packet */
      if (rw.cmd_type == DAT_PAYL_PTR_MSG_CID_DATA_PACKET)
      {
         /* process cid packet */
         irq_VMMC_SIG_CID_RX_Data_Collect (pCh->pTapiCh, pPacket, rw.nDataBytes);
         /* discard cid packet now that we processed it */
         dropPacket = IFX_TRUE;
         processed = IFX_TRUE;
      }
#endif /* TAPI_CID */

#ifdef VMMC_FEAT_KPI
      /* check if voice packets needs redirection to KPI */
      if ((rw.cmd_type == DAT_PAYL_PTR_MSG_VOICE_PACKET) ||
          (rw.cmd_type == DAT_PAYL_PTR_MSG_EVENT_PACKET) ||
          (rw.cmd_type == DAT_PAYL_PTR_MSG_FAX_T38_PACKET) )
      {
         /* if the KPI group and channel number is 0 packets will take the
            normal route to the application through the fifo below - the
            processed flag is not set in this case
            if the KPI channel number is set packets will be put into the
            KPI group fifo stored in the stream switch */
         if (IFX_TAPI_KPI_ChGet(pCh->pTapiCh, IFX_TAPI_KPI_STREAM_COD))
         {
            if ( irq_IFX_TAPI_KPI_PutToEgress(pCh->pTapiCh,
                                              IFX_TAPI_KPI_STREAM_COD,
                                              pPacket, rw.nDataBytes)
                 != IFX_SUCCESS )
            {
               dropPacket = IFX_TRUE;
               /* update statistic */
               IFX_TAPI_Stat_Add(pCh->pTapiCh, IFX_TAPI_STREAM_COD,
                                 TAPI_STAT_COUNTER_EGRESS_DISCARDED, 1);
            }
            processed = IFX_TRUE;
         }
      }
      else
      /* check if DECT packets need redirection to KPI */
      if (rw.cmd_type == DAT_PAYL_PTR_MSG_DECT_PACKET)
      {
         if (IFX_TAPI_KPI_ChGet(pCh->pTapiCh, IFX_TAPI_KPI_STREAM_DECT))
         {
            if (irq_IFX_TAPI_KPI_PutToEgress(
                   pCh->pTapiCh, IFX_TAPI_KPI_STREAM_DECT,
                   pPacket, rw.nDataBytes) != IFX_SUCCESS)
            {
               dropPacket = IFX_TRUE;
               /* update statistic */
               IFX_TAPI_Stat_Add(pCh->pTapiCh, IFX_TAPI_STREAM_DECT,
                                 TAPI_STAT_COUNTER_EGRESS_DISCARDED, 1);
            }
         }
         else
         {
            /* if the KPI group and channel number is 0 discard DECT packets */
            dropPacket = IFX_TRUE;
            /* update statistic */
            IFX_TAPI_Stat_Add(pCh->pTapiCh, IFX_TAPI_STREAM_DECT,
                              TAPI_STAT_COUNTER_EGRESS_DISCARDED, 1);
         }
         processed = IFX_TRUE;
      }
#ifdef VMMC_FEAT_HDLC
      else
      /* check if HDLC packets need redirection to KPI */
      if (rw.cmd_type == DAT_PAYL_PTR_MSG_HDLC_PACKET)
      {
         /* if the KPI group and channel number is 0 packets will take the
            normal route to the application through the fifo below - the
            processed flag is not set in this case
            if the KPI channel number is set packets will be put into the
            KPI group fifo stored in the stream switch */
         if (IFX_TAPI_KPI_ChGet(pCh->pTapiCh, IFX_TAPI_KPI_STREAM_HDLC))
         {
            if (irq_IFX_TAPI_KPI_PutToEgress (pCh->pTapiCh,
               IFX_TAPI_KPI_STREAM_HDLC, pPacket, rw.nDataBytes) != IFX_SUCCESS)
            {
               dropPacket = IFX_TRUE;
               /* update statistic */
               IFX_TAPI_Stat_Add(pCh->pTapiCh, IFX_TAPI_STREAM_HDLC,
                                 TAPI_STAT_COUNTER_EGRESS_DISCARDED, 1);
            }
            pCh->nNoKpiPathError = 0;
         }
         else
         {
            if (0 == (pCh->nNoKpiPathError % 100))
            {
               IFX_TAPI_EVENT_t tapiEvent;

               memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
               tapiEvent.ch = pCh->nChannel - 1;
               tapiEvent.id = IFX_TAPI_EVENT_FAULT_HDLC_NO_KPI_PATH;

               IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
            }

            pCh->nNoKpiPathError++;

            dropPacket = IFX_TRUE;
            /* update statistic */
            IFX_TAPI_Stat_Add(pCh->pTapiCh, IFX_TAPI_STREAM_HDLC,
                              TAPI_STAT_COUNTER_EGRESS_DISCARDED, 1);
         }
         processed = IFX_TRUE;
      }
#endif /* VMMC_FEAT_HDLC */
#endif /* VMMC_FEAT_KPI */

#if defined(VMMC_FEAT_KPI) || defined(TAPI_CID)
      /* if flag is set drop the data packet and exit from the processing */
      if (dropPacket == IFX_TRUE)
      {
         IFX_TAPI_VoiceBufferPut((void *)pPacket);
         return;
      }
      if (processed == IFX_TRUE)
      {
         return;
      }
#endif /* VMMC_FEAT_KPI || TAPI_CID */

      /* sort the packets into the fifos towards application */

      if ((rw.cmd_type == DAT_PAYL_PTR_MSG_VOICE_PACKET) ||
          (rw.cmd_type == DAT_PAYL_PTR_MSG_EVENT_PACKET) ||
          (rw.cmd_type == DAT_PAYL_PTR_MSG_FAX_DATA_PACKET) ||
          (rw.cmd_type == DAT_PAYL_PTR_MSG_FAX_STATUS_PACKET) )
      {
         ret = IFX_TAPI_UpStreamFifo_Put(pCh->pTapiCh, IFX_TAPI_STREAM_COD,
                                        (IFX_void_t *)pPacket, rw.nDataBytes, 0);

         if (!(pCh->pTapiCh->nFlags & CF_NONBLOCK))
         {
            /* data available, wake up waiting upstream function */
            VMMC_OS_EventWakeUp (&pCh->pTapiCh->semReadBlock);
         }
         /* if a non-blocking read should be performed just wake up once */
         if (pCh->pTapiCh->nFlags & CF_NEED_WAKEUP)
         {
            pCh->pTapiCh->nFlags |= CF_WAKEUPSRC_STREAM;
            /* don't wake up any more */
            pCh->pTapiCh->nFlags &= ~CF_NEED_WAKEUP;
            VMMC_OS_DrvSelectQueueWakeUp (&pCh->pTapiCh->wqRead,
                                          VMMC_OS_DRV_SEL_WAKEUP_TYPE_RD);
         }

         if (ret != IFX_SUCCESS)
         {
            /* update statistic */
            IFX_TAPI_Stat_Add(pCh->pTapiCh, IFX_TAPI_STREAM_COD,
                              TAPI_STAT_COUNTER_EGRESS_CONGESTED, 1);
         }
      }

      if (IFX_ERROR == ret)
      {
         IFX_TAPI_VoiceBufferPut((void *)pPacket);
         if (nFailCnt == 0)
         {
            TRACE (VMMC, DBG_LEVEL_HIGH,
                  ("INFO: Upstream packet fifo full[ch(%d)]!\n",
                  (pCh->nChannel-1)));
         }
         nFailCnt += 1;
         nFailCnt %= 500;
      }
      else
      {
         nFailCnt = 0;
      }
   }
#endif /* VMMC_FEAT_PACKET */
}


/**
   Event Mailbox Callback routine and Event Handler

   \param  cookie       32bit value that is a reference to the device struct.
   \param  pEvtMsg      Pointer to a single event message. The length can be
                        retrieved from the event message header.
*/
static IFX_void_t VMMC_EvtMbx_Callback(u32 cookie, mps_event_msg *pEvtMsg)
{
   VMMC_DEVICE     *pDev = (VMMC_DEVICE *) cookie;
   VMMC_EvtMsg_t   *pEvt = /*lint --e(826)*/ (VMMC_EvtMsg_t *) pEvtMsg;
   VMMC_CHANNEL    *pCh  = &pDev->pChannel[pEvt->evt_ali_ovt.CHAN];
   TAPI_CHANNEL    *pChannel = pCh->pTapiCh;
   IFX_TAPI_EVENT_t tapiEvent;

   /* prepare tapi event, channel information is the same for all events */
   memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
   tapiEvent.ch = pEvt->evt_ali_ovt.CHAN;

   LOG_RD_EVENT_MBX(pDev->nDevNr, pDev->nChannel,
                    &pEvt->val[0],
                    /* 0 - only CMD header (32 bit), 4 - 32 bits are following */
                    pEvt->evt_ali_ovt.LENGTH);

   switch (pEvt->val[0] & VMMC_EVT_ID_MASK)
   {
      /* ALI Overtemperature ************************************************ */
      case VMMC_EVT_ID_ALI_OVT:
         if (pCh->pALM != IFX_NULL)
         {
            irq_VMMC_ALM_LineDisable (&pDev->pChannel[pEvt->evt_ali_ovt.CHAN]);
         }

         tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP;
         IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);

         TRACE(VMMC, DBG_LEVEL_HIGH,
              ("Vmmc%d, Ch%d: IRQ: WARNING: overtemperature,"
               " setting line to power down\n",
               pDev->nDevNr, (pCh->nChannel -1) ));
         VMMC_EVAL_EVENT(pCh, tapiEvent);
         break;

      /* ALI RAW Hook event ************************************************* */
      case VMMC_EVT_ID_ALI_RAW_HOOK:
         if (pEvt->evt_ali_raw_hook.RON == EVT_ALI_RAW_HOOK_RON_ONHOOK)
            tapiEvent.id = IFX_TAPI_EVENT_FXS_ONHOOK_INT;
         else
            tapiEvent.id = IFX_TAPI_EVENT_FXS_OFFHOOK_INT;
         tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
         tapiEvent.data.hook_int.nTime = VMMC_ALM_ElapsedTimeSinceLastHook(pCh);
         IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
         VMMC_EVAL_EVENT(pCh, tapiEvent);
         break;

      /* ALI LT END event *************************************************** */
      case VMMC_EVT_ID_ALI_LT_END:
         if (pCh->pALM != IFX_NULL)
         {
#ifdef VMMC_FEAT_CAP_MEASURE
            if (pCh->pALM->eCapMeasState == VMMC_CapMeasState_Started)
            {
               tapiEvent.id = IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY_INT;
            }
            else
#endif /* VMMC_FEAT_CAP_MEASURE */
            {
               tapiEvent.id = IFX_TAPI_EVENT_LT_GR909_RDY;
            }
            tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
         }
         VMMC_EVAL_EVENT(pCh, tapiEvent);
         break;

      /* ALI AR9DCC event *************************************************** */
      case VMMC_EVT_ID_ALI_AR9DCC:
         /* Ground fault is most important so we handle it first */
         if (pEvt->evt_ali_ar9dcc.EVT == EVT_ALI_AR9DCC_EVT_GF)
         {
            if (pCh->pALM != IFX_NULL)
            {
               /* Set the linemode to disabled. If the DCCtrl already did it
                  this disabled is the confirmation required by the DCCtrl.
                  If the DCCtrl did nothing this will switch off the line.
                  In both cases the line is then disabled and may be set to
                  other line states by the application. */
               irq_VMMC_ALM_LineDisable(&pDev->pChannel[pEvt->evt_ali_ar9dcc.CHAN]);
            }
            /* Inform the application through a TAPI event. */
            tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH;
            tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
         }
         else
         /* Ground key is second in importance */
         if (pEvt->evt_ali_ar9dcc.EVT == EVT_ALI_AR9DCC_EVT_GK)
         {
            /* Upon ground key we do nothing. Just a TAPI event is sent to
               the application. */
            tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_GK_LOW;
            tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
         }
         else
         /* Ground fault end */
         if (pEvt->evt_ali_ar9dcc.EVT == EVT_ALI_AR9DCC_EVT_GF_FIN)
         {
            tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH_END;
            tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
         }
         else
         /* Ground key end */
         if (pEvt->evt_ali_ar9dcc.EVT == EVT_ALI_AR9DCC_EVT_GK_FIN)
         {
            tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_END;
            tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
         }
         else
         /* Overtemperature end */
         if (pEvt->evt_ali_ar9dcc.EVT == EVT_ALI_AR9DCC_EVT_OTEMP_FIN)
         {
            tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP_END;
            tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
         }
#ifdef PMC_SUPPORTED
         else
         /* DART in sleep notification */
         if (pEvt->evt_ali_ar9dcc.EVT == EVT_ALI_AR9DCC_EVT_DART_IN_SLEEP)
         {
            /* Report only the request from channel 0. */
            if ((pEvt->evt_ali_ar9dcc.CHAN == 0) && (pCh->pALM != IFX_NULL))
            {
               irq_VMMC_PMC_DartInSleepEvent(pDev);
            }
         }
         else
         /* DART wakeup request */
         if (pEvt->evt_ali_ar9dcc.EVT == EVT_ALI_AR9DCC_EVT_DART_WAKEUP_REQ)
         {
            /* Report only the request from channel 0. */
            if ((pEvt->evt_ali_ar9dcc.CHAN == 0) && (pCh->pALM != IFX_NULL))
            {
               irq_VMMC_PMC_DartWakeupReqEvent(pDev);
            }
         }
#endif /* PMC_SUPPORTED */
         else
         /* OPMODE changes */
         if ((pEvt->evt_ali_ar9dcc.EVT == EVT_ALI_AR9DCC_EVT_OPC) &&
             (pCh->pALM != IFX_NULL))
         {
            irq_VMMC_ALM_UpdateOpModeAndWakeUp (pCh,
                                                pEvt->evt_ali_ar9dcc.OPMODE);

            /* To detect end of calibration this code looks for a transition
               from opmode calibration to any other opmode. */
            if (pEvt->evt_ali_ar9dcc.OPMODE == VMMC_SDD_OPMODE_CALIBRATE)
            {
               /* Set flag that now calibration is running. The check below
                  will use this flag to find the end of calibration. */
               pCh->pALM->bCalibrationRunning = IFX_TRUE;
            }
            if ((pCh->pALM->bCalibrationRunning == IFX_TRUE) &&
                (pEvt->evt_ali_ar9dcc.OPMODE != VMMC_SDD_OPMODE_CALIBRATE))
            {
               pCh->pALM->bCalibrationRunning = IFX_FALSE;
               tapiEvent.id = IFX_TAPI_EVENT_CALIBRATION_END_INT;
               tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
               IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
            }
         }
         else
         /* OPMODE change ignored */
         if ((pEvt->evt_ali_ar9dcc.EVT == EVT_ALI_AR9DCC_EVT_OMI) &&
             (pCh->pALM != IFX_NULL))
         {
            irq_VMMC_ALM_UpdateOpModeAndWakeUp (pCh, OPMODE_IGNORED);
         }
         else
         /* SSI crash is unlikely to occur and interrupts the service.
            No fast response is possible or needed so handle it last. */
         if (pEvt->evt_ali_ar9dcc.EVT == EVT_ALI_AR9DCC_EVT_SSI_CRASH)
         {
            /* Report only the first occurance from any of the channels. */
            if (pDev->bSSIcrash == IFX_FALSE)
            {
               /* Mark in the device struct that SSI has crashed. */
               pDev->bSSIcrash = IFX_TRUE;

               /* Send an TAPI event to the application. */
               tapiEvent.id = IFX_TAPI_EVENT_FAULT_HW_SSI_ERROR_INT;
               IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
            }
         }
         else
         /* SSI has been recovered from the crash. */
         if (pEvt->evt_ali_ar9dcc.EVT == EVT_ALI_AR9DCC_EVT_SSI_CRASH_FIN)
         {
            /* Report only the first occurance from any of the channels. */
            if (pDev->bSSIcrash == IFX_TRUE)
            {
               /* Mark in the device struct that SSI has crashed. */
               pDev->bSSIcrash = IFX_FALSE;

               /* Send an TAPI event to the application. */
               tapiEvent.id = IFX_TAPI_EVENT_FAULT_HW_SSI_FIXED_INT;
               IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
            }
         }
#ifdef VMMC_FEAT_METERING
         else
         /* Metering pulse finished */
         if (pEvt->evt_ali_ar9dcc.EVT == EVT_ALI_AR9DCC_EVT_TTXF)
         {
            tapiEvent.id = IFX_TAPI_EVENT_METERING_END;
            tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
         }
#endif /* VMMC_FEAT_METERING */
         /* Note: other events reported with this message are not handled
            at the moment. */
         break;

      /* COD Decoder Change event ******************************************* */
      case VMMC_EVT_ID_COD_DEC_CHANGE:
         /* Fill event structure; acknowledge of this interrupt will be
            triggered by the event dispatcher. */
         if (pEvt->evt_cod_dec_change.DC)
         {
            tapiEvent.id = IFX_TAPI_EVENT_COD_DEC_CHG;
            tapiEvent.module = IFX_TAPI_MODULE_TYPE_COD;
            tapiEvent.data.dec_chg.dec_type = VMMC_COD_trans_cod_fw2tapi(
                                              pEvt->evt_cod_dec_change.DEC,
                                              pCh->pParent->caps.bAMRE);
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
            /* additional TAPI event for AMR */
            if (pEvt->evt_cod_dec_change.CMR &&
                AMR_CODEC(pEvt->evt_cod_dec_change.DEC, pCh->pParent->caps.bAMRE))
            {
               VMMC_COD_CmrDec_Update (pCh, pEvt->evt_cod_dec_change.CMRC);
               tapiEvent.id = IFX_TAPI_EVENT_COD_DEC_CMR;
               tapiEvent.module = IFX_TAPI_MODULE_TYPE_COD;
               tapiEvent.data.cmr = (IFX_TAPI_EVENT_DATA_DEC_CMR_t)pEvt->evt_cod_dec_change.CMRC;
               IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
            }
         }
         break;

      /* COD Fax Data Pump event ******************************************** */
      case VMMC_EVT_ID_COD_FAX_REQ:
#ifdef VMMC_FEAT_FAX_T38
         if (pCh->TapiFaxStatus.nStatus & IFX_TAPI_FAX_T38_DP_ON)
         {
            IFX_boolean_t bReportEvt = IFX_TRUE;
            switch (pEvt->evt_cod_fax_req.FDP_EVT)
            {
               case EVT_COD_FAX_REQ_FDP_EVT_FDP_REQ:
                  pCh->pTapiCh->bFaxDataRequest = IFX_TRUE;
                  VMMC_OS_DrvSelectQueueWakeUp (&pChannel->wqWrite,
                                                VMMC_OS_DRV_SEL_WAKEUP_TYPE_WR);
                  pCh->nFdpReq++;
                  bReportEvt = IFX_FALSE;
                  break;
               case EVT_COD_FAX_REQ_FDP_EVT_MBSU:
                  tapiEvent.id = IFX_TAPI_EVENT_T38_ERROR_DATA;
                  tapiEvent.module = IFX_TAPI_MODULE_TYPE_COD;
                  tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_DATA_MBSU;
                  break;
               case EVT_COD_FAX_REQ_FDP_EVT_DBSO:
                  tapiEvent.id = IFX_TAPI_EVENT_T38_ERROR_DATA;
                  tapiEvent.module = IFX_TAPI_MODULE_TYPE_COD;
                  tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_DATA_DBSO;
                  break;
               case EVT_COD_FAX_REQ_FDP_EVT_MBDO:
                  tapiEvent.id = IFX_TAPI_EVENT_T38_ERROR_DATA;
                  tapiEvent.module = IFX_TAPI_MODULE_TYPE_COD;
                  tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_DATA_MBDO;
                  break;
               case EVT_COD_FAX_REQ_FDP_EVT_MBDU:
                  tapiEvent.id = IFX_TAPI_EVENT_T38_ERROR_DATA;
                  tapiEvent.module = IFX_TAPI_MODULE_TYPE_COD;
                  tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_DATA_MBDU;
                  break;
               case EVT_COD_FAX_REQ_FDP_EVT_DBDO:
                  tapiEvent.id = IFX_TAPI_EVENT_T38_ERROR_DATA;
                  tapiEvent.module = IFX_TAPI_MODULE_TYPE_COD;
                  tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_DATA_DBDO;
                  break;
               default:
                  /* unknown event type */
                  break;
            }
            if (bReportEvt)
            {
               IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
            }
         }
#endif /* VMMC_FEAT_FAX_T38 */
         break;

      /* SIG UTG1 event ***************************************************** */
      case VMMC_EVT_ID_SIG_UTG1:
         if (TAPI_ToneState (pCh->pTapiCh, 0) == TAPI_CT_ACTIVE)
         {
            tapiEvent.id = IFX_TAPI_EVENT_TONE_GEN_END_RAW;
            /* value stores the utg resource number */
            tapiEvent.data.value = 0;
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
         }
         break;

      /* SIG UTG2 event ***************************************************** */
      case VMMC_EVT_ID_SIG_UTG2:
         if (TAPI_ToneState (pCh->pTapiCh, 1) == TAPI_CT_ACTIVE)
         {
            tapiEvent.id = IFX_TAPI_EVENT_TONE_GEN_END_RAW;
            /* value stores the utg resource number */
            tapiEvent.data.value = 1;
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
         }
         break;

      /* DECT UTG event ***************************************************** */
      case VMMC_EVT_ID_DECT_UTG:
         if (TAPI_ToneState (pCh->pTapiCh, 2) == TAPI_CT_ACTIVE)
         {
            tapiEvent.id = IFX_TAPI_EVENT_TONE_GEN_END_RAW;
            tapiEvent.module = IFX_TAPI_MODULE_TYPE_DECT;
            /* value stores the utg resource number */
            tapiEvent.data.value = 2;
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
         }
         break;

      /* SIG DTMF Generator event ******************************************* */
      case VMMC_EVT_ID_SIG_DTMFG:
         if (pCh->pSIG != IFX_NULL)
         {
            switch (pEvt->evt_sig_dtmfg.EVENT)
            {
               case EVT_SIG_DTMFG_EVENT_READY:
                  irq_VMMC_SIG_DtmfOnReady(pCh);
                  break;
               case EVT_SIG_DTMFG_EVENT_BUF_REQUEST:
                  irq_VMMC_SIG_DtmfOnRequest(pCh);
                  break;
               case EVT_SIG_DTMFG_EVENT_BUF_UNDERFLOW:
                  irq_VMMC_SIG_DtmfOnUnderrun(pCh);
                  break;
               default:
                  break;
            }
         }
         break;

      /* SIG CID sender event *********************************************** */
      case VMMC_EVT_ID_SIG_CIDS:
#ifdef TAPI_CID
         switch (pEvt->evt_sig_cids.EVENT)
         {
            case EVT_SIG_CIDS_EVENT_READY:
               /* CID end of transmission and CIS disabled
                  via autodeactivation if not offhook or error. */
               VMMC_CidFskMachine(pCh, VMMC_CID_ACTION_STOP);
               /* send event to inform that the data transmission has ended */
               memset(&tapiEvent, 0, sizeof(tapiEvent));
               tapiEvent.id = IFX_TAPI_EVENT_CID_TX_END;
               IFX_TAPI_Event_Dispatch(pChannel,&tapiEvent);
               break;

            case EVT_SIG_CIDS_EVENT_BUF_REQUEST:
               VMMC_CidFskMachine(pCh, VMMC_CID_ACTION_REQ_DATA);
               break;

            case EVT_SIG_CIDS_EVENT_BUF_UNDERFLOW:
               VMMC_CidFskMachine(pCh, VMMC_CID_ACTION_STOP);
               /* send event to inform about this error */
               memset(&tapiEvent, 0, sizeof(tapiEvent));
               tapiEvent.id = IFX_TAPI_EVENT_CID_TX_UNDERRUN_ERR;
               IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
               break;

            default:
               break;
         }
#endif /* TAPI_CID */
         break;

      /* SIG DTMF detector event ******************************************** */
      case VMMC_EVT_ID_SIG_DTMFD:
         if ((IFX_uint32_t)(pCh->nChannel - 1) < pDev->caps.nDTMFD)
         {
            /* separate DTMF_START events from DTMF_END events */
            if (pEvt->evt_sig_dtmfd.EVT == EVT_SIG_DTMFD_EVT_DTMF_START)
            {
               tapiEvent.id = IFX_TAPI_EVENT_DTMF_DIGIT;
               /* translate fw-coding to tapi enum coding */
               tapiEvent.data.dtmf.digit =
                  irq_VMMC_SIG_DTMF_encode_fw2tapi(pEvt->evt_sig_dtmfd.DTMF);
               /* translate fw-coding to ascii charater */
               tapiEvent.data.dtmf.ascii =
                  (IFX_uint8_t)irq_VMMC_SIG_DTMF_encode_fw2ascii(
                     pEvt->evt_sig_dtmfd.DTMF);
               /* Remember the DTMF sign for DTMF end reporting below */
               if (pCh->pSIG != NULL)
               {
                  pCh->pSIG->nLastDtmfSign = pEvt->evt_sig_dtmfd.DTMF;
               }
            }
            else
            {
               tapiEvent.id = IFX_TAPI_EVENT_DTMF_END;
               /* The DTMF end event carries no DTMF sign so repeat the last
                  sign that was detected. If the variable does not exist the
                  event reports 0x00 which means "no digit". Because of this
                  keep the assignment to the event within this if-statement. */
               if (pCh->pSIG != NULL)
               {
                  pEvt->evt_sig_dtmfd.DTMF = pCh->pSIG->nLastDtmfSign;
                  /* translate fw-coding to tapi enum coding */
                  tapiEvent.data.dtmf.digit =
                     irq_VMMC_SIG_DTMF_encode_fw2tapi(pEvt->evt_sig_dtmfd.DTMF);
                  /* translate fw-coding to ascii charater */
                  tapiEvent.data.dtmf.ascii =
                     (IFX_uint8_t)irq_VMMC_SIG_DTMF_encode_fw2ascii(
                        pEvt->evt_sig_dtmfd.DTMF);
               }
            }

            if (pEvt->evt_sig_dtmfd.I == EVT_SIG_DTMFD_I_I1)
            {
               /* SIG1 of signalling channel is input to the signalling
                  channel, which is by convention the output of the
                  respective ALM channel */
               tapiEvent.data.dtmf.local = 1;
            }
            else
            {
               /* SIG2 of signalling channel is input to the signalling
                  channel which is by convention the output of the
                  respective COD channel */
               tapiEvent.data.dtmf.network = 1;
            }
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
            VMMC_EVAL_EVENT(pCh, tapiEvent);
         }
         break;

      /* SIG CPTD events **************************************************** */
      case VMMC_EVT_ID_SIG_CPTD:
         tapiEvent.id = IFX_TAPI_EVENT_TONE_DET_CPT;
         tapiEvent.data.tone_det.index =
            VMMC_SIG_CPTD_ToneFromEvtGet(pCh, &pEvt->evt_sig_cptd);

         if (pEvt->evt_sig_cptd.I == EVT_SIG_CPTD_I_I1)
         {
            /* SIG1 of signalling channel is input to the signalling
               channel, which is by convention the output of the
               respective ALM channel */
            tapiEvent.data.tone_det.local = 1;
         }
         else
         if (pEvt->evt_sig_cptd.I == EVT_SIG_CPTD_I_I2)
         {
            /* SIG2 of signalling channel is input to the signalling
               channel which is by convention the output of the
               respective COD channel */
            tapiEvent.data.tone_det.network = 1;
         }
         else
         if (pEvt->evt_sig_cptd.I == EVT_SIG_CPTD_I_I12)
         {
            tapiEvent.data.tone_det.local = 1;
            tapiEvent.data.tone_det.network = 1;
         }

         IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
         break;

      /* SIG MFTD events **************************************************** */
      case VMMC_EVT_ID_SIG_MFTD:
         if ((pEvt->evt_sig_mftd.I1) && (pCh->pSIG != NULL))
            irq_VMMC_SIG_MFTD_Event (pCh, pEvt->evt_sig_mftd.MFTD1, IFX_FALSE);
         if ((pEvt->evt_sig_mftd.I2) && (pCh->pSIG != NULL))
            irq_VMMC_SIG_MFTD_Event (pCh, pEvt->evt_sig_mftd.MFTD2, IFX_TRUE);
         break;

      /* SIG RFC2833 event ************************************************** */
      case VMMC_EVT_ID_SIG_RFC2833DET:
         tapiEvent.id = IFX_TAPI_EVENT_RFC2833_EVENT;
         tapiEvent.module = IFX_TAPI_MODULE_TYPE_COD;
         tapiEvent.data.rfc2833.event = (IFX_TAPI_PKT_EV_NUM_t) pEvt->evt_sig_rfcdet.EVT;
         IFX_TAPI_Event_Dispatch(pChannel,&tapiEvent);
         break;

#ifdef VMMC_FEAT_FAX_T38_FW
      /* FAX event ********************************************************** */
      case VMMC_EVT_ID_COD_FAX_STATE:
         tapiEvent.id = IFX_TAPI_EVENT_T38_STATE_CHANGE;
         tapiEvent.module = IFX_TAPI_MODULE_TYPE_COD;
         switch (pEvt->evt_cod_fax_state.T38_FAX_STATE)
         {
            case EVT_COD_FAX_STATE_ST_NEG:
               tapiEvent.data.t38 = IFX_TAPI_EVENT_T38_NEG;
               break;
            case EVT_COD_FAX_STATE_ST_MOD:
               tapiEvent.data.t38 = IFX_TAPI_EVENT_MOD;
               break;
            case EVT_COD_FAX_STATE_ST_DEM:
               tapiEvent.data.t38 = IFX_TAPI_EVENT_DEM;
               break;
            case EVT_COD_FAX_STATE_ST_TRANS:
               tapiEvent.data.t38 = IFX_TAPI_EVENT_TRANS;
               break;
            case EVT_COD_FAX_STATE_ST_PP:
               tapiEvent.data.t38 = IFX_TAPI_EVENT_PP;
               break;
            case EVT_COD_FAX_STATE_ST_INT:
               tapiEvent.data.t38 = IFX_TAPI_EVENT_INT;
               break;
            case EVT_COD_FAX_STATE_ST_DCN:
               tapiEvent.data.t38 = IFX_TAPI_EVENT_DCN;
               break;
            default:
               /* TODO */
               break;
         }
         IFX_TAPI_Event_Dispatch(pChannel,&tapiEvent);
    break;

      /* FAX channel event ************************************************** */
      case VMMC_EVT_ID_COD_FAX_ERR_EVT:
         tapiEvent.id = IFX_TAPI_EVENT_T38_ERROR_DATA;
         tapiEvent.module = IFX_TAPI_MODULE_TYPE_COD;
         switch (pEvt->evt_cod_fax_err.FOIP_ERR)
         {
            case EVT_COD_FAX_ERR_MBSU:
               tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_DATA_MBSU;
               break;
            case EVT_COD_FAX_ERR_DBSO:
               tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_DATA_DBSO;
               break;
            case EVT_COD_FAX_ERR_MBDO:
               tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_DATA_MBDO;
               break;
            case EVT_COD_FAX_ERR_MBDU:
               tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_DATA_MBDU;
               break;
            case EVT_COD_FAX_ERR_DBDO:
               tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_DATA_DBDO;
               break;
            case EVT_COD_FAX_ERR_WPIP:
               tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_IP_PACKET;
               break;
            case EVT_COD_FAX_ERR_CBO:
               tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_CB_OVERFLOW;
               break;
            case EVT_COD_FAX_ERR_DBO:
               tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_DB_OVERFLOW;
               break;
            case EVT_COD_FAX_ERR_WRC:
               tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_BAD_CMD;
               break;
            case EVT_COD_FAX_ERR_AF1:
               tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_SESS_START;
               break;
            case EVT_COD_FAX_ERR_AF2:
               tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_SESS_STOP;
               break;
            case EVT_COD_FAX_ERR_TFF:
               tapiEvent.data.value = IFX_TAPI_EVENT_T38_ERROR_FLUSH_FIN;
               break;
            default:
               /* unknown event type */
               break;
         }
         IFX_TAPI_Event_Dispatch(pChannel,&tapiEvent);
         break;
#endif /* VMMC_FEAT_FAX_T38_FW */

      case VMMC_EVT_ID_COD_ANN_END:
#ifdef VMMC_FEAT_ANNOUNCEMENTS
         VMMC_AnnEndEventServe (pCh);

         tapiEvent.id = IFX_TAPI_EVENT_COD_ANNOUNCE_END;
         tapiEvent.module = IFX_TAPI_MODULE_TYPE_COD;
         tapiEvent.data.announcement.nAnnIdx = pEvt->evt_cod_ann_end.ANNID;
         IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
#endif /* VMMC_FEAT_ANNOUNCEMENTS */
         break;

      case VMMC_EVT_ID_CPD_STAT_MOS:
         if (pCh->pCOD != NULL)
         {
            tapiEvent.id = IFX_TAPI_EVENT_COD_MOS;
            tapiEvent.module = IFX_TAPI_MODULE_TYPE_COD;
            //VMMC_OS_MutexGet(&pCh->chAcc);
            tapiEvent.data.mos.nR = pCh->pCOD->fw_cod_cfg_stat_mos.R_DEF;
            tapiEvent.data.mos.nCTI = pCh->pCOD->fw_cod_cfg_stat_mos.CTI;
            tapiEvent.data.mos.nAdvantage = pCh->pCOD->fw_cod_cfg_stat_mos.A_FACT;
            //VMMC_OS_MutexRelease (&pCh->chAcc);
            tapiEvent.data.mos.nMOS = pEvt->evt_cpd_stat_mos.MOS_CQE;
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
         }
         break;

#ifdef VMMC_FEAT_HDLC
      /* HDLC events ******************************************************** */
      case VMMC_EVT_ID_PCM_HDLC_RDY:
         if (pEvt->evt_pcm_hdlc.TE)
         {
            irq_VMMC_PCM_HDLC_BufferReadySet(pCh);
         }
         if (pEvt->evt_pcm_hdlc.TO)
         {
            tapiEvent.id = IFX_TAPI_EVENT_FAULT_HDLC_TX_OVERFLOW;
            IFX_TAPI_Event_Dispatch(pChannel,&tapiEvent);
         }
         break;
#endif /* VMMC_FEAT_HDLC */

      /* FXO events ********************************************************* */
      case VMMC_EVT_ID_FXO:
         if (pCh->pALM != NULL)
         {
            switch (pEvt->evt_fxo.EVENT)
            {
               case EVT_FXO_RING_ON:
                  pCh->pALM->fxo_flags |= (1 << FXO_RING);
                  tapiEvent.id = IFX_TAPI_EVENT_FXO_RING_START;
                  break;
               case EVT_FXO_RING_OFF:
                  pCh->pALM->fxo_flags &= ~(1 << FXO_RING);
                  tapiEvent.id = IFX_TAPI_EVENT_FXO_RING_STOP;
                  break;
               case EVT_FXO_BATT_ON:
                  pCh->pALM->fxo_flags |= (1 << FXO_BATTERY);
                  tapiEvent.id = IFX_TAPI_EVENT_FXO_BAT_FEEDED;
                  break;
               case EVT_FXO_BATT_OFF:
                  pCh->pALM->fxo_flags &= ~(1 << FXO_BATTERY);
                  tapiEvent.id = IFX_TAPI_EVENT_FXO_BAT_DROPPED;
                  break;
               case EVT_FXO_OSI_END:
                  tapiEvent.id = IFX_TAPI_EVENT_FXO_OSI;
                  break;
               case EVT_FXO_APOH_ON:
                  pCh->pALM->fxo_flags |= (1 << FXO_APOH);
                  tapiEvent.id = IFX_TAPI_EVENT_FXO_APOH;
                  break;
               case EVT_FXO_APOH_OFF:
                  pCh->pALM->fxo_flags &= ~(1 << FXO_APOH);
                  tapiEvent.id = IFX_TAPI_EVENT_FXO_NOPOH;
                  break;
               case EVT_FXO_POLARITY_REVERSED:
                  pCh->pALM->fxo_flags &= ~(1 << FXO_POLARITY);
                  tapiEvent.id = IFX_TAPI_EVENT_FXO_POLARITY;
                  tapiEvent.data.fxo_polarity =
                     IFX_TAPI_EVENT_DATA_FXO_POLARITY_REVERSED;
                  break;
               case EVT_FXO_POLARITY_NORMAL:
                  pCh->pALM->fxo_flags |= (1 << FXO_POLARITY);
                  tapiEvent.id = IFX_TAPI_EVENT_FXO_POLARITY;
                  tapiEvent.data.fxo_polarity =
                     IFX_TAPI_EVENT_DATA_FXO_POLARITY_NORMAL;
                  break;
               default:
                  tapiEvent.id = IFX_TAPI_EVENT_FXO_NONE;
                  break;
            }
            tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
         }
         break;

      /* the following events are _intentionally_ not handled at the moment */
      case VMMC_EVT_ID_SIG_RFC2833STAT:
         VMMC_EVAL_UNHANDLED_EVENT(pCh,
                   (VMMC_EVT_ID_t) (pEvt->val[0] & VMMC_EVT_ID_MASK));
         break;
      case VMMC_EVT_ID_COD_VPOU_LIMIT:
         VMMC_EVAL_UNHANDLED_EVENT(pCh,
                   (VMMC_EVT_ID_t) (pEvt->val[0] & VMMC_EVT_ID_MASK));
         break;
      case VMMC_EVT_ID_COD_VPOU_STAT:
         VMMC_EVAL_UNHANDLED_EVENT(pCh,
                   (VMMC_EVT_ID_t) (pEvt->val[0] & VMMC_EVT_ID_MASK));
         break;
      case VMMC_EVT_ID_SYS_INT_ERR:
         /* VMMC_EVAL_UNHANDLED_EVENT(pCh,
                   (VMMC_EVT_ID_t) (pEvt->val[0] & VMMC_EVT_ID_MASK)); */
         break;
      case VMMC_EVT_ID_COD_LIN_REQ:
         break;
      case VMMC_EVT_ID_COD_LIN_UNDERFLOW:
         break;
      default:
         TRACE(VMMC, DBG_LEVEL_HIGH,
              ("VMMC unknown event id 0x%08X\n", pEvt->val[0]));
   }
}


#ifdef VMMC_FEAT_VPE1_SW_WD
/**
   Watchdog Timer callback routine. Called by MPS driver.
   \param flags    Zero for the moment. Reserved for future use.

   \return         success, cannot fail.
*/
IFX_int32_t VMMC_WDT_Callback (IFX_uint32_t flags)
{
   VMMC_DEVICE     *pDev;
   VMMC_CHANNEL    *pCh;
   TAPI_CHANNEL    *pChannel;
   IFX_TAPI_EVENT_t tapiEvent;

   VMMC_UNUSED(flags);

   VMMC_GetDevice (0, &pDev);

   /* this specific event is being reported on channel 0 */
   pCh = &pDev->pChannel[0];
   pChannel = pCh->pTapiCh;

   memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
   tapiEvent.id = IFX_TAPI_EVENT_FAULT_FW_WATCHDOG;
   IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);

   return 0;
}
#endif /* VMMC_FEAT_VPE1_SW_WD */

/*
   - improvement: implement cascaded event structure in VMMC_EvtMbx_Callback

   - mbx events are currently intentionally not enabled for
      - Voice Playout Unit event
      - Voice Playout Unit statistics change
      - RFC2833 statistics change
      - Linear Channel Data request

   - DTMFG READY evt, DTMF generation has finished, report? hl tapi trigger??
     currently disabled and skipped in interrupt routine
*/
