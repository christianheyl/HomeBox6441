/*
 * pecostat_lkm.c LKM to access MIPS_34K and 24K hardware performance counters
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2007 MIPS Technologies, Inc.  All rights reserved.
 * Written by Zenon Fortuna, zenon@mips.com, zenon@fortuna.org
 */


#include "pecostat_version.h"	/* for PECOSTAT_VERSION */

#ifndef __KERNEL__
#  define __KERNEL__
#endif

#ifndef MODULE
#  define MODULE
#endif

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <asm/mipsregs.h>

#include "pecostat.h"

#define DEBUG 0

#define DRIVER_AUTHOR "Zenon Fortuna <zenon@mips.com,zenon@fortuna.org>"
#define DRIVER_DESC   "Read the MIPS hardware performance counters"

MODULE_LICENSE("GPL");

static int __init pecostat_init(void);
static void __exit pecostat_exit(void);

static int pecostat_device_open(struct inode *, struct file *);
static int pecostat_device_release(struct inode *, struct file *);
static loff_t pecostat_device_llseek(struct file *, loff_t, int);
static ssize_t pecostat_device_read(struct file *, char *, size_t, loff_t *);
static ssize_t pecostat_device_write(struct file *, const char *, size_t, loff_t *);

static void save_current_perfctrl(void);
static void restore_perfctrl(void);
static void set_new_events(void);
#ifndef noIRQ
static int pecostat_irq(struct pt_regs *);
static int (*perf_irq_stolen)(struct pt_regs *regs);
#endif

#define DEVICE_NAME "pecostat"	/* Dev name as it appears in /proc/devices */


static int Device_Open = 0;	/* Is device open?  
				 * Used to prevent multiple access to device */
static unsigned char *EventBuffer;
static unsigned PerfctrlInitial[MIPS_34K_HPC_MAX];
static int IrqStolen;
static int PerfctrlSaved;
static int EventsCount;
static int CurrentEvent = 0;
static unsigned long long Extencount[MIPS_34K_HPC_MAX];

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .llseek = pecostat_device_llseek,
    .read = pecostat_device_read,
    .write = pecostat_device_write,
    .open = pecostat_device_open,
    .release = pecostat_device_release
};

static struct miscdevice pecostat_miscdevice = { PECOSTAT_MINOR, "pecostat", &fops };

#ifndef noIRQ
extern int (*perf_irq)(struct pt_regs *regs);
#endif

/*
 * Functions
 */

/*
 * This function is called when the module is loaded
 */
static int __init pecostat_init(void)
{
    int ret;

#ifndef noIRQ
    printk(KERN_ALERT "pecostat module loaded\n");
#else
    printk(KERN_ALERT "pecostat_noIRQ module loaded\n");
#endif

    if ((ret = misc_register(&pecostat_miscdevice)) < 0) {
	  printk(KERN_ALERT "misc_register failed with %d\n", ret);
	  return ret;
    }
    printk(KERN_INFO "pecostat installed, version %s.\n", PECOSTAT_VERSION);

    return 0;
}

/*
 * This function is called when the module is unloaded
 */
static void __exit pecostat_exit(void)
{
    int ret;

    /*
     * Return stolen interrupt handler
     */
    if (IrqStolen) {
#ifndef noIRQ
        perf_irq = perf_irq_stolen;
#endif
        IrqStolen = 0;
    }

    restore_perfctrl();

    /*
     * The EventBuffer is freed automatically after unload, so don't bother
     */

    if ((ret = misc_deregister(&pecostat_miscdevice)) < 0)
	  printk(KERN_ALERT "misc_deregister failed with %d\n", ret);

#ifndef noIRA
    printk(KERN_ALERT "pecostat module unloaded\n");
#else
    printk(KERN_ALERT "pecostat_noIRQ module unloaded\n");
#endif
}

static void
set_new_events()
{
    unsigned long *up_start;
    int i;

    /*
     * Don't do anything, if events not prepared
     */
    if (EventsCount <= 0)
        return;

    /*
     * find the position at the EventBuffer, depended on the CurrentEvent
     */
    up_start = (unsigned long *)(EventBuffer + sizeof(PECOSTAT_INFO) +
        CurrentEvent * MIPS_34K_HPC_MAX * sizeof(unsigned long));

    /*
     * Set the events even if zero (for simplicity)
     */
    write_c0_perfctrl0(up_start[0]);
    write_c0_perfctrl1(up_start[1]);
    write_c0_perfctrl2(up_start[2]);
    write_c0_perfctrl3(up_start[3]);

    /*
     * Set the performance counters to 0
     */
    write_c0_perfcntr0((unsigned long)0);
    write_c0_perfcntr1((unsigned long)0);
    write_c0_perfcntr2((unsigned long)0);
    write_c0_perfcntr3((unsigned long)0);

    for (i=0; i<MIPS_34K_HPC_MAX; i++)
        Extencount[i] = 0;
}

static void
save_current_perfctrl()
{
    /* get the current perfctrl values */
    PerfctrlInitial[0] = read_c0_perfctrl0();
    PerfctrlInitial[1] = read_c0_perfctrl1();
    PerfctrlInitial[2] = read_c0_perfctrl2();
    PerfctrlInitial[3] = read_c0_perfctrl3();

    PerfctrlSaved = 1;
}

static void
restore_perfctrl()
{
    /*
     * Don't do anything, if perfctrl not saved
     */
    if (PerfctrlSaved != 1)
        return;

    /*
     * Use saved controls, then set values to 0
     */
    write_c0_perfctrl0(PerfctrlInitial[0]);
    write_c0_perfctrl1(PerfctrlInitial[1]);
    write_c0_perfctrl2(PerfctrlInitial[2]);
    write_c0_perfctrl3(PerfctrlInitial[3]);

    write_c0_perfcntr0((unsigned long)0);
    write_c0_perfcntr1((unsigned long)0);
    write_c0_perfcntr2((unsigned long)0);
    write_c0_perfcntr3((unsigned long)0);

    PerfctrlSaved = 0;
}

#ifndef noIRQ
static
int pecostat_irq(struct pt_regs *regs)
{
    unsigned long snapshot;

    snapshot = read_c0_perfcntr0();
    if ((long)snapshot < 0) {
        Extencount[0] += (unsigned long long)((unsigned)read_c0_perfcntr0());
        write_c0_perfcntr0((unsigned long)0);
    }

    snapshot = read_c0_perfcntr1();
    if ((long)snapshot < 0) {
        Extencount[1] += (unsigned long long)((unsigned)read_c0_perfcntr1());
        write_c0_perfcntr1((unsigned long)0);
    }

    snapshot = read_c0_perfcntr2();
    if ((long)snapshot < 0) {
        Extencount[2] += (unsigned long long)((unsigned)read_c0_perfcntr2());
        write_c0_perfcntr2((unsigned long)0);
    }

    snapshot = read_c0_perfcntr3();
    if ((long)snapshot < 0) {
        Extencount[3] += (unsigned long long)((unsigned)read_c0_perfcntr3());
        write_c0_perfcntr3((unsigned long)0);
    }

    return 0;
}
#endif

/*
 * Methods
 */

/* 
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int pecostat_device_open(struct inode *inode, struct file *file)
{
    if (Device_Open)
        return -EBUSY;

    Device_Open++;

    /*
     * In 2.6: Deprecated
     *   Instead, set the ".owner = THIS_MODULE" in the fops
     */
    try_module_get(THIS_MODULE);

    return 0;
}

/* 
 * Called when a process closes the device file.
 */
static int pecostat_device_release(struct inode *inode, struct file *file)
{
    /*
     * Return stolen interrupt handler
     */
    if (IrqStolen) {
#ifndef noIRQ
        perf_irq = perf_irq_stolen;
#endif
        IrqStolen = 0;
    }

    restore_perfctrl();

    if (EventBuffer) {
        kfree(EventBuffer);
        EventBuffer = NULL;
    }

    /* 
     * Decrement the usage count, or else once you opened the file, you'll
     * never get get rid of the module. 
     */
    Device_Open--;

    /*
     * In 2.6: Deprecated
     *   Instead, set the ".owner = THIS_MODULE" in the fops
     */
    module_put(THIS_MODULE);

    return 0;
}

/* 
 * Called to "position" the module; Not in use currently
 */
static loff_t pecostat_device_llseek(struct file *filp,
			   loff_t  offset,
                           int     value)
{
    /* loff_t ret = -EINVAL; */

    return -EINVAL;
}

/* 
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t pecostat_device_read(
                                  struct file *filp,/* see include/linux/fs.h */
                                  char *buffer,	 /* buffer to fill with data */
                                  size_t length, /* length of the buffer */
                                  loff_t * offset)
{
#if DEBUG
    printk(KERN_ALERT "request to read %d bytes\n", length);
#endif
    /*
     * We expect the length to be proper length in bytes
     */
    if (length != sizeof(unsigned long long)*MIPS_34K_HPC_MAX)
        return -EINVAL;

    /*
     * Read the performance counters
     */
    Extencount[0] += (unsigned long long)((unsigned)read_c0_perfcntr0());
    Extencount[1] += (unsigned long long)((unsigned)read_c0_perfcntr1());
    Extencount[2] += (unsigned long long)((unsigned)read_c0_perfcntr2());
    Extencount[3] += (unsigned long long)((unsigned)read_c0_perfcntr3());

#if DEBUG
    printk(KERN_ALERT "Extencount=%Lu %Lu %Lu %Lu\n", Extencount[0],
        Extencount[1], Extencount[2], Extencount[3] );
#endif

    copy_to_user(buffer, Extencount, length);

    /*
     * Increment the CurrentEvent, and set the next event
     */
    CurrentEvent = (CurrentEvent + 1) % EventsCount;
    set_new_events();

    return length;
}

/*  
 * Called when a process writes to dev file
 */
static ssize_t
pecostat_device_write(struct file *filp,
                    const char *buffer,
                    size_t length,
                    loff_t * offset)
{
    PECOSTAT_INFO *psip;
#if DEBUG
    unsigned long *up_start;
    int i;
#endif

    /*
     * The "pecostat_tool" will write always a PECOSTAT_INFO followed by
     * a bunch of events.
     * The length is calculated as follows:
     *   length = sizeof(PECOSTAT_INFO) + count*HPC_MAX*sizeof(unsigned)
     * so the
     *   count = (length - sizeof(PECOSTAT_INFO))/(HPC_MAX*sizeof(unsigned))
     */
    EventsCount = (length - sizeof(PECOSTAT_INFO))/(MIPS_34K_HPC_MAX*sizeof(unsigned long));

    if (EventsCount <= 0)
        return -ENOMEM;

    /*
     * Let's allocate local static space for the events, and copy into it
     * the incoming buffer
     */
    if (EventBuffer != NULL)
        kfree(EventBuffer);
    EventBuffer = (unsigned char *)kmalloc(length, GFP_KERNEL);
    copy_from_user(EventBuffer, buffer, length);
    psip = (PECOSTAT_INFO *)EventBuffer;

#if DEBUG
    printk(KERN_ALERT "Got %d event(s)\n", EventsCount);
    up_start = (unsigned long *)(EventBuffer + sizeof(PECOSTAT_INFO) +
        CurrentEvent * MIPS_34K_HPC_MAX * sizeof(unsigned long));
    for (i=0; i<EventsCount; i++)
        printk(KERN_ALERT "ctrls: %lx %lx %lx %lx\n", up_start[i*4+0],
            up_start[i*4+1], up_start[i*4+2], up_start[i*4+3]);
#endif

    /*
     * Let's make a simple test if the "counts" are OK:
     */
    if (psip->events_count != EventsCount) {
        printk(KERN_ALERT "pecostat write: incorrect event_count\n");
        return -ENOMEM;
    }

    /*
     * Install the interrupt handler
     */
    if (IrqStolen == 0) {
#ifndef noIRQ
        perf_irq_stolen = perf_irq;
        perf_irq = pecostat_irq;
#endif
        IrqStolen = 1;
    }

    /*
     * Save current performance counters controls.
     * These will be restored upon close of connection.
     */
    save_current_perfctrl();

    /* Load the first event */
    CurrentEvent = 0;
    set_new_events();

    return length;
}

module_init(pecostat_init);
module_exit(pecostat_exit);
