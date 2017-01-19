/******************************************************************************
**
** FILE NAME    : hn1.h
** MODULES      : BSP Basic
**
** DATE         : 11 Jan 2011
** AUTHOR       : Kishore Kankipati
** DESCRIPTION  : header file for HN1
** COPYRIGHT    :       Copyright (c) 2009
**                      Lantiq Deutschland GmbH
**                      Am Campeon 3, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author         $Comment
** 11 Jan 2011   Kishore 	First version for HN1 derived from VR9
** 11 Feb 2011   Yinglei     	Modified for HN1
*******************************************************************************/



#ifndef HN1_H
#define HN1_H

#include <asm/bootinfo.h>

#define MACH_GROUP_IFX MACH_GROUP_HN1
#define MACH_TYPE_IFX  MACH_HN1


/***********************************************************************/
/*  Module      :  WDT register address and bits                       */
/***********************************************************************/

#define IFX_WDT                                 (KSEG1 | 0x1F880000)

/***Watchdog Timer Control Register ***/
#define IFX_WDT_CR                              ((volatile u32*)(IFX_WDT + 0x03F0))
#define IFX_WDT_CR_GEN                          (1 << 31)
#define IFX_WDT_CR_DSEN                         (1 << 30)
#define IFX_WDT_CR_LPEN                         (1 << 29)
#define IFX_WDT_CR_PWL_GET(value)               (((value) >> 26) & ((1 << 2) - 1))
#define IFX_WDT_CR_PWL_SET(value)               (((( 1 << 2) - 1) & (value)) << 26)
#define IFX_WDT_CR_CLKDIV_GET(value)            (((value) >> 24) & ((1 << 2) - 1))
#define IFX_WDT_CR_CLKDIV_SET(value)            (((( 1 << 2) - 1) & (value)) << 24)
#define IFX_WDT_CR_PW_GET(value)                (((value) >> 16) & ((1 << 8) - 1))
#define IFX_WDT_CR_PW_SET(value)                (((( 1 << 8) - 1) & (value)) << 16)
#define IFX_WDT_CR_RELOAD_GET(value)            (((value) >> 0) & ((1 << 16) - 1))
#define IFX_WDT_CR_RELOAD_SET(value)            (((( 1 << 16) - 1) & (value)) << 0)

/***Watchdog Timer Status Register***/
#define IFX_WDT_SR                              ((volatile u32*)(IFX_WDT + 0x03F8))
#define IFX_WDT_SR_EN                           (1 << 31)
#define IFX_WDT_SR_AE                           (1 << 30)
#define IFX_WDT_SR_PRW                          (1 << 29)
#define IFX_WDT_SR_EXP                          (1 << 28)
#define IFX_WDT_SR_PWD                          (1 << 27)
#define IFX_WDT_SR_DS                           (1 << 26)
#define IFX_WDT_SR_VALUE_GET(value)             (((value) >> 0) & ((1 << 16) - 1))
#define IFX_WDT_SR_VALUE_SET(value)             (((( 1 << 16) - 1) & (value)) << 0)


/***********************************************************************/
/*  Module      :  RCU register address and bits                       */
/***********************************************************************/

#define IFX_RCU                                 (KSEG1 | 0x1F203000)

/* Reset Request Register */
#define IFX_RCU_RST_REQ                         ((volatile u32*)(IFX_RCU + 0x0010))
#define IFX_RCU_RST_REQ_HOT_RST                 0x00000001    /* Hot reset, domain 0*/

#define IFX_RCU_RST_STAT                        ((volatile u32*)(IFX_RCU + 0x0014))
#define IFX_RCU_GPIO_STRAP                      ((volatile u32*)(IFX_RCU + 0x001C))
#define IFX_RCU_GPHY0_FW_ADDR                   ((volatile u32*)(IFX_RCU + 0x0020))
#define IFX_RCU_SLIC_USB_RST_STAT               ((volatile u32*)(IFX_RCU + 0x0024))
#define IFX_RCU_PCIE_PHY_CON_STAT               ((volatile u32*)(IFX_RCU + 0x0030))
#define IFX_RCU_GPHY01_MDIO_ADD                 ((volatile u32*)(IFX_RCU + 0x0044))
#define IFX_RCU_GPHY0_RST_REQ                   ((volatile u32*)(IFX_RCU + 0x0048))

/* AHB Endian Register */
#define IFX_RCU_AHB_ENDIAN                      ((volatile u32*)(IFX_RCU + 0x004C))

#define IFX_RCU_AHB_BE_PCIE_M                    0x00000001  /* Configure AHB master port that connects to PCIe RC in big endian */
#define IFX_RCU_AHB_BE_XBAR_M                    0x00000002  /* Configure AHB master port that connects to XBAR in big endian */
#define IFX_RCU_AHB_BE_USIF                      0x00000004  /* Configure AHB slave port that connects to USIF in big endian */
#define IFX_RCU_AHB_BE_XBAR_S                    0x00000008  /* Configure AHB slave port that connects to XBAR in big endian */
#define IFX_RCU_AHB_BE_PCIE_S                    0x00000010  /* Configure AHB slave port that connects to PCIe RC in little endian */
#define IFX_RCU_AHB_BE_PCIE_DBI                  0x00000020  /* Configure DBI module in big endian*/
#define IFX_RCU_AHB_BE_DC_PDI                    0x00000040  /* Configure DC PDI module in big endian*/
#define IFX_RCU_AHB_BE_PCIE_PDI                  0x00000080  /* Configure PCIE PDI module in big endian*/

#define IFX_RCU_CPU_CFG                         ((volatile u32*)(IFX_RCU + 0x0060))

/* Reset Request Register */
#define IFX_RCU_RST_REQ_GPHY0                   (1 << 31)
#define IFX_RCU_RST_REQ_SRST                    (1 << 30)
#define IFX_RCU_RST_REQ_GPHY1                   (1 << 29)
#define IFX_RCU_RST_REQ_MIPS0                   (1 << 1)

/* CPU0, CPU1, CPUSUB, HRST, WDT0, WDT1, DMA, ETHPHY1, ETHPHY0 */
#define IFX_RCU_RST_REQ_ALL                     (IFX_RCU_RST_REQ_SRST | IFX_RCU_RST_REQ_GPHY0 | IFX_RCU_RST_REQ_GPHY1 | IFX_RCU_RST_REQ_MIPS0)

#define IFX_RCU_RST_REQ_DFE                     (1 << 7)
#define IFX_RCU_RST_REQ_AFE                     (1 << 11)
#define IFX_RCU_RST_REQ_ARC_JTAG                (1 << 20)


/***********************************************************************/
/*  Module      :  BCU  register address and bits                      */
/***********************************************************************/

#define IFX_BCU_BASE_ADDR                       (KSEG1 | 0x1E100000)
#define IFX_SLAVE_BCU_BASE_ADDR                 (KSEG1 | 0x1E000000)

/***BCU Control Register (0010H)***/
#define IFX_BCU_CON                             ((volatile u32*)(0x0010 + IFX_BCU_BASE_ADDR))
#define IFX_SLAVE_BCU_CON                       ((volatile u32*)(0x0010 + IFX_SLAVE_BCU_BASE_ADDR))
#define IFX_BCU_STARVATION_MASK                 (0xFF << 24)
#define IFX_BCU_STARVATION_SHIFT                24
#define IFX_BCU_TOUT_MASK                       0xFFFF
#define IFX_BCU_CON_SPC(value)                  (((( 1 << 8) - 1) & (value)) << 24)
#define IFX_BCU_CON_SPE                         (1 << 19)
#define IFX_BCU_CON_PSE                         (1 << 18)
#define IFX_BCU_CON_DBG                         (1 << 16)
#define IFX_BCU_CON_TOUT(value)                 (((( 1 << 16) - 1) & (value)) << 0)

/***BCU Error Control Capture Register (0020H)***/
#define IFX_BCU_ECON                            ((volatile u32*)(0x0020 + IFX_BCU_BASE_ADDR))
#define IFX_SLAVE_BCU_ECON                      ((volatile u32*)(0x0020 + IFX_SLAVE_BCU_BASE_ADDR))
#define IFX_BCU_ECON_TAG(value)                 (((( 1 << 4) - 1) & (value)) << 24)
#define IFX_BCU_ECON_RDN                        (1 << 23)
#define IFX_BCU_ECON_WRN                        (1 << 22)
#define IFX_BCU_ECON_SVM                        (1 << 21)
#define IFX_BCU_ECON_ACK(value)                 (((( 1 << 2) - 1) & (value)) << 19)
#define IFX_BCU_ECON_ABT                        (1 << 18)
#define IFX_BCU_ECON_RDY                        (1 << 17)
#define IFX_BCU_ECON_TOUT                       (1 << 16)
#define IFX_BCU_ECON_ERRCNT(value)              (((( 1 << 16) - 1) & (value)) << 0)
#define IFX_BCU_ECON_OPC(value)                 (((( 1 << 4) - 1) & (value)) << 28)

/***BCU Error Address Capture Register (0024 H)***/
#define IFX_BCU_EADD                            ((volatile u32*)(0x0024 + IFX_BCU_BASE_ADDR))
#define IFX_SLAVE_BCU_EADD                      ((volatile u32*)(0x0024 + IFX_SLAVE_BCU_BASE_ADDR))

/***BCU Error Data Capture Register (0028H)***/
#define IFX_BCU_EDAT                            ((volatile u32*)(0x0028 + IFX_BCU_BASE_ADDR))
#define IFX_SLAVE_BCU_EDAT                      ((volatile u32*)(0x0028 + IFX_SLAVE_BCU_BASE_ADDR))
#define IFX_BCU_IRNEN                           ((volatile u32*)(0x00F4 + IFX_BCU_BASE_ADDR))
#define IFX_SLAVE_BCU_IRNEN                     ((volatile u32*)(0x00F4 + IFX_SLAVE_BCU_BASE_ADDR))
#define IFX_BCU_IRNICR                          ((volatile u32*)(0x00F8 + IFX_BCU_BASE_ADDR))
#define IFX_SLAVE_BCU_IRNICR                    ((volatile u32*)(0x00F8 + IFX_SLAVE_BCU_BASE_ADDR))
#define IFX_BCU_IRNCR                           ((volatile u32*)(0x00FC + IFX_BCU_BASE_ADDR))
#define IFX_SLAVE_BCU_IRNCR                     ((volatile u32*)(0x00FC + IFX_SLAVE_BCU_BASE_ADDR))


/***********************************************************************/
/*  Module      :  HSNAND register address and bits                    */
/***********************************************************************/
#define IFX_HSNAND_BASE				(KSEG1 | 0x1E100400)

/****** HSNAND REGISTERS *******/
#define IFX_NDAC_CTL1				((volatile u32*)(0x0010 + IFX_HSNAND_BASE))
#define IFX_NDAC_CTL2				((volatile u32*)(0x0014 + IFX_HSNAND_BASE))
#define IFX_BASE_A				((volatile u32*)(0x0018 + IFX_HSNAND_BASE))
#define IFX_RX_CNT				((volatile u32*)(0x001C + IFX_HSNAND_BASE))
#define IFX_DPLUS_CTRL				((volatile u32*)(0x0020 + IFX_HSNAND_BASE))
#define IFX_HSNAND_INTR_MASK_CTRL		((volatile u32*)(0x0024 + IFX_HSNAND_BASE))
#define IFX_HSNAND_INTR_STAT			((volatile u32*)(0x0028 + IFX_HSNAND_BASE))
#define IFX_HSMD_CTRL				((volatile u32*)(0x0030 + IFX_HSNAND_BASE))
#define IFX_CS_BASE_A				((volatile u32*)(0x0034 + IFX_HSNAND_BASE))
#define IFX_NAND_INFO				((volatile u32*)(0X0038 + IFX_HSNAND_BASE))

#define IFX_HSNAND_CE_SEL			(0xF<<3)
#define IFX_HSNAND_CE_SEL_S			3
#define IFX_HSNAND_CE_SEL_NONE			0
#define IFX_HSNAND_CE_SEL_CS0			1
#define IFX_HSNAND_CE_SEL_CS1			2
#define IFX_HSNAND_CE_SEL_CS2 			4
#define IFX_HSNAND_CE_SEL_CS3  			8

#define IFX_HSNAND_FSM				(1<<2)
#define IFX_HSNAND_FSM_S			2
enum {
    IFX_HSNAND_FSM_DISABLED = 0,
    IFX_HSNAND_FSM_ENABLED,
};

#define IFX_HSNAND_ENR				(3<<0)
#define IFX_HSNAND_ENR_S			0
enum {
    IFX_HSNAND_ENR_XIP = 0,
    IFX_HSNAND_ENR_HSDMA,
    IFX_HSNAND_ENR_IO,
    IFX_HSNAND_ENR_NONE
};

#define IFX_HSNAND_XFER_SEL			(7<<0)
#define IFX_HSNAND_XFER_SEL_S			7
enum {
    IFX_HSNAND_NO_XFER = 0,
    IFX_HSNAND_START_XFER
};

/***********************************************************************/
/*  Module      :  GPIO register address and bits                      */
/***********************************************************************/

#define IFX_GPIO                                (KSEG1 | 0x1E100B00)

/***Port 0 Data Output Register (0010H)***/
#define IFX_GPIO_P0_OUT                         ((volatile u32 *)(IFX_GPIO + 0x0010))
/***Port 1 Data Output Register (0040H)***/
#define IFX_GPIO_P1_OUT                         ((volatile u32 *)(IFX_GPIO + 0x0040))
/***Port 2 Data Output Register (0070H)***/
#define IFX_GPIO_P2_OUT                         ((volatile u32 *)(IFX_GPIO + 0x0070))
/***Port 3 Data Output Register (00A0H)***/
#define IFX_GPIO_P3_OUT                         ((volatile u32 *)(IFX_GPIO + 0x00A0))
/***Port 0 Data Input Register (0014H)***/
#define IFX_GPIO_P0_IN                          ((volatile u32 *)(IFX_GPIO + 0x0014))
/***Port 1 Data Input Register (0044H)***/
#define IFX_GPIO_P1_IN                          ((volatile u32 *)(IFX_GPIO + 0x0044))
/***Port 2 Data Input Register (0074H)***/
#define IFX_GPIO_P2_IN                          ((volatile u32 *)(IFX_GPIO + 0x0074))
/***Port 3 Data Input Register (00A4H)***/
#define IFX_GPIO_P3_IN                          ((volatile u32 *)(IFX_GPIO + 0x00A4))
/***Port 0 Direction Register (0018H)***/
#define IFX_GPIO_P0_DIR                         ((volatile u32 *)(IFX_GPIO + 0x0018))
/***Port 1 Direction Register (0048H)***/
#define IFX_GPIO_P1_DIR                         ((volatile u32 *)(IFX_GPIO + 0x0048))
/***Port 2 Direction Register (0078H)***/
#define IFX_GPIO_P2_DIR                         ((volatile u32 *)(IFX_GPIO + 0x0078))
/***Port 3 Direction Register (0048H)***/
#define IFX_GPIO_P3_DIR                         ((volatile u32 *)(IFX_GPIO + 0x00A8))
/***Port 0 Alternate Function Select Register 0 (001C H) ***/
#define IFX_GPIO_P0_ALTSEL0                     ((volatile u32 *)(IFX_GPIO + 0x001C))
/***Port 1 Alternate Function Select Register 0 (004C H) ***/
#define IFX_GPIO_P1_ALTSEL0                     ((volatile u32 *)(IFX_GPIO + 0x004C))
/***Port 2 Alternate Function Select Register 0 (007C H) ***/
#define IFX_GPIO_P2_ALTSEL0                     ((volatile u32 *)(IFX_GPIO + 0x007C))
/***Port 3 Alternate Function Select Register 0 (00AC H) ***/
#define IFX_GPIO_P3_ALTSEL0                     ((volatile u32 *)(IFX_GPIO + 0x00AC))
/***Port 0 Alternate Function Select Register 1 (0020 H) ***/
#define IFX_GPIO_P0_ALTSEL1                     ((volatile u32 *)(IFX_GPIO + 0x0020))
/***Port 1 Alternate Function Select Register 0 (0050 H) ***/
#define IFX_GPIO_P1_ALTSEL1                     ((volatile u32 *)(IFX_GPIO + 0x0050))
/***Port 2 Alternate Function Select Register 0 (0080 H) ***/
#define IFX_GPIO_P2_ALTSEL1                     ((volatile u32 *)(IFX_GPIO + 0x0080))
/***Port 3 Alternate Function Select Register 0 (0064 H) ***/
#define IFX_GPIO_P3_ALTSEL1                     ((volatile u32 *)(IFX_GPIO + 0x0064))
/***Port 0 Open Drain Control Register (0024H)***/
#define IFX_GPIO_P0_OD                          ((volatile u32 *)(IFX_GPIO + 0x0024))
/***Port 1 Open Drain Control Register (0054H)***/
#define IFX_GPIO_P1_OD                          ((volatile u32 *)(IFX_GPIO + 0x0054))
/***Port 2 Open Drain Control Register (0084H)***/
#define IFX_GPIO_P2_OD                          ((volatile u32 *)(IFX_GPIO + 0x0084))
/***Port 3 Open Drain Control Register (0034H)***/
#define IFX_GPIO_P3_OD                          ((volatile u32 *)(IFX_GPIO + 0x0034))
/***Port 0 Input Schmitt-Trigger Off Register (0028 H) ***/
#define IFX_GPIO_P0_STOFF                       ((volatile u32 *)(IFX_GPIO + 0x0028))
/***Port 1 Input Schmitt-Trigger Off Register (0058 H) ***/
#define IFX_GPIO_P1_STOFF                       ((volatile u32 *)(IFX_GPIO + 0x0058))
/***Port 2 Input Schmitt-Trigger Off Register (0088 H) ***/
#define IFX_GPIO_P2_STOFF                       ((volatile u32 *)(IFX_GPIO + 0x0088))
/***Port 3 Input Schmitt-Trigger Off Register (0094 H) ***/

/***Port 0 Pull Up/Pull Down Select Register (002C H)***/
#define IFX_GPIO_P0_PUDSEL                      ((volatile u32 *)(IFX_GPIO + 0x002C))
/***Port 1 Pull Up/Pull Down Select Register     (005C H)***/
#define IFX_GPIO_P1_PUDSEL                      ((volatile u32 *)(IFX_GPIO + 0x005C))
/***Port 2 Pull Up/Pull Down Select Register     (008C H)***/
#define IFX_GPIO_P2_PUDSEL                      ((volatile u32 *)(IFX_GPIO + 0x008C))
/***Port 3 Pull Up/Pull Down Select Register     (0038 H)***/
#define IFX_GPIO_P3_PUDSEL                      ((volatile u32 *)(IFX_GPIO + 0x0038))
/***Port 0 Pull Up Device Enable Register (0030 H)***/
#define IFX_GPIO_P0_PUDEN                       ((volatile u32 *)(IFX_GPIO + 0x0030))
/***Port 1 Pull Up Device Enable Register (0060 H)***/
#define IFX_GPIO_P1_PUDEN                       ((volatile u32 *)(IFX_GPIO + 0x0060))
/***Port 2 Pull Up Device Enable Register (0090 H)***/
#define IFX_GPIO_P2_PUDEN                       ((volatile u32 *)(IFX_GPIO + 0x0090))
/***Port 3 Pull Up Device Enable Register (003c H)***/
#define IFX_GPIO_P3_PUDEN                       ((volatile u32 *)(IFX_GPIO + 0x003C))


/***********************************************************************/
/*  Module      :  PMU register address and bits                       */
/***********************************************************************/

#define IFX_PMU                                 (KSEG1 | 0x1F102000)

#define IFX_PMU_PWDCR                           ((volatile u32*)(IFX_PMU + 0x001C))


/***********************************************************************/
/*  Module      :  CGU register address and bits                       */
/***********************************************************************/

#define IFX_CGU                                 (KSEG1 | 0x1F103000)

/***CGU Clock PLL0 ***/
#define IFX_CGU_PLL0_CFG                        ((volatile u32*)(IFX_CGU + 0x0004))
/***CGU Clock PLL1 ***/
#define IFX_CGU_PLL1_CFG                        ((volatile u32*)(IFX_CGU + 0x0008))
/***CGU Clock PLL2 ***/
#define IFX_CGU_PLL2_CFG                        ((volatile u32*)(IFX_CGU + 0x0060))
/***CGU Clock SYS Mux Register***/
#define IFX_CGU_SYS                             ((volatile u32*)(IFX_CGU + 0x000C))
/***CGU CGU Clock Frequency Select Register***/
#define IFX_CGU_CLKFSR                          ((volatile u32*)(IFX_CGU + 0x0010))
/**Update CGU Register***/
#define IFX_CGU_UPDATE                          ((volatile u32*)(IFX_CGU + 0x0020))
/***CGU Interface Clock Control Register***/
#define IFX_CGU_IF_CLK                          ((volatile u32*)(IFX_CGU + 0x0024))

/***********************************************************************/
/*  Module      :  MCD register address and bits                       */
/***********************************************************************/

#define IFX_MCD                                 (KSEG1 | 0x1F106000)

/***Manufacturer Identification Register***/
#define IFX_MCD_MANID                           ((volatile u32*)(IFX_MCD + 0x0024))
#define IFX_MCD_MANID_MANUF(value)              (((( 1 << 11) - 1) & (value)) << 5)

/***Chip Identification Register***/
#define IFX_MCD_CHIPID                          ((volatile u32*)(IFX_MCD + 0x0028))
#define IFX_MCD_CHIPID_VERSION_GET(value)       (((value) >> 28) & ((1 << 4) - 1))
#define IFX_MCD_CHIPID_VERSION_SET(value)       (((( 1 << 4) - 1) & (value)) << 28)
#define IFX_MCD_CHIPID_PART_NUMBER_GET(value)   (((value) >> 12) & ((1 << 16) - 1))
#define IFX_MCD_CHIPID_PART_NUMBER_SET(value)   (((( 1 << 16) - 1) & (value)) << 12)
#define IFX_MCD_CHIPID_MANID_GET(value)         (((value) >> 1) & ((1 << 11) - 1))
#define IFX_MCD_CHIPID_MANID_SET(value)         (((( 1 << 11) - 1) & (value)) << 1)

#define IFX_CHIPID_STANDARD                     0x00EB
#define IFX_CHIPID_YANGTSE                      0x00ED

/***Redesign Tracing Identification Register***/
#define IFX_MCD_RTID                            ((volatile u32*)(IFX_MCD + 0x002C))
#define IFX_MCD_RTID_LC                         (1 << 15)
#define IFX_MCD_RTID_RIX(value)                 (((( 1 << 3) - 1) & (value)) << 0)



/***********************************************************************/
/*  Module      :  EBU register address and bits                       */
/***********************************************************************/

#define IFX_EBU                                 (KSEG1 | 0x1E105300)

/***EBU Clock Control Register***/
#define IFX_EBU_CLC                             ((volatile u32*)(IFX_EBU + 0x0000))
#define IFX_EBU_CLC_DISS                        (1 << 1)
#define IFX_EBU_CLC_DISR                        (1 << 0)

#define IFX_EBU_ID                              ((volatile u32*)(IFX_EBU + 0x0008))

/***EBU Global Control Register***/
#define IFX_EBU_CON                             ((volatile u32*)(IFX_EBU + 0x0010))
#define IFX_EBU_CON_DTACS(value)                (((( 1 << 3) - 1) & (value)) << 20)
#define IFX_EBU_CON_DTARW(value)                (((( 1 << 3) - 1) & (value)) << 16)
#define IFX_EBU_CON_TOUTC(value)                (((( 1 << 8) - 1) & (value)) << 8)
#define IFX_EBU_CON_ARBMODE(value)              (((( 1 << 2) - 1) & (value)) << 6)
#define IFX_EBU_CON_ARBSYNC                     (1 << 5)

/***EBU Address Select Register 0***/
#define IFX_EBU_ADDSEL0                         ((volatile u32*)(IFX_EBU + 0x0020))
#define IFX_EBU_ADDSEL0_BASE(value)             (((( 1 << 20) - 1) & (value)) << 12)
#define IFX_EBU_ADDSEL0_MASK(value)             (((( 1 << 4) - 1) & (value)) << 4)
#define IFX_EBU_ADDSEL0_MIRRORE                 (1 << 1)
#define IFX_EBU_ADDSEL0_REGEN                   (1 << 0)

/***EBU Address Select Register 1***/
#define IFX_EBU_ADDSEL1                         ((volatile u32*)(IFX_EBU + 0x0024))
#define IFX_EBU_ADDSEL1_BASE(value)            (((( 1 << 20) - 1) & (value)) << 12)
#define IFX_EBU_ADDSEL1_MASK(value)            (((( 1 << 4) - 1) & (value)) << 4)
#define IFX_EBU_ADDSEL1_MIRRORE                 (1 << 1)
#define IFX_EBU_ADDSEL1_REGEN                   (1 << 0)

/***EBU Address Select Register 2***/
#define IFX_EBU_ADDSEL2                         ((volatile u32*)(IFX_EBU + 0x0028))
#define IFX_EBU_ADDSEL2_BASE(value)             (((( 1 << 20) - 1) & (value)) << 12)
#define IFX_EBU_ADDSEL2_MASK(value)             (((( 1 << 4) - 1) & (value)) << 4)
#define IFX_EBU_ADDSEL2_MIRRORE                 (1 << 1)
#define IFX_EBU_ADDSEL2_REGEN                   (1 << 0)

/***EBU Address Select Register 3***/
#define IFX_EBU_ADDSEL3                         ((volatile u32*)(IFX_EBU + 0x002C))
#define IFX_EBU_ADDSEL3_BASE(value)             (((( 1 << 20) - 1) & (value)) << 12)
#define IFX_EBU_ADDSEL3_MASK(value)             (((( 1 << 4) - 1) & (value)) << 4)
#define IFX_EBU_ADDSEL3_MIRRORE                 (1 << 1)
#define IFX_EBU_ADDSEL3_REGEN                   (1 << 0)

/***EBU Bus Configuration Register 0***/
#define IFX_EBU_BUSCON0                         ((volatile u32*)(IFX_EBU+ 0x0060))

#define IFX_EBU_BUSCON0_CMULT                   0x00000003
#define IFX_EBU_BUSCON0_CMULT_S                 0
enum {
    IFX_EBU_BUSCON0_CMULT1 = 0,
    IFX_EBU_BUSCON0_CMULT4,
    IFX_EBU_BUSCON0_CMULT8,
    IFX_EBU_BUSCON0_CMULT16,  /* Default after reset */
};

#define IFX_EBU_BUSCON0_RECOVC                  0x00000000c
#define IFX_EBU_BUSCON0_RECOVC_S                2
enum {
    IFX_EBU_BUSCON0_RECOVC0 = 0,
    IFX_EBU_BUSCON0_RECOVC1,
    IFX_EBU_BUSCON0_RECOVC2,
    IFX_EBU_BUSCON0_RECOVC3,  /* Default */
};
#define IFX_EBU_BUSCON0_HOLDC                   0x00000030
#define IFX_EBU_BUSCON0_HOLDC_S                 4
enum {
    IFX_EBU_BUSCON0_HOLDC0 = 0,
    IFX_EBU_BUSCON0_HOLDC1,
    IFX_EBU_BUSCON0_HOLDC2,
    IFX_EBU_BUSCON0_HOLDC3,  /* Default */
};
#define IFX_EBU_BUSCON0_WAITRDC                 0x000000c0
#define IFX_EBU_BUSCON0_WAITRDC_S               6
enum {
    IFX_EBU_BUSCON0_WAITRDC0 = 0,
    IFX_EBU_BUSCON0_WAITRDC1,
    IFX_EBU_BUSCON0_WAITRDC2,
    IFX_EBU_BUSCON0_WAITRDC3,  /* Default */
};
#define IFX_EBU_BUSCON0_WAITWRC                 0x00000700
#define IFX_EBU_BUSCON0_WAITWRC_S               8
enum {
    IFX_EBU_BUSCON0_WAITWRC0 = 0,
    IFX_EBU_BUSCON0_WAITWRC1,
    IFX_EBU_BUSCON0_WAITWRC2,
    IFX_EBU_BUSCON0_WAITWRC3,
    IFX_EBU_BUSCON0_WAITWRC4,
    IFX_EBU_BUSCON0_WAITWRC5,
    IFX_EBU_BUSCON0_WAITWRC6,
    IFX_EBU_BUSCON0_WAITWRC7, /* Default */
};

#define IFX_EBU_BUSCON0_BCGEN                   0x00003000
#define IFX_EBU_BUSCON0_BCGEN_S                 12
enum {
    IFX_EBU_BUSCON0_BCGEN_CS = 0,
    IFX_EBU_BUSCON0_BCGEN_INTEL, /* Default */
    IFX_EBU_BUSCON0_BCGEN_MOTOROLA,
    IFX_EBU_BUSCON0_BCGEN_RES,
};

#define IFX_EBU_BUSCON0_ALEC                    0x0000c000
#define IFX_EBU_BUSCON0_ALEC_S                  14
enum {
    IFX_EBU_BUSCON0_ALEC0 = 0,
    IFX_EBU_BUSCON0_ALEC1,
    IFX_EBU_BUSCON0_ALEC2,
    IFX_EBU_BUSCON0_ALEC3,   /* Default */
};

#define IFX_EBU_BUSCON0_XDM                     0x00030000
#define IFX_EBU_BUSCON0_XDM_S                   16
enum {
    IFX_EBU_BUSCON0_XDM8 = 0,
    IFX_EBU_BUSCON0_XDM16,  /* Default */
};

#define IFX_EBU_BUSCON0_VN_EN                   0x00040000

#define IFX_EBU_BUSCON0_WAITINV_HI              0x00080000 /* low by default */

#define IFX_EBU_BUSCON0_WAIT                    0x00300000
#define IFX_EBU_BUSCON0_WAIT_S                  20
enum {
    IFX_EBU_BUSCON0_WAIT_DISABLE = 0,
    IFX_EBU_BUSCON0_WAIT_ASYNC,
    IFX_EBU_BUSCON0_WAIT_SYNC,
};
#define IFX_EBU_BUSCON0_SETUP_EN                0x00400000 /* Disable by default */

#define IFX_EBU_BUSCON0_AGEN                    0x07000000
#define IFX_EBU_BUSCON0_AGEN_S                  24
enum {
    IFX_EBU_BUSCON0_AGEN_DEMUX = 0, /* Default */
    IFX_EBU_BUSCON0_AGEN_RES,
    IFX_EBU_BUSCON0_AGEN_MUX,
};

#define IFX_EBU_BUSCON0_PG_EN                   0x20000000
#define IFX_EBU_BUSCON0_ADSWP                   0x40000000 /* Disable by default */
#define IFX_EBU_BUSCON0_WRDIS                   0x80000000 /* Disable by default */

/***EBU Bus Configuration Register 1***/
#define IFX_EBU_BUSCON1                         ((volatile u32*)(IFX_EBU + 0x0064))
#define IFX_EBU_BUSCON1_CMULT                   0x00000003
#define IFX_EBU_BUSCON1_CMULT_S                 0
enum {
    IFX_EBU_BUSCON1_CMULT1 = 0,
	IFX_EBU_BUSCON1_CMULT4,
	IFX_EBU_BUSCON1_CMULT8,
	IFX_EBU_BUSCON1_CMULT16,  /* Default after reset */
	 };

#define IFX_EBU_BUSCON1_RECOVC                  0x00000000c
#define IFX_EBU_BUSCON1_RECOVC_S                2
enum {
    IFX_EBU_BUSCON1_RECOVC0 = 0,
    IFX_EBU_BUSCON1_RECOVC1,
    IFX_EBU_BUSCON1_RECOVC2,
    IFX_EBU_BUSCON1_RECOVC3,  /* Default */
     };
#define IFX_EBU_BUSCON1_HOLDC                   0x00000030
#define IFX_EBU_BUSCON1_HOLDC_S                 4
enum {
    IFX_EBU_BUSCON1_HOLDC0 = 0,
    IFX_EBU_BUSCON1_HOLDC1,
    IFX_EBU_BUSCON1_HOLDC2,
    IFX_EBU_BUSCON1_HOLDC3,  /* Default */
	};
#define IFX_EBU_BUSCON1_WAITRDC                 0x000000c0
#define IFX_EBU_BUSCON1_WAITRDC_S               6
enum {
    IFX_EBU_BUSCON1_WAITRDC0 = 0,
    IFX_EBU_BUSCON1_WAITRDC1,
    IFX_EBU_BUSCON1_WAITRDC2,
    IFX_EBU_BUSCON1_WAITRDC3,  /* Default */
	};
#define IFX_EBU_BUSCON1_WAITWRC                 0x00000700
#define IFX_EBU_BUSCON1_WAITWRC_S               8
enum {
    IFX_EBU_BUSCON1_WAITWRC0 = 0,
    IFX_EBU_BUSCON1_WAITWRC1,
    IFX_EBU_BUSCON1_WAITWRC2,
    IFX_EBU_BUSCON1_WAITWRC3,
    IFX_EBU_BUSCON1_WAITWRC4,
    IFX_EBU_BUSCON1_WAITWRC5,
    IFX_EBU_BUSCON1_WAITWRC6,
    IFX_EBU_BUSCON1_WAITWRC7, /* Default */
	};
#define IFX_EBU_BUSCON1_BCGEN                   0x00003000
#define IFX_EBU_BUSCON1_BCGEN_S                 12
enum {
    IFX_EBU_BUSCON1_BCGEN_CS = 0,
    IFX_EBU_BUSCON1_BCGEN_INTEL, /* Default */
    IFX_EBU_BUSCON1_BCGEN_MOTOROLA,
    IFX_EBU_BUSCON1_BCGEN_RES,
     };
#define IFX_EBU_BUSCON1_ALEC                    0x0000c000
#define IFX_EBU_BUSCON1_ALEC_S                  14
enum {
    IFX_EBU_BUSCON1_ALEC0 = 0,
    IFX_EBU_BUSCON1_ALEC1,
    IFX_EBU_BUSCON1_ALEC2,
    IFX_EBU_BUSCON1_ALEC3,   /* Default */
     };

#define IFX_EBU_BUSCON1_SETUP                   (1 << 22)

#define IFX_EBU_BUSCON1_WRDIS                   (1 << 31)
//#define IFX_EBU_BUSCON1_ALEC(value)             (((( 1 << 2) - 1) & (value)) << 29)
//#define IFX_EBU_BUSCON1_BCGEN(value)            (((( 1 << 2) - 1) & (value)) << 27)
//#define IFX_EBU_BUSCON1_AGEN(value)             (((( 1 << 2) - 1) & (value)) << 24)
//#define IFX_EBU_BUSCON1_CMULTR(value)           (((( 1 << 2) - 1) & (value)) << 22)
//#define IFX_EBU_BUSCON1_WAIT(value)             (((( 1 << 2) - 1) & (value)) << 20)
//#define IFX_EBU_BUSCON1_WAITINV                 (1 << 19)
//#define IFX_EBU_BUSCON1_SETUP                   (1 << 18)
//#define IFX_EBU_BUSCON1_PORTW(value)            (((( 1 << 2) - 1) & (value)) << 16)
//#define IFX_EBU_BUSCON1_WAITRDC(value)          (((( 1 << 7) - 1) & (value)) << 9)
//#define IFX_EBU_BUSCON1_WAITWRC(value)          (((( 1 << 3) - 1) & (value)) << 6)
//#define IFX_EBU_BUSCON1_HOLDC(value)            (((( 1 << 2) - 1) & (value)) << 4)
//#define IFX_EBU_BUSCON1_RECOVC(value)           (((( 1 << 2) - 1) & (value)) << 2)
//#define IFX_EBU_BUSCON1_CMULT(value)            (((( 1 << 2) - 1) & (value)) << 0)

/***EBU Bus Configuration Register 2***/
#define IFX_EBU_BUSCON2                         ((volatile u32*)(IFX_EBU + 0x0068))
#define IFX_EBU_BUSCON2_WRDIS                   (1 << 31)
#define IFX_EBU_BUSCON2_ALEC(value)             (((( 1 << 2) - 1) & (value)) << 29)
#define IFX_EBU_BUSCON2_BCGEN(value)            (((( 1 << 2) - 1) & (value)) << 27)
#define IFX_EBU_BUSCON2_AGEN(value)             (((( 1 << 2) - 1) & (value)) << 24)
#define IFX_EBU_BUSCON2_CMULTR(value)           (((( 1 << 2) - 1) & (value)) << 22)
#define IFX_EBU_BUSCON2_WAIT(value)             (((( 1 << 2) - 1) & (value)) << 20)
#define IFX_EBU_BUSCON2_WAITINV                 (1 << 19)
#define IFX_EBU_BUSCON2_SETUP                   (1 << 18)
#define IFX_EBU_BUSCON2_PORTW(value)            (((( 1 << 2) - 1) & (value)) << 16)
#define IFX_EBU_BUSCON2_WAITRDC(value)          (((( 1 << 7) - 1) & (value)) << 9)
#define IFX_EBU_BUSCON2_WAITWRC(value)          (((( 1 << 3) - 1) & (value)) << 6)
#define IFX_EBU_BUSCON2_HOLDC(value)            (((( 1 << 2) - 1) & (value)) << 4)
#define IFX_EBU_BUSCON2_RECOVC(value)           (((( 1 << 2) - 1) & (value)) << 2)
#define IFX_EBU_BUSCON2_CMULT(value)            (((( 1 << 2) - 1) & (value)) << 0)

/***EBU Bus Configuration Register 2***/
#define IFX_EBU_BUSCON3                         ((volatile u32*)(IFX_EBU + 0x006C))
#define IFX_EBU_BUSCON3_WRDIS                   (1 << 31)
#define IFX_EBU_BUSCON3_ADSWP(value)            (1 << 30)
#define IFX_EBU_BUSCON3_PG_EN(value)            (1 << 29)
#define IFX_EBU_BUSCON3_AGEN(value)             (((( 1 << 3) - 1) & (value)) << 24)
#define IFX_EBU_BUSCON3_SETUP                   (1 << 22)
#define IFX_EBU_BUSCON3_WAIT(value)             (((( 1 << 2) - 1) & (value)) << 20)
#define IFX_EBU_BUSCON3_WAITINV                 (1 << 19)
#define IFX_EBU_BUSCON3_VN_EN                   (1 << 18)
#define IFX_EBU_BUSCON3_PORTW(value)            (((( 1 << 2) - 1) & (value)) << 16)
#define IFX_EBU_BUSCON3_ALEC(value)             (((( 1 << 2) - 1) & (value)) << 14)
#define IFX_EBU_BUSCON3_BCGEN(value)            (((( 1 << 2) - 1) & (value)) << 12)
#define IFX_EBU_BUSCON3_WAITWDC(value)          (((( 1 << 4) - 1) & (value)) << 8)
#define IFX_EBU_BUSCON3_WAITRRC(value)          (((( 1 << 2) - 1) & (value)) << 6)
#define IFX_EBU_BUSCON3_HOLDC(value)            (((( 1 << 2) - 1) & (value)) << 4)
#define IFX_EBU_BUSCON3_RECOVC(value)           (((( 1 << 2) - 1) & (value)) << 2)
#define IFX_EBU_BUSCON3_CMULT(value)            (((( 1 << 2) - 1) & (value)) << 0)

#define IFX_EBU_PCC_ISTAT                       IFX_EBU_ECC_ISTAT 
#define IFX_EBU_ECC_ISTAT                       ((volatile u32*)(IFX_EBU+ 0x00A0))
#define IFX_EBU_ECC_IEN                         ((volatile u32*)(IFX_EBU+ 0x00A4))
#define IFX_EBU_ECC_IEN_PCI_EN                  0x00000010

#define IFX_EBU_ECC_INT_OUT                     ((volatile u32*)(IFX_EBU+ 0x00A8))

#define IFX_EBU_NAND_CON                        (volatile u32*)(IFX_EBU + 0xB0)
#define IFX_EBU_NAND_WAIT                       (volatile u32*)(IFX_EBU + 0xB4)
#define IFX_EBU_NAND_ECC0                       (volatile u32*)(IFX_EBU + 0xB8)
#define IFX_EBU_NAND_ECC_AC                     (volatile u32*)(IFX_EBU + 0xBC)
#define IFX_EBU_NAND_ECC_CR			            (volatile u32*)(IFX_EBU + 0xC0)
#define IFX_EBU_SYN_CON1			            (volatile u32*)(IFX_EBU + 0xC4)

#define IFX_EBU_NAND_CON_NANDM                  (1<<0)
#define IFX_EBU_NAND_CON_NANDM_S                 0
enum {
    IFX_EBU_NAND_CON_NANDM_DISABLE = 0,/* Default after reset */
	IFX_EBU_NAND_CON_NANDM_ENABLE,
	 };

#define IFX_EBU_NAND_CON_CSMUX_E                 (1<<1)
#define IFX_EBU_NAND_CON_CSMUX_E_S                 1
enum {
    IFX_EBU_NAND_CON_CSMUX_E_DISABLE = 0,/* Default after reset */
    IFX_EBU_NAND_CON_CSMUX_E_ENABLE,
     };

#define IFX_EBU_NAND_CON_ALE_P			(1<<2)
#define IFX_EBU_NAND_CON_ALE_P_S		2
enum {
    IFX_EBU_NAND_CON_ALE_P_HIGH = 0,
    IFX_EBU_NAND_CON_ALE_P_LOW,
};

#define IFX_EBU_NAND_CON_CLE_P 			(1<<3)
#define IFX_EBU_NAND_CON_CLE_P_S		3
enum {
    IFX_EBU_NAND_CON_CLE_P_HIGH = 0,
    IFX_EBU_NAND_CON_CLE_P_LOW,
};

#define IFX_EBU_NAND_CON_CS_P                   (1<<4)
#define IFX_EBU_NAND_CON_CS_P_S                 4
enum {
    IFX_EBU_NAND_CON_CS_P_HIGH = 0,
    IFX_EBU_NAND_CON_CS_P_LOW,     /* Default after reset */
     };

#define IFX_EBU_NAND_CON_SE_P                   (1<<5)
#define IFX_EBU_NAND_CON_SE_P_S                 5
enum {
    IFX_EBU_NAND_CON_SE_P_HIGH = 0,
    IFX_EBU_NAND_CON_SE_P_LOW,     /* Default after reset */
     };
#define IFX_EBU_NAND_CON_WP_P                   (1<<6)
#define IFX_EBU_NAND_CON_WP_P_S                 6
enum {
    IFX_EBU_NAND_CON_WP_P_HIGH = 0,
	IFX_EBU_NAND_CON_WP_P_LOW,     /* Default after reset */
	 };

#define IFX_EBU_NAND_CON_PRE_P                   (1<<7)
#define IFX_EBU_NAND_CON_PRE_P_S                 7
enum {
    IFX_EBU_NAND_CON_PRE_P_HIGH = 0,
    IFX_EBU_NAND_CON_PRE_P_LOW,     /* Default after reset */
     };

#define IFX_EBU_NAND_CON_IN_CS                   (3<<8)
#define IFX_EBU_NAND_CON_IN_CS_S                 8
enum {
    IFX_EBU_NAND_CON_IN_CS0 = 0,    /* Default after reset */
    IFX_EBU_NAND_CON_IN_CS1,
     };

#define IFX_EBU_NAND_CON_OUT_CS                   (3<<10)
#define IFX_EBU_NAND_CON_OUT_CS_S                 10
enum {
    IFX_EBU_NAND_CON_OUT_CS0 = 0,   /* Default after reset */
    IFX_EBU_NAND_CON_OUT_CS1,
     };

#define IFX_EBU_NAND_CON_ECC			(1<<31)
#define IFX_EBU_NAND_CON_ECC_S			31
enum {
    IFX_EBU_NAND_CON_ECC_OFF = 0,
    IFX_EBU_NAND_CON_ECC_ON,
};

#define IFX_EBU_NAND_CON_LAT_EN			(0x3F << 18)
#define IFX_EBU_NAND_CON_LAT_EN_S		18
enum {
    IFX_EBU_NAND_CON_LAT_EN_DEF = 0x3D,
};

#define IFX_EBU_NAND_ECC_CRM			(1<<31)
#define IFX_EBU_NAND_ECC_CRM_S			31
enum {
    IFX_EBU_NAND_ECC_CRM_DISABLE = 0,
    IFX_EBU_NAND_ECC_CRM_ENABLE,
};

#define IFX_EBU_NAND_ECC_PAGE			(3<<14)
#define IFX_EBU_NAND_ECC_PAGE_S			14
enum {
    IFX_EBU_NAND_ECC_PAGE_256 = 0,
    IFX_EBU_NAND_ECC_PAGE_512,
    IFX_EBU_NAND_ECC_PAGE_RES,
};

#define IFX_EBU_ECC_IEN_IR			(1<<5)
#define IFX_EBU_ECC_IEN_IR_S			5
enum {
   IFX_EBU_ECC_IEN_DISABLE = 0,
   IFX_EBU_ECC_IEN_ENABLE,
};

#define IFX_EBU_NAND_ECC_STATE			(3<<0)
#define IFX_EBU_NAND_ECC_STATE_S		0

#define IFX_EBU_NAND_ECC_ROW_VAL		(0x1FF<<5)
#define IFX_EBU_NAND_ECC_ROW_VAL_S		5

#define IFX_EBU_NAND_ECC_BIT_POS		(7<<2)
#define IFX_EBU_NAND_ECC_BIT_POS_S		2

#define IFX_EBU_NAND_WAIT_RD                    (0x1)
#define IFX_EBU_NAND_WAIT_BY_E                  (1<<1)
#define IFX_EBU_NAND_WAIT_RD_E                  (1<<2)
#define IFX_EBU_NAND_WAIT_WR_C                  (1<<3)

#define IFX_EBU_NAND_ECC0                       (volatile u32*)(IFX_EBU + 0xB8) 
#define IFX_EBU_NAND_ECC_AC                     (volatile u32*)(IFX_EBU + 0xBC) 

/***********************************************************************/
/*  Module      :  I2C register address and bits                     */
/***********************************************************************/
#define IFX_I2C                               (KSEG1 | 0x1DB00000)

#define HNX_I2C_BASE                          IFX_I2C
#define HNX_I2C_END                           (IFX_I2C + 0xD000)


#if 0  //YLH: Not exist anymore
/***********************************************************************/
/*  Module      :  SDRAM register address and bits                     */
/***********************************************************************/

#define IFX_SDRAM                               (KSEG1 | 0x1F800000)

/***MC Access Error Cause Register***/
#define IFX_SDRAM_MC_ERRCAUSE                   ((volatile u32*)(IFX_SDRAM + 0x0100))
#define IFX_SDRAM_MC_ERRCAUSE_ERR               (1 << 31)
#define IFX_SDRAM_MC_ERRCAUSE_PORT(value)       (((( 1 << 4) - 1) & (value)) << 16)
#define IFX_SDRAM_MC_ERRCAUSE_CAUSE(value)      (((( 1 << 2) - 1) & (value)) << 0)
#define IFX_SDRAM_MC_ERRCAUSE_Res(value)        (((( 1 << NaN) - 1) & (value)) << NaN)

/***MC Access Error Address Register***/
#define IFX_SDRAM_MC_ERRADDR                    ((volatile u32*)(IFX_SDRAM + 0x0108))

/***MC I/O General Purpose Register***/
#define IFX_SDRAM_MC_IOGP                       ((volatile u32*)(IFX_SDRAM + 0x0800))
#define IFX_SDRAM_MC_IOGP_GPR6(value)           (((( 1 << 4) - 1) & (value)) << 28)
#define IFX_SDRAM_MC_IOGP_GPR5(value)           (((( 1 << 4) - 1) & (value)) << 24)
#define IFX_SDRAM_MC_IOGP_GPR4(value)           (((( 1 << 4) - 1) & (value)) << 20)
#define IFX_SDRAM_MC_IOGP_GPR3(value)           (((( 1 << 4) - 1) & (value)) << 16)
#define IFX_SDRAM_MC_IOGP_GPR2(value)           (((( 1 << 4) - 1) & (value)) << 12)
#define IFX_SDRAM_MC_IOGP_CPS                   (1 << 11)
#define IFX_SDRAM_MC_IOGP_CLKDELAY(value)       (((( 1 << 3) - 1) & (value)) << 8)
#define IFX_SDRAM_MC_IOGP_CLKRAT(value)         (((( 1 << 4) - 1) & (value)) << 4)
#define IFX_SDRAM_MC_IOGP_RDDEL(value)          (((( 1 << 4) - 1) & (value)) << 0)

/***MC Self Refresh Register***/
#define IFX_SDRAM_MC_SELFRFSH                   ((volatile u32*)(IFX_SDRAM + 0x0A00))
#define IFX_SDRAM_MC_SELFRFSH_PWDS              (1 << 1)
#define IFX_SDRAM_MC_SELFRFSH_PWD               (1 << 0)
#define IFX_SDRAM_MC_SELFRFSH_Res(value)        (((( 1 << 30) - 1) & (value)) << 2)

/***MC Enable Register***/
#define IFX_SDRAM_MC_CTRLENA                    ((volatile u32*)(IFX_SDRAM + 0x1000))
#define IFX_SDRAM_MC_CTRLENA_ENA                (1 << 0)
#define IFX_SDRAM_MC_CTRLENA_Res(value)         (((( 1 << 31) - 1) & (value)) << 1)

/***MC Mode Register Setup Code***/
#define IFX_SDRAM_MC_MRSCODE                    ((volatile u32*)(IFX_SDRAM + 0x1008))
#define IFX_SDRAM_MC_MRSCODE_UMC(value)         (((( 1 << 5) - 1) & (value)) << 7)
#define IFX_SDRAM_MC_MRSCODE_CL(value)          (((( 1 << 3) - 1) & (value)) << 4)
#define IFX_SDRAM_MC_MRSCODE_WT                 (1 << 3)
#define IFX_SDRAM_MC_MRSCODE_BL(value)          (((( 1 << 3) - 1) & (value)) << 0)

/***MC Configuration Data-word Width Register***/
#define IFX_SDRAM_MC_CFGDW                      ((volatile u32*)(IFX_SDRAM + 0x1010))
#define IFX_SDRAM_MC_CFGDW_DW(value)            (((( 1 << 4) - 1) & (value)) << 0)
#define IFX_SDRAM_MC_CFGDW_Res(value)           (((( 1 << 28) - 1) & (value)) << 4)

/***MC Configuration Physical Bank 0 Register***/
#define IFX_SDRAM_MC_CFGPB0                     ((volatile u32*)(IFX_SDRAM + 0x1018))
#define IFX_SDRAM_MC_CFGPB0_MCSEN0(value)       (((( 1 << 4) - 1) & (value)) << 12)
#define IFX_SDRAM_MC_CFGPB0_BANKN0(value)       (((( 1 << 4) - 1) & (value)) << 8)
#define IFX_SDRAM_MC_CFGPB0_ROWW0(value)        (((( 1 << 4) - 1) & (value)) << 4)
#define IFX_SDRAM_MC_CFGPB0_COLW0(value)        (((( 1 << 4) - 1) & (value)) << 0)
#define IFX_SDRAM_MC_CFGPB0_Res(value)          (((( 1 << 16) - 1) & (value)) << 16)

/***MC Latency Register***/
#define IFX_SDRAM_MC_LATENCY                    ((volatile u32*)(IFX_SDRAM + 0x1038))
#define IFX_SDRAM_MC_LATENCY_TRP(value)         (((( 1 << 4) - 1) & (value)) << 16)
#define IFX_SDRAM_MC_LATENCY_TRAS(value)        (((( 1 << 4) - 1) & (value)) << 12)
#define IFX_SDRAM_MC_LATENCY_TRCD(value)        (((( 1 << 4) - 1) & (value)) << 8)
#define IFX_SDRAM_MC_LATENCY_TDPL(value)        (((( 1 << 4) - 1) & (value)) << 4)
#define IFX_SDRAM_MC_LATENCY_TDAL(value)        (((( 1 << 4) - 1) & (value)) << 0)
#define IFX_SDRAM_MC_LATENCY_Res(value)         (((( 1 << 12) - 1) & (value)) << 20)

/***MC Refresh Cycle Time Register***/
#define IFX_SDRAM_MC_TREFRESH                   ((volatile u32*)(IFX_SDRAM + 0x1040))
#define IFX_SDRAM_MC_TREFRESH_TREF(value)       (((( 1 << 13) - 1) & (value)) << 0)
#define IFX_SDRAM_MC_TREFRESH_Res(value)        (((( 1 << 19) - 1) & (value)) << 13)

/***MC Status Register***/
#define IFX_SDRAM_MC_STAT                       ((volatile u32*)(IFX_SDRAM + 0x0070))

/***MC DDR Control Register 00***/
#define IFX_DDR_MC_DC00                         ((volatile u32*)(IFX_SDRAM + 0x1000))
/***MC DDR Control Register 03***/
#define IFX_DDR_MC_DC03                         ((volatile u32*)(IFX_SDRAM + 0x1030))
/***MC DDR Control Register 17***/
#define IFX_DDR_MC_DC17                         ((volatile u32*)(IFX_SDRAM + 0x1110))
#endif 

/***********************************************************************/
/*  Module      :  ASC1 register address and bits                      */
/***********************************************************************/

#define IFX_ASC1                                (KSEG1 | 0x1E100C00)

/***ASC Clock Control Register***/
#define IFX_ASC1_CLC                            ((volatile u32*)(IFX_ASC1 + 0x0000))
#define IFX_ASC1_CLC_RMC(value)                 (((( 1 << 8) - 1) & (value)) << 8)
#define IFX_ASC1_CLC_DISS                       (1 << 1)
#define IFX_ASC1_CLC_DISR                       (1 << 0)

/***ASC Port Input Select Register***/
#define IFX_ASC1_PISEL                          ((volatile u32*)(IFX_ASC1 + 0x0004))
#define IFX_ASC1_ID                             ((volatile u32*)(IFX_ASC1 + 0x0008))
#define IFX_ASC1_PISEL_RIS                      (1 << 0)

/***ASC Control Register***/
#define IFX_ASC1_CON                            ((volatile u32*)(IFX_ASC1 + 0x0010))
#define IFX_ASC1_CON_BEN                        (1 << 20)
#define IFX_ASC1_CON_TOEN                       (1 << 20)
#define IFX_ASC1_CON_ROEN                       (1 << 19)
#define IFX_ASC1_CON_RUEN                       (1 << 18)
#define IFX_ASC1_CON_FEN                        (1 << 17)
#define IFX_ASC1_CON_PAL                        (1 << 16)
#define IFX_ASC1_CON_R                          (1 << 15)
#define IFX_ASC1_CON_ACO                        (1 << 14)
#define IFX_ASC1_CON_LB                         (1 << 13)
#define IFX_ASC1_CON_ERCLK                      (1 << 10)
#define IFX_ASC1_CON_FDE                        (1 << 9)
#define IFX_ASC1_CON_BRS                        (1 << 8)
#define IFX_ASC1_CON_STP                        (1 << 7)
#define IFX_ASC1_CON_SP                         (1 << 6)
#define IFX_ASC1_CON_ODD                        (1 << 5)
#define IFX_ASC1_CON_PEN                        (1 << 4)
#define IFX_ASC1_CON_M(value)                   (((( 1 << 3) - 1) & (value)) << 0)

/***ASC Staus Register***/
#define IFX_ASC1_STATE                          ((volatile u32*)(IFX_ASC1 + 0x0014))
/***ASC Write Hardware Modified Control Register***/
#define IFX_ASC1_WHBSTATE                       ((volatile u32*)(IFX_ASC1 + 0x0018))
#define IFX_ASC1_WHBSTATE_SETBE                 (1 << 113)
#define IFX_ASC1_WHBSTATE_SETTOE                (1 << 12)
#define IFX_ASC1_WHBSTATE_SETROE                (1 << 11)
#define IFX_ASC1_WHBSTATE_SETRUE                (1 << 10)
#define IFX_ASC1_WHBSTATE_SETFE                 (1 << 19)
#define IFX_ASC1_WHBSTATE_SETPE                 (1 << 18)
#define IFX_ASC1_WHBSTATE_CLRBE                 (1 << 17)
#define IFX_ASC1_WHBSTATE_CLRTOE                (1 << 6)
#define IFX_ASC1_WHBSTATE_CLRROE                (1 << 5)
#define IFX_ASC1_WHBSTATE_CLRRUE                (1 << 4)
#define IFX_ASC1_WHBSTATE_CLRFE                 (1 << 3)
#define IFX_ASC1_WHBSTATE_CLRPE                 (1 << 2)
#define IFX_ASC1_WHBSTATE_SETREN                (1 << 1)
#define IFX_ASC1_WHBSTATE_CLRREN                (1 << 0)

/***ASC Baudrate Timer/Reload Register***/
#define IFX_ASC1_BG                             ((volatile u32*)(IFX_ASC1 + 0x0050))
#define IFX_ASC1_BG_BR_VALUE(value)             (((( 1 << 13) - 1) & (value)) << 0)

/***ASC Fractional Divider Register***/
#define IFX_ASC1_FDV                            ((volatile u32*)(IFX_ASC1 + 0x0058))
#define IFX_ASC1_FDV_FD_VALUE(value)            (((( 1 << 9) - 1) & (value)) << 0)

/***ASC Transmit Buffer Register***/
#define IFX_ASC1_TBUF                           ((volatile u32*)(IFX_ASC1 + 0x0020))
#define IFX_ASC1_TBUF_TD_VALUE(value)           (((( 1 << 9) - 1) & (value)) << 0)

/***ASC Receive Buffer Register***/
#define IFX_ASC1_RBUF                           ((volatile u32*)(IFX_ASC1 + 0x0024))
#define IFX_ASC1_RBUF_RD_VALUE(value)           (((( 1 << 9) - 1) & (value)) << 0)

/***ASC Autobaud Control Register***/
#define IFX_ASC1_ABCON                          ((volatile u32*)(IFX_ASC1 + 0x0030))
#define IFX_ASC1_ABCON_RXINV                    (1 << 11)
#define IFX_ASC1_ABCON_TXINV                    (1 << 10)
#define IFX_ASC1_ABCON_ABEM(value)              (((( 1 << 2) - 1) & (value)) << 8)
#define IFX_ASC1_ABCON_FCDETEN                  (1 << 4)
#define IFX_ASC1_ABCON_ABDETEN                  (1 << 3)
#define IFX_ASC1_ABCON_ABSTEN                   (1 << 2)
#define IFX_ASC1_ABCON_AUREN                    (1 << 1)
#define IFX_ASC1_ABCON_ABEN                     (1 << 0)

/***Receive FIFO Control Register***/
#define IFX_ASC1_RXFCON                         ((volatile u32*)(IFX_ASC1 + 0x0040))
#define IFX_ASC1_RXFCON_RXFITL(value)           (((( 1 << 6) - 1) & (value)) << 8)
#define IFX_ASC1_RXFCON_RXFFLU                  (1 << 1)
#define IFX_ASC1_RXFCON_RXFEN                   (1 << 0)

/***Transmit FIFO Control Register***/
#define IFX_ASC1_TXFCON                         ((volatile u32*)(IFX_ASC1 + 0x0044))
#define IFX_ASC1_TXFCON_TXFITL(value)           (((( 1 << 6) - 1) & (value)) << 8)
#define IFX_ASC1_TXFCON_TXFFLU                  (1 << 1)
#define IFX_ASC1_TXFCON_TXFEN                   (1 << 0)

/***FIFO Status Register***/
#define IFX_ASC1_FSTAT                          ((volatile u32*)(IFX_ASC1 + 0x0048))
#define IFX_ASC1_FSTAT_TXFFL(value)             (((( 1 << 6) - 1) & (value)) << 8)
#define IFX_ASC1_FSTAT_RXFFL(value)             (((( 1 << 6) - 1) & (value)) << 0)
#define IFX_ASC1_FSTAT_TXFREE_GET(value)        (((value) >> 24) & ((1 << 6) - 1))
#define IFX_ASC1_FSTAT_TXFREE_SET(value)        (((( 1 << 6) - 1) & (value)) << 24)
#define IFX_ASC1_FSTAT_RXFREE_GET(value)        (((value) >> 16) & ((1 << 6) - 1))
#define IFX_ASC1_FSTAT_RXFREE_SET(value)        (((( 1 << 6) - 1) & (value)) << 16)
#define IFX_ASC1_FSTAT_TXFFL_GET(value)         (((value) >> 8) & ((1 << 6) - 1))
#define IFX_ASC1_FSTAT_TXFFL_SET(value)         (((( 1 << 6) - 1) & (value)) << 8)
#define IFX_ASC1_FSTAT_RXFFL_GET(value)         (((value) >> 0) & ((1 << 6) - 1))
#define IFX_ASC1_FSTAT_RXFFL_SET(value)         (((( 1 << 6) - 1) & (value)) << 0)


/***ASC Autobaud Status Register***/
#define IFX_ASC1_ABSTAT                         ((volatile u32*)(IFX_ASC1 + 0x0034))
#define IFX_ASC1_ABSTAT_DETWAIT                 (1 << 4)
#define IFX_ASC1_ABSTAT_SCCDET                  (1 << 3)
#define IFX_ASC1_ABSTAT_SCSDET                  (1 << 2)
#define IFX_ASC1_ABSTAT_FCCDET                  (1 << 1)
#define IFX_ASC1_ABSTAT_FCSDET                  (1 << 0)

/***ASC Write HW Modified Autobaud Status Register***/
#define IFX_ASC1_WHBABSTAT                      ((volatile u32*)(IFX_ASC1 + 0x003C))
#define IFX_ASC1_WHBABSTAT_SETDETWAIT           (1 << 9)
#define IFX_ASC1_WHBABSTAT_CLRDETWAIT           (1 << 8)
#define IFX_ASC1_WHBABSTAT_SETSCCDET            (1 << 7)
#define IFX_ASC1_WHBABSTAT_CLRSCCDET            (1 << 6)
#define IFX_ASC1_WHBABSTAT_SETSCSDET            (1 << 5)
#define IFX_ASC1_WHBABSTAT_CLRSCSDET            (1 << 4)
#define IFX_ASC1_WHBABSTAT_SETFCCDET            (1 << 3)
#define IFX_ASC1_WHBABSTAT_CLRFCCDET            (1 << 2)
#define IFX_ASC1_WHBABSTAT_SETFCSDET            (1 << 1)
#define IFX_ASC1_WHBABSTAT_CLRFCSDET            (1 << 0)

/***ASC IRNCR0 **/
#define IFX_ASC1_IRNREN                         ((volatile u32*)(IFX_ASC1 + 0x00F4))
#define IFX_ASC1_IRNICR                         ((volatile u32*)(IFX_ASC1 + 0x00FC))
/***ASC IRNCR1 **/
#define IFX_ASC1_IRNCR                          ((volatile u32*)(IFX_ASC1 + 0x00F8))
#define IFX_ASC_IRNCR_TIR                       0x1
#define IFX_ASC_IRNCR_RIR                       0x2
#define IFX_ASC_IRNCR_EIR                       0x4



/***********************************************************************/
/*  Module      :  DMA register address and bits                       */
/***********************************************************************/

#define IFX_DMA                                 (KSEG1 | 0x1E104100)

#define IFX_DMA_BASE                            IFX_DMA
#define IFX_DMA_CLC                             (volatile u32*)(IFX_DMA_BASE + 0x00)
#define IFX_DMA_ID                              (volatile u32*)(IFX_DMA_BASE + 0x08)
#define IFX_DMA_CTRL                            (volatile u32*)(IFX_DMA_BASE + 0x10)
#define IFX_DMA_CPOLL                           (volatile u32*)(IFX_DMA_BASE + 0x14)

#define IFX_DMA_CS(i)                           (volatile u32*)(IFX_DMA_BASE + 0x18 + 0x38 * (i))
#define IFX_DMA_CCTRL(i)                        (volatile u32*)(IFX_DMA_BASE + 0x1C + 0x38 * (i))
#define IFX_DMA_CDBA(i)                         (volatile u32*)(IFX_DMA_BASE + 0x20 + 0x38 * (i))
#define IFX_DMA_CDLEN(i)                        (volatile u32*)(IFX_DMA_BASE + 0x24 + 0x38 * (i))
#define IFX_DMA_CIS(i)                          (volatile u32*)(IFX_DMA_BASE + 0x28 + 0x38 * (i))
#define IFX_DMA_CIE(i)                          (volatile u32*)(IFX_DMA_BASE + 0x2C + 0x38 * (i))

#define IFX_DMA_CGBL                            (volatile u32*)(IFX_DMA_BASE + 0x30)
#define IFX_DMA_CDPTNRD                         (volatile u32*)(IFX_DMA_BASE + 0x34)

#define IFX_DMA_PS(i)                           (volatile u32*)(IFX_DMA_BASE + 0x40 + 0x30 * (i))
#define IFX_DMA_PCTRL(i)                        (volatile u32*)(IFX_DMA_BASE + 0x44 + 0x30 * (i))

#define IFX_DMA_IRNEN                           (volatile u32*)(IFX_DMA_BASE + 0xf4)
#define IFX_DMA_IRNCR                           (volatile u32*)(IFX_DMA_BASE + 0xf8)
#define IFX_DMA_IRNICR                          (volatile u32*)(IFX_DMA_BASE + 0xfc)
/* Global Software Reset (0) */
#define IFX_DMA_CTRL_RST                        (0x1)

/* Channel Polling Register */

/* Enable (31) */
#define IFX_DMA_CPOLL_EN                        (0x1 << 31)
#define IFX_DMA_CPOLL_EN_VAL(val)               (((val) & 0x1) << 31)

/* Counter (15:4) */
#define IFX_DMA_CPOLL_CNT                       (0xfff << 4)
#define IFX_DMA_CPOLL_CNT_VAL(val)              (((val) & 0xfff) << 4)

/* Channel Control Register */

/* Peripheral to Peripheral Copy (24) */
#define IFX_DMA_CCTRL_P2PCPY                    (0x1 << 24)
#define IFX_DMA_CCTRL_P2PCPY_VAL(val)           (((val) & 0x1) << 24)
#define IFX_DMA_CCTRL_P2PCPY_GET(val)           ((((val) & IFX_DMA_CCTRL_P2PCPY) >> 24) & 0x1)

/* Channel Weight for Transmit Direction (17:16) */
#define IFX_DMA_CCTRL_TXWGT                     (0x3 << 16)
#define IFX_DMA_CCTRL_TXWGT_VAL(val)            (((val) & 0x3) << 16)
#define IFX_DMA_CCTRL_TXWGT_GET(val)            ((((val) & IFX_DMA_CCTRL_TXWGT) >> 16) & 0x3)

/* Port Assignment (13:11) */
#define IFX_DMA_CCTRL_PRTNR                     (0x7 << 11)
#define IFX_DMA_CCTRL_PRTNR_GET(val)            ((((val) & IFX_DMA_CCTRL_PRTNR) >> 11) & 0x7)

/* Class (10:9) */
#define IFX_DMA_CCTRL_CLASS                     (0x3 << 9)
#define IFX_DMA_CCTRL_CLASS_VAL(val)            (((val) & 0x3) << 9)
#define IFX_DMA_CCTRL_CLASS_GET(val)            ((((val) & IFX_DMA_CCTRL_CLASS) >> 9) & 0x3)

/* Direction (8) */
#define IFX_DMA_CCTRL_DIR                       (0x1 << 8)
/* Reset (1) */
#define IFX_DMA_CCTRL_RST                       (0x1 << 1)
/* Channel On or Off (0) */
#define IFX_DMA_CCTRL_ON                        (0x1)

/* Channel Interrupt Status Register  */

/* SAI Read Error Interrupt (5) */
#define IFX_DMA_CIS_RDERR                       (0x1 << 5)
/* Channel Off Interrupt (4) */
#define IFX_DMA_CIS_CHOFF                       (0x1 << 4)
/* Descriptor Complete Interrupt (3) */
#define IFX_DMA_CIS_DESCPT                      (0x1 << 3)
/* Descriptor Under-Run Interrupt (2) */
#define IFX_DMA_CIS_DUR                         (0x1 << 2)
/* End of Packet Interrupt (1) */
#define IFX_DMA_CIS_EOP                         (0x1 << 1)

#define IFX_DMA_CIS_ALL                         (IFX_DMA_CIS_RDERR | IFX_DMA_CIS_CHOFF| \
                                                 IFX_DMA_CIS_DESCPT | IFX_DMA_CIS_DUR | \
                                                 IFX_DMA_CIS_EOP)

/*  Channel Interrupt Enable Register */

/* SAI Read Error Interrupt (5) */
#define IFX_DMA_CIE_RDERR                       (0x1 << 5)
/* Channel Off Interrupt (4) */
#define IFX_DMA_CIE_CHOFF                       (0x1 << 4)
/* Descriptor Complete Interrupt Enable (3) */
#define IFX_DMA_CIE_DESCPT                      (0x1 << 3)
/* Descriptor Under Run Interrupt Enable (2) */
#define IFX_DMA_CIE_DUR                         (0x1 << 2)
/* End of Packet Interrupt Enable (1) */
#define IFX_DMA_CIE_EOP                         (0x1 << 1)

#define IFX_DMA_CIE_DEFAULT                     (IFX_DMA_CIE_DESCPT | IFX_DMA_CIE_EOP)

/* Port Select Register */

/* Port Selection (2:0) */
#define IFX_DMA_PS_PS                           (0x7)
#define IFX_DMA_PS_PS_VAL(val)                  (((val) & 0x7) << 0)

/* Port Control Register */

/* General Purpose Control (16) */
#define IFX_DMA_PCTRL_GPC                       (0x1 << 16)
#define IFX_DMA_PCTRL_GPC_VAL(val)              (((val) & 0x1) << 16)

/* Port Weight for Transmit Direction (14:12) */
#define IFX_DMA_PCTRL_TXWGT                     (0x7 << 12)
#define IFX_DMA_PCTRL_TXWGT_VAL(val)            (((val) & 0x7) << 12)
/* Endianness for Transmit Direction (11:10) */
#define IFX_DMA_PCTRL_TXENDI                    (0x3 << 10)
#define IFX_DMA_PCTRL_TXENDI_VAL(val)           (((val) & 0x3) << 10)
/* Endianness for Receive Direction (9:8) */
#define IFX_DMA_PCTRL_RXENDI                    (0x3 << 8)
#define IFX_DMA_PCTRL_RXENDI_VAL(val)           (((val) & 0x3) << 8)
/* Packet Drop Enable (6) */
#define IFX_DMA_PCTRL_PDEN                      (0x1 << 6)
#define IFX_DMA_PCTRL_PDEN_VAL(val)             (((val) & 0x1) << 6)
/* Burst Length for Transmit Direction (5:4) */
#define IFX_DMA_PCTRL_TXBL                      (0x3 << 4)
#define IFX_DMA_PCTRL_TXBL_VAL(val)             (((val) & 0x3) << 4)
/* Burst Length for Receive Direction (3:2) */
#define IFX_DMA_PCTRL_RXBL                      (0x3 << 2)
#define IFX_DMA_PCTRL_RXBL_VAL(val)             (((val) & 0x3) << 2)



/***********************************************************************/
/*  Module      :  Debug register address and bits                     */
/***********************************************************************/

#define IFX_Debug                               (KSEG1 | 0x1F106000)

/***MCD Break Bus Switch Register***/
#define IFX_Debug_MCD_BBS                       ((volatile u32*)(IFX_Debug + 0x0000))
#define IFX_Debug_MCD_BBS_BTP1                  (1 << 19)
#define IFX_Debug_MCD_BBS_BTP0                  (1 << 18)
#define IFX_Debug_MCD_BBS_BSP1                  (1 << 17)
#define IFX_Debug_MCD_BBS_BSP0                  (1 << 16)
#define IFX_Debug_MCD_BBS_BT5EN                 (1 << 15)
#define IFX_Debug_MCD_BBS_BT4EN                 (1 << 14)
#define IFX_Debug_MCD_BBS_BT5                   (1 << 13)
#define IFX_Debug_MCD_BBS_BT4                   (1 << 12)
#define IFX_Debug_MCD_BBS_BS5EN                 (1 << 7)
#define IFX_Debug_MCD_BBS_BS4EN                 (1 << 6)
#define IFX_Debug_MCD_BBS_BS5                   (1 << 5)
#define IFX_Debug_MCD_BBS_BS4                   (1 << 4)

/***MCD Multiplexer Control Register***/
#define IFX_Debug_MCD_MCR                       ((volatile u32*)(IFX_Debug+ 0x0008))
#define IFX_Debug_MCD_MCR_MUX5                  (1 << 4)
#define IFX_Debug_MCD_MCR_MUX4                  (1 << 3)
#define IFX_Debug_MCD_MCR_MUX1                  (1 << 0)



/***********************************************************************/
/*  Module      :  ICU register address and bits                       */
/***********************************************************************/

#define IFX_ICU                                 (KSEG1 | 0x1F880200)

#define IFX_ICU_IM0_ISR                         ((volatile u32*)(IFX_ICU + 0x0000))
#define IFX_ICU_IM0_IER                         ((volatile u32*)(IFX_ICU + 0x0008))
#define IFX_ICU_IM0_IOSR                        ((volatile u32*)(IFX_ICU + 0x0010))
#define IFX_ICU_IM0_IRSR                        ((volatile u32*)(IFX_ICU + 0x0018))
#define IFX_ICU_IM0_IMR                         ((volatile u32*)(IFX_ICU + 0x0020))

#define IFX_ICU_IM1_ISR                         ((volatile u32*)(IFX_ICU + 0x0028))
#define IFX_ICU_IM1_IER                         ((volatile u32*)(IFX_ICU + 0x0030))
#define IFX_ICU_IM1_IOSR                        ((volatile u32*)(IFX_ICU + 0x0038))
#define IFX_ICU_IM1_IRSR                        ((volatile u32*)(IFX_ICU + 0x0040))
#define IFX_ICU_IM1_IMR                         ((volatile u32*)(IFX_ICU + 0x0048))

#define IFX_ICU_IM2_ISR                         ((volatile u32*)(IFX_ICU + 0x0050))
#define IFX_ICU_IM2_IER                         ((volatile u32*)(IFX_ICU + 0x0058))
#define IFX_ICU_IM2_IOSR                        ((volatile u32*)(IFX_ICU + 0x0060))
#define IFX_ICU_IM2_IRSR                        ((volatile u32*)(IFX_ICU + 0x0068))
#define IFX_ICU_IM2_IMR                         ((volatile u32*)(IFX_ICU + 0x0070))

#define IFX_ICU_IM3_ISR                         ((volatile u32*)(IFX_ICU + 0x0078))
#define IFX_ICU_IM3_IER                         ((volatile u32*)(IFX_ICU + 0x0080))
#define IFX_ICU_IM3_IOSR                        ((volatile u32*)(IFX_ICU + 0x0088))
#define IFX_ICU_IM3_IRSR                        ((volatile u32*)(IFX_ICU + 0x0090))
#define IFX_ICU_IM3_IMR                         ((volatile u32*)(IFX_ICU + 0x0098))

#define IFX_ICU_IM4_ISR                         ((volatile u32*)(IFX_ICU + 0x00A0))
#define IFX_ICU_IM4_IER                         ((volatile u32*)(IFX_ICU + 0x00A8))
#define IFX_ICU_IM4_IOSR                        ((volatile u32*)(IFX_ICU + 0x00B0))
#define IFX_ICU_IM4_IRSR                        ((volatile u32*)(IFX_ICU + 0x00B8))
#define IFX_ICU_IM4_IMR                         ((volatile u32*)(IFX_ICU + 0x00C0))

/***Interrupt Vector Value Register***/
#define IFX_ICU_IM_VEC                          ((volatile u32*)(IFX_ICU + 0x00C8))

/***********************************************************************/

#define IFX_ICU_VPE1                            (KSEG1 | 0x1F880300)
#define IFX_ICU1                                IFX_ICU_VPE1

#define IFX_ICU_VPE1_IM0_ISR                    ((volatile u32*)(IFX_ICU_VPE1 + 0x0000))
#define IFX_ICU_VPE1_IM0_IER                    ((volatile u32*)(IFX_ICU_VPE1 + 0x0008))
#define IFX_ICU_VPE1_IM0_IOSR                   ((volatile u32*)(IFX_ICU_VPE1 + 0x0010))
#define IFX_ICU_VPE1_IM0_IRSR                   ((volatile u32*)(IFX_ICU_VPE1 + 0x0018))
#define IFX_ICU_VPE1_IM0_IMR                    ((volatile u32*)(IFX_ICU_VPE1 + 0x0020))

#define IFX_ICU_VPE1_IM1_ISR                    ((volatile u32*)(IFX_ICU_VPE1 + 0x0028))
#define IFX_ICU_VPE1_IM1_IER                    ((volatile u32*)(IFX_ICU_VPE1 + 0x0030))
#define IFX_ICU_VPE1_IM1_IOSR                   ((volatile u32*)(IFX_ICU_VPE1 + 0x0038))
#define IFX_ICU_VPE1_IM1_IRSR                   ((volatile u32*)(IFX_ICU_VPE1 + 0x0040))
#define IFX_ICU_VPE1_IM1_IMR                    ((volatile u32*)(IFX_ICU_VPE1 + 0x0048))

#define IFX_ICU_VPE1_IM2_ISR                    ((volatile u32*)(IFX_ICU_VPE1 + 0x0050))
#define IFX_ICU_VPE1_IM2_IER                    ((volatile u32*)(IFX_ICU_VPE1 + 0x0058))
#define IFX_ICU_VPE1_IM2_IOSR                   ((volatile u32*)(IFX_ICU_VPE1 + 0x0060))
#define IFX_ICU_VPE1_IM2_IRSR                   ((volatile u32*)(IFX_ICU_VPE1 + 0x0068))
#define IFX_ICU_VPE1_IM2_IMR                    ((volatile u32*)(IFX_ICU_VPE1 + 0x0070))

#define IFX_ICU_VPE1_IM3_ISR                    ((volatile u32*)(IFX_ICU_VPE1 + 0x0078))
#define IFX_ICU_VPE1_IM3_IER                    ((volatile u32*)(IFX_ICU_VPE1 + 0x0080))
#define IFX_ICU_VPE1_IM3_IOSR                   ((volatile u32*)(IFX_ICU_VPE1 + 0x0088))
#define IFX_ICU_VPE1_IM3_IRSR                   ((volatile u32*)(IFX_ICU_VPE1 + 0x0090))
#define IFX_ICU_VPE1_IM3_IMR                    ((volatile u32*)(IFX_ICU_VPE1 + 0x0098))

#define IFX_ICU_VPE1_IM4_ISR                    ((volatile u32*)(IFX_ICU_VPE1 + 0x00A0))
#define IFX_ICU_VPE1_IM4_IER                    ((volatile u32*)(IFX_ICU_VPE1 + 0x00A8))
#define IFX_ICU_VPE1_IM4_IOSR                   ((volatile u32*)(IFX_ICU_VPE1 + 0x00B0))
#define IFX_ICU_VPE1_IM4_IRSR                   ((volatile u32*)(IFX_ICU_VPE1 + 0x00B8))
#define IFX_ICU_VPE1_IM4_IMR                    ((volatile u32*)(IFX_ICU_VPE1 + 0x00C0))

/***Interrupt Vector Value Register***/
#define IFX_ICU_VPE1_IM_VEC                   ((volatile u32*)(IFX_ICU_VPE1 + 0x00C8))
#define IFX_ICU_IM_VEC1                         IFX_ICU_VPE1_IM_VEC

/* MSI PIC */
#define IFX_MSI_PIC_REG_BASE                    (KSEG1 | 0x1F700000)

#define IFX_MSI_PIC_BIG_ENDIAN                  1
#define IFX_MSI_PIC_LITTLE_ENDIAN               0

#define IFX_MSI_PCI_INT_DISABLE                 0x80000000
#define IFX_MSI_PIC_INT_LINE                    0x30000000
#define IFX_MSI_PIC_INT_LINE_S                  28
#define IFX_MSI_PIC_MSG_ADDR                    0x0FFF0000
#define IFX_MSI_PIC_MSG_ADDR_S                  16
#define IFX_MSI_PIC_MSG_DATA                    0x0000FFFF
#define IFX_MSI_PIC_MSG_DATA_S                  0x0

/***Interrupt Vector Value Mask***/
#define IFX_ICU_IM0_VEC_MASK                    (0x3F << 0)
#define IFX_ICU_IM1_VEC_MASK                    (0x3F << 6)
#define IFX_ICU_IM2_VEC_MASK                    (0x3F << 12)
#define IFX_ICU_IM3_VEC_MASK                    (0x3F << 18)
#define IFX_ICU_IM4_VEC_MASK                    (0x3F << 24)

/***External Interrupt Control Register***/
#define IFX_ICU_EIU                             (KSEG1 | 0x1F101000)
#define IFX_ICU_EIU_EXIN_C                      ((volatile u32 *)(IFX_ICU_EIU + 0x0000))
#define IFX_ICU_EIU_INIC                        ((volatile u32 *)(IFX_ICU_EIU + 0x0004))
#define IFX_ICU_EIU_INC                         ((volatile u32 *)(IFX_ICU_EIU + 0x0008))
#define IFX_ICU_EIU_INEN                        ((volatile u32 *)(IFX_ICU_EIU + 0x000C))
#define IFX_YIELDEN(n)                          ((volatile u32 *)(IFX_ICU_EIU + 0x0010 + (n) * 4)
#define IFX_NMI_CR                              ((volatile u32 *)(IFX_ICU_EIU + 0x00F0))
#define IFX_NMI_SR                              ((volatile u32 *)(IFX_ICU_EIU + 0x00F4))



/***********************************************************************/
/*  Module      :  MPS register address and bits                       */
/***********************************************************************/

#define IFX_MPS                                 (KSEG1 | 0x1F107000)

#define IFX_MPS_CHIPID                          ((volatile u32*)(IFX_MPS + 0x0344))
#define IFX_MPS_CHIPID_VERSION_GET(value)       (((value) >> 28) & ((1 << 4) - 1))
#define IFX_MPS_CHIPID_VERSION_SET(value)       (((( 1 << 4) - 1) & (value)) << 28)
#define IFX_MPS_CHIPID_PARTNUM_GET(value)       (((value) >> 12) & ((1 << 16) - 1))
#define IFX_MPS_CHIPID_PARTNUM_SET(value)       (((( 1 << 16) - 1) & (value)) << 12)
#define IFX_MPS_CHIPID_MANID_GET(value)         (((value) >> 1) & ((1 << 10) - 1))
#define IFX_MPS_CHIPID_MANID_SET(value)         (((( 1 << 10) - 1) & (value)) << 1)


/* notification enable register */
#define IFX_MPS_CPU0_NFER                       ((volatile u32*)(IFX_MPS + 0x0060))
#define IFX_MPS_CPU1_NFER                       ((volatile u32*)(IFX_MPS + 0x0064))
/* CPU to CPU interrup request register */
#define IFX_MPS_CPU0_2_CPU1_IRR                 ((volatile u32*)(IFX_MPS + 0x0070))
#define IFX_MPS_CPU0_2_CPU1_IER                 ((volatile u32*)(IFX_MPS + 0x0074))
/* Global interrupt request and request enable register */
#define IFX_MPS_GIRR                            ((volatile u32*)(IFX_MPS + 0x0078))
#define IFX_MPS_GIER                            ((volatile u32*)(IFX_MPS + 0x007C))
#define IFX_MPS_VPE0_2_VPE1_ICR                 ((volatile u32*)(IFX_MPS + 0x0080))
#define IFX_MPS_VPE0_2_VPE1_IRDR                ((volatile u32*)(IFX_MPS + 0x0084))
#define IFX_MPS_GIRDR                           ((volatile u32*)(IFX_MPS + 0x0088))
#define IFX_MPS_GICR                            ((volatile u32*)(IFX_MPS + 0x008C))
#define IFX_MPS_VPE0_NFICR                      ((volatile u32*)(IFX_MPS + 0x0090))
#define IFX_MPS_VPE1_NFICR                      ((volatile u32*)(IFX_MPS + 0x0094))
#define IFX_MPS_VPE0_BINSEM0                    ((volatile u32*)(IFX_MPS + 0x0100))
#define IFX_MPS_VPE1_BINSEM0                    ((volatile u32*)(IFX_MPS + 0x0200))

#define IFX_MPS_SRAM                            ((volatile u32*)(KSEG1 | 0x1F200000))

#define IFX_MPS_VCPU_FW_AD                      ((volatile u32*)(KSEG1 | 0x1F2001E0))

//YLH: 0x354 is FAB_LOT_ID0, 0x35C is chip configuration fuse register
//   
#define IFX_FUSE_ID_CFG                         ((volatile u32*)(KSEG1 | 0x1F107350))  
#define IFX_FUSE_BASE_ADDR                      (KSEG1 | 0x1F107354)

/************************************************************************/
/*   Module       :   XBAR Register definition                          */
/************************************************************************/
#define IFX_XBAR_REG_BASE                        (KSEG1 | 0x1F400000)

#define IFX_XBAR_ALWAYS_LAST                     (volatile u32*)(IFX_XBAR_REG_BASE + 0x430)
#define IFX_XBAR_FPI_BURST_EN                     0x00000002
#define IFX_XBAR_AHB_BURST_EN                     0x00000004
#define IFX_XBAR_DDR_SEL_EN                       0x00000001

/*
 *  Routine for Voice
 */
extern const void (*ifx_bsp_basic_mps_decrypt)(unsigned int addr, int n);

#endif /* HN1_H */
