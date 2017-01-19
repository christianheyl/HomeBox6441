/******************************************************************************

                              Copyright (c) 2013
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
/**
   \file  drv_vmmc_res.c  Implementation of functionality used by
                          multiple modules.

   This file handles functionality that is implemented as a resource in
   firmware and can be used on different modules. The functionality is:
    -Echo suppressor (can be used on ALM and PCM)
    -Line Echo Canceller (can be used on ALM and PCM)
   To be moved to this location in the future is:
    -Universal Tone Generator (can be used on SIG, DECT and AUDIO)
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_api.h"
#include "drv_vmmc_res_priv.h"
#include "drv_vmmc_res.h"

#include "drv_mps_vmmc.h"
#include "drv_vmmc_stream.h"

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* ============================= */
/* Global variable definition    */
/* ============================= */

/** Constant that identifies an empty resource id. */
const VMMC_RES_ID_t  VMMC_RES_ID_NULL = { 0, IFX_NULL };

/* Calculated table to convert the gain in 'dB' into the FW values */
/* This table is placed here because the values are shared between the
   various firmware messages and there is no implemntation file for the
   firmware messages. The export symbol for this variable is placed in
   drv_vmmc_fw_commands.h to group it with the firmware messages. */
const IFX_uint16_t VMMC_Gaintable[] =
{
   /* dB: -24;-23;-22;-21;-20;-19;-18;-17; */
   0x0204, 0x0243, 0x028A, 0x02DA, 0x0333, 0x0397, 0x0407, 0x0485,
   /* dB: -16;-15;-14;-13;-12;-11;-10;-9; */
   0x0512, 0x05B0, 0x0662, 0x072A, 0x0809, 0x0904, 0x0A1E, 0x0B5A,
   /* dB: -8;-7;-6;-5;-4;-3;-2;-1; */
   0x0CBD, 0x0E4B, 0x1009, 0x11FE, 0x1430, 0x16A7, 0x196B, 0x1C85,
   /* dB; 0 */
   0x2000,
   /* dB: +1;+2;+3;+4;+5;+6;+7;+8 */
   0x23E7, 0x2849, 0x2D33, 0x32B7, 0x38E8, 0x3FD9, 0x47A4, 0x5061,
   /* dB: +9;+10;+11;+12;+13;+14;+15;+16 */
   0x5A30, 0x6531, 0x718A, 0x7F65, 0x7F65, 0x7F65, 0x7F65, 0x7F65,
   /* dB: +17;+18;+19;+20;+21;+22;+23;+24; */
   0x7F65, 0x7F65, 0x7F65, 0x7F65, 0x7F65, 0x7F65, 0x7F65, 0x7F65
};


/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local function declaration    */
/* ============================= */

/* ============================= */
/* Local variable declaration    */
/* ============================= */

/* Default coefficient tables.
  The values here are used to initialise the resources upon allocation. */

/** ES default coefficient for ES without LEC+NLP */
static IFX_uint16_t vmmc_res_es_without_nlp[] =
{
   0x0E, 0x06, 0x12, 0x36, 0xC0, 0x20, 0xF0,
   0xC0, 0x05, 0x05, 0x00, 0x19, 0x80, 0x00,
   0x4200, 0x1900, 0x0000, 0x0010, 0x0040, 0x442A, 0x5000, 0x3500
};

/** ES default coefficient for ES together with LEC+NLP */
static IFX_uint16_t vmmc_res_es_with_nlp[] =
{
   0x0E, 0x06, 0x12, 0x51, 0xC0, 0x20, 0xF0,
   0xC0, 0x05, 0x05, 0xF7, 0x0D, 0x80, 0x43,
   0x4200, 0x1F49, 0xFC00, 0x0014, 0x0040, 0x5000, 0x5000, 0x5000
};

/* ALM default LEC-coefficients NLEC-mode narrowband */
static IFX_uint16_t vmmc_res_lec_alm_nlec_nb[] =
{
   0x10, 0x3D, 0x08, 0x08, 0x2000, 0x2000, 0x2000, 0x10, 0x34, 0x00, 0x08
};

/* ALM default LEC-coefficients WLEC-mode narrowband */
static IFX_uint16_t vmmc_res_lec_alm_wlec_nb[] =
{
   0x10, 0x3D, 0x08, 0x08, 0x2000, 0x2000, 0x2000, 0x08, 0x34, 0x00, 0x08
};

/* ALM default LEC-coefficients NLEC-mode wideband */
static IFX_uint16_t vmmc_res_lec_alm_nlec_wb[] =
{
   0x08, 0x3D, 0x08, 0x08, 0x2000, 0x2000, 0x2000, 0x08, 0x34, 0x00, 0x08
};

/* PCM default LEC-coefficients NLEC-mode narrowband */
static IFX_uint16_t vmmc_res_lec_pcm_nlec_nb[] =
{
   0x10, 0x3D, 0x08, 0x08, 0x2000, 0x2000, 0x2000, 0x10, 0x34, 0x00, 0x08
};

/* PCM default LEC-coefficients WLEC-mode narrowband */
static IFX_uint16_t vmmc_res_lec_pcm_wlec_nb[] =
{
   0x10, 0x3D, 0x08, 0x08, 0x2000, 0x2000, 0x2000, 0x08, 0x34, 0x00, 0x08
};

/* PCM default LEC-coefficients NLEC-mode wideband */
static IFX_uint16_t vmmc_res_lec_pcm_nlec_wb[] =
{
   0x08, 0x3D, 0x08, 0x08, 0x2000, 0x2000, 0x2000, 0x08, 0x34, 0x00, 0x08
};

/** ALM default NLP-Coefficients narrowband */
static IFX_uint8_t vmmc_res_lec_alm_nlp_nb[] =
{
   0x15, 0x02, 0x45, 0x40, 0x0C, 0x40, 0x41, 0xFE, 0x11,
   0x06, 0x50, 0x50, 0x0D, 0x10, 0x64, 0x80, 0x08, 0x10
};

/** ALM default NLP-Coefficients wideband */
static IFX_uint8_t vmmc_res_lec_alm_nlp_wb[] =
{
   0x0A, 0x01, 0x45, 0x40, 0x0C, 0x40, 0x41, 0xFD, 0x11,
   0x06, 0x50, 0x50, 0x0D, 0x10, 0x32, 0x40, 0x04, 0x10
};

/** PCM default NLP-Coefficients narrowband */
static IFX_uint8_t vmmc_res_lec_pcm_nlp_nb[] =
{
   0x15, 0x02, 0x45, 0x40, 0x0C, 0x40, 0x41, 0xFE, 0x11,
   0x06, 0x46, 0x47, 0x0D, 0x10, 0x64, 0x80, 0x08, 0x10
};

/** PCM default NLP-Coefficients wideband */
static IFX_uint8_t vmmc_res_lec_pcm_nlp_wb[] =
{
   0x0A, 0x01, 0x45, 0x40, 0x0C, 0x40, 0x41, 0xFD, 0x11,
   0x06, 0x46, 0x47, 0x0D, 0x10, 0x32, 0x40, 0x04, 0x10
};

/** ALM&PCM default NLP-Coefficients narrowband with Echo Suppressor */
static IFX_uint8_t vmmc_res_lec_nlp_nb_with_es[] =
{
   0x15, 0x01, 0x45, 0x40, 0x0C, 0x40, 0x41, 0xFD, 0x11,
   0x06, 0x46, 0x47, 0x0D, 0x13, 0x32, 0x08, 0x08, 0x13
};



/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Allocate data structures for housekeeping of the resources handled here.

   \param  pDev         Pointer to the VMMC device structure.

   \return
      - VMMC_statusNoMem Memory allocation failed
      - VMMC_statusOk if successful

   \remarks The device parameter is no longer checked because the calling
            function assures correct values.
*/
IFX_int32_t VMMC_RES_StructuresAllocate (VMMC_DEVICE *pDev)
{
   VMMC_RES_StructuresFree (pDev);

   /* echo suppressor */
   pDev->pResEs = VMMC_OS_Malloc (sizeof(VMMC_RES_ES_t) * pDev->caps.nES);
   if (pDev->pResEs == NULL)
   {
      RETURN_DEVSTATUS (VMMC_statusNoMem);
   }
   memset(pDev->pResEs, 0, sizeof(VMMC_RES_ES_t) * pDev->caps.nES);

   /* line echo canceller */
   pDev->pResLec =  VMMC_OS_Malloc (sizeof(VMMC_RES_LEC_t) * pDev->caps.nNLEC);
   if (pDev->pResLec == NULL)
   {
      RETURN_DEVSTATUS (VMMC_statusNoMem);
   }
   memset(pDev->pResLec, 0, sizeof(VMMC_RES_LEC_t) * pDev->caps.nNLEC);

   /* Call Progress Tone Detector (CPTD) */
   pDev->pResCptd =  VMMC_OS_Malloc (sizeof(VMMC_RES_CPTD_t) * pDev->caps.nCPTD);

   if (pDev->pResCptd == NULL)
   {
      RETURN_DEVSTATUS (VMMC_statusNoMem);
   }
   memset(pDev->pResCptd, 0, sizeof(VMMC_RES_CPTD_t) * pDev->caps.nCPTD);

#ifdef VMMC_FEAT_HDLC
   /* HDLC */
   pDev->pResHdlc =  VMMC_OS_Malloc (sizeof(VMMC_RES_HDLC_t) * pDev->caps.nHDLC);
   if (pDev->pResHdlc == NULL)
   {
      RETURN_DEVSTATUS (VMMC_statusNoMem);
   }
   memset(pDev->pResHdlc, 0, sizeof(VMMC_RES_HDLC_t) * pDev->caps.nHDLC);
#endif /* VMMC_FEAT_HDLC */

   return VMMC_statusOk;
}

/**
   Free data structures for housekeeping of the resources handled here.

   \param  pDev         Pointer to the VMMC device structure.
*/
IFX_void_t VMMC_RES_StructuresFree (VMMC_DEVICE *pDev)
{
   if ((pDev != IFX_NULL) && (pDev->pResEs != IFX_NULL))
   {
      VMMC_OS_Free (pDev->pResEs);
      pDev->pResEs = IFX_NULL;
   }

   if ((pDev != IFX_NULL) && (pDev->pResLec != IFX_NULL))
   {
      VMMC_OS_Free (pDev->pResLec);
      pDev->pResLec = IFX_NULL;
   }

   if ((pDev != IFX_NULL) && (pDev->pResCptd != IFX_NULL))
   {
      VMMC_OS_Free (pDev->pResCptd);
      pDev->pResCptd = IFX_NULL;
   }

#ifdef VMMC_FEAT_HDLC
   if ((pDev != IFX_NULL) && (pDev->pResHdlc != IFX_NULL))
   {
      IFX_uint8_t i;

      for (i = 0; i < pDev->caps.nHDLC; i ++)
      {
         fifoFree (pDev->pResHdlc[i].pIngressFifo);

         VMMC_OS_MutexDelete (&pDev->pResHdlc[i].semProtectIngressFifo);
      }
      VMMC_OS_Free (pDev->pResHdlc);
      pDev->pResHdlc = IFX_NULL;
   }
#endif /* VMMC_FEAT_HDLC */

}


/**
   Initialise the structures for housekeeping of the resources handled here.

  \param  pDev          Pointer to the VMMC device structure.
*/
IFX_void_t VMMC_RES_StructuresInit (VMMC_DEVICE *pDev)
{
   IFX_uint8_t  i;

   /* Initialise all echo suppressor structures. */
   for (i=0; i < pDev->caps.nES; i++)
   {
      ALI_ES_t  *p_fw_es = &pDev->pResEs[i].fw_es;
      RES_ES_COEF_t *p_fw_esCoef = &pDev->pResEs[i].fw_esCoef;

      /* ES command */
      memset (p_fw_es, 0, sizeof (ALI_ES_t));
      p_fw_es->ESNR   = i;
      /* The following will be set on allocation: CHAN, MOD, ECMD, EN. */

      memset (p_fw_esCoef, 0, sizeof (RES_ES_COEF_t));
      p_fw_esCoef->CMD     = CMD_EOP;
      p_fw_esCoef->CHAN    = i;
      p_fw_esCoef->MOD     = MOD_RESOURCE;
      p_fw_esCoef->ECMD    = RES_ES_COEF_ECMD;

      /* Mark this echo suppressor as unused. */
      pDev->pResEs[i].bUsed = 0;
   }

   /* Initialise all line echo canceller structures. */
   for (i=0; i < pDev->caps.nNLEC; i++)
   {
      ALI_LEC_t *p_fw_ctrl = &pDev->pResLec[i].fw_ctrl;
      RES_LEC_COEF_t *p_fw_lecCoef = &pDev->pResLec[i].fw_lecCoef;
      RES_LEC_NLP_COEF_t *p_fw_nlpCoef = &pDev->pResLec[i].fw_nlpCoef;
      COD_ASSOCIATED_LECNR_t *p_fw_codAssociate
                                       = &pDev->pResLec[i].fw_codAssociate;

      /* LEC control command */
      memset (p_fw_ctrl, 0, sizeof (ALI_LEC_t));
      p_fw_ctrl->CMD       = CMD_EOP;
      p_fw_ctrl->DCF       = ALI_LEC_DCF_EN;
      p_fw_ctrl->LECNR     = i;
      /* The following will be set on allocation: CHAN, MOD, ECMD, EN. */

      /* LEC coefficient message. */
      memset (p_fw_lecCoef, 0, sizeof (RES_LEC_COEF_t));
      p_fw_lecCoef->CMD    = CMD_EOP;
      p_fw_lecCoef->CHAN   = i;
      p_fw_lecCoef->MOD    = MOD_RESOURCE;
      p_fw_lecCoef->ECMD   = RES_LEC_COEF_ECMD;

      /* LEC NLP coefficient message. */
      memset (p_fw_nlpCoef, 0, sizeof (RES_LEC_NLP_COEF_t));
      p_fw_nlpCoef->CMD    = CMD_EOP;
      p_fw_nlpCoef->CHAN   = i;
      p_fw_nlpCoef->MOD    = MOD_RESOURCE;
      p_fw_nlpCoef->ECMD   = RES_LEC_NLP_COEF_ECMD;

      /* COD associated LEC resource number message. */
      memset (p_fw_codAssociate, 0, sizeof (*p_fw_codAssociate));
      p_fw_codAssociate->CMD  = CMD_RTCP_XR;
      /* The channel number is set with the associate function at runtime. */
      p_fw_codAssociate->MOD  = MOD_CODER;
      p_fw_codAssociate->ECMD = COD_ASSOCIATED_LECNR_ECMD;
      p_fw_codAssociate->LECNR = i;

      /* Mark this line echo canceller as unused. */
      pDev->pResLec[i].bUsed = 0;
   }

   /* Initialise all CPTD structures. */
   for (i=0; i < pDev->caps.nCPTD; i++)
   {
      RES_CPTD_COEF_t *p_fw_cptdCoef = &pDev->pResCptd[i].fw_cptdCoef;

      memset (p_fw_cptdCoef, 0, sizeof (RES_CPTD_COEF_t));
      p_fw_cptdCoef->CMD = CMD_EOP;
      p_fw_cptdCoef->CHAN = i;
      p_fw_cptdCoef->MOD = MOD_RESOURCE;
      p_fw_cptdCoef->ECMD = RES_CPTD_COEF_ECMD;

      pDev->pResCptd[i].bUsed = 0;
      pDev->pResCptd[i].nResNr = i;
   }

#ifdef VMMC_FEAT_HDLC
   /* Initialise all HDLC structures. */
   for (i=0; i < pDev->caps.nHDLC; i++)
   {
      PCM_DCHAN_t *pPcmDCh = &pDev->pResHdlc[i].fw_pcm_hdlc;

      /* PCM D-Channel Command */
      memset (pPcmDCh, 0, sizeof (PCM_DCHAN_t));
      pPcmDCh->CMD      = CMD_EOP;
      pPcmDCh->CHAN     = i;
      pPcmDCh->MOD      = MOD_PCM;
      pPcmDCh->ECMD     = PCM_DCHAN_ECMD;
      pPcmDCh->EN       = PCM_DCHAN_DISABLE;
      pPcmDCh->DCR      = i;
      pPcmDCh->ITF      = PCM_DCHAN_ITF_FLAGS;
#ifdef HDLC_IDLE_PATTERN
      if (pDev->caps.bDIP)
      {
         pPcmDCh->ITF = PCM_DCHAN_ITF_IDLE;
      }
      else
      {
         /* driver was configured for HDLC inter frame idle pattern 0xFF,
            but the FW supports only 0x7E. Trace at low level and switch
            to 0x7E. */
         TRACE (VMMC, DBG_LEVEL_LOW,
                ("Res: HDLC %d, idle pattern ignored. FW Cap DIP: %d\n",
                 i, pDev->caps.bDIP));
      }
#endif /*HDLC_IDLE_PATTERN*/

      pDev->pResHdlc[i].pIngressFifo = fifoInit(VMMC_HDLC_MAX_FIFO_SIZE);

      VMMC_OS_MutexInit (&pDev->pResHdlc[i].semProtectIngressFifo);

      pDev->pResHdlc[i].bTxBufferEmpty = IFX_FALSE;
      pDev->pResHdlc[i].bHandle_DD_MBX = IFX_FALSE;

      /* Mark this HDLC as unused. */
      pDev->pResHdlc[i].bUsed = IFX_FALSE;
   }
#endif /* VMMC_FEAT_HDLC */
}


/**
   Allocate an echo suppressor from the resource pool.

   \param  pCh          Pointer to the VMMC channel structure.
   \param  nModule      Specifies the ALM or PCM module. (ALM is default)

   \return Resource ID to be used in successive calls.
           VMMC_RES_ID_NULL if no echo suppressor was available.

   \remarks
   This code uses a linear search for free resources. Because the number of
   resources is very small (<10) no performance problem is expected.
*/
VMMC_RES_ID_t VMMC_RES_ES_Allocate (VMMC_CHANNEL *pCh,
                                    VMMC_RES_MOD_t nModule)
{
   VMMC_DEVICE  *pDev = pCh->pParent;
   IFX_uint8_t  i, j;

   /* Search a free echo suppressor resource in all elements of the pool. */
   for (i=0; i < pDev->caps.nES; i++)
   {
      if (pDev->pResEs[i].bUsed == 0)
      {
         VMMC_RES_ID_t ret;
         VMMC_RES_ES_t *pResEs = &pDev->pResEs[i];

         /* Found an unused ES resource. */
         ALI_ES_t  *p_fw_es = &pDev->pResEs[i].fw_es;

         /* Specific to the module the resource is bound to. */
         if (nModule == VMMC_RES_MOD_PCM)
         {
            p_fw_es->MOD  = MOD_PCM;
            p_fw_es->ECMD = PCM_ES_ECMD;
            p_fw_es->CMD  = CMD_EOP;
         }
         else if (nModule == VMMC_RES_MOD_ALM)
         {
            p_fw_es->MOD  = MOD_ALI;
            p_fw_es->ECMD = ALI_ES_ECMD;
            p_fw_es->CMD  = CMD_EOP;
         }
         else if (nModule == VMMC_RES_MOD_DECT)
         {
            p_fw_es->MOD  = MOD_CODER;
            p_fw_es->ECMD = DECT_ES_ECMD;
            p_fw_es->CMD  = CMD_DECT;
         }

         /* Set channel field of the echo suppressor command. */
         p_fw_es->CHAN = pCh->nChannel - 1;
         /* Set ES to disabled. */
         p_fw_es->EN = 0;

         /* Copy the default coefficients into the storage of this resource. */
         for (j=0; j < 22; j++)
         {
            pResEs->es_coefs[VMMC_RES_ES_COEF_WITHOUT_NLP][j] =
               vmmc_res_es_without_nlp[j];
            pResEs->es_coefs[VMMC_RES_ES_COEF_WITH_NLP][j] =
               vmmc_res_es_with_nlp[j];
         }

         /* Mark coefficient-sets for writing by storing an invalid value. */
         pResEs->nLastEsCoefSetWritten = VMMC_RES_ES_COEF_INVALID;

         /* Mark this echo suppressor as in use. */
         pResEs->bUsed = 1;

         /* resource number is 1..n */
         ret.nResNr = i+1;
         ret.pDev = pDev;

         /* Return: resource successful allocated */
         return ret;
      }
   }

   /* Return: no resource available */
   return VMMC_RES_ID_NULL;
}


/**
   Release an echo suppressor back to the resource pool.

   \param  nResId       Resource id of echo suppressor to be released.

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_RES_ES_Release (VMMC_RES_ID_t nResId)
{
   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nES))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return VMMC_statusParam;
   }

   /* Mark this echo suppressor as unused. */
   nResId.pDev->pResEs[nResId.nResNr - 1].bUsed = 0;

   return VMMC_statusOk;
}


/**
   Enable or disable an echo suppressor.

   \param  nResId       Resource ID of echo suppressor.
   \param  nEnable      value = 0: disable, value <> 0: enable.

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusCmdWr Writing the command has failed
      - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_RES_ES_Enable (VMMC_RES_ID_t nResId, IFX_uint8_t nEnable)
{
   VMMC_DEVICE    *pDev;
   VMMC_RES_ES_t  *pResEs;
   ALI_ES_t       *p_fw_es;
   IFX_int32_t    ret = VMMC_statusOk;

   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nES))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return VMMC_statusParam;
   }

   pDev = nResId.pDev;
   pResEs = &pDev->pResEs[nResId.nResNr - 1];
   p_fw_es = &pResEs->fw_es;

   /* set nEnable defined to 1 or 0 */
   nEnable = nEnable ? 1 : 0;

   /* protect fwmsg against concurrent tasks */
   VMMC_OS_MutexGet (&pDev->mtxMemberAcc);

   /* if the end state should be enable AND the FW includes the enhanced ES
      write the coefficients now */
   if ((nEnable == 1) && (pDev->caps.bEnhancedES == 1))
   {
      /* This section writes coefficients for the enhanced echo suppressor. */

      VMMC_RES_ES_COEF_t nEsCoefSet;

      /* Determine which ES coefficient-set to use */
      nEsCoefSet = (pResEs->nActiveNLP == IFX_ENABLE) ?
                   VMMC_RES_ES_COEF_WITH_NLP : VMMC_RES_ES_COEF_WITHOUT_NLP;

      /* Write the ES coefficient message if needed */
      if (pResEs->nLastEsCoefSetWritten != nEsCoefSet)
      {
         RES_ES_COEF_t *p_fw_esCoef = &pResEs->fw_esCoef;

         p_fw_esCoef->LP1R        = pResEs->es_coefs[nEsCoefSet][0];
         p_fw_esCoef->OFFR        = pResEs->es_coefs[nEsCoefSet][1];
         p_fw_esCoef->LP2LR       = pResEs->es_coefs[nEsCoefSet][2];
         p_fw_esCoef->LIMR        = pResEs->es_coefs[nEsCoefSet][3];
         p_fw_esCoef->PDSR        = pResEs->es_coefs[nEsCoefSet][4];
         p_fw_esCoef->LP2SR       = pResEs->es_coefs[nEsCoefSet][5];
         p_fw_esCoef->PDNR        = pResEs->es_coefs[nEsCoefSet][6];
         p_fw_esCoef->LP2NR       = pResEs->es_coefs[nEsCoefSet][7];
         p_fw_esCoef->TAT         = pResEs->es_coefs[nEsCoefSet][8];
         p_fw_esCoef->T0          = pResEs->es_coefs[nEsCoefSet][9];
         p_fw_esCoef->ATT_ES      = pResEs->es_coefs[nEsCoefSet][10];
         p_fw_esCoef->ECHOT       = pResEs->es_coefs[nEsCoefSet][11];
         p_fw_esCoef->LPS         = pResEs->es_coefs[nEsCoefSet][12];
         p_fw_esCoef->LIM_DS      = pResEs->es_coefs[nEsCoefSet][13];
         p_fw_esCoef->MAX_ATT_ES  = pResEs->es_coefs[nEsCoefSet][14];
         p_fw_esCoef->ERL_THRESH  = pResEs->es_coefs[nEsCoefSet][15];
         p_fw_esCoef->BN_ADJ      = pResEs->es_coefs[nEsCoefSet][16];
         p_fw_esCoef->BN_INC      = pResEs->es_coefs[nEsCoefSet][17];
         p_fw_esCoef->BN_DEC      = pResEs->es_coefs[nEsCoefSet][18];
         p_fw_esCoef->BN_MAX      = pResEs->es_coefs[nEsCoefSet][19];
         p_fw_esCoef->BN_LEV_X    = pResEs->es_coefs[nEsCoefSet][20];
         p_fw_esCoef->BN_LEV_R    = pResEs->es_coefs[nEsCoefSet][21];

         ret = CmdWrite (pDev, (IFX_uint32_t*) p_fw_esCoef,
                         sizeof (*p_fw_esCoef) - CMD_HDR_CNT);
         if ( !VMMC_SUCCESS(ret) )
         {
            VMMC_OS_MutexRelease (&pDev->mtxMemberAcc);
            return ret;
         }

         pResEs->nLastEsCoefSetWritten = nEsCoefSet;
      }
   }

   /* write message only if change is needed */
   if (p_fw_es->EN != nEnable)
   {
      p_fw_es->EN = nEnable;
      ret = CmdWrite(pDev, (IFX_uint32_t *)p_fw_es, ALI_ES_LEN);
   }

   /* unlock */
   VMMC_OS_MutexRelease (&pDev->mtxMemberAcc);

   return ret;
}


/**
   Select the parameter set that is used for the echo suppressor.

   When the Echo Suppressor is used together with LEC+NLP a different set of
   parameters should be used. Because this resource is unaware of the LEC+NLP
   the status of the NLP has to be set here.

   \param  nResId       Resource ID of the line echo canceller.
   \param  nActiveNLP   Set flag that shows if LEC+NLP is active.

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_RES_ES_ParameterSelect (VMMC_RES_ID_t nResId,
                                         IFX_enDis_t nActiveNLP)
{
   VMMC_DEVICE    *pDev;

   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nES))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return VMMC_statusParam;
   }

   pDev = nResId.pDev;

   /* Just save the mode for the _Enable() function where it is used. */
   pDev->pResEs[nResId.nResNr - 1].nActiveNLP = nActiveNLP;

   return VMMC_statusOk;
}


#ifdef VMMC_FEAT_HDLC
/**
   Allocate an HDLC from the resource pool.

   \param  pCh          Pointer to the VMMC channel structure.
   \param  nModule      Specifies the ALM or PCM module. (PCM is default)

   \return Resource ID to be used in successive calls.
           VMMC_RES_ID_NULL if no HDLC was available.

   \remarks
   This code uses a linear search for free resources. Because the number of
   resources is very small (<10) no performance problem is expected.
*/
VMMC_RES_ID_t VMMC_RES_HDLC_Allocate (VMMC_CHANNEL *pCh,
                                      VMMC_RES_MOD_t nModule)
{
   VMMC_DEVICE    *pDev = IFX_NULL;
   IFX_uint8_t    i     = 0;
   VMMC_RES_ID_t  ret   = VMMC_RES_ID_NULL;

   if (pCh == IFX_NULL)
   {
      /* Channel pointer invalid */
      return ret;
   }

   pDev = pCh->pParent;

   /* protect resource against concurrent tasks */
   VMMC_OS_MutexGet (&pDev->mtxMemberAcc);

   /* Search a free HDLC resource in all elements of the pool. */
   for (i=0; i < pDev->caps.nHDLC; i++)
   {
      if (pDev->pResHdlc[i].bUsed == IFX_FALSE)
      {
         /* Mark this HDLC resource as in use. */
         pDev->pResHdlc[i].bUsed = IFX_TRUE;

         pDev->pResHdlc[i].fw_pcm_hdlc.CHAN = pCh->nChannel - 1;

         /* resource number is 1..n */
         ret.nResNr = i+1;
         ret.pDev = pDev;

         /* Return: resource successful allocated */
         break;
      }
   }

   /* unlock */
   VMMC_OS_MutexRelease (&pDev->mtxMemberAcc);

   return ret;
}

/**
   Release an HDLC back to the resource pool.

   \param  nResId       Resource id of HDLC to be released.

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_RES_HDLC_Release (VMMC_RES_ID_t nResId)
{
   VMMC_DEVICE *pDev = nResId.pDev;

   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nHDLC))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return VMMC_statusParam;
   }

   /* protect resource against concurrent tasks */
   VMMC_OS_MutexGet (&pDev->mtxMemberAcc);

   /* Mark this HDLC resource as unused. */
   nResId.pDev->pResHdlc[nResId.nResNr - 1].bUsed = IFX_FALSE;

   /* unlock */
   VMMC_OS_MutexRelease (&pDev->mtxMemberAcc);

   return VMMC_statusOk;
}

/**
   Configure timeslot for HDLC resource.

   \param  nResId       Resource ID of HDLC.
   \param  nTimeslot    Time slot.

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_RES_HDLC_TimeslotSet (VMMC_RES_ID_t nResId,
                                       IFX_uint32_t nTimeslot)
{
   VMMC_DEVICE    *pDev    = nResId.pDev;
   PCM_DCHAN_t    *pPcmDCh = IFX_NULL;

   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nHDLC))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return VMMC_statusParam;
   }

   /* protect fwmsg against concurrent tasks */
   VMMC_OS_MutexGet (&pDev->mtxMemberAcc);

   pPcmDCh = &pDev->pResHdlc[nResId.nResNr - 1].fw_pcm_hdlc;

   /* Set the timeslots and enable the channel */
   pPcmDCh->TS = nTimeslot;

   /* unlock */
   VMMC_OS_MutexRelease (&pDev->mtxMemberAcc);

   return VMMC_statusOk;
}

/**
   Retrieve timeslot of HDLC resource.

   \param  nResId       Resource ID of HDLC.
   \param  pTimeslot    Time slot.

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_RES_HDLC_TimeslotGet (VMMC_RES_ID_t nResId,
                                       IFX_uint32_t *pTimeslot)
{
   VMMC_DEVICE    *pDev    = nResId.pDev;
   PCM_DCHAN_t    *pPcmDCh = IFX_NULL;

   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nHDLC))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return VMMC_statusParam;
   }

   /* protect fwmsg against concurrent tasks */
   VMMC_OS_MutexGet (&pDev->mtxMemberAcc);

   pPcmDCh = &pDev->pResHdlc[nResId.nResNr - 1].fw_pcm_hdlc;

   /* Set the timeslots and enable the channel */
   *pTimeslot = pPcmDCh->TS;

   /* unlock */
   VMMC_OS_MutexRelease (&pDev->mtxMemberAcc);

   return VMMC_statusOk;
}

/**
   Write HDLC data to the HDLC resource.

   \param  nResId    Resource ID of HDLC.
   \param  pBuf      Pointer to a buffer with the data to be sent.
   \param  nLen      Data length in bytes.

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusHdlcFifoOverflow Internal fifo overflow
      - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_RES_HDLC_Write (VMMC_RES_ID_t nResId,
                                 const IFX_uint8_t *pBuf,
                                 IFX_int32_t nLen)
{
   VMMC_DEVICE      *pDev        = nResId.pDev;
   VMMC_RES_HDLC_t  *pResHdlc    = IFX_NULL;
   IFX_int32_t       ret         = VMMC_statusOk;

   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nHDLC))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return VMMC_statusParam;
   }

   /* protect resource against concurrent tasks */
   if (!VMMC_OS_IN_INTERRUPT())
      VMMC_OS_MutexGet (&pDev->mtxMemberAcc);

   /* take the HDLC resource from array */
   pResHdlc = pDev->pResHdlc + (nResId.nResNr - 1);

   if (!VMMC_OS_IN_INTERRUPT())
      VMMC_OS_MutexGet (&pResHdlc->semProtectIngressFifo);

   /* global irq lock */
   Vmmc_IrqLockDevice (pDev);

   /* if not ready to take further frames OR there is already a backlog
      we have to queue this frame locally - if possible */
   if ((pResHdlc->bTxBufferEmpty == IFX_FALSE) ||
       (!fifoEmpty (pResHdlc->pIngressFifo)))
   {
      if (IFX_SUCCESS != fifoPut (pResHdlc->pIngressFifo, pBuf, nLen))
      {
         /* Internal fifo overflow */
         ret = VMMC_statusHdlcFifoOverflow;
      }
#ifdef TAPI_PACKET_OWNID
      else
      {
         IFX_TAPI_VoiceBufferChOwn ((IFX_void_t*)pBuf, IFX_TAPI_BUFFER_OWNER_HDLC_VMMC);
      }
#endif /* TAPI_PACKET_OWNID */
   }
   else
   {
      VMMC_CHANNEL  *pCh = IFX_NULL;
      mps_message    msg;

      memset (&msg, 0, sizeof (msg));
      msg.cmd_type   = DAT_PAYL_PTR_MSG_HDLC_PACKET;

      msg.pData      = (IFX_uint8_t *)pBuf;
      msg.nDataBytes = (IFX_uint32_t)nLen;

      pCh = &pDev->pChannel[pResHdlc->fw_pcm_hdlc.CHAN];

#ifdef TAPI_PACKET_OWNID
      IFX_TAPI_VoiceBufferChOwn ((IFX_void_t*)pBuf, IFX_TAPI_BUFFER_OWNER_HDLC_MPS);
#endif /* TAPI_PACKET_OWNID */
      ret = VMMC_MPS_Write (pCh, IFX_TAPI_STREAM_HDLC, &msg);
      if (!VMMC_SUCCESS (ret))
      {
         if (IFX_SUCCESS != fifoPut (pResHdlc->pIngressFifo, pBuf, nLen))
         {
            /* errmsg: Internal HDLC fifo overflow */
            ret = VMMC_statusHdlcFifoOverflow;
         }
#ifdef TAPI_PACKET_OWNID
         else
         {
            IFX_TAPI_VoiceBufferChOwn ((IFX_void_t*)pBuf, IFX_TAPI_BUFFER_OWNER_HDLC_VMMC);
         }
#endif /* TAPI_PACKET_OWNID */
      }
      pResHdlc->bTxBufferEmpty = IFX_FALSE;
   }

   /* global irq unlock */
   Vmmc_IrqUnlockDevice (pDev);

   /* unlock */
   if (!VMMC_OS_IN_INTERRUPT())
   {
      VMMC_OS_MutexRelease (&pResHdlc->semProtectIngressFifo);
      VMMC_OS_MutexRelease (&pDev->mtxMemberAcc);
   }

   return ret;
}

/**
   Retrieve readable information of HDLC resource.

   \param  pDev      Pointer to VMMC device structure.
   \param  nResNr    HDLC resource number.
   \param  pBuf      Pointer to a buffer.
   \param  nLen      Buffer length in bytes.

   \return
      length of prepared data or zero on failure
*/
IFX_int32_t VMMC_RES_HDLC_InfoGet (VMMC_DEVICE *pDev,
                                   IFX_uint16_t nResNr,
                                   IFX_char_t *pBuf,
                                   IFX_int32_t nLen)
{
   VMMC_RES_HDLC_t  *pResHdlc = IFX_NULL;
   IFX_int32_t       nOutLen  = 0;

   if ((pDev == IFX_NULL) ||
       (nResNr == 0) || (nResNr > pDev->caps.nHDLC))
   {
      return nOutLen;
   }

   /* protect device against concurrent tasks */
   VMMC_OS_MutexGet (&pDev->mtxMemberAcc);

   /* take the HDLC resource from array */
   pResHdlc = pDev->pResHdlc + (nResNr - 1);

   /* protect resource against concurrent tasks */
   VMMC_OS_MutexGet (&pResHdlc->semProtectIngressFifo);

   nOutLen += snprintf (pBuf + nOutLen, nLen - nOutLen,
                        "%3d %3d %3d "
                        "%5s "
                        "%6s "
                        "%5s "
                        "%u/%u\n",
                        pDev->nDevNr, pResHdlc->fw_pcm_hdlc.CHAN, nResNr,
                        pResHdlc->bUsed == IFX_TRUE ? "used" : "free",
                        pResHdlc->bHandle_DD_MBX == IFX_TRUE ? "high" : "low",
                        pResHdlc->bTxBufferEmpty == IFX_TRUE ? "avail" : "busy",
                        pResHdlc->pIngressFifo->fifoSize,
                        pResHdlc->pIngressFifo->fifoElements);

   /* unlock */
   VMMC_OS_MutexRelease (&pResHdlc->semProtectIngressFifo);
   VMMC_OS_MutexRelease (&pDev->mtxMemberAcc);

   return nOutLen;
}

/**
   DD_MBX interrupt handler for HDLC.

   Used then MPS write command was failed.

   \param   pDev     VMMC device handle

   \return
      None
*/
IFX_void_t irq_VMMC_RES_HDLC_DD_MBX_Handler (VMMC_DEVICE *pDev)
{
   IFX_int32_t    err = IFX_SUCCESS;
   IFX_int32_t    i = 0;

   for (i=0; i < pDev->caps.nHDLC; i++)
   {
      /* take the HDLC resource from array */
      VMMC_RES_HDLC_t *pResHdlc = pDev->pResHdlc + i;

      /* Handle resources which required only */
      if (pResHdlc->bHandle_DD_MBX != IFX_TRUE)
         continue;

      /* global irq lock */
      Vmmc_IrqLockDevice (pDev);

      if (!fifoEmpty (pResHdlc->pIngressFifo))
      {
         VMMC_CHANNEL   *pCh = IFX_NULL;
         mps_message    msg;

         memset (&msg, 0, sizeof (msg));
         msg.cmd_type = DAT_PAYL_PTR_MSG_HDLC_PACKET;

         msg.pData = (IFX_uint8_t *) fifoPeek(pResHdlc->pIngressFifo,
                                              &msg.nDataBytes);

         pCh = pDev->pChannel + pResHdlc->fw_pcm_hdlc.CHAN;

#ifdef TAPI_PACKET_OWNID
         IFX_TAPI_VoiceBufferChOwn (msg.pData, IFX_TAPI_BUFFER_OWNER_HDLC_MPS);
#endif /* TAPI_PACKET_OWNID */
         err = VMMC_MPS_Write (pCh, IFX_TAPI_STREAM_HDLC, &msg);
         if (VMMC_SUCCESS (err))
         {
            /* Deactivate DD_MBX interrupt */
            ifx_mps_dd_mbx_int_disable ();

            /* Mark resource as handled */
            pResHdlc->bHandle_DD_MBX = IFX_FALSE;

            /* Remove package from fifo on success */
            (void)fifoGet(pResHdlc->pIngressFifo, IFX_NULL);
            /* signal that internal fifos have space */
            IFX_TAPI_KPI_ScheduleIngressHandling();
         }
      }

      /* global irq unlock */
      Vmmc_IrqUnlockDevice (pDev);

      if (err == IFX_SUCCESS)
      {
         /* only one resouce handled at one interrupt */
         break;
      }
   }
}

/**
   Mark HDLC buffer as ready for new data.

   \param  nResId    Resource ID of HDLC.

   \return
      None

   \remarks
      If internal fifo contain data that function will write
      that data in to the mps.
*/
IFX_void_t irq_VMMC_RES_HDLC_BufferReadySet (VMMC_RES_ID_t nResId)
{
   VMMC_DEVICE       *pDev       = nResId.pDev;
   VMMC_RES_HDLC_t   *pResHdlc   = IFX_NULL;

   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nHDLC))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return;
   }

   /* take the HDLC resource from array */
   pResHdlc = pDev->pResHdlc + (nResId.nResNr - 1);

   /* global irq lock */
   Vmmc_IrqLockDevice (pDev);

   /* if there is no local backlog, schedule KPI ingress handling */
   if (fifoEmpty(pResHdlc->pIngressFifo))
   {
      pResHdlc->bTxBufferEmpty = IFX_TRUE;
      /* signal the event that internal fifos have a space */
      IFX_TAPI_KPI_ScheduleIngressHandling();
   }
   else /* handle local backlog ... */
   {
      IFX_uint32_t   nLen  = 0;
      IFX_int32_t    err   = VMMC_statusOk;
      VMMC_CHANNEL  *pCh   = IFX_NULL;
      mps_message    msg;

      memset (&msg, 0, sizeof (msg));
      msg.cmd_type   = DAT_PAYL_PTR_MSG_HDLC_PACKET;

      msg.pData      = (IFX_uint8_t *) fifoPeek(pResHdlc->pIngressFifo, &nLen);
      msg.nDataBytes = (IFX_uint32_t)nLen;

      /* take the correct channel from array */
      pCh = pDev->pChannel + pResHdlc->fw_pcm_hdlc.CHAN;

#ifdef TAPI_PACKET_OWNID
      IFX_TAPI_VoiceBufferChOwn (msg.pData, IFX_TAPI_BUFFER_OWNER_HDLC_MPS);
#endif /* TAPI_PACKET_OWNID */
      err = VMMC_MPS_Write (pCh, IFX_TAPI_STREAM_HDLC, &msg);
      if (VMMC_SUCCESS (err))
      {
         /* Remove package from fifo on success */
         (void)fifoGet(pResHdlc->pIngressFifo, IFX_NULL);
         /* schedule KPI Ingress handler as our FIFO has space now */
         IFX_TAPI_KPI_ScheduleIngressHandling();
      }
      else
      {
         /* Mark resource for DD_MBX interrupt handling */
         pResHdlc->bHandle_DD_MBX = IFX_TRUE;

         /* Activate DD_MBX interrupt */
         ifx_mps_dd_mbx_int_enable ();
      }
   }

   /* global irq unlock */
   Vmmc_IrqUnlockDevice (pDev);
}

/**
   Enable or disable an HDLC.

   \param  nResId       Resource ID of HDLC.
   \param  nEnable      value = 0: disable, value <> 0: enable.

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusCmdWr Writing the command has failed
      - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_RES_HDLC_Enable (VMMC_RES_ID_t nResId, IFX_uint8_t nEnable)
{
   VMMC_DEVICE       *pDev       = nResId.pDev;
   VMMC_RES_HDLC_t   *pResHdlc   = IFX_NULL;
   PCM_DCHAN_t       *pPcmDCh    = IFX_NULL;
   IFX_int32_t       ret         = VMMC_statusOk;

   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nHDLC))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return VMMC_statusParam;
   }

   /* protect resource against concurrent tasks */
   VMMC_OS_MutexGet (&pDev->mtxMemberAcc);

   /* take the HDLC resource from array */
   pResHdlc = pDev->pResHdlc + (nResId.nResNr - 1);

   pPcmDCh = &pResHdlc->fw_pcm_hdlc;

   nEnable = nEnable ? PCM_DCHAN_ENABLE : PCM_DCHAN_DISABLE;

   /* write message only if change is needed */
   if (pPcmDCh->EN != nEnable)
   {
      pPcmDCh->EN = nEnable;

      ret = CmdWrite(pDev, (IFX_uint32_t *) pPcmDCh, PCM_DCHAN_LEN);
   }

   if (pPcmDCh->EN == PCM_DCHAN_DISABLE)
   {
      VMMC_OS_MutexGet (&pResHdlc->semProtectIngressFifo);
      fifoReset(pResHdlc->pIngressFifo);
      VMMC_OS_MutexRelease (&pResHdlc->semProtectIngressFifo);
   }

   /* unlock */
   VMMC_OS_MutexRelease (&pDev->mtxMemberAcc);

   return ret;
}
#endif /* VMMC_FEAT_HDLC */


/**
   Allocate an line echo canceller from the resource pool.

   This function tries to allocate always the same resource to a channel/module
   combination if it is not in use otherwise. This is done to support the
   coefficient freeze feature. For the moment the feature is only supported for
   ALM modules and the allocation strategy reflects this. This algorithm tries
   to allocate LEC for ALM modules in the first free slot while all other
   modules prefer the last free slot.

   \param  pCh          Pointer to the VMMC channel structure.
   \param  nModule      Specifies the ALM or PCM module. (ALM is default)

   \return Resource ID to be used in successive calls.
           VMMC_RES_ID_NULL if no line echo canceller was available.

   \remarks
   This code uses a linear search for free resources. Because the number of
   resources is very small (<10) no performance problem is expected.
*/
VMMC_RES_ID_t VMMC_RES_LEC_Allocate (VMMC_CHANNEL *pCh,
                                     VMMC_RES_MOD_t nModule)
{
   VMMC_DEVICE  *pDev = pCh->pParent;
   IFX_uint8_t  i, j,
                nFirstFree, nLastFree, nPrevious, nLecCnt;

   nFirstFree = nLastFree = nPrevious = nLecCnt = pDev->caps.nNLEC;

   /* Search a free line-echo-canceller resource in all elements of the pool. */
   for (i=0; i < nLecCnt; i++)
   {
      if (pDev->pResLec[i].bUsed == 0)
      {
         nLastFree = i;
         if (nFirstFree == nLecCnt)
         {
            nFirstFree = i;
         }
         if ((nModule == pDev->pResLec[i].nLastModule) &&
             (pCh->nChannel == pDev->pResLec[i].nLastChannel))
         {
            nPrevious = i;
         }
      }
   }
   /* here i == nLecCnt*/

   if (nPrevious < nLecCnt)
   {
      /* An available previously used resource is preferred for all modules. */
      i = nPrevious;
   }
   else
   {
      /* For ALM take the first free slot for all others the last free slot. */
      i = (nModule == VMMC_RES_MOD_ALM) ? nFirstFree : nLastFree;
   }
   /* In the worst case no resource was free and still i == nLecCnt. */

   if (i < nLecCnt)
   {
      /* Found an unused LEC resource. */

      VMMC_RES_ID_t ret;
      VMMC_RES_LEC_t *pResLec = &pDev->pResLec[i];
      ALI_LEC_t *p_fw_ctrl = &pResLec->fw_ctrl;

      /* Mark this line echo canceller as in use. */
      pResLec->bUsed = 1;

      /* Remember this allocation. */
      pResLec->nLastModule = nModule;
      /* channel 1..n - reset value 0 indicates an unused resource */
      pResLec->nLastChannel = pCh->nChannel;

      /* Set fwmsg fields specific to the module the resource is bound to. */
      if (nModule == VMMC_RES_MOD_PCM)
      {
         p_fw_ctrl->MOD  = MOD_PCM;
         p_fw_ctrl->ECMD = PCM_LEC_ECMD;
      }
      else
      {
         p_fw_ctrl->MOD  = MOD_ALI;
         p_fw_ctrl->ECMD = ALI_LEC_ECMD;
      }
      /* Set channel field of the LEC command. */
      p_fw_ctrl->CHAN = pCh->nChannel - 1;
      /* Set LEC to disabled. */
      p_fw_ctrl->EN = 0;
      /* Set parameter freeze defaults */
      p_fw_ctrl->RP = ALI_LEC_RP_ON;
      p_fw_ctrl->PF = ALI_LEC_PF_OFF;
      p_fw_ctrl->APF = ALI_LEC_APF_OFF;
      /* If parameter freeze is supported activate auto freeze on ALM */
      if (pDev->caps.bECA1 && (nModule == VMMC_RES_MOD_ALM))
      {
         p_fw_ctrl->APF = ALI_LEC_APF_ON;
         p_fw_ctrl->RP = (nPrevious < nLecCnt) ? ALI_LEC_RP_OFF : ALI_LEC_RP_ON;
      }

      /* Copy the default coefficients into the storage of this resource. */
      if (nModule == VMMC_RES_MOD_PCM)
      {
         for (j=0; j < 18; j++)
         {
            pResLec->nlp_coefs[VMMC_RES_LEC_NLP_COEF_NB][j] =
               vmmc_res_lec_pcm_nlp_nb[j];
            pResLec->nlp_coefs[VMMC_RES_LEC_NLP_COEF_WB][j] =
               vmmc_res_lec_pcm_nlp_wb[j];
            pResLec->nlp_coefs[VMMC_RES_LEC_NLP_COEF_NB_WITH_ES][j] =
               vmmc_res_lec_nlp_nb_with_es[j];
         }
         for (j=0; j < 11; j++)
         {
            pResLec->lec_coefs[VMMC_RES_LEC_COEF_NB_NLEC][j] =
               vmmc_res_lec_pcm_nlec_nb[j];
            pResLec->lec_coefs[VMMC_RES_LEC_COEF_NB_WLEC][j] =
               vmmc_res_lec_pcm_wlec_nb[j];
            pResLec->lec_coefs[VMMC_RES_LEC_COEF_WB_NLEC][j] =
               vmmc_res_lec_pcm_nlec_wb[j];
         }
      }
      else
      {
         for (j=0; j < 18; j++)
         {
            pResLec->nlp_coefs[VMMC_RES_LEC_NLP_COEF_NB][j] =
               vmmc_res_lec_alm_nlp_nb[j];
            pResLec->nlp_coefs[VMMC_RES_LEC_NLP_COEF_WB][j] =
               vmmc_res_lec_alm_nlp_wb[j];
            pResLec->nlp_coefs[VMMC_RES_LEC_NLP_COEF_NB_WITH_ES][j] =
               vmmc_res_lec_nlp_nb_with_es[j];
         }
         for (j=0; j < 11; j++)
         {
            pResLec->lec_coefs[VMMC_RES_LEC_COEF_NB_NLEC][j] =
               vmmc_res_lec_alm_nlec_nb[j];
            pResLec->lec_coefs[VMMC_RES_LEC_COEF_NB_WLEC][j] =
               vmmc_res_lec_alm_wlec_nb[j];
            pResLec->lec_coefs[VMMC_RES_LEC_COEF_WB_NLEC][j] =
               vmmc_res_lec_alm_nlec_wb[j];
         }
      }

      /* Mark coefficient-sets for writing by storing an invalid value. */
      pResLec->nLastNlpCoefSetWritten = VMMC_RES_LEC_COEF_INVALID;
      pResLec->nLastLecCoefSetWritten = VMMC_RES_LEC_COEF_INVALID;

      /* resource number is 1..n */
      ret.nResNr = i+1;
      ret.pDev = pDev;

      /* Return: resource successful allocated */
      return ret;
   }

   /* Return: no resource available */
   return VMMC_RES_ID_NULL;
}


/**
   Release an line echo canceller back to the resource pool.

   \param  nResId       Resource id of line echo canceller to be released.

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_RES_LEC_Release (VMMC_RES_ID_t nResId)
{
   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nNLEC))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return VMMC_statusParam;
   }

   /* Mark this line echo canceller as unused. */
   nResId.pDev->pResLec[nResId.nResNr - 1].bUsed = 0;

   return VMMC_statusOk;
}


/**
   Enable or disable an line echo canceller.

   \param  nResId       Resource ID of the line echo canceller.
   \param  nEnable      value = 0: disable, value <> 0: enable.

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusCmdWr Writing the command has failed
      - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_RES_LEC_Enable (VMMC_RES_ID_t nResId, IFX_uint8_t nEnable)
{
   VMMC_DEVICE    *pDev;
   VMMC_RES_LEC_t *pResLec;
   ALI_LEC_t      *p_fw_ctrl;
   IFX_uint32_t   oldNLP;
   IFX_int32_t    ret = VMMC_statusOk;

   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nNLEC))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return VMMC_statusParam;
   }

   pDev = nResId.pDev;
   pResLec = &pDev->pResLec[nResId.nResNr - 1];
   p_fw_ctrl = &pResLec->fw_ctrl;

   /* set nEnable to defined values */
   nEnable = nEnable ? ALI_LEC_ENABLE : ALI_LEC_DISABLE;

   /* protect fwmsg against concurrent tasks */
   VMMC_OS_MutexGet (&pDev->mtxMemberAcc);

   /* Remember the current NLP state to check at the end if it has changed */
   oldNLP = p_fw_ctrl->NLP;

   /* if the end state should be enable write the coefficients now */
   if (nEnable == ALI_LEC_ENABLE)
   {
      VMMC_RES_LEC_LEC_COEF_t nLecCoefSet;
      VMMC_RES_LEC_NLP_COEF_t nNlpCoefSet;
      IFX_uint32_t            nLecMode;
      IFX_boolean_t           bWriteLecCoefSet,
                              bWriteNlpCoefSet;

      /* Determine which NLP coefficient-set to use */
      nNlpCoefSet = pResLec->nSamplingMode ?
                       VMMC_RES_LEC_NLP_COEF_WB : VMMC_RES_LEC_NLP_COEF_NB;
      if ((nNlpCoefSet == VMMC_RES_LEC_NLP_COEF_NB) &&
          (pDev->caps.bEnhancedES) && (pResLec->nActiveES))
      {
         /* The enhanced ES is running on this narrowband channel -> use an NLP
            parameter-set the is optimised for use together with the ES. */
         nNlpCoefSet = VMMC_RES_LEC_NLP_COEF_NB_WITH_ES;
      }

      /* Determine if we need to write the NLP coefficients */
      /* Needed only if NLP is requested and set was not written before. */
      bWriteNlpCoefSet = ((pResLec->nNLP == IFX_ENABLE) &&
                          (pResLec->nLastNlpCoefSetWritten != nNlpCoefSet)) ?
                         IFX_TRUE : IFX_FALSE;

      /* Determine which LEC coefficient-set to use and the LEC operation mode
         possible based on the configuration. */
      switch (pResLec->nSamplingMode)
      {
         default:
         case NB_8_KHZ:
            /* NB sampling mode */
            switch (pResLec->nOperatingMode)
            {
               default:
               case VMMC_RES_LEC_MODE_NLEC:
                  /* NLEC operating mode */
                  nLecCoefSet = VMMC_RES_LEC_COEF_NB_NLEC;
                  nLecMode = ALI_LEC_MW_FIX;
                  break;
               case VMMC_RES_LEC_MODE_WLEC:
                  /* WLEC operating mode */
                  nLecCoefSet = VMMC_RES_LEC_COEF_NB_WLEC;
                  nLecMode = ALI_LEC_MW_ADAPTIVE;
                  break;
            }
            break;
         case WB_16_KHZ:
            /* WB sampling mode */
            nLecCoefSet = VMMC_RES_LEC_COEF_WB_NLEC;
            /* In wideband mode there is only NLEC operating mode possible */
            nLecMode = ALI_LEC_MW_FIX;
            break;
      }

      /* Determine if we need to write the LEC coefficients */
      bWriteLecCoefSet = (pResLec->nLastLecCoefSetWritten != nLecCoefSet) ?
                         IFX_TRUE : IFX_FALSE;

      /* LEC must be disabled before coefficients can be written or if
         LEC mode changes. */
      if (p_fw_ctrl->EN &&
          (bWriteLecCoefSet || bWriteNlpCoefSet || (p_fw_ctrl->MW != nLecMode)))
      {
         p_fw_ctrl->EN = ALI_LEC_DISABLE;
         ret = CmdWrite(pDev, (IFX_uint32_t *)p_fw_ctrl, ALI_LEC_LEN);
      }

      /* Write the NLP coefficient message if needed */
      if (bWriteNlpCoefSet)
      {
         RES_LEC_NLP_COEF_t *p_fw_nlpCoef = &pResLec->fw_nlpCoef;

         p_fw_nlpCoef->C_POW_INC     = pResLec->nlp_coefs[nNlpCoefSet][0];
         p_fw_nlpCoef->C_POW_DEC     = pResLec->nlp_coefs[nNlpCoefSet][1];
         p_fw_nlpCoef->C_BN_LEV_X    = pResLec->nlp_coefs[nNlpCoefSet][2];
         p_fw_nlpCoef->C_BN_LEV_R    = pResLec->nlp_coefs[nNlpCoefSet][3];
         p_fw_nlpCoef->C_BN_INC      = pResLec->nlp_coefs[nNlpCoefSet][4];
         p_fw_nlpCoef->C_BN_DEC      = pResLec->nlp_coefs[nNlpCoefSet][5];
         p_fw_nlpCoef->C_BN_MAX      = pResLec->nlp_coefs[nNlpCoefSet][6];
         p_fw_nlpCoef->C_BN_ADJ      = pResLec->nlp_coefs[nNlpCoefSet][7];
         p_fw_nlpCoef->C_RE_MIN_ERLL = pResLec->nlp_coefs[nNlpCoefSet][8];
         p_fw_nlpCoef->C_RE_EST_ERLL = pResLec->nlp_coefs[nNlpCoefSet][9];
         p_fw_nlpCoef->C_SD_LEV_X    = pResLec->nlp_coefs[nNlpCoefSet][10];
         p_fw_nlpCoef->C_SD_LEV_R    = pResLec->nlp_coefs[nNlpCoefSet][11];
         p_fw_nlpCoef->C_SD_LEV_BN   = pResLec->nlp_coefs[nNlpCoefSet][12];
         p_fw_nlpCoef->C_SD_LEV_RE   = pResLec->nlp_coefs[nNlpCoefSet][13];
         p_fw_nlpCoef->C_SD_OT_DT    = pResLec->nlp_coefs[nNlpCoefSet][14];
         p_fw_nlpCoef->C_ERL_LPF     = pResLec->nlp_coefs[nNlpCoefSet][15];
         p_fw_nlpCoef->C_ERL_LPS     = pResLec->nlp_coefs[nNlpCoefSet][16];
         p_fw_nlpCoef->C_CT_LEV_RE   = pResLec->nlp_coefs[nNlpCoefSet][17];

         ret = CmdWrite (pDev, (IFX_uint32_t*) p_fw_nlpCoef,
                         sizeof (*p_fw_nlpCoef) - CMD_HDR_CNT);
         if ( !VMMC_SUCCESS(ret) )
         {
            VMMC_OS_MutexRelease (&pDev->mtxMemberAcc);
            return ret;
         }

         pResLec->nLastNlpCoefSetWritten = nNlpCoefSet;
      }

      /* Write the LEC coefficient message if needed */
      if (bWriteLecCoefSet)
      {
         RES_LEC_COEF_t *p_fw_lecCoef = &pResLec->fw_lecCoef;

         p_fw_lecCoef->LEN         = pResLec->lec_coefs[nLecCoefSet][0];
         p_fw_lecCoef->POWR        = pResLec->lec_coefs[nLecCoefSet][1];
         p_fw_lecCoef->DELTA_P     = pResLec->lec_coefs[nLecCoefSet][2];
         p_fw_lecCoef->DELTA_Q     = pResLec->lec_coefs[nLecCoefSet][3];
         p_fw_lecCoef->GAIN_XI     = pResLec->lec_coefs[nLecCoefSet][4];
         p_fw_lecCoef->GAIN_XO     = pResLec->lec_coefs[nLecCoefSet][5];
         p_fw_lecCoef->GAIN_RI     = pResLec->lec_coefs[nLecCoefSet][6];
         p_fw_lecCoef->LEN_FIX_WIN = pResLec->lec_coefs[nLecCoefSet][7];
         p_fw_lecCoef->PMW_POWR    = pResLec->lec_coefs[nLecCoefSet][8];
         p_fw_lecCoef->PMW_DELTAP  = pResLec->lec_coefs[nLecCoefSet][9];
         p_fw_lecCoef->PMW_DELTAQ  = pResLec->lec_coefs[nLecCoefSet][10];

         ret = CmdWrite (pDev, (IFX_uint32_t*) p_fw_lecCoef,
                         sizeof (*p_fw_lecCoef) - CMD_HDR_CNT);
         if ( !VMMC_SUCCESS(ret) )
         {
            VMMC_OS_MutexRelease (&pDev->mtxMemberAcc);
            return ret;
         }

         pResLec->nLastLecCoefSetWritten = nLecCoefSet;
      }

      /* Set the LEC operating mode only here where LEC is disabled and about
         to be enabled. Flags should not be changed in disable command. */
      /* Set the flag for LEC operation mode */
      p_fw_ctrl->MW  = nLecMode;
      /* Set the flag for NLP operation mode */
      p_fw_ctrl->NLP = pResLec->nNLP ? ALI_LEC_NLP_ON : ALI_LEC_NLP_OFF;
   }

   /* If RTCP XR is supported clear on disable the association between
      LEC and COD channel. */
   if ((nEnable == ALI_LEC_DISABLE) &&
       (pResLec->pAssociatedCod) && (pDev->caps.bRtcpXR))
   {
      COD_ASSOCIATED_LECNR_t *p_fw_codAssociate;

      p_fw_codAssociate = &pResLec->fw_codAssociate;

      p_fw_codAssociate->C = COD_ASSOCIATED_LECNR_C_CLEAR;
      p_fw_codAssociate->CHAN = pResLec->pAssociatedCod->nChannel - 1;
      ret = CmdWrite (pDev, (IFX_uint32_t *)p_fw_codAssociate,
                      COD_ASSOCIATED_LECNR_LEN);
      if ( !VMMC_SUCCESS(ret) )
      {
         VMMC_OS_MutexRelease (&pDev->mtxMemberAcc);
         return ret;
      }
      pResLec->pAssociatedCod = IFX_NULL;
   }

   /* write control message only if change is needed */
   if ((p_fw_ctrl->EN != nEnable) || (p_fw_ctrl->NLP != oldNLP))
   {
      p_fw_ctrl->EN = nEnable;

      ret = CmdWrite(pDev, (IFX_uint32_t *)p_fw_ctrl, ALI_LEC_LEN);
   }

   /* unlock */
   VMMC_OS_MutexRelease (&pDev->mtxMemberAcc);

   return ret;
}


/**
   Set the operating mode of the line echo canceller.

   \param  nResId       Resource ID of the line echo canceller.
   \param  nOperatingMode  VMMC_RES_LEC_MODE_NLEC or VMMC_RES_LEC_MODE_WLEC.
   \param  nNLP         Enable or disable "Non Linear Processing"

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_RES_LEC_OperatingModeSet (VMMC_RES_ID_t nResId,
                                           VMMC_RES_LEC_MODE_t nOperatingMode,
                                           IFX_enDis_t nNLP)
{
   VMMC_DEVICE    *pDev;

   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nNLEC))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return VMMC_statusParam;
   }

   pDev = nResId.pDev;

   /* Just save the mode for the _Enable() function where it is used. */
   pDev->pResLec[nResId.nResNr - 1].nOperatingMode = nOperatingMode;
   pDev->pResLec[nResId.nResNr - 1].nNLP = nNLP;

   return VMMC_statusOk;
}


/**
   Set the sampling mode of the line echo canceller.

   \param  nResId       Resource ID of the line echo canceller.
   \param  nSamplingMode  NB_8_KHZ (narrowband) or WB_16_KHZ (wideband).

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_RES_LEC_SamplingModeSet (VMMC_RES_ID_t nResId,
                                          OPMODE_SMPL nSamplingMode)
{
   VMMC_DEVICE    *pDev;

   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nNLEC))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return VMMC_statusParam;
   }

   pDev = nResId.pDev;

   /* Just save the mode for the _Enable() function where it is used. */
   pDev->pResLec[nResId.nResNr - 1].nSamplingMode = nSamplingMode;

   return VMMC_statusOk;
}


/**
   Set the window sizes of the line echo canceller.

   \param  nResId       Resource ID of the line echo canceller.
   \param  nSamplingMode   NB_8_KHZ (narrowband) or WB_16_KHZ (wideband).
   \param  nOperatingMode  VMMC_RES_LEC_MODE_NLEC or VMMC_RES_LEC_MODE_WLEC.
   \param  nTotalLength    Sum of the near end and the far end window [ms].
   \param  nMovingLength   Size of the far end window [ms].

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_RES_LEC_CoefWinSet (VMMC_RES_ID_t nResId,
                                     OPMODE_SMPL nSamplingMode,
                                     VMMC_RES_LEC_MODE_t nOperatingMode,
                                     IFX_uint8_t nTotalLength,
                                     IFX_uint8_t nMovingLength)
{
   VMMC_DEVICE    *pDev;
   VMMC_RES_LEC_t *pResLec;
   VMMC_RES_LEC_LEC_COEF_t nLecCoefSet;
   IFX_boolean_t   bHasChanged = IFX_FALSE;

   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nNLEC))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return VMMC_statusParam;
   }

   pDev = nResId.pDev;
   pResLec = &pDev->pResLec[nResId.nResNr - 1];

   /* Determine which LEC coefficient-set to use */
   switch (nSamplingMode)
   {
      default:
      case NB_8_KHZ:
         /* NB sampling mode */
         switch (nOperatingMode)
         {
            default:
            case VMMC_RES_LEC_MODE_NLEC:
               /* NLEC operating mode */
               nLecCoefSet = VMMC_RES_LEC_COEF_NB_NLEC;
               break;
            case VMMC_RES_LEC_MODE_WLEC:
               /* WLEC operating mode */
               nLecCoefSet = VMMC_RES_LEC_COEF_NB_WLEC;
               break;
         }
         break;
      case WB_16_KHZ:
         /* WB sampling mode */
         /* In wideband mode there is only NLEC operating mode possible */
         nLecCoefSet = VMMC_RES_LEC_COEF_WB_NLEC;
         break;
   }

   if ((pResLec->lec_coefs[nLecCoefSet][0] != nTotalLength) ||
       (pResLec->lec_coefs[nLecCoefSet][7] != nTotalLength - nMovingLength))
   {
      /* Set the window coefficients */
      pResLec->lec_coefs[nLecCoefSet][0] = nTotalLength;
      pResLec->lec_coefs[nLecCoefSet][7] = nTotalLength - nMovingLength;
      /* Set flag that an update might be needed */
      bHasChanged = IFX_TRUE;
   }

   /* If we changed the coefficient-set that was written down last we need
      to write it again before the next usage. */
   if (bHasChanged && (pResLec->nLastLecCoefSetWritten == nLecCoefSet))
   {
      /* Force writing LEC coefficients again. */
      pResLec->nLastLecCoefSetWritten = VMMC_RES_LEC_COEF_INVALID;
   }

   return VMMC_statusOk;
}


/**
   Do parameter checks on the window size parameters and correct them if
   neccessary and possible.

   \param  nModule         Specifies the ALM or PCM module. (ALM is default)
   \param  nOperatingMode  Operating mode NLEC/WLEC.
   \param  pLecConf        Pointer to data struct with window sizes.

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_RES_LEC_CoefWinValidate (VMMC_RES_MOD_t nModule,
                                          VMMC_RES_LEC_MODE_t nOperatingMode,
                                          TAPI_LEC_DATA_t *pLecConf)
{
   /* set 0 values to default values from the coefficient arrays */

   switch (nOperatingMode)
   {
      default:
      case VMMC_RES_LEC_MODE_NLEC:
         if (pLecConf->nNBNEwindow == 0)
         {
            if (nModule == VMMC_RES_MOD_ALM)
            {
               pLecConf->nNBNEwindow =
                  (IFX_TAPI_WLEC_WIN_SIZE_t)vmmc_res_lec_alm_nlec_nb[0];
            }
            else
            {
               pLecConf->nNBNEwindow =
                  (IFX_TAPI_WLEC_WIN_SIZE_t)vmmc_res_lec_pcm_nlec_nb[0];
            }
         }
         /* For NLEC operation the FE window is not needed.
            It is set to a length of 0 after the validations below. */
         break;
      case VMMC_RES_LEC_MODE_WLEC:
         if (pLecConf->nNBNEwindow == 0)
         {
            if (nModule == VMMC_RES_MOD_ALM)
            {
               pLecConf->nNBNEwindow =
                  (IFX_TAPI_WLEC_WIN_SIZE_t)vmmc_res_lec_alm_wlec_nb[7];
            }
            else
            {
               pLecConf->nNBNEwindow =
                  (IFX_TAPI_WLEC_WIN_SIZE_t)vmmc_res_lec_pcm_wlec_nb[7];
            }
         }
         if (pLecConf->nNBFEwindow == 0)
         {
            if (nModule == VMMC_RES_MOD_ALM)
            {
               pLecConf->nNBFEwindow =
                  (IFX_TAPI_WLEC_WIN_SIZE_t)(vmmc_res_lec_alm_wlec_nb[0] -
                                             vmmc_res_lec_alm_wlec_nb[7]);
            }
            else
            {
               pLecConf->nNBFEwindow =
                  (IFX_TAPI_WLEC_WIN_SIZE_t)(vmmc_res_lec_pcm_wlec_nb[0] -
                                             vmmc_res_lec_pcm_wlec_nb[7]);
            }
         }
         break;
   }

   if (pLecConf->nWBNEwindow == 0)
   {
      if (nModule == VMMC_RES_MOD_ALM)
      {
         pLecConf->nWBNEwindow =
            (IFX_TAPI_WLEC_WIN_SIZE_t)vmmc_res_lec_alm_nlec_wb[0];
      }
      else
      {
         pLecConf->nWBNEwindow =
            (IFX_TAPI_WLEC_WIN_SIZE_t)vmmc_res_lec_pcm_nlec_wb[0];
      }
   }

   /* set values below the lower limit to the lower limit of 4 ms */
   if (pLecConf->nNBNEwindow < IFX_TAPI_WLEN_WSIZE_4)
      pLecConf->nNBNEwindow = IFX_TAPI_WLEN_WSIZE_4;
   if (pLecConf->nNBFEwindow < IFX_TAPI_WLEN_WSIZE_4)
      pLecConf->nNBFEwindow = IFX_TAPI_WLEN_WSIZE_4;
   if (pLecConf->nWBNEwindow < IFX_TAPI_WLEN_WSIZE_4)
      pLecConf->nWBNEwindow = IFX_TAPI_WLEN_WSIZE_4;

   /* set values above the upper limit to the upper limit of 16 ms */
   if (pLecConf->nNBNEwindow > IFX_TAPI_WLEN_WSIZE_16)
      pLecConf->nNBNEwindow = IFX_TAPI_WLEN_WSIZE_16;
   if (pLecConf->nNBFEwindow > IFX_TAPI_WLEN_WSIZE_16)
      pLecConf->nNBFEwindow = IFX_TAPI_WLEN_WSIZE_16;
   /* the upper limit in wideband mode is defined as 8 ms */
   if (pLecConf->nWBNEwindow > IFX_TAPI_WLEN_WSIZE_8)
      pLecConf->nWBNEwindow = IFX_TAPI_WLEN_WSIZE_8;

   /* in NLEC operation mode set the unused FE window to 0 ms */
   if (nOperatingMode == VMMC_RES_LEC_MODE_NLEC)
   {
      pLecConf->nNBFEwindow = (IFX_TAPI_WLEC_WIN_SIZE_t) 0;
   }

   /* if the combined window size exceeds the limit of 16 ms return error */
   if ((nOperatingMode == VMMC_RES_LEC_MODE_WLEC) &&
       (((IFX_uint8_t)pLecConf->nNBNEwindow +
         (IFX_uint8_t)pLecConf->nNBFEwindow) > IFX_TAPI_WLEN_WSIZE_16))
   {
      /* combined window sizes for narrwoband mode exceed the 16 ms limit */
      return VMMC_statusParam;
   }

   return VMMC_statusOk;
}


/**
   Select the parameter set that is used for the LEC NLP.

   When LEC+NLP is used together with the Echo Suppressor a different set of
   parameters should be used. Because this resource is unaware of the ES the
   status of the ES has to be set here.

   \param  nResId       Resource ID of the line echo canceller.
   \param  nActiveES    Set flag that shows if the ES is active.

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusOk if successful
*/
IFX_int32_t VMMC_RES_LEC_ParameterSelect (VMMC_RES_ID_t nResId,
                                         IFX_enDis_t nActiveES)
{
   VMMC_DEVICE    *pDev;

   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nNLEC))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return VMMC_statusParam;
   }

   pDev = nResId.pDev;

   /* Just save the selector for the _Enable() function where it is used. */
   pDev->pResLec[nResId.nResNr - 1].nActiveES = nActiveES;

   return VMMC_statusOk;
}


/**
   Set which COD module is associated with this line echo canceller.

   \param  nResId       Resource ID of the line echo canceller.
   \param  pCodCh       Pointer to the VMMC channel structure containing the
                        associated COD module or IFX_NULL if no association
                        should be done or an existing should be cleared.

   \return
      - VMMC_statusParam The parameters are wrong
      - VMMC_statusOk if successful or feature not supported
*/
IFX_int32_t VMMC_RES_LEC_AssociatedCodSet (VMMC_RES_ID_t nResId,
                                           VMMC_CHANNEL *pCodCh)
{
   VMMC_DEVICE    *pDev;

   if ((nResId.pDev == IFX_NULL) ||
       (nResId.nResNr == 0) || (nResId.nResNr > nResId.pDev->caps.nNLEC))
   {
      /* Device pointer invalid or resource number out of range. */
      /* Cannot log error because pDEV may be NULL. */
      return VMMC_statusParam;
   }
   pDev = nResId.pDev;

   /* Only if RTCP XR is supported set the association between
      LEC and COD channel. */
   if (pDev->caps.bRtcpXR)
   {
      VMMC_RES_LEC_t *pResLec;
      COD_ASSOCIATED_LECNR_t *p_fw_codAssociate;
      IFX_int32_t    ret = VMMC_statusOk;

      pResLec = &pDev->pResLec[nResId.nResNr - 1];

      p_fw_codAssociate = &pResLec->fw_codAssociate;

      if (pCodCh != IFX_NULL)
      {
         p_fw_codAssociate->C = COD_ASSOCIATED_LECNR_C_SET;
         p_fw_codAssociate->CHAN = pCodCh->nChannel - 1;
      }
      else
      {
         p_fw_codAssociate->C = COD_ASSOCIATED_LECNR_C_CLEAR;
      }
      ret = CmdWrite (pDev, (IFX_uint32_t *)p_fw_codAssociate,
                      COD_ASSOCIATED_LECNR_LEN);
      if ( !VMMC_SUCCESS(ret) )
      {
         return ret;
      }
      pResLec->pAssociatedCod = pCodCh;
   }

   return VMMC_statusOk;
}


/**
   Allocate CPTD (Call Progress Tone Detection) from the resource pool.

   \param  pCh          Pointer to the VMMC channel structure.
   \param  pResNr       Pointer to resource number.

   \return
   - VMMC_statusOk if resource allocated
   - VMMC_statusNoRes resource data not allocated
   - VMMC_statusSigCptdNoRes if no resource available
*/
IFX_int32_t VMMC_RES_CPTD_Allocate (VMMC_CHANNEL *pCh,
                                    IFX_uint8_t *pResNr)
{
   VMMC_DEVICE  *pDev = pCh->pParent;
   IFX_uint8_t  i;

   if (IFX_NULL == pDev->pResCptd)
      RETURN_STATUS(VMMC_statusNoRes);

   /* Search a free line-echo-canceller resource in all elements of the pool. */
   for (i=0; i < pDev->caps.nCPTD; i++)
   {
      if (pDev->pResCptd[i].bUsed == 0)
      {
         pDev->pResCptd[i].bUsed = 1;

         *pResNr = pDev->pResCptd[i].nResNr;

         RETURN_STATUS(VMMC_statusOk);
      }
   }
   /* Return: no resource available */
   RETURN_STATUS(VMMC_statusSigCptdNoRes);
}

/**
   Release CPTD (Call Progress Tone Detector) back to the resource pool.

   \param  pCh          Pointer to the VMMC channel structure.
   \param  nResNr       Resource number of CPTD to be released.

   \return
      - VMMC_statusFuncParm The parameters are wrong
      - VMMC_statusNoRes resource data not allocated
      - VMMC_statusOk if successful released resource
*/
IFX_int32_t VMMC_RES_CPTD_Release (VMMC_CHANNEL *pCh,
                                   IFX_uint8_t nResNr)
{
   VMMC_DEVICE *pDev = pCh->pParent;

   if (IFX_NULL == pDev->pResCptd)
      RETURN_STATUS(VMMC_statusNoRes);

   if (nResNr >= pDev->caps.nCPTD)
   {
      /* Resource number out of range. */
      RETURN_STATUS(VMMC_statusFuncParm);
   }

   /* Mark this CPTD as unused and reset tone index. */
   pDev->pResCptd[nResNr].bUsed = 0;

   RETURN_STATUS(VMMC_statusOk);
}


