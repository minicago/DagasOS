#ifndef __PROCESS__H__
#define __PROCESS__H__

#include "dagaslib.h"
#include "vmm.h"
#include "spinlock.h"
#include <setjmp.h>

extern jmp_buf kernel_env[];

#define MAX_PROCESS 256

#define USER_ENTRY 0x00000000ull
#define USER_EXIT 0x80000000ull

enum PROCESS_STATE {
    UNUSED,
    USED,
    ZOMBIE,
};

typedef struct {
    spinlock_t lock;
    // basic information
    enum PROCESS_STATE state;
    int64 pid;
    pagetable_t* pagetable;

    // sub process
    struct process_t* parent;
    // linked_list_t* child; // current wo don't save children.

    // threads;
    int thread_count;
} process_t;

extern process_t process_pool[];

process_t* alloc_process();
void free_process(process_t* process);

#endif