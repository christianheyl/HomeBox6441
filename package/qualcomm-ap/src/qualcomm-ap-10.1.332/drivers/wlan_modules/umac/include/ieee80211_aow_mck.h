
/*
 * =====================================================================================
 *
 *       Filename:  ieee80211_aow.h
 *
 *    Description:  Header file for AOW Functions
 *
 *        Version:  1.0
 *        Created:  Friday 13 November 2009 04:47:44  IST
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#ifndef IEEE80211_AOW_H
#define IEEE80211_AOW_H

#if  ATH_SUPPORT_AOW

#include "if_upperproto.h"
#include "ieee80211_aow_shared.h"
#include "if_athrate.h"
#include "ah_desc.h"
#include "ath_desc.h"
#include "ieee80211.h"
#include "if_aow.h"
#include "if_aow_app.h"


#define AOW_LINUX_VERSION  (KERNEL_VERSION(2,6,15))

/* Simulation of audio sample sizes higher than 16 bits,
   by adding dummy data at the end of MPDUs. */
#define ATH_SUPPORT_AOW_SAMPSIZE_SIM                0

/* Debugging of deltas between timestamp and current TSF
   at receiver.
   (to catch cases where current TSF is smaller than
    timestamp, or equal, or is larger but the difference is too
    small).*/
#define ATH_SUPPORT_AOW_TIMESTAMP_DELTA_DEBUG       1

/* Rough profiling of Dynamic Rx Timeout calculation.
   
   IMPORTANT: For internal study by developers only.
              Do NOT enable in production code, ever.
   
   How to interpret resulting prints (formatted for 
   external analysis scripts, and shortened due to kernel
   print buffer constraints):
   AoW DRP : AoW Dynamic Rx Timeout Profiling
   t       : TSF time of log
   s       : AoW Sequence number (can be NA)
   ss      : AoW Sub-Sequence number (can be NA)
   r       : Rx Timeout value in usec. (Can be 'Def' for Default,
             or NA)
   a       : Action
             C -> Timer cancellation
             S -> Setting Rx Timeout 
             E -> Entering timer function
             H -> Hole detected by timer function, breaking out with
                  new Rx timeout
             D -> Delivery of head AoW frame by timer function
   
   Note: It would be preferable to increase CONFIG_LOG_BUF_SHIFT
   on Linux, when using this profiling option.
*/
//#define ATH_SUPPORT_AOW_DYNRXTIMEOUT_PROFILING      1

/* AoW device */
extern aow_dev_t wlan_aow_dev;

/* place holder for ic and sc pointers */
typedef struct aow_info {
    struct ieee80211com* ic;
    struct ath_softc* sc;
} aow_info_t;


/* data structures */
typedef enum {
    AOW_DATA_PKT,
    AOW_CTRL_PKT
} AOW_PKT_TYPE;    

#define IEEE80211_AOW_UPDATE_RX_VOLUME_INFO(ic, apkt)                                       \
    do {                                                                                    \
        u_int32_t i = 0;                                                                    \
        /* copy the volume information */                                                   \
        for (i = 0; i < AOW_MAX_AUDIO_CHANNELS; i++) {                                      \
            OS_MEMCPY(ic->ic_aow.volume_info.ch[i].info, apkt->volume_info.ch[i].info, AOW_VOLUME_DATA_LENGTH);  \
        }                                                                                   \
        ic->ic_aow.volume_info.volume_flag = apkt->volume_info.volume_flag;                 \
    }while(0)                                                                               \


#define IEEE80211_AOW_INSERT_VOLUME_INFO(ic, apkt)                                          \
    do {                                                                                    \
        u_int32_t i = 0;                                                                    \
        /* copy the volume information */                                                   \
        for (i = 0; i < AOW_MAX_AUDIO_CHANNELS; i++) {                                      \
            OS_MEMCPY(apkt->volume_info.ch[i].info, ic->ic_aow.volume_info.ch[i].info, AOW_VOLUME_DATA_LENGTH);  \
        }                                                                                   \
        apkt->volume_info.volume_flag = ic->ic_aow.volume_info.volume_flag;                 \
    }while(0)                                                                               \

#define IS_VOLUME_INFO_CHANGED(ic, apkt) (ic->ic_aow.volume_info.volume_flag != apkt->volume_info.volume_flag?1:0)
    
#define AOW_MAC_ADDR_LENGTH     6
typedef struct aow_host_pkt {
    u_int32_t signature;
    u_int32_t type;
    u_int32_t subtype;
    u_int32_t length;
    u_int8_t  addr[AOW_MAC_ADDR_LENGTH];
    u_int8_t* data;
}aow_host_pkt_t;    
#define AOW_HOST_PKT_SIZE   sizeof(aow_host_pkt_t)

struct audio_pkt {
    u_int32_t signature;
    u_int32_t seqno;
    u_int64_t timestamp;
    u_int32_t pkt_len;
    AOW_PKT_TYPE pkt_type;
    u_int32_t params;
    u_int32_t audio_channel;
    ch_volume_data_t volume_info;
    u_int32_t crc32; /* This should always be the last element. */
}__packed;

/*
 * XXX : The following definitions are for command interface
 *       The exact details to follow soon, this is place holder
 *
 */

#define AOW_PROTO_PKT_LEN   10
#define AOW_PROTO_SUPER_PKT_LEN (AOW_PROTO_PKT_LEN * 3)

typedef struct aow_proto_pkt {
    u_int8_t    header;
    u_int8_t    address;
    u_int16_t   ch_info;
    u_int8_t    dlen;
    u_int8_t    data[AOW_PROTO_PKT_LEN];
    u_int8_t    chksum;
} aow_proto_pkt_t;    

typedef struct aow_proto_super_pkt {
    u_int8_t header;
    u_int8_t dlen;
    u_int8_t data[AOW_PROTO_SUPER_PKT_LEN];
} aow_proto_super_pkt_t;    


#define AOW_NI_CAPABLE          0x1
#define AOW_NI_DEV_TYPE_MASK    0x3
#define AOW_NI_DEV_TYPE_SOURCE  0x1
#define AOW_NI_DEV_TYPE_SINK    0x2
#define AOW_NI_DEV_TYPE_SHIFT   0x1
#define IS_NI_AOW_CAPABLE(ni)   ((ni->ni_aow.flags & AOW_NI_CAPABLE)?1:0)
#define GET_AOW_DEV_TYPE(ni)    ((ni->ni_aow.flags > AOW_NI_DEV_TYPE_SHIFT) & AOW_NI_DEV_TYPE_MASK)
#define SET_AOW_DEV_CAPABILITY(ni)  (ni->ni_aow.flags |= AOW_NI_CAPABLE)
#define CLR_AOW_DEV_CAPABILITY(ni)  (ni->ni_aow.flags &= ~AOW_NI_CAPABLE)



#define AOW_SIM_CTRL_CMD_CHANNEL_MASK   0x30
#define AOW_SIM_CTRL_CMD_SHIFT          4
#define AOW_SIM_CTRL_CMD_MASK           0xf
    
#define GET_AOW_PKT(b)   ((struct audio_pkt *)((u_int8_t*)(wbuf_raw_data(b)) + sizeof(struct ether_header)))
#define GET_AOW_DATA(b)  ((char*)(wbuf_raw_data(b)) + sizeof(struct ether_header) + sizeof(struct audio_pkt) + ATH_QOS_FIELD_SIZE)
#define GET_AOW_DATA_LEN(b) (wbuf_get_pktlen(b) - sizeof(struct ether_header) - sizeof(struct audio_pkt) - ATH_QOS_FIELD_SIZE)

#define GET_AOW_PROTO_CMD_TYPE(h)       (h & 0x80)
#define GET_AOW_PROTO_CMD_NUMBER(h)     ((h & 0x78) >> 3)
#define GET_AOW_PROTO_CMD_PKT_NUMBER(h) (h & 0x7)
#define IS_AOW_PROTO_CH_0_SET(c)        (c & 0x3)
#define IS_AOW_PROTO_CH_1_SET(c)        (c & 0xc)
#define IS_AOW_PROTO_CH_2_SET(c)        (c & 0x30)
#define IS_AOW_PROTO_CH_3_SET(c)        (c & 0xc0)
#define GET_AOW_PROTO_CH_0_INFO(c)      (c & 0x3)
#define GET_AOW_PROTO_CH_1_INFO(c)      ((c & 0xc) >> 2)
#define GET_AOW_PROTO_CH_2_INFO(c)      ((c & 0x30) >> 4)
#define GET_AOW_PROTO_CH_3_INFO(c)      ((c & 0xc0) >> 6)

#define    AOW_CMD_COM_AR_MOT_POWER_ON_LEN          2
#define    AOW_CMD_COM_AR_MOT_POWER_OFF_LEN         2
#define    AOW_CMD_REQ_AR_DAT_STATUS_LEN            2
#define    AOW_CMD_STT_AR_DAT_STATUS_LEN            13
#define    AOW_CMD_REQ_AR_DAT_SUB_UNIT_STATUS_LEN   2
#define    AOW_CMD_STT_AR_DAT_SUB_UNIT_STATUS_LEN   5
#define    AOW_CMD_REQ_AR_DAT_VERSION_LEN           3
#define    AOW_CMD_STT_AR_DAT_VERSION_LEN           11
#define    AOW_CMD_REQ_AR_DAT_PROFILE_LEN           2
#define    AOW_CMD_RES_AR_DATA_PROFILE_LEN          12
#define    AOW_CMD_REQ_AR_DAT_STREAM_INFO_LEN       2
#define    AOW_CMD_RES_AR_DAT_STREAM_INFO_LEN       6
#define    AOW_CMD_REQ_NW_STATUS_LEN              4
#define    AOW_CMD_STT_NW_STATUS_LEN              22
#define    AOW_CMD_REQ_NW_VERSION_LEN             5
#define    AOW_CMD_STT_NW_VERSION_LEN             13
#define    AOW_CMD_REQ_NW_VACS_CONFIG_LEN         5
#define    AOW_CMD_STT_NW_VACS_CONFIG_LEN         25
#define    AOW_CMD_LINK_COM_LINK_LEN                8
#define    AOW_CMD_LINK_STT_LINK_LEN                4

/* The number of elements in the enum is made 
 * as part of the enum defines
 */
typedef enum  {
    AOW_CMD_COM_AR_MOT_POWER_ON,
    AOW_CMD_COM_AR_MOT_POWER_OFF,
    AOW_CMD_REQ_AR_DAT_STATUS,
    AOW_CMD_STT_AR_DAT_STATUS,
    AOW_CMD_REQ_AR_DAT_SUB_UNIT_STATUS,
    AOW_CMD_STT_AR_DAT_SUB_UNIT_STATUS,
    AOW_CMD_REQ_AR_DAT_VERSION,
    AOW_CMD_STT_AR_DAT_VERSION,
    AOW_CMD_REQ_AR_DAT_PROFILE,
    AOW_CMD_RES_AR_DATA_PROFILE,
    AOW_CMD_REQ_AR_DAT_STREAM_INFO,
    AOW_CMD_RES_AR_DAT_STREAM_INFO,
    AOW_CMD_REQ_NW_STATUS,
    AOW_CMD_STT_NW_STATUS,
    AOW_CMD_REQ_NW_VERSION,
    AOW_CMD_STT_NW_VERSION,
    AOW_CMD_REQ_NW_VACS_CONFIG,
    AOW_CMD_STT_NW_VACS_CONFIG,
    AOW_CMD_LINK_COM_LINK,
    AOW_CMD_LINK_STT_LINK,
    NUM_AOW_CTRL_COMMANDS,
} AOW_COMMAND_TYPE_T;

/* L2 Packet Error Stats */
struct l2_pkt_err_stats {
    bool isInUse;
    bool wasAudStopped;
    u_int32_t srNo;
    u_int32_t fcs_ok;
    u_int32_t fcs_nok;
    systime_t start_time;
    systime_t end_time;
};

/* Packet Lost Rate Stats */
struct pkt_lost_rate_stats {
    bool isInUse;
    bool wasAudStopped;
    u_int32_t srNo;
    u_int32_t num_mpdus;      /* Orig stats: Number of MPDUs received
                              w/o_fail */
    u_int32_t nok_framecount; /* Orig stats: Number of MPDUs that were
                                    delayed*/
    u_int32_t late_mpdu;      /* Orig stats: Number of MPDUs that came
                                    late */
    u_int32_t start_seqno;
    u_int16_t start_subseqno; 
    u_int32_t end_seqno;
    u_int16_t end_subseqno; 
    systime_t start_time;
    systime_t end_time;
};

/* MCS statistics */

/* No. of MCS we are interested in, from MCS 0 upwards.
   If this is ever changed, keep the code in aow_update_mcsinfo()
   in sync with the changed value */
#define ATH_AOW_NUM_MCS             (16)


/* TODO: Re-architect the code to place it in lmac. Till then,
   use custom defined values and structures.*/

/* Start of custom definitions to be removed */

#define ATH_AOW_TXMAXTRY            (13)


typedef enum {
/* MCS Rates */
    AOW_ATH_MCS0 = 0x80,
    AOW_ATH_MCS1,
    AOW_ATH_MCS2,
    AOW_ATH_MCS3,
    AOW_ATH_MCS4,
    AOW_ATH_MCS5,
    AOW_ATH_MCS6,
    AOW_ATH_MCS7,
    AOW_ATH_MCS8,
    AOW_ATH_MCS9,
    AOW_ATH_MCS10,
    AOW_ATH_MCS11,
    AOW_ATH_MCS12,
    AOW_ATH_MCS13,
    AOW_ATH_MCS14,
    AOW_ATH_MCS15,
} AOW_ATH_MCS_RATES;

#define AOW_WME_BA_BMP_SIZE     64
#define AOW_WME_MAX_BA          AOW_WME_BA_BMP_SIZE

#define __bswap16(_x) ( (u_int16_t)( (((const u_int8_t *)(&_x))[0] ) |\
                         ( ( (const u_int8_t *)( &_x ) )[1]<< 8) ) )

#define GPIO_HIGH   1
#define GPIO_LOW    0

#define AOW_NEW_PKT 1
#define AOW_OLD_PKT !(AOW_NEW_PKT)

#define AOW_RX_CHANNEL_SELECT_MASK  0x10
#define AOW_RX_CHANNEL_VALUE_MASK   0xf

/*
 * desc accessor macros
 */
#define ATH_AOW_DS_BA_SEQ(_ts)          (_ts)->ts_seqnum
#define ATH_AOW_DS_BA_BITMAP(_ts)       (&(_ts)->ba_low)
#define ATH_AOW_DS_TX_BA(_ts)           ((_ts)->ts_flags & HAL_TX_BA)
#define ATH_AOW_DS_TX_STATUS(_ts)       (_ts)->ts_status
#define ATH_AOW_DS_TX_FLAGS(_ts)        (_ts)->ts_flags

#define ATH_AOW_BA_INDEX(_st, _seq)     (((_seq) - (_st)) & (IEEE80211_SEQ_MAX - 1))
#define ATH_AOW_BA_ISSET(_bm, _n)       (((_n) < (AOW_WME_BA_BMP_SIZE)) &&          \
                                        ((_bm)[(_n) >> 5] & (1 << ((_n) & 31))))

/* End of custom definitions to be removed */

struct mcs_stats_element {
    u_int32_t ok_txcount[ATH_AOW_TXMAXTRY];
    u_int32_t nok_txcount[ATH_AOW_TXMAXTRY];
};

struct mcs_stats {
    struct ether_addr recvr_addr;
    struct mcs_stats_element mcs_info[ATH_AOW_NUM_MCS];
    int populated;
};

#define ATH_AOW_GET_AOWPKTINDEX(_sc, _wbuf) \
                                          ((_sc)->sc_ieee_ops->ieee80211_aow_apktindex( \
                                                    (struct ieee80211com*)(\
                                                        (_sc)->sc_ieee),  \
                                                    (_wbuf)))

#define ATH_AOW_ACCESS_APKT(_wbuf, _index) \
                                          ((struct audio_pkt *)\
                                             (((char*)(wbuf_header((_wbuf)))) + \
                                              (_index)))

#define ATH_AOW_ACCESS_APKT_SEQ(_wbuf, _index) \
                                          (ATH_AOW_ACCESS_APKT( \
                                               (_wbuf),(_index))->seqno)

#define ATH_AOW_ACCESS_APKT_SUBSEQ(_wbuf, _index) \
                                          ((ATH_AOW_ACCESS_APKT( \
                                               (_wbuf),(_index))->params) \
                                           & ATH_AOW_PARAMS_SUB_FRAME_NO_MASK)

#define ATH_AOW_ACCESS_RECVR(_wbuf)       ((struct ether_addr*) \
                                             (((struct ieee80211_frame *) \
                                                wbuf_header((_wbuf)))->i_addr1))

#define ATH_AOW_ACCESS_WLANSEQ(_wbuf)     ((le16toh(*(u_int16_t *) \
                                            (((struct ieee80211_frame *) \
                                               wbuf_header((_wbuf)))->i_seq))) \
                                            >> IEEE80211_SEQ_SEQ_SHIFT)

/* defines */
#define ATH_AOW_SEQNO_LIMIT             4096

#define ATH_AOW_SIGNATURE               0xdeadbeef
#define ATH_AOW_RESET_COMMAND           0xa5a5a5a5
#define ATH_AOW_START_COMMAND           0xb5b5b5b5
#define ATH_AOW_SET_CHANNEL_COMMAND     0x00000001

#define ATH_AOW_SIG_SIZE                sizeof(u_int32_t)
#define ATH_AOW_TS_SIZE                 sizeof(u_int64_t)
#define ATH_QOS_FIELD_SIZE              sizeof(u_int16_t)
#define ATH_AOW_SEQNO_SIZE              sizeof(u_int32_t)
#define ATH_AOW_LENGTH_SIZE             sizeof(u_int16_t)
#define ATH_AOW_PARAMS_SIZE             sizeof(u_int16_t)

#define ATH_AOW_PARAMS_SUB_FRAME_NO_S       (0)
#define ATH_AOW_PARAMS_SUB_FRAME_NO_MASK    (0x3)
#define ATH_AOW_PARAMS_INTRPOL_ON_FLG_MASK  (0x1)
#define ATH_AOW_PARAMS_INTRPOL_ON_FLG_S     (2)
#define ATH_AOW_PARAMS_SAMP_SIZE_MASK       (0x1f)
#define ATH_AOW_PARAMS_SAMP_SIZE_MASK_S     (3)
#define ATH_AOW_FRAME_SIZE_MASK             (0x1)
#define ATH_AOW_FRAME_SIZE_S                (8)
#define ATH_AOW_PARAMS_LOGSYNC_FLG_MASK     (0x1)
#define ATH_AOW_PARAMS_LOGSYNC_FLG_S        (9)

/* Maximum sample size is 4 byets */
#define ATH_AOW_SAMPLE_SIZE_MAX (3)

/* Stereo sample size to write to I2S */
#define AOW_I2S_WRITE_STREO_SAMPLE_SIZE         12
#define AOW_AUDIO_SYNC_SAMPLE_CORRECTION_COUNT  6

#define AOW_SET_AUDIO_SYNC_CTRL_STATE(ic, state)        ((ic->ic_aow.ctrl.aow_state) = (state))
#define AOW_SET_AUDIO_SYNC_CTRL_SUB_STATE(ic, state)    ((ic->ic_aow.ctrl.aow_sub_state) = (state))
#define AOW_GET_AUDIO_SYNC_CTRL_STATE(ic)               (ic->ic_aow.ctrl.aow_state)
#define AOW_GET_AUDIO_SYNC_CTRL_SUB_STATE(ic)           (ic->ic_aow.ctrl.aow_sub_state)
#define get_aow_wr_ptr(ic)                              (ic->ic_aow.audio_data_wr_ptr)
#define get_aow_rd_ptr(ic)                              (ic->ic_aow.audio_data_rd_ptr)
#define set_aow_wr_ptr(ic, ptr)                         ((ic->ic_aow.audio_data_wr_ptr) = (ptr))
#define set_aow_rd_ptr(ic, ptr)                         ((ic->ic_aow.audio_data_rd_ptr) = (ptr))


/* 
 * This are the defines for the MPDU/AMPDU structure. 
 * There are 4 MPDUs, with 2 ms worth of 
 * data per MPDU. 
 */
/* Assuming 48k sampling rate and 4 MPDUs/audio frame */
#define ATH_AOW_NUM_MPDU_LOG2       (2)
#define ATH_AOW_NUM_MPDU            (1<<ATH_AOW_NUM_MPDU_LOG2)

#define ATH_AOW_NUM_SAMP_PER_AMPDU   (192*4)
#define ATH_AOW_NUM_SAMP_PER_MPDU   (192)
#define ATH_AOW_MPDU_MOD_MASK       ((1 << ATH_AOW_NUM_MPDU_LOG2)-1)

#define ATH_AOW_STATS_THRESHOLD     (10000)

#define ATH_AOW_PLAY_LOCAL_MASK 1

#define AUDIO_INFO_BUF_SIZE_LOG2    (2) 
#define AUDIO_INFO_BUF_SIZE         (1<<AUDIO_INFO_BUF_SIZE_LOG2)
#define AUDIO_INFO_BUF_SIZE_MASK    (AUDIO_INFO_BUF_SIZE-1)
#define AOW_ZERO_BUF_IDX            (AUDIO_INFO_BUF_SIZE)

#define AUDIO_INFO_BUF_SIZE_1_FRAME_LOG2    (5) 
#define AUDIO_INFO_BUF_SIZE_1_FRAME         (1<<AUDIO_INFO_BUF_SIZE_1_FRAME_LOG2)
#define AUDIO_INFO_BUF_SIZE_1_FRAME_MASK   (AUDIO_INFO_BUF_SIZE_1_FRAME-1)

#define AOW_MAX_MISSING_PKT         (20)
#define AOW_MAX_MISSING_PKT_1_FRAME (20*4)

#define AOW_MAX_PLAYBACK_SIZE       (12)


/* All the times below are in micro seconds */
#define AOW_PKT_RX_PROC_THRESH_1_FRAME (4000)
#define AOW_PKT_RX_PROC_BUFFER      (500)
/* AOW_PKT_TX_THRESH_1_FRAME defines the maximum time to wait before the tx timeout value is reached */
#define AOW_PKT_TX_THRESH_1_FRAME (AOW_PKT_RX_PROC_THRESH_1_FRAME + 3000)

/* Buffer for Rx timeout calculation, in usec */
#define ATH_AOW_RX_TIMEOUT_BUFFER   (AOW_PKT_RX_PROC_BUFFER + 1000)

/* AoW Latency limits in microseconds */
/* Note: Keep these limits in sync with no. of available audio buffers */
#define AOW_MIN_LATENCY             (17000)
#define AOW_MAX_LATENCY             (32000)

#define AOW_DYNRXTIMEOUT_THRESHOLD  1000   /* In usec */

/* print macros */
#define IEEE80211_AOW_DPRINTF   printk
#define IEEE80211_AOW_INFO_DPRINTF(fmt, args...)   \
                                    printk(KERN_DEBUG fmt, ##args)

/* 
 * Frequency at which errors are simulated. 
 * The subframes for one out of every AOW_EC_ERROR_SIMULATION_FREQUENCY 
 * frames are subjected to errors as given in fmap. 
 */ 
#define AOW_EC_ERROR_SIMULATION_FREQUENCY (10)

#define IS_ETHER_ADDR_NULL(macaddr) (((macaddr[0] == 0 ) &&  \
                                      (macaddr[1] == 0 ) &&  \
                                      (macaddr[2] == 0 ) &&  \
                                      (macaddr[3] == 0 ) &&  \
                                      (macaddr[4] == 0 ) &&  \
                                      (macaddr[5] == 0 ))?TRUE:FALSE)


/* audio_params defination
 * This structure contains the definations of different parameters 
 * for the audio tx, Rx and playback based on rate and sample size
 */
 typedef struct _audio_params {
     /* This is a self index to help locate it in an array */
     u_int16_t audio_param_id;
     /* samples/ms, based on sampling rate -- Stereo
     * 48k samples would be 96, 96k sample rate would be 192 and so forth
     */
    u_int16_t samples_per_ms; 
             
    /* sample_size_bits 
     * Size of each sample in bits 
     */
    u_int16_t sample_size_bits;
    /* sample_size_bytes
     * Sample size in bytes 
     */
    u_int16_t sample_size_bytes;
    
    /* sample_size_bytes_stereo holds to sample size in stereo */
    u_int16_t sample_size_bytes_stereo;
    
    /* mpdu_time_slice gives the audio data per mpdu in micro sec */
    u_int16_t mpdu_time_slice;
    
    /* mpdu_size gives the size of the MPDU in 16 bit word. This is
     * dependent on the rate and sample size. 16 bit word is chosen because
     * it is more efficient then bytes and if we assume stereo, it should always
     * be short boundry alligned 
     */
    u_int16_t mpdu_size;
    
    /* mpdu_samples gives the number of samples in every MPDU. It is based on the sampling rate 
    */
    u_int16_t mpdu_samples;
    
    
    /* mpdu_per_audio_frame 
     * This is based on the interleaving depth. If no interleaving is used, it is always 1. 
     */ 
    u_int16_t mpdu_per_audio_frame;
    
    /* deint_buf_size --
     * This buffer is used to store de-interleaved data. 
     * It is based on mpdu_size as well as rate, sample size and mpdu_per_audio_frame
     */
    u_int16_t dient_buf_size;
    
    /* sample_per_audio_frame --
     * Based on the number of mpdus/audio frame and number of samples per mpdu */
    u_int16_t sample_per_audio_frame; 
    
    /* i2s_audio_buf_size -- Holds the audio buffers before it goes into the I2S buffer */
    u_int16_t i2s_audio_buf_size;
    
    /* i2s_write_size -- The amount of data to be written per i2s discriptor, in bytes, based on the
     * sampling rate and size 
     */
    u_int16_t i2s_write_size;
    
    /* clock_sync_drop_size -- the number of 16 bit words to drop for clock sync. This is based on 
     * sample size and clock drift
     */
    u_int16_t clock_sync_drop_size;
    /* clock_correction_sz -- Number of stereo samples to be added/removed for clock synchronization */
    u_int16_t clock_correction_sz;
    
    /* start_mute_sz -- The size of the start mute region. This is same as 2ms worth of stereo data 
     * The size is number of 16 bit words 
     */
    u_int16_t start_mute_sz;
    /* Ramp up/down size is the number of 16 bit words that are used for ramp up/ramp down. 
     * This would depend on the sample size and is computed in MONO
     */
    u_int16_t ramp_size;
 } audio_params_t;

typedef struct _audio_info_1_frame {
    u_int32_t seqNo;
    u_int16_t WLANSeqNos[ATH_AOW_NUM_MPDU];
    u_int16_t foundBlocks;    
    u_int16_t audBuffer[ATH_AOW_NUM_SAMP_PER_MPDU*2];
    u_int16_t inUse;
    u_int16_t params;
    u_int16_t datalen;
    u_int64_t startTime;
//    u_int8_t  localSimCount; 
    bool      logSync[ATH_AOW_NUM_MPDU];
#ifdef ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
    u_int64_t rxTime[ATH_AOW_NUM_MPDU];
    u_int64_t deliveryTime[ATH_AOW_NUM_MPDU];
#endif 
} audio_info_1_frame;


typedef enum aow_state {
    AOW_STOP_STATE,
    AOW_START_STATE,
    AOW_IDLE_STATE,
    AOW_CONSUME_STATE,
    AOW_I2S_PROGRAM_STATE,
    AOW_I2S_START_STATE,
} aow_state_t;

typedef enum aow_sub_state {
    AOW_START_RECEIVE_STATE,
    AOW_STEADY_RECEIVE_STATE,
    AOW_STOP_RECEIVE_STATE,
} aow_sub_state_t;    

typedef struct _audio_control {

    /* AOW State realted */
    aow_state_t aow_state;
    aow_sub_state_t aow_sub_state;
    u_int32_t last_seqNo;
    u_int32_t num_missing;
    u_int16_t ramp_down_set;
    u_int32_t first_rx_seq;
    u_int32_t capture_data;
    u_int32_t capture_done;
    u_int32_t force_input;
    u_int16_t pkt_drop_sim_cnt;
} audio_ctrl;

#define AOW_DBG_WND_LEN (256)
typedef struct _audio_stats {
    u_int16_t rxFlg;
    u_int32_t num_ampdus;
    u_int32_t num_mpdus;
    u_int32_t num_missing_ampdu;
    u_int32_t num_missing_mpdu;
    u_int32_t num_missing_mpdu_hist[ATH_AOW_NUM_MPDU];
#if ATH_SUPPORT_AOW_TIMESTAMP_DELTA_DEBUG
    /* Tiny positive delta between cur TSF and timestamp */
    u_int32_t timestamp_tiny_pve_delta; 
    /* Negative delta between cur TSF and timestamp */
    u_int32_t timestamp_nve_delta; 
    /* Zero delta between cur TSF and timestamp */
    u_int32_t timestamp_zero_delta; 
#endif
    u_int32_t late_mpdu;
    u_int32_t mpdu_not_consumed;
    u_int32_t ampdu_size_short;
    u_int32_t too_late_ampdu;
    u_int32_t datalenCnt;
    u_int64_t lastPktTime;
    u_int64_t expectedPktTime;
    u_int32_t numPktMissing;
    u_int32_t datalenBuf[AOW_DBG_WND_LEN];
    u_int32_t datalenLen[AOW_DBG_WND_LEN];
    u_int32_t dbgCnt;
    u_int32_t frcDataCnt;
    u_int64_t prevTsf;
    u_int32_t sumTime;
    u_int32_t grtCnt;
    u_int32_t consume_aud_cnt;
    u_int32_t consume_aud_abort;
} audio_stats;

#define AOW_SYNC_COUNTER_THRESHOLD              125
#define AOW_SYNC_COUNTER_THRESHOLD_1_FRAME      500
#define AOW_SYNC_EXPECTED_CONSUMED_SAMPLE_COUNT (ATH_AOW_NUM_SAMP_PER_MPDU * AOW_SYNC_COUNTER_THRESHOLD_1_FRAME)
#define AOW_SYNC_EXPECTED_CONSUME_TIME_LIMIT    1001000
#define AOW_SYNC_EXPECTED_CONSUME_TIME_LOW_WM   999800
#define AOW_SYNC_CONSUME_TIME_DELTA             500
#define AOW_SYNC_USED_DESC_EXPECTED (32)
#define AOW_SYNC_USED_DESC_DELTA (3)
#define AOW_SYNC_USED_DESC_MIN_THRES (AOW_SYNC_USED_DESC_EXPECTED - AOW_SYNC_USED_DESC_DELTA)
#define AOW_SYNC_USED_DESC_MAX_THRES (AOW_SYNC_USED_DESC_EXPECTED + AOW_SYNC_USED_DESC_DELTA)
#define AOW_SYNC_SAMPLE_ADJUST_SIZE (6)
#define AOW_SYNC_CORRECTION_TIME_UNIT (19)
/* This is used as rounding factor before fixed point divide*/
#define AOW_SYNC_CORRECTION_FACTOR              19 
/* This is the fixed point representation of 1 sample period at 48k 
 * samples in micro seconds 
 */
#define AOW_SYNC_SAMPLE_ADJUST_LIMIT            192
#define AOW_SYNC_INITIAL_SAMPLE_ADJUST_LIMIT    192
#define AOW_SYNC_SAMPLE_ADJUST_COUNT            5
#define AOW_MUTE_RAMP_DWN_NUM_TAPS              32
/* The values below are the sample size * num taps * 2 * num taps for 
 * stereo, in words */
#define AOW_MUTE_RAMP_DOWN_SIZE_16_BIT          (32*2)
#define AOW_MUTE_RAMP_DOWN_SIZE_24_BIT          (48*2)

typedef struct sync_stats
{
    u_int32_t overflow;
    u_int32_t underflow;
    u_int32_t i2s_sample_repeat_count;
    u_int32_t i2s_sample_dropped_count;

#define MAX_WLAN_PTR_LOG (200)
    int32_t wlan_wr_ptr_before[MAX_WLAN_PTR_LOG];
    int32_t wlan_wr_ptr_after[MAX_WLAN_PTR_LOG];
    int32_t wlan_rd_ptr_before[MAX_WLAN_PTR_LOG];
    int32_t wlan_rd_ptr_after[MAX_WLAN_PTR_LOG];
    int32_t i2s_disCnt[MAX_WLAN_PTR_LOG];
    int32_t i2s_timeDiff[MAX_WLAN_PTR_LOG];
    u_int32_t wlan_data_buf_idx;
    u_int32_t wlan_data_log_idx;
    u_int64_t prev_tsf;
    u_int32_t samp_consumed;
    u_int32_t time_consumed;
    u_int32_t sec_counter;
    u_int32_t startState;
    int32_t samp_to_adj;
    u_int32_t words_to_write;
    int32_t tot_samp_adj;    
    int32_t filt_val;
    u_int16_t filt_cnt;
} sync_stats_t;

typedef struct _ext_stats
{
    /* Packet Latency stats */
    u_int32_t *aow_latency_stats;
    u_int32_t aow_latency_max_bin;
    u_int32_t aow_latency_stat_expired;

    /* L2 Packet Error Stats*/
    
    /* Array for L2 Packet Error Stats FIFO */
    struct l2_pkt_err_stats *aow_l2pe_stats;
    
    /* FIFO array position to write next element to. */
    u_int32_t aow_l2pe_stats_write; 
    
    /* FIFO array position to read next element from. */
    u_int32_t aow_l2pe_stats_read;
    
    /* Current L2 Packet Error Stats Group (to be written into
       for the current interval of ATH_AOW_L2PE_GROUP_INTERVAL seconds) */
    struct l2_pkt_err_stats l2pe_curr;
    
    /* Cumulative L2 Packet Error Stats Group (to be written into
       for the entire duration of measurement). */
    struct l2_pkt_err_stats l2pe_cumltv;

    /* Serial No. to use for the next L2 Packet Error Stats Group */
    u_int32_t l2pe_next_srNo;

    
    /* Packet Lost Rate Stats*/
    
    /* Array for Packet Lost Rate Stats FIFO */
    struct pkt_lost_rate_stats *aow_plr_stats;
    
    /* FIFO array position to write next element to. */
    u_int32_t aow_plr_stats_write; 
    
    /* FIFO array position to read next element from. */
    u_int32_t aow_plr_stats_read;
    
    /* Current Packet Lost Rate Stats Group (to be written into
       for the current interval of ATH_AOW_ES_GROUP_INTERVAL seconds) */
    struct pkt_lost_rate_stats plr_curr;
    
    /* Cumulative Packet Lost Rate Stats Group (to be written into
       for the entire duration of measurement). */
    struct pkt_lost_rate_stats plr_cumltv;

    /* Serial No. to use for the next Packet Lost Rate Stats Group */
    u_int32_t plr_next_srNo;

    /* MCS statistics */
    struct mcs_stats aow_mcs_stats[AOW_MAX_RECVRS];
    

} ext_stats;

/* TODO : This needs to move to common header
 * file between app and driver module
 */
#define ATH_AOW_MAX_BUFFER_SIZE     786
#define ATH_AOW_MAX_BUFFER_ELEMENTS 16
#define AOW_I2S_DATA_BLOCK_SIZE     24

#define I2S_START_CMD   0x1
#define I2S_STOP_CMD    0x2
#define I2S_PAUSE_CMD   0x4
#define I2S_DMA_START   0x8

#define SET_I2S_START_FLAG(flag)    ((flag) = (flag) | (I2S_START_CMD))
#define CLR_I2S_START_FLAG(flag)    ((flag) = (flag) & ~(I2S_START_CMD))
#define SET_I2S_STOP_FLAG(flag)     ((flag) = (flag) | (I2S_STOP_CMD))
#define CLR_I2S_STOP_FLAG(flag)     ((flag) = (flag) & ~(I2S_STOP_CMD))
#define SET_I2S_PAUSE_FLAG(flag)    ((flag) = (flag) | (I2S_PAUSE_CMD))
#define CLR_I2S_PAUSE_FLAG(flag)    ((flag) = (flag) & ~(I2S_PAUSE_CMD))
#define SET_I2S_DMA_START(flag)     ((flag) = (flag) | (I2S_DMA_START))
#define CLR_I2S_DMA_START(flag)     ((flag) = (flag) & ~(I2S_DMA_START))

#define IS_I2S_START_DONE(flag)     (((flag) & I2S_START_CMD)?1:0)
#define IS_I2S_STOP_DONE(flag)      (((flag) & I2S_STOP_CMD)?1:0)
#define IS_I2S_NEED_DMA_START(flag) (((flag) & I2S_DMA_START)?1:0)



/* function declarations */


/**
 * @brief       init routine for CI function
 * @param[in]   ic   : handle ic data structure
 */
extern int aow_init_ci(struct ieee80211com *ic);

/**
 * @brief       Deinit routine for CI function
 * @param[in]   ic   : handle ic data structure
 */
extern void aow_deinit_ci(struct ieee80211com* ic);

/**
 * @brief       Send the AoW command message to CI app
 * @param[in]   ic   : handle ic data structure
 * @param[in]   data : handle the message 
 * @param[in]   size : size of the message
 */
extern int aow_ci_send(struct ieee80211com *ic, char *data, int size);

extern int init_aow_ie(struct ieee80211com* ic);
extern void print_aow_ie(struct ieee80211com* ic); 
extern void ieee80211_update_node_aow_info(struct ieee80211_node* ni, aow_ie_t *ie);

extern u_int32_t ieee80211_get_aow_seqno(struct ieee80211com *ic);
extern u_int32_t ieee80211_aow_sim_ctrl_msg(struct ieee80211com* ic, u_int32_t val);
extern u_int32_t ieee80211_aow_get_ctrl_cmd(struct ieee80211com* ic);

extern void aow_print_ctrl_pkt(aow_proto_pkt_t* pkt);
extern void ieee80211_aow_hexdump(unsigned char *buf, unsigned int len);
extern void ieee80211_aow_attach(struct ieee80211com *ic);
extern void ieee80211_audio_print_capture_data(struct ieee80211com *ic);
extern void ieee80211_set_audio_data_capture(struct ieee80211com *ic);
extern void ieee80211_set_force_aow_data(struct ieee80211com *ic, int32_t flg);
extern void ieee80211_audio_stat_clear(struct ieee80211com *ic);
extern void ieee80211_audio_stat_print(struct ieee80211com *ic);

extern u_int32_t aow_get_channel_id(struct ieee80211com* ic, aow_proto_pkt_t* pkt);

extern wbuf_t ieee80211_get_aow_frame(struct ieee80211_node* ni, u_int8_t **frm, u_int32_t pktlen);
extern int ieee80211_aow_get_stats(struct ieee80211com *ic);
extern int ieee80211_audio_receive(struct ieee80211vap *vap, wbuf_t wbuf, struct ieee80211_rx_status *rs);
extern int ieee80211_aow_detach(struct ieee80211com *);
extern int ieee80211_aow_cmd_handler(struct ieee80211com* ic, int command);
extern int32_t ieee80211_aow_ctrl_cmd_handler(struct ieee80211com *ic, wbuf_t wbuf,
                                       struct ieee80211_rx_status *rs);
extern int aow_i2s_init(struct ieee80211com* ic);
extern int aow_i2s_deinit(struct ieee80211com* ic);
extern int ieee80211_consume_audio_data(struct ieee80211com *ic, u_int64_t curTime);
extern u_int32_t ieee80211_i2s_write_interrupt(struct ieee80211com *ic);
extern int16_t ieee80211_aow_apktindex(struct ieee80211com *ic, wbuf_t wbuf);
extern int32_t ieee80211_aow_send_to_host(struct ieee80211com *ic, u_int8_t* data, u_int32_t len, u_int16_t type, u_int16_t subtype,
u_int8_t* addr);
extern int i2s_audio_data_stop(void);
extern void init_debug_dummy_data(void);
extern int init_aow_verinfo(struct ieee80211com* ic);
extern int32_t ieee80211_aow_request_version(struct ieee80211com* ic);
extern int ieee80211_aow_update_volume(struct ieee80211com* ic, u_int8_t* data, u_int32_t len);
extern int ieee80211_aow_join_indicate(struct ieee80211com* ic, STA_JOIN_STATE_T state, struct ieee80211_node *ni);

/******************************************************************************
    Functions relating to AoW PoC 2
    Interface functions between USB and Wlan

******************************************************************************/    

extern int aow_get_macaddr(int channel, int index, struct ether_addr* addr);
extern void wlan_aow_set_audioparams(audio_type_t audiotype);
extern int wlan_aow_tx(char* data, int datalen, int channel, u_int64_t tsf);
extern int ieee80211_aow_setframesize_to_usb(unsigned int framesize);
extern int ieee80211_aow_setaltsetting_to_usb(unsigned int altsetting);
#if ATH_SUPPORT_AOW_TXSCHED
extern int wlan_aow_dispatch_data(void);
extern int ieee80211_aow_tx_stage(struct ieee80211com *ic, wbuf_t wbuf);
#endif
extern int ieee80211_send_aow_data_ipformat(struct ieee80211_node *ni, void *data, 
                        int len, u_int32_t seqno, u_int64_t tsf, u_int32_t channel, bool setlogSync);
extern int ieee80211_send_aow_data_ipformat_new(struct ieee80211_node *ni, void *data, 
                                            int len, u_int32_t seqno, u_int64_t tsf, u_int32_t channel, bool setlogSync);
extern int ieee80211_send_aow_ctrl_ipformat(struct ieee80211_node *ni, void *data, 
                        u_int32_t len, u_int32_t seqno, u_int64_t tsf, u_int32_t channel, bool setlogSync);
extern u_int32_t ieee80211_aow_tx_ctrl(u_int8_t* data, u_int32_t datalen, u_int64_t tsf);
extern int aow_register_usb_calls_to_wlan(void* recv_data,
                                          void* recv_ctrl,
                                          void* set_frame_size,
                                          void *set_alt_setting);
extern int is_aow_usb_calls_registered(void);
extern u_int32_t ieee80211_aow_disconnect_device(struct ieee80211com* ic, u_int32_t channel);

/* 
 * XXX : This is the interface between wlan and i2s
 *       When adding info, here please make sure that
 *       it is reflected in the i2s side also
 */
typedef struct i2s_stats {
    u_int32_t write_fail;
    u_int32_t rx_underflow;
    u_int32_t aow_sync_enabled;
    u_int32_t sync_buf_full;
    u_int32_t sync_buf_empty;
    u_int32_t tasklet_count;
    u_int32_t repeat_count;
} i2s_stats_t;    

extern void ieee80211_aow_i2s_stats(struct ieee80211com* ic);
extern void ieee80211_aow_ec_stats(struct ieee80211com* ic);
extern void ieee80211_aow_clear_i2s_stats(struct ieee80211com* ic);
extern void i2s_get_stats(struct i2s_stats *p);
extern void i2s_clear_stats(void);
extern void ieee80211_aow_sync_stats(struct ieee80211com *ic);

#define IEEE80211_ENAB_AOW(ic)  (1)

/* Error Recovery related */
#define AOW_ER_ENAB(ic)     (ic->ic_get_aow_er(ic))
#define AOW_EC_ENAB(ic)     (ic->ic_get_aow_ec(ic))
#define AOW_EC_RAMP(ic)     (ic->ic_get_aow_ec_ramp(ic))
#define AOW_AS_ENAB(ic)     (ic->ic_get_aow_as(ic))

#define IS_AOW_ASSOC_SET(ic)  (ic->ic_get_aow_assoc_policy(ic))

#define AOW_ER_DATA_VALID   0x1
#define AOW_ER_MAX_RETRIES  3

#define AOW_SUB_FRAME_IDX_0 0
#define AOW_SUB_FRAME_IDX_1 1
#define AOW_SUB_FRAME_IDX_2 2
#define AOW_SUB_FRAME_IDX_3 3

#define AOW_ER_DATA_OK  (1)
#define AOW_ER_DATA_NOK (0)

enum { ER_IDLE, ER_IN, ER_OUT };

typedef struct aow_er {
    int cstate;             /* current er state */
    int expected_seqno;     /* next expected sequence number */
    int flag;               /* control flags */
    int datalen;            /* data length */
    int count;              /* error packet, occurance count */
    
    /* stats */
    int bad_fcs_count;      /* indicates number of bad fcs frame received */
    int recovered_frame_count;  /* indicates number of recovered frames */
    int aow_er_crc_valid;       /* valid aow crc, in bad fcs frame */
    int aow_er_crc_invalid;     /* invalid aow crc, in bad fcs frame */

    /* lock related */
    spinlock_t  lock;     /* AoW ER lock */
    unsigned long lock_flags;

    unsigned char data[ATH_AOW_NUM_SAMP_PER_AMPDU];    /* pointer to data */
} aow_er_t;    

typedef struct aow_ec {

    u_int32_t index0_bad;
    u_int32_t index1_bad;
    u_int32_t index2_bad;
    u_int32_t index3_bad;

    u_int32_t index0_fixed;
    u_int32_t index1_fixed;
    u_int32_t index2_fixed;
    u_int32_t index3_fixed;

    /* place holder */
    u_int16_t prev_data[ATH_AOW_NUM_SAMP_PER_MPDU];
    u_int16_t predicted_data[ATH_AOW_NUM_SAMP_PER_MPDU];
} aow_ec_t;

#define AOW_MAX_DATABUFFER_SIZE     24
#define AOW_MAX_SYNC_BUFFERS        256
#define AOW_NUM_PKTS_PER_EIGHTMS    64
typedef  struct _buffer {
    int     length;
    char    data[AOW_MAX_DATABUFFER_SIZE];
} _buffer_t;    

typedef struct wlan_sync_buffers {
    int read_index;
    int write_index;
    int count;
    int prev_index;
    _buffer_t buf[AOW_MAX_SYNC_BUFFERS];
} wlan_sync_buffers_t;

#if ATH_SUPPORT_AOW_TXSCHED

/* Number of AoW Staging entries to keep ready,
   per receiver */
#define AOW_NUM_AWS_ENTRIES_PER_RECVR   (3)

#define AOW_MAX_AWS_ENTRIES             (AOW_MAX_RECEIVER_COUNT * \
                                         AOW_NUM_AWS_ENTRIES_PER_RECVR)

typedef struct _aow_wbuf_staging_entry {
    int staging_order;
    struct ieee80211_node *ni;
    wbuf_t wbuf;
} aow_wbuf_staging_entry;
#endif

typedef struct ieee80211_aow {

    /* stats related */
    u_int16_t gpio11Flg;
    u_int32_t ok_framecount;
    u_int32_t nok_framecount;  
    u_int32_t tx_framecount;
    u_int32_t tx_ctrl_framecount;
    u_int32_t rx_ctrl_framecount;
    u_int32_t macaddr_not_found;
    u_int32_t node_not_found;
    u_int32_t channel_set_flag; /* holds the set channel mask, for fast tx check */
    u_int32_t i2s_open_count;
    u_int32_t i2s_close_count;
    u_int32_t i2s_dma_start;

    /* pointer to current audio pointer */
    audio_params_t *cur_audio_params;
    
    /* control flags */
    u_int32_t i2sOpen;    

    /* bit 0 - START 
     * bit 1 - RESET
     * bit 2 - PAUSE
     * bit 3 - DMA START NEEDED
     */
    u_int32_t i2s_flags;
    u_int32_t i2s_dmastart;     /* This flag indicates the i2s DMA need to started */

    spinlock_t   lock;
    unsigned long lock_flags;
    
    spinlock_t aow_lh_lock;          /* Lock control for Latency Histogram */
    spinlock_t aow_essc_lock;        /* Lock control for ESS Counts */
    spinlock_t aow_l2pe_lock;        /* Lock control for L2 Packet Error
                                        Measurement */
    spinlock_t aow_plr_lock;         /* Lock control for Packet Lost Rate
                                        Measurement */
    spinlock_t aow_mcsmap_lock;      /* Lock control for Receiver Address to
                                        MCS Entry Group mapping. */
#if ATH_SUPPORT_AOW_TXSCHED
    spinlock_t aow_wbuf_staging_lock; /* Lock control for wbuf staging */
#endif

    audio_info_1_frame info_1_frame[AUDIO_INFO_BUF_SIZE_1_FRAME];
    u_int32_t last_played_frame;
    audio_ctrl ctrl;

    u_int16_t audio_data_deintBuf[ATH_AOW_SAMPLE_SIZE_MAX * ATH_AOW_NUM_SAMP_PER_AMPDU];

    u_int16_t zero_data[ATH_AOW_SAMPLE_SIZE_MAX * ATH_AOW_NUM_SAMP_PER_AMPDU];
#define AUDIO_WLAN_BUF_LEN (ATH_AOW_NUM_SAMP_PER_MPDU * ATH_AOW_SAMPLE_SIZE_MAX)

    u_int16_t audio_data_buf[AUDIO_WLAN_BUF_LEN]; 
    u_int16_t ramp_down_buf[AOW_MUTE_RAMP_DOWN_SIZE_24_BIT];
    u_int32_t audio_data_wr_ptr;
    u_int32_t audio_data_rd_ptr;

    u_int64_t playTime;
    
    audio_stats stats;      /* AoW Stats */

    ext_stats estats;       /* AoW Extended Stats */

    sync_stats_t sync_stats;    /* Sync Stats */

    /* Extended statistics synchronization - (remaining) no. of audio frames
       for which synchronized logging is to be carried out. */
    u_int32_t ess_count[AOW_MAX_AUDIO_CHANNELS];

    /* Node type, whether transmitter or receiver. */
    u_int16_t node_type;

    /* Netlink related data structure. */
    void  *aow_nl_sock;

    /* Netlink sock interface to command interpreter */
    void  *aow_ci_sock;

    /* Feature Flags */
    u_int16_t   interleave;     /* Interleave/Deinterleave feature */
//    aow_er_t    er;                 /* Error Recovery */
//    aow_ec_t    ec;                 /* Error Concealment */

    /* Audio Parameters */
    u_int16_t   frame_size;     /* Audio Frame Size */
    u_int16_t   frame_size_set; /* Whether Audio Frame Size has been set.*/

    /* TEMPORARY: This is a workaround for some USB issues encountered
       when changing alt settings. */
    u_int16_t   alt_setting;      /* altsetting to be expected by USB. */
    u_int16_t   alt_setting_set;  /* Whether USB altsetting has been configured.*/

    /* AoW IE Template */
    aow_ie_t ie;

#if ATH_SUPPORT_AOW_TXSCHED
    /* wbuf staging entries for Dynamic Tx Scheduling. 
       We use a static array to increase staging efficiency.
       The space cost is small. */
    aow_wbuf_staging_entry aws_entry[AOW_MAX_AWS_ENTRIES];
    
    /* Pointer array for reduced-copy sorting. */
    aow_wbuf_staging_entry *aws_entry_ptr[AOW_MAX_AWS_ENTRIES];
    
    int aws_entry_count;
#endif

    u_int32_t version;
    ch_volume_data_t volume_info;
    ch_volume_data_t rx_volume_info;

} ieee80211_aow_t;

typedef struct ieee80211_vap_aow_info {
    /** Id used to set and get Application IE's */
    struct wlan_mlme_app_ie *app_ie_handle;
} ieee80211_vap_aow_info_t;    
    


#define SET_I2S_OPEN_STATE(_ic, val)    ((_ic)->ic_aow.i2sOpen = val)
#define IS_I2S_OPEN(_ic)                (((_ic)->ic_aow.i2sOpen)?1:0)
#define IS_AOW_AUDIO_CAPTURE_DONE(_ic)  ((_ic)->ic_aow.ctrl.capture_done)
#define IS_AOW_FORCE_INPUT(_ic)         ((_ic)->ic_aow.ctrl.force_input)
#define IS_AOW_CAPTURE_DATA(_ic)        ((_ic)->ic_aow.ctrl.capture_data)
#define AOW_INTERLEAVE_ENAB(_ic)        (((_ic)->ic_aow.interleave)?1:0)

#define AOW_LOCK_INIT(_ic)      spin_lock_init(&(_ic)->ic_aow.lock)
#define AOW_LOCK(_ic)           spin_lock(&(_ic)->ic_aow.lock)    
#define AOW_UNLOCK(_ic)         spin_unlock(&(_ic)->ic_aow.lock)
#define AOW_LOCK_DESTROY(_ic)   spin_lock_destroy(&(_ic)->ic_aow.lock)
#define AOW_IRQ_LOCK(_ic)       spin_lock_irqsave(&(_ic)->ic_aow.lock, (_ic)->ic_aow.lock_flags)
#define AOW_IRQ_UNLOCK(_ic)     spin_unlock_irqrestore(&(_ic)->ic_aow.lock, (_ic)->ic_aow.lock_flags)

#define AOW_LH_LOCK_INIT(_ic)             spin_lock_init(\
                                            &(_ic)->ic_aow.aow_lh_lock)
#define AOW_LH_LOCK(_ic)                  spin_lock(\
                                            &(_ic)->ic_aow.aow_lh_lock)
#define AOW_LH_UNLOCK(_ic)                spin_unlock(\
                                            &(_ic)->ic_aow.aow_lh_lock)
#define AOW_LH_LOCK_DESTROY(_ic)          spin_lock_destroy(\
                                            &(_ic)->ic_aow.aow_lh_lock)

#define AOW_ESSC_LOCK_INIT(_ic)           spin_lock_init(\
                                                &(_ic)->ic_aow.aow_essc_lock)
#define AOW_ESSC_LOCK(_ic)                spin_lock(\
                                                &(_ic)->ic_aow.aow_essc_lock)
#define AOW_ESSC_UNLOCK(_ic)              spin_unlock(\
                                                &(_ic)->ic_aow.aow_essc_lock)
#define AOW_ESSC_LOCK_DESTROY(_ic)        spin_lock_destroy(\
                                                &(_ic)->ic_aow.aow_essc_lock)

#define AOW_L2PE_LOCK_INIT(_ic)           spin_lock_init(\
                                                &(_ic)->ic_aow.aow_l2pe_lock)
#define AOW_L2PE_LOCK(_ic)                spin_lock(\
                                                &(_ic)->ic_aow.aow_l2pe_lock)
#define AOW_L2PE_UNLOCK(_ic)              spin_unlock(\
                                                &(_ic)->ic_aow.aow_l2pe_lock)
#define AOW_L2PE_LOCK_DESTROY(_ic)        spin_lock_destroy(\
                                                &(_ic)->ic_aow.aow_l2pe_lock)

#define AOW_PLR_LOCK_INIT(_ic)            spin_lock_init(\
                                                 &(_ic)->ic_aow.aow_plr_lock)
#define AOW_PLR_LOCK(_ic)                 spin_lock(\
                                                 &(_ic)->ic_aow.aow_plr_lock)
#define AOW_PLR_UNLOCK(_ic)               spin_unlock(\
                                                 &(_ic)->ic_aow.aow_plr_lock)
#define AOW_PLR_LOCK_DESTROY(_ic)         spin_lock_destroy(\
                                                &(_ic)->ic_aow.aow_plr_lock)

#define AOW_MCSMAP_LOCK_INIT(_ic)         spin_lock_init(\
                                                &(_ic)->ic_aow.aow_mcsmap_lock)
#define AOW_MCSMAP_LOCK(_ic)              spin_lock(\
                                                &(_ic)->ic_aow.aow_mcsmap_lock)
#define AOW_MCSMAP_UNLOCK(_ic)            spin_unlock(\
                                                &(_ic)->ic_aow.aow_mcsmap_lock)
#define AOW_MCSMAP_LOCK_DESTROY(_ic)      spin_lock_destroy(\
                                                &(_ic)->ic_aow.aow_mcsmap_lock)

#if ATH_SUPPORT_AOW_TXSCHED
#define AOW_WBUFSTAGING_LOCK_INIT(_ic)    spin_lock_init(\
                                                 &(_ic)->ic_aow.aow_wbuf_staging_lock)
#define AOW_WBUFSTAGING_LOCK(_ic)         spin_lock(\
                                                 &(_ic)->ic_aow.aow_wbuf_staging_lock)
#define AOW_WBUFSTAGING_UNLOCK(_ic)       spin_unlock(\
                                                 &(_ic)->ic_aow.aow_wbuf_staging_lock)
#define AOW_WBUFSTAGING_LOCK_DESTROY(_ic) spin_lock_destroy(\
                                                 &(_ic)->ic_aow.aow_wbuf_staging_lock)
#endif

#define AOW_ER_SYNC_LOCK_INIT(_ic)      spin_lock_init(&(_ic)->ic_aow.er.lock)
#define AOW_ER_SYNC_LOCK_DESTORY(_ic)   spin_lock_destroy(&(_ic)->ic_aow.er.lock)
#define AOW_ER_SYNC_IRQ_LOCK(_ic)       spin_lock_irqsave(&(_ic)->ic_aow.er.lock, (_ic)->ic_aow.er.lock_flags)
#define AOW_ER_SYNC_IRQ_UNLOCK(_ic)     spin_unlock_irqrestore(&(_ic)->ic_aow.er.lock, (_ic)->ic_aow.er.lock_flags)
#define AOW_ER_SYNC_LOCK(_ic)           spin_lock(&(_ic)->ic_aow.er.lock)
#define AOW_ER_SYNC_UNLOCK(_ic)         spin_unlock(&(_ic)->ic_aow.er.lock)


#define AOW_EC_PKT_INDEX_0  0
#define AOW_EC_PKT_INDEX_1  1
#define AOW_EC_PKT_INDEX_2  2
#define AOW_EC_PKT_INDEX_3  3
#define AOW_EC_PKT_INDEX_MASK   0x3
#define AOW_EC_SUB_FRAME_MASK   0xf

#define AOW_INDEX_0 0
#define AOW_INDEX_1 1
#define AOW_INDEX_2 2
#define AOW_INDEX_3 3


extern u_int32_t chksum_crc32 (unsigned char *block, u_int32_t length);
extern u_int32_t cumultv_chksum_crc32(u_int32_t crc_prev,
                                      unsigned char *block,
                                      u_int32_t length);
extern void chksum_crc32gentab (void);
extern int aow_er_handler(struct ieee80211vap *vap, wbuf_t wbuf, struct ieee80211_rx_status *rs);
extern int aow_ec_handler(struct ieee80211com *ic, audio_info *info);

/* VAP related APIs */
extern int ieee80211_aow_vattach(struct ieee80211vap* vap);
extern int ieee80211_aow_vdetach(struct ieee80211vap* vap);
extern int ieee80211_update_aow_ie(struct ieee80211com* ic, u_int8_t* ie, int len);
extern int ieee80211_delete_aow_ie(struct ieee80211com* ic);
extern int ieee80211_aow_ie_attach(struct ieee80211vap* vap);
extern int ieee80211_aow_ie_detach(struct ieee80211vap* vap);
extern int is_aow_assoc_only_set(struct ieee80211com* ic, struct ieee80211_node* ni);
extern int update_aow_capability_for_node(struct ieee80211_node* ni);
extern int is_aow_id_match(struct ieee80211com* ic, aow_ie_t* ie);

/* Extended stats related */
#define AOW_ES_ENAB(ic)         (ic->ic_get_aow_es(ic))
#define AOW_ESS_ENAB(ic)        (ic->ic_get_aow_ess(ic))
#define AOW_ESS_SYNC_SET(param) (((param) >> ATH_AOW_PARAMS_LOGSYNC_FLG_S)\
                                  & ATH_AOW_PARAMS_LOGSYNC_FLG_MASK)
#define AOW_IS_RX_CHANNEL_ENABLED(ic)       ((ic->ic_get_aow_audio_ch(ic) & AOW_RX_CHANNEL_SELECT_MASK)?1:0)
#define AOW_GET_AUDIO_CHANNEL_TO_PLAY(ic)   ((ic->ic_get_aow_audio_ch(ic) & AOW_RX_CHANNEL_VALUE_MASK))

/* Size of each latency range in microseconds */
#define ATH_AOW_LATENCY_RANGE_SIZE  (200)

/* Maximum value of latency in milliseconds which will be covered
   by the Latency Histogram */
#define ATH_AOW_LH_MAX_LATENCY      (300U)

/* Maximum value of latency in microseconds which will be covered
   by the Latency Histogram.
   Make sure this can never exceed max value representable by 
   unsigned int */
#define ATH_AOW_LH_MAX_LATENCY_US   (ATH_AOW_LH_MAX_LATENCY * 1000U)

/* Max number of ES/ESS Statistics groups to be maintained.
 * These groups are inserted into a fixed size FIFO */
#define ATH_AOW_MAX_ES_GROUPS       (8)

/* Time interval for which each ES/ESS Statistics group is
   measured (in seconds). */
#define ATH_AOW_ES_GROUP_INTERVAL   (10)

/* Time interval for which each ES/ESS Statistics group is
   measured (in milliseconds). */
#define ATH_AOW_ES_GROUP_INTERVAL_MS  (ATH_AOW_ES_GROUP_INTERVAL *\
                                        NUM_MILLISEC_PER_SEC)

/* Number of digits after the decimal point, required for the ratios
   reported for extended stats. */
#define ATH_AOW_ES_PRECISION        (4)  
#define ATH_AOW_ES_MAX_PRECISION    (6)  

/* 10 to the power ATH_AOW_ES_PRECISION. Used for determining fractional
   component of ratio */
#define ATH_AOW_ES_PRECISION_MULT       (10000)  
#define ATH_AOW_ES_MAX_PRECISION_MULT   (1000000)  

#define ATH_AOW_PLR_TYPE_NUM_MPDUS  (1)
#define ATH_AOW_PLR_TYPE_NOK_FRMCNT (2)
#define ATH_AOW_PLR_TYPE_LATE_MPDU  (3)


extern int is_aow_audio_stopped(struct ieee80211com* ic);
extern int aow_es_base_init(struct ieee80211com *ic);
extern void aow_es_base_deinit(struct ieee80211com *ic);

extern void aow_clear_ext_stats(struct ieee80211com *ic);
extern void aow_print_ext_stats(struct ieee80211com * ic);

extern int aow_lh_init(struct ieee80211com *ic);
extern int aow_lh_reset(struct ieee80211com *ic);
extern int aow_lh_add(struct ieee80211com *ic, const u_int64_t latency);
extern int aow_lh_add_expired(struct ieee80211com *ic);
extern int aow_lh_print(struct ieee80211com *ic);
extern void aow_lh_deinit(struct ieee80211com *ic);

extern void aow_update_mcsinfo(struct ieee80211com *ic,
                               struct ath_buf *bf,
                               struct ether_addr *recvr_addr,
                               struct ath_tx_status *ts,
                               bool is_success,
                               int *num_attempts);
extern void aow_update_txstatus(struct ieee80211com *ic,
                                struct ath_buf *bf,
                                struct ath_tx_status *ts);

extern void aow_get_node_type(struct ieee80211com *ic, u_int16_t *node_type);

/* Netlink related declarations */
extern int aow_am_init(struct ieee80211com *ic);
extern void aow_am_deinit(struct ieee80211com *ic);
extern int aow_am_send(struct ieee80211com *ic, char *data, int size);

extern int aow_nl_send_rxpl(struct ieee80211com *ic,
                            u_int32_t seqno,
                            u_int8_t subfrme_seqno,
                            u_int16_t wlan_seqno,
#if ATH_SUPPORT_AOW_ES_ADVNCD_RXDEBUG
                            u_int64_t timestamp,
                            u_int64_t rxTime,
                            u_int64_t deliveryTime,
#endif
                            u_int8_t rxstatus);

extern int aow_nl_send_txpl(struct ieee80211com *ic,
                            struct ether_addr recvr_addr,
                            u_int32_t seqno,
                            u_int8_t subfrme_seqno,
                            u_int16_t wlan_seqno,
                            u_int8_t txstatus,
#if ATH_SUPPORT_AOW_ES_ADVNCD_TXDEBUG
                            struct aow_advncd_txinfo *atxinfo,
#endif
                            u_int8_t numretries);

extern void aow_essc_init(struct ieee80211com *ic);
extern void aow_essc_setall(struct ieee80211com *ic, u_int32_t val);
extern bool aow_essc_testdecr(struct ieee80211com *ic, int channel);
extern void aow_essc_deinit(struct ieee80211com *ic);

extern int aow_l2pe_init(struct ieee80211com *ic);
extern void aow_l2pe_reset(struct ieee80211com *ic);
extern void aow_l2pe_deinit(struct ieee80211com *ic);
extern int aow_l2pe_record(struct ieee80211com *ic, bool is_success);
extern void aow_l2pe_audstopped(struct ieee80211com *ic);
extern int aow_l2pe_print(struct ieee80211com *ic);

extern int aow_plr_init(struct ieee80211com *ic);
extern void aow_plr_reset(struct ieee80211com *ic);
extern void aow_plr_deinit(struct ieee80211com *ic);
extern int aow_plr_record(struct ieee80211com *ic,
                          u_int32_t seqno,
                          u_int16_t subseqno,
                          u_int8_t type);
extern void aow_plr_audstopped(struct ieee80211com *ic);
extern int aow_plr_print(struct ieee80211com *ic);
extern void aow_mcss_print(struct ieee80211com *ic);


/* few of the external functions, that the AoW module needs */

extern void ieee80211_send_setup( struct ieee80211vap *vap, 
                           struct ieee80211_node *ni,
                           struct ieee80211_frame *wh, 
                           u_int8_t type, 
                           const u_int8_t *sa, 
                           const u_int8_t *da,
                           const u_int8_t *bssid);

//extern struct sk_buff* ieee80211_get_aow_frame(u_int8_t **frm, u_int32_t pktlen);
extern int ath_tx_send(wbuf_t wbuf);

extern int wlan_put_pkt(struct ieee80211com *ic, char* data, int len);
extern int wlan_get_pkt(struct ieee80211com *ic, int* index);
extern int wlan_init_sync_buf(struct ieee80211com *ic);
extern int ieee80211_print_tx_info_all_nodes(struct ieee80211com* ic);

#else   /* ATH_SUPPORT_AOW */

#define IEEE80211_ENAB_AOW(ic)  (0)
#define AOW_ES_ENAB(ic)         (0)
#define AOW_ESS_ENAB(ic)        (0)
#define AOW_ESS_SYNC_SET(param) (0)
#define AOW_ER_ENAB(ic)         (0)


#define ieee80211_aow_attach(a)             do{}while(0)
#define ieee80211_audio_receive(a,b,c)      do{}while(0)
#define ieee80211_aow_detach(a)             do{}while(0)
#define IEEE80211_AOW_DPRINTF(...)

#define  wlan_put_pkt(a, b, c)  do{}while(0)
#define  wlan_get_pkt(a, b)     do{}while(0)
#define  wlan_init_sync_buf(a)  do{}while(0)

#define ieee80211_aow_vattach(a)    do{}while(0)
#define ieee80211_aow_vdetach(a)    do{}while(0)
#define ieee80211_aow_ie_attach(a)  do{}while(0)
#define ieee80211_aow_ie_detach(a)  do{}while(0)
#define is_aow_assoc_only_set(a, b)  (1)
#define update_aow_capability_for_node(a)   do{}while(0)
#define ieee80211_aow_disconnect_device(a, b)   do{}while(0)

#endif  /* ATH_SUPPORT_AOW */
#endif  /* IEEE80211_AOW_H */
