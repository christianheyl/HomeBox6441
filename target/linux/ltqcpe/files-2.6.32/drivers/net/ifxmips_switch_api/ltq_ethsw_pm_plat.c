/****************************************************************************
                              Copyright (c) 2010
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************
   \file ifx_ethsw_pm_plat.c
   \remarks implement power management platform dependency code
 *****************************************************************************/
#include <ltq_ethsw_pm.h>
#include <ltq_ethsw_pm_plat.h>
#include <ltq_ethsw_pm_pmcu.h>

#if defined(CONFIG_AR9)
	#include <ltq_tantos_core.h>
#endif /* CONFIG_AR9 */
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
	#include <ltq_flow_core.h>
#endif /* CONFIG_VR9 */

IFX_ETHSW_PMPlatCTX_t *gPMPlatCtx = IFX_NULL;
/**
   This is init function in the power management hardware-dependent module.
   \param pDevCtx  This parameter is a pointer to the IFX_ETHSW_PMPlatCTX_t context.
   \param nModuleNr  This parameter is a module number for PMCU module
   \return Return value as follows:
   - IFX_SUCCESS: if successful
*/
IFX_void_t *IFX_ETHSW_PM_PLAT_Init(IFX_void_t *pCtx, IFX_uint8_t nModuleNr)
{
	IFX_ETHSW_PMPlatCTX_t *pPMPlatCtx;
    IFX_uint8_t i;
    
    pPMPlatCtx = (IFX_ETHSW_PMPlatCTX_t*) IFXOS_BlockAlloc (sizeof (IFX_ETHSW_PMPlatCTX_t));
    if ( pPMPlatCtx == IFX_NULL) {
    	IFXOS_PRINT_INT_RAW("ERROR: : %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);
        return IFX_NULL;
    }
    // init Tantos3G on Ar9 platform
    pPMPlatCtx->pPMCtx = pCtx;
    pPMPlatCtx->nPHYNum = PHY_NO;
    pPMPlatCtx->PHY[0].nPHYAddr = 0;
    pPMPlatCtx->PHY[1].nPHYAddr = 1;
#if defined(CONFIG_VR9)
    pPMPlatCtx->PHY[2].nPHYAddr = 0x11;
    pPMPlatCtx->PHY[3].nPHYAddr = 0x12;
	pPMPlatCtx->PHY[4].nPHYAddr = 0x13;
#if defined(CONFIG_GE_MODE)
	pPMPlatCtx->PHY[5].nPHYAddr = 0x5;
#else
	pPMPlatCtx->PHY[5].nPHYAddr = 0x14;
#endif /*CONFIG_GE_MODE*/
#else
	pPMPlatCtx->PHY[2].nPHYAddr = 2;
    pPMPlatCtx->PHY[3].nPHYAddr = 3;
	pPMPlatCtx->PHY[4].nPHYAddr = 4;
#if defined(CONFIG_AR10)
	pPMPlatCtx->PHY[5].nPHYAddr = 5;
#endif
#endif
    for (i=0; i<PHY_NO; i++) {
    	pPMPlatCtx->PHY[i].bStatus = IFX_TRUE;
        pPMPlatCtx->PHY[i].bLinkForce = IFX_FALSE;
    }
	gPMPlatCtx = pPMPlatCtx;
	return pPMPlatCtx;
}

/**
   This is cleanup function in the power management hardware-dependent module.
   \param pDevCtx  This parameter is a pointer to the IFX_ETHSW_PMPlatCTX_t context.
   \return Return value as follows:
   - IFX_SUCCESS: if successful
*/
IFX_return_t IFX_ETHSW_PM_PLAT_CleanUp(IFX_void_t *pCtx)
{
	IFX_ETHSW_PMPlatCTX_t *pPMPlatCtx = (IFX_ETHSW_PMPlatCTX_t *)pCtx;
	if (pPMPlatCtx != IFX_NULL) {
		IFXOS_BlockFree(pPMPlatCtx);
		pPMPlatCtx = IFX_NULL;
	}
	return IFX_SUCCESS;
}
/**
   This is IFX_ETHSW_PHY_MDstatusGet function to call switch core layer.
   \param pDevCtx  This parameter is a pointer to the IFX_PSB6970_switchDev_t context.
   \return Return the result for calling function.
*/
IFX_boolean_t IFX_ETHSW_PHY_MDstatusGet(IFX_void_t *pDevCtx, IFX_uint8_t nPHYAddr)
{
	IFX_boolean_t Status;
	// The Tantos CoC feature is only dedicated to the Tantos internal PHYs
    //if ( nPHYAddr == 4 ) // this is a external PHY.
    //    Status = IFX_PSB6970_PHY_linkStatusGet(pDevCtx, nPHYAddr);
    //else 
#if defined(CONFIG_AR9)
	Status = IFX_PSB6970_PHY_mediumDetectStatusGet(pDevCtx, nPHYAddr);
#endif /*CONFIG_AR9 */ 
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
	Status = IFX_FLOW_PHY_mediumDetectStatusGet(pDevCtx, nPHYAddr);
#endif /*CONFIG_VR9 */ 
	return Status;
}

/**
   This is IFX_ETHSW_PHY_statusSet function to store MD status into PMPlatCtx structure
   \param pDevCtx  This parameter is a pointer to the IFX_ETHSW_PMPlatCTX_t context.
   \param nPHYIdx  This parameter is PHY index.
   \return Return the result for calling function.
*/
IFX_boolean_t IFX_ETHSW_PHY_statusSet(IFX_void_t *pDevCtx, IFX_uint8_t nPHYIdx, IFX_boolean_t bStatus)
{
	IFX_ETHSW_PMPlatCTX_t *pPMPlatCtx = (IFX_ETHSW_PMPlatCTX_t *)pDevCtx;
	pPMPlatCtx->PHY[nPHYIdx].bStatus = bStatus;
	return IFX_SUCCESS;
}

/**
   This is IFX_ETHSW_PHY_statusGet function to store MD status into PMPlatCtx structure
   \param pDevCtx  This parameter is a pointer to the IFX_ETHSW_PMPlatCTX_t context.
   \param nPHYIdx  This parameter is PHY index.
   \return Return the result for calling function.
*/
IFX_boolean_t IFX_ETHSW_PHY_statusGet(IFX_void_t *pDevCtx, IFX_uint8_t nPHYIdx)
{
	IFX_ETHSW_PMPlatCTX_t *pPMPlatCtx = (IFX_ETHSW_PMPlatCTX_t *)pDevCtx;
	return pPMPlatCtx->PHY[nPHYIdx].bStatus;
}

/**
   This is IFX_ETHSW_PHY_powerDown function to call switch core layer.
   \param pDevCtx  This parameter is a pointer to the IFX_PSB6970_switchDev_t context.
   \return Return the result for calling function.
*/
IFX_return_t IFX_ETHSW_PHY_powerDown(IFX_void_t *pDevCtx, IFX_uint8_t nPHYAddr)
{
#if defined(CONFIG_AR9)
	return IFX_PSB6970_PHY_PDN_Set(pDevCtx, nPHYAddr);
#endif
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
	return IFX_FLOW_PHY_PDN_Set(pDevCtx, nPHYAddr);
#endif /* CONFIG_VR9 */
}

/**
   This is IFX_ETHSW_PHY_powerUp function to call switch core layer.
   \param pDevCtx  This parameter is a pointer to the IFX_PSB6970_switchDev_t context.
   \return Return the result for calling function.
*/
IFX_return_t IFX_ETHSW_PHY_powerUp(IFX_void_t *pDevCtx, IFX_uint8_t nPHYAddr)
{
#if defined(CONFIG_AR9)
	return IFX_PSB6970_PHY_PDN_Clear(pDevCtx, nPHYAddr);
#endif
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
	return IFX_FLOW_PHY_PDN_Clear(pDevCtx, nPHYAddr);
#endif /* CONFIG_VR9 */
}

/**
   This is IFX_ETHSW_AllPHY_powerDown function to call switch core layer.
   \param pDevCtx  This parameter is a pointer to the IFX_PSB6970_switchDev_t context.
   \return Return the result for calling function.
*/
IFX_return_t IFX_ETHSW_AllPHY_powerDown(IFX_void_t *pDevCtx, IFX_void_t *pPlatCtx)
{
	IFX_ETHSW_PMPlatCTX_t *pPMPlatCtx = (IFX_ETHSW_PMPlatCTX_t *)pPlatCtx;
	IFX_uint8_t i;
	for (i=0; i<pPMPlatCtx->nPHYNum; i++) {
		if (pPMPlatCtx->PHY[i].bLinkForce == IFX_TRUE)
			continue;
#if defined(CONFIG_AR9)
		if (IFX_PSB6970_PHY_PDN_Set(pDevCtx ,pPMPlatCtx->PHY[i].nPHYAddr) != IFX_SUCCESS) {
			IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_PHY_powerDown[%d] error\n",__LINE__,i);
			return IFX_ERROR;
		}
#endif /* CONFIG_AR9 */
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
		if (IFX_FLOW_PHY_PDN_Set(pDevCtx ,pPMPlatCtx->PHY[i].nPHYAddr) != IFX_SUCCESS) {
			IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_PHY_powerDown[%d] error\n",__LINE__,i);
			return IFX_ERROR;
		}
#endif /* CONFIG_VR9 */
		IFX_ETHSW_DEBUG_PRINT("PHY[%d] power down\n", i);
	}
	return IFX_SUCCESS;
}


IFX_return_t IFX_ETHSW_AllextPHY_powerDown(IFX_void_t *pDevCtx, IFX_void_t *pPlatCtx)
{
	IFX_ETHSW_PMPlatCTX_t *pPMPlatCtx = (IFX_ETHSW_PMPlatCTX_t *)pPlatCtx;
	IFX_uint8_t i;
	for (i=0; i<pPMPlatCtx->nPHYNum; i++) {
		if (pPMPlatCtx->PHY[i].bLinkForce == IFX_TRUE)
			continue;
		if ( i == 0 || i== 5) {
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
			if (IFX_FLOW_PHY_PDN_Set(pDevCtx ,pPMPlatCtx->PHY[i].nPHYAddr) != IFX_SUCCESS) {
				IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_PHY_powerDown[%d] error\n",__LINE__,i);
				return IFX_ERROR;
			}
#endif /* CONFIG_VR9 */
		}
		if ( i == 1 ) {
#if defined(CONFIG_VR9)
			if (IFX_FLOW_PHY_PDN_Set(pDevCtx ,pPMPlatCtx->PHY[i].nPHYAddr) != IFX_SUCCESS) {
				IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_PHY_powerDown[%d] error\n",__LINE__,i);
				return IFX_ERROR;
			}
#endif /* CONFIG_VR9 */
		}
		IFX_ETHSW_DEBUG_PRINT("PHY[%d] power down\n", i);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_ETHSW_AllintPHY_powerDown(IFX_void_t *pDevCtx, IFX_void_t *pPlatCtx)
{
	IFX_ETHSW_PMPlatCTX_t *pPMPlatCtx = (IFX_ETHSW_PMPlatCTX_t *)pPlatCtx;
	IFX_uint8_t i;
	for (i=0; i<pPMPlatCtx->nPHYNum; i++) {
		if (pPMPlatCtx->PHY[i].bLinkForce == IFX_TRUE)
			continue;
		if ( i == 2 || i == 4 ) {
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
			if (IFX_FLOW_PHY_PDN_Set(pDevCtx ,pPMPlatCtx->PHY[i].nPHYAddr) != IFX_SUCCESS) {
				IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_PHY_powerDown[%d] error\n",__LINE__,i);
				return IFX_ERROR;
			}
#endif /* CONFIG_VR9 */
		}
		IFX_ETHSW_DEBUG_PRINT("PHY[%d] power down\n", i);
	}
	return IFX_SUCCESS;
}

/**
   This is IFX_ETHSW_AllPHY_powerUp function to call switch core layer.
   \param pDevCtx  This parameter is a pointer to the IFX_PSB6970_switchDev_t context.
   \return Return the result for calling function.
*/
IFX_return_t IFX_ETHSW_AllPHY_powerUp(IFX_void_t *pDevCtx, IFX_void_t *pPlatCtx)
{
	IFX_ETHSW_PMPlatCTX_t *pPMPlatCtx = (IFX_ETHSW_PMPlatCTX_t *)pPlatCtx;
	IFX_uint8_t i;

	for (i=0; i<pPMPlatCtx->nPHYNum; i++) {
		if (pPMPlatCtx->PHY[i].bLinkForce == IFX_TRUE)
			continue;
#if defined(CONFIG_AR9)
		if (IFX_PSB6970_PHY_PDN_Clear(pDevCtx ,pPMPlatCtx->PHY[i].nPHYAddr) != IFX_SUCCESS) {
			IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_PHY_powerUp[%d] error\n",__LINE__,i);
			return IFX_ERROR;
		}
#endif
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
		if (IFX_FLOW_PHY_PDN_Clear(pDevCtx ,pPMPlatCtx->PHY[i].nPHYAddr) != IFX_SUCCESS) {
			IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_PHY_powerUp[%d] error\n",__LINE__,i);
			return IFX_ERROR;
		}
#endif
		IFX_ETHSW_DEBUG_PRINT("PHY[%d] power up\n", i);
	}
	return IFX_SUCCESS;
}

IFX_int_t IFX_ETHSW_AllPHY_LinkStatus(IFX_void_t *pDevCtx, IFX_void_t *pPlatCtx)
{
	IFX_ETHSW_PMPlatCTX_t *pPMPlatCtx = (IFX_ETHSW_PMPlatCTX_t *)pPlatCtx;
	IFX_uint8_t i, link = 0, status =0, external=0, internal = 0;
	for (i=0; i<pPMPlatCtx->nPHYNum; i++) {
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
#if defined(CONFIG_GE_MODE)
	if ( i!=3)
		link |= (IFX_FLOW_PHY_Link_Status_Get(pDevCtx ,pPMPlatCtx->PHY[i].nPHYAddr) << i);
#endif
#if defined(CONFIG_FE_MODE)
	link |= (IFX_FLOW_PHY_Link_Status_Get(pDevCtx ,pPMPlatCtx->PHY[i].nPHYAddr) << i);
#endif
#endif
	}
	for (i=0; i<pPMPlatCtx->nPHYNum; i++) {
		if ( (i == 0) || (i == 1) || (i == 5 ) ) {
			if ( ((link >> i) & 0x1) == 1 ) {
				status = IFX_PMCU_STATE_D1;
				external = 1;
			}
		}
		if ( (i == 2) || (i == 4 ) || (i==3) ) {
			if ( ((link >> i) & 0x1) == 1 ) {
				status = IFX_PMCU_STATE_D2;
				internal = 1;
			}
		}
	}
//	IFXOS_PRINT_INT_RAW(" link [0x%08x]. external:%d, inetrnal:%d \n", link, external,internal);
	if (external && internal)
		status = IFX_PMCU_STATE_D0;
	if (!external)
		if (!internal)
			status = IFX_PMCU_STATE_D3;
	return (status);
}


/**
   This is IFX_ETHSW_AllPHY_powerUp function to call switch core layer.
   \param pDevCtx  This parameter is a pointer to the IFX_PSB6970_switchDev_t context.
   \return Return the result for calling function.
*/
IFX_return_t IFX_ETHSW_AllextPHY_powerUp(IFX_void_t *pDevCtx, IFX_void_t *pPlatCtx)
{
	IFX_ETHSW_PMPlatCTX_t *pPMPlatCtx = (IFX_ETHSW_PMPlatCTX_t *)pPlatCtx;
	IFX_uint8_t i;

	for (i=0; i<pPMPlatCtx->nPHYNum; i++) {
		if (pPMPlatCtx->PHY[i].bLinkForce == IFX_TRUE)
			continue;
		if ( i == 0 || i == 1 || i== 5) {
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
			if (IFX_FLOW_PHY_PDN_Clear(pDevCtx ,pPMPlatCtx->PHY[i].nPHYAddr) != IFX_SUCCESS) {
				IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_PHY_powerUp[%d] error\n",__LINE__,i);
				return IFX_ERROR;
			}
#endif
		}
		IFX_ETHSW_DEBUG_PRINT("PHY[%d] power up\n", i);
	}
	return IFX_SUCCESS;
}

/**
   This is IFX_ETHSW_AllPHY_powerUp function to call switch core layer.
   \param pDevCtx  This parameter is a pointer to the IFX_PSB6970_switchDev_t context.
   \return Return the result for calling function.
*/
IFX_return_t IFX_ETHSW_AllintPHY_powerUp(IFX_void_t *pDevCtx, IFX_void_t *pPlatCtx)
{
	IFX_ETHSW_PMPlatCTX_t *pPMPlatCtx = (IFX_ETHSW_PMPlatCTX_t *)pPlatCtx;
	IFX_uint8_t i;

	for (i=0; i<pPMPlatCtx->nPHYNum; i++) {
		if (pPMPlatCtx->PHY[i].bLinkForce == IFX_TRUE)
			continue;
		if ( i == 2 || i == 4) {
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
			if (IFX_FLOW_PHY_PDN_Clear(pDevCtx ,pPMPlatCtx->PHY[i].nPHYAddr) != IFX_SUCCESS) {
				IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_PHY_powerUp[%d] error\n",__LINE__,i);
				return IFX_ERROR;
			}
#endif
		}
		if (i == 1 ) {
#if defined(CONFIG_AR10)
			if (IFX_FLOW_PHY_PDN_Clear(pDevCtx ,pPMPlatCtx->PHY[i].nPHYAddr) != IFX_SUCCESS) {
				IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_PHY_powerUp[%d] error\n",__LINE__,i);
				return IFX_ERROR;
			}
#endif
		}
		IFX_ETHSW_DEBUG_PRINT("PHY[%d] power up\n", i);
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_ETHSW_PM_PLAT_linkForceSet(IFX_void_t *pPlatCtx, IFX_uint8_t nPHYIdx, IFX_boolean_t bLinkForce)
{
	IFX_ETHSW_PMPlatCTX_t *pPMPlatCtx = (IFX_ETHSW_PMPlatCTX_t *)pPlatCtx;
	pPMPlatCtx->PHY[nPHYIdx].bLinkForce = bLinkForce;
	return IFX_SUCCESS;
}

IFX_return_t IFX_ETHSW_PM_PLAT_linkForceGet(IFX_void_t *pPlatCtx, IFX_uint8_t nPHYIdx, IFX_boolean_t *pLinkForce)
{
	IFX_ETHSW_PMPlatCTX_t *pPMPlatCtx = (IFX_ETHSW_PMPlatCTX_t *)pPlatCtx;
	*pLinkForce = pPMPlatCtx->PHY[nPHYIdx].bLinkForce;
	return IFX_SUCCESS;
}

IFX_return_t IFX_ETHSW_EXT_PHY_Link_Up(IFX_void_t *pCtx) 
{
	IFX_ETHSW_PM_CTX_t *pPMCtx = (IFX_ETHSW_PM_CTX_t *)pCtx;
	if (IFX_ETHSW_AllextPHY_powerUp(pPMCtx->pCoreDev ,pPMCtx->pPlatCtx) != IFX_SUCCESS) {
		IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_PHY_Link_Up error\n",__LINE__);
		return IFX_ERROR;
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_ETHSW_INT_PHY_Link_Up(IFX_void_t *pCtx) 
{
	IFX_ETHSW_PM_CTX_t *pPMCtx = (IFX_ETHSW_PM_CTX_t *)pCtx;
	if (IFX_ETHSW_AllintPHY_powerUp(pPMCtx->pCoreDev ,pPMCtx->pPlatCtx) != IFX_SUCCESS) {
		IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_PHY_Link_Up error\n",__LINE__);
		return IFX_ERROR;
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_ETHSW_PHY_Link_Up(IFX_void_t *pCtx) 
{
	IFX_ETHSW_PM_CTX_t *pPMCtx = (IFX_ETHSW_PM_CTX_t *)pCtx;
	if (IFX_ETHSW_AllPHY_powerUp(pPMCtx->pCoreDev ,pPMCtx->pPlatCtx) != IFX_SUCCESS) {
		IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_PHY_Link_Up error\n",__LINE__);
		return IFX_ERROR;
	}
	return IFX_SUCCESS;
}

IFX_int_t IFX_ETHSW_PHY_Link_Status(IFX_void_t *pCtx) 
{
	int ret;
	IFX_ETHSW_PM_CTX_t *pPMCtx = (IFX_ETHSW_PM_CTX_t *)pCtx;
	ret = IFX_ETHSW_AllPHY_LinkStatus(pPMCtx->pCoreDev ,pPMCtx->pPlatCtx);
//	IFXOS_PRINT_INT_RAW("%s:%s:%d ,ret:%d\n", __FILE__, __FUNCTION__, __LINE__,ret);
	return (ret);
}

IFX_return_t IFX_ETHSW_EXT_PHY_Link_Down(IFX_void_t *pCtx) 
{
	IFX_ETHSW_PM_CTX_t *pPMCtx = (IFX_ETHSW_PM_CTX_t *)pCtx;
	if (IFX_ETHSW_AllextPHY_powerDown(pPMCtx->pCoreDev ,pPMCtx->pPlatCtx) != IFX_SUCCESS) {
		IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_AllPHY_powerDown error\n",__LINE__);
		return IFX_ERROR;
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_ETHSW_INT_PHY_Link_Down(IFX_void_t *pCtx) 
{
	IFX_ETHSW_PM_CTX_t *pPMCtx = (IFX_ETHSW_PM_CTX_t *)pCtx;
	if (IFX_ETHSW_AllintPHY_powerDown(pPMCtx->pCoreDev ,pPMCtx->pPlatCtx) != IFX_SUCCESS) {
		IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_AllPHY_powerDown error\n",__LINE__);
		return IFX_ERROR;
	}
	return IFX_SUCCESS;
}

IFX_return_t IFX_ETHSW_PHY_Link_Down(IFX_void_t *pCtx) 
{
	IFX_ETHSW_PM_CTX_t *pPMCtx = (IFX_ETHSW_PM_CTX_t *)pCtx;
	if (IFX_ETHSW_AllPHY_powerDown(pPMCtx->pCoreDev ,pPMCtx->pPlatCtx) != IFX_SUCCESS) {
		IFXOS_PRINT_INT_RAW(  "[%d]: IFX_ETHSW_AllPHY_powerDown error\n",__LINE__);
		return IFX_ERROR;
	}
	return IFX_SUCCESS;
}