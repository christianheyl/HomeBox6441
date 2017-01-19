#ifndef _FLASH_H_
#define _FLASH_H_

int flash_sect_erase(unsigned long addr_first,unsigned long addr_last,unsigned int bPartial);

int flash_write(unsigned long srcAddr,unsigned long destAddr,int srcLen);

void flash_sect_protect(int mode,unsigned long addr_first,unsigned long addr_last);

#endif /* _FLASH_H_ */
