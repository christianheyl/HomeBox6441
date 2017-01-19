/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#ifndef REMOVE_PKT_LOG
#include "adf_os_mem.h"
#include "athdefs.h"
#include <pktlog_ac_i.h>

static struct ath_pktlog_info *g_pktlog_info = NULL;
static int g_pktlog_mode = PKTLOG_MODE_SYSTEM;

void pktlog_init(struct ol_ath_softc_net80211 *scn);
int pktlog_enable(struct ol_ath_softc_net80211 *scn, int32_t log_state);
int pktlog_setsize(struct ol_ath_softc_net80211 *scn, int32_t log_state);
int pktlog_disable(struct ol_ath_softc_net80211 *scn);
//extern void pktlog_disable_adapter_logging(void);
//extern void pktlog_disable_adapter_logging(void);
//extern int pktlog_alloc_buf(ol_ath_generic_softc_handle sc,
 //                                struct ath_pktlog_info *pl_info);
//void pktlog_release_buf(struct ath_pktlog_info *pl_info);
//extern void pktlog_release_buf(void *pinfo);

wdi_event_subscribe PKTLOG_TX_SUBSCRIBER;
wdi_event_subscribe PKTLOG_RX_SUBSCRIBER;
wdi_event_subscribe PKTLOG_RX_REMOTE_SUBSCRIBER;
wdi_event_subscribe PKTLOG_RCFIND_SUBSCRIBER;
wdi_event_subscribe PKTLOG_RCUPDATE_SUBSCRIBER;
wdi_event_subscribe PKTLOG_DBG_PRINT_SUBSCRIBER;

struct ol_pl_arch_dep_funcs ol_pl_funcs = {
    .pktlog_init = pktlog_init,
    .pktlog_enable = pktlog_enable,
    .pktlog_setsize = pktlog_setsize,
    .pktlog_disable = pktlog_disable, //valid for f/w disable
    /*.pktlog_detach = pktlog_detach,
    .pktlog_text = pktlog_text,
    .pktlog_start = pktlog_start,
    .pktlog_readhdr = pktlog_read_hdr,
    .pktlog_readbuf = pktlog_read_buf,*/
};

struct ol_pktlog_dev_t ol_pl_dev = {
    .pl_funcs = &ol_pl_funcs,
};

void ol_pl_sethandle(ol_pktlog_dev_handle *pl_handle,
                    struct ol_ath_softc_net80211 *scn)
{
    ol_pl_dev.scn = scn;
    *pl_handle = &ol_pl_dev;
}

static inline A_STATUS pktlog_enable_tgt(
        struct ol_ath_softc_net80211 *_scn,
        uint32_t log_state)
{
    uint32_t types = 0;
    if(log_state & ATH_PKTLOG_TX) {
        types |= WMI_PKTLOG_EVENT_TX;
        }
    if(log_state & ATH_PKTLOG_RX) {
        types |= WMI_PKTLOG_EVENT_RX;
    }
    if(log_state & ATH_PKTLOG_RCFIND) {
        types |= WMI_PKTLOG_EVENT_RCF;
    }
    if(log_state & ATH_PKTLOG_RCUPDATE) {
        types |= WMI_PKTLOG_EVENT_RCU;
    }
    if(log_state & ATH_PKTLOG_DBG_PRINT) {
        types |= WMI_PKTLOG_EVENT_DBG_PRINT;
    }

    if(pktlog_wmi_send_cmd(_scn, types,
                              WMI_PDEV_PKTLOG_ENABLE_CMDID)) {
            return A_ERROR;
    }
    return A_OK;
}

static inline A_STATUS wdi_pktlog_subscribe(
        struct ol_txrx_pdev_t *txrx_pdev, int32_t log_state)
{
    if (!txrx_pdev) {
        printk("Invalid pdev in %s\n", __FUNCTION__);
        return A_ERROR;
    }
    if (log_state & ATH_PKTLOG_TX) {
        if(wdi_event_sub(txrx_pdev,
                    &PKTLOG_TX_SUBSCRIBER,
                    WDI_EVENT_TX_STATUS)) {
            return A_ERROR;
        }
    }
    if (log_state & ATH_PKTLOG_RX) {
        if(wdi_event_sub(txrx_pdev,
                    &PKTLOG_RX_SUBSCRIBER,
                    WDI_EVENT_RX_DESC)) {
            return A_ERROR;
        }
        if(wdi_event_sub(txrx_pdev,
                    &PKTLOG_RX_REMOTE_SUBSCRIBER,
                    WDI_EVENT_RX_DESC_REMOTE)) {
            return A_ERROR;
        }
    }
    if (log_state & ATH_PKTLOG_RCFIND) {
        if(wdi_event_sub(txrx_pdev,
                    &PKTLOG_RCFIND_SUBSCRIBER,
                    WDI_EVENT_RATE_FIND)) {
            return A_ERROR;
        }
    }
    if (log_state & ATH_PKTLOG_RCUPDATE) {
        if(wdi_event_sub(txrx_pdev,
                    &PKTLOG_RCUPDATE_SUBSCRIBER,
                    WDI_EVENT_RATE_UPDATE)) {
            return A_ERROR;
        }
    }
    if (log_state & ATH_PKTLOG_DBG_PRINT) {
        if(wdi_event_sub(txrx_pdev,
                    &PKTLOG_DBG_PRINT_SUBSCRIBER,
                    WDI_EVENT_DBG_PRINT)) {
            return A_ERROR;
        }
    }
    return A_OK;
}

void
pktlog_callback(void *pdev, enum WDI_EVENT event, void *log_data)
{
    switch(event) {
        case WDI_EVENT_TX_STATUS: 
        {
            /*
             * process TX message
             */
            if(process_tx_info(pdev, log_data)) {
                printk("Unable to process TX info\n");
                return;
            }
            break; 
        }
        case WDI_EVENT_RX_DESC: 
        {
            /*
             * process RX message for local frames
             */
            if(process_rx_info(pdev, log_data)) {
                printk("Unable to process RX info\n");
                return;
            }
           break; 
        }
        case WDI_EVENT_RX_DESC_REMOTE: 
        {
             /*
              * process RX message for remote frames
              */
            if(process_rx_info_remote(pdev, log_data)) {
                printk("Unable to process RX info\n");
                return;
            }
           break; 
        } 
        case WDI_EVENT_RATE_FIND: 
        {
            /*
             * process RATE_FIND message
             */
            if(process_rate_find(pdev, log_data)) {
                printk("Unable to process RC_FIND info\n");
                return;
            }
           break; 
        }
        case WDI_EVENT_RATE_UPDATE: 
        {
            /*
             * process RATE_UPDATE message
             */
            if(process_rate_update(pdev, log_data)) {
                adf_os_print("Unable to process RC_UPDATE\n");
                return;
            }
           break; 
        }
        case WDI_EVENT_DBG_PRINT:
        {
            /*
             * Process DBG prints
             */
            if (process_dbg_print(pdev, log_data)) {
                adf_os_print("Unable to process DBG_PRINT\n");
                return;
            }
            break;
        }
        default:
            break;
    }
}

static inline A_STATUS
wdi_pktlog_unsubscribe(struct ol_txrx_pdev_t *txrx_pdev, uint32_t log_state)
{
    if(log_state & ATH_PKTLOG_TX) {
        if(wdi_event_unsub(
                    txrx_pdev,
                    &PKTLOG_TX_SUBSCRIBER,
                    WDI_EVENT_TX_STATUS)) {
            return A_ERROR;
        }
    }
    if(log_state & ATH_PKTLOG_RX) {
        if(wdi_event_unsub(
                    txrx_pdev,
                    &PKTLOG_RX_SUBSCRIBER,
                    WDI_EVENT_RX_DESC)) {
            return A_ERROR;
        }
        if(wdi_event_unsub(
                    txrx_pdev,
                    &PKTLOG_RX_REMOTE_SUBSCRIBER,
                    WDI_EVENT_RX_DESC_REMOTE)) {
            return A_ERROR;
        }
    }
    if(log_state & ATH_PKTLOG_RCFIND) {
        if(wdi_event_unsub(
                    txrx_pdev,
                    &PKTLOG_RCFIND_SUBSCRIBER,
                    WDI_EVENT_RATE_FIND)) {
            return A_ERROR;
        } 
    }
    if(log_state & ATH_PKTLOG_RCUPDATE) {
        if(wdi_event_unsub(
                    txrx_pdev,
                    &PKTLOG_RCUPDATE_SUBSCRIBER,
                    WDI_EVENT_RATE_UPDATE)) {
            return A_ERROR;
        }
    }
    if (log_state & ATH_PKTLOG_DBG_PRINT) {
        if(wdi_event_unsub(
                    txrx_pdev,
                    &PKTLOG_DBG_PRINT_SUBSCRIBER,
                    WDI_EVENT_DBG_PRINT)) {
            return A_ERROR;
        }
    }
    return A_OK;
}

int
pktlog_wmi_send_cmd(struct ol_ath_softc_net80211 *scn,
        WMI_PKTLOG_EVENT PKTLOG_EVENT, WMI_CMD_ID CMD_ID)
{
    wmi_pdev_pktlog_enable_cmd *cmd;
    int len = 0;
    wmi_buf_t buf;

    switch(CMD_ID) {
        case WMI_PDEV_PKTLOG_ENABLE_CMDID: 
            len = sizeof(wmi_pdev_pktlog_enable_cmd);
            buf = wmi_buf_alloc(scn->wmi_handle, len);
            if (!buf) {
                printk("%s:wmi_buf_alloc failed\n", __FUNCTION__);
                goto wmi_send_failed;
            }
            cmd = (wmi_pdev_pktlog_enable_cmd *)wmi_buf_data(buf);
            cmd->evlist = PKTLOG_EVENT;
            if(!wmi_unified_cmd_send(scn->wmi_handle, buf, len,
                        WMI_PDEV_PKTLOG_ENABLE_CMDID)) {
                goto wmi_send_passed; 
            }
        case WMI_PDEV_PKTLOG_DISABLE_CMDID:
            /*
             * No command data for _pktlog_disable
             * len = 0
             */
            buf = wmi_buf_alloc(scn->wmi_handle, 0);
            if (!buf) {
                printk("%s:wmi_buf_alloc failed\n", __FUNCTION__);
                goto wmi_send_failed;
            }
            if(!wmi_unified_cmd_send(scn->wmi_handle, buf, len,
                        WMI_PDEV_PKTLOG_DISABLE_CMDID)) {
                goto wmi_send_passed; 
            }
        default:
            break;
    }

wmi_send_passed:
    return 0;

wmi_send_failed:
    return -1;
}

int
pktlog_disable(struct ol_ath_softc_net80211 *scn)
{
    return PKTLOG_DISABLE(scn);
}

void
pktlog_init(struct ol_ath_softc_net80211 *scn)
{
    struct ath_pktlog_info *pl_info = (scn) ?
                                        scn->pl_dev->pl_info : g_pktlog_info;
    OS_MEMZERO(pl_info, sizeof(*pl_info));
    
    PKTLOG_LOCK_INIT(pl_info);

    pl_info->buf_size = PKTLOG_DEFAULT_BUFSIZE;
    pl_info->buf = NULL;
    pl_info->log_state = 0;
    pl_info->sack_thr = PKTLOG_DEFAULT_SACK_THR;
    pl_info->tail_length = PKTLOG_DEFAULT_TAIL_LENGTH;
    pl_info->thruput_thresh = PKTLOG_DEFAULT_THRUPUT_THRESH;
    pl_info->per_thresh = PKTLOG_DEFAULT_PER_THRESH;
    pl_info->phyerr_thresh = PKTLOG_DEFAULT_PHYERR_THRESH;
    pl_info->trigger_interval = PKTLOG_DEFAULT_TRIGGER_INTERVAL;
	pl_info->pktlen = 0;
	pl_info->start_time_thruput = 0;
	pl_info->start_time_per = 0;
    PKTLOG_TX_SUBSCRIBER.callback = pktlog_callback;
    PKTLOG_RX_SUBSCRIBER.callback = pktlog_callback;
    PKTLOG_RX_REMOTE_SUBSCRIBER.callback = pktlog_callback;
    PKTLOG_RCFIND_SUBSCRIBER.callback = pktlog_callback;
    PKTLOG_RCUPDATE_SUBSCRIBER.callback = pktlog_callback;
    PKTLOG_DBG_PRINT_SUBSCRIBER.callback = pktlog_callback;
    printk("Initializing Pktlogs for 11ac\n");
    
    /* 
     * Add the WDI subscribe command
     * Might be moved to enable function because,
     * it is not consuming the WDI unless we really need it.
     */

}

int
pktlog_enable(struct ol_ath_softc_net80211 *scn, int32_t log_state)
{
    struct ol_pktlog_dev_t *pl_dev = scn->pl_dev;
    struct ath_pktlog_info *pl_info = (scn) ?
                                        pl_dev->pl_info : g_pktlog_info;
    struct ol_txrx_pdev_t *txrx_pdev = scn->pdev_txrx_handle;
    int error;

    pl_dev->sc_osdev = scn->sc_osdev;
    if (!pl_info) {
        return 0;
    }

    if (log_state != 0 && !pl_dev->tgt_pktlog_enabled) {
        if (!scn) {
            if (g_pktlog_mode == PKTLOG_MODE_ADAPTER) {
                pktlog_disable_adapter_logging();
                g_pktlog_mode = PKTLOG_MODE_SYSTEM;
            }
        }
        else {
            if (g_pktlog_mode == PKTLOG_MODE_SYSTEM) {
                g_pktlog_mode = PKTLOG_MODE_ADAPTER;
            }
        } 

        if (pl_info->buf == NULL) {
            error = pktlog_alloc_buf(scn);
            if (error != 0)
                return error;
                
            pl_info->buf->bufhdr.version = CUR_PKTLOG_VER;
            pl_info->buf->bufhdr.magic_num = PKTLOG_MAGIC_NUM;
            pl_info->buf->wr_offset = 0;
            pl_info->buf->rd_offset = -1;
        }
	    pl_info->start_time_thruput = OS_GET_TIMESTAMP();
	    pl_info->start_time_per = pl_info->start_time_thruput;

        /* WDI subscribe */
        if(wdi_pktlog_subscribe(txrx_pdev, log_state)) {
            printk("Unable to subscribe to the WDI %s\n",
                    __FUNCTION__);
            return -1;
        }
        /* WMI command to enable pktlog on the firmware */
        if(pktlog_enable_tgt(scn, log_state)) {
            printk("Device cannot be enabled, %s\n", __FUNCTION__);
            return -1;
        }
        else {
            pl_dev->tgt_pktlog_enabled = true;
        }
    }
    else if(!log_state && pl_dev->tgt_pktlog_enabled) {
        pl_dev->pl_funcs->pktlog_disable(scn);
        pl_dev->tgt_pktlog_enabled = false;
        if(wdi_pktlog_unsubscribe(txrx_pdev, pl_info->log_state)) {
            printk("Cannot unsubscribe pktlog from the WDI\n");
            return -1;
        }
    }

    pl_info->log_state = log_state;
    return 0;
}

int
pktlog_setsize(struct ol_ath_softc_net80211 *scn, int32_t size)
{
    struct ol_pktlog_dev_t *pl_dev = scn->pl_dev;
    struct ath_pktlog_info *pl_info = (scn) ?
                                        pl_dev->pl_info : g_pktlog_info;

    printk("I am setting the size of the pktlog buffer:%s\n", __FUNCTION__);
    if (size < 0)
        return -EINVAL;

    if (size == pl_info->buf_size)
        return 0;

    if (pl_info->log_state) {
        printk("Logging should be disabled before changing bufer size\n");
        return -EINVAL;
    }

    if (pl_info->buf != NULL)
        pktlog_release_buf(scn);

    if (size != 0)
        pl_info->buf_size = size;

    return 0;
}

#endif /* REMOVE_PKT_LOG */
