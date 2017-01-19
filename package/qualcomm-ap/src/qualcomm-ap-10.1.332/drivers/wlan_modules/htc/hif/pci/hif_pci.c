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
#include <adf_os_irq.h>
#include <adf_os_io.h>
//#include <adf_os_pci.h>
#include <adf_os_timer.h>
#include <adf_os_time.h>
#include <adf_os_module.h>
#include <adf_os_util.h>
#include <adf_os_defer.h>
#include <adf_nbuf.h>

#include <athdefs.h>
#include <a_osapi.h>
#include <hif.h>

#include "hif_pci.h"
#include "fwd/fwd.h"

/**
 * ********************************************************
 * *********************** Enums **************************
 * ********************************************************
 */

/**
 * @brief PCI Standard registers
 */
enum __pci_std_regs{
    __PCI_LATENCY_TIMER      = 0x0d,
    __PCI_CACHE_LINE_SIZE    = 0x0c,
    __PCI_MAGPIE_RETRY_COUNT = 0x41
};

/**
 * @brief DMA Burst sizes
 */
enum __dma_burst_size{
    DMA_BURST_4W   = 0x00,
    DMA_BURST_8W   = 0x01,
    DMA_BURST_16W  = 0x02
};

enum __dma_prio_val {
    PRIO_DEF   = 0x0,
    PRIO_LOW   = 0x1,
    PRIO_MED   = 0x2,
    PRIO_HIGH  = 0x3
};

/**
 * @brief Enable or Disable Byte / Descriptor swapping
 */
enum __dma_byte_swap{
    DMA_BYTE_SWAP_OFF = 0x00,
    DMA_BYTE_SWAP_ON  = 0x01,
    DMA_DESC_SWAP_ON  = 0x02 /* Required for Big Endian Host*/
};

/**
 * @brief AHB Mode register values
 */
enum __pci_ahb_mode{
    __AHB_MODE_WRITE_EXACT     = (0x00 << 0),
    __AHB_MODE_WRITE_BUFFER    = (0x01 << 0),
    __AHB_MODE_READ_EXACT      = (0x00 << 1),
    __AHB_MODE_READ_CACHE_LINE = (0x01 << 1),
    __AHB_MODE_READ_PRE_FETCH  = (0x02 << 1),
    __AHB_MODE_PAGE_SIZE_4K    = 0x18,
    __AHB_MODE_CUST_BURST      = 0x40
};

enum print_pci{
    PRN_DMA_CHAN0 = (1 << 0), /* RX0 */
    PRN_DMA_CHAN1 = (1 << 1), /* RX1 */
    PRN_DMA_CHAN2 = (1 << 2), /* TX0 */
    PRN_DMA_CHAN3 = (1 << 3), /* TX1 */
    PRN_DMA_CHAN4 = (1 << 4), /* TX2 */
    PRN_DMA_CHAN5 = (1 << 5), /* TX3 */
    PRN_DMA_THR   = (1 << 6), /* Throughput*/
    PRN_PCI_INTR  = (1 << 7), /* PCI Interrupts*/
    PRN_Q_STOP    = (1 << 8)  /* TX Ring is empty*/
};

/**
 * @brief Interrupt Enable register will cause the Interrupt
 *        status register to set for the corresponding interrupt
 *        the Interrupt Mask Register will cause the
 *        corresponding interrupt to be reported on the PCI/PCIe
 *        Bus
 */
typedef enum __hostif_reg {
    __HOSTIF_REG_AHB_RESET     = 0x4000,/*AHB Reset reg*/
    __HOSTIF_REG_AHB_MODE      = 0x4024,/*AHB Mode reg*/
    __HOSTIF_REG_INTR_CLR      = 0x4028,/*Interrupt Status Reg,to clr */ 
    __HOSTIF_REG_INTR_STATUS   = 0x4028,/*Interrupt Status Reg */
    __HOSTIF_REG_INTR_ENB      = 0x402c,/*Interrupt Enable Reg */
    __HOSTIF_REG_INTR_MSK      = 0x4034,/*Interrupt Mask Reg */
} __hostif_reg_t;

/**
 * @brief AHB reset register
 */
typedef enum __hostif_ahb_reset {
    __HOSTIF_RESET_ALL      = (1 << 16),
    __HOSTIF_RESET_HST_DMA  = (1 << 24)
} __hostif_ahb_reset;

/**
 * @brief PCI host interrupt bits
 */
typedef enum __hostif_intr_bits {
    __HOSTIF_INTR_TGT_DMA_RST   = (1 << 0),
    __HOSTIF_INTR_DMA           = (1 << 1)
} __hostif_intr_bits_t;

/**
 * @brief DMA Engine
 * Note: H/W priority order ( n+1 > n > n-1 > n-2)
 */
typedef enum __dma_eng {
    __DMA_ENGINE_RX0 = 0,
    __DMA_ENGINE_RX1 = 1,
    __DMA_ENGINE_TX0 = 2,
    __DMA_ENGINE_TX1 = 3,
    __DMA_ENGINE_TX2 = 4,
    __DMA_ENGINE_TX3 = 5,
    __DMA_ENGINE_MAX
} __dma_eng_t;

/**
 * @brief TX Pipe enum
 * 
 */
typedef enum hif_tx_pipe{
    HIF_PIPE_TX0 = 0,
    HIF_PIPE_TX1 = 1,
    HIF_PIPE_TX2 = 2,
    HIF_PIPE_TX3 = 3,
    HIF_PIPE_TX_MAX
} hif_tx_pipe_t;

/**
 * @brief RX Pipe Enum
 */
typedef enum hif_rx_pipe{
    HIF_PIPE_RX0,
    HIF_PIPE_RX1,
    HIF_PIPE_RX_MAX
} hif_rx_pipe_t;

/**
 * @brief Interrupt status bits
 */
typedef enum __dma_intr_bits {
    __DMA_INTR_TX3_END   = (1 << 27),/*TX3 reached the end or Under run*/
    __DMA_INTR_TX2_END   = (1 << 26),/*TX2 reached the end or Under run*/
    __DMA_INTR_TX1_END   = (1 << 25),/*TX1 reached the end or Under run*/
    __DMA_INTR_TX0_END   = (1 << 24),/*TX0 reached the end or Under run*/
    __DMA_INTR_TX3_DONE  = (1 << 19),/*TX3 has transmitted a packet*/
    __DMA_INTR_TX2_DONE  = (1 << 18),/*TX2 has transmitted a packet*/
    __DMA_INTR_TX1_DONE  = (1 << 17),/*TX1 has transmitted a packet*/
    __DMA_INTR_TX0_DONE  = (1 << 16),/*TX0 has transmitted a packet*/
    __DMA_INTR_RX1_END   = (1 << 9), /*RX1 reached the end or Under run*/
    __DMA_INTR_RX0_END   = (1 << 8), /*RX0 reached the end or Under run*/
    __DMA_INTR_RX1_DONE  = (1 << 1), /*RX1 received a packet*/
    __DMA_INTR_RX0_DONE  = 1,        /*RX0 received a packet*/
} __dma_intr_bits_t;

typedef enum __dma_eng_prio {
    __DMA_ENG_PRIO_RX1 = (PRIO_LOW << 20),
    __DMA_ENG_PRIO_RX0 = (PRIO_LOW << 16),
    __DMA_ENG_PRIO_TX3 = (PRIO_LOW << 12),
    __DMA_ENG_PRIO_TX2 = (PRIO_LOW << 8),
    __DMA_ENG_PRIO_TX1 = (PRIO_HIGH << 4),
    __DMA_ENG_PRIO_TX0 = PRIO_LOW
} __dma_eng_prio_t;

/**
 * @brief Engine offset to add for per engine register reads or
 *        writes
 */
typedef enum __dma_eng_off {
    __DMA_ENG_OFF_RX0 = 0x800,
    __DMA_ENG_OFF_RX1 = 0x900,
    __DMA_ENG_OFF_TX0 = 0xc00,
    __DMA_ENG_OFF_TX1 = 0xd00,
    __DMA_ENG_OFF_TX2 = 0xe00,
    __DMA_ENG_OFF_TX3 = 0xf00,
} __dma_eng_off_t;

/**
 *@brief DMA registers
 */
typedef enum __dma_reg_off {
    /**
     * Common for all Engines
     */
    __DMA_REG_ISR      = 0x00,/* Interrupt Status Register */
    __DMA_REG_IMR      = 0x04,/* Interrupt Mask Register */
    __DMA_REG_PRIO     = 0x08,/* DMA Engine Priority Register*/
    /**
     * Transmit
     */
    __DMA_REG_TXDESC   = 0x00,/* TX DP */
    __DMA_REG_TXSTART  = 0x04,/* TX start */
    __DMA_REG_INTRLIM  = 0x08,/* TX Interrupt limit */
    __DMA_REG_TXBURST  = 0x0c,/* TX Burst Size */
    __DMA_REG_TXSWAP   = 0x18,/* TX swap */
    /**
     * Receive
     */
    __DMA_REG_RXDESC   = 0x00,/* RX DP */
    __DMA_REG_RXSTART  = 0x04,/* RX Start */
    __DMA_REG_RXBURST  = 0x08,/* RX Burst Size */
    __DMA_REG_RXPKTOFF = 0x0c,/* RX Packet Offset */
    __DMA_REG_RXSWAP   = 0x1c /* RX Desc Swap */
} __dma_reg_off_t;

/**
 * Register Values for DMA related operation
 */ 
typedef enum __dma_reg_val {
    __DMA_REG_SET_TXCTRL = 0x01,/*TX Start bit*/
    __DMA_REG_SET_RXCTRL = 0x01 /*RX Start bit*/
} __dma_reg_val_t;    

/**
 * @brief Application registeration specific values
 * Note: this in the order of loading
 */ 
typedef enum hif_app_type {
    APP_FWD = 0, /* Firmware downloader*/
    APP_HTC = 1, /* HTC or Loopback app*/
    APP_MAX
} hif_app_type_t;

/**
 * ********************************************************
 * ******************** Constants *************************
 * ********************************************************
 */    
static const a_uint32_t     host_intr_mask =    __HOSTIF_INTR_DMA;

static const a_uint32_t     host_enb_mask  = (  __HOSTIF_INTR_TGT_DMA_RST |
                                                __HOSTIF_INTR_DMA );

static const a_uint32_t     def_ahb_mode   = (  __AHB_MODE_CUST_BURST       | 
                                                __AHB_MODE_PAGE_SIZE_4K     |
                                                __AHB_MODE_READ_PRE_FETCH   |
                                                __AHB_MODE_WRITE_BUFFER );

static const a_uint32_t     dma_prio_mask  = (  __DMA_ENG_PRIO_RX1 |
                                                __DMA_ENG_PRIO_RX0 |
                                                __DMA_ENG_PRIO_TX3 |
                                                __DMA_ENG_PRIO_TX2 |
                                                __DMA_ENG_PRIO_TX1 |
                                                __DMA_ENG_PRIO_TX0);

/**
 * *********************************************************
 * ******************** Modifiable *************************
 * *********************************************************
 */


/**
 * Normal Interrupt Mask
 */
static a_uint32_t     dma_intr_mask  = (  __DMA_INTR_RX0_DONE |
                                          __DMA_INTR_RX0_END  |
                                          __DMA_INTR_RX1_DONE |
                                          __DMA_INTR_RX1_END  |
                                          __DMA_INTR_TX0_DONE | 
                                          __DMA_INTR_TX1_DONE |
                                          __DMA_INTR_TX2_DONE |
                                          __DMA_INTR_TX3_DONE );
/**
 * RX Interrupt Mitigation mask
 */
static a_uint32_t     dma_intm_mask = (   __DMA_INTR_RX0_DONE |
                                          __DMA_INTR_RX0_END  |
                                          __DMA_INTR_TX0_DONE | 
                                          __DMA_INTR_TX1_DONE |
                                          __DMA_INTR_TX2_DONE |
                                          __DMA_INTR_TX3_DONE );
    
/**
 * ********************************************************
 * ********************** Defines *************************
 * ********************************************************
 */

#define MAX_PCI_DMAENG_RX       2
#define MAX_PCI_DMAENG_TX       4

#define HIF_PCI_MAX_INTR        20

#define DMA_MAX_INTR_TIMO       0xFFF
#define DMA_MAX_INTR_CNT        0xF
#define DMA_MAX_INTR_LIM        ((DMA_MAX_INTR_TIMO << 4) | DMA_MAX_INTR_CNT)
#define XMIT_THRESH             64
/**
 * XXX:These are all for Debugging purpose
 */ 
#define HIF_PCI_POLL_TIME       1000 


#define ATH_VID                 0x168c /* Vendor ID*/
#define ATH_MAGPIE_PCI          0xff1d /* Magpie PCI Device ID  */
#define ATH_MAGPIE_PCIE         0xff1c /* Magpie PCIE Device ID */
#define ATH_OWL_PCI             0x0024 /* Owl Device ID */
#define ATH_MERLIN_PCI          0x002a /* Merlin Device ID*/

/**
 * @brief helper for Device insertion and removal , packet
 *        received & xmitted
 */
#define app_dev_inserted(_cb, _hif_hdl)             \
    (_cb).deviceInsertedHandler((_hif_hdl), (_hif_hdl->os_hdl))
#define app_dev_removed(_cb, _app_hdl)              \
    (_cb).deviceRemovedHandler((_app_hdl))
#define app_ind_xmitted(_cb, _ctx, _pkt)            \
    (_cb).txCompletionHandler((_ctx), (_pkt))
#define app_ind_pkt(_cb, _ctx, _pkt, _pipe)         \
    (_cb).rxCompletionHandler((_ctx), (_pkt), (_pipe))

#define hif_pci_sc(_hdl)      (hif_pci_softc_t *)(((adf_os_handle_t)_hdl)->host_hif_handle)

#define cnv_mbits(_bytes)      (((_bytes) * 8)/(1024 * 1024))

/**
 * ********************************************************
 * ******************* Data Types *************************
 * ********************************************************
 */

/**
 * @brief Context buffer info for registered applications
 */
typedef struct hif_cbinfo {
    HTC_DRVREG_CALLBACKS    dev;
    HTC_CALLBACKS           pkt;
    void                   *app_ctx;/*XXX: Why do we need this ???*/
} hif_cbinfo_t;

/**
 * @brief Softc for HIF_PCI
 */
typedef struct hif_pci_softc {
    /* Application & callbacks*/
    hif_cbinfo_t            cb[APP_MAX];
    hif_app_type_t          cur_app;
    hif_app_type_t          next_app;

    /* Device & Engines*/
    adf_os_device_t         osdev;/*Device handle*/
    pci_dma_softc_t         dma_eng[__DMA_ENGINE_MAX];

    /* Locks*/
    adf_os_spinlock_t       lock_irq;/* Intr. contention*/
    adf_os_mutex_t          lock_mtx;/* Sleepable*/

    /* Polling or timer */
    adf_os_timer_t          poll;

    /* Other handles*/
    adf_os_handle_t         os_hdl;
} hif_pci_softc_t;

/**
 * @brief This is used for statistics collection from HIF_PCI
 *        prespective
 */
typedef struct __hif_pci_dma_stats {
    a_uint32_t      npkts; /* Packets processed */
    a_uint32_t      bytes; /* Bytes consumed */
    a_uint32_t      drop;  /* Drops made */
    a_uint32_t      ent;   /* Times invoked */
    a_uint32_t      stop;  /* Times DMA stoppped */
    a_uint32_t      grab;  /* Times DMA grabbed a pkt */
    a_uint64_t      ticks; /* Delta Ticks */
    a_uint32_t      cycles;/* cycles per Delta*/ 
} __hif_pci_dma_stats_t;

typedef struct __hif_pci_stats {
    a_uint32_t                pci_intr;
    a_uint32_t                dma_intr;
    __hif_pci_dma_stats_t     dma[__DMA_ENGINE_MAX];
} __hif_pci_stats_t;


/**
 * ********************************************************
 * ******************* Prototypes *************************
 * ********************************************************
 */

//adf_drv_handle_t        hif_pci_attach(adf_os_pci_data_t *, adf_os_device_t );
//void                    hif_pci_detach(adf_drv_handle_t );
//void                    hif_pci_suspend(adf_drv_handle_t , adf_os_pm_t );
//void                    hif_pci_resume(adf_drv_handle_t );
adf_os_irq_resp_t       hif_pci_intr(adf_drv_handle_t);
a_status_t              hif_pci_hard_xmit(hif_pci_softc_t *, adf_nbuf_t , 
                                          __dma_eng_t );

void                    __hif_pci_recv_pkt(hif_pci_softc_t   *, __dma_eng_t );
void                    __hif_pci_recv_start(hif_pci_softc_t *, __dma_eng_t );
void                    __hif_pci_xmit_done(hif_pci_softc_t  *, __dma_eng_t );
void                    __hif_pci_dma_intr(hif_pci_softc_t  *);
void                    __hif_pci_dma_bh(void  *);


/**
 * ********************************************************
 * ********************* Globals **************************
 * ********************************************************
 */
__dma_eng_off_t eng_tbl[__DMA_ENGINE_MAX] = {
    __DMA_ENG_OFF_RX0,
    __DMA_ENG_OFF_RX1,
    __DMA_ENG_OFF_TX0,
    __DMA_ENG_OFF_TX1,
    __DMA_ENG_OFF_TX2,
    __DMA_ENG_OFF_TX3
};

/* const a_uint32_t            print_buf_lim  = 10; */
static __hif_pci_stats_t       hif_stats;
/* a_uint32_t                  hif_pci_dbg_info = 0; */

hif_cbinfo_t        hif_cb[APP_MAX] = {{{0}}};
a_uint32_t          next_app = 0;
hif_pci_softc_t    *dev_found[HIF_PCI_MAX_DEVS] = {0};

//typedef struct ag7100_stats_info {
//    a_uint32_t  rx_intr;
//    a_uint32_t  rx_npkts;
//    a_uint32_t  rx_sched;
//    a_uint64_t  rx_ticks;
//    a_uint32_t  rx_cycles;
//} ag7100_stats_info_t;

/* extern ag7100_stats_info_t   ag7100_stats; */

#if 0
static adf_os_pci_devid_t   pci_devids[] = {
    { ATH_VID, ATH_MAGPIE_PCI, ADF_OS_PCI_ANY_ID, ADF_OS_PCI_ANY_ID},
    { ATH_VID, ATH_OWL_PCI, ADF_OS_PCI_ANY_ID, ADF_OS_PCI_ANY_ID},
    { ATH_VID, ATH_MAGPIE_PCIE, ADF_OS_PCI_ANY_ID, ADF_OS_PCI_ANY_ID},
    { ATH_VID, ATH_MERLIN_PCI, ADF_OS_PCI_ANY_ID, ADF_OS_PCI_ANY_ID},
    {0},
};

adf_os_pci_drvinfo_t   drv_info = adf_os_pci_set_drv(hif_pci,&pci_devids[0],
                                                     hif_pci_attach,
                                                     hif_pci_detach,
                                                     hif_pci_suspend,
                                                     hif_pci_resume);
#endif

a_uint32_t      hif_pci_intm   = 1; /* Default RX Mitigation turned on*/
a_uint32_t      hif_pci_debug  = 0; /* Default Debug prints disabled*/
a_uint32_t      pci_prn_mask   = 0; /* Default Debug prints */
/**
 * ********************************************************
 * ********************** APIs ****************************
 * ********************************************************
 */

/**
 * @brief convert the pipe number into the index into DMA engine
 *        queue
 * 
 * @param pipe
 * 
 * @return a_int32_t
 */
static inline __dma_eng_t
__get_tx_engno(hif_tx_pipe_t  pipe)
{
    switch (pipe) {
        /**
         * TX Engine Index
         */
    case HIF_PIPE_TX0:
        return __DMA_ENGINE_TX0;

    case HIF_PIPE_TX1:
        return __DMA_ENGINE_TX1;

    case HIF_PIPE_TX2:
        return __DMA_ENGINE_TX2;

    case HIF_PIPE_TX3:
        return __DMA_ENGINE_TX3;
        /**
         * Error
         */
    default:
        adf_os_print("Invalid TX Pipe number = %d\n", pipe);
        adf_os_assert(0);
        return __DMA_ENGINE_MAX;
    }
}

/**
 * @brief Convert a Engine number into a pipe number
 * 
 * @param eng_no
 * 
 * @return hif_rx_pipe_t
 */
static inline hif_rx_pipe_t
__get_rx_pipe(__dma_eng_t  eng_no)
{
    switch (eng_no) {
    case __DMA_ENGINE_RX0:
        return HIF_PIPE_RX0;

    case __DMA_ENGINE_RX1:
        return HIF_PIPE_RX1;

    default:
        adf_os_print("Invalid RX Engine number = %d\n", eng_no);
        adf_os_assert(0);
        return HIF_PIPE_RX_MAX;
    }
}

/**
 * @brief DMA packet received,
 * 
 * @param sc
 * @param eng
 * 
 * @return a_uint8_t
 */
void
__hif_pci_recv_pkt(hif_pci_softc_t *sc, __dma_eng_t  eng)
{
    adf_nbuf_t          buf;
    HTC_CALLBACKS       pkt = {0};
    adf_os_device_t     osdev = sc->osdev;
    pci_dma_softc_t   * dma_q = &sc->dma_eng[eng];
    struct zsDmaDesc  * hwdesc;
    zdma_swdesc_t     * swdesc;
    a_uint32_t          head, pkt_len;

    pkt = sc->cb[sc->cur_app].pkt;

    hif_stats.dma[eng].ent ++;

    head = dma_q->head;

    do {
        swdesc = &dma_q->sw_ring[head];

        hwdesc = swdesc->descp;

        if (hw_desc_own(hwdesc))
            break;

        pkt_len = hw_desc_len(hwdesc);

        buf = pci_dma_unlink_buf(osdev, swdesc);
        pci_dma_recv_refill(osdev, swdesc, MAX_NBUF_SIZE);

        adf_nbuf_put_tail(buf, pkt_len);

        head = ring_rx_incr(head);
        
        hif_stats.dma[eng].npkts ++;
        hif_stats.dma[eng].bytes += adf_nbuf_len(buf);

        adf_os_assert(buf);

        app_ind_pkt(pkt, pkt.Context, buf, eng);
        
    } while(1);

    dma_q->head = dma_q->tail = head;
}

/**
 * @brief Restart the RX Engine
 * 
 * @param sc
 * @param eng
 */
void
__hif_pci_recv_start(hif_pci_softc_t   *sc, __dma_eng_t  eng)
{
    a_uint32_t   addr = eng_tbl[eng];

    hif_stats.dma[eng].stop ++;

    adf_os_reg_write32(sc->osdev, addr + __DMA_REG_RXSTART, 
                       __DMA_REG_SET_RXCTRL);
}

/**
 * @brief DMA Xmit done
 * 
 * @param sc
 * @param eng
 * 
 * @return a_status_t
 */
void
__hif_pci_xmit_done(hif_pci_softc_t *sc, __dma_eng_t  eng)
{
    adf_nbuf_t          buf;
    HTC_CALLBACKS       pkt = {0};
    adf_os_device_t     osdev = sc->osdev;
    pci_dma_softc_t   * dma_q = &sc->dma_eng[eng];
    struct zsDmaDesc  * hwdesc;
    zdma_swdesc_t     * swdesc;
    a_uint32_t          tail, head;

    pkt = sc->cb[sc->cur_app].pkt;

    hif_stats.dma[eng].ent ++;

    head = dma_q->head;
    tail = dma_q->tail;

    do {
        swdesc = &dma_q->sw_ring[tail];

        if (ring_empty(head, tail))
            break;

        hwdesc = swdesc->descp;

        if ( hw_desc_own(hwdesc) )
            break;
        
        buf = pci_dma_unlink_buf(osdev, swdesc);

        tail = ring_tx_incr(tail);

        hif_stats.dma[eng].npkts ++;
        hif_stats.dma[eng].bytes += adf_nbuf_len(buf);

        adf_os_assert(buf);

        app_ind_xmitted(pkt, pkt.Context, buf);

    } while(1);

    dma_q->tail = tail;   
}

/**
 * @brief Interrupt handler
 * 
 * @param hdl
 * 
 * @return adf_os_irq_resp_t
 */
adf_os_irq_resp_t 
hif_pci_intr(adf_drv_handle_t hdl)
{
    hif_pci_softc_t *sc = hif_pci_sc(hdl);
    a_uint32_t   status;

    /**
     * Apparently this is useless in PB44 which doesn't sit on a
     * shared PCI link
     */
/*     status = adf_os_reg_read32(sc->osdev, __HOSTIF_REG_INTR_STATUS); */
/*  */
/*     if ( adf_os_unlikely((status & host_intr_mask) == 0)){ */
/*         adf_os_print("others\n"); */
/*         return ADF_OS_IRQ_NONE; */
/*     } */
        

    /**
     * Enable this if you are using this for dedicated PCI boards
     */
    status = __HOSTIF_INTR_DMA;

    hif_stats.pci_intr ++;
/*     adf_os_reg_write32(sc->osdev, __HOSTIF_REG_INTR_CLR, status); */

    if (status & __HOSTIF_INTR_DMA)
        __hif_pci_dma_intr(sc);

    /* debug we shouldn't get this interrupt: remove later */
    if ( status & __HOSTIF_INTR_TGT_DMA_RST )
        adf_os_print("TGT Reset Interrupt\n");

/*     adf_os_reg_write32(sc->osdev, __HOSTIF_REG_INTR_CLR, status); */
/*     adf_os_reg_read32(sc->osdev, __HOSTIF_REG_INTR_CLR); */

    return ADF_OS_IRQ_HANDLED;
}

/**
 * @brief Disable the interrupts
 * 
 * @param sc
 * @param bits
 */
void
__dma_intr_disable(hif_pci_softc_t  *sc, __dma_intr_bits_t  bits)
{
    dma_intr_mask &= ~bits;
    adf_os_reg_write32(sc->osdev, __DMA_REG_IMR, dma_intr_mask);
}

/**
 * @brief Enable the DMA intrrupts
 * 
 * @param sc
 * @param bits
 */
void
__dma_intr_enable(hif_pci_softc_t  *sc, __dma_intr_bits_t  bits)
{
    dma_intr_mask |= bits;
    adf_os_reg_write32(sc->osdev, __DMA_REG_IMR, dma_intr_mask);
}

void
hif_recv(void *dev)
{
    hif_pci_softc_t  *sc = dev_found[0];

    __hif_pci_recv_pkt(sc, __DMA_ENGINE_RX1);
}


/**
 * @brief The DMA interrupt handler
 * 
 * @param sc
 * 
 * @return a_status_t
 */
void
__hif_pci_dma_intr(hif_pci_softc_t *sc)
{
    a_uint32_t  status;
/*     a_uint32_t  free_desc; */

    status  = adf_os_reg_read32(sc->osdev, __DMA_REG_ISR);

    adf_os_reg_write32(sc->osdev, __HOSTIF_REG_INTR_CLR, __HOSTIF_INTR_DMA);

/*    prefetch(sc->dma_eng); */
    status &= dma_intr_mask;

    do{
        /**
         * XXX: This is specific to Hydra, ADF version pending
         */ 

//#ifdef __LINUX_MIPS32_ARCH__
//        ar7100_flush_pci();
//#endif
       /**
        * TX done reap
        */  
        if(status & __DMA_INTR_TX0_DONE)
            __hif_pci_xmit_done(sc, __DMA_ENGINE_TX0);

        if(status & __DMA_INTR_TX3_DONE)
            __hif_pci_xmit_done(sc, __DMA_ENGINE_TX3);

       if(status & __DMA_INTR_TX2_DONE)
            __hif_pci_xmit_done(sc, __DMA_ENGINE_TX2);

        if(status & __DMA_INTR_TX1_DONE) {
            __hif_pci_xmit_done(sc, __DMA_ENGINE_TX1);
            
/*             free_desc = ring_tx_free(sc->dma_eng[__DMA_ENGINE_TX1].head, */
/*                                      sc->dma_eng[__DMA_ENGINE_TX1].tail); */
/*  */
/*             if (free_desc){ */
/*                 __dma_intr_disable(sc, __DMA_INTR_TX1_DONE); */
/*             } */
        }
        /**
         * RX reap and enable
         */ 
        if(status & __DMA_INTR_RX1_DONE)
            __hif_pci_recv_pkt(sc, __DMA_ENGINE_RX1);

        if(status & __DMA_INTR_RX1_END)
            __hif_pci_recv_start(sc, __DMA_ENGINE_RX1);

        if(status & __DMA_INTR_RX0_DONE)
            __hif_pci_recv_pkt(sc, __DMA_ENGINE_RX0);
        
        if(status & __DMA_INTR_RX0_END)
            __hif_pci_recv_start(sc, __DMA_ENGINE_RX0);
        
        status  = adf_os_reg_read32(sc->osdev, __DMA_REG_ISR);

        hif_stats.dma_intr ++;

        status &= dma_intr_mask;

    } while ( status );

}

/**
 * @brief Get the number of Free Descriptors available for TX
 * 
 * @param hHIF
 * @param PipeID
 * 
 * @return a_uint16_t
 */
a_uint16_t 
HIFGetFreeQueueNumber(HIF_HANDLE hHIF, a_uint8_t PipeID)
{
//    hif_pci_softc_t  *sc = hif_pci_sc(hHIF);
    hif_pci_softc_t  *sc = (hif_pci_softc_t *)hHIF;
    __dma_eng_t       eng;
    a_uint32_t       irq_flags = 0;
    a_uint16_t       is_full;
    a_uint16_t       free_num = 0;

    eng = __get_tx_engno(PipeID);

    adf_os_spin_lock_irq(&sc->lock_irq, irq_flags);

    is_full = ring_full(sc->dma_eng[eng].head, sc->dma_eng[eng].tail);

    if (PipeID == 1)
        free_num = ring_tx_free(sc->dma_eng[eng].head, sc->dma_eng[eng].tail);

    adf_os_spin_unlock_irq(&sc->lock_irq, irq_flags);

    if (is_full)
        hif_stats.dma[eng].stop++;

    if (PipeID == 1) {
        return free_num;
    }

    return !is_full;
}



/**
 * @brief This Xmits a given nbuf on a given pipe / DMA engine,
 *        this should pull a vdesc out from the ready Queue and
 *        put it in the Run queue for transmission and hit the
 *        launch button.
 * 
 * @param hdl
 * @param nbuf
 * @param pipe
 * 
 * @return a_status_t
 */
a_status_t
hif_pci_slow_xmit(hif_pci_softc_t  *sc, adf_nbuf_t  buf, __dma_eng_t  eng_no)
{
    a_uint32_t        addr;
    a_uint32_t        irq_flags = 0;
    a_uint32_t        head;
    zdma_swdesc_t   * swdesc;
    pci_dma_softc_t * dma_q = &sc->dma_eng[eng_no]; 


    addr = eng_tbl[eng_no];
  
    /**
     * Locks, qlen is updated in enq (intr context) & deq
     */
    adf_os_spin_lock_irq(&sc->lock_irq, irq_flags);

    
    if (ring_full(dma_q->head, dma_q->tail)) {
        adf_os_spin_unlock_irq(&sc->lock_irq, irq_flags);
        goto fail;
    }

    head = dma_q->head;

    dma_q->head = ring_tx_incr(head);

    swdesc = &dma_q->sw_ring[head];

    adf_os_assert(swdesc->nbuf == NULL);

    pci_dma_link_buf(sc->osdev, swdesc, buf);

    pci_zdma_mark_rdy(swdesc, (ZM_FS_BIT | ZM_LS_BIT));

    adf_os_spin_unlock_irq(&sc->lock_irq, irq_flags);

    adf_os_reg_write32(sc->osdev, addr + __DMA_REG_TXSTART, 
                       __DMA_REG_SET_TXCTRL);

    return A_STATUS_OK;

fail:

    hif_stats.dma[eng_no].drop ++;
 
    return A_STATUS_ENOMEM;    
}

a_status_t
hif_pci_fast_xmit(hif_pci_softc_t  *sc, adf_nbuf_t  buf, __dma_eng_t  eng_no)
{
    a_uint32_t        addr;
    a_uint32_t        irq_flags;
    a_uint32_t        head;
    zdma_swdesc_t   * swdesc;
    pci_dma_softc_t * dma_q = &sc->dma_eng[eng_no]; 
/*     a_uint32_t        free_desc; */

    addr = eng_tbl[eng_no];
  
    /**
     * Locks, qlen is updated in enq (intr context) & deq
     */
    adf_os_spin_lock_irq(&sc->lock_irq, irq_flags);

    if (adf_os_unlikely(ring_full(dma_q->head, dma_q->tail))) {
        adf_os_spin_unlock_irq(&sc->lock_irq, irq_flags);
        adf_os_assert(0);
        goto fail;
    }

    head = dma_q->head;

    dma_q->head = ring_tx_incr(head);

    swdesc = &dma_q->sw_ring[head];

    adf_os_assert(swdesc->nbuf == NULL);

    pci_dma_link_buf(sc->osdev, swdesc, buf);

    pci_zdma_mark_rdy(swdesc, (ZM_FS_BIT | ZM_LS_BIT));

    adf_os_spin_unlock_irq(&sc->lock_irq, irq_flags);

    adf_os_reg_write32(sc->osdev, addr + __DMA_REG_TXSTART, 
                       __DMA_REG_SET_TXCTRL);

/*     adf_os_reg_read32(sc->osdev, addr + __DMA_REG_TXSTART); */

    /* Lazy Reaper */

/*     adf_os_spin_lock_irq(&sc->lock_irq, irq_flags); */
/*  */
/*     adf_os_assert(!(dma_intr_mask & __DMA_INTR_TX1_DONE)); */
/*  */
/*     free_desc = ring_tx_free(dma_q->head, dma_q->tail); */
/*  */
/*     if ((free_desc < XMIT_THRESH)) { */
/*  */
/*         __hif_pci_xmit_done(sc, eng_no); */
/*  */
/*         free_desc = ring_tx_free(dma_q->head, dma_q->tail); */
/*  */
/*         if (!free_desc) { */
/*             __dma_intr_enable(sc, __DMA_INTR_TX1_DONE); */
/*             adf_os_reg_read32(sc->osdev, addr + __DMA_REG_TXSTART); */
/*         } */
/*     } */
/*  */
/*     adf_os_spin_unlock_irq(&sc->lock_irq, irq_flags); */

    return A_STATUS_OK;

fail:

    hif_stats.dma[eng_no].drop ++;
 
    return A_STATUS_ENOMEM;    
}

/**
 * @brief HIF Send routine
 * 
 * @param dev
 * @param PipeID
 * @param hdr_buf
 * @param buf
 * 
 * @return hif_status_t
 */
hif_status_t
HIFSend(void *dev, a_uint8_t PipeID, adf_nbuf_t hdr_buf, adf_nbuf_t buf)
{
/*     __dma_eng_t      eng; */
/*     eng = __get_tx_engno(PipeID); */
/*     eng = PipeID + 2; */
/*     if(eng == __DMA_ENGINE_MAX) */
/*         return A_EINVAL; */
/*     if(adf_os_unlikely( hdr_buf != NULL)) { */
/*         xmit = hdr_buf; */
/*         adf_nbuf_cat(xmit, buf); */
/*     } */

//    return hif_pci_fast_xmit(hif_pci_sc(dev), buf, (PipeID + 2));
    return hif_pci_fast_xmit(dev, buf, (PipeID + 2));
}

/**
 * @brief Poll controller
 * 
 * @param arg
 */
void 
__hif_pci_poll(void *arg)
{
    hif_pci_softc_t         *sc = hif_pci_sc(arg);
    a_uint32_t               flags = 0;
    __hif_pci_dma_stats_t   *pkt_cnt = hif_stats.dma;

    /**
     * This is the default setting
     */
    if (!pci_prn_mask) {

        adf_os_print("hif:dma (%d: %d) ", hif_stats.pci_intr, 
                     hif_stats.dma_intr);
        adf_os_print("r0 i:n (%d: %d) ",pkt_cnt[0].ent, pkt_cnt[0].npkts);
        adf_os_print("t1 i:n (%d: %d) ",pkt_cnt[3].ent, pkt_cnt[3].npkts); 

    }else {

        if (pci_prn_mask & PRN_PCI_INTR)
            adf_os_print("hif:dma (%d: %d) ", hif_stats.pci_intr, 
                         hif_stats.dma_intr);

        if (pci_prn_mask & PRN_DMA_CHAN0)
            adf_os_print("r0 i:n (%d: %d) ",pkt_cnt[0].ent, pkt_cnt[0].npkts);

        if (pci_prn_mask & PRN_DMA_CHAN1)
            adf_os_print("r1 i:n (%d: %d) ",pkt_cnt[1].ent, pkt_cnt[1].npkts);

        if (pci_prn_mask & PRN_DMA_CHAN2)
            adf_os_print("t0 i:n (%d: %d) ",pkt_cnt[2].ent, pkt_cnt[2].npkts); 

        if (pci_prn_mask & PRN_DMA_CHAN3)
            adf_os_print("t1 i:n (%d: %d) ",pkt_cnt[3].ent, pkt_cnt[3].npkts); 

        if (pci_prn_mask & PRN_DMA_CHAN4)
            adf_os_print("t2 i:n (%d: %d) ",pkt_cnt[4].ent, pkt_cnt[4].npkts); 

        if (pci_prn_mask & PRN_DMA_CHAN5)
            adf_os_print("t3 i:n (%d: %d) ",pkt_cnt[5].ent, pkt_cnt[5].npkts); 

        if (pci_prn_mask & PRN_DMA_THR)
            adf_os_print("Thr = %d ", cnv_mbits(pkt_cnt[0].bytes + 
                                                pkt_cnt[1].bytes +
                                                pkt_cnt[2].bytes +
                                                pkt_cnt[3].bytes +
                                                pkt_cnt[4].bytes +
                                                pkt_cnt[5].bytes));
        if (pci_prn_mask & PRN_Q_STOP) 
            adf_os_print("TXQ stop (%d:%d:%d:%d) ",pkt_cnt[2].stop,
                         pkt_cnt[3].stop,
                         pkt_cnt[4].stop,
                         pkt_cnt[5].stop);

    }

    adf_os_print("\n");

    adf_os_spin_lock_irq(&sc->lock_irq, flags);
    adf_os_mem_zero(&hif_stats, sizeof(struct __hif_pci_stats));

    adf_os_spin_unlock_irq(&sc->lock_irq, flags);
    adf_os_timer_start(&sc->poll, HIF_PCI_POLL_TIME);
}

/**
 * @brief Setup the descriptors of engines in the H/W Dp(s)
 * 
 * @param sc
 */
void
__hif_pci_setup_rxeng(hif_pci_softc_t *sc)
{
    a_uint32_t  i = 0, addr = 0, paddr;

    for (i = 0; i < MAX_PCI_DMAENG_RX; i++) {
        addr = eng_tbl[i]; 
        paddr = pci_dma_tail_addr(&sc->dma_eng[i]);

        adf_os_reg_write32(sc->osdev, addr + __DMA_REG_RXDESC, paddr);
        adf_os_reg_write32(sc->osdev, addr + __DMA_REG_RXBURST, DMA_BURST_8W);
        
#if defined (ADF_BIG_ENDIAN_MACHINE)
        adf_os_reg_write32(sc->osdev, addr + __DMA_REG_RXSWAP, 
                           DMA_DESC_SWAP_ON);
#endif
        adf_os_reg_write32(sc->osdev, addr + __DMA_REG_RXSTART, 
                           __DMA_REG_SET_RXCTRL);
    }
    
}
/**
 * @brief Setup the descriptors of engines in the H/W Dp(s)
 * 
 * @param sc
 */
void
__hif_pci_setup_txeng(hif_pci_softc_t   *sc)
{
    a_uint32_t  i = 0, addr = 0, paddr;

    for (i = __DMA_ENGINE_TX0; i < __DMA_ENGINE_MAX; i++) {
        addr = eng_tbl[i];
        paddr = pci_dma_tail_addr(&sc->dma_eng[i]);

        adf_os_reg_write32(sc->osdev, addr + __DMA_REG_TXDESC, paddr);
        adf_os_reg_write32(sc->osdev, addr + __DMA_REG_TXBURST, DMA_BURST_16W);
        /**
         * Limit the TX interrupt to 16 packets or wait for 0xfff x 32
         * Cycles, which ever happens earlier
         */
        if (i == __DMA_ENGINE_TX1)
            adf_os_reg_write32(sc->osdev, addr + __DMA_REG_INTRLIM,
                               DMA_MAX_INTR_LIM);

#if defined (ADF_BIG_ENDIAN_MACHINE)
        adf_os_reg_write32(sc->osdev, addr + __DMA_REG_TXSWAP, 
                           DMA_DESC_SWAP_ON);
#endif

    }
}

/**
 * @brief Pull the Magpie out of reset
 * 
 * @param sc
 */
void
__hif_pci_pull_reset(hif_pci_softc_t   *sc)
{
    a_uint32_t    r_data;
    r_data = adf_os_reg_read32(sc->osdev, __HOSTIF_REG_AHB_RESET);
    r_data &= ~__HOSTIF_RESET_ALL;
    adf_os_reg_write32(sc->osdev, __HOSTIF_REG_AHB_RESET, r_data);

    adf_os_mdelay(1);
}

/**
 * @brief Put the Magpie into reset
 * 
 * @param sc
 */
void
__hif_pci_put_reset(hif_pci_softc_t   *sc)
{
    a_uint32_t    r_data;
    
    r_data = adf_os_reg_read32(sc->osdev, __HOSTIF_REG_AHB_RESET);
    r_data |= __HOSTIF_RESET_ALL;
    adf_os_reg_write32(sc->osdev, __HOSTIF_REG_AHB_RESET, r_data);

    adf_os_mdelay(1);
}

/**
 * @brief Put the DMA into Reset state
 */ 
void
__hif_pci_dma_put_reset(hif_pci_softc_t   *sc)
{
    a_uint32_t    r_data;

    r_data = adf_os_reg_read32(sc->osdev, __HOSTIF_REG_AHB_RESET);
    r_data |= __HOSTIF_RESET_HST_DMA;
    adf_os_reg_write32(sc->osdev, __HOSTIF_REG_AHB_RESET, r_data);
    adf_os_reg_read32(sc->osdev, __HOSTIF_REG_AHB_RESET);

    adf_os_mdelay(1);

}

/**
 * @brief Pull the DMA out of reset
 * 
 * @param sc
 */
void
__hif_pci_dma_pull_reset(hif_pci_softc_t   *sc)
{
    a_uint32_t    r_data;

    r_data = adf_os_reg_read32(sc->osdev, __HOSTIF_REG_AHB_RESET);
    r_data &= ~__HOSTIF_RESET_HST_DMA;
    adf_os_reg_write32(sc->osdev, __HOSTIF_REG_AHB_RESET, r_data);
    adf_os_reg_read32(sc->osdev, __HOSTIF_REG_AHB_RESET);
    
    adf_os_mdelay(1);
}

/**
 * @brief Target reset over install the DMA interrupts and start
 *        the DMA Engines
 * 
 * @param sc
 */
void
__hif_pci_tgt_reset_done(hif_pci_softc_t *sc)
{
    adf_os_reg_write32(sc->osdev, __DMA_REG_IMR,  0x0);

    if (hif_pci_intm)
        adf_os_reg_write32(sc->osdev, __DMA_REG_IMR,  dma_intm_mask);
    else
        adf_os_reg_write32(sc->osdev, __DMA_REG_IMR,  dma_intr_mask);

    adf_os_print("DMR read= %#x\n",
                  adf_os_reg_read32(sc->osdev, __DMA_REG_IMR));

/*     adf_os_reg_write32(sc->osdev, __DMA_REG_PRIO, dma_prio_mask); */

    __hif_pci_setup_rxeng(sc);
    __hif_pci_setup_txeng(sc);
}

/**
 * @brief Reset the Host DMA and wait until the Target has seen
 *        the Reset
 * 
 * @param sc
 */
void
__hif_pci_tgt_reset(hif_pci_softc_t   *sc)
{
    a_uint32_t   status = 0;

    __hif_pci_dma_put_reset(sc);
    
    for (;;) {
        status = adf_os_reg_read32(sc->osdev, __HOSTIF_REG_INTR_STATUS);

        if ( status & __HOSTIF_INTR_TGT_DMA_RST )
            break;
    }

    __hif_pci_dma_pull_reset(sc);

    /**
     * clear the intr source
     */
    adf_os_reg_read32(sc->osdev, __HOSTIF_REG_AHB_RESET);

    /**
     * Handle target reset completion
     */
    __hif_pci_tgt_reset_done(sc);

    /* clear the interrupt */
    adf_os_reg_write32(sc->osdev, __HOSTIF_REG_INTR_STATUS, status);
    adf_os_reg_read32(sc->osdev, __HOSTIF_REG_INTR_STATUS);

    return;
}

/**
 * @brief Host side Magpie reset
 * 
 * @param sc
 */
void
__hif_pci_reset(hif_pci_softc_t   *sc)
{
    /**
     *  Big Hammer reset
     * 
     *  NOTE: Hard reset (button) on the target resets the pci
     *  config space as well.
     */
    __hif_pci_pull_reset(sc);

    /**
     * Reset the Host side DMA
     */
    __hif_pci_dma_put_reset(sc);

    /**
     * Wait for the Target to reset itself
     */
    __hif_pci_tgt_reset(sc);

    adf_os_reg_write32(sc->osdev, __HOSTIF_REG_AHB_MODE, def_ahb_mode);
}

/**
 * @brief Flush all the TX done skbs
 * 
 * @param sc
 */
void
__hif_pci_flush_txeng(hif_pci_softc_t  *sc)
{
    a_uint32_t  i = 0;

    for (i = __DMA_ENGINE_TX0; i < __DMA_ENGINE_MAX; i++)
            __hif_pci_xmit_done(sc, i);
}

/**
 * @brief Rearm the RX engines with new skbs and freeing the old
 *        ones
 * 
 * @param sc
 */
void    
__hif_pci_flush_rxeng(hif_pci_softc_t   *sc)
{   
    a_uint32_t  i = 0;
    
    for (i = __DMA_ENGINE_RX0; i < MAX_PCI_DMAENG_RX; i++) {
        
/*         do { */
/*             if ((buf = pci_dma_recv(sc->osdev, &sc->dma_eng[i])) == NULL) */
/*                 break; */
/*  */
/*             adf_nbuf_free(buf); */
/*  */
/*         } while(1); */
    }
}

/**
 * @brief Flush all the engines
 * Note: We can also flush the RX if needed
 * @param sc
 */
void
__hif_pci_flush_engine(hif_pci_softc_t *sc)
{
    __hif_pci_flush_txeng(sc);
}

/**
 * @brief Setup the TX & RX descriptors
 * 
 * @param sc
 * @param num_desc
 * 
 */
void
__hif_pci_setup_rxdesc(hif_pci_softc_t  *sc, a_uint32_t num_desc)
{
    a_uint32_t i = 0;

    for (i = __DMA_ENGINE_RX0; i < MAX_PCI_DMAENG_RX; i ++)
        pci_dma_init_rx(sc->osdev, &sc->dma_eng[i], num_desc, MAX_NBUF_SIZE);
}

/**
 * @brief Setup the TX descriptors
 * 
 * @param sc
 * @param num_desc
 */
void
__hif_pci_setup_txdesc(hif_pci_softc_t  *sc, a_uint32_t num_desc)
{
    a_uint32_t i = 0;

    for (i = __DMA_ENGINE_TX0; i < __DMA_ENGINE_MAX; i++)
        pci_dma_init_tx(sc->osdev, &sc->dma_eng[i], num_desc);
}

/**
 * @brief Free the RX descriptors
 * 
 * @param sc
 */
void
__hif_pci_free_rxdesc(hif_pci_softc_t  *sc)
{
    a_uint32_t  i = 0;

    for (i = __DMA_ENGINE_RX0; i < MAX_PCI_DMAENG_RX; i++)
        pci_dma_deinit_rx(sc->osdev, &sc->dma_eng[i]);
}

/**
 * @brief Free the TX descriptors
 * 
 * @param sc
 */
void
__hif_pci_free_txdesc(hif_pci_softc_t  *sc)
{
    a_uint32_t  i = 0;

    for (i = __DMA_ENGINE_TX0; i < __DMA_ENGINE_MAX; i++)
        pci_dma_deinit_tx(sc->osdev, &sc->dma_eng[i]);

}

/**
 * @brief setup the interrupt handler
 * 
 * @param sc
 * 
 * @return a_status_t
 */
a_status_t
__hif_pci_setup_intr(hif_pci_softc_t *sc)
{
    a_status_t  error = A_STATUS_OK;

    /**
     * Clear all interrupts
     */
    adf_os_reg_write32(sc->osdev, __HOSTIF_REG_INTR_ENB, 0x0);
    adf_os_reg_write32(sc->osdev, __HOSTIF_REG_INTR_MSK, 0x0);

    error = adf_os_setup_intr(sc->osdev, hif_pci_intr);
    if(error)
        return A_STATUS_EIO;
    
    /**
     * Set all the Interrupts
     */

    adf_os_reg_write32(sc->osdev, __HOSTIF_REG_INTR_ENB, host_enb_mask);
    adf_os_reg_write32(sc->osdev, __HOSTIF_REG_INTR_MSK, host_intr_mask);
    
    adf_os_print("IER read = %#x\n",
                  adf_os_reg_read32(sc->osdev, __HOSTIF_REG_INTR_ENB));
    adf_os_print("IMR read = %#x\n",
                  adf_os_reg_read32(sc->osdev, __HOSTIF_REG_INTR_MSK));
    
    return error;
}

/**
 * @brief register the Poll controllers
 * 
 * @param sc
 * 
 * @return a_status_t
 */
a_status_t
__hif_pci_setup_poll(hif_pci_softc_t  *sc)
{
    adf_os_print("Setting up the timer\n");
    adf_os_timer_init(NULL, &sc->poll, __hif_pci_poll, sc);
    adf_os_timer_start(&sc->poll, HIF_PCI_POLL_TIME);

    return A_STATUS_OK;
}

/**
 * @brief Free the interrupt handler
 * 
 * @param sc
 */
void
__hif_pci_free_intr(hif_pci_softc_t  *sc)
{
    adf_os_reg_write32(sc->osdev, __HOSTIF_REG_INTR_ENB, 0x0);
    adf_os_reg_write32(sc->osdev, __HOSTIF_REG_INTR_MSK, 0x0);
    adf_os_reg_write32(sc->osdev, __DMA_REG_IMR, 0x0);

    adf_os_free_intr(sc->osdev);
}

#if 0
/**
 * @brief Attach function
 * 
 * @param res
 * @param count
 * @param data
 * @param osdev
 * 
 * @return adf_drv_handle_t
 */
adf_drv_handle_t 
hif_pci_attach(adf_os_pci_data_t *data, adf_os_device_t osdev)
{
    hif_pci_softc_t  *sc = NULL;
    a_uint32_t  i = 0;
    
    for (i = 0; i < HIF_PCI_MAX_DEVS; i++) {
        if(dev_found[i] == NULL)
            break;
    }
    /**
     * Note: If you are hitting the assert it means there is no
     * empty slot, time to change the MAX_DEVS as there are more
     * than MAX_DEVS number of devices in the system, if this is not
     * the case then there is something terribly wrong
     */
    adf_os_assert(i < HIF_PCI_MAX_DEVS);
    
    if (i >= HIF_PCI_MAX_DEVS){
        adf_os_print("FATAL: Check if there are more than %d devices\n",
                     HIF_PCI_MAX_DEVS);
        adf_os_print("FATAL: if yes than change HIF_PCI_MAX_DEVS in driver\n");
        adf_os_print("FATAL: Otherwise there is H/W or OS issue\n");
        return NULL;
    }
 
    sc = adf_os_mem_alloc(osdev, sizeof (struct hif_pci_softc));
    if(!sc)
        return NULL;
       
    dev_found[i] = sc;

    /* Softc book keeping*/
    sc->osdev         = osdev;
    sc->cur_app       = APP_FWD;
    sc->os_hdl        = adf_os_pcidev_to_os(sc->osdev);

    /* Locks & Mutexes */
    adf_os_spinlock_init(&sc->lock_irq);
    adf_os_init_mutex(&sc->lock_mtx);

    /* Descriptor Allocation*/
    __hif_pci_setup_rxdesc(sc, HIF_PCI_MAX_RX_DESC);
    __hif_pci_setup_txdesc(sc, HIF_PCI_MAX_TX_DESC);

    /* Latency timer = 128 cycles*/
    adf_os_pci_config_write8(sc->osdev, __PCI_LATENCY_TIMER, 0x80);
    adf_os_pci_config_write8(sc->osdev, __PCI_CACHE_LINE_SIZE, 0x08);

    adf_os_pci_config_write8(sc->osdev, __PCI_MAGPIE_RETRY_COUNT, 0x0);

    return sc;
}
/**
 * @brief Device detach handler
 * 
 * @param hdl
 */
void       
hif_pci_detach(adf_drv_handle_t hdl)
{
    hif_pci_softc_t  *sc = hif_pci_sc(hdl);
    a_uint32_t i;

    adf_os_print("HIF PCI Detached\n");

    if (hif_pci_debug)
        adf_os_timer_cancel(&sc->poll);

    __hif_pci_put_reset(sc);
    __hif_pci_tgt_reset(sc); 

    __hif_pci_free_intr(sc);

    __hif_pci_flush_engine(sc);

    __hif_pci_free_rxdesc(sc);
    __hif_pci_free_txdesc(sc);

    for (i = 0; i < HIF_PCI_MAX_DEVS; i++) {
        sc = dev_found[i];
        if(!sc)
            break;
        adf_os_mem_free(sc);
    }
}

/**
 * @brief Device Suspend handler
 * 
 * @param hdl
 * @param pm
 */
void       
hif_pci_suspend(adf_drv_handle_t hdl, adf_os_pm_t pm)
{
    return;
}

/**
 * @brief Device resume handler
 * 
 * @param hdl
 */
void       
hif_pci_resume(adf_drv_handle_t hdl)
{
    return;
}
#endif

/**
 * @brief HIF init handler
 * 
 * @return a_status_t
 */
a_status_t
hif_pci_init(void)
{
//    return adf_os_pci_drv_reg(&drv_info);
    adf_os_print("hif_pci_init\n");
    return 0;
}

/**
 * @brief HIF exit handler
 */
void
hif_pci_exit(void)
{
//    adf_os_pci_drv_unreg("hif_pci");
    adf_os_print("hif_pci unloaded.\n");
}

//void
//hif_pci_dev_attach(void)
//{
//    hif_pci_softc_t   *sc;
//    a_uint32_t         i = 0;
//    
//    for (i = 0; i < HIF_PCI_MAX_DEVS; i++) {
//        sc = dev_found[i];
//        if(!sc)
//            break;
//        app_dev_inserted(hif_cb[next_app].dev, sc);
//    }
//}

void
hif_pci_dev_attach(void)
{
    hif_pci_softc_t *sc = dev_found[0];

    app_dev_inserted(hif_cb[next_app].dev, sc);
}

a_status_t
hif_boot_start(HIF_HANDLE  hdl)
{
//    hif_pci_softc_t *sc = hif_pci_sc(hdl);
    hif_pci_softc_t *sc = (hif_pci_softc_t *) hdl;

    __hif_pci_setup_intr(sc);

    if (hif_pci_debug)
        __hif_pci_setup_poll(sc);

    __hif_pci_reset(sc); 

    sc->cur_app  = APP_FWD;
    
    adf_os_mutex_acquire(&sc->lock_mtx);
    
    return A_STATUS_OK;
}

void *HIFInit(adf_os_handle_t os_hdl)
{
    hif_pci_softc_t *hif_dev;

    /* allocate memory for HIF_DEVICE */
    hif_dev = (hif_pci_softc_t *) adf_os_mem_alloc(os_hdl, sizeof(hif_pci_softc_t));  
    if (hif_dev == NULL) {
        return NULL;
    }

    adf_os_mem_zero(hif_dev, sizeof(hif_pci_softc_t));

    hif_dev->os_hdl = os_hdl;

    //*((u_int32_t *) os_hdl) = (u_int32_t) hif_dev;
    os_hdl->host_hif_handle = hif_dev;

    /* Save the hif_pic_softc */
    dev_found[0] = hif_dev;

    /* Softc book keeping*/
    hif_dev->osdev         = os_hdl;
    hif_dev->cur_app       = APP_FWD;

    /* Locks & Mutexes */
    adf_os_spinlock_init(&hif_dev->lock_irq);
    adf_os_init_mutex(&hif_dev->lock_mtx);

    /* Descriptor Allocation*/
    __hif_pci_setup_rxdesc(hif_dev, HIF_PCI_MAX_RX_DESC);
    __hif_pci_setup_txdesc(hif_dev, HIF_PCI_MAX_TX_DESC);

    return hif_dev;
}

void HIF_PCIDeviceInserted(adf_os_handle_t os_hdl)
{
    void *hHIF = NULL;

    hHIF = HIFInit(os_hdl);

    /* Inform HTC */
//    if ((hHIF != NULL) && htcDrvRegCallbacks.deviceInsertedHandler) {
//        htcDrvRegCallbacks.deviceInsertedHandler(hHIF, os_hdl);
//    }
}

/**
 * @brief Remove the FWD association from the HIF, this will be
 *        called on per device basis for which download's are
 *        over
 * 
 * @param hdl
 */
void
hif_boot_done(HIF_HANDLE  hdl)
{
//    hif_pci_softc_t  *sc = hif_pci_sc(hdl);
    hif_pci_softc_t *sc = (hif_pci_softc_t *) hdl;

    __hif_pci_flush_engine(sc);

    sc->cur_app = APP_HTC;

    adf_os_mutex_release(&sc->lock_mtx);
}

/**
 * @brief HIF registration interface
 * 
 * @param dev_cb
 * 
 * @return hif_status_t
 */
hif_status_t
HIF_register(HTC_DRVREG_CALLBACKS *dev_cb)
{
    hif_cb[next_app].dev = *dev_cb;

    hif_pci_dev_attach();
    next_app ++;

    return HIF_OK;
}

/**
 * @brief Used to register the TX Completion & RX pkt handler
 *        routines for the Application
 * 
 * @param HIFHandle
 * @param hHTC
 * @param app_cb
 */
void 
HIFPostInit(void *HIFHandle, void *hHTC, HTC_CALLBACKS *app_cb)
{
//    hif_pci_softc_t *sc = hif_pci_sc(HIFHandle);
    hif_pci_softc_t *sc = (hif_pci_softc_t *) HIFHandle;

    sc->cb[sc->next_app].pkt     = *app_cb;
    sc->cb[sc->next_app].app_ctx = hHTC;
    sc->next_app++;
}

/**
* @brief Start the HIF interface
* 
* @param hHIF
 */
void 
HIFStart(HIF_HANDLE hHIF)
{
//    hif_pci_softc_t  *sc = hif_pci_sc(hHIF);
    hif_pci_softc_t  *sc = (hif_pci_softc_t  *) hHIF;

    adf_os_print("Check for boot over");
    
    adf_os_mutex_acquire(&sc->lock_mtx); 

    adf_os_print(".. done\n");

    __hif_pci_tgt_reset(sc);
    
    return;    
}

/**
 * @brief Return control channel's corresponding pipe number
 * 
 * @param HIFHandle
 * @param tx_pipe
 * @param rx_pipe
 */
void 
HIFGetDefaultPipe(void *HIFHandle, a_uint8_t *tx_pipe, a_uint8_t *rx_pipe)
{
    *tx_pipe = HIF_PIPE_TX0;
    *rx_pipe = HIF_PIPE_RX0;
}

/**
 * @brief return MAX TX channels supported
 * 
 * @return a_uint8_t
 */
a_uint8_t 
HIFGetULPipeNum(void)
{
    return HIF_PIPE_TX_MAX;
}

/**
 * @brief Return MAX RX Channels supported
 * 
 * @return a_uint8_t
 */
a_uint8_t 
HIFGetDLPipeNum(void)
{
    return HIF_PIPE_RX_MAX;
}

void HIFEnableFwRcv(HIF_HANDLE hHIF)
{
    return;
}

/**
 * @brief Shutdown the interface
 * 
 * @param HIFHandle
 */
void 
HIFShutDown(void *HIFHandle)
{
    /**
     * XXX: put the target into the reset
     */
    adf_os_print("Shutting the interface\n"); 

    return;
}

/**
 * @brief Application deregisteration
 * 
 * @return A_STATUS
 */
A_STATUS 
HIF_deregister(void)
{
    hif_pci_softc_t *sc = NULL;

    adf_os_print("HIF PCI deregistered \n");

    sc = dev_found[0];
    app_dev_removed(hif_cb[sc->cur_app].dev, 
                    sc->cb[sc->cur_app].app_ctx);

    return A_OK;
}

void       
hif_pci_detach(void)
{
    hif_pci_softc_t  *sc = dev_found[0];

    adf_os_print("HIF PCI Detached\n");

    if (hif_pci_debug)
        adf_os_timer_cancel(&sc->poll);

    __hif_pci_put_reset(sc);
    __hif_pci_tgt_reset(sc); 

    __hif_pci_free_intr(sc);

    __hif_pci_flush_engine(sc);

    __hif_pci_free_rxdesc(sc);
    __hif_pci_free_txdesc(sc);

    adf_os_mem_free(sc);
}

void HIF_PCIDeviceDetached(adf_os_handle_t os_hdl)
{
    /* Dereigster HIF */
    HIF_deregister();

    /* Invoke the PCI detach function for recycle resources */
    hif_pci_detach();    
}

a_uint16_t HIFGetMaxQueueNumber(HIF_HANDLE hHIF, a_uint8_t PipeID)
{
    return HIF_PCI_MAX_TX_DESC;
}

/*****************************************************************************/
/************************** Debugging Functions ******************************/
/*****************************************************************************/

/**
 * @brief this will print the nbuf
 * @param buf
 */
/* void */
/* __hif_pci_print_buf(adf_nbuf_t buf) */
/* { */
/*     adf_os_sglist_t  sg = {0}; */
/*     int i, j, min; */
/*  */
/*     adf_nbuf_frag_info(buf, &sg); */
/*  */
/*     for(i = 0; i < sg.nsegs; i++){ */
/*         min = adf_os_min(sg.sg_segs[i].len, PRINT_BUF_LIM); */
/*         min = sg.sg_segs[i].len; */
/*         for(j = 0; j < min; j++) */
/*             adf_os_print("%x ", sg.sg_segs[i].vaddr[j]); */
/*     } */
/*     adf_os_print("\n"); */
/* } */
/**
 * @brief fill the buffer with sym
 * 
 * @param buf
 * @param sym
 */
/* void */
/* __hif_pci_fill_buf(adf_nbuf_t  buf, char sym) */
/* { */
/*     adf_os_sglist_t  sg = {0}; */
/*     int i, j; */
/*  */
/*     adf_nbuf_frag_info(buf, &sg); */
/*  */
/*     for(i = 0; i < sg.nsegs; i++){ */
/*         for(j = 0; j < sg.sg_segs[i].len; j++) */
/*             sg.sg_segs[i].vaddr[j] = sym; */
/*     } */
/* } */
/**
 * @brief For Debugging, Print the Descriptor geometry
 * 
 * @param head
 */
/* void */
/* __hif_pci_print_desc(pci_dma_softc_t  *dma_q) */
/* { */
/*     struct zsDmaDesc  *desc; */
/*     zdma_swdesc_t  *head =  descq->head); */
/*     int i = 0; */
/*  */
/*     do { */
/*         i ++; */
/*         desc = &head->hw_desc_buf; */
/*         adf_os_print("{"); */
/*         adf_os_print("lAddr = %#x ",(a_uint32_t)desc->lastAddr); */
/*         adf_os_print("nAddr = %#x ",(a_uint32_t)desc->nextAddr); */
/*         adf_os_print("dAddr = %#x ",desc->dataAddr); */
/*         adf_os_print("status = %#x ",desc->status); */
/*         adf_os_print("ctrl = %#x ",desc->ctrl); */
/*         adf_os_print("Len = %#x ",desc->totalLen); */
/*         adf_os_print("dlen = %#x ",desc->dataSize); */
/*         adf_os_print("next_desc = %#x ",(a_uint32_t)head->next_desc); */
/*         adf_os_print("buf_addr = %#x ",(a_uint32_t)head->buf_addr); */
/*         adf_os_print("buf_size = %#x ",head->buf_size); */
/*         adf_os_print("nbuf = %#x ",(a_uint32_t)head->nbuf); */
/*         adf_os_print("hw_desc_paddr = %#x ",head->hw_desc_paddr); */
/*         adf_os_print("hw_desc_buf = %#x ",(a_uint32_t)desc); */
/*         adf_os_print("}\n"); */
/*  */
/*         head = head->next_desc; */
/*     }while(head); */
/*  */
/* } */
/**
 * @brief Setup the descriptors of engines in the H/W Dp(s)
 * 
 * @param sc
 */
/* void */
/* __hif_pci_print_rxeng(hif_pci_softc_t   *sc) */
/* { */
/*     a_uint32_t  i = 0, addr = 0; */
/*  */
/*     for (i = 0; i < MAX_PCI_DMAENG_RX; i++) { */
/*         addr = eng_tbl[i].eng_off; */
/*  */
/*         printk("RXDP[%d] = %#x\t",i, */
/*                 adf_os_reg_read32(sc->osdev, addr + __DMA_REG_RXDESC)); */
/*         printk("RXCE[%d] = %#x\n",i, */
/*                 adf_os_reg_read32(sc->osdev, addr + __DMA_REG_RXSTART)); */
/*     } */
/*  */
/* } */
/**
 * @brief Setup the descriptors of engines in the H/W Dp(s)
 * 
 * @param sc
 */
/* void */
/* __hif_pci_print_txeng(hif_pci_softc_t   *sc) */
/* { */
/*     a_uint32_t  i = 0, addr = 0; */
/*  */
/*     for (i = __DMA_ENGINE_TX0; i < __DMA_ENGINE_MAX; i++) { */
/*         addr = eng_tbl[i].eng_off; */
/*  */
/*         printk("TXDP[%d] = %#x\n", i, */
/*                 adf_os_reg_read32(sc->osdev, addr + __DMA_REG_TXDESC)); */
/*     } */
/* } */
/**
 * @brief This used to schedule the packets based on TCP port 
 *        numbers
 */ 
/* __dma_eng_t */
/* __hif_pci_port_mapper(adf_nbuf_t  eth_nbuf, __dma_eng_t  eng_no) */
/* { */
/*     a_uint16_t  sport = 0; */
/*  */
/*     if (eth_nbuf->len > 1200) { */
/*         sport = adf_os_ntohs(*(a_uint16_t *)&eth_nbuf->data[78]); */
/*         switch(sport) { */
/*             case SPORT_TO_SNIFF: */
/*                 eng_no = __DMA_ENGINE_TX1; */
/*                 break; */
/*             case (SPORT_TO_SNIFF + 1): */
/*                 eng_no = __DMA_ENGINE_TX2; */
/*                 break; */
/*             case (SPORT_TO_SNIFF + 2): */
/*                 eng_no = __DMA_ENGINE_TX3; */
/*                 break; */
/*             default: */
/*                 break; */
/*         } */
/*     } */
/*  */
/*     return eng_no; */
/* } */
/**
 * @brief For debugging purpose
 */ 
/* void */
/* hif_pci_set_dbginfo(a_uint32_t value) */
/* { */
/*     hif_pci_dbg_info ^= 0x01; */
/* } */
/* adf_os_export_symbol(hif_pci_set_dbginfo); */



adf_os_export_symbol(HIF_register);
adf_os_export_symbol(HIF_deregister);

adf_os_export_symbol(HIFGetDefaultPipe);
adf_os_export_symbol(HIFStart);
adf_os_export_symbol(hif_boot_start);
adf_os_export_symbol(hif_boot_done);
adf_os_export_symbol(HIFPostInit);
adf_os_export_symbol(HIFSend);
adf_os_export_symbol(HIFShutDown);
adf_os_export_symbol(HIFGetULPipeNum);
adf_os_export_symbol(HIFGetDLPipeNum);
adf_os_export_symbol(HIFEnableFwRcv);
adf_os_export_symbol(HIFGetFreeQueueNumber);
adf_os_export_symbol(hif_recv);
adf_os_export_symbol(HIF_PCIDeviceInserted);
adf_os_export_symbol(HIF_PCIDeviceDetached);
adf_os_export_symbol(HIFGetMaxQueueNumber);

adf_os_declare_param(hif_pci_intm, ADF_OS_PARAM_TYPE_UINT32);
adf_os_declare_param(hif_pci_debug, ADF_OS_PARAM_TYPE_UINT32);
adf_os_declare_param(pci_prn_mask, ADF_OS_PARAM_TYPE_UINT32);

//adf_os_pci_module_init(hif_pci_init);
//adf_os_pci_module_exit(hif_pci_exit);
