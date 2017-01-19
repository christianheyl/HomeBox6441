/****************************************************************************
                              Copyright (c) 2011
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************/
/*!
  \defgroup IFX_PCIE_EP_VRX318_INTERNAL Internal functions
  \ingroup IFX_PCIE_EP_VRX318
  \brief IFX PCIe EP internal driver functions
*/

/*!
  \defgroup IFX_PCIE_EP_VRX318_OS OS APIs
  \ingroup IFX_PCIE_EP_VRX318
  \brief IFX PCIe EP OS APIs for driver interface
*/

/*!
   \file ifxmips_pcie_ep_vrx318.c
   \ingroup IFX_PCIE_EP_VRX318
   \brief SmartPHY PCIe EP address mapping driver source file
*/
#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/types.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/pci_regs.h>

/* Project header file */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>
#include "ifxmips_pcie_ep_vrx318.h"

//#define IFX_EP_DBG

#define IFX_PCIE_EP_VER_MAJOR          1
#define IFX_PCIE_EP_VER_MID            0
#define IFX_PCIE_EP_VER_MINOR          1

static ifx_pcie_ep_info_t g_pcie_ep_info;
static const char ltq_pcie_driver_name[] = "ltq_pcie";
static DEFINE_SPINLOCK(ifx_pcie_ep_lock);

int ifx_pcie_ep_dev_num_get(int *dev_num)
{
    if ((g_pcie_ep_info.dev_num <= 0) || (g_pcie_ep_info.dev_num > IFX_PCIE_EP_MAX_NUM)) {
        return -EIO;
    }
    
    *dev_num = g_pcie_ep_info.dev_num;
    return 0;
}
EXPORT_SYMBOL_GPL(ifx_pcie_ep_dev_num_get);

int ifx_pcie_ep_dev_info_req(int dev_idx, ifx_pcie_ep_int_module_t module, ifx_pcie_ep_dev_t *dev)
{
    int i;
    
    if ((dev_idx < 0) || (dev_idx >= IFX_PCIE_EP_MAX_NUM)) {
        printk("%s invalid device index %d\n", __func__, dev_idx);
        return -EIO;
    }
    
    if (atomic_read(&g_pcie_ep_info.pcie_ep[dev_idx].refcnt) >= IFX_PCIE_EP_MAX_REFCNT) {
        printk("%s mismatch request/release module usage\n", __func__);
        return -EIO;
    }
    
    switch (module) {
        case IFX_PCIE_EP_INT_PPE:
            dev->irq = g_pcie_ep_info.pcie_ep[dev_idx].irq_base;
            break;
            
        case IFX_PCIE_EP_INT_MEI:
            dev->irq = g_pcie_ep_info.pcie_ep[dev_idx].irq_base + 1;
            /* XXX, Hardcode workaround for PCIe switch bonding for RC0 */
            if (dev->irq == 158) {
                dev->irq = 30;
            }
            break;
            
        case IFX_PCIE_EP_INT_DYING_GASP:
        case IFX_PCIE_EP_INT_EDMA:
        case IFX_PCIE_EP_INT_FPI_BCU:
        case IFX_PCIE_EP_INT_ARC_LED0:
        case IFX_PCIE_EP_INT_ARC_LED1:
            dev->irq = g_pcie_ep_info.pcie_ep[dev_idx].irq_base + 2;
            break;
            
        case IFX_PCIE_EP_INT_DMA:
            if (g_pcie_ep_info.pcie_ep[dev_idx].rc_idx == 0) {
                dev->irq = 30; /* Hardcode */  
            }
            else {
                dev->irq = g_pcie_ep_info.pcie_ep[dev_idx].irq_base + 3;
            }
            break;
            
        default:
            dev->irq = g_pcie_ep_info.pcie_ep[dev_idx].irq_base;
            break;  
    }

    dev->membase = g_pcie_ep_info.pcie_ep[dev_idx].membase;
    dev->phy_membase = g_pcie_ep_info.pcie_ep[dev_idx].phy_membase;
    dev->peer_num = g_pcie_ep_info.pcie_ep[dev_idx].peer_num;
    for (i = 0; i < dev->peer_num; i++) {
        dev->peer_membase[i] = g_pcie_ep_info.pcie_ep[dev_idx].peer_membase[i];
        dev->peer_phy_membase[i] = g_pcie_ep_info.pcie_ep[dev_idx].peer_phy_membase[i];
    }
    atomic_inc(&g_pcie_ep_info.pcie_ep[dev_idx].refcnt);
    return 0;
}
EXPORT_SYMBOL_GPL(ifx_pcie_ep_dev_info_req);

int ifx_pcie_ep_dev_info_release(int dev_idx)
{
    if ((dev_idx < 0) || (dev_idx >= IFX_PCIE_EP_MAX_NUM)) {
        printk("%s invalid device index %d\n", __func__, dev_idx);
        return -EIO;
    }
    
    if (atomic_read(&g_pcie_ep_info.pcie_ep[dev_idx].refcnt) <= 0) {
        printk("%s mismatch request/release module usage\n", __func__);
        return -EIO;
    }
    atomic_dec(&g_pcie_ep_info.pcie_ep[dev_idx].refcnt);
    return 0;
}
EXPORT_SYMBOL_GPL(ifx_pcie_ep_dev_info_release);

static struct pci_device_id ltq_pcie_id_table[] = {
    { 0x1bef, 0x0020, PCI_ANY_ID, PCI_ANY_ID }, /* SmartPHY */
    { 0 },
};

/**
 * \fn  static int pci_msi_max_vector_get(struct pci_dev *dev, int *nvec)
 *
 * \brief  This function tries to get device's MSI interrupt vector number
 *
 * \param  dev      Device to operate
 * \param  nvec     Numer of interrupts to get
 * \return 0        on success
 * \return -EINVAL  The device doesn't support MSI capability
 *
 * \ingroup IFX_PCIE_EP_VRX318_INTERNAL
 */
static int 
pci_msi_max_vector_get(struct pci_dev *dev, int *nvec)
{
    int pos, maxvec;
    u16 msgctl;

    pos = pci_find_capability(dev, PCI_CAP_ID_MSI);
    if (!pos) {
        return -EINVAL;
    }
    pci_read_config_word(dev, pos + PCI_MSI_FLAGS, &msgctl);
    maxvec = 1 << ((msgctl & PCI_MSI_FLAGS_QMASK) >> 1);
    *nvec = maxvec;
    return 0;
}

/**
 * \fn  static int pci_msi_max_vector_set(struct pci_dev *dev, int nvec)
 *
 * \brief  This function tries to set device's MSI interrupt vector number
 *
 * \param  dev      Device to operate
 * \param  nvec     Numer of interrupts to set
 * \return 0        on success
 * \return -EINVAL  The device doesn't support MSI capability or invalid interrupt number
 *
 * \remark smartPHY only supports 1 or 2 or 4 interrupts depend on other module usage
 * \ingroup IFX_PCIE_EP_VRX318_INTERNAL
 */
static int 
pci_msi_max_vector_set(struct pci_dev *dev, int nvec)
{
    int pos;
    u16 msgctl;

    if ((nvec != 1) && (nvec != 2) && (nvec != 4)) {
        return -EINVAL;
    }
    pos = pci_find_capability(dev, PCI_CAP_ID_MSI);
    if (!pos) {
        return -EINVAL;
    }
    pci_read_config_word(dev, pos + PCI_MSI_FLAGS, &msgctl);
    msgctl &= ~PCI_MSI_FLAGS_QSIZE;
    msgctl |= (nvec >> 1) << 4;
    pci_write_config_word(dev, pos + PCI_MSI_FLAGS, msgctl);
    pci_read_config_word(dev, pos + PCI_MSI_FLAGS, &msgctl);
    /* ICU control if necessary */
    return 0;
}

#ifdef IFX_EP_DBG
static void
ltq_pcie_iatu_dump(struct pci_dev *pdev, int outbound)
{
    u32 val;
    
    switch (outbound) {
        case 0:
            /* Inbound iATU0 */
            pci_read_config_dword(pdev, PCIE_PL_IATU_VIEWPORT, &val);
            val &= ~PCIE_PL_IATU_REGION_IDX;
            /* Inbound, region0 */
            val |= PCIE_PL_IATU_REGION_INBOUND | SM(PCIE_PL_IATU_REGION0, PCIE_PL_IATU_REGION_IDX);
            pci_write_config_dword(pdev, PCIE_PL_IATU_VIEWPORT, val);
            break;
        case 1:
        default:
            pci_read_config_dword(pdev, PCIE_PL_IATU_VIEWPORT, &val);
            val &= ~PCIE_PL_IATU_REGION_IDX;
            /* Outbound, region1 */
            val &= ~PCIE_PL_IATU_REGION_INBOUND;
            val |= SM(PCIE_PL_IATU_REGION0, PCIE_PL_IATU_REGION_IDX);
            pci_write_config_dword(pdev, PCIE_PL_IATU_VIEWPORT, val);
            break;
    }
    printk("iATU %s\n", outbound? "Outbound" : "Inbound");
    pci_read_config_dword(pdev, PCIE_PL_IATU_VIEWPORT, &val);
    printk("PCIE_PL_IATU_VIEWPORT: %08x\n", val);
    pci_read_config_dword(pdev, PCIE_PL_IATU_REGION_LOWER_BASE_ADDR, &val);
    printk("PCIE_PL_IATU_REGION_LOWER_BASE_ADDR: %08x\n", val);
    pci_read_config_dword(pdev, PCIE_PL_IATU_REGION_UPPER_BASE_ADDR, &val);
    printk("PCIE_PL_IATU_REGION_UPPER_BASE_ADDR: %08x\n", val);
    pci_read_config_dword(pdev, PCIE_PL_IATU_REGION_LIMIT, &val);
    printk("PCIE_PL_IATU_REGION_LIMIT: %08x\n", val);
    pci_read_config_dword(pdev, PCIE_PL_IATU_REGION_LOWER_TARGET_ADDR, &val);
    printk("PCIE_PL_IATU_REGION_LOWER_TARGET_ADDR: %08x\n", val);
    pci_read_config_dword(pdev, PCIE_PL_IATU_REGION_UPPER_TARGET_ADDR, &val);
    printk("PCIE_PL_IATU_REGION_UPPER_TARGET_ADDR: %08x\n", val);
    pci_read_config_dword(pdev, PCIE_PL_IATU_REGION_CTRL1, &val);
    printk("PCIE_PL_IATU_REGION_CTRL1: %08x\n", val);
    pci_read_config_dword(pdev, PCIE_PL_IATU_REGION_CTRL2, &val);
    printk("PCIE_PL_IATU_REGION_CTRL2: %08x\n", val);
}
#endif /* IFX_EP_DBG */

/**
 * \fn  static void  ltq_pcie_iatu_setup(struct pci_dev *pdev)
 *
 * \brief  This function configures smartPHY PCIe EP inbound address translation iATU0
 *         and Outbound address translation <iATU1>. Detailed information, please refer
 *         to related design documentation.
 *
 * \param  dev      PCI device to configure
 * \return none
 *
 * \ingroup IFX_PCIE_EP_VRX318_INTERNAL
 */
static void 
ltq_pcie_iatu_setup(struct pci_dev *pdev)
{
    u32 val;
    
    /* Inbound iATU0 */
    pci_read_config_dword(pdev, PCIE_PL_IATU_VIEWPORT, &val);
    val &= ~PCIE_PL_IATU_REGION_IDX;
    /* Inbound, region0 */
    val |= PCIE_PL_IATU_REGION_INBOUND | SM(PCIE_PL_IATU_REGION0, PCIE_PL_IATU_REGION_IDX);
    pci_write_config_dword(pdev, PCIE_PL_IATU_VIEWPORT, val);

    /* BAR match used, there is no need to configure base and limit register */
    pci_write_config_dword(pdev, PCIE_PL_IATU_REGION_LOWER_TARGET_ADDR, (u32)PCIE_EP_INBOUND_INTERNAL_BASE);

    pci_write_config_dword(pdev, PCIE_PL_IATU_REGION_UPPER_TARGET_ADDR, 0);
    
    pci_write_config_dword(pdev, PCIE_PL_IATU_REGION_CTRL1, 0);
    /* Inbound BAR match, BAR0 used only */
    val = PCIE_PL_IATU_REGION_MATCH_EN | PCIE_PL_IATU_REGION_EN | SM(PCIE_PL_IATU_REGION_BAR0, PCIE_PL_IATU_REGION_BAR);
    pci_write_config_dword(pdev, PCIE_PL_IATU_REGION_CTRL2, val);
    
    /* Outbound iATU1 */
    pci_read_config_dword(pdev, PCIE_PL_IATU_VIEWPORT, &val);
    val &= ~PCIE_PL_IATU_REGION_IDX;
    /* Outbound, region0 */
    val &= ~PCIE_PL_IATU_REGION_INBOUND;
    val |= SM(PCIE_PL_IATU_REGION0, PCIE_PL_IATU_REGION_IDX);
    pci_write_config_dword(pdev, PCIE_PL_IATU_VIEWPORT, val);

    /* 32 bit only, base address  */
    pci_write_config_dword(pdev, PCIE_PL_IATU_REGION_LOWER_BASE_ADDR, (u32)PCIE_EP_OUTBOUND_INTERNAL_BASE);
    pci_write_config_dword(pdev, PCIE_PL_IATU_REGION_UPPER_BASE_ADDR, 0);

    /* Region limit from phymem to phymem + memsize -1 */
    pci_write_config_dword(pdev, PCIE_PL_IATU_REGION_LIMIT, (u32)PCIE_EP_OUTBOUND_INTERNAL_BASE + PCIE_EP_OUTBOUND_MEMSIZE - 1);

    /* Mapped to 0x00000000 ~ 0x7FFFFFFF, total 2GB */
    pci_write_config_dword(pdev, PCIE_PL_IATU_REGION_LOWER_TARGET_ADDR, 0);

    pci_write_config_dword(pdev, PCIE_PL_IATU_REGION_UPPER_TARGET_ADDR, 0);
    
    pci_write_config_dword(pdev, PCIE_PL_IATU_REGION_CTRL1, 0);
    /* Outbound, address match */
    val = PCIE_PL_IATU_REGION_EN;
    pci_write_config_dword(pdev, PCIE_PL_IATU_REGION_CTRL2, val);

#ifdef IFX_EP_DBG
    ltq_pcie_iatu_dump(pdev, 0); /* Inbound */
    ltq_pcie_iatu_dump(pdev, 1); /* Outbound */
#endif /* #ifdef IFX_EP_DBG */
}

/**
 * \fn  static int ltq_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *id)
 *
 * \brief  This function initializes an adapter identified by a pci_dev structure.
 *         The OS initialization, configuring of the adapter private structure.
 *
 * \param  pdev     PCI device information struct
 * \param  id       entry in ltq_pcie_id_table
 * \return 0        OK
 * \return -ENODEV  Failed to initialize an adapter identified by pci_dev
 *
 * \ingroup IFX_PCIE_EP_VRX318_OS
 */
static int
ltq_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int i;
    int j;
    int k;
    unsigned long phymem;
    void __iomem *mem;
    size_t     memsize;
    static int cards_found = 0;
    struct ifx_pcie_controller *ctrl = pdev->bus->sysdata;
    int pcie_port = ctrl->port; /* RC core physical information */
    int nvec, err;
    ifx_pcie_ep_adapter_t *adapter;
    struct pci_bus *bus;
    int peer_num;
    u32 reg;

    bus = pdev->bus;
    
    if ((err = pci_enable_device(pdev))) {
        return err;
    }

    /* XXX 32-bit addressing only */
    if (pci_set_dma_mask(pdev, DMA_BIT_MASK(32))) {
        printk(KERN_ERR "%s: 32-bit DMA not available\n", __func__);
        goto bad;
    }

    pci_set_master(pdev);

    /* Physical address */
    phymem = pci_resource_start(pdev, 0); /* BAR zero*/
    memsize = pci_resource_len(pdev, 0);
    if (!request_mem_region(phymem, memsize, ltq_pcie_driver_name)) {
        printk(KERN_ERR "%s: cannot reserve PCI memory region\n", __func__);
        goto bad;
    }

    /* Virtual address */
    mem = ioremap_nocache(phymem, memsize);
    if (!mem) {
        printk(KERN_ERR "%s: cannot remap PCI memory region\n", __func__) ;
        goto bad1;
    }

    adapter = kmalloc(sizeof(ifx_pcie_ep_adapter_t), GFP_KERNEL);
    if (adapter == NULL) {
        goto err_mem;
    }

    pci_set_drvdata(pdev, adapter);
    
    err = pci_msi_max_vector_get(pdev, &nvec);
    if (err) {
        printk(KERN_ERR "%s: device doesn't support MSI, error code: %d", __func__, err);
        goto err_msi;
    }

    /* Overwrite maximum vector number according to the specific requirement */
    if (nvec > IFX_PCIE_EP_DEFAULT_MSI_VECTOR) {
        nvec = IFX_PCIE_EP_DEFAULT_MSI_VECTOR;
    }
    pci_msi_max_vector_set(pdev, nvec);
    
    err = pci_enable_msi(pdev);
    if (err) {
        printk(KERN_ERR "%s: Failed to enable MSI interrupts for the device, error code: %d\n", __func__, err);
        goto err_msi;
    }

    /* Enough information to configure address translation */
    ltq_pcie_iatu_setup(pdev);

    adapter->pdev = pdev;
    adapter->device_id = pdev->device;
    adapter->mem = mem;
    adapter->phy_mem = phymem;
    adapter->memsize = memsize;
    adapter->rc_phy_idx = pcie_port;
    adapter->msi_nvec = nvec;
    adapter->irq_base = pdev->irq;
    adapter->card_num = cards_found++; /* Logical index */
    /* EMI control stuff */
    reg = IFX_REG_R32(adapter->mem + PCIE_EP_IF_CLK);
    reg |= PCIE_EP_IF_CLK_NO_36MHZ_CLKOUT;
    IFX_REG_W32(reg, adapter->mem + PCIE_EP_IF_CLK);

#if 1
/* 
arcadyan: GPIO1 on VRX318, a unstable signal Clear 
1. set P0_ALTSEL1 Bit 1 to 0
2. set P0_PUDSEL and P0_PUDEN bit1 to 1
*/
    reg = IFX_REG_R32(adapter->mem + PCIE_EP_P0_ALTSEL1);
    reg &= ~PCIE_EP_P0_ALTSEL1_PIN1_SET;
    IFX_REG_W32(reg, adapter->mem + PCIE_EP_P0_ALTSEL1);

    reg = IFX_REG_R32(adapter->mem + PCIE_EP_P0_PUDSEL);
    reg |= PCIE_EP_P0_ALTSEL1_PIN1_SET;
    IFX_REG_W32(reg, adapter->mem + PCIE_EP_P0_PUDSEL);

    reg = IFX_REG_R32(adapter->mem + PCIE_EP_P0_PUDEN);
    reg |= PCIE_EP_P0_ALTSEL1_PIN1_SET;
    IFX_REG_W32(reg, adapter->mem + PCIE_EP_P0_PUDEN);
#else
    reg = IFX_REG_R32(adapter->mem + PCIE_EP_P0_ALTSEL1);
    reg &= ~PCIE_EP_P0_ALTSEL1_PIN1_SET;
    IFX_REG_W32(reg, adapter->mem + PCIE_EP_P0_ALTSEL1);
#endif

#if 1
/* 
arcadyan: GPIO12 on VRX318, a 72Mhz signal Clear 
1. set P0_ALTSEL1 Bit 12 to 0
2. set P0_PUDSEL and P0_PUDEN bit12 to 1
*/
    reg = IFX_REG_R32(adapter->mem + PCIE_EP_P0_ALTSEL1);
    //printk("\n @@ %s, PCIE_EP_P0_ALTSEL1(0x%x) in 0x%x", __FUNCTION__, adapter->mem + PCIE_EP_P0_ALTSEL1, reg);
    reg &= ~PCIE_EP_P0_ALTSEL1_BIT12_SET;
    IFX_REG_W32(reg, adapter->mem + PCIE_EP_P0_ALTSEL1);
    //read out
    reg = IFX_REG_R32(adapter->mem + PCIE_EP_P0_ALTSEL1);
    //printk("\n @@ %s, PCIE_EP_P0_ALTSEL1(0x%x) out 0x%x", __FUNCTION__,adapter->mem + PCIE_EP_P0_ALTSEL1, reg);

    reg = IFX_REG_R32(adapter->mem + PCIE_EP_P0_PUDSEL);
    //printk("\n @@ %s, IE_EP_P0_PUDSEL(0x%x) in 0x%x", __FUNCTION__,adapter->mem + PCIE_EP_P0_PUDSEL, reg);
    reg |= PCIE_EP_P0_PUDSEL_BIT12_SET;
    IFX_REG_W32(reg, adapter->mem + PCIE_EP_P0_PUDSEL);
    //read out
    reg = IFX_REG_R32(adapter->mem + PCIE_EP_P0_PUDSEL);
    //printk("\n @@ %s, IE_EP_P0_PUDSEL(0x%x) out 0x%x", __FUNCTION__,adapter->mem + PCIE_EP_P0_PUDSEL, reg);

    reg = IFX_REG_R32(adapter->mem + PCIE_EP_P0_PUDEN);
    //printk("\n @@ %s, PCIE_EP_P0_PUDEN(0x%x) in 0x%x", __FUNCTION__,adapter->mem + PCIE_EP_P0_PUDEN,  reg);
    reg |= PCIE_EP_P0_PUDEN_BIT12_SET;
    IFX_REG_W32(reg, adapter->mem + PCIE_EP_P0_PUDEN);
    //read out
    reg = IFX_REG_R32(adapter->mem + PCIE_EP_P0_PUDEN);
    //printk("\n @@ %s, PCIE_EP_P0_PUDEN(0x%x) out 0x%x", __FUNCTION__,adapter->mem + PCIE_EP_P0_PUDEN, reg);
#endif

    spin_lock(&ifx_pcie_ep_lock);
    g_pcie_ep_info.dev_num = cards_found; /* Existing cards */
    atomic_set(&g_pcie_ep_info.pcie_ep[adapter->card_num].refcnt, 0);
    g_pcie_ep_info.pcie_ep[adapter->card_num].card_idx = adapter->card_num;
    g_pcie_ep_info.pcie_ep[adapter->card_num].membase = adapter->mem;
    g_pcie_ep_info.pcie_ep[adapter->card_num].phy_membase = adapter->phy_mem;
    g_pcie_ep_info.pcie_ep[adapter->card_num].memsize = adapter->memsize;
    g_pcie_ep_info.pcie_ep[adapter->card_num].rc_idx = adapter->rc_phy_idx;
    g_pcie_ep_info.pcie_ep[adapter->card_num].irq_base = adapter->irq_base;
    g_pcie_ep_info.pcie_ep[adapter->card_num].irq_num = adapter->msi_nvec;

    /*
     * More cards supported, exchange address information
     * For example, suppose three cards dected.
     * 0, <1, 2> 
     * 1, <0, 2>
     * 2, <0, 1>
     * For four cards detected
     * 0, <1, 2, 3>
     * 1, <0, 2, 3>
     * 2, <0, 1, 3>
     * 3, <0, 1, 2>
     * and etc
     */
    if (cards_found > 1) {
        peer_num = cards_found - 1;
        for (i = 0; i < cards_found; i++) {
            j = 0;
            k = 0;
            g_pcie_ep_info.pcie_ep[i].peer_num = peer_num;
            do {
                if (j == i) {
                    j++;
                    continue;
                }
                g_pcie_ep_info.pcie_ep[i].peer_membase[k] = g_pcie_ep_info.pcie_ep[j].membase;
                g_pcie_ep_info.pcie_ep[i].peer_phy_membase[k] = g_pcie_ep_info.pcie_ep[j].phy_membase;
                g_pcie_ep_info.pcie_ep[i].peer_memsize[k] = g_pcie_ep_info.pcie_ep[j].memsize;
                k++;
                j++;
            } while ((k < peer_num) && (j < cards_found));
        }
    }
    spin_unlock(&ifx_pcie_ep_lock);
    
#ifdef IFX_EP_DBG
    printk("Total cards found %d\n", cards_found);
    /* Dump detailed debug information */
    for (i = 0; i < cards_found; i++) {
        printk("card %d attached RC %d\n", g_pcie_ep_info.pcie_ep[i].card_idx, g_pcie_ep_info.pcie_ep[i].rc_idx);
        printk("irq base %d irq numbers %d\n", g_pcie_ep_info.pcie_ep[i].irq_base, g_pcie_ep_info.pcie_ep[i].irq_num);
        printk("its own phy membase  0x%08x virtual membase 0x%p size 0x%08x\n",
            g_pcie_ep_info.pcie_ep[i].phy_membase, g_pcie_ep_info.pcie_ep[i].membase, g_pcie_ep_info.pcie_ep[i].memsize);
        if (cards_found > 1) {
            for (j = 0; j < g_pcie_ep_info.pcie_ep[i].peer_num; j++)
            printk("its peer phy membase  0x%08x virtual membase 0x%p size 0x%08x\n",
                g_pcie_ep_info.pcie_ep[i].peer_phy_membase[j], g_pcie_ep_info.pcie_ep[i].peer_membase[j],
                g_pcie_ep_info.pcie_ep[i].peer_memsize[j]);
        }
    }
#endif /* IFX_EP_DBG */
    return 0;
err_msi:
    kfree(adapter);
err_mem:
    iounmap(mem);
bad1:
    release_mem_region(phymem, memsize);
bad:
    pci_disable_device(pdev);
    return (-ENODEV);
}

/**
 * \fn  static void ltq_pcie_remove(struct pci_dev *pdev)
 *
 * \brief  This function is called by the PCI subsystem to alert the driver
 *         that it should release a PCI device because the driver is going 
 *         to be removed from memory.
 *
 * \param  pdev     PCI device information struct
 * \return none
 *
 * \ingroup IFX_PCIE_EP_VRX318_OS
 */ 
static void
ltq_pcie_remove(struct pci_dev *pdev)
{
    ifx_pcie_ep_adapter_t *adapter = (ifx_pcie_ep_adapter_t *)pci_get_drvdata(pdev);

    if (atomic_read(&g_pcie_ep_info.pcie_ep[adapter->card_num].refcnt) != 0 ) {
        printk(KERN_ERR "%s still being used, can't remove\n", __func__);
    }
    
    iounmap(adapter->mem);
    release_mem_region(adapter->phy_mem, adapter->memsize);
    pci_disable_msi(pdev);
    pci_disable_device(pdev);
    kfree(adapter);
    adapter = NULL;
}

MODULE_DEVICE_TABLE(pci, ltq_pcie_id_table);

static struct pci_driver ltq_pcie_driver = {
    .name       = (char *)ltq_pcie_driver_name,
    .id_table   = ltq_pcie_id_table,
    .probe      = ltq_pcie_probe,
    .remove     = ltq_pcie_remove,
    /* PM not supported */
    /* AER is controlled by RC */
};

/**
 * \fn  static int __init init_ltq_pcie(void)
 *
 * \brief  This function registered PCIe EP device driver with OS PCI subsystem 
 *         and initializes PCIe EP address mapping driver.
 *
 * \return 0 on success
 * \return -ENODEV No related PCIe EP found
 *
 * \ingroup IFX_PCIE_EP_VRX318_OS
 */ 
static int __init
init_ltq_pcie(void)
{
    char ver_str[256] = {0};

    memset(&g_pcie_ep_info, 0, sizeof (ifx_pcie_ep_info_t));
    
    if (pci_register_driver(&ltq_pcie_driver) < 0) {
        printk("%s: No devices found, driver not installed.\n", __func__);
        return (-ENODEV);
    }
    ifx_drv_ver(ver_str, "PCIe EP ", IFX_PCIE_EP_VER_MAJOR, IFX_PCIE_EP_VER_MID, IFX_PCIE_EP_VER_MINOR);
    printk(KERN_INFO "%s", ver_str);    
    return (0);
}
module_init(init_ltq_pcie);

/**
 * \fn  static void __exit exit_ltq_pcie(void)
 *
 * \brief  This function unregister PCIe EP device driver with OS PCI subsystem
 *
 * \return none
 *
 * \ingroup IFX_PCIE_EP_VRX318_OS
 */ 
static void __exit
exit_ltq_pcie(void)
{
    pci_unregister_driver(&ltq_pcie_driver);

    printk(KERN_INFO "%s: %s driver unloaded\n", __func__, ltq_pcie_driver_name);
}
module_exit(exit_ltq_pcie);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chuanhua.Lei@lantiq.com");
MODULE_SUPPORTED_DEVICE("Lantiq SmartPHY PCIe EP");
MODULE_DESCRIPTION("Lantiq SmartPHY PCIe EP address mapping driver");

