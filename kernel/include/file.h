#ifndef __FILE__H__
#define __FILE__H__

#include "types.h"

#define MAX_INODE 128

struct inode {
  uint32 dev;           // Device number
  uint32 id;          // Inode number
  int refcnt;            // Reference count
  int nlink;
  // struct sleeplock lock; // protects everything below here
  int valid;          // inode has been read from disk?
  uint16 type;         // copy of disk inode
  uint32 size;
};

struct superblock_ops {
    int (*read_inode)(struct inode *inode);
    int (*write_inode)(struct inode *inode);
    int (*create_inode)(struct inode *inode);
    int (*delete_inode)(struct inode *inode);
    int (*read_block)(uint32 block, char *buf);
    int (*write_block)(uint32 block, char *buf);
    int (*create_block)(uint32 block);
    int (*delete_block)(uint32 block);
};

struct superblock {
    uint32 dev;
    uint32 block_size;
    struct superblock_ops* ops;
    void* extra;
};
#endif