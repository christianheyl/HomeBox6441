/**\file
 *  This file serves as the wrapper for the platform/OS dependent functions
 *  It is needed to modify these functions accordingly based on the platform and the
 *  OS. Whenever the synopsys GMAC driver ported on to different platform, this file
 *  should be handled at most care.
 *  The corresponding function definitions for non-inline functions are available in 
 *  synopGMAC_plat.c file.
 * \internal
 * -------------------------------------REVISION HISTORY---------------------------
 * Synopsys 				01/Aug/2007		 	   Created
 */
 
 
#ifndef SYNOP_GMAC_PLAT_H
#define SYNOP_GMAC_PLAT_H 1


#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/pci.h>

/*TO CHANGE: !!! */
//#define CONFIG_ARCH_NPU

#define TR0(fmt, args...) printk(KERN_CRIT "SynopGMAC: " fmt, ##args)				

#ifdef DEBUG
#undef TR
#  define TR(fmt, args...) printk(KERN_CRIT "SynopGMAC: " fmt, ##args)
#else
# define TR(fmt, args...) /* not debugging: nothing */
#endif

#if 0
typedef int bool;
enum synopGMAC_boolean
 { 
    false = 0,
    true = 1 
 };
#endif

//#define CONFIG_AHB_INTERFACE

#define DEFAULT_DELAY_VARIABLE  10 /*origin*/
//#define DEFAULT_DELAY_VARIABLE  1000
#define DEFAULT_LOOP_VARIABLE   10000
//#define DEFAULT_LOOP_VARIABLE   100000


/* There are platform related endian conversions
 *
 */

#define LE32_TO_CPU	__le32_to_cpu
#define BE32_TO_CPU	__be32_to_cpu
#define CPU_TO_LE32	__cpu_to_le32

/* Error Codes */
#define ESYNOPGMACNOERR   0
#define ESYNOPGMACNOMEM   1
#define ESYNOPGMACPHYERR  2
#define ESYNOPGMACBUSY    3

struct Network_interface_data
{
	u32 unit;
	u32 addr;
	u32 data;
};

#ifdef CONFIG_AHB_INTERFACE
#define device_struct platform_device
#else
#define device_struct pci_dev
#endif
/*
#define DEVICE_REG_ADDR_MAC1_START  0xA7040000
#define DEVICE_REG_ADDR_MAC2_START  0xA7180000
#define DEVICE_REG_SIZE        010000
#define DEVICE_REG_ADDR_MAC1_END    DEVICE_REG_ADDR_MAC1_START + DEVICE_REG_SIZE -1
#define DEVICE_REG_ADDR_MAC2_END    DEVICE_REG_ADDR_MAC2_START + DEVICE_REG_SIZE -1
*/

#ifdef CONFIG_MIPS_UNCACHED
#define readl(addr) \
              (((*(volatile unsigned int *)(addr))))
#define writel(b,addr) \
              ((*(volatile unsigned int *)(addr)) = ((b)))
#endif

#define WAVE400_USE_WR_DELAY
//#define WAVE400_TEST_DMA

/**
  * These are the wrapper function prototypes for OS/platform related routines
  */ 

void * plat_alloc_memory(u32 );
void   plat_free_memory(void *);
void   plat_free_irq(struct net_device *netdev);

#ifdef CONFIG_AHB_INTERFACE
//extern int irqOut[];

#if 0
unsigned char plat_get_irq(struct device_struct *, int );
#endif
dma_addr_t plat_map_single(struct device_struct *, u32 *, u32 , u32 ); 
void *plat_alloc_consistent_dmaable_memory(struct device_struct */* pcidev*/, u32/*  size*/, u32 */*  addr*/, u32/* direction*/, void ** /*DescFree*/);
void  plat_free_consistent_dmaable_memory (struct device_struct */* pcidev*/, u32, void *, u32, u32/* direction*/);
void  plat_unmap_single(struct device_struct */* *pcidev*/,u32 dma_addr, u32 size, u32 direction); 
#else
dma_addr_t plat_map_single(struct pci_dev *, u32 *, u32 , u32 ); 
void *plat_alloc_consistent_dmaable_memory(struct pci_dev *, u32, u32 *, u32, void **/*DescFree*/);
void  plat_free_consistent_dmaable_memory (struct pci_dev *, u32, void *, u32, u32/* direction*/);
void  plat_unmap_single(struct pci_dev *pcidev,u32 dma_addr, u32 size, u32 direction); 
#endif

void   plat_delay(u32);


/**
 * The Low level function to read register contents from Hardware.
 * 
 * @param[in] pointer to the base of register map  
 * @param[in] Offset from the base
 * \return  Returns the register contents 
 */
// /*static*/ u32 /*__inline__*/ synopGMACReadReg(u32 *RegBase, u32 RegOffset)
static u32 __inline__ synopGMACReadReg(u32 *RegBase, u32 RegOffset)
{

  u32 addr = (u32)RegBase + RegOffset;
  u32 data = readl((void *)addr);
  //TR("%s RegBase = 0x%08x RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, (u32)RegBase, RegOffset, data );
  return data;

}

//#define WAVE400_DUMP_REGS

#ifdef WAVE400_DUMP_REGS
static u32 __inline__ synopGMACDumpReg(u32 *RegBase, u32 RegOffset)
{
  u32 addr = (u32)RegBase + RegOffset;
  u32 data = readl((void *)addr);
  TR0(" 0x%08x\n",data );
  return data;
}
#endif

/**
 * The Low level function to write to a register in Hardware.
 * 
 * @param[in] pointer to the base of register map  
 * @param[in] Offset from the base
 * @param[in] Data to be written 
 * \return  void 
 */
static void  __inline__ synopGMACWriteReg(u32 *RegBase, u32 RegOffset, u32 RegData)
{

  u32 addr = (u32)RegBase + RegOffset;
//  TR("%s RegBase = 0x%08x RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__,(u32) RegBase, RegOffset, RegData );
#ifdef WAVE400_USE_WR_DELAY
plat_delay(DEFAULT_DELAY_VARIABLE);	
#endif
  writel(RegData,(void *)addr);
  return;
}

/**
 * The Low level function to set bits of a register in Hardware.
 * 
 * @param[in] pointer to the base of register map  
 * @param[in] Offset from the base
 * @param[in] Bit mask to set bits to logical 1 
 * \return  void 
 */
static void __inline__ synopGMACSetBits(u32 *RegBase, u32 RegOffset, u32 BitPos)
{
  u32 addr = (u32)RegBase + RegOffset;
  u32 data = readl((void *)addr);
  data |= BitPos; 
#ifdef WAVE400_USE_WR_DELAY
plat_delay(DEFAULT_DELAY_VARIABLE);	
#endif
//  TR("%s !!!!!!!!!!!!! RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data );
  writel(data,(void *)addr);
//  TR("%s !!!!!!!!!!!!! RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data );
  return;
}


/**
 * The Low level function to clear bits of a register in Hardware.
 * 
 * @param[in] pointer to the base of register map  
 * @param[in] Offset from the base
 * @param[in] Bit mask to clear bits to logical 0 
 * \return  void 
 */
static void __inline__ synopGMACClearBits(u32 *RegBase, u32 RegOffset, u32 BitPos)
{
  u32 addr = (u32)RegBase + RegOffset;
  u32 data = readl((void *)addr);
  data &= (~BitPos); 
#ifdef WAVE400_USE_WR_DELAY
plat_delay(DEFAULT_DELAY_VARIABLE);	
#endif
//  TR("%s !!!!!!!!!!!!!! RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data );
  writel(data,(void *)addr);
//  TR("%s !!!!!!!!!!!!! RegOffset = 0x%08x RegData = 0x%08x\n", __FUNCTION__, RegOffset, data );
  return;
}

/**
 * The Low level function to Check the setting of the bits.
 * 
 * @param[in] pointer to the base of register map  
 * @param[in] Offset from the base
 * @param[in] Bit mask to set bits to logical 1 
 * \return  returns TRUE if set to '1' returns FALSE if set to '0'. Result undefined there are no bit set in the BitPos argument.
 * 
 */
static bool __inline__ synopGMACCheckBits(u32 *RegBase, u32 RegOffset, u32 BitPos)
{
  u32 addr = (u32)RegBase + RegOffset;
  u32 data = readl((void *)addr);
  data &= BitPos; 
  if(data)  return true;
  else	    return false;

}


#endif
