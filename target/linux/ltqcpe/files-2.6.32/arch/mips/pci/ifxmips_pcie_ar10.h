/****************************************************************************
                              Copyright (c) 2010
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************/
/*!
  \file ifxmips_pcie_ar10.h
  \ingroup IFX_PCIE
  \brief PCIe RC driver ar10 specific file
*/

#ifndef IFXMIPS_PCIE_AR10_H
#define IFXMIPS_PCIE_AR10_H
#include <linux/types.h>
#include <linux/delay.h>

/* Project header file */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/common_routines.h>
#include <asm/ifx/ifx_pmu.h>
#include <asm/ifx/ifx_gpio.h>
#include <asm/ifx/ifx_ledc.h>

/* PCIe Address Mapping Base */
#if defined(CONFIG_IFX_PCIE_1ST_CORE)
#define PCIE_CFG_PHY_BASE        0x1D000000UL
#define PCIE_CFG_BASE           (KSEG1 + PCIE_CFG_PHY_BASE)
#define PCIE_CFG_SIZE           (8 * 1024 * 1024)

#define PCIE_MEM_PHY_BASE        0x1C000000UL
#define PCIE_MEM_BASE           (KSEG1 + PCIE_MEM_PHY_BASE)
#define PCIE_MEM_SIZE           (16 * 1024 * 1024)
#define PCIE_MEM_PHY_END        (PCIE_MEM_PHY_BASE + PCIE_MEM_SIZE - 1)

#define PCIE_IO_PHY_BASE         0x1D800000UL
#define PCIE_IO_BASE            (KSEG1 + PCIE_IO_PHY_BASE)
#define PCIE_IO_SIZE            (1 * 1024 * 1024)
#define PCIE_IO_PHY_END         (PCIE_IO_PHY_BASE + PCIE_IO_SIZE - 1)

#define PCIE_RC_CFG_BASE        (KSEG1 + 0x1D900000)
#define PCIE_APP_LOGIC_REG      (KSEG1 + 0x1E100900)
#define PCIE_MSI_PHY_BASE        0x1F600000UL

#define PCIE_PDI_PHY_BASE        0x1F106800UL
#define PCIE_PDI_BASE           (KSEG1 + PCIE_PDI_PHY_BASE)
#define PCIE_PDI_SIZE            0x200
#endif /* CONFIG_IFX_PCIE_1ST_CORE */

#if defined(CONFIG_IFX_PCIE_2ND_CORE)
#define PCIE1_CFG_PHY_BASE        0x19000000UL
#define PCIE1_CFG_BASE           (KSEG1 + PCIE1_CFG_PHY_BASE)
#define PCIE1_CFG_SIZE           (8 * 1024 * 1024)

#define PCIE1_MEM_PHY_BASE        0x18000000UL
#define PCIE1_MEM_BASE           (KSEG1 + PCIE1_MEM_PHY_BASE)
#define PCIE1_MEM_SIZE           (16 * 1024 * 1024)
#define PCIE1_MEM_PHY_END        (PCIE1_MEM_PHY_BASE + PCIE1_MEM_SIZE - 1)

#define PCIE1_IO_PHY_BASE         0x19800000UL
#define PCIE1_IO_BASE            (KSEG1 + PCIE1_IO_PHY_BASE)
#define PCIE1_IO_SIZE            (1 * 1024 * 1024)
#define PCIE1_IO_PHY_END         (PCIE1_IO_PHY_BASE + PCIE1_IO_SIZE - 1)

#define PCIE1_RC_CFG_BASE        (KSEG1 + 0x19900000)
#define PCIE1_APP_LOGIC_REG      (KSEG1 + 0x1E100700)
#define PCIE1_MSI_PHY_BASE        0x1F400000UL

#define PCIE1_PDI_PHY_BASE        0x1F700400UL
#define PCIE1_PDI_BASE           (KSEG1 + PCIE1_PDI_PHY_BASE)
#define PCIE1_PDI_SIZE            0x200
#endif /* CONFIG_IFX_PCIE_2ND_CORE */

#ifdef CONFIG_AR10_GRX390
#if defined(CONFIG_IFX_PCIE_3RD_CORE)
#define PCIE2_CFG_PHY_BASE        0x1A800000UL
#define PCIE2_CFG_BASE           (KSEG1 + PCIE2_CFG_PHY_BASE)
#define PCIE2_CFG_SIZE           (8 * 1024 * 1024)

#define PCIE2_MEM_PHY_BASE        0x1B000000UL
#define PCIE2_MEM_BASE           (KSEG1 + PCIE2_MEM_PHY_BASE)
#define PCIE2_MEM_SIZE           (16 * 1024 * 1024)
#define PCIE2_MEM_PHY_END        (PCIE2_MEM_PHY_BASE + PCIE2_MEM_SIZE - 1)

#define PCIE2_IO_PHY_BASE         0x19A00000UL
#define PCIE2_IO_BASE            (KSEG1 + PCIE2_IO_PHY_BASE)
#define PCIE2_IO_SIZE            (1 * 1024 * 1024)
#define PCIE2_IO_PHY_END         (PCIE2_IO_PHY_BASE + PCIE2_IO_SIZE - 1)

#define PCIE2_RC_CFG_BASE        (KSEG1 + 0x19B00000)
#define PCIE2_APP_LOGIC_REG      (KSEG1 + 0x1E100400)
#define PCIE2_MSI_PHY_BASE        0x1F700A00UL

#define PCIE2_PDI_PHY_BASE        0x1F106A00UL
#define PCIE2_PDI_BASE           (KSEG1 + PCIE2_PDI_PHY_BASE)
#define PCIE2_PDI_SIZE            0x200
#endif /* CONFIG_IFX_PCIE_3RD_CORE */
#endif /* CONFIG_AR10_GRX390 */

#if defined(CONFIG_IFX_PCIE_1ST_CORE) && defined(CONFIG_IFX_PCIE_2ND_CORE)\
    && defined(CONFIG_IFX_PCIE_3RD_CORE)
#define IFX_PCIE_PORT_MAX    3
#elif (defined(CONFIG_IFX_PCIE_1ST_CORE) && defined(CONFIG_IFX_PCIE_2ND_CORE)) || \
    (defined(CONFIG_IFX_PCIE_2ND_CORE) && defined(CONFIG_IFX_PCIE_3RD_CORE)) || \
    (defined(CONFIG_IFX_PCIE_1ST_CORE) && defined(CONFIG_IFX_PCIE_3RD_CORE))
#define IFX_PCIE_PORT_MAX    2
#elif defined(CONFIG_IFX_PCIE_1ST_CORE) || defined(CONFIG_IFX_PCIE_2ND_CORE)\
    || defined(CONFIG_IFX_PCIE_3RD_CORE)
#define IFX_PCIE_PORT_MAX    1
#else
#error "AR10/GRX390 not select any PCIe core!!!!"
#endif

/* Please note, LED shift bit reversed by hardware boards */
#if defined (CONFIG_AR10_FAMILY_BOARD_1_1) || defined (CONFIG_AR10_EVAL_BOARD)
#define PCIE_RC0_LED_RST  12
#define PCIE_RC1_LED_RST  11
#elif defined (CONFIG_AR10_FAMILY_BOARD_1_2) || defined (CONFIG_USE_EMULATOR)
#define PCIE_RC0_LED_RST  12
#define PCIE_RC1_LED_RST  11
#else
#error "AR10/GRX390 Board Version not defined!!!!"
#endif

#if defined (CONFIG_AR10_GRX390)
#define PCIE_RC2_LED_RST  1
#endif /* CONFIG_AR10_GRX390 */

//#define ARC_KPN_BOARD 1
//#if defined (ARC_KPN_BOARD)
#define CONFIG_IFX_PCIE_RST_EP_ACTIVE_HIGH 0
#define CONFIG_IFX_PCIE1_RST_EP_ACTIVE_HIGH 0
//#endif 

static const int pcie_port_to_rst_pin[] = {
    PCIE_RC0_LED_RST,
    PCIE_RC1_LED_RST,
#ifdef CONFIG_AR10_GRX390
    PCIE_RC2_LED_RST,
#endif
};


#ifdef CONFIG_QUALCOMM_AP_PPA
//#define CONFIG_IFX_PCIE_RST_EP_ACTIVE_HIGH
//#define ARC_LED_GPIO

#define AR10_LED                       0xBE100BB0
#define AR10_LED_CPU0                  ((volatile unsigned int *)(AR10_LED + 0x0008))
static inline void do_led_set_data(ret)
{
#ifdef CONFIG_IFX_PCIE_RST_EP_ACTIVE_HIGH
	if ( !ret ) *AR10_LED_CPU0 &= ~0x10;
#else
	if ( ret ) *AR10_LED_CPU0 &= ~0x10;
#endif
	else *AR10_LED_CPU0 |= 0x10;
}

static inline void pcie_ep_rst_init(int pcie_port)
{
 if (pcie_port == IFX_PCIE_PORT0)
 	do_led_set_data(0);
}
#else
static inline void pcie_ep_rst_init(int rc_port)
{
    if (rc_port == IFX_PCIE_PORT0) {
//    #ifdef CONFIG_IFX_PCIE_RST_EP_ACTIVE_HIGH
//        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 0);
//    #else
        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 1);
//    #endif
    }
    else if (rc_port == IFX_PCIE_PORT1) {
//    #ifdef CONFIG_IFX_PCIE1_RST_EP_ACTIVE_HIGH
//        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 0);
//    #else
        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 1);
//    #endif
    }
#ifdef CONFIG_AR10_GRX390
    else if (rc_port == IFX_PCIE_PORT2) {
    #ifdef CONFIG_IFX_PCIE2_RST_EP_ACTIVE_HIGH
        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 0);
    #else
        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 1);
    #endif
    }
#endif /* CONFIG_AR10_GRX390 */
    mdelay(1000);
}
#endif

static inline int pcie_rc_fused(int rc_port)
{
    u32 reg = IFX_REG_R32(IFX_FUSE_ID_CFG);
    if (rc_port == IFX_PCIE_PORT0) {
        return (reg & 0x00000400)? 1 : 0; /* Bit 10 */
    }
    else if (rc_port == IFX_PCIE_PORT1) {
        return (reg & 0x00000200)? 1 : 0; /* Bit 9*/
    }
#ifdef CONFIG_AR10_GRX390
    else if (rc_port == IFX_PCIE_PORT2) {
        return (reg & 0x00010000)? 1 : 0; /* Bit 16 */
    }
#endif /* CONFIG_AR10_GRX390 */
    else {
        IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
    }
}

#ifdef CONFIG_AR10_GRX390
static inline void pcie_ahb_arbiter_setup(void)
{
    u32 reg;
    ifx_chipid_t chipid;
    
    reg = IFX_REG_R32(IFX_RCU_RST_REQ2);
    reg &= ~IFX_RCU_PCIE_ARBITER_MASK;
    ifx_get_chipid(&chipid);
    switch(chipid.part_number) {
        case IFX_PARTNUM_GRX383:
            reg |= IFX_RCU_PCIE_ARBITER_RC0;
            break;
        case IFX_PARTNUM_GRX369:
        case IFX_PARTNUM_GRX389:
            reg |= IFX_RCU_PCIE_ARBITER_RC0_RC1_RC2;
            break;
        case IFX_PARTNUM_GRX387:
            reg |= IFX_RCU_PCIE_ARBITER_RC0_RC1;
            break;
        default:
            reg |= IFX_RCU_PCIE_ARBITER_RC0_RC1;
            break;
}
    IFX_REG_W32(reg, IFX_RCU_RST_REQ2);
}
#endif /* CONFIG_AR10_GRX390 */

static inline void pcie_rcu_endian_setup(int rc_port)
{
    u32 reg;

    reg = IFX_REG_R32(IFX_RCU_AHB_ENDIAN);
#ifdef CONFIG_AR10_GRX390
#if defined(CONFIG_IFX_PCIE_INBOUND_NO_HW_SWAP) \
    && defined(CONFIG_IFX_PCIE1_INBOUND_NO_HW_SWAP) \
    && defined(CONFIG_IFX_PCIE2_INBOUND_NO_HW_SWAP)
    /* Both inbound swap disabled */    
    reg &= ~IFX_RCU_BE_AHB4S;
#else
    /* Otherwise, we try to keep compatibility */
    reg |= IFX_RCU_BE_AHB4S;
#endif
#else /* CONFIG_AR10 */
#if defined(CONFIG_IFX_PCIE_INBOUND_NO_HW_SWAP) \
    && defined(CONFIG_IFX_PCIE1_INBOUND_NO_HW_SWAP)
    /* Both inbound swap disabled */    
    reg &= ~IFX_RCU_BE_AHB4S;
#else
    /* Otherwise, we try to keep compatibility */
    reg |= IFX_RCU_BE_AHB4S;
#endif
#endif /* CONFIG_AR10_GRX390 */
    /*
     * Outbound, common setting is little endian, PCI_S can be controlled seperately
     * according to sw swap requirement.
     */
    reg &= ~IFX_RCU_BE_AHB3M;
    if (rc_port == IFX_PCIE_PORT0) {
    #ifdef CONFIG_IFX_PCIE_INBOUND_NO_HW_SWAP
        reg &= ~IFX_RCU_BE_PCIE0M;
    #else
        reg |= IFX_RCU_BE_PCIE0M;
    #endif /* CONFIG_IFX_PCIE_INBOUND_NO_HW_SWAP */

    #ifdef CONFIG_IFX_PCIE_HW_SWAP
        /* Outbound, software swap needed */
        reg |= IFX_RCU_BE_PCIE0S;
    #else
        /* Outbound converteded to little endian, no swap needed */
        reg &= ~IFX_RCU_BE_PCIE0S;
    #endif
    }
    else if (rc_port == IFX_PCIE_PORT1){
    #ifdef CONFIG_IFX_PCIE1_INBOUND_NO_HW_SWAP
        reg &= ~IFX_RCU_BE_PCIE1M;
    #else
        reg |= IFX_RCU_BE_PCIE1M;
    #endif /* IFX_PCIE1_INBOUND_NO_HW_SWAP */
    
    #ifdef CONFIG_IFX_PCIE1_HW_SWAP
        /* Outbound, software swap needed */
        reg |= IFX_RCU_BE_PCIE1S;
    #else
        /* Outbound converteded to little endian, no swap needed */
        reg &= ~IFX_RCU_BE_PCIE1S;
    #endif /* CONFIG_IFX_PCIE1_HW_SWAP */
    }
#ifdef CONFIG_AR10_GRX390
    else if (rc_port == IFX_PCIE_PORT2){
    #ifdef CONFIG_IFX_PCIE2_INBOUND_NO_HW_SWAP
        reg &= ~IFX_RCU_BE_PCIE2M;
    #else
        reg |= IFX_RCU_BE_PCIE2M;
    #endif /* CONFIG_IFX_PCIE2_INBOUND_NO_HW_SWAP */
    
    #ifdef CONFIG_IFX_PCIE2_HW_SWAP
        /* Outbound, software swap needed */
        reg |= IFX_RCU_BE_PCIE2S;
    #else
        /* Outbound converteded to little endian, no swap needed */
        reg &= ~IFX_RCU_BE_PCIE2S;
    #endif /* CONFIG_IFX_PCIE2_HW_SWAP */
    }
#endif /* CONFIG_AR10_GRX390 */
    else {
        IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
    }

    IFX_REG_W32(reg, IFX_RCU_AHB_ENDIAN);
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s port %d IFX_RCU_AHB_ENDIAN: 0x%08x\n",
        __func__, rc_port, IFX_REG_R32(IFX_RCU_AHB_ENDIAN));
}

static inline void pcie_phy_pmu_enable(int rc_port)
{
    if (rc_port == IFX_PCIE_PORT0) { /* XXX, should use macro*/
        PCIE0_PHY_PMU_SETUP(IFX_PMU_ENABLE);
    }
    else if (rc_port == IFX_PCIE_PORT1) {
        PCIE1_PHY_PMU_SETUP(IFX_PMU_ENABLE);
    }
#ifdef CONFIG_AR10_GRX390
    else if (rc_port == IFX_PCIE_PORT2) {
        PCIE2_PHY_PMU_SETUP(IFX_PMU_ENABLE);
    }
#endif /* CONFIG_AR10_GRX390 */
    else {
        IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
    }
}

static inline void pcie_phy_pmu_disable(int rc_port)
{
    if (rc_port == IFX_PCIE_PORT0) { /* XXX, should use macro*/
        PCIE0_PHY_PMU_SETUP(IFX_PMU_DISABLE);
    }
    else if (rc_port == IFX_PCIE_PORT1) {
        PCIE1_PHY_PMU_SETUP(IFX_PMU_DISABLE);
    }
#ifdef CONFIG_AR10_GRX390
    else if (rc_port == IFX_PCIE_PORT2) {
        PCIE2_PHY_PMU_SETUP(IFX_PMU_DISABLE);
    }
#endif /* CONFIG_AR10_GRX390 */
    else {
        IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
    }
}

static inline void pcie_pdi_big_endian(int rc_port)
{
    u32 reg;

    reg = IFX_REG_R32(IFX_RCU_AHB_ENDIAN);
    if (rc_port == IFX_PCIE_PORT0) {
        /* Config AHB->PCIe and PDI endianness */
        reg |= IFX_RCU_BE_PCIE0_PDI;
    }
    else if (rc_port == IFX_PCIE_PORT1) {
        /* Config AHB->PCIe and PDI endianness */
        reg |= IFX_RCU_BE_PCIE1_PDI;
    }
#ifdef CONFIG_AR10_GRX390
    else if (rc_port == IFX_PCIE_PORT2) {
        /* Config AHB->PCIe and PDI endianness */
        reg |= IFX_RCU_BE_PCIE2_PDI;
    }
#endif /* CONFIG_AR10_GRX390 */
    else {
        IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
    }
    IFX_REG_W32(reg, IFX_RCU_AHB_ENDIAN);
}

static inline void pcie_pdi_pmu_enable(int rc_port)
{
    if (rc_port == IFX_PCIE_PORT0) {
        /* Enable PDI to access PCIe PHY register */
        PDI0_PMU_SETUP(IFX_PMU_ENABLE);
    }
    else if (rc_port == IFX_PCIE_PORT1) {
        PDI1_PMU_SETUP(IFX_PMU_ENABLE);
    }
#ifdef CONFIG_AR10_GRX390
    else if (rc_port == IFX_PCIE_PORT2) {
        PDI2_PMU_SETUP(IFX_PMU_ENABLE);
    }
#endif /* CONFIG_AR10_GRX390 */
    else {
        IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
    }
}

static inline void pcie_core_rst_assert(int rc_port)
{
    u32 reg;

    /* Reset Core, bit 22 */
    if (rc_port == IFX_PCIE_PORT0) {
        reg = IFX_REG_R32(IFX_RCU_RST_REQ);
        reg |= 0x00400000;
        IFX_REG_W32(reg, IFX_RCU_RST_REQ);
    }
    else if (rc_port == IFX_PCIE_PORT1) {
        reg = IFX_REG_R32(IFX_RCU_RST_REQ);
        reg |= 0x08000000; /* Bit 27 */
        IFX_REG_W32(reg, IFX_RCU_RST_REQ);
    }
#ifdef CONFIG_AR10_GRX390
    else if (rc_port == IFX_PCIE_PORT2) {
        reg = IFX_REG_R32(IFX_RCU_RST_REQ2);
        reg |= 0x10000000; /* Bit 28 */
        IFX_REG_W32(reg, IFX_RCU_RST_REQ2);
    }
#endif /* CONFIG_AR10_GRX390 */
    else {
        IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
    }
}

static inline void pcie_core_rst_deassert(int rc_port)
{
    u32 reg;

    /* Make sure one micro-second delay */
    udelay(1);

    if (rc_port == IFX_PCIE_PORT0) {
    reg = IFX_REG_R32(IFX_RCU_RST_REQ);
        reg &= ~0x00400000; /* Bit 22 */
        IFX_REG_W32(reg, IFX_RCU_RST_REQ);
    }
    else if (rc_port == IFX_PCIE_PORT1) {
        reg = IFX_REG_R32(IFX_RCU_RST_REQ);
        reg &= ~0x08000000; /* Bit 27 */
        IFX_REG_W32(reg, IFX_RCU_RST_REQ);
    }
#ifdef CONFIG_AR10_GRX390
    else if (rc_port == IFX_PCIE_PORT2) {
        reg = IFX_REG_R32(IFX_RCU_RST_REQ2);
        reg &= ~0x10000000; /* Bit 28 */
        IFX_REG_W32(reg, IFX_RCU_RST_REQ2);
    }
#endif /* CONFIG_AR10_GRX390 */
    else {
        IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
    }
}

static inline void pcie_phy_rst_assert(int rc_port)
{
    u32 reg;

    if (rc_port == IFX_PCIE_PORT0) {
    reg = IFX_REG_R32(IFX_RCU_RST_REQ);
        reg |= 0x00001000; /* Bit 12 */
        IFX_REG_W32(reg, IFX_RCU_RST_REQ);
    }
    else if (rc_port == IFX_PCIE_PORT1) {
        reg = IFX_REG_R32(IFX_RCU_RST_REQ);
        reg |= 0x00002000; /* Bit 13 */
        IFX_REG_W32(reg, IFX_RCU_RST_REQ);
    }
#ifdef CONFIG_AR10_GRX390
    else if (rc_port == IFX_PCIE_PORT2) {
        reg = IFX_REG_R32(IFX_RCU_RST_REQ2);
        reg |= 0x20000000; /* Bit 29 */
        IFX_REG_W32(reg, IFX_RCU_RST_REQ2);
    }
#endif /* CONFIG_AR10_GRX390 */
    else {
        IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
    }
}

static inline void pcie_phy_rst_deassert(int rc_port)
{
    u32 reg;

    /* Make sure one micro-second delay */
    udelay(1);

    reg = IFX_REG_R32(IFX_RCU_RST_REQ);
    if (rc_port == IFX_PCIE_PORT0) {
        reg = IFX_REG_R32(IFX_RCU_RST_REQ);
        reg &= ~0x00001000; /* Bit 12 */
        IFX_REG_W32(reg, IFX_RCU_RST_REQ);
    }
    else if (rc_port == IFX_PCIE_PORT1) {
        reg = IFX_REG_R32(IFX_RCU_RST_REQ);
        reg &= ~0x00002000; /* Bit 13 */
        IFX_REG_W32(reg, IFX_RCU_RST_REQ);
    }
#ifdef CONFIG_AR10_GRX390
    else if (rc_port == IFX_PCIE_PORT2) {
        reg = IFX_REG_R32(IFX_RCU_RST_REQ2);
        reg &= ~0x20000000; /* Bit 29 */
        IFX_REG_W32(reg, IFX_RCU_RST_REQ2);
    }
#endif /* CONFIG_AR10_GRX390 */
    else {
        IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
    }
}

#ifdef CONFIG_QUALCOMM_AP_PPA
static inline void pcie_device_rst_assert(int pcie_port)
{
	if (pcie_port == IFX_PCIE_PORT0)
 	 do_led_set_data(1);
}
#else
static inline void pcie_device_rst_assert(int rc_port)
{
    if (rc_port == IFX_PCIE_PORT0) {
//    #ifdef CONFIG_IFX_PCIE_RST_EP_ACTIVE_HIGH
//        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 1);
//    #else
        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 0);
//    #endif
    }
    else if (rc_port == IFX_PCIE_PORT1) {
//    #ifdef CONFIG_IFX_PCIE1_RST_EP_ACTIVE_HIGH
//        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 1);
//    #else
        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 0);
//    #endif
    }
#ifdef CONFIG_AR10_GRX390
    else if (rc_port == IFX_PCIE_PORT2) {
    #ifdef CONFIG_IFX_PCIE2_RST_EP_ACTIVE_HIGH
        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 1);
    #else
        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 0);
    #endif
    }
#endif /* CONFIG_AR10_GRX390 */
    else {
        IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
    }
}
#endif

#ifdef CONFIG_QUALCOMM_AP_PPA
static inline void pcie_device_rst_deassert(int pcie_port)
{
	if (pcie_port == IFX_PCIE_PORT0) {
  /* Some devices need more than 200ms reset time */
  //mdelay(200);
  //anne: for KPN 389, long delay is necessary for reset PCIe.
  mdelay(400);
  do_led_set_data(0);
	}
}
#else
static inline void pcie_device_rst_deassert(int rc_port)
{
    /* Some devices need more than 200ms reset time */
    mdelay(600);
    if (rc_port == IFX_PCIE_PORT0) {
//    #ifdef CONFIG_IFX_PCIE_RST_EP_ACTIVE_HIGH
//        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 0);
//    #else
        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 1);
//    #endif
    }
    else if (rc_port == IFX_PCIE_PORT1) {
//    #ifdef CONFIG_IFX_PCIE1_RST_EP_ACTIVE_HIGH
//        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 0);
//    #else
        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 1);
//    #endif
    }
#ifdef CONFIG_AR10_GRX390
    else if (rc_port == IFX_PCIE_PORT2) {
    #ifdef CONFIG_IFX_PCIE2_RST_EP_ACTIVE_HIGH
        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 0);
    #else
        ifx_ledc_set_data(pcie_port_to_rst_pin[rc_port], 1);
    #endif
    }
#endif /* CONFIG_AR10_GRX390 */
    else {
        IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
    }
}
#endif

static inline void pcie_core_pmu_setup(int rc_port)
{
    if (rc_port == IFX_PCIE_PORT0) {
        PCIE0_CTRL_PMU_SETUP(IFX_PMU_ENABLE);
    }
    else if (rc_port == IFX_PCIE_PORT1) {
        PCIE1_CTRL_PMU_SETUP(IFX_PMU_ENABLE); 
    }
#ifdef CONFIG_AR10_GRX390
    else if (rc_port == IFX_PCIE_PORT2) {
        PCIE2_CTRL_PMU_SETUP(IFX_PMU_ENABLE);
    }
#endif /* CONFIG_AR10_GRX390 */
    else {
        IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
    }
}

static inline void pcie_msi_init(int pcie_port)
{
    int rc_port = pcie_port_idx_to_rc_port(pcie_port);

    if (rc_port == IFX_PCIE_PORT0) {
        MSI0_PMU_SETUP(IFX_PMU_ENABLE);
    }
    else if (rc_port == IFX_PCIE_PORT1){
        MSI1_PMU_SETUP(IFX_PMU_ENABLE);
    }
#ifdef CONFIG_AR10_GRX390
    else if (rc_port == IFX_PCIE_PORT2){
        MSI2_PMU_SETUP(IFX_PMU_ENABLE);
    }
#endif /* CONFIG_AR10_GRX390 */
    else {
        IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
    }
    pcie_msi_pic_init(pcie_port);
}
#endif /* IFXMIPS_PCIE_AR10_H */

