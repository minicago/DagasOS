#ifndef __STRAP__H__
#define __STRAP__H__

#include "types.h"
#include "thread.h"

extern void* trampoline;
void uservec();
void userret(trapframe_t* trapframe);

typedef typeof(userret) userret_t; 

#define USER_VEC_OFFSET ((uint64) ((void*) uservec - trampoline))
#define USER_RET_OFFSET ((uint64) ((void*) userret - trampoline))


void set_strap_uservec();

void set_strap_stvec();

void stvec();

void strap_handler();

void strap_init();

void intr_on();

void intr_off();

void intr_push_on();

void intr_push_off();

void usertrap();

#endif 