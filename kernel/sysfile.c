#include "sysfile.h"
#include "process.h"
#include "file.h"
#include "spinlock.h"
#include "fs.h"
#include "pmm.h"
#include "coro.h"
#include "fat32.h"
#include "cpu.h"

// the buf addr is in user space
int sys_write(int fd, uint64 va, uint64 size)
{
    printf("sys_write: fd=%d, va=%p, size=%d\n", fd, va, size);
    if (fd < 0 || fd >= MAX_FD) {
        printf("sys_write: fd error\n");
        return -1;
    }
    if (size < 0) {
        printf("sys_write: n error\n");
        return -1;
    }
    process_t *proc = get_current_proc();
    if (proc->open_files[fd] == NULL) {
        printf("sys_write: fd points to NULL\n");
        return -1;
    }
    return file_write(proc->open_files[fd], va, size);
}

int sys_read(int fd, uint64 va, uint64 size)
{
    printf("sys_read: fd=%d, va=%p, size=%d\n", fd, va, size);
    if (fd < 0 || fd >= MAX_FD) {
        printf("sys_read: fd error\n");
        return -1;
    }
    if (size < 0) {
        printf("sys_read: n error\n");
        return -1;
    }
    process_t *proc = get_current_proc();
    if (proc->open_files[fd] == NULL) {
        printf("sys_read: fd points to NULL\n");
        return -1;
    }
    return file_read(proc->open_files[fd], va, size);
}

int sys_getcwd(uint64 va, uint64 size)
{
    printf("sys_getcwd: va=%p, size=%d\n", va, size);
    process_t *proc = get_current_proc();
    int p_num = PG_CEIL(size)/PG_SIZE;
    char* mem = palloc_n(p_num);
    char* buffer = mem;
    
    acquire_spinlock(&proc->lock);
    inode_t* node = proc->cwd;
    release_spinlock(&proc->lock);

    int len = get_inode_path(node,buffer,size);
    if(len==-1) 
        goto sys_getcwd_error;
    if(len==0) len = 1;
    len++;
    if(size<len) {
        printf("sys_getcwd: size is too small\n");
        goto sys_getcwd_error;
    }
    copy_to_va(thread_pool[get_tid()].stack_pagetable,va,buffer,len);
    pfree(mem);
    return va;

sys_getcwd_error:
    pfree(mem);
    return -1;
}

int sys_openat(int dirfd, uint64 va, int flags, int mode)
{
    printf("sys_openat: dirfd=%d, va=%p, flags=%d, mode=%d\n", dirfd, va, flags, mode);
    char* mem = palloc();
    char* path = mem;
    if (copy_string_from_user(va, path, MAX_PATH) < 0) {
        printf("sys_openat: copy_string_from_user error\n");
        goto sys_openat_error;
    }
    inode_t *dir_node;
    if(path[0]=='/') {
        dir_node = get_root();
        path++;
    } else {
        if(dirfd == AT_FDCWD) {
            dir_node = get_current_proc()->cwd;
        } else {
            file_t* tmp = get_current_proc()->open_files[dirfd];
            if(tmp==NULL) {    
                goto sys_openat_error;
            }
            dir_node = tmp->node;
        }
        
        // because root has no dir '.', so should judge this case
        if(dir_node==get_root()&&path[0]=='.') {
            if(path[1]=='\0') {
                path++;
            }
            else if(path[1]=='/') {
                path+=2;
            }
        }
    }
    file_t *file = NULL;
    if(path[0]=='\0') {
        file = file_create_by_inode(dir_node);
    }
    else {
        file = file_openat(dir_node, path, flags, mode);
        
    }
    if(file == NULL) {
        printf("sys_openat: file_openat error\n");
        goto sys_openat_error;
    }
    int fd = create_fd(get_current_proc(), file);
    file_close(file);
    pfree(mem);
    return fd;

sys_openat_error:
    pfree(mem);
    return -1;
}

int sys_mkdirat(int dirfd, uint64 va, int mode)
{
    printf("sys_mkdirat: dirfd=%d, va=%p, mode=%d\n", dirfd, va, mode);
    char path_arr[MAX_PATH];
    char* path = path_arr;
    if (copy_string_from_user(va, path, MAX_PATH) < 0) {
        printf("sys_mkdirat: copy_string_from_user error\n");
        return -1;
    }
    inode_t *dir_node;
    if(path[0]=='/') {
        dir_node = get_root();
        path++;
    } else {
        if(dirfd == AT_FDCWD) {
            dir_node = get_current_proc()->cwd;
        } else {
            file_t* tmp = get_current_proc()->open_files[dirfd];
            if(tmp==NULL) return -1;
            dir_node = tmp->node;
        }
    }
    return file_mkdirat(dir_node, path, mode);
}

int sys_close(int fd)
{
    real_printf("sys_close: fd=%d\n", fd);
    if (fd < 0 || fd >= MAX_FD) {
        printf("sys_close: fd error\n");
        return -1;
    }
    process_t *proc = get_current_proc();
    if (proc->open_files[fd] == NULL) {
        printf("sys_close: fd points to NULL\n");
        return -1;
    }
    file_close(proc->open_files[fd]);
    proc->open_files[fd] = NULL;
    return 0;
}

int sys_dup(int fd)
{
    printf("sys_dup: fd=%d\n", fd);
    if (fd < 0 || fd >= MAX_FD) {
        printf("sys_dup: fd error\n");
        return -1;
    }
    process_t *proc = get_current_proc();
    if (proc->open_files[fd] == NULL) {
        printf("sys_dup: fd points to NULL\n");
        return -1;
    }
    file_t *file = proc->open_files[fd];
    return create_fd(proc, file);
}

int sys_dup3(int oldfd, int newfd) {
    printf("sys_dup3: oldfd=%d, newfd=%d\n", oldfd, newfd);
    if (oldfd < 0 || oldfd >= MAX_FD || newfd < 0 || newfd >= MAX_FD) {
        printf("sys_dup3: fd error\n");
        return -1;
    }
    process_t *proc = get_current_proc();
    if (proc->open_files[oldfd] == NULL) {
        printf("sys_dup3: oldfd points to NULL\n");
        return -1;
    }
    if (proc->open_files[newfd] != NULL) {
        int res = sys_close(newfd);
        if (res < 0) {
            printf("sys_dup3: sys_close error\n");
            return -1;
        }
    }
    file_t *file = proc->open_files[oldfd];
    proc->open_files[newfd] = file;
    file->refcnt++;
    return newfd;
}

int sys_getdents64(int fd, uint64 va, int len) {
    printf("sys_getdents64: fd=%d, va=%x, len=%d\n",fd,va,len);
    process_t *proc = get_current_proc();
    if (proc->open_files[fd] == NULL) {
        printf("sys_getdents64: fd points to NULL\n");
        return -1;
    }
    file_t *file = proc->open_files[fd];
    int p_num = PG_CEIL(len)/PG_SIZE;
    char* mem = palloc_n(p_num);
    char* buffer = mem;
    if(file->type == T_PIPE) {
        printf("sys_getdents64: file type is T_PIPE\n");
        goto sys_getdents64_error;
    }
    int res = get_dirent(file->node, len, (dirent_t *)buffer);
    if(res==-1) {
        printf("sys_getdents64: get_dirent error\n");
        goto sys_getdents64_error;
    }
    copy_to_va(thread_pool[get_tid()].stack_pagetable, va, buffer, res);
    pfree(mem);
    return res;

sys_getdents64_error:
    pfree(mem);
    return -1;
}

int sys_chdir(uint64 va) {
    printf("sys_chdir: va=%p\n", va);
    char* mem = palloc();
    char* path = mem;
    if (copy_string_from_user(va, path, MAX_PATH) < 0) {
        printf("sys_chdir: copy_string_from_user error\n");
        goto sys_chdir_error;
    }
    inode_t *dir_node;
    if(path[0]=='/') {
        dir_node = get_root();
        path++;
    } else {
        dir_node = get_current_proc()->cwd;
    }
    file_t *file = file_openat(dir_node, path, 0, 0);
    if(file == NULL) {
        printf("sys_chdir: file_openat error\n");
        goto sys_chdir_error;
    }
    inode_t *ori_cwd = get_current_proc()->cwd;
    get_current_proc()->cwd = file->node;
    // char *name = palloc();
    // get_inode_name(file->node,name,20);
    // int n_len = strlen(name);
    // printf("sys_chdir: len=%d, cwd=%s\n", n_len,name);
    pin_inode(file->node);
    release_inode(ori_cwd);
    file_close(file);
    pfree(mem);
    return 0;

sys_chdir_error:
    pfree(mem);
    return -1;
}

int sys_fstat(int fd, uint64 va) {
    printf("sys_fstat: fd=%d, va=%p\n", fd, va);
    if (fd < 0 || fd >= MAX_FD) {
        printf("sys_fstat: fd error\n");
        return -1;
    }
    process_t *proc = get_current_proc();
    if (proc->open_files[fd] == NULL) {
        printf("sys_fstat: fd points to NULL\n");
        return -1;
    }
    file_t *file = proc->open_files[fd];
    struct kstat st;
    st.st_dev = file->node->sb->identifier;
    st.st_ino = file->node->id;
    st.st_mode = file->flags;
    st.st_nlink = file->node->nlink;
    st.st_uid = 0;
    st.st_gid = 0;
    st.st_rdev = 0;
    st.st_size = file->node->size;
    st.st_blksize = 0;
    st.st_blocks = 0;
    st.st_atime_sec = 0;
    st.st_atime_nsec = 0;
    st.st_mtime_sec = 0;
    st.st_mtime_nsec = 0;
    st.st_ctime_sec = 0;
    st.st_ctime_nsec = 0;
    copy_to_va(thread_pool[get_tid()].stack_pagetable,va, &st, sizeof(struct kstat));
    return 0;
}

int sys_mount(uint64 source_va,uint64 target_va,uint64 fs_type_va,uint64 flags,uint64 data_va) {
    printf("sys_mount: source=%p, target=%p, fs_type=%p, flags=%p, data=%p\n",source_va,target_va,fs_type_va,flags,data_va);
    char* mem_source = kmalloc(MAX_PATH);
    char* source = mem_source;
    char* mem_target = kmalloc(MAX_PATH);
    char* target = mem_target;
    copy_string_from_user(source_va, source, MAX_PATH);
    copy_string_from_user(target_va, target, MAX_PATH);
    inode_t *dir_node = get_current_proc()->cwd;
    file_t *source_file = file_openat(dir_node, source, 0, 0);
    file_t *target_file = file_openat(dir_node, target, 0, 0);
    if(source_file == NULL) {
        real_printf("sys_mount: source_file open error\n");
        goto sys_mount_error;
    }
    if(target_file == NULL) {
        real_printf("sys_mount: target_file open error\n");
        goto sys_mount_error;
    }
    inode_t *source_node = source_file->node;
    inode_t *target_node = target_file->node;
    if(target_node->type != T_DIR) {
        goto sys_mount_error;
    }
    
    superblock_t *sb = alloc_superblock();
    if(fat32_superblock_init(source_node, source_node->sb, sb, get_new_sb_identifier())==0) {
        real_printf("sys_mount: fat32_superblock_init error\n");
        goto sys_mount_error;
    }
    
    if(mount_inode(target_node,sb)==0) {
        real_printf("sys_mount: mount_inode error\n");
        free_superblock(sb);
        goto sys_mount_error;
    }
    
    LOG("sys_mount: sb->identifier=%d %d\n",sb->identifier,target_node->sb->identifier);
    print_fs_info(target_node);
    
    file_close(source_file);
    
    file_close(target_file);
    
    LOG("sys_mount: target_node->ref%d %p\n",target_node->refcnt,target_node);
    kfree(mem_source);
    kfree(mem_target);
    
    return 0;
sys_mount_error:
    if(source_file) file_close(source_file);
    if(target_file) file_close(target_file);
    kfree(mem_source);
    kfree(mem_target);
    return -1;
}

int sys_umount(uint64 target_va) {
    printf("sys_umount: target=%p\n",target_va);
    char* mem_target = kmalloc(MAX_PATH);
    char* target = mem_target;
    copy_string_from_user(target_va, target, MAX_PATH);
    inode_t *dir_node = get_current_proc()->cwd;
    file_t *target_file = file_openat(dir_node, target, 0, 0);
    if(target_file == NULL) {
        printf("sys_umount: target_file open error\n");
        goto sys_umount_error;
    }
    inode_t *target_node = target_file->node;
    if(target_node->type != T_DIR) {
        printf("sys_umount: target_file is not a directory\n");
        goto sys_umount_error;
    }
    if(umount_inode(target_node)==0) {
        printf("sys_umount: umount_inode error\n");
        goto sys_umount_error;
    }
    file_close(target_file);
    kfree(mem_target);
    return 0;
sys_umount_error:
    if(target_file) file_close(target_file);
    kfree(mem_target);
    return -1;
}