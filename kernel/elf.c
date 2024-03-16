#include "elf.h"
#include "process.h"
#include "fat32.h"
#include "vmm.h"

int load_elf(process_t* process, inode_t* elf){
    
    elf_hdr_t elf_hdr;
    if(read_inode(elf, 0, sizeof(elf_hdr_t),&elf_hdr) != sizeof(elf_hdr_t) )
        goto elf_hdr_err;
    if(elf_hdr.magic != ELF_MAGIC)
        goto elf_hdr_err;
    
    process->pagetable = alloc_user_pagetable();
    if(process->pagetable == NULL) goto pagetable_err;
    
    for(uint64 i=0, off = elf_hdr.phoff; i < elf_hdr.phnum; i++){
        prog_hdr_t prog_hdr;
        if(read_inode(elf, off, sizeof(prog_hdr), &prog_hdr) != sizeof(prog_hdr_t))
            goto prog_hdr_err;
        
        load_and_map(elf, process->pagetable, prog_hdr.vaddr, prog_hdr.off, prog_hdr.filesz, FLAGS2PERM(prog_hdr.flags));
    } 


elf_hdr_err:
pagetable_err:
prog_hdr_err:
    //to be added
    return 0;
    

}