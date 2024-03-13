#ifndef __CPU__H__
#define __CPU__H__

#include "thread.h"
#include "coro.h"

typedef struct{
    context_t* kernel_context;
    thread_t* thread;
    uint64 push_off_num;
    int intr_status;
    coro_t* current_coro;
    coro_t scheduler_coro;
}cpu_t;

cpu_t* get_cpu();

#endif