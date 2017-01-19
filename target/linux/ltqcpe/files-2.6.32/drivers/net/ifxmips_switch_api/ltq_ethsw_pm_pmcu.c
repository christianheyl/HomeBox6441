/**************************************************************************** Copyright (c) 2010
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************
   \file ifx_ethsw_pm_pmcu.c
   \remarks implement power management abstraction for pmcu module
 *****************************************************************************/
#include <ltq_ethsw_pm.h>
#include <ltq_ethsw_pm_plat.h>
#include <ltq_ethsw_pm_pmcu.h>
#include <ltq_lxfreq.h>
#include <linux/cpufreq.h>

IFX_ETHSW_PM_PMCUCtx_t *gPM_pmcuCtx=IFX_NULL;

IFX_PMCU_RETURN_t IFX_ETHSW_PM_PMCU_stateRequest( IFX_PMCU_STATE_t pmcuState);
IFX_PMCU_RETURN_t IFX_ETHSW_PM_PMCU_stateGet( IFX_PMCU_STATE_t *pmcuState );
IFX_PMCU_RETURN_t IFX_ETHSW_PM_PMCU_preCB( IFX_PMCU_MODULE_t pmcuModule, IFX_PMCU_STATE_t newState, IFX_PMCU_STATE_t oldState);
IFX_PMCU_RETURN_t IFX_ETHSW_PM_PMCU_postCB( IFX_PMCU_MODULE_t pmcuModule, IFX_PMCU_STATE_t newState, IFX_PMCU_STATE_t oldState);
IFX_PMCU_RETURN_t IFX_ETHSW_PM_PMCU_switch( IFX_PMCU_PWR_STATE_ENA_t pmcuPwrStateEna);
IFX_int_t IFX_ETHSW_PM_PMCU_Link_Status( void);


#ifdef CONFIG_CPU_FREQ
/* Linux CPUFREQ support start */
extern struct list_head ltq_lxfreq_head_mod_list_g;

struct LTQ_LXFREQ_MODSTRUCT ethsw_pm_lxfreq_mod_g = {
	.name							= "Switch/GPhy support",
	.pmcuModule						= IFX_PMCU_MODULE_SWITCH,
	.pmcuModuleNr					= 0,
	.powerFeatureStat				= IFX_PMCU_PWR_STATE_ON,
	.ltq_lxfreq_state_get			= IFX_ETHSW_PM_PMCU_stateGet,
	.ltq_lxfreq_pwr_feature_switch	= IFX_ETHSW_PM_PMCU_switch,
};

static int ethsw_pm_cpufreq_notifier(struct notifier_block *nb, 
									 unsigned long val, void *data);
static struct notifier_block ethsw_pm_cpufreq_notifier_block = {
	.notifier_call  = ethsw_pm_cpufreq_notifier
};

/* keep track of frequency transitions */
static int
ethsw_pm_cpufreq_notifier(struct notifier_block *nb, unsigned long val, 
						  void *data)
{
	struct cpufreq_freqs *freq = data;
	IFX_PMCU_STATE_t new_State,old_State;
	IFX_PMCU_RETURN_t ret;

	new_State = ltq_lxfreq_get_ps_from_khz(freq->new);
	if(new_State == IFX_PMCU_STATE_INVALID) {
		return NOTIFY_STOP_MASK | (IFX_PMCU_MODULE_SWITCH<<4);
	}
	old_State = ltq_lxfreq_get_ps_from_khz(freq->old);
	if(old_State == IFX_PMCU_STATE_INVALID) {
		return NOTIFY_STOP_MASK | (IFX_PMCU_MODULE_SWITCH<<4);
	}
	if (val == CPUFREQ_PRECHANGE){
		ret = IFX_ETHSW_PM_PMCU_preCB(IFX_PMCU_MODULE_SWITCH, new_State, 
									  old_State);
		if (ret == IFX_PMCU_RETURN_DENIED) {
			return NOTIFY_STOP_MASK | (IFX_PMCU_MODULE_SWITCH<<4);
		}
		ret = IFX_ETHSW_PM_PMCU_stateRequest(new_State);
		if (ret == IFX_PMCU_RETURN_DENIED) {
			return NOTIFY_STOP_MASK | (IFX_PMCU_MODULE_SWITCH<<4);
		}
	} else if (val == CPUFREQ_POSTCHANGE){
		ret = IFX_ETHSW_PM_PMCU_postCB(IFX_PMCU_MODULE_SWITCH, new_State, 
									   old_State);
		if (ret == IFX_PMCU_RETURN_DENIED) {
			return NOTIFY_STOP_MASK | (IFX_PMCU_MODULE_SWITCH<<4);
		}
	}else{
		return NOTIFY_OK | (IFX_PMCU_MODULE_SWITCH<<4);
	}
	return NOTIFY_OK | (IFX_PMCU_MODULE_SWITCH<<4);
}
#endif
/* Linux CPUFREQ support end */


#ifdef CONFIG_IFX_PMCU
/* define dependency list;  state D0D3 means don't care */
/* static declaration is necessary to let gcc accept this static initialisation. */
static IFX_PMCU_MODULE_DEP_t depList=
{
    1,
    {
        {IFX_PMCU_MODULE_CPU, 0, IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D0D3, IFX_PMCU_STATE_D0D3, IFX_PMCU_STATE_D0D3}
    }
};
#endif
/**
   This is init function.
   \param pDevCtx  This parameter is a pointer to the IFX_ETHSW_PM_PMCUCtx_t context.
   \param nModuleNr  This parameter is a module number for PMCU module
   \return Return value as follows:
   - IFX_SUCCESS: if successful
*/
IFX_void_t *IFX_ETHSW_PM_PMCU_Init(IFX_void_t *pCtx, IFX_uint8_t nModuleNr)
{
	IFX_ETHSW_PM_PMCUCtx_t *pPmcuCtx;
#ifdef CONFIG_IFX_PMCU
    IFX_PMCU_REGISTER_t pmcuRegister;
#endif
	// allocate memory space.
/*    IFXOS_PRINT_INT_RAW("%s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__); */
    pPmcuCtx = (IFX_ETHSW_PM_PMCUCtx_t*) IFXOS_BlockAlloc (sizeof (IFX_ETHSW_PM_PMCUCtx_t));
    pPmcuCtx->pPMCtx = pCtx;
    pPmcuCtx->ePMCU_State = IFX_PMCU_STATE_D0;
//	pPmcuCtx->ePMCU_State = IFX_ETHSW_PM_PMCU_Link_Status();
    pPmcuCtx->nModuleNr = nModuleNr;
    // assgin to global pointer
    gPM_pmcuCtx = pPmcuCtx;
//	register nModuleNr and set callback function
#ifdef CONFIG_IFX_PMCU
    memset (&pmcuRegister, 0, sizeof(IFX_PMCU_REGISTER_t));
    pmcuRegister.pmcuModule=IFX_PMCU_MODULE_SWITCH;
    pmcuRegister.pmcuModuleNr=0;
    pmcuRegister.pmcuModuleDep = &depList;
    pmcuRegister.ifx_pmcu_state_change=IFX_ETHSW_PM_PMCU_stateRequest; 
    pmcuRegister.ifx_pmcu_state_get=IFX_ETHSW_PM_PMCU_stateGet;
    pmcuRegister.ifx_pmcu_pwr_feature_switch=IFX_ETHSW_PM_PMCU_switch;
    pmcuRegister.pre=IFX_ETHSW_PM_PMCU_preCB;
    pmcuRegister.post=IFX_ETHSW_PM_PMCU_postCB;
    if ( ifx_pmcu_register ( &pmcuRegister ) != IFX_PMCU_RETURN_SUCCESS) {
    	IFXOS_PRINT_INT_RAW("ERROR: %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);
        IFXOS_BlockFree(pPmcuCtx);
        pPmcuCtx = IFX_NULL;
		gPM_pmcuCtx = IFX_NULL;
    }
#endif

#ifdef CONFIG_CPU_FREQ
/* do not register currently!
   if ( cpufreq_register_notifier(&ethsw_pm_cpufreq_notifier_block, 
                                  CPUFREQ_TRANSITION_NOTIFIER) ) {  
       printk(KERN_ERR "Fail in registering ETHSW_PM to CPUFREQ\n");
   }                                                                
*/
   list_add_tail(&ethsw_pm_lxfreq_mod_g.list, &ltq_lxfreq_head_mod_list_g);
#endif
   return pPmcuCtx;
}

/**
   This is a cleanup function.
   \param pDevCtx  This parameter is a pointer to the IFX_ETHSW_PM_PMCUCtx_t context.

   \return Return value as follows:
   - IFX_SUCCESS: if successful
*/
IFX_return_t IFX_ETHSW_PM_PMCU_CleanUp(IFX_void_t *pCtx)
{
	IFX_ETHSW_PM_PMCUCtx_t *pPmcuCtx = (IFX_ETHSW_PM_PMCUCtx_t *)pCtx;
#ifdef CONFIG_IFX_PMCU
	IFX_PMCU_REGISTER_t pmcuUnRegister;
    // unregister nModuleNr
    pmcuUnRegister.pmcuModule = IFX_PMCU_MODULE_SWITCH; 
    pmcuUnRegister.pmcuModuleNr = pPmcuCtx->nModuleNr;
    if (ifx_pmcu_unregister(&pmcuUnRegister) == IFX_ERROR) {
    	IFXOS_PRINT_INT_RAW("[%d]: unregister error\n",__LINE__);
        //return IFX_ERROR;
	}
#endif
	if (pPmcuCtx != IFX_NULL) {
		IFXOS_BlockFree(pPmcuCtx);
		pPmcuCtx = IFX_NULL;
	}

#ifdef CONFIG_CPU_FREQ
    if ( cpufreq_unregister_notifier(&ethsw_pm_cpufreq_notifier_block,
                                     CPUFREQ_TRANSITION_NOTIFIER) ) { 
    printk(KERN_ERR "Fail in unregistering ETHSW_PM from CPUFREQ\n"); 
    }                                                                 
    list_del(&ethsw_pm_lxfreq_mod_g.list);                            
#endif                                                                
	return IFX_SUCCESS;
}

/**
   This is the callback function to request pmcu state in the 
   power management hardware-dependent module
   \param pmcuState This parameter is a PMCU state.
   \return Return value as follows:
   - IFX_SUCCESS: if successful
*/

IFX_PMCU_RETURN_t IFX_ETHSW_PM_PMCU_stateRequest( IFX_PMCU_STATE_t pmcuState)
{
	if( gPM_pmcuCtx == IFX_NULL)
		return IFX_PMCU_RETURN_ERROR;
	/* IFXOS_PRINT_INT_RAW("%s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__); */
	switch(pmcuState) {
		case IFX_PMCU_STATE_D0:
			/* D0 : all internal and external GPHYs and switch are in OnState(no power Saving feature active) */
//			IFXOS_PRINT_INT_RAW(  "IFX_PMCU_STATE_D0\n");
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
			IFX_ETHSW_PHY_Link_Up(gPM_pmcuCtx->pPMCtx);
#endif
//			IFX_ETHSW_powerStateD0(gPM_pmcuCtx->pPMCtx);
			break;
		case IFX_PMCU_STATE_D1: 
			/* D1 : only the external GPHYs are in OnState; 
			the internal GPHYs are in power saving mode and switch is clock gated */
//			IFX_ETHSW_powerStateD1(gPM_pmcuCtx->pPMCtx);
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
			IFX_ETHSW_EXT_PHY_Link_Up(gPM_pmcuCtx->pPMCtx);
			IFX_ETHSW_INT_PHY_Link_Down(gPM_pmcuCtx->pPMCtx);
#endif
//			IFXOS_PRINT_INT_RAW(  "IFX_PMCU_STATE_D1\n");
			break;
		case IFX_PMCU_STATE_D2: 
			/*  D2 : only internal GPHYs and switch are in OnState; 
			external GPHYs are in power saving mode. */
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
			IFX_ETHSW_INT_PHY_Link_Up(gPM_pmcuCtx->pPMCtx);
			IFX_ETHSW_EXT_PHY_Link_Down(gPM_pmcuCtx->pPMCtx);
#endif
//			IFXOS_PRINT_INT_RAW(  "IFX_PMCU_STATE_D2\n");
			break;
		case IFX_PMCU_STATE_D3: 
			/* D3 : all are in power saving mode	*/
#if (defined(CONFIG_VR9) || defined(CONFIG_AR10))
			IFX_ETHSW_PHY_Link_Down(gPM_pmcuCtx->pPMCtx);
#endif
//			IFXOS_PRINT_INT_RAW(  "IFX_PMCU_STATE_D3\n");
			break;
		case IFX_PMCU_STATE_INVALID:
		case IFX_PMCU_STATE_D0D3:
		default:
			return IFX_PMCU_RETURN_ERROR;
	}
	gPM_pmcuCtx->ePMCU_State = pmcuState;
	return IFX_PMCU_RETURN_SUCCESS;
}

IFX_int_t IFX_ETHSW_PM_PMCU_Link_Status( void)
{
	int ret;
	ret = IFX_ETHSW_PHY_Link_Status(gPM_pmcuCtx->pPMCtx);
/*	IFXOS_PRINT_INT_RAW("%s:%s:%d ,ret:%d\n", __FILE__, __FUNCTION__, __LINE__,ret);*/
	gPM_pmcuCtx->ePMCU_State = ret;
	return (ret);
}

IFX_PMCU_RETURN_t IFX_ETHSW_PM_PMCU_switch( IFX_PMCU_PWR_STATE_ENA_t pmcuPwrStateEna)
{
/*	IFXOS_PRINT_INT_RAW("%s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__); */
	 if (pmcuPwrStateEna == IFX_PMCU_PWR_STATE_ON) {
		IFX_ETHSW_PHY_Link_Up(gPM_pmcuCtx->pPMCtx);
/*        printk(KERN_DEBUG "Power saving features are switched on\n"); */
        return IFX_PMCU_RETURN_SUCCESS;
    }
    if (pmcuPwrStateEna == IFX_PMCU_PWR_STATE_OFF) {
/*        printk(KERN_DEBUG "Power  saving features are switched off\n"); */
		IFX_ETHSW_PHY_Link_Down(gPM_pmcuCtx->pPMCtx);
        return IFX_PMCU_RETURN_SUCCESS;
    }
	 return IFX_PMCU_RETURN_SUCCESS;
}


/**
   This is the callback function to get pmcu state in the 
   power management hardware-dependent module
   \return Return value as follows:
   - IFX_SUCCESS: if successful
*/
IFX_PMCU_RETURN_t IFX_ETHSW_PM_PMCU_stateGet( IFX_PMCU_STATE_t *pmcuState )
{
//	*pmcuState = gPM_pmcuCtx->ePMCU_State;
	*pmcuState = IFX_ETHSW_PM_PMCU_Link_Status();
//	IFXOS_PRINT_INT_RAW("%s:%s:%d, value:0x%08x \n", __FILE__, __FUNCTION__, __LINE__,*pmcuState);
	return IFX_PMCU_RETURN_SUCCESS;
}

IFX_PMCU_RETURN_t IFX_ETHSW_PM_PMCU_preCB( IFX_PMCU_MODULE_t pmcuModule, IFX_PMCU_STATE_t newState, IFX_PMCU_STATE_t oldState)
{
/*	IFXOS_PRINT_INT_RAW("%s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);*/
	return IFX_PMCU_RETURN_SUCCESS;
}

IFX_PMCU_RETURN_t IFX_ETHSW_PM_PMCU_postCB( IFX_PMCU_MODULE_t pmcuModule, IFX_PMCU_STATE_t newState, IFX_PMCU_STATE_t oldState)
{
/*	IFXOS_PRINT_INT_RAW("%s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__);*/
    return IFX_PMCU_RETURN_SUCCESS;
}

IFX_return_t IFX_ETHSW_PM_PMCU_StateReq(IFX_PMCU_STATE_t newState)
{
/*	IFXOS_PRINT_INT_RAW("%s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__); */
	if (ifx_pmcu_state_req ( IFX_PMCU_MODULE_SWITCH, 0, newState) != IFX_PMCU_RETURN_SUCCESS)
		return IFX_ERROR;
	return IFX_SUCCESS;
}
