/******************************************************************************
**
** FILE NAME    : ifxmips_pcie.c
** PROJECT      : IFX UEIP for VRX200
** MODULES      : PCI MSI sub module
**
** DATE         : 02 Mar 2009
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
** 0.0.1    02 Mar,2009  Lei Chuanhua    Initial version
*******************************************************************************/
 /*!
  \file ifxmips_pcie.c
  \ingroup IFX_PCIE
  \brief PCI express bus driver source file
*/
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <asm/paccess.h>
#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/pci.h>
#include <linux/pci_regs.h>

/* Project header file */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>
#include <asm/irq.h>
#include <asm/ifx/ifx_pmu.h>

#include "ifxmips_pcie.h"
#include "ifxmips_pcie_reg.h"

#define IFX_PCIE_VER_MAJOR          2
#define IFX_PCIE_VER_MID            0
#define IFX_PCIE_VER_MINOR          0
//#define IFX_PCIE_PHY_DBG

/* Enable 32bit io due to its mem mapped io nature */
#define IFX_PCIE_IO_32BIT

static int ifx_pcie_read_config(struct pci_bus *bus, u32 devfn, 
                    int where, int size, u32 *value);
static int ifx_pcie_write_config(struct pci_bus *bus, u32 devfn,
                  int where, int size, u32 value);

u32 g_pcie_debug_flag = PCIE_MSG_ANY & (~PCIE_MSG_CFG);

#if defined(CONFIG_IFX_PCIE_1ST_CORE)
static struct resource ifx_pcie_io_resource = {
    .name   = "PCIe0 I/O space",
    .start  = PCIE_IO_PHY_BASE,
    .end    = PCIE_IO_PHY_END,
    .flags  = IORESOURCE_IO,
};

static struct resource ifx_pcie_mem_resource = {
    .name   = "PCIe0 Memory space",
    .start  = PCIE_MEM_PHY_BASE,
    .end    = PCIE_MEM_PHY_END,
    .flags  = IORESOURCE_MEM,
};
#endif /* CONFIG_IFX_PCIE_1ST_CORE */

static struct pci_ops ifx_pcie_ops = {
    .read  = ifx_pcie_read_config,
    .write = ifx_pcie_write_config,
};

#if  defined(CONFIG_IFX_PCIE_2ND_CORE)
static struct resource ifx_pcie1_io_resource = {
    .name   = "PCIe1 I/O space",
    .start  = PCIE1_IO_PHY_BASE,
    .end    = PCIE1_IO_PHY_END,
    .flags  = IORESOURCE_IO,
};

static struct resource ifx_pcie1_mem_resource = {
    .name   = "PCIe1 Memory space",
    .start  = PCIE1_MEM_PHY_BASE,
    .end    = PCIE1_MEM_PHY_END,
    .flags  = IORESOURCE_MEM,
};
#endif /* CONFIG_IFX_PCIE_2ND_CORE */

#if defined(CONFIG_IFX_PCIE_3RD_CORE)
static struct resource ifx_pcie2_io_resource = {
    .name   = "PCIe2 I/O space",
    .start  = PCIE2_IO_PHY_BASE,
    .end    = PCIE2_IO_PHY_END,
    .flags  = IORESOURCE_IO,
};

static struct resource ifx_pcie2_mem_resource = {
    .name   = "PCIe2 Memory space",
    .start  = PCIE2_MEM_PHY_BASE,
    .end    = PCIE2_MEM_PHY_END,
    .flags  = IORESOURCE_MEM,
};
#endif /* CONFIG_IFX_PCIE_3RD_CORE */

ifx_pcie_port_t g_pcie_port_defs[IFX_PCIE_PORT_MAX] = {
#if defined(CONFIG_IFX_PCIE_1ST_CORE)
{
    .port_to_addr = {
        .cfg_base       = PCIE_CFG_BASE,
        .mem_base       = PCIE_MEM_BASE,
        .io_base        = PCIE_IO_BASE,
        .mem_phy_base   = PCIE_MEM_PHY_BASE,
        .mem_phy_end    = PCIE_MEM_PHY_END,
        .io_phy_base    = PCIE_IO_PHY_BASE,
        .io_phy_end     = PCIE_IO_PHY_END,
        .app_logic_base = PCIE_APP_LOGIC_REG,
        .rc_addr_base   = PCIE_RC_CFG_BASE,
        .phy_base       = PCIE_PDI_BASE,
     },
    .controller = {
        .pcic = {
            .pci_ops      = &ifx_pcie_ops,
            .mem_resource = &ifx_pcie_mem_resource,
            .io_resource  = &ifx_pcie_io_resource,
        },
        .port = IFX_PCIE_PORT0,
    },
    .legacy_irqs = {
        .ir_irq = {
            .irq  = IFX_PCIE_IR,
            .name = "ifx_pcie_rc0",
        },

        .legacy_irq = {
            {
                .irq_bit = PCIE_IRN_INTA,
                .irq     = IFX_PCIE_INTA,
            },
            {
                .irq_bit = PCIE_IRN_INTB,
                .irq     = IFX_PCIE_INTB,
            },
            {
                .irq_bit = PCIE_IRN_INTC,
                .irq     = IFX_PCIE_INTC,
            },
            {
                .irq_bit = PCIE_IRN_INTD,
                .irq     = IFX_PCIE_INTD,
            },
        },
    },
    .msi_irqs = {
        .msi_pic_p = (const volatile ifx_msi_pic_t)IFX_MSI_PIC_REG_BASE,
        .msi_phy_base = PCIE_MSI_PHY_BASE,
        .msi_irq_idx = {
            {IFX_PCIE_MSI_IR0, IFX_PCIE_MSI_IDX0}, {IFX_PCIE_MSI_IR1, IFX_PCIE_MSI_IDX1},
            {IFX_PCIE_MSI_IR2, IFX_PCIE_MSI_IDX2}, {IFX_PCIE_MSI_IR3, IFX_PCIE_MSI_IDX3},
            {IFX_PCIE_MSI_IR0, IFX_PCIE_MSI_IDX0}, {IFX_PCIE_MSI_IR1, IFX_PCIE_MSI_IDX1},
            {IFX_PCIE_MSI_IR2, IFX_PCIE_MSI_IDX2}, {IFX_PCIE_MSI_IR3, IFX_PCIE_MSI_IDX3},
            {IFX_PCIE_MSI_IR0, IFX_PCIE_MSI_IDX0}, {IFX_PCIE_MSI_IR1, IFX_PCIE_MSI_IDX1},
            {IFX_PCIE_MSI_IR2, IFX_PCIE_MSI_IDX2}, {IFX_PCIE_MSI_IR3, IFX_PCIE_MSI_IDX3},
            {IFX_PCIE_MSI_IR0, IFX_PCIE_MSI_IDX0}, {IFX_PCIE_MSI_IR1, IFX_PCIE_MSI_IDX1},
            {IFX_PCIE_MSI_IR2, IFX_PCIE_MSI_IDX2}, {IFX_PCIE_MSI_IR3, IFX_PCIE_MSI_IDX3},
        },
        .msi_free_irq_bitmask = 0,
        .msi_multiple_irq_bitmask= 0,
    },
    .rc_physical_port = IFX_PCIE_PORT0,
},
#endif /* CONFIG_IFX_PCIE_1ST_CORE */

#if defined(CONFIG_IFX_PCIE_2ND_CORE)
{
    .port_to_addr = {
        .cfg_base       = PCIE1_CFG_BASE,
        .mem_base       = PCIE1_MEM_BASE,
        .io_base        = PCIE1_IO_BASE,
        .mem_phy_base   = PCIE1_MEM_PHY_BASE,
        .mem_phy_end    = PCIE1_MEM_PHY_END,
        .io_phy_base    = PCIE1_IO_PHY_BASE,
        .io_phy_end     = PCIE1_IO_PHY_END,
        .app_logic_base = PCIE1_APP_LOGIC_REG,
        .rc_addr_base   = PCIE1_RC_CFG_BASE,
        .phy_base       = PCIE1_PDI_BASE,
     },
    .controller = {
        .pcic = {
            .pci_ops      = &ifx_pcie_ops,
            .mem_resource = &ifx_pcie1_mem_resource,
            .io_resource  = &ifx_pcie1_io_resource,
        },
        .port = IFX_PCIE_PORT1,
    },
    .legacy_irqs = {
        .ir_irq = {
            .irq  = IFX_PCIE1_IR,
            .name = "ifx_pcie_rc1",
        },

        .legacy_irq = {
            {
                .irq_bit = PCIE_IRN_INTA,
                .irq     = IFX_PCIE1_INTA,
            },
            {
                .irq_bit = PCIE_IRN_INTB,
                .irq     = IFX_PCIE1_INTB,
            },
            {
                .irq_bit = PCIE_IRN_INTC,
                .irq     = IFX_PCIE1_INTC,
            },
            {
                .irq_bit = PCIE_IRN_INTD,
                .irq     = IFX_PCIE1_INTD,
            },
        },
    },
    .msi_irqs = {
        .msi_pic_p = (const volatile ifx_msi_pic_t)IFX_MSI1_PIC_REG_BASE,
        .msi_phy_base = PCIE1_MSI_PHY_BASE,
        .msi_irq_idx = {
            {IFX_PCIE1_MSI_IR0, IFX_PCIE_MSI_IDX0}, {IFX_PCIE1_MSI_IR1, IFX_PCIE_MSI_IDX1},
            {IFX_PCIE1_MSI_IR2, IFX_PCIE_MSI_IDX2}, {IFX_PCIE1_MSI_IR3, IFX_PCIE_MSI_IDX3},
            {IFX_PCIE1_MSI_IR0, IFX_PCIE_MSI_IDX0}, {IFX_PCIE1_MSI_IR1, IFX_PCIE_MSI_IDX1},
            {IFX_PCIE1_MSI_IR2, IFX_PCIE_MSI_IDX2}, {IFX_PCIE1_MSI_IR3, IFX_PCIE_MSI_IDX3},
            {IFX_PCIE1_MSI_IR0, IFX_PCIE_MSI_IDX0}, {IFX_PCIE1_MSI_IR1, IFX_PCIE_MSI_IDX1},
            {IFX_PCIE1_MSI_IR2, IFX_PCIE_MSI_IDX2}, {IFX_PCIE1_MSI_IR3, IFX_PCIE_MSI_IDX3},
            {IFX_PCIE1_MSI_IR0, IFX_PCIE_MSI_IDX0}, {IFX_PCIE1_MSI_IR1, IFX_PCIE_MSI_IDX1},
            {IFX_PCIE1_MSI_IR2, IFX_PCIE_MSI_IDX2}, {IFX_PCIE1_MSI_IR3, IFX_PCIE_MSI_IDX3},
        },
        .msi_free_irq_bitmask = 0,
        .msi_multiple_irq_bitmask= 0,
    },
    .rc_physical_port = IFX_PCIE_PORT1,
},
#endif /* CONFIG_IFX_PCIE_2ND_CORE */

/* Third core */
#if defined(CONFIG_IFX_PCIE_3RD_CORE)
{
    .port_to_addr = {
        .cfg_base       = PCIE2_CFG_BASE,
        .mem_base       = PCIE2_MEM_BASE,
        .io_base        = PCIE2_IO_BASE,
        .mem_phy_base   = PCIE2_MEM_PHY_BASE,
        .mem_phy_end    = PCIE2_MEM_PHY_END,
        .io_phy_base    = PCIE2_IO_PHY_BASE,
        .io_phy_end     = PCIE2_IO_PHY_END,
        .app_logic_base = PCIE2_APP_LOGIC_REG,
        .rc_addr_base   = PCIE2_RC_CFG_BASE,
        .phy_base       = PCIE2_PDI_BASE,
     },
    .controller = {
        .pcic = {
            .pci_ops      = &ifx_pcie_ops,
            .mem_resource = &ifx_pcie2_mem_resource,
            .io_resource  = &ifx_pcie2_io_resource,
        },
        .port = IFX_PCIE_PORT2,
    },
    .legacy_irqs = {
        .ir_irq = {
            .irq  = IFX_PCIE2_IR,
            .name = "ifx_pcie_rc2",
        },

        .legacy_irq = {
            {
                .irq_bit = PCIE_IRN_INTA,
                .irq     = IFX_PCIE2_INTA,
            },
            {
                .irq_bit = PCIE_IRN_INTB,
                .irq     = IFX_PCIE2_INTB,
            },
            {
                .irq_bit = PCIE_IRN_INTC,
                .irq     = IFX_PCIE2_INTC,
            },
            {
                .irq_bit = PCIE_IRN_INTD,
                .irq     = IFX_PCIE2_INTD,
            },
        },
    },
    .msi_irqs = {
        .msi_pic_p = (const volatile ifx_msi_pic_t)IFX_MSI2_PIC_REG_BASE,
        .msi_phy_base = PCIE2_MSI_PHY_BASE,
        .msi_irq_idx = {
            {IFX_PCIE2_MSI_IR0, IFX_PCIE_MSI_IDX0}, {IFX_PCIE2_MSI_IR1, IFX_PCIE_MSI_IDX1},
            {IFX_PCIE2_MSI_IR2, IFX_PCIE_MSI_IDX2}, {IFX_PCIE2_MSI_IR3, IFX_PCIE_MSI_IDX3},
            {IFX_PCIE2_MSI_IR0, IFX_PCIE_MSI_IDX0}, {IFX_PCIE2_MSI_IR1, IFX_PCIE_MSI_IDX1},
            {IFX_PCIE2_MSI_IR2, IFX_PCIE_MSI_IDX2}, {IFX_PCIE2_MSI_IR3, IFX_PCIE_MSI_IDX3},
            {IFX_PCIE2_MSI_IR0, IFX_PCIE_MSI_IDX0}, {IFX_PCIE2_MSI_IR1, IFX_PCIE_MSI_IDX1},
            {IFX_PCIE2_MSI_IR2, IFX_PCIE_MSI_IDX2}, {IFX_PCIE2_MSI_IR3, IFX_PCIE_MSI_IDX3},
            {IFX_PCIE2_MSI_IR0, IFX_PCIE_MSI_IDX0}, {IFX_PCIE2_MSI_IR1, IFX_PCIE_MSI_IDX1},
            {IFX_PCIE2_MSI_IR2, IFX_PCIE_MSI_IDX2}, {IFX_PCIE2_MSI_IR3, IFX_PCIE_MSI_IDX3},
        },
        .msi_free_irq_bitmask = 0,
        .msi_multiple_irq_bitmask= 0,
    },
    .rc_physical_port = IFX_PCIE_PORT2,
},
#endif /* CONFIG_IFX_PCIE_3RD_CORE */
};

void 
ifx_pcie_debug(const char *fmt, ...)
{
    static char buf[256] = {0};      /* XXX */
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap); 

    printk("%s", buf);
}

#ifdef IFX_PCI_PHY_DBG
/* Generate hot reset, XXX must catpure to verify */
static INLINE void 
pcie_secondary_bus_reset(int pcie_port)
{
    int i;
    u32 reg;
#define IFX_PCIE_RESET_TIME   20

    /* Assert Secondary Bus Reset */
    reg = IFX_REG_R32(PCIE_INTRBCTRL(pcie_port));
    reg |= PCIE_INTRBCTRL_RST_SECONDARY_BUS;
    IFX_REG_W32(reg, PCIE_INTRBCTRL(pcie_port));

    /* De-assert Secondary Bus Reset */
    reg &= ~PCIE_INTRBCTRL_RST_SECONDARY_BUS;
    IFX_REG_W32(reg, PCIE_INTRBCTRL(pcie_port));

    /* XXX, wait at least 100 ms, then restore again */
    for (i = 0; i < IFX_PCIE_RESET_TIME; i++) {
        mdelay(10);
    }
#undef IFX_PCIE_RESET_TIME
}

/* Error or L0s to L0 */
static INLINE int 
pcie_retrain_link(int pcie_port)
{
    int i;
    u32 reg;
#define IFX_PCIE_RETRAIN_TIME  1000

    reg = IFX_REG_R32(PCIE_LCTLSTS(pcie_port));
    reg |= PCIE_LCTLSTS_RETRIAN_LINK;
    IFX_REG_W32(reg, PCIE_LCTLSTS(pcie_port));

    /* Wait for the link to come up */
    for (i = 0; i < IFX_PCIE_RETRAIN_TIME; i++) {
        if (!(IFX_REG_R32(PCIE_LCTLSTS(pcie_port)) & PCIE_LCTLSTS_RETRAIN_PENDING)) {
            break;
        }
        udelay(100);
    }
    if (i >= IFX_PCIE_RETRAIN_TIME) {
        IFX_PCIE_PRINT(PCIE_MSG_INIT, "%s retrain timeout\n", __func__);
        return -1;
    }
    return 0;
#undef IFX_PCIE_RETRAIN_TIME
}

static INLINE void 
pcie_disable_scrambling(int pcie_port)
{
    u32 reg;

    reg = IFX_REG_R32(PCIE_PLCR(pcie_port));
    reg |= PCIE_PLCR_SCRAMBLE_DISABLE;
    IFX_REG_W32(reg, PCIE_PLCR(pcie_port));
}
#endif /* IFX_PCI_PHY_DBG */

/* Physical to logical */
int pcie_rc_port_to_pcie_port(int rc_port, int *pcie_port)
{
    int i;

    for (i = IFX_PCIE_PORT0; i < IFX_PCIE_PORT_MAX; i++) {
        if (g_pcie_port_defs[i].rc_physical_port == rc_port) {
            *pcie_port = i;
            return 0;
        }
    }
    if (i >= IFX_PCIE_PORT_MAX) {
        printk(KERN_ERR "%s invalid pcie physical port %d to logical port index\n",
            __func__, rc_port);
        return -EINVAL;
    }
    return 0;
}

int pcie_port_idx_to_rc_port(int pcie_port)
{
    return g_pcie_port_defs[pcie_port].rc_physical_port;
}

static INLINE int 
pcie_ltssm_enable(int pcie_port) 
{
    int i;
#define IFX_PCIE_LTSSM_ENABLE_TIMEOUT 10

    IFX_REG_W32(PCIE_RC_CCR_LTSSM_ENABLE, PCIE_RC_CCR(pcie_port)); /* Enable LTSSM */

    /* Wait for the link to come up */
    for (i = 0; i < IFX_PCIE_LTSSM_ENABLE_TIMEOUT; i++) {
        if (!(IFX_REG_R32(PCIE_LCTLSTS(pcie_port)) & PCIE_LCTLSTS_RETRAIN_PENDING)) {
            break;
        }
        udelay(10);
    }
    if (i >= IFX_PCIE_LTSSM_ENABLE_TIMEOUT) {
        IFX_PCIE_PRINT(PCIE_MSG_INIT, "%s link timeout!!!!!\n", __func__);
        return -1;
    }
    return 0;
#undef IFX_PCIE_LTSSM_ENABLE_TIMEOUT
}

static INLINE void 
pcie_ltssm_disable(int pcie_port) 
{
    IFX_REG_W32(0, PCIE_RC_CCR(pcie_port)); /* Disable LTSSM */
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_RC_CCR 0x%08x\n", 
        __func__, IFX_REG_R32(PCIE_RC_CCR(pcie_port)));
}

static INLINE void
pcie_ahb_bus_error_suppress(int pcie_port)
{
    IFX_REG_W32(PCIE_AHB_CTRL_BUS_ERROR_SUPPRESS, PCIE_AHB_CTRL(pcie_port));
}

static INLINE void 
pcie_status_register_clear(int pcie_port)
{
   /* Clear the status register, XXX, seperate function */
    IFX_REG_W32(0, PCIE_RC_DR(pcie_port));
    IFX_REG_W32(0, PCIE_PCICMDSTS(pcie_port));
    IFX_REG_W32(0, PCIE_DCTLSTS(pcie_port));
    IFX_REG_W32(0, PCIE_LCTLSTS(pcie_port));
    IFX_REG_W32(0, PCIE_SLCTLSTS(pcie_port));
    IFX_REG_W32(0, PCIE_RSTS(pcie_port));
    IFX_REG_W32(0, PCIE_UES_R(pcie_port));
    IFX_REG_W32(0, PCIE_UEMR(pcie_port));
    IFX_REG_W32(0, PCIE_UESR(pcie_port));
    IFX_REG_W32(0, PCIE_CESR(pcie_port));
    IFX_REG_W32(0, PCIE_CEMR(pcie_port));
    IFX_REG_W32(0, PCIE_RESR(pcie_port));
    IFX_REG_W32(0, PCIE_PVCCRSR(pcie_port));
    IFX_REG_W32(0, PCIE_VC0_RSR0(pcie_port));
    IFX_REG_W32(0, PCIE_TPFCS(pcie_port));
    IFX_REG_W32(0, PCIE_TNPFCS(pcie_port));
    IFX_REG_W32(0, PCIE_TCFCS(pcie_port));
    IFX_REG_W32(0, PCIE_QSR(pcie_port));
    IFX_REG_W32(0, PCIE_IOBLSECS(pcie_port));
}

/* 
 * Must be done after ltssm due to based on negotiated link 
 * width and payload size
 * Update the Replay Time Limit. Empirically, some PCIe 
 * devices take a little longer to respond than expected under 
 * load. As a workaround for this we configure the Replay Time 
 * Limit to the value expected for a 512 byte MPS instead of 
 * our actual 128 byte MPS. The numbers below are directly 
 * from the PCIe spec table 3-4/5.
 * Please note, our PIPE interface is 16b/125MHz, so the table
 * value has to be divided by 2
 */ 
static INLINE void
pcie_replay_time_update(int pcie_port)
{
    u32 reg;
    int nlw;
    int rtl;

    reg = IFX_REG_R32(PCIE_LCTLSTS(pcie_port));
    nlw = MS(reg, PCIE_LCTLSTS_NEGOTIATED_LINK_WIDTH);
    switch (nlw) {
        case PCIE_MAX_LENGTH_WIDTH_X1:
            if (pcie_phy_clock_ppm_enabled()) {
                rtl = 6215; /* 4KB replay timeout */
            }
            else {
                rtl = 839; /* 512B replay timeout */
            }
            break;
        case PCIE_MAX_LENGTH_WIDTH_X2:
            rtl = 818;
            break;
        case PCIE_MAX_LENGTH_WIDTH_X4:
            rtl = 423;
            break;
        case PCIE_MAX_LENGTH_WIDTH_X8:
            rtl = 225;
            break;
        default:
            rtl = 1606;
            break;
    }
    reg = IFX_REG_R32(PCIE_ALTRT(pcie_port));
    reg &= ~PCIE_ALTRT_REPLAY_TIME_LIMIT;
    reg |= SM(rtl, PCIE_ALTRT_REPLAY_TIME_LIMIT);
    IFX_REG_W32(reg, PCIE_ALTRT(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_ALTRT 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_ALTRT(pcie_port)));
}

static inline int 
ifx_pcie_link_up(int pcie_port)
{
    return (IFX_REG_R32(PCIE_PHY_SR(pcie_port)) & PCIE_PHY_SR_PHY_LINK_UP) ? 1 : 0;
}

#ifdef IFX_PCIE_DBG
static void 
pcie_status_registers_dump(int pcie_port)
{
    printk(KERN_INFO "PCIe_PCICMDSTS: 0x%08x\n", IFX_REG_R32(PCIE_PCICMDSTS(pcie_port)));
    printk(KERN_INFO "PCIe_RC_DR:     0x%08x\n", IFX_REG_R32(PCIE_RC_DR(pcie_port)));
    printk(KERN_INFO "PCIe_DCTLSTS:   0x%08x\n", IFX_REG_R32(PCIE_DCTLSTS(pcie_port)));
    printk(KERN_INFO "PCIe_LCTLSTS:   0x%08x\n", IFX_REG_R32(PCIE_LCTLSTS(pcie_port)));
    printk(KERN_INFO "PCIe_SLCTLSTS:  0x%08x\n", IFX_REG_R32(PCIE_SLCTLSTS(pcie_port)));
    printk(KERN_INFO "PCIe_RSTS:      0x%08x\n", IFX_REG_R32(PCIE_RSTS(pcie_port)));
    printk(KERN_INFO "PCIe_UES_R:     0x%08x\n", IFX_REG_R32(PCIE_UES_R(pcie_port)));
    printk(KERN_INFO "PCIe_UEMR:      0x%08x\n", IFX_REG_R32(PCIE_UEMR(pcie_port)));
    printk(KERN_INFO "PCIe_UESR:      0x%08x\n", IFX_REG_R32(PCIE_UESR(pcie_port)));
    printk(KERN_INFO "PCIe_CESR:      0x%08x\n", IFX_REG_R32(PCIE_CESR(pcie_port)));
    printk(KERN_INFO "PCIe_CEMR:      0x%08x\n", IFX_REG_R32(PCIE_CEMR(pcie_port)));
    printk(KERN_INFO "PCIe_RESR:      0x%08x\n", IFX_REG_R32(PCIE_RESR(pcie_port)));
    printk(KERN_INFO "PCIe_ESIR:      0x%08x\n", IFX_REG_R32(PCIE_ESIR(pcie_port)));
    printk(KERN_INFO "PCIe_PVCCRSR:   0x%08x\n", IFX_REG_R32(PCIE_PVCCRSR(pcie_port)));
    printk(KERN_INFO "PCIe_VC0_RSR0:  0x%08x\n", IFX_REG_R32(PCIE_VC0_RSR0(pcie_port)));
    printk(KERN_INFO "PCIe_TPFCS:     0x%08x\n", IFX_REG_R32(PCIE_TPFCS(pcie_port)));
    printk(KERN_INFO "PCIe_TNPFCS:    0x%08x\n", IFX_REG_R32(PCIE_TNPFCS(pcie_port)));
    printk(KERN_INFO "PCIe_TCFCS:     0x%08x\n", IFX_REG_R32(PCIE_TCFCS(pcie_port)));
    printk(KERN_INFO "PCIe_QSR:       0x%08x\n", IFX_REG_R32(PCIE_QSR(pcie_port)));
    printk(KERN_INFO "PCIe_VCTAR1:    0x%08x\n", IFX_REG_R32(PCIE_VCTAR1(pcie_port)));
    printk(KERN_INFO "PCIe_VCTAR2:    0x%08x\n", IFX_REG_R32(PCIE_VCTAR2(pcie_port)));
    printk(KERN_INFO "PCIe_IOBLSECS:  0x%08x\n", IFX_REG_R32(PCIE_IOBLSECS(pcie_port)));
    printk(KERN_INFO "PCIe_ALTRT:     0x%08x\n", IFX_REG_R32(PCIE_ALTRT(pcie_port)));
    printk(KERN_INFO "PCIe_SNR:       0x%08x\n", IFX_REG_R32(PCIE_SNR(pcie_port)));
    printk(KERN_INFO "PCIe_DBR0:      0x%08x\n", IFX_REG_R32(PCIE_DBR0(pcie_port)));
    printk(KERN_INFO "PCIe_DBR1:      0x%08x\n", IFX_REG_R32(PCIE_DBR1(pcie_port)));
}

static void 
pcie_post_dump(int pcie_port)
{
    printk(KERN_INFO "PCIe_PCICMDSTS:  0x%08x\n", IFX_REG_R32(PCIE_PCICMDSTS(pcie_port)));
    printk(KERN_INFO "PCIe_MBML:       0x%08x\n", IFX_REG_R32(PCIE_MBML(pcie_port)));
    printk(KERN_INFO "PCIe_PBML:       0x%08x\n", IFX_REG_R32(PCIE_PMBL(pcie_port)));
    printk(KERN_INFO "PCIe_IOBLSECS:   0x%08x\n", IFX_REG_R32(PCIE_IOBLSECS(pcie_port)));
    printk(KERN_INFO "PCIe_IO_BANDL:   0x%08x\n", IFX_REG_R32(PCIE_IO_BANDL(pcie_port)));
    printk(KERN_INFO "PCIe_INTRBCTRL:  0x%08x\n", IFX_REG_R32(PCIE_INTRBCTRL(pcie_port)));
    printk(KERN_INFO "Power State:     D%1d\n", IFX_REG_R32(PCIE_PM_CSR(pcie_port)) & PCIE_PM_CSR_POWER_STATE);
    printk(KERN_INFO "Negotiated Link Width: %d\n", MS(IFX_REG_R32(PCIE_LCTLSTS(pcie_port)), PCIE_LCTLSTS_NEGOTIATED_LINK_WIDTH));
    printk(KERN_INFO "Number of VCs:    %d\n", IFX_REG_R32(PCIE_PVC1(pcie_port)) & PCIE_PVC1_EXT_VC_CNT);
    printk(KERN_INFO "Low-priority VCs: %d\n", MS(IFX_REG_R32(PCIE_PVC1(pcie_port)), PCIE_PVC1_LOW_PRI_EXT_VC_CNT));
    printk(KERN_INFO "VC Arbitration:   0x%08x\n", IFX_REG_R32(PCIE_PVC2(pcie_port)) & PCIE_PVC2_VC_ARB_WRR);
    printk(KERN_INFO "Port Arbitration: 0x%08x\n", IFX_REG_R32(PCIE_VC0_RC(pcie_port)) & PCIE_VC0_RC_PORT_ARB);

    if (ifx_pcie_link_up(pcie_port)) {
        printk(KERN_INFO "PCIe PHY Link is UP\n");
    }
    else {
        printk(KERN_INFO "PCIe PHY Link is DOWN!\n");
    }
    if ((IFX_REG_R32(PCIE_RC_DR(pcie_port)) & PCIE_RC_DR_DLL_UP)) {
        printk(KERN_INFO "PCIe DLL is UP\n");
    }
    else {
        printk(KERN_INFO "PCIe DLL is DOWN!\n");
    }

    if ((IFX_REG_R32(PCIE_LCTLSTS(pcie_port)) & PCIE_LCTLSTS_DLL_ACTIVE)) {
        printk(KERN_INFO "PCIE_LCTLSTS in DL_Active state!\n");
    }
    else {
        printk(KERN_INFO "PCIE_LCTLSTS NOT in DL_Active state!\n");
    }
 }
#endif /* IFX_PCIE_DBG */

/* XXX, this function is not needed in fact */
static INLINE void
pcie_mem_io_setup(int pcie_port)
{
    u32 reg;
    /*
     * BAR[0:1] readonly register 
     * RC contains only minimal BARs for packets mapped to this device 
     * Mem/IO filters defines a range of memory occupied by memory mapped IO devices that
     * reside on the downstream side fo the bridge.
     */
    reg = SM((PCIE_MEM_PHY_PORT_TO_END(pcie_port) >> 20), PCIE_MBML_MEM_LIMIT_ADDR)
        | SM((PCIE_MEM_PHY_PORT_TO_BASE(pcie_port) >> 20), PCIE_MBML_MEM_BASE_ADDR);

    IFX_REG_W32(reg, PCIE_MBML(pcie_port));

    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_MBML: 0x%08x\n", 
        __func__, IFX_REG_R32(PCIE_MBML(pcie_port)));

#ifdef IFX_PCIE_PREFETCH_MEM_64BIT
    reg = SM((PCIE_MEM_PHY_PORT_TO_END(pcie_port) >> 20), PCIE_PMBL_END_ADDR)
        | SM((PCIE_MEM_PHY_PORT_TO_BASE(pcie_port) >> 20), PCIE_PMBL_UPPER_12BIT)
        | PCIE_PMBL_64BIT_ADDR;
    IFX_REG_W32(reg, PCIE_PMBL(pcie_port));

    /* Must configure upper 32bit */
    IFX_REG_W32(0, PCIE_PMBU32(pcie_port));
    IFX_REG_W32(0, PCIE_PMLU32(pcie_port));
#else
    /* PCIe_PBML, same as MBML */
    IFX_REG_W32(IFX_REG_R32(PCIE_MBML(pcie_port)), PCIE_PMBL(pcie_port));
#endif 
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_PMBL: 0x%08x\n",
    __func__, IFX_REG_R32(PCIE_PMBL(pcie_port)));

    /* IO Address Range */
    reg = SM((PCIE_IO_PHY_PORT_TO_END(pcie_port) >> 12), PCIE_IOBLSECS_IO_LIMIT_ADDR)
        | SM((PCIE_IO_PHY_PORT_TO_BASE(pcie_port) >> 12), PCIE_IOBLSECS_IO_BASE_ADDR);
#ifdef IFX_PCIE_IO_32BIT    
    reg |= PCIE_IOBLSECS_32BIT_IO_ADDR;
#endif /* IFX_PCIE_IO_32BIT */
    IFX_REG_W32(reg, PCIE_IOBLSECS(pcie_port));

    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_IOBLSECS: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_IOBLSECS(pcie_port)));
#ifdef IFX_PCIE_IO_32BIT
    reg = SM((PCIE_IO_PHY_PORT_TO_END(pcie_port) >> 16), PCIE_IO_BANDL_UPPER_16BIT_IO_LIMIT)
        | SM((PCIE_IO_PHY_PORT_TO_BASE(pcie_port) >> 16), PCIE_IO_BANDL_UPPER_16BIT_IO_BASE);
    IFX_REG_W32(reg, PCIE_IO_BANDL(pcie_port));

    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_IO_BANDL: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_IO_BANDL(pcie_port)));
#endif /* IFX_PCIE_IO_32BIT */
}

static INLINE void
pcie_msi_setup(int pcie_port)
{
    u32 reg;

    /* XXX, MSI stuff should only apply to EP */
    /* MSI Capability: Only enable 32-bit addresses */
    reg = IFX_REG_R32(PCIE_MCAPR(pcie_port));
    reg &= ~PCIE_MCAPR_ADDR64_CAP;

    reg |= PCIE_MCAPR_MSI_ENABLE;

    /* Disable multiple message */
    reg &= ~(PCIE_MCAPR_MULTI_MSG_CAP | PCIE_MCAPR_MULTI_MSG_ENABLE);
    IFX_REG_W32(reg, PCIE_MCAPR(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_MCAPR: 0x%08x\n", 
        __func__, IFX_REG_R32(PCIE_MCAPR(pcie_port)));
}

static INLINE void
pcie_pm_setup(int pcie_port)
{
    u32 reg;

    /* Enable PME, Soft reset enabled */
    reg = IFX_REG_R32(PCIE_PM_CSR(pcie_port));
    reg |= PCIE_PM_CSR_PME_ENABLE | PCIE_PM_CSR_SW_RST;
    IFX_REG_W32(reg, PCIE_PM_CSR(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_PM_CSR: 0x%08x\n", 
        __func__, IFX_REG_R32(PCIE_PM_CSR(pcie_port)));
}

static INLINE void
pcie_bus_setup(int pcie_port) 
{
    u32 reg;

    reg = SM(0, PCIE_BNR_PRIMARY_BUS_NUM) | SM(1, PCIE_PNR_SECONDARY_BUS_NUM) | SM(0xFF, PCIE_PNR_SUB_BUS_NUM);
    IFX_REG_W32(reg, PCIE_BNR(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_BNR: 0x%08x\n", 
        __func__, IFX_REG_R32(PCIE_BNR(pcie_port)));
}

static INLINE void
pcie_device_setup(int pcie_port)
{
    u32 reg;

    /* Device capability register, set up Maximum payload size */
    reg = IFX_REG_R32(PCIE_DCAP(pcie_port));
    reg |= PCIE_DCAP_ROLE_BASE_ERR_REPORT;
    reg |= SM(PCIE_MAX_PAYLOAD_128, PCIE_DCAP_MAX_PAYLOAD_SIZE);

    /* Only available for EP */
    reg &= ~(PCIE_DCAP_EP_L0S_LATENCY | PCIE_DCAP_EP_L1_LATENCY);
    IFX_REG_W32(reg, PCIE_DCAP(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_DCAP: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_DCAP(pcie_port)));

    /* Device control and status register */
    /* Set Maximum Read Request size for the device as a Requestor */
    reg = IFX_REG_R32(PCIE_DCTLSTS(pcie_port));

    /* 
     * Request size can be larger than the MPS used, but the completions returned 
     * for the read will be bounded by the MPS size.
     * In our system, Max request size depends on AHB/DMA burst size. It is 32 bytes.
     * but we set it as 128 as minimum one.
     */
    reg |= SM(PCIE_MAX_PAYLOAD_128, PCIE_DCTLSTS_MAX_READ_SIZE)
            | SM(PCIE_MAX_PAYLOAD_128, PCIE_DCTLSTS_MAX_PAYLOAD_SIZE);

    /* Enable relaxed ordering, no snoop, and all kinds of errors */
    reg |= PCIE_DCTLSTS_RELAXED_ORDERING_EN | PCIE_DCTLSTS_ERR_EN | PCIE_DCTLSTS_NO_SNOOP_EN;

    IFX_REG_W32(reg, PCIE_DCTLSTS(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_DCTLSTS: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_DCTLSTS(pcie_port)));
}

static INLINE void
pcie_link_setup(int pcie_port)
{
    u32 reg;

    /*
     * XXX, Link capability register, bit 18 for EP CLKREQ# dynamic clock management for L1, L2/3 CPM 
     * L0s is reported during link training via TS1 order set by N_FTS
     */
    reg = IFX_REG_R32(PCIE_LCAP(pcie_port));
    reg &= ~PCIE_LCAP_L0S_EIXT_LATENCY;
    reg |= SM(3, PCIE_LCAP_L0S_EIXT_LATENCY);
    IFX_REG_W32(reg, PCIE_LCAP(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_LCAP: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_LCAP(pcie_port)));

    /* Link control and status register */
    reg = IFX_REG_R32(PCIE_LCTLSTS(pcie_port));

    /* Link Enable, ASPM enabled  */
    reg &= ~PCIE_LCTLSTS_LINK_DISABLE;

    /*  
     * We use the same physical reference clock that the platform provides on the connector 
     * It paved the way for ASPM to calculate the new exit Latency
     */
    reg |= PCIE_LCTLSTS_SLOT_CLK_CFG;
    reg |= PCIE_LCTLSTS_COM_CLK_CFG;
#ifdef CONFIG_PCIEASPM
    /*
     * We should disable ASPM by default except that we have dedicated power management support
     * Enable ASPM will cause the system hangup/instability, performance degration
     */
    reg |= PCIE_LCTLSTS_ASPM_ENABLE;
#else
    reg &= ~PCIE_LCTLSTS_ASPM_ENABLE;
#endif /* CONFIG_PCIEASPM */

    /* 
     * The maximum size of any completion with data packet is bounded by the MPS setting 
     * in  device control register 
     */

    /* RCB may cause multiple split transactions, two options available, we use 64 byte RCB */
    reg &= ~ PCIE_LCTLSTS_RCB128;

    IFX_REG_W32(reg, PCIE_LCTLSTS(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_LCTLSTS: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_LCTLSTS(pcie_port)));
}

static INLINE void
pcie_error_setup(int pcie_port)
{
    u32 reg;

    /* 
     * Forward ERR_COR, ERR_NONFATAL, ERR_FATAL to the backbone 
     * Poisoned write TLPs and completions indicating poisoned TLPs will set the PCIe_PCICMDSTS.MDPE 
     */
    reg = IFX_REG_R32(PCIE_INTRBCTRL(pcie_port));
    reg |= PCIE_INTRBCTRL_SERR_ENABLE | PCIE_INTRBCTRL_PARITY_ERR_RESP_ENABLE;

    IFX_REG_W32(reg, PCIE_INTRBCTRL(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_INTRBCTRL: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_INTRBCTRL(pcie_port)));

    /* Uncorrectable Error Mask Register, Unmask <enable> all bits in PCIE_UESR */
    reg = IFX_REG_R32(PCIE_UEMR(pcie_port));
    reg &= ~PCIE_ALL_UNCORRECTABLE_ERR;
    IFX_REG_W32(reg, PCIE_UEMR(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_UEMR: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_UEMR(pcie_port)));

    /* Uncorrectable Error Severity Register, ALL errors are FATAL */
    IFX_REG_W32(PCIE_ALL_UNCORRECTABLE_ERR, PCIE_UESR(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_UESR: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_UESR(pcie_port)));

    /* Correctable Error Mask Register, unmask <enable> all bits */
    reg = IFX_REG_R32(PCIE_CEMR(pcie_port));
    reg &= ~PCIE_CORRECTABLE_ERR;
    IFX_REG_W32(reg, PCIE_CEMR(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_CEMR: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_CEMR(pcie_port)));

    /* Advanced Error Capabilities and Control Register */
    /* plat_dev_init will do the related stuff */
}

static INLINE void
pcie_root_setup(int pcie_port)
{
    /* plat_dev_init will do the related stuff */
}

static INLINE void
pcie_vc_setup(int pcie_port)
{
    u32 reg;

    /* Port VC Capability Register 2 */
    reg = IFX_REG_R32(PCIE_PVC2(pcie_port));
    reg &= ~PCIE_PVC2_VC_ARB_WRR;
    reg |= PCIE_PVC2_VC_ARB_16P_FIXED_WRR;
    IFX_REG_W32(reg, PCIE_PVC2(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_PVC2: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_PVC2(pcie_port)));

    /* VC0 Resource Capability Register */
    reg = IFX_REG_R32(PCIE_VC0_RC(pcie_port));
    reg &= ~PCIE_VC0_RC_REJECT_SNOOP;
    IFX_REG_W32(reg, PCIE_VC0_RC(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_VC0_RC: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_VC0_RC(pcie_port)));
}

static INLINE void
pcie_port_logic_setup(int pcie_port)
{
    u32 reg;

    /* FTS number, default 12, increase to 63, may increase time from/to L0s to L0  */
    reg = IFX_REG_R32(PCIE_AFR(pcie_port));
    reg &= ~(PCIE_AFR_FTS_NUM | PCIE_AFR_COM_FTS_NUM);
    reg |= SM(PCIE_AFR_FTS_NUM_DEFAULT, PCIE_AFR_FTS_NUM)
        | SM(PCIE_AFR_FTS_NUM_DEFAULT, PCIE_AFR_COM_FTS_NUM);
    /* L0s and L1 entry latency */
    reg &= ~(PCIE_AFR_L0S_ENTRY_LATENCY | PCIE_AFR_L1_ENTRY_LATENCY);
    reg |= SM(PCIE_AFR_L0S_ENTRY_LATENCY_DEFAULT, PCIE_AFR_L0S_ENTRY_LATENCY)
        | SM(PCIE_AFR_L1_ENTRY_LATENCY_DEFAULT, PCIE_AFR_L1_ENTRY_LATENCY);
    IFX_REG_W32(reg, PCIE_AFR(pcie_port));

    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_AFR: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_AFR(pcie_port)));

    /* Port Link Control Register */
    reg = IFX_REG_R32(PCIE_PLCR(pcie_port));
    reg |= PCIE_PLCR_DLL_LINK_EN;  /* Enable the DLL link */
    IFX_REG_W32(reg, PCIE_PLCR(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_PLCR: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_PLCR(pcie_port)));

    /* Lane Skew Register */
    reg = IFX_REG_R32(PCIE_LSR(pcie_port));
    /* Enable ACK/NACK and FC */
    reg &= ~(PCIE_LSR_ACKNAK_DISABLE | PCIE_LSR_FC_DISABLE); 
    IFX_REG_W32(reg, PCIE_LSR(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_LSR: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_LSR(pcie_port)));

    /* Symbol Timer Register and Filter Mask Register 1 */
    reg = IFX_REG_R32(PCIE_STRFMR(pcie_port));

    /* Default SKP interval is very accurate already, 5us */
    /* Enable IO/CFG transaction */
    reg |= PCIE_STRFMR_RX_CFG_TRANS_ENABLE | PCIE_STRFMR_RX_IO_TRANS_ENABLE;
    /* Disable FC WDT */
    reg &= ~PCIE_STRFMR_FC_WDT_DISABLE;
    IFX_REG_W32(reg, PCIE_STRFMR(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_STRFMR: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_STRFMR(pcie_port)));

    /* Filter Masker Register 2 */
    reg = IFX_REG_R32(PCIE_FMR2(pcie_port));
    reg |= PCIE_FMR2_VENDOR_MSG1_PASSED_TO_TRGT1 | PCIE_FMR2_VENDOR_MSG0_PASSED_TO_TRGT1;
    IFX_REG_W32(reg, PCIE_FMR2(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_FMR2: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_FMR2(pcie_port)));

    /* VC0 Completion Receive Queue Control Register */
    reg = IFX_REG_R32(PCIE_VC0_CRQCR(pcie_port));
    reg &= ~PCIE_VC0_CRQCR_CPL_TLP_QUEUE_MODE;
    reg |= SM(PCIE_VC0_TLP_QUEUE_MODE_BYPASS, PCIE_VC0_CRQCR_CPL_TLP_QUEUE_MODE);
    IFX_REG_W32(reg, PCIE_VC0_CRQCR(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_VC0_CRQCR: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_VC0_CRQCR(pcie_port)));
}

static INLINE void 
pcie_rc_cfg_reg_setup(int pcie_port)
{
    pcie_ltssm_disable(pcie_port);
    pcie_mem_io_setup(pcie_port);
    pcie_msi_setup(pcie_port);
    pcie_pm_setup(pcie_port);
    pcie_bus_setup(pcie_port);
    pcie_device_setup(pcie_port);
    pcie_link_setup(pcie_port);
    pcie_error_setup(pcie_port);
    pcie_root_setup(pcie_port);
    pcie_vc_setup(pcie_port);
    pcie_port_logic_setup(pcie_port);
}

static int 
ifx_pcie_wait_phy_link_up(int pcie_port)
{
#define IFX_PCIE_PHY_LINK_UP_TIMEOUT  6000 /* XXX, tunable */
    int i;
    int rc_port = pcie_port_idx_to_rc_port(pcie_port);

    /* Wait for PHY link is up */
    for (i = 0; i < IFX_PCIE_PHY_LINK_UP_TIMEOUT; i++) {
        if (ifx_pcie_link_up(pcie_port)) {
            break;
        }
        udelay(100);
    }
    if (i >= IFX_PCIE_PHY_LINK_UP_TIMEOUT) {
        printk(KERN_ERR "%s port %d timeout\n", __func__, rc_port);
        return -1;
    }
    /* Check data link up or not */
    if (!(IFX_REG_R32(PCIE_RC_DR(pcie_port)) & PCIE_RC_DR_DLL_UP)) {
        printk(KERN_ERR "%s port %d DLL link is still down\n", __func__, rc_port);
        return -1;
    }
    /* Check Data link active or not */
    if (!(IFX_REG_R32(PCIE_LCTLSTS(pcie_port)) & PCIE_LCTLSTS_DLL_ACTIVE)) {
        printk(KERN_ERR "%s port %d DLL is not active\n", __func__, rc_port);
        return -1;
    }
    return 0;
#undef IFX_PCIE_PHY_LINK_UP_TIMEOUT
}

static INLINE int
pcie_app_loigc_setup(int pcie_port)
{
    int rc_port = pcie_port_idx_to_rc_port(pcie_port);
#ifdef IFX_PCIE_PHY_DBG
    pcie_disable_scrambling(pcie_port);
#endif /* IFX_PCIE_PHY_DBG */
    pcie_ahb_bus_error_suppress(pcie_port);
    /* Pull PCIe EP out of reset */
    pcie_device_rst_deassert(rc_port);
    /* Start LTSSM training between RC and EP */
    pcie_ltssm_enable(pcie_port);
    /* Check PHY status after enabling LTSSM */
    if (ifx_pcie_wait_phy_link_up(pcie_port) != 0) {
        return -1;
    }
    return 0;
}

/*
 * Table 359 Enhanced Configuration Address Mapping1)
 * 1) This table is defined in Table 7-1, page 341, PCI Express Base Specification v1.1
 * Memory Address PCI Express Configuration Space
 * A[(20+n-1):20] Bus Number 1 < n < 8
 * A[19:15] Device Number
 * A[14:12] Function Number
 * A[11:8] Extended Register Number
 * A[7:2] Register Number
 * A[1:0] Along with size of the access, used to generate Byte Enables
 * For VR9, only the address bits [22:0] are mapped to the configuration space:
 * . Address bits [22:20] select the target bus (1-of-8)1)
 * . Address bits [19:15] select the target device (1-of-32) on the bus
 * . Address bits [14:12] select the target function (1-of-8) within the device.
 * . Address bits [11:2] selects the target dword (1-of-1024) within the selected function.s configuration space
 * . Address bits [1:0] define the start byte location within the selected dword.
 */
static inline u32
pcie_bus_addr(u8 bus_num, u16 devfn, int where)
{
    u32 addr;
    u8  bus;

    if (!bus_num) {
        /* type 0 */
        addr = ((PCI_SLOT(devfn) & 0x1F) << 15) | ((PCI_FUNC(devfn) & 0x7) << 12) | ((where & 0xFFF)& ~3);
    } 
    else {
        bus = bus_num;
        /* type 1, only support 8 buses  */
        addr = ((bus & 0x7) << 20) | ((PCI_SLOT(devfn) & 0x1F) << 15) |
                ((PCI_FUNC(devfn) & 0x7) << 12) | ((where & 0xFFF) & ~3);
    }
    IFX_PCIE_PRINT(PCIE_MSG_CFG, "%s: bus addr : %02x:%02x.%01x/%02x, addr=%08x\n",
        __func__, bus_num, PCI_SLOT(devfn), PCI_FUNC(devfn), where, addr);
    return addr;
}

static int 
pcie_valid_config(int pcie_port, int bus, int dev)
{
    /* RC itself */
    if ((bus == 0) && (dev == 0)) {
         return 1;
    }

    /* No physical link */
    if (!ifx_pcie_link_up(pcie_port)) {
        return 0;
    }

    /* Bus zero only has RC itself
     * XXX, check if EP will be integrated 
     */
    if ((bus == 0) && (dev != 0)) {
        return 0;
    }

    /* Maximum 8 buses supported */
    if (bus > 8) {
        return 0;
    }

    /* 
     * PCIe is PtP link, one bus only supports only one device 
     * except bus zero and PCIe switch which is virtual bus device
     * The following two conditions really depends on the system design
     * and attached the device.
     * XXX, how about more new switch
     */
    if ((bus == 1) && (dev != 0)) {
        return 0;
    }

    if ((bus >= 3) && (dev != 0)) {
        return 0;
    }
    return 1;
}

static INLINE u32 
ifx_pcie_cfg_rd(int pcie_port, u32 reg)
{
    return IFX_REG_R32((volatile u32 *)(PCIE_CFG_PORT_TO_BASE(pcie_port) + reg));
}

static INLINE void 
ifx_pcie_cfg_wr(int pcie_port, unsigned int reg, u32 val)
{
    IFX_REG_W32( val, (volatile u32 *)(PCIE_CFG_PORT_TO_BASE(pcie_port) + reg));
}

static INLINE u32 
ifx_pcie_rc_cfg_rd(int pcie_port, u32 reg)
{
    /* 
     * WAR for HN device:
     * The BAR0 & BAR1 register have to return 0x00 in case of HN as RC
     * for any reason the HN device reports other values than  0x00.
     */
#ifdef CONFIG_HN1
    if((reg == PCI_BASE_ADDRESS_0) || (reg == PCI_BASE_ADDRESS_1)) {
        return 0x00000000;
    }
#endif /* CONFIG_HN1*/
    return IFX_REG_R32((volatile u32 *)(PCIE_RC_PORT_TO_BASE(pcie_port) + reg));
}

static INLINE void 
ifx_pcie_rc_cfg_wr(int pcie_port, unsigned int reg, u32 val)
{
    IFX_REG_W32(val, (volatile u32 *)(PCIE_RC_PORT_TO_BASE(pcie_port) + reg));
}

static u32 
ifx_pcie_bus_enum_read_hack(int bus_nr_offset, int where, u32 value)
{
    u32 tvalue = value;

    if (where == PCI_PRIMARY_BUS) {
        u8 primary, secondary, subordinate;

        primary = tvalue & 0xFF;
        secondary = (tvalue >> 8) & 0xFF;
        subordinate = (tvalue >> 16) & 0xFF;
        primary += bus_nr_offset;
        secondary += bus_nr_offset;
        subordinate += bus_nr_offset;
        tvalue = (tvalue & 0xFF000000) | (u32)primary | (u32)(secondary << 8) | (u32)(subordinate << 16);
    }
    return tvalue;
}

static u32 
ifx_pcie_bus_enum_write_hack(int bus_nr_offset, int where, u32 value)
{
    u32 tvalue = value;

    if (where == PCI_PRIMARY_BUS) {
        u8 primary, secondary, subordinate;

        primary = tvalue & 0xFF;
        secondary = (tvalue >> 8) & 0xFF;
        subordinate = (tvalue >> 16) & 0xFF;
        if (primary > 0 && primary != 0xFF) {
            primary -= bus_nr_offset;
        }

        if (secondary > 0 && secondary != 0xFF) {
            secondary -= bus_nr_offset;
        }
        if (subordinate > 0 && subordinate != 0xFF) {
            subordinate -= bus_nr_offset;
        }
        tvalue = (tvalue & 0xFF000000) | (u32)primary | (u32)(secondary << 8) | (u32)(subordinate << 16);
    }
    else if (where == PCI_SUBORDINATE_BUS) {
        u8 subordinate = tvalue & 0xFF;

        subordinate = subordinate > 0 ? subordinate - bus_nr_offset : 0;
        tvalue = subordinate;
    }
    return tvalue;
}

static int
pcie_dev_header_type_bridge(struct pci_bus *bus, u32 devfn)
{
    int ret = 1;
    struct pci_dev *pdev;

    /* Sanity check */
    pdev = pci_get_slot(bus, devfn);
    if (pdev == NULL) {
        ret = 0;
        goto out;
    }

    /* Only care about PCI bridge */
    if (pdev->hdr_type != PCI_HEADER_TYPE_BRIDGE) {
        ret = 0;
    }
out:
    return ret;
}

/** 
 * \fn static int ifx_pcie_read_config(struct pci_bus *bus, u32 devfn, 
 *                   int where, int size, u32 *value)
 * \brief Read a value from configuration space 
 * 
 * \param[in] bus    Pointer to pci bus
 * \param[in] devfn  PCI device function number
 * \param[in] where  PCI register number 
 * \param[in] size   Register read size
 * \param[out] value    Pointer to return value
 * \return  PCIBIOS_BAD_REGISTER_NUMBER Invalid register number
 * \return  PCIBIOS_FUNC_NOT_SUPPORTED  PCI function not supported
 * \return  PCIBIOS_DEVICE_NOT_FOUND    PCI device not found
 * \return  PCIBIOS_SUCCESSFUL          OK
 * \ingroup IFX_PCIE_OS
 */ 
static int 
ifx_pcie_read_config(struct pci_bus *bus, u32 devfn, 
                    int where, int size, u32 *value)
{
    u32 data = 0;
    int bus_number = bus->number;
    int bus_nr_offset;
    static const u32 mask[8] = {0, 0xff, 0xffff, 0, 0xffffffff, 0, 0, 0};    
    int ret = PCIBIOS_SUCCESSFUL;
    struct ifx_pcie_controller *ctrl = bus->sysdata;
    int rc_port = ctrl->port;
    int pcie_port;

    pcie_rc_port_to_pcie_port(rc_port, &pcie_port);

    if (unlikely(size != 1 && size != 2 && size != 4)){
        ret = PCIBIOS_BAD_REGISTER_NUMBER;
        goto out;
    }

    /* Make sure the address is aligned to natural boundary */
    if (unlikely(((size - 1) & where))) {
        ret = PCIBIOS_BAD_REGISTER_NUMBER;
        goto out;
    }

    /* 
     * If we are not the first controller, we have to deduce the bus number before the current bus 
     * so that it assume its bus number starts from 0 in host controller
     */
    bus_nr_offset = pci_find_preexist_bus_nr(bus);
    bus_number -= bus_nr_offset;

    /* 
     * We need to force the bus number to be zero on the root 
     * bus. Linux numbers the 2nd root bus to start after all 
     * busses on root 0. 
     */ 
    if (bus->parent == NULL) {
        bus_number = 0; 
    }

    /* 
     * PCIe only has a single device connected to it. It is 
     * always device ID 0. Don't bother doing reads for other 
     * device IDs on the first segment. 
     */ 
    if ((bus_number == 0) && (PCI_SLOT(devfn) != 0)) {
        ret = PCIBIOS_FUNC_NOT_SUPPORTED;
        goto out; 
    }

    if (pcie_valid_config(pcie_port, bus_number, PCI_SLOT(devfn)) == 0) {
        *value = 0xffffffff;
        ret = PCIBIOS_DEVICE_NOT_FOUND;
        goto out;
    }

    IFX_PCIE_PRINT(PCIE_MSG_READ_CFG, "%s: %02x:%02x.%01x/%02x:%01d\n", __func__, bus_number,
        PCI_SLOT(devfn), PCI_FUNC(devfn), where, size);

    PCIE_IRQ_LOCK(ctrl->lock);
    if (bus_number == 0) { /* RC itself */
        u32 t;

        t = (where & ~3);
        data = ifx_pcie_rc_cfg_rd(pcie_port, t);
        IFX_PCIE_PRINT(PCIE_MSG_READ_CFG, "%s: rd local cfg, offset:%08x, data:%08x\n",
            __func__, t, data);
    }
    else {
        u32 addr = pcie_bus_addr(bus_number, devfn, where);

        data = ifx_pcie_cfg_rd(pcie_port, addr);
        if (rc_port == IFX_PCIE_PORT0) {
    #ifdef CONFIG_IFX_PCIE_HW_SWAP
            data = le32_to_cpu(data);
    #endif /* CONFIG_IFX_PCIE_HW_SWAP */
        }
        else if (rc_port == IFX_PCIE_PORT1) {
    #ifdef CONFIG_IFX_PCIE1_HW_SWAP
            data = le32_to_cpu(data);
    #endif /* CONFIG_IFX_PCIE1_HW_SWAP */
        }
        else if (rc_port == IFX_PCIE_PORT2) {
    #ifdef CONFIG_IFX_PCIE2_HW_SWAP
            data = le32_to_cpu(data);
    #endif /* CONFIG_IFX_PCIE2_HW_SWAP */
        }
        else {
            IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
        }
    }
    
    /* To get a correct PCI topology, we have to restore the bus number to OS */
    if (pcie_dev_header_type_bridge(bus, devfn)) {
        data = ifx_pcie_bus_enum_read_hack(bus_nr_offset, where, data); 
    }
    PCIE_IRQ_UNLOCK(ctrl->lock);
    IFX_PCIE_PRINT(PCIE_MSG_READ_CFG, "%s: read config: data=%08x raw=%08x\n",
        __func__, (data >> (8 * (where & 3))) & mask[size & 7], data); 

    *value = (data >> (8 * (where & 3))) & mask[size & 7];
out:
    return ret;
}

static u32 
ifx_pcie_size_to_value(int where, int size, u32 data, u32 value)
{
    u32 shift;
    u32 tdata = data;

    switch (size) {
        case 1:
            shift = (where & 0x3) << 3;
            tdata &= ~(0xffU << shift);
            tdata |= ((value & 0xffU) << shift);
            break;
        case 2:
            shift = (where & 3) << 3;
            tdata &= ~(0xffffU << shift);
            tdata |= ((value & 0xffffU) << shift);
            break;
        case 4:
            tdata = value;
            break;
    }
    return tdata;
}

/** 
 * \fn static static int ifx_pcie_write_config(struct pci_bus *bus, u32 devfn,
 *                 int where, int size, u32 value)
 * \brief Write a value to PCI configuration space 
 * 
 * \param[in] bus    Pointer to pci bus
 * \param[in] devfn  PCI device function number
 * \param[in] where  PCI register number 
 * \param[in] size   The register size to be written
 * \param[in] value  The valule to be written
 * \return PCIBIOS_BAD_REGISTER_NUMBER Invalid register number
 * \return PCIBIOS_DEVICE_NOT_FOUND    PCI device not found
 * \return PCIBIOS_SUCCESSFUL          OK
 * \ingroup IFX_PCIE_OS
 */ 
static int 
ifx_pcie_write_config(struct pci_bus *bus, u32 devfn,
                  int where, int size, u32 value)
{
    int bus_number = bus->number;
    int bus_nr_offset;
    int ret = PCIBIOS_SUCCESSFUL;
    struct ifx_pcie_controller *ctrl = bus->sysdata;
    u32 tvalue = value;
    u32 data;
    int rc_port = ctrl->port;
    int pcie_port;

    pcie_rc_port_to_pcie_port(rc_port, &pcie_port);

    /* Make sure the address is aligned to natural boundary */
    if (unlikely(((size - 1) & where))) {
        ret = PCIBIOS_BAD_REGISTER_NUMBER;
        goto out;
    }
    
    /* 
     * If we are not the first controller, we have to deduce the bus number before the current bus 
     * so that it assume its bus number starts from 0 in host controller
     */
    bus_nr_offset = pci_find_preexist_bus_nr(bus);
    bus_number -= bus_nr_offset;

    /* 
     * We need to force the bus number to be zero on the root 
     * bus. Linux numbers the 2nd root bus to start after all 
     * busses on root 0. 
     */ 
    if (bus->parent == NULL) {
        bus_number = 0; 
    }

    if (pcie_valid_config(pcie_port, bus_number, PCI_SLOT(devfn)) == 0) {
        ret = PCIBIOS_DEVICE_NOT_FOUND;
        goto out;
    }

    IFX_PCIE_PRINT(PCIE_MSG_WRITE_CFG, "%s: %02x:%02x.%01x/%02x:%01d value=%08x\n", __func__, 
        bus_number, PCI_SLOT(devfn), PCI_FUNC(devfn), where, size, value);

    /* XXX, some PCIe device may need some delay */
    PCIE_IRQ_LOCK(ctrl->lock);

    /* 
     * To configure the correct bus topology using native way, we have to cheat Os so that
     * it can configure the PCIe hardware correctly.
     */
    if (pcie_dev_header_type_bridge(bus, devfn)) {
        tvalue = ifx_pcie_bus_enum_write_hack(bus_nr_offset, where, value); 
    }
    
    if (bus_number == 0) { /* RC itself */
        u32 t;

        t = (where & ~3);
        IFX_PCIE_PRINT(PCIE_MSG_WRITE_CFG,"%s: wr local cfg, offset:%08x, fill:%08x\n", __func__, t, value);
        data = ifx_pcie_rc_cfg_rd(pcie_port, t);
        IFX_PCIE_PRINT(PCIE_MSG_WRITE_CFG,"%s: rd local cfg, offset:%08x, data:%08x\n", __func__, t, data);

        data = ifx_pcie_size_to_value(where, size, data, tvalue);

        IFX_PCIE_PRINT(PCIE_MSG_WRITE_CFG,"%s: wr local cfg, offset:%08x, value:%08x\n", __func__, t, data);
        ifx_pcie_rc_cfg_wr(pcie_port, t, data);     
        IFX_PCIE_PRINT(PCIE_MSG_WRITE_CFG,"%s: rd local cfg, offset:%08x, value:%08x\n", 
            __func__, t, ifx_pcie_rc_cfg_rd(pcie_port, t));
    }
    else {
        u32 addr = pcie_bus_addr(bus_number, devfn, where);

        IFX_PCIE_PRINT(PCIE_MSG_WRITE_CFG,"%s: wr cfg, offset:%08x, fill:%08x\n", __func__, addr, value);
        data = ifx_pcie_cfg_rd(pcie_port, addr);
        if (rc_port == IFX_PCIE_PORT0) {
    #ifdef CONFIG_IFX_PCIE_HW_SWAP
            data = le32_to_cpu(data);
    #endif /* CONFIG_IFX_PCIE_HW_SWAP */
        }
        else if (rc_port == IFX_PCIE_PORT1) {
    #ifdef CONFIG_IFX_PCIE1_HW_SWAP
            data = le32_to_cpu(data);
    #endif /* CONFIG_IFX_PCIE1_HW_SWAP */
        }
        else if (rc_port == IFX_PCIE_PORT2) {
    #ifdef CONFIG_IFX_PCIE2_HW_SWAP
            data = le32_to_cpu(data);
    #endif /* CONFIG_IFX_PCIE2_HW_SWAP */
        }
        else {
            IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
        }
        IFX_PCIE_PRINT(PCIE_MSG_WRITE_CFG,"%s: rd cfg, offset:%08x, data:%08x\n", __func__, addr, data);

        data = ifx_pcie_size_to_value(where, size, data, tvalue);
        if (rc_port == IFX_PCIE_PORT0) {
    #ifdef CONFIG_IFX_PCIE_HW_SWAP
            data = cpu_to_le32(data);
    #endif /* CONFIG_IFX_PCIE_HW_SWAP */
        }
        else if (rc_port == IFX_PCIE_PORT1){
    #ifdef CONFIG_IFX_PCIE1_HW_SWAP
            data = cpu_to_le32(data);
    #endif /* CONFIG_IFX_PCIE1_HW_SWAP */
        }
        else if (rc_port == IFX_PCIE_PORT2){
    #ifdef CONFIG_IFX_PCIE2_HW_SWAP
            data = cpu_to_le32(data);
    #endif /* CONFIG_IFX_PCIE2_HW_SWAP */
        }
        else {
            IFX_KASSERT(0, ("%s invalid port %d\n", __func__, rc_port));
        }
        IFX_PCIE_PRINT(PCIE_MSG_WRITE_CFG, "%s: wr cfg, offset:%08x, value:%08x\n", __func__, addr, data);
        ifx_pcie_cfg_wr(pcie_port, addr, data);
        IFX_PCIE_PRINT(PCIE_MSG_WRITE_CFG, "%s: rd cfg, offset:%08x, value:%08x\n", 
            __func__, addr, ifx_pcie_cfg_rd(pcie_port, addr));
    }
    PCIE_IRQ_UNLOCK(ctrl->lock);
out:
    return ret;
}

#ifdef IFX_PCIE_BASIC_ERROR_INT

extern struct pci_dev *aer_dev_to_pci_dev(void *context);
extern irqreturn_t aer_irq(int irq, void *context);

#ifdef IFX_PCIE_DBG
static const char *pcie_err_str[] = {
    "AER",
    "AER-MSI(X)",
    "PME interrupt",
    "Hotplug",
    "Vendor Msg",
    "Correctable Error",
    "Non-Fatal Error",
    "Fatal Error",
    "PME Msg",
    "PME Turn Off Ack",
    "AHB Fatal Error",
    "Link Auto BW",
    "BW Mgmt",
    "INTA",
    "INTB",
    "INTC",
    "INTD",
    "Wakeup"
};

static const char *
pcie_errno_to_str(u32 value) 
{
    /* If multiple interrupt bits set, only the first low bit reported */
    if (ffs(value) > ARRAY_SIZE(pcie_err_str)) {
        return "Spurious Interrupt";
    }
    else {
        return pcie_err_str[ffs(value) - 1];
    }
}
#endif /* IFX_PCIE_DBG */

static INLINE void 
pcie_core_int_clear_all(int irq, struct pci_dev *dev, void *context)
{
    u32 reg;
    struct ifx_pcie_controller *ctrl = dev->bus->sysdata;
    int pcie_port;
    int rc_port = ctrl->port;
    
    pcie_rc_port_to_pcie_port(rc_port, &pcie_port);
    reg = IFX_REG_R32(PCIE_IRNCR(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_ISR, "%s port %d PCIE_IRNCR: %s\n", 
        __func__, rc_port, pcie_errno_to_str(IFX_REG_R32(PCIE_IRNCR(pcie_port))));
#ifdef IFX_PCIE_AER_REPORT
    if ((reg & PCIE_IRN_AER_REPORT) != 0) {
        aer_irq(irq, context);
    }
#endif /* IFX_PCIE_AER_REPORT */
    reg &= PCIE_RC_CORE_COMBINED_INT;
    IFX_REG_W32(reg, PCIE_IRNCR(pcie_port));
}

static irqreturn_t
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
pcie_rc_core_isr(int irq, void *context)
#else
pcie_rc_core_isr(int irq, void *context, struct pt_regs *regs)
#endif
{
    struct pci_dev *dev = aer_dev_to_pci_dev(context);
    pcie_core_int_clear_all(irq, dev, context);
    return IRQ_HANDLED;
}

static void
pcie_rc_core_int_init(int pcie_port)
{
    IFX_PCIE_PRINT(PCIE_MSG_INIT, "%s enter \n", __func__);

    /* Disable core interrupt */
    IFX_REG_CLR_BIT(PCIE_RC_CORE_COMBINED_INT, PCIE_IRNEN(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_IRNEN: 0x%08x\n", 
        __func__, IFX_REG_R32(PCIE_IRNEN(pcie_port)));

    /* Clear it first */
    IFX_REG_SET_BIT(PCIE_RC_CORE_COMBINED_INT, PCIE_IRNCR(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_IRNCR: 0x%08x\n",
        __func__, IFX_REG_R32(PCIE_IRNCR(pcie_port)));

    IFX_PCIE_PRINT(PCIE_MSG_INIT, "%s exit \n", __func__);
}

int ifx_pcie_rc_aer_irq_register(struct pci_dev *dev, void *context)
{
    int ret =  0;
    struct ifx_pcie_controller *ctrl = dev->bus->sysdata;
    int rc_port = ctrl->port;
    int pcie_port;

    pcie_rc_port_to_pcie_port(rc_port, &pcie_port);

    ret = request_irq(g_pcie_port_defs[pcie_port].legacy_irqs.ir_irq.irq, pcie_rc_core_isr, IRQF_DISABLED,
                g_pcie_port_defs[pcie_port].legacy_irqs.ir_irq.name, context);
    if (ret) {
        printk(KERN_ERR "%s request irq %d failed\n", __func__, IFX_PCIE_IR);
    }
    /* Enable error interrupts */
    IFX_REG_SET_BIT(PCIE_RC_CORE_COMBINED_INT, PCIE_IRNEN(pcie_port));    
    return ret;
}
EXPORT_SYMBOL_GPL(ifx_pcie_rc_aer_irq_register);

#endif /* IFX_PCIE_BASIC_ERROR_INT */

/** 
 * \fn int ifx_pcie_bios_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
 * \brief Map a PCI device to the appropriate interrupt line 
 * 
 * \param[in] dev    The Linux PCI device structure for the device to map 
 * \param[in] slot   The slot number for this device on __BUS 0__. Linux 
 *                   enumerates through all the bridges and figures out the 
 *                   slot on Bus 0 where this device eventually hooks to. 
 * \param[in] pin    The PCI interrupt pin read from the device, then swizzled 
 *                   as it goes through each bridge. 
 * \return Interrupt number for the device
 * \ingroup IFX_PCIE_OS
 */ 
int 
ifx_pcie_bios_map_irq(IFX_PCI_CONST struct pci_dev *dev, u8 slot, u8 pin)
{
    u32 irq_bit = 0;
    int irq = 0;
    struct ifx_pcie_controller *ctrl = dev->bus->sysdata;
    int pcie_port;
    int rc_port = ctrl->port;

    pcie_rc_port_to_pcie_port(rc_port, &pcie_port);
    IFX_PCIE_PRINT(PCIE_MSG_FIXUP, "%s port %d dev %s slot %d pin %d \n", __func__, rc_port, pci_name(dev), slot, pin);

    if ((pin == PCIE_LEGACY_DISABLE) || (pin > PCIE_LEGACY_INT_MAX)) {
        printk(KERN_WARNING "WARNING: dev %s: invalid interrupt pin %d\n", pci_name(dev), pin);
        return -1;
    }
    /* Pin index so minus one */
    irq_bit = g_pcie_port_defs[pcie_port].legacy_irqs.legacy_irq[pin - 1].irq_bit;
    irq = g_pcie_port_defs[pcie_port].legacy_irqs.legacy_irq[pin - 1].irq;
    IFX_REG_SET_BIT(irq_bit, PCIE_IRNEN(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_IRNEN: 0x%08x\n", __func__, IFX_REG_R32(PCIE_IRNEN(pcie_port)));
    IFX_REG_SET_BIT(irq_bit, PCIE_IRNCR(pcie_port));
    IFX_PCIE_PRINT(PCIE_MSG_REG, "%s PCIE_IRNCR: 0x%08x\n", __func__, IFX_REG_R32(PCIE_IRNCR(pcie_port)));
    IFX_PCIE_PRINT(PCIE_MSG_FIXUP, "%s dev %s irq %d assigned\n", __func__, pci_name(dev), irq);
    IFX_PCIE_PRINT(PCIE_MSG_FIXUP, "%s dev %s: exit\n", __func__, pci_name(dev));
    return irq;
}

/** 
 * \fn int ifx_pcie_bios_plat_dev_init(struct pci_dev *dev)
 * \brief Called to perform platform specific PCI setup 
 * 
 * \param[in] dev The Linux PCI device structure for the device to map
 * \return OK
 * \ingroup IFX_PCIE_OS
 */ 
int 
ifx_pcie_bios_plat_dev_init(struct pci_dev *dev)
{
    u16 config;
#ifdef IFX_PCIE_BASIC_ERROR_INT
    int pos;
#ifdef IFX_PCIE_AER_REPORT
    u32 dconfig; 
#endif /* IFX_PCIE_AER_REPORT */
#endif /* IFX_PCIE_BASIC_ERROR_INT */

    IFX_PCIE_PRINT(PCIE_MSG_INIT, "%s enter \n", __func__);
    /* Enable reporting System errors and parity errors on all devices */ 
    /* Enable parity checking and error reporting */ 
    pci_read_config_word(dev, PCI_COMMAND, &config);
    config |= PCI_COMMAND_PARITY | PCI_COMMAND_SERR /*| PCI_COMMAND_INVALIDATE |
          PCI_COMMAND_FAST_BACK*/;
    pci_write_config_word(dev, PCI_COMMAND, config);

    if (dev->subordinate) {
        /* Set latency timers on sub bridges */
        pci_write_config_byte(dev, PCI_SEC_LATENCY_TIMER, 0x40); /* XXX, */
        /* More bridge error detection */
        pci_read_config_word(dev, PCI_BRIDGE_CONTROL, &config);
        config |= PCI_BRIDGE_CTL_PARITY | PCI_BRIDGE_CTL_SERR;
        pci_write_config_word(dev, PCI_BRIDGE_CONTROL, config);
    }
#ifdef IFX_PCIE_BASIC_ERROR_INT
    /* Enable the PCIe normal error reporting */
    pos = pci_find_capability(dev, PCI_CAP_ID_EXP);
    if (pos) {

        /* Disable system error generation in response to error messages */
        pci_read_config_word(dev, pos + PCI_EXP_RTCTL, &config);
        config &= ~(PCI_EXP_RTCTL_SECEE | PCI_EXP_RTCTL_SENFEE | PCI_EXP_RTCTL_SEFEE);
        pci_write_config_word(dev, pos + PCI_EXP_RTCTL, config);

        /* Clear PCIE Capability's Device Status */
        pci_read_config_word(dev, pos + PCI_EXP_DEVSTA, &config);
        pci_write_config_word(dev, pos + PCI_EXP_DEVSTA, config);

        /* Update Device Control */ 
        pci_read_config_word(dev, pos + PCI_EXP_DEVCTL, &config);
        /* Correctable Error Reporting */
        config |= PCI_EXP_DEVCTL_CERE;
        /* Non-Fatal Error Reporting */
        config |= PCI_EXP_DEVCTL_NFERE;
        /* Fatal Error Reporting */
        config |= PCI_EXP_DEVCTL_FERE;
        /* Unsupported Request */
        config |= PCI_EXP_DEVCTL_URRE;
        pci_write_config_word(dev, pos + PCI_EXP_DEVCTL, config);
    }
#ifdef IFX_PCIE_AER_REPORT
    /* Find the Advanced Error Reporting capability */
    pos = pci_find_ext_capability(dev, PCI_EXT_CAP_ID_ERR);
    if (pos) {
        /* Clear Uncorrectable Error Status */ 
        pci_read_config_dword(dev, pos + PCI_ERR_UNCOR_STATUS, &dconfig);
        pci_write_config_dword(dev, pos + PCI_ERR_UNCOR_STATUS, dconfig);
        /* Enable reporting of all uncorrectable errors */
        /* Uncorrectable Error Mask - turned on bits disable errors */
        pci_write_config_dword(dev, pos + PCI_ERR_UNCOR_MASK, 0);
        /* 
        * Leave severity at HW default. This only controls if 
        * errors are reported as uncorrectable or 
        * correctable, not if the error is reported. 
        */ 
        /* PCI_ERR_UNCOR_SEVER - Uncorrectable Error Severity */
        /* Clear Correctable Error Status */
        pci_read_config_dword(dev, pos + PCI_ERR_COR_STATUS, &dconfig);
        pci_write_config_dword(dev, pos + PCI_ERR_COR_STATUS, dconfig);
        /* Enable reporting of all correctable errors */
        /* Correctable Error Mask - turned on bits disable errors */
        pci_write_config_dword(dev, pos + PCI_ERR_COR_MASK, 0);
        /* Advanced Error Capabilities */ 
        pci_read_config_dword(dev, pos + PCI_ERR_CAP, &dconfig);
        /* ECRC Generation Enable */
        if (dconfig & PCI_ERR_CAP_ECRC_GENC) {
            dconfig |= PCI_ERR_CAP_ECRC_GENE;
        }
        /* ECRC Check Enable */
        if (dconfig & PCI_ERR_CAP_ECRC_CHKC) {
            dconfig |= PCI_ERR_CAP_ECRC_CHKE;
        }
        pci_write_config_dword(dev, pos + PCI_ERR_CAP, dconfig);

        /* PCI_ERR_HEADER_LOG - Header Log Register (16 bytes) */
        /* Enable Root Port's interrupt in response to error messages */
        pci_write_config_dword(dev, pos + PCI_ERR_ROOT_COMMAND,
              PCI_ERR_ROOT_CMD_COR_EN |
              PCI_ERR_ROOT_CMD_NONFATAL_EN |
              PCI_ERR_ROOT_CMD_FATAL_EN); 
        /* Clear the Root status register */
        pci_read_config_dword(dev, pos + PCI_ERR_ROOT_STATUS, &dconfig);
        pci_write_config_dword(dev, pos + PCI_ERR_ROOT_STATUS, dconfig);
    }
#endif /* IFX_PCIE_AER_REPORT */
#endif /* IFX_PCIE_BASIC_ERROR_INT */
    /* WAR, only 128 MRRS is supported, force all EPs to support this value */
    pcie_set_readrq(dev, 128);
    IFX_PCIE_PRINT(PCIE_MSG_INIT, "%s exit \n", __func__);
    return 0;
}

static void
pcie_phy_rst(int pcie_port)
{
     pcie_phy_rst_assert(pcie_port);
     pcie_phy_rst_deassert(pcie_port);

     /* Make sure PHY PLL is stable */
     udelay(20);
}

static int
pcie_rc_initialize(int pcie_port)
{
    int i;
    int rc_port = pcie_port_idx_to_rc_port(pcie_port);
#define IFX_PCIE_PHY_LOOP_CNT  5

    if (pcie_rc_fused(rc_port)) {
        printk(KERN_INFO "%s: PCIe Core %d and SERDES fused!!!!!!!!!!!\n", __func__, pcie_port);
        return -EIO;
    }
    
    pcie_rcu_endian_setup(rc_port);

    pcie_ep_rst_init(rc_port);
    /* 
     * XXX, PCIe elastic buffer bug will cause not to be detected. One more 
     * reset PCIe PHY will solve this issue 
     */
    for (i = 0; i < IFX_PCIE_PHY_LOOP_CNT; i++) {
        /* Disable PCIe PHY Analog part for sanity check */
        pcie_phy_pmu_disable(rc_port);
        pcie_phy_rst(rc_port);
        /* PCIe Core reset enabled, low active, sw programmed */
        pcie_core_rst_assert(rc_port);
        /* Put PCIe EP in reset status */
        pcie_device_rst_assert(rc_port);

        /* PCI PHY & Core reset disabled, high active, sw programmed */
        pcie_core_rst_deassert(rc_port);
    #ifndef CONFIG_USE_EMULATOR
        /* Already in a quiet state, program PLL, enable PHY, check ready bit */
        if (pcie_phy_clock_mode_setup(pcie_port) !=0){
        #ifndef CONFIG_HN1
            return -EIO;
        #endif
        }
    #endif /* CONFIG_USE_EMULATOR */

        /* Enable PCIe PHY and Clock */
        pcie_core_pmu_setup(rc_port);

        /* Clear status registers */
        pcie_status_register_clear(pcie_port);

    #ifdef CONFIG_PCI_MSI
        pcie_msi_init(pcie_port);
    #endif /* CONFIG_PCI_MSI */
        pcie_rc_cfg_reg_setup(pcie_port);

        /* Once link is up, break out */
        if (pcie_app_loigc_setup(pcie_port) == 0) {
            break;
        }
    }
    if (i >= IFX_PCIE_PHY_LOOP_CNT) {
        printk(KERN_ERR "%s port %d link up failed!!!!!\n", __func__, rc_port);
        return -EIO;
    }
    /* NB, don't increase ACK/NACK timer timeout value, which will cause a lot of COR errors */
    pcie_replay_time_update(pcie_port);
#ifdef IFX_PCIE_DBG
    pcie_post_dump(pcie_port);
    pcie_status_registers_dump(pcie_port);
#endif /* IFX_PCIE_DBG */
    return 0;
}

/** 
 * \fn static int __init ifx_pcie_bios_init(void)
 * \brief Initialize the IFX PCIe controllers
 *
 * \return -EIO    PCIe PHY link is not up
 * \return -ENOMEM Configuration/IO space failed to map
 * \return 0       OK
 * \ingroup IFX_PCIE_OS
 */ 
static int __init 
ifx_pcie_bios_init(void)
{
    char ver_str[128] = {0};
    void __iomem *io_map_base;
    int pcie_port; /* Logical index */

    IFX_PCIE_PRINT(PCIE_MSG_INIT, "%s enter \n", __func__);

#ifdef CONFIG_VR9
    /* Enable AHB Master/ Slave, shared by all RCs */
    pcie_ahb_pmu_setup();
#endif /* CONFIG_VR9 */

#ifdef CONFIG_AR10_GRX390
    pcie_ahb_arbiter_setup();
#endif /* CONFIG_GRX390 */
    
    for (pcie_port = IFX_PCIE_PORT0; pcie_port < IFX_PCIE_PORT_MAX; pcie_port++) {
            if (pcie_rc_initialize(pcie_port) == 0) {
                IFX_PCIE_PRINT(PCIE_MSG_INIT, "%s: ifx_pcie_cfg_base 0x%p\n", 
                     __func__, PCIE_CFG_PORT_TO_BASE(pcie_port));
                /* Otherwise, warning will pop up */
                io_map_base = ioremap(PCIE_IO_PHY_PORT_TO_BASE(pcie_port),
                    (g_pcie_port_defs[pcie_port].port_to_addr.io_phy_end - g_pcie_port_defs[pcie_port].port_to_addr.io_phy_base) + 1);
                if (io_map_base == NULL) {
                    IFX_PCIE_PRINT(PCIE_MSG_ERR, "%s io space ioremap failed\n", __func__);
                    return -ENOMEM;
                }
                spin_lock_init(&g_pcie_port_defs[pcie_port].controller.lock);
                g_pcie_port_defs[pcie_port].controller.pcic.io_map_base = (unsigned long)io_map_base;

                register_pci_controller(&g_pcie_port_defs[pcie_port].controller.pcic);
                /* XXX, clear error status */
                IFX_PCIE_PRINT(PCIE_MSG_INIT, "%s: mem_resource 0x%p, io_resource 0x%p\n", 
                                  __func__, &g_pcie_port_defs[pcie_port].controller.pcic.mem_resource, 
                                  &g_pcie_port_defs[pcie_port].controller.pcic.io_resource);

            #ifdef IFX_PCIE_BASIC_ERROR_INT
                pcie_rc_core_int_init(pcie_port);
            #endif /* IFX_PCIE_BASIC_ERROR_INT */
            }
        }

    ifx_drv_ver(ver_str, "PCIe Root Complex", IFX_PCIE_VER_MAJOR, IFX_PCIE_VER_MID, IFX_PCIE_VER_MINOR);
    printk(KERN_INFO "%s", ver_str);
    return 0;
#undef IFX_PCIE_PHY_LOOP_CNT
}
arch_initcall(ifx_pcie_bios_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chuanhua.Lei@infineon.com");
MODULE_SUPPORTED_DEVICE("Infineon builtin PCIe RC module");
MODULE_DESCRIPTION("Infineon builtin PCIe RC driver");

