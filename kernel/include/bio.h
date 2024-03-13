#ifndef __BIO__H__
#define __BIO__H__

#include "types.h"
#include "buf.h"

void block_cache_init(void);
struct buf *read_block(uint32, uint32);
void release_block(struct buf *);
void write_block(struct buf *);
void pin_block(struct buf *);
void unpin_block(struct buf *);

#endif