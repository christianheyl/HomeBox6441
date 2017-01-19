/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#ifndef __AMP_SECURITY_H__
#define __AMP_SECURITY_H__

#define IEEE80211_AUTH_TIMEOUT (200)
#define IEEE80211_AUTH_MAX_TRIES 3
#define IEEE80211_ASSOC_TIMEOUT (200)
#define IEEE80211_ASSOC_MAX_TRIES 3
#define IEEE80211_MONITORING_INTERVAL (2000)
#define IEEE80211_PROBE_INTERVAL (60000)
#define IEEE80211_RETRY_AUTH_INTERVAL (1500)
#define IEEE80211_SCAN_INTERVAL (2000)
#define IEEE80211_SCAN_INTERVAL_SLOW (15000)
#define IEEE80211_IBSS_JOIN_TIMEOUT (20000)

#define IEEE80211_PROBE_DELAY (33)
#define IEEE80211_CHANNEL_TIME (33)
#define IEEE80211_PASSIVE_CHANNEL_TIME (200)
#define IEEE80211_SCAN_RESULT_EXPIRE (10000)
#define IEEE80211_IBSS_MERGE_INTERVAL (30000)
#define IEEE80211_IBSS_INACTIVITY_LIMIT (60000)

#define EAPOL_BODY_LEN_OFFSET 2
#define EAPOL_HEADER_LEN      4

#define MIC_LENGTH                    (16)
#define MIC_OFFSET			(17+32+16+8+8)

#define INDEX_COPY(dest, src, sz, buf, cnt  ) \
{\
    os_memcpy(dest, src, cnt);   \
    sz  += cnt;\
    buf += cnt;\
}\

/* ------------------Security Related Definitions --------------------------------------*/
#define KEY_REPLAY_COUNTER_START    9
#define KEY_REPLAY_COUNTER_OFFSET	16
#define NONCE_OFFSET				17
#define KEY_REPLAY_COUNTER_STR_LEN  8

void    create_auth_frame(A_UINT8 *dest, A_UINT8 *src, A_UINT8 *bssid, A_UINT8 initiator,A_UINT8 status_code, A_UINT8 *buf, A_UINT8 *sz);
void    create_assoc_frame(AMP_ASSOC_INFO* r_amp ,A_UINT8 *dest, A_UINT8 *src, A_UINT8 *bssid,A_UINT8 *buf, A_UINT8 *sz);
void    create_assoc_resp_frame(AMP_ASSOC_INFO* r_amp, A_UINT8 *dest, A_UINT8 *src, A_UINT8 *bssid, A_UINT8 *buf, A_UINT8 *sz);
void    create_eop_m1(AMP_ASSOC_INFO *r_amp, A_UINT8 *data, A_UINT8 *sz);
void    parse_m1_create_m2(AMP_ASSOC_INFO *r_amp, A_UINT8 *data, A_UINT8 *buf, A_UINT16 len, A_UINT8 *sz);
void    create_eop_m3(AMP_ASSOC_INFO *r_amp, A_UINT8* data, A_UINT8 *sz);
void    decrypt_key(AMP_ASSOC_INFO *r_amp, A_UINT8 *encrypted_data, A_UINT8 keydatalen);
void    generate_ptk(A_UINT8 *pmk, A_UINT8 * n1, A_UINT8 *n2, A_UINT8 *addr1, A_UINT8 *addr2, A_UINT8 *ptk);
A_UINT8 verify_mic(A_UINT8 *buf, A_UINT8 len, A_UINT8 *mic, A_UINT8 *kck);
A_UINT8 create_encrypted_key(AMP_ASSOC_INFO *r_amp, A_UINT16 gtk_len, A_UINT8 *result);
A_UINT8 parse_and_decode_m2(AMP_ASSOC_INFO *r_amp, A_UINT8 *buf, A_UINT8 len, A_UINT8 *mic);
A_UINT8 parse_and_decode_m3(AMP_ASSOC_INFO *r_amp, A_UINT8 *buf, A_UINT8 *mic, A_UINT8 *replay_counter);
A_UINT8 parse_m3_create_m4(AMP_ASSOC_INFO *r_amp, A_UINT8 *data, A_UINT8 *buf, A_UINT16 len, A_UINT8 *sz, A_UINT8 *mic);
#endif
