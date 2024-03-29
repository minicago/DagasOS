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
        files[i].type = FD_NONE;
        files[i].refcnt = 0;
    }
    init_spinlock(&files_lock);
}

file_t *file_alloc()
{
    acquire_spinlock(&files_lock);
    for (int i = RESERVED_FILE_CNT; i < MAX_FILE; i++)
    {
        if (files[i].refcnt == 0)
        {
            files[i].refcnt = 1;
            release_spinlock(&files_lock);
            return &files[i];
        }
    }
    release_spinlock(&files_lock);
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
        file->type = FD_NONE;
    }
    release_spinlock(&files_lock);
    if (file->type == FD_INODE || file->type == FD_DEVICE)
    {

        // TODO: use tmp to fresh inode
    }
}

int file_read(file_t *file, uint64 va, int size)
{
    if (file->readable == 0)
    {
        return -1;
    }
    if (file->type == FD_DEVICE)
    {
        return devices[file->major].read(1, va, size);
    }
    else if (file->type == FD_INODE)
    {
        pagetable_t pagetable = get_cpu()->thread->process->pagetable;
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
    }
    else
    {
        panic("file_read: file->type error");
        return -1;
    }
}
