#include "file.h"
#include "bio.h"
#include "types.h"
#include "pmm.h"
#include "print.h"
#include "dagaslib.h"
#include "fat32.h"

#pragma pack(1)
struct sfn_entry
{
    char name[8];
    char ex_name[3];
    uint8 type;
    uint8 r1;
    uint8 tms;      // 10ms
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

static void utf16_to_ascii(const uint16 *utf16_str, char *ascii_str, uint32 len);
static void lfn_cpy(const struct lfn_entry *lfn, char *name);
static void print_sfn_entry(struct sfn_entry *entry);
// static void print_lfn_entry(struct lfn_entry *entry);
static int fat32_lookup_inode(inode_t *dir, char *filename, inode_t *node);
static int fat32_read_inode(inode_t *node, int offset, int size, void *buffer);
// static void print_fat_info(fat32_info_t *info);
static int find_first_entry(void *buffer, uint32 size, struct sfn_entry *entry, char *name);
static int get_cluster_offset(superblock_t *sb, uint32 cid);
static int get_next_cid(superblock_t *sb, uint32 cid);
static int lookup_entry(superblock_t *sb, uint32 cid, char *filename, struct sfn_entry *entry);
static void sfn_entry2inode(superblock_t *sb, struct sfn_entry *entry, inode_t *node);

static void utf16_to_ascii(const uint16 *utf16_str, char *ascii_str, uint32 len)
{
    for (int i = 0; i < len; ++i)
    {
        *ascii_str++ = (char)(*utf16_str++);
    }
    *ascii_str = '\0';
}

static void lfn_cpy(const struct lfn_entry *lfn, char *name)
{
    utf16_to_ascii(lfn->name1, name, 5);
    utf16_to_ascii(lfn->name2, name + 5, 6);
    utf16_to_ascii(lfn->name3, name + 11, 2);
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
//     lfn_cpy(entry, name);
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
    if (lookup_entry(sb, dir->id, filename, &entry))
    {
        sfn_entry2inode(sb, &entry, node);
        return 1;
    }
    return 0;
}

static int fat32_read_inode(inode_t *node, int offset, int size, void *buffer)
{
    superblock_t *sb = node->sb;
    int real_size = size;
    //printf("fat32_read_inode: begin read file\n");
    if (sb->fs_type != FS_TYPE_FAT32)
    {
        printf("fat32: not a fat32 filesystem\n");
        return 0;
    }
    if (offset < 0 || offset > node->size)
    {
        printf("fat32: offset is invalid\n");
        return 0;
    }
    if (offset + size > node->size)
    {
        real_size = size = (node->size - offset);
    }
    uint32 cid = node->id;
    while (offset >= CLUSTER_SIZE)
    {
        cid = get_next_cid(sb, cid);
        if (!FAT32_CID_IS_VALID(cid))
        {
            printf("fat32: cid is invalid\n");
            return 0;
        }
        offset -= CLUSTER_SIZE;
        
        //printf("fat32_read_inode: cid%d\n",cid);
    }
    //printf("fat32_read_inode:cid%d off%d %d \n",cid,offset, size);
    while (size > 0)
    {
        if (!FAT32_CID_IS_VALID(cid))
        {
            printf("fat32: cid is invalid\n");
            return 0;
        }
        if (size >= CLUSTER_SIZE - offset)
        {
            read_bytes_to_buffer(sb->dev, get_cluster_offset(sb, cid), offset, CLUSTER_SIZE - offset, buffer);
            buffer += CLUSTER_SIZE - offset;
            size -= CLUSTER_SIZE - offset;
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
    if (info->sectors_per_fat * info->bytes_per_sector > PG_SIZE)
    {
        panic("fat32: fat size is larger than one page size");
    }
    info->fat = (uint32 *)palloc();
    
    printf("fat32: info->fat%x\n",info->fat);
    read_to_buffer(dev, info->fat_offset * info->blocks_per_sector, info->sectors_per_fat * info->blocks_per_sector, info->fat);

    sb->block_size = info->sectors_per_cluster * info->bytes_per_sector;

    // init ops
    sb->lookup_inode = fat32_lookup_inode;
    sb->read_inode = fat32_read_inode;
}

// static void print_fat_info(fat32_info_t *info)
// {
//     printf("Bytes per sector: %d\n", info->bytes_per_sector);
//     printf("Sectors per cluster: %d\n", info->sectors_per_cluster);
//     printf("Reserved sector count: %d\n", info->reserved_sectors);
//     printf("Number of FATs: %d\n", info->fat_cnt);
//     printf("Total sectors: %d\n", info->total_sectors);
//     printf("Sectors per FAT: %d\n", info->sectors_per_fat);
//     printf("Root begin cluster: %d\n", info->root_cid);
// }

// return the end offset of this entry
static int find_first_entry(void *buffer, uint32 size, struct sfn_entry *entry, char *name)
{
    int len = 32;
    struct sfn_entry *entries = (struct sfn_entry *)buffer;
    if (len > size)
    {
        return FAT32_OVER_SIZE;
    }
    if (*((uint8 *)entries) == FAT32_E_FREE)
    {
        printf("can't find a file\n");
        return 0;
    }
    while (entries->type == FAT32_T_LFN || entries->type == FAT32_T_FREE)
    {
        entries++;
        len += 32;
        if (len > size)
        {
            return FAT32_OVER_SIZE;
        }
        if (*((uint8 *)entries) == FAT32_E_FREE)
        {
            printf("can't find a file\n");
            return 0;
        }
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
            lfn_cpy(lentries, name);
            name += 13;
            lentries--;
        }
        lfn_cpy(lentries, name);
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

static int lookup_entry(superblock_t *sb, uint32 cid, char *filename, struct sfn_entry *entry)
{
    char name[256];
    char buffer[CLUSTER_SIZE * 2];
    int size = CLUSTER_SIZE;
    //printf("lookup_entry: %d %d %d\n",cid, get_cluster_offset(sb, cid), sb->block_size / BSIZE);
    read_to_buffer(sb->dev, get_cluster_offset(sb, cid), sb->block_size / BSIZE, buffer);

    int offset = 0, tmp;
    while ((tmp = find_first_entry(buffer + offset, size - offset, entry, name)))
    {
        if (tmp == FAT32_OVER_SIZE)
        {
            cid = get_next_cid(sb, cid);
            if (FAT32_CID_IS_VALID(cid))
            {
                if (size >= CLUSTER_SIZE * 2)
                {
                    printf("fat32: dir size is too large");
                    return 0;
                }
                read_to_buffer(sb->dev, get_cluster_offset(sb, cid), sb->block_size / BSIZE, buffer + size);
                size += CLUSTER_SIZE;
                continue;
            }
            else
            {
                printf("fat32: can't find file\n");
                return 0;
            }
        }
        if (strcmp(name, filename) == 0)
        {
            return 1;
        }
        offset += tmp;
    }
    return 0;
}

static void sfn_entry2inode(superblock_t *sb, struct sfn_entry *entry, inode_t *node)
{
    node->dev = sb->dev;
    node->sb = sb;
    node->id = FAT32_E_CID(entry);
    node->type = entry->type == FAT32_T_DIR ? T_DIR : T_FILE;
    node->size = entry->size;
    node->refcnt = 0;
    node->valid = 1;
    node->nlink = 1;
}

int fat32_test()
{
    superblock_t sb;
    fat32_superblock_init(VIRTIO_DISK_DEV, &sb);
    struct sfn_entry root, entry;
    char name[256];
    char buffer[CLUSTER_SIZE];
    root.high_cid = 0;
    root.low_cid = ((fat32_info_t *)sb.extra)->root_cid;
    read_to_buffer(sb.dev, get_cluster_offset(&sb, FAT32_E_CID(&root)), sb.block_size / BSIZE, buffer);
    int offset = 0, tmp;
    printf("fat32: test list root\n");
    while ((tmp = find_first_entry(buffer + offset, CLUSTER_SIZE - offset, &entry, name)))
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