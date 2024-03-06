# include "types.h"
# include "uart.c"

void dosomething(){}

int main(){
    dosomething();
    uartputc_sync('a');
    return 0;
}
