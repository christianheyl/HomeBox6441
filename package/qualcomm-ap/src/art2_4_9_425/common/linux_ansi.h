#ifndef __LINUX_ANSI_H_
#define __LINUX_ANSI_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus 

#include <sys/types.h>

extern void ansi_init();
extern void itoa(int,char *,int);
extern int kbhit();
extern int getch();
extern int strnicmp(const char *str1,const char *str2,size_t len);	
extern char *strlwr(char *str);
extern int _stricmp(const char *str1,const char *str2);
extern char *_strdate(char *tmpbuf);
extern void *_lfind(const void *key, const void *base,size_t *nmemb,size_t size,
				                int (*compar)(const void *,const void *));
extern void _makepath(char *,char *,char *,char *,char *);



#ifdef __cplusplus
}
#endif // __cplusplus

#endif



