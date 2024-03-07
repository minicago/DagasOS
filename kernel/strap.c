#include "strap.h"
#include "csr.h"
#include "print.h"

void strap_init(){

    W_CSR(stvec, (uint64) stvec);
    goto checkstvec;
    checkstvec:

    uint64 x = 1;
    R_CSR(stvec, x);    
    printf("stvec = %p %p\n",x, (uint64) stvec);
    

}

void strap_handler(){
    printf("uh oh, unexpected!\n");
    uint64 sepc = 0;
    R_CSR(sepc, sepc);
    sepc += 4;
    W_CSR(sepc, sepc);
}