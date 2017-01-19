/*
 * $Log:   P:/user/amir/lite/vcs/flcustom.h_v  $

      Rev 1.10   14 Aug 1997 16:05:12   danig
   MIN_CLUSTER_SIZE

      Rev 1.9   04 Aug 1997 13:13:48   danig
   Low level API

      Rev 1.8   24 Jul 1997 18:11:14   amirban
   Moved stuff to flsystem.h

      Rev 1.7   07 Jul 1997 15:23:42   amirban
   Ver 2.0

      Rev 1.6   21 Oct 1996 18:12:58   amirban
   Split/join option

      Rev 1.5   14 Aug 1996 14:16:26   amirban
   BACKGROUND

      Rev 1.4   14 Jul 1996 16:48:56   amirban
   No more formatting parameters

      Rev 1.3   09 Jul 1996 14:36:38   amirban
   CPU_i386 define

      Rev 1.2   08 Jul 1996 17:21:00   amirban
   Added ABS_READ_WRITE

      Rev 1.1   19 Jun 1996 15:23:10   amirban
   Added EXIT option

      Rev 1.0   19 May 1996 19:44:42   amirban
   Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1996			*/
/*									*/
/************************************************************************/


#ifndef FLCUSTOM_H
#define FLCUSTOM_H

/*
 *
 *		Section 2: File System Customization
 *		------------------------------------
 */

/* ISSUE #2.1: Number of drives
 *
 * Defines the maximum number of drives (and sockets) supported.
 *
 * The actual number of drives depends on which socket controllers are
 * actually registered.
 */

#define DRIVES	5


/* ISSUE #2.1: Number of open files
 *
 * Defines the maximum number of files that may be open at a time.
 */

#define FILES	  0


/* ISSUE #2.2: Sector size
 *
 * Define the log2 of sector size for the FAT & FTL layer. Note that the
 * default 512 bytes is the de-facto standard and practically the only one
 * that provides real PC interoperability.
 */

#define SECTOR_SIZE_BITS	9


/* ISSUE #2.3: Formatting
 *
 * Uncomment the following line if you need to format with fsFormatVolume.
 */

#define FORMAT_VOLUME


/* ISSUE #2.4: Defragmentation
 *
 * Uncomment the following line if you need to defragment with
 * fsDefragmentVolume.
 */

/* #define DEFRAGMENT_VOLUME */


/* ISSUE #2.5: Sub-directories
 *
 * Uncomment the following line if you need support for sub-directories
 */

/* #define SUB_DIRECTORY */


/* ISSUE #2.6: Rename file
 *
 * Uncomment the following line if you need to rename files with fsRenameFile.
 */

/* #define RENAME_FILE */


/* ISSUE #2.7: Split / join file
 *
 * Uncomment the following line if you need to split or join files with
 * fsSplitFile and fsJoinFile
 */

/* #define SPLIT_JOIN_FILE */


/* ISSUE #2.8: 12-bit FAT support
 *
 * Comment the following line if you do not need support for DOS media with
 * 12-bit FAT (typically media of 2 MBytes or less).
 */

#define FAT_12BIT


/* ISSUE #2.9: Parse path function
 *
 * Uncomment the following line if you need to parse DOS-like path names
 * with fsParsePath.
 */

/* #define PARSE_PATH */


/* ISSUE #2.10: Maximum supported medium size
 *
 * Define here the largest Flash medium size (in MBytes) you want supported.
 */

#define MAX_VOLUME_MBYTES	40


/* ISSUE #2.11: Assumed card parameters
 *
 * This issue is relevant only if you are not defining any allocation
 * routines in #5.2.
 *
 * The following are assumptions about FTL parameters of the Flash media.
 * They affect the size of the heap buffer allocated in FTLLITE.C.
 */

#define ASSUMED_FTL_UNIT_SIZE	0x20000l	/* Intel interleave-2 */
#define	ASSUMED_VM_LIMIT	0x10000l	/* limit at 64 KB */
#define ASSUMED_NFTL_UNIT_SIZE	0x4000l


/* ISSUE #2.12: Number of buffers
 *
 * Normally two sector buffers are needed, one for the FAT layer, another
 * for the FTL layer. You may save about 512 bytes of RAM requirements by
 * uncommenting the following definition.
 * This will come at the cost of both read and write performance.
 */

/*#define SINGLE_BUFFER */


/* ISSUE #2.13: Absolute read & write
 *
 * Uncomment the following line if you want to be able to read & write
 * sectors by absolute sector number (i.e. without regard to files and
 * directories).
 */

#define ABS_READ_WRITE


/* ISSUE #2.14: Low level operations
 *
 * Uncomment the following line if you want to do low level operations.
 * Low level operations include: read from a physical address, write to
 * a physical address, erase a unit according to its physical unit number.
 */

#define LOW_LEVEL


/* ISSUE #2.15: Application exit
 *
 * If the FLite application ever exits, it needs to call fsEXit before
 * exitting. Uncomment the following line to enable this.
 */

/* #define EXIT */


/* ISSUE #2.16: Number of sectors per cluster
 *
 * Define the minimum cluster size in sectors.
 */

#define MIN_CLUSTER_SIZE   4


/*
 *
 *		Section 3: Socket Hardware Customization
 *		----------------------------------------
 */

/* ISSUE #3.1: Vpp voltage
 *
 * If your socket does not supply 12 volts, comment the following line. In
 * this case, you will be able to work only with Flash cards that do not
 * require external 12 Volt Vpp.
 *
 */

#define SOCKET_12_VOLTS


/* ISSUE #3.2: Fixed or removable media
 *
 * If your Flash media is fixed, uncomment the following line, and ignore
 * issue #3.3.
 */

/* #define FIXED_MEDIA */


/* ISSUE #3.3: Hardware card change detection
 *
 * This issue is now obsolete
 *
 */


/* ISSUE #3.4: Interval timer
 *
 * The following defines a timer polling interval in milliseconds. If the
 * value is 0, an interval timer is not installed.
 *
 * If you select an interval timer, you should provide an implementation
 * for 'installTimer' defined in timer.h.
 *
 * An interval timer is not a must, but it is recommended. The following
 * will occur if an interval timer is absent:
 *
 * - Card changes can be recognized only if socket hardware detects them
 *   (see issue #3.2).
 * - The Vpp delayed turn-off procedure is not applied. This may downgrade
 *   write performance significantly if the Vpp switching time is slow.
 * - The watchdog timer that guards against certain operations being stuck
 *   indefinitely will not be active.
 */

#define	POLLING_INTERVAL 100		/* Polling interval in millisec.
					   if 0, no polling is done */


/*
 *
 *		       Section 4: MTD Customization
 *		       ----------------------------
 */

/* ISSUE #4.1: MTD Installation
 *
 * This issue is now obsolete
 *
 */


/* ISSUE #4.2: Background erasing
 *
 * If you include support for Flash technology that has suspend-for-write
 * capability, you can gain considerable write performance and improve
 * real-time response for your write operations by including background erase
 * capability. In some cases, you can gain performance by including this
 * feature even if no suspend-for-write capability is supported. See the
 * FLite manual for complete details.
 *
 * On the other hand, this feature adds to required code & RAM, makes
 * necessary some additional customization on your part, and depends on
 * compiler features that, while defined as ANSI-C standard, may have
 * a problematic implementation in your particular environment. See the
 * FLite manual for complete details.
 *
 * Uncomment the following line to support background erasing.
 */

/* #define	BACKGROUND */


/* ISSUE #4.3: Maximum MTD's and Translation Layers
 *
 * Define here the maximum number of Memory Technology Drivers and
 * Translation Layers that may be installed. Note that the actual
 * number installed is determined by which components are installed in
 * 'registerComponents' (custom.c)
 */

#define MTDS	10	/* Up to 5 MTD's */

#define	TLS	5	/* Up to 3 Translation Layers */


#endif
