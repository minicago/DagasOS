#ifndef __PRINT__H__
#define __PRINT__H__

#include <stdarg.h>

#include "uart.h"
#include "types.h"
#include "defs.h"
#include "coro.h"

#ifndef LOG
#define LOG(format, ...) \
    printf("TID:%d %s:%d in %s ",get_tid(), __FILE__, __LINE__, __FUNCTION__); \
    printf(format, ##__VA_ARGS__)
#endif

#ifndef TLOG
#define TLOG(ENABLE, format, ...) \
    if(ENABLE) {\
        LOG(format, ##__VA_ARGS__); \
    }
#endif

void printf(char *fmt, ...);
void panic(char *str);
void puts(char *str);

#endif