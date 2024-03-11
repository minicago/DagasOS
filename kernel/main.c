#include "print.h"
#include "strap.h"
#include "csr.h"
#include "pmm.h"
#include "vmm.h"
#include "virtio_disk.h"
#include "buf.h"

void dosomething(){}

int main(){
    dosomething();
    uartinit();
    strap_init();
    pmem_init();
    kvminit();
    virtio_disk_init();
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

    // Test for virtio_disk read
    struct buf* buf = palloc();
    buf->blockno = 0;
    buf->disk = 0;
    buf->valid = 0;
    buf->refcnt = 1;
    printf("print disk");
    virtio_disk_rw(buf, 0);
    for(int i=1;i<=1000;i++);
    printf("buf->disk: %d\n", buf->disk);
    printf("first 20 of buf->data: ");
    for(int i=0;i<20;i++){
        printf("%x ", buf->data[i]);
    }
    printf("\n");
    printf("last 20 of buf->data: ");
    for(int i=BSIZE-20-1;i<BSIZE;i++){
        printf("%x ", buf->data[i]);
    }
    printf("\n");


    // Open Intr
    intr_on();
    
    while(1);
    return 0;
}
