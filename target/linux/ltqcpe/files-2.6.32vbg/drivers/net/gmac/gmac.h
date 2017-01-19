/*
 * linux/arch/arm/mach-oxnas/gmac.h
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
#if !defined(__GMAC_H__)
#define __GMAC_H__

#include <linux/semaphore.h>
#include <asm/types.h>
#include <linux/mii.h>
#include <linux/netdevice.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/ethtool.h>
#include <linux/kobject.h>
#include "desc_alloc.h"

//#define GMAC_DEBUG

#define MAC_BASE 0x0000			// The Mac Base address offset is 0x0000
#define DMA_BASE 0x1000			// Dma base address starts with an offset 0x1000

#define VBG400_DEBUG_LEN_ERROR

#ifdef GMAC_DEBUG
#define DBG(n, args...)\
    do {\
        if ((n) <= priv->msg_level)\
            printk(args);\
    } while (0)
#else
#define DBG(n, args...)   do { } while(0)
#endif

#define MS_TO_JIFFIES(x) (((x) < (1000/(HZ))) ? 1 : (x) * (HZ) / 1000)

//#define USE_RX_CSUM
#define USE_HW_FLOW_CTL

typedef struct gmac_desc_list_info {
    volatile gmac_dma_desc_t *base_ptr;
	gmac_dma_desc_t          *shadow_ptr;
    int                       num_descriptors;
    int                       empty_count;
    int                       full_count;
    int                       r_index;
    int                       w_index;
} gmac_desc_list_info_t;

typedef struct tx_frag_info {
    dma_addr_t  phys_adr;
    u16         length;
} tx_frag_info_t;


// Private data structure for the GMAC driver
typedef struct gmac_priv {
    /** Base address of GMAC MAC registers */
    u32                       macBase;
    /** Base address of GMAC DMA registers */
    u32                       dmaBase;
    /** Base address of SMA Master- use this address to access Address and Data registers */
    u32                       macBaseMasterSma ;

    struct net_device*        netdev;

    struct net_device_stats   stats;

    u32                       msg_level;
    
    /** Whether we own an IRQ */
    int                       have_irq;

	/* Device number (MAC0,1 etc) */
	int						  dev_id;
    /** Pointer to outstanding tx packet that has not yet been queued due to
     *  lack of descriptors */
    struct sk_buff           *tx_pending_skb;
    tx_frag_info_t            tx_pending_fragments[18];
    int                       tx_pending_fragment_count;

    /** DMA consistent physical address of outstanding tx packet */
    dma_addr_t                tx_pending_dma_addr;
    unsigned long             tx_pending_length;

    /** To synchronise ISR and thread TX activities' access to private data */
    spinlock_t                tx_spinlock_;

    /** To synchronise access to the PHY */
    spinlock_t                phy_lock;

    /** The timer for NAPI polling when out of memory when trying to fill RX
     *  descriptor ring */

    /** PHY related info */
    struct mii_if_info        mii;
    struct ethtool_cmd        ethtool_cmd;
	struct ethtool_pauseparam ethtool_pauseparam;
    u32                       phy_addr;
    u32                       phy_type;
    int                       gmii_csr_clk_range;

    /** Periodic timer to check link status etc */
    struct timer_list         watchdog_timer;
    volatile int              watchdog_timer_shutdown;
#ifdef VBG400_DEBUG_LEN_ERROR
    struct timer_list         len_error_timer;
#endif
    /** The number of descriptors in the gmac_dma_desc_t array holding both the TX and
     *  RX descriptors. The TX descriptors reside at the start of the array */
    unsigned                  total_num_descriptors;
    /** The CPU accessible virtual address of the start of the descriptor array */
    gmac_dma_desc_t*          desc_tx_addr;
    gmac_dma_desc_t*          desc_rx_addr;
	void*					  desc_tx_vaddr; //kmalloc
	void*					  desc_rx_vaddr; //kmalloc

    /** The hardware accessible physical address of the start of the descriptor array */
    dma_addr_t                desc_tx_dma_addr;
    dma_addr_t                desc_rx_dma_addr;

    /** Descriptor list management */
    gmac_desc_list_info_t     tx_gmac_desc_list_info;
    gmac_desc_list_info_t     rx_gmac_desc_list_info;
    
    /** Record of disabling RX overflow interrupts */
    unsigned                  rx_overflow_ints_disabled;

    /** The result of the last H/W DMA generated checksum operation */
    u16                       tx_csum_result_;

    /** Whether we deal in jumbo frames */
    int                       jumbo_;

    volatile int              mii_init_media;
    volatile int              phy_force_negotiation;

    spinlock_t       cmd_que_lock_;
	u32				  rx_buffer_size_;
	int				  rx_buffers_per_page;
	gmac_dma_desc_t *tx_desc_shadow_;
	gmac_dma_desc_t *rx_desc_shadow_;

	struct napi_struct napi_struct;

	/** sysfs dir tree root for recovery button driver */
	struct kset        link_state_kset;
	struct kobject     link_state_kobject;
	struct work_struct link_state_change_work;
	int                link_state;

	struct device *device;
	struct platform_device *plat_dev;
	int napi_enabled;
	int interface_up;
//#ifndef CONFIG_VBG400_CHIPIT
	struct work_struct restart_gmac_work;
    struct tasklet_struct tasklet_tx;
    bool (*clean_rx)(struct net_device *dev, int *work_done, int budget);
//#endif
} gmac_priv_t;

#endif        //  #if !defined(__GMAC_H__)

