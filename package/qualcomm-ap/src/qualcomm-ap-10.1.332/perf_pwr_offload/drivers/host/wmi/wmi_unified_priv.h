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

/*
 * This file contains the API definitions for the Unified Wireless Module Interface (WMI).
 */

#include <osdep.h>
#include <wbuf.h>
#include "a_types.h"
#include "wmi.h"
#include "wmi_unified.h"

#define WMI_UNIFIED_MAX_EVENT 0x1000
typedef wbuf_t wmi_buf_t;

struct wmi_unified {
    ol_scn_t scn_handle; /* handle to device */
    osdev_t  osdev;      /* handle to use OS-independent services */
    HTC_ENDPOINT_ID wmi_endpoint_id;
    wmi_unified_event_handler event_handler[WMI_UNIFIED_MAX_EVENT];
    void *event_handler_cookie[WMI_UNIFIED_MAX_EVENT];
    void *htc_handle;
};

