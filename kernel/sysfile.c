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
int sys_write(int fd, uint64 va, int size)
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