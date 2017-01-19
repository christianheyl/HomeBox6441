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

//#include <adf_os_types.h>
//#include <adf_os_dma.h>
//#include <adf_os_timer.h>
//#include <adf_os_time.h>
//#include <adf_os_lock.h>
//#include <adf_os_io.h>
//#include <adf_os_mem.h>
//#include <adf_os_module.h>

#include "hif_internal.h"

#include "HIF_api.h"  /* for exporting target side inprocess-HIF symbols */

#define MAX_HIF_SUPPORTED 5

typedef struct _hst_tgt_hif_mapping {
    A_BOOL valid;
    void *tgt_hif_ctx;
    void *hst_hif_ctx;
} HST_TGT_HIF_MAPPING;

HST_TGT_HIF_MAPPING g_hst_tgt_hif_map[MAX_HIF_SUPPORTED];

typedef struct _hif_device_inproc {
    void *htc_handle;
    HTC_CALLBACKS htcCallbacks;
    void *target_handle;
} HIF_DEVICE_INPROC;

void HIFSetTargetHandle(HIF_HANDLE hHIF, void *targetHandle);
void HIFRecvBufFromTarget(HIF_HANDLE hHIF, wbuf_t netbuf, a_uint8_t pipeID);
void HIFIndicateTxComplete(HIF_HANDLE hHIF, wbuf_t netbuf);
void HIF_DeviceInserted(void *tgt_hif_ctx);
void HIF_DeviceDetached(void *tgt_hif_ctx);
void HIF_WlanTxCompleted(HIF_HANDLE hHIF, a_uint8_t epnum);

extern int HIFinproc_recv_buffer_from_host(void* handle, int pipe, wbuf_t hdr_buf, wbuf_t buf);
extern void HIFinproc_set_host_hif_handle(void *handle, void *hostHandle);
extern a_uint32_t HIFinproc_query_queue_depth(hif_handle_t handle, a_uint8_t epnum);

HTC_DRVREG_CALLBACKS htcDrvRegCallbacks;

void HIF_DeviceInserted(void *tgt_hif_ctx)
{
    a_uint8_t i;

printk(KERN_ERR "##### in %s #####\n", __FUNCTION__);

    for (i = 0; i < MAX_HIF_SUPPORTED; i++) {
        if (g_hst_tgt_hif_map[i].valid == FALSE) {
            g_hst_tgt_hif_map[i].valid = TRUE;
            g_hst_tgt_hif_map[i].tgt_hif_ctx = tgt_hif_ctx;    /* record the target hif handle */
            break;
        }
    }

    if (i == MAX_HIF_SUPPORTED) {
        printk("Exceeds the maximum number of targets!\n");
        return;
    }

    /* Inform HTC */
    if (htcDrvRegCallbacks.deviceInsertedHandler) {
printk(KERN_ERR "deviceInsertedHandler != NULL\n");
        htcDrvRegCallbacks.deviceInsertedHandler();
    }
}

void HIF_DeviceDetached(void *tgt_hif_ctx)
{
    a_uint8_t i;
    HIF_DEVICE_INPROC *hif_dev;

    for (i = 0; i < MAX_HIF_SUPPORTED; i++) {
        if (g_hst_tgt_hif_map[i].valid == TRUE && g_hst_tgt_hif_map[i].tgt_hif_ctx == tgt_hif_ctx) {
            break;
        }
    }

    hif_dev = (HIF_DEVICE_INPROC *)g_hst_tgt_hif_map[i].hst_hif_ctx;

    /* Inform HTC */
    if (htcDrvRegCallbacks.deviceRemovedHandler) {
        htcDrvRegCallbacks.deviceRemovedHandler(hif_dev->htc_handle);
    }
}

void HIF_WlanTxCompleted(HIF_HANDLE hHIF, a_uint8_t epnum)
{
    HIF_DEVICE_INPROC *hif_dev = (HIF_DEVICE_INPROC *)hHIF;

    hif_dev->htcCallbacks.wlanTxCompletionHandler(hif_dev->htcCallbacks.Context, epnum);
}

hif_status_t HIF_register(HTC_DRVREG_CALLBACKS *callbacks)
{
    htcDrvRegCallbacks.deviceInsertedHandler = callbacks->deviceInsertedHandler;
    htcDrvRegCallbacks.deviceRemovedHandler = callbacks->deviceRemovedHandler;

    return HIF_OK;
}

hif_status_t HIFSend(HIF_HANDLE hHIF, a_uint8_t PipeID, wbuf_t hdr_buf, wbuf_t buf)
{
    hif_status_t status = HIF_OK;
    HIF_DEVICE_INPROC *hif_dev = (HIF_DEVICE_INPROC *)hHIF;

    /* If necessary, link hdr_buf & buf */
    if (hdr_buf != NULL) {
        //adf_nbuf_cat(hdr_buf, buf);
        wbuf_cat(hdr_buf, buf);
        HIFinproc_recv_buffer_from_host(hif_dev->target_handle, PipeID, NULL, hdr_buf);
    }
    else {
        HIFinproc_recv_buffer_from_host(hif_dev->target_handle, PipeID, NULL, buf);
    }

    //HIFinproc_recv_buffer_from_host(hif_dev->target_handle, PipeID, hdr_buf, buf);
    
    return status;
}

void *HIFInit(void *hHTC, HTC_CALLBACKS *callbacks)
{
    a_uint8_t i;
    HIF_DEVICE_INPROC *hif_dev;

    /* allocate memory for HIF_DEVICE */
    //hif_dev = (HIF_DEVICE_INPROC *)adf_os_mem_alloc(sizeof(HIF_DEVICE_INPROC));
    hif_dev = (HIF_DEVICE_INPROC *) kzalloc(sizeof(HIF_DEVICE_INPROC), GFP_KERNEL);

    if (hif_dev == NULL) {
        return NULL;
    }

    hif_dev->htc_handle = hHTC;
    hif_dev->htcCallbacks.Context = callbacks->Context;
    hif_dev->htcCallbacks.rxCompletionHandler = callbacks->rxCompletionHandler;
    hif_dev->htcCallbacks.txCompletionHandler = callbacks->txCompletionHandler;
    hif_dev->htcCallbacks.wlanTxCompletionHandler = callbacks->wlanTxCompletionHandler;

    for (i = 0; i < MAX_HIF_SUPPORTED; i++) {
        if (g_hst_tgt_hif_map[i].valid == TRUE && g_hst_tgt_hif_map[i].hst_hif_ctx == NULL) {
            g_hst_tgt_hif_map[i].hst_hif_ctx = hif_dev;
            break;
        }
    }

    /* record the target-side hif handle */
    hif_dev->target_handle = g_hst_tgt_hif_map[i].tgt_hif_ctx;

    /* set the host-side hif handle to target-side hif */
    HIFinproc_set_host_hif_handle(hif_dev->target_handle, hif_dev);

    return hif_dev;
}

/* undo what was done in HIFInit() */
void HIFShutDown(HIF_HANDLE hHIF)
{
    a_uint8_t i;
    HIF_DEVICE_INPROC *hif_dev = (HIF_DEVICE_INPROC *)hHIF;

    for (i = 0; i < MAX_HIF_SUPPORTED; i++) {
        if (g_hst_tgt_hif_map[i].valid == TRUE && g_hst_tgt_hif_map[i].hst_hif_ctx == hHIF) {
            g_hst_tgt_hif_map[i].hst_hif_ctx = NULL;
            g_hst_tgt_hif_map[i].tgt_hif_ctx = NULL;
            g_hst_tgt_hif_map[i].valid = FALSE;
            break;
        }
    }
    /* need to handle packets queued in the HIF */

    /* free memory for hif device */
    //adf_os_mem_free(hif_dev);
    kfree(hif_dev);
}

void HIFSetTargetHandle(HIF_HANDLE hHIF, void *targetHandle)
{
    HIF_DEVICE_INPROC *hif_dev = (HIF_DEVICE_INPROC *)hHIF;
    
    hif_dev->target_handle = targetHandle;
}

void HIFRecvBufFromTarget(HIF_HANDLE hHIF, wbuf_t netbuf, a_uint8_t pipeID)
{
    HIF_DEVICE_INPROC *hif_dev = (HIF_DEVICE_INPROC *)hHIF;
    
    hif_dev->htcCallbacks.rxCompletionHandler(hif_dev->htcCallbacks.Context, netbuf, pipeID);
}

void HIFIndicateTxComplete(HIF_HANDLE hHIF, wbuf_t netbuf)
{
    HIF_DEVICE_INPROC *hif_dev = (HIF_DEVICE_INPROC *)hHIF;
    hif_dev->htcCallbacks.txCompletionHandler(hif_dev->htcCallbacks.Context, netbuf);
}

a_uint32_t HIFQueryQueueDepth(HIF_HANDLE hHIF, a_uint8_t epnum)
{
    HIF_DEVICE_INPROC *hif_dev = (HIF_DEVICE_INPROC *)hHIF;
    return HIFinproc_query_queue_depth(hif_dev->target_handle, epnum);
}

int init_inproc_hif(void);
int init_inproc_hif(void)
{
    printk("In-process HIF Version 1.xx Loaded...\n");
    return 0;
}

void exit_inproc_hif(void);
void exit_inproc_hif(void)
{
    printk("In-process HIF UnLoaded...\n");
}

EXPORT_SYMBOL(hif_module_install);
EXPORT_SYMBOL(HIF_register);
EXPORT_SYMBOL(HIFShutDown);
EXPORT_SYMBOL(HIFSend);
EXPORT_SYMBOL(HIFInit);
EXPORT_SYMBOL(HIFQueryQueueDepth);
EXPORT_SYMBOL(HIFIndicateTxComplete);
EXPORT_SYMBOL(HIF_DeviceInserted);
EXPORT_SYMBOL(HIF_WlanTxCompleted);
EXPORT_SYMBOL(HIFRecvBufFromTarget);

//adf_os_virt_module_init(init_inproc_hif);
//adf_os_virt_module_exit(exit_inproc_hif);
//adf_os_module_dep(inproc_hif, adf_net);
//adf_os_virt_module_name(inproc_hif);
