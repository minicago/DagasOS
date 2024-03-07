# include "print.h"

const static char digits[] = "0123456789abcdef";

void consputc(char ch) {
    uartputc_sync(ch);
}

void printint(int val, int base, int sign) {
    char buf[16];
    int real_val = val;
    int i;

    if(sign) {
        if(val < 0) {
            real_val = -val;
        } else {
            sign = 0;
        }
    }

    i = 0;
    while(real_val) {
        buf[i++] = digits[real_val % base];
        real_val /= base;
    }

    while(--i >= 0) {
        consputc(buf[i]);
    }
}