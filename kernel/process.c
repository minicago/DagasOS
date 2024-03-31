#include "process.h"
#include "memory_layout.h"
#include "vmm.h"
#include "strap.h"
#include "cpu.h"
#include "file.h"
#include "console.h"

spinlock_t process_pool_lock;

process_t process_pool[MAX_PROCESS];

process_t* free_process_head = NULL;


__attribute__ ((aligned(0x1000))) uint64 MAGIC_CODE[PG_SIZE] = {
    0x73, 0x00, 0x00, 0x00,

};

void process_pool_init(){
    init_spinlock(&process_pool_lock);
    acquire_spinlock(&process_pool_lock);
    free_process_head = &process_pool[0];
    for(uint64 i = 1; i < MAX_PROCESS; i++){
        *(uint64*)&process_pool[i] = (uint64) &process_pool[i - 1];
    }
    release_spinlock(&process_pool_lock);
}

// must called after console_init
void init_process(process_t* process){
    init_spinlock(&process->lock);
    acquire_spinlock(&process->lock);
    process->pagetable = alloc_user_pagetable();
    process->thread_count = 0;
    process->pid = process - process_pool;
    for(int i=0;i<MAX_FD;i++){
        process->open_files[i] = NULL;
    }
    file_t* tmp;
    tmp = process->open_files[FD_STDIN] = file_create_by_inode(get_stdin());
    tmp->writable = 0;
    tmp->readable = 1;
    tmp = process->open_files[FD_STDOUT] = file_create_by_inode(get_stdout());
    tmp->writable = 1;
    tmp->readable = 0;
    tmp = process->open_files[FD_STDERR] = file_create_by_inode(get_stderr());
    tmp->writable = 1;
    tmp->readable = 0;
    release_spinlock(&process->lock);
}

process_t* alloc_process(){
    acquire_spinlock(&process_pool_lock);
    process_t* new_process = free_process_head;
    free_process_head = (process_t*)*(uint64*)free_process_head;
    release_spinlock(&process_pool_lock);
    return new_process;
}

void free_process(process_t* process){
    acquire_spinlock(&process_pool_lock);
    *(process_t**) process = free_process_head;
    free_process_head = process;
    release_spinlock(&process_pool_lock);
}

void map_elf(process_t* process){
    printf("%p\n",MAGIC_CODE);
    mappages(process->pagetable, 0x0ull, (uint64) MAGIC_CODE, PG_SIZE, PTE_X | PTE_U | PTE_R); //read_only
    print_page_table((pagetable_t)walk(process->pagetable, 0, 0));


}

// Return the current struct proc *, or zero if none.
process_t* get_current_proc(void)
{
    intr_push();
    cpu_t *c = get_cpu();
    
    //printf("get_current_proc: %p %p\n", c, c->thread);
    process_t *proc = c->thread->process;
    intr_pop();
    return proc;
}