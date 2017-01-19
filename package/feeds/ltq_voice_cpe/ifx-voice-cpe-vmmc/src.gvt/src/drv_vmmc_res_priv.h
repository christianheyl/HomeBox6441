#ifndef _DRV_VMMC_RES_PRIV_H
#define _DRV_VMMC_RES_PRIV_H
/******************************************************************************

                              Copyright (c) 2013
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
/**
   \file  drv_vmmc_res_priv.h  Declaration of functionality used by
                               multiple modules.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

/* includes */
#include "drv_vmmc_fw_commands_voip.h"
#include "drv_vmmc_res.h"
#ifdef VMMC_FEAT_HDLC
#include <lib_fifo.h>
#endif /* VMMC_FEAT_HDLC */

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/** low level fifo buffer size to store buffers upfront the MPS mailbox and
    the one frame buffer per HDLC resource */
#define VMMC_HDLC_MAX_FIFO_SIZE        20

/* ============================= */
/* Global types declaration      */
/* ============================= */

/** Enum to address the internal ES coefficient storage */
typedef enum
{
   /** ES without LEC+NLP */
   VMMC_RES_ES_COEF_WITHOUT_NLP = 0,
   /** ES with LEC+NLP */
   VMMC_RES_ES_COEF_WITH_NLP = 1,
   /** Special value that marks the last written coefficient-set as invalid */
   VMMC_RES_ES_COEF_INVALID = 255
} VMMC_RES_ES_COEF_t;

/** Enum to address the internal LEC coefficient storage */
typedef enum
{
   /** Narrowband / NLEC */
   VMMC_RES_LEC_COEF_NB_NLEC = 0,
   /** Narrowband / WLEC */
   VMMC_RES_LEC_COEF_NB_WLEC = 1,
   /** Wideband / NLEC */
   VMMC_RES_LEC_COEF_WB_NLEC = 2,
   /** Special value that marks the last written coefficient-set as invalid */
   VMMC_RES_LEC_COEF_INVALID = 255
} VMMC_RES_LEC_LEC_COEF_t;

/** Enum to address the internal NLP coefficient storage */
typedef enum
{
   /** Narrowband */
   VMMC_RES_LEC_NLP_COEF_NB = 0,
   /** Wideband */
   VMMC_RES_LEC_NLP_COEF_WB = 1,
   /** Narrowband; alternate parameters for use together with Echo Suppressor */
   VMMC_RES_LEC_NLP_COEF_NB_WITH_ES = 2
} VMMC_RES_LEC_NLP_COEF_t;

/** Structure for the echo suppressor (ES) */
struct VMMC_RES_ES
{
   /* Echo suppressor command message. */
   ALI_ES_t               fw_es;
   /* Echo suppressor coefficient message. */
   RES_ES_COEF_t          fw_esCoef;

   /* ES coefficient-set storage (ES without LEC+NLP, ES with LEC+NLP) */
   IFX_uint16_t           es_coefs[2][22];

   /* Configured parameter selectors */
   IFX_enDis_t            nActiveNLP;

   /* Keep track which coefficient-set was written last. */
   IFX_uint8_t            nLastEsCoefSetWritten;

   /* Used flag. To make sure that entire struct is a multiple of words
      keep the size of one word. */
   IFX_uint32_t           bUsed;
};

/** Structure for the line echo canceller (LEC) */
struct VMMC_RES_LEC
{
   /* Line echo canceller control message. */
   ALI_LEC_t              fw_ctrl;
   /* Line echo canceller coefficient message. */
   RES_LEC_COEF_t         fw_lecCoef;
   /* Line echo canceller NLP coefficient message. */
   RES_LEC_NLP_COEF_t     fw_nlpCoef;

   /* Configured operating modes */
   OPMODE_SMPL            nSamplingMode;
   VMMC_RES_LEC_MODE_t    nOperatingMode;
   IFX_enDis_t            nNLP;

   /* Configured parameter selectors */
   IFX_enDis_t            nActiveES;

   /* NLP coefficient-set storage (NB/WB/NB+ES) */
   IFX_uint8_t            nlp_coefs[3][18];
   /* LEC coefficient-set storage (NB-NLEC/NB-WLEC/WB-NLEC) */
   IFX_uint16_t           lec_coefs[3][11];

   /* Keep track which coefficient-set was written last. */
   IFX_uint8_t            nLastNlpCoefSetWritten;
   IFX_uint8_t            nLastLecCoefSetWritten;

   /* Keep track of last allocation for parameter freeze feature. */
   IFX_uint8_t            nLastChannel;
   VMMC_RES_MOD_t         nLastModule;

   /* COD module associated with this LEC resource. Needed to calculate
      the residual echo return loss value for the RTCP XR statistics. */
   VMMC_CHANNEL           *pAssociatedCod;
   /* Message to associate the LEC number with the COD channel. */
   COD_ASSOCIATED_LECNR_t fw_codAssociate;

   /* Used flag. */
   IFX_uint32_t           bUsed;

/*To make sure that entire struct is a multiple of words
      keep the size of one word. */
};

/** Structure for the Call Progress Tone Detector (CPTD) */
struct VMMC_RES_CPTD
{
   /* CPTD coefficient message. */
   RES_CPTD_COEF_t         fw_cptdCoef;

   /* Used flag. */
   IFX_uint32_t           bUsed;
   /* Resource number. */
   IFX_uint32_t           nResNr;
};

#ifdef VMMC_FEAT_HDLC
struct VMMC_RES_HDLC
{
   /* D-channel control message */
   PCM_DCHAN_t          fw_pcm_hdlc;

   /* Ingress fifo */
   FIFO_ID              *pIngressFifo;
   VMMC_OS_mutex_t      semProtectIngressFifo;

   /* Empty buffer flag. */
   IFX_boolean_t        bTxBufferEmpty;

   /* Used flag. */
   IFX_boolean_t        bUsed;

   /* Required DD_MBX interrupt handling */
   IFX_boolean_t        bHandle_DD_MBX;
};
#endif /* VMMC_FEAT_HDLC */



#endif /* _DRV_VMMC_RES_PRIV_H */
