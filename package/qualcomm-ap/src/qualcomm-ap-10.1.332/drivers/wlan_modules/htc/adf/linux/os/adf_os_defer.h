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

#ifndef _ADF_OS_DEFER_PVT_H
#define _ADF_OS_DEFER_PVT_H

#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

#include <adf_os_types.h>

typedef struct tasklet_struct  adf_os_bh_t;

#if LINUX_VERSION_CODE  <= KERNEL_VERSION(2,6,19)
typedef struct work_struct     adf_os_work_t;
#else
/**
 * wrapper around the real task func
 */
typedef struct {
    struct work_struct   work;
    adf_os_defer_fn_t    fn;
    void                 *arg;
} adf_os_work_t;

#endif

#define __adf_os_work_t     adf_os_work_t

typedef void (*adf_os_bh_fn_t)(unsigned long arg);

static inline void 
adf_os_defer_func(struct work_struct *work)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,19)
    return;
#else
    adf_os_work_t  *ctx = container_of(work, __adf_os_work_t, work);
    ctx->fn(ctx->arg);
#endif
}

static inline void 
adf_os_init_work(adf_os_handle_t    hdl,
                   adf_os_work_t      *work,
                   adf_os_defer_fn_t    func,
                   void                 *arg)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,19)
    INIT_WORK(work, func, arg);
#else
    INIT_WORK(&work->work, adf_os_defer_func);
#endif
}

static inline void  adf_os_create_bh(adf_os_handle_t  hdl,
                                     struct tasklet_struct *bh,
                                     adf_os_defer_fn_t  func,
                                     void               *arg)
{
     tasklet_init(bh, (adf_os_bh_fn_t)func, (unsigned long)arg);
}

static inline void 
adf_os_sched_work(adf_os_handle_t hdl, adf_os_work_t* work)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,19)
    schedule_work(work);
#else
    schedule_work(&work->work);
#endif
}

static inline void adf_os_sched_bh(adf_os_handle_t hdl, 
                                     struct tasklet_struct* bh)
{
    tasklet_schedule(bh);
}

static inline void 
adf_os_disable_work(adf_os_handle_t hdl, adf_os_work_t* work)
{
   /**
    * XXX:???
    */
    return;
}

static inline void 
adf_os_destroy_bh(adf_os_handle_t hdl, struct tasklet_struct *bh)
{
    tasklet_kill(bh);
}

#endif /*_ADF_OS_DEFER_PVT_H*/
