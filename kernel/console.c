#include "console.h"
#include "uart.h"
#include "file.h"
#include "fs.h"
#include "uart.h"
#include "bio.h"
#include "print.h"

static inode_t *stdin, *stdout, *stderr;
static int console_write(int from_user, uint64 src, int len);

// must be called after uart_init and filesystem_init
void console_init()
{
    inode_t *root = get_root();
    if (root == NULL)
        panic("console_init: root is NULL");
    inode_t *dev;
    if ((dev = look_up_path(root, "dev")) == NULL)
    {
        dev = create_inode(root, "dev", 0, T_DIR);
    }
    if (dev == NULL)
        panic("console_init: dev is NULL");
    if ((stdin = look_up_path(dev, "stdin")) == NULL)
    {
        printf("console_init: stdin is NULL, create\n");
        stdin = create_inode(dev, "stdin", CONSOLE_DEV, T_DEVICE);
    }
    if (stdin == NULL)
        panic("console_init: stdin is NULL");
    if ((stdout = look_up_path(dev, "stdout")) == NULL)
    {
        stdout = create_inode(dev, "stdout", CONSOLE_DEV, T_DEVICE);
    }
    if (stdout == NULL)
        panic("console_init: stdout is NULL");
    if ((stderr = look_up_path(dev, "stderr")) == NULL)
    {
        stderr = create_inode(dev, "stderr", CONSOLE_DEV, T_DEVICE);
    }
    if (stderr == NULL)
        panic("console_init: stderr is NULL");
    devices[CONSOLE_DEV].write = console_write;
    flush_cache_to_disk();
}

inode_t *get_stdin()
{
    return stdin;
}

inode_t *get_stdout()
{
    return stdout;
}

inode_t *get_stderr()
{
    return stderr;
}

static int console_write(int from_user, uint64 src, int len)
{
    printf("console_write: from_user=%d, src=%p, len=%d\n", from_user, src, len);
    int i;
    for (i = 0; i < len; i++)
    {
        char c;
        if (copy_to_pa(&c, src+i, 1, from_user) == -1)
            break;
        uartputc_sync(c);
    }
    return i;
}