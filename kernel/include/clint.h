#ifndef __CLINT__H__
#define __CLINT__H__

#include "memory_layout.h"

#define CLINT_MTIMECMP (CLINT0 + 0x4000)
#define CLINT_MTIME (CLINT0 + 0xBFF8) // cycles since boot.

#endif