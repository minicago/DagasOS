#ifndef __VMM__H__
#define __VMM__H__

#include "types.h"
#include "defs.h"
#include "memory_layout.h"

typedef uint64 pte_t;
typedef pte_t* pagetable_t;

// The risc-v Sv39 scheme has three levels of page-table
// pages. A page-table page contains 512 64-bit PTEs.
// A 64-bit virtual address is split into five fields:
//   39..63 -- must be zero.
//   30..38 -- 9 bits of level-2 index.
//   21..29 -- 9 bits of level-1 index.
//   12..20 -- 9 bits of level-0 index.
//    0..11 -- 12 bits of byte offset within the page.

#define PTE_NUM (PG_SIZE / sizeof(pte_t))
#define PTE_PNN_MASK 0x1ffffffffffc00ull
#define PTE_PNN_SHIFT 10
#define PTE_RSW_MASK 0x400ull
#define PTE_PG_LV_SHIFT 9
#define PTE_PG_LV_MASK 0x1ff
#define PTE_D 0x80 //Dirty
#define PTE_A 0x40 //Accessed
#define PTE_G 0x20 //Global
#define PTE_U 0x10 //User
#define PTE_X 0x8 
#define PTE_W 0x4
#define PTE_R 0x2
#define PTE_V 0x1 //availble

#define PTE_INDEX(va, level) \
    ( ( (va) >> (PG_OFFSET_SHIFT + (level) * PTE_PG_LV_SHIFT) ) & PTE_PG_LV_MASK )

#define PA2PTE(pa) \
    (( (uint64) pa >> PG_OFFSET_SHIFT ) << PTE_PNN_SHIFT)

#define PTE2PA(pte) \
    ((( (uint64) pte & PTE_PNN_MASK) >> PTE_PNN_SHIFT )  << PG_OFFSET_SHIFT)

#define ATP_MODE_MASK 0xf000000000000000ull
#define ATP_MODE_NONE 0x0000000000000000ull
#define ATP_MODE_SV39 0x8000000000000000ull
#define ATP_MODE_SV48 0x9000000000000000ull
#define ATP_ASID_MASK 0x0ffff00000000000ull
#define ATP_ASID_OFFSET 44
#define ATP_PNN_MASK  0x00000fffffffffffull
#define ATP_PNN_OFFSET 0

#define ATP(ASID, PNN) (((uint64) ((ASID)) << ATP_ASID_OFFSET) | \
    ((uint64) (PNN) >> PG_OFFSET_SHIFT << ATP_PNN_OFFSET) | \
    ATP_MODE_SV39)

extern pagetable_t kernel_pagetable;

#define sfencevma(addr, asid) \
    asm("sfence.vma %0, %1"::"r"(addr),"r"((asid)))

#define sfencevma_all(asid) \
    asm("sfence.vma x0, %0"::"r"((asid)))

pte_t* walk(pagetable_t pagetable, uint64 va, int alloc);

int addpages(pagetable_t pagetable, uint64 va, uint64 sz, uint64 perm);

int mappages(pagetable_t pagetable, uint64 va, uint64 pa, uint64 sz, uint64 perm);

void unmappages(pagetable_t pagetable, uint64 va, uint64 sz, uint64 free_p);

void kvminit();

void print_page_table(pagetable_t pagetable);

pagetable_t alloc_user_pagetable();

void free_user_pagetable(pagetable_t pagetable);

#endif