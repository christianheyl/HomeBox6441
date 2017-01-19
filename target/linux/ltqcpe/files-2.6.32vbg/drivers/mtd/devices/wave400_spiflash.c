/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <linux/version.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 18)
#include <linux/config.h>
#else
#include <linux/autoconf.h>
#endif
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/platform_device.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <asm/io.h>
#include <wave400_defs.h>
#include <wave400_cnfg.h>
#include <wave400_chadr.h>
#include <wave400_chipreg.h>

/*
see drivers/mtd/mtdchar.
*/
#define WAVE400_VLSI_BUSY_BIT_BUG

#define REG_SPI_MODE				 0x130
/*flash mux to host*/
#define REG_UART_CONTROL_MUX_MPU_BIT 0x400


#define WAVE400_FLASH_ENV_SECT 4 
#define CFG_MAX_FLASH_SECT	(128)	/* max number of sectors on one chip */

#define CFG_GCLK_ADDR MT_LOCAL_MIPS_BASE_ADDRESS

#define FLAG_PROTECT_CLEAR 0
#define FLAG_PROTECT_SET   1

struct spi_flash_info
{
	u32 sector_count;
	u32 sector_size;
	u32 pages;
	u32 size;
   u32 flash_id;
	u32	start[CFG_MAX_FLASH_SECT];	  /* physical sector start addresses */
	u8 	protect[CFG_MAX_FLASH_SECT];	/* sector protection status	*/
	struct mtd_info mtd;
#ifdef CONFIG_MTD_PARTITIONS
	struct mtd_partition *parsed_parts; /* parsed partitions */
#endif
};

#define CFG_MAX_FLASH_BANKS 1
static struct spi_flash_info spi_flash_bank[CFG_MAX_FLASH_BANKS];

#define MTD_UBOOT_OFFSET	0x0
#define MTD_UBOOT_SIZE		0x10000 /*FLASH_SECT_SIZE*/
#define MTD_ROOTFS_OFFSET	MTD_UBOOT_OFFSET+MTD_UBOOT_SIZE
#define MTD_ROOTFS_SIZE		0x7c0000 
#define MTD_SYSPARAM_OFFSET	MTD_ROOTFS_OFFSET+MTD_ROOTFS_SIZE
#define MTD_SYSPARAM_SIZE	0x10000
#define MTD_ENVPARAMS_OFFSET	MTD_SYSPARAM_OFFSET+MTD_SYSPARAM_SIZE
#define MTD_ENVPARAMS_SIZE	0x10000
#define MTD_CALIBRATION_OFFSET	MTD_ENVPARAMS_OFFSET+MTD_ENVPARAMS_SIZE
#define MTD_CALIBRATION_SIZE	0x10000

static char module_name[] = "ifx_sflash";
#ifdef CONFIG_MTD_PARTITIONS
#define	mtd_has_partitions()	(1)
/*
 * Partition definition structure:
 *
 * An array of struct partition is passed along with a MTD object to
 * add_mtd_partitions() to create them.
 *
  * Note - changing this table, update INDEX_PART_MAX define below
*/
static struct mtd_partition fixed_parts[] = {
	{
		.name =		"uboot",
		.offset =	MTD_UBOOT_OFFSET,
		.size =		MTD_UBOOT_SIZE,
	},
	{
		.name =		"rootfs",
		.offset =	MTD_ROOTFS_OFFSET,
		.size =		MTD_ROOTFS_SIZE,
	},
	{
		.name =		"sysconfig",
		.offset =	MTD_SYSPARAM_OFFSET,
		.size =		MTD_SYSPARAM_SIZE,
	},
	{
		.name =		"ubootconfig",
		.offset =	MTD_ENVPARAMS_OFFSET,
		.size =		MTD_ENVPARAMS_SIZE,
	},
	{
		.name =		"calibration",
		.offset =	MTD_CALIBRATION_OFFSET,
		.size =		MTD_CALIBRATION_SIZE,
	}
};
#define INDEX_PART_MAX 5 

#else
#define	mtd_has_partitions()	(0)
#endif

#define FLASH_UNKNOWN	   0x0
#define FLASH_MAN_S25FL064A 0x00000001
#define FLASH_MX_S25FL064A	0x000000C2
#define FLASH_SECT_SIZE	 0x00010000
#define FLASH_INTRFC_ADDR   MT_LOCAL_MIPS_BASE_ADDRESS
/*flash commands/data*/
#define S25_CMD_WR_ENABLE   0x06
#define S25_WR_DUMMY		0x0
#define S25_NEW_WR_CYCLE	0x0100
#define S25_CMD_WR		  0x02
#define S25_CMD_RD		  0x03
#define S25_BASE_ADDR_0	 0x0
#define S25_BASE_ADDR_1	 0x0
#define S25_BASE_ADDR_2	 0x0
#define S25_CMD_WR_DISABLE  0x04
#define S25_RD_CHIP_ID	  0x9F
#define S25_CMD_RDSR		0x05
#define S25_CMD_BULK_ERASE  0xC7
#define S25_CMD_SECT_ERASE  0xD8

#define WAVE400_FLASH_WAIT \
for (k=0; k<50; k++) {;}

#define WAVE400_FLASH_WAIT_LONG \
for (k=0; k<50000; k++) {;}

#define WRITE_FLASH(x) \
	WAVE400_SPI_WRITE = x; \
	spiflash_wait_ready();

#define WAVE400_SPI_WR_ADDR   (MT_LOCAL_MIPS_BASE_ADDRESS+0x120)
#define WAVE400_SPI_READ_ADDR (MT_LOCAL_MIPS_BASE_ADDRESS+0x124)

unsigned char addr_inc = 0;
unsigned char ManufacturerId;
unsigned char MemoryType;
unsigned char MemoryCapacity;
//unsigned int* cnfGclkAddr = CFG_GCLK_ADDR;		  //use read/write or memcpy instead
#define WAVE400_CNF_GCLK			   (*((u32 volatile *)CFG_GCLK_ADDR))
//unsigned int* wave400SpiReadAddr = WAVE400_SPI_READ_ADDR; //use read/write or memcpy instead
#define WAVE400_SPI_READ			   (*((u32 volatile *)WAVE400_SPI_READ_ADDR))
//unsigned int* wave400SpiWriteAddr = WAVE400_SPI_WR_ADDR;  //use read/write or memcpy instead
#define WAVE400_SPI_WRITE			   (*((u32 volatile *)WAVE400_SPI_WR_ADDR))

#define PRINTK(x, ...)

/*-----------------------------------------------------------------------
 * Functions
 */
int spiflash_status_ready(struct spi_flash_info *info);
static u32 spiflash_probe(u32 addr, struct spi_flash_info *info);
static void spiflash_reset(struct spi_flash_info *info);
static void spiflash_get_offsets(u32 base, struct spi_flash_info *info);
//static struct spi_flash_info *spiflash_get_info(u32 base);


/* This is a trivial atoi implementation since we don't have one available */
int atoi(char *string)
{
	int length;
	int retval = 0;
	int i;
	int sign = 1;

	length = strlen(string);
	for (i = 0; i < length; i++) {
		if (0 == i && string[0] == '-') {
			sign = -1;
			continue;
		}
		if (string[i] > '9' || string[i] < '0') {
			break;
		}
		retval *= 10;
		retval += string[i] - '0';
	}
	retval *= sign;
	return retval;
}

void spiflash_wait_ready(void)
{
	int k, i=0;
#ifdef WAVE400_VLSI_BUSY_BIT_BUG	
	for (i=0;i<1000;i++) ;
	i=0;
#endif	
	while (((WAVE400_SPI_READ) & 0x0100) == 0x0) {
		WAVE400_FLASH_WAIT;
		if (i > 255) {
			printk("timeout in spiflash_wait_ready\n");
			return;
		}
		i++;
	}
}

unsigned long spiflash_size (void)
{
	return spi_flash_bank[0].size;
}

/*
 * get_sect_index
 *
 * return index to sector that match the offset.
 */
int get_sect_index(struct spi_flash_info* info, u32 offset)
{
	int i;

	for (i = 0; i < info->sector_count-1 ; i++) {
		if (offset == fixed_parts[i].offset || offset < fixed_parts[i+1].offset)
			return i;
	}
	/*offset not alligned to partition, located in last partition*/
	return info->sector_count-1;
}

/*
 * get_part_index
 *
 * return index in mtd_partition table that match the offset.
 */
int get_part_index(struct spi_flash_info* info, u32 offset)
{
	int i;

	if (offset > info->size) {
		printk("error - illegal address \n");
		return -1;
	}
	for (i = 0; i < INDEX_PART_MAX; i++) {
		if (offset == fixed_parts[i].offset || offset < fixed_parts[i+1].offset)
			return i;
	}
	/*offset not alligned to partition, located in last partition*/
	return INDEX_PART_MAX;
}

/*
 * spiflash_is_protect
 *
 * return 1 if it is protected area.
 * return error if cross more than one partition.
 */
int spiflash_is_protect (struct spi_flash_info* info, u32 to, u32 len)
{
	u32 start, end, i;
	u32 offset = to+len-1;
	
	PRINTK("spiflash_is_protect to = 0x%08x, len = 0x%08x, offset = 0x%08x \n",to,len,offset);
	if (get_part_index(info, to) != get_part_index(info, offset)) { 
		printk("error, write more than one partition \n");
		return -1;
	}
	start = get_sect_index(info, to);
	end = get_sect_index(info, offset);
	PRINTK("start = %d, end = %d \n",start,end);
	for (i=start; i<end; ++i) {
		PRINTK("info->protect[%d] = %d \n",i,info->protect[i]);
		if (info->protect[i] == 1)
			return 1;
	}
	return 0;
}

/*
 * get_to
 *
 * return address of the end region.
 */
u32 get_to(u32 from, struct spi_flash_info* info)
{
	int i;
	for (i = 0; i <= INDEX_PART_MAX; i++) {
		//TODO - error if from is not aligned to fixed_parts[i].offset (not error in called from spiflash_protect)!! 
		if (from == fixed_parts[i].offset) 
			break;
	}
	if (i > INDEX_PART_MAX)
		return info->sector_count-1; /*may be same as from, handled by one_sect param*/
	else
		return fixed_parts[i+1].offset;
}

/*
 * spiflash_protect()
 * Set protection status for monitor sectors
 *
 * The monitor is always located in the _first_ Flash bank.
 * If necessary you have to map the second bank at lower addresses.
 */
void spiflash_protect (int flag, u32 from, u32 to, struct spi_flash_info* info)
{
	u32 fl_end = info->start[0] + info->size - 1;	/* flash end address */
	int i,one_sect = 0/*in case of one sect, do it*/;

	/* Do nothing if input data is bad. */
	if (info->sector_count == 0 || info->size == 0 || to < from) {
		printk("error validation in spiflash_protect\n");
		return;
	}

	PRINTK("spiflash_protect %s: from 0x%08x to 0x%08x\n",
		(flag & FLAG_PROTECT_SET) ? "ON" :"OFF", from, to);

	/* There is nothing to do if we have no data about the flash
	 * or the protect range and flash range don't overlap.
	 */
	if (info->flash_id == FLASH_UNKNOWN ||
		to < info->start[0] || from > fl_end) {
		printk("error flash_id in spiflash_protect\n");
		return;
	}

	/*in case of one partition, we have to calc the end of it. Not to disable/enable only one sector 
	*/
	if (from == to) {
		PRINTK("before: to = 0x%08x\n",to);
		to = get_to(from, info);
		PRINTK("after: to = 0x%08x\n",to);
	}

	for (i=0; i<info->sector_count; ++i) { //include from, exclude to
		PRINTK("spiflash_protect: info->start[%d]=0x%08x\n",i,info->start[i]);
		if (from == to && from == info->start[i])
			one_sect = 1;
		if ((info->start[i]>=from && info->start[i]<to) || one_sect) {
			if (flag & FLAG_PROTECT_SET) {
				info->protect[i] = 1;
				PRINTK("protect on in i=%d, info->start[i]=0x%08x\n",i,info->start[i]);
			}
			else/* if (flag & FLAG_PROTECT_CLEAR)*/ {
				info->protect[i] = 0;
				PRINTK("protect off in i=%d, info->start[i]=0x%08x\n",i,info->start[i]);
			}
			if (one_sect)
				break;
		}
	}
}

/*
 * set_spi_mode_to_sw()
 * MUX: enable HW or SW access to flash.
 *
 * This function sets mux to SW control.
 */
void set_spi_mode_to_sw(void)
{
	unsigned long val = MT_RdReg(MT_LOCAL_MIPS_BASE_ADDRESS,REG_SPI_MODE); 
	PRINTK("REG_SPI_MODE = 0x%08x\n",val);
	MT_WrReg(MT_LOCAL_MIPS_BASE_ADDRESS,REG_SPI_MODE, (val|REG_UART_CONTROL_MUX_MPU_BIT)); 
}


int spiflash_driver_init (struct spi_flash_info* flash_info)
{
	unsigned long size = 0;

	/* Init: no FLASHes known */
	u32 flashbase = 0;
	PRINTK("in spiflash_driver_init\n");

	memset(flash_info, 0, sizeof(struct spi_flash_info));
	/*enable gated clock to SPI master: 0x20 -> CFG_GCLK_ADDR*/
	WAVE400_CNF_GCLK= 0xFF;

	set_spi_mode_to_sw();

	if (spiflash_probe(flashbase, flash_info) != 0) {
		printk("error in spiflash_probe !!!\n");
		return -ENODEV;
	}

	if (flash_info->flash_id == FLASH_UNKNOWN) {
		printk("## Unknown FLASH - Size = 0x%08x\n",
		flash_info->size);
	}

	size = flash_info->size;
	/* monitor protection ON by default */
	spiflash_protect(/*FLAG_PROTECT_SET*/FLAG_PROTECT_CLEAR,
			  MTD_UBOOT_OFFSET,
			  MTD_ROOTFS_OFFSET,
			  flash_info);

	return 0;
}

/*-----------------------------------------------------------------------
 */
static void spiflash_reset(struct spi_flash_info *info)
{
	//int k;
	/* Put FLASH back in read mode */
	WRITE_FLASH(S25_CMD_WR_DISABLE);
	WRITE_FLASH(S25_NEW_WR_CYCLE);
}

/*-----------------------------------------------------------------------
 */
static void spiflash_get_offsets (u32 base, struct spi_flash_info *info)
{
	int i;

	if (info->flash_id == FLASH_UNKNOWN) {
		return;
	}

	PRINTK("set offset, base = 0x%08x\n",base);
	switch (info->flash_id) {
	case FLASH_MAN_S25FL064A:
	case FLASH_MX_S25FL064A:
		for (i = 0; i < info->sector_count; i++) {
			info->start[i] = base + i * FLASH_SECT_SIZE;
		}
		return;

	default:
		printk("Don't know sector offsets for flash type 0x%08x\n",
		info->flash_id);
		return;
	}
}

/*-----------------------------------------------------------------------
static struct spi_flash_info *spiflash_get_info(u32 base)
{
	int i;
	struct spi_flash_info * info;

	for (i = 0; i < CFG_MAX_FLASH_BANKS; i ++) {
		info = & spi_flash_bank[i];
		if (info->start[0] <= base && base < info->start[0] + info->size)
			break;
	}

	return i == CFG_MAX_FLASH_BANKS ? 0 : info;
}
*/

/*
 * The following code cannot be run from FLASH!
 */

//#define WAVE400_DEBUG_DO_NOT_ERASE
int spiflash_serial_sect_erase(u32 addr, struct spi_flash_info *info)
{
	//int k;
	
	PRINTK("!! sect erase, please wait, addr = 0x%08x\n",addr);
#ifndef WAVE400_DEBUG_DO_NOT_ERASE
	switch (info->flash_id) {
	case (u32)FLASH_MAN_S25FL064A:
	case (u32)FLASH_MX_S25FL064A:
		WRITE_FLASH(S25_NEW_WR_CYCLE);
		WRITE_FLASH(S25_CMD_WR_ENABLE);
		WRITE_FLASH(S25_NEW_WR_CYCLE);
		WRITE_FLASH(S25_CMD_SECT_ERASE);
		WRITE_FLASH((addr & 0x00FF0000)>>16);
		WRITE_FLASH((addr & 0x0000FF00)>>8);
		WRITE_FLASH(addr & 0x000000FF);
		WRITE_FLASH(S25_NEW_WR_CYCLE);
	break;
	default:
		printk("unknown flash\n");
	break;
	}

	if(spiflash_status_ready(info) != 0)
		return -EBUSY;

	WRITE_FLASH(S25_CMD_WR_DISABLE);
	WRITE_FLASH(S25_NEW_WR_CYCLE);
#else	
	printk("test only, no erase done :-) \n");
#endif	
	return 0;
}

/************************************************************************
 * spiflash_status_ready
 *
 * perform the read from flash.
 * params:
 * src - address in flash
 * addr - ptr to memory
 * return:
 *   0 - OK
 *   !0 - error
 *
 * TODO: Add T.O and error when occur
 */
int spiflash_status_ready(struct spi_flash_info *info)
{
	int /*k, */ret = -EBUSY;
	unsigned int val;
	
//TODO - start timer
redo:
	switch (info->flash_id) {
	case (u8)FLASH_MAN_S25FL064A:
	case (u8)FLASH_MX_S25FL064A:
//TODO - test timer, return error if timeout (avoid deadlock)
		WRITE_FLASH(S25_CMD_RDSR);
		WRITE_FLASH(S25_WR_DUMMY);
		val = WAVE400_SPI_READ;
		WRITE_FLASH(S25_NEW_WR_CYCLE);
		if (((0xFF & val) & 0x01) == 0x01)
		  goto redo;
		ret = 0;
	break;
	default:
		printk("unknown flash\n");
		ret = -ENODEV;
	break;
	}
	return ret;
}

/************************************************************************
 * spiflash_probe
 *
 * perform the erase sectors in flash.
 */
u32 spiflash_probe (u32 addr, struct spi_flash_info *info)
{
	 //int k;

	printk("in spiflash_probe\n");
	WRITE_FLASH(S25_NEW_WR_CYCLE);
	/*read manufacturer*/
	WRITE_FLASH(S25_RD_CHIP_ID);
	WRITE_FLASH(S25_WR_DUMMY);
	ManufacturerId = WAVE400_SPI_READ;
	printk("ManufacturerId = %d, (u8)FLASH_MX_S25FL064A = %d\n",ManufacturerId,FLASH_MX_S25FL064A);
	WRITE_FLASH(S25_WR_DUMMY);
	MemoryType = WAVE400_SPI_READ;
	printk("MemoryType = %d\n",MemoryType);
	WRITE_FLASH(S25_WR_DUMMY);
	MemoryCapacity = WAVE400_SPI_READ;
	printk("MemoryCapacity = %d\n",MemoryCapacity);

	WRITE_FLASH(S25_NEW_WR_CYCLE);
	
	/* The manufacturer codes are only 1 byte, so just use 1 byte.
	 * This works for any bus width and any FLASH device width.
	 */
	switch (ManufacturerId) {
	case (u8)FLASH_MAN_S25FL064A:
		printk("FLASH_MAN_S25FL064A\n");
		/*Min HW protect range (SA127:SA126, 1/6 flash size), addr: 7E0000h�7FFFFFh
		*/
		info->flash_id = FLASH_MAN_S25FL064A;
		info->sector_count = 128;
		info->sector_size = FLASH_SECT_SIZE;
		info->size = 0x00800000;
		break;

	case (u8)FLASH_MX_S25FL064A:
		printk("FLASH_MX_S25FL064A\n");
		/*Min HW protect range (SA127:SA126, 1/6 flash size), addr: 7E0000h�7FFFFFh
		*/
		info->flash_id = FLASH_MX_S25FL064A;
		info->sector_count = 128;
		info->sector_size = FLASH_SECT_SIZE;
		info->size = 0x00800000;
		break;

	default:
		printk("FLASH_UNKNOWN !!!\n");
		info->flash_id = FLASH_UNKNOWN;
		info->sector_count = 0;
		info->sector_size = 0;
		info->size = 0;
		break;
	}

	spiflash_get_offsets(addr, info);

	/* Put FLASH back in read mode */
	spiflash_reset(info);

	return (0);
}

/************************************************************************
 * wave400_spiflash_erase
 *
 * called by mtd layer to erase sectors in flash.
 */
static int wave400_spiflash_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct spi_flash_info *flash_info = mtd->priv;
	u32 sect;
	u32 start_sector = 0;
	u32 end_sector = 0xffffffff;

	PRINTK("SPI spiflash_erase: addr=0x%08llx len=%lld, flash_info->sector_size = %d, \
			flash_info->sector_count = %d\n", 
			instr->addr, instr->len, flash_info->sector_size,
			flash_info->sector_count);

	/* Sanity checks */
	if (instr->addr + instr->len > mtd->size)
		return -EINVAL;

	/* Casting to uint32_t done cause we're not supporting modulo 
	 * operations for uint64_t. (Lack of __umoddi3() in MIPS 24K implementation)
	 * On ther other hand, 64 bit addr size spi flash is not
	 * our case :) */
	if (((uint32_t)instr->len % mtd->erasesize != 0) 
			|| ((uint32_t)instr->len % flash_info->sector_size != 0))
		return -EINVAL;
	if (((uint32_t)instr->addr % flash_info->sector_size) != 0)
		return -EINVAL;

	for (sect = 0; sect < flash_info->sector_count; sect++) {
		PRINTK("SPI spiflash_erase: sect=%d, ((sect * flash_info->sector_size) = %d\n"
				,sect,(sect * flash_info->sector_size));
		if ((instr->addr <= (sect * flash_info->sector_size)) && (start_sector == 0)) {
			start_sector = sect;
			PRINTK("SPI spiflash_erase: start_sector=%d, (sect * flash_info->sector_size) = 0x%08x\n"
					,start_sector,(sect * flash_info->sector_size));
		}
		if ((instr->addr + instr->len) > (sect * flash_info->sector_size) &&
				 (instr->addr + instr->len) <= ((sect+1) * flash_info->sector_size)) {
			end_sector = sect;
			PRINTK("SPI end_sector = %d,(instr->addr + instr->len) = 0x%08x\n",end_sector,(instr->addr + instr->len));
			break;
		}
	}
	if (end_sector == 0xffffffff)
		end_sector = sect;
	PRINTK("SPI start_sector = %d,end_sector = %d\n", start_sector,end_sector);
	
	for (sect = start_sector; sect <= end_sector; sect++) {
		if (flash_info->protect[sect] != 0) {
			PRINTK("SPI flash sector %d is protected !!\n", sect);
			/* protected, skip it */
			goto erase_err;
			//continue;
		}
		
		/*TODO - test for HW protection....!
		* May add size validation to enable (release HW protection) write
		* on boot related areas (avoid mistakes)
		*/

		/* erase sector */
		if(spiflash_serial_sect_erase(flash_info->start[sect], flash_info) != 0) {
			printk("SPI flash sector %d erase error !!\n", sect);
			goto erase_err;
		} else {
			PRINTK("SPI flash sector %d erase ok\n", sect);
		}
	}
	/* Inform MTD subsystem that erase is complete */
	instr->state = MTD_ERASE_DONE;
	if (instr->callback)
		instr->callback(instr);

	return 0;

erase_err:
	instr->state = MTD_ERASE_FAILED;
	if (instr->callback)
		instr->callback(instr);

	return -EIO;
}

/************************************************************************
 * spiflash_write_buff
 *
 * perform the write to flash. It writes the whole bytes in buffer while
 * maintain the flash sequence required - negate CS at end of page.
 * params:
 * src - ptr to memory
 * addr - address in flash
 * ret:
 *   0 - OK
 *   !0 - write timeout
 */
int spiflash_write_buff (struct spi_flash_info *info, u32 addr, const u_char *src, u32 cnt)
{
	int res=0;
	u32 i;
	//u8 data;

	PRINTK("spiflash_write_buff: src = 0x%p, addr = 0x%08x, cnt = %d\n",src,addr,cnt);
//for (i=0;i<10;i++)
//  printk("*src=0x%x ",*src);
//printk("\n ");
	/* write to the flash */
	switch (info->flash_id) {
		case (u32)FLASH_MAN_S25FL064A:
		case (u32)FLASH_MX_S25FL064A:
			/*write handling:============================
			*/
			addr = addr&0x00FFFFFF; /*delete ?*/
			cnt +=addr;
			WRITE_FLASH(S25_NEW_WR_CYCLE);
			do {
				//printk("addr = 0x%08x, src = 0x%p\n",addr,src);
				WRITE_FLASH(S25_CMD_WR_ENABLE);
				WRITE_FLASH(S25_NEW_WR_CYCLE);
				WRITE_FLASH(S25_CMD_WR);
				WRITE_FLASH((addr>>16)&0xFF);
				WRITE_FLASH((addr>>8)&0xFF);
				WRITE_FLASH(addr&0xFF);
				for (i = addr;	i < cnt; i++,addr++,src++) {
					//data = *src;
					//printk("i = %d, data = 0x%x. ",i,data);
					WRITE_FLASH(*src);
					//WRITE_FLASH(data);
					if ((i != 0 ) && ((i&0xFF) == 0xFF)) {
						addr+=1;
						src+=1;
						//printk("new page,i = %d, addr = 0x%08x, src = 0x%p \n",i,addr,src);
						break;
					}
				}
				/*Write "1" in order to negate CS at the end
				  of the transaction:					   */
				WRITE_FLASH(S25_NEW_WR_CYCLE);
				if(spiflash_status_ready(info) != 0)
					res = -EBUSY;
				//WAVE400_FLASH_WAIT_LONG
			} while (i < cnt);

			PRINTK("i = %d\n",i);
			PRINTK("status ready\n");
		break;
		default:
			/* unknown flash type, error! */
			printk("missing or unknown FLASH type\n");
			res = -ENODEV;
			break;
	}

	return (res);
}

/************************************************************************
 * wave400_spiflash_write
 *
 * called by mtd layer to write to flash.
 */
static int 
wave400_spiflash_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
	struct spi_flash_info *flash_info = mtd->priv;
	int res;

	PRINTK("in wave400_spiflash_write, mtd=0x%p,to=0x%x,len=0x%x,retlen=%d,buf=0x%p \n", mtd, (int)to, (int)len, (int)*retlen, buf);
	/* Sanity checks */
	if (!len) {
		printk("error - len = 0, return\n");
		return 0;
	}
	if (to + len > mtd->size) {
		printk("error - to + len > mtd->size, return\n");
		return -EINVAL;
	}
	*retlen = 0;

	/*TODO - test for SW protection....! */
	if (spiflash_is_protect(flash_info, to, len) != 0) {
		printk("error, protected area found\n");
		return -EIO;
	}

	/*TODO - test for HW protection....!
	* May add size validation to enable (release HW protection) write
	* on boot related areas (avoid mistakes)
	*/
	res=spiflash_write_buff(flash_info, to, buf, len);
	*retlen = len;
	if (res) {
		printk("spiflash_write_buff failed with %d\n", res);
		return -EIO;
	}

	return 0;
}

/************************************************************************
 * spiflash_read_buff
 *
 * perform the read from flash.
 * params:
 * src - address in flash
 * addr - ptr to memory
 * return:
 *   0 - OK
 *   !0 - error
 */
int spiflash_read_buff (struct spi_flash_info *info, u32 src, u8 *addr, u32 cnt)
{
	int i, res=0;
	
	PRINTK("in spiflash_read_buff, src = 0x%08x, addr = 0x%p, cnt = %d\n",src,addr,cnt);
	
	switch (info->flash_id) {
		case (u32)FLASH_MAN_S25FL064A:
		case (u32)FLASH_MX_S25FL064A:

			WRITE_FLASH(S25_NEW_WR_CYCLE);
			WRITE_FLASH(S25_CMD_RD);
			WRITE_FLASH((src>>16)&0xFF);
			WRITE_FLASH((src>>8)&0xFF);
			WRITE_FLASH(src&0xFF);

			for (i=0; i<cnt; i++,addr++) {
				WRITE_FLASH(S25_WR_DUMMY);
				*addr = WAVE400_SPI_READ;
			}

			WRITE_FLASH(S25_NEW_WR_CYCLE);
		break;
		default:
			/* unknown flash type, error! */
			printk("missing or unknown FLASH type\n");
			res = -ENODEV;
			break;
	}
	
	return (res);
}

/************************************************************************
 * wave400_spiflash_read
 *
 * called by mtd layer to read from flash.
 */
static int 
wave400_spiflash_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
	struct spi_flash_info *flash_info = mtd->priv;

   	/* sanity checks */
   	if (!len)
   		return (0);
   	if (from + len > mtd->size)
   		return (-EINVAL);

	spiflash_read_buff (flash_info, from, buf, len);
   	/* we always read len bytes */
   	*retlen = len;

	return 0;
}

/************************************************************************
 * wave400_spiflash_lock
 *
 * called by mtd layer to lock flash:
 *   ret = mtd->lock(mtd, info.start, info.length)

	struct erase_info_user {
		   uint32_t start;
		   uint32_t length;
	};
*/
static int wave400_spiflash_lock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	struct spi_flash_info *flash_info;

	flash_info = &spi_flash_bank[0];
	PRINTK("wave400_spiflash_lock: start = 0x%08x, len = 0x%08x, end = 0x%08x \n",
			ofs,len,(ofs+len));
	spiflash_protect(FLAG_PROTECT_SET,
			  ofs,
			  ofs+len,
			  flash_info);
	return 0;
}

/************************************************************************
 * wave400_spiflash_unlock
 *
 * called by mtd layer to unlock flash:
 *   ret = mtd->unlock(mtd, info.start, info.length)
 */
static int wave400_spiflash_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	struct spi_flash_info *flash_info;

	flash_info = &spi_flash_bank[0];
	PRINTK("cfi_atmel_unlock: start = 0x%08x, len = 0x%08x, end = 0x%08x\n",ofs,len,(ofs+len));
	spiflash_protect(FLAG_PROTECT_CLEAR,
			  ofs,
			  ofs+len,
			  flash_info);
	return 0;
}

//#define WAVE400_DEVICE_LAST
static int __devinit wave400_spiflash_init(void)
{
	struct mtd_info *mtd;
	struct spi_flash_info *flash_info;
	int retval;

   	printk("SPI: Probing for Serial flash ...\n");
	flash_info = &spi_flash_bank[0];
	/* initialize spi channel 0 */
	if (spiflash_driver_init(flash_info) != 0) {
   		printk("error in spiflash_driver_init !!!\n");
		return -ENODEV;
	}

	mtd = &flash_info->mtd;
   	PRINTK("SPI: mtd addr = 0x%p \n",mtd);
	mtd->name = module_name;
	mtd->type = MTD_DATAFLASH;
	mtd->flags = MTD_WRITEABLE;
	mtd->writesize = flash_info->sector_size;
	mtd->size = flash_info->size;
	mtd->erasesize = flash_info->sector_size;
	mtd->numeraseregions = 0; 
	mtd->erase = wave400_spiflash_erase;
	mtd->read = wave400_spiflash_read;
	mtd->write = wave400_spiflash_write;
	mtd->lock = wave400_spiflash_lock;
	mtd->unlock = wave400_spiflash_unlock;
	mtd->priv = flash_info;
	mtd->owner = THIS_MODULE;

	if (mtd_has_partitions()) {
#ifdef CONFIG_MTD_PARTITIONS
		struct mtd_partition *parts;
		int nr_parts = 0;
#ifdef CONFIG_MTD_CMDLINE_PARTS
		static const char *part_probes[] = { "cmdlinepart", NULL, };
		nr_parts = parse_mtd_partitions(mtd, part_probes, &parts, 0);

		printk("SPI: in CONFIG_MTD_CMDLINE_PARTS, nr_parts = %d \n",nr_parts);
#endif
		printk("SPI: in CONFIG_MTD_PARTITIONS \n");
		if (nr_parts > 0) {
			retval = add_mtd_partitions(mtd, parts, nr_parts);
			if (retval != 0)
				goto error;
			flash_info->parsed_parts = parts;
		} else {
			retval = add_mtd_partitions(mtd, fixed_parts, ARRAY_SIZE(fixed_parts));
			if (retval != 0)
				goto error;
			flash_info->parsed_parts = fixed_parts;
		}
#endif
	}
	else
		printk("SPI: mtd_has no partitions !! \n");

	retval = add_mtd_device(mtd) == 1 ? -ENODEV : 0;
	PRINTK("SPI: add_mtd_device return %d \n",retval);
	if (retval != 0)
		goto error;

	return 0;

error:
	return -ENODEV;
}

module_init (wave400_spiflash_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Lantiq VBG400 SPI Flash MTD driver");
