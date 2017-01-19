/*
 * SPI Flash Memory support header file.
 *
 * $Id: //depot/sw/branches/art2_main/src/art2/driver/linux/modules/include/dk_flash.h#2 $
 *
 *
 * Copyright (c) 2005, Atheros Communications Inc.
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#define CYGNUM_FLASH_BLOCK_SIZE  0x10000
#define FALSE 0
#define TRUE 1
#define CYGNUM_FLASH_END_RESERVED_BYTES 0x10000

#define DK_BOARDCONFIG_SIZE 0x1000
#define DK_RADIOCONFIG_SIZE 0x1000

int flash_init(void);
void flash_exit(void);
unsigned int dk_flash_read (int flc, unsigned int offset, size_t len, UINT8 * buf);
unsigned int dk_flash_write (int flc, unsigned int offset, size_t len, UINT8 * buf);

/**********************************************************************/
