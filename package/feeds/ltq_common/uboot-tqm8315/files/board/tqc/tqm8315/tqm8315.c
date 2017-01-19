/*
 * Copyright (C) 2009 TQ-Systems GmbH
 *
 * Author: Thomas Waehner <thomas.waehner@tqs.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS for A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <i2c.h>
#include <libfdt.h>
#include <fdt_support.h>
#include <pci.h>
#include <mpc83xx.h>
#include <netdev.h>
#include <sata.h>
#include <asm/io.h>
#include <asm/fsl_serdes.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_83XX_GENERIC_PCIE)
#if !defined(CONFIG_FSL_SERDES)
#error PCIe operation needs CONFIG_FSL_SERDES
#endif
#endif

#define PHYCTRLCFG_REFCLK_MASK		0x00000070
#define PHYCTRLCFG_REFCLK_125MHZ	0x00000070

int board_early_init_f(void)
{
#ifdef CONFIG_FSL_SERDES
	/* Setup SERDES for PCIe operation */
	fsl_setup_serdes(CONFIG_FSL_SERDES1, FSL_SERDES_PROTO_PEX,
			 FSL_SERDES_CLK_100, FSL_SERDES_VDD_1V);
#endif /* CONFIG_FSL_SERDES */
	return 0;
}

int checkboard(void)
{
	puts("Board: TQM8315\n");
	return 0;
}

static struct pci_region pci_regions[] = {
	{
		.bus_start = CONFIG_SYS_PCI_MEM_BASE,
		.phys_start = CONFIG_SYS_PCI_MEM_PHYS,
		.size = CONFIG_SYS_PCI_MEM_SIZE,
		.flags = PCI_REGION_MEM | PCI_REGION_PREFETCH
	},
	{
		.bus_start = CONFIG_SYS_PCI_MMIO_BASE,
		.phys_start = CONFIG_SYS_PCI_MMIO_PHYS,
		.size = CONFIG_SYS_PCI_MMIO_SIZE,
		.flags = PCI_REGION_MEM
	},
	{
		.bus_start = CONFIG_SYS_PCI_IO_BASE,
		.phys_start = CONFIG_SYS_PCI_IO_PHYS,
		.size = CONFIG_SYS_PCI_IO_SIZE,
		.flags = PCI_REGION_IO
	}
};

#ifdef CONFIG_83XX_GENERIC_PCIE
static struct pci_region pcie_regions_0[] = {
	{
		.bus_start = CONFIG_SYS_PCIE1_MEM_BASE,
		.phys_start = CONFIG_SYS_PCIE1_MEM_PHYS,
		.size = CONFIG_SYS_PCIE1_MEM_SIZE,
		.flags = PCI_REGION_MEM,
	},
	{
		.bus_start = CONFIG_SYS_PCIE1_IO_BASE,
		.phys_start = CONFIG_SYS_PCIE1_IO_PHYS,
		.size = CONFIG_SYS_PCIE1_IO_SIZE,
		.flags = PCI_REGION_IO,
	},
	{
		.size = 0,
	}
};

static struct pci_region pcie_regions_1[] = {
	{
		.bus_start = CONFIG_SYS_PCIE2_MEM_BASE,
		.phys_start = CONFIG_SYS_PCIE2_MEM_PHYS,
		.size = CONFIG_SYS_PCIE2_MEM_SIZE,
		.flags = PCI_REGION_MEM,
	},
	{
		.bus_start = CONFIG_SYS_PCIE2_IO_BASE,
		.phys_start = CONFIG_SYS_PCIE2_IO_PHYS,
		.size = CONFIG_SYS_PCIE2_IO_SIZE,
		.flags = PCI_REGION_IO,
	},
	{
		.size = 0,
	}
};
#endif

void pci_init_board(void)
{
	volatile immap_t *immr = (volatile immap_t *)CONFIG_SYS_IMMR;
	volatile clk83xx_t *clk = (volatile clk83xx_t *)&immr->clk;
	volatile law83xx_t *pci_law = immr->sysconf.pcilaw;
	struct pci_region *reg[] = { pci_regions };
#ifdef CONFIG_83XX_GENERIC_PCIE
	volatile sysconf83xx_t *sysconf = &immr->sysconf;
	volatile law83xx_t *pcie_law = sysconf->pcielaw;
	struct pci_region *pcie_reg[] = { pcie_regions_0, pcie_regions_1, };
#endif

	/* Enable all 3 PCI_CLK_OUTPUTs. */
	clk->occr |= 0xe0000000;

	/*
	 * Configure PCI Local Access Windows
	 */
	pci_law[0].bar = CONFIG_SYS_PCI_MEM_PHYS & LAWBAR_BAR;
	pci_law[0].ar = LBLAWAR_EN | LBLAWAR_512MB;

	pci_law[1].bar = CONFIG_SYS_PCI_IO_PHYS & LAWBAR_BAR;
	pci_law[1].ar = LBLAWAR_EN | LBLAWAR_1MB;

	mpc83xx_pci_init(1, reg, 0);

#ifdef CONFIG_83XX_GENERIC_PCIE
	/* Configure the clock for PCIE controller */
	clrsetbits_be32(&clk->sccr, SCCR_PCIEXP1CM | SCCR_PCIEXP2CM,
			 SCCR_PCIEXP1CM_1 | SCCR_PCIEXP2CM_1);

	/* Deassert the resets in the control register */
	out_be32(&sysconf->pecr1, 0xE0008000);
	out_be32(&sysconf->pecr2, 0xE0008000);
	udelay(2000);

	/* Configure PCI Express Local Access Windows */
	out_be32(&pcie_law[0].bar, CONFIG_SYS_PCIE1_BASE & LAWBAR_BAR);
	out_be32(&pcie_law[0].ar, LBLAWAR_EN | LBLAWAR_512MB);

	out_be32(&pcie_law[1].bar, CONFIG_SYS_PCIE2_BASE & LAWBAR_BAR);
	out_be32(&pcie_law[1].ar, LBLAWAR_EN | LBLAWAR_512MB);

	mpc83xx_pcie_init(2, pcie_reg, 0);
#endif
}

#ifdef CONFIG_LAST_STAGE_INIT
int last_stage_init(void)
{
#ifdef CONFIG_FSL_SATA
	/* Init SATA PHY CLK rate which must only be done once at startup */
	init_mpc8315_sata_phy();
#endif
	return 0;
}
#endif

#if defined(CONFIG_OF_BOARD_SETUP)
void ft_board_setup(void *blob, bd_t *bd)
{
	ft_cpu_setup(blob, bd);
#ifdef CONFIG_PCI
	ft_pci_setup(blob, bd);
#endif
}
#endif

int board_eth_init(bd_t *bis)
{
	cpu_eth_init(bis);	/* Initialize TSECs first */
	return pci_eth_init(bis);
}
