#include "elf.h"
#include "process.h"
#include "file.h"
#include "fs.h"
#include "vmm.h"
#include "strap.h"

int load_elf_from_inode(process_t* process, inode_t* elf){
    
    elf_hdr_t elf_hdr;
    if(read_inode(elf, 0, sizeof(elf_hdr_t),&elf_hdr) != sizeof(elf_hdr_t) )
        goto elf_hdr_err;
    if(elf_hdr.magic != ELF_MAGIC)
        goto elf_hdr_err;

    // process->pagetable = alloc_user_pagetable();
    if(process->pagetable == NULL) goto pagetable_err;

    for(uint64 i=0, off = elf_hdr.phoff; i < elf_hdr.phnum; i++, off += sizeof(prog_hdr_t)){
        prog_hdr_t prog_hdr;
        if(read_inode(elf, off, sizeof(prog_hdr), &prog_hdr) != sizeof(prog_hdr_t))
            goto prog_hdr_err;
        if(prog_hdr.type != ELF_PROG_LOAD) continue;
        LOG("vaddr:%p paddr:%p off:%p filesz:%p memsz:%p, flags:%p\n",prog_hdr.vaddr,prog_hdr.paddr,prog_hdr.off, prog_hdr.filesz, prog_hdr.memsz, prog_hdr.flags);
        // addpages(process->pagetable, prog_hdr.vaddr, prog_hdr.memsz, FLAGS2PERM(prog_hdr.flags));
        int vm_type = (prog_hdr.flags & ELF_PROG_FLAG_WRITE)?VM_PA_SHARED:0;
       
        vm_t* vm = alloc_vm(process, prog_hdr.vaddr, prog_hdr.memsz, NULL, FLAGS2PERM(prog_hdr.flags), vm_type);

        read_inode(elf, prog_hdr.off, prog_hdr.filesz, (void*) vm->pm->pa);
        
        // load_from_inode_to_page(elf, process->pagetable, prog_hdr.vaddr, prog_hdr.off, prog_hdr.filesz);
    } 
    
    LOG("pagetable:%p\n", process->pagetable);
    return 1;

elf_hdr_err:
pagetable_err:
prog_hdr_err:
    //to be added
    return 0;
}

int load_elf(process_t* process, char* path){
    process->heap_vm = alloc_vm(process, HEAP_SPACE, HEAP_SIZE, 
        NULL, PTE_R | PTE_W | PTE_U, VM_NO_ALLOC ); 
    vm_insert_pm(process->heap_vm, 
    alloc_pm(0, 0, PG_SIZE));
    heap_init(process->pagetable, 1);
    inode_t* elf = look_up_path(process->cwd, path, NULL);
    print_inode(elf);
    LOG("inode:%p\n", elf);
    return load_elf_from_inode(process, elf);
    LOG("inode:%p\n", elf);
}