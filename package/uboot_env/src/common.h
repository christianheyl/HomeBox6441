#ifndef _COMMON_H_
#define _COMMON_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <fcntl.h>
#include <unistd.h>

//#define	IFX_CONFIG_MEMORY_SIZE		32
//#define	IFX_CONFIG_MEMORY_SIZE		64
//#define	IFX_CONFIG_FLASH_SIZE		2
//#define	IFX_CONFIG_FLASH_SIZE		4
//#define	IFX_CONFIG_FLASH_SIZE		8
//#define	IFX_CONFIG_FLASH_SIZE		16
//#define	IFX_CONFIG_FLASH_SIZE		32
//#define	CFG_BOOT_FROM_NOR			1
//#define	IN_SUPERTASK				0
//#define	CFG_BOOT_FROM_NAND			1
//#define	CFG_BOOT_FROM_SPI			1
//#define	BUILD_FROM_IFX_KERNEL		1
#define	BUILD_FROM_IFX_UTILITIES		1

//#define IFX_DEBUG

#if 0//def IFX_DEBUG
  #define	ifx_debug_printf(format, args...) do { fprintf( stderr, format, ##args); fflush( stderr ); } while (0)
  //#define	DBG_dump( label, p, len )	DBG_dump2( label, p, len, __FILE__, __LINE__ )
  #define	DBG_dump
  //extern void DBG_dump2(const char *label, const void *p, size_t len, char *InFile, unsigned int OnLine);
#else
  #define	ifx_debug_printf(format, args...)
  #define	DBG_dump
#endif

#endif /* _COMMON_H_ */
