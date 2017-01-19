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
#include <adf_os_util.h>
#include <adf_os_module.h>
#include <adf_os_lock.h>
#include <adf_nbuf.h>
#include <adf_net.h>
#include <adf_net_types.h>

#include <athdefs.h>
#include <a_types.h>
#include <hif.h>


typedef struct __test_sc{
    adf_net_handle_t        net_handle;
    a_uint8_t               tx_pipe;
    a_uint8_t               rx_pipe;
    a_uint32_t              next_idx;/*next softc index*/
    adf_os_spinlock_t       lock_irq;
    adf_net_dev_info_t      dev_info;
}__test_sc_t;

#define TEST_SEND_PKT       0
#define TEST_PIPE_NUM       0
#define TEST_DEFAULT_PIPE   0
#define TEST_MAX_DEVS       2

#define HIF_PCI_PIPE_RX0    0
#define HIF_PCI_PIPE_RX1    1
#define HIF_PCI_PIPE_TX0    0
#define HIF_PCI_PIPE_TX1    1

__test_sc_t       test_sc[TEST_MAX_DEVS] =   {
    { /* Pipe 0*/
      .next_idx = 1, 
      .dev_info = { .if_name = "sn" , .dev_addr = "\0SNUL0"},
      .tx_pipe = HIF_PCI_PIPE_TX0,
      .rx_pipe = HIF_PCI_PIPE_RX0,
    },
    { /*Pipe 1*/
      .next_idx = 0,
      .dev_info = { .if_name = "sn" , .dev_addr = "\0SNUL1"},
      .tx_pipe = HIF_PCI_PIPE_TX0,
      .rx_pipe = HIF_PCI_PIPE_RX0,
    }
};

a_uint8_t         test_pipe = TEST_PIPE_NUM;
HIF_HANDLE        hif_handle = NULL;

/******************************* DUMMY CALLS **********************/    
a_status_t
test_cmd(adf_drv_handle_t  hdl, adf_net_cmd_t  cmd, 
         adf_net_cmd_data_t *data)
{
    adf_os_print("Inside %s\n",__FUNCTION__);

    return A_STATUS_OK;
}
a_status_t
test_tx_timeout(adf_drv_handle_t  hdl)
{

    adf_os_print("Inside %s\n",__FUNCTION__);
    return A_STATUS_OK;
}
/******************************** TX & RX handlers *****************/
a_uint8_t
test_swap_ip(adf_nbuf_t    buf)
{
    adf_os_sglist_t   sg   = {0};
    adf_net_iphdr_t   *ih  = {0};
    a_uint32_t        *src, *dst;
    int               i = 0, len;

    adf_nbuf_frag_info(buf, &sg);

    for(i = 0, len = 0; i < sg.nsegs; i++){
        if((len + sg.sg_segs[i].len) > sizeof(adf_net_ethhdr_t))
            break;
        len += sg.sg_segs[i].len;
    }
    /**
     * No segements found fatal
     */
    adf_os_assert(i < sg.nsegs);
    /**
     * The segment should atleast contain an IP header
     */
    adf_os_assert((sg.sg_segs[i].len - len) >= 
                  sizeof(adf_net_iphdr_t)); 

    ih = (adf_net_iphdr_t *)(sg.sg_segs[i].vaddr + 
                             sizeof(adf_net_ethhdr_t) - len);

    /**
     * Toggle the third octet (Class C)
     */
    src = &ih->ip_saddr;
    dst = &ih->ip_daddr;

    ((a_uint8_t *)src)[2] ^= 1;
    ((a_uint8_t *)dst)[2] ^= 1;

    return (((a_uint8_t *)dst)[2] & 1);
}
void
test_fix_cksum(adf_nbuf_t  buf)
{
    adf_nbuf_rx_cksum_t  cksum = {0};

    cksum.result    = ADF_NBUF_RX_CKSUM_UNNECESSARY;
    
    adf_nbuf_set_rx_cksum(buf, &cksum);
}
/**
 * @brief Test Receive Function
 * 
 * @param context
 * @param buf
 * @param pipe
 * 
 * @return A_STATUS
 */
A_STATUS
test_recv(void *context, adf_nbuf_t buf, a_uint8_t pipe)
{
    a_uint32_t  len = 0;
    __test_sc_t   *sc = NULL;
    a_uint8_t sc_idx;

    /**
     * Assert if the pipe != 0 or 1
     */
    adf_os_assert(pipe == 0 || pipe == 1);

    len = adf_nbuf_len(buf);

    /**
     * Swap the of network number
     */
    sc_idx = test_swap_ip(buf);
    /**
     * Single pipe
     */ 
#if 1
    sc = &test_sc[sc_idx];
#endif
    /**
     * For Multiple pipes
     */ 
#if 0    
    sc = &test_sc[pipe];
#endif
    
    /**
     * Need to fix the IP Checksum
     */
    test_fix_cksum(buf);

    adf_net_indicate_packet(sc->net_handle, buf, len);
    
    return A_OK;
}   

a_status_t
test_tx(adf_drv_handle_t  hdl, adf_nbuf_t  buf)
{
    __test_sc_t   *sc = (__test_sc_t  *)hdl;
    a_uint32_t    flags = 0;

    adf_os_spin_lock_irq(&sc->lock_irq, flags);

    HIFSend(hif_handle, sc->tx_pipe, NULL, buf);
        
    adf_os_spin_unlock_irq(&sc->lock_irq, flags);
    
    return A_STATUS_OK;
}
A_STATUS
test_xmit_done(void *context, adf_nbuf_t buf)
{
    adf_nbuf_free(buf);

    return A_OK;
}
/******************************* Open & Close *************************/
a_status_t
test_open(adf_drv_handle_t  hdl)
{
    __test_sc_t   *sc = (__test_sc_t  *)hdl;

    adf_net_start_queue(sc->net_handle);
    
    HIFStart(hif_handle);

    return A_STATUS_OK;
}
void
test_close(adf_drv_handle_t  hdl)
{  
    __test_sc_t   *sc = (__test_sc_t  *)hdl;
    
    adf_net_stop_queue(sc->net_handle);
}
/*************************** Device Inserted & Removed handler ********/
A_STATUS
test_create_dev(a_uint8_t   dev_num)
{
    __test_sc_t     *sc = &test_sc[dev_num];
    adf_dev_sw_t    sw = {0};


    sw.drv_open         = test_open;
    sw.drv_close        = test_close;
    sw.drv_cmd          = test_cmd;
    sw.drv_tx           = test_tx;
    sw.drv_tx_timeout   = test_tx_timeout;


    sc->net_handle = adf_net_dev_create(sc, &sw, &sc->dev_info);

    if ( !sc->net_handle ) {
        adf_os_print("Failed to create netdev\n");
        return A_NO_MEMORY;
    }
    
    adf_os_spinlock_init(&sc->lock_irq);
    return A_OK;
}

A_STATUS
test_insert_dev(HIF_HANDLE hif)
{
    HTC_CALLBACKS   pkt_cb = {0};
    A_STATUS        ret = A_OK;
    a_uint8_t       i = 0;

    for (i = 0; i < TEST_MAX_DEVS; i++) {
        if((ret = test_create_dev(i)) != A_OK)
            return ret;
    }

    hif_handle = hif;

    pkt_cb.Context              = &test_sc;
    pkt_cb.rxCompletionHandler  = test_recv;
    pkt_cb.txCompletionHandler  = test_xmit_done;
    
    HIFPostInit(hif_handle, NULL, &pkt_cb);
              
    return A_OK;    
}

A_STATUS
test_remove_dev(void *ctx)
{
    a_uint8_t   i = 0;

    HIFShutDown(hif_handle);

    for( i = 0; i < TEST_MAX_DEVS; i++)
        adf_net_dev_delete(test_sc[i].net_handle);
    
    return A_OK;
}
/************************** Module Init & Exit ********************/
int
test_init(void)
{
    HTC_DRVREG_CALLBACKS cb;

    cb.deviceInsertedHandler = test_insert_dev;
    cb.deviceRemovedHandler  = test_remove_dev;

    HIF_register(&cb);
        
    return 0;
}
void
test_exit(void)
{
    test_remove_dev(NULL);
}
adf_os_export_symbol(test_open);
adf_os_export_symbol(test_close);
adf_os_export_symbol(test_tx);
adf_os_export_symbol(test_cmd);
adf_os_export_symbol(test_tx_timeout);

adf_os_virt_module_init(test_init);
adf_os_virt_module_exit(test_exit);

