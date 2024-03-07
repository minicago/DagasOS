# include "types.h"
# include "uart.c"

void dosomething(){}

int main(){
    dosomething();
    uartinit();
    uartputc_sync('a');
    return 0;
}
