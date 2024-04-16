#include "vmm.h"
#include "memory_layout.h"
#include "print.h"
#include "pmm.h"
#include "csr.h"
#include "process.h"
#include "strap.h"
#include "coro.h"

pagetable_t kernel_pagetable;

void heap_init(pagetable_t pagetable, int user);

void print_pte(pte_t *pte)
{
    printf("pa : %p\n", PTE2PA(*pte));
}

void print_page_table(pagetable_t pagetable)
{
    printf("*******\n");
    printf("page_table:%p\n", (uint64)pagetable);
    for (int i = 0; i < 512; i++)
    {
        if (pagetable[i] != 0)
        {
            printf("index %d:", i);
            print_pte(&pagetable[i]);
        }
    }
}

pte_t *walk(pagetable_t pagetable, uint64 va, int alloc)
{
    if ((uint64)pagetable >= MAX_VA)
    {
        panic("walk : bad root pagetable");
    }
    for (int level = 2; level > 0; level--)
    {
        pte_t *pte = &pagetable[PTE_INDEX(va, level)];
        if (*pte & PTE_V)
        {
            pagetable = (pagetable_t)PTE2PA(*pte);
        }
        else
        {
            if (alloc)
            {
                pagetable = palloc();
                if (pagetable == 0)
                    return 0;
                else
                {
                    memset(pagetable, 0, PG_SIZE);
                    *pte = PA2PTE(pagetable) | PTE_V;
                }
            }
            else
                return 0;
        }
    }
    return &pagetable[PTE_INDEX(va, 0)];
}

void walk_and_free(pagetable_t pagetable, int level)
{
    if ((uint64)pagetable >= MAX_VA)
    {
        panic("walk_and_free : bad root pagetable");
    }
    for (int i = 0; i < PTE_NUM; i++)
    {
        pte_t *pte = &pagetable[i];
        if (*pte & PTE_V)
        {
            walk_and_free((pagetable_t)PTE2PA(*pte), level - 1);
            if (level == 1)
            {
                if (!(*pte & PTE_G))
                    pfree((void *)PTE2PA(*pte));
            }
        }
    }
    pfree((void *)pagetable);
}

uint64 va2pa(pagetable_t pagetable, uint64 va)
{
    if (va > MAX_VA)
        return 0;
    pte_t *pte = walk(pagetable, va, 0);
    if (*pte == 0)
        return 0;
    else
        return PTE2PA(*pte) | (va & PG_OFFSET_MASK);
}

int addpages(pagetable_t pagetable, uint64 va, uint64 sz, uint64 perm)
{
    uint64 va_l = PG_FLOOR(va);
    uint64 va_r = PG_CEIL(va + sz);

    pte_t *pte;

    if (sz == 0)
        return 0;

    for (uint64 i = va_l; i < va_r; i += PG_SIZE)
    {
        pte = walk(pagetable, i, 1);
        if (pte == NULL)
            return -1;
        if (*pte & PTE_V)
            panic("mappages : remmap");
        uint64 pa = (uint64)palloc();
        if ((void *)pa == NULL)
            panic("addpages: no physic pages");
        *pte = PA2PTE(pa) | perm | PTE_V;
    }
    return 0;
}

int mappages(pagetable_t pagetable, uint64 va, uint64 pa, uint64 sz, uint64 perm)
{
    uint64 va_l = PG_FLOOR(va);
    uint64 va_r = PG_CEIL(va + sz);

    pte_t *pte;

    if (sz == 0)
        return 0;

    for (uint64 i = va_l; i < va_r; i += PG_SIZE)
    {
        pte = walk(pagetable, i, 1);
        if (pte == NULL)
            return -1;
        if (*pte & PTE_V)
            panic("mappages : remmap");

        *pte = PA2PTE(pa) | perm | PTE_V;
        pa += PG_SIZE;
    }
    return 0;
}

void unmappages(pagetable_t pagetable, uint64 va, uint64 sz, uint64 free_p)
{
    pte_t *pte;
    for (uint64 i = 0; i < sz; i += PG_SIZE)
    {
        pte = walk(pagetable, va + i, 0);
        if (pte == 0 || !(*pte & PTE_V))
        {
            panic("unmappages : no such mmap");
        }
        if (free_p)
        {
            pfree((void *)PTE2PA(*pte));
        }
        *pte = 0;
    }
}

void kvminit()
{
    kernel_pagetable = palloc();
    if (kernel_pagetable == NULL)
        panic("kvminit : alloc kernel pagetable");
    memset(kernel_pagetable, 0, PG_SIZE);
    
    mappages(kernel_pagetable, VIRTIO0, VIRTIO0, PG_SIZE, PTE_R | PTE_W);
    mappages(kernel_pagetable, UART0, UART0, PG_SIZE, PTE_R | PTE_W);
    mappages(kernel_pagetable, PLIC0, PLIC0, 0x400000, PTE_R | PTE_W);
    mappages(kernel_pagetable, KERNEL0, KERNEL0, PMEM0 - KERNEL0, PTE_R | PTE_W | PTE_X);
    mappages(kernel_pagetable, PMEM0, PMEM0, MAX_PA - PMEM0, PTE_R | PTE_W);
    mappages(kernel_pagetable, TRAMPOLINE, (uint64)trampoline, PG_SIZE, PTE_R | PTE_X);
    heap_init(kernel_pagetable, 0);

    sfencevma_all(MAX_PROCESS);

    W_CSR(satp, ATP(MAX_THREAD, kernel_pagetable));

    sfencevma_all(MAX_PROCESS);
}

pagetable_t alloc_user_pagetable()
{
    pagetable_t u_pagetable = palloc();
    memset(u_pagetable, 0, PG_SIZE);
    mappages(u_pagetable, TRAMPOLINE, (uint64)trampoline, PG_SIZE, PTE_R | PTE_X);
    heap_init(u_pagetable, 1);
    return u_pagetable;
}

void free_user_pagetable(pagetable_t pagetable)
{
    unmappages(pagetable, TRAMPOLINE, PG_SIZE, 0);

    walk_and_free(pagetable, 2);
}

// Copy from user to kernel.
// Copy len bytes to dst from virtual address srcva in a given page table.
// Return 0 on success, -1 on error.
int copy_from_va(pagetable_t pagetable, void* dst, uint64 srcva, uint64 len)
{
    uint64 n, va0, pa0;
    uint64 tmp = len;
    while (len > 0)
    {
        va0 = PG_FLOOR(srcva);
        pa0 = va2pa(pagetable, va0);
        if (pa0 == 0)
            return -1;
        n = PG_SIZE - (srcva - va0);
        if (n > len)
            n = len;
        memcpy(dst, (void *)(pa0 + (srcva - va0)), n);
        len -= n;
        dst += n;
        srcva = va0 + PG_SIZE;
    }
    return tmp;
}

int copy_to_pa(void *dst, uint64 src, uint64 len, uint8 from_user)
{
    if(from_user)
    {
        if(copy_from_va(get_current_proc()->pagetable, dst, src, len) < 0)
            return -1;
    } else {
        memcpy(dst, (void *)src, len);
    }
    return len;
}

// copy from pa to va
int copy_to_va(pagetable_t pagetable, uint64 va, void *src, uint64 len)
{
    
    uint64 n, va0, pa0;
    uint64 tmp = len;
    while (len > 0)
    {
        va0 = PG_FLOOR(va);
        pa0 = va2pa(pagetable, va0);
        // printf("copy_to_va: voff:%p pa:%p src:%p\n",va - va0, pa0, src);
        if (pa0 == 0)
            return -1;
        n = PG_SIZE - (va - va0);
        if (n > len)
            n = len;
        memcpy((void *)(pa0 + (va - va0)), src, n);
        len -= n;
        src += n;
        va = va0 + PG_SIZE;
    }
    return tmp;
}

int copy_string_from_user(uint64 va, char *buf, int size)
{
    int n = 0;
    for (int i = 0; i < size; i++)
    {
        if (copy_to_pa(buf + i, va + i, 1, 1) < 0)
            return -1;
        if (buf[i] == '\0')
            return i;
        n++;
    }
    return -1;
}

typedef struct brk_struct brk_t;

struct brk_struct{
    brk_t* next;
    uint64 size;
    int used;
};

void heap_init(pagetable_t pagetable, int user){
    printf("!init heap\n");
    uint64 pa = (uint64) palloc();
    mappages(pagetable, HEAP_SPACE, pa, PG_SIZE,(user?PTE_U:0) | PTE_W | PTE_R );
    brk_t* brk_first = (brk_t*) pa;
    brk_first->next = NULL;
    brk_first->size = PG_SIZE - sizeof(brk_t);;
    brk_first->used = 0;
}

void* vmalloc_r(pagetable_t pagetable, int size, int user, int pid){
    printf("vmalloc_r(%d,%d,%d)\n",size,user,pid);
    uint64 va;
    brk_t *brk,*next_brk;
    for(va = HEAP_SPACE; ; va = (uint64)brk->next){
        // printf("%p\n",va);
        brk = (brk_t*) va2pa(pagetable, va);
        
        if(brk->used) continue;
        if(brk->next == NULL) break;
        
        next_brk = (brk_t*) va2pa(pagetable, (uint64) brk->next);
        

        while(!next_brk->used) {
            brk->size += next_brk->size + sizeof(brk_t);
            brk->next = next_brk->next;
            next_brk = (brk_t*) va2pa(pagetable, (uint64) brk->next);
        }
        if(brk->size >= sizeof(brk_t) + size ) break;
        
    }
    
    while(brk->size < sizeof(brk_t) + size ){
        uint64 pa = (uint64) palloc();
        mappages(pagetable, (uint64) brk + sizeof(brk_t) + brk->size, pa, PG_SIZE,(user?PTE_U:0) | PTE_W | PTE_R );
        if(size <= MIN_ALL_SFENCE_PG * PG_SIZE) sfencevma((uint64) brk + sizeof(brk_t) + brk->size, pid);
        brk->size += PG_SIZE;
    }
    
    if(size > MIN_ALL_SFENCE_PG * PG_SIZE) sfencevma_all(pid);

    

    brk->next = (brk_t*) (va + size + sizeof(brk_t));
    // printf("%p!\n",brk->next);
    next_brk = (brk_t*) va2pa(pagetable, (uint64) brk->next);
    next_brk->size = brk->size - size - sizeof(brk_t);
    next_brk->next = NULL;
    brk->used = 1;
    brk->size = size;
    return (void*) va + sizeof(brk_t);
}

void vfree_r(pagetable_t pagetable, void* ptr) {
    brk_t* brk = (brk_t*) va2pa(pagetable, (uint64) ptr - sizeof(brk_t));
    brk->used = 0;
}

void* kvmalloc(int size){
    return vmalloc_r(kernel_pagetable, ((size - 1) & (~7)) + 8, 0, MAX_PROCESS);
}

void* uvmalloc(process_t* process, int size){
    return vmalloc_r(process->pagetable, ((size - 1) & (~7)) + 8, 1, process->pid);
}

void kvfree(void* ptr){
    vfree_r(kernel_pagetable, ptr);
}

void uvfree(process_t* process, void* ptr){
    vfree_r(process->pagetable, ptr);
}