#ifndef __PRINT_H__
#define __PRINT_H__

#include <stdarg.h>

#include "uart.h"
#include "types.h"
#include "defs.h"

void printf(char *fmt, ...);
void panic(char *str);

#endif