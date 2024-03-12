#include "pmm.h"
#include "spinlock.h"

spinlock_t pmm_lock;

extern char pmem_base[];

struct ppage {
    struct ppage* next;
};

struct {
    struct ppage* free_pg_list;
} pmem;

void pmem_init() {
    init_spinlock(&pmm_lock);
    p_range_free((void *)pmem_base, PMEM_END);
}

void p_range_free(void *start, void* end) {
    void *pa = (void *)PG_CEIL((uint64)start);
    for(; pa + PG_SIZE <= end; pa += PG_SIZE) {
        pfree(pa);
    }
}

/*
    After we free a page, a page should be full
    of 'U' for each byte expect the first 8 byte
    which should be a pointer point next free page.
*/
void pfree(void *pa) {
    acquire_spinlock(&pmm_lock);
    struct ppage* r;

    if((uint64)pa % PG_SIZE != 0 || (char *)pa < pmem_base || pa >= PMEM_END) {
        panic("palloc error, pa not specified correct.");
    }

    memset((char*) pa, 'U', PG_SIZE);

    r = pmem.free_pg_list;
    pmem.free_pg_list = pa;
    ((struct ppage*)pa)->next = r;
    release_spinlock(&pmm_lock);
}

/*
    After we alloc a page, a page should be full
    of 'N' for each byte.
*/
void* palloc() {
    struct ppage* r = pmem.free_pg_list;
    acquire_spinlock(&pmm_lock);
    if(r != NULL) {
        pmem.free_pg_list = r->next;
        memset((char*) r, 'N', sizeof(r));
    }
    release_spinlock(&pmm_lock);
    return (void*)r;
}