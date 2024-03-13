#include "types.h"
#include "spinlock.h"
#include "fs.h"
#include "buf.h"
#include "bio.h"
#include "print.h"
#include "virtio_disk.h"

struct
{
  spinlock_t *lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} block_cache;

void block_cache_init(void)
{
  struct buf *b;

  init_spinlock(block_cache.lock);

  // Create linked list of buffers
  block_cache.head.prev = &block_cache.head;
  block_cache.head.next = &block_cache.head;
  for (b = block_cache.buf; b < block_cache.buf + NBUF; b++)
  {
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
static struct buf *get_block(uint32 dev, uint32 block_idx)
{
  struct buf *b;

  acquire_spinlock(block_cache.lock);

  // Is the block already cached?
  for (b = block_cache.head.next; b != &block_cache.head; b = b->next)
  {
    if (b->dev == dev && b->block_idx == block_idx)
    {
      b->refcnt++;
      release_spinlock(block_cache.lock);
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
      b->dev = dev;
      b->block_idx = block_idx;
      b->valid = 0;
      b->refcnt = 1;
      b->disk = 0;
      release_spinlock(block_cache.lock);
      // acquiresleep(&b->lock);
      return b;
    }
  }
  panic("get_block: no buffers");
  return NULL;
}

// Return a locked buf with the contents of the indicated block.
struct buf *read_block(uint32 dev, uint32 block_idx)
{
  struct buf *b;
  printf("read_block: read");
  b = get_block(dev, block_idx);
  if (!b->valid)
  {
    printf("read_block: vir read");
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
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void release_block(struct buf *b)
{
  // if(!holdingsleep(&b->lock))
  //   panic("brelse");

  // releasesleep(&b->lock);

  acquire_spinlock(block_cache.lock);
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

  release_spinlock(block_cache.lock);
}

void pin_block(struct buf *b)
{
  acquire_spinlock(block_cache.lock);
  b->refcnt++;
  release_spinlock(block_cache.lock);
}

void unpin_block(struct buf *b)
{
  acquire_spinlock(block_cache.lock);
  b->refcnt--;
  release_spinlock(block_cache.lock);
}
