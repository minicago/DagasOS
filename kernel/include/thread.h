#ifndef __THREAD__H__
#define __THREAD__H__

#include "types.h"
#include "process.h"
#include "vmm.h"
#include "spinlock.h"
#include "waitqueue.h"

typedef struct wait_queue_struct wait_queue_t;

enum THREAD_STATE {
    T_UNUSED,
    T_PREPARING,
    T_RUNNING,
    T_READY,
    T_SLEEPING,
};

typedef struct trapframe_struct{
  /*   0 */ uint64 kernel_satp;   // kernel page table
  /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
  /*  16 */ uint64 kernel_trap;   // usertrap()
  /*  24 */ uint64 epc;           // saved user program counter
  /*  32 */ uint64 kernel_hartid; // saved kernel tp
  /*  40 */ uint64 ra;
  /*  48 */ uint64 sp;
  /*  56 */ uint64 gp;
  /*  64 */ uint64 tp;
  /*  72 */ uint64 t0;
  /*  80 */ uint64 t1;
  /*  88 */ uint64 t2;
  /*  96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
} trapframe_t;

typedef struct thread_struct{
    thread_t* next;
    spinlock_t lock;
    wait_queue_t* waiting;

    enum THREAD_STATE state;
    trapframe_t* trapframe;

    vm_t* stack_vm;
    pagetable_t stack_pagetable;

    process_t* process;
    uint64 tid;

} thread_t;

#define MAX_THREAD 256
extern thread_t thread_pool[];

void thread_pool_init();

// only work after init thread manager coro
void entry_main(thread_t* thread);

void attach_to_process(thread_t* thread, process_t* process);

void init_thread(thread_t* thread);

thread_t* alloc_thread();

void free_thread(thread_t* thread);

void entry_to_user();

void sched();

void sleep();

void awake(thread_t* thread);

int sys_fork();

int sys_exec(char* path);

int sys_exit(int ret);

int sys_wait(int pid, uint64 exit_id);

#endif