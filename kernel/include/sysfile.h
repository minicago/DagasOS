#ifndef __SYSFILE__H__
#define __SYSFILE__H__

#include "types.h"

int sys_write(int fd, uint64 va, uint64 size);
int sys_read(int fd, uint64 va, uint64 size);
int sys_getcwd(uint64 va, uint64 size);
int sys_openat(int dirfd, uint64 va, int flags, int mode);
int sys_mkdirat(int dirfd, uint64 va, int mode);
int sys_close(int fd);
int sys_dup(int fd);

#endif