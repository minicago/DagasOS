#include "waitqueue.h"

wait_queue_t* alloc_wait_queue(int type){
    wait_queue_t* wait_queue = kmalloc(sizeof(wait_queue_t));
    wait_queue->type = type;
    wait_queue->left = NULL;
    wait_queue->right = &(wait_queue->left);
    return wait_queue;
}

void wait_queue_push_back(wait_queue_t* wait_queue, thread_t* thread, uint64* value){
    wait_node_t* wait_node = kmalloc(sizeof(wait_node_t));
    wait_node->thread = thread;
    wait_node->value = value;
    wait_node->next = NULL;
    *(wait_queue->right) = wait_node;
    wait_queue->right = &(wait_node->next);
}

void wait_queue_pop(wait_queue_t* wait_queue){
    wait_node_t* wait_node = wait_queue->left;
    wait_queue->left = wait_node->next;
    if(wait_queue->left == NULL)
        wait_queue->right = &(wait_queue->left);
    kfree(wait_node);
}

void free_wait_queue(wait_queue_t* wait_queue){
    while(wait_queue->left != NULL) wait_queue_pop(wait_queue);
    kfree(wait_queue);
}

void awake_wait_queue(wait_queue_t* wait_queue, uint64 value){
    wait_node_t* wait_node;
    while(wait_queue->left != NULL){
        wait_node = wait_queue->left;
        LOG("wake up:%d %p %d\n", wait_node->thread->tid, wait_node->thread->waiting,wait_node->thread->state);
        if(wait_node->thread->waiting != wait_queue || wait_node->thread->state != T_SLEEPING) {
            wait_queue_pop(wait_queue);
            continue;
        }
        
        *wait_node->value = value;
        awake(wait_node->thread);
        LOG("really wake up:%d %p %d\n", wait_node->thread->tid, wait_node->thread->waiting,wait_node->thread->state);
        wait_queue_pop(wait_queue);
        if(!(wait_queue->type | WAIT_QUEUE_ALLRELEASE)) break;
    }
}