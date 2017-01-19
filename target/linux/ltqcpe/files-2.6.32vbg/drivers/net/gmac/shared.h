/*
 * linux//drivers/net/gmac/shared.h
 *
 * Copyright (C) 2011 ....
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
 */

#if !defined(__SHARED_H__)
#define __SHARED_H__

#include <asm/types.h>

#ifndef CONFIG_VBG400_CHIPIT

/*
*** TODO: fill database with Reversed Mode Data (Reversed mode not supported at this stage)
*/

/*************************************************************************************
** These definitions are for the shared register configuration (clock and delay configuration)
*/

/****************************************************************
* REG:
* GEN3_SHRD_GMAC_MODE_REG:
*/

/* fields mask definitions for gen3_gmac_mode_reg */
#define GMAC_MODE_GMAC0_SEL_CLK_SWITCH_MASK         0x00000003
#define GMAC_MODE_GMAC0_DIV_CLK_SRC_SEL_MASK        0x0000000C
#define GMAC_MODE_GMAC0_PHY_CLK_REF_OE_N_MASK       0x00000010
#define GMAC_MODE_RESERVED_MASK                     0x00000020
#define GMAC_MODE_GMAC0_INV_PHY_CLK_TX_OUT_MASK     0x00000040
#define GMAC_MODE_GMAC0_PHY_INTERFACE_SELMASK       0x00000380
#define GMAC_MODE_GMAC0_PHY_CLK_RX_OE_N_MASK        0x00000400
#define GMAC_MODE_GMAC0_PHY_CLK_TX_OE_N_MASK        0x00000800
#define GMAC_MODE_GMAC0_USE_EXT_PHY_CLK_TX_MASK     0x00001000
#define GMAC_MODE_GMII_MODE_MASK                    0x00002000
#define GMAC_MODE_PHY_CLK_REF_OE_N_MASK             0x00004000
#define GMAC_MODE_RESERVED_MASK_1                   0x00008000
#define GMAC_MODE_GMAC1_SEL_CLK_SWITCH_MASK         0x00030000
#define GMAC_MODE_GMAC1_DIV_CLK_SRC_SEL_MASK        0x000c0000
#define GMAC_MODE_RESERVED_MASK_2                   0x00300000
#define GMAC_MODE_GMAC1_INV_PHY_CLK_TX_OUT_MASK     0x00400000
#define GMAC_MODE_GMAC1_PHY_INTERFACE_SEL_MASK      0x01800000
#define GMAC_MODE_GMAC1_PHY_CLK_RX_OE_N_MASK        0x02000000
#define GMAC_MODE_GMAC1_PHY_CLK_TX_OE_N_MASK        0x04000000
#define GMAC_MODE_GMAC_SMA_LP_MASK                  0x08000000
#define GMAC_MODE_GMAC1_MSTR_SMA_MASK               0x10000000
#define GMAC_MODE_GMAC_REVMII_SYNPS_MASK            0x20000000
#define GMAC_MODE_GMAC0_REF_CLK_SEL_MASK            0x40000000
#define GMAC_MODE_GMAC1_REF_CLK_SEL_MASK            0x80000000

/*Reg MASK: registers are shared betwenn gma0/gma1,
* so there are two different mask definitions according gmac index:
*/
#define GMAC_MODE_REG_GMAC0_MASK    (GMAC_MODE_GMAC0_SEL_CLK_SWITCH_MASK |\
                                    GMAC_MODE_GMAC0_DIV_CLK_SRC_SEL_MASK |\
                                    GMAC_MODE_GMAC0_PHY_CLK_REF_OE_N_MASK |\
                                    GMAC_MODE_GMAC0_INV_PHY_CLK_TX_OUT_MASK |\
                                    GMAC_MODE_GMAC0_PHY_INTERFACE_SELMASK |\
                                    GMAC_MODE_GMAC0_PHY_CLK_RX_OE_N_MASK |\
                                    GMAC_MODE_GMAC0_PHY_CLK_TX_OE_N_MASK |\
                                    GMAC_MODE_GMAC0_USE_EXT_PHY_CLK_TX_MASK |\
                                    GMAC_MODE_GMII_MODE_MASK |\
                                    GMAC_MODE_PHY_CLK_REF_OE_N_MASK |\
                                    GMAC_MODE_GMAC_SMA_LP_MASK |\
                                    GMAC_MODE_GMAC_REVMII_SYNPS_MASK |\
                                    GMAC_MODE_GMAC1_MSTR_SMA_MASK |\
                                    GMAC_MODE_GMAC0_REF_CLK_SEL_MASK)

#define GMAC_MODE_REG_GMAC1_MASK    (GMAC_MODE_GMAC1_SEL_CLK_SWITCH_MASK |\
                                    GMAC_MODE_GMAC1_DIV_CLK_SRC_SEL_MASK |\
                                    GMAC_MODE_GMAC1_INV_PHY_CLK_TX_OUT_MASK |\
                                    GMAC_MODE_GMAC1_PHY_INTERFACE_SEL_MASK |\
                                    GMAC_MODE_GMAC1_PHY_CLK_RX_OE_N_MASK |\
                                    GMAC_MODE_GMAC1_PHY_CLK_TX_OE_N_MASK |\
                                    GMAC_MODE_GMAC_SMA_LP_MASK |\
                                    GMAC_MODE_GMAC1_MSTR_SMA_MASK |\
                                    GMAC_MODE_GMAC_REVMII_SYNPS_MASK |\
                                    GMAC_MODE_GMAC1_REF_CLK_SEL_MASK)
/*
#define GMAC_MODE_REG_GMAC1_MASK    (GMAC_MODE_GMII_MODE_MASK |\
                                    GMAC_MODE_PHY_CLK_REF_OE_N_MASK |\
                                    GMAC_MODE_GMAC1_SEL_CLK_SWITCH_MASK |\
                                    GMAC_MODE_GMAC1_DIV_CLK_SRC_SEL_MASK |\
                                    GMAC_MODE_GMAC1_INV_PHY_CLK_TX_OUT_MASK |\
                                    GMAC_MODE_GMAC1_PHY_INTERFACE_SEL_MASK |\
                                    GMAC_MODE_GMAC1_PHY_CLK_RX_OE_N_MASK |\
                                    GMAC_MODE_GMAC1_PHY_CLK_TX_OE_N_MASK |\
                                    GMAC_MODE_GMAC_SMA_LP_MASK |\
                                    GMAC_MODE_GMAC1_MSTR_SMA_MASK |\
                                    GMAC_MODE_GMAC_REVMII_SYNPS_MASK |\
                                    GMAC_MODE_GMAC1_REF_CLK_SEL_MASK)
*/

/*Reg DATA is Interface & Speed related, so there are many options:
*/
#define GMAC_MODE_REG_MII_10M_DATA          0xC0004C10
#define GMAC_MODE_REG_MII_10M_GMAC1_DATA          0/*not legal interface*/
#define GMAC_MODE_REG_MII_100M_DATA         0xC0004C10
#define GMAC_MODE_REG_MII_100M_GMAC1_DATA          0/*not legal interface*/
#define GMAC_MODE_REG_GMII_1000M_DATA       0xC0004c50
#define GMAC_MODE_REG_GMII_1000M_GMAC1_DATA       0/*not legal interface*/
#define GMAC_MODE_REG_RGMII_10M_DATA        0xC0004491
#define GMAC_MODE_REG_RGMII_10M_GMAC1_DATA        0xD2814491/*last work=0xD2814491*//*0xC2811491*//*0x02810000*/
#define GMAC_MODE_REG_RGMII_100M_DATA       0xC0004492
#define GMAC_MODE_REG_RGMII_100M_GMAC1_DATA       0xD2820000/*last work=0xD2824492*//*0xD28E4492*//*0xC2821492*//*0x02820000*/
#define GMAC_MODE_REG_RGMII_1000M_DATA      0xC0004490
#define GMAC_MODE_REG_RGMII_1000M_GMAC1_DATA      0xD2800000/*last work=0xD2804490*//*0xC2801490*//*0x02800000*/
#define GMAC_MODE_REG_RMII_10M_DATA         0xC0004C10
#define GMAC_MODE_REG_RMII_10M_GMAC1_DATA         0x07000000
#define GMAC_MODE_REG_RMII_100M_DATA        0xC0004C10
#define GMAC_MODE_REG_RMII_100M_GMAC1_DATA        0x07000000
/*the next is conditionaly used in the handlers:*/
#define GMAC_MODE_REG_SMA_MASTER_GMAC0_FIELD    0x0
#define GMAC_MODE_REG_SMA_MASTER_GMAC1_FIELD    0x10000000
#define GMAC_MODE_REG_SMA_MASTER_REVERSED_FIELD 0x18000000

/**** Reversed ****************************
*/
#define GMAC_MODE_REG_MII_10M_R_DATA        0//TODO
#define GMAC_MODE_REG_MII_10M_R_GMAC1_DATA        0/*not legal interface*/
#define GMAC_MODE_REG_MII_100M_R_DATA       0//TODO
#define GMAC_MODE_REG_MII_100M_R_GMAC1_DATA       0/*not legal interface*/
#define GMAC_MODE_REG_GMII_1000M_R_DATA     0//TODO
#define GMAC_MODE_REG_GMII_1000M_R_GMAC1_DATA     0/*not legal interface*/
#define GMAC_MODE_REG_RGMII_10M_R_DATA      0//TODO
#define GMAC_MODE_REG_RGMII_10M_R_GMAC1_DATA      0/*not legal interface*/
#define GMAC_MODE_REG_RGMII_100M_R_DATA     0//TODO
#define GMAC_MODE_REG_RGMII_100M_R_GMAC1_DATA     0/*not legal interface*/
#define GMAC_MODE_REG_RGMII_1000M_R_DATA    0//TODO
#define GMAC_MODE_REG_RGMII_1000M_R_GMAC1_DATA    0/*not legal interface*/
#define GMAC_MODE_REG_RMII_10M_R_DATA       0//TODO
#define GMAC_MODE_REG_RMII_10M_R_GMAC1_DATA       0/*not legal interface*/
#define GMAC_MODE_REG_RMII_100M_R_DATA      0//TODO
#define GMAC_MODE_REG_RMII_100M_R_GMAC1_DATA      0/*not legal interface*/

/****************************************************************
*  REG:
*  GEN3_SHRD_GMAC_DIV_RATIO_REG:
*/

/* fields mask definitions for gen3_gmac_mode2_reg */
#define GMAC_DIV_RATIO_GMAC0_DIV_RATIO_MASK             0x7F
#define GMAC_DIV_RATIO_GMAC0_INV_PHY_CLK_RX_MASK        0x08
#define GMAC_DIV_RATIO_GMAC1_DIV_RATIO_MASK             0x7F00
#define GMAC_DIV_RATIO_GMAC1_INV_PHY_CLK_RX_MASK        0x8000
#define GMAC_DIV_RATIO_GMAC0_CDIV_ENABLE_MASK           0x30000
#define GMAC_DIV_RATIO_GMAC1_CDIV_ENABLE_MASK           0xC0000

/*Reg MASK is GMAC related, so there are two different mask definitions:
*/
#define GMAC_DIV_RATIO_REG_GMAC0_MASK       (GMAC_DIV_RATIO_GMAC0_DIV_RATIO_MASK |\
                                            GMAC_DIV_RATIO_GMAC0_INV_PHY_CLK_RX_MASK |\
                                            GMAC_DIV_RATIO_GMAC0_CDIV_ENABLE_MASK)
                                            
#define GMAC_DIV_RATIO_REG_GMAC1_MASK       (GMAC_DIV_RATIO_GMAC1_DIV_RATIO_MASK |\
                                            GMAC_DIV_RATIO_GMAC1_INV_PHY_CLK_RX_MASK |\
                                            GMAC_DIV_RATIO_GMAC1_CDIV_ENABLE_MASK)

/*Reg DATA is Interface & Speed related, so there are many options:
*/
#define GMAC_DIV_RATIO_REG_MII_10M_DATA       0
#define GMAC_DIV_RATIO_REG_MII_10M_GMAC1_DATA       0/*not legal interface*/       
#define GMAC_DIV_RATIO_REG_MII_100M_DATA      0
#define GMAC_DIV_RATIO_REG_MII_100M_GMAC1_DATA      0/*not legal interface*/      
#define GMAC_DIV_RATIO_REG_GMII_1000M_DATA    0
#define GMAC_DIV_RATIO_REG_GMII_1000M_GMAC1_DATA    0/*not legal interface*/    
#define GMAC_DIV_RATIO_REG_RGMII_10M_DATA     0x00020031
#define GMAC_DIV_RATIO_REG_RGMII_10M_GMAC1_DATA     0x00083131/*last work=0x000a3131*//*0x000e3131*//*0x000c3100*/     
#define GMAC_DIV_RATIO_REG_RGMII_100M_DATA    0x00010000
#define GMAC_DIV_RATIO_REG_RGMII_100M_GMAC1_DATA    0x00040000/*last work=0x00050000*//*0x00040000/*0x00040000*/
#define GMAC_DIV_RATIO_REG_RGMII_1000M_DATA   0
#define GMAC_DIV_RATIO_REG_RGMII_1000M_GMAC1_DATA   0x0   
#define GMAC_DIV_RATIO_REG_RMII_10M_DATA      0
#define GMAC_DIV_RATIO_REG_RMII_10M_GMAC1_DATA      0x0      
#define GMAC_DIV_RATIO_REG_RMII_100M_DATA     0
#define GMAC_DIV_RATIO_REG_RMII_100M_GMAC1_DATA     0x0     

/**** Reversed ****************************
*/
#define GMAC_DIV_RATIO_REG_MII_10M_R_DATA     0//TODO
#define GMAC_DIV_RATIO_REG_MII_10M_R_GMAC1_DATA         0/*not legal interface*/
#define GMAC_DIV_RATIO_REG_MII_100M_R_DATA    0//TODO
#define GMAC_DIV_RATIO_REG_MII_100M_R_GMAC1_DATA        0/*not legal interface*/
#define GMAC_DIV_RATIO_REG_GMII_1000M_R_DATA  0//TODO
#define GMAC_DIV_RATIO_REG_GMII_1000M_R_GMAC1_DATA      0/*not legal interface*/
#define GMAC_DIV_RATIO_REG_RGMII_10M_R_DATA   0//TODO
#define GMAC_DIV_RATIO_REG_RGMII_10M_R_GMAC1_DATA       0/*not legal interface*/
#define GMAC_DIV_RATIO_REG_RGMII_100M_R_DATA  0//TODO
#define GMAC_DIV_RATIO_REG_RGMII_100M_R_GMAC1_DATA      0/*not legal interface*/
#define GMAC_DIV_RATIO_REG_RGMII_1000M_R_DATA 0//TODO
#define GMAC_DIV_RATIO_REG_RGMII_1000M_R_GMAC1_DATA     0/*not legal interface*/
#define GMAC_DIV_RATIO_REG_RMII_10M_R_DATA    0//TODO
#define GMAC_DIV_RATIO_REG_RMII_10M_R_GMAC1_DATA        0/*not legal interface*/
#define GMAC_DIV_RATIO_REG_RMII_100M_R_DATA   0//TODO
#define GMAC_DIV_RATIO_REG_RMII_100M_R_GMAC1_DATA       0/*not legal interface*/

/****************************************************************
*  REG:
*  GEN3_SHRD_GMAC_DLY_PGM_REG:
*/

/* fields mask definitions for gen3_gmac_dly_pgm */
#define GMAC_DLY_PGM_GMAC0_DELAY_PHY_CLOCK_RX_MASK             0x7F
#define GMAC_DLY_PGM_GMAC0_DELAY_PHY_CLOCK_TX_MASK             0x7F00
#define GMAC_DLY_PGM_GMAC1_DELAY_PHY_CLOCK_RX_MASK             0x7F0000
#define GMAC_DLY_PGM_GMAC1_DELAY_PHY_CLOCK_TX_MASK             0x7F000000

/*Reg MASK is GMAC related, so there are two different mask definitions:
*/
/*For gmac0/1_dly_phy_clk_tx value already in place, do not clear it,
clk_rx is not important*/
#define GMAC_DLY_PGM_REG_GMAC0_MASK    (GMAC_DLY_PGM_GMAC0_DELAY_PHY_CLOCK_RX_MASK)

#define GMAC_DLY_PGM_REG_GMAC1_MASK    (GMAC_DLY_PGM_GMAC1_DELAY_PHY_CLOCK_RX_MASK)

/*Reg DATA is done dynamically:
  - therefore same value = 0 for both mac0 and mac1
*/
#define GMAC_DLY_PGM_REG_MII_10M_DATA       0
#define GMAC_DLY_PGM_REG_MII_10M_GMAC1_DATA       0
#define GMAC_DLY_PGM_REG_MII_100M_DATA      0
#define GMAC_DLY_PGM_REG_MII_100M_GMAC1_DATA      0
#define GMAC_DLY_PGM_REG_GMII_1000M_DATA    0
#define GMAC_DLY_PGM_REG_GMII_1000M_GMAC1_DATA    0
#define GMAC_DLY_PGM_REG_RGMII_10M_DATA     0
#define GMAC_DLY_PGM_REG_RGMII_10M_GMAC1_DATA     0
#define GMAC_DLY_PGM_REG_RGMII_100M_DATA    0
#define GMAC_DLY_PGM_REG_RGMII_100M_GMAC1_DATA    0
#define GMAC_DLY_PGM_REG_RGMII_1000M_DATA   0
#define GMAC_DLY_PGM_REG_RGMII_1000M_GMAC1_DATA   0
#define GMAC_DLY_PGM_REG_RMII_10M_DATA      0
#define GMAC_DLY_PGM_REG_RMII_10M_GMAC1_DATA      0
#define GMAC_DLY_PGM_REG_RMII_100M_DATA     0
#define GMAC_DLY_PGM_REG_RMII_100M_GMAC1_DATA     0

/**** Reversed ****************************
*/
#define GMAC_DLY_PGM_REG_MII_10M_R_DATA     0//TODO
#define GMAC_DLY_PGM_REG_MII_10M_R_GMAC1_DATA     0/*not legal interface*/
#define GMAC_DLY_PGM_REG_MII_100M_R_DATA    0//TODO
#define GMAC_DLY_PGM_REG_MII_100M_R_GMAC1_DATA    0/*not legal interface*/
#define GMAC_DLY_PGM_REG_GMII_1000M_R_DATA  0//TODO
#define GMAC_DLY_PGM_REG_GMII_1000M_R_GMAC1_DATA  0/*not legal interface*/
#define GMAC_DLY_PGM_REG_RGMII_10M_R_DATA   0//TODO
#define GMAC_DLY_PGM_REG_RGMII_10M_R_GMAC1_DATA   0/*not legal interface*/
#define GMAC_DLY_PGM_REG_RGMII_100M_R_DATA  0//TODO
#define GMAC_DLY_PGM_REG_RGMII_100M_R_GMAC1_DATA  0/*not legal interface*/
#define GMAC_DLY_PGM_REG_RGMII_1000M_R_DATA 0//TODO
#define GMAC_DLY_PGM_REG_RGMII_1000M_R_GMAC1_DATA 0/*not legal interface*/
#define GMAC_DLY_PGM_REG_RMII_10M_R_DATA    0//TODO
#define GMAC_DLY_PGM_REG_RMII_10M_R_GMAC1_DATA    0/*not legal interface*/
#define GMAC_DLY_PGM_REG_RMII_100M_R_DATA   0//TODO
#define GMAC_DLY_PGM_REG_RMII_100M_R_GMAC1_DATA   0/*not legal interface*/



enum interfaceMode
{
    INTERFACE_MII_GMII = 0,    
    INTERFACE_RGMII = 1,    
    INTERFACE_RMII = 2,    
    INTERFACE_MII_GMII_R = 3,    
    INTERFACE_RGMII_R = 4,    
    INTERFACE_RMII_R = 5,    
};

enum phy_speed
{
    SPEED_10M = 0,    
    SPEED_100M = 1,    
    SPEED_1000M = 2,    
};

enum mac_reversed
{
    MAC_NORMAL = 0,    
    MAC_REVERSED = 1,    
};

enum mac_index
{
    INDEX_GMAC0 = 0,    
    INDEX_GMAC1 = 1,    
    INDEX_GMACS_BOTH = 2,
};

/* TO DELETE these definitions, add them to config mechanism !!!!!!!!!!!!!!!!!!!!!!
*/
/*
#define CONFIG_MAC_MODE_REVERSED MAC_NORMAL
#define CONFIG_VBG400_MAC_INTERFACE_MODE INTERFACE_MII
#define CONFIG_VBG400_MAC_GMAC_INDEX INDEX_GMAC0
*/
#define CONFIG_MAC_MODE_REVERSED MAC_NORMAL
#define CONFIG_VBG400_MAC_GMAC_INDEX_TEMP INDEX_GMACS_BOTH

//#define VBG400_USE_SMA_SELECT


#endif
#endif __SHARED_H__
