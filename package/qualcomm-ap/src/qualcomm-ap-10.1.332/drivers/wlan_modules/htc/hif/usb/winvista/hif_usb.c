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

#include <adf_os_types.h>
//#include <adf_os_dma.h>
//#include <adf_os_timer.h>
//#include <adf_os_time.h>
#include <adf_os_lock.h>
#include <adf_os_io.h>
#include <adf_os_mem.h>
#include <adf_os_module.h>
#include <adf_nbuf.h>

#include "a_types.h"
#include "athdefs.h"
#include "a_osapi.h"
#include "hif.h"

#ifndef ATH_WINHTC
#include <linux/usb.h>
#endif

#include "hif_usb_internal.h"

static void *HIFInit(adf_os_handle_t os_hdl);

HTC_DRVREG_CALLBACKS htcDrvRegCallbacks;

void HIF_UsbDataOut_Callback(void *phifhdl, adf_nbuf_t context)
{
    HIF_DEVICE_USB  *hHIF = (HIF_DEVICE_USB *)phifhdl;

    hHIF->htcCallbacks.txCompletionHandler(hHIF->htcCallbacks.Context, context);
}

void HIF_UsbDataIn_Callback(void *phifhdl, adf_nbuf_t buf)
{
    HIF_DEVICE_USB  *hHIF = (HIF_DEVICE_USB *)phifhdl;

    hHIF->htcCallbacks.rxCompletionHandler(hHIF->htcCallbacks.Context, buf, USB_WLAN_RX_PIPE);
}

void HIF_UsbMsgIn_Callback(void *phifhdl, adf_nbuf_t buf)
{
    HIF_DEVICE_USB  *hHIF = (HIF_DEVICE_USB *)phifhdl;

    hHIF->htcCallbacks.rxCompletionHandler(hHIF->htcCallbacks.Context, buf, USB_REG_IN_PIPE);
}

void HIF_Usb_Stop_Callback(void *phifhdl, void *pwmihdl, a_uint8_t usb_status)
{
    HIF_DEVICE_USB  *hHIF = (HIF_DEVICE_USB *)phifhdl;
    hHIF->htcCallbacks.usbStopHandler(pwmihdl, usb_status);
}

void HIF_UsbCmdOut_Callback(void *phifhdl, adf_nbuf_t context)
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

    adf_os_print("HIF_USBDeviceDetached\n");
    
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

A_STATUS HIF_register(HTC_DRVREG_CALLBACKS *callbacks)
{
    htcDrvRegCallbacks.deviceInsertedHandler = callbacks->deviceInsertedHandler;
    htcDrvRegCallbacks.deviceRemovedHandler = callbacks->deviceRemovedHandler;

    return A_OK;
}

A_STATUS HIFSend(HIF_HANDLE hHIF, a_uint8_t PipeID, adf_nbuf_t hdr_buf, adf_nbuf_t buf)
{
    A_STATUS status = A_OK;
    HIF_DEVICE_USB *macp = (HIF_DEVICE_USB *)hHIF;
    adf_nbuf_t sendBuf;
    a_uint8_t *data = NULL;
    a_uint32_t len = 0;

    /* If necessary, link hdr_buf & buf */
    if (hdr_buf != NULL) 
    {
        adf_nbuf_cat(hdr_buf, buf);
        sendBuf = hdr_buf;
    }
    else
    {
        sendBuf = buf;
    }

    adf_nbuf_peek_header(sendBuf, &data, &len);
    
    if ( PipeID == HIF_USB_PIPE_COMMAND ) 
    {
#ifdef ATH_WINHTC
        a_uint8_t *data = NULL;
        a_uint32_t len;
        adf_nbuf_peek_header(sendBuf, &data, &len);

        status = OS_Usb_SubmitCmdOutUrb(macp->os_hdl, data, len, (void*)sendBuf);        
#else        
        status = ((osdev_t)macp->os_hdl)->os_usb_submitCmdOutUrb(macp->os_hdl, data, len, (void*)sendBuf);
#endif        
    }
    else if ( PipeID == HIF_USB_PIPE_TX )
    {
#ifdef ATH_WINHTC
        a_uint8_t *data = NULL;
        a_uint32_t len;

        adf_nbuf_peek_header(sendBuf, &data, &len);

        status = OS_Usb_SubmitTxUrb(macp->os_hdl, data, len, (void*)sendBuf);
#else        
        status = ((osdev_t)macp->os_hdl)->os_usb_submitTxUrb(macp->os_hdl, data, len, (void*)sendBuf, &(((osdev_t)macp->os_hdl)->TxPipe));
#endif        
    }
    else if ( PipeID == HIF_USB_PIPE_HP_TX )
    {
#ifdef ATH_WINHTC
        a_uint8_t *data = NULL;
        a_uint32_t len;

        adf_nbuf_peek_header(sendBuf, &data, &len);

        status = OS_Usb_SubmitTxUrb(macp->os_hdl, data, len, (void*)sendBuf);
#else        
        status = ((osdev_t)macp->os_hdl)->os_usb_submitTxUrb(macp->os_hdl, data, len, (void*)sendBuf, &(((osdev_t)macp->os_hdl)->HPTxPipe));        
#endif        
    }
    else
    {
        adf_os_print("Unknown pipe %d\n", PipeID);
        adf_nbuf_free(sendBuf);
    }

    return status;
}

a_uint16_t HIFGetFreeQueueNumber(HIF_HANDLE hHIF, a_uint8_t PipeID)
{
    HIF_DEVICE_USB  *macp = (HIF_DEVICE_USB *)hHIF;
    a_uint16_t      freeQNum = 0;
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
        adf_os_print("%s: not support pipe id = %d\n", __func__, PipeID);
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
        adf_os_print("%s: not support pipe id = %d\n", __func__, PipeID);
        freeQNum = 0;
        break;
    }
#endif
    return freeQNum;
}

a_uint16_t HIFGetMaxQueueNumber(HIF_HANDLE hHIF, a_uint8_t PipeID)
{
    HIF_DEVICE_USB  *macp = (HIF_DEVICE_USB *)hHIF;
    a_uint16_t      maxQNum = 0;

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
        adf_os_print("%s: not support pipe id = %d\n", __func__, PipeID);
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

    adf_os_print("HIFPostInit %p\n", hHTC);

    hif_dev->htc_handle = hHTC;
    hif_dev->htcCallbacks.Context = callbacks->Context;
    hif_dev->htcCallbacks.rxCompletionHandler = callbacks->rxCompletionHandler;
    hif_dev->htcCallbacks.txCompletionHandler = callbacks->txCompletionHandler;    
    hif_dev->htcCallbacks.usbStopHandler = callbacks->usbStopHandler;
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

void HIFGetDefaultPipe(void *HIFHandle, a_uint8_t *ULPipe, a_uint8_t *DLPipe)
{
    *ULPipe = HIF_USB_PIPE_COMMAND;
    *DLPipe = HIF_USB_PIPE_INTERRUPT;
}

a_uint8_t HIFGetULPipeNum(void)
{
    return HIF_USB_MAX_TXPIPES;
}

a_uint8_t HIFGetDLPipeNum(void)
{
    return HIF_USB_MAX_RXPIPES;
}

void HIFEnableFwRcv(HIF_HANDLE hHIF)
{
    HIF_DEVICE_USB *macp = (HIF_DEVICE_USB *)hHIF;

    OS_Usb_EnableFwRcv(macp->os_hdl);
}

int init_usb_hif(void);
int init_usb_hif(void)
{
    adf_os_print("USBHIF Version 1.xx Loaded...\n");
        return 0;
}

void exit_usb_hif(void);
void exit_usb_hif(void)
{

    adf_os_print("USB HIF UnLoaded...\n");

}

adf_os_export_symbol(HIF_UsbDataOut_Callback);
adf_os_export_symbol(HIF_UsbDataIn_Callback);
adf_os_export_symbol(HIF_UsbMsgIn_Callback);
adf_os_export_symbol(HIF_Usb_Stop_Callback);
adf_os_export_symbol(HIF_UsbCmdOut_Callback);

adf_os_export_symbol(HIF_USBDeviceInserted);
adf_os_export_symbol(HIF_USBDeviceDetached);

//adf_os_export_symbol(hif_module_install);
adf_os_export_symbol(HIF_register);
adf_os_export_symbol(HIFShutDown);
adf_os_export_symbol(HIFSend);
adf_os_export_symbol(HIFStart);
adf_os_export_symbol(HIFPostInit);
adf_os_export_symbol(HIFGetDefaultPipe);
adf_os_export_symbol(HIFGetULPipeNum);
adf_os_export_symbol(HIFGetDLPipeNum);
adf_os_export_symbol(HIFEnableFwRcv);
adf_os_export_symbol(HIFGetFreeQueueNumber);
adf_os_export_symbol(HIFGetMaxQueueNumber);
adf_os_export_symbol(HIFGetQueueNumber);
adf_os_export_symbol(HIFGetQueueThreshold);

adf_os_virt_module_init(init_usb_hif);
adf_os_virt_module_exit(exit_usb_hif);
adf_os_module_dep(usb_hif, adf_net);
adf_os_virt_module_name(usb_hif);
