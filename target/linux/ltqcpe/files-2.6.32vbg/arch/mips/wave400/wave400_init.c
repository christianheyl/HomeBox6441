/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000, 2001, 2004 MIPS Technologies, Inc.
 * Copyright (C) 2001 Ralf Baechle
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/irq_cpu.h>
#include <asm/mipsregs.h>
#include <asm/system.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/kernel_stat.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <linux/platform_device.h>
#include <asm/mips-boards/generic.h>
#include <asm/mips-boards/prom.h>
#include <asm/bootinfo.h>
#include <asm/reboot.h>
#include <linux/netdevice.h>
#include "wave400_emerald_env_regs.h"
#include "wave400_defs.h"
#include "wave400_cnfg.h"
#include "wave400_chadr.h"
#include "wave400_chipreg.h"
#include "wave400_interrupt.h"
#include <synopGMAC_network_interface.h>

#define VBG400_USE_ETH1

const char *get_system_type(void)
{
	return "Lantiq VBG400";
}

const char display_string[] = "       LINUX ON WAVE400       ";

void board_reboot(char *dummy)
{
	uint32 regdata;
	
	//TBD - disable IRQ, reset the board, change to HW mode and jump to serial flash
	printk("in board_reboot\n");
    wave400_disable_irq_all();
#if 1	
	/* Configure FLASH to be in HW mode - needed? */
	regdata = (*(volatile unsigned long *)(WAVE400_SPI_MODE_ADDR)) ;
	regdata &= (~WAVE400_SPI_MODE_SW_BIT);
	*(volatile unsigned long *)(WAVE400_SPI_MODE_ADDR) = regdata;
#endif
	/* start from flash */
    printk("please wait while reboot\n");
    regdata = (*(volatile unsigned long *)(WAVE400_RESET_ADDR)) ;
	/*clear reset bit*/
	regdata &= WAVE400_REBOOT_BIT_MASK_NOT;
	*(volatile unsigned long *)(WAVE400_RESET_ADDR) = regdata;

    while (1) ;
}

/* Memory mapping for Hyperion 3.5
 0xa0000000	- 32M of DDR memory
 0xa7000000	- UART and other sys interface registers
 0xa7040000	- GMAC
 0xa6000000	- 64K of shared RAM
 0xbfc00000	- 4 bytes of very first boot code
*/ 


/* List of the devices and their resources for enumeration during platform init */

#define WAVE400_DEVICE_NAME    "mtlk"

#define WAVE400_MEM_BAR0_NAME  "wave400_wlan_bar0"
/* This memory region covers DDR memory mapped into */
/* CPU address space                                */
#define WAVE400_G35_CPU_MB_NUMBER (31) /* Number of megabyte of DDR that */
                                    /* is mapped into BB CPU internal */
                                    /* memory region                  */


#define WAVE400_MEM_BAR0_START (WAVE400_G35_CPU_MB_NUMBER*1024*1024)
#define WAVE400_MEM_BAR0_END   (WAVE400_MEM_BAR0_START + 1024*1024)

#define WAVE400_MEM_BAR1_NAME  "wave400_wlan_bar1"
/* This memory region covers NPU memory from  */
/* start of shared RAM and until end of HTExt */
/* See http://narnia/svn/hyp3.5/VLSI/Specs/NPU/DRD-054-006%20NPU%20top%20VLSI%20spec.docx */
#define WAVE400_MEM_BAR1_START (0x06000000)
#define WAVE400_MEM_BAR1_END   (0x062FFFFF)
 

#ifndef WAVE400_REGISTER_NET

#ifndef TR0
#define TR0(fmt, args...) printk(KERN_CRIT "SynopGMAC: " fmt, ##args)				

#ifdef DEBUG
#undef TR
#  define TR(fmt, args...) printk(KERN_CRIT "SynopGMAC: " fmt, ##args)
#else
# define TR(fmt, args...) /* not debugging: nothing */
#endif
#endif


#define DEVICE_NAME_RESOURCE_1 "synopGMACresource_1"
#define DEVICE_NAME_RESOURCE_2 "synopGMACresource_2"

/*TODO: delete irqIn and irqOut after change code in network driver
*/
int irqIn[] = {WAVE400_SYNOP_ETHER_IRQ_IN_INDEX,WAVE400_SYNOP_ETHER_IRQ_GMAC2_IN_INDEX};
int irqOut[] = {WAVE400_SYNOP_ETHER_IRQ_OUT_INDEX,WAVE400_SYNOP_ETHER_IRQ_GMAC2_OUT_INDEX};

//static char* synopGMAC_resources_name[] = {DEVICE_NAME_RESOURCE_1,DEVICE_NAME_RESOURCE_2};

u64 synopGMAC_dma_mask = 0xffffffff;

static struct resource synopGMAC_resources_1[] = {
  /* Configuration space */
  {
      .name  = DEVICE_NAME_RESOURCE_1,
      .start = DEVICE_REG_ADDR_MAC1_START, /* Base */
      .end   = DEVICE_REG_ADDR_MAC1_END, /* Base + Size - 1 */
      .flags = IORESOURCE_MEM,
  },
  {
    .start = WAVE400_SYNOP_ETHER_IRQ_IN_INDEX,
    .end   = WAVE400_SYNOP_ETHER_IRQ_OUT_INDEX,
    .flags = IORESOURCE_IRQ,
  }
};

static struct resource synopGMAC_resources_2[] = {
  /* Configuration space */
  {
      .name  = DEVICE_NAME_RESOURCE_2,
      .start = DEVICE_REG_ADDR_MAC2_START, /* Base */
      .end   = DEVICE_REG_ADDR_MAC2_END, /* Base + Size - 1 */
      .flags = IORESOURCE_MEM,
  },
  {
    .start = WAVE400_SYNOP_ETHER_IRQ_GMAC2_IN_INDEX,
    .end   = WAVE400_SYNOP_ETHER_IRQ_GMAC2_OUT_INDEX,
    .flags = IORESOURCE_IRQ,
  }
};

static void synopGMAC_device_release(struct device * dev) {
  TR0("Device release...");
}

static struct platform_device synopGMAC_net_device[] = {
  {
  .name		 = synopGMAC_driver_name,
  .id            = SYNOP_PORT_1,
  .num_resources = ARRAY_SIZE(synopGMAC_resources_1),
  .resource      = synopGMAC_resources_1,
  .dev           = {
    .release     = synopGMAC_device_release, /*TODO*/
    .coherent_dma_mask	= 0x0,
    .dma_mask      = &synopGMAC_dma_mask,
  },
  },
#ifdef VBG400_USE_ETH1
  {
  .name			 = synopGMAC_driver_name,
  .id            = SYNOP_PORT_2,
  .num_resources = ARRAY_SIZE(synopGMAC_resources_2),
  .resource      = synopGMAC_resources_2,
  .dev           = {
    .coherent_dma_mask	= 0x0,
    .dma_mask      = &synopGMAC_dma_mask,
  },
  },
#endif
};
#endif //WAVE400_REGISTER_NET

static struct resource wave400_ahb_resources[] = {
  [0] = {
    .name  = WAVE400_MEM_BAR0_NAME,
    .start = WAVE400_MEM_BAR0_START,
    .end   = WAVE400_MEM_BAR0_END,
    .flags = IORESOURCE_MEM,
  },
  [1] = {
    .name  = WAVE400_MEM_BAR1_NAME,
    .start = WAVE400_MEM_BAR1_START,
    .end   = WAVE400_MEM_BAR1_END,
    .flags = IORESOURCE_MEM,
  },
  [2] = {
    .start = WAVE400_WIRELESS_IRQ_IN_INDEX,
    .end   = WAVE400_WIRELESS_IRQ_OUT_INDEX,
    .flags = IORESOURCE_IRQ,
    }
};

static struct platform_device wave400_device = {
	.name = WAVE400_DEVICE_NAME,
	.id	= 0,
    .num_resources = ARRAY_SIZE(wave400_ahb_resources),
	.resource = wave400_ahb_resources,
};

/* If needed, more devices should be added to this array */
static struct platform_device *wave400_platform_devices[] __initdata = {
	&wave400_device,
#ifndef WAVE400_REGISTER_NET
    &(synopGMAC_net_device)[0],
#ifdef VBG400_USE_ETH1
    &(synopGMAC_net_device)[1],
#endif
#endif
};

/* End device\resource list */

void __init plat_mem_setup(void)
{
	unsigned char *memsize_str;
	unsigned long memsize;

	printk(KERN_INFO ": plat_mem_setup() *************\n");
	memsize_str = prom_getenv("memsize");
	if (!memsize_str) {
		memsize = 32*1024*1024;
	} else {
		memsize = simple_strtol(memsize_str, NULL, 0);
		printk(KERN_INFO "memsize param: %s\n", memsize_str);
		memsize *= (1024*1024);
	}

	add_memory_region(0, memsize - 1024*1024, BOOT_MEM_RAM); 
    _machine_restart = board_reboot;
#ifdef CONFIG_KGDB
	printk(KERN_INFO ": plat_mem_setup() ************* before kgdb_config()!!\n");
	kgdb_config();
#endif

}
void __init arch_init_irq(void)
{
	wave400_init_irq();
}

static int __init wave400_platform_init(void)
{
	return platform_add_devices(wave400_platform_devices,
			                    ARRAY_SIZE(wave400_platform_devices));
}

arch_initcall(wave400_platform_init);
