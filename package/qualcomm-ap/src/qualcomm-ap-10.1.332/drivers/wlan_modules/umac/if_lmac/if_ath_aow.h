/*
 * Copyright (c) 2010 Atheros Communications, Inc.
 * All rights reserved.
 *
 */
/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */


#ifndef _ATH_AOW_H_
#define _ATH_AOW_H_

#if  ATH_SUPPORT_AOW

extern void ath_net80211_aow_send2all_nodes(void *reqvap, void *data, int len, u_int32_t seqno, u_int64_t tsf);
extern int32_t ath_net80211_aow_update_ie(ieee80211_handle_t ieee, u_int8_t* data, u_int32_t len, u_int32_t action);
extern int32_t ath_net80211_aow_update_volume(ieee80211_handle_t ieee, u_int8_t* data, u_int32_t len);
extern int32_t ath_net80211_aow_request_version(ieee80211_handle_t ieee);
extern u_int32_t ath_net80211_aow_tx_ctrl(u_int8_t* data, u_int32_t datalen, u_int64_t tsf);
extern int32_t ath_net80211_aow_send_to_host(ieee80211_handle_t ieee, u_int8_t* data, u_int32_t len, u_int16_t type, u_int16_t subtype,
u_int8_t* addr);
extern void ath_aow_attach(struct ieee80211com* ic, struct ath_softc_net80211 *scn);
extern int  ath_net80211_aow_consume_audio_data(ieee80211_handle_t ieee, u_int64_t tsf);
extern u_int32_t ath_net80211_i2s_write_interrupt(ieee80211_handle_t ieee);
extern int16_t ath_net80211_aow_apktindex(ieee80211_handle_t ieee, wbuf_t wbuf);
extern void ath_list_audio_channel(struct ieee80211com *ic);
extern void ath_net80211_aow_l2pe_record(struct ieee80211com *ic, bool is_success);
extern u_int32_t ath_net80211_aow_chksum_crc32 (unsigned char *block,
                                       unsigned int length);
extern u_int32_t ath_net80211_aow_cumultv_chksum_crc32(u_int32_t crc_prev,
                                              unsigned char *block,
                                              unsigned int length);

extern int wlan_put_pkt(struct ieee80211com *ic, char* data, int len);
extern int wlan_get_pkt(struct ieee80211com *ic, int* index);
extern int wlan_init_sync_buf(struct ieee80211com *ic);

extern int ath_aow_fill_i2s_dma(struct ieee80211com *ic);
extern int is_aow_audio_stopped(struct ieee80211com *ic);
#else   /* ATH_SUPPORT_AOW */

#define ath_aow_attach(a, b) do{}while(0)
#define ath_net80211_aow_l2pe_record(a, b) do{}while(0)
#define ath_net80211_aow_chksum_crc32(a, b) do{}while(0)
#define ath_net80211_aow_cumultv_chksum_crc32(a, b, c) do{}while(0)

#define  wlan_put_pkt(a, b, c)  do{}while(0)
#define  wlan_get_pkt(a, b)     do{}while(0)
#define  wlan_init_sync_buf(a)  do{}while(0)

#endif  /* ATH_SUPPORT_AOW */

#endif  /* _ATH_AOW_H_ */
