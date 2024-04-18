#ifndef __PRINT__H__
#define __PRINT__H__

#include <stdarg.h>

#include "uart.h"
#include "types.h"
#include "defs.h"

void printf(char *fmt, ...);
void panic(char *str);
void puts(char *str);

#endif