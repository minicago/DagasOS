#ifndef __PALLOC_H__
#define __PALLOC_H__

#include "dagaslib.h"

#include "memory_layout.h"
#include "types.h"
#include "print.h"
#include "defs.h"

void pmem_init();
void pfree(void *pa);
void *palloc();
void p_range_free(void *start, void *end);

#endif