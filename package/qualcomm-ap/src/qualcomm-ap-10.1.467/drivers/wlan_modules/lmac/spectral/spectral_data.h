/*
 * Copyright (c) 2010, Atheros Communications Inc.
 * All Rights Reserved.
 *
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 */

#ifndef _SPECTRAL_DATA_H_
#define _SPECTRAL_DATA_H_

#include "spec_msg_proto.h"

#ifndef MAX_SPECTRAL_MSG_ELEMS
#define MAX_SPECTRAL_MSG_ELEMS 10
#endif

#define MAX_SPECTRAL_CHAINS  3

#define SPECTRAL_SIGNATURE  0xdeadbeef

#ifndef NETLINK_ATHEROS
#define NETLINK_ATHEROS              (NETLINK_GENERIC + 1)
#endif

#ifdef WIN32
#pragma pack(push, spectral_data, 1)
#define __ATTRIB_PACK
#else
#ifndef __ATTRIB_PACK
#define __ATTRIB_PACK __attribute__ ((packed))
#endif
#endif

typedef struct spectral_classifier_params {
        int spectral_20_40_mode; /* Is AP in 20-40 mode? */
        int spectral_dc_index;
        int spectral_dc_in_mhz;
        int upper_chan_in_mhz;
        int lower_chan_in_mhz;
} __ATTRIB_PACK SPECTRAL_CLASSIFIER_PARAMS;


/* 11AC will have max of 512 bins */
#define MAX_NUM_BINS 512

typedef struct spectral_data {
    int16_t        spectral_data_len;
    int16_t        spectral_rssi;
    int16_t        spectral_bwinfo;
    int32_t        spectral_tstamp;
    int16_t        spectral_max_index;
    int16_t        spectral_max_mag;
} __ATTRIB_PACK SPECTRAL_DATA;

struct spectral_scan_data {
    u_int16_t chanMag[128];
    u_int8_t  chanExp;
    int16_t   primRssi;
    int16_t   extRssi;
    u_int16_t dataLen;
    u_int32_t timeStamp;
    int16_t   filtRssi;
    u_int32_t numRssiAboveThres;
    int16_t   noiseFloor;
    u_int32_t center_freq;
};

typedef struct spectral_msg {
        int16_t      num_elems;
        SPECTRAL_DATA data_elems[MAX_SPECTRAL_MSG_ELEMS];
} SPECTRAL_MSG;

/*
 * SAMP : Spectral Analysis Messaging Protocol
 *        Data format
 */
typedef struct spectral_samp_data {
    int16_t     spectral_data_len;                              /* indicates the bin size */
    int16_t     spectral_rssi;                                  /* indicates RSSI */
    int8_t      spectral_combined_rssi;                         /* indicates combined RSSI froma ll antennas */
    int8_t      spectral_upper_rssi;                            /* indicates RSSI of upper band */
    int8_t      spectral_lower_rssi;                            /* indicates RSSI of lower band */
    int8_t      spectral_chain_ctl_rssi[MAX_SPECTRAL_CHAINS];   /* RSSI for control channel, for all antennas */
    int8_t      spectral_chain_ext_rssi[MAX_SPECTRAL_CHAINS];   /* RSSI for extension channel, for all antennas */
    u_int8_t    spectral_max_scale;                             /* indicates scale factor */
    int16_t     spectral_bwinfo;                                /* indicates bandwidth info */
    int32_t     spectral_tstamp;                                /* indicates timestamp */
    int16_t     spectral_max_index;                             /* indicates the index of max magnitude */
    int16_t     spectral_max_mag;                               /* indicates the maximum magnitude */
    u_int8_t    spectral_max_exp;                               /* indicates the max exp */
    int32_t     spectral_last_tstamp;                           /* indicates the last time stamp */
    int16_t     spectral_upper_max_index;                       /* indicates the index of max mag in upper band */
    int16_t     spectral_lower_max_index;                       /* indicates the index of max mag in lower band */
    u_int8_t    spectral_nb_upper;                              /* Not Used */
    u_int8_t    spectral_nb_lower;                              /* Not Used */
    SPECTRAL_CLASSIFIER_PARAMS classifier_params;               /* indicates classifier parameters */
    u_int16_t   bin_pwr_count;                                  /* indicates the number of FFT bins */
    u_int8_t    bin_pwr[MAX_NUM_BINS];                          /* contains FFT magnitudes */
    struct INTERF_SRC_RSP interf_list;                          /* list of interfernce sources */
    int16_t noise_floor;                                        /* indicates the current noise floor */
    u_int32_t ch_width;                                         /* Channel width 20/40/80 MHz */
} __ATTRIB_PACK SPECTRAL_SAMP_DATA;

/*
 * SAMP : Spectral Analysis Messaging Protocol
 *        Message format
 */
typedef struct spectral_samp_msg {
        u_int32_t      signature;               /* Validates the SAMP messga */
        u_int16_t      freq;                    /* Operating frequency in MHz */
        u_int16_t      freq_loading;            /* How busy was the channel */
        u_int8_t       int_type;           /* Indicates the presence of CW interfernce */
        u_int8_t       macaddr[6];              /* Indicates the device interface */
        SPECTRAL_SAMP_DATA samp_data;           /* SAMP Data */
} __ATTRIB_PACK SPECTRAL_SAMP_MSG;


/* noise power cal support */
#define NOISE_PWR_SCAN_IOCTL            200
#define NOISE_PWR_SCAN_SET_DEBUG_IOCTL  201

#define NOISE_PWR_MAX_CHAINS            3
#define MAX_NOISE_PWR_REPORTS           10000

/*
 * units are: 4 x dBm - NOISE_PWR_DATA_OFFSET (e.g. -25 = (-25/4 - 90) = -96.25 dBm)
 * range (for 6 signed bits) is (-32 to 31) + offset => -122dBm to -59dBm
 * resolution (2 bits) is 0.25dBm
 */
typedef signed char pwr_dBm;

typedef struct {
    int rptcount;                       /* count of reports in pwr array */
    pwr_dBm un_cal_nf;                  /* uncalibrated noise floor */
    pwr_dBm factory_cal_nf;             /* noise floor as calibrated at the factory for module */
    pwr_dBm median_pwr;                 /* median power (median of pwr array)  */
    pwr_dBm pwr[];                      /* power reports */
} __ATTRIB_PACK CHAIN_NOISE_PWR_INFO;

typedef struct {
    pwr_dBm cal;
    pwr_dBm pwr;
} __ATTRIB_PACK CHAIN_NOISE_PWR_CAL;

typedef struct {
    uint8_t valid_chain_mask;           /* which chain cals to override (0x1 = chain 0, etc) */
    CHAIN_NOISE_PWR_CAL chain[NOISE_PWR_MAX_CHAINS];
} __ATTRIB_PACK NOISE_PWR_CAL;

typedef struct {
    uint16_t rptcount;                  /* count of reports required */
    uint8_t ctl_chain_mask;             /* ctl chains required (0x1 = chain 0, etc) */
    uint8_t ext_chain_mask;             /* ext chains required (0x1 = chain 0, etc) */
    NOISE_PWR_CAL cal_override;         /* override cal values - valid_chain_mask should be 0 for no override */
} __ATTRIB_PACK CHAIN_NOISE_PWR_IOCTL_REQ;


#ifdef WIN32
#pragma pack(pop, spectral_data)
#endif
#ifdef __ATTRIB_PACK
#undef __ATTRIB_PACK
#endif

#endif
