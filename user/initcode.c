#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#define NULL ((void *)0)
#define SIZE 4096+10

int main(int argc, char* argv[]) {
    printf("initcode\n");
    int pid = fork();
    if(pid == 0){
        execve("initrd_mnt/test_syscall", NULL, NULL);
    }
    waitpid(-1, NULL, 0);
    return 0;
}