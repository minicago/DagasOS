// On-disk file system format.
// Both the kernel and user programs use this header file.
#ifndef __FS__H__
#define __FS__H__

#define ROOTINO  1   // root i-number
#define BSIZE 512  // block size
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define NULL_DEV 0
#define VIRTIO_DISK_DEV 1

#endif