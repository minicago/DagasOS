#ifndef __VIRTIO_DISK__H__
#define __VIRTIO_DISK__H__

#include "buf.h"

#define VIRTIO0_IRQ 1

void virtio_disk_init(void);
void virtio_disk_rw(struct buf *, int);
void virtio_disk_intr(void);

#endif