#include "strap.h"
#include "csr.h"
#include "print.h"
#include "cpu.h"
#include "uart.h"
#include "virtio_disk.h"
#include "plic.h"
#include "memory_layout.h"
#include "syscall.h"

void intr_on()
{
    S_CSR(sstatus, SSTATUS_SIE);
}

void intr_off()
{
    C_CSR(sstatus, SSTATUS_SIE);
}

void intr_pop(){
    uint64 push_off_num = --get_cpu()->push_off_num;
    if (push_off_num == 0)
    {
        if (get_cpu()->intr_status != 0)
            intr_on();
    }
}

void intr_push(){
    if (get_cpu()->push_off_num++ == 0){
        uint64 sstatus;
        R_CSR(sstatus, sstatus);
        if (sstatus & SSTATUS_SIE)
        {
            get_cpu()->intr_status = 1;
            intr_off();
        }
        else
            get_cpu()->intr_status = 0;
    }
}

int dev_intr()
{
    uint64 scause;
    R_CSR(scause, scause);
    //printf("dev_intr: enter devintr 0x%p\n", scause);
    if ((scause & INTR_HEAD) &&
        (scause & 0xff) == 9)
    {
        // this is a supervisor external interrupt, via PLIC.

        // irq indicates which device interrupted.
        int irq = plic_claim();

        if (irq == UART0_IRQ)
        {
            //printf("dev_intr: uart intr!!!!!!!!!!!!!!\n");
            //TODO: uart_intr();
        }
        else if (irq == VIRTIO0_IRQ)
        {
            //printf("dev_intr: virtio intr!!!!!!!!!!!!!!\n");
            virtio_disk_intr();
            //printf("dev_intr: finish virtio disk intr\n");
        }
        else if (irq)
        {
            printf("unexpected interrupt irq=%d\n", irq);
        }

        // the PLIC allows each device to raise at most one
        // interrupt at a time; tell the PLIC the device is
        // now allowed to interrupt again.
        if (irq)
        {
            plic_complete(irq);
            //printf("dev_intr: complete irq\n");
        }

        return 1;
    }
    else if (scause == (0x8000000000000001))
    {
        // software interrupt from a machine-mode timer interrupt,
        // forwarded by timervec in kernelvec.S.

        if (get_cpu_id() == 0)
        {
            //TODO: clock_intr();
        }
        // acknowledge the software interrupt by clearing
        // the SSIP bit in sip.
        uint64 sip;
        R_CSR(sip,sip);
        W_CSR(sip, sip & ~2);
        return 2;
    }
    else
    {
        return 0;
    }
}

void intr_print() {
    uint64 sstatus;
    R_CSR(sstatus, sstatus);
    printf("intr = %p cpu_push_nums%d\n", sstatus & SSTATUS_SIE, get_cpu()->push_off_num);

}

int intr_get() {
    uint64 sstatus;
    R_CSR(sstatus, sstatus);
    return (sstatus & SSTATUS_SIE) != 0;
}

void set_strap_stvec()
{
    W_CSR(stvec, stvec);
}

void set_strap_uservec()
{
    // printf("trap:%p\n", TRAMPOLINE + USER_VEC_OFFSET);
    W_CSR(stvec, (TRAMPOLINE + USER_VEC_OFFSET));
}

void strap_init()
{
    set_strap_stvec();
}

void resolve_ecall(){
    uint64 stval = 0, sscratch = 0, sepc = 0;
    R_CSR(stval, stval);
    R_CSR(sscratch, sscratch);
    R_CSR(sepc, sepc);
    int tid = get_tid();
    trapframe_t* trapframe = thread_pool[tid].trapframe;
    syscall_handler(trapframe);
    
}

void resolve_unknown_trap(){
    uint64 scause = 0, stval = 0, sscratch = 0, sepc = 0, sip = 0;
    R_CSR(stval, stval);
    R_CSR(sscratch, sscratch);
    R_CSR(sepc, sepc);
    
    R_CSR(sip, sip);
    R_CSR(scause, scause);
    printf("Unknown user trap:\nscause = %p, stval = %p, sscratch = %p, sepc = %p, sip = %p\n", scause, stval, sscratch, sepc, sip);
    sys_exit(-1);
}

int resolve_page_fault(){
    
    thread_t* thread = thread_pool + get_tid();
    uint64 scause = 0, stval = 0;
    R_CSR(scause, scause);
    R_CSR(stval, stval);
    LOG("page_fault! va:%p\n", stval);

    vm_t* vm = vm_lookup(thread->stack_vm, stval);

    if(vm == NULL) return 0;

    if(vm->type & VM_LAZY_ALLOC){
        // printf("vm: %p %p\n",vm->pagetable, thread->stack_pagetable);
        vm_insert_pm(vm, 
            alloc_pm(PG_FLOOR(stval - vm->va), 0, PG_SIZE));
        return 1;
    }
    
    // if(vm->type & VM_TO_THREAD_STACK){
    //     thread->trapframe->sp += thread->stack_vm->va - vm->va;
    //     printf("sp : %p\n",thread->trapframe->sp);
    //     vm_insert_pm(thread->stack_vm, 
    //         alloc_pm(PG_FLOOR(stval - vm->va), 0, PG_SIZE));
    //     return 1;
    // }
        
    // // fake stack
    // if(thread->trapframe->sp >= FAKE_STACK0
    // && thread->trapframe->sp < FAKE_STACK_BOTTOM
    // && stval > thread->trapframe->sp
    // && stval < FAKE_STACK_BOTTOM) {
    //     thread->trapframe->sp += TSTACK0(thread->tid) - FAKE_STACK0;
    //     uint64 pa = (uint64) palloc();
    //     uint64 va = (stval + TSTACK0(thread->tid) - FAKE_STACK0) & ~PG_OFFSET_MASK;
    //     // thread->user_stack_size = MAX(thread->user_stack_size, TSTACK_BOTTOM(thread->tid) - va);
    //     mappages(thread->process->pagetable , va, pa, PG_SIZE, PTE_R | PTE_W | PTE_U);
    //     // sfencevma(va, thread->process->pid);
    //     return 1;
    // }

    // // stack
    // if(thread->trapframe->sp >= TSTACK0(thread->tid)
    // && thread->trapframe->sp < TSTACK_BOTTOM(thread->tid)
    // && stval > thread->trapframe->sp
    // && stval < FAKE_STACK_BOTTOM) {
    //     uint64 pa = (uint64) palloc();
    //     uint64 va = stval & ~PG_OFFSET_MASK;
    //     // thread->user_stack_size = MAX(thread->user_stack_size, TSTACK_BOTTOM(thread->tid) - va);
    //     mappages(thread->process->pagetable ,va, pa, PG_SIZE, PTE_R | PTE_W | PTE_U);
    //     // sfencevma(va, thread->process->pid);
    //     return 1;
    // }    
    // printf("Real page fault");
    return 0;
}

void usertrap()
{
    
    int tid = get_tid();
    uint64 scause = 0, sepc = 0;
    int which_dev;

    intr_push();
    

    set_strap_stvec();
    
    R_CSR(scause, scause);
    R_CSR(sepc, sepc);

    thread_pool[tid].trapframe->epc = sepc;
    LOG("User trap\n scause = %p, sepc = %p\n", scause, sepc);
    assert(get_cpu()->push_off_num == 2);
    switch (scause)
    {
    case SCAUSE_ECALL:
        resolve_ecall();
        goto return_to_user;
        break;

    case SCAUSE_LOAD_PAGE_FAULT:
    case SCAUSE_STORE_AMO_PAGE_FAULT:
        if (resolve_page_fault()) {
            goto return_to_user;
        } else {
            sys_exit(-1);
            goto switch_to;
        }
        break;
    
    default:
        if((which_dev = dev_intr()) == 0) {
            resolve_unknown_trap();
            // thread_pool[tid].state = T_SLEEPING;
            goto switch_to;
        } else {
            thread_pool[tid].state = T_READY;
            goto switch_to;
        }
        break;
    }

return_to_user:
    intr_pop();
    entry_to_user();
switch_to:
    sched();
    intr_pop();
    entry_to_user();
}

//TODO: off the intr before call this function
void strap_handler()
{
    intr_push();
    uint64 stval = 0, sscratch = 0, sepc = 0, sip = 0, scause = 0;
    int which_dev;
    R_CSR(stval, stval);
    R_CSR(sscratch, sscratch);
    R_CSR(sepc, sepc);
    R_CSR(sip, sip);
    R_CSR(scause, scause);
    
    if((which_dev = dev_intr()) == 0) {
        ERROR("Unknown trap\n scause = %p, stval = %p, sscratch = %p, sepc = %p, sip = %p\n", scause, stval, sscratch, sepc, sip);
        while (1)
            ;
    } else {
        intr_pop();
        W_CSR(sepc, sepc);
        W_CSR(sscratch, sscratch);
        //printf("strap_handler: return\n");
        //asm("sret");
        }
}

