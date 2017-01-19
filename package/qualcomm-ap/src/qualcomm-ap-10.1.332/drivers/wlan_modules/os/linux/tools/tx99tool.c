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
 
 /*
 * tx99tool set
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/if.h>
#include <linux/wireless.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <getopt.h>


/*
 * Linux uses __BIG_ENDIAN and __LITTLE_ENDIAN while BSD uses _foo
 * and an explicit _BYTE_ORDER.  Sorry, BSD got there first--define
 * things in the BSD way...
 */
#define	_LITTLE_ENDIAN	1234	/* LSB first: i386, vax */
#define	_BIG_ENDIAN	4321	/* MSB first: 68000, ibm, net */
#include <asm/byteorder.h>
#if defined(__LITTLE_ENDIAN)
#define	_BYTE_ORDER	_LITTLE_ENDIAN
#elif defined(__BIG_ENDIAN)
#define	_BYTE_ORDER	_BIG_ENDIAN
#else
#error "Please fix asm/byteorder.h"
#endif

/*
** Need to find proper references in UMAC code
*/

#include "ieee80211_external.h"
#include "tx99_wcmd.h"

static void usage(void);

int
main(int argc, char *argv[])
{
	const char *ifname, *cmd;
	tx99_wcmd_t i_req;
	int s;
	struct ifreq ifr;

	if (argc < 3)
		usage();

	ifname = argv[1];
	cmd = argv[2];
	
    (void) memset(&ifr, 0, sizeof(ifr));
    (void) strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    (void) memset(&i_req, 0, sizeof(i_req));
    (void) strncpy(i_req.if_name, ifname, IFNAMSIZ);

    /* enable */
	if(!strcasecmp(cmd, "start")) {
		if(argc > 3) {
			fprintf(stderr,"Too many arguments\n");
			fprintf(stderr,"usage: tx99tool wifi0 start\n");
			return -1;
		}
		i_req.type = TX99_WCMD_ENABLE;
	}	
	/* disable */
	else if(!strcasecmp(cmd, "stop")) {
		if(argc > 3) {
			fprintf(stderr,"Too many arguments\n");
			fprintf(stderr,"usage: tx99tool wifi0 stop\n");
			return -1;
		}
		i_req.type = TX99_WCMD_DISABLE;
	} 
	/* get */
	else if (!strcasecmp(cmd, "get")) {
	    if(argc > 3) {
			fprintf(stderr,"Too many arguments\n");
			fprintf(stderr,"usage: tx99tool wifi0 get\n");
			return -1;
		}
        i_req.type = TX99_WCMD_GET;
    } 
    /* set */
    else if (!strcasecmp(cmd, "set")) {
        if (argc < 4)
		    usage();
    
    	/* Tx frequency, bandwidth and extension channel offset */
    	if(!strcasecmp(argv[3], "freq")) {
    		if(argc != 7) {
    			fprintf(stderr,"Wrong arguments\n");
    			fprintf(stderr,"usage: tx99tool wifi0 set freq [freq] [bandwidth] [ext offset]\n");
    			return -1;
    		}
    		i_req.type = TX99_WCMD_SET_FREQ;
    		i_req.data.freq = atoi(argv[4]);
    		i_req.data.htmode = atoi(argv[5]);
    		i_req.data.htext = atoi(argv[6]);
    	}
    	/* rate - Kbits/s */
    	else if(!strcasecmp(argv[3], "rate")) {
    		if(argc != 5) {
    			fprintf(stderr,"Wrong arguments\n");
    			fprintf(stderr,"usage: tx99tool wifi0 set rate [Tx rate]\n");
    			return -1;
    		}
    		i_req.type = TX99_WCMD_SET_RATE;
    		i_req.data.rate = atoi(argv[4]);
    	}
    	/* Tx power */
    	else if(!strcasecmp(argv[3], "pwr")) {
    		if(argc != 5) {
    			fprintf(stderr,"Wrong arguments\n");
    			fprintf(stderr,"usage: tx99tool wifi0 set pwr [Tx pwr]\n");
    			return -1;
    		}
    		i_req.type = TX99_WCMD_SET_POWER;
    		i_req.data.power = atoi(argv[4]);
    	}
    	/* tx chain mask - 1, 2, 3 (for 2*2) */
    	else if(!strcasecmp(argv[3], "txchain")) {
    		if(argc != 5) {
    			fprintf(stderr,"Wrong arguments\n");
    			fprintf(stderr,"usage: tx99tool wifi0 set txchain [Tx chain mask]\n");
    			return -1;
    		}
    		i_req.type = TX99_WCMD_SET_CHANMASK;
    		i_req.data.chanmask = atoi(argv[4]);
    	}
    	/* Tx type - 0: data modulated, 1:single carrier */
    	else if(!strcasecmp(argv[3], "type")) {
    		if(argc != 5) {
    			fprintf(stderr,"Wrong arguments\n");
    			fprintf(stderr,"usage: tx99tool wifi0 set type [Tx type]\n");
    			return -1;
    		}
    		i_req.type = TX99_WCMD_SET_TYPE;
    		i_req.data.type = atoi(argv[4]);
    	}
        else if(!strcasecmp(argv[3], "txmode")) {
            if(argc != 5) {
                fprintf(stderr,"Wrong arguements \n");
                fprintf(stderr,"usage: tx99tool wifi0 set txmode [1: 11A 2: 11B 3: 11G 7: 11NAHT20 8: 11NGHT20 9: 11NAHT40PLUS 10: 11NAHT40MINUS 11: 11NGHT40PLUS 12: 11NGHT40MINUS 13: 11NGHT40 14: 11NAHT40 ] \n");
                return -1;
            }
            i_req.type = TX99_WCMD_SET_TXMODE;
            i_req.data.txmode = atoi(argv[4]);
        }
    	else 
        	usage();

    } else
        usage();
    
    ifr.ifr_data = (void *) &i_req;
    
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		err(1, "socket(SOCK_DRAGM)");
	
    if (ioctl(s, SIOCIOCTLTX99, &ifr) < 0)
		err(1, "ioctl");


    return 0;
}

static void
usage(void)
{
	fprintf(stderr, "usage: tx99tool wifi0 [start|stop|set]\n");
	fprintf(stderr, "usage: tx99tool wifi0 set [freq|rate|txchain|type|pwr|txmode]\n");
	exit(-1);
}

