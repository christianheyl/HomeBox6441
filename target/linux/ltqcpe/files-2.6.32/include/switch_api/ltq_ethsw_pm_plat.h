/****************************************************************************
                              Copyright (c) 2010
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************
   \file ifx_ethsw_pm_plat.h
   \remarks power management header file for platform dependency. 
 ****************************************************************************/
#ifndef _IFX_ETHSW_PM_PLAT_H_
#define _IFX_ETHSW_PM_PLAT_H_
#include <ifx_ethsw_api.h>

/*********************************************/
/* Structure and Enumeration Type Defintions */
/*********************************************/
#if defined(CONFIG_VR9) || defined(CONFIG_AR10)
	#define PHY_NO 6
#endif /* CONFIG_VR9 */
#if defined(CONFIG_AR9)
	#define PHY_NO 5
#endif /* CONFIG_AR9 */
typedef struct {
	IFX_boolean_t		bStatus; // for debug
	IFX_boolean_t		bLinkForce;
	IFX_uint8_t			nPHYAddr;
} IFX_ETHSW_PHY_t;

typedef struct {
    IFX_void_t			*pPMCtx;
    IFX_uint8_t			nPHYNum;
    IFX_ETHSW_PHY_t		PHY[PHY_NO];
} IFX_ETHSW_PMPlatCTX_t;

/************************/
/* Function Propotype   */
/************************/
IFX_void_t *IFX_ETHSW_PM_PLAT_Init(IFX_void_t *pCtx, IFX_uint8_t nModuleNr);
IFX_return_t IFX_ETHSW_PM_PLAT_CleanUp(IFX_void_t *pCtx);
IFX_boolean_t IFX_ETHSW_PHY_MDstatusGet(IFX_void_t *pDevCtx, IFX_uint8_t nPHYAddr);
IFX_boolean_t IFX_ETHSW_PHY_statusSet(IFX_void_t *pDevCtx, IFX_uint8_t nPHYIdx, IFX_boolean_t bStatus);
IFX_boolean_t IFX_ETHSW_PHY_statusGet(IFX_void_t *pDevCtx, IFX_uint8_t nPHYIdx);
IFX_return_t IFX_ETHSW_PHY_powerDown(IFX_void_t *pDevCtx, IFX_uint8_t nPHYNum);
IFX_return_t IFX_ETHSW_PHY_powerUp(IFX_void_t *pDevCtx, IFX_uint8_t nPHYNum);
IFX_return_t IFX_ETHSW_AllPHY_powerDown(IFX_void_t *pDevCtx, IFX_void_t *pPlatCtx);
IFX_return_t IFX_ETHSW_AllPHY_powerUp(IFX_void_t *pDevCtx, IFX_void_t *pPlatCtx);
IFX_int_t IFX_ETHSW_AllPHY_LinkStatus(IFX_void_t *pDevCtx, IFX_void_t *pPlatCtx);
IFX_return_t IFX_ETHSW_PM_PLAT_linkForceSet(IFX_void_t *pPlatCtx, IFX_uint8_t nPHYIdx, IFX_boolean_t bLinkForce);
IFX_return_t IFX_ETHSW_PM_PLAT_linkForceGet(IFX_void_t *pPlatCtx, IFX_uint8_t nPHYIdx, IFX_boolean_t *pLinkForce);
IFX_return_t IFX_ETHSW_PHY_Link_Up(IFX_void_t *pCtx);
IFX_int_t IFX_ETHSW_PHY_Link_Status(IFX_void_t *pCtx);
IFX_return_t IFX_ETHSW_PHY_Link_Down(IFX_void_t *pCtx);
IFX_return_t IFX_ETHSW_EXT_PHY_Link_Up(IFX_void_t *pCtx) ;
IFX_return_t IFX_ETHSW_INT_PHY_Link_Down(IFX_void_t *pCtx);
IFX_return_t IFX_ETHSW_EXT_PHY_Link_Down(IFX_void_t *pCtx);
IFX_return_t IFX_ETHSW_INT_PHY_Link_Up(IFX_void_t *pCtx);
IFX_return_t IFX_ETHSW_AllintPHY_powerUp(IFX_void_t *pDevCtx, IFX_void_t *pPlatCtx);
IFX_return_t IFX_ETHSW_AllextPHY_powerUp(IFX_void_t *pDevCtx, IFX_void_t *pPlatCtx);

#endif    /* _IFX_ETHSW_PM_PLAT_H_ */
