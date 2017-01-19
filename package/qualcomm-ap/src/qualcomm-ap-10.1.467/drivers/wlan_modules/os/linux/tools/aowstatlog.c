/*
 * Copyright (c) 2013 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */


/*
 * =====================================================================================
 *
 *       Filename:  aowstatlog.c
 *
 *    Description:  Application to get AoW stats
 *
 *        Version:  1.0
 *        Created:  Monday, June 28 2010
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Krishna C. Rao
 *        Company:  Atheros Communications 
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/netlink.h>

#include "if_athioctl.h"
#include "aowstatlog.h"
#include "ieee80211_aow_shared.h"

/* Please note that the below code was inherited from athplay.c, and follows a
   somewhat similar style as of now. */

/* Static functions */
static void usage(void);
static int aow_get_recvr_code(struct pli_log_info *pl_info,
                              struct ether_addr *macaddr);
static int process_aow_nl_packet(struct nlmsghdr * nlh,
                                 int bytes_read,
                                 struct pli_log_info *pl_info,
                                 int index);
static int timeval_subtract(struct timeval *result,
                            struct timeval *tv1,
                            struct timeval *tv2);

static int aow_netlink_read(struct pli_log_info *pl_info, int duration);

static int get_aow_plilogs(int duration, char* file_name);
static int pl_element_compr(const void *obj1, const void *obj2);
static void sort_txpli_info(struct pl_element *pl_stats, int numentries);
static int pl_element_isequal(struct pl_element elmnt1,
                              struct pl_element elmnt2);
static void partial_copy_pl_element(struct pl_element *destn,
                                    struct pl_element *src);
static void consolidate_txpli_info(struct pl_element *pl_stats,
                                int numentries,
                                int *new_numentries);
static int write_aow_logs_tx(struct pli_log_info *pl_info, FILE *fp);
static int write_aow_logs_rx(struct pli_log_info *pl_info, FILE *fp);

static void usage(void)
{
    fprintf(stderr,"usage : aowstatlog [cmd] <params>\n"
                   "aowstatlog pli  <duration in seconds> [destn_file_name]\n");
}    

/* 
 * If the address macaddr has been seen before, return code for it.
 * Else, if there is space, create a new code for it and return the same.
 * If there is no space, return an error.
 */
static int aow_get_recvr_code(struct pli_log_info *pl_info,
                              struct ether_addr *macaddr)
{
    int i;

    for (i = 0; i < pl_info->num_rcvrs; i++)
    {
       if(memcmp(macaddr,
                 &pl_info->rcvr_mac_addresses[i],
                 sizeof(struct ether_addr)) == 0) {
           break;
       }
    }

    if (i < pl_info->num_rcvrs) {
        return i;
    }

    if (i < RCVR_ADDR_STORE_SIZE){
        memcpy(&pl_info->rcvr_mac_addresses[i],
               macaddr,
               sizeof(struct ether_addr));
        pl_info->num_rcvrs++;
        return i;
    } else {
        return -1;
    }
}

/*
 * Process a packet received over Netlink. Decipher if it is a Tx or Rx log
 * packet, and store into the local array of Packet Lost Indication Records.
 */  
static int process_aow_nl_packet(struct nlmsghdr * nlh,
                                 int bytes_read,
                                 struct pli_log_info *pl_info,
                                 int index)
{
    struct aow_nl_packet *nlpkt;
    int recvr_code;
    int payload_len;
    
    nlpkt = (struct aow_nl_packet *)NLMSG_DATA(nlh);
    payload_len = NLMSG_PAYLOAD(nlh, bytes_read);

    if ((nlpkt->header & ATH_AOW_NL_TYPE_MASK) == ATH_AOW_NODE_TYPE_TX)
    {
        if (payload_len < AOW_NL_PACKET_TX_SIZE) {
            return -1;
        }

        pl_info->aow_node_type = ATH_AOW_NODE_TYPE_TX;
       
        if ((recvr_code =
                    aow_get_recvr_code(pl_info, &nlpkt->body.txdata.recvr)) < 0)
        {
            return -1;
        }

        /* Set a code here to reduce storage and increase sorting speed */
        nlpkt->body.txdata.elmnt.info |=
           (recvr_code & ATH_AOW_PL_RECVR_CODE_MASK) << ATH_AOW_PL_RECVR_CODE_S;

        memcpy(&pl_info->plstats[index],
               &nlpkt->body.txdata.elmnt,
               sizeof(struct pl_element));

        return 0;

    } else if ((nlpkt->header & ATH_AOW_NL_TYPE_MASK) == ATH_AOW_NODE_TYPE_RX) {
        
        pl_info->aow_node_type = ATH_AOW_NODE_TYPE_RX;
        
        if (payload_len < AOW_NL_PACKET_RX_SIZE) {
            return -1;
        }
        
        memcpy(&pl_info->plstats[index],
               &nlpkt->body.rxdata.elmnt,
               sizeof(struct pl_element));

        return 0;
    } else {
        return -1;
    }

} 

/* 
 *  Subtract the struct timeval value 'tv2' from 'tv1', storing the
 *  result in 'result'.
 *  Return values:
 *     1 if the difference is negative
 *     0 otherwise 
 */
static int timeval_subtract(struct timeval *result,
                            struct timeval *tv1,
                            struct timeval *tv2)
{
    int temp;

    /* Taken from GNU libc manual. It gives the best way
       to subtract two timeval structures, considering various
       value combination possibilities. */

    if (tv1->tv_usec < tv2->tv_usec) {
        temp = (tv2->tv_usec - tv1->tv_usec)/NUM_MICROSEC_PER_SEC + 1;
        tv2->tv_usec -= NUM_MICROSEC_PER_SEC * temp;
        tv2->tv_sec += temp;
    }
       
    if (tv1->tv_usec - tv2->tv_usec > NUM_MICROSEC_PER_SEC) { 
        temp = (tv1->tv_usec - tv2->tv_usec)/NUM_MICROSEC_PER_SEC; 
        tv2->tv_usec += NUM_MICROSEC_PER_SEC * temp;
        tv2->tv_sec -= temp;
    }
     
    result->tv_sec = tv1->tv_sec - tv2->tv_sec;
    result->tv_usec = tv1->tv_usec - tv2->tv_usec;
     
    return tv1->tv_sec < tv2->tv_sec;
}
 
static int aow_netlink_read(struct pli_log_info *pl_info, int duration)
{
    int retval, bytes_read;
    struct sockaddr_nl src_addr;
    struct nlmsghdr *nlh = NULL;
    int sock_fd;
    char buf[NLMSG_SPACE(sizeof(struct aow_nl_packet))]; 
    fd_set read_fds; 
    struct timeval timeout, start_time, end_time, curr_time;
    unsigned int index = 0;

    nlh = (struct nlmsghdr *)buf;
    
    sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ATHEROS_AOW_ES);
    
    if (sock_fd < 0) {
        err(1, "socket");
        return -1;
    }

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = PF_NETLINK;
    src_addr.nl_pid = getpid();  /* self pid */
    src_addr.nl_groups = 1;
    
    if(retval = bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) < 0) {
        err(1, "bind");
        return retval;
    }
    
    FD_ZERO(&read_fds);
    FD_SET(sock_fd, &read_fds);

    pl_info->plstats_recvd = 0;

    gettimeofday(&start_time, NULL);
    end_time.tv_sec = start_time.tv_sec + duration;
    end_time.tv_usec = start_time.tv_usec;


    while (index < pl_info->plstats_size)
    {
        /* If we rely on the Linux behaviour of returning time remaining in timeout,
           we will end up with additive drift */
        gettimeofday(&curr_time, NULL);
        if ((retval = timeval_subtract(&timeout, &end_time, &curr_time)) != 0) {
           break;
        } 

        if ((retval = select(sock_fd + 1, &read_fds, NULL, NULL, &timeout)) == -1) {
            err(1, "select");
            return -1;
        }    
        
        if (retval == 0) {
            //timeout
            break;
        }

        if (FD_ISSET(sock_fd, &read_fds)) {
            bytes_read = recvfrom(sock_fd,
                                  buf,
                                  sizeof(buf),
                                  MSG_WAITALL,
                                  NULL,
                                  NULL);

            if (bytes_read < 0) {
                err(1, "recvfrom");
                return retval;
            }
            
            if ((retval = process_aow_nl_packet(nlh,
                                                bytes_read,
                                                pl_info,
                                                index)) == 0) { 
                index++;
            }
        }
    }

    pl_info->plstats_recvd = index;

    close(sock_fd);
}

/*
 * Get AoW Packet Lost Indication (PLI) logs
 */
static int get_aow_plilogs(int duration, char* file_name)
{
    int retval;
    FILE* fp;
    struct pli_log_info pl_info;

    if (duration > MAX_PLILOG_DURATION)
    {
        printf("Maximum permitted PLI logging duration: %d seconds\n",
                MAX_PLILOG_DURATION);
        return -1;
    }

    memset(&pl_info, 0, sizeof(struct pli_log_info));

    if (file_name == NULL)
    {
        fp = stdout;
    }
    else
    {
        fp = fopen(file_name, "w");
        if (NULL == fp)
        {
            err(1, "fopen");
            return -1;
        }
    }

   /* We over-provision a bit.
      But under certain circumstances, this space can fall slightly short.
      (E.g too many entries for the same MPDU).
      We bail out by stopping the logging if pl_info.plstats_size is exceeded. */
   pl_info.plstats_size = ((duration * NUM_MILLISEC_PER_SEC)/
                                ATH_AOW_TIME_PER_MPDU) * \
                          AOW_MAX_RECVRS * \
                          PLSTATSIZE_BUFF_FACTOR;

   pl_info.plstats = (struct pl_element*)
                malloc(sizeof(struct pl_element) * pl_info.plstats_size);

   if (NULL == pl_info.plstats)
   {
        err(1, "malloc");
        return -1;
   }

   aow_netlink_read(&pl_info, duration);

   if (pl_info.aow_node_type == ATH_AOW_NODE_TYPE_TX) {
       retval = write_aow_logs_tx(&pl_info, fp);
   } else if (pl_info.aow_node_type == ATH_AOW_NODE_TYPE_RX) {
       retval = write_aow_logs_rx(&pl_info, fp);
   }

   free(pl_info.plstats);

   if (0 == retval) {
        printf("Successfully wrote Packet Lost Indication records\n");
   }

   return retval;
}

/* 
 * pl_element comparator function for use by standard qsort
 */
static int pl_element_compr(const void *obj1, const void *obj2)
{
    struct pl_element *elmnt1 = (struct pl_element*)obj1;
    struct pl_element *elmnt2 = (struct pl_element*)obj2;

    if (((elmnt2->info >> ATH_AOW_PL_RECVR_CODE_S)
                & ATH_AOW_PL_RECVR_CODE_MASK) >
        ((elmnt1->info >> ATH_AOW_PL_RECVR_CODE_S)
                & ATH_AOW_PL_RECVR_CODE_MASK)) {
        return OBJ2_GT_OBJ1; 
    }
    
    if (((elmnt1->info >> ATH_AOW_PL_RECVR_CODE_S)
                & ATH_AOW_PL_RECVR_CODE_MASK) >
        ((elmnt2->info >> ATH_AOW_PL_RECVR_CODE_S)
                & ATH_AOW_PL_RECVR_CODE_MASK)) {
        return OBJ1_GT_OBJ2; 
    }

    /* Receiver is the same, at this point */

    if (elmnt2->seqno > elmnt1->seqno) {
        return OBJ2_GT_OBJ1; 
    }

    if (elmnt1->seqno > elmnt2->seqno) {
        return OBJ1_GT_OBJ2; 
    }

    /* Audio frame nos are the same, at this point */

    if ((elmnt2->subfrme_wlan_seqnos & ATH_AOW_PL_SUBFRME_SEQ_MASK) >
        (elmnt1->subfrme_wlan_seqnos & ATH_AOW_PL_SUBFRME_SEQ_MASK)) {
        return OBJ2_GT_OBJ1; 
    }
    
    if ((elmnt1->subfrme_wlan_seqnos & ATH_AOW_PL_SUBFRME_SEQ_MASK) >
        (elmnt2->subfrme_wlan_seqnos & ATH_AOW_PL_SUBFRME_SEQ_MASK)) {
        return OBJ1_GT_OBJ2; 
    }

    /* Audio sub-frame nos are the same, at this point */
   
    return OBJ1_EQ_OBJ2;
}

static void sort_txpli_info(struct pl_element *pl_stats, int numentries)
{
    qsort( pl_stats, numentries, sizeof(struct pl_element),pl_element_compr);
}

static int pl_element_isequal(struct pl_element elmnt1,
                              struct pl_element elmnt2)
{
    if (((elmnt2.info >> ATH_AOW_PL_RECVR_CODE_S)
                & ATH_AOW_PL_RECVR_CODE_MASK) !=
        ((elmnt1.info >> ATH_AOW_PL_RECVR_CODE_S)
                & ATH_AOW_PL_RECVR_CODE_MASK)) {
        return 0; 
    }
 
    if (elmnt2.seqno != elmnt1.seqno) {
        return 0; 
    }

    if (elmnt2.subfrme_wlan_seqnos != elmnt1.subfrme_wlan_seqnos) {
        return 0;
    }
    
    if (((elmnt2.info >> ATH_AOW_PL_TX_STATUS_S)
                & ATH_AOW_PL_TX_STATUS_MASK) !=
        ((elmnt1.info >> ATH_AOW_PL_TX_STATUS_S)
                & ATH_AOW_PL_TX_STATUS_MASK)) {
        return 0;
    }
    
    return 1;
}

/*
 * Copy only the sequence numbers, receiver code, and Tx status. Do not 
 * copy the number of transmission attempts.
 */
static void partial_copy_pl_element(struct pl_element *destn,
                                    struct pl_element *src)
{
    destn->seqno = src->seqno;
    destn->subfrme_wlan_seqnos = src->subfrme_wlan_seqnos;
    destn->info =
        src->info &
            ((ATH_AOW_PL_TX_STATUS_MASK << ATH_AOW_PL_TX_STATUS_S) |
             (ATH_AOW_PL_RECVR_CODE_MASK << ATH_AOW_PL_RECVR_CODE_S));
}

/*
 * Eliminate duplicate consecutive pl_element entries by adding up their 
 * 'Number of attempts' field.
 * pl_stats is assumed to be sorted.
 * Note that two entries whose transmission status are different are not
 * considered duplicate.
 */
static void consolidate_txpli_info(struct pl_element *pl_stats,
                                int numentries,
                                int *new_numentries)
{
    int i = 0, j = 0, total_attempts = 0, lindex = 0;

    struct pl_element *lpl_stats =
        (struct pl_element *)malloc(sizeof(struct pl_element) * numentries);

    while (i < numentries)
    {
        j = i;
        total_attempts = pl_stats[i].info & ATH_AOW_PL_NUM_ATTMPTS_MASK; 

        while ((j < numentries - 1) && pl_element_isequal(pl_stats[j], pl_stats[j + 1])) {
            total_attempts +=
                (pl_stats[j + 1].info & ATH_AOW_PL_NUM_ATTMPTS_MASK);
            j++;
        }
        
        partial_copy_pl_element(&lpl_stats[lindex], &pl_stats[i]);
        lpl_stats[lindex++].info |=
            (total_attempts & ATH_AOW_PL_NUM_ATTMPTS_MASK); 

        i = j + 1;
        
    }  
    
    memcpy(pl_stats, lpl_stats, sizeof(struct pl_element) * lindex);
    if (numentries - lindex) {
        memset(pl_stats + lindex, 0, sizeof(struct pl_element) * (numentries - lindex));
    }
    
    *new_numentries = lindex;

    free(lpl_stats);
}

#define HANDLE_FPRINTF_ERR(retval)  if ((retval) < 0) \
                                    {\
                                      err(1, "fprintf"); \
                                      return -1; \
                                    }

/* 
 * CSV format logging of Tx side log Information.
 */
static int write_aow_logs_tx(struct pli_log_info *pl_info, FILE *fp)
{
    int retval = 0;
    int i;
    int new_count;
    
    /* Packet Lost Indication */

#if ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG == 0
    sort_txpli_info(pl_info->plstats, pl_info->plstats_recvd);
    consolidate_txpli_info(pl_info->plstats,
                           pl_info->plstats_recvd,
                           &new_count);
#else
    new_count = pl_info->plstats_recvd;
#endif

    retval = fprintf(fp, "Packet Status (Tx)\n\n");
    HANDLE_FPRINTF_ERR(retval);
    
    retval = fprintf(fp, "STA Num,MAC Address\n");
    HANDLE_FPRINTF_ERR(retval);

    for (i = 0; i < pl_info->num_rcvrs; i++)
    {
        retval = fprintf(fp, "%d,%s\n",
                             i,
                             ether_ntoa(&pl_info->rcvr_mac_addresses[i])); 
        HANDLE_FPRINTF_ERR(retval);
    }

    retval = fprintf(fp, "\nStatus Code,Meaning\n"
                         "S,Success\n"
                         "F,Failure\n");
    HANDLE_FPRINTF_ERR(retval);
    
    retval = fprintf(fp, "\nSTA Num,Audio Frame,Audio Sub-Frame,WLAN Seq No,Status Code,Attempts");
    HANDLE_FPRINTF_ERR(retval);

#if ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG
    retval = fprintf(fp, ",ts_tstamp,"
                         "ts_seqnum,"
                         "ts_status,"
                         "ts_ratecode,"
                         "ts_rateindex,"
                         "ts_rssi,"
                         "ts_shortretry,"
                         "ts_longretry,"
                         "ts_virtcol,"
                         "ts_antenna,"
                         "ts_flags,"
                         "ts_rssi_ctl0,"
                         "ts_rssi_ctl1,"
                         "ts_rssi_ctl2,"
                         "ts_rssi_ext0,"
                         "ts_rssi_ext1,"
                         "ts_rssi_ext2,"
                         "queue_id,"
                         "desc_id,"
                         "ba_low,"
                         "ba_high,"
                         "evm0,"
                         "evm1,"
                         "evm2,"
                         "ts_txbfstatus,"
                         "tid,"
                         "bf_avail_buf,"
                         "bf_status,"
                         "bf_flags,"
                         "bf_reftxpower,"
#if ATH_SUPPORT_IQUE && ATH_SUPPORT_IQUE_EXT
                         "bf_txduration,"
#endif
                         "bfs_nframes,"
                         "bfs_al,"
                         "bfs_frmlen,"
                         "bfs_seqno,"
                         "bfs_tidno,"
                         "bfs_retries,"
                         "bfs_useminrate,"
                         "bfs_ismcast,"
                         "bfs_isdata,"
                         "bfs_isaggr,"
                         "bfs_isampdu,"
                         "bfs_ht,"
                         "bfs_isretried,"
                         "bfs_isxretried,"
                         "bfs_shpreamble,"
                         "bfs_isbar,"
                         "bfs_ispspoll,"
                         "bfs_aggrburst,"
                         "bfs_calcairtime,"
#ifdef ATH_SUPPORT_UAPSD
                         "bfs_qosnulleosp,"
#endif
                         "bfs_ispaprd,"
                         "bfs_isswaborted,"
#if ATH_SUPPORT_CFEND
                         "bfs_iscfend,"
#endif
#ifdef ATH_SWRETRY
                         "bfs_isswretry,"
                         "bfs_swretries,"
                         "bfs_totaltries,"
#endif
                         "bfs_qnum,"
                         "bfs_rifsburst_elem,"
                         "bfs_nrifsubframes,"
                         "bfs_txbfstatus,"
                         "loopcount"       
        );
    HANDLE_FPRINTF_ERR(retval);
#endif
    
    retval = fprintf(fp, "\n");
    HANDLE_FPRINTF_ERR(retval);
    
    for (i = 0; i < new_count; i++) {
        retval = fprintf(fp,
                         "%u,%u,%hu,%hu,",
                         (pl_info->plstats[i].info >> ATH_AOW_PL_RECVR_CODE_S)
                             & ATH_AOW_PL_RECVR_CODE_MASK,
                         pl_info->plstats[i].seqno,
                         pl_info->plstats[i].subfrme_wlan_seqnos
                             & ATH_AOW_PL_SUBFRME_SEQ_MASK,
                         (pl_info->plstats[i].subfrme_wlan_seqnos
                              >> ATH_AOW_PL_WLAN_SEQ_S)
                             & ATH_AOW_PL_WLAN_SEQ_MASK);
        HANDLE_FPRINTF_ERR(retval);

        switch((pl_info->plstats[i].info >> ATH_AOW_PL_TX_STATUS_S)
                    & ATH_AOW_PL_TX_STATUS_MASK){
            case ATH_AOW_PL_INFO_TX_SUCCESS:
                 retval = fprintf(fp, "S,");
                 break;

            case ATH_AOW_PL_INFO_TX_FAIL:
                 retval = fprintf(fp, "F,");
                 break;
        }
        HANDLE_FPRINTF_ERR(retval);
        
        retval = fprintf(fp, "%hhu", pl_info->plstats[i].info
                                        & ATH_AOW_PL_NUM_ATTMPTS_MASK);
        HANDLE_FPRINTF_ERR(retval);
    
#if ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG
        retval = fprintf(fp,",%u,"
                            "%hu,"
                            "%hhu,"
                            "%hhu,"
                            "%hhu,"
                            "%hhd,"
                            "%hhu,"
                            "%hhu,"
                            "%hhu,"
                            "%hhu,"
                            "%hhu,"
                            "%hhd,"
                            "%hhd,"
                            "%hhd,"
                            "%hhd,"
                            "%hhd,"
                            "%hhd,"
                            "%hhu,"
                            "%hu,"
                            "%u,"
                            "%u,"
                            "%u,"
                            "%u,"
                            "%u,"
                            "%hhu,"
                            "%hhu,"
                            "%hu,"
                            "%u,"
                            "%hu,"
                            "%hu,"
#if ATH_SUPPORT_IQUE && ATH_SUPPORT_IQUE_EXT
                            "%u,"
#endif                     
                            "%d,"
                            "%hu,"
                            "%hu,"
                            "%d,"
                            "%d,"
                            "%d,"
                            "%hhu,"
                            "%hhu,"
                            "%hhu,"
                            "%hhu,"
                            "%hhu,"
                            "%hhu,"
                            "%hhu,"
                            "%hhu,"
                            "%hhu,"
                            "%hhu,"
                            "%hhu,"
                            "%hhu,"
                            "%hhu,"
#ifdef ATH_SUPPORT_UAPSD
                            "%hhu,"
#endif
                            "%hhu,"
                            "%hhu,"
#if ATH_SUPPORT_CFEND
                            "%hhu,"
#endif
#ifdef ATH_SWRETRY
                            "%hhu,"
                            "%d,"
                            "%d,"
#endif
                            "%d,"
                            "%d,"
                            "%d,"
                            "%hhu,"
                            "%d",
                            pl_info->plstats[i].atxinfo.ts_tstamp,
                            pl_info->plstats[i].atxinfo.ts_seqnum,
                            pl_info->plstats[i].atxinfo.ts_status,
                            pl_info->plstats[i].atxinfo.ts_ratecode,
                            pl_info->plstats[i].atxinfo.ts_rateindex,
                            pl_info->plstats[i].atxinfo.ts_rssi,
                            pl_info->plstats[i].atxinfo.ts_shortretry,
                            pl_info->plstats[i].atxinfo.ts_longretry,
                            pl_info->plstats[i].atxinfo.ts_virtcol,
                            pl_info->plstats[i].atxinfo.ts_antenna,
                            pl_info->plstats[i].atxinfo.ts_flags,
                            pl_info->plstats[i].atxinfo.ts_rssi_ctl0,
                            pl_info->plstats[i].atxinfo.ts_rssi_ctl1,
                            pl_info->plstats[i].atxinfo.ts_rssi_ctl2,
                            pl_info->plstats[i].atxinfo.ts_rssi_ext0,
                            pl_info->plstats[i].atxinfo.ts_rssi_ext1,
                            pl_info->plstats[i].atxinfo.ts_rssi_ext2,
                            pl_info->plstats[i].atxinfo.queue_id,
                            pl_info->plstats[i].atxinfo.desc_id,
                            pl_info->plstats[i].atxinfo.ba_low,
                            pl_info->plstats[i].atxinfo.ba_high,
                            pl_info->plstats[i].atxinfo.evm0,
                            pl_info->plstats[i].atxinfo.evm1,
                            pl_info->plstats[i].atxinfo.evm2,
                            pl_info->plstats[i].atxinfo.ts_txbfstatus,
                            pl_info->plstats[i].atxinfo.tid,
                            pl_info->plstats[i].atxinfo.bf_avail_buf,
                            pl_info->plstats[i].atxinfo.bf_status,
                            pl_info->plstats[i].atxinfo.bf_flags,
                            pl_info->plstats[i].atxinfo.bf_reftxpower,
#if ATH_SUPPORT_IQUE && ATH_SUPPORT_IQUE_EXT
                            pl_info->plstats[i].atxinfo.bf_txduration,
#endif
                            pl_info->plstats[i].atxinfo.bfs_nframes,
                            pl_info->plstats[i].atxinfo.bfs_al,
                            pl_info->plstats[i].atxinfo.bfs_frmlen,
                            pl_info->plstats[i].atxinfo.bfs_seqno,
                            pl_info->plstats[i].atxinfo.bfs_tidno,
                            pl_info->plstats[i].atxinfo.bfs_retries,
                            pl_info->plstats[i].atxinfo.bfs_useminrate,
                            pl_info->plstats[i].atxinfo.bfs_ismcast,
                            pl_info->plstats[i].atxinfo.bfs_isdata,
                            pl_info->plstats[i].atxinfo.bfs_isaggr,
                            pl_info->plstats[i].atxinfo.bfs_isampdu,
                            pl_info->plstats[i].atxinfo.bfs_ht,
                            pl_info->plstats[i].atxinfo.bfs_isretried,
                            pl_info->plstats[i].atxinfo.bfs_isxretried,
                            pl_info->plstats[i].atxinfo.bfs_shpreamble,
                            pl_info->plstats[i].atxinfo.bfs_isbar,
                            pl_info->plstats[i].atxinfo.bfs_ispspoll,
                            pl_info->plstats[i].atxinfo.bfs_aggrburst,
                            pl_info->plstats[i].atxinfo.bfs_calcairtime,
#ifdef ATH_SUPPORT_UAPSD
                            pl_info->plstats[i].atxinfo.bfs_qosnulleosp,
#endif
                            pl_info->plstats[i].atxinfo.bfs_ispaprd,
                            pl_info->plstats[i].atxinfo.bfs_isswaborted,
#if ATH_SUPPORT_CFEND
                            pl_info->plstats[i].atxinfo.bfs_iscfend,
#endif
#ifdef ATH_SWRETRY
                            pl_info->plstats[i].atxinfo.bfs_isswretry,
                            pl_info->plstats[i].atxinfo.bfs_swretries,
                            pl_info->plstats[i].atxinfo.bfs_totaltries,
#endif
                            pl_info->plstats[i].atxinfo.bfs_qnum,
                            pl_info->plstats[i].atxinfo.bfs_rifsburst_elem,
                            pl_info->plstats[i].atxinfo.bfs_nrifsubframes,
                            pl_info->plstats[i].atxinfo.bfs_txbfstatus,
                            pl_info->plstats[i].atxinfo.loopcount);
        HANDLE_FPRINTF_ERR(retval);
#endif
        retval = fprintf(fp, "\n");
        HANDLE_FPRINTF_ERR(retval);
    }

    return 0;
}

/* 
 * CSV format logging of Rx side log Information.
 */
static int write_aow_logs_rx(struct pli_log_info *pl_info, FILE *fp)
{
    int retval = 0;
    int i;

    /* Packet Lost Indication */
    
    retval = fprintf(fp, "Packet Status (Rx)\n\n");
    HANDLE_FPRINTF_ERR(retval);
    
    retval = fprintf(fp, "\nStatus Code,Meaning\n"
                         "I,In-Time\n"
                         "D,Delayed/Came Late\n");
    HANDLE_FPRINTF_ERR(retval);

    retval = fprintf(fp, "\nAudio Frame,Audio Sub-Frame,WLAN Seq No,Status Code");
    HANDLE_FPRINTF_ERR(retval);
    
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
    retval = fprintf(fp, ",Timestamp(TSF),Driver Receive Time(TSF),"
                         "AoW Module Receive Time(TSF),Tx processing + OTA Latency (usec),"
                         "Rx processing time till AoW (usec),AoW Reception Latency (usec)");
    HANDLE_FPRINTF_ERR(retval);
#endif
    
    retval = fprintf(fp, "\n");
    HANDLE_FPRINTF_ERR(retval);
    
    for (i = 0; i < pl_info->plstats_recvd; i++) {
        retval = fprintf(fp, "%u,%hu,%hu,",
                          pl_info->plstats[i].seqno,
                          pl_info->plstats[i].subfrme_wlan_seqnos
                              & ATH_AOW_PL_SUBFRME_SEQ_MASK,
                          (pl_info->plstats[i].subfrme_wlan_seqnos
                           >> ATH_AOW_PL_WLAN_SEQ_S)
                              & ATH_AOW_PL_WLAN_SEQ_MASK);
        HANDLE_FPRINTF_ERR(retval);
    

        switch(pl_info->plstats[i].info){
            case ATH_AOW_PL_INFO_INTIME:
                 retval = fprintf(fp, "I");
                 break;
            case ATH_AOW_PL_INFO_DELAYED:
                 retval = fprintf(fp, "D");
                 break;
            //case ATH_AOW_PL_INFO_MPDU_LOST:
            //     retval = fprintf(fp, "Lost\n");
            //     break;
            default:
                 retval = fprintf(fp, "<Unknown Code>");
                 break;
        }
        HANDLE_FPRINTF_ERR(retval);

#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
        retval = fprintf(fp, ",%llu,%llu,%llu,%llu,%llu,%llu",
                         pl_info->plstats[i].arxinfo.timestamp,
                         pl_info->plstats[i].arxinfo.rxTime,
                         pl_info->plstats[i].arxinfo.deliveryTime,
                         pl_info->plstats[i].arxinfo.rxTime - 
                            pl_info->plstats[i].arxinfo.timestamp,
                         pl_info->plstats[i].arxinfo.deliveryTime -
                            pl_info->plstats[i].arxinfo.rxTime,
                         pl_info->plstats[i].arxinfo.deliveryTime -
                            pl_info->plstats[i].arxinfo.timestamp
                         );
        HANDLE_FPRINTF_ERR(retval);
#endif
        
        retval = fprintf(fp, "\n");
        HANDLE_FPRINTF_ERR(retval);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    int i;

    if (argc < 2) {
        usage();
        return -1;
    }            

    if (!strcmp(argv[1], "pli")) {
        if (argc < 3 || argc > 4) {
            usage();
            return -1;
        }

        for(i = 0; i < strlen(argv[2]); i++)
        {
            if (!isdigit(argv[2][i])) {
                usage();
                return -1;
            }
        }

        if (argc == 3) {
            get_aow_plilogs(atoi(argv[2]), NULL);
        } else {
            get_aow_plilogs(atoi(argv[2]),argv[3]);
        }
    }
    
    return 0;
}
