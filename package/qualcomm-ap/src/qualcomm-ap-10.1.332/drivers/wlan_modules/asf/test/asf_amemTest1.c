#include <amem.h>
#include <stdio.h>

#define MIN_ELEM 4
#define MAX_ELEM 10
#define ALLOCS (MAX_ELEM+2)

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

int main(void)
{
    int i, first_fail;
    amem_cache_handle h;
    struct foo *ptrs[ALLOCS];

    h = amem_cache_create("foo pool", sizeof(struct foo), MIN_ELEM, MAX_ELEM);

    first_fail = 1;
    for ( i = 0 ; i < ALLOCS ; i++ ) {
        ptrs[i] = amem_cache_alloc(h); 
        printf("new allocation: %p\n", ptrs[i]);
        if ( ! ptrs[i] && first_fail ) {
            amem_size_t limit, new_limit;
            first_fail = 0;
            limit = amem_sbrk(0);
            new_limit = 2*limit;
            printf("Increasing total mem limit from %lld to %lld\n",
                (long long) limit, (long long) new_limit);
            amem_sbrk(new_limit);
        }
        if ( ! ((i-1) % 2) ) amem_status_print();
    }
    amem_allocs_print(amem_alloc_all, 0);
    try_destroy(h);
    for ( i = 0 ; i < ALLOCS ; i++ ) {
        if ( ! ptrs[i] ) continue;
        printf("freeing %p\n", ptrs[i]);
        amem_cache_free(h, (void *) ptrs[i]);
    }
    amem_status_print();
    try_destroy(h);
    amem_status_print();
}


