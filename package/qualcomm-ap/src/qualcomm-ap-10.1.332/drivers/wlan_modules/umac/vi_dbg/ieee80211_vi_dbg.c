/*
 *  Copyright (c) 2010 Atheros Communications Inc.
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
/********************************************************************/
/* \file ieee80211_vi_dbg.c
 * \brief Video debug tool implementation
 *
 * This is a framework for the  implementation of a debug  tool that can be 
 * used to  debug  video features with any third party tool.  The framework
 * lets the user define a set of markers  used to identify the  packets  of
 * interest. Upto 4 streams are supported and upto 4 markers can be defined
 * for each of the streams.  Each of the markers can be defined in terms of
 * an offset from the start of the wbuf, the marker field size (max 4 bytes)
 * & the marker pattern match value.
 * The user can also define the packet sequence number field for the packets
 * of interest which is spefied in terms of an offset from the start of the
 * wbuf & the field size (max 4 bytes).  This is used to track the received 
 * packet sequence numbers for sequence jumps that indicate packet loss. Pkt
 * loss information is logged into pktlog.
 */

#include <ieee80211_vi_dbg_priv.h>
#include "ieee80211_vi_dbg.h"

#if UMAC_SUPPORT_VI_DBG
u_int32_t wifi_rx_drop = 0;

/* Function that acts as an interface to packet log. Used to log packet losss information */
void 
ieee80211_vi_dbg_pktlog(struct ieee80211com *ic, const char *fmt, ...) 
{
     char                   tmp_buf[OS_TEMP_BUF_SIZE];  
     va_list                ap;             
     va_start(ap, fmt);                                         
     vsnprintf (tmp_buf, OS_TEMP_BUF_SIZE, fmt, ap);            
     va_end(ap);                                                                                  
     ic->ic_log_text(ic, tmp_buf);             
}

/* Function to display marker information */
void
ieee80211_vi_dbg_print_marker(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_vi_dbg *vi_dbg = ic->ic_vi_dbg;
    uint8_t i,j;
    
    if (vi_dbg == NULL) {
        return;
    }	
    for (i = 0; i < ic->ic_vi_dbg_params->vi_num_streams; i++) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "Stream%d\n", i+1);
	    for (j = 0; j < ic->ic_vi_dbg_params->vi_num_markers; j++) {
	       IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "    marker%d: Offset %#04x Size %#04x Match %#08x\n",  j+1,vi_dbg->markers[i][j].offset, vi_dbg->markers[i][j].num_bytes, vi_dbg->markers[i][j].match);
	    }
    }
}

/* This function implements the set parameter functionality for all the video debug iwpriv 
 * parameters  
 */
void 
ieee80211_vi_dbg_set_param(wlan_if_t vaphandle, ieee80211_param param, u_int32_t val)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    switch (param) {
	case IEEE80211_VI_DBG_CFG:
	    ic->ic_vi_dbg_params->vi_dbg_cfg = val;
	    if (val & EN_LOG_STAT) { 
	        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s :Enabling logging of debug stats\n",__func__);
	        ic->ic_set_vi_dbg_log(ic, 1);
	    }else {
	        ic->ic_set_vi_dbg_log(ic, 0);
        }

	    break;
	case IEEE80211_VI_DBG_NUM_STREAMS:
        ic->ic_vi_dbg_params->vi_num_streams = (u_int8_t) val;
        break;
	case IEEE80211_VI_STREAM_NUM:
        ic->ic_vi_dbg_params->vi_stream_num  = (u_int8_t) val;
        break;
    case IEEE80211_VI_DBG_NUM_MARKERS:
        ic->ic_vi_dbg_params->vi_num_markers = (u_int8_t) val;
        break;
    case IEEE80211_VI_MARKER_NUM:
        ic->ic_vi_dbg_params->vi_marker_num  = (u_int8_t) val;
        break;
    case IEEE80211_VI_MARKER_OFFSET_SIZE:
        ic->ic_vi_dbg_params->vi_marker_offset_size = val;
        break;
    case IEEE80211_VI_MARKER_MATCH:
        ic->ic_vi_dbg_params->vi_marker_match = val;
        break;
    case IEEE80211_VI_RXSEQ_OFFSET_SIZE:
        ic->ic_vi_dbg_params->vi_rxseq_offset_size = val;
	    ic->ic_vi_dbg->rxseq_num_bytes = val & 0x0000FFFF;
	    ic->ic_vi_dbg->rxseq_offset = val >> 16;
  	    IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "RX Seq Offset = %d, Num Bytes = %d\n", 
  	                      ic->ic_vi_dbg->rxseq_offset, ic->ic_vi_dbg->rxseq_num_bytes);
        break;
	case IEEE80211_VI_RX_SEQ_RSHIFT:
        ic->ic_vi_dbg_params->vi_rx_seq_rshift = val;
        break;
	case IEEE80211_VI_RX_SEQ_MAX:
        ic->ic_vi_dbg_params->vi_rx_seq_max = val;
        break;
	case IEEE80211_VI_RX_SEQ_DROP:
        ic->ic_vi_dbg_params->vi_rx_seq_drop = val;
        break;
    case IEEE80211_VI_TIME_OFFSET_SIZE:
        ic->ic_vi_dbg_params->vi_time_offset_size = val;
	    ic->ic_vi_dbg->time_num_bytes = val & 0x0000FFFF;
        ic->ic_vi_dbg->time_offset = val >> 16;			
        break;
    case IEEE80211_VI_RESTART:
        ic->ic_vi_dbg_params->vi_dbg_restart = val;
  	    ic->ic_set_vi_dbg_restart(ic);
 	    ic->ic_vi_dbg_params->vi_rx_seq_drop = 0;
        wifi_rx_drop = 0;
        break;
    case IEEE80211_VI_RXDROP_STATUS:
	    IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s :Not Impelented\n",__func__);
        break;
    default:
        break;
    }
}

/* This function implements the get parameter functionality for all the video debug iwpriv 
 * parameters 
 */

u_int32_t
ieee80211_vi_dbg_get_param(wlan_if_t vaphandle, ieee80211_param param)
{
    struct ieee80211vap *vap = vaphandle;
    struct ieee80211com *ic = vap->iv_ic;
    u_int32_t val = 0;

    switch (param) {
    case IEEE80211_VI_DBG_CFG:
	    val = ic->ic_vi_dbg_params->vi_dbg_cfg;
	    if (val & EN_MRK_PRNT) {
	        ieee80211_vi_dbg_print_marker(vap);
	    }
	    break;
    case IEEE80211_VI_DBG_NUM_STREAMS:
        val = (u_int32_t) ic->ic_vi_dbg_params->vi_num_streams;
        break;
	case IEEE80211_VI_STREAM_NUM:
        val = (u_int32_t) ic->ic_vi_dbg_params->vi_stream_num;
        break;
    case IEEE80211_VI_DBG_NUM_MARKERS:
        val = (u_int32_t) ic->ic_vi_dbg_params->vi_num_markers;
        break;
    case IEEE80211_VI_MARKER_NUM:
        val = ic->ic_vi_dbg_params->vi_marker_num;
        break;
    case IEEE80211_VI_MARKER_OFFSET_SIZE:
        val = ic->ic_vi_dbg_params->vi_marker_offset_size;
        break;
    case IEEE80211_VI_MARKER_MATCH:
        val = ic->ic_vi_dbg_params->vi_marker_match;
        break;
    case IEEE80211_VI_RXSEQ_OFFSET_SIZE:
        val = ic->ic_vi_dbg_params->vi_rxseq_offset_size;
        break;
	case IEEE80211_VI_RX_SEQ_RSHIFT:
        val = ic->ic_vi_dbg_params->vi_rx_seq_rshift;
	    break;
	case IEEE80211_VI_RX_SEQ_MAX:
        val = ic->ic_vi_dbg_params->vi_rx_seq_max;
        break;
	case IEEE80211_VI_RX_SEQ_DROP:
        val = ic->ic_vi_dbg_params->vi_rx_seq_drop;
        break;        
    case IEEE80211_VI_TIME_OFFSET_SIZE:
        val = ic->ic_vi_dbg_params->vi_time_offset_size;
        break;
    case IEEE80211_VI_RESTART:
        val = ic->ic_vi_dbg_params->vi_dbg_restart;
        break;
    case IEEE80211_VI_RXDROP_STATUS:
        val = wifi_rx_drop;
        break;
    default:
        break;
    }
    return val;
}


/* The function is used set up the markers for pkt filtering via iwpriv. It sets the marker_info
 * fields in the ieee80211_vi_dbg stucture 
 */
void 
ieee80211_vi_dbg_get_marker(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_vi_dbg *vi_dbg = ic->ic_vi_dbg;
    u_int16_t offset;
    u_int16_t num_bytes;
    u_int32_t match;
    u_int32_t marker_num;
    u_int32_t stream_num;
	
    if (vi_dbg == NULL) {
        return;
    }
    	
    offset     = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_MARKER_OFFSET_SIZE) >> 16;
    num_bytes  = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_MARKER_OFFSET_SIZE) & 0x0000FFFF;
    match      = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_MARKER_MATCH);
    marker_num = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_MARKER_NUM);
    stream_num = ieee80211_vi_dbg_get_param(vap, IEEE80211_VI_STREAM_NUM);
    
    if (stream_num < 1 || stream_num > VI_MAX_NUM_STREAMS || marker_num < 1 || marker_num > VI_MAX_NUM_MARKERS) {
         IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "%s : Invalid number of streams or markers set. Skipping marker assignment\n",__func__);
         return;
    }
	
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "Setting Stream%d Marker%d Offset: %d Num Bytes: %d Matching pattern: %d\n",
                                               stream_num, marker_num, offset, num_bytes, match);
    vi_dbg->markers[stream_num-1][marker_num-1].offset    = offset;
    vi_dbg->markers[stream_num-1][marker_num-1].num_bytes = num_bytes;
    vi_dbg->markers[stream_num-1][marker_num-1].match     = match; 										 
}

/* This function is used to filter the received wbuf based on the defined markers */ 
int32_t
ieee80211_vi_dbg_check_markers(struct ieee80211vap *vap, wbuf_t mywbuf)
{
    struct ieee80211com *ic = vap->iv_ic;
    u_int8_t *payload;
    u_int8_t match;
    u_int8_t i, j, s; 
    u_int32_t mfail;

    payload = (u_int8_t *)wbuf_raw_data(mywbuf);
	
    for (s = 0; s < ic->ic_vi_dbg_params->vi_num_streams; s++) {
	    mfail = 0;  /* Reset marker failures for new stream. mfail is used to reduce checks if maker checks
	                 * fail for any stream. If a filure is detected immediately move on to the markers for
	  	             * the next stream */ 
	    for (i = 0; i < ic->ic_vi_dbg_params->vi_num_markers; i++) {
	        for (j = 0; j < ic->ic_vi_dbg->markers[s][i].num_bytes; j++) {
	            match = ((ic->ic_vi_dbg->markers[s][i].match) >> (8*(ic->ic_vi_dbg->markers[s][i].num_bytes-1-j))) & 0x000000FF;
		        if (payload[ic->ic_vi_dbg->markers[s][i].offset + j] != match) {
		            mfail = 1; /* Marker failure for this stream. Move to next stream */ 
		            break;
		        }
	        }
	        if (mfail == 1) {
		        break;        /* Marker failure for this stream. Skip other markers for this 
		                         stream & move to next stream */
	        }
	    }
	    
	    if (!mfail) {
		   return s;
	    }
    }
    return -1;
}

/* This function is used to collect stats from the received wbuf after the marker
 * filtering has been completed 
 */
void
ieee80211_vi_dbg_stats(struct ieee80211vap *vap, wbuf_t mywbuf, u_int32_t stream_num)
{
    struct ieee80211com *ic = vap->iv_ic;
    u_int8_t *payload;
    u_int32_t i, rx_seq_num = 0;
    int32_t   diff;
    static u_int32_t prev_rx_seq_num[VI_MAX_NUM_STREAMS] = {0, 0, 0, 0}; 
    static u_int32_t current_seq_num[VI_MAX_NUM_STREAMS]   = {0, 0, 0, 0}; 
    static u_int8_t start[VI_MAX_NUM_STREAMS]           = {0, 0, 0, 0};
    u_int16_t offset, num_bytes;
	
    if (!(ic->ic_vi_dbg_params->vi_dbg_cfg & EN_MRK_FILT))
	    return;
	
	/* If restart is set, re-initialize internal variables & reset the restart parameter */
    if (ic->ic_vi_dbg_params->vi_dbg_restart) 	{
	    for (i = 0; i < VI_MAX_NUM_STREAMS; i++) {
	        start[i]           = 0;
	        prev_rx_seq_num[i] = 0;
	        current_seq_num[i]   = 0;
            wifi_rx_drop = 0;            
	    }	
	    ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_RESTART, 0);
    }

    payload   = (u_int8_t *)wbuf_raw_data(mywbuf);
    offset    = ic->ic_vi_dbg->rxseq_offset;
    num_bytes = ic->ic_vi_dbg->rxseq_num_bytes;
	
    for (i = 0; i < num_bytes; i++) {
	    rx_seq_num += (payload[offset + i] << (8 * (3 - i)));
    }
    rx_seq_num = rx_seq_num >> ic->ic_vi_dbg_params->vi_rx_seq_rshift;

   if (start[stream_num] == 0)
   {    // check for 31st bit Set; 31st bit on means iperf sending 1st packet
	   IEEE80211_DPRINTF(vap, IEEE80211_MSG_ANY, "stream :%d  start seq no : 0x%08x \n",stream_num+1,rx_seq_num);
        if (rx_seq_num & 0x80000000)
            rx_seq_num &=0x7fffffff;

   }
   current_seq_num[stream_num] = rx_seq_num;
   diff = current_seq_num[stream_num] - prev_rx_seq_num[stream_num];
    if ((diff < 0) && start[stream_num] == 1)
    {  // wrap case & Loss in Wrap
       diff = ic->ic_vi_dbg_params->vi_rx_seq_max-prev_rx_seq_num[stream_num]+current_seq_num[stream_num];
       // not handling out of order case
    }


  if ((diff > 1) && start[stream_num] == 1)
  {
        wifi_rx_drop += diff;        
        ieee80211_vi_dbg_pktlog(ic, "wifi: Stream %d, Diff %d Rx pkt loss from %d  to %d\n", stream_num + 1,
		        	                diff,prev_rx_seq_num[stream_num],current_seq_num[stream_num]);
	    ieee80211_vi_dbg_set_param(vap, IEEE80211_VI_RX_SEQ_DROP, 1);
  }

    if (diff == 0 && start[stream_num] == 1) {
        ieee80211_vi_dbg_pktlog(ic, "Rx duplicate pkt %d \n", rx_seq_num); 
    }
    prev_rx_seq_num[stream_num] = current_seq_num[stream_num];
    start[stream_num] = 1;
}

/* This function is used to check the received wbufs before they are handed over to the 
 * netif_rx  function.  The  wbuf is checked for the signature  indicated by the  debug
 * markers that have been already set. 
 */
void
ieee80211_vi_dbg_input(struct ieee80211vap *vap, wbuf_t wbuf)
{
    int stream_num;
    struct ieee80211com *ic = vap->iv_ic;
	
    if (!(ic->ic_vi_dbg_params->vi_dbg_cfg & EN_MRK_FILT)) {
	    return;
	}    
	
    stream_num = ieee80211_vi_dbg_check_markers(vap, wbuf);
	
    if (stream_num != -1) {
	    ieee80211_vi_dbg_stats(vap, wbuf, stream_num);	
    }
}

/* This function is used to attach the video debug structures & initialization */
int 
ieee80211_vi_dbg_attach(struct ieee80211com *ic)
{   
    if (!ic) {
        return -EFAULT;
    }     

    ic->ic_vi_dbg_params = ( struct ieee80211_vi_dbg_params *) 
                           OS_MALLOC(ic->ic_osdev, sizeof(struct ieee80211_vi_dbg_params), GFP_KERNEL); 
    
    if (!ic->ic_vi_dbg_params) {
        printk("%s : Memory not allocated for ic->ic_vi_dbg_params\n",__func__); /* Not expected */
        return -ENOMEM;
    }

    OS_MEMSET(ic->ic_vi_dbg_params, 0, sizeof(struct ieee80211_vi_dbg_params));
	
    ic->ic_vi_dbg_params->vi_num_streams = 1;
    ic->ic_vi_dbg_params->vi_stream_num  = 1;

    ic->ic_vi_dbg = (struct ieee80211_vi_dbg *) 
                    OS_MALLOC(ic->ic_osdev, sizeof(struct ieee80211_vi_dbg), GFP_KERNEL);

    if (!ic->ic_vi_dbg) {
        printk(" %s : Memory not allocated for ic->ic_vi_dbg\n",__func__); /* Not expected */
        OS_FREE(ic->ic_vi_dbg_params);
        ic->ic_vi_dbg_params = NULL;
        return -ENOMEM;
    }
    return 0;
}

/* This function is used to detach the video debug structures & initialization */
int
ieee80211_vi_dbg_detach(struct ieee80211com *ic)
{
    int err = 0;

    if (!ic) {
        return -EFAULT;
    }    

    if (!ic->ic_vi_dbg) {
        printk("%s :Memory not allocated for ic->ic_vi_dbg\n",__func__); /* Not Expected */
        err = -ENOMEM;
        goto bad;
    } else {
        OS_FREE(ic->ic_vi_dbg);
        ic->ic_vi_dbg = NULL;  
    }
    if (!ic->ic_vi_dbg_params) {
        printk("%s :Memory not allocated for ic->ic_vi_dbg_params\n",__func__); /* Not expected */
        err = -ENOMEM;
        goto bad;
    } else {
        OS_FREE(ic->ic_vi_dbg_params); 
        ic->ic_vi_dbg_params = NULL;
    }
bad:
   return err;
}
#endif
