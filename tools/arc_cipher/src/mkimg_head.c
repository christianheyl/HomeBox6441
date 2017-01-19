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
 * File Name  : mkimg_head.c
 *
 * Description: This program reads pre-built U-Boot common MKIMAGE header
 *				Via its sizing and builetime to give information regard of enc/dec
 *				
 * Updates    : 01/08/2013  bchan.  Created.
 *				16/08/2013	bchan.	Add SEC_BOOT merge into uImage.
 *				23/09/2013	bchan.	Add ARC_BOOT encode and merge into uImage.
 *          
 ***************************************************************************/

#include "mkimg_head.h"

#define BUF_SIZE    	(1<<8)
#define ARCBOOT_RAW		"_RAW"
#define DECODE_TEMPLATE	"_dec"
#define FAKEN_CONTENT	"ZabuzaNarutoSasukeSakuraKakashiGaaraObitoItachiMadaraNidaimeSanju"

void arc_img_enc(uint8_t *data, uint32_t total, uint32_t *rnd);
void arc_img_dec(uint8_t *dec_data, uint32_t total, uint32_t *rnd);

void usage(char *func_name)
{
    printf("%s usage :\n\r", func_name);
    printf("\t%s Input Output SEC_BOOT ARC_BOOT\n\r", func_name);
    exit(-1);
}

void help(char *func_name){
    printf("%s usage :\n\r", func_name);
    printf("\t%s Input Output\n\r", func_name);
    exit(-1);	
}


int MKIMG_HDR_MAIN (int argc, char **argv)
{
	uint32_t errno			= 0;
	uint8_t	*data 			= NULL;
	uint8_t *end_buf		= NULL;
	uint8_t	*secboot_buf	= NULL;
	uint8_t *arcboot_buf	= NULL;

    uint32_t outLen			= 0;
	uint32_t end_len		= 0;
	uint32_t arcbootRAW_len	= 0;
	
    char inputBinFile[BUF_SIZE];
	char outputFile[BUF_SIZE];
	char secbootFile[BUF_SIZE];
	char arcbootFile[BUF_SIZE];
	char arcbootRAWFile[BUF_SIZE];
	char OutputDecBin[BUF_SIZE];

	FILE *hInput 			= NULL;
	FILE *hOutput			= NULL;
	FILE *hSecInput			= NULL;
	FILE *hArcInput 		= NULL;
	FILE *dec_fp 			= NULL;

	struct stat StatBuf;
	image_header_t	*hdr 			= NULL;
	image_header_t	*fkn_hdr 		= NULL;
	image_header_t	*arcboot_hdr	= NULL;
    char *function = argv[0];

	printf("<%30s:%5d>: dec = %d, sec =  %d , arc = %d\n", __func__, __LINE__, 
		sizeof(image_header_t),
		SECBOOT_LEN,
		ARCBOOT_LEN);
    if(argc<5)
        usage(function);

	strcpy(inputBinFile, argv[1]);
    strcpy(outputFile, argv[2]);
	strcpy(secbootFile, argv[3]);
	strcpy(arcbootFile, argv[4]);

	memcpy(arcbootRAWFile, arcbootFile, BUF_SIZE);
	strcat(arcbootRAWFile, ARCBOOT_RAW);
	memcpy(OutputDecBin, inputBinFile, BUF_SIZE);
	strcat(OutputDecBin, DECODE_TEMPLATE);

	if(	(access(inputBinFile, F_OK))||(access(secbootFile, F_OK)!=0)		||
		(access(arcbootFile, F_OK)!=0)||(access(arcbootRAWFile, F_OK)!=0)	||
		!(hInput = fopen(inputBinFile, "rb"))||!(hSecInput = fopen(secbootFile, "rb"))	||
		!(hArcInput= fopen(arcbootFile, "rb"))||!(hOutput = fopen(outputFile, "w+"))	||
		!(dec_fp = fopen(OutputDecBin, "w+"))				
	){
		errno = 1;
		goto END;
	}

	hdr = (image_header_t *)malloc(MKIMG_LEN);
	if(fread(hdr, sizeof(char), MKIMG_LEN, hInput) != MKIMG_LEN){
		errno = 2;
		goto END;
	}
	
	if((image_print_contents(hdr))!=0){
		errno = 3;
		goto END;
	}

	if((fkn_hdr = (image_header_t *)malloc(MKIMG_LEN))){
		uint8_t 	fkn_enc[MKIMG_LEN];
		uint32_t	new_size;

		memcpy(fkn_enc, FAKEN_CONTENT, MKIMG_LEN);
		arc_img_enc(fkn_enc, MKIMG_LEN, NULL);
		memcpy(fkn_hdr, fkn_enc, MKIMG_LEN);
		
		new_size = uimage_to_cpu(hdr->ih_size) + SECBOOT_LEN + MKIMG_LEN + ARCBOOT_LEN;
		fkn_hdr->ih_size = cpu_to_uimage(new_size);
	}

	outLen = image_get_data_size(hdr);
    data = (uint8_t*)malloc(outLen);

	if((fseek(hInput, SKIP_LEN, SEEK_SET))==-1){
		errno = 4;
		goto END;
	}

	if(fread(data, sizeof(char), outLen, hInput) != outLen){
		errno = 5;
		goto END;
	}

	if(stat(inputBinFile, &StatBuf )){
		errno = 6;
		goto END;
	}
	end_len = StatBuf.st_size - SKIP_LEN - outLen;

	if(end_len>0){
		end_buf = (uint8_t*)malloc(end_len);
		if(end_buf){
			if(fread(end_buf, sizeof(char), end_len, hInput)!=end_len){
				errno = 7;
    			goto END;
			}
		}
	}

	secboot_buf = (uint8_t *)malloc(SECBOOT_LEN);
	if(secboot_buf){
		if((fread(secboot_buf, sizeof(char), SECBOOT_LEN, hSecInput))!= SECBOOT_LEN){
			errno = 8;
			goto END;
		}
	}

	memset(&StatBuf, 0, sizeof(StatBuf));
	if(stat(arcbootRAWFile, &StatBuf )){
		errno = 9;
		goto END;
	}
	arcbootRAW_len = StatBuf.st_size;
	if((arcboot_hdr = (image_header_t *)malloc(MKIMG_LEN))){
		memcpy(arcboot_hdr, hdr, MKIMG_LEN);
		arcboot_hdr->ih_size = cpu_to_uimage(arcbootRAW_len);
	}
	
	arcboot_buf = (uint8_t *)malloc(ARCBOOT_LEN);
	if(arcboot_buf){
		if((fread(arcboot_buf, sizeof(char), ARCBOOT_LEN, hArcInput))!= ARCBOOT_LEN){
			errno = 10;
			goto END;
		}
	}

	arc_img_enc(arcboot_buf, arcbootRAW_len, &arcboot_hdr->ih_time);
	arc_img_enc(data, outLen, &hdr->ih_time);
	arc_img_enc((uint8_t *)arcboot_hdr, MKIMG_LEN, NULL);	
	arc_img_enc((uint8_t *)hdr, MKIMG_LEN, NULL);
	if(	fwrite(fkn_hdr, 	sizeof(char), MKIMG_LEN, 	hOutput)!= MKIMG_LEN	||
		fwrite(secboot_buf,	sizeof(char), SECBOOT_LEN, 	hOutput)!= SECBOOT_LEN	||
		fwrite(arcboot_hdr,	sizeof(char), MKIMG_LEN, 	hOutput)!= MKIMG_LEN	||
		fwrite(arcboot_buf,	sizeof(char), ARCBOOT_LEN,	hOutput)!= ARCBOOT_LEN	||
		fwrite(hdr, 		sizeof(char), MKIMG_LEN, 	hOutput)!= MKIMG_LEN	||
		fwrite(data, 		sizeof(char), outLen, 		hOutput)!= outLen		||
		fwrite(end_buf, 	sizeof(char), end_len, 		hOutput)!= end_len
	){
		errno = 11;
		goto END;
	}
	
	arc_img_dec((uint8_t *)arcboot_hdr, MKIMG_LEN, NULL);
	arc_img_dec((uint8_t *)hdr, MKIMG_LEN, NULL);
	arc_img_dec(arcboot_buf, arcbootRAW_len, &arcboot_hdr->ih_time);
	arc_img_dec(data, outLen, &hdr->ih_time);
	
	if(	fwrite(fkn_hdr, 	sizeof(char), MKIMG_LEN, 	dec_fp)!= MKIMG_LEN		||
		fwrite(secboot_buf,	sizeof(char), SECBOOT_LEN,	dec_fp)!= SECBOOT_LEN	||
		fwrite(arcboot_hdr,	sizeof(char), MKIMG_LEN, 	dec_fp)!= MKIMG_LEN		||
		fwrite(arcboot_buf,	sizeof(char), ARCBOOT_LEN,	dec_fp)!= ARCBOOT_LEN	||		
		fwrite(hdr, 		sizeof(char), MKIMG_LEN, 	dec_fp)!= MKIMG_LEN		||
		fwrite(data, 		sizeof(char), outLen, 		dec_fp)!= outLen		||
		fwrite(end_buf, 	sizeof(char), end_len, 		dec_fp)!= end_len
	){
		errno = 12;
		goto END;
	}

END:
	if(errno)		printf("ERROR #%d\n\r", errno);
	if(hdr)			free(hdr);	
	if(fkn_hdr)		free(fkn_hdr);
	if(data)		free(data);
	if(end_buf)		free(end_buf);
	if(secboot_buf)	free(secboot_buf);
	if(arcboot_buf)	free(arcboot_buf);
	if(hInput)		fclose(hInput);
	if(hOutput)		fclose(hOutput);
	if(hSecInput)	fclose(hSecInput);
	if(hArcInput)	fclose(hArcInput);
	if(dec_fp)		fclose(dec_fp);

    return errno;
}
/**
*	@arc:
*	@argv:
*/
int MKIMG_MAIN (int argc, char **argv)
{
	uint32_t errno			= 0;
	uint32_t fSize			= 0;
	
	char *function = argv[0];		// application name
	char *pTarget = argv[1];			// input raw data
	char *pCypher = argv[2];			// output cypher file
	uint8_t *pImage = NULL;
	
	FILE *hInput = NULL;
	FILE *hOutput = NULL;

	
	if(argc<3){
        help(function);
	}
	
	if(	(access(pTarget, F_OK))	|| !(hInput = fopen(pTarget, "rb")) ||
		!(hOutput = fopen(pCypher, "w+")) ){
		goto END;
	}	
	
	// get file size
	fseek (hInput, 0, SEEK_END);
	fSize=ftell (hInput);
	//printf("<%30s:%5d>: file size %d\n", __func__, __LINE__, fSize); 
	// point to file head
	fseek (hInput, 0, SEEK_SET);
	
	// size(4) + image 
	printf("<%30s:%5d>: file size %d\n", __func__, __LINE__, fSize);
	/* Terry 20160713, add 4 bytes to fix buf[26] == 0x0b problem */
	pImage = malloc(sizeof(fSize) + fSize + 4);
	
	if(fread(pImage, sizeof(char), fSize, hInput) != fSize){
		printf("<%30s:%5d>: file size %d\n", __func__, __LINE__, fSize); 
		goto END;
	}
	
	// encode
	arc_img_enc(pImage, fSize, NULL);

	/* Terry 20160713, workaround to fix buf[26] == 0x0b problem */
	/* which could damage u-boot when upgrading via recovery page. */
	if (pImage[26] == 0x0b) {
#if 1
		/* Terry 20160714, to minize the risky, we drop the firmware if the problem detected */
		printf("[%s] buf[26] == 0x0b problem detected. Please re-build firmware again!\n", __FUNCTION__);
		goto END;
#else
		int cbytes;
		srand(time(NULL));
		do {
			/* This image would have problem for recovery page */
			cbytes = rand();
			printf("*** buf[26] == 0x0b problem detected. cbytes %d ***\n", cbytes);
			fseek(hInput, 0, SEEK_SET);
			/* Read from file */
			if (fread(pImage, sizeof(char), fSize, hInput) != fSize) {
				printf("<%30s:%5d>: file size %d\n", __func__, __LINE__, fSize); 
				goto END;
			}
			
			*((int *)(&pImage[fSize])) = cbytes;
			arc_img_enc(pImage, fSize + 4, NULL);
		} while (pImage[26] == 0x0b);
		fSize += 4;
#endif
	}
	
	// output
	printf("<%30s:%5d>: file size (0x)%x\n", __func__, __LINE__, fSize); 
	if(fwrite(&fSize, 	sizeof(uint32_t), 1, 	hOutput)!= 1){
		printf("<%30s:%5d>: write file size error\n", __func__, __LINE__); 
		goto END;
	}	
	if(fwrite(pImage, 	sizeof(char), fSize, 	hOutput)!= fSize){
		printf("<%30s:%5d>: file size %d\n", __func__, __LINE__, fSize); 
		goto END;
	}
	
END:	
	if(pImage)		free(pImage);
	if(hInput)		fclose(hInput);
	if(hOutput)		fclose(hOutput);
	
	return errno;
}
