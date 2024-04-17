#include "process.h"
#include "memory_layout.h"
#include "vmm.h"
#include "strap.h"
#include "cpu.h"
#include "file.h"
#include "console.h"
#include "dagaslib.h"

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
    uvminit(process);
    // process->pagetable = alloc_user_pagetable();
    process->thread_count = 0;
    process->pid = process - process_pool;



    // release_spinlock(&process->lock);
}

void prepare_initcode_process(process_t* process){
    // acquire_spinlock(&process->lock);
    
    printf("ok!\n");

    alloc_vm(process, TRAMPOLINE, PG_SIZE, 
        alloc_pm(0, (uint64) trampoline, PG_SIZE), PTE_R | PTE_X , VM_PA_SHARED );
    printf("ok!\n");
    alloc_vm(process, FAKE_STACK0, MAX_TSTACK_SIZE, 
        NULL , PTE_R | PTE_X , VM_NO_ALLOC | VM_TO_THREAD_STACK);

    process->arg_vm = alloc_vm(process, ARG_PAGE, PG_SIZE, 
        NULL, PTE_R | PTE_W | PTE_U, 0 );        

    process->heap_vm = alloc_vm(process, HEAP_SPACE, HEAP_SIZE, 
        NULL, PTE_R | PTE_W | PTE_U, VM_NO_ALLOC ); 

    vm_insert_pm(process->heap_vm, 
    alloc_pm(0, 0, PG_SIZE));

    heap_init(process->pagetable, 1);
    
    
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
    
    process->cwd = get_root();
    strcpy(process->cwd_name, "/");
    // release_spinlock(&process->lock);
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

// Return the current struct proc *, or zero if none.
process_t* get_current_proc(void)
{
    intr_push();
    // cpu_t *c = get_cpu();
    
    //printf("get_current_proc: %p %p\n", c, c->thread);
    process_t *proc = process_pool + get_tid();
    intr_pop();
    return proc;
}

int create_fd(process_t* process, file_t* file){
    acquire_spinlock(&process->lock);
    if(file == NULL){
        return -1;
    }
    for(int i=0;i<MAX_FD;i++){
        if(process->open_files[i] == NULL){
            process->open_files[i] = file;
            file->refcnt++;
            release_spinlock(&process->lock);
            return i;
        }
    }
    printf("create_fd: no free fd\n");
    release_spinlock(&process->lock);
    return -1;
}

void set_arg(process_t* process, int argc, char** argv){
    // acquire_spinlock(&process->lock);
    // uint64 pa = (uint64) palloc();
    // mappages(process->pagetable, ARG_PAGE, pa, PG_SIZE, PTE_W | PTE_U | PTE_R);
    // sfencevma(ARG_PAGE, process->pid);
    uint64 pa = va2pa(process->pagetable, ARG_PAGE);
    // printf("ok!\n");
    *(int*) pa = argc;
    for(int i = 0; i < argc; i++){
        char* ptr = uvmalloc(process, 7);
        // printf("ptr:%p\n",ptr);
        copy_to_va(process->pagetable, (uint64) ptr, argv[i], 7);
        *(char**) (pa + 8 + i * 8) = ptr;
    }
    // release_spinlock(&process->lock);
}

process_t* fork_process(process_t* process){
    process_t* process_new = alloc_process();
    init_process(process_new);
    for(vm_t* vm = process->vm_list; vm != NULL; vm = vm->next){
        if(vm->type | VM_NO_FORK) continue;
        vm_t* vm_new = alloc_vm(process_new, vm->va, vm->size, vm->pm, vm->perm, vm->type);
        if(process->arg_vm == vm) process_new->arg_vm = vm_new;
        if(process->heap_vm == vm) process_new->heap_vm = vm_new;
    }
    for(int i = 0; i < MAX_FD; i++){
        process_new->open_files[i] = process->open_files[i];
    }
    process_new->cwd = process->cwd;
    memcpy(process_new->cwd_name, process->cwd, MAX_PATH);
    process_new->parent = process;
    release_spinlock(&process_new->lock);
    return process_new; 
}