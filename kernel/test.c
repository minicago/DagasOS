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
#include "elf.h"
#include "plic.h"
#include "console.h"
#include "file.h"

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
    printf("flush all disk\n");
    flush_cache_to_disk();
    return 1;
}

int print_test(){
    // Test for printf;
    printf("%s, %c, %d, %u, %x, %p, Helloworld, %% \n", "Helloworld", 'H', -16, -1, -1, -1);
    puts("test put");
    LOG("%s\n", "test LOG");
    #define PTEST 1
    TLOG(PTEST, "%s\n", "test TLOG true");
    #undef PTEST
    #define PTEST 0
    TLOG(PTEST, "%s\n", "test TLOG true");
    #undef PTEST
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

int syscall_test() {
    file_t* file = file_openat(get_root(), "test", 0,0);
    print_inode(file->node);
    return 1;
}

int kmalloc_test(){
    int* ptr = kmalloc(100);
    printf("ptr: %p\n",ptr);
    kfree(ptr);
    ptr = kmalloc(100);
    printf("ptr: %p\n",ptr);
    kfree(ptr);
    return 1;
}

int kernel_test(){
    printf("**************\nprint_test:\n");
    if(print_test() == 0) panic("print error!");
    else printf("print_test pass\n");

    printf("**************\nkmalloc_test:\n");
    if(kmalloc_test() == 0) panic("kmalloc error!"); 
    else printf("kmalloc_test pass\n");

    printf("**************\nspinlock_test:\n");
    if(spinlock_test() == 0) panic("spinlock error!"); 
    else printf("spinlock_test pass\n");

    printf("**************\ncoro_test:\n");
    if(coro_test() == 0) panic("coro error!"); 
    else printf("coro_test pass\n");


    //bio test halt?
    // printf("**************\nbio_test:\n");
    // if(bio_test() == 0) panic("bio error!");
    // else printf("bio_test pass\n");

    // printf("**************\nfat32_test:\n");
    // if(fat32_test() == 0) panic("fat32 error!");
    // else printf("fat32_test pass\n");

    // printf("**************\nfile_test:\n");
    // if(file_test() == 0) panic("file error!");
    // else printf("file_test pass\n");

    printf("**************\nsyscall_test:\n");
    if(syscall_test() == 0) panic("syscall error!");
    else printf("syscall_test pass\n");

    printf("********************************\n");
    printf("* Congrulation! ALL TEST PASS! *\n");
    printf("********************************\n");
    return 1;
}