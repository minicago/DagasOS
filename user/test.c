#include "user.h"

int main() {
    //syscall(SYS_PRINT);
    printf("user: hello world\n");
    int a = 1;
    for(int i=1;i<=10;i++) {
        for(int j=1;j<=100;j++) {
            a++;
        }
        printf("user: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    }
    
    printf("user: good bye\n");
    return a;
}