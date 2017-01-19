/*
 * This MTD supports CFI flash devices:
 *	- BCS or SCS parts such as Intel.
 *	- AMD/FJ parts such as Toshiba.
 *
 * For the AR531X flash design we split the flash into 3 sections:
 *	0		bootrom
 *	ROM_SIZE	tffs area
 *	N-3 sectors	configuration area
 *
 * This is optimized for a top boot block part so small sectors can
 * be used configuration - boot line, radio "eeprom" configuration,
 * and board configuration.  It is anticipated that the board and
 * radio sectors will be write protected after programming.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <asm/delay.h>
#include <asm/io.h>
#include "dk.h"
#include "dk_pci_bus.h"
#include "client.h"
#include "dk_flash.h"
#include  "flbase.h"
#include  "flflash.h"


#define NFLC            3 
#define FLC_BOOTLINE    0 
#define FLC_BOARDDATA   1 
#define FLC_RADIOCFG    2
#define INCLUDE_CFI_AMDFJ               /* AMD/FJ part for production */



/* CFI commands */
#define CFI_QUERY			0x98

/* BCS/SCS commands */
#define SCS_CONFIRM_SET_LOCKBIT		0x01
#define SCS_SETUP_BLOCK_ERASE		0x20
#define SCS_SETUP_QUEUE_ERASE		0x28
#define SCS_SETUP_CHIP_ERASE		0x30
#define SCS_CLEAR_STATUS		0x50
#define SCS_SET_LOCKBIT			0x60
#define SCS_CLEAR_LOCKBIT		0x60
#define SCS_READ_STATUS			0x70
#define SCS_READ_ID			0x90
#define SCS_SUSPEND_WRITE		0xb0
#define SCS_SUSPEND_ERASE		0xb0
#define SCS_CONFIG			0xb8
#define SCS_CONFIRM_WRITE		0xd0
#define SCS_RESUME_WRITE		0xd0
#define SCS_CONFIRM_ERASE		0xd0
#define SCS_RESUME_ERASE		0xd0
#define SCS_CONFIRM_CLEAR_LOCKBIT	0xd0
#define SCS_WRITE_TO_BUFFER		0xe8
#define SCS_READ_ARRAY			0xff

#define SCS_WSM_ERROR		0x3a	/* BCS/SCS status register bits */
#define SCS_SR_BLOCK_LOCK	0x02
#define SCS_SR_WRITE_SUSPEND	0x04
#define SCS_SR_VPP_ERROR	0x08
#define SCS_SR_WRITE_ERROR	0x10
#define SCS_SR_LOCK_SET_ERROR	0x10
#define SCS_SR_ERASE_ERROR	0x20
#define SCS_SR_LOCK_RESET_ERROR	0x20
#define SCS_SR_ERASE_SUSPEND	0x40
#define SCS_SR_READY		0x80

/* AMD commands */
#define AMD_SETUP_ERASE		0x80
#define AMD_SETUP_WRITE		0xa0
#define AMD_READ_ID 		0x90
#define AMD_SUSPEND_ERASE	0xb0
#define AMD_SECTOR_ERASE	0x30
#define AMD_RESUME_ERASE	0x30
#define AMD_RESET_ARRAY		0xf0

/*
 * SST flash distinguishes between SECTOR_ERASE (0x1000 bytes)
 * and BLOCK_ERASE (0x10000 bytes).
 */
#define SST_BLOCK_ERASE         0x50

#define	AMD_UNLOCK_1		0xaa
#define AMD_UNLOCK_2		0x55

#define AMD_UNLOCK_ADDR1	0x555
#define	AMD_UNLOCK_ADDR2	0x2aa

#define	AMD_D2			4	/* Toggles when erase suspended */
#define AMD_D5			0x20	/* Set when programming timeout */
#define	AMD_D6			0x40	/* Toggles when programming */

/* CFI command set IDs */
#define INTEL_COMMAND_SET	0x0001
#define AMDFUJ_COMMAND_SET	0x0002

/* CFI identification strings */
#define ID_STR_LENGTH		3
#define QUERY_ID_STR		"QRY"
#define PRIMARY_ID_STR		"PRI"
#define ALTERNATE_ID_STR	"ALT"

/* optional commands support */
#define CHIP_ERASE_SUPPORT	0x0001
#define SUSPEND_ERASE_SUPPORT	0x0002
#define SUSPEND_WRITE_SUPPORT	0x0004
#define LOCK_SUPPORT		0x0008
#define QUEUED_ERASE_SUPPORT	0x0010

#ifndef DEBUG_PRINT
#define DEBUG_PRINT(args)
#endif

/*
 * Indicates that we're using an SST flash part (39VF160).
 * This part uses an AMD/Fujitsu flash command set, but with
 * different addresses than usual (0x5555 rather than 0x555,
 * and 0x2aaa rather than 0x2aa).
 */
static BOOLEAN SSTflash;

/*
 * Structure to hold flash parameters.  Stores additional data for
 * sysFlash interface.
 */
struct cfi {
    UINT8 *base;	        /* base of config flash */
    UINT16 type;		/* type of flash device */
    UINT32 size;		/* size of flash device */
    UINT32 blockSize;		/* erase block size for tffs */
    UINT32 cap;			/* size CAP for tffs */
    UINT16 wb;			/* write buffer depth */
    UINT16 cmdSet;		/* Intel or AMD */
    UINT32 eraseTimeoutMs;	/* Max erase timeout in milliseconds */
    UINT32 writeTimeoutUs;	/* Max write timeout in microseconds */
    BOOLEAN vpp;		/* true if need vpp */
    BOOLEAN x16;		/* width of device */
    BOOLEAN ok;			/* sysFlashConfigInit status */
    BOOLEAN top;		/* top boot block part */
    UINT8 *flcPtr[NFLC];	/* segment base address */
    int flcSize[NFLC];		/* segment size */
    int flcReq[NFLC];		/* segment required size */
    FLStatus (*write)(FlashPTR flashPtr, int offset, UINT8 *p, int len);
    FLStatus (*erase)(FlashPTR flashPtr);
    INT8 readCmd;		/* READ_ARRAY command */
    UINT32 eBlocks;             /* cache of erase block layout */
#define MAX_EBLOCKS 8
    UINT32 ebSize[MAX_EBLOCKS];
    UINT32 ebNum[MAX_EBLOCKS];
} cfi;

#define QRY(FP, mul) \
	(((char)FP[0x10*(mul)] == 'Q') && \
	 ((char)FP[0x11*(mul)] == 'R') && \
	 ((char)FP[0x12*(mul)] == 'Y'))

/*
 * Mutual exclusion for tffs vs config and boot rom update.
 *
 * On flash mode changes (erase and write) we need to prevent
 * mixed access to the flash device.  Reads are also special
 * as tffs reads are done outside the driver, so boot accesses
 * also prevent rescheduling for the duration of their access.
 *
 * Boot changes are infrequent and usually quick for config
 * updates, so this is not a major issue.
 *
 * It is ok for the sema allocation to not take place.  This
 * can happen during boot rom start-up.  In this environment
 * there are no races anyway.
 */
static int *flSem = NULL;
#define FL_LOCK_BOOT	0x01
#define FL_LOCK_WRITE	0x02
static int
cfiLock(int mode)
{
    int rc = 0;

    if (flSem == NULL) {
        //flSem = semBCreate(SEM_Q_PRIORITY, SEM_FULL);
    }
    if (flSem != NULL) {
        //semTake(flSem, WAIT_FOREVER);
        if (mode & FL_LOCK_BOOT) {
            //taskLock();
        }
    }

    if (mode & FL_LOCK_WRITE) {
//        rc = sysRegRead(AR531X_FLASHCTL0);
//        sysRegWrite(AR531X_FLASHCTL0, rc & ~FLASHCTL_WP);
    }

    return rc;
}

static void
cfiUnlock(int mode, int cookie)
{
    if (mode & FL_LOCK_WRITE) {
//        sysRegWrite(AR531X_FLASHCTL0, cookie);
    }

    if (flSem) {
        //semGive(flSem);
        if (mode & FL_LOCK_BOOT) {
            //taskUnlock();
        }
    }
}

#ifdef INCLUDE_MTD_CFISCS

#include "backgrnd.h"

#define thisCFI   ((struct cfi *)vol.mtdVars)

/*
 * cfiWrite - MTD write interface for all flash types.
 */
static FLStatus
cfiWrite(FLFlash vol, CardAddress address, const void *buffer,
         int length, int mode)
{
    FLStatus status = flOK;
    int from, eachWrite;
    FlashPTR flashPtr;
    int s;

    DEBUG_PRINT(("Debug: write 0x%x for %d.\n", (int)address, length));

    /* Only write words on word-boundary for x16 devices */
    if (thisCFI->x16 && ((length & 1) || (address & 1))) {
        return flBadParameter;
    }

    if (flWriteProtected(vol.socket)) {
        return flWriteProtect;
    }

#ifdef SOCKET_12_VOLTS
    if (thisCFI->vpp) {
        checkStatus(flNeedVpp(vol.socket));
    }
#endif

    s = cfiLock(FL_LOCK_WRITE);

    eachWrite = thisCFI->wb;

    for (from = 0; from < length && status == flOK; from += eachWrite) {
        flashPtr = (FlashPTR)flMap(vol.socket, address + from);
        status = thisCFI->write(flashPtr, 0,
                                ((char *)buffer) + from,
                                min(length - from, eachWrite));
    }

#ifdef SOCKET_12_VOLTS
    if (thisCFI->vpp) {
        flDontNeedVpp(vol.socket);
    }
#endif

    flashPtr = (FlashPTR) flMap(vol.socket, address);
    /* verify the data */
    if (status == flOK && tffscmp((void *)flashPtr, buffer, length)) {
        printk("cfiscs: write failed in verification @ %p.\n",
               (void *)flashPtr);
        status = flWriteFault;
    }

    cfiUnlock(FL_LOCK_WRITE, s);

    return status;
}

/*
 * cfiErase - MTD erase wrapper.
 */
static FLStatus
cfiErase(FLFlash vol, int firstErasableBlock, int numOfErasableBlocks)
{
    FLStatus status = flOK;       /* unless proven otherwise */
    FlashPTR flashPtr;
    int blk;
    int s;

    DEBUG_PRINT(("Debug: erase 0x%lx %d blocks.\n",
                 firstErasableBlock  * vol.erasableBlockSize,
                 numOfErasableBlocks));

    if (flWriteProtected(vol.socket)) {
        return flWriteProtect;
    }

#ifdef SOCKET_12_VOLTS
    if (thisCFI->vpp) {
        checkStatus(flNeedVpp(vol.socket));
    }
#endif

    s = cfiLock(FL_LOCK_WRITE);

    for (blk = 0; blk < numOfErasableBlocks && status == flOK; blk++) {
        flashPtr = (FlashPTR)vol.map(&vol,
                                     (firstErasableBlock + blk) * vol.erasableBlockSize,
                                     vol.interleaving);

        status = thisCFI->erase(flashPtr);
    }

    cfiUnlock(FL_LOCK_WRITE, s);

#ifdef SOCKET_12_VOLTS
    if (thisCFI->vpp) {
        flDontNeedVpp(vol.socket);
    }
#endif

    return status;
}

/*
 * cfiscsIdentify - Identifies media based on CFI and registers as an
 *                  MTD for such.
 *
 * This routine will be placed on the MTD list in custom.h.  It must be
 * an extern routine.
 *
 * On successful identification, the Flash structure is filled out and
 * the write and erase routines registered.
 */
FLStatus
cfiscsIdentify(FLFlash vol)
{
    int s;

    DEBUG_PRINT(("Debug: entering CFISCS identification routine.\n"));

    flSetWindowSpeed(vol.socket, 120);    /* 120 nsec. */
    flSetWindowSize(vol.socket, 2);       /* 8 KBytes */

    /*
     * Interleaving probing support has been removed for
     * simplicity and to allow the boot and tffs interfaces
     * to share code.
     */
    vol.interleaving = 1;
    vol.mtdVars = &cfi;
    vol.noOfChips = 1;

    if (thisCFI->ok == FALSE) {
        DEBUG_PRINT(("Debug: device not ok.\n"));
        return flUnknownMedia;
    }

    s = cfiLock(FL_LOCK_WRITE);

    flSetWindowBusWidth(vol.socket, thisCFI->x16 ? 16 : 8);

    vol.write = cfiWrite;
    vol.erase = cfiErase;
    vol.type = thisCFI->type;
    vol.erasableBlockSize = thisCFI->blockSize;
    vol.chipSize = thisCFI->cap;

    cfiUnlock(FL_LOCK_WRITE, s);
    DEBUG_PRINT(("Debug: ok.\n"));

    return flOK;
}
#endif /* INCLUDE_MTD_CFISCS */

#ifdef INCLUDE_CFI_SCS
/*
 * cfiscsBlockErase_x8 - Erase the block pointed to by flashPtr.
 */
static FLStatus
cfiscsBlockErase_x8(FlashPTR flashPtr)
{
    FLStatus status = flOK;       /* unless proven otherwise */
    int maxTo = cfi.eraseTimeoutMs * 10;
    FLBoolean finished;
    int to = 0;

    /* Intel part does not have queued erase support */
    flashPtr[0] = SCS_SETUP_BLOCK_ERASE;
    flashPtr[0] = SCS_CONFIRM_ERASE;

    do {
        finished = TRUE;
        flashPtr[0] = SCS_READ_STATUS;
        udelay(100);			/* .1ms */
        if (!(flashPtr[0] & SCS_SR_READY)) {
            finished = FALSE;
        } else if (flashPtr[0] & SCS_SR_ERASE_SUSPEND) {
            flashPtr[0] = SCS_RESUME_ERASE;
            finished = FALSE;
        } else if (to++ > maxTo) {
            status = flTimedOut;
            finished = TRUE;
        } else {
            if (flashPtr[0] & SCS_WSM_ERROR) {
                DEBUG_PRINT(("Debug: CFISCS erase error.\n"));
                status = (flashPtr[0] & SCS_SR_VPP_ERROR) ?
                    flVppFailure : flWriteFault;
                flashPtr[0] = SCS_CLEAR_STATUS;
            }
        }
    } while (!finished);

    flashPtr[0] = SCS_READ_ARRAY;

    return status;
}

/*
 * cfiscsBlockErase_x16 - Erase the block pointed to by flashPtr.
 */
static FLStatus
cfiscsBlockErase_x16(FlashPTR bytePtr)
{
    FlashWPTR flashPtr = (FlashWPTR)bytePtr;
    FLStatus status = flOK;       /* unless proven otherwise */
    int maxTo = cfi.eraseTimeoutMs * 10;
    FLBoolean finished;
    int to = 0;

    /* Intel part does not have queued erase support */
    flashPtr[0] = SCS_SETUP_BLOCK_ERASE; /* FlashPtr points at block */
    flashPtr[0] = SCS_CONFIRM_ERASE;

    do {
        finished = TRUE;
        flashPtr[0] = SCS_READ_STATUS;
        udelay(100);		/* .1ms */
        if (!(flashPtr[0] & SCS_SR_READY)) {
            finished = FALSE;
        } else if (flashPtr[0] & SCS_SR_ERASE_SUSPEND) {
            flashPtr[0] = SCS_RESUME_ERASE;
            finished = FALSE;
        } else if (to++ > maxTo) {
            status = flTimedOut;
            finished = TRUE;
        } else {
            if (flashPtr[0] & SCS_WSM_ERROR) {
                DEBUG_PRINT(("Debug: CFISCS erase error.\n"));
                status = (flashPtr[0] & SCS_SR_VPP_ERROR) ?
                    flVppFailure : flWriteFault;
                flashPtr[0] = SCS_CLEAR_STATUS;
            }
        }
    } while (!finished);

    flashPtr[0] = SCS_READ_ARRAY;

    return status;
}

/*
 * cfiscsBufferWrite_x8 - write len bytes from p at flashPtr.
 */
static FLStatus
cfiscsBufferWrite_x8(FlashPTR flashPtr, int offset, UINT8 *p, int len)
{
    FLStatus status = flOK;
    int i, to;

    to = 0;
    do {
        *flashPtr = SCS_WRITE_TO_BUFFER;
        udelay(20);
    } while (!(*flashPtr & SCS_SR_READY) && (to++ < 1000));
    if (!(*flashPtr & SCS_SR_READY)) {
        printk("cfiscs: timeout error for WRITE_TO_BUFFER @ %p.\n",
               flashPtr);
        status = flTimedOut;
    }

    *flashPtr = len - 1;		/* 0 == 1 byte */

    /* tffscpy() */
    for (i = 0; i < len; i++) {
        flashPtr[offset+i] = p[i];
    }

    *flashPtr = SCS_CONFIRM_WRITE;

    for (to=0; !(*flashPtr & SCS_SR_READY) && (to < cfi.writeTimeoutUs); to++) {
        udelay(1);
    }
    if (!(*flashPtr & SCS_SR_READY)) {
        printk("cfiscs: timeout error for CONFIRM_WRITE @ %p.\n",
               flashPtr);
        status = flTimedOut;
    }
    if (*flashPtr & SCS_WSM_ERROR) {
        printk("cfiscs: WSM_ERROR @ %p.\n", flashPtr);
        status = (*flashPtr & SCS_SR_VPP_ERROR) ? flVppFailure : flWriteFault;
        *flashPtr = SCS_CLEAR_STATUS;
    }

    *flashPtr = SCS_READ_ARRAY;

    return status;
}

/*
 * cfiscsBufferWrite_x16 - write len bytes from p at flashPtr.  Interface
 * is in bytes, not words.
 */
static FLStatus
cfiscsBufferWrite_x16(FlashPTR bytePtr, int offset, UINT8 *p, int len)
{
    FlashWPTR flashPtr = (FlashWPTR)bytePtr;
    FLStatus status = flOK;
    int i, idx, to;
    UINT16 val;
    int pad;

    to = 0;
    do {
        *flashPtr = SCS_WRITE_TO_BUFFER;
        udelay(20);
    } while (!(*flashPtr & SCS_SR_READY) && (to++ < 1000));
    if (!(*flashPtr & SCS_SR_READY)) {
        printk("cfiscs: timeout error for WRITE_TO_BUFFER @ %p.\n",
               flashPtr);
        status = flTimedOut;
    }

    /* handle pad byte at start and/or end.  Caller accomidates this */
    pad = (offset & 1) + ((offset & 1) ^ (len & 1));
    *flashPtr = ((len + pad) >> 1) - 1;		/* 0 == 1 word */

    /* tffscpy() */
    for (i = 0; i < len; ) {
        idx = (offset + i) >> 1;
        if (((offset+i) & 1) == 0) {
            val = p[i++] << 8;
            val |= (i < len) ? (p[i++] & 0xff) : 0x00ff;
        } else {
            val = (p[i++] & 0xff) | 0xff00;
        }
        flashPtr[idx] = val;
    }

    *flashPtr = SCS_CONFIRM_WRITE;

    for (to=0; !(*flashPtr & SCS_SR_READY) && (to < cfi.writeTimeoutUs); to++) {
        udelay(1);
    }
    if (!(*flashPtr & SCS_SR_READY)) {
        printk("cfiscs: timeout error for CONFIRM_WRITE @ %p.\n",
               flashPtr);
        status = flTimedOut;
    }
    if (*flashPtr & SCS_WSM_ERROR) {
        printk("cfiscs: WSM_ERROR @ %p.\n", flashPtr);
        status = (*flashPtr & SCS_SR_VPP_ERROR) ? flVppFailure : flWriteFault;
        *flashPtr = SCS_CLEAR_STATUS;
    }

    *flashPtr = SCS_READ_ARRAY;

    return status;
}
#endif  /* INCLUDE_MTD_CFISCS */

#ifdef INCLUDE_CFI_AMDFJ
/*
 * cfiamdBlockErase_x8 - Erase the block pointed to by flashPtr.
 */
#ifdef UNUSED
static FLStatus
cfiamdBlockErase_x8(FlashPTR flashPtr)
{
    int maxTo = cfi.eraseTimeoutMs * 10;
    int to = 0;

    cfi.base[0xaaa] = AMD_UNLOCK_1;
    cfi.base[0x555] = AMD_UNLOCK_2;
    cfi.base[0xaaa] = AMD_SETUP_ERASE;
    cfi.base[0xaaa] = AMD_UNLOCK_1;
    cfi.base[0x555] = AMD_UNLOCK_2;
    flashPtr[0] = AMD_SECTOR_ERASE;

    while (1) {
        udelay(100);			/* .1ms */
        if (((flashPtr[0] ^ flashPtr[0]) & AMD_D6) == 0) {
            break;
        }
        if (to++ > maxTo) {
            printk("cfiamd: erase timeout @ %p\n", (void *)flashPtr);
            *flashPtr = AMD_RESET_ARRAY;
            return flWriteFault;
        }
    }
    if (flashPtr[0] != 0xff) {
        if (flashPtr[0] & AMD_D5) {
            printk("cfiamd: D5 erase timeout @ %p\n", (void *)flashPtr);
        } else {
            printk("cfiamd: erase mismatch @ %p\n", (void *)flashPtr);
        }
        *flashPtr = AMD_RESET_ARRAY;
        return flWriteFault;
    }

    return flOK;
}
#endif
/*
 * cfiamdBlockErase_x16 - Erase the block pointed to by flashPtr.
 */
static FLStatus
cfiamdBlockErase_x16(FlashPTR bytePtr)
{
    FlashWPTR flashBase = (FlashWPTR)cfi.base;
    FlashWPTR flashPtr = (FlashWPTR)bytePtr;
    int maxTo = cfi.eraseTimeoutMs * 10;
    int to = 0;

    if (SSTflash) {
        flashBase[0x5555] = AMD_UNLOCK_1;
        flashBase[0x2aaa] = AMD_UNLOCK_2;
        flashBase[0x5555] = AMD_SETUP_ERASE;
        flashBase[0x5555] = AMD_UNLOCK_1;
        flashBase[0x2aaa] = AMD_UNLOCK_2;
        flashPtr[0] = SST_BLOCK_ERASE;
    } else {
        flashBase[0x555] = AMD_UNLOCK_1;
        flashBase[0x2aa] = AMD_UNLOCK_2;
        flashBase[0x555] = AMD_SETUP_ERASE;
        flashBase[0x555] = AMD_UNLOCK_1;
        flashBase[0x2aa] = AMD_UNLOCK_2;
        flashPtr[0] = AMD_SECTOR_ERASE;
    }

    while (1) {
        udelay(100);			/* 0.1ms */
        if (((flashPtr[0] ^ flashPtr[0]) & AMD_D6) == 0) {
            break;
        }
        if (to++ > maxTo) {
            printk("cfiamd: erase timeout @ %p\n", (void *)flashPtr);
            *flashPtr = AMD_RESET_ARRAY;
            return flWriteFault;
        }
    }
    if (flashPtr[0] != 0xffff) {
        if (flashPtr[0] & AMD_D5) {
            printk("cfiamd: D5 erase timeout @ %p\n", (void *)flashPtr);
        } else {
            printk("cfiamd: erase mismatch @ %p\n", (void *)flashPtr);
        }
        *flashPtr = AMD_RESET_ARRAY;
        return flWriteFault;
    }

    return flOK;
}

/*
 * cfiamdBufferWrite_x8 - write len bytes from p at flashPtr.
 */
#ifdef UNUSED
static FLStatus
cfiamdBufferWrite_x8(FlashPTR flashPtr, int offset, UINT8 *p, int len)
{
    FlashPTR flashBase = (FlashPTR)cfi.base;
    int maxTo = cfi.writeTimeoutUs;
    int i, to;

    for (i = 0; i < len; i++) {
        flashBase[0xaaa] = AMD_UNLOCK_1;
        flashBase[0x555] = AMD_UNLOCK_2;
        flashBase[0xaaa] = AMD_SETUP_WRITE;   
        flashPtr[offset+i] = p[i];

        to = 0;
        while (1) {
            udelay(1);
            if (((flashPtr[offset+i] ^ flashPtr[offset+i]) & AMD_D6) == 0) {
                break;
            }
            if (to++ > maxTo) {
                printk("cfiamd: write timeout @ %p\n", (void *)flashPtr+offset+i);
                *flashPtr = AMD_RESET_ARRAY;
            }
        }

        if (flashPtr[offset+i] != p[i]) {
            printk("cfiamd: write %s @ %p (%x != %x)\n",
                   (flashPtr[offset+i] & AMD_D5) ? "D5 timeout" : "mismatch",
                   (void *)flashPtr+offset+i, flashPtr[offset+i], p[i]);
            *flashPtr = AMD_RESET_ARRAY;
            return flWriteFault;
        }
    }

    return flOK;
}
#endif
/*
 * cfiamdBufferWrite_x16 - write len bytes from p at flashPtr.  Interface
 * is in bytes, not words.
 */
static FLStatus
cfiamdBufferWrite_x16(FlashPTR bytePtr, int offset, UINT8 *p, int len)
{
    FlashWPTR flashBase = (FlashWPTR)cfi.base;
    FlashWPTR flashPtr = (FlashWPTR)bytePtr;
    int maxTo = cfi.writeTimeoutUs;
    int i, idx, to;
    UINT16 val;

    for (i = 0; i < len; ) {

        idx = (offset + i) >> 1;
        if (((offset+i) & 1) == 0) {
            val = p[i++] << 8;
            val |= (i < len) ? (p[i++] & 0xff) : (flashPtr[idx] & 0xff);
        } else {
            val = (p[i++] & 0xff) | (flashPtr[idx] & 0xff00);
        }
        if (SSTflash) {
            flashBase[0x5555] = AMD_UNLOCK_1;
            flashBase[0x2aaa] = AMD_UNLOCK_2;
            flashBase[0x5555] = AMD_SETUP_WRITE;
        } else {
            flashBase[0x555] = AMD_UNLOCK_1;
            flashBase[0x2aa] = AMD_UNLOCK_2;
            flashBase[0x555] = AMD_SETUP_WRITE;
        }
        flashPtr[idx] = val;

        to = 0;
        while (1) {
            udelay(1);
            if (((flashPtr[idx] ^ flashPtr[idx]) & AMD_D6) == 0) {
                break;
            }
            if (to++ > maxTo) {
                printk("cfiamd: write timeout @ %p\n", (void *)flashPtr+offset+i);
                *flashPtr = AMD_RESET_ARRAY;
            }
        }
        if (flashPtr[idx] != val) {
            printk("cfiamd: write %s @ %p (%x != %x)\n",
                   (flashPtr[idx] & AMD_D5) ? "D5 timeout" : "mismatch",
                   (void *)flashPtr+offset+i, flashPtr[offset+i], val);
            *flashPtr = AMD_RESET_ARRAY;
            return flWriteFault;
        }
    }

    return flOK;
}

/*
 * EEPROM on flash emulation low level support.  Uses common low level
 * code and locking to avoid boot vs tffs contention.
 *
 * The last NFLC flash sector are reserved for various
 * configuration items that do not belong in the flash filesystem
 * as they are low level configuration (bootline), or wish to
 * use the locking features of the flash device (board config).
 *
 * A top boot block part is prefered as it will waste less space
 * as these configuration items are saved at the end of the device.
 */
#endif
#ifdef UNUSED
static BOOLEAN
sysFlashConfigInit_x8(FlashPTR flashPtr)
{
    int i, j, off, nb, sz;
    char *base;

    cfi.size = 0;

    /* Query the flash device */
    flashPtr[0] = SCS_READ_ARRAY;
    flashPtr[0x55*2] = CFI_QUERY;

    /* Some boards have a x8 AMD part stuffed -- skip it */
    if (!QRY(flashPtr, 2)) {
        return FALSE;
    }

    cfi.cmdSet = flashPtr[0x13*2] | ((unsigned)flashPtr[0x14*2] << 8);
#ifdef INCLUDE_CFI_SCS
    if (cfi.cmdSet == INTEL_COMMAND_SET) {
        cfi.write = cfiscsBufferWrite_x8;
        cfi.erase = cfiscsBlockErase_x8;
        cfi.readCmd = SCS_READ_ARRAY;
    }
#endif
#ifdef INCLUDE_CFI_AMDFJ
    if (cfi.cmdSet == AMDFUJ_COMMAND_SET) {
        cfi.write = cfiamdBufferWrite_x8;
        cfi.erase = cfiamdBlockErase_x8;
        cfi.readCmd = AMD_RESET_ARRAY;
    }
#endif
    if (cfi.write == NULL) {
        return FALSE;
    }

    cfi.size = 1L << flashPtr[0x27*2];
    cfi.type = (FlashType) ((flashPtr[0] << 8) | flashPtr[0x1*2]);
    cfi.vpp = (flashPtr[0x1d*2] != 0) ? TRUE : FALSE;
    cfi.wb = 1L << (flashPtr[0x2a*2] | ((unsigned)flashPtr[0x2b*2] << 8));

    cfi.eraseTimeoutMs = (1 << flashPtr[0x21*2]) * (1 << flashPtr[0x25*2]);
    if (cfi.wb == 1) {
        /* single write */
        cfi.writeTimeoutUs = (1 << flashPtr[0x1f*2]) * (1 << flashPtr[0x23*2]);
    } else {
        /* buffer write */
        cfi.writeTimeoutUs = (1 << flashPtr[0x20*2]) * (1 << flashPtr[0x24*2]);
    }

    if (cfi.cmdSet == AMDFUJ_COMMAND_SET) {
        /* check for top part */
        off = flashPtr[0x15*2] | ((unsigned)flashPtr[0x16*2]<<8);
        cfi.top = (flashPtr[(off+0xf)*2] == 3) ? TRUE : FALSE;
    }

    cfi.blockSize = 0;
    cfi.eBlocks = flashPtr[0x2c*2];
    off = 0x2d;
    if (cfi.top) {
        for (i=cfi.eBlocks-1; i >= 0; i--) {
            cfi.ebNum[i] = (flashPtr[off*2] |
                            ((unsigned)flashPtr[(off+1)*2]) << 8) + 1;
            cfi.ebSize[i] = (flashPtr[(off+2)*2] |
                             ((unsigned)flashPtr[(off+3)*2]) << 8) * 0x100;
            if (cfi.ebSize[i] > cfi.blockSize) {
                cfi.blockSize = cfi.ebSize[i];
            }
            off += 4;
        }
    } else {
        for (i=0; i < cfi.eBlocks; i++) {
            cfi.ebNum[i] = (flashPtr[off*2] |
                            ((unsigned)flashPtr[(off+1)*2]) << 8) + 1;
            cfi.ebSize[i] = (flashPtr[(off+2)*2] |
                             ((unsigned)flashPtr[(off+3)*2]) << 8) * 0x100;
            if (cfi.ebSize[i] > cfi.blockSize) {
                cfi.blockSize = cfi.ebSize[i];
            }
            off += 4;
        }
    }
    base = cfi.base + cfi.size;
    off = cfi.eBlocks - 1;
    i = NFLC - 1;
    while (i >= 0) {
        nb = cfi.ebNum[off];
        //sz = cfi.ebSize[off];  // Siva
        sz = (DK_BOARDCONFIG_SIZE > DK_RADIOCONFIG_SIZE)?DK_BOARDCONFIG_SIZE:DK_RADIOCONFIG_SIZE;
        for (j=0; (j < nb) && (i >= 0); j++) {
            base -= sz;
            cfi.flcPtr[i] = base;
            cfi.flcSize[i] = sz;
            i--;
        }
        off--;
    }

    flashPtr[0] = cfi.readCmd;

    return TRUE;
}
#endif
/*
 * We support two different varieties of flash parts with slightly
 * different command sets (Intel versus AMD).  If a CFI QRY fails
 * with the Intel command set, we try AMD.
 *
 * AR5311 flash parts all use Intel command set.
 * AR5312 flash may use an SST flash part, which
 * uses the AMD/Fujitsu command set at different addresses.
 * We determine which part we have dynamically by trying
 * several QRY commands and seeing which one works.
 */
static BOOLEAN
sysFlashConfigInit_x16(FlashWPTR ptr)
{
    int i, off;
    char *base;
    FlashWPTR flashPtr;
    char * basePtr;

    cfi.size = 0;
    cfi.base = (UINT8 *)(ioremap((phys_t)ptr, 0x10000));
    flashPtr = (FlashWPTR)(cfi.base);

    /* Query the flash device */
    flashPtr[0] = SCS_READ_ARRAY;
    flashPtr[0x55] = CFI_QUERY;

printk("ptr is 0x%x\n", (unsigned int)ptr);
printk("flashPtr is 0x%x\n", (unsigned int)flashPtr);

    /* look for the query identification string "QRY" */
    if (!QRY(flashPtr, 1)) {
        flashPtr[0] = SCS_READ_ARRAY;
        flashPtr[0x5555] = 0xaa;
        flashPtr[0x2aaa] = 0x55;
        flashPtr[0x5555] = CFI_QUERY;
        if (!QRY(flashPtr, 1)) {
            return FALSE;
        } else {
            /* Force SST device to look like AMD */
            cfi.cmdSet = AMDFUJ_COMMAND_SET;
            SSTflash = TRUE;
        }
    } else {
        cfi.cmdSet = flashPtr[0x13] | (flashPtr[0x14] << 8);
        SSTflash = FALSE;
    }
#ifdef INCLUDE_CFI_SCS
    if (cfi.cmdSet == INTEL_COMMAND_SET) {
        cfi.write = cfiscsBufferWrite_x16;
        cfi.erase = cfiscsBlockErase_x16;
        cfi.readCmd = SCS_READ_ARRAY;
    }
#endif
#ifdef INCLUDE_CFI_AMDFJ
    if (cfi.cmdSet == AMDFUJ_COMMAND_SET) {
        cfi.write = cfiamdBufferWrite_x16;
        cfi.erase = cfiamdBlockErase_x16;
        cfi.readCmd = AMD_RESET_ARRAY;
    }
#endif
    if (cfi.write == NULL) {
        return FALSE;
    }

    cfi.size = 1L << flashPtr[0x27];
    /* This is still bytes, which the low level code counts */
    cfi.type = (flashPtr[0] << 8) | flashPtr[1];
    cfi.vpp = (flashPtr[0x1d] != 0) ? TRUE : FALSE;
    cfi.wb = 1L << (flashPtr[0x2a] | ((unsigned)flashPtr[0x2b] << 8));
    if (cfi.wb > 2) {
        /*
         * Conservative for odd byte alignments, but stay a power
         * of two to avoid odd erase block strides.
         */
        cfi.wb >>= 1;
    }
    /* AMD code only writes 1 x16 per cycle, so pass in more than 1B */
    if (cfi.cmdSet == AMDFUJ_COMMAND_SET) {
        if (cfi.wb == 1) {
            cfi.wb = 16;
        }
        /* check for top part */
        off = flashPtr[0x15] | ((unsigned)flashPtr[0x16]<<8);
        cfi.top = (flashPtr[off+0xf] == 3) ? TRUE : FALSE;
    }
printk("cfi.size is 0x%x\n", cfi.size);
printk("cfi.type is 0x%x\n", cfi.type);
printk("cfi.vpp is 0x%x\n", cfi.vpp);
printk("cfi.wb is 0x%x\n", cfi.wb);
printk("cfi.top is 0x%x\n", cfi.top);

    cfi.eraseTimeoutMs = (1 << flashPtr[0x21]) * (1 << flashPtr[0x25]);
    cfi.writeTimeoutUs = (1 << flashPtr[0x1f]) * (1 << flashPtr[0x23]);
printk("cfi.eraseTimeoutMs is 0x%x\n", cfi.eraseTimeoutMs);
printk("cfi.writeTimeoutUs is 0x%x\n", cfi.writeTimeoutUs);

    cfi.blockSize = 0;
    cfi.eBlocks = flashPtr[0x2c];
printk("cfi.bBlocks is 0x%x\n", cfi.eBlocks);
    off = 0x2d;
    if (cfi.top) {
        for (i=cfi.eBlocks-1; i >= 0; i--) {
            cfi.ebNum[i] = (flashPtr[off] |
                            ((unsigned)flashPtr[(off+1)]) << 8) + 1;
            cfi.ebSize[i] = (flashPtr[(off+2)] |
                             ((unsigned)flashPtr[(off+3)]) << 8) * 0x100;
            if (cfi.ebSize[i] > cfi.blockSize) {
                cfi.blockSize = cfi.ebSize[i];
            }
            off += 4;
        }
    } else {
        for (i=0; i < cfi.eBlocks; i++) {
            cfi.ebNum[i] = (flashPtr[off] |
                            ((unsigned)flashPtr[(off+1)]) << 8) + 1;
            cfi.ebSize[i] = (flashPtr[(off+2)] |
                             ((unsigned)flashPtr[(off+3)]) << 8) * 0x100;
            if (cfi.ebSize[i] > cfi.blockSize) {
                cfi.blockSize = cfi.ebSize[i];
            }
            off += 4;
printk("i is %x, cfi.ebNum is 0x%x\n", i, cfi.ebNum[i]);
printk("i is %x, cfi.ebSize is 0x%x\n", i, cfi.ebSize[i]);
        }
    }
//    base = cfi.base + cfi.size;
printk("ptr is 0x%x\n", (unsigned int)ptr);
printk("cfi.base is 0x%x\n", cfi.size);
printk("cfi.blockSize is 0x%x\n", cfi.blockSize);
    off = cfi.eBlocks - 1;
    base = (UINT8 *)(ptr) + cfi.size - cfi.ebSize[off];
    basePtr = (UINT8 *)(ioremap((phys_t)base, cfi.ebSize[off]));
    i = NFLC - 1;

printk("base is 0x%x\n", (unsigned int)base);
printk("basePtr is 0x%x\n", (unsigned int)basePtr);
printk("off is 0x%x\n", off);
    cfi.flcPtr[FLC_BOOTLINE] = basePtr;
    cfi.flcSize[FLC_BOOTLINE] = 0;
    cfi.flcPtr[FLC_BOARDDATA] = basePtr;
    cfi.flcSize[FLC_BOARDDATA] = DK_BOARDCONFIG_SIZE;
    cfi.flcPtr[FLC_RADIOCFG] = basePtr + DK_BOARDCONFIG_SIZE;
    cfi.flcSize[FLC_RADIOCFG] = DK_RADIOCONFIG_SIZE;
printk("i=0:fPtr=0x%x:fS=0x%x:ebSize=0x%x\n", (unsigned int)cfi.flcPtr[0], cfi.flcSize[0], cfi.ebSize[off]);
printk("i=1:fPtr=0x%x:fS=0x%x:ebSize=0x%x\n", (unsigned int)cfi.flcPtr[1], cfi.flcSize[1], cfi.ebSize[off]);
printk("i=2:fPtr=0x%x:fS=0x%x:ebSize=0x%x\n", (unsigned int)cfi.flcPtr[2], cfi.flcSize[2], cfi.ebSize[off]);

    flashPtr[0] = cfi.readCmd;

    return TRUE;
}

void
sysFlashConfigInit(void *ptr, INT32 bl, INT32 bd, INT32 rc)
{
    int i;
    if (cfi.ok == TRUE) {
        /* No need to re-initialize the flash on reboot */
        return;
    }

    cfi.ok = FALSE;
    cfi.flcReq[FLC_BOOTLINE] = bl;
    cfi.flcReq[FLC_BOARDDATA] = bd;
    cfi.flcReq[FLC_RADIOCFG] = rc;
    cfi.top = FALSE;
    /* try x16 */
    if (cfi.ok == FALSE) {
        cfi.ok = sysFlashConfigInit_x16((FlashWPTR)ptr);
        if (cfi.ok == FALSE) {
            printk("\r\nCould not find flash device!\r\n");
            return;
        }
        cfi.x16 = TRUE;
    }

    /* Calculate cap at end of the device */
    cfi.cap = cfi.size;
    if (cfi.top == TRUE) {
        cfi.cap -= cfi.ebNum[cfi.eBlocks-1] * cfi.ebSize[cfi.eBlocks-1];
    } else {
        for (i=0; i < NFLC; i++) {
            cfi.cap -= cfi.flcSize[i];
        }
    }
}

/*
 * sysFlashConfigWriteX - write taking the device width into account.
 */
static FLStatus
sysFlashConfigWriteX(FlashPTR flashPtr, int offset, UINT8 *p, int len)
{
    FLStatus rc = flOK;
    int i;

    for (i=0; i < len; i += cfi.wb) {
        rc = cfi.write(flashPtr, offset+i, p+i, min((int)cfi.wb, len-i));
        if (rc != flOK) {
            break;
        }
    }
    return rc;
}

static UINT8
sysFlashConfigReadLocked(int flcn, int offset)
{
    volatile UINT8 *flashPtr = (volatile UINT8 *)cfi.flcPtr[flcn];

    return (cfi.ok == TRUE) ? flashPtr[offset] : 'x';
}

UINT8
sysFlashConfigRead(int flcn, int offset)
{
    UINT8 val;
    int s;

    s = cfiLock(FL_LOCK_BOOT);
    val = sysFlashConfigReadLocked(flcn, offset);
    cfiUnlock(FL_LOCK_BOOT, s);

    return val;
}

static void
sysFlashConfigEraseLocked(int flcn)
{
    FLStatus rc;

    if (cfi.ok == TRUE) {
        rc = cfi.erase(cfi.flcPtr[flcn]);
    }

    return;
}

void
sysFlashConfigErase(int flcn)
{
    int s;

    s = cfiLock(FL_LOCK_BOOT | FL_LOCK_WRITE);
    sysFlashConfigEraseLocked(flcn);
    cfiUnlock(FL_LOCK_BOOT | FL_LOCK_WRITE, s);

    return;
}

unsigned int 
sysFlashConfigWrite(int flcn, int offset, UINT8 *data, int len)
{
    int i, malloced = TRUE;
    FLStatus rc;
    UINT8 *p;
    UINT8 b;
    int s;

    if (cfi.ok == FALSE) {
		return (0);
    }

    //printk("sysFlashConfigWrite: flc=%d:offset=%d:len=%d\n", flcn, offset, len);
    s = cfiLock(FL_LOCK_BOOT | FL_LOCK_WRITE);

    for (i=0; i < len; i++) {
        b = sysFlashConfigReadLocked(flcn, offset+i);
        /* If any 0 bits need to be 1, then we need to erase */
        if ((~b & data[i]) != 0) {
            /* erase needed, save, merge, rewrite */
//            p = kmalloc(cfi.flcReq[flcn], GFP_KERNEL);
            /* all data are in one sector */
            p = kmalloc(cfi.flcReq[FLC_BOOTLINE] + 
                        cfi.flcReq[FLC_BOARDDATA] + 
                        cfi.flcReq[FLC_RADIOCFG], GFP_KERNEL);
            if (p == NULL) {
                /* kmalloc can fail during early init */
                malloced = FALSE;
		printk("memory allocation fail\n");
		return (-ENXIO);
            }
            for (i=0; i < cfi.flcReq[FLC_BOOTLINE]; i++) {
                p[i]  = sysFlashConfigReadLocked(FLC_BOOTLINE, i);
            }
            for (i=0; i < cfi.flcReq[FLC_BOARDDATA]; i++) {
                p[cfi.flcReq[FLC_BOOTLINE] + i]  = 
                       sysFlashConfigReadLocked(FLC_BOARDDATA, i);
            }
            for (i=0; i < cfi.flcReq[FLC_RADIOCFG]; i++) {
                p[cfi.flcReq[FLC_BOOTLINE] + cfi.flcReq[FLC_BOARDDATA] + i]  = 
                       sysFlashConfigReadLocked(FLC_RADIOCFG, i);
            }
            for (i=0; i < len; i++) {
                if(flcn == FLC_BOOTLINE) p[offset + i] = data[i];
                if(flcn == FLC_BOARDDATA) 
                    p[cfi.flcReq[FLC_BOOTLINE] + offset + i] = data[i];
                if(flcn == FLC_RADIOCFG) 
                    p[cfi.flcReq[FLC_BOOTLINE] + cfi.flcReq[FLC_BOARDDATA] + offset+i] = data[i];
            }
            sysFlashConfigEraseLocked(flcn);
            rc = sysFlashConfigWriteX((FlashPTR)cfi.flcPtr[0], 0, p, 
                  (cfi.flcReq[FLC_BOOTLINE] + 
                  cfi.flcReq[FLC_BOARDDATA] + cfi.flcReq[FLC_RADIOCFG]));
            cfiUnlock(FL_LOCK_BOOT | FL_LOCK_WRITE, s);
            if (rc != flOK) {
                printk("sysFlashConfigWrite: failed rc=%d\n", rc);
		return (-EINVAL);
            }
            if (malloced) {
                kfree(p);
            }
            return 1;
        }
    }
    rc = sysFlashConfigWriteX(cfi.flcPtr[flcn], offset, data, len);
    cfiUnlock(FL_LOCK_BOOT | FL_LOCK_WRITE, s);

    if (rc != flOK) {
        printk("sysFlashConfigWrite: failed rc=%d\n", rc);
	return (-EINVAL);
    }
    return 1;
}

int flash_init(void) {


    	sysFlashConfigInit((void *)(0x50000000),
                       0,
                       DK_BOARDCONFIG_SIZE,
                       DK_RADIOCONFIG_SIZE);

	 return 0;
}

void flash_exit(void) {
     iounmap((void *)cfi.flcPtr[FLC_BOOTLINE]);
     iounmap((void *)cfi.base);
}

unsigned int dk_flash_read (int flc, unsigned int offset, size_t len, UINT8 * buf) {
   UINT16 iIndex;
	
   //Dprintk("buf=%x\n", buf);
   for(iIndex=0; iIndex<len; iIndex++) 
       buf[iIndex] = sysFlashConfigRead(flc, offset+iIndex);

   return (1);

}

unsigned int dk_flash_write (int flc, unsigned int offset, size_t len, UINT8 * buf) {
  return sysFlashConfigWrite(flc, offset, buf, len);
}



