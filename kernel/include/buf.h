#ifndef __BUF__H__
#define __BUF__H__

#include "types.h"
#include "fs.h"

struct buf {
  int dirty;   //0: clean, 1: dirty
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint32 dev;
  uint32 block_id;
  // struct sleeplock lock;
  uint32 refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  uint8 data[BSIZE];
};

#endif