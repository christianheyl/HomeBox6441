/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_TARGET_C$
 */

#include <qheap_api.h>
#include <stdlib.h>

#if defined(ATH_TARGET)
#include <athdefs.h> /* A_UINT32 */
#include <osapi.h>   /* A_ALLOCRAM */
#define MEM_ALLOC(nbytes) A_ALLOCRAM(nbytes)
#define MEM_FREE(ptr) A_PRINTF("warning: qheap RAM free fail\n");
#else
#include <a_types.h> /* A_UINT32 */
#include <a_osapi.h> /* A_MALLOC */
#define MEM_ALLOC(nbytes) A_MALLOC(nbytes)
#define MEM_FREE(ptr) A_FREE(ptr)
#endif

#define QHEAP_WRAP_AROUND_PATTERN 0xaa55f00f

struct qheap_ctrl {
    A_UINT32 size;
    char *free;
    char *in_use;
};
#define QHEAP_CTRL_SZ sizeof(struct qheap_ctrl)
#define QHEAP_BUF(qh) (((char *)(qh)) + QHEAP_CTRL_SZ)

struct qheap_alloc_hdr {
    A_UINT32 size;
};

/*
 * extra_space is required for the qheap header and 4 bytes
 * space between in_use and free pointers to differentiate 
 * qheap is full (free + 4 == in_use) or qheap is empty 
 * (free == in_use)
 */
#define QHEAP_ALLOC_EXTRA_SZ    (sizeof(struct qheap_alloc_hdr) + 4)

/* confirm that the qheap_ctrl struct's size is a multiple of 4B */
A_COMPILE_TIME_ASSERT(qheap_ctrl_size_quantum,
    (sizeof(struct qheap_ctrl) & 0x3) == 0);

/* confirm that the qheap_alloc_hdr struct's size is a multiple of 4B */
A_COMPILE_TIME_ASSERT(qheap_alloc_hdr_size_quantum,
    (sizeof(struct qheap_alloc_hdr) & 0x3) == 0);

#define QHEAP_SAFE 1
#if QHEAP_SAFE >= 1
    #define QHEAP_ASSERT1(condition) A_ASSERT(condition)
    #if QHEAP_SAFE >= 2
        #define QHEAP_ASSERT2(condition) A_ASSERT(condition)
    #else
        #define QHEAP_ASSERT2(condition)
    #endif
#else
    #define QHEAP_ASSERT1(condition)
    #define QHEAP_ASSERT2(condition)
#endif
#define QHEAP_ASSERT0(condition) A_ASSERT(condition)

#ifndef QHEAP_DEBUG
#define QHEAP_DEBUG 0
#endif
#if QHEAP_DEBUG
    #include <stdio.h>
    #define QHEAP_DPRINTF(fmt, ...) printf(fmt, ## __VA_ARGS__)
#else
    #define QHEAP_DPRINTF(fmt, ...)
#endif

#define ROUND_TO_4(n) A_ROUND_UP_PWR2((n), 4)

void *_qheap_init(int bytes)
{
    struct qheap_ctrl *qh;

    QHEAP_ASSERT2(bytes > 0);

    bytes = ROUND_TO_4(bytes);

    /* allocate enough mem for both the control struct and the heap-buffer */
    qh = MEM_ALLOC(QHEAP_CTRL_SZ + bytes);
    if (!qh) {
        return NULL; /* allocation failure */
    }
    /* verify that the underlying allocation has 4B alignment */
    QHEAP_ASSERT1((((unsigned) qh) & 0x3) == 0);

    qh->size = bytes;
    qh->free = QHEAP_BUF(qh);
    /*
     * Initialize in_use to just beyond the end of the buffer rather than
     * index 0 within the buffer.  This simplifies the wrap-around logic,
     * because then in_use > free, and the space available can be simply
     * computed as in_use - free - 1, without worrying about wraparound.
     * Else, if in_use is at index 0 rather than index size, the wraparound
     * logic kicks in, and if there's not enough room at the tail of the
     * buffer an extra check would be required to determine that the number
     * of bytes free at the head of the buffer is actually 0.
     */
    qh->in_use = qh->free + qh->size - sizeof(A_UINT32);
    *((A_UINT32 *) qh->in_use) = QHEAP_WRAP_AROUND_PATTERN;
    QHEAP_DPRINTF("created queue-heap of %d bytes at %p\n", qh->size, qh);
    return qh;
}

int _qheap_avail_max_buf_sz(qheap_handle_t qh)
{
    /* check if wraparound applies */
    if (qh->in_use <= qh->free) {
        int space_to_end, space_at_head;

        space_to_end = QHEAP_BUF(qh) + qh->size - qh->free - 
            QHEAP_ALLOC_EXTRA_SZ;
        space_at_head = qh->in_use - QHEAP_BUF(qh) -
            QHEAP_ALLOC_EXTRA_SZ; 
        return space_to_end > space_at_head ? space_to_end : space_at_head;
    } else {
        return (qh->in_use - qh->free - QHEAP_ALLOC_EXTRA_SZ); 
    }
}

char *_qheap_alloc(qheap_handle_t qh, int bytes)
{
    char *buf;
    int space;

    QHEAP_ASSERT2(qh);        /* verify the queue-heap is valid */
    QHEAP_ASSERT2(bytes > 0); /* verify the request is valid */

    QHEAP_DPRINTF(
        "qh %p: alloc request: %d bytes (free idx = %d, in-use idx = %d)\n",
        qh, bytes,
        qh->free - QHEAP_BUF(qh),
        qh->in_use - QHEAP_BUF(qh));

    bytes = ROUND_TO_4(bytes);
    bytes += sizeof(struct qheap_alloc_hdr);

    /* check if wraparound applies */
    if (qh->in_use <= qh->free) {
       int space_to_end;
       char *base = QHEAP_BUF(qh);
       space_to_end = base + qh->size - qh->free;
       /*
        * Allocate from the head of the buffer if there's either insufficient
        * space (space_to_end < bytes) at the tail (duh!) or if there's
        * just-sufficient space (space_to_end == bytes) at the tail.
        * In the just-sufficient case, we could allocate from the tail, but
        * then we'd have to do an extra check below to wrap around qh->free.
        * By not using the last byte (actually last 4 bytes) of the buffer,
        * the wraparound check done here is the only check required.
        */
       if (space_to_end > bytes) {
           buf = qh->free;
       } else {
           /*
            * space_to_end is guaranteed to be at least 4.
            * It's > 0 because the above "space_to_end > bytes" check
            * (rather than "space_to_end >= bytes") for the prior
            * allocation would not have placed the prior allocation at
            * the end of the buffer unless there were extra space left
            * beyond the prior allocation.
            * Furthermore, space_to_end is at least 4 because allocations
            * use a 4-byte quantum.
            * Thus, it's safe to store a 4-byte pattern at *qh->free.
            * This pattern is used during wraparound between allocations
            * to confirm that buffers are freed in the same order they
            * were allocated.
            */
           *((A_UINT32 *) qh->free) = QHEAP_WRAP_AROUND_PATTERN;
           buf = base;
       }
    } else {
        buf = qh->free;
    }
    space = qh->in_use - buf - 1;
    if (space < 0) {
        space += qh->size;
    }
    if (space < bytes) {
        QHEAP_DPRINTF("qh %p: insufficient space (%d, %d)\n", qh, space, bytes);
        return NULL;
    }

    /* remember the allocation size */
    ((struct qheap_alloc_hdr *) buf)->size = bytes;

    /*
     * Advance the free pointer beyond this new allocation.
     * Wraparound is automatic based on the conditional choice of
     * whether to assign buf to QHEAP_BUF(qh) or qh->free.
     */
    qh->free = buf + bytes;

    /*
     * Advance the buffer pointer beyond the header, to get to the part
     * of the buffer usable by the caller.
     */
    buf += sizeof(struct qheap_alloc_hdr);

    QHEAP_DPRINTF(
        "qh %p: allocation from index %d\n",
        qh, buf - QHEAP_BUF(qh));

    return buf;
}

int _qheap_realloc(qheap_handle_t qh, char *buf, int size)
{
    int extra_bytes;

    QHEAP_ASSERT2(qh); /* verify the queue-heap is valid */

    /* verify the buffer is valid */
    QHEAP_ASSERT2(
        buf >= QHEAP_BUF(qh) &&
        buf < QHEAP_BUF(qh) + qh->size);

    buf -= sizeof(struct qheap_alloc_hdr);

    /* verify this is the most recent allocation */
    QHEAP_ASSERT2(buf + ((struct qheap_alloc_hdr *) buf)->size == qh->free);

    /* be conservative - round up to 4 octets aligned size */
    size = ROUND_TO_4(size) + sizeof(struct qheap_alloc_hdr);
    extra_bytes = size - ((struct qheap_alloc_hdr *) buf)->size;

    QHEAP_DPRINTF(
        "qh %p: change index %d alloc from %d bytes to %d\n",
        qh, buf - QHEAP_BUF(qh), size - extra_bytes, size);

    /*
     * Though there's no need to do the following checks if extra_bytes <= 0,
     * it's preferable to do them anyway, just to avoid adding an extra test
     * to see whether extra_bytes > 0.  This will slightly slow down the
     * case when extra_bytes < 0, but avoids slowing down the case when
     * extra_bytes > 0.
     */
    if (qh->in_use > buf ) {
        if (buf + size >=  qh->in_use) {
            QHEAP_DPRINTF("qh %p: realloc failure (no space) (%d >= %d)\n",
                qh, buf + size - QHEAP_BUF(qh), qh->in_use - QHEAP_BUF(qh));
            return 0;
        }
    } else {
        if (buf + size >= QHEAP_BUF(qh) + qh->size) {
            QHEAP_DPRINTF("qh %p: realloc failure (hit tail) (%d >= %d)\n",
                qh, buf + size - QHEAP_BUF(qh), qh->size);
            return 0;
        }
    }

    *((unsigned *)buf) = size;
    qh->free += extra_bytes;

    return extra_bytes;
}

void _qheap_free(qheap_handle_t qh, char *buf)
{
    int size;

    QHEAP_ASSERT2(qh); /* verify the queue-heap is valid */

    /* verify the buffer is valid */
    QHEAP_ASSERT2(
        buf >= QHEAP_BUF(qh) &&
        buf < QHEAP_BUF(qh) + qh->size);

    buf -= sizeof(struct qheap_alloc_hdr);
    size = ((struct qheap_alloc_hdr *) buf)->size;

    /*
     * Check that the qheap_free calls happen
     * in the same order as the qheap_alloc calls.
     * Usually, buf should equal in_use.
     * It's legitimate for buf to be less than in_use
     * if the allocation wrapped around (and in_use
     * is still at the tail).
     * It's illegal for buf to be more than in_use.
     */
    QHEAP_ASSERT1(
        /* usual case */
        (buf == qh->in_use)
        ||
        /* wrap-around */
        (
            /* check that this alloc is at the buffer head */
            (buf == QHEAP_BUF(qh)) &&
            /* check that the last freed alloc was at the buffer tail */
            ((*(A_UINT32 *)qh->in_use) == QHEAP_WRAP_AROUND_PATTERN)
        ));

    if (buf + size == qh->free) {
        /* No outstanding allocation. Reset the qheap state */
        qh->free = QHEAP_BUF(qh);
        qh->in_use = qh->free + qh->size - sizeof(A_UINT32);
    } else {
        qh->in_use = buf + size;
    }

    QHEAP_DPRINTF(
        "qh %p: free %d bytes from idx %d (%d) results in in_use idx %d\n",
        qh, size,
        buf - QHEAP_BUF(qh),
        buf + sizeof(struct qheap_alloc_hdr) - QHEAP_BUF(qh),
        qh->in_use - QHEAP_BUF(qh));
}

void _qheap_destroy(qheap_handle_t qh)
{
    QHEAP_ASSERT2(qh); /* verify the queue-heap is valid */

    /* verify there are no outstanding allocations */
    QHEAP_ASSERT0(
        qh->free == qh->in_use ||
        (qh->free == QHEAP_BUF(qh) && qh->in_use == qh->free + qh->size));

    QHEAP_DPRINTF("qh %p: delete queue heap", qh);

    MEM_FREE(qh);
}

#if defined(ATH_TARGET) && ! defined(DISABLE_FUNCTION_INDIRECTION)
QHEAP_API_FN  _qheap_fn = {
    _qheap_init,
    _qheap_alloc,
    _qheap_avail_max_buf_sz,
    _qheap_realloc,
    _qheap_free,
    _qheap_destroy,
};
#endif


