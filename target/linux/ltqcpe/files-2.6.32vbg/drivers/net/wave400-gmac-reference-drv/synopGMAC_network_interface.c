/** \file
 * This is the network dependent layer to handle network related functionality.
 * This file is tightly coupled to neworking frame work of linux 2.6.xx kernel.
 * The functionality carried out in this file should be treated as an example only
 * if the underlying operating system is not Linux. 
 * 
 * \note Many of the functions other than the device specific functions
 *  changes for operating system other than Linux 2.6.xx
 * \internal 
 *-----------------------------REVISION HISTORY-----------------------------------
 * Synopsys			01/Aug/2007				Created
 */


#include <linux/version.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 18)
#include <linux/config.h>
#else
#include <linux/autoconf.h>
#endif
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>

//#include <linux/device.h> /*in platform_device*/
#include <linux/platform_device.h>

#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/interrupt.h>

#include "synopGMAC_Host.h"
//#include "synopGMAC_plat.h"
#include "synopGMAC_network_interface.h"
#include "wave400_interrupt.h"
#include <linux/delay.h>
#include <linux/spinlock.h>


//#define WAVE400_OLD_IRQACTION
//#define DEBUG_WAVE400_LOOPBACK

/* Note- for WAVE400_BRIDGE_TIME test do IPERF test with "-M" option
*/
//#define WAVE400_BRIDGE_TIME

#define LOCK_XMIT(lock,flags)    spin_lock_irqsave(lock/*&adapter->tx_lock/rx_lock*/, flags);
#define UNLOCK_XMIT(lock,flags)  spin_unlock_irqrestore(lock, flags)
#define LOCK_XMIT_Q(lock)        spin_is_locked(lock)

#ifdef DEBUG_WAVE400_LOOPBACK
#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ARG(x) ((u8*)(x))[0],((u8*)(x))[1],((u8*)(x))[2],((u8*)(x))[3],((u8*)(x))[4],((u8*)(x))[5]
#endif

int init_desc = 0;

int dma_owner_found = 0;

#define WAVE400_IOCTL_RESTART_DMA

#define IOCTL_READ_REGISTER  SIOCDEVPRIVATE+1
#define IOCTL_WRITE_REGISTER SIOCDEVPRIVATE+2
#define IOCTL_READ_IPSTRUCT  SIOCDEVPRIVATE+3
#define IOCTL_READ_RXDESC    SIOCDEVPRIVATE+4
#define IOCTL_READ_TXDESC    SIOCDEVPRIVATE+5
#define IOCTL_POWER_DOWN     SIOCDEVPRIVATE+6
#ifdef WAVE400_IOCTL_RESTART_DMA
#define IOCTL_RESTART_DMA    SIOCDEVPRIVATE+7
#endif
#define IOCTL_CPU_STATS      SIOCDEVPRIVATE+8

static u32 GMAC_Power_down; // This global variable is used to indicate the ISR whether the interrupts occured in the process of powering down the mac or not


/*These are the global pointers for their respecive structures*/
extern synopGMACPciNetworkAdapter * synopGMACadapter[WAVE400_MAX_NET_ADAPTER];
extern u32 adapterId;
#if 0 /*? is it error?*/
extern struct net_dev             * synopGMACnetdev;
#else
extern struct net_device          * synopGMACnetdev;
#endif

/*Sample Wake-up frame filter configurations*/

u32 synopGMAC_wakeup_filter_config0[] = {
					0x00000000,	// For Filter0 CRC is not computed may be it is 0x0000
					0x00000000,	// For Filter1 CRC is not computed may be it is 0x0000
					0x00000000,	// For Filter2 CRC is not computed may be it is 0x0000
					0x5F5F5F5F,     // For Filter3 CRC is based on 0,1,2,3,4,6,8,9,10,11,12,14,16,17,18,19,20,22,24,25,26,27,28,30 bytes from offset
					0x09000000,     // Filter 0,1,2 are disabled, Filter3 is enabled and filtering applies to only multicast packets
					0x1C000000,     // Filter 0,1,2 (no significance), filter 3 offset is 28 bytes from start of Destination MAC address 
					0x00000000,     // No significance of CRC for Filter0 and Filter1
					0xBDCC0000      // No significance of CRC for Filter2, Filter3 CRC is 0xBDCC
					};
u32 synopGMAC_wakeup_filter_config1[] = {
					0x00000000,	// For Filter0 CRC is not computed may be it is 0x0000
					0x00000000,	// For Filter1 CRC is not computed may be it is 0x0000
					0x7A7A7A7A,	// For Filter2 CRC is based on 1,3,4,5,6,9,11,12,13,14,17,19,20,21,25,27,28,29,30 bytes from offset
					0x00000000,     // For Filter3 CRC is not computed may be it is 0x0000
					0x00010000,     // Filter 0,1,3 are disabled, Filter2 is enabled and filtering applies to only unicast packets
					0x00100000,     // Filter 0,1,3 (no significance), filter 2 offset is 16 bytes from start of Destination MAC address 
					0x00000000,     // No significance of CRC for Filter0 and Filter1
					0x0000A0FE      // No significance of CRC for Filter3, Filter2 CRC is 0xA0FE
					};
u32 synopGMAC_wakeup_filter_config2[] = {
					0x00000000,	// For Filter0 CRC is not computed may be it is 0x0000
					0x000000FF,	// For Filter1 CRC is computed on 0,1,2,3,4,5,6,7 bytes from offset
					0x00000000,	// For Filter2 CRC is not computed may be it is 0x0000
					0x00000000,     // For Filter3 CRC is not computed may be it is 0x0000
					0x00000100,     // Filter 0,2,3 are disabled, Filter 1 is enabled and filtering applies to only unicast packets
					0x0000DF00,     // Filter 0,2,3 (no significance), filter 1 offset is 223 bytes from start of Destination MAC address 
					0xDB9E0000,     // No significance of CRC for Filter0, Filter1 CRC is 0xDB9E
					0x00000000      // No significance of CRC for Filter2 and Filter3 
					};

/*
The synopGMAC_wakeup_filter_config3[] is a sample configuration for wake up filter. 
Filter1 is used here
Filter1 offset is programmed to 50 (0x32)
Filter1 mask is set to 0x000000FF, indicating First 8 bytes are used by the filter
Filter1 CRC= 0x7EED this is the CRC computed on data 0x55 0x55 0x55 0x55 0x55 0x55 0x55 0x55

Refer accompanied software DWC_gmac_crc_example.c for CRC16 generation and how to use the same.
*/

u32 synopGMAC_wakeup_filter_config3[] = {
					0x00000000,	// For Filter0 CRC is not computed may be it is 0x0000
					0x000000FF,	// For Filter1 CRC is computed on 0,1,2,3,4,5,6,7 bytes from offset
					0x00000000,	// For Filter2 CRC is not computed may be it is 0x0000
					0x00000000,     // For Filter3 CRC is not computed may be it is 0x0000
					0x00000100,     // Filter 0,2,3 are disabled, Filter 1 is enabled and filtering applies to only unicast packets
					0x00003200,     // Filter 0,2,3 (no significance), filter 1 offset is 50 bytes from start of Destination MAC address 
					0x7eED0000,     // No significance of CRC for Filter0, Filter1 CRC is 0x7EED, 
					0x00000000      // No significance of CRC for Filter2 and Filter3 
					};

#ifdef CONFIG_AHB_INTERFACE
#define WAVE400_DMA_TO_DEVICE   DMA_TO_DEVICE
#define WAVE400_DMA_FROM_DEVICE DMA_FROM_DEVICE
#else
#define WAVE400_DMA_TO_DEVICE   PCI_DMA_TODEVICE
#define WAVE400_DMA_FROM_DEVICE PCI_DMA_FROMDEVICE
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#define IRQF_DISABLED SA_INTERRUPT
#define IRQF_SHARED SA_SHIRQ
#endif

/**
 * Function used to detect the cable plugging and unplugging.
 * This function gets scheduled once in every second and polls
 * the PHY register for network cable plug/unplug. Once the 
 * connection is back the GMAC device is configured as per
 * new Duplex mode and Speed of the connection.
 * @param[in] u32 type but is not used currently. 
 * \return returns void.
 * \note This function is tightly coupled with Linux 2.6.xx.
 * \callgraph
 */

static void synopGMAC_linux_cable_unplug_function(u32 notused)
{
s32 status;
u16 data;
struct net_device *netdev = (struct net_device *)notused;
synopGMACdevice            *gmacdev;
synopGMACPciNetworkAdapter *adapter = (synopGMACPciNetworkAdapter *) netdev->priv;

gmacdev = adapter->synopGMACdev;

/* arad: TODO - add locks (shared by the two interfaces)
*/
init_timer(&adapter->synopGMAC_cable_unplug_timer);

adapter->synopGMAC_cable_unplug_timer.expires = CHECK_TIME + jiffies;

	status = synopGMAC_read_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase,PHY_STATUS_REG, &data);
	TR("data = 0x%x, status = 0x%08x\n",data,status);
	if(status) {
		TR0("Error reading PHY, status = 0x%08x\n",status);
		return;
    }
    if((data & Mii_Link) == 0){
   	   	TR0("No Link: %08x\n",data);
  	  	gmacdev->LinkState = 0;
		gmacdev->DuplexMode = 0;
		gmacdev->Speed = 0;
		gmacdev->LoopBackMode = 0; 
		if (netif_carrier_ok(netdev)) {
        	TR0("netif_carrier_ok, call netif_carrier_off\n");
		    netif_stop_queue(netdev);
		    netif_carrier_off(netdev);
        }
    }
    else {
		if(!gmacdev->LinkState){
        	TR0("..cable_unplug_function, adapter (notused) = 0x%p\n",adapter);
			TR0("not in link state, gmacdev->LinkState = 0x%08x \n",gmacdev->LinkState);
#if 0
			/*full Init*/
			del_timer(&adapter->synopGMAC_cable_unplug_timer);
			schedule_work(&adapter->tx_timeout_task);
#else
			/*only required changes*/
			status = synopGMAC_check_phy_discon(gmacdev);
			
			if (!netif_carrier_ok(netdev)) {
            	TR0("!netif_carrier_ok, call netif_carrier_on\n");
				netif_carrier_on(netdev);
		    	netif_wake_queue(netdev);
            }
#endif

#ifdef WAVE400_DUMP_REGS
			dump_registers(gmacdev);
#endif
		}
    }
add_timer(&adapter->synopGMAC_cable_unplug_timer);
}

static void synopGMAC_linux_cable_unplug_function_1(u32 notused)
{
synopGMAC_linux_cable_unplug_function(notused);
}


static void synopGMAC_linux_powerdown_mac(synopGMACdevice *gmacdev)
{
	TR0("Put the GMAC to power down mode..\n");
	// Disable the Dma engines in tx path
	GMAC_Power_down = 1;	// Let ISR know that Mac is going to be in the power down mode
	synopGMAC_disable_dma_tx(gmacdev);
	plat_delay(10000);		//allow any pending transmission to complete
	// Disable the Mac for both tx and rx
	synopGMAC_tx_disable(gmacdev);
	synopGMAC_rx_disable(gmacdev);
        plat_delay(10000); 		//Allow any pending buffer to be read by host
	//Disable the Dma in rx path
        synopGMAC_disable_dma_rx(gmacdev);

	//enable the power down mode
	//synopGMAC_pmt_unicast_enable(gmacdev);
	
	//prepare the gmac for magic packet reception and wake up frame reception
	synopGMAC_magic_packet_enable(gmacdev);
	synopGMAC_write_wakeup_frame_register(gmacdev, synopGMAC_wakeup_filter_config3);

	synopGMAC_wakeup_frame_enable(gmacdev);

	//gate the application and transmit clock inputs to the code. This is not done in this driver :).

	//enable the Mac for reception
	synopGMAC_rx_enable(gmacdev);

	//Enable the assertion of PMT interrupt
	synopGMAC_pmt_int_enable(gmacdev);
	//enter the power down mode
	synopGMAC_power_down_enable(gmacdev);
	return;
}

static void synopGMAC_linux_powerup_mac(synopGMACdevice *gmacdev)
{
	GMAC_Power_down = 0;	// Let ISR know that MAC is out of power down now
#if 0 //close to eliminate process time
	if( synopGMAC_is_magic_packet_received(gmacdev))
		TR("GMAC wokeup due to Magic Pkt Received\n");
	if(synopGMAC_is_wakeup_frame_received(gmacdev))
		TR("GMAC wokeup due to Wakeup Frame Received\n");
#endif
	//Disable the assertion of PMT interrupt
	synopGMAC_pmt_int_disable(gmacdev);
	//Enable the mac and Dma rx and tx paths
	synopGMAC_rx_enable(gmacdev);
	synopGMAC_enable_dma_rx(gmacdev);

	synopGMAC_tx_enable(gmacdev);
	synopGMAC_enable_dma_tx(gmacdev);
	return;
}

int synopGMAC_print_tx_desc_debug(synopGMACdevice * gmacdev, struct device_struct *device)
{
    s32 i, count=0;
    u32 val1,val2;
    struct sk_buff *skb;
    gmacdev->TxDescCount = TRANSMIT_DESC_SIZE;
    gmacdev->RxDescCount = RECEIVE_DESC_SIZE;

    TR0("dma tx state=0x%08x",(synopGMAC_get_status_dma(gmacdev) & 0x00700000));
    TR0("dma rx state=0x%08x",(synopGMAC_get_status_dma(gmacdev) & 0x000e0000));
    TR0("TX:\n");
    //if(desc_mode == RINGMODE){
      for(i =0; i < gmacdev -> TxDescCount; i++){
//        TR0("tx: %02d %08x \n",i, (unsigned int)(gmacdev->TxDesc + i) );
        val1 = synopGMAC_print_desc_own(gmacdev->TxDesc + i);
        val2 = synopGMAC_print_desc_empty(gmacdev->TxDesc + i);
        synopGMAC_print_desc(gmacdev->TxDesc + i);
        if ((val1 == 0) && (val2 != 0)) //look for mismatch
          count++;
      }
      TR0("RX:\n");
      for(i =0; i < gmacdev -> RxDescCount; i++) {
        synopGMAC_print_desc(gmacdev->RxDesc + i);
          skb = (struct sk_buff *)((gmacdev->RxDesc + i)->data1);
          TR0("0x%08x\n",(u32)plat_map_single(device, (u32 *)skb->data, (u32)skb_tailroom(skb), WAVE400_DMA_FROM_DEVICE));
          /*dma_addr1=plat_map_single(device,(u32 *)skb->data,(u32)skb_tailroom(skb),WAVE400_DMA_FROM_DEVICE)*/
      }
    //}
    TR0("count=%d\n",count);   
    return count;
}


/**
  * This sets up the transmit Descriptor queue in ring or chain mode.
  * This function is tightly coupled to the platform and operating system
  * Device is interested only after the descriptors are setup. Therefore this function
  * is not included in the device driver API. This function should be treated as an
  * example code to design the descriptor structures for ring mode or chain mode.
  * This function depends on the device structure for allocation consistent dma-able memory in case of linux.
  * This limitation is due to the fact that linux uses pci structure to allocate a dmable memory
  *	- Allocates the memory for the descriptors.
  *	- Initialize the Busy and Next descriptors indices to 0(Indicating first descriptor).
  *	- Initialize the Busy and Next descriptors to first descriptor address.
  * 	- Initialize the last descriptor with the endof ring in case of ring mode.
  *	- Initialize the descriptors in chain mode.
  * @param[in] pointer to synopGMACdevice.
  * @param[in] pointer to pci_device structure.
  * @param[in] number of descriptor expected in tx descriptor queue.
  * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
  * \return 0 upon success. Error code upon failure.
  * \note This function fails if allocation fails for required number of descriptors in Ring mode, but in chain mode
  *  function returns -ESYNOPGMACNOMEM in the process of descriptor chain creation. once returned from this function
  *  user should for gmacdev->TxDescCount to see how many descriptors are there in the chain. Should continue further
  *  only if the number of descriptors in the chain meets the requirements  
  */

s32 synopGMAC_setup_tx_desc_queue(synopGMACdevice * gmacdev,struct device_struct * device,u32 no_of_desc, u32 desc_mode)
{
	s32 i;
	DmaDesc *first_desc = NULL;
	dma_addr_t dma_addr;
	gmacdev->TxDescCount = 0;

	TR("Total size of memory required for Tx Descriptors in Ring Mode = 0x%08x\n",((sizeof(DmaDesc) * no_of_desc)));
	first_desc = (DmaDesc *)plat_alloc_consistent_dmaable_memory (device, sizeof(DmaDesc) * no_of_desc,&dma_addr, WAVE400_DMA_TO_DEVICE, &gmacdev->TxDescFree);
	if(first_desc == NULL){
		TR0("Error in Tx Descriptors memory allocation\n");
		return -ESYNOPGMACNOMEM;
	}
	gmacdev->TxDescCount = no_of_desc;
	gmacdev->TxDesc      = first_desc;
	gmacdev->TxDescDma   = dma_addr;
	
	for(i =0; i < gmacdev -> TxDescCount; i++){
		TR("%s: i = %d\n",__FUNCTION__,i);
		synopGMAC_tx_desc_init_ring(gmacdev->TxDesc + i, i == gmacdev->TxDescCount-1);
		TR("tx setup: %02d %08x \n",i, (unsigned int)(gmacdev->TxDesc + i) );
	}

	gmacdev->TxNext = 0;
	gmacdev->TxBusy = 0;
	gmacdev->TxNextDesc = gmacdev->TxDesc;
	gmacdev->TxBusyDesc = gmacdev->TxDesc;
	gmacdev->BusyTxDesc  = 0; 

	return -ESYNOPGMACNOERR;
}


/**
  * This sets up the receive Descriptor queue in ring or chain mode.
  * This function is tightly coupled to the platform and operating system
  * Device is interested only after the descriptors are setup. Therefore this function
  * is not included in the device driver API. This function should be treated as an
  * example code to design the descriptor structures in ring mode or chain mode.
  * This function depends on the device structure for allocation of consistent dma-able memory in case of linux.
  * This limitation is due to the fact that linux uses pci structure to allocate a dmable memory
  *	- Allocates the memory for the descriptors.
  *	- Initialize the Busy and Next descriptors indices to 0(Indicating first descriptor).
  *	- Initialize the Busy and Next descriptors to first descriptor address.
  * 	- Initialize the last descriptor with the endof ring in case of ring mode.
  *	- Initialize the descriptors in chain mode.
  * @param[in] pointer to synopGMACdevice.
  * @param[in] pointer to pci_device structure.
  * @param[in] number of descriptor expected in rx descriptor queue.
  * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
  * \return 0 upon success. Error code upon failure.
  * \note This function fails if allocation fails for required number of descriptors in Ring mode, but in chain mode
  *  function returns -ESYNOPGMACNOMEM in the process of descriptor chain creation. once returned from this function
  *  user should for gmacdev->RxDescCount to see how many descriptors are there in the chain. Should continue further
  *  only if the number of descriptors in the chain meets the requirements  
  */
s32 synopGMAC_setup_rx_desc_queue(synopGMACdevice * gmacdev,struct device_struct * device,u32 no_of_desc, u32 desc_mode)
{
	s32 i;
	DmaDesc *first_desc = NULL;
	dma_addr_t dma_addr;
	gmacdev->RxDescCount = 0;

	TR("total size of memory required for Rx Descriptors in Ring Mode = 0x%08x\n",((sizeof(DmaDesc) * no_of_desc)));
	first_desc = (DmaDesc *)plat_alloc_consistent_dmaable_memory (device, sizeof(DmaDesc) * no_of_desc, &dma_addr, WAVE400_DMA_FROM_DEVICE, &gmacdev->RxDescFree);
	if(first_desc == NULL){
		TR0("Error in Rx Descriptor Memory allocation in Ring mode\n");
		return -ESYNOPGMACNOMEM;
	}
	gmacdev->RxDescCount = no_of_desc;
	gmacdev->RxDesc      = first_desc;
	gmacdev->RxDescDma   = dma_addr;
	
	for(i =0; i < gmacdev -> RxDescCount; i++){
		init_desc = 1;
		TR("desc %02d %08x \n",i, (unsigned int)(gmacdev->RxDesc + i));
		synopGMAC_rx_desc_init_ring(gmacdev->RxDesc + i, i == gmacdev->RxDescCount-1);
		TR("%02d %08x \n",i, (unsigned int)(gmacdev->RxDesc + i));
	}
	init_desc = 0;
	gmacdev->RxNext = 0;
	gmacdev->RxBusy = 0;
	gmacdev->RxNextDesc = gmacdev->RxDesc;
	gmacdev->RxBusyDesc = gmacdev->RxDesc;

	gmacdev->BusyRxDesc   = 0; 

	return -ESYNOPGMACNOERR;
}

/**
  * This gives up the receive Descriptor queue in ring or chain mode.
  * This function is tightly coupled to the platform and operating system
  * Once device's Dma is stopped the memory descriptor memory and the buffer memory deallocation,
  * is completely handled by the operating system, this call is kept outside the device driver Api.
  * This function should be treated as an example code to de-allocate the descriptor structures in ring mode or chain mode
  * and network buffer deallocation.
  * This function depends on the device structure for dma-able memory deallocation for both descriptor memory and the
  * network buffer memory under linux.
  * The responsibility of this function is to 
  *	 - Free the network buffer memory if any.
  *	- Fee the memory allocated for the descriptors.
  * @param[in] pointer to synopGMACdevice.
  * @param[in] pointer to pci_device structure.
  * @param[in] number of descriptor expected in rx descriptor queue.
  * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
  * \return 0 upon success. Error code upon failure.
  * \note No referece should be made to descriptors once this function is called. This function is invoked when the device is closed.
  */
void synopGMAC_giveup_rx_desc_queue(synopGMACdevice * gmacdev, struct device_struct *device, u32 desc_mode)
{
	s32 i;
	u32 status;
	dma_addr_t dma_addr1;
	u32 length1;
	u32 data1;

	for(i =0; i < gmacdev -> RxDescCount; i++){
		synopGMAC_get_desc_data(gmacdev->RxDesc + i, &status, &dma_addr1, &length1, &data1);
		if((length1 != 0) && (data1 != 0)){
			plat_unmap_single(device,dma_addr1, 0, WAVE400_DMA_FROM_DEVICE);
			//pci_unmap_single(device,dma_addr1,0,PCI_DMA_FROMDEVICE);
			dev_kfree_skb((struct sk_buff *) data1);	// free buffer1
		}
	}
	plat_free_consistent_dmaable_memory(device,(sizeof(DmaDesc) * gmacdev->RxDescCount),/*gmacdev->RxDesc*/&gmacdev->RxDescFree,gmacdev->RxDescDma,WAVE400_DMA_FROM_DEVICE); //free descriptors memory
	gmacdev->RxDescFree= NULL;
	gmacdev->RxDesc    = NULL;
	gmacdev->RxDescDma = 0;
	return;
}

/**
  * This gives up the transmit Descriptor queue in ring or chain mode.
  * This function is tightly coupled to the platform and operating system
  * Once device's Dma is stopped the memory descriptor memory and the buffer memory deallocation,
  * is completely handled by the operating system, this call is kept outside the device driver Api.
  * This function should be treated as an example code to de-allocate the descriptor structures in ring mode or chain mode
  * and network buffer deallocation.
  * This function depends on the device structure for dma-able memory deallocation for both descriptor memory and the
  * network buffer memory under linux.
  * The responsibility of this function is to 
  *	 - Free the network buffer memory if any.
  *	- Fee the memory allocated for the descriptors.
  * @param[in] pointer to synopGMACdevice.
  * @param[in] pointer to pci_device structure.
  * @param[in] number of descriptor expected in tx descriptor queue.
  * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
  * \return 0 upon success. Error code upon failure.
  * \note No reference should be made to descriptors once this function is called. This function is invoked when the device is closed.
  */
void synopGMAC_giveup_tx_desc_queue(synopGMACdevice * gmacdev,struct device_struct * device, u32 desc_mode)
{
	s32 i;
	u32 status;
	dma_addr_t dma_addr1;
	u32 length1;
	u32 data1;
	for(i =0; i < gmacdev -> TxDescCount; i++){
		synopGMAC_get_desc_data(gmacdev->TxDesc + i,&status, &dma_addr1, &length1, &data1);
		if((length1 != 0) && (data1 != 0)){
			plat_unmap_single(device,dma_addr1, 0, WAVE400_DMA_TO_DEVICE);
			//pci_unmap_single(device,dma_addr1,0,PCI_DMA_TODEVICE);
			dev_kfree_skb((struct sk_buff *) data1);	// free buffer1
		}
	}
	TR("free Memory allocated %08x for Tx Desriptors (ring) is given back (TxDescFree is %08x)\n",(u32)gmacdev->TxDesc, &gmacdev->TxDescFree);
	plat_free_consistent_dmaable_memory(device,(sizeof(DmaDesc) * gmacdev->TxDescCount),/*gmacdev->TxDesc*/&gmacdev->TxDescFree,gmacdev->TxDescDma,WAVE400_DMA_TO_DEVICE); //free descriptors

	gmacdev->TxDescFree= NULL;
	gmacdev->TxDesc    = NULL;
	gmacdev->TxDescDma = 0;
	return;
}


/**
 * Function to handle housekeeping after a packet is transmitted over the wire.
 * After the transmission of a packet DMA generates corresponding interrupt 
 * (if it is enabled). It takes care of returning the sk_buff to the linux
 * kernel, updating the networking statistics and tracking the descriptors.
 * @param[in] pointer to net_device structure. 
 * \return void.
 * \note This function runs in interrupt context
 */
void synop_handle_transmit_over(struct net_device *netdev)
{
	synopGMACPciNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
	struct device_struct *device;
	s32 desc_index;
	u32 data1;
	u32 status;
	u32 length1;
	u32 dma_addr1;
#ifdef ENH_DESC_8W
	u32 ext_status;
	u16 time_stamp_higher;
	u32 time_stamp_high;
	u32 time_stamp_low;
#endif
	unsigned long flags;

	adapter = netdev->priv;
	if(unlikely(adapter == NULL)){
		TR0("Unknown Device \n");
		return;
	}
	
	gmacdev = adapter->synopGMACdev;
	if(unlikely(gmacdev == NULL)){
		TR0("GMAC device structure is missing \n");
		return;
	}

#if CPU_STAT_ID_IS_ENABLED(CPU_STAT_ID_RELEASE_TX_DATA)
	CPU_STAT_BEGIN_TRACK(gmacdev->cpu_stat, CPU_STAT_ID_RELEASE_TX_DATA);
#endif
 	device  = (struct device_struct *)adapter->synopGMACahbDev;	
	/*Handle the transmit Descriptors*/
	do {
		LOCK_XMIT(&adapter->tx_lock, flags);

#ifdef ENH_DESC_8W
		desc_index = synopGMAC_get_tx_qptr(gmacdev, &status, &dma_addr1, &length1, &data1,&ext_status,&time_stamp_high,&time_stamp_low);
		synopGMAC_TS_read_timestamp_higher_val(gmacdev, &time_stamp_higher);
#else
		desc_index = synopGMAC_get_tx_qptr(gmacdev, &status, &dma_addr1, &length1, &data1);
#endif
		UNLOCK_XMIT(&adapter->tx_lock, flags);
	//desc_index = synopGMAC_get_tx_qptr(gmacdev, &status, &dma_addr, &length, &data1);
		if(desc_index >= 0 && data1 != 0){
			TR("Finished Transmit at Tx Descriptor %d for skb 0x%08x and buffer = %08x whose status is %08x \n", desc_index,data1,dma_addr1,status);
			#ifdef	IPC_OFFLOAD
			if(unlikely(synopGMAC_is_tx_ipv4header_checksum_error(gmacdev, status))){
			TR0("Harware Failed to Insert IPV4 Header Checksum\n");
			}
			if(unlikely(synopGMAC_is_tx_payload_checksum_error(gmacdev, status))){
			TR0("Harware Failed to Insert Payload Checksum\n");
			}
			#endif
			plat_unmap_single(device,dma_addr1, length1, WAVE400_DMA_TO_DEVICE);
			//pci_unmap_single(device,dma_addr1,length1,PCI_DMA_TODEVICE);
			dev_kfree_skb_irq((struct sk_buff *)data1);

			if(likely(synopGMAC_is_desc_valid(status))){
				adapter->synopGMACNetStats.tx_bytes += length1;
				adapter->synopGMACNetStats.tx_packets++;
			}
			else {	
				TR("Error in Status %08x\n",status);
				adapter->synopGMACNetStats.tx_errors++;
				adapter->synopGMACNetStats.tx_aborted_errors += synopGMAC_is_tx_aborted(status);
				adapter->synopGMACNetStats.tx_carrier_errors += synopGMAC_is_tx_carrier_error(status);
			}
		}	adapter->synopGMACNetStats.collisions += synopGMAC_get_tx_collision_count(status);
	} while(desc_index >= 0);
	//netif_wake_queue(netdev);
#if CPU_STAT_ID_IS_ENABLED(CPU_STAT_ID_RELEASE_TX_DATA)
	CPU_STAT_END_TRACK(gmacdev->cpu_stat, CPU_STAT_ID_RELEASE_TX_DATA);
#endif
}



#define WAVE400_PATCH_LEN_BUG

static u32 count_index = 0;
static u32 false_rx = 0;
static u32 real_rx = 0;

/**
 * Function to Receive a packet from the interface.
 * After Receiving a packet, DMA transfers the received packet to the system memory
 * and generates corresponding interrupt (if it is enabled). This function prepares
 * the sk_buff for received packet after removing the ethernet CRC, and hands it over
 * to linux networking stack.
 * 	- Updataes the networking interface statistics
 *	- Keeps track of the rx descriptors
 * @param[in] pointer to net_device structure. 
 * \return void.
 * \note This function runs in interrupt context.
 */

void synop_handle_received_data(struct net_device *netdev)
{
	synopGMACPciNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
	struct device_struct *device;
	s32 desc_index;
	
	u32 data1;
	u32 len;
	u32 status;
	u32 dma_addr1;
#ifdef ENH_DESC_8W
//	u32 data2;
//	u32 dma_addr2;
	u32 ext_status;
	u16 time_stamp_higher;
	u32 time_stamp_high;
	u32 time_stamp_low;
#endif
	//u32 length;
	unsigned long flags;
#ifdef WAVE400_PATCH_LEN_BUG
	int len_err = 0;
#endif
	struct sk_buff *skb; //This is the pointer to hold the received data
#if 1
	u32 checksum_error;
//#define anyRxChkError (/*RxNoChkError|*/RxIpHdrChkError/*|RxLenLT600*/|RxIpHdrPayLoadChkBypass|RxChkBypass|RxPayLoadChkError)
#endif
//#define WAVE400_TEST_MISS_RX_CALLS
#ifdef WAVE400_TEST_MISS_RX_CALLS
	static u32 no_of_rx_calls = 0;
	static u32 no_of_rx_done = 0;
	u32 rx_called = 0;
#endif
//#define WAVE400_TEST_MANUAL_RX_TIME
#ifdef WAVE400_TEST_MANUAL_RX_TIME
	u32 timer_enter;
	u32 timer_out;
	u32 in_rx_loop = 0;
#endif
#ifdef WAVE400_BRIDGE_TIME
	u32* timeArray;
#endif

	
	TR("%s\n",__FUNCTION__);	
	
	adapter = netdev->priv;
	if(adapter == NULL){
		false_rx++;
		TR0("Unknown Device\n");
		return;
	}
	
	gmacdev = adapter->synopGMACdev;
	if(gmacdev == NULL){
		false_rx++;
		TR0("GMAC device structure is missing\n");
		return;
	}

	real_rx++;

#if CPU_STAT_ID_IS_ENABLED(CPU_STAT_ID_RX_DATA_FRAME)
	CPU_STAT_BEGIN_TRACK(gmacdev->cpu_stat, CPU_STAT_ID_RX_DATA_FRAME);
#endif

#ifdef WAVE400_TEST_MANUAL_RX_TIME
	no_of_rx_calls++; //WAVE400_TEST_MISS_RX_CALLS
	timer_enter = MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_TIMER_1_COUNT_VALUE); //WAVE400_TEST_MANUAL_RX_TIME
#endif

 	device  = (struct device_struct *)adapter->synopGMACahbDev;	
	/*Handle the Receive Descriptors*/
	LOCK_XMIT(&adapter->rx_lock, flags);
	do{
#if CPU_STAT_ID_IS_ENABLED(CPU_STAT_ID_GET_TEMP)
	CPU_STAT_BEGIN_TRACK(gmacdev->cpu_stat, CPU_STAT_ID_GET_TEMP);
#endif
#ifdef ENH_DESC_8W
		desc_index = synopGMAC_get_rx_qptr(gmacdev, &status,&dma_addr1,NULL, &data1/*,&dma_addr2,NULL,&data2*/,&ext_status,&time_stamp_high,&time_stamp_low);
		if(desc_index >0){ 
			synopGMAC_TS_read_timestamp_higher_val(gmacdev, &time_stamp_higher);
			TR("S:%08x ES:%08x DA1:%08x d1:%08x DA2:%08x d2:%08x TSH:%08x TSL:%08x TSHW:%08x \n",status,ext_status,dma_addr1, data1/*,dma_addr2,data2*/, time_stamp_high,time_stamp_low,time_stamp_higher);
		}

#else
		//for stats - put CPU_STAT_ID_GET_RX_QPTR here
		desc_index = synopGMAC_get_rx_qptr(gmacdev, &status,&dma_addr1,NULL, &data1/*,&dma_addr2,NULL,&data2*/);
#endif
		if(likely(desc_index >= 0/*empty*/ && data1 != 0)){
			TR("Received Data at Rx Descriptor %d for skb 0x%08x skb->data = 0x%08x whose status is %08x\n",desc_index,(struct sk_buff *)data1,((struct sk_buff *)data1)->data,status);

			//for stats - put CPU_STAT_ID_UNMAP here
			/*At first step unmapped the dma address*/
			plat_unmap_single(device,dma_addr1, 0, WAVE400_DMA_FROM_DEVICE);
			//pci_unmap_single(device,dma_addr1,0,PCI_DMA_FROMDEVICE);

			skb = (struct sk_buff *)data1;

			if(likely(synopGMAC_is_rx_desc_valid(status))){
				len =  synopGMAC_get_rx_desc_frame_length(status) - 4; //Not interested in Ethernet CRC bytes
#ifdef WAVE400_PATCH_LEN_BUG
                if (unlikely(len > 1522)) {
				    printk(KERN_CRIT "len err = 0x%08x\n",len);
            		len_err = 1;
            		goto set_free;
                }
                else
#endif
				skb_put(skb,len);//for stats - put CPU_STAT_ID_SKB_PUT here

			#ifdef IPC_OFFLOAD
				// Now lets check for the IPC offloading
				/*  Since we have enabled the checksum offloading in hardware, lets inform the kernel
					not to perform the checksum computation on the incoming packet. Note that ip header 
  					checksum will be computed by the kernel immaterial of what we inform. Similary TCP/UDP/ICMP
					pseudo header checksum will be computed by the stack. What we can inform is not to perform
					payload checksum. 		
   					When CHECKSUM_UNNECESSARY is set kernel bypasses the checksum computation.			
				*/
	
				TR("Checksum Offloading will be done now\n");
				skb->ip_summed = CHECKSUM_UNNECESSARY;
				
				#ifdef ENH_DESC_8W
				if(synopGMAC_is_ext_status(gmacdev, status)){ // extended status present indicates that the RDES4 need to be probed
					TR("Extended Status present\n");
					if(synopGMAC_ES_is_IP_header_error(gmacdev,ext_status)){       // IP header (IPV4) checksum error
					//Linux Kernel doesnot care for ipv4 header checksum. So we will simply proceed by printing a warning ....
					TR("(EXTSTS)Error in IP header error\n");
					skb->ip_summed = CHECKSUM_NONE;     //Let Kernel compute the checkssum
					}	
					if(synopGMAC_ES_is_rx_checksum_bypassed(gmacdev,ext_status)){   // Hardware engine bypassed the checksum computation/checking
					TR("(EXTSTS)Hardware bypassed checksum computation\n");	
					skb->ip_summed = CHECKSUM_NONE;             // Let Kernel compute the checksum
					}
					if(synopGMAC_ES_is_IP_payload_error(gmacdev,ext_status)){       // IP payload checksum is in error (UDP/TCP/ICMP checksum error)
					TR("(EXTSTS) Error in EP payload\n");	
					skb->ip_summed = CHECKSUM_NONE;             // Let Kernel compute the checksum
					}				
				}
				else{ // No extended status. So relevant information is available in the status itself
					if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxNoChkError ){
					TR("Ip header and TCP/UDP payload checksum Bypassed <Chk Status = 4>  \n");
					skb->ip_summed = CHECKSUM_UNNECESSARY;	//Let Kernel bypass computing the Checksum
					}
					if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxIpHdrChkError ){
					//Linux Kernel doesnot care for ipv4 header checksum. So we will simply proceed by printing a warning ....
					TR(" Error in 16bit IPV4 Header Checksum <Chk Status = 6>  \n");
					skb->ip_summed = CHECKSUM_UNNECESSARY;	//Let Kernel bypass the TCP/UDP checksum computation
					}				
					if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxLenLT600 ){
					TR("IEEE 802.3 type frame with Length field Lesss than 0x0600 <Chk Status = 0> \n");
					skb->ip_summed = CHECKSUM_NONE;	//Let Kernel compute the Checksum
					}
					if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxIpHdrPayLoadChkBypass ){
					TR("Ip header and TCP/UDP payload checksum Bypassed <Chk Status = 1>\n");
					skb->ip_summed = CHECKSUM_NONE;	//Let Kernel compute the Checksum
					}
					if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxChkBypass ){
					TR("Ip header and TCP/UDP payload checksum Bypassed <Chk Status = 3>  \n");
					skb->ip_summed = CHECKSUM_NONE;	//Let Kernel compute the Checksum
					}
					if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxPayLoadChkError ){
					TR(" TCP/UDP payload checksum Error <Chk Status = 5>  \n");
					skb->ip_summed = CHECKSUM_NONE;	//Let Kernel compute the Checksum
					}
					if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxIpHdrChkError ){
					//Linux Kernel doesnot care for ipv4 header checksum. So we will simply proceed by printing a warning ....
					TR(" Both IP header and Payload Checksum Error <Chk Status = 7>  \n");
					skb->ip_summed = CHECKSUM_NONE;	        //Let Kernel compute the Checksum
					}
				}
				#else	
#if 1 //reduce execution time
				checksum_error = synopGMAC_is_rx_checksum_error(gmacdev, status);
				skb->ip_summed = CHECKSUM_UNNECESSARY;	//Let Kernel bypass computing the Checksum
                if (checksum_error != RxNoChkError) {
					TR0("checksum_error ! <Chk Status = 0x%x>  \n",checksum_error);
                    if(synopGMAC_is_rx_checksum_error(gmacdev, status) != RxIpHdrChkError )
				        skb->ip_summed = CHECKSUM_NONE;	//Let Kernel compute the Checksum
                }
#else
				if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxNoChkError ){
				TR("Ip header and TCP/UDP payload checksum Bypassed <Chk Status = 4>  \n");
				skb->ip_summed = CHECKSUM_UNNECESSARY;	//Let Kernel bypass computing the Checksum
				}
				if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxIpHdrChkError ){
				//Linux Kernel doesnot care for ipv4 header checksum. So we will simply proceed by printing a warning ....
				TR(" Error in 16bit IPV4 Header Checksum <Chk Status = 6>  \n");
				skb->ip_summed = CHECKSUM_UNNECESSARY;	//Let Kernel bypass the TCP/UDP checksum computation
				}				
				if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxLenLT600 ){
				TR("IEEE 802.3 type frame with Length field Lesss than 0x0600 <Chk Status = 0> \n");
				skb->ip_summed = CHECKSUM_NONE;	//Let Kernel compute the Checksum
				}
				if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxIpHdrPayLoadChkBypass ){
				TR("Ip header and TCP/UDP payload checksum Bypassed <Chk Status = 1>\n");
				skb->ip_summed = CHECKSUM_NONE;	//Let Kernel compute the Checksum
				}
				if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxChkBypass ){
				TR("Ip header and TCP/UDP payload checksum Bypassed <Chk Status = 3>  \n");
				skb->ip_summed = CHECKSUM_NONE;	//Let Kernel compute the Checksum
				}
				if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxPayLoadChkError ){
				TR(" TCP/UDP payload checksum Error <Chk Status = 5>  \n");
				skb->ip_summed = CHECKSUM_NONE;	//Let Kernel compute the Checksum
				}
/* arad - why called twice !!?				if(synopGMAC_is_rx_checksum_error(gmacdev, status) == RxIpHdrChkError ){
				//Linux Kernel doesnot care for ipv4 header checksum. So we will simply proceed by printing a warning ....
				TR(" Both IP header and Payload Checksum Error <Chk Status = 7>  \n");
				skb->ip_summed = CHECKSUM_NONE;	        //Let Kernel compute the Checksum
				}
*/
#endif
				#endif
			#endif //IPC_OFFLOAD	
				skb->dev = netdev;
				skb->protocol = eth_type_trans(skb, netdev);

#ifdef DEBUG_WAVE400_LOOPBACK //for throughput by SmartBits
				  netdev->hard_start_xmit(skb, netdev);
#else

#ifdef WAVE400_BRIDGE_TIME
				if (adapter->port_id == 0) {
					    timeArray = skb->tail;
				    timeArray[0] = MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_TIMER_2_COUNT_VALUE);
				    timeArray[1] = 0x12343210;
				}
#endif
				//for stats - put CPU_STAT_ID_NETIF_RX here
				netif_rx(skb);
#endif
				//rx_called = 1; //WAVE400_TEST_MISS_RX_CALLS
                //in_rx_loop++; //WAVE400_TEST_MANUAL_RX_TIME
				netdev->last_rx = jiffies;
				adapter->synopGMACNetStats.rx_packets++;
				adapter->synopGMACNetStats.rx_bytes += len;
			}
			else{
#ifdef WAVE400_PATCH_LEN_BUG
set_free:
				//printk(KERN_CRIT "!crit: %08x, len_err = %d\n",status,len_err);
				len_err = 0;
#else
				//printk(KERN_CRIT "crit: %08x\n",status);
#endif
				/*Now the present skb should be set free*/
				TR("err: free %p\n",skb);
				dev_kfree_skb_irq(skb);
				adapter->synopGMACNetStats.rx_errors++;
				adapter->synopGMACNetStats.collisions       += synopGMAC_is_rx_frame_collision(status);
				adapter->synopGMACNetStats.rx_crc_errors    += synopGMAC_is_rx_crc(status);
				adapter->synopGMACNetStats.rx_frame_errors  += synopGMAC_is_frame_dribbling_errors(status);
				adapter->synopGMACNetStats.rx_length_errors += synopGMAC_is_rx_frame_length_errors(status);
			}
			
			//for stats - put CPU_STAT_ID_ALLOC_SKB here
			//Now lets allocate the skb for the emptied descriptor
			skb = dev_alloc_skb(netdev->mtu + ETHERNET_PACKET_EXTRA);
			TR("alloc %08x\n",skb);
			if(unlikely(skb == NULL)){
				TR0("SKB memory allocation failed \n");
				adapter->synopGMACNetStats.rx_dropped++;

#if CPU_STAT_ID_IS_ENABLED(CPU_STAT_ID_GET_TEMP)
				CPU_STAT_END_TRACK(gmacdev->cpu_stat, CPU_STAT_ID_GET_TEMP);
#endif
#if CPU_STAT_ID_IS_ENABLED(CPU_STAT_ID_RX_DATA_FRAME)
				CPU_STAT_END_TRACK(gmacdev->cpu_stat, CPU_STAT_ID_RX_DATA_FRAME);
#endif
				return; //fixMe - need to initiate recieve at a later stage !
			}
			
			//for stats - put CPU_STAT_ID_MAP here
			dma_addr1 = plat_map_single(device,(u32 *)skb->data,(u32)skb_tailroom(skb),WAVE400_DMA_FROM_DEVICE);

			TR("%s: allocating skb for RX 0x%x with data 0x%x\n", __FUNCTION__, dma_addr1, skb->data);
			desc_index = synopGMAC_set_rx_qptr(gmacdev,dma_addr1, skb_tailroom(skb), (u32)skb/*,0,0,0*/);
			
			if(unlikely(desc_index < 0)){
				TR0("Cannot set Rx Descriptor for skb %08x\n",(u32)skb);
				dev_kfree_skb_irq(skb);
			}
					
		}

#if CPU_STAT_ID_IS_ENABLED(CPU_STAT_ID_GET_TEMP)
	CPU_STAT_END_TRACK(gmacdev->cpu_stat, CPU_STAT_ID_GET_TEMP);
#endif
	}while(desc_index >= 0);

	UNLOCK_XMIT(&adapter->rx_lock, flags);

#if CPU_STAT_ID_IS_ENABLED(CPU_STAT_ID_RX_DATA_FRAME)
	CPU_STAT_END_TRACK(gmacdev->cpu_stat, CPU_STAT_ID_RX_DATA_FRAME);
#endif

#ifdef WAVE400_TEST_MISS_RX_CALLS
	if (rx_called)
   		no_of_rx_done++;

	if ((no_of_rx_calls % 1000) == 0 )
  		printk(KERN_CRIT "calls = %08x, done = %08x\n",no_of_rx_calls,no_of_rx_done);
#endif

#ifdef WAVE400_TEST_MANUAL_RX_TIME
    if (count_index >= 1000) {
      if (count_index == 0/*ignor first read*/) return;
      timer_out = MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_TIMER_1_COUNT_VALUE);
      printk("%ld, %ld\n",(timer_out-timer_enter),in_rx_loop);
      if (count_index >= 1010) {
        count_index = 0;
        printk("in %08x, out %08x\n",timer_enter,timer_out);
      }
    }
    count_index++;
#endif
#if 0
    count_index++;
    if (count_index >= 1000) {
        printk("false_rx %ld, real_rx %ld\n",false_rx,real_rx);
        count_index = 0;
    }
#endif
}


void synopGMAC_intr_bottom_half_rx(unsigned long param)
{
	synopGMACdevice * gmacdev;
	synopGMACPciNetworkAdapter *adapter = ((struct net_device *)param)->priv;
	u32 DmaIntEnable_mirr;

	gmacdev = adapter->synopGMACdev;
	DmaIntEnable_mirr = adapter->DmaIntMaskStatus;
	gmacdev = adapter->synopGMACdev;
	synop_handle_received_data((struct net_device *)param);
#if 0
	if (DmaIntEnable_mirr & DmaIntRxNormMask)
		printk(KERN_CRIT"In Rx, already set !!!\n");
#endif
	adapter->DmaIntMaskStatus |= DmaIntRxNormMask;
	synopGMAC_enable_interrupt(gmacdev,(DmaIntEnable_mirr|DmaIntRxNormMask));
}

void synopGMAC_intr_bottom_half_tx(unsigned long param)
{	   
	synopGMACdevice * gmacdev;
	synopGMACPciNetworkAdapter *adapter = ((struct net_device *)param)->priv;
	u32 DmaIntEnable_mirr;

	gmacdev = adapter->synopGMACdev;
	DmaIntEnable_mirr = adapter->DmaIntMaskStatus;
	gmacdev = adapter->synopGMACdev;
	synop_handle_transmit_over((struct net_device *)param);
#if 0
	if (DmaIntEnable_mirr & DmaIntTxNormMask)
		printk(KERN_CRIT"In Tx, already set !!!\n");
#endif
	adapter->DmaIntMaskStatus |= DmaIntTxNormMask;
	synopGMAC_enable_interrupt(gmacdev,(DmaIntEnable_mirr|DmaIntTxNormMask));
}

/**
 * Interrupt service routing.
 * This is the function registered as ISR for device interrupts.
 * @param[in] interrupt number. 
 * @param[in] void pointer to device unique structure (Required for shared interrupts in Linux).
 * @param[in] pointer to pt_regs (not used).
 * \return Returns IRQ_NONE if not device interrupts IRQ_HANDLED for device interrupts.
 * \note This function runs in interrupt context
 *
 */
// /*irqreturn_t*/int  synopGMAC_intr_handler(s32 intr_num, void * dev_id, struct pt_regs *regs)

int synopGMAC_intr_handler_1(/*s32*/int intr_num, void * dev_id)
{	   
	/*Kernels passes the netdev structure in the dev_id. So grab it*/
	struct net_device *netdev;
	synopGMACPciNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
	struct device_struct *device;
	u32 interrupt,dma_status_reg;
	s32 status;
	u32 dma_addr;
	u32 DmaIntEnable_mirr;

	netdev  = (struct net_device *) dev_id;
	if(netdev == NULL){
		TR0("Unknown Device: netdev = 0x%p\n",netdev);
		return -1;
	}

	adapter  = netdev->priv;
	if(adapter == NULL){
		TR0("Adapter Structure Missing\n");
		return -1;
	}

	gmacdev = adapter->synopGMACdev;
	if(gmacdev == NULL){
		TR0("GMAC device structure Missing\n");
		return -1;
	}

	device = (struct device_struct *)adapter->synopGMACahbDev;
	

	/*Read the Dma interrupt status to know whether the interrupt got generated by our device or not*/
	dma_status_reg = synopGMACReadReg((u32 *)gmacdev->DmaBase, DmaStatus);
	
	if(dma_status_reg == 0) {
		TR0("IRQ_NONE\n");
		return 0/*IRQ_NONE*/;
    }
#if CPU_STAT_ID_IS_ENABLED(CPU_STAT_ID_ISR_LATENCY)
		CPU_STAT_BEGIN_TRACK(gmacdev->cpu_stat, CPU_STAT_ID_ISR_LATENCY);
#endif

#if 1
	DmaIntEnable_mirr = adapter->DmaIntMaskStatus/*synopGMAC_get_interrupt_mask(gmacdev)*/;
	synopGMAC_disable_interrupt_all(gmacdev);
#endif

	if(dma_status_reg & GmacPmtIntr){
		TR("%s:: Interrupt due to PMT module\n",__FUNCTION__);
		synopGMAC_linux_powerup_mac(gmacdev);
	}
#if 0 //close to eliminate process time
	if(dma_status_reg & GmacMmcIntr){
		TR("%s:: Interrupt due to MMC module\n",__FUNCTION__);
		TR("%s:: synopGMAC_rx_int_status = %08x\n",__FUNCTION__,synopGMAC_read_mmc_rx_int_status(gmacdev));
		TR("%s:: synopGMAC_tx_int_status = %08x\n",__FUNCTION__,synopGMAC_read_mmc_tx_int_status(gmacdev));
	}

	if(dma_status_reg & GmacLineIntfIntr){
		TR("%s:: Interrupt due to GMAC LINE module\n",__FUNCTION__);
	}
#endif
	/*Now lets handle the DMA interrupts*/  
	interrupt = synopGMAC_get_interrupt_type(gmacdev);
	TR("%s:Interrupts to be handled: 0x%08x\n",__FUNCTION__,interrupt);


    /**************************************************
    * Error interrupts handled first
    *
    */
	if (unlikely(interrupt & (synopGMACDmaError_any))) {
		if(interrupt & synopGMACDmaError){
			//after soft reset, configure the MAC address to default value
            /* TODO- flash tasklets
            */
			u8 mac_addr0[6] = DEFAULT_MAC_ADDRESS;
			TR0("%s::Fatal Bus Error Inetrrupt Seen\n",__FUNCTION__);
			synopGMAC_disable_dma_tx(gmacdev);
			synopGMAC_disable_dma_rx(gmacdev);
					
			synopGMAC_take_desc_ownership_tx(gmacdev);
			synopGMAC_take_desc_ownership_rx(gmacdev);
			
			synopGMAC_init_tx_rx_desc_queue(gmacdev);
			
			synopGMAC_reset(gmacdev);//reset the DMA engine and the GMAC ip
			/*arad TODO - change to last MAC address?*/		
			synopGMAC_set_mac_addr(gmacdev,GmacAddr0High,GmacAddr0Low, mac_addr0); 
#ifdef ENH_DESC_8W
			synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength32 |
												 DmaDescriptorSkip2 |
												 DmaDescriptor8Words);
#else
#ifdef WAVE400_LEONID_CONF
			synopGMAC_dma_bus_mode_init(gmacdev, 0x03C16008);
#else
			synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength4 /*fix?*/|
											 	DmaDescriptorSkip2 |
							                 	DmaTxPriorityRatio21 |
											 	DmaFixedBurstEnable |
											 	/*DmaRxBurstLength32*/DmaRxBurstLength16 |
											 	DmaUseSeparatePBL |
											 	DmaAddrAlignedBeats/*0x2c14008*//*fix: 0x2a14408*/);
#endif
#endif//ENH_DESC_8W
#ifdef WAVE400_LEONID_CONF
			synopGMAC_dma_control_init(gmacdev, 0x06202ACE);
#else
			synopGMAC_dma_control_init(gmacdev,DmaRxThreshCtrl128 |
										   	DmaFwdUnderSzFrames |
										   	DmaFwdErrorFrames |
										   	DmaTxThreshCtrl256 |
										   	DmaTxStoreAndForward |
										   	DmaRxStoreAndForward |
	                                       	DmaTxSecondFrame |
										   	DmaDisableDropTcpCs/*0x0620c0dc*/);	
#endif
			synopGMAC_init_rx_desc_base(gmacdev);
			synopGMAC_init_tx_desc_base(gmacdev);
			synopGMAC_mac_init(gmacdev);
			synopGMAC_enable_dma_rx(gmacdev);
			synopGMAC_enable_dma_tx(gmacdev);
	
	        goto end_isr;
		}
	
		if(interrupt & synopGMACDmaRxAbnormal){
		TR("%s::Abnormal Rx Interrupt Seen\n",__FUNCTION__);
#if 1
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
				 adapter->synopGMACNetStats.rx_over_errors++;
				/*Now Descriptors have been created in synop_handle_received_data(). Just issue a poll demand to resume DMA operation*/
				synopGMAC_resume_dma_rx(gmacdev);//To handle GBPS with 12 descriptors
			}
#endif
		}
	
		if(interrupt & synopGMACDmaRxStopped){
			TR0("%s::Receiver stopped seeing Rx interrupts\n",__FUNCTION__); //Receiver gone in to stopped state
#if 1
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
			adapter->synopGMACNetStats.rx_over_errors++;
			do{
				struct sk_buff *skb = alloc_skb(netdev->mtu + ETHERNET_HEADER + ETHERNET_CRC, GFP_ATOMIC);
				if(skb == NULL){
					TR("%s::ERROR in skb buffer allocation Better Luck Next time\n",__FUNCTION__);
					break;
					//			return -ESYNOPGMACNOMEM;
				}
				dma_addr = plat_map_single(device,(u32 *)skb->data,skb_tailroom(skb),WAVE400_DMA_FROM_DEVICE);
				TR("%s: allocating skb for RX 0x%x with data 0x%x, skb = %p\n", __FUNCTION__, dma_addr, skb->data, skb);
	
				status = synopGMAC_set_rx_qptr(gmacdev,dma_addr, skb_tailroom(skb), (u32)skb/*,0,0,0*/);
				if(status < 0) {
					dev_kfree_skb_irq(skb);//changed from dev_free_skb. If problem check this again--manju
				}
			}while(status >= 0);
		
			synopGMAC_enable_dma_rx(gmacdev);
			}
#endif
		}
	
		if(interrupt & synopGMACDmaTxAbnormal){
			TR0("%s::Abnormal Tx Interrupt Seen\n",__FUNCTION__);
#if 1
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
				 if ((interrupt & synopGMACDmaTxNormal) == 0) //if not scheduled already
				 	tasklet_schedule(&adapter->tasklet_tx);
			}
#endif
		}
	
		if(interrupt & synopGMACDmaTxStopped){
			TR0("%s::Transmitter stopped sending the packets\n",__FUNCTION__);
#if 1
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
				synopGMAC_disable_dma_tx(gmacdev);
				synopGMAC_take_desc_ownership_tx(gmacdev);
			
				synopGMAC_enable_dma_tx(gmacdev);
				netif_wake_queue(netdev);
				TR("%s::Transmission Resumed\n",__FUNCTION__);
			}
#endif
		}
	}

    /**************************************************
    * Normal interrupts
    *
    */
	if(interrupt & /*synopGMACDmaRxNormal*/0x00000040){
		TR("%s:: Rx Normal \n", __FUNCTION__);
        tasklet_schedule(&adapter->tasklet_rx);
        DmaIntEnable_mirr = DmaIntEnable_mirr & (~DmaIntRxNormMask);
	}

	if(interrupt & synopGMACDmaTxNormal){
		//xmit function has done its job
		TR("%s::Finished Normal Transmission \n",__FUNCTION__);
        tasklet_schedule(&adapter->tasklet_tx);		
        DmaIntEnable_mirr = DmaIntEnable_mirr & (~DmaIntTxNormMask);
	}


end_isr:
	/* Enable the interrrupt before returning from ISR*/
	adapter->DmaIntMaskStatus = DmaIntEnable_mirr;
	synopGMAC_enable_interrupt(gmacdev,DmaIntEnable_mirr);

#if CPU_STAT_ID_IS_ENABLED(CPU_STAT_ID_ISR_LATENCY)
	CPU_STAT_END_TRACK(gmacdev->cpu_stat, CPU_STAT_ID_ISR_LATENCY);
#endif
	return 1/*IRQ_HANDLED*/;
}


/**
 * Function used when the interface is opened for use.
 * We register synopGMAC_linux_open function to linux open(). Basically this 
 * function prepares the device for operation . This function is called whenever ifconfig (in Linux)
 * activates the device (for example "ifconfig eth0 up"). This function registers
 * system resources needed 
 * 	- Attaches device to device specific structure
 * 	- Programs the MDC clock for PHY configuration
 * 	- Check and initialize the PHY interface 
 *	- ISR registration
 * 	- Setup and initialize Tx and Rx descriptors
 *	- Initialize MAC and DMA
 *	- Allocate Memory for RX descriptors (The should be DMAable)
 * 	- Initialize one second timer to detect cable plug/unplug
 *	- Configure and Enable Interrupts
 *	- Enable Tx and Rx
 *	- start the Linux network queue interface
 * @param[in] pointer to net_device structure. 
 * \return Returns 0 on success and error status upon failure.
 * \callgraph
 */

s32 synopGMAC_linux_open(struct net_device *netdev)
{
	s32 status = 0;
	s32 retval = 0;
	s32 ijk;
	//s32 reserve_len=2;
	u32 dma_addr;
	struct sk_buff *skb;
	synopGMACPciNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
	struct device_struct *device;
#if 0
#ifdef CONFIG_AHB_INTERFACE
	struct irqaction *action = &net_irqaction;
#endif
#endif
	adapter = (synopGMACPciNetworkAdapter *) netdev->priv;
	gmacdev = (synopGMACdevice *)adapter->synopGMACdev;
	device  = (struct device_struct *)adapter->synopGMACahbDev;
    TR0("adapter= %p, netdev (net_device) = 0x%p, &netdev->dev (struct device) = 0x%p, adapter->port_id = %d\n"
         ,adapter, netdev, &netdev->dev, adapter->port_id);

	/*Now platform dependent initialization.*/

#if 0
#ifndef DO_CPU_STAT //if not !!!
	/*Lets reset the IP*/
	TR0("reset the IP\n");
	TR0("adapter= %08x gmacdev = %08x netdev = %08x device= %08x\n",(u32)adapter,(u32)gmacdev,(u32)netdev,(u32)device);
	synopGMAC_reset(gmacdev);
	/*Attach the device to MAC struct This will configure all the required base addresses
	  such as Mac base, configuration base, phy base address(out of 32 possible phys )*/
	synopGMAC_attach(adapter->synopGMACdev,(u32)adapter->synopGMACMappedAddr + MACBASE,(u32)adapter->synopGMACMappedAddr + DMABASE, DEFAULT_PHY_BASE);
#endif
#endif

	/*Lets read the version of ip in to device structure*/	
	synopGMAC_read_version(gmacdev);
	
	synopGMAC_get_mac_addr(adapter->synopGMACdev,GmacAddr0High,GmacAddr0Low, netdev->dev_addr); 

	/*Now set the broadcast address*/	
	for(ijk = 0; ijk < 6; ijk++){
		netdev->broadcast[ijk] = 0xff;
	}

	for(ijk = 0; ijk <6; ijk++){
		TR("netdev->dev_addr[%d] = %02x and netdev->broadcast[%d] = %02x\n",ijk,netdev->dev_addr[ijk],ijk,netdev->broadcast[ijk]);
	}
	/*Check for Phy initialization*/
	synopGMAC_set_mdc_clk_div(gmacdev,GmiiCsrClk2);
	gmacdev->ClockDivMdc = synopGMAC_get_mdc_clk_div(gmacdev);
	status = synopGMAC_phy_init(gmacdev);
	status = synopGMAC_check_phy_init(gmacdev);

	if (!netif_carrier_ok(netdev)) {
    	TR0("!netif_carrier_ok, call netif_carrier_on\n");
		netif_carrier_on(netdev);
    }

#if 0
/*********************************************************
* Warning:
* Also for the PCI mode need special init of interrupt.
* Note:
* Move the init interrupt to synopGMAC_init_network_interface.
* So we can now use the 'ifconfig ..down/up' with no errors. 
*/
#ifdef CONFIG_AHB_INTERFACE
	/*move register network for use in interrupt handler
	 *external interrupt vector style
	 */
	action->dev_id = netdev;
	//adapter->irq = plat_get_irq(device, SYNOP_ETHER_IRQ_INDEX);
	wave400_register_static_irq(WAVE400_SYNOP_ETHER_IRQ_IN_INDEX, WAVE400_SYNOP_ETHER_IRQ_OUT_INDEX, &net_irqaction, wave400_net_irq);
	adapter->irq = WAVE400_SYNOP_ETHER_IRQ_OUT_INDEX;
// 	TR0("%s owns a shared interrupt on line %d\n",netdev->name, adapter->irq);
#else
	/*Request for an shared interrupt. Instead of using netdev->irq lets use device->irq*/
	if(request_irq (device->irq, synopGMAC_intr_handler, IRQF_SHARED | IRQF_DISABLED, netdev->name, netdev)){
 		TR0("Error in request_irq\n");
		goto error_in_irq;	
	}
#endif
#endif//0

	/*Set up the tx and rx descriptor queue/ring*/

	synopGMAC_setup_tx_desc_queue(gmacdev,device,TRANSMIT_DESC_SIZE, RINGMODE);
//	synopGMAC_setup_tx_desc_queue(gmacdev,device,TRANSMIT_DESC_SIZE, CHAINMODE);
	synopGMAC_init_tx_desc_base(gmacdev);	//Program the transmit descriptor base address in to DmaTxBase addr


	synopGMAC_setup_rx_desc_queue(gmacdev,device,RECEIVE_DESC_SIZE, RINGMODE);
//	synopGMAC_setup_rx_desc_queue(gmacdev,device,RECEIVE_DESC_SIZE, CHAINMODE);


	synopGMAC_init_rx_desc_base(gmacdev);	//Program the transmit descriptor base address in to DmaTxBase addr

	/*DmaDescriptorSkip2 to skip data1 and data2 words that are in Descriptor struct after header
	*/
#ifdef ENH_DESC_8W
	synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength32 |
										 DmaDescriptorSkip2 |
										 DmaDescriptor8Words);
#else
#ifdef WAVE400_LEONID_CONF
	synopGMAC_dma_bus_mode_init(gmacdev, 0x03C16008);
#else
	synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength4 /*fix?*/|
										 DmaDescriptorSkip2 |
										 DmaTxPriorityRatio21 |
										 DmaFixedBurstEnable |
										 /*DmaRxBurstLength32*/DmaRxBurstLength16 |
										 DmaUseSeparatePBL |
										 DmaAddrAlignedBeats/*0x2c14008*//*fix: 0x2a14408*/);
#endif
#endif //ENH_DESC_8W
#ifdef WAVE400_LEONID_CONF
	synopGMAC_dma_control_init(gmacdev, 0x06202ACE);
#else
	synopGMAC_dma_control_init(gmacdev,DmaRxThreshCtrl128 |
									   DmaFwdUnderSzFrames |
									   DmaFwdErrorFrames |
									   DmaTxThreshCtrl256 |
									   DmaTxStoreAndForward |
									   DmaRxStoreAndForward |
                                       DmaTxSecondFrame |
									   DmaDisableDropTcpCs/*0x0620c0dc*/);	
#endif

	/*Initialize the mac interface*/
	synopGMAC_mac_init(gmacdev);

#ifdef WAVE400_Rx_FIFO_4K_OR_BIGGER
	synopGMAC_pause_control(gmacdev); // This enables the pause control in Full duplex mode of operation
#endif
	#ifdef IPC_OFFLOAD
	/*IPC Checksum offloading is enabled for this driver. Should only be used if Full Ip checksum offload engine is configured in the hardware*/
	synopGMAC_enable_rx_chksum_offload(gmacdev);  	//Enable the offload engine in the receive path
	synopGMAC_rx_tcpip_chksum_drop_enable(gmacdev); // This is default configuration, DMA drops the packets if error in encapsulated ethernet payload
	// The FEF bit in DMA control register is configured to 0 indicating DMA to drop the errored frames.
	/*Inform the Linux Networking stack about the hardware capability of checksum offloading*/
	netdev->features = NETIF_F_HW_CSUM;
	#endif

	 do{
		skb = alloc_skb(netdev->mtu + ETHERNET_HEADER + ETHERNET_CRC, GFP_ATOMIC);
		if(skb == NULL){
			TR0("ERROR in skb buffer allocation\n");
			break;
//			return -ESYNOPGMACNOMEM;
		}
//		skb_reserve(skb,reserve_len);
//		TR("skb = %08x skb->tail = %08x skb_tailroom(skb)=%08x skb->data = %08x\n",(u32)skb,(u32)skb->tail,(skb_tailroom(skb)),(u32)skb->data);
//		skb->dev = netdev;
		dma_addr = plat_map_single(device,(u32 *)skb->data,skb_tailroom(skb),WAVE400_DMA_FROM_DEVICE);
		TR("%s: allocating RX %p skb dma 0x%08x with data 0x%p\n", __FUNCTION__, skb, dma_addr, skb->data);
		status = synopGMAC_set_rx_qptr(gmacdev,dma_addr, skb_tailroom(skb), (u32)skb/*,0,0,0*/);
		if(status < 0) {
			TR0("status < 0, free skb ! \n");
			dev_kfree_skb(skb);
		}	
	}while(status >= 0);
	
	TR0("Setting up the cable unplug timer, struct addr = 0x%p\n",&adapter->synopGMAC_cable_unplug_timer);
	init_timer(&adapter->synopGMAC_cable_unplug_timer);
	if (adapter->port_id == 0) {
	  adapter->synopGMAC_cable_unplug_timer.function = (void *)synopGMAC_linux_cable_unplug_function;
      TR0("cable_unplug_function\n");
	}
	else {
	  adapter->synopGMAC_cable_unplug_timer.function = (void *)synopGMAC_linux_cable_unplug_function_1;
      TR0("cable_unplug_function_1\n");
	}
	adapter->synopGMAC_cable_unplug_timer.data = (u32) netdev;
	adapter->synopGMAC_cable_unplug_timer.expires = CHECK_TIME + jiffies;
	add_timer(&adapter->synopGMAC_cable_unplug_timer);

	synopGMAC_clear_interrupt(gmacdev);
	/*
	Disable the interrupts generated by MMC and IPC counters.
	If these are not disabled ISR should be modified accordingly to handle these interrupts.
	*/	
	synopGMAC_disable_mmc_tx_interrupt(gmacdev, 0xFFFFFFFF);
	synopGMAC_disable_mmc_rx_interrupt(gmacdev, 0xFFFFFFFF);
	synopGMAC_disable_mmc_ipc_rx_interrupt(gmacdev, 0xFFFFFFFF);

	synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);

	synopGMAC_enable_dma_rx(gmacdev);
	synopGMAC_enable_dma_tx(gmacdev);

	netif_start_queue(netdev);
	TR0("%s: init ok \n",__FUNCTION__);

    wave400_enable_irq(irqOut[adapter->port_id]);
#ifdef WAVE400_DUMP_REGS
	dump_registers(gmacdev);
#endif
	return retval;

#ifndef CONFIG_AHB_INTERFACE
error_in_irq:
#endif
	/*Lets free the allocated memory*/
	TR0("%s: error_in_irq !!!!! :-( \n",__FUNCTION__);
	plat_free_memory(gmacdev);
	TR0("%s: after free_memory \n",__FUNCTION__);
	return -ESYNOPGMACBUSY;
}

/**
 * Function used when the interface is closed.
 *
 * This function is registered to linux stop() function. This function is 
 * called whenever ifconfig (in Linux) closes the device (for example "ifconfig eth0 down").
 * This releases all the system resources allocated during open call.
 * system resources int needs 
 * 	- Disable the device interrupts
 * 	- Stop the receiver and get back all the rx descriptors from the DMA
 * 	- Stop the transmitter and get back all the tx descriptors from the DMA 
 * 	- Stop the Linux network queue interface
 *	- Free the irq (ISR registered is removed from the kernel)
 * 	- Release the TX and RX descripor memory
 *	- De-initialize one second timer rgistered for cable plug/unplug tracking
 * @param[in] pointer to net_device structure. 
 * \return Returns 0 on success and error status upon failure.
 * \callgraph
 */

s32 synopGMAC_linux_close(struct net_device *netdev)
{
	
//	s32 status = 0;
//	s32 retval = 0;
//	u32 dma_addr;
	synopGMACPciNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
	struct device_struct *device;
    u32 portId;
	
	TR0("%s\n",__FUNCTION__);
	adapter = (synopGMACPciNetworkAdapter *) netdev->priv;
	if(adapter == NULL){
		TR0("OOPS adapter is null\n");
		return -1;
	}

	gmacdev = (synopGMACdevice *) adapter->synopGMACdev;
	if(gmacdev == NULL){
		TR0("OOPS gmacdev is null\n");
		return -1;
	}

	device  = (struct device_struct *)adapter->synopGMACahbDev;
	if(device == NULL){
		TR("OOPS device is null\n");
		return -1;
	}
    portId = adapter->port_id;
	//portId = to_platform_device(&netdev->dev)->id;
	TR0("synopGMAC_linux_open: portId = %d\n",portId);

	if (netif_carrier_ok(netdev)) {
    	TR0("netif_carrier_ok, call netif_carrier_off\n");
	    netif_carrier_off(netdev);
    }

	if (!netif_queue_stopped(netdev))
		netif_stop_queue(netdev);
	/*Disable all the interrupts*/
    wave400_disable_irq(irqOut[portId]);
	synopGMAC_disable_interrupt_all(gmacdev);
	TR("the synopGMAC interrupt has been disabled\n");

	/*Disable the reception*/	
	synopGMAC_disable_dma_rx(gmacdev);
	synopGMAC_take_desc_ownership_rx(gmacdev);
	TR("the synopGMAC Reception has been disabled\n");

	/*Disable the transmission*/
	synopGMAC_disable_dma_tx(gmacdev);
	synopGMAC_take_desc_ownership_tx(gmacdev);

#if 0
	/*Now free the irq: This will detach the interrupt handler registered*/
	plat_free_irq(netdev);
	TR("the synopGMAC interrupt handler has been removed\n");
#endif	
	/*Free the Rx Descriptor contents*/
	TR("Now calling synopGMAC_giveup_rx_desc_queue \n");
	synopGMAC_giveup_rx_desc_queue(gmacdev, device, RINGMODE);
//	synopGMAC_giveup_rx_desc_queue(gmacdev, device, CHAINMODE);
	TR("Now calling synopGMAC_giveup_tx_desc_queue \n");
	synopGMAC_giveup_tx_desc_queue(gmacdev, device, RINGMODE);
//	synopGMAC_giveup_tx_desc_queue(gmacdev, device, CHAINMODE);
	
	TR("Freeing the cable unplug timer\n");	
	del_timer(&adapter->synopGMAC_cable_unplug_timer);

	return -ESYNOPGMACNOERR;

//	TR("%s called \n",__FUNCTION__);
}

int validate_addr_collision(synopGMACdevice * gmacdev, u32 Buffer1)
{
	s32 i;
	u32 status;
	dma_addr_t dma_addr1;
	u32 length1;
	u32 data1;

	for(i =0; i < gmacdev -> RxDescCount; i++){
		synopGMAC_get_desc_data(gmacdev->RxDesc + i, &status, &dma_addr1, &length1, &data1);
		printk("desc index = 0x%p, dma = 0x%08x \n",gmacdev->RxDesc + i, dma_addr1);
		if (dma_addr1 == Buffer1) {
			printk("ERROR - allocated dma addr 0x%08x exists in Rx desc !!!, Buffer1 = 0x%08x\n",dma_addr1,Buffer1);
			return 1; //error
		}
	}

	return 0;

}

#ifdef WAVE400_BRIDGE_TIME
u32 count_diff = 0;
u32 count_diff_div = 0;
#endif
/**
 * Function to transmit a given packet on the wire.
 * Whenever Linux Kernel has a packet ready to be transmitted, this function is called.
 * The function prepares a packet and prepares the descriptor and 
 * enables/resumes the transmission.
 * @param[in] pointer to sk_buff structure. 
 * @param[in] pointer to net_device structure.
 * \return Returns 0 on success and Error code on failure. 
 * \note structure sk_buff is used to hold packet in Linux networking stacks.
 */
s32 synopGMAC_linux_xmit_frames(struct sk_buff *skb, struct net_device *netdev)
{
	s32 status = 0;
	u32 offload_needed = 0;
	u32 dma_addr;
	//u32 flags;
	synopGMACPciNetworkAdapter *adapter;
	synopGMACdevice * gmacdev;
	struct device_struct *device;
	unsigned long flags;
#ifdef WAVE400_BRIDGE_TIME
	u32 *timeArray;
	u32 timeNow;
#endif

//static int count = 0;
//count++;
//if(count >= 100){
//	TR0("  %d!  ",count);
//}

	if(skb == NULL){
		TR0("skb is NULL What happened to Linux Kernel? \n ");
		return -1;
	}
	
	adapter = (synopGMACPciNetworkAdapter *) netdev->priv;
	if(adapter == NULL) {
		TR0("adapter == NULL \n ");
		return -1;
	}
	gmacdev = (synopGMACdevice *) adapter->synopGMACdev;
	if(gmacdev == NULL) {
		TR0("gmacdev == NULL \n ");
		return -1;
	}

#ifdef WAVE400_BRIDGE_TIME
	if (adapter->port_id == 1) {
	    timeArray = skb->tail;
	    if (timeArray[1] == 0x12343210) {
	        timeNow = MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS, REG_TIMER_2_COUNT_VALUE);
	        count_diff += (timeNow-timeArray[0]);
	        count_diff_div++;
	        if ((count_diff_div % 1000) == 0) {
	            printk(KERN_CRIT"timeNow = 0x%08x, timeArray[0] = 0x%08x, diff = %ld\n",timeNow,timeArray[0],(count_diff/count_diff_div));
	        }
    	}
	}
#endif

#if CPU_STAT_ID_IS_ENABLED(CPU_STAT_ID_XMIT_START)
	TR("gmacdev = 0x%p, gmacdev->cpu_stat = 0x%p\n",gmacdev, gmacdev->cpu_stat);
	CPU_STAT_BEGIN_TRACK(gmacdev->cpu_stat, CPU_STAT_ID_XMIT_START);
#endif

	device  = (struct device_struct *)adapter->synopGMACahbDev;
	/*Stop the network queue*/	
	//netif_stop_queue(netdev); 

		
/*From kernel 2.6.19, the CHECKSUM_HW value has long been used in the networking subsystem to support
  hardware checksumming. That value has been replaced with CHECKSUM_PARTIAL
  (intended for outgoing packets where the job must be completed by the hardware) and CHECKSUM_COMPLETE
  (for incoming packets which have been completely checksummed by the hardware)
*/
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 19)
		if(skb->ip_summed == CHECKSUM_HW){
#else
		if(skb->ip_summed == CHECKSUM_PARTIAL){
#endif
		/*	
		   In Linux networking, if kernel indicates skb->ip_summed = CHECKSUM_HW, then only checksum offloading should be performed
		   Make sure that the OS on which this code runs have proper support to enable offloading.
		*/
		offload_needed = 0x00000001;
		#if 0
		printk(KERN_CRIT"skb->ip_summed = CHECKSUM_HW\n");
		//printk(KERN_CRIT"skb->h.th=%08x skb->h.th->check=%08x\n",(u32)(skb->h.th),(u32)(skb->h.th->check));
		//printk(KERN_CRIT"skb->h.uh=%08x skb->h.uh->check=%08x\n",(u32)(skb->h.uh),(u32)(skb->h.uh->check));
		//printk(KERN_CRIT"\n skb->len = %d skb->mac_len = %d skb->data = %08x skb->csum = %08x skb->h.raw = %08x\n",skb->len,skb->mac_len,(u32)(skb->data),skb->csum,(u32)(skb->h.raw));
		printk(KERN_CRIT"DST MAC addr:%02x %02x %02x %02x %02x %02x\n",*(skb->data+0),*(skb->data+1),*(skb->data+2),*(skb->data+3),*(skb->data+4),*(skb->data+5));
		printk(KERN_CRIT"SRC MAC addr:%02x %02x %02x %02x %02x %02x\n",*(skb->data+6),*(skb->data+7),*(skb->data+8),*(skb->data+9),*(skb->data+10),*(skb->data+11));
		printk(KERN_CRIT"Len/type	:%02x %02x\n",*(skb->data+12),*(skb->data+13));
		if(((*(skb->data+14)) & 0xF0) == 0x40){
			printk(KERN_CRIT"IPV4 Header:\n");
			printk(KERN_CRIT"%02x %02x %02x %02x\n",*(skb->data+14),*(skb->data+15),*(skb->data+16),*(skb->data+17));
			printk(KERN_CRIT"%02x %02x %02x %02x\n",*(skb->data+18),*(skb->data+19),*(skb->data+20),*(skb->data+21));
			printk(KERN_CRIT"%02x %02x %02x %02x\n",*(skb->data+22),*(skb->data+23),*(skb->data+24),*(skb->data+25));
			printk(KERN_CRIT"%02x %02x %02x %02x\n",*(skb->data+26),*(skb->data+27),*(skb->data+28),*(skb->data+29));
			printk(KERN_CRIT"%02x %02x %02x %02x\n\n",*(skb->data+30),*(skb->data+31),*(skb->data+32),*(skb->data+33));
			for(counter = 34; counter < skb->len; counter++)
				printk("%02X ",*(skb->data + counter));
		}
		else{
			printk(KERN_CRIT "IPV6 FRAME:\n");
			for(counter = 14; counter < skb->len; counter++)
				printk("%02X ",*(skb->data + counter));
		}
		#endif
		}


	
	/*Now we have skb ready and OS invoked this function. Lets make our DMA know about this*/
	dma_addr = plat_map_single(device,(u32 *)skb->data,skb->len,WAVE400_DMA_TO_DEVICE);
	TR("%s: allocating skb for TX 0x%x with data 0x%p, skb = 0x%p\n", __FUNCTION__, dma_addr, skb->data, skb);
	LOCK_XMIT(&adapter->tx_lock, flags);
	
    //for stats - put CPU_STAT_ID_SET_TX_QPTR here
	status = synopGMAC_set_tx_qptr(gmacdev, dma_addr, skb->len, (u32)skb/*,0,0,0*/,offload_needed);

	
	if(status < 0){
#ifdef WAVE400_DEBUG_FIX_FULL_BUFF
#ifdef WAVE400_DEBUG_OWN_MISMATCH
		if (synopGMAC_print_tx_desc_debug(gmacdev) > 0) {
		  //TR0("call over\n");
		  synop_handle_transmit_over(netdev);
		}		
#endif
#ifdef WAVE400_DEBUG_USE_DMA_SM_STATUS
		if ((synopGMAC_get_status_dma(gmacdev) & 0x00600000) == 0x00600000)
#endif
		synopGMAC_resume_dma_tx(gmacdev);
#endif //WAVE400_DEBUG_FIX_FULL_BUFF
	
		TR("%s No More Free Tx Descriptors\n",__FUNCTION__);
		//netif_stop_queue(netdev) called already
		UNLOCK_XMIT(&adapter->tx_lock, flags);
#if CPU_STAT_ID_IS_ENABLED(CPU_STAT_ID_XMIT_START)
		CPU_STAT_END_TRACK(gmacdev->cpu_stat, CPU_STAT_ID_XMIT_START);
#endif
		return -EBUSY;
	}
	
	UNLOCK_XMIT(&adapter->tx_lock, flags);
	/*Now force the DMA to start transmission*/	
	synopGMAC_resume_dma_tx(gmacdev);
	netdev->trans_start = jiffies;
	
	/*Now start the netdev queue*/
	//netif_wake_queue(netdev);
//if(count >= 100){
//	TR0("  end !  ");
//	count = 0;
//}
	
#if CPU_STAT_ID_IS_ENABLED(CPU_STAT_ID_XMIT_START)
	CPU_STAT_END_TRACK(gmacdev->cpu_stat, CPU_STAT_ID_XMIT_START);
#endif

	return -ESYNOPGMACNOERR;
}

/**
 * Function provides the network interface statistics.
 * Function is registered to linux get_stats() function. This function is 
 * called whenever ifconfig (in Linux) asks for networkig statistics
 * (for example "ifconfig eth0").
 * @param[in] pointer to net_device structure. 
 * \return Returns pointer to net_device_stats structure.
 * \callgraph
 */
struct net_device_stats *  synopGMAC_linux_get_stats(struct net_device *netdev)
{
TR("%s called \n",__FUNCTION__);
return( &(((synopGMACPciNetworkAdapter *)(netdev->priv))->synopGMACNetStats) );
}

/**
 * Function to set multicast and promiscous mode.
 * @param[in] pointer to net_device structure. 
 * \return returns void.
 */
void synopGMAC_linux_set_multicast_list(struct net_device *netdev)
{
synopGMACPciNetworkAdapter *adapter;

//TR0("%s called \n",__FUNCTION__);
adapter = (synopGMACPciNetworkAdapter *) netdev->priv;
TR0("2 promiscuity = %d \n",adapter->synopGMACnetdev->promiscuity);
if (netdev->flags & IFF_PROMISC) {
	TR0("flag + IFF_PROMISC\n");
	synopGMAC_promisc_enable(adapter->synopGMACdev);
}
synopGMAC_multicast_enable(adapter->synopGMACdev);
synopGMAC_broadcast_enable(adapter->synopGMACdev);
TR0("2 promiscuity after = %d \n",adapter->synopGMACnetdev->promiscuity);
return;
}

/**
 * Function to set ethernet address of the NIC.
 * @param[in] pointer to net_device structure. 
 * @param[in] pointer to an address structure. 
 * \return Returns 0 on success Errorcode on failure.
 */
s32 synopGMAC_linux_set_mac_address(struct net_device *netdev, void * macaddr)
{

synopGMACPciNetworkAdapter *adapter = NULL;
synopGMACdevice * gmacdev = NULL;
struct sockaddr *addr = macaddr;

adapter = (synopGMACPciNetworkAdapter *) netdev->priv;
if(adapter == NULL) {
	TR("adapter == NULL !!! \n");
	return -1;
}

gmacdev = adapter->synopGMACdev;
if(gmacdev == NULL) {
	TR("gmacdev == NULL !!! \n");
	return -1;
}

if(!is_valid_ether_addr(addr->sa_data)) {
	TR("is_valid_ether_addr !!! \n");
	return -EADDRNOTAVAIL;
}

synopGMAC_set_mac_addr(gmacdev,GmacAddr0High,GmacAddr0Low, addr->sa_data); 
synopGMAC_get_mac_addr(adapter->synopGMACdev,GmacAddr0High,GmacAddr0Low, netdev->dev_addr); 

TR("%s called \n",__FUNCTION__);
return 0;
}

/**
 * Function to change the Maximum Transfer Unit.
 * @param[in] pointer to net_device structure. 
 * @param[in] New value for maximum frame size.
 * \return Returns 0 on success Errorcode on failure.
 */
s32 synopGMAC_linux_change_mtu(struct net_device *netdev, s32 newmtu)
{
TR("%s called \n",__FUNCTION__);
netdev->mtu = newmtu;
return 0;

}

#ifdef DO_CPU_STAT
/* timer stats
*/

static void cpu_stats_show(synopGMACdevice * gmacdev)
{
  int i=0;

  TR0("\n"
             "CPU Utilization Statistics (measurement unit is '" CPU_STAT_UNIT "')\n"
             "\n"
             "-------------+-------------+-------------+-------------+-------------+\n"
             " Count       | Average     | Peek        | Peek SN     | Name\n"
             "-------------+-------------+-------------+-------------+-------------+\n");

  _CPU_STAT_FOREACH_TRACK_IDX(gmacdev->cpu_stat, i) {
    cpu_stat_node_t node;
    char name[32];

    TR0("i=%d\n",i);
    if (!CPU_STAT_ID_IS_ENABLED(i)) {
      TR0("skip, not enabled\n");
      continue;
    }
    
    _CPU_STAT_GET_DATA(gmacdev->cpu_stat, 
                       i,
                       &node);

    if (!node.count) {
      TR0("count = 0\n");
      continue;
    }
    _CPU_STAT_GET_NAME_EX(gmacdev->cpu_stat, i, name, sizeof(name));

    TR0(" %-12u| %-12u| %-12u| %-12u| %s %s\n",
        (uint32)node.count,
        (uint32)node.total/node.count,
        (uint32)node.peak,
        (uint32)node.peak_sn,
        name,
        _CPU_STAT_IS_ENABLED(gmacdev->cpu_stat, i)?"[*]":"");
  }

  TR0("-------------+-------------+-------------+-------------+-------------+\n");
}

static void cpu_stats_do_reset(synopGMACdevice * gmacdev)
{
  CPU_STAT_RESET(gmacdev->cpu_stat);
#ifdef WAVE400_BRIDGE_TIME
  count_diff = 0;
  count_diff_div = 0;
#endif
  return;
}

static void cpu_stats_do_reset_hw(synopGMACdevice * gmacdev, u32 data)
{
  CPU_STAT_PRINT_TIMESTAMP();
  npu_counter_enable(data);
  CPU_STAT_PRINT_TIMESTAMP();
  count_index = 0;
  return;
}

static int cpu_stats_enable_read(synopGMACdevice * gmacdev)
{
  int res = 0;
  char name[32];
  int i = 0;


  TR0("************************************");
  TR0("* CPU Statistics Available Indexes:");
  TR0("************************************");
  _CPU_STAT_FOREACH_TRACK_IDX(gmacdev->cpu_stat, i) {
    if (i != CPU_STAT_ID_NONE && !CPU_STAT_ID_IS_ENABLED(i))
      continue;

    _CPU_STAT_GET_NAME_EX(gmacdev->cpu_stat, i, name, sizeof(name));
    
    TR0("* %03d - %s %s", 
         i, name,
         _CPU_STAT_IS_ENABLED(gmacdev->cpu_stat, i)?"[*]":"");
  }
  TR0("************************************");

  return res;
}

static void cpu_stats_enable_write(synopGMACdevice * gmacdev, u32 data)
{
  TR("enable write to id %d \n",data);
  CPU_STAT_ENABLE(gmacdev->cpu_stat, data);

  return;
}
#endif

/**
 * IOCTL interface.
 * This function is mainly for debugging purpose.
 * This provides hooks for Register read write, Retrieve descriptor status
 * and Retreiving Device structure information.
 * @param[in] pointer to net_device structure. 
 * @param[in] pointer to ifreq structure.
 * @param[in] ioctl command. 
 * \return Returns 0 on success Error code on failure.
 */
s32 synopGMAC_linux_do_ioctl(struct net_device *netdev, struct ifreq *ifr, s32 cmd)
{
s32 retval = 0;
u16 temp_data = 0;
synopGMACdevice * gmacdev = NULL;
synopGMACPciNetworkAdapter *adapter = NULL;
struct ifr_data_struct
{
	u32 unit;
	u32 addr;
	u32 data;
} *req;
#ifdef DO_CPU_STAT
struct stat_data_struct
{
	u32 switch_case;
	u32 data;
} *req1;
#endif
struct device_struct *device;
#define CPU_STATS_SHOW         1
#define CPU_STATS_RESET        2
#define CPU_STATS_ENABLE_READ  3
#define CPU_STATS_ENABLE_WRITE 4
#define CPU_STATS_SET_PRESCALE 5
#define CPU_STATS_RESET_HW     6


if(netdev == NULL)
	return -1;
if(ifr == NULL)
	return -1;

req = (struct ifr_data_struct *)ifr->ifr_data;

adapter = (synopGMACPciNetworkAdapter *) netdev->priv;
if(adapter == NULL)
	return -1;

gmacdev = adapter->synopGMACdev;
if(gmacdev == NULL)
	return -1;
device  = (struct device_struct *)adapter->synopGMACahbDev;
//TR("%s :: on device %s req->unit = %08x req->addr = %08x req->data = %08x cmd = %08x \n",__FUNCTION__,netdev->name,req->unit,req->addr,req->data,cmd);

switch(cmd)
{
	case IOCTL_READ_REGISTER:		//IOCTL for reading IP registers : Read Registers
		if      (req->unit == 0)	// Read Mac Register
			req->data = synopGMACReadReg((u32 *)gmacdev->MacBase,req->addr);
		else if (req->unit == 1)	// Read DMA Register
			req->data = synopGMACReadReg((u32 *)gmacdev->DmaBase,req->addr);
		else if (req->unit == 2){	// Read Phy Register
			retval = synopGMAC_read_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase,req->addr,&temp_data);
			req->data = (u32)temp_data;
			if(retval != -ESYNOPGMACNOERR)
				TR("ERROR in Phy read\n");	
		}
		break;

	case IOCTL_WRITE_REGISTER:		//IOCTL for reading IP registers : Read Registers
		if      (req->unit == 0)	// Write Mac Register
			synopGMACWriteReg((u32 *)gmacdev->MacBase,req->addr,req->data);
		else if (req->unit == 1)	// Write DMA Register
			synopGMACWriteReg((u32 *)gmacdev->DmaBase,req->addr,req->data);
		else if (req->unit == 2){	// Write Phy Register
			retval = synopGMAC_write_phy_reg((u32 *)gmacdev->MacBase,gmacdev->PhyBase,req->addr,req->data);
			if(retval != -ESYNOPGMACNOERR)
				TR("ERROR in Phy read\n");	
		}
		break;

	case IOCTL_READ_IPSTRUCT:		//IOCTL for reading GMAC DEVICE IP private structure
	        memcpy(ifr->ifr_data, gmacdev, sizeof(synopGMACdevice));
		break;

	case IOCTL_READ_RXDESC:			//IOCTL for Reading Rx DMA DESCRIPTOR
		memcpy(ifr->ifr_data, gmacdev->RxDesc + ((DmaDesc *) (ifr->ifr_data))->data1, sizeof(DmaDesc) );
		break;

	case IOCTL_READ_TXDESC:			//IOCTL for Reading Tx DMA DESCRIPTOR
		memcpy(ifr->ifr_data, gmacdev->TxDesc + ((DmaDesc *) (ifr->ifr_data))->data1, sizeof(DmaDesc) );
		break;
	case IOCTL_POWER_DOWN:
		if	(req->unit == 1){	//power down the mac
			TR("============I will Power down the MAC now =============\n");
			// If it is already in power down don't power down again
			retval = 0;
			if(((synopGMACReadReg((u32 *)gmacdev->MacBase,GmacPmtCtrlStatus)) & GmacPmtPowerDown) != GmacPmtPowerDown){
			synopGMAC_linux_powerdown_mac(gmacdev);			
			retval = 0;
			}
		}
		if	(req->unit == 2){	//Disable the power down  and wake up the Mac locally
			TR("============I will Power up the MAC now =============\n");
			//If already powered down then only try to wake up
			retval = -1;
			if(((synopGMACReadReg((u32 *)gmacdev->MacBase,GmacPmtCtrlStatus)) & GmacPmtPowerDown) == GmacPmtPowerDown){
			synopGMAC_power_down_disable(gmacdev);
			synopGMAC_linux_powerup_mac(gmacdev);
			retval = 0;
			}
		}
		break;
#ifdef WAVE400_IOCTL_RESTART_DMA
	case IOCTL_RESTART_DMA:			//IOCTL for Reading Tx DMA DESCRIPTOR
			TR0("\n lock status is: tx=%d, rx=%d\n",LOCK_XMIT_Q(&adapter->tx_lock),LOCK_XMIT_Q(&adapter->rx_lock));
			synopGMAC_print_tx_desc_debug(gmacdev,device);
			synop_handle_transmit_over(netdev);
			synopGMAC_resume_dma_tx(gmacdev);
		break;
#endif

	case IOCTL_CPU_STATS:			//IOCTL for Reading timer statistics
#ifdef DO_CPU_STAT
		req1 = (struct stat_data_struct *)ifr->ifr_data;
		TR0("case IOCTL_CPU_STATS, req1->switch_case = %d \n",req1->switch_case);
		
		switch(req1->switch_case) {
		case CPU_STATS_SHOW:
			TR0("call cpu_stats_show \n");
			cpu_stats_show(gmacdev);
			break;
		case CPU_STATS_RESET:
			TR0("call cpu_stats_do_reset \n");
			cpu_stats_do_reset(gmacdev);
			break;
		case CPU_STATS_ENABLE_READ:
			TR0("call cpu_stats_enable_read \n");
			cpu_stats_enable_read(gmacdev);
			break;
		case CPU_STATS_ENABLE_WRITE:
			TR("call cpu_stats_enable_write, req1->data = 0x%08x \n",req1->data);
			cpu_stats_enable_write(gmacdev, req1->data);
			break;
		case CPU_STATS_SET_PRESCALE:
			TR0("prescale = 0x%08x \n",prescale);
			TR0("set new value = 0x%08x \n",req1->data);
			prescale = req1->data;
			break;
		case CPU_STATS_RESET_HW:
			TR("call cpu_stats_enable_write, req1->data = 0x%08x \n",req1->data);
			cpu_stats_do_reset_hw(gmacdev, req1->data);
			break;
		}
#else
		TR0("IOCTL_CPU_STATS, NOT SUPPORTED !!!\n");
		retval = -1;
#endif
		break;

	default:
		retval = -1;

}


return retval;
}

/**
 * Function to handle a Tx Hang.
 * This is a software hook (Linux) to handle transmitter hang if any.
 * We get transmitter hang in the device interrupt status, and is handled
 * in ISR. This function is here as a place holder.
 * @param[in] pointer to net_device structure 
 * \return void.
 */
void synopGMAC_linux_tx_timeout (struct net_device *netdev)
{
	synopGMACPciNetworkAdapter *adapter = (synopGMACPciNetworkAdapter *) netdev->priv;
	schedule_work(&adapter->tx_timeout_task);

	TR0("%s called \n",__FUNCTION__);
		return;
}

/**
 * Function to initialize the Linux network interface.
 * 
 * Linux dependent Network interface is setup here. This provides 
 * an example to handle the network dependent functionality.
 *
 * \return Returns 0 on success and Error code on failure.
 */
s32 __init synopGMAC_init_network_interface(void)
{
//moved to prob handler
	return 0;
}


/**
 * Function to initialize the Linux network interface.
 * Linux dependent Network interface is setup here. This provides 
 * an example to handle the network dependent functionality.
 * \return Returns 0 on success and Error code on failure.
 */
void __exit synopGMAC_exit_network_interface(void)
{
	uint i;
#ifdef DO_CPU_STAT
	synopGMACdevice * gmacdev = NULL;
#endif

	TR0("Now Calling network_unregister\n");
    for (i = 0; i < WAVE400_MAX_NET_ADAPTER; i++) {
#if 0
		/************************************************
		* Warning:
		* Do not use the free_irq as it free the irqaction struct,
		* but as it was not allocated by kmalloc, it fails.
		* (due to const param .name in the struct)
		* TODO:
		* If need to support modul driver, do it other way !?
		*/
		plat_free_irq(synopGMACadapter[i]->synopGMACnetdev);
#endif

#ifdef DO_CPU_STAT
		gmacdev = synopGMACadapter[i]->synopGMACdev;
    	CPU_STAT_CLEANUP(&gmacdev->cpu_stat);
#endif

		TR("the synopGMAC interrupt handler has been removed\n");
		unregister_netdev(synopGMACadapter[i]->synopGMACnetdev);	
    }
}

/*
module_init(synopGMAC_init_network_interface);
module_exit(synopGMAC_exit_network_interface);

MODULE_AUTHOR("Synopsys India");
MODULE_LICENSE("GPL/BSD");
MODULE_DESCRIPTION("SYNOPSYS GMAC DRIVER Network INTERFACE");

EXPORT_SYMBOL(synopGMAC_init_bus_interface);
*/
