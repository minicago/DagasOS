# include "coro.h"
# include "thread.h"
# include "cpu.h"

coro_t thread_manager_coro[MAX_THREAD];
coro_t scheduler_coro;

coro_t* current;

void switch_coro(coro_t* dest){
    if(coro_setjmp(&get_cpu()->current_coro->env) == 0){
        get_cpu()->current_coro = dest;
        coro_longjmp(&dest->env, 1);
    }
}

void init_thread_manager_coro(uint64 tid){
    get_cpu()->current_coro = &thread_manager_coro[tid];
    switch_coro(&scheduler_coro);
    return ;
}

void coro_init(){
    get_cpu()->current_coro = &scheduler_coro;
    for(uint64 i = 0; i < MAX_THREAD; i++){
        init_thread_manager_coro(i);
        //thread_manager_coro[i].env.ra =(uint64) forkret;
        //thread_manager_coro[i].env.sp = 
    }
}
// current == 

void scheduler_loop(){
    while (1)
    {
        for (int i = 0 ; i < MAX_THREAD; i++){
            if (try_acquire_spinlock(&thread_pool[i].lock) != 0){
                if(thread_pool[i].state == T_READY){
                    thread_pool[i].state = T_RUNNING;
                    switch_coro(&thread_manager_coro[i]);
                }
                
                release_spinlock(&thread_pool[i].lock);
            }
        }
    }
    

}



