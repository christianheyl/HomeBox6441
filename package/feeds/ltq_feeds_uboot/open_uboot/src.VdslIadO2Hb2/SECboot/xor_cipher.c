#include <common.h>
#include <exports.h>
#include "sec_boot.h"

__inline__ void arc_dec(u8 *s1, u8 *s2, u32 len){
	u32 cnt = len;
	while(cnt--) *s1++ ^= *s2++;
}


void *my_memset(void *s, int c, unsigned long n)
{
	unsigned long  i;
	char *ss = (char *)s;

	for (i = 0; i < n; i++) ss[i] = (int)c;
	return s;
}

void my_memcpy(void *dest, void *src, unsigned long n)
{
	unsigned long  i;
	long *Dest = (long *) dest;
	long *Src = (long *) src;

	for (i = 0; i < (n>>2); i++) {
		Dest[i] = Src[i];
	}
}

int arc_img_dec(u8 *dec_data, u32 total, u32 *rnd){

	u32	key_blk 	= 0;
	u32	nxt			= 0;
	u32	end_blk 	= 0;

	u32	dec_odr		= 0;
	u32 offset		= 0;
	u32	dec_offset	= 0;
	u32	blk_size 	= 0;
	u32	key_start 	= 0;
	u32	end_start	= 0;

	u32	idv			= 0;
	u32	cmn			= 0;
	
	printf("<%30s:%5d>: data = %x \n", __func__, __LINE__, dec_data);

	if(rnd){
		idv = *rnd;
		idv %= 10;
		if(idv <= 1)	idv += 2;
	}
	printf("<%30s:%5d>:\n", __func__, __LINE__);
	
blk_loop:	
	for(end_blk = ROLL_MAX; end_blk >= ROLL_MIN; end_blk--){
		if(!(total%end_blk)){
			if((rnd) && (++cmn!=idv))	continue;
			blk_size = total / end_blk;

			if(rnd){
				if(idv&0x01)		key_blk = (((end_blk>>1)+(end_blk>>idv))>end_blk)?end_blk>>1:((end_blk>>1)+(end_blk>>idv));
				else				key_blk = ((end_blk>>idv)>idv)?(end_blk>>idv):(end_blk>>2);
				if(end_blk&0x01)	key_blk -= (key_blk>>2)+(((key_blk>>4)>idv)?((key_blk>>3)+idv):(idv>>1));
				else				key_blk += (key_blk>>1)-(((key_blk>>3)>idv)?((key_blk>>2)-idv):(idv>>1));							
			}else
				key_blk = end_blk >> 1;
			key_start = blk_size * key_blk;

			end_start = blk_size * (end_blk-1);
			break;
		}
	}
	printf("<%30s:%5d>:\n", __func__, __LINE__);
	if(!key_blk || !blk_size ){
		printf("<%30s:%5d>:\n", __func__, __LINE__);
		idv = cmn;
		cmn = 0;
		goto blk_loop;
	}
	
	printf("<%30s:%5d>:\n", __func__, __LINE__);

	offset = key_start;
	dec_offset = blk_size;
	arc_dec(dec_data+offset, dec_data+dec_offset, blk_size);

	printf("<%30s:%5d>:\n", __func__, __LINE__);
	
	
	for(dec_odr=1; dec_odr < end_blk; dec_odr++){
		offset = dec_offset;
		nxt = dec_odr+1;
		dec_offset = blk_size*nxt;
		if(dec_offset == key_start){
			dec_offset += blk_size;
			dec_odr++;
		}else if(offset == end_start){
			dec_offset = 0;
		}
		arc_dec(dec_data+offset, dec_data+dec_offset, blk_size);
	}
	
	printf("<%30s:%5d>:\n", __func__, __LINE__);

	arc_dec(dec_data, dec_data+key_start, blk_size);

	return 0;
}
