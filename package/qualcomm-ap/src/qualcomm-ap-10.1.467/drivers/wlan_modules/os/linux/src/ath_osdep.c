/*
 * Copyright (c) 2009, Atheros Communications Inc.
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
 *
 *  This module contains an interface/shim layer between the HW layer and our 
 *  ath layer.
 */

#include "ath_internal.h"
#include <osdep.h>
#include <wbuf.h>


#include "ath_internal.h"
#include "version.h"
#include "if_athvar.h"
#include <linux/proc_fs.h>
#include "osif_private.h"

#include <_ieee80211.h>

#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

#if ATH_DEBUG
int ACBKMinfree = 48;
int ACBEMinfree = 32;
int ACVIMinfree = 16;
int ACVOMinfree = 0;
int CABMinfree = 48;
int UAPSDMinfree = 0;
#endif

struct ath_softc_net80211 *global_scn[10];
int num_global_scn = 0;

/*
 * Indication of hardware PHY state change
 */
void
ath_hw_phystate_change(struct ath_softc *sc, int newstate)
{
    if (sc->sc_hw_phystate == newstate)
        return;

    sc->sc_hw_phystate = newstate;  /* save the hardware switch state */

#ifdef ATH_SUPPORT_LINUX_STA
    /* TODO: rfkill support in Linux*/
    if (newstate)
    {	
        ath_radio_enable(sc);
        sc->sc_ieee_ops->ath_net80211_resume(sc->sc_ieee);
    }	
    else
    {
        sc->sc_ieee_ops->ath_net80211_suspend(sc->sc_ieee);
        ath_radio_disable(sc);
    }	
#endif
}

/* 
 * XXX : This is temp export, testing the reset hack
 *
 */
void ath_internal_reset(struct ath_softc *sc)
{
#if ATH_SUPPORT_FLOWMAC_MODULE
    if (sc->sc_osnetif_flowcntrl) {
        ath_netif_stop_queue(sc);
    }
#endif
    if ( (sc->sc_reset_type ==  ATH_RESET_NOLOSS) ){
        /* setting the no-flush, to preserve the packets already 
         * there in SW and HW queues. */
        ath_reset_start(sc, RESET_RETRY_TXQ, 0, 0);
        /*
         * when we pass RESET_RETRY_TXQ flag to ath_reset_start, 
         * ath_reset() will be called inside ath_reset_start().
         * So ath_reset() is commented out here for now. 
         */
        //ath_reset(sc);
        ath_reset_end(sc, RESET_RETRY_TXQ);
    } else {
        ath_reset_start(sc, 0, 0, 0);
        ath_reset(sc);
        ath_reset_end(sc, 0);
    }

#if ATH_SUPPORT_FLOWMAC_MODULE
    if (sc->sc_osnetif_flowcntrl) {
        ath_netif_wake_queue(sc);
    }
#endif
}
EXPORT_SYMBOL(ath_internal_reset);

void ath_handle_tx_intr(struct ath_softc *sc)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(sc->sc_ieee);

    scn->sc_ops->tx_proc(sc);
}

void ath_handle_rx_intr(struct ath_softc *sc)
{
    struct ath_softc_net80211 *scn = ATH_SOFTC_NET80211(sc->sc_ieee);

    scn->sc_ops->rx_proc(scn->sc_dev, 0 /*flush*/);
}

/*
 * Linux's specific rx indicator
 * 
 * In Linux, the wbuf indicated to upper stack won't be returned to us.
 * So we have to allocate a new one and queue it by ourselves.
 */
#ifndef ATH_SUPPORT_HTC
#if USE_MULTIPLE_BUFFER_RCV
static int ath_rx_assemble_buf(struct ath_softc *sc, wbuf_t *wbuf, 
                        ieee80211_rx_status_t *status, 
                        u_int16_t keyix) 
{

    struct ath_buf *bf = NULL;
    struct sk_buff *kb = NULL; 

    wbuf_t nwbuf;

    if (wbuf_next(*wbuf)) { /* for linux multiple receive buffer */


          bf = ATH_GET_RX_CONTEXT_BUF(wbuf_next(*wbuf));
          bf->bf_mpdu = wbuf_next(*wbuf);
          /* unlink the top half buffer and bottom half buffer */
          wbuf_setnextpkt(*wbuf, NULL); 
          /* use skb_copy_expand for combining two buffer, nwbuf = wbuf##twbuf */
          nwbuf = skb_copy_expand((struct sk_buff *)*wbuf, 
                  0, wbuf_get_pktlen(*wbuf)+wbuf_get_pktlen(bf->bf_mpdu),
                  GFP_ATOMIC);

          if (nwbuf != NULL) {
              skb_copy_bits(bf->bf_mpdu, 0, 
                      wbuf_header(nwbuf) + wbuf_get_pktlen(*wbuf), 
                      wbuf_get_pktlen(bf->bf_mpdu));
              kb = (struct sk_buff *)nwbuf;
              ((struct ieee80211_cb *)kb->cb)->context =
                       &(((struct ieee80211_cb *)kb->cb)[1]);
              skb_put(nwbuf,wbuf_get_pktlen(bf->bf_mpdu));

              wbuf_free(bf->bf_mpdu); 
              bf->bf_mpdu = nwbuf;
              ATH_SET_RX_CONTEXT_BUF(nwbuf, bf);
         } else {

              ATH_GET_RX_CONTEXT_BUF(*wbuf)->bf_status |= ATH_BUFSTATUS_FREE; 

              if (sc->sc_enhanceddmasupport) {
                wbuf_push(*wbuf, sc->sc_rxstatuslen);
                wbuf_push(bf->bf_mpdu, sc->sc_rxstatuslen);
              }

              wbuf_trim(bf->bf_mpdu, wbuf_get_pktlen(bf->bf_mpdu));
              wbuf_trim(*wbuf, wbuf_get_pktlen(*wbuf));

              bf->bf_buf_addr[0] = wbuf_map_single(sc->sc_osdev, 
                                                   bf->bf_mpdu, 
                                                   BUS_DMA_FROMDEVICE, 
                                                   OS_GET_DMA_MEM_CONTEXT(bf, bf_dmacontext));

              /* relink unused wbuf to H/W */
              ath_rx_requeue(sc, *wbuf);
              ath_rx_requeue(sc, bf->bf_mpdu);
              return -1;
         }

           ATH_GET_RX_CONTEXT_BUF(*wbuf)->bf_status |= ATH_BUFSTATUS_FREE; 

           if (sc->sc_enhanceddmasupport) {
              wbuf_push(*wbuf, sc->sc_rxstatuslen);
           }

           wbuf_trim(*wbuf, wbuf_get_pktlen(*wbuf));
           /* relink unused wbuf to H/W */
           ath_rx_requeue(sc, *wbuf);
           *wbuf = bf->bf_mpdu;
    }
    return 0;

}
#endif
int ath_rx_indicate(struct ath_softc *sc, wbuf_t wbuf, ieee80211_rx_status_t *status, u_int16_t keyix)
{
    struct ath_buf *bf = ATH_GET_RX_CONTEXT_BUF(wbuf);
    wbuf_t nwbuf;
    int type=-1;



#if USE_MULTIPLE_BUFFER_RCV
    
    if (ath_rx_assemble_buf(sc, &wbuf, status, keyix))
        return -1;

    bf = ATH_GET_RX_CONTEXT_BUF(wbuf);
    /* wbuf might be assign to diffrent buffer, bf should be updated. */
#endif

    /* indicate frame to the stack, which will free the old wbuf. only indicate when we can get new buffer */
    wbuf_set_next(wbuf, NULL);
#if ATH_RXBUF_RECYCLE
	/*
	 * if ATH_RXBUF_RECYCLE is enabled to recycle the skb, 
	 * do the rx_indicate before we recycle the skb to avoid 
	 * skb competition and headline block of the recycle queue.
	 */
    type = sc->sc_ieee_ops->rx_indicate(sc->sc_ieee, wbuf, status, keyix);
    if (sc->sc_osdev->rbr_ops.osdev_wbuf_recycle) {
        nwbuf = (wbuf_t)(sc->sc_osdev->rbr_ops.osdev_wbuf_recycle((void *)sc));
    }
    else
#endif
    {
    /* allocate a new wbuf and queue it to for H/W processing */
        nwbuf = ath_rxbuf_alloc(sc, sc->sc_rxbufsize);
    }
    if (nwbuf != NULL) {
#if !ATH_RXBUF_RECYCLE
        type = sc->sc_ieee_ops->rx_indicate(sc->sc_ieee, wbuf, status, keyix);
#endif
        bf->bf_mpdu = nwbuf;
		/*
		 * do not invalidate the cache for the new/recycled skb,
		 * because the cache will be invalidated in rx ISR/tasklet
		 */	
		bf->bf_buf_addr[0] = bf->bf_dmacontext = virt_to_phys(nwbuf->data);	
        ATH_SET_RX_CONTEXT_BUF(nwbuf, bf);

        /* queue the new wbuf to H/W */
        ath_rx_requeue(sc, nwbuf);
    } 
#if !ATH_RXBUF_RECYCLE
	else {
        struct sk_buff *skb;

        /* Could not allocate the buffer
         * give the wbuf back, reset the wbuf fields
		 *
		 * do not invalidate the cache for the new/recycled skb,
		 * because the cache will be invalidated in rx ISR/tasklet
		 */	
        skb = (struct sk_buff *)wbuf;
        OS_MEMZERO(skb, offsetof(struct sk_buff, tail));
        skb->data = skb->head + NET_SKB_PAD;
        skb->tail = skb->data;
        skb->dev = sc->sc_osdev->netdev; 

		bf->bf_buf_addr[0] = bf->bf_dmacontext = virt_to_phys(wbuf->data);	

        /* Set the rx context back, as it was cleared before */
        /* wbuf_set_next(wbuf, NULL) clears it */
        ATH_SET_RX_CONTEXT_BUF(wbuf, bf);

        /* queue back the old wbuf to H/W */
        ath_rx_requeue(sc, wbuf);        

        /* it's a drop, record it */
        sc->sc_stats.ast_rx_nobuf++;
    }
#endif
    return type;
}
#else
int ath_rx_indicate(struct ath_softc *sc, wbuf_t wbuf, ieee80211_rx_status_t *status, u_int16_t keyix)
{
    int type;

    /* indicate frame to the stack */
    type = sc->sc_ieee_ops->rx_indicate(sc->sc_ieee, wbuf, status, keyix);

    return type;
}
#endif

#ifdef ATH_SUPPORT_LINUX_STA
#ifdef CONFIG_SYSCTL

enum {
        ATH_RADIO_ON    = 1,
};

static int
ATH_SYSCTL_DECL(ath_sysctl_halparam, ctl, write, filp, buffer, lenp, ppos)
{
        struct ath_softc *sc = ctl->extra1;
        static u_int val;
        int ret=0;

        ctl->data = &val;
        ctl->maxlen = sizeof(val);
        if (!write) {
		switch ((long)ctl->extra2) {
			case ATH_RADIO_ON:
				if (sc->sc_hw_phystate)
					val = 1;
				else
					val = 0;
				break;
			default:
				return -EINVAL;
		}
                ret = ATH_SYSCTL_PROC_DOINTVEC(ctl, write, filp, buffer,
                                lenp, ppos);
        }
	return ret;
}


/* sysctl entries: /proc/sys/dev/wifiN/... */
static const ctl_table ath_sysctl_template[] = {
        { .ctl_name     = CTL_AUTO,
          .procname     = "radio_on",
          .mode         = 0444,
          .proc_handler = ath_sysctl_halparam,
          .extra2       = (void *)ATH_RADIO_ON,
        },
	/* may add more in the future */
        { 0 }
};

void
ath_dynamic_sysctl_register(struct ath_softc *sc)
{
        int i, space;
	char *devname = sc->sc_osdev->netdev->name ;

        space = 5*sizeof(struct ctl_table) + sizeof(ath_sysctl_template);
        sc->sc_sysctls = kmalloc(space, GFP_KERNEL);
        if (sc->sc_sysctls == NULL) {
                printk("%s: no memory for sysctl table!\n", __func__);
                return;
        }

        /* setup the table */
        memset(sc->sc_sysctls, 0, space);
        sc->sc_sysctls[0].ctl_name = CTL_DEV;
        sc->sc_sysctls[0].procname = "dev";
        sc->sc_sysctls[0].mode = 0555;
        sc->sc_sysctls[0].child = &sc->sc_sysctls[2];
        /* [1] is NULL terminator */
        sc->sc_sysctls[2].ctl_name = CTL_AUTO;
        sc->sc_sysctls[2].procname = devname ;
        sc->sc_sysctls[2].mode = 0555;
        sc->sc_sysctls[2].child = &sc->sc_sysctls[4];
        /* [3] is NULL terminator */
        /* copy in pre-defined data */
        memcpy(&sc->sc_sysctls[4], ath_sysctl_template,
                sizeof(ath_sysctl_template));

        /* add in dynamic data references */
        for (i = 4; sc->sc_sysctls[i].procname; i++)
                if (sc->sc_sysctls[i].extra1 == NULL)
                        sc->sc_sysctls[i].extra1 = sc;

        /* and register everything */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,20)
        sc->sc_sysctl_header = register_sysctl_table(sc->sc_sysctls);
#else
        sc->sc_sysctl_header = register_sysctl_table(sc->sc_sysctls, 1);
#endif
        if (!sc->sc_sysctl_header) {
                printk("%s: failed to register sysctls!\n", devname);
                kfree(sc->sc_sysctls);
                sc->sc_sysctls = NULL;
        }
}

void
ath_dynamic_sysctl_unregister(struct ath_softc *sc)
{
        if (sc->sc_sysctl_header) {
                unregister_sysctl_table(sc->sc_sysctl_header);
                sc->sc_sysctl_header = NULL;
        }
        if (sc->sc_sysctls) {
                kfree(sc->sc_sysctls);
                sc->sc_sysctls = NULL;
        }
}

EXPORT_SYMBOL(ath_dynamic_sysctl_register);
EXPORT_SYMBOL(ath_dynamic_sysctl_unregister);

#endif /* CONFIG_SYSCTL */
#endif /* ATH_SUPPORT_LINUX_STA */


static struct proc_dir_entry *parent_entry;
static struct proc_dir_entry *parent_version_entry;
unsigned long ath_ioctl_debug = 0;      /* for run-time debugging of ioctl commands */
EXPORT_SYMBOL( ath_ioctl_debug);

#if ATH_DEBUG
static struct proc_dir_entry *parent_rtscts_entry;
unsigned long ath_rtscts_enable = 0;
EXPORT_SYMBOL(ath_rtscts_enable);
#endif

static struct proc_dir_entry *parent_nodefixedrate_entry;

#ifdef ATH_SUPPORT_DFS
static struct proc_dir_entry *parent_ignoredfs_entry;
unsigned long ath_ignoredfs_enable = 0;
EXPORT_SYMBOL(ath_ignoredfs_enable);
#endif



int ath_proc_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    unsigned long *this = data;

    return(snprintf(buf, count, "%ld\n", *this));
}

int ath_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    unsigned long *this = data;

    *this = simple_strtoul(buffer, NULL, 10);
    return(count);
}

int ath_proc_read_version(char *buf, char **start, off_t off, int count, int *eof, void *data)
{

    return(snprintf(buf, count, "%s\n", ATH_DRIVER_VERSION));
}

int ath_proc_write_version(struct file *file, const char *buffer, unsigned long count, void *data)
{
    return(count);
}

#if ATH_DEBUG
int ath_proc_read_rtscts(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    unsigned long *this = data;

    return(snprintf(buf, count, "%ld\n", *this));
}

int ath_proc_write_rtscts(struct file *file, const char *buffer, unsigned long count, void *data)
{
    unsigned long *this = data;

    *this = simple_strtoul(buffer, NULL, 10);
    return(count);
}
#endif

int ath_proc_read_nodefixedrate(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    return(snprintf(buf, count, "%s\n", "node fixed rate proc entry"));
}

int ath_proc_write_nodefixedrate(struct file *file, const char *buffer, unsigned long count, void *data)
{
    u_int8_t macaddr[6];
    u_int32_t fixedratecode;
    char tmp[3];
    u_int8_t i;
    tmp[2] = '\0';    

    for (i=0; i<6; i++) {
        tmp[0] = *(buffer + 2*i);
        tmp[1] = *(buffer + 2*i + 1);
        macaddr[i] = (u_int8_t)simple_strtoul(tmp, NULL, 16);
    }
    printk("Proc write: macaddr = %s\n", ether_sprintf(macaddr));
    fixedratecode = (u_int32_t)simple_strtoul(buffer+15, NULL, 16);
    printk("Proc write: fixedratecode = %x\n", fixedratecode);
    
    for (i=0; i<num_global_scn; i++) {
        printk("Searching for node in wifi%d\n", i);
        ath_node_set_fixedrate_proc(global_scn[i], macaddr, fixedratecode);
    }
    
    return(count);
}

#ifdef ATH_SUPPORT_DFS
int ath_proc_read_ignoredfs(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    unsigned long *this = data;

    return(snprintf(buf, count, "%ld\n", *this));
}

int ath_proc_write_ignoredfs(struct file *file, const char *buffer, unsigned long count, void *data)
{
    unsigned long *this = data;

    *this = simple_strtoul(buffer, NULL, 10);
    return(count);
}
#endif


static void ath_create_proc_debug(void)
{
    parent_entry = create_proc_entry("athdebug", 0644, NULL);
    parent_entry->nlink = 1;
    parent_entry->data = (void *)&ath_ioctl_debug;
    parent_entry->read_proc = ath_proc_read;
    parent_entry->write_proc = ath_proc_write;
}

static void ath_create_proc_version(void)
{
    parent_version_entry = create_proc_entry("athversion", 0644, NULL);
    parent_version_entry->nlink = 1;
    parent_version_entry->data = (void *)NULL;
    parent_version_entry->read_proc = ath_proc_read_version;
    parent_version_entry->write_proc = ath_proc_write_version;
}

#if ATH_DEBUG
static void ath_create_proc_rtscts(void)
{
    parent_rtscts_entry = create_proc_entry("athrtscts", 0644, NULL);
    parent_rtscts_entry->nlink = 1;
    parent_rtscts_entry->data = (void *)&ath_rtscts_enable;
    parent_rtscts_entry->read_proc = ath_proc_read_rtscts;
    parent_rtscts_entry->write_proc = ath_proc_write_rtscts;
}
#endif

static void ath_create_proc_nodefixedrate(void)
{
    parent_nodefixedrate_entry = create_proc_entry("athnodefixedrate", 0644, NULL);
    parent_nodefixedrate_entry->nlink = 1;
    parent_nodefixedrate_entry->data = (void *)NULL;
    parent_nodefixedrate_entry->read_proc = ath_proc_read_nodefixedrate;
    parent_nodefixedrate_entry->write_proc = ath_proc_write_nodefixedrate;
}

#ifdef ATH_SUPPORT_DFS
static void ath_create_proc_ignoredfs(void)
{
    parent_ignoredfs_entry = create_proc_entry("athignoredfs", 0644, NULL);
    parent_ignoredfs_entry->nlink = 1;
    parent_ignoredfs_entry->data = (void *)&ath_ignoredfs_enable;
    parent_ignoredfs_entry->read_proc = ath_proc_read_ignoredfs;
    parent_ignoredfs_entry->write_proc = ath_proc_write_ignoredfs;
}
#endif

static void ath_remove_proc_debug(void)
{
    printk("Removing athdebug proc file\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
    remove_proc_entry("athdebug", NULL);
#else
    remove_proc_entry("athdebug", &proc_root);
#endif
}

static void ath_remove_proc_version(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
    remove_proc_entry("athversion", NULL);
#else
    remove_proc_entry("athversion", &proc_root);
#endif
}

#if ATH_DEBUG
static void ath_remove_proc_rtscts(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
    remove_proc_entry("athrtscts", NULL);
#else
    remove_proc_entry("athrtscts", &proc_root);
#endif
}
#endif

static void ath_remove_proc_nodefixedrate(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
    remove_proc_entry("athnodefixedrate", NULL);
#else
    remove_proc_entry("athnodefixedrate", &proc_root);
#endif
}

#ifdef ATH_SUPPORT_DFS
static void ath_remove_proc_ignoredfs(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
    remove_proc_entry("athignoredfs", NULL);
#else
    remove_proc_entry("athignoredfs", &proc_root);
#endif
}
#endif


#ifndef ATH_WLAN_COMBINE
/*
 * Linux module glue
 */
static char *dev_info = "ath_dev";

MODULE_AUTHOR("Atheros Communications, Inc.");
MODULE_DESCRIPTION("Atheros Device Module");
#ifdef MODULE_LICENSE
MODULE_LICENSE("Proprietary");
#endif

static int __init
init_ath_dev(void)
{
    /* XXX version is a guess; where should it come from? */
    printk(KERN_INFO "%s: "
           "Copyright (c) 2001-2007 Atheros Communications, Inc, "
           "All Rights Reserved\n", dev_info);

   ath_create_proc_debug();
   ath_create_proc_version();
#if ATH_DEBUG
    ath_create_proc_rtscts();
#endif
    ath_create_proc_nodefixedrate();
#ifdef ATH_SUPPORT_DFS
    ath_create_proc_ignoredfs();
#endif
    return 0;
}
module_init(init_ath_dev);

static void __exit
exit_ath_dev(void)
{
    ath_remove_proc_debug();
    ath_remove_proc_version();
#if ATH_DEBUG
    ath_remove_proc_rtscts();
#endif
    ath_remove_proc_nodefixedrate();
#ifdef ATH_SUPPORT_DFS
    ath_remove_proc_ignoredfs();
#endif
    printk(KERN_INFO "%s: driver unloaded\n", dev_info);
}
module_exit(exit_ath_dev);

#if ATH_DEBUG
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,52))
MODULE_PARM(ACBKMinfree, "i");
MODULE_PARM(ACBEMinfree, "i");
MODULE_PARM(ACVIMinfree, "i");
MODULE_PARM(ACVOMinfree, "i");
MODULE_PARM(CABMinfree, "i");
MODULE_PARM(UAPSDMinfree, "i");
#else
#include <linux/moduleparam.h>
module_param(ACBKMinfree, int, 0600);
module_param(ACBEMinfree, int, 0600);
module_param(ACVIMinfree, int, 0600);
module_param(ACVOMinfree, int, 0600);
module_param(CABMinfree, int, 0600);
module_param(UAPSDMinfree, int, 0600);
#endif
EXPORT_SYMBOL(ACBKMinfree);
EXPORT_SYMBOL(ACBEMinfree);
EXPORT_SYMBOL(ACVIMinfree);
EXPORT_SYMBOL(ACVOMinfree);
EXPORT_SYMBOL(CABMinfree);
EXPORT_SYMBOL(UAPSDMinfree);
#endif
EXPORT_SYMBOL(global_scn);
EXPORT_SYMBOL(num_global_scn);

EXPORT_SYMBOL(ath_dev_attach);
EXPORT_SYMBOL(ath_dev_free);
EXPORT_SYMBOL(ath_initialize_timer);
EXPORT_SYMBOL(ath_cancel_timer);
EXPORT_SYMBOL(ath_start_timer);
EXPORT_SYMBOL(ath_set_timer_period);
EXPORT_SYMBOL(ath_timer_is_active);
EXPORT_SYMBOL(ath_free_timer);
EXPORT_SYMBOL(ath_pkt_log_text);
#if ATH_VAP_DELETE_ON_RMMOD
EXPORT_SYMBOL(ath_draintxq);
#endif /* ATH_VAP_DELETE_ON_RMMOD */
#endif /* #ifndef ATH_WLAN_COMBINE */

#ifndef REMOVE_PKT_LOG
#include "pktlog.h"
extern struct ath_pktlog_funcs *g_pktlog_funcs;
EXPORT_SYMBOL(g_pktlog_funcs);
#endif
