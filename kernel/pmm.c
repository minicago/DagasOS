#include "pmm.h"
#include "spinlock.h"

#define PMM_PGADDR(PG_ID) ((void *)PG_CEIL((uint64)start) + PG_SIZE * (PG_ID))

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
    buddy_init();
    memset((char*) pmem_base, 'U', (char*)PMEM_END - pmem_base - 1);
}

/*
    After we free a page, a page should be full
    of 'U' for each byte expect the first 8 byte
    which should be a pointer point next free page.
*/
void pfree(void *pa) {
    if((uint64)pa % PG_SIZE != 0) {
        panic("free pointer must align to page size");
    }

    int idx = ((char*)pa - pmem_base) / PG_SIZE;
    int free_num = buddy_free(idx);
    memset((char*) pa, 'U', PG_SIZE * free_num);
}

/*
    After we alloc a page, a page should be full
    of 'N' for each byte.
*/
void* palloc_n(int alloc_num) {
    int offset = buddy_alloc(alloc_num);
    char* pa = (pmem_base + PG_SIZE * offset);
    memset((char*) pa, 'N', PG_SIZE * alloc_num);
    return (void*)pa;
}

void* palloc() {
    return palloc_n(1);
}

// buddy system

#define IS_POW2(x) (!((x)&((x)-1)))
#define LE_CHILD(x) ((x) << 1)
#define RI_CHILD(x) ((x) << 1 | 1)
#define PARENT(x) ((x) >> 1)

static uint64 fixsize(uint64 size) {
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    size |= size >> 32;
    return size + 1;
}

int buddy_size;
uint32 buddy_tree[KMEMORY / PG_SIZE];

void buddy_init(){
    uint64 pmem_size = fixsize(((char*)PMEM_END - pmem_base - 1) / PG_SIZE);
    buddy_size = pmem_size / 2;
    if(!IS_POW2(buddy_size)) {
        panic("buddy size must be pow of 2");
        return;
    }
    uint64 node_size = buddy_size * 2;
    for(uint64 i = 1; i < 2 * buddy_size; ++i) {
        if(IS_POW2(i)) {
            node_size /= 2;
        }
        buddy_tree[i] = node_size;
    }
}


int buddy_alloc(int size) {
    uint64 index = 1;
    int offset;
    uint64 node_size;

    if(size <= 0) {
        panic("size must be positive");
    }
    if(!IS_POW2(size)) {
        size = fixsize(size);
    }
    
    if(buddy_tree[index] < size) {
        return -1;
    }

    for(node_size = buddy_size; node_size != size; node_size /= 2) {
        if(buddy_tree[LE_CHILD(index)] >= size) {
            index = LE_CHILD(index);
        } else {
            index = RI_CHILD(index);
        }
    }

    offset = index * node_size - buddy_size;

    buddy_tree[index] = 0;
    index = PARENT(index);

    while(index) {
        buddy_tree[index] = 
         MAX(buddy_tree[LE_CHILD(index)], buddy_tree[RI_CHILD(index)]);
        index = PARENT(index);
    }

    return offset;
}

int buddy_free(int offset) {
    int node_size, index = 0;
    int left_longest, right_longest, free_num;

    assert(offset >= 0 && offset < buddy_size);

    node_size = 1;
    index = offset + buddy_size;

    for (; buddy_tree[index]; index = PARENT(index)) {
        node_size *= 2;
        if (index == 1) return node_size;
    }

    buddy_tree[index] = node_size;
    free_num = node_size;

    while (index > 1) {
        index /= 2;
        node_size *= 2;

        left_longest = buddy_tree[LE_CHILD(index)];
        right_longest = buddy_tree[RI_CHILD(index)];
        
        if (left_longest + right_longest == node_size) 
        buddy_tree[index] = node_size;
        else
        buddy_tree[index] = MAX(left_longest, right_longest);
    }
    
    return free_num;
}