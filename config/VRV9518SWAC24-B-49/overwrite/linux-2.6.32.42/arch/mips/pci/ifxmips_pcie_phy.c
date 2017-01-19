/******************************************************************************
**
** FILE NAME    : ifxmips_pcie_phy.c
** PROJECT      : IFX UEIP for VRX200
** MODULES      : PCIe PHY sub module
**
** DATE         : 14 May 2009
** AUTHOR       : Lei Chuanhua
** DESCRIPTION  : PCIe Root Complex Driver
** COPYRIGHT    :       Copyright (c) 2009
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
** HISTORY
** $Version $Date        $Author         $Comment
** 0.0.1    14 May,2009  Lei Chuanhua    Initial version
*******************************************************************************/
/*!
 \file ifxmips_pcie_phy.c
 \ingroup IFX_PCIE  
 \brief PCIe PHY PLL register programming source file
*/
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/paccess.h>
#include <linux/delay.h>

/* Project header file */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>
#include <asm/ifx/ifx_pmu.h>

#include "ifxmips_pcie_reg.h"
#include "ifxmips_pcie.h"

/* PCIe PDI only supports 16 bit operation */

#define IFX_PCIE_PHY_REG_WRITE16(__addr, __data) \
    ((*(volatile u16 *) (__addr)) = (__data))
    
#define IFX_PCIE_PHY_REG_READ16(__addr)  \
    (*(volatile u16 *) (__addr))

#define IFX_PCIE_PHY_REG16(__addr)   \
    (*(volatile u16 *) (__addr))

#define IFX_PCIE_PHY_REG(__reg, __value, __mask) do { \
    u16 read_data;                                    \
    u16 write_data;                                   \
    read_data = IFX_PCIE_PHY_REG_READ16((__reg));      \
    write_data = (read_data & ((u16)~(__mask))) | (((u16)(__value)) & ((u16)(__mask)));\
    IFX_PCIE_PHY_REG_WRITE16((__reg), write_data);               \
} while (0)

#define IFX_PCIE_PLL_TIMEOUT 1000 /* Tunnable */

//#define IFX_PCI_PHY_REG_DUMP

/* Terry 20140306, SSC mode enabled / disabled by kernel params */
#if 1
int pcie_ssc = 0;
#endif

#ifdef IFX_PCI_PHY_REG_DUMP
static void
pcie_phy_reg_dump(int pcie_port) 
{
    printk("Port %d PLL REGFILE\n", pcie_port_idx_to_rc_port(pcie_port));
    printk("PCIE_PHY_PLL_CTRL1    0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_PLL_CTRL1(pcie_port)));
    printk("PCIE_PHY_PLL_CTRL2    0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_PLL_CTRL2(pcie_port)));
    printk("PCIE_PHY_PLL_CTRL3    0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_PLL_CTRL3(pcie_port)));
    printk("PCIE_PHY_PLL_CTRL4    0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_PLL_CTRL4(pcie_port)));
    printk("PCIE_PHY_PLL_CTRL5    0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_PLL_CTRL5(pcie_port)));
    printk("PCIE_PHY_PLL_CTRL6    0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_PLL_CTRL6(pcie_port)));
    printk("PCIE_PHY_PLL_CTRL7    0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_PLL_CTRL7(pcie_port)));
    printk("PCIE_PHY_PLL_A_CTRL1  0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_PLL_A_CTRL1(pcie_port)));
    printk("PCIE_PHY_PLL_A_CTRL2  0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_PLL_A_CTRL2(pcie_port)));
    printk("PCIE_PHY_PLL_A_CTRL3  0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_PLL_A_CTRL3(pcie_port)));
    printk("PCIE_PHY_PLL_STATUS   0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_PLL_STATUS(pcie_port)));

    printk("TX1 REGFILE\n");
    printk("PCIE_PHY_TX1_CTRL1    0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_TX1_CTRL1(pcie_port)));
    printk("PCIE_PHY_TX1_CTRL2    0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_TX1_CTRL2(pcie_port)));
    printk("PCIE_PHY_TX1_CTRL3    0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_TX1_CTRL3(pcie_port)));
    printk("PCIE_PHY_TX1_A_CTRL1  0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_TX1_A_CTRL1(pcie_port)));
    printk("PCIE_PHY_TX1_A_CTRL2  0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_TX1_A_CTRL2(pcie_port)));
    printk("PCIE_PHY_TX1_MOD1     0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_TX1_MOD1(pcie_port)));
    printk("PCIE_PHY_TX1_MOD2     0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_TX1_MOD2(pcie_port)));
    printk("PCIE_PHY_TX1_MOD3     0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_TX1_MOD3(pcie_port)));

    printk("TX2 REGFILE\n");
    printk("PCIE_PHY_TX2_CTRL1    0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_TX2_CTRL1(pcie_port)));
    printk("PCIE_PHY_TX2_CTRL2    0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_TX2_CTRL2(pcie_port)));
    printk("PCIE_PHY_TX2_A_CTRL1  0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_TX2_A_CTRL1(pcie_port)));
    printk("PCIE_PHY_TX2_A_CTRL2  0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_TX2_A_CTRL2(pcie_port)));
    printk("PCIE_PHY_TX2_MOD1     0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_TX2_MOD1(pcie_port)));
    printk("PCIE_PHY_TX2_MOD2     0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_TX2_MOD2(pcie_port)));
    printk("PCIE_PHY_TX2_MOD3     0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_TX2_MOD3(pcie_port)));

    printk("RX1 REGFILE\n");
    printk("PCIE_PHY_RX1_CTRL1    0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_RX1_CTRL1(pcie_port)));
    printk("PCIE_PHY_RX1_CTRL2    0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_RX1_CTRL2(pcie_port)));
    printk("PCIE_PHY_RX1_CDR      0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_RX1_CDR(pcie_port)));
    printk("PCIE_PHY_RX1_EI       0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_RX1_EI(pcie_port)));
    printk("PCIE_PHY_RX1_A_CTRL   0x%04x\n", IFX_PCIE_PHY_REG16(PCIE_PHY_RX1_A_CTRL(pcie_port)));
}
#endif /* IFX_PCI_PHY_REG_DUMP */

static void
pcie_phy_comm_setup(int pcie_port)
{
    /* PLL Setting */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL1(pcie_port), 0x120e, 0xFFFF);
    /* increase the bias reference voltage */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL2(pcie_port), 0x39D7, 0xFFFF);

    /* Endcnt */
    IFX_PCIE_PHY_REG(PCIE_PHY_RX1_EI(pcie_port), 0x0004, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_RX1_A_CTRL(pcie_port), 0x6803, 0xFFFF);

    /* force */
    IFX_PCIE_PHY_REG(PCIE_PHY_TX1_CTRL1(pcie_port), 0x0008, 0x0008);

    /* predrv_ser_en */
    IFX_PCIE_PHY_REG(PCIE_PHY_TX1_A_CTRL2(pcie_port), 0x0706, 0xFFFF);
    /* ctrl_lim */
    IFX_PCIE_PHY_REG(PCIE_PHY_TX1_CTRL3(pcie_port), 0x1FFF, 0xFFFF);
    /* ctrl */
    IFX_PCIE_PHY_REG(PCIE_PHY_TX1_A_CTRL1(pcie_port), 0x0810, 0xFFFF);
    /* predrv_ser_en */
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_A_CTRL2(pcie_port), 0x4702, 0x7F00);

    /* RTERM*/
    IFX_PCIE_PHY_REG(PCIE_PHY_TX1_CTRL2(pcie_port), 0x2e00, 0xFFFF);

    /* Improved 100MHz clock output  */
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_CTRL2(pcie_port), 0x3096, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_A_CTRL2(pcie_port), 0x4707, 0xFFFF);
    /* Reduced CDR BW to avoid glitches */
    IFX_PCIE_PHY_REG(PCIE_PHY_RX1_CDR(pcie_port), 0x0235, 0xFFFF);
}

static int pcie_ppm_enabled = 0;

static int __init setup_pcie_ppm(char *str)
{
    if (!strcmp(str, "off"))
        pcie_ppm_enabled = 0;
    else if (!strcmp(str, "on"))
        pcie_ppm_enabled = 1;
    else
        return 0;
    return 1;
}
__setup("pcie_ppm=", setup_pcie_ppm);

int pcie_phy_clock_ppm_enabled(void)
{
    return pcie_ppm_enabled;
}

/* Terry 20140306, SSC mode enabled / disabled by kernel params */
#if 1 // def CONFIG_IFX_PCIE_PHY_36MHZ_MODE
static void 
pcie_phy_36mhz_mode_setup(int pcie_port) 
{
    int rc_port = pcie_port_idx_to_rc_port(pcie_port);
    
    IFX_PCIE_PRINT(PCIE_MSG_PHY, "%s pcie_port %d ppm %s enter\n", 
        __func__, rc_port, pcie_ppm_enabled ? "enable" :"disable");
#ifdef IFX_PCI_PHY_REG_DUMP
    IFX_PCIE_PRINT(PCIE_MSG_PHY, "Initial PHY register dump\n");
    pcie_phy_reg_dump(pcie_port);
#endif
    if (pcie_ppm_enabled) {
        int p0_ppm = 400;
        int p1_ppm = -400;
        u16 sdm_val1 = 0x38e4;
        u16 sdm_val2 = 0xee;

            /* en_ext_mmd_div_ratio */
        IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL3(pcie_port), 0x0000, 0xFFFF);

            /* pll_ensdm */
        IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL2(pcie_port), 0x0300, 0xFF00);

            /* en_const_sdm */
            /* mmd */
        IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL3(pcie_port), 0x2900, 0xFFFF);
            
            /* lf_mode */
            IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL2(pcie_port), 0x0000, 0x4000);
            
        if (rc_port == IFX_PCIE_PORT0) {
                if (p0_ppm == 200) {
                    sdm_val1 = 0xaaa9;
                    sdm_val2 = 0xee;
                } 
                else if (p0_ppm == 300) {
                    sdm_val1 = 0xe38d;
                    sdm_val2 = 0xee;
                }
                else if (p0_ppm == 350) {
                    sdm_val1 = 0xffff;
                    sdm_val2 = 0xee;
                }
                else if (p0_ppm == 400) {
                    sdm_val1 = 0x1c70;
                    sdm_val2 = 0xef;
                }
                else if (p0_ppm == 500) {
                  sdm_val1 = 0x5554;
                    sdm_val2 = 0xef;
                }
                else { 
                    /* default 0 ppm */
                    sdm_val1 = 0x38e4;
                    sdm_val2 = 0xee;
                }
            }

        if (rc_port == IFX_PCIE_PORT1) {
                if (p1_ppm == -200) {
                    sdm_val1 = 0xc71b;
                    sdm_val2 = 0xed;
                }
                else if (p1_ppm == -300) {
                    sdm_val1 = 0x8e37;
                    sdm_val2 = 0xed;
                }
                else if (p1_ppm == -350) {
                    sdm_val1 = 0x71c6;
                    sdm_val2 = 0xed;
                }
                else if (p1_ppm == -400) {
                    sdm_val1 = 0x5554;
                    sdm_val2 = 0xed;
                }
                else if (p1_ppm == -500) {
                    sdm_val1 = 0x1c70;
                    sdm_val2 = 0xed;
                }
                else{ 
                    /* default 0 ppm */
                    sdm_val1 = 0x38e4;
                    sdm_val2 = 0xee;
                }
            }
          /* const_sdm */
          IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL1(pcie_port), sdm_val1, 0xFFFF);
          /* const sdm */
          IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL2(pcie_port), sdm_val2, 0x00FF);
          
          /* pllmod */
          IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL7(pcie_port), 0x0002, 0xFFFF);
          IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL6(pcie_port), 0x3a04, 0xFFFF);
          IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL5(pcie_port), 0xfae3, 0xFFFF);
          IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL4(pcie_port), 0x1b72, 0xFFFF);
    }
    else {
        /* en_ext_mmd_div_ratio */
        IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL3(pcie_port), 0x0000, 0xFFFF);
        /* const sdm */
        IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL2(pcie_port), 0x03ee, 0xFFFF);

        /* mmd */
        IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL3(pcie_port), 0x2900, 0xFFFF);

        /* lf_mode */
        IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL2(pcie_port), 0x0000, 0x4000);
        /* const_sdm */
        IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL1(pcie_port), 0x38e4, 0xFFFF);
        /* pllmod */
        IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL7(pcie_port), 0x0002, 0xFFFF);
        IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL6(pcie_port), 0x3a04, 0xFFFF);
        IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL5(pcie_port), 0xfae3, 0xFFFF);
        IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL4(pcie_port), 0x1b72, 0xFFFF);
    }
    IFX_PCIE_PRINT(PCIE_MSG_PHY, "%s pcie_port %d ppm %s exit\n",
        __func__, rc_port,  pcie_ppm_enabled ? "enable" :"disable");
}
#endif /* CONFIG_IFX_PCIE_PHY_36MHZ_MODE */

#if 1 // def CONFIG_IFX_PCIE_PHY_36MHZ_SSC_MODE
static void 
pcie_phy_36mhz_ssc_mode_setup(int pcie_port) 
{
    IFX_PCIE_PRINT(PCIE_MSG_PHY, "%s pcie_port %d enter\n",
        __func__, pcie_port_idx_to_rc_port(pcie_port););
#ifdef IFX_PCI_PHY_REG_DUMP
    IFX_PCIE_PRINT(PCIE_MSG_PHY, "Initial PHY register dump\n");
    pcie_phy_reg_dump(pcie_port);
#endif

    /* Increase the bias reference voltage */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL2(pcie_port), 0x39D7, 0xFFFF);

    /* Endcnt */
    IFX_PCIE_PHY_REG(PCIE_PHY_RX1_EI(pcie_port), 0x0004, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_RX1_A_CTRL(pcie_port), 0x6803, 0xFFFF);

    /* Force */
    IFX_PCIE_PHY_REG(PCIE_PHY_TX1_CTRL1(pcie_port), 0x0008, 0x0008);

    /* Predrv_ser_en */
    IFX_PCIE_PHY_REG(PCIE_PHY_TX1_A_CTRL2(pcie_port), 0x0706, 0xFFFF);

    /* ctrl_lim */
    IFX_PCIE_PHY_REG(PCIE_PHY_TX1_CTRL3(pcie_port), 0x1FFF, 0xFFFF);

    /* ctrl */
    IFX_PCIE_PHY_REG(PCIE_PHY_TX1_A_CTRL1(pcie_port), 0x0810, 0xFFFF);

    /* predrv_ser_en */
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_A_CTRL2(pcie_port), 0x4702, 0x7F00);

    /* RTERM*/
    IFX_PCIE_PHY_REG(PCIE_PHY_TX1_CTRL2(pcie_port), 0x2e00, 0xFFFF);

    /* en_ext_mmd_div_ratio */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL3(pcie_port), 0x0000, 0xFFFF);
    /* const sdm */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL2(pcie_port), 0x06ee, 0xFFFF);
    /* ext_mmd_div_ratio*/

    /* pll_ensdm */
    /* mmd */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL3(pcie_port), 0x2900, 0xFFFF);

    /* lf_mode */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL2(pcie_port), 0x0000, 0x4000);

    /* const_sdm */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL1(pcie_port), 0x38e4, 0xFFFF);


    /* pllmod */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL7(pcie_port), 0x0002, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL6(pcie_port), 0x3a04, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL5(pcie_port), 0xfae3, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL4(pcie_port), 0x1c72, 0xFFFF);

    /* improved 100MHz clock output  */
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_CTRL2(pcie_port), 0x3096, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_A_CTRL2(pcie_port), 0x4707, 0xFFFF);

    /* reduced CDR BW to avoid glitches */
    IFX_PCIE_PHY_REG(PCIE_PHY_RX1_CDR(pcie_port), 0x0235, 0xFFFF);
    
    IFX_PCIE_PRINT(PCIE_MSG_PHY, "%s pcie_port %d exit\n",
        __func__, pcie_port_idx_to_rc_port(pcie_port););
}
#endif /* CONFIG_IFX_PCIE_PHY_36MHZ_SSC_MODE */

#ifdef CONFIG_IFX_PCIE_PHY_25MHZ_MODE
static void 
pcie_phy_25mhz_mode_setup(int pcie_port) 
{
    IFX_PCIE_PRINT(PCIE_MSG_PHY, "%s pcie_port %d enter\n",
        __func__, pcie_port_idx_to_rc_port(pcie_port););
#ifdef IFX_PCI_PHY_REG_DUMP
    IFX_PCIE_PRINT(PCIE_MSG_PHY, "Initial PHY register dump\n");
    pcie_phy_reg_dump(pcie_port);
#endif
    /* en_const_sdm */

    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL1(pcie_port), 0x130e, 0xFFFF);

    /* en_ext_mmd_div_ratio*/
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL3(pcie_port), 0x0042, 0xFFFF);

    /* pll_ensdm */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL2(pcie_port), 0x0183, 0x0200);

    /* ext_mmd_div_ratio*/

    /* mmd */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL3(pcie_port), 0x6900, 0xFFFF);

    /* lf_mode */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL2(pcie_port), 0x4000, 0x4000);

    IFX_PCIE_PRINT(PCIE_MSG_PHY, "%s pcie_port %d exit\n",
        __func__, pcie_port_idx_to_rc_port(pcie_port););
}
#endif /* CONFIG_IFX_PCIE_PHY_25MHZ_MODE */

#ifdef CONFIG_IFX_PCIE_PHY_25MHZ_SSC_MODE
static void 
pcie_phy_25mhz_ssc_mode_setup(int pcie_port) 
{
    IFX_PCIE_PRINT(PCIE_MSG_PHY, "%s pcie_port %d enter\n",
        __func__, pcie_port_idx_to_rc_port(pcie_port););
#ifdef IFX_PCI_PHY_REG_DUMP
    IFX_PCIE_PRINT(PCIE_MSG_PHY, "Initial PHY register dump\n");
    pcie_phy_reg_dump(pcie_port);
#endif 
    /* en_const_sdm */
   /* pll_wavegen*/    
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL2(pcie_port), 0x0683, 0x0400);

     /* en_ext_mmd_div_ratio */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL3(pcie_port), 0x0040, 0xFFFF);

    /* ext_mmd_div_ratio*/

    /* mmd */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL3(pcie_port), 0x6900, 0xFFFF);

    /* lf_mode */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL2(pcie_port), 0x4000, 0x4000);

    /* pll_mod */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL7(pcie_port), 0x0001, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL6(pcie_port), 0x8C0A, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL5(pcie_port), 0x52F0, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL4(pcie_port), 0x0000, 0xFFFF);

    IFX_PCIE_PRINT(PCIE_MSG_PHY, "%s pcie_port %d exit\n",
        __func__, pcie_port_idx_to_rc_port(pcie_port););
}
#endif /* CONFIG_IFX_PCIE_PHY_25MHZ_SSC_MODE */

#ifdef CONFIG_IFX_PCIE_PHY_100MHZ_MODE
static void 
pcie_phy_100mhz_mode_setup(int pcie_port) 
{
    IFX_PCIE_PRINT(PCIE_MSG_PHY, "%s pcie_port %d enter\n",
        __func__, pcie_port_idx_to_rc_port(pcie_port););
#ifdef IFX_PCI_PHY_REG_DUMP
    IFX_PCIE_PRINT(PCIE_MSG_PHY, "Initial PHY register dump\n");
    pcie_phy_reg_dump(pcie_port);
#endif 

     /* en_ext_mmd_div_ratio */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL3(pcie_port), 0x0000, 0xFFFF);
    /* const sdm */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL2(pcie_port), 0x00ee, 0xFFFF);
    /* ext_mmd_div_ratio*/
    /* pll_ensdm */
    /* mmd */
    /* ref_clk_sel */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL3(pcie_port), 0x8B00, 0xFFFF);

    /* lf_mode */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_A_CTRL2(pcie_port), 0x4000, 0x4000);

    /* const_sdm */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL1(pcie_port), 0x38e4, 0xFFFF);

    /* pllmod */
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL7(pcie_port), 0x0002, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL6(pcie_port), 0x3a04, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL5(pcie_port), 0xfae3, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_PLL_CTRL4(pcie_port), 0x1b72, 0xFFFF);
    
    /* Tx2 settings required to receive the external differential clock */
    /* rxck_en =1; pd =1; */
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_A_CTRL1(pcie_port), 0x4000, 0x4000); // rxclken =1
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_CTRL1(pcie_port), 0x0000, 0x0001); // pd =1
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_CTRL1(pcie_port), 0x0004, 0x0004); // ei_en =1
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_A_CTRL1(pcie_port), 0x1000, 0x1000); // predrv_byp_ser_i =1
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_A_CTRL2(pcie_port), 0x8000, 0x8000); // ser_en =0 (register is inverted)
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_A_CTRL1(pcie_port), 0x0000, 0x0001); // clk_byp_en =0
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_A_CTRL1(pcie_port), 0x0000, 0x0010); // diffout_en =0 
    IFX_PCIE_PRINT(PCIE_MSG_PHY, "%s pcie_port %d exit\n",
        __func__, pcie_port_idx_to_rc_port(pcie_port););
}
#endif /* CONFIG_IFX_PCIE_PHY_100MHZ_MODE */

static int
pcie_phy_wait_startup_ready(int pcie_port)
{
    int i;

    for (i = 0; i < IFX_PCIE_PLL_TIMEOUT; i++) {
        if ((IFX_PCIE_PHY_REG16(PCIE_PHY_PLL_STATUS(pcie_port)) & 0x0070) == 0x0070) {
            break;
        }
        udelay(10);
    }
    if (i >= IFX_PCIE_PLL_TIMEOUT) {
        printk(KERN_ERR "%s port %d PLL Link timeout\n", __func__, pcie_port_idx_to_rc_port(pcie_port));
        return -1;
    }
    return 0;
}

static void 
pcie_phy_load_enable(int pcie_port, int slice) 
{
    /* Set the load_en of tx/rx slice to '1' */
    switch (slice) {
        case 1:
            IFX_PCIE_PHY_REG(PCIE_PHY_TX1_CTRL1(pcie_port), 0x0010, 0x0010);
            break;
        case 2:
            IFX_PCIE_PHY_REG(PCIE_PHY_TX2_CTRL1(pcie_port), 0x0010, 0x0010);
            break;
        case 3:
            IFX_PCIE_PHY_REG(PCIE_PHY_RX1_CTRL1(pcie_port), 0x0002, 0x0002);
            break;
    }
}

static void 
pcie_phy_load_disable(int pcie_port, int slice) 
{ 
    /* set the load_en of tx/rx slice to '0' */ 
    switch (slice) {
        case 1:
            IFX_PCIE_PHY_REG(PCIE_PHY_TX1_CTRL1(pcie_port), 0x0000, 0x0010);
            break;
        case 2:
            IFX_PCIE_PHY_REG(PCIE_PHY_TX2_CTRL1(pcie_port), 0x0000, 0x0010);
            break;
        case 3: 
            IFX_PCIE_PHY_REG(PCIE_PHY_RX1_CTRL1(pcie_port), 0x0000, 0x0002);
            break;
    }
}

static void 
pcie_phy_load_war(int pcie_port)
{
    int slice;

    for (slice = 1; slice < 4; slice++) {
        pcie_phy_load_enable(pcie_port, slice);
        udelay(1);
        pcie_phy_load_disable(pcie_port, slice);
    }
}

static void 
pcie_phy_tx2_modulation(int pcie_port)
{
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_MOD1(pcie_port), 0x1FFE, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_MOD2(pcie_port), 0xFFFE, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_MOD3(pcie_port), 0x0601, 0xFFFF);
    mdelay(1);
    IFX_PCIE_PHY_REG(PCIE_PHY_TX2_MOD3(pcie_port), 0x0001, 0xFFFF);
}

static void 
pcie_phy_tx1_modulation(int pcie_port)
{
    IFX_PCIE_PHY_REG(PCIE_PHY_TX1_MOD1(pcie_port), 0x1FFE, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_TX1_MOD2(pcie_port), 0xFFFE, 0xFFFF);
    IFX_PCIE_PHY_REG(PCIE_PHY_TX1_MOD3(pcie_port), 0x0601, 0xFFFF);
    mdelay(1);
    IFX_PCIE_PHY_REG(PCIE_PHY_TX1_MOD3(pcie_port), 0x0001, 0xFFFF);
}

static void
pcie_phy_tx_modulation_war(int pcie_port)
{
    int i;

#define PCIE_PHY_MODULATION_NUM 5 
    for (i = 0; i < PCIE_PHY_MODULATION_NUM; i++) {
        pcie_phy_tx2_modulation(pcie_port);
        pcie_phy_tx1_modulation(pcie_port);
    }
#undef PCIE_PHY_MODULATION_NUM
}

int
pcie_phy_clock_mode_setup(int pcie_port)
{
    int rc_port = pcie_port_idx_to_rc_port(pcie_port);

    pcie_pdi_big_endian(rc_port);

    /* Enable PDI to access PCIe PHY register */
    pcie_pdi_pmu_enable(rc_port);

    /* Configure PLL and PHY clock */
    pcie_phy_comm_setup(pcie_port);

	/* Terry 20140306, SSC mode enabled / disabled by kernel params */
#if 1
  #if defined(CONFIG_IFX_PCIE_PHY_36MHZ_MODE) || defined(CONFIG_IFX_PCIE_PHY_36MHZ_SSC_MODE)
	if (pcie_ssc == 0)
		pcie_phy_36mhz_mode_setup(pcie_port);
	else
		pcie_phy_36mhz_ssc_mode_setup(pcie_port);
  #elif defined(CONFIG_IFX_PCIE_PHY_25MHZ_MODE)
    pcie_phy_25mhz_mode_setup(pcie_port);
  #elif defined (CONFIG_IFX_PCIE_PHY_100MHZ_MODE)
    pcie_phy_100mhz_mode_setup(pcie_port);
  #else
  #error "PCIE PHY Clock Mode must be chosen first!!!!"
  #endif /* CONFIG_IFX_PCIE_PHY_36MHZ_MODE */
#else
#ifdef CONFIG_IFX_PCIE_PHY_36MHZ_MODE
    pcie_phy_36mhz_mode_setup(pcie_port);
#elif defined(CONFIG_IFX_PCIE_PHY_36MHZ_SSC_MODE)
    pcie_phy_36mhz_ssc_mode_setup(pcie_port);
#elif defined(CONFIG_IFX_PCIE_PHY_25MHZ_MODE)
    pcie_phy_25mhz_mode_setup(pcie_port);
#elif defined (CONFIG_IFX_PCIE_PHY_100MHZ_MODE)
    pcie_phy_100mhz_mode_setup(pcie_port);
#else
    #error "PCIE PHY Clock Mode must be chosen first!!!!"
#endif /* CONFIG_IFX_PCIE_PHY_36MHZ_MODE */
#endif
    /* Enable PCIe PHY and make PLL setting take effect */
    pcie_phy_pmu_enable(rc_port);
    /* Check if we are in startup_ready status */
    if (pcie_phy_wait_startup_ready(pcie_port) != 0) {
        return -1;
    }
    pcie_phy_load_war(pcie_port);

    /* Apply TX modulation workarounds */
    pcie_phy_tx_modulation_war(pcie_port);

#ifdef IFX_PCI_PHY_REG_DUMP
    IFX_PCIE_PRINT(PCIE_MSG_PHY, "Modified PHY register dump\n");
    pcie_phy_reg_dump(pcie_port);
#endif
    return 0;
}

static int __init get_pcie_ssc(char *str)
{
	int retval;
	retval = get_option(&str, &pcie_ssc);
	return 1;
}

/* Terry 20140306, SSC mode enabled / disabled by kernel params */
#if 1
static int __init pcie_platform_init(void)
{
	printk("[%s] pcie_ssc %d\n", __FUNCTION__, pcie_ssc);

	return 0;
}

__setup("pcie_ssc=", get_pcie_ssc);
arch_initcall(pcie_platform_init);
#endif
