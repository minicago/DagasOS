#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#define NULL ((void *)0)
#define SIZE 4096+10

#define run_test(name) \
    if(fork() == 0){ \
        printf("exec "#name"\n");\
        execve(""#name, NULL, NULL); \
    }\
    else { \
        waitpid(-1, NULL, 0); \
        printf(#name" fin\n");\
    }
int main(int argc, char* argv[]) {
    // return 0;
    // now can umount /mnt, because we have mounted initrd.img to /mnt
    run_test(chdir);
    run_test(getcwd);
    run_test(getpid);
    run_test(getppid);
    run_test(dup);
    run_test(dup2);
    run_test(execve);
    run_test(getdents);
    run_test(mkdir_);
    run_test(open);
    run_test(wait);
    run_test(exit);
    run_test(openat);
    run_test(read);
    run_test(fork);
    run_test(clone);
    run_test(write);
    run_test(mount);
    run_test(umount);
    return 0;
}