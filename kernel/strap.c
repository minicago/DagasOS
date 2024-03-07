#include "strap.h"
#include "csr.h"

void strap_init(){
    W_CSR(stvec, (uint64) stvec);

}