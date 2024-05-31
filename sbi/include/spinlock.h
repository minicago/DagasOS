#ifndef __SPINLOCK__H__
#define __SPINLOCK__H__

#include "types.h"
#include "defs.h"
#define MAX_SPINLOCK 1024
#define AMOSWAP(lock, variable) \
asm("amoswap.d %0,%1,%2":"=r"(variable),"=r"(lock):"r"(variable))
 
typedef uint32 spinlock_t;

void init_spinlock(spinlock_t* spinlock);

void acquire_spinlock(spinlock_t* spinlock);

void release_spinlock(spinlock_t* spinlock);

int try_acquire_spinlock(spinlock_t* spinlock);

#endif