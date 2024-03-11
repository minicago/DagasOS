#ifndef __THREAD__H__
#define __THREAD__H__

#include "types.h"
#include "process.h"

enum THREAD_STATE {
    RUNNING,
    READY,
    SLEEPING,
};

typedef struct {
    uint32 ra;
    uint32 sp;

    uint32 s[12];
} context_content_t;

struct trapframe_t {
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
};

typedef struct {
    uint64 addr;
} thread_context_t;

typedef struct {
    spinlock_t lock;

    enum THREAD_STATE state;
    thread_context_t context;

    uint64 kstack_bottom;
    trapframe_t* trapframe;

    struct trapframe_t trapframe;
} thread_t;

void context_switch(thread_context_t *pre, thread_context_t* cur);

thread_t* alloc_thread(process_t* porc, uint64 entry);
void free_thread(thread_t thread);

#endif