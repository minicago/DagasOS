#ifndef __FILE__H__
#define __FILE__H__

#include "fs.h"
#include "types.h"
#include "vmm.h"

#define MAX_INODE 128
#define MAX_FILE 256
#define MAX_DEV 16
#define RESERVED_FILE_CNT 10

#define T_NONE 0   // None
#define T_DIR 1    // Directory
#define T_FILE 2   // File
#define T_DEVICE 3 // Device
#define T_PIPE 4   // Pipe

#define CONSOLE_DEV 1

#define O_RDONLY 0x000
#define O_WRONLY 0x001
#define O_RDWR 0x002 // 可读可写
//#define O_CREATE 0x200
#define O_CREATE 0x40
#define O_DIRECTORY 0x0200000

#define AT_FDCWD -100

struct file_struct;
typedef struct file_struct file_t;

struct dev_struct;
typedef struct dev_struct dev_t;

struct file_struct
{
    uint8 type;
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

void files_init();
void file_close(file_t *file);
int file_read(file_t *file, uint64 va, int size);
int file_write(file_t *file, uint64 va, int size);
file_t* file_create_by_inode(inode_t *node);
int file_dup(file_t *file);
file_t* file_openat(inode_t *dir_node, const char *path, int flags, int mode);

#endif