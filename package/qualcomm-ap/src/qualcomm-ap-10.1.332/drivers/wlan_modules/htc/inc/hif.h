/*
 * Copyright (c) 2010, Atheros Communications Inc.
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

#ifndef _HIF_H_
#define _HIF_H_

/**
 * @file hif.h
 * HostInterFace layer.
 * Abstracts interface-dependent transmit/receive functionality.
 */ 

#include <adf_os_types.h>
#include <adf_nbuf.h>
#include "athdefs.h"


typedef struct htc_drvreg_callbacks HTC_DRVREG_CALLBACKS;
typedef struct htc_callbacks HTC_CALLBACKS;
typedef void * HIF_HANDLE;
typedef void * hif_handle_t;


#define HIF_MAX_DEVICES             1

typedef enum hif_status {
    HIF_OK = 0,
    HIF_ERROR = -1
}hif_status_t;

/**
 * @brief List of registration callbacks - filled in by HTC.
 */
struct htc_drvreg_callbacks {
    hif_status_t (* deviceInsertedHandler)(HIF_HANDLE hHIF, adf_os_handle_t os_hdl);
    hif_status_t (* deviceRemovedHandler)(void *instance);
};

/**
 * @brief List of callbacks - filled in by HTC.
 */ 
struct htc_callbacks {
    void *Context;  /**< context meaningful to HTC */
    hif_status_t (* txCompletionHandler)(void *Context, adf_nbuf_t netbuf);
    hif_status_t (* rxCompletionHandler)(void *Context, adf_nbuf_t netbuf, a_uint8_t pipeID);
    hif_status_t (* usbStopHandler)(void *Context, a_uint8_t status);
    hif_status_t (* usbDeviceRemovedHandler)(void *Context);
#ifdef MAGPIE_SINGLE_CPU_CASE
    void (*wlanTxCompletionHandler)(void *Context, a_uint8_t epnum);
#endif
};

hif_status_t HIF_register(HTC_DRVREG_CALLBACKS *callbacks);

/** 
 * @brief: This API is used by the HTC layer to initialize the HIF layer and to
 * register different callback routines. Support for following events has
 * been captured - DSR, Read/Write completion, Device insertion/removal,
 * Device suspension/resumption/wakeup. In addition to this, the API is
 * also used to register the name and the revision of the chip. The latter
 * can be used to verify the revision of the chip read from the device
 * before reporting it to HTC.
 * @param[in]: callbacks - List of HTC callbacks
 * @param[out]:
 * @return: an opaque HIF handle
 */
//void *HIFInit(void *hHTC, HTC_CALLBACKS *callbacks);

void HIFPostInit(void *HIFHandle, void *hHTC, HTC_CALLBACKS *callbacks);

void HIFStart(HIF_HANDLE hHIF);

/**
 * @brief: Send a buffer to HIF for transmission to the target.
 * @param[in]: dev - HIF handle
 * @param[in]: pipeID - pipe to use
 * @param[in]: netbuf - buffer to send
 * @param[out]:
 * @return: Status of the send operation.
 */ 
hif_status_t HIFSend(void *dev, a_uint8_t PipeID, adf_nbuf_t hdr_buf, adf_nbuf_t buf);

/**
 * @brief: Shutdown the HIF layer.
 * @param[in]: HIFHandle - opaque HIF handle.
 * @param[out]:
 * @return:
 */ 
void HIFShutDown(void *HIFHandle);

a_uint16_t HIFGetFreeQueueNumber(HIF_HANDLE hHIF, a_uint8_t PipeID);
a_uint16_t HIFGetMaxQueueNumber(HIF_HANDLE hHIF, a_uint8_t PipeID);
a_uint16_t HIFGetQueueNumber(HIF_HANDLE hHIF, a_uint8_t PipeID);
a_uint16_t HIFGetQueueThreshold(HIF_HANDLE hHIF, a_uint8_t PipeID);

#ifdef MAGPIE_SINGLE_CPU_CASE
a_uint32_t HIFQueryQueueDepth(HIF_HANDLE hHIF, a_uint8_t epnum);
#endif

void HIFGetDefaultPipe(void *HIFHandle, a_uint8_t *ULPipe, a_uint8_t *DLPipe);

a_uint8_t HIFGetULPipeNum(void);
a_uint8_t HIFGetDLPipeNum(void);
void HIFEnableFwRcv(HIF_HANDLE hHIF);

#if defined MAGPIE_HIF_PCI

/* PCI interface */
void HIF_PCIDeviceInserted(osdev_t os_hdl);
void HIF_PCIDeviceDetached(osdev_t os_hdl);
A_STATUS fwd_module_init(osdev_t os_hdl);

/**
 * For boot /Firmware downloader
 */
a_status_t  hif_boot_start(HIF_HANDLE   hdl);
void        hif_boot_done(HIF_HANDLE    hdl);

#elif defined MAGPIE_HIF_GMAC

A_STATUS    fwd_module_init(void);
a_status_t  hif_boot_start(HIF_HANDLE   hdl);
void        hif_boot_done(HIF_HANDLE    hdl);

#else

/* USB interface */
A_STATUS HIF_USBDeviceInserted(adf_os_handle_t os_hdl);
void HIF_USBDeviceDetached(adf_os_handle_t os_hdl);

#endif /* MAGPIE_HIF_PCI */

#endif	/* !_HIF_H_ */
