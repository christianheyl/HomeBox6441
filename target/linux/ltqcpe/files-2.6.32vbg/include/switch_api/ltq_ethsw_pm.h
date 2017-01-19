/****************************************************************************
                              Copyright (c) 2010
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************
   \file ifx_ethsw_pm.h
   \remarks power management header file 
 ****************************************************************************/
#ifndef _IFX_ETHSW_PM_H_
#define _IFX_ETHSW_PM_H_

#include <ifx_ethsw_api.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#if 0
// Support power management of external Tantos3G
typedef enum
{
	IFX_ETHSW_PM_MODULENR_TANTOS3G	= 1,
    IFX_ETHSW_PM_MODULENR_GSWIP		= 2,
    IFX_ETHSW_PM_MODULENR_MAX		= 3
} IFX_PSB6970__t;
#endif

#define		IFX_ETHSW_PM_MODULENR_TANTOS3G	0x10
#define		IFX_ETHSW_PM_MODULENR_GSWIP		0x20

typedef struct {
	IFX_void_t                  *pCoreDev;
    IFX_boolean_t               bPMThreadEnable;
#ifndef IFXOS_SUPPORT 
	struct task_struct          *pPMThread;
    wait_queue_head_t           PHY_wait;
#endif
#ifdef CONFIG_IFX_ETHSW_API_COC_PMCU
	IFX_void_t                  *pPmcuCtx;
#endif
    IFX_void_t                  *pPlatCtx;
} IFX_ETHSW_PM_CTX_t;

#ifdef IFXOS_SUPPORT 
	#define IFX_ETHSW_STACKSIZE 2048
	#define IFX_ETHSW_PRIORITY  64
	IFXOS_ThreadCtrl_t PHY_ThrCntrl;
#endif

IFX_void_t *IFX_ETHSW_PM_powerManegementInit(IFX_void_t *pDev, IFX_uint8_t nModuleNr);
IFX_return_t IFX_ETHSW_PM_powerManegementCleanUp(IFX_void_t *pCtx);
IFX_boolean_t IFX_ETHSW_PM_powerManegementStatusGet(IFX_void_t *pCtx);
IFX_return_t IFX_ETHSW_powerStateD0(IFX_void_t *pCtx);
IFX_return_t IFX_ETHSW_powerStateD1(IFX_void_t *pCtx);
IFX_return_t IFX_ETHSW_powerManagementActivated(IFX_void_t *pCtx);
IFX_return_t IFX_ETHSW_powerManagementDeActivated(IFX_void_t *pCtx);
IFX_return_t IFX_ETHSW_PM_linkForceSet(IFX_void_t *pCtx, IFX_uint8_t nPHYIdx, IFX_boolean_t bLinkForce);
//IFX_return_t IFX_PSB6970_allPHY_powerup(IFX_void_t *pDevCtx);
//IFX_return_t IFX_PSB6970_allPHY_powerdown(IFX_void_t *pDevCtx);

#endif    /* _IFX_ETHSW_PM_H_ */
