# TASK

- test_read
- test_brk	0	3
- test_exit	2	2
- test_execve	3	3
- test_fork	3	3
- test_mmap	0	3
- test_yield	0	4
- test_munmap	0	4
- test_pipe	0	4
- test_getpid	3	3
- test_mount	5	5
- test_dup2	2	2
- test_getppid	2	2
- test_close	2	2
- test_wait	3	4
- test_write	2	2
- test_open	3	3
- test_waitpid	0	4
- test_times	0	6
- test_clone	0	4
- test_getdents	5	5
- test_gettimeofday	0	3
- test_mkdir	3	3
- test_sleep	0	2
- test_dup	2	2
- test_openat	4	4
- test_getcwd	2	2
- test_chdir	3	3
- test_unlink	0	2
- test_fstat	0	3
- test_umount	5	5
- test_uname	0	2

# BUGS

- kmalloc (in fat32_superblock_init fat32.c:409)
- mount and umount can't find vda