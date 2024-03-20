#include "syscall.h"
#include "types.h"

uint64 do_user_syscall(uint64 sysnum, uint64 a0, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 a6, uint64 a7){
    uint64 ret;
    __asm__ __volatile__ ("ecall\n sd a0, %0": "=m"(ret):);
    return ret;
}