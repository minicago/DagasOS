#include "process.h"

process_t process_pool[MAX_PROCESS];

process_t* freed_process_head = NULL;

void process_pool_init(){
    freed_process_head = &process_pool[0];
    for(uint64 i = 1; i < MAX_PROCESS; i++){
        *(uint64*)&process_pool[i] = (uint64) &process_pool[i - 1];
    }
}

void init_process(process_t* process){
    init_spinlock(&process->lock);
    acquire_spinlock(&process->lock);
    process->pagetable = make_u_pagetable();
    process->thread_count = 0;
    process->pid = process - process_pool;
    release_spinlock(&process->lock);
}

process_t* alloc_process(){
    process_t* new_process = freed_process_head;
    freed_process_head = (process_t*)*(uint64*)freed_process_head;
    init_process(new_process);
    return new_process;
}

void process_thread(process_t* process){
    *(process_t**) process = freed_process_head;
    freed_process_head = process;
}