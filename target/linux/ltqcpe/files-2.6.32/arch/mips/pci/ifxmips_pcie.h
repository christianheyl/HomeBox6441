/******************************************************************************
**
** FILE NAME    : ifxmips_pcie.h
** PROJECT      : IFX UEIP for VRX200
** MODULES      : PCIe module
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
** 0.0.1    17 Mar,2009  Lei Chuanhua    Initial version
*******************************************************************************/
#ifndef IFXMIPS_PCIE_H
#define IFXMIPS_PCIE_H
#include <linux/version.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>

#include <asm/ifx/ifx_pcie.h>
#include "ifxmips_pci_common.h"
#include "ifxmips_pcie_reg.h"

/*!
 \defgroup IFX_PCIE  PCI Express bus driver module   
 \brief  PCI Express IP module support VRX200/ARX300/HN 
*/

/*!
 \defgroup IFX_PCIE_OS OS APIs
 \ingroup IFX_PCIE
 \brief PCIe bus driver OS interface functions
*/

/*!
 \file ifxmips_pcie.h
 \ingroup IFX_PCIE  
 \brief header file for PCIe module common header file
*/

/* Debug option, more will be coming */

//#define IFX_PCIE_DBG

/* Reuse kernel stuff, but we need to differentiate baseline error reporting and AEE */
#ifdef CONFIG_PCIEAER
#define IFX_PCIE_BASIC_ERROR_INT
#endif /* CONFIG_PCIEAER */

/* XXX, should be only enabled after IFX_PCIE_BASIC_ERRORINT */
#define IFX_PCIE_AER_REPORT


#define PCIE_IRQ_LOCK(lock) do {             \
    unsigned long flags;                     \
    spin_lock_irqsave(&(lock), flags);
#define PCIE_IRQ_UNLOCK(lock)                \
    spin_unlock_irqrestore(&(lock), flags);  \
} while (0)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#define IRQF_SHARED SA_SHIRQ
#endif

#define PCIE_MSG_MSI        0x00000001
#define PCIE_MSG_ISR        0x00000002
#define PCIE_MSG_FIXUP      0x00000004
#define PCIE_MSG_READ_CFG   0x00000008
#define PCIE_MSG_WRITE_CFG  0x00000010
#define PCIE_MSG_CFG        (PCIE_MSG_READ_CFG | PCIE_MSG_WRITE_CFG)
#define PCIE_MSG_REG        0x00000020
#define PCIE_MSG_INIT       0x00000040
#define PCIE_MSG_ERR        0x00000080
#define PCIE_MSG_PHY        0x00000100
#define PCIE_MSG_ANY        0x000001ff

/* Port number definition */
enum {
    IFX_PCIE_PORT0 = 0,
    IFX_PCIE_PORT1,
    IFX_PCIE_PORT2,
};

#if defined(IFX_PCIE_DBG)
#define IFX_PCIE_PRINT(_m, _fmt, args...) do {   \
    if (g_pcie_debug_flag & (_m)) {              \
        ifx_pcie_debug((_fmt), ##args);          \
    }                                            \
} while (0)

#define INLINE 
#else
#define IFX_PCIE_PRINT(_m, _fmt, args...)   \
    do {} while(0)
#define INLINE inline
#endif

#define IFX_MSI_IRQ_NUM    16

enum {
    IFX_PCIE_MSI_IDX0 = 0,
    IFX_PCIE_MSI_IDX1,
    IFX_PCIE_MSI_IDX2,
    IFX_PCIE_MSI_IDX3,
};

/* The structure will store mapping address to support multiple RC */
typedef struct ifx_pcie_addr_map {
    const u32 cfg_base;
    const u32 mem_base;
    const u32 io_base;
    const u32 mem_phy_base;
    const u32 mem_phy_end;
    const u32 io_phy_base;
    const u32 io_phy_end;
    const u32 app_logic_base;
    const u32 rc_addr_base;
    const u32 phy_base;
} ifx_pcie_addr_map_t;

typedef struct ifx_msi_irq_idx {
    const int irq;
    const int idx;
}ifx_msi_irq_idx_t;

struct ifx_msi_pic {
    volatile u32  pic_table[IFX_MSI_IRQ_NUM];
    volatile u32  pic_endian;    /* 0x40  */
};
typedef struct ifx_msi_pic *ifx_msi_pic_t;

typedef struct ifx_msi_irq {
    const volatile ifx_msi_pic_t msi_pic_p;
    const u32 msi_phy_base;
    const ifx_msi_irq_idx_t msi_irq_idx[IFX_MSI_IRQ_NUM];
    spinlock_t msi_lock;
    /*
     * Each bit in msi_free_irq_bitmask represents a MSI interrupt that is 
     * in use.
     */
    u16 msi_free_irq_bitmask;

    /*
     * Each bit in msi_multiple_irq_bitmask tells that the device using 
     * this bit in msi_free_irq_bitmask is also using the next bit. This 
     * is used so we can disable all of the MSI interrupts when a device 
     * uses multiple.
     */
    u16 msi_multiple_irq_bitmask;
}ifx_msi_irq_t;

typedef struct ifx_pcie_ir_irq {
    const unsigned int irq;
    const char name[16];
}ifx_pcie_ir_irq_t;

typedef struct ifx_pcie_legacy_irq{
    const u32 irq_bit;
    const int irq;
}ifx_pcie_legacy_irq_t;

typedef struct ifx_pcie_irq {
    ifx_pcie_ir_irq_t ir_irq;
    ifx_pcie_legacy_irq_t legacy_irq[PCIE_LEGACY_INT_MAX];
}ifx_pcie_irq_t;

typedef struct ifx_pcie_port {
    ifx_pcie_addr_map_t port_to_addr;
    ifx_pci_ctrl_t controller;
    ifx_pcie_irq_t legacy_irqs;
    ifx_msi_irq_t msi_irqs;
    int rc_physical_port;
} ifx_pcie_port_t;

extern u32 g_pcie_debug_flag;
extern void ifx_pcie_debug(const char *fmt, ...);
extern int pcie_phy_clock_ppm_enabled(void);
extern int pcie_phy_clock_mode_setup(int pcie_port);
extern void pcie_msi_pic_init(int pcie_port);
extern int pcie_rc_port_to_pcie_port(int rc_port, int *pcie_port);
extern int pcie_port_idx_to_rc_port(int pcie_port);

#if defined (CONFIG_VR9) || defined (CONFIG_HN1)
#include "ifxmips_pcie_vr9.h"
#elif defined (CONFIG_AR10)
#include "ifxmips_pcie_ar10.h"
#else
#error "PCIE: platform not defined"
#endif /* CONFIG_VR9 */

/* Port number defined in platform specific file */
extern ifx_pcie_port_t g_pcie_port_defs[IFX_PCIE_PORT_MAX];

#endif  /* IFXMIPS_PCIE_H */

