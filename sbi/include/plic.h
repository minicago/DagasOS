#ifndef __PLIC__H__
#define __PLIC__H__

#include "memory_layout.h"

#define PLIC_PRIORITY (PLIC0 + 0x0)
#define PLIC_PENDING (PLIC0 + 0x1000)
#define PLIC_M_ENABLE(hart) (PLIC0 + 0x2000  + (hart)*0x100)
#define PLIC_S_ENABLE(hart) (PLIC0 + 0x2080 + (hart)*0x100)
#define PLIC_M_PRIORITY(hart) (PLIC0 + 0x200000 + (hart)*0x2000)
#define PLIC_S_PRIORITY(hart) (PLIC0 + 0x201000 + (hart)*0x2000)
#define PLIC_M_CLAIM(hart) (PLIC0 + 0x200004 + (hart)*0x2000)
#define PLIC_S_CLAIM(hart) (PLIC0 + 0x201004 + (hart)*0x2000)

void plic_init(void);

void plic_init_hart(void);
// ask the PLIC what interrupt we should serve.
int plic_claim(void);
// tell the PLIC we've served this IRQ.
void plic_complete(int irq);

#endif