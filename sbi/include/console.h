#ifndef __CONSOLE__H__
#define __CONSOLE__H__

#include "file.h"
#include "fs.h"

void console_init();

inode_t *get_stdin();

inode_t *get_stdout();

inode_t *get_stderr();

#endif