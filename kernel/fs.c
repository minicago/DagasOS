#include "file.h"
#include "spinlock.h"
#include "types.h"
#include "fs.h"
#include "print.h"
#include "fat32.h"
#include "bio.h"

static spinlock_t cache_lock;
static inode_t inode[MAX_INODE];
static uint32 next[MAX_INODE];
static uint32 prev[MAX_INODE];
static uint32 head;
static inode_t root = {.sb = NULL};
static superblock_t virtual_disk = {.fs_type = FS_TYPE_DISK,.parent=NULL,.real_dev = VIRTIO_DISK_DEV,.identifier=0};
static superblock_t root_superblock;
static int superblock_cnt = 1;

//TIP: can only get inode by lookup_inode or create_inode or look_up_path, and must release_inode after using

static void inode_cache_init(void){
    init_spinlock(&cache_lock);
    head = 0;
    prev[head] = head;
    next[head] = head;
    
    for (int i = 0; i < MAX_INODE; i++){
        inode[i].sb = NULL;
        prev[i] = head;
        next[i] = next[head];
        prev[next[head]] = i;
        next[head] = i;
    }
}

inode_t* get_root() {
    if(root.sb==NULL) panic("get_root: root not initialized");
    return &root;
}

inode_t* get_inode(superblock_t* sb, uint32 id) {
    acquire_spinlock(&cache_lock);
    for (int i = 0;i<MAX_INODE;i++){
        if (inode[i].sb!=NULL&&inode[i].sb->identifier == sb->identifier && inode[i].id == id){
            inode[i].refcnt++;
            release_spinlock(&cache_lock);
            if(inode[i].is_mnt) {
                LOG("get_inode: is_mnt %d\n",inode[i].is_mnt);
            }
            return &inode[i];
        }
    }
    for (int i = 0;i<MAX_INODE;i++){
        if (inode[i].refcnt == 0 && inode[i].is_mnt!=1){
            inode[i].sb = sb;
            inode[i].id = id;
            inode[i].refcnt = 1;
            inode[i].valid = 0;
            inode[i].is_mnt = 0;
            release_spinlock(&cache_lock);
            return &inode[i];
        }
    }
    release_spinlock(&cache_lock);
    panic("get_inode: no free inode");
    return NULL;
}

int get_new_sb_identifier() {
    return superblock_cnt++;
}

void filesystem_init(uint32 type) {
    inode_cache_init();
    root.sb = &root_superblock;
    switch(type) {
        case FS_TYPE_NULL:
            panic("filesystem_init: null filesystem");
            break;
        case FS_TYPE_FAT32:
            fat32_superblock_init(NULL,&virtual_disk, root.sb, get_new_sb_identifier());
            root.id = root.sb->root_id;
            root.refcnt = 1;
            root.valid = 1;
            root.type = T_DIR;
            root.size = 0;
            root.parent = (inode_t*)NULL;
            root.index_in_parent = -1;
            break;
        default:
            panic("filesystem_init: unknown filesystem");
            break;
    }
}

superblock_t* alloc_superblock() {
    superblock_t* sb = kmalloc(sizeof(superblock_t));
    return sb;
}

int free_superblock(superblock_t* sb) {
    sb->free_extra(sb->extra);
    kfree(sb);
    return 1;
}

int mount_inode(inode_t *dir, superblock_t *sb) {
    if (dir->type != T_DIR) {
        LOG("mount_inode: not a directory");
        return 0;
    }
    if (dir->valid == 0) {
        LOG("mount_inode: invalid inode");
        return 0;
    }
    dir->mnt_sb = sb;
    dir->mnt_root_id = sb->root_id;
    dir->is_mnt = 1;
    return 1;
}

int umount_inode(inode_t *dir) {
    if (dir->type != T_DIR) {
        LOG("umount_inode: not a directory");
        return 0;
    }
    if (dir->valid == 0) {
        LOG("umount_inode: invalid inode");
        return 0;
    }
    if(dir->is_mnt == 0) {
        LOG("umount_inode: not a mounted directory");
        return 0;
    }
    if(dir->parent==NULL) {
        LOG("umount_inode: root can't be unmounted or just have no parent");
        return 0;
    }
    free_superblock(dir->mnt_sb);
    dir->mnt_sb = NULL;
    dir->mnt_root_id = -1;
    dir->is_mnt = 0;
    return 1;
}

inode_t* lookup_inode(inode_t *dir, char *filename) {
    inode_t node;
    if (dir->type != T_DIR) {
        panic("lookup_inode: not a directory");
    }
    if (dir->valid == 0) {
        panic("lookup_inode: invalid inode");
    }
    if(strcmp(filename, ".")==0) {
        pin_inode(dir);
        return dir;
    }
    if(strcmp(filename, "..")==0) {
        pin_inode(dir->parent);
        return dir->parent;
    }
    printf("lookup_inode: is_mnt %d\n",dir->is_mnt);
    if(dir->is_mnt) {
        LOG("lookup_inode: begin mnt_sb id %d\n", dir->mnt_sb->identifier);
        if (dir->mnt_sb->lookup_inode(dir, filename, &node) == 0) {
            printf("lookup_inode: mnt_sb id %d\n", dir->mnt_sb->identifier);
            printf("lookup_inode: can't find file %s\n",filename);
            return NULL;
        }
    } else if (dir->sb->lookup_inode(dir, filename, &node) == 0) {
        printf("lookup_inode: sb id %d\n", dir->sb->identifier);
        printf("lookup_inode: can't find file %s\n",filename);
        return NULL;
    }
    printf("lookup_inode: find file %s\n",filename);
    inode_t* res = get_inode(node.sb, node.id);
    //LOG("lookup_inode: get_inode finished\n");
    acquire_spinlock(&cache_lock);
    printf("lookup_inode: file %s valid %d\n", filename, res->valid);
    if(!(res->valid)) {
        *res = node;
        res->refcnt = 1;
        res->is_mnt = 0;
    }
    release_spinlock(&cache_lock);
    return res;
}

void release_inode(inode_t *node) {
    if(node==&root) {
        //panic("release_inode: root can't be released\n");
        printf("warning: release_inode: root can't be released\n");
        return;
    }
    if(node==NULL) {
        printf("warning: release_inode: node == NULL\n");
        return;
    }
    acquire_spinlock(&cache_lock);
    if (node->refcnt == 0) {
        panic("release_inode: already released");
    }
    node->refcnt--;
    release_spinlock(&cache_lock);
    if (node->refcnt == 0) {
        if(node->parent!=NULL) release_inode(node->parent);
        update_inode(node);
    }
}

int read_inode(inode_t *node, int offset, int size, void *buffer) {
    int real_size = size;
    
    //printf("read_inode: read %d bytes\n", real_size);
    if (node->valid == 0) {
        panic("read_inode: invalid inode");
    }
    if (((real_size = node->sb->read_inode(node, offset, size, buffer)) == -1)) {
        panic("read_inode: read error");
    }
    printf("read_inode: read %d bytes\n", real_size);
    return real_size;
}

// if cover is 1, will change size even so not write to the last
int write_inode(inode_t *node, int offset, int size,int cover, void *buffer) {
    int real_size = size;
    if (node->valid == 0) {
        panic("write_inode: invalid inode");
    }
    if ((real_size = node->sb->write_inode(node, offset, size,cover, buffer)) == -1) {
        panic("write_inode: write error");
    }
    return real_size;
}

void update_inode(inode_t *node) {
    if (node->valid == 0) {
        panic("update_inode: invalid inode");
    }
    node->sb->update_inode(node);
}

inode_t* create_inode(inode_t* dir, char* filename, uint8 major, uint8 type) {
    inode_t node;
    if (dir->type != T_DIR) {
        panic("create_inode: not a directory");
    }
    if (dir->valid == 0) {
        panic("create_inode: invalid inode");
    }
    
    if ((dir->sb->create_inode(dir, filename, type, major, &node)) == 0) {
        printf("create_inode: create error");
        return NULL;
    }
    inode_t* res = get_inode(node.sb, node.id);
    acquire_spinlock(&cache_lock);
    if(!res->valid) {
        *res = node;
        res->refcnt = 1;
    }
    release_spinlock(&cache_lock);
    return res;
}

void print_inode(inode_t *node) {
    printf("inode: sb->identifier=%d, id=%d, refcnt=%d, valid=%d, type=%d, size=%d\n", node->sb->identifier, node->id, node->refcnt, node->valid, node->type, node->size);
}

int file_test() {
    inode_t* root = get_root();
    print_fs_info(root);
    print_inode(root);
    inode_t *node = look_up_path(root, "test", NULL);
    print_inode(node);
    //printf("file: root txt's inode finished\n");
    //char buffer[4096];


    //print_inode(node);


    // {
    // int ss = node->size > 4096 ? 4096 : node->size;
    // int size = read_inode(node, 1040, ss, buffer);
    // //printf("file: read test.txt finished%d\n",size);
    // buffer[size] = '\0';
    // for(int i = 0; i < size; i++) {
    //     if(buffer[i]<0x10) printf("0");
    //     printf("%x ", buffer[i]);
    // }
    // printf("\n");
    // release_inode(node);}
    
    inode_t *ndir = create_inode(root, "newdir", 0, T_DIR);
    print_inode(ndir);
    inode_t *ndev = create_inode(ndir, "newdev", 0, T_DEVICE);
    print_inode(ndev);
    release_inode(ndir);
    release_inode(ndev);
    printf("file_test: flush\n");
    flush_cache_to_disk();
    
    printf("file_test: done\n");
    return 1;
}

void print_fs_info(inode_t *node) {
    if (node->valid == 0) {
        panic("print_fs_info: invalid inode");
    }
    node->sb->print_fs_info(node);
}

int load_from_inode_to_page(inode_t *inode, pagetable_t pagetable, uint64 va, int offset, int size){
    uint64 va_l = PG_FLOOR(va);
    uint64 va_r = PG_CEIL(va + size);
    printf("%p, %p\n",va_l, va_r);
    int sum_size = 0;
    for(uint64 i = va_l; i < va_r; i += PG_SIZE){
        pte_t* pte = walk(pagetable, i, 0);
        uint64 pa;
        if(pte == 0 || !(*pte & PTE_V)){
            // pa = (uint64) palloc();
            // mappages(pagetable, i, pa, PG_SIZE, perm);
            panic("load_from_inode_to_page: no map!\n");
            return sum_size;
        } 
        else {
            pa = PTE2PA(*pte);
            // if((*pte | perm) != perm) return 0;
        }
        int size_in_page = PG_SIZE;
        int real_size = 0;
        if (i + PG_SIZE > va + size) {
            size_in_page -= (i + PG_SIZE) - (va + size);
        }        
        if (i < va){
            size_in_page -= va -  i;
            real_size = read_inode(inode, offset, size_in_page,(void*) (pa + va - i));
        }
        else {
            real_size = read_inode(inode, i - va + offset, size_in_page,(void*) pa);
        }
        sum_size += real_size;
        if (real_size != size_in_page) break;
    }
    return sum_size;
}

//TODO: relative path
//depth is the last success dir's index in path
//the first char of path can't be '/'
inode_t* look_up_path(inode_t* root,const char *ori_path, int* depth){
   
    inode_t* res = root;
    char *mem = palloc();
    // LOG("%p\n",ori_path);
    strcpy(mem, ori_path);
    // LOG("%p\n",mem);

    char* path = mem; 
    if(ori_path[0] == '/') {
        res = get_root();
        path++;
    }
    char* filename = path;
    if(depth!=NULL) *depth = 0;
    int end = 0;
    for(int i = 0; ; i++){
        if(path[i] == '/' || path[i] == '\0'){
            if(path[i] == '\0') end = 1;
            path[i] = '\0';
            inode_t* tmp = res;
            res = lookup_inode(res, filename);
            if(tmp!=root) release_inode(tmp);
            if(res==NULL) goto look_up_path_error;
            if(depth!=NULL) (*depth)++;
            filename = path + i + 1;
            if(end != 0) break;
            path[i] = '/';
        }
    }
    pfree(mem);
    return res;

look_up_path_error:
    pfree(mem);
    return NULL;
} 

void pin_inode(inode_t* node) {
    if(node==&root) {
        //panic("pin_inode: root can't be pinned\n");
        printf("warning: pin_inode: root can't be pinned\n");
        return;
    }
    acquire_spinlock(&cache_lock);
    if (node->refcnt == 0) {
        panic("pin_inode: already released");
    }
    node->refcnt++;
    release_spinlock(&cache_lock);
}

// size include dirent other filed not just d_name, return bytes read
int get_dirent(inode_t *node, int size, dirent_t *dirent) {
    if (node->valid == 0) {
        panic("get_dirent: invalid inode");
    }
    return node->sb->get_dirent(node, size, dirent);
}

// TODO: inode_t should have lock
int get_inode_path(inode_t *node, char *buf, int size) {
    if(node==NULL) {
        printf("get_file_path: file->node == NULL\n");
        return -1;
    }
    if(node->type!=T_FILE && node->type!=T_DIR) {
        printf("get_file_path: node->type error\n");
        return -1;
    }
    if(node==get_root()) {
        if(size<2) {
            printf("get_file_path: size is too small\n");
            return -1;
        }
        buf[0] = '/';
        buf[1] = '\0';
        return 0;// len-1, because root should be handled specially
    }
    if(node->parent==NULL) {
        printf("get_file_path: node->parent == NULL\n");
        return -1;
    }
    int len = get_inode_path(node->parent, buf, size);
    if(len==-1) return -1;
    buf+=len;
    *buf = '/';
    buf++;
    size -= len+1;
    //TODO: change to get_node_name
    int res = get_inode_name(node, buf, size);
    
    printf("get_inode_path1: size%d %s\n",size, buf);
    printf("get_inode_path2: size%d %s\n",size-len-1, buf-len-1);
    printf("get_inode_path3: len%d\n",res+len+1);
    if(res == -1) return -1;
    return res+len+1;
}

int get_inode_name(inode_t* node, char* buffer, int size) {
    return node->sb->get_inode_name(node,buffer,size);
}

int get_real_dev(superblock_t *sb) {
    if(sb==NULL) return -1;
    while(sb->parent!=NULL) {
        sb = sb->parent;
    }
    return sb->real_dev;
}

int get_real_bid(superblock_t *sb, int bid) {
    if(sb==NULL) return -1;
    int id_in_parent = sb->id_in_parent;
    while(sb->parent!=NULL) {
        sb = sb->parent;
        bid = sb->get_bid_from_son(sb, id_in_parent, bid);
        id_in_parent = sb->id_in_parent;
    }
    return bid;
}