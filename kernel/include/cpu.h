#ifndef __CPU__H__
#define __CPU__H__

#include "thread.h"

typedef struct{
    context_t kernel_context;
    thread_t* thread;
    uint64 push_off_num;
    int intr_status;
}cpu_t;

cpu_t* get_cpu();

#endif