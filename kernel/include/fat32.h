#ifndef __FAT32__H__
#define __FAT32__H__

#include "file.h"

#define DBS_SIZE 512
#define CLUSTER_SIZE 512*4

#define FAT32_T_READONLY 0x01
#define FAT32_T_HIDDEN  0x02
#define FAT32_T_SYSTEM  0x04
#define FAT32_T_VOLUME  0x08
#define FAT32_T_DIR  0x10
#define FAT32_T_ARCHIVE  0x20
#define FAT32_T_DEV 0x40
#define FAT32_T_FREE 0x80
#define FAT32_T_LFN 0x0f

#define FAT32_E_FREE 0x00
#define FAT32_E_LFN_END 0x40
#define FAT32_E_SPECIAL 0x2e
#define FAT32_E_DEL 0xe5

#define FAT32_E_CID(entry) ((entry)->high_cid<<16|(entry)->low_cid)

#define FAT32_LFN_LEN 13

void fat32_superblock_init(uint32 dev, struct superblock *sb);
int fat32_test();

#endif