#ifndef __BIO__H__
#define __BIO__H__

#include "types.h"
#include "buf.h"
#include "fs.h"

void block_cache_init(void);
struct buf *read_block(superblock_t *sb, uint32 block_id);
void release_block(struct buf *b);
void write_block(struct buf *b);
void flush_block(struct buf *b);
void pin_block(struct buf *);
void unpin_block(struct buf *);
void read_to_buffer(superblock_t *sb,  uint32 block_id, uint32 cnt, void* buffer);
void write_to_disk(superblock_t *sb, uint32 block_id, uint32 cnt, void* buffer);
void read_bytes_to_buffer(superblock_t *sb, uint32 block_id, int offset, int size, void* buffer);
void write_bytes_to_disk(superblock_t *sb, uint32 block_id, int offset, int size, void* buffer);
void flush_cache_to_disk();

#endif