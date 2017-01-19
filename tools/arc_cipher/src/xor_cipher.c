/***************************************************************************
 *
 * <:copyright-arcadyan
 * Copyright 2013 Arcadyan Technology 
 * All Rights Reserved. 
 * 
 * Arcadyan Confidential; Need to Know only. Protected as an unpublished work.
 * 
 * The computer program listings, specifications and documentation herein are
 * the property of Arcadyan Technology and shall not be reproduced, copied,
 * disclosed, or used in whole or in part for any reason without the prior
 * express written permission of Arcadyan Technology
 * :>
 *
 * File Name  : xor_cipher.c
 *
 * Description: This program accordingly XOR data via size and buildtime mattering
 *				Do not to use array as each platform may varies from memset support 
 *				
 * Updates    : 02/08/2013  bchan.  Created.
 *               
 ***************************************************************************/

#include "mkimg_head.h"

__inline__ void arc_xor(uint8_t *s1, uint8_t *s2, uint32_t len){
	uint32_t cnt = len;
	while(cnt--) *s1++ ^= *s2++;
}

void arc_img_enc(uint8_t *data, uint32_t total, uint32_t *rnd){

	uint8_t		*blks 		= NULL;
	uint8_t		*enc_blks 	= NULL;
	uint32_t 	key_blk 	= 0;
	uint32_t	end_blk 	= 0;
	uint32_t 	offset 		= 0;
	uint32_t	blk_size 	= 0;
	uint32_t	key_start 	= 0;
	uint32_t 	time_val 	= 0;
	uint32_t	cmn_divisor	= 0;
	
	if(rnd){
		time_val = endian_swap(*rnd);
		time_val %= 10;
		if(time_val <= 1)	time_val += 2;
	}

ENC_SRCH:
	for(end_blk = ROLL_MAX; end_blk >= ROLL_MIN; end_blk--){
		if(!(total%end_blk)){
			if((rnd) && (++cmn_divisor!=time_val))	continue;
			blk_size = total / end_blk;
			
			if(rnd){
				if(time_val&0x01)	key_blk = (((end_blk>>1)+(end_blk>>time_val))>end_blk)?end_blk>>1:((end_blk>>1)+(end_blk>>time_val));
				else				key_blk = ((end_blk>>time_val)>time_val)?(end_blk>>time_val):(end_blk>>2);
				if(end_blk&0x01)	key_blk -= (key_blk>>2)+(((key_blk>>4)>time_val)?((key_blk>>3)+time_val):(time_val>>1));
				else				key_blk += (key_blk>>1)-(((key_blk>>3)>time_val)?((key_blk>>2)-time_val):(time_val>>1));				
			}else
				key_blk = end_blk >> 1;
			key_start = blk_size * key_blk;
			
			printf(	"%s : %d blocks, each block size: %d, key @ %dth block 0x%08X "
					, __func__, end_blk , blk_size, key_blk, key_start
			);
			if(rnd)	printf("rnd_val: %d\n\r", time_val);
			else	printf("\r\n");
			break;
		}
	}
	
	if(!key_blk || !blk_size ){
		time_val = cmn_divisor;
		cmn_divisor = 0;
		goto ENC_SRCH;
	}

	if(!(enc_blks = (uint8_t *)malloc(blk_size)) || !(blks = (uint8_t *)malloc(blk_size))){
		printf("Malloc error\n\r");
		return;
	}else{
		bzero(enc_blks, blk_size);	bzero(blks, blk_size);	
	}

	memcpy(enc_blks, data+key_start ,blk_size);		
	memcpy(blks, data+offset, blk_size);	
	arc_xor(blks, enc_blks, blk_size);
		
	memcpy(data+offset, blks, blk_size);	
	
	while(end_blk){
		offset = blk_size*(--end_blk);
		if(offset == key_start || offset == 0)
			continue;
		
		bzero(enc_blks, blk_size);
		memcpy(enc_blks, blks, blk_size);	
		bzero(blks, blk_size);
		
		memcpy(blks,data+offset,blk_size);	
		
		arc_xor(blks, enc_blks, blk_size);
		memcpy(data+offset, blks, blk_size);
	};

	arc_xor(data+key_start, blks, blk_size);

	free(enc_blks);
	free(blks);
	
}

void arc_img_dec(uint8_t *dec_data, uint32_t total, uint32_t *rnd){

	uint32_t 	key_blk 	= 0;
	uint32_t 	nxt			= 0;
	uint32_t	end_blk 	= 0;
	uint32_t	dec_odr		= 0;
	uint32_t 	offset		= 0;
	uint32_t	dec_offset	= 0;
	uint32_t	blk_size 	= 0;
	uint32_t	key_start 	= 0;
	uint32_t	end_start	= 0;
	uint32_t 	time_val 	= 0;
	uint32_t	cmn_divisor	= 0;

	if(rnd){
		time_val = endian_swap(*rnd);
		time_val %= 10;
		if(time_val <= 1)	time_val += 2;
	}

DEC_SRCH:
	for(end_blk = ROLL_MAX; end_blk >= ROLL_MIN; end_blk--){
		if(!(total%end_blk)){
			if((rnd) && (++cmn_divisor!=time_val))	continue;
			blk_size = total / end_blk;
			
			if(rnd){
				if(time_val&0x01)	key_blk = (((end_blk>>1)+(end_blk>>time_val))>end_blk)?end_blk>>1:((end_blk>>1)+(end_blk>>time_val));
				else				key_blk = ((end_blk>>time_val)>time_val)?(end_blk>>time_val):(end_blk>>2);
				if(end_blk&0x01)	key_blk -= (key_blk>>2)+(((key_blk>>4)>time_val)?((key_blk>>3)+time_val):(time_val>>1));
				else				key_blk += (key_blk>>1)-(((key_blk>>3)>time_val)?((key_blk>>2)-time_val):(time_val>>1));				
			}else
				key_blk = end_blk >> 1;
			key_start = blk_size * key_blk;
			
			end_start = blk_size * (end_blk-1);
			printf(	"%s : %d blocks, each block size: %d, key @ %dth block 0x%08X "
					, __func__, end_blk , blk_size, key_blk, key_start
			);
			if(rnd)	printf("rnd_val: %d\n\r", time_val);
			else	printf("\r\n");
			break;
		}
	}
	
	if(!key_blk || !blk_size ){
		time_val = cmn_divisor;
		cmn_divisor = 0;
		goto DEC_SRCH;
	}

	offset = key_start;
	dec_offset = blk_size;
	arc_xor(dec_data+offset, dec_data+dec_offset, blk_size);

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
		arc_xor(dec_data+offset, dec_data+dec_offset, blk_size);
	}
	arc_xor(dec_data, dec_data+key_start, blk_size);

}
