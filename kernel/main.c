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
#include "test.h"

int main(){
    intr_off();
    uartinit();
    printf("uart init finished!\n");
    strap_init();
    printf("strap init finished!\n");
    pmem_init();
    printf("pmem init finished!\n");
    kvminit();
    printf("kvm init finished!\n");
    printf("trapret:%p\n", *(uint64*) trampoline );
    virtio_disk_init();
    printf("virtio disk init finished!\n");
    block_cache_init();
    printf("block cache init finished!\n");
    plic_init();
    plic_init_hart();
    printf("plic init finished!\n");
    init_as_scheduler();
    process_pool_init();
    thread_pool_init();        

    intr_on();
    
    filesystem_init(FS_TYPE_FAT32);
    printf("filesystem init finished!\n");
    console_init();
    printf("console init finished!\n");
    kernel_test();

    process_t* p = alloc_process(); 
    printf("get process\n");
    init_process(p);
    prepare_initcode_process(p);
    printf("init process\n");
    load_elf(p, "test");
    // map_elf(p);
    //map_elf(p);
    printf("load process\n");

    char* argv[] = {"/test","test"};
    set_arg(p, 1, argv);
    // map_elf(p);
    //map_elf(p);
    printf("set arg\n");   

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

    printf("fake_stack:%p\n", FAKE_STACK0);

    scheduler_loop();
    
    /*
        Follow codes should be replaced by user program
    */
 

    // Open Intr
    
    
    while(1);
    return 0;
}
