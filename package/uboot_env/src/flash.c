
#include "common.h"
#include "command.h"
#include "cmd_upgrade.h"
#include <sys/ioctl.h>
#include "mtd/mtd-user.h"

int flash_sect_erase(unsigned long addr_first,unsigned long addr_last,unsigned int bPartial)
{
	char mtd_dev[16];
	unsigned long part_begin_addr;
	struct mtd_info_user mtd;
	struct erase_info_user erase;
	unsigned long erase_begin,erase_sect_begin;
	unsigned long erase_end,erase_sect_end;
	long preImageSize,postImageSize;
	int dev_fd;
	char *preImage = NULL, *postImage = NULL;
	int blocks = 0;
	
	memset(mtd_dev,0x00,sizeof(mtd_dev));
	part_begin_addr = find_mtd(addr_first,mtd_dev);
	if(strcmp(mtd_dev,"") == 0)
	{
		ifx_debug_printf("For addr_first, partition can not be found out\n",addr_first);
		return 1;
	}

	erase_begin = addr_first - part_begin_addr;
	erase_end = addr_last - part_begin_addr;

	erase_end += 1; /*Nirav: Fix for upgrade problem */

	ifx_debug_printf("addr_first 0x%08lx, addr_last 0x%08lx, part_begin_addr 0x%08lx,"
	                  "erase_begin 0x%08lx, erase_end 0x%08lx\n", addr_first, addr_last,
	                  part_begin_addr, erase_begin, erase_end);

	dev_fd = open(mtd_dev,O_SYNC | O_RDWR);
	if(dev_fd < 0){
		ifx_debug_printf("The device %s could not be opened\n",mtd_dev);
		return 1;
	}
	/* get some info about the flash device */
	if (ioctl (dev_fd,MEMGETINFO,&mtd) < 0){
		ifx_debug_printf("%s This doesn't seem to be a valid MTD flash device!\n",mtd_dev);
		return 1;
	}

	if(erase_begin >= mtd.size || erase_end > mtd.size || erase_end <= erase_begin){
		ifx_debug_printf("Erase begin 0x%08lx or Erase end 0x%08lx are out of boundary of mtd size 0x%08lx\n",erase_begin,erase_end,mtd.size);
		return 1;
	}

	erase_sect_begin = erase_begin & ~(mtd.erasesize - 1);
	erase_sect_end = erase_end & ~(mtd.erasesize - 1);
	if(erase_sect_end < erase_end)
		erase_sect_end = erase_sect_end + mtd.erasesize;

	ifx_debug_printf("erase_sect_begin: 0x%08lx, erase_sect_end: 0x%08lx, mtd.erasesize:0x%08x\n", erase_sect_begin, erase_sect_end, mtd.erasesize);

	preImageSize = erase_begin - erase_sect_begin;
	if(preImageSize > 0){
		ifx_debug_printf("Saving %d data as erase_begin 0x%08lx is not on sector boundary 0x%08lx\n",preImageSize,erase_begin,erase_sect_begin);
		preImage = (char *)calloc(preImageSize,sizeof(char));
		if(preImage == NULL)
		{
			ifx_debug_printf("flash_erase : Could not allocate memory for preImage of size %d\n",preImageSize);
			if(dev_fd)
				close(dev_fd);
			return 1;
		}
		lseek(dev_fd,0L,SEEK_SET);
		lseek(dev_fd,erase_sect_begin,SEEK_CUR);
		if(read(dev_fd,preImage,preImageSize) != preImageSize)
		{
			printf("flash_erase : Could not read %d bytes of data from %s for preImage\n",preImageSize,mtd_dev);
			if(dev_fd)
				close(dev_fd);
			if(preImage)
				free(preImage);
			return 1;
		}
		ifx_debug_printf("flash_erase : read %d bytes for preImage\n",preImageSize);
	}

	postImageSize = erase_sect_end	- erase_end;
	ifx_debug_printf("preImageSize: 0x%08lx, postImageSize: 0x%08lx\n", preImageSize, postImageSize);
	if(postImageSize > 0){
		ifx_debug_printf("Saving %d data as erase_end 0x%08lx is not on sector boundary 0x%08lx\n",postImageSize,erase_end,erase_sect_end);
		postImage = (char *)calloc(postImageSize,sizeof(char));
		if(postImage == NULL)
		{
			ifx_debug_printf("flash_erase : Could not allocate memory for postImage of size %d\n",postImageSize);
			if(dev_fd)
				close(dev_fd);
			if(preImage)
				free(preImage);
			return 1;
		}
		lseek(dev_fd,0L,SEEK_SET);
		lseek(dev_fd,erase_end,SEEK_CUR);
		if(read(dev_fd,postImage,postImageSize) != postImageSize)
		{
			printf("flash_erase : Could not read %d bytes of data from %s for preImage\n",postImageSize,mtd_dev);
			if(dev_fd)
				close(dev_fd);
			if(preImage)
				free(preImage);
			if(postImage)
				free(postImage);
			return 1;
		}
		ifx_debug_printf("flash_erase : read %d bytes for postImage\n",postImageSize);
	}

	blocks = (erase_sect_end - erase_sect_begin) / mtd.erasesize;
	erase.start = erase_sect_begin;
	erase.length = mtd.erasesize * blocks;
	if (ioctl (dev_fd,MEMERASE,&erase) < 0)
	{
		ifx_debug_printf ("Error : While erasing 0x%.8x-0x%.8x on %s: %m\n",
				(unsigned int) erase.start,(unsigned int) (erase.start + erase.length),mtd_dev);
		return 1;
	}

	if(preImageSize > 0){
		ifx_debug_printf("Writing back %d data as erase_begin 0x%08lx is not on sector boundary 0x%08lx\n",preImageSize,erase_begin,erase_sect_begin);
		lseek(dev_fd,0L,SEEK_SET);
		lseek(dev_fd,erase_sect_begin,SEEK_CUR);
		preImageSize = write(dev_fd,preImage,preImageSize);
		ifx_debug_printf("Wrote back at 0x%08lx size %d\n",erase_sect_begin,preImageSize);
		free(preImage);
	}

	if(postImageSize > 0){
		ifx_debug_printf("Writing back %d data as erase_end 0x%08lx is not on sector boundary 0x%08lx\n",postImageSize,erase_end,erase_sect_end);
		lseek(dev_fd,0L,SEEK_SET);
		lseek(dev_fd,erase_end,SEEK_CUR);
		postImageSize = write(dev_fd,postImage,postImageSize);
		ifx_debug_printf("Wrote back at 0x%08lx size %d\n",erase_end,postImageSize);
		free(postImage);
	}

	close(dev_fd);
	return 0;
}

#define PASS_SIZE 0x10000

int flash_write(unsigned long srcAddr,unsigned long destAddr,int srcLen)
{
	char mtd_dev[16];
	unsigned long part_begin_addr;
	int bWrote = 0;
	int dev_fd;
	int i,writeLen,nPass;
  #ifdef IFX_DEBUG
	char buf[20];
  #endif

	memset(mtd_dev,0x00,sizeof(mtd_dev));
	part_begin_addr = find_mtd(destAddr,mtd_dev);
	if(strcmp(mtd_dev,"") == 0)
	{
		ifx_debug_printf("For srcAddr, partition can not be found out\n",srcAddr);
		return 1;
	}
	ifx_debug_printf("destAddr 0x%08lx, part_begin_addr 0x%08lx\n", destAddr, part_begin_addr);
	destAddr -= part_begin_addr;

	dev_fd = open(mtd_dev,O_SYNC | O_RDWR);
	if(dev_fd < 0){
		ifx_debug_printf("The device %s could not be opened\n",mtd_dev);
		return 1;
	}
  #ifdef IFX_DEBUG
	lseek(dev_fd,0L,SEEK_SET);
	read(dev_fd, (void *)buf, 20);
	DBG_dump("****buf:\n", buf, 20);
  #endif

	nPass = (int)(srcLen / PASS_SIZE) + 1;
	ifx_debug_printf("destAddr 0x%08lx, srcLen 0x%08lx\n", destAddr, srcLen);
	for(i = 0; i < nPass; i++)
	{
		if(srcLen > PASS_SIZE)
			writeLen = PASS_SIZE;
		else
			writeLen = srcLen;
		lseek(dev_fd,0L,SEEK_SET);
		ifx_debug_printf("seek offset 0x%08lx, \n", destAddr + i * PASS_SIZE);
		lseek(dev_fd,destAddr + i * PASS_SIZE,SEEK_CUR);
		if((bWrote = write(dev_fd,(char *)srcAddr + i * PASS_SIZE,writeLen)) < writeLen)
		{
			ifx_debug_printf("Error : Only %d outof %d bytes could be written into %s\n",i * PASS_SIZE + bWrote,srcLen + i * PASS_SIZE,mtd_dev);
			return 1;
		}
		ifx_debug_printf("flash_write : Wrote %d bytes\n",bWrote);
		srcLen -= writeLen;
	}

  #ifdef IFX_DEBUG
	lseek(dev_fd,0L,SEEK_SET);
	read(dev_fd, (void *)buf, 20);
	DBG_dump("****buf:\n", buf, 20);
	DBG_dump("****srcAddr:\n", (char *)srcAddr, 20);
  #endif

    if(dev_fd) {
        close(dev_fd);
		ifx_debug_printf("close dev_fd\n");
    }
	return 0;
}

void flash_sect_protect(int mode,unsigned long addr_first,unsigned long addr_last)
{
#ifdef DO_FLASH_PROTECT
	char mtd_dev[16];
	unsigned long part_begin_addr;
	struct mtd_info_user mtd;
	struct erase_info_user erase;
	unsigned long erase_begin,erase_sect_begin;
	unsigned long erase_end,erase_sect_end;
	//long preImageSize,postImageSize;
	int dev_fd;
	//char *preImage = NULL, *postImage = NULL;
	int blocks = 0;

	memset(mtd_dev,0x00,sizeof(mtd_dev));
	part_begin_addr = find_mtd(addr_first,mtd_dev);
	if(strcmp(mtd_dev,"") == 0)
	{
		ifx_debug_printf("For addr_first, partition can not be found out\n",addr_first);
		return;
	}

	erase_begin = addr_first - part_begin_addr;
	erase_end = addr_last - part_begin_addr;

	erase_end += 1; /*Nirav: Fix for upgrade problem */

	ifx_debug_printf("addr_first 0x%08lx, addr_last 0x%08lx, part_begin_addr 0x%08lx, xxlock_begin 0x%08lx, xxlock_end 0x%08lx\n",
	                  addr_first, addr_last, part_begin_addr, erase_begin, erase_end);

	ifx_debug_printf("open\n",addr_first);
	dev_fd = open(mtd_dev,O_SYNC | O_RDWR);
	if(dev_fd < 0){
		ifx_debug_printf("The device %s could not be opened\n",mtd_dev);
		return;
	}
	/* get some info about the flash device */
	ifx_debug_printf("ioctl\n",addr_first);
	if (ioctl (dev_fd,MEMGETINFO,&mtd) < 0){
		ifx_debug_printf("%s This doesn't seem to be a valid MTD flash device!\n",mtd_dev);
		return;
	}

	if(erase_begin >= mtd.size || erase_end > mtd.size || erase_end <= erase_begin){
		ifx_debug_printf("xxlock begin 0x%08lx or xxlock end 0x%08lx are out of boundary of mtd size 0x%08lx\n",erase_begin,erase_end,mtd.size);
		return;
	}

	erase_sect_begin = erase_begin & ~(mtd.erasesize - 1);
	erase_sect_end = erase_end & ~(mtd.erasesize - 1);
	if(erase_sect_end < erase_end)
		erase_sect_end = erase_sect_end + mtd.erasesize;

ifx_debug_printf("protect_sect_begin: 0x%08lx, protect_sect_end: 0x%08lx, mtd.erasesize:0x%08x\n", erase_sect_begin, erase_sect_end, mtd.erasesize);

// =====================================================================
	blocks = (erase_sect_end - erase_sect_begin) / mtd.erasesize;
	erase.start = erase_sect_begin;
	erase.length = mtd.erasesize * blocks;

    if (mode == 0) {    // unlock
	ifx_debug_printf("ioctl unlock\n",addr_first);
	if (ioctl (dev_fd,MEMUNLOCK,&erase) < 0)
	{
		ifx_debug_printf ("Error : While unlocking 0x%.8x-0x%.8x on %s: %m\n",
				(unsigned int) erase.start,(unsigned int) (erase.start + erase.length),mtd_dev);
		return;
	}
    }
    if (mode == 1) {    // lock
	ifx_debug_printf("ioctl lock\n",addr_first);
	if (ioctl (dev_fd,MEMLOCK,&erase) < 0)
	{
		ifx_debug_printf ("Error : While locking 0x%.8x-0x%.8x on %s: %m\n",
				(unsigned int) erase.start,(unsigned int) (erase.start + erase.length),mtd_dev);
		return;
	}
    }

    if(dev_fd) {
        close(dev_fd);
	ifx_debug_printf("close dev_fd\n",addr_first);
    }

	ifx_debug_printf("return\n",addr_first);
#endif
	return;
}



void ubootCfgDump(int num) {

	char mtd_dev[16];
        unsigned long part_begin_addr, ubootCfgAddr, begin_addr;
	struct mtd_info_user mtd;
	int dev_fd, cfgSize, i;
	char *cfgImage;


	memset(mtd_dev,0x00,sizeof(mtd_dev));

	ubootCfgAddr = IFX_CFG_FLASH_UBOOT_CFG_START_ADDR;
	cfgSize = IFX_CFG_FLASH_UBOOT_CFG_SIZE;

	part_begin_addr = find_mtd(ubootCfgAddr, mtd_dev);
	if(strcmp(mtd_dev,"") == 0)
	{
		ifx_debug_printf("For ubootCfgAddr:0x%08x, partition can not be found out\n",ubootCfgAddr);
		return;
	}
	dev_fd = open(mtd_dev,O_SYNC | O_RDWR);
	if(dev_fd < 0){
		ifx_debug_printf("The device %s could not be opened\n",mtd_dev);
		return;
	}

	if (ioctl (dev_fd,MEMGETINFO,&mtd) < 0){
		ifx_debug_printf("%s This doesn't seem to be a valid MTD flash device!\n",mtd_dev);
		return;
	}

	begin_addr = ubootCfgAddr - part_begin_addr;

	cfgImage = (char *)calloc(cfgSize,sizeof(char));

	if(cfgImage == NULL)
	{
		ifx_debug_printf("Could not allocate memory for preImage of size %d\n",cfgImage);
		if(dev_fd)
			close(dev_fd);
		return;
	}
	lseek(dev_fd,0L,SEEK_SET);
	lseek(dev_fd,begin_addr,SEEK_CUR);
	if(read(dev_fd,cfgImage,cfgSize) != cfgSize)
	{
		printf("Could not read %d bytes of data from %s for preImage\n",cfgSize,mtd_dev);
		if(dev_fd)
			close(dev_fd);
		if(cfgImage)
			free(cfgImage);
		return;
	}
	printf("[%s() in %s #%d] read %d bytes for cfg    - %d\n\n\nDumping it ...\n[",__FUNCTION__, __FILE__, __LINE__, cfgSize, num);

	DBG_dump("ubootCfg:\n", cfgImage, cfgSize);

	//for (i=0; i< cfgSize; i++) {
	//    printf("%c(%02X)", cfgImage[i], cfgImage[i]&0xff);
	//}
	//printf("]\nend of ubootCfg. [%s() in %s #%d]\n\n\n", __FUNCTION__, __FILE__, __LINE__);

	if(dev_fd)
	    close(dev_fd);

	if(cfgImage)
            free(cfgImage);
}
