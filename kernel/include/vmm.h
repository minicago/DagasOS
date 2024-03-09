#ifndef __VMM__H__
#define __VMM__H__

#define PTE_PNN_MASK 0x1ffffffffffc00ull
#define PTE_PNN_SHIFT 10
#define PTE_RSW_MASK 0x400ull
#define PTE_PAGE_OFFSET_SHIFT 12
#define PTE_PAGE_LEVEL_SHIFT 9
#define PTE_PAGE_LEVEL_MASK 0x1ff
#define PTE_D 0x80 //Dirty
#define PTE_A 0x40 //Accessed
#define PTE_G 0x20 //Global
#define PTE_U 0x10 //User
#define PTE_X 0x8 
#define PTE_W 0x4
#define PTE_R 0x2
#define PTE_V 0x1 //availble

#define PTE_INDEX(va, level) \
    ( (va) >> (PTE_PNN_SHIFT + PTE_PAGE_OFFSET_SHIFT + (level) * PTE_PAGE_LEVEL_SHIFT) ) & PTE_PAGE_LEVEL_MASK

#define PTA_MODE_MASK 0xf000000000000000ull
#define PTA_MODE_NONE 0x0000000000000000ull
#define PTA_MODE_SV39 0x4000000000000000ull
#define PTA_MODE_SV48 0x5000000000000000ull
#define PTA_ASID_MASK 0x0ffff00000000000ull
#define PTA_ASID_OFFSET 44
#define PTA_PNN_MASK  0x00000fffffffffffull
#define PTA_PNN_OFFSET 0

#endif