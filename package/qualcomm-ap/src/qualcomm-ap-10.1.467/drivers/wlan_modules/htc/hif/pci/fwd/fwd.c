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

#include <adf_os_stdtypes.h>
#include <adf_os_types.h>
//#include <adf_net.h>
#include <adf_os_mem.h>
#include <adf_os_util.h>
#include <adf_os_lock.h>
#include <adf_os_timer.h>
#include <adf_os_time.h>
//#include <asf_bitmap.h>
#include <adf_os_module.h>
#include <adf_os_stdtypes.h>
#include <adf_os_atomic.h>
#include <adf_nbuf.h>
#include <adf_os_io.h>
//#include <asf_queue.h>

#include "fwd.h"

a_uint32_t fw_exec_addr = 0x00906000;
a_uint32_t fw_target_addr = 0x00501000;

extern const unsigned char zcFwImage[]; 
extern const unsigned long zcFwImageSize;

/**
 * Prototypes
 */ 
a_status_t      fwd_send_next(fwd_softc_t *sc);
static void     fwd_start_upload(fwd_softc_t *sc);

/********************************************************************/

void 
fwd_timer_expire(void *arg)
{
  fwd_softc_t *sc = arg;

  sc->chunk_retries ++;
  adf_os_print("Retry ");

  if (sc->chunk_retries >= FWD_MAX_TRIES) {
      adf_os_print("\nFWD:Failed ...uploaded %#x bytes\n", sc->offset);
      HIFShutDown(sc->hif_handle);
  }
  else
      fwd_send_next(sc);
}
  
#define fwd_is_first(_sc)   ((_sc)->offset == 0)
#define fwd_is_last(_sc)    (((_sc)->size - (_sc)->offset) <= FWD_MAX_CHUNK)

static a_uint32_t
fwd_chunk_len(fwd_softc_t *sc)
{
    a_uint32_t left, max_chunk = FWD_MAX_CHUNK;
    
    left     =   sc->size - sc->offset;
    return(adf_os_min(left, max_chunk));
}

a_status_t
fwd_send_next(fwd_softc_t *sc) 
{
    a_uint32_t len, alloclen; 
    adf_nbuf_t nbuf;
    fwd_cmd_t  *h;
    a_uint8_t  *pld;
    a_uint32_t  target_jmp_loc;

    len      =   fwd_chunk_len(sc);
    alloclen =   sizeof(fwd_cmd_t) + len;

    if (fwd_is_first(sc) || fwd_is_last(sc))
        alloclen += 4;

    nbuf = adf_nbuf_alloc(sc->hif_handle, alloclen + 20, 20, 0);

    if (!nbuf) {
        adf_os_print("FWD: packet allocation failed. \n");
        return A_STATUS_ENOMEM;
    }

    h            =  (fwd_cmd_t *)adf_nbuf_put_tail(nbuf, alloclen);        

    h->more_data =  adf_os_htons(!fwd_is_last(sc));
    h->len       =  adf_os_htons(len);
    h->offset    =  adf_os_htonl(sc->offset);

    pld          =  (a_uint8_t *)(h + 1);

    if (fwd_is_first(sc)) {
        *(a_uint32_t *)pld  = adf_os_htonl(sc->target_upload_addr);
                       pld += 4;
    }

    adf_os_mem_copy(pld, &sc->image[sc->offset], len);

    if(h->more_data == 0) {
        target_jmp_loc = adf_os_htonl(fw_exec_addr);
        adf_os_mem_copy(pld+len, (a_uint8_t *)&target_jmp_loc, 4);
    }

    HIFSend(sc->hif_handle, sc->tx_pipe, NULL, nbuf);
    adf_os_timer_start(&sc->tmr, FWD_TIMEOUT_MSECS);

    return A_STATUS_OK;
}

A_STATUS
fwd_txdone(void *context, adf_nbuf_t nbuf)
{
  adf_nbuf_free(nbuf);  
  return A_STATUS_OK;
}

A_STATUS
fwd_recv(void *context, adf_nbuf_t nbuf, a_uint8_t epid)
{
    fwd_softc_t *sc = (fwd_softc_t *)context;
    a_uint8_t *pld;
    a_uint32_t plen, rsp, offset;
    fwd_rsp_t *h; 

    adf_nbuf_peek_header(nbuf, &pld, &plen);

    h       = (fwd_rsp_t *)pld;
    rsp     = adf_os_ntohl(h->rsp);
    offset  = adf_os_ntohl(h->offset);
    
    adf_os_timer_cancel(&sc->tmr);

    switch(rsp) {

    case FWD_RSP_ACK:
        if (offset == sc->offset) {
            // adf_os_printk("ACK for %#x\n", offset);
            adf_os_print(".");
                sc->offset += fwd_chunk_len(sc);
                fwd_send_next(sc);
        }
        break;
            
    case FWD_RSP_SUCCESS:
        adf_os_print("done!\n");

        hif_boot_done(sc->hif_handle);
        break;

    case FWD_RSP_FAILED:
        if (sc->ntries < FWD_MAX_TRIES) 
            fwd_start_upload(sc);
        else
                adf_os_print("FWD: Error: Max retries exceeded\n");
        break;
        
    default:
            adf_os_assert(0);
    }

    adf_nbuf_free(nbuf);

    return A_OK;
}


A_STATUS
//fwd_device_removed(void *ctx, a_uint8_t surpriseRemoved)
fwd_device_removed(void *ctx)
{
  adf_os_mem_free(ctx);
  return A_OK;
}

static void 
fwd_start_upload(fwd_softc_t *sc)
{
    sc->ntries ++;
    sc->offset  = 0;
    fwd_send_next(sc);
}

A_STATUS
fwd_device_inserted(HIF_HANDLE hif, adf_os_handle_t os_hdl)
{
    fwd_softc_t    *sc;
    HTC_CALLBACKS   fwd_cb;

    sc = adf_os_mem_alloc(os_hdl, sizeof(fwd_softc_t));
    if (!sc) {
      adf_os_print("FWD: No memory for fwd context\n");
      return -1;
    }

//    adf_os_print("fwd  : ctx allocation done = %p\n",sc);

    adf_os_mem_set(sc, 0, sizeof(fwd_softc_t));

    sc->hif_handle = hif;

    adf_os_timer_init(NULL, &sc->tmr, fwd_timer_expire, sc);
    HIFGetDefaultPipe(hif, &sc->rx_pipe, &sc->tx_pipe);

    sc->image                   = (a_uint8_t *)zcFwImage;
    sc->size                    = zcFwImageSize;
    sc->target_upload_addr      = fw_target_addr;

    fwd_cb.Context              = sc;
    fwd_cb.rxCompletionHandler  = fwd_recv;
    fwd_cb.txCompletionHandler  = fwd_txdone;

    sc->hif_handle              = hif;

adf_os_print("%s, hif: 0x%08x\n", __FUNCTION__, (a_uint32_t)hif);

    HIFPostInit(hif, NULL, &fwd_cb);

adf_os_print("%s, hif: 0x%08x\n", __FUNCTION__, (a_uint32_t)hif);

    hif_boot_start(hif);

    adf_os_print("Downloading\t");

    fwd_start_upload(sc);

    return A_STATUS_OK;
}

A_STATUS 
fwd_module_init(adf_os_handle_t os_hdl)
{
    HTC_DRVREG_CALLBACKS cb;

    adf_os_print("FWD:loaded\n");

    cb.deviceInsertedHandler = fwd_device_inserted;
    cb.deviceRemovedHandler  = fwd_device_removed;
  
    HIF_register(&cb);
  
    return A_OK;
}

adf_os_export_symbol(fwd_module_init);

//void
//fwd_module_exit(void) 
//{
//    adf_os_print("FWD:unloaded\n");
//}

//adf_os_module_dep(fwd, hif_pci);
//
//adf_os_export_symbol(fwd_recv);
//
//adf_os_declare_param(fw_exec_addr, ADF_OS_PARAM_TYPE_UINT32);
//adf_os_declare_param(fw_target_addr, ADF_OS_PARAM_TYPE_UINT32);
//
//
//adf_os_virt_module_name(fwd);
//adf_os_virt_module_init(fwd_module_init);
//adf_os_virt_module_exit(fwd_module_exit);
