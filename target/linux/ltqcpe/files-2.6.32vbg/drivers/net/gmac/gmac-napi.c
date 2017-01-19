/*
 * linux/arch/arm/mach-oxnas/gmac.c
 *
 * Copyright (C) 2005 Oxford Semiconductor Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/crc32.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/in.h>
#include <net/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <asm/io.h>
#include <linux/platform_device.h>
#include <asm/mips-boards/prom.h>
#include <synopGMAC_network_interface.h>
#include <linux/proc_fs.h> 

#include "wave400_interrupt.h"

#include "gmac.h"
#include "gmac_ethtool.h"
#include "gmac_phy.h"
#include "gmac_desc.h"
#include "gmac_reg.h"
#include "shared.h"
#include "wave400_chadr.h"

//#define VBG400_DEBUG_NET
#ifdef VBG400_DEBUG_NET
#define PRINTK(fmt, args...) do { printk("%s(): ", __func__); printk(fmt, ##args); } while (0)
#else
#define PRINTK(fmt, args...)
#endif


#define MAX_GMAC_UNITS 1
#define ALLOW_AUTONEG
//#define SUPPORT_IPV6

#define NUM_RX_DMA_DESCRIPTORS 256
#define NUM_TX_DMA_DESCRIPTORS 256


#if (((NUM_RX_DMA_DESCRIPTORS) + (NUM_TX_DMA_DESCRIPTORS)) > (NUM_GMAC_DMA_DESCRIPTORS))
#error "GMAC TX+RX descriptors exceed allocation"
#endif
struct proc_dir_entry *gmac_proc = NULL;

#define DESC_SINCE_REFILL_LIMIT ((NUM_RX_DMA_DESCRIPTORS) / 4)
static const u32 MAC_BASE_OFFSET = 0x0000;
static const u32 DMA_BASE_OFFSET = 0x1000;

static const int MIN_PACKET_SIZE = 68;
static const int NORMAL_PACKET_SIZE = 1500;
static const int MAX_JUMBO = 9000;

#define RX_BUFFER_SIZE 796	// Must be multiple of 4, If not defined will size buffer to hold a single MTU-sized packet
static const int EXTRA_RX_SKB_SPACE = 22;	// Ethernet header 14, VLAN 4, CRC 4

// The amount of header to copy from a receive packet into the skb buffer
static const int GMAC_HLEN = 66;

#define GMAC_ALLOC_ORDER 0
static const int GMAC_ALLOC_SIZE = ((1 << GMAC_ALLOC_ORDER) * PAGE_SIZE);

static const u32 AUTO_NEGOTIATION_WAIT_TIMEOUT_MS = 5000;

static const u32 NAPI_POLL_WEIGHT = 16/*64*/;
static const u32 NAPI_OOM_POLL_INTERVAL_MS = 50;

//static const int WATCHDOG_TIMER_INTERVAL = 1500*HZ/1000; -test
static const int WATCHDOG_TIMER_INTERVAL = 500*HZ/1000;

#ifdef VBG400_DEBUG_LEN_ERROR_LOG
static const int LEN_ERR_TIMER_INTERVAL = 5000*HZ/1000;
#endif

//#define AUTO_NEG_MS_WAIT 1500 -test
#define AUTO_NEG_MS_WAIT 500
static const int AUTO_NEG_INTERVAL = (AUTO_NEG_MS_WAIT)*HZ/1000;
//static const int START_RESET_INTERVAL = 500*HZ/1000; -test
static const int START_RESET_INTERVAL = 50*HZ/1000;
static const int RESET_INTERVAL = 10*HZ/1000;
static const int ATOMIC_BUSY_INTERVAL = 50*HZ/1000;

static const int GMAC_RESET_TIMEOUT_MS = 10000;

// This is the IP's phy address. This is unique address for every MAC in the universe
#define DEFAULT_MAC_ADDRESS {0x00, 0x55, 0x7B, 0xB5, 0x7D, 0xF7}

MODULE_AUTHOR("Brian Clarke (Oxford Semiconductor Ltd) & Patched for MIPS Lantiq Wave400 by Yan Yanovsky.");
MODULE_DESCRIPTION("GMAC Network Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("v2.0");

/* Ethernet MAC adr to assign to interface */
static int mac_adr[] = { 0x00, 0x30, 0xe0, 0x00, 0x00, 0x00 };
module_param_array(mac_adr, int, NULL, S_IRUGO);

/* PHY type kernel cmdline options */
static int phy_is_rgmii = 0;
module_param(phy_is_rgmii, int, S_IRUGO);

static int synop_probe(struct platform_device *pdev);
u8 synopGMAC_driver_name[] = "gmac-wd";
EXPORT_SYMBOL(synopGMAC_driver_name);

static struct platform_driver plat_driver = {
	.probe = synop_probe,
	.driver	= {
		.name	= synopGMAC_driver_name,
	},
};

#ifndef CONFIG_VBG400_CHIPIT
/* Below is database that holds the configuration of the 3 NPU Shared IF Registers.
*/

#define _TODO    0   //for compilation, delete after database is completed

typedef struct _gmac_interface {
	u32 gen3_shrd_gmac_mode_reg;
	u32 gen3_shrd_gmac_div_ratio_reg;
	u32 gen3_shrd_gmac_dly_pgm_reg;
}gmac_interface;

typedef struct _gmac_mask {
	u32 gen3_shrd_gmac_mode_reg_mask;
	u32 gen3_shrd_gmac_div_ratio_reg_mask;
	u32 gen3_shrd_gmac_dly_pgm_reg_mask;
}gmac_mask;

/*gmacx_mask_array: x=0/1*/
gmac_mask  gmac0_mask_array[]=
{
	{GMAC_MODE_REG_GMAC0_MASK},     /*gen3_shrd_gmac_mode_reg_mask*/
	{GMAC_DIV_RATIO_REG_GMAC0_MASK},/*gen3_shrd_gmac_div_ratio_reg_mask*/
	{GMAC_DLY_PGM_REG_GMAC0_MASK}   /*gen3_shrd_gmac_dly_pgm_reg_mask*/
};

gmac_mask  gmac1_mask_array[]=
{
	{GMAC_MODE_REG_GMAC1_MASK},     /*gen3_shrd_gmac_mode_reg_mask*/
	{GMAC_DIV_RATIO_REG_GMAC1_MASK},/*gen3_shrd_gmac_div_ratio_reg_mask*/
	{GMAC_DLY_PGM_REG_GMAC1_MASK}   /*gen3_shrd_gmac_dly_pgm_reg_mask*/
};

/*both normal and revered*/
gmac_mask* interface_mask_array[]=
{
    gmac0_mask_array,
    gmac1_mask_array
};

#define _NULL 0

/*mii_array- speeds supported: 10M 100M*/
gmac_interface mii_array[]=
{
               /* gmac_mode_reg,            gmac_div_ratio_reg,               gmac_dly_pgm_reg*/ 
    /*10M*/  { GMAC_MODE_REG_MII_10M_DATA,  GMAC_DIV_RATIO_REG_MII_10M_DATA,  GMAC_DLY_PGM_REG_MII_10M_DATA},
    /*100M*/ { GMAC_MODE_REG_MII_100M_DATA, GMAC_DIV_RATIO_REG_MII_100M_DATA, GMAC_DLY_PGM_REG_MII_100M_DATA},
    /*1000M*/{ _NULL,                       _NULL,                             _NULL}
};

gmac_interface mii_array_gmac1[]=
{
    { GMAC_MODE_REG_MII_10M_GMAC1_DATA,    GMAC_DIV_RATIO_REG_MII_10M_GMAC1_DATA,     GMAC_DLY_PGM_REG_MII_10M_GMAC1_DATA},
    { GMAC_MODE_REG_MII_100M_GMAC1_DATA,   GMAC_DIV_RATIO_REG_MII_100M_GMAC1_DATA,    GMAC_DLY_PGM_REG_MII_100M_GMAC1_DATA},
    { GMAC_MODE_REG_GMII_1000M_GMAC1_DATA, GMAC_DIV_RATIO_REG_GMII_1000M_GMAC1_DATA,  GMAC_DLY_PGM_REG_GMII_1000M_GMAC1_DATA} 
};


gmac_interface  rgmii_array[]=
{
    { GMAC_MODE_REG_RGMII_10M_DATA,   GMAC_DIV_RATIO_REG_RGMII_10M_DATA,   GMAC_DLY_PGM_REG_RGMII_10M_DATA},
    { GMAC_MODE_REG_RGMII_100M_DATA,  GMAC_DIV_RATIO_REG_RGMII_100M_DATA,  GMAC_DLY_PGM_REG_RGMII_100M_DATA},
    { GMAC_MODE_REG_RGMII_1000M_DATA, GMAC_DIV_RATIO_REG_RGMII_1000M_DATA, GMAC_DLY_PGM_REG_RGMII_1000M_DATA} 
};

gmac_interface  rgmii_array_gmac1[]=
{
    { GMAC_MODE_REG_RGMII_10M_GMAC1_DATA,   GMAC_DIV_RATIO_REG_RGMII_10M_GMAC1_DATA,   GMAC_DLY_PGM_REG_RGMII_10M_GMAC1_DATA},
    { GMAC_MODE_REG_RGMII_100M_GMAC1_DATA,  GMAC_DIV_RATIO_REG_RGMII_100M_GMAC1_DATA,  GMAC_DLY_PGM_REG_RGMII_100M_GMAC1_DATA},
    { GMAC_MODE_REG_RGMII_1000M_GMAC1_DATA, GMAC_DIV_RATIO_REG_RGMII_1000M_GMAC1_DATA, GMAC_DLY_PGM_REG_RGMII_1000M_GMAC1_DATA}
};


gmac_interface  rmii_array[]=
{
    { GMAC_MODE_REG_RMII_10M_DATA,  GMAC_DIV_RATIO_REG_RMII_10M_DATA,  GMAC_DLY_PGM_REG_RMII_10M_DATA},
    { GMAC_MODE_REG_RMII_100M_DATA, GMAC_DIV_RATIO_REG_RMII_100M_DATA, GMAC_DLY_PGM_REG_RMII_100M_DATA},
    { NULL, NULL, NULL} 
};

gmac_interface  rmii_array_gmac1[]=
{
    { GMAC_MODE_REG_RMII_10M_GMAC1_DATA,  GMAC_DIV_RATIO_REG_RMII_10M_GMAC1_DATA,  GMAC_DLY_PGM_REG_RMII_10M_GMAC1_DATA},
    { GMAC_MODE_REG_RMII_100M_GMAC1_DATA, GMAC_DIV_RATIO_REG_RMII_100M_GMAC1_DATA, GMAC_DLY_PGM_REG_RMII_100M_GMAC1_DATA},
    { NULL, NULL, NULL}
};

/******************** Reversed ***************************************************
*/

gmac_interface  mii_array_r[]=
{
    { _TODO, _TODO, _TODO},
    { _TODO, _TODO, _TODO},
    { _TODO, _TODO, _TODO} 
};

gmac_interface  mii_array_r_gmac1[]=
{
    { _TODO, _TODO, _TODO},
    { _TODO, _TODO, _TODO},
    { _TODO, _TODO, _TODO}
};

gmac_interface  rgmii_array_r[]=
{
    { _TODO, _TODO, _TODO},
    { _TODO, _TODO, _TODO},
    { _TODO, _TODO, _TODO} 
};

gmac_interface  rgmii_array_r_gmac1[]=
{
    { _TODO, _TODO, _TODO},
    { _TODO, _TODO, _TODO},
    { _TODO, _TODO, _TODO}
};

gmac_interface  rmii_array_r[]=
{
    { _TODO, _TODO, _TODO},
    { _TODO, _TODO, _TODO},
    { _TODO, _TODO, _TODO} 
};

gmac_interface  rmii_array_r_gmac1[]=
{
    { _TODO, _TODO, _TODO},
    { _TODO, _TODO, _TODO},
    { _TODO, _TODO, _TODO}
};


/*interface array:
*/
gmac_interface* interface_mode_array[]=
{
    mii_array, /*ptr to array*/
    rgmii_array,
    rmii_array,
    mii_array_r,
    rgmii_array_r,
    rmii_array_r,
};

gmac_interface* interface_mode_array_gmac1[]=
{
    mii_array_gmac1, /*ptr to array*/
    rgmii_array_gmac1,
    rmii_array_gmac1,
    mii_array_r_gmac1,
    rgmii_array_r_gmac1,
    rmii_array_r_gmac1,
};

enum vbg400_npu_config{
VBG400_PRE_CONFIG,
VBG400_INIT,
VBG400_RUN_TIME
};

enum config_from {
ON_INIT,
ON_THE_FLY
};

extern unsigned int MT_RdReg(unsigned int unit, unsigned int reg);
extern void MT_WrReg(unsigned int unit, unsigned int reg, unsigned int data);
extern void MT_WrRegMask(unsigned int unit, unsigned int reg, unsigned int mask,unsigned int data);

void init_npu_gmac_regs(gmac_priv_t *priv, int interface_mode, int reversed, int gmac_index, enum vbg400_npu_config config);
#if 0
int clock_delay_calibration(int interface_mode, int reversed, int gmac_index, int speed);
void init_npu_gmac_mode_reg(gmac_priv_t *priv, int interface_mode, int reversed, int gmac_index, int speed);
#endif
void get_phy_speed(gmac_priv_t* priv);
static void set_rx_packet_info(struct net_device *dev);
void set_gmac_10_100_giga(gmac_priv_t* priv, u32* reg_data, int interface_mode, enum config_from mode);

//#define VBG400_NPU_RESET_GMAC
#define VBG400_CONFIG_DLY_CALIB
//#define DUMP_REGS_ON_GMAC_UP
//#define TEST_GMAC1_ERROR
#define VBG400_GMAC0_PHY_ID 0
#define VBG400_GMAC1_PHY_ID 5
#define VBG400_GMACS_PHY_ID_MAX 2
#define VBG400_PLL2_WORKAROUND

#endif //CONFIG_VBG400_CHIPIT ***********************************




#define DUMP_REGS
/* if no shared required, also define in wave400_interrupt.h file
*/
//#define VBG400_NO_ETH_SHARED_IRQ
//#define VBG400_TEST_MMC_PMT_GLI_IN_IRQ

#ifdef DUMP_REGS
#define VBG400_SUPPORT_PROC_DUMP
#endif

#ifdef VBG400_SUPPORT_PROC_DUMP
gmac_priv_t *priv_mirror[] = {0,0};
#endif


#define VBG400_USE_TX_NAPI

#ifdef VBG400_USE_TX_NAPI
static int finish_xmit(struct net_device *dev, int budget);
#else
static void finish_xmit(struct net_device *dev);
#endif


static int gmac_up(struct net_device *dev);
static void gmac_down(struct net_device *dev);


#ifdef DUMP_REGS
static void dump_mac_regs(gmac_priv_t *priv, u32 macBase, u32 dmaBase)
{
    int n = 0;
    int i = 0;

    for (n=0; n<0xDC/*0x60*/; n+=4) {
        if (n<0x54 || n>0xBC)
            printk(KERN_INFO "%d MAC Register %08x (%08x) = %08x\n", i, n, macBase+n, readl(macBase+n));
        i++;
    }
    i=0;
    for (n=0; n<0x5C/*0x60*/; n+=4) {
        if (n<0x30 || n>0x44)
            printk(KERN_INFO "%d DMA Register %08x (%08x) = %08x\n", i, n, dmaBase+n, readl(dmaBase+n));
        i++;
    }
    for (n=0; n<0x001f; n+=1) {
        printk(KERN_INFO "%d PHY Register %08x\n",n,phy_read(priv->netdev, priv->phy_addr, n));
    }
}
#endif

static void gmac_int_en_set(
    gmac_priv_t *priv,
    u32          mask)
{
    unsigned long irq_flags = 0;

    spin_lock_irqsave(&priv->cmd_que_lock_, irq_flags);
    dma_reg_set_mask(priv, DMA_INT_ENABLE_REG, mask);
    spin_unlock_irqrestore(&priv->cmd_que_lock_, irq_flags);
}

static void gmac_int_en_clr(
    gmac_priv_t *priv,
    u32          mask,
    u32         *new_value,
    int          in_irq)
{
    unsigned long temp;
    unsigned long irq_flags = 0;

    if (in_irq)
        spin_lock(&priv->cmd_que_lock_);
    else
        spin_lock_irqsave(&priv->cmd_que_lock_, irq_flags);

    temp = dma_reg_clear_mask(priv, DMA_INT_ENABLE_REG, mask);

    if (in_irq)
        spin_unlock(&priv->cmd_que_lock_);
    else
        spin_unlock_irqrestore(&priv->cmd_que_lock_, irq_flags);

    if (new_value) {
        *new_value = temp;
    }
}

/**
 * May be invoked from either ISR or process context
 */
static void change_rx_enable(
    gmac_priv_t *priv,
    u32          start,
    int          waitForAck,
    int          in_irq)
{
    start ? dma_reg_set_mask(priv, DMA_OP_MODE_REG, 1UL << DMA_OP_MODE_SR_BIT) :
            dma_reg_clear_mask(priv, DMA_OP_MODE_REG, 1UL << DMA_OP_MODE_SR_BIT);
}

/**
 * Invoked from the watchdog timer action routine which runs as softirq, so
 * must disable interrupts when obtaining locks
 */
static void change_gig_mode(gmac_priv_t *priv)
{
	unsigned int is_gig = priv->mii.using_1000;
	static const int MAX_TRIES = 1000;
	int tries = 0;
    // Mask to extract the transmit status field from the status register
    u32 ts_mask = ((1UL << DMA_STATUS_TS_NUM_BITS) - 1) << DMA_STATUS_TS_BIT;

    printk(KERN_WARNING "In change_gig_mode\n");
    // Must stop transmission in order to change store&forward mode
    dma_reg_clear_mask(priv, DMA_OP_MODE_REG, (1UL << DMA_OP_MODE_ST_BIT));

    // Transmission only stops after current Tx frame has completed trans-
    // mission, so wait for the Tx state machine to enter the stopped state
    while ((dma_reg_read(priv, DMA_STATUS_REG) & ts_mask) != (DMA_STATUS_TS_STOPPED << DMA_STATUS_TS_BIT)) {
		mdelay(1);
		if (unlikely(++tries == MAX_TRIES)) {
			break;
		}
	}

	if (unlikely(tries == MAX_TRIES)) {
		printk(KERN_WARNING "Timed out of wait for Tx to stop\n");
	}

    if (is_gig) 
        mac_reg_clear_mask(priv, MAC_CONFIG_REG, (1UL << MAC_CONFIG_PS_BIT));
    else 
        mac_reg_set_mask(priv, MAC_CONFIG_REG, (1UL << MAC_CONFIG_PS_BIT));

    // Re-start transmission after store&forward change applied
    dma_reg_set_mask(priv, DMA_OP_MODE_REG, (1UL << DMA_OP_MODE_ST_BIT));
}

/**
 * Invoked from the watchdog timer action routine which runs as softirq, so
 * must disable interrupts when obtaining locks
 */
static void change_pause_mode(gmac_priv_t *priv)
{
	unsigned int enable_pause = priv->mii.using_pause;

    if (enable_pause) {
        mac_reg_set_mask(priv, MAC_FLOW_CNTL_REG, (1UL << MAC_FLOW_CNTL_TFE_BIT));
    } else {
        mac_reg_clear_mask(priv, MAC_FLOW_CNTL_REG, (1UL << MAC_FLOW_CNTL_TFE_BIT));
    }
}

static void refill_rx_ring(struct net_device *dev)
{
    gmac_priv_t *priv = (gmac_priv_t*)netdev_priv(dev);
	int filled = 0;

	if (unlikely(priv->rx_buffers_per_page)) {
		// Receive into pages
		struct page *page = 0;
		int offset = 0;
		dma_addr_t phys_adr = 0;

		// While there are empty RX descriptor ring slots
		while (1) {
			int available;
			int desc;
			rx_frag_info_t frag_info;

			// Have we run out of space in the current page?
			if (offset + NET_IP_ALIGN + priv->rx_buffer_size_ > GMAC_ALLOC_SIZE) {
				page = 0;
				offset = 0;
			}

			if (!page) {
				// Start a new page
				available = available_for_write(&priv->rx_gmac_desc_list_info);
				if (available < priv->rx_buffers_per_page) 
					break;

				// Allocate a page to hold a received packet
				page = alloc_pages(GFP_ATOMIC, GMAC_ALLOC_ORDER);
				if (unlikely(page == NULL)) {
					printk(KERN_WARNING "refill_rx_ring() Could not alloc page\n");
					break;
				}

				// Get a consistent DMA mapping for the entire page that will be
				// DMAed to - causing an invalidation of any entries in the CPU's
				// cache covering the memory region
				phys_adr = dma_map_page(priv->device, page, 0, GMAC_ALLOC_SIZE, DMA_FROM_DEVICE);
				BUG_ON(dma_mapping_error(priv->device, phys_adr));
			} else {
				// Using the current page again
				get_page(page);
			}

			// Ensure IP header is quad aligned
			offset += NET_IP_ALIGN;
			frag_info.page = page;
			frag_info.length = priv->rx_buffer_size_;
			frag_info.phys_adr = phys_adr + offset;

			// Try to associate a descriptor with the fragment info
			desc = set_rx_descriptor(priv, &frag_info);
			if (desc >= 0) 
				filled = 1;
			 else {
				// Failed to associate the descriptor, so release the DMA mapping
				// for the socket buffer
				dma_unmap_page(priv->device, frag_info.phys_adr, frag_info.length, DMA_FROM_DEVICE);
				printk(KERN_INFO "gmac: refill_rx_ring(): failed! dma_unmap_page()!\n");

				// No more RX descriptor ring entries to refill
				break;
			}

			// Account for the space used in the current page
			offset += frag_info.length;

			// Start next packet on a cacheline boundary
			offset = SKB_DATA_ALIGN(offset);
		}
	} else {
		// Preallocate MTU-sized SKBs
		while (1) {
			struct sk_buff *skb;
			rx_frag_info_t frag_info;
			int desc;

			if (!available_for_write(&priv->rx_gmac_desc_list_info)) {
				break;
			}

			// Allocate a new skb for the descriptor ring which is large enough
			// for any packet received from the link
			skb = dev_alloc_skb(priv->rx_buffer_size_ + NET_IP_ALIGN);
			
			if (!skb) {
				// Can't refill any more RX descriptor ring entries
				break;
			} else {
				if(unlikely(skb_is_nonlinear(skb)))	{
					printk(KERN_INFO "gmac: SKB NONLINEAR refill_rx_ring() ********************\n");
					printk(KERN_INFO "gmac: skb_nonlinear(): skb:0x%x skb->data:0x%x skb->len:%d skb->mac_len:%d skb->protocol:%d skb->data_len:%d\n",
					 (unsigned int) skb, (unsigned int) skb->data, skb->len, skb->mac_len, skb->protocol, skb->data_len);
					printk(KERN_INFO "gmac: refill_rx_ring() ******\n\n");
					break;
				}

				// Despite what the comments in the original code from Synopsys
				// claimed, the GMAC DMA can cope with non-quad aligned buffers
				// - it will always perform quad transfers but zero/ignore the
				// unwanted bytes.
				skb_reserve(skb, NET_IP_ALIGN);
			}

			// Get a consistent DMA mapping for the memory to be DMAed to
			// causing invalidation of any entries in the CPU's cache covering
			// the memory region
			frag_info.page = (struct page*)skb;
			frag_info.length = skb_tailroom(skb);
			frag_info.phys_adr = dma_map_single(priv->device, (u32*) skb->data, frag_info.length, DMA_FROM_DEVICE);
			BUG_ON(dma_mapping_error(priv->device, frag_info.phys_adr));
#ifdef GMAC_DEBUG
			printk(KERN_INFO "gmac: refill_rx_ring() ******************************************************\n");
			printk(KERN_INFO "gmac: skb:0x%x skb->data:0x%x skb->tail:0x%x tailroom:%d ***\n",
				skb, skb->data, skb->tail, skb_tailroom(skb));
#endif

			// Associate the skb with the descriptor
			desc = set_rx_descriptor(priv, &frag_info);
			if (desc >= 0) {
				filled = 1;
			} else {
				// No, so release the DMA mapping for the socket buffer
				dma_unmap_single(priv->device, frag_info.phys_adr, frag_info.length, DMA_FROM_DEVICE);
				printk(KERN_INFO "gmac: refill_rx_ring(): failed! dma_unmap_single()!\n");

				// Free the socket buffer
				dev_kfree_skb(skb);

				// No more RX descriptor ring entries to refill
				break;
			}
		}
	}
#ifdef GMAC_DEBUG
	printk(KERN_INFO "gmac: refill_rx_ring() ******************************************************\n\n");
#endif
	if (likely(filled)) {
		// Issue a RX poll demand to restart RX descriptor processing and DFF
		// mode does not automatically restart descriptor processing after a
		// descriptor unavailable event
		dma_reg_write(priv, DMA_RX_POLL_REG, 0);
	}
}

/* Yan
static inline void set_phy_type_rgmii(void)
{
	// Use sysctrl to switch MAC link lines into either (G)MII or RGMII mode
	u32 reg_contents = readl(SYS_CTRL_GMAC_CTRL);
	reg_contents |= (1UL << SYS_CTRL_GMAC_RGMII);
    writel(reg_contents, SYS_CTRL_RSTEN_SET_CTRL);
}
*/

static void do_pre_reset_actions(gmac_priv_t* priv)
{
	switch (priv->phy_type) {
		case PHY_TYPE_LSI_ET1011C:
		case PHY_TYPE_LSI_ET1011C2:
#ifdef CONFIG_VBG400_CHIPIT
		case 0xd565a400/*11G*/:
#endif
			{
				u32 phy_reg;

				printk(KERN_INFO "%s: LSI ET1011C PHY no Rx clk workaround start\n", priv->netdev->name);

				// Enable all digital loopback
				phy_reg = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, ET1011C_MII_LOOPBACK_CNTL);
				phy_reg &= ~(1UL << ET1011C_MII_LOOPBACK_MII_LOOPBACK);
				phy_reg |=  (1UL << ET1011C_MII_LOOPBACK_DIGITAL_LOOPBACK);
				priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, ET1011C_MII_LOOPBACK_CNTL, phy_reg);

				// Disable auto-negotiation and enable loopback
				phy_reg = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, MII_BMCR);
				phy_reg &= ~BMCR_ANENABLE;
				phy_reg |=  BMCR_LOOPBACK;
				priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, MII_BMCR, phy_reg);
			}
			break;
#ifdef CONFIG_VBG400_CHIPIT
		default:
				printk(KERN_INFO "do_pre_reset_actions: %s, doing nothing for PHY ID 0x%08x\n", priv->netdev->name,priv->phy_type);
#endif
	}
}

static void do_post_reset_actions(gmac_priv_t* priv)
{
printk(KERN_INFO "%s: do_post_reset_actions\n", priv->netdev->name);
	switch (priv->phy_type) {
		case PHY_TYPE_LSI_ET1011C:
		case PHY_TYPE_LSI_ET1011C2:
#ifdef CONFIG_VBG400_CHIPIT
		case 0xd565a400/*11G*/:
#endif
			{
				u32 phy_reg;

				printk(KERN_INFO "%s: LSI ET1011C PHY no Rx clk workaround end\n", priv->netdev->name);

				// Disable loopback and enable auto-negotiation
				phy_reg = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, MII_BMCR);
				phy_reg |=  BMCR_ANENABLE;
				phy_reg &= ~BMCR_LOOPBACK;
				priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, MII_BMCR, phy_reg);
			}
			break;
#ifdef CONFIG_VBG400_CHIPIT
		default:
				printk(KERN_INFO "do_post_reset_actions: %s, doing nothing for PHY ID 0x%08x\n", priv->netdev->name,priv->phy_type);
#endif
	}
}

/* Yan */
/*
static struct kobj_type ktype_gmac_link_state = {
	.release = 0,
	.sysfs_ops = 0,
	.default_attrs = 0,
}; 

static int gmac_link_state_hotplug_filter(struct kset* kset, struct kobject* kobj) {
	return get_ktype(kobj) == &ktype_gmac_link_state;
} 

static const char* gmac_link_state_hotplug_name(struct kset* kset, struct kobject* kobj) {
	return "gmac_link_state";
}

static struct kset_uevent_ops gmac_link_state_uevent_ops = {
	.filter = gmac_link_state_hotplug_filter,
	.name   = gmac_link_state_hotplug_name,
	.uevent = NULL,
};*/

static int gmac_link_state_init_sysfs(gmac_priv_t* priv)
{
/*	int err = 0;

	// Prepare the sysfs interface for use 
	kobject_set_name(&priv->link_state_kset.kobj, "gmac_link_state");
// Yan.	priv->link_state_kset.ktype = &ktype_gmac_link_state;

	err = kset_register(&priv->link_state_kset);
	if (err)
		return err;

	// Setup hotplugging 
	priv->link_state_kset.uevent_ops = &gmac_link_state_uevent_ops;

	// Setup the heirarchy, the name will be set on detection 
	kobject_init(&priv->link_state_kobject, &ktype_gmac_link_state);
	priv->link_state_kobject.kset = kset_get(&priv->link_state_kset);
	priv->link_state_kobject.parent = &priv->link_state_kset.kobj;

	// Build the sysfs entry 
	kobject_set_name(&priv->link_state_kobject, "gmac_link_state-1");
	return kobject_add(&priv->link_state_kobject, &priv->link_state_kset.kobj, "%s");*/
	return 0;
}

static void work_handler(struct work_struct *ws) {
	gmac_priv_t *priv = container_of(ws, gmac_priv_t, link_state_change_work);

	kobject_uevent(&priv->link_state_kobject, priv->link_state ? KOBJ_ONLINE : KOBJ_OFFLINE);
    printk("work_handler: priv->link_state=%d, priv->dev_id=%d\n",priv->link_state,priv->dev_id);
}

static void link_state_change_callback(
	int   link_state,
	void *arg)
{
	gmac_priv_t* priv = (gmac_priv_t*)arg;

	priv->link_state = link_state;
	schedule_work(&priv->link_state_change_work);
    printk("link_state_change_callback: schedule_work, priv->dev_id=%d\n",priv->dev_id);
} 

static void start_watchdog_timer(gmac_priv_t* priv)
{
    priv->watchdog_timer.expires = jiffies + WATCHDOG_TIMER_INTERVAL;
    priv->watchdog_timer_shutdown = 0;
    mod_timer(&priv->watchdog_timer, priv->watchdog_timer.expires);
}

#ifdef VBG400_DEBUG_LEN_ERROR_LOG
static void start_len_error_timer(gmac_priv_t* priv)
{
    priv->len_error_timer.expires = jiffies + LEN_ERR_TIMER_INTERVAL;
    mod_timer(&priv->len_error_timer, priv->len_error_timer.expires);
}

static void delete_len_error_timer(gmac_priv_t* priv)
{
    del_timer_sync(&priv->len_error_timer);
}
#endif

static void delete_watchdog_timer(gmac_priv_t* priv)
{
    // Ensure link/PHY state watchdog timer won't be invoked again
    priv->watchdog_timer_shutdown = 1;
    del_timer_sync(&priv->watchdog_timer);
}

static inline int is_auto_negotiation_in_progress(gmac_priv_t* priv)
{
    return !(phy_read(priv->netdev, priv->phy_addr, MII_BMSR) & BMSR_ANEGCOMPLETE);
}

void post_phy_reset(gmac_priv_t *priv)
{
u32 phy_reg;
#define PHY_MIPSCR_TXSKEW_MASK 0x00007700
#define PHY_MIPSCR_TXSKEW_DATA 0x00004400
#define MIIM_PHYCTL2_CLKSEL 0x00000400

    //configure clk select and skew:

    /*read speed as default may be error*/
//	printk(KERN_INFO "post_phy_reset: mii_init_media=1, read speed as default may be error\n");
//    get_phy_speed(priv);
//    printk(KERN_INFO "post_phy_reset: using_10=%d, using_100=%d, using_1000=%d\n",priv->mii.using_10,priv->mii.using_100,priv->mii.using_1000);

    /*clk select:
    * config 125 on gmac0 and force autoneg in order to have change take action (HW limitation due to PLL2 malfunction).
    * if it is gmac0, autoneg would be follow later.
    * in any case also gmac1 set it, for if it would have HW change to connect pin also to gmac1 (in this case can renove gmac0 special handling)
    */
#ifdef VBG400_PLL2_WORKAROUND
    {
        int phy_id;
        int loops = 1; /*do only one loop, change if needed*/
        int i;
        /*gmac0_125_clk - test if gmac0 was configured*/
        int gmac0_125_clk = priv->mii.mdio_read(priv->netdev, VBG400_GMAC0_PHY_ID, MII_NWAYTEST) & MIIM_PHYCTL2_CLKSEL;
        
        if ((priv->dev_id == INDEX_GMAC1) && (gmac0_125_clk == 0 ))
            loops = VBG400_GMACS_PHY_ID_MAX;
        for (i=0; i<loops; i++)
        {
    	    phy_id=VBG400_GMAC0_PHY_ID;
	        if (i == 1/*loops=2*/ || (gmac0_125_clk && priv->dev_id == INDEX_GMAC1))
    	        phy_id=VBG400_GMAC1_PHY_ID;
	        printk(KERN_INFO "post_phy_reset: configure clk select for dev no. %d\n",phy_id);
	        phy_reg = priv->mii.mdio_read(priv->netdev, phy_id, MII_NWAYTEST);
            phy_reg |= MIIM_PHYCTL2_CLKSEL;
	        priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, MII_NWAYTEST, phy_reg);
        }
        /*if gmac0 was not configured - force autoneg on gmac0 in order to have change take action*/
        if (/*priv->dev_id == INDEX_GMAC1 &&*/ gmac0_125_clk == 0)
            priv->mii.mdio_write(priv->netdev, VBG400_GMAC0_PHY_ID, MII_BMCR, BMCR_ANRESTART|BMCR_ANENABLE);

    }
#endif
    //skew:
	printk(KERN_INFO "post_phy_reset: configure skew\n");
	phy_reg = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, MII_RESV1/*PHY_LBR*/);
    phy_reg &= ~(PHY_MIPSCR_TXSKEW_MASK);
    phy_reg |= PHY_MIPSCR_TXSKEW_DATA;
	priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, MII_RESV1/*PHY_LBR*/, phy_reg);
}

void config_phy_skew(gmac_priv_t *priv)
{
u32 phy_reg;
#define PHY_MIPSCR_TXSKEW_MASK 0x00007700
#define PHY_MIPSCR_TXSKEW_DATA 0x00004400
#define MIIM_PHYCTL2_CLKSEL 0x00000400

    /*config speed as default may be error*/
	printk(KERN_INFO "config_phy_skew: mii_init_media=1, read speed as default may be error\n");
#ifndef CONFIG_VBG400_CHIPIT
    get_phy_speed(priv);
#endif
    printk(KERN_INFO "config_phy_skew: using_10=%d, using_100=%d, using_1000=%d\n",priv->mii.using_10,priv->mii.using_100,priv->mii.using_1000);

    //skew:
	printk(KERN_INFO "config_phy_skew: configure skew\n");
	phy_reg = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, MII_RESV1/*PHY_LBR*/);
    phy_reg &= ~(PHY_MIPSCR_TXSKEW_MASK);
    phy_reg |= PHY_MIPSCR_TXSKEW_DATA;
	priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, MII_RESV1/*PHY_LBR*/, phy_reg);
}

#ifdef VBG400_NPU_RESET_GMAC
void restart_gmac(struct work_struct *work)
{
    gmac_priv_t* priv = (gmac_priv_t*)container_of(work, gmac_priv_t, restart_gmac_work);
    int speed;
    int reversed = CONFIG_MAC_MODE_REVERSED;
    int gmac_index = priv->dev_id;
    int status;
	struct net_device *dev = priv->netdev;
    
    printk(KERN_INFO "restart_gmac: speed_changed || gigabit_changed, gmac_index = %d\n",gmac_index);
    /* need to reset and Init GMAC
    */
    gmac_down(dev);

	// Set length etc. of rx packets
	set_rx_packet_info(dev);

    // Reset the PHY to get it into a known state and ensure we have TX/RX
    // clocks to allow the GMAC reset to complete
	printk(KERN_INFO "restart_gmac: call to phy_reset() **\n");
    if (phy_reset(priv->netdev)) {
        printk( KERN_ERR "restart_gmac: %s Failed to reset PHY\n", dev->name);
        status = -EIO;
    } else {
        // Record whether jumbo frames should be enabled
        priv->jumbo_ = (dev->mtu > NORMAL_PACKET_SIZE);

        // Force or auto-negotiate PHY mode
        priv->phy_force_negotiation = 1;

        // Reallocate buffers with new MTU
        gmac_up(dev);
    }
}
#endif

atomic_t watchdog_atomic=ATOMIC_INIT(0);
EXPORT_SYMBOL(watchdog_atomic);

static void watchdog_timer_action(unsigned long arg)
{
    typedef enum watchdog_state {
        WDS_IDLE,
        WDS_RESETTING,
        WDS_NEGOTIATING
    } watchdog_state_t;

    static int state[] = {WDS_IDLE,WDS_IDLE};
    gmac_priv_t* priv = (gmac_priv_t*)arg;
    unsigned long new_timeout = jiffies + WATCHDOG_TIMER_INTERVAL;
    int ready;
    int duplex_changed=0;
    int gigabit_changed=0;
    int pause_changed=0;
#ifndef CONFIG_VBG400_CHIPIT
    int speed;
    int reversed = CONFIG_MAC_MODE_REVERSED;
    unsigned long flags;
    u32 reg_data;
    int interface_mode = CONFIG_VBG400_PHY_INTERFACE_MODE; //interface_mode - 0=mii/gmii, 1=rgmii, 2=rmii 3=mii_r/gmii_r, 4=rgmii_r, 5=rmii_r
	struct net_device *dev = priv->netdev;
    int status;
    int gmac_index = priv->dev_id;
#endif
    int speed_changed=0;

    
#ifdef VBG400_USE_WD_ATOMIC
    atomic_inc(&watchdog_atomic);
    if (atomic_read(&watchdog_atomic) != 1)
    {
        new_timeout = jiffies + ATOMIC_BUSY_INTERVAL;
        printk( KERN_ERR "watchdog_timer_action: Error in watchdog_atomic (%s)\n",priv->netdev->name);
        goto atomic_err_out;
    }
#endif
	// Interpret the PHY/link state.
	if (priv->phy_force_negotiation || (state[priv->dev_id] == WDS_RESETTING)) {
		mii_check_link(&priv->mii);
		ready = 0;
	} else {
		duplex_changed = mii_check_media_ex(&priv->mii, 1, priv->mii_init_media, &speed_changed, &gigabit_changed, &pause_changed, link_state_change_callback, priv);
		priv->mii_init_media = 0;
		ready = netif_carrier_ok(priv->netdev);
        //printk( KERN_INFO "watchdog_timer_action: after mii_check_media_ex, ready=%d state=%d (%s)\n",ready, state[priv->dev_id], priv->netdev->name);
	}

    if (!ready) {
        if (priv->phy_force_negotiation) {
            if (netif_carrier_ok(priv->netdev)) {
                state[priv->dev_id] = WDS_RESETTING;
                printk( KERN_INFO "watchdog_timer_action: change state to WDS_RESETTING (%s)\n",priv->netdev->name);
            } else {
                state[priv->dev_id] = WDS_IDLE;
                printk( KERN_INFO "watchdog_timer_action: change state to WDS_IDLE (%s)\n",priv->netdev->name);
            }

            priv->phy_force_negotiation = 0;
        }

        // May be a good idea to restart everything here, in an attempt to clear
        // out any fault conditions
        if ((state[priv->dev_id] == WDS_NEGOTIATING) && is_auto_negotiation_in_progress(priv)) {
            printk(KERN_INFO"watchdog_timer_action(): WDS_NEGOTIATING && is_auto_negotiation_in_progress (%s)\n",priv->netdev->name);
            new_timeout = jiffies + AUTO_NEG_INTERVAL;
        } else {
            switch (state[priv->dev_id]) {
                case WDS_IDLE:
                    // Reset the PHY to get it into a known state
					printk(KERN_INFO "watchdog_timer_action(): %s, Reset the PHY to get it into a known state\n",priv->netdev->name);
                    start_phy_reset(priv);
#ifndef CONFIG_VBG400_CHIPIT
                    post_phy_reset(priv);
#endif
                    new_timeout = jiffies + START_RESET_INTERVAL;
                    state[priv->dev_id] = WDS_RESETTING;
                    break;
                case WDS_RESETTING:
                    if (!is_phy_reset_complete(priv)) {
                        new_timeout = jiffies + RESET_INTERVAL;
                    } else {
					    printk(KERN_INFO "watchdog_timer_action(): %s, WDS_RESETTING case, reset completed\n",priv->netdev->name);
                        // Force or auto-negotiate PHY mode
//#ifndef CONFIG_VBG400_CHIPIT
//                        printk( KERN_INFO "watchdog_timer_action() %s: call post_phy_reset\n", priv->netdev->name);
//                        post_phy_reset(priv);
//#endif
                        set_phy_negotiate_mode(priv->netdev);
                        state[priv->dev_id] = WDS_NEGOTIATING;
                        new_timeout = jiffies + AUTO_NEG_INTERVAL;
                    }
                    break;
                default:
                    printk( KERN_INFO "watchdog_timer_action() %s: Unexpected state (%d) (%s)\n", priv->netdev->name,state[priv->dev_id],priv->netdev->name);
                    state[priv->dev_id] = WDS_IDLE;
                    break;
            }
        }
    } else {
        state[priv->dev_id] = WDS_IDLE;
        //printk( KERN_INFO "watchdog_timer_action: ready, change state to WDS_IDLE (%s)\n",priv->netdev->name);
        if (duplex_changed) {
            priv->mii.full_duplex ? mac_reg_set_mask(priv,   MAC_CONFIG_REG, (1UL << MAC_CONFIG_DM_BIT)) :
                                    mac_reg_clear_mask(priv, MAC_CONFIG_REG, (1UL << MAC_CONFIG_DM_BIT));
        }

#ifndef CONFIG_VBG400_CHIPIT
        if (speed_changed || gigabit_changed)
        {
            printk(KERN_INFO "watchdog_timer_action: speed_changed || gigabit_changed (%s)\n",priv->netdev->name);
            /* On change MII to GMII and vice versa, reset and Init GMAC */
            init_npu_gmac_regs(priv, interface_mode, reversed, gmac_index, VBG400_RUN_TIME);
            // Initialise the work queue entry to be used to issue hotplug events to userspace
            if (gigabit_changed) {
#ifdef VBG400_NPU_RESET_GMAC
                /* does 100 to 1G and vice versa require reset?
                */
                schedule_work(&priv->restart_gmac_work);
#else
                change_gig_mode(priv);
#endif
            }
            else {
                reg_data=mac_reg_read(priv, MAC_CONFIG_REG);
                set_gmac_10_100_giga(priv, &reg_data, interface_mode, ON_THE_FLY);
            }
        }
#else
        if (gigabit_changed) {
            change_gig_mode(priv);
        }
#endif
		if (pause_changed) {
			change_pause_mode(priv);
		}
    }

    // Re-trigger the timer, unless some other thread has requested it be stopped
    if (!priv->watchdog_timer_shutdown) {
        // Restart the timer
        mod_timer(&priv->watchdog_timer, new_timeout);
    }
#ifdef VBG400_USE_WD_ATOMIC
    atomic_dec(&watchdog_atomic);
#endif
    return;

atomic_err_out:
    printk( KERN_INFO "detect atomic busy\n");
    // Re-trigger the timer, unless some other thread has requested it be stopped
    if (!priv->watchdog_timer_shutdown) {
        // Restart the timer
        mod_timer(&priv->watchdog_timer, new_timeout);
    }
#ifdef VBG400_USE_WD_ATOMIC
    atomic_dec(&watchdog_atomic);
#endif
}

static int inline is_ip_packet(unsigned short eth_protocol)
{
    return (eth_protocol == ETH_P_IP)
#ifdef SUPPORT_IPV6
		|| (eth_protocol == ETH_P_IPV6)
#endif // SUPPORT_IPV6
		;
}

static int inline is_ipv4_packet(unsigned short eth_protocol)
{
    return eth_protocol == ETH_P_IP;
}

#ifdef SUPPORT_IPV6
static int inline is_ipv6_packet(unsigned short eth_protocol)
{
    return eth_protocol == ETH_P_IPV6;
}
#endif // SUPPORT_IPV6

static int inline is_hw_checksummable(unsigned short protocol)
{
    return (protocol == IPPROTO_TCP) || (protocol == IPPROTO_UDP)
           || (protocol == IPPROTO_ICMP);
}

static u32 unmap_rx_page(
	gmac_priv_t *priv,
	dma_addr_t   phys_adr)
{
	u32 offset = phys_adr & ~PAGE_MASK;
	u32 next_offset = offset + priv->rx_buffer_size_;
	next_offset = SKB_DATA_ALIGN(next_offset);
	next_offset += NET_IP_ALIGN;

	// If this is the last packet in a page
	if (next_offset > GMAC_ALLOC_SIZE) {
		// Release the DMA mapping for the page
		dma_unmap_page(priv->device, phys_adr & PAGE_MASK, GMAC_ALLOC_SIZE, DMA_FROM_DEVICE);
	}

	return offset;
}

#define FCS_LEN 4		// Ethernet CRC length

static inline int get_desc_len(
	u32 desc_status,
	int last)
{
	int length = get_rx_length(desc_status);

	if (last) {
		length -= FCS_LEN;
	}

	return length;
}

static int      length_error_print[]={0,0};

static int process_rx_packet_skb(gmac_priv_t *priv)
{
	int             desc;
	int             last;
	u32             desc_status = 0;
	rx_frag_info_t  frag_info;
	int             packet_len;
	struct 			sk_buff *skb = NULL;
	int             valid;
	int             ip_summed;
    int             length_error=0;

#ifdef GMAC_DEBUG
	printk(KERN_INFO "gmac: process_rx_packet_skb() ***********************************\n");
#endif
	desc = get_rx_descriptor(priv, &last, &desc_status, &frag_info);
	if (desc < 0) 
	{
#ifdef GMAC_DEBUG
		printk(KERN_INFO "gmac: process_rx_packet_skb() No more RX desciptors ready!!\n");
		printk(KERN_INFO "gmac: process_rx_packet_skb() ***********************************\n");
#endif
//printk(KERN_INFO "gmac: process_rx_packet_skb() No more RX desciptors ready, priv->dev_id=%d !!\n",priv->dev_id);
		return 0;
	}
	// Release the DMA mapping for the received data
    dma_unmap_single(priv->device, frag_info.phys_adr, frag_info.length, DMA_FROM_DEVICE);

	// Get the packet data length
	packet_len = get_desc_len(desc_status, last);
	if(packet_len > 1522)
	{
		length_error=1;
		length_error_print[priv->dev_id]++;
		goto not_valid_skb;
	}

	// Get pointer to the SKB
	skb = (struct sk_buff*)frag_info.page;
	#ifdef GMAC_DEBUG
		printk(KERN_INFO "gmac: on arrive descriptor!!!! skb:0x%x skb->data:0x%x skb->len:%d skb->mac_len:%d skb->protocol:%d skb->data_len:%d\n",
			 skb, skb->data, skb->len, skb->mac_len, skb->protocol, skb->data_len);
	#endif
	// Is the packet entirely contained within the descriptors and without errors?
	valid = !(desc_status & (1UL << RDES0_ES_BIT));
	if (unlikely(!valid)) {
		#ifdef GMAC_DEBUG
		printk(KERN_INFO "gmac: process_rx_packet_skb() Received packet has bad desc_status: 0x%x not_valid_skb!\n",
			 desc_status);
		#endif
		goto not_valid_skb;
	}
	ip_summed = CHECKSUM_NONE;

#ifdef USE_RX_CSUM
	// Has the h/w flagged an IP header checksum failure?
	valid = !(desc_status & (1UL << RDES0_IPC_BIT));
	if (likely(valid)) {
		// Determine whether Ethernet frame contains an IP packet -
		// only bother with Ethernet II frames, but do cope with
		// 802.1Q VLAN tag presence
		int vlan_offset = 0;
		unsigned short eth_protocol = ntohs(((struct ethhdr*)skb->data)->h_proto);
		int is_ip = is_ip_packet(eth_protocol);
		
		if (!is_ip) {
			// Check for VLAN tag
			if (eth_protocol == ETH_P_8021Q) {
				// Extract the contained protocol type from after
				// the VLAN tag
				eth_protocol = ntohs(*(unsigned short*)(skb->data + ETH_HLEN));
				is_ip = is_ip_packet(eth_protocol);

				// Adjustment required to skip the VLAN stuff and
				// get to the IP header
				vlan_offset = 4;
			}
		}

		// Only offload checksum calculation for IP packets
		if (is_ip) {
			struct iphdr* ipv4_header = 0;
			if (unlikely(desc_status & (1UL << RDES0_PCE_BIT))) 
				valid = 0;
			else if (is_ipv4_packet(eth_protocol)) {
				ipv4_header = (struct iphdr*)(skb->data + ETH_HLEN + vlan_offset);

				// H/W can only checksum non-fragmented IP packets
				if (!(ipv4_header->frag_off & htons(IP_MF | IP_OFFSET))) {
					if (is_hw_checksummable(ipv4_header->protocol))
						ip_summed = CHECKSUM_UNNECESSARY;
				}
			}
			else {
#ifdef SUPPORT_IPV6
				struct ipv6hdr* ipv6_header = (struct ipv6hdr*)(skb->data + ETH_HLEN + vlan_offset);

				if (is_hw_checksummable(ipv6_header->nexthdr)) {
					ip_summed = CHECKSUM_UNNECESSARY;
				}
#endif // SUPPORT_IPV6
			}
		}
	}

	if (unlikely(!valid)) {
		printk(KERN_INFO "gmac: process_rx_packet_skb() a2\n");
		goto not_valid_skb;
	}
#endif // USE_RX_CSUM

	// Increase the skb's data pointer to account for the RX packet that has
	// been DMAed into it
	if(unlikely(skb_is_nonlinear(skb)))
	{
		printk(KERN_INFO "gmac: SKB NONLINEAR ********************\n");
		printk(KERN_INFO "gmac: skb_nonlinear(): packet_len:%d skb:0x%x skb->data:0x%x skb->len:%d skb->mac_len:%d skb->protocol:%d skb->data_len:%d\n",
			 packet_len, (unsigned int) skb, (unsigned int) skb->data, skb->len, skb->mac_len, skb->protocol, skb->data_len);
		printk(KERN_INFO "gmac: process_rx_packet_skb() ******\n\n");
		goto not_valid_skb;
	}

#ifdef GMAC_DEBUG
	if (unlikely(skb->tail > skb->end))
		printk(KERN_INFO "gmac: NOW YOU'LL GET KERNEL PANIC!!!!!!!!!!!!!!!! *******\n");
	printk(KERN_INFO "gmac: process_rx_packet_skb() ******\n\n");
#endif
	skb_put(skb, packet_len);
	// Set the device for the skb
	skb->dev = priv->netdev;

	// Set packet protocol
	skb->protocol = eth_type_trans(skb, priv->netdev);

	// Record whether h/w checksumed the packet
	skb->ip_summed = ip_summed;

	// Send the packet up the network stack
	netif_receive_skb(skb);

	// Update receive statistics
	priv->netdev->last_rx = jiffies;
	++priv->stats.rx_packets;
	priv->stats.rx_bytes += packet_len;

	return 1;

not_valid_skb:
#ifdef GMAC_DEBUG
	printk(KERN_INFO "gmac: not_valid_skb!!!! ****\n");
#endif
//printk(KERN_INFO "gmac: not_valid_skb!!!! ****\n");
	dev_kfree_skb(skb);

	// Update receive statistics from the descriptor status
	if (is_rx_collision_error(desc_status)) {
		#ifdef GMAC_DEBUG
		printk(KERN_INFO "process_rx_packet_skb() %s: Collision (0x%08x:%u bytes)\n",
			 priv->netdev->name, desc_status, packet_len);
		#endif
		
		++priv->stats.collisions;
	}
	if (is_rx_crc_error(desc_status)) {
		#ifdef GMAC_DEBUG
		printk(KERN_INFO "process_rx_packet_skb() %s: CRC error (0x%08x:%u bytes)\n", 
			priv->netdev->name, desc_status, packet_len);
		#endif
		
		++priv->stats.rx_crc_errors;
		++priv->stats.rx_errors;
	}
	if (is_rx_frame_error(desc_status)) {
		#ifdef GMAC_DEBUG
		printk(KERN_INFO "process_rx_packet_skb() %s: frame error (0x%08x:%u bytes)\n", 
			priv->netdev->name, desc_status, packet_len);
		#endif
		++priv->stats.rx_frame_errors;
		++priv->stats.rx_errors;
	}
	if (is_rx_length_error(desc_status)) {
		#ifdef GMAC_DEBUG
		printk(KERN_INFO "process_rx_packet_skb() %s: Length error (0x%08x:%u bytes)\n", 
			priv->netdev->name, desc_status, packet_len);
		#endif
		++priv->stats.rx_length_errors;
		++priv->stats.rx_errors;
	}
	if (is_rx_csum_error(desc_status)) {
		#ifdef GMAC_DEBUG
		printk(KERN_INFO "process_rx_packet_skb() %s: Checksum error (0x%08x:%u bytes)\n",
			priv->netdev->name, desc_status, packet_len);
		#endif
		++priv->stats.rx_frame_errors;
		++priv->stats.rx_errors;
	}
    if (length_error) {
		length_error = 0;
		#ifdef GMAC_DEBUG
		printk(KERN_INFO "process_rx_packet_skb() %s: Length error (0x%08x:%u bytes)\n",
			priv->netdev->name, desc_status, packet_len);
		#endif
		++priv->stats.rx_errors;
#ifdef VBG400_DEBUG_LEN_ERROR
	    return -1;
#else
	    return 0;
#endif
    }

	return 0;
}

static int process_rx_packet(gmac_priv_t *priv)
{
	struct sk_buff *skb = NULL;
	int             last;
	u32             desc_status;
	rx_frag_info_t  frag_info;
	int             desc;
	u32             offset;
	int             desc_len;
	unsigned char  *packet;
	int             valid;
	int             desc_used = 0;
	int             hlen = 0;
	int             partial_len = 0;
	int             first = 1;

#ifdef GMAC_DEBUG
	printk(KERN_INFO "gmac: process_rx_packet()\n");
#endif
	// Check that there is at least one Rx descriptor available. Cache the
	// descriptor information so we don't have to touch the uncached/unbuffered
	// descriptor memory more than necessary when we come to use that descriptor
	if (!rx_available_for_read(&priv->rx_gmac_desc_list_info, &desc_status)) {
		return 0;
	}

	// Attempt to allocate an skb before we change anything on the Rx descriptor ring
	skb = dev_alloc_skb(GMAC_HLEN + NET_IP_ALIGN);
	if (unlikely(skb == NULL)) {
		return 0;
	}

	// Increase the skb's data pointer to account for the RX packet that has
	// been DMAed into it
	if(unlikely(skb_is_nonlinear(skb)))
	{
		printk(KERN_INFO "gmac: SKB NONLINEAR after dev_alloc_skb() ********************\n");
		printk(KERN_INFO "gmac: skb_nonlinear(): skb:0x%x skb->data:0x%x skb->len:%d skb->mac_len:%d skb->protocol:%d skb->data_len:%d\n",
			 (unsigned int) skb, (unsigned int) skb->data, skb->len, skb->mac_len, skb->protocol, skb->data_len);
		return 0;
	}


	// Align IP header start in header storage
	skb_reserve(skb, NET_IP_ALIGN);

	// Process all descriptors associated with the packet
	while (1) {
		int prev_len;

		// First call to get_rx_descriptor() will use the status read from the
		// first descriptor by the call to rx_available_for_read() above
		while ((desc = get_rx_descriptor(priv, &last, &desc_status, &frag_info)) < 0) {
			// We are part way through processing a multi-descriptor packet
			// and the GMAC hasn't finished with the next descriptor for the
			// packet yet, so have to poll until it becomes available
			desc_status = 0;
			udelay(1);
		}

		// We've consumed a descriptor
		++desc_used;

		if (!frag_info.page) {
			panic("process_rx_packet() %s: Found RX descriptor without attached page\n", priv->netdev->name);
		}

		// If this is the last packet in the page, release the DMA mapping
		offset = unmap_rx_page(priv, frag_info.phys_adr);
		if (!first) {
			// The buffer adr of descriptors associate with middle or last
			// parts of a packet have ls 2 bits of buffer adr ignored by GMAC DMA
			offset &= ~0x3;
		}

		// Get the length of the packet excluding CRC, h/w csum etc.
		prev_len = partial_len;
		partial_len = get_desc_len(desc_status, last);
		desc_len = partial_len - prev_len;

		// Get a pointer to the start of the packet data received into page
		packet = page_address(frag_info.page) + offset;

		// Is the packet entirely contained within the desciptors and without errors?
		valid = !(desc_status & (1UL << RDES0_ES_BIT));

		if (unlikely(!valid)) {
			goto not_valid;
		}

		if (first) {
			// Store headers in skb buffer
			hlen = min(GMAC_HLEN, desc_len);

			// Copy header into skb buffer
			memcpy(skb->data, packet, hlen);
			skb->tail += hlen;

			if (desc_len > hlen) {
				// Point skb frags array at remaining packet data in pages
				skb_shinfo(skb)->nr_frags = 1;
				skb_shinfo(skb)->frags[0].page = frag_info.page;
				skb_shinfo(skb)->frags[0].page_offset = offset + hlen;
				skb_shinfo(skb)->frags[0].size = desc_len - hlen;
			} else {
				// Entire packet now in skb buffer so don't require page anymore
				put_page(frag_info.page);
			}

			first = 0;
		} else {
			// Store intermediate descriptor data into packet
			int frag_index = skb_shinfo(skb)->nr_frags;
			skb_shinfo(skb)->frags[frag_index].page = frag_info.page;
			skb_shinfo(skb)->frags[frag_index].page_offset = offset;
			skb_shinfo(skb)->frags[frag_index].size = desc_len;
			++skb_shinfo(skb)->nr_frags;
		}

		if (last) {
			int ip_summed = CHECKSUM_NONE;

			// Update total packet length skb metadata
			skb->len = partial_len;
			skb->data_len = skb->len - hlen;
			skb->truesize = skb->len + sizeof(struct sk_buff);

#ifdef USE_RX_CSUM
			// Has the h/w flagged an IP header checksum failure?
			valid = !(desc_status & (1UL << RDES0_IPC_BIT));

			// Are we offloading RX checksuming?
			if (likely(valid)) {
				// Determine whether Ethernet frame contains an IP packet -
				// only bother with Ethernet II frames, but do cope with
				// 802.1Q VLAN tag presence
				int vlan_offset = 0;
				unsigned short eth_protocol = ntohs(((struct ethhdr*)skb->data)->h_proto);
				int is_ip = is_ip_packet(eth_protocol);

				if (!is_ip) {
					// Check for VLAN tag
					if (eth_protocol == ETH_P_8021Q) {
						// Extract the contained protocol type from after
						// the VLAN tag
						eth_protocol = ntohs(*(unsigned short*)(skb->data + ETH_HLEN));
						is_ip = is_ip_packet(eth_protocol);

						// Adjustment required to skip the VLAN stuff and
						// get to the IP header
						vlan_offset = 4;
					}
				}

				// Only offload checksum calculation for IP packets
				if (is_ip) {
					struct iphdr* ipv4_header = 0;
					if (unlikely(desc_status & (1UL << RDES0_PCE_BIT))) 
						valid = 0;
					else if (is_ipv4_packet(eth_protocol)) {
						ipv4_header = (struct iphdr*)(skb->data + ETH_HLEN + vlan_offset);

						// H/W can only checksum non-fragmented IP packets
						if (!(ipv4_header->frag_off & htons(IP_MF | IP_OFFSET))) {
							if (is_hw_checksummable(ipv4_header->protocol)) 
								ip_summed = CHECKSUM_UNNECESSARY;
						}
					}
					else {
#ifdef SUPPORT_IPV6
						struct ipv6hdr* ipv6_header = (struct ipv6hdr*)(skb->data + ETH_HLEN + vlan_offset);

						if (is_hw_checksummable(ipv6_header->nexthdr)) 
							ip_summed = CHECKSUM_UNNECESSARY;
#endif // SUPPORT_IPV6
					}
				}
			}

			if (unlikely(!valid)) 
				goto not_valid;
#endif // USE_RX_CSUM

			// Initialise other required skb header fields
			skb->dev = priv->netdev;
			skb->protocol = eth_type_trans(skb, priv->netdev);

			// Record whether h/w checksumed the packet
			skb->ip_summed = ip_summed;

			// Send the skb up the network stack
			netif_receive_skb(skb);

			// Update receive statistics
			priv->netdev->last_rx = jiffies;
			++priv->stats.rx_packets;
			priv->stats.rx_bytes += partial_len;

			break;
		}

		// Want next call to get_rx_descriptor() to read status from descriptor
		desc_status = 0;
	}
    return desc_used;

not_valid:
	if (!skb_shinfo(skb)->nr_frags) {
		// Free the page as it wasn't attached to the skb
		put_page(frag_info.page);
	}

	dev_kfree_skb(skb);

	printk( KERN_WARNING "process_rx_packet() %s: Received packet has bad desc_status = 0x%08x\n", priv->netdev->name, desc_status);

	// Update receive statistics from the descriptor status
	if (is_rx_collision_error(desc_status)) {
		printk(KERN_INFO "process_rx_packet() %s: Collision (0x%08x:%u bytes)\n", priv->netdev->name, desc_status, desc_len);
		++priv->stats.collisions;
	}
	if (is_rx_crc_error(desc_status)) {
		printk(KERN_INFO "process_rx_packet() %s: CRC error (0x%08x:%u bytes)\n", priv->netdev->name, desc_status, desc_len);
		++priv->stats.rx_crc_errors;
		++priv->stats.rx_errors;
	}
	if (is_rx_frame_error(desc_status)) {
		printk(KERN_INFO "process_rx_packet() %s: frame error (0x%08x:%u bytes)\n", priv->netdev->name, desc_status, desc_len);
		++priv->stats.rx_frame_errors;
		++priv->stats.rx_errors;
	}
	if (is_rx_length_error(desc_status)) {
		printk(KERN_INFO "process_rx_packet() %s: Length error (0x%08x:%u bytes)\n", priv->netdev->name, desc_status, desc_len);
		++priv->stats.rx_length_errors;
		++priv->stats.rx_errors;
	}
	if (is_rx_csum_error(desc_status)) {
		printk(KERN_INFO "process_rx_packet() %s: Checksum error (0x%08x:%u bytes)\n", priv->netdev->name, desc_status, desc_len);
		++priv->stats.rx_frame_errors;
		++priv->stats.rx_errors;
	}

	return desc_used;
}

#ifdef VBG400_USE_TX_NAPI
static int poll(
	struct napi_struct *napi,
	int                 budget)
{
	gmac_priv_t *priv = NULL;
	struct net_device *dev = NULL;
    int tx_clean_complete = 0, work_done = 0;

	priv = container_of(napi, gmac_priv_t, napi_struct);
	dev = priv->netdev;

    /*Call Tx handling
    */
    tx_clean_complete = finish_xmit(dev, budget);

    /*Call Rx handling
    */
    priv->clean_rx(dev, &work_done, budget);

    if (tx_clean_complete == 0) {
        /*Not finished */
        work_done = budget;
    }
    if (work_done < budget)
    {
        /*Finished, stop NAPI and enable interrupts*/
        napi_complete(napi);
        // Enable interrupts caused by received packets that may have been
        // disabled in the ISR before entering polled mode
        gmac_int_en_set(priv, (1UL << DMA_INT_ENABLE_RI_BIT) |
                              (1UL << DMA_INT_ENABLE_RU_BIT) |
                              (1UL << DMA_INT_ENABLE_OV_BIT) |
                              (1UL << DMA_INT_ENABLE_TI_BIT) |
                              (1UL << DMA_INT_ENABLE_ETE_BIT) |
                              (1UL << DMA_STATUS_TJT_BIT) |
                              (1UL << DMA_STATUS_UNF_BIT));
    }
    return work_done;
}
#endif

#ifdef VBG400_DEBUG_LEN_ERROR_LOG
static void len_error_action(unsigned long arg)
{
    unsigned long new_timeout = jiffies + LEN_ERR_TIMER_INTERVAL;
    gmac_priv_t* priv = (gmac_priv_t*)arg;

    printk("length_error_print[%d]=%d\n",priv->dev_id, length_error_print[priv->dev_id]);
    if (!priv->watchdog_timer_shutdown) {
        // Restart the timer
        mod_timer(&priv->len_error_timer, new_timeout);
    }
}
#endif

#ifdef VBG400_USE_TX_NAPI
static int poll_rx(struct net_device *dev, int *work_done, int budget)
{
    gmac_priv_t *priv = (gmac_priv_t*)netdev_priv(dev);
#else
static int poll(
	struct napi_struct *napi,
	int                 budget)
{
	gmac_priv_t *priv = NULL;
	struct net_device *dev = NULL;
    int work_done = 0;
#endif
    int continue_polling = 0;
    int rx_work_limit = budget;
    int finished = 0;
    int available = 0;
	int desc_since_refill = 0;
#ifndef VBG400_USE_TX_NAPI
	priv = container_of(napi, gmac_priv_t, napi_struct);
	dev = priv->netdev;
#endif


    finished = 0;
    do {
        u32 status;

        // While there are receive polling jobs to be done
        while (rx_work_limit) {
			int desc_used;

			if (unlikely(priv->rx_buffers_per_page)) {
#ifdef GMAC_DEBUG
				printk(KERN_INFO "gmac: poll. call to process_rx_packet() **\n");
#endif
				desc_used = process_rx_packet(priv);
			} else {
				desc_used = process_rx_packet_skb(priv);
			}
#ifdef VBG400_DEBUG_LEN_ERROR
			if (desc_used==0) {
				break;
			}
#else
			if (!desc_used) {
				break;
			}
#endif

            // Increment count of processed packets
#ifdef VBG400_USE_TX_NAPI
            ++*work_done;
#else
            ++work_done;
#endif
            // Decrement our remaining budget
            if (rx_work_limit > 0) {
                --rx_work_limit;
            }

            // Rx overflows seem to upset the GMAC, so try to ensure we never see them
			desc_since_refill += desc_used;
            if (desc_since_refill >= DESC_SINCE_REFILL_LIMIT) {
                desc_since_refill = 0;
                refill_rx_ring(dev);
            }
        }

        if (rx_work_limit) {
            // We have unused budget remaining, but apparently no Rx packets to
            // process
            available = 0;

            // Clear any RI status so we don't immediately get reinterrupted
            // when we leave polling, due to either a new RI event, or a left
            // over interrupt from one of the RX descriptors we've already
            // processed
            status = dma_reg_read(priv, DMA_STATUS_REG);
            if (status & (1UL << DMA_STATUS_RI_BIT)) {
                // Ack the RI, including the normal summary sticky bit
                dma_reg_write(priv, DMA_STATUS_REG, ((1UL << DMA_STATUS_RI_BIT)  |
                                                     (1UL << DMA_STATUS_NIS_BIT)));

                // Must check again for available RX descriptors, in case the RI
                // status came from a new RX descriptor
                available = rx_available_for_read(&priv->rx_gmac_desc_list_info, 0);
            }
            if (!available) {
                // We have budget left but no Rx packets to process so stop
                // polling
                continue_polling = 0;
                finished = 1;
            }
        } else {
            // If we have consumed all our budget, don't cancel the
            // poll, the NAPI instructure assumes we won't
            // Must leave poll() routine as no budget left
            continue_polling = 1;
            finished = 1;
        }
    } while (!finished);

    // Attempt to fill any empty slots in the RX ring
    refill_rx_ring(dev);

    // Decrement the budget even if we didn't process any packets
#ifdef VBG400_USE_TX_NAPI
    if (!*work_done)
        *work_done = 1;
    return *work_done;
#else
    if (!work_done)
        work_done = 1;
    if (!continue_polling) {
        // No more received packets to process so return to interrupt mode
        //netif_rx_complete(dev, napi);
        napi_complete(napi);
        // Enable interrupts caused by received packets that may have been
        // disabled in the ISR before entering polled mode
        gmac_int_en_set(priv, (1UL << DMA_INT_ENABLE_RI_BIT) |
                              (1UL << DMA_INT_ENABLE_RU_BIT) |
                              (1UL << DMA_INT_ENABLE_OV_BIT));
    }
    return work_done;
#endif
}


#ifdef VBG400_USE_TX_NAPI
static int finish_xmit(struct net_device *dev, int budget)
{
    unsigned long           irq_flags;
#else
static void finish_xmit(struct net_device *dev)
{
    int budget=100;
#endif
    gmac_priv_t *priv = (gmac_priv_t*)netdev_priv(dev);
    unsigned     descriptors_freed = 0;
    u32          desc_status = 0;
    int tx_packets_max_limit=0;

    // Handle transmit descriptors for the completed packet transmission
    while (1) {
        struct sk_buff *skb;
        tx_frag_info_t  fragment;
        int             buffer_owned;
		int				 desc_index;

         // Get tx descriptor content, accumulating status for all buffers
        // contributing to each packet
#ifdef VBG400_USE_TX_NAPI
		if (tx_packets_max_limit >= budget) {
			break;
		}
//		spin_lock_irqsave(&priv->tx_spinlock_, irq_flags);
		desc_index = get_tx_descriptor(priv, &skb, &desc_status, &fragment, &buffer_owned);
//		spin_unlock_irqrestore(&priv->tx_spinlock_, irq_flags);
#else
		spin_lock(&priv->tx_spinlock_);
		desc_index = get_tx_descriptor(priv, &skb, &desc_status, &fragment, &buffer_owned);
		spin_unlock(&priv->tx_spinlock_);
#endif
		if (desc_index < 0) {
			// No more completed Tx packets
#ifdef GMAC_DEBUG
			printk(KERN_INFO "gmac: finish_xmit(): No more ready Tx packets!!\n");
#endif
			break;
		}

        // Only unmap DMA buffer if descriptor owned the buffer
        if (buffer_owned) {
            // Release the DMA mapping for the buffer
#ifdef GMAC_DEBUG
			printk(KERN_INFO "gmac: finish_xmit(): unmap_dma: addr:0x%x length:%d\n",
				fragment.phys_adr, fragment.length);
#endif
            dma_unmap_single(priv->device, fragment.phys_adr, fragment.length, DMA_TO_DEVICE);
        }

        // When all buffers contributing to a packet have been processed
        if (skb) {
            // Check the status of the transmission
            if (likely(is_tx_valid(desc_status))) {
                priv->stats.tx_bytes += skb->len;
                priv->stats.tx_packets++;
                tx_packets_max_limit++;
            } else {
                priv->stats.tx_errors++;
                if (is_tx_aborted(desc_status)) {
                    ++priv->stats.tx_aborted_errors;
                }
                if (is_tx_carrier_error(desc_status)) {
                    ++priv->stats.tx_carrier_errors;
                }
            }

            if (unlikely(is_tx_collision_error(desc_status))) {
                ++priv->stats.collisions;
            }

            // Inform the network stack that packet transmission has finished
            dev_kfree_skb_irq(skb);

            // Start accumulating status for the next packet
            desc_status = 0;
        }

        // Track how many descriptors we make available, so we know
        // if we need to re-start of network stack's TX queue processing
        ++descriptors_freed;
    }

    // If the TX queue is stopped, there may be a pending TX packet waiting to
    // be transmitted
    if (unlikely(netif_queue_stopped(dev))) {
		// No locking with hard_start_xmit() required, as queue is already
		// stopped so hard_start_xmit() won't touch the h/w

        // If any TX descriptors have been freed and there is an outstanding TX
        // packet waiting to be queued due to there not having been a TX
        // descriptor available when hard_start_xmit() was presented with an skb
        // by the network stack
        if (priv->tx_pending_skb) {
            // Construct the GMAC specific DMA descriptor
            if (set_tx_descriptor(priv,
                                  priv->tx_pending_skb,
                                  priv->tx_pending_fragments,
                                  priv->tx_pending_fragment_count,
                                  priv->tx_pending_skb->ip_summed == CHECKSUM_PARTIAL) >= 0) {
                // No TX packets now outstanding
                priv->tx_pending_skb = 0;
                priv->tx_pending_fragment_count = 0;

                // We have used one of the TX descriptors freed by transmission
                // completion processing having occured above
                --descriptors_freed;

                // Issue a TX poll demand to restart TX descriptor processing, as we
                // have just added one, in case it had found there were no more
                // pending transmission
#ifdef VBG400_USE_TX_NAPI
		        //spin_lock_irqsave(&priv->tx_spinlock_, irq_flags);
                dma_reg_write(priv, DMA_TX_POLL_REG, 0);
		        //spin_unlock_irqrestore(&priv->tx_spinlock_, irq_flags);
#else
                dma_reg_write(priv, DMA_TX_POLL_REG, 0);
#endif
            }
        }

        // If there are TX descriptors available we should restart the TX queue
        if (descriptors_freed) {
            // The TX queue had been stopped by hard_start_xmit() due to lack of
            // TX descriptors, so restart it now that we've freed at least one
#ifdef GMAC_DEBUG
			printk(KERN_INFO "gmac: finish_xmit() wakeup the netif queue!\n");
#endif
            netif_wake_queue(dev);
        }
    }
#ifdef VBG400_USE_TX_NAPI
    /*Note: if exactly number of empty descriptors as budget, idle call to NAPI result*/
    return ((tx_packets_max_limit >= budget) ? 0 : 1);
#endif
}

static void process_non_dma_ints(gmac_priv_t *priv, u32 raw_status)
{
    u32 reg_contents;

//	printk(KERN_ERR "Found GPI/GMI/GLI interrupt: 0x%08x\n",raw_status);
    // Clear any pending interrupt by read GMAC status reg
    reg_contents = mac_reg_read(priv, MAC_RGMII_STATUS_REG);

    reg_contents = ((1UL << DMA_STATUS_GPI_BIT) |
                   (1UL << DMA_STATUS_GMI_BIT) |
                   (1UL << DMA_STATUS_GLI_BIT));
	printk(KERN_ERR "process_non_dma_ints: clear DMA_STATUS_REG 0x%08x\n",raw_status);
    dma_reg_write(priv, DMA_STATUS_REG, reg_contents);
//    dma_reg_write(priv, DMA_STATUS_REG, dma_reg_read(priv, DMA_STATUS_REG));
}

irqreturn_t synopGMAC_intr_handler(int intr_num, void *dev_id)
{
    struct net_device *dev = NULL;
    gmac_priv_t* priv = NULL;
    u32 int_enable;
    int rx_polling;
    u32 raw_status;
    u32 status;
    static int done_gpi_gmi_gli[] = {0,0};
	
	dev = (struct net_device *)dev_id; 
	if(!dev) {
		return -1;
    }
	priv = (gmac_priv_t*)netdev_priv(dev);
	if(!priv) {
		return -1;
    }

    /** Read the interrupt enable register to determine if we're in rx poll mode
     *  Id like to get rid of this read, if a more efficient way of determining
     *  whether we are polling is available */
    spin_lock(&priv->cmd_que_lock_);
    int_enable = dma_reg_read(priv, DMA_INT_ENABLE_REG);
    spin_unlock(&priv->cmd_que_lock_);

//    rx_polling = !(int_enable & (1UL << DMA_INT_ENABLE_RI_BIT));

    // Get interrupt status
    raw_status = dma_reg_read(priv, DMA_STATUS_REG);

/*********************** TEST ***************************************************/
#if 0
    if ((raw_status == (1UL << DMA_STATUS_GPI_BIT)) ||
        (raw_status == (1UL << DMA_STATUS_GMI_BIT)) ||
        (raw_status == (1UL << DMA_STATUS_GLI_BIT))) {
        printk("synopGMAC_intr_handler: GMAC Interrupts only !!!!!! (0x%08x), int_enable = 0x%08x, id = %d\n",raw_status,int_enable,priv->dev_id);
    }
#endif
/*********************** TEST END ***********************************************/


#ifdef VBG400_TEST_MMC_PMT_GLI_IN_IRQ
    /* Must test before "& int_enable",
       MMC, PMT and GLI interrupts are not masked by the interrupt enable register,
       so must deal with them on the raw status */
    if (unlikely( !done_gpi_gmi_gli[priv->dev_id] ))
        printk("synopGMAC_intr_handler: unlikely \n");
       if (raw_status & ((1UL << DMA_STATUS_GPI_BIT) |
                      (1UL << DMA_STATUS_GMI_BIT) |
                      (1UL << DMA_STATUS_GLI_BIT))) {
        process_non_dma_ints(priv, raw_status);
        status = raw_status & int_enable;
        if (!status) {
            printk("synopGMAC_intr_handler: !status %d\n",priv->dev_id);
            done_gpi_gmi_gli[priv->dev_id] = 1;
        }
    }
#endif
    // Get status of enabled interrupt sources
    status = raw_status & int_enable;

#ifndef VBG400_NO_ETH_SHARED_IRQ
    if (!status) {
        return IRQ_NONE;
    }
#endif

    while (status) {
        // Whether the link/PHY watchdog timer should be restarted
        int restart_watchdog = 0;
        int restart_tx       = 0;
        int poll_tx          = 0;
        u32 int_disable_mask = 0;

        // Test for RX interrupt resulting from sucessful reception of a packet-
        // must do this before ack'ing, else otherwise can get into trouble with
        // the sticky summary bits when we try to disable further RI interrupts
        if (status & (1UL << DMA_STATUS_RI_BIT)
#ifdef VBG400_USE_TX_NAPI
                      |
                      (1UL << DMA_STATUS_TI_BIT) |
                      (1UL << DMA_STATUS_ETI_BIT) |
                      (1UL << DMA_STATUS_TJT_BIT) |
                      (1UL << DMA_STATUS_UNF_BIT)) {
#else
                      ) {
#endif
            #ifdef GMAC_DEBUG
            printk(KERN_INFO "gmac: RX int!!!!\n");
            #endif
            
            // Disable interrupts caused by received packets as henceforth
            // we shall poll for packet reception
            int_disable_mask |= (1UL << DMA_INT_ENABLE_RI_BIT);
#ifdef VBG400_USE_TX_NAPI
            int_disable_mask |= (1UL << DMA_INT_ENABLE_TI_BIT);
            int_disable_mask |= (1UL << DMA_INT_ENABLE_ETE_BIT);
            int_disable_mask |= (1UL << DMA_STATUS_TJT_BIT);
            int_disable_mask |= (1UL << DMA_STATUS_UNF_BIT);
#endif

            // Do NAPI compatible receive processing for RI interrupts
            if (likely(napi_schedule_prep(&priv->napi_struct))) {
                // Remember that we are polling, so we ignore RX events for the
                // remainder of the ISR
                #ifdef GMAC_DEBUG
                printk(KERN_INFO "gmac: napi_schedule()!! **\n");
                #endif
                rx_polling = 1;

                // Tell system we have work to be done
                __napi_schedule(&priv->napi_struct);
            }
#if 0
            else
                printk(KERN_ERR "int_handler() %s: RX interrupt while in poll\n", dev->name);
#endif
        }

#ifdef VBG400_USE_TX_NAPI
        /* Save time - test all, if error continue
        */
        if (unlikely(status & (1UL << DMA_STATUS_RU_BIT) |
                      (1UL << DMA_STATUS_OVF_BIT) |
                      (1UL << DMA_STATUS_TJT_BIT) |
                      (1UL << DMA_STATUS_UNF_BIT))) {
#endif
            // Test for unavailable RX buffers - must do this before ack'ing, else
            // otherwise can get into trouble with the sticky summary bits
            if (unlikely(status & (1UL << DMA_STATUS_RU_BIT))) {
			    u32	val = 0;
                // Accumulate receive statistics
                ++priv->stats.rx_over_errors;
                ++priv->stats.rx_errors;

                // Disable RX buffer unavailable reporting, so we don't get swamped
                int_disable_mask |= (1UL << DMA_INT_ENABLE_RU_BIT);

			    PRINTK(KERN_INFO "gmac: No available RX descriptors!\n");

			    /* Flow control. Start sending Pause Control frames. */
			    val = mac_reg_read(priv, MAC_FLOW_CNTL_REG);
			    if(!(val & 0x1)) {
				    PRINTK(KERN_INFO "gmac: Sending Pause Control frames!\n");
				    val |= 0x1; 
				    mac_reg_write(priv, MAC_FLOW_CNTL_REG, val);
			    }
            }

		    if (unlikely(status & (1UL << DMA_STATUS_OVF_BIT))) {
			    u32	val = 0;

			    PRINTK(KERN_INFO "int_handler() %s: RX overflow\n", dev->name);
			    // Accumulate receive statistics
			    ++priv->stats.rx_fifo_errors;
			    ++priv->stats.rx_errors;

			    /* Flow control. Start sending Pause Control frames. */
			    val = mac_reg_read(priv, MAC_FLOW_CNTL_REG);
			    if(!(val & 0x1)) {
				    PRINTK(KERN_INFO "gmac: Start sending Pause Control frames!\n");
				    val |= 0x1; 
				    mac_reg_write(priv, MAC_FLOW_CNTL_REG, val);
			    }
                // Disable RX overflow reporting, so we don't get swamped
                int_disable_mask |= (1UL << DMA_INT_ENABLE_OV_BIT);
		    }
#ifdef VBG400_USE_TX_NAPI
            if (unlikely(status & (1UL << DMA_STATUS_TJT_BIT))) {
                // A transmit jabber timeout causes the transmitter to enter the
                // stopped state
                printk(KERN_INFO "int_handler() %s: TX jabber timeout\n", dev->name);
                restart_tx = 1;
                poll_tx = 1;
            }
		    if (unlikely(status & (1UL << DMA_STATUS_UNF_BIT))) {
                printk(KERN_INFO "int_handler() %s: TX underflow\n", dev->name);
                poll_tx = 1;
            }
#endif
#ifdef VBG400_USE_TX_NAPI
        }
#endif
        // Do any interrupt disabling with a single register write
        if (int_disable_mask) {
            gmac_int_en_clr(priv, int_disable_mask, 0, 1);

            // Update our record of the current interrupt enable status
            int_enable &= ~int_disable_mask;
        }

        // The broken GMAC interrupt mechanism with its sticky summary bits
        // means that we have to ack all asserted interrupts here; we can't not
        // ack the RI interrupt source as we might like to (in order that the
        // poll() routine could examine the status) because if it was asserted
        // prior to being masked above, then the summary bit(s) would remain
        // asserted and cause an immediate re-interrupt.
        dma_reg_write(priv, DMA_STATUS_REG, status | ((1UL << DMA_STATUS_NIS_BIT) |
                                                      (1UL << DMA_STATUS_AIS_BIT)));
#ifndef VBG400_USE_TX_NAPI
        // Test for normal TX interrupt
        if (status & ((1UL << DMA_STATUS_TI_BIT) |
                      (1UL << DMA_STATUS_ETI_BIT))) {
            // Finish packet transmision started by start_xmit
            finish_xmit(dev);
        }

        // Test for abnormal transmitter interrupt where there may be completed
        // packets waiting to be processed
        if (unlikely(status & ((1UL << DMA_STATUS_TJT_BIT) |
                               (1UL << DMA_STATUS_UNF_BIT)))) {
            // Complete processing of any TX packets closed by the DMA
			printk(KERN_INFO "gmac: TX call to finish_xmit() Abnormal TX***\n");
            
            finish_xmit(dev);
            
            if (status & (1UL << DMA_STATUS_TJT_BIT)) {
                // A transmit jabber timeout causes the transmitter to enter the
                // stopped state
                printk(KERN_INFO "int_handler() %s: TX jabber timeout\n", dev->name);
                restart_tx = 1;
            } else {
                printk(KERN_INFO "int_handler() %s: TX underflow\n", dev->name);
            }

            // Issue a TX poll demand in an attempt to restart TX descriptor
            // processing
            poll_tx = 1;
        }
#endif //#ifndef VBG400_USE_TX_NAPI*********************************

        // Test for any of the error states which we deal with directly within
        // this interrupt service routine. Also for any other interrupts.
        if (unlikely(status & ((1UL << DMA_STATUS_ERI_BIT) |
                               (1UL << DMA_STATUS_RWT_BIT) |
                               (1UL << DMA_STATUS_RPS_BIT) |
                               (1UL << DMA_STATUS_TPS_BIT) |
                               (1UL << DMA_STATUS_FBE_BIT)
                               ))) {
            // Test for early RX interrupt
            if (status & (1UL << DMA_STATUS_ERI_BIT)) {
                // Don't expect to see this, as never enable it
                printk(KERN_WARNING "int_handler() %s: Early RX \n", dev->name);
            }

            if (status & (1UL << DMA_STATUS_RWT_BIT)) {
                printk(KERN_INFO "int_handler() %s: RX watchdog timeout\n", dev->name);
                // Accumulate receive statistics
                ++priv->stats.rx_frame_errors;
                ++priv->stats.rx_errors;
#ifndef VBG400_DEBUG_LEN_ERROR
                restart_watchdog = 1;
#endif
            }

            if (status & (1UL << DMA_STATUS_RPS_BIT)) {
				// Mask to extract the receive status field from the status register
				u32 rs_mask = ((1UL << DMA_STATUS_RS_NUM_BITS) - 1) << DMA_STATUS_RS_BIT;
				u32 rs = (status & rs_mask) >> DMA_STATUS_RS_BIT;
				printk(/*printk(KERN_INFO */"int_handler() %s: RX process stopped 0x%x\n", dev->name, rs);
				++priv->stats.rx_errors;
				restart_watchdog = 1;

                // Restart the receiver
                printk(/*printk(KERN_INFO */"int_handler() %s: Restarting receiver\n", dev->name);
                dma_reg_set_mask(priv, DMA_OP_MODE_REG, (1UL << DMA_OP_MODE_SR_BIT));
            }

            if (status & (1UL << DMA_STATUS_TPS_BIT)) {
				// Mask to extract the transmit status field from the status register
				//u32 ts_mask = ((1UL << DMA_STATUS_TS_NUM_BITS) - 1) << DMA_STATUS_TS_BIT;
				//u32 ts = (status & ts_mask) >> DMA_STATUS_TS_BIT;
                ++priv->stats.tx_errors;
                restart_watchdog = 1;
                restart_tx = 1;
            }
            // Test for pure error interrupts
            if (status & (1UL << DMA_STATUS_FBE_BIT)) {
				// Mask to extract the bus error status field from the status register
				//u32 eb_mask = ((1UL << DMA_STATUS_EB_NUM_BITS) - 1) << DMA_STATUS_EB_BIT;
				//u32 eb = (status & eb_mask) >> DMA_STATUS_EB_BIT;
                restart_watchdog = 1;
            }

            if (restart_watchdog) {
                // Restart the link/PHY state watchdog immediately, which will
                // attempt to restart the system
                mod_timer(&priv->watchdog_timer, jiffies);
                restart_watchdog = 0;
            }
        }

        if (unlikely(restart_tx)) {
            // Restart the transmitter
            printk(KERN_INFO "int_handler() %s: Restarting transmitter\n", dev->name);
            dma_reg_set_mask(priv, DMA_OP_MODE_REG, (1UL << DMA_OP_MODE_ST_BIT));
        }

        if (unlikely(poll_tx)) {
            // Issue a TX poll demand in an attempt to restart TX descriptor
            // processing
            printk(KERN_INFO "int_handler() %s: Issuing Tx poll demand\n", dev->name);
            dma_reg_write(priv, DMA_TX_POLL_REG, 0);
        }

        // Read the record of current interrupt requests again, in case some
        // more arrived while we were processing
        raw_status = dma_reg_read(priv, DMA_STATUS_REG);
        // Get status of enabled interrupt sources.
        status = raw_status & int_enable;
    }

    return IRQ_HANDLED;
}

static void gmac_down(struct net_device *dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    int desc;

	if (priv->napi_enabled) {
		// Stop NAPI
		napi_disable(&priv->napi_struct);
		priv->napi_enabled = 0;
	}

    // Stop further TX packets being delivered to hard_start_xmit();
    netif_stop_queue(dev);
    netif_carrier_off(dev);

    // Disable all GMAC interrupts
    gmac_int_en_clr(priv, ~0UL, 0, 0);

    // Stop receiver, waiting until it's really stopped and then take ownership
    // of all rx descriptors
    change_rx_enable(priv, 0, 1, 0);

    if (priv->desc_rx_addr) {
        rx_take_ownership(&priv->rx_gmac_desc_list_info);
    }

    // Stop all timers
    delete_watchdog_timer(priv);
#ifdef VBG400_DEBUG_LEN_ERROR_LOG
    delete_len_error_timer(priv);
#endif
    if (priv->desc_rx_addr) {
        // Free receive descriptors
		printk(KERN_INFO "gmac: gmac_down() freeing rx descriptors!\n");
        do {
            int first_last = 0;
            rx_frag_info_t frag_info;

            desc = get_rx_descriptor(priv, &first_last, 0, &frag_info);
            if (desc >= 0) {
				if (unlikely(priv->rx_buffers_per_page)) {
					// If this is the last packet in the page, release the DMA mapping
					unmap_rx_page(priv, frag_info.phys_adr);
					put_page(frag_info.page);
				} else {
                    // Release the DMA mapping for the packet buffer
                    dma_unmap_single(priv->device, frag_info.phys_adr, frag_info.length, DMA_FROM_DEVICE);

                    // Free the skb
                    dev_kfree_skb((struct sk_buff *)frag_info.page);
				}
            }
        } while (desc >= 0);

		printk(KERN_INFO "gmac: gmac_down() freeing tx descriptors!\n");
        // Free transmit descriptors
        do {
            struct sk_buff *skb;
            tx_frag_info_t  frag_info;
            int             buffer_owned;

            desc = get_tx_descriptor(priv, &skb, 0, &frag_info, &buffer_owned);
            if (desc >= 0) {
                if (buffer_owned) {
                    // Release the DMA mapping for the packet buffer
                    dma_unmap_single(priv->device, frag_info.phys_adr, frag_info.length, DMA_FROM_DEVICE);
                }

                if (skb) {
                    // Free the skb
                    dev_kfree_skb(skb);
                }
            }
        } while (desc >= 0);

		printk(KERN_INFO "gmac: gmac_down() freeing tx_frag_info!\n");
        // Free any resources associated with the buffers of a pending packet
        if (priv->tx_pending_fragment_count) {
            tx_frag_info_t *frag_info = priv->tx_pending_fragments;

            while (priv->tx_pending_fragment_count--) {
                dma_unmap_single(priv->device, frag_info->phys_adr, frag_info->length, DMA_FROM_DEVICE);
                ++frag_info;
            }
        }

		printk(KERN_INFO "gmac: gmac_down() freeing tx_pending_skb!\n");
        // Free the socket buffer of a pending packet
        if (priv->tx_pending_skb) {
            dev_kfree_skb(priv->tx_pending_skb);
            priv->tx_pending_skb = 0;
        }
    }

    // Power down the PHY
    phy_powerdown(dev);
}

static int stop(struct net_device *dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);

    gmac_down(dev);

	printk(KERN_INFO "gmac: free tx_desc_shadow!\n");
	// Free the shadow descriptor memory
	kfree(priv->tx_desc_shadow_);
	priv->tx_desc_shadow_ = 0;

	printk(KERN_INFO "gmac: free rx_desc_shadow!\n");
	kfree(priv->rx_desc_shadow_);
	priv->rx_desc_shadow_ = 0;

    // Release the IRQ
    if (priv->have_irq) {
		if(priv->dev_id == 0)	
			wave400_disable_irq(WAVE400_SYNOP_ETHER_IRQ_OUT_INDEX);
		else
			wave400_disable_irq(WAVE400_SYNOP_ETHER_IRQ_GMAC2_OUT_INDEX);
        priv->have_irq = 0;
    }

    // Free the sysfs resources
    //kobject_del(&priv->link_state_kobject);
    //kset_unregister(&priv->link_state_kset);

	priv->interface_up = 0;
	printk(KERN_INFO "gmac: stop() finished!\n");
    return 0;
}

static void hw_set_mac_address(struct net_device *dev, unsigned char* addr)
{
    u32 mac_lo;
    u32 mac_hi;

    mac_lo  =  (u32)addr[0];
    mac_lo |= ((u32)addr[1] << 8);
    mac_lo |= ((u32)addr[2] << 16);
    mac_lo |= ((u32)addr[3] << 24);

    mac_hi  =  (u32)addr[4];
    mac_hi |= ((u32)addr[5] << 8);
    mac_reg_write(netdev_priv(dev), MAC_ADR0_LOW_REG, mac_lo);
    mac_reg_write(netdev_priv(dev), MAC_ADR0_HIGH_REG, mac_hi);
}

static int set_mac_address(struct net_device *dev, void *p)
{
    struct sockaddr *addr = p;

    if (!is_valid_ether_addr(addr->sa_data)) {
        return -EADDRNOTAVAIL;
    }

    memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);
    hw_set_mac_address(dev, addr->sa_data);

    return 0;
}

/*
static void multicast_hash(struct dev_mc_list *dmi, u32 *hash_lo, u32 *hash_hi)
{
    u32 crc = ether_crc_le(dmi->dmi_addrlen, dmi->dmi_addr);
    u32 mask = 1 << ((crc >> 26) & 0x1F);

    if (crc >> 31) {
        *hash_hi |= mask;
    } else {
        *hash_lo |= mask;
    }
} */

static void set_multicast_list(struct net_device *dev)
{
    gmac_priv_t* priv = netdev_priv(dev);
    u32 mode = 0;

    // Promiscuous mode overrides all-multi which overrides other filtering
    if (dev->flags & IFF_PROMISC) {
        mode |= (1 << MAC_FRAME_FILTER_PR_BIT);
    } else if (dev->flags & IFF_ALLMULTI) {
        mode |= (1 << MAC_FRAME_FILTER_PM_BIT);
    } else {
		// Set SA fitler in NORMAL mode.
		u32 addr = 0;

		addr =  dev->dev_addr[0];
		addr |= (u32) dev->dev_addr[1] << 8;
		addr |= (u32) dev->dev_addr[2] << 16;
		addr |= (u32) dev->dev_addr[3] << 24;
		mac_adrlo_reg_write(priv, 0, addr);

		addr  = dev->dev_addr[4];
		addr |= (u32)dev->dev_addr[5] << 8;
		addr |= (1 << MAC_ADR1_HIGH_AE_BIT);
	   mac_adrhi_reg_write(priv, 0, addr);
    }

  // Update FILTER register. 
  mac_reg_write(priv, MAC_FRAME_FILTER_REG, mode);
}

#ifndef CONFIG_VBG400_CHIPIT
void get_phy_speed(gmac_priv_t* priv)
{
    int supports_gmii = priv->mii.mdio_read(priv->mii.dev, priv->mii.phy_id, MII_BMSR) & BMSR_ESTATEN;
    int lpa, lpa2=0, advertise, advertise2=0;
	unsigned int negotiated_10_100, negotiated_1000;
	int i;

/* first, a dummy read, needed to latch some MII phys */
	priv->mii.mdio_read(priv->mii.dev, priv->mii.phy_id, MII_BMSR);
	printk(KERN_INFO "get_phy_speed: priv->dev_id = %d, MII_BMSR=0x%08x\n",priv->dev_id, priv->mii.mdio_read(priv->mii.dev, priv->mii.phy_id, MII_BMSR));
	if ((priv->mii.mdio_read(priv->mii.dev, priv->mii.phy_id, MII_BMSR) & BMSR_LSTATUS) == 0)
	{
		for (i=0; i<5; i++) {
			mdelay(1000);
			printk(KERN_INFO "get_phy_speed: %d\n",i);
			if (priv->mii.mdio_read(priv->mii.dev, priv->mii.phy_id, MII_BMSR) & BMSR_LSTATUS)
				break;
		}
		if (i>=5) {
			printk(KERN_INFO "get_phy_speed: link down, no much to do\n");
			return;
		}
	}
    advertise = priv->mii.mdio_read(priv->mii.dev, priv->mii.phy_id, MII_ADVERTISE);
	if (supports_gmii) {
		advertise2 = priv->mii.mdio_read(priv->mii.dev, priv->mii.phy_id, MII_CTRL1000);
	    lpa2 = priv->mii.mdio_read(priv->mii.dev, priv->mii.phy_id, MII_STAT1000);
		printk(KERN_INFO "get_phy_speed: supports_gmii, advertise=0x%08x, advertise2=0x%08x\n",advertise,advertise2);
	}
	for (i=0; i<5; i++) {
		lpa = priv->mii.mdio_read(priv->mii.dev, priv->mii.phy_id, MII_LPA);
		if (lpa!=0)
			break;
	}
    printk(KERN_INFO "get_phy_speed: lpa=0x%08x, lpa2=0x%08x, lopps=%i\n",lpa,lpa2,i);
	if (lpa==0) {
		printk(KERN_INFO "get_phy_speed: error in lpa, set to 0x0100\n");
		lpa = 0x0100;
	}
    /* Determine negotiated mode/duplex from our and link partner's advertise values */
    negotiated_10_100 = mii_nway_result(lpa & advertise);
    negotiated_1000   = mii_nway_result_1000(lpa2, advertise2);
    printk(KERN_INFO "get_phy_speed: negotiated_10_100=0x%08x, negotiated_1000=0x%08x\n",negotiated_10_100,negotiated_1000);

/*need lock?*/
     priv->mii.using_1000 = 0;
     priv->mii.using_100 = 0;
     priv->mii.using_10 = 0;
    /* Determine the rate we're operating at */
    if (negotiated_1000 & (LPA_1000FULL | LPA_1000HALF)) {
        priv->mii.using_1000 = 1;
    } else {
        if (negotiated_10_100 & (LPA_100FULL | LPA_100HALF)) 
            priv->mii.using_100 = 1;
        else 
            priv->mii.using_10 = 1;
    }
	printk(KERN_INFO "get_phy_speed: using_10=%d, using_100=%d, using_1000=%d\n",priv->mii.using_10,priv->mii.using_100,priv->mii.using_1000);
}

void set_gmac_10_100_giga(gmac_priv_t* priv, u32* reg_data, int interface_mode, enum config_from mode)
{
    /* Some configurations are interface mode related.
       In "MAC Configuration Register" handle bits 14 and 24
       TODO: what about interrupts ? ("	GMAC Register 15 bit 0, ) */
    if (interface_mode == INTERFACE_RMII || interface_mode == INTERFACE_RGMII)
    {
        printk( KERN_INFO "gmac_up(): INTERFACE_RMII/INTERFACE_RGMII\n");
        if (priv->mii.using_10) { /*speed: 0=10, 1=100*/
            printk( KERN_INFO "gmac_up(): using_10, clear field 0x00004000\n");
            (*reg_data) &= ~(0x00004000);
        } else if (priv->mii.using_100) { /*speed: 0=10, 1=100*/
            printk( KERN_INFO "gmac_up(): using_100, set field 0x00004000\n");
            (*reg_data) |= 1 << AC_CONFIG_FES_BIT;
        } else if (mode != ON_THE_FLY){ /*port select: 0=gmii, 1=mii*/
            printk( KERN_INFO "gmac_up(): using_1000, clear field 0x00008000\n");
            (*reg_data) &= ~(0x00008000);
        }
    }
    else if (interface_mode == INTERFACE_MII_GMII)
    {
        printk( KERN_INFO "gmac_up(): INTERFACE_MII_GMII\n");
        if (priv->mii.using_1000 && mode != ON_THE_FLY) { /*port select: 0=gmii, 1=mii*/
            printk( KERN_INFO "gmac_up(): clear field 0x00008000\n");
            (*reg_data) &= ~(0x00008000);
        }
    }
}
#endif


static int gmac_up(struct net_device *dev)
{
    int status = 0;
    gmac_priv_t *priv = (gmac_priv_t*)netdev_priv(dev);
    u32 reg_contents;
#ifndef CONFIG_VBG400_CHIPIT
    int reversed = CONFIG_MAC_MODE_REVERSED;
    int gmac_index = priv->dev_id;
    int speed;
    int interface_mode = CONFIG_VBG400_PHY_INTERFACE_MODE; //interface_mode - 0=mii/gmii, 1=rgmii, 2=rmii 3=mii_r/gmii_r, 4=rgmii_r, 5=rmii_r
	printk(KERN_INFO "gmac_up: interface_mode=%d\n",interface_mode);
#endif

	printk(KERN_INFO "gmac_up: wave400_disable_irq (%d)\n",irqOut[priv->dev_id]);
	/*disable out interrupt because it is enabled (from u-boot?) and cause interrupt after GMAC reset below*/
	wave400_disable_irq(irqOut[priv->dev_id]);

 	// Perform any actions required before GMAC reset
	do_pre_reset_actions(priv);

    // Reset the entire GMAC
    dma_reg_write(priv, DMA_BUS_MODE_REG, 1UL << DMA_BUS_MODE_SWR_BIT);

    // Ensure reset is performed before testing for completion
    wmb();

    // Wait for the reset operation to complete
    status = -EIO;
	printk(KERN_INFO "Resetting GMAC\n");
    for (;;) {
        if (!(dma_reg_read(priv, DMA_BUS_MODE_REG) & (1UL << DMA_BUS_MODE_SWR_BIT))) {
            status = 0;
            break;
        }
    }

	printk(KERN_INFO "call do_post_reset_actions\n");
	// Perform any actions required after GMAC reset
	do_post_reset_actions(priv);

    // Did the GMAC reset operation fail?
    if (status) {
        printk(KERN_ERR "gmac_up() %s: GMAC reset failed\n", dev->name);
        goto gmac_up_err_out;
    }
	printk(KERN_INFO "GMAC reset complete\n");
#ifndef CONFIG_VBG400_CHIPIT
    post_phy_reset(priv);
    /* watchdog detect changes on the fly, but because it is not working yet
    * a change from 100M to 1G and vice versa would not be handled.
    * That means, when disconnect net cable, watchdog does not detect it.
    */
    init_npu_gmac_regs(priv, interface_mode, reversed, gmac_index, VBG400_RUN_TIME);
    printk(KERN_INFO "gmac_up: using_10=%d, using_100=%d, using_1000=%d\n",priv->mii.using_10,priv->mii.using_100,priv->mii.using_1000);

    //disable RGMII interrupt:
    {
        u32 reg_contents;
        reg_contents = mac_reg_read(priv, MAC_INTERRUPT_MASK_REG);
        printk(KERN_INFO "gmac_up(): disable RGMII interrupt reg_contents = 0x%08x\n", reg_contents);
        reg_contents |= 0x00000001;
        mac_reg_write(priv, MAC_INTERRUPT_MASK_REG, reg_contents);
#ifndef VBG400_TEST_MMC_PMT_GLI_IN_IRQ
        /* Must clear MMC, PMT and GLI interrupts */
        mac_reg_read(priv, MAC_RGMII_STATUS_REG);
        process_non_dma_ints(priv, ( (1UL << DMA_STATUS_GPI_BIT) | (1UL << DMA_STATUS_GMI_BIT) | (1UL << DMA_STATUS_GLI_BIT) ));
#endif
    }
#endif
	/* Initialise MAC config register contents
	 */
    reg_contents = 0;
    if (!priv->mii.using_1000) {
        printk( KERN_INFO "gmac_up() %s: PHY in 10/100Mb mode\n", dev->name);
        reg_contents |= (1UL << MAC_CONFIG_PS_BIT);
    } else {
        printk( KERN_INFO "gmac_up() %s: PHY in 1000Mb mode\n", dev->name);
    }
    if (priv->mii.full_duplex) {
        reg_contents |= (1UL << MAC_CONFIG_DM_BIT);
    }

#ifdef USE_RX_CSUM
	printk(KERN_INFO "gmac: USE_RX_CSUM (HW_CHECKSUM) is ON!! **\n");
	reg_contents |= (1UL << MAC_CONFIG_IPC_BIT);
#endif // USE_RX_CSUM

    if (priv->jumbo_) {
		// Allow passage of jumbo frames through both transmitter and receiver
		reg_contents |=	((1UL << MAC_CONFIG_JE_BIT) |
                        (1UL << MAC_CONFIG_JD_BIT) |
						 (1UL << MAC_CONFIG_WD_BIT));
	}

	// Enable transmitter and receiver
    reg_contents |= ((1UL << MAC_CONFIG_TE_BIT) |
                     (1UL << MAC_CONFIG_RE_BIT));

	// Select the minimum IFG - I found that 80 bit times caused very poor
	// IOZone performance, so stcik with the 96 bit times default
	reg_contents |= (0UL << MAC_CONFIG_IFG_BIT);

#ifndef CONFIG_VBG400_CHIPIT
    set_gmac_10_100_giga(priv, &reg_contents, interface_mode, ON_INIT);
#endif
    // Write MAC config setup to the GMAC
    mac_reg_write(priv, MAC_CONFIG_REG, reg_contents);

	/* Initialise MAC VLAN register contents
	 */
    reg_contents = 0;
    mac_reg_write(priv, MAC_VLAN_TAG_REG, reg_contents);

    // Initialise the hardware's record of our primary MAC address
    hw_set_mac_address(dev, dev->dev_addr);

    // Initialise multicast and promiscuous modes
    //set_multicast_list(dev);

    // Disable all MMC interrupt sources
    mac_reg_write(priv, MMC_RX_MASK_REG, ~0UL);
    mac_reg_write(priv, MMC_TX_MASK_REG, ~0UL);

    // Remember how large the unified descriptor array is to be
    priv->total_num_descriptors = NUM_TX_DMA_DESCRIPTORS + NUM_RX_DMA_DESCRIPTORS;

    // Initialise the structures managing the TX descriptor list
    init_tx_desc_list(&priv->tx_gmac_desc_list_info,
                      priv->desc_tx_addr,
					  priv->tx_desc_shadow_,
                      NUM_TX_DMA_DESCRIPTORS);

    // Initialise the structures managing the RX descriptor list
    init_rx_desc_list(&priv->rx_gmac_desc_list_info,
                      priv->desc_rx_addr,
					  priv->rx_desc_shadow_,
                      NUM_RX_DMA_DESCRIPTORS,
                      priv->rx_buffer_size_); 
#ifdef GMAC_DEBUG
	printk(KERN_INFO "gmac: priv->rx_gmac_desc_list_info.empty:%d\n", priv->rx_gmac_desc_list_info.empty_count);
#endif
    // Reset record of pending Tx packet
    priv->tx_pending_skb = 0;
    priv->tx_pending_fragment_count = 0;

    // Write the physical DMA consistent address of the start of the tx descriptor array
    dma_reg_write(priv, DMA_TX_DESC_ADR_REG, priv->desc_tx_dma_addr);

    // Write the physical DMA consistent address of the start of the rx descriptor array
    dma_reg_write(priv, DMA_RX_DESC_ADR_REG, priv->desc_rx_dma_addr);

    // Initialise the GMAC DMA bus mode register
    dma_reg_write(priv, DMA_BUS_MODE_REG, ( (1UL << DMA_BUS_MODE_AAL_BIT) | // AAL Address aligned beats
										  (1UL << DMA_BUS_MODE_FB_BIT)   |	// Force bursts
                                          (8UL << DMA_BUS_MODE_PBL_BIT)  |	// AHB burst size
                                          (1UL << DMA_BUS_MODE_DA_BIT)));// |	// Round robin Rx/Tx 
										 // (2UL << DMA_BUS_MODE_DSL_BIT)));	// Skip Length - 2 

	//dma_reg_write(priv, DMA_BUS_MODE_REG, 0x02a14408); 
    // Prepare receive descriptors
#ifdef GMAC_DEBUG
	printk(KERN_INFO "gmac: gmac_up refill_rx_ring()!***\n");
#endif
    refill_rx_ring(dev);

    // Clear any pending interrupt requests
    dma_reg_write(priv, DMA_STATUS_REG, dma_reg_read(priv, DMA_STATUS_REG));

	/* Initialise flow control register contents
	 */
	// Enable Rx flow control
    reg_contents = (1UL << MAC_FLOW_CNTL_RFE_BIT);

	if (priv->mii.using_pause) {
		// Enable Tx flow control
		reg_contents |= (1UL << MAC_FLOW_CNTL_TFE_BIT);
	}

    // Set the duration of the pause frames generated by the transmitter when
	// the Rx fifo fill threshold is exceeded
	reg_contents |= ((0x100UL << MAC_FLOW_CNTL_PT_BIT) |	// Pause for 256 slots
					  (0x1UL << MAC_FLOW_CNTL_PLT_BIT));

	// Write flow control setup to the GMAC
    mac_reg_write(priv, MAC_FLOW_CNTL_REG, reg_contents);

	// Initialise operation mode register contents
    // Initialise the GMAC DMA operation mode register. Set Tx/Rx FIFO thresholds
    // to make best use of our limited SDRAM bandwidth when operating in gigabit
	reg_contents = ((DMA_OP_MODE_TTC_256 << DMA_OP_MODE_TTC_BIT) |    // Tx threshold
                    (1UL << DMA_OP_MODE_FUF_BIT) |    					// Forward Undersized good Frames
    	            (DMA_OP_MODE_RTC_128 << DMA_OP_MODE_RTC_BIT) |	// Rx threshold 128 bytes
                    (1UL << DMA_OP_MODE_OSF_BIT) | // Operate on 2nd frame
					(1UL << DMA_OP_MODE_DT_BIT));  // (DT) Dis. drop. of tcp/ip CS error frames

//Yan
#ifdef USE_RX_CSUM
	//clear bit 26
	reg_contents &= ~(1UL << DMA_OP_MODE_DT_BIT);
#endif
	
#ifdef USE_HW_FLOW_CTL
	// Enable hardware flow control
	reg_contents |= (1UL << DMA_OP_MODE_EFC_BIT);

	// Set threshold for enabling hardware flow control at (full - 1kb) to give
	// space for upto two in-flight std MTU packets to arrive after pause frame
	// has been sent.
	
	//For FIFO 4KB
	reg_contents &= ~(3UL << DMA_OP_MODE_RFA_BIT);

	// Set threshold for disabling hardware flow control (-2KB)
	reg_contents |= (1UL << DMA_OP_MODE_RFD_BIT);
#endif

    // Don't flush Rx frames from FIFO just because there's no descriptor available
	reg_contents |= (1UL << DMA_OP_MODE_DFF_BIT);

	// Write settings to operation mode register
	dma_reg_write(priv, DMA_OP_MODE_REG, reg_contents);

    // GMAC requires store&forward in order to compute Tx checksums
    dma_reg_set_mask(priv, DMA_OP_MODE_REG, (1UL << DMA_OP_MODE_SF_BIT));

    // Ensure setup is complete, before enabling TX and RX
    wmb();

	// Start NAPI
	BUG_ON(priv->napi_enabled);
	napi_enable(&priv->napi_struct);
	priv->napi_enabled = 1;

    // Start the transmitter and receiver
	dma_reg_set_mask(priv, DMA_OP_MODE_REG, (1UL << DMA_OP_MODE_ST_BIT));
    change_rx_enable(priv, 1, 0, 0);

    // Enable interesting GMAC interrupts
    gmac_int_en_set(priv, ((1UL << DMA_INT_ENABLE_NI_BIT)  |
                           (1UL << DMA_INT_ENABLE_AI_BIT)  |
                           (1UL << DMA_INT_ENABLE_FBE_BIT) |
                           (1UL << DMA_INT_ENABLE_RI_BIT)  |
                           (1UL << DMA_INT_ENABLE_RU_BIT)  |
                           (1UL << DMA_INT_ENABLE_OV_BIT)  |
                           (1UL << DMA_INT_ENABLE_RW_BIT)  |
                           (1UL << DMA_INT_ENABLE_RS_BIT)  |
                           (1UL << DMA_INT_ENABLE_TI_BIT)  |
                           (1UL << DMA_INT_ENABLE_UN_BIT)  |
                           (1UL << DMA_INT_ENABLE_TJ_BIT)  |
                           (1UL << DMA_INT_ENABLE_TS_BIT))); 
	
    // (Re)start the link/PHY state monitoring timer
    start_watchdog_timer(priv);
#ifdef VBG400_DEBUG_LEN_ERROR_LOG
    start_len_error_timer(priv);
#endif
    // Allow the network stack to call hard_start_xmit()
    netif_start_queue(dev);
	printk(KERN_INFO "gmac_up: wave400_enable_irq (%d)\n",irqOut[priv->dev_id]);
	wave400_enable_irq(irqOut[priv->dev_id]);
#ifdef DUMP_REGS_ON_GMAC_UP
    dump_mac_regs(priv, priv->macBase, priv->dmaBase);
#endif
	printk(KERN_INFO "gmac_up: status=%d\n",status);
    return status;

gmac_up_err_out:
	printk(KERN_INFO "gmac_up: ERROR, call stop !***\n");
    stop(dev);

    return status;
}

static void set_rx_packet_info(struct net_device *dev)
{
    gmac_priv_t *priv = (gmac_priv_t*)netdev_priv(dev);
	int max_packet_buffer_size = dev->mtu + EXTRA_RX_SKB_SPACE;

	if (max_packet_buffer_size > max_descriptor_length()) {
#ifndef RX_BUFFER_SIZE
		priv->rx_buffer_size_ = max_packet_buffer_size;
#else // !RX_BUFFER_SIZE
		priv->rx_buffer_size_ = RX_BUFFER_SIZE;
#endif // ! RX_BUFFER_SIZE
		priv->rx_buffers_per_page = GMAC_ALLOC_SIZE / (priv->rx_buffer_size_ + NET_IP_ALIGN);
	} else {
		priv->rx_buffer_size_ = max_packet_buffer_size;
		priv->rx_buffers_per_page = 0;
	}
}

static int change_mtu(struct net_device *dev, int new_mtu)
{
    int status = 0;
    gmac_priv_t *priv = (gmac_priv_t*)netdev_priv(dev);
    int original_mtu = dev->mtu;

    // Check that new MTU is within supported range
    if ((new_mtu < MIN_PACKET_SIZE) || (new_mtu > MAX_JUMBO)) {
        printk( KERN_WARNING "change_mtu() %s: Invalid MTU %d\n", dev->name, new_mtu);
        status = -EINVAL;
    } else if (priv->interface_up) {
        // Put MAC/PHY into quiesent state, causing all current buffers to be
        // deallocated and the PHY to powerdown
        gmac_down(dev);

        // Record the new MTU, so bringing the MAC back up will allocate
        // resources to suit the new MTU
        dev->mtu = new_mtu;

		// Set length etc. of rx packets
		set_rx_packet_info(dev);

        // Reset the PHY to get it into a known state and ensure we have TX/RX
        // clocks to allow the GMAC reset to complete
		printk(KERN_INFO "gmac: change_mtu() call to phy_reset() **\n");
        if (phy_reset(priv->netdev)) {
            printk( KERN_ERR "change_mtu() %s: Failed to reset PHY\n", dev->name);
            status = -EIO;
        } else {
            // Record whether jumbo frames should be enabled
            priv->jumbo_ = (dev->mtu > NORMAL_PACKET_SIZE);

            // Force or auto-negotiate PHY mode
            priv->phy_force_negotiation = 1;

            // Reallocate buffers with new MTU
            gmac_up(dev);
        }
	} else {
        // Record the new MTU, so bringing the interface up will allocate
        // resources to suit the new MTU
        dev->mtu = new_mtu;
    }

    // If there was a failure
    if (status) {
        // Return the MTU to its original value
        printk( KERN_INFO "change_mtu() Failed, returning MTU to original value\n");
        dev->mtu = original_mtu;
    }

    return status;
}

//Yan
void* alloc_dmaable_memory(struct device *device,
					   u32 size, u32 * addr, u32 direction,
					   void **desc)
{
	if(!(*desc = kmalloc(size, GFP_KERNEL)))
		return NULL;
	memset(*desc, 0, size);
	*addr =	dma_map_single(device, *desc, size, direction);
#ifdef GMAC_DEBUG
	printk(KERN_INFO "gmac: alloc_dmaable_memory(): dir:%s dma_addr:0x%x desc_addr:0x%x &desc_vaddr:0x%x\n",
	direction == 1 ? "DMA_TO_DEVICE":"DMA_FROM_DEVICE", *addr,  CKSEG1ADDR(*desc), desc); 
#endif
	return (void*) (CKSEG1ADDR(*desc));
} 


static int open(struct net_device *dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    int status;
#ifndef CONFIG_VBG400_CHIPIT
    int speed;
    int reversed = CONFIG_MAC_MODE_REVERSED;
    int interface_mode = CONFIG_VBG400_PHY_INTERFACE_MODE; //interface_mode - 0=mii/gmii, 1=rgmii, 2=rmii 3=mii_r/gmii_r, 4=rgmii_r, 5=rmii_r
    int gmac_index = priv->dev_id;
	u32 phy_reg;
#endif

//    printk( KERN_ERR "open() %s: gmac_index=%d\n", dev->name,gmac_index);
    // Reset the PHY to get it into a known state and ensure we have TX/RX clocks
    // to allow the GMAC reset to complete
    if (phy_reset(priv->netdev)) {
        printk( KERN_ERR "open() %s: Failed to reset PHY\n", dev->name);
        status = -EIO;
        goto open_err_out;
    }
#ifndef CONFIG_VBG400_CHIPIT
    config_phy_skew(priv);
	/*Call delay config handler for setting second gmac.
	  In u-boot only one gmac is calibrated.
      Assume:
        if only one phy configure, it is already configured in u-boot
        if both, gmac1 need to be configured
        TODO: pass linux param indicate what gmac configured in u-boot, use this info !!!!!
    */
#ifdef VBG400_SUPPORT_INTERFACE_REVERSED
    if ((reversed == MAC_REVERSED) || CONFIG_VBG400_MAC_GMAC_INDEX == INDEX_GMAC1)
#else
//    if (gmac_index == INDEX_GMAC1 && (/*CONFIG_VBG400_MAC_GMAC_INDEX*/CONFIG_VBG400_MAC_GMAC_INDEX_TEMP == INDEX_GMAC1 || /*CONFIG_VBG400_MAC_GMAC_INDEX*/CONFIG_VBG400_MAC_GMAC_INDEX_TEMP == INDEX_GMACS_BOTH))
#endif
//        clock_delay_calibration(gmac_index);
    /* On Init NPU shared registers have to be configured before GMAC Init */
    init_npu_gmac_regs(priv, interface_mode, reversed, gmac_index, VBG400_INIT);
#endif

    // Check that the MAC address is valid.  If it's not, refuse to bring the
    // device up
    if (!is_valid_ether_addr(dev->dev_addr)) {
        printk( KERN_ERR "open() %s: MAC address invalid\n", dev->name);
        status = -EINVAL;
        goto open_err_out;
    }

    // Allocate the IRQ
    priv->have_irq = 1;

	//Allocate all descriptors (both TX and RX).
	if(!(priv->desc_tx_addr = alloc_dmaable_memory(priv->device, 
		NUM_TX_DMA_DESCRIPTORS * sizeof(gmac_dma_desc_t),
		&priv->desc_tx_dma_addr, DMA_TO_DEVICE, (void**) &priv->desc_tx_vaddr))) {
	     printk( KERN_ERR "open() %s: Failed to allocate consistent memory for DMA descriptors\n", dev->name);
    	 status = -ENOMEM;
         goto open_err_out;
	}

	if(!(priv->desc_rx_addr = alloc_dmaable_memory(priv->device, 
		NUM_RX_DMA_DESCRIPTORS * sizeof(gmac_dma_desc_t),
		&priv->desc_rx_dma_addr, DMA_FROM_DEVICE, (void**) &priv->desc_rx_vaddr))) {
	     printk( KERN_ERR "open() %s: Failed to allocate consistent memory for DMA descriptors\n", dev->name);
    	 status = -ENOMEM;
         goto open_err_out;
	}

	// Allocate memory to hold shadow of GMAC descriptors
	if (!(priv->tx_desc_shadow_ = kmalloc(NUM_TX_DMA_DESCRIPTORS * sizeof(gmac_dma_desc_t), GFP_KERNEL))) {
        DBG(1, KERN_ERR "open() %s: Failed to allocate memory for Tx descriptor shadows\n", dev->name);
        status = -ENOMEM;
        goto open_err_out;
	}
	if (!(priv->rx_desc_shadow_ = kmalloc(NUM_RX_DMA_DESCRIPTORS * sizeof(gmac_dma_desc_t), GFP_KERNEL))) {
        DBG(1, KERN_ERR "open() %s: Failed to allocate memory for Rx descriptor shadows\n", dev->name);
        status = -ENOMEM;
        goto open_err_out;
	}

	printk(KERN_INFO "gmac: open: dev->mtu:%d\n", dev->mtu);
	// Record whether jumbo frames should be enabled
    priv->jumbo_ = (dev->mtu > NORMAL_PACKET_SIZE);

	set_rx_packet_info(dev);

	// Yan
    // Initialise sysfs for link state reporting
	status = gmac_link_state_init_sysfs(priv);
    if (status) {
        printk(KERN_ERR "open() %s: Failed to initialise sysfs support\n", dev->name);
        goto open_err_out;
    }

    // Initialise the work queue entry to be used to issue hotplug events to userspace
    INIT_WORK(&priv->link_state_change_work, work_handler);

    // Do startup operations that are in common with gmac_down()/_up() processing
    priv->mii_init_media = 1;
    priv->phy_force_negotiation = 1;
    status = gmac_up(dev);
    if (status) {
        goto open_err_out;
    }
	priv->interface_up = 1;
    return 0;

open_err_out:
	printk(KERN_INFO "probe: ERROR, call stop !***\n");
    stop(dev);

    return status;
}

static inline void unmap_fragments(
	gmac_priv_t     *priv,
    tx_frag_info_t *frags,
    int             count)
{
    while (count--) {
        dma_unmap_single(priv->device, frags->phys_adr, frags->length, DMA_TO_DEVICE);
        ++frags;
    }
}

static int hard_start_xmit(
    struct sk_buff    *skb,
    struct net_device *dev)
{
    gmac_priv_t            *priv = (gmac_priv_t*)netdev_priv(dev);
    unsigned long           irq_flags;
    struct skb_shared_info *shinfo = skb_shinfo(skb);
    int                     fragment_count = shinfo->nr_frags + 1;
    tx_frag_info_t          fragments[fragment_count];
    int                     frag_index;	

    // Get consistent DMA mappings for the SDRAM to be DMAed from by the GMAC,
    // causing a flush from the CPU's cache to the memory.

    // Do the DMA mappings before acquiring the tx lock, even though it complicates
    // the later code, as this can be a long operation involving cache flushing

    // Map the main buffer
    fragments[0].length = skb_headlen(skb);
    fragments[0].phys_adr = dma_map_single(priv->device, skb->data, skb_headlen(skb), DMA_TO_DEVICE);
    BUG_ON(dma_mapping_error(priv->device, fragments[0].phys_adr));

    // Map any SG fragments
    for (frag_index = 0; frag_index < shinfo->nr_frags; ++frag_index) {
        skb_frag_t *frag = &shinfo->frags[frag_index];

		printk(KERN_INFO "gmac: hard_start_xmit() map any SG segments!\n");
        fragments[frag_index + 1].length = frag->size;
        fragments[frag_index + 1].phys_adr = dma_map_page(priv->device, frag->page, frag->page_offset, frag->size, DMA_TO_DEVICE);
        BUG_ON(dma_mapping_error(priv->device, fragments[frag_index + 1].phys_adr));
    }

    // Protection against concurrent operations in ISR and hard_start_xmit()
    if (unlikely(!spin_trylock_irqsave(&priv->tx_spinlock_, irq_flags))) {
        unmap_fragments(priv, fragments, fragment_count);
		printk(KERN_INFO "gmac: hard_start_xmit() (NETDEV_TX_LOCKED)\n");
        return NETDEV_TX_LOCKED;
    }

    // NETIF_F_LLTX apparently introduces a potential for hard_start_xmit() to
    // be called when the queue has been stopped (although I think only in SMP)
    // so do a check here to make sure we should proceed
    if (unlikely(netif_queue_stopped(dev))) {
        unmap_fragments(priv, fragments, fragment_count);
        spin_unlock_irqrestore(&priv->tx_spinlock_, irq_flags);
		printk(KERN_INFO "gmac: hard_start_xmit() (NETDEV_TX_BUSY)\n");
        return NETDEV_TX_BUSY;
    }

    // Construct the GMAC DMA descriptor
    if (unlikely(set_tx_descriptor(priv,
                          skb,
                          fragments,
                          fragment_count,
                          skb->ip_summed == CHECKSUM_PARTIAL) < 0)) {
        // Shouldn't see a full ring without the queue having already been
        // stopped, and the queue should already have been stopped if we have
        // already queued a single pending packet
        if (priv->tx_pending_skb) {
            printk(KERN_WARNING "hard_start_xmit() Ring full and pending packet already queued\n");
            unmap_fragments(priv, fragments, fragment_count);
            spin_unlock_irqrestore(&priv->tx_spinlock_, irq_flags);
			printk(KERN_INFO "gmac: hard_start_xmit() a3\n");
            return NETDEV_TX_BUSY;
        }
	
	#ifdef GMAC_DEBUG	
		printk(KERN_INFO ":gmac set_tx_descriptor() failed!!! All TX descriptors are busy!! *******\n");
		printk(KERN_INFO ":gmac DMA_INT_REG:0x%x\n", dma_reg_read(priv, DMA_INT_ENABLE_REG));
		printk(KERN_INFO ":gmac DMA_STATUS_REG:0x%x\n", dma_reg_read(priv, DMA_STATUS_REG));
	#endif
		//print_descs_owner(priv, 0);

        // Should keep a record of the skb that we haven't been able to queue
        // for transmission and queue it as soon as a descriptor becomes free
        priv->tx_pending_skb = skb;
        priv->tx_pending_fragment_count = fragment_count;

        // Copy the fragment info to the allocated storage
        memcpy(priv->tx_pending_fragments, fragments, sizeof(tx_frag_info_t) * fragment_count);

        // Stop further calls to hard_start_xmit() until some descriptors are
        // freed up by already queued TX packets being completed
        netif_stop_queue(dev);
    } else {
        // Record start of transmission, so timeouts will work once they're
        // implemented
        dev->trans_start = jiffies;

        // Poke the transmitter to look for available TX descriptors, as we have
        // just added one, in case it had previously found there were no more
        // pending transmission
        dma_reg_write(priv, DMA_TX_POLL_REG, 0);
    }

    spin_unlock_irqrestore(&priv->tx_spinlock_, irq_flags);
    return NETDEV_TX_OK;
}

static struct net_device_stats *get_stats(struct net_device *dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    return &priv->stats;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
/**
 * Polling 'interrupt' - used by things like netconsole to send skbs without
 * having to re-enable interrupts. It's not called while the interrupt routine
 * is executing.
 */
static void netpoll(struct net_device *netdev)
{
    disable_irq(netdev->irq);
    int_handler(netdev->irq, netdev, NULL);
    enable_irq(netdev->irq);
}
#endif // CONFIG_NET_POLL_CONTROLLER

/**
  * Function to reset the GMAC core. 
  * This resets the DMA and GMAC core. After reset all the registers holds their respective reset value
  * @param[in] pointer to synopGMACdevice.
  * \return 0 on success else return the error status.
  */
void gmac_reset(struct net_device *dev)
{
	gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
	u32 data = 0;

	data = dma_reg_read(priv, DMA_BUS_MODE_REG);
	dma_reg_write(priv, DMA_BUS_MODE_REG, 1);
	printk(KERN_INFO ": gmac_reset() before mdelay()\n");
	mdelay(1);
	data = dma_reg_read(priv, DMA_BUS_MODE_REG);
}

//Yan
/************************************************
* Warning:
* Do not use the free_irq as it free the irqaction struct,
* but as it was not allocated by kmalloc, it fails.
* (due to const param .name in the struct)
*/
static struct irqaction net_irqaction[] = {
	{.handler = synopGMAC_intr_handler,
#ifndef VBG400_NO_ETH_SHARED_IRQ
     .flags = IRQF_DISABLED | IRQF_SHARED,
#else
	 .flags = IRQF_DISABLED,	/*disable nested interrupts */
#endif
	 /* Lior.H - when we need to use-> IRQF_NOBALANCING ? */
	 .name = "wave400_net",
	 },
	{.handler = synopGMAC_intr_handler,
#ifndef VBG400_NO_ETH_SHARED_IRQ
	 .flags = IRQF_DISABLED | IRQF_SHARED,
#else
	 .flags = IRQF_DISABLED,	/*disable nested interrupts */
#endif
	 /* Lior.H - when we need to use-> IRQF_NOBALANCING ? */
	 .name = "wave400_net",
	 },
};
void *irq_handlers[] = { wave400_net_irq, wave400_net_g2_irq };

static const struct net_device_ops gmac_dev_ops = {
    .ndo_open = open,
    .ndo_stop = stop,
    .ndo_start_xmit = hard_start_xmit,
    .ndo_get_stats = get_stats,
	 .ndo_change_mtu = change_mtu,
#ifdef CONFIG_NET_POLL_CONTROLLER
    .ndo_poll_controller = netpoll,
#endif // CONFIG_NET_POLL_CONTROLLER
    .ndo_set_mac_address = set_mac_address,
    .ndo_set_multicast_list = set_multicast_list,
};

#ifndef CONFIG_VBG400_CHIPIT

#define VBG400_USE_FIX_DELAY_VALUE
#define GMAC0_SW_RST_N                  0x00000010
#define GMAC1_SW_RST_N                  0x00004000
#define GMAC0_FIX_VALUE                 0x600
#define GMAC1_FIX_VALUE                 0x6000000
void init_npu_gmac_regs(gmac_priv_t *priv, int interface_mode, int reversed, int gmac_index, enum vbg400_npu_config config)
{
    gmac_interface* interface_mode_ptr;
    gmac_mask* mask_ptr;
    gmac_mask* mask_ptr_gmac0;
    u32 data;
    int speed;
    /*VBG400_PRE_CONFIG*/
#ifdef VBG400_USE_FIX_DELAY_VALUE
    int field_value[] = {0x300,0x4000000};        /*add this value to field gmac0_dly_phy_clk_tx */;
#else
TODO !!!
#endif
    int fix_delay[] = {GMAC0_FIX_VALUE, GMAC1_FIX_VALUE};
    u32 reset_mac_mask[] = {GMAC0_SW_RST_N, GMAC1_SW_RST_N};
#define GMAC_MODE_SMA_MASTER_MASK   0xE7FFFFFF
#define GMAC_MODE_SMA_MASTER_REVERSED  0x18000000
#define GMAC_MODE_SMA_MASTER_GMAC1  0x10000000


    if (priv->mii.using_10)
        speed = SPEED_10M;
    else if (priv->mii.using_100)
        speed = SPEED_100M;
    else
        speed = SPEED_1000M;

    if (config == VBG400_PRE_CONFIG) /*in pre, speed is not known yet. Select 100M*/
        speed = SPEED_100M;

    /*point to data of the relevant interface*/
    interface_mode_ptr  = interface_mode_array[interface_mode];
    if (gmac_index == INDEX_GMAC1)
        interface_mode_ptr  = interface_mode_array_gmac1[interface_mode];
    /*point to mask of the relevant gmac*/
    mask_ptr            = interface_mask_array[gmac_index];
    printk(KERN_INFO "init_npu_gmac_regs: interface_mode = %d, gmac_index = %d, speed = %d, config = %d\n",interface_mode,gmac_index,speed,config);

    /*lock access to NPU registers, 
    * After read register, other interface can change the value and the data is not valid any mode
    */

    /*GMAC_MODE_REG_ADDR:
    */
    data=MT_RdReg(WAVE400_SHARED_GMAC_BASE_ADDR, WAVE400_GMAC_MODE_REG_OFFSET);
    printk(KERN_INFO "init_npu_gmac_regs: GMAC_MODE_REG_ADDR, data = 0x%08x\n",data);
    //clean relevant bits, some fields are gmac0/1 related, defaults are needed
    data = data & ~(mask_ptr->gen3_shrd_gmac_mode_reg_mask);
#ifdef TEST_GMAC1_ERROR
    /*if gmac1 keep gmac0 configuration (data of gmac1 includes gmac0 setting)*/
    mask_ptr_gmac0 = interface_mask_array[0];
    data = data & ~(mask_ptr_gmac0->gen3_shrd_gmac_mode_reg_mask);
#endif
#ifdef VBG400_SUPPORT_INTERFACE_REVERSED
    /*for reversed mode gmii_r, rgmii_r and rmii_r GMAC1 need to control the sma bus (gmac_sma_lp and gmac1_mstr_sma so): */
    if ((reversed && (interface_mode != mii_r)) || CONFIG_VBG400_MAC_GMAC_INDEX == INDEX_GMAC1)
        data &= (data & GMAC_MODE_SMA_MASTER_MASK) | GMAC_MODE_SMA_MASTER_REVERSED;
    else
#endif
#ifdef TEST_GMAC1_ERROR
    if (CONFIG_VBG400_MAC_GMAC_INDEX_TEMP == INDEX_GMAC1)
#else
    if (CONFIG_VBG400_MAC_GMAC_INDEX == INDEX_GMAC1)
#endif
        data &= ((data & GMAC_MODE_SMA_MASTER_MASK) | GMAC_MODE_SMA_MASTER_GMAC1);

    data = data | (interface_mode_ptr + speed)->gen3_shrd_gmac_mode_reg;
#ifndef VBG400_USE_SMA_SELECT
    /*work sma via gmac0*/
    data &= GMAC_MODE_SMA_MASTER_MASK;
#endif
    MT_WrReg(WAVE400_SHARED_GMAC_BASE_ADDR, WAVE400_GMAC_MODE_REG_OFFSET, data);
    printk(KERN_INFO "init_npu_gmac_regs: write 0x%08x\n",data);

    /*GMAC_DIV_RATIO_REG_ADDR:
    */
    data=MT_RdReg(WAVE400_SHARED_GMAC_BASE_ADDR, WAVE400_GMAC_MODE_2_REG_OFFSET);
    printk(KERN_INFO "init_npu_gmac_regs: GMAC_DIV_RATIO_REG_ADDR, data = 0x%08x\n",data);
    //clean relevant bits, some fields are gmac0/1 related, defaults are needed
    data = data & ~(mask_ptr->gen3_shrd_gmac_div_ratio_reg_mask);
#ifdef TEST_GMAC1_ERROR
    /*if gmac1 keep gmac0 configuration (data of gmac1 includes gmac0 setting)*/
    data = data & ~(mask_ptr_gmac0->gen3_shrd_gmac_div_ratio_reg_mask);
#endif
    //or with data
    data = data | (interface_mode_ptr + speed)->gen3_shrd_gmac_div_ratio_reg;
    MT_WrReg(WAVE400_SHARED_GMAC_BASE_ADDR, WAVE400_GMAC_MODE_2_REG_OFFSET, data);
    printk(KERN_INFO "init_npu_gmac_regs: write 0x%08x\n",data);

    /*GMAC_DLY_PGM_REG_ADDR:
    */
    data=MT_RdReg(WAVE400_SHARED_GMAC_BASE_ADDR, WAVE400_DLY_PGM_REG_OFFSET);
    printk(KERN_INFO "init_npu_gmac_regs: GMAC_DLY_PGM_REG_ADDR, data = 0x%08x\n",data);
    if ((config == VBG400_PRE_CONFIG) && (data != (field_value[0] | field_value[1])))
    {
        /*overrun register*/
        data = field_value[gmac_index];
#if 1
//#ifdef TEST_GMAC1_ERROR
        /*if gmac1, make sure to have gmac0 as well*/
        data |= field_value[0];
#endif
	    printk(KERN_INFO "clock_delay_calibration: write delay value 0x%08x\n",data);
        MT_WrReg(WAVE400_SHARED_GMAC_BASE_ADDR,WAVE400_DLY_PGM_REG_OFFSET, data);
        /*reset release*/
#if 0
        data = MT_RdReg(WAVE400_SHARED_GMAC_BASE_ADDR,WAVE400_RESET_OFFSET);
        data &= (~reset_mac_mask[gmac_index]);
        MT_WrReg(WAVE400_SHARED_GMAC_BASE_ADDR,WAVE400_RESET_OFFSET, data);
        MT_WrReg(WAVE400_SHARED_GMAC_BASE_ADDR,WAVE400_RESET_OFFSET, data | reset_mac_mask[gmac_index]);
#endif
    }
}

#endif

#define DEVICE_REG_ADDR_MAC2_START  0xA7180000

static int probe(
    struct net_device      *netdev,
    struct platform_device *pdev,
    u32                     vaddr)
{
    int err = 0;
    u32 version;
    int i;
    unsigned synopsis_version;
    unsigned vendor_version;
    gmac_priv_t* priv;
	struct irqaction *action = &net_irqaction[pdev->id];
	struct resource *resource = NULL;
#ifndef CONFIG_VBG400_CHIPIT
    int reversed = CONFIG_MAC_MODE_REVERSED;        //0=normal, 1=reversed
    int interface_mode = CONFIG_VBG400_PHY_INTERFACE_MODE; //interface_mode - 0=mii/gmii, 1=rgmii, 2=rmii 3=mii_r/gmii_r, 4=rgmii_r, 5=rmii_r
#endif

    printk(KERN_INFO "In probe\n");
    
    /* Driver working mode debug information:
    *************************************************/
    printk(KERN_INFO "Work mode:\n");
    printk(KERN_INFO "NAPI_POLL_WEIGHT=%d\n",NAPI_POLL_WEIGHT);
#ifndef VBG400_NO_ETH_SHARED_IRQ
    printk(KERN_INFO "Shared IRQ\n");
#else
    printk(KERN_INFO "NO shared IRQ\n");
#endif
#ifdef VBG400_USE_TX_NAPI
    printk(KERN_INFO "TX in NAPI\n");
#else
    printk(KERN_INFO "TX in IRQ\n");
#endif
#ifdef VBG400_DEBUG_NET
    printk(KERN_INFO "Debug print on Rx buffer full\n");
#else
    printk(KERN_INFO "No debug print on Rx buffer full\n");
#endif
    /* Debug information end ************************/
    
    
    // Ensure all of the device private data are zero, so we can clean up in
    // the event of a later failure to initialise all fields
    priv = (gmac_priv_t*)netdev_priv(netdev);
    memset(priv, 0, sizeof(gmac_priv_t));

    // No debug messages allowed
    priv->msg_level = 0UL;

	priv->dev_id = pdev->id;
    printk(KERN_INFO "In probe: priv->dev_id=%d\n",priv->dev_id);
#ifdef VBG400_SUPPORT_PROC_DUMP
    priv_mirror[priv->dev_id] = priv;
#endif

#ifdef VBG400_CONFIG_DLY_CALIB
    if (interface_mode == INTERFACE_RGMII)
        init_npu_gmac_regs(priv, interface_mode, reversed, priv->dev_id, VBG400_PRE_CONFIG);
#endif

    // Initialise the ISR/hard_start_xmit() lock
    spin_lock_init(&priv->tx_spinlock_);
    
    // Set hardware device base addresses
    priv->macBaseMasterSma = /*vaddr*/DEVICE_REG_ADDR_MAC1_START + MAC_BASE_OFFSET; /*only one master on the phy, set to gmac0, change if needed*/
	printk(KERN_INFO "probe: set macBaseMasterSma to GMAC 0, addr = 0x%08x\n",priv->macBaseMasterSma);
    priv->macBase = vaddr + MAC_BASE_OFFSET;
    priv->dmaBase = vaddr + DMA_BASE_OFFSET;
#ifndef CONFIG_VBG400_CHIPIT
#ifdef VBG400_SUPPORT_INTERFACE_REVERSED
    if ((reversed == MAC_REVERSED) || CONFIG_VBG400_MAC_GMAC_INDEX == INDEX_GMAC1)
#else
    if (CONFIG_VBG400_MAC_GMAC_INDEX == INDEX_GMAC1)
#endif
    {
        /*TODO: what about illegal configuration: "gmac_index == GMAC0 & reversed == YES"*/
        priv->macBaseMasterSma = DEVICE_REG_ADDR_MAC2_START + MAC_BASE_OFFSET;
		printk(KERN_INFO "probe: set macBaseMasterSma to GMAC 1, addr = 0x%08x\n",priv->macBaseMasterSma);
    }
#endif

    // Initialise IRQ ownership to not owned
    priv->have_irq = 1;

    // Lock protecting access to CoPro command queue functions or direct access
    // to the GMAC interrupt enable register if CoPro is not in use
    spin_lock_init(&priv->cmd_que_lock_);
    //Lock protecting access to NPU shared registers

    init_timer(&priv->watchdog_timer);
    priv->watchdog_timer.function = &watchdog_timer_action;
    priv->watchdog_timer.data = (unsigned long)priv;

#ifdef VBG400_DEBUG_LEN_ERROR_LOG
    init_timer(&priv->len_error_timer);
    priv->len_error_timer.function = &len_error_action;
    priv->len_error_timer.data = (unsigned long)priv;
    printk(KERN_INFO "probe: len_error_timer.data = = 0x%p\n",priv);
#endif

    // Set pointer to device in private data
    priv->netdev = netdev;
    priv->plat_dev = pdev;
    priv->device = &pdev->dev;

#ifdef VBG400_NPU_RESET_GMAC
    // Initialise the work queue entry to be used to configure NPU registers on giga change
    INIT_WORK(&priv->restart_gmac_work, restart_gmac);
#endif

	gmac_reset(netdev);

    /** Do something here to detect the present or otherwise of the MAC
     *  Read the version register as a first test */
    version = mac_reg_read(priv, MAC_VERSION_REG);
    synopsis_version = version & 0xff;
    vendor_version   = (version >> 8) & 0xff;

    /** Assume device is at the adr and irq specified until have probing working */
    netdev->base_addr  = vaddr;

    //disable RGMII interrupt:
    {
        u32 reg_contents;
        reg_contents = mac_reg_read(priv, MAC_INTERRUPT_MASK_REG);
        printk(KERN_INFO "gmac_up(): disable RGMII interrupt reg_contents = 0x%08x\n", reg_contents);
        reg_contents |= 0x00000001;
        mac_reg_write(priv, MAC_INTERRUPT_MASK_REG, reg_contents);
#ifndef VBG400_TEST_MMC_PMT_GLI_IN_IRQ
        /* Must clear MMC, PMT and GLI interrupts */
        mac_reg_read(priv, MAC_RGMII_STATUS_REG);
        process_non_dma_ints(priv, ( (1UL << DMA_STATUS_GPI_BIT) | (1UL << DMA_STATUS_GMI_BIT) | (1UL << DMA_STATUS_GLI_BIT) ));
#endif
    }
	/*register network for use in interrupt handler
	 *external interrupt vector style
	 */
	printk(KERN_INFO "probe: wave400_disable_irq (%d)\n",irqOut[priv->dev_id]);
	/*disable out interrupt because it is enabled (from u-boot?) and cause interrupt after GMAC reset below*/
	wave400_disable_irq(irqOut[priv->dev_id]);
	
	action->dev_id = netdev;
	resource = platform_get_resource(pdev, IORESOURCE_IRQ, 0);//get first occurence of IORESOURCE_IRQ type
	wave400_register_static_irq(resource->start, resource->end, action, irq_handlers[pdev->id]);
	netdev->irq  = resource->end;

	printk(KERN_INFO "register irqIn[%d] = %d and irqOut[%d] = %d, irq_handlers entry = 0x%p\n", 
						pdev->id, resource->start, pdev->id, resource->end, irq_handlers[pdev->id]);

    // Allocate the IRQ
    // Initialise the ethernet device with std. contents
    ether_setup(netdev);

    // Setup operations pointers
/*    netdev->open               = &open;
    netdev->hard_start_xmit    = &hard_start_xmit;
    netdev->stop               = &stop;
    netdev->get_stats          = &get_stats;
    netdev->change_mtu         = &change_mtu;
#ifdef CONFIG_NET_POLL_CONTROLLER
    netdev->poll_controller    = &netpoll;
#endif // CONFIG_NET_POLL_CONTROLLER
    netdev->set_mac_address    = &set_mac_address;
    netdev->set_multicast_list = &set_multicast_list;*/

	netdev->netdev_ops = &gmac_dev_ops;

	// Initialise NAPI support
	netif_napi_add(netdev, &priv->napi_struct, &poll, NAPI_POLL_WEIGHT);

   set_ethtool_ops(netdev);

#ifdef USE_RX_CSUM
    // Do TX H/W checksum and SG list processing
    netdev->features |= NETIF_F_HW_CSUM;
#endif
    netdev->features |= NETIF_F_SG;

    // We take care of our own TX locking
    netdev->features |= NETIF_F_LLTX;

    // Initialise PHY support
    priv->mii.phy_id_mask   = 0x1f;
    priv->mii.reg_num_mask  = 0x1f;
    priv->mii.force_media   = 0;
    priv->mii.full_duplex   = 1;
    priv->mii.using_10      = 0; //for CONFIG_VBG400_CHIPIT
    priv->mii.using_100     = 0;
    priv->mii.using_1000    = 1;
	priv->mii.using_pause   = 1;
    priv->mii.dev           = netdev;
    priv->mii.mdio_read     = phy_read;
    priv->mii.mdio_write    = phy_write;

#ifndef CONFIG_VBG400_CHIPIT
    priv->gmii_csr_clk_range = 4;
#else
    priv->gmii_csr_clk_range = 5; //Why 5 and not 2 ? (Arad)
#endif
#ifdef VBG400_USE_TX_NAPI
    priv->clean_rx = poll_rx;
#endif

    // Remember whether auto-negotiation is allowed
#ifdef ALLOW_AUTONEG
    priv->ethtool_cmd.autoneg = 1;
	priv->ethtool_pauseparam.autoneg = 1;
#else // ALLOW_AUTONEG
    priv->ethtool_cmd.autoneg = 0;
	priv->ethtool_pauseparam.autoneg = 0;
#endif // ALLOW_AUTONEG

    // Set up PHY mode for when auto-negotiation is not allowed
    priv->ethtool_cmd.speed = SPEED_1000;
    priv->ethtool_cmd.duplex = DUPLEX_FULL;
    priv->ethtool_cmd.port = PORT_MII;
    priv->ethtool_cmd.transceiver = XCVR_INTERNAL;

	// We can support both reception and generation of pause frames
	priv->ethtool_pauseparam.rx_pause = 1;
	priv->ethtool_pauseparam.tx_pause = 1;

    // Initialise the set of features we would like to advertise as being
	// available for negotiation
    priv->ethtool_cmd.advertising = (ADVERTISED_10baseT_Half |
                                     ADVERTISED_10baseT_Full |
                                     ADVERTISED_100baseT_Half |
                                     ADVERTISED_100baseT_Full |
#if defined(ALLOW_OX800_1000M)
                                     ADVERTISED_1000baseT_Half |
                                     ADVERTISED_1000baseT_Full |
									  ADVERTISED_Pause |
									  ADVERTISED_Asym_Pause |
#endif
#ifndef CONFIG_VBG400_CHIPIT
                                     ADVERTISED_1000baseT_Half |
                                     ADVERTISED_1000baseT_Full |
/* TODO:
* add 1000 only if not RMII
* if (interface_mode != INTERFACE_RMII) 
*   priv->ethtool_cmd.advertising |= (ADVERTISED_1000baseT_Half | ADVERTISED_1000baseT_Full)
*/
#endif
                                     ADVERTISED_Autoneg |
                                     ADVERTISED_MII);

    // Attempt to locate the PHY
    phy_detect(netdev);
    priv->ethtool_cmd.phy_address = priv->mii.phy_id;

    // Did we find a PHY?
	if (priv->phy_type == PHY_TYPE_NONE) {
		printk(KERN_WARNING "%s: No PHY found\n", netdev->name);
		err = ENXIO;
		goto probe_err_out;
    }

	// Find out what modes the PHY supports
	priv->ethtool_cmd.supported = get_phy_capabilities(priv);
    printk(KERN_INFO "probe() %s: priv->ethtool_cmd.supported = 0x%08x\n", netdev->name,priv->ethtool_cmd.supported);

    // Register the device with the network intrastructure
    err = register_netdev(netdev);
    if (err) {
        printk(KERN_ERR "probe() %s: Failed to register device\n", netdev->name);
        goto probe_err_out;
    }

    // Record details about the hardware we found
    printk(KERN_NOTICE "%s: GMAC ver = %u, vendor ver = %u at 0x%lx, IRQ %d\n", netdev->name, synopsis_version, vendor_version, netdev->base_addr, netdev->irq);
    printk(KERN_NOTICE "%s: Found PHY at address %u, type 0x%08x -> %s\n", priv->netdev->name, priv->phy_addr, priv->phy_type, (priv->ethtool_cmd.supported & SUPPORTED_1000baseT_Full) ? "10/100/1000" : "10/100");
    printk(KERN_NOTICE "%s: Ethernet addr: ", priv->netdev->name);
    for (i = 0; i < 5; i++) {
        printk("%02x:", netdev->dev_addr[i]);
    }
    printk("%02x\n", netdev->dev_addr[5]);
	priv->interface_up = 0;

    return 0;

probe_err_out:
    return err;
}

static int gmac_found_count = 0;
static struct net_device* gmac_netdev[MAX_GMAC_UNITS];
//Yan
#define DEVICE_REG_ADDR_MAC1_START  0xA7040000
#define DEVICE_REG_SIZE        010000
#define DEVICE_REG_ADDR_MAC1_END    DEVICE_REG_ADDR_MAC1_START + DEVICE_REG_SIZE -1

/* OPEN: can't write to PHY in case SMA master is MAC2, to change
*/
int write_mac_reg(struct file *file, const char __user *buffer,
                             unsigned long count, void *data)
{
	unsigned long reg = 0, val = 0, index = 0;

	sscanf(buffer, "%lu %lx %d", &reg, &val, &index);
	
	writel(val, (void*) ((index==0 ? DEVICE_REG_ADDR_MAC1_START : DEVICE_REG_ADDR_MAC2_START) + MAC_BASE_OFFSET + (reg << 2)));
	return count;
}

int write_dma_reg(struct file *file, const char __user *buffer,
                             unsigned long count, void *data)
{	
	unsigned long reg = 0, val = 0, index = 0;
	sscanf(buffer, "%lu %lx %d", &reg, &val, &index);
	writel(val, (void*) ((index==0 ? DEVICE_REG_ADDR_MAC1_START : DEVICE_REG_ADDR_MAC2_START) + DMA_BASE_OFFSET + (reg << 2)));
	return count;
}

int read_mac_reg(struct file *file, const char __user *buffer,
                             unsigned long count, void *data)
{
	int reg = 0, index = 0;

	sscanf(buffer, "%d %d", &reg, &index);
	printk(KERN_INFO "gmac:  reg=%d, index=%d\n",reg, index);
	printk(KERN_INFO "gmac: mac reg:%d addr:0x%x val:0x%x\n", reg, 
					(index==0 ? DEVICE_REG_ADDR_MAC1_START : DEVICE_REG_ADDR_MAC2_START) + MAC_BASE_OFFSET + (reg << 2), 
					readl((void*) ((index==0 ? DEVICE_REG_ADDR_MAC1_START : DEVICE_REG_ADDR_MAC2_START) + MAC_BASE_OFFSET + (reg << 2))));
	return count;
}

int read_dma_reg(struct file *file, const char __user *buffer,
                             unsigned long count, void *data)
{	
	int reg = 0, index = 0;

	sscanf(buffer, "%d %d", &reg, &index);
	printk(KERN_INFO "gmac: dma reg:%d addr:0x%x val:0x%x\n", reg,
					(index==0 ? DEVICE_REG_ADDR_MAC1_START : DEVICE_REG_ADDR_MAC2_START) + DMA_BASE_OFFSET + (reg << 2), 
					readl((void*) ((index==0 ? DEVICE_REG_ADDR_MAC1_START : DEVICE_REG_ADDR_MAC2_START) + DMA_BASE_OFFSET + (reg << 2))));
	return count;
}


#ifdef VBG400_SUPPORT_PROC_DUMP
int proc_dump_interface_regs(struct file *file, const char __user *buffer,
                             unsigned long count, void *data)
{
	int interface = 0;

	sscanf(buffer, "%d", &interface);
    if (interface==0 || interface==1)
        if (priv_mirror[interface])
            dump_mac_regs(priv_mirror[interface], priv_mirror[interface]->macBase, priv_mirror[interface]->dmaBase);
    else
        printk("Error: illegal interface, 0/1\n");
	return count;
}

int proc_dump_all_regs(struct file *file, const char __user *buffer,
                             unsigned long count, void *data)
{
	int interface = 0;

	sscanf(buffer, "%d", &interface);
    printk("* interface 0\n");
    if (priv_mirror[0])
        dump_mac_regs(priv_mirror[0], priv_mirror[0]->macBase, priv_mirror[0]->dmaBase);
    printk("\n* interface 1\n");
    if (priv_mirror[1])
        dump_mac_regs(priv_mirror[1], priv_mirror[0]->macBase, priv_mirror[0]->dmaBase);
	return count;
}
#endif

/**
 * External entry point to the driver, called from Space.c to detect a card
 */
static int synop_probe(struct platform_device *pdev)
{
   int err = 0;
	u32 the_register_resource_size;
   struct net_device *netdev = alloc_etherdev(sizeof(gmac_priv_t));
	struct proc_dir_entry *mac_proc_read, *mac_proc_write, *dma_proc_read, *dma_proc_write;
	struct resource *resource = NULL;
	struct resource *region = NULL;
	unsigned char ethernet_addr[6] = DEFAULT_MAC_ADDRESS;

   printk(KERN_NOTICE "Probing for Synopsis GMAC, pdev->id %d\n", pdev->id);
	if (get_ethernet_addr(ethernet_addr, pdev->id)) {
		return -ENODEV;
	}
	memcpy(netdev->dev_addr, ethernet_addr, 6);

	resource = pdev->resource;
	the_register_resource_size = resource->end - resource->start + 1;
	printk(KERN_INFO "gmac: mem region: start:0x%x size:%d name:%s\n",
		resource->start, the_register_resource_size, pdev->name);
	region = request_mem_region(resource->start, the_register_resource_size, pdev->name);

	if(!region)
	{
		printk(KERN_INFO ": gmac: request_mem_region() failed! dev:%s id:%d start:0x%x end:0x%x\n",
				pdev->name, pdev->id, resource->start, resource->end);
		return -ENOMEM;
	}

    // Will allocate private data later, as may want descriptors etc in special memory
    if (!netdev) {
        printk(KERN_WARNING "synopsys_gmac_probe() failed to alloc device\n");
        err = -ENODEV;
    } else {
            sprintf(netdev->name, "eth%d", pdev->id);
            err = probe(netdev, pdev, resource->start);
            if (err) 
			{
              printk(KERN_WARNING "synopsys_gmac_probe() Probing failed for %s, err = %d\n", netdev->name, err);
			}
            else 
              ++gmac_found_count;

        if (err) {
            netdev->reg_state = NETREG_UNREGISTERED;
            free_netdev(netdev);
        } else 
			gmac_netdev[pdev->id] = netdev;
    }

	if(!gmac_proc) {
		gmac_proc = proc_mkdir("driver/gmac", NULL);
#ifdef VBG400_SUPPORT_PROC_DUMP
	    mac_proc_read = create_proc_entry("dump_all_regs",  S_IFREG | S_IRUGO | S_IWUSR, gmac_proc);
	    mac_proc_read->write_proc = proc_dump_interface_regs;
	    mac_proc_read->read_proc = proc_dump_all_regs;
#endif
        mac_proc_read = create_proc_entry("read_mac_reg",  S_IFREG | S_IRUGO | S_IWUSR, gmac_proc);
        mac_proc_read->write_proc = read_mac_reg;
        mac_proc_read->read_proc = NULL;
        dma_proc_read = create_proc_entry("read_dma_reg",  S_IFREG | S_IRUGO | S_IWUSR, gmac_proc);
        dma_proc_read->write_proc = read_dma_reg;
        dma_proc_read->read_proc = NULL;
        mac_proc_write = create_proc_entry("write_mac_reg",  S_IFREG | S_IRUGO | S_IWUSR, gmac_proc);
        mac_proc_write->write_proc = write_mac_reg;
        mac_proc_write->read_proc = NULL;
        dma_proc_write = create_proc_entry("write_dma_reg",  S_IFREG | S_IRUGO | S_IWUSR, gmac_proc);
        dma_proc_write->write_proc = write_dma_reg;
        dma_proc_write->read_proc = NULL;
	}

    return err;
}

static int __init gmac_module_init(void)
{	
	int err = platform_driver_register(&plat_driver);

	if (err) {
		printk(KERN_WARNING "gmac_module_init() Failed to register platform driver\n");
		return err;
	}

	return 0;
}
module_init(gmac_module_init);

static void __exit gmac_module_cleanup(void)
{
	int i = 0;
	for (i=0; i < gmac_found_count; i++) {
		struct net_device* netdev = gmac_netdev[i];
		gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(netdev);

		unregister_netdev(netdev);
		netdev->reg_state = NETREG_UNREGISTERED;
		free_netdev(netdev);
		platform_device_unregister(priv->plat_dev);
	}

	platform_driver_unregister(&plat_driver);
}
module_exit(gmac_module_cleanup);
