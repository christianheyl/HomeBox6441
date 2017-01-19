/******************************************************************************
**
** FILE NAME    : ifxmips_deu_vr9.h
** PROJECT      : IFX UEIP
** MODULES      : DEU Module for VR9
**
** DATE         : September 8, 2009
** AUTHOR       : Mohammad Firdaus
** DESCRIPTION  : Data Encryption Unit Driver
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
** 08,Sept 2009 Mohammad Firdaus    Initial UEIP release
*******************************************************************************/
/*!
  \defgroup IFX_DEU IFX_DEU_DRIVERS
  \ingroup API
  \brief deu driver module
*/

/*!
  \file	ifxmips_deu_vr9.h
  \ingroup IFX_DEU
  \brief board specific deu driver header file for vr9
*/

/*!
  \defgroup IFX_DEU_DEFINITIONS IFX_DEU_DEFINITIONS
  \brief deu driver header file
*/


#ifndef IFXMIPS_DEU_VR9_H
#define IFXMIPS_DEU_VR9_H

/* Project Header Files */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/crypto.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <asm/byteorder.h>
#include <crypto/algapi.h>
#include <asm/ifx/ifx_regs.h>	
#include <asm/ifx/ifx_types.h>
#include <asm/ifx/common_routines.h>
#include <asm/ifx/irq.h>
#include <asm/ifx/ifx_pmu.h>
#include <asm/ifx/ifx_gpio.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <asm/scatterlist.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include "ifxmips_deu.h"

#ifdef CONFIG_CRYPTO_DEV_DMA
#include "ifxmips_deu_dma.h"
#include <asm/ifx/irq.h>
#include <asm/ifx/ifx_dma_core.h>
extern _ifx_deu_device ifx_deu[1];
#define DEU_DWORD_REORDERING(ptr, buffer, in_out, bytes)      memory_alignment(ptr, buffer, in_out, bytes)
#define AES_MEMORY_COPY(outcopy, out_dma, out_arg, nbytes)    aes_dma_memory_copy(outcopy, out_dma, out_arg, nbytes)
#define DES_MEMORY_COPY(outcopy, out_dma, out_arg, nbytes)    des_dma_memory_copy(outcopy, out_dma, out_arg, nbytes)
#define BUFFER_IN    1
#define BUFFER_OUT   0
#define AES_ALGO     1
#define DES_ALGO     0
#define ALLOCATE_MEMORY(val, type)    1
#define FREE_MEMORY(buff)	
#endif /* CONFIG_CRYPTO_DEV_DMA */

/* proc stuffs */
#define MAX_DEU_ALGO 7
struct deu_proc {
    char name[10];
    int deu_status;
    void (*toggle_algo)(int mode);
    int (*get_deu_algo_state)(void);
};

void ifx_aes_toggle_algo (int mode);
void ifx_des_toggle_algo (int mode);
void ifx_md5_toggle_algo (int mode);
void ifx_sha1_toggle_algo (int mode);
void ifx_md5_hmac_toggle_algo (int mode);
void ifx_sha1_hmac_toggle_algo (int mode);
void ifx_arc4_toggle_algo (int mode);

int read_aes_algo_status(void);
int read_des_algo_status(void);
int read_arc4_algo_status(void);
int read_md5_algo_status(void);
int read_sha1_algo_status(void);
int read_md5_hmac_algo_status(void);
int read_sha1_hmac_algo_status(void);

void md5_init_registers(void);

#define AES_START IFX_AES_CON
#define DES_3DES_START  IFX_DES_CON

#if 0
#define AES_IDLE 0
#define AES_BUSY 1
#define AES_STARTED 2
#define AES_COMPLETED 3
#define DES_IDLE 0
#define DES_BUSY 1
#define DES_STARTED 2
#define DES_COMPLETED 3
#endif

/* SHA1 CONSTANT */
#define HASH_CON_VALUE    0x0701002C

#define INPUT_ENDIAN_SWAP(input)    input_swap(input)
#define DEU_ENDIAN_SWAP(input)    endian_swap(input)
#define FIND_DEU_CHIP_VERSION    chip_version() 

#if defined (CONFIG_AR10)
#define DELAY_PERIOD    10
#else
#define DELAY_PERIOD    10
#endif
				      
#define WAIT_AES_DMA_READY()          \
    do { 			      \
        int i;			      \
        volatile struct deu_dma_t *dma = (struct deu_dma_t *) IFX_DEU_DMA_CON; \
        volatile struct aes_t *aes = (volatile struct aes_t *) AES_START; \
        for (i = 0; i < 10; i++)      \
            udelay(DELAY_PERIOD);     \
        while (dma->controlr.BSY) {}; \
        while (aes->controlr.BUS) {}; \
    } while (0)

#define WAIT_DES_DMA_READY()          \
    do { 			      \
        int i;			      \
        volatile struct deu_dma_t *dma = (struct deu_dma_t *) IFX_DEU_DMA_CON; \
        volatile struct des_t *des = (struct des_t *) DES_3DES_START; \
        for (i = 0; i < 10; i++)      \
            udelay(DELAY_PERIOD);     \
        while (dma->controlr.BSY) {}; \
        while (des->controlr.BUS) {}; \
    } while (0)

#define AES_DMA_MISC_CONFIG()        \
    do { \
        volatile struct aes_t *aes = (volatile struct aes_t *) AES_START; \
        aes->controlr.KRE = 1;        \
        aes->controlr.GO = 1;         \
    } while(0)

#define SHA_HASH_INIT                \
    do {                               \
        volatile struct deu_hash_t *hash = (struct deu_hash_t *) HASH_START; \
        hash->controlr.ENDI = 1;  \
        hash->controlr.SM = 1;    \
        hash->controlr.ALGO = 0;  \
        hash->controlr.INIT = 1;  \
    } while(0)

#define MD5_HASH_INIT                \
    do {                               \
        volatile struct deu_hash_t *hash = (struct deu_hash_t *) HASH_START; \
        hash->controlr.ENDI = 1;  \
        hash->controlr.SM = 1;    \
        hash->controlr.ALGO = 1;  \
        hash->controlr.INIT = 1;  \
    } while(0)

/* DEU Common Structures for AR9*/
 
struct clc_controlr_t {
	u32 Res:26;
	u32 FSOE:1;
	u32 SBWE:1;
	u32 EDIS:1;
	u32 SPEN:1;
	u32 DISS:1;
	u32 DISR:1;

};

struct des_t {
	struct des_controlr {	
		u32 KRE:1;
		u32 reserved1:5;
		u32 GO:1;
		u32 STP:1;
		u32 Res2:6;
                u32 NDC:1;
                u32 ENDI:1;
                u32 Res3:2;
		u32 F:3;
		u32 O:3;
		u32 BUS:1;
		u32 DAU:1;
		u32 ARS:1;
		u32 SM:1;
		u32 E_D:1;
		u32 M:3;

	} controlr;
	u32 IHR;	
	u32 ILR;	
	u32 K1HR;	
	u32 K1LR;
	u32 K2HR;
	u32 K2LR;
	u32 K3HR;
	u32 K3LR;	
	u32 IVHR;	
	u32 IVLR;
	u32 OHR;
	u32 OLR;
};

struct aes_t {
	struct aes_controlr {

		u32 KRE:1;
		u32 reserved1:4;
		u32 PNK:1;
		u32 GO:1;
		u32 STP:1;
		u32 reserved2:6;
		u32 NDC:1;
		u32 ENDI:1;
		u32 reserved3:2;
		u32 F:3;
		u32 O:3;
		u32 BUS:1;	
		u32 DAU:1;
		u32 ARS:1;
		u32 SM:1;
		u32 E_D:1;
		u32 KV:1;
		u32 K:2;
	} controlr;
	u32 ID3R;	
	u32 ID2R;	
	u32 ID1R;
	u32 ID0R;
	u32 K7R;
	u32 K6R;
	u32 K5R;
	u32 K4R;
	u32 K3R;
	u32 K2R;
	u32 K1R;
	u32 K0R;
	u32 IV3R;
	u32 IV2R;
	u32 IV1R;
	u32 IV0R;
	u32 OD3R;
	u32 OD2R;
	u32 OD1R;	
	u32 OD0R;
};

struct arc4_t {
	struct arc4_controlr {

		u32 KRE:1;
		u32 KLEN:4;
		u32 KSAE:1;
		u32 GO:1;
		u32 STP:1;
		u32 reserved1:6;
		u32 NDC:1;
		u32 ENDI:1;
		u32 reserved2:8;
		u32 BUS:1;
		u32 reserved3:1;
		u32 ARS:1;
		u32 SM:1;
		u32 reserved4:4;						

	} controlr;
	u32 K3R;	
	u32 K2R;	
	u32 K1R;	
	u32 K0R;
    u32 IDLEN;	
	u32 ID3R;		
	u32 ID2R;		
	u32 ID1R;
	u32 ID0R;	
	
	u32 OD3R;
	u32 OD2R;	
	u32 OD1R;	
	u32 OD0R;
};

struct deu_hash_t {
	struct hash_controlr {
		u32 reserved1:5;
		u32 KHS:1;		
		u32 GO:1;
		u32 INIT:1;
		u32 reserved2:6;
		u32 NDC:1;
		u32 ENDI:1;
		u32 reserved3:7;
		u32 DGRY:1;		
		u32 BSY:1;
		u32 reserved4:1;
		u32 IRCL:1;
		u32 SM:1;
		u32 KYUE:1;
		u32 HMEN:1;
		u32 SSEN:1;
		u32 ALGO:1;

	} controlr;
	u32 MR;	
	u32 D1R;
	u32 D2R;
	u32 D3R;	
	u32 D4R;
	u32 D5R;

	u32 dummy;

	u32 KIDX;	
	u32 KEY;
	u32 DBN;	
};


struct deu_dma_t {
	struct dma_controlr {
		u32 reserved1:22;
		u32 BS:2;
		u32 BSY:1;
		u32 reserved2:1;
		u32 ALGO:2;
		u32 RXCLS:2;
		u32 reserved3:1;
		u32 EN:1;
	} controlr;
};

#endif /* IFXMIPS_DEU_VR9_H */
