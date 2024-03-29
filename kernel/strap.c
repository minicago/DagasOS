#include "strap.h"
#include "csr.h"
#include "print.h"
#include "cpu.h"
#include "uart.h"
#include "virtio_disk.h"
#include "plic.h"
#include "memory_layout.h"

void set_strap_stvec()
{
    W_CSR(stvec, stvec);
}

void set_strap_uservec()
{
    printf("trap:%p\n", TRAMPOLINE + USER_VEC_OFFSET);
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
    if (trapframe->a0 == SYS_PRINT){
        printf("USER: Hello, world!\n");
        thread_pool[tid].trapframe->epc += 4;
        printf("epc : %x\n",  thread_pool[tid].trapframe->epc);
    }else {
        printf("Unknown ecall!\nstval = %p, sscratch = %p, sepc = %p, ecall_id = %p\n", stval, sscratch, sepc, trapframe->a0);
        // while (1);
        
    }
    
}

void usertrap()
{
    int tid = get_tid();
    uint64 stval = 0, sscratch = 0, sepc = 0, sip = 0, scause = 0;
    int which_dev;

    intr_push();
    

    set_strap_stvec();
    

    R_CSR(stval, stval);
    R_CSR(sscratch, sscratch);
    R_CSR(sepc, sepc);
    
    R_CSR(sip, sip);
    R_CSR(scause, scause);
    thread_pool[tid].trapframe->epc = sepc;
    if (scause == BREAK_POINT_EXC && !(scause & INTR_HEAD) ){
        resolve_ecall();
        thread_pool[tid].state = T_READY;
        intr_pop();
        goto return_to_user;
    } else if((which_dev = dev_intr()) == 0) {
        printf("Unknown user trap:\nscause = %p, stval = %p, sscratch = %p, sepc = %p, sip = %p\n", scause, stval, sscratch, sepc, sip);
        thread_pool[tid].state = T_SLEEPING;
        intr_pop();
        goto switch_to;
    } else {
        intr_pop();
        goto return_to_user;

    }

switch_to:
    release_spinlock(&thread_pool[tid].lock);
    switch_coro(&get_cpu()->scheduler_coro);
return_to_user:
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
        printf("scause = %p, stval = %p, sscratch = %p, sepc = %p, sip = %p\n", scause, stval, sscratch, sepc, sip);
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