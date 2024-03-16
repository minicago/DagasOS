#ifndef __ELF__H__
#define __ELF__H__
#include "types.h"
#include "vmm.h"
#include "process.h"
#include "file.h"

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header
typedef struct elf_hdr_struct {
  uint32 magic;  // must equal ELF_MAGIC
  char elf[12];
  uint16 type;
  uint16 machine;
  uint32 version;
  uint64 entry;
  uint64 phoff;
  uint64 shoff;
  uint32 flags;
  uint16 ehsize;
  uint16 phentsize;
  uint16 phnum;
  uint16 shentsize;
  uint16 shnum;
  uint16 shstrndx;
}elf_hdr_t;

// Program section header
typedef struct prog_hdr_struct{
  uint32 type;
  uint32 flags;
  uint64 off;
  uint64 vaddr;
  uint64 paddr;
  uint64 filesz;
  uint64 memsz;
  uint64 align;
}prog_hdr_t;

#define FLAGS2PERM(flags) \
    ((((flags)&0x1)?PTE_X:0)|(((flags)&0x02)?PTE_W:0)|PTE_R|PTE_U)

// Values for Proghdr type
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4

int load_elf(process_t* process, inode_t* elf);

#endif