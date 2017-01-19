#ifndef IFXMIPS_PCIE_EP_VRX318_TEST_H
#define IFXMIPS_PCIE_EP_VRX318_TEST_H
#include <linux/types.h>

/* PPE interrupt */
#define PPE_MBOX_TEST_BIT     0x1
#define PPE_MBOX_IRQ_TEST_NUM 10000

#define PPE_MBOX_OFFSET       0x200000
#define PEE_MBOX_ATU(X)       (((X) - 0x7000 + 0xd000) << 2)

#define PPE_MBOX_IGU0_ISRS(__mem_base)   ((__mem_base) + PPE_MBOX_OFFSET + PEE_MBOX_ATU(0x7200))
#define PPE_MBOX_IGU0_ISRC(__mem_base)   ((__mem_base) + PPE_MBOX_OFFSET + PEE_MBOX_ATU(0X7201))
#define PPE_MBOX_IGU0_ISR(__mem_base)    ((__mem_base) + PPE_MBOX_OFFSET + PEE_MBOX_ATU(0X7202))
#define PPE_MBOX_IGU0_IER(__mem_base)    ((__mem_base) + PPE_MBOX_OFFSET + PEE_MBOX_ATU(0X7203))
#define PPE_MBOX_IGU1_ISRS(__mem_base)   ((__mem_base) + PPE_MBOX_OFFSET + PEE_MBOX_ATU(0X7204))
#define PPE_MBOX_IGU1_ISRC(__mem_base)   ((__mem_base) + PPE_MBOX_OFFSET + PEE_MBOX_ATU(0X7205))
#define PPE_MBOX_IGU1_ISR(__mem_base)    ((__mem_base) + PPE_MBOX_OFFSET + PEE_MBOX_ATU(0X7206))
#define PPE_MBOX_IGU1_IER(__mem_base)    ((__mem_base) + PPE_MBOX_OFFSET + PEE_MBOX_ATU(0X7207))
#define PPE_MBOX_IGU2_ISRS(__mem_base)   ((__mem_base) + PPE_MBOX_OFFSET + PEE_MBOX_ATU(0X7210))
#define PPE_MBOX_IGU2_ISRC(__mem_base)   ((__mem_base) + PPE_MBOX_OFFSET + PEE_MBOX_ATU(0X7211))
#define PPE_MBOX_IGU2_ISR(__mem_base)    ((__mem_base) + PPE_MBOX_OFFSET + PEE_MBOX_ATU(0X7212))
#define PPE_MBOX_IGU2_IER(__mem_base)    ((__mem_base) + PPE_MBOX_OFFSET + PEE_MBOX_ATU(0X7213))

/* Central DMA */

/* Inbound address translation for iATU0 */
#define PCIE_EP_INBOUND_INTERNAL_BASE          0x1E000000
#define PCIE_EP_OUTBOUND_INTERNAL_BASE         0x20000000
#define PCIE_EP_OUTBOUND_MEMSIZE               0x80000000

#define VRX218_MASK_ADDR(X)  (0x00FFFFFF & (X))
#define VRX218_ADDR(X)       ((0x00FFFFFF & (X)) | 0x1e000000)

/* VRX218 internal address */
#define VRX218_PDRAM_BASE    0x1e080000
#define PPE_SB_RAM_BLOCK0    (0x1e200000 + (0x8000 << 2))
#define PPE_SB_RAM_BLOCK1    (0x1e200000 + (0x9000 << 2))
#define PPE_SB_RAM_BLOCK2    (0x1e200000 + (0xa000 << 2))
#define PPE_SB_RAM_BLOCK3    (0x1e200000 + (0xb000 << 2))

#define LOCAL_DRAMBASE  0xa0d00000

#define DATA_DDR /* SoC data in DDR instead of SRAM */

#ifdef DATA_DDR
#define LOCAL_TX1_DATA_LOC  (LOCAL_DRAMBASE + 0x00000) 
#define LOCAL_RX1_DATA_LOC  (LOCAL_DRAMBASE + 0x10000) /* 64K */
#else
#define LOCAL_TX1_DATA_LOC  (0xBF107400) 
#define LOCAL_RX1_DATA_LOC  (0xBF107400) /* 16K */
#endif

/* Special test case for bonding */
#define BONDING_TX1_DATA_LOC  (VRX218_PDRAM_BASE + 0x0000) 
#define BONDING_RX1_DATA_LOC  (VRX218_PDRAM_BASE + 0x8000) /* 64K */

#define DESC_SB /* Descriptor in VRX218 PPE SB, instead of PDRAM */
//#define DESC_DATA_SB /* Descriptor/ Data in VRX218 PPE SB */

#ifdef DESC_SB
#define REMOTE_TX1_DATA_LOC    VRX218_PDRAM_BASE
#define REMOTE_RX1_DATA_LOC    VRX218_PDRAM_BASE + 0xc000
#define VRX218_TX_DESC         PPE_SB_RAM_BLOCK0
#define VRX218_RX_DESC         PPE_SB_RAM_BLOCK1
#elif defined (DESC_DATA_SB)
#define REMOTE_TX1_DATA_LOC    PPE_SB_RAM_BLOCK0
#define REMOTE_RX1_DATA_LOC    PPE_SB_RAM_BLOCK1
#define VRX218_TX_DESC         PPE_SB_RAM_BLOCK2
#define VRX218_RX_DESC         PPE_SB_RAM_BLOCK2 + 0x800
#else
#define REMOTE_TX1_DATA_LOC    VRX218_PDRAM_BASE
#define REMOTE_RX1_DATA_LOC    VRX218_PDRAM_BASE + 0x8000
#define VRX218_TX_DESC         VRX218_PDRAM_BASE + 0x10000
#define VRX218_RX_DESC         VRX218_PDRAM_BASE + 0x11000
#endif

#define VRX218_CDMA_OFFSET                0x00104100
#define CDMA_CLC(__membase)                   ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x0000))
#define CDMA_ID(__membase)                    ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x0008))
#define CDMA_CTRL(__membase)                  ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x0010))

#define CDMA_PS(__membase)                    ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x0040))
#define CDMA_PCTRL(__membase)                 ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x0044))
#define CDMA_IRNEN(__membase)                 ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x00F4))
#define CDMA_IRNCR(__membase)                 ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x00F8))
#define CDMA_IRNICR(__membase)                ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x00FC))

#define CDMA_CS(__membase)                    ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x0018))
#define CDMA_CCTRL(__membase)                 ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x001C))
#define CDMA_CDBA(__membase)                  ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x0020))
#define CDMA_CGBL(__membase)                  ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x0030))
#define CDMA_CDPTNRD(__membase)               ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x0034))
#define CDMA_CDPTNRD1(__membase)              ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x0038))
#define CDMA_CIE(__membase)                   ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x002C))
#define CDMA_CIS(__membase)                   ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x0028))
#define CDMA_CDLEN(__membase)                 ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x0024))
#define CDMA_CPOLL(__membase)                 ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x0014))
#define CDMA_CPDCNT(__membase)                ((volatile u32*)((__membase) + VRX218_CDMA_OFFSET + 0x0080))

#define CDMA_MEMCOPY_PORT         0

#define CDMA_MEMCOPY_RX_CHAN      0
#define CDMA_MEMCOPY_TX_CHAN      1


/** End of packet interrupt */
#define DMA_CIS_EOP  	 				0x00000002	
/** Descriptor Under-Run Interrupt  */
#define DMA_CIS_DUR 					0x00000004	
/** Descriptor Complete Interrupt  */
#define DMA_CIS_DESCPT 					0x00000008	
/** Channel Off Interrupt  */
#define DMA_CIS_CHOFF   				0x00000010	
/** SAI Read Error Interrupt */
#define DMA_CIS_RDERR 					0x00000020	
/** all interrupts */
#define DMA_CIS_ALL     				( DMA_CIS_EOP		\
										| DMA_CIS_DUR 		\
										| DMA_CIS_DESCPT 	\
										| DMA_CIS_CHOFF 	\
										| DMA_CIS_RDERR	)

/** End of packet interrupt enable */
#define DMA_CIE_EOP 	 	 			0x00000002	
/** Descriptor Under-Run Interrupt enable  */
#define DMA_CIE_DUR                     0x00000004	
/** Descriptor Complete Interrupt  enable*/
#define DMA_CIE_DESCPT 					0x00000008
/** Channel Off Interrupt enable */
#define DMA_CIE_CHOFF   				0x00000010	
/** SAI Read Error Interrupt enable*/
#define DMA_CIE_RDERR 					0x00000020


#define DMA_CIE_ALL                     (DMA_CIE_EOP 		\
										| DMA_CIE_DUR 		\
										| DMA_CIE_DESCPT 	\
										| DMA_CIE_CHOFF		\
										| DMA_CIE_RDERR	)

/** default enabled interrupts */
#define DMA_CIE_DEFAULT                     ( DMA_CIE_DESCPT    \
                                            | DMA_CIE_EOP )
/** disable all interrupts */
#define DMA_CIE_DISABLE_ALL                 0 

typedef struct
{
    union {
        struct {
            volatile u32 OWN                 :1;
            volatile u32 C                   :1;
            volatile u32 Sop                 :1;
            volatile u32 Eop                 :1;
            volatile u32 reserved            :3;
            volatile u32 Byteoffset          :2;
            volatile u32 rx_sideband         :4;
            volatile u32 reserve             :3;
            volatile u32 DataLen             :16;
        }field;

        volatile u32 word;
    }status;

    volatile u32 DataPtr;
} cdma_rx_descriptor_t;

typedef struct
{
    union {
        struct {
            volatile u32 OWN                 :1;
            volatile u32 C                   :1;
            volatile u32 Sop                 :1;
            volatile u32 Eop                 :1;
            volatile u32 Byteoffset          :5;
            volatile u32 reserved            :7;
            volatile u32 DataLen             :16;
        }field;

        volatile u32 word;
    }status;

    volatile u32 DataPtr;
} cdma_tx_descriptor_t;

enum {
    SOC_TO_EP = 0,
    EP_TO_SOC,
};

/* ICU */
#define VRX218_ICU_OFFSET 0x00000000

#define ICU_IM_SR(__membase)    ((volatile u32*)((__membase) + VRX218_ICU_OFFSET + 0x0040))
#define ICU_IM_ER(__membase)    ((volatile u32*)((__membase) + VRX218_ICU_OFFSET + 0x0044))
#define ICU_IM_OSR(__membase)   ((volatile u32*)((__membase) + VRX218_ICU_OFFSET + 0x0048))


#define PPE2HOST_INT_0      0
#define PPE2HOST_INT_1      1
#define DSL_DYING_GASP      3

#define DSL_MEI_IRQ         8
#define EDMA_INT            9
#define FPI_BCU_INT         12
#define ARC_LED0            13
#define ARC_LED1            14
#define CDMA_CH0            16
#define CDMA_CH1            17
#define CDMA_CH2            18
#define CDMA_CH3            19
#define CDMA_CH4            20
#define CDMA_CH5            21
#define CDMA_CH6            22
#define CDMA_CH7            23

#define VRX218_CGU_OFFSET 0x00003000

#define PMU_PWDCR(__membase)    ((volatile u32*)((__membase) + VRX218_CGU_OFFSET + 0x011C))
#define PMU_SR(__membase)       ((volatile u32*)((__membase) + VRX218_CGU_OFFSET + 0x0120))
#define CGU_CLKFSR(__membase)   ((volatile u32*)((__membase) + VRX218_CGU_OFFSET + 0x0010))
#define CGU_CLKGSR(__membase)   ((volatile u32*)((__membase) + VRX218_CGU_OFFSET + 0x0014))
#define CGU_CLKGCR0(__membase)  ((volatile u32*)((__membase) + VRX218_CGU_OFFSET + 0x0018))
#define CGU_CLKGCR1(__membase)  ((volatile u32*)((__membase) + VRX218_CGU_OFFSET + 0x001C))
#define CGU_IF_CLK(__membase)   ((volatile u32*)((__membase) + VRX218_CGU_OFFSET + 0x0024))
#define CGU_PLL_CFG(__membase)  ((volatile u32*)((__membase) + VRX218_CGU_OFFSET + 0x0060))

#define CGU_DMA_CLK_EN      0x00000004

#define VRX218_RCU_OFFSET 0x00002000

#define RCU_RST_REQ(__membase)    ((volatile u32*)((__membase) + VRX218_RCU_OFFSET + 0x0010))
#define RCU_RST_STAT(__membase)   ((volatile u32*)((__membase) + VRX218_RCU_OFFSET + 0x0014))

#define RCU_RST_REQ_DMA          (1 << 9)

#define RCU_AHB_ENDIAN(__membase) ((volatile u32*)((__membase) + VRX218_RCU_OFFSET + 0x004C))

/* Endian control bit for enable or pin strapping */
#define VRX218_XBAR_AHB_PCIEM_EN   0x00010000
#define VRX218_XBAR_AHBM_PCIE_EN   0x00020000
#define VRX218_XBAR_AHBS_DSL_EN    0x00040000
#define VRX218_XBAR_AHBS_PCIE_EN   0x00080000
#define VRX218_XBAR_AHB_PCIES_EN   0x00100000
#define VRX218_XBAR_AHB_DBI_EN     0x00200000

#define VRX218_XBAR_AHB_PCI_EN_ALL (VRX218_XBAR_AHB_PCIEM_EN | VRX218_XBAR_AHBM_PCIE_EN | \
                                    VRX218_XBAR_AHBS_PCIE_EN |  VRX218_XBAR_AHB_PCIES_EN | \
                                    VRX218_XBAR_AHB_DBI_EN | VRX218_XBAR_AHBS_DSL_EN)

#define VRX218_XBAR_AHB_PCIEM   0x00000001
#define VRX218_XBAR_AHBM_PCIE   0x00000002
#define VRX218_XBAR_AHBS_DSL    0x00000004
#define VRX218_XBAR_AHBS_PCIE   0x00000008
#define VRX218_XBAR_AHB_PCIES   0x00000010
#define VRX218_XBAR_AHB_DBI     0x00000020

enum {
    PPE_TEST = 0,
    CDMA_TEST,
};

#endif /* IFXMIPS_PCIE_EP_VRX318_TEST_H */
