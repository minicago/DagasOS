#include "user.h"
#include "types.h"
#include "syscall.h"

void printf(const char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    syscall(SYS_WRITE, 1, (uint64)str, len);
}