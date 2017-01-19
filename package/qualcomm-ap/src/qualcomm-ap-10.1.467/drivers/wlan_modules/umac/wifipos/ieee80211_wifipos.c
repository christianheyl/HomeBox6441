/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc..
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <net/genetlink.h>
#include <linux/netlink.h>
#include <ieee80211_var.h>
#include <ath_dev.h>
#include <ath_internal.h>
#include <ieee80211_wpc.h>
#include <if_athvar.h>
#include <ieee80211_vap_tsf_offset.h>

// Note:TO enable dynamic debug use mask as 0x20 Eg:iwpriv ath0 dbgLVL_high 0x20

#define WIFIPOS_DPRINTK(format, args...) printk(KERN_DEBUG "%s:%d " format "\n",__FUNCTION__, __LINE__, ## args)

#define IEEE80211_HDR_SERVICE_BITS_LEN 16
#define IEEE80211_HDR_TAIL_BITS_LEN 6
#define SYMBOL_DURATION_MICRO_SEC 4
#define ACK_PKT_BYTE_SIZE 14

#define TRUE 1
#define FALSE 0
#define TX_DESC 1
#define RX_DESC 0
#define ATH_WIFIPOS_SINGLE_BAND 0
#define ATH_WIFIPOS_POSITIONING 1
#define NSP_HW_VERSION 0x01

#define NO_OF_CLK_CYCLES_PER_MICROSEC 44
#define L_LTF 8
#define L_STF 8
#define L_SIG 4
#define FC0_ACK_PKT 0xd4
#define ONE_SIFS_DURATION 704  // 16*44
#define TWO_SIFS_DURATION 1408 // (16*44)*2

#if ATH_SUPPORT_WIFIPOS
#define MAX_CTS_TO_SELF_TIME 31 // This is updated to 31 fo taking into account the channel change delay.
#define MAX_PROBE_TIME 3
#define MAX_PKT_TYPES 3
#define MAX_TIMEOUT 120000
#define IEEE80211_FRAME_LEN     (sizeof(struct ieee80211_frame) + IEEE80211_CRC_LEN)
#define IEEE80211_QOSFRAME_LEN  (sizeof(struct ieee80211_frame) + IEEE80211_CRC_LEN )


#define IEEE80211_NODE_SAVEQ_MGMTQ(_ni)     (&_ni->ni_mgmtq)
#define IEEE80211_NODE_SAVEQ_FULL(_nsq)     ((_nsq)->nsq_len >= (_nsq)->nsq_max_len)

static int packetlength[MAX_PKT_TYPES] = {  IEEE80211_FRAME_LEN, IEEE80211_QOSFRAME_LEN, 0 };
static int databits_per_symbol[] = { 192, 96, 48,  24, 216, 144, 72, 36 };
static int databits_per_symbol_11n[] = { 26, 52, 78, 104,156, 208, 234, 260, 52, 104, 156, 208, 312, 416,   };
static int databits_per_symbol_11b[] = { 0, 0, 0, 0};
static int proc_index = 0;
static int wifipos_tx_index = 0;
static int wifipos_rx_index = 0;
static int wakeup_sta_count = 0;

#define MIN_TSF_TIMER_TIME  5000 /* 5 msec */
#define     TIME_UNIT_TO_MICROSEC   1024    /* 1 TU equals 1024 microsecs */

/* Get the beacon interval in microseconds */
static INLINE
u_int32_t get_beacon_interval(wlan_if_t vap) 
{
        /* The beacon interval is in terms of Time Units */
            return(vap->iv_bss->ni_intval * TIME_UNIT_TO_MICROSEC);
}  

/*
* get the nextbtt for the given vap.
* for the second STA vap the tsf offset is used for calculating tbtt.
*/
static u_int64_t ieee80211_get_next_tbtt_tsf_64(wlan_if_t vap)
{
   struct ieee80211com *ic = vap->iv_ic;
   u_int64_t           curr_tsf64, nexttbtt_tsf64;
   u_int32_t           bintval; /* beacon interval in micro seconds */
   ieee80211_vap_tsf_offset tsf_offset_info;

   curr_tsf64 = ic->ic_get_TSF64(ic);
   /* calculate the next tbtt */

   /* tsf offset from our HW tsf */
   ieee80211_vap_get_tsf_offset(vap,&tsf_offset_info);

   /* adjust tsf to the clock of the GO */
   if (tsf_offset_info.offset_negative) {
       curr_tsf64 -= tsf_offset_info.offset;
   } else {
       curr_tsf64 += tsf_offset_info.offset;
   }

   bintval = get_beacon_interval(vap);

   nexttbtt_tsf64 =  curr_tsf64 + bintval;

   nexttbtt_tsf64  = nexttbtt_tsf64 - OS_MOD64_TBTT_OFFSET(nexttbtt_tsf64, bintval);

   if ((nexttbtt_tsf64 - curr_tsf64) < MIN_TSF_TIMER_TIME ) {  /* if the immediate next tbtt is too close go to next one */
       nexttbtt_tsf64 += bintval;
   }

   /* adjust tsf back to the clock of our HW */
   if (tsf_offset_info.offset_negative) {
       nexttbtt_tsf64 += tsf_offset_info.offset;
   } else {
       nexttbtt_tsf64 -= tsf_offset_info.offset;
   }

   return nexttbtt_tsf64;
}

static unsigned int get_dbps_by_rate(int rate )
{
	int mode = (rate & 0x000000F0)>>4;
	int index = (rate & 0x0000000F);
	unsigned int dbps = 0;

	switch(mode)
	{
		//11 G
		case 0 : 
			if(index >= 0x08 && index <= 0x0f)
				dbps = databits_per_symbol[index - 0x08];
			else
				WIFIPOS_DPRINTK("WifiPositioning:Invalid get_dbps_by_rate() rate %d\n",rate);
			break;
		//11 B TODO: Identify proper values and fill in the structure
		case 1:
			if (index == 0x08 || index == 0x09 || index == 0x0a || index == 0x0b)
				dbps = databits_per_symbol_11b[index - 0x08];
			else
				WIFIPOS_DPRINTK("WifiPositioning:Invalid get_dbps_by_rate() rate %d\n",rate);
			break;
		//11 N
		case 8:
			if(index >= 0x0 && index <= 0x0f)
				dbps =  databits_per_symbol_11n[index];
			else
				WIFIPOS_DPRINTK("WifiPositioning:Invalid get_dbps_by_rate() rate %d\n",rate);
			break;

		default :
			WIFIPOS_DPRINTK("WifiPositioning:Invalid get_dbps_by_rate() mode %d rate %d\n",mode,rate );
			break;
	}
	return dbps;
}

spinlock_t wakeup_lock;
spinlock_t tsf_req_lock;

struct ieee80211_wifipos{
    void *wifipos_sock;
    u_int32_t process_pid;
    u_int32_t txcorrection;
    u_int32_t rxcorrection;
    struct ieee80211com *ic;
    ieee80211_wifipos_reqdata_t *reqdata;
    struct work_struct work;
    struct work_struct tsf_work;
    spinlock_t lock;
    os_timer_t wifipos_timer;
    os_timer_t wifipos_ps_timer;
    /* This spin locks are created and can be used to 
     * disable bh interrupts.
     */
    adf_os_spinlock_t wifipos_imasklock;
    unsigned long     wifipos_imasklockflags;
    os_timer_t wifipos_wakeup_timer;
    struct ieee80211_node *ni_tmp_sta;
    u_int32_t channel;
    u_int32_t sta_cnt;
}*ieee80211_wifiposdata_g;
#define WIFI_IRQ_LOCK(_ieee80211_wifiposdata_g)       spin_lock_irqsave(&(ieee80211_wifiposdata_g)->wifipos_imasklock, (ieee80211_wifiposdata_g)->wifipos_imasklockflags)
#define WIFI_IRQ_UNLOCK(_ieee80211_wifiposdata_g)       spin_unlock_irqrestore(&(ieee80211_wifiposdata_g)->wifipos_imasklock, (ieee80211_wifiposdata_g)->wifipos_imasklockflags)

/* the netlink family */
#define ATH_WIFIPOS_MAX_CONCURRENT_PROBES 500
#define ATH_WIFIPOS_MAX_CONCURRENT_WAKEUP_STA 20
#define ATH_WIFIPOS_MAX_CONCURRENT_TSF_AP 20
#define MAX_HDUMP_SIZE 390
struct ieee80211_wifipos_table_s {
    u_int32_t tod;
    u_int32_t toa;
    u_int32_t rate;
    u_int8_t valid;
    u_int8_t rssi[MAX_CHAINS];
    unsigned char no_of_chains;
    unsigned char no_of_retries;
    int8_t type1_payld[MAX_HDUMP_SIZE];
    u_int8_t sta_mac_addr[ETH_ALEN];
    u_int32_t request_id;
}ieee80211_wifipos_table_g[ATH_WIFIPOS_MAX_CONCURRENT_PROBES];

struct ieee80211_wifipos_wakeup_s {
    unsigned char sta_mac_addr[ETH_ALEN];
    u_int8_t wakeup_interval;
    u_int32_t timestamp;
}ieee80211_wifipos_wakeup_g[ATH_WIFIPOS_MAX_CONCURRENT_WAKEUP_STA];

struct ieee80211_wifipos_tsfreq_s {
    unsigned char assoc_ap_mac_addr[ETH_ALEN];
    u_int64_t   probe_ap_tsf;
    u_int64_t   assoc_ap_tsf;
    u_int32_t   diff_tsf;
    unsigned char valid;
}ieee80211_wifipos_tsfreq_g[ATH_WIFIPOS_MAX_CONCURRENT_TSF_AP];

void ieee80211_wifipos_nlsend_tsf_update(u_int8_t *source_mac_addr) 
{
    struct ieee80211vap *vap;
    int i;
    vap = TAILQ_FIRST(&ieee80211_wifiposdata_g->ic->ic_vaps);
    if (vap == NULL)
    {
        WIFIPOS_DPRINTK("vap is null\n");
        return;
    }

    spin_lock(&tsf_req_lock);
    for(i=0; i < ATH_WIFIPOS_MAX_CONCURRENT_TSF_AP; i++) {
        if(IEEE80211_ADDR_EQ(ieee80211_wifipos_tsfreq_g[i].assoc_ap_mac_addr, source_mac_addr)) {
            ieee80211_wifipos_tsfreq_g[i].probe_ap_tsf = vap->iv_local_tsf_tstamp;
            ieee80211_wifipos_tsfreq_g[i].assoc_ap_tsf = vap->iv_tsf_sync;
            ieee80211_wifipos_tsfreq_g[i].valid = 1;
            schedule_work(&ieee80211_wifiposdata_g->tsf_work);
        }
    }
    spin_unlock(&tsf_req_lock);
}

int ieee80211_wifipos_set_txcorrection(wlan_if_t vaphandle, unsigned int corr)
{
    struct ieee80211vap *vap = vaphandle;
	if(ieee80211_wifiposdata_g) {
		ieee80211_wifiposdata_g->txcorrection = corr;
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Setting txcorrection = %d\n",ieee80211_wifiposdata_g->txcorrection);
	}
	return 0;
}

int ieee80211_wifipos_set_rxcorrection(wlan_if_t vaphandle, unsigned int corr)
{
    struct ieee80211vap *vap = vaphandle;
	if(ieee80211_wifiposdata_g) {
		ieee80211_wifiposdata_g->rxcorrection = corr;
		IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Setting rxcorrection = %d\n",ieee80211_wifiposdata_g->rxcorrection);
	}
	return 0;
}

unsigned int ieee80211_wifipos_get_rxcorrection(void)
{
	if(ieee80211_wifiposdata_g)
		return ieee80211_wifiposdata_g->rxcorrection;
	return 0;
}

unsigned int ieee80211_wifipos_get_txcorrection(void)
{
    if(ieee80211_wifiposdata_g)
        return ieee80211_wifiposdata_g->txcorrection;
    return 0;
}

void ieee80211_print_tsfrqst(struct ieee80211vap *vap, struct nsp_tsf_req *tsfrqst)
{
    unsigned char *s;
    s = (char *)tsfrqst->ap_mac_addr;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:--------------TSF_REQUEST-----------------\n");
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Request ID            :%x\n",tsfrqst->request_id);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:AP MAC addr          :%02x:%02x:%02x:%02x:%02x:%02x\n", s[0],s[1],s[2],s[3],s[4],s[5]);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Channel               :%x\n",tsfrqst->channel);
}

void ieee80211_print_tsfresp(struct ieee80211vap *vap, struct nsp_tsf_resp *tsfresp)
{
    unsigned char *s;
    s = (unsigned char *)tsfresp->ap_mac_addr;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:---------------TSF_RESPONSE-----------------\n");
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Request ID            :%x\n",tsfresp->request_id);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:MAC addr              :%x:%x:%x:%x:%x:%x\n", s[0],s[1],s[2],s[3],s[4],s[5]);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Associated AP's tsf   :%llx\n",tsfresp->assoc_tsf);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Probing AP's tsf      :%llx\n",tsfresp->prob_tsf);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Result                :%x\n",tsfresp->result);
}

void ieee80211_print_mrqst(struct ieee80211vap *vap, struct nsp_mrqst *mrqst)
{
    unsigned char *s = (char *)mrqst->sta_mac_addr;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:----------------TYPE1_MEASUREMENT_REQUEST-----------------\n");
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Request ID            :%x\n",mrqst->request_id);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Mode                  :%x\n",mrqst->mode);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:STA MAC addr          :%02x:%02x:%02x:%02x:%02x:%02x\n", s[0],s[1],s[2],s[3],s[4],s[5]);
    s = (unsigned char *)mrqst->spoof_mac_addr;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Spoof MAC addr        :%02x:%02x:%02x:%02x:%02x:%02x\n", s[0],s[1],s[2],s[3],s[4],s[5]);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:STA Info              :%x\n",mrqst->sta_info);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Channel               :%x\n",mrqst->channel);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Number of measurements:%x\n",mrqst->no_of_measurements);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Transmit Rate         :%x\n",mrqst->transmit_rate);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Timeout               :%x\n",mrqst->timeout);
}

void ieee80211_print_type1_payld_resp(struct ieee80211vap *vap, struct nsp_type1_resp *type1_payld_resp)
{
#ifdef IEEE80211_DEBUG
    unsigned int i, tmp;
#endif
    unsigned int type1_payld_size, chain_cnt;
    type1_payld_size  = ATH_DESC_CDUMP_SIZE(type1_payld_resp->no_of_chains);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:TOA                   :%llx\n",type1_payld_resp->toa);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:TOD                   :%llx\n",type1_payld_resp->tod);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:RTT                   :%llx\n",type1_payld_resp->toa - type1_payld_resp->tod);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Send rate             :%x\n",type1_payld_resp->send_rate);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Receive rate          :%x\n",type1_payld_resp->receive_rate);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:No of chains          :%x\n",type1_payld_resp->no_of_chains);
    for (chain_cnt = 0; chain_cnt < type1_payld_resp->no_of_chains; chain_cnt++)
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:RSSI for CHAIN %d      :%x\n",chain_cnt + 1, type1_payld_resp->rssi[chain_cnt]);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:TYPE1PAYLOAD          :%d\n", type1_payld_size);
#ifdef IEEE80211_DEBUG
    for (i=0; i <= type1_payld_size/4; i++) {
        if (i%7 == 0 )
            WIFIPOS_DPRINTK("\n");
        tmp = *(((int *)type1_payld_resp->type1_payld) + i);
        /* convert the channel dump to little endian format */
        tmp = ((tmp << 24) | ((tmp << 8) & 0x00ff0000) | ((tmp >> 8) & 0x0000ff00) | (tmp >> 24));
        WIFIPOS_DPRINTK("%x ", tmp);
    }
#endif
}

void ieee80211_print_slresp(struct ieee80211vap *vap, struct nsp_sleep_resp *slresp)
{
    unsigned char *s;
    s = (char *)slresp->sta_mac_addr;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:---------------SLEEP_RESPONSE-----------------\n");
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Request ID            :%x\n",slresp->request_id);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:MAC addr              :%x:%x:%x:%x:%x:%x\n", s[0],s[1],s[2],s[3],s[4],s[5]);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Result                :%x\n",slresp->result);
}

void ieee80211_print_wresp(struct ieee80211vap *vap, struct nsp_wakeup_resp *wresp)
{
    unsigned char *s;
    s = (char *)wresp->sta_mac_addr;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:---------------WAKEUP_RESPONSE-----------------\n");
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Request ID            :%x\n",wresp->request_id);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:MAC addr              :%x:%x:%x:%x:%x:%x\n", s[0],s[1],s[2],s[3],s[4],s[5]);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Result                :%x\n",wresp->result);
}
void ieee80211_print_wrqst(struct ieee80211vap *vap, struct nsp_wakeup_req *wreq)
{
    unsigned char *s;
    s = (char *)wreq->sta_mac_addr;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:---------------WAKEUP_REQUEST-----------------\n");
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Request ID            :%x\n",wreq->request_id);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:MAC addr              :%x:%x:%x:%x:%x:%x\n", s[0],s[1],s[2],s[3],s[4],s[5]);
}
void ieee80211_print_slrqst(struct ieee80211vap *vap, struct nsp_sleep_req *sreq)
{
    unsigned char *s;
    s = (char *)sreq->sta_mac_addr;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:---------------SLEEP_REQUEST-----------------\n");
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Request ID            :%x\n",sreq->request_id);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:MAC addr              :%x:%x:%x:%x:%x:%x\n", s[0],s[1],s[2],s[3],s[4],s[5]);
}
void ieee80211_print_srqst(struct ieee80211vap *vap, struct nsp_sreq *sreq)
{
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:---------------STATUS_REQUEST-----------------\n");
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Request ID            :%x\n",sreq->request_id);
}
void ieee80211_print_crqst(struct ieee80211vap *vap, struct nsp_cap_req *creq)
{
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:---------------CAPABILITY_REQUEST-----------------\n");
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Request ID            :%x\n",creq->request_id);
}
void ieee80211_print_cresp(struct ieee80211vap *vap, struct nsp_cap_resp *cresp)
{
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:---------------CAPABILITY_RESPONSE-----------------\n");
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Request ID            :%x\n",cresp->request_id);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Band                  :%x\n",cresp->band);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Positioning           :%x\n",cresp->positioning);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:SW Version            :%x\n",cresp->sw_version);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:HW Version            :%x\n",cresp->hw_version);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Clock Frequency in HZ :%d\n",cresp->clk_freq);
}
void ieee80211_print_sresp(struct ieee80211vap *vap, struct nsp_sresp *sresp)
{   
    unsigned char *s;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:---------------STATUS_RESPONSE-----------------\n");
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Request ID            :%x\n",sresp->request_id);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:No of stations        :%x\n",sresp->no_of_stations);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Request timestamp     :%x\n",sresp->req_tstamp);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Response timestamp    :%x\n",sresp->resp_tstamp);
    s = (char *)sresp->ap1_mac;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:AP1 Mac address    :%x %x %x %x %x %x\n",s[0],s[1],s[2],s[3],s[4],s[5]);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:AP1 channel        :%x\n",sresp->ap1_channel);
    s = (char *)sresp->ap2_mac;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:AP2 Mac address    :%x %x %x %x %x %x\n",s[0],s[1],s[2],s[3],s[4],s[5]);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:AP2 channel        :%x\n",sresp->ap2_channel);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:SSID               :%s\n",sresp->ssid);
}

void ieee80211_print_sta_info(struct ieee80211vap *vap, struct nsp_station_info *sta_info)
{
    unsigned char *s;
    s = (char *)sta_info->sta_mac_addr;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:---------------STATUS_INFO-----------------\n");
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:STA Mac address    :%x %x %x %x %x %x\n",s[0],s[1],s[2],s[3],s[4],s[5]);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:STA channel        :%x\n",sta_info->channel);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:STA last timestamp :%llx\n",sta_info->tstamp);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Station info       :%x\n",sta_info->info);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Station RSSI       :%x\n",sta_info->rssi);

}

void ieee80211_print_mresp(struct ieee80211vap *vap, struct nsp_mresp *mresp)
{
    unsigned char *s;
    s = (char *)mresp->sta_mac_addr;
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:---------------TYPE1 MEASUREMENT_RESPONSE-----------------\n");
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Request ID            :%x\n",mresp->request_id);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:MAC addr              :%x:%x:%x:%x:%x:%x\n", s[0],s[1],s[2],s[3],s[4],s[5]);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:No of responses       :%x\n",mresp->no_of_responses);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Request timestamp     :%x\n",mresp->req_tstamp);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Response timestamp    :%x\n",mresp->resp_tstamp);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Result                :%x\n",mresp->result);
}

void ieee80211_print_nsphdr(struct ieee80211vap *vap, struct nsp_header *nsp_hdr)
{
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:---------------NSP_HEADER-----------------\n");
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Start of Frame Delimiter :%x\n",nsp_hdr->SFD);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Version                  :%x\n",nsp_hdr->version);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Frame Type               :%x\n",nsp_hdr->frame_type);
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Frame Length             :%x\n",nsp_hdr->frame_length);
}

/*
 * Convert MHz frequency to IEEE channel number.
 */
static u_int
ieee80211_mhz2channel(u_int freq)
{
#define IS_CHAN_IN_PUBLIC_SAFETY_BAND(_c) ((_c) > 4940 && (_c) < 4990)

	if (freq == 2484)
        return 14;
    if (freq < 2484)
        return (freq - 2407) / 5;
    if (freq < 5000) {
        if (IS_CHAN_IN_PUBLIC_SAFETY_BAND(freq)) {
            return ((freq * 10) +   
                (((freq % 5) == 2) ? 5 : 0) - 49400)/5;
        } else if (freq > 4900) {
            return (freq - 4000) / 5;
        } else {
            return 15 + ((freq - 2512) / 20);
        }
    }
    return (freq - 5000) / 5;
}

static void
sta_list(void *arg, struct ieee80211_node *ni)
{
    struct nsp_station_info sta_info;
    char *buf = arg;
    static int i;
    wlan_rssi_info rssi_info;
    
    if(i < ieee80211_wifiposdata_g->sta_cnt) {
        wlan_chan_t chan = wlan_node_get_chan(ni);
        u_int freq = wlan_channel_frequency(chan);
        sta_info.channel = ieee80211_mhz2channel(freq);
        IEEE80211_ADDR_COPY(sta_info.sta_mac_addr, ni->ni_macaddr);
        if (wlan_node_getrssi(ni, &rssi_info, WLAN_RSSI_RX) == 0) {
            sta_info.rssi = rssi_info.avg_rssi;
        }
        sta_info.info = 0x1; 
        sta_info.tstamp =  ni->ni_tstamp.tsf; 
        ieee80211_print_sta_info(ni->ni_vap, &sta_info);
        memcpy((buf + NSP_HDR_LEN + SRES_LEN + i*SINFO_LEN), &sta_info, SINFO_LEN);
        i++;
    }
    else
    {
        i =0;
    }
}

static void ieee80211_wifipos_nlsend_status_resp(
        struct nsp_sreq *srqst)
{
    wbuf_t nlwbuf;
    struct ieee80211vap *vap, *nextvap;
    void *nlh;
    struct nsp_header nsp_hdr;
    struct nsp_sresp sresp;
    char *buf;
    ieee80211_ssid ssidlist[1];

    memset(&sresp, 0, sizeof(sresp));
    nsp_hdr.SFD = START_OF_FRAME;
    nsp_hdr.version = NSP_VERSION;
    nsp_hdr.frame_type = NSP_SRESP;
    nsp_hdr.frame_length = SRES_LEN;
  
    if((ieee80211_wifiposdata_g->ic == NULL) || (ieee80211_wifiposdata_g->ic->ic_osdev == NULL) ||
            (ieee80211_wifiposdata_g->ic->ic_osdev->netdev == NULL)) {
        WIFIPOS_DPRINTK("WifiPositioning:NULL ic");
        return;
    }
    vap = TAILQ_FIRST(&ieee80211_wifiposdata_g->ic->ic_vaps);
    if (vap == NULL)
    {
        WIFIPOS_DPRINTK("vap is null\n");
        return;
    }
    ieee80211_print_nsphdr(vap, &nsp_hdr);
   
    sresp.request_id = srqst->request_id;
    sresp.req_tstamp = OS_GET_TIMESTAMP();
    ieee80211_wifiposdata_g->sta_cnt = wlan_iterate_station_list(vap, NULL, NULL);
    sresp.no_of_stations = ieee80211_wifiposdata_g->sta_cnt;
    IEEE80211_ADDR_COPY(sresp.ap1_mac, vap->iv_myaddr);
    sresp.ap1_channel = vap->iv_ic->ic_curchan->ic_ieee;
    buf = OS_MALLOC(ieee80211_wifiposdata_g->ic->ic_osdev, NSP_HDR_LEN + SRES_LEN+ ieee80211_wifiposdata_g->sta_cnt*SINFO_LEN, GFP_KERNEL);
    memcpy(buf, &nsp_hdr, NSP_HDR_LEN);
    wlan_iterate_station_list(vap, sta_list, buf);
    nextvap = TAILQ_NEXT(vap, iv_next);
    if (nextvap != NULL) {
        IEEE80211_ADDR_COPY(sresp.ap2_mac, nextvap->iv_myaddr);
        sresp.ap2_channel = nextvap->iv_ic->ic_curchan->ic_ieee;
        wlan_iterate_station_list(nextvap, sta_list, buf);
    }
    sresp.resp_tstamp = OS_GET_TIMESTAMP();
    wlan_get_bss_essid(vap, ssidlist);
    OS_MEMCPY(sresp.ssid, ssidlist[0].ssid, ssidlist[0].len);
    ieee80211_print_sresp(vap, &sresp);
    memcpy((buf + NSP_HDR_LEN), &sresp, SRES_LEN);
    nlwbuf = wbuf_alloc(ieee80211_wifiposdata_g->ic->ic_osdev,
            WBUF_MAX_TYPE, OS_NLMSG_SPACE(NSP_HDR_LEN + SRES_LEN + ieee80211_wifiposdata_g->sta_cnt*SINFO_LEN));
    if (nlwbuf ==NULL)
    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:wbuf allocation failure");
        return;
    }
    wbuf_append(nlwbuf,OS_NLMSG_SPACE(NSP_HDR_LEN + SRES_LEN + ieee80211_wifiposdata_g->sta_cnt*SINFO_LEN));

    nlh = wbuf_raw_data(nlwbuf);
    OS_SET_NETLINK_HEADER(nlh, NLMSG_LENGTH(NSP_HDR_LEN + SRES_LEN + ieee80211_wifiposdata_g->sta_cnt*SINFO_LEN) ,
            0,0,0,ieee80211_wifiposdata_g->process_pid);
    wbuf_set_netlink_pid(nlwbuf,0);
    wbuf_set_netlink_dst_group(nlwbuf,0);
    memcpy(OS_NLMSG_DATA(nlh), buf, OS_NLMSG_SPACE(NSP_HDR_LEN + SRES_LEN + ieee80211_wifiposdata_g->sta_cnt*SINFO_LEN));
    OS_NETLINK_UCAST(ieee80211_wifiposdata_g->wifipos_sock, nlwbuf, 
            ieee80211_wifiposdata_g->process_pid, 0);
    OS_FREE(buf);
}
static void ieee80211_wifipos_nlsend_cap_resp(
        struct nsp_cap_req *crqst)
{
    wbuf_t nlwbuf;
    void *nlh;
    struct nsp_header nsp_hdr;
    struct nsp_cap_resp cresp;
    char buf[OS_NLMSG_SPACE(NSP_HDR_LEN + CAPRES_LEN)];
    struct ieee80211vap *vap;

    memset(&cresp, 0, sizeof(cresp));
    memset(&buf, 0, sizeof(buf));
    nsp_hdr.SFD = START_OF_FRAME;
    nsp_hdr.version = NSP_VERSION;
    nsp_hdr.frame_type = NSP_CRESP;
    nsp_hdr.frame_length = CAPRES_LEN;
    
    vap = TAILQ_FIRST(&ieee80211_wifiposdata_g->ic->ic_vaps);
    if (vap == NULL)
    {
        WIFIPOS_DPRINTK("vap is null\n");
        return;
    }
    ieee80211_print_nsphdr(vap, &nsp_hdr);
    memcpy(buf, &nsp_hdr, NSP_HDR_LEN);

    cresp.request_id = crqst->request_id;
    cresp.band = ATH_WIFIPOS_SINGLE_BAND;
    cresp.positioning = ATH_WIFIPOS_POSITIONING;
    cresp.sw_version = NSP_VERSION;
    cresp.hw_version = NSP_HW_VERSION;
    cresp.clk_freq = 44;
    
    ieee80211_print_cresp(vap, &cresp);

    memcpy((buf + NSP_HDR_LEN), &cresp, CAPRES_LEN);
    nlwbuf = wbuf_alloc(ieee80211_wifiposdata_g->ic->ic_osdev,
            WBUF_MAX_TYPE, OS_NLMSG_SPACE(NSP_HDR_LEN + CAPRES_LEN));
    if (nlwbuf ==NULL)
    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:wbuf allocation failure");
        return;
    }
    wbuf_append(nlwbuf,OS_NLMSG_SPACE(NSP_HDR_LEN + CAPRES_LEN));

    nlh = wbuf_raw_data(nlwbuf);
    OS_SET_NETLINK_HEADER(nlh, NLMSG_LENGTH(NSP_HDR_LEN + CAPRES_LEN) ,
            0,0,0,ieee80211_wifiposdata_g->process_pid);
    wbuf_set_netlink_pid(nlwbuf,0);
    wbuf_set_netlink_dst_group(nlwbuf,0);
    memcpy(OS_NLMSG_DATA(nlh), buf, OS_NLMSG_SPACE(NSP_HDR_LEN + CAPRES_LEN));
    OS_NETLINK_UCAST(ieee80211_wifiposdata_g->wifipos_sock, nlwbuf, 
            ieee80211_wifiposdata_g->process_pid, 0);
}
static void ieee80211_wifipos_nlsend_tsf_resp(
        struct ieee80211vap *vap,
        struct ieee80211_wifipos_tsfreq_s *tsfres)
{
    wbuf_t nlwbuf;
    void *nlh;
    struct nsp_header nsp_hdr;
    struct nsp_tsf_resp tsfresp={0};
    char buf[OS_NLMSG_SPACE(NSP_HDR_LEN + TSFRES_LEN)];

    memset(&buf, 0, sizeof(buf));
    nsp_hdr.SFD = START_OF_FRAME;
    nsp_hdr.version = 1;
    nsp_hdr.frame_type = NSP_TSFRESP;
    nsp_hdr.frame_length = TSFRES_LEN;

    ieee80211_print_nsphdr(vap, &nsp_hdr);
    memcpy(buf, &nsp_hdr, NSP_HDR_LEN);

    tsfresp.request_id = 1; // tsfres->request_id;
    memcpy(tsfresp.ap_mac_addr, tsfres->assoc_ap_mac_addr, ETH_ALEN);
    tsfresp.result = 0;
    tsfresp.assoc_tsf = tsfres->assoc_ap_tsf;
    tsfresp.prob_tsf = tsfres->probe_ap_tsf;
    ieee80211_print_tsfresp(vap, &tsfresp);

    memcpy((buf + NSP_HDR_LEN), &tsfresp, TSFRES_LEN);
    nlwbuf = wbuf_alloc(ieee80211_wifiposdata_g->ic->ic_osdev,
            WBUF_MAX_TYPE, OS_NLMSG_SPACE(NSP_HDR_LEN + TSFRES_LEN));
    if (nlwbuf ==NULL)
    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:wbuf allocation failure");
        return;
    }
    wbuf_append(nlwbuf,OS_NLMSG_SPACE(NSP_HDR_LEN + TSFRES_LEN));

    nlh = wbuf_raw_data(nlwbuf);
    OS_SET_NETLINK_HEADER(nlh, NLMSG_LENGTH(NSP_HDR_LEN + TSFRES_LEN) ,
            0,0,0,ieee80211_wifiposdata_g->process_pid);
    wbuf_set_netlink_pid(nlwbuf,0);
    wbuf_set_netlink_dst_group(nlwbuf,0);
    memcpy(OS_NLMSG_DATA(nlh), buf, OS_NLMSG_SPACE(NSP_HDR_LEN + TSFRES_LEN));
    OS_NETLINK_UCAST(ieee80211_wifiposdata_g->wifipos_sock, nlwbuf,
            ieee80211_wifiposdata_g->process_pid, 0);
}

static int ieee80211_wifipos_tsf_request(struct nsp_tsf_req *tsfrqst)
{
    struct ieee80211vap *vap;
    struct ieee80211_node *bss_node;
    int err = -1, skip=0;
    int i, nodeindex;
    static int index = 0;

    if((ieee80211_wifiposdata_g->ic == NULL) || (ieee80211_wifiposdata_g->ic->ic_osdev == NULL) ||
            (ieee80211_wifiposdata_g->ic->ic_osdev->netdev == NULL)) {
        WIFIPOS_DPRINTK("WifiPositioning:NULL ic");
        return -1;
    }

    spin_lock(&tsf_req_lock);
    for(i=0; i < ATH_WIFIPOS_MAX_CONCURRENT_TSF_AP; i++) {
        if(IEEE80211_ADDR_EQ(ieee80211_wifipos_tsfreq_g[i].assoc_ap_mac_addr, tsfrqst->ap_mac_addr)) {
            skip = 1;
            nodeindex = i;
            break;
        } else {
            continue;
        }
    }

    if (skip ==0) {
        IEEE80211_ADDR_COPY(ieee80211_wifipos_tsfreq_g[index].assoc_ap_mac_addr, tsfrqst->ap_mac_addr);
        nodeindex = index;
        ++index;
        if (index > 20)
            index = 0;
    }
    vap = TAILQ_FIRST(&ieee80211_wifiposdata_g->ic->ic_vaps);
    if (vap == NULL)
    {
        WIFIPOS_DPRINTK("vap is null\n");
        return -1;
    }
    bss_node = ieee80211_ref_bss_node(vap);

    if (tsfrqst->channel != ieee80211_wifiposdata_g->ic->ic_curchan->ic_ieee) {
        ieee80211_send_cts(vap->iv_bss,MAX_CTS_TO_SELF_TIME);
        OS_SET_TIMER(&ieee80211_wifiposdata_g->wifipos_timer, MAX_CTS_TO_SELF_TIME);
        wlan_set_channel(vap,tsfrqst->channel);
    }
    err = ieee80211_send_probereq(bss_node,
            vap->iv_myaddr,
            ieee80211_wifipos_tsfreq_g[nodeindex].assoc_ap_mac_addr,
            vap->iv_myaddr,
            tsfrqst->ssid,   
            strlen(tsfrqst->ssid), 
            NULL,
            0);

    spin_unlock(&tsf_req_lock);
    ieee80211_free_node(bss_node);
    return err;
}

static void ieee80211_wifipos_nlsend_sleep_resp(
        struct nsp_sleep_req *slrqst,
        unsigned char result)
{
    wbuf_t nlwbuf;
    void *nlh;
    struct nsp_header nsp_hdr;
    struct nsp_sleep_resp slresp={0};
    char buf[OS_NLMSG_SPACE(NSP_HDR_LEN + SLEEPRES_LEN)];
    struct ieee80211vap *vap;

    memset(&buf, 0, sizeof(buf));
    nsp_hdr.SFD = START_OF_FRAME;
    nsp_hdr.version = NSP_VERSION;
    nsp_hdr.frame_type = NSP_STARETURNTOSLEEPRES;
    nsp_hdr.frame_length = SLEEPRES_LEN;
    
    vap = TAILQ_FIRST(&ieee80211_wifiposdata_g->ic->ic_vaps);
    if (vap == NULL)
    {
        WIFIPOS_DPRINTK("vap is null\n");
        return;
    }
    ieee80211_print_nsphdr(vap, &nsp_hdr);
    memcpy(buf, &nsp_hdr, NSP_HDR_LEN);

    slresp.request_id = slrqst->request_id;
    memcpy(slresp.sta_mac_addr, slrqst->sta_mac_addr, ETH_ALEN);
    slresp.result = result;
                                        
    ieee80211_print_slresp(vap, &slresp);

    memcpy((buf + NSP_HDR_LEN), &slresp, SLEEPRES_LEN);
    nlwbuf = wbuf_alloc(ieee80211_wifiposdata_g->ic->ic_osdev,
            WBUF_MAX_TYPE, OS_NLMSG_SPACE(NSP_HDR_LEN + SLEEPRES_LEN));
    if (nlwbuf ==NULL)
    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:wbuf allocation failure");
        return;
    }
    wbuf_append(nlwbuf,OS_NLMSG_SPACE(NSP_HDR_LEN + SLEEPRES_LEN));

    nlh = wbuf_raw_data(nlwbuf);
    OS_SET_NETLINK_HEADER(nlh, NLMSG_LENGTH(NSP_HDR_LEN + SLEEPRES_LEN) ,
            0,0,0,ieee80211_wifiposdata_g->process_pid);
    wbuf_set_netlink_pid(nlwbuf,0);
    wbuf_set_netlink_dst_group(nlwbuf,0);
    memcpy(OS_NLMSG_DATA(nlh), buf, OS_NLMSG_SPACE(NSP_HDR_LEN + SLEEPRES_LEN));
    OS_NETLINK_UCAST(ieee80211_wifiposdata_g->wifipos_sock, nlwbuf, 
            ieee80211_wifiposdata_g->process_pid, 0);
}

static void ieee80211_wifipos_nlsend_wakeup_resp(
        struct nsp_wakeup_req *wrqst,
        unsigned char result)
{
    wbuf_t nlwbuf;
    void *nlh;
    struct nsp_header nsp_hdr;
    struct nsp_wakeup_resp wresp={0};
    char buf[OS_NLMSG_SPACE(NSP_HDR_LEN + WAKEUPRES_LEN)];
    struct ieee80211vap *vap;

    memset(&buf, 0, sizeof(buf));
    nsp_hdr.SFD = START_OF_FRAME;
    nsp_hdr.version = NSP_VERSION;
    nsp_hdr.frame_type = NSP_WAKEUPRESP;
    nsp_hdr.frame_length = WAKEUPRES_LEN;
    
    vap = TAILQ_FIRST(&ieee80211_wifiposdata_g->ic->ic_vaps);
    if (vap == NULL)
    {
        WIFIPOS_DPRINTK("vap is null\n");
        return;
    }
    ieee80211_print_nsphdr(vap, &nsp_hdr);
    memcpy(buf, &nsp_hdr, NSP_HDR_LEN);

    wresp.request_id = wrqst->request_id;
    memcpy(wresp.sta_mac_addr, wrqst->sta_mac_addr, ETH_ALEN);
    wresp.result = result;
    
    ieee80211_print_wresp(vap, &wresp);

    memcpy((buf + NSP_HDR_LEN), &wresp, WAKEUPRES_LEN);
    nlwbuf = wbuf_alloc(ieee80211_wifiposdata_g->ic->ic_osdev,
            WBUF_MAX_TYPE, OS_NLMSG_SPACE(NSP_HDR_LEN + WAKEUPRES_LEN));
    if (nlwbuf ==NULL)
    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:wbuf allocation failure");
        return;
    }
    wbuf_append(nlwbuf,OS_NLMSG_SPACE(NSP_HDR_LEN + WAKEUPRES_LEN));

    nlh = wbuf_raw_data(nlwbuf);
    OS_SET_NETLINK_HEADER(nlh, NLMSG_LENGTH(NSP_HDR_LEN + WAKEUPRES_LEN) ,
            0,0,0,ieee80211_wifiposdata_g->process_pid);
    wbuf_set_netlink_pid(nlwbuf,0);
    wbuf_set_netlink_dst_group(nlwbuf,0);
    memcpy(OS_NLMSG_DATA(nlh), buf, OS_NLMSG_SPACE(NSP_HDR_LEN + WAKEUPRES_LEN));
    OS_NETLINK_UCAST(ieee80211_wifiposdata_g->wifipos_sock, nlwbuf, 
            ieee80211_wifiposdata_g->process_pid, 0);
}
static int ieee80211_wifipos_status_request(struct nsp_sreq *srqst)
{
    ieee80211_wifipos_nlsend_status_resp(srqst);
    return 0;
}
static int ieee80211_wifipos_cap_request(struct nsp_cap_req *crqst)
{
    ieee80211_wifipos_nlsend_cap_resp(crqst);
    return 0;
}
static int ieee80211_wifipos_sleep_request(struct nsp_sleep_req *slrqst)
{
    struct ieee80211vap *vap;
    struct ieee80211_node *ni;
    int i, result = 0;
    unsigned char s[IEEE80211_ADDR_LEN];
    memset(s, 0, IEEE80211_ADDR_LEN);

    if((ieee80211_wifiposdata_g->ic == NULL) || (ieee80211_wifiposdata_g->ic->ic_osdev == NULL) ||
            (ieee80211_wifiposdata_g->ic->ic_osdev->netdev == NULL)) {
        WIFIPOS_DPRINTK("WifiPositioning:NULL ic");
        return -1;
    }
    vap = TAILQ_FIRST(&ieee80211_wifiposdata_g->ic->ic_vaps);
    if (vap == NULL)
    {
        WIFIPOS_DPRINTK("vap is null\n");
        return -1;
    }

    if(IEEE80211_ADDR_EQ(slrqst->sta_mac_addr, s)) {
        spin_lock(&tsf_req_lock);
        for(i=0; i < ATH_WIFIPOS_MAX_CONCURRENT_TSF_AP; i++) {
            memset(&ieee80211_wifipos_tsfreq_g[i].assoc_ap_mac_addr, 0, IEEE80211_ADDR_LEN);
            ieee80211_wifipos_tsfreq_g[i].probe_ap_tsf = 0;
            ieee80211_wifipos_tsfreq_g[i].assoc_ap_tsf = 0;
            ieee80211_wifipos_tsfreq_g[i].diff_tsf = 0;
            ieee80211_wifipos_tsfreq_g[i].valid = 0;
        }
        spin_unlock(&tsf_req_lock);
        ieee80211_wifipos_nlsend_sleep_resp(slrqst,result);
    } else {
        for(i=0; i < ATH_WIFIPOS_MAX_CONCURRENT_WAKEUP_STA; i++) {
            spin_lock(&wakeup_lock);
            if(IEEE80211_ADDR_EQ(ieee80211_wifipos_wakeup_g[i].sta_mac_addr, slrqst->sta_mac_addr)) {
                ni = ieee80211_find_node(&ieee80211_wifiposdata_g->ic->ic_sta, ieee80211_wifipos_wakeup_g[i].sta_mac_addr); 
                if (ni == NULL) {
                    result = -1;
                    spin_unlock(&wakeup_lock);
                    ieee80211_wifipos_nlsend_sleep_resp(slrqst,result);
                    return -1;
                }
                ni->ni_flags &= ~IEEE80211_NODE_WAKEUP;
                memset(&ieee80211_wifipos_wakeup_g[i].sta_mac_addr, 0, IEEE80211_ADDR_LEN);
                --wakeup_sta_count;
                ieee80211_wifipos_nlsend_sleep_resp(slrqst,result);
            } 
            spin_unlock(&wakeup_lock);
        }
        if(wakeup_sta_count == 0) {
            vap->iv_wakeup = 0;
            OS_CANCEL_TIMER(&ieee80211_wifiposdata_g->wifipos_wakeup_timer);
        }
    }
    return 0;
}

static int ieee80211_wifipos_wakeup_request(struct nsp_wakeup_req *wrqst)
{
    struct ieee80211_node *ni;
    struct ieee80211vap *vap;
    static int index = 0;
    int i, skip = 0;
    int result = 0;

    if((ieee80211_wifiposdata_g->ic == NULL) || (ieee80211_wifiposdata_g->ic->ic_osdev == NULL) ||
            (ieee80211_wifiposdata_g->ic->ic_osdev->netdev == NULL)) {
        WIFIPOS_DPRINTK("WifiPositioning:NULL ic");
        return -1;
    }

    spin_lock(&wakeup_lock);
    for(i=0; i < ATH_WIFIPOS_MAX_CONCURRENT_WAKEUP_STA; i++) {
        if(IEEE80211_ADDR_EQ(ieee80211_wifipos_wakeup_g[i].sta_mac_addr, wrqst->sta_mac_addr)) {
            skip = 1;
            break;
        } else {
            continue;
        }
    }

    ni = ieee80211_find_node(&ieee80211_wifiposdata_g->ic->ic_sta, wrqst->sta_mac_addr); 
    if (ni == NULL || IEEE80211_AID(ni->ni_associd) == 0) {
        result = -1;
        spin_unlock(&wakeup_lock);
        ieee80211_wifipos_nlsend_wakeup_resp(wrqst, result);
        return -1;
    }

    ieee80211_wifipos_nlsend_wakeup_resp(wrqst, result);

    vap = TAILQ_FIRST(&ieee80211_wifiposdata_g->ic->ic_vaps);
    if (vap == NULL)
    {
        WIFIPOS_DPRINTK("vap is null\n");
        return -1;
    }

    vap->iv_wakeup = wrqst->mode;

    if (skip == 0) {
        memcpy(ieee80211_wifipos_wakeup_g[index].sta_mac_addr, wrqst->sta_mac_addr, ETH_ALEN);
        ieee80211_wifipos_wakeup_g[index].wakeup_interval = wrqst->wakeup_interval;
        ++wakeup_sta_count;
        index++;
        if(index >= ATH_WIFIPOS_MAX_CONCURRENT_WAKEUP_STA) {
            index = 0;
        }
    }
    spin_unlock(&wakeup_lock);

    OS_SET_TIMER(&ieee80211_wifiposdata_g->wifipos_wakeup_timer, 0);
    return 0;
}

static int ieee80211_wifipos_send_probe(struct nsp_mrqst *mrqst)
{
    struct ieee80211_node *ni;
    struct ieee80211vap *vap;
    static bool update_bit_pos = true;
    struct ath_softc_net80211 *scn;
    u_int8_t pwr_save, cnt;
    int qosresp, wifipos_tsf_index;
    int ps_enabled = 0;
    struct ieee80211com *ic;
    u_int32_t diff_tbtt;
    int i;

    if((ieee80211_wifiposdata_g->ic == NULL) || (ieee80211_wifiposdata_g->ic->ic_osdev == NULL) ||
            (ieee80211_wifiposdata_g->ic->ic_osdev->netdev == NULL)) {
        WIFIPOS_DPRINTK("WifiPositioning:NULL ic");
        return -1;
    }

    vap = TAILQ_FIRST(&ieee80211_wifiposdata_g->ic->ic_vaps);
    if (vap == NULL) {
        WIFIPOS_DPRINTK("WifiPositioning:Vap is null\n");
        return -1;
    }
    scn = ATH_SOFTC_NET80211(vap->iv_ic);
    ic = vap->iv_ic;
    if(update_bit_pos) {
        scn->sc_ops->update_loc_ctl_reg(scn->sc_dev, 1);
        update_bit_pos = false;
    }
    ni = ieee80211_find_node(&ieee80211_wifiposdata_g->ic->ic_sta, mrqst->sta_mac_addr);
    if (ni == NULL) {
        if (vap == NULL) {
            WIFIPOS_DPRINTK("WifiPositioning:Vap is NULL");
            return -1;
        }
        ni = ieee80211_tmp_node(vap, mrqst->sta_mac_addr);
        if(mrqst->spoof_mac_addr[0] || mrqst->spoof_mac_addr[1] 
            || mrqst->spoof_mac_addr[2] || mrqst->spoof_mac_addr[3]
            || mrqst->spoof_mac_addr[4] || mrqst->spoof_mac_addr[5]) {
            IEEE80211_ADDR_COPY(ni->ni_bssid, mrqst->spoof_mac_addr);
        }
    }
    if (mrqst->channel != ieee80211_wifiposdata_g->ic->ic_curchan->ic_ieee) {
        ieee80211_send_cts(vap->iv_bss,MAX_CTS_TO_SELF_TIME);
    }

    if (mrqst->transmit_rate) {
        ieee80211_wifiposdata_g->reqdata->rateset = (mrqst->transmit_rate | mrqst->transmit_rate << 8 |
                mrqst->transmit_rate << 16 | mrqst->transmit_rate << 24);
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:\nRate:%x rateset:%x", mrqst->transmit_rate, ieee80211_wifiposdata_g->reqdata->rateset);
    }
    else {
        /* if rate is not specified in measurement request 
         * use 6 mbps */
        ieee80211_wifiposdata_g->reqdata->rateset = 0x0b0b0b0b;
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:\nUse 6 mbps rateset:%x", ieee80211_wifiposdata_g->reqdata->rateset);
    }
    ieee80211_wifiposdata_g->reqdata->retryset = 0x04040404;
    memcpy(ieee80211_wifiposdata_g->reqdata->sta_mac_addr, mrqst->sta_mac_addr, ETH_ALEN);
    ieee80211_wifiposdata_g->reqdata->request_id = mrqst->request_id;
    ieee80211_wifiposdata_g->reqdata->req_tstamp = CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP());
    ieee80211_wifiposdata_g->reqdata->pkt_type = (mrqst->mode & FRAME_TYPE_HASH)>>2;

    if(vap->iv_wakeup) {
        for(i=0; i < ATH_WIFIPOS_MAX_CONCURRENT_WAKEUP_STA; i++) {
            if(IEEE80211_ADDR_EQ(ieee80211_wifipos_wakeup_g[i].sta_mac_addr, mrqst->sta_mac_addr)) {
                ieee80211_wifipos_wakeup_g[i].timestamp = CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP());
                break;
            }
        }
    }

    if (mrqst->channel != ieee80211_wifiposdata_g->ic->ic_curchan->ic_ieee) {
        OS_SET_TIMER(&ieee80211_wifiposdata_g->wifipos_timer, MAX_CTS_TO_SELF_TIME);
        wlan_set_channel(vap,mrqst->channel);
    }
    ieee80211_wifiposdata_g->ni_tmp_sta = ni;
    pwr_save = ieee80211node_has_flag(ni,IEEE80211_NODE_PWR_MGT); 
    
    if((mrqst->mode & TX_CHAINMASK_HASH) == TX_CHAINMASK_1)
        ieee80211_wifiposdata_g->reqdata->txchainmask = 1;
    if((mrqst->mode & TX_CHAINMASK_HASH) == TX_CHAINMASK_2)
        ieee80211_wifiposdata_g->reqdata->txchainmask = 3;	    
    if((mrqst->mode & TX_CHAINMASK_HASH) == TX_CHAINMASK_3)
        ieee80211_wifiposdata_g->reqdata->txchainmask = 7;
    if((mrqst->mode & RX_CHAINMASK_HASH) == RX_CHAINMASK_1)
        ieee80211_wifiposdata_g->reqdata->rxchainmask = 1;
    if((mrqst->mode & RX_CHAINMASK_HASH) == RX_CHAINMASK_2)
        ieee80211_wifiposdata_g->reqdata->rxchainmask = 3;
    if((mrqst->mode & RX_CHAINMASK_HASH) == RX_CHAINMASK_3)
        ieee80211_wifiposdata_g->reqdata->rxchainmask = 7;
    
    ieee80211_wifiposdata_g->reqdata->request_cnt = mrqst->no_of_measurements;
    ieee80211_wifiposdata_g->channel = mrqst->channel;
    if (mrqst->mode & SYNCHRONIZE) {
        spin_lock(&tsf_req_lock);
        for(wifipos_tsf_index = 0; wifipos_tsf_index < ATH_WIFIPOS_MAX_CONCURRENT_TSF_AP; wifipos_tsf_index++) {
            if (IEEE80211_ADDR_EQ(ieee80211_wifipos_tsfreq_g[wifipos_tsf_index].assoc_ap_mac_addr, mrqst->spoof_mac_addr)) {
                if (ieee80211_wifipos_tsfreq_g[wifipos_tsf_index].probe_ap_tsf != 0 &&
                        ieee80211_wifipos_tsfreq_g[wifipos_tsf_index].assoc_ap_tsf != 0) {
                    ps_enabled = 1;
                    IEEE80211_ADDR_COPY(ieee80211_wifiposdata_g->reqdata->spoof_mac_addr, mrqst->spoof_mac_addr);
                    break;
                }
            }
        } 
        spin_unlock(&tsf_req_lock);
    }
    if (ps_enabled == 1) {
        spin_lock(&tsf_req_lock);
        for(wifipos_tsf_index = 0; wifipos_tsf_index < ATH_WIFIPOS_MAX_CONCURRENT_TSF_AP; wifipos_tsf_index++) {
            if (IEEE80211_ADDR_EQ(ieee80211_wifipos_tsfreq_g[wifipos_tsf_index].assoc_ap_mac_addr, mrqst->spoof_mac_addr)) {
                if (ieee80211_wifipos_tsfreq_g[wifipos_tsf_index].probe_ap_tsf != 0 &&
                        ieee80211_wifipos_tsfreq_g[wifipos_tsf_index].assoc_ap_tsf != 0) {
                    diff_tbtt = ic->ic_get_TSF64(ic) - (ieee80211_get_next_tbtt_tsf_64(vap) - 102400);
                    if(diff_tbtt < ieee80211_wifipos_tsfreq_g[wifipos_tsf_index].diff_tsf) {
                        diff_tbtt = ieee80211_wifipos_tsfreq_g[wifipos_tsf_index].diff_tsf - diff_tbtt;
                    }
                    else {
                        diff_tbtt = ieee80211_get_next_tbtt_tsf_64(vap) - ic->ic_get_TSF64(ic) + ieee80211_wifipos_tsfreq_g[wifipos_tsf_index].diff_tsf;
                    }
                    diff_tbtt = diff_tbtt/1000;
                    OS_SET_TIMER(&ieee80211_wifiposdata_g->wifipos_ps_timer, diff_tbtt);
                    
                }
            }
        }
        spin_unlock(&tsf_req_lock);
    } else {
        for (cnt = 0; cnt < ieee80211_wifiposdata_g->reqdata->request_cnt; cnt++)
        {
            if ((mrqst->mode & FRAME_TYPE_HASH) == QOS_NULL_FRAME) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:QOS NULL frame:%d",cnt);
                qosresp = ieee80211_send_qosnull_probe(ni, WME_AC_VO, pwr_save, (void *)ieee80211_wifiposdata_g->reqdata);
            }
            else if ((mrqst->mode & FRAME_TYPE_HASH) == NULL_FRAME) {
                IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:NULL frame:%d",cnt);
                ieee80211_send_null_probe(ni, pwr_save, (void *)ieee80211_wifiposdata_g->reqdata);
            }
        }
    }
    ieee80211_free_node(ieee80211_wifiposdata_g->ni_tmp_sta);
    return qosresp;
}

void ieee80211_wifpos_transmit_probe(wbuf_t msg)
{
    void *nlh;
    struct nsp_header *nsp_hdr;
    struct nsp_mrqst *mrqst;
    struct nsp_sreq *srqst;
    struct nsp_cap_req *crqst;
    struct nsp_wakeup_req *wrqst;
    struct nsp_sleep_req *slrqst;
    struct nsp_tsf_req *tsfrqst;
    char *buf;
    struct ieee80211vap *vap;
    
    nlh = wbuf_raw_data(msg);
    ieee80211_wifiposdata_g->process_pid = ((struct nlmsghdr *)nlh)->nlmsg_pid; /*pid of sending process */
    nsp_hdr = (struct nsp_header *)OS_NLMSG_DATA(nlh);
    buf = (char *)nsp_hdr;
    vap = TAILQ_FIRST(&ieee80211_wifiposdata_g->ic->ic_vaps);
    if (vap == NULL) {
        WIFIPOS_DPRINTK("WifiPositioning:Vap is null\n");
        return;
    }
    ieee80211_print_nsphdr(vap, nsp_hdr);
    if (nsp_hdr->frame_type == NSP_WAKEUPRQST) {
        wrqst = (struct nsp_wakeup_req *)(buf + NSP_HDR_LEN);
        ieee80211_print_wrqst(vap, wrqst);
        if (ieee80211_wifipos_wakeup_request(wrqst) != 0) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Error in wakeup_request\n");
        }
    } else if (nsp_hdr->frame_type == NSP_MRQST) {
        mrqst = (struct nsp_mrqst *)(buf + NSP_HDR_LEN);
        ieee80211_print_mrqst(vap, mrqst);
        if (ieee80211_wifipos_send_probe(mrqst) != 0) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Error in send_qosnull\n");
        }
    } else if (nsp_hdr->frame_type == NSP_SRQST) {
        srqst = (struct nsp_sreq *)(buf + NSP_HDR_LEN);
        ieee80211_print_srqst(vap, srqst);
        if (ieee80211_wifipos_status_request(srqst) != 0) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Error in status request\n");
        }
    } else if (nsp_hdr->frame_type == NSP_CRQST) {
        crqst = (struct nsp_cap_req *)(buf + NSP_HDR_LEN);
        ieee80211_print_crqst(vap, crqst);
        if (ieee80211_wifipos_cap_request(crqst) != 0) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Error in Capability Request\n");
        }
    } else if (nsp_hdr->frame_type == NSP_STARETURNTOSLEEPREQ) {
        slrqst = (struct nsp_sleep_req *)(buf + NSP_HDR_LEN);
        ieee80211_print_slrqst(vap, slrqst);
        if (ieee80211_wifipos_sleep_request(slrqst) != 0) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Error in sleep_request\n");
        }
    } else if (nsp_hdr->frame_type == NSP_TSFRQST) {
        tsfrqst = (struct nsp_tsf_req *)(buf + NSP_HDR_LEN);
        ieee80211_print_tsfrqst(vap, tsfrqst);
        if (ieee80211_wifipos_tsf_request(tsfrqst) != 0) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Error in tsf_request\n");
        }
    }
}

static void do_framelength_correction_in_tod(ieee80211_wifiposdata_t *wifiposdata)
{
    unsigned int txframe_bitlen = 0, txframe_cycles = 0;
    unsigned int dbps = 0;
    unsigned int padbits = 0;
    txframe_bitlen =  IEEE80211_HDR_SERVICE_BITS_LEN + packetlength[ieee80211_wifiposdata_g->reqdata->pkt_type] * 8 + IEEE80211_HDR_TAIL_BITS_LEN;
    dbps = get_dbps_by_rate(ieee80211_wifiposdata_g->reqdata->rateset >> 24);
    if(txframe_bitlen % dbps) {
	    padbits = dbps - (txframe_bitlen % dbps);
	    txframe_bitlen += padbits ;
    }
    txframe_cycles = ((txframe_bitlen/dbps) * SYMBOL_DURATION_MICRO_SEC  + L_LTF + L_STF + L_SIG) * NO_OF_CLK_CYCLES_PER_MICROSEC;
    
    wifiposdata->tod = wifiposdata->tod + txframe_cycles + ieee80211_wifiposdata_g->txcorrection;
}

static void do_framelength_correction_in_toa(ieee80211_wifiposdata_t *wifiposdata)
{
    unsigned int rxframe_bitlen = 0, rxframe_cycles = 0;
    unsigned int dbps = 0;
    unsigned int padbits = 0;

    dbps = get_dbps_by_rate(wifiposdata->rate); 
    rxframe_bitlen = IEEE80211_HDR_SERVICE_BITS_LEN + ACK_PKT_BYTE_SIZE * 8 + IEEE80211_HDR_TAIL_BITS_LEN;
    padbits = 0;
    if(rxframe_bitlen % dbps) {
	    padbits = dbps - (rxframe_bitlen % dbps);
	    rxframe_bitlen += padbits;
    }
    rxframe_cycles = ((rxframe_bitlen/dbps) * SYMBOL_DURATION_MICRO_SEC + L_LTF + L_STF + L_SIG) * NO_OF_CLK_CYCLES_PER_MICROSEC;

    wifiposdata->toa = wifiposdata->toa - rxframe_cycles - ieee80211_wifiposdata_g->rxcorrection;
}

static void ieee80211_wifipos_nlsend_probe_resp(struct ieee80211_wifipos_table_s *wifipos_entry)
{
    wbuf_t nlwbuf;
    void *nlh;
    struct nsp_header nsp_hdr;
    struct nsp_mresp mresp={0};
    struct nsp_type1_resp chanest_resp={0};
    char buf[OS_NLMSG_SPACE(NSP_HDR_LEN + MRES_LEN + TYPE1RES_LEN)];
    unsigned int type1_payld_size;
    struct ieee80211vap *vap;

    memset(&buf, 0, sizeof(buf));
    nsp_hdr.SFD = START_OF_FRAME;
    nsp_hdr.version = NSP_VERSION;
    nsp_hdr.frame_type = NSP_TYPE1RESP;
    nsp_hdr.frame_length = MREQ_LEN + TYPE1RES_LEN;
    
    vap = TAILQ_FIRST(&ieee80211_wifiposdata_g->ic->ic_vaps);
    if (vap == NULL) {
        WIFIPOS_DPRINTK("WifiPositioning:Vap is null\n");
        return;
    }
    ieee80211_print_nsphdr(vap, &nsp_hdr);
    memcpy(buf, &nsp_hdr, NSP_HDR_LEN);

    mresp.request_id = wifipos_entry->request_id;
    memcpy(mresp.sta_mac_addr, wifipos_entry->sta_mac_addr, ETH_ALEN);
    mresp.no_of_responses = 1;
    mresp.req_tstamp = ieee80211_wifiposdata_g->reqdata->req_tstamp;
    mresp.resp_tstamp = CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP());
    
    ieee80211_print_mresp(vap, &mresp);

    chanest_resp.toa = wifipos_entry->toa;
    chanest_resp.tod = wifipos_entry->tod;
    chanest_resp.send_rate = ieee80211_wifiposdata_g->reqdata->rateset >> 24;
    chanest_resp.receive_rate = wifipos_entry->rate;
    chanest_resp.rssi[0] = wifipos_entry->rssi[0];
    chanest_resp.rssi[1] = wifipos_entry->rssi[1];
    chanest_resp.rssi[2] = wifipos_entry->rssi[2];
    chanest_resp.no_of_chains = wifipos_entry->no_of_chains;
    chanest_resp.no_of_retries = wifipos_entry->no_of_retries;
    type1_payld_size  = ATH_DESC_CDUMP_SIZE(chanest_resp.no_of_chains);
    memcpy(chanest_resp.type1_payld, wifipos_entry->type1_payld, type1_payld_size);
    ieee80211_print_type1_payld_resp(vap, &chanest_resp);

    memcpy((buf + NSP_HDR_LEN), &mresp, MRES_LEN);
    memcpy((buf + NSP_HDR_LEN + MRES_LEN), &chanest_resp, TYPE1RES_LEN);

    nlwbuf = wbuf_alloc(ieee80211_wifiposdata_g->ic->ic_osdev,
                   WBUF_MAX_TYPE, OS_NLMSG_SPACE(NSP_HDR_LEN + MRES_LEN + TYPE1RES_LEN));
    if (nlwbuf ==NULL)
    {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:wbuf allocation failure");
        return;
    }
    wbuf_append(nlwbuf,OS_NLMSG_SPACE(NSP_HDR_LEN + MRES_LEN + TYPE1RES_LEN));

    nlh = wbuf_raw_data(nlwbuf);
    OS_SET_NETLINK_HEADER(nlh, NLMSG_LENGTH(NSP_HDR_LEN + MRES_LEN + TYPE1RES_LEN) ,
                     0,0,0,ieee80211_wifiposdata_g->process_pid);
    wbuf_set_netlink_pid(nlwbuf,0);
    wbuf_set_netlink_dst_group(nlwbuf,0);
    memcpy(OS_NLMSG_DATA(nlh), buf, OS_NLMSG_SPACE(NSP_HDR_LEN + MRES_LEN + TYPE1RES_LEN));
    OS_NETLINK_UCAST(ieee80211_wifiposdata_g->wifipos_sock, nlwbuf, 
                    ieee80211_wifiposdata_g->process_pid, 0);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
static void ieee80211_wifipos_wqhandler_tsf(void *work_p)
#else
static void ieee80211_wifipos_wqhandler_tsf(struct work_struct *work_p)
#endif
{
    struct ieee80211vap *vap;
    int i;
    u_int64_t tsf_sync, local_tsf_tstamp;
    u_int64_t beacon_time, diff;

    vap = TAILQ_FIRST(&ieee80211_wifiposdata_g->ic->ic_vaps);
    if (vap == NULL) {
        WIFIPOS_DPRINTK("WifiPositioning:Vap is null\n");
        return;
    }
    spin_lock(&tsf_req_lock);
    for(i=0; i<ATH_WIFIPOS_MAX_CONCURRENT_TSF_AP; i++) {
        if (ieee80211_wifipos_tsfreq_g[i].valid == 1) {
            ieee80211_wifipos_nlsend_tsf_resp(vap, &ieee80211_wifipos_tsfreq_g[i]); 
            ieee80211_wifipos_tsfreq_g[i].valid =0; 
            //WIFIPOS_DPRINTK("Local and other AP tsf (respective time stamp) %llx - %llx\n",
            //    ieee80211_wifipos_tsfreq_g[i].probe_ap_tsf,
            //    le64_to_cpu(ieee80211_wifipos_tsfreq_g[i].assoc_ap_tsf));

            tsf_sync = le64_to_cpu(vap->iv_tsf_sync);
            beacon_time = 102400 - OS_MOD64_TBTT_OFFSET(tsf_sync, 102400);
            //WIFIPOS_DPRINTK("Beacon time is %llx\t",beacon_time);
            tsf_sync = beacon_time + vap->iv_local_tsf_tstamp;
            local_tsf_tstamp = ieee80211_get_next_tbtt_tsf_64(vap);
            //WIFIPOS_DPRINTK("Local and other AP tsf(reference to local timestamp) %llx - %llx\n",
            //        tsf_sync, local_tsf_tstamp);

            if(local_tsf_tstamp > tsf_sync) {
                diff = local_tsf_tstamp - tsf_sync;
                diff = 102400 - OS_MOD64_TBTT_OFFSET(diff, 102400);
            } else {
                diff = tsf_sync - local_tsf_tstamp;
                diff = OS_MOD64_TBTT_OFFSET(diff, 102400);
            }
            ieee80211_wifipos_tsfreq_g[i].diff_tsf = diff;
            //WIFIPOS_DPRINTK("\nFinal tbtt timer time is %x in msec %d\n\n", 
            //    ieee80211_wifipos_tsfreq_g[i].diff_tsf, ieee80211_wifipos_tsfreq_g[i].diff_tsf/1000);
            break;
        }
    }
    spin_unlock(&tsf_req_lock);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
static void ieee80211_wifipos_wqhandler(void *work_p)
#else
static void ieee80211_wifipos_wqhandler(struct work_struct *work_p)
#endif
{
    static int index = 0;
    int temp = index;
    temp = temp ? temp-1 : ATH_WIFIPOS_MAX_CONCURRENT_PROBES-1;
    spin_lock(&ieee80211_wifiposdata_g->lock);
    while(!ieee80211_wifipos_table_g[index].valid && index != temp && index != proc_index) {
        ieee80211_wifipos_table_g[index].toa = ieee80211_wifipos_table_g[index].tod = 0;
        ieee80211_wifipos_table_g[index].valid = FALSE;
        index++;
        if(index >= ATH_WIFIPOS_MAX_CONCURRENT_PROBES) {
            index = 0;
        }
    }
    while (ieee80211_wifipos_table_g[index].valid) {
        ieee80211_wifipos_nlsend_probe_resp(&ieee80211_wifipos_table_g[index]);
        ieee80211_wifipos_table_g[index].toa = ieee80211_wifipos_table_g[index].tod = 0;
        ieee80211_wifipos_table_g[index].valid = FALSE;
        index++;
        if(index >= ATH_WIFIPOS_MAX_CONCURRENT_PROBES) {
            index = 0;
        }
    }
    spin_unlock(&ieee80211_wifiposdata_g->lock);
}

static int find_matching_node(ieee80211_wifiposdata_t *wifiposdata, int desc_type)
{
    int i=0;
    int match_node = -1 ;

    if(desc_type == TX_DESC) {

        i = proc_index;
        while(i != wifipos_rx_index) {
            if( ieee80211_wifipos_table_g[i].toa && (ieee80211_wifipos_table_g[i].toa - wifiposdata->tod) > ONE_SIFS_DURATION  &&
                    (ieee80211_wifipos_table_g[i].toa - wifiposdata->tod) < TWO_SIFS_DURATION ) {
                match_node = i;
                break;
            }
            i++;
            if(i >= ATH_WIFIPOS_MAX_CONCURRENT_PROBES)	
                i=0;

        }
    }
    else { 
        i = proc_index; 
        while(i != wifipos_tx_index) {
            if( ieee80211_wifipos_table_g[i].tod && (wifiposdata->toa - ieee80211_wifipos_table_g[i].tod) > ONE_SIFS_DURATION  &&
                    (wifiposdata->toa - ieee80211_wifipos_table_g[i].tod) < TWO_SIFS_DURATION ) {
                match_node = i;
                break;
            }
            i++;
            if(i >= ATH_WIFIPOS_MAX_CONCURRENT_PROBES)	
                i=0;
        }
    }
    return match_node; 
}

int ieee80211_isthere_wakeup_request(struct ieee80211_node *ni)
{
    if(ni == NULL)
        return 0;

    return (ni->ni_flags & IEEE80211_NODE_WAKEUP);
}

int ieee80211_update_wifipos_stats(ieee80211_wifiposdata_t *wifiposdata)
{
    u_int8_t expected_rx_pkt = 0x00;
    int index = 0;

    if(ieee80211_wifiposdata_g->reqdata->pkt_type == NULL_PKT)
        expected_rx_pkt = FC0_ACK_PKT;
    else if(ieee80211_wifiposdata_g->reqdata->pkt_type == QOS_NULL_PKT)
        expected_rx_pkt = FC0_ACK_PKT;

    spin_lock(&ieee80211_wifiposdata_g->lock);
    if (wifiposdata->flags & ATH_WIFIPOS_TX_UPDATE) {
        if (wifiposdata->flags & ATH_WIFIPOS_TX_STATUS) {
            do_framelength_correction_in_tod(wifiposdata);
            index = find_matching_node(wifiposdata, TX_DESC);
            memcpy(ieee80211_wifipos_table_g[wifipos_tx_index].sta_mac_addr, wifiposdata->sta_mac_addr, ETH_ALEN);
            ieee80211_wifipos_table_g[wifipos_tx_index].request_id = wifiposdata->request_id;
            ieee80211_wifipos_table_g[wifipos_tx_index].no_of_retries = wifiposdata->retries;
            if (index >= 0) {
                ieee80211_wifipos_table_g[index].tod = wifiposdata->tod;
                ieee80211_wifipos_table_g[index].valid = TRUE;
                wifipos_tx_index = index+1;
                proc_index = index;
            }
            else {
            ieee80211_wifipos_table_g[wifipos_tx_index].tod = wifiposdata->tod;
                wifipos_tx_index = wifipos_tx_index+1;
            }
            if(wifipos_tx_index >= ATH_WIFIPOS_MAX_CONCURRENT_PROBES) {
                wifipos_tx_index = 0;
            }
        }
        else {
            WIFIPOS_DPRINTK("WifiPositioning:Tx Status flag not set\n");
        }
    }
    else {
        if(wifiposdata->rx_pkt_type == expected_rx_pkt) {
            do_framelength_correction_in_toa(wifiposdata);
            index = find_matching_node(wifiposdata, RX_DESC);

            if (index >= 0) {

                ieee80211_wifipos_table_g[index].toa = wifiposdata->toa;
                ieee80211_wifipos_table_g[index].no_of_chains = wifiposdata->rxchain;
                ieee80211_wifipos_table_g[index].rate = wifiposdata->rate;

                ieee80211_wifipos_table_g[index].rssi[0] = wifiposdata->rssi0;
                ieee80211_wifipos_table_g[index].rssi[1] = wifiposdata->rssi1;
                ieee80211_wifipos_table_g[index].rssi[2] = wifiposdata->rssi2;
                ieee80211_wifipos_table_g[index].valid = TRUE;
                memcpy(ieee80211_wifipos_table_g[index].type1_payld, wifiposdata->hdump, 
                        ATH_DESC_CDUMP_SIZE(wifiposdata->rxchain));
                wifipos_rx_index = index +1;
                proc_index = index; 

            } 
            else {

            ieee80211_wifipos_table_g[wifipos_rx_index].toa = wifiposdata->toa;
            ieee80211_wifipos_table_g[wifipos_rx_index].no_of_chains = wifiposdata->rxchain;
            ieee80211_wifipos_table_g[wifipos_rx_index].rate = wifiposdata->rate;

            ieee80211_wifipos_table_g[wifipos_rx_index].rssi[0] = wifiposdata->rssi0;
            ieee80211_wifipos_table_g[wifipos_rx_index].rssi[1] = wifiposdata->rssi1;
            ieee80211_wifipos_table_g[wifipos_rx_index].rssi[2] = wifiposdata->rssi2;
            memcpy(ieee80211_wifipos_table_g[wifipos_rx_index].type1_payld, wifiposdata->hdump, 
                    ATH_DESC_CDUMP_SIZE(wifiposdata->rxchain));

                wifipos_rx_index = wifipos_rx_index +1;
            }

            if(wifipos_rx_index >= ATH_WIFIPOS_MAX_CONCURRENT_PROBES) {
                wifipos_rx_index = 0;
            }
        }
        else {
            WIFIPOS_DPRINTK("WifiPositioning:Rx for invalid packet\n");
        }
    }
    if (ieee80211_wifipos_table_g[proc_index].valid && ieee80211_wifipos_table_g[proc_index].toa && ieee80211_wifipos_table_g[proc_index].tod)
    {
        proc_index++;
        schedule_work(&ieee80211_wifiposdata_g->work);
        if(proc_index >= ATH_WIFIPOS_MAX_CONCURRENT_PROBES) {
            proc_index = 0;
        }
    }
    spin_unlock(&ieee80211_wifiposdata_g->lock);
    return 0;
}

static OS_TIMER_FUNC(ieee80211_wifipos_ps_timer)
{
    struct ieee80211_node *ni;
    struct ieee80211vap *vap;
    int cnt, no_of_measurements;

    no_of_measurements = ieee80211_wifiposdata_g->reqdata->request_cnt;
    vap = TAILQ_FIRST(&ieee80211_wifiposdata_g->ic->ic_vaps);
    if (vap == NULL)
    {
        WIFIPOS_DPRINTK("vap is null\n");
        return;
    }
    ni = ieee80211_find_node(&ieee80211_wifiposdata_g->ic->ic_sta, ieee80211_wifiposdata_g->reqdata->sta_mac_addr); 
    if (ni != NULL) {
        IEEE80211_ADDR_COPY(ni->ni_bssid, ieee80211_wifiposdata_g->reqdata->spoof_mac_addr);
    } else {
        ni = ieee80211_tmp_node(vap, ieee80211_wifiposdata_g->reqdata->sta_mac_addr);
        IEEE80211_ADDR_COPY(ni->ni_bssid, ieee80211_wifiposdata_g->reqdata->spoof_mac_addr);
    }
    ieee80211_wifiposdata_g->ni_tmp_sta = ni;
    if (ieee80211_wifiposdata_g->channel != ieee80211_wifiposdata_g->ic->ic_curchan->ic_ieee) {
        ieee80211_send_cts(vap->iv_bss,MAX_CTS_TO_SELF_TIME);
        OS_SET_TIMER(&ieee80211_wifiposdata_g->wifipos_timer, MAX_CTS_TO_SELF_TIME);
        wlan_set_channel(vap,ieee80211_wifiposdata_g->channel);
    }
    for (cnt = 0; cnt < no_of_measurements; cnt++)
    {
        if(ieee80211_wifiposdata_g->reqdata->pkt_type == NULL_PKT)
            ieee80211_send_null_probe(ni, 0, (void *)ieee80211_wifiposdata_g->reqdata);
        else if(ieee80211_wifiposdata_g->reqdata->pkt_type == QOS_NULL_PKT)
            ieee80211_send_qosnull_probe(ni, WME_AC_VO, 0, (void *)ieee80211_wifiposdata_g->reqdata);
    }
    ieee80211_free_node(ieee80211_wifiposdata_g->ni_tmp_sta);
    OS_CANCEL_TIMER(&ieee80211_wifiposdata_g->wifipos_ps_timer);
}

static OS_TIMER_FUNC(ieee80211_wifipos_wakeup_timer)
{
    struct ieee80211_node *ni;
    struct ieee80211vap *vap;
    struct node_powersave_queue *psq;
    int index;
    int ac = WME_AC_VO;
    int i = 0;

    vap = TAILQ_FIRST(&ieee80211_wifiposdata_g->ic->ic_vaps);
    if (vap == NULL)
    {
        WIFIPOS_DPRINTK("vap is null\n");
        return;
    }

    for(index=0; index < ATH_WIFIPOS_MAX_CONCURRENT_WAKEUP_STA; index++) {
        spin_lock(&wakeup_lock);
        if (IEEE80211_ADDR_IS_VALID(ieee80211_wifipos_wakeup_g[index].sta_mac_addr)) {
            ni = ieee80211_find_node(&ieee80211_wifiposdata_g->ic->ic_sta, ieee80211_wifipos_wakeup_g[index].sta_mac_addr); 
            if (ni != NULL) {
                if((ieee80211_wifipos_wakeup_g[index].timestamp != 0) &&
                        ((CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP()) - ieee80211_wifipos_wakeup_g[index].timestamp) > MAX_TIMEOUT)) {
                    printk(" current time %x, %x\n", 
                            CONVERT_SYSTEM_TIME_TO_MS(OS_GET_TIMESTAMP()), 
                            ieee80211_wifipos_wakeup_g[index].timestamp);
                    memset(&ieee80211_wifipos_wakeup_g[index].sta_mac_addr, 0, IEEE80211_ADDR_LEN);
                    ieee80211_wifipos_wakeup_g[index].timestamp = 0;
                    --wakeup_sta_count;
                }
                if(wakeup_sta_count == 0) {
                    vap->iv_wakeup = 0;
                    OS_CANCEL_TIMER(&ieee80211_wifiposdata_g->wifipos_wakeup_timer);
                } else {
                    psq = IEEE80211_NODE_SAVEQ_MGMTQ(ni);
                    ni->ni_flags |= IEEE80211_NODE_WAKEUP;

                    if(!IEEE80211_NODE_SAVEQ_FULL(psq))
                    {
                        if (ni->ni_flags & IEEE80211_NODE_UAPSD) {
                            for(i = WME_NUM_AC-1; i >= 0; i--) {
                                if (ni->ni_uapsd_ac_trigena[i]) {
                                    ac = i;
                                    break;
                                }
                            }
                            ieee80211_send_qosnulldata_keepalive(ni, ac, 0, true);
                        }
                        else
                            ieee80211_send_nulldata_keepalive(ni, 0, true);
                    }
                }
            }
            OS_SET_TIMER(&ieee80211_wifiposdata_g->wifipos_wakeup_timer, ieee80211_wifipos_wakeup_g[index].wakeup_interval); 
        }
        spin_unlock(&wakeup_lock);
    }
}

static OS_TIMER_FUNC(ieee80211_wifipos_timer)
{
    struct ieee80211vap *vap;
    struct ath_softc_net80211 *scn;
    vap = TAILQ_FIRST(&ieee80211_wifiposdata_g->ic->ic_vaps);
    if (vap == NULL)
    {
        WIFIPOS_DPRINTK("vap is null\n");
        return;
    }
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:CTS timeout, changing to old channel: %d current channel: %d",
            ieee80211_wifiposdata_g->ic->ic_prevchan->ic_ieee, ieee80211_wifiposdata_g->ic->ic_curchan->ic_ieee);

    scn = ATH_SOFTC_NET80211(vap->iv_ic);
    wlan_set_channel(vap, ieee80211_wifiposdata_g->ic->ic_prevchan->ic_ieee); 
    spin_lock(&ieee80211_wifiposdata_g->lock);
    while (wifipos_tx_index != wifipos_rx_index) 
    {
        if (wifipos_rx_index > wifipos_tx_index) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Remove toa:%x rx_index:%d", ieee80211_wifipos_table_g[wifipos_rx_index].tod,
                    wifipos_rx_index);
            ieee80211_wifipos_table_g[wifipos_rx_index].toa = 0;
            wifipos_rx_index--;
        }
        else {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_WIFIPOS, "WifiPositioning:Remove tod:%x tx_index:%d", ieee80211_wifipos_table_g[wifipos_tx_index].tod,
                    wifipos_tx_index);
            ieee80211_wifipos_table_g[wifipos_tx_index].tod = 0;
            wifipos_tx_index--;
        }
    }
    spin_unlock(&ieee80211_wifiposdata_g->lock);
}

int ieee80211_wifipos_vattach(struct ieee80211vap *vap)
{
    int index;
    struct ieee80211com *ic = vap->iv_ic;

    if (ieee80211_wifiposdata_g == NULL) {
        ieee80211_wifiposdata_g = (struct ieee80211_wifipos *) (OS_MALLOC(ic->ic_osdev, sizeof(struct ieee80211_wifipos), GFP_KERNEL));
        if (!ieee80211_wifiposdata_g) {
            WIFIPOS_DPRINTK("Unable to allocate memory\n");
            return -ENOMEM;
        }
        ieee80211_wifiposdata_g->reqdata = (ieee80211_wifipos_reqdata_t *)OS_MALLOC(ic->ic_osdev, 
                sizeof(ieee80211_wifipos_reqdata_t), GFP_KERNEL);
        if (!ieee80211_wifiposdata_g->reqdata) {
            WIFIPOS_DPRINTK("Unable to allocate memory\n");
            return -ENOMEM;
        }
        ieee80211_wifiposdata_g->ic = ic;
        spin_lock_init(&ieee80211_wifiposdata_g->lock);
        spin_lock_init(&wakeup_lock);
        if (ieee80211_wifiposdata_g->wifipos_sock == NULL) {
            ieee80211_wifiposdata_g->wifipos_sock = OS_NETLINK_CREATE(NETLINK_WIFIPOS, 
                    1, (void *)ieee80211_wifpos_transmit_probe, THIS_MODULE);
            WIFIPOS_DPRINTK("Netlink socket created for wifipos:%p\n", ieee80211_wifiposdata_g->wifipos_sock);
            if (ieee80211_wifiposdata_g->wifipos_sock == NULL) {
                ieee80211_wifiposdata_g->wifipos_sock = OS_NETLINK_CREATE(NETLINK_WIFIPOS+1, 
                        1, (void *)&ieee80211_wifpos_transmit_probe, THIS_MODULE);
                if (ieee80211_wifiposdata_g->wifipos_sock == NULL) {
                    WIFIPOS_DPRINTK("netlink_kernel_create failed for wifipos\n");
                    return -ENODEV;
                }
            }
        }
        WIFIPOS_DPRINTK("Registered WIFIPOS netlink family");
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
        INIT_WORK(&ieee80211_wifiposdata_g->work, ieee80211_wifipos_wqhandler, NULL);
        INIT_WORK(&ieee80211_wifiposdata_g->tsf_work, ieee80211_wifipos_wqhandler_tsf, NULL);
#else
        INIT_WORK(&ieee80211_wifiposdata_g->work, ieee80211_wifipos_wqhandler);
        INIT_WORK(&ieee80211_wifiposdata_g->tsf_work, ieee80211_wifipos_wqhandler_tsf);
#endif
        OS_INIT_TIMER(ic->ic_osdev, &ieee80211_wifiposdata_g->wifipos_timer, ieee80211_wifipos_timer, NULL);
        OS_INIT_TIMER(ic->ic_osdev, &ieee80211_wifiposdata_g->wifipos_wakeup_timer, ieee80211_wifipos_wakeup_timer, NULL);
        OS_INIT_TIMER(ic->ic_osdev, &ieee80211_wifiposdata_g->wifipos_ps_timer, ieee80211_wifipos_ps_timer, NULL);

        for(index=0; index < ATH_WIFIPOS_MAX_CONCURRENT_WAKEUP_STA; index++) {
            memset(&ieee80211_wifipos_wakeup_g[index].sta_mac_addr, 0, IEEE80211_ADDR_LEN);
            ieee80211_wifipos_wakeup_g[index].timestamp = 0; 
        }
        for(index=0; index < ATH_WIFIPOS_MAX_CONCURRENT_TSF_AP; index++) {
            memset(&ieee80211_wifipos_tsfreq_g[index].assoc_ap_mac_addr, 0, IEEE80211_ADDR_LEN);
            ieee80211_wifipos_tsfreq_g[index].probe_ap_tsf = 0;
            ieee80211_wifipos_tsfreq_g[index].assoc_ap_tsf = 0;
            ieee80211_wifipos_tsfreq_g[index].diff_tsf = 0;
            ieee80211_wifipos_tsfreq_g[index].valid = 0;
        }
    }

    for(index=0; index < ATH_WIFIPOS_MAX_CONCURRENT_PROBES; index++) {
        ieee80211_wifipos_table_g[wifipos_tx_index].tod = 0;
        ieee80211_wifipos_table_g[wifipos_tx_index].toa = 0;
        ieee80211_wifipos_table_g[wifipos_tx_index].valid = FALSE;
    }

    return 0;
}

int ieee80211_wifipos_vdetach(struct ieee80211vap *vap)
{
    if (ieee80211_wifiposdata_g ) {
        if (ieee80211_wifiposdata_g->wifipos_sock) {
            WIFIPOS_DPRINTK("Release the socket");
            OS_SOCKET_RELEASE(ieee80211_wifiposdata_g->wifipos_sock);
            ieee80211_wifiposdata_g->wifipos_sock = NULL;
        }
        if (ieee80211_wifiposdata_g->reqdata)
            OS_FREE(ieee80211_wifiposdata_g->reqdata);
        OS_FREE_TIMER(&ieee80211_wifiposdata_g->wifipos_timer);
        OS_FREE_TIMER(&ieee80211_wifiposdata_g->wifipos_ps_timer);
        OS_FREE(ieee80211_wifiposdata_g);
        ieee80211_wifiposdata_g = NULL;
        WIFIPOS_DPRINTK("UnRegistered WIFI positioning netlink family");
    }
    return 0;
}

#endif /* ATH_SUPPORT_WIFIPOS */
