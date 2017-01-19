/****************************************************************************
                              Copyright (c) 2011
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************/
#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <asm/types.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

/* Project header file */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>
#include <asm/ifx/ifx_pcie.h>
#include "ifxmips_pcie_ep_vrx318_test.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
#define MODULE_PARM(a, b)         module_param(a, int, 0)
#endif

#define REG32(addr)         (*((volatile u32*)(addr)))

static int test_module = PPE_TEST;
static int dma_data_length = 1024;
static int dma_mode = 0;
static int dma_burst = 8;
static int desc_num = 32;
static int tx_byte_offset = 0;
static int rx_byte_offset = 0;
static int byte_enabled = 1;

static ifx_pcie_ep_dev_t pcie_dev[IFX_PCIE_EP_MAX_PEER + 1] = {{0}, {0}};
static int ppe_irq_num = 0;

MODULE_PARM(test_module, "i");
MODULE_PARM_DESC(test_module, "0 -- PPE, 1 -- CDMA");

MODULE_PARM(dma_data_length, "i");
MODULE_PARM_DESC(dma_data_length, "Single packet length");

MODULE_PARM(dma_mode,"i");
MODULE_PARM_DESC(dma_mode, "mode 0 -- Soc->EP, mode 1-- EP->SoC");

MODULE_PARM(dma_burst,"i");
MODULE_PARM_DESC(dma_burst, "dma burst 2, 4, 8");

MODULE_PARM(desc_num,"i");
MODULE_PARM_DESC(desc_num, "desc number 8, 16, 32");

MODULE_PARM(tx_byte_offset,"i");
MODULE_PARM_DESC(tx_byte_offset, "DMA tx byte offset 1, 2, 3");

MODULE_PARM(rx_byte_offset,"i");
MODULE_PARM_DESC(rx_byte_offset, "DMA rx byte offset 1, 2, 3");

MODULE_PARM(byte_enabled,"i");
MODULE_PARM_DESC(byte_enabled, "DMA byte enabled or not");

static irqreturn_t
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
ifx_pcie_ep_ppe_intr(int irq, void *dev_id)
#else
ifx_pcie_ep_ppe_intr(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
    ifx_pcie_ep_dev_t *dev = dev_id;
    u32 membase = (u32)(dev->membase);
    ppe_irq_num++;
    if (IFX_REG_R32(PPE_MBOX_IGU0_ISR(membase)) == 0) {
        printk("Fatal error, dummy interrupt\n");
    }
    IFX_REG_W32(PPE_MBOX_TEST_BIT, PPE_MBOX_IGU0_ISRC(membase));
    return IRQ_HANDLED;
}

static void ppe_mbox_reg_dump(u32 membase)
{
    printk("PPE_MBOX_IGU0_ISRS addr 0x%08x data 0x%08x\n", PPE_MBOX_IGU0_ISRS(membase), IFX_REG_R32(PPE_MBOX_IGU0_ISRS(membase)));
    printk("PPE_MBOX_IGU0_ISRC addr 0x%08x data 0x%08x\n", PPE_MBOX_IGU0_ISRC(membase), IFX_REG_R32(PPE_MBOX_IGU0_ISRC(membase)));
    printk("PPE_MBOX_IGU0_ISR  addr 0x%08x data 0x%08x\n", PPE_MBOX_IGU0_ISR(membase), IFX_REG_R32(PPE_MBOX_IGU0_ISR(membase)));
    printk("PPE_MBOX_IGU0_IER  addr 0x%08x data 0x%08x\n", PPE_MBOX_IGU0_IER(membase), IFX_REG_R32(PPE_MBOX_IGU0_IER(membase)));
    printk("PPE_MBOX_IGU1_ISRS addr 0x%08x data 0x%08x\n", PPE_MBOX_IGU1_ISRS(membase), IFX_REG_R32(PPE_MBOX_IGU1_ISRS(membase)));
    printk("PPE_MBOX_IGU1_ISRC addr 0x%08x data 0x%08x\n", PPE_MBOX_IGU1_ISRC(membase), IFX_REG_R32(PPE_MBOX_IGU1_ISRC(membase)));
    printk("PPE_MBOX_IGU1_ISR  addr 0x%08x data 0x%08x\n", PPE_MBOX_IGU1_ISR(membase), IFX_REG_R32(PPE_MBOX_IGU1_ISR(membase)));                    
    printk("PPE_MBOX_IGU1_IER  addr 0x%08x data 0x%08x\n", PPE_MBOX_IGU1_IER(membase), IFX_REG_R32(PPE_MBOX_IGU1_IER(membase)));
    printk("PPE_MBOX_IGU2_ISRS addr 0x%08x data 0x%08x\n", PPE_MBOX_IGU2_ISRS(membase), IFX_REG_R32(PPE_MBOX_IGU2_ISRS(membase)));
    printk("PPE_MBOX_IGU2_ISRC addr 0x%08x data 0x%08x\n", PPE_MBOX_IGU2_ISRC(membase), IFX_REG_R32(PPE_MBOX_IGU2_ISRC(membase)));
    printk("PPE_MBOX_IGU2_ISR  addr 0x%08x data 0x%08x\n", PPE_MBOX_IGU2_ISR(membase), IFX_REG_R32(PPE_MBOX_IGU2_ISR(membase)));
    printk("PPE_MBOX_IGU2_IER  addr 0x%08x data 0x%08x\n", PPE_MBOX_IGU2_IER(membase), IFX_REG_R32(PPE_MBOX_IGU2_IER(membase)));
}

#define PPE_INT_TIMEOUT 100
static int ppe_mbox_int_stress_test(ifx_pcie_ep_dev_t *dev)
{
    int i;
    int j;
    int ret;
    u32 membase = (u32)(dev->membase);
    
    /* Clear it first */
    IFX_REG_W32(PPE_MBOX_TEST_BIT, PPE_MBOX_IGU0_ISRC(membase));

    ret = request_irq(dev->irq, ifx_pcie_ep_ppe_intr, IRQF_DISABLED, "PPE_MSI", dev);
    if (ret) {
        printk(KERN_ERR "%s request irq %d failed\n", __func__, dev->irq);
        return -1;
    }
    printk("PPE test\n");
    IFX_REG_W32(PPE_MBOX_TEST_BIT, PPE_MBOX_IGU0_IER(membase));
    ppe_irq_num = 0;
    /* Purposely trigger interrupt */
    for (i = 0; i < PPE_MBOX_IRQ_TEST_NUM; i++) {
        j = 0;
        while((IFX_REG_R32(PPE_MBOX_IGU0_ISR(membase)) & PPE_MBOX_TEST_BIT)) {
            udelay(10);
            j++;
            if (j > PPE_INT_TIMEOUT) {
                break;
            }
        }
        IFX_REG_W32(PPE_MBOX_TEST_BIT, PPE_MBOX_IGU0_ISRS(membase));
    }
    udelay(100);
    printk("irq triggered %d expected %d\n", ppe_irq_num, PPE_MBOX_IRQ_TEST_NUM);
    ppe_mbox_reg_dump(membase);
    ppe_irq_num = 0;
    return 0;
}

static void icu_im_enable(u32 membase, int module)
{
    u32 reg;
    
    reg = REG32(ICU_IM_ER(membase));

    reg |= (1 << module);
    
    REG32(ICU_IM_ER(membase)) = reg;
}

static void cdma_module_reset (u32 membase)
{
    REG32(PMU_PWDCR(membase)) &= ~CGU_DMA_CLK_EN;
    printk("PMU_SR addr 0x%08x data 0x%08x\n", (u32)PMU_SR(membase), REG32(PMU_SR(membase)));
    /* Enable/disable the DMA*/
    REG32(RCU_RST_REQ(membase)) |= (0x00000200) ;  /*DMA(9) */
    udelay(10);
    REG32(CDMA_CTRL(membase)) |= (1);  /*Reset DMA module */
    udelay(10);
    REG32(CDMA_CLC(membase)) = 0x00000000;
    printk("CDMA_CLC addr 0x%08x data 0x%08x\n", (u32)CDMA_CLC(membase), REG32(CDMA_CLC(membase)));

    /* Enable central DMA interrupts */
    icu_im_enable(membase, CDMA_CH0);
    icu_im_enable(membase, CDMA_CH1);
    printk("Reset DMA module done\n");
}

static void cdma_flush_memcopy_buf (u32 membase)
{
    REG32(CDMA_PS(membase)) = CDMA_MEMCOPY_PORT;
    REG32(CDMA_PCTRL(membase)) |= 0x10000; 
    udelay(2);
    REG32(CDMA_PCTRL(membase)) &= ~(0x10000); 
}

static void reset_cdma_channel(u32 membase, int channel)
{
    /*reset all DMA channel to PPE Switch*/
    REG32(CDMA_CS(membase)) = channel;
    REG32(CDMA_CCTRL(membase)) = 0x2; 
    while ( REG32(CDMA_CCTRL(membase)) & 0x01 ) { 
        udelay(10);
        printk("Reset DMA channel not done\n");
    } 
}   

static void cdma_memory_port_cfg(u32 membase, int burstlen)
{
    REG32(CDMA_PS(membase)) = CDMA_MEMCOPY_PORT;
    REG32(CDMA_PCTRL(membase)) &= ~0xf3F;  
    
    if (burstlen == 2 ) { 
        REG32(CDMA_PCTRL(membase)) |= 0x14;  
    } 
    else if (burstlen == 4 ) { 
        REG32(CDMA_PCTRL(membase)) |= 0x28;  
    } 
    else if (burstlen == 8) {
        REG32(CDMA_PCTRL(membase)) |= 0x3c; 
    }
}  

static void cdma_byte_enable_cfg(u32 membase, int enable)
{
    if (enable) {
        REG32(CDMA_CTRL(membase)) |= (1 << 9); /* Default one */
    }
    else {
        REG32(CDMA_CTRL(membase)) &= ~(1 << 9); /* Disable byte enable bit */
    }
}

static void cdma_memory_copy_init(u32 membase)
{
    cdma_module_reset(membase);
    reset_cdma_channel(membase, CDMA_MEMCOPY_TX_CHAN); /* TX */
    reset_cdma_channel(membase, CDMA_MEMCOPY_RX_CHAN); /* RX */
    cdma_flush_memcopy_buf(membase);
}

static void cdma_tx_ch_cfg (u32 membase, int dir, int ch_num, u32 desc_ptr_base, u32 data_ptr_base, int desc_num) 
{
    unsigned int i;
    cdma_tx_descriptor_t *tx_desc;

    for (i = 0; i < desc_num; i++) {
        tx_desc = (cdma_tx_descriptor_t *)(desc_ptr_base + (i * sizeof(cdma_tx_descriptor_t)));
        /* Trick !!! */
        tx_desc->status.word = 0;
        tx_desc->status.field.OWN = 1;
        tx_desc->status.field.C = 0;
        tx_desc->status.field.Sop = 1;
        tx_desc->status.field.Eop = 1;
        tx_desc->status.field.Byteoffset = tx_byte_offset;
        tx_desc->status.field.DataLen = dma_data_length;
        if (dir == SOC_TO_EP) { /* src is SoC, dst is VRX218 */
            tx_desc->DataPtr = CPHYSADDR((u32)(data_ptr_base + ( i * dma_data_length ))) + PCIE_EP_OUTBOUND_INTERNAL_BASE;
        }
        else {
            tx_desc->DataPtr = VRX218_ADDR(CPHYSADDR((u32)(data_ptr_base + ( i * dma_data_length ))));
        }
        printk("Tx desc num %d word 0x%08x data pointer 0x%08x\n",
            i, tx_desc->status.word, tx_desc->DataPtr);
    }
    
    REG32(CDMA_CS(membase)) = ch_num;
    REG32(CDMA_CDBA(membase)) = VRX218_ADDR(CPHYSADDR(desc_ptr_base));
    REG32(CDMA_CDLEN(membase)) = desc_num;
    REG32(CDMA_CIE(membase)) = 0;
    REG32(CDMA_CPOLL(membase)) = 0x80000020;
    REG32(CDMA_CCTRL(membase)) |= (0x1 << 8); /* TX DIR */
}

static void cdma_rx_ch_cfg (u32 membase, int dir, int ch, u32 desc_ptr_base, unsigned int data_ptr_base, int desc_num) 
{

    unsigned int i;
    cdma_rx_descriptor_t *rx_desc;
    
    for(i = 0; i < desc_num; i++) {
        /* Trick !!! */
        rx_desc = (cdma_rx_descriptor_t *)(desc_ptr_base + (i * sizeof(cdma_rx_descriptor_t)));
        rx_desc->status.word = 0; 
        rx_desc->status.field.OWN = 1;
        rx_desc->status.field.Sop = 1;
        rx_desc->status.field.Eop = 1;
        rx_desc->status.field.Byteoffset = rx_byte_offset;
        rx_desc->status.field.DataLen = roundup(dma_data_length, dma_burst << 2);
        if (dir == SOC_TO_EP) { /* src is VRX218, dst is SoC */
            rx_desc->DataPtr = VRX218_ADDR(CPHYSADDR((u32)(data_ptr_base + (i * roundup(dma_data_length, dma_burst << 2)))));
        }
        else {
            rx_desc->DataPtr = CPHYSADDR((u32)(data_ptr_base + (i * roundup(dma_data_length, dma_burst << 2)))) + PCIE_EP_OUTBOUND_INTERNAL_BASE;
        }
         printk("Rx desc num %d word 0x%08x data pointer 0x%08x\n",
            i, rx_desc->status.word, rx_desc->DataPtr);       
    }
    REG32(CDMA_CS(membase)) = ch;
    REG32(CDMA_CDBA(membase)) = VRX218_ADDR(CPHYSADDR(desc_ptr_base));
    REG32(CDMA_CDLEN(membase)) = desc_num;
    REG32(CDMA_CIE(membase)) = 0;
    REG32(CDMA_CPOLL(membase)) = 0x80000020;
    REG32(CDMA_CCTRL(membase)) &= ~(0x1 << 8); /* RX DIR */
    return;
}

static void cdma_reg_dump(u32 membase)
{
    printk("CDMA_CLC   addr 0x%08x data 0x%08x\n", (u32)CDMA_CLC(membase), REG32(CDMA_CLC(membase)));
    printk("CDMA_ID    addr 0x%08x data 0x%08x\n", (u32)CDMA_ID(membase), REG32(CDMA_ID(membase)));
    printk("CDMA_CTRL  addr 0x%08x data 0x%08x\n", (u32)CDMA_CTRL(membase), REG32(CDMA_CTRL(membase)));
    printk("CDMA_CPOLL addr 0x%08x data 0x%08x\n", (u32)CDMA_CPOLL(membase), REG32(CDMA_CPOLL(membase)));
    printk("CDMA_CS    addr 0x%08x data 0x%08x\n", (u32)CDMA_CS(membase), REG32(CDMA_CS(membase)));
    printk("CDMA_CCTRL addr 0x%08x data 0x%08x\n", (u32)CDMA_CCTRL(membase), REG32(CDMA_CCTRL(membase)));
    printk("CDMA_CDBA  addr 0x%08x data 0x%08x\n", (u32)CDMA_CDBA(membase), REG32(CDMA_CDBA(membase)));
    printk("CDMA_CDLEN addr 0x%08x data 0x%08x\n", (u32)CDMA_CDLEN(membase), REG32(CDMA_CDLEN(membase)));
    printk("CDMA_CIS   addr 0x%08x data 0x%08x\n", (u32)CDMA_CIS(membase), REG32(CDMA_CIS(membase)));
    printk("CDMA_CIE   addr 0x%08x data 0x%08x\n", (u32)CDMA_CIE(membase), REG32(CDMA_CIE(membase)));
    printk("CDMA_CGBL  addr 0x%08x data 0x%08x\n", (u32)CDMA_CGBL(membase), REG32(CDMA_CGBL(membase)));
    printk("CDMA_PS    addr 0x%08x data 0x%08x\n", (u32)CDMA_PS(membase), REG32(CDMA_PS(membase)));
    printk("CDMA_PCTRL addr 0x%08x data 0x%08x\n", (u32)CDMA_PCTRL(membase), REG32(CDMA_PCTRL(membase)));
    printk("CDMA_IRNEN addr 0x%08x data 0x%08x\n", (u32)CDMA_IRNEN(membase), REG32(CDMA_IRNEN(membase)));
    printk("CDMA_IRNCR addr 0x%08x data 0x%08x\n", (u32)CDMA_IRNCR(membase), REG32(CDMA_IRNCR(membase)));
    printk("CDMA_IRNICR addr 0x%08x data 0x%08x\n", (u32)CDMA_CLC(membase), REG32(CDMA_IRNICR(membase)));
}

/* Trigger MSI interrupt */
static void cdma_channel_irq_en(u32 membase, u8 channel)
{
    u32 reg = DMA_CIE_DEFAULT;

    REG32(CDMA_CS(membase)) = channel;
    REG32(CDMA_CIS(membase)) = DMA_CIS_ALL;
    REG32(CDMA_CIE(membase)) = reg;

    reg = REG32(CDMA_IRNEN(membase));
    reg |= (1 << channel);
    REG32(CDMA_IRNEN(membase)) = reg;

    //printk("CDMA_IRNEN addr 0x%08x data 0x%08x\n", (u32)CDMA_IRNEN(membase), REG32(CDMA_IRNEN(membase))); 
}

static void cdma_channel_irq_dis(u32 membase, u8 channel)
{
    u32 reg = DMA_CIE_DEFAULT;
    
    REG32(CDMA_CS(membase)) = channel;
    REG32(CDMA_CIE(membase)) = DMA_CIE_DISABLE_ALL;
    REG32(CDMA_CIS(membase)) = DMA_CIS_ALL;
    reg = REG32(CDMA_IRNEN(membase));
    reg &= ~(1 << channel);
    REG32(CDMA_IRNEN(membase)) = reg;
    //printk("CDMA_IRNEN addr 0x%08x data 0x%08x\n", (u32)CDMA_IRNEN(membase), REG32(CDMA_IRNEN(membase)));
}

static void cdma_channel_on(u32 membase, u8 channel)
{
    REG32(CDMA_CS(membase)) = channel;
    REG32(CDMA_CCTRL(membase)) |= ((0x3<<16)| 0x1);
    cdma_channel_irq_en(membase, channel);
}

static void cdma_channel_off(u32 membase, u8 channel)
{
    REG32(CDMA_CS(membase)) = channel;
    REG32(CDMA_CCTRL(membase)) &= ~0x1;
    udelay(10);
    while (REG32(CDMA_CCTRL(membase)) & 0x01 ) { 
        REG32(CDMA_CS(membase)) = channel;
        udelay(10);
    } 
    cdma_channel_irq_dis(membase, channel);
}

#define DEFAULT_TEST_PATTEN 0x12345678

static void cdma_sdram_preload(u32 sdram_data_tx_ptr, u32 sdram_data_rx_ptr )
{
    u32 i=0,j;
    u32 testaddr = sdram_data_tx_ptr;

    for (i = 0; i < desc_num; i++) {
        for (j = 0; j <dma_data_length; j = j + 4 ) {
            REG32(testaddr + i * dma_data_length + j) = DEFAULT_TEST_PATTEN;
        }
    }
    
    printk("SDR Preload(0x55aa00ff) with Data on Memcopy Tx location done\n");

    testaddr = sdram_data_rx_ptr; 
    printk("RX Preload start address:0x%08x\n",(u32)(testaddr));

    for (i = 0; i < desc_num; i++) {
        for (j = 0; j <roundup(dma_data_length, dma_burst << 2); j = j + 4 ) {
            REG32(testaddr + i * dma_data_length + j) = 0xcccccccc;
        }
    }
    printk("SDR locations for Memcopy RX preset to 0xcccccccc done\n");
}

static void memcopy_data_check(u32 rx_data_addr)
{
    int i, j;
    u32 read_data;
    
    for (i = 0; i < desc_num; i++) {
        for(j = 0; j < dma_data_length; j = j + 4) {
            read_data = REG32(rx_data_addr + i * dma_data_length + j);
            if(read_data != DEFAULT_TEST_PATTEN) {
                printk("\nMemcopy ERROR at addr 0x%08x data 0x%08x\n", (rx_data_addr + j),(read_data));;
            }
        }
    }
}

static irqreturn_t
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
ifx_pcie_ep_cdma_intr(int irq, void *dev_id)
#else
ifx_pcie_ep_cdma_intr(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
    printk("DMA interrupt %d received\n", irq);
    return IRQ_HANDLED;
}

static void vrx218_central_dma_test(ifx_pcie_ep_dev_t *dev)
{
    int ret;
    u8 burstlen;
    u32 delay = 0;
    u32 tx_data_addr, rx_data_addr;
    u32 start, end;
    u32 cycles;
    u32 rx_desc_base;
    u32 tx_desc_base;
    u32 last_tx_desc_base;
    u32 last_rx_desc_base;
    u32 membase = (u32)(dev->membase);

    if (dma_mode == SOC_TO_EP) { /* Read from SoC DDR to local PDBRAM  */
        tx_desc_base = (u32)(membase + VRX218_MASK_ADDR(VRX218_TX_DESC));
        rx_desc_base = (u32)(membase + VRX218_MASK_ADDR(VRX218_RX_DESC));
        tx_data_addr = (u32)(LOCAL_TX1_DATA_LOC);
        rx_data_addr = (u32)(membase + VRX218_MASK_ADDR(REMOTE_RX1_DATA_LOC));
    }
    else if (dma_mode == EP_TO_SOC) { /* Write from local PDBRAM to remote DDR */
        tx_desc_base = (u32)(membase + VRX218_MASK_ADDR(VRX218_TX_DESC));
        rx_desc_base = (u32)(membase + VRX218_MASK_ADDR(VRX218_RX_DESC));
        tx_data_addr = (u32)(membase + VRX218_MASK_ADDR(REMOTE_TX1_DATA_LOC));
        rx_data_addr = (u32)(LOCAL_RX1_DATA_LOC);
    }
    else {
        return;
    }

    printk("tx_desc_base 0x%08x tx_data_addr 0x%08x rx_desc_base 0x%08x rx_data_addr 0x%08x\n",
        tx_desc_base, tx_data_addr, rx_desc_base, rx_data_addr);


    printk("dma burst %d desc number %d packet size %d\n", dma_burst, desc_num, dma_data_length);
    burstlen = dma_burst;
    last_tx_desc_base = tx_desc_base + (desc_num - 1) * sizeof (cdma_tx_descriptor_t);
    last_rx_desc_base = rx_desc_base + (desc_num - 1) * sizeof (cdma_tx_descriptor_t);

    cdma_memory_copy_init(membase);
    cdma_memory_port_cfg(membase, burstlen);
    cdma_byte_enable_cfg(membase, byte_enabled);
    
    cdma_sdram_preload(tx_data_addr, rx_data_addr);

    cdma_tx_ch_cfg(membase, dma_mode, CDMA_MEMCOPY_TX_CHAN, tx_desc_base, tx_data_addr, desc_num);
    cdma_rx_ch_cfg(membase, dma_mode, CDMA_MEMCOPY_RX_CHAN, rx_desc_base, rx_data_addr, desc_num);
   
    ret = request_irq(dev->irq, ifx_pcie_ep_cdma_intr, IRQF_DISABLED, "CDMA_MSI", dev);
    if (ret) {
        printk(KERN_ERR "%s request irq %d failed\n", __func__, dev->irq);
        return;
    }
    udelay(5); /* Make sure that RX descriptor prefetched */
    
    start = read_c0_count();
    cdma_channel_on(membase, CDMA_MEMCOPY_RX_CHAN);        
    cdma_channel_on(membase, CDMA_MEMCOPY_TX_CHAN);
   
    //wait till tx chan desc own is 0
    while((REG32(last_tx_desc_base) & 0x80000000) == 0x80000000){
        
        delay++;
        udelay(1);
    }
    end = read_c0_count();
    cycles = end - start;
    printk("cylces %d throughput %dMb\n", cycles, ((u32)(dma_data_length *desc_num * 8 * 1000 / cycles)) >> 2);
    printk("loop times %d\n", delay);
    while((REG32(last_rx_desc_base) & 0x80000000) == 0x80000000){
        delay++;
        udelay(1);
    }
   
    memcopy_data_check(rx_data_addr);
    printk(" Before stopping DMA\n");
    cdma_reg_dump(membase);

    cdma_channel_off(membase, CDMA_MEMCOPY_RX_CHAN);
    cdma_channel_off(membase, CDMA_MEMCOPY_TX_CHAN);
    printk(" After stopping DMA\n");
    cdma_reg_dump(membase);
}

static int __init 
ifx_pcie_ep_test_init(void)
{
    int i;
    int j;
    char ver_str[128] = {0};
    int dev_num;
    ifx_pcie_ep_dev_t dev;
    int module;
    
    if (ifx_pcie_ep_dev_num_get(&dev_num)) {
        printk("%s failed to get total device number\n", __func__);
        return -EIO;
    }

    printk(KERN_INFO "%s: total %d EPs found\n", __func__, dev_num);

    for (i = 0; i < dev_num; i++) {
        if (test_module == PPE_TEST) {
            module = IFX_PCIE_EP_INT_PPE;
        }
        else if (test_module == CDMA_TEST) {
            module = IFX_PCIE_EP_INT_DMA;
        }
        else {
            module = IFX_PCIE_EP_INT_PPE;
        }
        if (ifx_pcie_ep_dev_info_req(i, module, &dev)) {
            printk("%s failed to get pcie ep %d information\n", __func__, i);
        }
        printk("irq %d\n", dev.irq);
        printk("phyiscal membase 0x%08x virtual membase 0x%p\n", dev.phy_membase, dev.membase);
        if (dev_num > 1) {
            for (j = 0; j < dev.peer_num; j++) {
                printk("phyiscal peer membase 0x%08x virtual peer membase 0x%p\n", 
                    dev.peer_phy_membase[j], dev.peer_membase[j]);
            }
        }

        pcie_dev[i].irq = dev.irq;
        pcie_dev[i].membase = dev.membase;
        pcie_dev[i].phy_membase = dev.phy_membase;
        if (module == IFX_PCIE_EP_INT_PPE) {
            ppe_mbox_int_stress_test(&pcie_dev[i]);
        }
        else if (module == IFX_PCIE_EP_INT_DMA) {
            vrx218_central_dma_test(&pcie_dev[i]);
        }
    }
    ifx_drv_ver(ver_str, "PCIe test", 0, 0, 1);
    printk(KERN_INFO "%s", ver_str);
    return 0;
}

static void __exit 
ifx_pcie_ep_test_exit(void)
{
    int i;
    int dev_num;
    
    if (ifx_pcie_ep_dev_num_get(&dev_num)) {
        printk("%s failed to get total device number\n", __func__);
        return;
    }    
    printk(KERN_INFO "%s: total %d EPs found\n", __func__, dev_num);
    for (i = 0; i < dev_num; i++) {
        
        free_irq(pcie_dev[i].irq, &pcie_dev[i]);

        if (ifx_pcie_ep_dev_info_release(i)) {
            printk("%s failed to release pcie ep %d information\n", __func__, i);
        }
    }
}

module_init(ifx_pcie_ep_test_init);
module_exit(ifx_pcie_ep_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LeiChuanhua <Chuanhua.lei@lantiq.com>");
MODULE_DESCRIPTION("Lantiq VRX218 PCIe EP Address Mapping test driver");
MODULE_SUPPORTED_DEVICE ("Lantiq VRX218 SmartPHY PCIe EP");

