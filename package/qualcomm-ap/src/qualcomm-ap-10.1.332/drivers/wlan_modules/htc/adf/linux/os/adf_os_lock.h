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

#ifndef _ADF_OS_LOCK_PVT_H
#define _ADF_OS_LOCK_PVT_H

#ifndef  __i386__
#include <asm/semaphore.h>
#else
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#endif
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <adf_os_types.h>

typedef spinlock_t           adf_os_spinlock_t;
typedef struct semaphore     adf_os_mutex_t;

static inline void
adf_os_init_mutex(struct semaphore *m)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
    init_MUTEX(m);
#else
    sema_init(m, 1);
#endif
}

static inline int
adf_os_mutex_acquire(struct semaphore *m)
{

    return down_interruptible(m);

}

static inline void
adf_os_mutex_release(struct semaphore *m)
{

    up(m);

}

static inline void 
adf_os_spinlock_init(spinlock_t *lock)
{
    spin_lock_init(lock);
}

#define adf_os_spinlock_destroy adf_os_spin_lock_destroy
static inline void
adf_os_spin_lock_destroy(spinlock_t *lock)
{
}

#if 0
/*
 * Synchronous versions - only for OS' that have interrupt disable
 */
static inline void
adf_os_spin_lock_irq(spinlock_t *lock, uint32_t flags)
{
    spin_lock_irqsave(lock, flags);
}

static inline void
adf_os_spin_unlock_irq(spinlock_t *lock, uint32_t flags)
{
    unsigned long tflags = flags;
    spin_unlock_irqrestore(lock, tflags);
}
#else
#define adf_os_spin_lock_irq(_pLock, _flags)    spin_lock_irqsave(_pLock, _flags)
#define adf_os_spin_unlock_irq(_pLock, _flags)  spin_unlock_irqrestore(_pLock, _flags)
#endif

static inline void
adf_os_spin_lock_bh(adf_os_spinlock_t *lock)
{
    if (irqs_disabled()) 
        spin_lock(lock);	
    else 
        spin_lock_bh(lock);
}

static inline void
adf_os_spin_lock_bh_outline(adf_os_spinlock_t *lock)
{
	if (irqs_disabled()) 
		spin_lock(lock);	
	else 
		spin_lock_bh(lock);	    
}

static inline void
adf_os_spin_unlock_bh(adf_os_spinlock_t *lock)
{
    if (irqs_disabled()) 
        spin_unlock(lock);	
    else 
        spin_unlock_bh(lock);	
}

static inline void
adf_os_spin_unlock_bh_outline(adf_os_spinlock_t *lock)
{
    if (irqs_disabled()) 
        spin_unlock(lock);	
    else 
        spin_unlock_bh(lock);	
}

static inline a_bool_t
adf_os_spinlock_irq_exec(adf_os_handle_t  hdl, 
                           spinlock_t      *lock, 
                           adf_os_irqlocked_func_t func, 
                           void            *arg)
{
    unsigned long flags;
    a_bool_t ret;

    spin_lock_irqsave(lock, flags);
    ret = func(arg);
    spin_unlock_irqrestore(lock, flags);

    return ret;
}

#endif /*_ADF_OS_LOCK_PVT_H*/
