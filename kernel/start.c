#include "types.h"
#include "csr.h"
#include "sbi.h"
#define BACKSPACE 0x100
void consputc(int c)
{
    if (c == BACKSPACE)
    {
        // if the user typed backspace, overwrite with a space.
#ifdef visionfive
        uart8250_putc('\b');
        uart8250_putc(' ');
        uart8250_putc('\b');
#else
        sbi_console_putchar('\b');
        sbi_console_putchar(' ');
        sbi_console_putchar('\b');
#endif
    }
    else if (c == '\n' || c == '\r')
    {
#ifdef visionfive
        uart8250_putc('\r');
        uart8250_putc('\n');
#else
        sbi_console_putchar('\n');
#endif
    }
    else
    {
#ifdef visionfive
        uart8250_putc(c);
#else
        sbi_console_putchar(c);
#endif
    }
}
int main();
uint64 mstatus = 0; 
int start()
{

    consputc('a');
    consputc('b');

    // //set mstatus.mpp as S-mode
    // C_CSR(mstatus, MSTATUS_MPP_MASK);
    // S_CSR(mstatus, MSTATUS_MPP_S);

    // // set mepc = main
    // W_CSR(mepc, (uint64) main);

    // W_CSR(satp, 0);

    // W_CSR(pmpaddr0, PMPADDR0_S_TOR);
    // W_CSR(pmpcfg0, PMPCFG_R | PMPCFG_W | PMPCFG_X | PMPCFG_A_TOR);

    // // mret to main
    // asm("mret");

    return 0;
}
