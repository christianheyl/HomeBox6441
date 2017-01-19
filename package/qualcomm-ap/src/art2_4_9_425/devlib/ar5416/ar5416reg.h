/*
 *  Copyright (c) 2005 Atheros Communications, Inc., All Rights Reserved
 */

/* ar5416reg.h Register definitions for Atheros AR5416 chipset */
#ifndef	_AR5416REG_H
#define	_AR5416REG_H

// "ACI $Header: //depot/sw/branches/art2_main_per_cs/src/art2/devlib/ar5416/ar5416reg.h#1 $"

#define IS_MAC_5416_2_0_UP(x)      ((((x) & 0x7) >= 1)||( ((x)&0xfff)==0x140))     /* For Owl 2.0,Howl */

// PHY registers
#define PHY_BASE_CHAIN0            0x9800  // PHY registers base address for chain0
#define PHY_BASE_CHAIN1            0xa800  // PHY registers base address for chain0
#define PHY_BASE_CHAIN_BOTH        0xb800  // PHY registers base address for chain0

// MAC PCU Registers
#define F2_STA_ID0          0x8000  // MAC station ID0 register - low 32 bits
#define F2_STA_ID1          0x8004  // MAC station ID1 register - upper 16 bits
#define F2_STA_ID1_SADH_MASK   0x0000FFFF // Mask for upper 16 bits of MAC addr
#define F2_STA_ID1_STA_AP      0x00010000 // Device is AP
#define F2_STA_ID1_AD_HOC      0x00020000 // Device is ad-hoc
#define F2_STA_ID1_PWR_SAV     0x00040000 // Power save reporting in self-generated frames
#define F2_STA_ID1_KSRCHDIS    0x00080000 // Key search disable
#define F2_STA_ID1_PCF		   0x00100000 // Observe PCF
#define F2_STA_ID1_USE_DEFANT  0x00200000 // Use default antenna
#define F2_STA_ID1_DEFANT_UPDATE 0x00400000 // Update default antenna w/ TX antenna
#define F2_STA_ID1_RTS_USE_DEF   0x00800000 // Use default antenna to send RTS
#define F2_STA_ID1_ACKCTS_6MB  0x01000000 // Use 6Mb/s rate for ACK & CTS


#define F2_DEF_ANT			0x8058 //default antenna register

#define F2_RXDP             0x000C  // MAC receive queue descriptor pointer

#define F2_QCU_0		  0x0001

#define F2_IMR_S0             0x00a4 // MAC Secondary interrupt mask register 0
//#define F2_IMR_S0_QCU_TXOK_M    0x0000FFFF // Mask for TXOK (QCU 0-15)
#define F2_IMR_S0_QCU_TXDESC_M  0xFFFF0000 // Mask for TXDESC (QCU 0-15)
#define F2_IMR_S0_QCU_TXDESC_S  16		   // Shift for TXDESC (QCU 0-15)

#define F2_IMR               0x00a0  // MAC Primary interrupt mask register
#define F2_IMR_TXDESC        0x00000080 // Transmit interrupt request

// Interrupt status registers (read-and-clear access, secondary shadow copies)
#define F2_ISR_RAC           0x00c0 // MAC Primary interrupt status register,
#define F2_ISR               0x0080 // MAC Primary interrupt status register,

#define F2_IER               0x0024  // MAC Interrupt enable register
#define F2_IER_ENABLE        0x00000001 // Global interrupt enable
#define F2_IER_DISABLE       0x00000000 // Global interrupt disable

#define F2_Q0_TXDP           0x0800 // MAC Transmit Queue descriptor pointer

#define F2_RX_FILTER         0x803C  // MAC receive filter register
#define F2_RX_FILTER_ALL     0x00000000 // Disallow all frames
#define F2_RX_UCAST          0x00000001 // Allow unicast frames
#define F2_RX_MCAST          0x00000002 // Allow multicast frames
#define F2_RX_BCAST          0x00000004 // Allow broadcast frames
#define F2_RX_CONTROL        0x00000008 // Allow control frames
#define F2_RX_BEACON         0x00000010 // Allow beacon frames
#define F2_RX_PROM           0x00000020 // Promiscuous mode, all packets

#define F2_DIAG_SW           0x8048  // MAC PCU control register
#define F2_DIAG_RX_DIS       0x00000020 // disable receive
#define F2_DIAG_CHAN_INFO    0x00000100 // dump channel info
#define F2_DUAL_CHAIN_CHAN_INFO    0x01000000 // dump channel info

#define F2_CR                0x0008  // MAC Control Register - only write values of 1 have effect
#define F2_CR_RXE            0x00000004 // Receive enable
#define F2_CR_RXD            0x00000020 // Receive disable
#define F2_CR_SWI            0x00000040 // One-shot software interrupt

#define F2_Q_TXE             0x0840 // MAC Transmit Queue enable
#define F2_Q_TXE_M			  0x0000FFFF // Mask for TXE (QCU 0-15)

#define F2_QDCKLGATE         0x005c // MAC QCU/DCU clock gating control register
#define F2_QDCKLGATE_QCU_M    0x0000FFFF // Mask for QCU clock disable 
#define F2_QDCKLGATE_DCU_M    0x07FF0000 // Mask for DCU clock disable 

#define F2_DIAG_ENCRYPT_DIS  0x00000008 // disable encryption

#define F2_D0_QCUMASK     0x1000 // MAC QCU Mask

#define F2_D0_LCL_IFS     0x1040 // MAC DCU-specific IFS settings
#define F2_D_LCL_IFS_AIFS_M		   0x0FF00000 // Mask for AIFS
#define F2_D_LCL_IFS_AIFS_S		   20         // Shift for AIFS

#define F2_D_LCL_IFS_CWMIN_M	   0x000003FF // Mask for CW_MIN
#define F2_D_LCL_IFS_CWMIN_S	   0 // shift for CW_MIN

#define F2_D_LCL_IFS_CWMAX_M	   0x000FFC00 // Mask for CW_MAX
#define F2_D_LCL_IFS_CWMAX_S	   10 // shift for CW_MAX

#define F2_D0_RETRY_LIMIT	  0x1080 // MAC Retry limits
#define F2_Q0_MISC         0x09c0 // MAC Miscellaneous QCU settings

#define F2_D_GBL_IFS_SIFS	0x1030 // MAC DCU-global IFS settings: SIFS duration
#define F2_D_GBL_IFS_EIFS	0x10b0 // MAC DCU-global IFS settings: EIFS duration

#define F2_D_FPCTL			0x1230		//Frame prefetch

#define F2_SREV             0x4020  // Silicon Revision register
#define PHY_CHIP_ID         0x9818  // PHY chip revision ID

#define F2_TIME_OUT         0x8014  // MAC ACK & CTS time-out

#define F2_Q_TXD             0x0880 // MAC Transmit Queue disable

#define F2_KEY_CACHE_START          0x8800  // keycache start address

#define F2_IMR_RXDESC        0x00000002 // Receive interrupt request

#define F2_RPGTO            0x0050  // MAC receive frame gap timeout
#define F2_RPGTO_MASK        0x000003FF // Mask for receive frame gap timeout

#define F2_RPCNT            0x0054 // MAC receive frame count limit
#define F2_RPCNT_MASK        0x0000001F // Mask for receive frame count limit

#define PHY_FRAME_CONTROL1   0x9944  // rest of old PHY frame control register
#define PHY_CHAN_INFO        0x99DC  // 5416 CHAN_INFO register
#define PHY_CCA              0xC864
#define PHY_CCA_CH0          0x9864
#define PHY_CCA_CH1          0xa864
#define PHY_CCA_CH2          0xb864
#define PHY_CCA_CH0_EXT      0x99bc
#define PHY_CCA_CH1_EXT      0xa9bc
#define PHY_CCA_CH2_EXT      0xb9bc
#define PHY_CCA_THRESH62_M   0x0007F000
#define PHY_CCA_THRESH62_S   12

#define PHY_TIMING4          0x

#define TPCRG1_REG 0xa258
#define TPCRG5_REG 0xa26c
#define BB_PD_GAIN_OVERLAP_MASK       0x0000000f

#endif /* _AR5416REG_H */

