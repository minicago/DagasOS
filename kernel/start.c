# include "types.h"
# include "csr.h"
int main();
uint64 mstatus = 0; 
int start(){

    //set mstatus.mpp as S-mode 
    C_CSR(mstatus, MSTATUS_MPP_MASK);
    S_CSR(mstatus, MSTATUS_MPP_S);

    // set mepc = main
    W_CSR(mepc, (uint64) main);

    W_CSR(satp, 0);

    W_CSR(pmpaddr0, PMPADDR0_S_TOR);
    W_CSR(pmpcfg0, PMPCFG_R | PMPCFG_W | PMPCFG_X | PMPCFG_A_TOR);

    // mret to main
    asm("mret");


    return 0;
}
