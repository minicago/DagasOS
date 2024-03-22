#ifndef __SYSCALL__H__
#define __SYSCALL__H__

#include "types.h"

uint64 do_user_syscall(uint64 sysnum, uint64 a0, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 a6, uint64 a7);

#define SYSCALL0(sysnum) (do_user_syscall((sysnum),0, 0, 0, 0, 0, 0, 0, 0))
#define SYSCALL1(sysnum, a0) (do_user_syscall((sysnum), (a0), 0, 0, 0, 0, 0, 0, 0))
#define SYSCALL2(sysnum, a0, a1) (do_user_syscall((sysnum), (a0), (a1), 0, 0, 0, 0, 0, 0))
#define SYSCALL3(sysnum, a0, a1, a2) (do_user_syscall((sysnum), (a0), (a1), (a2), 0, 0, 0, 0, 0))
#define SYSCALL4(sysnum, a0, a1, a2, a3) (do_user_syscall((sysnum), (a0), (a1), (a2), (a3), 0, 0, 0, 0))
#define SYSCALL5(sysnum, a0, a1, a2, a3, a4) (do_user_syscall((sysnum), (a0), (a1), (a2), (a3), (a4), 0, 0, 0))
#define SYSCALL6(sysnum, a0, a1, a2, a3, a4, a5) (do_user_syscall((sysnum), (a0), (a1), (a2), (a3), (a4), (a5), 0, 0))
#define SYSCALL7(sysnum, a0, a1, a2, a3, a4, a5, a6) (do_user_syscall((sysnum), (a0), (a1), (a2), (a3), (a4), (a5), (a6), 0))
#define SYSCALL8(sysnum, a0, a1, a2, a3, a4, a5, a6, a7) (do_user_syscall((sysnum), (a0), (a1), (a2), (a3), (a4), (a5), (a6), (a7)))

#define COUNT_ARG_R(a8, a7, a6, a5, a4, a3, a2, a1, a0, res, ...) res
#define COUNT_ARG(...) COUNT_ARG_R(__VA_ARGS__,8, 7, 6, 5, 4, 3, 2, 1, 0)

#define CAT_R(a,b) a##b
#define CAT(a,b) CAT_R(a, b)

#define syscall(...) CAT(SYSCALL, COUNT_ARG( __VA_ARGS__)) ( __VA_ARGS__)


#define SYS_PRINT 0x0

#endif