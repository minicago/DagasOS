#ifndef __WAITQUEUE__H__
#define __WAITQUEUE__H__
#include "thread.h"
#include "spinlock.h"
typedef struct wait_node_struct wait_node_t;
typedef struct wait_queue_struct wait_queue_t;
typedef struct thread_struct thread_t;

struct wait_node_struct{
    wait_node_t *next;
    thread_t* thread;
    uint64* value; 
};

#define WAIT_QUEUE_ONERELEASE 0x0
#define WAIT_QUEUE_ALLRELEASE 0x1

struct wait_queue_struct{
    spinlock_t lock;
    wait_node_t *left, **right;
    int type;
};

wait_queue_t* alloc_wait_queue(int type);
void wait_queue_push_back(wait_queue_t* wait_queue, thread_t* thread, uint64* value);
void wait_queue_pop(wait_queue_t* wait_queue);
void free_wait_queue(wait_queue_t* wait_queue);
void awake_wait_queue(wait_queue_t* wait_queue, uint64 value);

#endif