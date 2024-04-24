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

void pfree(void *pa);
void *palloc();
void *palloc_n(int alloc_num);

void buddy_init();
int buddy_alloc(int size);
int buddy_free(int offset);

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
    LOG("%p\n",offset);
    return offset;
}

int buddy_free(int offset) {
    int node_size, index = 0;
    int left_longest, right_longest, free_num;
    LOG("%p\n",offset);
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

// lightest slab

typedef struct object_struct object_t;
typedef struct kmem_cache_struct kmem_cache_t;
typedef struct slab_struct slab_t;

struct object_struct
{
    object_t* next;
};


struct slab_struct{
    int empty;
    int size;
    slab_t* next;
    object_t* freelist;
};

#define MIN_SLAB_OBJECT_SIZE 8
#define MAX_SLAB_OBJECT_SIZE PG_SIZE / 2
#define SLAB_SIZE_MULTIPLY 4

#define MAX_KMEM_CACHE 9

struct kmem_cache_struct{
    int object_size;
    int object_num;
    int offset;
    slab_t* slab_list;
} kmem_cache_list[MAX_KMEM_CACHE];

int get_kmem_cache_index(int size){
    for(int i = 0; i <= MAX_KMEM_CACHE; i++){
        if(size <= (MIN_SLAB_OBJECT_SIZE << i)) return i; 
    }
    return -1;
}


void init_kmem_cache(){
    for(int i = 0, size = MIN_SLAB_OBJECT_SIZE; i < MAX_KMEM_CACHE ; i++, size = size * 2){
        if (size > MAX_SLAB_OBJECT_SIZE) panic("init_kmem_cache: MAX_SLAB_OBJECT_SIZE is too small\n");
        kmem_cache_list[i].object_size = size;
        kmem_cache_list[i].object_num = (PG_SIZE - sizeof(slab_t)) / size;
        kmem_cache_list[i].offset = PG_SIZE - kmem_cache_list[i].object_num * size;
        kmem_cache_list[i].slab_list = NULL;
    }
}

slab_t* alloc_slab(int size, int offset, int num){
    slab_t* slab = palloc_n(1);
    if(slab == NULL) panic("alloc slab: No page for alloc slab");
    slab->freelist = ((void*) (slab)) + offset;
    for(int i = 0; i < num; i++){
        if (i == num - 1) ((object_t*) (((void*) slab->freelist )+ i * size))->next = NULL;
        else ((object_t*) (((void*) slab->freelist )+ i * size))->next = (((void*) slab->freelist )+ (i + 1) * size) ;
    } 
    slab->empty = 0;
    slab->size = size;
    return slab;
}

void* kmalloc_object(kmem_cache_t* kmem_cache){
    void* ptr = NULL;
    if(kmem_cache->slab_list == NULL){
        kmem_cache->slab_list = alloc_slab(kmem_cache->object_size, kmem_cache->offset, kmem_cache->object_num);
    }
    assert(kmem_cache->slab_list != NULL);
    ptr = kmem_cache->slab_list->freelist;
    kmem_cache->slab_list->freelist = ((object_t*)(kmem_cache->slab_list->freelist))->next;
    if(kmem_cache->slab_list->freelist == NULL) kmem_cache->slab_list = kmem_cache->slab_list->next;  
    return ptr;
}

void kfree_object(void* ptr){
    slab_t* slab = (slab_t*) PG_FLOOR((uint64) ptr);
    ((object_t*) ptr)->next = slab->freelist;
    slab->freelist = ptr;
    if(slab->empty){
        slab->empty = 0;
        slab->next = kmem_cache_list[get_kmem_cache_index(slab->size)].slab_list;
        kmem_cache_list[get_kmem_cache_index(slab->size)].slab_list = slab;
    }
}

void* kmalloc(int size){
    if(size > MAX_SLAB_OBJECT_SIZE) {
        return palloc_n(PG_CEIL(size) >> PG_OFFSET_SHIFT);
    }
    else return kmalloc_object(kmem_cache_list + get_kmem_cache_index(size));
}

void kfree(void* ptr){
    if((((uint64) ptr) & PG_OFFSET_MASK) == 0) pfree(ptr);
    else kfree_object(ptr);
}

void pmem_init() {
    init_spinlock(&pmm_lock);
    buddy_init();
    memset((char*) pmem_base, 'U', (char*)PMEM_END - pmem_base - 1);
    init_kmem_cache();
}

pm_t* alloc_pm(uint64 v_offset, uint64 pa, uint64 size){
    pm_t* pm = kmalloc(sizeof(pm_t));
    pm->next = NULL;
    pm->v_offset = v_offset;
    pm->size = size;
    if(pa == 0) pm->pa = (uint64) palloc_n(PG_CEIL(size)/ PG_SIZE);
    else pm->pa = pa;
    pm->cnt = 0;
    return pm;
}

void free_pm(pm_t* pm){
    pm->cnt--;
    if(pm->cnt == 0) {
        LOG("%p\n",pm->pa);
        pfree((void*) pm->pa);
        kfree(pm);
    }
}