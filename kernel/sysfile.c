#include "sysfile.h"
#include "process.h"
#include "file.h"

// will add file->refcnt
// static int fd_alloc(file_t *file)
// {
//     int fd;
//     process_t *proc = get_current_proc();

//     for (fd = 0; fd < MAX_FD; fd++)
//     {
//         if (proc->open_files[fd] == NULL)
//         {
//             proc->open_files[fd] = file;
//             file_dup(file);
//             return fd;
//         }
//     }
//     return -1;
// }

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
    uint64 len = strlen(proc->cwd_name) + 1;
    if (len > size) {
        printf("sys_getcwd: size is too small\n");
        return -1;
    }
    return copy_to_va(get_current_proc()->pagetable ,va, proc->cwd_name, len);
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
    }
    file_t *file = file_openat(dir_node, path, flags, mode);
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
    printf("sys_close: fd=%d\n", fd);
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