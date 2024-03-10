#include "vmm.h"
#include "memory_layout.h"
#include "print.h"
#include "pmm.h"
#include "csr.h"

pagetable_t kernel_pagetable;

/*
*walk in pagetable to find pte
*alloc : alloc a physical page for pagetable on route.
*/
pte_t* walk(pagetable_t pagetable, uint64 va, int alloc){
    if( (uint64) pagetable >= MAX_VA ) {
        panic("walk : bad root pagetable");
    }
    for(int level = 2; level > 0; level --){
        pte_t *pte = &pagetable[PTE_INDEX(va, level)];
        if(*pte & PTE_V) {
            pagetable = (pagetable_t) PTE2PA(*pte);
        } else {
            if(alloc){
                pagetable = palloc();
                if (pagetable == 0) return 0;
                else{
                    memset(pagetable, 0, PG_SIZE);
                    * pte = PA2PTE(pagetable) | PTE_V;
                }
            } else return 0;
        }
    }
    return &pagetable[PTE_INDEX(va, 0)];
}

int mappages(pagetable_t pagetable, uint64 va, uint64 pa, uint64 sz, uint64 perm){
    uint64 vpg_l = PG_FLOOR(va);
    uint64 vpg_r = PG_CEIL(va + sz);
    
    uint64 *pte;

    if (sz == 0) return 0;

    for(int i = vpg_l; i < vpg_r; i+=PG_SIZE){
        pte = walk(pagetable, i, 1);
        if(pte == NULL) return -1;
        if(*pte & PTE_V) panic("mappages : remmap");
        *pte = PA2PTE(pa) | perm | PTE_V;
        pa += PG_SIZE;
    }
    return 0;
}

void kvminit(){
    kernel_pagetable = palloc();
    if(kernel_pagetable == NULL) panic("kvminit : alloc kernel pagetable");
    memset(kernel_pagetable, 0, PG_SIZE);

    mappages(kernel_pagetable, UART0, UART0, PG_SIZE, PTE_R | PTE_W);
    mappages(kernel_pagetable, PLIC0, PLIC0, PG_SIZE, PTE_R | PTE_W);
    mappages(kernel_pagetable, KERNEL0, KERNEL0, PMEM0 - KERNEL0, PTE_R | PTE_W | PTE_W);
    mappages(kernel_pagetable, PMEM0, PMEM0, MAX_PA - PMEM0, PTE_R | PTE_W);

    W_CSR(satp, (uint64) kernel_pagetable);
}
