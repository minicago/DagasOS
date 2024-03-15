#include "print.h"
#include "strap.h"
#include "csr.h"
#include "pmm.h"
#include "vmm.h"
#include "virtio_disk.h"
#include "buf.h"
#include "bio.h"
#include "spinlock.h"
#include "coro.h"
#include "fat32.h"

int bio_test(){
    //Test for bio read and write
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
    flush_cache_to_disk();
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

    printf("**************\nbio_test:\n");
    if(bio_test() == 0) panic("bio error!");
    else printf("bio_test pass\n");

    // printf("**************\nfat32_test:\n");
    // if(fat32_test() == 0) panic("fat32 error!");
    // else printf("fat32_test pass\n");

    printf("**************\nfile_test:\n");
    if(file_test() == 0) panic("file error!");
    else printf("file_test pass\n");

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
    block_cache_init();
    printf("block cache init finished!\n");

    init_as_scheduler();
    
    kernel_test();

    process_pool_init();
    thread_pool_init();

    

    process_t* p = alloc_process();
    printf("get process\n");
    init_process(p);
    printf("init process\n");
    map_elf(p);
    printf("map process\n");
    

    thread_t* t = alloc_thread();
    printf("get thread\n");
    init_thread(t);
    printf("init thread\n");
    attach_to_process(t, p);
    printf("attached done\n");
    

    init_thread_manager_coro(t->tid);
    printf("init manager done\n");
    entry_main(t);
    printf("entry done\n");

    scheduler_loop();
    
    /*
        Follow codes should be replaced by user program
    */
 

    // Open Intr
    intr_on();
    
    while(1);
    return 0;
}
