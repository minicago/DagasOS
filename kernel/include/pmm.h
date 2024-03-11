#ifndef __PALLOC__H__
#define __PALLOC__H__

#include "dagaslib.h"

#include "memory_layout.h"
#include "types.h"
#include "print.h"
#include "defs.h"

#define PMEM_END ((void *)(MAX_PA))

extern char pmem_base[];

void pmem_init();
void pfree(void *pa);
void *palloc();
void p_range_free(void *start, void *end);

#endif