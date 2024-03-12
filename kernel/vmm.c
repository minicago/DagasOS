#include "vmm.h"
#include "memory_layout.h"
#include "print.h"
#include "pmm.h"
#include "csr.h"
#include "process.h"

pagetable_t kernel_pagetable;

/*
*walk in pagetable to find pte
*alloc : alloc a physical page for pagetable on route.
*/
void print_pte(pte_t* pte){
    printf("pa : %p\n",PTE2PA(*pte));
}

void print_page_table(pagetable_t pagetable){
    printf("*******\n");
    printf("page_table:%p\n", (uint64) pagetable);
    for(int i = 0; i < 512; i++){
        if(pagetable[i] != 0){
            printf("index %d:",i);
            print_pte(&pagetable[i]);            
        }

    }   
}

pte_t* walk(pagetable_t pagetable, uint64 va, int alloc){
    if( (uint64) pagetable >= MAX_VA ) {
        panic("walk : bad root pagetable");
    }
    for(int level = 2; level >= 1; level --){
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

uint64 va2pa(pagetable_t pagetable, uint64 va){
    if(va > MAX_VA) return 0;
    return PTE2PA (*walk(pagetable, va, 0)) | (va & PG_OFFSET_MASK);
}

int mappages(pagetable_t pagetable, uint64 va, uint64 pa, uint64 sz, uint64 perm){
    uint64 va_l = PG_FLOOR(va);
    uint64 va_r = PG_CEIL(va + sz);
    
    pte_t *pte;

    if (sz == 0) return 0;

    for(uint64 i = va_l; i < va_r; i+=PG_SIZE){
        pte = walk(pagetable, i, 1);
        if(pte == NULL) return -1;
        if(*pte & PTE_V) panic("mappages : remmap");
        
        *pte = PA2PTE(pa) | perm | PTE_V;
        pa += PG_SIZE;
    }
    return 0;
}

void unmappages(pagetable_t pagetable, uint64 va, uint64 sz, uint64 free_p){
    pte_t *pte;
    for (uint64 i = 0; i < sz; i++){
        pte = walk(pagetable, va + i * PG_SIZE, 0);
        if(pte == 0 || !(*pte & PTE_V)){
            panic("unmappages : no such mmap");
        }
        if(free_p){
            pfree((void*) PTE2PA(*pte));
        }
        *pte = 0;
    }
}

void kvminit(){
    kernel_pagetable = palloc();
    if(kernel_pagetable == NULL) panic("kvminit : alloc kernel pagetable");
    memset(kernel_pagetable, 0, PG_SIZE);

    mappages(kernel_pagetable, UART0, UART0, PG_SIZE, PTE_R | PTE_W);
    mappages(kernel_pagetable, PLIC0, PLIC0, PG_SIZE, PTE_R | PTE_W);
    mappages(kernel_pagetable, VIRTIO0, VIRTIO0, PG_SIZE, PTE_R | PTE_W);
    mappages(kernel_pagetable, KERNEL0, KERNEL0, PMEM0 - KERNEL0, PTE_R | PTE_W | PTE_X);
    mappages(kernel_pagetable, PMEM0, PMEM0, MAX_PA - PMEM0, PTE_R | PTE_W);

    sfencevma_all(MAX_PROCESS);
    
    W_CSR(satp, ATP_MODE_SV39 | ((uint64) kernel_pagetable >> PG_OFFSET_SHIFT) | ((uint64) MAX_PROCESS << ATP_ASID_OFFSET) );
    
    printf("%p\n", va2pa(kernel_pagetable, 0x80000002));
    sfencevma_all(MAX_PROCESS);
    
}

pagetable_t* make_u_pagetable(){
    pagetable_t* u_pagetable = palloc();
    memset(u_pagetable, 0, sizeof(u_pagetable));
    return u_pagetable;
}
