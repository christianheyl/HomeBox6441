/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#ifndef __PAL_EVT_H__
#define __PAL_EVT_H__

void 
pal_cmd_compl_evt(AMP_DEV *Dev, A_UINT16 cmd_opcode, A_UINT8 *params, A_UINT8 param_len);

void 
pal_hw_err_evt(AMP_DEV *Dev, A_UINT8 err_code);

void 
pal_flush_occured_evt(AMP_DEV *Dev, A_UINT16 handle);

void 
pal_enhanced_flush_complete_event(AMP_DEV *dev, A_UINT16 handle);

void 
pal_cmd_status_evt(AMP_DEV *Dev, A_UINT16 cmd_opcode, A_UINT8 status);

void 
pal_chan_sel_evt(AMP_DEV *Dev, A_UINT8 phy_link_hdl);

void 
pal_phy_link_compl_evt(AMP_DEV *Dev, A_UINT8 status, A_UINT8 phy_link_hdl);

void 
pal_disconnect_phy_link_compl_evt(AMP_DEV *Dev, A_UINT8 status, A_UINT8 phy_link_hdl, A_UINT8 reason);

void 
pal_logical_link_compl_evt(AMP_DEV *Dev, A_UINT8 status, A_UINT16 logical_link, A_UINT8 phy_hdl, A_UINT8 TxID);

void 
pal_disconnect_logical_link_compl_evt(AMP_DEV *Dev, A_UINT8 status, A_UINT16 id, A_UINT8 reason);

void        
pal_flow_spec_modify_evt (AMP_DEV *Dev, A_UINT8 status, A_UINT16 handle);

void
pal_num_of_compl_data_blocks_evt(AMP_DEV *Dev, 
                                 A_UINT16 num_data_blks, A_UINT8 num_handles, 
                                 A_UINT16 *handles, A_UINT16 *num_compl_pkts, 
                                 A_UINT16 *num_compl_blks);

void
pal_num_of_compl_packet_evt(AMP_DEV *Dev, A_UINT8 num_handles, 
                            A_UINT16 *handles, A_UINT16 *num_compl_pkts);

void
pal_srm_change_compl_evt(AMP_DEV *Dev, A_UINT8 status, A_UINT8 phy_link, A_UINT8 sr_state);

void        
pal_send_event(AMP_DEV *dev, A_UINT8 *buf, A_UINT16 sz);

void
pal_amp_status_changed_evt(AMP_DEV *Dev, A_UINT8 AmpStatus);

#endif /* __PAL_EVT_H__ */
