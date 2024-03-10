# include "print.h"

const static char digits[] = "0123456789abcdef";

void consputc(char ch) {
    uartputc_sync(ch);
}

/*
val: the integer which should be print.
base: should between binary to hex.
sign: 0 means unsigned, 1 means signed.
*/
static void printint(int val, int base, int sign) {
    char buf[16];
    uint32 real_val = val;
    int i;

    if(sign) {
        if(val < 0) {
            real_val = -val;
        } else {
            sign = 0;
        }
    }

    i = 0;
    do {
        buf[i++] = digits[real_val % base];
        real_val /= base;
    } while(real_val);

    if(sign) {
        buf[i++] = '-';
    }

    while(--i >= 0) {
        consputc(buf[i]);
    }
}

static void printptr(uint64 ptr) {
    int i;
    consputc('0'); consputc('x');
    for(i = 0; i < sizeof(uint64) * 2; ++i, ptr <<= 4) {
        consputc(digits[ptr >> (sizeof(uint64) * 8 - 4)]);
    }
}

/*
Typical printf.

Now support:

- %c
- %d
- %u
- %x
- %p
- %s

No argument are supported.
*/

void printf(char *fmt, ...) {
    va_list args;
    char ch;
    char *str;
    int i;

    if(fmt == NULL) {
        // panic
    }

    va_start(args, fmt);
    for(i = 0; (ch = fmt[i] & 0xff) != 0; ++i) {
        // unformatted strings just put itself.
        if(ch != '%') {
            consputc(ch);
            continue;
        }
        // deal with formatted.
        ch = fmt[++i] & 0xff;
        if(ch == 0) {
            break;
        }
        switch(ch) {
            case 'c':
                consputc(va_arg(args, int) & 0xff);
                break;
            case 'd':
                printint(va_arg(args, int), 10, 1);
                break;
            case 'u':
                printint(va_arg(args, uint32), 10, 0);
                break;
            case 'x':
                printint(va_arg(args, int), 16, 1);
                break;
            case 'p':
                printptr(va_arg(args, uint64));
                break;
            case 's':
                str = va_arg(args, char*);
                for(; *str; str++) {
                    consputc(*str);
                }
                break;
            case '%':
                consputc('%'); // for %%, puts %
                break;
            default:
                consputc('%'); consputc(ch); // other cases, puts %[ch].
                break;
        }
    }
}

void panic(char *str) {
    printf("panic: %s\n", str);
    while(1) {}
}