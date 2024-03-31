#include "user.h"
#define N 4096*3+10
char buf[N];

int main() {
    //syscall(SYS_PRINT);
    printf("user: hello world\n");
    int a = 1;
    for(int i=0;i<N;i++) {
        buf[i] = 'a'+i%26;
    }
    buf[N-1] = '\n';
    buf[N] = '\0';
    printf(buf);
    printf("user: good bye\n");
    return a;
}