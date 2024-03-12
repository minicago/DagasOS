#ifndef __MEMORY__LAYOUT__H__
#define __MEMORY__LAYOUT__H__

// optional setting

#include "pmm.h"
#include "defs.h"

#define QEMU_MEMORY_SIZE 128 * 1024 * 1024

#define PG_SIZE 4096

#define PG_OFFSET_SHIFT 12

#define PG_OFFSET_MASK 0xfff

// physic layout

#define UART0 0x10000000ull

// virtio mmio interface
#define VIRTIO0 0x10001000ull
#define VIRTIO0_IRQ 1

#define CLINT0 0x2000000ull

#define KERNEL0 0x80000000ull

#define PLIC0 0x0c000000ull

#define PMEM0 (uint64) pmem_base

#define MAX_PA (KERNEL0 + QEMU_MEMORY_SIZE)

// kernel virtual layout



#define TRAMPOLINE (MAX_VA - PG_SIZE)

#define MAX_VA (1ull << (9 + 9 + 9 + 12 - 1))

// user virtual layout in same place

#define TSTACK00 0x200000000ull

#define MAX_TSTACK_SIZE 0x10000ull

#define TSTACK0(tid) TSTACK00 + MAX_TSTACK_SIZE * tid


//TRAMPOLINE here





#define PG_CEIL(addr) (((addr) + PG_SIZE - 1) & ~(PG_SIZE - 1))

#define PG_FLOOR(addr) ((addr) & ~(PG_SIZE - 1))

#endif