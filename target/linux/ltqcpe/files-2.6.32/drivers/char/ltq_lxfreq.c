/*****************************************************************************

							   Copyright (c) 2012
							Lantiq Deutschland GmbH
					 Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*****************************************************************************/

#include <linux/version.h> 
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)) 
	#include <linux/utsrelease.h> 
#else 
	#include <generated/utsrelease.h> 
#endif
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ctype.h>
#include <linux/smp.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/cpu.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/cpufreq.h>
#include <ifx_pmcu.h>
#include <ifx_cpufreq.h>
#include <ltq_lxfreq.h>

/*==========================================================================*/
/* LXFREQ internal ENUMERATION												*/
/*==========================================================================*/
/** LTQ_LXFREQ_REQ_ID_t
	Status of a powerState REQUEST
*/
typedef enum {
	/** no pending powerState request pending in the ringBuffer */
	LTQ_LXFREQ_NO_PENDING_REQ		= 0,
	/** powerState request pending in the ringBuffer */
	LTQ_LXFREQ_PENDING_REQ			= 1,
	/** unknown powerState request in buffer or buffer overflow */
	LTQ_LXFREQ_PENDING_REQ_ERROR	= -1,
} LTQ_LXFREQ_REQ_ID_t;

/*==========================================================================*/
/* LXFREQ internal STRUCTURES 	   										    */
/*==========================================================================*/
/** LTQ_LXFREQ_REQ_STATE_t
	struct used in REQUEST ringBuffer to keep all relevant info's for one
	powerState request
*/
typedef struct {
	IFX_PMCU_MODULE_STATE_t	moduleState;
	LTQ_LXFREQ_REQ_ID_t			reqId;
}LTQ_LXFREQ_REQ_STATE_t;


/*==========================================================================*/
/* LOCAL FUNCTIONS PROTOTYPES   											*/
/*==========================================================================*/
unsigned int ltq_lxfreq_getfreq_khz(unsigned int cpu);
static int ltq_lxfreq_init (struct cpufreq_policy *policy);
static int ltq_lxfreq_verify (struct cpufreq_policy *policy);
static int ltq_lxfreq_target (struct cpufreq_policy *policy,
								unsigned int target_freq,
								unsigned int relation);
static int ltq_lxfreq_exit (struct cpufreq_policy *policy);
static int ltq_lxfreq_resume (struct cpufreq_policy *policy);
static LTQ_LXFREQ_REQ_STATE_t ltq_lxfreq_get_req( void );
static int ltq_lxfreq_put_req( LTQ_LXFREQ_REQ_STATE_t req );
static void ltq_lxfreq_process_req(struct work_struct *work);


/*==========================================================================*/
/* EXTERNAL FUNCTIONS PROTOTYPES											*/
/*==========================================================================*/
/* LTQ cgu driver */
extern unsigned int ifx_get_cpu_hz(void);

/* LINUX cpufreq driver */
extern  ssize_t store_scaling_governor(struct cpufreq_policy *policy,
										const char *buf, size_t count);

/*=============================================================================*/
/* WORKQUEUE DECLARATION  													   */
/*=============================================================================*/
static DECLARE_WORK(work_obj, ltq_lxfreq_process_req);

/*==========================================================================*/
/* module DEFINES 											                */
/*==========================================================================*/
/* size of request ringbuffer */
#define LTQ_REQ_BUFFER_SIZE 5

/** \ingroup LTQ_LXFREQ
    \brief  The ltq_lxfreq_lock is used to protect all data structures
			accessed from Interrupt AND Process-Context.
*/
static DEFINE_SPINLOCK(ltq_lxfreq_lock);

/*==========================================================================*/
/* EXTERNAL variable DECLARATIONS 											*/
/*==========================================================================*/
extern struct cpufreq_governor cpufreq_gov_performance;
extern int ltq_cpufreq_log_level_g;
extern int ltq_cpufreq_latency_g[3][3];
extern int ltq_cpufreq_adjust_udelay_g; /* defined in ifxmips_clk.c */


/*==========================================================================*/
/* EXTERNAL module parameters    											*/
/*==========================================================================*/
unsigned int transition_latency = 1000000;/*ns*/
module_param(transition_latency, uint, 0644);
MODULE_PARM_DESC(transition_latency, "used from the ONDEMAND governor to "
			"calculate the sampling_rate. This is how often you want the"
			"kernel to look for the CPU usage and make decisions on what to "
			"do with the frequency. Value must be given in ns. The"
			"sampling_rate is calculated by transition_latency * 1000."
			"E.g.: sampling_rate should be 5sec -> set transition_latency"
			"to 5000000.");

/*==========================================================================*/
/* Internal module global variable DECLARATIONS 							*/
/*==========================================================================*/
/*init linked list during compile time*/
LIST_HEAD(ltq_lxfreq_head_mod_list_g);
EXPORT_SYMBOL(ltq_lxfreq_head_mod_list_g);

struct workqueue_struct *ltq_lxfreq_wq;
static int ltq_state_change_enable = 1;

/** \ingroup LTQ_LXFREQ
    \brief  The powerState request ringBuffer collect the incoming power state
    		requests. The ringBuffer has a size of LTQ_REQ_BUFFER_SIZE,
			and will continuously start from the beginning with each overflow.  
*/
static LTQ_LXFREQ_REQ_STATE_t ltq_lxfreq_reqBuffer[ LTQ_REQ_BUFFER_SIZE ];

/** \ingroup LTQ_LXFREQ
	\brief  The reqGetIndex points to the next request entry to be processed.  
*/
static unsigned int ltq_lxfreq_reqGetIndex = 0;

/** \ingroup LTQ_LXFREQ
    \brief  The reqPutIndex points to the next free entry inside the ringBuffer
			to place the next incoming request.  
*/
static unsigned int ltq_lxfreq_reqPutIndex = 0;

/** \ingroup LTQ_LXFREQ
    \brief  The reqBufferSize is the ringBuffer watchdog to signalize a real
    		overflow of the ringBuffer. A real overflow means that the
			reqbuffer contains already LTQ_REQ_BUFFER_SIZE request entries
			and a new incoming request can not be placed and will be discarded.
*/
static unsigned int ltq_lxfreq_reqBufferSize = 0;


/** \ingroup LTQ_LXFREQ
	\brief  Control the powerstate change from the drivers.
			- 0 =   All powerstate changes received by the LXFREQ
					will be rejected. Default
			- 1  =  Powerstate changes are accepted by the LXFREQ
*/
static int ltq_state_change_control   = 0;

/* module names; array correspond to enum ITQ_CPUFREQ_MODULE_t */
char* ltq_lxfreq_mod[IFX_PMCU_MODULE_ID_MAX+1] = {
	"ALL" ,
	"CPU" ,
	"ETH" ,
	"USB" ,
	"DSL" ,
	"WLAN" ,
	"DECT" ,
	"FXS" ,
	"FXO" ,
	"VE" ,
	"PPE" ,
	"SWITCH" ,
	"UART" ,
	"SPI" ,
	"SDIO" ,
	"PCI" ,
	"VYLINQ" ,
	"DEU" ,
	"CPU_PS" ,
	"GPTC" ,
	"USIF_UART" ,
	"USIF_SPI" ,
	"PCIE" ,
	"INVALID" ,
};

/* CPUFREQ specific power state translation */
static char* ltq_lxfreq_ps[] = {
	"-1",
	"performance",	/*D0*/
	"ondemand",		/*D1*/
	"ondemand",		/*D2*/
	"ondemand",		/*D3*/
};

/* PMCU specific power state translation */
char* ltq_pmcu_ps[] = {
	"-1",
	"D0",
	"D1",
	"D2",
	"D3",
	"D0D3" ,
};


/** \ingroup LQ_LNXFREQ
	\brief  hold the current and previous frequency of the CPU
			- cpu  = 0 only one CPU is supported
			- old  = previous frequency
			- new  = current CPU frequency
			- flags= None
*/
static struct cpufreq_freqs ltq_freqs={0,0,0,0}; 

struct cpufreq_frequency_table ltq_freq_tab[]={
	{.index = 1, .frequency = 000000,},/*D0*/
	{.index = 2, .frequency = 000000,},/*D1*/
	{.index = 3, .frequency = 000000,},/*D2*/
	{.index = 4, .frequency = 000000,},/*D3*/
	{.index = 0, .frequency = CPUFREQ_TABLE_END,}
};


static struct freq_attr *ltq_freq_attr[] = {
			&cpufreq_freq_attr_scaling_available_freqs,
			NULL,
};

struct cpufreq_driver ltq_lxfreq_driver={

	.name		= "ltq_lxfreq_drv\n",          
	.owner		= THIS_MODULE,
	.init		= ltq_lxfreq_init,
	.flags		= 0,
	.verify		= ltq_lxfreq_verify,
	.setpolicy	= NULL,
	.target		= ltq_lxfreq_target,
	.get		= ltq_lxfreq_getfreq_khz,
	.getavg		= NULL,
	.exit		= ltq_lxfreq_exit,
	.suspend	= NULL,
	.resume		= ltq_lxfreq_resume,
	.attr		= ltq_freq_attr,
};

int ltq_lxfreq_state_change_disable(void) {                         
                                                                    
    ltq_state_change_enable = 0;                                    
    ltq_lxfreq_state_req(IFX_PMCU_MODULE_CPU, 0, IFX_PMCU_STATE_D0);
    return 0;                                                       
}                                                                   
                                                                    
int ltq_lxfreq_state_change_enable(void) {                          
                                                                    
    ltq_state_change_enable = 1;                                    
    ltq_lxfreq_state_req(IFX_PMCU_MODULE_CPU, 0, IFX_PMCU_STATE_D3);
    return 0;                                                       
}                                                                   

static void ltq_lxfreq_process_req(struct work_struct *work)
{
	struct cpufreq_policy policy_struct;
	struct cpufreq_policy *policy = &policy_struct;
	LTQ_LXFREQ_REQ_STATE_t reqState;

	LTQDBG_FUNC(LNXFREQ, "%s is called\n",__FUNCTION__);

	while (ltq_lxfreq_reqBufferSize > 0) {
		reqState = ltq_lxfreq_get_req();
		if (reqState.reqId == LTQ_LXFREQ_PENDING_REQ_ERROR) {
			LTQDBG_ERR(LNXFREQ, "LXFREQ Request Buffer underflow\n");
			return;
		}
		if (reqState.reqId == LTQ_LXFREQ_NO_PENDING_REQ) {
			LTQDBG_ERR(LNXFREQ, "No valid LXFREQ Request in buffer, "
								"but workqueue is scheduled!!!!!\n");
			return;
		}
	
        if (!ltq_state_change_enable){                                     
            LTQDBG_DEBUG(LNXFREQ, "Frequency down scaling disabled. Force "
                                  "PERFORMANCE governor\n");               
            reqState.moduleState.pmcuState = IFX_PMCU_STATE_D0;            
        }                                                                  
		policy->cpu = 0; /* we have only one CPU */
		policy = cpufreq_cpu_get(policy->cpu); 
		if (policy){
			if (lock_policy_rwsem_write(policy->cpu) < 0){
				goto fail;
			}
			policy->cpuinfo.transition_latency	= transition_latency; /* ns */
			store_scaling_governor(policy,
							   ltq_lxfreq_ps[reqState.moduleState.pmcuState],
						sizeof(ltq_lxfreq_ps[reqState.moduleState.pmcuState]));
			unlock_policy_rwsem_write(policy->cpu);
		fail:
			cpufreq_cpu_put(policy);               
		}
	}
	return;
}


int ltq_lxfreq_state_req (	IFX_PMCU_MODULE_t module, 
							unsigned char moduleNr, 
							IFX_PMCU_STATE_t newState)
{
	struct cpufreq_policy policy_struct;
	struct cpufreq_policy *policy = &policy_struct;
	LTQ_LXFREQ_REQ_STATE_t reqState;

	LTQDBG_FUNC(LNXFREQ, "%s is called: Module=%s, ModuleNr=%d, State=%s\n",
				__FUNCTION__,ltq_lxfreq_mod[module],moduleNr, 
				ltq_pmcu_ps[newState]);
	LTQDBG_REQ(LNXFREQ, "-%s- module request new governor -%s-\n",
			   ltq_lxfreq_mod[module],ltq_lxfreq_ps[newState]);
	/* check if the acception of the powerstate request is enabled */
	if (ltq_state_change_control == 0) {
		LTQDBG_DEBUG(LNXFREQ, "The PowerStateChange from driver level is "
							  "disabled. Each request will be rejected\n");
		return 0;
	}

	/*check if the new request is already active*/
	policy->cpu = 0; /* we have only one CPU */
	policy = cpufreq_cpu_get(policy->cpu);
	if (policy){
		if(strcmp(policy->governor->name, ltq_lxfreq_ps[newState]) == 0){
			return 0;
		}
	}

	reqState.moduleState.pmcuModule = module;
	reqState.moduleState.pmcuModuleNr = moduleNr;
	reqState.moduleState.pmcuState = newState;
	reqState.reqId = LTQ_LXFREQ_NO_PENDING_REQ;

	/* put new powerState request into request buffer */
	if (ltq_lxfreq_put_req(reqState) == IFX_PMCU_RETURN_ERROR) {
		LTQDBG_ERR(LNXFRQ, "LXFREQ RequestBuffer overflow !!!\n");
	}

	/* feed the LTQ_LXFREQ workqueue.
	   fetch powerState request from requestBuffer by the wq */
	if ( !queue_work(ltq_lxfreq_wq, &work_obj) ){
		LTQDBG_DEBUG(LNXFRQ, "LXFREQ workqueue successfully loaded\n");
	}

	return 0;
}
EXPORT_SYMBOL(ltq_lxfreq_state_req);

static int ltq_lxfreq_put_req( LTQ_LXFREQ_REQ_STATE_t req )
{
	int i;
	unsigned long iflags;

	LTQDBG_FUNC(LNXFREQ, "%s is called\n",__FUNCTION__);
	if ( ltq_lxfreq_reqBufferSize >= LTQ_REQ_BUFFER_SIZE ) {
		return -1;
	}

	spin_lock_irqsave(&ltq_lxfreq_lock, iflags);

	/* first check if there is already a request for this module pending.
	   If this is the case, reject the actual one. */
	for (i=0;i<LTQ_REQ_BUFFER_SIZE;i++) {
		if (ltq_lxfreq_reqBuffer[i].reqId == LTQ_LXFREQ_NO_PENDING_REQ) {
			continue;
		}
		if ((ltq_lxfreq_reqBuffer[i].moduleState.pmcuModule == req.moduleState.pmcuModule) &&
			(ltq_lxfreq_reqBuffer[i].moduleState.pmcuModuleNr == req.moduleState.pmcuModuleNr) && 
			(ltq_lxfreq_reqBuffer[i].moduleState.pmcuState == req.moduleState.pmcuState) ) 
		{
			spin_unlock_irqrestore(&ltq_lxfreq_lock, iflags);
			LTQDBG_FUNC(LNXFREQ, "%s detect's multiple requests from same "
								 "module. Discard request.\n",__FUNCTION__);
			return 0;
		}
	}

	req.reqId = LTQ_LXFREQ_PENDING_REQ;
	memcpy(&ltq_lxfreq_reqBuffer[ ltq_lxfreq_reqPutIndex ], &req, sizeof(req));
	ltq_lxfreq_reqPutIndex++;
	if ( ltq_lxfreq_reqPutIndex >= LTQ_REQ_BUFFER_SIZE ) {
		ltq_lxfreq_reqPutIndex = 0;
	}
	ltq_lxfreq_reqBufferSize++;
	spin_unlock_irqrestore(&ltq_lxfreq_lock, iflags);
	return 0;
}

static LTQ_LXFREQ_REQ_STATE_t ltq_lxfreq_get_req( void )
{
	LTQ_LXFREQ_REQ_STATE_t req;

	LTQDBG_FUNC(LNXFREQ, "%s is called\n",__FUNCTION__);
	req.reqId = LTQ_LXFREQ_PENDING_REQ_ERROR;

	if ( !ltq_lxfreq_reqBufferSize ) {
		return req;
	}
	ltq_lxfreq_reqBufferSize--;
	memcpy(&req, &ltq_lxfreq_reqBuffer[ ltq_lxfreq_reqGetIndex ], sizeof(req));
	ltq_lxfreq_reqBuffer[ ltq_lxfreq_reqGetIndex ].reqId = LTQ_LXFREQ_NO_PENDING_REQ;
	ltq_lxfreq_reqGetIndex++;
	if ( ltq_lxfreq_reqGetIndex >= LTQ_REQ_BUFFER_SIZE ) {
		ltq_lxfreq_reqGetIndex = 0;
	}
	return req;
}

/* convert given frequency in kHz to corresponding power state */
IFX_PMCU_STATE_t ltq_lxfreq_get_ps_from_khz(unsigned int freqkhz)
{
	struct cpufreq_frequency_table* ltq_freq_tab_p;
	ltq_freq_tab_p = ltq_freq_tab;
	//LTQDBG_FUNC(LNXFREQ, "%s is called\n",__FUNCTION__);

	while(ltq_freq_tab_p->frequency != CPUFREQ_TABLE_END) {
		if(ltq_freq_tab_p->frequency == freqkhz) {
			return (IFX_PMCU_STATE_t)ltq_freq_tab_p->index;
		}
		ltq_freq_tab_p++;
	}
	return (IFX_PMCU_STATE_t)ltq_freq_tab_p->index;
}
EXPORT_SYMBOL(ltq_lxfreq_get_ps_from_khz);


unsigned int ltq_lxfreq_getfreq_khz(unsigned int cpu)
{
	//LTQDBG_FUNC(LNXFREQ, "%s is called\n",__FUNCTION__);
	/* The driver only support single cpu */
	if (cpu != 0)
		return -1;

	return (ifx_get_cpu_hz() / 1000000) * 1000; /* round off */;
}
EXPORT_SYMBOL(ltq_lxfreq_getfreq_khz);

struct cpufreq_policy *policy_sav;

static int ltq_lxfreq_init (struct cpufreq_policy *policy)
{
	LTQDBG_FUNC(LNXFREQ, "%s is called\n",__FUNCTION__);
	policy_sav = policy;
	policy->cpuinfo.min_freq			= 125000; /* 125MHz */
	policy->cpuinfo.max_freq			= 500000; /* 500MHz */
	policy->cpuinfo.transition_latency	= transition_latency; /* ns */
	policy->cur							= (ifx_get_cpu_hz() / 1000000) * 1000;
	policy->min							= 125000;
	policy->max							= 500000;
	policy->policy						= 0;
	policy->governor					= &cpufreq_gov_performance;

	if (policy->cpu != 0){
		return -EINVAL;
	}
	cpufreq_frequency_table_cpuinfo(policy,ltq_freq_tab);
	cpufreq_frequency_table_get_attr(ltq_freq_tab, policy->cpu);
	ltq_freqs.new = policy->cur;
	return 0;
}

static int ltq_lxfreq_verify (struct cpufreq_policy *policy)
{
	LTQDBG_FUNC(LNXFREQ, "%s is called\n",__FUNCTION__);
	cpufreq_frequency_table_verify(policy,ltq_freq_tab);
	return 0;
}

static int ltq_lxfreq_target (struct cpufreq_policy *policy,
								unsigned int target_freq,
								unsigned int relation)
{
	unsigned int tab_index,ret=0;
	int freqOk = 0;
	struct cpufreq_freqs freqs;
	u32 count_start;

	LTQDBG_REQ(LNXFREQ, "%s is called\n",__FUNCTION__);

	if (!ltq_state_change_control){
		LTQDBG_DEBUG(LNXFREQ, "Frequency down scaling disabled.\n");
		return -EINVAL;
	}
	if (cpufreq_frequency_table_target(policy, ltq_freq_tab,
		 target_freq, relation, &tab_index)){
		return -EINVAL;
	}
	freqs.new = ltq_freq_tab[tab_index].frequency;
	freqs.old = (ifx_get_cpu_hz() / 1000000) * 1000; /* round off */
	freqs.cpu = 0;
	ltq_cpufreq_adjust_udelay_g = 0; /* indicator for cgu_set_clock() */
	/* interrupt the process here if frequency doesn't change */
	if (freqs.new == freqs.old){
		return 0; /* nothing to do */
	}

	LTQDBG_DEBUG(LNXFREQ, "cpufreq_notify_transition, CPUFREQ_PRECHANGE\n");
	count_start = ltq_count0_read();
	ret = cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
	//ltq_count0_diff(count_start, ltq_count0_read(),"PRE",0);
	if((ret & NOTIFY_STOP_MASK) == NOTIFY_STOP_MASK) {
		ret = (ret >> 4) & 0x1F; /*mask module id*/
		LTQDBG_DEBUG(LNXFREQ, "Frequency scaling was denied by module %s\n",ltq_lxfreq_mod[ret]);
	}else{
		count_start = ltq_count0_read();
        if (ifx_cpufreq_state_set(ltq_freq_tab[tab_index].index)){
        }else{                                                    
            freqOk = 1;                                           
        }                                                         
        //ltq_count0_diff(count_start, ltq_count0_read(),"SET",1);   
	}
	if(!freqOk){
	/* if frequency change is denied, call post processing with new=old */
		freqs.new = freqs.old;
	}
	LTQDBG_DEBUG(LNXFREQ, "cpufreq_notify_transition, CPUFREQ_POSTCHANGE\n");
	count_start = ltq_count0_read();
	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	//ltq_count0_diff(count_start, ltq_count0_read(),"POST",2);

	/* copy loops_per_jiffy to cpu_data */
	cpu_data[smp_processor_id()].udelay_val = loops_per_jiffy;
	ltq_cpufreq_adjust_udelay_g = 1; /* set to default */

	return 0;
}

static int ltq_lxfreq_exit (struct cpufreq_policy *policy)
{
	LTQDBG_FUNC(LNXFREQ, "%s is called\n",__FUNCTION__);
	return 0;
}

static int ltq_lxfreq_resume (struct cpufreq_policy *policy)
{
	LTQDBG_FUNC(LNXFREQ, "%s is called\n",__FUNCTION__);
	return 0;
}

/*==========================================================================*/
/* SYSFS HELPER FUNCTION CALLED FROM LINUX CPUFREQ							*/
/*==========================================================================*/
int ltq_lxfreq_read_modstat(char *buf)
{
	int len = 0;
	LTQ_LXFREQ_MODSTRUCT_t* cur_list_pos;
	IFX_PMCU_STATE_t pState;

	LTQDBG_FUNC(LNXFREQ,"%s is called\n",__FUNCTION__);
	len += sprintf(buf+len, "\n\n");
	len += sprintf(buf+len, "  ltq_state_change_control = %s\n\n",
									(ltq_state_change_control==1)?"ON":"OFF");
	len += sprintf(buf+len, BLUE"  Legend:\n"NORMAL);
	len += sprintf(buf+len, BLUE"  D0 = OnState, D3 = IdleState, D1,D2 = "
							"intermediate.\n"NORMAL);
	len += sprintf(buf+len, BLUE"  PS = current Power State of the module\n"NORMAL);
	len += sprintf(buf+len, BLUE"  SubNo = Module instance\n\n"NORMAL);
	list_for_each_entry(cur_list_pos, &ltq_lxfreq_head_mod_list_g,list) {
		len += sprintf(buf+len, BLUE"  Comment = %s\n"NORMAL,cur_list_pos->name);
		len += sprintf(buf+len, "  Mod = %s\tSubNo = %d",
					   ltq_lxfreq_mod[cur_list_pos->pmcuModule],
					   cur_list_pos->pmcuModuleNr);
		cur_list_pos->ltq_lxfreq_state_get(&pState);
		len += sprintf(buf+len, "\tPS = %s\t",ltq_pmcu_ps[pState]);
		if ((cur_list_pos->powerFeatureStat == IFX_PMCU_PWR_STATE_ON)
			|| (cur_list_pos->powerFeatureStat == IFX_PMCU_PWR_STATE_OFF)){
			len += sprintf(buf+len, "\tPowerFeature = %s\n\n",
					(cur_list_pos->powerFeatureStat == IFX_PMCU_PWR_STATE_ON ? 
																"ON" : "OFF"));
		}else{
			len += sprintf(buf+len, "\n\n");
		}
	}
	len += sprintf(buf+len, "\n\n");
	return len;
}
EXPORT_SYMBOL(ltq_lxfreq_read_modstat);

int ltq_lxfreq_write_modstat(const char *buf, unsigned long count)
{
	char str[20];
	int len, i, ret;
	char* str_p;
	char* str_pp;
	IFX_PMCU_MODULE_t module = -1;
	int subModNr;
	IFX_PMCU_PWR_STATE_ENA_t pwrF;
	LTQ_LXFREQ_MODSTRUCT_t* cur_list_pos;

	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);
	len = count < sizeof(str) ? count : sizeof(str) - 1;
	strcpy(str, buf);
	/*convert to uppercase*/
	for (i = 0; i < count; i++)	{ 
		str[i]=toupper(str[i]);
	}
	/*substitute \n by \0*/
	str_p = strchr ( str, '\n' ); 
	if (str_p){
		*str_p = '\0';
	}
    str_p = str;                 
                                 
	/*get moduleNr*/
	str_pp = strsep( &str_p, " ");
	for(i=0;i<=IFX_PMCU_MODULE_ID_MAX;i++){
		if(0 == strcmp(str_pp,ltq_lxfreq_mod[i])){
			module = i; /*string found*/
			break;
		}
	}
	if(module < 0){
		LTQDBG_ERR(CPUFREQ,"given module name not found\n");
		return count;
	}

	/*get SubmoduleNr*/
	str_pp = strsep( &str_p, " ");
	subModNr = (int)simple_strtol(str_pp, NULL, 0);

	/*get powerFeature*/
	str_pp = strsep( &str_p, " ");
	if(0 == strcmp(str_pp,"ON")){
		pwrF = IFX_PMCU_PWR_STATE_ON;
	}else if (0 == strcmp(str_pp,"OFF")) {
		pwrF = IFX_PMCU_PWR_STATE_OFF;
	}else{
		LTQDBG_ERR(CPUFREQ,"given powerFeature parameter is invalid. "
						   " ('ON' or 'OFF')\n");
		return count;
	}
/*LTQDBG_DEBUG(CPUFREQ,"module=%d, subNr=%d, pf=%d\n",module,subModNr,pwrF);*/
	list_for_each_entry(cur_list_pos, &ltq_lxfreq_head_mod_list_g,list) {
		if((cur_list_pos->pmcuModule == module) || 
		   (module == IFX_PMCU_MODULE_PMCU)){
			if((cur_list_pos->pmcuModuleNr == subModNr) || 
			   (module == IFX_PMCU_MODULE_PMCU)){
				if ((cur_list_pos->powerFeatureStat == IFX_PMCU_PWR_STATE_ON)
				|| (cur_list_pos->powerFeatureStat == IFX_PMCU_PWR_STATE_OFF)){
					ret=cur_list_pos->ltq_lxfreq_pwr_feature_switch(pwrF);
					if(ret == IFX_PMCU_RETURN_SUCCESS){
						cur_list_pos->powerFeatureStat = pwrF;
						LTQDBG_INFO(CPUFREQ,"PowerFeature of module %s, "
									"SubNo %d is set to %s\n",
									ltq_lxfreq_mod[module],
									subModNr,
									(pwrF==IFX_PMCU_PWR_STATE_ON)?"ON":"OFF");
					}
				}
			}
		}
	}
	return count;
}
EXPORT_SYMBOL(ltq_lxfreq_write_modstat);


int ltq_lxfreq_read_state_change_control(char *buf)
{
	int len = 0;

	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);
	len += sprintf(buf, "ltq_state_change_control = %s\n", 
				   (ltq_state_change_control==1)?"ON":"OFF");
	return len;
}
EXPORT_SYMBOL(ltq_lxfreq_read_state_change_control);

int ltq_lxfreq_write_state_change_control(const char *buf, unsigned long count)
{
	char str[20];
	int len, i;
	char* str_p;
	char* str_pp;

	LTQDBG_FUNC(CPUFREQ,"%s is called\n",__FUNCTION__);
	len = count < sizeof(str) ? count : sizeof(str) - 1;
	strcpy(str, buf);
	/*convert to uppercase*/
	for (i = 0; i < count; i++)	{ 
		str[i]=toupper(str[i]);
	}
	/*substitute \n by \0*/
	str_p = strchr ( str, '\n' ); 
	if (str_p){
		*str_p = '\0';
	}
	str_p = str;
	str_pp = strsep( &str_p, " ");
	if(0 == strcmp(str_pp,"ON")){
		ltq_state_change_control = 1;
	}else if (0 == strcmp(str_pp,"OFF")) {
		ltq_state_change_control = 0;
	}else{
		LTQDBG_ERR(CPUFREQ,"given state_change_control parameter is invalid. "
						   " ('ON' or 'OFF')\n");
	}
	return count;
}
EXPORT_SYMBOL(ltq_lxfreq_write_state_change_control);

/* time measurement routines for debug purpose only */
#define COUNT0_MAX 0xFFFFFFFF
#define RED		"\033[31;1m"
#define NORMAL	"\033[0m"
extern unsigned int ifx_get_cpu_hz(void);
static int cpu_freq_MHz;

u32 ltq_count0_diff(u32 count_start, u32 count_end, char* ident, int index)
{
    u64 diff,time;
    if (count_start >= count_end){
		 diff = (COUNT0_MAX - count_start) + count_end;
    }else{ 
		diff = count_end - count_start;
	}
	time = 2 * diff; /* count runs with cpu/2 */
	do_div(time, cpu_freq_MHz);
	switch(index){
		case 0:/*PRE*/
		{
			if((u32)time < ltq_cpufreq_latency_g[0][0]){
				ltq_cpufreq_latency_g[0][0] = (u32)time;/*min time*/
			}
			ltq_cpufreq_latency_g[0][1] = (u32)time;/*current time*/
			if((u32)time > ltq_cpufreq_latency_g[0][2]){
				ltq_cpufreq_latency_g[0][2] = (u32)time;/*max time*/
			}
			break;
		}
		case 1:/*STATE CHANGE*/
		{
			if((u32)time < ltq_cpufreq_latency_g[1][0]){
				ltq_cpufreq_latency_g[1][0] = (u32)time;/*min time*/
			}
			ltq_cpufreq_latency_g[1][1] = (u32)time;/*current time*/
			if((u32)time > ltq_cpufreq_latency_g[1][2]){
				ltq_cpufreq_latency_g[1][2] = (u32)time;/*max time*/
			}
			break;
		}
		case 2:/*POST*/
		{
			if((u32)time < ltq_cpufreq_latency_g[2][0]){
				ltq_cpufreq_latency_g[2][0] = (u32)time;/*min time*/
			}
			ltq_cpufreq_latency_g[2][1] = (u32)time;/*current time*/
			if((u32)time > ltq_cpufreq_latency_g[2][2]){
				ltq_cpufreq_latency_g[2][2] = (u32)time;/*max time*/
			}
			break;
		}
		default: break;
	}

	if(ident){
		printk(KERN_ERR RED"%s: delay_time measured = %llu[us], @%dHz, "
						"count_val=%llu\n"NORMAL,ident,time,cpu_freq_MHz,diff);
	}
	return diff;
}
EXPORT_SYMBOL(ltq_count0_diff);

u32 ltq_count0_read(void)
{
	cpu_freq_MHz=ifx_get_cpu_hz()/1000000;
	return read_c0_count();
}
EXPORT_SYMBOL(ltq_count0_read);

