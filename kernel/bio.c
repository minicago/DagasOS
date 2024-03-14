#include "types.h"
#include "spinlock.h"
#include "fs.h"
#include "buf.h"
#include "bio.h"
#include "print.h"
#include "virtio_disk.h"
#include "dagaslib.h"

struct
{
  spinlock_t lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} block_cache;

void block_cache_init(void)
{
  struct buf *b;

  init_spinlock(&block_cache.lock);

  // Create linked list of buffers
  block_cache.head.prev = &block_cache.head;
  block_cache.head.next = &block_cache.head;
  for (b = block_cache.buf; b < block_cache.buf + NBUF; b++)
  {
    b->dirty = 0;
    b->next = block_cache.head.next;
    b->prev = &block_cache.head;
    b->dev = NULL_DEV;
    // initsleeplock(&b->lock, "buffer");
    block_cache.head.next->prev = b;
    block_cache.head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *get_block(uint32 dev, uint32 block_id)
{
  struct buf *b;

  acquire_spinlock(&block_cache.lock);

  // Is the block already cached?
  for (b = block_cache.head.next; b != &block_cache.head; b = b->next)
  {
    if (b->dev == dev && b->block_id == block_id)
    {
      b->refcnt++;
      release_spinlock(&block_cache.lock);
      // acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for (b = block_cache.head.prev; b != &block_cache.head; b = b->prev)
  {
    if (b->refcnt == 0)
    {
      flush_block(b);
      b->dev = dev;
      b->block_id = block_id;
      b->valid = 0;
      b->refcnt = 1;
      b->disk = 0;
      release_spinlock(&block_cache.lock);
      // acquiresleep(&b->lock);
      return b;
    }
  }
  panic("get_block: no buffers");
  return NULL;
}

// Return a locked buf with the contents of the indicated block.
struct buf *read_block(uint32 dev, uint32 block_id)
{
  struct buf *b;
  b = get_block(dev, block_id);
  if (!b->valid)
  {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void write_block(struct buf *b)
{
  // if(!holdingsleep(&b->lock))
  //   panic("bwrite");
  b->dirty = 1;
}

// Write b's contents to disk.  Must be locked.
void flush_block(struct buf *b)
{
  // if(!holdingsleep(&b->lock))
  //   panic("bwrite");
  if(b->dirty) {
    virtio_disk_rw(b, 1);
    b->dirty = 0;
  }
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void release_block(struct buf *b)
{
  // if(!holdingsleep(&b->lock))
  //   panic("brelse");

  // releasesleep(&b->lock);

  acquire_spinlock(&block_cache.lock);
  b->refcnt--;
  if (b->refcnt == 0)
  {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = block_cache.head.next;
    b->prev = &block_cache.head;
    block_cache.head.next->prev = b;
    block_cache.head.next = b;
  }

  release_spinlock(&block_cache.lock);
}

void pin_block(struct buf *b)
{
  acquire_spinlock(&block_cache.lock);
  b->refcnt++;
  release_spinlock(&block_cache.lock);
}

void unpin_block(struct buf *b)
{
  acquire_spinlock(&block_cache.lock);
  b->refcnt--;
  release_spinlock(&block_cache.lock);
}

void read_to_buffer(uint32 dev, uint32 block_id, uint32 cnt, void* buffer)
{
  struct buf *b;
  for(int i=0;i<cnt;i++){
    b = read_block(dev, block_id+i);
    memcpy(buffer+i*BSIZE, b->data, BSIZE);
    release_block(b);
  }
}

void flush_cache_to_disk() {
  struct buf *b;
  acquire_spinlock(&block_cache.lock);
  for (b = block_cache.head.next; b != &block_cache.head; b = b->next)
  {
    flush_block(b);
  }
  release_spinlock(&block_cache.lock);
}