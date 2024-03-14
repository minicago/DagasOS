#include "file.h"
#include "spinlock.h"
#include "types.h"
#include "fs.h"
#include "print.h"

static spinlock_t cache_lock;
static struct inode inode[MAX_INODE];
static uint32 next[MAX_INODE];
static uint32 prev[MAX_INODE];
static uint32 head;

void inode_cache_init(void){
    init_spinlock(&cache_lock);
    head = 0;
    prev[head] = head;
    next[head] = head;
    
    for (int i = 0; i < MAX_INODE; i++){
        inode[i].dev = NULL_DEV;
        prev[i] = head;
        next[i] = next[head];
        prev[next[head]] = i;
        next[head] = i;
    }
}

// static struct inode* get_inode(uint32 dev, uint32 id) {
//     acquire_spinlock(&cache_lock);
//     for (int i = next[head]; i != head; i = next[i]){
//         if (inode[i].dev == dev && inode[i].id == id){
//             inode[i].refcnt++;
//             release_spinlock(&cache_lock);
//             return &inode[i];
//         }
//     }
//     for (int i = prev[head]; i != head; i = prev[i]){
//         if (inode[i].refcnt == 0){
//             inode[i].dev = dev;
//             inode[i].id = id;
//             inode[i].refcnt = 1;
//             inode[i].valid = 0;
//             release_spinlock(&cache_lock);
//             return &inode[i];
//         }
//     }
//     release_spinlock(&cache_lock);
//     panic("get_inode: no free inode");
//     return NULL;
// }

// inode* read_inode