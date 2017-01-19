/**\file
 *  This file defines the wrapper for the platform/OS related functions
 *  The function definitions needs to be modified according to the platform 
 *  and the Operating system used.
 *  This file should be handled with greatest care while porting the driver
 *  to a different platform running different operating system other than
 *  Linux 2.6.xx.
 * \internal
 * ----------------------------REVISION HISTORY-----------------------------
 * Synopsys			01/Aug/2007			Created
 */
 
#include <linux/platform_device.h>
#include <linux/netdevice.h>
//#include "synopGMAC_plat.h"
#include "synopGMAC_Host.h"
#if 1
#include <linux/delay.h>
#endif

//extern synopGMACPciNetworkAdapter * synopGMACadapter;
/**
  * This is a wrapper function for Memory allocation routine. In linux Kernel 
  * it it kmalloc function
  * @param[in] bytes in bytes to allocate
  */

void *plat_alloc_memory(u32 bytes) 
{
	return kmalloc((size_t)bytes, GFP_KERNEL);
}


#ifdef CONFIG_AHB_INTERFACE
void plat_free_irq(struct net_device *netdev)
{
  synopGMACPciNetworkAdapter * adapter = (synopGMACPciNetworkAdapter *) netdev->priv;

  if (adapter->irq) {
    printk("free adapter->irq %d",adapter->irq);
    free_irq(adapter->irq, netdev);
    adapter->irq=0;
  }
}

#if 0
unsigned char plat_get_irq(struct device_struct *device, int num)
{
  int irq = platform_get_irq(device, num);

  return irq;
}
#endif

dma_addr_t plat_map_single(struct device_struct *device, u32 *addr, u32 size, u32 direction) 
{
//TODO - (change to struct device) dma_addr_t dma_map_single(struct device *dev, void *ptr, size_t size, enum dma_data_direction direction)

  return (dma_map_single(&device->dev, addr, size, direction));
}

/**
  * This is a wrapper function for consistent dma-able Memory allocation routine. 
  * @param[in] bytes in bytes to allocate

  * NOTE - free only the kmalloc, not the CKSEG1ADDR address !!!
  */
void *plat_alloc_consistent_dmaable_memory(struct device_struct *device, u32 size, u32 *addr, u32 direction, /*DmaDesc*/void **DescFree) 
{

  TR0 ("%s: \n",__FUNCTION__);
  *DescFree = kmalloc(size, GFP_KERNEL);
  if (*DescFree == NULL) {
    return NULL;
  }
  memset(*DescFree, 0, size);
  *addr = dma_map_single(&device->dev, *DescFree, size, direction);
  TR0 ("%s: DescFree = 0x%p, cpu_addr = 0x%p, dma_addr = 0x%08x\n",__FUNCTION__, DescFree, CKSEG1ADDR(*DescFree),*addr);
  return CKSEG1ADDR(*DescFree);
}

/**
  * This is a wrapper function for freeing consistent dma-able Memory.
  * In linux Kernel, it depends on pci dev structure
  * @param[in] bytes in bytes to allocate
  */
void plat_free_consistent_dmaable_memory(struct device_struct *device, u32 size, void * addr, u32 dma_addr, u32 direction) 
{
  dma_unmap_single(&device->dev, dma_addr, size, direction);
TR0(" kfree %08x \n",addr);
  kfree(addr);
  return;
}

void plat_unmap_single(struct device_struct *device,u32 dma_addr, u32 size, u32 direction) /* what about size 0 ? */ 
{
  dma_unmap_single(&device->dev, dma_addr, size, direction);
}

#else

void plat_free_irq(struct net_device *netdev)
{
  synopGMACPciNetworkAdapter * adapter = (synopGMACPciNetworkAdapter *) netdev->priv;
  struct pci_dev * pcidev = (struct pci_dev *)adapter->synopGMACpcidev;

	free_irq(pcidev->irq, netdev);
}

dma_addr_t plat_map_single(struct pci_dev *pcidev, u32 *addr, u32 size, u32 direction) 
{
  return (pci_map_single(pcidev, addr, size, direction));
}

/**
  * This is a wrapper function for consistent dma-able Memory allocation routine. 
  * @param[in] bytes in bytes to allocate
  */
void *plat_alloc_consistent_dmaable_memory(struct pci_dev *pcidev, u32 size, u32 *addr, u32 direction, void** DescFree) 
{
  return (pci_alloc_consistent (pcidev,size,addr));
}

/**
  * This is a wrapper function for freeing consistent dma-able Memory.
  * In linux Kernel, it depends on pci dev structure
  */
void plat_free_consistent_dmaable_memory(struct pci_dev *pcidev, u32 size, void * addr,u32 dma_addr, u32 direction) 
{
  pci_free_consistent (pcidev,size,addr,dma_addr);
  return;
}

void plat_unmap_single(struct pci_dev *pcidev,u32 dma_addr, u32 size, u32 direction) 
{
  pci_unmap_single(pcidev, dma_addr, size, direction);
}
#endif


/**
  * This is a wrapper function for Memory free routine. In linux Kernel 
  * it it kfree function
  * @param[in] buffer pointer to be freed
  */
void plat_free_memory(void *buffer) 
{
	kfree(buffer);
	return ;
}


/**
  * This is a wrapper function for platform dependent delay 
  * Take care while passing the argument to this function 
  * @param[in] buffer pointer to be freed
  */
void plat_delay(volatile u32 delay)
{
#if 1
	while (delay--) ;
#else
    mdelay(1);
#endif
	return;
}



