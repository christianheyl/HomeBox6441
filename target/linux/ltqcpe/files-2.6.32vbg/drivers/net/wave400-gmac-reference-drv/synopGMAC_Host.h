#ifndef SYNOP_GMAC_HOST_H
#define SYNOP_GMAC_HOST_H


#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>

#include "synopGMAC_plat.h"
#include "synopGMAC_pci_bus_interface.h"
#include "synopGMAC_Dev.h"

#define WAVE400_SUPPORT_DEV_TIMEOUT


//#define WAVE400_MAX_NET_ADAPTER 2

#define LOCK_TIMEOUT(lock,flags)    spin_lock_irqsave(lock/*&adapter->timeout_lock*/, flags);
#define UNLOCK_TIMEOUT(lock,flags)  spin_unlock_irqrestore(lock, flags)
#define LOCK_TIMEOUT_Q(lock)        spin_is_locked(lock)

typedef struct synopGMACAdapterStruct{
  /*Device Dependent Data structur
  */
  synopGMACdevice * synopGMACdev;
  /*Os/Platform Dependent Data Structures*/
  struct device_struct *synopGMACahbDev;
  struct net_device *synopGMACnetdev;
  struct net_device_stats synopGMACNetStats;
  u32 synopGMACPciState[16];
  u32 irq;
#ifdef WAVE400_SUPPORT_DEV_TIMEOUT
  struct work_struct tx_timeout_task;
#endif	
  spinlock_t tx_lock;
  spinlock_t rx_lock;
  spinlock_t timeout_lock;
  /*for dual gmac support use in struct params instead of global*/
  u8 *synopGMACMappedAddr;
  u32 synopGMACMappedAddrSize;
  u32 port_id;
  struct timer_list synopGMAC_cable_unplug_timer;
  /*for data handling*/
  struct tasklet_struct tasklet_rx;
  struct tasklet_struct tasklet_tx;
  /*for mem debug*/
  struct tasklet_struct tasklet_mem_debug_varient;
  struct tasklet_struct tasklet_mem_debug_scattered;
  struct tasklet_struct tasklet_mem_debug_exausted;
//  u32 dma_status_reg;
  /*keep mask status for selective enable/disable (reduce no. of ISRs)*/
  u32 DmaIntMaskStatus; 
} synopGMACPciNetworkAdapter;


#endif
