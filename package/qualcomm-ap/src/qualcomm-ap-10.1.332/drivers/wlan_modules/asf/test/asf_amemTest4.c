#include <amem.h>
#include <stdio.h>

struct foo {
    int a[24];
};

int try_destroy(amem_cache_handle h)
{
    enum amem_status status;
    status = amem_cache_destroy(h);
    if ( status != amem_status_success ) {
        printf("Unable to destroy memory pool: %s\n",
            amem_status_to_string(status));
    } else {
        printf("Destroyed memory pool.\n");
    }
}

float RandFract(void)
{
    return (random() & 1023) / 1024.0;
}

/* return the nearest power of 2 less than or equal to the input value */
int FloorPow2(int val)
{
    int msb = -1;
    while ( val ) {
        val >>= 1;
        msb++;
    }
    return 1 << msb;
}

#define min(a,b) ((a) < (b) ? (a) : (b))


#define DEFAULT_REPS 10
#define DEFAULT_VERB 3

#define MAX_ALLOC_ELEM 1000
#define MIN_ELEM_BYTES 1
#define MAX_ELEM_BYTES 1024
#define ELEM_BYTES_RANGE (MAX_ELEM_BYTES - MIN_ELEM_BYTES)

int main(int argc, const char **argv)
{
    int rep, reps;
    int verbosity;

    verbosity = DEFAULT_VERB;
    reps = DEFAULT_REPS;

    if ( argc > 1 ) {
        reps = atoi(argv[1]);
        if ( argc > 2 ) {
            verbosity = atoi(argv[2]);
        }
    }

    amem_sbrk(0);
    rep = 0;
    while ( reps > 0 && rep++ < reps ) {
        int i, num;
        int alloc_elem;
        int align_bytes;
        amem_size_t elem_bytes;
        void *ptrs[MAX_ALLOC_ELEM];

        alloc_elem = MAX_ALLOC_ELEM * RandFract();

        if ( verbosity > 1 ) printf("rep %d of %d\n", rep, reps);
        if ( verbosity > 2 ) amem_status_print();

        for ( i = 0, num = 0; i < alloc_elem; i++ ) {
            elem_bytes = MIN_ELEM_BYTES + ELEM_BYTES_RANGE * RandFract();
            align_bytes = min(8, FloorPow2(elem_bytes));

            ptrs[i] = amalloc(elem_bytes);

            if ( verbosity > 3 ) {
                printf("  alloc %d of %d, size = %d, align = %d (%d): %p\n",
                    i+1, alloc_elem, elem_bytes, align_bytes, num, ptrs[i]);
            }

            if ( ! ptrs[i] ) {
                printf("alloc %d failed", i);
                exit(1);
            } else {
                num++;
            }
            if ( ptrs[i] && (((unsigned) ptrs[i]) & (align_bytes-1)) ) {
                printf("alignment failure: %p, %d, %#x\n",
                    ptrs[i], align_bytes,
                    (unsigned) ptrs[i] & (align_bytes-1));
                exit(1);
            }
            if ( RandFract() < 0.25 && ptrs[i] ) {
                if ( verbosity > 3 ) {
                    printf("  free 1: %p\n", ptrs[i]);
                }
                afree(ptrs[i]);
                ptrs[i] = NULL;
                num--;
            }
        }
        if ( verbosity > 2 ) amem_allocs_print(amem_alloc_all, 1);
        for ( i = 0; i < alloc_elem; i++ ) {
            if ( ptrs[i] ) {
                if ( verbosity > 3 ) {
                    printf("  free 2: %p\n", ptrs[i]);
                }
                afree(ptrs[i]);
            }
        }
    }
    amem_status_print();
}


