#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#define NULL ((void *)0)
#define SIZE 4096

#define run_test(name) \
    if(fork() == 0){ \
        printf("exec "#name"\n");\
        execve(""#name, NULL, NULL); \
    }\
    else { \
        waitpid(-1, NULL, 0); \
        printf(#name" fin\n");\
    }

char buf[SIZE];
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
    run_test(close);
    run_test(clone);
    run_test(write);
    int fd = open("/dev/vda2", 0);
    if(fd==-1) {
        printf("/dev/vda2 doesn't exist, ready to create from initrd.img\n");
        fd = open("/dev/vda2", O_CREATE|O_RDWR);
        if(fd==-1) {
            printf("create /dev/vda2 failed\n");
        } else {
            int initrd = open("/initrd.img", O_RDWR);
            if(initrd==-1) {
                printf("open /initrd.img failed\n");
            } else {
                int n;
                while((n = read(initrd, buf, SIZE)) > 0) {
                    write(fd, buf, n);
                    //printf("write %d bytes\n", n);
                }
                close(initrd);
                close(fd);
                printf("create /dev/vda2 success\n");
            }
        }
    } else {
        printf("open /dev/vda2 success\n");
        close(fd);
    }
    run_test(mount);
    run_test(umount);
    return 0;
}