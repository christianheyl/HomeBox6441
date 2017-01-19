/*
 * linux/arch/arm/mach-oxnas/gmac_phy.c
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
#include <linux/delay.h>
#include "gmac.h"
#include "gmac_phy.h"
#include "gmac_reg.h"
#include "wave400_chadr.h"
#include "wave400_interrupt.h"

static const int PHY_TRANSFER_TIMEOUT_MS = 100;
extern unsigned int MT_RdReg(unsigned int unit, unsigned int reg);
extern void MT_WrReg(unsigned int unit, unsigned int reg, unsigned int data);
extern void MT_WrRegMask(unsigned int unit, unsigned int reg, unsigned int mask,unsigned int data);

atomic_t read_atomic=ATOMIC_INIT(0);
atomic_t write_atomic=ATOMIC_INIT(0);
EXPORT_SYMBOL(read_atomic);
EXPORT_SYMBOL(write_atomic);

/*Two new handlers to properly access SMA bus,
* Only one bus that has to be controlled by one MAC.
* macBaseMasterSma is set according to the MAC that controlls it.
* It is true only to Address and Data registers of the MAC.
*/
inline void mac_reg_write_sma_master(gmac_priv_t* priv, int reg_num, u32 value)
{
#ifdef VBG400_USE_SMA_SELECT
    writel(value, (void*) (priv->macBase + (reg_num << 2)));
#else
#ifndef CONFIG_VBG400_CHIPIT
    writel(value, (void*) (priv->macBaseMasterSma + (reg_num << 2)));
#else
    writel(value, (void*) (priv->macBase + (reg_num << 2)));
#endif
#endif
}

inline u32 mac_reg_read_sma_master(gmac_priv_t* priv, int reg_num)
{
#ifdef VBG400_USE_SMA_SELECT
    return readl((void*) (priv->macBase + (reg_num << 2)));
#else
#ifndef CONFIG_VBG400_CHIPIT
    return readl((void*) (priv->macBaseMasterSma + (reg_num << 2)));
#else
    return readl((void*) (priv->macBase + (reg_num << 2)));
#endif
#endif
}

/*
 * Reads a register from the MII Management serial interface
 */
int phy_read(struct net_device *dev, int phyaddr, int phyreg)
{
    int data;
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    unsigned long end;

#ifdef VBG400_USE_PHY_ATOMIC
    atomic_inc(&read_atomic);
    if (atomic_read(&read_atomic) != 1)
    {
        atomic_dec(&read_atomic);
        printk( KERN_ERR "phy_read: Error in read_atomic !!!!!!!!!!!!!!!!!!\n");
        return;
    }
#endif

#ifdef VBG400_USE_SMA_SELECT
    /*sm master*/
    data=MT_RdReg(WAVE400_SHARED_GMAC_BASE_ADDR, WAVE400_GMAC_MODE_REG_OFFSET);
    //printk(KERN_INFO "phy_read: priv->dev_id=%d, data = 0x%08x\n",priv->dev_id, data);
    if (priv->dev_id==1)
    {
        data |= 0x10000000;
    } 
    else {
        data &= 0xe7ffffff;
    }
    MT_WrReg(WAVE400_SHARED_GMAC_BASE_ADDR, WAVE400_GMAC_MODE_REG_OFFSET, data);
    //printk(KERN_INFO "phy_read: set with = 0x%08x\n",data);
#endif

    u32 addr = (phyaddr << MAC_GMII_ADR_PA_BIT) |
               (phyreg << MAC_GMII_ADR_GR_BIT) |
               (priv->gmii_csr_clk_range << MAC_GMII_ADR_CR_BIT) |
               (1UL << MAC_GMII_ADR_GB_BIT);

    mac_reg_write_sma_master(priv, MAC_GMII_ADR_REG, addr);

    end = jiffies + MS_TO_JIFFIES(PHY_TRANSFER_TIMEOUT_MS);
    while (time_before(jiffies, end)) {
        if (!(mac_reg_read_sma_master(priv, MAC_GMII_ADR_REG) & (1UL << MAC_GMII_ADR_GB_BIT))) {
            // Successfully read from PHY
            data = mac_reg_read_sma_master(priv, MAC_GMII_DATA_REG) & 0xFFFF;
            break;
        }
    }

#ifdef VBG400_USE_PHY_ATOMIC
    atomic_dec(&read_atomic);
#endif
    DBG(1, KERN_INFO "phy_read() %s: phyaddr=0x%x, phyreg=0x%x, phydata=0x%x\n", dev->name, phyaddr, phyreg, data);
    return data;
}

/*
 * Writes a register to the MII Management serial interface
 */
void phy_write(struct net_device *dev, int phyaddr, int phyreg, int phydata)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    unsigned long end;

    int data;
#ifdef VBG400_USE_PHY_ATOMIC
    atomic_inc(&write_atomic);
    if (atomic_read(&write_atomic) != 1)
    {
        atomic_dec(&write_atomic);
        printk( KERN_ERR "phy_write: Error in write_atomic !!!!!!!!!!!!!!!!!!\n");
        return;
    }
#endif

#ifdef VBG400_USE_SMA_SELECT
    /*sm master*/
    data=MT_RdReg(WAVE400_SHARED_GMAC_BASE_ADDR, WAVE400_GMAC_MODE_REG_OFFSET);
    //printk(KERN_INFO "phy_write: priv->dev_id=%d, data = 0x%08x\n",priv->dev_id, data);
    if (priv->dev_id==1)
    {
        data |= 0x10000000;
    } 
    else {
        data &= 0xe7ffffff;
    }
    MT_WrReg(WAVE400_SHARED_GMAC_BASE_ADDR, WAVE400_GMAC_MODE_REG_OFFSET, data);
//    printk(KERN_INFO "phy_write: set with = 0x%08x\n",data);
#endif

    u32 addr = (phyaddr << MAC_GMII_ADR_PA_BIT) |
               (phyreg << MAC_GMII_ADR_GR_BIT) |
               (priv->gmii_csr_clk_range << MAC_GMII_ADR_CR_BIT) |
               (1UL << MAC_GMII_ADR_GW_BIT) |
               (1UL << MAC_GMII_ADR_GB_BIT);

    mac_reg_write_sma_master(priv, MAC_GMII_DATA_REG, phydata);
    mac_reg_write_sma_master(priv, MAC_GMII_ADR_REG, addr);

    end = jiffies + MS_TO_JIFFIES(PHY_TRANSFER_TIMEOUT_MS);
    while (time_before(jiffies, end)) {
        if (!(mac_reg_read_sma_master(priv, MAC_GMII_ADR_REG) & (1UL << MAC_GMII_ADR_GB_BIT))) {
            break;
        }
    }
#ifdef VBG400_USE_PHY_ATOMIC
    atomic_dec(&write_atomic);
#endif
    DBG(1, KERN_INFO "phy_write() %s: phyaddr=0x%x, phyreg=0x%x, phydata=0x%x\n", dev->name, phyaddr, phyreg, phydata);
}

/*
 * Finds and reports the PHY address
 */
void phy_detect(struct net_device *dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    unsigned int id1, id2;
    int phyaddr=0;

#ifndef CONFIG_VBG400_CHIPIT
    /* TODO:
    *  add new config - PHY address, use it to configure phyaddr
    */
    priv->phy_type = 0;
    DBG(2, KERN_INFO "phy_detect() %s: Entered\n", priv->netdev->name);
    printk(KERN_INFO "phy_detect() %s: Entered\n", priv->netdev->name);

    // Read the PHY identifiers
    if (priv->dev_id == 0) {
        id1 = phy_read(priv->netdev, 0 & 31, MII_PHYSID1);
        id2 = phy_read(priv->netdev, 0 & 31, MII_PHYSID2);
        phyaddr = 0;
    }
    else {
        id1 = phy_read(priv->netdev, 5 & 31, MII_PHYSID1);
        id2 = phy_read(priv->netdev, 5 & 31, MII_PHYSID2);
        phyaddr = 5;
    }
	printk(KERN_INFO "phy_detect() %s: phy_id1=0x%x, phy_id2=0x%x\n", priv->netdev->name, id1, id2);
    if (id1 != 0x0000 && id1 != 0xffff && id1 != 0x8000 &&
        id2 != 0x0000 && id2 != 0xffff && id2 != 0x8000) {
        DBG(2, KERN_NOTICE "phy_detect() %s: Found PHY at address = %u\n", priv->netdev->name, phyaddr);
        printk(KERN_NOTICE "phy_detect() %s: Found PHY at address = %u\n", priv->netdev->name, phyaddr);
        priv->mii.phy_id = phyaddr & 31;
        priv->phy_type = id1 << 16 | id2;
        priv->phy_addr = phyaddr;
    }
#else
    priv->phy_type = 0;
    for (phyaddr = 0/*1*/; phyaddr < 33; ++phyaddr) {
        // Read the PHY identifiers
        id1 = phy_read(priv->netdev, phyaddr & 31, MII_PHYSID1);
        id2 = phy_read(priv->netdev, phyaddr & 31, MII_PHYSID2);

		printk(KERN_INFO "phy_detect() %s: PHY adr = %u -> phy_id1=0x%x, phy_id2=0x%x\n", priv->netdev->name, phyaddr, id1, id2);
        DBG(2, KERN_INFO "phy_detect() %s: PHY adr = %u -> phy_id1=0x%x, phy_id2=0x%x\n", priv->netdev->name, phyaddr, id1, id2);

        // Make sure it is a valid identifier
        if (id1 != 0x0000 && id1 != 0xffff && id1 != 0x8000 &&
            id2 != 0x0000 && id2 != 0xffff && id2 != 0x8000) {
            if (phyaddr!=0)
            {
                DBG(2, KERN_NOTICE "phy_detect() %s: Found PHY at address = %u\n", priv->netdev->name, phyaddr);
                priv->mii.phy_id = phyaddr & 31;
                priv->phy_type = id1 << 16 | id2;
                priv->phy_addr = phyaddr;
            }
            break;
        }
    }
#endif
}

void start_phy_reset(gmac_priv_t* priv)
{
    // Ask the PHY to reset
    phy_write(priv->netdev, priv->phy_addr, MII_BMCR, BMCR_RESET);
}

int is_phy_reset_complete(gmac_priv_t* priv)
{
    int complete = 0;
    int bmcr;

    // Read back the status until it indicates reset, or we timeout
    bmcr = phy_read(priv->netdev, priv->phy_addr, MII_BMCR);
    if (!(bmcr & BMCR_RESET)) {
        complete = 1;
    }

    return complete;
}

int phy_reset(struct net_device *dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    int complete = 0;
    unsigned long end;

    // Start the reset operation
    start_phy_reset(priv);

    // Total time to wait for reset to complete
    end = jiffies + MS_TO_JIFFIES(PHY_TRANSFER_TIMEOUT_MS);

    // Should apparently wait at least 50mS before reading back from PHY; this
    // could just be a nasty feature of the SMC91x MAC/PHY and not apply to us
    msleep(50);
    // Read back the status until it indicates reset, or we timeout
    while (!(complete = is_phy_reset_complete(priv)) && time_before(jiffies, end)) {
        msleep(1);
    }

    return !complete;
}

void phy_powerdown(struct net_device *dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);

    unsigned int bmcr = phy_read(dev, priv->phy_addr, MII_BMCR);
    phy_write(dev, priv->phy_addr, MII_BMCR, bmcr | BMCR_PDOWN);

	if (priv->phy_type == PHY_TYPE_ICPLUS_IP1001) {
		// Cope with weird ICPlus PHY behaviour
		phy_read(dev, priv->phy_addr, MII_BMCR);
	}
}

void set_phy_negotiate_mode(struct net_device *dev)
{
    gmac_priv_t        *priv = (gmac_priv_t*)netdev_priv(dev);
    struct mii_if_info *mii = &priv->mii;
    struct ethtool_cmd *ecmd = &priv->ethtool_cmd;
    u32                 bmcr;

	bmcr = mii->mdio_read(dev, mii->phy_id, MII_BMCR);

printk(KERN_INFO "set_phy_negotiate_mode() ecmd->advertising = 0x%08x\n",ecmd->advertising);
    if (ecmd->autoneg == AUTONEG_ENABLE) {
        u32 advert, tmp;
        u32 advert2 = 0, tmp2 = 0;

        // Advertise only what has been requested
        advert = mii->mdio_read(dev, mii->phy_id, MII_ADVERTISE);
        tmp = advert & ~(ADVERTISE_ALL | ADVERTISE_100BASE4 |
		                 ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM);

        if (ecmd->supported & (SUPPORTED_1000baseT_Full | ADVERTISE_1000HALF)) {
            advert2 = mii->mdio_read(dev, mii->phy_id, MII_CTRL1000);
            tmp2 = advert2 & ~(ADVERTISE_1000HALF | ADVERTISE_1000FULL);
        }

        if (ecmd->advertising & ADVERTISED_10baseT_Half) {
            tmp |= ADVERTISE_10HALF;
        }
        if (ecmd->advertising & ADVERTISED_10baseT_Full) {
            tmp |= ADVERTISE_10FULL;
        }
        if (ecmd->advertising & ADVERTISED_100baseT_Half) {
            tmp |= ADVERTISE_100HALF;
        }
        if (ecmd->advertising & ADVERTISED_100baseT_Full) {
            tmp |= ADVERTISE_100FULL;
        }
        if ((ecmd->supported & SUPPORTED_1000baseT_Half) &&
            (ecmd->advertising & ADVERTISED_1000baseT_Half)) {
                tmp2 |= ADVERTISE_1000HALF;
        }
        if ((ecmd->supported & SUPPORTED_1000baseT_Full) &&
            (ecmd->advertising & ADVERTISED_1000baseT_Full)) {
                tmp2 |= ADVERTISE_1000FULL;
        }

        if (ecmd->advertising & ADVERTISED_Pause) {
            tmp |= ADVERTISE_PAUSE_CAP;
        }
        if (ecmd->advertising & ADVERTISED_Asym_Pause) {
            tmp |= ADVERTISE_PAUSE_ASYM;
        }

        if (advert != tmp) {
            mii->mdio_write(dev, mii->phy_id, MII_ADVERTISE, tmp);
            mii->advertising = tmp;
        }
        if (advert2 != tmp2) {
            mii->mdio_write(dev, mii->phy_id, MII_CTRL1000, tmp2);
        }

        // Auto-negotiate the link state
        bmcr |= (BMCR_ANRESTART | BMCR_ANENABLE);
        mii->mdio_write(dev, mii->phy_id, MII_BMCR, bmcr);
    } else {
        u32 tmp;

        // Turn off auto negotiation, set speed and duplicitly unilaterally
        tmp = bmcr & ~(BMCR_ANENABLE | BMCR_SPEED100 | BMCR_SPEED1000 | BMCR_FULLDPLX);
        if (ecmd->speed == SPEED_1000) {
            tmp |= BMCR_SPEED1000;
        } else if (ecmd->speed == SPEED_100) {
            tmp |= BMCR_SPEED100;
        }

        if (ecmd->duplex == DUPLEX_FULL) {
            tmp |= BMCR_FULLDPLX;
            mii->full_duplex = 1;
        } else {
            mii->full_duplex = 0;
        }

        if (bmcr != tmp) {
            mii->mdio_write(dev, mii->phy_id, MII_BMCR, tmp);
        }
    }
}

u32 get_phy_capabilities(gmac_priv_t* priv)
{
    struct mii_if_info *mii = &priv->mii;
	// Ask the PHY for it's capabilities
	u32 reg = mii->mdio_read(priv->netdev, mii->phy_id, MII_BMSR);
    int i;

	// Assume PHY has MII interface
	u32 features = SUPPORTED_MII;
#if 0
	if ((priv->mii.mdio_read(priv->mii.dev, priv->mii.phy_id, MII_BMSR) & BMSR_LSTATUS) == 0)
	{
		for (i=0; i<5; i++) {
			mdelay(1000);
			printk(KERN_INFO "get_phy_capabilities: %d\n",i);
			if (priv->mii.mdio_read(priv->mii.dev, priv->mii.phy_id, MII_BMSR) & BMSR_LSTATUS)
				break;
		}
		if (i>=5) {
			printk(KERN_INFO "get_phy_capabilities: link down\n");
		}
	}
#endif
	if (reg & BMSR_ANEGCAPABLE) {
		features |= SUPPORTED_Autoneg;
	}
	if (reg & BMSR_100FULL) {
		features |= SUPPORTED_100baseT_Full;
	}
	if (reg & BMSR_100HALF) {
		features |= SUPPORTED_100baseT_Half;
	}
	if (reg & BMSR_10FULL) {
		features |= SUPPORTED_10baseT_Full;
	}
	if (reg & BMSR_10HALF) {
		features |= SUPPORTED_10baseT_Half;
	}

	// Does the PHY have the extended status register?
	if (reg & BMSR_ESTATEN) {
		reg = mii->mdio_read(priv->netdev, mii->phy_id, MII_ESTATUS);

		if (reg & ESTATUS_1000_TFULL)
			features |= SUPPORTED_1000baseT_Full;
		if (reg & ESTATUS_1000_THALF)
			features |= SUPPORTED_1000baseT_Half;
	}

	return features;
}
