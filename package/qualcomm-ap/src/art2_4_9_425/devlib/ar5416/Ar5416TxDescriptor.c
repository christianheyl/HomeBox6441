



#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "wlantype.h"
#include "rate_constants.h"

#include "smatch.h"

#include "TxDescriptor.h"
#include "Ar5416TxDescriptor.h"

#define MBUFFER 1024


//
// ar9223 chip. merlin.
//
// words 2-13 are control, 14-23 are status
//

#ifdef ARCH_BIG_ENDIAN
struct Ar5416TxDescriptor
{
	unsigned long link_ptr;				// word 0

	unsigned long buf_ptr;				// word 1

	// control words 2-13

	unsigned long cts_enable:1;			// word 2 
	unsigned long dest_index_valid:1;
	unsigned long int_req:1;
	unsigned long res2_1:4;
	unsigned long clear_dest_mask:1;
	unsigned long veol:1;
	unsigned long rts_enable:1;
	unsigned long tpc_0:6;
	unsigned long clear_retry:1;
	unsigned long low_rx_chain:1;
	unsigned long res2:1;
	unsigned long vmf:1;
	unsigned long frame_length:12;		


	unsigned long more_rifs:1;			// word 3
	unsigned long is_agg:1;
	unsigned long more_agg:1;
    unsigned long ext_and_ctl:1;
    unsigned long ext_only:1;
    unsigned long corrupt_fcs:1;
	unsigned long insert_timestamp:1;
	unsigned long no_ack:1;
	unsigned long frame_type:4;
	unsigned long dest_index:7;
	unsigned long more:1;
	unsigned long buf_len:12;			


	unsigned long tx_tries3:4;			// word 4
	unsigned long tx_tries2:4;
	unsigned long tx_tries1:4;
	unsigned long tx_tries0:4;
	unsigned long dur_update_en:1;
	unsigned long burst_duration:15;	

	unsigned long tx_rate2:8;
	unsigned long tx_rate1:8;
	unsigned long tx_rate3:8;			// word 5
	unsigned long tx_rate0:8;			

	unsigned long rts_cts_qual1:1;		// word 6
	unsigned long packet_duration1:15;
	unsigned long rts_cts_qual0:1;
	unsigned long packet_duration0:15;	

	unsigned long rts_cts_qual3:1;		// word 7
	unsigned long packet_duration3:15;
	unsigned long rts_cts_qual2:1;
	unsigned long packet_duration2:15;	

	unsigned long res8_1:3;				// word 8
	unsigned long encrypt_type:3;
	unsigned long pad_delim:8;
	unsigned long res8:2;
	unsigned long agg_length:16;		


	unsigned long stbc:4;				// word 9
	unsigned long rts_cts_rate:8;
	unsigned long chain_sel_3:3;
	unsigned long gi_3:1;
	unsigned long h20_40_3:1;
	unsigned long chain_sel_2:3;
	unsigned long gi_2:1;
	unsigned long h20_40_2:1;
	unsigned long chain_sel_1:3;
	unsigned long gi_1:1;
	unsigned long h20_40_1:1;
	unsigned long chain_sel_0:3;
	unsigned long gi_0:1;
	unsigned long h20_40_0:1;			

	unsigned long res2_10:8;			// word 10
	unsigned long antenna0:24;			

	unsigned long res2_11:8;			// word 11
	unsigned long antenna1:24;			

	unsigned long res2_12:8;			// word 12
	unsigned long antenna2:24;			

	unsigned long res2_13:8;			// word 13
	unsigned long antenna3:24;			

	// status words 14-23

	unsigned long res2_14_2:1;			// word 14
	unsigned long ba_status:1;
	unsigned long res2_14_1:6;
	unsigned long res2_14:8;
	unsigned long rssi_ant01:8;
	unsigned long rssi_ant00:8;			


	unsigned long res2_15:12;			// word 15
	unsigned long tx_timer_expired:1;
	unsigned long desc_config_error:1;
	unsigned long tx_data_underrun_err:1;
	unsigned long tx_dlimitr_underrun_err:1;
	unsigned long virtual_retry_cnt:4;
	unsigned long data_fail_cnt:4;
	unsigned long rts_fail_cnt:4;
	unsigned long filtered:1;
	unsigned long fifo_underrun:1;
	unsigned long excessive_retries:1;
	unsigned long frm_xmit_ok:1;		

	unsigned long send_timestamp;		// word 16

	unsigned long ba_bitmap_0_31;		// word 17

	unsigned long ba_bitmap_32_63;		// word 18

	unsigned long ack_rssi_combined:8;	// word 19
	unsigned long res2_19:8;
	unsigned long rssi_ant11:8;
	unsigned long rssi_ant10:8;			

	unsigned long evm0;					// word 20

	unsigned long evm1;					// word 21

	unsigned long evm2;					// word 22

	unsigned long tid:4;				// word 23
	unsigned long res23_3:2;
	unsigned long pwr_management:1;
	unsigned long res23_2:2;
	unsigned long final_tx_index:2;
	unsigned long res23_1:3;
	unsigned long txop_exceeded:1;
	unsigned long res23:4;
	unsigned long seqnum:12;
	unsigned long done:1;				
};

#else 
// For little endian architecture
struct Ar5416TxDescriptor
{
	unsigned long link_ptr;				// word 0

	unsigned long buf_ptr;				// word 1

	// control words 2-13

	unsigned long frame_length:12;		// word 2 
	unsigned long vmf:1;
	unsigned long res2:1;
	unsigned long low_rx_chain:1;
	unsigned long clear_retry:1;
	unsigned long tpc_0:6;
	unsigned long rts_enable:1;
	unsigned long veol:1;
	unsigned long clear_dest_mask:1;
	unsigned long res2_1:4;
	unsigned long int_req:1;
	unsigned long dest_index_valid:1;
	unsigned long cts_enable:1;

	unsigned long buf_len:12;			// word 3
	unsigned long more:1;
	unsigned long dest_index:7;
	unsigned long frame_type:4;
	unsigned long no_ack:1;
	unsigned long insert_timestamp:1;
    unsigned long corrupt_fcs:1;
    unsigned long ext_only:1;
    unsigned long ext_and_ctl:1;
	unsigned long more_agg:1;
	unsigned long is_agg:1;
	unsigned long more_rifs:1;

	unsigned long burst_duration:15;	// word 4
	unsigned long dur_update_en:1;
	unsigned long tx_tries0:4;
	unsigned long tx_tries1:4;
	unsigned long tx_tries2:4;
	unsigned long tx_tries3:4;

	unsigned long tx_rate0:8;			// word 5
	unsigned long tx_rate1:8;
	unsigned long tx_rate2:8;
	unsigned long tx_rate3:8;

	unsigned long packet_duration0:15;	// word 6
	unsigned long rts_cts_qual0:1;
	unsigned long packet_duration1:15;
	unsigned long rts_cts_qual1:1;

	unsigned long packet_duration2:15;	// word 7
	unsigned long rts_cts_qual2:1;
	unsigned long packet_duration3:15;
	unsigned long rts_cts_qual3:1;

	unsigned long agg_length:16;		// word 8
	unsigned long res8:2;
	unsigned long pad_delim:8;
	unsigned long encrypt_type:3;
	unsigned long res8_1:3;

	unsigned long h20_40_0:1;			// word 9
	unsigned long gi_0:1;
	unsigned long chain_sel_0:3;
	unsigned long h20_40_1:1;
	unsigned long gi_1:1;
	unsigned long chain_sel_1:3;
	unsigned long h20_40_2:1;
	unsigned long gi_2:1;
	unsigned long chain_sel_2:3;
	unsigned long h20_40_3:1;
	unsigned long gi_3:1;
	unsigned long chain_sel_3:3;
	unsigned long rts_cts_rate:8;
	unsigned long stbc:4;

	unsigned long antenna0:24;			// word 10
	unsigned long res2_10:8;

	unsigned long antenna1:24;			// word 11
	unsigned long res2_11:8;

	unsigned long antenna2:24;			// word 12
	unsigned long res2_12:8;

	unsigned long antenna3:24;			// word 13
	unsigned long res2_13:8;

	// status words 14-23

	unsigned long rssi_ant00:8;			// word 14
	unsigned long rssi_ant01:8;
	unsigned long res2_14:8;
	unsigned long res2_14_1:6;
	unsigned long ba_status:1;
	unsigned long res2_14_2:1;

	unsigned long frm_xmit_ok:1;		// word 15
	unsigned long excessive_retries:1;
	unsigned long fifo_underrun:1;
	unsigned long filtered:1;
	unsigned long rts_fail_cnt:4;
	unsigned long data_fail_cnt:4;
	unsigned long virtual_retry_cnt:4;
	unsigned long tx_dlimitr_underrun_err:1;
	unsigned long tx_data_underrun_err:1;
	unsigned long desc_config_error:1;
	unsigned long tx_timer_expired:1;
	unsigned long res2_15:12;

	unsigned long send_timestamp;		// word 16

	unsigned long ba_bitmap_0_31;		// word 17

	unsigned long ba_bitmap_32_63;		// word 18

	unsigned long rssi_ant10:8;			// word 19
	unsigned long rssi_ant11:8;
	unsigned long res2_19:8;
	unsigned long ack_rssi_combined:8;

	unsigned long evm0;					// word 20

	unsigned long evm1;					// word 21

	unsigned long evm2;					// word 22

	unsigned long done:1;				// word 23
	unsigned long seqnum:12;
	unsigned long res23:4;
	unsigned long txop_exceeded:1;
	unsigned long res23_1:3;
	unsigned long final_tx_index:2;
	unsigned long res23_2:2;
	unsigned long pwr_management:1;
	unsigned long res23_3:2;
	unsigned long tid:4;
};
#endif

//
// print all of the fields of the descriptor
//
static int Ar5416TxDescriptorPrint(void *block, char *buffer, int max)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;
	int lc, nc;

	lc=SformatOutput(buffer,max-1,"`%x`", dr->link_ptr);

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x`", dr->buf_ptr);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x %x %x %x %x %x %x %x`",
	    dr->cts_enable,
	    dr->dest_index_valid,
	    dr->int_req,
	    dr->res2_1,
	    dr->clear_dest_mask,
	    dr->veol,
	    dr->rts_enable,
	    dr->tpc_0,
	    dr->clear_retry,
	    dr->low_rx_chain,
	    dr->res2,
	    dr->vmf,
		dr->frame_length
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x %x %x %x %x %x %x`", 	
	    dr->more_rifs,
	    dr->is_agg,
	    dr->more_agg,
        dr->ext_and_ctl,
        dr->ext_only,
        dr->corrupt_fcs,
	    dr->insert_timestamp,
 	    dr->no_ack,
	    dr->frame_type,
	    dr->dest_index,
	    dr->more,
		dr->buf_len
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x`", 	
	    dr->tx_tries3,
	    dr->tx_tries2,
	    dr->tx_tries1,
	    dr->tx_tries0,
	    dr->dur_update_en,
		dr->burst_duration
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x`", 	
	    dr->tx_rate3,
	    dr->tx_rate2,
	    dr->tx_rate1,
	    dr->tx_rate0);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x`", 	
	    dr->rts_cts_qual1,
	    dr->packet_duration1,
	    dr->rts_cts_qual0,
	    dr->packet_duration0
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x`", 	
	    dr->rts_cts_qual3,
	    dr->packet_duration3,
	    dr->rts_cts_qual2,
	    dr->packet_duration2
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x`", 	
	    dr->res8_1,
	    dr->encrypt_type,
	    dr->pad_delim,
	    dr->res8,
	    dr->agg_length
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x %x %x %x %x %x %x %x %x`", 	
	    dr->stbc,
	    dr->rts_cts_rate,
	    dr->chain_sel_3,
	    dr->gi_3,
	    dr->h20_40_3,
	    dr->chain_sel_2,
	    dr->gi_2,
	    dr->h20_40_2,
	    dr->chain_sel_1,
	    dr->gi_1,
	    dr->h20_40_1,
	    dr->chain_sel_0,
	    dr->gi_0,
		dr->h20_40_0
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x`", 	
	    dr->res2_10,
	    dr->antenna0
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x`", 	
	    dr->res2_11,
	    dr->antenna1
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x`", 	
	    dr->res2_12,
	    dr->antenna2
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x`", 	
	    dr->res2_13,
	    dr->antenna3
		);
	if(nc>0)
	{
		lc+=nc;
	}

	// status words 14-23

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x`", 	
	    dr->res2_14_2,
	    dr->ba_status,
	    dr->res2_14_1,
	    dr->res2_14,
	    dr->rssi_ant01,
		dr->rssi_ant00
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x %x %x %x %x %x %x`", 	
	    dr->res2_15,
	    dr->tx_timer_expired,
	    dr->desc_config_error,
	    dr->tx_data_underrun_err,
	    dr->tx_dlimitr_underrun_err,
	    dr->virtual_retry_cnt,
	    dr->data_fail_cnt,
	    dr->rts_fail_cnt,
	    dr->filtered,
	    dr->fifo_underrun,
	    dr->excessive_retries,
		dr->frm_xmit_ok
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x`", 	
	    dr->send_timestamp);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x`", 	
	    dr->ba_bitmap_0_31);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x`", 	
	    dr->ba_bitmap_32_63);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x`", 	
	    dr->ack_rssi_combined,
	    dr->res2_19,
	    dr->rssi_ant11,
	    dr->rssi_ant10
		);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x`", 	
	    dr->evm0);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x`", 	
	    dr->evm1);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x`", 	
	    dr->evm2);
	if(nc>0)
	{
		lc+=nc;
	}

	nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x %x %x %x %x`", 	
	    dr->tid,
	    dr->res23_3,
	    dr->pwr_management,
	    dr->res23_2,
	    dr->final_tx_index,
	    dr->res23_1,
	    dr->txop_exceeded,
	    dr->res23,
	    dr->seqnum,
		dr->done
		);
	if(nc>0)
	{
		lc+=nc;
	}

	return lc;
}

static int Ar5416TxDescriptorLinkPtrSet(void *block, unsigned int ptr)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		dr->link_ptr=ptr;
		return 0;
	}
	return -1;
}

static unsigned int Ar5416TxDescriptorLinkPtr(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->link_ptr;
	}
	return 0;
}

static unsigned int Ar5416TxDescriptorBufPtr(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->buf_ptr;
	}
	return 0;
}

static unsigned int Ar5416TxDescriptorBufLen(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->buf_len;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorIntReq(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->int_req;
	}
	return 0;
}

static unsigned int Ar5416TxDescriptorFrameLength(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->frame_length;
	}
	return 0;
}

#ifdef UNUSED
static unsigned char Ar5416TxDescriptorVmf(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->vmf;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorLowRxChain(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->low_rx_chain;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorClearRetry(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->clear_retry;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorTpc0(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tpc_0;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorRtsEnable(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rts_enable;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorVeol(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->veol;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorClearDestMask(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->clear_dest_mask;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorDestIndexValid(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->dest_index_valid;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorCtsEnable(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->cts_enable;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorMore(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->more;
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorDestIndex(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->dest_index;
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorFrameType(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->frame_type;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorNoAck(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->no_ack;
	}
	return 0;
}
#endif

static unsigned char Ar5416TxDescriptorMoreAgg(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->more_agg;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorAggregate(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->is_agg;
	}
	return 0;
}

#ifdef UNUSED
static unsigned char Ar5416TxDescriptorMoreRifs(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->more_rifs;
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorBurstDuration(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->burst_duration;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorDurUpdateEn(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->dur_update_en;
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorTxTries0(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tx_tries0;
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorTxTries1(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tx_tries1;
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorTxTries2(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tx_tries2;
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorTxTries3(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tx_tries3;
	}
	return 0;
}
#endif

static int Ar5416TxDescriptorTxRate0Set(void *block, unsigned long rate)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		dr->tx_rate0=rateValues[rate];		// check index range
		dr->h20_40_0=IS_HT40_RATE_INDEX(rate);
		return 0;
	}
	return -1;
}

static unsigned long Ar5416TxDescriptorTxRate0(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
        return descRate2RateIndex(dr->tx_rate0, (unsigned char)dr->h20_40_0);
	}
	return 0;
}

#ifdef UNUSED
static unsigned long Ar5416TxDescriptorTxRate1(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
        return descRate2RateIndex(dr->tx_rate1, dr->h20_40_1);
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorTxRate2(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
        return descRate2RateIndex(dr->tx_rate2, dr->h20_40_2);
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorTxRate3(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
        return descRate2RateIndex(dr->tx_rate3, dr->h20_40_3);
	}
	return 0;
}
#endif

static int Ar5416TxDescriptorTxRateSet(void *block, unsigned int rate)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return Ar5416TxDescriptorTxRate0Set(block,rate);
	}
	return -1;
}

static unsigned int Ar5416TxDescriptorTxRate(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		//
		// figure out which rate it was sent at #######
		//
        return Ar5416TxDescriptorTxRate0(dr);
	}
	return 0;
}

#ifdef UNUSED
static unsigned long Ar5416TxDescriptorPacketDuration0(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->packet_duration0;
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorPacketDuration1(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->packet_duration1;
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorPacketDuration2(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->packet_duration2;
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorPacketDuration3(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->packet_duration3;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorRtsCtsQual0(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rts_cts_qual0;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorRtsCtsQual1(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rts_cts_qual1;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorRtsCtsQual2(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rts_cts_qual2;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorRtsCtsQual3(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rts_cts_qual3;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorGi0(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->gi_0;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorGi1(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->gi_1;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorGi2(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->gi_2;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorGi3(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->gi_3;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorChainSelect0(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->chain_sel_0;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorChainSelect1(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->chain_sel_1;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorChainSelect2(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->chain_sel_2;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorChainSelect3(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->chain_sel_3;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorH20400(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->h20_40_0;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorH20401(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->h20_40_1;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorH20402(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->h20_40_2;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorH20403(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->h20_40_3;
	}
	return 0;
}
#endif

static unsigned int Ar5416TxDescriptorAggLength(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->agg_length;
	}
	return 0;
}

#ifdef UNUSED
static unsigned long Ar5416TxDescriptorPadDelim(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->pad_delim;
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorEncryptType(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->encrypt_type;
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorRtsCtsRate(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
        return descRate2RateIndex(dr->rts_cts_rate, 0);
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorStbc(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->stbc;
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorAntenna0(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->antenna0;
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorAntenna1(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->antenna1;
	}
	return 0;
}

static unsigned long Ar5416TxDescriptorAntenna2(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->antenna2;
	}
	return 0;
}
#endif



// status words 14-23


static unsigned int Ar5416TxDescriptorRssiCombined(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->ack_rssi_combined;
	}
	return 0;
}

static unsigned int Ar5416TxDescriptorRssiAnt00(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_ant00;
	}
	return 0;
}

static unsigned int Ar5416TxDescriptorRssiAnt01(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_ant01;
	}
	return 0;
}

static unsigned int Ar5416TxDescriptorRssiAnt10(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_ant10;
	}
	return 0;
}

static unsigned int Ar5416TxDescriptorRssiAnt11(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rssi_ant11;
	}
	return 0;
}

#ifdef UNUSED
static double Ar5416TxDescriptorEvm0(void *block)
{
	block=0;
	return 0;
}

static double Ar5416TxDescriptorEvm1(void *block)
{
	block=0;
	return 0;
}
#endif

static unsigned char Ar5416TxDescriptorDone(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->done;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorFrameTxOk(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->frm_xmit_ok;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorBaStatus(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->ba_status;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorExcessiveRetries(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->excessive_retries;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorFifoUnderrun(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return (unsigned char)dr->fifo_underrun;
	}
	return 0;
}

#ifdef UNUSED
static unsigned char Ar5416TxDescriptorFiltered(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->filtered;
	}
	return 0;
}
#endif

static unsigned int Ar5416TxDescriptorRtsFailCount(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rts_fail_cnt;
	}
	return 0;
}

static unsigned int Ar5416TxDescriptorDataFailCount(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->data_fail_cnt;
	}
	return 0;
}

#ifdef UNUSED
static unsigned long Ar5416TxDescriptorVirtualRetryCount(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->virtual_retry_cnt;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorUnderrunError(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tx_dlimitr_underrun_err;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorConfigError(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->desc_config_error;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorPostTxTimerExpired(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tx_timer_expired;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorPowerManagement(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->pwr_management;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorTxOpExceeded(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->txop_exceeded;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorFinalTxIndex(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->final_tx_index;
	}
	return 0;
}

static unsigned char Ar5416TxDescriptorTid(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tid;
	}
	return 0;
}
#endif	

static unsigned int Ar5416TxDescriptorSendTimestamp(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->send_timestamp;
	}
	return 0;
}

#ifdef UNUSED
static unsigned long Ar5416TxDescriptorSeqnum(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->seqnum;
	}
	return 0;
}
#endif

static unsigned int Ar5416TxDescriptorBaBitmapLow(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->ba_bitmap_0_31;
	}
	return 0;
}

static unsigned int Ar5416TxDescriptorBaBitmapHigh(void *block)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->ba_bitmap_32_63;
	}
	return 0;
}

//
// setup a descriptor with the standard required fields
//
static int Ar5416TxDescriptorSetup(void *block, 
	unsigned int link_ptr, unsigned int buf_ptr, int buf_len,
	int broadcast, int retry,
	int rate, int ht40, int shortGi, unsigned int txchain,
	int nagg, int moreagg,
	int id)
{
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;
    memset(dr, 0, sizeof(struct Ar5416TxDescriptor));

	id=0;			// we don't use this field for merlin

	if(nagg<=1)
	{
		nagg=0;
	}
	//
	// initialize the descriptors:
	//    buffer size
	//    buffer address
	//    next descriptor pointer 
	//
    dr->buf_ptr = buf_ptr;
    dr->link_ptr = link_ptr;
	// 
	// install default values in all of the control words 2-13
    //
	dr->frame_length=buf_len;       //+4;		// what is this for?
	dr->vmf=0;
	dr->res2=0;
	dr->low_rx_chain=0;
	dr->clear_retry=0;
	dr->tpc_0;
	dr->rts_enable=0;
	dr->veol=0;
	dr->clear_dest_mask=1;
	dr->res2_1=0;
	dr->int_req=0;
	dr->dest_index_valid=0;
	dr->cts_enable=0;

	dr->buf_len=buf_len;
	dr->more=0;
	dr->dest_index= (broadcast?0:4);		// huh??
	dr->frame_type=0;
	dr->no_ack= broadcast;
	dr->insert_timestamp=1;					
	dr->more_agg=moreagg&(nagg>1);
	dr->is_agg=(nagg>1);
	dr->more_rifs=0;		//(nagg>1);

	dr->burst_duration=0;
	dr->dur_update_en=0;
	if(retry<1)
	{
		retry=1;
	}
	if(retry>15)
	{
		retry=15;
	}
	dr->tx_tries0=retry;
	dr->tx_tries1=0;
	dr->tx_tries2=0;
	dr->tx_tries3=0;

	dr->tx_rate0=rateValues[rate];
	dr->tx_rate1=0;
	dr->tx_rate2=0;
	dr->tx_rate3=0;

	dr->packet_duration0=0;
	dr->rts_cts_qual0=0;
	dr->packet_duration1=0;
	dr->rts_cts_qual1=0;

	dr->packet_duration2=0;
	dr->rts_cts_qual2=0;
	dr->packet_duration3=0;
	dr->rts_cts_qual3=0;

	dr->agg_length=(nagg*(4+dr->frame_length));
	dr->res8=0;
	dr->pad_delim=0;
	dr->encrypt_type=0;
	dr->res8_1=0;

	dr->h20_40_0=IS_HT40_RATE_INDEX(rate);
	dr->gi_0=shortGi;
	dr->chain_sel_0=txchain;
	dr->h20_40_1=0;
	dr->gi_1=0;
	dr->chain_sel_1=0;
	dr->h20_40_2=0;
	dr->gi_2=0;
	dr->chain_sel_2=0;
	dr->h20_40_3=0;
	dr->gi_3=0;
	dr->chain_sel_3=0;
	dr->rts_cts_rate=0;
	dr->stbc=0;

	dr->antenna0=0;
	dr->res2_10=0;

	dr->antenna1=0;
	dr->res23_2=0;

	dr->antenna2=0;
	dr->res2_12=0;

	dr->antenna3=0;
	dr->res2_13=0;

	return 0;
}

//
// reset the descriptor so that it can be used again
//
static int Ar5416TxDescriptorReset(void *block)
{  
	struct Ar5416TxDescriptor *dr=(struct Ar5416TxDescriptor *)block;
	//
	// clear the status fields, leave the control fields unchanged
	//
#ifdef UNUSED
	dr->rssi_ant00=0;
	dr->rssi_ant01=0;
	dr->res2_14=0;
	dr->res2_14_1=0;
	dr->ba_status=0;
	dr->res2_14_2=0;

	dr->frm_xmit_ok=0;
	dr->excessive_retries=0;
	dr->fifo_underrun=0;
	dr->filtered=0;
	dr->rts_fail_cnt=0;
	dr->data_fail_cnt=0;
	dr->virtual_retry_cnt=0;
	dr->tx_dlimitr_underrun_err=0;
	dr->tx_data_underrun_err=0;
	dr->desc_config_error=0;
	dr->tx_timer_expired=0;
	dr->res2_15=0;

	dr->send_timestamp=0;

	dr->ba_bitmap_0_31=0;

	dr->ba_bitmap_32_63=0;

	dr->rssi_ant10=0;
	dr->rssi_ant11=0;
	dr->res2_19=0;
	dr->ack_rssi_combined=0;

	dr->evm0=0;

	dr->evm1=0;

	dr->evm2=0;

	dr->done=0;
	dr->seqnum=0;
	dr->res23=0;
	dr->txop_exceeded=0;
	dr->res23_1=0;
	dr->final_tx_index=0;
	dr->res23_2=0;
	dr->pwr_management=0;
	dr->res23_3=0;
	dr->tid=0;
#else
	dr->send_timestamp=0;
	dr->done=0;
#endif
	return 0;
}

//
// return the size of a descriptor 
//
int Ar5416TxDescriptorSize(void)
{
   return sizeof(struct Ar5416TxDescriptor);
}

#ifdef UNUSED
//
// copy the descriptor from application memory to the shared memory
//
static void Ar5416TxDescriptorWrite(void *block, unsigned long physical)
{
    OSmemWrite(physical, block, sizeof(Ar5416TxDescriptor));
}

//
// copy the descriptor from the shared memory to application memory
//
static void Ar5416TxDescriptorRead(void *block, unsigned long physical, void (*read)()
{
    LIB_DEV_INFO *pLibDev = gLibInfo.pLibDevArray[devNum];

    pLibDev->devMap.OSmemRead(devNum, physical,
                block, sizeof(Ar5416TxDescriptor));

    OSmemRead(physical, block, );
}
#endif

static struct _TxDescriptorFunction _Ar5416TxDescriptor=
{
    Ar5416TxDescriptorLinkPtrSet,
    Ar5416TxDescriptorTxRateSet,
    Ar5416TxDescriptorBaStatus,
    Ar5416TxDescriptorAggLength,
    Ar5416TxDescriptorBaBitmapLow,
    Ar5416TxDescriptorBaBitmapHigh,
    Ar5416TxDescriptorFifoUnderrun,
    Ar5416TxDescriptorExcessiveRetries,
    Ar5416TxDescriptorRtsFailCount,
    Ar5416TxDescriptorDataFailCount,
    Ar5416TxDescriptorLinkPtr,
    Ar5416TxDescriptorBufPtr,
    Ar5416TxDescriptorBufLen,
    Ar5416TxDescriptorIntReq,
    Ar5416TxDescriptorRssiCombined,
    Ar5416TxDescriptorRssiAnt00,
    Ar5416TxDescriptorRssiAnt01,
    0,										// Ar5416TxDescriptorRssiAnt02,
    Ar5416TxDescriptorRssiAnt10,
    Ar5416TxDescriptorRssiAnt11,
    0,										// Ar5416TxDescriptorRssiAnt12,
    Ar5416TxDescriptorTxRate,
    0,										// CHANGEKIWI Ar5416TxDescriptorDataLen,
    0,										// Ar5416TxDescriptorMore,
    0,										// Ar5416TxDescriptorNumDelim,
    Ar5416TxDescriptorSendTimestamp,
    0,										// Ar5416TxDescriptorGi,
    0,										// Ar5416TxDescriptorH2040,
    0,										// Ar5416TxDescriptorDuplicate,
    0,										// Ar5416TxDescriptorTxAntenna,
    0,										// Ar5416TxDescriptorEvm0,
    0,										// Ar5416TxDescriptorEvm1,
    0,										// Ar5416TxDescriptorEvm2,
    Ar5416TxDescriptorDone,
    Ar5416TxDescriptorFrameTxOk,
    0,										// Ar5416TxDescriptorCrcError,
    0,										// Ar5416TxDescriptorDecryptCrcErr,
    0,										// Ar5416TxDescriptorPhyErr,
    0,										// Ar5416TxDescriptorMicError,
    0,										// Ar5416TxDescriptorPreDelimCrcErr,
    0,										// Ar5416TxDescriptorKeyIdxValid,
    0,										// Ar5416TxDescriptorKeyIdx,
    Ar5416TxDescriptorMoreAgg,
    Ar5416TxDescriptorAggregate,
    0,										// Ar5416TxDescriptorPostDelimCrcErr,
    0,										// Ar5416TxDescriptorDecryptBusyErr,
    0,										// Ar5416TxDescriptorKeyMiss,
    Ar5416TxDescriptorSetup,
    0,										// Ar5416TxDescriptorStatusSetup,
    Ar5416TxDescriptorReset,
    Ar5416TxDescriptorSize,
    0,										// Ar5416TxDescriptorStatusSize,
    Ar5416TxDescriptorPrint,
    0,										// Ar5416TxDescriptorPAPDSetup,
#ifdef UNUSED
    Ar5416TxDescriptorWrite,
    Ar5416TxDescriptorRead,
#endif
};


int Ar5416TxDescriptorFunctionSelect()
{
	return TxDescriptorFunctionSelect(&_Ar5416TxDescriptor);
}
