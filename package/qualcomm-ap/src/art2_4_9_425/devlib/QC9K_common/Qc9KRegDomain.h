/*
 * Copyright (c) 2002-2004 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/art2_main_per_cs/src/art2/devlib/QC9K_common/Qc9KRegDomain.h#2 $
 */

#ifndef _QC9K_REG_DOMAIN_H_
#define _QC9K_REG_DOMAIN_H_

typedef enum {
    DFS_UNINIT_DOMAIN   = 0,    /* Uninitialized dfs domain */
    DFS_FCC_DOMAIN      = 1,    /* FCC3 dfs domain */
    DFS_ETSI_DOMAIN     = 2,    /* ETSI dfs domain */
    DFS_MKK4_DOMAIN     = 3,    /* Japan dfs domain */
} HAL_DFS_DOMAIN;

typedef enum {
    HAL_M_IBSS              = 0,    /* IBSS (adhoc) station */
    HAL_M_STA               = 1,    /* infrastructure station */
    HAL_M_HOSTAP            = 6,    /* Software Access Point */
    HAL_M_MONITOR           = 8,    /* Monitor mode */
    HAL_M_RAW_ADC_CAPTURE   = 10    /* Raw ADC capture mode */
} HAL_OPMODE;

#define	SD_NO_CTL       0xf0
#define	NO_CTL          0xff
#define	CTL_MODE_M      0x0f
#define	CTL_11A         0
#define	CTL_11B         1
#define	CTL_11G         2
#define	CTL_TURBO       3
#define	CTL_108G        4
#define CTL_2GHT20      5
#define CTL_5GHT20      6
#define CTL_2GHT40      7
#define CTL_5GHT40      8
#define CTL_5GVHT80     9
#define CTL_2GVHT20     10
#define CTL_5GVHT20     11
#define CTL_2GVHT40     12
#define CTL_5GVHT40     13

typedef A_UINT16 HAL_CTRY_CODE;        /* country code */
typedef A_UINT16 HAL_REG_DOMAIN;       /* regulatory domain code */

typedef enum {
    HAL_VENDOR_APPLE    = 1,
} HAL_VENDORS;

#define HAL_NF_CAL_HIST_LEN_FULL  5
#define HAL_NF_CAL_HIST_LEN_SMALL 1
#define NUM_NF_READINGS           6   /* 3 chains * (ctl + ext) */
#define NF_LOAD_DELAY             1000 

#ifdef CTRY_DEFAULT
#undef CTRY_DEFAULT
#endif

enum {
    CTRY_DEBUG      = 0x1ff,    /* debug country code */
    CTRY_DEFAULT    = 0         /* default country code */
};

typedef enum {
        REG_EXT_FCC_MIDBAND            = 0,
        REG_EXT_JAPAN_MIDBAND          = 1,
        REG_EXT_FCC_DFS_HT40           = 2,
        REG_EXT_JAPAN_NONDFS_HT40      = 3,
        REG_EXT_JAPAN_DFS_HT40         = 4
} REG_EXT_BITMAP;       

enum {
    HAL_MODE_11A              = 0x00000001,      /* 11a channels */
    HAL_MODE_TURBO            = 0x00000002,      /* 11a turbo-only channels */
    HAL_MODE_11B              = 0x00000004,      /* 11b channels */
    HAL_MODE_PUREG            = 0x00000008,      /* 11g channels (OFDM only) */
#ifdef notdef                 
    HAL_MODE_11G              = 0x00000010,      /* 11g channels (OFDM/CCK) */
#else                         
    HAL_MODE_11G              = 0x00000008,      /* XXX historical */
#endif                        
    HAL_MODE_108G             = 0x00000020,      /* 11a+Turbo channels */
    HAL_MODE_108A             = 0x00000040,      /* 11g+Turbo channels */
    HAL_MODE_XR               = 0x00000100,      /* XR channels */
    HAL_MODE_11A_HALF_RATE    = 0x00000200,      /* 11A half rate channels */
    HAL_MODE_11A_QUARTER_RATE = 0x00000400,      /* 11A quarter rate channels */
    HAL_MODE_11NG_HT20        = 0x00000800,      /* 11N-G HT20 channels */
    HAL_MODE_11NA_HT20        = 0x00001000,      /* 11N-A HT20 channels */
    HAL_MODE_11NG_HT40PLUS    = 0x00002000,      /* 11N-G HT40 + channels */
    HAL_MODE_11NG_HT40MINUS   = 0x00004000,      /* 11N-G HT40 - channels */
    HAL_MODE_11NA_HT40PLUS    = 0x00008000,      /* 11N-A HT40 + channels */
    HAL_MODE_11NA_HT40MINUS   = 0x00010000,      /* 11N-A HT40 - channels */
    HAL_MODE_11AC_VHT20_2G      = 0x00020000,      /* 2Ghz, VHT20 */
    HAL_MODE_11AC_VHT40PLUS_2G  = 0x00040000,      /* 2Ghz, VHT40 + channels */
    HAL_MODE_11AC_VHT40MINUS_2G = 0x00080000,      /* 2Ghz  VHT40 - channels */
    HAL_MODE_11AC_VHT20_5G      = 0x00100000,      /* 5Ghz, VHT20 */
    HAL_MODE_11AC_VHT40PLUS_5G  = 0x00200000,      /* 5Ghz, VHT40 + channels */
    HAL_MODE_11AC_VHT40MINUS_5G = 0x00400000,      /* 5Ghz  VHT40 - channels */
    HAL_MODE_11AC_VHT80_0       = 0x00800000,      /* 5Ghz, VHT80 channels */
    HAL_MODE_11AC_VHT80_1       = 0x01000000,      /* 5Ghz, VHT80 channels */
    HAL_MODE_11AC_VHT80_2       = 0x02000000,      /* 5Ghz, VHT80 channels */
    HAL_MODE_11AC_VHT80_3       = 0x04000000,      /* 5Ghz, VHT80 channels */
    HAL_MODE_ALL                = 0xffffffff
};

#define HAL_MODE_11N_MASK ( HAL_MODE_11NG_HT20 | HAL_MODE_11NA_HT20 | HAL_MODE_11NG_HT40PLUS | HAL_MODE_11NG_HT40MINUS | HAL_MODE_11NA_HT40PLUS | HAL_MODE_11NA_HT40MINUS )
#define HAL_MODE_11AC_MASK ( HAL_MODE_11AC_VHT20_2G | HAL_MODE_11AC_VHT40PLUS_2G | HAL_MODE_11AC_VHT40MINUS_2G | \
                             HAL_MODE_11AC_VHT20_5G | HAL_MODE_11AC_VHT40PLUS_5G | HAL_MODE_11AC_VHT40MINUS_5G | \
                             HAL_MODE_11AC_VHT80_0 | HAL_MODE_11AC_VHT80_1 | HAL_MODE_11AC_VHT80_2 | HAL_MODE_11AC_VHT80_3 )
#define HAL_MODE_ALL_MASK ( HAL_MODE_11A_HALF_RATE | HAL_MODE_11A_QUARTER_RATE | \
                            HAL_MODE_11N_MASK | HAL_MODE_11AC_MASK | HAL_MODE_11A | HAL_MODE_11B | HAL_MODE_11G)

#define IS_CHAN_2GHZ(_c)    (((_c)->channel_flags & CHANNEL_2GHZ) != 0)

/* priv_flags */
#define CHANNEL_INTERFERENCE    0x01 /* Software use: channel interference 
                                        used for as AR as well as RADAR 
                                        interference detection */
#define CHANNEL_DFS             0x02 /* DFS required on channel */
#define CHANNEL_4MS_LIMIT       0x04 /* 4msec packet limit on this channel */
#define CHANNEL_DFS_CLEAR       0x08 /* if channel has been checked for DFS */
#define CHANNEL_DISALLOW_ADHOC  0x10 /* ad hoc not allowed on this channel */
#define CHANNEL_PER_11D_ADHOC   0x20 /* ad hoc support is per 11d */
#define CHANNEL_EDGE_CH         0x40 /* Edge Channel */

#define CHANNEL_A           (CHANNEL_5GHZ|CHANNEL_OFDM)
#define CHANNEL_B           (CHANNEL_2GHZ|CHANNEL_CCK)
#define CHANNEL_PUREG       (CHANNEL_2GHZ|CHANNEL_OFDM)
#ifdef notdef
#define CHANNEL_G           (CHANNEL_2GHZ|CHANNEL_DYN)
#else
#define CHANNEL_G           (CHANNEL_2GHZ|CHANNEL_OFDM)
#endif
#define CHANNEL_T           (CHANNEL_5GHZ|CHANNEL_OFDM|CHANNEL_TURBO)
#define CHANNEL_ST          (CHANNEL_T|CHANNEL_STURBO)
#define CHANNEL_108G        (CHANNEL_2GHZ|CHANNEL_OFDM|CHANNEL_TURBO)
#define CHANNEL_108A        CHANNEL_T
#define CHANNEL_X           (CHANNEL_5GHZ|CHANNEL_OFDM|CHANNEL_XR)
#define CHANNEL_G_HT20      (CHANNEL_2GHZ|CHANNEL_HT20)
#define CHANNEL_A_HT20      (CHANNEL_5GHZ|CHANNEL_HT20)
#define CHANNEL_G_HT40PLUS  (CHANNEL_2GHZ|CHANNEL_HT40PLUS)
#define CHANNEL_G_HT40MINUS (CHANNEL_2GHZ|CHANNEL_HT40MINUS)
#define CHANNEL_A_HT40PLUS  (CHANNEL_5GHZ|CHANNEL_HT40PLUS)
#define CHANNEL_A_HT40MINUS (CHANNEL_5GHZ|CHANNEL_HT40MINUS)
#define CHANNEL_A_HALF_RATE (CHANNEL_5GHZ|CHANNEL_HALF)
#define CHANNEL_A_QUARTER_RATE (CHANNEL_5GHZ|CHANNEL_QUARTER)
#define CHANNEL_G_VHT20      (CHANNEL_2GHZ|CHANNEL_VHT20)
#define CHANNEL_G_VHT40PLUS  (CHANNEL_2GHZ|CHANNEL_VHT40PLUS)
#define CHANNEL_G_VHT40MINUS (CHANNEL_2GHZ|CHANNEL_VHT40MINUS)
#define CHANNEL_A_VHT20      (CHANNEL_5GHZ|CHANNEL_VHT20)
#define CHANNEL_A_VHT40PLUS  (CHANNEL_5GHZ|CHANNEL_VHT40PLUS)
#define CHANNEL_A_VHT40MINUS (CHANNEL_5GHZ|CHANNEL_VHT40MINUS)
#define CHANNEL_A_VHT80_0   (CHANNEL_5GHZ|CHANNEL_VHT80_0)
#define CHANNEL_A_VHT80_1   (CHANNEL_5GHZ|CHANNEL_VHT80_1)
#define CHANNEL_A_VHT80_2   (CHANNEL_5GHZ|CHANNEL_VHT80_2)
#define CHANNEL_A_VHT80_3   (CHANNEL_5GHZ|CHANNEL_VHT80_3)

#define CHANNEL_ALL (CHANNEL_OFDM|CHANNEL_CCK| CHANNEL_2GHZ | CHANNEL_5GHZ | CHANNEL_TURBO | CHANNEL_HT20 | \
        CHANNEL_HT40PLUS | CHANNEL_HT40MINUS | CHANNEL_VHT20 | CHANNEL_VHT40PLUS | CHANNEL_VHT40MINUS | \
        CHANNEL_VHT80_0 | CHANNEL_VHT80_1 | CHANNEL_VHT80_2 | CHANNEL_VHT80_3)
#define CHANNEL_ALL_NOTURBO (CHANNEL_ALL &~ CHANNEL_TURBO)

/* regulatory capabilities */
#define AR_EEPROM_EEREGCAP_EN_FCC_MIDBAND   0x0040
#define AR_EEPROM_EEREGCAP_EN_KK_U1_EVEN    0x0080
#define AR_EEPROM_EEREGCAP_EN_KK_U2	        0x0100
#define AR_EEPROM_EEREGCAP_EN_KK_MIDBAND    0x0200
#define AR_EEPROM_EEREGCAP_EN_KK_U1_ODD	    0x0400
#define AR_EEPROM_EEREGCAP_EN_KK_NEW_11A    0x0800

typedef struct {
    A_UINT16    channel;    /* NB: must be first for casting */
    A_UINT32    channel_flags;
    A_UINT8     priv_flags;
    //A_INT8    max_reg_tx_power;
    A_INT8      max_tx_power;
    A_INT8      min_tx_power; /* as above... */
    A_UINT8    regClassId; /* Regulatory class id */
    //A_UINT8  paprd_done:1,           /* 1: PAPRD DONE, 0: PAPRD Cal not done */
    //          paprd_table_write_done:1; /* 1: DONE, 0: Cal data write not done */
    //bool      bssSendHere;
    //A_UINT8    gainI;
    //bool      iqCalValid;
    //int32_t   cal_valid;
    //bool      one_time_cals_done;
    //A_INT8    i_coff;
    //A_INT8    q_coff;
    //int16_t   raw_noise_floor;
    //int16_t     noiseFloorAdjust;
    //???A_INT8      antennaMax;
    A_UINT32   regDmnFlags;    /* Flags for channel use in reg */
    A_UINT32 conformance_test_limit; /* conformance test limit from reg domain */
    //u_int64_t       ah_tsf_last;            /* tsf @ which time accured is computed */
    //u_int64_t       ah_channel_time;        /* time on the channel  */
    //A_UINT16   mainSpur;       /* cached spur value for this cahnnel */
    //u_int64_t dfs_tsf;
    /*
     * Each channels has a NF history buffer.
     * If ATH_NF_PER_CHAN is defined, this history buffer is full-sized
     * (HAL_NF_CAL_HIST_MAX elements).  Else, this history buffer only
     * stores a single element.
     */
    //HAL_CHAN_NFCAL_HIST  nf_cal_hist;
} QC9K_CHANNEL_INTERNAL;

/* nf - parameters related to noise floor filtering */
struct noise_floor_limits {
    A_INT16 nominal; /* what is the expected NF for this chip / band */
    A_INT16 min;     /* maximum expected NF for this chip / band */
    A_INT16 max;     /* minimum expected NF for this chip / band */
};

//
// Function Declarations
//
extern void Qc9KRegDomainInit();
extern void Qc9KRegulatoryDomainOverride(unsigned int regdmn);
extern int Qc9KInitChannels(int *chans, int *flags, A_UINT32 maxchans, unsigned int *nchans,
              A_UINT8 *regclassids, A_UINT32 maxregids, A_UINT32 *nregids,
              int cc, A_UINT32 modeSelect,
              int enableOutdoor, int enableExtendedChannels);

#endif  //_QC9K_REG_DOMAIN_H_

