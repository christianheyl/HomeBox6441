/******************************************************************************
**
** FILE NAME    : ifxmips_wdt.c
** PROJECT      : IFX UEIP
** MODULES      : WDT
**
** DATE         : 12 Aug 2009
** AUTHOR       : Lee Yao Chye
** DESCRIPTION  : Watchdog Timer
** COPYRIGHT    :       Copyright (c) 2009
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
** HISTORY
** $Date        $Author         $Comment
** 12 Aug 2009  Lee Yao Chye    Initial UEIP version
** 19 Oct 2009  Lee Yao Chye    Add Linux style ioctls
*******************************************************************************/

/*!
  \file ifxmips_wdt.c
  \ingroup IFX_WDT
  \brief wdt driver file
*/

/*!
  \defgroup IFX_WDT_FUNCTIONS wdt driver functions
  \ingroup IFX_WDT
  \brief IFX wdt driver functions
*/

/*!
  \defgroup IFX_WDT_INTERNAL wdt linux-specific functions
  \ingroup IFX_WDT
  \brief IFX wdt linux-specific functions
*/


/*
 * ####################################
 *              Version No.
 * ####################################
 */
#define IFX_WDT_VER_MAJOR	1
#define IFX_WDT_VER_MID		0
#define IFX_WDT_VER_MINOR	5


/*
 * ####################################
 *              Head File
 * ####################################
 */

/*
 *  Common Head File
 */
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kallsyms.h>
#include <asm/delay.h>

/*
 *  Chip Specific Head File
 */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>

#if defined(MODVERSIONS)
#include <linux/modversions.h>
#endif

#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/tty.h>
#include <linux/selection.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/kdev_t.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include "ifxmips_wdt.h"
#include "ifx_wdt.h"

typedef struct wdt_dev{
      char name[16];
      int major;
      int minor;
      int full;
      char buff[10];
}wdt_dev;

#define IFX_WDT_EMSG(fmt, args...) printk( "%s: " fmt, __FUNCTION__ , ##args)
#define IFX_WDT_DMSG(fmt, args...) printk(" %s:" fmt, __FUNCTION__, ##args)


extern unsigned int ifx_get_fpi_hz(void);

static struct wdt_dev *ifx_wdt_dev;
static int occupied=0;

/** 
 * \fn static int wdt_enable(u32 timeout)
 
 * \brief Enable wdt module with provided timeout value.

 * \param   timeout    Number of seconds before watchdog timeout happens

 * \return  -EINVAL    timeout value too large
 *         0           OK
 *
 * \ingroup IFX_WDT_FUNCTIONS
 */
static int wdt_enable(u32 timeout)
{
    u32 wdt_cr=0;
    u32 wdt_reload=0;
    u32 wdt_clkdiv=0, clkdiv, wdt_pwl=0, pwl, ffpi;
    
    /* clock divider & prewarning limit */
    clkdiv = IFX_WDT_CR_CLKDIV_GET(*IFX_WDT_CR);
    switch(clkdiv)
    {
        case 0: wdt_clkdiv = 1; break;
        case 1: wdt_clkdiv = 64; break;
        case 2: wdt_clkdiv = 4096; break;
        case 3: wdt_clkdiv = 262144; break;
    }
    
    pwl = IFX_WDT_CR_PWL_GET(*IFX_WDT_CR);
    switch(pwl)
    {
        case 0: wdt_pwl = 0x8000; break;
        case 1: wdt_pwl = 0x4000; break;
        case 2: wdt_pwl = 0x2000; break;
        case 3: wdt_pwl = 0x1000; break;
    }

#ifdef CONFIG_USE_EMULATOR
    ffpi = 10000000; //1250000;
#else
    ffpi = ifx_get_fpi_hz();
#endif

    /* calculate reload value */
    wdt_reload = (timeout * (ffpi / wdt_clkdiv)) + wdt_pwl;

    if (wdt_reload > 0xFFFF)
    {
        IFX_WDT_EMSG("timeout too large %d\n", timeout);

        IFX_WDT_EMSG("wdt_pwl=0x%x, wdt_clkdiv=%d, ffpi=%d, wdt_reload = 0x%x\n", wdt_pwl, wdt_clkdiv, ffpi, wdt_reload);

        return -EINVAL;
    }
 
    /* Write first part of password access */
    *IFX_WDT_CR = IFX_WDT_CR_PW_SET(IFX_WDT_PW1);

    wdt_cr = *IFX_WDT_CR;
    wdt_cr &= ( !IFX_WDT_CR_PW_SET(0xff) &
                !IFX_WDT_CR_PWL_SET(0x3) &
                !IFX_WDT_CR_CLKDIV_SET(0x3) &
                !IFX_WDT_CR_RELOAD_SET(0xffff));

    wdt_cr |= ( IFX_WDT_CR_PW_SET(IFX_WDT_PW2) |
                IFX_WDT_CR_PWL_SET(pwl) |
                IFX_WDT_CR_CLKDIV_SET(clkdiv) |
                IFX_WDT_CR_RELOAD_SET(wdt_reload) |
                IFX_WDT_CR_GEN);

    /* Set reload value in second password access */
    *IFX_WDT_CR = wdt_cr;

    return 0 ;
}

/** 
 * \fn static int wdt_disable(void)
 
 * \brief Disable wdt module

 * \return  none
 
 * \ingroup IFX_WDT_FUNCTIONS
 */
static void wdt_disable(void)
{
    /* Write first part of password access */
    *IFX_WDT_CR = IFX_WDT_CR_PW_SET(IFX_WDT_PW1);

    /* Disable the watchdog in second password access (GEN=0) */
    *IFX_WDT_CR = IFX_WDT_CR_PW_SET(IFX_WDT_PW2);
}

/** 
 * \fn static void wdt_low_power(int en)
 
 * \brief Low power clock freeze mode.

 * \param   en 1 for enable   

 * \return none
 *
 * \ingroup IFX_WDT_FUNCTIONS
 */
static void wdt_low_power(int en)
{
    u32 wdt_cr=0;
    
    *IFX_WDT_CR = IFX_WDT_CR_PW_SET(IFX_WDT_PW1);

    wdt_cr = *IFX_WDT_CR;

    if(en)
    {
        wdt_cr &= (!IFX_WDT_CR_PW_SET(0xff));
        wdt_cr |= (IFX_WDT_CR_PW_SET(IFX_WDT_PW2) | IFX_WDT_CR_LPEN);
    }
    else
    {
        wdt_cr &= (!IFX_WDT_CR_PW_SET(0xff) &
                   !IFX_WDT_CR_LPEN);
        wdt_cr |= IFX_WDT_CR_PW_SET(IFX_WDT_PW2);
    }               

    /* Set reload value in second password access */
    *IFX_WDT_CR = wdt_cr;
}

/** 
 * \fn static void wdt_debug_suspend(int en)
 
 * \brief Debug suspend mode.

 * \param   en 1 for enable   

 * \return none
 *
 * \ingroup IFX_WDT_FUNCTIONS
 */
static void wdt_debug_suspend(int en)
{
    u32 wdt_cr=0;

    *IFX_WDT_CR = IFX_WDT_CR_PW_SET(IFX_WDT_PW1);

    wdt_cr = *IFX_WDT_CR;
    if(en)
    {
        wdt_cr &= (!IFX_WDT_CR_PW_SET(0xff));
        wdt_cr |= (IFX_WDT_CR_PW_SET(IFX_WDT_PW2) | IFX_WDT_CR_DSEN);
    }
    else
    {
        wdt_cr &= (!IFX_WDT_CR_PW_SET(0xff) &
                   !IFX_WDT_CR_DSEN);
        wdt_cr |= IFX_WDT_CR_PW_SET(IFX_WDT_PW2);
    }

    /* Set reload value in second password access */
    *IFX_WDT_CR = wdt_cr; 
}

/** 
 * \fn static void wdt_prewarning_limit(int pwl)
 
 * \brief Configure the prewarning level for WDT

 * \param   pwl    0 for 1/2 of the max WDT period
 *                 1 for 1/4 of the max WDT period
 *                 2 for 1/8 of the max WDT period
 *                 3 for 1/16 of the max WDT period
 
 * \return  none
 *
 * \ingroup IFX_WDT_FUNCTIONS
 */
static void wdt_prewarning_limit(int pwl)
{
    u32 wdt_cr=0;
    
    wdt_cr = *IFX_WDT_CR;
    *IFX_WDT_CR = IFX_WDT_CR_PW_SET(IFX_WDT_PW1);

    wdt_cr &= 0xff00ffff;//(!IFX_WDT_CR_PW_SET(0xff));
    wdt_cr &= 0xf3ffffff;//(!IFX_WDT_CR_PWL_SET(3));
    wdt_cr |= (IFX_WDT_CR_PW_SET(IFX_WDT_PW2) | 
               IFX_WDT_CR_PWL_SET(pwl));

    /* Set reload value in second password access */
    *IFX_WDT_CR = wdt_cr;
}

/** 
 * \fn static void wdt_set_clkdiv(int clkdiv)
 
 * \brief Configure the clock divider for WDT

 * \param   clkdiv    0 for CLK_WDT = 1 x CLK_TIMER
 *                    1 for CLK_WDT = 64 x CLK_TIMER
 *                    2 for CLK_WDT = 4096 x CLK_TIMER
 *                    3 for CLK_WDT = 262144 x CLK_TIMER
 
 * \return  none
 *
 * \ingroup IFX_WDT_FUNCTIONS
 */
static void wdt_set_clkdiv(int clkdiv)
{
    u32 wdt_cr=0;

    wdt_cr = *IFX_WDT_CR;
    *IFX_WDT_CR = IFX_WDT_CR_PW_SET(IFX_WDT_PW1);

    wdt_cr &= 0xff00ffff; //(!IFX_WDT_CR_PW_SET(0xff));
    wdt_cr &= 0xfcffffff; //(!IFX_WDT_CR_CLKDIV_SET(3));
    wdt_cr |= (IFX_WDT_CR_PW_SET(IFX_WDT_PW2) |
               IFX_WDT_CR_CLKDIV_SET(clkdiv));

    /* Set reload value in second password access */
    *IFX_WDT_CR = wdt_cr;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
static int wdt_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
static int wdt_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
    int result=0;
    int en=0;
    int istatus;
    int pwl, clkdiv;
    static int timeout = -1; // static variable remains last called value
    u32 timeval = 0;
    u32 wdt_clkdiv = 0, wdt_pwl, ffpi, wdios_option;
    u32 timeleft;
    u32 temp;
                            
    switch(cmd){
	case WDIOC_SETTIMEOUT:
	    // set these to allow longer timeout
	    wdt_set_clkdiv(3);
	    wdt_prewarning_limit(3);

            // proceed with IFX_WDT_IOC_START

        case IFX_WDT_IOC_START:
            IFX_WDT_DMSG("enable watch dog timer!\n");
            if ( copy_from_user((void*)&timeout, (void*)arg, sizeof (int)) )
            {
                IFX_WDT_EMSG("invalid argument\n");
                result=-EINVAL;
            }
            else
            {
                if((result = wdt_enable(timeout)) < 0)
                {
                    timeout = -1;
                }
            }
            break;

        case IFX_WDT_IOC_STOP:
            IFX_WDT_DMSG("disable watch dog timer\n");
            timeout = -1;
            wdt_disable();
            break;

        case IFX_WDT_IOC_PING:
	case WDIOC_KEEPALIVE:
            if(timeout < 0)
            {
                result = -EIO;
            }
	    else
	    {
                result = wdt_enable(timeout);
            }
            break;

        case IFX_WDT_IOC_SET_PWL:
	case WDIOC_SETPRETIMEOUT:
            if ( copy_from_user((void*)&pwl, (void*)arg, sizeof (int)) )
            {
                IFX_WDT_EMSG("invalid argument\n");
                result=-EINVAL;
            }                       
            wdt_prewarning_limit(pwl);
            break;

        case IFX_WDT_IOC_SET_DSEN:
            if ( copy_from_user((void*)&en, (void*)arg, sizeof (int)) )
            {
                IFX_WDT_EMSG("invalid argument\n");
                result=-EINVAL;
            }                              
            wdt_debug_suspend(en);
            break;

        case IFX_WDT_IOC_SET_LPEN:
            if ( copy_from_user((void*)&en, (void*)arg, sizeof (int)) )
            {
                IFX_WDT_EMSG("invalid argument\n");
                result=-EINVAL;
            }                              
            wdt_low_power(en);
            break;

        case IFX_WDT_IOC_SET_CLKDIV:
            if ( copy_from_user((void*)&clkdiv, (void*)arg, sizeof (int)) )
            {
                IFX_WDT_EMSG("invalid argument\n");
                result=-EINVAL;
            }
            wdt_set_clkdiv(clkdiv);
            break;
            
        case IFX_WDT_IOC_GET_STATUS:
	case WDIOC_GETSTATUS:
	case WDIOC_GETBOOTSTATUS:
            istatus = *IFX_WDT_SR;     
            copy_to_user((int *)arg, (int *)&istatus, sizeof(int));
            break;

        case WDIOC_GETTIMEOUT:
            copy_to_user((int *)arg, (int *)&timeout, sizeof(int));	    
            break;

	case WDIOC_GETTIMELEFT:
            ffpi = ifx_get_fpi_hz();

            pwl = IFX_WDT_CR_PWL_GET(*IFX_WDT_CR);
            switch(pwl)
            {
                case 0: wdt_pwl = 0x8000; break;
                case 1: wdt_pwl = 0x4000; break;
                case 2: wdt_pwl = 0x2000; break;
                case 3: wdt_pwl = 0x1000; break;
            }

            clkdiv = IFX_WDT_CR_CLKDIV_GET(*IFX_WDT_CR);
            switch(clkdiv)
            {
                case 0: wdt_clkdiv = 1; break;
                case 1: wdt_clkdiv = 64; break;
                case 2: wdt_clkdiv = 4096; break;
                case 3: wdt_clkdiv = 262144; break;
            }

            timeval = *IFX_WDT_SR;
	    timeval &= IFX_WDT_READ_TIMER_VAL_MASK;     

            temp = ffpi / wdt_clkdiv;
	    timeval = timeval - wdt_pwl;
	    
            timeleft = timeval / temp;

            copy_to_user((int *)arg, (int *)&timeleft, sizeof(int));	    
            break;

	case WDIOC_GETPRETIMEOUT:  
            pwl = IFX_WDT_CR_PWL_GET(*IFX_WDT_CR);
            switch(pwl)
            {
                case 0: wdt_pwl = 0x8000; break;
                case 1: wdt_pwl = 0x4000; break;
                case 2: wdt_pwl = 0x2000; break;
                case 3: wdt_pwl = 0x1000; break;
            }

            copy_to_user((int *)arg, (int *)&wdt_pwl, sizeof(int));	    
            break;

        case WDIOC_SETOPTIONS:
            copy_from_user((void*)&wdios_option, (void*)arg, sizeof (int));

            if (wdios_option == WDIOS_ENABLECARD)
            {
                wdt_enable(timeout);            
            }
            else if (wdios_option == WDIOS_DISABLECARD)
            {           
                wdt_disable();
            }

            break;

      } // end of switch

      return result;
}

static int wdt_open(struct inode *inode, struct file *file)
{
    if (occupied == 1) return -EBUSY;
    occupied = 1;
    
    return 0;
}

static int wdt_release(struct inode *inode, struct file *file)
{
    occupied = 0;

    return 0;
}

/**
 * \fn static inline int ifx_wdt_drv_ver(char *buf)
 * \brief Display WDT driver version
 * 
 * \return number of bytes will be printed
 * \ingroup IFX_WDT_INTERNAL 
 */
static inline int 
ifx_wdt_drv_ver(char *buf)
{
    return ifx_drv_ver(buf, "WDT", IFX_WDT_VER_MAJOR, IFX_WDT_VER_MID, IFX_WDT_VER_MINOR);
}


/**
 * \fn static int wdt_register_proc_read(char *buf, char **start, off_t offset, int count, int *eof, void *data)
 * \brief Display information via proc file
 * 
 * \return number of bytes will be printed
 * \ingroup IFX_WDT_INTERNAL 
 */
static int wdt_register_proc_read(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
    int len=0;
    len+=sprintf(buf+len,"IFX_WDT_PROC_READ\n");
    
    len+=sprintf(buf+len,"IFX_WDT_CR(0x%08x)      : 0x%08x\n", (unsigned int)IFX_WDT_CR, *IFX_WDT_CR);  
    len+=sprintf(buf+len,"IFX_WDT_SR(0x%08x)      : 0x%08x\n", (unsigned int)IFX_WDT_SR, *IFX_WDT_SR);

    *eof = 1;
    return len;
}

static struct file_operations wdt_fops =
{
    owner:          THIS_MODULE,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
    unlocked_ioctl: wdt_ioctl,
#else
    ioctl:   	    wdt_ioctl,
#endif
    open:           wdt_open,
    release:        wdt_release,
};

/**
 * \fn static int __init ifx_wdt_init_module(void)
 * \brief Initialize WDT module
 * 
 * \return -ENOMEM  Failed to allocate memory
 *         -EINVAL  Failed to register char device
 *         0        OK
 * \ingroup IFX_WDT_INTERNAL 
 */
static int __init ifx_wdt_init_module(void)
{
    int result =0; 
    char ver_str[128] = {0};

    ifx_wdt_dev = (wdt_dev*)kmalloc(sizeof(wdt_dev),GFP_KERNEL);

    ifx_wdt_drv_ver(ver_str);
    
    if (ifx_wdt_dev == NULL){
            return -ENOMEM;
    }
    memset(ifx_wdt_dev,0,sizeof(wdt_dev));
    
    strcpy(ifx_wdt_dev->name, DEVICE_NAME);
    
    result = register_chrdev(0,ifx_wdt_dev->name,&wdt_fops);

    if (result < 0) {
  
            IFX_WDT_EMSG("cannot register device\n");
            kfree(ifx_wdt_dev);
            return -EINVAL;                         
    }
     
    ifx_wdt_dev->major = result; 

    /* Create proc file */
    create_proc_read_entry("ifx_wdt", 0, NULL, wdt_register_proc_read , NULL);
    return 0;
}

/**
 * \fn static void ifx_wdt_cleanup_module(void)
 * \brief WDT Module Cleanup.
 *
 * Upon removal of the WDT module this function will free all allocated 
 * resources and unregister devices. 
 * \return none
 * \ingroup IFX_WDT_INTERNAL
 */
static void ifx_wdt_cleanup_module(void)
{
    unregister_chrdev(ifx_wdt_dev->major,ifx_wdt_dev->name);
    remove_proc_entry("ifx_wdt", NULL);
    kfree(ifx_wdt_dev); 
    return;
}


module_init(ifx_wdt_init_module);
module_exit(ifx_wdt_cleanup_module);


