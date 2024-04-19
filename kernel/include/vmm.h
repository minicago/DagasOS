#ifndef __VMM__H__
#define __VMM__H__

#include "types.h"
#include "defs.h"
#include "memory_layout.h"

typedef struct process_struct process_t;
typedef struct thread_struct thread_t;

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

//ASID = PID
#define ATP(ASID, PNN) (((uint64) ((ASID)) << ATP_ASID_OFFSET) | \
    ((uint64) (PNN) >> PG_OFFSET_SHIFT << ATP_PNN_OFFSET) | \
    ATP_MODE_SV39)

extern pagetable_t kernel_pagetable;

#define sfencevma(addr, asid) \
    asm("sfence.vma %0, %1"::"r"(addr),"r"((asid + 1)))

#define sfencevma_all(asid) \
    asm("sfence.vma x0, %0"::"r"((asid + 1)))

pte_t* walk(pagetable_t pagetable, uint64 va, int alloc);

int addpages(pagetable_t pagetable, uint64 va, uint64 sz, uint64 perm);

int mappages(pagetable_t pagetable, uint64 va, uint64 pa, uint64 sz, uint64 perm);

void unmappages(pagetable_t pagetable, uint64 va, uint64 sz, uint64 free_p);

pagetable_t alloc_stack_pagetable();

void switch_stack_pagetable(pagetable_t pagetable, pagetable_t stack_pagetable);

void kvminit();

void print_page_table(pagetable_t pagetable);

void uvminit(process_t* process);

// void free_user_pagetable(pagetable_t pagetable);

int copy_to_pa(void *dst, uint64 src, uint64 len, uint8 from_user);

uint64 va2pa(pagetable_t pagetable, uint64 va);

int copy_to_va(pagetable_t pagetable, uint64 va, void *src, uint64 len);

int copy_string_from_user(uint64 va, char *buf, int size);

void* kvmalloc(int size);

void* uvmalloc(process_t* process, int size);

void kvfree(void* ptr);

void uvfree(process_t* process, void* ptr);

void heap_init(pagetable_t pagetable, int user);

#define MIN_ALL_SFENCE_PG 0x10

#define VM_PA_SHARED 0x1
#define VM_LAZY_ALLOC 0x2
#define VM_THREAD_STACK 0x4
#define VM_NO_FORK 0x8
#define VM_NO_ALLOC 0x10
#define VM_GLOBAL 0x20

typedef struct vm_struct vm_t;
typedef struct pm_struct pm_t;

struct vm_struct
{
    pagetable_t pagetable;
    uint64 va;
    uint64 size;
    int type;
    uint64 perm;
    pm_t* pm;
    vm_t* next;
    
} ;

vm_t* alloc_vm(process_t* process, uint64 va, uint64 size, pm_t* pm, int perm, int type);
vm_t* alloc_vm_stack(thread_t* thread, uint64 va, uint64 size, pm_t* pm, int perm, int type);

void vm_clear_pm(vm_t* vm);
void free_vm(vm_t* vm);
void vm_insert_pm(vm_t* vm, pm_t* pm);

void vm_insert(process_t* process, vm_t* vm);

int vm_rm(process_t* process, vm_t* rm_vm);

vm_t* vm_lookup(vm_t* vm_list, uint64 va);

void vm_list_free(process_t* process, int deep);

#endif