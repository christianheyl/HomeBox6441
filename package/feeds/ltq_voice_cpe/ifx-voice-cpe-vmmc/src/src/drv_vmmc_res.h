#ifndef _DRV_VMMC_RES_H
#define _DRV_VMMC_RES_H
/******************************************************************************

                              Copyright (c) 2013
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
/**
   \file  drv_vmmc_res.h  Declaration of functionality used by multiple modules.
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_vmmc_api.h"

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

#define VMMC_RES_ID_VALID(resId) ((resId.nResNr != 0) &&             \
                                  (resId.pDev != IFX_NULL))

/* ============================= */
/* Global types declaration      */
/* ============================= */

/** Resource ID used to specify a resource. */
typedef struct
{
   /** resource number */
   IFX_uint8_t nResNr;
   /** device that this resource belongs to */
   VMMC_DEVICE *pDev;
} VMMC_RES_ID_t;

/** Specifies the module type the resource is bound to. */
typedef enum
{
   VMMC_RES_MOD_ALM  = 0,
   VMMC_RES_MOD_PCM  = 1,
   VMMC_RES_MOD_DECT = 2
} VMMC_RES_MOD_t;

typedef enum
{
   VMMC_RES_LEC_MODE_NLEC = 0,
   VMMC_RES_LEC_MODE_WLEC = 1
} VMMC_RES_LEC_MODE_t;

/* ============================= */
/* Global variable definition    */
/* ============================= */

/** Constant that identifies an empty resource id. */
extern const VMMC_RES_ID_t  VMMC_RES_ID_NULL;


/* ============================= */
/* Global function declaration   */
/* ============================= */

extern IFX_int32_t   VMMC_RES_StructuresAllocate (VMMC_DEVICE *pDev);
extern IFX_void_t    VMMC_RES_StructuresFree     (VMMC_DEVICE *pDev);
extern IFX_void_t    VMMC_RES_StructuresInit     (VMMC_DEVICE *pDev);

extern VMMC_RES_ID_t VMMC_RES_ES_Allocate   (VMMC_CHANNEL *pCh,
                                             VMMC_RES_MOD_t nModule);
extern IFX_int32_t   VMMC_RES_ES_Release    (VMMC_RES_ID_t nResId);
extern IFX_int32_t   VMMC_RES_ES_Enable     (VMMC_RES_ID_t nResId,
                                             IFX_uint8_t nEnable);
extern IFX_int32_t   VMMC_RES_ES_ParameterSelect (
                                             VMMC_RES_ID_t nResId,
                                             IFX_enDis_t nActiveNLP);

extern VMMC_RES_ID_t VMMC_RES_LEC_Allocate  (VMMC_CHANNEL *pCh,
                                             VMMC_RES_MOD_t nModule);
extern IFX_int32_t   VMMC_RES_LEC_Release   (VMMC_RES_ID_t nResId);
extern IFX_int32_t   VMMC_RES_LEC_Enable    (VMMC_RES_ID_t nResId,
                                             IFX_uint8_t nEnable);
extern IFX_int32_t   VMMC_RES_LEC_OperatingModeSet (
                                             VMMC_RES_ID_t nResId,
                                             VMMC_RES_LEC_MODE_t nOperatingMode,
                                             IFX_enDis_t nNLP);
extern IFX_int32_t   VMMC_RES_LEC_SamplingModeSet (
                                             VMMC_RES_ID_t nResId,
                                             OPMODE_SMPL nSamplingMode);
extern IFX_int32_t   VMMC_RES_LEC_CoefWinSet (
                                             VMMC_RES_ID_t nResId,
                                             OPMODE_SMPL nSamplingMode,
                                             VMMC_RES_LEC_MODE_t nOperatingMode,
                                             IFX_uint8_t nTotalLength,
                                             IFX_uint8_t nMovingLength);
extern IFX_int32_t   VMMC_RES_LEC_CoefWinValidate (
                                             VMMC_RES_MOD_t nModule,
                                             VMMC_RES_LEC_MODE_t nOperatingMode,
                                             TAPI_LEC_DATA_t *pLecConf);
extern IFX_int32_t   VMMC_RES_LEC_ParameterSelect (
                                             VMMC_RES_ID_t nResId,
                                             IFX_enDis_t nActiveES);
extern IFX_int32_t   VMMC_RES_LEC_AssociatedCodSet (
                                             VMMC_RES_ID_t nResId,
                                             VMMC_CHANNEL *pCodCh);

#ifdef VMMC_FEAT_HDLC
extern VMMC_RES_ID_t VMMC_RES_HDLC_Allocate  (VMMC_CHANNEL *pCh,
                                              VMMC_RES_MOD_t nModule);
extern IFX_int32_t   VMMC_RES_HDLC_Release   (VMMC_RES_ID_t nResId);
extern IFX_int32_t   VMMC_RES_HDLC_Enable    (VMMC_RES_ID_t nResId,
                                              IFX_uint8_t nEnable);
extern IFX_int32_t   VMMC_RES_HDLC_TimeslotSet (
                                              VMMC_RES_ID_t nResId,
                                              IFX_uint32_t nTimeslot);
extern IFX_int32_t   VMMC_RES_HDLC_TimeslotGet (
                                              VMMC_RES_ID_t nResId,
                                              IFX_uint32_t *pTimeslot);
extern IFX_int32_t   VMMC_RES_HDLC_Write     (VMMC_RES_ID_t nResId,
                                              IFX_uint8_t *pBuf,  /* Charles */
                                              IFX_int32_t nLen);
extern IFX_int32_t   VMMC_RES_HDLC_InfoGet   (VMMC_DEVICE *pDev,
                                              IFX_uint16_t nResNr,
                                              IFX_char_t *pBuf,
                                              IFX_int32_t nLen);

extern IFX_void_t    irq_VMMC_RES_HDLC_BufferReadySet (VMMC_RES_ID_t nResId);

extern IFX_void_t    irq_VMMC_RES_HDLC_DD_MBX_Handler (VMMC_DEVICE *pDev);
#endif /* VMMC_FEAT_HDLC */

extern IFX_int32_t VMMC_RES_CPTD_Allocate (VMMC_CHANNEL *pCh,
                                           IFX_uint8_t *pResNr);
extern IFX_int32_t VMMC_RES_CPTD_Release (VMMC_CHANNEL *pCh,
                                          IFX_uint8_t nResNr);
#endif /* _DRV_VMMC_RES_H */
