/*
 * Copyright (C) 2009 TQ-Systems GmbH
 *
 * Author: Thomas Waehner <thomas.waehner@tqs.de>
 *
 * Copyright (C) 2007 Freescale Semiconductor, Inc.
 *
 * Authors: Nick.Spence@freescale.com
 *          Wilson.Lo@freescale.com
 *          scottwood@freescale.com
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
#include <mpc83xx.h>
#include <spd_sdram.h>

#include <asm/bitops.h>
#include <asm/io.h>

#include <asm/processor.h>

DECLARE_GLOBAL_DATA_PTR;

/* Fixed sdram init -- doesn't use serial presence detect.
 *
 * This is useful for faster booting in configs where the RAM is unlikely
 * to be changed, or for things like NAND booting where space is tight.
 */
static long fixed_sdram(void)
{
	volatile immap_t *im = (volatile immap_t *)CONFIG_SYS_IMMR;
	u32 msize = CONFIG_SYS_DDR_SIZE * 1024 * 1024;
	u32 msize_log2 = __ilog2(msize);
	volatile u32 law;
	int i;

	/* configure local access window for SDRAM address range */
	im->sysconf.ddrlaw[0].bar = CONFIG_SYS_DDR_SDRAM_BASE  & 0xfffff000;
	im->sysconf.ddrlaw[0].ar = LBLAWAR_EN | (msize_log2 - 1);
	/*
	 * According to MPC8315ERM Rev. 1 read back last configured local
	 * access window configuration register, followed by an isync.
	 */
	law = im->sysconf.ddrlaw[0].ar;
	isync();

	im->sysconf.ddrcdr = CONFIG_SYS_DDRCDR_VALUE;
	/*
	 * Erratum DDR3 requires a 50ms delay after clearing DDRCDR[DDR_cfg],
	 * or the DDR2 controller may fail to initialize correctly.
	 */
	for (i = 0; i < 50; i++)
		udelay(1000);

	im->ddr.csbnds[0].csbnds = (msize - 1) >> 24;
	im->ddr.cs_config[0] = CONFIG_SYS_DDR_CS0_CONFIG;

	/* Currently we use only one CS, so disable the other bank. */
	im->ddr.cs_config[1] = 0;

	im->ddr.sdram_clk_cntl = CONFIG_SYS_DDR_SDRAM_CLK_CNTL;
	im->ddr.timing_cfg_3 = CONFIG_SYS_DDR_TIMING_3;
	im->ddr.timing_cfg_1 = CONFIG_SYS_DDR_TIMING_1;
	im->ddr.timing_cfg_2 = CONFIG_SYS_DDR_TIMING_2;
	im->ddr.timing_cfg_0 = CONFIG_SYS_DDR_TIMING_0;

	if (im->pmc.pmccr1 & PMCCR1_POWER_OFF)
		im->ddr.sdram_cfg = CONFIG_SYS_DDR_SDRAM_CFG | SDRAM_CFG_BI;
	else
		im->ddr.sdram_cfg = CONFIG_SYS_DDR_SDRAM_CFG;

	im->ddr.sdram_cfg2 = CONFIG_SYS_DDR_SDRAM_CFG2;
	im->ddr.sdram_mode = CONFIG_SYS_DDR_MODE;
	im->ddr.sdram_mode2 = CONFIG_SYS_DDR_MODE2;
	im->ddr.sdram_md_cntl = CONFIG_SYS_DDR_SDRAM_MD_CNTL;
	im->ddr.sdram_interval = CONFIG_SYS_DDR_INTERVAL;
	eieio();

	/*
	 * According to Freescale's application note AN2583 Rev. 8 wait 200 us
	 * after the DLL is locked before enabling the DDR controller.
	 */
	udelay(200);

	/* enable DDR controller */
	im->ddr.sdram_cfg |= SDRAM_CFG_MEM_EN;
	eieio();

	return msize;
}

phys_size_t initdram(int board_type)
{
	volatile immap_t *im = (volatile immap_t *)CONFIG_SYS_IMMR;
	u32 msize;

	if ((im->sysconf.immrbar & IMMRBAR_BASE_ADDR) != (u32)im)
		return -1;

	/* DDR SDRAM */
	msize = fixed_sdram();

	/* return total bus SDRAM size(bytes)  -- DDR */
	return msize;
}
