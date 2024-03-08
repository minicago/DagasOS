#include "palloc.h"

extern char pmem_base[];

struct ppage {
    struct ppage* next;
};

struct {
    struct ppage* free_pg_list;
} pmem;

void pmem_init() {
    printf("%p\n", PMEM_END);
    *(char *)PMEM_END = 'a';
    printf("test: %c\n", *(char *)PMEM_END);
    p_range_free((void *)pmem_base, PMEM_END);
}

void p_range_free(void *start, void* end) {
    void *pa = (void *)PG_CEIL((uint64)start);
    for(; pa + PG_SIZE <= end; pa += PG_SIZE) {
        pfree(pa);
    }
}

void pfree(void *pa) {
    struct ppage* r;
    if((uint64)pa % PG_SIZE != 0 || (char *)pa < pmem_base || pa >= PMEM_END) {
        panic("palloc error, pa not specified correct.");
    }

    memset((char*) pa, 1, PG_SIZE);

    r = pmem.free_pg_list->next;
    pmem.free_pg_list->next = pa;
    ((struct ppage*)pa)->next = r;
}

void* palloc() {
    struct ppage* r = pmem.free_pg_list->next;

    if(r != NULL) {
        pmem.free_pg_list->next = r->next;
        memset((char*) r, 5, sizeof(r));
    }

    return (void*)r;
}