/*
 * Copyright (c) 2009, Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/usb.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>

#include <osdep.h>
#include "hif.h"

#ifndef ATH_WINHTC
#include <linux/usb.h>
#endif

#include "../include/hif_usb_internal.h"

static void *HIFInit(adf_os_handle_t os_hdl);

HTC_DRVREG_CALLBACKS htcDrvRegCallbacks;

void HIF_UsbDataOut_Callback(void *phifhdl, struct sk_buff * context)
{
    HIF_DEVICE_USB  *hHIF = (HIF_DEVICE_USB *)phifhdl;

    hHIF->htcCallbacks.txCompletionHandler(hHIF->htcCallbacks.Context, context);
}

void HIF_UsbDataIn_Callback(void *phifhdl, struct sk_buff * buf)
{
    HIF_DEVICE_USB  *hHIF = (HIF_DEVICE_USB *)phifhdl;

    hHIF->htcCallbacks.rxCompletionHandler(hHIF->htcCallbacks.Context, buf, USB_WLAN_RX_PIPE);
}

void HIF_UsbMsgIn_Callback(void *phifhdl, struct sk_buff * buf)
{
    HIF_DEVICE_USB  *hHIF = (HIF_DEVICE_USB *)phifhdl;

    hHIF->htcCallbacks.rxCompletionHandler(hHIF->htcCallbacks.Context, buf, USB_REG_IN_PIPE);
}

void HIF_Usb_Stop_Callback(void *phifhdl, void *pwmihdl, uint8_t usb_status)
{
    HIF_DEVICE_USB  *hHIF = (HIF_DEVICE_USB *)phifhdl;

    /* To check wheter usb device removed in initializtion stage */
    hHIF->htcCallbacks.usbDeviceRemovedHandler(hHIF->htcCallbacks.Context);
    hHIF->htcCallbacks.usbStopHandler(pwmihdl, usb_status);
}

void HIF_UsbCmdOut_Callback(void *phifhdl, struct sk_buff * context)
{
    HIF_DEVICE_USB  *hHIF = (HIF_DEVICE_USB *)phifhdl;
    
    hHIF->htcCallbacks.txCompletionHandler(hHIF->htcCallbacks.Context, context);
}

A_STATUS HIF_USBDeviceInserted(adf_os_handle_t os_hdl)
{
    void *hHIF = NULL;
    A_STATUS status = A_OK;

    hHIF = HIFInit(os_hdl);

    /* Inform HTC */
    if ( (hHIF != NULL) && htcDrvRegCallbacks.deviceInsertedHandler) {
        status = htcDrvRegCallbacks.deviceInsertedHandler(hHIF, os_hdl);
        if (A_SUCCESS(status))
            return A_OK;
        else
            return A_ERROR;
    }
    else {
        return A_ERROR;
    }
}

void HIF_USBDeviceDetached(adf_os_handle_t os_hdl)
{ 
    void* hif_handle = OS_GET_HIF_HANDLE(os_hdl);
    void* htc_handle = OS_GET_HTC_HANDLE(os_hdl);

    printk("HIF_USBDeviceDetached\n");
    
    /* Inform HTC */
    if (htc_handle != NULL) {
        if (htcDrvRegCallbacks.deviceRemovedHandler) {
            htcDrvRegCallbacks.deviceRemovedHandler(htc_handle);
        }
    }

    if (hif_handle != NULL) {
        HIFShutDown(hif_handle);
        OS_GET_HIF_HANDLE(os_hdl) = NULL;
    }
}

hif_status_t HIF_register(HTC_DRVREG_CALLBACKS *callbacks)
{
    htcDrvRegCallbacks.deviceInsertedHandler = callbacks->deviceInsertedHandler;
    htcDrvRegCallbacks.deviceRemovedHandler = callbacks->deviceRemovedHandler;

    return HIF_OK;
}

hif_status_t HIFSend(HIF_HANDLE hHIF, uint8_t PipeID, struct sk_buff * hdr_buf, struct sk_buff * buf)
{
    hif_status_t status = HIF_OK;
    HIF_DEVICE_USB *macp = (HIF_DEVICE_USB *)hHIF;
    struct sk_buff * sendBuf;
    uint8_t *data = NULL;
    uint32_t len = 0;

    /* If necessary, link hdr_buf & buf */
    if (hdr_buf != NULL) 
    {
 		if(!pskb_expand_head(hdr_buf, 0, buf->len, GFP_ATOMIC))
             memcpy(skb_put(hdr_buf, buf->len), buf->data, buf->len);

        sendBuf = hdr_buf;
    }
    else
    {
        sendBuf = buf;
    }

		data = sendBuf->data;
		len  = sendBuf->len;
    
    if ( PipeID == HIF_USB_PIPE_COMMAND ) 
    {
#ifdef ATH_WINHTC
        uint8_t *data = NULL;
        uint32_t len;
		data	= sendBuf->data;
        len		= sendBuf->len;


        status = OS_Usb_SubmitCmdOutUrb(macp->os_hdl, data, len, (void*)sendBuf);        
#else        
        status = ((osdev_t)macp->os_hdl)->os_usb_submitCmdOutUrb(macp->os_hdl, data, len, (void*)sendBuf);
#endif        
    }
    else if ( PipeID == HIF_USB_PIPE_TX )
    {
#ifdef ATH_WINHTC
        uint8_t *data = NULL;
        uint32_t len;

		data	= sendBuf->data;
		len		= sendBuf->len;


        status = OS_Usb_SubmitTxUrb(macp->os_hdl, data, len, (void*)sendBuf);
#else        
        status = ((osdev_t)macp->os_hdl)->os_usb_submitTxUrb(macp->os_hdl, data, len, (void*)sendBuf, &(((osdev_t)macp->os_hdl)->TxPipe));
#endif        
    }
    else if ( PipeID == HIF_USB_PIPE_HP_TX )
    {
#ifdef ATH_WINHTC
        uint8_t *data = NULL;
        uint32_t len;

		data	= sendBuf->data;
		len		= sendBuf->len;


        status = OS_Usb_SubmitTxUrb(macp->os_hdl, data, len, (void*)sendBuf);
#else        
        status = ((osdev_t)macp->os_hdl)->os_usb_submitTxUrb(macp->os_hdl, data, len, (void*)sendBuf, &(((osdev_t)macp->os_hdl)->HPTxPipe));        
#endif        
    }
    else
    {
        printk("Unknown pipe %d\n", PipeID);
        dev_kfree_skb_any(sendBuf);
		
    }

    return (status ? HIF_ERROR : HIF_OK);
}

uint16_t HIFGetFreeQueueNumber(HIF_HANDLE hHIF, uint8_t PipeID)
{
    HIF_DEVICE_USB  *macp = (HIF_DEVICE_USB *)hHIF;
    uint16_t      freeQNum = 0;
#ifdef ATH_WINHTC
    switch (PipeID)
    {
    case 1:
        freeQNum = OS_Usb_GetFreeTxBufferCnt(macp->os_hdl);
        break;
    case 4:
        freeQNum = OS_Usb_GetFreeCmdOutBufferCnt(macp->os_hdl);
        break;
    case 5:
        KASSERT(0, ("%s: endpoint 5 not support now !", __FUNCTION__));
        break;    
    default:
        printk("%s: not support pipe id = %d\n", __func__, PipeID);
        freeQNum = 0;
        break;
    }
#else
    switch (PipeID)
    {
    case 1:
        freeQNum = ((osdev_t)macp->os_hdl)->os_usb_getFreeTxBufferCnt(macp->os_hdl, &(((osdev_t)macp->os_hdl)->TxPipe));
        break;
    case 5:
        freeQNum = ((osdev_t)macp->os_hdl)->os_usb_getFreeTxBufferCnt(macp->os_hdl, &(((osdev_t)macp->os_hdl)->HPTxPipe));
        break;

    case 4:
        freeQNum = ((osdev_t)macp->os_hdl)->os_usb_getFreeCmdOutBufferCnt(macp->os_hdl);
        break;
    default:
        printk("%s: not support pipe id = %d\n", __func__, PipeID);
        freeQNum = 0;
        break;
    }
#endif
    return freeQNum;
}

uint16_t HIFGetMaxQueueNumber(HIF_HANDLE hHIF, uint8_t PipeID)
{
    HIF_DEVICE_USB  *macp = (HIF_DEVICE_USB *)hHIF;
    uint16_t      maxQNum = 0;

    switch (PipeID)
    {
    case 1:
#ifdef ATH_WINHTC        
        maxQNum = OS_Usb_GetMaxTxBufferCnt(macp->os_hdl);
#else        
        maxQNum = ((osdev_t)macp->os_hdl)->os_usb_getMaxTxBufferCnt(macp->os_hdl);
#endif        
        break;
    default:
        printk("%s: not support pipe id = %d\n", __func__, PipeID);
        maxQNum = 0;
        break;
    }

    return maxQNum;
}

a_uint16_t HIFGetQueueNumber(HIF_HANDLE hHIF, a_uint8_t PipeID)
{
    HIF_DEVICE_USB  *macp = (HIF_DEVICE_USB *)hHIF;
    a_uint16_t      QNum = 0;
#ifdef ATH_WINHTC
    switch (PipeID)
    {
    case 1:
        QNum = OS_Usb_GetTxBufferCnt(macp->os_hdl);
        break;
    case 4:
        QNum = 0;
        break;
    case 5:
        KASSERT(0, ("%s: endpoint 5 not support now !", __FUNCTION__));
        break;    
    default:
        adf_os_print("%s: not support pipe id = %d\n", __func__, PipeID);
        QNum = 0;
        break;
    }
#else
    switch (PipeID)
    {
    case 1:
        QNum = ((osdev_t)macp->os_hdl)->os_usb_getTxBufferCnt(macp->os_hdl, &(((osdev_t)macp->os_hdl)->TxPipe));
        break;
    case 5:
        QNum = ((osdev_t)macp->os_hdl)->os_usb_getTxBufferCnt(macp->os_hdl, &(((osdev_t)macp->os_hdl)->HPTxPipe));
        break;

    case 4:
        QNum = 0;
        break;
    default:
        adf_os_print("%s: not support pipe id = %d\n", __func__, PipeID);
        QNum = 0;
        break;
    }
#endif
    return QNum;
}

a_uint16_t HIFGetQueueThreshold(HIF_HANDLE hHIF, a_uint8_t PipeID)
{
    HIF_DEVICE_USB  *macp = (HIF_DEVICE_USB *)hHIF;
    a_uint16_t      QThreshold = 0;

    switch (PipeID)
    {
    case 1:
#ifdef ATH_WINHTC        
        QThreshold = OS_Usb_GetTxBufferThreshold(macp->os_hdl);
#else        
        QThreshold = ((osdev_t)macp->os_hdl)->os_usb_getTxBufferThreshold(macp->os_hdl);
#endif        
        break;
    default:
        adf_os_print("%s: not support pipe id = %d\n", __func__, PipeID);
        QThreshold = 0;
        break;
    }

    return QThreshold;
}

void HIFPostInit(HIF_HANDLE hHIF, void *hHTC, HTC_CALLBACKS *callbacks)
{
    HIF_DEVICE_USB *hif_dev = (HIF_DEVICE_USB *) hHIF;

    printk("HIFPostInit %p\n", hHTC);

    hif_dev->htc_handle = hHTC;
    hif_dev->htcCallbacks.Context = callbacks->Context;
    hif_dev->htcCallbacks.rxCompletionHandler = callbacks->rxCompletionHandler;
    hif_dev->htcCallbacks.txCompletionHandler = callbacks->txCompletionHandler;    
    hif_dev->htcCallbacks.usbStopHandler = callbacks->usbStopHandler;
    hif_dev->htcCallbacks.usbDeviceRemovedHandler = callbacks->usbDeviceRemovedHandler;
}

//spinlock need free when unload
static void *HIFInit(adf_os_handle_t os_hdl)
{
    HIF_DEVICE_USB *hif_dev;

    /* allocate memory for HIF_DEVICE */
    hif_dev = (HIF_DEVICE_USB *) adf_os_mem_alloc(os_hdl, sizeof(HIF_DEVICE_USB));
    if (hif_dev == NULL) {
        return NULL;
    }

    adf_os_mem_zero(hif_dev, sizeof(HIF_DEVICE_USB));

    hif_dev->os_hdl = os_hdl;
    
    return hif_dev;
}

void HIFStart(HIF_HANDLE hHIF)
{
    HIF_DEVICE_USB *hif_dev = (HIF_DEVICE_USB *) hHIF;
#ifdef ATH_WINHTC
    OS_Usb_SubmitRxUrb(hif_dev->os_hdl);

    OS_Usb_SubmitMsgInUrb(hif_dev->os_hdl);
#else
    ((osdev_t)hif_dev->os_hdl)->os_usb_initTxRxQ(hif_dev->os_hdl);
    ((osdev_t)hif_dev->os_hdl)->os_usb_submitRxUrb(hif_dev->os_hdl);
    ((osdev_t)hif_dev->os_hdl)->os_usb_submitMsgInUrb(hif_dev->os_hdl);
#endif    

}

void HIFStop(HIF_HANDLE hHIF)
{
}

/* undo what was done in HIFInit() */
void HIFShutDown(HIF_HANDLE hHIF)
{
    HIF_DEVICE_USB *hif_dev = (HIF_DEVICE_USB *)hHIF;

    /* need to handle packets queued in the HIF */

    /* free memory for hif device */
    adf_os_mem_free(hif_dev);
}

void HIFGetDefaultPipe(void *HIFHandle, uint8_t *ULPipe, uint8_t *DLPipe)
{
    *ULPipe = HIF_USB_PIPE_COMMAND;
    *DLPipe = HIF_USB_PIPE_INTERRUPT;
}

uint8_t HIFGetULPipeNum(void)
{
    return HIF_USB_MAX_TXPIPES;
}

uint8_t HIFGetDLPipeNum(void)
{
    return HIF_USB_MAX_RXPIPES;
}

void HIFEnableFwRcv(HIF_HANDLE hHIF)
{
    HIF_DEVICE_USB *hif_dev = (HIF_DEVICE_USB *) hHIF;
    ((struct _NIC_DEV *)hif_dev->os_hdl)->os_usb_enable_fwrcv(hif_dev->os_hdl);
}

int init_usb_hif(void);
int init_usb_hif(void)
{
    printk("USBHIF Version 1.xx Loaded...\n");
        return 0;
}

void exit_usb_hif(void);
void exit_usb_hif(void)
{

    printk("USB HIF UnLoaded...\n");

}

EXPORT_SYMBOL(HIF_UsbDataOut_Callback);
EXPORT_SYMBOL(HIF_UsbDataIn_Callback);
EXPORT_SYMBOL(HIF_UsbMsgIn_Callback);
EXPORT_SYMBOL(HIF_Usb_Stop_Callback);
EXPORT_SYMBOL(HIF_UsbCmdOut_Callback);

EXPORT_SYMBOL(HIF_USBDeviceInserted);
EXPORT_SYMBOL(HIF_USBDeviceDetached);

//EXPORT_SYMBOL(hif_module_install);
EXPORT_SYMBOL(HIF_register);
EXPORT_SYMBOL(HIFShutDown);
EXPORT_SYMBOL(HIFSend);
EXPORT_SYMBOL(HIFStart);
EXPORT_SYMBOL(HIFPostInit);
EXPORT_SYMBOL(HIFGetDefaultPipe);
EXPORT_SYMBOL(HIFGetULPipeNum);
EXPORT_SYMBOL(HIFGetDLPipeNum);
EXPORT_SYMBOL(HIFEnableFwRcv);
EXPORT_SYMBOL(HIFGetFreeQueueNumber);
EXPORT_SYMBOL(HIFGetMaxQueueNumber);
EXPORT_SYMBOL(HIFGetQueueNumber);
EXPORT_SYMBOL(HIFGetQueueThreshold);

module_init(init_usb_hif);
module_exit(exit_usb_hif);

MODULE_LICENSE("Dual BSD/GPL");
