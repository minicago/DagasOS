#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#define NULL ((void *)0)
#define SIZE 4096+10

#define run_test(name) \
    if(fork() == 0){ \
        printf("exec "#name"\n");\
        execve("syscalls_test/"#name, NULL, NULL); \
    }\
    // else waitpid(-1, NULL, 0);

int main(int argc, char* argv[]) {
    // return 0;
    run_test(chdir);
    run_test(getcwd);
    return 0;
}