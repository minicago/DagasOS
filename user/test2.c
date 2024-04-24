#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#define NULL ((void *)0)
#define SIZE 4096+10

int main(int argc, char* argv[]) {
    fork();
    printf("!!I am test2!\n");
    exit(0);
}