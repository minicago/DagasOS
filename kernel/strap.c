#include "strap.h"
#include "csr.h"
#include "print.h"

void strap_init(){
    W_CSR(stvec, (uint64) stvec);

}

void strap_handler(){
    printf("uh oh, unexpected!");
}