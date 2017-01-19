/******************************************************************************
**
** FILE NAME    : ifxmips_mtd_mlnand.c
** PROJECT      : UEIP
** MODULES      : NAND Flash
**
** DATE         : 06 October 2011
** AUTHOR       : Mohammad Firdaus B Alias Thani
** DESCRIPTION  : High Speed NAND Flash MTD Driver
** COPYRIGHT    :       Copyright (c) 2006
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author                            $Version   $Comment
** 06  Oct 2011 Mohammad Firdaus B Alias Thani    1.0         First release
*******************************************************************************/

/*!
  \defgroup IFX_NAND_DRV UEIP Project - nand flash driver
  \brief UEIP Project - Nand flash driver, supports LANTIQ CPE platforms(Danube/ASE/ARx/VRx).
 */

/*!
  \defgroup IFX_NAND_DRV_API External APIs
  \ingroup IFX_NAND_DRV
  \brief External APIs definitions for other modules.
 */

/*!
  \defgroup IFX_NAND_DRV_STRUCTURE Driver Structures
  \ingroup IFX_NAND_DRV
  \brief Definitions/Structures of nand module.
 */

/*!
  \file ifxmips_mtd_nand.h
  \ingroup IFX_NAND_DRV
  \brief Header file for LANTIQ nand driver
 */

/*!
  \file ifxmips_mtd_mlcnand.c
  \ingroup IFX_NAND_DRV
  \brief nand driver main source file.
*/


#include <linux/version.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/smp_lock.h>
#include <asm/io.h>
#include <asm/system.h>

/* Project header */
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/ifx_gpio.h>
#include <asm/ifx/common_routines.h>
#include <asm/ifx/irq.h>
#include <ifx_dma_core.h>
#include <asm/ifx/ifx_pmu.h>

#include "ifxmips_mtd_nand.h"

#define MLC_NAND_DMA_BURST_LEN    DMA_BURSTL_8DW
#define LQ_MTD_MLC_NAND_BANK_NAME "ifx_nand"
#define MLCNAND_EVENT		  0x98
#define MLCNAND_WRITE		  0x80
#define READ_NAND               *((volatile u8*)(NAND_BASE_ADDRESS | (NAND_READ_DATA))); while((IFX_REG_R32(IFX_EBU_NAND_WAIT) & IFX_EBU_NAND_WAIT_WR_C) == 0);
//#define READ_NAND               *((volatile u8*)(NAND_BASE_ADDRESS | (NAND_READ_DATA)));
#if defined (CONFIG_NAND_CS0)
#define NAND_CS0_TRIGGER	  0x00000008
#elif defined (CONFIG_NAND_CS1)
#define NAND_CS1_TRIGGER	  0x00000010
#else
#error "Unknown chip select number!"
#endif

#if defined(CONFIG_MTD_PARTITIONS)
#if defined(CONFIG_MTD_CMDLINE_PARTS)
static const char *part_probes[] = { "cmdlinepart", NULL };
#endif /* CONFIG_MTD_PARTITIONS */
#endif /*  CONFIG_MTD_CMDLINE_PARTS */

#if defined (CONFIG_MTD_MLC_EMBEDDED_MODE)
#define MLC_ECC_LOCATION 	1
#elif defined (CONFIG_MTD_MLC_SPARE_MODE)
#define MLC_ECC_LOCATION	0
#endif

#if defined (CONFIG_MTD_MLC_SAFE_MODE)
#define MLC_ECC_STRENGTH	0
#elif defined(CONFIG_MTD_MLC_ADVANCE_MODE)
#define MLC_ECC_STRENGTH	1
#endif

#if defined (CONFIG_MTD_MLC_ECC_4B_MODE)
#define MLC_ECC_MODE		1
#elif defined(CONFIG_MTD_MLC_ECC_3B_MODE)
#define MLC_ECC_MODE		0
#endif

static struct dma_device_info *dma_device = NULL;
extern const struct mtd_partition g_ifx_mtd_nand_partitions[];
extern const int g_ifx_mtd_nand_partion_num;

static inline void NAND_DISABLE_CE(struct nand_chip *nand) { IFX_REG_W32_MASK(NAND_CON_CE,0,IFX_EBU_NAND_CON); }
static inline void NAND_ENABLE_CE(struct nand_chip *nand)  { IFX_REG_W32_MASK(0,NAND_CON_CE,IFX_EBU_NAND_CON); }
void update_mlc_nand_addr_lp(struct mtd_info *mtd, int page_addr, int col, int cmd);
void update_mlc_nand_addr_sp(struct mtd_info *mtd, int page, int cmd);
u32 latchcmd = 0;

struct mlc_nand_priv {
    u32 ndac_ctl_1;
    u32 ndac_ctl_2;
    u32 current_page;
 
    int type;  		/* ONFI / SAMSUNG */
    int plane_mode;	/* Single / 2-plane mode */
    int pcount;		/* no. of cont. rd/wr operations */
    int nand_mode;     
    int ecc_mode;       /* 3Byte / 4 Byte mode*/
    int ecc_location;	/* Embedded / Spare area mode */
    int ecc_strength;   /* Safe / Advance mode */
    int pib;		/* Number pages in a block*/
    int pagesize;       /* NAND page size */
 
    int chip_id; 
    int addr_cycle;    
    int ecc_status;     
    int write_partial;   /* write partial oob flag */

    int dma_ecc_mode;    /* toggle between ECC and transparent mode */

    int partial_page_attr;
    int oob_data_status;  /* determines whether oob has data or not */

    u8 chip_info[8];
    u8 multiplane_wr_cmd;
    u8 multiplane_rd_cmd;

    struct mtd_info *mtd_priv;

    wait_queue_head_t mlc_nand_wait;
    volatile long wait_flag;

};

u8 *tmp_wr_buf;
u8 *tmp_rd_buf; 
struct mlc_nand_priv *mlc_nand_dev;
static struct mtd_info *mlc_nand_mtd = NULL;

static u8 g_mul(u8 arg1, u8 arg2) {
    u8 s = 0;
    u8 g_num2alpha[256] = {0,0,1,99,2,198,100,106,3,205,199,188,101,126,107,42,4,141,206,
             78,200,212,189,225,102,221,127,49,108,32,43,243,5,87,142,232,207,
             172,79,131,201,217,213,65,190,148,226,180,103,39,222,240,128,177,
             50,53,109,69,33,18,44,13,244,56,6,155,88,26,143,121,233,112,208,
             194,173,168,80,117,132,72,202,252,218,138,214,84,66,36,191,152,149,
             249,227,94,181,21,104,97,40,186,223,76,241,47,129,230,178,63,51,
             238,54,16,110,24,70,166,34,136,19,247,45,184,14,61,245,164,57,59,7,
             158,156,157,89,159,27,8,144,9,122,28,234,160,113,90,209,29,195,123,
             174,10,169,145,81,91,118,114,133,161,73,235,203,124,253,196,219,30,
             139,210,215,146,85,170,67,11,37,175,192,115,153,119,150,92,250,82,
             228,236,95,74,182,162,22,134,105,197,98,254,41,125,187,204,224,211,
             77,140,242,31,48,220,130,171,231,86,179,147,64,216,52,176,239,38,
             55,12,17,68,111,120,25,154,71,116,167,193,35,83,137,251,20,93,248,
             151,46,75,185,96,15,237,62,229,246,135,165,23,58,163,60,183};


    u8 g_alpha2num[256] = {1,2,4,8,16,32,64,128,135,137,149,173,221,61,122,244,111,222,59,118,
             236,95,190,251,113,226,67,134,139,145,165,205,29,58,116,232,87,174,
             219,49,98,196,15,30,60,120,240,103,206,27,54,108,216,55,110,220,63,
             126,252,127,254,123,246,107,214,43,86,172,223,57,114,228,79,158,187,
             241,101,202,19,38,76,152,183,233,85,170,211,33,66,132,143,153,181,
             237,93,186,243,97,194,3,6,12,24,48,96,192,7,14,28,56,112,224,71,142,
             155,177,229,77,154,179,225,69,138,147,161,197,13,26,52,104,208,39,
             78,156,191,249,117,234,83,166,203,17,34,68,136,151,169,213,45,90,
             180,239,89,178,227,65,130,131,129,133,141,157,189,253,125,250,115,
             230,75,150,171,209,37,74,148,175,217,53,106,212,47,94,188,255,121,
             242,99,198,11,22,44,88,176,231,73,146,163,193,5,10,20,40,80,160,
             199,9,18,36,72,144,167,201,21,42,84,168,215,41,82,164,207,25,50,
             100,200,23,46,92,184,247,105,210,35,70,140,159,185,245,109,218,51,
             102,204,31,62,124,248,119,238,91,182,235,81,162,195,1};


    if ((arg1 == 0) | (arg2 == 0)) 
        return 0;

    s = (s + g_num2alpha[arg1]) % 255;
    s = (s + g_num2alpha[arg2]) % 255;
    return g_alpha2num[s];
}

u8 g_add(u8 arg3, u8 arg4) {
       u8 s = 0;
       s = s ^ arg3;
       s = s ^ arg4;
       return s;    //add in G math is XOR
};

void reed_solomn_128bytes_ecc(const u8 *data_bytes_partial, u8 *ecc_data) {
    u8 g[4];
    u8 temp[3] = {0, 0, 0};
    u8 s[4] = {0, 0, 0, 0};
    u8 degree;
    u8 bytes;
    u8 y;
    u8 i;
    struct mlc_nand_priv *mlc = mlc_nand_dev;

    if (mlc->ecc_mode == 0) { // 3 bytes ECC
         g[3] = 14;
         g[2] = 56;
         g[1] = 64;
         g[0] = 0 ;
         degree = 3;
     }
     else { // 4 bytes ECC
         g[3] = 205;
         g[2] =  63;
         g[1] =  92;
         g[0] =  32;
         degree = 4;
     }

     if (mlc->ecc_location == 0)  //Spare mode
         bytes = 128; 
     else   // Embedded mode
         bytes = 124; 

    for (i = 0; i < bytes; i++) {
         y = g_add(s[3], data_bytes_partial[i]);
         temp[0] = g_mul(y, g[3]);
         temp[1] = g_mul(y, g[2]);
         temp[2] = g_mul(y, g[1]);
         s[3] = g_add(s[2], temp[0]);
         s[2] = g_add(s[1], temp[1]);
         s[1] = g_add(s[0], temp[2]);
         s[0] = g_mul(y, g[0]);
    };

    if (mlc->ecc_mode == 0)  // 3bytes ECC mode
        s[0] = 255;

    for (i = 0; i < degree; i++) {
	ecc_data[i] = s[(degree - 1) - i];
    }
}

void NAND_WAIT_READY(struct nand_chip *nand)
{
    while(!NAND_READY){}
}

static int lq_mlcnand_ready(struct mtd_info *mtd)
{
      struct nand_chip *nand = mtd->priv;

      NAND_WAIT_READY(nand);
      return 1;
}

static void lq_init_read_buf(struct mtd_info *mtd, u8 *buf, int len)
{
    int i;
  
    for (i = 0; i < len; i++) {
        asm("sync");    
        buf[i]=READ_NAND;
        asm("sync");    
    }
    //while((IFX_REG_R32(IFX_EBU_NAND_WAIT) & IFX_EBU_NAND_WAIT_WR_C) == 0) {};

#if 0
    printk("output tmp read buf: ");
    for (i = 0; i < len; i++) {
        if (!(i % 4))
            printk("\nbyte[%d]: ", i);
        printk("%02x", buf[i]);
    }
    printk("\n");
#endif
}

static u_char lq_mlcnand_read_byte(struct mtd_info *mtd)
{
    u_char ret;

    asm("sync");
    ret=READ_NAND;
    asm("sync");

    return ret;
}

void lq_mlcnand_select_chip(struct mtd_info *mtd, int chip)
{
    //struct nand_chip *nand = mtd->priv;

    switch (chip) {
        case -1:
             //NAND_DISABLE_CE(nand);
	     NAND_CE_CLEAR;
             IFX_REG_W32_MASK(IFX_EBU_NAND_CON_NANDM, 0, IFX_EBU_NAND_CON);
             break;
        case 0:
             IFX_REG_W32_MASK(0, IFX_EBU_NAND_CON_NANDM, IFX_EBU_NAND_CON);
	         NAND_CE_SET;
             //NAND_ENABLE_CE(nand);
			 /* Similar to SLC NAND driver, the reset is removed to prevent 
			  * performance reduction and circumvent issues with some MICRON 
			  * NAND flashes */
             //NAND_WRITE(NAND_WRITE_CMD, NAND_WRITE_CMD_RESET); // Reset nand chip
             break;

        default:
             printk(KERN_ERR "Unknown chip select option\n");
        }
}

void lq_mlcnand_cmd_ctrl (struct mtd_info *mtd, int data, unsigned int ctrl)
{
    struct nand_chip *this = mtd->priv;

    if (ctrl & NAND_CTRL_CHANGE){
        if (ctrl & NAND_CLE) {
	    	NAND_ALE_CLEAR;
	    	NAND_CLE_SET;
	    	latchcmd=NAND_WRITE_CMD;
        }
        else if(ctrl & NAND_ALE) {
            NAND_CLE_CLEAR
            NAND_ALE_SET; 
            latchcmd=NAND_WRITE_ADDR;
		}
		else {
            NAND_ALE_CLEAR;
			NAND_CLE_CLEAR;
			latchcmd = NAND_WRITE_DATA;
		}
    }

    if (data != NAND_CMD_NONE){
        *(volatile u8*)((u32)this->IO_ADDR_W | latchcmd) = data;
        while((IFX_REG_R32(IFX_EBU_NAND_WAIT) & IFX_EBU_NAND_WAIT_WR_C) == 0);
    }
    else {
	NAND_ALE_CLEAR;
	NAND_CLE_CLEAR;
    }
    return;
}

void lq_mlcnand_hwctl(struct mtd_info *mtd, int mode)
{

    if (mode & NAND_CMD_READID)
 	NAND_CE_CLEAR
    else if (mode & NAND_CLE)
	NAND_CE_SET;

    return; 
}


static int lq_nand_write_oob(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
    u8 *buf = chip->oob_poi;    
    int i, length = mtd->oobsize;
    int offs = 0;
    struct nand_oobfree *free = chip->ecc.layout->oobfree;
    struct mlc_nand_priv *mlc = mlc_nand_dev;

    /* for partial oob writes [location after hw based ecc] */
    if (mlc->write_partial) {
        offs = free->offset;
        length = free->length;
        chip->cmdfunc(mtd, NAND_CMD_SEQIN, mtd->writesize + offs, page); 
    }
    else
       chip->cmdfunc(mtd, NAND_CMD_SEQIN, mtd->writesize, page);

    for (i = 0; i < length; i++) {
	asm("sync");
        NAND_WRITE(NAND_WRITE_DATA, buf[i + offs]);
	asm("sync");
    }
    
    NAND_CE_SET;
    NAND_READY_CLEAR;

    chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
    chip->dev_ready(mtd);

    NAND_CE_CLEAR;

#if 0
    if (debug == 1) {
    	printk("output: ");
    	for (i = 0; i < mtd->oobsize; i++) {
       	    if (!(i % 16))
                printk("\nbyte[%d]: ", i);
            printk("%02x", buf[i]);
    	}
    	printk("\n");
    }
#endif

    return 0;
}

static int lq_nand_read_oob(struct mtd_info *mtd, struct nand_chip *chip, int page,
                                    int sndcmd)
{
    int i;
    u8 *buf  = chip->oob_poi;

    if (sndcmd) {
        chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
        chip->dev_ready(mtd);
		sndcmd = 0;
    }

    for (i = 0; i < mtd->oobsize; i++) {
        asm("sync");
        buf[i] = READ_NAND;
        asm("sync");
    }
    //while((IFX_REG_R32(IFX_EBU_NAND_WAIT) & IFX_EBU_NAND_WAIT_WR_C) == 0) {};

#if 0
    printk("output read_oob, page: %d", page);
    for (i = 0; i < mtd->oobsize; i++) {
        if (!(i % 8))
            printk("\nbyte[%d]: ", i);
        printk("%02x", buf[i]);
    }
    printk("\n");
#endif
    return sndcmd;
}

/* [Work around] - Check whether the page is empty because when fs erase nand eraseblock, 
 * there will be no HW ECC data in OOB area causing subsequent H/W reads to fail.
 * 
 * /return 1 if not empty and 0 if page is empty 
 */

static int check_empty_page(const u8 *buf, int len)
{
    int i;

    for (i = 0; i < len; i++) {
	if (buf[i] != 0xff) {
	    return 1;
	}
    }
    return 0;
}


void lq_nand_write_page_raw(struct mtd_info *mtd, struct nand_chip *chip, 
					const u8 *buf)
{
    int writesize = mtd->writesize;
    struct mlc_nand_priv *mlc = mlc_nand_dev;

    chip->write_buf(mtd, buf, writesize);
    chip->ecc.write_oob(mtd, chip, mlc->current_page);
    return;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,20))
static int lq_nand_read_page_raw(struct mtd_info *mtd, struct nand_chip *chip, u8 *buf, 
				 int page)
{
    int sndcmd = 1;
    int writesize = mtd->writesize;
    struct mlc_nand_priv *mlc = mlc_nand_dev;
    
    /* read bypassing ecc hardware block */
    mlc->dma_ecc_mode = 1;
    asm("sync");
    chip->read_buf(mtd, tmp_rd_buf, writesize);
#else
static int lq_nand_read_page_raw(struct mtd_info *mtd, struct nand_chip *chip, u8 *buf)
{
    int sndcmd = 1;
    struct mlc_nand_priv *mlc = mlc_nand_dev;
    int page = mlc->current_page;
    int writesize = mtd->writesize;

   /* read bypassing ecc hardware block */
    mlc->dma_ecc_mode = 3;
    asm("sync");
    chip->read_buf(mtd, tmp_rd_buf, writesize);
#endif

    memcpy(buf, tmp_rd_buf, writesize);
    chip->ecc.read_oob(mtd, chip, page, sndcmd);
    mlc->dma_ecc_mode = 1;

    return 0;
}


#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,20))
static int lq_nand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
                                        u8 *buf, int page)
#else
static int lq_nand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
                                        u8 *buf)
#endif
{
    int ret, writesize = mtd->writesize;
    struct mlc_nand_priv *mlc = mlc_nand_dev;
    //int i; 

    NAND_CE_SET;
    asm("sync");

    memset (tmp_rd_buf, 0, writesize);
    chip->read_buf(mtd, tmp_rd_buf, writesize);
    memcpy(buf, tmp_rd_buf, writesize);
    chip->ecc.read_oob(mtd, chip, mlc->current_page, 1);

    /* if intr has occured, let mtd layer handle the badblock */
    if ((mlc->ecc_status) && (mlc->dma_ecc_mode == 1) ) {
        ret = check_empty_page((const u8 *) buf, writesize);
	if (!ret) {
	    mlc->ecc_status = 0;
            return 0;
	} 
#if 0
	for (i = 0; i < mtd->writesize; i++) {
	    if (!(i%32))
		printk("\n pos %d: ", i);
	    printk("%02x ", buf[i]);
	}
	printk("\n");
#endif
	printk(KERN_WARNING "ECC calculation failed @ page: %d\n", mlc->current_page);
        mtd->ecc_stats.failed++;
        mlc->ecc_status = 0;
    }   

    return 0;
}

static int lq_nand_verify_buf(struct mtd_info *mtd, const u8 *buf, int len)
{
    u8 *tmp_buf;
    int i, writesize = mtd->writesize;
    struct nand_chip *chip = mtd->priv;
    struct mlc_nand_priv *mlc = mlc_nand_dev;

    tmp_buf = kmalloc(writesize, GFP_KERNEL);
    if (!tmp_buf) {
        printk(KERN_ERR "Error allocating buffer during buf verification\n");
        return -ENOMEM;
    }

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,20))
    chip->ecc.read_page(mtd, chip, tmp_buf, mlc->current_page);
#else
    chip->ecc.read_page(mtd, chip, tmp_buf);
#endif
    
    for (i = 0; i < len; i++) {
        if (tmp_buf[i] != buf[i]) {
            kfree(tmp_buf);
            return -EFAULT;
        }
    }
    kfree(tmp_buf);
    return 0;

}

static void write_nand_via_ebu(struct mtd_info *mtd, struct nand_chip *chip, const u8 *buf,
				int len, int page)
{
    int i;
    int oobsize = mtd->oobsize;
    u8 *oobbuf = chip->oob_poi;

    chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00 , page);

    for (i = 0; i < len; i++) {
        asm("sync");
        NAND_WRITE(NAND_WRITE_DATA, buf[i]);
        asm("sync");
    }

    for (i = 0; i < oobsize; i++) {
        asm("sync");
        NAND_WRITE(NAND_WRITE_DATA, oobbuf[i]);
        asm("sync");
    }

    NAND_CLE_SET;
    NAND_READY_CLEAR;

    chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
    chip->dev_ready(mtd);

    NAND_CLE_CLEAR;

}

static void lq_nand_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, 
					const u8 *buf)
{
    struct mlc_nand_priv *mlc = mlc_nand_dev;
    int i, writesize = mtd->writesize;
    int pagestatus = check_empty_page(buf, writesize);
    int oobstatus = mlc->oob_data_status;
    int eccbytes = chip->ecc.bytes;
    int eccsteps = chip->ecc.steps;
    int eccsize = chip->ecc.size;
    int *eccpos = chip->ecc.layout->eccpos;
    int page_addr = mlc->current_page; 
    u8 *ecc_data = chip->buffers->ecccalc;
    //int ooblen = mtd->oobsize;
    const u8 *p = buf;

    memset(tmp_wr_buf, 0, writesize);
    memcpy(tmp_wr_buf, buf, writesize);

    /* We would only want to use the EBU method to write into the NAND  
     * flash when there are valid data in the OOB area. Otherwise, it
     * would suffice to just use the DMA method to write to the NAND 
    */
    if (oobstatus == 1) {
        //printk("writing using EBU method at page %d\n", page_addr);
        for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
            reed_solomn_128bytes_ecc(p, &ecc_data[i]);
        }
  	
        for (i = 0; i < chip->ecc.total; i++) {
            chip->oob_poi[eccpos[i]] = ecc_data[i];
        }
#if 0        
        for (i = 0; i < ooblen; i++) {
	    if (!(i % 8 )) 
		printk("\n byte[%d]:", i);
	    printk(" %02x ", chip->oob_poi[i]);
        }
#endif
        write_nand_via_ebu(mtd, chip, buf, writesize, page_addr);

		return;
    }

    /* jffs2 FS does not erase empty pages before writing into them. 
     * this is always an issue especially for padded images where 
     * the empty padded areas are written to NAND flash. This generates 
     * ECC data written to spare area. Subsequent writes to this empty area will write 
     * over these ECC data causing inaccurate ECC data stored to flash
     *
     * for NAND flash that does not support partial page, we try not to write 
     * the page even if it's all 0xff because we will not be able to guarantee
     * that the next write has no bit errors.
     */

     // blank page check need for new filesystem? *fixme when filesystem is chosen
    if (pagestatus) {
        mlc->dma_ecc_mode = 0x1;
        NAND_CE_SET;
        chip->write_buf(mtd, tmp_wr_buf, writesize);
    }

    return;
}

static void lq_nand_write_buf(struct mtd_info *mtd, const u8 *buf, int len)
{
    struct dma_device_info *dma_dev = dma_device;
    struct mlc_nand_priv *mlc = mlc_nand_dev;
    int ret_len;
    int type = mlc->type;
    int page_count = mlc->pcount;
    int ecc_mode = mlc->ecc_mode;
    int plane_mode = mlc->plane_mode;
    int ecc_loc = mlc->ecc_location;
    int ecc_strength = mlc->ecc_strength;
    int block_cnt = mlc->pib;
    int page, pagesize = mlc->pagesize;
    int dma_ecc_mode = mlc->dma_ecc_mode;
    u32 reg; 
    
    NAND_ALE_SET;

    if (pagesize == 512)
	page = 0;
    else if (pagesize == 2048)
	page = 1;
    else if (pagesize == 4096)
	page = 2;
    else 
	page = 3;

    reg = SM(type, IFX_NAND_TYPE) |
	      SM(page_count, IFX_NAND_PCOUNT) | SM(plane_mode, IFX_NAND_PLANE_MODE) |
          SM(ecc_mode, IFX_NAND_ECC_MODE) | SM(ecc_loc, IFX_NAND_ECC_LOC) |
          SM(ecc_strength, IFX_NAND_ECC_STRENGTH) | 
          SM(block_cnt, IFX_NAND_PIB) | page;

    DEBUG(MTD_DEBUG_LEVEL1, "NAND write PARA0 reg: %08x\n", reg);
    DEBUG(MTD_DEBUG_LEVEL1, "NAND write length: %d, page: %08x\n", len, mlc->current_page);

    IFX_REG_W32(reg, IFX_ND_PARA0); 

    *IFX_HSMD_CTL = 0x0;
    reg = 0x0;

#if defined(CONFIG_NAND_CS0)
    reg |= (NAND_CS0_TRIGGER | (1 << 10) | dma_ecc_mode);
#elif defined(CONFIG_NAND_CS1)
    reg |= (NAND_CS1_TRIGGER | (1 << 10) | dma_ecc_mode);
#endif

    asm("sync");
    IFX_REG_W32(reg, IFX_HSMD_CTL);
    asm("sync");
	
    /* Update ndac address registers */
    IFX_REG_W32(mlc->ndac_ctl_1, IFX_NDAC_CTL_1);
    IFX_REG_W32(mlc->ndac_ctl_2, IFX_NDAC_CTL_2);

	*IFX_HSMD_CTL |= (1 << 2);

    ret_len = dma_device_write(dma_dev, (u8 *)buf, len, NULL);    
    if (ret_len != len) {
        printk("DMA write to NAND failed!\n");
        return;
    }

    wait_event_interruptible(mlc->mlc_nand_wait, test_bit(MLCNAND_EVENT,
                             &mlc->wait_flag));
    clear_bit(MLCNAND_EVENT, &mlc->wait_flag);

    /* Just to make sure that the write to NAND flash is fully done */
    while (!(IFX_REG_R32(EBU_INT_STAT) & (1 << 4)));

    IFX_REG_W32((IFX_REG_R32(EBU_INT_STAT) | (1 << 4)), EBU_INT_STAT);

    return;
    
}

static void lq_nand_read_buf(struct mtd_info *mtd, u8 *buf, int len)
{
    struct dma_device_info *dma_dev = dma_device;
    struct mlc_nand_priv *mlc = mlc_nand_dev;
    int type = mlc->type;
    int page_count = mlc->pcount;
    int ecc_mode = mlc->ecc_mode;
    int plane_mode = mlc->plane_mode;
    int ecc_loc = mlc->ecc_location;
    int ecc_strength = mlc->ecc_strength;
    int block_cnt = mlc->pib;
    int page, pagesize = mlc->pagesize;
    int chan = dma_device->current_rx_chan;
    int dma_ecc_mode = mlc->dma_ecc_mode;
    u32 reg = 0;

    //printk("Pagesize = %d\n", pagesize);
 
    NAND_ALE_SET;

    if (pagesize == 512)
        page = 0;
    else if (pagesize == 2048)
        page = 1;
    else if (pagesize == 4096)
        page = 2;
    else
        page = 3;

    DEBUG(MTD_DEBUG_LEVEL2, "type: %d, pagecnt: %d, ecc_mode: %d, plane_mode: %d\n", type, page_count, ecc_mode, plane_mode);
    DEBUG(MTD_DEBUG_LEVEL2, "ecc_loc: %d, ecc_strength: %d, blk_cnt: %d, pagesize: %d\n", ecc_loc, ecc_strength, block_cnt, page);

    reg = SM(type, IFX_NAND_TYPE) |
	      SM(page_count, IFX_NAND_PCOUNT) | SM(plane_mode, IFX_NAND_PLANE_MODE) |
          SM(ecc_mode, IFX_NAND_ECC_MODE) | SM(ecc_loc, IFX_NAND_ECC_LOC) |
          SM(ecc_strength, IFX_NAND_ECC_STRENGTH) |
          SM(block_cnt, IFX_NAND_PIB) | page;

    DEBUG(MTD_DEBUG_LEVEL1, "NAND READ PARA0 reg: %08x\n", reg);
    DEBUG(MTD_DEBUG_LEVEL1, "NAND read length: %d\n", len);
    IFX_REG_W32(reg, IFX_ND_PARA0);
    
    *IFX_HSMD_CTL = 0x1;
    //printk("reg: %08x, PARA0: %08x, dma chan: %d\n", reg, IFX_REG_R32(IFX_ND_PARA0), chan);
    dma_device_desc_setup(dma_dev, (u8 *) buf, len);

    reg = 0x0;

#if defined(CONFIG_NAND_CS0)
	reg |= (NAND_CS0_TRIGGER | dma_ecc_mode);
#elif defined(CONFIG_NAND_CS1)
    reg |= (NAND_CS1_TRIGGER | dma_ecc_mode);
#endif

    asm("sync");
    IFX_REG_W32(reg, IFX_HSMD_CTL);
    asm("sync");

    /* Update ndac address registers */
    IFX_REG_W32(mlc->ndac_ctl_1, IFX_NDAC_CTL_1);
    IFX_REG_W32(mlc->ndac_ctl_2, IFX_NDAC_CTL_2);

    dma_dev->rx_chan[chan]->open(dma_dev->rx_chan[chan]);

	*IFX_HSMD_CTL |= (1 << 2);

    //printk(KERN_ERR "HSMD_CTL: %08x\n", IFX_REG_R32(IFX_HSMD_CTL));

    wait_event_interruptible(mlc->mlc_nand_wait, test_bit(MLCNAND_EVENT,
                             &mlc->wait_flag));
    clear_bit(MLCNAND_EVENT, &mlc->wait_flag);

    IFX_REG_W32((IFX_REG_R32(EBU_INT_STAT) | (7 << 4)), EBU_INT_STAT);

    /* we have to poll the complete and OWN bit so as to allow the bits 
	 * to be cleared before the next read, otherwise DMA descriptor update
	 * will fail */
	poll_dma_ownership_bit(dma_device);

    return;
}

void update_mlc_nand_addr_sp(struct mtd_info *mtd, int page, int cmd)
{
    struct mlc_nand_priv *mlc = mlc_nand_dev;
    int writesize = mlc->pagesize;
    u32 tmp_addr, addr_0, addr_1, addr_2 = 0;
    u32 addr_3 = 0;
    u32 addr_4 = 0;
    
    tmp_addr = page * writesize;
    addr_0 = tmp_addr & 0x000000FF;
    addr_1 = (tmp_addr & 0x0001FE00) >> 9;
    addr_2 = (tmp_addr & 0x001FE000) >> 17; 
    
    mlc->current_page = page;

    if (mlc->addr_cycle == 4) {
        if (mlc->chip_id == ST_512WB2_NAND) 
            addr_3 = (tmp_addr & 0x00300000) >> 25;
        else
	    addr_3 = (tmp_addr & 0x00200000) >> 25;
    }

    mlc->ndac_ctl_1 = (addr_2 << 24) | (addr_1 << 16) | (addr_0 << 8) |
			cmd;

    if (cmd == MLCNAND_WRITE) {
        mlc->ndac_ctl_2 = (mlc->multiplane_wr_cmd << 19) | 
			  ((mlc->addr_cycle) << 16) | (addr_4 << 8) | addr_3;
    }
    else {
        mlc->ndac_ctl_2 = (mlc->multiplane_rd_cmd << 19) |
                          ((mlc->addr_cycle) << 16) | (addr_4 << 8) | addr_3;
    }

    DEBUG(MTD_DEBUG_LEVEL1, "ndac_1: %08x, ndac_2: %08x\n", mlc->ndac_ctl_1, 
				 mlc->ndac_ctl_2);

    return;
}

void update_mlc_nand_addr_lp(struct mtd_info *mtd, int page_addr, int col, int cmd)
{
    struct mlc_nand_priv *mlc = mlc_nand_dev;
    int writesize = mlc->pagesize;
    u32 tmp_addr; 
    u32 addr_0 = 0;
    u32 addr_1 = 0, addr_2 = 0;
    u32 addr_3 = 0, addr_4 = 0;

    mlc->current_page = page_addr;
    //printk("page address: %d\n", page_addr);
    //printk("multiplane: %d, address cycle: %d", mlc->multiplane_rd_cmd, (mlc->addr_cycle));

    if (writesize == 2048) {
	tmp_addr = page_addr << 12;
        addr_0 = tmp_addr & 0x000000FF;
        addr_1 = (tmp_addr & 0x00000F00) >> 8;
	addr_2 = (tmp_addr & 0x000FF000) >> 12;
	addr_3 = (tmp_addr & 0x0FF00000) >> 20;

        if (mlc->chip_id == HYNIX_MLC_FLASH) 
	    addr_4 = (tmp_addr & 0xF0000000) >> 28;
	else
	    addr_4 = (tmp_addr & 0x70000000) >> 28;
    }
    else if (writesize == 4096) {
        tmp_addr = page_addr << 13;
	addr_0 = tmp_addr & 0x000000FF;
	addr_1 = (tmp_addr & 0x00001F00) >> 8;
	addr_2 = (tmp_addr & 0x001FE000) >> 13;
	addr_3 = (tmp_addr & 0x1FE00000) >> 21;
	addr_4 = (tmp_addr & 0xE0000000) >> 29;
    }
    else if (writesize == 8192) {
	tmp_addr = page_addr << 14;
	addr_0 = tmp_addr & 0x000000FF;
	addr_1 = (tmp_addr & 0x00003F00) >> 8;        
	addr_2 = (tmp_addr & 0x003FC000) >> 14;
	addr_3 = (tmp_addr & 0x3FC00000) >> 22;
	addr_4 = (tmp_addr & 0xC0000000) >> 30;
    }
    mlc->ndac_ctl_1 = (addr_2 << 24) | (addr_1 << 16) | (addr_0 << 8) |
                        cmd;

    if (cmd == MLCNAND_WRITE) {
        mlc->ndac_ctl_2 = (mlc->multiplane_wr_cmd << 19) |
                          ((mlc->addr_cycle) << 16) | (addr_4 << 8) | addr_3;
    }
    else {
        mlc->ndac_ctl_2 = (mlc->multiplane_rd_cmd << 19) |
                          ((mlc->addr_cycle) << 16) | (addr_4 << 8) | addr_3;
    }

    DEBUG(MTD_DEBUG_LEVEL1, "cur page: %d, ndac_1: %08x, ndac_2: %08x\n", mlc->current_page,
                                 mlc->ndac_ctl_1, mlc->ndac_ctl_2);

    return;
}

void lq_mlcnand_command_sp(struct mtd_info *mtd, unsigned int command,
                         int column, int page_addr)
{
        register struct nand_chip *chip = mtd->priv;
        int ctrl = NAND_CTRL_CLE | NAND_CTRL_CHANGE;
        struct mlc_nand_priv *mlc = mlc_nand_dev;

        /* for commands with NAND_CMD_SEQIN or NAND_CMD_READ0 with
         * column bigger than pagesize are meant for oob reads which
         * we still want it to do in normal nand mode.
         */
#if 1
        /* write page command */
        if ((command == NAND_CMD_SEQIN) && (column < mtd->writesize)) {
            update_mlc_nand_addr_sp(mtd, page_addr, MLCNAND_WRITE);
            return;
        }

        /* read page command */
        if (command == NAND_CMD_READ0 && (column < mtd->writesize)) {
            update_mlc_nand_addr_sp(mtd, page_addr, mlc->multiplane_rd_cmd);
            return;
        }
#endif
        /*
         * Write out the command to the device.
         */
        if (command == NAND_CMD_SEQIN) {
                int readcmd;

                if (column >= mtd->writesize) {
                        /* OOB area */
                        column -= mtd->writesize;
                        readcmd = NAND_CMD_READOOB;
                } else if (column < 256) {
                        /* First 256 bytes --> READ0 */
                        readcmd = NAND_CMD_READ0;
                } else {
                        column -= 256;
                        readcmd = NAND_CMD_READ1;
                }
                chip->cmd_ctrl(mtd, readcmd, ctrl);
                ctrl &= ~NAND_CTRL_CHANGE;
        }
        chip->cmd_ctrl(mtd, command, ctrl);

        /*
         * Address cycle, when necessary
         */
        ctrl = NAND_CTRL_ALE | NAND_CTRL_CHANGE;

        /* Samsung requirements for the chip during OOB access */
        if ((command == NAND_CMD_READOOB) && (mlc->chip_id == SAMSUNG_512_3ADDR))
            ndelay(10);

        /* Serially input address */
        if (column != -1) {
                /* Adjust columns for 16 bit buswidth */
                if (chip->options & NAND_BUSWIDTH_16)
                        column >>= 1;
                chip->cmd_ctrl(mtd, column, ctrl);
                ctrl &= ~NAND_CTRL_CHANGE;
        }
        if (page_addr != -1) {
                chip->cmd_ctrl(mtd, page_addr, ctrl);
                ctrl &= ~NAND_CTRL_CHANGE;
                chip->cmd_ctrl(mtd, page_addr >> 8, ctrl);
                /* One more address cycle for devices > 32MiB */
                if (chip->chipsize > (32 << 20))
                        chip->cmd_ctrl(mtd, page_addr >> 16, ctrl);
        }
        chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

        /*
         * program and erase have their own busy handlers
         * status and sequential in needs no delay
         */
        switch (command) {

        case NAND_CMD_PAGEPROG:
        case NAND_CMD_ERASE1:
        case NAND_CMD_ERASE2:
        case NAND_CMD_SEQIN:
        case NAND_CMD_STATUS:
                return;

        case NAND_CMD_RESET:
                if (chip->dev_ready)
                        break;
                udelay(chip->chip_delay);
                chip->cmd_ctrl(mtd, NAND_CMD_STATUS,
                               NAND_CTRL_CLE | NAND_CTRL_CHANGE);
                chip->cmd_ctrl(mtd,
                               NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
                while (!(chip->read_byte(mtd) & NAND_STATUS_READY)) ;
                return;

                /* This applies to read commands */

        default:
                /*
                 * If we don't have access to the busy pin, we apply the given
                 * command delay
                 */
                if (!chip->dev_ready) {
                        udelay(chip->chip_delay);
                        return;
                }
        }
        /* Apply this short delay always to ensure that we do wait tWB in
         * any case on any machine. */
        ndelay(100);

        nand_wait_ready(mtd);
}

void lq_mlcnand_command_lp(struct mtd_info *mtd, unsigned int command,
                            int column, int page_addr)
{
        register struct nand_chip *chip = mtd->priv;
		u8 *oob = chip->ops.oobbuf;
        struct mlc_nand_priv *mlc = mlc_nand_dev;

        mlc->oob_data_status = 0;
        
        /* write page command */
        if ((command == NAND_CMD_SEQIN) && (column < mtd->writesize)) {
            if (!(likely(!oob))) 
				mlc->oob_data_status = 1;

            if (mlc->oob_data_status == 0) { 
                update_mlc_nand_addr_lp(mtd, page_addr, column, MLCNAND_WRITE);
                return;
            }
	    /* write using EBU method */
            else
                mlc->current_page = page_addr;
       }

        /* read page command */
        if (command == NAND_CMD_READ0 && (column < mtd->writesize))  {
             update_mlc_nand_addr_lp(mtd, page_addr, column, mlc->multiplane_rd_cmd);
             return;
        }

        /* Emulate NAND_CMD_READOOB */
        if (command == NAND_CMD_READOOB) {
                column += mtd->writesize;
                command = NAND_CMD_READ0;
        }

        NAND_CLE_SET;
        
        /* Command latch cycle */
        chip->cmd_ctrl(mtd, command & 0xff,
                       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
        asm("sync");

        NAND_CLE_CLEAR;
        //NAND_ALE_SET;  // no need to set
        //NAND_READY_CLEAR;
        if (column != -1 || page_addr != -1) {
                int ctrl = NAND_CTRL_CHANGE | NAND_ALE | NAND_NCE;
              
                DEBUG(MTD_DEBUG_LEVEL2, "Reading @ column 0x%x, page: %d\n",
                      column, page_addr);

                /* Serially input address */
                if (column != -1) {
                       /* Adjust columns for 16 bit buswidth */
                        if (chip->options & NAND_BUSWIDTH_16)
                                column >>= 1;
                        chip->cmd_ctrl(mtd, column & 0xff, ctrl);
                        ctrl &= ~NAND_CTRL_CHANGE;
                        chip->cmd_ctrl(mtd, (column >> 8) & 0xff, ctrl);
                }
                if (page_addr != -1) {
                        chip->cmd_ctrl(mtd, page_addr & 0xff, ctrl);
                        chip->cmd_ctrl(mtd, (page_addr >> 8) & 0xff,
                                       ctrl);
                        /* One more address cycle for devices > 128MiB */
                        if (chip->chipsize > (128 << 20))
                                chip->cmd_ctrl(mtd, (page_addr >> 16) & 0xff,
                                               NAND_NCE | NAND_ALE);
                }
        }

        NAND_ALE_CLEAR;
        chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE); //fir
        /*
         * program and erase have their own busy handlers
         * status, sequential in, and deplete1 need no delay
         */
        switch (command) {

        case NAND_CMD_CACHEDPROG:
        case NAND_CMD_PAGEPROG:
        case NAND_CMD_ERASE1:
        case NAND_CMD_ERASE2:
        case NAND_CMD_SEQIN:
        case NAND_CMD_RNDIN:
        case NAND_CMD_STATUS:
        case NAND_CMD_DEPLETE1:
                return;

                /*
                 * read error status commands require only a short delay
                 */
        case NAND_CMD_STATUS_ERROR:
        case NAND_CMD_STATUS_ERROR0:
        case NAND_CMD_STATUS_ERROR1:
        case NAND_CMD_STATUS_ERROR2:
        case NAND_CMD_STATUS_ERROR3:
                udelay(chip->chip_delay);
                return;

        case NAND_CMD_RESET:
                if (chip->dev_ready)
                        break;
                udelay(chip->chip_delay);
                chip->cmd_ctrl(mtd, NAND_CMD_STATUS,
                              NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
                chip->cmd_ctrl(mtd, NAND_CMD_NONE,
                               NAND_NCE | NAND_CTRL_CHANGE);
                while (!(chip->read_byte(mtd) & NAND_STATUS_READY)) ;
                return;

        case NAND_CMD_RNDOUT:
                /* No ready / busy check necessary */
                chip->cmd_ctrl(mtd, NAND_CMD_RNDOUTSTART,
                               NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
                chip->cmd_ctrl(mtd, NAND_CMD_NONE,
                               NAND_NCE | NAND_CTRL_CHANGE);
                return;

        case NAND_CMD_READ0:
                /* This applies to read commands, newer high density
                 * flash device needs a 2nd read cmd for READ0.
                 */
		//NAND_CLE_SET;
                chip->cmd_ctrl(mtd, NAND_CMD_READSTART,
                               NAND_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE,
                               NAND_NCE | NAND_CTRL_CHANGE);

	        NAND_CLE_CLEAR;

        default:
                /*
                 * If we don't have access to the busy pin, we apply the given
                 * command delay
                 */
                if (!chip->dev_ready) {
                        udelay(chip->chip_delay);
                        return;
                }
        }

        /* Apply this short delay always to ensure that we do wait tWB in
         * any case on any machine. */
        ndelay(100);

        nand_wait_ready(mtd);
}

static int lq_dma_mlcnand_intr_handler(struct dma_device_info *dma_dev, int status)
{
    struct mlc_nand_priv *mlc = mlc_nand_dev;
    int i;
    u32 ecc_stat;

    switch (status) {
        case RCV_INT:
	        for (i = 0; i < dma_dev->num_rx_chan; i++)
                dma_dev->rx_chan[i]->close(dma_dev->rx_chan[i]);

            ecc_stat = IFX_REG_R32(EBU_INT_STAT); 
            /* check whether ecc intr has occured 
	         * clear ecc intr 
             */
	        if (((ecc_stat >> 5) & 0x3) && (mlc->dma_ecc_mode == 1)) {
                mlc->ecc_status = 1;
				IFX_REG_W32((IFX_REG_R32(EBU_INT_STAT) | (3 << 5)), EBU_INT_STAT);
	        }
            asm("sync");
            DEBUG(MTD_DEBUG_LEVEL1, " ----  ecc_stat: %08x  ---- \n", ecc_stat);

            NAND_ALE_CLEAR;
            set_bit(MLCNAND_EVENT, &mlc->wait_flag);
            wake_up_interruptible(&mlc->mlc_nand_wait);
	 
            break;

        case TX_BUF_FULL_INT:
            for (i = 0; i < dma_dev->num_tx_chan; i++) {
                if (dma_dev->tx_chan[i]->control == IFX_DMA_CH_ON)
                     dma_dev->tx_chan[i]->enable_irq(dma_dev->tx_chan[i]);
            }
            break;

        case TRANSMIT_CPT_INT:
            for (i = 0; i < dma_dev->num_tx_chan; i++)
                 dma_dev->tx_chan[i]->disable_irq(dma_dev->tx_chan[i]);


            //NAND_ALE_CLEAR;
            set_bit(MLCNAND_EVENT, &mlc->wait_flag);
            wake_up_interruptible(&mlc->mlc_nand_wait);
           break;
    }
 

    return IFX_SUCCESS;

}

static u8 *mlc_nand_buffer_alloc(int len, int *byte_offset, void **opt)
{
    return NULL;
}

static int mlc_nand_buffer_free(u8 *dataptr, void *opt)
{
    return 0;
}

int mlc_nand_info_query(struct mlc_nand_priv *mlc, struct mtd_info *mtd)
{
    int writesize = mtd->writesize;
    int blocksize = mtd->erasesize;
    int addr_cycle, chipshift, pageshift, addr_cycle_count = 0;
    int pg_per_blk;
    struct nand_chip *chip = mtd->priv;

    mlc->pagesize = writesize;

    /* if EMBEDDED loc, change mtd write size, otherwise 
     * use as default nand pagesize
     */
    if (mlc->ecc_location) { //spare: 0, embed: 1
 	if (!mlc->ecc_mode) {  // 3b: 0, 4b: 1
		printk("NAND embedded mode does not support 3B ECC\n");
		return -EINVAL;	     
	}
	/* Overwrite the nand page size */
	mtd->writesize = writesize - (4 * (writesize / 128));  
    }

    pg_per_blk = ((blocksize / writesize) >> 6);
    mlc->pib = (pg_per_blk >= 4) ? 3 : pg_per_blk;

    /* if addr cycle is not calculated (for non ONFI flash) */
    if (!(mlc->addr_cycle)) {
        chipshift = chip->chip_shift;
        pageshift = chip->page_shift;

        /* column */
        for (; writesize > 0; ) {
            addr_cycle_count++;
            writesize >>= 10;
        }
        /* rows */
        addr_cycle = chipshift - (pageshift + 1);
        for (; addr_cycle > 0; ) {
            addr_cycle_count++;
            addr_cycle >>= 2;
        }

        mlc->addr_cycle = addr_cycle_count - 2;
    }
    //printk("writesize: %d, pib: %d\n",  mtd->writesize,  mlc->pib);
    //printk("writesize: %d, addr cycle: %d\n", mlc->pagesize, mlc->addr_cycle);

    return 0; 
}

static int mlc_nand_dma_setup(void)
{

    int i;

    dma_device = dma_device_reserve("HSNAND");
    if (dma_device == NULL)
    {
        printk("Reserve DMA for MLC NAND failed!\n");
        return -EAGAIN;
    }

    dma_device->intr_handler = &lq_dma_mlcnand_intr_handler;
    dma_device->buffer_alloc = &mlc_nand_buffer_alloc;
    dma_device->buffer_free = &mlc_nand_buffer_free;
    dma_device->tx_endianness_mode = IFX_DMA_ENDIAN_TYPE3;
    dma_device->rx_endianness_mode = IFX_DMA_ENDIAN_TYPE3;
    dma_device->tx_burst_len = MLC_NAND_DMA_BURST_LEN;
    dma_device->rx_burst_len = MLC_NAND_DMA_BURST_LEN;
    dma_device->num_rx_chan = 1;
    dma_device->num_tx_chan = 1;

    /* DMA Channel Config for TX direction */
    for (i = 0; i < dma_device->num_tx_chan; i++)
    {
        dma_device->tx_chan[i]->desc_len = 1;
        dma_device->tx_chan[i]->byte_offset = 0;
        dma_device->tx_chan[i]->control = IFX_DMA_CH_ON;
    }

    /* DMA Channel Config for RX direction */
    for (i = 0; i < dma_device->num_rx_chan; i++)
    {
        dma_device->rx_chan[i]->desc_len = 1;
        dma_device->rx_chan[i]->byte_offset = 0;
        dma_device->rx_chan[i]->control = IFX_DMA_CH_ON;
    }

    dma_device->current_tx_chan = 0;
    dma_device->current_rx_chan = 0;

    i = dma_device_register(dma_device);

    if (i != IFX_SUCCESS) {
        printk(KERN_ERR "%s[%d]: DMA register failed!\n", __func__, __LINE__);
        return -EAGAIN;
    }

#if 0
    for (i = 0; i < dma_device->num_rx_chan; i++) {
        //dma_device->rx_chan[i]->reset(dma_device->rx_chan[i]);
        //dma_device->rx_chan[i]->close(dma_device->rx_chan[i]);
    }
#endif

    return 0;
}

void pre_allocate_ecc_location(struct mtd_info *mtd, struct mlc_nand_priv *mlc,
                               struct nand_chip *chip, int oobsize)
{
    /* Allocate ecc layout info which are not 
     * supported by Linux kernel due to the diff.
     * oobsize 
     */ 

    /* check if spare area mode */
    if (!(mlc->ecc_location)) {

        printk("oobsize: %d\n", oobsize);

        switch (oobsize) {
            case 64:
               if (mlc->ecc_mode)
                   chip->ecc.layout = &B4_byte_ecc_oobinfo_2048;
                else
                   chip->ecc.layout = &B3_byte_ecc_oobinfo_2048;
                break;
             case 218:
                if (mlc->ecc_mode)
                   chip->ecc.layout = &B4_byte_ecc_oobinfo_4096_other;
                else
                   chip->ecc.layout = &B3_byte_ecc_oobinfo_4096_other;
                break;
              case 224:
                if (mlc->ecc_mode)
                    chip->ecc.layout = &B4_byte_ecc_oobinfo_4096;
                else
                    chip->ecc.layout = &B3_byte_ecc_oobinfo_4096;
                break;
              case 436:
              case 448:
                if (mlc->ecc_mode)
                    chip->ecc.layout = &B4_byte_ecc_oobinfo_8192;
                else
                    chip->ecc.layout = &B3_byte_ecc_oobinfo_8192;
                break;

              default:
                  printk("Unknown flash oobsize\n");
            }
        }
    /* embedded mode can ignore ecc layout */
    else {
        chip->ecc.layout = &embedded_ecc_generic;
    }

    mtd->ecclayout =  chip->ecc.layout;

}

void pre_scan_chip_info(struct mtd_info *mtd)
{
    struct mlc_nand_priv *mlc = mlc_nand_dev;
    struct nand_chip *chip = mtd->priv;
    struct nand_onfi_params *p = &chip->onfi_params;
    int i, ret, oobsize, extid, writesize;
    //int pmode;
    //u8 tmp[5];
    u8 id[8];
    u16 tmp_id;
   
    mlc->ecc_mode = MLC_ECC_MODE; // selected in menuconfig
    mlc->ecc_location = MLC_ECC_LOCATION; // selected in menuconfig
    mlc->ecc_strength = MLC_ECC_STRENGTH; // selected in menuconfig
    mlc->pagesize = 0;  //set to zero first and config later
    mlc->pcount = 0; /* try out one page read/write 1st */
    mlc->ecc_status = 0;
    mlc->addr_cycle = 0;
    mlc->pib = 0;

    chip->select_chip(mtd, 0);
    chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);
    chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

    for (i = 0; i < 8; i++) {
        id[i] = chip->read_byte(mtd);
    }

    tmp_id = (id[0] << 8) | id[1];
    mlc->chip_id = (int) tmp_id;

    printk("manf id: %02x, chip id: %02x\n", id[0], id[1]);

    ret = nand_flash_detect_onfi(mtd, chip, 0);
  
    if (ret) {
        //mlc->chip_id = p->model[0] << 8 | p->model[1]; 
        oobsize = le16_to_cpu(p->spare_bytes_per_page);

        mlc->addr_cycle = (p->addr_cycles >> 4) + (p->addr_cycles & 0xf);
        mlc->addr_cycle -= 3;
        //printk("addr cycle: %d\n", mlc->addr_cycle);

        /* multiplane rd/wr commands for ONFI flash */
        mlc->type = 1; // ONFI complient
        mlc->multiplane_wr_cmd = 0x80;
        mlc->multiplane_rd_cmd = 0x0;

        mlc->partial_page_attr = p->programs_per_page;
	//printk("Partial page write: %d\n", mlc->partial_page_attr); 
        /* Check multiplane support*/
#if 0
       if (le16_to_cpu(p->features) & (1 << 3)) 
           mlc->plane_mode = 1;
       else
           mlc->plane_mode = 0;
#endif    // support 1 plane xfer at the moment

        mlc->plane_mode = 0;
       
    }
    else { // if (ret) 
        extid = id[3];
        if (id[0] == id[6] && id[1] == id[7] &&
                          (id[0] == NAND_MFR_SAMSUNG || 
			   id[0] == NAND_MFR_HYNIX ) &&
                          (id[2] & NAND_CI_CELLTYPE_MSK) &&
                          id[5] != 0x00) {
         extid >>= 2;

         /* Calc oobsize */
         switch (extid & 0x03) {
                case 1:
		     if (id[0] == NAND_MFR_HYNIX)
                         oobsize = 224;
		     else
			 oobsize = 128;
                     break;
                case 2:
		     if (id[0] == NAND_MFR_HYNIX)
                         oobsize = 448;
		     else 
			 oobsize = 218;
                     break;
                case 3:
                     oobsize = 400;
                     break;
		default: /* 0 */
		     if (id[0] == NAND_MFR_HYNIX)
		         oobsize = 128;
                     else
		         oobsize = 436;
                }
    	        //printk("%s: oobsize: %d\n", __func__, oobsize);
        }
        else {
	         writesize = 1024 << (extid & 0x3);    
             extid >>= 2;
             /* Calc oobsize */
             oobsize = (8 << (extid & 0x01)) * (writesize >> 9); 
    	     //printk("oobsize: %d\n", oobsize);
       }

       /* for SLC NAND, it is possible to write both data & OOB area 
        * seperately
       */
       mlc->partial_page_attr = 2;

        /* multiplane rd/wr commands for non-ONFI flash*/
#if 0
        pmode = ((id[3] >> 2) & 0x3);
        if (!pmode)
            mlc->plane_mode = 0;
        else
            mlc->plane_mode = 1;
#endif    // support 1 plane xfer at the moment
        mlc->plane_mode = 0;
        mlc->type = 0;
#if 0
        if (id[0] == NAND_MFR_SAMSUNG) {
            mlc->multiplane_wr_cmd = 0x81;
            mlc->multiplane_rd_cmd = 0x60;
        }
        else {
            mlc->multiplane_wr_cmd = 0x80;
            mlc->multiplane_rd_cmd = 0x00;
        }
#endif
        mlc->multiplane_wr_cmd = 0x80;
        mlc->multiplane_rd_cmd = 0x00;
    }

    pre_allocate_ecc_location(mtd, mlc, chip, oobsize);
}

static int nand_define_flash_bbt(int pagesize, struct nand_chip *chip)
{
    struct mlc_nand_priv *mlc = mlc_nand_dev;
    
    switch(pagesize) {
        case 512:
                /* no oob area left for 4B ECC mode with spare area mode */
                if (mlc->ecc_mode) 
                    return -1;
        case 496:
                chip->cmdfunc = lq_mlcnand_command_sp;
                chip->badblock_pattern = &factory_default; 
                chip->bbt_td = &ifx_nand_main_desc_512;
                chip->bbt_md = &ifx_nand_mirror_desc_512; 
                break;
        case 2048:
        case 1984:
                /* no oob area left for 4B ECC mode with spare area mode */
                if (mlc->ecc_mode)
                    return -1;
                chip->cmdfunc = lq_mlcnand_command_lp;
                chip->badblock_pattern = &factory_default;
                chip->bbt_td = &ifx_nand_main_desc_2048;
                chip->bbt_md = &ifx_nand_mirror_desc_2048;
	        break;
        case 4096:
        case 3968:
                chip->cmdfunc = lq_mlcnand_command_lp;
                chip->badblock_pattern = &factory_default;
                chip->bbt_td = &ifx_nand_main_desc_4096;
                chip->bbt_md = &ifx_nand_mirror_desc_4096;
                break;
        case 8192:
        case 7936:
                chip->cmdfunc = lq_mlcnand_command_lp;
                chip->badblock_pattern = &factory_default;
                chip->bbt_td = &ifx_nand_main_desc_8192;
                chip->bbt_md = &ifx_nand_mirror_desc_8192;
                break;
        default:
               printk("Unable to determind page size for BBT definition\n");
               return -1;
    }
 
    return 0;
}


/* Setup of GPIO control pins */
void lq_mlc_nand_chip_init(void)
{
    u32 reg = 0;

    EBU_PMU_SETUP(IFX_EBU_ENABLE);

    IFX_REG_W32(0x0, IFX_EBU_CLC);

#if defined(CONFIG_NAND_CS0)
    reg = (NAND_BASE_ADDRESS & 0x1fffff00)| IFX_EBU_ADDSEL0_MASK(1)| IFX_EBU_ADDSEL0_REGEN;
    IFX_REG_W32(reg, IFX_EBU_ADDSEL0); 
    //printk("debug nand_addresssel0: %08x\n", IFX_REG_R32(IFX_EBU_ADDSEL0));
#endif

#if defined(CONFIG_NAND_CS1)    
    reg = (NAND_BASE_ADDRESS & 0x1fffff00)| IFX_EBU_ADDSEL1_MASK(2)| IFX_EBU_ADDSEL1_REGEN;
    IFX_REG_W32(reg, IFX_EBU_ADDSEL1); 
#endif

    /* Register GPIO pins */
    /* Port1.24 NAND_CLE used as output*/
    ifx_gpio_pin_reserve(IFX_NAND_CLE, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_CLE, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_CLE, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_CLE, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_CLE, IFX_GPIO_MODULE_NAND);
   
    /* Port0.13 NAND_ALE used as output */
    ifx_gpio_pin_reserve(IFX_NAND_ALE, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_ALE, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_ALE, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_ALE, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_ALE, IFX_GPIO_MODULE_NAND);

    /* Port3.48 NAND Read Busy used as input*/
    ifx_gpio_pin_reserve(IFX_NAND_RDY, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_in_set(IFX_NAND_RDY, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_RDY, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_RDY, IFX_GPIO_MODULE_NAND);

    /* Port3.49 NAND READ used as output */
    ifx_gpio_pin_reserve(IFX_NAND_RD, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_RD, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_RD, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_RD, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_RD, IFX_GPIO_MODULE_NAND);

    /* Port3.50 NAND_D1 used as output */
    ifx_gpio_pin_reserve(IFX_NAND_D1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_D1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_D1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_D1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_D1, IFX_GPIO_MODULE_NAND);

    /* Port3.51 NAND_D0 used as output */
    ifx_gpio_pin_reserve(IFX_NAND_D0, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_D0, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_D0, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_D0, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_D0, IFX_GPIO_MODULE_NAND);

    /* Port3.52 NAND_D2_P1 used as output */
    ifx_gpio_pin_reserve(IFX_NAND_D2_P1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_D2_P1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_D2_P1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_D2_P1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_D2_P1, IFX_GPIO_MODULE_NAND);

    /* Port3.53 NAND_D2_P2 used as output */
    ifx_gpio_pin_reserve(IFX_NAND_D2_P2, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_D2_P2, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_D2_P2, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_D2_P2, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_D2_P2, IFX_GPIO_MODULE_NAND);

    /* Port3.54 NAND_D6 used as output */
    ifx_gpio_pin_reserve(IFX_NAND_D6, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_D6, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_D6, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_D6, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_D6, IFX_GPIO_MODULE_NAND);

    /* Port3.55 NAND_D5_P1 used as output */
    ifx_gpio_pin_reserve(IFX_NAND_D5_P1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_D5_P1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_D5_P1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_D5_P1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_D5_P1, IFX_GPIO_MODULE_NAND);

    /* Port3.56 NAND_D5_P2 used as output */
    ifx_gpio_pin_reserve(IFX_NAND_D5_P2, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_D5_P2, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_D5_P2, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_D5_P2, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_D5_P2, IFX_GPIO_MODULE_NAND);

    /* Port3.57 NAND_D3 used as output */
    ifx_gpio_pin_reserve(IFX_NAND_D3, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_D3, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_D3, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_D3, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_D3, IFX_GPIO_MODULE_NAND);

    /* Port3.59 NAND_WR used as output */
    ifx_gpio_pin_reserve(IFX_NAND_WR, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_WR, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_WR, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_WR, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_WR, IFX_GPIO_MODULE_NAND);

    /* Port3.60 NAND_WP used as output */
    ifx_gpio_pin_reserve(IFX_NAND_WP, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_WP, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_WP, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_WP, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_WP, IFX_GPIO_MODULE_NAND);
    
    /* Port3.61 NAND_SE used as output */
    ifx_gpio_pin_reserve(IFX_NAND_SE, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_SE, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_SE, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_SE, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_SE, IFX_GPIO_MODULE_NAND);

#if defined(CONFIG_NAND_CS0)
    /* Port3.58 NAND CS0 used as output */
    ifx_gpio_pin_reserve(IFX_NAND_CS0, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_CS0, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_CS0, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_CS0, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_CS0, IFX_GPIO_MODULE_NAND);
    
    reg = 0;
    reg = IFX_EBU_BUSCON0_SETUP_EN |
          SM(IFX_EBU_BUSCON0_ALEC3, IFX_EBU_BUSCON0_ALEC) |
          SM(IFX_EBU_BUSCON0_WAITWRC7, IFX_EBU_BUSCON0_WAITWRC) |
          SM(IFX_EBU_BUSCON0_WAITRDC3, IFX_EBU_BUSCON0_WAITRDC) |
          SM(IFX_EBU_BUSCON0_HOLDC3, IFX_EBU_BUSCON0_HOLDC) |
          SM(IFX_EBU_BUSCON0_RECOVC3, IFX_EBU_BUSCON0_RECOVC) |
          SM(IFX_EBU_BUSCON0_CMULT8, IFX_EBU_BUSCON0_CMULT);
    //IFX_REG_W32(reg, IFX_EBU_BUSCON0);
    IFX_REG_W32(0x0040c7fe, IFX_EBU_BUSCON0);	

    reg = 0;
    reg = SM(IFX_EBU_NAND_CON_PRE_P_LOW, IFX_EBU_NAND_CON_PRE_P) |
          SM(IFX_EBU_NAND_CON_WP_P_LOW, IFX_EBU_NAND_CON_WP_P) |
          SM(IFX_EBU_NAND_CON_SE_P_LOW, IFX_EBU_NAND_CON_SE_P) |
          SM(IFX_EBU_NAND_CON_CS_P_LOW, IFX_EBU_NAND_CON_CS_P) |
          SM(IFX_EBU_NAND_CON_CSMUX_E_ENALBE, IFX_EBU_NAND_CON_CSMUX_E) |
          SM(IFX_EBU_NAND_CON_NANDM_ENABLE, IFX_EBU_NAND_CON_NANDM);
    IFX_REG_W32(reg, IFX_EBU_NAND_CON);
    
    //printk("Debug nand_con: %08x, buscon %08x\n", IFX_REG_R32(IFX_EBU_NAND_CON), 
    //			IFX_REG_R32(IFX_EBU_BUSCON0));
#elif defined(CONFIG_NAND_CS1)
    /* Port1.23 NAND CS1 used as output */
    ifx_gpio_pin_reserve(IFX_NAND_CS1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_dir_out_set(IFX_NAND_CS1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel0_set(IFX_NAND_CS1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_altsel1_clear(IFX_NAND_CS1, IFX_GPIO_MODULE_NAND);
    ifx_gpio_open_drain_set(IFX_NAND_CS1, IFX_GPIO_MODULE_NAND);

    reg = IFX_EBU_BUSCON1_SETUP_EN |
          SM(IFX_EBU_BUSCON1_ALEC3, IFX_EBU_BUSCON1_ALEC) |
          SM(IFX_EBU_BUSCON1_WAITWRC7, IFX_EBU_BUSCON1_WAITWRC) |
          SM(IFX_EBU_BUSCON1_WAITRDC3, IFX_EBU_BUSCON1_WAITRDC) |
          SM(IFX_EBU_BUSCON1_HOLDC3, IFX_EBU_BUSCON1_HOLDC) |
          SM(IFX_EBU_BUSCON1_RECOVC3, IFX_EBU_BUSCON1_RECOVC) |
          SM(IFX_EBU_BUSCON1_CMULT8, IFX_EBU_BUSCON1_CMULT);
    IFX_REG_W32(reg, IFX_EBU_BUSCON1);

    reg = 0;
    reg = SM(IFX_EBU_NAND_CON_OUT_CS1, IFX_EBU_NAND_CON_OUT_CS) |
	  SM(IFX_EBU_NAND_CON_IN_CS1, IFX_EBU_NAND_CON_IN_CS) |
	  SM(IFX_EBU_NAND_CON_PRE_P_LOW, IFX_EBU_NAND_CON_PRE_P) |
          SM(IFX_EBU_NAND_CON_WP_P_LOW, IFX_EBU_NAND_CON_WP_P) |
          SM(IFX_EBU_NAND_CON_SE_P_LOW, IFX_EBU_NAND_CON_SE_P) |
          SM(IFX_EBU_NAND_CON_CS_P_LOW, IFX_EBU_NAND_CON_CS_P) |
          SM(IFX_EBU_NAND_CON_CSMUX_E_ENALBE, IFX_EBU_NAND_CON_CSMUX_E) |
          SM(IFX_EBU_NAND_CON_NANDM_ENABLE, IFX_EBU_NAND_CON_NANDM);
    IFX_REG_W32(reg, IFX_EBU_NAND_CON);

#else
    printk("Error, unknown pin-select for NAND CS, please check menuconfig!\n");
#endif /* CONFIG_CSx */

    /* Enable ECC intr. and clean intr */
    IFX_REG_W32((IFX_REG_R32(EBU_INT_MSK_CTL) | (0x3 << 5)), EBU_INT_MSK_CTL);
    IFX_REG_W32((IFX_REG_R32(EBU_INT_STAT) | (0x3 << 5)), EBU_INT_STAT);

    NAND_WRITE(NAND_WRITE_CMD, NAND_WRITE_CMD_RESET); // Reset nand chip
    return;
}

static int __init lq_mlcnand_init(void)
{
    struct nand_chip *this;
    int err = 0;

    /* clear address */
    mlc_nand_dev = kmalloc(sizeof(struct mlc_nand_priv), GFP_KERNEL);
    if (!mlc_nand_dev)
    {
        printk("Unable to allocate memory for NAND device structure\n");
        err = -ENOMEM;
        return err;
    }
    mlc_nand_dev->ndac_ctl_1 = 0x0;
    mlc_nand_dev->ndac_ctl_2 = 0x0;
    mlc_nand_dev->current_page = 0x0;
    mlc_nand_dev->dma_ecc_mode = 1;

    printk("Initializing MLCNAND driver\n");
    lq_mlc_nand_chip_init();

    mlc_nand_mtd = kmalloc(sizeof(struct mtd_info) +
                             sizeof (struct nand_chip), GFP_KERNEL);
    if (!mlc_nand_mtd)
    {
        printk(KERN_ERR "Unable to allocate MLC MTD device structure\n");
        err = -ENOMEM;
        return err;
    }

    this = (struct nand_chip *)(&mlc_nand_mtd[1]);
    memset(mlc_nand_mtd, 0, sizeof(struct mtd_info));
    memset(this, 0, sizeof(struct nand_chip));

    mlc_nand_mtd->name = (char *) kmalloc(16, GFP_KERNEL);
    if (mlc_nand_mtd->name == NULL)
    {
        printk(KERN_ERR "Unable to allocate MLC MTD device name\n");
        err = -ENOMEM;
        goto out;
    }
    
    memset(mlc_nand_mtd, 0, 16);
    sprintf((char *)mlc_nand_mtd->name, LQ_MTD_MLC_NAND_BANK_NAME);

    /* Associate MTD priv members with the current MTD info*/
    mlc_nand_mtd->priv = this;
    mlc_nand_mtd->owner = THIS_MODULE;

    this->IO_ADDR_R = (void *) NAND_BASE_ADDRESS;
    this->IO_ADDR_W = (void *) NAND_BASE_ADDRESS;
    this->cmd_ctrl = lq_mlcnand_cmd_ctrl;

    /* 30 us command delay, similar to NAND driver specs */
    this->chip_delay = 30;

    /* hw ecc specific read/write functions */
    this->ecc.mode = NAND_ECC_HW;
    this->ecc.hwctl = lq_mlcnand_hwctl;
    this->ecc.write_page_raw = lq_nand_write_page_raw;
    this->ecc.write_page = lq_nand_write_page_hwecc;
    this->ecc.read_page_raw = lq_nand_read_page_raw;
    this->ecc.read_page = lq_nand_read_page_hwecc;
    this->ecc.read_oob = lq_nand_read_oob;
    this->ecc.write_oob = lq_nand_write_oob;

    this->options = NAND_SKIP_BBTSCAN | NAND_USE_FLASH_BBT |
		    NAND_NO_SUBPAGE_WRITE;

    this->read_byte = lq_mlcnand_read_byte;

    /* this is needed for the ONFI read of ~768B.
     * the ONFI read is done per byte using traditional EBU access.
     * DMA read is unable to handle such data width.
     */
    this->read_buf = lq_init_read_buf;

    this->write_buf = lq_nand_write_buf;
    this->verify_buf = lq_nand_verify_buf;
    this->dev_ready = lq_mlcnand_ready;
    this->select_chip = lq_mlcnand_select_chip;
    this->cmdfunc = lq_mlcnand_command_lp;

    err = mlc_nand_dma_setup();
    if (err < 0)
    {
        printk("MLC NAND DMA setup failed\n");
        goto out;
    }

    /* check if ONFI, initate ecc.layout to make linux kernel happy 
     * else BUG() will be called as 2.6.32 does not support nand flash
     * with oob size > 128B
     */
 
    pre_scan_chip_info(mlc_nand_mtd);

    if (mlc_nand_dev->ecc_mode)
        this->ecc.bytes = 4;
    else 
        this->ecc.bytes = 3;

    /* dummy otherwise nand scan will fail*/
    this->ecc.size = 128;

    printk("Probe for NAND Flash. . \n");
    if (nand_scan(mlc_nand_mtd, 1))
    {
        printk(KERN_ERR "Probing for NAND flash failed, flash not found!\n");
        err = -ENXIO;
        goto out;
    }
#if 1
    tmp_rd_buf = kmalloc(mlc_nand_mtd->writesize, GFP_KERNEL);
    if (!tmp_rd_buf) {
	    printk (KERN_ERR "Error allocating read buffer\n");
	    goto out;
    }

    tmp_wr_buf = kmalloc(mlc_nand_mtd->writesize, GFP_KERNEL);
    if (!tmp_wr_buf) {
        printk (KERN_ERR "Error allocating write buffer\n");
        goto out;
    }
#endif

    this->read_buf = lq_nand_read_buf;

    mlc_nand_info_query(mlc_nand_dev, mlc_nand_mtd);

    DEBUG(MTD_DEBUG_LEVEL1, "Chip id: %08x.%08x\n", mlc_nand_dev->chip_info[0],
		mlc_nand_dev->chip_info[1]);

    err = nand_define_flash_bbt(mlc_nand_mtd->writesize, this);

    if (err < 0)
        printk(KERN_ERR "There are some issues handling BBT definition, \
		please check your kernel configurations!\n");

    /* scan & create a bbt table where appropriate*/
    mlc_nand_dev->mtd_priv = mlc_nand_mtd;
    init_waitqueue_head(&mlc_nand_dev->mlc_nand_wait);
    this->scan_bbt(mlc_nand_mtd);

#ifdef CONFIG_MTD_PARTITIONS
#ifdef CONFIG_MTD_CMDLINE_PARTS
    int n = 0;
    struct mtd_partition *mtd_parts = NULL;

    n = parse_mtd_partitions(mlc_nand_mtd, part_probes, &mtd_parts, 0);
    if (n <= 0)
    {
        kfree(mlc_nand_mtd);
        return err;
    }
    add_mtd_partitions(mlc_nand_mtd, mtd_parts, n);
#else
    err = add_mtd_partitions(mlc_nand_mtd, g_ifx_mtd_nand_partitions, g_ifx_mtd_nand_partion_num);
#endif /* CONFIG_MTD_CMDLINE_PARTS */
    err = add_mtd_device(mlc_nand_mtd);
#endif /* CONFIG_MTD_PARTITIONS */

    printk("ecc size: %d, ecc bytes: %d, ecc total: %d, ecc step: %d, oobsize: %d\n",
		this->ecc.size, this->ecc.bytes, this->ecc.total, this->ecc.steps, mlc_nand_mtd->oobsize);
    printk("Success in initializing MLC NAND\n");

out:
    return err;

}

void __exit lq_mlcnand_exit(void)
{
    struct mlc_nand_priv *mlc = mlc_nand_dev;
    // NAND_ALE
    ifx_gpio_pin_free(IFX_NAND_ALE, IFX_GPIO_MODULE_NAND);
    // NAND_CLE
    ifx_gpio_pin_free(IFX_NAND_CLE, IFX_GPIO_MODULE_NAND);
    // CS1
    ifx_gpio_pin_free(IFX_NAND_CS1, IFX_GPIO_MODULE_NAND);
    // READY
    ifx_gpio_pin_free(IFX_NAND_RDY, IFX_GPIO_MODULE_NAND);
    // READ
    ifx_gpio_pin_free(IFX_NAND_RD, IFX_GPIO_MODULE_NAND);

    ifx_gpio_pin_free(IFX_NAND_D0, IFX_GPIO_MODULE_NAND);

    ifx_gpio_pin_free(IFX_NAND_D1, IFX_GPIO_MODULE_NAND);
 
    ifx_gpio_pin_free(IFX_NAND_D2_P1, IFX_GPIO_MODULE_NAND);    
 
    ifx_gpio_pin_free(IFX_NAND_D2_P2, IFX_GPIO_MODULE_NAND);

    ifx_gpio_pin_free(IFX_NAND_D3, IFX_GPIO_MODULE_NAND);

    ifx_gpio_pin_free(IFX_NAND_D5_P1, IFX_GPIO_MODULE_NAND);

    ifx_gpio_pin_free(IFX_NAND_D5_P2, IFX_GPIO_MODULE_NAND);
 
    ifx_gpio_pin_free(IFX_NAND_D6, IFX_GPIO_MODULE_NAND);

    ifx_gpio_pin_free(IFX_NAND_WR, IFX_GPIO_MODULE_NAND);

    ifx_gpio_pin_free(IFX_NAND_WP, IFX_GPIO_MODULE_NAND);
   
    ifx_gpio_pin_free(IFX_NAND_SE, IFX_GPIO_MODULE_NAND);

#if defined(CONFIG_NAND_CS0)
    ifx_gpio_pin_free(IFX_NAND_CS0, IFX_GPIO_MODULE_NAND);
#elif defined(CONFIG_NAND_CS1)
    ifx_gpio_pin_free(IFX_NAND_CS1, IFX_GPIO_MODULE_NAND);
#endif

    nand_release(mlc_nand_mtd);
 
    dma_device_release(dma_device);
    dma_device_unregister(dma_device);
    kfree (mlc_nand_mtd);
    kfree(mlc);
    kfree(tmp_rd_buf);
    kfree(tmp_wr_buf);

    return;

}

module_init(lq_mlcnand_init);
module_exit(lq_mlcnand_exit);


