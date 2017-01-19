
#ifndef FLBASE_H
#define FLBASE_H

#include "flcustom.h"

/* standard type definitions */
typedef int 		FLBoolean;

/* Boolean constants */

#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define	TRUE	1
#endif

#ifndef ON
#define	ON	1
#endif
#ifndef OFF
#define	OFF	0
#endif

#define SECTOR_SIZE		(1 << SECTOR_SIZE_BITS)



#define FAR_LEVEL       0
/* define SectorNo range according to media maximum size */
#if (MAX_VOLUME_MBYTES * 0x100000l) / SECTOR_SIZE > 0x10000l
typedef unsigned long SectorNo;
#define	UNASSIGNED_SECTOR 0xffffffffl
#else
typedef unsigned short SectorNo;
#define UNASSIGNED_SECTOR 0xffff
#endif

#if FAR_LEVEL > 0
  #define FAR0	far
#else
  #define FAR0
#endif

#if FAR_LEVEL > 1
  #define FAR1	far
#else
  #define FAR1
#endif

#if FAR_LEVEL > 2
  #define FAR2	far
#else
  #define FAR2
#endif


#define vol (*pVol)


#ifndef BIG_ENDIAN

typedef unsigned short LEushort;
typedef unsigned long LEulong;

#define LE2(arg)	arg
#define	toLE2(to,arg)	(to) = (arg)
#define LE4(arg)	arg
#define	toLE4(to,arg)	(to) = (arg)
#define COPY2(to,arg)	(to) = (arg)
#define COPY4(to,arg)	(to) = (arg)

typedef unsigned char Unaligned[2];
typedef Unaligned Unaligned4[2];

#define UNAL2(arg)	fromUNAL(arg)
#define toUNAL2(to,arg)	toUNAL(to,arg)

#define UNAL4(arg)	fromUNALLONG(arg)
#define toUNAL4(to,arg)	toUNALLONG(to,arg)

extern void toUNAL(unsigned char FAR0 *unal, unsigned n);

extern unsigned short fromUNAL(unsigned char const FAR0 *unal);

extern void toUNALLONG(Unaligned FAR0 *unal, unsigned long n);

extern unsigned long fromUNALLONG(Unaligned const FAR0 *unal);

#else

typedef unsigned char LEushort[2];
typedef unsigned char LEulong[4];

#define LE2(arg)	fromLEushort(arg)
#define	toLE2(to,arg)	toLEushort(to,arg)
#define LE4(arg)	fromLEulong(arg)
#define	toLE4(to,arg)	toLEulong(to,arg)
#define COPY2(to,arg)	copyShort(to,arg)
#define COPY4(to,arg)	copyLong(to,arg)

#define	Unaligned	LEushort
#define	Unaligned4	LEulong

extern void toLEushort(unsigned char FAR0 *le, unsigned n);

extern unsigned short fromLEushort(unsigned char const FAR0 *le);

extern void toLEulong(unsigned char FAR0 *le, unsigned long n);

extern unsigned long fromLEulong(unsigned char const FAR0 *le);

extern void copyShort(unsigned char FAR0 *to,
		      unsigned char const FAR0 *from);

extern void copyLong(unsigned char FAR0 *to,
		     unsigned char const FAR0 *from);

#define	UNAL2		LE2
#define	toUNAL2		toLE2

#define	UNAL4		LE4
#define	toUNAL4		toLE4

#endif /* BIG_ENDIAN */


typedef enum {flOK 		= 0,	/* Status code for operation.
					   A zero value indicates success,
					   other codes are the extended
					   DOS codes. */
	     flBadFunction	= 1,
	     flFileNotFound	= 2,
	     flPathNotFound	= 3,
	     flTooManyOpenFiles = 4,
	     flNoWriteAccess	= 5,
	     flBadFileHandle	= 6,
	     flDriveNotAvailable = 9,
	     flNonFATformat	= 10,
	     flFormatNotSupported = 11,
	     flNoMoreFiles	= 18,
	     flWriteProtect 	= 19,
	     flBadDriveHandle	= 20,
	     flDriveNotReady 	= 21,
	     flUnknownCmd 	= 22,
	     flBadFormat	= 23,
	     flBadLength	= 24,
	     flDataError	= 25,
	     flUnknownMedia 	= 26,
	     flSectorNotFound 	= 27,
	     flOutOfPaper 	= 28,
	     flWriteFault 	= 29,
	     flReadFault	= 30,
	     flGeneralFailure 	= 31,
	     flDiskChange 	= 34,
	     flVppFailure	= 50,
	     flBadParameter	= 51,
	     flNoSpaceInVolume	= 52,
	     flInvalidFATchain	= 53,
	     flRootDirectoryFull = 54,
	     flNotMounted	= 55,
	     flPathIsRootDirectory = 56,
	     flNotADirectory	= 57,
	     flDirectoryNotEmpty = 58,
	     flFileIsADirectory	= 59,
	     flAdapterNotFound	= 60,
	     flFormattingError	= 62,
	     flNotEnoughMemory	= 63,
	     flVolumeTooSmall	= 64,
	     flBufferingError	= 65,
	     flFileAlreadyExists = 80,

	     flIncomplete	= 100,
	     flTimedOut		= 101,
	     flTooManyComponents = 102

	     } FLStatus;

/* call a procedure returning status and fail if it fails. This works only in
   routines that return Status */

#define checkStatus(exp)      {	FLStatus status = (exp); \
				if (status != flOK)	 \
				  return status; }


#endif
