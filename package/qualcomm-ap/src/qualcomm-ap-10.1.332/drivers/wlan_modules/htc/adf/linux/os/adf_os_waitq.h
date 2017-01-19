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

#ifndef __ADF_OS_WAITQ_PVT_H
#define __ADF_OS_WAITQ_PVT_H

#include <linux/wait.h>

typedef struct __adf_os_waitq {
    wait_queue_head_t    waitq;
    volatile int         cond;
} adf_os_waitq_t;

/**
 * @brief Initialize the waitq
 * 
 * @param wq
 */
static inline void
adf_os_init_waitq(adf_os_waitq_t *wq)
{
    init_waitqueue_head(&wq->waitq);
    wq->cond = 1;
}

/**
 * @brief Sleep on the waitq, the thread is woken up if somebody
 *        wakes it or the timeout occurs, whichever is earlier
 * @Note  Locks should be taken by the driver
 * 
 * @param wq
 * @param timeout
 * 
 * @return a_status_t
 */
static inline a_status_t
adf_os_sleep_waitq(adf_os_waitq_t *wq, uint32_t timeout)
{
    volatile int cond;
    int count =0;

    if(in_irq())
      panic("Called from HARD IRQ\n");

    if ( wq->cond == 0 ) {
        printk("Wakeup before wait!\n");
        goto exit;
    }

    wq->cond = 1;
    do
    {
        cond = wq->cond;
#if 1 
        count++;
        if ( count == 8000000 )
        {
            printk(KERN_ERR "wait in %s, in_interrupt: %d\n",
                __FUNCTION__, (int)in_interrupt());
            dump_stack();
        }
#endif
    }
    while(cond);

    if ( count == 8000000 )
        ;//dump_stack();

exit:
    wq->cond = 1;
//    return (wait_event_timeout(wq->waitq, wq->cond, timeout) < 0);
    return 0;
}

/**
 * @brief Wake the first guy sleeping in the queue
 * 
 * @param wq
 * 
 * @return a_status_t
 */
static inline a_status_t
adf_os_wake_waitq(adf_os_waitq_t *wq)
{
    if(!in_interrupt())
      dump_stack();

    wq->cond = 0;
//    wake_up(&wq->waitq);

    return A_STATUS_OK;
}

static inline a_status_t
adf_os_reset_waitq(adf_os_waitq_t *wq)
{
    return A_STATUS_OK;
    
    #if 0
#if (WQ_METHOD == WQ_EVENT)
    KeClearEvent(&wq->wqEvent);

    return A_STATUS_OK;
#else
    //TODO: unfinish
    adf_os_assert(0);

    return A_STATUS_OK;
#endif
    #endif
}

#endif
