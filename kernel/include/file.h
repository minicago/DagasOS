#ifndef __FILE__H__
#define __FILE__H__

#include "types.h"
#include "vmm.h"

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

// root(/) inode, can only be used after filesystem_init called
extern inode_t root;

// will init inode cache, root inode's superblock and root inode
void filesystem_init(uint32 type);
// will lookup inode in dir, and return inode
inode_t* lookup_inode(inode_t *dir, char *filename);
// once inode is not used, release it
void release_inode(inode_t *node);
// read inode's data to buffer, the unit of offset is byte
int read_inode(inode_t *node, int offset, int size, void *buffer);

void print_inode(inode_t *node);

int file_test();

int load_from_inode_to_page(inode_t *inode, pagetable_t pagetable, uint64 va, int offset, int size);

inode_t* look_up_path(inode_t* root, char *path);
#endif