#include "types.h"
#include "memory_layout.h"
#include "plic.h"
#include "cpu.h"
#include "virtio_disk.h"

//
// the riscv Platform Level Interrupt Controller (PLIC).
//

void plic_init(void)
{
  // set desired IRQ priorities non-zero (otherwise disabled).
  *(uint32*)(PLIC + UART0_IRQ*4) = 1;
  *(uint32*)(PLIC + VIRTIO0_IRQ*4) = 1;
}

void plic_init_hart(void)
{
  
  int hart = get_cpu_id();
  
  // set enable bits for this hart's S-mode
  // for the uart and virtio disk.
  *(uint32*)PLIC_S_ENABLE(hart) = (1 << UART0_IRQ) | (1 << VIRTIO0_IRQ);

  // set this hart's S-mode priority threshold to 0.
  *(uint32*)PLIC_S_PRIORITY(hart) = 0;
}

// ask the PLIC what interrupt we should serve.
int plic_claim(void)
{
  int hart = get_cpu_id();
  int irq = *(uint32*)PLIC_S_CLAIM(hart);
  return irq;
}

// tell the PLIC we've served this IRQ.
void plic_complete(int irq)
{
  int hart = get_cpu_id();
  *(uint32*)PLIC_S_CLAIM(hart) = irq;
}
