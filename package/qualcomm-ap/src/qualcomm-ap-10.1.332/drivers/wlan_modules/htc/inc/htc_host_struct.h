/*
 * Copyright (c) 2010, Atheros Communications Inc.
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

#ifndef _HTC_HOST_STRUCT_H
#define  _HTC_HOST_STRUCT_H

typedef struct _HTC_HOST_TGT_MIB_STATS {
    u_int32_t   tx_shortretry;
    u_int32_t   tx_longretry;
    u_int32_t   tx_xretries;
    u_int32_t   ht_tx_unaggr_xretry;   // Exceed Retry
    u_int32_t   ht_tx_xretries;
    u_int32_t   tx_pkt;
    u_int32_t   tx_aggr;
    u_int32_t   tx_retry;
    u_int32_t   txaggr_retry;
    u_int32_t   txaggr_sub_retry;
} HTC_HOST_TGT_MIB_STATS;

typedef struct _HTC_HOST_TGT_RATE_INFO {
    u_int32_t   txRateKbps;
    struct {
        u_int8_t  rssiThres;
        u_int8_t  per;
    } RateCtrlState;
} HTC_HOST_TGT_RATE_INFO;


#endif 
