#include "file.h"
#include "spinlock.h"
#include "vmm.h"
#include "process.h"
#include "cpu.h"
#include "memory_layout.h"

static file_t files[MAX_FILE];
static spinlock_t files_lock;

dev_t devices[MAX_DEV];

void files_init()
{
    for (int i = 0; i < MAX_FILE; i++)
    {
        files[i].type = T_NONE;
        files[i].refcnt = 0;
    }
    init_spinlock(&files_lock);
}

// will assign refcnt to 1
file_t *file_alloc()
{
    acquire_spinlock(&files_lock);
    for (int i = 0;i < MAX_FILE; i++)
    {
        if (files[i].refcnt == 0)
        {
            files[i].refcnt = 1;
            release_spinlock(&files_lock);
            return &files[i];
        }
    }
    release_spinlock(&files_lock);
    panic("file_alloc: no free file block");
    return NULL;
}

void file_close(file_t *file)
{
    //file_t tmp;
    //tmp = *file;
    acquire_spinlock(&files_lock);
    if (file->refcnt == 0)
    {
        release_spinlock(&files_lock);
        panic("file_close: file->refcnt == 0");
    }
    file->refcnt--;
    if (file->refcnt == 0)
    {
        file->type = T_NONE;
    }
    release_spinlock(&files_lock);
    if (file->type == T_FILE || file->type == T_DEVICE)
    {

        // TODO: use tmp to fresh inode
    }
}

// the buf addr is in user space
int file_read(file_t *file, uint64 va, int size)
{
    if (file->readable == 0)
    {
        printf("file_read: file->readable == 0\n");
        return -1;
    }
    if (file->type == T_DEVICE)
    {
        return devices[file->major].read(1, va, size);
    }
    else if (file->type == T_FILE)
    {
        pagetable_t pagetable = get_current_proc()->pagetable;
        uint64 va0 = PG_FLOOR(va);
        uint64 pa0 = PTE2PA(walk(pagetable, va, 0));
        if(pa0==0) return -1;
        int offset = va - va0;
        int n = PG_SIZE - offset;
        uint64 pa = pa0 + offset;
        int real_size = 0;
        while (size > 0)
        {
            if(size<n) n = size;
            real_size += read_inode(file->node, file->off, n, (void *)pa);
            pa += n;
            file->off+=n;
            offset = 0;
            size-=n;
            n = PG_SIZE;
        }
        return real_size;
    } else if(file->type == T_PIPE)
    {
        panic("file_read: T_PIPE not implemented");
        return -1;
    } else {
        panic("file_read: file->type error");
        return -1;
    }
}

int file_write(file_t *file, uint64 va, int size) {
    printf("file_write: file->type = %d major = %d\n", file->type, file->major);
    if (file->writable == 0) {
        printf("file_write: file->writable == 0\n");
        return -1;
    }
    if (file->type == T_DEVICE) {
        return devices[file->major].write(1, va, size);
    } else if (file->type == T_FILE) {
        panic("file_write: T_FILE not implemented");
        return -1;
        // pagetable_t pagetable = get_current_proc()->pagetable;
        // uint64 va0 = PG_FLOOR(va);
        // uint64 pa0 = PTE2PA(walk(pagetable, va, 0));
        // if(pa0==0) return -1;
        // int offset = va - va0;
        // int n = PG_SIZE - offset;
        // uint64 pa = pa0 + offset;
        // int real_size = 0;
        // while (size > 0) {
        //     if(size<n) n = size;
        //     real_size += write_inode(file->node, file->off, n, (void *)pa);
        //     pa += n;
        //     file->off+=n;
        //     offset = 0;
        //     size-=n;
        //     n = PG_SIZE;
        // }
        // return real_size;
    } else if(file->type == T_PIPE) {
        panic("file_write: T_PIPE not implemented");
        return -1;
    } else {
        panic("file_write: file->type error");
        return -1;
    }
}

file_t* file_create_by_inode(inode_t *node)
{
    file_t *file = file_alloc();
    if(file == NULL) return NULL;
    acquire_spinlock(&files_lock);
    file->type = node->type;
    file->node = node;
    file->off = 0;
    if(file->type==T_DEVICE) {
        file->major = node->major;
    } else {
        file->major = -1;
    }
    release_spinlock(&files_lock);
    return file;
}

int file_dup(file_t *file)
{
    acquire_spinlock(&files_lock);
    if(file->refcnt <= 0)
    {
        release_spinlock(&files_lock);
        panic("file_dup: file->refcnt <= 0");
        return -1;
    }
    file->refcnt++;
    release_spinlock(&files_lock);
    return 0;
}