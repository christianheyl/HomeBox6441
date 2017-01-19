/** \file
 * Header file for the nework dependent functionality.
 * The function prototype listed here are linux dependent.
 *   
 * \internal
 * ---------------------------REVISION HISTORY-------------------
 * Synopsys 			01/Aug/2007		Created
 */
 
 
 
#ifndef SYNOP_GMAC_NETWORK_INTERFACE_H
#define SYNOP_GMAC_NETWORK_INTERFACE_H 1

#define NET_IF_TIMEOUT (10*HZ)
#define CHECK_TIME (HZ)

#ifndef WAVE400_REGISTER_NET
typedef enum
{
  SYNOP_PORT_1,
  SYNOP_PORT_2,
}synopPortId;

#define WAVE400_MAX_NET_ADAPTER 2

#define DEVICE_REG_ADDR_MAC1_START  0xA7040000
#define DEVICE_REG_ADDR_MAC2_START  0xA7180000
#define DEVICE_REG_SIZE        010000
#define DEVICE_REG_ADDR_MAC1_END    DEVICE_REG_ADDR_MAC1_START + DEVICE_REG_SIZE -1
#define DEVICE_REG_ADDR_MAC2_END    DEVICE_REG_ADDR_MAC2_START + DEVICE_REG_SIZE -1

#endif


extern u8 synopGMAC_driver_name[];

extern int irqIn[];
extern int irqOut[];

#ifdef SYNOP_GMAC

static const u32 MAC_BASE_OFFSET = 0x0000;
static const u32 DMA_BASE_OFFSET = 0x1000;

extern void synopGMAC_intr_bottom_half_rx(unsigned long param);
extern void synopGMAC_intr_bottom_half_tx(unsigned long param);
extern s32   synopGMAC_init_network_interface(void);
extern void  synopGMAC_exit_network_interface(void);

extern s32 synopGMAC_linux_open(struct net_device *);
extern s32 synopGMAC_linux_close(struct net_device *);
extern s32 synopGMAC_linux_xmit_frames(struct sk_buff *, struct net_device *);
extern struct net_device_stats * synopGMAC_linux_get_stats(struct net_device *);
extern void synopGMAC_linux_set_multicast_list(struct net_device *);
extern s32 synopGMAC_linux_set_mac_address(struct net_device *,void *);
extern s32 synopGMAC_linux_change_mtu(struct net_device *,s32);
extern s32 synopGMAC_linux_do_ioctl(struct net_device *,struct ifreq *,s32);
extern void synopGMAC_linux_tx_timeout(struct net_device *);

extern /*irqreturn_t*/int synopGMAC_intr_handler(/*s32*/int , void * );
#endif

#endif /* End of file */
