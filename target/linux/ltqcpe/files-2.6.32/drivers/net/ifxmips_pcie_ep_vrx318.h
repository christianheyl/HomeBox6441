/****************************************************************************
                              Copyright (c) 2011
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************/
/*!
 \file ifxmips_pcie_ep_vrx318.h
 \ingroup IFX_PCIE_EP_VRX318   
 \brief header file for SmartPHY PCIe EP address mapping driver internal definition
*/
#ifndef IFX_PCIE_EP_VRX318_H
#define IFX_PCIE_EP_VRX318_H

#include <linux/version.h>
#include <linux/types.h>
#include <linux/pci.h>

#include <asm/ifx/ifx_pcie.h>

#if ( LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24) )
#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))
#endif /* < 2.6.24 */

#define IFX_PCIE_EP_MAX_NUM                    (IFX_PCIE_EP_MAX_PEER + 1)

/* Maximum 4, if PCIe switch attached, 2 is used. 2 is also default one */
#ifdef CONFIG_VRX318_PCIE_SW_DSL_BONDING
#define IFX_PCIE_EP_DEFAULT_MSI_VECTOR         2
#else
#define IFX_PCIE_EP_DEFAULT_MSI_VECTOR         4
#endif /*  CONFIG_VRX318_PCIE_SW_DSL_BONDING*/

#define IFX_PCIE_EP_MAX_REFCNT                 IFX_PCIE_EP_INT_MODULE_MAX

/* iATU specific register offset definition */

/* View Point Register */
#define PCIE_PL_IATU_VIEWPORT                  0x900

#define PCIE_PL_IATU_REGION_IDX                0x0000000F
#define PCIE_PL_IATU_REGION_IDX_S              0

/* Inbound and outbound has seperate regions, each one has 8 regions from index 0 */
enum {
    PCIE_PL_IATU_REGION0 = 0,
    PCIE_PL_IATU_REGION1,
    PCIE_PL_IATU_REGION2,
    PCIE_PL_IATU_REGION3,
    PCIE_PL_IATU_REGION4,
    PCIE_PL_IATU_REGION5,
    PCIE_PL_IATU_REGION6,
    PCIE_PL_IATU_REGION7,
};
#define PCIE_PL_IATU_REGION_INBOUND            0x80000000

/* Region control registe for all kinds of types */
#define PCIE_PL_IATU_REGION_CTRL1              0x904

#define PCIE_PL_IATU_REGION_CTRL2              0x908
#define PCIE_PL_IATU_REGION_BAR                0x00000700
#define PCIE_PL_IATU_REGION_BAR_S              8

enum {
    PCIE_PL_IATU_REGION_BAR0 = 0,
    PCIE_PL_IATU_REGION_BAR1,
    PCIE_PL_IATU_REGION_BAR2,
    PCIE_PL_IATU_REGION_BAR3,
    PCIE_PL_IATU_REGION_BAR4,
    PCIE_PL_IATU_REGION_BAR5,
};
#define PCIE_PL_IATU_REGION_MATCH_EN           0x40000000
#define PCIE_PL_IATU_REGION_EN                 0x80000000

#define PCIE_PL_IATU_REGION_LOWER_BASE_ADDR    0x90C
#define PCIE_PL_IATU_REGION_UPPER_BASE_ADDR    0x910
#define PCIE_PL_IATU_REGION_LIMIT              0x914
#define PCIE_PL_IATU_REGION_LOWER_TARGET_ADDR  0x918
#define PCIE_PL_IATU_REGION_UPPER_TARGET_ADDR  0x91C

/* Target & Base address definition for Inbound/Outbound */

/* Inbound address translation for iATU0 */
#define PCIE_EP_INBOUND_INTERNAL_BASE          0x1E000000
#define PCIE_EP_OUTBOUND_INTERNAL_BASE         0x20000000
#define PCIE_EP_OUTBOUND_MEMSIZE               0x80000000

/* EMI control stuff */
/* 36MHz clockout */
#define PCIE_EP_IF_CLK                         0x00003024
#define PCIE_EP_IF_CLK_NO_36MHZ_CLKOUT         0x00000400

/* GPIO 1 Alternate1 Set/Clear */
#define PCIE_EP_P0_ALTSEL1                     0x00102B20
#define PCIE_EP_P0_ALTSEL1_PIN1_SET            0x00000002

#if 1
/* arcadyan: GPIO12 on VRX318, a 72Mhz signal Clear 
1. set P0_ALTSEL1 Bit 12 to 0
2. set P0_PUDSEL and P0_PUDEN bit12 to 1
*/
//#define PCIE_EP_P0_ALTSEL1                   0x00102B20
#define PCIE_EP_P0_ALTSEL1_BIT12_SET        0x00001000
#define PCIE_EP_P0_PUDSEL                   0x00102B2C
#define PCIE_EP_P0_PUDSEL_BIT12_SET         0x00001000
#define PCIE_EP_P0_PUDEN                    0x00102B30
#define PCIE_EP_P0_PUDEN_BIT12_SET          0x00001000
#endif

/** Structure used to extract attached EP detailed information for PPE/DSL_MEI driver/Bonding */
typedef struct pcie_ep_dev_priv {
    u32 rc_idx;              /*!< which Root Complex is attached by the current EP, 0 -- RC0, 1 -- RC1*/
    u32 card_idx;            /*!< EP logical index, the first found one will be 0 regardless of RC physical index */
    u32 irq_base;            /*!< The first MSI interrupt number */
    u32 irq_num;             /*!< How many MSI interrupt supported */
    u8 __iomem *membase;     /*!< The EP inbound memory base address derived from BAR0, SoC virtual address for PPE/DSL_MEI driver */
    u32 phy_membase;         /*!< The EP inbound memory base address derived from BAR0, physical address for PPE FW */
    size_t memsize;          /*!< The EP inbound memory window size */
    u32  peer_num;           /*!< Bonding peer number available */
    u8 __iomem *peer_membase[IFX_PCIE_EP_MAX_PEER];  /*!< The bonding peer EP inbound memory base address derived from its BAR0, SoC virtual address for PPE/DSL_MEI driver */
    u32 peer_phy_membase[IFX_PCIE_EP_MAX_PEER];      /*!< The bonding peer EP inbound memory base address derived from its BAR0, physical address for PPE FW */
    size_t peer_memsize[IFX_PCIE_EP_MAX_PEER];       /*!< The bonding peer inbound memory window size */
    atomic_t refcnt;         /*!< The EP mapping driver referenced times by other modules */
}ifx_pcie_ep_dev_priv_t;

typedef struct ifx_pcie_ep_info {
    int dev_num;
    ifx_pcie_ep_dev_priv_t pcie_ep[IFX_PCIE_EP_MAX_NUM];
} ifx_pcie_ep_info_t;

/* Card specific private data structure */
typedef struct ifx_pcie_ep_adapter {
    /* OS defined structs */
    struct pci_dev *pdev;
    unsigned long phy_mem;  /* Physical address */
    u8 __iomem *mem;      /* Virtual address */
    size_t      memsize;
    u32         card_num;  /* EP card index */
    u32         rc_phy_idx; /* Attached which RC */

    /* PCI config space info */
    u16         device_id ;
    u16         irq_base; /* irq base for multiple MSI */
    u32         msi_nvec;   /* MSI vector number supported */
}ifx_pcie_ep_adapter_t;

#endif /* IFX_PCIE_EP_VRX318_H */

