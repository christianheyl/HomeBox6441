/******************************************************************************
**
** FILE NAME    : ifx_pmon_test.c
** PROJECT      : IFX UEIP
** MODULES      : PMON
**
** DATE         : 21 July 2009
** AUTHOR       : Lei Chuanhua
** DESCRIPTION  : IFX Performance Monitor Interface Test files
** COPYRIGHT    :       Copyright (c) 2009
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author         $Comment
** 21 July 2009  Lei Chuanhua    The first UEIP release
** 29 Mar  2012  Kishore Kankipati Modified so that device would be created automatically
				   if it doesn't exist. No need to execute separate
				   command from console.
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

//#include "../../../bsp_basic_drv/src/include/ifx_pmon.h"
#include <asm/ifx/ifx_pmon.h>

#define IFX_PMON_TEST_VER  "2.0.1"

#define N(X)  (sizeof(X) /sizeof((X)[0]))

#define PMON_DEVICE "/dev/ifx_pmon"
#define PMON_MAJOR 10
#define PMON_MINOR 245


static void pmon_help(void)
{
    printf("Usage:pmon [options] [parameter] ...\n");
    printf("    options:\n");
    printf("    --help,      -h   Display help information\n"); 
    printf("    --version,   -v   Display PMON version information\n");
    printf("    --list,      -l   List all events and event source\n");
    printf("    --disable,   -d   Disable All PMON interfaces\n");     
    printf("    --event,     -e        	 \n");
    printf("        0 ~ 17        Event number\n");
    printf("    --tc,        -t        	 \n");
    printf("        0 ~ 3         TC number\n");    
    printf("    --counter,   -c         \n");
    printf("        0 ~ 1         performance counter 0,1 per TC\n"); 
    printf("IFX PMON test version %s by Chuanhua.lei@infineon.com\n", IFX_PMON_TEST_VER);
}

typedef struct ifx_pmon_event2str {
    unsigned int idx;
    unsigned int event;
    const unsigned char str[32];
}ifx_pmon_event2str_t;

typedef struct ifx_pmon_tc_counter_event { 
    unsigned int event;
    const unsigned char event_name[64];
}ifx_pmon_tc_counter_event_t;

#if defined(CONFIG_VR9) || defined(CONFIG_HN1)
static ifx_pmon_event2str_t event_to_str[] = {
    {0, IFX_PMON_EVENT_NONE, "none"},
    {1, IFX_PMON_EVENT_DDR_READ, "DDR read"},
    {2, IFX_PMON_EVENT_DDR_WRITE, "DDR write"},
    {3, IFX_PMON_EVENT_DDR_MASK_WRITE, "DDR mask write"},
    {4, IFX_PMON_EVENT_DDR_ONE_WORD_64BIT_READ, "DDR burst1 read"},
    {5, IFX_PMON_EVENT_DDR_TWO_WORD_64BIT_READ, "DDR burst2 read"},
    {6, IFX_PMON_EVENT_DDR_FOUR_WORD_64BIT_READ, "DDR burst4 read"},    
    {7, IFX_PMON_EVENT_DDR_EIGHT_WORD_64BIT_READ,"DDR burst8 read"},
    {8, IFX_PMON_EVENT_DDR_ONE_WORD_64BIT_WRITE, "DDR burst1 write"},
    {9, IFX_PMON_EVENT_DDR_TWO_WORD_64BIT_WRITE, "DDR burst2 write"},
    {10, IFX_PMON_EVENT_DDR_FOUR_WORD_64BIT_WRITE,"DDR burst4 write"},
    {11, IFX_PMON_EVENT_DDR_EIGHT_WORD_64BIT_WIRTE,"DDR burst8 wirte"},
    {12, IFX_PMON_EVENT_AHB_READ_CYCLES,    "AHB read cycles"},
    {13, IFX_PMON_EVENT_AHB_READ_CPT,     "AHB read complete"},
    {14, IFX_PMON_EVENT_AHB_WRITE_CYCLES, "AHB write cycles"},
    {15, IFX_PMON_EVENT_AHB_WRITE_CPT,    "AHB write complete"},
    {16, IFX_PMON_EVENT_DMA_RX_BLOCK_CNT, "DMA rx block counter"},
    {17, IFX_PMON_EVENT_DMA_TX_BLOCK_CNT, "DMA tx block counter"},
};
#elif defined CONFIG_AR10
static ifx_pmon_event2str_t event_to_str[] = {
    {0, IFX_PMON_EVENT_NONE, "none"},
    {1, IFX_PMON_BIU0_READ_EVENT, "BIU0 read"},
    {2, IFX_PMON_BIU0_WRITE_EVENT, "BIU0 write"},
    {3, IFX_PMON_BIU1_READ_EVENT, "BIU1 read"},
    {4, IFX_PMON_BIU1_WRITE_EVENT, "BIU1 write"},
    {5, IFX_PMON_DMA_READ_EVENT, "DMA read"},
    {6, IFX_PMON_DMA_WRITE_EVENT, "DMA write"},
    {7, IFX_PMON_DMA_RX_EVENT, "DMA RX"},
    {9, IFX_PMON_DMA_TX_EVENT, "DMA TX"},    
    {9, IFX_PMON_FPI1S_READ_EVENT,"FPI1 Slave read"},
    {10, IFX_PMON_FPI1S_WRITE_EVENT, "FPI1 Slave write"},
    {11, IFX_PMON_AHB1S_READ_EVENT, "AHB1 Slave read"},
    {12, IFX_PMON_AHB1S_WRITE_EVENT, "AHB1 Slave write"},
    {13, IFX_PMON_AHB2S_READ_EVENT, "AHB2 Slave read"},
    {14, IFX_PMON_AHB2S_WRITE_EVENT, "AHB2 Slave write"},
    {15, IFX_PMON_AHB4S_READ_EVENT,  "AHB4 Slave read"},
    {16, IFX_PMON_AHB4S_WRITE_EVENT, "AHB4 Slave write"},
    {17, IFX_PMON_DDR_READ_EVENT,    "DDR single read"},
    {18, IFX_PMON_DDR_WRITE_EVENT, "DDR single write"},
    {19, IFX_PMON_DDR_CMD_QUEUE_ALMOST_FULL, "DDR cmd queue almost full"},
    {20, IFX_PMON_DDR_CKE_STAT,"DDR CKE state"},
    {21, IFX_PMON_DDR_REFRESH_IN_PROGRESS, "DDR refresh in progress"},
    {22, IFX_PMON_DDR_CONTROLLER_BUSY, "DDR controller busy"},
    {23, IFX_PMON_DDR_CMD_QUEUE_FULL, "DDR cmd queue full"},
    {24, IFX_PMON_SRAM_READ_EVENT, "SRAM read"},
    {25, IFX_PMON_SRAM_WRITE_EVENT, "SRAM write"},
    {26, IFX_PMON_FPI2M_READ_EVENT,  "FPI2 master read"},
    {27, IFX_PMON_FPI2M_WRITE_EVENT, "FPI2 master write"},
    {28, IFX_PMON_FPI3M_READ_EVENT,  "FPI3 master read"},
    {29, IFX_PMON_FPI3M_WRITE_EVENT, "FPI3 master write"},
    {30, IFX_PMON_AHB3M_READ_EVENT,  "AHB3 master read"},
    {31, IFX_PMON_AHB3M_WRITE_EVENT, "AHB3 master write"}, 
};
#else
#error "No platform defined"
#endif

#if defined(CONFIG_VR9) || defined(CONFIG_HN1)

#define IFX_MIPS_MAX_TC (IFX_MIPS_TC1 + 1)
static ifx_pmon_tc_counter_event_t tc_counter_event_str[IFX_MIPS_TC1+1][IFX_PMON_MAX_PERF_CNT_PER_TC] = {
    {
    { 21, "Performance Counter 0 event 21-L2WB"},
    { 21, "Performance Counter 1 event 21-L2ACC"},
    },
    {
    { 22, "Performance Counter 0 event 22-L2CACHMISS"},
    { 22, "Performance Counter 1 event 38-L2DATAMISS"},
    },
};
#else

#define IFX_MIPS_MAX_TC (IFX_MIPS_TC3 + 1)
static ifx_pmon_tc_counter_event_t tc_counter_event_str[IFX_MIPS_TC3+1][IFX_PMON_MAX_PERF_CNT_PER_TC] = {
    {
    { 64, "Performance Counter 0 event system specific"},
    { 64, "Performance Counter 1 event system specific"},
    },
    {
    { 65, "Performance Counter 0 event system specific"},
    { 65, "Performance Counter 1 event system specific"},
    },
    {
    { 66, "Performance Counter 0 event system specific"},
    { 66, "Performance Counter 1 event system specific"},
    },
    {
    { 67, "Performance Counter 0 event system specific"},
    { 67, "Performance Counter 1 event system specific"},
    },    
};
#endif

int open_pmon_device()
{
    int fd = -1;
    if (access(PMON_DEVICE, R_OK|W_OK) != 0) {
    	if (mknod(PMON_DEVICE, S_IFCHR+0644, makedev(PMON_MAJOR, PMON_MINOR)) != 0) {
	   perror("mknod");
	   exit(-1);
	}
    }
    if ((fd = open(PMON_DEVICE, O_RDWR)) < 0) {
    	perror("open");
	fprintf(stderr,"Can't find PMON device %s\n", PMON_DEVICE);
	exit(-1);
    }
    return fd;
}

int main (int argc, char** argv) 
{
    int i, j;
    int c;
    int fd;
    int finished = 0;
    int event = 0;
    int counter = 0;
    int value = 0;
    int tc = 0;
        
    if ( argc < 2 ) {
        pmon_help();
        return -1;
    }
    
    fd = open_pmon_device();
    /*
    fd = open("/dev/ifx_pmon",O_RDWR);
    if(fd == -1) {
        printf("cannot find PMON module, check /dev/ifx_pmon setting!\n");
        exit(-1);
    }
    */ 
    while (1) {
        const static char optstring[] = "hvlde:t:c:";    
        const static struct option longopts[] ={
            {"help",    0, NULL, 'h'},
            {"version", 0, NULL, 'v'},
            {"list", 0, NULL, 'l'},            
            {"disable", 0, NULL, 'd'},
            /* These options don't set a flag. We distinguish them by their indices. */
            {"event",      1, NULL, 'e'},
            {"tc",         1, NULL, 't'},
            {"counter",     1, NULL, 'c'},
            {NULL,      0, NULL, 0},        
        }; 
        /* getopt_long stores the option index here. */
        int option_index = 0;
        
        c = getopt_long(argc, argv, optstring, longopts, &option_index);
        /* Detect the end of the options. */
        if (c == -1) {
            break;
        }
        switch ( c ) {
            case 'h':
                pmon_help();
                finished = 1;
                break;

            case 'v':
                {
                    struct ifx_pmon_ioctl_version version = {0};
                    if (ioctl(fd, IFX_PMON_IOC_VERSION, &version) == -1) {
                        printf("Failed to get PMON version\n");
                    } 
                    printf("PMON driver version %d.%d.%d\n", version.major, version.mid, version.minor); 
                    
                }
                finished = 1;      
                break;

            case 'l':
                printf("PMON Event list:\n");
                for (i = 0; i < N(event_to_str); i++) {
                    printf("id %d event[%d] {%s}\n", i, event_to_str[i].event, event_to_str[i].str);
                }
                printf("PMON TC event performance counter list:\n");
                for (i = 0; i < IFX_MIPS_MAX_TC; i++) {
                    for (j = 0; j < IFX_PMON_MAX_PERF_CNT_PER_TC; j++) {
                        printf("TC%d Counter %d event %d {%s}\n", 
                            i, j, tc_counter_event_str[i][j].event, tc_counter_event_str[i][j].event_name);
                    }
                }
                finished = 1;
                break;    
                                                
            case 'd':
                value = 1;
                if (ioctl(fd, IFX_PMON_IOC_DISABLE, &value) == -1) {
                    printf("Failed to disable PMON\n");
                }                 
                finished = 1;
                break;    
            
            case 'e':
                event = atoi(optarg);
                if (event < IFX_PMON_EVENT_MIN || event > IFX_PMON_EVENT_MAX) {
                    printf("Invalid module <%d~%d>\n", IFX_PMON_EVENT_MIN, IFX_PMON_EVENT_MAX);
                    finished = 1;
                    break;
                }        
                break;

            case 't':
                tc = atoi(optarg);
                if (tc < IFX_MIPS_TC0 || tc > IFX_MIPS_TC3) {
                    printf("Invalid TC <%d~%d>\n", IFX_MIPS_TC0, IFX_MIPS_TC3);
                    finished = 1;
                    break;
                }        
                break;
                
            case 'c':
                counter = atoi(optarg);
                if (counter < IFX_PMON_XTC_COUNTER0|| counter > IFX_PMON_XTC_COUNTER1) {
                    printf("Invalid counter <%d~%d>\n", IFX_PMON_XTC_COUNTER0, IFX_PMON_XTC_COUNTER1);
                    finished = 1;
                    break;
                }     
                break;
    
            default:
                finished = 1;
                pmon_help();
                break;
        }
    }
    if (!finished) {
        struct ifx_pmon_ioctl_event pmon_event;
        
        pmon_event.pmon_event = event_to_str[event].event;
        pmon_event.tc = tc;
        pmon_event.counter  = counter;
        if (ioctl(fd, IFX_PMON_IOC_EVENT, &pmon_event) == -1) {
            printf("Failed to configure PMON event\n");
        }
        if (event != IFX_PMON_EVENT_NONE) {
            printf("TC%d Counter%d external event {%s} to internal event %d {%s}\n", 
                tc, counter, event_to_str[event].str, 
                tc_counter_event_str[tc][counter].event, tc_counter_event_str[tc][counter].event_name);
        }
        else {
            printf("All PMI interfaces have been disabled\n");
        }
    }
     
    close(fd);
    return 0;
}
