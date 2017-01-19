/******************************************************************************
**
** FILE NAME    : ar10.c
** PROJECT      : IFX UEIP
** MODULES      : BSP Basic
**
** DATE         : 27 May 2009
** AUTHOR       : Xu Liang
** DESCRIPTION  : source file for AR10
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
** $Date        $Author         $Comment
** 27 May 2009   Xu Liang        The first UEIP release
*******************************************************************************/



#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif /* AUTOCONF_INCLUDED */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kallsyms.h>
#include <linux/version.h>
#ifdef CONFIG_QUALCOMM_AP_PPA
/* ARCADYAN #ARC_RC_SD2#, 2013/6/18, Dino, support of load 9880 cal data from flash */
#include <linux/mtd/mtd.h>
#include <linux/err.h>
#endif

/*
 *  Chip Specific Head File
 */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/ifx_pmu.h>
#include <asm/ifx/common_routines.h>

/*
 *  Voice firmware decryt function pointer in Boot Rom
 */
const void (*ifx_bsp_basic_mps_decrypt)(unsigned int addr, int n);

static char g_pkt_base[2048] __initdata __attribute__((__aligned__(32)));
static unsigned int g_desc_base[2] __initdata __attribute__((__aligned__(32)));
static void __init ifx_dplus_clean(void)
{
#define PPE_REG_ADDR(x)     ((volatile unsigned int *)KSEG1ADDR(0x1E180000 | (((x) + 0x4000) << 2)))
#define DMRX_PGCNT          PPE_REG_ADDR(0x0615)
#define DMRX_PKTCNT         PPE_REG_ADDR(0x0616)
#define DSRX_PGCNT          PPE_REG_ADDR(0x0713)

#define AR10_SWIP_MACRO                 0x1E108000
#define AR10_SWIP_MACRO_REG(off)        ((volatile unsigned int *)KSEG1ADDR(AR10_SWIP_MACRO + (off) * 4))
#define AR10_SWIP_TOP                   (AR10_SWIP_MACRO | (0x0C40 * 4))
#define AR10_SWIP_TOP_REG(off)          ((volatile unsigned int *)KSEG1ADDR(AR10_SWIP_TOP + (off) * 4))
#define PCE_PCTRL_REG(port, reg)        AR10_SWIP_MACRO_REG(0x480 + (port) * 0xA + (reg))    //  port < 12, reg < 4
#define FDMA_PCTRL_REG(port)            AR10_SWIP_MACRO_REG(0xA80 + (port) * 6)  //  port < 7
#define SDMA_PCTRL_REG(port)            AR10_SWIP_MACRO_REG(0xBC0 + (port) * 6)  //  port < 7

    volatile unsigned int *desc_base = (volatile unsigned int *)KSEG1ADDR((unsigned int)g_desc_base);
    int i, j, k;

    for ( i = 0; i < 6; i++ )
        IFX_REG_W32_MASK(1, 0, SDMA_PCTRL_REG(i));  //  stop port 0 - 5

    if ( (IFX_REG_R32(DMRX_PGCNT) & 0x00FF) == 0 && (IFX_REG_R32(DSRX_PGCNT) & 0x00FF) == 0 )
        return;

    IFX_REG_W32(0, IFX_DMA_PS(0));
    IFX_REG_W32(0x1F68, IFX_DMA_PCTRL(0));
    IFX_REG_W32(0, IFX_DMA_IRNEN);  // disable all DMA interrupt

    for ( k = 0; k < 8; k++ ) {
        unsigned int imap[8] = {0, 2, 4, 6, 20, 21, 22, 23};

        i = imap[k];
        IFX_REG_W32(i, IFX_DMA_CS(0));
        IFX_REG_W32_MASK(0, 2, IFX_DMA_CCTRL(0));       //  reset channel
        while ( (IFX_REG_R32(IFX_DMA_CCTRL(0)) & 2) );  //  wait until reset finish
        IFX_REG_W32(0, IFX_DMA_CIE(0));                 //  disable channel interrupt
        IFX_REG_W32(1, IFX_DMA_CDLEN(0));               //  only 1 descriptor
        IFX_REG_W32(CPHYSADDR((unsigned int)desc_base), IFX_DMA_CDBA(0));       //  use local variable (array) as descriptor base address
        desc_base[0] = 0x80000000 | sizeof(g_pkt_base);
        desc_base[1] = CPHYSADDR((unsigned int)g_pkt_base);

        IFX_REG_W32_MASK(0, 1, IFX_DMA_CCTRL(0));       //  start receiving
        while ( 1 ) {
            for ( j = 0; j < 1000 && (desc_base[0] & 0x80000000) != 0; j++ );   //  assume packet can be finished within 1000 loops
            if ( (desc_base[0] & 0x80000000) != 0 )     //  no more packet
                break;
            desc_base[0] = 0x80000000 | sizeof(g_pkt_base);
        }
        IFX_REG_W32_MASK(1, 0, IFX_DMA_CCTRL(0));       //  stop receiving
    }

    if ( (IFX_REG_R32(DMRX_PGCNT) & 0x00FF) != 0 || (IFX_REG_R32(DMRX_PKTCNT) & 0x00FF) != 0 || (IFX_REG_R32(DSRX_PGCNT) & 0x00FF) != 0 )
        prom_printf("%s error: IFX_REG_R32(DMRX_PGCNT) = 0x%08x, IFX_REG_R32(DMRX_PKTCNT) = 0x%08x, IFX_REG_R32(DSRX_PGCNT) = 0x%08x\n", __func__, IFX_REG_R32(DMRX_PGCNT), IFX_REG_R32(DMRX_PKTCNT), IFX_REG_R32(DSRX_PGCNT));
}

static inline void ifx_xbar_fpi_burst_disable(void)
{
    u32 reg;

    reg = IFX_REG_R32(IFX_XBAR_ALWAYS_LAST);
    reg &= ~ IFX_XBAR_FPI_BURST_EN;
    IFX_REG_W32(reg, IFX_XBAR_ALWAYS_LAST);
}

/*
 *  Chip Specific Variable/Function
 */
void __init ifx_chip_setup(void)
{
    u32 chip_id, chip_version;

    ifx_xbar_fpi_burst_disable();
    ifx_dplus_clean();
    // only disable it when PMU is tested 	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
    //ifx_pmu_disable_all_modules();
#else
    //ifx_pmu_disable_all_modules();
#endif

    if (IFX_REG_R32(IFX_MPS_GRX390_CHIPID))
    {
	//printk("hsur: GRX390 detected !!!!!!!!!!!!!! \n");
	ifx_bsp_basic_mps_decrypt = (const void (*)(unsigned int, int))0xbfc00030;}
    else 	
    {	
	//printk("hsur: GRX388 detected !!!!!!!!!!!!!!\n");
    	chip_id = IFX_REG_R32(IFX_MPS_CHIPID);
    	chip_version = IFX_MPS_CHIPID_VERSION_GET(chip_id);

    	if ((chip_version == 1) || (chip_version == 0))
		ifx_bsp_basic_mps_decrypt = (const void (*)(unsigned int, int))0xbfc01918;
    	else 
		ifx_bsp_basic_mps_decrypt = (const void (*)(unsigned int, int))0xbfc012c0;

    } 
}

EXPORT_SYMBOL(ifx_bsp_basic_mps_decrypt);

#ifdef CONFIG_QUALCOMM_AP_PPA
/* ARCADYAN #ARC_RC_SD2#, 2013/6/18, Dino, support of load 9880 cal data from flash */
#ifdef CONFIG_MTD
/*
 * NAND Read API for Calibration data
 *
 */
int
ath_nand_local_read(u_char *cal_part,loff_t from, size_t len,
                size_t *retlen, u_char *buf)
{
        int     ret;
        struct mtd_info *mtd;

        if (!len || !retlen) return (0);

        printk("Reading Flash for Calibraton data from 0x%llx and partition name is %s\n",from,cal_part);

        mtd = get_mtd_device_nm(cal_part);
        if (mtd == ERR_PTR(-ENODEV)) {
                printk("MTD partition doesn't exist \n");
                ret = -EIO;
                return ret;
        }
        ret = mtd->read(mtd, from, len, retlen, buf);
        return ret;
}
EXPORT_SYMBOL(ath_nand_local_read);
#endif
#endif
