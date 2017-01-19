/* main.c main 'C' file for the linux dk driver */

/* Copyright (c) 2001 Atheros Communications, Inc., All Rights Reserved */

// Include files
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>


#if defined(OWL_PB42) || defined(PYTHON_EMU)
#include <linux/pci.h>
#endif
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/page-flags.h>
#include <asm/io.h>

#include "dk.h"
#include "client.h"
#if defined(OWL_PB42) || defined(PYTHON_EMU)
//#ifdef PCI_BUS
#include "dk_pci_bus.h"
//#endif
#endif
#define CHIP_ID_LOCATION 0xb8060090
#define HOWL_WMAC_BASE_PHY_ADDRESS 0x180c0000
#define MERLIN_PCI_COMMAND_REG_ADDRESS 0xb4000004
#define SCORPION_PCI_COMMAND_REG_ADDRESS 0xb6000004
#define VIRIAN_BASE_ADDRESS 0xb80f0000
#define SCORPION_BASE_ADDRESS 0xb8280000
#define WASP_WMAC_BASE_PHY_ADDRESS 0x18100000
#define HORNET_WMAC_BASE_PHY_ADDRESS 0x18100000
#define PCIE_1_LINK_ADDRESS 0xb80f0018
#define PCIE_2_LINK_ADDRESS 0xb8280018

#define CHIP_REV_ID_SCORPION_A 0x013 // last nibble is for Chip revision which is ignored
#define CHIP_REV_ID_SCORPION_B 0x113 // last nibble is for Chip revision which is ignored

extern INT32  dk_dev_init(void);
#if  defined(P1020)
extern A_UINT_PTR get_pci_reg_addr();
#endif
extern void dk_dev_exit(void);
extern INT32 get_chip_id(INT32 cli_id,INT32 offset,INT32 size,INT32 *ret_val);
int init_wmac_device(void);
static INT32 __init dk_module_init(void)
{
		INT32 error;
#if  defined(P1020)
	A_UINT_PTR baseaddr[1];
	UINT32 len[1];
	UINT32 irq;
#ifndef PYTHON_EMU
	UINT8  csz;
	UINT32 val;
#endif
	UINT32 iIndex, numBars;
#ifdef DK_DEBUG
    UINT32 device_id, vendor_id;
#endif
	INT8 ret_val;
	UINT32 sIndex = WMAC_FN_DEV_START_NUM;
 	VOID *dev;
#endif

#if  !defined(P1020)
#if  defined(PYTHON_EMU)
        	UINT32 *addr;
		INT32 ret_val;
#endif
#ifndef OCTEON
        INT32 chip_rev_id=0;
#endif

#ifdef DK_DEBUG
		printk("DK::Module init \n");
#endif // DK_DEBUG

#ifndef OWL_PB42
        get_chip_id(0,CHIP_ID_LOCATION,4,&chip_rev_id); // for getting the chip rev_id; to differentiate between PB and AP
        printk("CHIP REV ID: %x\n",chip_rev_id);
#endif
#if  defined(PYTHON_EMU)
if (((chip_rev_id& 0xfff0)>>4 == CHIP_REV_ID_SCORPION_A)||((chip_rev_id& 0xfff0)>>4 == CHIP_REV_ID_SCORPION_B)){ // Scorpion packages A and B (PCIE_2_LINK_ADDRESS is only valid for Scorpion)

          addr = (UINT32 *)(PCIE_2_LINK_ADDRESS);
          ret_val = readl(addr);
	if(ret_val==7){
         	printk("Writing value 0x6 to  PCI_2 command register\n");
         	addr = (UINT32 *)(SCORPION_PCI_COMMAND_REG_ADDRESS);
        	writel(0x6,addr); // enabling ddr and dma of Merlin
	}else{
         	printk("No link on PCIe_2\n");
	}
}
          addr = (UINT32 *)(PCIE_1_LINK_ADDRESS);
          ret_val = readl(addr);
	if(ret_val==7){
         	printk("Writing value 0x6 to PCI_1 command register\n");
        	addr = (UINT32 *)(MERLIN_PCI_COMMAND_REG_ADDRESS);
        	writel(0x6,addr); // enabling ddr and dma of Merlin
	}else{
         	printk("No link on PCIe_1\n");
	}
	if((chip_rev_id==0x100)||(chip_rev_id==0x1100)||(chip_rev_id==0x101)||(chip_rev_id==0x1101)
	||(chip_rev_id==0x2120)||(chip_rev_id==0x1120)||(chip_rev_id==0x0120) // Wasp 1.0 package C, B and A
	||(chip_rev_id==0x2121)||(chip_rev_id==0x1121)||(chip_rev_id==0x0121) // Wasp 1.1 package C, B and A
	||(chip_rev_id==0x2122)||(chip_rev_id==0x1122)||(chip_rev_id==0x0122) // Wasp 1.2 package C, B and A
	||(chip_rev_id==0x2123)||(chip_rev_id==0x1123)||(chip_rev_id==0x0123) // Wasp 1.3 package C, B and A
	||(chip_rev_id==0x12123)||(chip_rev_id==0x11123)||(chip_rev_id==0x10123)) {// Wasp 1.3.0.1 package C, B and A
	        addr = (UINT32 *)(VIRIAN_BASE_ADDRESS);
        	writel(readl(addr)& 0xfffeffff,addr);
	        printk("Resetting bit 16 of VIRIAN register 0xb80f0000\n");
	} else if (((chip_rev_id& 0xfff0)>>4 == CHIP_REV_ID_SCORPION_A)||((chip_rev_id& 0xfff0)>>4 == CHIP_REV_ID_SCORPION_B)){ // Scorpion packages A and B
          	addr = (UINT32 *)(PCIE_2_LINK_ADDRESS);
          	ret_val = readl(addr);
		if(ret_val==7){
	        	printk("Bit 16 of 0xb8280000 need not be reset\n");
		}
          	addr = (UINT32 *)(PCIE_1_LINK_ADDRESS);
          	ret_val = readl(addr);
		if(ret_val==7){
	        	printk("Resetting bit 16 of VIRIAN register 0xb80f0000\n");
	        	addr = (UINT32 *)(VIRIAN_BASE_ADDRESS);
        		writel(readl(addr)& 0xfffeffff,addr);
		}
   	} else {
                addr = (UINT32 *)(0xb80f001c );
                writel(readl(addr)& 0xfffeffff,addr);
                printk("Resetting bit 16 of Python  register 0xb80f001c \n");

	}

#endif
#endif // #if  !defined(P1020)

		error = dk_dev_init();
		if (error < 0) {
			printk("DK::Cannot register device \n");
			return error;
		}
		init_client();
#if  !defined(P1020)
#ifdef AP83
                if (init_wmac_device()){ // enabling the wmac ; setting the handle for applications
                         printk("Error in initializing wmac \n");
                         return error;
                }

#ifndef WASP_OSPREY
		return 0;
#endif

#endif
#endif

#if defined(OWL_PB42) || defined(PYTHON_EMU)
#if  !defined(P1020)
		     error = bus_module_init();
#endif
#if  defined(P1020)
	iIndex=0;
	baseaddr[iIndex] = (A_UINT_PTR)get_pci_reg_addr();
	printk(KERN_ERR" Base Phsycal address :0x%08lx\n", baseaddr[iIndex]);
	len[iIndex] = 0x20000;
    numBars = 1;
	irq = 17;
	sIndex = 0;

	if (add_client(dev,baseaddr,len,irq, numBars, sIndex,0) < 0) {
		printk(KERN_ERR "DK:: unable to add client \n");
#if LINUX_VERSION_CODE > 132098
//		pci_disable_device(dev);
#endif
		return -ENODEV;
	}
#endif
#endif

#if !defined(OWL_PB42) && !defined(PYTHON_EMU)
		if (error < 0) {
			cleanup_client();
			dk_dev_exit();
			printk("DK::Cannot locate device. Reset the machine \n");
			return error;
		}
#endif
		return 0;
}

#ifdef AP83
int init_wmac_device()
{

        VOID *dev;
	UINT32 baseaddr[MAX_BARS];
	UINT32 len[MAX_BARS];
	UINT32 irq;
	UINT32 iIndex, numBars;
#ifdef DK_DEBUG
    UINT16 device_id, vendor_id;
#endif
	UINT32 sIndex = WMAC_FN_DEV_START_NUM;
	dev=0;
for (iIndex=0; iIndex<1; iIndex++) { // assume that only one wmac
#ifdef WASP
	  baseaddr[iIndex] =WASP_WMAC_BASE_PHY_ADDRESS;
#elif HORNET
	  baseaddr[iIndex] =HORNET_WMAC_BASE_PHY_ADDRESS;
#else
	  baseaddr[iIndex] =HOWL_WMAC_BASE_PHY_ADDRESS;
#endif
	printk(KERN_ERR" Base Phsycal address :0x%x\n", baseaddr[iIndex]);
	  len[iIndex] = 0x00ffffff;
      if (len[iIndex] == 0) break;
    }
    numBars = iIndex;
    irq=2;
if (add_client(dev,(A_UINT_PTR *)baseaddr,len,irq, numBars, sIndex,0) < 0) {
		printk(KERN_ERR "DK:: unable to add client \n");
#if LINUX_VERSION_CODE > 132098
		//pci_disable_device(dev);
                //MKDEV
                //processEepromWriteByteBasedBlockCmd
                //pci_enable_device
#endif
		return -1;
	}
	return 0;


}
#endif


static void __exit  dk_module_exit(void)
{
#ifdef DK_DEBUG
		printk("DK::Module exit \n");
#endif // DK_DEBUG
#if defined(OWL_PB42) || defined(PYTHON_EMU)
		bus_module_exit();
#endif
		cleanup_client();
		dk_dev_exit();

		return;
}

#ifdef MODULE

#if LINUX_VERSION_CODE > 132098
	MODULE_LICENSE(MOD_LICENCE);
#endif

MODULE_AUTHOR(MOD_AUTHOR);
MODULE_DESCRIPTION(MOD_DESCRIPTION);

module_init(dk_module_init);
module_exit(dk_module_exit);

#endif // MODULE

