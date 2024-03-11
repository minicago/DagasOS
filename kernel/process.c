#include "process.h"

process_t* process;

process_t* runable_process_head = NULL;

process_t* freed_process_head = NULL;

process_t* process_pool_init(){
    for(uint64 i = 1; i < MAX_PROCESS; i++){
        *(uint64*)&process[i] = (uint64) &process[i - 1];
    }
    return NULL;
}

process_t* init_process(){
    return NULL;
}

process_t* alloc_process(){
    return NULL;
}