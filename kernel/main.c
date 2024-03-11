#include "print.h"
#include "strap.h"
#include "csr.h"
#include "pmm.h"
#include "vmm.h"
#include "spinlock.h"

int main(){
    uartinit();
    strap_init();
    pmem_init();
    kvminit();
    /*
        Follow codes should be replaced by user program
    */

    // Test for printf;
    printf("%s, %c, %d, %u, %x, %p, Helloworld, %% \n", "Helloworld", 'H', -16, -1, -1, -1);
    
    // Test for pmem management
    char* pg1 = palloc();
    char* pg2 = palloc();
    *pg1 = 'A';
    *pg2 = 'B';
    printf("pg1: %c, pg2: %c\n", *pg1, *pg2);
    pfree(pg1);
    printf("pg1: %u, pg2: %p\n", *pg1, *pg2);
    //asm("ebreak");

    //test for spinlock
    spinlock_t spinlock_test;
    init_spinlock(&spinlock_test);
    acquire_spinlock(&spinlock_test);
    // acquire_spinlock(&spinlock_test);
    release_spinlock(&spinlock_test);   

    // Open Intr
    intr_on();
    
    while(1);
    return 0;
}
