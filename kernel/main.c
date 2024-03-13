#include "print.h"
#include "strap.h"
#include "csr.h"
#include "pmm.h"
#include "vmm.h"
#include "virtio_disk.h"
#include "buf.h"
#include "bio.h"

void dosomething(){}
#include "spinlock.h"

int main(){
    uartinit();
    strap_init();
    pmem_init();
    kvminit();
    virtio_disk_init();
    block_cache_init();
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

    // Test for bio read and write
    printf("read disk 0\n");
    struct buf* buf = read_block(VIRTIO_DISK_DEV, 0);
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
    release_block(buf);

    printf("read disk 1\n");
    buf = read_block(VIRTIO_DISK_DEV, 1);
    printf("first 20 of buf->data: ");
    for(int i=0;i<20;i++){
        printf("%x ", buf->data[i]);
    }
    printf("\n");

    printf("write disk 1\n");
    for(int i=0;i<20;i++){
        buf->data[i]++;
    }
    write_block(buf);
    
    printf("read disk 1\n");
    printf("first 20 of buf->data: ");
    for(int i=0;i<20;i++){
        printf("%x ", buf->data[i]);
    }
    printf("\n");
    release_block(buf);

    printf("read disk 0\n");
    buf = read_block(VIRTIO_DISK_DEV, 0);
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
    release_block(buf);

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
