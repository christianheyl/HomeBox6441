/*****************************************************************************
 **   FILE NAME       : ifxusb_cif.c
 **   PROJECT         : IFX USB sub-system V3
 **   MODULES         : IFX USB sub-system Host and Device driver
 **   SRC VERSION     : 1.0
 **   SRC VERSION     : 3.2
 **   DATE            : 1/Jan/2011
 **   DESCRIPTION     : The Core Interface provides basic services for accessing and
 **                     managing the IFX USB hardware. These services are used by both the
 **                     Host Controller Driver and the Peripheral Controller Driver.
 **   FUNCTIONS       :
 **   COMPILER        : gcc
 **   REFERENCE       : Synopsys DWC-OTG Driver 2.7
 **   COPYRIGHT       :  Copyright (c) 2010
 **                      LANTIQ DEUTSCHLAND GMBH,
 **                      Am Campeon 3, 85579 Neubiberg, Germany
 **
 **    This program is free software; you can redistribute it and/or modify
 **    it under the terms of the GNU General Public License as published by
 **    the Free Software Foundation; either version 2 of the License, or
 **    (at your option) any later version.
 **
 **  Version Control Section  **
 **   $Author$
 **   $Date$
 **   $Revisions$
 **   $Log$       Revision history
 *****************************************************************************/

/*
 * This file contains code fragments from Synopsys HS OTG Linux Software Driver.
 * For this code the following notice is applicable:
 *
 * ==========================================================================
 *
 * Synopsys HS OTG Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================== */

/*!
 \file ifxusb_cif.c
 \ingroup IFXUSB_DRIVER_V3
 \brief This file contains the interface to the IFX USB Core.
*/

#include <linux/version.h>
#include "ifxusb_version.h"

#include <asm/byteorder.h>
#include <asm/unaligned.h>

#ifdef __DEBUG__
	#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#endif

#if defined(__UEIP__)
	#include <asm/ifx/ifx_pmu.h>
#endif


#include "ifxusb_plat.h"
#include "ifxusb_regs.h"
#include "ifxusb_cif.h"


#ifdef __IS_DEVICE__
	#include "ifxpcd.h"
#endif

#ifdef __IS_HOST__
	#include "ifxhcd.h"
#endif

#include <linux/mm.h>

#include <linux/gfp.h>

#if defined(__UEIP__)
	#include <asm/ifx/ifx_board.h>
#endif

#include <asm/ifx/ifx_gpio.h>
#if defined(__UEIP__)
	#include <asm/ifx/ifx_led.h>
#endif



#if defined(__UEIP__)
	#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__) || defined(__IS_AMAZON_SE__)
		#ifndef USB_CTRL_PMU_SETUP
			#define USB_CTRL_PMU_SETUP(__x) USB0_CTRL_PMU_SETUP(__x)
		#endif
		#ifndef USB_PHY_PMU_SETUP
			#define USB_PHY_PMU_SETUP(__x) USB0_PHY_PMU_SETUP(__x)
		#endif
	#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__) || defined(__IS_AMAZON_SE__)
#endif // defined(__UEIP__)

/*!
 \brief This function is called to allocate buffer of specified size.
        The allocated buffer is mapped into DMA accessable address.
 \param size Size in BYTE to be allocated
 \param clear 0: don't do clear after buffer allocated, other: do clear to zero
 \return 0/NULL: Fail; uncached pointer of allocated buffer
 */
#ifdef __IS_HOST__
void *ifxusb_alloc_buf_h(size_t size, int clear)
#else
void *ifxusb_alloc_buf_d(size_t size, int clear)
#endif
{
	uint32_t *cached,*uncached;
	uint32_t totalsize,page;

	if(!size)
		return 0;

	size=(size+3)&0xFFFFFFFC;
	totalsize=size + 12;
	page=get_order(totalsize);

	cached = (void *) __get_free_pages(( GFP_ATOMIC | GFP_DMA), page);

	if(!cached)
	{
		IFX_PRINT("%s Allocation Failed size:%d\n",__func__,size);
		return NULL;
	}

	uncached = (uint32_t *)(KSEG1ADDR(cached));
	if(clear)
		memset(uncached, 0, totalsize);

	*(uncached+0)=totalsize;
	*(uncached+1)=page;
	*(uncached+2)=(uint32_t)cached;
	return (void *)(uncached+3);
}


/*!
 \brief This function is called to free allocated buffer.
 \param vaddr the uncached pointer of the buffer
 */
#ifdef __IS_HOST__
void ifxusb_free_buf_h(void *vaddr)
#else
void ifxusb_free_buf_d(void *vaddr)
#endif
{
	uint32_t totalsize,page;
	uint32_t *cached,*uncached;

	if(vaddr != NULL)
	{
		uncached=vaddr;
		uncached-=3;
		totalsize=*(uncached+0);
		page=*(uncached+1);
		cached=(uint32_t *)(*(uncached+2));
		if(totalsize && page==get_order(totalsize) && cached==(uint32_t *)(KSEG0ADDR(uncached)))
		{
			free_pages((unsigned long)cached, page);
			return;
		}
		// the memory is not allocated by ifxusb_alloc_buf. Allowed but must be careful.
		return;
	}
}



/*!
   \brief This function is called to initialize the IFXUSB CSR data
 	 structures.  The register addresses in the device and host
 	 structures are initialized from the base address supplied by the
 	 caller.  The calling function must make the OS calls to get the
 	 base address of the IFXUSB controller registers.

   \param _core_if        Pointer of core_if structure
   \param _irq            irq number
   \param _reg_base_addr  Base address of IFXUSB core registers
   \param _fifo_base_addr Fifo base address
   \param _fifo_dbg_addr  Fifo debug address
   \return 	0: success;
 */
#ifdef __IS_HOST__
int ifxusb_core_if_init_h(ifxusb_core_if_t *_core_if,
#else
int ifxusb_core_if_init_d(ifxusb_core_if_t *_core_if,
#endif
                        int               _irq,
                        uint32_t          _reg_base_addr,
                        uint32_t          _fifo_base_addr,
                        uint32_t          _fifo_dbg_addr)
{
	int retval = 0;
	uint32_t *reg_base  =NULL;
    uint32_t *fifo_base =NULL;
    uint32_t *fifo_dbg  =NULL;

    int i;

	IFX_DEBUGPL(DBG_CILV, "%s(%p,%d,0x%08X,0x%08X,0x%08X)\n", __func__,
	                                             _core_if,
	                                             _irq,
	                                             _reg_base_addr,
	                                             _fifo_base_addr,
	                                             _fifo_dbg_addr);

	if( _core_if == NULL)
	{
		IFX_ERROR("%s() invalid _core_if\n", __func__);
		retval = -ENOMEM;
		goto fail;
	}

	//memset(_core_if, 0, sizeof(ifxusb_core_if_t));

	_core_if->irq=_irq;

	reg_base  =ioremap_nocache(_reg_base_addr , IFXUSB_IOMEM_SIZE  );
	fifo_base =ioremap_nocache(_fifo_base_addr, IFXUSB_FIFOMEM_SIZE);
	fifo_dbg  =ioremap_nocache(_fifo_dbg_addr , IFXUSB_FIFODBG_SIZE);
	if( reg_base == NULL || fifo_base == NULL || fifo_dbg == NULL)
	{
		IFX_ERROR("%s() usb ioremap() failed\n", __func__);
		retval = -ENOMEM;
		goto fail;
	}

	_core_if->core_global_regs = (ifxusb_core_global_regs_t *)reg_base;

	/*
	 * Attempt to ensure this device is really a IFXUSB Controller.
	 * Read and verify the SNPSID register contents. The value should be
	 * 0x45F42XXX
	 */
	{
		int32_t snpsid;
		snpsid = ifxusb_rreg(&_core_if->core_global_regs->gsnpsid);
		if ((snpsid & 0xFFFFF000) != 0x4F542000)
		{
			IFX_ERROR("%s() snpsid error(0x%08x) failed\n", __func__,snpsid);
			retval = -EINVAL;
			goto fail;
		}
		_core_if->snpsid=snpsid;
	}

	#ifdef __IS_HOST__
		_core_if->host_global_regs = (ifxusb_host_global_regs_t *)
		    ((uint32_t)reg_base + IFXUSB_HOST_GLOBAL_REG_OFFSET);
		_core_if->hprt0 = (uint32_t*)((uint32_t)reg_base + IFXUSB_HOST_PORT_REGS_OFFSET);

		for (i=0; i<MAX_EPS_CHANNELS; i++)
		{
			_core_if->hc_regs[i] = (ifxusb_hc_regs_t *)
			    ((uint32_t)reg_base + IFXUSB_HOST_CHAN_REGS_OFFSET +
			    (i * IFXUSB_CHAN_REGS_OFFSET));
			IFX_DEBUGPL(DBG_CILV, "hc_reg[%d]->hcchar=%p\n",
			    i, &_core_if->hc_regs[i]->hcchar);
		}
	#endif //__IS_HOST__

	#ifdef __IS_DEVICE__
		_core_if->dev_global_regs =
		    (ifxusb_device_global_regs_t *)((uint32_t)reg_base + IFXUSB_DEV_GLOBAL_REG_OFFSET);

		for (i=0; i<MAX_EPS_CHANNELS; i++)
		{
			_core_if->in_ep_regs[i] = (ifxusb_dev_in_ep_regs_t *)
			    ((uint32_t)reg_base + IFXUSB_DEV_IN_EP_REG_OFFSET +
			    (i * IFXUSB_EP_REG_OFFSET));
			_core_if->out_ep_regs[i] = (ifxusb_dev_out_ep_regs_t *)
			    ((uint32_t)reg_base + IFXUSB_DEV_OUT_EP_REG_OFFSET +
			    (i * IFXUSB_EP_REG_OFFSET));
			IFX_DEBUGPL(DBG_CILV, "in_ep_regs[%d]->diepctl=%p/%p %p/0x%08X/0x%08X\n",
			    i, &_core_if->in_ep_regs[i]->diepctl, _core_if->in_ep_regs[i],
			    reg_base,IFXUSB_DEV_IN_EP_REG_OFFSET,(i * IFXUSB_EP_REG_OFFSET)
			    );
			IFX_DEBUGPL(DBG_CILV, "out_ep_regs[%d]->doepctl=%p/%p %p/0x%08X/0x%08X\n",
			    i, &_core_if->out_ep_regs[i]->doepctl, _core_if->out_ep_regs[i],
			    reg_base,IFXUSB_DEV_OUT_EP_REG_OFFSET,(i * IFXUSB_EP_REG_OFFSET)
			    );
		}
	#endif //__IS_DEVICE__

	/* Setting the FIFO and other Address. */
	for (i=0; i<MAX_EPS_CHANNELS; i++)
	{
		_core_if->data_fifo[i] = fifo_base + (i * IFXUSB_DATA_FIFO_SIZE);
		IFX_DEBUGPL(DBG_CILV, "data_fifo[%d]=0x%08x\n",
		    i, (unsigned)_core_if->data_fifo[i]);
	}

	_core_if->data_fifo_dbg = fifo_dbg;
	_core_if->pcgcctl = (uint32_t*)(((uint32_t)reg_base) + IFXUSB_PCGCCTL_OFFSET);

	/*
	 * Store the contents of the hardware configuration registers here for
	 * easy access later.
	 */
	_core_if->hwcfg1.d32 = ifxusb_rreg(&_core_if->core_global_regs->ghwcfg1);
	_core_if->hwcfg2.d32 = ifxusb_rreg(&_core_if->core_global_regs->ghwcfg2);
	_core_if->hwcfg3.d32 = ifxusb_rreg(&_core_if->core_global_regs->ghwcfg3);
	_core_if->hwcfg4.d32 = ifxusb_rreg(&_core_if->core_global_regs->ghwcfg4);

	IFX_DEBUGPL(DBG_CILV,"hwcfg1=%08x\n",_core_if->hwcfg1.d32);
	IFX_DEBUGPL(DBG_CILV,"hwcfg2=%08x\n",_core_if->hwcfg2.d32);
	IFX_DEBUGPL(DBG_CILV,"hwcfg3=%08x\n",_core_if->hwcfg3.d32);
	IFX_DEBUGPL(DBG_CILV,"hwcfg4=%08x\n",_core_if->hwcfg4.d32);


	#ifdef __DED_FIFO__
	{
		unsigned int countdown=0xFFFF;
		IFX_PRINT("Waiting for PHY Clock Lock!\n");
		while(--countdown && !( ifxusb_rreg(&_core_if->core_global_regs->grxfsiz) & (1<<9)))
		{
			UDELAY(1);
		}
		if(countdown)
			IFX_PRINT("PHY Clock Locked!\n");
		else
			IFX_PRINT("PHY Clock Not Locked! %08X\n",ifxusb_rreg(&_core_if->core_global_regs->grxfsiz));
	}
	#endif

	/* Create new workqueue and init works */
#if 0
	_core_if->wq_usb = create_singlethread_workqueue(_core_if->core_name);

	if(_core_if->wq_usb == 0)
	{
		IFX_DEBUGPL(DBG_CIL, "Creation of wq_usb failed\n");
		retval = -EINVAL;
		goto fail;
	}

	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
		INIT_WORK(&core_if->w_conn_id, w_conn_id_status_change, core_if);
		INIT_WORK(&core_if->w_wkp, w_wakeup_detected, core_if);
	#else
		INIT_WORK(&core_if->w_conn_id, w_conn_id_status_change);
		INIT_DELAYED_WORK(&core_if->w_wkp, w_wakeup_detected);
	#endif
#endif
	return 0;

fail:
	if( reg_base  != NULL) iounmap(reg_base );
	if( fifo_base != NULL) iounmap(fifo_base);
	if( fifo_dbg  != NULL) iounmap(fifo_dbg );
	return retval;
}

/*!
 \brief This function free the mapped address in the IFXUSB CSR data structures.
 \param _core_if Pointer of core_if structure
 */
#ifdef __IS_HOST__
void ifxusb_core_if_remove_h(ifxusb_core_if_t *_core_if)
#else
void ifxusb_core_if_remove_d(ifxusb_core_if_t *_core_if)
#endif
{
	/* Disable all interrupts */
	if( _core_if->core_global_regs  != NULL)
	{
		gusbcfg_data_t usbcfg   ={.d32 = 0};
		usbcfg.d32 = ifxusb_rreg( &_core_if->core_global_regs->gusbcfg);
		usbcfg.b.ForceDevMode=0;
		usbcfg.b.ForceHstMode=0;
		ifxusb_wreg( &_core_if->core_global_regs->gusbcfg,usbcfg.d32);
		ifxusb_mreg( &_core_if->core_global_regs->gahbcfg, 1, 0);
		ifxusb_wreg( &_core_if->core_global_regs->gintmsk, 0);
	}

	if( _core_if->core_global_regs  != NULL) iounmap(_core_if->core_global_regs );
	if( _core_if->data_fifo[0]      != NULL) iounmap(_core_if->data_fifo[0]     );
	if( _core_if->data_fifo_dbg     != NULL) iounmap(_core_if->data_fifo_dbg    );

#if 0
	if (_core_if->wq_usb)
		destroy_workqueue(_core_if->wq_usb);
#endif
	memset(_core_if, 0, sizeof(ifxusb_core_if_t));
}




/*!
 \brief This function enbles the controller's Global Interrupt in the AHB Config register.
 \param _core_if Pointer of core_if structure
 */
#ifdef __IS_HOST__
void ifxusb_enable_global_interrupts_h( ifxusb_core_if_t *_core_if )
#else
void ifxusb_enable_global_interrupts_d( ifxusb_core_if_t *_core_if )
#endif
{
	gahbcfg_data_t ahbcfg ={ .d32 = 0};
	ahbcfg.b.glblintrmsk = 1; /* Enable interrupts */
	ifxusb_mreg(&_core_if->core_global_regs->gahbcfg, 0, ahbcfg.d32);
}

/*!
 \brief This function disables the controller's Global Interrupt in the AHB Config register.
 \param _core_if Pointer of core_if structure
 */
#ifdef __IS_HOST__
void ifxusb_disable_global_interrupts_h( ifxusb_core_if_t *_core_if )
#else
void ifxusb_disable_global_interrupts_d( ifxusb_core_if_t *_core_if )
#endif
{
	gahbcfg_data_t ahbcfg ={ .d32 = 0};
	ahbcfg.b.glblintrmsk = 1; /* Enable interrupts */
	ifxusb_mreg(&_core_if->core_global_regs->gahbcfg, ahbcfg.d32, 0);
}




/*!
 \brief Flush Tx and Rx FIFO.
 \param _core_if Pointer of core_if structure
 */
#ifdef __IS_HOST__
void ifxusb_flush_both_fifo_h( ifxusb_core_if_t *_core_if )
#else
void ifxusb_flush_both_fifo_d( ifxusb_core_if_t *_core_if )
#endif
{
	ifxusb_core_global_regs_t *global_regs = _core_if->core_global_regs;
	volatile grstctl_t greset ={ .d32 = 0};
	int count = 0;

	IFX_DEBUGPL((DBG_CIL|DBG_PCDV), "%s\n", __func__);
	greset.b.rxfflsh = 1;
	greset.b.txfflsh = 1;
	greset.b.txfnum = 0x10;
	greset.b.intknqflsh=1;
	greset.b.hstfrm=1;
	ifxusb_wreg( &global_regs->grstctl, greset.d32 );

	do
	{
		greset.d32 = ifxusb_rreg( &global_regs->grstctl);
		if (++count > 10000)
		{
			IFX_WARN("%s() HANG! GRSTCTL=%0x\n", __func__, greset.d32);
			break;
		}
	} while (greset.b.rxfflsh == 1 || greset.b.txfflsh == 1);
	/* Wait for 3 PHY Clocks*/
	UDELAY(1);
}

/*!
 \brief Flush a Tx FIFO.
 \param _core_if Pointer of core_if structure
 \param _num Tx FIFO to flush. ( 0x10 for ALL TX FIFO )
 */
#ifdef __IS_HOST__
void ifxusb_flush_tx_fifo_h( ifxusb_core_if_t *_core_if, const int _num )
#else
void ifxusb_flush_tx_fifo_d( ifxusb_core_if_t *_core_if, const int _num )
#endif
{
	ifxusb_core_global_regs_t *global_regs = _core_if->core_global_regs;
	volatile grstctl_t greset ={ .d32 = 0};
	int count = 0;

	IFX_DEBUGPL((DBG_CIL|DBG_PCDV), "Flush Tx FIFO %d\n", _num);

	greset.b.intknqflsh=1;
	greset.b.txfflsh = 1;
	greset.b.txfnum = _num;
	ifxusb_wreg( &global_regs->grstctl, greset.d32 );

	do
	{
		greset.d32 = ifxusb_rreg( &global_regs->grstctl);
		if (++count > 10000&&(_num==0 ||_num==0x10))
		{
			IFX_WARN("%s() HANG! GRSTCTL=%0x GNPTXSTS=0x%08x\n",
			    __func__, greset.d32,
			ifxusb_rreg( &global_regs->gnptxsts));
			break;
		}
	} while (greset.b.txfflsh == 1);
	/* Wait for 3 PHY Clocks*/
	UDELAY(1);
}


/*!
 \brief Flush Rx FIFO.
 \param _core_if Pointer of core_if structure
 */
#ifdef __IS_HOST__
void ifxusb_flush_rx_fifo_h( ifxusb_core_if_t *_core_if )
#else
void ifxusb_flush_rx_fifo_d( ifxusb_core_if_t *_core_if )
#endif
{
	ifxusb_core_global_regs_t *global_regs = _core_if->core_global_regs;
	volatile grstctl_t greset ={ .d32 = 0};
	int count = 0;

	IFX_DEBUGPL((DBG_CIL|DBG_PCDV), "%s\n", __func__);
	greset.b.rxfflsh = 1;
	ifxusb_wreg( &global_regs->grstctl, greset.d32 );

	do
	{
		greset.d32 = ifxusb_rreg( &global_regs->grstctl);
		if (++count > 10000)
		{
			IFX_WARN("%s() HANG! GRSTCTL=%0x\n", __func__, greset.d32);
			break;
		}
	} while (greset.b.rxfflsh == 1);
	/* Wait for 3 PHY Clocks*/
	UDELAY(1);
}


#define SOFT_RESET_DELAY 100 /*!< Delay in msec of detection after soft-reset of usb core */

/*!
 \brief Do a soft reset of the core.  Be careful with this because it
        resets all the internal state machines of the core.
 \param _core_if Pointer of core_if structure
 */
#ifdef __IS_HOST__
int ifxusb_core_soft_reset_h(ifxusb_core_if_t *_core_if)
#else
int ifxusb_core_soft_reset_d(ifxusb_core_if_t *_core_if)
#endif
{
	ifxusb_core_global_regs_t *global_regs = _core_if->core_global_regs;
	volatile grstctl_t greset ={ .d32 = 0};
	int count = 0;

	IFX_DEBUGPL(DBG_CILV, "%s\n", __func__);
	/* Wait for AHB master IDLE state. */
	do
	{
		UDELAY(10);
		greset.d32 = ifxusb_rreg( &global_regs->grstctl);
		if (++count > 100000)
		{
			IFX_WARN("%s() HANG! AHB Idle GRSTCTL=%0x %x\n", __func__,
			greset.d32, greset.b.ahbidle);
			break;
		}
	} while (greset.b.ahbidle == 0);

	UDELAY(1);

	/* Core Soft Reset */
	count = 0;
	greset.b.csftrst = 1;
	ifxusb_wreg( &global_regs->grstctl, greset.d32 );

	#ifdef SOFT_RESET_DELAY
		MDELAY(SOFT_RESET_DELAY);
	#endif

	do
	{
		UDELAY(10);
		greset.d32 = ifxusb_rreg( &global_regs->grstctl);
		if (++count > 100000)
		{
			IFX_WARN("%s() HANG! Soft Reset GRSTCTL=%0x\n", __func__, greset.d32);
			return -1;
		}
	} while (greset.b.csftrst == 1);

	#ifdef SOFT_RESET_DELAY
		MDELAY(SOFT_RESET_DELAY);
	#endif

	// This is to reset the PHY of VR9
	#if defined(__IS_VR9__)
		if(_core_if->core_no==0)
		{
			set_bit (4, VR9_RCU_USBRESET2);
			MDELAY(50);
			clear_bit (4, VR9_RCU_USBRESET2);
		}
		else
		{
			set_bit (5, VR9_RCU_USBRESET2);
			MDELAY(50);
			clear_bit (5, VR9_RCU_USBRESET2);
		}
		MDELAY(50);
	#endif //defined(__IS_VR9__)

	IFX_PRINT("USB core #%d soft-reset\n",_core_if->core_no);

	return 0;
}

/*!
 \brief Turn on the USB Core Power
 \param _core_if Pointer of core_if structure
*/
#ifdef __IS_HOST__
void ifxusb_power_on_h (ifxusb_core_if_t *_core_if)
#else
void ifxusb_power_on_d (ifxusb_core_if_t *_core_if)
#endif
{
	IFX_DEBUGPL(DBG_ENTRY, "%s() %d\n", __func__, __LINE__ );
	#if defined(__UEIP__)

		// set clock gating
		#if defined(__IS_TWINPASS) || defined(__IS_DANUBE__)
			set_bit (4, (volatile unsigned long *)DANUBE_CGU_IFCCR);
			set_bit (5, (volatile unsigned long *)DANUBE_CGU_IFCCR);
		#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
		#if defined(__IS_AMAZON_SE__)
		//	clear_bit (4, (volatile unsigned long *)AMAZON_SE_CGU_IFCCR);
			clear_bit (5, (volatile unsigned long *)AMAZON_SE_CGU_IFCCR);
		#endif //defined(__IS_AMAZON_SE__)
		#if defined(__IS_AR9__)
			set_bit (0, (volatile unsigned long *)AR9_CGU_IFCCR);
			set_bit (1, (volatile unsigned long *)AR9_CGU_IFCCR);
		#endif //defined(__IS_AR9__)
		#if defined(__IS_VR9__)
//			set_bit (0, (volatile unsigned long *)VR9_CGU_IFCCR);
//			set_bit (1, (volatile unsigned long *)VR9_CGU_IFCCR);
		#endif //defined(__IS_VR9__)
		#if defined(__IS_AR10__)
//			set_bit (0, (volatile unsigned long *)VR9_CGU_IFCCR);
//			set_bit (1, (volatile unsigned long *)VR9_CGU_IFCCR);
		#endif //defined(__IS_AR10__)

		MDELAY(50);

		// set power
		AHBM_PMU_SETUP(IFX_PMU_ENABLE);
		#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__) || defined(__IS_AMAZON_SE__)
			USB_CTRL_PMU_SETUP(IFX_PMU_ENABLE);
			//#if defined(__IS_TWINPASS__)
			//	ifxusb_enable_afe_oc();
			//#endif
		#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__) || defined(__IS_AMAZON_SE__)
		#if defined(__IS_AR9__) || defined(__IS_VR9__)
			if(_core_if->core_no==0)
				USB0_CTRL_PMU_SETUP(IFX_PMU_ENABLE);
			else
				USB1_CTRL_PMU_SETUP(IFX_PMU_ENABLE);
		#endif //defined(__IS_AR9__) || defined(__IS_VR9__)
		#if defined(__IS_AR10__)
			if(_core_if->core_no==0)
				USB0_CTRL_PMU_SETUP(IFX_PMU_ENABLE);
			else
				USB1_CTRL_PMU_SETUP(IFX_PMU_ENABLE);
		#endif //defined(__IS_AR10__)

		MDELAY(50);

		if(_core_if->pcgcctl)
		{
			pcgcctl_data_t pcgcctl = {.d32=0};
			pcgcctl.b.gatehclk = 1;
			ifxusb_mreg(_core_if->pcgcctl, pcgcctl.d32, 0);
		}


		if(_core_if->core_global_regs)
		{
			// PHY configurations.
			#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			#if defined(__IS_AMAZON_SE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AMAZON_SE__)
			#if defined(__IS_AR9__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AR9__)
			#if defined(__IS_VR9__)
				//ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_VR9__)
			#if defined(__IS_AR10__)
				//ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AR10__)
		}
	#else //defined(__UEIP__)
		// set clock gating
		#if defined(__IS_TWINPASS) || defined(__IS_DANUBE__)
			set_bit (4, (volatile unsigned long *)DANUBE_CGU_IFCCR);
			set_bit (5, (volatile unsigned long *)DANUBE_CGU_IFCCR);
		#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
		#if defined(__IS_AMAZON_SE__)
		//	clear_bit (4, (volatile unsigned long *)AMAZON_SE_CGU_IFCCR);
			clear_bit (5, (volatile unsigned long *)AMAZON_SE_CGU_IFCCR);
		#endif //defined(__IS_AMAZON_SE__)
		#if defined(__IS_AR9__)
			set_bit (0, (volatile unsigned long *)AMAZON_S_CGU_IFCCR);
			set_bit (1, (volatile unsigned long *)AMAZON_S_CGU_IFCCR);
		#endif //defined(__IS_AR9__)

		MDELAY(50);

		// set power
		#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			clear_bit (6,  (volatile unsigned long *)DANUBE_PMU_PWDCR);//USB
			clear_bit (9,  (volatile unsigned long *)DANUBE_PMU_PWDCR);//DSL
			clear_bit (15, (volatile unsigned long *)DANUBE_PMU_PWDCR);//AHB
			#if defined(__IS_TWINPASS__)
				ifxusb_enable_afe_oc();
			#endif
		#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
		#if defined(__IS_AMAZON_SE__)
			clear_bit (6,  (volatile unsigned long *)AMAZON_SE_PMU_PWDCR);
			clear_bit (9,  (volatile unsigned long *)AMAZON_SE_PMU_PWDCR);
			clear_bit (15, (volatile unsigned long *)AMAZON_SE_PMU_PWDCR);
		#endif //defined(__IS_AMAZON_SE__)
		#if defined(__IS_AR9__)
			if(_core_if->core_no==0)
				clear_bit (6, (volatile unsigned long *)AMAZON_S_PMU_PWDCR);//USB
			else
				clear_bit (27, (volatile unsigned long *)AMAZON_S_PMU_PWDCR);//USB
			clear_bit (9, (volatile unsigned long *)AMAZON_S_PMU_PWDCR);//DSL
			clear_bit (15, (volatile unsigned long *)AMAZON_S_PMU_PWDCR);//AHB
		#endif //defined(__IS_AR9__)

		if(_core_if->core_global_regs)
		{
			// PHY configurations.
			#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			#if defined(__IS_AMAZON_SE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AMAZON_SE__)
			#if defined(__IS_AR9__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AR9__)
		}

	#endif //defined(__UEIP__)
}

/*!
 \brief Turn off the USB Core Power
 \param _core_if Pointer of core_if structure
*/
#ifdef __IS_HOST__
void ifxusb_power_off_h (ifxusb_core_if_t *_core_if)
#else
void ifxusb_power_off_d (ifxusb_core_if_t *_core_if)
#endif

{
	#ifdef __IS_HOST__
	ifxusb_phy_power_off_h (_core_if);
	#else
	ifxusb_phy_power_off_d (_core_if);
	#endif

	#if defined(__UEIP__)
		AHBM_PMU_SETUP(IFX_PMU_DISABLE);
		// set power
		if(_core_if->pcgcctl)
		{
			pcgcctl_data_t pcgcctl = {.d32=0};
			pcgcctl.b.gatehclk = 1;
			pcgcctl.b.stoppclk = 1;
			ifxusb_mreg(_core_if->pcgcctl, 0, pcgcctl.d32);
		}
		#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__) || defined(__IS_AMAZON_SE__)
			USB_CTRL_PMU_SETUP(IFX_PMU_DISABLE);
		#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__) || defined(__IS_AMAZON_SE__)
		#if defined(__IS_AR9__) || defined(__IS_VR9__)
			if(_core_if->core_no==0)
				USB0_CTRL_PMU_SETUP(IFX_PMU_DISABLE);
			else
				USB1_CTRL_PMU_SETUP(IFX_PMU_DISABLE);
		#endif //defined(__IS_AR9__) || defined(__IS_VR9__)
		#if defined(__IS_AR10__)
			if(_core_if->core_no==0)
				USB0_CTRL_PMU_SETUP(IFX_PMU_DISABLE);
			else
				USB1_CTRL_PMU_SETUP(IFX_PMU_DISABLE);
		#endif //defined(__IS_AR10__)
	#else //defined(__UEIP__)
		// set power
		#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			set_bit (6, (volatile unsigned long *)DANUBE_PMU_PWDCR);//USB
		#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
		#if defined(__IS_AMAZON_SE__)
			set_bit (6, (volatile unsigned long *)AMAZON_SE_PMU_PWDCR);//USB
		#endif //defined(__IS_AMAZON_SE__)
		#if defined(__IS_AR9__)
			if(_core_if->core_no==0)
				set_bit (6, (volatile unsigned long *)AMAZON_S_PMU_PWDCR);//USB
			else
				set_bit (27, (volatile unsigned long *)AMAZON_S_PMU_PWDCR);//USB
		#endif //defined(__IS_AR9__)
	#endif //defined(__UEIP__)
}

/*!
 \brief Turn on the USB PHY Power
 \param _core_if Pointer of core_if structure
*/
#ifdef __IS_HOST__
void ifxusb_phy_power_on_h (ifxusb_core_if_t *_core_if)
#else
void ifxusb_phy_power_on_d (ifxusb_core_if_t *_core_if)
#endif
{
	#if defined(__UEIP__)
		if(_core_if->core_global_regs)
		{
			#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			#if defined(__IS_AMAZON_SE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AMAZON_SE__)
			#if defined(__IS_AR9__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AR9__)

			#if  defined(__IS_VR9__) || defined(__IS_AR10__)
			{
				volatile rcu_ana_cfg1_data_t rcu_ana_cfg1 = {.d32=0};
				#if  defined(__IS_VR9__)
					if(_core_if->core_no==0)
						rcu_ana_cfg1.d32=ifxusb_rreg(((volatile uint32_t *)VR9_RCU_USB_ANA_CFG1A));
					else
						rcu_ana_cfg1.d32=ifxusb_rreg(((volatile uint32_t *)VR9_RCU_USB_ANA_CFG1B));
				#endif
				#if  defined(__IS_AR10__)
					if(_core_if->core_no==0)
						rcu_ana_cfg1.d32=ifxusb_rreg(((volatile uint32_t *)AR10_RCU_USB_ANA_CFG1A));
					else
						rcu_ana_cfg1.d32=ifxusb_rreg(((volatile uint32_t *)AR10_RCU_USB_ANA_CFG1B));
				#endif

				if(_core_if->params.ana_disconnect_threshold >=0 && _core_if->params.ana_disconnect_threshold <=7)
					rcu_ana_cfg1.b.dis_thr = _core_if->params.ana_disconnect_threshold ;
				if(_core_if->params.ana_squelch_threshold >=0 && _core_if->params.ana_squelch_threshold <=7)
					rcu_ana_cfg1.b.squs_thr = _core_if->params.ana_squelch_threshold ;
				if(_core_if->params.ana_transmitter_crossover >=0 && _core_if->params.ana_transmitter_crossover <=3)
					rcu_ana_cfg1.b.txhs_xv = _core_if->params.ana_transmitter_crossover ;
				if(_core_if->params.ana_transmitter_impedance >=0 && _core_if->params.ana_transmitter_impedance <=15)
					rcu_ana_cfg1.b.txsrci_xv = _core_if->params.ana_transmitter_impedance ;
				if(_core_if->params.ana_transmitter_dc_voltage >=0 && _core_if->params.ana_transmitter_dc_voltage <=15)
					rcu_ana_cfg1.b.txhs_dc = _core_if->params.ana_transmitter_dc_voltage ;
				if(_core_if->params.ana_transmitter_risefall_time >=0 && _core_if->params.ana_transmitter_risefall_time <=1)
					rcu_ana_cfg1.b.tx_edge = _core_if->params.ana_transmitter_risefall_time ;
				if(_core_if->params.ana_transmitter_pre_emphasis >=0 && _core_if->params.ana_transmitter_pre_emphasis <=1)
					rcu_ana_cfg1.b.tx_pee = _core_if->params.ana_transmitter_pre_emphasis ;

				#if  defined(__IS_VR9__)
					if(_core_if->core_no==0)
						ifxusb_wreg(((volatile uint32_t *)VR9_RCU_USB_ANA_CFG1A),rcu_ana_cfg1.d32);
					else
						ifxusb_wreg(((volatile uint32_t *)VR9_RCU_USB_ANA_CFG1B),rcu_ana_cfg1.d32);
				#endif
				#if  defined(__IS_AR10__)
					if(_core_if->core_no==0)
						ifxusb_wreg(((volatile uint32_t *)AR10_RCU_USB_ANA_CFG1A),rcu_ana_cfg1.d32);
					else
						ifxusb_wreg(((volatile uint32_t *)AR10_RCU_USB_ANA_CFG1B),rcu_ana_cfg1.d32);
				#endif
			}
			#endif

			if(_core_if->pcgcctl)
			{
				pcgcctl_data_t pcgcctl = {.d32=0};
				pcgcctl.b.stoppclk = 1;
				ifxusb_mreg(_core_if->pcgcctl, pcgcctl.d32, 0);
			}
		}

		#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__) || defined(__IS_AMAZON_SE__)
			USB_PHY_PMU_SETUP(IFX_PMU_ENABLE);
		#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__) || defined(__IS_AMAZON_SE__)
		#if defined(__IS_AR9__) || defined(__IS_VR9__) || defined(__IS_AR10__)
			if(_core_if->core_no==0)
				USB0_PHY_PMU_SETUP(IFX_PMU_ENABLE);
			else
				USB1_PHY_PMU_SETUP(IFX_PMU_ENABLE);
		#endif //defined(__IS_AR9__) || defined(__IS_VR9__)

		// PHY configurations.
		if(_core_if->core_global_regs)
		{
			#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			#if defined(__IS_AMAZON_SE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AMAZON_SE__)
			#if defined(__IS_AR9__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AR9__)
		}
	#else //defined(__UEIP__)
		// PHY configurations.
		if(_core_if->core_global_regs)
		{
			#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			#if defined(__IS_AMAZON_SE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AMAZON_SE__)
			#if defined(__IS_AR9__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AR9__)
		}

		#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			clear_bit (0,  (volatile unsigned long *)DANUBE_PMU_PWDCR);//PHY
		#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
		#if defined(__IS_AMAZON_SE__)
			clear_bit (0,  (volatile unsigned long *)AMAZON_SE_PMU_PWDCR);
		#endif //defined(__IS_AMAZON_SE__)
		#if defined(__IS_AR9__)
			if(_core_if->core_no==0)
				clear_bit (0,  (volatile unsigned long *)AMAZON_S_PMU_PWDCR);//PHY
			else
				clear_bit (26, (volatile unsigned long *)AMAZON_S_PMU_PWDCR);//PHY
		#endif //defined(__IS_AR9__)

		// PHY configurations.
		if(_core_if->core_global_regs)
		{
			#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			#if defined(__IS_AMAZON_SE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AMAZON_SE__)
			#if defined(__IS_AR9__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AR9__)
		}
	#endif //defined(__UEIP__)
}


/*!
 \brief Turn off the USB PHY Power
 \param _core_if Pointer of core_if structure
*/
#ifdef __IS_HOST__
void ifxusb_phy_power_off_h (ifxusb_core_if_t *_core_if)
#else
void ifxusb_phy_power_off_d (ifxusb_core_if_t *_core_if)
#endif
{
	#if defined(__UEIP__)
		if(_core_if->pcgcctl)
		{
			pcgcctl_data_t pcgcctl = {.d32=0};
			pcgcctl.b.stoppclk = 1;
			ifxusb_mreg(_core_if->pcgcctl, 0, pcgcctl.d32);
		}
		#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__) || defined(__IS_AMAZON_SE__)
			USB_PHY_PMU_SETUP(IFX_PMU_DISABLE);
		#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__) || defined(__IS_AMAZON_SE__)
		#if defined(__IS_AR9__) || defined(__IS_VR9__) || defined(__IS_AR10__)
			if(_core_if->core_no==0)
				USB0_PHY_PMU_SETUP(IFX_PMU_DISABLE);
			else
				USB1_PHY_PMU_SETUP(IFX_PMU_DISABLE);
		#endif // defined(__IS_AR9__) || defined(__IS_VR9__)
	#else //defined(__UEIP__)
		#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			set_bit (0, (volatile unsigned long *)DANUBE_PMU_PWDCR);//PHY
		#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
		#if defined(__IS_AMAZON_SE__)
			set_bit (0, (volatile unsigned long *)AMAZON_SE_PMU_PWDCR);//PHY
		#endif //defined(__IS_AMAZON_SE__)
		#if defined(__IS_AR9__)
			if(_core_if->core_no==0)
				set_bit (0, (volatile unsigned long *)AMAZON_S_PMU_PWDCR);//PHY
			else
				set_bit (26, (volatile unsigned long *)AMAZON_S_PMU_PWDCR);//PHY
		#endif //defined(__IS_AR9__)
	#endif //defined(__UEIP__)
}


/*!
 \brief Reset on the USB Core RCU
 \param _core_if Pointer of core_if structure
 */
#if defined(__IS_VR9__) || defined(__IS_AR10__)
static int CheckAlready(void)
{
	gusbcfg_data_t usbcfg   ={.d32 = 0};
	usbcfg.d32 = ifxusb_rreg((volatile uint32_t *)0xBE10100C);
	if(usbcfg.b.ForceDevMode)
		return 1;
	if(usbcfg.b.ForceHstMode)
		return 1;
	usbcfg.d32 = ifxusb_rreg((volatile uint32_t *)0xBE10600C);
	if(usbcfg.b.ForceDevMode)
		return 1;
	if(usbcfg.b.ForceHstMode)
		return 1;
	return 0;
}
#endif

#ifdef __IS_HOST__
	void ifxusb_hard_reset_h(ifxusb_core_if_t *_core_if)
#else
	void ifxusb_hard_reset_d(ifxusb_core_if_t *_core_if)
#endif
{
	#if defined(__UEIP__)
		#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			#if defined (__IS_HOST__)
				clear_bit (DANUBE_USBCFG_HDSEL_BIT, (volatile unsigned long *)DANUBE_RCU_USBCFG);
			#elif defined (__IS_DEVICE__)
				set_bit (DANUBE_USBCFG_HDSEL_BIT, (volatile unsigned long *)DANUBE_RCU_USBCFG);
			#endif
		#endif //defined(__IS_AMAZON_SE__)

		#if defined(__IS_AMAZON_SE__)
			#if defined (__IS_HOST__)
				clear_bit (AMAZON_SE_USBCFG_HDSEL_BIT, (volatile unsigned long *)AMAZON_SE_RCU_USBCFG);
			#elif defined (__IS_DEVICE__)
				set_bit (AMAZON_SE_USBCFG_HDSEL_BIT, (volatile unsigned long *)AMAZON_SE_RCU_USBCFG);
			#endif
		#endif //defined(__IS_AMAZON_SE__)

		#if defined(__IS_AR9__)
			if(_core_if->core_no==0)
			{
				#if defined (__IS_HOST__)
					clear_bit (AR9_USBCFG_HDSEL_BIT, (volatile unsigned long *)AR9_RCU_USB1CFG);
				#elif defined (__IS_DEVICE__)
					set_bit (AR9_USBCFG_HDSEL_BIT, (volatile unsigned long *)AR9_RCU_USB1CFG);
				#endif
			}
			else
			{
				#if defined (__IS_HOST__)
					clear_bit (AR9_USBCFG_HDSEL_BIT, (volatile unsigned long *)AR9_RCU_USB2CFG);
				#elif defined (__IS_DEVICE__)
					set_bit (AR9_USBCFG_HDSEL_BIT, (volatile unsigned long *)AR9_RCU_USB2CFG);
				#endif
			}
		#endif //defined(__IS_AR9__)

		#if defined(__IS_VR9__)
			if(!CheckAlready())
			{
				#if defined (__IS_HOST__)
					#if   defined (__IS_DUAL__)
						clear_bit (VR9_USBCFG_HDSEL_BIT, (volatile unsigned long *)VR9_RCU_USB1CFG);
						clear_bit (VR9_USBCFG_HDSEL_BIT, (volatile unsigned long *)VR9_RCU_USB2CFG);
					#elif defined (__IS_FIRST__)
						clear_bit (VR9_USBCFG_HDSEL_BIT, (volatile unsigned long *)VR9_RCU_USB1CFG);
						set_bit (VR9_USBCFG_HDSEL_BIT, (volatile unsigned long *)VR9_RCU_USB2CFG);
					#elif defined (__IS_SECOND__)
						set_bit (VR9_USBCFG_HDSEL_BIT, (volatile unsigned long *)VR9_RCU_USB1CFG);
						clear_bit (VR9_USBCFG_HDSEL_BIT, (volatile unsigned long *)VR9_RCU_USB2CFG);
					#endif
				#endif
				#if defined (__IS_DEVICE__)
					#if   defined (__IS_FIRST__)
						set_bit (VR9_USBCFG_HDSEL_BIT, (volatile unsigned long *)VR9_RCU_USB1CFG);
						clear_bit (VR9_USBCFG_HDSEL_BIT, (volatile unsigned long *)VR9_RCU_USB2CFG);
					#elif defined (__IS_SECOND__)
						clear_bit (VR9_USBCFG_HDSEL_BIT, (volatile unsigned long *)VR9_RCU_USB1CFG);
						set_bit (VR9_USBCFG_HDSEL_BIT, (volatile unsigned long *)VR9_RCU_USB2CFG);
					#endif
				#endif
			}
		#endif //defined(__IS_VR9__)

		#if defined(__IS_AR10__)
			if(!CheckAlready())
			{
				#if defined (__IS_HOST__)
					#if   defined (__IS_DUAL__)
						clear_bit (AR10_USBCFG_HDSEL_BIT, (volatile unsigned long *)AR10_RCU_USB1CFG);
						clear_bit (AR10_USBCFG_HDSEL_BIT, (volatile unsigned long *)AR10_RCU_USB2CFG);
					#elif defined (__IS_FIRST__)
						clear_bit (AR10_USBCFG_HDSEL_BIT, (volatile unsigned long *)AR10_RCU_USB1CFG);
						set_bit (AR10_USBCFG_HDSEL_BIT, (volatile unsigned long *)AR10_RCU_USB2CFG);
					#elif defined (__IS_SECOND__)
						set_bit (AR10_USBCFG_HDSEL_BIT, (volatile unsigned long *)AR10_RCU_USB1CFG);
						clear_bit (AR10_USBCFG_HDSEL_BIT, (volatile unsigned long *)AR10_RCU_USB2CFG);
					#endif
				#endif
				#if defined (__IS_DEVICE__)
					#if   defined (__IS_FIRST__)
						set_bit (AR10_USBCFG_HDSEL_BIT, (volatile unsigned long *)AR10_RCU_USB1CFG);
						clear_bit (AR10_USBCFG_HDSEL_BIT, (volatile unsigned long *)AR10_RCU_USB2CFG);
					#elif defined (__IS_SECOND__)
						clear_bit (AR10_USBCFG_HDSEL_BIT, (volatile unsigned long *)AR10_RCU_USB1CFG);
						set_bit (AR10_USBCFG_HDSEL_BIT, (volatile unsigned long *)AR10_RCU_USB2CFG);
					#endif
				#endif
			}
		#endif //defined(__IS_AR10__)

		// set the HC's byte-order to big-endian
		#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			set_bit   (DANUBE_USBCFG_HOST_END_BIT, (volatile unsigned long *)DANUBE_RCU_USBCFG);
			clear_bit (DANUBE_USBCFG_SLV_END_BIT, (volatile unsigned long *)DANUBE_RCU_USBCFG);
		#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
		#if defined(__IS_AMAZON_SE__)
			set_bit (AMAZON_SE_USBCFG_HOST_END_BIT, (volatile unsigned long *)AMAZON_SE_RCU_USBCFG);
			clear_bit (AMAZON_SE_USBCFG_SLV_END_BIT, (volatile unsigned long *)AMAZON_SE_RCU_USBCFG);
		#endif //defined(__IS_AMAZON_SE__)
		#if defined(__IS_AR9__)
			if(_core_if->core_no==0)
			{
				set_bit   (AR9_USBCFG_HOST_END_BIT, (volatile unsigned long *)AR9_RCU_USB1CFG);
				clear_bit (AR9_USBCFG_SLV_END_BIT, (volatile unsigned long *)AR9_RCU_USB1CFG);
			}
			else
			{
				set_bit   (AR9_USBCFG_HOST_END_BIT, (volatile unsigned long *)AR9_RCU_USB2CFG);
				clear_bit (AR9_USBCFG_SLV_END_BIT, (volatile unsigned long *)AR9_RCU_USB2CFG);
			}
		#endif //defined(__IS_AR9__)
		#if defined(__IS_VR9__)
			if(_core_if->core_no==0)
			{
				set_bit   (VR9_USBCFG_HOST_END_BIT, (volatile unsigned long *)VR9_RCU_USB1CFG);
				clear_bit (VR9_USBCFG_SLV_END_BIT, (volatile unsigned long *)VR9_RCU_USB1CFG);
			}
			else
			{
				set_bit   (VR9_USBCFG_HOST_END_BIT, (volatile unsigned long *)VR9_RCU_USB2CFG);
				clear_bit (VR9_USBCFG_SLV_END_BIT, (volatile unsigned long *)VR9_RCU_USB2CFG);
			}
		#endif //defined(__IS_VR9__)
		#if defined(__IS_AR10__)
			if(_core_if->core_no==0)
			{
				set_bit   (AR10_USBCFG_HOST_END_BIT, (volatile unsigned long *)AR10_RCU_USB1CFG);
				clear_bit (AR10_USBCFG_SLV_END_BIT, (volatile unsigned long *)AR10_RCU_USB1CFG);
			}
			else
			{
				set_bit   (AR10_USBCFG_HOST_END_BIT, (volatile unsigned long *)AR10_RCU_USB2CFG);
				clear_bit (AR10_USBCFG_SLV_END_BIT, (volatile unsigned long *)AR10_RCU_USB2CFG);
			}
		#endif //defined(__IS_AR10__)

		#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
		    set_bit (4, DANUBE_RCU_RESET);
			MDELAY(50);
		    clear_bit (4, DANUBE_RCU_RESET);
			MDELAY(50);
		#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)

		#if defined(__IS_AMAZON_SE__)
		    set_bit (4, AMAZON_SE_RCU_RESET);
			MDELAY(50);
		    clear_bit (4, AMAZON_SE_RCU_RESET);
			MDELAY(50);
		#endif //defined(__IS_AMAZON_SE__)

		#if defined(__IS_AR9__)
			if(_core_if->core_no==0)
			{
				set_bit (4, AR9_RCU_USBRESET);
				MDELAY(50);
				clear_bit (4, AR9_RCU_USBRESET);
			}
			else
			{
				set_bit (28, AR9_RCU_USBRESET);
				MDELAY(50);
				clear_bit (28, AR9_RCU_USBRESET);
			}
			MDELAY(50);
		#endif //defined(__IS_AR9__)
		#if defined(__IS_VR9__)
			if(!CheckAlready())
			{
				set_bit (4, VR9_RCU_USBRESET);
				MDELAY(50);
				clear_bit (4, VR9_RCU_USBRESET);
				MDELAY(50);
			}
		#endif //defined(__IS_VR9__)
		#if defined(__IS_AR10__)
			if(!CheckAlready())
			{
				set_bit (4, AR10_RCU_USBRESET);
				MDELAY(50);
				clear_bit (4, AR10_RCU_USBRESET);
				MDELAY(50);
			}
		#endif //defined(__IS_AR10__)

		#if defined(__IS_TWINPASS__)
			ifxusb_enable_afe_oc();
		#endif

		if(_core_if->core_global_regs)
		{
			// PHY configurations.
			#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			#if defined(__IS_AMAZON_SE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AMAZON_SE__)
			#if defined(__IS_AR9__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AR9__)
			#if defined(__IS_VR9__)
			//	ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_VR9__)
			#if defined(__IS_AR10__)
			//	ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AR10__)
		}
	#else //defined(__UEIP__)
		#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			#if defined (__IS_HOST__)
				clear_bit (DANUBE_USBCFG_HDSEL_BIT, (volatile unsigned long *)DANUBE_RCU_USBCFG);
			#elif defined (__IS_DEVICE__)
				set_bit (DANUBE_USBCFG_HDSEL_BIT, (volatile unsigned long *)DANUBE_RCU_USBCFG);
			#endif
		#endif //defined(__IS_AMAZON_SE__)

		#if defined(__IS_AMAZON_SE__)
			#if defined (__IS_HOST__)
				clear_bit (AMAZON_SE_USBCFG_HDSEL_BIT, (volatile unsigned long *)AMAZON_SE_RCU_USBCFG);
			#elif defined (__IS_DEVICE__)
				set_bit (AMAZON_SE_USBCFG_HDSEL_BIT, (volatile unsigned long *)AMAZON_SE_RCU_USBCFG);
			#endif
		#endif //defined(__IS_AMAZON_SE__)

		#if defined(__IS_AR9__)
			if(_core_if->core_no==0)
			{
				#if defined (__IS_HOST__)
					clear_bit (AMAZON_S_USBCFG_HDSEL_BIT, (volatile unsigned long *)AMAZON_S_RCU_USB1CFG);
				#elif defined (__IS_DEVICE__)
					set_bit (AMAZON_S_USBCFG_HDSEL_BIT, (volatile unsigned long *)AMAZON_S_RCU_USB1CFG);
				#endif
			}
			else
			{
				#if defined (__IS_HOST__)
					clear_bit (AMAZON_S_USBCFG_HDSEL_BIT, (volatile unsigned long *)AMAZON_S_RCU_USB2CFG);
				#elif defined (__IS_DEVICE__)
					set_bit (AMAZON_S_USBCFG_HDSEL_BIT, (volatile unsigned long *)AMAZON_S_RCU_USB2CFG);
				#endif
			}
		#endif //defined(__IS_AR9__)

		// set the HC's byte-order to big-endian
		#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			set_bit   (DANUBE_USBCFG_HOST_END_BIT, (volatile unsigned long *)DANUBE_RCU_USBCFG);
			clear_bit (DANUBE_USBCFG_SLV_END_BIT, (volatile unsigned long *)DANUBE_RCU_USBCFG);
		#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
		#if defined(__IS_AMAZON_SE__)
			set_bit (AMAZON_SE_USBCFG_HOST_END_BIT, (volatile unsigned long *)AMAZON_SE_RCU_USBCFG);
			clear_bit (AMAZON_SE_USBCFG_SLV_END_BIT, (volatile unsigned long *)AMAZON_SE_RCU_USBCFG);
		#endif //defined(__IS_AMAZON_SE__)
		#if defined(__IS_AR9__)
			if(_core_if->core_no==0)
			{
				set_bit   (AMAZON_S_USBCFG_HOST_END_BIT, (volatile unsigned long *)AMAZON_S_RCU_USB1CFG);
				clear_bit (AMAZON_S_USBCFG_SLV_END_BIT, (volatile unsigned long *)AMAZON_S_RCU_USB1CFG);
			}
			else
			{
				set_bit   (AMAZON_S_USBCFG_HOST_END_BIT, (volatile unsigned long *)AMAZON_S_RCU_USB2CFG);
				clear_bit (AMAZON_S_USBCFG_SLV_END_BIT, (volatile unsigned long *)AMAZON_S_RCU_USB2CFG);
			}
		#endif //defined(__IS_AR9__)

		#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
		    set_bit (4, DANUBE_RCU_RESET);
		#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
		#if defined(__IS_AMAZON_SE__)
		    set_bit (4, AMAZON_SE_RCU_RESET);
		#endif //defined(__IS_AMAZON_SE__)
		#if defined(__IS_AR9__)
			if(_core_if->core_no==0)
			{
				set_bit (4, AMAZON_S_RCU_USBRESET);
			}
			else
			{
				set_bit (28, AMAZON_S_RCU_USBRESET);
			}
		#endif //defined(__IS_AR9__)

		MDELAY(50);

		#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
		    clear_bit (4, DANUBE_RCU_RESET);
		#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
		#if defined(__IS_AMAZON_SE__)
		    clear_bit (4, AMAZON_SE_RCU_RESET);
		#endif //defined(__IS_AMAZON_SE__)
		#if defined(__IS_AR9__)
			if(_core_if->core_no==0)
			{
				clear_bit (4, AMAZON_S_RCU_USBRESET);
			}
			else
			{
				clear_bit (28, AMAZON_S_RCU_USBRESET);
			}
		#endif //defined(__IS_AR9__)

		MDELAY(50);

		#if defined(__IS_TWINPASS__)
			ifxusb_enable_afe_oc();
		#endif

		if(_core_if->core_global_regs)
		{
			// PHY configurations.
			#if defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_TWINPASS__) || defined(__IS_DANUBE__)
			#if defined(__IS_AMAZON_SE__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AMAZON_SE__)
			#if defined(__IS_AR9__)
				ifxusb_wreg (&_core_if->core_global_regs->guid,0x14014);
			#endif //defined(__IS_AR9__)
		}
	#endif //defined(__UEIP__)
}

#if defined(__GADGET_LED__) || defined(__HOST_LED__)
	#if defined(__UEIP__)
		static void *g_usb_led_trigger  = NULL;
	#endif

	void ifxusb_led_init(ifxusb_core_if_t *_core_if)
	{
		#if defined(__UEIP__)
			#if defined(IFX_LEDGPIO_USB_LED) || defined(IFX_LEDLED_USB_LED)
				if ( !g_usb_led_trigger )
				{
					ifx_led_trigger_register("usb_link", &g_usb_led_trigger);
					if ( g_usb_led_trigger != NULL )
					{
						struct ifx_led_trigger_attrib attrib = {0};
						attrib.delay_on     = 250;
						attrib.delay_off    = 250;
						attrib.timeout      = 2000;
						attrib.def_value    = 1;
						attrib.flags        = IFX_LED_TRIGGER_ATTRIB_DELAY_ON | IFX_LED_TRIGGER_ATTRIB_DELAY_OFF | IFX_LED_TRIGGER_ATTRIB_TIMEOUT | IFX_LED_TRIGGER_ATTRIB_DEF_VALUE;
						IFX_DEBUGP("Reg USB LED!!\n");
						ifx_led_trigger_set_attrib(g_usb_led_trigger, &attrib);
					}
				}
			#endif
		#endif //defined(__UEIP__)
	}

	void ifxusb_led_free(ifxusb_core_if_t *_core_if)
	{
		#if defined(__UEIP__)
			if ( g_usb_led_trigger )
			{
			    ifx_led_trigger_deregister(g_usb_led_trigger);
			    g_usb_led_trigger = NULL;
			}
		#endif //defined(__UEIP__)
	}

	/*!
	   \brief Turn off the USB 5V VBus Power
	   \param _core_if        Pointer of core_if structure
	 */
	void ifxusb_led(ifxusb_core_if_t *_core_if)
	{
		#if defined(__UEIP__)
			if(g_usb_led_trigger)
				ifx_led_trigger_activate(g_usb_led_trigger);
		#else
		#endif //defined(__UEIP__)
	}
#endif // defined(__GADGET_LED__) || defined(__HOST_LED__)


//#define __VERBOSE_DUMP__
/*!
 \brief internal routines for debugging
 */
#ifdef __IS_HOST__
void ifxusb_dump_msg_h(const u8 *buf, unsigned int length)
#else
void ifxusb_dump_msg_d(const u8 *buf, unsigned int length)
#endif
{
#ifdef __DEBUG__
	unsigned int	start, num, i;
	char		line[52], *p;

	if (length >= 512)
		return;
	start = 0;
	while (length > 0)
	{
		num = min(length, 16u);
		p = line;
		for (i = 0; i < num; ++i)
		{
			if (i == 8)
				*p++ = ' ';
			sprintf(p, " %02x", buf[i]);
			p += 3;
		}
		*p = 0;
		IFX_PRINT( "%6x: %s\n", start, line);
		buf += num;
		start += num;
		length -= num;
	}
#endif
}

/*!
 \brief internal routines for debugging, reads the SPRAM and prints its content
 */
#ifdef __IS_HOST__
void ifxusb_dump_spram_h(ifxusb_core_if_t *_core_if)
#else
void ifxusb_dump_spram_d(ifxusb_core_if_t *_core_if)
#endif
{
#ifdef __ENABLE_DUMP__
	volatile uint8_t *addr, *start_addr, *end_addr;
	uint32_t size;
	IFX_PRINT("SPRAM Data:\n");
	start_addr = (void*)_core_if->core_global_regs;
	IFX_PRINT("Base Address: 0x%8X\n", (uint32_t)start_addr);

	start_addr = (void*)_core_if->data_fifo_dbg;
	IFX_PRINT("Starting Address: 0x%8X\n", (uint32_t)start_addr);

	size=_core_if->hwcfg3.b.dfifo_depth;
	size<<=2;
	size+=0x200;
	size&=0x0003FFFC;

	end_addr = (void*)_core_if->data_fifo_dbg;
	end_addr += size;

	for(addr = start_addr; addr < end_addr; addr+=16)
	{
		IFX_PRINT("0x%8X:  %02X  %02X  %02X  %02X  %02X  %02X  %02X  %02X  %02X  %02X  %02X  %02X  %02X  %02X  %02X  %02X \n", (uint32_t)addr,
			addr[ 0], addr[ 1], addr[ 2], addr[ 3],
			addr[ 4], addr[ 5], addr[ 6], addr[ 7],
			addr[ 8], addr[ 9], addr[10], addr[11],
			addr[12], addr[13], addr[14], addr[15]
			);
	}
	return;
#endif //__ENABLE_DUMP__
}


/*!
 \brief internal routines for debugging, reads the core global registers and prints them
 */
#ifdef __IS_HOST__
void ifxusb_dump_registers_h(ifxusb_core_if_t *_core_if)
#else
void ifxusb_dump_registers_d(ifxusb_core_if_t *_core_if)
#endif
{
#ifdef __ENABLE_DUMP__
	int i;
	volatile uint32_t *addr;
	#ifdef __IS_DEVICE__
		volatile uint32_t *addri,*addro;
	#endif

	IFX_PRINT("Core #%d\n",_core_if->core_no);
	IFX_PRINT("========================================\n");
	IFX_PRINT("Core Global Registers\n");
	{
		gotgctl_data_t gotgctl;
		addr=&_core_if->core_global_regs->gotgctl;
		gotgctl.d32=ifxusb_rreg(addr);
		IFX_PRINT("  GOTGCTL   @0x%08X : 0x%08X\n",(uint32_t)addr,gotgctl.d32);
		#ifdef __VERBOSE_DUMP__
		IFX_PRINT("            currmod       : %x\n",gotgctl.b.currmod);
		IFX_PRINT("            bsesvld       : %x\n",gotgctl.b.bsesvld);
		IFX_PRINT("            asesvld       : %x\n",gotgctl.b.asesvld);
		IFX_PRINT("            conidsts      : %x\n",gotgctl.b.conidsts);
		IFX_PRINT("            devhnpen      : %x\n",gotgctl.b.devhnpen);
		IFX_PRINT("            hstsethnpen   : %x\n",gotgctl.b.hstsethnpen);
		IFX_PRINT("            hnpreq        : %x\n",gotgctl.b.hnpreq);
		IFX_PRINT("            hstnegscs     : %x\n",gotgctl.b.hstnegscs);
		IFX_PRINT("            sesreq        : %x\n",gotgctl.b.sesreq);
		IFX_PRINT("            sesreqscs     : %x\n",gotgctl.b.sesreqscs);
		#endif // __VERBOSE_DUMP__
	}
	{
		gotgint_data_t gotgint;
		addr=&_core_if->core_global_regs->gotgint;
		gotgint.d32=ifxusb_rreg(addr);
		IFX_PRINT("  GOTGINT   @0x%08X : 0x%08X\n",(uint32_t)addr,gotgint.d32);
		#ifdef __VERBOSE_DUMP__
		IFX_PRINT("            debdone           : %x\n",gotgint.b.debdone);
		IFX_PRINT("            adevtoutchng      : %x\n",gotgint.b.adevtoutchng);
		IFX_PRINT("            hstnegdet         : %x\n",gotgint.b.hstnegdet);
		IFX_PRINT("            hstnegsucstschng  : %x\n",gotgint.b.hstnegsucstschng);
		IFX_PRINT("            sesreqsucstschng  : %x\n",gotgint.b.sesreqsucstschng);
		IFX_PRINT("            sesenddet         : %x\n",gotgint.b.sesenddet);
		#endif // __VERBOSE_DUMP__
	}
	{
		gahbcfg_data_t gahbcfg;
		addr=&_core_if->core_global_regs->gahbcfg;
		gahbcfg.d32=ifxusb_rreg(addr);
		IFX_PRINT("  GAHBCFG   @0x%08X : 0x%08X\n",(uint32_t)addr,gahbcfg.d32);
		#ifdef __VERBOSE_DUMP__
		IFX_PRINT("            ptxfemplvl        : %x\n",gahbcfg.b.ptxfemplvl);
		IFX_PRINT("            nptxfemplvl       : %x\n",gahbcfg.b.nptxfemplvl);
		IFX_PRINT("            dmaenable         : %x\n",gahbcfg.b.dmaenable);
		IFX_PRINT("            hburstlen         : %x\n",gahbcfg.b.hburstlen);
		IFX_PRINT("            glblintrmsk       : %x\n",gahbcfg.b.glblintrmsk);
		#endif // __VERBOSE_DUMP__
	}
	{
		gusbcfg_data_t gusbcfg;
		addr=&_core_if->core_global_regs->gusbcfg;
		gusbcfg.d32=ifxusb_rreg(addr);
		IFX_PRINT("  GUSBCFG   @0x%08X : 0x%08X\n",(uint32_t)addr,gusbcfg.d32);
		#ifdef __VERBOSE_DUMP__
		IFX_PRINT("            ForceDevMode            : %x\n",gusbcfg.b.ForceDevMode);
		IFX_PRINT("            ForceHstMode            : %x\n",gusbcfg.b.ForceHstMode);
		IFX_PRINT("            TxEndDelay              : %x\n",gusbcfg.b.TxEndDelay);
		IFX_PRINT("            term_sel_dl_pulse       : %x\n",gusbcfg.b.term_sel_dl_pulse);
		IFX_PRINT("            otgutmifssel            : %x\n",gusbcfg.b.otgutmifssel);
		IFX_PRINT("            phylpwrclksel           : %x\n",gusbcfg.b.phylpwrclksel);
		IFX_PRINT("            usbtrdtim               : %x\n",gusbcfg.b.usbtrdtim);
		IFX_PRINT("            hnpcap                  : %x\n",gusbcfg.b.hnpcap);
		IFX_PRINT("            srpcap                  : %x\n",gusbcfg.b.srpcap);
		IFX_PRINT("            physel                  : %x\n",gusbcfg.b.physel);
		IFX_PRINT("            fsintf                  : %x\n",gusbcfg.b.fsintf);
		IFX_PRINT("            ulpi_utmi_sel           : %x\n",gusbcfg.b.ulpi_utmi_sel);
		IFX_PRINT("            phyif                   : %x\n",gusbcfg.b.phyif);
		IFX_PRINT("            toutcal                 : %x\n",gusbcfg.b.toutcal);
		#endif // __VERBOSE_DUMP__
	}
	{
		grstctl_t grstctl;
		addr=&_core_if->core_global_regs->grstctl;
		grstctl.d32=ifxusb_rreg(addr);
		IFX_PRINT("  GRSTCTL   @0x%08X : 0x%08X\n",(uint32_t)addr,grstctl.d32);
		#ifdef __VERBOSE_DUMP__
		IFX_PRINT("            ahbidle         : %x\n",grstctl.b.ahbidle);
		IFX_PRINT("            dmareq          : %x\n",grstctl.b.dmareq);
		IFX_PRINT("            txfnum          : %x\n",grstctl.b.txfnum);
		IFX_PRINT("            txfflsh         : %x\n",grstctl.b.txfflsh);
		IFX_PRINT("            rxfflsh         : %x\n",grstctl.b.rxfflsh);
		IFX_PRINT("            intknqflsh      : %x\n",grstctl.b.intknqflsh);
		IFX_PRINT("            hstfrm          : %x\n",grstctl.b.hstfrm);
		IFX_PRINT("            hsftrst         : %x\n",grstctl.b.hsftrst);
		IFX_PRINT("            csftrst         : %x\n",grstctl.b.csftrst);
		#endif // __VERBOSE_DUMP__
	}
	{
		gint_data_t gint;
		addr=&_core_if->core_global_regs->gintsts;
		gint.d32=ifxusb_rreg(addr);
		IFX_PRINT("  GINTSTS   @0x%08X : 0x%08X\n",(uint32_t)addr,gint.d32);
		#ifdef __VERBOSE_DUMP__
		IFX_PRINT("            wkupintr      : %x\n",gint.b.wkupintr);
		IFX_PRINT("            sessreqintr   : %x\n",gint.b.sessreqintr);
		IFX_PRINT("            disconnect    : %x\n",gint.b.disconnect);
		IFX_PRINT("            conidstschng  : %x\n",gint.b.conidstschng);
		IFX_PRINT("            ptxfempty     : %x\n",gint.b.ptxfempty);
		IFX_PRINT("            hcintr        : %x\n",gint.b.hcintr);
		IFX_PRINT("            portintr      : %x\n",gint.b.portintr);
		IFX_PRINT("            fetsuspmsk    : %x\n",gint.b.fetsuspmsk);
		IFX_PRINT("            incomplisoout : %x\n",gint.b.incomplisoout);
		IFX_PRINT("            incomplisoin  : %x\n",gint.b.incomplisoin);
		IFX_PRINT("            outepintr     : %x\n",gint.b.outepintr);
		IFX_PRINT("            inepintr      : %x\n",gint.b.inepintr);
		IFX_PRINT("            epmismatch    : %x\n",gint.b.epmismatch);
		IFX_PRINT("            eopframe      : %x\n",gint.b.eopframe);
		IFX_PRINT("            isooutdrop    : %x\n",gint.b.isooutdrop);
		IFX_PRINT("            enumdone      : %x\n",gint.b.enumdone);
		IFX_PRINT("            usbreset      : %x\n",gint.b.usbreset);
		IFX_PRINT("            usbsuspend    : %x\n",gint.b.usbsuspend);
		IFX_PRINT("            erlysuspend   : %x\n",gint.b.erlysuspend);
		IFX_PRINT("            i2cintr       : %x\n",gint.b.i2cintr);
		IFX_PRINT("            goutnakeff    : %x\n",gint.b.goutnakeff);
		IFX_PRINT("            ginnakeff     : %x\n",gint.b.ginnakeff);
		IFX_PRINT("            nptxfempty    : %x\n",gint.b.nptxfempty);
		IFX_PRINT("            rxstsqlvl     : %x\n",gint.b.rxstsqlvl);
		IFX_PRINT("            sofintr       : %x\n",gint.b.sofintr);
		IFX_PRINT("            otgintr       : %x\n",gint.b.otgintr);
		IFX_PRINT("            modemismatch  : %x\n",gint.b.modemismatch);
		#endif // __VERBOSE_DUMP__
		addr=&_core_if->core_global_regs->gintmsk;
		gint.d32=ifxusb_rreg(addr);
		IFX_PRINT("  GINTMSK   @0x%08X : 0x%08X\n",(uint32_t)addr,gint.d32);
		#ifdef __VERBOSE_DUMP__
		IFX_PRINT("            wkupintr      : %x\n",gint.b.wkupintr);
		IFX_PRINT("            sessreqintr   : %x\n",gint.b.sessreqintr);
		IFX_PRINT("            disconnect    : %x\n",gint.b.disconnect);
		IFX_PRINT("            conidstschng  : %x\n",gint.b.conidstschng);
		IFX_PRINT("            ptxfempty     : %x\n",gint.b.ptxfempty);
		IFX_PRINT("            hcintr        : %x\n",gint.b.hcintr);
		IFX_PRINT("            portintr      : %x\n",gint.b.portintr);
		IFX_PRINT("            fetsuspmsk    : %x\n",gint.b.fetsuspmsk);
		IFX_PRINT("            incomplisoout : %x\n",gint.b.incomplisoout);
		IFX_PRINT("            incomplisoin  : %x\n",gint.b.incomplisoin);
		IFX_PRINT("            outepintr     : %x\n",gint.b.outepintr);
		IFX_PRINT("            inepintr      : %x\n",gint.b.inepintr);
		IFX_PRINT("            epmismatch    : %x\n",gint.b.epmismatch);
		IFX_PRINT("            eopframe      : %x\n",gint.b.eopframe);
		IFX_PRINT("            isooutdrop    : %x\n",gint.b.isooutdrop);
		IFX_PRINT("            enumdone      : %x\n",gint.b.enumdone);
		IFX_PRINT("            usbreset      : %x\n",gint.b.usbreset);
		IFX_PRINT("            usbsuspend    : %x\n",gint.b.usbsuspend);
		IFX_PRINT("            erlysuspend   : %x\n",gint.b.erlysuspend);
		IFX_PRINT("            i2cintr       : %x\n",gint.b.i2cintr);
		IFX_PRINT("            goutnakeff    : %x\n",gint.b.goutnakeff);
		IFX_PRINT("            ginnakeff     : %x\n",gint.b.ginnakeff);
		IFX_PRINT("            nptxfempty    : %x\n",gint.b.nptxfempty);
		IFX_PRINT("            rxstsqlvl     : %x\n",gint.b.rxstsqlvl);
		IFX_PRINT("            sofintr       : %x\n",gint.b.sofintr);
		IFX_PRINT("            otgintr       : %x\n",gint.b.otgintr);
		IFX_PRINT("            modemismatch  : %x\n",gint.b.modemismatch);
		#endif // __VERBOSE_DUMP__
	}
	{
		gi2cctl_data_t gi2cctl;
		addr=&_core_if->core_global_regs->gi2cctl;
		gi2cctl.d32=ifxusb_rreg(addr);
		IFX_PRINT("  GI2CCTL   @0x%08X : 0x%08X\n",(uint32_t)addr,gi2cctl.d32);
		#ifdef __VERBOSE_DUMP__
		IFX_PRINT("            bsydne     : %x\n",gi2cctl.b.bsydne);
		IFX_PRINT("            rw         : %x\n",gi2cctl.b.rw);
		IFX_PRINT("            i2cdevaddr : %x\n",gi2cctl.b.i2cdevaddr);
		IFX_PRINT("            i2csuspctl : %x\n",gi2cctl.b.i2csuspctl);
		IFX_PRINT("            ack        : %x\n",gi2cctl.b.ack);
		IFX_PRINT("            i2cen      : %x\n",gi2cctl.b.i2cen);
		IFX_PRINT("            addr       : %x\n",gi2cctl.b.addr);
		IFX_PRINT("            regaddr    : %x\n",gi2cctl.b.regaddr);
		IFX_PRINT("            rwdata     : %x\n",gi2cctl.b.rwdata);
		#endif // __VERBOSE_DUMP__
	}
	addr=&_core_if->core_global_regs->gpvndctl;
	IFX_PRINT("  GPVNDCTL  @0x%08X : 0x%08X\n",(uint32_t)addr,ifxusb_rreg(addr));
	addr=&_core_if->core_global_regs->ggpio;
	IFX_PRINT("  GGPIO     @0x%08X : 0x%08X\n",(uint32_t)addr,ifxusb_rreg(addr));
	addr=&_core_if->core_global_regs->guid;
	IFX_PRINT("  GUID      @0x%08X : 0x%08X\n",(uint32_t)addr,ifxusb_rreg(addr));
	addr=&_core_if->core_global_regs->gsnpsid;
	IFX_PRINT("  GSNPSID   @0x%08X : 0x%08X\n",(uint32_t)addr,ifxusb_rreg(addr));
	{
		hwcfg1_data_t hwcfg1;
		addr=&_core_if->core_global_regs->ghwcfg1;
		hwcfg1.d32=ifxusb_rreg(addr);
		IFX_PRINT("  GHWCFG1   @0x%08X : 0x%08X\n",(uint32_t)addr,hwcfg1.d32);
		#ifdef __VERBOSE_DUMP__
		IFX_PRINT("            ep_dir15 : %x\n",hwcfg1.b.ep_dir15);
		IFX_PRINT("            ep_dir14 : %x\n",hwcfg1.b.ep_dir14);
		IFX_PRINT("            ep_dir13 : %x\n",hwcfg1.b.ep_dir13);
		IFX_PRINT("            ep_dir12 : %x\n",hwcfg1.b.ep_dir12);
		IFX_PRINT("            ep_dir11 : %x\n",hwcfg1.b.ep_dir11);
		IFX_PRINT("            ep_dir10 : %x\n",hwcfg1.b.ep_dir10);
		IFX_PRINT("            ep_dir09 : %x\n",hwcfg1.b.ep_dir09);
		IFX_PRINT("            ep_dir08 : %x\n",hwcfg1.b.ep_dir08);
		IFX_PRINT("            ep_dir07 : %x\n",hwcfg1.b.ep_dir07);
		IFX_PRINT("            ep_dir06 : %x\n",hwcfg1.b.ep_dir06);
		IFX_PRINT("            ep_dir05 : %x\n",hwcfg1.b.ep_dir05);
		IFX_PRINT("            ep_dir04 : %x\n",hwcfg1.b.ep_dir04);
		IFX_PRINT("            ep_dir03 : %x\n",hwcfg1.b.ep_dir03);
		IFX_PRINT("            ep_dir02 : %x\n",hwcfg1.b.ep_dir02);
		IFX_PRINT("            ep_dir01 : %x\n",hwcfg1.b.ep_dir01);
		IFX_PRINT("            ep_dir00 : %x\n",hwcfg1.b.ep_dir00);
		#endif // __VERBOSE_DUMP__
	}
	{
		hwcfg2_data_t hwcfg2;
		addr=&_core_if->core_global_regs->ghwcfg2;
		hwcfg2.d32=ifxusb_rreg(addr);
		IFX_PRINT("  GHWCFG2   @0x%08X : 0x%08X\n",(uint32_t)addr,hwcfg2.d32);
		#ifdef __VERBOSE_DUMP__
		IFX_PRINT("            dev_token_q_depth      : %x\n",hwcfg2.b.dev_token_q_depth);
		IFX_PRINT("            host_perio_tx_q_depth  : %x\n",hwcfg2.b.host_perio_tx_q_depth);
		IFX_PRINT("            nonperio_tx_q_depth    : %x\n",hwcfg2.b.nonperio_tx_q_depth);
		IFX_PRINT("            rx_status_q_depth      : %x\n",hwcfg2.b.rx_status_q_depth);
		IFX_PRINT("            dynamic_fifo           : %x\n",hwcfg2.b.dynamic_fifo);
		IFX_PRINT("            perio_ep_supported     : %x\n",hwcfg2.b.perio_ep_supported);
		IFX_PRINT("            num_host_chan          : %x\n",hwcfg2.b.num_host_chan);
		IFX_PRINT("            num_dev_ep             : %x\n",hwcfg2.b.num_dev_ep);
		IFX_PRINT("            fs_phy_type            : %x\n",hwcfg2.b.fs_phy_type);
		IFX_PRINT("            hs_phy_type            : %x\n",hwcfg2.b.hs_phy_type);
		IFX_PRINT("            point2point            : %x\n",hwcfg2.b.point2point);
		IFX_PRINT("            architecture           : %x\n",hwcfg2.b.architecture);
		IFX_PRINT("            op_mode                : %x\n",hwcfg2.b.op_mode);
		#endif // __VERBOSE_DUMP__
	}
	{
		hwcfg3_data_t hwcfg3;
		addr=&_core_if->core_global_regs->ghwcfg3;
		hwcfg3.d32=ifxusb_rreg(addr);
		IFX_PRINT("  GHWCFG3   @0x%08X : 0x%08X\n",(uint32_t)addr,hwcfg3.d32);
		#ifdef __VERBOSE_DUMP__
		IFX_PRINT("            dfifo_depth            : %x\n",hwcfg3.b.dfifo_depth);
		IFX_PRINT("            synch_reset_type       : %x\n",hwcfg3.b.synch_reset_type);
		IFX_PRINT("            optional_features      : %x\n",hwcfg3.b.optional_features);
		IFX_PRINT("            vendor_ctrl_if         : %x\n",hwcfg3.b.vendor_ctrl_if);
		IFX_PRINT("            i2c                    : %x\n",hwcfg3.b.otg_func);
		IFX_PRINT("            otg_func               : %x\n",hwcfg3.b.otg_func);
		IFX_PRINT("            packet_size_cntr_width : %x\n",hwcfg3.b.packet_size_cntr_width);
		IFX_PRINT("            xfer_size_cntr_width   : %x\n",hwcfg3.b.xfer_size_cntr_width);
		#endif // __VERBOSE_DUMP__
	}
	{
		hwcfg4_data_t hwcfg4;
		addr=&_core_if->core_global_regs->ghwcfg4;
		hwcfg4.d32=ifxusb_rreg(addr);
		IFX_PRINT("  GHWCFG4   @0x%08X : 0x%08X\n",(uint32_t)addr,hwcfg4.d32);
		#ifdef __VERBOSE_DUMP__
		IFX_PRINT("            desc_dma_dyn         : %x\n",hwcfg4.b.desc_dma_dyn);
		IFX_PRINT("            desc_dma             : %x\n",hwcfg4.b.desc_dma);
		IFX_PRINT("            num_in_eps           : %x\n",hwcfg4.b.num_in_eps);
		IFX_PRINT("            ded_fifo_en          : %x\n",hwcfg4.b.ded_fifo_en);
		IFX_PRINT("            session_end_filt_en  : %x\n",hwcfg4.b.session_end_filt_en);
		IFX_PRINT("            b_valid_filt_en      : %x\n",hwcfg4.b.b_valid_filt_en);
		IFX_PRINT("            a_valid_filt_en      : %x\n",hwcfg4.b.a_valid_filt_en);
		IFX_PRINT("            vbus_valid_filt_en   : %x\n",hwcfg4.b.vbus_valid_filt_en);
		IFX_PRINT("            iddig_filt_en        : %x\n",hwcfg4.b.iddig_filt_en);
		IFX_PRINT("            num_dev_mode_ctrl_ep : %x\n",hwcfg4.b.num_dev_mode_ctrl_ep);
		IFX_PRINT("            utmi_phy_data_width  : %x\n",hwcfg4.b.utmi_phy_data_width);
		IFX_PRINT("            min_ahb_freq         : %x\n",hwcfg4.b.min_ahb_freq);
		IFX_PRINT("            power_optimiz        : %x\n",hwcfg4.b.power_optimiz);
		IFX_PRINT("            num_dev_perio_in_ep  : %x\n",hwcfg4.b.num_dev_perio_in_ep);
		#endif // __VERBOSE_DUMP__
	}

	{
		pcgcctl_data_t pcgcctl;
		addr=_core_if->pcgcctl;
		pcgcctl.d32=ifxusb_rreg(addr);
		IFX_PRINT("  PCGCCTL   @0x%08X : 0x%08X\n",(uint32_t)addr,pcgcctl.d32);
		#ifdef __VERBOSE_DUMP__
		IFX_PRINT("            physuspended  : %x\n",pcgcctl.b.physuspended);
		IFX_PRINT("            rstpdwnmodule : %x\n",pcgcctl.b.rstpdwnmodule);
		IFX_PRINT("            pwrclmp       : %x\n",pcgcctl.b.pwrclmp);
		IFX_PRINT("            gatehclk      : %x\n",pcgcctl.b.gatehclk);
		IFX_PRINT("            stoppclk      : %x\n",pcgcctl.b.stoppclk);
		#endif // __VERBOSE_DUMP__
	}

	addr=&_core_if->core_global_regs->grxfsiz;
	IFX_PRINT("  GRXFSIZ   @0x%08X : 0x%08X\n",(uint32_t)addr,ifxusb_rreg(addr));
	{
		fifosize_data_t fifosize;
		#ifdef __IS_HOST__
			addr=&_core_if->core_global_regs->gnptxfsiz;
			fifosize.d32=ifxusb_rreg(addr);
			IFX_PRINT("  GNPTXFSIZ @0x%08X : 0x%08X\n",(uint32_t)addr,fifosize.d32);
			#ifdef __VERBOSE_DUMP__
			IFX_PRINT("            depth     : %x\n",fifosize.b.depth);
			IFX_PRINT("            startaddr : %x\n",fifosize.b.startaddr);
			#endif // __VERBOSE_DUMP__
			addr=&_core_if->core_global_regs->hptxfsiz;
			fifosize.d32=ifxusb_rreg(addr);
			IFX_PRINT("  HPTXFSIZ  @0x%08X : 0x%08X\n",(uint32_t)addr,fifosize.d32);
			#ifdef __VERBOSE_DUMP__
			IFX_PRINT("            depth     : %x\n",fifosize.b.depth);
			IFX_PRINT("            startaddr : %x\n",fifosize.b.startaddr);
			#endif // __VERBOSE_DUMP__
		#endif //__IS_HOST__
		#ifdef __IS_DEVICE__
			#ifdef __DED_FIFO__
				addr=&_core_if->core_global_regs->gnptxfsiz;
				fifosize.d32=ifxusb_rreg(addr);
				IFX_PRINT("    GNPTXFSIZ @0x%08X : 0x%08X\n",(uint32_t)addr,fifosize.d32);
				#ifdef __VERBOSE_DUMP__
				IFX_PRINT("            depth     : %x\n",fifosize.b.depth);
				IFX_PRINT("            startaddr : %x\n",fifosize.b.startaddr);
				#endif // __VERBOSE_DUMP__
				for (i=0; i<= _core_if->hwcfg4.b.num_in_eps; i++)
				{
					addr=&_core_if->core_global_regs->dptxfsiz_dieptxf[i];
					fifosize.d32=ifxusb_rreg(addr);
					IFX_PRINT("    DPTXFSIZ[%d] @0x%08X : 0x%08X\n",i,(uint32_t)addr,fifosize.d32);
					#ifdef __VERBOSE_DUMP__
					IFX_PRINT("            depth     : %x\n",fifosize.b.depth);
					IFX_PRINT("            startaddr : %x\n",fifosize.b.startaddr);
					#endif // __VERBOSE_DUMP__
				}
			#else
				addr=&_core_if->core_global_regs->gnptxfsiz;
				fifosize.d32=ifxusb_rreg(addr);
				IFX_PRINT("    TXFSIZ[00] @0x%08X : 0x%08X\n",(uint32_t)addr,fifosize.d32);
				#ifdef __VERBOSE_DUMP__
				IFX_PRINT("            depth     : %x\n",fifosize.b.depth);
				IFX_PRINT("            startaddr : %x\n",fifosize.b.startaddr);
				#endif // __VERBOSE_DUMP__
				for (i=0; i< _core_if->hwcfg4.b.num_dev_perio_in_ep; i++)
				{
					addr=&_core_if->core_global_regs->dptxfsiz_dieptxf[i];
					fifosize.d32=ifxusb_rreg(addr);
					IFX_PRINT("    TXFSIZ[%02d] @0x%08X : 0x%08X\n",i+1,(uint32_t)addr,fifosize.d32);
					#ifdef __VERBOSE_DUMP__
					IFX_PRINT("            depth     : %x\n",fifosize.b.depth);
					IFX_PRINT("            startaddr : %x\n",fifosize.b.startaddr);
					#endif // __VERBOSE_DUMP__
				}
			#endif
		#endif //__IS_DEVICE__
	}

	#ifdef __IS_HOST__
		IFX_PRINT("  Host Global Registers\n");
		{
			hcfg_data_t hcfg;
			addr=&_core_if->host_global_regs->hcfg;
			hcfg.d32=ifxusb_rreg(addr);
			IFX_PRINT("    HCFG      @0x%08X : 0x%08X\n",(uint32_t)addr,hcfg.d32);
			#ifdef __VERBOSE_DUMP__
			IFX_PRINT("            fslssupp      : %x\n",hcfg.b.fslssupp);
			IFX_PRINT("            fslspclksel   : %x\n",hcfg.b.fslspclksel);
			#endif // __VERBOSE_DUMP__
		}
		addr=&_core_if->host_global_regs->hfir;
		IFX_PRINT("    HFIR      @0x%08X : 0x%08X\n",(uint32_t)addr,ifxusb_rreg(addr));
		{
			hfnum_data_t hfnum;
			addr=&_core_if->host_global_regs->hfnum;
			hfnum.d32=ifxusb_rreg(addr);
			IFX_PRINT("    HFNUM     @0x%08X : 0x%08X\n",(uint32_t)addr,hfnum.d32);
			#ifdef __VERBOSE_DUMP__
			IFX_PRINT("            frrem : %x\n",hfnum.b.frrem);
			IFX_PRINT("            frnum : %x\n",hfnum.b.frnum);
			#endif // __VERBOSE_DUMP__
		}
		{
			hptxsts_data_t hptxsts;
			addr=&_core_if->host_global_regs->hptxsts;
			hptxsts.d32=ifxusb_rreg(addr);
			IFX_PRINT("    HPTXSTS   @0x%08X : 0x%08X\n",(uint32_t)addr,hptxsts.d32);
			#ifdef __VERBOSE_DUMP__
			IFX_PRINT("            ptxqtop_odd       : %x\n",hptxsts.b.ptxqtop_odd);
			IFX_PRINT("            ptxqtop_chnum     : %x\n",hptxsts.b.ptxqtop_chnum);
			IFX_PRINT("            ptxqtop_token     : %x\n",hptxsts.b.ptxqtop_token);
			IFX_PRINT("            ptxqtop_terminate : %x\n",hptxsts.b.ptxqtop_terminate);
			IFX_PRINT("            ptxqspcavail      : %x\n",hptxsts.b.ptxqspcavail);
			IFX_PRINT("            ptxfspcavail      : %x\n",hptxsts.b.ptxfspcavail );
			#endif // __VERBOSE_DUMP__
		}
		addr=&_core_if->host_global_regs->haint;
		IFX_PRINT("    HAINT     @0x%08X : 0x%08X\n",(uint32_t)addr,ifxusb_rreg(addr));
		addr=&_core_if->host_global_regs->haintmsk;
		IFX_PRINT("    HAINTMSK  @0x%08X : 0x%08X\n",(uint32_t)addr,ifxusb_rreg(addr));
		{
			hprt0_data_t hprt0;
			addr= _core_if->hprt0;
			hprt0.d32=ifxusb_rreg(addr);
			IFX_PRINT("    HPRT0     @0x%08X : 0x%08X\n",(uint32_t)addr,hprt0.d32);
			#ifdef __VERBOSE_DUMP__
			IFX_PRINT("            prtspd          : %x\n",hprt0.b.prtspd);
			IFX_PRINT("            prttstctl       : %x\n",hprt0.b.prttstctl);
			IFX_PRINT("            prtpwr          : %x\n",hprt0.b.prtpwr);
			IFX_PRINT("            prtlnsts        : %x\n",hprt0.b.prtlnsts);
			IFX_PRINT("            prtrst          : %x\n",hprt0.b.prtrst);
			IFX_PRINT("            prtsusp         : %x\n",hprt0.b.prtsusp);
			IFX_PRINT("            prtres          : %x\n",hprt0.b.prtres);
			IFX_PRINT("            prtovrcurrchng  : %x\n",hprt0.b.prtovrcurrchng);
			IFX_PRINT("            prtovrcurract   : %x\n",hprt0.b.prtovrcurract);
			IFX_PRINT("            prtenchng       : %x\n",hprt0.b.prtenchng);
			IFX_PRINT("            prtena          : %x\n",hprt0.b.prtena);
			IFX_PRINT("            prtconndet      : %x\n",hprt0.b.prtconndet );
			IFX_PRINT("            prtconnsts      : %x\n",hprt0.b.prtconnsts);
			#endif // __VERBOSE_DUMP__
		}

		for (i=0; i<MAX_EPS_CHANNELS; i++)
		{
			IFX_PRINT("  Host Channel %d Specific Registers\n", i);
			{
				hcchar_data_t hcchar;
				addr=&_core_if->hc_regs[i]->hcchar;
				hcchar.d32=ifxusb_rreg(addr);
				IFX_PRINT("    HCCHAR    @0x%08X : 0x%08X\n",(uint32_t)addr,hcchar.d32);
				#ifdef __VERBOSE_DUMP__
				IFX_PRINT("            chen      : %x\n",hcchar.b.chen);
				IFX_PRINT("            chdis     : %x\n",hcchar.b.chdis);
				IFX_PRINT("            oddfrm    : %x\n",hcchar.b.oddfrm);
				IFX_PRINT("            devaddr   : %x\n",hcchar.b.devaddr);
				IFX_PRINT("            multicnt  : %x\n",hcchar.b.multicnt);
				IFX_PRINT("            eptype    : %x\n",hcchar.b.eptype);
				IFX_PRINT("            lspddev   : %x\n",hcchar.b.lspddev);
				IFX_PRINT("            epdir     : %x\n",hcchar.b.epdir);
				IFX_PRINT("            epnum     : %x\n",hcchar.b.epnum);
				IFX_PRINT("            mps       : %x\n",hcchar.b.mps);
				#endif // __VERBOSE_DUMP__
			}
			{
				hcsplt_data_t hcsplt;
				addr=&_core_if->hc_regs[i]->hcsplt;
				hcsplt.d32=ifxusb_rreg(addr);
				IFX_PRINT("    HCSPLT    @0x%08X : 0x%08X\n",(uint32_t)addr,hcsplt.d32);
				#ifdef __VERBOSE_DUMP__
				IFX_PRINT("            spltena  : %x\n",hcsplt.b.spltena);
				IFX_PRINT("            compsplt : %x\n",hcsplt.b.compsplt);
				IFX_PRINT("            xactpos  : %x\n",hcsplt.b.xactpos);
				IFX_PRINT("            hubaddr  : %x\n",hcsplt.b.hubaddr);
				IFX_PRINT("            prtaddr  : %x\n",hcsplt.b.prtaddr);
				#endif // __VERBOSE_DUMP__
			}
			{
				hcint_data_t hcint;
				addr=&_core_if->hc_regs[i]->hcint;
				hcint.d32=ifxusb_rreg(addr);
				IFX_PRINT("    HCINT     @0x%08X : 0x%08X\n",(uint32_t)addr,hcint.d32);
				#ifdef __VERBOSE_DUMP__
				IFX_PRINT("            datatglerr : %x\n",hcint.b.datatglerr);
				IFX_PRINT("            frmovrun   : %x\n",hcint.b.frmovrun);
				IFX_PRINT("            bblerr     : %x\n",hcint.b.bblerr);
				IFX_PRINT("            xacterr    : %x\n",hcint.b.xacterr);
				IFX_PRINT("            nyet       : %x\n",hcint.b.nyet);
				IFX_PRINT("            ack        : %x\n",hcint.b.ack);
				IFX_PRINT("            nak        : %x\n",hcint.b.nak);
				IFX_PRINT("            stall      : %x\n",hcint.b.stall);
				IFX_PRINT("            ahberr     : %x\n",hcint.b.ahberr);
				IFX_PRINT("            chhltd     : %x\n",hcint.b.chhltd);
				IFX_PRINT("            xfercomp   : %x\n",hcint.b.xfercomp);
				#endif // __VERBOSE_DUMP__
				addr=&_core_if->hc_regs[i]->hcintmsk;
				hcint.d32=ifxusb_rreg(addr);
				IFX_PRINT("    HCINTMSK  @0x%08X : 0x%08X\n",(uint32_t)addr,hcint.d32);
				#ifdef __VERBOSE_DUMP__
				IFX_PRINT("            datatglerr : %x\n",hcint.b.datatglerr);
				IFX_PRINT("            frmovrun   : %x\n",hcint.b.frmovrun);
				IFX_PRINT("            bblerr     : %x\n",hcint.b.bblerr);
				IFX_PRINT("            xacterr    : %x\n",hcint.b.xacterr);
				IFX_PRINT("            nyet       : %x\n",hcint.b.nyet);
				IFX_PRINT("            ack        : %x\n",hcint.b.ack);
				IFX_PRINT("            nak        : %x\n",hcint.b.nak);
				IFX_PRINT("            stall      : %x\n",hcint.b.stall);
				IFX_PRINT("            ahberr     : %x\n",hcint.b.ahberr);
				IFX_PRINT("            chhltd     : %x\n",hcint.b.chhltd);
				IFX_PRINT("            xfercomp   : %x\n",hcint.b.xfercomp);
				#endif // __VERBOSE_DUMP__
			}
			{
				hctsiz_data_t hctsiz;
				addr=&_core_if->hc_regs[i]->hctsiz;
				hctsiz.d32=ifxusb_rreg(addr);
				IFX_PRINT("    HCTSIZ    @0x%08X : 0x%08X\n",(uint32_t)addr,hctsiz.d32);
				#ifdef __VERBOSE_DUMP__
				IFX_PRINT("            dopng     : %x\n",hctsiz.b.dopng);
				IFX_PRINT("            pid       : %x\n",hctsiz.b.pid);
				IFX_PRINT("            pktcnt    : %x\n",hctsiz.b.pktcnt);
				IFX_PRINT("            xfersize  : %x\n",hctsiz.b.xfersize);
				#endif // __VERBOSE_DUMP__
			}
			addr=&_core_if->hc_regs[i]->hcdma;
			IFX_PRINT("    HCDMA     @0x%08X : 0x%08X\n",(uint32_t)addr,ifxusb_rreg(addr));
		}
	#endif //__IS_HOST__

	#ifdef __IS_DEVICE__
		IFX_PRINT("  Device Global Registers\n");
		{
			dcfg_data_t dcfg;
			addr=&_core_if->dev_global_regs->dcfg;
			dcfg.d32=ifxusb_rreg(addr);
			IFX_PRINT("    DCFG      @0x%08X : 0x%08X\n",(uint32_t)addr,dcfg.d32);
			#ifdef __VERBOSE_DUMP__
			IFX_PRINT("            perschintvl     : %x\n",dcfg.b.perschintvl);
			IFX_PRINT("            descdma         : %x\n",dcfg.b.descdma);
			IFX_PRINT("            epmscnt         : %x\n",dcfg.b.epmscnt);
			IFX_PRINT("            perfrint        : %x\n",dcfg.b.perfrint);
			IFX_PRINT("            devaddr         : %x\n",dcfg.b.devaddr);
			IFX_PRINT("            nzstsouthshk    : %x\n",dcfg.b.nzstsouthshk);
			IFX_PRINT("            devspd          : %x\n",dcfg.b.devspd);
			#endif // __VERBOSE_DUMP__
		}
		{
			dctl_data_t dctl;
			addr=&_core_if->dev_global_regs->dctl;
			dctl.d32=ifxusb_rreg(addr);
			IFX_PRINT("    DCTL      @0x%08X : 0x%08X\n",(uint32_t)addr,dctl.d32);
			#ifdef __VERBOSE_DUMP__
			IFX_PRINT("            ifrmnum        : %x\n",dctl.b.ifrmnum);
			IFX_PRINT("            gmc            : %x\n",dctl.b.gmc);
			IFX_PRINT("            gcontbna       : %x\n",dctl.b.gcontbna);
			IFX_PRINT("            pwronprgdone   : %x\n",dctl.b.pwronprgdone);
			IFX_PRINT("            cgoutnak       : %x\n",dctl.b.cgoutnak);
			IFX_PRINT("            sgoutnak       : %x\n",dctl.b.sgoutnak);
			IFX_PRINT("            cgnpinnak      : %x\n",dctl.b.cgnpinnak);
			IFX_PRINT("            sgnpinnak      : %x\n",dctl.b.sgnpinnak);
			IFX_PRINT("            tstctl         : %x\n",dctl.b.tstctl);
			IFX_PRINT("            goutnaksts     : %x\n",dctl.b.goutnaksts);
			IFX_PRINT("            gnpinnaksts    : %x\n",dctl.b.gnpinnaksts);
			IFX_PRINT("            sftdiscon      : %x\n",dctl.b.sftdiscon);
			IFX_PRINT("            rmtwkupsig     : %x\n",dctl.b.rmtwkupsig);
			#endif // __VERBOSE_DUMP__
		}
		{
			dsts_data_t dsts;
			addr=&_core_if->dev_global_regs->dsts;
			dsts.d32=ifxusb_rreg(addr);
			IFX_PRINT("    DSTS      @0x%08X : 0x%08X\n",(uint32_t)addr,dsts.d32);
			#ifdef __VERBOSE_DUMP__
			IFX_PRINT("            soffn          : %x\n",dsts.b.soffn);
			IFX_PRINT("            errticerr      : %x\n",dsts.b.errticerr);
			IFX_PRINT("            enumspd        : %x\n",dsts.b.enumspd);
			IFX_PRINT("            suspsts        : %x\n",dsts.b.suspsts);
			#endif // __VERBOSE_DUMP__
		}
		{
			diepint_data_t diepint;
			addr=&_core_if->dev_global_regs->diepmsk;
			diepint.d32=ifxusb_rreg(addr);
			IFX_PRINT("    DIEPMSK   @0x%08X : 0x%08X\n",(uint32_t)addr,diepint.d32);
			#ifdef __VERBOSE_DUMP__
			IFX_PRINT("            nakmsk          : %x\n",diepint.b.nakmsk);
			IFX_PRINT("            bna             : %x\n",diepint.b.bna);
			IFX_PRINT("            txfifoundrn     : %x\n",diepint.b.txfifoundrn);
			IFX_PRINT("            emptyintr       : %x\n",diepint.b.emptyintr);
			IFX_PRINT("            inepnakeff      : %x\n",diepint.b.inepnakeff);
			IFX_PRINT("            intknepmis      : %x\n",diepint.b.intknepmis);
			IFX_PRINT("            intktxfemp      : %x\n",diepint.b.intktxfemp);
			IFX_PRINT("            timeout         : %x\n",diepint.b.timeout);
			IFX_PRINT("            ahberr          : %x\n",diepint.b.ahberr);
			IFX_PRINT("            epdisabled      : %x\n",diepint.b.epdisabled);
			IFX_PRINT("            xfercompl       : %x\n",diepint.b.xfercompl);
			#endif // __VERBOSE_DUMP__
		}
		{
			doepint_data_t doepint;
			addr=&_core_if->dev_global_regs->doepmsk;
			doepint.d32=ifxusb_rreg(addr);
			IFX_PRINT("    DOEPMSK   @0x%08X : 0x%08X\n",(uint32_t)addr,doepint.d32);
			#ifdef __VERBOSE_DUMP__
			IFX_PRINT("            nyetmsk        : %x\n",doepint.b.nyetmsk);
			IFX_PRINT("            nakmsk         : %x\n",doepint.b.nakmsk);
			IFX_PRINT("            bbleerrmsk     : %x\n",doepint.b.bbleerrmsk );
			IFX_PRINT("            bna            : %x\n",doepint.b.bna);
			IFX_PRINT("            outpkterr      : %x\n",doepint.b.outpkterr);
			IFX_PRINT("            back2backsetup : %x\n",doepint.b.back2backsetup);
			IFX_PRINT("            stsphsercvd    : %x\n",doepint.b.stsphsercvd);
			IFX_PRINT("            outtknepdis    : %x\n",doepint.b.outtknepdis);
			IFX_PRINT("            setup          : %x\n",doepint.b.setup);
			IFX_PRINT("            ahberr         : %x\n",doepint.b.ahberr);
			IFX_PRINT("            epdisabled     : %x\n",doepint.b.epdisabled);
			IFX_PRINT("            xfercompl      : %x\n",doepint.b.xfercompl);
			#endif // __VERBOSE_DUMP__
		}
		{
			daint_data_t daint;
			addr=&_core_if->dev_global_regs->daintmsk;
			daint.d32=ifxusb_rreg(addr);
			IFX_PRINT("    DAINTMSK  @0x%08X : 0x%08X\n",(uint32_t)addr,daint.d32);
			#ifdef __VERBOSE_DUMP__
			IFX_PRINT("            out       :%x\n",daint.eps.out);
			IFX_PRINT("            in        :%x\n",daint.eps.in);
			#endif // __VERBOSE_DUMP__
			addr=&_core_if->dev_global_regs->daint;
			daint.d32=ifxusb_rreg(addr);
			IFX_PRINT("    DAINT     @0x%08X : 0x%08X\n",(uint32_t)addr,daint.d32);
			#ifdef __VERBOSE_DUMP__
			IFX_PRINT("            out       :%x\n",daint.eps.out);
			IFX_PRINT("            in        :%x\n",daint.eps.in);
			#endif // __VERBOSE_DUMP__
		}
		addr=&_core_if->dev_global_regs->dvbusdis;
		IFX_PRINT("    DVBUSID   @0x%08X : 0x%08X\n",(uint32_t)addr,ifxusb_rreg(addr));
		addr=&_core_if->dev_global_regs->dvbuspulse;
		IFX_PRINT("    DVBUSPULS @0x%08X : 0x%08X\n", (uint32_t)addr,ifxusb_rreg(addr));

		addr=&_core_if->dev_global_regs->dtknqr1;
		IFX_PRINT("    DTKNQR1   @0x%08X : 0x%08X\n",(uint32_t)addr,ifxusb_rreg(addr));
		if (_core_if->hwcfg2.b.dev_token_q_depth > 6) {
			addr=&_core_if->dev_global_regs->dtknqr2;
			IFX_PRINT("    DTKNQR2   @0x%08X : 0x%08X\n", (uint32_t)addr,ifxusb_rreg(addr));
		}

		if (_core_if->hwcfg2.b.dev_token_q_depth > 14)
		{
			addr=&_core_if->dev_global_regs->dtknqr3_dthrctl;
			IFX_PRINT("    DTKNQR3_DTHRCTL  @0x%08X : 0x%08X\n", (uint32_t)addr, ifxusb_rreg(addr));
		}

		if (_core_if->hwcfg2.b.dev_token_q_depth > 22)
		{
			addr=&_core_if->dev_global_regs->dtknqr4_fifoemptymsk;
			IFX_PRINT("    DTKNQR4  @0x%08X : 0x%08X\n", (uint32_t)addr, ifxusb_rreg(addr));
		}

		//for (i=0; i<= MAX_EPS_CHANNELS; i++)
		//for (i=0; i<= 10; i++)
		for (i=0; i<= 3; i++)
		{
			IFX_PRINT("  Device EP %d Registers\n", i);
			addri=&_core_if->in_ep_regs[i]->diepctl;addro=&_core_if->out_ep_regs[i]->doepctl;
			IFX_PRINT("    DEPCTL    I: 0x%08X O: 0x%08X\n",ifxusb_rreg(addri),ifxusb_rreg(addro));
			                                        addro=&_core_if->out_ep_regs[i]->doepfn;
			IFX_PRINT("    DEPFN     I:            O: 0x%08X\n",ifxusb_rreg(addro));
			addri=&_core_if->in_ep_regs[i]->diepint;addro=&_core_if->out_ep_regs[i]->doepint;
			IFX_PRINT("    DEPINT    I: 0x%08X O: 0x%08X\n",ifxusb_rreg(addri),ifxusb_rreg(addro));
			addri=&_core_if->in_ep_regs[i]->dieptsiz;addro=&_core_if->out_ep_regs[i]->doeptsiz;
			IFX_PRINT("    DETSIZ    I: 0x%08X O: 0x%08X\n",ifxusb_rreg(addri),ifxusb_rreg(addro));
			addri=&_core_if->in_ep_regs[i]->diepdma;addro=&_core_if->out_ep_regs[i]->doepdma;
			IFX_PRINT("    DEPDMA    I: 0x%08X O: 0x%08X\n",ifxusb_rreg(addri),ifxusb_rreg(addro));
			addri=&_core_if->in_ep_regs[i]->dtxfsts;
			IFX_PRINT("    DTXFSTS   I: 0x%08X\n",ifxusb_rreg(addri)                   );
			addri=&_core_if->in_ep_regs[i]->diepdmab;addro=&_core_if->out_ep_regs[i]->doepdmab;
			IFX_PRINT("    DEPDMAB   I: 0x%08X O: 0x%08X\n",ifxusb_rreg(addri),ifxusb_rreg(addro));
		}
	#endif //__IS_DEVICE__
#endif //__ENABLE_DUMP__
}



#ifdef __IS_HOST__
void do_suspend_h(ifxusb_core_if_t *core_if)
{
	hprt0_data_t               hprt0 = {.d32 = 0};

	core_if->issuspended=1;
	hprt0.d32 = ifxusb_read_hprt0 (core_if);
	hprt0.b.prtsusp = 1;
	hprt0.b.prtres = 0;
	hprt0.b.prtpwr = 0;
	ifxusb_wreg(core_if->hprt0, hprt0.d32);

	ifxusb_vbus_off(core_if);
	mdelay(100);
	ifxusb_phy_power_off_h(core_if);
	ifxusb_power_off_h(core_if);
}
void do_resume_h(ifxusb_core_if_t *core_if)
{
	hprt0_data_t               hprt0 = {.d32 = 0};
	ifxusb_vbus_on(core_if);
	mdelay(100);
	ifxusb_power_on_h(core_if);
	ifxusb_phy_power_on_h(core_if);

	hprt0.d32 = ifxusb_read_hprt0 (core_if);
	hprt0.b.prtsusp = 0;
	hprt0.b.prtres = 1;
	hprt0.b.prtpwr = 1;
	ifxusb_wreg(core_if->hprt0, hprt0.d32);
	mdelay(100);
	hprt0.d32 = ifxusb_read_hprt0 (core_if);
	hprt0.b.prtsusp = 0;
	hprt0.b.prtres = 0;
	ifxusb_wreg(core_if->hprt0, hprt0.d32);
	core_if->issuspended=0;
}
#endif
#ifdef __IS_DEVICE__
void do_suspend_d(ifxusb_core_if_t *core_if)
{
	ifxusb_phy_power_off_d(core_if);
	ifxusb_power_off_d(core_if);
}
void do_resume_d(ifxusb_core_if_t *core_if)
{
	dctl_data_t dctl = {.d32=0};

	ifxusb_power_on_d(core_if);
	ifxusb_phy_power_on_d(core_if);
	dctl.d32=ifxusb_rreg(&core_if->dev_global_regs->dctl);
	dctl.b.sftdiscon=1;
	ifxusb_wreg(&core_if->dev_global_regs->dctl,dctl.d32);
	mdelay(50);
	dctl.b.sftdiscon=0;
	ifxusb_wreg(&core_if->dev_global_regs->dctl,dctl.d32);
}
#endif

