#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "common.h"
//#include "uboot_cfg.h"

#define IFX_CFG_FLASH_SYSTEM_CFG_START_ADDR 0
#define IFX_CFG_FLASH_UBOOT_CFG_START_ADDR 0
#define IFX_CFG_FLASH_UBOOT_CFG_SIZE 256*1024
#if 1 /*ctc 20091202 modified flash partition tables*/
 #define MTD_CONFIG_DEV_NAME	"/dev/mtd1"
 #define MTD_DEV_START_ADD		IFX_CFG_FLASH_SYSTEM_CFG_START_ADDR
#else
 #define MTD_CONFIG_DEV_NAME	IFX_CFG_FLASH_UBOOT_CFG_MTDBLOCK_NAME
 #define MTD_DEV_START_ADD	 	IFX_CFG_FLASH_UBOOT_CFG_MTDBLOCK_START_ADDR
#endif

#define CFG_ENV_ADDR			IFX_CFG_FLASH_SYSTEM_CFG_START_ADDR
#define CFG_ENV_SIZE			IFX_CFG_FLASH_UBOOT_CFG_SIZE
#define ENV_HEADER_SIZE			(sizeof(unsigned long))

#if CFG_ENV_SIZE > (0x40000L-4)
  #define	ENV_SIZE			(0x40000L-4)	// 256K-ENV_HEADER_SIZE
#else
  #define ENV_SIZE				(CFG_ENV_SIZE - ENV_HEADER_SIZE)
#endif

typedef	struct environment_s {
	unsigned long	crc;		/* CRC32 over data bytes	*/
	unsigned char	data[ENV_SIZE]; /* Environment data		*/
} env_t;

extern int read_env();
extern int envmatch (unsigned char *s1, int i2);
extern char *get_env (unsigned char *name);
extern int set_env(char *name,char *val);
extern int saveenv();
extern unsigned long find_mtd(unsigned long addr_first,char *mtd_dev);

#endif /* _COMMAND_H_ */
