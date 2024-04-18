#ifndef __SYSFILE__H__
#define __SYSFILE__H__

#include "types.h"

typedef unsigned int mode_t;
typedef long int off_t;
struct kstat {
        uint64 st_dev;
        uint64 st_ino;
        mode_t st_mode;
        uint32 st_nlink;
        uint32 st_uid;
        uint32 st_gid;
        uint64 st_rdev;
        unsigned long __pad;
        off_t st_size;
        uint32 st_blksize;
        int __pad2;
        uint64 st_blocks;
        long st_atime_sec;
        long st_atime_nsec;
        long st_mtime_sec;
        long st_mtime_nsec;
        long st_ctime_sec;
        long st_ctime_nsec;
        unsigned __unused[2];
};

int sys_write(int fd, uint64 va, uint64 size);
int sys_read(int fd, uint64 va, uint64 size);
int sys_getcwd(uint64 va, uint64 size);
int sys_openat(int dirfd, uint64 va, int flags, int mode);
int sys_mkdirat(int dirfd, uint64 va, int mode);
int sys_close(int fd);
int sys_dup(int fd);
int sys_dup3(int oldfd, int newfd);
int sys_chdir(uint64 va);
int sys_getdents64(int fd, uint64 va, int size);
int sys_fstat(int fd, uint64 va);

#endif