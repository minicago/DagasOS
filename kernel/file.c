#include "file.h"
#include "spinlock.h"
#include "types.h"
#include "fs.h"
#include "print.h"
#include "fat32.h"

static spinlock_t cache_lock;
static struct inode inode[MAX_INODE];
static uint32 next[MAX_INODE];
static uint32 prev[MAX_INODE];
static uint32 head;
struct inode root;
struct superblock root_superblock;

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

static struct inode* get_inode(uint32 dev, uint32 id) {
    acquire_spinlock(&cache_lock);
    for (int i = next[head]; i != head; i = next[i]){
        if (inode[i].dev == dev && inode[i].id == id){
            inode[i].refcnt++;
            release_spinlock(&cache_lock);
            return &inode[i];
        }
    }
    for (int i = prev[head]; i != head; i = prev[i]){
        if (inode[i].refcnt == 0){
            inode[i].dev = dev;
            inode[i].id = id;
            inode[i].refcnt = 1;
            inode[i].valid = 0;
            release_spinlock(&cache_lock);
            return &inode[i];
        }
    }
    release_spinlock(&cache_lock);
    panic("get_inode: no free inode");
    return NULL;
}

void filesystem_init(uint32 type) {
    root.sb = &root_superblock;
    switch(type) {
        case FS_TYPE_NULL:
            panic("filesystem_init: null filesystem");
            break;
        case FS_TYPE_FAT32:
            fat32_superblock_init(VIRTIO_DISK_DEV, root.sb);
            root.dev = VIRTIO_DISK_DEV;
            root.id = ((struct fat32_info *)root.sb->extra)->root_cid;
            root.refcnt = 1;
            root.valid = 1;
            root.type = T_DIR;
            root.size = 0;
            break;
        default:
            panic("filesystem_init: unknown filesystem");
            break;
    }
}

struct inode* lookup_inode(struct inode *dir, char *filename) {
    struct inode node;
    if (dir->type != T_DIR) {
        panic("lookup_inode: not a directory");
    }
    if (dir->valid == 0) {
        panic("lookup_inode: invalid inode");
    }
    if (dir->sb->lookup_inode(dir, filename, &node) == 0) {
        panic("lookup_inode: can't find file");
    }
    struct inode* res = get_inode(node.dev, node.id);
    acquire_spinlock(&cache_lock);
    if(res->valid) {
        res->refcnt++;
    } else {
        res->sb = node.sb;
        res->type = node.type;
        res->size = node.size;
        res->valid = 1;
        res->nlink = node.nlink;
    }
    release_spinlock(&cache_lock);
    return res;
}

void release_inode(struct inode *node) {
    acquire_spinlock(&cache_lock);
    if (node->refcnt == 0) {
        panic("release_inode: already released");
    }
    node->refcnt--;
    release_spinlock(&cache_lock);
}

int read_inode(struct inode *node, int offset, int size, void *buffer) {
    int real_size = size;
    
    //printf("read_inode: read %d bytes\n", real_size);
    if (node->valid == 0) {
        panic("read_inode: invalid inode");
    }
    if (((real_size = node->sb->read_inode(node, offset, size, buffer)) == 0)) {
        panic("read_inode: read error");
    }
    printf("read_inode: read %d bytes\n", real_size);
    return real_size;
}

void print_inode(struct inode *node) {
    printf("inode: dev=%d, id=%d, refcnt=%d, valid=%d, type=%d, size=%d\n", node->dev, node->id, node->refcnt, node->valid, node->type, node->size);
}

int file_test() {
    inode_cache_init();
    filesystem_init(FS_TYPE_FAT32);
    printf("file: filesystem init\n");
    print_inode(&root);
    struct inode *node = lookup_inode(&root, "test");
    print_inode(node);
    //printf("file: root txt's inode finished\n");
    char buffer[4096];
    //print_inode(node);
    int ss = node->size > 4096 ? 4096 : node->size;
    int size = read_inode(node, 0, ss, buffer);
    //printf("file: read test.txt finished%d\n",size);
    buffer[size] = '\0';
    for(int i = 0; i < size; i++) {
        if(buffer[i]<0x10) printf("0");
        printf("%x ", buffer[i]);
    }
    printf("\n");
    release_inode(node);
    return 1;
}