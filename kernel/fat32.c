#include "file.h"
#include "bio.h"
#include "types.h"
#include "pmm.h"
#include "print.h"
#include "dagaslib.h"
#include "fat32.h"

struct fat32_info
{
    uint32 bytes_per_sector;
    uint32 sectors_per_cluster;
    uint32 reserved_sectors;
    uint32 hidden_sectors;
    uint32 fat_cnt;
    uint32 total_sectors;
    uint32 sectors_per_fat;
    uint32 root_cid;   // cluster id of root directory
    uint32 fat_offset; // offset of fat in sectors
    uint32 root_offset;
    uint32 blocks_per_sector;
    uint32 *fat;
};

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

void utf16_to_ascii(const uint16 *utf16_str, char *ascii_str, uint32 len)
{
    for (int i = 0; i < len; ++i)
    {
        *ascii_str++ = (char)(*utf16_str++);
    }
    *ascii_str = '\0';
}

void lfn_cpy(const struct lfn_entry *lfn, char *name)
{
    utf16_to_ascii(lfn->name1, name, 5);
    utf16_to_ascii(lfn->name2, name + 5, 6);
    utf16_to_ascii(lfn->name3, name + 11, 2);
}

void print_sfn_entry(struct sfn_entry *entry)
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

void print_lfn_entry(struct lfn_entry *entry)
{
    printf("ID: %02x\n", entry->id);
    printf("Type: %02x\n", entry->type);
    printf("Check: %02x\n", entry->check);
    char name[14];
    lfn_cpy(entry, name);
    name[13] = '\0';
    printf("Name: %s\n", name);
}

void fat32_superblock_init(uint32 dev, struct superblock *sb)
{
    // read boot sector
    struct buf *b = read_block(dev, 0);

    sb->extra = palloc();
    sb->dev = dev;

    struct fat32_info *info = (struct fat32_info *)sb->extra;
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
    read_to_buffer(dev, info->fat_offset * info->blocks_per_sector, info->sectors_per_fat * info->blocks_per_sector, info->fat);

    sb->block_size = info->sectors_per_cluster * info->bytes_per_sector;

    // init ops
}

void print_fat_info(struct fat32_info *info)
{
    printf("Bytes per sector: %d\n", info->bytes_per_sector);
    printf("Sectors per cluster: %d\n", info->sectors_per_cluster);
    printf("Reserved sector count: %d\n", info->reserved_sectors);
    printf("Number of FATs: %d\n", info->fat_cnt);
    printf("Total sectors: %d\n", info->total_sectors);
    printf("Sectors per FAT: %d\n", info->sectors_per_fat);
    printf("Root begin cluster: %d\n", info->root_cid);
}

// return the end offset of this entry
int find_first_entry(void *buffer, uint32 size, struct sfn_entry *entry, char *name)
{
    int len = 32;
    struct sfn_entry *entries = (struct sfn_entry *)buffer;
    if (len > size || *((uint8 *)entries) == FAT32_E_FREE)
    {
        printf("can't find a file\n");
        return 0;
    }
    while (entries->type == FAT32_T_LFN || entries->type == FAT32_T_FREE)
    {
        entries++;
        len += 32;
        if (len > size || *((uint8 *)entries) == FAT32_E_FREE)
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

int get_cluster_offset(struct superblock *sb, uint32 cid)
{
    return ((struct fat32_info *)sb->extra)->root_offset + (cid - 2) * ((struct fat32_info *)sb->extra)->sectors_per_cluster;
}

int lookup_entry(struct superblock *sb, struct sfn_entry *dir, char *filename, struct sfn_entry *entry)
{
    char name[256];
    char buffer[CLUSTER_SIZE];
    read_to_buffer(sb->dev, get_cluster_offset(sb, FAT32_E_CID(dir)), sb->block_size / BSIZE, buffer);
    int offset = 0, tmp;
    while ((tmp = find_first_entry(buffer + offset, CLUSTER_SIZE - offset, entry, name)))
    {
        if (strcmp(name, filename) == 0)
        {
            return 1;
        }
        offset += tmp;
    }
    return 0;
}

int fat32_test()
{
    struct superblock sb;
    fat32_superblock_init(VIRTIO_DISK_DEV, &sb);
    struct sfn_entry root, entry;
    char name[256];
    char buffer[CLUSTER_SIZE];
    root.high_cid = 0;
    root.low_cid = ((struct fat32_info *)sb.extra)->root_cid;
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
    return 1;
}