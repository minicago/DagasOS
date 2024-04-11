// On-disk file system format.
// Both the kernel and user programs use this header file.
#ifndef __FS__H__
#define __FS__H__

#include "types.h"
#include "vmm.h"

#define BSIZE 512  // block size
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define NULL_DEV 0
#define VIRTIO_DISK_DEV 1

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

  //ops, node.refcnt = 0 but others is proper. return 0 if can't find
  int (*lookup_inode)(inode_t *dir, char *filename, inode_t *node);

  // return the real size. return -1 is error
  int (*read_inode)(inode_t *node, int offset, int size, void *buffer);

  void (*update_inode)(inode_t *node);

  // return 0 is error
  int (*create_inode)(inode_t* dir, char* filename, uint8 type, uint8 major, inode_t* inode);

  void (*print_fs_info)(inode_t *node);
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
  uint8 major;
  superblock_t *sb;
  inode_t *parent;
  int index_in_parent;
};

// root(/) inode, can only be used after filesystem_init called
inode_t* get_root();
inode_t* get_inode(uint32 dev, uint32 id);
// will init inode cache, root inode's superblock and root inode
void filesystem_init(uint32 type);
// will lookup inode in dir, and return inode
inode_t* lookup_inode(inode_t *dir, char *filename);
// once inode is not used, release it
void release_inode(inode_t *node);
// read inode's data to buffer, the unit of offset is byte
int read_inode(inode_t *node, int offset, int size, void *buffer);

void update_inode(inode_t *node);

inode_t* create_inode(inode_t* dir, char* filename, uint8 major, uint8 type);

void print_inode(inode_t *node);

void print_fs_info(inode_t *node);

int file_test();

int load_from_inode_to_page(inode_t *inode, pagetable_t pagetable, uint64 va, int offset, int size);

inode_t* look_up_path(inode_t* root, const char *path, int *depth);

void pin_inode(inode_t* node);

#endif