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
#include <ieee80211_var.h>
#include <ieee80211_defines.h>
#include <ieee80211_prdperfstats.h>

/*
 * Periodic performance measurement (currently throughput and PER).
 */


#if UMAC_SUPPORT_PERIODIC_PERFSTATS 

static OS_TIMER_FUNC(ieee80211_prdperfstats_gather);



void ieee80211_prdperfstats_attach(struct ieee80211com *ic)
{
    IEEE80211_PRDPERFSTATS_LOCK_INIT(ic);

    OS_INIT_TIMER(ic->ic_osdev,
                  &(ic->ic_prdperfstats_timer),
                  ieee80211_prdperfstats_gather,
                  (void *) (ic));
    
    ieee80211_prdperfstat_thrput_attach(ic);
    ieee80211_prdperfstat_per_attach(ic);
}

void ieee80211_prdperfstats_detach(struct ieee80211com *ic)
{
    /* Shut down all measurements, timers, etc */

    IEEE80211_PRDPERFSTATS_THRPUT_LOCK(ic);
    if (ic->ic_thrput.is_enab) {
        ieee80211_prdperfstat_thrput_disable(ic);
    }
    IEEE80211_PRDPERFSTATS_THRPUT_UNLOCK(ic);

    IEEE80211_PRDPERFSTATS_PER_LOCK(ic);
    if (ic->ic_per.is_enab) {
        ieee80211_prdperfstat_per_disable(ic);
    }
    IEEE80211_PRDPERFSTATS_PER_UNLOCK(ic);

    IEEE80211_PRDPERFSTATS_LOCK(ic);
    ieee80211_prdperfstats_signal(ic);
    IEEE80211_PRDPERFSTATS_UNLOCK(ic);

    ieee80211_prdperfstat_thrput_detach(ic);
    ieee80211_prdperfstat_per_detach(ic);
    
    OS_FREE_TIMER(&ic->ic_prdperfstats_timer);

    IEEE80211_PRDPERFSTATS_LOCK_DESTROY(ic);
}

void ieee80211_prdperfstats_start(struct ieee80211com *ic)
{
    OS_SET_TIMER(&ic->ic_prdperfstats_timer,
                 PRDPERFSTATS_PERIOD_MS);

    /* Since this is a shared timer, and enabling/disabling 
       of measurement is user driven, actual measurement begins
       only on the next invocation of the timer handler,
       for synchronization purposes. */
}

void ieee80211_prdperfstats_stop(struct ieee80211com *ic)
{
    OS_CANCEL_TIMER(&(ic->ic_prdperfstats_timer));
}

void ieee80211_prdperfstats_signal(struct ieee80211com *ic)
{
    u_int8_t is_started_new = 0;

    is_started_new   = (ic->ic_thrput.is_enab || ic->ic_per.is_enab);

    if (is_started_new != ic->ic_prdperfstats_is_started) {
        is_started_new? \
            ieee80211_prdperfstats_start(ic) : ieee80211_prdperfstats_stop(ic);

        ic->ic_prdperfstats_is_started = is_started_new;
    }
}

/*
 * Periodically gather performance statistics. Currently,
 * we measure throughput and PER over a time window.
 */
static OS_TIMER_FUNC(ieee80211_prdperfstats_gather)
{
    struct ieee80211com *ic;
    OS_GET_TIMER_ARG(ic, struct ieee80211com *);

    IEEE80211_PRDPERFSTATS_THRPUT_LOCK(ic);
    if (ic->ic_thrput.is_enab) {
        if (unlikely(!ic->ic_thrput.is_started)) { 
            ieee80211_prdperfstat_thrput_start(ic);
        } else {
            ic->ic_thrput.timer_count++;
            
            if (ic->ic_thrput.timer_count == PRDPERFSTAT_THRPUT_INTERVAL_MULT)
            {
                ieee80211_prdperfstat_thrput_update_hist(ic);
                ic->ic_thrput.timer_count = 0;
            }
        }
    }
    IEEE80211_PRDPERFSTATS_THRPUT_UNLOCK(ic);
    
    IEEE80211_PRDPERFSTATS_PER_LOCK(ic);
    if (ic->ic_per.is_enab) {
        if (unlikely(!ic->ic_per.is_started)) { 
            ieee80211_prdperfstat_per_start(ic);
        } else {
            ic->ic_per.timer_count++;
            
            if (ic->ic_per.timer_count == PRDPERFSTAT_PER_INTERVAL_MULT)
            {
                ieee80211_prdperfstat_per_update_hist(ic);
                ic->ic_per.timer_count = 0;
            }
        }
    }
    IEEE80211_PRDPERFSTATS_PER_UNLOCK(ic);

    OS_SET_TIMER(&ic->ic_prdperfstats_timer,
                 PRDPERFSTATS_PERIOD_MS);
}

void ieee80211_prdperfstat_thrput_attach(struct ieee80211com *ic)
{
   struct ieee80211_prdperfstat_thrput *thrput = &ic->ic_thrput;

   thrput->histogram_size = PRDPERFSTAT_THRPUT_DEF_HISTSIZE;
   IEEE80211_PRDPERFSTATS_THRPUT_LOCK_INIT(ic);
}

void ieee80211_prdperfstat_thrput_detach(struct ieee80211com *ic)
{
    IEEE80211_PRDPERFSTATS_THRPUT_LOCK_DESTROY(ic);
}

int ieee80211_prdperfstat_thrput_enable(struct ieee80211com *ic)
{
    struct ieee80211_prdperfstat_thrput *thrput = &ic->ic_thrput;
    
    thrput->histogram = 
                (u_int32_t *)OS_MALLOC(ic->ic_osdev,
                                       (thrput->histogram_size *
                                        sizeof(u_int32_t)),
                                       GFP_KERNEL);

    if (NULL == thrput->histogram) {
        IEEE80211_PRDPERFSTATS_DPRINTF(  \
                "%s : Failed to allocate memory for throughput histogram\n",
                __func__
                );
        thrput->is_enab         = 0;
        return -ENOMEM;
    }

    thrput->is_enab              = 1;
    
    return 0;
}

void ieee80211_prdperfstat_thrput_disable(struct ieee80211com *ic)
{
    if (ic->ic_thrput.histogram != NULL) {
        OS_FREE(ic->ic_thrput.histogram);
    }

    ic->ic_thrput.is_started       = 0;
    ic->ic_thrput.is_enab          = 0;
   
    return;
}

void ieee80211_prdperfstat_thrput_start(struct ieee80211com *ic)
{
    struct ieee80211_prdperfstat_thrput *thrput = &ic->ic_thrput;
    
    OS_MEMSET(thrput->histogram, 0, thrput->histogram_size * sizeof(u_int32_t));

    thrput->histogram_head          = 0;
    thrput->histogram_tail          = 0;
    thrput->is_histogram_full       = 0;
    thrput->histogram_bytecount     = 0;
    thrput->curr_bytecount          = 0;
    thrput->last_save_ms            = OS_GET_TIMESTAMP();
    
    thrput->timer_count             = 0;
    thrput->is_started              = 1;
}

void ieee80211_prdperfstat_thrput_update_hist(struct ieee80211com *ic)
{
    struct ieee80211_prdperfstat_thrput *thrput = &ic->ic_thrput;

    if (thrput->is_histogram_full) {
        thrput->histogram_bytecount -=
            thrput->histogram[thrput->histogram_tail]; 
        
        /* We do not use modulo since it is expensive */
        if (thrput->histogram_tail == (thrput->histogram_size - 1))
        {    
            thrput->histogram_tail = 0;
        } else {
            thrput->histogram_tail++;
        }
    }

    thrput->histogram_bytecount += thrput->curr_bytecount;
    thrput->histogram[thrput->histogram_head] = thrput->curr_bytecount; 
    thrput->curr_bytecount = 0;
     
    thrput->last_save_ms  = OS_GET_TIMESTAMP();

    /* Slide ahead */
    if (thrput->histogram_head == (thrput->histogram_size - 1)) {
        thrput->is_histogram_full = 1;
        thrput->histogram_head = 0;
    } else {
        thrput->histogram_head++;
    }
}

/* Return throughput in kbps */
u_int32_t ieee80211_prdperfstat_thrput_get(struct ieee80211com *ic)
{
    struct ieee80211_prdperfstat_thrput *thrput = &ic->ic_thrput;
    u_int32_t total_bytes          = 0;
    u_int32_t elapsed_time_ms      = 0;
    u_int32_t thrput_kbps          = 0;
    int num_histogram_entries      = 0;

    if (!thrput->is_started) {
        return 0;
    }
    
    if (unlikely(!thrput->is_histogram_full)) {
        num_histogram_entries =
            thrput->histogram_head - thrput->histogram_tail;
    } else {
        num_histogram_entries = thrput->histogram_size;
    } 
    
    total_bytes = (thrput->histogram_bytecount + thrput->curr_bytecount);

    elapsed_time_ms = (num_histogram_entries * PRDPERFSTAT_THRPUT_INTERVAL_MS) +
                      (OS_GET_TIMESTAMP() - thrput->last_save_ms);

    thrput_kbps = (total_bytes * 8)/ elapsed_time_ms;

    return thrput_kbps;
}

/* PER */

void ieee80211_prdperfstat_per_attach(struct ieee80211com *ic)
{
   struct ieee80211_prdperfstat_per *per = &ic->ic_per;

   per->histogram_size = PRDPERFSTAT_PER_DEF_HISTSIZE;
   IEEE80211_PRDPERFSTATS_PER_LOCK_INIT(ic);
}

void ieee80211_prdperfstat_per_detach(struct ieee80211com *ic)
{
    IEEE80211_PRDPERFSTATS_PER_LOCK_DESTROY(ic);
}

int ieee80211_prdperfstat_per_enable(struct ieee80211com *ic)
{
    struct ieee80211_prdperfstat_per *per = &ic->ic_per;
    
    per->histogram = 
                (struct per_components *)OS_MALLOC(ic->ic_osdev,
                                                   (per->histogram_size *
                                                   sizeof(struct per_components)),
                                                   GFP_KERNEL);

    if (NULL == per->histogram) {
        IEEE80211_PRDPERFSTATS_DPRINTF(  \
                "%s : Failed to allocate memory for PER histogram\n",
                __func__
                );
        per->is_enab = 0;
        return -ENOMEM;
    }

    per->is_enab     = 1;
    
    return 0;
}

void ieee80211_prdperfstat_per_disable(struct ieee80211com *ic)
{
    if (ic->ic_per.histogram != NULL) {
        OS_FREE(ic->ic_per.histogram);
    }
    
    ic->ic_per.is_started        = 0;
    ic->ic_per.is_enab           = 0;
   
    return;
}

void ieee80211_prdperfstat_per_start(struct ieee80211com *ic)
{
    struct ieee80211_prdperfstat_per *per = &ic->ic_per;
    
    OS_MEMSET(per->histogram,
              0,
              per->histogram_size * sizeof(struct per_components));

    per->histogram_head          = 0;
    per->histogram_tail          = 0;
    per->is_histogram_full       = 0;
    per->histogram_hw_retries    = 0;
    per->histogram_hw_success    = 0;
    per->last_save_hw_retries    = ic->ic_get_tx_hw_retries(ic);
    per->last_save_hw_success    = ic->ic_get_tx_hw_success(ic);

    per->timer_count             = 0;
    
    per->is_started              = 1;
}

void ieee80211_prdperfstat_per_update_hist(struct ieee80211com *ic)
{
    struct ieee80211_prdperfstat_per *per = &ic->ic_per;
    u_int32_t hw_retries_increment = 0;
    u_int32_t hw_success_increment = 0;
    u_int64_t hw_retries_curr = 0;
    u_int64_t hw_success_curr = 0;

    hw_retries_curr     = ic->ic_get_tx_hw_retries(ic);
    hw_success_curr     = ic->ic_get_tx_hw_success(ic);

    hw_retries_increment = hw_retries_curr-
                           per->last_save_hw_retries;

    hw_success_increment = hw_success_curr -
                           per->last_save_hw_success;

    per->last_save_hw_retries = hw_retries_curr; 
    per->last_save_hw_success = hw_success_curr; 

    if (per->is_histogram_full) {
        per->histogram_hw_retries -=
            per->histogram[per->histogram_tail].hw_retries; 
        
        per->histogram_hw_success -=
            per->histogram[per->histogram_tail].hw_success; 
        
        /* We do not use modulo since it is expensive */
        if (per->histogram_tail == (per->histogram_size - 1))
        {    
            per->histogram_tail = 0;
        } else {
            per->histogram_tail++;
        }
    }

    per->histogram_hw_retries += hw_retries_increment;
    per->histogram_hw_success += hw_success_increment;
    per->histogram[per->histogram_head].hw_retries = hw_retries_increment; 
    per->histogram[per->histogram_head].hw_success = hw_success_increment; 
     
    /* Slide ahead */
    if (per->histogram_head == (per->histogram_size - 1)) {
        per->is_histogram_full = 1;
        per->histogram_head = 0;
    } else {
        per->histogram_head++;
    }
}

/* Return PER percentage */
u_int8_t ieee80211_prdperfstat_per_get(struct ieee80211com *ic)
{
    struct ieee80211_prdperfstat_per *per = &ic->ic_per;
    u_int32_t hw_retries_increment = 0;
    u_int32_t hw_success_increment = 0;
    u_int32_t failures = 0;
    u_int32_t success = 0;

    if (!per->is_started) {
        return 0;
    }
    
    hw_retries_increment = ic->ic_get_tx_hw_retries(ic) -
                           per->last_save_hw_retries;

    hw_success_increment = ic->ic_get_tx_hw_success(ic) -
                           per->last_save_hw_success;

    failures = per->histogram_hw_retries + hw_retries_increment;
    success  = per->histogram_hw_success + hw_success_increment;

    if ((success + failures) == 0) {
        return 0;
    }

    return ((failures * 100) / (success + failures));
}

#endif /* UMAC_SUPPORT_PERIODIC_PERFSTATS */

