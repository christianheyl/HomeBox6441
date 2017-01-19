#include "common.h"
#include "command.h"
#include "cmd_upgrade.h"
#include "flash.h"
#include <mtd/mtd-user.h>	/* LINUX26 */
#include <sys/ioctl.h>
#include <limits.h>

#ifdef CONFIG_BOOT_FROM_SPI
#if defined(BUILD_FROM_IFX_UTILITIES)
#define u32 unsigned int
#endif

#include <spi_flash.h>
static struct spi_flash *flash_spi;
#ifndef CONFIG_ENV_SPI_BUS
# define CONFIG_ENV_SPI_BUS	0
#endif
#ifndef CONFIG_ENV_SPI_CS
# define CONFIG_ENV_SPI_CS		0
#endif
#ifndef CONFIG_ENV_SPI_MAX_HZ
# define CONFIG_ENV_SPI_MAX_HZ	1000000
#endif
#ifndef CONFIG_ENV_SPI_MODE
# define CONFIG_ENV_SPI_MODE	SPI_MODE_3
#endif

DECLARE_GLOBAL_DATA_PTR;
#endif

#ifdef CONFIG_BOOT_FROM_NAND
#if defined(BUILD_FROM_IFX_UTILITIES)
#define u16 unsigned short
#define u32 unsigned int
#define phys_addr_t unsigned long
#include <stdint.h>
#endif
#include <nand.h>
extern nand_info_t nand_info[];
#endif

#if defined(BUILD_FROM_IFX_UTILITIES)
#include "crc32.h"
#define getenv(x)					get_env(x)
#define simple_strtoul(a,b,c)				strtoul(a,b,c)
#define setenv(x,y)					set_env(x,y)
#define uchar						unsigned char
#define ulong						unsigned long
#define uint32_t					unsigned int					
#define uint8_t						unsigned char
					
 #if IFX_CONFIG_FLASH_SIZE <= 256
  #define	NAND_BLK_SIZE			0x4000
 #else
  #define	NAND_BLK_SIZE			0x10000
 #endif

#endif






#include "image.h"

//#define	DEBUG_UPGRADE_MECHANISM

#ifdef DEBUG_UPGRADE_MECHANISM
#	ifdef BUILD_FROM_IFX_UTILITIES
#		define upgrade_debug_printf(format,args...)	do { printf(format,##args); fflush( stdout ); } while(0)
#	else
#		define upgrade_debug_printf		debug
#	endif
#else
#	ifdef BUILD_FROM_IFX_UTILITIES
#		define upgrade_debug_printf(...)
#	else
#		define upgrade_debug_printf(...)
#	endif
#endif

static int rootfs_programming=0;

/*
 * Open an MTD device
 * @param	mtd	path to or partition name of MTD device
 * @param	flags	open() flags
 * @return	return value of open()
 */
int mtd_open(const char *mtd, int flags)
{
	FILE *fp;
	char dev[PATH_MAX];
	int i;

	upgrade_debug_printf("open %s\n", mtd);

	if ((fp = fopen("/proc/mtd", "r"))) {
		while (fgets(dev, sizeof(dev), fp)) {
			if (sscanf(dev, "mtd%d:", &i) && strstr(dev, mtd)) {
				snprintf(dev, sizeof(dev), "/dev/mtd%d", i);
				fclose(fp);

				return open(dev, flags);
			}
		}
		fclose(fp);
	}

	return open(mtd, flags);
}

int program_img_2(ulong srcAddr,int srcLen,ulong destAddr)
{
	int mtd_fd = -1;
	mtd_info_t mtd_info;
	erase_info_t erase_info;
	static char buf[1024*1024];

	upgrade_debug_printf("program_img : the srcAddr is 0x%08lx, length is %d while destAddr is 0x%08lx\n",srcAddr,srcLen,destAddr);

	/* Open MTD device and get sector size */
	if ((mtd_fd = mtd_open(MTD_CONFIG_DEV_NAME, O_RDWR)) < 0 || ioctl(mtd_fd, MEMGETINFO, &mtd_info) != 0)
	{
		perror(MTD_CONFIG_DEV_NAME);
		goto fail;
	}

	erase_info.length = mtd_info.erasesize;
	erase_info.start = 0;

	memset(buf, 0xff, sizeof(buf));
	memcpy(buf, srcAddr, srcLen);

	(void)ioctl(mtd_fd, MEMUNLOCK, &erase_info);
	if (ioctl(mtd_fd, MEMERASE, &erase_info) != 0 || write(mtd_fd, buf,  mtd_info.erasesize) != mtd_info.erasesize)
	{
		upgrade_debug_printf("Write image to flash fail\n");
		perror(MTD_CONFIG_DEV_NAME);
		goto fail;
	}
 fail:
		/* Dummy read to ensure chip(s) are out of lock/suspend state */
		(void) read(mtd_fd, buf, 2);

	if (mtd_fd >= 0)
		close(mtd_fd);
	return 0;
}

int program_img(ulong srcAddr,int srcLen,ulong destAddr)
{
	upgrade_debug_printf("program_img : the srcAddr is 0x%08lx, length is %d while destAddr is 0x%08lx\n",srcAddr,srcLen,destAddr);
#if defined(CFG_BOOT_FROM_NOR)
	flash_sect_protect(0,destAddr,destAddr + srcLen-1);
	upgrade_debug_printf("Erase Flash from 0x%08lx to 0x%08lx\n", destAddr, destAddr + srcLen-1);
	if(flash_sect_erase(destAddr,destAddr + srcLen-1,1)) {
		return 1;
	}
	DBG_dump("srcAddr:\n", srcAddr, 24);
	puts ("Writing to Flash... ");
	if(flash_write(srcAddr,destAddr,srcLen)) {
		return 1;
	}
	DBG_dump("srcAddr:\n", srcAddr, 24);
	upgrade_debug_printf("Image at 0x%08lx with size %d is programmed at 0x%08lx successfully\n",srcAddr,srcLen,destAddr);
	printf ("Saving Environment ...\n");
	flash_sect_protect(1,destAddr,destAddr + srcLen-1);
	upgrade_debug_printf ("end of program_img\n");
#elif defined(CONFIG_BOOT_FROM_SPI)
	//puts ("Writing to Serial Flash... ");
	flash_spi = spi_flash_probe(CONFIG_ENV_SPI_BUS, CONFIG_ENV_SPI_CS,
			CONFIG_ENV_SPI_MAX_HZ, CONFIG_ENV_SPI_MODE);
	if (!flash_spi){
		  printf("probe fails!\n");
		  return 1;
	}		
	spi_flash_write(flash_spi, destAddr, srcLen, (uchar *)srcAddr);
	/*
	if(eeprom_write (NULL, destAddr, (uchar *)srcAddr, srcLen)) {
		puts ("Serial flash write failed !!");
		return (1);
	}
	*/
	//puts ("done\n");
#elif defined(CFG_BOOT_FROM_NAND)
 #if !defined(BUILD_FROM_IFX_UTILITIES)
  if(rootfs_programming)
  {
      int ret;
      ulong rootfs_size; 
	  nand_erase_options_t opts;
	  rootfs_size = simple_strtoul((char *)getenv("f_rootfs_size"),NULL,16);
      memset(&opts, 1, sizeof(opts));
		  opts.offset = destAddr;
		  opts.length = rootfs_size;
		  opts.jffs2  = 1;
		  opts.quiet  = 0;
		  printf("erasing 0x%08x size 0x%08x\n",destAddr,srcLen);
		  ret = nand_erase_opts(&nand_info[0], &opts);
		  printf("erase %s\n", ret ? "ERROR" : "OK");
	
	    printf("writing to 0x%08x size 0x%08x\n",destAddr,srcLen);
	    ret = nand_write_skip_bad(&nand_info[0], destAddr, &srcLen,
							  (u_char *)srcAddr);
	    printf(" write 0x%08x bytes: %s\n", srcLen, ret ? "ERROR" : "OK");		
	  
	   rootfs_programming = 0;		
   }else{
     nand_write_partial(&nand_info[0], destAddr, &srcLen, (u_char*)srcAddr);
   }
 #else
   nand_flash_write(srcAddr, destAddr, srcLen);       
   puts ("NAND write done\n");
 #endif                                                           

#endif
	return 0;
}

int FindPartBoundary(ulong img_addr,ulong *curpart_begin,ulong *nextpart_begin)
{
	char strPartName[16];
	ulong part_begin_addr[MAX_PARTITION];
	int nPartNo,i;

	nPartNo = simple_strtoul((char *)getenv("total_part"),NULL,10);
	if(nPartNo <= 0 || nPartNo >= MAX_PARTITION){
		printf("Total no. of current partitions [%d] is out of limit (0,%d)\n",MAX_PARTITION);
		return 1;
	}

	for(i = 0; i < nPartNo; i++){
		memset(strPartName,0x00,sizeof(strPartName));
		sprintf(strPartName,"part%d_begin",i);
		part_begin_addr[i] = simple_strtoul((char *)getenv(strPartName),NULL,16);
	}

	part_begin_addr[i] = simple_strtoul((char *)getenv("flash_end"),NULL,16) + 1;

	for(i = 0; i < nPartNo; i++){
		if(img_addr >= part_begin_addr[i] && img_addr < part_begin_addr[i+1]){
			*curpart_begin = part_begin_addr[i];
			*nextpart_begin = part_begin_addr[i+1];
			return 0;
		}
	}

	printf("The begining of the image to be programmed [0x%08lx] is not within current patition boundary\n",img_addr);
	return 1;
}

int FindNPImgLoc(ulong img_addr,ulong *nextStartAddr,ulong *preEndAddr)
{
	ulong Img_startAddr[MAX_DATABLOCK];
	ulong Img_size[MAX_DATABLOCK];
	ulong nDBNo;
	char strDBName[MAX_DATABLOCK][32];
	char strTemp[32];
	char *strT;
	ulong curpart_begin,nextpart_begin;
	int i;

	nDBNo = simple_strtoul((char *)getenv("total_db"),NULL,10);
	if(nDBNo <= 0 || nDBNo >= MAX_DATABLOCK){
		printf("Total no. of current data blocks [%d] is out of limit (0,%d)\n",nDBNo,MAX_PARTITION);
		return 1;
	}
	
	if(FindPartBoundary(img_addr,&curpart_begin,&nextpart_begin))
		return 1;

	upgrade_debug_printf("For the image address 0x%08lx, partition boundary is found as [0x%08lx,0x%08lx]\n",img_addr,curpart_begin,nextpart_begin);
	*nextStartAddr = nextpart_begin;
	*preEndAddr = curpart_begin;

	for(i = 0; i < nDBNo; i++){
		memset(strDBName[i],0x00,sizeof(strDBName[i]));
		memset(strTemp,0x00,sizeof(strTemp));
		sprintf(strTemp,"data_block%d",i);
		strcpy(strDBName[i],(char *)getenv(strTemp));
		if(strcmp(strDBName[i],"") == 0){
			printf("Variable %s is not set\n",strTemp);
			return 1;
		}
		upgrade_debug_printf("strDBName[%d]:[%s]\n",i,strDBName[i]);
	}
	
	for(i = 0; i < nDBNo; i++){
		memset(strTemp,0x00,sizeof(strTemp));
		strT = NULL;
		sprintf(strTemp,"f_%s_addr",strDBName[i]);
		strT = (char *)getenv(strTemp);
		if(strT == NULL){
			printf("Variable %s is not set\n",strTemp);
			return 1;
		}
		Img_startAddr[i] = simple_strtoul((char *)strT,NULL,16);
		upgrade_debug_printf("%s: 0x%08x\n", strTemp, Img_startAddr[i]);

		memset(strTemp,0x00,sizeof(strTemp));
		strT = NULL;
		sprintf(strTemp,"f_%s_size",strDBName[i]);
		strT = (char *)getenv(strTemp);
		if(strT == NULL){
			printf("Variable %s is not set\n",strTemp);
			return 1;
		}
		Img_size[i] = simple_strtoul((char *)strT,NULL,16);
		upgrade_debug_printf("%s: 0x%08x\n", strTemp, Img_size[i]);
	}

	for(i = 0; i < nDBNo; i++){
		if(Img_startAddr[i] > img_addr && Img_startAddr[i] < *nextStartAddr)
			*nextStartAddr = Img_startAddr[i];
		if(Img_startAddr[i] + Img_size[i] < img_addr && Img_startAddr[i] + Img_size[i] > *preEndAddr)
			*preEndAddr = Img_startAddr[i] + Img_size[i];
	}
	upgrade_debug_printf("For img_addr 0x%08lx, nextStartAddr 0x%08lx and preEndAddr 0x%08lx\n",img_addr,*nextStartAddr,*preEndAddr);
	return 0;
}

int upgrade_img(ulong srcAddr, ulong srcLen, char *imgName, enum ExpandDir dir, int bSaveEnv)
{
	ulong img_addr,img_size,img_endaddr;
	char newimg_para[32];
	char strimg_addr[32],strimg_size[32];
	ulong nextStartAddr,preEndAddr;
	image_header_t *pimg_header = NULL;
	char *srcData_Copy=NULL;
  #ifdef CONFIG_BOOT_FROM_NAND
	ulong allocsize;
  #endif

	memset(strimg_addr,0x00,sizeof(strimg_addr));
	sprintf(strimg_addr,"f_%s_addr",imgName);
	sprintf(strimg_size,"f_%s_size",imgName);
	img_addr = simple_strtoul((char *)getenv(strimg_addr),NULL,16);
	if (img_addr == 0) {
		printf("The environment variable %s not found\n",strimg_addr);
		return 1;
	}

	if (FindNPImgLoc(img_addr,&nextStartAddr,&preEndAddr))
		return 1;
	pimg_header = (image_header_t *)srcAddr;
	if(pimg_header->ih_magic == IH_MAGIC)
	{
		printf("Image contains header with name [%s]\n",pimg_header->ih_name);
		if(pimg_header->ih_type != IH_TYPE_KERNEL)
		{
			upgrade_debug_printf("This is not kernel image and so removing header\n");
			srcAddr += sizeof(*pimg_header);
			srcLen -= sizeof(*pimg_header);
		}
		img_size = simple_strtoul((char *)getenv(strimg_size),NULL,16); //509061:tc.chen
	}
	else if (!strcmp(imgName,"uboot"))
	{
		img_size = simple_strtoul((char *)getenv(strimg_size),NULL,16); //509061:tc.chen
	}
	else
	{
		struct conf_header *header;
#if !defined(BUILD_FROM_IFX_UTILITIES)
		srcData_Copy = srcAddr;
		memmove(srcData_Copy + sizeof(struct conf_header) ,(void *)srcAddr,srcLen);
#else
	  #ifdef CONFIG_BOOT_FROM_NAND
		allocsize = (srcLen + sizeof(struct conf_header) + NAND_BLK_SIZE - 1) / NAND_BLK_SIZE * NAND_BLK_SIZE;
		srcData_Copy = malloc( allocsize );
		memset( srcData_Copy + allocsize - NAND_BLK_SIZE, 0xff, NAND_BLK_SIZE );
	  #else
		srcData_Copy = malloc(srcLen + sizeof(struct conf_header));
	  #endif
		memcpy(srcData_Copy+sizeof(struct conf_header),(void*)srcAddr,srcLen);
#endif
		DBG_dump("srcData_Copy:\n", srcData_Copy, 24);
		header = (struct conf_header *)(srcData_Copy);
		header->size = srcLen;
#if defined(BUILD_FROM_IFX_UTILITIES)
		header->crc = 0 ^ 0xffffffffL;
#else
		header->crc = 0;
#endif
		header->crc = crc32(header->crc,srcData_Copy+sizeof(struct conf_header),srcLen);
#if defined(BUILD_FROM_IFX_UTILITIES)
		header->crc ^= 0xffffffffL;
#endif
		DBG_dump("srcData_Copy:\n", srcData_Copy, 32);
	}	

	if (dir == FORWARD)
	{
		if (img_addr + srcLen > nextStartAddr)
		{
			printf("Cannot upgrade image %s.\n Error : From start address 0x%08lx, the new size %d is bigger than begining of next image/end of the partition 0x%08lx\n",strimg_addr,img_addr,srcLen,nextStartAddr);
			return 1;
		}
		upgrade_debug_printf("Programming for FORWARD direction\n");
	}
	else if (dir == BACKWARD)
	{
		img_endaddr = nextStartAddr - 1;
		if(img_endaddr - srcLen < preEndAddr)
		{
			printf("Cannot upgrade image %s.\n Error : From end address 0x%08lx, the new size %d is bigger than end of previous image/begining of the partition 0x%08lx\n",strimg_addr,img_endaddr,srcLen,preEndAddr);
			return 1;
		}

		img_addr = img_endaddr - srcLen + 1;
		img_addr = (img_addr/16)*16;

		upgrade_debug_printf("Programming for BACKWARD direction\n");
	}
	else
	{
		printf("The expansion direction [%d] is invalid\n",dir);
		return 1;
	}
	if (srcData_Copy)
	{
	  #if defined(BUILD_FROM_IFX_UTILITIES) && defined(CONFIG_BOOT_FROM_NAND)
		if (program_img((ulong)srcData_Copy,allocsize,img_addr))
	  #else
		if (program_img((ulong)srcData_Copy,srcLen+sizeof(struct conf_header),img_addr))
	  #endif
		{
		  #if defined(BUILD_FROM_IFX_UTILITIES)
			free(srcData_Copy);
		  #endif
			return 1;
		}
	}
	else
	{
		if (program_img(srcAddr,srcLen,img_addr)) {
			return 1;
		}
	}
	
#if !defined(BUILD_FROM_IFX_UTILITIES)
	if (strcmp(strimg_addr,"f_uboot_addr") == 0)
	{
		ulong erase_addr1=0, erase_addr2=0;
		erase_addr1 = simple_strtoul((char *)getenv("f_firmware_addr"),NULL,16);
		erase_addr2 = simple_strtoul((char *)getenv("flash_end"),NULL,16);
#if defined(CFG_BOOT_FROM_NOR) || defined(BUILD_FROM_IFX_UTILITIES)
		flash_sect_erase(erase_addr1, erase_addr2, 1);
#elif defined(CFG_BOOT_FROM_SPI)
		{
			ulong length=erase_addr2-erase_addr1+1;
			memset(0x80400000, 0xFF, length);
			eeprom_write (NULL, erase_addr1, (uchar *)0x80400000, length);
		}
#endif
		do_reset (NULL, 0, 0, NULL);
		return 0;
	}
#endif

	memset(newimg_para,0x00,sizeof(newimg_para));
	sprintf(newimg_para,"0x%08lx",srcLen);
	setenv(strimg_size,newimg_para);
	upgrade_debug_printf("New variables %s = %s set\n",strimg_size,newimg_para);

	memset(newimg_para,0x00,sizeof(newimg_para));
	sprintf(newimg_para,"0x%08lx",img_addr);
	setenv(strimg_addr,newimg_para);
	upgrade_debug_printf("New variables %s = %s set\n",strimg_addr,newimg_para);

	if (strcmp(strimg_addr,"f_kernel_addr") == 0)
	{
		setenv("kernel_addr",newimg_para);
		upgrade_debug_printf("New variables kernel_addr = %s set\n",newimg_para);
	}

	if (strcmp(strimg_addr,"f_rootfs_addr") == 0)
	{
		memset(newimg_para,0x00,sizeof(newimg_para));
		sprintf(newimg_para,"0x%08lx",img_addr + srcLen);
		setenv("f_rootfs_end",newimg_para);
		upgrade_debug_printf("New variables kernel_addr = %s set\n",newimg_para);
	}

	if (bSaveEnv)
	{
		saveenv();
#ifdef UBOOT_ENV_COPY
		saveenv_copy();
#endif //UBOOT_ENV_COPY
	}
#if defined(BUILD_FROM_IFX_UTILITIES)
	if (srcData_Copy)
		free(srcData_Copy);
#endif
	return 0;
}

