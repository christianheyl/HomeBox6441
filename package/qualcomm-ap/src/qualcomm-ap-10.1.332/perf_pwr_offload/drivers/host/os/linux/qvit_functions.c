#include <osdep.h>
#include <wbuf.h>
#include <linux/firmware.h>
#include <linux/pci.h>
#include <linux/if_ether.h>
#include <linux/sched.h>
#include "sw_version.h"
#include "ieee80211_var.h"
#include "ieee80211_ioctl.h"
#include "ol_if_athvar.h"
#include "ol_txrx_types.h"
#include "if_athioctl.h"
#include "osif_private.h"
#include "osapi_linux.h"
#include "if_media.h"
#include "bmi_msg.h" /* TARGET_TYPE_ */
#include "bmi.h"
#include "target_reg_table.h"
#include "ol_ath.h"
#include "sim_io.h"
#include <wdi_in.h>
#include "epping_test.h"
#include "common_drv.h"
#include "ol_helper.h"
#include "a_debug.h"
#include "targaddrs.h"
#include "adf_os_mem.h"   /* adf_os_mem_alloc,free */
#include "adf_os_types.h" /* adf_os_vprint */
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <net/iw_handler.h>

#include <qvit/QMF.h>
#include <qvit/qvit_defs.h>

#define QVIT_DEVICE "qvit"

#define isprint(c)      ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
#define IEEE80211_DBGREQ_GETMSTATS 25
#define IEEE80211_DBGREQ_GETSTATS 24

#define	IEEE80211_IOCTL_SETPARAM	(SIOCIWFIRSTPRIV+0)
#define	IEEE80211_IOCTL_GETPARAM	(SIOCIWFIRSTPRIV+1)
#define	IEEE80211_IOCTL_SETKEY		(SIOCIWFIRSTPRIV+2)
#define	IEEE80211_IOCTL_SETWMMPARAMS	(SIOCIWFIRSTPRIV+3)
#define	IEEE80211_IOCTL_DELKEY		(SIOCIWFIRSTPRIV+4)
#define	IEEE80211_IOCTL_GETWMMPARAMS	(SIOCIWFIRSTPRIV+5)
#define	IEEE80211_IOCTL_SETMLME		(SIOCIWFIRSTPRIV+6)
#define	IEEE80211_IOCTL_GETCHANINFO	(SIOCIWFIRSTPRIV+7)
#define	IEEE80211_IOCTL_SETOPTIE	(SIOCIWFIRSTPRIV+8)
#define	IEEE80211_IOCTL_GETOPTIE	(SIOCIWFIRSTPRIV+9)
#define	IEEE80211_IOCTL_ADDMAC		(SIOCIWFIRSTPRIV+10)        /* Add ACL MAC Address */
#define	IEEE80211_IOCTL_DELMAC		(SIOCIWFIRSTPRIV+12)        /* Del ACL MAC Address */
#define	IEEE80211_IOCTL_GETCHANLIST	(SIOCIWFIRSTPRIV+13)
#define	IEEE80211_IOCTL_SETCHANLIST	(SIOCIWFIRSTPRIV+14)
#define IEEE80211_IOCTL_KICKMAC		(SIOCIWFIRSTPRIV+15)
#define	IEEE80211_IOCTL_CHANSWITCH	(SIOCIWFIRSTPRIV+16)
#define	IEEE80211_IOCTL_GETMODE		(SIOCIWFIRSTPRIV+17)
#define	IEEE80211_IOCTL_SETMODE		(SIOCIWFIRSTPRIV+18)
#define IEEE80211_IOCTL_GET_APPIEBUF	(SIOCIWFIRSTPRIV+19)
#define IEEE80211_IOCTL_SET_APPIEBUF	(SIOCIWFIRSTPRIV+20)
#define IEEE80211_IOCTL_SET_ACPARAMS	(SIOCIWFIRSTPRIV+21)
#define IEEE80211_IOCTL_FILTERFRAME	(SIOCIWFIRSTPRIV+22)
#define IEEE80211_IOCTL_SET_RTPARAMS	(SIOCIWFIRSTPRIV+23)
#define IEEE80211_IOCTL_DBGREQ	        (SIOCIWFIRSTPRIV+24)
#define IEEE80211_IOCTL_SEND_MGMT	(SIOCIWFIRSTPRIV+26)
#define IEEE80211_IOCTL_SET_MEDENYENTRY (SIOCIWFIRSTPRIV+27)
#define IEEE80211_IOCTL_GET_MACADDR	(SIOCIWFIRSTPRIV+29)        /* Get ACL List */
#define IEEE80211_IOCTL_SET_HBRPARAMS	(SIOCIWFIRSTPRIV+30)
#define IEEE80211_IOCTL_SET_RXTIMEOUT	(SIOCIWFIRSTPRIV+31)

#define IW_PRIV_TYPE_OPTIE  IW_PRIV_TYPE_BYTE | IEEE80211_MAX_OPT_IE

#define IW_PRIV_TYPE_KEY \
    IW_PRIV_TYPE_BYTE | sizeof(struct ieee80211req_key)
#define IW_PRIV_TYPE_DELKEY \
    IW_PRIV_TYPE_BYTE | sizeof(struct ieee80211req_del_key)
#define IW_PRIV_TYPE_MLME \
    IW_PRIV_TYPE_BYTE | sizeof(struct ieee80211req_mlme)
#define IW_PRIV_TYPE_CHANLIST \
    IW_PRIV_TYPE_BYTE | sizeof(struct ieee80211req_chanlist)
#define IW_PRIV_TYPE_DBGREQ \
    IW_PRIV_TYPE_BYTE | sizeof(struct ieee80211req_athdbg)

#define IW_PRIV_TYPE_ACLMACLIST  (IW_PRIV_TYPE_ADDR | 256)


static const struct iw_priv_args qvit_ieee80211_priv_args[] =
{

    {
        IEEE80211_IOCTL_SETOPTIE,
        IW_PRIV_TYPE_OPTIE, 0,            "setoptie"
    },

    {
        IEEE80211_IOCTL_GETOPTIE,
        0, IW_PRIV_TYPE_OPTIE,            "getoptie"
    },

    {
        IEEE80211_IOCTL_SETKEY,
        IW_PRIV_TYPE_KEY | IW_PRIV_SIZE_FIXED, 0, "setkey"
    },

    {
        IEEE80211_IOCTL_DELKEY,
        IW_PRIV_TYPE_DELKEY | IW_PRIV_SIZE_FIXED, 0,  "delkey"
    },

    {
        IEEE80211_IOCTL_SETMLME,
        IW_PRIV_TYPE_MLME | IW_PRIV_SIZE_FIXED, 0,    "setmlme"
    },

    {
        IEEE80211_IOCTL_ADDMAC,
        IW_PRIV_TYPE_ADDR | IW_PRIV_SIZE_FIXED | 1, 0,"addmac"
    },

    {
        IEEE80211_IOCTL_DELMAC,
        IW_PRIV_TYPE_ADDR | IW_PRIV_SIZE_FIXED | 1, 0,"delmac"
    },

    {
        IEEE80211_IOCTL_GET_MACADDR,
        0, IW_PRIV_TYPE_ACLMACLIST ,"getmac"
    },

    {
        IEEE80211_IOCTL_KICKMAC,
        IW_PRIV_TYPE_ADDR | IW_PRIV_SIZE_FIXED | 1, 0,"kickmac"
    },

    {
        IEEE80211_IOCTL_SETCHANLIST,
        IW_PRIV_TYPE_CHANLIST | IW_PRIV_SIZE_FIXED, 0,"setchanlist"
    },

    {
        IEEE80211_IOCTL_GETCHANLIST,
        0, IW_PRIV_TYPE_CHANLIST | IW_PRIV_SIZE_FIXED,"getchanlist"
    },

    {
        IEEE80211_IOCTL_SETMODE,
        IW_PRIV_TYPE_CHAR |  16, 0, "mode"
    },

    {
        IEEE80211_IOCTL_GETMODE,
        0, IW_PRIV_TYPE_CHAR | 16, "get_mode"
    },


    {
        IEEE80211_IOCTL_GETPARAM,
        IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, ""
    },

    {
        IEEE80211_PARAM_TXRX_FW_MSTATS,
        IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "txrx_fw_mstats"
    },
    {
        IEEE80211_PARAM_TXRX_FW_STATS,
        IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1, 0, "txrx_fw_stats"
    },
};

static VDEV_FUNC cp;

A_UINT8 buf[MAX_QVIT_EVENT_LENGTH + sizeof(u_int32_t)];

struct sock *nl_sock = NULL;
struct sockaddr_nl src_addr,dest_addr;
struct iovec iov;
struct msghdr msg;

struct peer_list_elem
{
    struct peer_list_elem *next;
    u_int8_t mac_addr[6];
    ol_txrx_peer_handle peer;
};

typedef struct _RS_HDR
{
    unsigned char da[6];
    unsigned char sa[6];
    unsigned char pn1;
    unsigned char pn2;
} RS_HDR;

void qvit_ieee80211_ioctl_vattach(struct net_device *dev);
void qvit_osif_deliver_data(void *osptr, struct sk_buff *skb_list);
void qvit_osif_deliver_monitor_data( ol_osif_vdev_handle osif_vdev, adf_nbuf_t mpdu, void *rx_desc);

#ifdef QVIT_NETLINK_TEST
// We set time to 10 seconds for test
#define QVIT_NETLINK_TIME_VAL 10000
static struct timer_list qvit_test_timer;
static void qvit_test_timer_func(unsigned long __data);
#endif


static int qvit_ieee80211_ioctl_giwname(struct net_device *dev,
                                        struct iw_request_info *info,
                                        char *name, char *extra);

static int
qvit_ieee80211_ioctl_dbgreq(struct net_device *dev, struct iw_request_info *info,
                            void *w, char *extra);


static const iw_handler qvit_ieee80211_handlers[] =
{
    (iw_handler) NULL,                          /* SIOCSIWCOMMIT */
    (iw_handler) qvit_ieee80211_ioctl_giwname,       /* SIOCGIWNAME */
    (iw_handler) NULL,                          /* SIOCSIWNWID */
    (iw_handler) NULL,                          /* SIOCGIWNWID */
    (iw_handler) NULL,                          /* SIOCSIWFREQ */
    (iw_handler) NULL,                          /* SIOCGIWFREQ */
    (iw_handler) NULL,                          /* SIOCSIWMODE */
    (iw_handler) NULL,                          /* SIOCGIWMODE */
    (iw_handler) NULL,       /* SIOCSIWSENS */
    (iw_handler) NULL,       /* SIOCGIWSENS */
    (iw_handler) NULL /* not used */,       /* SIOCSIWRANGE */
    (iw_handler) NULL,  /* SIOCGIWRANGE */
    (iw_handler) NULL /* not used */,       /* SIOCSIWPRIV */
    (iw_handler) NULL /* kernel code */,        /* SIOCGIWPRIV */
    (iw_handler) NULL /* not used */,       /* SIOCSIWSTATS */
    (iw_handler) NULL /* kernel code */,        /* SIOCGIWSTATS */
    (iw_handler) NULL,              /* SIOCSIWSPY */
    (iw_handler) NULL,              /* SIOCGIWSPY */
    (iw_handler) NULL,              /* -- hole -- */
    (iw_handler) NULL,              /* -- hole -- */
    (iw_handler) NULL,     /* SIOCSIWAP */
    (iw_handler) NULL,     /* SIOCGIWAP */
    (iw_handler) NULL,              /* -- hole -- */
    (iw_handler) NULL,      /* SIOCGIWAPLIST */
    (iw_handler) NULL,              /* SIOCSIWSCAN */
    (iw_handler) NULL,              /* SIOCGIWSCAN */
    (iw_handler) NULL,      /* SIOCSIWESSID */
    (iw_handler) NULL,      /* SIOCGIWESSID */
    (iw_handler) NULL,      /* SIOCSIWNICKN */
    (iw_handler) NULL,      /* SIOCGIWNICKN */
    (iw_handler) NULL,              /* -- hole -- */
    (iw_handler) NULL,              /* -- hole -- */
    (iw_handler) NULL,       /* SIOCSIWRATE */
    (iw_handler) NULL,       /* SIOCGIWRATE */
    (iw_handler) NULL,        /* SIOCSIWRTS */
    (iw_handler) NULL,        /* SIOCGIWRTS */
    (iw_handler) NULL,       /* SIOCSIWFRAG */
    (iw_handler) NULL,       /* SIOCGIWFRAG */
    (iw_handler) NULL,      /* SIOCSIWTXPOW */
    (iw_handler) NULL,      /* SIOCGIWTXPOW */
    (iw_handler) NULL,      /* SIOCSIWRETRY */
    (iw_handler) NULL,      /* SIOCGIWRETRY */
    (iw_handler) NULL,     /* SIOCSIWENCODE */
    (iw_handler) NULL,     /* SIOCGIWENCODE */
    (iw_handler) NULL,      /* SIOCSIWPOWER */
    (iw_handler) NULL,      /* SIOCGIWPOWER */
    (iw_handler) NULL,              /* -- hole -- */
    (iw_handler) NULL,              /* -- hole -- */
#if WIRELESS_EXT >= 18
    (iw_handler) NULL,      /* SIOCSIWGENIE */
    (iw_handler) NULL,      /* SIOCGIWGENIE */
    (iw_handler) NULL,      /* SIOCSIWAUTH */
    (iw_handler) NULL,      /* SIOCGIWAUTH */
    (iw_handler) NULL,  /* SIOCSIWENCODEEXT */
    (iw_handler) NULL,  /* SIOCGIWENCODEEXT */
#endif /* WIRELESS_EXT >= 18 */
};


static const iw_handler qvit_ieee80211_priv_handlers[] =
{
    (iw_handler) NULL,      /* SIOCWFIRSTPRIV+0 */
    (iw_handler) qvit_ieee80211_ioctl_dbgreq,      /* SIOCWFIRSTPRIV+1 */
    (iw_handler) NULL,        /* SIOCWFIRSTPRIV+2 */
    (iw_handler) NULL,  /* SIOCWFIRSTPRIV+3 */
    (iw_handler) NULL,        /* SIOCWFIRSTPRIV+4 */
    (iw_handler) NULL,  /* SIOCWFIRSTPRIV+5 */
    (iw_handler) NULL,       /* SIOCWFIRSTPRIV+6 */
    (iw_handler) NULL,   /* SIOCWFIRSTPRIV+7 */
    (iw_handler) NULL,      /* SIOCWFIRSTPRIV+8 */
    (iw_handler) NULL,      /* SIOCWFIRSTPRIV+9 */
    (iw_handler) NULL,        /* SIOCWFIRSTPRIV+10 */
    (iw_handler) NULL,/* SIOCWFIRSTPRIV+11 */
    (iw_handler) NULL,        /* SIOCWFIRSTPRIV+12 */
    (iw_handler) NULL,   /* SIOCWFIRSTPRIV+13 */
    (iw_handler) NULL,   /* SIOCWFIRSTPRIV+14 */
    (iw_handler) NULL,       /* SIOCWFIRSTPRIV+15 */
    (iw_handler) NULL,    /* SIOCWFIRSTPRIV+16 */
    (iw_handler) NULL,       /* SIOCWFIRSTPRIV+17 */
    (iw_handler) NULL,       /* SIOCWFIRSTPRIV+18 */
    (iw_handler) NULL,   /* SIOCWFIRSTPRIV+19 */
    (iw_handler) NULL,   /* SIOCWFIRSTPRIV+20 */
#if ATH_SUPPORT_IQUE
    (iw_handler) NULL,   /* SIOCWFIRSTPRIV+21 */
#else
    (iw_handler) NULL,                          /* SIOCWFIRSTPRIV+21 */
#endif
    (iw_handler) NULL,     /* SIOCWFIRSTPRIV+22 */
#if ATH_SUPPORT_IQUE
    (iw_handler) NULL,   /* SIOCWFIRSTPRIV+23 */
#else
    (iw_handler) NULL,                          /* SIOCWFIRSTPRIV+23 */
#endif
    (iw_handler) qvit_ieee80211_ioctl_dbgreq,        /* SIOCWFIRSTPRIV+24 */
#if ATH_SUPPORT_LINKDIAG_EXT
    (iw_handler) NULL, /* SIOCWFIRSTPRIV+25 */
#else
    (iw_handler) NULL,                          /* SIOCWFIRSTPRIV+25 */
#endif
    (iw_handler) NULL,      /* SIOCWFIRSTPRIV+26 */
#if ATH_SUPPORT_IQUE
    (iw_handler) NULL, /* SIOCWFIRSTPRIV+27 */
#else
    (iw_handler) NULL,                          /* SIOCWFIRSTPRIV+27 */
#endif
    (iw_handler) NULL,                          /* SIOCWFIRSTPRIV+28 */
    (iw_handler) NULL,  /* SIOCWFIRSTPRIV+29 */
#if ATH_SUPPORT_IQUE
    (iw_handler) NULL,  /* SIOCWFIRSTPRIV+30 */
#else
    (iw_handler) NULL,
#endif
#ifdef notyet
    (iw_handler) NULL,  /* SIOCWFIRSTPRIV+31 */
#else
    (iw_handler) NULL,
#endif /* notyet */
#if 0
    /*
     * MCAST_GROUP is used only for debugging, with the conflicting
     * set_rxtimeout disabled.
     */
    (iw_handler) ieee80211_ioctl_mcast_group,   /* SIOCWFIRSTPRIV+31 */
#endif

};

#define N(a)    (sizeof (a) / sizeof (a[0]))

static iw_handler qvit_ieee80211_wrapper_handlers[sizeof(qvit_ieee80211_handlers)/sizeof(qvit_ieee80211_handlers[0])];
static iw_handler qvit_ieee80211_priv_wrapper_handlers[N(qvit_ieee80211_priv_handlers)];

static struct iw_handler_def qvit_ieee80211_iw_handler_def =
{
    .standard       = (iw_handler *) qvit_ieee80211_wrapper_handlers,
    .num_standard       = (sizeof(qvit_ieee80211_wrapper_handlers)/sizeof(qvit_ieee80211_wrapper_handlers[0])),
    .private        = (iw_handler *) qvit_ieee80211_priv_wrapper_handlers,
    .num_private        = N(qvit_ieee80211_priv_wrapper_handlers),
    .private_args       = (struct iw_priv_args *) qvit_ieee80211_priv_args,
    .num_private_args   = N(qvit_ieee80211_priv_args),
#if WIRELESS_EXT > 18
//    .get_wireless_stats =   ieee80211_iw_getstats
#endif
};
#undef N



static int qvit_ieee80211_ioctl_giwname(struct net_device *dev,
                                        struct iw_request_info *info,
                                        char *name, char *extra)
{

    DiagVi_PrivType *ap;
    struct ieee80211com 	*ic;
    struct ieee80211_channel *c ;
    struct ol_ath_softc_net80211 *scn;

    //wlan_if_t vap;

    ap = netdev_priv(dev);
    scn = ap->scn;

    ic = &scn->sc_ic;
    //vap = scn->sc_osdev->os_if;

    //c = wlan_get_current_channel(vap, true);
    c  = ic->ic_curchan;

    if (IEEE80211_IS_CHAN_108G(c))
        strncpy(name, "IEEE 802.11Tg", IFNAMSIZ);
    else if (IEEE80211_IS_CHAN_108A(c))
        strncpy(name, "IEEE 802.11Ta", IFNAMSIZ);
    else if (IEEE80211_IS_CHAN_TURBO(c))
        strncpy(name, "IEEE 802.11T", IFNAMSIZ);
    else if (IEEE80211_IS_CHAN_ANYG(c))
        strncpy(name, "IEEE 802.11g", IFNAMSIZ);
    else if (IEEE80211_IS_CHAN_A(c))
        strncpy(name, "IEEE 802.11a", IFNAMSIZ);
    else if (IEEE80211_IS_CHAN_B(c))
        strncpy(name, "IEEE 802.11b", IFNAMSIZ);
    else if (IEEE80211_IS_CHAN_11NG(c))
        strncpy(name, "IEEE 802.11ng", IFNAMSIZ);
    else if (IEEE80211_IS_CHAN_11NA(c))
        strncpy(name, "IEEE 802.11na", IFNAMSIZ);
    else
        strncpy(name, "IEEE 802.11", IFNAMSIZ);
    return 0;
}


static int
qvit_ieee80211_ioctl_dbgreq(struct net_device *dev, struct iw_request_info *info,
                            void *w, char *extra)
{
    int retv = 0;
    int status;
    int *i = (int *) extra;
    DiagVi_PrivType *ap = netdev_priv(dev);
    struct ol_ath_softc_net80211 *scn = ap->scn;
    ol_txrx_pdev_handle txrx_pdev;
    int value = i[1];
    struct ieee80211req_athdbg *req = (struct ieee80211req_athdbg *) extra;
    /* get pdev */
    txrx_pdev = scn->pdev_txrx_handle;
#ifdef QVIT_DEBUG
    printk(KERN_INFO "QVIT: %s IEEE80211_IOCTL_DBGREQ\n", dev->name);
#endif
    switch (req->cmd)
    {

    case IEEE80211_DBGREQ_GETMSTATS:
    {
        struct ol_txrx_stats_req req;
        memset(&req, 0, sizeof(req));
        req.print.verbose = 1;
        req.stats_type_upload_mask = value;
        ol_txrx_fw_stats_get(ap->txrx_vdev, &req);
        ol_txrx_pdev_display(txrx_pdev, 4);
    }
    break;
    case IEEE80211_DBGREQ_GETSTATS:
    {

        struct ol_txrx_stats_req req;
        memset(&req, 0, sizeof(req));
        req.print.verbose = 1; /* default */

        /*
         * Backwards compatibility: use the same old input values, but
         * translate from the old values to the corresponding new bitmask
         * value.
         */
        if (value <= 4)
        {
            req.stats_type_upload_mask = 1 << (value - 1);
        }
        else if (value == 5)
        {
            /*
             * Stats request 5 is the same as stats request 4,
             * but with only a concise printout.
             */
            req.print.concise = 1;
            req.stats_type_upload_mask = 1 << (4 - 1);
        }
        status = ol_txrx_fw_stats_get(ap->txrx_vdev, &req);
        retv = status;
        ol_txrx_stats_display(txrx_pdev);

    }
    break;

    default:
        printk(KERN_INFO "QVIT: %s : Dbg command %d is not supported \n", __func__, req->cmd);
        break;
    }
    return retv;
}


/**
 * @brief Channel Number to Frequency
 *
 * Note: This function doesn't check if the frequency if valid for a country
 *
 * @param Channel Number
 *
 * @return Channel Frequency in MHz
 */
unsigned int
chan2mhz(unsigned int chan)
{
    /* Calculate first 2.4Ghz frequencies */
    if(chan == 14)  return 2484;

    if(chan < 14)   return 2407 + (chan * 5);

    /* 5GHz channels */
    return 5000 + (chan * 5);
}




/** ---------------------------------------------------------------------------

    \fn - pal_ani_hexdump

    \brief -

    \param

    \return - void.

  -------------------------------------------------------------------------------*/
void qvit_hexdump( const unsigned char *buffer, unsigned int len )
{
    char string[80], hex[10];
    int offset, i, l;

    for (offset = 0; offset < len; offset += 16)
    {
        sprintf( string, "%03d: ", offset );

        for (i = 0; i < 16; i++)
        {
            if ((i + offset) < len)
                sprintf( hex, "%02x ", buffer[offset + i] );
            else
                strcpy( hex, "   " );

            strcat( string, hex );
        }
        strcat( string, "  " );
        l = strlen( string );

        for (i = 0; (i < 16) && ((i + offset) < len); i++)
            string[l++] = isprint( buffer[offset + i] ) ? buffer[offset + i] : '.';

        string[l] = '\0';
        printk( KERN_INFO "%s\n", string );
    }
}


void
qvit_osif_deliver_monitor_data(
    ol_osif_vdev_handle osif_vdev,
    adf_nbuf_t mpdu,
    void *rx_desc)
{
    /* Should use adf_nbuf_next_ext to check if there are multiple nbufs comprising the MPDU */
    adf_nbuf_free(mpdu);
}

void qvit_osif_deliver_data(void *osptr, struct sk_buff *skb_list)
{
    RS_HDR 	*pRsh;
    DiagVi_PrivType *ap;
    struct net_device *dev =  (struct net_device *)osptr;
    int packetLen;

#ifdef QVIT_DEBUG_RX
    printk(KERN_INFO "QVIT: osif = 0x%p\n", osptr);
    printk(KERN_INFO "QVIT: Entering qvit_osif_deliver_data(). skb_list = 0x%p\n", skb_list);
#endif
    while (skb_list)
    {
        struct sk_buff *skb;

        skb = skb_list;
        skb_list = skb_list->next;

        skb->dev = dev;
        ap = netdev_priv(dev);

        pRsh = (RS_HDR *)adf_nbuf_push_head(skb, 14);

        adf_os_mem_set((void *)pRsh, 0x1, 12);
        pRsh->pn1 = 'Q';
        pRsh->pn2 = 'V';
        packetLen = adf_nbuf_get_frag_len(skb, 0);

        ap->stats.rx_packets++;
        ap->stats.rx_bytes += packetLen;
#ifdef QVIT_DEBUG_RX
        qvit_hexdump ((unsigned char *)adf_nbuf_data(skb), packetLen);
#endif

#ifdef USE_HEADERLEN_RESV
        skb->protocol = ath_eth_type_trans(skb, dev);
#else
        skb->protocol = eth_type_trans(skb, dev);
#endif

        if (in_interrupt())
            netif_rx(skb);
        else
            netif_rx_ni(skb);

    }
}



int qvit_vdev_attach(ol_scn_t scn_handle, u_int8_t *macAddr, u_int32_t vdevType, u_int8_t vdevId, struct net_device *dev)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)scn_handle;
    struct ol_txrx_osif_ops ops;
    ol_txrx_pdev_handle txrx_pdev;
    DiagVi_PrivType *ap;
    ap = netdev_priv(dev);

    //g_peer_list = NULL;

    // b0 is BSSID - DUT1, packets are from a0 and forward to STA at c0 STA DUT2

    txrx_pdev = scn->pdev_txrx_handle;

    /* pdev attach and pdev target attach are already done in init */
#ifdef QVIT
    printk(KERN_INFO "QVIT: In %s()\n", __FUNCTION__);
    printk(KERN_INFO "QVIT: vdev_id = 0x%x, vdevType = %d\n", vdevId, vdevType);
#endif
    ap->txrx_vdev = ol_txrx_vdev_attach(txrx_pdev, macAddr /* vdev_mac_Addr */, vdevId, vdevType /* vdevType, AP =1, STA = 3 */);
#ifdef QVIT
    printk(KERN_INFO "QVIT: vdev handle = 0x%x\n", (unsigned int)ap->txrx_vdev);
#endif
    ops.rx.std = (ol_txrx_rx_fp)     qvit_osif_deliver_data;
    // ops.rx.mon = (ol_txrx_rx_mon_fp) qvit_osif_deliver_monitor_data;
    ops.rx.mon = NULL;
    ol_txrx_osif_vdev_register(ap->txrx_vdev, (ol_osif_vdev_handle)dev, &ops);
    ap->vdev_tx = ops.tx.std;
#ifdef QVIT
    printk(KERN_INFO "QVIT: vdev tx ptr = 0x%x from ol_txrx_osif_vdev_register\n", (unsigned int)ap->vdev_tx);
#endif
    return (0);
}


int qvit_vdev_detach(u_int8_t vdevId)
{

    char devName[10];
    char number[10];
    struct net_device *dev;
    DiagVi_PrivType *ap;
    int ret = 1;

    // ol_txrx_vdev_handle txrx_vdev;
    strcpy(devName, QVIT_DEVICE);
    sprintf(number,"%d", vdevId);
    strcat(devName, number);
    dev = __dev_get_by_name(&init_net, devName);

    if (dev)
    {
        ap = netdev_priv(dev);
        //get vdev handle
        ol_txrx_vdev_detach(ap->txrx_vdev, NULL, NULL);
        ret = 0;
    }
    return ret;
}


ol_txrx_peer_handle qvit_peer_alloc(ol_scn_t scn_handle, u_int8_t *macAddr, int vdevId)
{
    struct ol_ath_softc_net80211 *scn = (struct ol_ath_softc_net80211 *)scn_handle;
    char devName[10];
    char number[10];
    ol_txrx_pdev_handle txrx_pdev;
    struct net_device *dev;
    DiagVi_PrivType *ap;
    ol_txrx_peer_handle txrx_peer;

    strcpy(devName, QVIT_DEVICE);
    sprintf(number,"%d", vdevId);
    strcat(devName, number);
    dev = __dev_get_by_name(&init_net, devName);

    if (dev)
    {
        ap = netdev_priv(dev);

        txrx_pdev = scn->pdev_txrx_handle;
#ifdef QVIT
        printk(KERN_INFO "QVIT: In %s() dev[%s]\n", __FUNCTION__, devName);
        printk(KERN_INFO "QVIT: peer mac addr = %02x:%02x:%02x:%02x:%02x:%02x txrx_vdev [0x%p]\n",
               macAddr[0],
               macAddr[1],
               macAddr[2],
               macAddr[3],
               macAddr[4],
               macAddr[5],ap->txrx_vdev);
#endif
        txrx_peer = ol_txrx_peer_attach(txrx_pdev,  ap->txrx_vdev, macAddr);

#ifdef QVIT
        printk(KERN_INFO "QVIT: txrx_peer handle = 0x%x\n", (unsigned int)txrx_peer);
#endif
        return txrx_peer;
    }
    printk(KERN_ERR "QVIT: Could not find device [%s]\n", devName);
    return NULL;
}


int
qvit_unified_ioctl (struct ol_ath_softc_net80211 *scn, struct ifreq *ifr)
{
    int error = 0;
    unsigned int cmd,length;
    char *userdata;
    unsigned char *buffer;
    unsigned int rc;
    char number[10];
    char devName[10];
    ol_txrx_peer_handle txrx_peer_handle;
    struct net_device *dev;

    QMF_HDR *pQmf;
    QVIT_Msg_Wal_VDEV_Attach_Req *pWva;


    get_user(cmd, (int *)((unsigned int *)ifr->ifr_data)+1);
    userdata = (char *)(((unsigned int *)ifr->ifr_data)+2);
    switch (cmd)
    {
    case ATH_XIOCTL_UNIFIED_QVIT_CMD:
    {
        get_user(length, (unsigned int *)userdata);

        if ( length > MAX_QVIT_EVENT_LENGTH )
            return -EFAULT;

        buffer = (unsigned char*)OS_MALLOC((void*)scn->sc_osdev, length, GFP_KERNEL);
        if (buffer != NULL)
        {
            OS_MEMZERO(buffer, length);

            if (copy_from_user(buffer, &userdata[sizeof(length)], length))
            {
                error = -EFAULT;
            }
            else
            {
                pQmf = (QMF_HDR *)buffer;

                if (pQmf->msgCode == QVIT_MSG_WAL_VDEV_ATTACH_REQ)
                {
                    pWva = (QVIT_Msg_Wal_VDEV_Attach_Req *)buffer;
#ifdef QVIT_DEBUG
                    printk(KERN_INFO "QVIT: QVIT message received, CMD=%x, Len=%d\n", pQmf->msgCode, pQmf->msgLen);
#endif
                    // Send command to FA first
                    error = ol_ath_qvit_cmd(scn,(u_int8_t*)buffer,(u_int16_t)length);
                    if (error)
                    {
                        printk(KERN_ERR "QVIT: %s ol_ath_qvit_cmd failed error = %d\n", __FUNCTION__, error);
                        break;
                    }
                    ol_txrx_print_level_set(5); // set debug level to TXRX_PRINT_LEVEL_INFO2
                    strcpy(devName, QVIT_DEVICE);
                    sprintf(number,"%d", pWva->vdevId);
                    strcat(devName, number);
                    strcpy(cp.vdev_name, devName);
                    cp.vdev_id = pWva->vdevId;

                    // Ready to create netdevice
                    dev = qvit_create_vdev(&cp, &pWva->macAddr[0], scn);

                    if (dev)
                    {
                        // Now to create txrx vdev and hook other fcn ptr
                        error = qvit_vdev_attach(scn, &pWva->macAddr[0], pWva->vdevType, pWva->vdevId, dev);
                        
                        error = ol_ath_qvit_cmd(scn,(u_int8_t*)buffer,(u_int16_t)length);
                        if (error)
                            printk(KERN_ERR "QVIT: %s() error = %d\n", __FUNCTION__, error);
                    }
                    else
                    {
                        error = -EFAULT;
                        break;
                    }


                }
                else if (pQmf->msgCode == QVIT_MSG_WAL_PEER_ALLOC_REQ)
                {
                    QVIT_Msg_Wal_Peer_Alloc_Req *pWpa = (QVIT_Msg_Wal_Peer_Alloc_Req *)buffer;
#ifdef QVIT_DEBUG
                    printk(KERN_INFO "QVIT: QVIT message received, CMD=%x, Len=%d\n", pQmf->msgCode, pQmf->msgLen);
                    printk(KERN_INFO "QVIT: pWpa->vdevId [%d]\n",pWpa->vdevId);
#endif
                    /* Perform driver TxRx peer attach */
                    txrx_peer_handle = qvit_peer_alloc(scn, &pWpa->macAddr[0],pWpa->vdevId);
                    if (txrx_peer_handle == NULL)
                    {
                        printk(KERN_ERR "QVIT: peer_alloc function failed\n");
                        error = -EFAULT;
                        break;
                    }
                    /* Process peer alloc Qvit command in TE */
                    error = ol_ath_qvit_cmd(scn,(u_int8_t*)buffer,(u_int16_t)length);

                }
#if 0               /* Do NOT remove for now. Maybe used to trigger Tx. Do NOT trap this in Rx side */
                else if (pQmf->msgCode == QVIT_MSG_WAL_VDEV_SET_INPUT_DATA_PKT_TYPE_REQ)
                {

                    /* Need to set packet type in driver *************** */

                    /* kick start the packet after PKT_TYPE specified */
                    printk("QVIT DATA_PKT_TYPE_RSP message done, CMD=%x, Len=%d\n", pQmf->msgCode, pQmf->msgLen);
                    /* error = ol_ath_qvit_start_xmit(scn); */
                }
#endif
                else if (pQmf->msgCode == QVIT_MSG_WAL_PEER_DEALLOC_REQ)
                {
                    QVIT_Msg_Wal_Peer_DeAlloc_Req *pWpda = (QVIT_Msg_Wal_Peer_DeAlloc_Req *)buffer;
#ifdef QVIT
                    printk(KERN_INFO "QVIT: QVIT_MSG_WAL_PEER_DEALLOC_REQ message received, CMD=%x, Len=%d\n", pQmf->msgCode, pQmf->msgLen);
                    printk(KERN_INFO "QVIT: pWpda->vdevId [%d] peerHandle 0x%x vdevHandle [0x%x]\n",pWpda->vdevId, pWpda->peerHandle, pWpda->vdevHandle );
#endif
                    /* Send peer dealloc message FA */
                    /* pWpda->peerHandle is wal_peerHandle */
                    /* In order to delete this peer from ol_rxtx we will need txrx_peerhandle  */
                    /* Extract this handle from our vdev */
                    error = ol_ath_qvit_cmd(scn,(u_int8_t*)buffer,(u_int16_t)length);
                    //qvit_delete_peer(pWpda->vdevId, pWpda->macAddr);

                }

                else if (pQmf->msgCode == QVIT_MSG_WAL_VDEV_DETACH_REQ)
                {
                    QVIT_Msg_Wal_VDEV_Detach_Req *pVDEVda = (QVIT_Msg_Wal_VDEV_Detach_Req *)buffer;
#ifdef QVIT
                    printk(KERN_INFO "QVIT: QVIT_MSG_WAL_VDEV_DETACH_REQ message received, CMD=%x, Len=%d\n", pQmf->msgCode, pQmf->msgLen);
                    printk(KERN_INFO "QVIT: pWpda->vdevId [%d] vdevHandle [0x%x]\n",pVDEVda->vdevId, pVDEVda->vdevHandle );
#endif
                    /* Send vdev detach message to FA */
                    /* pVDEVda->vdevHandle is wal_vdevHandle */
                    /* In order to delete this vdev from ol_rxtx we will need txrx_peerhandle  */
                    /* Extract this handle from our vdev */
                    error = ol_ath_qvit_cmd(scn,(u_int8_t*)buffer,(u_int16_t)length);
                    qvit_delete_peers(pVDEVda->vdevId);

                    qvit_vdev_detach(pVDEVda->vdevId);

                }
                else if (pQmf->msgCode == QVIT_MSG_WAL_VDEV_STOP_REQ)
                {
                    QVIT_Msg_Wal_VDEV_Stop_Req *pVDEVStop = (QVIT_Msg_Wal_VDEV_Stop_Req *)buffer;
#ifdef QVIT
                    printk(KERN_INFO "QVIT: QVIT_MSG_WAL_VDEV_STOP_REQ message received, CMD=%x, Len=%d\n", pQmf->msgCode, pQmf->msgLen);
                    printk(KERN_INFO "QVIT: pVDEVStop->vdevId [%d] pVDEVStop [0x%x]\n",pVDEVStop->vdevId, pVDEVStop->vdevHandle );
#endif
                    /* Send vdev detach message to FA */
                    /* pVDEVda->vdevHandle is wal_vdevHandle */
                    /* In order to delete this vdev from ol_rxtx we will need txrx_peerhandle  */
                    /* Extract this handle from our vdev */
                    error = ol_ath_qvit_cmd(scn,(u_int8_t*)buffer,(u_int16_t)length);

                }

                else if (pQmf->msgCode == QVIT_MSG_WAL_VDEV_DOWN_REQ)
                {
                    QVIT_Msg_Wal_VDEV_Down_Req *pVDEVDown = (QVIT_Msg_Wal_VDEV_Down_Req *)buffer;
#ifdef QVIT
                    printk(KERN_INFO "QVIT: QVIT_MSG_WAL_VDEV_DOWN_REQ message received, CMD=%x, Len=%d\n", pQmf->msgCode, pQmf->msgLen);
                    printk(KERN_INFO "QVIT: pVDEVDown->vdevId [%d] pVDEVDown [0x%x]\n",pVDEVDown->vdevId, pVDEVDown->vdevHandle );
#endif
                    /* Send vdev detach message to FA */
                    /* pVDEVda->vdevHandle is wal_vdevHandle */
                    /* In order to delete this vdev from ol_rxtx we will need txrx_peerhandle  */
                    /* Extract this handle from our vdev */
                    error = ol_ath_qvit_cmd(scn,(u_int8_t*)buffer,(u_int16_t)length);

                }

                else
                {
                    error = ol_ath_qvit_cmd(scn,(u_int8_t*)buffer,(u_int16_t)length);
                }
            }
            OS_FREE(buffer);
        }
        else
            error = -ENOMEM;
    }
    break;
    case ATH_XIOCTL_UNIFIED_QVIT_RSP:
    {
        error = ol_ath_qvit_rsp(scn,(u_int8_t*)buf);

        if ( !error )
        {
            rc = copy_to_user(ifr->ifr_data, buf, (MAX_QVIT_EVENT_LENGTH + sizeof(u_int32_t)));
            if (rc)
                error = -EFAULT;

        }
        else
            error = -EAGAIN;
    }
    break;
    }

    return error;
}


/* For pDEV only */
void diag_vi_pdev_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *drv_info)
{
#ifdef A_SIMOS_DEVHOST
#else
    
    struct ol_ath_softc_net80211 *scn;
    char buffer[64];
    scn = netdev_priv(dev);
#endif
#ifdef QVIT_DEBUG
    printk("QVIT: %s: called\n", __FUNCTION__);
#endif
    memset(drv_info, 0, sizeof(*drv_info));
    strcpy (drv_info->driver, DRV_NAME);
#ifdef A_SIMOS_DEVHOST
    strcpy (drv_info->version, DRV_VERSION);
    strcpy (drv_info->bus_info, BUS_INFO);
    strcpy (drv_info->fw_version,FW_VERSION);
#else
    sprintf(buffer,"%d", scn->target_type);
    strcpy(drv_info->version, buffer);
    strcpy (drv_info->bus_info, "pci");
    sprintf(buffer,"0x%x", scn->target_version);
    strcpy(drv_info->fw_version, buffer);
#endif
    return;
}

// for pDEV only
static struct ethtool_ops diag_vi_ethtool_ops =
{
    .get_drvinfo            = diag_vi_pdev_get_drvinfo,
};

// for vDEV
static int  diag_vi_get_settings(struct net_device *, struct ethtool_cmd *);
static int  diag_vi_set_settings(struct net_device *, struct ethtool_cmd *);
static int  diag_vi_change_mtu(struct net_device *dev, int new_mtu);
static int  diag_vi_open(struct net_device *dev);
static int  diag_vi_hard_start_xmit(struct sk_buff *skb, struct net_device *dev);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0))
static void diag_vi_set_multicast_list(struct net_device *dev);
#endif
static int  diag_vi_set_address(struct net_device *dev, void *p);
static int  diag_vi_stop(struct net_device *dev);
static struct net_device_stats *diag_vi_get_stats(struct net_device *dev);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
static int  diag_vi_validate_add(struct net_device *dev);
static struct net_device_ops diag_vi_ops =
{
    .ndo_open               = diag_vi_open,
    .ndo_start_xmit         = diag_vi_hard_start_xmit,
    .ndo_set_mac_address    = diag_vi_set_address,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0))
    .ndo_set_multicast_list = diag_vi_set_multicast_list,
#endif
    .ndo_stop               = diag_vi_stop,
    .ndo_get_stats          = diag_vi_get_stats,
    .ndo_validate_addr      = diag_vi_validate_add,
    .ndo_change_mtu         = diag_vi_change_mtu,
};
#endif


static int diag_vi_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
#ifdef QVIT_DEBUG
    printk(KERN_INFO "QVIT: %s: called\n", __FUNCTION__);
#endif
    return 0;
}

static int diag_vi_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
#ifdef QVIT_DEBUG
    printk(KERN_INFO "QVIT: %s: called\n", __FUNCTION__);
#endif
    return 0;
}


int diag_vi_open(struct net_device *dev)
{
#ifdef QVIT_DEBUG
    printk(KERN_INFO "QVIT: %s called\n", __FUNCTION__);
#endif
    netif_carrier_on(dev);
    netif_start_queue(dev);
    return 0;
}

int diag_vi_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    DiagVi_PrivType *ap;
    struct ethhdr *eth_hdr;
    a_status_t status = A_STATUS_FAILED;
    eth_hdr = ( struct ethhdr *)skb->data;

    ap = netdev_priv(dev);
    if (eth_hdr->h_proto == htons(QVIT_PROTO))
    {
#ifdef QVIT_DEBUG_1
        printk(KERN_INFO "QVIT: %s: entered on %s\n", __FUNCTION__, dev->name);
#endif
        /* skip 14 bytes (sizeof eth hdr) dummy header */
        skb_pull(skb, sizeof(struct ethhdr));

        /* Map buffer for DMA */
        status = adf_nbuf_map((adf_os_device_t)ap->scn->sc_osdev , skb, ADF_OS_DMA_TO_DEVICE);
        if (status != A_STATUS_OK)
        {
            kfree_skb(skb);
            printk(KERN_ERR "QVIT: %s() adf_nbuf_map failed. Packet dropped\n", __func__);
            ap->stats.tx_dropped++;
            return 0;
        }
        /* Deliver to TxRx tx handler */
        ap->vdev_tx(ap->txrx_vdev, skb);
#ifdef QVIT_DEBUG_1
        printk(KERN_INFO "QVIT: %s: packet sent on %s\n", __FUNCTION__, dev->name);
#endif
        ap->stats.tx_packets++;
        ap->stats.tx_bytes += skb->len;
        return 0;
    }
    kfree_skb(skb);
    return 0;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0))
void diag_vi_set_multicast_list(struct net_device *dev)
{
#ifdef QVIT_DEBUG
    printk(KERN_INFO "QVIT: %s: called\n", __FUNCTION__);
#endif
}
#endif

int diag_vi_set_address(struct net_device *dev, void *p)
{
    struct sockaddr *sa = p;
#ifdef QVIT_DEBUG
    printk(KERN_INFO "QVIT: %s called\n", __FUNCTION__);
#endif
    if (!is_valid_ether_addr((const u8 *)sa->sa_data))
    {
        return -EADDRNOTAVAIL;
    }
    memcpy(dev->dev_addr, sa->sa_data, ETH_ALEN);
    return 0;
}


int diag_vi_stop(struct net_device *dev)
{
#ifdef QVIT_DEBUG
    printk(KERN_INFO "QVIT: %s called\n", __FUNCTION__);
#endif
    netif_carrier_off(dev);
    netif_stop_queue(dev);
    return 0;
}

int diag_vi_validate_add(struct net_device *dev)
{
#ifdef QVIT_DEBUG
    printk(KERN_INFO "QVIT: %s called for %s\n", __FUNCTION__, dev->name);
    printk(KERN_INFO "MAC addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
           dev->dev_addr[0],
           dev->dev_addr[1],
           dev->dev_addr[2],
           dev->dev_addr[3],
           dev->dev_addr[4],
           dev->dev_addr[5]);

#endif
    return 0;
}

static int diag_vi_change_mtu(struct net_device *dev, int new_mtu)
{
#ifdef QVIT_DEBUG
    printk(KERN_INFO "QVIT: New mtu [%d] old mtu [%d]\n", new_mtu, dev->mtu);
#endif
    dev->mtu = new_mtu;
    return 0;
}

#if 0
static int  diag_vi_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
    printk(KERN_INFO "QVIT: %s called cmd 0x%x\n", __FUNCTION__, cmd);
    return 0;
}
#endif


static struct net_device_stats *diag_vi_get_stats(struct net_device *dev)
{
    DiagVi_PrivType *ap = netdev_priv(dev);
    return &ap->stats;
}

#define N(a)    (sizeof (a) / sizeof (a[0]))

static int
qvit_ieee80211_ioctl_wrapper(struct net_device *dev,
                             struct iw_request_info *info,
                             union iwreq_data *wrqu, char *extra)
{
    int cmd_id;
    iw_handler handler;


    if (info->cmd >= SIOCIWFIRSTPRIV)
    {
        cmd_id = (info->cmd - SIOCIWFIRSTPRIV);
        if (cmd_id < 0 || cmd_id >= N(qvit_ieee80211_priv_handlers))
        {
            printk(KERN_INFO "QVIT: %s() : #### wrong private ioctl 0x%x\n", __func__, info->cmd);
            return -EINVAL;
        }

        handler = qvit_ieee80211_priv_handlers[cmd_id];
        if (handler)
            return handler(dev, info, wrqu, extra);

        printk(KERN_INFO "QVIT: %s() : #### no registered private ioctl function for cmd 0x%x\n", __func__, info->cmd);
    }
    else   /* standard ioctls */
    {
        cmd_id = (info->cmd - SIOCSIWCOMMIT);
        if (cmd_id < 0 || cmd_id >= N(qvit_ieee80211_handlers))
        {
            printk(KERN_INFO "QVIT: %s() : #### wrong standard ioctl 0x%x\n", __func__, info->cmd);
            return -EINVAL;
        }

        handler = qvit_ieee80211_handlers[cmd_id];
        if (handler)
            return handler(dev, info, wrqu, extra);

        printk(KERN_INFO "QVIT: %s() : #### no registered standard ioctl function for cmd 0x%x\n", __func__, info->cmd);
    }

    return -EINVAL;

}



#if 0

struct ioctl_name_tbl
{
    unsigned int ioctl;
    char *name;
};

static struct ioctl_name_tbl ioctl_names[] =
{
    {SIOCG80211STATS, "SIOCG80211STATS"},
    {SIOC80211IFDESTROY,"SIOC80211IFDESTROY"},
    {IEEE80211_IOCTL_GETKEY, "IEEE80211_IOCTL_GETKEY"},
    {IEEE80211_IOCTL_GETWPAIE,"IEEE80211_IOCTL_GETWPAIE"},
    {IEEE80211_IOCTL_RES_REQ,"IEEE80211_IOCTL_RES_REQ:"},
    {IEEE80211_IOCTL_STA_STATS, "IEEE80211_IOCTL_STA_STATS"},
    {IEEE80211_IOCTL_SCAN_RESULTS,"IEEE80211_IOCTL_SCAN_RESULTS"},
    {IEEE80211_IOCTL_STA_INFO, "IEEE80211_IOCTL_STA_INFO"},
    {IEEE80211_IOCTL_GETMAC, "IEEE80211_IOCTL_GETMAC"},
    {IEEE80211_IOCTL_P2P_BIG_PARAM, "IEEE80211_IOCTL_P2P_BIG_PARAM"},
    {IEEE80211_IOCTL_GET_SCAN_SPACE, "IEEE80211_IOCTL_GET_SCAN_SPACE"},
    {0, NULL}
};
#endif

void qvit_ieee80211_ioctl_vattach(struct net_device *dev)
{
    int index;
#ifdef QVIT_DEBUG
    printk(KERN_INFO "QVIT: %s called\n", __FUNCTION__);
#endif
    /* initialize handler array */
    for(index = 0; index < N(qvit_ieee80211_wrapper_handlers); index++)
    {
        if(qvit_ieee80211_handlers[index])
            qvit_ieee80211_wrapper_handlers[index] = qvit_ieee80211_ioctl_wrapper;
        else
            qvit_ieee80211_wrapper_handlers[index] = NULL;
    }

    for(index = 0; index < N(qvit_ieee80211_priv_wrapper_handlers); index++)
    {
        if(qvit_ieee80211_priv_handlers[index])
        {
#ifdef QVIT_DEBUG
            printk(KERN_INFO "QVIT: %s: priv index [%d]\n", __FUNCTION__, index);
#endif
            qvit_ieee80211_priv_wrapper_handlers[index] = qvit_ieee80211_ioctl_wrapper;
        }
        else
            qvit_ieee80211_priv_wrapper_handlers[index] = NULL;
    }

    dev->wireless_handlers = &qvit_ieee80211_iw_handler_def;

    printk(KERN_INFO "QVIT: %s() num_private [%d]\n", __FUNCTION__, qvit_ieee80211_iw_handler_def.num_private);

}

#undef N


struct net_device *qvit_create_vdev(VDEV_FUNC *cp, char *mac_address, struct ol_ath_softc_net80211 *scn)
{

    struct net_device *dev;
    DiagVi_PrivType *ap;
    int ret;

    // try to reuse device if it was added before

    dev = dev_get_by_name(&init_net, cp->vdev_name);

    if (dev == NULL)
    {
        dev = alloc_etherdev(sizeof(DiagVi_PrivType));
        if (dev == NULL)
        {
            printk(KERN_ERR "QVIT: %s() - Error: alloc_etherdev failed\n", __FUNCTION__);
            return NULL;
        }
#ifdef QVIT
        printk(KERN_INFO "QVIT: %s() Allocated VI device [%s] dev 0x%p\n", __FUNCTION__, cp->vdev_name, dev);
#endif
        sprintf(dev->name, "%s", cp->vdev_name );
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
        dev->netdev_ops = &diag_vi_ops;
#else
        dev->open               = diag_vi_open;
        dev->hard_start_xmit    = diag_vi_hard_start_xmit;
        dev->stop               = diag_vi_stop;
//    dev->do_ioctl           = diag_vi_ioctl;
        dev->set_mac_address    = diag_vi_set_address;
        dev->set_multicast_list = diag_vi_set_multicast_list;
        dev->get_stats          = diag_vi_get_stats;
        dev->change_mtu         = diag_vi_change_mtu;
#endif
        dev->mtu                = 3072;

        //qvit_net_dev = dev;

        if ((ret = register_netdevice(dev)))
        {
            printk(KERN_ERR "QVIT: %s: device %s register failed ret = %d\n", __FUNCTION__, dev->name, ret);
            return NULL;
        }
        else
        {
            ap = netdev_priv(dev);
            memset(ap, 0, sizeof(DiagVi_PrivType));
            ap->scn = scn;
            ap->dev = dev;
            ap->vdev_id = cp->vdev_id;
            memset(&ap->stats, 0, sizeof(ap->stats));

            netif_carrier_off(dev);
            netif_stop_queue(dev);

            if (mac_address)
                memcpy(dev->dev_addr, mac_address, ETH_ALEN);
#ifdef QVIT
            printk(KERN_INFO "QVIT: %s()  device (%s) successfully created. Index [%d]\n", __FUNCTION__, dev->name, dev->ifindex);
#endif
            qvit_ieee80211_ioctl_vattach(dev);
        }
    }
    else
    {
        dev_put(dev); // Unlock device
        dev->mtu                = 3072; // reset to this value
        printk(KERN_ERR "QVIT: %s() Device (%s) already allocated. Reusing!\n", __FUNCTION__, dev->name);
        
        ap = netdev_priv(dev);
        memset(ap, 0, sizeof(DiagVi_PrivType));
        ap->scn = scn;
        ap->dev = dev;
        ap->vdev_id = cp->vdev_id;
        memset(&ap->stats, 0, sizeof(ap->stats));
        
        netif_carrier_off(dev);
        netif_stop_queue(dev);
        
        if (mac_address)
        {
            if (memcmp(dev->dev_addr, mac_address, ETH_ALEN) != 0)
                memcpy(dev->dev_addr, mac_address, ETH_ALEN);
        }
        
        qvit_ieee80211_ioctl_vattach(dev);
    }
    /* Up the device  */
    dev_open(dev);

    return dev;
}


ol_txrx_peer_handle qvit_get_peer_handle_from_mac_addr(ol_txrx_vdev_handle txrx_vdev, u_int8_t *mac_addr)
{

    struct ol_txrx_peer_t *peer=NULL;

    TAILQ_FOREACH(peer, &txrx_vdev->peer_list, peer_list_elem)
    {
        if (memcmp((void *)&peer->mac_addr.raw[0], (void *)mac_addr, 6) == 0)
        {
#ifdef QVIT_DEBUG
            printk(KERN_INFO "QVIT: Found peer 0x%p\n", peer);
#endif
            return peer;
        }
    }
    printk(KERN_ERR "QVIT: Peer not found\n");
    return NULL;
}


void qvit_delete_linux_vdev(struct net_device *gdev)
{

    // Now we have to delete all peers attached to this vdev
    if (gdev->reg_state == NETREG_REGISTERED)
        unregister_netdev(gdev);
    free_netdev(gdev);
    return;
}
#if TXRX_DEBUG_LEVEL > 4
void
ol_txrx_peer_find_display(ol_txrx_pdev_handle pdev, int indent);
#endif

void  ol_txrx_peer_find_hash_erase(struct ol_txrx_pdev_t *pdev);


void ol_txrx_peer_unref_delete(struct ol_txrx_peer_t *peer);

void qvit_delete_peers(int vdevId)
{

    char devName[10];
    char number[10];
    //ol_txrx_peer_handle txrx_peer;
    struct net_device *gdev;
    DiagVi_PrivType *ap;
    struct ol_ath_softc_net80211 *scn;
    strcpy(devName, QVIT_DEVICE);
    sprintf(number,"%d", vdevId);
    strcat(devName, number);
    gdev = __dev_get_by_name(&init_net, devName);

    if (gdev)
    {
        ap = netdev_priv(gdev);
        scn = ap->scn;
        ol_txrx_peer_find_hash_erase(scn->pdev_txrx_handle);
#if TXRX_DEBUG_LEVEL > 4
        ol_txrx_peer_find_display(scn->pdev_txrx_handle, 3);
#endif
    }
    else
        printk(KERN_ERR "QVIT: device %s not found\n", devName);
}


void qvit_set_ethtool(struct net_device *dev)
{
    // This function  is for pDEV only
    if (nl_sock == NULL)
    {
        if (qvit_netlink_init(&nl_sock, 0))
            printk(KERN_ERR "QVIT: %s() qvit_netlink_init failed\n", __FUNCTION__);
    }
    dev->ethtool_ops = &diag_vi_ethtool_ops;
    return;
}

void qvit_nlsendmsg_ucast(const char *message, int pid)
{
    struct nlmsghdr *nlh;
    struct sk_buff *nl_skb;

    nl_skb = alloc_skb(NLMSG_SPACE(MAX_QVIT_NETLINK_PAYLOAD),GFP_ATOMIC);
    if(nl_skb==NULL)
    {
        printk(KERN_ERR "QVIT: %s() alloc_skb failed\n", __FUNCTION__);
        return;
    }

    nlh = nlmsg_put(nl_skb, 0, 0, 0, NLMSG_SPACE(MAX_QVIT_NETLINK_PAYLOAD) - sizeof(struct nlmsghdr), 0);
    NETLINK_CB(nl_skb).pid = 0;
    strcpy(NLMSG_DATA(nlh),message);
    netlink_unicast(nl_sock, nl_skb, pid, MSG_DONTWAIT);
}

void qvit_nlsendmsg_bcast(const char *message, int group)
{
    struct nlmsghdr *nlh;
    struct sk_buff *nl_skb;

    nl_skb = alloc_skb(NLMSG_SPACE(MAX_QVIT_NETLINK_PAYLOAD),GFP_ATOMIC);
    if(nl_skb==NULL)
    {
        printk(KERN_ERR "QVIT: %s() alloc_skb failed\n", __FUNCTION__);
        return;
    }

    nlh = nlmsg_put(nl_skb, 0, 0, 0, NLMSG_SPACE(MAX_QVIT_NETLINK_PAYLOAD) - sizeof(struct nlmsghdr), 0);
    NETLINK_CB(nl_skb).pid = 0;
    NETLINK_CB(nl_skb).dst_group = group;
    strcpy(NLMSG_DATA(nlh),message);
    netlink_broadcast(nl_sock, nl_skb, 0, group, GFP_KERNEL);
}

int qvit_netlink_init(struct sock **nl_sock, int groups)
{
    *nl_sock = netlink_kernel_create(&init_net,QVIT_NETLINK_PORT, groups, NULL, NULL, THIS_MODULE);
    if(! *nl_sock)
    {
        printk(KERN_ERR "QVIT: %s() netlink_kernel_create failed!\n", __FUNCTION__);
        return -ENOMEM;
    }
#ifdef QVIT_DEBUG
    printk(KERN_INFO "QVIT: %s() netlink_kernel_create OK -  nl_sock [0x%x]\n", __FUNCTION__, (unsigned int)*nl_sock);
#endif
#ifdef QVIT_NETLINK_TEST
#ifdef QVIT_DEBUG
    printk(KERN_INFO "QVIT: %s() Calling setup_timer for qvit_test_timer\n", __FUNCTION__);
#endif
    setup_timer(&qvit_test_timer, qvit_test_timer_func, QVIT_NETLINK_GROUP);
    mod_timer(&qvit_test_timer, jiffies + msecs_to_jiffies(QVIT_NETLINK_TIME_VAL));
#endif
    return 0;
}

void qvit_netlink_exit(struct sock *nl_sock)
{
    if(NULL!=nl_sock)
    {
#ifdef QVIT_NETLINK_TEST
#ifdef QVIT_DEBUG
        printk(KERN_INFO "QVIT: %s() Calling del_timer_sync\n", __FUNCTION__);
#endif
        del_timer_sync(&qvit_test_timer);
#endif
#ifdef QVIT_DEBUG
        printk(KERN_INFO "QVIT: %s() Closing netlink socket\n", __FUNCTION__);
#endif
        netlink_kernel_release(nl_sock);
        nl_sock = NULL;
    }
}

void qvit_cleanup(void)
{
    int i;
    struct net_device *gdev;
    char devname[10];
    for (i=0; i<QVIT_MAX_DEV ; i++)
    {
        sprintf(devname, "qvit%d", i);
        gdev = __dev_get_by_name(&init_net, devname);
        if (gdev)
        {
#ifdef QVIT_DEBUG
            printk(KERN_INFO "QVIT: %s() dev_name [%s]\n", __FUNCTION__, devname);
#endif
            diag_vi_stop(gdev);
            qvit_delete_linux_vdev(gdev);
        }
    }

    qvit_netlink_exit(nl_sock);
    // add more later
}

#ifdef QVIT_NETLINK_TEST
static void qvit_test_timer_func(unsigned long __data)
{
    static int seq = 0;
    char data[32];
    scnprintf(data, sizeof(data), "counter = %u",
              seq) + 1;
#ifdef QVIT_DEBUG
    printk(KERN_INFO "QVIT: in %s sequence [%d]\n", __FUNCTION__, seq);
#endif
    qvit_nlsendmsg_bcast(data, (int)__data);
    seq++;
    mod_timer(&qvit_test_timer, jiffies + msecs_to_jiffies(QVIT_NETLINK_TIME_VAL));
}
#endif

#if 0
// For debug only
// ============================== do not delete =========================
void osif_vdev_delete_done(void *context)
{
    ol_txrx_vdev_handle vdev;
    vdev = *((ol_txrx_vdev_handle *) context);
#ifdef QVIT_DEBUG
    printf(KERN_INFO "QVIT: vdev 0x%p deletion complete\n", vdev);
#endif
}


static A_UINT32 rand(void)
{
    /* Park Miller rand */
    static A_UINT32 next_rand = 1111;

    next_rand = (next_rand * (16807));
    next_rand &= 0x7fffffff;

    return next_rand;
}


void framework_osif_tx_gen(
    ol_txrx_tx_fp vdev_tx,
    ol_txrx_vdev_handle txrx_vdev,
    int num_msdus,
    int batch_size_min,
    int batch_size_max,
    int msdu_len)
{
    a_status_t status;




    adf_nbuf_t tx_msdu = NULL;

    // WAL TX: seg0 buf: Length 50
    char seg0[114] = {0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0x8, 0x0, 0x45, 0x0, 	\
                      0x0, 0x6c, 0x0, 0x0, 0x5, 0x67, 0x32, 0x11, 0x23, 0xc6, 0x64, 0x33, 0xdc, 0xff, 0x2a, 0x55, 	\
                      0x1f, 0xcd, 0x40, 0x40, 0x40, 0x40, 0x0, 0x0, 0x0, 0x40, 0x0, 0x0, 0x0, 0x80, 0x23, 0x23, 	\
                      0x23, 0x23, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, \
                      0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, \
                      0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, \
                      0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, \
                      0x93, 0x94
                     };

    // Send only one packet and one seg for now
    adf_nbuf_t new = adf_nbuf_alloc(NULL, 114, 0, 4, FALSE);
    adf_nbuf_set_next(new, tx_msdu);
    adf_nbuf_put_tail(new, 114);

    adf_os_mem_copy(adf_nbuf_data(new), &seg0[0], 114);
#ifdef QVIT_DEBUG
    printk(KERN_INFO "Pkt len=%d, data=0x%x\n", (int)adf_nbuf_len(new),(int) adf_nbuf_data(new));
#endif
    status = adf_nbuf_map(NULL /* pdev->osdev */, new, ADF_OS_DMA_TO_DEVICE);
    tx_msdu = new;
#ifdef QVIT_DEBUG
    printk(KERN_INFO "\tSending packet...\n");
#endif

    vdev_tx(txrx_vdev, tx_msdu);

#ifdef QVIT_DEBUG
    printk(KERN_INFO "\tSend packet complete\n");
#endif
    /*
            adf_nbuf_t new = adf_nbuf_alloc(NULL, msdu_len, 0, 4);
            adf_nbuf_set_next(new, tx_msdu);
            adf_nbuf_put_tail(new, msdu_len);
    	adf_os_mem_copy(adf_nbuf_data(msdu), &seg[1], 64);
            tx_msdu = new;
    */

}


int ol_ath_qvit_start_xmit(ol_scn_t scn_handle)
{
#ifdef QVIT_DEBUG
    printk("start pkt xmit\n");
#endif

    framework_osif_tx_gen(vdev_tx, txrx_vdev, 10, 1, 3, 40);

    /* **************** Debugging Messages ********************** */
    //ol_txrx_pdev_display(txrx_pdev, 4); // handle, display indent
    //ol_txrx_vdev_display(txrx_vdev, 8);
    //ol_txrx_peer_display(txrx_peer, 12);
    //ol_txrx_stats_display(txrx_pdev);
    /* ********************************************************** */
    ol_txrx_peer_detach(txrx_peer);
    ol_txrx_vdev_detach(txrx_vdev, osif_vdev_delete_done /* callback */, (void *) &txrx_vdev /* context */);
#ifdef QVIT_DEBUG
    printk("start xmit exit...\n");
#endif
    return (0);

}
#endif
