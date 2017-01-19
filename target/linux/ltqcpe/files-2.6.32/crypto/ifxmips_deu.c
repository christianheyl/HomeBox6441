/******************************************************************************
**
** FILE NAME    : ifxmips_deu.c
** PROJECT      : IFX UEIP
** MODULES      : DEU Module for Danube
**
** DATE         : September 8, 2009
** AUTHOR       : Mohammad Firdaus
** DESCRIPTION  : Data Encryption Unit Driver
** COPYRIGHT    :       Copyright (c) 2009
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author             $Comment
** 08,Sept 2009 Mohammad Firdaus    Initial UEIP release
*******************************************************************************/

/*!
  \defgroup IFX_DEU IFX_DEU_DRIVERS
  \ingroup API
  \brief ifx deu driver module
*/

/*!
  \file	ifxmips_deu.c
  \ingroup IFX_DEU
  \brief main deu driver file
*/

/*!
 \defgroup IFX_DEU_FUNCTIONS IFX_DEU_FUNCTIONS
 \ingroup IFX_DEU
 \brief IFX DEU functions
*/

/* Project header */
#include <linux/version.h>
#if defined(CONFIG_MODVERSIONS)
#define MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/module.h>
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/crypto.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>       /* Stuff about file systems that we need */
#include <linux/cpufreq.h>
#include <asm/byteorder.h>
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>
#include <asm/ifx/irq.h>
#include <asm/ifx/ifx_dma_core.h>
#include <asm/ifx/ifx_pmu.h>
#include <asm/ifx/ifx_pmcu.h>
#include <asm/ifx/ifx_gpio.h>
#include "ifxmips_deu.h"
#include <ltq_lxfreq.h>

#if defined(CONFIG_DANUBE)
#include "ifxmips_deu_danube.h"
#elif defined(CONFIG_AR9)
#include "ifxmips_deu_ar9.h"
#elif defined(CONFIG_VR9) || defined(CONFIG_AR10)
#include "ifxmips_deu_vr9.h"
#else 
#error "Platform unknown!"
#endif /* CONFIG_xxxx */

#ifdef CONFIG_CRYPTO_DEV_DMA
static int init_dma = 0;
void deu_dma_priv_init(void);
#endif
static u32 deu_power_flag;
spinlock_t pwr_lock;
spinlock_t global_deu_lock;
void powerup_deu(int crypto);
void powerdown_deu(int crypto);
void aes_chip_init(void);
void des_chip_init(void);

void chip_version(void);
extern struct deu_proc ifx_deu_algo[];
struct proc_dir_entry *g_deu_proc_dir = NULL;

#ifdef CONFIG_CPU_FREQ
/* Linux CPUFREQ support start */
static IFX_PMCU_RETURN_t ifx_deu_stateGet(IFX_PMCU_STATE_t *pmcuModState);
static IFX_PMCU_RETURN_t ifx_deu_postChange(IFX_PMCU_MODULE_t pmcuModule, IFX_PMCU_STATE_t newState, IFX_PMCU_STATE_t oldState);
static IFX_PMCU_RETURN_t ifx_deu_preChange(IFX_PMCU_MODULE_t pmcuModule, IFX_PMCU_STATE_t newState, IFX_PMCU_STATE_t oldState);
static IFX_PMCU_RETURN_t ifx_deu_stateChange(IFX_PMCU_STATE_t newState);
static IFX_PMCU_RETURN_t ifx_deu_pwrFeatureSwitch(IFX_PMCU_PWR_STATE_ENA_t pmcuPwrStateEna);

extern struct list_head ltq_lxfreq_head_mod_list_g;

struct LTQ_LXFREQ_MODSTRUCT ifx_deu_lxfreq_mod_g = {
	.name							= "DEU clock gating support",
	.pmcuModule						= IFX_PMCU_MODULE_DEU,
	.pmcuModuleNr					= 0,
	.powerFeatureStat				= IFX_PMCU_PWR_STATE_ON,
	.ltq_lxfreq_state_get			= ifx_deu_stateGet,
	.ltq_lxfreq_pwr_feature_switch	= ifx_deu_pwrFeatureSwitch,
};

static int ifx_deu_cpufreq_notifier(struct notifier_block *nb, unsigned long val, void *data);
static struct notifier_block ifx_deu_cpufreq_notifier_block = {
	.notifier_call  = ifx_deu_cpufreq_notifier
};

/* keep track of frequency transitions */
static int
ifx_deu_cpufreq_notifier(struct notifier_block *nb, unsigned long val, void *data)
{
	struct cpufreq_freqs *freq = data;
	IFX_PMCU_STATE_t new_State, old_State;
	IFX_PMCU_RETURN_t ret;

	new_State = ltq_lxfreq_get_ps_from_khz(freq->new);
	if(new_State == IFX_PMCU_STATE_INVALID) {
		return NOTIFY_STOP_MASK | (IFX_PMCU_MODULE_DEU<<4);
	}
	old_State = ltq_lxfreq_get_ps_from_khz(freq->old);
	if(old_State == IFX_PMCU_STATE_INVALID) {
		return NOTIFY_STOP_MASK | (IFX_PMCU_MODULE_DEU<<4);
	}
	if (val == CPUFREQ_PRECHANGE){
		ret = ifx_deu_preChange(IFX_PMCU_MODULE_DEU, new_State, old_State);
		if (ret == IFX_PMCU_RETURN_DENIED) {
			return NOTIFY_STOP_MASK | (IFX_PMCU_MODULE_DEU<<4);
		}
		ret = ifx_deu_stateChange(new_State);
		if (ret == IFX_PMCU_RETURN_DENIED) {
			return NOTIFY_STOP_MASK | (IFX_PMCU_MODULE_DEU<<4);
		}
	} else if (val == CPUFREQ_POSTCHANGE){
		ret = ifx_deu_postChange(IFX_PMCU_MODULE_DEU, new_State, old_State);
		if (ret == IFX_PMCU_RETURN_DENIED) {
			return NOTIFY_STOP_MASK | (IFX_PMCU_MODULE_DEU<<4);
		}
	}else{
		return NOTIFY_OK | (IFX_PMCU_MODULE_DEU<<4);
	}
	return NOTIFY_OK | (IFX_PMCU_MODULE_DEU<<4);
}
#endif
/* Linux CPUFREQ support end */

/* CoC flag, 1: CoC is turned on. 0: CoC is turned off. */    
static int deu_power_status;

#define MAX_NAME_MASK 40

/*! \fn static ifx_deu_write_proc(struct file *file, const char *buf,
 *                                unsigned long count, void *data)
 *  \ingroup IFX_DEU_FUNCTIONS
 *  \brief proc interface write function
 *  \return buffer length
*/

static int ifx_deu_write_proc(struct file *file, const char *buf,
                                         unsigned long count, void *data)
{
    int i;
    int add;
    int pos = 0;
    int done = 0;
    const char *x;
    char *mask_name;
    char substring[MAX_NAME_MASK];
    
    while (!done && (pos < count)) {
        done = 1;
	while ((pos < count) && isspace(buf[pos]))
	    pos++;

	switch (buf[pos]) {
	    case '+': 
	    case '-':
	    case '=':
	        add = buf[pos];
			pos++;
		break;

	    default:
		add = ' ';
		break;
        }
	
	mask_name = NULL;

	for (x = buf + pos, i = 0;
	    (*x == '_' || (*x >= 'a' && *x <= 'z') || (*x >= '0' && *x <= '9')) && 
	     i < MAX_NAME_MASK; x++, i++, pos++) 	
	    substring[i] = *x;

        substring[i]='\0';
		printk("substring: %s\n", substring);

        for (i = 0; i < MAX_DEU_ALGO; i++) {
	    	if (strcmp(substring, ifx_deu_algo[i].name) == 0) {
				done = 0;
				mask_name = ifx_deu_algo[i].name;	
				printk("mask name: %s\n", mask_name);
				break;	
	    	}
        }

	if (mask_name != NULL) {
	    switch(add) {
		case '+':
		    if (ifx_deu_algo[i].deu_status == 0) {
				ifx_deu_algo[i].deu_status = 1;
		 		ifx_deu_algo[i].toggle_algo(0);
		    }
		    break;
		case '-':
            if (ifx_deu_algo[i].deu_status == 1) {
                ifx_deu_algo[i].deu_status = 0;
				ifx_deu_algo[i].toggle_algo(1);
            }
		    break;
		default:
		    break;
	    }
	}
	
    }

    return count;

}

/*! \fn static  int ifx_deu_read_proc(char *page,char **start,
 *	                               off_t offset, int count, int *eof, void *data)
 *  \ingroup IFX_DEU_FUNCTIONS
 *  \brief proc interface read function
 *  \return buffer length
*/

static int ifx_deu_read_proc(char *page,
                           char **start,
                           off_t offset, int count, int *eof, void *data)
{
    int i, ttl_len = 0, len = 0;
    char str[1024];
    char *pstr;

    pstr = *start = page;
    
    for (i = 0; i < MAX_DEU_ALGO; i++) {
        len = sprintf(str, "DEU algo name: %s, registered: %s\n", ifx_deu_algo[i].name, 
        	   (ifx_deu_algo[i].deu_status ? "YES" : "NO"));
    
        if (ttl_len <= offset && len + ttl_len > offset) {
	    memcpy(pstr, str + offset - ttl_len, len + ttl_len - offset);
	    pstr += ttl_len + len - offset;
	}
	else if (ttl_len > offset) {
            memcpy(pstr, str, len);
	    pstr += len;
        }
      
        ttl_len += len;

        if (ttl_len >= (offset + count))
            goto PROC_READ_OVERRUN;
    }

    *eof = 1;

     return ttl_len - offset;

PROC_READ_OVERRUN:
     return ttl_len - len - offset;

}

/*! \fn static  int ifx_deu_proc_create(void)
 *  \ingroup IFX_DEU_FUNCTIONS
 *  \brief proc interface initialization
 *  \return 0 if success, -ENOMEM if fails 
*/

static int ifx_deu_proc_create(void)
{
    g_deu_proc_dir = create_proc_entry("driver/ifx_deu", 0775,  NULL);
    if (g_deu_proc_dir == NULL) {
        return -ENOMEM;
    }

    g_deu_proc_dir->write_proc = ifx_deu_write_proc;
    g_deu_proc_dir->read_proc = ifx_deu_read_proc;
    g_deu_proc_dir->data = NULL;

    return 0;
}

#if defined (CONFIG_CPU_FREQ) || defined (CONFIG_IFX_PMCU)
static IFX_PMCU_RETURN_t ifx_deu_preChange(IFX_PMCU_MODULE_t pmcuModule, IFX_PMCU_STATE_t newState, IFX_PMCU_STATE_t oldState)
{
    return IFX_PMCU_RETURN_SUCCESS;
}

static IFX_PMCU_RETURN_t ifx_deu_postChange(IFX_PMCU_MODULE_t pmcuModule, IFX_PMCU_STATE_t newState, IFX_PMCU_STATE_t oldState)
{
    return IFX_PMCU_RETURN_SUCCESS;
}

static IFX_PMCU_RETURN_t ifx_deu_stateChange(IFX_PMCU_STATE_t newState)
{
    return IFX_PMCU_RETURN_SUCCESS;
}

/*! \fn static IFX_PMCU_RETURN_t ifx_deu_stateGet(IFX_PMCU_STATE_t *pmcuModState)
 *  \ingroup IFX_DEU_FUNCTIONS
 *  \brief provide the current state of the DEU hardware
 *  \return IFX_PMCU_RETURN_SUCCESS
*/

static IFX_PMCU_RETURN_t ifx_deu_stateGet(IFX_PMCU_STATE_t *pmcuModState)
{

    if (deu_power_status == 0)
	    *pmcuModState = IFX_PMCU_STATE_INVALID;

    if (deu_power_status == 1) {
        if (deu_power_flag == 0)
  	    *pmcuModState = IFX_PMCU_STATE_D3;
	else 
        *pmcuModState = IFX_PMCU_STATE_D0;
    }

    return IFX_PMCU_RETURN_SUCCESS;
}
#endif /* CONFIG_CPU_FREQ || CONFIG_IFX_PMCU */

static void power_on_deu_module(char *algo_name)
{
	unsigned long flag; 

	CRTCL_SECT_START;

    if (strcmp(algo_name, "aes") == 0) {
        powerup_deu(AES_INIT);
    }
    else if (strcmp(algo_name, "des") == 0) {
        powerup_deu(DES_INIT);
    }
    else if (strcmp(algo_name, "md5") == 0) {
        powerup_deu(MD5_INIT);
    }
    else if (strcmp(algo_name, "sha1") == 0) {
        powerup_deu(SHA1_INIT);
    }
    else if (strcmp(algo_name, "arc4") == 0) {
        powerup_deu(ARC4_INIT);
    }
    else if (strcmp(algo_name, "md5_hmac") == 0) {
        powerup_deu(MD5_HMAC_INIT);
    }
    else if (strcmp(algo_name, "sha1_hmac") == 0) {
        powerup_deu(SHA1_HMAC_INIT);
    }

	CRTCL_SECT_END;

    return;
}

static void power_off_deu_module(char *algo_name)
{
	unsigned long flag;

	CRTCL_SECT_START;

    if (strcmp(algo_name, "aes") == 0) {
	powerdown_deu(AES_INIT);
    }
    else if (strcmp(algo_name, "des") == 0) {
	powerdown_deu(DES_INIT);
    }
    else if (strcmp(algo_name, "md5") == 0) {
	powerdown_deu(MD5_INIT);
    }
    else if (strcmp(algo_name, "sha1") == 0) {
	powerdown_deu(SHA1_INIT); 
    }
    else if (strcmp(algo_name, "arc4") == 0) {
	powerdown_deu(ARC4_INIT);
    }
    else if (strcmp(algo_name, "md5_hmac") == 0) {
	powerdown_deu(MD5_HMAC_INIT);
    }
    else if (strcmp(algo_name, "sha1_hmac") == 0) {
	powerdown_deu(SHA1_HMAC_INIT);
    }
  
  	CRTCL_SECT_END; 

    return;
}

#if defined (CONFIG_CPU_FREQ) || defined (CONFIG_IFX_PMCU)
/*! \fn static IFX_PMCU_RETURN_t ifx_deu_pwrFeatureSwitch(IFX_PMCU_PWR_STATE_ENA_t pmcuPwrStateEna)
 *  \ingroup IFX_DEU_FUNCTIONS
 *  \brief switches the PMCU state to ON or OFF state
 *  \return IFX_PMCU_RETURN_SUCCESS
*/

static IFX_PMCU_RETURN_t ifx_deu_pwrFeatureSwitch(IFX_PMCU_PWR_STATE_ENA_t pmcuPwrStateEna)
{
    int i;
   
    /* 1. When there is a PMCU request to switch on, we need to power off the algos which are idling. 
     *    This is done by toggling the deu_power_flag. 
     * 2. When we are toggling to switch on/off, we must make sure that no ALGO is currenly running. 
     *    Due to the H/W design of DEU, we can only switch the whole DEU module on/off. Hence, we must
     *    make sure that all DEU algo are in idle state before switching it off.
     * 3. When a PMCU request to switch PMCU off, we just ignore all the flags and switch on all the 
     *	  DEU algo flags.  
     */

    if (pmcuPwrStateEna == IFX_PMCU_PWR_STATE_ON) {
        if (deu_power_status == 1)
            return IFX_PMCU_RETURN_SUCCESS;

        deu_power_status = 1;

        for (i = 0; i < MAX_DEU_ALGO; i++) {
            if (ifx_deu_algo[i].get_deu_algo_state() == CRYPTO_IDLE) {
                /* check power flag, if ON, switch off */
		        power_off_deu_module(ifx_deu_algo[i].name);
	        }
        }
    }
    else if (pmcuPwrStateEna == IFX_PMCU_PWR_STATE_OFF) {
        if (deu_power_status == 0)
	        return IFX_PMCU_RETURN_SUCCESS;

        for (i = 0; i < MAX_DEU_ALGO; i++) {
            if (ifx_deu_algo[i].get_deu_algo_state() == CRYPTO_IDLE) {
                /* if idle, switch on the algo */
		       power_on_deu_module(ifx_deu_algo[i].name);
            }
        }

        deu_power_status = 0;
    }
    else 
	    printk("Unknown power state feature command!\n");

    return IFX_PMCU_RETURN_SUCCESS;
}

/*! \fn void ifx_deu_pmcu_init (void)
 *  \ingroup IFX_DEU_FUNCTIONS
 *  \brief  PMCU initialization function
 *  \return void
*/

void ifx_deu_pmcu_init(void)
{
    IFX_PMCU_REGISTER_t pmcuRegister;

    memset(&pmcuRegister, 0, sizeof(pmcuRegister));
    pmcuRegister.pmcuModule = IFX_PMCU_MODULE_DEU;
    pmcuRegister.pmcuModuleNr = 0;
    pmcuRegister.pmcuModuleDep = NULL;
    pmcuRegister.pre = ifx_deu_preChange;
    pmcuRegister.post = ifx_deu_postChange;
    pmcuRegister.ifx_pmcu_state_change = ifx_deu_stateChange;
    pmcuRegister.ifx_pmcu_state_get = ifx_deu_stateGet;
    pmcuRegister.ifx_pmcu_pwr_feature_switch = ifx_deu_pwrFeatureSwitch;

    ifx_pmcu_register(&pmcuRegister);

    deu_power_status = 1;

}
#endif /* CONFIG_CPU_FREQ || CONFIG_IFX_PMCU */

/*! \fn static int __init deu_init (void)
 *  \ingroup IFX_DEU_FUNCTIONS
 *  \brief link all modules that have been selected in kernel config for ifx hw crypto support   
 *  \return ret 
*/  
                           
static int __init deu_init (void)
{
    int ret = -ENOSYS;

    deu_power_flag = 0;

#if defined (CONFIG_CPU_FREQ) || defined (CONFIG_IFX_PMCU)
    ifx_deu_pmcu_init();
#else
	/* if PMCU/CPU CLK gating is not enabled, we should enable the DEU
	 * drivers during init and init the different ciphers/hash. However,
	 * drivers normally will run under PMCU or CLK Gating scenario. This 
	 * is to satisfy build scenarios where PMCU & CLK gating drivers are
	 * not ready
	 */
	START_DEU_POWER;
	deu_power_flag = 0x0F;

	aes_chip_init();
	des_chip_init();
	SHA_HASH_INIT;
	md5_init_registers();

#endif  /* CONFIG_CPU_FREQ || CONFIG_IFX_PMCU */

	CRTCL_SECT_INIT;

    /* Initialize the dma priv members
     * have to do here otherwise the tests will fail
     * caused by exceptions. Only needed in DMA mode. 
     */
#ifdef CONFIG_CRYPTO_DEV_DMA
    deu_dma_priv_init();
	deu_dma_init();

	init_dma = 0;
#endif

#define IFX_DEU_DRV_VERSION         "2.0.0"
    printk(KERN_INFO "Infineon Technologies DEU driver version %s \n", IFX_DEU_DRV_VERSION);

    FIND_DEU_CHIP_VERSION;

#if defined(CONFIG_CRYPTO_DEV_DES)
    if ((ret = ifxdeu_init_des ())) {
        printk (KERN_ERR "IFX DES initialization failed!\n");
    }
#endif
#if defined(CONFIG_CRYPTO_DEV_AES)
    if ((ret = ifxdeu_init_aes ())) {
        printk (KERN_ERR "IFX AES initialization failed!\n");
    }

#endif
#if defined(CONFIG_CRYPTO_DEV_ARC4)
    if ((ret = ifxdeu_init_arc4 ())) {
        printk (KERN_ERR "IFX ARC4 initialization failed!\n");
    }

#endif
#if defined(CONFIG_CRYPTO_DEV_SHA1)
    if ((ret = ifxdeu_init_sha1 ())) {
        printk (KERN_ERR "IFX SHA1 initialization failed!\n");
    }
#endif
#if defined(CONFIG_CRYPTO_DEV_MD5)
    if ((ret = ifxdeu_init_md5 ())) {
        printk (KERN_ERR "IFX MD5 initialization failed!\n");
    }

#endif
#if defined(CONFIG_CRYPTO_DEV_SHA1_HMAC)
    if ((ret = ifxdeu_init_sha1_hmac ())) {
        printk (KERN_ERR "IFX SHA1_HMAC initialization failed!\n");
    }
#endif
#if defined(CONFIG_CRYPTO_DEV_MD5_HMAC)
    if ((ret = ifxdeu_init_md5_hmac ())) {
        printk (KERN_ERR "IFX MD5_HMAC initialization failed!\n");
    }
#endif

#if defined(CONFIG_CRYPTO_ASYNC_AES)
   if ((ret = lqdeu_async_aes_init())) {
        printk (KERN_ERR "Lantiq async AES initialization failed!\n");
    }
#endif

#if defined(CONFIG_CRYPTO_ASYNC_DES)
   if ((ret = lqdeu_async_des_init())) {
        printk (KERN_ERR "Lantiq async AES initialization failed!\n");
    }
#endif

    ifx_deu_proc_create();

    printk("DEU driver initialization complete!\n");

    return ret;

}

/*! \fn static void __exit deu_fini (void)
 *  \ingroup IFX_DEU_FUNCTIONS
 *  \brief remove the loaded crypto algorithms   
*/                                 
void __exit deu_fini (void)
{
#if defined(CONFIG_CRYPTO_DEV_DES)
    ifxdeu_fini_des ();
#endif
#if defined(CONFIG_CRYPTO_DEV_AES)
    ifxdeu_fini_aes ();
#endif
#if defined(CONFIG_CRYPTO_DEV_ARC4)
    ifxdeu_fini_arc4 ();
#endif
#if defined(CONFIG_CRYPTO_DEV_SHA1)
    ifxdeu_fini_sha1 ();
#endif
#if defined(CONFIG_CRYPTO_DEV_MD5)
    ifxdeu_fini_md5 ();
#endif
#if defined(CONFIG_CRYPTO_DEV_SHA1_HMAC)
    ifxdeu_fini_sha1_hmac ();
#endif
#if defined(CONFIG_CRYPTO_DEV_MD5_HMAC)
    ifxdeu_fini_md5_hmac ();
#endif
    printk("DEU has exited successfully\n");

#if defined(CONFIG_CRYPTO_DEV_DMA)
    ifxdeu_fini_dma();
    printk("DMA has deregistered successfully\n");
#endif

#if defined(CONFIG_CRYPTO_ASYNC_AES)
    lqdeu_fini_async_aes();
    printk("Async AES has deregistered successfully\n");
#endif
}

/*! \fn void powerup_deu(int crypto)
 *  \ingroup BOARD_SPECIFIC_FUNCTIONS
 *  \brief to power up the chosen cryptography module
 *  \param crypto AES_INIT, DES_INIT, etc. are the cryptographic algorithms chosen
 *  \return void
*/

void powerup_deu(int crypto)
{
#if defined (CONFIG_CPU_FREQ) || defined (CONFIG_IFX_PMCU)
    u32 temp;
    temp = 0;

#ifdef CONFIG_CRYPTO_DEV_DMA
    if (!init_dma) {
        init_dma = 1;
		START_DEU_POWER;
    }
#endif /* CONFIG_CRYPTO_DEV_DMA */

    temp = 1 << crypto;

    switch (crypto)  {
#if  defined(CONFIG_CRYPTO_DEV_AES) || defined(CONFIG_ASYNC_AES)
        case AES_INIT:
            if (!(deu_power_flag & temp)) {
                START_DEU_POWER;
                deu_power_flag |= temp;
            }
            aes_chip_init();
            break;
#endif
#if defined(CONFIG_CRYPTO_DEV_DES) || defined(CONFIG_ASYNC_DES)
        case DES_INIT:
            if (!(deu_power_flag & temp)) {
                START_DEU_POWER;
                deu_power_flag |= temp;
            }
            des_chip_init();
            break;
#endif
#ifdef CONFIG_CRYPTO_DEV_SHA1
        case SHA1_INIT:
            if (!(deu_power_flag & temp)) {
                START_DEU_POWER;
                deu_power_flag |= temp;
            }
	    	SHA_HASH_INIT;
            break;
#endif
#ifdef CONFIG_CRYPTO_DEV_MD5
        case MD5_INIT:
            if (!(deu_power_flag & temp)) {
                START_DEU_POWER;
                deu_power_flag |= temp;
            }
	    	md5_init_registers();
            break;
#endif
#ifdef CONFIG_CRYPTO_DEV_ARC4
        case ARC4_INIT:
            if (!(deu_power_flag & temp)) {
                START_DEU_POWER;
                deu_power_flag |= temp;
            }
            break;
                
#endif
#ifdef CONFIG_CRYPTO_DEV_MD5_HMAC
        case MD5_HMAC_INIT:  
            if (!(deu_power_flag & temp)) {
                START_DEU_POWER;
                deu_power_flag |= temp;
            }
            break;
#endif
#ifdef CONFIG_CRYPTO_DEV_SHA1_HMAC
       case SHA1_HMAC_INIT:
            if (!(deu_power_flag & temp)) {
                START_DEU_POWER;
                deu_power_flag |= temp;
            }
            break;
#endif
        default:
            printk(KERN_ERR "Error finding initialization crypto\n");
            break;
    }

#endif /* CONFIG_CPU_FREQ || CONFIG_IFX_PMCU */
	return;
}

/*! \fn void powerdown_deu(int crypto)
 *  \ingroup BOARD_SPECIFIC_FUNCTIONS
 *  \brief to power down the deu module totally if there are no other DEU encrypt/decrypt/hash
           processes still in progress. If there is, do not power down the module.
 *  \param crypto defines the cryptographic algo to module to be switched off
 *  \return void
*/

void powerdown_deu(int crypto)
{
#if defined (CONFIG_CPU_FREQ) || defined (CONFIG_IFX_PMCU)
    u32 temp;

    if (deu_power_status == 0 ) {
	   return;
    }

    temp = 0;
    
    temp = 1 << crypto;

    if (!(temp & deu_power_flag)) {
		/* Uncomment to debug, else leave it commented */
		/* printk(KERN_DEBUG "Module already switched off\n"); */
	    return;
    }

    if ((deu_power_flag &= ~temp)) { 
        /* printk(KERN_DEBUG "DEU power down not permitted. DEU modules still in use! \n"); */
        return;
    }
     
    STOP_DEU_POWER;    
    /* printk(KERN_DEBUG "DEU Module is turned OFF, deu_power_flag: %08x, algo: %d\n",
	 *		deu_power_flag, crypto);
	 */

#endif /* CONFIG_CPU_FREQ || CONFIG_IFX_PMCU */
	return;
}

module_init (deu_init);
module_exit (deu_fini);

MODULE_DESCRIPTION ("Infineon DEU crypto engine support.");
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Mohammad Firdaus");

