/******************************************************************************
**
** FILE NAME    : ifxmips_arc4.c
** PROJECT      : IFX UEIP
** MODULES      : DEU Module
**
** DATE         : September 8, 2009
** AUTHOR       : Mohammad Firdaus
** DESCRIPTION  : Data Encryption Unit Driver for ARC4 Algorithm
** COPYRIGHT    :       Copyright (c) 2009
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author             $Comment
** 08 Sept 2009 Mohammad Firdaus    Initial UEIP release
*******************************************************************************/

/*!
  \defgroup IFX_DEU IFX_DEU_DRIVERS
  \ingroup API
  \brief ifx deu driver module
*/

/*!
  \file		ifxmips_arc4.c
  \ingroup 	IFX_DEU
  \brief 	ARC4 encryption DEU driver file
*/

/*! 
  \defgroup IFX_ARC4_FUNCTIONS IFX_ARC4_FUNCTIONS
  \ingroup IFX_DEU
  \brief IFX deu driver functions
*/

/* Project header */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/crypto.h>
#include <crypto/algapi.h>
#include <linux/interrupt.h>
#include <asm/byteorder.h>
#include <linux/delay.h>
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>
#include <asm/ifx/irq.h>
#include <asm/ifx/ifx_dma_core.h>
#include <asm/ifx/ifx_pmu.h>
#include <asm/ifx/ifx_gpio.h>

/* Board specific header files */
#ifdef CONFIG_CRYPTO_DEV_ARC4_AR9
#include "ifxmips_deu_ar9.h"
#endif
#ifdef CONFIG_CRYPTO_DEV_ARC4_VR9
#include "ifxmips_deu_vr9.h"
#endif
 
/* Preprocessor declerations */
#define ARC4_MIN_KEY_SIZE       1
//#define ARC4_MAX_KEY_SIZE     256
#define ARC4_MAX_KEY_SIZE       16
#define ARC4_BLOCK_SIZE         1
#define ARC4_START   IFX_ARC4_CON

#define ARC4_STATUS     1

static int arc4_algo_status;

#ifdef CRYPTO_DEBUG
extern char debug_level;
#define DPRINTF(level, format, args...) if (level < debug_level) printk(KERN_INFO "[%s %s %d]: " format, __FILE__, __func__, __LINE__, ##args);
#else
#define DPRINTF(level, format, args...)
#endif

/* 
 * \brief arc4 private structure
*/
struct arc4_ctx {
        int key_length;
        u8 buf[120];
};

static int disable_multiblock = 0;
void powerup_deu(int crypto);
void powerdown_deu(int crypto);
void chip_version(void);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
module_param(disable_multiblock, int, 0);
#else
MODULE_PARM_DESC(disable_multiblock, "Disable encryption of whole multiblock buffers");
#endif
#ifdef CONFIG_CRYPTO_DEV_DMA
static int disable_deudma = 0;
#else 
static int disable_deudma = 1;
#endif

void set_arc4_algo_status(unsigned int arc4_algo, int cmd)
{
    arc4_algo_status = cmd;
}

int read_arc4_algo_status(void)
{
    int status;
    unsigned long flag;
    CRTCL_SECT_START;
    status = arc4_algo_status; 
    CRTCL_SECT_END;
    return status;
}

/*! \fn static void _deu_arc4 (void *ctx_arg, u8 *out_arg, const u8 *in_arg, u8 *iv_arg, u32 nbytes, int encdec, int mode)
    \ingroup IFX_ARC4_FUNCTIONS
    \brief main interface to ARC4 hardware   
    \param ctx_arg crypto algo context  
    \param out_arg output bytestream  
    \param in_arg input bytestream   
    \param iv_arg initialization vector  
    \param nbytes length of bytestream  
    \param encdec 1 for encrypt; 0 for decrypt  
    \param mode operation mode such as ebc, cbc, ctr  
*/                                 
static void _deu_arc4 (void *ctx_arg, u8 *out_arg, const u8 *in_arg,
            u8 *iv_arg, u32 nbytes, int encdec, int mode)
{
	volatile struct arc4_t *arc4 = (struct arc4_t *) ARC4_START;
	
	int i = 0;
	unsigned long flag;
	
#if 1 	/* need to handle nbytes not multiple of 16 */
	volatile u32 tmp_array32[4];
	volatile u8 *tmp_ptr8;
	int remaining_bytes, j;
#endif

	CRTCL_SECT_START;

	arc4->IDLEN = nbytes;

#if 1
	while (i < nbytes) {
		arc4->ID3R = *((u32 *) in_arg + (i>>2) + 0);
		arc4->ID2R = *((u32 *) in_arg + (i>>2) + 1);    
		arc4->ID1R = *((u32 *) in_arg + (i>>2) + 2);
		arc4->ID0R = *((u32 *) in_arg + (i>>2) + 3);    
		
		arc4->controlr.GO = 1; 
		
		while (arc4->controlr.BUS) {
		}

#if 1
		// need to handle nbytes not multiple of 16 
		tmp_array32[0] = arc4->OD3R;
		tmp_array32[1] = arc4->OD2R;
		tmp_array32[2] = arc4->OD1R;
		tmp_array32[3] = arc4->OD0R;

		remaining_bytes = nbytes - i;
		if (remaining_bytes > 16)
			 remaining_bytes = 16;
		
		tmp_ptr8 = (u8 *)&tmp_array32[0];
		for (j = 0; j < remaining_bytes; j++)
			 *out_arg++ = *tmp_ptr8++;
#else                                
		*((u32 *) out_arg + (i>>2) + 0) = arc4->OD3R;
		*((u32 *) out_arg + (i>>2) + 1) = arc4->OD2R;
		*((u32 *) out_arg + (i>>2) + 2) = arc4->OD1R;
		*((u32 *) out_arg + (i>>2) + 3) = arc4->OD0R;
#endif

		i += 16;
	}
#else

#endif 

        CRTCL_SECT_END;
}

/*! \fn arc4_chip_init (void)
    \ingroup IFX_ARC4_FUNCTIONS
    \brief initialize arc4 hardware   
*/                                 
static void arc4_chip_init (void)
{
        /* do nothing */
}

/*! \fn static int arc4_set_key(struct crypto_tfm *tfm, const u8 *in_key, unsigned int key_len)
    \ingroup IFX_ARC4_FUNCTIONS
    \brief sets ARC4 key    
    \param tfm linux crypto algo transform  
    \param in_key input key  
    \param key_len key lengths less than or equal to 16 bytes supported  
*/    
static int arc4_set_key(struct crypto_tfm *tfm, const u8 *inkey,
                       unsigned int key_len)
{
	volatile struct arc4_t *arc4 = (struct arc4_t *) ARC4_START;
	unsigned long flag;
	u32 *in_key = (u32 *)inkey;
      
        CRTCL_SECT_START;

	set_arc4_algo_status(ARC4_STATUS, CRYPTO_STARTED);
	powerup_deu(ARC4_INIT);
			
	/* must program all bits at one go?!!! */
#ifndef CONFIG_CRYPTO_DEV_DMA
	/* NDC=1,ENDI=1,GO=0,KSAE=1,SM=0 */
	*IFX_ARC4_CON = ( (1<<31) | ((key_len - 1)<<27) | (1<<26) | (3<<16) );

	arc4->K3R = *((u32 *) in_key + 0);
	arc4->K2R = *((u32 *) in_key + 1);
	arc4->K1R = *((u32 *) in_key + 2);
	arc4->K0R = *((u32 *) in_key + 3);
#else 
	/* NDC=1,ENDI=1,GO=0,KSAE=1,SM=1 */
	*IFX_ARC4_CON = ( (1<<31) | ((key_len - 1)<<27) | (1<<26) | (3<<16) | (1<<4) );


#if 1
	arc4->K3R = *((u32 *) in_key + 0);
	arc4->K2R = *((u32 *) in_key + 1);
	arc4->K1R = *((u32 *) in_key + 2);
	arc4->K0R = *((u32 *) in_key + 3);
#else
	arc4->K3R = endian_swap(*((u32 *) in_key + 0));
	arc4->K2R = endian_swap(*((u32 *) in_key + 1));
	arc4->K1R = endian_swap(*((u32 *) in_key + 2));
	arc4->K0R = endian_swap(*((u32 *) in_key + 3));
#endif

#endif

#if 0 /* arc4 is a ugly state machine, KSAE can only be set once per session  */
        ctx->key_length = key_len;

        memcpy ((u8 *) (ctx->buf), in_key, key_len);
#endif
	CRTCL_SECT_END;
        return 0;
}

/*! \fn static void _deu_arc4_ecb(void *ctx, uint8_t *dst, const uint8_t *src, uint8_t *iv, size_t nbytes, int encdec, int inplace)
    \ingroup IFX_ARC4_FUNCTIONS
    \brief sets ARC4 hardware to ECB mode   
    \param ctx crypto algo context  
    \param dst output bytestream  
    \param src input bytestream  
    \param iv initialization vector   
    \param nbytes length of bytestream  
    \param encdec 1 for encrypt; 0 for decrypt  
    \param inplace not used  
*/                               
static void _deu_arc4_ecb(void *ctx, uint8_t *dst, const uint8_t *src,
                uint8_t *iv, size_t nbytes, int encdec, int inplace)
{
	_deu_arc4 (ctx, dst, src, NULL, nbytes, encdec, 0);
}

/*! \fn static void arc4_crypt(struct crypto_tfm *tfm, u8 *out, const u8 *in)
    \ingroup IFX_ARC4_FUNCTIONS
    \brief encrypt/decrypt ARC4_BLOCK_SIZE of data   
    \param tfm linux crypto algo transform  
    \param out output bytestream  
    \param in input bytestream  
*/     
static void arc4_crypt(struct crypto_tfm *tfm, u8 *out, const u8 *in)
{
	struct arc4_ctx *ctx = crypto_tfm_ctx(tfm);

	_deu_arc4 (ctx, out, in, NULL, ARC4_BLOCK_SIZE,
				CRYPTO_DIR_DECRYPT, 0);
}

/*
 * \brief ARC4 function mappings
*/
static struct crypto_alg ifxdeu_arc4_alg = {
        .cra_name               =       "arc4",
        .cra_driver_name        =       "ifxdeu-arc4",
        .cra_flags              =       CRYPTO_ALG_TYPE_CIPHER,
        .cra_blocksize          =       ARC4_BLOCK_SIZE,
        .cra_ctxsize            =       sizeof(struct arc4_ctx),
        .cra_module             =       THIS_MODULE,
        .cra_list               =       LIST_HEAD_INIT(ifxdeu_arc4_alg.cra_list),
        .cra_u                  =       {
                .cipher = {
                        .cia_min_keysize        =       ARC4_MIN_KEY_SIZE,
                        .cia_max_keysize        =       ARC4_MAX_KEY_SIZE,
                        .cia_setkey             =       arc4_set_key,
                        .cia_encrypt            =       arc4_crypt,
                        .cia_decrypt            =       arc4_crypt,
                }
        }
};

/*! \fn static int ecb_arc4_encrypt(struct blkcipher_desc *desc, struct scatterlist *dst, struct scatterlist *src, unsigned int nbytes)
    \ingroup IFX_ARC4_FUNCTIONS
    \brief ECB ARC4 encrypt using linux crypto blkcipher    
    \param desc blkcipher descriptor  
    \param dst output scatterlist  
    \param src input scatterlist  
    \param nbytes data size in bytes  
*/                                     
static int ecb_arc4_encrypt(struct blkcipher_desc *desc,
                           struct scatterlist *dst, struct scatterlist *src,
                           unsigned int nbytes)
{
	struct arc4_ctx *ctx = crypto_blkcipher_ctx(desc->tfm);
	struct blkcipher_walk walk;
	int err;
unsigned long flag;

	DPRINTF(1, "\n");
	blkcipher_walk_init(&walk, dst, src, nbytes);
	err = blkcipher_walk_virt(desc, &walk);

	while ((nbytes = walk.nbytes)) {
		_deu_arc4_ecb(ctx, walk.dst.virt.addr, walk.src.virt.addr, 
					   NULL, nbytes, CRYPTO_DIR_ENCRYPT, 0);
		nbytes &= ARC4_BLOCK_SIZE - 1;
		err = blkcipher_walk_done(desc, &walk, nbytes);
	}

	CRTCL_SECT_START;
	set_arc4_algo_status(ARC4_STATUS, CRYPTO_IDLE);
	powerdown_deu(ARC4_INIT);
	CRTCL_SECT_END;
	return err;
}

/*! \fn static int ecb_arc4_decrypt(struct blkcipher_desc *desc, struct scatterlist *dst, struct scatterlist *src, unsigned int nbytes)
    \ingroup IFX_ARC4_FUNCTIONS
    \brief ECB ARC4 decrypt using linux crypto blkcipher    
    \param desc blkcipher descriptor  
    \param dst output scatterlist  
    \param src input scatterlist  
    \param nbytes data size in bytes  
*/                                     
static int ecb_arc4_decrypt(struct blkcipher_desc *desc,
                           struct scatterlist *dst, struct scatterlist *src,
                           unsigned int nbytes)
{
	struct arc4_ctx *ctx = crypto_blkcipher_ctx(desc->tfm);
	struct blkcipher_walk walk;
	int err;
	unsigned long flag;

	DPRINTF(1, "\n");
	blkcipher_walk_init(&walk, dst, src, nbytes);
	err = blkcipher_walk_virt(desc, &walk);

	while ((nbytes = walk.nbytes)) {
		_deu_arc4_ecb(ctx, walk.dst.virt.addr, walk.src.virt.addr, 
					   NULL, nbytes, CRYPTO_DIR_DECRYPT, 0);
		nbytes &= ARC4_BLOCK_SIZE - 1;
		err = blkcipher_walk_done(desc, &walk, nbytes);
	}

	CRTCL_SECT_START;
	set_arc4_algo_status(ARC4_STATUS, CRYPTO_IDLE);
	powerdown_deu(ARC4_INIT);
	CRTCL_SECT_END;
	return err;
}

/*
 * \brief ARC4 function mappings
*/
static struct crypto_alg ifxdeu_ecb_arc4_alg = {
        .cra_name               =       "ecb(arc4)",
        .cra_driver_name        =       "ifxdeu-ecb(arc4)",
        .cra_flags              =       CRYPTO_ALG_TYPE_BLKCIPHER,
        .cra_blocksize          =       ARC4_BLOCK_SIZE,
        .cra_ctxsize            =       sizeof(struct arc4_ctx),
        .cra_type               =       &crypto_blkcipher_type,
        .cra_module             =       THIS_MODULE,
        .cra_list               =       LIST_HEAD_INIT(ifxdeu_ecb_arc4_alg.cra_list),
        .cra_u                  =       {
                .blkcipher = {
                        .min_keysize            =       ARC4_MIN_KEY_SIZE,
                        .max_keysize            =       ARC4_MAX_KEY_SIZE,
                        .setkey                 =       arc4_set_key,
                        .encrypt                =       ecb_arc4_encrypt,
                        .decrypt                =       ecb_arc4_decrypt,
                }
        }
};

void ifx_arc4_toggle_algo(int mode)
{
    int ret = 0;

    if (mode) {
		crypto_unregister_alg(&ifxdeu_arc4_alg);
		crypto_unregister_alg(&ifxdeu_ecb_arc4_alg);
		ifxdeu_arc4_alg.cra_flags = CRYPTO_ALG_TYPE_BLKCIPHER;
		ifxdeu_ecb_arc4_alg.cra_flags = CRYPTO_ALG_TYPE_BLKCIPHER;
    }
    else {
		ret = crypto_register_alg(&ifxdeu_arc4_alg);
		if (ret < 0)
			goto arc4_err;

		ret = crypto_register_alg(&ifxdeu_ecb_arc4_alg);
		if (ret < 0)
			goto ecb_arc4_err;
    }

    return;

arc4_err:
	crypto_unregister_alg(&ifxdeu_arc4_alg);
	printk(KERN_ERR "IFX arc4 initialization failed!\n");
	return;
ecb_arc4_err:
	crypto_unregister_alg(&ifxdeu_ecb_arc4_alg);
	printk (KERN_ERR "IFX ecb_arc4 initialization failed!\n");
	return;

}

/*! \fn int __init ifxdeu_init_arc4(void)
    \ingroup IFX_ARC4_FUNCTIONS
    \brief initialize arc4 driver    
*/                                 
int __init ifxdeu_init_arc4(void)
{
    int ret = -ENOSYS;

#define IFX_DEU_DRV_VERSION         "2.0.0"
printk(KERN_INFO "Infineon Technologies DEU driver version %s \n", IFX_DEU_DRV_VERSION);

	if ((ret = crypto_register_alg(&ifxdeu_arc4_alg)))
		goto arc4_err;

	if ((ret = crypto_register_alg(&ifxdeu_ecb_arc4_alg)))
		goto ecb_arc4_err;

	arc4_chip_init ();

	set_arc4_algo_status(ARC4_STATUS, CRYPTO_IDLE);

	printk (KERN_NOTICE "IFX DEU ARC4 initialized%s%s.\n", disable_multiblock ? "" : " (multiblock)", disable_deudma ? "" : " (DMA)");
	return ret;

arc4_err:
	crypto_unregister_alg(&ifxdeu_arc4_alg);
	printk(KERN_ERR "IFX arc4 initialization failed!\n");
	return ret;
ecb_arc4_err:
	crypto_unregister_alg(&ifxdeu_ecb_arc4_alg);
	printk (KERN_ERR "IFX ecb_arc4 initialization failed!\n");
	return ret;

}

/*! \fn void __exit ifxdeu_fini_arc4(void)
    \ingroup IFX_ARC4_FUNCTIONS
    \brief unregister arc4 driver   
*/                                 
void __exit ifxdeu_fini_arc4(void)
{
	crypto_unregister_alg (&ifxdeu_arc4_alg);
	crypto_unregister_alg (&ifxdeu_ecb_arc4_alg);

#if defined(CONFIG_CRYPTO_DEV_DMA)
	printk("DMA has deregistered successfully\n");
#endif
}


