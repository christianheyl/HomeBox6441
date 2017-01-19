#include <amem.h>

#include <stdio.h>
#include <stdlib.h>

/*----------------------------------------------------------------------*/

struct dbl_link_list {
    struct dbl_link_list *prev;
    struct dbl_link_list *next;
    int num_elem;
};

#define LIST_INIT(list) \
    {                   \
        .next = list,  \
        .prev = list,  \
        .num_elem = 0,  \
    }
extern void ListInit(struct dbl_link_list *list);
extern void ListPush(struct dbl_link_list *list, struct dbl_link_list *node);
extern struct dbl_link_list *ListUnshift(struct dbl_link_list *list);
extern struct dbl_link_list *ListNext(struct dbl_link_list *list);
extern struct dbl_link_list *ListRemove(
    struct dbl_link_list *list,
    struct dbl_link_list *node);
extern int ListLength(struct dbl_link_list *list);


void ListInit(struct dbl_link_list *list)
{
    list->next = list->prev = list;
    list->num_elem = 0;
}

void ListPush(struct dbl_link_list *list, struct dbl_link_list *node)
{
    list->prev->next = node;
    node->prev = list->prev;
    list->prev = node;
    list->num_elem++;
    node->next = list;
}

struct dbl_link_list *ListUnshift(struct dbl_link_list *list)
{
    struct dbl_link_list *node = list->next;
    if ( node == list ) return NULL;
    list->next = node->next;
    list->next->prev = list;
    ListInit(node);
    list->num_elem--;
    return node;
}

struct dbl_link_list *ListNext(struct dbl_link_list *list)
{
    return list->next;
}

struct dbl_link_list *ListRemove(
    struct dbl_link_list *list,
    struct dbl_link_list *node)
{
    if ( node->next == node ) return NULL;
    node->next->prev = node->prev;
    node->prev->next = node->next;
    list->num_elem--;
    return node;
}

int ListLength(struct dbl_link_list *list)
{
    int numElem = 0;
    struct dbl_link_list *p = ListNext(list);

    return list->num_elem;
}

/*----------------------------------------------------------------------*/


void AllocGrowShrink(amem_cache_handle h, int usePool)
{
    static int num_alloc = 0;
    static int grow;
    static struct dbl_link_list list;

    if ( num_alloc == 0 ) {
        grow = 1;
        ListInit(&list);
    } else if ( grow ) {
        /* consider stopping growing */
        if ( random() % 1000 < num_alloc ) grow = 0;
    } else {
        /* consider starting growing */
        if ( random() % 1000 > num_alloc ) grow = 1;
    }    
    if ( grow ) {
        struct dbl_link_list *node;
        if ( usePool ) {
            node = amem_cache_alloc(h);
        } else {
            amem_size_t size;
            size = sizeof(struct dbl_link_list) + random() % 1024;
            node = amalloc(size);
        }
        ListPush(&list, node);
        num_alloc++;
    } else {
        struct dbl_link_list *node = ListUnshift(&list);
        if ( node ) {
            if ( usePool ) {
                amem_cache_free(h, (void *) node);
            } else {
                afree((void *) node);
            }
        }
        num_alloc--;
    }
}

void AllocLeakRandom(amem_cache_handle h, float leakRate, int usePool)
{
    static struct dbl_link_list list = LIST_INIT(&list);
    struct dbl_link_list *node;

    /* Usually but not always deallocate an old node.
       Remove it from the list anyway, so we won't keep references to it.
     */
    node = ListUnshift(&list);
#define RANGE 10000
    if ( random() % RANGE < RANGE * (1.0 - leakRate) ) {
        if ( random() % 100 < 30 ) {
            if ( node ) {
                if ( usePool ) {
                    amem_cache_free(h, (void *) node);
                } else {
                    afree((void *) node);
                }
            }
        } else {
            if ( node ) {
                if ( usePool ) {
                    amem_cache_free(h, (void *) node);
                } else {
                    afree((void *) node);
                }
            }
        }
    } else {
        #if 0
        // SHOULD DO:
        if ( node ) {
            if ( usePool ) {
                amem_cache_free(h, (void *) node);
            } else {
                afree((void *) node);
            }
        }
        #endif
    }

    /* allocate a new node */
    if ( usePool ) {
        node = amem_cache_alloc(h);
    } else {
        amem_size_t size;
        size = sizeof(struct dbl_link_list) + random() % 1024;
        node = amalloc(size);
    }
    ListPush(&list, node);
}

int main(int argc, const char **argv)
{
    amem_cache_handle h;
    int i, reps = 30000;
    float leakRate = 0.001;
    int usePool = 1;
    int condense = 1;

    if ( argc > 1 ) {
        reps = atoi(argv[1]);
        if ( argc > 2 ) {
            leakRate = atof(argv[2]);
            if ( argc > 3 ) {
                usePool = atoi(argv[3]);
                if ( argc > 4 ) {
                    condense = atoi(argv[4]);
                }
            }
        }
    }
    amem_sbrk(1024*1024);
    printf(
        "memory leak detection test: %d reps, %g leak rate\n",
        reps, leakRate);
    h = amem_cache_create("mem leak nodes", sizeof(struct dbl_link_list), 0, 0);
    for ( i = 0 ; i < reps ; i++ ) {
        AllocGrowShrink(h, usePool);
        AllocLeakRandom(h, leakRate, usePool);
    }
    amem_allocs_print(amem_alloc_leaks, condense);
    //amem_allocs_print(amem_alloc_all, 1);
}
