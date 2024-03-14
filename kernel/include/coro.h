#ifndef __CORO__H__
#define __CORO__H__

#include "types.h"
#include "thread.h"

typedef struct
{
  uint64 ra;
  uint64 sp;
  uint64 s[12];
} env_t;


int coro_setjmp(env_t*);

int coro_longjmp(env_t*, int);

typedef struct {
  env_t env;
  uint64 coro_stack_bottom;
  uint64 coro_stack_size;
} coro_t;

// extern coro_t thread_manager_coro[];
// extern coro_t scheduler_coro;

// extern coro_t* current;

void switch_coro(coro_t* );

int get_tid();

void init_as_scheduler();

void init_thread_manager_coro(uint64 tid);

void scheduler_loop();

#endif