#include "file.h"
#include "bio.h"
#include "types.h"
#include "pmm.h"
#include "print.h"
#include "dagaslib.h"
#include "fat32.h"
#include "fs.h"

#define MAX_CLUSTER_SIZE 512*8 // means don't support cluster size larger than 2KB

#pragma pack(1)
struct sfn_entry
{
    char name[8];
    char ex_name[3];
    uint8 type;
    uint8 r1;
    union 
    {
        uint8 tms; // 10ms in normal file
        uint8 major; // in device file
    };
    uint16 sec : 5; // 2s
    uint16 min : 6;
    uint16 hour : 5;
    uint16 day : 5;
    uint16 mon : 4;
    uint16 year : 7;
    uint16 recent_day : 5;
    uint16 recent_mon : 4;
    uint16 recent_year : 7;
    uint16 high_cid;
    uint16 change_sec : 5;
    uint16 change_min : 6;
    uint16 change_hour : 5;
    uint16 change_day : 5;
    uint16 change_mon : 4;
    uint16 change_year : 7;
    uint16 low_cid;
    uint32 size;
};

#pragma pack(1)
struct lfn_entry
{
    uint8 id;
    uint16 name1[5];
    uint8 type;
    uint8 r1;
    uint8 check;
    uint16 name2[6];
    uint16 r2;
    uint16 name3[2];
};

static uint8 new_dir_date[BSIZE] = {
    0x2e, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x00, 0x90, 0xce, 0x50,
    0x7e, 0x58, 0x7e, 0x58, 0x00, 0x00, 0xce, 0x50, 0x7e, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2e, 0x2e, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x00, 0x90, 0xce, 0x50,
    0x7e, 0x58, 0x7e, 0x58, 0x00, 0x00, 0xce, 0x50, 0x7e, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static uint8 lfn_checksum (const uint8 *pFcbName);
static void utf16_to_ascii(const uint16 *utf16_str, char *ascii_str, uint32 len);
static void lfn2name(const struct lfn_entry *lfn, char *name);
static void name2lfn(const char *name, struct lfn_entry *lfn, uint8 checksum);
static void print_sfn_entry(struct sfn_entry *entry);
// static void print_lfn_entry(struct lfn_entry *entry);
static int fat32_lookup_inode(inode_t *dir, char *filename, inode_t *node);
static int fat32_read_inode(inode_t *node, int offset, int size, void *buffer);
static void print_fat_info(inode_t* node);
static int find_first_entry(void *buffer, uint32 size, struct sfn_entry *entry, char *name);
static int get_cluster_offset(superblock_t *sb, uint32 cid);
static int get_next_cid(superblock_t *sb, uint32 cid);
static int lookup_entry(superblock_t *sb, uint32 cid, char *filename, struct sfn_entry *entry);
static void sfn_entry2inode(superblock_t *sb, struct sfn_entry *entry,inode_t *parent,int index, inode_t *node);
static void fat32_update_inode(inode_t *node);
static void set_fat(superblock_t *sb, uint32 cid, uint32 value);
static int fresh_fat(superblock_t *sb);
static int get_free_cluster(superblock_t *sb);
static int fat32_create_inode(inode_t* dir, char* filename, uint8 type, uint8 major, inode_t* node);


static uint8 lfn_checksum (const uint8 *pFcbName)
{
	int i;
	uint8 sum=0;

	for (i=11; i; i--)
		sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *pFcbName++;
	return sum;
}

static void utf16_to_ascii(const uint16 *utf16_str, char *ascii_str, uint32 len)
{
    for (int i = 0; i < len; ++i)
    {
        *ascii_str++ = (char)(*utf16_str++);
    }
    *ascii_str = '\0';
}

static void lfn2name(const struct lfn_entry *lfn, char *name)
{
    utf16_to_ascii(lfn->name1, name, 5);
    utf16_to_ascii(lfn->name2, name + 5, 6);
    utf16_to_ascii(lfn->name3, name + 11, 2);
}

static void name2lfn(const char *name, struct lfn_entry *lfn, uint8 checksum)
{
    lfn->check = checksum;
    lfn->type = FAT32_T_LFN;
    int len = 0;
    while(*name && len<5) {
        lfn->name1[len] = *name;
        name++;
        len++;
    }
    len = 0;
    while(*name && len<6) {
        lfn->name2[len] = *name;
        name++;
        len++;
    }
    len = 0;
    while(*name && len<2) {
        lfn->name3[len] = *name;
        name++;
        len++;
    }
}

static void print_sfn_entry(struct sfn_entry *entry)
{
    printf("Name: ");
    for (int i = 0; i < 8; ++i)
    {
        printf("%c", entry->name[i]);
    }
    printf("\n");
    printf("Ex name: ");
    for (int i = 0; i < 3; ++i)
    {
        printf("%c", entry->ex_name[i]);
    }
    printf("\n");
    printf("Type: %02x\n", entry->type);
    printf("CreateDate: %d-%d-%d %d:%d:%d\n", entry->year + 1980, entry->mon,
           entry->day, entry->hour, entry->min, entry->sec * 2);
    printf("RecentDate: %d-%d-%d\n", entry->recent_year + 1980, entry->recent_mon, entry->recent_day);
    printf("ChangeDate: %d-%d-%d %d:%d:%d\n", entry->change_year + 1980, entry->change_mon, entry->change_day,
           entry->change_hour, entry->change_min, entry->change_sec * 2);
    printf("Cluster: %d\n", entry->high_cid << 16 | entry->low_cid);
    printf("Size: %d\n", entry->size);
}

// static void print_lfn_entry(struct lfn_entry *entry)
// {
//     printf("ID: %02x\n", entry->id);
//     printf("Type: %02x\n", entry->type);
//     printf("Check: %02x\n", entry->check);
//     char name[14];
//     lfn2name(entry, name);
//     name[13] = '\0';
//     printf("Name: %s\n", name);
// }

static int fat32_lookup_inode(inode_t *dir, char *filename, inode_t *node)
{
    superblock_t *sb = dir->sb;
    if (sb->fs_type != FS_TYPE_FAT32)
    {
        printf("fat32: not a fat32 filesystem\n");
        return 0;
    }
    struct sfn_entry entry;
    int index;
    printf("fat32: lookup inode\n");
    if ((index=lookup_entry(sb, dir->id, filename, &entry))!=-1)
    {
        printf("fat32: find file\n");
        sfn_entry2inode(sb, &entry,dir,index, node);
        return 1;
    }
    return 0;
}

static int fat32_read_inode(inode_t *node, int offset, int size, void *buffer)
{
    superblock_t *sb = node->sb;
    int cluster_size = sb->block_size;
    int real_size = size;
    //printf("fat32_read_inode: begin read file\n");
    if (sb->fs_type != FS_TYPE_FAT32)
    {
        printf("fat32: not a fat32 filesystem\n");
        return -1;
    }
    if (offset < 0 || offset > node->size)
    {
        printf("fat32: offset is invalid\n");
        return -1;
    }
    if (offset + size > node->size)
    {
        real_size = size = (node->size - offset);
    }
    uint32 cid = node->id;
    while (offset >= cluster_size)
    {
        cid = get_next_cid(sb, cid);
        if (!FAT32_CID_IS_VALID(cid))
        {
            printf("fat32: cid is invalid\n");
            return -1;
        }
        offset -= cluster_size;
        
        //printf("fat32_read_inode: cid%d\n",cid);
    }
    //printf("fat32_read_inode:cid%d off%d %d \n",cid,offset, size);
    while (size > 0)
    {
        if (!FAT32_CID_IS_VALID(cid))
        {
            printf("fat32: cid is invalid\n");
            return -1;
        }
        if (size >= cluster_size - offset)
        {
            read_bytes_to_buffer(sb->dev, get_cluster_offset(sb, cid), offset, cluster_size - offset, buffer);
            buffer += cluster_size - offset;
            size -= cluster_size - offset;
        }
        else
        {
            read_bytes_to_buffer(sb->dev, get_cluster_offset(sb, cid), offset, size, buffer);
            buffer += size;
            size = 0;
        }
        offset = 0;
        cid = get_next_cid(sb, cid);
        //printf("fat32_read_inode: %d\n",size);
    }
    return real_size;
}

// will not fresh immediately
static void set_fat(superblock_t *sb, uint32 cid, uint32 value)
{
    ((fat32_info_t *)sb->extra)->fat[cid] = value;
}

static int fresh_fat(superblock_t *sb)
{
    if(sb->fs_type != FS_TYPE_FAT32)
    {
        printf("fat32: not a fat32 filesystem\n");
        return 0;
    }
    fat32_info_t *info = (fat32_info_t *)sb->extra;
    //fat1
    write_to_disk(sb->dev, info->fat_offset * info->blocks_per_sector, info->fat_blocks, info->fat);
    //fat2
    write_to_disk(sb->dev, info->fat_offset * info->blocks_per_sector + info->sectors_per_fat * info->blocks_per_sector,
     info->fat_blocks, info->fat);
    return 1;
}

void fat32_superblock_init(uint32 dev, superblock_t *sb)
{
    // read boot sector
    struct buf *b = read_block(dev, 0);
    sb->extra = palloc();
    printf("fat32: sb->extra%x\n",sb->extra);
    sb->dev = dev;
    sb->fs_type = FS_TYPE_FAT32;

    fat32_info_t *info = (fat32_info_t *)sb->extra;
    info->bytes_per_sector = *(uint16 *)(b->data + 0xb);
    info->sectors_per_cluster = *(uint8 *)(b->data + 0xd);
    info->reserved_sectors = *(uint16 *)(b->data + 0xe);
    info->hidden_sectors = *(uint32 *)(b->data + 0x1c);
    info->fat_cnt = *(uint8 *)(b->data + 0x10);
    uint32 tmp1 = *(uint16 *)(b->data + 0x13);
    uint32 tmp2 = *(uint32 *)(b->data + 0x20);
    info->total_sectors = tmp1 == 0 ? tmp2 : tmp1;
    info->sectors_per_fat = *(uint32 *)(b->data + 0x24);
    info->root_cid = *(uint32 *)(b->data + 0x2c);
    info->fat_offset = info->reserved_sectors;
    info->root_offset = info->fat_offset + info->fat_cnt * info->sectors_per_fat + info->hidden_sectors + (info->root_cid - 2) * info->sectors_per_cluster;

    // read fat
    info->blocks_per_sector = info->bytes_per_sector / BSIZE;
    info->blocks_per_cluster = info->sectors_per_cluster * info->blocks_per_sector;
    uint32 fat_blocks = info->sectors_per_fat * info->blocks_per_sector;
    if (fat_blocks * BSIZE > PG_SIZE)
    {
        printf("fat32: fat size is %d B %d %d\n", fat_blocks * BSIZE, info->bytes_per_sector,info->sectors_per_cluster );
        // fat_blocks = PG_SIZE / BSIZE;
        printf("fat32: warning: fat size is larger than one page size\n");
    }

    //TODO: make the memory alloced to fat is continuous by more official function
    info->fat_blocks = fat_blocks;
    int fat_size = fat_blocks*BSIZE;
    // fat_size -= PG_SIZE;
    // uint64 tmp = (uint64)palloc();
    // while(fat_size > 0) {
    //     uint64 tmp2 = (uint64)palloc();
    //     if(tmp2+PG_SIZE != tmp) {
    //         panic("fat32: fat memory is not continuous\n");
    //     }
    //     fat_size -= PG_SIZE;
    //     tmp = tmp2;
    // }
    int fat_pg_num = PG_CEIL(fat_size) / PG_SIZE;
    info->fat = (uint32 *)palloc_n(fat_pg_num);
    printf("fat32: info->fat%p\n",info->fat);
    read_to_buffer(dev, info->fat_offset * info->blocks_per_sector, fat_blocks, info->fat);

    sb->block_size = info->sectors_per_cluster * info->bytes_per_sector;
    info->fat_items = info->sectors_per_fat * info->bytes_per_sector / 4;

    // init ops
    sb->lookup_inode = fat32_lookup_inode;
    sb->read_inode = fat32_read_inode;
    sb->update_inode = fat32_update_inode;
    sb->create_inode = fat32_create_inode;
    sb->print_fs_info = print_fat_info;
}

static void print_fat_info(inode_t *node)
{
    if(node->sb->fs_type != FS_TYPE_FAT32)
    {
        printf("fat32: not a fat32 filesystem\n");
        return;
    }
    fat32_info_t *info = node->sb->extra;
    printf("This is FAT32 File System\n");
    printf("Bytes per sector: %d\n", info->bytes_per_sector);
    printf("Sectors per cluster: %d\n", info->sectors_per_cluster);
    printf("Reserved sector count: %d\n", info->reserved_sectors);
    printf("Number of FATs: %d\n", info->fat_cnt);
    printf("Total sectors: %d\n", info->total_sectors);
    printf("Sectors per FAT: %d\n", info->sectors_per_fat);
    printf("Blocks per sector: %d\n", info->blocks_per_sector);
    printf("Root begin cluster: %d\n", info->root_cid);
}

// return the end offset of this entry
// TODO: add lfn checksum check
static int find_first_entry(void *buffer, uint32 size, struct sfn_entry *entry, char *name)
{
    int len = 32;
    struct sfn_entry *entries = (struct sfn_entry *)buffer;
    if (len > size)
    {
        return FAT32_OVER_SIZE;
    }
    // if (*((uint8 *)entries) == FAT32_E_FREE)
    // {
    //     printf("can't find a file\n");
    //     return 0;
    // }
    while (entries->type == FAT32_T_LFN || entries->type == FAT32_T_FREE)
    {
        entries++;
        len += 32;
        if (len > size)
        {
            return FAT32_OVER_SIZE;
        }
        // if (*((uint8 *)entries) == FAT32_E_FREE)
        // {
        //     printf("can't find a file\n");
        //     return 0;
        // }
    }
    *entry = *entries;
    entries--;
    struct lfn_entry *lentries = (struct lfn_entry *)entries;
    if (entries->type != FAT32_T_LFN || lentries->id == FAT32_E_DEL)
    {
        memcpy(name, entry, 8);
        name += 8;
    }
    else
    {
        while ((lentries->id & FAT32_E_LFN_END) == 0)
        {
            lfn2name(lentries, name);
            name += 13;
            lentries--;
        }
        lfn2name(lentries, name);
        name += 13;
    }
    name = '\0';
    return len;
}

// the unit of offset is sector, not byte
static int get_cluster_offset(superblock_t *sb, uint32 cid)
{
    return ((fat32_info_t *)sb->extra)->root_offset + (cid - 2) * ((fat32_info_t *)sb->extra)->sectors_per_cluster;
}

static int get_next_cid(superblock_t *sb, uint32 cid)
{
    return ((fat32_info_t *)sb->extra)->fat[cid];
}

// return index in dir, -1 is error
static int lookup_entry(superblock_t *sb, uint32 cid, char *filename, struct sfn_entry *entry)
{
    int cluster_size = sb->block_size;
    int p_num = 1;
    char *mem = (char*)palloc_n(p_num);
    char *name_mem = palloc();
    char *name = name_mem;
    char *buffer = mem;
    int size = cluster_size;
    int max_size = PG_SIZE * p_num;
    //printf("lookup_entry: %d %d %d\n",cid, get_cluster_offset(sb, cid), sb->block_size / BSIZE);
    read_to_buffer(sb->dev, get_cluster_offset(sb, cid) * ((fat32_info_t *)sb->extra)->blocks_per_sector,
     cluster_size / BSIZE, buffer);

    int offset = 0, tmp;
    while ((tmp = find_first_entry(buffer + offset, size - offset, entry, name)))
    {
        if (tmp == FAT32_OVER_SIZE)
        {
            cid = get_next_cid(sb, cid);
            if (FAT32_CID_IS_VALID(cid))
            {
                if (size >= max_size)
                {
                    // printf("fat32: dir size is too large");
                    // goto lookup_entry_error;
                    // to larger buffer
                    char* tmp = mem;
                    p_num*=2;
                    mem = (char*)palloc_n(p_num);
                    memcpy(mem, buffer, max_size);
                    pfree(tmp);
                    max_size = PG_SIZE * p_num;
                    buffer = mem;
                }
                read_to_buffer(sb->dev, get_cluster_offset(sb, cid) * ((fat32_info_t *)sb->extra)->blocks_per_sector,
                 cluster_size / BSIZE, buffer + size);
                size += cluster_size;
                continue;
            }
            else
            {
                printf("fat32: can't find file\n");
                goto lookup_entry_error;
            }
        }
        offset += tmp;
        if (strcmp(name, filename) == 0)
        {
            pfree(mem);
            pfree(name_mem);
            return (offset / 32) - 1;
        }
    }

lookup_entry_error:
    pfree(mem);
    pfree(name_mem);
    return -1;
}

static void sfn_entry2inode(superblock_t *sb, struct sfn_entry *entry,inode_t *parent,int index, inode_t *node)
{
    node->dev = sb->dev;
    node->sb = sb;
    node->id = FAT32_E_CID(entry);
    node->size = entry->size;
    node->refcnt = 0;
    node->valid = 1;
    node->nlink = 1;
    node->parent = parent;
    if(parent!=NULL) pin_inode(parent);
    node->index_in_parent = index;
    node->major = -1;
    switch(entry->type) {
        case FAT32_T_FILE:
            node->type = T_FILE;
            break;
        case FAT32_T_DIR:
            node->type = T_DIR;
            break;
        case FAT32_T_DEV:
            node->type = T_DEVICE;
            node->major = entry->major;
            break;
        default:
            node->type = T_FILE;
            printf("warning: unknown file type\n");
            break;
    }
}

//-1 is error
static int get_free_cluster(superblock_t *sb)
{
    if(sb->fs_type != FS_TYPE_FAT32)
    {
        printf("fat32: not a fat32 filesystem\n");
        return -1;
    }
    fat32_info_t *info = (fat32_info_t *)sb->extra;
    for (int i = 2; i < info->fat_items; ++i)
    {
        if (info->fat[i] == 0)
        {
            return i;
        }
    }
    return -1;
}

static void fat32_update_inode(inode_t *node)
{
    superblock_t *sb = node->sb;
    int cluster_size = sb->block_size;
    if (sb->fs_type != FS_TYPE_FAT32)
    {
        printf("fat32: not a fat32 filesystem\n");
        return;
    }
    char *mem = palloc();
    char *buffer = mem;
    int index = node->index_in_parent;
    int cid = node->parent->id;
    int max_cnt = cluster_size / sizeof(struct sfn_entry);
    while(index>=max_cnt) {
        index -= max_cnt;
        cid = get_next_cid(sb, cid);
        if(FAT32_CID_IS_VALID(cid)==0) {
            printf("fat32: index is invalid\n");
            goto fat32_update_inode_end;
        }
    }
    read_to_buffer(sb->dev, get_cluster_offset(sb, cid) * ((fat32_info_t *)sb->extra)->blocks_per_sector,
     cluster_size / BSIZE, buffer);
    struct sfn_entry *entries = (struct sfn_entry *)buffer;
    entries += index;
    entries->size = node->size;
    write_to_disk(sb->dev, get_cluster_offset(sb, cid) * ((fat32_info_t *)sb->extra)->blocks_per_sector,
     cluster_size / BSIZE, buffer);

fat32_update_inode_end:
    pfree(mem);
}

static int fat32_create_inode(inode_t* dir, char* filename, uint8 type, uint8 major, inode_t* node)
{
    superblock_t *sb = dir->sb;
    int cluster_size = sb->block_size;
    if (sb->fs_type != FS_TYPE_FAT32)
    {
        printf("fat32: not a fat32 filesystem\n");
        return 0;
    }
    if (dir->type != T_DIR)
    {
        printf("fat32: not a directory\n");
        return 0;
    }
    struct sfn_entry tmp;
    if (lookup_entry(sb, dir->id, filename, &tmp)!=-1)
    {
        printf("fat32: file exists\n");
        return 0;
    }

    // get free index, if dir is too small to add the file, will add cluster
    int cid = dir->id;
    char *mem = palloc();
    char *buffer = mem;
    struct sfn_entry* entry;
    int max_cnt = cluster_size / sizeof(struct sfn_entry);
    int name_len = strlen(filename);
    int need_cnt = 1+ceil_div(name_len,13);  //every lfn contains no more than 13 characters 
    int index = 0;  
    // -2 because dir . and ..
    if(need_cnt>max_cnt-2) {
        printf("fat32: file name is too long\n");
        goto fat32_create_inode_error;
    }
    while(1) {
        read_to_buffer(sb->dev, get_cluster_offset(sb, cid) * ((fat32_info_t *)sb->extra)->blocks_per_sector,
         cluster_size / BSIZE, buffer);  
        entry = (struct sfn_entry*)buffer;
        int flag = 0;
        for(int i=0;i<max_cnt;) {
            if(entry->type == FAT32_T_FREE||entry->type == 0) {
                int j = i;
                while(j<max_cnt && (entry->type == FAT32_T_FREE||entry->type == 0) && j-i<need_cnt-1) {
                    j++;
                    index++;
                    entry++;
                }
                if(j-i>=need_cnt-1) {
                    flag = 1;
                    break;
                }
                i = j;
            } else {
                i++;
                entry++;
                index++;
            }
        }
        if(flag == 1) {
            break;
        } else {
            int ori_cid = cid;
            cid = get_next_cid(sb, cid);
            if(cid==FAT32_END_CID) {
                cid = get_free_cluster(sb);
                if(cid==-1) {
                    printf("fat32: no free cluster\n");
                    goto fat32_create_inode_error;
                }
                set_fat(sb, ori_cid, cid);
                set_fat(sb, cid, FAT32_END_CID);
                fresh_fat(sb);
                dir->size += cluster_size;
                fat32_update_inode(dir);
            }
        }
    }

    // write sfn entry
    entry-=(need_cnt-1);
    memset((void*)entry,0,sizeof(struct sfn_entry)*need_cnt);
    entry+=(need_cnt-1);
    int len = name_len < 8 ? name_len : 8;
    for (int i = 0; i < len; ++i)
    {
        entry->name[i] = filename[i];
    }
    for (int i = 0; i < 3; ++i)
    {
        entry->ex_name[i] = ' ';
    }    
    int new_cid = get_free_cluster(sb);
    if(new_cid==-1) {
        printf("fat32: no free cluster\n");
        goto fat32_create_inode_error;
    }
    set_fat(sb, new_cid, FAT32_END_CID);
    entry->high_cid = new_cid >> 16;
    entry->low_cid = new_cid & 0xffff;
    entry->size = 0;
    if(type==T_DEVICE) {
        entry->type = FAT32_T_DEV;
        entry->major = major;
    } else if(type==T_DIR) {
        entry->type = FAT32_T_DIR;
        struct sfn_entry *c_entry = (struct sfn_entry *)new_dir_date;
        c_entry[0].high_cid = new_cid >> 16;
        c_entry[0].low_cid = new_cid & 0xffff;
        c_entry[1].high_cid = (dir->id) >> 16;
        c_entry[1].low_cid = (dir->id) & 0xffff;
        // please promise BSIZE >= 64bytes
        write_to_disk(sb->dev, get_cluster_offset(sb, new_cid) * ((fat32_info_t *)sb->extra)->blocks_per_sector,
         BSIZE, new_dir_date);
    } else {
        entry->type = FAT32_T_FILE;
    }
    sfn_entry2inode(sb, entry, dir, index, node);

    // write lfn entries
    uint8 checksum = lfn_checksum((uint8 *)entry);
    int lfn_id = 1;
    while(name_len>0) {
        entry--;
        name2lfn(filename, (struct lfn_entry*)entry, checksum);
        name_len -= 13;
        filename += 13;
        if(name_len<=0) lfn_id|=0x40;
        ((struct lfn_entry*)entry)->id = lfn_id;
        lfn_id++;
    }

    // write to disk
    write_to_disk(sb->dev, get_cluster_offset(sb, cid) * ((fat32_info_t *)sb->extra)->blocks_per_sector,
     cluster_size / BSIZE, buffer);
    // no need to fat32_update_inode(node), because write_to_disk is ok
    fresh_fat(sb);

    pfree(mem);
    return 1;

fat32_create_inode_error:
    pfree(mem);
    return 0;
}

int fat32_test()
{
    superblock_t sb;
    fat32_superblock_init(VIRTIO_DISK_DEV, &sb);
    int cluster_size = sb.block_size;
    struct sfn_entry root, entry;
    char name[256];
    char buffer[MAX_CLUSTER_SIZE];
    root.high_cid = 0;
    root.low_cid = ((fat32_info_t *)sb.extra)->root_cid;
    read_to_buffer(sb.dev, get_cluster_offset(&sb, FAT32_E_CID(&root))*((fat32_info_t *)sb.extra)->blocks_per_sector, 
     cluster_size / BSIZE, buffer);
    int offset = 0, tmp;
    printf("fat32: test list root\n");
    while ((tmp = find_first_entry(buffer + offset, cluster_size - offset, &entry, name)))
    {
        print_sfn_entry(&entry);
        printf("%s\n", name);
        printf("\n");
        offset += tmp;
    }

    printf("fat32: find u\n");
    lookup_entry(&sb, FAT32_E_CID(&root), "bin", &entry);
    print_sfn_entry(&entry);
    lookup_entry(&sb, FAT32_E_CID(&entry), "u", &entry);
    print_sfn_entry(&entry);
    return 1;
}