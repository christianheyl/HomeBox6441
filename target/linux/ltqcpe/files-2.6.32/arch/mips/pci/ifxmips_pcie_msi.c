/******************************************************************************
**
** FILE NAME    : ifxmips_pcie_msi.c
** PROJECT      : IFX UEIP for VRX200
** MODULES      : PCI MSI sub module
**
** DATE         : 02 Mar 2009
** AUTHOR       : Lei Chuanhua
** DESCRIPTION  : PCIe MSI Driver
** COPYRIGHT    :       Copyright (c) 2009
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
** HISTORY
** $Date        $Author         $Comment
** 02 Mar,2009  Lei Chuanhua    Initial version
*******************************************************************************/
/*!
 \defgroup IFX_PCIE_MSI MSI OS APIs
 \ingroup IFX_PCIE
 \brief PCIe bus driver OS interface functions
*/

/*!
 \file ifxmips_pcie_msi.c
 \ingroup IFX_PCIE 
 \brief PCIe MSI OS interface file
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/msi.h>

#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>
#include <asm/ifx/irq.h>

#include "ifxmips_pcie_reg.h"
#include "ifxmips_pcie.h"

#define PCIE_MSI_MAX_IRQ_NUM     4

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,39)
#define set_irq_msi(irq, desc) irq_set_msi_desc((irq), (desc))
#endif

/* Keep base data lower bits as zero since MSI has maximum 32 vectors */
#define IFX_PCIE_MSI_BASE_DATA  0x4AE0

void pcie_msi_pic_init(int pcie_port)
{
    int rc_port = pcie_port_idx_to_rc_port(pcie_port);

    spin_lock_init(&g_pcie_port_defs[pcie_port].msi_irqs.msi_lock);

    spin_lock(&g_pcie_port_defs[pcie_port].msi_irqs.msi_lock);
#ifdef CONFIG_IFX_PCIE_1ST_CORE
    if (rc_port == IFX_PCIE_PORT0) {
#ifdef CONFIG_IFX_PCIE_INBOUND_NO_HW_SWAP
        g_pcie_port_defs[pcie_port].msi_irqs.msi_pic_p->pic_endian = IFX_MSI_PIC_LITTLE_ENDIAN;
#else
        g_pcie_port_defs[pcie_port].msi_irqs.msi_pic_p->pic_endian = IFX_MSI_PIC_BIG_ENDIAN;
#endif /* CONFIG_IFX_PCIE_INBOUND_NO_HW_SWAP */
    }
#endif /* CONFIG_IFX_PCIE_1ST_CORE */

#ifdef CONFIG_IFX_PCIE_2ND_CORE
    if (rc_port == IFX_PCIE_PORT1) {
#ifdef CONFIG_IFX_PCIE1_INBOUND_NO_HW_SWAP
        g_pcie_port_defs[pcie_port].msi_irqs.msi_pic_p->pic_endian = IFX_MSI_PIC_LITTLE_ENDIAN;
#else
        g_pcie_port_defs[pcie_port].msi_irqs.msi_pic_p->pic_endian = IFX_MSI_PIC_BIG_ENDIAN;
#endif /* CONFIG_IFX_PCIE1_INBOUND_NO_HW_SWAP */
    }
#endif /* CONFIG_IFX_PCIE_2ND_CORE */

#ifdef CONFIG_IFX_PCIE_3RD_CORE
    if (rc_port == IFX_PCIE_PORT2) {
#ifdef CONFIG_IFX_PCIE2_INBOUND_NO_HW_SWAP
        g_pcie_port_defs[pcie_port].msi_irqs.msi_pic_p->pic_endian = IFX_MSI_PIC_LITTLE_ENDIAN;
#else
        g_pcie_port_defs[pcie_port].msi_irqs.msi_pic_p->pic_endian = IFX_MSI_PIC_BIG_ENDIAN;
#endif /* CONFIG_IFX_PCIE2_INBOUND_NO_HW_SWAP */
    }
#endif /* CONFIG_IFX_PCIE_3RD_CORE */
    else {
        
    }
    spin_unlock(&g_pcie_port_defs[pcie_port].msi_irqs.msi_lock);
}

/** 
 * \fn int arch_setup_msi_irq(struct pci_dev *pdev, struct msi_desc *desc)
 * \brief Called when a driver request MSI interrupts instead of the 
 * legacy INT A-D. This routine will allocate multiple interrupts 
 * for MSI devices that support them. A device can override this by 
 * programming the MSI control bits [6:4] before calling 
 * pci_enable_msi(). 
 * 
 * \param[in] pdev   Device requesting MSI interrupts 
 * \param[in] desc   MSI descriptor 
 * 
 * \return   -EINVAL Invalid pcie root port or invalid msi bit
 * \return    0        OK
 * \ingroup IFX_PCIE_MSI
 */
int 
arch_setup_msi_irq(struct pci_dev *pdev, struct msi_desc *desc)
{
    int  i;
    int  irq_base;
    int  pos;
    u16  control;
    int  irq_idx;
    int  irq_step;
    int configured_private_bits;
    int request_private_bits;
    struct msi_msg msg;
    u16 search_mask;
    u16 msi_base_data;
    struct ifx_pcie_controller *ctrl = pdev->bus->sysdata;
    int pcie_port;
    int rc_port = ctrl->port;

    pcie_rc_port_to_pcie_port(rc_port, &pcie_port);
    IFX_PCIE_PRINT(PCIE_MSG_MSI, "%s %s port %d enter\n", __func__, pci_name(pdev), rc_port);

    /* Skip RC and switch ports since we have limited interrupt resource available */
    if (pdev->pcie_type != PCI_EXP_TYPE_ENDPOINT) {
        IFX_PCIE_PRINT(PCIE_MSG_MSI, "%s RC %d or Switch Port doesn't use MSI interrupt\n", __func__, rc_port);
        return -EINVAL;
    }

    /*
     * Read the MSI config to figure out how many IRQs this device 
     * wants.  Most devices only want 1, which will give 
     * configured_private_bits and request_private_bits equal 0. 
     */
    pci_read_config_word(pdev, desc->msi_attrib.pos + PCI_MSI_FLAGS, &control);

    /*
     * If the number of private bits has been configured then use 
     * that value instead of the requested number. This gives the 
     * driver the chance to override the number of interrupts 
     * before calling pci_enable_msi(). 
     */
    configured_private_bits = (control & PCI_MSI_FLAGS_QSIZE) >> 4;
    
    if (configured_private_bits == 0) {
        /* Nothing is configured, so use the hardware requested size */
        request_private_bits = (control & PCI_MSI_FLAGS_QMASK) >> 1;
    }
    else {
        /*
         * Use the number of configured bits, assuming the 
         * driver wanted to override the hardware request 
         * value.
         */
        request_private_bits = configured_private_bits;
    }

    /*
     * The PCI 2.3 spec mandates that there are at most 32
     * interrupts. If this device asks for more, only give it one.
     */
    if (request_private_bits > 5) {
        request_private_bits = 0;
    }
again:
    /*
     * The IRQs have to be aligned on a power of two based on the
     * number being requested.
     */
    irq_step = (1 << request_private_bits);

    /* Mask with one bit for each IRQ */
    search_mask = (1 << irq_step) - 1;
    
    /*
     * We're going to search msi_free_irq_bitmask_lock for zero 
     * bits. This represents an MSI interrupt number that isn't in 
     * use.
     */
    spin_lock(&g_pcie_port_defs[pcie_port].msi_irqs.msi_lock);
    for (pos = 0; pos < IFX_MSI_IRQ_NUM; pos += irq_step) {
        if ((g_pcie_port_defs[pcie_port].msi_irqs.msi_free_irq_bitmask & (search_mask << pos)) == 0) {
            g_pcie_port_defs[pcie_port].msi_irqs.msi_free_irq_bitmask |= search_mask << pos; 
            g_pcie_port_defs[pcie_port].msi_irqs.msi_multiple_irq_bitmask |= (search_mask >> 1) << pos;
            break; 
        }
    }
    spin_unlock(&g_pcie_port_defs[pcie_port].msi_irqs.msi_lock); 

    /* Make sure the search for available interrupts didn't fail */ 
    if (pos >= IFX_MSI_IRQ_NUM) {
        if (request_private_bits) {
            IFX_PCIE_PRINT(PCIE_MSG_MSI, "%s: Unable to find %d free "
                  "interrupts, trying just one", __func__, 1 << request_private_bits);
            request_private_bits = 0;
            goto again;
        }
        else {
            printk(KERN_ERR "%s: Unable to find a free MSI interrupt\n", __func__);
            return -EINVAL;
        }
    }
    
    /* Only assign the base irq to msi entry */
    irq_base = g_pcie_port_defs[pcie_port].msi_irqs.msi_irq_idx[pos].irq;
    irq_idx = g_pcie_port_defs[pcie_port].msi_irqs.msi_irq_idx[pos].idx;

    IFX_PCIE_PRINT(PCIE_MSG_MSI, "pos %d, irq %d irq_idx %d\n", pos, irq_base, irq_idx);

    /*
     * Initialize MSI. This has to match the memory-write endianess from the device 
     * Address bits [23:12]
     * For multiple MSI, we have to assign and enable sequence MSI data
     * Make sure that base data lower bits as zero since multiple MSI just modify lower
     * several bits to generate different interrupts
     */
    msi_base_data = ((1 << pos) + IFX_PCIE_MSI_BASE_DATA) & (~search_mask);
    spin_lock(&g_pcie_port_defs[pcie_port].msi_irqs.msi_lock); 
    for (i = 0; i < irq_step; i++) {
        g_pcie_port_defs[pcie_port].msi_irqs.msi_pic_p->pic_table[pos + i] = SM((irq_idx + i) % PCIE_MSI_MAX_IRQ_NUM, IFX_MSI_PIC_INT_LINE) |
                        SM((g_pcie_port_defs[pcie_port].msi_irqs.msi_phy_base >> 12), IFX_MSI_PIC_MSG_ADDR) |
                        SM((msi_base_data + i) , IFX_MSI_PIC_MSG_DATA);

        /* Enable this entry */
        g_pcie_port_defs[pcie_port].msi_irqs.msi_pic_p->pic_table[pos + i] &= ~IFX_MSI_PCI_INT_DISABLE;
        IFX_PCIE_PRINT(PCIE_MSG_MSI, "pic_table[%d]: 0x%08x\n",
            (pos + i), g_pcie_port_defs[pcie_port].msi_irqs.msi_pic_p->pic_table[pos + i]);

    }
    spin_unlock(&g_pcie_port_defs[pcie_port].msi_irqs.msi_lock);
    
    /* XXX, Don't update this field which involves kernel patch */
    //desc->msi_attrib.multiple = request_private_bits;

    /* Assign base irq and base data to the first MSI entry */
    set_irq_msi(irq_base, desc);
    msg.address_hi = 0x0;
    msg.address_lo = g_pcie_port_defs[pcie_port].msi_irqs.msi_phy_base;
    msg.data = SM(msi_base_data, IFX_MSI_PIC_MSG_DATA);
    IFX_PCIE_PRINT(PCIE_MSG_MSI, "base msi_data: pos %d data 0x%08x irq %d-%d\n",
        pos, msg.data, irq_base, irq_base + irq_step - 1);

    write_msi_msg(irq_base, &msg);

    /* Update the number of IRQs the device has available to it */
    control &= ~PCI_MSI_FLAGS_QSIZE;
    control |= (request_private_bits << 4);
    pci_write_config_word(pdev, desc->msi_attrib.pos + PCI_MSI_FLAGS, control);
    
    IFX_PCIE_PRINT(PCIE_MSG_MSI, "%s port %d exit\n", __func__, rc_port);
    return 0;
}

static int
pcie_msi_irq_to_port(unsigned int irq, int *port)
{
    int ret = 0;
#ifdef CONFIG_IFX_PCIE_1ST_CORE
    if (irq == IFX_PCIE_MSI_IR0 || irq == IFX_PCIE_MSI_IR1 ||
        irq == IFX_PCIE_MSI_IR2 || irq == IFX_PCIE_MSI_IR3) {
        *port = IFX_PCIE_PORT0;
        goto done;
    }
#endif /* CONFIG_IFX_PCIE_1ST_CORE */
#ifdef CONFIG_IFX_PCIE_2ND_CORE
    if (irq == IFX_PCIE1_MSI_IR0 || irq == IFX_PCIE1_MSI_IR1 ||
        irq == IFX_PCIE1_MSI_IR2 || irq == IFX_PCIE1_MSI_IR3) {
        *port = IFX_PCIE_PORT1;
        goto done;
    }
#endif /* CONFIG_IFX_PCIE_2ND_CORE */

#ifdef CONFIG_IFX_PCIE_3RD_CORE
    if (irq == IFX_PCIE2_MSI_IR0 || irq == IFX_PCIE2_MSI_IR1 ||
        irq == IFX_PCIE2_MSI_IR2 || irq == IFX_PCIE2_MSI_IR3) {
        *port = IFX_PCIE_PORT2;
        goto done;
    }
#endif /* CONFIG_IFX_PCIE_3RD_CORE */
    else {
        printk(KERN_ERR "%s: Attempted to teardown illegal " 
            "MSI interrupt (%d)\n", __func__, irq);
        ret = -EINVAL;
    }
done:
    return ret;
}

/** 
 * \fn void arch_teardown_msi_irq(unsigned int irq)
 * \brief Called when a device no longer needs its MSI interrupts. All 
 * MSI interrupts for the device are freed. 
 * 
 * \param irq   The devices first irq number. There may be multple in sequence.
 * \return none
 * \ingroup IFX_PCIE_MSI
 */
void 
arch_teardown_msi_irq(unsigned int irq)
{
    int i;
    int pos;
    int number_irqs; 
    u16 bitmask;
    int pcie_port;
    int rc_port = IFX_PCIE_PORT0;

    IFX_PCIE_PRINT(PCIE_MSG_MSI, "%s enter\n", __func__);

    BUG_ON(irq > INT_NUM_IM4_IRL31);

    if (pcie_msi_irq_to_port(irq, &rc_port) != 0) {
        return;
    }

    pcie_rc_port_to_pcie_port(rc_port, &pcie_port);

    /* Shift the mask to the correct bit location, not always correct 
     * Probally, the first match will be chosen.
     */
    for (pos = 0; pos < IFX_MSI_IRQ_NUM; pos++) {
        if ((g_pcie_port_defs[pcie_port].msi_irqs.msi_irq_idx[pos].irq == irq) 
            && (g_pcie_port_defs[pcie_port].msi_irqs.msi_free_irq_bitmask & ( 1 << pos))) {
            break;
        }
    }
    
    if (pos >= IFX_MSI_IRQ_NUM) {
        printk(KERN_ERR "%s: Unable to find a matched MSI interrupt\n", __func__);
        return;
    }
    /*
     * Count the number of IRQs we need to free by looking at the
     * msi_multiple_irq_bitmask. Each bit set means that the next
     * IRQ is also owned by this device.
     */ 
    number_irqs = 0;
    while (((pos + number_irqs) < IFX_MSI_IRQ_NUM) && 
        (g_pcie_port_defs[pcie_port].msi_irqs.msi_multiple_irq_bitmask & (1 << (pos + number_irqs)))) {
        number_irqs++;
    }
    number_irqs++;

    /* Disable entries if multiple MSI  */
    spin_lock(&g_pcie_port_defs[pcie_port].msi_irqs.msi_lock);
    for (i = 0; i < number_irqs; i++) {
        g_pcie_port_defs[pcie_port].msi_irqs.msi_pic_p->pic_table[pos + i] |= IFX_MSI_PCI_INT_DISABLE;
        g_pcie_port_defs[pcie_port].msi_irqs.msi_pic_p->pic_table[pos + i] &= ~(IFX_MSI_PIC_INT_LINE | IFX_MSI_PIC_MSG_ADDR | IFX_MSI_PIC_MSG_DATA);
    }
    spin_unlock(&g_pcie_port_defs[pcie_port].msi_irqs.msi_lock);
    
    /* Mask with one bit for each IRQ */
    bitmask = (1 << number_irqs) - 1;

    bitmask <<= pos;
    if ((g_pcie_port_defs[pcie_port].msi_irqs.msi_free_irq_bitmask & bitmask) != bitmask) {
        printk(KERN_ERR "%s: Attempted to teardown MSI "
             "interrupt (%d) not in use\n", __func__, irq);
        return;
    }
    
    /* Checks are done, update the in use bitmask */
    spin_lock(&g_pcie_port_defs[pcie_port].msi_irqs.msi_lock);
    g_pcie_port_defs[pcie_port].msi_irqs.msi_free_irq_bitmask &= ~bitmask;
    g_pcie_port_defs[pcie_port].msi_irqs.msi_multiple_irq_bitmask &= ~(bitmask >> 1);
    spin_unlock(&g_pcie_port_defs[pcie_port].msi_irqs.msi_lock);

    IFX_PCIE_PRINT(PCIE_MSG_MSI, "%s port %d exit\n", __func__, rc_port);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chuanhua.Lei@infineon.com");
MODULE_SUPPORTED_DEVICE("Infineon PCIe IP builtin MSI PIC module");
MODULE_DESCRIPTION("Infineon PCIe IP builtin MSI PIC driver");

