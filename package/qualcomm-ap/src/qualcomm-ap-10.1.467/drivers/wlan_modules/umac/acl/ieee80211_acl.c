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


#include <ieee80211_var.h>
#include "ieee80211_ioctl.h"

#if UMAC_SUPPORT_ACL

/*! \file ieee80211_acl.c
**  \brief 
**
** Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
** Copyright (c) 2004-2007 Atheros Communications, Inc.
**
 */

/*
 * IEEE 802.11 MAC ACL support.
 *
 * When this module is loaded the sender address of each received
 * frame is passed to the iac_check method and the module indicates
 * if the frame should be accepted or rejected.  If the policy is
 * set to ACL_POLICY_OPEN then all frames are accepted w/o checking
 * the address.  Otherwise, the address is looked up in the database
 * and if found the frame is either accepted (ACL_POLICY_ALLOW)
 * or rejected (ACL_POLICY_DENT).
 */

enum
{
    ACL_POLICY_OPEN             = 0,/* open, don't check ACL's */
    ACL_POLICY_ALLOW	        = 1,/* allow traffic from MAC */
    ACL_POLICY_DENY             = 2,/* deny traffic from MAC */
};

#define	ACL_HASHSIZE	32

/* 
 * The ACL list is accessed from process context when ioctls are made to add,
 * delete mac entries or set/get policy (read/write operations). It is also 
 * accessed in tasklet context for read purposes only. Hence, we must use
 * spinlocks with DPCs disabled to protect this list. 
 * 
 * It may be noted that ioctls are serialized by the big kernel lock in Linux 
 * and so the process context code does not use mutual exclusion. It may not
 * be true for other OSes. In such cases, this code must be made safe for 
 * ioctl mutual exclusion. 
 */
struct ieee80211_acl_entry
{
    /* 
     * list element for linking on acl_list 
     */
    TAILQ_ENTRY(ieee80211_acl_entry)     ae_list; 

    /* 
     * list element for linking on acl_hash list 
     */
    LIST_ENTRY(ieee80211_acl_entry)      ae_hash; 

    u_int8_t                             ae_macaddr[IEEE80211_ADDR_LEN];
};
struct ieee80211_acl
{
    osdev_t                              acl_osdev;
    spinlock_t                           acl_lock;
    int	                                 acl_policy;
    TAILQ_HEAD(, ieee80211_acl_entry)    acl_list; /* list of all acl_entries */
    ATH_LIST_HEAD(, ieee80211_acl_entry) acl_hash[ACL_HASHSIZE];
};

/* 
 * simple hash is enough for variation of macaddr 
 */
#define	ACL_HASH(addr)	\
    (((const u_int8_t *)(addr))[IEEE80211_ADDR_LEN - 1] % ACL_HASHSIZE)

static void acl_free_all_locked(ieee80211_acl_t acl);

int ieee80211_acl_attach(wlan_if_t vap)
{
    ieee80211_acl_t acl;

    if (vap->iv_acl)
        return EOK; /* already attached */

    acl = (ieee80211_acl_t) OS_MALLOC(vap->iv_ic->ic_osdev, 
                                sizeof(struct ieee80211_acl), 0);
    if (acl) {
        OS_MEMZERO(acl, sizeof(struct ieee80211_acl));
        acl->acl_osdev  = vap->iv_ic->ic_osdev;
        vap->iv_acl = acl;

        spin_lock_init(&acl->acl_lock);
        TAILQ_INIT(&acl->acl_list);
        acl->acl_policy = ACL_POLICY_OPEN;

        return EOK;
    }

    return ENOMEM;
}

int ieee80211_acl_detach(wlan_if_t vap)
{
    ieee80211_acl_t acl;

    if (vap->iv_acl == NULL)
        return EINPROGRESS; /* already detached or never attached */

    acl = vap->iv_acl;
    acl_free_all_locked(acl);

    spin_lock_destroy(&acl->acl_lock);

    OS_FREE(acl);

    vap->iv_acl = NULL;

    return EOK;
}

static __inline struct ieee80211_acl_entry * 
_find_acl(ieee80211_acl_t acl, const u_int8_t *macaddr)
{
    struct ieee80211_acl_entry *entry;
    int hash;

    hash = ACL_HASH(macaddr);
    LIST_FOREACH(entry, &acl->acl_hash[hash], ae_hash) {
        if (IEEE80211_ADDR_EQ(entry->ae_macaddr, macaddr))
            return entry;
    }
    return NULL;
}

/* 
 * This function is always called from tasklet context and it may be noted
 * that the same tasklet is not scheduled on more than one CPU at the same 
 * time. The user context functions that modify the ACL use spin_lock_dpc 
 * which disable softIrq on the current CPU. However, a softIrq scheduled 
 * on another CPU could execute the rx tasklet. Hence, protection is needed 
 * here. spinlock is sufficient as it disables kernel preemption and if the 
 * user task is accessing this list, the rx tasklet will wait until the user 
 * task releases the spinlock. The original code didn't use any protection.
 */
int 
ieee80211_acl_check(wlan_if_t vap, const u_int8_t mac[IEEE80211_ADDR_LEN])
{
    ieee80211_acl_t acl = vap->iv_acl;

    if (acl == NULL) return 1;

    /* EV : 89216
     * WPS2.0 : Ignore MAC Address Filtering if WPS Enabled
     * Display the message.
     * return 1 to report success
     */
    if(vap->iv_wps_mode){
        printk("\n WPS Enabled : Ignoring MAC Filtering\n");
        return 1;
    }

    switch (acl->acl_policy) {
        struct ieee80211_acl_entry *entry;
        case ACL_POLICY_OPEN:
            return 1;
        case ACL_POLICY_ALLOW:
            spin_lock(&acl->acl_lock);
            entry = _find_acl(acl, mac);
            spin_unlock(&acl->acl_lock);
            return entry != NULL;
        case ACL_POLICY_DENY:
            spin_lock(&acl->acl_lock);
            entry = _find_acl(acl, mac);
            spin_unlock(&acl->acl_lock);
            return entry == NULL;
    }
    return 0;		/* should not happen */
}

/* 
 * The ACL list is modified when in user context and the list needs to be 
 * protected from rx tasklet. Using spin_lock alone won't be sufficient as
 * that only disables task pre-emption and not irq or softIrq preemption.
 * Hence, effective protection is possible only by disabling softIrq on
 * local CPU and spin_lock_dpc needs to be used.
 */
int 
ieee80211_acl_add(wlan_if_t vap, const u_int8_t mac[IEEE80211_ADDR_LEN])
{
    ieee80211_acl_t acl = vap->iv_acl;
    struct ieee80211_acl_entry *entry, *new;
    int hash, rc;

    if (acl == NULL) {
        rc = ieee80211_acl_attach(vap);
        if (rc != EOK) return rc;
        acl = vap->iv_acl;
    }

    new = (struct ieee80211_acl_entry *) OS_MALLOC(acl->acl_osdev, 
                                              sizeof(struct ieee80211_acl_entry), 0);
    if (new == NULL) return ENOMEM;

    spin_lock_dpc(&acl->acl_lock);
    hash = ACL_HASH(mac);
    LIST_FOREACH(entry, &acl->acl_hash[hash], ae_hash) {
        if (IEEE80211_ADDR_EQ(entry->ae_macaddr, mac)) {
            spin_unlock_dpc(&acl->acl_lock);
            OS_FREE(new);
            return EEXIST;
        }
    }
    IEEE80211_ADDR_COPY(new->ae_macaddr, mac);
    TAILQ_INSERT_TAIL(&acl->acl_list, new, ae_list);
    LIST_INSERT_HEAD(&acl->acl_hash[hash], new, ae_hash);
    spin_unlock_dpc(&acl->acl_lock);

    return 0;
}

static void
_acl_free(ieee80211_acl_t acl, struct ieee80211_acl_entry *entry)
{
    TAILQ_REMOVE(&acl->acl_list, entry, ae_list);
    LIST_REMOVE(entry, ae_hash);
    OS_FREE(entry);
}

int 
ieee80211_acl_remove(wlan_if_t vap, const u_int8_t mac[IEEE80211_ADDR_LEN])
{
    ieee80211_acl_t acl = vap->iv_acl;
    struct ieee80211_acl_entry *entry;

    if (acl == NULL) return EINVAL;

    spin_lock_dpc(&acl->acl_lock);
    entry = _find_acl(acl, mac);
    if (entry != NULL)
        _acl_free(acl, entry);
    spin_unlock_dpc(&acl->acl_lock);

    return (entry == NULL ? ENOENT : 0);
}

int 
ieee80211_acl_get(wlan_if_t vap, u_int8_t *macList, int len, int *num_mac)
{
    ieee80211_acl_t acl = vap->iv_acl;
    struct ieee80211_acl_entry *entry;
	int rc;

    if (acl == NULL) {
        rc = ieee80211_acl_attach(vap);
        if (rc != EOK) return rc;
        acl = vap->iv_acl;
    }

    if ((macList == NULL) || (!len)) {
        return ENOMEM;
	}

    *num_mac = 0;

    spin_lock_dpc(&acl->acl_lock);
    TAILQ_FOREACH(entry, &acl->acl_list, ae_list) {
        len -= IEEE80211_ADDR_LEN;
        if (len < 0) {
            spin_unlock_dpc(&acl->acl_lock);
            return E2BIG;
        }
        IEEE80211_ADDR_COPY(&(macList[*num_mac*IEEE80211_ADDR_LEN]), entry->ae_macaddr);
        (*num_mac)++;
    }
    spin_unlock_dpc(&acl->acl_lock);

    return 0;
}

static void
acl_free_all_locked(ieee80211_acl_t acl)
{
    struct ieee80211_acl_entry *entry;

    spin_lock_dpc(&acl->acl_lock); 
    while (!TAILQ_EMPTY(&acl->acl_list)) {
        entry = TAILQ_FIRST(&acl->acl_list);
        _acl_free(acl, entry);
    }
    spin_unlock_dpc(&acl->acl_lock);
}

int ieee80211_acl_flush(wlan_if_t vap)
{
    ieee80211_acl_t acl = vap->iv_acl;
    if (acl == NULL) return EINVAL;
    acl_free_all_locked(acl);
    return 0;
}

int ieee80211_acl_setpolicy(wlan_if_t vap, int policy)
{
    ieee80211_acl_t acl = vap->iv_acl;
    int rc;

    if (acl == NULL) {
        rc = ieee80211_acl_attach(vap);
        if (rc != EOK) return rc;
        acl = vap->iv_acl;
    }
    switch (policy)
    {
        case IEEE80211_MACCMD_POLICY_OPEN:
            acl->acl_policy = ACL_POLICY_OPEN;
            break;
        case IEEE80211_MACCMD_POLICY_ALLOW:
            acl->acl_policy = ACL_POLICY_ALLOW;
            break;
        case IEEE80211_MACCMD_POLICY_DENY:
            acl->acl_policy = ACL_POLICY_DENY;
            break;
        default:
            return EINVAL;
    }
    return 0;
}

int ieee80211_acl_getpolicy(wlan_if_t vap)
{
    ieee80211_acl_t acl = vap->iv_acl;
    int rc;
    
    if (acl == NULL) {
        rc = ieee80211_acl_attach(vap);
        if (rc != EOK) return rc;
        acl = vap->iv_acl;
    }

    if (acl == NULL) return EINVAL;
    return acl->acl_policy;
}

int wlan_set_acl_policy(wlan_if_t vap, int policy)
{
    switch (policy) {
    case IEEE80211_MACCMD_POLICY_OPEN:
    case IEEE80211_MACCMD_POLICY_ALLOW:
    case IEEE80211_MACCMD_POLICY_DENY:
        ieee80211_acl_setpolicy(vap, policy);
        break;
    case IEEE80211_MACCMD_FLUSH:
        ieee80211_acl_flush(vap);
        break;
    case IEEE80211_MACCMD_DETACH:
        ieee80211_acl_detach(vap);
        break;
    }    

    return 0;
}

int wlan_get_acl_policy(wlan_if_t vap)
{
    return ieee80211_acl_getpolicy(vap);
}

int wlan_set_acl_add(wlan_if_t vap, const u_int8_t mac[IEEE80211_ADDR_LEN])
{
    return ieee80211_acl_add(vap, mac);
}

int wlan_set_acl_remove(wlan_if_t vap, const u_int8_t mac[IEEE80211_ADDR_LEN])
{
    return ieee80211_acl_remove(vap, mac);
}

int wlan_get_acl_list(wlan_if_t vap, u_int8_t *macList, int len, int *num_mac)
{
    return ieee80211_acl_get(vap, macList, len, num_mac);
}
#endif /* UMAC_SUPPORT_ACL */

