/******************************************************************************

							   Copyright (c) 2010
							Lantiq Deutschland GmbH
					 Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
#ifndef _LTQ_LXFREQ_H_
#define _LTQ_LXFREQ_H_

#ifdef CONFIG_CPU_FREQ
typedef struct LTQ_LXFREQ_MODSTRUCT {
	struct list_head 		list;
	char*  					name;
	/** Module identifier */
	IFX_PMCU_MODULE_t		pmcuModule;
	/** instance identification of a Module;
	 *  values 0,1,2,..... (0=first instance) */
	unsigned char 			pmcuModuleNr;
	/** PowerFeature Status */
	IFX_PMCU_PWR_STATE_ENA_t	powerFeatureStat;
	/** Optional: Callback used to get module's power state.
	 *  Set to NULL if unused */
	IFX_PMCU_RETURN_t		(*ltq_lxfreq_state_get)  
									( IFX_PMCU_STATE_t *pmcuState );
	/** Callback used to enable/disable the power features of the module */
	IFX_PMCU_RETURN_t	(*ltq_lxfreq_pwr_feature_switch) ( 
									IFX_PMCU_PWR_STATE_ENA_t pmcuPwrStateEna );
} LTQ_LXFREQ_MODSTRUCT_t;



int ltq_lxfreq_state_req (IFX_PMCU_MODULE_t module, 
							unsigned char moduleNr, 
							IFX_PMCU_STATE_t newState);
IFX_PMCU_STATE_t ltq_lxfreq_get_ps_from_khz(unsigned int freqkhz);
unsigned int ltq_lxfreq_getfreq_khz(unsigned int cpu);
int ltq_lxfreq_state_change_disable(void);
int ltq_lxfreq_state_change_enable(void);
u32 ltq_count0_diff(u32 count_start, u32 count_end, char* ident, int index);
u32 ltq_count0_read(void);

#else		/* CONFIG_CPU_FREQ */
static inline ssize_t ltq_lxfreq_state_change(IFX_PMCU_STATE_t powerState, 
											   IFX_PMCU_MODULE_t module)
{
	return 0;
}
static inline IFX_PMCU_STATE_t ltq_lxfreq_get_ps_from_khz(unsigned int freqkhz)
{
	return 0;
}

static inline unsigned int ltq_lxfreq_getfreq_khz(unsigned int cpu)
{
	return 0;
}

static inline int ltq_lxfreq_state_change_disable(void)
{
	return 0;
}

static inline int ltq_lxfreq_state_change_enable(void)
{
	return 0;
}
#endif		/* CONFIG_CPU_FREQ */


#endif		/* _LTQ_LXFREQ_H_ */
