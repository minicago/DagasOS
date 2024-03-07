# include "print.h"
# include "types.h"

const static char digits[] = "0123456789abcdef";

void consputc(char ch) {
    uartputc_sync(ch);
}

static void printint(int val, int base, int sign) {
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

void printptr(uint64 ptr) {
    int i;
    consputc('0'); consputc('x');
    for(i = 0; i < sizeof(uint64) * 2; ++i, ptr <<= 4) {
        consputc(digits[ptr >> (sizeof(uint64) * 8 - 4)]);
    }
}