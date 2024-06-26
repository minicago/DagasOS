# include "types.h"
# include "csr.h"
# include "strap.h"
# include "timer.h"
# include "defs.h"

int start(){

    uint64 mhartid = 0;

    R_CSR(mhartid, mhartid);

    if(mhartid != 0){
        while(1);
    }

    //set mstatus.mpp as S-mode 
    C_CSR(mstatus, MSTATUS_MPP_MASK);
    S_CSR(mstatus, MSTATUS_MPP_S);

    // set mepc = main
    W_CSR(mepc, (uint64) 0x80200000);

    W_CSR(satp, 0);

    W_CSR(pmpaddr0, PMPADDR0_S_TOR);
    W_CSR(pmpcfg0, PMPCFG_R | PMPCFG_W | PMPCFG_X | PMPCFG_A_TOR);

    //delegate interrupt to S-mode
    W_CSR(medeleg, EXC_MASK);
    W_CSR(mideleg, S_INTR_MASK);
    S_CSR(sie, S_INTR_MASK);
    
    //timer init
    timer_init();

    // mret to main
    asm("mret");


    return 0;
}
