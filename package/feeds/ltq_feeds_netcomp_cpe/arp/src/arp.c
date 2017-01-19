/* ============================================================= *
 * Copyright (C) 2010 LANTIQ Technologies AG.
 *
 * All rights reserved.
 * ============================================================= *
 * ============================================================= *
 * This document contains proprietary information belonging to LANTIQ 
 * Technologies AG. Passing on and copying of this document, and communication
 * of its contents is not permitted without prior written authorisation.
 * 
************************************************************************************************
**
** FILE NAME    : arp.c
** PROJECT      : UGW
** UTILITY      : ARP
**
** DATE         : 22 FEB 2010
** AUTHOR       : 
** COPYRIGHT    :       Copyright (c) 2010
**                      LANTIQ Technologies AG
**                      Germany
**
 * DESCRIPTION  : ARP Utility
 *  This file contains code that controls the ARP Table in the kernel. This
 *  utility allows ARP Entries to be created, deleted and displayed. This
 *  is based on the OPEN Source Nettools ARP daemon but has been customized
 *  for size.
 *
 *  CALL-INs    :
 *
 *  CALL-OUTs   :
 *
 *  User-Configurable Items:
 *
 *  (C) Copyright 2010, LANTIQ Technologies AG
 ***********************************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <linux/if_ether.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

/* The directory path where the kernel maintains the ARP Table. */
#define PATH_PROCNET_ARP   "/proc/net/arp"

/* The Version of ARP. */
char *Version = "NSP ARP Utility Version 1.0 (2006-02-06)";

int sockfd = 0;			    /* active socket descriptor     */
char device[16] = "";		/* current device               */

/**************************************************************************
 * FUNCTION NAME : arp_parse_ip_address
 **************************************************************************
 * DESCRIPTION   :
 *  The function parses the IP Address passed to the function and populates
 *  the sockaddress structure that has been passed to it.
 *
 * RETURNS       :
 *  0   -   Success
 *  -1  -   Error
 **************************************************************************/
static int arp_parse_ip_address(char *ip_address, struct sockaddr_in *sin)
{
    /* Initialize the fields inside the sock address structure. */
    sin->sin_family = AF_INET;
    sin->sin_port = 0;

    /* Get the IP address from the  */
    if (inet_aton(ip_address, &sin->sin_addr)) 
	    return 0;

    /* Error: Could not convert the IP Address. */
    return -1;
}

/**************************************************************************
 * FUNCTION NAME : arp_parse_mac_address
 **************************************************************************
 * DESCRIPTION   :
 *  The function parses the MAC Address passed to the function and populates
 *  the sockaddress structure that has been passed to it.
 *
 * RETURNS       :
 *  0   -   Success
 *  -1  -   Error
 **************************************************************************/
static int arp_parse_mac_address(char *bufp, struct sockaddr *sap)
{
    unsigned char *ptr;
    char c, *orig;
    int i;
    unsigned val;

    /* Populate the structure. */
    sap->sa_family = ARPHRD_ETHER;

    /* Get the pointer to the data. */
    ptr = sap->sa_data;

    i = 0;
    orig = bufp;
    while ((*bufp != '\0') && (i < ETH_ALEN)) 
    {
        /* Initialize the value. */
    	val = 0;

        /* Get the first character. */
	    c = *bufp++;
    	if (isdigit(c))
	        val = c - '0';
    	else if (c >= 'a' && c <= 'f')
	        val = c - 'a' + 10;
    	else if (c >= 'A' && c <= 'F')
	        val = c - 'A' + 10;
    	else 
	        return (-1);

    	val <<= 4;

        /* Get the second character. */
	    c = *bufp;
    	if (isdigit(c))
	        val |= c - '0';
    	else if (c >= 'a' && c <= 'f')
	        val |= c - 'a' + 10;
    	else if (c >= 'A' && c <= 'F')
	        val |= c - 'A' + 10;
    	else if (c == ':' || c == 0)
	        val >>= 4;
    	else 
	        return (-1);

        /* Have we reached the end of the string? */
    	if (c != 0)
	        bufp++;

        /* Store the computed value into the socket address structure. */
    	*ptr++ = (unsigned char) (val & 0xFF);

        /* Increment the counter. */
	    i++;

    	/* We might get a semicolon here - not required. */
	    if (*bufp == ':') 
    	    bufp++;
    }

    /* Succesfully parsed. */
    return (0);
}

/**************************************************************************
 * FUNCTION NAME : arp_del
 **************************************************************************
 * DESCRIPTION   :
 *  Delete an entry from the ARP cache.
 *
 * RETURNS       :
 *  0   -   Success
 *  -1  -   Error
 **************************************************************************/
static int arp_del(char **args)
{
    char            host[128];
    struct arpreq   req;
    struct sockaddr sa;
    int             err;

    memset((char *) &req, 0, sizeof(req));

    /* Resolve the host name. */
    if (*args == NULL) 
    {
    	printf ("ARP: Need to specify an IP Address.\n");
	    return -1;
    }

    /* Get the Host Name. */
    strncpy(host, *args, (sizeof host));

    /* Parse the IP Address and fill the socket address structure also. */
    if (arp_parse_ip_address (host, (struct sockaddr_in *)&sa) < 0)
    {
        printf ("Error: IP Address parse failed\n");
        return -1;
    }

    /* If a host has more than one address, use the correct one! */
    memcpy((char *) &req.arp_pa, (char *) &sa, sizeof(struct sockaddr));

    /* Specify the flags. */
    req.arp_flags = ATF_PERM;

    strcpy(req.arp_dev, device);

    /* Call the kernel to delete the ARP Entry.  */
	if ((err = ioctl(sockfd, SIOCDARP, &req) < 0)) 
    {
	    if (errno == ENXIO) 
        {
            req.arp_flags |= ATF_PUBL;
            if (ioctl(sockfd, SIOCDARP, &req) < 0) 
            {            
                printf("No ARP entry for %s\n", host);
                return (-1);
            }
	    }
	    perror("SIOCDARP(priv)");
	    return (-1);
	}

    /* The entry has been successfully deleted. */
    return 0;
}

/**************************************************************************
 * FUNCTION NAME : arp_set
 **************************************************************************
 * DESCRIPTION   :
 *  Set an entry in the ARP cache.
 *
 * RETURNS       :
 *  0   -   Success
 *  -1  -   Error
 **************************************************************************/
static int arp_set(char **args)
{
    char            host[128];
    struct arpreq   req;
    struct sockaddr sa;

    /* Initialize the request. */
    memset((char *) &req, 0, sizeof(req));

    /* We must have an IP Address! */
    if (*args == NULL) 
    {
        /* No IP Address specified. */
    	printf("Error: Need to specify an IP Address\n");
	    return -1;
    }

    /* Copy the IP Address. */
    strncpy(host, *args++, (sizeof host));

    /* Parse the IP Address and fill the socket address structure also. */
    if (arp_parse_ip_address (host, (struct sockaddr_in *)&sa) < 0)
    {
        printf ("Error: IP Address parse failed\n");
        return -1;
    }

    /* If a host has more than one address, use the correct one! */
    memcpy((char *) &req.arp_pa, (char *) &sa, sizeof(struct sockaddr));

    /* We must have the MAC Address. */
    if (*args == NULL) 
    {
	    printf("Error: Need hardware address\n");
    	return -1;
    }

    /* Pass the hardware address to the family. */
	if (arp_parse_mac_address(*args++, &req.arp_ha) < 0) 
    {
	    printf("Error: invalid hardware address\n");
	    return (-1);
	}

    /* Fill in the remainder of the request. */
    req.arp_flags = ATF_PERM | ATF_COM;
    strcpy(req.arp_dev, device);

    /* Call the kernel. */
    if (ioctl(sockfd, SIOCSARP, &req) < 0) 
    {
        perror("SIOCSARP");
	    return (-1);
    }
    return (0);
}

/**************************************************************************
 * FUNCTION NAME : arp_display
 **************************************************************************
 * DESCRIPTION   :
 *  Print the contents of an ARP request block.
 **************************************************************************/
static void arp_display(char *name, int type, int arp_flags, char *hwa, char *mask, char *dev)
{
    static int title = 0;
    char flags[10];

    if (title++ == 0) {
	printf("Address                  HWtype  HWaddress           Flags Mask            Iface\n");
    }
    /* Setup the flags. */
    flags[0] = '\0';
    if (arp_flags & ATF_COM)
	strcat(flags, "C");
    if (arp_flags & ATF_PERM)
	strcat(flags, "M");

    printf("%-23.23s  ", name);

    if (!(arp_flags & ATF_COM)) {
	if (arp_flags & ATF_PUBL)
	    printf("%-8.8s%-20.20s", "*", "*");
	else
	    printf("%-8.8s%-20.20s", "", "(incomplete)");
    } else {
	printf("%-8.8s%-20.20s", "ether", hwa);
    }

    printf("%-6.6s%-15.15s %s\n", flags, mask, dev);
}

/**************************************************************************
 * FUNCTION NAME : arp_show
 **************************************************************************
 * DESCRIPTION   :
 *  Display the contents of the ARP cache in the kernel.
 *
 * RETURNS       :
 *  0   -   Success
 *  -1  -   Error
 **************************************************************************/
static int arp_show(char *name)
{
    char ip[100];
    char hwa[100];
    char mask[100];
    char line[200];
    char dev[100];
    int type, flags;
    FILE *fp;

    /* Open the PROCps kernel table. */
    if ((fp = fopen(PATH_PROCNET_ARP, "r")) == NULL) 
    {
    	perror(PATH_PROCNET_ARP);
	    return (-1);
    }

    /* Bypass header -- read until newline */
    if (fgets(line, sizeof(line), fp) != (char *) NULL) 
    {
    	strcpy(mask, "-");
	    strcpy(dev, "-");

    	/* Read the ARP cache entries. */
	    for (; fgets(line, sizeof(line), fp);) 
        {
            /* Parse the line and get the values */
    	    int num = sscanf(line, "%s 0x%x 0x%x %100s %100s %100s\n", ip, &type, &flags, hwa, mask, dev);
    	    if (num < 4)
	        	break;

            /* Display the ARP Cache Entry. */
            arp_display(ip, type, flags, hwa, mask, dev);
	    }
    }

    /* Close the PROC File that was opened*/
    (void) fclose(fp);

    /* Work has been succesfully completed. */
    return (0);
}

/**************************************************************************
 * FUNCTION NAME : usage
 **************************************************************************
 * DESCRIPTION   :
 *  Prints the help screen that describes the various parameters that can
 *  be passed to the ARP Utility.
 **************************************************************************/
static void usage(void)
{
    printf ("Usage:\n");
    printf("  arp [-n]                      <-Display ARP cache\n");
    printf("  arp -d  <hostIP>              <-Delete ARP entry\n");
    printf("  arp -s  <hostIP> <hwaddr>     <-Add entry\n");
    printf("  arp -n                        <-Dump ARP Table\n");
    printf("    -s, --set                set a new ARP entry\n");
    printf("    -d, --delete             delete a specified entry\n");
    printf("    -n, --numeric            don't resolve names just list all entries\n");
}

/**************************************************************************
 * FUNCTION NAME : main
 **************************************************************************
 * DESCRIPTION   :
 *  The main entry point for the ARP Daemon.
 **************************************************************************/
int main(int argc, char **argv)
{
    int i, lop, what;
    struct option longopts[] =
    {
	{"delete", 0, 0, 'd'},
	{"numeric", 0, 0, 'n'},
	{"set", 0, 0, 's'},
	{"help", 0, 0, 'h'},
	{NULL, 0, 0, 0}
    };
   
    what = 0;

    /* Open the socket for all IOCTL operations. */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
    {
	    perror("socket");
    	exit(-1);
    }

    /* Fetch the command-line arguments. */
    while ((i = getopt_long(argc, argv, "A:H:adfp:nsei:t:vh?DNV", longopts, &lop)) != EOF)
    {
        /* Process each option specified on the command line. */
    	switch (i) 
        {
    	    case 'd':
            {
                /* Delete the ARP Entry. */
    	        what = arp_del(&argv[optind]);
	            break;
            }
        	case 's':
            {
                /* Add the ARP Entry. */
	            what = arp_set(&argv[optind]);
	            break;
            }
            case 'n':
            {
                /* List all the ARP Entries. */
                what = arp_show(argv[optind]);
                break;
            }
	        case 'V':
            {
                /* Print the version of the ARP Utility. */
    	        fprintf(stderr, "%s\n", Version);
                break;
            }
    	    case '?':
        	case 'h':
	        default:
            {
                /* Print the Help. */
        	    usage();
                break;
            }
	    }
    }

    /* Close the socket before exiting the program. */
    close(sockfd);

    /* Exit with the correct status code. */
    exit(what);
}

