/*
 * Copyright (c) 2011, Qualcomm Atheros Inc.
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
 * Description:  Simple application to retrieve AP statistics at the AP level,
 *               Per-Radio level, Per-VAP level, and Per-Node level.
 *               Note: There are a variety of statistics which can be
 *               considered to fall under AP statistics. However, we limit
 *               ourselves to specific stats requested by customers. As further
 *               requests come in, this application can be expanded.
 *
 * Version    :  0.1
 * Created    :  Thursday, June 30, 2011
 * Revision   :  none
 * Compiler   :  gcc
 *
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/if.h>
#include <linux/wireless.h>
#include <netinet/ether.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <endian.h>
#include <errno.h>
#include <stdint.h>

#include "apstats.h"
#define DESC_FIELD_PROVISIONING         31

#define APSTATS_ERROR(fmt, args...) \
        fprintf(stderr, "apstats: Error - "fmt , ## args)

#define APSTATS_WARNING(fmt, args...) \
        fprintf(stdout, "apstats: Warning - "fmt , ## args)

#define APSTATS_MESG(fmt, args...) \
        fprintf(stdout, "apstats: "fmt , ## args)

#define APSTATS_PRINT_GENERAL(fmt, args...) \
        fprintf(stdout, fmt , ## args)

#define APSTATS_PRINT_STAT64(descr, x) \
        fprintf(stdout, "%-*s = %llu\n", DESC_FIELD_PROVISIONING, (descr), (x))

#define APSTATS_PRINT_STAT32(descr, x) \
        fprintf(stdout, "%-*s = %u\n", DESC_FIELD_PROVISIONING, (descr), (x))

#define APSTATS_PRINT_STAT16(descr, x) \
        fprintf(stdout, "%-*s = %hu\n", DESC_FIELD_PROVISIONING, (descr), (x))

#define APSTATS_PRINT_STAT8(descr, x) \
        fprintf(stdout, "%-*s = %hhu\n", DESC_FIELD_PROVISIONING, (descr), (x))

#define APSTATS_PRINT_STATFLT(descr, x, precsn) \
        fprintf(stdout, "%-*s = %.*f\n", DESC_FIELD_PROVISIONING, (descr), \
                                         (precsn), (x))

#define APSTATS_PRINT_STAT_UNVLBL(descr, msg) \
        fprintf(stdout, "%-*s = %s\n", DESC_FIELD_PROVISIONING, (descr), (msg))

#if !UMAC_SUPPORT_TXDATAPATH_NODESTATS
#define APSTATS_TXDPATH_NODESTATS_UNVLBL_MSG  "<Unavailable (Tx datapath " \
                                              "performance optimization)>"
#endif

/* Options processing */
static const char *optString = "arvsi:m:Rh?";

static const struct option longOpts[] = {
    { "aplevel", no_argument, NULL, 'a' },              /* Default */
    { "radiolevel", no_argument, NULL, 'r' },
    { "vaplevel", no_argument, NULL, 'v' },
    { "stalevel", no_argument, NULL, 's' },
    { "ifname", required_argument, NULL, 'i' },
    { "stamacaddr", required_argument, NULL, 'm' },
    { "recursive", no_argument, NULL, 'R' },            /* Default */
    { "help", no_argument, NULL, 'h' },
    { NULL, no_argument, NULL, 0 },
};

static void display_help();

static int is_radio_ifname_valid(const char *ifname);
static int is_vap_ifname_valid(const char *ifname);

static int is_vap_radiochild(const struct ether_addr *vapaddr,
                             const struct ether_addr *radioaddr);
static int nodestats_hierarchy_init(vaplevel_stats_t *apstats, int sockfd);
static int stats_hierachy_init(aplevel_stats_t *apstats, int sockfd);
static radiolevel_stats_t* stats_hierarchy_findradio(aplevel_stats_t *apstats,
                                                     char *ifname);
static vaplevel_stats_t* stats_hierarchy_findvap(aplevel_stats_t *apstats,
                                                 char *ifname);
static nodelevel_stats_t*
stats_hierarchy_findnode(aplevel_stats_t *apstats,
                         struct ether_addr *stamacaddr);
static int stats_hierachy_deinit(aplevel_stats_t *apstats);

static int get_vap_priv_int_param(int sockfd, const char *ifname, int param);
static int get_radio_priv_int_param(int sockfd, const char *ifname, int param);

static int ethernet_gather_stats(int sockfd,
                                 ethernet_stats_t *ethstats);
static int ethernet_display_stats(ethernet_stats_t *ethstats);
static int aplevel_gather_stats(int sockfd,
                                aplevel_stats_t *apstats,
                                apstats_recursion_t recursion);
static int aplevel_display_stats(aplevel_stats_t *apstats,
                                 apstats_recursion_t recursion);
static int radiolevel_gather_stats(int sockfd,
                                   radiolevel_stats_t *radiostats,
                                   apstats_recursion_t recursion);
static int radiolevel_display_stats(radiolevel_stats_t *radiostats,
                                    apstats_recursion_t recursion);
static int vaplevel_gather_stats(int sockfd,
                                 vaplevel_stats_t *vapstats,
                                 apstats_recursion_t recursion);
static int vaplevel_display_stats(vaplevel_stats_t *vapstats,
                                    apstats_recursion_t recursion);
static int nodelevel_gather_stats(int sockfd,
                                  nodelevel_stats_t *nodestats);
static char* macaddr_to_str(const struct ether_addr *addr);
static int nodelevel_display_stats(nodelevel_stats_t *nodestats);

static int aplevel_handler(int sockfd,
                           aplevel_stats_t *apstats,
                           apstats_recursion_t recursion);
static int radiolevel_handler(int sockfd,
                              radiolevel_stats_t *radiostats,
                              apstats_recursion_t recursion);
static int vaplevel_handler(int sockfd,
                            vaplevel_stats_t *vapstats,
                            apstats_recursion_t recursion);
static int nodelevel_handler(int sockfd,
                             nodelevel_stats_t *nodestats);

static int apstats_main(apstats_config_t *config);

/* We assume ifname has atleast IFNAMSIZ characters,
   as is the convention for Linux interface names. */
static int is_radio_ifname_valid(const char *ifname)
{
    int i;

    assert(ifname != NULL);
   
    /* At this step, we only validate if the string makes sense.
       If the interface doesn't actually exist, we'll throw an
       error at the place where we make system calls to try and
       use the interface.
       Reduces the no. of ioctl calls. */
    
    if (strncmp(ifname, "wifi", 4) != 0) {
        return 0;
    }

    if (!ifname[4] || !isdigit(ifname[4])) {
        return 0;
    }
	
    /* We don't make any assumptions on max no. of radio interfaces,
       at this step. */
    for (i = 5; i < IFNAMSIZ; i++)
    {
        if (!ifname[i]) {
            break;
        }

        if (!isdigit(ifname[i])) {
            return 0;
        }
    }

    return 1;
}

/* We assume ifname has atleast IFNAMSIZ characters,
   as is the convention for Linux interface names. */
static int is_vap_ifname_valid(const char *ifname)
{
    int i;
    int basename_len = 0;

    assert(ifname != NULL);

    /* At this step, we only validate if the string makes sense.
       If the interface doesn't actually exist, we'll throw an
       error at the place where we make system calls to try and
       use the interface.
       Reduces the no. of ioctl calls. */
    
    if (strncmp(ifname, "ath", 3) == 0) {
        basename_len = 3;
    } else if (strncmp(ifname, "wlan", 4) == 0) {
        basename_len = 4;
    } else {
        return 0;
    }

    if (!ifname[basename_len] || !isdigit(ifname[basename_len])) {
        return 0;
    }

    /* We don't make any assumptions on max no. of VAP interfaces,
       at this step. */
    for (i = basename_len + 1; i < IFNAMSIZ; i++)
    {
        if (!ifname[i]) {
            break;
        }

        if (!isdigit(ifname[i])) {
            return 0;
        }

    }

    return 1;
}


static void display_help()
{
    printf("\napstats v"APSTATS_VERSION": Display Access Point Statistics.\n"
           "\n"
           "Usage:\n"
           "\n"
           "apstats [Level] [[-i interface_name] | [-m STA_MAC_Address]] [-R]  \n"
           "\n"
           "One of four levels can be selected. The levels are as follows (from \n"
           "the top to the bottom of the hierarchy): \n"
           "Entire Access Point, Radio, VAP and STA. \n"
           "The default is Entire Access Point. Information for sub-levels below \n"
           "the selected level can be displayed by choosing the -R (recursive) \n"
           "option (not applicable for the STA level, where information for a \n"
           "single STA is displayed). \n"
           "\n"
           "LEVELS:\n"
           "\n"
           "-a or --aplevel\n"
           "    Entire Access Point Level (WLAN and Ethernet).\n"
           "    No Interface or STA MAC needs to be provided.\n"
           "\n"
           "-r or --radiolevel\n"
           "    Radio Level.\n"
           "    Radio Interface Name needs to be provided.\n"
           "\n"
           "-v or --vaplevel\n"
           "    VAP Level.\n"
           "    VAP Interface Name needs to be provided.\n"
           "\n"
           "-s or --stalevel\n"
           "    STA Level.\n"
           "    STA MAC Address needs to be provided.\n"
           "\n"
           "INTERFACE:\n"
           "\n"
           "If Radio Level is selected:\n"
           "-i wifiX or --ifname=wifiX\n"
           "\n"
           "If VAP Level is selected:\n"
           "-i athX or --ifname=athX\n"
           "-i wlanX or --ifname=wlanX\n"
           "\n"
           "STA MAC ADDRESS:\n"
           "\n"
           "Required if STA Level is selected:\n"
           "-m xx:xx:xx:xx:xx:xx or --stamacaddr xx:xx:xx:xx:xx:xx\n"
           "\n"
           "OTHER OPTIONS:\n"
           "-R or --recursive\n"
           "    Recursive display\n"
          );
}

/* Determine if VAP with given H/W address is the child of
   radio with given H/W address.
   To determine this, we check if the last 5 octets are the same. */
static int is_vap_radiochild(const struct ether_addr *vapaddr,
                             const struct ether_addr *radioaddr)
{
    if (memcmp(&(vapaddr->ether_addr_octet[1]),
               &(radioaddr->ether_addr_octet[1]),
               5) == 0) {
        return 1;
    } else {
        return 0;
    }
}

int sys_ifnames_init(sys_ifnames *ifs, int base_max_size)
{
    int i;

    assert(ifs != NULL);

    ifs->ifnames = (char**)malloc(base_max_size * sizeof(char*));
    if(!(ifs->ifnames)) {
        return -ENOMEM;
    }

    ifs->curr_size = 0;
    
    for (i = 0; i < base_max_size; i++) {
        ifs->ifnames[i] = (char*)malloc(IFNAMSIZ * sizeof(char));
        if (!(ifs->ifnames[i])) {
            sys_ifnames_deinit(ifs);
            return -1;
        }
        ifs->max_size += 1;
    }
}

int sys_ifnames_extend(sys_ifnames *ifs, int additional_size)
{
    int i;
    char **tempptr;

    assert(ifs != NULL);

    tempptr = (char**)realloc(ifs->ifnames,
                              (ifs->max_size + additional_size) * sizeof(char *));

    if(!tempptr) {
        /* Original block untouched */
        return -ENOMEM;
    }

    ifs->ifnames = tempptr;

    for (i = ifs->max_size; i < (ifs->max_size + additional_size); i++) {
        ifs->ifnames[i] = (char*)malloc(IFNAMSIZ * sizeof(char));

        if (!(ifs->ifnames[i])) {
            return -ENOMEM;
        }
        ifs->max_size += 1;
    }
}

void sys_ifnames_deinit(sys_ifnames *ifs)
{
    int i;

    assert(ifs != NULL);
   
    if (!ifs->ifnames) {
        return;
    }

    for (i = 0; i < ifs->max_size; i++) {
        free(ifs->ifnames[i]);
    }

    free(ifs->ifnames);
    ifs->ifnames = NULL;
    ifs->max_size = 0;
    ifs->curr_size = 0;
}

int sys_ifnames_add(sys_ifnames *ifs, char *ifname)
{
   int tempidx = 0;
   
   if (ifs->curr_size == ifs->max_size) {
        // Full
        return -ENOMEM;
   }

   tempidx = ifs->curr_size;
   strncpy(ifs->ifnames[tempidx], ifname, IFNAMSIZ);
   ifs->curr_size++;
}

/* Helper function to help stats_hierachy_init() fill in node level
   information. */
#define EXPECTED_MAX_NODES_PERVAP       (50)
#define EXPECTED_NODES_PERVAP_INCREMENT (10)
static int nodestats_hierarchy_init(vaplevel_stats_t *vapstats, int sockfd)
{
    struct iwreq iwr;
    struct ifreq ifr;
    u_int8_t *buf = NULL, *cp = NULL, *tempptr = NULL;
    struct ieee80211req_sta_info *si = NULL;
    nodelevel_stats_t *curr_ns_ptr = NULL;
    int buflen = 0;
    int remaining_len = 0;

    buflen = sizeof(struct ieee80211req_sta_info) *
             EXPECTED_MAX_NODES_PERVAP;

    buf = (u_int8_t*)malloc(buflen);

    if (NULL == buf) {
        APSTATS_ERROR("Unable to allocate memory for node information\n");
        return -ENOMEM;
    }

    memset(&iwr, 0, sizeof(iwr));
    strncpy(iwr.ifr_name, vapstats->ifname, sizeof(iwr.ifr_name));
    iwr.u.data.pointer = (void *)buf;
    iwr.u.data.length = buflen;
    if (ioctl(sockfd, IEEE80211_IOCTL_STA_INFO, &iwr) < 0) {
        perror("IEEE80211_IOCTL_STA_INFO");
        free(buf);
        return -1;
    }
    
    if (0 == iwr.u.data.length) {
        free(buf);
        return 0;
    }

    while (iwr.u.data.length == buflen)
    {
        /* Attempt to find the required buffer size */

        buflen += EXPECTED_NODES_PERVAP_INCREMENT;
        tempptr = (u_int8_t*)realloc(buf, buflen);
        if (NULL == tempptr) {
            APSTATS_ERROR("Unable to allocate memory for node information\n");
            free(buf);
            return -ENOMEM;
        }
        buf = tempptr;

        iwr.u.data.pointer = (void *)buf;
        iwr.u.data.length = buflen;

        if (ioctl(sockfd, IEEE80211_IOCTL_STA_INFO, &iwr) < 0) {
            perror("IEEE80211_IOCTL_STA_INFO");
            free(buf);
            return -1;
        }
    }

    remaining_len = iwr.u.data.length;
    if (remaining_len >= sizeof(struct ieee80211req_sta_info)) {
        cp = buf;
        do {
            si = (struct ieee80211req_sta_info *) cp;
   
            nodelevel_stats_t *nodestats =
                (nodelevel_stats_t *)malloc(sizeof(nodelevel_stats_t));

            if (NULL == nodestats) {
                APSTATS_ERROR("Unable to allocate memory for node stats\n");
                free(buf);
                /* For now, the caller of stats_hierachy_init() is expected to 
                   call stats_hierachy_deinit() to clean up the linked
                   list under apstats */
                return -ENOMEM;
            }
            
            memcpy(nodestats->macaddr.ether_addr_octet,
                   si->isi_macaddr,
                   IEEE80211_ADDR_LEN);

            /* As an optimization, copy ieee80211req_sta_info right 
               away. */
            memcpy(&nodestats->si, si, sizeof(struct ieee80211req_sta_info));
            
            nodestats->parent = vapstats;

            if (!vapstats->firstchild) {
                vapstats->firstchild = nodestats;
            } else {
                curr_ns_ptr = (nodelevel_stats_t*)vapstats->firstchild;
            
                while(curr_ns_ptr->next != NULL)
                {
                    curr_ns_ptr = curr_ns_ptr->next; 
                }

                curr_ns_ptr->next = nodestats;
                nodestats->prev = curr_ns_ptr;
            }

            cp += si->isi_len, remaining_len -= si->isi_len;
        } while (remaining_len >= sizeof(struct ieee80211req_sta_info));
    }

}

#define EXPECTED_MAX_RADIOS     (4)
#define EXPECTED_MAX_VAPS       (15 * EXPECTED_MAX_RADIOS)

/* For now, we require the caller to call stats_hierachy_deinit() 
   in case stats_hierachy_init() fails. */
static int stats_hierachy_init(aplevel_stats_t *apstats, int sockfd)
{
    int           i, j;
    int           ret;
    struct ifreq  dev;
    FILE          *fp;
    char          buf[512];
    char          temp_name[IFNAMSIZ];
    char          **radio_names;
    char          **vap_names;
    sys_ifnames   radio_listing;
    sys_ifnames   vap_listing;
    radiolevel_stats_t *curr_rs_ptr = NULL;
    vaplevel_stats_t *curr_vs_ptr = NULL;

    assert(apstats != NULL);

    memset(apstats, 0, sizeof(aplevel_stats_t));

    sys_ifnames_init(&radio_listing, EXPECTED_MAX_RADIOS);
    sys_ifnames_init(&vap_listing, EXPECTED_MAX_VAPS);

    /* Note: Since the number of interfaces is small, we
       use a simple algorithm to build the stats hierarchy.
       Also, since this is a quick single run program, we assume the
       interfaces do not 'disappear' during its execution. */


    /* Get radio and VAP names */

    fp = fopen(PATH_PROCNET_DEV, "r");

    if (NULL == fp) {
        perror(PATH_PROCNET_DEV);
        return -1;
    }

    /* Skip unwanted lines */
    fgets(buf, sizeof(buf), fp);
    fgets(buf, sizeof(buf), fp);

    while (fgets(buf, sizeof(buf), fp))
    {
        i = 0;
        j = 0;

        while (j < (IFNAMSIZ - 1) && buf[i] != ':') {
            if (isalnum(buf[i])) {
                temp_name[j] = buf[i];
                j++;
            }
            i++;
        }
        temp_name[j] = '\0';
       
        if (is_radio_ifname_valid(temp_name)) {
            ret = sys_ifnames_add(&radio_listing, temp_name);

            if (ret < 0) {
                ret = sys_ifnames_extend(&radio_listing, 10);
                
                if (ret < 0)
                {
                    sys_ifnames_deinit(&radio_listing);
                    sys_ifnames_deinit(&vap_listing);
                    fclose(fp);
                    return ret;
                }

                ret = sys_ifnames_add(&radio_listing, temp_name);
    
                if (ret < 0)
                {
                    sys_ifnames_deinit(&radio_listing);
                    sys_ifnames_deinit(&vap_listing);
                    fclose(fp);
                    return ret;
                }
            }
        } else if (is_vap_ifname_valid(temp_name)) {
            ret = sys_ifnames_add(&vap_listing, temp_name);

            if (ret < 0) {
                ret = sys_ifnames_extend(&vap_listing, 10);
                
                if (ret < 0)
                {
                    sys_ifnames_deinit(&radio_listing);
                    sys_ifnames_deinit(&vap_listing);
                    fclose(fp);
                    return ret;
                }

                ret = sys_ifnames_add(&vap_listing, temp_name);
    
                if (ret < 0)
                {
                    sys_ifnames_deinit(&radio_listing);
                    sys_ifnames_deinit(&vap_listing);
                    fclose(fp);
                    return ret;
                }
            }
        }
    }
    
    fclose(fp);

    /* Add radios */
    dev.ifr_addr.sa_family = AF_INET;

    for(i = 0; i < radio_listing.curr_size; i++)
    {
        radiolevel_stats_t *radiostats = NULL;
        
        strncpy(dev.ifr_name, radio_listing.ifnames[i], IFNAMSIZ);

        /* Get the MAC address */
        if(ioctl(sockfd, SIOCGIFHWADDR, &dev) < 0)
        {
            perror("SIOCGIFHWADDR");
            return -1;
        }
        
        radiostats = (radiolevel_stats_t *)malloc(sizeof(radiolevel_stats_t));
        memset(radiostats, 0, sizeof(radiolevel_stats_t));

        radiostats->parent = apstats;
        strncpy(radiostats->ifname, dev.ifr_name, sizeof(radiostats->ifname));
        memcpy(radiostats->macaddr.ether_addr_octet,
               dev.ifr_hwaddr.sa_data,
               6);
        
        if(curr_rs_ptr) {
            curr_rs_ptr->next = radiostats;
            radiostats->prev = curr_rs_ptr;
        } 

        curr_rs_ptr = radiostats;

        if (!(apstats->firstwlanchild)) {
            apstats->firstwlanchild = radiostats; 
        }
    }
    
    /* Add VAPs */

    for(i = 0; i < vap_listing.curr_size; i++)
    {
        vaplevel_stats_t *vapstats = NULL;
        
        strncpy(dev.ifr_name, vap_listing.ifnames[i], IFNAMSIZ);

        /* Get the MAC address */
        if(ioctl(sockfd, SIOCGIFHWADDR, &dev) < 0)
        {
            perror("SIOCGIFHWADDR");
            return -1;
        }
        
        vapstats = (vaplevel_stats_t *)malloc(sizeof(vaplevel_stats_t));
        memset(vapstats, 0, sizeof(vaplevel_stats_t));

        strncpy(vapstats->ifname, dev.ifr_name, sizeof(vapstats->ifname));
        memcpy(vapstats->macaddr.ether_addr_octet, dev.ifr_hwaddr.sa_data, 6);
       
        /* Find parent radio */

        curr_rs_ptr = (radiolevel_stats_t*)apstats->firstwlanchild;
        
        while(curr_rs_ptr != NULL)
        {
            if (is_vap_radiochild(&(vapstats->macaddr),
                                  &(curr_rs_ptr->macaddr))) {
                break;
            }

            curr_rs_ptr = curr_rs_ptr->next;
        }

        if (!curr_rs_ptr) {
            /* Huh? This should never happen. */
            APSTATS_ERROR("Serious: Found orphaned VAP\n");
            free(vapstats);
            return -ENOENT;
        }

        if ((ret = nodestats_hierarchy_init(vapstats, sockfd)) < 0)
        {
            free(vapstats);
            return ret;
        }

        vapstats->parent = curr_rs_ptr;

        if (!curr_rs_ptr->firstchild) {
            curr_rs_ptr->firstchild = vapstats;
        } else {
            curr_vs_ptr = (vaplevel_stats_t*)curr_rs_ptr->firstchild;
        
            while(curr_vs_ptr->next != NULL)
            {
                curr_vs_ptr = curr_vs_ptr->next; 
            }

            curr_vs_ptr->next = vapstats;
            vapstats->prev = curr_vs_ptr;
            /* No need to update curr_vs_ptr itself - since this is a 
               simple implementation, the ptr will be detected on the next pass.
               We have multiple radios, and we don't wan't to maintain 
               a curr_vs_ptr per radio */
        }
    }

    return 0;
}

static radiolevel_stats_t* stats_hierarchy_findradio(aplevel_stats_t *apstats,
                                                     char *ifname)
{
    radiolevel_stats_t *curr_rs_ptr = NULL;
    assert(apstats != NULL);

    curr_rs_ptr = (radiolevel_stats_t*)apstats->firstwlanchild;

    while(curr_rs_ptr) {
        if (!strncmp(curr_rs_ptr->ifname, ifname, IFNAMSIZ)) {
            return curr_rs_ptr;
        }
        curr_rs_ptr = curr_rs_ptr->next;
    }

    return NULL;
}

static vaplevel_stats_t* stats_hierarchy_findvap(aplevel_stats_t *apstats,
                                                char *ifname)
{
    radiolevel_stats_t *curr_rs_ptr = NULL;
    vaplevel_stats_t *curr_vs_ptr = NULL;
    
    assert(apstats != NULL);

    curr_rs_ptr = (radiolevel_stats_t*)apstats->firstwlanchild;

    while(curr_rs_ptr) {
        curr_vs_ptr = (vaplevel_stats_t*)curr_rs_ptr->firstchild;
        
        while(curr_vs_ptr) {
            if (!strncmp(curr_vs_ptr->ifname, ifname, IFNAMSIZ)) {
                return curr_vs_ptr;
            }
            curr_vs_ptr = curr_vs_ptr->next;
        }

        curr_rs_ptr = curr_rs_ptr->next;
    }

    return NULL;
}

/* The below code can be made more efficient. But we keep it simple
   since it is called just once during application init. */
static nodelevel_stats_t*
stats_hierarchy_findnode(aplevel_stats_t *apstats,
                         struct ether_addr *stamacaddr)
{
    radiolevel_stats_t *curr_rs_ptr = NULL;
    vaplevel_stats_t *curr_vs_ptr = NULL;
    nodelevel_stats_t *curr_ns_ptr = NULL;
    
    assert(apstats != NULL);

    curr_rs_ptr = (radiolevel_stats_t*)apstats->firstwlanchild;

    while(curr_rs_ptr) {
        curr_vs_ptr = (vaplevel_stats_t*)curr_rs_ptr->firstchild;
        
        while(curr_vs_ptr) {
            curr_ns_ptr = (nodelevel_stats_t*)curr_vs_ptr->firstchild;
            
            while(curr_ns_ptr) {
                if (memcmp(&curr_ns_ptr->macaddr,
                           stamacaddr,
                           sizeof(struct ether_addr)) == 0) {
                    return curr_ns_ptr;
                }
                
                curr_ns_ptr = curr_ns_ptr->next;
            }

            curr_vs_ptr = curr_vs_ptr->next;
        }

        curr_rs_ptr = curr_rs_ptr->next;
    }

    return NULL;
}

static int stats_hierachy_deinit(aplevel_stats_t *apstats)
{
    radiolevel_stats_t *curr_rs_ptr = NULL;
    vaplevel_stats_t *curr_vs_ptr = NULL;
    nodelevel_stats_t *curr_ns_ptr = NULL;
    radiolevel_stats_t *next_rs_ptr = NULL;
    vaplevel_stats_t *next_vs_ptr = NULL;
    nodelevel_stats_t *next_ns_ptr = NULL;

    assert(apstats != NULL);

    curr_rs_ptr = (radiolevel_stats_t*)apstats->firstwlanchild;

    while(curr_rs_ptr)
    {
        curr_vs_ptr = (vaplevel_stats_t*)curr_rs_ptr->firstchild;

        while(curr_vs_ptr) {

            curr_ns_ptr = (nodelevel_stats_t*)curr_vs_ptr->firstchild;

            while(curr_ns_ptr) {
                next_ns_ptr = curr_ns_ptr->next;
                free(curr_ns_ptr);
                curr_ns_ptr = next_ns_ptr;
            }

            next_vs_ptr = curr_vs_ptr->next;
            free(curr_vs_ptr);
            curr_vs_ptr = next_vs_ptr;
        }

        next_rs_ptr = curr_rs_ptr->next;
        free(curr_rs_ptr);
        curr_rs_ptr = next_rs_ptr;
    }

    apstats->firstwlanchild = NULL;
}

static int get_vap_priv_int_param(int sockfd, const char *ifname, int param)
{
	struct iwreq iwr;

	memset(&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, ifname, IFNAMSIZ);
	iwr.u.mode = param;

	if (ioctl(sockfd, IEEE80211_IOCTL_GETPARAM, &iwr) < 0) {
		perror("IEEE80211_IOCTL_GETPARAM");
		return -1;
	}
    
    return iwr.u.param.value;
}

static int get_radio_priv_int_param(int sockfd, const char *ifname, int param)
{
    struct iwreq iwr;

    memset(&iwr, 0, sizeof(iwr));
    strncpy(iwr.ifr_name, ifname, IFNAMSIZ);
    switch (param) {
    case OL_ATH_PARAM_GET_IF_ID:
        iwr.u.mode = param | ATH_PARAM_SHIFT;
        if (ioctl(sockfd, ATH_IOCTL_GETPARAM, &iwr) < 0) {
            perror("ATH_IOCTL_GETPARAM");
            return -1;
        }
        break;
    
    default:
        iwr.u.mode = param | ATH_PARAM_SHIFT;
            if (ioctl(sockfd, ATH_IOCTL_GETPARAM, &iwr) < 0) {
                perror("ATH_IOCTL_GETPARAM");
                return -1;
            }

    }
    return iwr.u.param.value;
}

static int ethernet_gather_stats(int sockfd,
                                 ethernet_stats_t *ethstats)
{
    struct eth_cfg_params etd;
    struct ifreq ifr;
    const char *ifname = "eth0";

    /* Here, we illustrate how to get stats for Ethernet GMAC */

    assert(ethstats != NULL);
    
    strncpy(etd.ad_name, ifname, sizeof(etd.ad_name));

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ifr.ifr_data = (void *) &etd;
    
    /* Get speed */

    etd.cmd = ATHR_PORT_STATS;
    
    if (ioctl (sockfd, ATHR_PHY_CTRL_IOC, &ifr) < 0) {
        perror("ATHR_PHY_CTRL_IOC");
        return -1;
    }
   
    switch(etd.phy_st.speed) {
        case ETHSPEED_10MBPS:
            ethstats->tx_rate = ethstats->rx_rate = RATE_10K_KBPS;
            break; 
        
        case ETHSPEED_100MBPS:
            ethstats->tx_rate = ethstats->rx_rate = RATE_100K_KBPS;
            break; 
        
        case ETHSPEED_1000MBPS:
            ethstats->tx_rate = ethstats->rx_rate = RATE_1000K_KBPS;
            break;

        /* At this point, we won't try to guess what codes
           will be used by the Ethernet driver in the future
           for speeds higher than 1 Gbps. Once the codes
           are introduced, they can be added here. */

         default:
            APSTATS_ERROR("Unknown Ethernet speed code returned by driver.\n");
            break;
    }
    
    /* Get Tx/Rx stats */

    etd.cmd = ATHR_GMAC_STATS;
    
    if (ioctl (sockfd, ATHR_GMAC_CTRL_IOC, &ifr) < 0) {
        perror("ATHR_GMAC_CTRL_IOC");
        return -1;
    }
    ethstats->rx_packets       = etd.rxmac.pkt_cntr;
    ethstats->rx_bytes         = etd.rxmac.byte_cntr;
    ethstats->tx_packets       = etd.txmac.pkt_cntr;
    ethstats->tx_bytes         = etd.txmac.byte_cntr;
   
    ethstats->rx_mcast_packets = etd.rxmac.mcast_cntr;
    ethstats->tx_mcast_packets = etd.txmac.mcast_cntr;
    ethstats->rx_bcast_packets = etd.rxmac.bcast_cntr;
    ethstats->tx_bcast_packets = etd.txmac.bcast_cntr;
    
    ethstats->rx_ucast_packets = ethstats->rx_packets -
                                 (ethstats->rx_mcast_packets +
                                  ethstats->rx_bcast_packets);
    ethstats->tx_ucast_packets = ethstats->tx_packets -
                                 (ethstats->tx_mcast_packets +
                                  ethstats->tx_bcast_packets);

    ethstats->tx_err           = etd.txmac.fcserr_cntr;

}

static int ethernet_display_stats(ethernet_stats_t *ethstats)
{
    assert(ethstats != NULL);
    
    APSTATS_PRINT_GENERAL("Ethernet Stats:\n");

    if (ethstats->rx_rate) {
        APSTATS_PRINT_STAT32("Rx Link Rate (kbps)",     ethstats->rx_rate);
    }

    if (ethstats->tx_rate) {
        APSTATS_PRINT_STAT32("Tx Link Rate (kbps)",     ethstats->tx_rate);
    }

    APSTATS_PRINT_STAT32("Rx Unicast Packets",      ethstats->rx_ucast_packets);
    APSTATS_PRINT_STAT32("Tx Unicast Packets",      ethstats->tx_ucast_packets);
    APSTATS_PRINT_STAT32("Rx Multicast Packets",    ethstats->rx_mcast_packets);
    APSTATS_PRINT_STAT32("Tx Multicast Packets",    ethstats->tx_mcast_packets);
    APSTATS_PRINT_STAT32("Rx Broadcast Packets",    ethstats->rx_bcast_packets);
    APSTATS_PRINT_STAT32("Tx Broadcast Packets",    ethstats->tx_bcast_packets);
    APSTATS_PRINT_STAT32("Tx Errors",               ethstats->tx_err);
    APSTATS_PRINT_STAT32("Rx Packets",              ethstats->rx_packets);
    APSTATS_PRINT_STAT32("Rx Bytes",                ethstats->rx_bytes);
    APSTATS_PRINT_STAT32("Tx Packets",              ethstats->tx_packets);
    APSTATS_PRINT_STAT32("Tx Bytes",                ethstats->tx_bytes);
    APSTATS_PRINT_GENERAL("\n");

}

static int aplevel_gather_stats(int sockfd,
                                aplevel_stats_t *apstats,
                                apstats_recursion_t recursion)
{
    radiolevel_stats_t *curr_rs_ptr = NULL;
    u_int32_t chan_util_numrtr = 0;
    u_int16_t chan_util_denomntr = 0;
    u_int16_t prdic_per_numrtr = 0;
    u_int16_t prdic_per_denomntr = 0;
    u_int16_t total_per_numrtr = 0;
    u_int16_t total_per_denomntr = 0;
    u_int64_t tx_rate_numrtr = 0;
    u_int64_t rx_rate_numrtr = 0;
    u_int16_t txrx_rate_denomntr = 0;

    assert(apstats != NULL);

    /* Gather WLAN statistics */
    apstats->res_util_enab  = 1;
    apstats->prdic_per_enab = 1;

    curr_rs_ptr = (radiolevel_stats_t*)apstats->firstwlanchild;

    while(curr_rs_ptr) {
        radiolevel_gather_stats(sockfd, curr_rs_ptr, recursion);

        apstats->tx_data_packets += curr_rs_ptr->tx_data_packets;
        apstats->tx_data_bytes   += curr_rs_ptr->tx_data_bytes;
        apstats->rx_data_packets += curr_rs_ptr->rx_data_packets;
        apstats->rx_data_bytes   += curr_rs_ptr->rx_data_bytes;
        
        apstats->tx_ucast_data_packets  +=
            curr_rs_ptr->tx_ucast_data_packets;
        apstats->tx_mbcast_data_packets +=
            curr_rs_ptr->tx_mbcast_data_packets;
        
        apstats->rx_phyerr       += curr_rs_ptr->rx_phyerr;
        apstats->rx_crcerr       += curr_rs_ptr->rx_crcerr;
        apstats->rx_micerr       += curr_rs_ptr->rx_micerr;
        apstats->rx_decrypterr   += curr_rs_ptr->rx_decrypterr;
        apstats->rx_err          += curr_rs_ptr->rx_err;

        apstats->tx_discard      += curr_rs_ptr->tx_discard;

        /* We need all radios to have valid channel utilization
           numbers, for the AP resource utilization to make sense. */
        apstats->res_util_enab   &= curr_rs_ptr->chan_util_enab;

        if (apstats->res_util_enab) {
            chan_util_numrtr         += curr_rs_ptr->chan_util;
            chan_util_denomntr++;
        }

        if (curr_rs_ptr->txrx_rate_available) {
            apstats->txrx_rate_available = 1;
            txrx_rate_denomntr++;
            tx_rate_numrtr += curr_rs_ptr->tx_rate;
            rx_rate_numrtr += curr_rs_ptr->rx_rate;
        }

        if (curr_rs_ptr->thrput_enab) {
            apstats->thrput_enab = 1;
            apstats->thrput += curr_rs_ptr->thrput;
        }
        
        total_per_numrtr += curr_rs_ptr->total_per;
        total_per_denomntr++;

        /* We need all radios to have valid periodic PER numbers,
           for the AP periodic PER to make sense. */
        apstats->prdic_per_enab   &= curr_rs_ptr->prdic_per_enab;

        if (apstats->prdic_per_enab) {
            prdic_per_numrtr         += curr_rs_ptr->prdic_per;
            prdic_per_denomntr++;
        }
        
        curr_rs_ptr = curr_rs_ptr->next;
    }
   
    if (apstats->firstwlanchild != NULL) {
        if (apstats->res_util_enab) {
            apstats->res_util = chan_util_numrtr/chan_util_denomntr;
        }

        if (apstats->txrx_rate_available) {
            apstats->tx_rate = tx_rate_numrtr/txrx_rate_denomntr;
            apstats->rx_rate = rx_rate_numrtr/txrx_rate_denomntr;
        }

        apstats->total_per = total_per_numrtr/total_per_denomntr;
        
        if(apstats->prdic_per_enab) {
            apstats->prdic_per = prdic_per_numrtr/prdic_per_denomntr;
        }
    }

    /* Gather Ethernet statistics */
    ethernet_gather_stats(sockfd, &apstats->ethstats);
}

static int aplevel_display_stats(aplevel_stats_t *apstats,
                                 apstats_recursion_t recursion)
{
    radiolevel_stats_t *curr_rs_ptr = NULL;

    assert(apstats != NULL);
    
    APSTATS_PRINT_GENERAL("AP Level Stats:\n");
    APSTATS_PRINT_GENERAL("\n");
    
    if (apstats->firstwlanchild != NULL) {
        APSTATS_PRINT_GENERAL("WLAN Stats:\n");
        APSTATS_PRINT_STAT64("Tx Data Packets",      apstats->tx_data_packets);
        APSTATS_PRINT_STAT64("Tx Data Bytes",        apstats->tx_data_bytes);
        APSTATS_PRINT_STAT64("Rx Data Packets",      apstats->rx_data_packets);
        APSTATS_PRINT_STAT64("Rx Data Bytes",        apstats->rx_data_bytes);
        APSTATS_PRINT_STAT64("Tx Unicast Data Packets",
                apstats->tx_ucast_data_packets);
        APSTATS_PRINT_STAT64("Tx Multi/Broadcast Data Packets",
                apstats->tx_mbcast_data_packets);
       
        if (apstats->res_util_enab) {
            APSTATS_PRINT_STAT16("Resource Utilization (0-255)",  apstats->res_util);
        } else {
            APSTATS_PRINT_STAT_UNVLBL("Resource Utilization (0-255)", "<DISABLED>");
        }

        if (apstats->txrx_rate_available) {
            APSTATS_PRINT_STAT32("Average Tx Rate (kbps)", apstats->tx_rate);
            APSTATS_PRINT_STAT32("Average Rx Rate (kbps)", apstats->rx_rate);
        } else {
            APSTATS_PRINT_STAT_UNVLBL("Average Tx Rate (kbps)", "<NO STA>");
            APSTATS_PRINT_STAT_UNVLBL("Average Rx Rate (kbps)", "<NO STA>");
        }

        APSTATS_PRINT_STAT64("Rx PHY errors",        apstats->rx_phyerr);
        APSTATS_PRINT_STAT64("Rx CRC errors",        apstats->rx_crcerr);
        APSTATS_PRINT_STAT64("Rx MIC errors",        apstats->rx_micerr);
        APSTATS_PRINT_STAT64("Rx Decryption errors", apstats->rx_decrypterr);
        APSTATS_PRINT_STAT64("Rx errors",            apstats->rx_err);
        APSTATS_PRINT_STAT64("Tx failures",          apstats->tx_discard);

        if (apstats->thrput_enab) {
            APSTATS_PRINT_STAT32("Throughput (kbps)", apstats->thrput);
        } else {
            APSTATS_PRINT_STAT_UNVLBL("Throughput (kbps)", "<DISABLED>");
        }
         
        APSTATS_PRINT_STAT8("Total PER (%)",          apstats->total_per);
        
        if (apstats->prdic_per_enab) {
            APSTATS_PRINT_STAT8("PER over configured period (%)",
                                apstats->prdic_per);
        } else {
            APSTATS_PRINT_STAT_UNVLBL("PER over configured period (%)",
                                      "<DISABLED>");
        }

        APSTATS_PRINT_GENERAL("\n");
    } else {
         APSTATS_PRINT_GENERAL("WLAN stats:\nNo radio interfaces found\n\n");
    }

    ethernet_display_stats(&apstats->ethstats);

    if (recursion == APSTATS_RECURSION_NO_VAL ||
        recursion == APSTATS_RECURSION_FULLY_DISABLED) {
        return 0;
    } 

    curr_rs_ptr = (radiolevel_stats_t*)apstats->firstwlanchild;

    while(curr_rs_ptr) {
        radiolevel_display_stats(curr_rs_ptr, recursion);
        curr_rs_ptr = curr_rs_ptr->next;
    }
}

static int radiolevel_gather_stats(int sockfd,
                                   radiolevel_stats_t *radiostats,
                                   apstats_recursion_t recursion)
{
    vaplevel_stats_t *curr_vs_ptr = NULL;
    struct ath_phy_stats phystats[WIRELESS_MODE_MAX];
    struct ath_stats stats = { 0 };
    struct ath_stats_container stats_cntnr;
    struct ol_ath_radiostats ol_stats;
    struct ifreq ifr;
    u_int32_t chan_util_numrtr = 0;
    u_int16_t chan_util_denomntr = 0;
    u_int64_t tx_rate_numrtr = 0;
    u_int64_t rx_rate_numrtr = 0;
    u_int16_t txrx_rate_denomntr = 0;
    int i;

    assert(radiostats != NULL);
    
    /* Gather pure radio-level stats */

    /* PHY stats */

    strncpy(ifr.ifr_name, radiostats->ifname, sizeof(ifr.ifr_name));
    /*  which interface is this Off-Load or Direct-Attach */
    radiostats->ol_enable = get_radio_priv_int_param(sockfd, radiostats->ifname,
                                    OL_ATH_PARAM_GET_IF_ID);
    if (!radiostats->ol_enable) {
        memset(phystats, 0, sizeof(phystats));
        ifr.ifr_data = (caddr_t) phystats;
        if (ioctl(sockfd, SIOCGATHPHYSTATS, &ifr) < 0) {
            perror("SIOCGATHPHYSTATS");
            return -1;
        }

        for (i = 0; i < WIRELESS_MODE_MAX; i++) {
            radiostats->rx_phyerr +=
                (phystats[i].ast_rx_phyerr -
                 phystats[i].ast_rx_phy[HAL_PHYERR_RADAR & 0x1f] -
                 phystats[i].ast_rx_phy[HAL_PHYERR_FALSE_RADAR_EXT & 0x1f] -
                 phystats[i].ast_rx_phy[HAL_PHYERR_SPECTRAL & 0x1f]);

            radiostats->rx_crcerr +=
                (phystats[i].ast_rx_crcerr);
        }

        /* Other radio level stats */

        stats_cntnr.size = sizeof(struct ath_stats);
        stats_cntnr.address = &stats;
        ifr.ifr_data = (caddr_t)&stats_cntnr;

        if (ioctl(sockfd, SIOCGATHSTATS, &ifr) < 0) {
            perror("SIOCGATHSTATS");
            return -1;
        }

        radiostats->tx_beacon_frames = stats.ast_be_xmit;
        radiostats->tx_mgmt_frames = stats.ast_tx_mgmt;
        radiostats->rx_mgmt_frames = stats.ast_rx_num_mgmt;

        radiostats->rx_rssi = stats.ast_rx_rssi;
    } else {
        memset(&ol_stats, 0, sizeof(ol_stats));
        ifr.ifr_data = (caddr_t) &ol_stats;
        if (ioctl(sockfd, SIOCGATHPHYSTATS, &ifr) < 0) {
            perror("SIOCGATHPHYSTATS");
            return -1;
        }
        radiostats->rx_phyerr = ol_stats.rx_phyerr;
        radiostats->tx_beacon_frames = ol_stats.tx_beacon;
        radiostats->tx_mgmt_frames = (long unsigned int)ol_stats.tx_mgmt;
        radiostats->rx_mgmt_frames = (long unsigned int)ol_stats.rx_mgmt;

        radiostats->rx_rssi = ol_stats.rx_rssi_comb;
        radiostats->rx_crcerr = ol_stats.rx_crcerr;
    }

#if UMAC_SUPPORT_PERIODIC_PERFSTATS
    /* Throughput */
    if (!radiostats->ol_enable) {
        radiostats->thrput_enab  = get_radio_priv_int_param(
                                              sockfd,
                                              radiostats->ifname,
                                              ATH_PARAM_PRDPERFSTAT_THRPUT_ENAB);

        if (radiostats->thrput_enab) {
            radiostats->thrput  = get_radio_priv_int_param(
                                                 sockfd,
                                                 radiostats->ifname,
                                                 ATH_PARAM_PRDPERFSTAT_THRPUT);
        }

        /* PER */
        radiostats->prdic_per_enab  = get_radio_priv_int_param(
                                               sockfd,
                                               radiostats->ifname,
                                               ATH_PARAM_PRDPERFSTAT_PER_ENAB);

        if (radiostats->prdic_per_enab) {
            radiostats->prdic_per  = get_radio_priv_int_param(
                                                 sockfd,
                                                 radiostats->ifname,
                                                 ATH_PARAM_PRDPERFSTAT_PER);
        }
    } else {
        radiostats->thrput_enab  = get_radio_priv_int_param(
                                                sockfd,
                                                radiostats->ifname,
                                                OL_ATH_PARAM_PRDPERFSTAT_THRPUT_ENAB);
        if (radiostats->thrput_enab) {
        radiostats->thrput  = get_radio_priv_int_param(
                                                sockfd,
                                                radiostats->ifname,
                                                OL_ATH_PARAM_PRDPERFSTAT_THRPUT);
        }
        /* PER */
        radiostats->prdic_per_enab  = get_radio_priv_int_param(
                                                sockfd,
                                                radiostats->ifname,
                                                OL_ATH_PARAM_PRDPERFSTAT_PER_ENAB);

        if (radiostats->prdic_per_enab) {
        radiostats->prdic_per  = get_radio_priv_int_param(
                                                sockfd,
                                                radiostats->ifname,
                                                OL_ATH_PARAM_PRDPERFSTAT_PER);

        }
    }
#else 
    radiostats->thrput_enab = 0;
    radiostats->prdic_per_enab = 0;
#endif /* UMAC_SUPPORT_PERIODIC_PERFSTATS */
    if (!radiostats->ol_enable) {
        radiostats->total_per  = get_radio_priv_int_param(
                                               sockfd,
                                               radiostats->ifname,
                                               ATH_PARAM_TOTAL_PER);
    } else {
    radiostats->total_per  = get_radio_priv_int_param(
                                                    sockfd,
                                                    radiostats->ifname,
                                                    OL_ATH_PARAM_TOTAL_PER);

    }

    /* Gather VAP level stats and populate stats which depend on
       VAP level */

    curr_vs_ptr = (vaplevel_stats_t*)radiostats->firstchild;
    
    while(curr_vs_ptr) {
        vaplevel_gather_stats(sockfd, curr_vs_ptr, recursion);
        
        radiostats->tx_data_packets += curr_vs_ptr->tx_data_packets;
        radiostats->tx_data_bytes   += curr_vs_ptr->tx_data_bytes;
        radiostats->rx_data_packets += curr_vs_ptr->rx_data_packets;
        radiostats->rx_data_bytes   += curr_vs_ptr->rx_data_bytes;

        radiostats->tx_ucast_data_packets  +=
            curr_vs_ptr->tx_ucast_data_packets;
        radiostats->tx_mbcast_data_packets +=
            curr_vs_ptr->tx_mbcast_data_packets;

        radiostats->rx_micerr       += curr_vs_ptr->rx_micerr;
        radiostats->rx_decrypterr   += curr_vs_ptr->rx_decrypterr;
        radiostats->rx_err          += curr_vs_ptr->rx_err;

        radiostats->tx_discard      += curr_vs_ptr->tx_discard;

        /* If at least one of the VAPs have channel utilization
           enabled, then we are good. */
        radiostats->chan_util_enab   |= curr_vs_ptr->chan_util_enab;

        if (curr_vs_ptr->chan_util_enab) {
            chan_util_numrtr            += curr_vs_ptr->chan_util;
            chan_util_denomntr++;
        }
        
        if (curr_vs_ptr->txrx_rate_available) {
            radiostats->txrx_rate_available = 1;
            txrx_rate_denomntr++;
            tx_rate_numrtr += curr_vs_ptr->tx_rate;
            rx_rate_numrtr += curr_vs_ptr->rx_rate;
        }

        curr_vs_ptr = curr_vs_ptr->next;
    }

    if (radiostats->firstchild != NULL) {
        if (radiostats->chan_util_enab) {
            /* Take the average of channel utilization values across VAPs.
               One might initially assume that these should be the same across
               VAPs.
               However, these are calculated as per the standard, factoring
               individual beacon periods. These periods are currently the same
               for all VAPS, but we shouldn't assume they will remain so in
               the future.
               Besides, there are variances in the time at which channel utilization
               calculation takes place for individual VAPs.
             */
            radiostats->chan_util = chan_util_numrtr/chan_util_denomntr;
        }
    
        if (radiostats->txrx_rate_available) {
            radiostats->tx_rate = tx_rate_numrtr/txrx_rate_denomntr;
            radiostats->rx_rate = rx_rate_numrtr/txrx_rate_denomntr;
        }

        /* Other averaging goes here */
    }

    return 0;
}

static int radiolevel_display_stats(radiolevel_stats_t *radiostats,
                                    apstats_recursion_t recursion)
{
    vaplevel_stats_t *curr_vs_ptr = NULL;

    assert(radiostats != NULL);
    
    APSTATS_PRINT_GENERAL("Radio Level Stats: %s\n",
                          radiostats->ifname);
    APSTATS_PRINT_STAT64("Tx Data Packets",      radiostats->tx_data_packets);
    APSTATS_PRINT_STAT64("Tx Data Bytes",        radiostats->tx_data_bytes);
    APSTATS_PRINT_STAT64("Rx Data Packets",      radiostats->rx_data_packets);
    APSTATS_PRINT_STAT64("Rx Data Bytes",        radiostats->rx_data_bytes);
    APSTATS_PRINT_STAT64("Tx Unicast Data Packets",
            radiostats->tx_ucast_data_packets);
    APSTATS_PRINT_STAT64("Tx Multi/Broadcast Data Packets",
            radiostats->tx_mbcast_data_packets);

    if (radiostats->chan_util_enab) {
        APSTATS_PRINT_STAT16("Channel Utilization (0-255)",  radiostats->chan_util);
    } else {
        APSTATS_PRINT_STAT_UNVLBL("Channel Utilization (0-255)", "<DISABLED>");
    }

    APSTATS_PRINT_STAT64("Tx Beacon Frames",     radiostats->tx_beacon_frames);
    APSTATS_PRINT_STAT64("Tx Mgmt Frames",       radiostats->tx_mgmt_frames);
    APSTATS_PRINT_STAT64("Rx Mgmt Frames",       radiostats->rx_mgmt_frames);
    APSTATS_PRINT_STAT8("Rx RSSI",               radiostats->rx_rssi);
    APSTATS_PRINT_STAT64("Rx PHY errors",        radiostats->rx_phyerr);
    APSTATS_PRINT_STAT64("Rx CRC errors",        radiostats->rx_crcerr);
    APSTATS_PRINT_STAT64("Rx MIC errors",        radiostats->rx_micerr);
    APSTATS_PRINT_STAT64("Rx Decryption errors", radiostats->rx_decrypterr);
    APSTATS_PRINT_STAT64("Rx errors",            radiostats->rx_err);
    APSTATS_PRINT_STAT64("Tx failures",          radiostats->tx_discard);
    
    if (radiostats->thrput_enab) {
        APSTATS_PRINT_STAT32("Throughput (kbps)",  radiostats->thrput);
    } else {
        APSTATS_PRINT_STAT_UNVLBL("Throughput (kbps)", "<DISABLED>");
    }

    if (radiostats->prdic_per_enab) {
        APSTATS_PRINT_STAT8("PER over configured period (%)",
                            radiostats->prdic_per);
    } else {
        APSTATS_PRINT_STAT_UNVLBL("PER over configured period (%)",
                                  "<DISABLED>");
    }


    APSTATS_PRINT_STAT8("Total PER (%)",         radiostats->total_per);
     
    APSTATS_PRINT_GENERAL("\n");

    if (recursion == APSTATS_RECURSION_NO_VAL ||
        recursion == APSTATS_RECURSION_FULLY_DISABLED) {
        return 0;
    } 

    curr_vs_ptr = (vaplevel_stats_t*)radiostats->firstchild;

    while(curr_vs_ptr) {
        vaplevel_display_stats(curr_vs_ptr, recursion);
        curr_vs_ptr = curr_vs_ptr->next;
    }
}

static int vaplevel_gather_stats(int sockfd,
                                 vaplevel_stats_t *vapstats,
                                 apstats_recursion_t recursion)
{
    int ret = -1;
    nodelevel_stats_t *curr_ns_ptr = NULL;
    u_int64_t tx_rate_numrtr = 0;
    u_int64_t rx_rate_numrtr = 0;
    u_int16_t txrx_rate_denomntr = 0;
    
    struct ieee80211_stats *stats;
    struct ieee80211_mac_stats *ucaststats;
    struct ieee80211_mac_stats *mcaststats;
    struct ifreq ifr;
    int    stats_total_len = sizeof(struct ieee80211_stats) + 
                             (2 * sizeof(struct ieee80211_mac_stats)) +
                             4;
    
    assert(vapstats != NULL);

    /* Gather pure VAP level stats */

    stats = (struct ieee80211_stats *)malloc(stats_total_len);

    memset(stats, 0, stats_total_len);

    strncpy(ifr.ifr_name, vapstats->ifname, sizeof(ifr.ifr_name));
    ifr.ifr_data = (caddr_t) stats;
    
    if (ioctl(sockfd, SIOCG80211STATS, &ifr) < 0) {
        perror("SIOCG80211STATS");
        return -1;
    }
    
    ucaststats = (struct ieee80211_mac_stats*)
                    ((unsigned char *)stats +
                      sizeof(struct ieee80211_stats) + 4);
    mcaststats = (struct ieee80211_mac_stats*)
                    ((unsigned char *)ucaststats +
                     sizeof(struct ieee80211_mac_stats));

    /* Note that mcaststats cover broadcast stats as well.
       Multicast and broadcast frames are treated the same
       way in most portions of the WLAN driver, and this is
       reflected in bunching together of the stats as well. */

    vapstats->tx_data_packets = ucaststats->ims_tx_data_packets +
                                mcaststats->ims_tx_data_packets;
    vapstats->tx_data_bytes   = ucaststats->ims_tx_data_bytes +
                                mcaststats->ims_tx_data_bytes;
    vapstats->rx_data_packets = ucaststats->ims_rx_data_packets +
                                mcaststats->ims_rx_data_packets;
    vapstats->rx_data_bytes   = ucaststats->ims_rx_data_bytes +
                                mcaststats->ims_rx_data_bytes;
    
    vapstats->tx_datapyld_bytes   = ucaststats->ims_tx_datapyld_bytes +
                                    mcaststats->ims_tx_datapyld_bytes;
    vapstats->rx_datapyld_bytes   = ucaststats->ims_rx_datapyld_bytes +
                                    mcaststats->ims_rx_datapyld_bytes;

    vapstats->tx_ucast_data_packets  = ucaststats->ims_tx_data_packets;
    vapstats->tx_mbcast_data_packets = mcaststats->ims_tx_data_packets;

    vapstats->rx_micerr       = ucaststats->ims_rx_tkipmic +
                                ucaststats->ims_rx_ccmpmic +
                                ucaststats->ims_rx_wpimic  +
                                mcaststats->ims_rx_tkipmic +
                                mcaststats->ims_rx_ccmpmic +
                                mcaststats->ims_rx_wpimic;

    vapstats->rx_decrypterr   = ucaststats->ims_rx_decryptcrc + 
                                mcaststats->ims_rx_decryptcrc;

    /* There are various definitions possible for Rx error count.
       We use the pre-existing, standard definition for 
       compatibility. */
    vapstats->rx_err          = stats->is_rx_tooshort +
                                stats->is_rx_decap +
                                stats->is_rx_nobuf +
                                vapstats->rx_decrypterr +
                                vapstats->rx_micerr +
                                ucaststats->ims_rx_tkipicv +
                                mcaststats->ims_rx_tkipicv +
                                ucaststats->ims_rx_wepfail +
                                mcaststats->ims_rx_wepfail;

    vapstats->tx_discard      = ucaststats->ims_tx_discard +
                                mcaststats->ims_tx_discard;

#if UMAC_SUPPORT_CHANUTIL_MEASUREMENT 
    vapstats->chan_util_enab  = get_vap_priv_int_param(
                                              sockfd,
                                              vapstats->ifname,
                                              IEEE80211_PARAM_CHAN_UTIL_ENAB);

    if (vapstats->chan_util_enab) {
        vapstats->chan_util       = get_vap_priv_int_param(
                                                   sockfd,
                                                   vapstats->ifname,
                                                   IEEE80211_PARAM_CHAN_UTIL);
    }
#else 
    vapstats->chan_util_enab = 0;
#endif /* UMAC_SUPPORT_CHANUTIL_MEASUREMENT */

    free(stats);

    /* Gather Node level stats. */

    curr_ns_ptr = (nodelevel_stats_t*)vapstats->firstchild;
    
    while(curr_ns_ptr) {
        nodelevel_gather_stats(sockfd, curr_ns_ptr);

        vapstats->txrx_rate_available = 1;
        txrx_rate_denomntr++;
        tx_rate_numrtr += curr_ns_ptr->tx_rate;
        rx_rate_numrtr += curr_ns_ptr->rx_rate;

        curr_ns_ptr = curr_ns_ptr->next;
    }


    if (vapstats->firstchild != NULL) {
        if (vapstats->txrx_rate_available) {
            vapstats->tx_rate = tx_rate_numrtr/txrx_rate_denomntr;
            vapstats->rx_rate = rx_rate_numrtr/txrx_rate_denomntr;
        }

        /* Other averaging goes here. */
    }
}

static int vaplevel_display_stats(vaplevel_stats_t *vapstats,
                                  apstats_recursion_t recursion)
{
    nodelevel_stats_t *curr_ns_ptr = NULL;

    assert(vapstats != NULL);
    
    APSTATS_PRINT_GENERAL("VAP Level Stats: %s (under radio %s)\n",
                          vapstats->ifname,
                          ((radiolevel_stats_t*)vapstats->parent)->ifname);
    APSTATS_PRINT_STAT64("Tx Data Packets",       vapstats->tx_data_packets);
    APSTATS_PRINT_STAT64("Tx Data Bytes",         vapstats->tx_data_bytes);
    APSTATS_PRINT_STAT64("Tx Data Payload Bytes", vapstats->tx_datapyld_bytes);
    APSTATS_PRINT_STAT64("Rx Data Packets",       vapstats->rx_data_packets);
    APSTATS_PRINT_STAT64("Rx Data Bytes",         vapstats->rx_data_bytes);
    APSTATS_PRINT_STAT64("Rx Data Payload Bytes", vapstats->rx_datapyld_bytes);
    APSTATS_PRINT_STAT64("Tx Unicast Data Packets",
            vapstats->tx_ucast_data_packets);
    APSTATS_PRINT_STAT64("Tx Multi/Broadcast Data Packets",
            vapstats->tx_mbcast_data_packets);

    if (vapstats->txrx_rate_available) {
        APSTATS_PRINT_STAT32("Average Tx Rate (kbps)",vapstats->tx_rate);
        APSTATS_PRINT_STAT32("Average Rx Rate (kbps)",vapstats->rx_rate);
    } else {
        APSTATS_PRINT_STAT_UNVLBL("Average Tx Rate (kbps)", "<NO STA>");
        APSTATS_PRINT_STAT_UNVLBL("Average Rx Rate (kbps)", "<NO STA>");
    }

    APSTATS_PRINT_STAT64("Rx errors",             vapstats->rx_err);
    APSTATS_PRINT_STAT64("Tx failures",           vapstats->tx_discard);
    APSTATS_PRINT_GENERAL("\n");

   if (recursion == APSTATS_RECURSION_NO_VAL ||
        recursion == APSTATS_RECURSION_FULLY_DISABLED) {
        return 0;
    } 

   curr_ns_ptr = (nodelevel_stats_t*)vapstats->firstchild;

   while(curr_ns_ptr) {
       nodelevel_display_stats(curr_ns_ptr);
       curr_ns_ptr = curr_ns_ptr->next;
   }
}

static int nodelevel_gather_stats(int sockfd,
                                  nodelevel_stats_t *nodestats)
{
    struct ifreq ifr;
    struct iwreq iwr;
    struct ieee80211req_sta_stats stats = {0};
    const struct ieee80211_nodestats *ns = &stats.is_stats;
    struct ieee80211req_sta_info *si = &nodestats->si;
    
    assert(nodestats != NULL);
    
    memset(&iwr, 0, sizeof(iwr));
    strncpy(iwr.ifr_name,
            ((vaplevel_stats_t*)nodestats->parent)->ifname,
            sizeof(iwr.ifr_name));
    iwr.u.data.pointer = (void *)&stats;
    iwr.u.data.length = sizeof(stats);
    memcpy(stats.is_u.macaddr,
           nodestats->macaddr.ether_addr_octet,
           IEEE80211_ADDR_LEN);
    
    if (ioctl(sockfd, IEEE80211_IOCTL_STA_STATS, &iwr) < 0) {
        perror("IEEE80211_IOCTL_STA_STATS");
        return -1;
    }

#if UMAC_SUPPORT_TXDATAPATH_NODESTATS
    nodestats->tx_data_packets  = ns->ns_tx_data_success;
    nodestats->tx_data_bytes    = ns->ns_tx_bytes_success;
#endif /* UMAC_SUPPORT_TXDATAPATH_NODESTATS */
    nodestats->rx_data_packets  = ns->ns_rx_data;
    nodestats->rx_data_bytes    = ns->ns_rx_bytes;
    
    nodestats->tx_ucast_data_packets  = ns->ns_tx_ucast;
    
    /* As an optimization, ieee80211req_sta_info has already
       been copied into nodestats after the IEEE80211_IOCTL_STA_INFO
       ioctl call in nodestats_hierarchy_init()*/ 
    
    if (si->isi_txratekbps == 0) {
           nodestats->tx_rate = ((si->isi_rates[si->isi_txrate] &
                                 IEEE80211_RATE_VAL) * 1000) / 2;
    }
    else {
           nodestats->tx_rate = si->isi_txratekbps;
    }

    nodestats->rx_rate          = si->isi_rxratekbps;

    nodestats->rx_micerr        = ns->ns_rx_tkipmic +
                                  ns->ns_rx_ccmpmic +
                                  ns->ns_rx_wpimic;

    nodestats->rx_decrypterr    = ns->ns_rx_decryptcrc;

    /* There are various definitions possible for Rx error count.
       We use the pre-existing, standard definition for VAP stats,
       and remove some of the components which do not apply. */
    nodestats->rx_err           = ns->ns_rx_decap +
                                  ns->ns_rx_decryptcrc +
                                  nodestats->rx_micerr +
                                  ns->ns_rx_tkipicv +
                                  ns->ns_rx_wepfail;

#if UMAC_SUPPORT_TXDATAPATH_NODESTATS
    nodestats->tx_discard       = ns->ns_tx_discard;
#endif /* UMAC_SUPPORT_TXDATAPATH_NODESTATS */

    nodestats->rx_rssi          = si->isi_rssi;
    
}

static char* macaddr_to_str(const struct ether_addr *addr)
{
    /* Similar to ether_ntoa, but use a prettier format
       where leading zeros are not discarded */
    static char string[3 * ETHER_ADDR_LEN];

    memset(string, 0, sizeof(string));
    
    if (addr != NULL) {

        snprintf(string,
                 sizeof(string),
                 "%02x:%02x:%02x:%02x:%02x:%02x",
                 addr->ether_addr_octet[0],
                 addr->ether_addr_octet[1],
                 addr->ether_addr_octet[2],
                 addr->ether_addr_octet[3],
                 addr->ether_addr_octet[4],
                 addr->ether_addr_octet[5]);
    }

    return string;
}

static int nodelevel_display_stats(nodelevel_stats_t *nodestats)
{
    assert(nodestats != NULL);
    
    APSTATS_PRINT_GENERAL("Node Level Stats: %s (under VAP %s)\n",
                          macaddr_to_str(&nodestats->macaddr),
                          ((vaplevel_stats_t*)nodestats->parent)->ifname);
#if UMAC_SUPPORT_TXDATAPATH_NODESTATS
    APSTATS_PRINT_STAT64("Tx Data Packets",       nodestats->tx_data_packets);
    APSTATS_PRINT_STAT64("Tx Data Bytes",         nodestats->tx_data_bytes);
#else
    APSTATS_PRINT_STAT_UNVLBL("Tx Data Packets",  APSTATS_TXDPATH_NODESTATS_UNVLBL_MSG);
    APSTATS_PRINT_STAT_UNVLBL("Tx Data Bytes",    APSTATS_TXDPATH_NODESTATS_UNVLBL_MSG);
#endif /* UMAC_SUPPORT_TXDATAPATH_NODESTATS */
    APSTATS_PRINT_STAT64("Rx Data Packets",       nodestats->rx_data_packets);
    APSTATS_PRINT_STAT64("Rx Data Bytes",         nodestats->rx_data_bytes);
    APSTATS_PRINT_STAT64("Tx Unicast Data Packets",
            nodestats->tx_ucast_data_packets);
    APSTATS_PRINT_STAT32("Average Tx Rate (kbps)",nodestats->tx_rate);
    APSTATS_PRINT_STAT32("Average Rx Rate (kbps)",nodestats->rx_rate);
    APSTATS_PRINT_STAT64("Rx MIC Errors",         nodestats->rx_micerr);
    APSTATS_PRINT_STAT64("Rx Decryption errors",  nodestats->rx_decrypterr);
    APSTATS_PRINT_STAT64("Rx errors",             nodestats->rx_err);
#if UMAC_SUPPORT_TXDATAPATH_NODESTATS
    APSTATS_PRINT_STAT64("Tx failures",           nodestats->tx_discard);
#else
    APSTATS_PRINT_STAT_UNVLBL("Tx failures",      APSTATS_TXDPATH_NODESTATS_UNVLBL_MSG);
#endif /* UMAC_SUPPORT_TXDATAPATH_NODESTATS */
    APSTATS_PRINT_STAT8("Rx RSSI",                nodestats->rx_rssi);
    APSTATS_PRINT_GENERAL("\n");

}

static int aplevel_handler(int sockfd,
                           aplevel_stats_t *apstats,
                           apstats_recursion_t recursion)
{
    int ret = -1;

    ret =  aplevel_gather_stats(sockfd, apstats, recursion);

    if (ret < 0) {
        return ret;
    }
    
    ret =  aplevel_display_stats(apstats, recursion);

    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int radiolevel_handler(int sockfd,
                              radiolevel_stats_t *radiostats,
                              apstats_recursion_t recursion)
{
    int ret = -1;

    ret =  radiolevel_gather_stats(sockfd, radiostats, recursion);

    if (ret < 0) {
        return ret;
    }
    
    ret =  radiolevel_display_stats(radiostats, recursion);

    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int vaplevel_handler(int sockfd,
                            vaplevel_stats_t *vapstats,
                            apstats_recursion_t recursion)
{
    int ret = -1;

    ret =  vaplevel_gather_stats(sockfd, vapstats, recursion);

    if (ret < 0) {
        return ret;
    }
    
    ret =  vaplevel_display_stats(vapstats, recursion);

    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int nodelevel_handler(int sockfd,
                             nodelevel_stats_t *nodestats)
{
    int ret = -1;

    ret =  nodelevel_gather_stats(sockfd, nodestats);

    if (ret < 0) {
        return ret;
    }
    
    ret =  nodelevel_display_stats(nodestats);

    if (ret < 0) {
        return ret;
    }

    return 0;

}


static int apstats_main(apstats_config_t *config)
{
    int ret = -1;
    int sockfd;
    aplevel_stats_t apstats;
    radiolevel_stats_t *pradiostats;
    vaplevel_stats_t *pvapstats;
    nodelevel_stats_t *pnodestats;

    assert(config != NULL);
   
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    /* Irrespective of the level requested by the user, we 
       initiate the whole hierarchy of stats. This is because
       there could be interdependencies among levels while
       calculating stats */
    if ((ret = stats_hierachy_init(&apstats, sockfd)) < 0) {
        /* TODO: See if we can handle this better. */
        stats_hierachy_deinit(&apstats); 
        return ret;
    }
    
    switch(config->level)
    {
        case APSTATS_LEVEL_AP:
            ret = aplevel_handler(sockfd,
                                  &apstats,
                                  config->recursion);
            break;

        case APSTATS_LEVEL_RADIO:
            pradiostats = stats_hierarchy_findradio(&apstats,
                                                    config->ifname);
            if (!pradiostats) {
                APSTATS_ERROR("Could not find specified radio.\n");
                ret = -ENOENT;
                break;
            }

            ret= radiolevel_handler(sockfd,
                                    pradiostats,
                                    config->recursion);
            break;

        case APSTATS_LEVEL_VAP:
            pvapstats = stats_hierarchy_findvap(&apstats,
                                                config->ifname);
            
            if (!pvapstats) {
                APSTATS_ERROR("Could not find specified VAP.\n");
                ret = -ENOENT;
                break;
            } 
            
            ret = vaplevel_handler(sockfd,
                                   pvapstats,
                                   config->recursion);
            break;

        case APSTATS_LEVEL_NODE:
            pnodestats = stats_hierarchy_findnode(&apstats,
                                                  &config->stamacaddr);
            
            if (!pnodestats) {
                APSTATS_ERROR("Could not find specified STA.\n");
                ret = -ENOENT;
                break;
            } 
            
            ret = nodelevel_handler(sockfd,
                                    pnodestats);
            break;

        default:
            APSTATS_ERROR("Unknown statistics level.\n");
            ret = -ENOENT;
            break;
    }

    stats_hierachy_deinit(&apstats); 
    close(sockfd);
    return ret;
}


int apstats_config_init(apstats_config_t *config)
{
    assert(config != NULL);
 
    /* Do NOT change the default to APSTATS_LEVEL_NO_VAL - doesn't 
       make sense, and will break a lot of stuff */
    config->level           = APSTATS_LEVEL_AP;
    config->recursion       = APSTATS_RECURSION_FULLY_DISABLED;

    memset(config->ifname, 0, sizeof(config->ifname));
    memset(&config->stamacaddr, 0, sizeof(config->stamacaddr));

    config->is_initialized  = 1;

    return 0;
}


int apstats_config_populate(apstats_config_t *config,
                            const apstats_level_t level,
                            const apstats_recursion_t recursion,
                            const char *ifname,
                            const char *stamacaddr)
{
    struct ether_addr *ret_ether_addr = NULL;
    apstats_recursion_t req_recursion;
    
    assert(config != NULL);
    assert(ifname != NULL);
    assert(stamacaddr != NULL);
    assert(1 == config->is_initialized);

    if (level != APSTATS_LEVEL_NO_VAL) {
        config->level = level;
    } /* else use defaults pre-configured by apstats_config_init() */
    
    if (recursion != APSTATS_RECURSION_NO_VAL) {
        config->recursion = recursion;
    } /* else use defaults pre-configured by apstats_config_init() */
    
    switch(config->level)
    {
        case APSTATS_LEVEL_AP:
            if (ifname[0]) {
                APSTATS_WARNING("Ignoring Interface Name input for AP "
                                "Level.\n");
            }
            
            if (stamacaddr[0]) {
                APSTATS_WARNING("Ignoring STA MAC address input for AP "
                                "Level.\n");
            }
            
            break;

        case APSTATS_LEVEL_RADIO:
            if (!ifname[0]) {
                APSTATS_ERROR("Radio Interface name not configured.\n");
                return -EINVAL;
            }
            
            if (!is_radio_ifname_valid(ifname)) {
                APSTATS_ERROR("Radio Interface name invalid.\n");
                return -EINVAL;
            }

            strncpy(config->ifname, ifname, IFNAMSIZ);

            if (stamacaddr[0]) {
                APSTATS_WARNING("Ignoring STA MAC address input for Radio "
                                "Level.\n");
            }

            break;
        
        case APSTATS_LEVEL_VAP:
            if (!ifname[0]) {
                APSTATS_ERROR("VAP Interface name not configured.\n");
                return -EINVAL;
            }
            
            if (!is_vap_ifname_valid(ifname)) {
                APSTATS_ERROR("VAP Interface name invalid.\n");
                return -EINVAL;
            }

            strncpy(config->ifname, ifname, IFNAMSIZ);

            if (stamacaddr[0]) {
                APSTATS_WARNING("Ignoring STA MAC address input for VAP "
                                "Level.\n");
            }

            break;

        case APSTATS_LEVEL_NODE:
            if (!stamacaddr[0]) {
                APSTATS_ERROR("STA MAC address not configured.\n");
                return -EINVAL;
            }

            if ((ret_ether_addr = ether_aton_r(stamacaddr,
                                               &config->stamacaddr)) == NULL) {
                APSTATS_ERROR("STA MAC address not valid.\n");
                return -EINVAL; 
            }

            config->recursion = APSTATS_RECURSION_FULLY_DISABLED;

            if (ifname[0]) {
                APSTATS_WARNING("Ignoring Interface name input for STA "
                                "Level.\n");
            }
            
            if (recursion != APSTATS_RECURSION_NO_VAL) {
                APSTATS_WARNING("Ignoring recursive display configuration "
                                "for STA level.\n");
            }

            break;

        default:
            APSTATS_ERROR("Unknown statistics level.\n");
            return -EINVAL;
            break;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int                 ret;
    apstats_config_t    config;
    int                 opt = 0;
    struct ifreq        ifr;
    int                 longIndex;
    int                 err;

    int                 is_level_set        = 0;
    int                 is_ifname_set       = 0;
    int                 is_stamacaddr_set   = 0;
    int                 is_option_selected  = 0;

    /* Temp variables to hold user input. */
    apstats_level_t     level_temp          = APSTATS_LEVEL_NO_VAL;
    apstats_recursion_t recursion_temp      = APSTATS_RECURSION_NO_VAL;
    char                ifname_temp[IFNAMSIZ];
    char                stamacaddr_temp[ETH_ALEN * 3];
    
    memset(ifname_temp, 0, sizeof(ifname_temp));
    memset(stamacaddr_temp, 0, sizeof(stamacaddr_temp));
    
    memset(&config, 0, sizeof(config));
    apstats_config_init(&config);
    
    opt = getopt_long(argc, argv, optString, longOpts, &longIndex);

    /* TODO: Auto detect required level from interface name/MAC
       address input, so as to make the UI a little more
       friendly. */

    while (opt != -1) {
        switch(opt) {
            case 'a':
                if (is_level_set) {
                    APSTATS_ERROR("Multiple statistics level arguments "
                                  "detected.\n");
                    display_help();
                    return -1;
                }

                level_temp = APSTATS_LEVEL_AP;
                is_level_set  = 1;
                is_option_selected = 1;
                break;
            
            case 'r':
                if (is_level_set) {
                    APSTATS_ERROR("Multiple statistics level arguments"
                                  "detected.\n");
                    display_help();
                    return -1;
                }

                level_temp = APSTATS_LEVEL_RADIO;
                is_level_set  = 1;
                is_option_selected = 1;
                break;
            
            case 'v':
                if (is_level_set) {
                    APSTATS_ERROR("Multiple statistics level arguments "
                                  "detected.\n");
                    display_help();
                    return -1;
                }

                level_temp = APSTATS_LEVEL_VAP;
                is_level_set  = 1;
                is_option_selected = 1;
                break;

             case 's':
                if (is_level_set) {
                    APSTATS_ERROR("Multiple statistics level arguments "
                                  "detected.\n");
                    display_help();
                    return -1;
                }

                level_temp = APSTATS_LEVEL_NODE;
                is_level_set  = 1;
                is_option_selected = 1;
                break;

            case 'i':
                if (is_ifname_set) {
                    APSTATS_ERROR("Multiple interface name arguments "
                                  "detected.\n");
                    display_help();
                    return -1;
                }

                strncpy(ifname_temp, optarg, sizeof(ifname_temp));
                is_ifname_set  = 1;
                is_option_selected = 1;
                break;
            
            case 'm':
                if (is_stamacaddr_set) {
                    APSTATS_ERROR("Multiple STA MAC address arguments "
                                  "detected.\n");
                    display_help();
                    return -1;
                }

                strncpy(stamacaddr_temp, optarg, sizeof(stamacaddr_temp));
                is_stamacaddr_set  = 1;
                is_option_selected = 1;
                break;
            
            case 'R':
                recursion_temp = APSTATS_RECURSION_FULLY_ENABLED;
                break;
                
            case 'h':
            case '?':
                display_help();
                is_option_selected = 1;
                return 0;

            default:
                APSTATS_ERROR("Unrecognized option.\n");
                display_help();
                is_option_selected = 1;
                return -1;
        }
        
        opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
    }
 
    if (!is_option_selected) {
        APSTATS_MESG("No application recognized options. "
                     "Using defaults: AP level, non-recursive.\n"        
                     "Use -h for help\n\n");        
    }

    ret = apstats_config_populate(&config,
                                  level_temp,
                                  recursion_temp,
                                  ifname_temp,
                                  stamacaddr_temp);
    if (ret < 0) {
        display_help();
        return -1;
    }

    apstats_main(&config);

    return 0;
}
