#include "syscall.h"

int main() {
    syscall(2,2,3,4);
    int a = 1;
    // for(int i=1;i<=1000000;i++) {
    //     for(int j=1;j<=1000000;j++) {
    //         a++;
    //     }
    // }
    return a;
}