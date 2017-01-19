/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#ifndef __MYWPA_H__
#define __MYWPA_H__

void 
wpa_pmk_to_ptk(const A_UINT8 *pmk, size_t pmk_len, const A_UINT8 *addr1, const A_UINT8 *addr2,
                    const A_UINT8 *nonce1, const A_UINT8 *nonce2, A_UINT8 *ptk, size_t ptk_len);

void wpa_eapol_key_mic(const A_UINT8 *key, int ver, const A_UINT8 *buf, size_t len, A_UINT8 *mic);

void wpa_gmk_to_gtk(A_UINT8 *gmk, A_UINT8 *addr, A_UINT8 *gnonce, A_UINT8 *gtk, size_t gtk_len);

#endif
