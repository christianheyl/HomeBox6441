/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_TARGET_C$
 */

#ifndef __POOL_MGR_INTERNAL_H__
#define __POOL_MGR_INTERNAL_H__

#include <queue.h>

#define POOL_MGR_NODE_INT_HDR   sizeof(void *)

typedef struct _node_t {
    STAILQ_ENTRY(_node_t)   node_entry;
}_node_t;


typedef struct pool_ctxt_t {
    int     elem_sz;
    int     num_of_elem;
    int     num_free_nodes;
    int     total_num_allocs;
    int     flags;
    void    *start_node;
    void    *end_node;
    void    *mem_start;
    STAILQ_HEAD(, _node_t)   node_list;
}POOL_CTXT;

#ifndef A_ASSERT
#define A_ASSERT    assert
#endif

#ifndef A_ALIGN_PTR_PWR2
#define A_ALIGN_PTR_PWR2(p,align) (void *)(((unsigned long)(p) + ((align)-1)) & ~((align)-1))
#endif

#endif /* __POOL_MGR_INTERNAL_H__ */
