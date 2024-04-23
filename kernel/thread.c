#include "thread.h"
#include "spinlock.h"
#include "vmm.h"
#include "memory_layout.h"
#include "csr.h"
#include "cpu.h"
#include "coro.h"
#include "strap.h"
#include "reg.h"
#include "process.h"
#include "strap.h"

spinlock_t thread_pool_lock;


thread_t thread_pool[MAX_THREAD];

thread_t* free_thread_head = NULL;


void thread_pool_init(){
    init_spinlock(&thread_pool_lock);
    acquire_spinlock(&thread_pool_lock);
    free_thread_head = &thread_pool[0];
    for(uint64 i = 0; i < MAX_THREAD - 1; i++){
        *(void**)(thread_pool+i) = thread_pool + i + 1;
        
    }
    printf("thread_pool_0:%p %p", thread_pool ,*(void**) (thread_pool + 0));
    release_spinlock(&thread_pool_lock);
}

// only work after init thread manager coro
void entry_main(thread_t* thread){        

    // acquire_spinlock(&thread->lock);
    thread->trapframe->kernel_satp = 
        ATP(MAX_THREAD, kernel_pagetable); 
    thread->trapframe->kernel_trap = (uint64 )usertrap;
    thread->trapframe->epc = USER_ENTRY;
    thread->trapframe->ra = USER_EXIT;
    thread->trapframe->sp = ARG_PAGE;
    thread->state = T_READY;
    alloc_vm_stack(thread, TSTACK0, MAX_TSTACK_SIZE, 
        NULL, PTE_U | PTE_W | PTE_R, VM_LAZY_ALLOC | VM_THREAD_STACK);
    // release_spinlock(&thread->lock);
}

void attach_to_process(thread_t* thread, process_t* process){
    // acquire_spinlock(&thread->lock);
    thread -> process = process;
    // release_spinlock(&thread->lock);
    acquire_spinlock(&process->lock);
    process -> thread_count ++;
    // uint64 pa = (uint64)palloc();
    // uint64 va = thread->user_stack_bottom - PG_SIZE;
    // mappages(process->pagetable, va, pa, PG_SIZE, PTE_R | PTE_W | PTE_U);
    // // sfencevma(va, process->pid);
    
    release_spinlock(&process->lock);
}

void init_thread(thread_t* thread){
    init_spinlock(&thread->lock);
    acquire_spinlock(&thread->lock);

    thread->tid = thread - thread_pool;
    thread->process = NULL;
 
    
    // release_spinlock(&thread->lock);
}

thread_t* alloc_thread(){
    acquire_spinlock(&thread_pool_lock);
    printf("alloc thread:%p\n" , free_thread_head);
    thread_t* new_thread = free_thread_head;
    free_thread_head = *(void**) free_thread_head;
    new_thread->stack_pagetable = alloc_stack_pagetable();
    release_spinlock(&thread_pool_lock);
    return new_thread;
}

void free_thread(thread_t* thread){
    thread->state = T_RUNNING;
    acquire_spinlock(&thread_pool_lock);
    *(thread_t**) thread = free_thread_head;
    free_thread_head = thread;
    release_spinlock(&thread_pool_lock);
}

void entry_to_user(){
    printf("entry to user\n");
    int tid = get_tid();
    if(tid == 1) printf("!!!!fork!!!!\n");
    if(tid == -1) panic("wrong coro");
    W_CSR(sepc, thread_pool[tid].trapframe->epc);
    C_CSR(sstatus, SSTATUS_SPP);
    W_CSR(sscratch, thread_pool[tid].trapframe);
    switch_stack_pagetable(thread_pool[tid].process->pagetable, thread_pool[tid].stack_pagetable);
    printf("sp:%p page_table:%p\n",thread_pool[tid].trapframe->kernel_sp, thread_pool[tid].process->pagetable);
    printf("go to user\n");
    printf("trampoline:%p\n", *(uint64*) ( va2pa(thread_pool[tid].stack_pagetable, TRAMPOLINE)) );
    set_strap_uservec();
    // printf("%p\n", PTE2PA( * walk( thread_pool[tid].process->pagetable ,0, 0)) );
    // printf("%p\n", PTE2PA( * walk( thread_pool[tid].process->pagetable ,0x1000, 0)) );
    ((userret_t*) (TRAMPOLINE + USER_RET_OFFSET) )(
    thread_pool[tid].trapframe, 
    ATP(tid, thread_pool[tid].stack_pagetable) );
    
}

void sched(){
    intr_pop();
    release_spinlock(&thread_pool[get_tid()].lock);
    switch_coro(&get_cpu()->scheduler_coro);    
}

void sleep(){
    thread_pool[get_tid()].state = T_SLEEPING;
    sched();
}

void awake(thread_t* thread){
    acquire_spinlock(&thread->lock);
    thread->state = T_READY;
    release_spinlock(&thread->lock);
}

void clone_thread(thread_t *thread, thread_t *thread_new){
    memcpy(thread_new->trapframe, thread->trapframe, sizeof(trapframe_t));
    thread_new->trapframe->kernel_sp = TRAPFRAME0(thread_new->tid);
    // thread_new->trapframe->sp = thread->trapframe->sp - TSTACK0(thread->tid) + TSTACK0(thread_new->tid);
    alloc_vm_stack(thread_new, TSTACK0, MAX_TSTACK_SIZE, thread->stack_vm->pm, thread->stack_vm->perm, thread->stack_vm->type);
    thread_new->state = T_READY;
    // for(uint64 va = thread->user_stack_bottom - thread->user_stack_size; va < thread->user_stack_bottom; va += PG_SIZE){
    //     uint64 pa = va2pa(thread->process->pagetable, va);
    //     if (pa == 0) continue;
    //     uint64 va_new = va + TSTACK0(thread_new->tid) - TSTACK0(thread->tid),pa_new = (uint64) palloc();
    //     memcpy((void*) pa_new, (void*) pa, PG_SIZE);
    //     mappages(thread_new->process->pagetable, va_new, pa_new, PG_SIZE, PTE_U | PTE_R | PTE_W);
    // }
}

int sys_fork(){
    printf("fork!\n");
    thread_t* thread = thread_pool+get_tid();
    printf("fork: 0\n");
    thread_t* thread_new = alloc_thread();
    init_thread(thread_new);
    printf("fork: 1\n");
    process_t* process_new = fork_process(thread->process);
    printf("fork: 2\n");
    attach_to_process(thread_new, process_new);
    printf("tid:%p\n", thread_new - thread_pool );
    init_thread_manager_coro(thread_new->tid);
    clone_thread(thread, thread_new);
    
    thread_new->trapframe->epc += 4;
    thread_new->trapframe->a0 = 0;
    thread_new->state = T_READY;
    vm_lookup(process_new->vm_list, 0);
    printf("*\n");
    vm_lookup(thread_new->stack_vm, 0);
    printf("fork epc:%p->epc:%p\n", thread->trapframe->epc, thread_new->trapframe->epc);
    release_spinlock(&thread_new->lock);
    return process_new->pid;
}

void deattach_thread(thread_t* thread){
    free_vm(thread->stack_vm);
    free_user_pagetable(thread->stack_pagetable);
    unmappages(thread->process->pagetable, COROSTACK0(thread->tid), PG_SIZE, 0);
    acquire_spinlock(&thread->process->lock);
    int thread_cnt = --thread->process->thread_count;
    release_spinlock(&thread->process->lock);
    if(thread_cnt == 0) release_process(thread->process);
    free_thread(thread);
}

void reset_stack(thread_t* thread){
    free_vm(thread->stack_vm);
    // thread->stack_vm = alloc_vm_stack(thread, TSTACK0, MAX_TSTACK_SIZE, 
    // NULL, PTE_U | PTE_W | PTE_R, VM_LAZY_ALLOC | VM_THREAD_STACK);
    // thread->trapframe->sp = TSTACK_BOTTOM;
}

int sys_exec(char* path){
    char* buf = kmalloc(MAX_PATH);
    LOG("buf:%p\n",buf);
    copy_to_pa(buf, (uint64)path, MAX_PATH, 1);    
    thread_t* thread = thread_pool + get_tid();
    reset_stack(thread);
    // LOG("buf:%p\n",buf);
    exec_process(thread->process, buf);
    
    entry_main(thread);
    // uint64 name = (uint64)uvmalloc(thread->process, MAX_PATH);
    // copy_to_va(thread->process->pagetable, name, buf, MAX_PATH);
    
    set_arg(thread->process, 1, &buf);
    kfree(buf);
    return 0;
}

int sys_exit(int ret){
    deattach_thread(thread_pool + get_tid() );
    sched();
    return ret;
}

int sys_wait(int pid){
    thread_t* thread = thread_pool + get_tid();
    int child_cnt = 0;
    uint64 child_pid = 0;
    acquire_spinlock(&wait_lock);
    for(process_t* child = thread->process->child_list; child != NULL; child = child->next){
        acquire_spinlock(&child->lock);
        if(pid == -1 || pid == child->pid){
            child_cnt++;
            if(child->state == ZOMBIE){
                release_spinlock(&child->lock);
                release_spinlock(&wait_lock);
                release_zombie(child);
                return child->pid;
            } else if(pid == child->pid) {
                wait_queue_push_back(child->wait_self, thread, &child_pid);
            }
        }
        release_spinlock(&child->lock);
    }
    if(child_cnt == 0) return -1;
    if(pid == -1) 
        wait_queue_push_back(thread->process->wait_child, thread, &child_pid);
    thread->state = T_SLEEPING;
    release_spinlock(&wait_lock);
    sched();
    release_zombie(thread_pool[child_pid].process);
    return child_pid;   
}