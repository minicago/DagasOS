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

// only work after init thread manager coro
void entry_main(thread_t* thread){
    acquire_spinlock(&thread->lock);
    thread->trapframe->epc = USER_ENTRY;
    thread->trapframe->ra = USER_EXIT;
    thread->trapframe->sp = thread->kstack_bottom - sizeof(trapframe_t);
    thread->state = T_READY;
    release_spinlock(&thread->lock);
}

void attach_to_process(thread_t* thread, process_t* process){
    acquire_spinlock(&thread->lock);
    thread -> process = process;
    process -> thread_count ++;

    uint64 pa = (uint64)palloc();
    uint64 va = thread->kstack_bottom - PG_SIZE;

    // mappages(kernel_pagetable, va, pa, PG_SIZE, PTE_R | PTE_W);
    mappages(*process->pagetable, va, pa, PG_SIZE, PTE_R | PTE_W | PTE_U);
    // sfencevma(va, MAX_THREAD);
    sfencevma(va, process->pid);

    release_spinlock(&thread->lock);
}

void init_thread(thread_t* thread){
    init_spinlock(&thread->lock);
    acquire_spinlock(&thread->lock);

    thread->tid = thread - thread_pool;
    thread->process = NULL;
    memset(&thread->trapframe, 0, sizeof(thread->trapframe)); 
    thread->kstack_bottom = TSTACK_BOTTOM(thread->tid);
    
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

void entry_to_user(){
    int tid = get_tid();
    if(tid == -1) panic("wrong coro");
    W_CSR(sepc, thread_pool[tid].trapframe->epc);
    C_CSR(sstatus, SSTATUS_SPP);
    // call trapret in trampoline
}