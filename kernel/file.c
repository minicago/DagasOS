#include "file.h"
#include "spinlock.h"
#include "vmm.h"
#include "process.h"
#include "cpu.h"
#include "memory_layout.h"
#include "dagaslib.h"
#include "fs.h"
#include "coro.h"
#include "bio.h"
#include "fat32.h"


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
    printf("file_close: file->refcnt = %d\n", file->refcnt);
    if (file->refcnt == 0)
    {
        //TODO: what should do when T_DEVICE
        if(file->type == T_FILE)
        {
            release_inode(file->node);
        }
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
        pagetable_t pagetable = thread_pool[get_tid()].stack_pagetable;
        int real_size = 0;
        
        if (file->off + size > file->node->size)
        {
            size = (file->node->size - file->off);
        }
        
        while (size > 0)
        {
            uint64 va0 = PG_FLOOR(va);
            uint64 pa0 = va2pa(pagetable,va0);
            if(pa0==0) return -1;
            int offset = va - va0;
            int n = PG_SIZE - offset;
            uint64 pa = pa0 + offset;
            if(size<n) n = size;
            printf("file_read: file->off = %d %d\n", file->off,n);
            real_size += read_inode(file->node, file->off, n, (void *)pa);
            file->off+=n;
            size-=n;
            va = va0 + PG_SIZE;
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
        pagetable_t pagetable = thread_pool[get_tid()].stack_pagetable;
        int real_size = 0;
        int cover = 1;
        while (size > 0)
        {
            uint64 va0 = PG_FLOOR(va);
            uint64 pa0 = va2pa(pagetable,va0);
            if(pa0==0) return -1;
            int offset = va - va0;
            int n = PG_SIZE - offset;
            uint64 pa = pa0 + offset;
            if(size<n) n = size;
            printf("file_write: file->off = %d %d\n", file->off,n);
            real_size += write_inode(file->node, file->off, n,cover, (void *)pa);
            file->off+=n;
            size-=n;
            va = va0 + PG_SIZE;
        }
        return real_size;
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
    pin_inode(node);
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

file_t* file_openat(inode_t *dir_node, const char *path, int flags, int mode)
{
    if(dir_node==NULL) return NULL;
    if(dir_node->type!=T_DIR) return NULL;
    int depth;
    inode_t *res =  look_up_path(dir_node, path, &depth);
    if(res==NULL) {
        if(flags &  O_CREATE) {
            int all_depth = get_file_depth(path);
            if(all_depth!=depth+1) return NULL;
            char *mem = palloc();
            char *npath = mem;
            char *filename = mem+MAX_PATH;
            strcpy(npath, path);
            remove_last_file(npath);
            get_last_file(path, filename);
            if(npath[0]!='\0') dir_node = look_up_path(dir_node, npath, NULL);
            if(flags & O_DIRECTORY) {
                res = create_inode(dir_node, filename, 0, T_DIR);
            } else {
                res = create_inode(dir_node, filename, 0, T_FILE);
            }
            release_inode(dir_node);
            pfree(mem);
            if(res==NULL) return NULL;
        } else {
            return NULL;
        }
    }
    file_t* file = file_create_by_inode(res);
    file->flags = flags;
    if(flags & O_APPEND) {
        file->off = res->size;
    } else {
        file->off = 0;
    }
    if(mode & O_RDONLY) {
        file->readable = 1;
        file->writable = 0;
    }
    if(mode & O_WRONLY) {
        file->readable = 0;
        file->writable = 1;
    }
    if(mode & O_RDWR) {
        file->readable = 1;
        file->writable = 1;
    }
    release_inode(res);
    return file;
}

int file_mkdirat(inode_t *dir_node, const char *path, int mode)
{
    if(dir_node==NULL) return -1;
    if(dir_node->type!=T_DIR) return -1;
    char *mem = palloc();
    char *npath = mem;
    char *filename = mem+MAX_PATH;
    strcpy(npath, path);
    remove_last_file(npath);
    get_last_file(path, filename);
    int len = strlen(filename);
    if(filename[len-1]=='/') filename[len-1] = '\0';
    if(npath[0]!='\0') dir_node = look_up_path(dir_node, npath, NULL);
    if(dir_node==NULL) goto file_mkdirat_error;
    inode_t *res = create_inode(dir_node, filename, 0, T_DIR);
    release_inode(res);
    release_inode(dir_node);
    if(res==NULL) goto file_mkdirat_error;
    pfree(mem);
    return 0;

file_mkdirat_error:
    pfree(mem);
    return -1;
}

int get_file_path(file_t *file, char *buf, int size)
{
    if(file->type == T_PIPE) {
        printf("get_file_path: pipe has no path\n");
        return -1;
    }
    inode_t *node = file->node;
    if(node==NULL) {
        printf("get_file_path: file's node is NULL\n");
        return -1;
    }
    return get_inode_path(node,buf,size);
}

void install_initrd_img(){
    inode_t* inode = lookup_inode(get_root(), "initrd.img");
    LOG("max_pa:%p\n",MAX_PA);
    if (inode == NULL) {
        inode = create_inode(get_root(), "initrd.img", 0, T_FILE);
        write_inode(inode, 0, INITRDIMG_SIZE, 1, (void*) INITRDIMG0);

    }

    file_mkdirat(get_root(),"initrd_mnt",0);
    inode_t* mnt_inode = lookup_inode(get_root(),"initrd_mnt");
    print_inode(inode);
    superblock_t *sb = alloc_superblock();
    if(fat32_superblock_init(inode, inode->sb, sb, get_new_sb_identifier())==0) {
        panic("sys_mount: fat32_superblock_init error\n");
    }
    if(mount_inode(mnt_inode,sb)==0) {
        panic("sys_mount: mount_inode error\n");
    }

    release_inode(inode);
    release_inode(mnt_inode);
    flush_cache_to_disk();
}