/*
 *	Copyright 2009 Lantiq
 */


#include <common.h>
#include <command.h>
#include <net.h>
#include "tcp.h"
#include "http-strings.h"
#include "httpd-fsdata.h"
#include "httpd-fsdata.c"
//#include "httpd.h"
#include "linux/mtd/nand.h"

#ifdef CONFIG_CMD_HTTPD
#undef ET_DEBUG
#undef CONTENT_LENGTH_DEBUG


#define ISO_nl      0x0a
#define ISO_return  0x0d
#define ISO_space   0x20
#define ISO_bang    0x21
#define ISO_percent 0x25
#define ISO_period  0x2e
#define ISO_slash   0x2f
#define ISO_colon   0x3a

static int post_flag=0;
static int post_content_length=0;
static int post_found_1st_crlfcrlf_flag=0;
static int post_content_count=0;
static int post_current_pos=0;
static int post_file_pos=0;
static int post_file_length=0;
static char* post_file_p=NULL;


static char post_boundary[100];
static int post_boundary_found=0;

/**********************************************************************/
/* from trxhdr.h */

#define TRX_MAGIC	0x30524448	/* "HDR0" */
#define TRX_MAX_LEN	0x4400000
#define TRX_NO_HEADER	1		/* Do not write TRX header */	

struct trx_header {
	uint32_t magic;			/* "HDR0" */
	uint32_t len;			/* Length of file including header */
	uint32_t crc32;			/* 32-bit CRC from flag_version to end of file */
	uint32_t flag_version;	/* 0:15 flags, 16:31 version */
	uint32_t offsets[4];	/* Offsets of partitions from start of header */
};

/**********************************************************************/

extern int http_upgrade(ulong srcAddr, int srcLen);

static int sleep_msec( u32 delay )
{
	ulong start = get_timer(0);

	while (get_timer(start) < delay) {
		if (ctrlc ()) {
			return (-1);
		}
		udelay (100);
	}

	return 0;
}

/* Jess Hsieh, to support HTTPD download buffer. */
unsigned long relocate_heap_base=0;
static unsigned long board_obtain_loading_buffer_addr(unsigned long need_space_size){
	unsigned long base_addr, diff_size, base_size=(CONFIG_IFX_MEMORY_SIZE * 1024 * 1024);
	char *s;
	if ((s = getenv ("loadaddr")) != NULL) {
		load_addr = simple_strtoul (s, NULL, 16);
	}

	base_size/=4;
	base_addr=load_addr+base_size;
	diff_size=relocate_heap_base-base_addr;
	if(diff_size<need_space_size){
		base_addr=load_addr;
	}
	// printf("%s[%d]... base_addr %08X, relocate_heap_base: %08X\n",__FUNCTION__,__LINE__,base_addr,relocate_heap_base);
	return base_addr;
}


static void
clear_buf(u8* data, int len)
{
	int i;
	for(i=0;i<len;i++)
	{
		*(data+i)=0;
	}
	return;
}

static void
print_buf(u8* data, int len)
{
	int i;
	for(i=0;i<len;i++)
	{
		if(!(i%16)) printf("\n");
		printf("%02x ",*(data+i));
	}
	printf("\n");
	return;
	
}

static int
readto(struct tcp_appstate *s, char c)
{
	 int i;
	 char value;
	 
	 // printf("readto %02x\n",c);
	 for (i = 0; i < ((s->len) - (s->count)); i++) {
	     value = *(s->buf + s->count + i);
	     // printf("%02x ",value);
		 if (i > 500) printf("[%s] inputbuf buffer overflow %d\n", __FUNCTION__, i);
	     s->inputbuf[i] = value;
	     if (value == c)
			 break;
	 }
	 s->count += i + 1;
	 // printf("\n");
	 return i + 1;
}


/* Terry 20141216, Read data from buf to inputbuf */
/* memcpy(&inputbuf[0], s->buf + s->count, s->len - s->count) */
/* Until c found */
static int
readto_content_length(struct tcp_appstate *s, char c)
{
	int i;
	char value;

	if (((s->len) - (s->count)) <= 0)
		return 0;

	for (i = 0; i < ((s->len) - (s->count)); i++) {
		value = *(s->buf + s->count + i);
		s->inputbuf[i] = value;
		if (i > 500) printf("[%s] inputbuf buffer overflow %d\n", __FUNCTION__, i);
		if (value == c) {
			s->count += i + 1;
			return i + 1;
		}
	}
	s->count += i;
	
	return i;
}

static u8 httpd_fs_strcmp(const char *str1, const char *str2)
{
  u8 i;
  i = 0;
loop:

  if(str2[i] == 0 ||
	 str1[i] == '\r' ||
	 str1[i] == '\n') {
				     return 0;
					   }

  if(str1[i] != str2[i]) {
				     return 1;
					   }

   ++i;
   goto loop;
}
								   

int httpd_fs_open(const char *name, struct httpd_fs_file *file)
{
    struct httpd_fsdata_file_noconst *f;

	  for(f = (struct httpd_fsdata_file_noconst *)HTTPD_FS_ROOT;
          f != NULL;
	      f = (struct httpd_fsdata_file_noconst *)f->next) {
	      	if(httpd_fs_strcmp(name, f->name) == 0) {
		        file->data = f->data;
   	            file->len = f->len;
				return 1;
			   }
      	    }
	  return 0;
}
														  

static void send_data(struct tcp_appstate *s, const char *data, int len)
{
	int slen;
	int count=0;

  if(len==0){/*no data, means FIN*/
  	 tcp_send_data(0,0); 
     return;
  }
	while(len>0){
	//printf("send_data!\n");
	//print_buf(data,len);
	 if(len>tcp_conn->mss){
	 	  slen=tcp_conn->mss;
	  }else{
	  	slen=len;
	  }
	 len-=slen;  
	 tcp_send_data(data+count, slen);
	 count+=slen;
  }

  //tcp_send_data(data, len);
	return;
}

static void send_headers(struct tcp_appstate *s, const char *statushdr)
{
	 char *ptr;


   send_data(s, statushdr, strlen(statushdr));

  ptr = strrchr(s->filename, ISO_period);

  if(ptr == NULL) {
    send_data(s, http_content_type_binary, strlen(http_content_type_binary));
  } else if(strncmp(http_html, ptr, 5) == 0 ||
            strncmp(http_shtml, ptr, 6) == 0) {
    send_data(s, http_content_type_html, strlen(http_content_type_html));
  }else if(strncmp(http_css, ptr, 4) == 0) {
    send_data(s, http_content_type_css, strlen(http_content_type_css));
  } else if(strncmp(http_png, ptr, 4) == 0) {
    send_data(s, http_content_type_png, strlen(http_content_type_png));
  } else if(strncmp(http_gif, ptr, 4) == 0) {
    send_data(s, http_content_type_gif, strlen(http_content_type_gif));
  } else if(strncmp(http_jpg, ptr, 4) == 0) {
    send_data(s, http_content_type_jpg, strlen(http_content_type_jpg));
  }else {
    send_data(s, http_content_type_plain, strlen(http_content_type_plain));
  }
  
}

static void send_file(struct tcp_appstate *s)
{
	 send_data(s, s->file.data, s->file.len);

}

static int find_value(char* str, int len, struct tcp_appstate *s)
{
	int flag=0;
	int read_len=0;
	
	printf("find value %s\n", str);
	do {
	 	clear_buf(s->inputbuf, TCP_MAX_INPUT_BUFFER_SIZE);
		read_len = readto(s, ISO_nl);
		
		if (strncmp(str, s->inputbuf, len) == 0) {
			printf("readlen=%d\n", read_len);
			printf("%s\n", s->inputbuf);
			flag = 1;
			break;
		}
	} while ((s->count) < (s->len));
	
	return flag; 
}

#ifdef CONTENT_LENGTH_DEBUG
static int isprint(int c)
{
	return ((c >= 0x20) && (c < 0x7f));
}

static void printHex(unsigned char *buf, int size) { 
	int x, y; 
	
	for( x=1; x<=size; x++ ) { 
		if( x == 1 ) printf( "0x%08X  ", buf + (x-1) ); /* Print an offset line header */ 
		printf( "%02X ", buf[x-1] ); /* print the hex value */ 
		if( x % 8 == 0 ) printf( " " ); /* padding space at 8 and 16 bytes */ 
		if( x % 16 == 0 ) { 
			/* We're at the end of a line of hex, print the printables */ 
			printf( " " ); 
			for( y = x - 15; y <= x; y++ ) { 
				if( isprint( buf[y-1] ) ) printf( "%c", buf[y-1] ); /* if it's printable, print it */ 
				else printf( "." ); /* otherwise substitute a period */ 
				if( y % 8 == 0 ) printf( " " ); /* 8 byte padding space */ 
			} 
			if( x < size ) printf( "\n0x%08X  ", buf + x ); /* Print an offset line header */ 
		} 
	} 
	x--; 
    /* If we didn't end on a 16 byte boundary, print some placeholder spaces before printing ascii */ 
    if( x % 16 != 0 ) { 
        for( y = x+1; y <= x + (16-(x % 16)); y++ ) { 
            printf( "   " ); /* hex value placeholder spaces */ 
            if( y % 8 == 0 ) printf( " " ); /* 8 and 16 byte padding spaces */ 
        }; 
        /* print the printables */ 
        printf( " " ); 
        for( y = (x+1) - (x % 16); y <= x; y++ ) { 
            if( isprint( buf[y-1] ) ) printf( "%c", buf[y-1] ); /* if it's printable, print it */ 
            else printf( "." ); /* otherwise substitute a period */ 
            if( y % 8 == 0 ) printf( " " ); /* 8 and 16 byte padding space */ 
        } 
    } 
	/* Done! */ 
	printf( "\n" ); 
}
#endif

static char g_tmpbuf[256];
static int g_tmpbuf_len;

static int find_value_content_length(char* str, int len, struct tcp_appstate *s, int *out_content_length)
{
	char tmp_c;
	int i;
	int flag = 0;
	int read_len = 0;
	
#ifdef CONTENT_LENGTH_DEBUG
	printf("[%s] find content length value %s\n", __FUNCTION__, str);
#endif
	
	do {
		/* Clean line buffer */
	 	clear_buf(s->inputbuf, TCP_MAX_INPUT_BUFFER_SIZE);
		
		/* Terry 20141216, if no characters copied (s->inputbuf is empty) */
		/* Read one line from the buffer to inputbuf */
		if ((read_len = readto_content_length(s, ISO_nl)) == 0) {
			printf("[%s] readto_content_length return 0.\n", __FUNCTION__);
			break;
		}

#ifdef CONTENT_LENGTH_DEBUG
		printHex(s->inputbuf, read_len);
		printf("[%s] read_len %d\n", __FUNCTION__, read_len);
#endif

		/* Terry 20141216, if ISO_nl not found in the buffer */
		/* s->inputbuf is not empty, need to copy to tmp buffer */
		if (s->inputbuf[read_len - 1] != ISO_nl) {
#ifdef CONTENT_LENGTH_DEBUG
			printf("[%s] inputbuf not end of ISO_nl.\n", __FUNCTION__);
#endif
			break;
		}

#ifdef CONTENT_LENGTH_DEBUG
		printf("[%s] g_tmpbuf_len %d\n", __FUNCTION__, g_tmpbuf_len);
#endif

		if (g_tmpbuf_len > 0) {
			if (g_tmpbuf_len + read_len > sizeof(g_tmpbuf)) {
				printf("[%s] Content-Length is out of packets buffer.\n", __FUNCTION__);
				g_tmpbuf_len = 0;
				
				return 0;
			}
			memcpy(&g_tmpbuf[g_tmpbuf_len], &s->inputbuf, read_len);
			g_tmpbuf_len += read_len;
#ifdef CONTENT_LENGTH_DEBUG
			printf("[%s] g_tmpbuf_len %d\n", __FUNCTION__, g_tmpbuf_len);
			printHex(g_tmpbuf, g_tmpbuf_len);
#endif
			
			if (strncmp(str, g_tmpbuf, len) == 0) {
#ifdef CONTENT_LENGTH_DEBUG
				printf("readlen(tmpbuf)=%d\n", g_tmpbuf_len);
				printf("%s(tmpbuf)\n", g_tmpbuf);
#endif
				for (i = 0; (i + len + 2) < g_tmpbuf_len; i++) {
					tmp_c = g_tmpbuf[len + 2 + i];
					if (ISO_return == tmp_c)
						break;
					*out_content_length = (*out_content_length * 10) + (tmp_c - 0x30);     		
				}
				g_tmpbuf_len = 0;
				flag = 1;
				break;
			}
		} else {
			/* g_tmpbuf_len == 0 */
			if (strncmp(str, s->inputbuf, len) == 0) {
#ifdef CONTENT_LENGTH_DEBUG
				printf("readlen=%d\n", read_len);
				printf("%s\n", s->inputbuf);
#endif
				for (i = 0; (i + len + 2) < read_len; i++) {
					tmp_c = s->inputbuf[len + 2 + i];
					if (ISO_return == tmp_c)
						break;
					*out_content_length = (*out_content_length * 10) + (tmp_c - 0x30);     		
				}			
				flag = 1;
				break;
			}
		}
	} while ((s->count) < (s->len));
	
	if (0 == flag && read_len > 0) {
#ifdef CONTENT_LENGTH_DEBUG
		printf("[%s] 'Content-Length: value+CRLF' not matched, enqueue temporary buffer.\n", __FUNCTION__);
#endif
		g_tmpbuf_len = 0;
		if (g_tmpbuf_len + read_len > sizeof(g_tmpbuf) - 1) {
			printf("[%s] Content-Length is out of packets buffer.\n", __FUNCTION__);
			g_tmpbuf_len = 0;
			
			return 0;
		}
		memcpy(&g_tmpbuf[g_tmpbuf_len], &s->inputbuf, read_len);
		g_tmpbuf_len += read_len;
	}
	
	return flag; 
}

static int find_post_file_length(struct tcp_appstate *s)
{
	char *end = NULL;
	char *start = s->buf + s->count;
	int total_len = s->len - s->count;
	int i;
	
	for (i = 0; i < total_len; i++) {
		end = s->buf + s->count + i;
		if (*end == 0x2d && *(end + 1) == 0x2d) {
			if (strncmp(end + 2, post_boundary, strlen(post_boundary) - 2) == 0 ) { /* Remove crlf */
				break;
			}
		}
	}
	end -= 2; /*remove 2 bytes for CRLF*/
	
	return end - start;
}

static int
move_to_dual_CRLF(struct tcp_appstate *s)
{
	int i;
	for(i=0;i<s->len-s->count;i++)
      	{
      		
      		if(s->buf[s->count+i]==ISO_return &&
      			 s->buf[s->count+i+1]==ISO_nl &&
      			 s->buf[s->count+i+2]==ISO_return &&
      			 s->buf[s->count+i+3]==ISO_nl){
      			 	s->count+=i;
     			 	 return 1; // Jess Hsieh Modified
        	}
      	}
	return 0; // Jess Hsieh Modified
}

static char new_index_html_bf[4 * 1024];  /* Jess Hsieh */
char isLogined = 0;
static void
handle_output(struct tcp_appstate *s)
{
	int flag;
	unsigned long base_addr;
	char *pw, *and_sign, pw_buf[21];
	/*	char new_index_html_bf[2000];   Jess Hsieh, remove it to avoid stack overflow */  
	//printf("handle output!\n");


	/*	There are 4 actions can be executed by U-boot web gui
 		1). Reboot 2). Factory Default 3). Firmware / U-boot Upgrade 4). Login
		Misora @ 20120801		*/
	if (post_flag) {
		if (strstr(s->buf, "auth") != NULL) {	
			printf("Do authentication...\n");
			/* Do authentication. Misora @ 20120730	*/
			pw = strstr(s->buf, "pw");
			if(pw!=NULL){
				and_sign = strstr(pw, "&");
				if(((and_sign-pw)-3) == 6){
					strncpy(pw_buf, pw+3, 6);
					pw_buf[6] = 0;
					/* Terry 20140526, TODO, remove authorization */
#if 0
					printf("Your input is %s, PIN code is %s\n", pw_buf, getenv_manuf("pin_code"));
					if(!strncmp(pw_buf, getenv_manuf("pin_code"), 6)){
						printf("Authenticated!!!\n");
						isLogined = 1;
						httpd_fs_open("/index.html", &s->file);
						strcpy(s->filename, "/index.html");
					} else {
						printf("Fail!\n");
						isLogined = 0;
					}
#endif
				} else {
					printf("Passord length should be 6, your input is %d\n", ((and_sign-pw)-3));
					isLogined = 0;
				}
			} else {
				printf("Need password.\n");
				isLogined = 0;
			}
			post_flag=0;
	  	  	post_content_length=0;
	      	post_content_count=0;
	      	post_current_pos=0;
	      	post_boundary_found=0;
	      	post_file_length=0;
			post_found_1st_crlfcrlf_flag=0;
	      	post_file_p=NULL;  
	      	s->len=0;
	      	s->count=0;
			goto EXIT;
		} else if (strstr(s->buf, "action=reboot") != NULL) {	
			httpd_fs_open(http_wait_html, &s->file);
			strcpy(s->filename, http_wait_html);
			send_headers(s, http_header_200);
			send_file(s);
			send_data(s, 0, 0); 

			/* 	Misora @ 20120801
				Delay task by 1 sec here so that wait.html has enough time to be sent to client.	*/
			sleep_msec(1000);
			
			run_command("reset", 0);
			s->len=0;
			s->count=0;
			return;
		} else if (strstr(s->buf, "action=default") != NULL) {			
			// printf("%s[%d]...\n",__FUNCTION__,__LINE__);
			httpd_fs_open(http_wait_html, &s->file);
			strcpy(s->filename, http_wait_html);
			send_headers(s, http_header_200);
			send_file(s);
			send_data(s, 0, 0); 
			/* Jess Hsieh : 
			  * 	Modified with the fixed size, since we assume Primary and secondary was consecutive address and PHYSICAL SIZE was 1MB per each MTD bank.
			  * 	Though we just need 128KB for each Configuration Partition.
			  *    DT724, UBOOT_CFG, UBOOT_BAKCFG, SYS_PRICFG, SYS_SECCFG, was record with contiguous addresses.
			  */
#if 1
			run_command("nand erase glbcfg", 0);
#else
			run_command("nand erase $(f_ubootcfg_addr) 0x400000", 0);
#endif
		} else if (strstr(s->buf, "action=factory") != NULL) {
			httpd_fs_open("/wait_factory.html", &s->file);
			strcpy(s->filename, "/wait_factory.html");
			send_headers(s, http_header_200);
			send_file(s);
			send_data(s, 0, 0);
			
			if (strstr(s->buf, "opType=0")) {
				printf("[%s] active BankA\n", __FUNCTION__);
				setenv("failcount_A", "0");
				setenv("update_chk", "0");
				saveenv();
			} else if (strstr(s->buf, "opType=1")) {
				printf("[%s] active BankB\n", __FUNCTION__);
				setenv("failcount_B", "0");
				setenv("update_chk", "2");
				saveenv();
			} else if (strstr(s->buf, "opType=2")) {
				printf("[%s] clone BankA to BankB\n", __FUNCTION__);
				run_command("run ubi_init", 0);
				do {
					run_command("run switchbankB", 0);
					/* Kernel */
					if (ubi_volume_read("kernelA", s->buf, 0x600000) < 0) {
						printf("[%s] Read KernelA fail!", __FUNCTION__);
						break;
					}
					printf("[%s] Read KernelA len %d\n", __FUNCTION__, 0x600000);
					ubi_remove_vol("kernelB");
					ubi_create_vol("kernelB", 0x600000, 1, -1);
					ubi_volume_write("kernelB", (void *)s->buf, 0x600000);
					/* RootFS */
					if (ubi_volume_read("rootfsA", s->buf, 0x4800000) < 0) {
						printf("[%s] Read rootfsA fail!", __FUNCTION__);
						break;
					}
					printf("[%s] Read rootfsA len %d\n", __FUNCTION__, 0x4800000);
					ubi_remove_vol("rootfsB");
					ubi_create_vol("rootfsB", 0x4800000, 1, -1);
					ubi_volume_write("rootfsB", (void *)s->buf, 0x4800000);
					setenv("failcount_B", "0");
					setenv("update_chk", "2");
					saveenv();
				} while (0);
			} else if (strstr(s->buf, "opType=3")) {
				printf("[%s] clone BankB to BankA\n", __FUNCTION__);
				run_command("run ubi_init", 0);
				do {
					run_command("run switchbankA", 0);
					/* Kernel */
					if (ubi_volume_read("kernelB", s->buf, 0x600000) < 0) {
						printf("[%s] Read KernelB fail!", __FUNCTION__);
						break;
					}
					printf("[%s] Read KernelB len %d\n", __FUNCTION__, 0x600000);
					ubi_remove_vol("kernelA");
					ubi_create_vol("kernelA", 0x600000, 1, -1);
					ubi_volume_write("kernelA", (void *)s->buf, 0x600000);
					/* RootFS */
					if (ubi_volume_read("rootfsB", s->buf, 0x4800000) < 0) {
						printf("[%s] Read rootfsB fail!", __FUNCTION__);
						break;
					}
					printf("[%s] Read rootfsB len %d\n", __FUNCTION__, 0x4800000);
					ubi_remove_vol("rootfsA");
					ubi_create_vol("rootfsA", 0x4800000, 1, -1);
					ubi_volume_write("rootfsA", (void *)s->buf, 0x4800000);
					setenv("failcount_A", "0");
					setenv("update_chk", "0");
					saveenv();
				} while (0);
			} else if (strstr(s->buf, "opType=4")) {
				if (get_serial_enable()) {
					char *ptr = 0;
					char mptest = 0;
					char mptest_buf[4] = "0";
					int i;
					
					ptr = strstr(s->buf, "mptest=");
					ptr += strlen("mptest=");
					/* mptest value only allow 0, 1, 2 */
					if (ptr[0] == '0' || ptr[0] == '1' || ptr[0] == '2')
						mptest_buf[0] = ptr[0];
					if (simple_strtoul(mptest_buf, NULL, 10) == 0) {
						/* Terry 20141226, TODO, block 5 first page */
						/* Terry 20150903, include bad block tolerance */
#if 1
						remove_serial_key();
#else
						run_command("nand erase 0x80000 0x90", 0);
#endif
					}
					setenv("mptest", mptest_buf);
					saveenv();
				}
			}
		} else {
		    if (post_boundary_found){
				int target = 5;
				/*
				 * 0 => Bootloader
				 * 1 => Kernel
				 * 2 => RootFS
				 * 4 => Fullimage (UBI)
				 * 5 => Firmware
				 * 6 => All Firmwares(both bankA and bankB)
				 */
				/* Skip first boundary and get target */
				do {
					if (strstr(&s->buf[s->count], "name=\"target\"") != NULL) {
						if (move_to_dual_CRLF(s)) {
							s->count += 4;
						}
						target = s->buf[s->count] - '0';
						s->count += 1;
					}
					s->count += 2;
				} while (0);
				
		    	/* Multi-part Upload */
				s->count=(s->count) + strlen(post_boundary) + 2 ;
				//printf("buf=%08x\n",(u32)(s->buf + s->count));
				if(move_to_dual_CRLF(s)){
					s->count=(s->count)+4;
				}
				post_file_p=s->buf + s->count;
				//printf("buf=%08x\n",(u32)(post_file_p));
				post_file_length=find_post_file_length(s);
				printf("file len=%d\n",post_file_length);
		
				httpd_fs_open(http_wait_html, &s->file);
			  	strcpy(s->filename, http_wait_html);
		  		send_headers(s, http_header_200);
				send_file(s);
				send_data(s, 0, 0);   
#if 1
				printf("[%s] target %d\n", __FUNCTION__, target);

				switch (target) {
					case 5: { // Firmware
						struct trx_header *trx;
						char *kernel_start = NULL;
						int kernel_len = 0;
						char *rootfs_start = NULL;
						int rootfs_len = 0;
						int active_bank = 0;
						
						/* Terry 20141216, init ubi */
						run_command("run ubi_init", 0);
						
						/* For Image Header Alignment Issue  */
						base_addr = load_addr;
						memmove((char*)base_addr, post_file_p, post_file_length);
						post_file_p = (char *)base_addr;
						
						trx = post_file_p;
						if (le32_to_cpu(trx->magic) != TRX_MAGIC) {
							printf("[%s] TRX header error (Wrong magic number).\n", __FUNCTION__);
							break;
						}
						/* Terry 20140528, TODO need to check CRC */
						kernel_start = post_file_p + le32_to_cpu(trx->offsets[0]);
						rootfs_start = post_file_p + le32_to_cpu(trx->offsets[1]);
						kernel_len = le32_to_cpu(trx->offsets[1]) - le32_to_cpu(trx->offsets[0]);
						rootfs_len = le32_to_cpu(trx->len) - sizeof(struct trx_header) - kernel_len;
						printf("[%s] kernel_start 0x%08x, kernel_len %d, rootfs_start 0x%08x, rootfs_len %d\n", __FUNCTION__,
								kernel_start, kernel_len, rootfs_start, rootfs_len);
						active_bank = simple_strtoul((char *)getenv("update_chk"), NULL, 10);
						printf("[%s] active_bank %d\n", __FUNCTION__, active_bank);
						
						/**** Format UBI partition linux (including WiFi/DECT calibration ****/
						/*
						run_command("nand erase 0x440000 0xf800000", 0);
						*/
						
						if (active_bank == 0 || active_bank == 3) {
							/* Current BankA */
							run_command("run switchbankB", 0);
							ubi_remove_vol("kernelB");
							ubi_create_vol("kernelB", 0x600000, 1, -1);
							if (upgrade_img(kernel_start, kernel_len, "kernel", 0, 0)) {
								printf("[B]Can not upgrade the image %s\n", "kernel");
								break;
							}
							ubi_remove_vol("rootfsB");
							ubi_create_vol("rootfsB", 0x4800000, 1, -1);
							if (upgrade_img(rootfs_start, rootfs_len, "rootfs", 0, 0)) {
								printf("[B]Can not upgrade the image %s\n", "rootfs");
								break;
							}
							setenv("failcount_B", "0");
							setenv("update_chk", "2");
						} else {
							/* active_bank == 1 || active_bank == 2 */
							/* Current BankB */
							run_command("run switchbankA", 0);
							ubi_remove_vol("kernelA");
							ubi_create_vol("kernelA", 0x600000, 1, -1);
							if (upgrade_img(kernel_start, kernel_len, "kernel", 0, 0)) {
								printf("[A]Can not upgrade the image %s\n", "kernel");
								break;
							}
							ubi_remove_vol("rootfsA");
							ubi_create_vol("rootfsA", 0x4800000, 1, -1);
							if (upgrade_img(rootfs_start, rootfs_len, "rootfs", 0, 0)) {
								printf("[A]Can not upgrade the image %s\n", "rootfs");
								break;
							}
							setenv("failcount_A", "0");
							setenv("update_chk", "0");
						}
						saveenv();
						printf("[%s] Upgrade Done.\n", __FUNCTION__);
						break;
					}
					case 6: { // Firmware (All banks)
						struct trx_header *trx;
						char *kernel_start = NULL;
						int kernel_len = 0;
						char *rootfs_start = NULL;
						int rootfs_len = 0;
						int active_bank = 0;
						
						/* Terry 20141216, init ubi */
						run_command("run ubi_init", 0);
						
						/* For Image Header Alignment Issue  */
						base_addr = load_addr;
						memmove((char*)base_addr, post_file_p, post_file_length);
						post_file_p = (char *)base_addr;
						
						trx = post_file_p;
						if (le32_to_cpu(trx->magic) != TRX_MAGIC) {
							printf("[%s] TRX header error (Wrong magic number).\n", __FUNCTION__);
							break;
						}
						/* Terry 20140528, TODO need to check CRC */
						kernel_start = post_file_p + le32_to_cpu(trx->offsets[0]);
						rootfs_start = post_file_p + le32_to_cpu(trx->offsets[1]);
						kernel_len = le32_to_cpu(trx->offsets[1]) - le32_to_cpu(trx->offsets[0]);
						rootfs_len = le32_to_cpu(trx->len) - sizeof(struct trx_header) - kernel_len;
						printf("[%s] kernel_start 0x%08x, kernel_len %d, rootfs_start 0x%08x, rootfs_len %d\n", __FUNCTION__,
								kernel_start, kernel_len, rootfs_start, rootfs_len);
						active_bank = simple_strtoul((char *)getenv("update_chk"), NULL, 10);
						printf("[%s] active_bank %d\n", __FUNCTION__, active_bank);
						
						/**** Format UBI partition linux (including WiFi/DECT calibration ****/
						/*
						run_command("nand erase 0x440000 0xf800000", 0);
						*/
						
						/* Current BankA */
						run_command("run switchbankB", 0);
						ubi_remove_vol("kernelB");
						ubi_create_vol("kernelB", 0x600000, 1, -1);
						if (upgrade_img(kernel_start, kernel_len, "kernel", 0, 0)) {
							printf("[B]Can not upgrade the image %s\n", "kernel");
							break;
						}
						ubi_remove_vol("rootfsB");
						ubi_create_vol("rootfsB", 0x4800000, 1, -1);
						if (upgrade_img(rootfs_start, rootfs_len, "rootfs", 0, 0)) {
							printf("[B]Can not upgrade the image %s\n", "rootfs");
							break;
						}
						setenv("failcount_B", "0");
						setenv("update_chk", "2");

						/* active_bank == 1 || active_bank == 2 */
						/* Current BankB */
						run_command("run switchbankA", 0);
						ubi_remove_vol("kernelA");
						ubi_create_vol("kernelA", 0x600000, 1, -1);
						if (upgrade_img(kernel_start, kernel_len, "kernel", 0, 0)) {
							printf("[A]Can not upgrade the image %s\n", "kernel");
							break;
						}
						ubi_remove_vol("rootfsA");
						ubi_create_vol("rootfsA", 0x4800000, 1, -1);
						if (upgrade_img(rootfs_start, rootfs_len, "rootfs", 0, 0)) {
							printf("[A]Can not upgrade the image %s\n", "rootfs");
							break;
						}
						setenv("failcount_A", "0");
						setenv("update_chk", "0");

						saveenv();
						printf("[%s] Upgrade Done.\n", __FUNCTION__);
						break;
					}
					default:
						printf("[%s] Unsupported target %d\n", __FUNCTION__, target);
				};
				printf("[%s] post_file_p 0x%08x, post_file_length %d\n", __FUNCTION__, post_file_p, post_file_length);
				printf("[%s] load_addr 0x%08x\n", __FUNCTION__, load_addr);
#else
				/* For Image Header Alignment Issue  */
				base_addr=board_obtain_loading_buffer_addr(post_file_length);
				memcpy((char*)base_addr,post_file_p,post_file_length);
				post_file_p = (char *)base_addr;	
				http_upgrade((u32)post_file_p, post_file_length);
#endif
		    } else {		    
				printf("POST error, not support for current project!\n");
				httpd_fs_open(http_404_html, &s->file);
				strcpy(s->filename, http_404_html);
		  		send_headers(s, http_header_200);
				send_file(s);
				send_data(s, 0, 0);   
		    }		
	  	}
	  	post_flag=0;
  	  	post_content_length=0;
      	post_content_count=0;
      	post_current_pos=0;
      	post_boundary_found=0;
      	post_file_length=0;
		post_found_1st_crlfcrlf_flag=0;
      	post_file_p=NULL;  
      	s->len=0;
      	s->count=0;
      	return;
	}
  
	if(!isLogined){
		// If not logined, response login.html to client still been authenticated @ Misora 20120730
		httpd_fs_open("/login.html", &s->file);
		strcpy(s->filename, "/login.html");
	} else if(!httpd_fs_open(s->filename, &s->file)){
		printf("open file error!\n");
		httpd_fs_open(http_404_html, &s->file);
		strcpy(s->filename, http_404_html);
	}

EXIT:  


	send_headers(s, http_header_200);
	
	printf("file name: %s\n", s->filename);	
	if (strstr(s->filename, "/savetop.html") != NULL ||
			strstr(s->filename, "/upgrade_firm_browse.html") != NULL ||
			strstr(s->filename, "/undoc_upgrade.html") != NULL) {
		/* 	If user request index.html, we have to insert fw_version, bl_version and hw_version into raw html code arry.
			Misora @ 20120529		*/
		int index, j;
		char *v;
#if 1
		if ((v = getenv("ver")) != NULL)
			sprintf(new_index_html_bf, s->file.data, v);
		else
			sprintf(new_index_html_bf, s->file.data, "N/A");
#else
		image_header_info_t info[2];
		/* 	Image index:
			0: u-boot (IMAGE_ID_UBOOT)
			1: kernel 1
			2: kernel 2	*/
		for(index=IMAGE_ID_UBOOT+1, j=0; index<=2; index++, j++){
			memset((char *) &info[j],0,sizeof(image_header_info_t));
			if(retrieve_system_image_info(index, &info[j])!=0)
				printf("No Content for the image header (%d)\n",index);
		}
		sprintf(new_index_html_bf, s->file.data, getenv("bl_version"), info[0].image_version, info[1].image_version, getenv_manuf("hw_ver"), getenv_manuf("serial_number"), getenv_manuf("mac_address_base"), getenv_manuf("pin_code"));
#endif
		s->file.data = new_index_html_bf;
		s->file.len = strlen(s->file.data);
	} else if (strstr(s->filename, "/undoc_factory.html") != NULL) {
		/* Terry 20141217, TODO, need to check security issue if we show the default WPA KEY on recovery page */
		char glbcfg_empty = 0;
		unsigned int *ui_ptr;
		int mpt_setting = 0;
		int bankA_fwt = 0;
		int bankB_fwt = 0;

		/* Check glbcfg */
		run_command("nand read 0x80100000 glbcfg", 0);

		ui_ptr = (unsigned int *)0x80100000;
		/* Terry 20141219, TODO, currently, we only detect first 0xfff bytes */
		while ((unsigned int)ui_ptr <= (0x80100000UL + 0xfff)) {
			if (*ui_ptr != 0xffffffff) {
				glbcfg_empty = 1;
				break;
			}
			ui_ptr++;
		}

		/* Set mptest */
		do {
			char *mptest_ptr = getenv("mptest");
			int mptest = simple_strtoul(mptest_ptr, NULL, 10);
			
			if (get_serial_enable() && mptest_ptr &&
					(mptest == 1 || mptest == 2)) {
				sprintf(s->buf, " \
				<tr><td> \
					<p align='center'><input type=input name=mptest value='%s' style='width:250'></p> \
					</td> \
					 <td><p align='left'><input type=button value='Set mptest' style='width:100' onclick='doConfirm(4)'></p></td> \
				</tr>", getenv("mptest"));
				mpt_setting = 1;
			}
		} while (0);

		/* Check FWT */
		do {
			char *fwt_file = "/sbin/arc_fwt";
			
			run_command("run ubi_init", 0);
#if 1
			/* bankA */
			if (run_command("ubifsmount rootfsA", 0) == -1)
				bankA_fwt = 2;
			else if (ubifs_ls(fwt_file) == 0)
				bankA_fwt = 1;
			/* bankB */
			if (run_command("ubifsmount rootfsB", 0) == -1)
				bankB_fwt = 2;
			else if (ubifs_ls(fwt_file) == 0)
				bankB_fwt = 1;
#endif
		} while (0);
		
		/* Generate the page */
		sprintf(new_index_html_bf, s->file.data,
				getenv("ethaddr"), getenv("sn"), getenv("hw_version"), getenv("essid"),
				((get_serial_enable()) ? getenv("wlkey") : "Hidden"), 
				((glbcfg_empty) ? "Using" : "Empty"), 
				((bankA_fwt == 2) ? "Error" : ((bankA_fwt == 1) ? "FWT" : "FW")), 
				((bankB_fwt == 2) ? "Error" : ((bankB_fwt == 1) ? "FWT" : "FW")), 
			   ((mpt_setting == 1) ? s->buf : ""));
		s->file.data = new_index_html_bf;
		s->file.len = strlen(s->file.data);
		// printf("[%s] s->file.len %d\n", __FUNCTION__, s->file.len);
	}

	send_file(s);
	send_data(s, 0, 0);
  
    return;
}


static int find_boundary(struct tcp_appstate *s)
{
	  int flag=0;
	  char *needle="boundary=";
      char *p=NULL;
	  flag=find_value("Content-Type",strlen("Content-Type"), s);
	  /* found content_type only */
	  // Re-design by Jess Hsieh.
	  if(flag){
	  	/* Boundary would be inside of the Content-Type */
      	p=strstr(s->inputbuf,needle); 	  
	  	if(p){
	  		clear_buf(post_boundary, 100);
	  		strcpy(post_boundary, p+9);
	  		printf("boundary=%s\n",post_boundary);
			flag=1;
	  	}else{
	  		flag=0;
	  	}
	  }else{
	    /* Boundary not found, Reset Boundary Flag */
	  	flag=0;
	  }
	  return flag;
}


static void
handle_post(struct tcp_appstate *s) 
{
	int i;
	char tmp;
	char *p = NULL;
	int found_flag = 0;
	int found_cl_len = 0;
	
	/* Terry 20141216, find boundary in Content-Type */
	if (post_boundary_found == 0) {   	  
		post_boundary_found = find_boundary(s);        
	}

	if (post_content_length == 0) {
		if (g_tmpbuf_len == 0)
			s->count = 0;
		found_flag = find_value_content_length("Content-Length", strlen("Content-Length"), s, &post_content_length);
		if (found_flag == 0) {
			/*if not found, then wait for new packets*/
			s->state = STATE_WAITING;
		} else {
			printf("value found!\n"); 
			// printf("%02x\n",s->inputbuf[strlen("Content-Length")+2]);
			/* Terry 20141120, fix http not response problem when performing firmware upgrade */
			/* The value of i should be less then read bytes + (strlen("Content-Length")+2) */
#if 0
			for (i = 0; (i + strlen("Content-Length") + 2) < found_cl_len; i++) {
				tmp = s->inputbuf[strlen("Content-Length") + 2 + i];
				if (tmp == ISO_return) break;
				post_content_length = (post_content_length * 10) + (tmp - 0x30);     		
			}
#endif
			// 
			printf("post_content_length=%d\n",post_content_length);
			// printf("s->len=%d, s->count=%d\n",s->len,s->count);
			//joelin for firefox bugfix
			
			// Re-design by Jess Hsieh.
			// for Firefox Content_Length would be the last parsing item, so look ahead the next 2 bytes was 0d 0a or not.
			// If yes,  Content_Length has been added with the 1st 0d 0a, and next 0d 0a was found then end of the HTTP Header, and start with the HTTP Payload
			if (s->buf[s->count] == ISO_return && s->buf[s->count+1] == ISO_nl) {
				 s->count += 2;
				 // printf("%s[%d]after move s->len=%d, s->count=%d\n",__FUNCTION__,__LINE__,s->len,s->count);
				 post_content_count = (s->len) - (s->count);
				 post_found_1st_crlfcrlf_flag = 1;
			} else {
				if (move_to_dual_CRLF(s)) {;// s->count, move to   \r\n 
					// Found the 0d 0a 0d 0a; otherwise we should not add with the 4 number to skip the header.
					s->count=s->count+4;/*remove dual CRLF*/
					// printf("%s[%d]after move s->len=%d, s->count=%d\n",__FUNCTION__,__LINE__,s->len,s->count);
					post_content_count=(s->len)-(s->count);
					post_found_1st_crlfcrlf_flag=1;
				}
			}//joelin for firefox

#ifdef CONTENT_LENGTH_DEBUG
			printf("%s[%d]...post_content_count[%u] vs post_content_length[%u] \n",__FUNCTION__,__LINE__,post_content_count,post_content_length);
			printf("[%s] post_found_1st_crlfcrlf_flag %d\n", __FUNCTION__, post_found_1st_crlfcrlf_flag);
#endif

			if (post_found_1st_crlfcrlf_flag && post_content_count>=post_content_length) {
				// To fix google chrome issue. Jess Hsieh [post_found_1st_crlfcrlf_flag] must be one condition.
				s->state=STATE_OUTPUT;
			} else { 
				s->state = STATE_WAITING;
			}  
		}
	} else {
		// Found "Content_Length:", but not founf the end of header 0d 0a 0d 0a , end of HTTP Post Header
		// Re-design by Jess Hsieh.

		/* Modified for IE8. @ Misora 20120801
		    IE8 would send out http POST header (not include POST attributes) over than 500 bytes.
		    It is divided to two TCP frames, so we should reset count here.	*/	
		if (!post_boundary_found) 
			s->count = 0;
		
		if (!post_found_1st_crlfcrlf_flag) {
			if (s->buf[s->count] == ISO_return && s->buf[s->count+1]==ISO_nl) {
		  		s->count+=2;
		  		// printf("%s[%d]after move s->len=%d, s->count=%d\n",__FUNCTION__,__LINE__,s->len,s->count);
		  		post_content_count = (s->len) - (s->count);
				post_found_1st_crlfcrlf_flag = 1;
			} else {
		  		if (move_to_dual_CRLF(s)) {;// s->count, move to	\r\n 
			 		// Found the 0d 0a 0d 0a; otherwise we should not add with the 4 number to skip the header.
			 		s->count=s->count+4;/*remove dual CRLF*/
			 		// printf("%s[%d]after move s->len=%d, s->count=%d\n",__FUNCTION__,__LINE__,s->len,s->count);
			 		post_content_count=(s->len)-(s->count);
					post_found_1st_crlfcrlf_flag=1;
			 		// printf("%s[%d]...post_content_count[%u] vs post_content_length[%u] \n",__FUNCTION__,__LINE__,post_content_count,post_content_length);
		  		}
			}
		}

#ifdef CONTENT_LENGTH_DEBUG
		if ((post_content_length - post_content_count) < 50 * 1024) {
			printf("%s[%d]...post_content_count[%u] vs post_content_length[%u] \n",__FUNCTION__,__LINE__,post_content_count,post_content_length);
			printf("[%s] post_found_1st_crlfcrlf_flag %d\n", __FUNCTION__, post_found_1st_crlfcrlf_flag);
		}
#endif
		
		if (post_found_1st_crlfcrlf_flag && post_content_count >= post_content_length) {
			// To fix google chrome issue. Jess Hsieh [post_found_1st_crlfcrlf_flag] must be one condition.
      		s->state = STATE_OUTPUT;
      	} else { 
      	    s->state = STATE_WAITING;
      	}  
   }
   return; 
}

static void
handle_input(struct tcp_appstate *s)
{
  int i,len;
	
  // printf("handle input!\n");
  // print_buf(s->buf,s->len);
  if(post_flag) {
  	// printf("%s[%d]...\n",__FUNCTION__,__LINE__);
    handle_post(s);
	return;
  }
  clear_buf(s->inputbuf, TCP_MAX_INPUT_BUFFER_SIZE);	
  s->count=0;
  len=readto(s, ISO_space);
  // printf("got\n");
  // print_buf(s->inputbuf,len);
  if(strncmp(s->inputbuf, http_get, 4) == 0) {
    printf("http get!\n");
    len=readto(s, ISO_space);
    if(s->inputbuf[0] != ISO_slash) {
     goto exit;
    }
    if(s->inputbuf[1] == ISO_space) {
       strncpy(s->filename, http_index_html, sizeof(s->filename));
    } else {
       s->inputbuf[len + 1] = 0;
       strncpy(s->filename, &s->inputbuf[0], sizeof(s->filename));
       printf("file=%s\n",s->filename);
    }
    s->state = STATE_OUTPUT;    
  }else if(strncmp(s->inputbuf, http_post, 5) == 0){
  	printf("http post!\n");
  	post_flag=1;
	handle_post(s);
	return;
  }else{
    print_buf(s->buf,s->len); /* Debugging Purpose */
    printf("unsupported method!\n");
  }
  

exit:
  s->count=0;
  s->len=0;   	 
  return;
}



static void
handle_connection(struct tcp_appstate *s)
{
  
  handle_input(s);
  if(s->state == STATE_OUTPUT) {
    handle_output(s);
  }
  return;
}


static void
HttpdHandler (uchar * data, int len, unsigned unused1, unsigned unused2)
{
	
	struct tcp_appstate *s = (struct tcp_appstate *)&(tcp_conn->appstate);
	int i;

/*
	printf("[%s] printHex %d\n", __FUNCTION__, len);
	printHex(data, len);
*/

	// print_buf(data,len);
	// printf("%s[%d] len=%d\n",__FUNCTION__,__LINE__,len);  
	memcpy(s->buf + s->len, data, len); 
	// printf(" %s[%d] s->buf  %08X\n",__FUNCTION__,__LINE__, (unsigned long) s->buf);
	s->len = s->len + len;
  
	/* Terry 20140527, show download progress */
	do {
		static int c, d = 1;
		
		if (s->len <= 1024) {
			d = 1;
		}
		
		c += len;
		if (c >= 100 * 1024) {
			printf("#");
			c = 0;
			d++;
		}
		if (d == 80) {
			printf("\n");
			d = 1;
		}
	} while (0);
  // printf("%s[%d] s->len=%d, s->count=%d\n",__FUNCTION__,__LINE__,s->len,s->count);
  // printf("%s[%d] post_content_length=%d\n",__FUNCTION__,__LINE__,post_content_length);
  // printf("%s[%d] post_content_count=%d\n",__FUNCTION__,__LINE__,post_content_count);
  // printf("%s[%d] post_boundary_found=%d\n",__FUNCTION__,__LINE__,post_boundary_found);
  if(strncmp(s->buf, http_post, 5) == 0){ // post_content_length>0)
  	post_content_count +=len;
	//s->count = 0;
	// printf(" %s[%d] post_content_count  %d ; len %d \n",__FUNCTION__,__LINE__, post_content_count,len);
    handle_connection(s);
  }else if(( *(data+len-2) == ISO_return ) && (*(data+len-1) == ISO_nl)){  
    // printf(" %s[%d] \n",__FUNCTION__,__LINE__);    
    handle_connection(s);
  }else{
  	s->state = STATE_WAITING;
  }
	return;
	
}

void HttpSend (uchar * pkt, unsigned len)
{
	
	return;
}


void HttpdStart (void)
{
   printf("http server start...\n");
   /* Recovery mini-HTTPD Global Variables should be reset, Jess Hsieh */
   post_flag=0;
   post_content_length=0;
   post_content_count=0;
   post_current_pos=0;
   post_boundary_found=0;
   post_file_length=0;
   post_found_1st_crlfcrlf_flag=0;
   post_file_p=NULL;
   /* Terry 20140526, TODO, remove PIN authorization */
#if 1
   isLogined = 1;   
#else
   isLogined = 0;   
#endif
   tcp_init();
   TcpSetHandler(HttpdHandler);
   tcp_listen(80);
   
   /* Terry 20141216, Remove UBI init */
   /* Terry 20140528, Init UBI */
   /* run_command("run ubi_init", 0); */
}

/* Terry 20150903, procedure to erase serial key */
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

void remove_serial_key() {
	int min_block_id = 4; /* 0 based */
	int max_block_id = 7;
	int key_len = 148; // 144+4, 36*4=144
	int i = 0;
	char cmd_buf[64];
	u8 page_buf[CONFIG_NAND_PAGE_SIZE];

	/* nand erase 0x80000 0x90 */
	/* must check serial key before erase it */
	for (i = min_block_id; i <= max_block_id; i++) {
		nand_read_page((CONFIG_NAND_BLOCK_SIZE * i) / CONFIG_NAND_PAGE_SIZE, page_buf);
		if (verify_serial_key(page_buf, key_len) == 0) {
			sprintf(cmd_buf, "nand erase 0x%x 0x%x", CONFIG_NAND_BLOCK_SIZE * i, key_len);
			// printf("[%s] cmd_buf %s\n", __FUNCTION__, cmd_buf);
			run_command(cmd_buf, 0);
			return;
		}
	}
}
#endif


#endif
