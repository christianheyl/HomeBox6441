#include <common.h>
#include <exports.h>
#include "sec_boot.h"

__inline__ void arc_memcpy(void *destination, const void *source, u32 len){
	
	if(len<0)
		return;

	char *dst = destination;
	const char *src = source;

	while(--len){
		*dst++ = *src++;
	};
}

__inline__ void arc_read(u32 ram_addr, u8 *dst, u32 len){
	printf("<%30s:%5d>: dst= %x, ram_addr =%x, len =%d\n", __func__, __LINE__, dst, ram_addr, len );
	arc_memcpy((void *)dst ,(void *)ram_addr, len);
}

