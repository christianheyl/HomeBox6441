#include <config.h>
#include <common.h>
#include <command.h>
#include <asm/addrspace.h>
#include <asm/ar10.h>
#include <environment.h>
#include <nand.h>

#include "ddr.h"

#ifdef CONFIG_GRX390_CPUFREQ_AUTO_DETECT
SET_DDR_VALUE(360M)
SET_DDR_VALUE(300M)
static ddr_t *ddr;
#endif

void nand_gpio_init(void)
{
     /* GPIO 13 NAND_ALE*/
     *AR10_GPIO_P0_DIR     |= 1 << 13;
     *AR10_GPIO_P0_ALTSEL0 |= 1 << 13;
     *AR10_GPIO_P0_ALTSEL1 &= ~(1 << 13);
     *AR10_GPIO_P0_OD      |= 1 << 13;

     /*GPIO 24 NAND_CLE */
     *AR10_GPIO_P1_DIR     |= 1 << 8;
     *AR10_GPIO_P1_ALTSEL0 |= 1 << 8;
     *AR10_GPIO_P1_ALTSEL1 &= ~(1 << 8);
     *AR10_GPIO_P1_OD      |= 1 << 8;

      /*GPIO 48, NAND Ready_Busy*/
      /*GPIO 49, NAND_RD*/
      /*GPIO 50, NAND_D1*/
      /*GPIO 51, NAND_D0*/
      /*GPIO 52, NAND_D2*/
      /*GPIO 53, NAND_D2*/
      /*GPIO 54, NAND_D6*/
      /*GPIO 55, NAND_D5*/
      /*GPIO 56, NAND_D5*/
      /*GPIO 57, NAND_D3*/
      /*GPIO 59, NAND_WR*/
      /*GPIO 60, NAND_WP*/
      /*GPIO 61, NAND_SE*/
      *AR10_GPIO_P3_DIR     = 0x3ffe;
      *AR10_GPIO_P3_ALTSEL0 = 0x3fff;
      *AR10_GPIO_P3_ALTSEL1 = 0;
      *AR10_GPIO_P3_OD      = 0x3ffe;
}																																	   


void *malloc(unsigned int size)
{
   
    return NULL;
}



void init_ddr(void)
{
    int i;
    REG32(0xBF106c98) = 0x94; /*interdcdc 1v8*/


    REG32(0xBF801000) = 0x0 ; /*Put DDR controller inactive*/
    for(i=0;i<66;i++){
        REG32(ddr[i].addr)=ddr[i].value;
    }
    /*Enable memory controller*/
    REG32(0xBF801000) = 0x401;
    while(!(REG32(0xBF8014B0)&0x1) || !(REG32(0xBF8014E0)&0x1));
    
	/*prioritize WLAN DDR access to highest in DDR placement logic design*/
	REG32(0xBF8001D0) = 0xFCF;

}
										


#ifndef CONFIG_NAND_ECC_HW_REED_SOLOMON


static void nand_read_page(u32 page_addr, u32 dest_addr)
{
   int i;
   u8 *tmp;
   u8 col_addr_num;
     u8 page_addr_num;

   if(CONFIG_NAND_PAGE_SIZE<=0x200){
     col_addr_num=1;

     if(CONFIG_NAND_FLASH_SIZE<32){
        page_addr_num=2;
      }else
        page_addr_num=3;
   }else{
     col_addr_num=2;

     if(CONFIG_NAND_FLASH_SIZE<128){
          page_addr_num=2;
      }else{
        page_addr_num=3;
      }
   }
   NAND_CE_SET;

   NAND_SETCLE;
   NAND_WRITE(WRITE_CMD,0);
   NAND_CLRCLE;
   NAND_SETALE;
   for(i=0;i<col_addr_num;i++){
    NAND_WRITE(WRITE_ADDR,0);
  }
   for(i=0;i<page_addr_num;i++){
     NAND_WRITE(WRITE_ADDR,(u8)((page_addr>>(i*8)) & 0xff ));
   }
   NAND_CLRALE;

   NAND_SETCLE;
   if(CONFIG_NAND_PAGE_SIZE>0x200){
     NAND_WRITE(WRITE_CMD,0x30);
   }
  NAND_CLRCLE;
  while(!NAND_READY){}

   /* Read page */
   tmp = (u8*)dest_addr;
   for (i = 0; i < CONFIG_NAND_PAGE_SIZE; i++)
   {
     NAND_READ(READ_DATA, *tmp++);
   }
   NAND_CE_CLEAR;

   while(!NAND_READY){}
}


void nand_spl_init(void)
{
	ulong  buffer[8];
	u8    page_buf[CONFIG_NAND_PAGE_SIZE];
	ulong ddr_magic=0x88888888;
    int i;	
	ulong ecc;
	asm("sync");
	/* Terry 20141221, read g_serial_key from NAND flash */
#if 1
	load_serial_key(page_buf);
#endif
    serial_init();
    buffer[0] = 0;

#ifdef CONFIG_GRX390_CPUFREQ_AUTO_DETECT
	if (REG32(AR10_ID_CFG) & 0x20000) 
		ddr = &ddr_300M[0];
	else
		ddr  = &ddr_360M[0];
#endif /* CONFIG_GRX390_CPUFREQ_AUTO_DETECT */

#ifdef CONFIG_TUNE_DDR    
	nand_read_page(((IFX_CFG_FLASH_DDR_CFG_END_ADDR+1)/CONFIG_NAND_PAGE_SIZE)-1,page_buf);
	   
        for(i=0;i<8;i++){
           buffer[i] = *(volatile u32*)(page_buf+CONFIG_NAND_PAGE_SIZE-32+i*4); 
    }
    
    if(buffer[0]==ddr_magic)
       {
             ecc=buffer[1]^buffer[2]^buffer[3]^buffer[4]^buffer[5]^buffer[6];
               if(ecc!=buffer[7]){
                  REG32(CONFIG_TUNING_STATUS)=0xff;
                }else{
                    REG32(CONFIG_TUNING_STATUS)=0;
                }
         }
     else{
        REG32(CONFIG_TUNING_STATUS)=0xff;
    }

    if(REG32(CONFIG_TUNING_STATUS)!=0xff){
          ddr[62].value = buffer[1]; /*PHYR6*/
          ddr[64].value = buffer[2]; /*PHYR8*/
          ddr[63].value = buffer[3]; /*PHYR7*/
          ddr[65].value = buffer[4]; /*PHYR9*/
          REG32(CSS_DDR_ECHO_DLL0) = buffer[5];
          REG32(CSS_DDR_ECHO_DLL1) = buffer[6];
      }
      asm("sync");
      
#endif
	   init_ddr(); 
#ifdef CONFIG_TUNE_DDR	  
	   tune_ddr();
#endif	   
	   nand_boot();
	
}

/* Terry 20150903, procedure to load serial key */
/* Terry TODO use 3th block's first page */
#if 1
static int verify_serial_key(unsigned char *page_buf, int key_len) {
	unsigned int *key_ptr = (unsigned int *)page_buf;
	unsigned int x_data = 0;
	int key_num = key_len / sizeof(unsigned int);
	int i;
	int is_invalid = 1;
	
	/* key_ptr[key_num - 1] is verify code */
	for (i = 0; i < (key_num - 1); i++) {
		x_data ^= key_ptr[i];
		if (key_ptr[i] != 0xffffffff && key_ptr[i] != 0x0)
			is_invalid = 0;
	}

	if (is_invalid == 1)
		return -1;

	return ((x_data == key_ptr[i]) ? 0 : -1);
}

void load_serial_key(unsigned char *page_buf) {
	volatile extern unsigned int g_serial_key[];
	int min_block_id = 4; /* 0 based */
	int max_block_id = 7;
	int key_len = 148; // 144+4, 36*4=144
	int i = 0;

	/* nand erase 0x80000 0x90 */
	/* must check serial key before erase it */
	for (i = min_block_id; i <= max_block_id; i++) {
		nand_read_page((CONFIG_NAND_BLOCK_SIZE * i) / CONFIG_NAND_PAGE_SIZE, page_buf);
		if (verify_serial_key(page_buf, key_len) == 0) {
			memcpy(g_serial_key, page_buf, key_len - 4); /* Bypass checking code */
			return;
		}
	}
	memset((char *)g_serial_key, 0xff, key_len - 4);
}
#endif

#else


void nand_spl_init(void)
{
    serial_init();
    nand_boot();
}
	

#endif

