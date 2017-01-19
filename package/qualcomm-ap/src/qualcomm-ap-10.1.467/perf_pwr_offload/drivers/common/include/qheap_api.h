/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_TARGET_C$
 */

#ifndef __QHEAP_H__
#define __QHEAP_H__

struct qheap_ctrl;
typedef struct qheap_ctrl *qheap_handle_t;

#if defined(ATH_TARGET) && ! defined(DISABLE_FUNCTION_INDIRECTION)

/**
 * qheap - a memory management scheme for users who make variable sized
 * allocations (heap), and free the allocations in the same order they
 * were allocated (queue).
 */
typedef struct {
    /**
     * @brief Create a queue-heap of the specified size.
     *
     * @description
     *    Create a queue-heap and return a handle to it.
     *    The handle is used to identify the queue-heap during
     *    allocations and deallocations, and also when deleting
     *    the queue-heap.
     *
     * @param bytes - the desired size of the queue-heap
     * @return
     *    success: handle to queue-heap
     *    failure: NULL
     */
    void *(*_qheap_init)(int bytes);

    /*
     * @brief Allocate a buffer from the queue-heap.
     *
     * @description
     *    Allocate a buffer of the specified size from within the
     *    specified queue-heap.  This buffer needs to be deallocated
     *    via a call to qheap_free() in the same order that it was
     *    allocated.
     *    The requested allocation size is rounded up to an allocation quantum.
     *
     * @param qh - handle to the queue-heap making the allocation
     * @param bytes - how large the allocation should be
     * @return
     *    success: pointer to the allocated buffer
     *    failure: NULL
     */
    char *(*_qheap_alloc)(qheap_handle_t qh, int bytes);

    /*
     * @brief Current maximum allowed buffer size to allocate
     *
     * @description
     *    The maximum allowed buffer size to allocate from the qheap now. 
     *    The very first qheap_alloc call after qheap_avail_max_buf_sz call
     *    with the return size is guaranteed to succeed.
     *
     * @param qh - handle to the queue-heap finding the maximum allowed
     *             allocation
     * @return Returns the maximum allowed number of bytes to allocate now. 
     */
    int (*_qheap_avail_max_buf_sz)(qheap_handle_t qh);

    /*
     * @brief Expand or shrink an existing queue-heap allocation.
     *
     * @description
     *    Expand or shrink the most-recent allocation to the specified
     *    amount, if possible.
     *    If the allocation has no room to expand because it has hit
     *    the tail of the queue-heap, the reallocation fails, even
     *    if there's room for a new allocation of the requested size
     *    at the head of the queue-heap.
     *    The requested expansion is rounded up to an allocation quantum.
     *    Thus, the actual expansion may be slightly larger than the
     *    requested expansion.  The actual expansion amount is returned.
     *    If the request is to shrink rather than grow the allocation,
     *    the request will succeed.
     *
     * @param qh - handle to the queue-heap making the reallocation
     * @param buf - original allocation
     * @param size - the size the allocation to expand/shrink to
     * @return Returns the number of bytes added to the allocation.
     *    Returns 0 if the reallocation failed.
     */
    int (*_qheap_realloc)(qheap_handle_t qh, char *buf, int size);

    /*
     * @brief Free an allocated buffer.
     *
     * @description
     *    Free a buffer back to the queue-heap.
     *    Buffers must be freed in the same order they were allocated.
     *
     * @param qh - handle to the queue-heap that made the allocation
     * @param buf - allocated buffer
     */
    void (*_qheap_free)(qheap_handle_t qh, char *buf);

    /*
     * @brief Delete a queue-heap
     *
     * @description
     *    Delete a queue-heap created by qheap_init.
     *    The queue-heap must have no outstanding allocations when it is
     *    destroyed.
     *
     * @param qh - handle to the queue-heap to destroy
     */
    void (*_qheap_destroy)(qheap_handle_t qh);
} QHEAP_API_FN;

extern QHEAP_API_FN _qheap_fn;

#define QHEAP_API_INDIR_FN(_x)    _qheap_fn._x

#else /* FUNCTION_INDIRECTION */
    void *_qheap_init(int bytes);
    char *_qheap_alloc(qheap_handle_t qh, int bytes);
    int *_qheap_avail_max_buf_sz(qheap_handle_t qh);
    int _qheap_realloc(qheap_handle_t qh, char *buf, int extra_bytes);
    void _qheap_free(qheap_handle_t qh, char *buf);
    void _qheap_destroy(qheap_handle_t qh);
#define QHEAP_API_INDIR_FN(_x)    ((_x))

#endif /* FUNCTION_INDIRECTION */


#define qheap_init(heap_size)  \
    QHEAP_API_INDIR_FN(_qheap_init((heap_size)))

#define qheap_alloc(qheap_handle, num_bytes)\
    QHEAP_API_INDIR_FN(_qheap_alloc((qheap_handle), (num_bytes)))

#define qheap_avail_max_buf_sz(qheap_handle)\
    QHEAP_API_INDIR_FN(_qheap_avail_max_buf_sz(qheap_handle))

#define qheap_realloc(qheap_handle, buf, extra_bytes) \
    QHEAP_API_INDIR_FN(_qheap_realloc((qheap_handle), (buf), (extra_bytes)))

#define qheap_free(qheap_handle, buf)\
    QHEAP_API_INDIR_FN(_qheap_free((qheap_handle), (buf)))

#define qheap_destroy(qheap_handle)\
    QHEAP_API_INDIR_FN(_qheap_destroy((qheap_handle)))


#endif /* __QHEAP_H__ */
