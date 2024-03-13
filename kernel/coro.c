#include "coro.h"
#include "thread.h"
#include "cpu.h"
#include "memory_layout.h"

coro_t thread_manager_coro[MAX_THREAD];

// in the future to realize multihart, we need to

// coro_t scheduler_coro;

coro_t* current;

void switch_coro(coro_t* dest){
    if(coro_setjmp(&get_cpu()->current_coro->env) == 0){
        get_cpu()->current_coro = dest;
        coro_longjmp(&dest->env, 1);
    }
}

void scheduler_loop(){
    while (1)
    {
        for (int i = 0 ; i < MAX_THREAD; i++){
            if (try_acquire_spinlock(&thread_pool[i].lock) != 0){
                if(thread_pool[i].state == T_READY){
                    thread_pool[i].state = T_RUNNING;
                    switch_coro(&thread_manager_coro[i]);
                }
                
                release_spinlock(&thread_pool[i].lock);
            }
        }
    }
}

void set_entry(coro_t* coro, uint64 entry){
    coro->env.ra = entry;
}

void set_sp(coro_t* coro, uint64 sp){
    coro->env.sp = sp;
}

void clean_s(coro_t* coro){
    memset(coro->env.s, 0, sizeof(coro->env.s));
}

// only work after that thread has been attached to process
void init_thread_manager_coro(uint64 tid){
    clean_s(&thread_manager_coro[tid]);
    thread_manager_coro[tid].coro_stack_bottom = COROSTACK_BOTTOM(tid);
    
    uint64 pa = (uint64)palloc();
    uint64 va = COROSTACK_BOTTOM(tid) - PG_SIZE;

    mappages(*thread_pool[tid].process->pagetable, va, pa, PG_SIZE, PTE_W | PTE_R);
    mappages(kernel_pagetable, va, pa, PG_SIZE, PTE_W | PTE_R) ;
    sfencevma(va, MAX_THREAD);
    sfencevma(va, thread_pool[tid].process->pid);

    thread_pool[tid].trapframe = (trapframe_t*) COROSTACK_BOTTOM(tid) - sizeof(trapframe_t);
    set_sp(&thread_manager_coro[tid], (uint64) thread_pool[tid].trapframe);
 
    
    set_entry(&thread_manager_coro[tid] ,(uint64) entry_to_user);

}

int get_tid(){
    int64 index = get_cpu()->current_coro - thread_manager_coro;
    if(index >= MAX_THREAD || index < 0) return -1;
    else return index;
}

void init_as_scheduler(){
    get_cpu()->current_coro = &get_cpu()->scheduler_coro;
}
