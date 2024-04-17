#ifndef __PALLOC__H__
#define __PALLOC__H__

#include "dagaslib.h"

#include "memory_layout.h"
#include "types.h"
#include "print.h"
#include "defs.h"

#define PMEM_END ((void *)(MAX_PA) - KSTACK_SIZE * CPUS)

extern char pmem_base[];

void pmem_init();
void pfree(void *pa);
void *palloc();
void *palloc_n(int alloc_num);

void buddy_init();
int buddy_alloc(int size);
int buddy_free(int offset);

void* kmalloc(int size);
void kfree(void* ptr);

typedef struct pm_struct pm_t;
struct pm_struct
{
    uint64 v_offset;
    uint64 pa;
    uint64 size;
    int cnt;
    pm_t* next;
};

pm_t* alloc_pm(uint64 v_offset, uint64 pa, uint64 size);
void free_pm(pm_t* pm);

#endif