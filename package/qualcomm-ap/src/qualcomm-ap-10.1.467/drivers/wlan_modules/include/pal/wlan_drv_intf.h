/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */
#ifndef __WLAN_INTF_DRV_H_
#define __WLAN_INTF_DRV_H_

#include "amp_db.h"

void amp_phy_init();

A_UINT32 driver_set_channel(A_UINT8 ch);

A_UINT32 driver_set_power(A_UINT8 power);

A_UINT16 create_mgmt_frame(char *buf, A_UINT16 seqnum, int type, 
                char *addr1, char *addr2);

void create_80211_fr_and_send(
	 AMP_ASSOC_INFO* r_amp, A_UINT8 *acl_data, A_UINT16 len,  A_UINT8 type, A_UINT8 serviceType, A_UINT32 LogicID);

A_INT32 send_pkt_to_driver(AMP_DEV *dev, A_UINT8 *buf, A_UINT16 len);

void send_pal_link_supervision_tx(AMP_ASSOC_INFO *amp, A_UINT8 type);

void disconnect_connection(A_UINT8 * macaddr);

void send_activity_report(AMP_DEV *dev, AMP_ASSOC_INFO *r_amp, 
                A_BOOL schedule_known, A_UINT16 num_rpts, SCHEDULE *sch);

#endif  /* __WLAN_INTF_DRV_H_ */


