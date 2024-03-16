#ifndef __FILE__H__
#define __FILE__H__

#include "types.h"

#define MAX_INODE 128

#define T_DIR 1    // Directory
#define T_FILE 2   // File
#define T_DEVICE 3 // Device

#define FS_TYPE_NULL 0
#define FS_TYPE_FAT32 1

struct inode_struct;
typedef struct inode_struct inode_t;
typedef struct superblock_struct
{
  uint32 dev;
  uint32 block_size;
  uint32 fs_type;
  void *extra;

  //ops
  int (*lookup_inode)(inode_t *dir, char *filename, inode_t *node);
  int (*read_inode)(inode_t *node, int offset, int size, void *buffer);
} superblock_t;

struct inode_struct{
  uint32 dev; // Device number
  uint32 id;  // Inode number
  int refcnt; // Reference count
  int nlink;
  // struct sleeplock lock; // protects everything below here
  int valid;   // inode has been read from disk?
  uint16 type; // copy of disk inode
  uint32 size;
  superblock_t *sb;
};

extern inode_t root;

void filesystem_init(uint32 type);
inode_t* lookup_inode(inode_t *dir, char *filename);
void release_inode(inode_t *node);
int read_inode(inode_t *node, int offset, int size, void *buffer);
int file_test();

#endif