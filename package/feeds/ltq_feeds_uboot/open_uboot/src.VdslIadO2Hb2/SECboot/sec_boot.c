/*****************************************************************************
;
;   (C) Unpublished Work of Arcadyan Incorporated.  All Rights Reserved.
;
;       THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL,
;       PROPRIETARY AND TRADESECRET INFORMATION OF ADMTEK INCORPORATED.
;       ACCESS TO THIS WORK IS RESTRICTED TO (I) ADMTEK EMPLOYEES WHO HAVE A
;       NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR ASSIGNMENTS
;       AND (II) ENTITIES OTHER THAN ADMTEK WHO HAVE ENTERED INTO APPROPRIATE
;       LICENSE AGREEMENTS.  NO PART OF THIS WORK MAY BE USED, PRACTICED,
;       PERFORMED, COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED,
;       ABBRIDGED, CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
;       TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT OF ADMTEK.
;       ANY USE OR EXPLOITATION OF THIS WORK WITHOUT AUTHORIZATION COULD
;       SUBJECT THE PERPETRATOR TO CRIMINAL AND CIVIL LIABILITY.
;	
;*****************************************************************************/
#include <common.h>
#include <exports.h>
#include "sec_boot.h"

#define TMP_KERNEL_ADDR		0xA4000000
#define REAL_KERNEL_ADDR	0xA0800000

int sec_boot (int argc, char *argv[])
{
	uint32_t p0, p1, p2, p3;
	uint8_t *ptr;
	uint32_t size;
		
	printf("<%30s:%5d>: Decrypt kernel at %x\n", __func__, __LINE__, TMP_KERNEL_ADDR);

	/* Terry 20141222, bypass un-encrypted kernel */
	/* TODO: Kernel maximum size is 6MB */
#if 1
	if (*((unsigned int *)TMP_KERNEL_ADDR) == IH_MAGIC) {
		printf("<%30s:%5d>: Unencrypted kernel detected %x\n", __func__, __LINE__, IH_MAGIC);
		my_memcpy(REAL_KERNEL_ADDR, TMP_KERNEL_ADDR, 0x600000);
		return 0;
	}
#endif
	
	ptr=TMP_KERNEL_ADDR;
	p0=ptr[0];
	p1=ptr[1];
	p2=ptr[2];
	p3=ptr[3];
	size=p3<<24|p2<<16|p1<<8|p0;
	printf("<%30s:%5d>: kernel size = %x\n", __func__, __LINE__, size);

	arc_img_dec(TMP_KERNEL_ADDR+(sizeof(uint32_t)), size, NULL);
	my_memcpy(REAL_KERNEL_ADDR, TMP_KERNEL_ADDR+(sizeof(uint32_t)), size);
	return 0;
}

