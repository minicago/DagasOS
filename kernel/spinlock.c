#include"spinlock.h"
#include"types.h"

void init_spinlock(spinlock_t* spinlock){
    spinlock = 0;
    __sync_synchronize();
}

void acquire_spinlock(spinlock_t* spinlock){
    while (__sync_lock_test_and_set(spinlock, 1) != 0)
        ;
    __sync_synchronize();
    
}

void release_spinlock(spinlock_t* spinlock){
    __sync_lock_test_and_set(spinlock, 0);
    __sync_synchronize();
}