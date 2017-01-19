/* 
 * This file contains some non ansi functions that are present in 
 * windows. They are mapped approximately equal functions 
 *
 * Compile command : gcc -c linux_anwi.c 
 */

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <search.h>
#include "linux_ansi.h"

static char peek=-1;
static char hex[16]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
static struct termios old;
static struct termios newtc;

void ansi_init()
{
	tcgetattr(0,&old);
	newtc=old;
	newtc.c_lflag&=~ICANON;
	newtc.c_lflag&=~ECHO;
	newtc.c_lflag &= ~ISIG;
	newtc.c_cc[VTIME] = 0;

	return;
}

int kbhit()
{
	char ch;
  	int nread;

	if (peek != -1) return 1;
	
    newtc.c_cc[VMIN]=0;
	tcsetattr(0, TCSANOW, &newtc);
  	nread = read(0,&ch,1);
	tcsetattr(0,TCSANOW, &old);
	
	if (nread == 1) {
	   peek = ch;
	   return 1;
	}

	return 0;
}

int getch()
{
 	char ch;
        int ret;

	if (peek != -1) {
		  ch = peek;
		  peek = -1;
		  return ch;
	}

    newtc.c_cc[VMIN]=1;
	tcsetattr(0, TCSANOW, &newtc);
	ret=read(0,&ch,1); 
        (void)ret;
	tcsetattr(0,TCSANOW, &old);
	
    return ch;
}

int strnicmp(const char *str1,const char *str2,size_t len) 
{
    return strncmp(str1,str2,len);
}

char *strlwr(char *str)
{
		return str;
}

int _stricmp(const char *str1,const char *str2)
{
	return strcmp(str1,str2);
}


char *_strdate(char *tmpbuf)
{      
	time_t curtime;
	time(&curtime);
	ctime_r(&curtime,tmpbuf); 

	return tmpbuf;
}

void *_lfind(const void *key, const void *base,size_t *nmemb,size_t size,
				int (*compar)(const void *,const void *))
{
	return lfind(key,base,nmemb,size,compar);
}

void itoa(int num,char *dest_str,int base) 
{
	int count; // number of characters in string       
	int i;
	int sign; // determine if the value is negative   
	char temp[50]; // temporary string array 

	count = 0;
	if (num < 0) {
		sign = 1;
		num = -num;
	}

	if ((base < 2) || (base > 16)) {
		return;
	}

   	/*
	 * NOTE: This process reverses the order of an integer, 
	 * ie:         
	 * value = -1234 equates to: char [4321-]
	 * Reorder the values using for {} loop below                    |
	 */
	
   	do { 
			temp[count] = hex[num % base]; // obtain modulu
			count++;
			num = (int)(num / base);
	}  while (num > 0);

	if (sign < 0) {
		temp[count] = '-';
        count++;                 
	}
	
	for (i=0;i<count;i++) { 
		dest_str[i]=temp[count-i-1];	
	}
	dest_str[count] = '\0';
																			    	return;
}

void _makepath(char *path,char *drive,char *dir,char *name,char *ext) 
{
		int i;
		
		if (!path) return;

		path[0]='\0';

		if (drive) {
			strcat(path,drive);
		}

		if (dir) {
			strcat(path,dir);
		}

		if (name) {
			strcat(path,"\\");
			strcat(path,name);
		}

		if (ext) {
			strcat(path,".");
			strcat(path,ext);
		}
		
		for (i=0;i<strlen(path);i++) {
				if (path[i] == '\\') {
						path[i]='/';
				}
		}
		return;
}
		
