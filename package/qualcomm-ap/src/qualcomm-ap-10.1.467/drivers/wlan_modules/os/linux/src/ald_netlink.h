/*
 * Copyright (c) 2004 Atheros Communications, Inc.
 * All rights reserved.
 *
 */
#ifndef _ALD_NETLINK_H_
#define _ALD_NETLINK_H_

#include <osdep.h>
#include "if_athvar.h"
#include "osif_private.h"
#include "ath_ald_external.h"

#define WLAN_DEFAULT_ALD_NETLINK_PID 0xffffff

struct ald_netlink {
    struct sock             *ald_sock;
    struct sk_buff          *ald_skb;
    struct nlmsghdr         *ald_nlh;
    u_int32_t               ald_pid;
    atomic_t                ald_refcnt;
};

extern struct net init_net;

#if ATH_SUPPORT_LINKDIAG_EXT

int ald_init_netlink(void);
int ald_destroy_netlink(void);
#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,24)
static void ald_nl_receive(struct sk_buff *__skb);
#else
static void ald_nl_receive(struct sock *sk, int len);
#endif
int ald_assoc_notify( wlan_if_t vap, u_int8_t *macaddr, u_int8_t aflag );
int ieee80211_ioctl_ald_getStatistics(struct net_device *dev, struct iw_request_info *info, void *w, char *extra);

#else /* ATH_SUPPORT_LINKDIAG_EXT */

#define ald_init_netlink()   do{}while(0)
#define ald_destroy_netlink()    do{}while(0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,24)
#define ald_nl_receive(a)    do{}while(0)
#else
#define ald_nl_receive(a, b) do{}while(0)
#endif
#define ald_assoc_notify(a, b, c)    do{}while(0)
#define ieee80211_ioctl_ald_getStatistics(a, b, c, d)   do{}while(0)

#endif /* ATH_SUPPORT_LINKDIAG_EXT */
#endif /* _ALD_NETLINK_H_ */
