/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_TARGET_C$
 */

#include <stdio.h>
#include <qheap_api.h>

#define HEAP_SIZE 100
#define NUM_BUFS 10

int main(void)
{
    qheap_handle_t qh;
    char *bufs[NUM_BUFS];
    int i, j, k;
    int rep;

    qh = qheap_init(HEAP_SIZE);

#if 0
    /* negative test: destroy queue-heap with outstanding allocations */
    bufs[0] = qheap_alloc(qh, 10);
    qheap_destroy(qh);
#endif

#if 0
    /* negative test: free out of order */
    bufs[0] = qheap_alloc(qh, 10);
    bufs[1] = qheap_alloc(qh, 10);
    qheap_free(qh, bufs[1]);
#endif

    /* positive test: repeatedly make then shrink allocations */
    printf("\nTEST CASE -1: shrink allocations\n");
    for (rep = 0; rep < 3; rep++) {
        int i, blocks, initial_size, adjustment_size = 4;
        blocks = 5;
        /* initial allocation - enough to overflow */
        initial_size = (HEAP_SIZE - blocks * 4) / (blocks - 1);
        for (i = 0; i < blocks; i++) {
            bufs[i] = qheap_alloc(qh, initial_size);
            if (bufs[i]) {
                qheap_realloc(qh, bufs[i], adjustment_size); 
            }
        }
        for (i = 0; i < blocks; i++) {
            if (bufs[i]) {
                qheap_free(qh, bufs[i]);
            }
        }
    }

    /* negative test: make an initial allocation, then expand until it fails */
    printf("\nTEST CASE 0: realloc to exhaust the heap\n");
    for (rep = 0; rep < 5; rep++) {
        int extra_bytes, alloc_size = 4;
        bufs[0] = qheap_alloc(qh, alloc_size);
        do {
            extra_bytes = qheap_realloc(qh, bufs[0], ++alloc_size); 
        } while (extra_bytes > 0);
        qheap_free(qh, bufs[0]);
    }


    /* negative test: allocate until heap is exhausted */
    printf("\nTEST CASE 1: exhaust the heap\n");
    for (rep = 0; rep < 2; rep++) {
        int size = (HEAP_SIZE + NUM_BUFS) / NUM_BUFS;

        printf("REP %d\n", rep);

        i = 0;
        while (1) {
            bufs[i] = qheap_alloc(qh, size);
            if (! bufs[i]) break;
            i++;
        } 
        for (j = 0; j < i; j++) {
            qheap_free(qh, bufs[j]);
        }
    }

    /* positive test: allocate random buffer sizes */
    printf("\nTEST CASE 2: pipeline\n");
    for (rep = 0; rep < 2; rep++) {
        int idx_alloc, idx_dealloc;
        int size;

        printf("REP %d\n", rep);

        /* startup - allocate some initial buffers */
        idx_dealloc = 0;
        idx_alloc = 0;
        for (k = 0; k < 3; k++) {
            size = 10 + random() % 10;
            bufs[idx_alloc] = qheap_alloc(qh, size);
            if (++idx_alloc == NUM_BUFS) idx_alloc = 0;
        }
        /* steady-state - deallocate old, allocate new */
        for (k = 0; k < 20; k++) {
            qheap_free(qh, bufs[idx_dealloc]);
            if (++idx_dealloc == NUM_BUFS) idx_dealloc = 0;
            size = 10 + random() % 10;
            bufs[idx_alloc] = qheap_alloc(qh, size);
            if (++idx_alloc == NUM_BUFS) idx_alloc = 0;
        }
        /* dealloc remaining buffers */
        while (idx_dealloc != idx_alloc) {
            qheap_free(qh, bufs[idx_dealloc]);
            if (++idx_dealloc == NUM_BUFS) idx_dealloc = 0;
        }
    }

    /* positive test: randomly allocate / deallocate buffers */
    printf("\nTEST CASE 3: random\n");
    {
        int idx_alloc, idx_dealloc;
        int size;

        idx_alloc = 0;
        idx_dealloc = 0;

        for (k = 0; k < 1000; k++) {
            int spaces, choice;
            spaces = idx_dealloc - idx_alloc - 1;
            if (spaces < 0) spaces += NUM_BUFS;
            choice = random() & 0x1;
            if (spaces > 0 && choice == 0) {
                size = 10 + random() % 10;
                bufs[idx_alloc] = qheap_alloc(qh, size);
                if (bufs[idx_alloc]) {
                    if (++idx_alloc == NUM_BUFS) idx_alloc = 0;
                }
            } else if (spaces < NUM_BUFS - 1) {
                qheap_free(qh, bufs[idx_dealloc]);
                if (++idx_dealloc == NUM_BUFS) idx_dealloc = 0;
            }
        }    

        /* dealloc remaining buffers */
        while (idx_dealloc != idx_alloc) {
            qheap_free(qh, bufs[idx_dealloc]);
            if (++idx_dealloc == NUM_BUFS) idx_dealloc = 0;
        }
    }

    qheap_destroy(qh);

    return 0;
}
