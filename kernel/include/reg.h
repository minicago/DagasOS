#ifndef __reg__h__
#define __reg__h__
#include "defs.h"

# define R_REG(reg, variable) \
asm("sd "#reg", %0":"=r"(variable):)

# define W_REG(reg, variable) \
asm("addi %0, "#reg", 0": :"r"((uint64) variable))

#endif