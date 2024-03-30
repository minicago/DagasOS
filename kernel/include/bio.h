#ifndef __BIO__H__
#define __BIO__H__

#include "types.h"
#include "buf.h"

void block_cache_init(void);
struct buf *read_block(uint32, uint32);
void release_block(struct buf *b);
void write_block(struct buf *b);
void flush_block(struct buf *b);
void pin_block(struct buf *);
void unpin_block(struct buf *);
void read_to_buffer(uint32 dev, uint32 block_id, uint32 cnt, void* buffer);
void write_to_disk(uint32 dev, uint32 block_id, uint32 cnt, void* buffer);
void read_bytes_to_buffer(uint32 dev, uint32 block_id, int offset, int size, void* buffer);
void flush_cache_to_disk();

#endif