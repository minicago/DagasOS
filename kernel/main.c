# include "types.h"
# include "uart.h"

void dosomething(){}

int main(){
    dosomething();
    uartinit();
    uartputc_sync('a');
    return 0;
}
