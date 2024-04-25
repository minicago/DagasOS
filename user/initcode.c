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
        execve("test_syscall", NULL, NULL);
    }
    while(1);
    exit(0);
}