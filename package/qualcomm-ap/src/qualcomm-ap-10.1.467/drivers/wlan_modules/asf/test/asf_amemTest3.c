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

#define MIN_MIN_ELEM 0
#define MAX_MIN_ELEM 100
#define MIN_ELEM_RANGE (MAX_MIN_ELEM - MIN_MIN_ELEM)
#define MAX_MAX_ELEM 200
#define MAX_ALLOC_ELEM 250
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
        int min_elem, max_elem, alloc_elem;
        int align_bytes;
        amem_size_t elem_bytes;
        void *ptrs[MAX_ALLOC_ELEM];
        amem_cache_handle h;

        min_elem = MIN_MIN_ELEM + MIN_ELEM_RANGE * RandFract();
        max_elem = min_elem + (MAX_MAX_ELEM - min_elem) * RandFract();
        alloc_elem = MAX_ALLOC_ELEM * RandFract();
        elem_bytes = MIN_ELEM_BYTES + ELEM_BYTES_RANGE * RandFract();

        align_bytes = min(8, FloorPow2(elem_bytes));

        if ( verbosity > 1 ) {
            printf(
                "rep %d of %d: size = %d, align = %d, "
                "min = %d, max = %d, alloc = %d\n",
                rep, reps, (int) elem_bytes, align_bytes,
                min_elem, max_elem, alloc_elem);
        }

        h = amem_cache_create("foo", elem_bytes, min_elem, max_elem);
        if ( verbosity > 2 ) amem_status_print();
        for ( i = 0, num = 0; i < alloc_elem; i++ ) {
            ptrs[i] = amem_cache_alloc(h);
            if ( verbosity > 3 ) {
                printf("  alloc %d of %d (%d): %p\n",
                    i+1, alloc_elem, num, ptrs[i]);
            }
            if ( num < max_elem ) {
                if ( ! ptrs[i] ) {
                    printf("cache alloc %d failed", i);
                    exit(1);
                } else {
                    num++;
                }
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
                amem_cache_free(h, ptrs[i]);
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
                amem_cache_free(h, ptrs[i]);
            }
        }
        if ( amem_cache_destroy(h) != amem_status_success ) {
            printf("cache destroy failed");
            exit(1);
        }
    }
    amem_status_print();
}


