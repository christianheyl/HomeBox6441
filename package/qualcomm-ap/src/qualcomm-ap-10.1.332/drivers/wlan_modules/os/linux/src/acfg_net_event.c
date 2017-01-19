/*
 * Copyright (c) 2008-2010, Atheros Communications Inc.
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

#include <linux/module.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/wireless.h>
#include <net/iw_handler.h>
#include <linux/if_arp.h>
#include <linux/rtnetlink.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
#include <net/net_namespace.h>
#endif
#include <net/netlink.h>
#include <net/sock.h>
#include <adf_os_types_pvt.h>
#include <acfg_api_types.h>
#include <acfg_event_types.h>
#include <acfg_drv_if.h>

struct sock *acfg_ev_sock = NULL;

/**
 * IW Events
 */
typedef a_status_t  (* __acfg_event_t)(struct net_device *,
                                      acfg_ev_data_t     *data);

#define ACFG_MAX_PAYLOAD 1024
//#define NETLINK_ACFG_EVENT 20

#define PROTO_IWEVENT(name)    \
    static a_status_t  acfg_iwevent_##name(struct net_device  *,\
                        acfg_ev_data_t *)

PROTO_IWEVENT(scan_done);
PROTO_IWEVENT(assoc_ap);
PROTO_IWEVENT(assoc_sta);
PROTO_IWEVENT(wsupp_generic);
PROTO_IWEVENT(wapi);

#define EVENT_IDX(x)    [x]

/**
 * @brief Table of functions to dispatch iw events 
 */
__acfg_event_t    iw_events[] = {
    EVENT_IDX(ACFG_EV_SCAN_DONE)    = acfg_iwevent_scan_done,
    EVENT_IDX(ACFG_EV_ASSOC_AP)     = acfg_iwevent_assoc_ap,
    EVENT_IDX(ACFG_EV_ASSOC_STA)    = acfg_iwevent_assoc_sta,
    EVENT_IDX(ACFG_EV_DISASSOC_AP)    = NULL,
    EVENT_IDX(ACFG_EV_DISASSOC_STA)    = NULL,
    EVENT_IDX(ACFG_EV_WAPI)    = acfg_iwevent_wapi,
    EVENT_IDX(ACFG_EV_DEAUTH_AP)    = NULL,
    EVENT_IDX(ACFG_EV_DEAUTH_STA)    = NULL,
    EVENT_IDX(ACFG_EV_NODE_LEAVE)    = NULL,
};


#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,24)
static void acfg_nl_recv(struct sk_buff *__skb)
#else 
static void acfg_nl_recv(struct sock *sk, int len)
#endif
{
	/*Nothing to receive as of now*/
	return;
}

/**
 * @brief Fill a skb with an acfg event 
 *
 * @param skb
 * @param dev
 * @param type
 * @param event
 *
 * @return
 */
static
int ev_fill_info(struct sk_buff * skb, struct net_device *dev, \
                 int type, acfg_os_event_t  *event)
{
    struct ifinfomsg *r;
    struct nlmsghdr  *nlh;

    nlh = nlmsg_put(skb, 0, 0, type, sizeof(*r), 0);
    if (nlh == NULL)
        return -EMSGSIZE;

    r = nlmsg_data(nlh);
    r->ifi_family = AF_UNSPEC;
    r->__ifi_pad = 0;
    r->ifi_type = dev->type;
    r->ifi_index = dev->ifindex;
    r->ifi_flags = dev_get_flags(dev);
    r->ifi_change = 0;

    NLA_PUT_STRING(skb, IFLA_IFNAME, dev->name);

    /* Add the event in the netlink packet */
    NLA_PUT(skb, IFLA_WIRELESS, sizeof(acfg_os_event_t) , event);
    return nlmsg_end(skb, nlh);

nla_put_failure:
    nlmsg_cancel(skb, nlh);
    return -EMSGSIZE;
}

/**
 * @brief Send scan done through IW
 *
 * @param sc
 * @param data
 *
 * @return 
 */
static a_status_t
acfg_iwevent_scan_done(struct net_device *dev, acfg_ev_data_t    *data)
{
    union iwreq_data wreq = {{0}};

    /* dispatch wireless event indicating scan completed */
    wireless_send_event(dev, SIOCGIWSCAN, &wreq, NULL);

    return A_STATUS_OK;
}

/**
 * @brief Send WAPI through IW
 *
 * @param sc
 * @param data
 *
 * @return 
 */
static a_status_t
acfg_iwevent_wapi(struct net_device *dev, acfg_ev_data_t    *data)
{
    union iwreq_data wreq = {{0}};
    char *buf;
    int  bufsize;
#define WPS_FRAM_TAG_SIZE 30

    acfg_wsupp_custom_message_t *custom = (acfg_wsupp_custom_message_t *)data;

    bufsize = sizeof(acfg_wsupp_custom_message_t) + WPS_FRAM_TAG_SIZE;
    buf = kmalloc(bufsize, GFP_KERNEL);
    if (buf == NULL) return -ENOMEM;
    memset(buf,0,sizeof(buf));
    wreq.data.length = bufsize;
    memcpy(buf, custom->raw_message, sizeof(acfg_wsupp_custom_message_t));

    /* 
     * IWEVASSOCREQIE is HACK for IWEVCUSTOM to overcome 256 bytes limitation
     *
     * dispatch wireless event indicating probe request frame 
     */
    wireless_send_event(dev, IWEVCUSTOM, &wreq, buf);

    kfree(buf);

    return A_STATUS_OK;
}

/**
 * @brief Send Assoc with AP event through IW
 *
 * @param sc
 * @param data
 *
 * @return
 */
static a_status_t
acfg_iwevent_assoc_ap (struct net_device *dev, acfg_ev_data_t    *data)
{
    union iwreq_data wreq = {{0}};

    memcpy(wreq.addr.sa_data, data->assoc_ap.bssid, ACFG_MACADDR_LEN);
    wreq.addr.sa_family = ARPHRD_ETHER;

    wireless_send_event(dev, IWEVREGISTERED, &wreq, NULL);

    return A_STATUS_OK;
}

/**
 * @brief Send Assoc with AP event through IW
 *
 * @param sc
 * @param data
 *
 * @return 
 */
static a_status_t
acfg_iwevent_assoc_sta (struct net_device *dev, acfg_ev_data_t    *data)
{
    union iwreq_data wreq = {{0}};

    memcpy(wreq.addr.sa_data, data->assoc_sta.bssid, ACFG_MACADDR_LEN);
    wreq.addr.sa_family = ARPHRD_ETHER;

    wireless_send_event(dev, SIOCGIWAP, &wreq, NULL);

    return A_STATUS_OK;
}

/**
 * @brief Indicate the ACFG Eevnt to the upper layer
 *
 * @param hdl
 * @param event
 *
 * @return
 */

a_status_t
acfg_net_indicate_event(struct net_device *dev, acfg_os_event_t    *event, 
							int send_iwevent)
{
    struct sk_buff *skb;
    int err;
    __acfg_event_t   fn;
#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,24)
    if (!net_eq(dev_net(dev), &init_net))
        return A_STATUS_FAILED  ;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,24)
    skb = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_ATOMIC);
#else
    skb = nlmsg_new(NLMSG_SPACE(ACFG_MAX_PAYLOAD));
#endif
    if (!skb)
        return A_STATUS_ENOMEM ;

    err = ev_fill_info(skb, dev, RTM_NEWLINK, event);
    if (err < 0) {
        kfree_skb(skb);
        return A_STATUS_FAILED ;
    }

    NETLINK_CB(skb).dst_group = RTNLGRP_LINK;
    NETLINK_CB(skb).pid = 0;  /* from kernel */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)    
    NETLINK_CB(skb).dst_pid = 0;  /* multicast */
#endif


    /* Send event to acfg */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)   
  	err = nlmsg_multicast(acfg_ev_sock, skb, 0, RTNLGRP_NOTIFY);
#else
    rtnl_notify(skb, &init_net, 0, RTNLGRP_NOTIFY, NULL, GFP_ATOMIC);
#endif
	if (send_iwevent) {
    /* Send iw event */
    fn = iw_events[event->id];
		if (fn != NULL) {
    fn(dev, &event->data);
		}
	}
    return A_STATUS_OK ;
}

a_status_t acfg_event_netlink_init(void)
{
	if (acfg_ev_sock == NULL) {
	
#if LINUX_VERSION_CODE > KERNEL_VERSION (2,6,30)
	return A_STATUS_OK;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,24)
		acfg_ev_sock = (struct sock *)netlink_kernel_create(&init_net, 
								NETLINK_ROUTE, 1, &acfg_nl_recv, 
								NULL, THIS_MODULE);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,22)
		acfg_ev_sock = (struct sock *)netlink_kernel_create(NETLINK_ROUTE, 
								1, &acfg_nl_recv, (struct mutex *) NULL, 
								THIS_MODULE);
#else
		acfg_ev_sock = (struct sock *)netlink_kernel_create(
									NETLINK_ACFG_EVENT, 
									1, &acfg_nl_recv, THIS_MODULE);
	
#endif
		if (acfg_ev_sock == NULL) {
			printk("%s: netlink create failed\n", __func__);
			return A_STATUS_FAILED;
		}	
	}
	return A_STATUS_OK;
}

void acfg_event_netlink_delete (void)
{
	if (acfg_ev_sock) {
		sock_release(acfg_ev_sock->sk_socket);
		acfg_ev_sock = NULL;
	}
}
