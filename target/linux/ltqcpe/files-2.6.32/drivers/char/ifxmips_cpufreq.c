/****************************************************************************

	Copyright (c) 2010
	Lantiq Deutschland GmbH
	Am Campeon 3; 85579 Neubiberg, Germany

	For licensing information, see the file 'LICENSE' in the root folder
	of this software module.

*****************************************************************************/

#include <linux/version.h> 
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)) 
	#include <linux/utsrelease.h> 
#else 
	#include <generated/utsrelease.h> 
#endif
#include <linux/module.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/init.h>
#include <linux/timex.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/interrupt.h> /* We want an interrupt */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
	#include <ltq_clk.h>
	#include <ltq_regs.h>
#else
	#include <ifx_clk.h>
	#include <ifx_regs.h>
#endif
#include <ifx_pmcu.h>
#include <ifx_cpufreq.h>
#include <ltq_lxfreq.h>
#include <common_routines.h>

/**
	\defgroup LQ_CPUFREQ CPU Frequency Driver
	\ingroup LQ_COC
	Ifx cpu frequency driver module
*/
/* @{ */


/** \defgroup LQ_CPUFREQ_PRG Program Guide
	\ingroup LQ_CPUFREQ
	<center>
	<B>"CPU Frequency Driver ( CPUFREQ )" </B>.
	</center>    
	\section Purpose Purpose
	The CPU Frequency (CPUFREQ) driver was introduced to map the hardware
	independent power states (D0, D1, D2, D3) to the hardware dependent
	clock frequencies. Since UGW 5.1 the driver additionally supports the
	voltage scaling feature for XWAY™ xRX200 only. Voltage scaling allows
	to reduce the CPU core voltage in a small defined range thus resulting
	in power saving. The value by which the 1V core voltage can be reduced
	depends on the clock frequency of the CPU and the DDR. Because of
	several critical frequency transitions, the CPUFREQ driver takes also
	care about these and provide a workaround if possible.\n
	During the development of UGW 5.2 the requirement to support
	CPU = 600 MHz / DDR = 300 MHz instead of CPU = 500 MHz / DDR = 250 MHz
	was required and approved with a change request to the system. The
	consequence for the frequency driver was that a new frequency table
	was introduced to support the new D0-State frequency.\n
	In general the system frequency for CPU and DDR are always defined
	inside uboot because DRAM interface and CPU must be proper configured
	before the kernel can be loaded. Therefore the recognition if the
	system runs with 500/250MHz or 600/300MHz will be done from the
	frequency driver during it's init phase by reading the related cgu_sys
	HW register. If this register is set to 600/300MHz the new freuency
	table is used, otherwise the table for 500/250MHz.
	\n
	\section freqseq Setting system frequency sequence
	\image html systemfreq.jpg
	\n
	\n\n\n Supported features in PROC FS:
		- read version code of cpufreq driver
		- read chip_id from HW register
		- enable/disable voltage scaling support
		- set log level of the cpufreq driver
	\n
	\n
*/ 

/*======================================================================*/
/* CPUFREQ DEBUG switch 						*/
/*======================================================================*/
#undef CPUFREQ_DEBUG_MODE

/*======================================================================*/
/* Register definition for the CHIPID_Register  			*/
/*======================================================================*/
#define IFX_MPS			(KSEG1 | 0x1F107000)
#define IFX_MPS_CHIPID		((volatile u32*)(IFX_MPS + 0x0344))
#define ARX188			0x016C
#define ARX168			0x016D
#define ARX182			0x016E /* before November 2009 */
#define ARX182_2		0x016F /* after November 2009 */
#define GRX188			0x0170
#define GRX168			0x0171
#define VRX288_A1x		0x01C0
#define VRX282_A1x		0x01C1
#define VRX268_A1x		0x01C2
#define GRX268_A1x		0x01C8
#define GRX288_A1x		0x01C9
#define VRX288_A2x		0x000B
#define VRX282_A2x		0x000C
#define VRX268_A2x		0x000E
#define GRX288_A2x		0x000D
#define ARX362			0x0004 /* 2x2 Router */
#define ARX382			0x0007 /* 2x2 Gateway */
#define GRX388			0x0009 /* 3x3 Ethernet Router/Gateway */
#define ARX368			0x0005 /* 3x3 HE Router */
#define ARX388			0x0008 /* 3x3 HE Gateway */
#define GRX383			0x0010 /* 2x2 Router */
#define GRX369			0x0011 /* 2x2 HE Router */
#define GRX387			0x000F /* 2x2 HE Eth Router/Gateway 3 GPHY */
#define GRX389			0x0012 /* 2x2 HE Eth Router/Gateway 4 GPHY */


extern u32 cgu_set_clock(u32 cpu_clk, u32 ddr_clk, u32 fpi_clk);
extern unsigned int ifx_get_cpu_hz(void);

typedef struct
{
	IFX_PMCU_STATE_t	oldState; /* oldState */
	IFX_PMCU_STATE_t	newState; /* newState */
	IFX_int32_t 		permit; /* permission for this transition */
	IFX_PMCU_STATE_t	intState1; /* intermediate State1 */
	IFX_PMCU_STATE_t	intState2; /* intermediate State2 */
	IFX_int32_t 		stateDir; /* state change direction up,down */
}IFX_FREQ_TRANSITION_t;


/*======================================================================*/
/* LOCAL MACROS & DEFINES  						*/
/*======================================================================*/
#ifndef _MKSTR_1
#define _MKSTR_1(x)	#x
#define _MKSTR(x)	_MKSTR_1(x)
#endif

/** driver version as string */
#define IFX_CPUFREQ_DRV_VERSION  _MKSTR(DRV_CPUFREQ_VER_MAJOR) "." \
                                 _MKSTR(DRV_CPUFREQ_VER_MINOR) "." \
                                 _MKSTR(DRV_CPUFREQ_VER_STEP)  "." \
                                 _MKSTR(DRV_CPUFREQ_VER_TYPE)

/** driver version, what string */
#define DRV_CPUFREQ_WHAT_STR "@(#)CPUFREQ, Version " IFX_CPUFREQ_DRV_VERSION

/*======================================================================*/
/* MODULE DEFINES   							*/
/*======================================================================*/
#define IFX_NO_TRANS		0x000 /* frequency transition prohibited */
#define IFX_TRANS_PERMIT	0x001 /* frequency transition permitted  */
#define IFX_TRANS_CRITICAL	0x002 /* frequency transition critical, need 
					intermediate step */
#define IFX_STATE_NC		0x100 /* no state change */
#define IFX_STATE_UP		0x200 /* state change up, means step from
					lower to higher frequency */
#define IFX_STATE_DOWN  	0x300 /* state change down, means step from
					higher to lower frequency */

#ifdef CPUFREQ_DEBUG_MODE
#define IFX_TRACE_COUNT 	50 /* test purpose only */
#endif


/*======================================================================*/
/* EXTERNAL VARIABLES  							*/
/*======================================================================*/
#ifdef CONFIG_CPU_FREQ
extern struct cpufreq_driver ltq_lxfreq_driver;
extern struct cpufreq_frequency_table ltq_freq_tab[];
extern struct list_head ltq_lxfreq_head_mod_list_g;
extern char* ltq_pmcu_ps[];
extern char* ltq_lxfreq_mod[];
extern struct workqueue_struct *ltq_lxfreq_wq;
#endif

/*======================================================================*/
/* LOCAL FUNCTIONS PROTOTYPES part 1					*/
/*======================================================================*/
static IFX_PMCU_RETURN_t 
ifx_cpufreq_state_get(IFX_PMCU_STATE_t *pmcuState);
static IFX_PMCU_RETURN_t 
ifx_cpucg_state_get(IFX_PMCU_STATE_t *pmcuState);
static IFX_PMCU_RETURN_t 
ifx_cpufreq_pwrFeatureSwitch(IFX_PMCU_PWR_STATE_ENA_t pwrStateEna);
static IFX_PMCU_RETURN_t 
ifx_cpucg_pwrFeatureSwitch(IFX_PMCU_PWR_STATE_ENA_t pwrStateEna);

/*======================================================================*/
/* MODULE GLOBAL VARIABLES						*/
/*======================================================================*/
#ifdef CONFIG_CPU_FREQ
struct LTQ_LXFREQ_MODSTRUCT cpuddr_lxfreq_mod_g = {
	.name				= "CPU/DDR FreqChange",
	.pmcuModule			= IFX_PMCU_MODULE_CPU,
	.pmcuModuleNr			= 0,
	.powerFeatureStat		= IFX_PMCU_PWR_STATE_ON,
	.ltq_lxfreq_state_get		= ifx_cpufreq_state_get,
	.ltq_lxfreq_pwr_feature_switch	= ifx_cpufreq_pwrFeatureSwitch,
};

struct LTQ_LXFREQ_MODSTRUCT cpuwait_lxfreq_mod_g = {
	.name				= "CPU sleep in idle loop",
	.pmcuModule			= IFX_PMCU_MODULE_CPU,
	.pmcuModuleNr			= 1,
	.powerFeatureStat		= IFX_PMCU_PWR_STATE_ON,
	.ltq_lxfreq_state_get		= ifx_cpucg_state_get,
	.ltq_lxfreq_pwr_feature_switch	= ifx_cpucg_pwrFeatureSwitch,
};
#endif

/** \ingroup LQ_CPUFREQ
	\brief  Container to save cpu_wait pointer 
		Used to enable/disable the mips_core clock gating feature
*/
static void (*cpu_wait_sav)(void ); /* container to save mips_wait */

#ifdef CONFIG_IFX_DCDC
/** \ingroup LQ_CPUFREQ
	\brief  switching On/Off the voltage scaling support 
			-  0  = disable voltage scaling
			-  1  = enable voltage scaling (default)
*/
static int dcdc_ctrl_g = 1;
#endif

/** \ingroup LQ_CPUFREQ
	\brief  hold the clock change latency in [us]
		pre, state and post-processing
		- [x1][0]	= minimum latency
    		- [x2][1]	= current latency
    		- [x3][2]	= maximum latency
    		- x1		= pre-processing
    		- x2		= state-change-processing
    		- x3		= post-processing
*/
int ltq_cpufreq_latency_g[3][3] = 
{
	{0xFFFFFFFF,0,0},
	{0xFFFFFFFF,0,0},
	{0xFFFFFFFF,0,0},
};

/** \ingroup LQ_CPUFREQ
	\brief  Control the  garrulity of the LTQ_LXFREQ/IFX_CPUFREQ module 
		- <0 = quiet
		-  0 = ERRORS
		-  1 = WARNINGS
		-  2 = + INFO (default)
		-  3 = + PS REQUEST
		-  4 = + FUNC CALL TRACE
		-  5 = + DEBUG
*/
int ltq_cpufreq_log_level_g = 2;

/** \ingroup LQ_CPUFREQ
	\brief  cpu frequency in MHz 
*/
static int cpu_freq_g = 0;

/** \ingroup LQ_CPUFREQ
	\brief  ddr frequency in MHz 
*/
static int ddr_freq_g = 0;

/** \ingroup LQ_CPUFREQ
	\brief  current chip_id
	- ARX188	0x016C
	- ARX168	0x016D
	- ARX182	0x016E  before November 2009
	- ARX182_2	0x016F  after  November 2009
	- GRX188	0x0170
	- GRX168	0x0171
	- VRX288_A1x	0x01C0
	- VRX282_A1x	0x01C1
	- VRX268_A1x	0x01C2
	- GRX268_A1x	0x01C8
	- GRX288_A1x	0x01C9
	- VRX288_A2x	0x000B
	- VRX282_A2x	0x000C
	- VRX268_A2x	0x000E
	- GRX288_A2x	0x000D
*/
static int chip_id_g;

/** \ingroup LQ_CPUFREQ
	\brief   hold always the last power state of cpu driver
*/
static IFX_PMCU_STATE_t lastPmcuState_g = IFX_PMCU_STATE_D0;

/** \ingroup LQ_CPUFREQ
    \brief  freqtab_p points to the right frequency table depending on the
    underlying hardware. The pointer is initialized during driver init.
*/
static IFX_CPUFREQ_STAT_t* freqtab_p = NULL;

/** \ingroup LQ_CPUFREQ
    \brief  freqTransp points to the right frequency transition table depending
    on the underlying hardware. The pointer is initialized during driver init.
*/
static IFX_FREQ_TRANSITION_t* freqTransp = NULL;

/** \ingroup LQ_CPUFREQ
    \brief  cpuFreqPwrFea_g indicates if the frequency change feature is
    enabled or disabled.
	-  0  = enabled (default)
	-  1  = disabled
*/
static int cpuFreqPwrFea_g = 0;

#ifdef CONFIG_PROC_FS
/* proc entry variables */
static struct proc_dir_entry* ifx_cpufreq_dir_cpufreq_g = NULL;
#endif


/** \ingroup LQ_CPUFREQ
	\brief  Frequency table for ARX168/ARX182 
	- 1st Col = CPU frequency in [MHz]
	- 2cd Col = DDR frequency in [MHz]
	- 3rd Col = core power domain voltage in [mV]
*/
IFX_CPUFREQ_STAT_t cpufreq_state_1[]={
	{333,166,0},/* D0 */
	{  0,  0,0},/* D1, not defined*/
	{166,166,0},/* D2 */
	{111,111,0} /* D3 */
};

/** \ingroup LQ_CPUFREQ
	\brief  Frequency table for ARX188/GRX188 
	- 1st Col = CPU frequency in [MHz]
	- 2cd Col = DDR frequency in [MHz]
	- 3rd Col = core power domain voltage in [mV]
*/
IFX_CPUFREQ_STAT_t cpufreq_state_2[]={
	{393,196,0},/* D0 */
	{  0,  0,0},/* D1, not defined*/
	{196,196,0},/* D2 */
	{131,131,0} /* D3 */
};

/** \ingroup LQ_CPUFREQ
	\brief  Frequency table for VRX268 
	- 1st Col = CPU frequency in [MHz]
	- 2cd Col = DDR frequency in [MHz]
	- 3rd Col = core power domain voltage in [mV]
*/
IFX_CPUFREQ_STAT_t cpufreq_state_3[]={
	{333,167,1000},/* D0 */
	{  0,  0,   0},/* D1, not defined*/
	{167,167,1000},/* D2 */
	{125,125, 930} /* D3 */
};

/** \ingroup LQ_CPUFREQ
	\brief  Frequency table for VRX288/GRX288_A1x 
	- 1st Col = CPU frequency in [MHz]
	- 2cd Col = DDR frequency in [MHz]
	- 3rd Col = core power domain voltage in [mV]
*/
IFX_CPUFREQ_STAT_t cpufreq_state_4[]={
#ifdef CONFIG_UBOOT_CONFIG_VR9_DDR1
	{500,200,1175},/* D0 */
#else
	{500,250,1175},/* D0 */
#endif
	{393,196,1050},/* D1,*/
	{333,167,1000},/* D2 */
	{125,125, 930} /* D3 */
};

/** \ingroup LQ_CPUFREQ
	\brief  Frequency table for GRX288_A2x 
	- 1st Col = CPU frequency in [MHz]
	- 2cd Col = DDR frequency in [MHz]
	- 3rd Col = core power domain voltage in [mV]
*/
IFX_CPUFREQ_STAT_t cpufreq_state_5[]={
	{600,300,1175},/* D0 */
	{393,196,1050},/* D1,*/
	{333,167,1000},/* D2 */
	{125,125, 930} /* D3 */
};

/** \ingroup LQ_CPUFREQ
	\brief  Frequency table for xRX300_1 
	- 1st Col = CPU frequency in [MHz]
	- 2cd Col = DDR frequency in [MHz]
	- 3rd Col = core power domain voltage in [mV]
*/
IFX_CPUFREQ_STAT_t cpufreq_state_6[]={
	{500,250,0},/* D0 */
	{250,250,0},/* D1,*/
	{250,125,0},/* D2 */
	{125,125,0} /* D3 */
};

/** \ingroup LQ_CPUFREQ
	\brief  Frequency table for xRX300_2 
	- 1st Col = CPU frequency in [MHz]
	- 2cd Col = DDR frequency in [MHz]
	- 3rd Col = core power domain voltage in [mV]
*/
IFX_CPUFREQ_STAT_t cpufreq_state_7[]={
	{600,300,0},/* D0 */
	{300,300,0},/* D1,*/
	{300,150,0},/* D2 */
	{150,150,0} /* D3 */
};

/** \ingroup LQ_CPUFREQ
	\brief  Frequency table for xRX300_3 
	- 1st Col = CPU frequency in [MHz]
	- 2cd Col = DDR frequency in [MHz]
	- 3rd Col = core power domain voltage in [mV]
*/
IFX_CPUFREQ_STAT_t cpufreq_state_8[]={
	{720,360,0},/* D0 */
	{360,360,0},/* D1,*/
	{360,180,0},/* D2 */
	{180,180,0} /* D3 */
};


/** \ingroup LQ_CPUFREQ
	\brief  Frequency table used as backup 
*/
IFX_CPUFREQ_STAT_t cpufreq_state_tmp[]={
	{0,0,0},/* D0 */
	{0,0,0},/* D1,*/
	{0,0,0},/* D2 */
	{0,0,0} /* D3 */
};

/** \ingroup LQ_CPUFREQ
	\brief  Dependency list of the cpu frequency driver
	State D0D3 means don't care.
	Static declaration is necessary to let gcc accept this static
	initialisation.
*/
/* define dependency list;  state D0D3 means don't care */
/* static declaration is necessary to let gcc accept this static 
initialisation. */
static IFX_PMCU_MODULE_DEP_t depList=
{
	11,
	{
	{IFX_PMCU_MODULE_VE,   1, IFX_PMCU_STATE_D0D3, IFX_PMCU_STATE_D2,   
		IFX_PMCU_STATE_D2,   IFX_PMCU_STATE_D2},
	{IFX_PMCU_MODULE_PPE,  0, IFX_PMCU_STATE_D0D3, IFX_PMCU_STATE_D3,   
		IFX_PMCU_STATE_D3,   IFX_PMCU_STATE_D3},
	{IFX_PMCU_MODULE_USB,  0, IFX_PMCU_STATE_D0D3, IFX_PMCU_STATE_D0D3, 
		IFX_PMCU_STATE_D0D3, IFX_PMCU_STATE_D0D3},
	{IFX_PMCU_MODULE_USB,  1, IFX_PMCU_STATE_D0D3, IFX_PMCU_STATE_D0D3, 
		IFX_PMCU_STATE_D0D3, IFX_PMCU_STATE_D0D3},
	{IFX_PMCU_MODULE_USB,  2, IFX_PMCU_STATE_D0D3, IFX_PMCU_STATE_D0D3, 
		IFX_PMCU_STATE_D0D3, IFX_PMCU_STATE_D0D3},
	{IFX_PMCU_MODULE_DSL,  0, IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D1,   
		IFX_PMCU_STATE_D2,   IFX_PMCU_STATE_D3},
	{IFX_PMCU_MODULE_WLAN, 0, IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D1,   
		IFX_PMCU_STATE_D2,   IFX_PMCU_STATE_D3},
	{IFX_PMCU_MODULE_DECT, 0, IFX_PMCU_STATE_D0D3, IFX_PMCU_STATE_D3,   
		IFX_PMCU_STATE_D3,   IFX_PMCU_STATE_D3},
	{IFX_PMCU_MODULE_GPTC, 0, IFX_PMCU_STATE_D0,   IFX_PMCU_STATE_D1,   
		IFX_PMCU_STATE_D2,   IFX_PMCU_STATE_D3},
	{IFX_PMCU_MODULE_SPI,  0, IFX_PMCU_STATE_D0,   IFX_PMCU_STATE_D1,   
		IFX_PMCU_STATE_D2,   IFX_PMCU_STATE_D3},
	{IFX_PMCU_MODULE_UART, 0, IFX_PMCU_STATE_D0,   IFX_PMCU_STATE_D1,   
		IFX_PMCU_STATE_D2,   IFX_PMCU_STATE_D3}
	}
};


#ifdef CONFIG_VR9
/** \ingroup LQ_CPUFREQ
	\brief	Frequency transition table for VRX268 Intermediate states are
	only relevant if permit = IFX_TRANS_CRITICAL. 
	Static declaration is necessary to let gcc accept this static 
	initialisation.
	- 1st Col = old power state
	- 2cd Col = new power state
	- 3rd Col = permission
	- 4th Col = intermediate state 1
	- 5th Col = intermediate state 2
	- 6th Col = up or down step
  */
static IFX_FREQ_TRANSITION_t freq_transition_3[]={
/* {	  oldState     ,	 newState     , 	permit  	 , intermediate state1 , intermediate state2 }  */
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D0, IFX_NO_TRANS	 , IFX_PMCU_STATE_D0	, IFX_PMCU_STATE_D0 , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D1, IFX_NO_TRANS	 , IFX_PMCU_STATE_D1	, IFX_PMCU_STATE_D1 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D2, IFX_TRANS_PERMIT , IFX_PMCU_STATE_D2	, IFX_PMCU_STATE_D2 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D3, IFX_TRANS_PERMIT , IFX_PMCU_STATE_D1	, IFX_PMCU_STATE_D1 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D0, IFX_NO_TRANS	 , IFX_PMCU_STATE_D0	, IFX_PMCU_STATE_D0 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D1, IFX_NO_TRANS	 , IFX_PMCU_STATE_D1	, IFX_PMCU_STATE_D1 , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D2, IFX_NO_TRANS	 , IFX_PMCU_STATE_D2	, IFX_PMCU_STATE_D2 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D3, IFX_NO_TRANS	 , IFX_PMCU_STATE_D3	, IFX_PMCU_STATE_D3 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D0, IFX_TRANS_PERMIT , IFX_PMCU_STATE_D1	, IFX_PMCU_STATE_D1 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D1, IFX_NO_TRANS	 , IFX_PMCU_STATE_D1	, IFX_PMCU_STATE_D1 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D2, IFX_NO_TRANS	 , IFX_PMCU_STATE_D2	, IFX_PMCU_STATE_D2 , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D3, IFX_TRANS_PERMIT , IFX_PMCU_STATE_D3	, IFX_PMCU_STATE_D3 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D0, IFX_TRANS_PERMIT , IFX_PMCU_STATE_D2	, IFX_PMCU_STATE_D1 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D1, IFX_NO_TRANS	 , IFX_PMCU_STATE_D2	, IFX_PMCU_STATE_D2 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D2, IFX_TRANS_PERMIT , IFX_PMCU_STATE_D2	, IFX_PMCU_STATE_D2 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D3, IFX_NO_TRANS	 , IFX_PMCU_STATE_D3	, IFX_PMCU_STATE_D3 , IFX_STATE_NC  } 
};

/** \ingroup LQ_CPUFREQ
	\brief  Frequency transition table for XRX288
	Intermediate states are only relevant if permit = IFX_TRANS_CRITICAL
	Static declaration is necessary to let gcc accept this static
	initialisation.
	- 1st Col = old power state
	- 2cd Col = new power state
	- 3rd Col = permission
	- 4th Col = intermediate state 1
	- 5th Col = intermediate state 2
	- 6th Col = up or down step
  */
static IFX_FREQ_TRANSITION_t freq_transition_4[]={
/* {	  oldState     ,	 newState     , 	  permit	  , intermediate state1, intermediate stat2 }    */
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D0, IFX_NO_TRANS	  , IFX_PMCU_STATE_D0  , IFX_PMCU_STATE_D0 , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D1, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D1  , IFX_PMCU_STATE_D1 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D2, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D2  , IFX_PMCU_STATE_D2 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D3, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D1  , IFX_PMCU_STATE_D1 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D0, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D0  , IFX_PMCU_STATE_D0 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D1, IFX_NO_TRANS	  , IFX_PMCU_STATE_D1  , IFX_PMCU_STATE_D1 , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D2, IFX_TRANS_CRITICAL, IFX_PMCU_STATE_D0  , IFX_PMCU_STATE_D0 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D3, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D3  , IFX_PMCU_STATE_D3 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D0, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D1  , IFX_PMCU_STATE_D1 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D1, IFX_TRANS_CRITICAL, IFX_PMCU_STATE_D0  , IFX_PMCU_STATE_D0 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D2, IFX_NO_TRANS	  , IFX_PMCU_STATE_D2  , IFX_PMCU_STATE_D2 , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D3, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D3  , IFX_PMCU_STATE_D3 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D0, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D2  , IFX_PMCU_STATE_D1 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D1, IFX_TRANS_CRITICAL, IFX_PMCU_STATE_D0  , IFX_PMCU_STATE_D0 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D2, IFX_TRANS_CRITICAL, IFX_PMCU_STATE_D0  , IFX_PMCU_STATE_D0 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D3, IFX_NO_TRANS	  , IFX_PMCU_STATE_D3  , IFX_PMCU_STATE_D3 , IFX_STATE_NC  } 
};
#endif

#ifdef CONFIG_AR9
/** \ingroup LQ_CPUFREQ
	\brief  Frequency transition table for XRX1XX
	Intermediate states are only relevant if permit = IFX_TRANS_CRITICAL
	Static declaration is necessary to let gcc accept this static
	initialisation.
	- 1st Col = old power state
	- 2cd Col = new power state
	- 3rd Col = permission
	- 4th Col = intermediate state 1
	- 5th Col = intermediate state 2
	- 6th Col = up or down step
  */
static IFX_FREQ_TRANSITION_t freq_transition_2[]={
/* {	  oldState     ,	 newState     , 	permit  	 , intermediate state1 , intermediate state2, DCDC control  }*/
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D0, IFX_NO_TRANS	 , IFX_PMCU_STATE_D0   , IFX_PMCU_STATE_D0  , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D1, IFX_NO_TRANS	 , IFX_PMCU_STATE_D1   , IFX_PMCU_STATE_D1  , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D2, IFX_TRANS_PERMIT , IFX_PMCU_STATE_D2   , IFX_PMCU_STATE_D2  , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D3, IFX_TRANS_PERMIT , IFX_PMCU_STATE_D3   , IFX_PMCU_STATE_D3  , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D0, IFX_NO_TRANS	 , IFX_PMCU_STATE_D0   , IFX_PMCU_STATE_D0  , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D1, IFX_NO_TRANS	 , IFX_PMCU_STATE_D1   , IFX_PMCU_STATE_D1  , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D2, IFX_NO_TRANS	 , IFX_PMCU_STATE_D2   , IFX_PMCU_STATE_D2  , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D3, IFX_NO_TRANS	 , IFX_PMCU_STATE_D3   , IFX_PMCU_STATE_D3  , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D0, IFX_TRANS_PERMIT , IFX_PMCU_STATE_D0   , IFX_PMCU_STATE_D0  , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D1, IFX_NO_TRANS	 , IFX_PMCU_STATE_D1   , IFX_PMCU_STATE_D1  , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D2, IFX_NO_TRANS	 , IFX_PMCU_STATE_D2   , IFX_PMCU_STATE_D2  , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D3, IFX_TRANS_PERMIT , IFX_PMCU_STATE_D3   , IFX_PMCU_STATE_D3  , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D0, IFX_TRANS_PERMIT , IFX_PMCU_STATE_D0   , IFX_PMCU_STATE_D0  , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D1, IFX_NO_TRANS	 , IFX_PMCU_STATE_D1   , IFX_PMCU_STATE_D1  , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D2, IFX_TRANS_PERMIT , IFX_PMCU_STATE_D2   , IFX_PMCU_STATE_D2  , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D3, IFX_NO_TRANS	 , IFX_PMCU_STATE_D3   , IFX_PMCU_STATE_D3  , IFX_STATE_NC  } 
};
#endif

#ifdef CONFIG_AR10
/** \ingroup LQ_CPUFREQ
	\brief  Frequency transition table for XRX300
	Intermediate states are only relevant if permit = IFX_TRANS_CRITICAL
	Static declaration is necessary to let gcc accept this static
	initialisation.
	- 1st Col = old power state
	- 2cd Col = new power state
	- 3rd Col = permission
	- 4th Col = intermediate state 1
	- 5th Col = intermediate state 2
	- 6th Col = up or down step
  */
static IFX_FREQ_TRANSITION_t freq_transition_67[]={
/* {	  oldState     ,	 newState     , 	  permit	  , intermediate state1, intermediate stat2 }    */
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D0, IFX_NO_TRANS	  , IFX_PMCU_STATE_D0  , IFX_PMCU_STATE_D0 , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D1, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D1  , IFX_PMCU_STATE_D1 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D2, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D2  , IFX_PMCU_STATE_D2 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D0, IFX_PMCU_STATE_D3, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D1  , IFX_PMCU_STATE_D1 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D0, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D0  , IFX_PMCU_STATE_D0 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D1, IFX_NO_TRANS	  , IFX_PMCU_STATE_D1  , IFX_PMCU_STATE_D1 , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D2, IFX_TRANS_CRITICAL, IFX_PMCU_STATE_D0  , IFX_PMCU_STATE_D0 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D1, IFX_PMCU_STATE_D3, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D3  , IFX_PMCU_STATE_D3 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D0, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D1  , IFX_PMCU_STATE_D1 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D1, IFX_TRANS_CRITICAL, IFX_PMCU_STATE_D0  , IFX_PMCU_STATE_D0 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D2, IFX_NO_TRANS	  , IFX_PMCU_STATE_D2  , IFX_PMCU_STATE_D2 , IFX_STATE_NC  },
	{ IFX_PMCU_STATE_D2, IFX_PMCU_STATE_D3, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D3  , IFX_PMCU_STATE_D3 , IFX_STATE_DOWN},
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D0, IFX_TRANS_PERMIT  , IFX_PMCU_STATE_D2  , IFX_PMCU_STATE_D1 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D1, IFX_TRANS_CRITICAL, IFX_PMCU_STATE_D0  , IFX_PMCU_STATE_D0 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D2, IFX_TRANS_CRITICAL, IFX_PMCU_STATE_D0  , IFX_PMCU_STATE_D0 , IFX_STATE_UP  },
	{ IFX_PMCU_STATE_D3, IFX_PMCU_STATE_D3, IFX_NO_TRANS	  , IFX_PMCU_STATE_D3  , IFX_PMCU_STATE_D3 , IFX_STATE_NC  } 
};
#endif



#ifdef CPUFREQ_DEBUG_MODE /* test purpose only */
	IFX_FREQ_TRANSITION_t state_trace[IFX_TRACE_COUNT];
	IFX_int32_t trace_count = 0;
#endif


/*======================================================================*/
/* EXTERNAL FUNCTIONS PROTOTYPES					*/
/*======================================================================*/
#ifdef CONFIG_IFX_DCDC
	extern int ifx_dcdc_power_voltage_set ( int voltage_value);
#endif

/*======================================================================*/
/* LOCAL FUNCTIONS PROTOTYPES part 2 					*/
/*======================================================================*/
static IFX_int32_t ifx_get_transition_permit(IFX_PMCU_STATE_t oldState, 
						IFX_PMCU_STATE_t newState, 
						IFX_PMCU_STATE_t* intState1, 
						IFX_PMCU_STATE_t* intState2);


/**
	Function return the supported cpu frequencies for clock scaling.

	\param   cpu_freq_p  pointer to cpu freq array
	\param   freq_tab_p  pointer to cpu/ddr freq array (current)
	\param   freq_tab_tmp_p  pointer to cpu/ddr freq array (org)

	\return Returns value as follows:
		- IFX_PMCU_RETURN_SUCCESS: if successful
		- IFX_PMCU_RETURN_ERROR: in case of an error
*/
IFX_PMCU_RETURN_t  ifx_cpufreq_freqtab_get(int* cpu_freq_p, 
					IFX_CPUFREQ_STAT_t** freq_tab_p,
					IFX_CPUFREQ_STAT_t** freq_tab_tmp_p)
{
	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);

	if(freqtab_p == NULL){
		return IFX_PMCU_RETURN_ERROR;
	}

	if(freq_tab_p != NULL){
		*freq_tab_p = freqtab_p;
	}
	if(freq_tab_tmp_p != NULL){
		*freq_tab_tmp_p = cpufreq_state_tmp;
	}

	if(cpu_freq_p != NULL){
		/*frequency table must have always 4 entries for cpu/ddr clock pairs*/
		if(((freqtab_p + 0)->cpu_freq) != 0){
			*(cpu_freq_p) = (freqtab_p + 0)->cpu_freq;
			cpu_freq_p++;
		}
		if(((freqtab_p + 1)->cpu_freq) != 0){
			*(cpu_freq_p) = (freqtab_p + 1)->cpu_freq;
			cpu_freq_p++;
		}
		if(((freqtab_p + 2)->cpu_freq) != 0){
			*(cpu_freq_p) = (freqtab_p + 2)->cpu_freq;
			cpu_freq_p++;
		}
		if(((freqtab_p + 3)->cpu_freq) != 0){
			*(cpu_freq_p) = (freqtab_p + 3)->cpu_freq;
		}
	}
	return IFX_PMCU_RETURN_SUCCESS;
}

/**
	Callback function registered at the PMCU.
    This function is called by the PMCU whenever a frequency change is
    requested, to inform all affected modules before the frequency change
    really happen.

	\param[in]   pmcuModule  module identifier
	\param[in]  newState new requested power state 
	\param[in]  oldState old power state 

	\return Returns value as follows:
		- IFX_PMCU_RETURN_SUCCESS: if successful
		- IFX_PMCU_RETURN_ERROR: in case of an error
*/
static IFX_PMCU_RETURN_t 
ifx_cpufreq_prechange(IFX_PMCU_MODULE_t pmcuModule, IFX_PMCU_STATE_t newState,
					  IFX_PMCU_STATE_t oldState)
{
	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);

	/* if newState is a vaild state and oldState is set to
	   IFX_PMCU_STATE_INVALID,*/
	/* this callback should just check if the requested new powerstate is
	   supported by HW. */
	if(oldState == IFX_PMCU_STATE_INVALID){
		if( (freqtab_p + (newState-1))->cpu_freq == 0 ){
			LTQDBG_WARNING(CPUFREQ, "Requested Powerstate is NOT supported\n");
			return IFX_PMCU_RETURN_ERROR;
		}else{
			LTQDBG_DEBUG(CPUFREQ, "Requested Powerstate is supported\n");
			return IFX_PMCU_RETURN_SUCCESS;
		}
	}

	/* check if the power saving feature is disabled */
	if (cpuFreqPwrFea_g > 0) {
		if(cpuFreqPwrFea_g == 2) {
			LTQDBG_DEBUG(CPUFREQ, "Frequency change is disabled. PowerState "
								  "change discarded.\n");
			return IFX_PMCU_RETURN_DENIED;
		}else{
			cpuFreqPwrFea_g = 2;
		}
	}

	return IFX_PMCU_RETURN_SUCCESS;
}

/**
    Callback function for cpu clock gating registered at the PMCU.
    Definition is mandatory but no real implementation is necessary.

	\param[in]   pmcuModule  module identifier
	\param[in]  newState new requested power state 
	\param[in]  oldState old power state 

	\return Returns value as follows:
		- IFX_PMCU_RETURN_NOACTIVITY
*/
static IFX_PMCU_RETURN_t 
ifx_cpucg_prechange(IFX_PMCU_MODULE_t pmcuModule, IFX_PMCU_STATE_t newState, 
					IFX_PMCU_STATE_t oldState)
{
	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);
	return IFX_PMCU_RETURN_NOACTIVITY;
}

/**
	Callback function registered at the PMCU.
    This function is called by the PMCU whenever a frequency change was
    requested, to inform all affected modules after a frequency change was
    really executed or in case the request was denied.

	\param[in]   pmcuModule  module identifier
	\param[in]  newState new requested power state 
	\param[in]  oldState old power state 

	\return Returns value as follows:
		- IFX_PMCU_RETURN_SUCCESS: if successful
		- IFX_PMCU_RETURN_ERROR: in case of an error
*/
static IFX_PMCU_RETURN_t 
ifx_cpufreq_postchange(IFX_PMCU_MODULE_t pmcuModule, IFX_PMCU_STATE_t newState,
					   IFX_PMCU_STATE_t oldState)
{
	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);
	#ifdef CONFIG_CPU_FREQ
	lastPmcuState_g = ltq_lxfreq_get_ps_from_khz(ltq_lxfreq_getfreq_khz(0));
	#endif
	return IFX_PMCU_RETURN_SUCCESS;
}

/**
    Callback function for cpu clock gating registered at the PMCU.
    Definition is mandatory but no real implementation is necessary.

	\param[in]   pmcuModule  module identifier
	\param[in]  newState new requested power state 
	\param[in]  oldState old power state 

	\return Returns value as follows:
		- IFX_PMCU_RETURN_NOACTIVITY
*/
static IFX_PMCU_RETURN_t 
ifx_cpucg_postchange(IFX_PMCU_MODULE_t pmcuModule, IFX_PMCU_STATE_t newState,
					 IFX_PMCU_STATE_t oldState)
{
	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);
	return IFX_PMCU_RETURN_NOACTIVITY;
}

/**
    Callback function registered at the PMCU.
    Enable/Disable of the mips frequency scaling.

    \param[in]  pwrStateEna
    			- IFX_PMCU_PWR_STATE_ON
    			- IFX_PMCU_PWR_STATE_OFF

	\return Returns value as follows:
		- IFX_PMCU_RETURN_SUCCESS: if successful
		- IFX_PMCU_RETURN_ERROR: in case of an error
*/
static IFX_PMCU_RETURN_t 
ifx_cpufreq_pwrFeatureSwitch(IFX_PMCU_PWR_STATE_ENA_t pwrStateEna)
{
	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);
	if (pwrStateEna == IFX_PMCU_PWR_STATE_ON) {
		cpuFreqPwrFea_g = 0;
		/* entering D3 state happen in the usual way -> PMD*/
		LTQDBG_DEBUG(CPUFREQ, "CPU/DDR frequency change feature is enabled.\n");
		#ifdef CONFIG_CPU_FREQ
		cpuddr_lxfreq_mod_g.powerFeatureStat = IFX_PMCU_PWR_STATE_ON;
		ltq_lxfreq_state_change_enable();
		#endif
		return IFX_PMCU_RETURN_SUCCESS;
	}
	if (pwrStateEna == IFX_PMCU_PWR_STATE_OFF) {
		cpuFreqPwrFea_g = 1;
		#ifdef CONFIG_IFX_PMCU
		if(IFX_PMCU_RETURN_SUCCESS == ifx_pmcu_state_req(IFX_PMCU_MODULE_CPU,0,
								 IFX_PMCU_STATE_D0)){
			LTQDBG_DEBUG(CPUFREQ, "CPU/DDR frequency will be set to normal "
						"operation value (D0-State)\n");
		}
		#endif
		#ifdef CONFIG_CPU_FREQ
		cpuddr_lxfreq_mod_g.powerFeatureStat = IFX_PMCU_PWR_STATE_OFF;
		ltq_lxfreq_state_change_disable();
		#endif
		return IFX_PMCU_RETURN_SUCCESS;
	}
	return IFX_PMCU_RETURN_ERROR;
}

/**
    Callback function registered at the PMCU.
    Enable/Disable of the mips_core clock gating (mips wait instruction).

    \param[in]  pwrStateEna
    			- IFX_PMCU_PWR_STATE_ON
    			- IFX_PMCU_PWR_STATE_OFF

	\return Returns value as follows:
		- IFX_PMCU_RETURN_SUCCESS: if successful
		- IFX_PMCU_RETURN_ERROR: in case of an error
*/
static IFX_PMCU_RETURN_t 
ifx_cpucg_pwrFeatureSwitch(IFX_PMCU_PWR_STATE_ENA_t pwrStateEna)
{
	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);

	if (pwrStateEna == IFX_PMCU_PWR_STATE_ON) {
		cpu_wait = cpu_wait_sav;
		LTQDBG_DEBUG(CPUFREQ, "Mips_core clock gating (mips wait instruction) "
							  "is enabled.\n");
		#ifdef CONFIG_CPU_FREQ
		cpuwait_lxfreq_mod_g.powerFeatureStat = IFX_PMCU_PWR_STATE_ON;
		#endif
		return IFX_PMCU_RETURN_SUCCESS;
	}
	if (pwrStateEna == IFX_PMCU_PWR_STATE_OFF) {
		cpu_wait = 0;
		LTQDBG_DEBUG(CPUFREQ, "Mips_core clock gating (mips wait instruction) "
							  "is disabled.\n");
		#ifdef CONFIG_CPU_FREQ
		cpuwait_lxfreq_mod_g.powerFeatureStat = IFX_PMCU_PWR_STATE_OFF;
		#endif
		return IFX_PMCU_RETURN_SUCCESS;
	}
	return IFX_PMCU_RETURN_ERROR;
}


/**
	Callback function registered at the PMCU.
    This function is called by the PMCU to get the current power state status
    of the cpu frequency driver.

	\param[out]   pmcuState  current power state

	\return Returns value as follows:
		- IFX_PMCU_RETURN_SUCCESS: if successful
		- IFX_PMCU_RETURN_ERROR: in case of an error
*/
static IFX_PMCU_RETURN_t 
ifx_cpufreq_state_get(IFX_PMCU_STATE_t *pmcuState)
{
	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);
	cpu_freq_g=ifx_get_cpu_hz()/1000000;
#ifdef CONFIG_AR9
//	cpu_freq_g=(int)cgu_get_cpu_clock()/1000000;
	ddr_freq_g=(int)cgu_get_io_region_clock()/1000000;
#endif
#if defined(CONFIG_VR9) || defined(CONFIG_AR10)
//	cpu_freq_g=ifx_get_cpu_hz()/1000000;
	ddr_freq_g=(int)ifx_get_ddr_hz()/1000000;
#endif

	*pmcuState = lastPmcuState_g;
	return IFX_PMCU_RETURN_SUCCESS;
}


/**
	Callback function registered at the PMCU.
    No other power state than D0 can be returned. If the mips_core is clock
    gated cpu is dead and can't return something.

	\param[out]   pmcuState  current power state

	\return Returns value as follows:
		- IFX_PMCU_RETURN_SUCCESS: if successful
		- IFX_PMCU_RETURN_ERROR: in case of an error
*/
static IFX_PMCU_RETURN_t 
ifx_cpucg_state_get(IFX_PMCU_STATE_t *pmcuState)
{
	*pmcuState = IFX_PMCU_STATE_D0;
	return IFX_PMCU_RETURN_SUCCESS;
}


/**
	Callback function registered at the PMCU.
    This function is called by the PMCU to set a new power state for the cpu
    frequency driver.

	\param[in]   pmcuState  new power state

	\return Returns value as follows:
		- IFX_PMCU_RETURN_SUCCESS: if successful
		- IFX_PMCU_RETURN_ERROR: in case of an error
*/
IFX_PMCU_RETURN_t  ifx_cpufreq_state_set(IFX_PMCU_STATE_t pmcuState)
{
	IFX_PMCU_STATE_t intState1 = 0;
	IFX_PMCU_STATE_t intState2 = 0;
	IFX_int32_t retVal;

	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);
#ifdef CPUFREQ_DEBUG_MODE
	if(trace_count >= IFX_TRACE_COUNT) {
		trace_count = 0;
	}
#endif
	/* if frequency table pointer is not already initialized by init function
	   return with error status */
	if (freqtab_p == NULL) {
		return IFX_ERROR;
	}

	/* HINT: IFX_PMCU_STATE_D0 = 1 not 0 !!! */
	if( (pmcuState < IFX_PMCU_STATE_D0) || (pmcuState > IFX_PMCU_STATE_D3) ){
		return IFX_ERROR; /* requested powerState out of range */
	}

	/* check if state refers to a valid frequency */
	if( (freqtab_p + (pmcuState-1))->cpu_freq == 0 ){
		return IFX_ERROR;
	}

	LTQDBG_DEBUG(CPUFREQ, "setting cpu_freq_g=%d,ddr_freq_g=%d,"
						 "core_voltage=%d\n",\
			(freqtab_p + (pmcuState-1))->cpu_freq,\
			(freqtab_p + (pmcuState-1))->ddr_freq,\
			(freqtab_p + (pmcuState-1))->core_voltage);

	/* Check if we have a critical frequency transition */
	retVal = ifx_get_transition_permit(lastPmcuState_g, pmcuState,
						&intState1, &intState2);
	#ifdef CPUFREQ_DEBUG_MODE
		state_trace[trace_count].newState = pmcuState;
		state_trace[trace_count].oldState = lastPmcuState_g;
		state_trace[trace_count].intState1 = intState1;
		state_trace[trace_count].intState2 = intState2;
		state_trace[trace_count].permit = retVal;
		trace_count++;
	#endif
	if((retVal & 0xFF) == IFX_NO_TRANS) {
		return IFX_PMCU_RETURN_SUCCESS; /* nothing to do */
	}


	if((retVal & 0xFF) == IFX_TRANS_CRITICAL) {
		#ifdef CONFIG_IFX_DCDC
			if(dcdc_ctrl_g == 1) {
				LTQDBG_DEBUG(CPUFREQ, "Critical transition always via D0 "
									  "state\n");
				ifx_dcdc_power_voltage_set((freqtab_p + 
					(IFX_PMCU_STATE_D0-1))->core_voltage);
				retVal = retVal & 0xFF; /*clear up/down direction*/
				retVal = retVal | IFX_STATE_DOWN;/*from D0 we go always down.*/
			}
		#endif
		cgu_set_clock((freqtab_p + (intState1-1))->cpu_freq,
					(freqtab_p + (intState1-1))->ddr_freq,
					(freqtab_p + (intState1-1))->ddr_freq);
		/*Currently not in use. Only one intermediate step necessary. 
		 If necessary take care on DCDC voltage scaling.*/
		if(intState1 != intState2) {
			cgu_set_clock((freqtab_p + (intState2-1))->cpu_freq,
						(freqtab_p + (intState2-1))->ddr_freq,
						(freqtab_p + (intState2-1))->ddr_freq);
		}
	} else {
		#ifdef CONFIG_IFX_DCDC
			if(((retVal & 0xF00) == IFX_STATE_UP) && (dcdc_ctrl_g == 1)) {
				LTQDBG_DEBUG(CPUFREQ, "UP step, dcdc voltage scaling is "
									  "called\n");
				ifx_dcdc_power_voltage_set((freqtab_p + 
						(pmcuState-1))->core_voltage);
			}
		#endif
	}

	cgu_set_clock((freqtab_p + (pmcuState-1))->cpu_freq,
				(freqtab_p + (pmcuState-1))->ddr_freq,
				(freqtab_p + (pmcuState-1))->ddr_freq);

#ifdef CONFIG_IFX_DCDC
	if(((retVal & 0xF00) == IFX_STATE_DOWN) && (dcdc_ctrl_g == 1)) {
		LTQDBG_DEBUG(CPUFREQ, "DOWN step, dcdc voltage scaling is called\n");
		ifx_dcdc_power_voltage_set((freqtab_p + (pmcuState-1))->core_voltage);
	}
#endif

	/* remember the last state */
	lastPmcuState_g = pmcuState;
	return IFX_PMCU_RETURN_SUCCESS;
}
EXPORT_SYMBOL(ifx_cpufreq_state_set);


/**
    Internal function of the CPUFREQ driver to reference, depending on the
    underlying hardware, the correct frequency table. The function initialize
    the two pointer freqtab_p and freqTransp, which are pointing to the right
    frequency table and frequency transition table.
 */
static void ifx_cpufreq_set_freqtab(void){
	/* read the current chipid from the HW register. */
	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);
	chip_id_g = (IFX_REG_R32(IFX_MPS_CHIPID) & 0x0FFFF000) >> 12;
	switch (chip_id_g) {
		case GRX168:
		case ARX168:
		case ARX182:
		case ARX182_2:
			freqtab_p = cpufreq_state_1;
			#ifdef CONFIG_AR9
			freqTransp = freq_transition_2;
			#endif
			break;
		case ARX188:
		case GRX188:
			freqtab_p = cpufreq_state_2;
			#ifdef CONFIG_AR9
			freqTransp = freq_transition_2;
			#endif
			break;
		case VRX268_A1x:
		case VRX268_A2x:
			freqtab_p = cpufreq_state_3;
			#ifdef CONFIG_VR9
			freqTransp = freq_transition_3;
			#endif
			break;
		case VRX288_A1x:
		case VRX288_A2x:
		case GRX288_A1x:
		case GRX288_A2x:
			#ifdef CONFIG_VR9
			freqTransp = freq_transition_4;
			/* check the cpu frequency in the cgu_sys register if
			we run on 500 or 600MHz */
			if (CLOCK_600M == ifx_get_cpu_hz()) {
				freqtab_p = cpufreq_state_5;
				break;
			}
			#endif
			freqtab_p = cpufreq_state_4;
			break;
		case ARX362:
		case ARX382:
		case GRX388:
		case ARX368:
		case ARX388:
			#ifdef CONFIG_AR10
			freqTransp = freq_transition_67;
			/* check the cpu frequency in the cgu_sys register
			if we run on 500 or 600MHz */
			if (CLOCK_600M == ifx_get_cpu_hz()) {
				freqtab_p = cpufreq_state_7;
				break;
			}
			#endif
			freqtab_p = cpufreq_state_6;
			break;
		case GRX383:
		case GRX369:
		case GRX387:
		case GRX389:
			#ifdef CONFIG_AR10
			freqTransp = freq_transition_67;
			/* check the cpu frequency in the cgu_sys register
			if we run on 600 or 720MHz */
			if (CLOCK_720M == ifx_get_cpu_hz()) {
				freqtab_p = cpufreq_state_8;
				break;
			}
			#endif
			freqtab_p = cpufreq_state_7;
			break;
		default:
			freqtab_p = cpufreq_state_1;
			#ifdef CONFIG_VR9
			freqTransp = freq_transition_4;
			#endif
			#ifdef CONFIG_AR9
			freqTransp = freq_transition_2;
			#endif
			#ifdef CONFIG_AR10
			freqTransp = freq_transition_67;
			freqtab_p = cpufreq_state_6;
			#endif
			break;
	}
	 memcpy(&cpufreq_state_tmp, freqtab_p, sizeof(cpufreq_state_tmp));
}

/**
    Internal function of the CPUFREQ driver to get the permission of the
    currently requested power state change.
	Check if it is a CRITICAL transition or not.
	
	\param[in]   oldState  old power state
	\param[in]   newState  new power state
	\param[out]   intState1  intermediate_1 power state
	\param[out]   intState2  intermediate_2 power state

*/
static IFX_int_t ifx_get_transition_permit(IFX_PMCU_STATE_t oldState,
					IFX_PMCU_STATE_t newState,
					IFX_PMCU_STATE_t* intState1,
					IFX_PMCU_STATE_t* intState2)
{
	IFX_int_t i;
	IFX_FREQ_TRANSITION_t freqTrans;

	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);
	for(i=0;i<16;i++) {
		freqTrans = *(freqTransp + i);
		if(freqTrans.oldState == oldState) {
			if(freqTrans.newState == newState) {
				*intState1 = freqTrans.intState1;
				*intState2 = freqTrans.intState2;
				return (freqTrans.permit | freqTrans.stateDir);
			}
		}
	}
	return (IFX_NO_TRANS | IFX_STATE_NC);
}

#ifdef CONFIG_PROC_FS
static ssize_t ifx_cpufreq_proc_latency_write(struct file *file,
						const char *buf,
						size_t count, loff_t *ppos)
{

	ltq_cpufreq_latency_g[0][0] = 0xFFFFFFFF;
	ltq_cpufreq_latency_g[0][1] = 0;
	ltq_cpufreq_latency_g[0][2] = 0;
	ltq_cpufreq_latency_g[1][0] = 0xFFFFFFFF;
	ltq_cpufreq_latency_g[1][1] = 0;
	ltq_cpufreq_latency_g[1][2] = 0;
	ltq_cpufreq_latency_g[2][0] = 0xFFFFFFFF;
	ltq_cpufreq_latency_g[2][1] = 0;
	ltq_cpufreq_latency_g[2][2] = 0;
	return count;
}

static inline int ifx_cpufreq_proc_latency_show(struct seq_file *s, void *v)
{
	seq_printf(s, "PRE__Latency MIN:%4d[us], CUR:%4d[us], MAX:%4d[us]\n",
					ltq_cpufreq_latency_g[0][0],
					ltq_cpufreq_latency_g[0][1],
					ltq_cpufreq_latency_g[0][2]);
	seq_printf(s, "STAT_Latency MIN:%4d[us], CUR:%4d[us], MAX:%4d[us]\n",
					ltq_cpufreq_latency_g[1][0],
					ltq_cpufreq_latency_g[1][1],
					ltq_cpufreq_latency_g[1][2]);
	seq_printf(s, "POST_Latency MIN:%4d[us], CUR:%4d[us], MAX:%4d[us]\n",
					ltq_cpufreq_latency_g[2][0],
					ltq_cpufreq_latency_g[2][1],
					ltq_cpufreq_latency_g[2][2]);
	return 0;
}

static inline int ifx_cpufreq_proc_loglevel_show(struct seq_file *s, void *v)
{
	seq_printf(s, "log_level = %d\n", ltq_cpufreq_log_level_g);
	seq_puts(s, "Control the garrulity of the module\n");
	seq_puts(s, "<0 = quiet\n");
	seq_puts(s, " 0 = EMERG, ERRORS,\n");
	seq_puts(s, " 1 = + WARNINGS\n");
	seq_puts(s, " 2 = + INFO  (default)\n");
	seq_puts(s, " 3 = + PS REQUEST\n");
	seq_puts(s, " 4 = + FUNC CALL TRACE\n");
	seq_puts(s, " 5 = + DEBUG\n");
	return 0;
}

static ssize_t ifx_cpufreq_proc_loglevel_write(struct file *file,
						const char *buf,
						size_t count, loff_t *ppos)
{
	char str[20];
	int len, rlen;
	char* str_p;
	char* str_pp;

	len = count < sizeof(str) ? count : sizeof(str) - 1;
	rlen = len - copy_from_user(str, buf, len);
	/*substitute \n by \0*/
	str_p = strchr ( str, '\n' ); 
	*str_p = '\0';
	str_p = str;
	str_pp = strsep( &str_p, " ");
	ltq_cpufreq_log_level_g = (int)simple_strtol(str_pp, NULL, 0);
	return count;
}

static int delay_time_g = 1000; /*1000us*/
static inline int ifx_cpufreq_proc_udelaytest_show(struct seq_file *s, void *v)
{
	seq_printf(s, "delay_time = %d[us]\n", delay_time_g);
	seq_puts(s, "Attention: CP0_COUNT runs with cpu/2\n");
	seq_puts(s, "Take this into account during CP0_COUNT period calculation\n");
	return 0;
}

/* test function for udelay */
static ssize_t ifx_cpufreq_proc_udelaytest_write(struct file *file,
						const char *buf,
						size_t count, loff_t *ppos)
{
	char str[20];
	int len, rlen;
	u32 count_start, count_end;
	u64 count_diff;
	u64 time;
	int cpu_freq_MHz;

	len = count < sizeof(str) ? count : sizeof(str) - 1;
	rlen = len - copy_from_user(str, buf, len);
	delay_time_g = (int)simple_strtol(str, NULL, 0);

	cpu_freq_MHz=ifx_get_cpu_hz()/1000000;

	count_start = read_c0_count();
	udelay(delay_time_g);
	count_end = read_c0_count();
	count_diff = count_end - count_start;
	time = 2 * count_diff; /* count runs with cpu/2 */
	do_div(time, cpu_freq_MHz);

	LTQDBG_INFO(CPUFREQ,"delay_time measured = %llu[us], @%dHz, "
			 "count_val=%llu\n",time,cpu_freq_MHz,count_diff);

	return count;
}


#ifdef CONFIG_IFX_DCDC
static inline int ifx_cpufreq_proc_dcctrl_show(struct seq_file *s, void *v)
{
	seq_printf(s, "dcdc_ctrl = %d\n", dcdc_ctrl_g);
	seq_puts(s, "switching On/Off the voltage scaling \n");
	seq_puts(s, "     0 = disable\n");
	seq_puts(s, "     1 = enable\n");
	return 0;
}


static ssize_t ifx_cpufreq_proc_dcctrl_write(struct file *file,
						 const char *buf,
						 size_t count, loff_t *ppos)
{
	char str[20];
	char str1[2];
	int len, rlen;

	len = count < sizeof(str) ? count : sizeof(str) - 1;
	rlen = len - copy_from_user(str, buf, len);
	strncpy(str1,str,1);
	str1[1]='\0';
	dcdc_ctrl_g = (int)simple_strtol(str1, NULL, 0);

	return count;
}
#endif

static inline int ifx_cpufreq_proc_version_show(struct seq_file *s, void *v)
{
	seq_puts(s, "CPUFREQ Version = "IFX_CPUFREQ_DRV_VERSION"\n");
	seq_puts(s, "LXFREQ Version = "LTQ_LXFREQ_DRV_VERSION"\n");
	seq_puts(s, "Compiled on "__DATE__" "__TIME__" for Linux kernel "UTS_RELEASE"\n");
	return 0;    
}

static inline int ifx_cpufreq_proc_chipid_show(struct seq_file *s, void *v)
{
	int chipId;

	chipId = IFX_REG_R32(IFX_MPS_CHIPID);
	seq_printf(s, "CHIP_ID_REG = %08x\n",chipId);
	chipId = (chipId & 0x0FFFF000) >> 12;
	switch (chipId) {
		case ARX188:
			seq_printf(s, "CHIP_ID = ARX188 [XRX100]\n");
			break;
		case ARX168:
			seq_printf(s, "CHIP_ID = ARX168 [XRX100]\n");
			break;
		case ARX182:
		case ARX182_2:
			seq_printf(s, "CHIP_ID = ARX182 [XRX100]\n");
			break;
		case GRX188:
			seq_printf(s, "CHIP_ID = GRX188 [XRX100]\n");
			break;
		case GRX168:
			seq_printf(s, "CHIP_ID = GRX168 [XRX100]\n");
			break;
		case VRX288_A1x:
			seq_printf(s, "CHIP_ID = VRX288_A1x [XRX200_A1]\n");
			break;
		case VRX268_A1x:
			seq_printf(s, "CHIP_ID = VRX268_A1x [XRX200_A1]\n");
			break;
		case VRX282_A1x:
			seq_printf(s, "CHIP_ID = VRX282_A1x [XRX200_A1]\n");
			break;
		case GRX268_A1x:
			seq_printf(s, "CHIP_ID = GRX268_A1x [XRX200_A1]\n");
			break;
		case GRX288_A1x:
			seq_printf(s, "CHIP_ID = GRX288_A1x [XRX200_A1]\n");
			break;
		case VRX288_A2x:
			seq_printf(s, "CHIP_ID = VRX288_A2x [XRX200_A2]\n");
			break;
		case VRX268_A2x:
			seq_printf(s, "CHIP_ID = VRX268_A2x [XRX200_A2]\n");
			break;
		case VRX282_A2x:
			seq_printf(s, "CHIP_ID = VRX282_A2x [XRX200_A2]\n");
			break;
		case GRX288_A2x:
			seq_printf(s, "CHIP_ID = GRX288_A2x [XRX200_A2]\n");
			break;
		case ARX362:
			seq_printf(s, "CHIP_ID = ARX362 2x2 Router [XRX300]\n");
			break;
		case ARX382:
			seq_printf(s, "CHIP_ID = ARX382 2x2 Gateway [XRX300]\n");
			break;
		case GRX388:
			seq_printf(s, "CHIP_ID = GRX388 3x3 Ethernet Router"
					"/Gateway [XRX300]\n");
			break;
		case ARX368:
			seq_printf(s, "CHIP_ID = ARX368 3x3 High-end"
					" Router [XRX300]\n");
			break;
		case ARX388:
			seq_printf(s, "CHIP_ID = ARX388 3x3 High-end"
					" Gateway [XRX300]\n");
			break;
		case GRX383:
			seq_printf(s, "CHIP_ID = GRX383 2x2 Router [XRX390]\n");
			break;
		case GRX369:
			seq_printf(s, "CHIP_ID = GRX369 2x2 High-end"
					" Router [XRX390]\n");
			break;
		case GRX387:
			seq_printf(s, "CHIP_ID = GRX387 2x2 High-end Ethernet"
				" Router/Gateway with 3 GPHY [XRX390]\n");
			break;
		case GRX389:
			seq_printf(s, "CHIP_ID = GRX389 2x2 High-end Ethernet"
				" Router/Gateway with 4 GPHY [XRX390]\n");
			break;
		default:
			seq_printf(s, "CHIP_ID = undefined\n");
			break;
	}
	return 0;
}

static int ifx_cpufreq_version_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ifx_cpufreq_proc_version_show, NULL);
}

static int ifx_cpufreq_chipid_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ifx_cpufreq_proc_chipid_show, NULL);
}

static int ifx_cpufreq_latency_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ifx_cpufreq_proc_latency_show, NULL);
}

static int ifx_cpufreq_loglevel_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ifx_cpufreq_proc_loglevel_show, NULL);
}

static int ifx_cpufreq_udelaytest_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ifx_cpufreq_proc_udelaytest_show, NULL);
}

#ifdef CONFIG_IFX_DCDC
static int ifx_cpufreq_dcctrl_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ifx_cpufreq_proc_dcctrl_show, NULL);
}
#endif

static const struct file_operations ifx_cpufreq_version_proc_fops = {
	.open		= ifx_cpufreq_version_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations ifx_cpufreq_chipid_proc_fops = {
	.open		= ifx_cpufreq_chipid_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations ifx_cpufreq_latency_proc_fops = {
	.open		= ifx_cpufreq_latency_proc_open,
	.read		= seq_read,
	.write		= ifx_cpufreq_proc_latency_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations ifx_cpufreq_loglevel_proc_fops = {
	.open		= ifx_cpufreq_loglevel_proc_open,
	.read		= seq_read,
	.write		= ifx_cpufreq_proc_loglevel_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations ifx_cpufreq_udelaytest_proc_fops = {
	.open		= ifx_cpufreq_udelaytest_proc_open,
	.read		= seq_read,
	.write		= ifx_cpufreq_proc_udelaytest_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

#ifdef CONFIG_IFX_DCDC
static const struct file_operations ifx_cpufreq_dcctrl_proc_fops = {
	.open		= ifx_cpufreq_dcctrl_proc_open,
	.read		= seq_read,
	.write		= ifx_cpufreq_proc_dcctrl_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

static int ifx_cpufreq_proc_EntriesInstall(void)
{
	/*create pmcu proc entry*/
	struct proc_dir_entry *res;

	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);
	ifx_cpufreq_dir_cpufreq_g = proc_mkdir("driver/ifx_cpufreq", NULL);
	res = proc_create("version", 0, ifx_cpufreq_dir_cpufreq_g, 
					  &ifx_cpufreq_version_proc_fops);
	if (!res){return -ENOMEM;}
	res = proc_create("chipid", 0, ifx_cpufreq_dir_cpufreq_g, 
					  &ifx_cpufreq_chipid_proc_fops);
	if (!res){return -ENOMEM;}
	res = proc_create("latency", 0, ifx_cpufreq_dir_cpufreq_g, 
					  &ifx_cpufreq_latency_proc_fops);
	if (!res){return -ENOMEM;}
	res = proc_create("log_level", 0, ifx_cpufreq_dir_cpufreq_g, 
					  &ifx_cpufreq_loglevel_proc_fops);
	if (!res){return -ENOMEM;}
	res = proc_create("udelay_test", 0, ifx_cpufreq_dir_cpufreq_g, 
					  &ifx_cpufreq_udelaytest_proc_fops);
	if (!res){return -ENOMEM;}

#ifdef CONFIG_IFX_DCDC
	res = proc_create("dcdc_ctrl", 0, ifx_cpufreq_dir_cpufreq_g, 
					  &ifx_cpufreq_dcctrl_proc_fops);
	if (!res){return -ENOMEM;}
#endif

	return 0;
}

/**
	Initialize and install the proc entry

	\return
	-1 or 0 on success
*/
static int ifx_cpufreq_proc_EntriesRemove(void)
{

	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);
#ifdef CONFIG_IFX_DCDC
	remove_proc_entry("dcdc_ctrl", ifx_cpufreq_dir_cpufreq_g);
#endif
	remove_proc_entry("udelay_test", ifx_cpufreq_dir_cpufreq_g);
	remove_proc_entry("log_level", ifx_cpufreq_dir_cpufreq_g);
	remove_proc_entry("latency", ifx_cpufreq_dir_cpufreq_g);
	remove_proc_entry("chipid", ifx_cpufreq_dir_cpufreq_g);
	remove_proc_entry("version", ifx_cpufreq_dir_cpufreq_g);
	remove_proc_entry(ifx_cpufreq_dir_cpufreq_g->name, NULL);
	ifx_cpufreq_dir_cpufreq_g = NULL;
	return 0;
}
#endif

#if 0
/* DyingGasp interrupt handler */
static irqreturn_t  ifx_cpufreq_dgasp_irq_handle(int int1, void *void0)
{
	LTQDBG_ERR(CPUFREQ,"%s is called\n",__FUNCTION__);
	return IRQ_HANDLED;
}
#endif

static int __init ifx_cpufreq_init(void)
{
	IFX_PMCU_REGISTER_t pmcuRegister;
	
	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);
	#ifdef CPUFREQ_DEBUG_MODE
	/* init powerstate transition trace */
	memset (&state_trace, 0, sizeof(state_trace));
	#endif

	/* define the usage of mips_wait instruction inside linux idle loop */
	cpu_wait_sav = cpu_wait; /* save cpu_wait pointer */

	/* register CPU frequency change to PMCU */
	memset (&pmcuRegister, 0, sizeof(pmcuRegister));
	pmcuRegister.pmcuModule=IFX_PMCU_MODULE_CPU;
	pmcuRegister.pmcuModuleNr=0;
	pmcuRegister.pmcuModuleDep = &depList;
	pmcuRegister.pre = ifx_cpufreq_prechange;
	pmcuRegister.post = ifx_cpufreq_postchange;
	/* Functions used to change CPU state */
	/* and to get actual CPU state */
	pmcuRegister.ifx_pmcu_state_change = ifx_cpufreq_state_set;
	pmcuRegister.ifx_pmcu_state_get = ifx_cpufreq_state_get;
	pmcuRegister.ifx_pmcu_pwr_feature_switch = ifx_cpufreq_pwrFeatureSwitch;
	ifx_pmcu_register ( &pmcuRegister );

	/* register CPU clock gating to PMCU */
	memset (&pmcuRegister, 0, sizeof(pmcuRegister));
	pmcuRegister.pmcuModule=IFX_PMCU_MODULE_CPU;
	pmcuRegister.pmcuModuleNr=1;
	pmcuRegister.pmcuModuleDep = NULL;
	pmcuRegister.pre = ifx_cpucg_prechange;
	pmcuRegister.post = ifx_cpucg_postchange;
	pmcuRegister.ifx_pmcu_state_change = NULL;
	pmcuRegister.ifx_pmcu_state_get = ifx_cpucg_state_get;
	pmcuRegister.ifx_pmcu_pwr_feature_switch = ifx_cpucg_pwrFeatureSwitch;
	ifx_pmcu_register ( &pmcuRegister );

	ifx_cpufreq_state_get(&lastPmcuState_g);

	/* set to right frequency table according to the underlaying hardware */
	(void)ifx_cpufreq_set_freqtab();

#ifdef CONFIG_CPU_FREQ
	/* get the valid frequency table */
	if(((freqtab_p + 0)->cpu_freq) == 0){
		ltq_freq_tab[0].frequency = CPUFREQ_ENTRY_INVALID;
	}else{
		ltq_freq_tab[0].frequency = (freqtab_p + 0)->cpu_freq * 1000;/* kHz */
	}
	if(((freqtab_p + 1)->cpu_freq) == 0){
		ltq_freq_tab[1].frequency = CPUFREQ_ENTRY_INVALID;
	}else{
		ltq_freq_tab[1].frequency = (freqtab_p + 1)->cpu_freq * 1000;/* kHz */
	}
	if(((freqtab_p + 2)->cpu_freq) == 0){
		ltq_freq_tab[2].frequency = CPUFREQ_ENTRY_INVALID;
	}else{
		ltq_freq_tab[2].frequency = (freqtab_p + 2)->cpu_freq * 1000;/* kHz */
	}
	if(((freqtab_p + 3)->cpu_freq) == 0){
		ltq_freq_tab[3].frequency = CPUFREQ_ENTRY_INVALID;
	}else{
		ltq_freq_tab[3].frequency = (freqtab_p + 3)->cpu_freq * 1000;/* kHz */
	}
	/* register driver to the linux cpufreq driver */
	cpufreq_register_driver(&ltq_lxfreq_driver);
	/* add two entrys to the module status linked list */
	list_add_tail(&cpuddr_lxfreq_mod_g.list, &ltq_lxfreq_head_mod_list_g);
	list_add_tail(&cpuwait_lxfreq_mod_g.list, &ltq_lxfreq_head_mod_list_g);

	ltq_lxfreq_wq = create_workqueue("LTQ_LXFREQ");
#endif

#ifdef CONFIG_PROC_FS
	ifx_cpufreq_proc_EntriesInstall();
#endif

/*
	if (request_irq(IFX_DSL_DYING_GASP_INT, ifx_cpufreq_dgasp_irq_handle,
		0, "DYING_GASP", NULL) != 0) {
		LTQDBG_ERR(CPUFREQ,"request_irq DYING_GASP failed\n");
		return -1;
	}
*/
	return 0;
}

static void __exit ifx_cpufreq_exit (void)
{
	IFX_PMCU_REGISTER_t pmcuRegister;

	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);
	/*unregister CPU frequency scaling from PMCU */
	memset (&pmcuRegister, 0, sizeof(pmcuRegister));
	pmcuRegister.pmcuModule=IFX_PMCU_MODULE_CPU;
	pmcuRegister.pmcuModuleNr=0;
	ifx_pmcu_unregister ( &pmcuRegister );

	/*unregister CPU clock gating from PMCU */
	memset (&pmcuRegister, 0, sizeof(pmcuRegister));
	pmcuRegister.pmcuModule=IFX_PMCU_MODULE_CPU;
	pmcuRegister.pmcuModuleNr=1;
	ifx_pmcu_unregister ( &pmcuRegister );

#ifdef CONFIG_CPU_FREQ
	list_del(&cpuddr_lxfreq_mod_g.list);
	list_del(&cpuwait_lxfreq_mod_g.list);
	cpufreq_unregister_driver(&ltq_lxfreq_driver);

	if(ltq_lxfreq_wq){
		destroy_workqueue(ltq_lxfreq_wq);
	}
#endif

#ifdef CONFIG_PROC_FS
	ifx_cpufreq_proc_EntriesRemove();
#endif

	return;
}

module_init(ifx_cpufreq_init);
module_exit(ifx_cpufreq_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR("Lantiq Deutschland GmbH");
MODULE_DESCRIPTION ("LANTIQ CPUFREQ driver");
MODULE_SUPPORTED_DEVICE ("Amazon SE, Danube, XRX100, XRX200");

/* @} */ /* LQ_CPUFREQ */