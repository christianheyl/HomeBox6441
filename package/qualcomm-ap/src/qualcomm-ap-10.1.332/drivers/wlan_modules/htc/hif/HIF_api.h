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
 * @File: HIF_api.h
 * 
 * @Abstract: Host Interface api
 * 
 * @Notes:
 * 
 */

#ifndef _HIF_API_H
#define _HIF_API_H

#include <wbuf.h>

/* mailbox hw module configuration structure */
typedef struct _HIF_CONFIG {
    int dummy;
} HIF_CONFIG;

typedef struct _HIF_CALLBACK {
    /* callback when a buffer has be sent to the host*/
    void (*send_buf_done)(wbuf_t buf, void *context);
    /* callback when a receive message is received */
    void (*recv_buf)(wbuf_t hdr_buf, wbuf_t buf, void *context);
#ifdef MAGPIE_SINGLE_CPU_CASE
    /* callback when querying hardware WLAN queue depth -- only valid for single cpu case */
    a_uint32_t (*query_qdepth)(a_uint8_t epnum, void *context);
#endif
    /* context used for all callbacks */
    void *context;
} HIF_CALLBACK;

typedef void* hif_handle_t;

/* hardware API table structure (API descriptions below) */
struct hif_api {
    hif_handle_t (*_init)(HIF_CONFIG *pConfig);
            
    void (* _shutdown)(hif_handle_t);
    
    void (*_register_callback)(hif_handle_t, HIF_CALLBACK *);
    
    int  (*_get_total_credit_count)(hif_handle_t);
    
    void (*_start)(hif_handle_t);

    void (*_config_pipe)(hif_handle_t handle, int pipe, int creditCount);
    
    int  (*_send_buffer)(hif_handle_t handle, int pipe, wbuf_t buf);

    void (*_return_recv_buf)(hif_handle_t handle, int pipe, wbuf_t buf);                                 
    //void (*_set_recv_bufsz)(int pipe, int bufsz);
    //void (*_pause_recv)(int pipe);
    //void (*_resume_recv)(int pipe);
    int  (*_is_pipe_supported)(hif_handle_t handle, int pipe);
    
    int  (*_get_max_msg_len)(hif_handle_t handle, int pipe);
    
    void (*_isr_handler)(hif_handle_t handle);
    
#ifdef MAGPIE_SINGLE_CPU_CASE
    void (*_notify_target_inserted)(hif_handle_t);
    void (*_notify_target_detached)(hif_handle_t);
    void (*_notify_wlan_txcomp)(hif_handle_t, a_uint8_t);
#endif

        /* room to expand this table by another table */
    void *pReserved;
};

extern void hif_module_install(struct hif_api *apis);

#endif /* #ifndef _HIF_API_H */
