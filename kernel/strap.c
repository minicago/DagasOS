#include "strap.h"
#include "csr.h"
#include "print.h"

void strap_init(){

    W_CSR(stvec, (uint64) stvec);
    uint64 x = 1;
    R_CSR(stvec, x);    
}

void strap_handler(){
    uint64 stval = 0, sscratch = 0, sepc = 0, sip = 0, scause = 0;
    R_CSR(stval, stval);
    R_CSR(sscratch, sscratch);
    R_CSR(sepc, sepc);
    R_CSR(sip, sip);
    R_CSR(scause, scause);
    printf("scause = %p, stval = %p, sscratch = %p, sepc = %p, sip = %p\n", scause, stval, sscratch, sepc, sip);
    while(1);
}

void intr_on(){
    S_CSR(sstatus, SSTATUS_SIE);
}

void intr_off(){
    C_CSR(sstatus, SSTATUS_SIE);
}