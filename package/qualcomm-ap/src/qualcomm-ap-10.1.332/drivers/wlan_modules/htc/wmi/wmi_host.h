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

#ifndef _WMI_HOST_H_
#define _WMI_HOST_H_


#ifdef WMI_RETRY
#include <adf_os_timer.h>
#endif

#define WMI_EVT_QUEUE_SIZE  256

#ifdef NBUF_PREALLOC_POOL
#define WMI_CMD_MAXBYTES   512 /* Verify this */
#endif

#ifdef WMI_RETRY
typedef void (*wmi_rsp_callback_t)(void *, WMI_COMMAND_ID, a_uint16_t, adf_nbuf_t, a_uint8_t *, a_uint32_t, a_uint16_t);
#else
typedef void (*wmi_rsp_callback_t)(void *, WMI_COMMAND_ID, a_uint16_t, adf_nbuf_t, a_uint8_t *, a_uint32_t);
#endif

struct wmiEventQ
{
    WMI_COMMAND_ID     evtid;
    adf_nbuf_t         buf;
    a_uint8_t            *netdata;
    a_uint32_t           netlen;
};

struct wmi_t {
    void                     *devt;
    adf_os_handle_t          os_hdl;
    HTC_HANDLE               HtcHandle;
    adf_os_spinlock_t        wmi_lock;
    unsigned long               wmi_flags;
#ifdef MAGPIE_SINGLE_CPU_CASE
    adf_os_spinlock_t        wmi_op_lock;
    a_uint32_t               wmi_op_flags;
#else
    adf_os_mutex_t           wmi_op_mutex;
    adf_os_mutex_t           wmi_cmd_mutex;
#endif
    adf_os_spinlock_t        wmi_op_lock_bh;
    HTC_ENDPOINT_ID          wmi_endpoint_id;
    wmi_event_callback_t     wmi_cb;
    wmi_stop_callback_t     wmi_stop_cb;
    a_uint16_t               txSeqId;
    a_uint16_t               wmi_stop_flag;
    a_uint8_t                wmiCmdFailCnt;
    a_uint8_t                wmi_usb_stop_flag;

#ifdef MAGPIE_SINGLE_CPU_CASE
#else
#ifdef __linux__
    htc_spin_t               spin;
#else
    adf_os_waitq_t           wq;
#endif
#endif
    a_uint8_t                *cmd_rsp_buf;
    a_uint32_t               cmd_rsp_len;

    a_uint32_t               wmi_in_progress;
    adf_os_bh_t              wmi_evt_bh;

    /* event queue related */
    a_uint16_t               evtHead;
    a_uint16_t               evtTail;
    struct wmiEventQ         wmi_evt_queue[WMI_EVT_QUEUE_SIZE];
#ifdef NBUF_PREALLOC_POOL
    adf_nbuf_t              wmi_cmd_nbuf;
#endif    
#ifdef WMI_RETRY
    adf_os_timer_t          wmi_retry_timer_instance;
    adf_nbuf_t              wmi_retryBuf;
    a_uint16_t              wmi_retrycmdLen;
    a_uint16_t              wmi_retrycnt;
    a_uint16_t              LastSeqId;
    WMI_COMMAND_ID          wmi_last_sent_cmd;
#endif

};

#define LOCK_WMI(w)     adf_os_spin_lock_irq(&(w)->wmi_lock, (w)->wmi_flags);
#define UNLOCK_WMI(w)   adf_os_spin_unlock_irq(&(w)->wmi_lock, (w)->wmi_flags);

#endif /* !_WMI_HOST_H_ */
