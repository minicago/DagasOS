#ifndef __MEMORY_LAYOUT__H__
#define __MEMORY_LAYOUT__H__

#define S_MEM_START 0x80000000L
#define PMEM_END ((void *)(S_MEM_START + 128 * 1024 * 1024))

#define PG_SIZE 4096

#define PG_CEIL(addr) (((addr) + PG_SIZE - 1) & ~(PG_SIZE - 1))

#endif