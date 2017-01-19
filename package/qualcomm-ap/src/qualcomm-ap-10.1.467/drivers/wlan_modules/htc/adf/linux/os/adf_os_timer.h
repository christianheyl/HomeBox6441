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

#ifndef _ADF_OS_TIMER_PVT_H
#define _ADF_OS_TIMER_PVT_H

#include <linux/delay.h>
#include <linux/timer.h>
#include <adf_os_types.h>

/* 
 * timer data type 
 */
typedef struct timer_list       adf_os_timer_t;

/*
 * ugly - but every other OS takes, sanely, a void*
 */
typedef void (*adf_dummy_timer_func_t)(unsigned long arg);

/* 
 * Initialize a timer
 */
static inline void
adf_os_timer_init(adf_os_handle_t      hdl,
                    struct timer_list   *timer,
                    void                *func,
                    void                *arg)
{
    init_timer(timer);
    timer->function = (adf_dummy_timer_func_t)func;
    timer->data = (unsigned long)arg;
}

/* 
 * start a timer 
 */
static inline void
adf_os_timer_start(struct timer_list *timer, int msec)
{
    timer->expires = jiffies + (msec * HZ / 1000);
    add_timer(timer);
}

/*
 * Cancel a timer
 *
 * Return: TRUE if timer was cancelled and deactived,
 *         FALSE if timer was cancelled but already got fired.
 */
static inline a_bool_t
adf_os_timer_cancel(struct timer_list *timer)
{
    if (likely(del_timer(timer)))
        return 1;
    else
        return 0;
}

/*
 * XXX Synchronously canel a timer
 *
 * Return: TRUE if timer was cancelled and deactived,
 *         FALSE if timer was cancelled but already got fired.
 *
 * Synchronization Rules:
 * 1. caller must make sure timer function will not use
 *    adf_os_set_timer to add iteself again.
 * 2. caller must not hold any lock that timer function
 *    is likely to hold as well.
 * 3. It can't be called from interrupt context.
 */
static inline a_bool_t
adf_os_timer_sync_cancel(struct timer_list *timer)
{
    return del_timer_sync(timer);
}

#endif /*_ADF_OS_TIMER_PVT_H*/
