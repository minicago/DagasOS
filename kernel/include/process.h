#ifndef __PROCESS__H__
#define __PROCESS__H__

#include "dagaslib.h"
#include "vmm.h"
#include "spinlock.h"
#include "fs.h"
#include "file.h"

#define MAX_PROCESS 256

#define USER_ENTRY 0x00000000ull
#define USER_EXIT 0x80000000ull

#define MAX_FD 16

#define FD_STDIN 0
#define FD_STDOUT 1
#define FD_STDERR 2

enum PROCESS_STATE {
    UNUSED,
    USED,
    ZOMBIE,
};

typedef struct process_struct {
    spinlock_t lock;
    // basic information
    enum PROCESS_STATE state;
    int64 pid;
    pagetable_t pagetable;

    // sub process
    struct process_t* parent;
    // linked_list_t* child; // current wo don't save children.

    // threads;
    int thread_count;
    file_t *open_files[MAX_FD];  // Open files
    inode_t *cwd;           // Current directory
} process_t;

extern process_t process_pool[];

void process_pool_init();
void init_process(process_t* process);
process_t* alloc_process();
void free_process(process_t* process);
void map_elf(process_t* process);
process_t* get_current_proc(void);

#endif