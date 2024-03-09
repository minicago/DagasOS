#ifndef __MEMORY_LAYOUT__H__
#define __MEMORY_LAYOUT__H__

// optional setting

#define QEMU_MEMORY_SIZE 128 * 1024 * 1024

#define PG_SIZE 4096

#define MAX_VA (S_MEM_START + QEMU_MEMORY_SIZE)

// physic layout

#define UART0 0x10000000ull

#define CLINT 0x2000000ull

#define S_MEM_START 0x80000000ull

#define PLIC 0x0c000000ull

#define PMEM_END ((void *)(S_MEM_START + QEMU_MEMORY_SIZE))

// kernel virtual layout

#define TRAMPOLINE (MAX_VA - PG_SIZE)

// user virtual layout in same place


#define TRAPFRAME (TRAMPOLINE - PG_SIZE)

//TRAMPOLINE here





#define PG_CEIL(addr) (((addr) + PG_SIZE - 1) & ~(PG_SIZE - 1))

#endif