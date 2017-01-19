
#ifndef __INCdkstructuresh
#define __INCdkstructuresh
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* includes */
#include "wlantype.h"

#if defined(Linux) || defined(__APPLE__)
#include "linuxdrv.h"
#endif
#ifdef WIN32
#include "ntdrv.h"
#endif

#ifndef WIN32
#define HANDLE A_INT32
#endif


#define RES_NONE    0
#define RES_INTERRUPT   1
#define RES_MEMORY  2
#define RES_IO  3

#define MAX_BARS  6
#ifdef WLAN_MAX_DEV
#undef WLAN_MAX_DEV
#endif
#define WLAN_MAX_DEV	8		/* Number of maximum supported devices */

/* WLAN_DRIVER_INFO structure will hold the driver global information.
 *
 */

#ifndef AR6000

typedef struct mdk_wlanDrvInfo
{
	A_UINT32           devCount;                     /* No. of currently connected devices */
	struct mdk_wlanDevInfo *pDevInfoArray[WLAN_MAX_DEV]; /* Array of devinfo pointers */
} MDK_WLAN_DRV_INFO;


#else

typedef struct mdk_wlanDrvInfo
{
	A_UINT32           devCount;                     /* No. of currently connected devices */
	struct mdk_wlanDevInfo pDevInfoArray[WLAN_MAX_DEV]; /* Array of devinfo pointers */
} MDK_WLAN_DRV_INFO;

#endif


#ifdef JUNGO 
	//structure to share info with the kernel plugin
	typedef struct dkKernelInfo
	{
		A_BOOL	volatile	anyEvents;		/* set to true by plugin if it has any events */
		A_UINT32	regMemoryTrns;
		A_UINT32	dmaMemoryTrns;
		A_UINT32	f2MapAddress;	    /* address of where f2 registers are mapped */
		A_UINT32	G_memoryRange;
		A_BOOL	volatile	rxOverrun	;	/* set by kernel plugin if it detects a receive overrun */
		A_UINT32		devMapAddress; // address of where f2 registers are mapped 
	} DK_KERNEL_INFO;


	/* holds all the dk specific information within DEV_INFO structure */
	typedef struct dkDevInfo
	{
		//A_UINT32	f2MapAddress;	    /* address of where f2 registers are mapped */
		A_UINT16	f2Mapped;		    /* true if the f2 registers are mapped */
		A_UINT16	instance;	/* used to track which F2 within system this is */
		A_UINT32	dwBus;		/* hold bus slot and function info about device */
		A_UINT32	dwSlot;
		A_UINT32	dwFunction;	
		A_BOOL	    haveEvent;   // set to true when we have an event created
		/* ##note these are jungo specific, should really be OS specific,  move later */	
		WD_CARD_REGISTER G_cardReg; /* holds resource info about pci card */
		WD_CARD_REGISTER pluginMemReg; /* holds info about the memory allocated in plugin */
		A_UINT32 G_baseMemory;
		WD_DMA	dma;				//holds info  about memory for transfers
		A_UINT32 G_resMemory;
		A_UINT32 G_intIndex;
		WD_INTERRUPT gIntrp;
		A_UINT32 volatile intEnabled;

		WD_KERNEL_PLUGIN kernelPlugIn; //handle for kernel plugin
		WD_DMA	kerplugDma;			   //handle to shared mem between plugin and client
		WD_CARD_REGISTER kerplugCardReg;

		/* shared info by kernel plugin */
		DK_KERNEL_INFO *pSharedInfo;    //pointer to structure after alignment
		DK_KERNEL_INFO *pSharedInfoMem; //pointer to the memory we allocated

		A_UINT16	devMapped;
		A_UINT32 dma_mem_addr; // holds the starting addr of the 1 MB memory used for DMA
		A_UINT32	memSize;
	} DK_DEV_INFO;
#elif defined(__APPLE__)
/* holds all the dk specific information within DEV_INFO structure */
typedef struct dkDevInfo {
	A_UINT32	instance;	/* used to track which F2 within system this is */
	A_UINT32	f2Mapped;		    /* true if the f2 registers are mapped */
	A_UINT32	devMapped;		    /* true if the f2 registers are mapped */
	A_UINT32 	f2MapAddress;
	A_UINT_PTR	regVirAddr;
	A_UINT32	regMapRange;
	A_UINT_PTR	memPhyAddr;
	A_UINT_PTR	memVirAddr;
	A_UINT32	memSize;
	A_UINT32		haveEvent;
    A_UINT_PTR    aregPhyAddr[MAX_BARS];
    A_UINT_PTR    aregVirAddr[MAX_BARS];
    A_UINT32    aregRange[MAX_BARS];
    A_UINT32    res_type[MAX_BARS];
    A_UINT32    bar_select;
    A_UINT32    numBars;
    A_UINT32    device_fn;
	A_UINT32		printPciWrites; //set to true when want to print pci reg writes
	A_UINT32	version;
} DK_DEV_INFO;
#else
/* holds all the dk specific information within DEV_INFO structure */
typedef struct dkDevInfo {
	A_UINT32	instance;	/* used to track which F2 within system this is */
	A_UINT32	f2Mapped;		    /* true if the f2 registers are mapped */
	A_UINT32	devMapped;		    /* true if the f2 registers are mapped */
	A_UINT32 	f2MapAddress;
	A_UINT32	regVirAddr;
	A_UINT32	regMapRange;
	A_UINT32	memPhyAddr;
	A_UINT32	memVirAddr;
	A_UINT32	memSize;
	A_UINT32		haveEvent;
    A_UINT32    aregPhyAddr[MAX_BARS];
    A_UINT32    aregVirAddr[MAX_BARS];
    A_UINT32    aregRange[MAX_BARS];
    A_UINT32    res_type[MAX_BARS];
    A_UINT32    bar_select;
    A_UINT32    numBars;
    A_UINT32    device_fn;
	A_UINT32		printPciWrites; //set to true when want to print pci reg writes
	A_UINT32	version;
} DK_DEV_INFO;
#endif // JUNGO

/*
 * MDK_WLAN_DEV_INFO structure will hold all kinds of device related information.
 * It will hold OS specific information about the device and a device number.
 * Most of the code doesn't need to know what's inside that structure.
 * The fields are very likely to change.
 */

typedef	struct	mdk_wlanDevInfo
{
	struct dkDevInfo *pdkInfo;    /* pointer to structure containing info for dk */
#ifdef LEGACY
	A_UINT32	cliId;
#else
#ifdef SIM
    A_UINT16 idSelect;
#else
	HANDLE hDevice;
#endif
#endif
	A_UCHAR	   *pbuffMapBytes;      /* holds bit maps for descriptors allocated */
    A_UINT16   *pnumBuffs;          /* holds number of buffers allocated by each index */
} MDK_WLAN_DEV_INFO;


// extern variable declarations
extern MDK_WLAN_DRV_INFO	globDrvInfo; 

typedef struct cliinfo_{
   A_UINT_PTR regPhyAddr; // retain this for backward compatibility
   A_UINT_PTR regVirAddr; // retain this for backward compatibility
   A_UINT_PTR memPhyAddr;
   A_UINT_PTR memVirAddr;
   A_UINT32 irqLevel; 
   A_UINT32 regRange; // retain this for backward compatibility
   A_UINT32 memSize;
   A_UINT_PTR aregPhyAddr[MAX_BARS];
   A_UINT_PTR aregVirAddr[MAX_BARS];
   A_UINT32 aregRange[MAX_BARS];
   A_UINT32 numBars;
   A_UINT32 res_type[MAX_BARS];
   A_UINT32 dma_mem_addr;
}CLI_INFO, *PCLI_INFO;

typedef struct versionInfo_ {
    A_UINT32 majorVersion;
    A_UINT32 minorVersion;
} DRV_VERSION_INFO, *PDRV_VERSION_INFO;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INCdkstructuresh */

