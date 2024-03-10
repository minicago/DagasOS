#ifndef __PLIC__H__
#define __PLIC__H__

#include "memory_layout.h"

#define PLIC_PRIORITY (PLIC0 + 0x0)
#define PLIC_PENDING (PLIC0 + 0x1000)
#define PLIC_MENABLE (PLIC0 + 0x2000)
#define PLIC_SENABLE (PLIC0 + 0x2080)
#define PLIC_MPRIORITY (PLIC0 + 0x200000)
#define PLIC_SPRIORITY (PLIC0 + 0x201000)
#define PLIC_MCLAIM (PLIC0 + 0x200004)
#define PLIC_SCLAIM (PLIC0 + 0x201004)

#endif