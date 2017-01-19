/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_TARGET_C$
 */

#if defined(ATH_TARGET)
#include <athdefs.h>
#include <osapi.h>
/* TODO: Does pool mgr require destroy support ?? */
#define ALLOCATOR_IS_SIMPLE /* no free, auto-cleared, no failure possible */
#else /* Linux */
#include <a_types.h>
#include <a_osapi.h>
#undef ALLOCATOR_IS_SIMPLE /* supports free, no auto-clear, failure possible */
#endif
#include <queue.h>

#include "pool_mgr_api.h"
#include "pool_mgr_internal.h"


void * 
_pool_init(unsigned int elem_sz, unsigned int num_of_elem, unsigned int flags)
{
    POOL_CTXT *ctxt = NULL;
    void    *block;
    unsigned int total_sz, i;

#if defined(ALLOCATOR_IS_SIMPLE)
    int which_arena = flags & POOL_IRAM_ALLOC ? ALLOCRAM_IRAM_ARENA : ALLOCRAM_DEFAULT_ARENA;

    ctxt = (POOL_CTXT *)A_ALLOCRAM_BY_ARENA(which_arena, sizeof(POOL_CTXT));
#else
    ctxt = (POOL_CTXT *)A_MALLOC(sizeof(POOL_CTXT));
    if (ctxt) {
        A_MEMSET(ctxt, 0, sizeof(*ctxt));
    } else {
        return NULL;
    }
#endif

    STAILQ_INIT(&ctxt->node_list);

    ctxt->elem_sz = elem_sz;
    ctxt->num_of_elem = num_of_elem;
    ctxt->flags = flags;

    /* There is an argument in favor and against allocating
     * memory in one contiguous block and diving it up in to
     * smaller nodes. A contiguous block may not be available
     * but smaller blocks could be. For now allocate in one 
     * block
     */
    total_sz = num_of_elem * elem_sz;
    if (flags & POOL_MEM_BLOCK_ALIGNED) {
        A_ASSERT((((int)elem_sz) & (((int)elem_sz) - 1) ) == 0);
        total_sz += elem_sz;
    }
#if defined(ALLOCATOR_IS_SIMPLE)
    block = (void *)A_ALLOCRAM_BY_ARENA(which_arena, total_sz);
#else
    block = (void *)A_MALLOC(total_sz);
    if (block) {
        A_MEMSET(block, 0, total_sz);
    } else {
        A_FREE(ctxt);
        return NULL;
    }
#endif

    ctxt->mem_start = block;
    ctxt->start_node = block;
    if (flags & POOL_MEM_BLOCK_ALIGNED) {
        ctxt->start_node = (void *)A_ALIGN_PTR_PWR2(block, elem_sz);
    }
    ctxt->end_node = (void *)((char *)ctxt->start_node + 
                        elem_sz * (num_of_elem - 1));

    for (i = 0; i < num_of_elem; i++) {
        pool_free(ctxt, (void *)((char *)ctxt->start_node + i*ctxt->elem_sz));
    }

    return (void *)ctxt;
}


void   
_pool_free(void *pool_ctxt, void *node)
{
    POOL_CTXT *ctxt = (POOL_CTXT *)pool_ctxt;

    if (node < ctxt->start_node || node > ctxt->end_node) {
        A_ASSERT(0);
    }

    if (ctxt->flags & POOL_CLEAR_MEM_ON_FREE) {
        A_MEMSET(node, 0, ctxt->elem_sz);
    }

    STAILQ_INSERT_TAIL(&(ctxt->node_list), (_node_t *)node, node_entry);
    ctxt->num_free_nodes++;
}


void *
_pool_alloc(void *pool_ctxt)
{
    POOL_CTXT *ctxt = (POOL_CTXT *)pool_ctxt;
    void *block = NULL;

    if ((block = STAILQ_FIRST(&(ctxt->node_list))) != NULL) {
        STAILQ_REMOVE_HEAD(&ctxt->node_list, node_entry);
        /* If we clearing the memory for the block, clear the
         * space which is used internally by the pool mgr
         * before allocating to upper layers */
        if (ctxt->flags & POOL_CLEAR_MEM_ON_FREE) {
            A_MEMSET(block, 0, sizeof(_node_t *));
        }
        ctxt->total_num_allocs++;
        ctxt->num_free_nodes--;
    }

    return block;
}

A_UINT16
_pool_node_to_id(void *pool_ctxt, void *node)
{
    POOL_CTXT *ctxt = (POOL_CTXT *)pool_ctxt;
    unsigned byte_offset;

    byte_offset = ((A_UINT8 *) node) - ((A_UINT8 *) ctxt->start_node);
    return byte_offset / ctxt->elem_sz;
}

void *
_pool_node_from_id(void *pool_ctxt, A_UINT16 node_id)
{
    POOL_CTXT *ctxt = (POOL_CTXT *)pool_ctxt;

    return ((A_UINT8 *) ctxt->start_node) + (node_id * ctxt->elem_sz);
}

void
_pool_destroy(void *ctxt)
{
#if defined(ALLOCATOR_IS_SIMPLE)
    A_ASSERT(0);
#else
    POOL_CTXT *p_ctxt = (POOL_CTXT *)ctxt;

    if (p_ctxt->num_of_elem != p_ctxt->num_free_nodes) {
        /* There is a leak; Assert! */
        A_ASSERT(0);
    }

    A_FREE(p_ctxt->mem_start);
    A_FREE(ctxt);
#endif
}

#if defined(ATH_TARGET) && ! defined(DISABLE_FUNCTION_INDIRECTION)
POOL_MGR_API_FN  _pool_mgr_fn = {
    _pool_init,
    _pool_alloc,
    _pool_free,
    _pool_destroy,
    _pool_node_to_id,
    _pool_node_from_id,
};
#endif
