#include "syscall.h"
#include "thread.h"
#include "sysfile.h"

void syscall_handler(trapframe_t* trapframe) {
    int sysid = trapframe->a0;
    switch(sysid) {
        case SYS_PRINT:
            printf("USER: Hello, world!\n");
            break;
        case SYS_WRITE:
            int fd = trapframe->a1;
            uint64 va = trapframe->a2;
            int size = trapframe->a3;
            trapframe->a0 = sys_write(fd, va, size);
            break;
        default:
            printf("syscall_handler: Unknown system call\n");
            break;
    }
    trapframe->epc+=4;
}