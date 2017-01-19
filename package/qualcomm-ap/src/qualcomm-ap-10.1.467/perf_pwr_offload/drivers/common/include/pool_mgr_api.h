/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_TARGET_C$
 */

#ifndef __POOL_MGR_H__
#define __POOL_MGR_H__

#if defined(ATH_TARGET)
#include <osapi.h>   /* A_UINT16 */
#else
#include <a_types.h> /* A_UINT16 */
#endif


#define POOL_CLEAR_MEM_ON_FREE  0x1
#define POOL_MEM_BLOCK_ALIGNED  0x2
#define POOL_IRAM_ALLOC         0x4

#if defined(ATH_TARGET) && ! defined(DISABLE_FUNCTION_INDIRECTION)

typedef struct {
    /*
     * Function: pool_init
     *      Argument-
     *          elem_sz:        size of memory node block
     *          num_of_elem:    number of elemens of size elem_sz in the pool
     *          flags:          flags to take action on pool nodes
     *
     *      Pool init builds on underlying malloc and free api's. It will
     *      try to allocate all the nodes and keep them in the pool. In
     *      case it can't find all the memory required for pool, it will
     *      destroy and thus use no memory.
     *
     *      Return-
     *          void *: context of the pool.
     */
    void *  (*_pool_init)(unsigned int elem_sz, unsigned int num_of_elem, unsigned int flags);


    /*
     * Function: pool_alloc
     *      Argument-
     *          pool_ctxt:      context of the pool returned at init time
     *
     *      Allocates a block of memory of size `elem_sz`, which was 
     *      configured at the time of pool initialization
     *
     *      Return-
     *          void *: pointer to block of memory of size `elem_sz`
     */
    void *  (*_pool_alloc)(void *pool_ctxt);

    /*
     * Function: pool_free
     *      Argument-
     *          pool_ctxt:      context of the pool returned at init time
     *          node:           pointer to memory block.
     *
     *      After using the memory block, a module can free the memory
     *      block and return it back to pool.
     *
     *      Return-
     *          void
     */
    void    (*_pool_free)(void *pool_ctxt, void *node);

    /*
     * Function: pool_destroy
     *      Argument-
     *          pool_ctxt:      context of the pool returned at init time
     *      
     *      Memory blocks in pool are completely freed to underlying OS. The 
     *      context is freed, as well.
     *
     *      Return-
     *          void
     */
    void    (*_pool_destroy)(void *pool_ctxt);

    /*
     * Function: pool_node_id
     *      Argument-
     *          pool_ctxt:      context of the pool returned at init time
     *          node:           allocation provided by pool_alloc
     *
     *      Converts a node pointer into a 16-bit integer ID that uniquely
     *      identifies the node within the specified pool.
     *      
     *      Return-
     *          ID for the specified node w.r.t. the specified pool
     */
    A_UINT16 (*_pool_node_to_id)(void *pool_ctxt, void *node);

    /*
     * Function: pool_node_from_id
     *      Argument-
     *          pool_ctxt:      context of the pool returned at init time
     *          node_id:        unique specifier of the node within the pool
     * 
     *      Converts a node ID into a regular pointer.
     *      Return-
     *          pointer to the specified node
     */
    void *   (*_pool_node_from_id)(void *pool_ctxt, A_UINT16 node_id);
} POOL_MGR_API_FN;

extern POOL_MGR_API_FN _pool_mgr_fn;

#define PMG_API_INDIR_FN(_x)    _pool_mgr_fn._x

#else   /* DISABLE_FUNCTION_INDIRECTION */

void *  _pool_init(unsigned int elem_sz, unsigned int num_of_elem, unsigned int flags);
void *  _pool_alloc(void *pool_ctxt);
A_UINT16  _pool_node_to_id(void *pool_ctxt, void *node);
void *  _pool_node_from_id(void *pool_ctxt, A_UINT16 node_id);
void    _pool_free(void *pool_ctxt, void *node);
void    _pool_destroy(void *pool_ctxt);

#define PMG_API_INDIR_FN(_x)    ((_x))

#endif


#define pool_init(elem_sz, num_of_elem, flags)  \
    PMG_API_INDIR_FN(_pool_init((elem_sz), (num_of_elem), (flags)))

#define pool_alloc(pool_ctxt)\
    PMG_API_INDIR_FN(_pool_alloc((pool_ctxt)))

#define pool_node_to_id(pool_ctxt, node)\
    PMG_API_INDIR_FN(_pool_node_to_id((pool_ctxt), (node)))

#define pool_node_from_id(pool_ctxt, node_id)\
    PMG_API_INDIR_FN(_pool_node_from_id((pool_ctxt), (node_id)))

#define pool_free(pool_ctxt, node)\
    PMG_API_INDIR_FN(_pool_free((pool_ctxt), (node)))

#define pool_destroy(pool_ctxt)\
    PMG_API_INDIR_FN(_pool_destroy((pool_ctxt)))

#endif /* __POOL_MGR_H__ */
