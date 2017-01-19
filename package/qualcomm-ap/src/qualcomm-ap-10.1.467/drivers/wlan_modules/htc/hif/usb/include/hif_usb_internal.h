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

#ifndef _HIF_USB_INTERNAL_H
#define _HIF_USB_INTERNAL_H

#include <adf_nbuf.h>

#include "a_types.h"
#include "athdefs.h"
#include "a_osapi.h"
#include "hif.h"

#ifndef ATH_WINHTC
#include <linux/usb.h>
#endif

#define HIF_USB_PIPE_COMMAND            4 /* 0 */
#define HIF_USB_PIPE_INTERRUPT          3 /* 1 */
#define HIF_USB_PIPE_TX                         1 /* 2 */
#define HIF_USB_PIPE_RX                         2 /* 3 */
#define HIF_USB_PIPE_HP_TX                      5
#define HIF_USB_PIPE_MP_TX                      6

#define HIF_USB_MAX_RXPIPES     2
#define HIF_USB_MAX_TXPIPES     4

struct _hif_device_usb;

typedef struct _hif_device_usb {
    void *htc_handle;
    HTC_CALLBACKS htcCallbacks;

    adf_os_handle_t         os_hdl;

} HIF_DEVICE_USB;

#ifdef ATH_WINHTC
/* USB Endpoint definition */
#define USB_WLAN_TX_PIPE                    1
#define USB_WLAN_RX_PIPE                    2
#define USB_REG_IN_PIPE                     3
#define USB_REG_OUT_PIPE                    4
#define USB_WLAN_HP_TX_PIPE                 5
#define USB_WLAN_MP_TX_PIPE                 6
#endif

#endif
