#include "strap.h"
#include "csr.h"
#include "print.h"
#include "cpu.h"

void set_strap_stvec(){
    W_CSR(stvec, stvec);
}

void set_strap_uservec(){

    
    printf("trap:%p\n",TRAMPOLINE + USER_VEC_OFFSET);
    W_CSR(stvec, (TRAMPOLINE + USER_VEC_OFFSET));
}

void strap_init(){
    set_strap_stvec();
}

void usertrap(){
    intr_off();
    set_strap_stvec();
    printf("Genius, you enter user trap!\n");
    int64 tid = get_tid();
    thread_pool[tid].state = T_SLEEPING;
    release_spinlock(&thread_pool[tid].lock);
    switch_coro(&get_cpu()->scheduler_coro);
    entry_to_user();
}

void strap_handler(){
    
    uint64 stval = 0, sscratch = 0, sepc = 0, sip = 0, scause = 0;
    R_CSR(stval, stval);
    R_CSR(sscratch, sscratch);
    R_CSR(sepc, sepc);
    R_CSR(sip, sip);
    R_CSR(scause, scause);
    
    if (scause == 0x8000000000000001ull) asm("sret");
    printf("scause = %p, stval = %p, sscratch = %p, sepc = %p, sip = %p\n", scause, stval, sscratch, sepc, sip);
    while(1);
}

void intr_on(){
    S_CSR(sstatus, SSTATUS_SIE);
}

void intr_off(){
    C_CSR(sstatus, SSTATUS_SIE);
}

void intr_push(){
    uint64 push_off_num = --get_cpu()->push_off_num;
    if (push_off_num == 0) {
        if(get_cpu()->intr_status != 0)
            intr_on();
    }
}

void intr_pop(){
    if (get_cpu()->push_off_num++ == 0){
        uint64 sstatus;
        R_CSR(sstatus, sstatus);
        if(sstatus & SSTATUS_SIE) {
            get_cpu()->intr_status = 1;
            intr_off();
        }
        else get_cpu()->intr_status = 0;
    }
}
