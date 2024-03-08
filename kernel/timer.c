#include "timer.h"
#include "clint.h"
#include "types.h"
#include "csr.h"

uint64 timer_scratch[5];

void timer_init(){
    // set clint
    *(uint64*) CLINT_MTIMECMP = *(uint64*) CLINT_MTIME + TIMER_INTERVAL;
    
    // set timer scratch
    timer_scratch[3] = CLINT_MTIMECMP;
    timer_scratch[4] = TIMER_INTERVAL;
    W_CSR(mscratch, (uint64)timer_scratch);

    // set CSRs :
    W_CSR(mtvec, mtvec);
    S_CSR(mstatus, MSTATUS_MIE);
    S_CSR(mie, M_TIMER_INTR);
    

}