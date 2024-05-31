#ifndef __MEMORY__LAYOUT__H__
#define __MEMORY__LAYOUT__H__

// optional setting

#include "pmm.h"
#include "defs.h"
//#include "thread.h"
#include "vmm.h"

#ifndef KMEMORY
#define KMEMORY 128 * 1024 * 1024
#endif

#ifndef CPUS
#define CPUS 1
#endif

#define KSTACK_SIZE 0x40 * PG_SIZE

#define PG_SIZE 4096

#define PG_OFFSET_SHIFT 12

#define PG_OFFSET_MASK 0xfffull

// physic layout

#define PLIC 0x0c000000ull

#define UART0 0x10000000ull

// virtio mmio interface
#define VIRTIO0 0x10001000ull

#define CLINT0 0x2000000ull

#define KERNEL0 0x80000000ull

#define INITRDIMG0 0x84200000ull

#ifndef INITRDIMG_SIZE

#define INITRDIMG_SIZE 1024*128*128

#endif

#define PLIC0 0x0c000000ull

#define PMEM0 (uint64)(pmem_base)

#define MAX_PA (KERNEL0 + KMEMORY)

// kernel virtual layout


#define HEAP_SPACE 0x100000000ull

#define HEAP_SIZE 0x080000000ull

#define CORO_SPACE 0x200000000ull

#define CORO_INTERVAL 0x10000ull

#define COROSTACK_OFFSET (CORO_INTERVAL - MAX_COROSTACK_SIZE)

#define MAX_COROSTACK_SIZE PG_SIZE

#define COROSTACK0(tid) (CORO_SPACE + CORO_INTERVAL * tid + COROSTACK_OFFSET) 

#define COROSTACK_BOTTOM(tid) (CORO_SPACE + CORO_INTERVAL * tid + COROSTACK_OFFSET + MAX_COROSTACK_SIZE) 

#define TRAPFRAME0(tid) (COROSTACK_BOTTOM(tid) - sizeof(trapframe_t))

#define TRAMPOLINE (MAX_VA - PG_SIZE)

#define MAX_VA (1ull << (9 + 9 + 9 + 12 - 1))

// user virtual layout in same place

#define TSTACK_PAGETABLE_INDEX (TSTACK0 / PG_SIZE / PTE_NUM / PTE_NUM)

#define TSTACK0 0x300000000ull

#define TSTACK_BOTTOM (TSTACK0 + MAX_TSTACK_SIZE)
                       
#define MAX_TSTACK_SIZE 0x40000000ull

#define ARG_PAGE TSTACK_BOTTOM

//TRAMPOLINE here



#define PG_CEIL(addr) (((addr) + PG_SIZE - 1) & ~(PG_SIZE - 1))

#define PG_FLOOR(addr) ((addr) & ~(PG_SIZE - 1))

#endif