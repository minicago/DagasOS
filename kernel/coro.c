# include "coro.h"
# include "thread.h"
# include "cpu.h"

coro_t thread_manager_coro[MAX_THREAD];

// in the future to realize multihart, we need to

// coro_t scheduler_coro;

coro_t* current;

void switch_coro(coro_t* dest){
    if(coro_setjmp(&get_cpu()->current_coro->env) == 0){
        get_cpu()->current_coro = dest;
        coro_longjmp(&dest->env, 1);
    }
}

void init_thread_manager_coro(uint64 tid){
    get_cpu()->current_coro = &thread_manager_coro[tid];
    return ;
}

void coro_init(){

} 

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

void fake_entry(coro_t* coro, uint64 entry){
    coro->env.ra = entry;
}

void fake_sp(coro_t* coro, uint64 sp){
    coro->env.sp = sp;
}

void clean_s(coro_t* coro, uint64 sp){
    memset(coro->env.s, 0, sizeof(coro->env.s));
}



