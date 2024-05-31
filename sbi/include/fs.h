// On-disk file system format.
// Both the kernel and user programs use this header file.
#ifndef __FS__H__
#define __FS__H__

#include "types.h"
#include "vmm.h"

#define BSIZE 512              // block size
#define MAXOPBLOCKS 10         // max # of blocks any FS op writes
#define NBUF (MAXOPBLOCKS * 3) // size of disk block cache
#define NULL_DEV 0
#define VIRTIO_DISK_DEV 1

#define FS_TYPE_NULL 0
#define FS_TYPE_DISK 1
#define FS_TYPE_FAT32 2

struct inode_struct;
typedef struct inode_struct inode_t;

struct dirent_struct;
typedef struct dirent_struct dirent_t;

struct superblock_struct;
typedef struct superblock_struct superblock_t;

struct superblock_struct
{
  superblock_t *parent;// if it is NULL, this is a real dev. otherwise, this references the
  // virtual dev's parent.
  uint32 identifier;
  uint32 block_size;
  uint32 fs_type;
  union {
    uint32 id_in_parent; //when virtual dev
    uint32 real_dev; // when real dev
  };
  int root_id;

  void *extra;

  // ops, node.refcnt = 0 but others is proper. return 0 if can't find
  int (*lookup_inode)(inode_t *dir, char *filename, inode_t *node);

  // return the real size. return -1 is error
  int (*read_inode)(inode_t *node, int offset, int size, void *buffer);

  int (*write_inode)(inode_t *node, int offset, int size, int cover, void *buffer);

  void (*update_inode)(inode_t *node);

  // return 0 is error
  int (*create_inode)(inode_t *dir, char *filename, uint8 type, uint8 major, inode_t *inode);

  void (*print_fs_info)(inode_t *node);
  int (*get_dirent)(inode_t *node, int size, dirent_t *dirent);
  int (*get_inode_name)(inode_t* node, char* buffer, int size);
  int (*get_bid_by_id)(superblock_t *sb, int id);
  int (*get_bid_from_son)(superblock_t *sb, int son_id, int bid);
  int (*free_extra)(void *extra);
};

struct inode_struct
{
  uint32 id;  // Inode number
  int refcnt; // Reference count
  int nlink;
  // struct sleeplock lock; // protects everything below here
  int valid;   // inode has been read from disk?
  uint16 type; // copy of disk inode
  uint32 size;
  uint8 major;
  superblock_t *sb;
  superblock_t *mnt_sb;
  int mnt_root_id;
  inode_t *parent;
  int index_in_parent;
  int is_mnt;
};


struct dirent_struct
{
  uint64 d_ino;
  int64 d_off;
  unsigned short d_reclen;
  unsigned char d_type;
  char d_name[];
};

// root(/) inode, can only be used after filesystem_init called
inode_t *get_root();
inode_t* get_inode(superblock_t* sb, uint32 id);
// will init inode cache, root inode's superblock and root inode
void filesystem_init(uint32 type);
// will lookup inode in dir, and return inode
inode_t *lookup_inode(inode_t *dir, char *filename);
// once inode is not used, release it
void release_inode(inode_t *node);
// read inode's data to buffer, the unit of offset is byte
int read_inode(inode_t *node, int offset, int size, void *buffer);

int write_inode(inode_t *node, int offset, int size, int cover, void *buffer);

void update_inode(inode_t *node);

inode_t *create_inode(inode_t *dir, char *filename, uint8 major, uint8 type);

void print_inode(inode_t *node);

void print_fs_info(inode_t *node);

int file_test();

int load_from_inode_to_page(inode_t *inode, pagetable_t pagetable, uint64 va, int offset, int size);

inode_t *look_up_path(inode_t *root, const char *path, int *depth);

void pin_inode(inode_t *node);

// return -1 if error
int get_dirent(inode_t *node, int size, dirent_t *dirent);

// return len of path, return 0 when '/', return -1 when error
int get_inode_path(inode_t *node, char *buf, int size);
int get_inode_name(inode_t* node, char* buffer, int size);

int get_real_dev(superblock_t *sb);

int get_real_bid(superblock_t *sb, int bid);

int mount_inode(inode_t *dir, superblock_t *sb);
int free_superblock(superblock_t* sb);
int umount_inode(inode_t *node);
int get_new_sb_identifier();
superblock_t* alloc_superblock();
#endif