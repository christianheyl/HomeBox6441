/****************************************************************************** 
** 
** FILE NAME    : ifxmips_nand.h 
** PROJECT      : UEIP 
** MODULES      : NAND Flash 
** 
** DATE         : 05 Aug 2008 
** AUTHOR       : Lei Chuanhua  
** DESCRIPTION  : NAND Flash MTD Driver 
** COPYRIGHT    :       Copyright (c) 2008 
**                      Infineon Technologies AG 
**                      Am Campeon 1-12, 85579 Neubiberg, Germany 
** 
**    This program is free software; you can redistribute it and/or modify 
**    it under the terms of the GNU General Public License as published by 
**    the Free Software Foundation; either version 2 of the License, or 
**    (at your option) any later version. 
** 
** HISTORY 
** $Date        $Author          $Version     $Comment 
** 05 Aug 2008  Lei Chuanhua       1.0        initial version 
** 08 Nov 2011  Mohammad Firdaus   1.1        Added Support for MLC NAND driver  
*******************************************************************************/ 
#ifndef IFXMIPS_NAND_H 
#define IFXMIPS_NAND_H 
 
#include <linux/mtd/nand.h> 
#include <linux/mtd/nand_ecc.h> 
 
#if defined(CONFIG_DANUBE) 
/*pliu:  20081107 +*/ 
#define ENABLE_PULSE_ALE                1 
/*1: ALE toggles between multiple address */ 
/*0: ALE stays valid across multiple address */ 
#define CS_LATCH		0 
/*1: CS stays valid until it clears explicitly */ 
/*0: CS toggles between multiple read/write cycles. ENABEL_PULSE_ALE has to 1 in this case */ 
#define CHIP_DELAY		20 
#define RDBY_NOT_USED		1 
/*1: ready/busy signal is not used */ 
/*pliu:  20081107 -*/ 
#endif 
 
#define IFX_EBU_ENABLE 		1 
 
#ifdef CONFIG_AR10 
#if defined (CONFIG_NAND_CS0) 
#define NAND_PHYS_BASE_CS0          0x10000000 
#define NAND_BASE_ADDRESS_CS0       (NAND_PHYS_BASE_CS0 | KSEG1) 
#define NAND_BASE_ADDRESS 	    NAND_BASE_ADDRESS_CS0 
#elif defined (CONFIG_NAND_CS1) 
#define NAND_PHYS_BASE_CS1	    0x14000000	 
#define NAND_BASE_ADDRESS_CS1       (NAND_PHYS_BASE_CS1 | KSEG1) 
#define NAND_BASE_ADDRESS	    NAND_BASE_ADDRESS_CS1 
#endif  /* CONFIG_NAND_CSx */ 
#else 
#define NAND_PHYS_BASE              0x14000000 
#define NAND_BASE_ADDRESS           (NAND_PHYS_BASE | KSEG1) 
#endif /* CONFIG_AR10 */ 
 
#define NAND_CON_CE         (1<<20)  
#define NAND_CON_ALE	    (1<<18) 
 
#define NAND_WRITE(addr, val)     *((volatile u8*)(NAND_BASE_ADDRESS | (addr))) = val; while((IFX_REG_R32(IFX_EBU_NAND_WAIT) & IFX_EBU_NAND_WAIT_WR_C) == 0); 
//#define NAND_READ(addr, val)      val = *((volatile u8*)(NAND_BASE_ADDRESS | (addr))); while((IFX_REG_R32(IFX_EBU_NAND_WAIT) & IFX_EBU_NAND_WAIT_WR_C) == 0); 
#define NAND_READ(addr, val)      val = *((volatile u8*)(NAND_BASE_ADDRESS | (addr))); while((IFX_REG_R32(IFX_EBU_NAND_WAIT) & IFX_EBU_NAND_WAIT_WR_C) == 0);
//#define NAND_CE_SET     IFX_REG_W32_MASK(0,NAND_CON_CE,IFX_EBU_NAND_CON); 
//#define NAND_CE_CLEAR   IFX_REG_W32_MASK(NAND_CON_CE,0,IFX_EBU_NAND_CON); 
#define NAND_READY        ((IFX_REG_R32(IFX_EBU_NAND_WAIT) & IFX_EBU_NAND_WAIT_RD) == IFX_EBU_NAND_WAIT_RD) 
#define NAND_READY_CLEAR  IFX_REG_W32(0,IFX_EBU_NAND_WAIT); 
//#define NAND_ALE_SET       IFX_REG_W32_MASK(0, NAND_CON_ALE, IFX_EBU_NAND_CON); 
//#define NAND_ALE_CLEAR     IFX_REG_W32_MASK(NAND_CON_ALE, 0, IFX_EBU_NAND_CON); 
 
#define NAND_CE_SET         IFX_REG_W32(IFX_REG_R32(IFX_EBU_NAND_CON) | (1<<20), IFX_EBU_NAND_CON); 
#define NAND_CE_CLEAR	    IFX_REG_W32(IFX_REG_R32(IFX_EBU_NAND_CON) & ~(1<<20), IFX_EBU_NAND_CON); 
#define NAND_ALE_SET        IFX_REG_W32(IFX_REG_R32(IFX_EBU_NAND_CON) | (1<<18), IFX_EBU_NAND_CON); 
#define NAND_ALE_CLEAR	    IFX_REG_W32(IFX_REG_R32(IFX_EBU_NAND_CON) & ~(1<<18), IFX_EBU_NAND_CON); 
#define NAND_CLE_SET        IFX_REG_W32(IFX_REG_R32(IFX_EBU_NAND_CON) | (1<<19), IFX_EBU_NAND_CON); 
#define NAND_CLE_CLEAR	    IFX_REG_W32(IFX_REG_R32(IFX_EBU_NAND_CON) & ~(1<<19), IFX_EBU_NAND_CON); 
/* 
 * In NAND_CON register, bit18~bit23 is related to the following cmd 
 * decoder  
 * Lat_en             cmd decoder      meaning 
 * --------------------------------------------- 
 * bit18              bit2             ALE 
 * bit19              bit3             CLE 
 * bit20              bit4             CS# 
 * bit21              bit5             SE# 
 * bit22              bit6             WP# 
 * bit23              bit7             PRE 
 */ 
  
#define NAND_CMD_ALE        (1<<2) 
#define NAND_CMD_CLE        (1<<3) 
#define NAND_CMD_NCLE       (0<<3) 
#define NAND_CMD_CS         (1<<4) 
 
#define NAND_WRITE_CMD      (NAND_CMD_CS | NAND_CMD_CLE) 
#define NAND_WRITE_CMD_LNAND (NAND_CMD_CS | NAND_CMD_NCLE)  
#define NAND_WRITE_ADDR     (NAND_CMD_CS | NAND_CMD_ALE) 
#define NAND_WRITE_DATA     (NAND_CMD_CS) 
#define NAND_READ_DATA      (NAND_CMD_CS) 
#define NAND_WRITE_CMD_RESET    0xff 
 
#if defined(CONFIG_DANUBE) 
#if defined(ENABLE_PULSE_ALE) && ENABLE_PULSE_ALE 
#define NAND_WRITE_ADDR_OVER         NAND_WRITE_ADDR 
#else 
#define NAND_WRITE_ADDR_OVER          (NAND_CMD_CS)  
#endif  
#endif //defined(CONFIG_DANUBE) 
 
#define IFX_ATC_NAND        0xc176 
#define IFX_BTC_NAND        0xc166 
#define ST_512WB2_NAND      0x2076 
#define SAMSUNG_512_3ADDR   0xec75 
#define HYNIX_MLC_FLASH	    0xaddc 
 
/* GPIO global view */ 
#define IFX_NAND_ALE    13 
#define IFX_NAND_CLE    24 
#define IFX_NAND_CS1    23 
#define IFX_NAND_RDY    48  /* NFLASH_READY */ 
#define IFX_NAND_RD     49  /* NFLASH_READ_N */ 
 
/* AR10 extra config */  
#define IFX_NAND_D1	50 
#define IFX_NAND_D0	51 
#define IFX_NAND_D2_P1  52 
#define IFX_NAND_D2_P2  53 
#define IFX_NAND_D6	54 
#define IFX_NAND_D5_P1	55 
#define IFX_NAND_D5_P2	56 
#define IFX_NAND_D3	57 
#define IFX_NAND_WR	59 
#define IFX_NAND_WP	60 
#define IFX_NAND_SE	61 
#define IFX_NAND_CS0	58 /* Use CS0 */ 
#define IFX_NAND_CS1	23 /* Use CS1 */ 
 
#define BBT_D (struct nand_bbt_descr) 
#define ECC_L (struct nand_ecclayout) 
 
/* bad block table descriptor which defines where in the 
 * NAND device the bad block indicator is stored. This information 
 * can be found in the NAND device data sheet (pattern, location, etc) 
 * Be careful not to write over the bad block indicators 
 */ 
 
uint8_t ifx_nand_2048_bbt_pattern[] = { 0xff, 0xff }; 
uint8_t ifx_nand_512_bbt_pattern[] = {0xff, 0xff}; 
uint8_t generic_bbt_pattern[] = {'B', 'b', 't', '0' }; 
uint8_t generic_mirror_pattern[] = {'1', 't', 'b', 'B' }; 
 
#if defined(CONFIG_MTD_IFX_MLCNAND)
struct nand_bbt_descr ifx_nand_main_desc_8192 = { 
    .options = NAND_BBT_CREATE | NAND_BBT_WRITE | NAND_BBT_SEARCH 
               | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_LASTBLOCK, 
    .offs = 264, 
    .len = 4, 
    .veroffs = 268, 
    .maxblocks = 4, 
    .pattern = generic_bbt_pattern, 
}; 
 
struct nand_bbt_descr ifx_nand_mirror_desc_8192 = { 
    .options = NAND_BBT_CREATE | NAND_BBT_WRITE | NAND_BBT_SEARCH 
               | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_LASTBLOCK, 
    .offs = 264, 
    .len = 4, 
    .veroffs = 268, 
    .maxblocks = 4, 
    .pattern = generic_mirror_pattern, 
}; 
 
struct nand_bbt_descr ifx_nand_main_desc_4096 = { 
    .options = NAND_BBT_CREATE | NAND_BBT_WRITE | NAND_BBT_SEARCH 
               | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_LASTBLOCK, 
    .offs = 136, 
    .len = 4, 
    .veroffs = 140, 
    .maxblocks = 4, 
    .pattern = generic_bbt_pattern, 
}; 
 
struct nand_bbt_descr ifx_nand_mirror_desc_4096 = { 
    .options = NAND_BBT_CREATE | NAND_BBT_WRITE | NAND_BBT_SEARCH 
               | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_LASTBLOCK, 
    .offs = 136, 
    .len = 4, 
    .veroffs = 140, 
    .maxblocks = 4, 
    .pattern = generic_mirror_pattern, 
}; 

#endif /* CONFIG_MTD_IFX_MLCNAND */
 
struct nand_bbt_descr ifx_nand_main_desc_2048 = { 
    .options = NAND_BBT_CREATE | NAND_BBT_WRITE | NAND_BBT_SEARCH 
               | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_LASTBLOCK, 
    .offs = 56, 
    .len = 4, 
    .veroffs = 60, 
    .maxblocks = 4, 
    .pattern = generic_bbt_pattern, 
}; 
 
struct nand_bbt_descr ifx_nand_mirror_desc_2048 = { 
    .options = NAND_BBT_CREATE | NAND_BBT_WRITE | NAND_BBT_SEARCH 
               | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_LASTBLOCK, 
    .offs = 56, 
    .len = 4, 
    .veroffs = 60, 
    .maxblocks = 4, 
    .pattern = generic_mirror_pattern, 
}; 
 
struct nand_bbt_descr ifx_nand_main_desc_512 = { 
    .options = NAND_BBT_CREATE | NAND_BBT_WRITE | NAND_BBT_SEARCH 
               | NAND_BBT_2BIT | NAND_BBT_ABSPAGE | NAND_BBT_LASTBLOCK, 
    .offs = 15, 
    .len = 1, 
    //.veroffs = 14, 
    .maxblocks = 4, 
    .pattern = generic_bbt_pattern, 
 
}; 
 
struct nand_bbt_descr ifx_nand_mirror_desc_512 = { 
    .options = NAND_BBT_CREATE | NAND_BBT_WRITE | NAND_BBT_SEARCH 
               | NAND_BBT_2BIT | NAND_BBT_ABSPAGE | NAND_BBT_LASTBLOCK, 
    .offs = 15, 
    .len = 1, 
    //.veroffs = 14, 
    .maxblocks = 4, 
    .pattern = generic_mirror_pattern, 
 
}; 
 
struct nand_bbt_descr factory_default = { 
    .options = NAND_BBT_SCAN2NDPAGE,  
    .offs = 0, 
    .len = 2, 
    //.veroffs = 14, 
    .pattern = ifx_nand_2048_bbt_pattern, 
}; 
 
 
/* Generic flash bbt decriptors. BBT layout that are predefined  
 * in chips can be defined here.  
*/ 
 
struct nand_bbt_descr generic_bbt_main_descr = { 
        .options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE 
                | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP, 
        .offs = 8, 
        .len = 4, 
        .veroffs = 12, 
        .maxblocks = 4, 
        .pattern = generic_bbt_pattern 
}; 
 
struct nand_bbt_descr generic_bbt_mirror_descr = { 
        .options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE 
                | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP, 
        .offs = 8, 
        .len = 4, 
        .veroffs = 12, 
        .maxblocks = 4, 
        .pattern = generic_mirror_pattern 
}; 
 
/* --> 512 page size for Samsung chips, bad block info 
 * is located @ the last byte of the oob (16th byte) 
 * area of the 1st two pages of every block. 
 * --> 2048 page size for Samsung chips, bad block info 
 * is located @ the first byte of the oob area of 
 * the first two pages of every block 
 */ 
 
struct nand_ecclayout ifx_oobinfo_512 = { 
    .eccbytes = 3, 
    .eccpos = {0, 1, 2}, 
    .oobavail = 9, 
    .oobfree = {{4, 13}} 
}; 
 
struct nand_ecclayout ifx_oobinfo_2048 = { 
    .eccbytes = 12, 
    .eccpos = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, 
    .oobavail = 50, 
    .oobfree = {{13, 63}} 
}; 
 
/* MLC driver ecc layout */ 
#if defined(CONFIG_MTD_IFX_MLCNAND)
struct nand_ecclayout B4_byte_ecc_oobinfo_2048 = { 
    .eccbytes = 64, 
    .eccpos = {0, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 
               13, 14, 15, 16, 17, 18, 19, 20, 21, 22,  
               23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 
               33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 
               43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 
               53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 
               63}, 
    .oobavail = 0, 
    .oobfree = {{0, 0}} 
}; 
 
struct nand_ecclayout B4_byte_ecc_oobinfo_4096 = { 
    .eccbytes = 128, 
    .eccpos = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 
		13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 
		23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 
		33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 
		43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 
		53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 
		63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 
		73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 
		83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 
		93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 
		103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 
		113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 
		123, 124, 125, 126, 127, 128  },     
    .oobavail = 96, 
    .oobfree = {{129, 96}} 
}; 
 
struct nand_ecclayout B4_byte_ecc_oobinfo_4096_other = { 
    .eccbytes = 128, 
    .eccpos = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 
		13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 
		23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 
		33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 
		43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 
		53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 
		63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 
		73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 
		83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 
		93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 
		103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 
		113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 
		123, 124, 125, 126, 127, 128 }, 
    .oobavail = 90, 
    .oobfree = {{129, 90}} 
}; 
 
struct nand_ecclayout B4_byte_ecc_oobinfo_8192 = { 
    .eccbytes = 256, 
    .eccpos = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 
		13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 
		23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 
		33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 
		43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 
		53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 
		63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 
		73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 
		83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 
		93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 
		103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 
		113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 
		123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 
		133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 
		143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 
		153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 
		163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 
		173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 
		183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 
		193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 
		203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 
		213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 
		223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 
		233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 
		243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 
		253, 254, 255, 256 }, 
    .oobavail = 176, 
    .oobfree = {{257, 176}} 
}; 
 
struct nand_ecclayout B3_byte_ecc_oobinfo_2048 = { 
    .eccbytes = 48, 
    .eccpos = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 
		13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 
		23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 
		33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 
		43, 44, 45, 46, 47 }, 
    .oobfree = {{48, 16}} 
}; 
 
struct nand_ecclayout B3_byte_ecc_oobinfo_4096 = { 
    .eccbytes = 96, 
    .eccpos = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 
		13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 
		23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 
		33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 
		43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 
		53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 
		63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 
		73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 
		83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 
		93, 94, 95, 96 	}, 
    .oobavail = 128, 
    .oobfree = {{97, 127}} 
}; 
 
struct nand_ecclayout B3_byte_ecc_oobinfo_4096_other = { 
    .eccbytes = 96, 
    .eccpos = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 
		13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 
		23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 
		33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 
		43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 
		53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 
		63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 
		73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 
		83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 
		93, 94, 95, 96 }, 
    .oobavail = 122, 
    .oobfree = {{97, 121}} 
}; 
 
struct nand_ecclayout B3_byte_ecc_oobinfo_8192 = { 
    .eccbytes = 192, 
    .eccpos = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 
		13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 
		23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 
		33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 
		43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 
		53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 
		63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 
		73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 
		83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 
		93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 
		103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 
		113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 
		123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 
		133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 
		143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 
		153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 
		163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 
		173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 
		183, 184, 185, 186, 187, 188, 189, 190, 191 }, 
    .oobavail = 240, 
    .oobfree = {{192, 241}} 
}; 
 
struct nand_ecclayout embedded_ecc_generic = { 
    .eccbytes = 0, 
    .eccpos = {0}, 
}; 
#endif /* CONFIG_MTD_IFX_MLCNAND */
 
/* OOB info for generic chips. For specific chips with 
 * predefined factry default OOB info, create a structure  
 * with the specifics of the chip's OOB layout 
 */ 
 
 
struct nand_ecclayout oobinfo_512_generic  = { 
        .eccbytes = 6, 
        .eccpos = {0, 1, 2, 3, 6, 7}, 
        .oobfree = { 
                {.offset = 8, 
                 . length = 8}} 
}; 
 
struct nand_ecclayout oobinfo_2048_generic = { 
        .eccbytes = 24, 
        .eccpos = { 
                   40, 41, 42, 43, 44, 45, 46, 47, 
                   48, 49, 50, 51, 52, 53, 54, 55, 
                   56, 57, 58, 59, 60, 61, 62, 63}, 
	.oobavail = 38, 
        .oobfree = { 
                {.offset = 2, 
                 .length = 38}} 
}; 
 
struct nand_extra_info { 
    u32 chip_id; 
    int addr_cycle; 
    struct nand_ecclayout *chip_ecclayout; 
    struct nand_bbt_descr *chip_bbt_main_descr; 
    struct nand_bbt_descr *chip_bbt_mirror_descr; 
}; 
 
struct nand_extra_info chip_extra_info[] = { 
    { 0xec75, 3, &ifx_oobinfo_512, &ifx_nand_main_desc_512, &ifx_nand_mirror_desc_512 }, 
    { 0xecf1, 5, &ifx_oobinfo_2048, &ifx_nand_main_desc_2048, &ifx_nand_mirror_desc_2048 }, 
    { 0xecdc, 5, &oobinfo_2048_generic, &ifx_nand_main_desc_2048, &ifx_nand_mirror_desc_2048 }, 
    { 0xc176, 4, NULL, NULL, NULL },  // IFX nand 
    { 0xecd3, 5, NULL, NULL, NULL }, //samsung mlc 
    { 0xaddc, 5, NULL, NULL, NULL }, // hynix 
    //{ 0x2c68, 5, &B4_byte_ecc_oobinfo_4096_other, &ifx_nand_main_desc_4096, &ifx_nand_mirror_desc_4096 }, // micron 
    { 0x2c88, 5, NULL, NULL, NULL }, // micron 
    { 0xFFFF, 3, &oobinfo_512_generic, &generic_bbt_main_descr, &generic_bbt_mirror_descr }, 
    { 0xFFFF, 3, &oobinfo_2048_generic, &generic_bbt_main_descr, &generic_bbt_mirror_descr }, 
 
 
}; 
 
#endif /* IFXMIPS_NAND_H */ 
 
