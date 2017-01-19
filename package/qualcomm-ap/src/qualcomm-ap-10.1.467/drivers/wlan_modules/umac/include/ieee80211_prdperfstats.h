/*
 *  Copyright (c) 2011 Qualcomm Atheros Inc.
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

#ifndef _IEEE80211_PRDPERFSTATS_H
#define _IEEE80211_PRDPERFSTATS_H

#include <ieee80211_var.h>

/*
 * Periodic performance measurement (currently throughput and PER).
 */

#if UMAC_SUPPORT_PERIODIC_PERFSTATS

#define PRDPERFSTATS_PERIOD_MS              (200)

/* Must not exceed 2^16 - 1 */ 
#define PRDPERFSTAT_THRPUT_INTERVAL_MULT    (1)

/* Interval between each throughput histogram entry.
   Must be a multiple of PRDPERFSTATS_PERIOD_MS */
#define PRDPERFSTAT_THRPUT_INTERVAL_MS      (PRDPERFSTATS_PERIOD_MS * \
                                             PRDPERFSTAT_THRPUT_INTERVAL_MULT)

/* Default throughput measuremnt window.
   Must be a multiple of PRDPERFSTAT_THRPUT_INTERVAL_MS */
#define PRDPERFSTAT_THRPUT_DEF_WINDOW_MS    \
                    (PRDPERFSTAT_THRPUT_INTERVAL_MS * 5 * 3)      
#define PRDPERFSTAT_THRPUT_MIN_WINDOW_MS    \
                    (PRDPERFSTAT_THRPUT_INTERVAL_MS * 5 * 1)
#define PRDPERFSTAT_THRPUT_MAX_WINDOW_MS    \
                    (PRDPERFSTAT_THRPUT_INTERVAL_MS * 5 * 30)

#define PRDPERFSTAT_THRPUT_DEF_HISTSIZE     \
                    (PRDPERFSTAT_THRPUT_DEF_WINDOW_MS /  \
                     PRDPERFSTAT_THRPUT_INTERVAL_MS)

/* Must not exceed 2^16 - 1 */ 
#define PRDPERFSTAT_PER_INTERVAL_MULT    (5)

/* Interval between each PER histogram entry.
   Must be a multiple of PRDPERFSTATS_PERIOD_MS */
#define PRDPERFSTAT_PER_INTERVAL_MS      (PRDPERFSTATS_PERIOD_MS * \
                                          PRDPERFSTAT_PER_INTERVAL_MULT)

/* Default PER measuremnt window.
   Must be a multiple of PRDPERFSTAT_PER_INTERVAL_MS */
#define PRDPERFSTAT_PER_DEF_WINDOW_MS    \
                    (PRDPERFSTAT_PER_INTERVAL_MS * 60 * 5)      
#define PRDPERFSTAT_PER_MIN_WINDOW_MS    \
                    (PRDPERFSTAT_PER_INTERVAL_MS * 3)
#define PRDPERFSTAT_PER_MAX_WINDOW_MS    \
                    (PRDPERFSTAT_PER_INTERVAL_MS * 60 * 15)

#define PRDPERFSTAT_PER_DEF_HISTSIZE     \
                    (PRDPERFSTAT_PER_DEF_WINDOW_MS /  \
                     PRDPERFSTAT_PER_INTERVAL_MS)


#define IEEE80211_PRDPERFSTAT_THRPUT_ADDCURRCNT(_ic, _v)                       \
                    {                                                          \
                        IEEE80211_PRDPERFSTATS_THRPUT_LOCK(_ic);               \
                        if ((_ic)->ic_thrput.is_started) {                     \
                            (_ic)->ic_thrput.curr_bytecount += (_v);           \
                        }                                                      \
                        IEEE80211_PRDPERFSTATS_THRPUT_UNLOCK(_ic);             \
                    }                                                          \

#define IEEE80211_PRDPERFSTAT_THRPUT_SUBCURRCNT(_ic, _v)                       \
                    {                                                          \
                        IEEE80211_PRDPERFSTATS_THRPUT_LOCK(_ic);               \
                        if ((_ic)->ic_thrput.is_started) {                     \
                            (_ic)->ic_thrput.curr_bytecount -= (_v);           \
                        }                                                      \
                        IEEE80211_PRDPERFSTATS_THRPUT_UNLOCK(_ic);             \
                    }                                                          \


#define IEEE80211_PRDPERFSTATS_LOCK_INIT(__ic)      \
            spin_lock_init(&(__ic)->ic_prdperfstats_lock)
#define IEEE80211_PRDPERFSTATS_LOCK(__ic)           \
            spin_lock(&(__ic)->ic_prdperfstats_lock)
#define IEEE80211_PRDPERFSTATS_UNLOCK(__ic)         \
            spin_unlock(&(__ic)->ic_prdperfstats_lock)
#define IEEE80211_PRDPERFSTATS_LOCK_DESTROY(__ic)   \
            spin_lock_destroy(&(__ic)->ic_prdperfstats_lock)

#define IEEE80211_PRDPERFSTATS_THRPUT_LOCK_INIT(__ic)      \
            spin_lock_init(&(__ic)->ic_thrput.lock)
#define IEEE80211_PRDPERFSTATS_THRPUT_LOCK(__ic)           \
            spin_lock(&(__ic)->ic_thrput.lock)
#define IEEE80211_PRDPERFSTATS_THRPUT_UNLOCK(__ic)         \
            spin_unlock(&(__ic)->ic_thrput.lock)
#define IEEE80211_PRDPERFSTATS_THRPUT_LOCK_DESTROY(__ic)   \
            spin_lock_destroy(&(__ic)->ic_thrput.lock)

#define IEEE80211_PRDPERFSTATS_PER_LOCK_INIT(__ic)      \
            spin_lock_init(&(__ic)->ic_per.lock)
#define IEEE80211_PRDPERFSTATS_PER_LOCK(__ic)           \
            spin_lock(&(__ic)->ic_per.lock)
#define IEEE80211_PRDPERFSTATS_PER_UNLOCK(__ic)         \
            spin_unlock(&(__ic)->ic_per.lock)
#define IEEE80211_PRDPERFSTATS_PER_LOCK_DESTROY(__ic)   \
            spin_lock_destroy(&(__ic)->ic_per.lock)

#define IEEE80211_PRDPERFSTATS_DPRINTF  printk

struct ieee80211_prdperfstat_thrput
{
    spinlock_t lock;
    u_int16_t  timer_count;

    u_int8_t   is_enab;
    u_int8_t   is_started;

    u_int32_t  *histogram;
    u_int32_t  histogram_size;
    u_int32_t  histogram_head;   /* Next position where an entry is to be
                                    made */
    u_int32_t  histogram_tail;   /* Position where the last entry was made */
    u_int8_t   is_histogram_full;

    u_int32_t  histogram_bytecount; /* Summation of all byte counts in histogram */
    u_int32_t  curr_bytecount;
    u_int64_t  last_save_ms;
};

struct per_components
{
    u_int32_t   hw_retries;
    u_int32_t   hw_success;
};

struct ieee80211_prdperfstat_per
{
    spinlock_t lock;
    u_int16_t  timer_count;

    u_int8_t   is_enab;
    u_int8_t   is_started;

    struct per_components *histogram;

    u_int32_t  histogram_size;
    u_int32_t  histogram_head;        /* Next position where an entry is to be
                                         made */
    u_int32_t  histogram_tail;        /* Position where the last entry was
                                         made */
    u_int32_t  histogram_hw_retries;  /* Summation of all hw retry counts in histogram */
    u_int32_t  histogram_hw_success;  /* Summation of all hw success counts in histogram */
    u_int64_t  last_save_hw_retries;  /* H/w retry count seen at last measurement */
    u_int64_t  last_save_hw_success;  /* H/w success count seen at last measurement */
    u_int8_t   is_histogram_full;
};

void ieee80211_prdperfstats_attach(struct ieee80211com *ic);
void ieee80211_prdperfstats_detach(struct ieee80211com *ic);
void ieee80211_prdperfstats_start(struct ieee80211com *ic);
void ieee80211_prdperfstats_stop(struct ieee80211com *ic);
void ieee80211_prdperfstats_signal(struct ieee80211com *ic);

void ieee80211_prdperfstat_thrput_attach(struct ieee80211com *ic);
void ieee80211_prdperfstat_thrput_detach(struct ieee80211com *ic);
int  ieee80211_prdperfstat_thrput_enable(struct ieee80211com *ic);
void ieee80211_prdperfstat_thrput_disable(struct ieee80211com *ic);
void ieee80211_prdperfstat_thrput_start(struct ieee80211com *ic);
void ieee80211_prdperfstat_thrput_update_hist(struct ieee80211com *ic);
u_int32_t ieee80211_prdperfstat_thrput_get(struct ieee80211com *ic);

void ieee80211_prdperfstat_per_attach(struct ieee80211com *ic);
void ieee80211_prdperfstat_per_detach(struct ieee80211com *ic);
int ieee80211_prdperfstat_per_enable(struct ieee80211com *ic);
void ieee80211_prdperfstat_per_disable(struct ieee80211com *ic);
void ieee80211_prdperfstat_per_start(struct ieee80211com *ic);
void ieee80211_prdperfstat_per_update_hist(struct ieee80211com *ic);
u_int8_t ieee80211_prdperfstat_per_get(struct ieee80211com *ic);

#else
#define IEEE80211_PRDPERFSTAT_THRPUT_ADDCURRCNT(_ic, _v)  do{} while(0)
#define IEEE80211_PRDPERFSTAT_THRPUT_SUBCURRCNT(_ic, _v)  do{} while(0)
#define ieee80211_prdperfstats_attach(_a)                 do{} while(0)
#define ieee80211_prdperfstats_detach(_a)                 do{} while(0)
#endif /* UMAC_SUPPORT_PERIODIC_PERFSTATS */


#endif /* _IEEE80211_PRDPERFSTATS_H */
