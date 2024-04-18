#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#define NULL ((void *)0)
#define SIZE 4096+10

char buf[SIZE];
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
void test_open_read_close(){
    printf("user: test open and read\n");
    int fd = open("test",0);
    if(fd == -1){
        printf("open failed\n");
        return;
    }
    ssize_t size = read(fd, buf, SIZE);
    buf[size-1] = 0;
    printf("read: %d bytes\n", size);
    if(size == -1){
        printf("read failed\n");
        return;
    }
    size = size >10? 10:size;
    for(int i = 0; i < size; i++){
        printf("%x", buf[i]);
    }
    printf("\n");
    int res = close(fd);
    if(res == -1){
        printf("close failed\n");
        return;
    }
}
void test_dup(){
	TEST_START(__func__);
	int fd = dup(STDOUT);
	assert(fd >=0);
	printf("  new fd is %d.\n", fd);
	TEST_END(__func__);
}
void test_mkdir(void){
    TEST_START(__func__);
    int rt, fd;

    rt = mkdir("test_mkdir", 0666);
    printf("mkdir ret: %d\n", rt);
    assert(rt != -1);
    fd = open("test_mkdir", O_RDONLY | O_DIRECTORY);
    if(fd > 0){
        printf("  mkdir1 success.\n");
        int fd2 = openat(fd, "test_mkdir2",O_RDONLY | O_CREATE |  O_DIRECTORY);
        if(fd2 > 0){
            printf("  mkdir2 success.\n");
            close(fd2);
        }
        else printf("  mkdir2 error.\n");
        close(fd);
    }
    else printf("  mkdir error.\n");
    TEST_END(__func__);
}
void test_write(){
	TEST_START(__func__);
	const char *str = "Hello operating system contest.\n";
    const char *app = "\n123456";
	int str_len = strlen(str);
    int fd = open("test_write", O_RDWR | O_CREATE);
    printf("  write: %d %d\n",write(fd, str, str_len), str_len);
    close(fd);
    
    fd = open("test_write", O_RDWR | 1024);
    write(fd, app, strlen(app));
    close(fd);
    fd = open("test_write", O_RDWR);
    str_len = read(fd, buf, SIZE);
    buf[str_len] = 0;
    printf("  read: %s\n", buf);
    close(fd);
	TEST_END(__func__);
}

int main(int argc, char* argv[]) {
    fork();
    printf("RUNNING %s\n", argv[0]);
    test_printf();
    test_getcwd();
    test_open_read_close();
    test_dup();
    //test_mkdir();
    test_write();
    printf("user: exit");
    return 0;
}