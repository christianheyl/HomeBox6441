#include <amem.h>
#include <stdio.h>
#include <adf.h>

typedef union {
    struct {
        unsigned fields[4];
    } rxDesc;
    struct {
        unsigned fields[10];
    } txDesc;
} Desc;

typedef union {
    unsigned currentState;
    void *data;
} Node;

int main(int argc, const char **argv)
{
    amem_instance_handle inst_desc;
    amem_cache_handle h_desc;
    amem_cache_handle h_std;
    Desc *dp;
    Node *np;

    amem_sbrk(0);

    inst_desc = amem_create(
        "descriptor mem", 8192,
        adf_dma_alloc_cover,
        adf_dma_free_cover);
    h_desc = amem_cache_create_adv(
        "descriptors", sizeof(Desc), 0, 0, 4, inst_desc);
    h_std = amem_cache_create("nodes", sizeof(Node), 0, 0);

    dp = amem_cache_alloc(h_desc);
    dp = amem_cache_alloc(h_desc);
    dp = amem_cache_alloc(h_desc);

    np = amem_cache_alloc(h_std);
    np = amem_cache_alloc(h_std);

    amem_status_print();
}
