/******************************************************************************
**
** FILE NAME    : ifxmips_ts_vr9.c
** PROJECT      : UEIP
** MODULES      : Thermal Sensor
**
** DATE         : 16 Aug 2011
** AUTHOR       : Xu Liang
** DESCRIPTION  : Thermal Sensor driver VR9 source file
** COPYRIGHT    :       Copyright (c) 2006
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date          $Author         $Comment
** Aug 16, 2011   Xu Liang        Init Version
*******************************************************************************/



/*
 * ####################################
 *              Version No.
 * ####################################
 */

#define VER_MAJOR                   1
#define VER_MID                     0
#define VER_MINOR                   3



/*
 * ####################################
 *              Head File
 * ####################################
 */

/*
 *  Common Head File
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>

/*
 *  Chip Specific Head File
 */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>
#include "ifxmips_ts.h"



/*
 * ####################################
 *              Definition
 * ####################################
 */

#define IFX_TS_REG                  KSEG1ADDR(0x1F103040)
#define IFX_TS_INT                  INT_NUM_IM3_IRL19



/*
 * ####################################
 *             Declaration
 * ####################################
 */

/*
 *  File Operations
 */
static int ts_open(struct inode *, struct file *);
static int ts_release(struct inode *, struct file *);
static int ts_ioctl(struct inode *, struct file *, unsigned int, unsigned long);

/*
 *  Interrupt Handler
 */
static irqreturn_t ts_irq_handler(int, void *);

/*
 *  Proc File Functions
 */
static INLINE void proc_file_create(void);
static INLINE void proc_file_delete(void);
static int proc_read_version(char *, char **, off_t, int, int *, void *);
static int proc_read_temp(char *, char **, off_t, int, int *, void *);

/*
 *  Init Help Functions
 */
static INLINE int ifx_ts_version(char *);
static INLINE int print_temp(char *);



/*
 * ####################################
 *            Local Variable
 * ####################################
 */

//static int g_temp_read_enable = 0;

static int g_ts_major;
static struct file_operations g_ts_fops = {
    .owner      = THIS_MODULE,
    .open       = ts_open,
    .release    = ts_release,
    .ioctl      = ts_ioctl,
};

static unsigned int g_dbg_enable = DBG_ENABLE_MASK_ERR;

static struct proc_dir_entry* g_proc_dir = NULL;



/*
 * ####################################
 *            Local Function
 * ####################################
 */

static int ts_open(struct inode *inode, struct file *filep)
{
    return 0;
}

static int ts_release(struct inode *inode, struct file *filep)
{
    return 0;
}

static int ts_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int retval = 0;

    printk(KERN_ERR "IOCTL: Undefined IOCTL call!\n");
    retval = -EACCES;

    return retval;
}

static irqreturn_t ts_irq_handler(int irq, void *dev_id)
{
    char str[16];

    print_temp(str);
    err("Chip is over heated (%s)!", str);
    return IRQ_HANDLED;
}

static INLINE void proc_file_create(void)
{
    g_proc_dir = proc_mkdir("driver/ifx_ts", NULL);

    create_proc_read_entry("version",
                            0,
                            g_proc_dir,
                            proc_read_version,
                            NULL);

    create_proc_read_entry("temp",
                            0,
                            g_proc_dir,
                            proc_read_temp,
                            NULL);
}

static INLINE void proc_file_delete(void)
{
    remove_proc_entry("temp", g_proc_dir);
    remove_proc_entry("version", g_proc_dir);
    remove_proc_entry("driver/ifx_ts", NULL);
}

static int proc_read_version(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;

    len += ifx_ts_version(buf + len);
    len += sprintf(buf + len, "build: %s %s\n", __DATE__, __TIME__);
    len += sprintf(buf + len, "major.minor: %d.0\n", g_ts_major);

    *eof = 1;

    return len - off;
}

static int proc_read_temp(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = print_temp(buf);

    *eof = 1;

    return len - off;
}

static INLINE int ifx_ts_version(char *buf)
{
    return ifx_drv_ver(buf, "Thermal Sensor", VER_MAJOR, VER_MID, VER_MINOR);
}

static INLINE int print_temp(char *buf)
{
    int len = 0;
    int temp_value = 0;
    int fraction;

    if ( ifx_ts_get_temp(&temp_value) == 0 ) {
        if ( temp_value < 0 ) {
            len += sprintf(buf + len, "=");
            temp_value = -temp_value;
        }
        len += sprintf(buf + len, "%d", temp_value / 10);
        if ( (fraction = temp_value % 10) != 0 )
            len += sprintf(buf + len, ".%d", fraction);
        len += sprintf(buf + len, "\n");
    }
    else
        len += sprintf(buf + len, "Failed in reading temperature info!\n");

    return len;
}



/*
 * ####################################
 *           Global Function
 * ####################################
 */

int ifx_ts_get_temp(int *p_temp)
{
    unsigned int temp_value;

    if ( p_temp == NULL )
        return -EINVAL;

//    if ( g_temp_read_enable == 0 )
//        return -EIO;

    //  value x 5 = (Celsius Degree + 38) x 10
    temp_value = (IFX_REG_R32(IFX_TS_REG) >> 9) & 0x01FF;
    *p_temp = (int)((temp_value << 2) + temp_value);
    *p_temp -= 380;

    return 0;
}
EXPORT_SYMBOL(ifx_ts_get_temp);


/*
 * ####################################
 *           Init/Cleanup API
 * ####################################
 */

static int __devinit ts_init(void)
{
    int ret;
    ifx_chipid_t chipid = {0};
    char ver_str[256];

    ifx_get_chipid(&chipid);
    if ( chipid.family_id != IFX_FAMILY_xRX200
        || chipid.family_ver != IFX_FAMILY_xRX200_A2x ) {
        err("Only xRX200 A2x chip is supported, family_id = %d, family_ver = %d\n",
            chipid.family_id, chipid.family_ver);
        return -EIO;
    }

    ret = register_chrdev(IFX_TS_MAJOR, "ifx_ts", &g_ts_fops);
#if IFX_TS_MAJOR == 0
    g_ts_major = ret;
#else
    g_ts_major = IFX_TS_MAJOR;
#endif
    if ( ret < 0 ) {
        err("Can not register thermal sensor device - %d", ret);
        return ret;
    }

    IFX_REG_W32_MASK(0, 0x00080000, IFX_TS_REG);    //  turn on Thermal Sensor

    ret = request_irq(IFX_TS_INT, ts_irq_handler, IRQF_DISABLED, "ts_isr", NULL);
    if ( ret ) {
        err("Can not get IRQ - %d", ret);
        unregister_chrdev(g_ts_major, "ifx_ts");
        return ret;
    }

    proc_file_create();

    ifx_ts_version(ver_str);
    printk(KERN_INFO "%s", ver_str);

    return 0;
}


static void __exit ts_exit(void)
{
    proc_file_delete();

    free_irq(IFX_TS_INT, NULL);

    IFX_REG_W32_MASK(0x00080000, 0, IFX_TS_REG);    //  turn off Thermal Sensor

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
    if ( unregister_chrdev(g_ts_major, "ifx_ts") ) {
        err("Can not unregister TS device (major %d)!", g_ts_major);
    }
#else
    unregister_chrdev(g_ts_major, "ifx_ts");
#endif
}

module_init(ts_init);
module_exit(ts_exit);

MODULE_AUTHOR("Xu Liang");
MODULE_LICENSE("GPL");
