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

#ifndef _WMI_HOST_API_H_
#define _WMI_HOST_API_H_

/**
 * @file wmi_host_api.h 
 * WirelessModuleInterface.
 * This API provides a mechanism to dispatch commands and receive events
 * at the wireless management layer.
 */ 

/**
 * @brief Callbacks filled in by the WMI user.
 */ 
typedef void (*wmi_event_callback_t)(void *, WMI_EVENT_ID, a_uint8_t *, a_uint32_t);
typedef void (*wmi_stop_callback_t)(void *);

typedef struct avt_s
{
    a_uint32_t addr;
    a_uint32_t value;
} avt_t;

/**
 * @brief WMI Initialization
 * @param[in]: devt - user context
 * @param[in]: cb - callbacks for user
 * @param[out]:
 * @return: opaque WMI handle.
 */ 
void * wmi_init(void *devt, adf_os_handle_t os_hdl, wmi_event_callback_t cb, wmi_stop_callback_t stop_cb);

/**
 * @brief Shuts WMI down
 * @param[in]: wmi_handle - opaque WMI handle returned by wmi_init()
 * @param[out]:
 * @return: 
 * @see: wmi_init()
 */ 
void wmi_shutdown(void *wmi_handle);

/**
 * @brief Connects to the WMI service using HTC.
 * @param[in]: hHTC - Opaque htc handle
 * @param[in]: hWMI - Opaque WMI handle
 * @param[out]:
 * @return: Status of connect operation
 * @see: wmi_init()
 */ 
A_STATUS wmi_connect(HTC_HANDLE hHTC, void *hWMI, HTC_ENDPOINT_ID *pWmi_ctrl_epid);

/**
 * @brief Start the WMI service, init WMI queue
 * @param[in]: hWMI - Opaque WMI handle
 * @param[out]:
 */ 
void wmi_start(void *wmi_handle);

/**
 * @brief Stop the WMI service
 * @param[in]: hWMI - Opaque WMI handle
 * @param[out]:
 */ 
void wmi_stop(void *wmi_handle);

/* WMI command helper function */
A_STATUS __ahdecl
wmi_cmd(void *wmi_handle, WMI_COMMAND_ID cmdId, a_uint8_t *pCmdBuffer, a_uint32_t length, a_uint8_t *pRspBuffer, a_uint32_t rspLen, a_uint32_t timeout);

void __ahdecl
wmi_reg_write_single(void *wmi_handle, u_int reg, u_int32_t val);
void __ahdecl
wmi_reg_write_delay(void *wmi_handle, u_int reg, u_int32_t val);
void __ahdecl
wmi_reg_write_flush(void *wmi_handle);
void __ahdecl
wmi_usb_stop(void *wmi_handle,a_uint8_t usb_status);
void __ahdecl
wmi_reg_rmw(void *wmi_handle, u_int reg, u_int32_t clr, u_int32_t set);
#endif /* _WMI_API_H_ */

