/*
 *  Copyright (c) 2010, Atheros Communications Inc.
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
#include "linux/if.h"
#include "linux/socket.h"
#include <net/rtnetlink.h>
#include <net/sock.h>

#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/cache.h>
#include <linux/proc_fs.h>

#if ATH_SUPPORT_LINKDIAG_EXT
#include "ald_netlink.h"

struct ald_netlink *ald_nl = NULL;

int ald_init_netlink(void)
{
    if (ald_nl == NULL) {
        ald_nl = (struct ald_netlink *)kzalloc(sizeof(struct ald_netlink), GFP_KERNEL);

        if(ald_nl == NULL)
            return -ENODEV;

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,24)
        ald_nl->ald_sock = (struct sock *)netlink_kernel_create(&init_net, NETLINK_ALD,
                                   1, &ald_nl_receive, NULL, THIS_MODULE);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,22)
        ald_nl->ald_sock = (struct sock *)netlink_kernel_create(NETLINK_ALD,
                                   1, &ald_nl_receive, (struct mutex *) NULL, THIS_MODULE);
#else		
        ald_nl->ald_sock = (struct sock *)netlink_kernel_create(NETLINK_ALD,
                                   1, &ald_nl_receive, THIS_MODULE);
#endif

        if (ald_nl->ald_sock == NULL) {
            kfree(ald_nl);
            ald_nl = NULL;

            printk("%s NETLINK_KERNEL_CREATE FAILED\n", __func__);

            return -ENODEV;
        }

        atomic_set(&ald_nl->ald_refcnt, 1);
        ald_nl->ald_pid = WLAN_DEFAULT_ALD_NETLINK_PID;
    } else {
        atomic_inc(&ald_nl->ald_refcnt);
    }

    return 0;    
}

int ald_destroy_netlink(void)
{
    if (!atomic_dec_return(&ald_nl->ald_refcnt)) {
        if (ald_nl->ald_sock)
            sock_release(ald_nl->ald_sock->sk_socket);

        kfree(ald_nl);
        ald_nl = NULL;
    }

    return 0;
}


static void ald_notify(wlan_if_t vap, u_int32_t info_cmd, u_int32_t info_len, void *info_data)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh = NULL;
    u_int8_t *nldata = NULL;
    u_int32_t pid = ald_nl->ald_pid;

    if (pid == WLAN_DEFAULT_ALD_NETLINK_PID) return;

    skb = nlmsg_new(info_len, GFP_ATOMIC);
    if (!skb) {
        printk("%s: No memory, info_cmd = %d\n", __func__, info_cmd);
        return;
    }

    nlh = nlmsg_put(skb, pid, 0, info_cmd, info_len, 0); 
    if (!nlh) {
        printk("%s: nlmsg_put() failed, info_cmd = %d\n", __func__, info_cmd);
        return;
    }

    nldata = NLMSG_DATA(nlh);
    memcpy(nldata, info_data, info_len);

    NETLINK_CB(skb).pid = 0;        /* from kernel */
    NETLINK_CB(skb).dst_group = 0;  /* unicast */
    netlink_unicast(ald_nl->ald_sock, skb, pid, MSG_DONTWAIT);
}

static void ald_send_info_iter_func(wlan_if_t vap)
{
    osif_dev *osifp;
    struct net_device *dev;
    struct ald_stat_info info;
    
    osifp = (osif_dev *)wlan_vap_get_registered_handle(vap);

    if (osifp->is_up && ald_nl->ald_pid != WLAN_DEFAULT_ALD_NETLINK_PID) {
        vap->iv_ald->staticp = &info;

        info.cmd = IEEE80211_ALD_ALL;

        dev = ((osif_dev *)vap->iv_ifp)->netdev;
        strcpy(info.name, dev->name);

        ath_net80211_ald_get_statistics(vap);

        ald_notify(vap, IEEE80211_ALD_ALL, sizeof(info), &info);
    }
}   

static void ald_nl_hifitbl_update(wlan_if_t vap, struct nlmsghdr *nlh)
{
    if (!vap->iv_me->me_hifi_enable)
        return;

    write_lock(&vap->iv_me->me_hifi_lock);
    if (nlh->nlmsg_len <= NLMSG_LENGTH(0)) {
        if (vap->iv_me->me_hifi_table) {
            kfree(vap->iv_me->me_hifi_table);
            vap->iv_me->me_hifi_table = NULL;
        }
    } else {
        unsigned int hifi_table_size = sizeof(*(vap->iv_me->me_hifi_table));
        void *hifi_table = kzalloc(hifi_table_size, GFP_ATOMIC);

        if (nlh->nlmsg_len - NLMSG_HDRLEN < hifi_table_size)
            hifi_table_size = nlh->nlmsg_len - NLMSG_HDRLEN;

        if (hifi_table) {
            memcpy(hifi_table, NLMSG_DATA(nlh), hifi_table_size);
            if (vap->iv_me->me_hifi_table)
                kfree(vap->iv_me->me_hifi_table);
            vap->iv_me->me_hifi_table = hifi_table;
        }
    }
    write_unlock(&vap->iv_me->me_hifi_lock);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,24)
static void ald_nl_receive(struct sk_buff *__skb)
#else
static void ald_nl_receive(struct sock *sk, int len)
#endif
{ 
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    wlan_if_t vap;
    u_int32_t pid;
    int32_t ifindex;
    struct net_device *dev;
    osif_dev  *osifp;

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,24)	   
	   if ((skb = skb_get(__skb)) != NULL) {
#else
	   if ((skb = skb_dequeue(&sk->sk_receive_queue)) != NULL) {
#endif	   	
        nlh = nlmsg_hdr(skb);
        pid = nlh->nlmsg_pid;
        ifindex = nlh->nlmsg_flags;
        
        dev = dev_get_by_index(&init_net, ifindex);
        if (!dev) {
            printk("%s: Invalid interface index:%d\n", __func__, ifindex);
            return;
        }

        osifp = ath_netdev_priv(dev);
        vap = osifp->os_if;

        if (ald_nl->ald_pid != pid)
            ald_nl->ald_pid = pid;

        if (nlh->nlmsg_type == IEEE80211_ALD_ALL)
            ald_send_info_iter_func(vap);
        else if (nlh->nlmsg_type == IEEE80211_ALD_MCTBL_UPDATE) {
            ald_nl_hifitbl_update(vap, nlh);
        }

        dev_put(dev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,24)	   
        kfree_skb(skb);
#endif
    }
}
	
int ald_assoc_notify(wlan_if_t vap, u_int8_t *macaddr, u_int8_t aflag)
{
    struct net_device *dev;
    wlan_chan_t chan;
    struct ald_assoc_info info; 

    info.cmd = IEEE80211_ALD_ASSOCIATE;
    dev = ((osif_dev *)vap->iv_ifp)->netdev;
    strcpy(info.name, dev->name);

    chan = wlan_get_current_channel(vap, true);
    if(chan->ic_freq * 100000 < 500000)
        info.afreq = ALD_FREQ_24G;
    else
        info.afreq = ALD_FREQ_5G;
    info.aflag = aflag;
    memcpy(info.macaddr, macaddr, IEEE80211_ADDR_LEN);

    ald_notify(vap, IEEE80211_ALD_ASSOCIATE, sizeof(info), &info);
    return 0;
}

int
ieee80211_ioctl_ald_getStatistics(struct net_device *dev, struct iw_request_info *info,
            void *w, char *extra)
{
    osif_dev *osifp = ath_netdev_priv(dev);
    wlan_if_t vap = osifp->os_if;
    int retv = 0;
    struct ald_stat_info *param = NULL;
    
    param = (struct ald_stat_info *)kmalloc(sizeof(struct ald_stat_info), GFP_KERNEL);
    if (!param)
        return -ENOMEM;
    
    if (copy_from_user(param, ((union iwreq_data *)w)->data.pointer, sizeof(struct ald_stat_info))) {
        OS_FREE(param);
        return -EFAULT;
    }
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_DEBUG,
            "%s parameter is %s 0x%x get1\n", __func__, param->name, param->cmd);
    retv = wlan_ald_get_statistics(vap, param);
    copy_to_user(((union iwreq_data *)w)->data.pointer, param, sizeof(struct ald_stat_info));
    kfree(param);
    return retv;
}
#endif
