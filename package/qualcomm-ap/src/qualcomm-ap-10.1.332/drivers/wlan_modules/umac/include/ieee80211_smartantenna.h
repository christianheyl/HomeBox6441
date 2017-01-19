#ifndef _IEEE80211_SMARTANTENNA_H_
#define _IEEE80211_SMARTANTENNA_H_

#ifndef UMAC_SUPPORT_SMARTANTENNA
#define UMAC_SUPPORT_SMARTANTENNA 0
#endif

#define WQNAME(name) #name

#define SA_TRAINING_MODE_VALID_BITS 0x1
#define SA_TRAINING_TYPE_VALID_BITS 0x3
#define SA_MAX_PERCENTAGE 100

#define MAX_RATES_PERSET (4+8+24) /* max rates 4 11b rates,8 11g/a rates,24 11n rates for 3 stream */
#define MAX_HT_RATES 24 
#define NUM_TRAIN_PKTS 32         /* number of training packets required to be self generated in an interval */
#define SA_NUM_LEGACY_TRAIN_PKTS 200 /* Number of packets required for legacy rates */
#define IS_HT_RATE(_rate)     ((_rate) & 0x80)

#define SMARTANT_TRAIN_TID 1 /* Use BK acces category for Training and use TID 1 */
#define RETRAIN_INTERVEL 1000  /* 1 sec */
#define DEFAULT_AP_TRAFFIC_GEN_TIMER 500  /* 500 ms */
#define DEFAULT_STA_TRAFFIC_GEN_TIMER 100  /* 100 ms */
#define TRAFFIC_GEN_INTERVEL 500  /* 30 sec */
#define RETRAIN_MISS_THRESHOLD 20  /* Max number of intervals for periodic training */
#define MAX_PRETRAIN_PACKETS 600 /* MAX Number of packets to be generated */ 
#define SA_PERDIFF_THRESHOLD 3 /* PER diff threshold to apply secondary metric */
#define SA_PER_UPPER_BOUND 80  /* Upper bound to move to next lower rate */
#define SA_PER_LOWER_BOUND 20  /* Lower bound to move to next higher rate */
#define SA_DEFALUT_HYSTERISYS 3 /* 3 intervals */
#define SA_MAX_PRD_RETRAIN_MISS 10 /* Upper bound for periodic retrain miss in case of station */
#define MIN_GOODPUT_THRESHOLD 6 /* Minimum good put drop to tigger retraining 
                                 * For most of rates which are below 50 Mbps, the difference between consecutive rates is 6.5 Mbps.
                                 */
#define SA_DEFAULT_PKT_LEN 1536 /* default packet length */
#define SA_DEFAULT_TP_THRESHOLD 10 /* default throughput threshold in percentage */
#define SA_DEFAULT_RETRAIN_INTERVAL 2 /* default retrain interval,2 - seconds*/
#define SA_DEFAULT_MIN_TP  344      /* default minimum throughput to consider as traffic (86 -- 2 mbps)*/
#define SA_GPUT_INGNORE_INTRVAL 2  /* default gudput ingnoring interval */ 
#define SA_GPUT_AVERAGE_INTRVAL 2  /* default gudput ingnoring interval */ 
#define SA_CTS_PROT 0x10000   /* indication to use self cts */

/* State Variable */
#define SMARTANTENNA_PRETRAIN           0 /* state where pretraining will happen */
#define SMARTANTENNA_TRAIN_INPROGRESS   1 /* state where training is in progress currently */
#define SMARTANTENNA_TRAIN_HOLD         2 /* state where stats are processed in training */
#define SMARTANTENNA_DEFAULT            3 /* default state where nothing will happen */

#define RATECODE_TO_MCS_MASK 0x7f
#define MCS_TO_RATECODE_MASK 0x80

#define MASK_BYTE0 0x000000FF
#define MASK_BYTE1 0x0000FF00
#define MASK_BYTE2 0x00FF0000
#define MASK_BYTE3 0xFF000000

/* protocol numbers */

#define SA_TRAFFICGEN_LOCK_INIT(_ic)      spin_lock_init(&(_ic)->sc_sa_trafficgen_lock)
#define SA_TRAFFICGEN_LOCK_DESTORY(_ic)   spin_lock_destroy(&(_ic)->sc_sa_trafficgen_lock)
#define SA_TRAFFICGEN_IRQ_LOCK(_ic)       spin_lock_irqsave(&(_ic)->sc_sa_trafficgen_lock, (_ic)->sc_sa_trafficgen_lock_flags)
#define SA_TRAFFICGEN_IRQ_UNLOCK(_ic)     spin_unlock_irqrestore(&(_ic)->sc_sa_trafficgen_lock, (_ic)->sc_sa_trafficgen_lock_flags)
#define SA_TRAFFICGEN_LOCK(_ic)           spin_lock(&(_ic)->sc_sa_trafficgen_lock)
#define SA_TRAFFICGEN_UNLOCK(_ic)         spin_unlock(&(_ic)->sc_sa_trafficgen_lock)

enum {
    SA_TRAINING_MODE_EXISTING=0,
    SA_TRAINING_MODE_MIXED=1
};
struct ieee80211_smartantenna_params {
    u_int8_t training_mode;      /* use existing traffic alone - 0
                                    use mixed (existing/proprietary) traffic - 1*/
    u_int8_t training_type;      /* Frame based or Protocol based */
    u_int8_t lower_bound;        /* lower bound of rate drop in training  */
    u_int8_t upper_bound;        /*  upper bound of rate increase in training */
    u_int8_t per_diff_threshold; /* per diff that treated as equal */
    u_int16_t n_packets_to_train;    /* number of packets used for training */
    u_int16_t packetlen;         /* packet lenght of propritory generated training packet */
    u_int8_t training_start;     /* value other than one indicates fixed rate training */
    u_int16_t traffic_gen_timer; /* traffic generation timer value */
    u_int32_t retraining_enable; /* contains performence retrain check timer value */
    u_int8_t num_tx_antennas;    /* possible tx antenna combinations */
    u_int16_t num_pkt_threshold; /* throshold for minimum traffic to consider */
    u_int16_t retrain_interval;  /* periodic timer value */
    u_int8_t max_throughput_change;/* threshold in throghput fluctuation for performence based trigger */
    u_int8_t hysteresis;         /* hsteresis value for throughput fluctuations */
    u_int8_t min_goodput_threshold; /* Minimum good put drop to tigger retraining */
    int8_t goodput_avg_interval;    /* goodput averaging interval */
#define SA_CONFIG_INTENSETRAIN 0x1     /* setting this bit in config indicates training with double number of packets */
#define SA_CONFIG_EXTRATRAIN   0x2     /* setting this bit in config indicates to do extra traing in case of conflits in first metric */
#define SA_CONFIG_SLECTSPROTEXTRA 0x4  /* setting this bit in config indicates to protect extra training frames with self CTS */
#define SA_CONFIG_SLECTSPROTALL   0x8  /* setting this bit in config indicates to protect all training frames with self CTS */
    u_int8_t config;             /* contains configuration for SA algorithm */
};

#define SA_MAX_INTENSETRAIN_PKTS 1000 /* maximum packets that can be used in case of intense training */
#if UMAC_SUPPORT_SMARTANTENNA
int  ieee80211_smartantenna_set_param(struct ieee80211com *ic, int param, u_int32_t val);
u_int32_t ieee80211_smartantenna_get_param(struct ieee80211com *ic, int param);
void smartantenna_traffic_gen_sm(void *data);
int32_t sa_move_rate(struct ieee80211_node *ni,int8_t train_nxt_rate);
int32_t sa_get_max_rateidx(struct ieee80211_node *ni);
void sa_set_traffic_gen_timer(struct ieee80211com *ic,struct ieee80211_node *ni);
u_int8_t get_rateindex(struct ieee80211_node *ni,u_int8_t rateCode);
inline int SA_MAXIMUM(int a, int b);

int  ieee80211_smartantenna_attach(struct ieee80211com *ic);
int  ieee80211_smartantenna_detach(struct ieee80211com *ic);
void ieee80211_smartantenna_training_init(struct ieee80211vap *vap);
int ieee80211_smartantenna_start_training(struct ieee80211_node *ni, struct ieee80211com *ic);
int ieee80211_smartantenna_train_node(struct ieee80211_node *ni, u_int8_t antenna ,u_int8_t rateidx);
void smartantenna_sm(struct ieee80211_node *ni);
void ieee80211_set_current_antenna(struct ieee80211vap *vap, u_int32_t antenna);
void smartantenna_prepare_rateset (struct ieee80211_node *ni);
void smartantenna_state_init(struct ieee80211_node *ni, u_int8_t rateCode);
void smartantenna_state_save(struct ieee80211_node *ni , u_int8_t antenna , u_int32_t itr, u_int32_t pending);
void smartantenna_state_restore(struct ieee80211_node *ni);
void smartantenna_setdefault_antenna(struct ieee80211_node *ni, u_int8_t antenna);
void ieee80211_smartantenna_training_init(struct ieee80211vap *vap);
void ieee80211_smartantenna_input(struct ieee80211vap *vap, wbuf_t wbuf, struct ether_header *eh, struct ieee80211_rx_status *rs);
void ieee80211_smartantenna_rx_proto_msg(struct ieee80211vap *vap, wbuf_t mywbuf, struct ieee80211_rx_status *rs);
void smartantenna_automation_sm(struct ieee80211_node *ni);
int ieee80211_smartantenna_tx_proto_msg(struct ieee80211vap *vap ,struct ieee80211_node *ni , u_int8_t msg_num, u_int8_t txantenna, u_int8_t rxantenna, u_int8_t ratecode);
void smartantenna_display_goodput(struct ieee80211vap *vap);
int8_t smartantenna_check_optimize(struct ieee80211_node *ni, u_int8_t rateCode);
int smartantenna_retrain_check(struct ieee80211_node *ni, int mode, u_int32_t* npackets);
int ieee80211_smartantenna_init_training(struct ieee80211_node *ni, struct ieee80211com *ic);
int ieee80211_smartantenna_update_trainstats(struct ieee80211_node *ni, void *per_stats);
void ieee80211_smartantenna_node_init(struct ieee80211_node *ni);
void ieee80211_smartantenna_select_rx_antenna(struct ieee80211_node *ni_cnode);
#else
static inline int  ieee80211_smartantenna_attach(struct ieee80211com *ic)
{
    return 0;
}
static inline int  ieee80211_smartantenna_detach(struct ieee80211com *ic)
{
    return 0;
}

#endif
#endif
