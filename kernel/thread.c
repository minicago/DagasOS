#include "thread.h"
#include "spinlock.h"
#include "memory_layout.h"

spinlock_t thread_pool_lock;


thread_t thread_pool[MAX_THREAD];

thread_t* runable_thread_head = NULL;

thread_t* freed_thread_head = NULL;

void thread_pool_init(){
    init_spinlock(&thread_pool_lock);
    acquire_spinlock(&thread_pool_lock);
    freed_thread_head = &thread_pool[0];
    for(uint64 i = 1; i < MAX_THREAD; i++){
        *(uint64*)&thread_pool[i] = (uint64) &thread_pool[i - 1];
    }
    release_spinlock(&thread_pool_lock);
}

void init_thread(thread_t* thread){
    init_spinlock(&thread->lock);
    acquire_spinlock(&thread->lock);

    thread->tid = thread - thread_pool;
    thread->process = NULL;
    memset(&thread->context, 0, sizeof(thread->context)); 
    thread->kstack_bottom = TSTACK0(thread->tid) + MAX_TSTACK_SIZE;
    
    
    release_spinlock(&thread->lock);
}

thread_t* alloc_thread(){
    acquire_spinlock(&thread_pool_lock);
    thread_t* new_thread = freed_thread_head;
    freed_thread_head = (thread_t*)*(uint64*) freed_thread_head;
    return new_thread;
    release_spinlock(&thread_pool_lock);
}

void free_thread(thread_t* thread){
    acquire_spinlock(&thread_pool_lock);
    *(thread_t**) thread = freed_thread_head;
    freed_thread_head = thread;
    release_spinlock(&thread_pool_lock);
}