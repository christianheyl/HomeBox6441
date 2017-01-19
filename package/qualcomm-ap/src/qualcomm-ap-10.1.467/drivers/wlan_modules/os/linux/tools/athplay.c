/*
 * Copyright (c) 2013 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

/*
 * =====================================================================================
 *
 *       Filename:  athplay.c
 *
 *    Description:  Music player for Sony Demo
 *
 *        Version:  1.0
 *        Created:  Wednesday 28 October 2009 11:06:38  IST
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Karthikeyan 
 *        Company:  Atheros Communications 
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <fcntl.h>
#include <signal.h>

#include "if_athioctl.h"

void cleanup(int sig);
int set_aow_channel(int channel, char* data);

/* globals */
int buf_size = 0;

/* The IOCTL defines should be moved to a respective header
 * file. 
 */

#define IOCTL_AOW_DATA (1)
#define FILE_TYPE_WAV   1
#define FILE_TYPE_PCM   2
#define ATH_AOW_RESET_COMMAND 0xa5a5a5a5
#define ATH_AOW_START_COMMAND 0xb5b5b5b5
#define ATH_AOW_SET_CHANNEL   0x00000001
#define ETHER_ADDR_SIZE       6

#ifndef ATH_DEFAULT
#define ATH_DEFAULT "wifi0"
#endif


/*
 * For a fixed throughput : Read at a constant rate in a given time
 *
 *
 */


#define NUM_BYTES_READ      (768*2)
#define WAIT_TIME_IN_USEC   8000


/* The global place holder for audio 
 * samples
 */
int audio_samples[NUM_BYTES_READ/sizeof(int)]; 
int wait_in_usec = WAIT_TIME_IN_USEC;


#define dp(...) do { if (dbg) { fprintf(stderr, __VA_ARGS__); } } while (0)
#define ep(...) do { fprintf(stderr, __VA_ARGS__); } while (0)

#if __BYTE_ORDER == __BIG_ENDIAN
#	if !defined(__NetBSD__)
#		include <byteswap.h>
#	else
#		include <sys/bswap.h>
#		define bswap_32 bswap32
#		define bswap_16 bswap16
#	endif
#endif

struct athplay_handler {
    int socket;
};    

int dbg = 1;
int mclk_sel = 0;


/* 
 * Sub Chunk Data format for the given
 * wave file
 */
typedef struct {
    u_int   chunk;  /* data */
    u_int   length; /* length */
} sub_chunk_data_t;    


/*
 * Data format for the WAVE file header
 */
typedef struct  {

    u_int   riff;                   /* RIFF : Main chunk */
    u_int   length;                 /* Length of the rest of file */
    u_int   chunk_type;             /* WAV */
    u_int   sub_chunk;              /* FMT */
    u_int   sub_chunk_length;       /* Length of sub chunk */
    u_short format;                 /* 1 for PCM */
    u_short mode;                   /* 1 for Mono, 2 for Stereo */
    u_int   sample_freq;            /* Sampling frequency */
    u_int   byte_per_sec;             /* Bytes per second */
    u_short byte_per_sample;        /* Bytes per sample */
    u_short bit_per_sample;         /* Bits per sample, 8,12 or 16 */

	/*
	 * FIXME:
	 * Apparently, two different formats exist.
	 * One with a sub chunk length of 16 and another of length 18.
	 * For the one with 18, there are two bytes here.  Don't know
	 * what they mean.  For the other type (i.e. length 16) this
	 * does not exist.
	 *
	 * To handle the above issue, some jugglery is done after we
	 * read the header
	 *		-Varada (Wed Apr 25 14:53:02 PDT 2007)
	 */

	u_char		pad[2];
	sub_chunk_data_t sc;

} wavheader_t;



char separator = ':';

char* convert_macaddr_to_string(char* addr, unsigned char* string)
{
    sprintf(addr, "%02x%c%02x%c%02x%c%02x%c%02x%c%02x",
            string[0] & 0xff,
            separator,
            string[1] & 0xff,
            separator,
            string[2] & 0xff,
            separator,
            string[3] & 0xff,
            separator,
            string[4] & 0xff,
            separator,
            string[5] & 0xff);

    return addr;
}


unsigned char * convert_addrstr_to_byte(char* addr, char* dst)
{
    int i = 0;

    for (i = 0; i < 6; ++i)
    {
        unsigned int inum = 0;
        char ch;

        ch = tolower(*addr++);

        if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f'))
            return NULL;


        inum = isdigit (ch)?(ch - '0'):(ch - 'a' + 10);
        ch = tolower(*addr);

        if ((i < 5 && ch != separator) ||
            (i == 5 && ch != '\0' && !isspace(ch)))
            {
                ++addr;
                if ((ch < '0' || ch > '9') &&
                    (ch < 'a' || ch > 'f'))
                        return NULL;

                inum <<= 4;
                inum += isdigit(ch) ? (ch - '0') : (ch - 'a' + 10);
                ch = *addr;

                if (i < 5 && ch != separator)
                    return NULL;
        }

        dst[i] = (unsigned char)inum;
        ++addr;
    }
    return dst;



}


/* utility function to swap bytes */
void swapbytes16 (char *buffer, int length)
{
	int i;
	u_int16_t	*ptr, temp;

	ptr = (u_int16_t *)buffer;
	length = length / sizeof(temp);

	for (i = 0; i < (length - 1); i += 2) {
		temp = ptr[i];
		ptr[i] = ptr[i+1];
		ptr[i+1] = temp;
	}
}

/* utility function to print wave file header */
void print_wav_hdr(wavheader_t *hdr)
{

    if (hdr) {
      printf("----------------\n");
      printf("Wave Header Info\n");
      printf("----------------\n");
      printf("Length = %d\n"
             "Sub Chunk Length = %d\n"
             "Format = %d\n"
             "Mode = %d\n"
             "Sampling Frequency = %d\n"
             "Bytes/Second = %d\n"
             "Bytes/Sample = %d\n"
             "Bits/Sample = %d\n", 
              hdr->length, 
              hdr->sub_chunk_length,
              hdr->format,
              hdr->mode,
              hdr->sample_freq,
              hdr->byte_per_sec,
              hdr->byte_per_sample,
              hdr->bit_per_sample);

      printf("----------------\n");
      printf("Sub Chunk Info\n");
      printf("----------------\n");
      printf("Pad[0] = %d\n"
              "Pad[1] = %d\n"
              "Chunk  = %d\n"
              "Length = %d\n", 
              hdr->pad[0], 
              hdr->pad[1],
              hdr->sc.chunk,
              hdr->sc.length); 
    }
}


/*
 * playwav:
 * This function reads the data from the given wave file
 * at a constant rate and passes them to wlan driver
 */
int playwav(int fd)
{

    wavheader_t hdr;
    sub_chunk_data_t sc;
    int tmpcount = 0;
    int ret = 0;
    int count = 0;
    int i = 0;
    char *audio_data;
    char *data;
    char *tmp;
    int total_bytes_read = 0;
    int iterations = 0;


    if (fd < 0)
        return -EINVAL;

    read(fd, &hdr, sizeof(hdr));

#if __BYTE_ORDER == __BIG_ENDIAN
    hdr.length = bswap_32(hdr.length);
    hdr.sub_chunk_length = bswap_32(hdr.sub_chunk_length);
    hdr.format = bswap_32(hdr.format);
    hdr.mode = bswap_32(hdr.mode);
    hdr.sample_freq = bswap_32(hdr.sample_freq);
    hdr.byte_per_sec = bswap_32(hdr.byte_per_sec);
    hdr.bit_per_sample = bswap_32(hdr.bit_per_sample);
#endif  /* __BYTE_ORDER */    


    if (hdr.sub_chunk_length == 16) {
        tmp = &hdr.pad[0];
        lseek(fd, -2, SEEK_CUR);
    } else if ( hdr.sub_chunk_length == 18) {
        tmp = &hdr.pad[2];
    } else {
        printf("Invalid Header\n");
        return -EINVAL;
    }        

    bcopy(tmp, &sc, sizeof(sc));

#if __BYTE_ORDER == __BIG_ENDIAN
    sc.chunk = bswap_32(sc.chunk);
    sc.length = bswap_32(sc.length);
#endif  /* __BYTE_ORDER */    

    if (buf_size <= 0) {
        buf_size = NUM_BYTES_READ;
    }        

#ifdef  MASTER_CLK    
    if (ioctl(fd, I2S_MCLK, mclk_sel) < 0) {
        perror("I2S_MCLK");
    }        
#endif  /* MASTER_CLK */    

    send_aow_start();
    audio_data = (char *)malloc(buf_size * sizeof(char));

    if (audio_data == NULL)
        return -ENOMEM;

    for (i = 0; i <= sc.length; i += buf_size) {

        count = buf_size;


        if ((i + count) > sc.length) {
            count = sc.length - i;
        }            

        iterations++;
        total_bytes_read += count;

        if ((count = read(fd, audio_data, count)) <= 0) {
            perror("Read audio data");
            break;
        }            
        
        if (hdr.bit_per_sample == 16) {
            swapbytes16(audio_data, count);
        }            
       
        /* Send the data to wlan device via ioctl interface
         * XXX Take care of errror
         */

        send2driver(data, tmpcount);
    }        
    send_aow_reset();
    printf("\nTotal bytes read = %d in %d iterations (wav file)\n", total_bytes_read, iterations);
    free(audio_data);
    return 0;
}


void print_buf(char *buf, int len)
{
    int i = 0;
    printf("\nApp Dump\n");
    while (i++ < len) {
        printf("0x%04x ", *(buf + i));
        if ( (i) && !(i % 8))
            printf("\n");
    }
    printf("\n");
}    


/*
 * play : 
 * This function reads the data from the given audio
 * file at a constant rate and passes them to the
 * wlan driver.
 *
 */      
int playpcm(int fd)
{
    int num_bytes_read = 0;
    int total_bytes_read = 0;
    int i = 0;
    int iterations = 0;

    struct timeval time_adjust_start;
    struct timeval time_adjust_end;
    int wait_time_adjust = 0;
    int total_xfer_msec = 0;


    if (fd < 0) {
       return -EINVAL;
    }        

    gettimeofday(&time_adjust_start, NULL);

     /* Removed the Sending the START command from athplay
      * need to be sent manually
      */
     //send_aow_start();
     while((num_bytes_read = read(fd, audio_samples, NUM_BYTES_READ)) 
            == NUM_BYTES_READ) {
            iterations++;

            /* Soft 16-bit swap needed by I2S */
            //swapbytes16((char *)audio_samples, num_bytes_read);

            /* send this data to driver */
            send2driver(audio_samples, num_bytes_read);
            total_bytes_read += num_bytes_read;

            gettimeofday(&time_adjust_end, NULL);
            if (((time_adjust_end.tv_usec - time_adjust_start.tv_usec) > 0) &&
                ((time_adjust_end.tv_usec - time_adjust_start.tv_usec) < wait_in_usec)) {
                    wait_time_adjust = time_adjust_end.tv_usec - time_adjust_start.tv_usec;
            }                    

            usleep(wait_in_usec - wait_time_adjust);
            gettimeofday(&time_adjust_start, NULL);
#if 0
            if (!(iterations % 20)) {
                printf("Current throughput (approx) %f Mbps\n", 
                        ((num_bytes_read * 8)/((time_adjust_start.tv_usec - time_adjust_end.tv_usec)/1000))/1000.00);
            }                        
#endif
     }

     /* Soft 16-bit swap needed by I2S */
     //swapbytes16((char *)audio_samples, num_bytes_read);
     send2driver(audio_samples, num_bytes_read);
     total_bytes_read += num_bytes_read;

     /* Removed the Sending the STOP command from athplay
      * need to be sent manually
      */
     //send_aow_reset();
     printf("\nTotal bytes read = %d in %d iterations (pcm file)\n", total_bytes_read, iterations);

}    

void usage(void)
{
    fprintf(stderr,"usage : mplay [cmd] <params>\n"
#ifdef  MASTER_CLK
                   "mplay filename <wav/pcm> delay <in us> mclk <1/0>\n"
#else
                   "mplay filename <wav/pcm> delay <in us>\n"
#endif                   
                   "mplay ch  <0-9> <macaddress xx:xx:xx:xx:xx:xx>\n"
                   "mplay start # Debug command\n"
                   "mplay stop  # Debug command\n"
                   "mplay test  # Debug command\n");
}    

struct aow_handler {
    int socket;
    struct ath_diag atd;
};    

int aow_init_ioctl(struct aow_handler *aow)
{

    memset(aow, 0, sizeof(struct aow_handler));
    aow->socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (aow->socket < 0) {
       err(1, "socket");
       return -1;
    }       

    strncpy(aow->atd.ad_name, ATH_DEFAULT, sizeof(aow->atd.ad_name));
    return 0;
}    

int send2driver(char* data, int size)
{

    u_int result = 0;
    struct ifreq ifr;
    struct aow_handler aow;

    if (aow_init_ioctl(&aow))
        return -1;

    aow.atd.ad_id = IOCTL_AOW_DATA | ATH_DIAG_IN;
    aow.atd.ad_in_data = data;
    aow.atd.ad_in_size = size;
    aow.atd.ad_out_data = NULL;
    aow.atd.ad_out_size = 0;
    strcpy(ifr.ifr_name, aow.atd.ad_name);
    ifr.ifr_data = (caddr_t)&aow.atd;

    if (ioctl(aow.socket, SIOCGATHAOW, &ifr) < 0)
        err(1, aow.atd.ad_name);

    close(aow.socket);
    return 0;
}

int send_data_pkt(int size)
{

    u_int16_t buffer[1024];
    int result = 0;
    struct ifreq ifr;
    struct aow_handler aow;
    int i = 0;

    if (size > 1024) {
        printf("Max packet size allowed is 1024\n");
        return 0;
    }        

    while ( i < size) {
        buffer[i] = i;//0xa5;
        i++;
    }        

    if (aow_init_ioctl(&aow))
        return -1;

    printf("Sending a test packet of size %d\n", size);

    aow.atd.ad_id = IOCTL_AOW_DATA | ATH_DIAG_IN;
    aow.atd.ad_in_data = (char *)buffer;
    aow.atd.ad_in_size = sizeof(u_int16_t)*size;
    aow.atd.ad_out_size = 0;
    aow.atd.ad_out_data = NULL;
    strcpy(ifr.ifr_name, aow.atd.ad_name);
    ifr.ifr_data = (caddr_t)&aow.atd;

    if (ioctl(aow.socket, SIOCGATHAOW, &ifr) < 0)
        err(1, aow.atd.ad_name);

    close(aow.socket);
    return 0;
}

int test_aow_ioctl()
{
    char buffer[NUM_BYTES_READ];
    u_int result = 0;
    struct ifreq ifr;
    struct aow_handler aow;
    int i = 0;

    {
        while (i < NUM_BYTES_READ) {
            buffer[i] = i;
            i++;            
        }            
    }            
        
    if (aow_init_ioctl(&aow))
        return -1;

    send_aow_start();

    printf("Sending a test packet of size %d\n", NUM_BYTES_READ);

    aow.atd.ad_id = IOCTL_AOW_DATA | ATH_DIAG_IN;
    aow.atd.ad_in_data = buffer;
    aow.atd.ad_in_size = NUM_BYTES_READ;
    aow.atd.ad_out_size = 0;
    aow.atd.ad_out_data = NULL;
    strcpy(ifr.ifr_name, aow.atd.ad_name);
    ifr.ifr_data = (caddr_t)&aow.atd;


    if (ioctl(aow.socket, SIOCGATHAOW, &ifr) < 0)
        err(1, aow.atd.ad_name);

    close(aow.socket);
    send_aow_reset();
    return 0;
}    

int send_aow_start()
{
    int start = ATH_AOW_START_COMMAND;
    u_int result = 0;
    struct ifreq ifr;
    struct aow_handler aow;

    if (aow_init_ioctl(&aow))
        return -1;

    
    aow.atd.ad_id = IOCTL_AOW_DATA | ATH_DIAG_IN;
    aow.atd.ad_in_data = (char*)&start;
    aow.atd.ad_in_size = sizeof(int);
    aow.atd.ad_out_size = 0;
    aow.atd.ad_out_data = NULL;
    strcpy(ifr.ifr_name, aow.atd.ad_name);
    ifr.ifr_data = (caddr_t)&aow.atd;

    if (ioctl(aow.socket, SIOCGATHAOW, &ifr) < 0)
        err(1, aow.atd.ad_name);

    close(aow.socket);
    return 0;
}

struct chan_data {
    int command;
    int channel;
    char addr[ETHER_ADDR_SIZE];
};    

int set_aow_channel(int channel, char* macaddr)
{
    struct chan_data cdata;
    char   address[ETHER_ADDR_SIZE];
    char   addrstr[30];
    int i = 0;

    strcpy(addrstr, macaddr);
    convert_addrstr_to_byte(addrstr, address);

    cdata.command = ATH_AOW_SET_CHANNEL;
    cdata.channel = channel;
    memcpy(&cdata.addr, address, ETHER_ADDR_SIZE);

    u_int result = 0;
    struct ifreq ifr;
    struct aow_handler aow;

    if (aow_init_ioctl(&aow))
        return -1;

    aow.atd.ad_id = IOCTL_AOW_DATA | ATH_DIAG_IN;
    aow.atd.ad_in_data = (char*)&cdata;
    aow.atd.ad_in_size = sizeof(int) + sizeof(int) + ETHER_ADDR_SIZE;
    aow.atd.ad_out_size = 0;
    aow.atd.ad_out_data = NULL;
    strcpy(ifr.ifr_name, aow.atd.ad_name);
    ifr.ifr_data = (caddr_t)&aow.atd;

    if (ioctl(aow.socket, SIOCGATHAOW, &ifr) < 0)
        err(1, aow.atd.ad_name);

    close(aow.socket);
    return 0;

}



int send_aow_reset()
{
    int reset = ATH_AOW_RESET_COMMAND;
    u_int result = 0;
    struct ifreq ifr;
    struct aow_handler aow;

    usleep(2000000);
    if (aow_init_ioctl(&aow))
        return -1;

    aow.atd.ad_id = IOCTL_AOW_DATA | ATH_DIAG_IN;
    aow.atd.ad_in_data = (char*)&reset;
    aow.atd.ad_in_size = sizeof(int);
    aow.atd.ad_out_size = 0;
    aow.atd.ad_out_data = NULL;
    strcpy(ifr.ifr_name, aow.atd.ad_name);
    ifr.ifr_data = (caddr_t)&aow.atd;

    if (ioctl(aow.socket, SIOCGATHAOW, &ifr) < 0)
        err(1, aow.atd.ad_name);

    close(aow.socket);
    return 0;

}    

/* 
 * Make a clean exit
 *
 */

void cleanup(int sig) {
    usleep(500);
    //send_aow_reset();
    exit(sig);
}    

int main(int argc, char* argv[])
{

    int fd;                     /* the audio file descriptor */
    int optc;                   /* for getopt */
    int filetype = FILE_TYPE_WAV;

    /* install the sig handler */
    (void)signal(SIGINT, cleanup);

#ifdef  MASTER_CLK
    if (argc < 7) {
#else
    if (argc < 5) {
#endif  /* MASTER_CLK */    

        if (argc < 2) {
            usage();
            return;
        }            

        /* TODO : Cleanup the mess */

        if (!strcmp(argv[1], "test")) {
            test_aow_ioctl();
            return;
        } else if (!strcmp(argv[1], "spkt")) {
            if (argc < 3) {
                send_data_pkt(1024);
            }else
                send_data_pkt(atoi(argv[2]));
            return;                
        } else if (!strcmp(argv[1], "stop")) {
            send_aow_reset();
            return;
        } else if (!strcmp(argv[1], "start")) {
          send_aow_start();
          return;
        } else if (!strcmp(argv[1], "ch")) {
          if (argc < 4) {
              printf("mplay ch <channel : 0-9> <macaddress xx:xx:xx:xx:xx:xx>\n");
              return;
          }                
          set_aow_channel(atoi(argv[2]), argv[3]);
          return;
        }

        usage();
        return;
    }

    if (!strcmp(argv[2], "wav")) {
        filetype = FILE_TYPE_WAV;
    } else if (!strcmp(argv[2], "pcm")) {
        filetype = FILE_TYPE_PCM;
    } else {
        usage();
        return;
    }

    if (!strcmp(argv[3], "delay")) {
         wait_in_usec = atoi(argv[4]);
    } else {
         wait_in_usec = WAIT_TIME_IN_USEC;
    }

#ifdef  MASTER_CLK
    if (!strcmp(argv[5], "mclk"))
        mclk_sel = 1;
    else
        mclk_sel = 0;
#endif  /* MASTER_CLK */


    if ((fd = open(argv[1], O_RDONLY)) == -1) {
        perror(argv[optind]);
        exit(-1);
    }

    switch(filetype) {
        case FILE_TYPE_PCM:
            playpcm(fd);
            break;
        case FILE_TYPE_WAV:
            playwav(fd);
            break;
        default:
            usage();
            break;
    }            

    close(fd);
    return 0;
}
