#ifndef __SYSFILE__H__
#define __SYSFILE__H__

#include "types.h"

int sys_write(int fd, uint64 va, uint64 size);
int sys_read(int fd, uint64 va, uint64 size);
int sys_getcwd(uint64 va, uint64 size);
int sys_openat(int dirfd, uint64 va, int flags, int mode);

#endif