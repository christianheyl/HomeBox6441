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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * High Level Configuration Options
 */
#define CONFIG_E300		1	/* E300 family */
#define CONFIG_MPC83xx		1	/* MPC83xx family */
#define CONFIG_MPC831x		1	/* MPC831x CPU family */
#define CONFIG_MPC8315		1	/* MPC8315 CPU specific */
#define CONFIG_TQM8315		1	/* TQM8315 board specific */

/*
 * System Clock Setup
 */
#define CONFIG_83XX_CLKIN	66666667 /* in Hz */
#define CONFIG_SYS_CLK_FREQ	CONFIG_83XX_CLKIN

/*
 * Hardware Reset Configuration Word
 * if CLKIN is 66.666 MHz, then
 * CSB = 133.333 MHz, CORE = 400 MHz, DDR-Contr. inp. = 266.666 MHz
 */
#define CONFIG_SYS_HRCW_LOW (\
	HRCWL_LCL_BUS_TO_SCB_CLK_1X1 |\
	HRCWL_DDR_TO_SCB_CLK_2X1 |\
	HRCWL_SVCOD_DIV_2 |\
	HRCWL_CSB_TO_CLKIN_2X1 |\
	HRCWL_CORE_TO_CSB_3X1)
#define CONFIG_SYS_HRCW_HIGH (\
	HRCWH_PCI_HOST |\
	HRCWH_PCI1_ARBITER_ENABLE |\
	HRCWH_CORE_ENABLE |\
	HRCWH_FROM_0X00000100 |\
	HRCWH_BOOTSEQ_DISABLE |\
	HRCWH_SW_WATCHDOG_DISABLE |\
	HRCWH_ROM_LOC_LOCAL_16BIT |\
	HRCWH_RL_EXT_LEGACY |\
	HRCWH_TSEC1M_IN_RGMII |\
	HRCWH_TSEC2M_IN_RGMII |\
	HRCWH_BIG_ENDIAN |\
	HRCWH_LALE_NORMAL)

/*
 * System IO Config
 */
#define CONFIG_SYS_SICRH		0x00000000
#define CONFIG_SYS_SICRL		0x00000000 /* 3.3V, no delay */

#define CONFIG_BOARD_EARLY_INIT_F	/* call board_pre_init */
#define CONFIG_LAST_STAGE_INIT		/* call last_stage_init for SATA init */

/*
 * IMMR new address
 */
#define CONFIG_SYS_IMMR		0xE0000000

/*
 * Arbiter Setup
 */
#define CONFIG_SYS_ACR_PIPE_DEP	3	/* Arbiter pipeline depth is 4 */
#define CONFIG_SYS_ACR_RPTCNT	3	/* Arbiter repeat count is 4 */
#define CONFIG_SYS_SPCR_TSECEP	3	/* eTSEC emergency prio is highest */

/*
 * DDR Setup
 */
#define CONFIG_SYS_DDR_BASE		0x00000000 /* DDR is system memory */
#define CONFIG_SYS_SDRAM_BASE		CONFIG_SYS_DDR_BASE
#define CONFIG_SYS_DDR_SDRAM_BASE	CONFIG_SYS_DDR_BASE
#define CONFIG_SYS_DDR_SDRAM_CLK_CNTL	DDR_SDRAM_CLK_CNTL_CLK_ADJUST_05
#define CONFIG_SYS_DDRCDR_VALUE		0x00080001

/*
 * Manually set up DDR parameters
 */
#define CONFIG_SYS_DDR_SIZE		256 /* MiB */
#define CONFIG_SYS_DDR_CS0_BNDS		0x0000000F

#define CONFIG_SYS_DDR_CS0_CONFIG	(CSCONFIG_EN \
					| CSCONFIG_ODT_WR_ALL \
					| CSCONFIG_BANK_BIT_3 \
					| CSCONFIG_ROW_BIT_13 \
					| CSCONFIG_COL_BIT_10)
					/* 0x80044102 */
#define CONFIG_SYS_DDR_TIMING_3		0x00000000

#define CONFIG_SYS_DDR_TIMING_0	((0 << TIMING_CFG0_RWT_SHIFT) \
				| (0 << TIMING_CFG0_WRT_SHIFT) \
				| (0 << TIMING_CFG0_RRT_SHIFT) \
				| (0 << TIMING_CFG0_WWT_SHIFT) \
				| (1 << TIMING_CFG0_ACT_PD_EXIT_SHIFT) \
				| (1 << TIMING_CFG0_PRE_PD_EXIT_SHIFT) \
				| (1 << TIMING_CFG0_ODT_PD_EXIT_SHIFT) \
				| (5 << TIMING_CFG0_MRS_CYC_SHIFT))
				/* 0x00110105 */
#define CONFIG_SYS_DDR_TIMING_1	((2 << TIMING_CFG1_PRETOACT_SHIFT) \
				| (6 << TIMING_CFG1_ACTTOPRE_SHIFT) \
				| (2 << TIMING_CFG1_ACTTORW_SHIFT) \
				| (5 << TIMING_CFG1_CASLAT_SHIFT) \
				| (9 << TIMING_CFG1_REFREC_SHIFT) \
				| (2 << TIMING_CFG1_WRREC_SHIFT) \
				| (2 << TIMING_CFG1_ACTTOACT_SHIFT) \
				| (2 << TIMING_CFG1_WRTORD_SHIFT))
				/* 0x26259222 */
#define CONFIG_SYS_DDR_TIMING_2	((1 << TIMING_CFG2_ADD_LAT_SHIFT) \
				| (3 << TIMING_CFG2_CPO_SHIFT) \
				| (2 << TIMING_CFG2_WR_LAT_DELAY_SHIFT) \
				| (2 << TIMING_CFG2_RD_TO_PRE_SHIFT) \
				| (2 << TIMING_CFG2_WR_DATA_DELAY_SHIFT) \
				| (3 << TIMING_CFG2_CKE_PLS_SHIFT) \
				| (7 << TIMING_CFG2_FOUR_ACT_SHIFT))
				/* 0x119048c7 */
#define CONFIG_SYS_DDR_INTERVAL	((520 << SDRAM_INTERVAL_REFINT_SHIFT) \
				| (0x0100 << SDRAM_INTERVAL_BSTOPRE_SHIFT))
				/* 0x02080100 */

#define CONFIG_SYS_DDR_SDRAM_CFG	(SDRAM_CFG_SREN \
					| SDRAM_CFG_SDRAM_TYPE_DDR2 \
					| SDRAM_CFG_32_BE \
					| SDRAM_CFG_HSE)
					/* 0x43080008 */
#define CONFIG_SYS_DDR_SDRAM_CFG2	0x00401000
#define CONFIG_SYS_DDR_MODE		((0x440e << SDRAM_MODE_ESD_SHIFT) \
					| (0x0232 << SDRAM_MODE_SD_SHIFT))
					/* 0x440e0232 */
#define CONFIG_SYS_DDR_MODE2		0x8000C000
#define CONFIG_SYS_DDR_SDRAM_MD_CNTL	0x00000000

/*
 * Memory test
 */
#undef CONFIG_SYS_DRAM_TEST		/* memory test, takes time */
#define CONFIG_SYS_ALT_MEMTEST
#define CONFIG_SYS_MEMTEST_START	0x00100000 /* memtest region */
#define CONFIG_SYS_MEMTEST_END		0x00200000

/*
 * The reserved memory
 */
#define CONFIG_SYS_MONITOR_BASE TEXT_BASE /* start of monitor */

#if (CONFIG_SYS_MONITOR_BASE < CONFIG_SYS_FLASH_BASE)
 #define CONFIG_SYS_RAMBOOT
#else
 #undef CONFIG_SYS_RAMBOOT
#endif

#define CONFIG_SYS_MONITOR_LEN	(512 * 1024)	/* Reserve 512 KiB for Mon*/
#define CONFIG_SYS_MALLOC_LEN	(512 * 1024)	/* Reserved for malloc */

/*
 * Initial RAM Base Address Setup
 */
#define CONFIG_SYS_INIT_RAM_LOCK	1
#define CONFIG_SYS_INIT_RAM_ADDR	0xF0000000 /* Initial RAM address */
#define CONFIG_SYS_INIT_RAM_END		0x1000	/* End of used area in RAM */
#define CONFIG_SYS_GBL_DATA_SIZE	0x100	/* num bytes initial data */
#define CONFIG_SYS_GBL_DATA_OFFSET	(CONFIG_SYS_INIT_RAM_END \
						- CONFIG_SYS_GBL_DATA_SIZE)

/*
 * Local Bus Configuration & Clock Setup
 */
#define CONFIG_SYS_LCRR			(LCRR_DBYP | LCRR_CLKDIV_2)
#define CONFIG_SYS_LBC_LBCR		0x00040000

#define CONFIG_SYS_FLASH_CFI		/* use the Common Flash Interface */
#define CONFIG_FLASH_CFI_DRIVER		/* use the CFI driver */
#define CONFIG_SYS_FLASH_CFI_WIDTH	FLASH_CFI_16BIT
#define CONFIG_SYS_FLASH_USE_BUFFER_WRITE

#define CONFIG_SYS_FLASH_BASE		0x80000000 /* FLASH base address */
#define CONFIG_SYS_FLASH_SIZE		64	/* FLASH size in MiB */
#define CONFIG_SYS_FLASH_PROTECTION	1	/* Use h/w Flash protection */

#define CONFIG_SYS_LBLAWBAR0_PRELIM	CONFIG_SYS_FLASH_BASE
#define CONFIG_SYS_LBLAWAR0_PRELIM	0x8000001D /* 1 GiB window size */

#define CONFIG_SYS_BR0_PRELIM	(CONFIG_SYS_FLASH_BASE	/* Flash Base addr */ \
				| (2 << BR_PS_SHIFT)	/* 16 bit port size */ \
				| BR_V)			/* valid */
				/* 0x80001001 */
/* Flash timing for LBC = 66.666 MHz */
#define CONFIG_SYS_OR0_PRELIM	((~(CONFIG_SYS_FLASH_SIZE - 1) << 20) \
				| OR_GPCM_CSNT \
				| OR_GPCM_SCY_3 \
				| OR_GPCM_TRLX)
				/* 0xFC000834 */

#define CONFIG_SYS_MAX_FLASH_BANKS	2 /* number of banks */
#define CONFIG_SYS_FLASH_BANKS_LIST	{CONFIG_SYS_FLASH_BASE, \
					(CONFIG_SYS_FLASH_BASE \
					+ ((CONFIG_SYS_FLASH_SIZE / 2) << 20))}
#define CONFIG_SYS_MAX_FLASH_SECT	259

#undef CONFIG_SYS_FLASH_CHECKSUM
#define CONFIG_SYS_FLASH_ERASE_TOUT	60000	/* Flash Erase Timeout (ms) */
#define CONFIG_SYS_FLASH_WRITE_TOUT	500	/* Flash Write Timeout (ms) */

/*
 * NAND Flash on the Local Bus
 * define CONFIG_NAND to use it
 */
#undef CONFIG_NAND

#ifdef CONFIG_NAND
 #define CONFIG_SYS_NAND_BASE		0xE0600000
 #define CONFIG_SYS_MAX_NAND_DEVICE	1
 #define CONFIG_MTD_NAND_VERIFY_WRITE	1
 #define CONFIG_NAND_FSL_ELBC		1

 /*
  * Use HW ECC
  * Port Size = 8 bit
  * MSEL = FCM
  */
 #define CONFIG_SYS_BR1_PRELIM	(CONFIG_SYS_NAND_BASE \
 				| (2 << BR_DECC_SHIFT) \
 				| BR_PS_8 \
 				| BR_MS_FCM \
 				| BR_V)
 #define CONFIG_SYS_OR1_PRELIM	(0xFFFF8000 /* address mask 32 KiB */ \
 				| OR_FCM_PGS \
 				| OR_FCM_CSCT \
 				| OR_FCM_CST \
 				| OR_FCM_CHT \
 				| OR_FCM_SCY_1 \
 				| OR_FCM_RST)

 #define CONFIG_SYS_LBLAWBAR1_PRELIM	CONFIG_SYS_NAND_BASE
 #define CONFIG_SYS_LBLAWAR1_PRELIM	0x8000000E	/* 32 KiB */
#endif

/*
 * Serial Port
 */
#define CONFIG_SERIAL_MULTI
#define CONFIG_CONS_INDEX		2
#undef CONFIG_SERIAL_SOFTWARE_FIFO
#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE	1
#define CONFIG_SYS_NS16550_CLK		get_bus_freq(0)

#define CONFIG_SYS_BAUDRATE_TABLE \
	{300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200}

#define CONFIG_SYS_NS16550_COM1	(CONFIG_SYS_IMMR+0x4500)
#define CONFIG_SYS_NS16550_COM2	(CONFIG_SYS_IMMR+0x4600)

/* Use the HUSH parser */
#define CONFIG_SYS_HUSH_PARSER
#ifdef CONFIG_SYS_HUSH_PARSER
 #define CONFIG_SYS_PROMPT_HUSH_PS2 "> "
#endif

/* Pass open firmware flat tree */
#define CONFIG_OF_LIBFDT		1
#define CONFIG_OF_BOARD_SETUP		1
#define CONFIG_OF_STDOUT_VIA_ALIAS	1

/* I2C */
#define CONFIG_HARD_I2C			/* I2C with hardware support */
#define CONFIG_FSL_I2C
#define CONFIG_SYS_I2C_SPEED		400000 /* I2C speed and slave address */
#define CONFIG_SYS_I2C_SLAVE		0x7F
#define CONFIG_SYS_I2C_OFFSET		0x3000
#define CONFIG_SYS_I2C2_OFFSET		0x3100
#define CONFIG_SYS_I2C_NOPROBES	{0x50}	/* Don't probe config eeprom */

/* EEPROM */
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN		2
#define CONFIG_SYS_EEPROM_PAGE_WRITE_BITS	5 /* 32 bytes per write */
#define CONFIG_SYS_EEPROM_PAGE_WRITE_ENABLE
#define CONFIG_SYS_EEPROM_PAGE_WRITE_DELAY_MS	12 /* 10ms +/- 20% */
#define CONFIG_SYS_I2C_MULTI_EEPROMS		1 /* more than one eeprom */

/*
 * Config on-board RTC
 */
#define CONFIG_RTC_DS1337
#define CONFIG_SYS_I2C_RTC_ADDR 0x68

/* I2C SYSMON (LM75) */
#define CONFIG_DTT_LM75			1	/* ON Semi's LM75 */
#define CONFIG_SYS_I2C_DTT_ADDR		0x48	/* Sensor base address */
/* Use the following define if you only want the TQM8315 on-module sensor */
#define CONFIG_DTT_SENSORS		{0}
/*
 * Use the following define if it is on a STK85xxNG
 * to read the temp sensors there, too
 */
/* #define CONFIG_DTT_SENSORS {0, 1, 2, 3} */
#define CONFIG_SYS_DTT_MAX_TEMP		70
#define CONFIG_SYS_DTT_LOW_TEMP		-30
#define CONFIG_SYS_DTT_HYSTERESIS	3

/*
 * General PCI
 * Addresses are mapped 1-1.
 */
#define CONFIG_PCI

#ifdef CONFIG_PCI
 #define CONFIG_83XX_GENERIC_PCI	1	/* Use generic PCI setup */
 #define CONFIG_83XX_GENERIC_PCIE	1
#endif

#ifdef CONFIG_83XX_GENERIC_PCI
 #define CONFIG_SYS_PCI_MEM_BASE	0xC0000000
 #define CONFIG_SYS_PCI_MEM_PHYS	CONFIG_SYS_PCI_MEM_BASE
 #define CONFIG_SYS_PCI_MEM_SIZE	0x10000000	/* 256 MiB */
 #define CONFIG_SYS_PCI_MMIO_BASE	0xD0000000
 #define CONFIG_SYS_PCI_MMIO_PHYS	CONFIG_SYS_PCI_MMIO_BASE
 #define CONFIG_SYS_PCI_MMIO_SIZE	0x10000000	/* 256 MiB */
 #define CONFIG_SYS_PCI_IO_BASE	(CONFIG_SYS_IMMR + 0x03000000)
 #define CONFIG_SYS_PCI_IO_PHYS	CONFIG_SYS_PCI_IO_BASE
 #define CONFIG_SYS_PCI_IO_SIZE	0x100000	/* 1 MiB */

 #define CONFIG_SYS_PCI_SLV_MEM_LOCAL	CONFIG_SYS_SDRAM_BASE
 #define CONFIG_SYS_PCI_SLV_MEM_BUS	0x00000000
 #define CONFIG_SYS_PCI_SLV_MEM_SIZE	0x80000000

 #define CONFIG_NET_MULTI
 #define CONFIG_PCI_PNP		/* do pci plug-and-play */

 #define CONFIG_EEPRO100		/* Intel Pro/100 NIC support */
 #define CONFIG_E1000			/* Intel Pro/1000 NIC support */
 #define CONFIG_PCI_SCAN_SHOW		/* show pci devices on startup */
 #define CONFIG_SYS_PCI_SUBSYS_VENDORID	0x1957	/* Freescale */

 #ifndef CONFIG_NET_MULTI
 #define CONFIG_NET_MULTI		1
#endif

#ifdef CONFIG_83XX_GENERIC_PCIE
 #define CONFIG_83XX_GENERIC_PCIE_REGISTER_HOSES

 #define CONFIG_SYS_PCIE1_BASE		0x40000000
 #define CONFIG_SYS_PCIE1_CFG_BASE	0x50000000
 #define CONFIG_SYS_PCIE1_CFG_SIZE	0x01000000
 #define CONFIG_SYS_PCIE1_MEM_BASE	0x40000000
 #define CONFIG_SYS_PCIE1_MEM_PHYS	0x40000000
 #define CONFIG_SYS_PCIE1_MEM_SIZE	0x10000000
 #define CONFIG_SYS_PCIE1_IO_BASE	0x00000000
 #define CONFIG_SYS_PCIE1_IO_PHYS	(CONFIG_SYS_IMMR + 0x03100000)
 #define CONFIG_SYS_PCIE1_IO_SIZE	0x00800000

 #define CONFIG_SYS_PCIE2_BASE		0x60000000
 #define CONFIG_SYS_PCIE2_CFG_BASE	0x70000000
 #define CONFIG_SYS_PCIE2_CFG_SIZE	0x01000000
 #define CONFIG_SYS_PCIE2_MEM_BASE	0x60000000
 #define CONFIG_SYS_PCIE2_MEM_PHYS	0x60000000
 #define CONFIG_SYS_PCIE2_MEM_SIZE	0x10000000
 #define CONFIG_SYS_PCIE2_IO_BASE	0x00000000
 #define CONFIG_SYS_PCIE2_IO_PHYS	(CONFIG_SYS_IMMR + 0x03180000)
 #define CONFIG_SYS_PCIE2_IO_SIZE	0x00800000
 #endif
#endif

#define CONFIG_HAS_FSL_DR_USB
#define CONFIG_SYS_SCCR_USBDRCM 1

#ifdef CONFIG_HAS_FSL_DR_USB
 #define CONFIG_USB_EHCI
 #define CONFIG_USB_EHCI_FSL
 #define CONFIG_EHCI_HCD_INIT_AFTER_RESET
 #define CONFIG_DOS_PARTITION
 #define CONFIG_USB_STORAGE
#endif

/*
 * TSEC
 */
#define CONFIG_TSEC_ENET	/* TSEC ethernet support */
#define CONFIG_SYS_TSEC1_OFFSET	0x24000
#define CONFIG_SYS_TSEC1	(CONFIG_SYS_IMMR+CONFIG_SYS_TSEC1_OFFSET)
#define CONFIG_SYS_TSEC2_OFFSET	0x25000
#define CONFIG_SYS_TSEC2	(CONFIG_SYS_IMMR+CONFIG_SYS_TSEC2_OFFSET)

/*
 * TSEC ethernet configuration
 */
#define CONFIG_MII		1 /* MII PHY management */
#define CONFIG_TSEC1		1
#define CONFIG_TSEC1_NAME	"eTSEC0"
#define CONFIG_TSEC2		1
#define CONFIG_TSEC2_NAME	"eTSEC1"
#define TSEC1_PHY_ADDR		0x02
#define TSEC2_PHY_ADDR		0x01
#define TSEC1_PHYIDX		0
#define TSEC2_PHYIDX		0
#define TSEC1_FLAGS		(TSEC_GIGABIT | TSEC_REDUCED)
#define TSEC2_FLAGS		(TSEC_GIGABIT | TSEC_REDUCED)

/* Options are: eTSEC[0-1] */
#define CONFIG_ETHPRIME		"eTSEC0"

/* SERDES */
#define CONFIG_FSL_SERDES
#define CONFIG_FSL_SERDES1	0xe3000

/*
 * SATA
 */
#define CONFIG_LIBATA
#define CONFIG_FSL_SATA
/* Choose the phy setting for your SATA clock
 * The following values are possible for CONFIG_SYS_SATA_CLK:
 *  PHYCTRLCFG_REFCLK_50MHZ
 *  PHYCTRLCFG_REFCLK_75MHZ
 *  PHYCTRLCFG_REFCLK_100MHZ
 *  PHYCTRLCFG_REFCLK_125MHZ
 *  PHYCTRLCFG_REFCLK_150MHZ
 */
#define CONFIG_SYS_SATA_CLK PHYCTRLCFG_REFCLK_125MHZ


#define CONFIG_SYS_SATA_MAX_DEVICE	2
#define CONFIG_SATA1
#define CONFIG_SYS_SATA1_OFFSET		0x18000
#define CONFIG_SYS_SATA1		(CONFIG_SYS_IMMR \
						+ CONFIG_SYS_SATA1_OFFSET)
#define CONFIG_SYS_SATA1_FLAGS		FLAGS_DMA
#define CONFIG_SATA2
#define CONFIG_SYS_SATA2_OFFSET		0x19000
#define CONFIG_SYS_SATA2		(CONFIG_SYS_IMMR \
						+ CONFIG_SYS_SATA2_OFFSET)
#define CONFIG_SYS_SATA2_FLAGS		FLAGS_DMA

#ifdef CONFIG_FSL_SATA
 #define CONFIG_LBA48
 #define CONFIG_DOS_PARTITION
#endif

/*
 * MTD
 */
#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS

/*
 * Environment
 */
#ifndef CONFIG_SYS_RAMBOOT
 #define CONFIG_ENV_IS_IN_FLASH	1
 #define CONFIG_ENV_SECT_SIZE	0x20000 /* 128 KiB (one sector) per env */
 #define CONFIG_ENV_ADDR	(CONFIG_SYS_MONITOR_BASE \
					+ CONFIG_SYS_MONITOR_LEN) /* env1 */
 #define CONFIG_ENV_ADDR_REDUND	(CONFIG_ENV_ADDR \
 					+ CONFIG_ENV_SECT_SIZE) /* env2 */
 #define CONFIG_ENV_SIZE	0x2000
#else
 #define CONFIG_SYS_NO_FLASH	1	/* Flash is not usable now */
 #define CONFIG_ENV_IS_NOWHERE	1	/* Store ENV in memory only */
 #define CONFIG_ENV_ADDR	(CONFIG_SYS_MONITOR_BASE - 0x1000)
 #define CONFIG_ENV_SIZE	0x2000
#endif

#define CONFIG_LOADS_ECHO		1	/* echo for serial download */
#define CONFIG_SYS_LOADS_BAUD_CHANGE	1	/* allow baudrate change */

/*
 * BOOTP options
 */
#define CONFIG_BOOTP_BOOTFILESIZE
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME

/*
 * Command line configuration.
 */
#include <config_cmd_default.h>

#define CONFIG_CMD_MII
#define CONFIG_CMD_PCI
#define CONFIG_CMD_PING

#ifdef CONFIG_MTD_PARTITIONS
 #define CONFIG_CMD_MTDPARTS
 #define MTDIDS_DEFAULT		"nor0=80000000.flash"
 #define MTDPARTS_DEFAULT	"mtdparts=80000000.flash:"	\
				"512k(uboot)ro,"		\
				"128k(env1)ro,"			\
				"128k(env2)ro,"			\
				"128k(dtb),"			\
				"2m(kernel),"			\
				"15m(root),"			\
				"-(user)"
#endif

#if defined(CONFIG_HARD_I2C) || defined(CONFIG_SOFT_I2C)

 #define CONFIG_CMD_DATE
 #define CONFIG_CMD_DTT
 #define CONFIG_CMD_EEPROM
 #define CONFIG_CMD_I2C
#endif

#ifdef CONFIG_HAS_FSL_DR_USB
 #define CONFIG_CMD_USB
#endif

#ifdef CONFIG_NAND
 #define CONFIG_CMD_NAND
#endif

#ifndef CONFIG_CMD_ELF
 #define CONFIG_CMD_ELF
#endif

#if defined(CONFIG_SYS_RAMBOOT)
 #undef CONFIG_CMD_ENV
 #undef CONFIG_CMD_LOADS
#endif

#ifdef CONFIG_FSL_SATA
 #define CONFIG_CMD_SATA
#endif

#if defined(CONFIG_HAS_FSL_DR_USB) || defined(CONFIG_FSL_SATA)
 #define CONFIG_CMD_EXT2
 #define CONFIG_CMD_FAT
#endif

#define CONFIG_CMDLINE_EDITING	1	/* add command line history */

/*
 * Miscellaneous configurable options
 */
#undef CONFIG_WATCHDOG			/* watchdog disabled */

#define CONFIG_SYS_LONGHELP	/* undef to save memory */
#define CONFIG_SYS_LOAD_ADDR	0x2000000 /* default load address */
#define CONFIG_SYS_PROMPT	"=> " /* Monitor Command Prompt */

#if defined(CONFIG_CMD_KGDB)
 #define CONFIG_SYS_CBSIZE	1024	/* Console I/O Buffer Size */
#else
 #define CONFIG_SYS_CBSIZE	256	/* Console I/O Buffer Size */
#endif

/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS	16	/* max number of command args */
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE
#define CONFIG_SYS_HZ		1000	/* decrementer freq: 1ms ticks */

/*
 * For booting Linux, the board info and command line data
 * have to be in the first 8 MiB of memory, since this is
 * the maximum mapped by the Linux kernel during initialization.
 */
#define CONFIG_SYS_BOOTMAPSZ	(8 << 20) /* Initial Memory map for Linux */

/*
 * Core HID Setup
 */
#define CONFIG_SYS_HID0_INIT	0x000000000
#define CONFIG_SYS_HID0_FINAL	(HID0_ENABLE_MACHINE_CHECK | \
				HID0_ENABLE_DYNAMIC_POWER_MANAGMENT)
#define CONFIG_SYS_HID2		HID2_HBE

/*
 * MMU Setup
 */
#define CONFIG_HIGH_BATS	1	/* High BATs supported */

/* IMMRBAR, PCI IO and NAND: cache-inhibit and guarded */
#define CONFIG_SYS_IBAT0L	(CONFIG_SYS_IMMR \
				| BATL_PP_10 \
				| BATL_CACHEINHIBIT \
				| BATL_GUARDEDSTORAGE)
#define CONFIG_SYS_IBAT0U	(CONFIG_SYS_IMMR \
				| BATU_BL_256M \
				| BATU_VS \
				| BATU_VP)
#define CONFIG_SYS_DBAT0L	CONFIG_SYS_IBAT0L
#define CONFIG_SYS_DBAT0U	CONFIG_SYS_IBAT0U

/* FLASH: icache cacheable, but dcache-inhibit and guarded */
#define CONFIG_SYS_IBAT1L	(CONFIG_SYS_FLASH_BASE \
				| BATL_PP_10 \
				| BATL_MEMCOHERENCE)
#define CONFIG_SYS_IBAT1U	(CONFIG_SYS_FLASH_BASE \
				| BATU_BL_256M \
				| BATU_VS \
				| BATU_VP)
#define CONFIG_SYS_DBAT1L	(CONFIG_SYS_FLASH_BASE \
				| BATL_PP_10 \
				| BATL_CACHEINHIBIT \
				| BATL_GUARDEDSTORAGE)
#define CONFIG_SYS_DBAT1U	CONFIG_SYS_IBAT1U

/* Stack in dcache: cacheable, no memory coherence */
#define CONFIG_SYS_IBAT2L	(CONFIG_SYS_INIT_RAM_ADDR \
				| BATL_PP_10)
#define CONFIG_SYS_IBAT2U	(CONFIG_SYS_INIT_RAM_ADDR \
				| BATU_BL_128K \
				| BATU_VS \
				| BATU_VP)
#define CONFIG_SYS_DBAT2L	CONFIG_SYS_IBAT2L
#define CONFIG_SYS_DBAT2U	CONFIG_SYS_IBAT2U

/* DDR: 512MiB cache cacheable */
#define CONFIG_SYS_IBAT3L	(CONFIG_SYS_SDRAM_BASE \
				| BATL_PP_10 \
				| BATL_MEMCOHERENCE)
#define CONFIG_SYS_IBAT3U	(CONFIG_SYS_SDRAM_BASE \
				| BATU_BL_256M \
				| BATU_VS \
				| BATU_VP)
#define CONFIG_SYS_DBAT3L	CONFIG_SYS_IBAT3L
#define CONFIG_SYS_DBAT3U	CONFIG_SYS_IBAT3U

#define CONFIG_SYS_IBAT4L	(CONFIG_SYS_SDRAM_BASE + 0x10000000 \
				| BATL_PP_10 \
				| BATL_MEMCOHERENCE)
#define CONFIG_SYS_IBAT4U	(CONFIG_SYS_SDRAM_BASE + 0x10000000 \
				| BATU_BL_256M \
				| BATU_VS \
				| BATU_VP)
#define CONFIG_SYS_DBAT4L	CONFIG_SYS_IBAT4L
#define CONFIG_SYS_DBAT4U	CONFIG_SYS_IBAT4U

#ifdef CONFIG_PCI
 /* PCI MEM space: cacheable */
 #define CONFIG_SYS_IBAT5L	(CONFIG_SYS_PCI_MEM_PHYS \
 				| BATL_PP_10 \
 				| BATL_MEMCOHERENCE)
 #define CONFIG_SYS_IBAT5U	(CONFIG_SYS_PCI_MEM_PHYS \
 				| BATU_BL_256M \
 				| BATU_VS \
 				| BATU_VP)
 #define CONFIG_SYS_DBAT5L	CONFIG_SYS_IBAT5L
 #define CONFIG_SYS_DBAT5U	CONFIG_SYS_IBAT5U

 /* PCI MMIO space: cache-inhibit and guarded */
 #define CONFIG_SYS_IBAT6L	(CONFIG_SYS_PCI_MMIO_PHYS \
 				| BATL_PP_10 \
 				| BATL_CACHEINHIBIT \
 				| BATL_GUARDEDSTORAGE)
 #define CONFIG_SYS_IBAT6U	(CONFIG_SYS_PCI_MMIO_PHYS \
 				| BATU_BL_256M \
 				| BATU_VS \
 				| BATU_VP)
 #define CONFIG_SYS_DBAT6L	CONFIG_SYS_IBAT6L
 #define CONFIG_SYS_DBAT6U	CONFIG_SYS_IBAT6U

 #define CONFIG_SYS_IBAT7L	0
 #define CONFIG_SYS_IBAT7U	0
 #define CONFIG_SYS_DBAT7L	CONFIG_SYS_IBAT7L
 #define CONFIG_SYS_DBAT7U	CONFIG_SYS_IBAT7U
#else
 #define CONFIG_SYS_IBAT5L	0
 #define CONFIG_SYS_IBAT5U	0
 #define CONFIG_SYS_DBAT5L	CONFIG_SYS_IBAT5L
 #define CONFIG_SYS_DBAT5U	CONFIG_SYS_IBAT5U
 #define CONFIG_SYS_IBAT6L	0
 #define CONFIG_SYS_IBAT6U	0
 #define CONFIG_SYS_DBAT6L	CONFIG_SYS_IBAT6L
 #define CONFIG_SYS_DBAT6U	CONFIG_SYS_IBAT6U
 #define CONFIG_SYS_IBAT7L	0
 #define CONFIG_SYS_IBAT7U	0
 #define CONFIG_SYS_DBAT7L	CONFIG_SYS_IBAT7L
 #define CONFIG_SYS_DBAT7U	CONFIG_SYS_IBAT7U
#endif

/*
 * Internal Definitions
 *
 * Boot Flags
 */
#define BOOTFLAG_COLD	0x01 /* Normal Power-On: Boot from FLASH */
#define BOOTFLAG_WARM	0x02 /* Software reboot */

#if defined(CONFIG_CMD_KGDB)
 #define CONFIG_KGDB_BAUDRATE	230400	/* speed of kgdb serial port */
 #define CONFIG_KGDB_SER_INDEX	2	/* which serial port to use */
#endif

/*
 * Environment Configuration
 */
#define CONFIG_ENV_OVERWRITE

#if defined(CONFIG_TSEC_ENET)
 #define CONFIG_HAS_ETH0
 #define CONFIG_HAS_ETH1
#endif

#define CONFIG_BAUDRATE 115200

#define CONFIG_LOADADDR 500000	/* default location for tftp and bootm */

#define CONFIG_BOOTDELAY 6	/* -1 disables auto-boot */
#undef CONFIG_BOOTARGS		/* the boot command will set bootargs */

#define CONFIG_EXTRA_ENV_SETTINGS \
	"netdev=eth0\0" \
	"consoledev=ttyS1\0" \
	"usb_phy_type=utmi\0" \
	"hostname=TQM8315\0" \
	"ubootaddr_f=" MK_STR(CONFIG_SYS_FLASH_BASE) "\0" \
	"fdtaddr_f=800C0000\0" \
	"kerneladdr_f=800E0000\0" \
	"jf_start_f=802E0000\0" \
	"kernelfile=openwrt-mpc83xx-uImage\0" \
	"fdtfile=openwrt-mpc83xx-easy85600.dtb\0" \
	"jf_file=easy85600_128k.jffs\0" \
	"updkrn=tftp $loadaddr $tftppath$kernelfile;" \
		"protect off $kerneladdr_f +$filesize;" \
		"erase $kerneladdr_f +$filesize;" \
		"cp.b $loadaddr $kerneladdr_f $filesize\0" \
	"updfdt=tftp $loadaddr $tftppath$fdtfile;" \
		"protect off $fdtaddr_f +$filesize;" \
		"erase $fdtaddr_f +$filesize;" \
		"cp.b $loadaddr $fdtaddr_f $filesize\0" \
	"upd_jf=tftp $loadaddr $tftppath$jf_file;" \
		"protect off $jf_start_f +$filesize;" \
		"erase $jf_start_f +$filesize;" \
		"cp.b $loadaddr $jf_start_f $filesize\0" \
	"update_linux=run updfdt; run updkrn; run upd_jf\0" \
	"rootpath=/exports/your_nfs_location\0" \
	"addip=set bootargs ${bootargs} " \
		"ip=${ipaddr}:${serverip}:${gatewayip}:${netmask}:${hostname}:${netdev}:off\0" \
	"flashargs=set bootargs init=/rcinit root=${mtdroot} rootfstype=jffs2 ${mtdparts}\0" \
	"nfsargs=set bootargs init=/rcinit root=/dev/nfs rw nfsroot=${serverip}:${rootpath}\0" \
	"flash_flash=" CONFIG_FLASHBOOTCOMMAND "\0" \
	"flash_nfs=" CONFIG_NFSBOOTCOMMAND "\0" \
	MTDPARTS_DEFAULT "\0" \
	"mtdroot=1f05\0" \
	""

#define CONFIG_NFSBOOTCOMMAND \
	"set serverip $nfsserverip; " \
	"run nfsargs addip; " \
	"console=$consoledev,$baudrate; " \
	"bootm ${kerneladdr_f} - ${fdtaddr_f}"

#define CONFIG_FLASHBOOTCOMMAND \
	"run flashargs addip;bootm ${kerneladdr_f} - ${fdtaddr_f}"

#define CONFIG_BOOTCOMMAND "run flash_flash"

#endif	/* __CONFIG_H */
