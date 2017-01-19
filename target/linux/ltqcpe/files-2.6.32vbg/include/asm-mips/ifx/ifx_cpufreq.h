/******************************************************************************

							   Copyright (c) 2010
							Lantiq Deutschland GmbH
					 Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
#ifndef _IFX_CPUFREQ_H_
#define _IFX_CPUFREQ_H_

/* Lantiq Linux frequency driver version  */
#define LTQ_LXFREQ_DRV_VERSION  "LTQ_LXFREQ_DRV_VERSION 1.0.3.0"

/* Lantiq CPUFREQ driver version  */
#define IFX_CPUFREQ_DRV_VERSION  "IFX_CPUFREQ_DRV_VERSION 3.0.11.0"

/*===========================================================================*/
/* LTQDBG printk specific's; colored print_message support */
/*===========================================================================*/
#define BLINK	"\033[31;1m"
#define RED		"\033[31;1m"
#define YELLOW	"\033[33;1m"
#define GREEN	"\033[32;2m"
#define BLUE	"\033[34;1m"
#define CYAN	"\033[36;2m"
#define DIM 	"\033[37;1m"
#define NORMAL	"\033[0m"

#define LTQDBG_EMERG(module,fmt,arg...)\
		if(ltq_cpufreq_log_level_g >= 0){\
		printk(KERN_EMERG RED #module NORMAL ": " fmt,##arg);\
		}

#define LTQDBG_ERR(module,fmt,arg...)\
		if(ltq_cpufreq_log_level_g >= 0){\
		printk(KERN_ERR RED #module NORMAL ": " fmt,##arg);\
		}

#define LTQDBG_WARNING(module,fmt,arg...)\
		if(ltq_cpufreq_log_level_g >= 1){\
		printk(KERN_WARNING YELLOW #module NORMAL ": " fmt,##arg);\
		}

#define LTQDBG_INFO(module,fmt,arg...)\
		if(ltq_cpufreq_log_level_g >= 2){\
		printk(KERN_ERR GREEN #module NORMAL ": " fmt,##arg);\
		}

#define LTQDBG_REQ(module,fmt,arg...)\
		if(ltq_cpufreq_log_level_g >= 3){\
		printk(KERN_ERR GREEN #module NORMAL ": " fmt,##arg);\
		}

#define LTQDBG_FUNC(module,fmt,arg...)\
		if(ltq_cpufreq_log_level_g >= 4){\
		printk(KERN_INFO GREEN #module NORMAL ": " fmt,##arg);\
		}

#define LTQDBG_DEBUG(module,fmt,arg...)\
		if(ltq_cpufreq_log_level_g >= 5){\
		printk(KERN_DEBUG CYAN #module NORMAL ": " fmt,##arg);\
		}
/*===========================================================================*/


/*===========================================================================*/
/* IFX CPUFREQ structs 																											                               */
/*===========================================================================*/
typedef struct
{
	int cpu_freq;   	/*cpu freqency in MHz*/
	int ddr_freq;   	/*ddr freqency in MHz*/
	int core_voltage;   /*core voltage in mV*/
}IFX_CPUFREQ_STAT_t;


IFX_PMCU_RETURN_t  ifx_cpufreq_state_set(IFX_PMCU_STATE_t pmcuState);
IFX_PMCU_RETURN_t  ifx_cpufreq_freqtab_get(int* cpu_freq_p, 
										   IFX_CPUFREQ_STAT_t** freq_tab_p,
										   IFX_CPUFREQ_STAT_t** freq_tab_tmp_p);

#endif   /* _IFX_CPUFREQ_H_ */
