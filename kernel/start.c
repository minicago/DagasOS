# include "types.h"
# include "csr.h"
int main();
uint64 mstatus = 0; 
int start(){
    R_CSR(mstatus, mstatus);

    //set mstatus.mpp as S-mode 
    C_CSR(mstatus, MSTATUS_MPP_MASK);
    S_CSR(mstatus, MSTATUS_MPP_S);

    // set mepc = main
    W_CSR(mepc, (uint64) main);

    
    R_CSR(mstatus, mstatus);

    // mret to main
    asm("mret");


    return 0;
}
