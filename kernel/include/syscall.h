#ifndef __SYSCALL__H__
#define __SYSCALL__H__

#include "thread.h"

#define SYS_PRINT 0
#define SYS_WRITE 1

void syscall_handler(trapframe_t* trapframe);

#endif