/******************************************************************************
**
** FILE NAME    : hn1.c
** PROJECT      : IFX UEIP
** MODULES      : BSP Basic
**
** DATE         : 27 May 2009
** AUTHOR       : Xu Liang
** DESCRIPTION  : source file for VR9
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
** 11 Jan 2011   Kishore 	 Adapted for HN1
*******************************************************************************/



#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif /* AUTOCONF_INCLUDED */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kallsyms.h>

/*
 *  Chip Specific Head File
 */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>

/*
 *  Chip Specific Variable/Function
 */

static inline void __init ifx_xbar_fpi_burst_disable(void)
{
    u32 reg;

    reg = IFX_REG_R32(IFX_XBAR_ALWAYS_LAST);
    reg &= ~ IFX_XBAR_FPI_BURST_EN;
    IFX_REG_W32(reg, IFX_XBAR_ALWAYS_LAST);
}

static inline void __init ifx_xbar_ahb_slave_big_endian(void)
{
    u32 reg;

    reg = IFX_REG_R32(IFX_RCU_AHB_ENDIAN);
    reg |= IFX_RCU_AHB_BE_XBAR_S;
    IFX_REG_W32(reg, IFX_RCU_AHB_ENDIAN);
}


#define IFX_PMU_USIF_ECO_FIX                  ((volatile u32*)(0xBF10220C))

static inline void  __init ifx_pmu_usif_eco_fix(void)
{
    u32 reg;

    reg = IFX_REG_R32(IFX_PMU_USIF_ECO_FIX);
    reg |= 0x40000000;
    IFX_REG_W32(reg, IFX_PMU_USIF_ECO_FIX);
}

/*
 *  Chip Specific Variable/Function
 */
void __init ifx_chip_setup(void)
{
    ifx_xbar_fpi_burst_disable();
    ifx_xbar_ahb_slave_big_endian();
    ifx_pmu_usif_eco_fix();
}

/*
 *  Voice firmware decryt function pointer in Boot Rom
 */
const void (*ifx_bsp_basic_mps_decrypt)(unsigned int addr, int n) = (const void (*)(unsigned int, int))0xbfc01ea4;
EXPORT_SYMBOL(ifx_bsp_basic_mps_decrypt);
