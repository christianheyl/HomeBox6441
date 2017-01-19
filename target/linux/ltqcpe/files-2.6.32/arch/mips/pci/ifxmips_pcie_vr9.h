/****************************************************************************
                              Copyright (c) 2010
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************/
/*!
  \file ifxmips_pcie_vr9.h
  \ingroup IFX_PCIE
  \brief PCIe RC driver vr9 specific file
*/

#ifndef IFXMIPS_PCIE_VR9_H
#define IFXMIPS_PCIE_VR9_H

#include <linux/types.h>
#include <linux/delay.h>

/* Project header file */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_pmu.h>
#include <asm/ifx/ifx_gpio.h>

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

#ifdef CONFIG_IFX_PCIE_1ST_CORE
#define IFX_PCIE_PORT_MAX    1
#else
#error "VR9 not select any PCIe Core!!!!"
#endif /* CONFIG_IFX_PCIE_1ST_CORE */

#define IFX_PCIE_GPIO_RESET  38


static const int ifx_pcie_gpio_module_id = IFX_GPIO_MODULE_PCIE;

static inline void pcie_ep_rst_init(int rc_port)
{
    ifx_gpio_pin_reserve(IFX_PCIE_GPIO_RESET, ifx_pcie_gpio_module_id);
#ifdef CONFIG_IFX_PCIE_RST_EP_ACTIVE_HIGH
    ifx_gpio_output_clear(IFX_PCIE_GPIO_RESET, ifx_pcie_gpio_module_id);
#else
    ifx_gpio_output_set(IFX_PCIE_GPIO_RESET, ifx_pcie_gpio_module_id);
#endif
    ifx_gpio_dir_out_set(IFX_PCIE_GPIO_RESET, ifx_pcie_gpio_module_id);
    ifx_gpio_altsel0_clear(IFX_PCIE_GPIO_RESET, ifx_pcie_gpio_module_id);
    ifx_gpio_altsel1_clear(IFX_PCIE_GPIO_RESET, ifx_pcie_gpio_module_id);
    ifx_gpio_open_drain_set(IFX_PCIE_GPIO_RESET, ifx_pcie_gpio_module_id);
    mdelay(100);
}

static inline int pcie_rc_fused(int rc_port)
{
    return (IFX_REG_R32(IFX_FUSE_ID_CFG) & 0x00000400)? 1 : 0;
}

static inline void pcie_ahb_pmu_setup(void) 
{
    /* Enable AHB bus master/slave */
    AHBM_PMU_SETUP(IFX_PMU_ENABLE);
    AHBS_PMU_SETUP(IFX_PMU_ENABLE);
}

static inline void pcie_rcu_endian_setup(int rc_port)
{
    u32 reg;

    reg = IFX_REG_R32(IFX_RCU_AHB_ENDIAN);

    /* Outbound configuration*/
#ifdef CONFIG_IFX_PCIE_HW_SWAP
    reg |= IFX_RCU_AHB_BE_PCIE_S;
#else
    reg &= ~IFX_RCU_AHB_BE_PCIE_S;
#endif /* CONFIG_IFX_PCIE_HW_SWAP */    
    /* Shared with USIF, don't touch */
    reg &= ~IFX_RCU_AHB_BE_XBAR_M;

    /* Inbound confiugration */
#ifdef CONFIG_IFX_PCIE_INBOUND_NO_HW_SWAP
    reg &= ~IFX_RCU_AHB_BE_PCIE_M;
#else
    reg |= IFX_RCU_AHB_BE_PCIE_M;
#endif /* CONFIG_IFX_PCIE_INBOUND_NO_HW_SWAP */
    /* Other bit shared with USB and DSL, don't touch */
    IFX_REG_W32(reg, IFX_RCU_AHB_ENDIAN);
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s IFX_RCU_AHB_ENDIAN: 0x%08x\n", __func__, IFX_REG_R32(IFX_RCU_AHB_ENDIAN));
}

static inline void pcie_phy_pmu_enable(int rc_port)
{
    PCIE_PHY_PMU_SETUP(IFX_PMU_ENABLE);
}

static inline void pcie_phy_pmu_disable(int rc_port)
{
    PCIE_PHY_PMU_SETUP(IFX_PMU_DISABLE);
}

static inline void pcie_pdi_big_endian(int rc_port)
{
    u32 reg;

    /* SRAM2PDI endianness control. */
    reg = IFX_REG_R32(IFX_RCU_AHB_ENDIAN);
    /* Config AHB->PCIe and PDI endianness */
    reg |= IFX_RCU_AHB_BE_PCIE_PDI;
    IFX_REG_W32(reg, IFX_RCU_AHB_ENDIAN);
}

static inline void pcie_pdi_pmu_enable(int rc_port)
{
    /* Enable PDI to access PCIe PHY register */
    PDI_PMU_SETUP(IFX_PMU_ENABLE);
}

static inline void pcie_core_rst_assert(int rc_port)
{
    u32 reg;

    reg = IFX_REG_R32(IFX_RCU_RST_REQ);

    /* Reset PCIe PHY & Core, bit 22, bit 26 may be affected if write it directly  */
    reg |= 0x00400000;
    IFX_REG_W32(reg, IFX_RCU_RST_REQ);
}

static inline void pcie_core_rst_deassert(int rc_port)
{
    u32 reg;

    /* Make sure one micro-second delay */
    udelay(1);

    /* Reset PCIe PHY & Core, bit 22 */
    reg = IFX_REG_R32(IFX_RCU_RST_REQ);
    reg &= ~0x00400000;
    IFX_REG_W32(reg, IFX_RCU_RST_REQ);
}

static inline void pcie_phy_rst_assert(int rc_port)
{
    u32 reg;

    reg = IFX_REG_R32(IFX_RCU_RST_REQ);
    reg |= 0x00001000; /* Bit 12 */
    IFX_REG_W32(reg, IFX_RCU_RST_REQ);
}

static inline void pcie_phy_rst_deassert(int rc_port)
{
    u32 reg;

    /* Make sure one micro-second delay */
    udelay(1);

    reg = IFX_REG_R32(IFX_RCU_RST_REQ);
    reg &= ~0x00001000; /* Bit 12 */
    IFX_REG_W32(reg, IFX_RCU_RST_REQ);
}

static inline void pcie_device_rst_assert(int rc_port)
{
#ifdef CONFIG_IFX_PCIE_RST_EP_ACTIVE_HIGH
    ifx_gpio_output_set(IFX_PCIE_GPIO_RESET, ifx_pcie_gpio_module_id);
#else
    ifx_gpio_output_clear(IFX_PCIE_GPIO_RESET, ifx_pcie_gpio_module_id);
#endif
}

static inline void pcie_device_rst_deassert(int rc_port)
{
    /* Some devices need more than 200ms reset time */
    mdelay(200);
#ifdef CONFIG_IFX_PCIE_RST_EP_ACTIVE_HIGH
    ifx_gpio_output_clear(IFX_PCIE_GPIO_RESET, ifx_pcie_gpio_module_id);
#else
    ifx_gpio_output_set(IFX_PCIE_GPIO_RESET, ifx_pcie_gpio_module_id);
#endif
}

static inline void pcie_core_pmu_setup(int rc_port)
{
    /* PCIe Core controller enabled */
    PCIE_CTRL_PMU_SETUP(IFX_PMU_ENABLE);

    /* Enable PCIe L0 Clock */
    PCIE_L0_CLK_PMU_SETUP(IFX_PMU_ENABLE);
}

static inline void pcie_msi_init(int pcie_port)
{
    MSI_PMU_SETUP(IFX_PMU_ENABLE);
    pcie_msi_pic_init(pcie_port);
}

#endif /* IFXMIPS_PCIE_VR9_H */

