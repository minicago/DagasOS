#include "thread.h"
#include "spinlock.h"
#include "vmm.h"
#include "memory_layout.h"
#include "csr.h"
#include "cpu.h"
#include "coro.h"

spinlock_t thread_pool_lock;


thread_t thread_pool[MAX_THREAD];

thread_t* free_thread_head = NULL;



void thread_pool_init(){
    init_spinlock(&thread_pool_lock);
    acquire_spinlock(&thread_pool_lock);
    free_thread_head = &thread_pool[0];
    for(uint64 i = 1; i < MAX_THREAD; i++){
        *(uint64*)&thread_pool[i] = (uint64) &thread_pool[i - 1];
    }
    release_spinlock(&thread_pool_lock);
}

void entry_main(thread_t* thread){
    acquire_spinlock(&thread->lock);
    thread->context->epc = USER_ENTRY;
    thread->context->ra = USER_EXIT;
    thread->context->sp = thread->kstack_bottom - sizeof(context_t);
    thread->state = T_READY;
    // memset(thread->context.s, 0, sizeof(thread->context.s));
    release_spinlock(&thread->lock);
}

void attach_to_process(thread_t* thread, process_t* process){
    acquire_spinlock(&thread->lock);
    thread -> process = process;
    process -> thread_count ++;

    uint64 pa = (uint64)palloc();
    uint64 va = thread->kstack_bottom - PG_SIZE;
    thread->context = (context_t*)thread->kstack_bottom - sizeof(context_t); // specific thread context at bottom of stack base
    mappages(kernel_pagetable, va, pa, PG_SIZE, PTE_R | PTE_W);
    mappages(*process->pagetable, va, pa, PG_SIZE, PTE_R | PTE_W | PTE_U);
    sfencevma(va, MAX_THREAD);
    sfencevma(va, process->pid);
    release_spinlock(&thread->lock);
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
    thread_t* new_thread = free_thread_head;
    free_thread_head = (thread_t*)*(uint64*) free_thread_head;
    release_spinlock(&thread_pool_lock);
    return new_thread;
}

void free_thread(thread_t* thread){
    acquire_spinlock(&thread_pool_lock);
    *(thread_t**) thread = free_thread_head;
    free_thread_head = thread;
    release_spinlock(&thread_pool_lock);
}