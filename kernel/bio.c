#include "types.h"
#include "spinlock.h"
#include "fs.h"
#include "buf.h"
#include "bio.h"
#include "print.h"
#include "virtio_disk.h"
#include "dagaslib.h"
#include "strap.h"

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
    b->wait_queue = alloc_wait_queue(WAIT_QUEUE_ONERELEASE);
  }
}


// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *get_block(superblock_t* sb, uint32 block_id)
{
  uint32 dev = get_real_dev(sb);
  block_id = get_real_bid(sb,block_id);
  if(block_id == -1) {
    panic("get_block: block_id error: -1");
  }
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
      
      release_spinlock(&block_cache.lock);
      if(b->dirty == 1) {
        
        flush_block(b);
      }
      


      acquire_spinlock(&block_cache.lock);
      
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
// the only api to get block from eternal
struct buf *read_block(superblock_t *sb, uint32 block_id)
{
  struct buf *b;
  
  b = get_block(sb, block_id);
  
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
  //virtio_disk_rw(b,1);
}

// Write b's contents to disk.  Must be locked. Please call only when b->dirty == 1
void flush_block(struct buf *b)
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

// cnt is block's count, not byte's count
void read_to_buffer(superblock_t *sb, uint32 block_id, uint32 cnt, void* buffer)
{
  struct buf *b;
  for(int i=0;i<cnt;i++){
    b = read_block(sb, block_id+i);
    memcpy(buffer+i*BSIZE, b->data, BSIZE);
    release_block(b);
  }
}

// cnt is block's count, not byte's count
void write_to_disk(superblock_t *sb, uint32 block_id, uint32 cnt, void* buffer)
{
  struct buf *b;
  for(int i=0;i<cnt;i++){
    b = read_block(sb, block_id+i);
    memcpy(b->data, buffer+i*BSIZE, BSIZE);
    write_block(b);
    release_block(b);
  }
}

void read_bytes_to_buffer(superblock_t *sb, uint32 block_id, int offset, int size, void* buffer)
{
  struct buf *b;
  block_id += offset/BSIZE;
  offset %= BSIZE;
  while(size>0) {
    b = read_block(sb, block_id);
    
    if(size>=BSIZE-offset) {
      memcpy(buffer, b->data+offset, BSIZE-offset);
      buffer+=BSIZE-offset;
      size-=BSIZE-offset;
    }
    else {
      memcpy(buffer, b->data+offset, size);
      buffer+=size;
      size-=size;
    }
    offset=0;
    
    release_block(b);
    block_id++;
  }
}

void write_bytes_to_disk(superblock_t *sb, uint32 block_id, int offset, int size, void* buffer)
{
  struct buf *b;
  block_id += offset/BSIZE;
  offset %= BSIZE;
  while(size>0) {
    b = read_block(sb, block_id);
    
    if(size>=BSIZE-offset) {
      memcpy(b->data+offset, buffer, BSIZE-offset);
      buffer+=BSIZE-offset;
      size-=BSIZE-offset;
    }
    else {
      memcpy(b->data+offset, buffer, size);
      buffer+=size;
      size-=size;
    }
    offset=0;
    write_block(b);
    release_block(b);
    block_id++;
  }
}

void flush_cache_to_disk() {
  struct buf *b;
  acquire_spinlock(&block_cache.lock);
  
  for (b = block_cache.head.next; b != &block_cache.head; b = b->next)
  {
    
    if(b->dirty) {
      release_spinlock(&block_cache.lock);
      flush_block(b);
      acquire_spinlock(&block_cache.lock);
    }
    //printf("flushed: b->block_id = %d, refcnt:%d\n", b->block_id, b->refcnt);
  } 
  release_spinlock(&block_cache.lock);
}