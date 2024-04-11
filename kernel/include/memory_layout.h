#ifndef __MEMORY__LAYOUT__H__
#define __MEMORY__LAYOUT__H__

// optional setting

#include "pmm.h"
#include "defs.h"
//#include "thread.h"
#include "vmm.h"

#define KSTACK_SIZE 0x40 * PG_SIZE

#define PG_SIZE 4096

#define PG_OFFSET_SHIFT 12

#define PG_OFFSET_MASK 0xfff

// physic layout

#define PLIC 0x0c000000ull

#define UART0 0x10000000ull

// virtio mmio interface
#define VIRTIO0 0x10001000ull

#define CLINT0 0x2000000ull

#define KERNEL0 0x80000000ull

#define PLIC0 0x0c000000ull

#define PMEM0 (uint64) pmem_base

#define MAX_PA (KERNEL0 + KMEMORY)

// kernel virtual layout

#define THREAD_SPACE 0x200000000ull

#define THREAD_INTERVAL 0x10000ull

#define COROSTACK_OFFSET (THREAD_INTERVAL - MAX_COROSTACK_SIZE)

#define MAX_COROSTACK_SIZE PG_SIZE

#define COROSTACK0(tid) (THREAD_SPACE + THREAD_INTERVAL * tid + COROSTACK_OFFSET) 

#define COROSTACK_BOTTOM(tid) (THREAD_SPACE + THREAD_INTERVAL * tid + COROSTACK_OFFSET + MAX_COROSTACK_SIZE) 

#define TRAPFRAME0(tid) (COROSTACK_BOTTOM(tid) - sizeof(trapframe_t))

#define TRAMPOLINE (MAX_VA - PG_SIZE)

#define MAX_VA (1ull << (9 + 9 + 9 + 12 - 1))

// user virtual layout in same place

#define TSTACK_OFFSET 0x0ull

#define MAX_TSTACK_SIZE (THREAD_INTERVAL - MAX_COROSTACK_SIZE)

#define TSTACK0(tid) (THREAD_SPACE + THREAD_INTERVAL * tid + TSTACK_OFFSET) 

#define TSTACK_BOTTOM(tid) (THREAD_SPACE + THREAD_INTERVAL * tid + TSTACK_OFFSET + MAX_TSTACK_SIZE) 

//TRAMPOLINE here





#define PG_CEIL(addr) (((addr) + PG_SIZE - 1) & ~(PG_SIZE - 1))

#define PG_FLOOR(addr) ((addr) & ~(PG_SIZE - 1))

#endif