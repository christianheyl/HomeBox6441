/******************************************************************************
**
** FILE NAME    : ifxmips_deu_dma.c
** PROJECT      : IFX UEIP
** MODULES      : DEU Module for Danube
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
** 08 Sept 2009 Mohammad Firdaus    Initial UEIP release
*******************************************************************************/
/*!
  \defgroup IFX_DEU IFX_DEU_DRIVERS
  \ingroup  IFX_API
  \brief ifx deu driver module
*/

/*!
  \file	ifxmips_deu_dma.c
  \ingroup IFX_DEU
  \brief DMA deu driver file 
*/

/*!
 \defgroup IFX_DMA_FUNCTIONS IFX_DMA_FUNCTIONS
 \ingroup IFX_DEU
 \brief deu-dma driver functions
*/

/* Project header files */ 

#ifdef CONFIG_CRYPTO_DEV_DMA

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/crypto.h>
#include <linux/spinlock.h>
#include <linux/smp_lock.h>
#include <crypto/algapi.h>
#include <crypto/scatterwalk.h>

#include <asm/io.h> 
#include <asm/ifx/ifx_regs.h>
#include <asm/ifx/common_routines.h>
#include <asm/ifx/irq.h>
#include <asm/ifx/ifx_dma_core.h>
#include "ifxmips_deu_dma.h"
#include "ifxmips_deu.h"

#if defined(CONFIG_DANUBE)
#include "ifxmips_deu_danube.h"
#elif defined(CONFIG_AR9)
#include "ifxmips_deu_ar9.h"
#elif defined(CONFIG_VR9) || defined(CONFIG_AR10)
#include "ifxmips_deu_vr9.h"
#else
#error "Platform unknown!"
#endif

extern _ifx_deu_device ifx_deu[1];
void aes_dma_memory_copy(u32 *outcopy, u32 *out_dma, u8 *out_arg, int nbytes);
extern deu_drv_priv_t deu_dma_priv;
#if defined(CONFIG_ASYNC_AES) 
extern aes_priv_t *aes_queue;
#endif
#if defined(CONFIG_ASYNC_DES)
extern des_priv_t *des_queue;
#endif

#ifdef CONFIG_DEBUG
static void hexdump(unsigned char *buf, unsigned int len) 
{
	print_hex_dump(KERN_CONT, "", DUMP_PREFIX_OFFSET,
							16, 1,
				   buf, len, false);
}

#endif

/*! \fn int deu_dma_intr_handler (struct dma_device_info *dma_dev, int status)
 *  \ingroup IFX_DMA_FUNCTIONS
 *  \brief callback function for deu dma interrupt   
 *  \param dma_dev dma device  
 *  \param status not used  
*/                                 
int deu_dma_intr_handler (struct dma_device_info *dma_dev, int status)
{
#if !defined(CONFIG_CRYPTO_DEV_POLL_DMA) || defined(CONFIG_ASYNC_DES) || \
     defined(CONFIG_ASYNC_AES)

    deu_drv_priv_t *deu_priv = (deu_drv_priv_t *)dma_dev->priv;
    u8 *buf;
    int len = 0;
	int nbytes;
	unsigned long flag;

    switch(status) {
        case RCV_INT:
		    /* unaligned crypto fix !! */
			if (deu_priv->deu_rx_len != deu_priv->rx_aligned_len) 
				nbytes = deu_priv->rx_aligned_len;
			else
				nbytes = deu_priv->deu_rx_len;

			printk(KERN_DEBUG "actual bytes xfer >> %d, memcpy bytes >> %d\n", nbytes, deu_priv->deu_rx_len);

			CRTCL_SECT_START;
            len = dma_device_read(dma_dev, (u8 **)&buf, NULL);
            if (len < nbytes) {
                printk(KERN_ERR "%s packet length %d is not equal to expect %d\n", 
                    __func__, len, deu_priv->deu_rx_len);
                return -EINVAL;
            }
			CRTCL_SECT_END;

#ifdef CONFIG_DEBUG 
            printk(" **** dumping buf of addr: %08x **** \n", buf);
            hexdump((char *)buf, nbytes);
#endif

            memcpy(deu_priv->deu_rx_buf, buf, deu_priv->deu_rx_len);
            deu_priv->deu_rx_buf = NULL;
            deu_priv->deu_rx_len = 0;
			deu_priv->rx_aligned_len = 0;

#if defined(CONFIG_ASYNC_DES) 
            if (deu_priv->event_src == DES_ASYNC_EVENT) {
				tasklet_schedule(&des_queue->des_task);
				return 0;
			}
#endif
#if defined(CONFIG_ASYNC_AES) 
            if (deu_priv->event_src == AES_ASYNC_EVENT) {
				tasklet_schedule(&aes_queue->aes_task);
				return 0;
			}
#endif
            break;
        case TX_BUF_FULL_INT:
             break;

        case TRANSMIT_CPT_INT:
            break;

        default:
            break;
    }
#endif

    return 0; 
}

extern u8 *g_dma_block;
extern u8 *g_dma_block2;

/*! \fn u8 *deu_dma_buffer_alloc (int len, int *byte_offset, void **opt)
 *  \ingroup IFX_DMA_FUNCTIONS
 *  \brief callback function for allocating buffers for dma receive descriptors   
 *  \param len not used  
 *  \param byte_offset dma byte offset  
 *  \param *opt not used  
 *
*/                                 
u8 *deu_dma_buffer_alloc (int len, int *byte_offset, void **opt)
{
    u8 *swap = NULL;
   
    /* dma-core needs at least 2 blocks of memory */
    swap = g_dma_block;
    g_dma_block = g_dma_block2;
    g_dma_block2 = swap;

    /*dma_cache_wback_inv((unsigned long) g_dma_block, (PAGE_SIZE >> 1));  */
    *byte_offset = 0;
    
    return g_dma_block;
}

/*! \fn int deu_dma_buffer_free (u8 * dataptr, void *opt)
 *  \ingroup IFX_DMA_FUNCTIONS
 *  \brief callback function for freeing dma transmit descriptors   
 *  \param dataptr data pointer to be freed  
 *  \param opt not used 
*/                                 
int deu_dma_buffer_free (u8 *dataptr, void *opt)
{
#if 0
    printk("Trying to free memory buffer\n");
    if (dataptr == NULL && opt == NULL)
        return 0;
    else if (opt == NULL) {
        kfree(dataptr);
        return 1;
    }
    else if (dataptr == NULL) {
       kfree(opt);
       return 1;
    }
    else {
       kfree(opt);
       kfree(dataptr);
    }
#endif
    return 0;    
}
#endif /* CONFIG_CRYPTO_DEV_DMA */
