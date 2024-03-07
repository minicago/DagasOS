# include "print.h"
# include "strap.h"
# include "csr.h"

void dosomething(){}

int main(){
    dosomething();
    uartinit();
    strap_init();
    //printf("%s, %c, %d, %u, %x, %p, Helloworld, %% \n", "Helloworld", 'H', -16, -1, -1, -1);
    asm("ebreak");
    while(1);
    return 0;
}
