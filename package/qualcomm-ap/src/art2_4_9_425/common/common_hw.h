
#ifndef __INCcommonhwh
#define __INCcommonhwh
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* includes */
#include "wlantype.h"
        /*

#ifdef LINUX
#include "linuxdrv.h"
#define HANDLE A_INT32
#else
#include "ntdrv.h"
#endif
*/
#include "dk_structures.h"
#include "common_hwext.h"
#include "dk_common.h"

#ifdef LINUX
#include "linux_ansi.h"
#endif

#if defined(LINUX) || defined(__APPLE__)
extern char *strupr(char* str);
#endif

#define PCI_SIM_MEM_SIZE (1024*1024)
#define PCI_SIM_MEM_PHY_ADDR 0x80000000
#define PCI_SIM_SHM_SIZE PCI_SIM_MEM_SIZE

/* defines */
#define INTERRUPT_F2    1
#define TIMEOUT         4
#define ISR_INTERRUPT   0x10
#define DEFAULT_TIMEOUT 0xff


#define F2_VENDOR_ID			0x168C		/* vendor ID for our device */
#define MAX_REG_OFFSET			0xfffc	    /* maximum platform register offset */
#define MAX_CFG_OFFSET          256         /* maximum locations for PCI config space per device */
#define MAX_MEMREAD_BYTES       2048*2        /* maximum of 4k location per OSmemRead action */

/* PCI Config space mapping */
#define F2_PCI_CMD				0x04		/* address of F2 PCI config command reg */
#define F2_PCI_BAR0_REG         0x10        
#define F2_PCI_BAR1_REG         0x14
#define F2_PCI_BAR2_REG         0x18        
#define F2_PCI_BAR3_REG         0x20        
#define F2_PCI_CACHELINESIZE    0x0C        /* address of F2 PCI cache line size value */
#define F2_PCI_LATENCYTIMER     0x0D        /* address of F2 PCI Latency Timer value */
#define F2_PCI_BAR				0x10		/* address of F2 PCI config BAR register */
#define F2_PCI_INTLINE          0x3C        /* address of F2 PCI Interrupt Line reg */
/* PCI Config space bitmaps */
#define MEM_ACCESS_ENABLE		0x002       /* bit mask to enable mem access for PCI */
#define MASTER_ENABLE           0x004       /* bit mask to enable bus mastering for PCI */
#define MEM_WRITE_INVALIDATE    0x010       /* bit mask to enable write and invalidate combos for PCI */
#define SYSTEMERROR_ENABLE      0x100		/* bit mask to enable system error */

#define A_MEM_ZERO(addr, len) memset(addr, 0, len)

#ifndef NULL
#define NULL    0
#endif
#ifndef FALSE
#define FALSE   0
#endif
#ifndef TRUE
#define TRUE    1
#endif

#if defined(LINUX) || defined(__APPLE__)
#define Sleep(x) milliSleep(x)
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INCcommonhwh */
