#ifndef __FWD_MAGPIE_
#define __FWD_MAGPIE_

#include <athdefs.h>
#include <hif.h>

/* fwd specific data - begin */

#define FWD_MAX_CHUNK           1024
#define FWD_MAX_TRIES           1000
#define FWD_TIMEOUT_MSECS       5000

/*
 * XXX Pack 'em
 */
typedef struct {
  a_uint16_t  more_data;     /* Is there more data? */
  a_uint16_t  len;           /* Len this segment    */
  a_uint32_t  offset;        /* Offset in the file  */
} fwd_cmd_t;

/*
 * No enums across platforms
 */
#define FWD_RSP_ACK     0x1
#define FWD_RSP_SUCCESS 0x2
#define FWD_RSP_FAILED  0x3

typedef struct {
    a_uint32_t   rsp;       /* ACK/SUCCESS/FAILURE */ 
    a_uint32_t   offset;    /* rsp for this ofset  */
}fwd_rsp_t;

typedef struct {
 HIF_HANDLE      hif_handle;     /* which i/f              */
 a_uint8_t       tx_pipe;        /* default tx pipe        */
 a_uint8_t       rx_pipe;        /* default rx pipe        */
/* adf_os_timer_t  tmr;             retry timer            */
 a_uint8_t      *image;         /* ptr to the image       */
 a_uint32_t      target_upload_addr;
 a_uint32_t      size;           /* total image size       */
 a_uint32_t      offset;         /* current offset         */
 a_uint16_t       chunk_retries;  /* Retries per chunk      */
 a_uint16_t       ntries;         /* num of download retries*/
} fwd_softc_t;


enum __mag_mii0_ctrl_type {
    MII0_CTRL_TYPE_MAC          = 0x00, /* Act as a MAC */
    MII0_CTRL_TYPE_PHY          = 0x04, /* Act as a PHY */
};

enum __mag_mii0_ctrl_speed {
	MII0_CTRL_SPEED_10          = 0x00, /* 10   Mbps*/
    MII0_CTRL_SPEED_100         = 0x10,	/* 100  Mbps*/
	MII0_CTRL_SPEED_1000        = 0x20, /* 1000 Mbps */
};

enum __mag_mii0_ctrl_delay {
    MII0_CTRL_RGMII_DELAY_NO    = 0x000, /* No delay */
    MII0_CTRL_RGMII_DELAY_SMALL = 0x100, /* small delay */
    MII0_CTRL_RGMII_DELAY_MED   = 0x200, /* Medium delay */
    MII0_CTRL_RGMII_DELAY_HUGE  = 0x300, /* Huge Delay */
};

typedef struct hif_gmac_params {
    a_uint8_t   mac_addr[6]; /* MAC Address */
    a_uint8_t   chip_type;    /* Act as MAC or PHY */
    a_uint8_t   link_speed;   /* RGMII Speed 10/100/1000 Mbps */
    a_uint16_t  rgmii_delay;  /* Delay no, small, medium, huge */
    a_uint16_t  dump_pkt;     /* Dump received packet at target */
    a_uint16_t  dump_pkt_lim; /* Dump packet limit */
}hif_gmac_params_t;

#endif
