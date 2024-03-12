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
} coro_t;

// extern coro_t thread_manager_coro[];
// extern coro_t scheduler_coro;

// extern coro_t* current;

void switch_coro(coro_t* );

#endif