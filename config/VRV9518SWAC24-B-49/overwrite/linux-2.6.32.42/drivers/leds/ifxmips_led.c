/******************************************************************************
**
** FILE NAME    : ifxmips_led.c
** PROJECT      : UEIP
** MODULES      : LED Driver
**
** DATE         : 16 Jul 2009
** AUTHOR       : Xu Liang
** DESCRIPTION  : LED driver common source file
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
** $Date        $Author         $Comment
** 16 JUL 2009  Xu Liang        Init Version
*******************************************************************************/



/*
 * ####################################
 *              Version No.
 * ####################################
 */

#define IFX_LED_VER_MAJOR               1
#define IFX_LED_VER_MID                 1
#define IFX_LED_VER_MINOR               4



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
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kallsyms.h>
#include <linux/timer.h>
#include <linux/device.h>
#include <linux/ctype.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <asm/delay.h>

/*
 *  Chip Specific Head File
 */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>
#include <asm/ifx/ifx_pmu.h>
#include <asm/ifx/ifx_gpio.h>
#include <asm/ifx/ifx_ledc.h>
#include <asm/ifx/ifx_ebu_led.h>
#include "leds.h"
#include "ifxmips_led.h"



/*
 * ####################################
 *        Kernel Version Adaption
 * ####################################
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
  #define IFX_LED_CLASS_DEVICE_ATTR         CLASS_DEVICE_ATTR
  #define ifx_led_class_device              class_device
  #define CLASS_DEVICE_ATTR_FUNC_PARAM
  #define ifx_led_class_get_devdata         class_get_devdata
#else
  #define IFX_LED_CLASS_DEVICE_ATTR         DEVICE_ATTR
  #define ifx_led_class_device              device
  #define CLASS_DEVICE_ATTR_FUNC_PARAM      struct device_attribute *attr,
  #define ifx_led_class_get_devdata         dev_get_drvdata
#endif



/*
 * ####################################
 *              Definition
 * ####################################
 */

#define DEFAULT_DELAY_ON        50
#define DEFAULT_DELAY_OFF       50
#define DEFAULT_TIMEOUT         500
#define DEFAULT_VALUE           0

/* Terry 20140608, for o2 HB2 specification, Power Led blink 1Hz in boot-up sequence */
#define BLINK_POWER_LED			1

/* Terry 20141021, implement all LEDs blink */
#ifndef IFX_LED
#define IFX_LED                       (KSEG1 | 0x1E100BB0)
#define IFX_LED_CON0                  ((volatile unsigned int *)(IFX_LED + 0x0000))
#define IFX_LED_CON1                  ((volatile unsigned int *)(IFX_LED + 0x0004))
#define IFX_LED_CPU0                  ((volatile unsigned int *)(IFX_LED + 0x0008))
#define IFX_LED_CPU1                  ((volatile unsigned int *)(IFX_LED + 0x000C))
#define IFX_LED_AR                    ((volatile unsigned int *)(IFX_LED + 0x0010))
#endif
/*
 * ####################################
 *             Declaration
 * ####################################
 */

/*
 *  File Operations
 */
static int ifx_led_open(struct inode *, struct file *);
static int ifx_led_release(struct inode *, struct file *);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
static int ifx_led_ioctl(struct file *, unsigned int, unsigned long);
#else
static int ifx_led_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
#endif

/*
 *  LED Device Functions
 */
static void gpio_brightness_set(struct led_classdev *, enum led_brightness);
static void ledc_brightness_set(struct led_classdev *, enum led_brightness);
static int led_device_create(struct ifx_led_device *);
static void led_device_destroy(struct ifx_led_device *);

/*
 *  LED Trigger Functions
 */
static INLINE int led_trigger_create(const char *, struct led_trigger **);
static INLINE void led_trigger_destroy(struct led_trigger *);
static void led_trig_activate(struct led_classdev *);
static void led_trig_deactivate(struct led_classdev *);
static ssize_t led_delay_on_show(struct ifx_led_class_device *, CLASS_DEVICE_ATTR_FUNC_PARAM char *);
static ssize_t led_delay_on_store(struct ifx_led_class_device *, CLASS_DEVICE_ATTR_FUNC_PARAM const char *, size_t);
static ssize_t led_delay_off_show(struct ifx_led_class_device *, CLASS_DEVICE_ATTR_FUNC_PARAM char *);
static ssize_t led_delay_off_store(struct ifx_led_class_device *, CLASS_DEVICE_ATTR_FUNC_PARAM const char *, size_t);
static ssize_t led_delay_timeout_show(struct ifx_led_class_device *, CLASS_DEVICE_ATTR_FUNC_PARAM char *);
static ssize_t led_delay_timeout_store(struct ifx_led_class_device *, CLASS_DEVICE_ATTR_FUNC_PARAM const char *, size_t);
static ssize_t led_color_show(struct ifx_led_class_device *, CLASS_DEVICE_ATTR_FUNC_PARAM char *);
static ssize_t led_color_store(struct ifx_led_class_device *, CLASS_DEVICE_ATTR_FUNC_PARAM const char *, size_t);
static void led_blink_timer_function(unsigned long);
static void led_timeout_timer_function(unsigned long);

/*
 *  Init Functions
 */
static INLINE int gpio_register(void);
static INLINE void gpio_deregister(void);

/*
 *  Proc File Functions
 */
static INLINE void proc_file_create(void);
static INLINE void proc_file_delete(void);
static int proc_read_version(char *, char **, off_t, int, int *, void *);
static int proc_write_all_led_blink(struct file *file, const char *buf, int count, void *data);
/*
 *  Proc Help Functions
 */

/*
 *  Init Help Functions
 */
static INLINE int ifx_led_version(char *);
/* Terry 20150317, need to hack for new/old boards. */
#if 1
extern int is_old_board(void);
#endif


/*
 *  External Variable
 */
extern struct ifx_led_device g_board_led_hw_config[];



/*
 * ####################################
 *            Local Variable
 * ####################################
 */

spinlock_t g_led_low_level_operation_lock;  //  use spinlock rather than semaphore or mutex
                                            //  because most functions run in user context
                                            //  and they do not take much time to finish operation

static int g_gpio_registered = 0;
static int g_ebu_enabled = 0;

static DEFINE_RWLOCK(g_trigger_list_lock);
static LIST_HEAD(g_trigger_list);

static IFX_LED_CLASS_DEVICE_ATTR(delay_on, 0644, led_delay_on_show, led_delay_on_store);
static IFX_LED_CLASS_DEVICE_ATTR(delay_off, 0644, led_delay_off_show, led_delay_off_store);
static IFX_LED_CLASS_DEVICE_ATTR(timeout, 0644, led_delay_timeout_show, led_delay_timeout_store);
static IFX_LED_CLASS_DEVICE_ATTR(color, 0644, led_color_show, led_color_store);

static struct file_operations g_led_fops = {
    .open    = ifx_led_open,
    .release = ifx_led_release,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
    .unlocked_ioctl = ifx_led_ioctl
#else
    .ioctl          = ifx_led_ioctl
#endif
};

static int g_major;

static unsigned int g_dbg_enable = DBG_ENABLE_MASK_ERR | DBG_ENABLE_MASK_ASSERT | DBG_ENABLE_MASK_DEBUG_PRINT;

static struct proc_dir_entry* g_led_dir = NULL;

#if defined(BLINK_POWER_LED) && BLINK_POWER_LED
static void *g_power_led_trigger = NULL;
#endif

/* Terry 20141021, implement all LEDs blink */
#if 1
static int g_all_led_blinking;
struct task_struct *all_led_blink_thread_id;
#endif
/*
 * ####################################
 *            Local Function
 * ####################################
 */

static int ifx_led_open(struct inode *inode, struct file *filep)
{
    return IFX_SUCCESS;
}

static int ifx_led_release(struct inode *inode, struct file *filelp)
{
    return IFX_SUCCESS;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
static int ifx_led_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int ifx_led_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
    int ret;

    //  check magic number
    if ( _IOC_TYPE(cmd) != IFX_LED_IOC_MAGIC )
        return -EINVAL;

    //  check read/write right
    if ( ((_IOC_DIR(cmd) & _IOC_WRITE) && !access_ok(VERIFY_READ, arg, _IOC_SIZE(cmd)))
        || ((_IOC_DIR (cmd) & _IOC_READ) && !access_ok(VERIFY_WRITE, arg, _IOC_SIZE(cmd))) )
        return -EACCES;

    switch ( cmd ) {
    case IFX_LED_IOC_VERSION:
        {
            struct ifx_led_ioctl_version version = {
                .major = IFX_LED_VER_MAJOR,
                .mid   = IFX_LED_VER_MID,
                .minor = IFX_LED_VER_MINOR
            };
            ret = copy_to_user((void *)arg, (void *)&version, sizeof(version));
        }
        break;
    case IFX_LED_IOC_TRIGGER_REGISTER:
    case IFX_LED_IOC_TRIGGER_DEREGISTER:
    case IFX_LED_IOC_TRIGGER_ACTIVATE:
    case IFX_LED_IOC_TRIGGER_DEACTIVATE:
    case IFX_LED_IOC_TRIGGER_SET_ATTRIB:
        {
            struct ifx_led_ioctl_trigger trigger;

            ret = copy_from_user((void *)&trigger, (void *)arg, sizeof(trigger));
            if ( ret != IFX_SUCCESS )
                break;

            switch ( cmd ) {
            case IFX_LED_IOC_TRIGGER_REGISTER:
                ret = ifx_led_trigger_register(trigger.name, &trigger.handler);
                if ( ret == IFX_SUCCESS )
                    ret = copy_to_user((void *)arg, (void *)&trigger, sizeof(trigger));
                break;
            case IFX_LED_IOC_TRIGGER_DEREGISTER:
                ifx_led_trigger_deregister(trigger.handler);
                ret = IFX_SUCCESS;
                break;
            case IFX_LED_IOC_TRIGGER_ACTIVATE:
                ifx_led_trigger_activate(trigger.handler);
                ret = IFX_SUCCESS;
                break;
            case IFX_LED_IOC_TRIGGER_DEACTIVATE:
                ifx_led_trigger_deactivate(trigger.handler);
                ret = IFX_SUCCESS;
                break;
            case IFX_LED_IOC_TRIGGER_SET_ATTRIB:
                ifx_led_trigger_set_attrib(trigger.handler, &trigger.attrib);
                ret = IFX_SUCCESS;
                break;
            }
        }
        break;
    default:
        ret = -EOPNOTSUPP;
    }

    return ret;
}

#if defined(CONFIG_IFX_GPIO) || defined(CONFIG_IFX_GPIO_MODULE) || defined(CONFIG_IFX_LEDC) || defined(CONFIG_IFX_LEDC_MODULE)
static inline void led_dual_calc_color(struct ifx_led_device *device_def,
                                     enum led_brightness value,
                                     unsigned int *ledc_value_led1,
                                     unsigned int *ledc_value_led2)
{
    switch ((value - LED_OFF) & 3) {
        case 0:			/* LED_OFF -> LED1 off, LED2 off */
            *ledc_value_led1 = device_def->value_off;
            *ledc_value_led2 = device_def->value_off;
	    break;
        case 1:			/* LED_OFF+1 -> LED1 on, LED2 off */
            *ledc_value_led1 = device_def->value_on;
            *ledc_value_led2 = device_def->value_off;
	    break;
        case 2:			/* LED_OFF+2 -> LED1 off, LED2 on */
            *ledc_value_led1 = device_def->value_off;
            *ledc_value_led2 = device_def->value_on;
	    break;
        case 3:			/* LED_OFF+3 -> LED1 on, LED2 on */
            *ledc_value_led1 = device_def->value_on;
            *ledc_value_led2 = device_def->value_on;
	    break;
    }
}
#endif

static void gpio_brightness_set(struct led_classdev *pled, enum led_brightness value)
{
#if defined(CONFIG_IFX_GPIO) || defined(CONFIG_IFX_GPIO_MODULE)
    struct ifx_led_device *device_def = (struct ifx_led_device *)pled;

	/* Terry 20141021, implement all LEDs blink */
#if 1
	device_def->last_led_brightness = value;
	if (g_all_led_blinking != 0)
		return;
#endif

    if (device_def->flags & IFX_LED_DEVICE_FLAG_DUAL_LED)
    {
        unsigned int gpio_value_led1 = 0, gpio_value_led2 = 0;
        unsigned phys_id_led1, phys_id_led2;

        if (value == LED_FULL || value == LED_HALF)
            led_dual_calc_color(device_def, (enum led_brightness)device_def->color, &gpio_value_led1, &gpio_value_led2);
        else
            led_dual_calc_color(device_def, value, &gpio_value_led1, &gpio_value_led2);

        phys_id_led1 = IFX_LED_DUAL_GET_PHYS_ID_LED1(device_def->phys_id);
        phys_id_led2 = IFX_LED_DUAL_GET_PHYS_ID_LED2(device_def->phys_id);
        if ( gpio_value_led1 )
            ifx_gpio_output_set(phys_id_led1, IFX_GPIO_MODULE_LED);
        else
            ifx_gpio_output_clear(phys_id_led1, IFX_GPIO_MODULE_LED);

        if ( gpio_value_led2 )
            ifx_gpio_output_set(phys_id_led2, IFX_GPIO_MODULE_LED);
        else
            ifx_gpio_output_clear(phys_id_led2, IFX_GPIO_MODULE_LED);
    }
    else
    {
        unsigned int gpio_value = value == LED_OFF ? device_def->value_off : device_def->value_on;
        if ( gpio_value )
            ifx_gpio_output_set(device_def->phys_id, IFX_GPIO_MODULE_LED);
        else
            ifx_gpio_output_clear(device_def->phys_id, IFX_GPIO_MODULE_LED);
    }
#endif
}

static void ledc_brightness_set(struct led_classdev *pled, enum led_brightness value)
{
#if defined(CONFIG_IFX_LEDC) || defined(CONFIG_IFX_LEDC_MODULE)
    struct ifx_led_device *device_def = (struct ifx_led_device *)pled;
    unsigned int ledc_value = value == LED_OFF ? device_def->value_off : device_def->value_on;

	/* Terry 20141021, implement all LEDs blink */
#if 1
	device_def->last_led_brightness = value;
	if (g_all_led_blinking != 0)
		return;
#endif

    ifx_ledc_set_data(device_def->phys_id, ledc_value);
#endif
}

static void ledc_dual_brightness_set(struct led_classdev *pled, enum led_brightness value)
{
#if defined(CONFIG_IFX_LEDC) || defined(CONFIG_IFX_LEDC_MODULE)
    struct ifx_led_device *device_def = (struct ifx_led_device *)pled;
    unsigned int ledc_value_led1 = 0, ledc_value_led2 = 0;
    unsigned phys_id_led1, phys_id_led2;

    if (value == LED_FULL || value == LED_HALF)
        led_dual_calc_color(device_def, (enum led_brightness)device_def->color, &ledc_value_led1, &ledc_value_led2);
    else
        led_dual_calc_color(device_def, value, &ledc_value_led1, &ledc_value_led2);
    phys_id_led1 = IFX_LED_DUAL_GET_PHYS_ID_LED1(device_def->phys_id);
    phys_id_led2 = IFX_LED_DUAL_GET_PHYS_ID_LED2(device_def->phys_id);

    ifx_ledc_set_data2(phys_id_led1, ledc_value_led1, phys_id_led2, ledc_value_led2);
#endif
}

static void ebu_brightness_set(struct led_classdev *pled, enum led_brightness value)
{
#if defined(CONFIG_IFX_EBU_LED) || defined(CONFIG_IFX_EBU_LED_MODULE)
    struct ifx_led_device *device_def = (struct ifx_led_device *)pled;
    unsigned int ledc_value = value == LED_OFF ? device_def->value_off : device_def->value_on;

    ifx_ebu_led_set_data(device_def->phys_id, ledc_value);
#endif
}

static int led_device_create(struct ifx_led_device *device_def)
{
    int ret;

    if ( (device_def->flags & IFX_LED_DEVICE_FLAG_CREATED) )
        return IFX_SUCCESS;

    memset(device_def, 0, sizeof(device_def->device));
    if (device_def->color == 0) device_def->color = 1;

    switch ( device_def->flags & IFX_LED_DEVICE_FLAG_PHYS_MASK ) {
        case IFX_LED_DEVICE_FLAG_PHYS_GPIO:
            {
                //  register for GPIO pins if necessary
                if ( (ret = gpio_register()) != IFX_SUCCESS )
                    return ret;
                //  init led_classdev
                if ( device_def->name[0] == 0 )
                    sprintf(device_def->name, "gpio_%d_%d", IFX_GPIO_PIN_ID_TO_PORT(device_def->phys_id), IFX_GPIO_PIN_ID_TO_PIN(device_def->phys_id));
                device_def->device.name             = device_def->name;
                device_def->device.brightness       = LED_OFF;
                device_def->device.brightness_set   = gpio_brightness_set;
                device_def->device.flags            = 0;
                device_def->device.default_trigger  = device_def->default_trigger;
                //  register
                if ( (ret = led_classdev_register(NULL, &device_def->device)) != 0 ) {
                    gpio_deregister();
                    return IFX_ERROR;
                }
                //  set flag
                device_def->flags |= IFX_LED_DEVICE_FLAG_CREATED;
            }
            break;
        case IFX_LED_DEVICE_FLAG_PHYS_LEDC:
            {
                //  init led_classdev
                if ( device_def->name[0] == 0 )
                    sprintf(device_def->name, "ledc_%d", device_def->phys_id);
                device_def->device.name             = device_def->name;
                device_def->device.brightness       = LED_OFF;
                if ( device_def->flags & IFX_LED_DEVICE_FLAG_DUAL_LED )
                    device_def->device.brightness_set   = ledc_dual_brightness_set;
                else
                    device_def->device.brightness_set   = ledc_brightness_set;
                device_def->device.flags            = 0;
                device_def->device.default_trigger  = device_def->default_trigger;
                //  register
                if ( (ret = led_classdev_register(NULL, &device_def->device)) != 0 )
                    return IFX_ERROR;
                //  set flag
                device_def->flags |= IFX_LED_DEVICE_FLAG_CREATED;
            }
            break;
        case IFX_LED_DEVICE_FLAG_PHYS_EBU:
            {
                unsigned long sys_flags;

                //  turn on EBU LED
                spin_lock_irqsave(&g_led_low_level_operation_lock, sys_flags);
                if ( g_ebu_enabled++ == 0 ) {
#if defined(CONFIG_IFX_EBU_LED) || defined(CONFIG_IFX_EBU_LED_MODULE)
                    ifx_ebu_led_enable();
#endif
                }
                spin_unlock_irqrestore(&g_led_low_level_operation_lock, sys_flags);
                //  init led_classdev
                if ( device_def->name[0] == 0 )
                    sprintf(device_def->name, "ebu_%d", device_def->phys_id);
                device_def->device.name             = device_def->name;
                device_def->device.brightness       = LED_OFF;
                device_def->device.brightness_set   = ebu_brightness_set;
                device_def->device.flags            = 0;
                device_def->device.default_trigger  = device_def->default_trigger;
                //  register
                if ( (ret = led_classdev_register(NULL, &device_def->device)) != 0 ) {
                    spin_lock_irqsave(&g_led_low_level_operation_lock, sys_flags);
                    if ( --g_ebu_enabled == 0 ) {
#if defined(CONFIG_IFX_EBU_LED) || defined(CONFIG_IFX_EBU_LED_MODULE)
                        ifx_ebu_led_disable();
#endif
                    }
                    spin_unlock_irqrestore(&g_led_low_level_operation_lock, sys_flags);
                    return IFX_ERROR;
                }
                //  set flag
                device_def->flags |= IFX_LED_DEVICE_FLAG_CREATED;
            }
            break;
        default:
            dbg("undefined led phys type: %d", device_def->flags & IFX_LED_DEVICE_FLAG_PHYS_MASK);
            return IFX_ERROR;
    }

    if ( device_def->flags & IFX_LED_DEVICE_FLAG_DUAL_LED ) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
        if ( class_device_create_file(device_def->device.class_dev, &class_device_attr_color) != 0 ) {
            led_classdev_unregister(&device_def->device);
            gpio_deregister();
            return IFX_ERROR;
        }
#else
        if ( device_create_file(device_def->device.dev, &dev_attr_color) != 0 ) {
            led_classdev_unregister(&device_def->device);
            gpio_deregister();
            return IFX_ERROR;
        }
#endif
    }

    /* led_set_brightness(&device_def->device, LED_FULL); */
    /* led_set_brightness(&device_def->device, LED_OFF); */

    return IFX_SUCCESS;
}

static void led_device_destroy(struct ifx_led_device *device_def)
{
    unsigned long sys_flags;

    //  no re-entry protection due to low frequency usage of this function
    //  this function can not be called in interrupt context
    if ( (device_def->flags & IFX_LED_DEVICE_FLAG_CREATED) ) {
        device_def->flags &= ~IFX_LED_DEVICE_FLAG_CREATED;
        if ( device_def->flags & IFX_LED_DEVICE_FLAG_DUAL_LED ) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
            class_device_remove_file(device_def->device.class_dev, &class_device_attr_color);
#else
            device_remove_file(device_def->device.dev, &dev_attr_color);
#endif
        }
        led_classdev_unregister(&device_def->device);
        switch ( device_def->flags & IFX_LED_DEVICE_FLAG_PHYS_MASK ) {
            case IFX_LED_DEVICE_FLAG_PHYS_GPIO:
                gpio_deregister();
                break;
            case IFX_LED_DEVICE_FLAG_PHYS_EBU:
                spin_lock_irqsave(&g_led_low_level_operation_lock, sys_flags);
                if ( --g_ebu_enabled == 0 ) {
#if defined(CONFIG_IFX_EBU_LED) || defined(CONFIG_IFX_EBU_LED_MODULE)
                    ifx_ebu_led_disable();
#endif
                }
                spin_unlock_irqrestore(&g_led_low_level_operation_lock, sys_flags);
        }
    }
}

static INLINE int led_trigger_create(const char *trigger, struct led_trigger **pphandler)
{
    struct ifx_led_trigger *ptrigger;
    unsigned long sys_flags;

    ptrigger = kzalloc(sizeof(*ptrigger) + strlen(trigger), GFP_KERNEL);
    if ( ptrigger == NULL )
        return IFX_ERROR;

    strcpy(ptrigger->name, trigger);

    ptrigger->trigger.name      = ptrigger->name;
    ptrigger->trigger.activate  = led_trig_activate;
    ptrigger->trigger.deactivate= led_trig_deactivate;

    ptrigger->delay_on  = DEFAULT_DELAY_ON;
    ptrigger->delay_off = DEFAULT_DELAY_OFF;
    ptrigger->timeout   = DEFAULT_TIMEOUT;
    ptrigger->def_value = DEFAULT_VALUE;

    if ( led_trigger_register(&ptrigger->trigger) != 0 ) {
        kfree(ptrigger);
        return IFX_ERROR;
    }

    write_lock_irqsave(&g_trigger_list_lock, sys_flags);
    list_add_tail(&ptrigger->next_trig, &g_trigger_list);
    write_unlock_irqrestore(&g_trigger_list_lock, sys_flags);

    *pphandler = &ptrigger->trigger;

    return IFX_SUCCESS;
}

static INLINE void led_trigger_destroy(struct led_trigger *phandler)
{
    struct ifx_led_trigger *ptrigger = (struct ifx_led_trigger *)phandler;
    unsigned long sys_flags;

    write_lock_irqsave(&g_trigger_list_lock, sys_flags);
    list_del(&ptrigger->next_trig);
    write_unlock_irqrestore(&g_trigger_list_lock, sys_flags);

    led_trigger_unregister(&ptrigger->trigger);

    kfree(ptrigger);
}

static void led_trig_activate(struct led_classdev *led_cdev)
{
    struct ifx_led_trigger_data *pdata;
    struct ifx_led_trigger *ptrigger;
    struct list_head *ptrigger_entry;
    struct led_classdev *pled;
    struct list_head *pled_entry;
    int quit_flag;

    pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
    if ( !pdata )
        return;

    led_cdev->trigger_data = pdata;

    init_timer(&pdata->blink_timer);
    pdata->blink_timer.function = led_blink_timer_function;
    pdata->blink_timer.data = (unsigned long)led_cdev;

    init_timer(&pdata->timeout_timer);
    pdata->timeout_timer.function = led_timeout_timer_function;
    pdata->timeout_timer.data = (unsigned long)led_cdev;

    pdata->delay_on         = DEFAULT_DELAY_ON;
    pdata->delay_off        = DEFAULT_DELAY_OFF;
    pdata->timeout          = DEFAULT_TIMEOUT;
    pdata->def_value        = led_cdev->brightness == LED_OFF ? 0 : 1;
    pdata->org_brightness   = led_cdev->brightness;

    quit_flag = 0;
    read_lock(&g_trigger_list_lock);
    list_for_each(ptrigger_entry, &g_trigger_list) {
        ptrigger = list_entry(ptrigger_entry, struct ifx_led_trigger, next_trig);
        read_lock(&ptrigger->trigger.leddev_list_lock);
        list_for_each(pled_entry, &ptrigger->trigger.led_cdevs) {
            pled = list_entry(pled_entry, struct led_classdev, trig_list);
            if ( pled == led_cdev ) {
                pdata->delay_on     = ptrigger->delay_on;
                pdata->delay_off    = ptrigger->delay_off;
                pdata->timeout      = ptrigger->timeout;
                if ( ptrigger->delay_on == 0 && ptrigger->delay_off == 0 && ptrigger->timeout == 0 ) {
                    pdata->def_value = ptrigger->def_value;
                    led_set_brightness(led_cdev, pdata->def_value == 0 ? LED_OFF : LED_FULL);
                }
                quit_flag = 1;
                break;
            }
        }
        read_unlock(&ptrigger->trigger.leddev_list_lock);
        if ( quit_flag )
            break;
    }
    read_unlock(&g_trigger_list_lock);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
    if ( class_device_create_file(led_cdev->class_dev, &class_device_attr_delay_on) != 0 )
        goto DELAY_ON_FAIL;
    if ( class_device_create_file(led_cdev->class_dev, &class_device_attr_delay_off) != 0 )
        goto DELAY_OFF_FAIL;
    if ( class_device_create_file(led_cdev->class_dev, &class_device_attr_timeout) != 0 )
        goto TIMEOUT_FAIL;
#else
    if ( device_create_file(led_cdev->dev, &dev_attr_delay_on) != 0 )
        goto DELAY_ON_FAIL;
    if ( device_create_file(led_cdev->dev, &dev_attr_delay_off) != 0 )
        goto DELAY_OFF_FAIL;
    if ( device_create_file(led_cdev->dev, &dev_attr_timeout) != 0 )
        goto TIMEOUT_FAIL;
#endif

    return;

TIMEOUT_FAIL:
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
    class_device_remove_file(led_cdev->class_dev, &class_device_attr_delay_off);
#else
    device_remove_file(led_cdev->dev, &dev_attr_delay_off);
#endif
DELAY_OFF_FAIL:
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
    class_device_remove_file(led_cdev->class_dev, &class_device_attr_delay_on);
#else
    device_remove_file(led_cdev->dev, &dev_attr_delay_on);
#endif
DELAY_ON_FAIL:
    led_cdev->trigger_data = NULL;
    kfree(pdata);
}

static void led_trig_deactivate(struct led_classdev *led_cdev)
{
    struct ifx_led_trigger_data *pdata = led_cdev->trigger_data;

    if ( pdata == NULL )
        return;

    led_set_brightness(led_cdev, pdata->org_brightness);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
    class_device_remove_file(led_cdev->class_dev, &class_device_attr_timeout);
    class_device_remove_file(led_cdev->class_dev, &class_device_attr_delay_off);
    class_device_remove_file(led_cdev->class_dev, &class_device_attr_delay_on);
#else
    device_remove_file(led_cdev->dev, &dev_attr_timeout);
    device_remove_file(led_cdev->dev, &dev_attr_delay_off);
    device_remove_file(led_cdev->dev, &dev_attr_delay_on);
#endif
    del_timer_sync(&pdata->timeout_timer);
    del_timer_sync(&pdata->blink_timer);
    kfree(pdata);
}

static ssize_t led_delay_on_show(struct ifx_led_class_device *dev, CLASS_DEVICE_ATTR_FUNC_PARAM char *buf)
{
    struct led_classdev *led_cdev = ifx_led_class_get_devdata(dev);
    struct ifx_led_trigger_data *pdata = led_cdev->trigger_data;
    int len;

    len = sprintf(buf, "%u\n", pdata->delay_on);

    return len + 1;
}

static ssize_t led_delay_on_store(struct ifx_led_class_device *dev, CLASS_DEVICE_ATTR_FUNC_PARAM const char *buf, size_t size)
{
    struct led_classdev *led_cdev = ifx_led_class_get_devdata(dev);
    struct ifx_led_trigger_data *pdata = led_cdev->trigger_data;
    int ret = -EINVAL;
    char *after;
    unsigned long state = simple_strtoul(buf, &after, 10);
    size_t count = after - buf;

    if ( *after && isspace(*after) )
        count++;

    if ( count == size ) {
        pdata->delay_on = state;
        if ( state == 0 ) {
            del_timer_sync(&pdata->blink_timer);
            if ( pdata->def_value == 0 ) {
                if ( led_cdev->brightness != LED_OFF )
                    led_set_brightness(led_cdev, LED_OFF);
            }
            else {
                if ( led_cdev->brightness == LED_OFF )
                    led_set_brightness(led_cdev, LED_FULL);
            }
        }
        ret = count;
    }

    return ret;
}

static ssize_t led_delay_off_show(struct ifx_led_class_device *dev, CLASS_DEVICE_ATTR_FUNC_PARAM char *buf)
{
    struct led_classdev *led_cdev = ifx_led_class_get_devdata(dev);
    struct ifx_led_trigger_data *pdata = led_cdev->trigger_data;
    int len;

    len = sprintf(buf, "%u\n", pdata->delay_off);

    return len + 1;
}

static ssize_t led_delay_off_store(struct ifx_led_class_device *dev, CLASS_DEVICE_ATTR_FUNC_PARAM const char *buf, size_t size)
{
    struct led_classdev *led_cdev = ifx_led_class_get_devdata(dev);
    struct ifx_led_trigger_data *pdata = led_cdev->trigger_data;
    int ret = -EINVAL;
    char *after;
    unsigned long state = simple_strtoul(buf, &after, 10);
    size_t count = after - buf;

    if ( *after && isspace(*after) )
        count++;

    if ( count == size ) {
        pdata->delay_off = state;
        if ( state == 0 ) {
            del_timer_sync(&pdata->blink_timer);
            if ( pdata->def_value == 0 ) {
                if ( led_cdev->brightness != LED_OFF )
                    led_set_brightness(led_cdev, LED_OFF);
            }
            else {
                if ( led_cdev->brightness == LED_OFF )
                    led_set_brightness(led_cdev, LED_FULL);
            }
        }
        ret = count;
    }

    return ret;
}

static ssize_t led_delay_timeout_show(struct ifx_led_class_device *dev, CLASS_DEVICE_ATTR_FUNC_PARAM char *buf)
{
    struct led_classdev *led_cdev = ifx_led_class_get_devdata(dev);
    struct ifx_led_trigger_data *pdata = led_cdev->trigger_data;
    int len;

    len = sprintf(buf, "%u\n", pdata->timeout);

    return len + 1;
}

static ssize_t led_delay_timeout_store(struct ifx_led_class_device *dev, CLASS_DEVICE_ATTR_FUNC_PARAM const char *buf, size_t size)
{
    struct led_classdev *led_cdev = ifx_led_class_get_devdata(dev);
    struct ifx_led_trigger_data *pdata = led_cdev->trigger_data;
    int ret = -EINVAL;
    char *after;
    unsigned long state = simple_strtoul(buf, &after, 10);
    size_t count = after - buf;

    if ( *after && isspace(*after) )
        count++;

    if ( count == size ) {
        pdata->timeout = state;
        if ( state == 0 )
            del_timer_sync(&pdata->timeout_timer);
        ret = count;
    }

    return ret;
}

static ssize_t led_color_show(struct ifx_led_class_device *dev, CLASS_DEVICE_ATTR_FUNC_PARAM char *buf)
{
    struct ifx_led_device *device_def = (struct ifx_led_device *)ifx_led_class_get_devdata(dev);
    int len;

    len = sprintf(buf, "%u\n", device_def->color);

    return len + 1;
}

static ssize_t led_color_store(struct ifx_led_class_device *dev, CLASS_DEVICE_ATTR_FUNC_PARAM const char *buf, size_t size)
{
    struct led_classdev *led_cdev = ifx_led_class_get_devdata(dev);
    struct ifx_led_device *device_def = (struct ifx_led_device *)ifx_led_class_get_devdata(dev);
    int ret = -EINVAL;
    char *after;
    unsigned long state = simple_strtoul(buf, &after, 10);
    size_t count = after - buf;

    if ( *after && isspace(*after) )
        count++;

    if ( count == size ) {
        device_def->color = state;
        ret = count;
    }
    led_set_brightness(led_cdev, led_cdev->brightness);

    return ret;
}

static void led_blink_timer_function(unsigned long data)
{
    struct led_classdev *led_cdev = (struct led_classdev *) data;
    struct ifx_led_trigger_data *pdata = led_cdev->trigger_data;
    unsigned long brightness;
    unsigned long delay;

    if ( pdata->delay_on == 0 || pdata->delay_off == 0 ) {
        led_set_brightness(led_cdev, LED_FULL);
        return;
    }

    if ( led_cdev->brightness == LED_OFF ) {
        brightness = LED_FULL;
        delay = pdata->delay_on;
    }
    else {
        brightness = LED_OFF;
        delay = pdata->delay_off;
    }

    led_set_brightness(led_cdev, brightness);
    mod_timer(&pdata->blink_timer, jiffies + msecs_to_jiffies(delay));
}

static void led_timeout_timer_function(unsigned long data)
{
    struct led_classdev *led_cdev = (struct led_classdev *) data;
    struct ifx_led_trigger_data *pdata = led_cdev->trigger_data;
    unsigned long sys_flags;

    spin_lock_irqsave(&g_led_low_level_operation_lock, sys_flags);
    if ( pdata->jiffies < jiffies ) {
        spin_unlock_irqrestore(&g_led_low_level_operation_lock, sys_flags);
        del_timer(&pdata->blink_timer);
        if ( pdata->def_value == 0 ) {
            if ( led_cdev->brightness != LED_OFF )
                led_set_brightness(led_cdev, LED_OFF);
        }
        else {
            if ( led_cdev->brightness == LED_OFF )
                led_set_brightness(led_cdev, LED_FULL);
        }
        return;
    }
    spin_unlock_irqrestore(&g_led_low_level_operation_lock, sys_flags);

    mod_timer(&pdata->timeout_timer, pdata->jiffies);
}

static INLINE int gpio_register(void)
{
#if defined(CONFIG_IFX_GPIO) || defined(CONFIG_IFX_GPIO_MODULE)
    unsigned long sys_flags;
    int ret = IFX_SUCCESS;

    spin_lock_irqsave(&g_led_low_level_operation_lock, sys_flags);
    if ( g_gpio_registered++ == 0 ) {
        if ( (ret = ifx_gpio_register(IFX_GPIO_MODULE_LED)) != IFX_SUCCESS )
            --g_gpio_registered;
    }
    spin_unlock_irqrestore(&g_led_low_level_operation_lock, sys_flags);

    return ret;
#else
    return IFX_ERROR;
#endif
}

static INLINE void gpio_deregister(void)
{
#if defined(CONFIG_IFX_GPIO) || defined(CONFIG_IFX_GPIO_MODULE)
    unsigned long sys_flags;

    spin_lock_irqsave(&g_led_low_level_operation_lock, sys_flags);
    if ( g_gpio_registered > 0 && --g_gpio_registered == 0 )
        ifx_gpio_deregister(IFX_GPIO_MODULE_LED);
    spin_unlock_irqrestore(&g_led_low_level_operation_lock, sys_flags);
#endif
}

static INLINE void proc_file_create(void)
{
	struct proc_dir_entry *ent = NULL;

    g_led_dir = proc_mkdir("driver/ifx_led", NULL);

    create_proc_read_entry("version",
                            0,
                            g_led_dir,
                            proc_read_version,
                            NULL);
/* Terry 20141021, implement all LEDs blink */
#if 1
    if (!(ent = create_proc_entry("all_led_blink", S_IFREG | S_IRWXU, g_led_dir))) {
        printk("create proc failed\n");
    } else {
        ent->write_proc = proc_write_all_led_blink;
    }
	g_all_led_blinking = 0;
#endif
}

static INLINE void proc_file_delete(void)
{
/* Terry 20141021, implement all LEDs blink */
#if 1
	remove_proc_entry("all_led_blink", g_led_dir);
#endif

    remove_proc_entry("version", g_led_dir);

    remove_proc_entry("driver/ifx_led", NULL);
}

static int proc_read_version(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
    int len = 0;

    len += ifx_led_version(buf + len);

    if ( offset >= len ) {
        *start = buf;
        *eof = 1;
        return 0;
    }
    *start = buf + offset;
    if ( (len -= offset) > count )
        return count;
    *eof = 1;
    return len;
}

static INLINE int ifx_led_version(char *buf)
{
    return ifx_drv_ver(buf, "LED", IFX_LED_VER_MAJOR, IFX_LED_VER_MID, IFX_LED_VER_MINOR);
}

/* Terry 20141021, implement all LEDs blink */
#if 1
/* Terry 20150317, need to hack for new/old boards. */
#if 1
extern int is_old_board(void);
#endif
static int all_led_blink_thread(void *arg)
{
	unsigned int led_latch = 0;

	/* Disable Power(GPIO26), enable Power red(GPIO27), */
	(*IFX_GPIO_P1_OUT) |=  (0x0400);
	(*IFX_GPIO_P1_OUT) &= ~(0x0800);

	/* Disable red LEDs */
	(*IFX_LED_CPU0)		|=  (0x90);

	while (!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);
		if (led_latch == 0) {
			if (is_old_board())
				(*IFX_GPIO_P0_OUT)	|=  (0x0c00);
			else
				(*IFX_GPIO_P0_OUT)	|=  (0x0408);
			(*IFX_LED_CPU0)		|=  (0x6e);
		} else {
			if (is_old_board())
				(*IFX_GPIO_P0_OUT)	&= ~(0x0c00);
			else
				(*IFX_GPIO_P0_OUT)	&= ~(0x0408);
			(*IFX_LED_CPU0)		&= ~(0x6e);
		}
		led_latch = !led_latch;
		msleep(1000);
	}

	/* Enable Power(GPIO26), disable Power red(GPIO27) */
	(*IFX_GPIO_P1_OUT) &= ~(0x0400);
	(*IFX_GPIO_P1_OUT) |=  (0x0800);
}

static char *all_led_name[] = {
	"108", /* dect */
	"111", /* dsl */
	"113", /* internet_dt */
	"112", /* internet_unconnect */
	"107", /* phone */
	"110", /* info_pin */
	"109", /* info_unpin */
	"115", /* wlan */
	0
};

static INLINE void all_led_blink(int on_off) {
	int i, j;

	if (on_off == 0) {
		if (all_led_blink_thread_id) {
			kthread_stop(all_led_blink_thread_id);
			all_led_blink_thread_id = 0;
		}
		for (i = 0; !(g_board_led_hw_config[i].flags & IFX_LED_DEVICE_FLAG_INVALID); i++) {
			struct ifx_led_device *device_def = &g_board_led_hw_config[i];
			for (j = 0; all_led_name[j]; j++) {
				if (!strcmp(all_led_name[j], device_def->name)) {
					switch (device_def->flags & IFX_LED_DEVICE_FLAG_PHYS_MASK) {
							case IFX_LED_DEVICE_FLAG_PHYS_GPIO:
								gpio_brightness_set((struct led_classdev *)device_def, device_def->last_led_brightness);
								break;
							case IFX_LED_DEVICE_FLAG_PHYS_LEDC:
								ledc_brightness_set((struct led_classdev *)device_def, device_def->last_led_brightness);
								break;
					}
					break;
				}
			}
		}
	} else {
		all_led_blink_thread_id = kthread_create(all_led_blink_thread, IFX_NULL, "all_led_blink_thread");
	
		if (!IS_ERR(all_led_blink_thread_id)) {
			printk ("ALL LED blink thread created..\n");
			wake_up_process(all_led_blink_thread_id);
		}
	}
}

static INLINE void all_green_on() {
	unsigned int led_latch = 0;

	/* Enable Power(GPIO26), disable Power red(GPIO27) */
	(*IFX_GPIO_P1_OUT) &= ~(0x0400);
	(*IFX_GPIO_P1_OUT) |=  (0x0800);

	/* Disable red LEDs */
	(*IFX_LED_CPU0)		|=  (0x90);

	/* Enable all green */
	if (is_old_board())
		(*IFX_GPIO_P0_OUT)	&= ~(0x0c00);
	else
		(*IFX_GPIO_P0_OUT)	&= ~(0x0408);
	(*IFX_LED_CPU0)		&= ~(0x6e);
}

static INLINE void all_red_on() {
	/* Disable Power(GPIO26), enable Power red(GPIO27), */
	(*IFX_GPIO_P1_OUT) |=  (0x0400);
	(*IFX_GPIO_P1_OUT) &= ~(0x0800);
	
	/* Disable all green */
	if (is_old_board())
		(*IFX_GPIO_P0_OUT)	|=  (0x0c00);
	else
		(*IFX_GPIO_P0_OUT)	|=  (0x0408);
	(*IFX_LED_CPU0)		|=  (0x6e);
	
	/* Enable red LEDs */
	(*IFX_LED_CPU0)		&= ~(0x90);
}

static INLINE void all_off() {
	/* Disable red LEDs */
	(*IFX_LED_CPU0)		|=  (0x90);
	
	/* Disable all green */
	if (is_old_board())
		(*IFX_GPIO_P0_OUT)	|=  (0x0c00);
	else
		(*IFX_GPIO_P0_OUT)	|=  (0x0408);
	(*IFX_LED_CPU0)		|=  (0x6e);
	
	/* Disable all power */
	(*IFX_GPIO_P1_OUT) |=  (0x0400);
	(*IFX_GPIO_P1_OUT) |=  (0x0800);
}


static int proc_write_all_led_blink(struct file *file,const char *buf,int count,void *data )
{
	char lbuf[4] = {0};
	int all_blink;

	if (count > 3)
		count = 3;

	if (count > 0) {
		if (copy_from_user(lbuf, buf, count))
			return -EFAULT;
		all_blink = simple_strtol(lbuf, NULL, 0);
		if (g_all_led_blinking != all_blink) {
			g_all_led_blinking = all_blink;
			switch (all_blink) {
				case 1:
					all_led_blink(all_blink);
					break;
				case 2:
					all_green_on();
					break;
				case 3:
					all_red_on();
					break;
				case 4:
					all_off();
					break;
				default:
					g_all_led_blinking = 0;
					all_led_blink(g_all_led_blinking);
			}
		}
	}

	return count;
}
#endif

#if defined(BLINK_POWER_LED) && BLINK_POWER_LED
static INLINE void power_led_flash(void)
{
    if ( g_power_led_trigger != NULL ) {
		struct ifx_led_trigger_attrib attrib = {0};

		/* 1Hz blinking */
		attrib.flags = IFX_LED_TRIGGER_ATTRIB_DELAY_ON | IFX_LED_TRIGGER_ATTRIB_DELAY_OFF | IFX_LED_TRIGGER_ATTRIB_TIMEOUT | IFX_LED_TRIGGER_ATTRIB_DEF_VALUE;
		attrib.delay_on = 500;
		attrib.delay_off = 500;
		attrib.timeout = 0;
		attrib.def_value = 0;
		ifx_led_trigger_set_attrib(g_power_led_trigger, &attrib);
        ifx_led_trigger_activate(g_power_led_trigger);
	}
}
#endif


/*
 * ####################################
 *           Global Function
 * ####################################
 */

/*!
  \fn       int ifx_led_trigger_register(const char *trigger, void **pphandler)
  \brief    Register trigger (event) controlling behavior of LED.

            User uses this function to register trigger (event) controlling behavior of LED.

  \param    trigger     - const char *, name of trigger (event)
  \param    pphandler   - void **, pointer to trigger object
  \return   IFX_SUCCESS     Operation succeed.
  \return   IFX_ERROR       Operation fail.
  \ingroup  IFX_LED_API
 */
int ifx_led_trigger_register(const char *trigger, void **pphandler)
{
    if ( trigger == NULL || pphandler == NULL )
        return IFX_ERROR;

    return led_trigger_create(trigger, (struct led_trigger **)pphandler);
}
EXPORT_SYMBOL(ifx_led_trigger_register);

/*!
  \fn       void ifx_led_trigger_deregister(void *phandler)
  \brief    Deregister trigger (event) controlling behavior of LED.

            User uses this function to deregister trigger (event) controlling behavior of LED.

  \param    phandler    - void **, pointer to trigger object
  \return   IFX_SUCCESS     Operation succeed.
  \return   IFX_ERROR       Operation fail.
  \ingroup  IFX_LED_API
 */
void ifx_led_trigger_deregister(void *phandler)
{
    if ( phandler != NULL )
        led_trigger_destroy(phandler);
}
EXPORT_SYMBOL(ifx_led_trigger_deregister);

/*!
  \fn       void ifx_led_trigger_activate(void *phandler)
  \brief    Event source triggers LED.

            User uses this function to trigger LED. This function is designed as triggering point of event.
            Event source (e.g. ingress/egress ATM packet) calls this function to signal the event.
            The LED has following possible behavior:
            1. LED on when event triggered and be turned off if no more event during certain time slot (e.g. 1 sec).
            2. LED blink.
            3. LED blink and stop blink if no more event during certain time slot (e.g. 1 sec).

  \param    phandler    - void **, pointer to trigger object
  \return   IFX_SUCCESS     Operation succeed.
  \return   IFX_ERROR       Operation fail.
  \ingroup  IFX_LED_API
 */
void ifx_led_trigger_activate(void *phandler)
{
    struct ifx_led_trigger *ptrigger = phandler;
    struct list_head *entry;
    struct led_classdev *led_cdev;
    struct ifx_led_trigger_data *pdata;
    unsigned long sys_flags;

    read_lock(&ptrigger->trigger.leddev_list_lock);
    list_for_each(entry, &ptrigger->trigger.led_cdevs) {
        led_cdev = list_entry(entry, struct led_classdev, trig_list);
        pdata = led_cdev->trigger_data;
        //  blink
        if ( pdata->delay_on != 0 && pdata->delay_off != 0 ) {
            if ( !timer_pending(&pdata->blink_timer) ) {
                pdata->def_value = led_cdev->brightness == LED_OFF ? 0 : 1;
                if ( led_cdev->brightness == LED_OFF )
                    led_set_brightness(led_cdev, LED_FULL);
                mod_timer(&pdata->blink_timer, jiffies + msecs_to_jiffies(pdata->delay_on));
            }
        }
        else {
            if ( pdata->def_value == 0 ) {
                if ( led_cdev->brightness == LED_OFF )
                    led_set_brightness(led_cdev, LED_FULL);
            }
            else {
                if ( led_cdev->brightness != LED_OFF )
                    led_set_brightness(led_cdev, LED_OFF);
            }
        }
        //  timeout
        if ( pdata->timeout != 0 ) {
            spin_lock_irqsave(&g_led_low_level_operation_lock, sys_flags);
#if 1
            pdata->jiffies = jiffies + msecs_to_jiffies(pdata->timeout);
            if ( !timer_pending(&pdata->timeout_timer) ) {
                spin_unlock_irqrestore(&g_led_low_level_operation_lock, sys_flags);
                mod_timer(&pdata->timeout_timer, pdata->jiffies);
            }
            else {
                spin_unlock_irqrestore(&g_led_low_level_operation_lock, sys_flags);
            }
#else
            mod_timer(&pdata->timeout_timer, jiffies + msecs_to_jiffies(pdata->timeout));
            spin_unlock_irqrestore(&g_led_low_level_operation_lock, sys_flags);
#endif
        }
    }
    read_unlock(&ptrigger->trigger.leddev_list_lock);
}
EXPORT_SYMBOL(ifx_led_trigger_activate);

/*!
  \fn       void ifx_led_trigger_deactivate(void *phandler)
  \brief    Restore LED to default state.

            User uses this function to restore LED state. This function provides possibility to force LED reset.

  \param    phandler    - void **, pointer to trigger object
  \return   IFX_SUCCESS     Operation succeed.
  \return   IFX_ERROR       Operation fail.
  \ingroup  IFX_LED_API
 */
void ifx_led_trigger_deactivate(void *phandler)
{
    struct ifx_led_trigger *ptrigger = phandler;
    struct list_head *entry;
    struct led_classdev *led_cdev;
    struct ifx_led_trigger_data *pdata;

    read_lock(&ptrigger->trigger.leddev_list_lock);
    list_for_each(entry, &ptrigger->trigger.led_cdevs) {
        led_cdev = list_entry(entry, struct led_classdev, trig_list);
        pdata = led_cdev->trigger_data;
        del_timer(&pdata->blink_timer);
        del_timer(&pdata->timeout_timer);
        if ( pdata->def_value == 0 ) {
            if ( led_cdev->brightness != LED_OFF )
                led_set_brightness(led_cdev, LED_OFF);
        }
        else {
            if ( led_cdev->brightness == LED_OFF )
                led_set_brightness(led_cdev, LED_FULL);
        }
    }
    read_unlock(&ptrigger->trigger.leddev_list_lock);
}
EXPORT_SYMBOL(ifx_led_trigger_deactivate);

/*!
  \fn       void ifx_led_trigger_set_attrib(void *phandler, struct ifx_led_trigger_attrib *attrib)
  \brief    Set default behavior of LED.

            User uses this function to set default behavior of LED.
            1. LED on when event triggered and be turned off if no more event during certain time slot (e.g. 1 sec).
            2. LED blink.
            3. LED blink and stop blink if no more event during certain time slot (e.g. 1 sec).

  \param    phandler    - void **, pointer to trigger object
  \return   IFX_SUCCESS     Operation succeed.
  \return   IFX_ERROR       Operation fail.
  \ingroup  IFX_LED_API
 */
void ifx_led_trigger_set_attrib(void *phandler, struct ifx_led_trigger_attrib *attrib)
{
    struct ifx_led_trigger *ptrigger = phandler;
    struct list_head *entry;
    struct led_classdev *led_cdev;
    struct ifx_led_trigger_data *pdata;

    if ( (attrib->flags & IFX_LED_TRIGGER_ATTRIB_DELAY_ON) )
        ptrigger->delay_on = attrib->delay_on;
    if ( (attrib->flags & IFX_LED_TRIGGER_ATTRIB_DELAY_OFF) )
        ptrigger->delay_off = attrib->delay_off;
    if ( (attrib->flags & IFX_LED_TRIGGER_ATTRIB_TIMEOUT) )
        ptrigger->timeout = attrib->timeout;
    if ( (attrib->flags & IFX_LED_TRIGGER_ATTRIB_DEF_VALUE) )
        ptrigger->def_value = attrib->def_value;

    read_lock(&ptrigger->trigger.leddev_list_lock);
    list_for_each(entry, &ptrigger->trigger.led_cdevs) {
        led_cdev = list_entry(entry, struct led_classdev, trig_list);
        pdata = led_cdev->trigger_data;
        del_timer(&pdata->blink_timer);
        del_timer(&pdata->timeout_timer);
        if ( (attrib->flags & IFX_LED_TRIGGER_ATTRIB_DELAY_ON) )
            pdata->delay_on = attrib->delay_on;
        if ( (attrib->flags & IFX_LED_TRIGGER_ATTRIB_DELAY_OFF) )
            pdata->delay_off = attrib->delay_off;
        if ( (attrib->flags & IFX_LED_TRIGGER_ATTRIB_TIMEOUT) )
            pdata->timeout = attrib->timeout;
        if ( (attrib->flags & IFX_LED_TRIGGER_ATTRIB_DEF_VALUE) ) {
            pdata->def_value = attrib->def_value;
            led_set_brightness(led_cdev, pdata->def_value == 0 ? LED_OFF : LED_FULL);
        }
    }
    read_unlock(&ptrigger->trigger.leddev_list_lock);
}
EXPORT_SYMBOL(ifx_led_trigger_set_attrib);



/*
 * ####################################
 *           Init/Cleanup API
 * ####################################
 */

/* Terry 20150317, need to hack for new/old boards. */
#if 1
static struct ifx_led_device *modify_board_led_table(struct ifx_led_device *in_cfg, int old_gpio, int new_gpio)
{
	struct ifx_led_device *p = in_cfg;
	
	if (!p)
		return p;
	
	while (p->flags != IFX_LED_DEVICE_FLAG_INVALID) {
		if (p->phys_id == old_gpio) {
			p->phys_id = new_gpio;
			printk("[%s] %d -> %d\n", __FUNCTION__, old_gpio, new_gpio);
			return p + 1;
		}
        p++;
    }
	
	return 0;
}
#endif

/**
 *  ifx_led_init - Initialize port structures
 *
 *  This function initializes the internal data structures of the driver
 *  and will create the proc file entry and device.
 *
 *      Return Value:
 *  @OK = OK
 */
static int __devinit ifx_led_init(void)
{
    char ver_str[256];
    int i;

    spin_lock_init(&g_led_low_level_operation_lock);

	/* Terry 20150317, need to hack for new/old boards. */
#if 1
	if (is_old_board()) {
		/* Hack g_board_led_hw_config */
		struct ifx_led_device *ptr_gpio_pin_cfg = &g_board_led_hw_config[0];
		
		printk("[ifxmips_led] Old board detected. Need to modify GPIO table.\n");
		/* GPIO14 -> GPIO58 */
		ptr_gpio_pin_cfg = modify_board_led_table(ptr_gpio_pin_cfg, 14, 58);
		
		/* GPIO11 -> GPIO3 */
		ptr_gpio_pin_cfg = modify_board_led_table(ptr_gpio_pin_cfg, 3, 11);
	}
#endif
    for ( i = 0; !(g_board_led_hw_config[i].flags & IFX_LED_DEVICE_FLAG_INVALID); i++ ) {
        if ( led_device_create(&g_board_led_hw_config[i]) != IFX_SUCCESS )
            goto LED_DEVICE_CREATE_FAIL;
    }

    g_major = register_chrdev(0, "ifx_led", &g_led_fops);
    if ( g_major <= 0 ) {
        err("Can not register LED device - %d", g_major);
        goto LED_DEVICE_CREATE_FAIL;
    }

    proc_file_create();

    ifx_led_version(ver_str);
    printk(KERN_INFO "%s", ver_str);

#if defined(BLINK_POWER_LED) && BLINK_POWER_LED
    ifx_led_trigger_register("power_led", &g_power_led_trigger);
	power_led_flash();
#endif
	
    return IFX_SUCCESS;

LED_DEVICE_CREATE_FAIL:
    while ( i-- > 0 )
        led_device_destroy(&g_board_led_hw_config[i]);
    return IFX_ERROR;
}

static void __exit ifx_led_exit(void)
{
    int i;

#if defined(BLINK_POWER_LED) && BLINK_POWER_LED
    ifx_led_trigger_deregister(g_power_led_trigger);
    g_power_led_trigger = NULL;
#endif
	
    proc_file_delete();

    unregister_chrdev(g_major, "ifx_led");

    for ( i = 0; !(g_board_led_hw_config[i].flags & IFX_LED_DEVICE_FLAG_INVALID); i++ )
        led_device_destroy(&g_board_led_hw_config[i]);
}

module_init(ifx_led_init);
module_exit(ifx_led_exit);

MODULE_AUTHOR("Xu Liang <liang.xu@infineon.com>");
MODULE_DESCRIPTION("IFX UEIP LED DRIVER");
MODULE_LICENSE("GPL");
