# include "print.h"

void dosomething(){}

int main(){
    dosomething();
    uartinit();
    printf("%s, %c, %d, %u, %x, %p, Helloworld, %% \n", "Helloworld", 'H', -16, -1, -1, -1);
    return 0;
}
