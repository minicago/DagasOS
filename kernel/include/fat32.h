#ifndef __FAT32__H__
#define __FAT32__H__

#include "file.h"

#define DBS_SIZE 512

#define FAT32_T_READONLY 0x01
#define FAT32_T_HIDDEN  0x02
#define FAT32_T_SYSTEM  0x04
#define FAT32_T_VOLUME  0x08
#define FAT32_T_DIR  0x10
#define FAT32_T_FILE  0x20
#define FAT32_T_DEV 0x40
#define FAT32_T_FREE 0x80
#define FAT32_T_LFN 0x0f

#define FAT32_E_FREE 0x00
#define FAT32_E_LFN_END 0x40
#define FAT32_E_SPECIAL 0x2e
#define FAT32_E_DEL 0xe5

#define FAT32_E_CID(entry) ((entry)->high_cid<<16|(entry)->low_cid)

#define FAT32_LFN_LEN 13

#define FAT32_OVER_SIZE -1
#define FAT32_OVER_BASE -2

#define FAT32_INVALID_CID_MASK 0x0ffffff0
#define FAT32_FREE_CID 0
#define FAT32_RESERVED_CID 1
#define FAT32_END_CID 0x0ffffff8
#define FAT32_BAD_CID 0x0ffffff7

#define FAT32_CID_IS_VALID(cid) ((((cid) & FAT32_INVALID_CID_MASK) ^ FAT32_INVALID_CID_MASK) \
    && ((cid) != FAT32_FREE_CID) && ((cid) != FAT32_RESERVED_CID))

typedef struct {
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
    uint32 blocks_per_sector; // sector_size / block_size
    uint32 blocks_per_cluster; // cluster_size / block_size
    uint32 fat_items;
    uint32 fat_blocks;
    uint32 *fat;
} fat32_info_t;

int fat32_superblock_init(inode_t *node, superblock_t *parent, superblock_t *sb, uint32 identifier);
int fat32_test();

#endif