#include "stdio.h"
#include "unistd.h"
#define NULL ((void *)0)
void test_printf(){
    printf("user: test printf\n");
    int a = 1;
    printf("user: test %d\n",a);
}
void test_getcwd(){
    printf("user: test getcwd\n");
    char buf[256];
    char *res = getcwd(buf, 256);
    if(res != NULL){
        printf("getcwd: %s\n", buf);
    }
}
void test_open_and_read(){
    printf("user: test open and read\n");
    int fd = open("test",0);
    if(fd == -1){
        printf("open failed\n");
        return;
    }
    char buf[256];
    ssize_t size = read(fd, buf, 256);
    if(size == -1){
        printf("read failed\n");
        return;
    }
    printf("read: %s\n", buf);
}
int main() {
    test_printf();
    test_getcwd();
    test_open_and_read();
    return 0;
}