

#ifdef _WINDOWS
#ifdef AR9300DLL
		#define AR9300DLLSPEC __declspec(dllexport)
	#else
		#define AR9300DLLSPEC __declspec(dllimport)
	#endif
#else
	#define AR9300DLLSPEC
#endif



#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "wlantype.h"
#include "rate_constants.h"

#include "smatch.h"

#include "TxDescriptor.h"
#include "Ar9300TxDescriptor.h"


#include "mEepStruct9300.h"
//#include "default9300.h"
#include "Sticky.h"
#include "ParameterSelect.h"   
#include "ParameterParse.h"   

#include "Card.h"

//
// hal header files
//
#include "ah.h"
#include "ah_internal.h"
#include "ar9300.h"
#include "ar9300eep.h"

#include "Ar9300EepromStructSet.h"

#define MBUFFER 1024


//
// ar9300 chip. osprey.
//
// descriptor is split into two parts: control and status.
// note that the first word is defined the same in both.
// we can figure out which by  looking at desc_ctrl_status.
//

#ifdef ARCH_BIG_ENDIAN
struct Ar9300TxDescriptorControl
{
	//
    // TXC  0 { atheros_id[31:16] = 0x168C, desc_tx_rx[15] = 1, desc_ctrl_stat[14] = 1,
    //         reserved[13:12], tx_qcu_num[11:8], desc_length[7:0] = 0x19 }
	unsigned int atheros_id:16;	// must be 0x168c
	unsigned int desc_tx_rx:1;		// must be 1
	unsigned int desc_ctrl_stat:1;	// must be 1	
	unsigned int res0:2;
	unsigned int tx_qcu_num:4;
	unsigned int desc_length:8;	// must be 0x19

	//
	// TXC  1 { link_ptr[31:0] }
	//
	unsigned int link_ptr;				
	//
	// TXC  2 { buf_ptr0[31:0] }
	//
	unsigned int buf_ptr;				
    //
	// TXC  3 { reserved[31:28], buf_len0[27:16], reserved[15:0] }
	//
	unsigned int res3_1:4;
	unsigned int buf_len:12;
	unsigned int res3:16;	
	//
	// TXC  4 { buf_ptr1[31:0] }
	//
	unsigned int buf_ptr1;			
    //
	// TXC  5 { reserved[31:28], buf_len1[27:16], reserved[15:0] }
	//
	unsigned int res5_1:4;
	unsigned int buf_len1:12;
	unsigned int res5:16;	
	//
	// TXC  6 { buf_ptr2[31:0] }
	//
	unsigned int buf_ptr2;			
    //
	// TXC  7 { reserved[31:28], buf_len2[27:16], reserved[15:0] }
	//
	unsigned int res7_1:4;
	unsigned int buf_len2:12;
	unsigned int res7:16;	
	//
	// TXC  8 { buf_ptr3[31:0] }
	//
	unsigned int buf_ptr3;				
    //
	// TXC  9 { reserved[31:28], buf_len3[27:16], reserved[15:0] }
	//
    unsigned int res9_1:4;
	unsigned int buf_len3:12;
	unsigned int res9:16;	
	//
	// TXC 10 { tx_desc_id[31:16], ptr_checksum[15:0] }
	//
	unsigned int tx_desc_id:16;
	unsigned int ptr_checksum:16;		
	//
	// TXC 11 { cts_enable[31], dest_index_valid[30], int_req[29], beam_form[28:25],
	//		 clear_dest_mask[24], veol[23], rts_enable[22], tpc[21:16], clear_retry[15],
	//		 low_rx_chain[14], fast_ant_mode[13], vmf[12], frame_length[11:0] }
	//
	unsigned int cts_enable:1;
	unsigned int dest_index_valid:1;
	unsigned int int_req:1;
	unsigned int beam_form:4;
	unsigned int clear_dest_mask:1;
	unsigned int veol:1;
	unsigned int rts_enable:1;
	unsigned int tpc_0:6;
	unsigned int clear_retry:1;
	unsigned int low_rx_chain:1;
	unsigned int fast_ant_mode:1;
	unsigned int vmf:1;
	unsigned int frame_length:12;		
	//
	//TXC 12 { more_rifs[31], is_agg[30], more_agg[29], ext_and_ctl[28], ext_only[27],
	//		 corrupt_fcs[26], insert_timestamp[25], no_ack[24], frame_type[23:20],
	//		 dest_index[19:13], more[12], pa_pre_distortion_chain_mask[2:0],reserved[8:0] }
	//
	unsigned int more_rifs:1;
	unsigned int is_agg:1;
	unsigned int more_agg:1;
	unsigned int ext_and_ctl:1;
	unsigned int ext_only:1;
	unsigned int corrupt_fcs:1;
	unsigned int insert_timestamp:1;
	unsigned int no_ack:1;
	unsigned int frame_type:4;
	unsigned int dest_index:7;
	unsigned int more:1;
	unsigned int pa_predistortion_chain_mask:3;
	unsigned int res12:9;
	//
	//
	// TXC 13 { tx_tries3[31:28], tx_tries2[27:24], tx_tries1[23:20], tx_tries0[19:16],
	//		 dur_update_en[15], burst_duration[14:0] }
	//
	unsigned int tx_tries3:4;
	unsigned int tx_tries2:4;
	unsigned int tx_tries1:4;
	unsigned int tx_tries0:4;
	unsigned int dur_update_en:1;
	unsigned int burst_duration:15;
	//
	// TXC 14 { tx_rate3[31:24], tx_rate2[23:16], tx_rate1[15:8], tx_rate0[7:0] }
	//
	unsigned int tx_rate3:8;
	unsigned int tx_rate2:8;
	unsigned int tx_rate1:8;
	unsigned int tx_rate0:8;
	//
	// TXC 15 { rts_cst_qual1[31], packet_duration1[30:16], rts_cts_qual0[15], packet_duration0[14:0] }
	//
	unsigned int rts_cts_qual1:1;
	unsigned int packet_duration1:15;
	unsigned int rts_cts_qual0:1;
	unsigned int packet_duration0:15;	
	//
	// TXC 16 { rts_cst_qual3[31], packet_duration3[30:16], rts_cts_qual2[15], packet_duration2[14:0] }
	//
	unsigned int rts_cts_qual3:1;
	unsigned int packet_duration3:15;
	unsigned int rts_cts_qual2:1;
	unsigned int packet_duration2:15;	
	//
	// TXC 17 { ldpc[31], calibrating[30], dc_ap_sta_sel[29], encrypt_type[28:26],
	//		 pad_delim[25:18], reserved[17:16], agg_length[15:0] }
	//
	unsigned int ldpc:1;
	unsigned int calibrating:1;
	unsigned int dc_ap_sta_sel:1;
	unsigned int encrypt_type:3;
	unsigned int pad_delim:8;
	unsigned int res17:2;
	unsigned int agg_length:16;	
	//
	// TXC 18 { stbc[31:28], rts_cts_rate[27:20], chain_sel_3[19:17], gi_3[16],20_40_3[15],
	//		 chain_sel_2[14:12], gi_2[11], 20_40_2[10], chain_sel_1[9:7], gi_1[6],
	//		 20_40_1[5], chain_sel_0[4:2], gi_0[1], 20_40_0[0] }
	//
	unsigned int stbc:4;
	unsigned int rts_cts_rate:8;
	unsigned int chain_sel_3:3;
	unsigned int gi_3:1;
	unsigned int h20_40_3:1;
	unsigned int chain_sel_2:3;
	unsigned int gi_2:1;
	unsigned int h20_40_2:1;
	unsigned int chain_sel_1:3;
	unsigned int gi_1:1;
	unsigned int h20_40_1:1;
	unsigned int chain_sel_0:3;
	unsigned int gi_0:1;
	unsigned int h20_40_0:1;			
	//
	// TXC 19 { Ness[31:30], not_sounding[29], rts_htc_trq[28], rts_htc_mrq[27],
	//		 rts_htc_msi[26:24], antenna_0[23:0] }
	//
	unsigned int ness:2;
	unsigned int not_sounding:1;
	unsigned int rts_htc_trq:1;
	unsigned int rts_htc_mrq:1;
	unsigned int rts_htc_msi:3;
	unsigned int antenna0:24;
	//
	// TXC 20 { reserved[31:30], tpc_1[29:24], antenna_1[23:0] }
	//
	unsigned int res20:2;
	unsigned int tpc_1:6;
	unsigned int antenna1:24;			
	//
	// TXC 21 { reserved[31:30], tpc_2[29:24], antenna_2[23:0] }
	//
	unsigned int res21:2;
	unsigned int tpc_2:6;
	unsigned int antenna2:24;	
	//
	// TXC 22 { reserved[31:30], tpc_3[29:24], antenna_3[23:0] }
	//
	unsigned int res22:2;
	unsigned int tpc_3:6;
	unsigned int antenna3:24;	
};


struct Ar9300TxDescriptorStatus
{
	//
    // TXS  0 { atheros_id[31:16] = 0x168C, desc_tx_rx[15] = 1, desc_ctrl_stat[14] = 0,
    //     reserved[13:12], tx_qcu_num[11:8], desc_length[7:0] = 0x09 }
    //     
	unsigned int atheros_id:16;	// must be 0x168c
	unsigned int desc_tx_rx:1;		// must be 1
	unsigned int desc_ctrl_stat:1;	// must be 0	
	unsigned int res0:2;
	unsigned int tx_qcu_num:4;
	unsigned int desc_length:8;	// must be 0x09
    //
	// TXS  1 { tx_desc_id[31:16], reserved[15:0] }
	//
	unsigned int tx_desc_id:16;
	unsigned int res1:16;		
	//
    // TXS  2 { reserved[31], ba_status[30], ack_rssi_and02[23:16], ack_rssi_ant01[15:8],
    //     ack_rssi_ant00[7:0] }
	//
	unsigned int res2_1:1;
	unsigned int ba_status:1;
	unsigned int res2:6;
	unsigned int rssi_ant02:8;
	unsigned int rssi_ant01:8;
	unsigned int rssi_ant00:8;	
	//
    // TXS  3 { reserved[31:20], tx_timer_expired[19], desc_config_error[18],
    //     tx_data_underrun_err[16], virtual_retry_cnt[15:12], data_fail_cnt[11:8],
    //     rts_fail_cnt[7:4], filtered[3], fifo_underrun[2], excessive_retries[1],
    //     frm_xmit_ok[0] }
	//
	unsigned int res3:12;
	unsigned int tx_timer_expired:1;
	unsigned int desc_config_error:1;
	unsigned int tx_data_underrun_err:1;
	unsigned int tx_dlimitr_underrun_err:1;
	unsigned int virtual_retry_cnt:4;
	unsigned int data_fail_cnt:4;
	unsigned int rts_fail_cnt:4;
	unsigned int filtered:1;
	unsigned int fifo_underrun:1;
	unsigned int excessive_retries:1;
	unsigned int frm_xmit_ok:1;		
	//
    // TXS  4 { send_timestamp[31:0] }
	//
	unsigned int send_timestamp;		
	//
    // TXS  5 { ba_bitmap_0_31[31:0] }
	//
	unsigned int ba_bitmap_0_31;		
	//
    // TXS  6 { ba_bitmap_32_63[31:0] }
	//
	unsigned int ba_bitmap_32_63;		
	//
    // TXS  7 { ack_rssi_combined[31:24], ack_rssi_ant12[23:16],
    //    ack_rssi_ant11[15:8], ack_rssi_ant10[7:0] }
    //
	unsigned int rssi_combined:8;		
	unsigned int rssi_ant12:8;
	unsigned int rssi_ant11:8;
	unsigned int rssi_ant10:8;			
	//
    // TXS  8 { tid[[31:28], reserved[27:26], pwr_mgmt[25], reserved[24:23], final_tx_index[22:21],
    //     txbf_cv_miss[20], txbf_stream_miss[19], txbf_bw_mismatch[18], txop_exceeded[17], 
	//  reserved[16:13], seq_num[12:1], done[0] }
	//
	unsigned int tid:4;
	unsigned int res8_2:2;
	unsigned int pwr_management:1;
	unsigned int res8_1:2;
	unsigned int final_tx_index:2;
	unsigned int txbf_cv_miss:1;
	unsigned int txbf_stream_miss:1;
	unsigned int txbf_bw_mismatch:1;
	unsigned int txop_exceeded:1;
	unsigned int res8:4;
	unsigned int seqnum:12;
	unsigned int done:1;				
};
#else 
// For little endian architecture
struct Ar9300TxDescriptorControl
{
	//
    // TXC  0 { atheros_id[31:16] = 0x168C, desc_tx_rx[15] = 1, desc_ctrl_stat[14] = 1,
    //         reserved[13:12], tx_qcu_num[11:8], desc_length[7:0] = 0x19 }
	unsigned int desc_length:8;	// must be 0x19
    unsigned int tx_qcu_num:4;
    unsigned int res0:2;
    unsigned int desc_ctrl_stat:1;	// must be 1	
    unsigned int desc_tx_rx:1;		// must be 1
	unsigned int atheros_id:16;	// must be 0x168c
    //
	// TXC  1 { link_ptr[31:0] }
	//
	unsigned int link_ptr;				
	//
	// TXC  2 { buf_ptr0[31:0] }
	//
	unsigned int buf_ptr;				
    //
	// TXC  3 { reserved[31:28], buf_len0[27:16], reserved[15:0] }
	//
	unsigned int res3:16;	
    unsigned int buf_len:12;
    unsigned int res3_1:4;
	//
	// TXC  4 { buf_ptr1[31:0] }
	//
	unsigned int buf_ptr1;			
    //
	// TXC  5 { reserved[31:28], buf_len1[27:16], reserved[15:0] }
	//
	unsigned int res5:16;	
    unsigned int buf_len1:12;
    unsigned int res5_1:4;
	//
	// TXC  6 { buf_ptr2[31:0] }
	//
	unsigned int buf_ptr2;			
    //
	// TXC  7 { reserved[31:28], buf_len2[27:16], reserved[15:0] }
	//
	unsigned int res7:16;	
    unsigned int buf_len2:12;
    unsigned int res7_1:4;
	//
	// TXC  8 { buf_ptr3[31:0] }
	//
	unsigned int buf_ptr3;				
    //
	// TXC  9 { reserved[31:28], buf_len3[27:16], reserved[15:0] }
	//
	unsigned int res9:16;	
    unsigned int buf_len3:12;
    unsigned int res9_1:4;
	//
	// TXC 10 { tx_desc_id[31:16], ptr_checksum[15:0] }
	//
	unsigned int ptr_checksum:16;		
	unsigned int tx_desc_id:16;
	//
	// TXC 11 { cts_enable[31], dest_index_valid[30], int_req[29], beam_form[28:25],
	//		 clear_dest_mask[24], veol[23], rts_enable[22], tpc[21:16], clear_retry[15],
	//		 low_rx_chain[14], fast_ant_mode[13], vmf[12], frame_length[11:0] }
	//
	unsigned int frame_length:12;		
	unsigned int vmf:1;
	unsigned int fast_ant_mode:1;
	unsigned int low_rx_chain:1;
	unsigned int clear_retry:1;
	unsigned int tpc_0:6;
	unsigned int rts_enable:1;
	unsigned int veol:1;
	unsigned int clear_dest_mask:1;
	unsigned int beam_form:4;
	unsigned int int_req:1;
	unsigned int dest_index_valid:1;
	unsigned int cts_enable:1;
	//
	//TXC 12 { more_rifs[31], is_agg[30], more_agg[29], ext_and_ctl[28], ext_only[27],
	//		 corrupt_fcs[26], insert_timestamp[25], no_ack[24], frame_type[23:20],
	//		 dest_index[19:13], more[12], pa_pre_distortion_chain_mask[2:0],reserved[8:0] }
	//
	unsigned int res12:9;
	unsigned int pa_predistortion_chain_mask:3;
	unsigned int more:1;
	unsigned int dest_index:7;
	unsigned int frame_type:4;
	unsigned int no_ack:1;
	unsigned int insert_timestamp:1;
	unsigned int corrupt_fcs:1;
	unsigned int ext_only:1;
	unsigned int ext_and_ctl:1;
	unsigned int more_agg:1;
	unsigned int is_agg:1;
	unsigned int more_rifs:1;
	//
	//
	// TXC 13 { tx_tries3[31:28], tx_tries2[27:24], tx_tries1[23:20], tx_tries0[19:16],
	//		 dur_update_en[15], burst_duration[14:0] }
	//
	unsigned int burst_duration:15;
	unsigned int dur_update_en:1;
	unsigned int tx_tries0:4;
	unsigned int tx_tries1:4;
	unsigned int tx_tries2:4;
	unsigned int tx_tries3:4;
	//
	// TXC 14 { tx_rate3[31:24], tx_rate2[23:16], tx_rate1[15:8], tx_rate0[7:0] }
	//
	unsigned int tx_rate0:8;
	unsigned int tx_rate1:8;
	unsigned int tx_rate2:8;
	unsigned int tx_rate3:8;
	//
	// TXC 15 { rts_cst_qual1[31], packet_duration1[30:16], rts_cts_qual0[15], packet_duration0[14:0] }
	//
	unsigned int packet_duration0:15;	
	unsigned int rts_cts_qual0:1;
	unsigned int packet_duration1:15;
	unsigned int rts_cts_qual1:1;
	//
	// TXC 16 { rts_cst_qual3[31], packet_duration3[30:16], rts_cts_qual2[15], packet_duration2[14:0] }
	//
	unsigned int packet_duration2:15;	
	unsigned int rts_cts_qual2:1;
	unsigned int packet_duration3:15;
	unsigned int rts_cts_qual3:1;
	//
	// TXC 17 { ldpc[31], calibrating[30], dc_ap_sta_sel[29], encrypt_type[28:26],
	//		 pad_delim[25:18], reserved[17:16], agg_length[15:0] }
	//
	unsigned int agg_length:16;	
	unsigned int res17:2;
	unsigned int pad_delim:8;
	unsigned int encrypt_type:3;
	unsigned int dc_ap_sta_sel:1;
	unsigned int calibrating:1;
	unsigned int ldpc:1;
	//
	// TXC 18 { stbc[31:28], rts_cts_rate[27:20], chain_sel_3[19:17], gi_3[16],20_40_3[15],
	//		 chain_sel_2[14:12], gi_2[11], 20_40_2[10], chain_sel_1[9:7], gi_1[6],
	//		 20_40_1[5], chain_sel_0[4:2], gi_0[1], 20_40_0[0] }
	//
	unsigned int h20_40_0:1;			// word 9
	unsigned int gi_0:1;
	unsigned int chain_sel_0:3;
	unsigned int h20_40_1:1;
	unsigned int gi_1:1;
	unsigned int chain_sel_1:3;
	unsigned int h20_40_2:1;
	unsigned int gi_2:1;
	unsigned int chain_sel_2:3;
	unsigned int h20_40_3:1;
	unsigned int gi_3:1;
	unsigned int chain_sel_3:3;
	unsigned int rts_cts_rate:8;
	unsigned int stbc:4;
	//
	// TXC 19 { Ness[31:30], not_sounding[29], rts_htc_trq[28], rts_htc_mrq[27],
	//		 rts_htc_msi[26:24], antenna_0[23:0] }
	//
	unsigned int antenna0:24;
	unsigned int rts_htc_msi:3;
	unsigned int rts_htc_mrq:1;
	unsigned int rts_htc_trq:1;
	unsigned int not_sounding:1;
	unsigned int ness:2;
	//
	// TXC 20 { reserved[31:30], tpc_1[29:24], antenna_1[23:0] }
	//
	unsigned int antenna1:24;			
	unsigned int tpc_1:6;
	unsigned int res20:2;
	//
	// TXC 21 { reserved[31:30], tpc_2[29:24], antenna_2[23:0] }
	//
	unsigned int antenna2:24;	
	unsigned int tpc_2:6;
	unsigned int res21:2;
	//
	// TXC 22 { reserved[31:30], tpc_3[29:24], antenna_3[23:0] }
	//
	unsigned int antenna3:24;	
	unsigned int tpc_3:6;
	unsigned int res22:2;
};


struct Ar9300TxDescriptorStatus
{
	//
    // TXS  0 { atheros_id[31:16] = 0x168C, desc_tx_rx[15] = 1, desc_ctrl_stat[14] = 0,
    //     reserved[13:12], tx_qcu_num[11:8], desc_length[7:0] = 0x09 }
    //     
	unsigned int desc_length:8;	// must be 0x09
    unsigned int tx_qcu_num:4;
    unsigned int res0:2;
    unsigned int desc_ctrl_stat:1;	// must be 0	
    unsigned int desc_tx_rx:1;		// must be 1
	unsigned int atheros_id:16;	// must be 0x168c
    //
	// TXS  1 { tx_desc_id[31:16], reserved[15:0] }
	//
	unsigned int res1:16;		
	unsigned int tx_desc_id:16;
	//
    // TXS  2 { reserved[31], ba_status[30], ack_rssi_and02[23:16], ack_rssi_ant01[15:8],
    //     ack_rssi_ant00[7:0] }
	//
	unsigned int rssi_ant00:8;	
	unsigned int rssi_ant01:8;
	unsigned int rssi_ant02:8;
	unsigned int res2:6;
	unsigned int ba_status:1;
	unsigned int res2_1:1;
	//
    // TXS  3 { reserved[31:20], tx_timer_expired[19], desc_config_error[18],
    //     tx_data_underrun_err[16], virtual_retry_cnt[15:12], data_fail_cnt[11:8],
    //     rts_fail_cnt[7:4], filtered[3], fifo_underrun[2], excessive_retries[1],
    //     frm_xmit_ok[0] }
	//
	unsigned int frm_xmit_ok:1;		
	unsigned int excessive_retries:1;
	unsigned int fifo_underrun:1;
	unsigned int filtered:1;
	unsigned int rts_fail_cnt:4;
	unsigned int data_fail_cnt:4;
	unsigned int virtual_retry_cnt:4;
	unsigned int tx_dlimitr_underrun_err:1;
	unsigned int tx_data_underrun_err:1;
	unsigned int desc_config_error:1;
	unsigned int tx_timer_expired:1;
	unsigned int res3:12;
	//
    // TXS  4 { send_timestamp[31:0] }
	//
	unsigned int send_timestamp;		
	//
    // TXS  5 { ba_bitmap_0_31[31:0] }
	//
	unsigned int ba_bitmap_0_31;		// word 17
	//
    // TXS  6 { ba_bitmap_32_63[31:0] }
	//
	unsigned int ba_bitmap_32_63;		// word 18
	//
    // TXS  7 { ack_rssi_combined[31:24], ack_rssi_ant12[23:16],
    //    ack_rssi_ant11[15:8], ack_rssi_ant10[7:0] }
    //
	unsigned int rssi_ant10:8;			// word 19
	unsigned int rssi_ant11:8;
	unsigned int rssi_ant12:8;
	unsigned int rssi_combined:8;
	//
    // TXS  8 { tid[[31:28], reserved[27:26], pwr_mgmt[25], reserved[24:23], final_tx_index[22:21],
    //     txbf_cv_miss[20], txbf_stream_miss[19], txbf_bw_mismatch[18], txop_exceeded[17], 
	//  reserved[16:13], seq_num[12:1], done[0] }
	//
	unsigned int done:1;				// word 23
	unsigned int seqnum:12;
	unsigned int res8:4;
	unsigned int txop_exceeded:1;
	unsigned int txbf_bw_mismatch:1;
	unsigned int txbf_stream_miss:1;
	unsigned int txbf_cv_miss:1;
	unsigned int final_tx_index:2;
	unsigned int res8_1:2;
	unsigned int pwr_management:1;
	unsigned int res8_2:2;
	unsigned int tid:4;
};
#endif   // end of endianess compile flag



AR9300DLLSPEC int Ar9300TxDescriptorControlValid(void *block)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;

	return (dr!=0 && dr->atheros_id==0x168c && dr->desc_tx_rx==1 && dr->desc_ctrl_stat==1 && dr->desc_length==0x19);

}


AR9300DLLSPEC int Ar9300TxDescriptorStatusValid(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;
	return (dr!=0 && dr->atheros_id==0x168c && dr->desc_tx_rx==1 && dr->desc_ctrl_stat==0 && dr->desc_length==0x09);

}


//
// print all of the fields of the descriptor
//
AR9300DLLSPEC int Ar9300TxDescriptorUnknownPrint(void *block, char *buffer, int max)
{
	int it;
	int lc, nc;
	unsigned int *dr=(unsigned int *)block;

 	lc=0;
	buffer[0]=0;

	for(it=0; it<16; it++)
	{
		nc=SformatOutput(&buffer[lc],max-lc-1,"%08x`",dr[it]);
		if(nc>0)
		{
			lc+=nc;

		}
	}
	return lc;
}


//
// print all of the fields of the descriptor
//
int Ar9300TxDescriptorControlPrint(void *block, char *buffer, int max)
{
	int lc, nc;
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;

 	lc=0;
	buffer[0]=0;

	if(Ar9300TxDescriptorControlValid(block))
	{
		nc=SformatOutput(&buffer[lc],max-lc-1,"`%x %x %x %x %x %x`", 	
			dr->atheros_id,
			dr->desc_tx_rx,
			dr->desc_ctrl_stat,
			dr->res0,
			dr->tx_qcu_num,
			dr->desc_length);
		if(nc>0)
		{
			lc+=nc;

		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x`", dr->link_ptr);
		if(nc>0)
		{
			lc+=nc;

		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x`", dr->buf_ptr);
		if(nc>0)
		{
			lc+=nc;
		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x`", dr->res3_1, dr->buf_len, dr->res3);
		if(nc>0)
		{
			lc+=nc;

		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x`", dr->buf_ptr1);
		if(nc>0)
		{
			lc+=nc;
		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x`", dr->res5_1, dr->buf_len1, dr->res5);
		if(nc>0)
		{
			lc+=nc;

		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x`", dr->buf_ptr2);
		if(nc>0)
		{
			lc+=nc;
		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x`", dr->res7_1, dr->buf_len2, dr->res7);
		if(nc>0)
		{
			lc+=nc;

		}
		nc=SformatOutput(&buffer[lc],max-lc-1,"%x`", dr->buf_ptr3);
		if(nc>0)
		{
			lc+=nc;
		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x`", dr->res9_1, dr->buf_len3, dr->res9);
		if(nc>0)
		{
			lc+=nc;

		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x`",
			dr->tx_desc_id,
			dr->ptr_checksum);
		if(nc>0)
		{
			lc+=nc;

		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x %x %x %x %x %x %x %x`",
			dr->cts_enable,
			dr->dest_index_valid,
			dr->int_req,
			dr->beam_form,
			dr->clear_dest_mask,
			dr->veol,
			dr->rts_enable,
			dr->tpc_0,
			dr->clear_retry,
			dr->low_rx_chain,
			dr->fast_ant_mode,
			dr->vmf,
			dr->frame_length);
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
			dr->pa_predistortion_chain_mask);
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
			dr->burst_duration);
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
			dr->packet_duration0);
		if(nc>0)
		{
			lc+=nc;
		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x`", 	
			dr->rts_cts_qual3,
			dr->packet_duration3,
			dr->rts_cts_qual2,
			dr->packet_duration2);
		if(nc>0)
		{
			lc+=nc;
		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x %x`", 	
			dr->ldpc,
			dr->calibrating,
			dr->dc_ap_sta_sel,
			dr->encrypt_type,
			dr->pad_delim,
			dr->res17,
			dr->agg_length);
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
			dr->h20_40_0);
		if(nc>0)
		{
			lc+=nc;
		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x`", 	
			dr->ness,
			dr->not_sounding,
			dr->rts_htc_trq,
			dr->rts_htc_mrq,
			dr->rts_htc_msi,
			dr->antenna0);
		if(nc>0)
		{
			lc+=nc;
		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x`", 	
			dr->res20,
			dr->tpc_1,
			dr->antenna1);
		if(nc>0)
		{
			lc+=nc;
		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x`", 	
			dr->res21,
			dr->tpc_2,
			dr->antenna1);
		if(nc>0)
		{
			lc+=nc;
		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x`", 	
			dr->res22,
			dr->tpc_3,
			dr->antenna1);
		if(nc>0)
		{
			lc+=nc;
		}
	}

	return lc;
}


//
// print all of the fields of the descriptor
//
int Ar9300TxDescriptorStatusPrint(void *block, char *buffer, int max)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;
	int lc, nc;

	lc=0;
	buffer[0]=0;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		nc=SformatOutput(&buffer[lc],max-lc-1,"`%x %x %x %x %x %x`", 	
			dr->atheros_id,
			dr->desc_tx_rx,
			dr->desc_ctrl_stat,
			dr->res0,
			dr->tx_qcu_num,
			dr->desc_length);
		if(nc>0)
		{
			lc+=nc;
		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x`", 	
			dr->tx_desc_id,
			dr->res1);
		if(nc>0)
		{
			lc+=nc;
		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x`", 	
			dr->res2_1,
			dr->ba_status,
			dr->res2,
			dr->rssi_ant00,
			dr->rssi_ant01,
			dr->rssi_ant00);
		if(nc>0)
		{
			lc+=nc;
		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x %x %x %x %x %x %x`", 	
			dr->res3,
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
			dr->frm_xmit_ok);
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
			dr->rssi_combined,
			dr->rssi_ant12,
			dr->rssi_ant11,
			dr->rssi_ant10);
		if(nc>0)
		{
			lc+=nc;
		}

		nc=SformatOutput(&buffer[lc],max-lc-1,"%x %x %x %x %x %x %x %x %x %x %x %x`", 	
			dr->tid,
			dr->res8_2,
			dr->pwr_management,
			dr->res8_1,
			dr->final_tx_index,
			dr->txbf_cv_miss,
			dr->txbf_stream_miss,
			dr->txbf_bw_mismatch,
			dr->txop_exceeded,
			dr->res8,
			dr->seqnum,
			dr->done);
		if(nc>0)
		{
			lc+=nc;
		}
	}

	return lc;
}



//
// print all of the fields of the descriptor
//
AR9300DLLSPEC int Ar9300TxDescriptorPrint(void *block, char *buffer, int max)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return Ar9300TxDescriptorStatusPrint(block,buffer,max);
	}
	else if(Ar9300TxDescriptorControlValid(block))
	{
		return Ar9300TxDescriptorControlPrint(block,buffer,max);
	}
	else
	{
		return Ar9300TxDescriptorUnknownPrint(block,buffer,max);
	}
}

int Ar9300TxDescriptorControlId(void *block)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;

	if(dr!=0)
	{
		return dr->tx_desc_id;
	}
	return 0;
}

int Ar9300TxDescriptorStatusId(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(dr!=0)
	{
		return dr->tx_desc_id;
	}
	return 0;
}


//
// fetch the id fields of the descriptor
//
int Ar9300TxDescriptorId(void *block)
{
	if(Ar9300TxDescriptorStatusValid(block))
	{
		return Ar9300TxDescriptorStatusId(block);
	}
	else if(Ar9300TxDescriptorControlValid(block))
	{
		return Ar9300TxDescriptorControlId(block);
	}
	else
	{
		return -1;
	}
}

AR9300DLLSPEC int Ar9300TxDescriptorLinkPtrSet(void *block, unsigned int ptr)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;

	if(dr!=0)
	{
		dr->link_ptr=ptr;
		return 0;
	}
	return -1;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorLinkPtr(void *block)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;

	if(Ar9300TxDescriptorControlValid(block))
	{
		return dr->link_ptr;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorBufPtr(void *block)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;

	if(Ar9300TxDescriptorControlValid(block))
	{
		return dr->buf_ptr;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorBufLen(void *block)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;

	if(Ar9300TxDescriptorControlValid(block))
	{
		return dr->buf_len;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorIntReq(void *block)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;

	if(Ar9300TxDescriptorControlValid(block))
	{
		return (unsigned char)dr->int_req;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorDataLen(void *block)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;

	if(dr!=0)
	{
		return dr->frame_length;
	}
	return 0;
}

#ifdef UNUSED
AR9300DLLSPEC unsigned char Ar9300TxDescriptorVmf(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->vmf;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorLowRxChain(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->low_rx_chain;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorClearRetry(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->clear_retry;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorTpc0(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tpc_0;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorRtsEnable(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rts_enable;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorVeol(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->veol;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorClearDestMask(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->clear_dest_mask;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorDestIndexValid(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->dest_index_valid;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorCtsEnable(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->cts_enable;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorMore(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->more;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorDestIndex(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->dest_index;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorFrameType(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->frame_type;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorNoAck(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->no_ack;
	}
	return 0;
}
#endif

AR9300DLLSPEC unsigned char Ar9300TxDescriptorMoreAgg(void *block)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;

	if(Ar9300TxDescriptorControlValid(block))
	{
		return (unsigned char)dr->more_agg;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorAggregate(void *block)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;

	if(Ar9300TxDescriptorControlValid(block))
	{
		return (unsigned char)dr->is_agg;
	}
	return 0;
}

#ifdef UNUSED
AR9300DLLSPEC unsigned char Ar9300TxDescriptorMoreRifs(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->more_rifs;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorBurstDuration(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->burst_duration;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorDurUpdateEn(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->dur_update_en;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorTxTries0(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tx_tries0;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorTxTries1(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tx_tries1;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorTxTries2(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tx_tries2;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorTxTries3(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tx_tries3;
	}
	return 0;
}
#endif

AR9300DLLSPEC int Ar9300TxDescriptorTxRate0Set(void *block, unsigned int rate)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;

	if(Ar9300TxDescriptorControlValid(block))
	{
		dr->tx_rate0=rateValues[rate];		// check index range
		dr->h20_40_0=IS_HT40_RATE_INDEX(rate);
		return 0;
	}
	return -1;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorTxRate0(void *block)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;

	if(Ar9300TxDescriptorControlValid(block))
	{
        return descRate2RateIndex(dr->tx_rate0, (unsigned char)dr->h20_40_0);
	}
	return 0;
}


#ifdef UNUSED
AR9300DLLSPEC unsigned int Ar9300TxDescriptorTxRate1(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
        return descRate2RateIndex(dr->tx_rate1, dr->h20_40_1);
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorTxRate2(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
        return descRate2RateIndex(dr->tx_rate2, dr->h20_40_2);
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorTxRate3(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
        return descRate2RateIndex(dr->tx_rate3, dr->h20_40_3);
	}
	return 0;
}
#endif

AR9300DLLSPEC int Ar9300TxDescriptorTxRateSet(void *block, unsigned int rate)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;

	if(Ar9300TxDescriptorControlValid(block))
	{
		return Ar9300TxDescriptorTxRate0Set(block,rate);
	}
	return -1;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorTxRate(void *block)
{
	if(Ar9300TxDescriptorControlValid(block))
	{
		//
		// figure out which rate it was sent at #######
		//
        return Ar9300TxDescriptorTxRate0(block);
	}
	return 0;
}

#ifdef UNUSED
AR9300DLLSPEC unsigned int Ar9300TxDescriptorPacketDuration0(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->packet_duration0;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorPacketDuration1(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->packet_duration1;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorPacketDuration2(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->packet_duration2;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorPacketDuration3(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->packet_duration3;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorRtsCtsQual0(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rts_cts_qual0;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorRtsCtsQual1(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rts_cts_qual1;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorRtsCtsQual2(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rts_cts_qual2;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorRtsCtsQual3(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->rts_cts_qual3;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorGi0(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->gi_0;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorGi1(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->gi_1;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorGi2(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->gi_2;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorGi3(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->gi_3;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorChainSelect0(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->chain_sel_0;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorChainSelect1(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->chain_sel_1;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorChainSelect2(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->chain_sel_2;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorChainSelect3(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->chain_sel_3;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorH20400(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->h20_40_0;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorH20401(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->h20_40_1;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorH20402(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->h20_40_2;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorH20403(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->h20_40_3;
	}
	return 0;
}
#endif

AR9300DLLSPEC unsigned int Ar9300TxDescriptorAggLength(void *block)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;

	if(Ar9300TxDescriptorControlValid(block))
	{
		return dr->agg_length;
	}
	return 0;
}

#ifdef UNUSED
AR9300DLLSPEC unsigned long Ar9300TxDescriptorPadDelim(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->pad_delim;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorEncryptType(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->encrypt_type;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorRtsCtsRate(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
        return descRate2RateIndex(dr->rts_cts_rate, 0);
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorStbc(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->stbc;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorAntenna0(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->antenna0;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorAntenna1(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->antenna1;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorAntenna2(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->antenna2;
	}
	return 0;
}
#endif



// status words 14-23


AR9300DLLSPEC unsigned int Ar9300TxDescriptorRssiCombined(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return dr->rssi_combined;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorRssiAnt00(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return dr->rssi_ant00;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorRssiAnt01(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return dr->rssi_ant01;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorRssiAnt02(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return dr->rssi_ant02;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorRssiAnt10(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return dr->rssi_ant10;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorRssiAnt11(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return dr->rssi_ant11;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorRssiAnt12(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return dr->rssi_ant12;
	}
	return 0;
}

#ifdef UNUSED
AR9300DLLSPEC double Ar9300TxDescriptorEvm0(void *block)
{
	block=0;
	return 0;
}

AR9300DLLSPEC double Ar9300TxDescriptorEvm1(void *block)
{
	block=0;
	return 0;
}
#endif

AR9300DLLSPEC unsigned char Ar9300TxDescriptorDone(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return (unsigned char)dr->done;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorFrameTxOk(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return (unsigned char)dr->frm_xmit_ok;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorBaStatus(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return (unsigned char)dr->ba_status;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorExcessiveRetries(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return (unsigned char)dr->excessive_retries;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorFifoUnderrun(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return (unsigned char)dr->fifo_underrun;
	}
	return 0;
}

#ifdef UNUSED
AR9300DLLSPEC unsigned char Ar9300TxDescriptorFiltered(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->filtered;
	}
	return 0;
}
#endif

AR9300DLLSPEC unsigned int Ar9300TxDescriptorRtsFailCount(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return dr->rts_fail_cnt;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorDataFailCount(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return dr->data_fail_cnt;
	}
	return 0;
}

#ifdef UNUSED
AR9300DLLSPEC unsigned int Ar9300TxDescriptorVirtualRetryCount(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->virtual_retry_cnt;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorUnderrunError(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tx_dlimitr_underrun_err;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorConfigError(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->desc_config_error;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorPostTxTimerExpired(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tx_timer_expired;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorPowerManagement(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->pwr_management;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorTxOpExceeded(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->txop_exceeded;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorFinalTxIndex(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->final_tx_index;
	}
	return 0;
}

AR9300DLLSPEC unsigned char Ar9300TxDescriptorTid(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->tid;
	}
	return 0;
}
#endif	

AR9300DLLSPEC unsigned int Ar9300TxDescriptorSendTimestamp(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return dr->send_timestamp;
	}
	return 0;
}

#ifdef UNUSED
AR9300DLLSPEC unsigned int Ar9300TxDescriptorSeqnum(void *block)
{
	struct Ar9300TxDescriptor *dr=(struct Ar9300TxDescriptor *)block;

	if(dr!=0)
	{
		return dr->seqnum;
	}
	return 0;
}
#endif

AR9300DLLSPEC unsigned int Ar9300TxDescriptorBaBitmapLow(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return dr->ba_bitmap_0_31;
	}
	return 0;
}

AR9300DLLSPEC unsigned int Ar9300TxDescriptorBaBitmapHigh(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;

	if(Ar9300TxDescriptorStatusValid(block))
	{
		return dr->ba_bitmap_32_63;
	}
	return 0;
}

//
// setup a tx status descriptor with the standard required fields
//
AR9300DLLSPEC int Ar9300TxDescriptorStatusSetup(void *block)
{
	struct Ar9300TxDescriptorStatus *dr=(struct Ar9300TxDescriptorStatus *)block;
    memset(dr, 0, sizeof(struct Ar9300TxDescriptorStatus));
	return 0;
}

AR9300DLLSPEC int Ar9300TxDescriptorReset(void *block)
{
	return Ar9300TxDescriptorStatusSetup(block);
}

AR9300DLLSPEC unsigned short Ar9300TxDescriptorChecksum(void *block)
{
	unsigned int *word=block;
	int it;
	unsigned int checksum;

	checksum=0;
	for(it=0; it<10; it++)
	{
		checksum+=word[it];
	}
	checksum = ((checksum&0xffff) + (checksum>>16))&0xffff;

	return (unsigned short)checksum;
}


//
// setup a descriptor with the standard required fields
//
AR9300DLLSPEC int Ar9300TxDescriptorSetup(void *block, 
	unsigned int link_ptr, 
	unsigned int buf_ptr, int buf_len,
	int broadcast, int retry,
	int rate, int ht40, int shortGi, unsigned int txchain,
	int nagg, int moreagg,
	int id)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;
    memset(dr, 0, sizeof(struct Ar9300TxDescriptorControl));

	if(nagg<=1)
	{
		nagg=0;
	}
	//
	// special control values. must always be set this way.
	//
	dr->desc_length=0x19;
    dr->desc_ctrl_stat=1;	
    dr->desc_tx_rx=1;
	dr->atheros_id=0x168c;
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

	dr->frame_length=buf_len;       // +4;				// what is this for?
	dr->clear_dest_mask=1;

	dr->agg_length=(nagg*(4+dr->frame_length));

	dr->buf_len=buf_len;
	dr->dest_index= 0;						// (broadcast?0:4);		// huh??
	dr->no_ack= broadcast;
	dr->insert_timestamp=1;					// I don't know why
	dr->more_agg=moreagg&(nagg>1);
	dr->is_agg=(nagg>1);

	if(retry<1)
	{
		retry=1;
	}
	if(retry>15)
	{
		retry=15;
	}
	dr->tx_tries0=retry;

	dr->tx_rate0=rateValues[rate];

	dr->h20_40_0=IS_HT40_RATE_INDEX(rate);
	dr->gi_0=shortGi;
	if((!IS_3STREAM_RATE_INDEX(rate)) && txchain==0x7 && Ar9300ChainMaskReduceGet() /* FIX LATER&& CardFrequency()>=4000 */)
	{
		dr->chain_sel_0=0x3;
	}
	else
	{
		dr->chain_sel_0=txchain;
	}
	dr->ldpc=0;								// ????

	dr->tx_desc_id=id;

	dr->ptr_checksum=Ar9300TxDescriptorChecksum(block);

	return 0;
}

#ifdef UNUSED
//
// reset the descriptor so that it can be used again
//
AR9300DLLSPEC int Ar9300TxDescriptorReset(void *block)
{  
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;
	//
	// clear the status fields, leave the control fields unchanged
	//
}
#endif

//
// return the size of a descriptor 
//
AR9300DLLSPEC int Ar9300TxDescriptorControlSize()
{
   return sizeof(struct Ar9300TxDescriptorControl);
}

//
// return the size of a descriptor 
//
AR9300DLLSPEC int Ar9300TxDescriptorStatusSize()
{
   return sizeof(struct Ar9300TxDescriptorStatus);
}

//
// return the size of a descriptor 
//
AR9300DLLSPEC int Ar9300TxDescriptorSize()
{
   return Ar9300TxDescriptorControlSize();
}

AR9300DLLSPEC int Ar9300TxDescriptorPAPDSetup(void *block, int chainNum)
{
	struct Ar9300TxDescriptorControl *dr=(struct Ar9300TxDescriptorControl *)block;
	dr->pa_predistortion_chain_mask = 1<<chainNum;
	return 0;
}

static struct _TxDescriptorFunction _Ar9300TxDescriptor=
{
    Ar9300TxDescriptorLinkPtrSet,
    Ar9300TxDescriptorTxRateSet,
    Ar9300TxDescriptorBaStatus,
    Ar9300TxDescriptorAggLength,
    Ar9300TxDescriptorBaBitmapLow,
    Ar9300TxDescriptorBaBitmapHigh,
    Ar9300TxDescriptorFifoUnderrun,
    Ar9300TxDescriptorExcessiveRetries,
    Ar9300TxDescriptorRtsFailCount,
    Ar9300TxDescriptorDataFailCount,
    Ar9300TxDescriptorLinkPtr,
    Ar9300TxDescriptorBufPtr,
    Ar9300TxDescriptorBufLen,
    Ar9300TxDescriptorIntReq,
    Ar9300TxDescriptorRssiCombined,
    Ar9300TxDescriptorRssiAnt00,
    Ar9300TxDescriptorRssiAnt01,
    Ar9300TxDescriptorRssiAnt02,
    Ar9300TxDescriptorRssiAnt10,
    Ar9300TxDescriptorRssiAnt11,
    Ar9300TxDescriptorRssiAnt12,
    Ar9300TxDescriptorTxRate,
    Ar9300TxDescriptorDataLen,
    0,										// Ar9300TxDescriptorMore,
    0,										// Ar9300TxDescriptorNumDelim,
    Ar9300TxDescriptorSendTimestamp,
    0,										// Ar9300TxDescriptorGi,
    0,										// Ar9300TxDescriptorH2040,
    0,										// Ar9300TxDescriptorDuplicate,
    0,										// Ar9300TxDescriptorTxAntenna,
    0,										// Ar9300TxDescriptorEvm0,
    0,										// Ar9300TxDescriptorEvm1,
    0,										// Ar9300TxDescriptorEvm2,
    Ar9300TxDescriptorDone,
    Ar9300TxDescriptorFrameTxOk,
    0,										// Ar9300TxDescriptorCrcError,
    0,										// Ar9300TxDescriptorDecryptCrcErr,
    0,										// Ar9300TxDescriptorPhyErr,
    0,										// Ar9300TxDescriptorMicError,
    0,										// Ar9300TxDescriptorPreDelimCrcErr,
    0,										// Ar9300TxDescriptorKeyIdxValid,
    0,										// Ar9300TxDescriptorKeyIdx,
    Ar9300TxDescriptorMoreAgg,
    Ar9300TxDescriptorAggregate,
    0,										// Ar9300TxDescriptorPostDelimCrcErr,
    0,										// Ar9300TxDescriptorDecryptBusyErr,
    0,										// Ar9300TxDescriptorKeyMiss,
    Ar9300TxDescriptorSetup,
    Ar9300TxDescriptorStatusSetup,
    Ar9300TxDescriptorReset,
    Ar9300TxDescriptorSize,
    Ar9300TxDescriptorStatusSize,
    Ar9300TxDescriptorPrint,
    Ar9300TxDescriptorPAPDSetup,
#ifdef UNUSED
    Ar9300TxDescriptorWrite,
    Ar9300TxDescriptorRead,
#endif
};


int Ar9300TxDescriptorFunctionSelect()
{
	return TxDescriptorFunctionSelect(&_Ar9300TxDescriptor);
}

