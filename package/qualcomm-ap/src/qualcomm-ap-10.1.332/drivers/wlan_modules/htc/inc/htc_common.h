#ifndef _HTC_COMMON_H
#define _HTC_COMMON_H

/* Please make sure the size of ALL headers is on word alignment */

#define RX_STATS_SIZE 10

/* Tx frame header flags definition */
//Reserved bit-0 for selfCTS
//Reserved bit-1 for RTS
#define TFH_FLAGS_USE_MIN_RATE                      0x100

struct  rx_frame_header {
    a_uint32_t rx_stats[RX_STATS_SIZE];
#ifdef ATH_HTC_SG_SUPPORT    
    a_uint32_t  sg_length;
    a_uint8_t   is_seg;
    a_uint8_t   sgseqno;
    a_uint8_t   numseg;   
    a_uint8_t   res;
    a_uint64_t  tsf;
#endif
};

typedef struct _mgt_header {
    a_uint8_t   ni_index;
    a_uint8_t   vap_index;
    a_uint8_t   tidno;
    a_uint8_t   flags;
    a_int8_t    keytype;
    a_int8_t    keyix;
    a_int16_t   byte_reserve;
} ath_mgt_hdr_t;


typedef struct _beacon_header {
    a_uint8_t   vap_index;   
    a_uint8_t   len_changed;    
    a_uint16_t  reserved;
}ath_beacon_hdr_t;

typedef struct __data_header {
    a_uint8_t   datatype;
    a_uint8_t   ni_index;
    a_uint8_t   vap_index;
    a_uint8_t   tidno;
    a_uint32_t  flags;  
    a_uint8_t    keytype;
    a_uint8_t    keyix;
#ifdef ENCAP_OFFLOAD    
    a_uint8_t   keyid;
    a_uint8_t   byte_reserve;
#else
    a_uint16_t  byte_reserve;
#endif    
    a_uint8_t  maxretry;
    a_uint8_t  pad[3];
#ifdef MAGPIE_HIF_GMAC
    a_uint32_t  pkt_type;
    a_uint8_t   reserved[16];       /*padding till config pipe fixes reserve*/
#else
#ifdef ENCAP_OFFLOAD
    a_uint8_t   reserved[20];       /*padding till config pipe fixes reserve*/
#endif
#endif

} ath_data_hdr_t;

#ifdef MAGPIE_HIF_GMAC
#define ATH_DATA_HDR_FRAG_FIRST 1
#define ATH_DATA_HDR_FRAG_LAST  2
#endif

#ifdef ENCAP_OFFLOAD
#define    ATH_SHORT_PREAMBLE       0x1
#define    ATH_DH_FLAGS_PRIVACY     0x2
#define    ATH_DH_FLAGS_MOREDATA    0x4
#define    ATH_DH_FLAGS_EOSP        0x8
#define    ATH_DH_FLAGS_QOSNULL     0x10
#endif

#endif 
