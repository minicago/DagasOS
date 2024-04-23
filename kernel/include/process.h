#ifndef __PROCESS__H__
#define __PROCESS__H__

#include "dagaslib.h"
#include "vmm.h"
#include "spinlock.h"
#include "fs.h"
#include "file.h"
#include "waitqueue.h"

typedef struct wait_queue_struct wait_queue_t;

#define MAX_PROCESS 256

// TIP: To satisfy the requirement of test's entry
#define USER_ENTRY 0x00001000ull
#define USER_EXIT 0x80000000ull

#define MAX_FD 256

#define FD_STDIN 0
#define FD_STDOUT 1
#define FD_STDERR 2

enum PROCESS_STATE {
    UNUSED,
    USED,
    ZOMBIE,
};

typedef struct process_struct process_t;
struct process_struct{
    spinlock_t lock;
    // basic information
    enum PROCESS_STATE state;
    int64 pid;
    wait_queue_t *wait_child, *wait_self;



    vm_t* vm_list;
    vm_t* arg_vm;
    vm_t* heap_vm;

    pagetable_t pagetable;

    // sub process
    process_t* parent;
    process_t* child_list;
    process_t **prev, *next;

    // threads;
    int thread_count;
    file_t *open_files[MAX_FD];  // Open files
    inode_t *cwd;           // Current directory
};

extern process_t process_pool[];
extern spinlock_t wait_lock;

void process_pool_init();
void init_process(process_t* process);
process_t* alloc_process();
void free_process(process_t* process);
void map_elf(process_t* process);
process_t* get_current_proc(void);
int create_fd(process_t* process, file_t* file);
void set_arg(process_t* process, int argc, char** argv);
void prepare_initcode_process(process_t* process);
process_t* fork_process(process_t* process);
void exec_process(process_t* process, char* path);

void release_zombie(process_t* process);
void release_process(process_t* process);

#endif