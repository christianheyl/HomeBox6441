/*
 * =====================================================================================
 *
 *       Filename:  aowstatlog.h
 *
 *    Description:  Header for application to get AoW stats
 *
 *        Version:  1.0
 *        Created:  Tuesday, July 6 2010
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Krishna C. Rao
 *        Company:  Atheros Communications 
 *
 * =====================================================================================
 */

#include <netinet/ether.h>


#define MAX_PLILOG_DURATION            (90)     /* In seconds */

/* Over-provisioning factor to accommodate multiple entries for 
   some MPDUs */
#define PLSTATSIZE_BUFF_FACTOR         (1.2)

/* Though we can support a max of 10 receivers, we provision for 
   more in case the receivers are changed in between */
#define RCVR_ADDR_STORE_SIZE           (0xFF)

/* Standard quick sort comparator codes */
#define OBJ1_GT_OBJ2                   (1)
#define OBJ2_GT_OBJ1                   (-1)
#define OBJ1_EQ_OBJ2                   (0)

struct pli_log_info
{
    /* Node type, whether transmitter or receiver */
    u_int16_t aow_node_type;

    /* List of unique receiver MAC addresses seen so far.
       Used only for Tx-side log information.
       Helps minimize space required for storing pl_element
       entries by using a code for every receiver MAC
       address rather than all the 6 bytes.
     */
    struct ether_addr rcvr_mac_addresses[RCVR_ADDR_STORE_SIZE];
    
    /* No. of unique receiver MAC addresses in rcvr_mac_addresses[] */
    u_int16_t num_rcvrs;

    /* Dynamically allocated array for holding pl_element entries
       received from driver. */
    struct pl_element *plstats;

    /* Size of plstats array. */
    unsigned int plstats_size;

    /* Number of pl_element entries received from driver and
       placed into plstats array. */
    unsigned int plstats_recvd;
};


