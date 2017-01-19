/*
 *  Copyright (c) 2009 Atheros Communications Inc.
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
 * \file ieee80211_me.c
 * \brief Atheros multicast enhancement algorithm, as part of 
 * Atheros iQUE feature set.
 *
 * This file contains the main implementation of the Atheros multicast 
 * enhancement functionality.
 *
 * The main purpose of this module is to convert (by translating or 
 * tunneling) the multicast stream into duplicated unicast streams for 
 * performance enhancement of home wireless applications. For more 
 * details, please refer to the design documentation.
 *
 */

#include <ieee80211_ique.h>

#if ATH_PERF_PWR_OFFLOAD
#include "ol_if_athvar.h"
#endif

#if ATH_SUPPORT_IQUE
#include "osdep.h"
#include "ieee80211_me_priv.h"
#include "if_athproto.h"


static void ieee80211_me_add_member_list(struct MC_GROUP_LIST_ENTRY* grp_list,
                                         struct MC_LIST_UPDATE* list_entry);
static void ieee80211_me_remove_expired_member(struct MC_GROUP_LIST_ENTRY* grp_list,
                                               struct ieee80211vap* vap,
                                               u_int32_t nowtimestamp);
static void ieee80211_me_remove_all_member_grp(struct MC_GROUP_LIST_ENTRY* grp_list,
                                               u_int8_t* grp_member_addr);
static struct MC_GRP_MEMBER_LIST* ieee80211_me_find_member_src(struct MC_GROUP_LIST_ENTRY* grp_list,
                                                               u_int8_t* grp_member_addr,
                                                               u_int32_t src_ip_addr);
static struct MC_GRP_MEMBER_LIST* ieee80211_me_find_member(struct MC_GROUP_LIST_ENTRY* grp_list,
                                                           u_int8_t* grp_member_addr);
static struct MC_GROUP_LIST_ENTRY*
ieee80211_me_find_group_list(struct ieee80211vap* vap, uint8_t *grp_addr);
static struct MC_GROUP_LIST_ENTRY* ieee80211_me_create_grp_list(struct ieee80211vap* vap,
                                                                u_int8_t* grp_addr);
static int ieee80211_me_SnoopIsDenied(struct ieee80211vap *vap, u_int32_t grpaddr);
static void ieee80211_me_SnoopListUpdate(struct MC_LIST_UPDATE* list_entry);
static u_int8_t ieee80211_me_count_member_anysrclist(struct MC_GROUP_LIST_ENTRY* grp_list,
                                                     u_int8_t* table,
                                                     u_int32_t timestamp);
static u_int8_t ieee80211_me_count_member_src_list(struct MC_GROUP_LIST_ENTRY* grp_list,
                                                   u_int32_t src_ip_addr,
                                                   u_int8_t* table, u_int32_t timestamp);
static uint8_t ieee80211_me_SnoopListGetMember(struct ieee80211vap* vap, uint8_t* grp_addr,
                                               u_int32_t src_ip_addr, uint8_t* table);
static void ieee80211_me_remove_node_grp(struct MC_GROUP_LIST_ENTRY* grp_list,
                                         struct ieee80211_node* ni);
static void ieee80211_me_clean_snp_list(struct ieee80211vap* vap);
static void ieee80211_me_SnoopListInit(struct ieee80211vap *vap);
static void ieee80211_me_SnoopInspecting(struct ieee80211vap *vap,
                                         struct ieee80211_node *ni, wbuf_t wbuf);
static void ieee80211_me_SnoopWDSNodeCleanup(struct ieee80211_node* ni);
static int ieee80211_me_SnoopConvert(struct ieee80211vap *vap, wbuf_t wbuf);
static void ieee80211_me_detach(struct ieee80211vap *vap);
static void ieee80211_me_SnoopShowDenyTable(struct ieee80211vap *vap);
static void ieee80211_me_SnoopClearDenyTable(struct ieee80211vap *vap);
static void ieee80211_me_SnoopAddDenyEntry(struct ieee80211vap *vap, int *grpaddr);
static void ieee80211_me_SnoopListDump(struct ieee80211vap* vap);
int ieee80211_me_attach(struct ieee80211vap * vap);
static void ol_ieee80211_add_member_list(struct MC_LIST_UPDATE* list_entry);
static void ol_ieee80211_remove_node_grp(struct MC_GRP_MEMBER_LIST *grp_member_list, 
                                      struct ieee80211vap *vap);
static void ol_ieee80211_remove_all_node_grp(struct MC_GRP_MEMBER_LIST *grp_member_list,
                              struct ieee80211vap *vap);
  
static void ol_ieee80211_add_member_list(struct MC_LIST_UPDATE* list_entry) 
{
    struct ieee80211vap *vap = list_entry->vap;
    int action = IGMP_ACTION_ADD_MEMBER, wildcard = IGMP_WILDCARD_SINGLE;
    
    if (!(vap->iv_ic->ic_is_mode_offload(vap->iv_ic))) {
        return;
    }
    vap->iv_ic->ic_mcast_group_update(vap->iv_ic, action, wildcard, &list_entry->grpaddr, 
                                      IGMP_IP_ADDR_LENGTH, list_entry->grp_member);
}

/* Add a member to the list */
static void
ieee80211_me_add_member_list(struct MC_GROUP_LIST_ENTRY* grp_list,
                             struct MC_LIST_UPDATE* list_entry) 
{
    struct MC_GRP_MEMBER_LIST* grp_member_list;
    struct MC_SNOOP_LIST *snp_list = &list_entry->vap->iv_me->ieee80211_me_snooplist;

    if(list_entry->vap->iv_ic->ic_is_mode_offload(list_entry->vap->iv_ic) && 
       atomic_dec_and_test(&snp_list->msl_group_member_limit)) 
    {
        /*
         * There is no space for more members,
         * so undo the allocation done by the "dec"
         * of the above atomic_dec_and_test.
         */
        atomic_inc(&snp_list->msl_group_member_limit);
        return;
    }
    grp_member_list = (struct MC_GRP_MEMBER_LIST*) OS_MALLOC(
                       list_entry->vap->iv_ic->ic_osdev,
                       sizeof(struct MC_GRP_MEMBER_LIST),
                       GFP_KERNEL);

    if(grp_member_list != NULL){
        TAILQ_INSERT_TAIL(&grp_list->src_list, grp_member_list, member_list);
        grp_member_list->src_ip_addr = list_entry->src_ip_addr;
        grp_member_list->niptr       = list_entry->ni;
        grp_member_list->vap         = list_entry->vap;
        grp_member_list->mode        = list_entry->cmd;
        grp_member_list->timestamp   = list_entry->timestamp;
        IEEE80211_ADDR_COPY(grp_member_list->grp_member_addr,list_entry->grp_member);
        grp_member_list->grpaddr = list_entry->grpaddr;
        ol_ieee80211_add_member_list(list_entry);
    } else {
        atomic_inc(&snp_list->msl_group_member_limit);
    }
} 

/* Remove group members whose timer are expired */
static void
ieee80211_me_remove_expired_member(struct MC_GROUP_LIST_ENTRY* grp_list,
                                   struct ieee80211vap* vap,
                                   u_int32_t nowtimestamp)
{
    struct MC_GRP_MEMBER_LIST* curr_grp_member_list;
    struct MC_GRP_MEMBER_LIST* next_grp_member_list;

    curr_grp_member_list = TAILQ_FIRST(&grp_list->src_list);

    while(curr_grp_member_list != NULL){
       next_grp_member_list = TAILQ_NEXT(curr_grp_member_list, member_list);
       /* if timeout remove member*/
       if((nowtimestamp - curr_grp_member_list->timestamp) > vap->iv_me->me_timeout){
           /* remove the member from list*/
           TAILQ_REMOVE(&grp_list->src_list, curr_grp_member_list, member_list);
           ol_ieee80211_remove_node_grp(curr_grp_member_list, vap);
           OS_FREE(curr_grp_member_list);
       }
       curr_grp_member_list = next_grp_member_list;
    }
}

/* Remove all entry of a given member address from a group list*/
static void
ieee80211_me_remove_all_member_grp(struct MC_GROUP_LIST_ENTRY* grp_list,
                                   u_int8_t* grp_member_addr)
{
    struct MC_GRP_MEMBER_LIST* curr_grp_member_list;
    struct MC_GRP_MEMBER_LIST* next_grp_member_list;

    curr_grp_member_list = TAILQ_FIRST(&grp_list->src_list);

    while(curr_grp_member_list != NULL){
       next_grp_member_list = TAILQ_NEXT(curr_grp_member_list, member_list);
       if(IEEE80211_ADDR_EQ(curr_grp_member_list->grp_member_addr,grp_member_addr)){
           /* remove the member from list*/
           TAILQ_REMOVE(&grp_list->src_list, curr_grp_member_list, member_list);
           ol_ieee80211_remove_node_grp(curr_grp_member_list, curr_grp_member_list->vap);
           OS_FREE(curr_grp_member_list);
       }
       curr_grp_member_list = next_grp_member_list;
    }
}

/* find a member based on member mac addr and source ip address */
static struct MC_GRP_MEMBER_LIST*
ieee80211_me_find_member_src(struct MC_GROUP_LIST_ENTRY* grp_list,
                             u_int8_t* grp_member_addr,
                             u_int32_t src_ip_addr)
{
    struct MC_GRP_MEMBER_LIST* grp_member_list;
    TAILQ_FOREACH(grp_member_list, &grp_list->src_list,member_list){
        if((IEEE80211_ADDR_EQ(grp_member_list->grp_member_addr,grp_member_addr))
            &&  (grp_member_list->src_ip_addr == src_ip_addr))
        {
            return(grp_member_list);
        }
    }
    return(NULL);
}

/* find a member based on group member mac addr */
static struct MC_GRP_MEMBER_LIST*
ieee80211_me_find_member(struct MC_GROUP_LIST_ENTRY* grp_list,
                         u_int8_t* grp_member_addr)
{
    struct MC_GRP_MEMBER_LIST* grp_member_list;

    TAILQ_FOREACH(grp_member_list, &grp_list->src_list,member_list){
       if(IEEE80211_ADDR_EQ(grp_member_list->grp_member_addr,grp_member_addr)){
           return(grp_member_list);
       }
    }
    return(NULL);
}

/* find a group based on group address */
static struct MC_GROUP_LIST_ENTRY*
ieee80211_me_find_group_list(struct ieee80211vap* vap, uint8_t *grp_addr)
{
    struct MC_SNOOP_LIST       *snp_list;
    struct MC_GROUP_LIST_ENTRY *grp_list;

    snp_list = &(vap->iv_me->ieee80211_me_snooplist);
    TAILQ_FOREACH(grp_list,&snp_list->msl_node,grp_list){
       if(IEEE80211_ADDR_EQ(grp_addr, grp_list->group_addr)){
           return(grp_list);
       }
    }
    return(NULL);
}

/* create a group list
 * find is group list is already present,
 * if no group list found, then create a new list
 * if group list is alreay existing return that group list */
static struct MC_GROUP_LIST_ENTRY*
ieee80211_me_create_grp_list(struct ieee80211vap* vap, u_int8_t* grp_addr)
{
    struct MC_SNOOP_LIST       *snp_list;
    struct MC_GROUP_LIST_ENTRY *grp_list;
    rwlock_state_t lock_state;

    snp_list = &vap->iv_me->ieee80211_me_snooplist;
    grp_list = ieee80211_me_find_group_list(vap,grp_addr);
    
    if(grp_list == NULL){
        if(vap->iv_ic->ic_is_mode_offload(vap->iv_ic) && 
           atomic_dec_and_test(&snp_list->msl_group_list_limit)) 
        {            
            /*
             * There is no space for more groups,
             * so undo the allocation done by the "dec"
             * of the above atomic_dec_and_test.
             */
            atomic_inc(&snp_list->msl_group_list_limit);
            return NULL;
        }
 
        grp_list = (struct MC_GROUP_LIST_ENTRY *)OS_MALLOC(
                   vap-> iv_ic->ic_osdev,       
                   sizeof(struct MC_GROUP_LIST_ENTRY),
                   GFP_KERNEL);

        if(grp_list != NULL){
            IEEE80211_SNOOP_LOCK(snp_list,&lock_state);
            TAILQ_INSERT_TAIL(&snp_list->msl_node,grp_list,grp_list);
            IEEE80211_ADDR_COPY(grp_list->group_addr,grp_addr);
            TAILQ_INIT(&grp_list->src_list);
            IEEE80211_SNOOP_UNLOCK(snp_list, &lock_state);
        } else {
            atomic_inc(&snp_list->msl_group_list_limit);
        }
    }
    return(grp_list);
}

/**************************************************************************
 * !
 * \brief Show the Group Deny Table
 *
 * \param vap
 *
 * \return N/A
 */
static void ieee80211_me_SnoopShowDenyTable(struct ieee80211vap *vap)
{
    int idx;
    struct MC_SNOOP_LIST *ps;
    rwlock_state_t lock_state;

    ps = &(vap->iv_me->ieee80211_me_snooplist);
    IEEE80211_SNOOP_LOCK(ps, &lock_state);
    if (ps->msl_deny_count == 0) {
        IEEE80211_SNOOP_UNLOCK(ps, &lock_state);
        return;
    }
    for (idx = 0 ; idx < ps->msl_deny_count; idx ++) {
        printk("%d - addr : %d.%d.%d.%d, mask : %d.%d.%d.%d\n", idx + 1,
               (ps->msl_deny_group[idx] >> 24) & 0xff,   
               (ps->msl_deny_group[idx] >> 16) & 0xff,   
               (ps->msl_deny_group[idx] >> 8) & 0xff,    
               (ps->msl_deny_group[idx] & 0xff),   
               (ps->msl_deny_mask[idx] >> 24) & 0xff,   
               (ps->msl_deny_mask[idx] >> 16) & 0xff,   
               (ps->msl_deny_mask[idx] >> 8) & 0xff,   
               (ps->msl_deny_mask[idx] & 0xff));   
    }
    IEEE80211_SNOOP_UNLOCK(ps, &lock_state);
}

/*
 * Check if the address is in deny list
 */
static int
ieee80211_me_SnoopIsDenied(struct ieee80211vap *vap, u_int32_t grpaddr)
{
    int idx;
    struct MC_SNOOP_LIST *ps;
    ps = &(vap->iv_me->ieee80211_me_snooplist);
    
    if (ps->msl_deny_count == 0) {
        return 0;
    }
    for (idx = 0 ; idx < ps->msl_deny_count; idx ++) {
        if (grpaddr != ps->msl_deny_group[idx])
        {
             continue;   
        }
        return 1;
    }
    return 0;
}

/*
 * Clear the Snoop Deny Table
 */
static void 
ieee80211_me_SnoopClearDenyTable(struct ieee80211vap *vap)
{
    struct MC_SNOOP_LIST *ps;
    
    ps = &(vap->iv_me->ieee80211_me_snooplist);
    ps->msl_deny_count = 0;
    return;
}

/*
 * Add the Snoop Deny Table entry
 */
static void 
ieee80211_me_SnoopAddDenyEntry(struct ieee80211vap *vap, int *grpaddr)
{
    u_int8_t idx;
    struct MC_SNOOP_LIST *ps;
    rwlock_state_t lock_state;
    ps = &(vap->iv_me->ieee80211_me_snooplist);
    idx = ps->msl_deny_count;
    IEEE80211_SNOOP_LOCK(ps, &lock_state);
    ps->msl_deny_count ++;
    ps->msl_deny_group[idx] = *(u_int32_t*)grpaddr;
    ps->msl_deny_mask[idx]  = 4294967040UL;

    IEEE80211_SNOOP_UNLOCK(ps, &lock_state);

    return;
}

/* print all the group entries */
static void
ieee80211_me_SnoopListDump(struct ieee80211vap* vap)
{
    struct MC_SNOOP_LIST *snp_list = &(vap->iv_me->ieee80211_me_snooplist);
    struct MC_GROUP_LIST_ENTRY* grp_list;
    struct MC_GRP_MEMBER_LIST* grp_member_list;
    rwlock_state_t lock_state;

    if(vap->iv_opmode == IEEE80211_M_HOSTAP &&
        vap->iv_me->mc_snoop_enable &&
        snp_list != NULL)
    {
        IEEE80211_SNOOP_LOCK(snp_list,&lock_state);
        TAILQ_FOREACH(grp_list,&snp_list->msl_node,grp_list){
            printk("group addr %x:%x:%x:%x:%x:%x \n",grp_list->group_addr[0],
                                                     grp_list->group_addr[1],
                                                     grp_list->group_addr[2],
                                                     grp_list->group_addr[3],
                                                     grp_list->group_addr[4],
                                                     grp_list->group_addr[5]);
             
            TAILQ_FOREACH(grp_member_list, &grp_list->src_list,member_list){
                printk("src_ip_addr %d:%d:%d:%d \n",((grp_member_list->src_ip_addr >> 24) & 0xff),
                                                    ((grp_member_list->src_ip_addr >> 16) & 0xff),
                                                    ((grp_member_list->src_ip_addr >>  8) & 0xff),
                                                    ((grp_member_list->src_ip_addr) & 0xff));
                printk("member addr %x:%x:%x:%x:%x:%x\n",grp_member_list->grp_member_addr[0],
                                                         grp_member_list->grp_member_addr[1],
                                                         grp_member_list->grp_member_addr[2],
                                                         grp_member_list->grp_member_addr[3],
                                                         grp_member_list->grp_member_addr[4],
                                                         grp_member_list->grp_member_addr[5]);

                printk("Mode %d \n",grp_member_list->mode); 
            }
        }
        IEEE80211_SNOOP_UNLOCK(snp_list, &lock_state);
    }
}

/* update the snoop table based on the received entries*/
static void
ieee80211_me_SnoopListUpdate(struct MC_LIST_UPDATE* list_entry)
{
    struct MC_SNOOP_LIST* snp_list;
    struct MC_GROUP_LIST_ENTRY* grp_list;
    struct MC_GRP_MEMBER_LIST* grp_member_list;
    systime_t timestamp;
    rwlock_state_t lock_state;
   
    timestamp  = OS_GET_TIMESTAMP();
    list_entry->timestamp = (u_int32_t) CONVERT_SYSTEM_TIME_TO_MS(timestamp);
    snp_list = &(list_entry->vap->iv_me->ieee80211_me_snooplist);
    
    grp_list = ieee80211_me_create_grp_list(list_entry->vap,list_entry->grp_addr);

    if(grp_list != NULL){
        IEEE80211_SNOOP_LOCK(snp_list,&lock_state);
        /* if entry is for particular source then,
         * find is member is already list in the table
         * if yes update the entry with new values
         * if no create a new entry and add member details*/
        if(list_entry->src_ip_addr){
            /*Not any source */
           grp_member_list = ieee80211_me_find_member_src(grp_list,list_entry->grp_member,list_entry->src_ip_addr);
            if(grp_member_list != NULL){
                grp_member_list->mode      = list_entry->cmd;
                grp_member_list->timestamp = list_entry->timestamp;
            } else {
                ieee80211_me_add_member_list(grp_list,list_entry);
            }
        } else {
            /*Any source
              find the grp_member, if src address is non zero, there is possiblity of few more members list
              if src_ip_address is zero then check the cmd, any source no action, exclude_list remove the entry
              Remove all grp member entries
              if cmd is any source, then include in the list, src addr 0 mode include
              if cmd is exclude_list, no need to add the list*/
            grp_member_list = ieee80211_me_find_member(grp_list,list_entry->grp_member);
            if(grp_member_list != NULL){
                if(grp_member_list->src_ip_addr) {
                    ieee80211_me_remove_all_member_grp(grp_list,list_entry->grp_member); 
                    if(grp_member_list->mode == IGMP_SNOOP_CMD_ADD_INC_LIST) {
                        /* alloc memory
                           add to the list*/
                       ieee80211_me_add_member_list(grp_list,list_entry);  
                    }
                } else if(list_entry->cmd == IGMP_SNOOP_CMD_ADD_EXC_LIST) {
                    /* remove the member from list*/
                    TAILQ_REMOVE(&grp_list->src_list, grp_member_list, member_list);
                    ol_ieee80211_remove_node_grp(grp_member_list, list_entry->vap);
                    OS_FREE(grp_member_list);              
                }
            } else {
                if(list_entry->cmd != IGMP_SNOOP_CMD_ADD_EXC_LIST) {
                    /* Alloc memory and add to the list*/
                    ieee80211_me_add_member_list(grp_list,list_entry);
                }

            }
        }
        IEEE80211_SNOOP_UNLOCK(snp_list, &lock_state);
    }
}

/*************************************************************************
 * !
 * \brief Build up the snoop list by monitoring the IGMP packets
 *
 * \param ni Pointer to the node
 *           skb Pointer to the skb buffer
 *
 * \return N/A
 */
static void
ieee80211_me_SnoopInspecting(struct ieee80211vap *vap, struct ieee80211_node *ni, wbuf_t wbuf)
{
    struct ether_header *eh;
    
    eh = (struct ether_header *) wbuf_header(wbuf);

    if (!vap->iv_me->mc_snoop_enable){
        return;
    }

    if (vap->iv_opmode == IEEE80211_M_HOSTAP ) {
        if (IEEE80211_IS_MULTICAST(eh->ether_dhost) &&
            !IEEE80211_IS_BROADCAST(eh->ether_dhost)) {
            if (ntohs(eh->ether_type) == ETHERTYPE_IP) {
                const struct ip_header *ip = (struct ip_header *)
                    (wbuf_header(wbuf) + sizeof (struct ether_header));
                if (ip->protocol == IPPROTO_IGMP) {
                    int                         ip_headerlen;       /*ip header len based on endiness*/
                    const struct igmp_header    *igmp;              /* igmp header for v1 and v2*/
                    const struct igmp_v3_report *igmpr3;            /* igmp header for v3*/
                    const struct igmp_v3_grec   *grec;              /* igmp group record*/
                    u_int32_t                     groupAddr = 0;     /* to hold group address from group record*/
                    u_int8_t                     *srcAddr = eh->ether_shost; /* source address which send the report and it is the member*/
                    u_int8_t                     groupAddrL2[IEEE80211_ADDR_LEN]; /*group multicast mac address*/
                    struct MC_SNOOP_LIST*        snp_list;
                    struct MC_LIST_UPDATE        list_entry;        /* list entry where all member details will be updated and passed on updating the snoop list*/
                    struct MC_GROUP_LIST_ENTRY*  grp_list;
                    struct MC_GRP_MEMBER_LIST*   grp_member_list;
                    rwlock_state_t lock_state;

                    /* Init multicast group address conversion */
                    groupAddrL2[0] = 0x01;
                    groupAddrL2[1] = 0x00;
                    groupAddrL2[2] = 0x5e;
                    
                    /*pre fill the list_entry members which are going to be const in this function*/
                    IEEE80211_ADDR_COPY(list_entry.grp_member,srcAddr);
                    list_entry.vap        = vap;
                    list_entry.ni         = ni;
                    
                    ip_headerlen = ip->version_ihl & 0x0F;

                    /* v1 & v2 */    
                    igmp = (struct igmp_header *)
                        (wbuf_header(wbuf) + sizeof (struct ether_header) + (4 * ip_headerlen));
                    /* ver 3*/
                    igmpr3 = (struct igmp_v3_report *) igmp;

                    /* If packet is not IGMP report or IGMP leave don't handle it*/
                    if(!IS_IGMP_REPORT_LEAVE_PACKET(igmp->type)){
                        return;
                    }
                    if(igmp->type != IGMPV3_REPORT_TYPE){
                        groupAddr = ntohl(igmp->group);
                        /* Check is group addres is in deny list */
                        if(ieee80211_me_SnoopIsDenied(vap,groupAddr)){
                            return;
                        }
                        if(igmp->type != IGMPV2_LEAVE_TYPE){
                            list_entry.cmd = IGMP_SNOOP_CMD_ADD_INC_LIST;
                        } else {
                            list_entry.cmd = IGMP_SNOOP_CMD_ADD_EXC_LIST;
                        }
                        groupAddrL2[3] = (groupAddr >> 16) & 0x7f;
                        groupAddrL2[4] = (groupAddr >>  8) & 0xff;
                        groupAddrL2[5] = (groupAddr >>  0) & 0xff;
                        list_entry.grpaddr = groupAddr;
                        IEEE80211_ADDR_COPY(list_entry.grp_addr,groupAddrL2);
                        list_entry.src_ip_addr = 0;
                        ieee80211_me_SnoopListUpdate(&list_entry);
                    } else {
                        u_int16_t no_grec; /* no of group records  */
                        u_int16_t no_srec; /* no of source records */

                        /*  V3 report handling */
                        no_grec = ntohs(igmpr3->ngrec);
                        snp_list = &(vap->iv_me->ieee80211_me_snooplist);
                        grec = (struct igmp_v3_grec*)((u_int8_t*)igmpr3 + 8);
                        /* loop to handle all group records */
                        while(no_grec){
                            u_int32_t *src_addr;
                            /* Group recod handling */
                            /* no of srcs record    */
                            list_entry.cmd = grec->grec_type;
                            groupAddr = ntohl(grec->grec_mca);
                            /* Check is group addres is in deny list */
                            if(ieee80211_me_SnoopIsDenied(vap,groupAddr)){
                                /* move the grec to next group record*/
                                grec = (struct igmp_v3_grec*)((u_int8_t*)grec + IGMPV3_GRP_REC_LEN(grec));
                                no_grec--;
                                continue;
                            }
                            no_srec = ntohs(grec->grec_nsrcs);
                            src_addr = (u_int32_t*)((u_int8_t*)grec + sizeof(struct igmp_v3_grec));
                            groupAddrL2[3] = (groupAddr >> 16) & 0x7f;
                            groupAddrL2[4] = (groupAddr >>  8) & 0xff;
                            groupAddrL2[5] = (groupAddr >>  0) & 0xff;
                            IEEE80211_ADDR_COPY(list_entry.grp_addr,groupAddrL2);
                            list_entry.grpaddr = groupAddr;                          
                            if (grec->grec_type == IGMPV3_CHANGE_TO_EXCLUDE ||
                                grec->grec_type == IGMPV3_MODE_IS_EXCLUDE)
                            {
                                list_entry.cmd = IGMP_SNOOP_CMD_ADD_EXC_LIST;
                                /* remove old member entries as new member entries are received */
                                grp_list = ieee80211_me_find_group_list(vap,list_entry.grp_addr);
                                if(grp_list != NULL){
                                    IEEE80211_SNOOP_LOCK(snp_list,&lock_state);
                                    ieee80211_me_remove_all_member_grp(grp_list,list_entry.grp_member);
                                    IEEE80211_SNOOP_UNLOCK(snp_list, &lock_state);
                                }
                                 /* if no source record is there then it is include for all source */
                                if(!no_srec){
                                    list_entry.src_ip_addr = 0;
                                    list_entry.cmd = IGMP_SNOOP_CMD_ADD_INC_LIST;
                                    ieee80211_me_SnoopListUpdate(&list_entry);
                                }
                            } else if (grec->grec_type == IGMPV3_CHANGE_TO_INCLUDE ||
                                       grec->grec_type == IGMPV3_MODE_IS_INCLUDE)
                            {
                                list_entry.cmd = IGMP_SNOOP_CMD_ADD_INC_LIST;
                                grp_list = ieee80211_me_find_group_list(vap,list_entry.grp_addr);
                                if(grp_list != NULL){
                                    IEEE80211_SNOOP_LOCK(snp_list,&lock_state);
                                    ieee80211_me_remove_all_member_grp(grp_list,list_entry.grp_member); 
                                    IEEE80211_SNOOP_UNLOCK(snp_list, &lock_state);
                                /* if no of srec is zero and mode is include, it is exclude for all source
                                 * no need to update as member is already removed in above lines */
                                }
                            } else if  (grec->grec_type == IGMPV3_ALLOW_NEW_SOURCES) {
                                list_entry.cmd = IGMP_SNOOP_CMD_ADD_INC_LIST;
                            }
 
                            while(no_srec){
                                list_entry.src_ip_addr = ntohl(*src_addr);
                                if(grec->grec_type != IGMPV3_BLOCK_OLD_SOURCES){
                                    /* Source record handling*/
                                    ieee80211_me_SnoopListUpdate(&list_entry);
                                } else {
                                    grp_list = ieee80211_me_find_group_list(vap,list_entry.grp_addr);
                                    if(grp_list != NULL){
                                        grp_member_list = ieee80211_me_find_member_src(grp_list,
                                                               list_entry.grp_member,list_entry.src_ip_addr);
                                        if(grp_member_list != NULL){
                                            TAILQ_REMOVE(&grp_list->src_list, grp_member_list, member_list);
                                            ol_ieee80211_remove_node_grp(grp_member_list, list_entry.vap);
                                            OS_FREE(grp_member_list);
                                        }
                                    }
                                }
                                src_addr++;
                                no_srec--;
                            } /* while of no_srec*/
                            /* move the grec to next group record*/
                            grec = (struct igmp_v3_grec*)((u_int8_t*)grec + IGMPV3_GRP_REC_LEN(grec));
                            no_grec--;
                        } /*while of no_grec*/
                    } /* else of IGMPV3_REPORT_TYPE*/
                }
            }
        }
    }

    /* multicast tunnel egress */
    if (ntohs(eh->ether_type) == ATH_ETH_TYPE &&
        vap->iv_opmode == IEEE80211_M_STA &&
        vap->iv_me->mc_snoop_enable)
    {
        struct athl2p_tunnel_hdr *tunHdr;

        tunHdr = (struct athl2p_tunnel_hdr *) wbuf_pull(wbuf, sizeof(struct ether_header));
        /* check protocol subtype */
        eh = (struct ether_header *) wbuf_pull(wbuf, sizeof(struct athl2p_tunnel_hdr));
    }
    return;
}

/* count no of members with anysrc, add the member mac address in the table */
static u_int8_t
ieee80211_me_count_member_anysrclist(struct MC_GROUP_LIST_ENTRY* grp_list, u_int8_t* table,
                                     u_int32_t timestamp)
{
    struct MC_GRP_MEMBER_LIST* grp_member_list;
    u_int8_t count = 0;
    
    TAILQ_FOREACH(grp_member_list, &grp_list->src_list,member_list){
        if(grp_member_list->src_ip_addr == 0){
            IEEE80211_ADDR_COPY(&table[count * IEEE80211_ADDR_LEN],grp_member_list->grp_member_addr);
            grp_member_list->timestamp = timestamp;
            count++;
        }
    }
    return(count);
}

/* count no of members with matching src ip, add the member mac address in the table */
static u_int8_t
ieee80211_me_count_member_src_list(struct MC_GROUP_LIST_ENTRY* grp_list,
                                   u_int32_t src_ip_addr,
                                   u_int8_t* table, u_int32_t timestamp)
{
    struct MC_GRP_MEMBER_LIST* grp_member_list;
    u_int8_t count = 0;
    TAILQ_FOREACH(grp_member_list, &grp_list->src_list,member_list){

        /*  if src addres matches and mode is include then add the list in table*/
        if(grp_member_list->src_ip_addr == src_ip_addr ){
            if(grp_member_list->mode == IGMP_SNOOP_CMD_ADD_INC_LIST){
                IEEE80211_ADDR_COPY(&table[count * IEEE80211_ADDR_LEN],grp_member_list->grp_member_addr);
                grp_member_list->timestamp = timestamp;
                count++;
            }
        } else {
        /* if src address not match and mode is exclude then include in table*/
            if(grp_member_list->mode == IGMP_SNOOP_CMD_ADD_EXC_LIST){
                IEEE80211_ADDR_COPY(&table[count * IEEE80211_ADDR_LEN],grp_member_list->grp_member_addr);
                grp_member_list->timestamp = timestamp;
                count++;
            }
        }
    }
    return(count);
}

/* Getmembers list from snoop table for current data */
static uint8_t
ieee80211_me_SnoopListGetMember(struct ieee80211vap* vap, uint8_t* grp_addr,
                                u_int32_t src_ip_addr,uint8_t* table)
{
    struct MC_SNOOP_LIST* snp_list;
    struct MC_GROUP_LIST_ENTRY* grp_list;
    uint8_t count;
    rwlock_state_t lock_state;

    systime_t timestamp  = OS_GET_TIMESTAMP();
    u_int32_t now        = (u_int32_t) CONVERT_SYSTEM_TIME_TO_MS(timestamp);

    count = 0;
    snp_list = &(vap->iv_me->ieee80211_me_snooplist);
    IEEE80211_SNOOP_LOCK(snp_list,&lock_state);
    grp_list = ieee80211_me_find_group_list(vap,grp_addr);
    
    if(grp_list != NULL){
        count  = ieee80211_me_count_member_anysrclist(grp_list,&table[0],now);
        count += ieee80211_me_count_member_src_list(grp_list,src_ip_addr,&table[count],now);
    }
    IEEE80211_SNOOP_UNLOCK(snp_list, &lock_state);
    return(count);

}

static void 
ol_ieee80211_remove_node_grp(struct MC_GRP_MEMBER_LIST *grp_member_list, 
                          struct ieee80211vap *vap) 
{
    int action = IGMP_ACTION_DELETE_MEMBER, wildcard = IGMP_WILDCARD_SINGLE;
    struct MC_SNOOP_LIST *snp_list;

    if (!(vap->iv_ic->ic_is_mode_offload(vap->iv_ic))) {
        return;
    }
    vap->iv_ic->ic_mcast_group_update(vap->iv_ic, action, wildcard, &grp_member_list->grpaddr, 
                                      IGMP_IP_ADDR_LENGTH, grp_member_list->grp_member_addr);
    snp_list = &vap->iv_me->ieee80211_me_snooplist;
    atomic_inc(&snp_list->msl_group_member_limit);
}

/* this function is called only for cleaning up the snoop table */
static void 
ol_ieee80211_remove_all_node_grp(struct MC_GRP_MEMBER_LIST *grp_member_list,
                              struct ieee80211vap *vap) 
{
    int action = IGMP_ACTION_DELETE_MEMBER, wildcard = IGMP_WILDCARD_ALL;
    struct MC_SNOOP_LIST *snp_list = &vap->iv_me->ieee80211_me_snooplist;

    if (!(vap->iv_ic->ic_is_mode_offload(vap->iv_ic))) {
        return;
    }
    vap->iv_ic->ic_mcast_group_update(vap->iv_ic, action, wildcard, &grp_member_list->grpaddr, 
                                      IGMP_IP_ADDR_LENGTH, grp_member_list->grp_member_addr);
    atomic_inc(&snp_list->msl_group_list_limit);
}

/* remove particular node from the group list, used mainly for node cleanup
 * if node pointer is NULL, then remove all the node in the group, used mainly for clean group list */
static void
ieee80211_me_remove_node_grp(struct MC_GROUP_LIST_ENTRY* grp_list,struct ieee80211_node* ni)
{
    struct MC_GRP_MEMBER_LIST* curr_grp_member_list;
    struct MC_GRP_MEMBER_LIST* next_grp_member_list;
    
    curr_grp_member_list = TAILQ_FIRST(&grp_list->src_list);
    if ((curr_grp_member_list != NULL) && (ni == NULL)) {
        ol_ieee80211_remove_all_node_grp(curr_grp_member_list,curr_grp_member_list->vap);
    }
    while(curr_grp_member_list != NULL){
       next_grp_member_list = TAILQ_NEXT(curr_grp_member_list, member_list);
       if((curr_grp_member_list->niptr == ni) || (ni == NULL)){
           /* remove the member from list*/
           TAILQ_REMOVE(&grp_list->src_list, curr_grp_member_list, member_list);
           if (ni != NULL) {
               ol_ieee80211_remove_node_grp(curr_grp_member_list, curr_grp_member_list->vap);
           }
           OS_FREE(curr_grp_member_list);
       }
       curr_grp_member_list = next_grp_member_list;
    }
}

/* clean the snoop list, remove all group and member entries */
static void
ieee80211_me_clean_snp_list(struct ieee80211vap* vap)
{
    struct MC_SNOOP_LIST *snp_list = &(vap->iv_me->ieee80211_me_snooplist);
    struct MC_GROUP_LIST_ENTRY* curr_grp_list;
    struct MC_GROUP_LIST_ENTRY* next_grp_list;
    rwlock_state_t lock_state;

    IEEE80211_SNOOP_LOCK(snp_list,&lock_state);
    curr_grp_list = TAILQ_FIRST(&snp_list->msl_node);    
    while(curr_grp_list != NULL){
        next_grp_list = TAILQ_NEXT(curr_grp_list,grp_list);
        ieee80211_me_remove_node_grp(curr_grp_list,NULL);
        TAILQ_REMOVE(&snp_list->msl_node,curr_grp_list,grp_list);
        OS_FREE(curr_grp_list);
        curr_grp_list = next_grp_list;
    }
    IEEE80211_SNOOP_UNLOCK(snp_list, &lock_state);
}

/* remove the node from the snoop list*/
static void
ieee80211_me_SnoopWDSNodeCleanup(struct ieee80211_node* ni)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct MC_SNOOP_LIST *snp_list = &(vap->iv_me->ieee80211_me_snooplist);
    struct MC_GROUP_LIST_ENTRY* curr_grp_list;
    struct MC_GROUP_LIST_ENTRY* next_grp_list;
    rwlock_state_t lock_state;

    if(vap->iv_opmode == IEEE80211_M_HOSTAP &&
        vap->iv_me->mc_snoop_enable &&
        snp_list != NULL)
    {
        IEEE80211_SNOOP_LOCK(snp_list,&lock_state);
        curr_grp_list = TAILQ_FIRST(&snp_list->msl_node);    
        while(curr_grp_list != NULL){
            next_grp_list = TAILQ_NEXT(curr_grp_list,grp_list);
            ieee80211_me_remove_node_grp(curr_grp_list,ni);

            if(!TAILQ_FIRST(&curr_grp_list->src_list)){
                TAILQ_REMOVE(&snp_list->msl_node,curr_grp_list,grp_list);
                OS_FREE(curr_grp_list);
            }
            curr_grp_list = next_grp_list;
        }
        IEEE80211_SNOOP_UNLOCK(snp_list, &lock_state);
    }
}

/*******************************************************************
 * !
 * \brief Mcast enhancement option 1: Tunneling, or option 2: Translate
 *
 * Add an IEEE802.3 header to the mcast packet using node's MAC address
 * as the destination address
 *
 * \param 
 *             vap Pointer to the virtual AP
 *             wbuf Pointer to the wbuf
 *
 * \return number of packets converted and transmitted, or 0 if failed
 */
static int
ieee80211_me_SnoopConvert(struct ieee80211vap *vap, wbuf_t wbuf)
{
    struct ieee80211_node *ni = NULL;
    struct ether_header *eh;
    u_int8_t *dstmac;                           /* reference to frame dst addr */
    u_int32_t src_ip_addr;
    u_int32_t grp_addr = 0;
    u_int8_t srcmac[IEEE80211_ADDR_LEN];    /* copy of original frame src addr */
    u_int8_t grpmac[IEEE80211_ADDR_LEN];    /* copy of original frame group addr */
                                            /* table of tunnel group dest mac addrs */
    u_int8_t empty_entry_mac[IEEE80211_ADDR_LEN];
    u_int8_t newmac[MAX_SNOOP_ENTRIES][IEEE80211_ADDR_LEN];
    uint8_t newmaccnt = 0;                        /* count of entries in newmac */
    uint8_t newmacidx = 0;                        /* local index into newmac */
    int xmited = 0;                            /* Number of packets converted and xmited */
    struct ether_header *eh2;                /* eth hdr of tunnelled frame */
    struct athl2p_tunnel_hdr *tunHdr;        /* tunnel header */
    wbuf_t wbuf1 = NULL;                    /* cloned wbuf if necessary */

    if ( (vap->iv_me->mc_snoop_enable == 0 ) || ( wbuf == NULL ) ) {
        /*
         * if snoop is disabled return -1 to indicate 
         * that the frame's not been transmitted and continue the 
         * regular xmit path in wlan_vap_send
         */
        return -1;
    }
    
    eh = (struct ether_header *)wbuf_header(wbuf);

    src_ip_addr = 0;
    /*  Extract the source ip address from the list*/
    if (vap->iv_opmode == IEEE80211_M_HOSTAP ) {
        if (IEEE80211_IS_MULTICAST(eh->ether_dhost) &&
            !IEEE80211_IS_BROADCAST(eh->ether_dhost)){
            if (ntohs(eh->ether_type) == ETHERTYPE_IP) {
                const struct ip_header *ip = (struct ip_header *)
                    (wbuf_header(wbuf) + sizeof (struct ether_header));
                    src_ip_addr = ntohl(ip->saddr);
                    grp_addr = ntohl(ip->daddr);
            }
        }
    }

    /* if grp address is in denied list then don't send process it here*/
    if(ieee80211_me_SnoopIsDenied(vap, grp_addr))
    {
        return -1;
    }
    
    /* Get members of this group */
    /* save original frame's src addr once */
    IEEE80211_ADDR_COPY(srcmac, eh->ether_shost);
    IEEE80211_ADDR_COPY(grpmac, eh->ether_dhost);
    OS_MEMSET(empty_entry_mac, 0, IEEE80211_ADDR_LEN);
    dstmac = eh->ether_dhost;

    newmaccnt = ieee80211_me_SnoopListGetMember(vap, grpmac, src_ip_addr,newmac[0]);

    /* save original frame's src addr once */
    /*
     * If newmaccnt is 0: no member intrested in this group
     *                 1: mc in table, only one dest, skb cloning not needed
     *                >1: multiple mc in table, skb cloned for each entry past
     *                    first one.
     */

    /* Maitain the original wbuf avoid being modified.
     */
    if (newmaccnt > 0 && vap->iv_me->mc_mcast_enable == 0)
    {
        /* We have members intrested in this group and no enhancement is required, send true multicast */
        return -1;
    } else if(newmaccnt == 0) {
        /* no members is intrested, no need to send, just drop it */
            wbuf_complete(wbuf);
        return 1;
    }

    wbuf1 = wbuf_copy(wbuf);
    wbuf_complete(wbuf);
    wbuf = wbuf1;
    wbuf1 = NULL;

    /* loop start */
    do {    
        if (newmaccnt > 0) {    
            /* reference new dst mac addr */
            dstmac = &newmac[newmacidx][0];
            

            /*
             * Note - cloned here pre-tunnel due to the way ieee80211_classify()
             * currently works. This is not efficient.  Better to split 
             * ieee80211_classify() functionality into per node and per frame,
             * then clone the post-tunnelled copy.
             * Currently, the priority must be determined based on original frame,
             * before tunnelling.
             */
            if (newmaccnt > 1) {
                wbuf1 = wbuf_copy(wbuf);
                if(wbuf1 != NULL) {
                    wbuf_set_node(wbuf1, ni);    
                    wbuf_clear_flags(wbuf1);
                }
            }
        } else {
            goto bad;
        }
        
        /* In case of loop */
        if(IEEE80211_ADDR_EQ(dstmac, srcmac)) {
            goto bad;
        }
        
        /* In case the entry is an empty one, it indicates that
         * at least one STA joined the group and then left. For this
         * case, if mc_discard_mcast is enabled, this mcast frame will
         * be discarded to save the bandwidth for other ucast streaming 
         */
        if (IEEE80211_ADDR_EQ(dstmac, empty_entry_mac)) {
            if (newmaccnt > 1 || vap->iv_me->mc_discard_mcast) {   
                goto bad;
            } else {
                /*
                 * If empty entry AND not to discard the mcast frames,
                 * restore dstmac to the mcast address
                 */    
                newmaccnt = 0;
                dstmac = eh->ether_dhost;
            }
        }

        /* Look up destination */
        ni = ieee80211_find_txnode(vap, dstmac);
        /* Drop frame if dest not found in tx table */
        if (ni == NULL) {
            goto bad2;
        }

        /* Drop frame if dest sta not associated */
        if (ni->ni_associd == 0 && ni != vap->iv_bss) {
            /* the node hasn't been associated */

            if(ni != NULL) {
                ieee80211_free_node(ni);
            }
            
            if (newmaccnt > 0) {
                ieee80211_me_SnoopWDSNodeCleanup(ni);
            }
            goto bad;
        }

        /* calculate priority so drivers can find the tx queue */
        if (ieee80211_classify(ni, wbuf)) {
            IEEE80211_NOTE(vap, IEEE80211_MSG_IQUE, ni,
                "%s: discard, classification failure", __func__);

            goto bad2;
        }

        /* Insert tunnel header
         * eh is the pointer to the ethernet header of the original frame.
         * eh2 is the pointer to the ethernet header of the encapsulated frame.
         *
         */
        if (newmaccnt > 0 /*&& vap->iv_me->mc_mcast_enable*/) {
            /*Option 1: Tunneling*/
            if (vap->iv_me->mc_mcast_enable & 1) {
                /* Encapsulation */
                tunHdr = (struct athl2p_tunnel_hdr *) wbuf_push(wbuf, sizeof(struct athl2p_tunnel_hdr));
                eh2 = (struct ether_header *) wbuf_push(wbuf, sizeof(struct ether_header));
        
                /* ATH_ETH_TYPE protocol subtype */
                tunHdr->proto = 17;
            
                /* copy original src addr */
                IEEE80211_ADDR_COPY(&eh2->ether_shost[0], srcmac);
    
                /* copy new ethertype */
                eh2->ether_type = htons(ATH_ETH_TYPE);

            } else if (vap->iv_me->mc_mcast_enable & 2) {/*Option 2: Translating*/
               eh2 = (struct ether_header *)wbuf_header(wbuf);
            } else {/* no tunnel and no-translate, just multicast */
                eh2 = (struct ether_header *)wbuf_header(wbuf);
            }

            /* copy new dest addr */
            IEEE80211_ADDR_COPY(&eh2->ether_dhost[0], &newmac[newmacidx][0]);

            /*
             *  Headline block removal: if the state machine is in
             *  BLOCKING or PROBING state, transmision of UDP data frames
             *  are blocked untill swtiches back to ACTIVE state.
             */
            if (vap->iv_ique_ops.hbr_dropblocked) {
                if (vap->iv_ique_ops.hbr_dropblocked(vap, ni, wbuf)) {
                    IEEE80211_DPRINTF(vap, IEEE80211_MSG_IQUE,
                                     "%s: packet dropped coz it blocks the headline\n",
                                     __func__);
                    goto bad2;
                }
            }
        }
        if (!ieee80211_vap_ready_is_set(vap)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_OUTPUT,
                              "%s: ignore data packet, vap is not active\n",
                              __func__);
            goto bad2;
        }
#ifdef IEEE80211_WDS
        if (vap->iv_opmode == IEEE80211_M_WDS)
            wbuf_set_wdsframe(wbuf);
        else
            wbuf_clear_wdsframe(wbuf);
#endif
        
        wbuf_set_node(wbuf, ni);
    
        /* power-save checks */
        if ((!WME_UAPSD_AC_ISDELIVERYENABLED(wbuf_get_priority(wbuf), ni)) && 
            (ieee80211node_is_paused(ni)) && 
            !ieee80211node_has_flag(ni, IEEE80211_NODE_TEMP)) {
            ieee80211node_pause(ni); 
            /* pause it to make sure that no one else unpaused it 
               after the node_is_paused check above, 
               pause operation is ref counted */
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_OUTPUT,
                    "%s: could not send packet, STA (%s) powersave %d paused %d\n", 
                    __func__, ether_sprintf(ni->ni_macaddr), 
                    (ni->ni_flags & IEEE80211_NODE_PWR_MGT) ?1 : 0, 
                    ieee80211node_is_paused(ni));
            ieee80211_node_saveq_queue(ni, wbuf, IEEE80211_FC0_TYPE_DATA);
            ieee80211node_unpause(ni); 
#if LMAC_SUPPORT_POWERSAVE_QUEUE
            ieee80211_vap_pause_update_xmit_stats(vap,wbuf); /* update the stats for vap pause module */
            ieee80211_send_wbuf(vap, ni, wbuf);
#endif
            /* unpause it if we are the last one, the frame will be flushed out */  
        }
        else
        {
            ieee80211_vap_pause_update_xmit_stats(vap,wbuf); /* update the stats for vap pause module */
            ieee80211_send_wbuf(vap, ni, wbuf);
        }

        /* ieee80211_send_wbuf will increase refcnt for frame to be sent, so decrease refcnt here for the increase by find_txnode. */
        ieee80211_free_node(ni); 
        goto loop_end;

    bad2:
        if (ni != NULL) {
            ieee80211_free_node(ni);
        } 
    bad:
        if (wbuf != NULL) {
            wbuf_complete(wbuf);
        }
         
        if (IEEE80211_IS_MULTICAST(dstmac))
            vap->iv_multicast_stats.ims_tx_discard++;
        else
            vap->iv_unicast_stats.ims_tx_discard++;
        
    
    loop_end:
        /* loop end */
        if (wbuf1 != NULL) {
            wbuf = wbuf1;
        }
        wbuf1 = NULL;
        newmacidx++;
        xmited ++;
        if(newmaccnt)
            newmaccnt--;
    } while (newmaccnt > 0 && vap->iv_me->mc_snoop_enable); 
    return xmited;
}

/**************************************************************************
 * !
 * \brief Initialize the mc snooping and encapsulation feature.
 *
 * \param vap
 *
 * \return N/A
 */
static void ieee80211_me_SnoopListInit(struct ieee80211vap *vap)
{
#if ATH_PERF_PWR_OFFLOAD
    struct ol_ath_softc_net80211 *scn = OL_ATH_SOFTC_NET80211(vap->iv_ic);

    atomic_set(&vap->iv_me->ieee80211_me_snooplist.msl_group_member_limit, scn->wlan_resource_config.num_mcast_table_elems+1);
    atomic_set(&vap->iv_me->ieee80211_me_snooplist.msl_group_list_limit, scn->wlan_resource_config.num_mcast_groups+1);
#endif
    vap->iv_me->ieee80211_me_snooplist.msl_group_list_count = 0;
    vap->iv_me->ieee80211_me_snooplist.msl_misc = 0;
    IEEE80211_SNOOP_LOCK_INIT(&vap->iv_me->ieee80211_me_snooplist, "Snoop Table");
    TAILQ_INIT(&vap->iv_me->ieee80211_me_snooplist.msl_node);
}

/********************************************************************
 * !
 * \brief Detach the resources for multicast enhancement
 *
 * \param sc Pointer to ATH object (this)
 *
 * \return N/A
 */
static void
ieee80211_me_detach(struct ieee80211vap *vap)
{
    if(vap->iv_me) {
        ieee80211_me_clean_snp_list(vap);
        IEEE80211_ME_LOCK(vap);
        OS_CANCEL_TIMER(&vap->iv_me->snooplist_timer);    
        OS_FREE_TIMER(&vap->iv_me->snooplist_timer);    
        IEEE80211_ME_UNLOCK(vap);
        IEEE80211_ME_LOCK_DESTROY(vap);
        OS_FREE(vap->iv_me);
        vap->iv_me = NULL;
    }
}

#if ATH_SUPPORT_LINKDIAG_EXT
static struct ieee80211_me_hifi_entry *ieee80211_me_hifi_find_entry(
        struct ieee80211_me_hifi_group *group, 
        struct ieee80211_me_hifi_table *table)
{
    int cnt;

    if (!group || !table)
        return NULL;

    for (cnt = 0; cnt < table->entry_cnt; cnt++) {
        if (!OS_MEMCMP(&table->entry[cnt].group, group, sizeof(*group)))
            break;
    }
    if (cnt == table->entry_cnt)
        return NULL;

    return &table->entry[cnt];
}

int ieee80211_me_hifi_filter(struct ieee80211_me_hifi_node *node, const void *ip_header, u_int16_t pro)
{
    int i;

    if (!node->filter_mode || (node->filter_mode == IEEE80211_ME_HIFI_EXCLUDE && !node->nsrcs))
        return 0;

    if (node->filter_mode == IEEE80211_ME_HIFI_INCLUDE && !node->nsrcs)
        return 1;

    if (pro == htobe16(ETHERTYPE_IP)) {
        u_int32_t ip4 = ((struct ip_header *)ip_header)->saddr;
        const u_int32_t *srcs = (u_int32_t *)node->srcs;
        for (i = 0; i < node->nsrcs; i++) {
            if (srcs[i] == ip4)
                break;
        }
    } else if (pro == htobe16(ETHERTYPE_IPV6)) {
        adf_net_ipv6_addr_t *ip6 = &((adf_net_ipv6hdr_t *)ip_header)->ipv6_saddr;
        adf_net_ipv6_addr_t *srcs = (adf_net_ipv6_addr_t *)node->srcs;
        for (i = 0; i < node->nsrcs; i++) {
            if (!OS_MEMCMP(&srcs[i], ip6, sizeof(*srcs)))
                break;
        }
    } else {
        return 0;
    }

    return ((node->filter_mode == IEEE80211_ME_HIFI_INCLUDE && i == node->nsrcs) ||
        (node->filter_mode == IEEE80211_ME_HIFI_EXCLUDE && i != node->nsrcs));
}

static void ieee80211_me_hifi_forward(struct ieee80211vap *vap, 
        wbuf_t wbuf, struct ieee80211_node *ni)
{
    if (ieee80211_classify(ni, wbuf)) {
        IEEE80211_NOTE(vap, IEEE80211_MSG_IQUE, ni, 
                "%s: discard, classification failure", __func__);
        wbuf_complete(wbuf);
        goto out;
    }

    if (vap->iv_ique_ops.hbr_dropblocked) {
        if (vap->iv_ique_ops.hbr_dropblocked(vap, ni, wbuf)) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_IQUE, 
                    "%s: packet dropped coz it blocks the headline\n", __func__);
            goto out;
        }
    }

    if(!(ni->ni_flags & IEEE80211_NODE_WDS)) {
        struct ether_header *eh = (struct ether_header *) wbuf_header(wbuf);
        IEEE80211_ADDR_COPY(eh->ether_dhost, ni->ni_macaddr);
    }

#ifdef IEEE80211_WDS
        if (vap->iv_opmode == IEEE80211_M_WDS)
            wbuf_set_wdsframe(wbuf);
        else
            wbuf_clear_wdsframe(wbuf);
#endif
       
    wbuf_set_node(wbuf, ni);
    wbuf_set_complete_handler(wbuf, NULL, NULL);
    ieee80211_vap_pause_update_xmit_stats(vap, wbuf);
    ieee80211_send_wbuf(vap, ni, wbuf);
out:
    ieee80211_free_node(ni); 
}

int ieee80211_me_hifi_convert(struct ieee80211vap *vap, wbuf_t wbuf)
{
    int n;
    rwlock_state_t lock_state;
    struct ether_header *eh = NULL;
    u_int8_t zero_mac[IEEE80211_ADDR_LEN];
    struct ieee80211_node *ni = NULL, *prev = NULL;
    struct ieee80211_me_hifi_group group;
    struct ieee80211_me_hifi_entry *entry = NULL;
    struct ieee80211_me_hifi_table *table = vap->iv_me->me_hifi_table;

    if (!vap->iv_me->me_hifi_enable ||
            !ieee80211_vap_ready_is_set(vap))
        return -1;

    eh = (struct ether_header *) wbuf_header(wbuf);
    switch (ntohs(eh->ether_type)) {
        case ETHERTYPE_IP: 
            {
            struct ip_header *iph = (struct ip_header *)(eh + 1);

            if (iph->protocol == IPPROTO_IGMP)
                return -1;

            OS_MEMSET(&group, 0, sizeof group);
            group.u.ip4 = ntohl(iph->daddr);
            group.pro = ETHERTYPE_IP;
            }
            break;
        case ETHERTYPE_IPV6:
            {
            adf_net_ipv6hdr_t *ip6h = (adf_net_ipv6hdr_t *)(eh + 1); 
            u_int8_t *nexthdr = (u_int8_t *)(ip6h + 1);

            if (ip6h->ipv6_nexthdr == IPPROTO_ICMPV6 ||
                    (ip6h->ipv6_nexthdr == IPPROTO_HOPOPTS &&
                     *nexthdr == IPPROTO_ICMPV6))
                return -1;

            OS_MEMSET(&group, 0, sizeof group);
            OS_MEMCPY(group.u.ip6, 
                    ip6h->ipv6_daddr.s6_addr,
                    sizeof(adf_net_ipv6_addr_t));
            group.pro = ETHERTYPE_IPV6;
            }
            break;
        default:
            return -1;
    }

    if (!table || !(entry = ieee80211_me_hifi_find_entry(&group, table)) ||
            !entry->node_cnt || !(wbuf = adf_nbuf_unshare(wbuf)))
        return -1;

    OS_MEMSET(zero_mac, 0, IEEE80211_ADDR_LEN);
    OS_RWLOCK_READ_LOCK(&vap->iv_me->me_hifi_lock, &lock_state);
    for (n = 0; n < entry->node_cnt; n++) {
        if (IEEE80211_ADDR_EQ(eh->ether_shost, entry->nodes[n].mac) ||
                IEEE80211_ADDR_EQ(zero_mac, entry->nodes[n].mac) ||
                ieee80211_me_hifi_filter(&entry->nodes[n], eh + 1, ntohs(eh->ether_type)) ||
                !(ni = ieee80211_find_txnode(vap, entry->nodes[n].mac)))
            continue;

        if (!ni->ni_associd || ni == vap->iv_bss) {
            ieee80211_free_node(ni);
            continue;
        }

        if (prev != NULL) {
            wbuf_t wbuf2 = wbuf_copy(wbuf);

            if (!wbuf2) break;

            wbuf_clear_flags(wbuf2);
            ieee80211_me_hifi_forward(vap, wbuf2, prev);    
        }
        prev = ni;
    }
    OS_RWLOCK_READ_UNLOCK(&vap->iv_me->me_hifi_lock, &lock_state);

    if (prev) {
        ieee80211_me_hifi_forward(vap, wbuf, prev);
        return 0;
    }

    return -1;
}
#endif

/*************************************************************************
 * !
 * \brief Function of the timer to update the snoop list 
 *
 * \param unsigned long arg been casted to pointer to vap
 *
 * \return N/A
 */
OS_TIMER_FUNC(ieee80211_me_SnoopListTimer)
{
    struct ieee80211vap *vap;
    struct MC_SNOOP_LIST *snp_list;    
    struct MC_GROUP_LIST_ENTRY* curr_grp_list;
    struct MC_GROUP_LIST_ENTRY* next_grp_list;
    
    rwlock_state_t lock_state;

    systime_t timestamp  = OS_GET_TIMESTAMP();
    u_int32_t now        = (u_int32_t) CONVERT_SYSTEM_TIME_TO_MS(timestamp);
    

    OS_GET_TIMER_ARG(vap, struct ieee80211vap *);
    /*If vap ptr is null or vap is getting deleted do not arm timer */
    if ((!vap) || (ieee80211_vap_deleted_is_set(vap))) {
       return;
    }
    snp_list = (struct MC_SNOOP_LIST *)&(vap->iv_me->ieee80211_me_snooplist);    
    IEEE80211_SNOOP_LOCK(snp_list,&lock_state);
    
    curr_grp_list = TAILQ_FIRST(&snp_list->msl_node);    
    while(curr_grp_list != NULL){
        next_grp_list = TAILQ_NEXT(curr_grp_list,grp_list);
        ieee80211_me_remove_expired_member(curr_grp_list,vap,now);
        if(!TAILQ_FIRST(&curr_grp_list->src_list)){
            TAILQ_REMOVE(&snp_list->msl_node,curr_grp_list,grp_list);
            OS_FREE(curr_grp_list);
        }        
        curr_grp_list = next_grp_list;
    }
    
    IEEE80211_SNOOP_UNLOCK(snp_list, &lock_state);
    OS_SET_TIMER(&vap->iv_me->snooplist_timer, vap->iv_me->me_timer);
    return;
}

/*******************************************************************
 * !
 * \brief Attach the ath_me module for multicast enhancement. 
 *
 * This function allocates the ath_me structure and attachs function 
 * entry points to the function table of ath_softc.
 *
 * \param  vap
 *
 * \return pointer to the mcastenhance op table (vap->iv_me_ops).
 */
int
ieee80211_me_attach(struct ieee80211vap * vap)
{
    struct ieee80211_ique_me *ame;

    ame = (struct ieee80211_ique_me *)OS_MALLOC(
                    vap-> iv_ic->ic_osdev,       
                    sizeof(struct ieee80211_ique_me), GFP_KERNEL);

    if (ame == NULL)
            return -ENOMEM;

    OS_MEMZERO(ame, sizeof(struct ieee80211_ique_me));

    /*Attach function entry points*/
    vap->iv_ique_ops.me_detach = ieee80211_me_detach;
    vap->iv_ique_ops.me_inspect = ieee80211_me_SnoopInspecting;
    vap->iv_ique_ops.me_convert = ieee80211_me_SnoopConvert;
    vap->iv_ique_ops.me_dump = ieee80211_me_SnoopListDump;
    vap->iv_ique_ops.me_clean = ieee80211_me_SnoopWDSNodeCleanup;
    vap->iv_ique_ops.me_showdeny = ieee80211_me_SnoopShowDenyTable;
    vap->iv_ique_ops.me_adddeny = ieee80211_me_SnoopAddDenyEntry;
    vap->iv_ique_ops.me_cleardeny = ieee80211_me_SnoopClearDenyTable;
#if ATH_SUPPORT_LINKDIAG_EXT
    vap->iv_ique_ops.me_hifi_convert = ieee80211_me_hifi_convert;
#endif
    vap->iv_me = ame;
    ame->me_iv = vap;
    ame->ieee80211_me_snooplist.msl_max_length = MAX_SNOOP_ENTRIES;

    OS_INIT_TIMER(vap->iv_ic->ic_osdev, &ame->snooplist_timer, ieee80211_me_SnoopListTimer, vap);
    ame->me_timer = DEF_ME_TIMER;
    ame->me_timeout = DEF_ME_TIMEOUT;
    OS_SET_TIMER(&ame->snooplist_timer, vap->iv_me->me_timer);
    ame->mc_discard_mcast = 1;
    ame->ieee80211_me_snooplist.msl_deny_count = 3;

    ame->ieee80211_me_snooplist.msl_deny_group[0] = 3758096385UL; /*224.0.0.1 */
    ame->ieee80211_me_snooplist.msl_deny_mask[0] = 4294967040UL;   /* 255.255.255.0*/

    ame->ieee80211_me_snooplist.msl_deny_group[1] = 4026531585UL;     /*239.255.255.1*/
    ame->ieee80211_me_snooplist.msl_deny_mask[1] = 4294967040UL;   /* 255.255.255.0*/
    
    /* IP address 224.0.0.22 should not filtered as IGMPv3*/ 
    ame->ieee80211_me_snooplist.msl_deny_group[2] = 3758096406UL;  /*224.0.0.22*/
    ame->ieee80211_me_snooplist.msl_deny_mask[2] = 4294967040UL;   /* 255.255.255.0*/
    
    IEEE80211_ME_LOCK_INIT(vap);
    ieee80211_me_SnoopListInit(vap);

#if ATH_SUPPORT_LINKDIAG_EXT
    ame->me_hifi_enable = 1;
    OS_RWLOCK_INIT(&ame->me_hifi_lock);
#endif

    return 0;
}

#endif /* ATH_SUPPORT_IQUE */

