#ifndef __FILE__H__
#define __FILE__H__

#include "fs.h"
#include "types.h"
#include "vmm.h"

#define MAX_INODE 128
#define MAX_FILE 256
#define MAX_DEV 16
#define RESERVED_FILE_CNT 10

#define T_DIR 1    // Directory
#define T_FILE 2   // File
#define T_DEVICE 3 // Device

#define CONSOLE_DEV 1

struct file_struct;
typedef struct file_struct file_t;

struct dev_struct;
typedef struct dev_struct dev_t;

struct file_struct
{
    enum
    {
        FD_NONE,
        FD_PIPE,
        FD_INODE,
        FD_DEVICE
    } type;
    uint32 refcnt; // reference count
    uint8 readable;
    uint8 writable;
    // struct pipe *pipe; // FD_PIPE
    inode_t *node;   // FD_INODE and FD_DEVICE
    uint32 off;    // FD_INODE
    uint8 major; // FD_DEVICE
};

struct dev_struct {
  int (*read)(int is_user, uint64 addr, int size);
  int (*write)(int is_user, uint64 addr, int size);
};

extern dev_t devices[];

#endif