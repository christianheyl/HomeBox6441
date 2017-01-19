/****************************************************************************
                               Copyright  2009
                            Infineon Technologies AG
                     Am Campeon 1-12; 81726 Munich, Germany
  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************

   \file ifxmips_vr9_gphy.c
   \remarks implement GPHY driver on ltq platform
 *****************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/moduleparam.h>
#include <linux/types.h>  /* size_t */
#include <linux/netdevice.h>   /* struct device, and other headers */
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/if_ether.h>
#include <linux/etherdevice.h> /* eth_type_trans */
#include <asm/delay.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <asm/mipsregs.h>

#include <switch_api/ifx_ethsw_kernel_api.h>
#include <switch_api/ifx_ethsw_api.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>
#include <asm/ifx/ifx_pmu.h>
#include <asm/ifx/ifx_rcu.h>
#include <asm/ifx/ifx_pmu.h>
#include <asm/ifx/ifx_gpio.h>
#include <ltq_xrx_platform.h>
/* Enable/Disable FW down from the user space app using the /proc filesystem *
** and select from the kernel configuration */
 /* #define CONFIG_FW_LOAD_FROM_USER_SPACE		1  */
/* Enable/Disable Nibble Alignment workaround for PHY's v1.3/v1.4 from the Kernel menuconfiguration*/
//#define CONFIG_GPHY_NIB_ALIN_WORKAROUND	1
/* Enable/Disable interrupt handling to detect link status from the Kernel menuconfiguration*/
#ifdef CONFIG_GPHY_INT_SUPPORT
	#define CONFIG_GPHY_INT_SUPPORT	1
	#define INT_TEST_CODE 0
#endif

#ifndef CONFIG_FW_LOAD_FROM_USER_SPACE
#if defined(CONFIG_GE_MODE)
	#include "gphy_fw_ge.h"
#endif 

#if defined(CONFIG_FE_MODE)
	#include "gphy_fw_fe.h"
#endif

#if defined(CONFIG_GEFE_MIXED_MODE)
	#include "gphy_fw_ge.h"
	#include "gphy_fw_fe.h"
#endif

#endif /* CONFIG_FW_LOAD_FROM_USER_SPACE */

#if  defined(CONFIG_AR10) 
#include <asm/ifx/ar10/ar10.h>
#endif

#if defined(CONFIG_GE_MODE)
	#define IFX_GPHY_MODE                   "GE Mode"
#endif /*CONFIG_GE_MODE*/

#if defined(CONFIG_FE_MODE)
	#define IFX_GPHY_MODE                   "FE Mode"
#endif /*CONFIG_FE_MODE*/


#if defined(CONFIG_GEFE_MIXED_MODE)
	#define IFX_GPHY_MODE                   "GE/FE Mixed Mode"
#endif /*CONFIG_FE_MODE*/

#ifndef CONFIG_GEFE_MIXED_MODE
#define	FW_GE_MODE					0
#define	FW_FE_MODE					1
#else
#define	FW_GE_MODE					1
#define	FW_FE_MODE					1
#endif
#define IFX_DRV_MODULE_NAME             "ifxmips_xrx_gphy"
#define IFX_DRV_MODULE_VERSION          "1.1.1"

static char version[]   __devinitdata = IFX_DRV_MODULE_NAME ": V" IFX_DRV_MODULE_VERSION "";
#define GPHY_FW_LEN		(64 * 1024)
#define GPHY_FW_LEN_D	(128 * 1024)
#ifndef CONFIG_GEFE_MIXED_MODE
static unsigned char gphy_fw_dma[GPHY_FW_LEN_D];
#else
static unsigned char gphy_fw_fe_dma[GPHY_FW_LEN_D];
static unsigned char gphy_fw_ge_dma[GPHY_FW_LEN_D];
#endif

//GPHY register GFS_ADD0 requires 16Kb starting address alignment boundary
//     It was set to 64Kb, but 16Kb works fine
#define GPHY_FW_START_ADDR_ALIGN        0x4000 
#define GPHY_FW_START_ADDR_ALIGN_MASK   (GPHY_FW_START_ADDR_ALIGN-1)
#ifdef CONFIG_FW_LOAD_FROM_USER_SPACE
#ifndef CONFIG_GEFE_MIXED_MODE
static unsigned char proc_gphy_fw_dma[GPHY_FW_LEN_D];
#else
#error "Mixed mode not support CONFIG_FW_LOAD_FROM_USER_SPACE yet. This code need to be patched....."
#endif
#define	IH_MAGIC_NO	0x27051956	/* Image Magic Number		*/
#define IH_NMLEN		32	/* Image Name Length		*/
/*
 * Legacy format image header,
 * all data in network byte order (aka natural aka bigendian).
 */
typedef struct gphy_image_header {
	uint32_t	ih_magic;	/* Image Header Magic Number	*/
	uint32_t	ih_hcrc;	/* Image Header CRC Checksum	*/
	uint32_t	ih_time;	/* Image Creation Timestamp	*/
	uint32_t	ih_size;	/* Image Data Size		*/
	uint32_t	ih_load;	/* Data	 Load  Address		*/
	uint32_t	ih_ep;		/* Entry Point Address		*/
	uint32_t	ih_dcrc;	/* Image Data CRC Checksum	*/
	uint8_t		ih_os;		/* Operating System		*/
	uint8_t		ih_arch;	/* CPU architecture		*/
	uint8_t		ih_type;	/* Image Type			*/
	uint8_t		ih_comp;	/* Compression Type		*/
	uint8_t		ih_name[IH_NMLEN];	/* Image Name		*/
} gphy_image_header_t;
#endif /* CONFIG_FW_LOAD_FROM_USER_SPACE */

#if defined(CONFIG_GPHY_INT_SUPPORT) && CONFIG_GPHY_INT_SUPPORT

#if defined(INT_TEST_CODE) && INT_TEST_CODE
ltq_phy_link_status_t g_gphy_status1;
ltq_phy_link_status_t g_gphy_status2;
ltq_phy_link_status_t g_gphy_status4;
#endif
#define GPHY_IMASK		0x1
static irqreturn_t gphy1_link_int_handler(int, void *);
static irqreturn_t gphy2_link_int_handler(int, void *);
static irqreturn_t gphy4_link_int_handler(int, void *);
#if defined(CONFIG_FE_MODE)
static irqreturn_t gphy3_link_int_handler(int, void *);
static irqreturn_t gphy5_link_int_handler(int, void *);
#endif
#if defined(CONFIG_GEFE_MIXED_MODE)
#error "Mixed mode not support CONFIG_GPHY_INT_SUPPORT yet. This code need to be patched....."
#endif
#endif /* CONFIG_GPHY_INT_SUPPORT */
#ifndef CONFIG_GEFE_MIXED_MODE
int ltq_gphy_firmware_config(int total_gphys, int mode, int dev_id, int ge_fe_mode );
static struct proc_dir_entry*   ifx_gphy_dir=NULL;
static int dev_id, total_gphys,ge_fe_mode ; 
#else
int ltq_gphy_firmware_config(int total_gphys, int mode, int dev_id, int gphy_fe_mode, int gphy_ge_mode );
static struct proc_dir_entry*   ifx_gphy_dir=NULL;
static int dev_id, total_gphys;
static int gphy_fw_fe_mode, gphy_fw_ge_mode;
#endif
IFX_ETHSW_HANDLE swithc_api_fd;
#if  defined(CONFIG_HN1) 
	#define NUM_OF_PORTS 2
#else
	#define NUM_OF_PORTS 6
#endif
/* 3 --> GPHY v1.3 and 4 --> GPHY v1.4 */
/*static int gphy_version[NUM_OF_PORTS] = {0,0,0,0,0,0}; */
/* Store the PHY address */
static int PHY_Address[NUM_OF_PORTS] = {0,1,2,3,4,5};
/* 1 --> GPHY 1.3 and GPHY 1.4 , 0 --> GPHY 1.5 and above */
static int gphy_version_detect[NUM_OF_PORTS] = {0};

#ifdef CONFIG_GPHY_NIB_ALIN_WORKAROUND

#define FSM_DETECT_ERROR	1
#define ERR_COUNT	3
#define AN_ADV_10BASE	0x060
#define AN_ADV_DEFAULT	0x1E0
/* Global variables */
/* 1 --> 100Mbps and 0 -->10/1000Mbps */
static int gphy_link_status[NUM_OF_PORTS] = {0};
/* 1 --> polling is require, 0 --> polling is not required */
//static int gphy_TSTAT_poll[NUM_OF_PORTS] = {0};
/* 1--> Configure the MDIO register for 10Mbps, 0 --> default register setting */
static int gphy_link_speed[NUM_OF_PORTS] = {0};
/* Counter the TSTAT errors per port */
static int gphy_err_detect_cnt[NUM_OF_PORTS] = {0};
/* Counter the TSTAT errors per port */
static int gphy_ADV_default_reg_value[NUM_OF_PORTS] = {0};
#if  defined(FSM_DETECT_ERROR)  && FSM_DETECT_ERROR
/* 1 -- GPHY nibble fix, 0 -- GPHY nibble not fixed */
static int gphy_version_nibble_fix[NUM_OF_PORTS] = {0};
#endif /* FSM_DETECT_ERROR */
/* Thread Related */
/* -------- */
struct task_struct *gphy_link_thread_id;
static int gphy_link_detect_thread (void *arg);

/* Signal Related */
/* -------- */
wait_queue_head_t gphy_link_thread_wait;

static int thread_wind_counter = 0;
static unsigned long thread_interval = 3;
#endif /*CONFIG_GPHY_NIB_ALIN_WORKAROUND */
/*  define and declare a semaphore, named mdio_sem, with a count of one */
static DECLARE_MUTEX(mdio_sem);
//extern struct semaphore swapi_sem;
#ifdef CONFIG_AR10_FAMILY_BOARD_2
#define GPHY_LED_SIGNAL_IMPLEMENTATION
#elif defined(CONFIG_TARGET_MGL7016AW_22_ZZ_R0B)
#define GPHY_LED_SIGNAL_IMPLEMENTATION
#define GPHY_LED_GPIO	11
#elif 1 /* defined(CONFIG_TARGET_VRV9518SWAC33_B_ZZ) | defined(CONFIG_TARGET_VRV9518SWAC33_B_49) | defined(CONFIG_TARGET_VRV9518SWAC24_B_49) */
#define GPHY_LED_SIGNAL_IMPLEMENTATION
#define GPHY_LED_GPIO	2
#define GPHY_LED_USE_SERIAL_LED
#endif
#ifdef GPHY_LED_SIGNAL_IMPLEMENTATION

#include <asm/ifx/ifx_gpio.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#ifdef GPHY_LED_USE_SERIAL_LED
#include <asm/ifx/ifx_ledc.h>
#endif
enum gphy_gpio_mapping
{
	GPHY_0_GPIO = 0,
	GPHY_1_GPIO = 9,
	GPHY_2_GPIO = 6,
	GPHY_3_GPIO = 3,
	GPHY_4_GPIO = 4,
	GPHY_5_GPIO = 10,
};
#define SWITCH_STATUS_REG  0x01
#define LINK_ON		   0x04
#define SWITCH_LED_CON_REG 0x1B

#define LED_OFF		0
#define LED_ON		1
#define LED_FLASH	2

/* Thread Related */
/* -------- */
struct task_struct *gphy_led_thread_id;
struct task_struct *gphy_rmon_poll_thread_id;

/* Signal Related */
/* -------- */
wait_queue_head_t gphy_led_thread_wait;
int gphy_any_led_need_to_flash=0;
int gphy_led_state[NUM_OF_PORTS] = {0,0,0,0,0,0};	/* 0: OFF, 1: ON, 2: flash */
int gphy_led_status_on[NUM_OF_PORTS] = {0,0,0,0,0,0};
int gphy_led_control_on = 0;  /* 0: OFF, 1: ON */
#if defined(GPHY_LED_SIGNAL_IMPLEMENTATION)
int zz_gphy_led_state = 0;
int zz_gphy_led_status_on = 0;
#endif
#define WAIT_TIMEOUT 10
#endif /*GPHY_LED_SIGNAL_IMPLEMENTATION*/

int vr9_gphy_pmu_set(void)
{
	/* Config GPIO3 clock */
	SWITCH_PMU_SETUP(IFX_PMU_ENABLE);
	GPHY_PMU_SETUP(IFX_PMU_ENABLE);
	return 0;
}
void inline ifxmips_mdelay( unsigned int delay){

	msleep_interruptible(delay);
}
static void lq_ethsw_mdio_data_write(unsigned int phyAddr, unsigned int regAddr,unsigned int data )
{
	IFX_ETHSW_MDIO_data_t mdio_data;
	memset(&mdio_data, 0, sizeof(IFX_ETHSW_MDIO_data_t));
	mdio_data.nAddressDev = phyAddr;
	mdio_data.nAddressReg = regAddr;
	mdio_data.nData = data;
	ifx_ethsw_kioctl(swithc_api_fd, IFX_ETHSW_MDIO_DATA_WRITE, (unsigned int)&mdio_data);
    return ;
}

static unsigned short lq_ethsw_mdio_data_read(unsigned int phyAddr, unsigned int regAddr )
{
	IFX_ETHSW_MDIO_data_t mdio_data;
	memset(&mdio_data, 0, sizeof(IFX_ETHSW_MDIO_data_t));
	mdio_data.nAddressDev = phyAddr;
	mdio_data.nAddressReg = regAddr;
	ifx_ethsw_kioctl(swithc_api_fd, IFX_ETHSW_MDIO_DATA_READ, (unsigned int)&mdio_data);
    return (mdio_data.nData );
}

static void force_to_phy_settings(int port, int link_up )
{
	unsigned int mdio_stat_reg, phy_addr_reg=0;
	
	if (port >=0 && port <=5 ) {
		phy_addr_reg = GSWIP_REG_ACCESS(PHY_ADDR_0-(port * 4));
		mdio_stat_reg = GSWIP_REG_ACCESS(MDIO_STAT_0+(port*4));
		if ( link_up ) {
			/* PHY active Status */
			if ( (mdio_stat_reg >> 6) & 0x1 ) {
				unsigned temp=0;
				/* Link Status */
				if ( (mdio_stat_reg >> 5) & 0x1 ) {
					phy_addr_reg &= ~(0xFFE0);
					phy_addr_reg |= (1 << 13); /* Link up */
					temp = ( (mdio_stat_reg >> 3) & 0x3); /*Speed */
					phy_addr_reg |= (temp << 11); /*Speed */
					if( (mdio_stat_reg >> 2) & 0x1) /*duplex */ {
						phy_addr_reg |= ( 0x1 << 9); /*duplex */
					} else {
						phy_addr_reg |= ( 0x3 << 9); 
					}
					if( (mdio_stat_reg >> 1) & 0x1) /*Receive Pause Enable Status */ {
						phy_addr_reg |= ( 0x1 << 5); /*Receive Pause Enable Status */
					} else {
						phy_addr_reg |= ( 0x3 << 5); 
					}
					if( (mdio_stat_reg >> 0) & 0x1) /*Transmit Pause Enable Status */ {
						phy_addr_reg |= ( 0x1 << 7); /*Transmit Pause Enable Status */
					} else {
						phy_addr_reg |= ( 0x3 << 7); 
					}
					SW_WRITE_REG32(phy_addr_reg, (PHY_ADDR_0-(port * 4)) );
				}
			}
		} else {
			phy_addr_reg &= ~(0xFFE0);
			phy_addr_reg |= (0x3 << 11 );
			SW_WRITE_REG32(phy_addr_reg, (PHY_ADDR_0-(port * 4)) );
		}
	}
}

static int lq_mmd_data_write(int port, unsigned int dev_addr, unsigned char phyaddr,	\
		unsigned int reg_addr, unsigned short data )
{
	unsigned int temp;
	temp = SW_READ_REG32(MDC_CFG_0_REG);
	force_to_phy_settings(port,1);
	temp &= ~(1 << port );
	/* Disable Switch Auto Polling for required  port */
	SW_WRITE_REG32(temp, MDC_CFG_0_REG);
	ifxmips_mdelay(20);
	/*  attempt to acquire the semaphore ... */
	if (down_interruptible(&mdio_sem)) {
		/*  signal received, semaphore not acquired ... */
		return IFX_ERROR;
	}
	lq_ethsw_mdio_data_write(phyaddr, 0xd, dev_addr);
	lq_ethsw_mdio_data_write(phyaddr, 0xe, reg_addr);
	lq_ethsw_mdio_data_write(phyaddr, 0xd, (0x4000 | (dev_addr & 0xFF)) );
	lq_ethsw_mdio_data_write(phyaddr, 0xe, data);
	/* Enable Switch Auto Polling for disabled port */
	temp = SW_READ_REG32(MDC_CFG_0_REG);
	temp |= (1 << port );
	SW_WRITE_REG32(temp, MDC_CFG_0_REG);
	ifxmips_mdelay(100);
	up(&mdio_sem);
	force_to_phy_settings(port,0);
	return IFX_TRUE;
}

static unsigned short lq_mmd_data_read(int port, unsigned int dev_addr, unsigned char phyaddr,	\
		unsigned int reg_addr )
{
	unsigned short data;
	unsigned int temp;
	temp = SW_READ_REG32(MDC_CFG_0_REG);
	force_to_phy_settings(port,1);
	temp &= ~(1 << port );
	/* Disable Switch Auto Polling for required  port */
	SW_WRITE_REG32(temp, MDC_CFG_0_REG);
	ifxmips_mdelay(20);
	/*  attempt to acquire the semaphore ... */
	if (down_interruptible(&mdio_sem)) {
		/*  signal received, semaphore not acquired ... */
		return IFX_ERROR;
	}
	lq_ethsw_mdio_data_write(phyaddr, 0xd, dev_addr);
	lq_ethsw_mdio_data_write(phyaddr, 0xe, reg_addr);
	lq_ethsw_mdio_data_write(phyaddr, 0xd, (0x4000 | (dev_addr & 0xFF)) );
	data = lq_ethsw_mdio_data_read(phyaddr, 0xe);
	/* Enable Switch Auto Polling for disabled port */
	temp = SW_READ_REG32(MDC_CFG_0_REG);
	temp |= (1 << port );
	SW_WRITE_REG32(temp, MDC_CFG_0_REG);
	ifxmips_mdelay(100);
	up(&mdio_sem);
	force_to_phy_settings(port,0);
	return (data);
}

/* Get the GPHY version number & PHY address */
static void Get_GPHY_Version(int port )
{
	unsigned int reg_val, phy_val, temp;
	if (port >=0 && port <=5 ) {
		phy_val = GSWIP_REG_ACCESS(PHY_ADDR_0-(port*4));
		phy_val &= (0x1F);
		PHY_Address[port] = phy_val; /* Keep the PHY address for reference */
		reg_val = lq_ethsw_mdio_data_read(phy_val, 0x1e);
		temp = reg_val & 0x0F00;
		if ( (temp == 0x0100) || (temp == 0x0200) || (temp == 0x0300) )
			gphy_version_detect[port] = 1; /*GPHY 1.3 and GPHY 1.4 */
		else
			gphy_version_detect[port] = 0;  /* GPHY 1.5 and above */
#if  defined(FSM_DETECT_ERROR)  && FSM_DETECT_ERROR
		if ( ( ( reg_val & 0x0F0F) == 0x10B ) || ( (reg_val & 0x0F0F) == 0x305) ) {
			gphy_version_nibble_fix[port] = 1;
			printk(KERN_ERR "GPHY version detetced for new APP workaround!!\n");
		}
		else {
			gphy_version_nibble_fix[port] = 0;
		}
#endif /* FSM_DETECT_ERROR */
	}
}

unsigned int inline  read_TSTAT_Reg(int port, int phy_addr)
{
	unsigned short ret;
#if defined(CONFIG_FE_MODE)
#if  defined(CONFIG_VR9)
	if ( (port == 3 ) || ( (port == 5) && phy_addr == 0x14) )
		ret = lq_mmd_data_read(port,0x1F,phy_addr, 0x07D4 );
	else
		ret = lq_mmd_data_read(port,0x1F,phy_addr, 0x078B );
#endif
#if  defined(CONFIG_AR10)
	if ( (port == 3 ) || (port == 5)  )
		ret = lq_mmd_data_read(port,0x1F,phy_addr, 0x07D4 );
	else
		ret = lq_mmd_data_read(port,0x1F,phy_addr, 0x078B );
#endif

#endif  /*CONFIG_FE_MODE */
	
#if defined(CONFIG_GE_MODE)
	ret = lq_mmd_data_read(port,0x1F,phy_addr, 0x078B );
#endif

#if defined(CONFIG_GEFE_MIXED_MODE)
	if ( (port == 3 ) || (port == 5)  )
		ret = lq_mmd_data_read(port,0x1F,phy_addr, 0x07D4 );
	else
		ret = lq_mmd_data_read(port,0x1F,phy_addr, 0x078B );
#endif
	return (ret);
}

void inline   write_TSTAT_Reg(int port, int phy_addr, int val) 
{
#if defined(CONFIG_FE_MODE)
#if  defined(CONFIG_VR9)
	if ( (port == 3 ) || ( (port == 5) && phy_addr == 0x14) )
		lq_mmd_data_write(port,0x1F, phy_addr, 0x07D4, (val&0xFFFF));
	else
		lq_mmd_data_write(port,0x1F, phy_addr, 0x078B, (val&0xFFFF));
#endif
#if  defined(CONFIG_AR10)
	if ( (port == 3 ) || (port == 5) )
		lq_mmd_data_write(port,0x1F, phy_addr, 0x07D4, (val&0xFFFF));
	else
		lq_mmd_data_write(port,0x1F, phy_addr, 0x078B, (val&0xFFFF));
#endif
#endif

#if defined(CONFIG_GE_MODE)
	lq_mmd_data_write(port,0x1F, phy_addr, 0x078B, (val&0xFFFF));
#endif

#if defined(CONFIG_GEFE_MIXED_MODE)
	if ( (port == 3 ) || (port == 5) )
		lq_mmd_data_write(port,0x1F, phy_addr, 0x07D4, (val&0xFFFF));
	else
		lq_mmd_data_write(port,0x1F, phy_addr, 0x078B, (val&0xFFFF));
#endif
}

/* Workaround for Clean-on-Read to external ports*/
static void gphy_COR_configure(int port,int phy_addr)
{
	unsigned short temp;
	temp = lq_mmd_data_read(port,0x1F,phy_addr, 0x1FF);
	temp |= 0x1;
	lq_mmd_data_write(port,0x1F,phy_addr, 0x1FF, temp);
}

 /* MDIO auto-polling for T2.12 version */
static void gphy_MDIO_AutoPoll_configure(int port, int phy_addr) 
{
	lq_mmd_data_write(port,0x1F, phy_addr,0x1FF, 0x1);
	/* Enable STICKY functionality for internal GPHY */
	lq_ethsw_mdio_data_write(phy_addr, 0x14, 0x8106);
	/* Issue GPHY reset */
	lq_ethsw_mdio_data_write(phy_addr, 0x0, 0x9000);
}

 /* Disable the MDIO interrupt for external GPHY */
static void gphy_disable_MDIO_interrupt(int port, int phy_addr) 
{
	lq_mmd_data_write(port,0x1F, phy_addr,0x0703, 0x2);
}

 /* preferred MASTER device */
static void configure_gphy_master_mode(int phy_addr)
{
   lq_ethsw_mdio_data_write(phy_addr, 0x9, 0x700);
}

/* Disabling of the Power-Consumption Scaling for external GPHY (Bit:2)*/
static void disable_power_scaling_mode(int phy_addr)
{
	unsigned short value;
	value =  lq_ethsw_mdio_data_read(phy_addr, 0x14);
	value &= ~(1<<2);
	lq_ethsw_mdio_data_write(phy_addr, 0x14, value);
}
#if defined(CONFIG_GE_MODE)	
static void ltq_gphy_led0_config (int port, unsigned char phyaddr)
{
	/* for GE modes, For LED0    (SPEED/LINK INDICATION ONLY) */
	lq_mmd_data_write(port,0x1F, phyaddr, 0x1e2, 0x42);
	lq_mmd_data_write(port,0x1F, phyaddr, 0x1e3, 0x10);
	/* Enable Switch Auto Polling for disabled port */
}

static void ltq_gphy_led1_config (int port, unsigned char phyaddr)
{
	/* for GE modes, For LED1, DATA TRAFFIC INDICATION ONLY */
	lq_mmd_data_write(port,0x1F, phyaddr, 0x1e4, 0x70);
	lq_mmd_data_write(port,0x1F, phyaddr,  0x1e5, 0x03);
}
#endif
 /* Disable the EEE Auto-Negotiation Advertisement  */
static void ltq_gphy_disable_EEE_Advertisement(int port, int phy_addr) 
{
	lq_mmd_data_write(port,0x07, phy_addr,0x003C, 0x0);
}
#if  defined(FSM_DETECT_ERROR)  && FSM_DETECT_ERROR
/* for APPLE Airport workaround */
static void ltq_gphy_configure_FSM_reset(int port, int phy_addr)
{
#if defined(CONFIG_GE_MODE)
	unsigned int fsm_value = 0x005;
#else
	unsigned int fsm_value = 0x007;
#endif
	lq_mmd_data_write(port, 0x1F, phy_addr, 0x1FF, (fsm_value&0xFFFF));	
}
#endif /* FSM_DETECT_ERROR */
/**
Mode			Link-LED			Data-LED
Link-Down		OFF					OFF
10baseT			BLINK-SLOW			PULSE on TRAFFIC
100baseTX		BLINK-FAST			PULSE on TRAFFIC
1000baseT		ON					PULSE on TRAFFIC
**/
void ltq_ext_gphy_led_config (void)
{
#if defined(CONFIG_GE_MODE)	
	ltq_gphy_led0_config(0,PHY_Address[0]);
	ltq_gphy_led1_config(0,PHY_Address[0]);
	ltq_gphy_led0_config(1,PHY_Address[1]);
	ltq_gphy_led1_config(1,PHY_Address[1]);
	ltq_gphy_led0_config(5,PHY_Address[5]);
	ltq_gphy_led1_config(5,PHY_Address[5]);
#endif
}
void ltq_int_gphy_led_config (void)
{
#if defined(CONFIG_GE_MODE)
#if  defined(CONFIG_AR10)	
	ltq_gphy_led0_config(0,PHY_Address[0]);
	ltq_gphy_led1_config(0,PHY_Address[0]);
#endif
	ltq_gphy_led0_config(2,PHY_Address[2]);
	ltq_gphy_led1_config(2,PHY_Address[2]);
	ltq_gphy_led0_config(4,PHY_Address[4]);
	ltq_gphy_led1_config(4,PHY_Address[4]);
#endif /*CONFIG_GE_MODE */
}

#ifdef CONFIG_GSWITCH_HD_IPG_WORKAROUND
static irqreturn_t gphy_int_handler(int, void *);
static irqreturn_t gphy_int_handler(int irq, void *dev_id)
{
	unsigned int status, port,reg_val;
/* 	printk("Got interrupt: : %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__); */
 	for (port=0; port < NUM_OF_PORTS; port++) {
 		status = SW_READ_REG32(VR9_MAC_PISR + (0xC * port) );
     	if (status)
     		SW_WRITE_REG32(status, VR9_MAC_PISR + (0xC * port)); /* Full Duplex Status */
     		
     	reg_val = SW_READ_REG32(VR9_MAC_PSTAT + (port * 0xC) );
     	status = SW_READ_REG32(VR9_MAC_CTRL_1 + (0xC * port) );
     	status &= ~(0xF);
     	if ( ( ( (reg_val >> 8) & 0x1 ) == 0 ) &&  ((reg_val >> 3) & 0x1 ) ) {
     		status |= 0xB;
     		SW_WRITE_REG32(status, VR9_MAC_CTRL_1 + (0xC * port));
     	} else {
     		status |= 0xC;
     		SW_WRITE_REG32(status, VR9_MAC_CTRL_1 + (0xC * port));
     	}
    }
    return IRQ_HANDLED;
} 
#endif/* CONFIG_GSWITCH_HD_IPG_WORKAROUND */
#if defined(CONFIG_GPHY_INT_SUPPORT) && CONFIG_GPHY_INT_SUPPORT

int unregister_gphy_interrupt(struct ltq_phy_link_status *g_status)
{
	switch(g_status->port_num) {
#if  defined(CONFIG_AR10) 
		case 1:
			lq_ethsw_mdio_data_write(PHY_Address[1], 0x19, 0x0);
			free_irq(LTQ_GPHY2_INT, (void*)g_status );
			break;
#endif
		case 2:
			lq_ethsw_mdio_data_write(PHY_Address[2], 0x19, 0x0);
			free_irq(LTQ_GPHY0_INT, (void*)g_status );
			break;
#if defined(CONFIG_FE_MODE)
		case 3:
			lq_ethsw_mdio_data_write(PHY_Address[3], 0x19, 0x0);
			free_irq(LTQ_GPHY0_INT, (void*)g_status );
			break;
		case 5:
			lq_ethsw_mdio_data_write(PHY_Address[5], 0x19, 0x0);
			free_irq(LTQ_GPHY1_INT, (void*)g_status );
			break;
#endif
		case 4:
			lq_ethsw_mdio_data_write(PHY_Address[4], 0x19, 0x0);
			free_irq(LTQ_GPHY1_INT, (void*)g_status );
		break;

	default:
		printk("Register GPHY port number (%d) Failed: : %s:%s:%d\n",g_status->port_num,__FILE__, __FUNCTION__, __LINE__);
		return IFX_ERROR;
	}
	return IFX_TRUE;
}

static irqreturn_t gphy1_link_int_handler(int irq, void *dev_id)
{
	ltq_phy_link_status_t  *status;
    unsigned int reg_val;
    status = (ltq_phy_link_status_t*)dev_id;
    /* clear the interrupt*/
	lq_ethsw_mdio_data_read(PHY_Address[1], 0x1A);
	reg_val = lq_ethsw_mdio_data_read(PHY_Address[1], 0x01);
 	if (status->ltq_gphy_int_handler) {
 		status->ltq_gphy_int_handler(status, (reg_val>>2 & 0x1) );
 	}
    return IRQ_HANDLED;
}

static irqreturn_t gphy2_link_int_handler(int irq, void *dev_id)
{
    ltq_phy_link_status_t  *status;
    unsigned int reg_val;
    status = (ltq_phy_link_status_t*)dev_id;
     /* clear the interrupt*/
    lq_ethsw_mdio_data_read(PHY_Address[2], 0x1A);
	reg_val = lq_ethsw_mdio_data_read(PHY_Address[2], 0x1);
 	if (status->ltq_gphy_int_handler) {
 		status->ltq_gphy_int_handler(status, (reg_val>>2 & 0x1) );
 	}
 	return IRQ_HANDLED;
} 
#if defined(CONFIG_FE_MODE)
static irqreturn_t gphy3_link_int_handler(int irq, void *dev_id)
{
    ltq_phy_link_status_t  *status;
    unsigned int reg_val;
    status = (ltq_phy_link_status_t*)dev_id;
     /* clear the interrupt*/
	lq_ethsw_mdio_data_read(PHY_Address[3], 0x1A);
	reg_val = lq_ethsw_mdio_data_read(PHY_Address[3], 0x01);
 	if (status->ltq_gphy_int_handler) {
 		status->ltq_gphy_int_handler(status, (reg_val>>2 & 0x1) );
 	}
 	return IRQ_HANDLED;
}
static irqreturn_t gphy5_link_int_handler(int irq, void *dev_id)
{
	ltq_phy_link_status_t  *status;
    unsigned int reg_val;
    status = (ltq_phy_link_status_t*)dev_id;
     /* clear the interrupt*/
	lq_ethsw_mdio_data_read(PHY_Address[5], 0x1A);
	reg_val = lq_ethsw_mdio_data_read(PHY_Address[5], 0x01);
 	if (status->ltq_gphy_int_handler) {
 		status->ltq_gphy_int_handler(status, (reg_val>>2 & 0x1) );
 	}   
    return IRQ_HANDLED;
}
#endif
static irqreturn_t gphy4_link_int_handler(int irq, void *dev_id)
{
	ltq_phy_link_status_t  *status;
    unsigned int reg_val;
    status = (ltq_phy_link_status_t*)dev_id;
     /* clear the interrupt*/
	lq_ethsw_mdio_data_read(PHY_Address[4], 0x1A);
	reg_val = lq_ethsw_mdio_data_read(PHY_Address[4], 0x01);
 	if (status->ltq_gphy_int_handler) {
 		status->ltq_gphy_int_handler(status, (reg_val>>2 & 0x1) );
 	}   
    return IRQ_HANDLED;
} 

int register_gphy_interrupt(struct ltq_phy_link_status *g_status)
{
	switch(g_status->port_num) {
#if  defined(CONFIG_AR10) 
		case 1:
			if ( request_irq(LTQ_GPHY2_INT, gphy1_link_int_handler, IRQF_DISABLED,	\
				"GPHY1_INT_ISR", (void*)g_status) ) {
				printk("Request GPHY(%d) IRQ Failed: : %s:%s:%d\n",LTQ_GPHY2_INT,__FILE__, __FUNCTION__, __LINE__);
				return IFX_ERROR;
			}
			lq_ethsw_mdio_data_write(PHY_Address[1], 0x19, GPHY_IMASK);
		break;
#endif /* CONFIG_AR10 */
		case 2:
			if ( request_irq(LTQ_GPHY0_INT, gphy2_link_int_handler, IRQF_DISABLED,	\
				"GPHY2_INT_ISR", (void*)g_status)) {
				printk("Request GPHY(%d) IRQ Failed: : %s:%s:%d\n",LTQ_GPHY0_INT,__FILE__, __FUNCTION__, __LINE__);
				return IFX_ERROR;
			}
			lq_ethsw_mdio_data_write(PHY_Address[2], 0x19, GPHY_IMASK);
		break;
#if defined(CONFIG_FE_MODE)
		case 3:
		if ( request_irq(LTQ_GPHY0_INT, gphy3_link_int_handler, IRQF_DISABLED,	\
				"GPHY2_INT_ISR", (void*)g_status)) {
				printk("Request GPHY(%d) IRQ Failed: : %s:%s:%d\n",LTQ_GPHY0_INT,__FILE__, __FUNCTION__, __LINE__);
				return IFX_ERROR;
		}
		lq_ethsw_mdio_data_write(PHY_Address[3], 0x19, GPHY_IMASK);
		break;
	case 5:
		if ( request_irq(LTQ_GPHY1_INT, gphy5_link_int_handler, IRQF_DISABLED,	\
				"GPHY4_INT_ISR", (void*)g_status ) ) {
			printk("Request GPHY(%d) IRQ Failed: : %s:%s:%d\n",LTQ_GPHY1_INT,__FILE__, __FUNCTION__, __LINE__);
			return IFX_ERROR;
		}
		lq_ethsw_mdio_data_write(PHY_Address[5], 0x19, GPHY_IMASK);
		break;
#endif
	case 4:
		if ( request_irq(LTQ_GPHY1_INT, gphy4_link_int_handler, IRQF_DISABLED,	\
				"GPHY4_INT_ISR", (void*)g_status ) ) {
			printk("Request GPHY(%d) IRQ Failed: : %s:%s:%d\n",LTQ_GPHY1_INT,__FILE__, __FUNCTION__, __LINE__);
			return IFX_ERROR;
		}
		lq_ethsw_mdio_data_write(PHY_Address[4], 0x19, GPHY_IMASK);
		break;
	default:
		printk("Register GPHY port number (%d) Failed: : %s:%s:%d\n",g_status->port_num,__FILE__, __FUNCTION__, __LINE__);
		return IFX_ERROR;
	}
	return IFX_TRUE;
}
#if defined(INT_TEST_CODE) && INT_TEST_CODE
int test_gphy_interrupt_handler1 (struct ltq_phy_link_status *g_status, int status)
{
	printk("GPHY Link Status : %s:%s:%d port number:%d, statu:%d\n",	\
		__FILE__, __FUNCTION__, __LINE__,g_status->port_num, status);
 	return 1;
}

int test_gphy_interrupt1( void)
{
	memset(&g_gphy_status1, 0, sizeof(ltq_phy_link_status_t));
	g_gphy_status1.port_num = 1;
	g_gphy_status1.ltq_gphy_int_handler = &test_gphy_interrupt_handler1;
	if ( register_gphy_interrupt(&g_gphy_status1) == IFX_ERROR ) {
		printk("Register GPHY port number  Failed: : %s:%s:%d\n",__FILE__, __FUNCTION__, __LINE__);
	}
	return 0;
}
int test_gphy_interrupt_handler2 (struct ltq_phy_link_status *g_status, int status)
{
	printk("GPHY Link Status : %s:%s:%d port number:%d, statu:%d\n",	\
		__FILE__, __FUNCTION__, __LINE__,g_status->port_num, status);
 	return 1;
}

int test_gphy_interrupt2( void)
{
	memset(&g_gphy_status2, 0, sizeof(ltq_phy_link_status_t));
	g_gphy_status2.port_num = 2;
	g_gphy_status2.ltq_gphy_int_handler = &test_gphy_interrupt_handler2;
	if ( register_gphy_interrupt(&g_gphy_status2) == IFX_ERROR ) {
		printk("Register GPHY port number  Failed: : %s:%s:%d\n",__FILE__, __FUNCTION__, __LINE__);
	}
	return 0;
}
int test_gphy_interrupt_handler4 (struct ltq_phy_link_status *g_status, int status)
{
	printk("GPHY Link Status : %s:%s:%d port number:%d, statu:%d\n",	\
		__FILE__, __FUNCTION__, __LINE__,g_status->port_num, status);
 	return 1;
}

int test_gphy_interrupt4( void)
{
	memset(&g_gphy_status4, 0, sizeof(ltq_phy_link_status_t));
	g_gphy_status4.port_num = 4;
	g_gphy_status4.ltq_gphy_int_handler = &test_gphy_interrupt_handler4;
	if ( register_gphy_interrupt(&g_gphy_status4) == IFX_ERROR ) {
		printk("Register GPHY port number  Failed: : %s:%s:%d\n",__FILE__, __FUNCTION__, __LINE__);
	}
	return 0;
}
#endif /* INT_TEST_CODE */
#endif /* CONFIG_GPHY_INT_SUPPORT */

#ifdef CONFIG_GPHY_NIB_ALIN_WORKAROUND
static int gphy_link_detect_thread (void *arg)
{
	allow_signal(SIGKILL);	
	/* Poll the Link Status  */
	while(!kthread_should_stop ()) {
		int port,i;
		unsigned int temp,reg_val;
		set_current_state(TASK_INTERRUPTIBLE);
		if (signal_pending(current))
			break;
		thread_interval = 3;
			/* Get the port Link Status  */
		for (port=0; port < NUM_OF_PORTS; port++) {
			int counter = 0, detect=0;
			reg_val = SW_READ_REG32( (VR9_MAC_PSTAT + (port * 0xC * 4) ) );
			if ( ( (reg_val >> 3 ) & 0x1 ) == 0x1 ) {
				if ( ( ( (reg_val >> 9) & 0x3 ) == 0x1 ) ) {
					gphy_link_status[port] = 1;
					if  ( gphy_version_detect[port] == 1  ) {
#if  defined(FSM_DETECT_ERROR)  && FSM_DETECT_ERROR
						if (gphy_version_nibble_fix[port] == 1) {
							temp = lq_ethsw_mdio_data_read(PHY_Address[port], 0xC);
							if ( ( (temp >> 15) & 0x1 )==  1) {
									unsigned int an_adv_reg;
									gphy_link_speed[port] = 1;
									an_adv_reg = lq_ethsw_mdio_data_read(PHY_Address[port], 0x4);
									gphy_ADV_default_reg_value[port] = an_adv_reg;
									an_adv_reg &= ~AN_ADV_DEFAULT;
									an_adv_reg |= AN_ADV_10BASE;
									lq_ethsw_mdio_data_write(PHY_Address[port], 0x4, an_adv_reg);
									lq_ethsw_mdio_data_write(PHY_Address[port], 0x0, 0x1200);
									gphy_link_status[port] = 0;
									thread_interval = 4;
							}
						} else 
#endif /*FSM_DETECT_ERROR*/						
						{

							do {
								temp = read_TSTAT_Reg(port, PHY_Address[port]);
								detect=0;
								if ( temp  == 0x8 ) {
									unsigned int an_adv_reg;
									write_TSTAT_Reg(port, PHY_Address[port], 0x8);
									gphy_err_detect_cnt[port]++;
									counter++;
									detect = 1;
									ifxmips_mdelay(350);
									if(gphy_err_detect_cnt[port] == ERR_COUNT ) {
										gphy_link_speed[port] = 1;
										an_adv_reg = lq_ethsw_mdio_data_read(PHY_Address[port], 0x4);
										gphy_ADV_default_reg_value[port] = an_adv_reg;
										an_adv_reg &= ~AN_ADV_DEFAULT;
										an_adv_reg |= AN_ADV_10BASE;
										lq_ethsw_mdio_data_write(PHY_Address[port], 0x4, an_adv_reg);
										lq_ethsw_mdio_data_write(PHY_Address[port], 0x0, 0x1200);
										gphy_err_detect_cnt[port]=0;
										gphy_link_status[port] = 0;
										thread_interval = 4;
									}
								}
							} while( (counter < 4) && (detect) && (gphy_link_speed[port] != 1));
						}
					}
				}
			} else {
				gphy_link_status[port] = 0;
				gphy_err_detect_cnt[port]=0;
				/*if Previously detected the error and was set 10Mbps, then configure the default value */
				if(gphy_link_speed[port] == 1 ) {
					lq_ethsw_mdio_data_write(PHY_Address[port], 0x4, gphy_ADV_default_reg_value[port]);
					gphy_link_speed[port] = 0;
				}
			}
		}
		thread_wind_counter++;
		if ( (thread_wind_counter%120 == 0 )  ) {
			for (  i  = 0; i < NUM_OF_PORTS; i++) {
				/* reset the global variable */
				gphy_err_detect_cnt[i]=0;
			}
			thread_wind_counter = 0;
		}
		/*poll again  once configured time is up */
		interruptible_sleep_on_timeout(&gphy_link_thread_wait, (thread_interval * HZ) );
	}
}
#endif /* CONFIG_GPHY_NIB_ALIN_WORKAROUND */

static int ltq_ext_Gphy_hw_Specify_init(void)
{
	/* Workaround for Clean-on-Read to external ports*/
	gphy_COR_configure(0,PHY_Address[0]);
	gphy_COR_configure(1,PHY_Address[1]);
	gphy_COR_configure(5,PHY_Address[5]);
	 /* Disable the MDIO interrupt for external GPHY */
	gphy_disable_MDIO_interrupt(0,PHY_Address[0]);
	gphy_disable_MDIO_interrupt(1,PHY_Address[1]);
	gphy_disable_MDIO_interrupt(5,PHY_Address[5]);
	
	disable_power_scaling_mode(PHY_Address[0]);
	disable_power_scaling_mode(PHY_Address[1]);
	disable_power_scaling_mode(PHY_Address[5]);
	/* preferred MASTER device */
	configure_gphy_master_mode(PHY_Address[0]);
	configure_gphy_master_mode(PHY_Address[1]);
	configure_gphy_master_mode(PHY_Address[5]);
	return 0;
}

static int ltq_Int_Gphy_hw_Specify_init(void)
{
	gphy_MDIO_AutoPoll_configure(2,0x11);
	configure_gphy_master_mode(0x11);
#if ( defined(CONFIG_VR9) )
	/* MDIO auto-polling for T2.12 version */
	gphy_MDIO_AutoPoll_configure(4,0x13);
	configure_gphy_master_mode(0x13);
#endif	
	return 0;
}

static int ltq_gphy_reset(int phy_num)
{
	unsigned int reg;
	reg = ifx_rcu_rst_req_read();
	if ( (phy_num == 3) || (phy_num == 4) ) {
#ifdef CONFIG_AR10_GRX390
		if ( phy_num == 4 ) 
			reg |= (1 << 10);
#endif  /* CONFIG_AR10_GRX390 */
		reg |= ( ( 1 << 31 ) | (1 << 29) | (1 << 28) );
	}	else if ( phy_num == 2 ) {
		reg |= ( 1 << 31 ) | (1 << 29);
	} else if ( phy_num == 0 ) {
		reg |= (1 << 31);
	} else if ( phy_num == 1 ) {
		reg |= (1 << 29);
	} else {
		return 1;
	}
	ifx_rcu_rst_req_write(reg, reg);
	return 0;
}
 
static int ltq_gphy_reset_released(int phy_num)
{
	unsigned int mask = 0;
	
	if ( (phy_num == 3) || (phy_num == 4) ) {
#ifdef CONFIG_AR10_GRX390
		if ( phy_num == 4 ) 
			mask = (1 << 10);
#endif  /* CONFIG_AR10_GRX390 */
		mask |= (( 1 << 31 ) | (1 << 29) | ( 1<<28) );
	}	else if (phy_num == 2) {
		mask = ( 1 << 31 ) | (1 << 29);
	}	else if ( phy_num == 0 ) {
		mask = ( 1 << 31 );
	}	else if ( phy_num == 1 ) {
		mask = ( 1 << 29 );
	}	else {
		return 1;
	}
	ifx_rcu_rst_req_write(0, mask);
	return 0;
}

/** Driver version info */
static inline int gphy_drv_ver(char *buf)
{
#if  (defined(CONFIG_VR9) || defined(CONFIG_HN1) )
	return sprintf(buf, "IFX GPHY driver %s, version %s - Firmware: %x\n", IFX_GPHY_MODE,	\
		version, lq_ethsw_mdio_data_read(0x11, 0x1e));
#endif
#if  defined(CONFIG_AR10)
	return sprintf(buf, "IFX GPHY driver %s, version %s - Firmware: %x\n", IFX_GPHY_MODE,	\
		version, lq_ethsw_mdio_data_read(0x2, 0x1e));
#endif
}

/** Displays the version of GPHY module via proc file */
static int gphy_proc_version(IFX_char_t *buf, IFX_char_t **start, off_t offset,
                         int count, int *eof, void *data)
{
	int len = 0;
	/* No sanity check cos length is smaller than one page */
	len += gphy_drv_ver(buf + len);
	*eof = 1;
	return len;
}
#ifdef CONFIG_GPHY_NIB_ALIN_WORKAROUND
/** Displays the version of GPHY module via proc file */
static int gphy_debug_info(IFX_char_t *buf, IFX_char_t **start, off_t offset,
                         int count, int *eof, void *data)
{
	int len = 0, i;
	/* No sanity check cos length is smaller than one page */
	for (i = 0; i < NUM_OF_PORTS; i++) {
		len += sprintf(buf + len , "Port:%d\n",i );
		len += sprintf(buf + len, "gphy_version_detect:%d\n",gphy_version_detect[i] );
#if  defined(FSM_DETECT_ERROR)  && FSM_DETECT_ERROR
		len += sprintf(buf + len, "gphy_version_nibble_fix:%d\n",gphy_version_nibble_fix[i] );
#endif /* FSM_DETECT_ERROR */
		len += sprintf(buf + len, "gphy_err_detect_cnt:%d\n",gphy_err_detect_cnt[i] );
		len += sprintf(buf + len, "gphy_link_status:%d\n",gphy_link_status[i] );
		len += sprintf(buf + len, "PHY_Address:%d\n",PHY_Address[i] );
		len += sprintf(buf + len, "gphy_link_speed:%d\n",gphy_link_speed[i] );
	}
	*eof = 1;
	return len;
}
#endif /* CONFIG_GPHY_NIB_ALIN_WORKAROUND */

#ifdef CONFIG_FW_LOAD_FROM_USER_SPACE
#if defined(CONFIG_GEFE_MIXED_MODE)
#error "Mixed mode not support CONFIG_FW_LOAD_FROM_USER_SPACE yet. This code need to be patched....."
#endif
static int proc_read_gphy(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    len += sprintf(page + len, "Not Implemented\n");
    *eof = 1;
    return len;
}

unsigned int found_magic = 0, found_img = 0, first_block =0, fw_len=0, rcv_size = 0, second_block = 0 ;
static int proc_write_gphy(struct file *file, const char *buf, unsigned long count, void *data)
{
	gphy_image_header_t header;
	int len = 0;
	char local_buf[4096] = {0};
	memset(&header, 0, sizeof(gphy_image_header_t));
    len = sizeof(local_buf) < count ? sizeof(local_buf) - 1 : count;
    len = len - copy_from_user((local_buf), buf, len);
   
	if (!found_img) {
		memcpy(&header, (local_buf + (found_magic * sizeof(gphy_image_header_t))), sizeof(gphy_image_header_t) );
		if (header.ih_magic == IH_MAGIC_NO) {
			found_magic++;
			first_block = 0;
			second_block = 0;
		}
	}
	if (!found_img) {
#if defined(CONFIG_GE_MODE)
		if( ( ( strncmp(header.ih_name, "VR9 V1.1 GPHY GE", sizeof(header.ih_name)) == 0)	\
			|| ( strncmp(header.ih_name, "AR10 V1.1 GPHY GE", sizeof(header.ih_name)) == 0) ) && (dev_id == 0) ) {
			first_block = 1;
			fw_len = header.ih_size;
			found_img = 1;
/*			printk(" Found V1.1 GPHY GE FW  \n"); */
		} else if( ( ( strncmp(header.ih_name, "VR9 V1.2 GPHY GE", sizeof(header.ih_name)) == 0)	\
			|| ( strncmp(header.ih_name, "AR10 V1.2 GPHY GE", sizeof(header.ih_name)) == 0) ) && (dev_id == 1) )	{
			first_block = 1;
			fw_len = header.ih_size;
			found_img = 1;
/*			printk(" Found V1.2 GPHY GE FW  \n"); */
		} 
#endif
#if defined(CONFIG_FE_MODE)
		if( ( ( strncmp(header.ih_name, "VR9 V1.1 GPHY FE", sizeof(header.ih_name)) == 0)	\
			 || ( strncmp(header.ih_name, "AR10 V1.1 GPHY FE", sizeof(header.ih_name)) == 0) ) && (dev_id == 0) ) {
			first_block = 1;
			fw_len = header.ih_size;
			found_img = 1;
/*			printk(" Found V1.1 GPHY FE FW  \n"); */
		} else if( ( ( strncmp(header.ih_name, "VR9 V1.2 GPHY FE", sizeof(header.ih_name)) == 0 )	\
			|| ( strncmp(header.ih_name, "AR10 V1.2 GPHY FE", sizeof(header.ih_name)) == 0 ) ) && (dev_id == 1) ) {
			first_block = 1;
			fw_len = header.ih_size;
			found_img = 1;
/*			printk(" Found V1.2 GPHY FE FW  \n"); */
		}	
#endif
	}	
	if ( (first_block == 1) && (!second_block) && found_img ) {
		memset(proc_gphy_fw_dma, 0, sizeof(proc_gphy_fw_dma));
		rcv_size = (len - (found_magic * sizeof(gphy_image_header_t)));
		memcpy(proc_gphy_fw_dma, local_buf+(found_magic * sizeof(gphy_image_header_t)), rcv_size);
		second_block = 1;
	} else if ((second_block == 1 ) && found_img) {
		if (rcv_size < (fw_len) ) {
			if ( (rcv_size + len) >= fw_len ) {
				memcpy(proc_gphy_fw_dma+rcv_size, local_buf, (fw_len-rcv_size ));
				first_block = 0;
				found_img = 0;
				second_block = 0;
				found_magic = 0;
			ltq_gphy_firmware_config(total_gphys, 1, dev_id, ge_fe_mode );
			} else {
				memcpy(proc_gphy_fw_dma+rcv_size, local_buf, (len ));
				rcv_size += len;
			}
		} else {
			first_block = 0;
			found_img = 0;
			second_block = 0;
			found_magic = 0;
			ltq_gphy_firmware_config(total_gphys, 1, dev_id, ge_fe_mode );
		}	
	}
	return len;
}
#endif /* CONFIG_FW_LOAD_FROM_USER_SPACE */

/* Terry 20140307, reset / reset_released gphy */
#if 1
void create_proc_ltq_gphy_reset();
#endif
/* Terry 20141021, enable / disable gphy led control */
#ifdef GPHY_LED_SIGNAL_IMPLEMENTATION
void create_proc_ltq_gphy_led_control();
#endif
	
/** create proc for debug  info, \used ifx_eth_module_init */
static int gphy_proc_create(void)
{
#ifdef CONFIG_FW_LOAD_FROM_USER_SPACE
	struct proc_dir_entry *res;
#endif /* CONFIG_FW_LOAD_FROM_USER_SPACE */
	/* procfs */
	ifx_gphy_dir = proc_mkdir ("driver/ifx_gphy", NULL);
	if (ifx_gphy_dir == NULL) {
		printk(KERN_ERR "%s: Create proc directory (/driver/ifx_gphy) failed!!!\n", __func__);
		return IFX_ERROR;
	}
#ifdef CONFIG_FW_LOAD_FROM_USER_SPACE
#if defined(CONFIG_GEFE_MIXED_MODE)
	#error "Mixed mode not support CONFIG_FW_LOAD_FROM_USER_SPACE yet. This code need to be patched....."
#endif
	res = create_proc_entry("phyfirmware", 0, ifx_gphy_dir);
	if ( res ) {
		res->read_proc  = proc_read_gphy;
        res->write_proc = proc_write_gphy;
    }
#endif /*CONFIG_FW_LOAD_FROM_USER_SPACE*/
	create_proc_read_entry("version", 0, ifx_gphy_dir, gphy_proc_version,  NULL);
#ifdef CONFIG_GPHY_NIB_ALIN_WORKAROUND
	create_proc_read_entry("debug", 0, ifx_gphy_dir, gphy_debug_info,  NULL);
#endif /* CONFIG_GPHY_NIB_ALIN_WORKAROUND */
	
/* Terry 20140307, reset / reset_released gphy */
#if 1
	create_proc_ltq_gphy_reset();
#endif
	
/* Terry 20141021, enable / disable gphy led control */
#ifdef GPHY_LED_SIGNAL_IMPLEMENTATION
	create_proc_ltq_gphy_led_control();
#endif

	return IFX_SUCCESS;
}

/** remove of the proc entries, \used ifx_eth_module_exit */
static void gphy_proc_delete(void)
{
#if 0
	remove_proc_entry("version", gphy_drv_ver);
#endif
}

#if defined(CONFIG_HN1_USE_TANTOS)
unsigned short tantos_read_mdio(unsigned int tRegAddr )
{
	unsigned int phyAddr, regAddr;
	if (tRegAddr >= 0x200) 
		return (0xff);
	regAddr = tRegAddr & 0x1f;          //5-bits regAddr
	phyAddr = (tRegAddr>>5) & 0x1f;     //5-bits phyAddr
    return (lq_ethsw_mdio_data_read(phyAddr, regAddr));
}

void tantos_write_mdio(unsigned int tRegAddr, unsigned int data)
{
    unsigned int phyAddr, regAddr;
    if (tRegAddr >= 0x200)
    	return;
    regAddr = tRegAddr & 0x1f;          //5-bits regAddr
    phyAddr = (tRegAddr>>5) & 0x1f;     //5-bits phyAddr
    return (lq_ethsw_mdio_data_write(phyAddr, regAddr, data));    
}
#define TANTOS_MIIAC    0x120
#define TANTOS_MIIWD    0x121
#define TANTOS_MIIRD    0x122
unsigned short tantos_read_mdio_phy(unsigned int phyAddr, unsigned int tPhyRegAddr )
{
    unsigned int cmd=0;
    if (phyAddr > 4 || tPhyRegAddr > 0x1D ) {
    	return;
    }
    cmd = (1<<15) | (1<<11) | (phyAddr << 5) | tPhyRegAddr; 
    tantos_write_mdio(TANTOS_MIIAC, cmd);
    return (tantos_read_mdio(TANTOS_MIIRD));
}

//phyAddr : 0 to 4 corresponds to Tantos internal PHY ports
unsigned short tantos_write_mdio_phy(unsigned int phyAddr, unsigned int tPhyRegAddr, unsigned int data)
{
    unsigned int cmd=0;
    if (phyAddr > 4 || tPhyRegAddr > 0x1D ) {
    	return;
    }
    cmd = (1<<15) | (1<<10) | (phyAddr << 5) | tPhyRegAddr; 
    tantos_write_mdio(TANTOS_MIIWD, data);
    tantos_write_mdio(TANTOS_MIIAC, cmd);

}
EXPORT_SYMBOL(tantos_read_mdio);
EXPORT_SYMBOL(tantos_write_mdio);
EXPORT_SYMBOL(tantos_read_mdio_phy);
EXPORT_SYMBOL(tantos_write_mdio_phy);
#endif

#ifndef CONFIG_GEFE_MIXED_MODE
int ltq_gphy_firmware_config(int total_gphys, int mode, int dev_id, int ge_fe_mode )
#else
int ltq_gphy_firmware_config(int total_gphys, int mode, int dev_id, int ghpy_fe_mode, int gphy_ge_mode)
#endif
{
	int i, fwSize = GPHY_FW_LEN;
#ifndef CONFIG_GEFE_MIXED_MODE
	unsigned int alignedPtr;
	unsigned char *pFw;
#else
	unsigned int alignedPtr_fe, alignedPtr_ge;
	unsigned char *pFw_fe, *pFw_ge;
#endif
	char ver_str[128] = {0};
	/* Disable Switch Auto Polling for all ports */
	SW_WRITE_REG32(0x0, MDC_CFG_0_REG);
	ifxmips_mdelay(100);
	/* RESET GPHY */
	if(ltq_gphy_reset(total_gphys) == 1)
		printk(KERN_ERR "GPHY driver init RESET FAILED !!\n");

	// find the 64kb aligned boundary in the 128kb buffer
#ifndef CONFIG_GEFE_MIXED_MODE
	alignedPtr = (u32)gphy_fw_dma;
	alignedPtr &= ~GPHY_FW_START_ADDR_ALIGN_MASK; 
	alignedPtr += GPHY_FW_START_ADDR_ALIGN;
#else
	alignedPtr_fe = (u32)gphy_fw_fe_dma;
	alignedPtr_fe &= ~GPHY_FW_START_ADDR_ALIGN_MASK;
	alignedPtr_fe += GPHY_FW_START_ADDR_ALIGN;

	alignedPtr_ge = (u32)gphy_fw_ge_dma;
	alignedPtr_ge &= ~GPHY_FW_START_ADDR_ALIGN_MASK;
	alignedPtr_ge += GPHY_FW_START_ADDR_ALIGN;
#endif
	if ( mode == 1 ) {
#ifdef CONFIG_FW_LOAD_FROM_USER_SPACE
		memset(gphy_fw_dma, 0, sizeof(gphy_fw_dma));
		pFw = (unsigned char *)proc_gphy_fw_dma;
		for(i=0; i<fwSize; i++)
			*((unsigned char *)(alignedPtr+i)) = pFw[i];
		if (ge_fe_mode) {
			printk("GPHY FIRMWARE LOAD SUCCESSFULLY AT ADDR (FE MODE) : %x\n", alignedPtr);
		} else {
			printk("GPHY FIRMWARE LOAD SUCCESSFULLY AT ADDR (GE MODE) : %x\n", alignedPtr);
		}
#if defined(CONFIG_GEFE_MIXED_MODE)
	#error "Mixed mode not support CONFIG_FW_LOAD_FROM_USER_SPACE yet. This code need to be patched....."
#endif
#endif /* CONFIG_FW_LOAD_FROM_USER_SPACE */
	} else {
#ifndef CONFIG_FW_LOAD_FROM_USER_SPACE
#if defined(CONFIG_FE_MODE)
		if (dev_id == 0) {
			pFw = (unsigned char *)gphy_fe_fw_data;
		} else {
			pFw = (unsigned char *)gphy_fe_fw_data_a12;
		}
		printk("GPHY FIRMWARE LOAD SUCCESSFULLY AT ADDR (FE MODE) : %x\n", alignedPtr);
#endif
#if defined(CONFIG_GE_MODE)
		if (dev_id == 0 ) {
			pFw = (unsigned char *)gphy_ge_fw_data;
		} else {
			pFw = (unsigned char *)gphy_ge_fw_data_a12;
		}
		printk("GPHY FIRMWARE LOAD SUCCESSFULLY AT ADDR (GE MODE) : %x\n", alignedPtr);
#endif
#if defined(CONFIG_GEFE_MIXED_MODE)
		pFw_fe = (unsigned char *)gphy_fe_fw_data;
		pFw_ge = (unsigned char *)gphy_ge_fw_data;
		printk("GPHY FIRMWARE LOAD SUCCESSFULLY AT ADDR (FE MODE) : %x\n", alignedPtr_fe);
		printk("GPHY FIRMWARE LOAD SUCCESSFULLY AT ADDR (GE MODE) : %x\n", alignedPtr_ge);
#endif

#ifndef CONFIG_GEFE_MIXED_MODE
	for(i=0; i<fwSize; i++)
		*((char *)(alignedPtr+i)) = pFw[i];
#else
	for(i=0; i<fwSize; i++)
		*((char *)(alignedPtr_fe+i)) = pFw_fe[i];

	for(i=0; i<fwSize; i++)
		*((char *)(alignedPtr_ge+i)) = pFw_ge[i];
#endif
#endif /* CONFIG_FW_LOAD_FROM_USER_SPACE */
	} 
	// remove the kseg prefix before giving this address to GPHY's DMA
#ifndef CONFIG_GEFE_MIXED_MODE
	alignedPtr &= 0x0FFFFFFF;
#else
	alignedPtr_fe &= 0x0FFFFFFF;
	alignedPtr_ge &= 0x0FFFFFFF;
#endif
#if  defined(CONFIG_VR9) 
	/* Load GPHY0 firmware module  */
	SW_WRITE_REG32( alignedPtr, GFS_ADD0);
	/* Load GPHY1 firmware module  */
	SW_WRITE_REG32( alignedPtr, GFS_ADD1);
/*	printk("GPHY FIRMWARE LOAD SUCCESSFULLY AT ADDR (VR9) : %x\n", alignedPtr); */
#endif  /*CONFIG_VR9*/

#if  defined(CONFIG_AR10) 
#ifndef CONFIG_GEFE_MIXED_MODE
	/* Load GPHY0 firmware module  */
	SW_WRITE_REG32( alignedPtr, GFS_ADD0);
	/* Load GPHY1 firmware module  */
	SW_WRITE_REG32( alignedPtr, GFS_ADD1);
	/* Load GPHY2 firmware module  */
	SW_WRITE_REG32( alignedPtr, GFS_ADD2);
#if   defined(CONFIG_AR10_GRX390)
/* Load GPHY2 firmware module  */
	SW_WRITE_REG32( alignedPtr, GFS_ADD3);
#endif /* CONFIG_AR10_GRX390 */
/*	printk("GPHY FIRMWARE LOAD SUCCESSFULLY AT ADDR (AR10) : %x\n", alignedPtr); */
#else
	/* Load GPHY0 firmware module  */
	SW_WRITE_REG32( alignedPtr_fe, GFS_ADD0);
	/* Load GPHY1 firmware module  */
	SW_WRITE_REG32( alignedPtr_ge, GFS_ADD1);
	/* Load GPHY2 firmware module  */
	SW_WRITE_REG32( alignedPtr_ge, GFS_ADD2);
#if   defined(CONFIG_AR10_GRX390)
/* Load GPHY2 firmware module  */
	SW_WRITE_REG32( alignedPtr_ge, GFS_ADD3);
#endif /* CONFIG_AR10_GRX390 */
/*	printk("GPHY FIRMWARE LOAD SUCCESSFULLY AT ADDR (AR10) : %x\n", alignedPtr); */
#endif
#endif /*CONFIG_AR10 */
#if  defined(CONFIG_HN1) 
	/* Load GPHY0 firmware module  */
	SW_WRITE_REG32( alignedPtr, GFS_ADD0);
/*	printk("GPHY FIRMWARE LOAD SUCCESSFULLY AT ADDR (GHN) : %x\n", alignedPtr); */
#endif  /*CONFIG_HN1*/
//	ifxmips_mdelay(1000);
	if(ltq_gphy_reset_released(total_gphys) == 1)
		printk(KERN_ERR "GPHY driver init RELEASE FAILED !!\n");
	ifxmips_mdelay(500);
	if (dev_id == 0) {
		if(ltq_Int_Gphy_hw_Specify_init() == 1)
			printk(KERN_ERR "GPHY driver Specify init FAILED !!\n");
	}
#if  defined(CONFIG_HN1)
	SW_WRITE_REG32(0x3, MDC_CFG_0_REG);
#else
	SW_WRITE_REG32(0x3f, MDC_CFG_0_REG);
#endif
	ifxmips_mdelay(500);
	ltq_int_gphy_led_config();
	Get_GPHY_Version(2 );
	Get_GPHY_Version(3 );
	Get_GPHY_Version(4 );
	Get_GPHY_Version(5 );
#if  defined(FSM_DETECT_ERROR)  && FSM_DETECT_ERROR
	/* FSM Reset  */
	for (i = 0; i < NUM_OF_PORTS; i++) {
		if ( gphy_version_nibble_fix[i] == 1 ) {
			ltq_gphy_configure_FSM_reset(i, PHY_Address[i]);
			lq_ethsw_mdio_data_write(PHY_Address[i], 0x15, 0x900);
		}
	}
#endif /* FSM_DETECT_ERROR */
	gphy_drv_ver(ver_str);
	printk(KERN_INFO "%s", ver_str);
#if defined(CONFIG_GPHY_INT_SUPPORT) && CONFIG_GPHY_INT_SUPPORT
#if defined(INT_TEST_CODE) && INT_TEST_CODE
	test_gphy_interrupt1();
	test_gphy_interrupt2();
	test_gphy_interrupt4();
#endif /* INT_TEST_CODE*/
#endif /* CONFIG_GPHY_INT_SUPPORT */
	return 0;
}

#ifdef GPHY_LED_SIGNAL_IMPLEMENTATION

/* Switches on the LED */
/* Input:   port
 *		:	on_off */
/* Process: Use the GPIO to ON/OFF the LED 
*/
void gphy_data_led_on_off (int port, int on_off)
{
	u32 gpio_pin;

#if defined(GPHY_LED_SIGNAL_IMPLEMENTATION)
	gpio_pin = port;
	
  #if defined(GPHY_LED_USE_SERIAL_LED)
	if (on_off)
		ifx_ledc_set_data(gpio_pin, 0);
	else
		ifx_ledc_set_data(gpio_pin, 1);
  #else
	if (on_off)
		ifx_gpio_output_clear(gpio_pin, IFX_GPIO_MODULE_LED);
	else
		ifx_gpio_output_set(gpio_pin, IFX_GPIO_MODULE_LED);
  #endif
#else
	switch (port)
	{
	case 1:
		gpio_pin = GPHY_1_GPIO;
		break;
	case 2:
		gpio_pin = GPHY_2_GPIO;
		break;
	case 3:
		gpio_pin = GPHY_3_GPIO;
		break;
	case 4:
		gpio_pin = GPHY_4_GPIO;
		break;	
	case 5:
		gpio_pin = GPHY_5_GPIO;
		break;
	}

	if (on_off)
		ifx_gpio_output_set(gpio_pin, IFX_GPIO_MODULE_LED);
	else
		ifx_gpio_output_clear(gpio_pin, IFX_GPIO_MODULE_LED);
#endif
}


/* The LED thread is signalled only if the counters
   are different from previous and
   currently the LED is not ON and currently NOT 
   flashing */

static int gphy_rmon_poll_thread (void *arg) 
{
	int port;
	int port_rx[NUM_OF_PORTS];
	int port_rx_prev[NUM_OF_PORTS] = {0,0,0,0,0,0};
	int port_tx[NUM_OF_PORTS];
	int port_tx_prev[NUM_OF_PORTS] = {0,0,0,0,0,0};
	IFX_ETHSW_RMON_cnt_t param;

	printk (KERN_INFO "start %p ..\n", current);
	allow_signal(SIGKILL);

	while(!kthread_should_stop ()) {
		set_current_state(TASK_INTERRUPTIBLE);

		if (signal_pending(current))
		   break;
		/* Check the RMON counter */
		//printk (KERN_INFO "Checking the RMON counter..!!\n");
		for (port=1; port < NUM_OF_PORTS; port++)
		{
			memset(&param, 0, sizeof(IFX_ETHSW_RMON_cnt_t));
			param.nPortId= port;
			ifx_ethsw_kioctl(swithc_api_fd, IFX_ETHSW_RMON_GET, (unsigned int)&param);

			port_rx[port] = param.nRxGoodPkts;
			port_tx[port] = param.nTxGoodPkts;
			if ((port_rx[port] != port_rx_prev[port]) || (port_tx[port] != port_tx_prev[port]))
			{
				//printk (KERN_INFO "Some packet received on port %d ..!!\n", port);
   				/* Signal the LED thread to blink it */
				gphy_led_state[port] = LED_FLASH;
				port_rx_prev[port] = port_rx[port];
				port_tx_prev[port] = port_tx[port];
			}
			else if (lq_ethsw_mdio_data_read(port, SWITCH_STATUS_REG) & LINK_ON)
			{
				gphy_led_state[port] = LED_ON;
			}
			else
			{
				gphy_led_state[port] = LED_OFF;
			}
			wake_up_interruptible (&gphy_led_thread_wait);
		}

		//printk (KERN_INFO "sleeping !!\n");
		//schedule_timeout (LED_TIMOUT * HZ);
		msleep(100);
	}
}


/* This threads waits for the signals from the RMON thread 
   and flashes the LED */
static int gphy_led_thread (void *arg) 
{
	int port;

	printk (KERN_INFO "start %p ..\n", current);
	allow_signal(SIGKILL);

	for (port=1; port<NUM_OF_PORTS; port++)
	   gphy_led_state[port] = 0; 	/* OFF */

	while(!kthread_should_stop ()) {
		set_current_state(TASK_INTERRUPTIBLE);

		if (signal_pending(current))
			break;

		wait_event_interruptible_timeout (gphy_led_thread_wait, (gphy_any_led_need_to_flash == 1), WAIT_TIMEOUT);

		if (!gphy_led_control_on)
			continue;
		
		//printk (KERN_INFO "gphy_led thread woke up with flash = %d\n", gphy_led_state);

#if defined(GPHY_LED_SIGNAL_IMPLEMENTATION)
#if 1
		zz_gphy_led_state = (gphy_led_state[2] | gphy_led_state[4]);
#else
		zz_gphy_led_state = (gphy_led_state[2] | gphy_led_state[3] | gphy_led_state[4] | gphy_led_state[5]);
#endif

		// zz_gphy_led_state might be
		//	0: LAN1~LAN4 all disconnected
		//	1: LAN1~LAN4 some ports are connected but no traffic
		//  2: LAN1~LAN4 all connected port do traffic
		//	3: LAN1~LAN4 some connected do traffic
		if (zz_gphy_led_state < LED_FLASH)
		{
			gphy_data_led_on_off(GPHY_LED_GPIO, zz_gphy_led_state);
			zz_gphy_led_status_on = zz_gphy_led_state;
		}
		else
		{
			if (zz_gphy_led_status_on == 0)
			{
				zz_gphy_led_status_on = 1;
				gphy_data_led_on_off(GPHY_LED_GPIO, LED_ON);
			}
			else
			{
				zz_gphy_led_status_on = 0;
				gphy_data_led_on_off(GPHY_LED_GPIO, LED_OFF);
			}
		}
#else
		for (port=1; port<NUM_OF_PORTS; port++)
		{
			if (gphy_led_state[port] != LED_FLASH)	/* either ON or OFF */
			{
				gphy_data_led_on_off (port, gphy_led_state[port]);
				gphy_led_status_on[port] = gphy_led_state[port];
			}
			else /* FLASH */
			{
				if (gphy_led_status_on[port] == 0)
				{
					gphy_led_status_on[port] = 1;
					gphy_data_led_on_off (port, LED_ON);
				}
				else
				{
					gphy_led_status_on[port] = 0;
					gphy_data_led_on_off (port, LED_OFF);
				}
			}
		}
#endif
	}
}


#ifdef CONFIG_AR10
int AR10_F2_GPHY_LED_init(void)
{
    /* Create the RMON monitoring and LED flashing threads */
    gphy_led_thread_id = kthread_create(gphy_led_thread, IFX_NULL, "gphy_led_thread");
    gphy_rmon_poll_thread_id = kthread_create(gphy_rmon_poll_thread, IFX_NULL, "gphy_rmon_poll_thread");

    if (!IS_ERR(gphy_led_thread_id)) {
	printk ("GPHY LED poll thread created..\n");
	wake_up_process(gphy_led_thread_id);
    }

    if (!IS_ERR(gphy_rmon_poll_thread_id)) {
	printk ("GPHY RMON poll thread created..\n");
	wake_up_process(gphy_rmon_poll_thread_id);
    }

    init_waitqueue_head(&gphy_led_thread_wait);
}
#endif
#endif /* GPHY_LED_SIGNAL_IMPLEMENTATION */

/* Terry 20140307, reset / reset_released gphy */
/* Possible values.
	0 => GPHY0
	1 => GPHY1
	2 => GPHY0, GPHY1
	3 => GPHY0, GPHY1, GPHY2
*/
#if 1
static int write_proc_reset(struct file *file,const char *buf,int count,void *data )
{
	char lbuf[4] = {0};
	unsigned long gphy_num = 0;
	
	if (count > 3)
		count = 3;
	
	if (count > 0) {
		if (copy_from_user(lbuf, buf, count))
			return -EFAULT;
		gphy_num = simple_strtol(lbuf, NULL, 0);
		ltq_gphy_reset(gphy_num);
	}
	
	return count;
}

static int write_proc_reset_released(struct file *file,const char *buf,int count,void *data )
{
	char lbuf[4] = {0};
	unsigned long gphy_num = 0;
	
	if (count > 3)
		count = 3;
	
	if (count > 0) {
		if (copy_from_user(lbuf, buf, count))
			return -EFAULT;
		gphy_num = simple_strtol(lbuf, NULL, 0);
		ltq_gphy_reset_released(gphy_num);
	}
	
	return count;
}

void create_proc_ltq_gphy_reset()
{
    struct proc_dir_entry *ent = NULL;
	
    ent = create_proc_entry("reset", S_IFREG | S_IRWXU, ifx_gphy_dir);
    if (!ent) {
        printk("create proc failed\n");
    } else {
        ent->write_proc = write_proc_reset;
    }

	ent = create_proc_entry("reset_released", S_IFREG | S_IRWXU, ifx_gphy_dir);
    if (!ent) {
        printk("create proc failed\n");
    } else {
        ent->write_proc = write_proc_reset_released;
    }
}
#endif

/* Terry 20141021, enable / disable gphy led control */
#ifdef GPHY_LED_SIGNAL_IMPLEMENTATION
static int write_proc_led_control(struct file *file,const char *buf,int count,void *data )
{
	char lbuf[4] = {0};
	
	if (count > 3)
		count = 3;
	
	if (count > 0) {
		if (copy_from_user(lbuf, buf, count))
			return -EFAULT;
		gphy_led_control_on = simple_strtol(lbuf, NULL, 0);
	}
	
	return count;
}

void create_proc_ltq_gphy_led_control()
{
    struct proc_dir_entry *ent = NULL;
	
    ent = create_proc_entry("led_control", S_IFREG | S_IRWXU, ifx_gphy_dir);
    if (!ent) {
        printk("create proc failed\n");
    } else {
        ent->write_proc = write_proc_led_control;
    }
	gphy_led_control_on = 1;
}
#endif

/** Initilization GPHY module */
int ifx_phy_module_init (void)
{
	int i;
	dev_id = 0;
	total_gphys = 0;
#ifndef CONFIG_GEFE_MIXED_MODE
	ge_fe_mode = 0;
#else
	gphy_fw_fe_mode = gphy_fw_ge_mode = 0;
#endif

	swithc_api_fd = ifx_ethsw_kopen("/dev/switch_api/0");
	if (swithc_api_fd == 0) {
		printk(KERN_ERR " Open Switch API device FAILED !!\n");
		return -EIO;
	}
#ifndef CONFIG_GEFE_MIXED_MODE
	memset(gphy_fw_dma, 0, sizeof(gphy_fw_dma));
#else
	memset(gphy_fw_fe_dma, 0, sizeof(gphy_fw_fe_dma));
	memset(gphy_fw_ge_dma, 0, sizeof(gphy_fw_ge_dma));
#endif
#if (defined(CONFIG_AR10) || defined(CONFIG_AR10_GRX390))
	{
		unsigned int temp = SW_READ_REG32(IFX_CGU_IF_CLK);
		temp &= ~(0x7<<2);
		temp |= (0x2 << 2);  /*Set 25MHz clock*/
		SW_WRITE_REG32(temp, IFX_CGU_IF_CLK);
	}
#ifdef CONFIG_AR10_GRX390
	total_gphys = 4;
#else
	total_gphys = 3;
#endif /* CONFIG_AR10_GRX390*/
	dev_id = 1;
	printk(KERN_ERR "GPHY FW load for ARX300 !!\n");
#endif  /*CONFIG_AR10/CONFIG_AR10_GRX390 */
#if  defined(CONFIG_HN1) 
	total_gphys = 2;
	dev_id = 0;
	printk(KERN_ERR "GPHY FW load for G.Hn !!\n");
#endif /* CONFIG_HN1 */
#if ( defined(CONFIG_VR9) )
	{
	ifx_chipid_t chipID;
#ifdef CONFIG_FW_LOAD_FROM_USER_SPACE
	memset(proc_gphy_fw_dma, 0, sizeof(proc_gphy_fw_dma));
#endif /* CONFIG_FW_LOAD_FROM_USER_SPACE */
	total_gphys = 2;
	ifx_get_chipid(&chipID);
	if (chipID.family_id == IFX_FAMILY_xRX200) {
		if ( chipID.family_ver == IFX_FAMILY_xRX200_A1x ) {
			printk(KERN_ERR "Init GPHY Driver for A11 !!\n");
			dev_id = 0;
		} else {
			dev_id = 1;
			printk(KERN_ERR "Init GPHY Driver for A12 !!\n");
		}
	}
#if 0
	{
		unsigned int temp = SW_READ_REG32(IFX_CGU_IF_CLK);
		temp &= ~(0x7<<2);
		temp |= (0x2 << 2);  /*Set 25MHz clock*/
		SW_WRITE_REG32(temp, IFX_CGU_IF_CLK);
	}
#endif
	}
#endif /* CONFIG_VR9*/
#if defined(CONFIG_GE_MODE)
	ge_fe_mode = FW_GE_MODE;
#elif defined(CONFIG_FE_MODE)
	ge_fe_mode = FW_FE_MODE;
#else
	gphy_fw_fe_mode = FW_FE_MODE;
	gphy_fw_ge_mode = FW_GE_MODE;
#endif

#ifndef CONFIG_FW_LOAD_FROM_USER_SPACE
#ifndef CONFIG_GEFE_MIXED_MODE
	ltq_gphy_firmware_config(total_gphys, 0, dev_id, ge_fe_mode );
#else
	ltq_gphy_firmware_config(3, 0, dev_id, gphy_fw_fe_mode, gphy_fw_ge_mode );
#endif
#endif /* CONFIG_FW_LOAD_FROM_USER_SPACE */
	if (dev_id == 0) {
		if(ltq_ext_Gphy_hw_Specify_init() == 1)
			printk(KERN_ERR "GPHY driver Specify init FAILED !!\n");
	}
	ltq_ext_gphy_led_config();
	/* Create proc entry */
	gphy_proc_create();
#ifdef CONFIG_GPHY_NIB_ALIN_WORKAROUND
	for (  i  = 0; i < NUM_OF_PORTS; i++) {
		/* reset the global variables */
		gphy_link_status[i] = 0;
		gphy_version_detect[i] = 0;
#if  defined(FSM_DETECT_ERROR)  && FSM_DETECT_ERROR
		gphy_version_nibble_fix[i] = 0;
#endif /* FSM_DETECT_ERROR */
		gphy_link_speed[i]=0;
		gphy_err_detect_cnt[i]=0;
		gphy_ADV_default_reg_value[i] = 0;
	}
	thread_wind_counter = 0;
	thread_interval = 3;
	init_waitqueue_head(&gphy_link_thread_wait);
	gphy_link_thread_id = kthread_create(gphy_link_detect_thread, IFX_NULL, "gphy_TSTAT_thread");
	if (!IS_ERR(gphy_link_thread_id)) {
		/* printk("Created: : %s:%s:%d \n", __FILE__, __FUNCTION__, __LINE__); */
		wake_up_process(gphy_link_thread_id);
	}
#endif /*CONFIG_GPHY_NIB_ALIN_WORKAROUND */
	for (  i  = 0; i < NUM_OF_PORTS; i++) {
		gphy_version_detect[i] = 0;
#if  defined(FSM_DETECT_ERROR)  && FSM_DETECT_ERROR
		gphy_version_nibble_fix[i] = 0;
#endif /* FSM_DETECT_ERROR */
		Get_GPHY_Version(i);
	}
#if  defined(FSM_DETECT_ERROR)  && FSM_DETECT_ERROR
	/* FSM Reset  */
	for (i = 0; i < NUM_OF_PORTS; i++) {
		if ( gphy_version_nibble_fix[i] == 1 ) {
			ltq_gphy_configure_FSM_reset(i, PHY_Address[i]);
			lq_ethsw_mdio_data_write(PHY_Address[i], 0x15, 0x900);
		}
	}
#endif /* FSM_DETECT_ERROR */
	for (  i  = 0; i < NUM_OF_PORTS; i++) {
		if ( gphy_version_detect[i] != 1 ) {
			ltq_gphy_disable_EEE_Advertisement (i, PHY_Address[i]);
		}
	}
#ifdef CONFIG_GSWITCH_HD_IPG_WORKAROUND
	if ( request_irq(IFX_GE_SW_INT, gphy_int_handler, IRQF_DISABLED, "GSWIP_INT_ISR", NULL) ) {
		printk(" ****** Request GSWIP IRQ Failed *********\n");
	}
	SW_WRITE_REG32(0x3F, VR9_MAC_IER_MACIEN); /* enable / disable all MAC interrupts per port */
	for (i=0; i < NUM_OF_PORTS; i++) {
		SW_WRITE_REG32(0x800, VR9_MAC_PIER + (0xC * i)); /* Full Duplex Status */
		SW_WRITE_REG32(0, VR9_MAC_PISR + (0xC * i)); /* Full Duplex Status */
	}
	SW_WRITE_REG32(0x4, VR9_ETHSW_IER); /* Ethernet MAC Interrupt Enable Constants */
#endif /* CONFIG_GSWITCH_HD_IPG_WORKAROUND */
#ifdef GPHY_LED_SIGNAL_IMPLEMENTATION
#ifdef CONFIG_AR10
    AR10_F2_GPHY_LED_init();
#endif
#endif
	
/* Terry 20140307, disable GPHY0 by default */
/* Possible values.
	0 => GPHY0
	1 => GPHY1
	2 => GPHY0, GPHY1
	3 => GPHY0, GPHY1, GPHY2
*/
#if 1
	printk("Disable GPHY on port 0.\n");
	ltq_gphy_reset(3);
	ltq_gphy_reset_released(2); /* Will release GPHY2 once vlan configured in autowandetect agent. */
#endif
	
	return  0;
}

void  ifx_phy_module_exit (void)
{
	ifx_ethsw_kclose(swithc_api_fd); 
	gphy_proc_delete();
	free_irq(IFX_GE_SW_INT, NULL);
}

module_init(ifx_phy_module_init);
module_exit(ifx_phy_module_exit);

MODULE_AUTHOR("Sammy Wu");
MODULE_DESCRIPTION("IFX GPHY driver (Supported LTQ devices)");
MODULE_LICENSE("GPL");
MODULE_VERSION(IFX_DRV_MODULE_VERSION);
