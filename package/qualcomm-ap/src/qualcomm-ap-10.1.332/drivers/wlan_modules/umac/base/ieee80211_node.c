/*
 *  Copyright (c) 2008 Atheros Communications Inc. 
 * All Rights Reserved.
 * 
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * 
 */

#include "ieee80211_node_priv.h"
#include "ieee80211_wds.h"
#include "ieee80211_var.h"
#include <ieee80211_aow.h>
#ifdef ATH_HTC_MII_RXIN_TASKLET
#include "htc_thread.h"
#endif



#ifdef IEEE80211_DEBUG_REFCNT
#define node_reclaim(nt,ni)  _node_reclaim(nt,ni,__func__,__LINE__)
#endif

#define	ieee80211_node_dectestref(_ni) \
    atomic_dec_and_test(&(_ni)->ni_refcnt)

static void
ieee80211_node_table_reset(struct ieee80211_node_table *nt, struct ieee80211vap *match);

struct ieee80211_iter_arg {
    int32_t count;
    wlan_if_t vap;
    u_int32_t flag;
    struct ieee80211_node *nodes[IEEE80211_AID_DEF];
};

#define IEEE80211_NODE_ITER_F_ASSOC_STA     0x1
#define IEEE80211_NODE_ITER_F_UNASSOC_STA   0x2
static void
ieee80211_node_iter(void *arg, struct ieee80211_node *ni);
#if UMAC_SUPPORT_PROXY_ARP
void
ieee80211_node_remove_ipv6_by_node(struct ieee80211_node_table *nt, struct ieee80211_node *ni);
#endif

static struct ieee80211_node *
node_alloc(struct ieee80211vap *vap, const u_int8_t *macaddr, bool tmpnode)
{
     struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni;

    /* create a node */
    ni = (struct ieee80211_node *)OS_MALLOC(ic->ic_osdev, sizeof(struct ieee80211_node), GFP_KERNEL);
    if (ni == NULL) {
        printk("Can't create an node\n");
        return NULL;
    }
    OS_MEMZERO(ni, sizeof(struct ieee80211_node));
    return ni;
}

/*
* allocates a node ,sets up the node and inserts the node into the node table.
* the allocated node will have 2 references one for adding it to the table and the
* the other for the caller to use.
*/

struct ieee80211_node *
ieee80211_alloc_node(struct ieee80211_node_table *nt,
                     struct ieee80211vap *vap,
                     const u_int8_t *macaddr)
{
    struct ieee80211com *ic = nt->nt_ic;
    struct ieee80211_node *ni;
    rwlock_state_t lock_state;
    int hash;
    int i;
    u_int8_t ac;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);

    
    if(IEEE80211_CHK_NODE_TARGET(ic))
        return NULL;
    ni = ic->ic_node_alloc(vap, macaddr, FALSE /* not temp node */);
    if (ni == NULL) {
        /* XXX msg */
        vap->iv_stats.is_rx_nodealloc++;
        return NULL;
    }

    ni->ni_4wayhstarttime = 0;
    ni->ni_2wayhstarttime = 0;
    ni->ni_4wayhstime = 0;
    ni->ni_2wayhstime = 0;

    ieee80211_ref_node(ni);     /* mark referenced */

    atomic_set(&(ni->ni_rxfrag_lock), 0);
#ifdef IEEE80211_DEBUG_NODELEAK
    OS_RWLOCK_WRITE_LOCK(&ic->ic_nodelock,&lock_state);
    TAILQ_INSERT_TAIL(&ic->ic_nodes, ni, ni_alloc_list);
    OS_RWLOCK_WRITE_UNLOCK(&ic->ic_nodelock,&lock_state);
#endif

    /* copy some default variables from parent */
    IEEE80211_ADDR_COPY(ni->ni_macaddr, macaddr);
    ni->ni_intval = ic->ic_intval; /* default beacon interval */
    ni->ni_txpower = ic->ic_txpowlimit;	/* max power */

    /* load inactivity values */
    ni->ni_inact_reload = vap->iv_inact_init;
    ni->ni_inact = ni->ni_inact_reload;
    /* init auth mode to be open and cipher sets to be clear */
    RSN_SET_AUTHMODE(&ni->ni_rsn, IEEE80211_AUTH_OPEN);
    RSN_SET_UCAST_CIPHER(&ni->ni_rsn, IEEE80211_CIPHER_NONE);
    RSN_SET_MCAST_CIPHER(&ni->ni_rsn, IEEE80211_CIPHER_NONE);
    
    /* init our unicast / receive key state */
    ieee80211_crypto_resetkey(vap, &ni->ni_ucastkey, IEEE80211_KEYIX_NONE);
    ieee80211_crypto_resetkey(vap, &ni->ni_persta.nips_hwkey, IEEE80211_KEYIX_NONE);
    for (i = 0; i < IEEE80211_WEP_NKID; i++) {
        ieee80211_crypto_resetkey(vap, &ni->ni_persta.nips_swkey[i], IEEE80211_KEYIX_NONE);
    }
    ni->ni_ath_defkeyindex = IEEE80211_INVAL_DEFKEY;

    /* IBSS-only: mark as unassociated by default. */
    ni->ni_assoc_state = IEEE80211_NODE_ADHOC_STATE_UNAUTH_UNASSOC;

    /* 11n or 11ac */
    ni->ni_chwidth = ic->ic_cwm_get_width(ic);

#if UMAC_SUPPORT_WNM
    ni->ni_wnm = (struct ieee80211_wnm_node *) OS_MALLOC(ic->ic_osdev,
                      (sizeof(struct ieee80211_wnm_node)),0);
    
    OS_MEMSET(ni->ni_wnm, 0 , sizeof(struct ieee80211_wnm_node));
    if(ni->ni_wnm == NULL) {
        return NULL;
    }
    ieee80211_wnm_nattach(ni);
            
#endif
 /* Initialize seq no of last & 2nd last received frames to 0xffff
     This is to avoid a case where valid frame (Retry bit is set & seq no as 0)
     gets dropped (assuming it as a duplicate frame) */
     for (i = 0; i < (IEEE80211_TID_SIZE+1); i++)
     {
        ni->ni_rxseqs[i] = ni->ni_last_rxseqs[i] = 0xffff;
     }

    IEEE80211_NODE_STATE_LOCK_INIT(ni);

    /* Add the node into hash table */
    hash = IEEE80211_NODE_HASH(macaddr);
    OS_BEACON_WRITE_LOCK(&nt->nt_nodelock, &lock_state, flags);
    TAILQ_INSERT_TAIL(&nt->nt_node, ni, ni_list);
    LIST_INSERT_HEAD(&nt->nt_hash[hash], ni, ni_hash);
    ni->ni_vap = vap;

    IEEE80211_VAP_LOCK(vap);
    vap->iv_node_count++;
    IEEE80211_VAP_UNLOCK(vap);
    
    ni->ni_ic = ic;
    ni->ni_table = nt;
    ieee80211_ref_node(ni);     /* mark referenced for adding it to  the node table*/
    ni->ni_bss_node = ni;
    OS_BEACON_WRITE_UNLOCK(&nt->nt_nodelock, &lock_state, flags);
    ieee80211_node_saveq_attach(ni);

    /* set default rate and channel */
    ieee80211_node_set_chan(ni);

    //WME_UAPSD_NODE_TRIGSEQINIT(ni);
    /*
     * if vap is deleted while we are waiting for lock, delete  the node .
     */
    if (ieee80211_vap_deleted_is_set(vap)) {
        ieee80211_sta_leave(ni);
        ieee80211_free_node(ni);
        return NULL;
    }

    for (ac = 0; ac < WME_NUM_AC; ac++) {
        ni->ni_uapsd_dyn_trigena[ac] = -1;
        ni->ni_uapsd_dyn_delivena[ac] = -1;
    }

    ieee80211_admctl_init(ni);
    IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                   "%s: vap=0x%x, nodecount=%d, ni=0x%x, ni_bss_node=0x%x bss_ref=%d \n",__func__,
                   vap, vap->iv_node_count, ni, ni->ni_bss_node, ni->ni_bss_node->ni_refcnt);
    return ni;
}

/*
 * Reclaim any resources in a node and reset any critical
 * state.  Typically nodes are free'd immediately after,
 * but in some cases the storage may be reused so we need
 * to insure consistent state (should probably fix that).
 */
static void
node_cleanup(struct ieee80211_node *ni)
{
    int i;
    struct ieee80211vap *vap = ni->ni_vap;

#define       N(a)    (sizeof(a)/sizeof(a[0]))

    ASSERT(vap);
    /* 
     * Tmp node didn't attach pwr save staff, so skip ps queue
     * cleanup
     */
    if (!ieee80211node_has_flag(ni, IEEE80211_NODE_TEMP)) {
        ieee80211_node_saveq_cleanup(ni);
    }

    /*
     * Preserve SSID, WPA, and WME ie's so the bss node is
     * reusable during a re-auth/re-assoc state transition.
     * If we remove these data they will not be recreated
     * because they come from a probe-response or beacon frame
     * which cannot be expected prior to the association-response.
     * This should not be an issue when operating in other modes
     * as stations leaving always go through a full state transition
     * which will rebuild this state.
     *
     * XXX does this leave us open to inheriting old state?
     */
    if (vap) {
        /* Remove unicast and persta keys */
        if (ni->ni_ucastkey.wk_valid) {
            ieee80211_crypto_delkey(vap, &ni->ni_ucastkey, ni);
        }
        if (ni->ni_persta.nips_hwkey.wk_valid) {
            ieee80211_crypto_delkey(vap, &ni->ni_persta.nips_hwkey, ni);
        }
        for (i = 0; i < IEEE80211_WEP_NKID; i++) {
            if (ni->ni_persta.nips_swkey[i].wk_valid) {
                ieee80211_crypto_delkey(vap, &ni->ni_persta.nips_swkey[i], ni);
            }
        }
        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni, 
                       "node_cleanup: vap=0x%x, nodecount=%d, ni=0x%x, ni_bss_node=0x%x bss_ref=%d \n",
                       vap, vap->iv_node_count, ni, ni->ni_bss_node, ni->ni_bss_node->ni_refcnt);
    }


    if (ni->ni_associd && vap && (vap->iv_aid_bitmap != NULL))
        IEEE80211_AID_CLR(vap, ni->ni_associd);
    ni->ni_associd = 0;
    ni->ni_assocuptime = 0;
    wep_mbssid_node_cleanup(ni);
    ni->ni_rxkeyoff = 0;

#if ATH_SUPPORT_AOW
    ni->ni_new_assoc_done = 0;
#endif

#undef N
}

static void
node_free(struct ieee80211_node *ni)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211vap *vap = ni->ni_vap;
    int i;
#define       N(a)    (sizeof(a)/sizeof(a[0]))

    ic->ic_node_cleanup(ni);

    for (i = 0; i < N(ni->ni_rxfrag); i++) {
        if (ni->ni_rxfrag[i] != NULL) {
            wbuf_free(ni->ni_rxfrag[i]);
            ni->ni_rxfrag[i] = NULL;
        }
    }

    if (ni->ni_challenge != NULL) {
        OS_FREE(ni->ni_challenge);
        ni->ni_challenge = NULL;
    }

    if (ni->ni_wpa_ie != NULL) {
        OS_FREE(ni->ni_wpa_ie);
        ni->ni_wpa_ie = NULL;
    }

    if (ni->ni_wps_ie != NULL) {
        OS_FREE(ni->ni_wps_ie);
        ni->ni_wps_ie = NULL;
    }

    if (ni->ni_ath_ie != NULL) {
        OS_FREE(ni->ni_ath_ie);
        ni->ni_ath_ie = NULL;
    }

#if UMAC_SUPPORT_WNM

        if (ni->ni_wnm != NULL) {
            ieee80211_wnm_ndetach(ni);
            OS_FREE(ni->ni_wnm);
            ni->ni_wnm = NULL;
        }
#endif

    if (ni->ni_wme_ie != NULL) {
        OS_FREE(ni->ni_wme_ie);
        ni->ni_wme_ie = NULL;
    }

#if ATH_SUPPORT_AOW
    if (ni->ni_aow_ie != NULL) {
        OS_FREE(ni->ni_aow_ie);
        ni->ni_aow_ie = NULL;
    }        
#endif  /* ATH_SUPPORT_AOW */

    if (vap) {
        /* free unicast and persta keys */
        ieee80211_crypto_freekey(vap, &ni->ni_ucastkey);
        ieee80211_crypto_freekey(vap, &ni->ni_persta.nips_hwkey);
        for (i = 0; i < IEEE80211_WEP_NKID; i++) {
            ieee80211_crypto_freekey(vap, &ni->ni_persta.nips_swkey[i]);
        }
    }
    /* Tmp node doesn't attach the pwrsave queue */
    if (!ieee80211node_has_flag(ni, IEEE80211_NODE_TEMP)) {
        ieee80211_node_saveq_detach(ni);
    }
    ieee80211_admctl_deinit(ni);
#undef N
    OS_FREE(ni);
}

static int8_t
node_getrssi(const struct ieee80211_node *ni,  int8_t chain, u_int8_t flags)
{
    return ni->ni_rssi;
}

void
_ieee80211_free_node(struct ieee80211_node *ni)
{
    struct ieee80211vap         *vap = ni->ni_vap;
    struct ieee80211_node       *ni_bss_node = ni->ni_bss_node;

    ASSERT(vap);
    ASSERT(!ni->ni_table);

    if (ni->ni_associd && vap->iv_aid_bitmap != NULL)
        IEEE80211_AID_CLR(vap, ni->ni_associd);

    if ((ni->ni_flags & IEEE80211_NODE_TEMP) == 0) {
        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                       "%s", "station free \n");
    }

    if (ni->ni_flags & IEEE80211_NODE_TDLS)
        printk("%s %p<%s> in %s table\n", __func__, ni,
                ether_sprintf(ni->ni_macaddr),
                ni->ni_table != NULL ? ni->ni_table->nt_name : "<gone>");

#ifdef IEEE80211_DEBUG_NODELEAK
    do {
        rwlock_state_t lock_state;
        OS_RWLOCK_WRITE_LOCK(&ni->ni_ic->ic_nodelock,&lock_state);
        TAILQ_REMOVE(&ni->ni_ic->ic_nodes, ni, ni_alloc_list);
        OS_RWLOCK_WRITE_UNLOCK(&ni->ni_ic->ic_nodelock,&lock_state);
    } while(0);
#endif

    IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                   "%s: vap=0x%x, nodecount=%d, ni=0x%x, ni_bss_node=0x%x bss_ref=%d \n",__func__,
                   vap, vap->iv_node_count, ni, ni->ni_bss_node, ni->ni_bss_node->ni_refcnt);

    IEEE80211_NODE_STATE_LOCK_DESTROY(ni); 

#ifdef ATH_SUPPORT_TxBF
    if ( ni->ni_explicit_compbf || ni->ni_explicit_noncompbf || ni->ni_implicit_bf){    
        OS_CANCEL_TIMER(&(ni->ni_cv_timer));
        OS_FREE_TIMER(&(ni->ni_cv_timer));
        OS_CANCEL_TIMER(&(ni->ni_report_timer));
        OS_FREE_TIMER(&(ni->ni_report_timer));
        ni->ni_txbf_timer_initialized = 0;
        
        /* clear TxBF mode active indicator*/
        ni->ni_explicit_compbf = 0;
        ni->ni_explicit_noncompbf = 0; 
        ni->ni_implicit_bf = 0;   
    }
#endif
    ni->ni_ic->ic_node_free(ni);

    if (ni != ni_bss_node) {
        IEEE80211_VAP_LOCK(vap);
        vap->iv_node_count--;
        IEEE80211_VAP_UNLOCK(vap);
        ieee80211_free_node(ni_bss_node);
    } else {
        ieee80211_vap_bss_node_freed(vap);
    }
}

/*
 * Free a node. It is mostly used for decrementing
 * node reference count of an active ap or an associated station.
 * If this is last reference of the node (refcnt reaches 0),
 * free the memory.
 */
void
#ifdef IEEE80211_DEBUG_REFCNT
ieee80211_free_node_debug(struct ieee80211_node *ni, const char *func, int line)
#else
ieee80211_free_node(struct ieee80211_node *ni)
#endif
{
    if (ni) {
#ifdef IEEE80211_DEBUG_REFCNT
        TRACENODE(ni, func, line);

        if (ni->ni_vap) {
            ieee80211_note(ni->ni_vap,"%s ,line %u: decrease node %p <%s> refcnt to %d\n",
               func, line, ni, ether_sprintf(ni->ni_macaddr),
               ieee80211_node_refcnt(ni)-1);
        }
#endif

        if (ieee80211_node_dectestref(ni)) {
            ASSERT(ni->ni_table == NULL); /* node can not be in node table */
            _ieee80211_free_node(ni);
        }
    }
}

/*
 * Reclaim a node. It is mostly used when a node leaves the network.
 * remove it from the node table and decrement the held reference..
 * It must be called with OS_WRITE_LOCK being held.
 */
static void
#ifdef IEEE80211_DEBUG_REFCNT
_node_reclaim(struct ieee80211_node_table *nt, struct ieee80211_node *ni,
             const char *func, int line)
#else
node_reclaim(struct ieee80211_node_table *nt, struct ieee80211_node *ni)
#endif
{
#if UMAC_SUPPORT_SMARTANTENNA
    int8_t remove = 0;
    struct ieee80211_node *ni_temp = NULL;
#endif    
#ifdef IEEE80211_DEBUG_REFCNT
    TRACENODE(ni, func, line);
    ieee80211_note(ni->ni_vap,"%s ,line %u: decrease node %p <%s> refcnt to %d\n",
           func, line, ni,
           ether_sprintf(ni->ni_macaddr),
           ieee80211_node_refcnt(ni)-1);
#endif
    if (ni->ni_table == NULL ) {
        return;
    }
    ASSERT(ieee80211_node_refcnt(ni));
    if (ieee80211_node_refcnt(ni) == 0) {
        ieee80211_note(ni->ni_vap,"%s ,line %u: nide_reclaim called with 0 refcount %d\n");
    }
    /*
     * Other references are present, just remove the
     * node from the table so it cannot be found.  When
     * the references are dropped storage will be
     * reclaimed.
     */
#if UMAC_SUPPORT_SMARTANTENNA
   /* 
    * Remove the node from ni_smartant_list also
    * Check if node is present in the smart antenna training list then remove  
    */
#if SMARTANTENNA_DEBUG    
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, ">>> %s : Node removed : %s \n", __func__, ether_sprintf(ni->ni_macaddr));
#endif    
    if (!TAILQ_EMPTY(&nt->nt_smartant_node))
    {   /* if node is present then remove from training list */
        TAILQ_FOREACH(ni_temp, &nt->nt_smartant_node, smartant_nodelist) {
            if (IEEE80211_ADDR_EQ(ni_temp->ni_macaddr, ni->ni_macaddr))
            {
                remove = 1;
                break;
            }
        } 
        if (remove)
            TAILQ_REMOVE(&nt->nt_smartant_node, ni, smartant_nodelist);
        else {
#if SMARTANTENNA_DEBUG   
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s: Node is not present in Training list\n", __func__);
#endif            
        }
    }
#endif    
    TAILQ_REMOVE(&nt->nt_node, ni, ni_list);
    LIST_REMOVE(ni, ni_hash);
#if UMAC_SUPPORT_PROXY_ARP
    if (ni->ni_ipv4_addr) {
        LIST_REMOVE(ni, ni_ipv4_hash);
    }
    ieee80211_node_remove_ipv6_by_node(nt, ni);
#endif
    ni->ni_table = NULL;    /* clear reference */

#ifdef IEEE80211_DEBUG_REFCNT
    TRACENODE(ni, __func__, __LINE__);
#endif
    ieee80211_free_node(ni); /* decrement the ref count */
}

struct ieee80211_node *
#ifdef IEEE80211_DEBUG_REFCNT
_ieee80211_find_node_debug(struct ieee80211_node_table *nt, const u_int8_t *macaddr,
                     const char *func, int line)
#else
_ieee80211_find_node(struct ieee80211_node_table *nt, const u_int8_t *macaddr)
#endif
{
    struct ieee80211_node *ni;
    int hash;

    hash = IEEE80211_NODE_HASH(macaddr);
    LIST_FOREACH(ni, &nt->nt_hash[hash], ni_hash) {
        if (IEEE80211_ADDR_EQ(ni->ni_macaddr, macaddr)) {
            ieee80211_ref_node(ni);	/* mark referenced */
#ifdef IEEE80211_DEBUG_REFCNT
            ieee80211_note(ni->ni_vap,"%s ,line %u: increase node %p <%s> refcnt to %d\n",
                   func, line, ni,
                   ether_sprintf(ni->ni_macaddr),
                   ieee80211_node_refcnt(ni));
#endif
            return ni;
        }       
    }
    return NULL;
}

#ifdef IEEE80211_DEBUG_REFCNT
#define	_ieee80211_find_node(nt, mac)   \
    _ieee80211_find_node_debug(nt, mac, __func__, __LINE__)
#endif

struct ieee80211_node *
#ifdef IEEE80211_DEBUG_REFCNT
ieee80211_find_node_debug(struct ieee80211_node_table *nt, const u_int8_t *macaddr,
                          const char *func, int line)
#else
ieee80211_find_node(struct ieee80211_node_table *nt, const u_int8_t *macaddr)
#endif
{
    struct ieee80211_node *ni;
    rwlock_state_t lock_state;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);

    OS_BEACON_READ_LOCK(&nt->nt_nodelock, &lock_state, flags);
    ni = _ieee80211_find_node(nt, macaddr);
    OS_BEACON_READ_UNLOCK(&nt->nt_nodelock, &lock_state, flags);

    return ni;
}

/*
 * Return a reference to the appropriate node for sending
 * a data frame.  This handles node discovery in adhoc networks.
 */
struct ieee80211_node *
#ifdef IEEE80211_DEBUG_REFCNT
ieee80211_find_txnode_debug(struct ieee80211vap *vap, const u_int8_t *macaddr,
                            const char *func, int line)
#else
ieee80211_find_txnode(struct ieee80211vap *vap, const u_int8_t *macaddr)
#endif
{
    struct ieee80211_node_table *nt = &vap->iv_ic->ic_sta;
    struct ieee80211_node *ni = NULL;
    rwlock_state_t lock_state;

    OS_BEACON_DECLARE_AND_RESET_VAR(flags);
    
    OS_BEACON_READ_LOCK(&nt->nt_nodelock, &lock_state, flags);

#if UMAC_SUPPORT_TDLS
    /* Use the TDLS node, only if it is enabled and mode is STA*/
    if(IEEE80211_TDLS_ENABLED(vap) && vap->iv_opmode == IEEE80211_M_STA) {
        if((ni = ieee80211_find_tdlsnode(vap, macaddr))) {
            OS_BEACON_READ_UNLOCK(&nt->nt_nodelock, &lock_state, flags);
            return ni;
        }
    }
#endif
    if (vap->iv_bss) {
        if (vap->iv_opmode == IEEE80211_M_STA ||
            vap->iv_opmode == IEEE80211_M_WDS) {
                ni = ieee80211_ref_node(vap->iv_bss);
#ifdef IEEE80211_DEBUG_REFCNT
            ieee80211_note(ni->ni_vap,"%s ,line %u: increase node %p <%s> refcnt to %d\n",
                   func, line, ni,
                   ether_sprintf(ni->ni_macaddr),
                   ieee80211_node_refcnt(ni));
#endif
        }
        else if  (IEEE80211_IS_MULTICAST(macaddr)) {
            if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
                if (vap->iv_sta_assoc > 0) {
                    ni = ieee80211_ref_node(vap->iv_bss);
                }
                else {
                    /* No station associated to AP */
                    vap->iv_stats.is_tx_nonode++;
                    ni = NULL;
                }
            }
            else {
                ni = ieee80211_ref_node(vap->iv_bss);
#ifdef IEEE80211_DEBUG_REFCNT
                ieee80211_note(ni->ni_vap,"%s ,line %u: increase node %p <%s> refcnt to %d\n",
                   func, line, ni,
                   ether_sprintf(ni->ni_macaddr),
                   ieee80211_node_refcnt(ni));
#endif
            }
        }
        else {
            ni = _ieee80211_find_node(nt, macaddr);
            if (ni == NULL) {
                if( (vap->iv_opmode == IEEE80211_M_HOSTAP)  && wlan_get_param(vap, IEEE80211_FEATURE_WDS) ) {
                    ni = ieee80211_find_wds_node(nt, macaddr);
                }
            }
        }
    }
    OS_BEACON_READ_UNLOCK(&nt->nt_nodelock, &lock_state, flags);

    /*
     * Since all vaps share the same node table, we may find someone else's
     * node (sigh!).
     */
    if (ni && ni->ni_vap != vap)
    {
        ieee80211_unref_node(&ni);
        return NULL;
    }
    return ni;
}

#ifdef IEEE80211_DEBUG_REFCNT
struct ieee80211_node *
ieee80211_find_rxnode_debug(struct ieee80211com *ic,
                            const struct ieee80211_frame_min *wh,
                            const char *func, int line)
#else
struct ieee80211_node *
ieee80211_find_rxnode(struct ieee80211com *ic,
                      const struct ieee80211_frame_min *wh)
#endif
{
#define	IS_CTL(wh)  \
    ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) == IEEE80211_FC0_TYPE_CTL)
#define	IS_PSPOLL(wh)   \
    ((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) == IEEE80211_FC0_SUBTYPE_PS_POLL)
#define	IS_BAR(wh) \
    ((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) == IEEE80211_FC0_SUBTYPE_BAR)

    struct ieee80211_node_table *nt = &ic->ic_sta;
    struct ieee80211_node *ni = NULL;
    rwlock_state_t lock_state;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);

    OS_BEACON_READ_LOCK(&nt->nt_nodelock, &lock_state, flags);
    if (IS_CTL(wh) && !IS_PSPOLL(wh) && !IS_BAR(wh))
        ni = _ieee80211_find_node(nt, wh->i_addr1);
    else
        ni = _ieee80211_find_node(nt, wh->i_addr2);
    OS_BEACON_READ_UNLOCK(&nt->nt_nodelock, &lock_state, flags);
    return ni;
#undef IS_BAR
#undef IS_PSPOLL
#undef IS_CTL
}

struct ieee80211_node *
ieee80211_find_rxnode_nolock(struct ieee80211com *ic,
                      const struct ieee80211_frame_min *wh)
{
#define	IS_CTL(wh)  \
    ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) == IEEE80211_FC0_TYPE_CTL)
#define	IS_PSPOLL(wh)   \
    ((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) == IEEE80211_FC0_SUBTYPE_PS_POLL)
#define	IS_BAR(wh) \
    ((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) == IEEE80211_FC0_SUBTYPE_BAR)

    struct ieee80211_node_table *nt = &ic->ic_sta;
    struct ieee80211_node *ni = NULL;

    if (IS_CTL(wh) && !IS_PSPOLL(wh) && !IS_BAR(wh))
        ni = _ieee80211_find_node(nt, wh->i_addr1);
    else
        ni = _ieee80211_find_node(nt, wh->i_addr2);
    return ni;
#undef IS_BAR
#undef IS_PSPOLL
#undef IS_CTL
}

#ifdef IEEE80211_DEBUG_REFCNT
struct ieee80211_node *
ieee80211_ref_node_debug(struct ieee80211_node *ni,
                          const char *func, int line)
{
    atomic_inc(&(ni->ni_refcnt));
    ieee80211_note(ni->ni_vap,"%s ,line %u: increase node %p <%s> refcnt to %d\n",
           func, line, ni, ether_sprintf(ni->ni_macaddr),
           ieee80211_node_refcnt(ni));
    return ni;
}

void
ieee80211_unref_node_debug(struct ieee80211_node **ni,
                          const char *func, int line)
{
    atomic_dec(&((*ni)->ni_refcnt));
    ieee80211_note((*ni)->ni_vap,"%s ,line %u: decrease node %p <%s> refcnt to %d\n",
                   func, line, (*ni), ether_sprintf((*ni)->ni_macaddr),
                   ieee80211_node_refcnt((*ni)));
    *ni = NULL;			/* guard against use */
}

#endif 

void
ieee80211_node_authorize(struct ieee80211_node *ni)
{
    struct ieee80211com *ic = ni->ni_ic;

    ni->ni_flags |= IEEE80211_NODE_AUTH;
    ni->ni_inact_reload = ni->ni_vap->iv_inact_run;

    if (ni->ni_inact > ni->ni_inact_reload)
        ni->ni_inact = ni->ni_inact_reload;

    if (ic->ic_node_authorize) {
        ic->ic_node_authorize(ni,TRUE);
    }
}

void
ieee80211_node_unauthorize(struct ieee80211_node *ni)
{
    struct ieee80211com *ic = ni->ni_ic;

    ni->ni_flags &= ~IEEE80211_NODE_AUTH;
    ni->ni_inact_reload = ni->ni_vap->iv_inact_auth;
    if (ni->ni_inact > ni->ni_inact_reload)
        ni->ni_inact = ni->ni_inact_reload;

    if (ic->ic_node_authorize) {
        ic->ic_node_authorize(ni,FALSE);
    }
}

static void ieee80211_node_saveq_age_iter(void *arg, struct ieee80211_node *ni)
{
    struct ieee80211_iter_arg *itr_arg = (struct ieee80211_iter_arg *)arg;

    if (ni->ni_associd != 0 && itr_arg->count < IEEE80211_AID_DEF) {
        /* increment the ref count so that the node is not freed */
        itr_arg->nodes[itr_arg->count] = ieee80211_ref_node(ni);
        ++itr_arg->count;
    }

}

/*
 * age out frames in save queue in each node.
 */
static void ieee80211_timeout_node_saveq_age( struct ieee80211_node_table *nt)
{
  struct ieee80211com *ic = nt->nt_ic;
  struct ieee80211_iter_arg itr_arg;
  u_int32_t i;

  itr_arg.count=0;
  itr_arg.vap=NULL;
  itr_arg.flag=0;

  ieee80211_iterate_node(ic,ieee80211_node_saveq_age_iter,(void *)&itr_arg);
  for (i=0;i<itr_arg.count; ++i) 
  {
      /*
       * Age frames on the power save queue.
       */
      ieee80211_node_saveq_age(itr_arg.nodes[i]);

      /* decrement the ref count which is incremented above in ieee80211_sta_iter */
      ieee80211_free_node(itr_arg.nodes[i]);
  }
}


/*
 * Timeout inactive stations and do related housekeeping.
 * Note that we cannot hold the node lock while sending a
 * frame as this would lead to a LOR.  Instead we use a
 * generation number to mark nodes that we've scanned and
 * drop the lock and restart a scan if we have to time out
 * a node.  Since we are single-threaded by virtue of
 * controlling the inactivity timer we can be sure this will
 * process each node only once.
 */
void
ieee80211_timeout_stations(struct ieee80211_node_table *nt)
{
    struct ieee80211_node *ni;
    u_int gen;
    rwlock_state_t lock_state;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);

    ieee80211_timeout_node_saveq_age(nt);
    gen = nt->nt_scangen++;
restart:
    OS_BEACON_WRITE_LOCK(&nt->nt_nodelock, &lock_state, flags);
    TAILQ_FOREACH(ni, &nt->nt_node, ni_list) {
        if (ni->ni_scangen == gen)/* previously handled */
            continue;
        /*
         * Special case ourself; we may be idle for extended periods
         * of time and regardless reclaiming our state is wrong.
         */
        if (ni == ni->ni_vap->iv_bss) {
            /* NB: don't permit it to go negative */
            if (ni->ni_inact > 0)
                ni->ni_inact--;
            continue;
        }

        ni->ni_scangen = gen;

        /* If the TDLS node is in RUN state, continue. Otherwise
         *  cleanup and delete the node
         */
        if (IEEE80211_IS_TDLS_NODE(ni) && !IEEE80211_IS_TDLS_TIMEOUT(ni)) {
            continue;
        }

        if (IEEE80211_IS_TDLS_NODE(ni))
           IEEE80211_DPRINTF(ni->ni_vap, IEEE80211_MSG_TDLS,
                              "%s:TDLS node is being removed:%s, refcnt:%d\n",
                              __FUNCTION__, ether_sprintf(ni->ni_macaddr), ni->ni_refcnt);


        ni->ni_inact--;
#if UMAC_SUPPORT_NAWDS
        /* Never deauth the timeout NAWDS station.
         * But keep checking if it's still inactive. 
         */
        if (ni->ni_flags & IEEE80211_NODE_NAWDS && ni->ni_inact <= 0) {
            ni->ni_inact = 1;
        }
#endif
        if (ni->ni_associd != 0 && !IEEE80211_IS_TDLS_NODE(ni)) {
            /*
             * Probe the station before time it out.  We
             * send a null data frame which may not be
             * universally supported by drivers (need it
             * for ps-poll support so it should be...).
             */
            if ((0 < ni->ni_inact) && (ni->ni_inact <= ni->ni_vap->iv_inact_probe)) {

                IEEE80211_NOTE(ni->ni_vap, IEEE80211_MSG_INACT, ni,
                               "probe station due to inactivity, inact %u \n",
                               ni->ni_inact);
                /*
                 * Grab a reference before unlocking the table
                 * so the node cannot be reclaimed before we
                 * send the frame. 
                 */
                ieee80211_ref_node(ni);
                OS_BEACON_WRITE_UNLOCK(&nt->nt_nodelock, &lock_state, flags);
                ieee80211_send_nulldata(ni, 0);
                ieee80211_free_node(ni);
                /* 
                 * once the node table in unlocked.
                 * we need to rstart iterating the table
                 * as the table might have changed. 
                 */
                goto restart;
            }
        }

        /*
         * Make sure to timeout STAs who have sent 802.11
         * authentication but not have associated (a la M68).
         * Hostapd does not have timers to handle this, since
         * they don't know about these STAs.
         */
        if (ni->ni_inact <= 0) {
            IEEE80211_NOTE(ni->ni_vap, IEEE80211_MSG_INACT, ni,
                           "station timed out due to inactivity (refcnt %u) associd %d\n",
                           ieee80211_node_refcnt(ni), IEEE80211_AID(ni->ni_associd));
            /*
             * Send a deauthenticate frame and drop the station.
             * We grab a reference before unlocking the table so
             * the node cannot be reclaimed before we complete our
             * work.
             *
             * Separately we must drop the node lock before sending
             * in case the driver takes a lock, as this may result
             * in a LOR between the node lock and the driver lock.
             */
            ni->ni_vap->iv_stats.is_node_timeout++;
            ieee80211_ref_node(ni);
            OS_BEACON_WRITE_UNLOCK(&nt->nt_nodelock, &lock_state, flags);
            if (ni->ni_associd != 0  && !IEEE80211_IS_TDLS_NODE(ni)) {
                /*
                 * wlan_mlme_deauth_request will call IEEE80211_NODE_LEAVE 
                 * and also send notification to registered handlers .
                 */
                wlan_mlme_deauth_request(ni->ni_vap,ni->ni_macaddr,IEEE80211_REASON_AUTH_EXPIRE);
            } else if ( ni->ni_vap->iv_opmode == IEEE80211_M_IBSS || ni->ni_vap->iv_opmode == IEEE80211_M_STA) {
                ieee80211_sta_leave(ni);
            } else {
#if ATH_SUPPORT_AOW
                ieee80211_aow_join_indicate(ni->ni_ic, AOW_STA_DISCONNECTED, ni);
#endif  /* ATH_SUPPORT_AOW */
                IEEE80211_NODE_LEAVE(ni);

                if (ni->ni_vap) {
                    /* EV77198 : Call MLME indication handler if node is in associated state */
                    IEEE80211_DELIVER_EVENT_MLME_DEAUTH_INDICATION(ni->ni_vap, ni->ni_macaddr, IEEE80211_REASON_AUTH_EXPIRE);
                }
            }
            ieee80211_free_node(ni);
            goto restart;
        }
    }
    OS_BEACON_WRITE_UNLOCK(&nt->nt_nodelock, &lock_state, flags);
}


void
ieee80211_node_set_chan(struct ieee80211_node *ni)
{
    struct ieee80211_channel *chan = ni->ni_vap->iv_bsschan;

    KASSERT(chan != IEEE80211_CHAN_ANYC, ("bss channel not setup\n"));
    ni->ni_chan = chan;
    ieee80211_init_node_rates(ni, chan);
}

void
ieee80211_iterate_node(struct ieee80211com *ic, ieee80211_iter_func *func, void *arg)
{
    struct ieee80211_node_table *nt = &ic->ic_sta;
    struct ieee80211_node *ni = NULL, *next=NULL;
    rwlock_state_t lock_state;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);
    OS_BEACON_READ_LOCK(&nt->nt_nodelock, &lock_state, flags);
    TAILQ_FOREACH_SAFE(ni, &nt->nt_node, ni_list, next) {
        /* ieee80211_sta_leave may be called or RWLOCK_WRITE_LOCK may be acquired */
        /* TBD: this is not multi-thread safe. Should use wlan_iterate_station_list */
        ieee80211_ref_node(ni);
        (*func)(arg, ni);
        ieee80211_free_node(ni);
    }
    OS_BEACON_READ_UNLOCK(&nt->nt_nodelock, &lock_state, flags);
}

void
ieee80211_copy_bss(struct ieee80211_node *nbss, const struct ieee80211_node *obss)
{
    /* propagate useful state */
    nbss->ni_ath_flags = obss->ni_ath_flags;
    nbss->ni_txpower = obss->ni_txpower;
    nbss->ni_vlan = obss->ni_vlan;
    nbss->ni_beacon_rstamp = obss->ni_beacon_rstamp;
    nbss->ni_rssi = obss->ni_rssi;
}


/*
 * Leave the specified IBSS/BSS network.  The node is assumed to
 * be passed in with a held reference.
 */
#ifdef IEEE80211_DEBUG_REFCNT
bool
ieee80211_sta_leave_debug(struct ieee80211_node *ni, const char *func, int line)
#else
bool
ieee80211_sta_leave(struct ieee80211_node *ni)
#endif
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211_node_table *nt = &ic->ic_sta;
    rwlock_state_t lock_state;
    bool node_reclaimed=false;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);

    IEEE80211_NOTE(ni->ni_vap, IEEE80211_MSG_ASSOC, ni,
                   "%s: 0x%x \n", __func__,ni);
    OS_BEACON_WRITE_LOCK(&nt->nt_nodelock, &lock_state, flags);
    if (ni->ni_table != NULL) { /* if it is in the table */
#ifdef IEEE80211_DEBUG_REFCNT
        TRACENODE(ni, func, line);
        ieee80211_note(ni->ni_vap,"%s ,line %u: decrease node %p <%s> refcnt to %d\n",
                       __func__, __LINE__, ni, ether_sprintf(ni->ni_macaddr),
                       ieee80211_node_refcnt(ni)-1);
#endif
        KASSERT((ni->ni_table == nt),("unexpected node table "));
	/* remove wds entries using that node */
	ieee80211_remove_wds_addr(nt, ni->ni_macaddr,IEEE80211_NODE_F_WDS_BEHIND | IEEE80211_NODE_F_WDS_REMOTE);
	ieee80211_del_wds_node(nt, ni);
        /* reclaim the node to remove it from node table */
        node_reclaim(nt, ni);
        node_reclaimed=true;
    }
    OS_BEACON_WRITE_UNLOCK(&nt->nt_nodelock, &lock_state, flags);
    /* cleanup the node */
    if (node_reclaimed) {
        IEEE80211_DELETE_NODE_TARGET(ni, ic, ni->ni_vap, 0);
        ic->ic_node_cleanup(ni);
    }

    return node_reclaimed;
}

/*
 * Join the specified IBSS/BSS network.  The node is assumed to
 * be passed in with a reference already held for use in assigning
 * to iv_bss.
 */
int
ieee80211_sta_join_bss(struct ieee80211_node *selbs)
{
    struct ieee80211vap *vap = selbs->ni_vap;
    struct ieee80211_node_table *nt = &vap->iv_ic->ic_sta;
    struct ieee80211_node *obss;
    rwlock_state_t lock_state;
    struct ieee80211com *ic;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);

    /*
     * Committed to selbs. Leave old bss node if necessary
     */
    /* 
     * iv_bss is used in:
     * 1. tx path in STA/WDS mode.
     * 2. rx input_all
     * 3. vap iteration
     * Use node table lock to synchronize the acess.
     */
    OS_BEACON_WRITE_LOCK(&nt->nt_nodelock, &lock_state, flags);
    obss = vap->iv_bss;
    ic = vap->iv_ic;
    vap->iv_bss = selbs;
    selbs->ni_bss_node = selbs;
    OS_BEACON_WRITE_UNLOCK(&nt->nt_nodelock, &lock_state, flags);
    if (obss != NULL) {
        ieee80211_node_removeall_wds(&ic->ic_sta,obss);	
        ieee80211_free_node(obss);
    }

    /* XXX: more to do when integrating with STATION layer */
    return 0;
}

int
ieee80211_setup_node_rsn(
    struct ieee80211_node *ni,
    ieee80211_scan_entry_t scan_entry
    )
{
    struct ieee80211vap *vap = ni->ni_vap;
 
    /* parse WPA/RSN IE and setup RSN info */
    if (ni->ni_capinfo & IEEE80211_CAPINFO_PRIVACY) {
        struct ieee80211_rsnparms rsn;
        u_int8_t *rsn_ie, *wpa_ie;
        u_int8_t *wapi_ie = NULL;

        int status = IEEE80211_STATUS_SUCCESS;

        rsn = ni->ni_rsn;
        rsn_ie  = ieee80211_scan_entry_rsn(scan_entry);
        wpa_ie  = ieee80211_scan_entry_wpa(scan_entry);
#if ATH_SUPPORT_WAPI
        wapi_ie = ieee80211_scan_entry_wapi(scan_entry);
#endif

        if (rsn_ie != NULL)
            status = ieee80211_parse_rsn(vap, rsn_ie, &rsn);

        /* if a RSN IE was not there, or it's not valid, check the WPA IE */
        if ((rsn_ie == NULL) || (status != IEEE80211_STATUS_SUCCESS)) {
            if (wpa_ie != NULL)
                status = ieee80211_parse_wpa(vap, wpa_ie, &rsn);
        }

#if ATH_SUPPORT_WAPI
        if (wapi_ie != NULL)
            status = ieee80211_parse_wapi(vap, wapi_ie, &rsn);
#endif
        /*
         * if both RSN, WPA and WAPI IEs are absent, then we are certain that cipher is WEP.
         * However, we can't decide whether it's open or shared-key yet.
         */
        if ((rsn_ie == NULL) && (wpa_ie == NULL) && (wapi_ie == NULL))
        {
            RSN_RESET_UCAST_CIPHERS(&rsn);
            RSN_SET_UCAST_CIPHER(&rsn, IEEE80211_CIPHER_WEP);
            RSN_RESET_MCAST_CIPHERS(&rsn);
            RSN_SET_MCAST_CIPHER(&rsn, IEEE80211_CIPHER_WEP);
            RSN_RESET_MCASTMGMT_CIPHERS(&rsn);
            RSN_SET_MCASTMGMT_CIPHER(&rsn, IEEE80211_CIPHER_NONE);
        }
        
        if (status != IEEE80211_STATUS_SUCCESS) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_NODE,
                              "%s: invalid security settings for node %s\n",
                              __func__, ether_sprintf(ni->ni_macaddr));
            return -EINVAL;
        }

        ni->ni_rsn = rsn;   /* update rsn parameters */
    }

    return 0;
}


#define IEEE80211_MODE_SET(bm,m)  ((bm) |= (1 << (m))) 
#define IEEE80211_MODE_IS_SET(bm,m)  (((bm) & (1 << (m))) != 0 ) 
/*
 * check if the phymode forced by user is compatible (sub phy mode) with the phy mode of the AP.
 * if  it is compatible then return the phy mode else  return AUTO phy mode.
 *  bss_chan : bss chan of the AP .   
 *  des_mode: mode forced by user for the STA.
 *  bss_mode: operating mode of the AP 
*/
 
enum ieee80211_phymode ieee80211_get_phy_mode(struct ieee80211com *ic,  
                                        struct ieee80211_channel *bss_chan,
                                        enum ieee80211_phymode des_mode, enum ieee80211_phymode bss_mode) 
{
    u_int32_t mode_bitmap = 0;

    /*
     * for the given APs phymode construct a bitmap of all compatible sub phy modes.
     */
    switch(bss_mode) {
      case IEEE80211_MODE_11AC_VHT80:
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT80);
          if (ieee80211_ieee2mhz(bss_chan->ic_vhtop_ch_freq_seg1, IEEE80211_CHAN_5GHZ) > bss_chan->ic_freq) {
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40PLUS);
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
          } else {
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40MINUS);
              IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
          }
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
          break;
      case IEEE80211_MODE_11AC_VHT40PLUS:
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40PLUS);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
          break;
      case IEEE80211_MODE_11AC_VHT40MINUS:
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40MINUS);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT40);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
          break;
      case IEEE80211_MODE_11AC_VHT20:
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11AC_VHT20);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
          break;
      case IEEE80211_MODE_11NA_HT40PLUS:
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40PLUS);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
          break;
      case IEEE80211_MODE_11NA_HT40MINUS:
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT40MINUS);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
          break;
      case IEEE80211_MODE_11NA_HT20:
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NA_HT20);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
          break;
      case IEEE80211_MODE_11NG_HT40PLUS:
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40PLUS);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT20);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11G);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
          break;
      case IEEE80211_MODE_11NG_HT40MINUS:
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40MINUS);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT40);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT20);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11G);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
          break;
      case IEEE80211_MODE_11NG_HT20:
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11NG_HT20);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11G);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
          break;
      case IEEE80211_MODE_11A:
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11A);
          break;
      case IEEE80211_MODE_11G:
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11G);
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
          break;
      case IEEE80211_MODE_11B:
          IEEE80211_MODE_SET(mode_bitmap, IEEE80211_MODE_11B);
          break;
    default:
        break;
          
    }
    /* 
     * check if the user selected mode for STA is part of the bitmap of compatible phy modes.
     */
    if (IEEE80211_MODE_IS_SET(mode_bitmap, des_mode)) {
      /*
       * if user requested HT40 then return HT40PLUS  if HT40PLUS is comaptible with AP
       * else return HT40MINUS.
       */ 
        switch (des_mode) {
            case IEEE80211_MODE_11NA_HT40:
                if (IEEE80211_MODE_IS_SET(mode_bitmap,IEEE80211_MODE_11NA_HT40PLUS)) {
                    des_mode = IEEE80211_MODE_11NA_HT40PLUS;
                } else {
                    des_mode = IEEE80211_MODE_11NA_HT40MINUS;
                }
                break;
            case IEEE80211_MODE_11AC_VHT40:
                if (IEEE80211_MODE_IS_SET(mode_bitmap,IEEE80211_MODE_11AC_VHT40PLUS)) {
                    des_mode = IEEE80211_MODE_11AC_VHT40PLUS;
                } else {
                    des_mode = IEEE80211_MODE_11AC_VHT40MINUS;
                }
                break;
            case IEEE80211_MODE_11NG_HT40:
                if (IEEE80211_MODE_IS_SET(mode_bitmap,IEEE80211_MODE_11NG_HT40PLUS)) {
                    des_mode = IEEE80211_MODE_11NG_HT40PLUS;
                } else {
                    des_mode = IEEE80211_MODE_11NG_HT40MINUS;
                }
               break;
            default:
               break;
        }
    } else {
        des_mode = IEEE80211_MODE_AUTO;
    }

    return des_mode;
}

/*
 * Setup a node based on the scan entry
 */
int
ieee80211_setup_node(
    struct ieee80211_node *ni,
    ieee80211_scan_entry_t scan_entry
    )
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic = ni->ni_ic;
    u_int8_t *rates, *xrates;
    struct ieee80211_country_ie* countryie;
    u_int8_t *htcap = NULL;
    u_int8_t *htinfo = NULL;
    u_int8_t *vhtcap = NULL;
    u_int8_t *vhtop = NULL;
    u_int8_t *wme = NULL;
    u_int8_t *athextcap = NULL;
    u_int8_t *extcapie = NULL;
    u_int8_t *ssid;
    int i;
    int ht_rates_allowed;
    int error = 0;
    enum ieee80211_phymode phymode = IEEE80211_MODE_AUTO;
     

    ASSERT((vap->iv_opmode == IEEE80211_M_STA) ||
           (vap->iv_opmode == IEEE80211_M_IBSS));
    
    ni->ni_beacon_rstamp = OS_GET_TIMESTAMP();
    /*
     * If NIC does not support the channels in this node, NULL is returned.
     */
    ni->ni_chan = ieee80211_scan_entry_channel(scan_entry);

    /* Assert this in debug driver, but fail gracefully in release driver. */
    ASSERT((ni->ni_chan != NULL) && (ni->ni_chan != IEEE80211_CHAN_ANYC));
    if ((ni->ni_chan == NULL) || (ni->ni_chan == IEEE80211_CHAN_ANYC))
        return -EIO;

    if ((ni->ni_chan == NULL) || (ni->ni_chan == IEEE80211_CHAN_ANYC))
        return -EIO;

    if (vap->iv_des_mode != IEEE80211_MODE_AUTO) {
        /* if desired mode is not auto then find if the requested mode
           is supported by the AP */ 
        phymode = ieee80211_get_phy_mode(ic,ni->ni_chan,vap->iv_des_mode,ieee80211_chan2mode(ni->ni_chan)); 
        if (phymode != IEEE80211_MODE_AUTO) {
             ieee80211_note(vap, "%s forcing sta to associate in %d mode\n", __func__, phymode);
            ni->ni_chan = ieee80211_find_dot11_channel(ic, ni->ni_chan->ic_ieee, phymode | ic->ic_chanbwflag);
            if ((ni->ni_chan == NULL) || (ni->ni_chan == IEEE80211_CHAN_ANYC)) {
                ieee80211_note(vap,"%s ,can not find a channel with the desired mode \n");
             return -EIO;
            }
        }
               
    }

    IEEE80211_ADDR_COPY(ni->ni_bssid, ieee80211_scan_entry_bssid(scan_entry));
    ssid = ieee80211_scan_entry_ssid(scan_entry, &ni->ni_esslen);
    if (ssid != NULL)
        OS_MEMCPY(ni->ni_essid, ssid, ni->ni_esslen);

    ni->ni_capinfo = ieee80211_scan_entry_capinfo(scan_entry);
    ni->ni_erp = ieee80211_scan_entry_erpinfo(scan_entry);

    countryie = (struct ieee80211_country_ie*)ieee80211_scan_entry_country(scan_entry);
    if(countryie) {
        ni->ni_cc[0] = countryie->cc[0];
        ni->ni_cc[1] = countryie->cc[1];
        ni->ni_cc[2] = countryie->cc[2];
    } else {
        ni->ni_cc[0] = 0;
        ni->ni_cc[1] = 0;
        ni->ni_cc[2] = 0;
    }

    ni->ni_intval = ieee80211_scan_entry_beacon_interval(scan_entry);
    ni->ni_lintval = ic->ic_lintval;
    LIMIT_BEACON_PERIOD(ni->ni_intval);

    /*
     * Verify that ATIM window is smaller than beacon interval.
     * This kind of misconfiguration can put hardware into unpredictable state
     */
    ASSERT(ni->ni_intval > vap->iv_atim_window);

    /* Clear node flags */
    //ni->ni_ext_caps = ni->ni_flags = ni->ni_ath_flags = ni->ni_htcap = 0;
    ni->ni_ext_caps = 0;
    ni->ni_flags = 0;
    ni->ni_htcap = 0;
    ni->ni_ath_flags = 0;
    ni->ni_vhtcap = 0;

    /* update WMM capability */
    if (((wme = ieee80211_scan_entry_wmeinfo_ie(scan_entry))  != NULL) ||
        ((wme = ieee80211_scan_entry_wmeparam_ie(scan_entry)) != NULL)) {
        u_int8_t    qosinfo;

        ni->ni_ext_caps |= IEEE80211_NODE_C_QOS;
        if (ieee80211_parse_wmeinfo(vap, wme, &qosinfo) >= 0) {
            if (qosinfo & WME_CAPINFO_UAPSD_EN) {
                ni->ni_ext_caps |= IEEE80211_NODE_C_UAPSD;
            }
        }
    }

    if ((athextcap = (u_int8_t *) ieee80211_scan_entry_athextcaps(scan_entry)) != NULL) {
        ieee80211_process_athextcap_ie(ni, athextcap);
    }

#if ATH_SUPPORT_AOW    
    /* parse AOW IE and setup AOW info */
    ieee80211_setup_node_aow(ni, scan_entry);
#endif  /* ATH_SUPPORT_AOW */    

    /* parse WPA/RSN IE and setup RSN info */
    error = ieee80211_setup_node_rsn(ni, scan_entry);

    /*
     * With WEP and TKIP encryption algorithms:
     * Diable aggregation if IEEE80211_NODE_WEPTKIPAGGR is not set.
     * Disable 11n if IEEE80211_FEXT_WEP_TKIP_HTRATE is not set.
     */
    ht_rates_allowed = 1;
    if((IEEE80211_VAP_IS_PRIVACY_ENABLED(vap) && 
       (RSN_CIPHER_IS_WEP(&vap->iv_rsn) || 
        (RSN_CIPHER_IS_TKIP(&vap->iv_rsn) && !RSN_CIPHER_IS_CCMP(&vap->iv_rsn)))) 
    || ((ni->ni_capinfo & IEEE80211_CAPINFO_PRIVACY) &&
        (RSN_CIPHER_IS_WEP(&ni->ni_rsn) ||
        (RSN_CIPHER_IS_TKIP(&ni->ni_rsn) && !RSN_CIPHER_IS_CCMP(&ni->ni_rsn))))) {
        ieee80211node_set_flag(ni, IEEE80211_NODE_WEPTKIP);
        if (ieee80211_ic_wep_tkip_htrate_is_set(ic)) {
            if (!ieee80211_has_weptkipaggr(ni))
                ieee80211node_set_flag(ni, IEEE80211_NODE_NOAMPDU);
        } else {
            ht_rates_allowed = 0;
        }
    }

    if ((vap->iv_opmode == IEEE80211_M_IBSS) &&
        !ieee80211_ic_ht20Adhoc_is_set(ic) &&
        !ieee80211_ic_ht40Adhoc_is_set(ic)) {
        ht_rates_allowed = 0;
    }

    if (ht_rates_allowed) {
        htcap  = ieee80211_scan_entry_htcap(scan_entry);
        htinfo = ieee80211_scan_entry_htinfo(scan_entry);
        if (htcap && (IEEE80211_IS_CHAN_11N(ni->ni_chan) || IEEE80211_IS_CHAN_VHT(ni->ni_chan) )) {
            ieee80211_parse_htcap(ni, htcap);
        }
        if (htinfo && (IEEE80211_IS_CHAN_11N(ni->ni_chan) || IEEE80211_IS_CHAN_VHT(ni->ni_chan) )) {
            ieee80211_parse_htinfo(ni, htinfo);
        }

        if ((vap->iv_opmode == IEEE80211_M_IBSS) && !ieee80211_ic_ht40Adhoc_is_set(ic)) {
            ni->ni_chwidth = IEEE80211_CWM_WIDTH20;
        }

        if ((vap->iv_opmode == IEEE80211_M_IBSS) && !ieee80211_ic_htAdhocAggr_is_set(ic)) {
            ieee80211node_set_flag(ni, IEEE80211_NODE_NOAMPDU);
        }

        vhtcap  = ieee80211_scan_entry_vhtcap(scan_entry);
        vhtop  = ieee80211_scan_entry_vhtop(scan_entry);

        if (vhtcap && IEEE80211_IS_CHAN_VHT(ni->ni_chan) ) {
            ieee80211_parse_vhtcap(ni, vhtcap);
        }

        if (vhtop && IEEE80211_IS_CHAN_VHT(ni->ni_chan) ) {
            ieee80211_parse_vhtop(ni, vhtop);
        }
    } 

    /* NB: must be after ni_chan is setup */
    rates = ieee80211_scan_entry_rates(scan_entry);
    xrates = ieee80211_scan_entry_xrates(scan_entry);
    if (rates) {
        ieee80211_setup_rates(ni, rates, xrates, IEEE80211_F_DOXSECT);
    }
    if (htcap && (IEEE80211_IS_CHAN_11N(ni->ni_chan) || IEEE80211_IS_CHAN_VHT(ni->ni_chan) )) {
        ieee80211_setup_ht_rates(ni, htcap, IEEE80211_F_DOXSECT);
    }
    if (htinfo && (IEEE80211_IS_CHAN_11N(ni->ni_chan) || IEEE80211_IS_CHAN_VHT(ni->ni_chan) )) {
        ieee80211_setup_basic_ht_rates(ni, htinfo);
    }
    if (vhtcap && IEEE80211_IS_CHAN_VHT(ni->ni_chan) ) {
        ieee80211_setup_vht_rates(ni, vhtcap, IEEE80211_F_DOXSECT);
    }

    /* 
     * ieee80211_parse_vhtop would hav set the channel width based on APs operating mode/channel.
     * if vap is forced to operate in a different lower mode than what AP is opearing,
     *  then set the channel width based on  the forced channel/phy mode .
     */
    if (phymode != IEEE80211_MODE_AUTO) {
        switch(phymode) {
        case IEEE80211_MODE_11A          : 
        case IEEE80211_MODE_11B          :
        case IEEE80211_MODE_11G          : 
        case IEEE80211_MODE_11NA_HT20    : 
        case IEEE80211_MODE_11NG_HT20    :  
        case IEEE80211_MODE_11AC_VHT20   :  
            ni->ni_chwidth = IEEE80211_CWM_WIDTH20;
            break;
        case IEEE80211_MODE_11NA_HT40PLUS:  
        case IEEE80211_MODE_11NA_HT40MINUS:
        case IEEE80211_MODE_11NG_HT40PLUS :
        case IEEE80211_MODE_11NG_HT40MINUS:
        case IEEE80211_MODE_11AC_VHT40PLUS:
        case IEEE80211_MODE_11AC_VHT40MINUS:
            ni->ni_chwidth = IEEE80211_CWM_WIDTH40;
            break;
        case IEEE80211_MODE_11AC_VHT80:
            ni->ni_chwidth = IEEE80211_CWM_WIDTH80;
            break;
        default :
            break;
            
        }
    }

    /* Find min basic supported rate */
    ni->ni_minbasicrate = 0;
    for (i=0; i < ni->ni_rates.rs_nrates; i++) {
        if ((ni->ni_minbasicrate == 0) ||
            ((ni->ni_minbasicrate & IEEE80211_RATE_VAL) > (ni->ni_rates.rs_rates[i] & IEEE80211_RATE_VAL))) {
            ni->ni_minbasicrate = ni->ni_rates.rs_rates[i];
        }
    }

    if ((extcapie = (u_int8_t *)ieee80211_scan_entry_extcaps(scan_entry)) != NULL) {
            ieee80211_process_extcap_ie(ni, extcapie);
        }

    /* Error at parsing WPA/RSN IE */
    if (error != 0)
        return error;
    
    return 0;
}

#ifdef IEEE80211_DEBUG_REFCNT
struct ieee80211_node *
ieee80211_ref_bss_node_debug(struct ieee80211vap *vap,
                         const char *func, int line)
#else  /* !IEEE80211_DEBUG_REFCNT */
struct ieee80211_node *
ieee80211_ref_bss_node(struct ieee80211vap *vap)
#endif  /* IEEE80211_DEBUG_REFCNT */
{
    struct ieee80211_node_table *nt = &vap->iv_ic->ic_sta;
    struct ieee80211_node *ni = NULL;
    rwlock_state_t lock_state;

    if (vap->iv_bss) {
        OS_BEACON_DECLARE_AND_RESET_VAR(flags);

        OS_BEACON_READ_LOCK(&nt->nt_nodelock, &lock_state, flags);
        ni = ieee80211_ref_node(vap->iv_bss);
        OS_BEACON_READ_UNLOCK(&nt->nt_nodelock, &lock_state, flags);

#ifdef IEEE80211_DEBUG_REFCNT
        ieee80211_note(ni->ni_vap,"%s ,line %u: increase node %p <%s> refcnt to %d\n",
               func, line, ni, ether_sprintf(ni->ni_macaddr),
               ieee80211_node_refcnt(ni));
#endif  //IEEE80211_DEBUG_REFCNT
    }
    return ni;
}

/*
 * Reset bss state on transition to the INIT state.
 * Clear any stations from the table (they have been
 * deauth'd) and reset the bss node (clears key, rate,
 * etc. state).
 */
int
ieee80211_reset_bss(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni, *obss;
    struct ieee80211_node_table *nt = &ic->ic_sta;
    rwlock_state_t lock_state;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);

    IEEE80211_DPRINTF(vap, IEEE80211_MSG_MLME, "%s\n", __func__);

    ieee80211_node_table_reset(&ic->ic_sta, vap);

    ni = ieee80211_alloc_node(nt, vap, vap->iv_myaddr);
    if (ni == NULL) {
        printk("Failed to create bss node\n");
        return -ENOMEM;
    }

    /* 
     * iv_bss is used in:
     * 1. tx path in STA/WDS mode.
     * 2. rx input_all
     * 3. vap iteration
     * Use node table lock to synchronize the acess.
     */
    OS_BEACON_WRITE_LOCK(&nt->nt_nodelock, &lock_state, flags);
    obss = vap->iv_bss;
    vap->iv_bss = ni; /* alloc node gives the needed extra reference */

    /*
     * XXX: remove the default node from node table, because
     * it's not associated to any one. This will fix reference count
     * leak when freeing the default node.
     */
    node_reclaim(nt, ni);
    OS_BEACON_WRITE_UNLOCK(&nt->nt_nodelock, &lock_state, flags);

    if (obss != NULL) {
        /* Do we really need obss info?? */
        ieee80211_copy_bss(ni, obss);
        ni->ni_intval = obss->ni_intval;
        IEEE80211_DELETE_NODE_TARGET(obss, ic, vap, 1);
        ieee80211_free_node(obss);
    }
    return 0;
}

/*
 * Node table support.
 */
static void
ieee80211_node_table_init(struct ieee80211com *ic,
                          struct ieee80211_node_table *nt,
                          const char *name
			  )
{
    int hash;

    nt->nt_ic = ic;
    OS_RWLOCK_INIT(&nt->nt_nodelock);
    OS_RWLOCK_INIT(&nt->nt_wds_nodelock);
    TAILQ_INIT(&nt->nt_node);
#if UMAC_SUPPORT_SMARTANTENNA
    TAILQ_INIT(&nt->nt_smartant_node);
#endif    
    for (hash = 0; hash < IEEE80211_NODE_HASHSIZE; hash++)
        LIST_INIT(&nt->nt_hash[hash]);
#if UMAC_SUPPORT_PROXY_ARP
    TAILQ_INIT(&nt->nt_ipv6_node);
    for (hash = 0; hash < IEEE80211_IPV4_HASHSIZE; hash++)
        LIST_INIT(&nt->nt_ipv4_hash[hash]);
    for (hash = 0; hash < IEEE80211_IPV6_HASHSIZE; hash++)
        LIST_INIT(&nt->nt_ipv6_hash[hash]);
#endif
    nt->nt_scangen = 1;
    nt->nt_name = name;
    ieee80211_wds_attach(nt);
}

static void
ieee80211_node_table_reset(struct ieee80211_node_table *nt, struct ieee80211vap *match)
{
    struct ieee80211_node *ni, *next;
    rwlock_state_t lock_state;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);

    OS_BEACON_WRITE_LOCK(&nt->nt_nodelock, &lock_state, flags);
	TAILQ_FOREACH_SAFE(ni, &nt->nt_node, ni_list, next) {
        if ((match != NULL) && (ni->ni_vap != match))
            continue;

        if (ni->ni_associd != 0) {
            struct ieee80211vap *vap = ni->ni_vap;            

            if (vap->iv_aid_bitmap != NULL)
                IEEE80211_AID_CLR(vap, ni->ni_associd);
        }

        node_reclaim(nt, ni);
    }
    OS_BEACON_WRITE_UNLOCK(&nt->nt_nodelock, &lock_state, flags);
}

void
ieee80211_node_attach(struct ieee80211com *ic)
{
    ieee80211_node_table_init(ic, &ic->ic_sta, "station");
#ifdef IEEE80211_DEBUG_NODELEAK
    TAILQ_INIT(&ic->ic_nodes);
    OS_RWLOCK_INIT(&(ic)->ic_nodelock);
#endif
    ic->ic_node_alloc = node_alloc;
    ic->ic_node_free = node_free;
    ic->ic_node_cleanup = node_cleanup;
    ic->ic_node_getrssi = node_getrssi;
    ic->ic_node_authorize = NULL;
}

void
ieee80211_node_detach(struct ieee80211com *ic)
{
    struct ieee80211_node_table *nt = &ic->ic_sta;
    ieee80211_node_table_reset(nt, NULL);
    OS_RWLOCK_DESTROY(&nt->nt_nodelock);
    ieee80211_wds_detach(nt);
}

void
ieee80211_node_vattach(struct ieee80211vap *vap)
{
    vap->iv_inact_init = IEEE80211_INACT_INIT;
    vap->iv_inact_auth = IEEE80211_INACT_AUTH;
    vap->iv_inact_run =  IEEE80211_INACT_RUN;
    vap->iv_inact_probe =IEEE80211_INACT_PROBE;
}

void
ieee80211_node_latevdetach(struct ieee80211vap *vap)
{
    KASSERT((ieee80211_vap_deleted_is_set(vap)), ("vap is not deleted by user")); 
    KASSERT((vap->iv_node_count == 0), ("vap still has nodes"));
    
    /*
     * free the aid bitmap.
     */
    if (vap->iv_aid_bitmap) {
        OS_FREE(vap->iv_aid_bitmap);
        vap->iv_max_aid = 0;
    }
}

void
ieee80211_node_vdetach(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;

    ieee80211_node_table_reset(&ic->ic_sta, vap);
    if (vap->iv_bss != NULL) {
        IEEE80211_DELETE_NODE_TARGET(vap->iv_bss, ic, vap, 0);
        ieee80211_free_node(vap->iv_bss);
    }
}

int
ieee80211_node_latevattach(struct ieee80211vap *vap)
{
    int error = 0;

    /*
     * Allocate these only if needed.  Beware that we
     * know adhoc mode doesn't support ATIM yet...
     */
    if (vap->iv_opmode == IEEE80211_M_HOSTAP || \
        vap->iv_opmode == IEEE80211_M_BTAMP  || \
        vap->iv_opmode == IEEE80211_M_IBSS) {
        unsigned long bm_size;

        KASSERT(vap->iv_max_aid != 0, ("0 max aid"));

        bm_size = howmany(vap->iv_max_aid, 32) * sizeof(u_int32_t);
        vap->iv_aid_bitmap = (u_int32_t *)OS_MALLOC(vap->iv_ic->ic_osdev,
                                                    bm_size,
                                                    GFP_KERNEL);
        if (vap->iv_aid_bitmap == NULL) {
            /* XXX no way to recover */
            printk("%s: no memory for AID bitmap!\n", __func__);
            vap->iv_max_aid = 0;
            return -ENOMEM;
        }
        OS_MEMZERO(vap->iv_aid_bitmap, bm_size);
    }

    error = ieee80211_reset_bss(vap);
    return error;
}

/*
 * Add the specified station to the station table.
 * calls alloc_node and hence return the node with 2 references.
 * one for adding it to the table and the
 * the other for the caller to use.
 */
struct ieee80211_node *
ieee80211_dup_bss(struct ieee80211vap *vap, const u_int8_t *macaddr)
{
	struct ieee80211com *ic = vap->iv_ic;
	struct ieee80211_node *ni;

    ni = ieee80211_alloc_node(&ic->ic_sta, vap, macaddr);
    if (ni != NULL) {
        /*
         * Inherit from iv_bss.
         */
        ni->ni_authmode = vap->iv_bss->ni_authmode;
        ni->ni_txpower = vap->iv_bss->ni_txpower;
        IEEE80211_ADDR_COPY(ni->ni_bssid, vap->iv_bss->ni_bssid);
        ni->ni_rsn = vap->iv_bss->ni_rsn;
        ni->ni_bss_node = ieee80211_ref_bss_node(vap);
#ifndef MAGPIE_HIF_GMAC        
        IEEE80211_ADD_NODE_TARGET(ni, vap, 0);
#endif    
    }
    return ni;
}

#ifdef IEEE80211_DEBUG_NODELEAK
void
ieee80211_dump_alloc_nodes(struct ieee80211com *ic)
{
    struct ieee80211_node *ni;
    u_int8_t  ssid[IEEE80211_NWID_LEN+4];
    rwlock_state_t lock_state;
    ieee80211_node_saveq_info qinfo;

    ieee80211com_note(ic, "dumping all allocated nodes ... \n");
    OS_RWLOCK_READ_LOCK(&ic->ic_nodelock,&lock_state);
    TAILQ_FOREACH(ni, &ic->ic_nodes, ni_alloc_list) {
        ieee80211com_note(ic, "node 0x%x mac %s  tmpnode: %d nodetable : %d flags 0x%x refcount: %d ",ni,
               ether_sprintf(ni->ni_macaddr), 
               (ni->ni_flags & IEEE80211_NODE_TEMP) ? 1 : 0,
               (ni->ni_table) ? 1 : 0,ni->ni_flags, ieee80211_node_refcnt(ni));
        if (ni->ni_esslen) {
            OS_MEMCPY(ssid, ni->ni_essid, ni->ni_esslen);
            ssid[ni->ni_esslen] = 0;
        }
        ieee80211_node_saveq_get_info(ni, &qinfo);
        ieee80211com_note(ic, "bssid %s cap 0x%x dqlen  %d mgtqlen %d  %s %s \n",
               ether_sprintf(ni->ni_bssid), ni->ni_capinfo, 
                          qinfo.data_count, qinfo.mgt_count,
               ni->ni_esslen ? "ssid ":"", 
               ni->ni_esslen ? (char *)ssid : "" );
        ic->ic_print_nodeq_info(ni);
    }
    OS_RWLOCK_READ_UNLOCK(&ic->ic_nodelock,&lock_state);
}

void
wlan_dump_alloc_nodes(wlan_dev_t devhandle)
{
    struct ieee80211com *ic = (struct ieee80211com *) devhandle;
    ieee80211_dump_alloc_nodes(ic);
}
#endif

/* External UMAC APIs */

u_int16_t wlan_node_getcapinfo(wlan_node_t node)
{
    return node->ni_capinfo;
}

int  wlan_node_getwpaie(wlan_if_t vap, u_int8_t *macaddr, u_int8_t *ie, u_int16_t *len)
{
    struct ieee80211_node *ni;
    ni = ieee80211_find_node(&vap->iv_ic->ic_sta, macaddr);
    if (ni == NULL)
        return EINVAL;
    if (ni->ni_wpa_ie != NULL) {
        int ielen = ni->ni_wpa_ie[1] + 2;
        if (ielen > *len) {
            ieee80211_free_node(ni);
            return EINVAL;
        }
        OS_MEMCPY(ie, ni->ni_wpa_ie, ielen);
        *len = ielen;
    } else {
        *len = 0;
    }
    ieee80211_free_node(ni);
    
    return 0;

}

int  wlan_node_getwpsie(wlan_if_t vap, u_int8_t *macaddr, u_int8_t *ie, u_int16_t *len)
{
    struct ieee80211_node *ni;
    ni = ieee80211_find_node(&vap->iv_ic->ic_sta, macaddr);
    if (ni == NULL)
        return EINVAL;
    if (ni->ni_wps_ie != NULL) {
        int ielen = ni->ni_wps_ie[1] + 2;
        if (ielen > *len) {
            ieee80211_free_node(ni);
            return EINVAL;
        }
        OS_MEMCPY(ie, ni->ni_wps_ie, ielen);
        *len = ielen;
    } else {
        *len = 0;
    }
    ieee80211_free_node(ni);
    
    return 0;

}

int  wlan_node_getathie(wlan_if_t vap, u_int8_t *macaddr, u_int8_t *ie, u_int16_t *len)
{
    struct ieee80211_node *ni;
    ni = ieee80211_find_node(&vap->iv_ic->ic_sta, macaddr);
    if (ni == NULL)
        return EINVAL;
    if (ni->ni_ath_ie != NULL) {
        int ielen = ni->ni_ath_ie[1] + 2;
        if (ielen > *len) {
            ieee80211_free_node(ni);
            return EINVAL;
        }
        OS_MEMCPY(ie, ni->ni_ath_ie, ielen);
        *len = ielen;
    } else {
        *len = 0;
    }
    ieee80211_free_node(ni);
    
    return 0;

}

int  wlan_node_getwmeie(wlan_if_t vap, u_int8_t *macaddr, u_int8_t *ie, u_int16_t *len)
{
    struct ieee80211_node *ni;
    ni = ieee80211_find_node(&vap->iv_ic->ic_sta, macaddr);
    if (ni == NULL)
        return EINVAL;
    if (ni->ni_wme_ie != NULL) {
        int ielen = ni->ni_wme_ie[1] + 2;
        if (ielen > *len) {
            ieee80211_free_node(ni);
            return EINVAL;
        }
        OS_MEMCPY(ie, ni->ni_wme_ie, ielen);
        *len = ielen;
    } else {
        *len = 0;
    }
    ieee80211_free_node(ni);
    
    return 0;

}


static void ieee80211_node_iter(void *arg, struct ieee80211_node *ni)
{
    struct ieee80211_iter_arg *itr_arg = (struct ieee80211_iter_arg *)arg;
    struct ieee80211vap *vap = ni->ni_vap;

    /*
     * ignore if the node does not belong to the requesting vap.
     */
    if (vap != itr_arg->vap)  {
         return;
    }

    /*
     * ignore BSS node for AP/IBSS mode 
     */
    if ((ni == ni->ni_bss_node) &&
        ((vap->iv_opmode == IEEE80211_M_HOSTAP) || (vap->iv_opmode == IEEE80211_M_IBSS) ||
         (vap->iv_opmode == IEEE80211_M_BTAMP))) {
        return;
    }

    if (!(itr_arg->flag & IEEE80211_NODE_ITER_F_UNASSOC_STA)) {
        /*
         * ignore un associated stations for AP mode 
         */
        if (vap->iv_opmode == IEEE80211_M_HOSTAP && (ni->ni_associd == 0)) {
            return;
        }

        /*
         * ignore un associated stations for IBSS mode 
         */
        if (vap->iv_opmode == IEEE80211_M_IBSS && 
            (ni->ni_assoc_state != IEEE80211_NODE_ADHOC_STATE_AUTH_ASSOC)) {
            return;
        }
    }

    if (!(itr_arg->flag & IEEE80211_NODE_ITER_F_ASSOC_STA)) {
        /*
         * ignore associated stations for AP mode 
         */
        if (vap->iv_opmode == IEEE80211_M_HOSTAP && (ni->ni_associd != 0)) {
            return;
        }

        /*
         * ignore associated stations for IBSS mode 
         */
        if (vap->iv_opmode == IEEE80211_M_IBSS && 
            (ni->ni_assoc_state == IEEE80211_NODE_ADHOC_STATE_AUTH_ASSOC)) {
            return;
        }
    }

    if (itr_arg->count < IEEE80211_AID_DEF) {
        /* increment the ref count so that the node is not freed */
        itr_arg->nodes[itr_arg->count] = ieee80211_ref_node(ni);
    }

    ++itr_arg->count;
}


static int32_t
ieee80211_iterate_node_list(wlan_if_t vap,ieee80211_sta_iter_func iter_func,void *arg, u_int32_t flag)
{
  struct ieee80211com *ic = vap->iv_ic;
  struct ieee80211_iter_arg itr_arg;
  int i;
  itr_arg.count=0;
  itr_arg.vap=vap;
  itr_arg.flag=flag;

  /*
   * we can not call the call back function iter_func from the ieee80211_sta_iter.
   * because the ieee80211_iter is called with nt lock held and will result in 
   * dead lock if the implementation of iter_func calls bcak into umac to query more
   * info about the node (which is more likely).
   * instaed the ieee80211_sta_iter collects all the nodes in to the nodes array
   * part of the itr_arg and also increments the ref count on these nodes so that
   * they wont get freed.
   */

  ieee80211_iterate_node(ic,ieee80211_node_iter,(void *)&itr_arg);
  for (i=0;i<itr_arg.count; ++i) 
  {
      if (i == IEEE80211_AID_DEF) break;
      if (iter_func) {
          /* 
           * node has been refed in ieee80211_sta_iter
           * so safe to acces the contentes of the node.
           */
          (* iter_func) (arg, itr_arg.nodes[i]);
      }
      /* decrement the ref count which is incremented above in ieee80211_sta_iter */
      ieee80211_free_node(itr_arg.nodes[i]);
  }

  return (itr_arg.count);
}

int32_t wlan_iterate_all_sta_list(wlan_if_t vap,ieee80211_sta_iter_func iter_func,void *arg)
{
    return ieee80211_iterate_node_list(vap, iter_func, arg,
                                       IEEE80211_NODE_ITER_F_ASSOC_STA |
                                       IEEE80211_NODE_ITER_F_UNASSOC_STA);
}

int32_t wlan_iterate_station_list(wlan_if_t vap,ieee80211_sta_iter_func iter_func,void *arg)
{
    return ieee80211_iterate_node_list(vap, iter_func, arg,
                                       IEEE80211_NODE_ITER_F_ASSOC_STA);
}

int wlan_node_txrate_info(wlan_node_t node, ieee80211_rate_info *rinfo)
{
    u_int8_t rc;
    rinfo->rate = node->ni_ic->ic_node_getrate(node, IEEE80211_RATE_TX);
    rinfo->lastrate = node->ni_ic->ic_node_getrate(node, IEEE80211_LASTRATE_TX);
    rc = (u_int8_t) node->ni_ic->ic_node_getrate(node, IEEE80211_RATECODE_TX);
    rinfo->mcs = rc;
    rinfo->type = (rinfo->mcs & 0x80)? IEEE80211_RATE_TYPE_MCS : IEEE80211_RATE_TYPE_LEGACY;
    rinfo->maxrate_per_client = node->ni_ic->ic_node_getrate(node,
                                                     IEEE80211_MAX_RATE_PER_CLIENT);
    return 0;
}

int wlan_node_rxrate_info(wlan_node_t node, ieee80211_rate_info *rinfo)
{
    u_int8_t rc;
    rinfo->rate = node->ni_ic->ic_node_getrate(node, IEEE80211_RATE_RX);
    rinfo->lastrate = node->ni_ic->ic_node_getrate(node, IEEE80211_LASTRATE_RX);
    rc = (u_int8_t) node->ni_ic->ic_node_getrate(node, IEEE80211_RATECODE_RX);
    rinfo->mcs = rc;
    rinfo->type = (rinfo->mcs & 0x80)? IEEE80211_RATE_TYPE_MCS : IEEE80211_RATE_TYPE_LEGACY;
    return 0;
}

int wlan_node_getrssi(wlan_node_t node,wlan_rssi_info *rssi_info,  wlan_rssi_type rssi_type )
{

    int chain_ix;
    u_int8_t flags=0;
    struct ieee80211_node *ni = node;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic = vap->iv_ic;

    if (rssi_type == WLAN_RSSI_TX)
        flags = IEEE80211_RSSI_TX;
    else if (rssi_type == WLAN_RSSI_RX)
        flags = IEEE80211_RSSI_RX;
    else if (rssi_type == WLAN_RSSI_BEACON)
        flags = IEEE80211_RSSI_BEACON;
    else if (rssi_type == WLAN_RSSI_RX_DATA)
        flags = IEEE80211_RSSI_RXDATA;

    if (rssi_type == WLAN_RSSI_TX) {
        rssi_info->valid_mask = ic->ic_tx_chainmask;  
    } else {
        rssi_info->valid_mask = ic->ic_rx_chainmask;  
    }

    rssi_info->avg_rssi = ic->ic_node_getrssi(ni,-1,flags);
    for(chain_ix=0;chain_ix<MAX_CHAINS; ++chain_ix) {
        rssi_info->rssi_ctrl[chain_ix] = ic->ic_node_getrssi(ni, chain_ix ,flags);
    }
    flags |= IEEE80211_RSSI_EXTCHAN;
    for(chain_ix=0;chain_ix<MAX_CHAINS; ++chain_ix) {
        rssi_info->rssi_ext[chain_ix] = ic->ic_node_getrssi(ni, chain_ix ,flags);
    }
    return 0;

}

u_int8_t *wlan_node_getmacaddr(wlan_node_t node)
{
    return node->ni_macaddr;
}

u_int8_t *wlan_node_getbssid(wlan_node_t node)
{
    return node->ni_bssid;
}

u_int32_t wlan_node_set_assoc_decision(wlan_if_t vap, u_int8_t *macaddr, u_int16_t assoc_status, u_int16_t p2p_assoc_status)
{
    
    struct ieee80211_node *ni;
    ni = ieee80211_find_node(&vap->iv_ic->ic_sta, macaddr);
    if (ni == NULL)
        return EINVAL;
    ni->ni_assocstatus = assoc_status;
    ni->ni_p2p_assocstatus = p2p_assoc_status;
    ieee80211_free_node(ni);
    
    return 0;
}

u_int32_t wlan_node_get_assoc_decision(wlan_if_t vap, u_int8_t *macaddr)
{
    
    struct ieee80211_node *ni;
    u_int32_t assocstatus;

    ni = ieee80211_find_node(&vap->iv_ic->ic_sta, macaddr);
    if (ni == NULL)
        return EINVAL;
    assocstatus = ni->ni_assocstatus;
    ieee80211_free_node(ni);

    return (assocstatus);
}

wlan_chan_t wlan_node_get_chan(wlan_node_t node)
{
    return node->ni_chan;
}

u_int32_t wlan_node_get_state_flag(wlan_node_t node)
{
    return node->ni_flags;
}

u_int8_t wlan_node_get_authmode(wlan_node_t node)
{
    return node->ni_authmode;
}

u_int8_t wlan_node_get_ath_flags(wlan_node_t node)
{
    return node->ni_ath_flags;
}

u_int8_t wlan_node_get_erp(wlan_node_t node)
{
    return node->ni_erp;
}

systick_t wlan_node_get_assocuptime(wlan_node_t node)
{
    return node->ni_assocuptime;
}

u_int16_t wlan_node_get_associd(wlan_node_t node)
{
    return ieee80211_node_get_associd((struct ieee80211_node *)node);
}

u_int16_t wlan_node_get_txpower(wlan_node_t node)
{
    return ieee80211_node_get_txpower((struct ieee80211_node *)node);
}

u_int16_t wlan_node_get_vlan(wlan_node_t node)
{
    return node->ni_vlan;
}

int
wlan_node_get_ucast_ciphers(wlan_node_t node, ieee80211_cipher_type types[], u_int len)
{
    struct ieee80211_node *ni = node;
    struct ieee80211_rsnparms *rsn = &ni->ni_rsn;
    ieee80211_cipher_type cipher;
    u_int count = 0;

    for (cipher = IEEE80211_CIPHER_WEP; cipher < IEEE80211_CIPHER_MAX; cipher++) {
        if (RSN_HAS_UCAST_CIPHER(rsn, cipher)) {
            /* Is input buffer big enough */
            if (len < count)
                return -EINVAL;

            types[count++] = cipher;
        }
    }

    return count;
}

void  wlan_node_get_txseqs(wlan_node_t node, u_int16_t *txseqs, u_int len)
{
    struct ieee80211_node *ni = node;

    if (len > sizeof(ni->ni_txseqs)) {
        len = sizeof(ni->ni_txseqs);
    }
    OS_MEMCPY(txseqs, ni->ni_txseqs, len);
}

void  wlan_node_get_rxseqs(wlan_node_t node, u_int16_t *rxseqs, u_int len)
{
    struct ieee80211_node *ni = node;

    if (len > sizeof(ni->ni_rxseqs)) {
        len = sizeof(ni->ni_rxseqs);
    }
    OS_MEMCPY(rxseqs, ni->ni_rxseqs, len);
}

u_int8_t wlan_node_get_uapsd(wlan_node_t node)
{
    return node->ni_uapsd;
}

u_int16_t wlan_node_get_inact(wlan_node_t node)
{
    struct ieee80211_node *ni = node;
    u_int16_t inact_time;

    /* NB: leave all cases in case we relax ni_associd == 0 check */
    if (ieee80211_node_is_authorized(ni)) {
        inact_time = ni->ni_vap->iv_inact_run;
    } else if (ni->ni_associd != 0) {
        inact_time = ni->ni_vap->iv_inact_auth;
    } else {
        inact_time = ni->ni_vap->iv_inact_init;
    }
    inact_time = (inact_time - ni->ni_inact) * IEEE80211_INACT_WAIT;

    return inact_time;
}

u_int16_t wlan_node_get_htcap(wlan_node_t node)
{
    return node->ni_htcap;
}

bool wlan_node_has_flag(struct ieee80211_node *ni, u_int16_t flag)
{
    return (ieee80211node_has_flag(ni, flag));
}

/* To check if WEP/TKIP Aggregation can be enabled for this node. */
int
ieee80211_has_weptkipaggr(struct ieee80211_node *ni)
{
    struct ieee80211com *ic = ni->ni_ic;

    /* Both the peer node and our hardware must support aggregation during wep/tkip */
    if ((ieee80211node_has_flag(ni, IEEE80211_NODE_WEPTKIPAGGR)) && 
        ieee80211com_has_athextcap(ic, IEEE80211_ATHEC_WEPTKIPAGGR)) {
        return 1;
    }
    return 0;
}


#if  ATH_SUPPORT_AOW
extern i2s_rx_dev_t *i2s;

/*
 * function : ieee80211_send2all_nodes
 * -----------------------------------------------------------
 *
 * input    : pointer to vap
 *            pointer to packet
 *            size of frame
 *
 * Transmit the data frame to all the associated nodes for the
 * given VAP interface with the copy of given data.
 *
 */
void
ieee80211_send2all_nodes( struct ieee80211vap *reqvap, 
                          void *data, 
                          int len, 
                          u_int32_t seqno, 
                          u_int64_t tsf)
{
    struct ieee80211com *ic = reqvap->iv_ic;
    struct ieee80211_node_table *nt = &ic->ic_sta;
    struct ieee80211_node *ni;
    rwlock_state_t lock_state;
    OS_BEACON_DECLARE_AND_RESET_VAR(flags);

    /* TODO : Review the Lock mechanism */

    OS_BEACON_READ_LOCK(&nt->nt_nodelock, &lock_state, flags);

    if (ic->ic_get_aow_playlocal(ic)) {
        ic->ic_start_aow_inter(ic, (tsf + ic->ic_get_aow_latency_us(ic)), 4000);
        i2s->write(len, data);
    }                 

    TAILQ_FOREACH(ni, &nt->nt_node, ni_list) {
        struct ieee80211vap *vap = ni->ni_vap;
        if ((vap && vap != reqvap))
            continue;
        /* send only to the associated node */
        if ((ni) && (ni->ni_associd) &&
             (ni != ni->ni_vap->iv_bss)) {
             (void) ieee80211_ref_node(ni);

             /* We ignore ESS for this path */

             /* Note the node reference is cleared by ieee80211_send_aow_data_ipformat() */
             ieee80211_send_aow_data_ipformat(ni, data, len, seqno, tsf, 0, 0);
        }
        
    }

    OS_BEACON_READ_UNLOCK(&nt->nt_nodelock, &lock_state, flags);

}EXPORT_SYMBOL(ieee80211_send2all_nodes);

#endif  /* ATH_SUPPORT_AOW */
void wlan_node_set_txpwr(wlan_if_t vap, u_int16_t txpowlevel, u_int8_t *addr)
{
    struct ieee80211_node_table *nt = &vap->iv_ic->ic_sta;
    struct ieee80211_node *ni;
    ni = ieee80211_find_node(nt, addr);
    ASSERT(ni);
    if (!ni)
        return;
    ieee80211node_set_txpower(ni, txpowlevel);
    ieee80211_free_node(ni);
}

int wlan_node_alloc_aid_bitmap(wlan_if_t vap, u_int32_t old_len)
{
    u_int8_t    *bitmap = NULL;
    u_int32_t   len = howmany(vap->iv_max_aid, 32) * sizeof(u_int32_t);

    //printk("[%s] entry\n",__func__);

    bitmap = OS_MALLOC(vap->iv_ic->ic_osdev, len, GFP_KERNEL);
    if(!bitmap) {
        vap->iv_max_aid = old_len;
        return -1;
    }
    OS_MEMZERO(bitmap, len);
    if (vap->iv_aid_bitmap) {
        OS_MEMCPY(bitmap, vap->iv_aid_bitmap, len > old_len ? old_len : len);
        OS_FREE(vap->iv_aid_bitmap);
    }
    vap->iv_aid_bitmap = (u_int32_t *)bitmap;

    //printk("[%s] exist\n",__func__);    

    return 0;
}


