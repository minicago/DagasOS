#include "vmm.h"
#include "memory_layout.h"
#include "print.h"
#include "pmm.h"

pte_t* walk(pagetable_t root, uint64 va, int alloc){
    if( (uint64) root >= MAX_VA ) {
        panic("walk : bad root pagetable");
    }
    for(int level = 2; level > 0; level --){
        pte_t *pte = &root[PTE_INDEX(va, level)];
        if(*pte & PTE_V) {
            root = (pagetable_t) PTE2PA(*pte);
        } else {
            if(alloc){
                root = palloc();
                if (root == 0) return 0;
                else{
                    memset(root, 0, PG_SIZE);
                    * pte = PA2PTE(root) | PTE_V;
                }
            } else return 0;
        }
    }
    return &root[PTE_INDEX(va, 0)];
}