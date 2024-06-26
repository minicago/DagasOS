#include "syscall.h"
#include "thread.h"
#include "sysfile.h"
#include "vmm.h"
#include "dagaslib.h"
#include "bio.h"

void syscall_handler(trapframe_t *trapframe)
{
    int sysid = trapframe->a7;
    // printf("%p\n", trapframe);
    LOG("%p\n", sysid);
    LOG("trapframe->epc : %p\n", trapframe->epc);
    switch (sysid)
    {
    case SYS_openat:
    {
        int dirfd = trapframe->a0;
        uint64 va = trapframe->a1;
        int flags = trapframe->a2;
        int mode = trapframe->a3;
        trapframe->a0 = sys_openat(dirfd, va, flags, mode);
        break;
    }
    case SYS_getcwd:
    {
        uint64 va = trapframe->a0;
        uint64 size = trapframe->a1;
        trapframe->a0 = sys_getcwd(va, size);
        break;
    }
    case SYS_write:
    {
        int fd = trapframe->a0;
        uint64 va = trapframe->a1;
        uint64 size = trapframe->a2;
        trapframe->a0 = sys_write(fd, va, size);
        break;
    }
    case SYS_read:
    {
        int fd = trapframe->a0;
        uint64 va = trapframe->a1;
        uint64 size = trapframe->a2;
        trapframe->a0 = sys_read(fd, va, size);
        break;
    }
    case SYS_mkdirat:
    {
        int dirfd = trapframe->a0;
        uint64 va = trapframe->a1;
        int mode = trapframe->a2;
        trapframe->a0 = sys_mkdirat(dirfd, va, mode);
        break;
    }
    case SYS_close:
    {
        int fd = trapframe->a0;
        trapframe->a0 = sys_close(fd);
        break;
    }
    case SYS_dup:
    {
        int fd = trapframe->a0;
        trapframe->a0 = sys_dup(fd);
        break;
    }
    case SYS_dup3:
    {
        int oldfd = trapframe->a0;
        int newfd = trapframe->a1;
        trapframe->a0 = sys_dup3(oldfd, newfd);
        break;
    }
    case SYS_chdir:
    {
        uint64 va = trapframe->a0;
        trapframe->a0 = sys_chdir(va);
        break;
    }
    case SYS_getdents64:
    {
        int fd = trapframe->a0;
        uint64 va = trapframe->a1;
        int len = trapframe->a2;
        trapframe->a0 = sys_getdents64(fd, va, len);
        break;
    }
    case SYS_fstat:
    {
        int fd = trapframe->a0;
        uint64 va = trapframe->a1;
        trapframe->a0 = sys_fstat(fd, va);
        break;
    }
    case SYS_exit:
    {
        printf("syscall_handler: SYS_exit, code is %d\n", trapframe->a0);
        //TODO: maybe should not flush here
        flush_cache_to_disk();
        sys_exit(trapframe->a0);
        break;
    }
    case SYS_clone:
    {
        trapframe->a0 = sys_fork();
        break;
    }
    case SYS_execve:
    {
        void* path = (void*) trapframe->a0;
        trapframe->a0 = sys_exec(path);
        break;
    }
    case SYS_wait4:
    {
        // static int tmp;
        trapframe->a0 = sys_wait(trapframe->a0, trapframe->a1);
        // trapframe->a0 = sys_wait(trapframe->a0, &tmp);
        break;
    }
    case SYS_getpid:
    {
        trapframe->a0 = sys_getpid();
        break;
    }
    case SYS_getppid:
    {
        trapframe->a0 = sys_getppid();
        break;
    }
    case SYS_mount:
    {
        uint64 source = trapframe->a0;
        uint64 target = trapframe->a1;
        uint64 fs_type = trapframe->a2;
        uint64 flags = trapframe->a3;
        uint64 data = trapframe->a4;
        trapframe->a0 = sys_mount(source, target, fs_type, flags, data);
        break;
    }
    case SYS_umount2:
    {
        uint64 target = trapframe->a0;
        trapframe->a0 = sys_umount(target);
        break;
    }
    default:
    {
        printf("syscall_handler: Unknown system call\n");
        break;
    }
    }
    trapframe->epc += 4;
    
}