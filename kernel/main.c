#include "print.h"
#include "strap.h"
#include "csr.h"
#include "pmm.h"
#include "vmm.h"
#include "virtio_disk.h"
#include "buf.h"
#include "spinlock.h"
#include "coro.h"

int virtio_test(){
    // Test for virtio_disk read and write
    struct buf* buf = palloc();
    buf->blockno = 0;
    buf->disk = 0;
    buf->valid = 0;
    buf->refcnt = 1;
    printf("read disk 0\n");
    virtio_disk_rw(buf, 0);
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

    printf("write disk 1\n");
    buf->blockno = 1;
    for(uint8 i=0;i<20;i++) {
        buf->data[i] = i;
    }
    for(int i=BSIZE-20-1;i<BSIZE;i++) {
        buf->data[i] = BSIZE-i;
    }
    virtio_disk_rw(buf, 1);

    printf("read disk 1\n");
    virtio_disk_rw(buf, 0);
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
    return 1;
}

int print_test(){
    // Test for printf;
    printf("%s, %c, %d, %u, %x, %p, Helloworld, %% \n", "Helloworld", 'H', -16, -1, -1, -1);
    return 1;
}

int coro_test(){
    //test for coro;
    int checkpoint = 2;
    coro_t coro_test;
    if(coro_setjmp(&coro_test.env) == 0){
        printf("coro check point one\n");
        checkpoint--;
        coro_longjmp(&coro_test.env, 1);
    }else {
        printf("coro check point two\n");
        checkpoint--;
    }
    if(checkpoint == 0) return 1;
    else return 0;
}

int spinlock_test(){
    int checkpoint = 2;
    spinlock_t spinlock_test;
    init_spinlock(&spinlock_test);
    if (try_acquire_spinlock(&spinlock_test) ){
        printf("spinlock check point one\n");
        checkpoint --;
    }
    if (try_acquire_spinlock(&spinlock_test) == 0){
        printf("spinlock check point two\n");
        checkpoint --;
    }
    release_spinlock(&spinlock_test);
    if(checkpoint == 0) return 1;
    else return 0;      
}

int kernel_test(){
    printf("**************\nprint_test:\n");
    if(print_test() == 0) panic("print error!");
    else printf("print_test pass\n");

    printf("**************\nspinlock_test:\n");
    if(spinlock_test() == 0) panic("spinlock error!"); 
    else printf("spinlock_test pass\n");

    printf("**************\ncoro_test:\n");
    if(coro_test() == 0) panic("coro error!"); 
    else printf("coro_test pass\n");

    printf("**************\nvirtio_test:\n");
    if(virtio_test() == 0) panic("virtio error!");
    else printf("virtio_test pass\n");

    printf("********************************\n");
    printf("* Congrulation! ALL TEST PASS! *\n");
    printf("********************************\n");
    return 1;
}

int main(){
    
    uartinit();
    printf("uart init finished!\n");
    strap_init();
    printf("strap init finished!\n");
    pmem_init();
    printf("pmem init finished!\n");
    kvminit();
    printf("kvm init finished!\n");
    virtio_disk_init();
    printf("virtio disk init finished!\n");
    init_as_scheduler();
    
    kernel_test();

    

    

    process_t* p = alloc_process();
    printf("get process\n");
    init_process(p);
    map_elf(p);
    

    thread_t* t = alloc_thread();
    init_thread(t);
    attach_to_process(t, p);

    init_thread_manager_coro(t->tid);

    entry_main(t);

    scheduler_loop();
    
    /*
        Follow codes should be replaced by user program
    */
 

    // Open Intr
    intr_on();
    
    while(1);
    return 0;
}
