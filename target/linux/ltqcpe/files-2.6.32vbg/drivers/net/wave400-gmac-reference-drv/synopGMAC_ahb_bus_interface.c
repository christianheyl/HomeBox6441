/**\file
 * This file encapsulates all the AHB dependent initialization and resource allocation
 * on Linux
 */

#include <linux/version.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 18)
#include <linux/config.h>
#else
#include <linux/autoconf.h>
#endif
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
  
#include "wave400_interrupt.h"
#include "synopGMAC_Host.h"
#include "synopGMAC_ahb_bus_interface.h"
#include "synopGMAC_network_interface.h"

#ifdef CONFIG_AHB_INTERFACE
/**********************************************************************************/
#define SYNOPGMAC_VENDOR_ID  0x0700
#define SYNOPGMAC_DEVICE_ID  0x1108

#define BAR0  0
#define BAR1  1
#define BAR2  2
#define BAR3  3
#define BAR4  4
#define BAR5  5

/*static*/ u8 synopGMAC_driver_name[] = "synopGMACdriver";

#ifdef WAVE400_REGISTER_NET
typedef enum
{
  SYNOP_PORT_1,
  SYNOP_PORT_2,
}synopPortId;


#define DEVICE_NAME_RESOURCE_1 "synopGMACresource_1"
#define DEVICE_NAME_RESOURCE_2 "synopGMACresource_2"
static char* synopGMAC_resources_name[] = {DEVICE_NAME_RESOURCE_1,DEVICE_NAME_RESOURCE_2};
#endif //WAVE400_REGISTER_NET


/************************************************
* Warning:
* Do not use the free_irq as it free the irqaction struct,
* but as it was not allocated by kmalloc, it fails.
* (due to const param .name in the struct)
*/
static struct irqaction net_irqaction[] = {
{		.handler        = synopGMAC_intr_handler,
		.flags          = IRQF_DISABLED, /*disable nested interrupts*/ 
		/* Lior.H - when we need to use-> IRQF_NOBALANCING ? */
		.name           = "wave400_net",
},
{		.handler        = synopGMAC_intr_handler,
		.flags          = IRQF_DISABLED, /*disable nested interrupts*/ 
		/* Lior.H - when we need to use-> IRQF_NOBALANCING ? */
		.name           = "wave400_net",
},
};

void* irq_handlers[] = {wave400_net_irq, wave400_net_g2_irq};
//void (*irq_handlers[2]) (void) = {wave400_net_irq, wave400_net_g2_irq};

u32 synop_using_dac;

#ifdef WAVE400_REGISTER_NET
int irqIn[] = {WAVE400_SYNOP_ETHER_IRQ_IN_INDEX,WAVE400_SYNOP_ETHER_IRQ_GMAC2_IN_INDEX};
int irqOut[] = {WAVE400_SYNOP_ETHER_IRQ_OUT_INDEX,WAVE400_SYNOP_ETHER_IRQ_GMAC2_OUT_INDEX};

//static struct resource synopGMAC_resources[] = {
static struct resource synopGMAC_resources[][2] = {
  /* Configuration space */
  {
    {
      .name  = DEVICE_NAME_RESOURCE_1,
      .start = DEVICE_REG_ADDR_MAC1_START, /* Base */
      .end   = DEVICE_REG_ADDR_MAC1_END, /* Base + Size - 1 */
      .flags = IORESOURCE_MEM,
    }
  },
  {
    {
      .name  = DEVICE_NAME_RESOURCE_2,
      .start = DEVICE_REG_ADDR_MAC2_START, /* Base */
      .end   = DEVICE_REG_ADDR_MAC2_END, /* Base + Size - 1 */
      .flags = IORESOURCE_MEM,
    }
  }
};
#endif //WAVE400_REGISTER_NET

#ifdef WAVE400_REGISTER_NET
unsigned int synopGMAC_dma_mask = 0xffffffff;

static void synopGMAC_device_release(struct device * dev);
#endif


/**********************************************************************************/ 

u32 adapterId = 0;

extern synopGMACPciNetworkAdapter * synopGMACadapter[WAVE400_MAX_NET_ADAPTER];

int ahb_enable_device(struct platform_device *pdev)
{
  TR0 ("in ahb_enable_device...\n");
  return 0;
}

void tx_timeout_task_handler(struct work_struct *work)
{
	unsigned long flags;
	synopGMACPciNetworkAdapter *adapter = container_of(work, synopGMACPciNetworkAdapter, tx_timeout_task);

	TR0("work queue is running \n");
	LOCK_TIMEOUT(&adapter->timeout_lock, flags);

	TR0("promiscuity = %d \n",adapter->synopGMACnetdev->promiscuity);
	synopGMAC_linux_close(adapter->synopGMACnetdev);
	synopGMAC_linux_open(adapter->synopGMACnetdev);
    if (adapter->synopGMACnetdev->promiscuity && ((synopGMAC_get_filter(adapter->synopGMACdev) & GmacPromiscuousMode) == 0)) {
		TR0("dev promiscuity ON, gmac promisc off, set it \n");
        synopGMAC_promisc_enable(adapter->synopGMACdev);
    }
	UNLOCK_TIMEOUT(&adapter->timeout_lock, flags);
}

/**
 * synop_probe function of Linux driver.
 * 	- Ioremap the BARx memory (It is BAR0 here) 
 *	- lock the memory for the device
 * return: Returns 0 on success and Error code on failure.
 *
 * struct platform_device {
 *         const char      * name;
 *         int             id;
 *         struct device   dev;
 *         u32             num_resources;
 *         struct resource * resource;
 * };
 *
 * in prob we get struct platform_device param from the OS:
 * struct platform_driver {
 * int (*probe)(struct platform_device *);
 * etc.
 * }
 */ 
//static int synop_probe (struct net_device *pdev) 
static int synop_probe(struct platform_device *pdev)
{
	u32 the_register_resource_base, the_register_resource_size;
	struct resource *resource = NULL;
	struct resource *region = NULL;
	s32 err = 0;
	struct net_device *netdev;
	synopGMACdevice * gmacdev = NULL;
	struct irqaction *action = &net_irqaction[pdev->id];
    u8 *synopGMACMappedAddr;
    u32 synopGMACMappedAddrSize;
    //struct platform_device *pdev = to_platform_device(dev);
    //u32 portId = to_platform_device(dev)->id;


  /* Do probing type stuff here.  
   * Like calling request_region();
   */ 
  TR0 ("ENABLING PCI DEVICE\n");
  TR0 ("adapterId (global) = %d, platform_device pdev = 0x%p, pdev->id = %d, pdev->dev = 0x%p \n"
        ,adapterId, pdev, pdev->id, pdev->dev);
  
  /* Enable the device */ 
  if (ahb_enable_device (pdev)) //Initialize device before it's used by a driver
  {
    TR0 ("error in ahb_enable_device\n");
    return -ENODEV;
  }

  if (pdev == 0 /*|| (pdev->dev.dma_mask == 0) arad - TODO, what value, 0 or ffff..?*/)
  {
    //TODO...?
    TR0 ("ERROR: null pdev !!!\n");
    return -ENODEV;
  }

  TR0 ("call alloc_etherdev()\n");
  netdev = alloc_etherdev(sizeof(synopGMACPciNetworkAdapter));
  if(!netdev){
    TR0 ("ERROR: cant allocate netdev !!!\n");
    err = -ESYNOPGMACNOMEM;
    goto err_alloc_etherdev;
  }
  TR0 ("netdev alocated = 0x%p \n",netdev);
//  dev_set_drvdata(dev, netdev);
  /*use heard coded synop_using_dac value to DMA 32
  Note- if DMA 64 is used - ERROR result !!!
  Also, if coherent dma is used Error result !!
  */
  synop_using_dac = 0;
//  *(pdev->dev.dma_mask) = DMA_32BIT_MASK;
//  pdev->dev.coherent_dma_mask = 0; //??? TODO

//arad TODO - move memory IO handling to the ahb_enable_device handler
  
  //resource = platform_get_resource_byname(pdev, IORESOURCE_MEM, synopGMAC_resources[pdev->id].name/*DEVICE_NAME_RESOURCE*/); /*Arad - test it !!!*/
#ifdef WAVE400_REGISTER_NET
  resource = platform_get_resource_byname(pdev, IORESOURCE_MEM, (char *)(synopGMAC_resources_name[pdev->id])/*DEVICE_NAME_RESOURCE*/); /*Arad - test it !!!*/
#else
  resource = pdev->resource;
#endif
  the_register_resource_size = resource->end - resource->start + 1;
  TR0 ("the_register_resource_size = %d,resource->start = 0x%08x\n",the_register_resource_size,resource->start);
  region = request_mem_region(resource->start, the_register_resource_size, pdev->name);

  if (!region)
    return -ENOMEM;

  
  the_register_resource_base = (u32)resource->start/*ioremap(resource->start, the_register_resource_size)*/;

  TR0 ("region Base is 0x%08x size is %d\n", the_register_resource_base,the_register_resource_size);
  
  /*
     Get the iomapped address which is nothing but the physical to virtual mapped address 
     ioremap_nocahe  is similare to ioremap on most of the architectures. But The pci-ahb bridge 
     BAR0 is mapped to 16M address space. If we ask for this much memory, Kernel refuses to 
     give the same. 
     
     Note that 16M is too much of memory to request from kernel. 
     Lets ask for less memory so that kernel is happy giving it  :)
   */ 
  //synopGMACMappedAddr = (u8 *) ioremap_nocache ((u32) the_register_resource_base, (size_t) DEVICE_REG_SIZE/*(128 * 1024)*/);
  synopGMACMappedAddrSize = DEVICE_REG_SIZE/*(128 * 1024)*/;	// this is needed for remove function
  synopGMACMappedAddr = (u8*)the_register_resource_base;
  
  TR0 ("Physical address = %08x\n", the_register_resource_base);
  TR0 ("Remapped address = %08x\n", (u32) synopGMACMappedAddr);
  
/*  
  //Check if region is already locked by any other driver ? 
  if (check_mem_region ((u32) synopGMACMappedAddr, synopGMACMappedAddrSize))
  {
    synopGMACMappedAddr = 0;	// Errored in checking memory region   
    TR0("Memory Already Locked !!\n");
    iounmap (synopGMACMappedAddr);
    return -EBUSY;
  }
  
  // Great We have free memory of required size.. Lets Lock it... 
  request_mem_region ((u32) synopGMACMappedAddr, synopGMACMappedAddrSize,"synopGMACmemory");
  TR0 ("Requested memory region for synopGMACMappedAddr = 0x%x\n", (u32) synopGMACMappedAddr);
*/
  
  /*Now bus interface is ready. Let give this information to the HOST module */ 
#if 0
  synopGMACahbDev = pdev;
#endif
//  return 0;			// Everything is fine. So return 0.

	TR("Now Going to Call register_netdev to register the network interface for GMAC core\n");
	/*
	Lets allocate and set up an ethernet device, it takes the sizeof the private structure. This is mandatory as a 32 byte 
	allignment is required for the private data structure.
	*/
#if 0
	netdev = alloc_etherdev(sizeof(synopGMACPciNetworkAdapter));
	if(!netdev){
	err = -ESYNOPGMACNOMEM;
	goto err_alloc_etherdev;
	}
#endif	
	
	synopGMACadapter[pdev->id] = (synopGMACPciNetworkAdapter *)netdev_priv(netdev);
	synopGMACadapter[pdev->id]->synopGMACnetdev         = netdev; /*net device*/
	synopGMACadapter[pdev->id]->synopGMACahbDev         = pdev;   /*device*/
	synopGMACadapter[pdev->id]->synopGMACdev            = NULL;
    synopGMACadapter[pdev->id]->synopGMACMappedAddr     = synopGMACMappedAddr;
    synopGMACadapter[pdev->id]->synopGMACMappedAddrSize = synopGMACMappedAddrSize;

	
	/*Allocate Memory for the the GMACip structure*/
	synopGMACadapter[pdev->id]->synopGMACdev = (synopGMACdevice *) plat_alloc_memory(sizeof (synopGMACdevice));
	if(!synopGMACadapter[pdev->id]->synopGMACdev){
	TR0("Error in Memory Allocataion \n");
	}

	/*Attach the device to MAC struct This will configure all the required base addresses
	  such as Mac base, configuration base, phy base address(out of 32 possible phys )*/
	synopGMAC_attach(synopGMACadapter[pdev->id]->synopGMACdev,(u32) synopGMACMappedAddr + MACBASE,(u32) synopGMACMappedAddr + DMABASE, DEFAULT_PHY_BASE);

	synopGMAC_reset(synopGMACadapter[pdev->id]->synopGMACdev);

	if(synop_using_dac){
	TR("netdev->features = 0x%08x\n",netdev->features);
	TR("synop_using dac is 0x%08x\n",synop_using_dac);
	netdev->features |= NETIF_F_HIGHDMA;
	TR("netdev->features = 0x%08x\n",netdev->features);
	}
	
	netdev->open = &synopGMAC_linux_open; 
	netdev->stop = &synopGMAC_linux_close;
	netdev->hard_start_xmit = &synopGMAC_linux_xmit_frames;
	netdev->get_stats = &synopGMAC_linux_get_stats;
	netdev->set_multicast_list = &synopGMAC_linux_set_multicast_list;
	netdev -> set_mac_address = &synopGMAC_linux_set_mac_address;
	netdev -> change_mtu = &synopGMAC_linux_change_mtu;
	netdev -> do_ioctl = &synopGMAC_linux_do_ioctl;
	netdev -> tx_timeout = &synopGMAC_linux_tx_timeout; 
	netdev->watchdog_timeo = 5 * HZ;

	/*Now start the network interface*/
	TR0("Now Registering the netdevice\n");
	if((err = register_netdev(netdev)) != 0) {
		TR0("Error in Registering netdevice\n");
		return err;
	}  
	
	/*register network for use in interrupt handler
	 *external interrupt vector style
	 */
	action->dev_id = netdev;
#ifdef WAVE400_REGISTER_NET
	wave400_register_static_irq(irqIn[pdev->id], irqOut[pdev->id], &net_irqaction[pdev->id], irq_handlers[pdev->id]);
	synopGMACadapter[pdev->id]->irq = irqOut[pdev->id];
	TR0("register irqIn[%d] = %d and irqOut[%d] = %d, irq_handlers entry = 0x%p\n"
	     ,pdev->id,irqIn[pdev->id],pdev->id,irqOut[pdev->id],irq_handlers[pdev->id]);
#else
    resource = platform_get_resource(pdev, IORESOURCE_IRQ, 0); //get first occurence of IORESOURCE_IRQ type
	wave400_register_static_irq(resource->start, resource->end, &net_irqaction[pdev->id], irq_handlers[pdev->id]);
	synopGMACadapter[pdev->id]->irq = resource->end;
	TR0("register irqIn[%d] = %d and irqOut[%d] = %d, irq_handlers entry = 0x%p\n"
	     ,pdev->id,resource->start,pdev->id,resource->end,irq_handlers[pdev->id]);
#endif

#ifdef DO_CPU_STAT
	gmacdev = synopGMACadapter[pdev->id]->synopGMACdev;
	TR0("sizeof(cpu_stat_t) = %d \n",sizeof(cpu_stat_t));
	TR0("gmacdev = 0x%p \n",gmacdev);
	gmacdev->cpu_stat = plat_alloc_memory(sizeof(cpu_stat_t));
	memset(gmacdev->cpu_stat, 0 ,sizeof(cpu_stat_t));
	TR0("gmacdev->cpu_stat = 0x%p \n",gmacdev->cpu_stat);
//	spin_lock_init(&gmacdev->cpu_stat->lock);
    CPU_STAT_INIT(/*&*/gmacdev->cpu_stat);
    gmacdev->cpu_stat->pgmac = (u32)gmacdev;
	TR0("again: gmacdev->cpu_stat = 0x%p \n",gmacdev->cpu_stat);
#endif
	spin_lock_init(&synopGMACadapter[pdev->id]->tx_lock);
	spin_lock_init(&synopGMACadapter[pdev->id]->rx_lock);
	spin_lock_init(&synopGMACadapter[pdev->id]->timeout_lock);
	INIT_WORK(&synopGMACadapter[pdev->id]->tx_timeout_task, tx_timeout_task_handler);

    tasklet_init(&synopGMACadapter[pdev->id]->tasklet_rx, synopGMAC_intr_bottom_half_rx, (unsigned long)netdev);
    tasklet_init(&synopGMACadapter[pdev->id]->tasklet_tx, synopGMAC_intr_bottom_half_tx, (unsigned long)netdev);

    TR0("netdev->ifindex = %d, netdev->id = %d \n",netdev->ifindex, netdev->dev_id);
    TR0("netdev (net_device) = 0x%p, &netdev->dev (struct device) = 0x%p, pdev (platform_device) = 0x%p, &pdev->dev ((struct device)) = 0x%p \n"
        ,netdev, &netdev->dev, pdev, &pdev->dev);

  synopGMACadapter[pdev->id]->port_id = pdev->id;
  synopGMACadapter[pdev->id]->DmaIntMaskStatus = DmaIntEnable;

  adapterId++;

  return err;

err_alloc_etherdev:
	TR0("Problem in alloc_etherdev()..Take Necessary action\n");
	return err;
}



/**
 * remove function of Linux ahb driver.
 * 
 * 	- Releases the memory allocated by probe function
 *	- Unmaps the memory region 
 *
 * \return Returns 0 on success and Error code on failure.
 */ 
static void remove (struct platform_device *pdev) 
{
  
    synopGMACPciNetworkAdapter *synopGMACadapter;
    u8* synopGMACMappedAddr;
    u32 synopGMACMappedAddrSize;

    synopGMACadapter = dev_get_drvdata(&pdev->dev);
    synopGMACMappedAddr = synopGMACadapter->synopGMACMappedAddr;
    synopGMACMappedAddrSize = synopGMACadapter->synopGMACMappedAddrSize;
    /* Do the reverse of what probe does */ 
    if (synopGMACMappedAddr)
    {
      TR0 ("Releaseing synopGMACMappedAddr 0x%p whose size is %d\n", synopGMACMappedAddr, synopGMACMappedAddrSize);
      
      /*release the memory region which we locked using request_mem_region */ 
      release_mem_region ((u32)synopGMACMappedAddr, synopGMACMappedAddrSize);
    }
  TR0 ("Unmapping synopGMACMappedAddr =0x%x\n", (u32) synopGMACMappedAddr);
#if 0     //?? needed?
  iounmap (synopGMACMappedAddr);
#endif
}

/**
struct platform_driver {
  int (*probe)(struct platform_device *);
  int (*remove)(struct platform_device *);
  void (*shutdown)(struct platform_device *);
  int (*suspend)(struct platform_device *, pm_message_t state);
  int (*suspend_late)(struct platform_device *, pm_message_t state);
  int (*resume_early)(struct platform_device *);
  int (*resume)(struct platform_device *);
  struct device_driver driver;
  };
*/
/*#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)*/
static struct platform_driver synopGMAC_driver = {
  .probe    = synop_probe,
  .remove   = remove,
  .driver   = {
  .name   = synopGMAC_driver_name,
  },
};
/*#else*/
//TODO
/*#endif*/

#ifdef WAVE400_REGISTER_NET
static void synopGMAC_device_release(struct device * dev) {
  TR0("Device release...");
}

static struct platform_device synopGMAC_net_device[] = {
  {
  .name          = synopGMAC_driver_name,
  .id            = SYNOP_PORT_1,
  .num_resources = ARRAY_SIZE(synopGMAC_resources[SYNOP_PORT_1]),
  .resource      = synopGMAC_resources[SYNOP_PORT_1],
//#ifdef MODULE
  .dev           = {
    .release     = synopGMAC_device_release, /*TODO*/
#if 0
    .coherent_dma_mask	= 0xffffffff,
    .dma_mask      = &synopGMAC_net_device.dev.coherent_dma_mask,
#else
    .coherent_dma_mask	= 0x0,
    .dma_mask      = &synopGMAC_dma_mask,
#endif
  },
//#endif
  },
  {
  .name          = synopGMAC_driver_name,
  .id            = SYNOP_PORT_2,
  .num_resources = ARRAY_SIZE(synopGMAC_resources[SYNOP_PORT_2]),
  .resource      = synopGMAC_resources[SYNOP_PORT_2],
//#ifdef MODULE
  .dev           = {
    .release     = synopGMAC_device_release, /*TODO*/
#if 0
    .coherent_dma_mask	= 0xffffffff,
    .dma_mask      = &synopGMAC_net_device.dev.coherent_dma_mask,
#else
    .coherent_dma_mask	= 0x0,
    .dma_mask      = &synopGMAC_dma_mask,
#endif
  },
//#endif
  },
};
#endif //WAVE400_REGISTER_NET


/**
 * Function to initialize the Linux AHB Bus Interface.
 * Registers the net driver 
 * return: Returns 0 on success and Error code on failure.
 */ 
s32 __init synopGMAC_init_bus_interface (void) 
{
  s32 retval, devId;
  TR0 ("Now Going to Call platform_device_register\n");
//  if (synopGMACMappedAddr)
//    return -EBUSY;
  retval = platform_driver_register(&synopGMAC_driver);

#ifdef WAVE400_REGISTER_NET
  if (!retval) {
    for (devId = 0; devId < WAVE400_MAX_NET_ADAPTER; devId++) {
      TR0 ("register device # %d, num_resources = %d\n",devId, synopGMAC_net_device[devId].num_resources);
      if ((retval = platform_device_register (&synopGMAC_net_device[devId])))
      {
        TR0 ("ERROR: unregister ethernet driver\n");
        platform_driver_unregister(&synopGMAC_driver);
        return retval;
      }
    }
  }
#else
  if (retval) {
    TR0 ("ERROR: in register synopGMAC_driver\n");
  }
#endif

#if 0 //arad TODO !!!
  if (!synopGMACMappedAddr)
  {
    platform_device_unregister (&synopGMAC_net_device);
    TR0 ("ERROR: unregister ethernet driver, null synopGMACMappedAddr\n");
    return -ENODEV;
  }
#endif
  TR0 ("finish synopGMAC_init_bus_interface :-)\n");
  
  return 0;
}


/**
 * Function to De-initialize the Linux AHB Bus Interface.
 * 
 * Unregisters the net driver 
 */ 
 void __exit synopGMAC_exit_bus_interface (void) 
{
  TR0 ("Now Calling platform_device_unregister\n");
#ifdef WAVE400_REGISTER_NET
  platform_device_unregister (&synopGMAC_net_device);
#endif
  platform_driver_unregister(&synopGMAC_driver);
} 

/*
module_init(synopGMAC_init_bus_interface);
module_exit(synopGMAC_exit_bus_interface);

MODULE_AUTHOR("Synopsys India");
MODULE_LICENSE("GPL/BSD");
MODULE_DESCRIPTION("SYNOPSYS GMAC DRIVER PCI INTERFACE");

EXPORT_SYMBOL(synopGMAC_init_bus_interface);
*/ 
#endif //CONFIG_AHB_INTERFACE
