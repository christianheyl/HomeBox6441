/******************************************************************************
**
** FILE NAME    : ifxmips_mtd_nor.c
** PROJECT      : IFX UEIP
** MODULES      : EBU NOR flash
**
** DATE         : 30 July 2009
** AUTHOR       : Lei Chuanhua
** DESCRIPTION  : EBU MTD NOR flash
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
** 30 Jul,2009  Lei Chuanhua    Initial version
*******************************************************************************/

#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif /* AUTOCONF_INCLUDED */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/io.h>

#include <linux/init.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/cfi.h>

/* Project header */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>

#include "ifxmips_mtd_nor.h"


/*!
  \defgroup IFX_NOR_DRV UEIP Project - nor flash driver
  \brief UEIP Project - NOR flash driver, supports LANTIQ CPE platforms(Danube/ASE/ARx/VRx).
 */


/*!
  \file ifxmips_mtd_nor.c
  \ingroup IFX_NOR_DRV
  \brief NOR driver main source file.
*/



#define IFX_NOR_FLASH_BANK_MAX     1
#define IFX_MTD_NOR_DATAWIDTH      2  /* 16 bit */

#define IFX_MTD_NOR_VER_MAJOR      1
#define IFX_MTD_NOR_VER_MID        1
#define IFX_MTD_NOR_VER_MINOR      1

#define IFX_MTD_NOR_BANK_NAME      "ifx_nor" /* cmd line bank name should be the same */
#define IFX_MTD_NOR_NAME_LEN       16

//#define IFX_MTD_DEBUG
#ifdef IFX_MTD_DEBUG
#define IFX_MTD_PRINT(fmt, args...) \
    printk("[%s]: " fmt, __func__, ##args)
#else
#define IFX_MTD_PRINT(fmt, args...)
#endif

/* Trivial struct to describe partition information */
struct mtd_part_def {
    int nums;
    u8 *type;
    struct mtd_partition *mtd_part;
};

/* static struct mtd_info *mymtd; */
static struct mtd_info *ifx_mtd_banks[IFX_NOR_FLASH_BANK_MAX];
static struct map_info ifx_map_banks[IFX_NOR_FLASH_BANK_MAX];
#ifdef CONFIG_MTD_PARTITIONS
static struct mtd_part_def ifx_part_banks[IFX_NOR_FLASH_BANK_MAX];
#endif
static u32 ifx_mtd_num_banks;
static unsigned long start_scan_addr;

/*
 * Endianess bug in ASE, Danube. CFI-compatible flash probing and read/write/erase
 * has different option. This variable is used to differentiate them so that different
 * strategies can be used.
 */
static int ifx_mtd_probing = 0;

#ifdef CONFIG_MTD_PARTITIONS
#ifdef CONFIG_MTD_CMDLINE_PARTS
static const char *part_probes[] = { "cmdlinepart", NULL };
#endif
#endif

#if defined(CONFIG_AR9) || defined(CONFIG_VR9)
static struct proc_dir_entry *g_nor_proc_dir = NULL;

/*
 * Delay during NOR access to avoid blocking MIPS core.
 */
static int mtd_delay=0;
static int atomic_len=0;
static unsigned long ini, end, lapse;
static unsigned long avg_lapse=0;
static unsigned long tot_lapse=0;
static unsigned long min_lapse=-1;
static unsigned long max_lapse=0;
static unsigned long lapse_count=0;
static unsigned long copy_to_num=0;
static unsigned long copy_from_num=0;
static unsigned long req_size_to=0;
static unsigned long req_size_from=0;
static unsigned long max_req_size_to=0;
static unsigned long max_req_size_from=0;
static unsigned long min_req_size_to=-1;
static unsigned long min_req_size_from=-1;
#endif

/*
 * NOR flash base address and size, defined in ifxmips_prom.c
 * Passed from u-boot or builtin parameters for emulation
 */
extern unsigned long g_ifx_nor_flash_start;
extern unsigned long g_ifx_nor_flash_size;

/* Partition table, defined in platform_board.c */
extern const struct mtd_partition g_ifx_mtd_nor_partitions[];
extern const int g_ifx_mtd_partion_num;

#ifdef CONFIG_DANUBE_EBU_PCI_SW_ARBITOR
extern void ifx_enable_ebu(void);
extern void ifx_disable_ebu(void);
#elif defined(CONFIG_AR9) || defined(CONFIG_VR9)
#define rdtscl(dest)        __asm__ __volatile__("mfc0 %0,$9;nop" : "=r" (dest))

static void inline Xudelay(unsigned long usecs)
{
    unsigned long cur_time;
    unsigned long initial_time;
    unsigned long interval=usecs*1000/6;

    rdtscl(initial_time);
    while (1) {
        rdtscl(cur_time);
        if ((cur_time-initial_time)>=interval)
            break;
    }
}

static void inline ifx_enable_ebu(void)
{
    Xudelay(mtd_delay);
    rdtscl(ini);
}

static void inline ifx_disable_ebu(void)
{
    unsigned long tmp;

    rdtscl(end);
    lapse=end-ini;
    tmp=tot_lapse+lapse;
    if (tmp<tot_lapse) {
       /*roll over*/
       tot_lapse=tot_lapse/lapse_count;
       lapse_count=1;
    }
    else {
      tot_lapse=tmp;
      lapse_count++;
    }
    if (lapse>max_lapse) max_lapse=lapse;
    if (lapse<min_lapse) min_lapse=lapse;
}
#else
static void inline ifx_enable_ebu(void)
{
}

static void inline ifx_disable_ebu(void)
{
}
#endif

/*!
  \fn map_word ifx_mtd_map_read(struct map_info * map, unsigned long ofs)
  \ingroup  IFX_NOR_DRV
  \brief read data from the chip
  \param map MTD map_info structure
  \param ofs offset
  \return value read from the chip
*/
static map_word
ifx_mtd_map_read(struct map_info * map, unsigned long ofs)
{
    map_word temp;

    if (map_bankwidth_is_1(map)) {

        ifx_enable_ebu();
        temp.x[0] = *(u8 *)(map->map_priv_1 + ofs);
        ifx_disable_ebu();

        IFX_MTD_PRINT("map_bankwidth_is_1 r8: [0x%08x + 0x%02x] ==> 0x%02x\n",
            (unsigned int)map->map_priv_1, (unsigned int)ofs, (unsigned int)temp.x[0]);
    }
    else if (map_bankwidth_is_2(map)) {
        /* WAR, kernel 2.4 and kernel 2.6 uses the hardcode address in probing */
        if (ifx_mtd_probing) {
            ofs ^= 2;
        }

        ifx_enable_ebu();
        temp.x[0] = *(u16 *)(map->map_priv_1 + ofs);
        ifx_disable_ebu();

        IFX_MTD_PRINT("map_bankwidth_is_2 r16: [0x%08x + %ld] ==> %x\n",
            (unsigned int)map->map_priv_1, ofs, (u16)temp.x[0]);
    }
    else if (map_bankwidth_is_4(map)) {

        ifx_enable_ebu();
        temp.x[0] = *(u32 *)(map->map_priv_1 + ofs);
        ifx_disable_ebu();

        IFX_MTD_PRINT("map_bankwidth_is_4 r32: [0x%08x + 0x%02x] ==> 0x%08x\n",
            (unsigned int)map->map_priv_1, (unsigned int)ofs, (unsigned int)temp.x[0]);
    }
    else if (map_bankwidth_is_large(map)) {
        int i;

        u8 *p = (u8 *)(map->map_priv_1 + ofs);
        u8 *d = (u8 *)temp.x;

        IFX_MTD_PRINT("map_bankwidth_is_large\n");
        ifx_enable_ebu();
        for (i = 0; i < map->bankwidth; i++) {
            *d++ = *p++;
        }
        ifx_disable_ebu();
    }
    return temp;
}

/*!
  \fn void ifx_mtd_map_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
  \ingroup  IFX_NOR_DRV
  \brief copy data from the chip
  \param map MTD map_info structure
  \param to   destination
  \param from source
  \param len  number of bytes to copy
  \return none
*/
static void
ifx_mtd_map_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
#if defined(CONFIG_AR9) || defined(CONFIG_VR9)
    /* WAR, EBU access that causes unaligned access exception */
    u8 *p;
    u8 *q;
    int delay_count=0;

    copy_from_num++;
    req_size_from=len;
    if (req_size_from > max_req_size_from)
        max_req_size_from = req_size_from;
    if (min_req_size_from == -1 || req_size_from < min_req_size_from)
        min_req_size_from = req_size_from;

    IFX_MTD_PRINT("from:%lux to:%p len:%d\n", from, to, len);

    ifx_enable_ebu();
    from = (unsigned long) (from + map->map_priv_1);
    p = (u8 *) (from);
    q = (u8 *) (to);
    while (len--) {
        delay_count++;
        if (delay_count==atomic_len) {
            Xudelay(mtd_delay);
            delay_count=0;
        }
        *q++ = *p++;
    }
    ifx_disable_ebu();
#else
    /* WAR, EBU access that causes unaligned access exception */
    u8 *p;
    u8 *q;

    IFX_MTD_PRINT("from:%lux to:%p len:%d\n", from, to, len);

    ifx_enable_ebu();
    from = (unsigned long) (from + map->map_priv_1);
    if ((((unsigned long) to) & 0x3) == (from & 0x3)) {
        memcpy_fromio (to, (void *) from, len);
    }
    else {
        p = (u8 *) (from);
        q = (u8 *) (to);
        while (len--) {
            *q++ = *p++;
        }
    }
    ifx_disable_ebu();
#endif
}

/*!
  \fn map_word ifx_mtd_map_write(struct map_info *map, const map_word datum, unsigned long ofs)
  \ingroup  IFX_NOR_DRV
  \brief write data to the chip
  \param map MTD map_info structure
  \param dataum data to write
  \param ofs offset
  \return none
*/
static void
ifx_mtd_map_write(struct map_info *map, const map_word datum, unsigned long ofs)
{
    if (map_bankwidth_is_1(map)) {

        ifx_enable_ebu();
        *(u8 *)(map->map_priv_1 + ofs) = (u8)datum.x[0];
        ifx_disable_ebu();

        IFX_MTD_PRINT("map_bankwidth_is_1 w8: [0x%08x + 0x%02x] <== 0x%02x\n",
            (unsigned int) map->map_priv_1, (unsigned int)ofs, (unsigned int)datum.x[0]);
    }
    else if (map_bankwidth_is_2(map)) {
        /* WAR, kernel 2.4 and kernel 2.6 uses the hardcode address in probing */
        if (ifx_mtd_probing) {
            ofs ^= 2;
        }

        ifx_enable_ebu();
        *(u16 *)(map->map_priv_1 + ofs) = (u16)datum.x[0];
        wmb();
        ifx_disable_ebu();

        IFX_MTD_PRINT("map_bankwidth_is_2 w16: [0x%08x + %ld] <== %x\n",
            (unsigned int)map->map_priv_1, ofs, (u16)datum.x[0]);
    }
    else if (map_bankwidth_is_4(map)){

        ifx_enable_ebu();
        *(u32 *)(map->map_priv_1 + ofs) = datum.x[0];
        ifx_disable_ebu();

        IFX_MTD_PRINT("map_bankwidth_is_4 w32: [0x%08x + 0x%02x] <== 0x%08x\n",
            (unsigned int)map->map_priv_1, (unsigned int)ofs, (unsigned int)datum.x[0]);
    }
    else if (map_bankwidth_is_large(map)) {
        int i;

        u8 *p = (u8 *)datum.x;
        u8 *d = (u8 *)(map->map_priv_1 + ofs);

        IFX_MTD_PRINT("map_bankwidth_is_large\n");

        ifx_enable_ebu();
        for (i = 0; i < map->bankwidth; i++) {
            *d++ = *p++;
        }
        ifx_disable_ebu();
    }
}

/*!
  \fn void ifx_mtd_map_copy_from(struct map_info *map, unsigned long to, const void *from,, ssize_t len)
  \ingroup  IFX_NOR_DRV
  \brief copy data to the chip
  \param map MTD map_info structure
  \param to  destination
  \param from source
  \param len  number of bytes to copy
  \return none
*/
static void
ifx_mtd_map_copy_to(struct map_info *map, unsigned long to, const void *from,
        ssize_t len)
{
#if defined(CONFIG_AR9) || defined(CONFIG_VR9)
    u8* p;
    u8* q;
    int delay_count=0;

    copy_to_num++;
    req_size_to=len;
    if (req_size_to > max_req_size_to)
        max_req_size_to = req_size_to;
    if (min_req_size_to == -1 || req_size_to < min_req_size_to)
        min_req_size_to = req_size_to;

    IFX_MTD_PRINT("from:%x to:%x len:%d", (unsigned int)from, (unsigned int)to, len);

    ifx_enable_ebu();
    to = (unsigned long)(map->map_priv_1 + to);
    p = (u8 *) (from);
    q = (u8 *) (to);
    while (len--) {
        delay_count++;
        if (delay_count==atomic_len) {
            Xudelay(mtd_delay);
            delay_count=0;
        }
        *q++ = *p++;
    }
    ifx_disable_ebu();
#else
    IFX_MTD_PRINT("from:%x to:%x len:%d", (unsigned int)from, (unsigned int)to, len);

    ifx_enable_ebu();
    memcpy_toio((void *)(map->map_priv_1 + to), from, len);
    ifx_disable_ebu();
#endif
}

#if defined(CONFIG_AR9) || defined(CONFIG_VR9)
static int proc_read_delay(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len=0;

    avg_lapse=tot_lapse/lapse_count;

    len += sprintf(page + off + len, "mtd_delay         = %dus\n",mtd_delay);
    len += sprintf(page + off + len, "atomic_len        = %dbytes\n", atomic_len);
    len += sprintf(page + off + len, "average lapse     = %li\n", avg_lapse);
    len += sprintf(page + off + len, "max lapse         = %li\n", max_lapse);
    len += sprintf(page + off + len, "min lapse         = %li\n", min_lapse);
    len += sprintf(page + off + len, "last lapse        = %li\n", lapse);
    len += sprintf(page + off + len, "copy to num       = %li\n", copy_to_num);
    len += sprintf(page + off + len, "copy from num     = %li\n", copy_from_num);
    len += sprintf(page + off + len, "req_size_to       = %li\n", req_size_to);
    len += sprintf(page + off + len, "req_size_from     = %li\n", req_size_from);
    len += sprintf(page + off + len, "max_req_size_to   = %li\n", max_req_size_to);
    len += sprintf(page + off + len, "max_req_size_from = %li\n", max_req_size_from);
    len += sprintf(page + off + len, "min_req_size_to   = %li\n", min_req_size_to);
    len += sprintf(page + off + len, "min_req_size_from = %li\n", min_req_size_from);

    *eof = 1;
    return len;
}

char* strtok(char *str,char *delim,char *ptr,int num)
{
    int n;
    int start=0;
    int end=0;
    int start_flag=0;
    int delim_flag=0;
    int delim_count=0;
    int count=0;

    for (n=0;n<100;n++) {
        if (str[n]==0x0a) {
            break;
        }else {
            count++;
        }
    }

    if (num==0) {
        start_flag=1;
        start=0;
    }

    for (n=0;n<count;n++) {
        if (str[n]==*delim) {
            if (start_flag==1) {
                end=n-1;
                break;
            }
            if (!delim_flag) {
                delim_flag=1;
                delim_count++;
            }
        } else {
            delim_flag=0;
            if (delim_count==num && !start_flag) {
                start=n;
                start_flag=1;
            }
            if (n==count-1) {
                end=n;
                break;
            }
        }
    }

    memcpy(ptr,str+start,(end-start+1));
    ptr[end-start+1]='\0';
    return ptr;
}

#define STRTOK(str,ptr,num) strtok(str," ",(char*)&ptr,num)

static int proc_write_delay(struct file *file, const char *buf, unsigned long count, void *data)
{
    char str[20];
    int ret;
    char ptr[10];

    ret=copy_from_user(str,buf,count);

    if (strcmp(STRTOK(str,ptr,0),"atomiclen")==0) {
        atomic_len=simple_strtoul(STRTOK(str,ptr,1),NULL,0);
        printk("set atomic len to %d bytes\n",atomic_len);
    }
    else if (strcmp(STRTOK(str,ptr,0),"delay")==0) {
        mtd_delay=simple_strtoul(STRTOK(str,ptr,1),NULL,0);
        printk("set delay to %d us\n",mtd_delay);
    }
    else if (strcmp(STRTOK(str,ptr,0),"reset")==0) {
        mtd_delay=0;
        atomic_len=0;
        avg_lapse=0;
        tot_lapse=0;
        min_lapse=-1;
        max_lapse=0;
        lapse_count=0;
        lapse=0;
        copy_to_num=0;
        copy_from_num=0;
        req_size_to=0;
        req_size_from=0;
        max_req_size_to=0;
        max_req_size_from=0;
        min_req_size_to=-1;
        min_req_size_from=-1;
        printk("counter reseted!\n");
    }
    else {
        printk("help            information\n");
        printk("atomiclen <len> len\n");
        printk("delay <delay>   delay\n");
        printk("reset           reset\n");
    }

    return count;
}

#endif

static inline void proc_file_create(void)
{
#if defined(CONFIG_AR9) || defined(CONFIG_VR9)
    struct proc_dir_entry *res;

    g_nor_proc_dir = proc_mkdir("driver/ifx_mtd_nor", NULL);

    res = create_proc_entry("delay", 0, g_nor_proc_dir);
    if (res) {
        res->read_proc  = proc_read_delay;
        res->write_proc = proc_write_delay;
    }
#endif
}

static inline void proc_file_delete(void)
{
#if defined(CONFIG_AR9) || defined(CONFIG_VR9)
    remove_proc_entry("delay", g_nor_proc_dir);

    remove_proc_entry("driver/ifx_mtd_nor", NULL);
#endif
}

static inline void
ifx_mtd_show_version(void)
{
    char ver_buf[128] = {0};

    ifx_drv_ver(ver_buf, "MTD NOR", IFX_MTD_NOR_VER_MAJOR, IFX_MTD_NOR_VER_MID, IFX_MTD_NOR_VER_MINOR);

    printk(KERN_INFO "%s", ver_buf);
}

static int __init
ifx_mtd_init(void)
{
    u32 reg;
    int idx = 0, ret = 0;
    u32 mtd_size = 0;
    u32 flash_size = 0;
    u32 flash_start = -1;

#ifdef CONFIG_MTD_CMDLINE_PARTS /* From u-boot */
    flash_size = g_ifx_nor_flash_size;
    flash_start = g_ifx_nor_flash_start;
#else
    flash_size = (CONFIG_MTD_IFX_NOR_FLASH_SIZE << 20); /* From kernel */
    flash_start = IFX_MTD_NOR_VIRT_ADDR;
#endif
    /* Sanity check */
    if ((flash_size == 0)) {
        printk(KERN_ERR "%s: invalid flash size!!!\n",  __func__);
        return -EINVAL;
    }

    if (flash_start == -1) {
        printk(KERN_ERR "%s: invalid flash address!!!\n", __func__);
        return -EINVAL;
    }

    /* Configure EBU */
    reg = SM(IFX_EBU_BUSCON0_XDM16, IFX_EBU_BUSCON0_XDM) |
            SM(IFX_EBU_BUSCON0_ALEC3, IFX_EBU_BUSCON0_ALEC) |
            SM(IFX_EBU_BUSCON0_BCGEN_INTEL,IFX_EBU_BUSCON0_BCGEN) |
            SM(IFX_EBU_BUSCON0_WAITWRC7, IFX_EBU_BUSCON0_WAITWRC) |
            SM(IFX_EBU_BUSCON0_WAITRDC3, IFX_EBU_BUSCON0_WAITRDC) |
            SM(IFX_EBU_BUSCON0_HOLDC3, IFX_EBU_BUSCON0_HOLDC) |
            SM(IFX_EBU_BUSCON0_RECOVC3, IFX_EBU_BUSCON0_RECOVC);

/* XXX, VR9 support */
#ifdef CONFIG_MTD_IFX_LESS_WAIT_CYCLE
#ifdef CONFIG_AR9
    /* 393/196MHz */
    if ((MS(IFX_REG_R32(IFX_CGU_SYS), IFX_CGU_SYS_SEL) == IFX_CGU_SYS_SEL_393)) {
        reg = SM(IFX_EBU_BUSCON0_XDM16, IFX_EBU_BUSCON0_XDM) |
            SM(IFX_EBU_BUSCON0_ALEC3, IFX_EBU_BUSCON0_ALEC) |
            SM(IFX_EBU_BUSCON0_BCGEN_INTEL,IFX_EBU_BUSCON0_BCGEN) |
            SM(IFX_EBU_BUSCON0_WAITWRC5, IFX_EBU_BUSCON0_WAITWRC) |
            SM(IFX_EBU_BUSCON0_WAITRDC2, IFX_EBU_BUSCON0_WAITRDC) |
            SM(IFX_EBU_BUSCON0_HOLDC2, IFX_EBU_BUSCON0_HOLDC) |
            SM(IFX_EBU_BUSCON0_RECOVC2, IFX_EBU_BUSCON0_RECOVC) |
            SM(IFX_EBU_BUSCON0_CMULT8, IFX_EBU_BUSCON0_CMULT);
    }
    /* 333/196MHz */
    else {
        reg |= SM(IFX_EBU_BUSCON0_CMULT4, IFX_EBU_BUSCON0_CMULT);
    }
#elif defined (CONFIG_VR9)
    reg |= SM(IFX_EBU_BUSCON0_CMULT4, IFX_EBU_BUSCON0_CMULT);
#endif /* CONFIG_AR9 */
#else
    reg |= SM(IFX_EBU_BUSCON0_CMULT16, IFX_EBU_BUSCON0_CMULT);
#endif /* defined(CONFIG_MTD_IFX_LESS_WAIT_CYCLE) */

    IFX_REG_W32(reg, IFX_EBU_BUSCON0);

    /* Get flash physical address first, then map it to virtual address */
    flash_start &= ~KSEG1;
    start_scan_addr = (unsigned long)ioremap_nocache(flash_start, flash_size);
    if (!start_scan_addr) {
        IFX_MTD_PRINT("Failed to ioremap address:0x%08x\n", flash_start);
        return -EIO;
    }

    IFX_MTD_PRINT("start_scan_addr: 0x%08lx, flash_start: 0x%08x flash_size %u\n",
        start_scan_addr, flash_start, flash_size);

    for (idx = 0; idx < IFX_NOR_FLASH_BANK_MAX; idx++) {
        struct map_info *map_bank;

        if (mtd_size >= flash_size) {
            break;
        }

        map_bank = &ifx_map_banks[idx];
        memset(map_bank , 0, sizeof (struct map_info));
        map_bank->name = kmalloc(IFX_MTD_NOR_NAME_LEN, GFP_KERNEL);
        if (map_bank->name == NULL) {
            ret = -ENOMEM;
            goto err1;
        }
        memset((void *)map_bank->name, 0, IFX_MTD_NOR_NAME_LEN);

        /* Bank name must be the same as command line bank name */
        sprintf((void *)map_bank->name, "%s%d", IFX_MTD_NOR_BANK_NAME, idx);
        map_bank->bankwidth  = IFX_MTD_NOR_DATAWIDTH;
        map_bank->read       = ifx_mtd_map_read;
        map_bank->copy_from  = ifx_mtd_map_copy_from;
        map_bank->write      = ifx_mtd_map_write;
        map_bank->copy_to    = ifx_mtd_map_copy_to;
        map_bank->map_priv_1 = start_scan_addr;
        map_bank->virt       = (void __iomem *)start_scan_addr;
        /* Make sure probing works */
        map_bank->size       = flash_size;

        /* Start to probe flash chips */
        ifx_mtd_probing = 1;
        ifx_mtd_banks[idx] = do_map_probe("cfi_probe", map_bank);
        ifx_mtd_probing = 0;
        if (ifx_mtd_banks[idx]) {
            struct cfi_private *cfi;

            ifx_mtd_banks[idx]->owner =  THIS_MODULE;
            mtd_size += ifx_mtd_banks[idx]->size;
            ifx_mtd_num_banks++;

            cfi = (struct cfi_private *)map_bank->fldrv_priv;

            /*
             * WAR, swap the address for correct access via EBU
             * Kernel 2.6 has different unlock address geneartion from kernel 2.4.x
             */
            cfi->addr_unlock1 ^= 1;
            cfi->addr_unlock2 ^= 1;
            IFX_MTD_PRINT("%s: bank%d, name:%s, size:%lluB\n", __func__, ifx_mtd_num_banks,
                ifx_mtd_banks[idx]->name, ifx_mtd_banks[idx]->size);
        }
    }
    /* No supported flash chips found */
    if (ifx_mtd_num_banks == 0) {
        printk(KERN_ERR "%s: No support flash chips found!\n", __func__);
        ret = -ENXIO;
        goto err1;
    }

#ifdef CONFIG_MTD_PARTITIONS
    for (idx = 0; idx < ifx_mtd_num_banks; idx++) {
    #ifdef CONFIG_MTD_CMDLINE_PARTS
        /* Select dynamic from cmdline partition definitions */
        ifx_part_banks[idx].type = "dynamic image";
        ifx_part_banks[idx].nums = parse_mtd_partitions(ifx_mtd_banks[idx], part_probes, &ifx_part_banks[idx].mtd_part, 0);
        if (ifx_part_banks[idx].nums <= 0) {
            printk(KERN_ERR "%s no valid mtd partion table configured in command line\n", __func__);
            goto err1;
        }
    #else
        /* Select Static partition definitions */
        ifx_part_banks[idx].mtd_part = (struct mtd_partition *)g_ifx_mtd_nor_partitions;
        ifx_part_banks[idx].type = "static image";
        ifx_part_banks[idx].nums = g_ifx_mtd_partion_num;

        if (ifx_part_banks[idx].nums == 0) {
            printk(KERN_INFO "%s flash%d no partition info available, registering whole flash at once\n",
                __func__, idx);
            add_mtd_device(ifx_mtd_banks[idx]);
        }
        else
    #endif  /* CONFIG_MTD_CMDLINE_PARTS */
        {
            printk(KERN_INFO "%s flash%d: Using %s partition\n",
                __func__, idx, ifx_part_banks[idx].type);
            add_mtd_partitions(ifx_mtd_banks[idx], ifx_part_banks[idx].mtd_part,
                ifx_part_banks[idx].nums);
        }
    }
#else
    printk(KERN_INFO "%s flash: registering %d whole flash banks at once\n",
        __func__, ifx_mtd_num_banks);

    for (idx = 0; idx < ifx_mtd_num_banks; idx++) {
        add_mtd_device(ifx_mtd_banks[idx]);
    }
#endif /* CONFIG_MTD_PARTITIONS */

    proc_file_create();

    ifx_mtd_show_version();
    return 0;
err1:
    for (idx = 0; idx < IFX_NOR_FLASH_BANK_MAX; idx++) {
        if (ifx_map_banks[idx].name != NULL) {
            kfree(ifx_map_banks[idx].name);
            ifx_map_banks[idx].name = NULL;
        }
    }
    iounmap((void *)start_scan_addr);
    return ret;
}

static void __exit
ifx_mtd_exit(void)
{
    int idx = 0;

    proc_file_delete();

    for (idx = 0; idx < ifx_mtd_num_banks; idx++) {
        /* destroy mtd_info previously allocated */
        if (ifx_mtd_banks[idx]) {
#ifdef CONFIG_MTD_PARTITIONS
            del_mtd_partitions(ifx_mtd_banks[idx]);
#endif
            map_destroy(ifx_mtd_banks[idx]);
        }
        /* release map_info not used anymore */
        kfree(ifx_map_banks[idx].name);
    }

    if (start_scan_addr) {
        iounmap((void *) start_scan_addr);
        start_scan_addr = 0;
    }
}

module_init(ifx_mtd_init);
module_exit(ifx_mtd_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Lei Chuanhua, chuanhua.lei@lantiq.com");
MODULE_SUPPORTED_DEVICE ("IFX CPE Reference Board");
MODULE_DESCRIPTION ("IFX CPE MTD MAP driver");
