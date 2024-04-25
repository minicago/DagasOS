//
// driver for qemu's virtio disk device.
// uses qemu's mmio interface to virtio.
//
// qemu ... -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
//

#include "types.h"
#include "virtio.h"
#include "memory_layout.h"
#include "pmm.h"
#include "fs.h"
#include "virtio_disk.h"
#include "spinlock.h"
#include "strap.h"
#include "waitqueue.h"

// the address of virtio mmio register r.
#define R(r) ((volatile uint32 *)(VIRTIO0 + (r)))

static struct disk
{
    // a set (not a ring) of DMA descriptors, with which the
    // driver tells the device where to read and write individual
    // disk operations. there are NUM descriptors.
    // most commands consist of a "chain" (a linked list) of a couple of
    // these descriptors.
    char pages[2 * PG_SIZE];
    struct virtq_desc *desc;

    // a ring in which the driver writes descriptor numbers
    // that the driver would like the device to process.  it only
    // includes the head descriptor of each chain. the ring has
    // NUM elements.
    struct virtq_avail *avail;

    // a ring in which the device writes descriptor numbers that
    // the device has finished processing (just the head of each chain).
    // there are NUM used ring entries.
    struct virtq_used *used;

    // our own book-keeping.
    char free[NUM];  // is a descriptor free?
    uint16 used_idx; // we've looked this far in used[2..NUM].

    // track info about in-flight operations,
    // for use when completion interrupt arrives.
    // indexed by first descriptor index of chain.
    struct
    {
        struct buf *b;
        char status;
    } info[NUM];

    // disk command headers.
    // one-for-one with descriptors, for convenience.
    struct virtio_blk_req ops[NUM];

    spinlock_t lock;

} __attribute__((aligned(PG_SIZE))) disk;

void virtio_disk_init(void)
{
    init_spinlock(&disk.lock);
    
    uint32 status = 0;

    if (*R(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
        *R(VIRTIO_MMIO_VERSION) != 1 ||
        *R(VIRTIO_MMIO_DEVICE_ID) != 2 ||
        *R(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551)
    {
        //printf("virtio_disk_init: could not find virtio disk");
        panic("virtio_disk_init: could not find virtio disk");
    }

  status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
  *R(VIRTIO_MMIO_STATUS) = status;

  status |= VIRTIO_CONFIG_S_DRIVER;
  *R(VIRTIO_MMIO_STATUS) = status;

  // negotiate features
  uint64 features = *R(VIRTIO_MMIO_DEVICE_FEATURES);
  features &= ~(1 << VIRTIO_BLK_F_RO);
  features &= ~(1 << VIRTIO_BLK_F_SCSI);
  features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
  features &= ~(1 << VIRTIO_BLK_F_MQ);
  features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
  features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
  features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
  *R(VIRTIO_MMIO_DRIVER_FEATURES) = features;

    // tell device that feature negotiation is complete.
  status |= VIRTIO_CONFIG_S_FEATURES_OK;
  *R(VIRTIO_MMIO_STATUS) = status;

   // tell device we're completely ready.
  status |= VIRTIO_CONFIG_S_DRIVER_OK;
  *R(VIRTIO_MMIO_STATUS) = status;

  *R(VIRTIO_MMIO_GUEST_PAGE_SIZE) = PG_SIZE;

  // initialize queue 0.
  *R(VIRTIO_MMIO_QUEUE_SEL) = 0;
  uint32 max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (max == 0)
        panic("virtio_disk_init: virtio disk has no queue 0");
    if (max < NUM)
        panic("virtio_disk_init: virtio disk max queue too short");

    // allocate and zero queue memory.
    disk.desc = (struct virtq_desc *)disk.pages;
    disk.avail = (struct virtq_avail *)(((char *)disk.desc) + NUM * sizeof(struct virtq_desc));
    disk.used = (struct virtq_used *)(disk.pages + PG_SIZE);
    if (!disk.desc || !disk.avail || !disk.used)
        panic("virtio_disk_init: virtio disk kalloc");

    // set queue size.
    *R(VIRTIO_MMIO_QUEUE_NUM) = NUM;
    memset(disk.pages, 0, sizeof(disk.pages));
    *R(VIRTIO_MMIO_QUEUE_PFN) = ((uint64)disk.pages) >> 12;

    // // write physical addresses.
    // *R(VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint64)disk.desc;
    // *R(VIRTIO_MMIO_QUEUE_DESC_HIGH) = (uint64)disk.desc >> 32;
    // *R(VIRTIO_MMIO_DRIVER_DESC_LOW) = (uint64)disk.avail;
    // *R(VIRTIO_MMIO_DRIVER_DESC_HIGH) = (uint64)disk.avail >> 32;
    // *R(VIRTIO_MMIO_DEVICE_DESC_LOW) = (uint64)disk.used;
    // *R(VIRTIO_MMIO_DEVICE_DESC_HIGH) = (uint64)disk.used >> 32;

    // queue is ready.
    // *R(VIRTIO_MMIO_QUEUE_READY) = 0x1;

    // all NUM descriptors start out unused.
    for (int i = 0; i < NUM; i++)
        disk.free[i] = 1;

    // // tell device we're completely ready.
    // status |= VIRTIO_CONFIG_S_DRIVER_OK;
    // *R(VIRTIO_MMIO_STATUS) = status;

    // plic.c and trap.c arrange for interrupts from VIRTIO0_IRQ.
}

// find a free descriptor, mark it non-free, return its index.
static int alloc_desc()
{
    for (int i = 0; i < NUM; i++)
    {
        if (disk.free[i])
        {
            disk.free[i] = 0;
            return i;
        }
    }
    return -1;
}

// mark a descriptor as free.
static void free_desc(int i)
{
    if (i >= NUM)
        panic("free_desc: index out of range");
    if (disk.free[i])
        panic("free_desc: double free");
    disk.desc[i].addr = 0;
    disk.desc[i].len = 0;
    disk.desc[i].flags = 0;
    disk.desc[i].next = 0;
    disk.free[i] = 1;
    // wakeup(&disk.free[0]);
}

// free a chain of descriptors.
static void free_chain(int i)
{
    while (1)
    {
        int flag = disk.desc[i].flags;
        int nxt = disk.desc[i].next;
        free_desc(i);
        if (flag & VRING_DESC_F_NEXT)
            i = nxt;
        else
            break;
    }
}

// allocate three descriptors (they need not be contiguous).
// disk transfers always use three descriptors.
static int alloc3_desc(int *idx)
{
    for (int i = 0; i < 3; i++)
    {
        idx[i] = alloc_desc();
        if (idx[i] < 0)
        {
            for (int j = 0; j < i; j++)
                free_desc(idx[j]);
            return -1;
        }
    }
    return 0;
}

void virtio_disk_rw(struct buf *b, int write)
{
    //printf("virtio_rw: %d %d\n", b->block_id, write);
    uint64 sector = b->block_id * (BSIZE / 512);
    
    acquire_spinlock(&disk.lock);
    
    // the spec's Section 5.2 says that legacy block operations use
    // three descriptors: one for type/reserved/sector, one for the
    // data, one for a 1-byte status result.
    // allocate the three descriptors.
    int idx[3];
    while (1)
    {
        if (alloc3_desc(idx) == 0)
        {
            break;
        }
        // sleep(&disk.free[0], &disk.vdisk_lock);
    }

    // format the three descriptors.
    // qemu's virtio-blk.c reads them.

    struct virtio_blk_req *buf0 = &disk.ops[idx[0]];

    if (write)
        buf0->type = VIRTIO_BLK_T_OUT; // write the disk
    else
        buf0->type = VIRTIO_BLK_T_IN; // read the disk
    buf0->reserved = 0;
    buf0->sector = sector;

    disk.desc[idx[0]].addr = (uint64)buf0;
    disk.desc[idx[0]].len = sizeof(struct virtio_blk_req);
    disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
    disk.desc[idx[0]].next = idx[1];

    disk.desc[idx[1]].addr = (uint64)b->data;
    disk.desc[idx[1]].len = BSIZE;
    if (write)
        disk.desc[idx[1]].flags = VRING_DESC_F_READ; // device reads b->data
    else
        disk.desc[idx[1]].flags = VRING_DESC_F_WRITE; // device writes b->data
    disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
    disk.desc[idx[1]].next = idx[2];

    disk.info[idx[0]].status = 0; // device writes 0 on success
    disk.desc[idx[2]].addr = (uint64)&disk.info[idx[0]].status;
    disk.desc[idx[2]].len = 1;
    disk.desc[idx[2]].flags = VRING_DESC_F_WRITE; // device writes the status
    disk.desc[idx[2]].next = 0;

    // record struct buf for virtio_disk_intr().
    b->disk = 1;
    disk.info[idx[0]].b = b;

    // tell the device the first index in our chain of descriptors.
    disk.avail->ring[disk.avail->idx % NUM] = idx[0];

    __sync_synchronize();

    // tell the device another avail ring entry is available.
    disk.avail->idx += 1; // not % NUM ...

    __sync_synchronize();

    //printf("virtio_rw: test_disk wait%p\n", b);
    // Wait for virtio_disk_intr() to say request has finished.
    
    release_spinlock(&disk.lock);

    //TODO: after sleep thread is ok, must delete intr_on and intr_off this.
    // this just for get int in user syscall without sleep.
    int int_on = 0;
    
    if(get_tid()==-1) {
        // intr_pop();
        if(!intr_get()) {
            // intr_print();
            // panic("virtio_disk_rw: not in interrupt");
            intr_on();
            int_on = 1;
        }
    }

    uint64 result;
    if(get_tid() != -1) {
        thread_pool[get_tid()].waiting = b->wait_queue;
        thread_pool[get_tid()].state = T_SLEEPING;
        wait_queue_push_back(b->wait_queue, &thread_pool[get_tid()], &result);
        LOG("virtio_disk: pre to sched %p\n", b->wait_queue->left);
        
    *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value is queue number
        sched();
    } else {
    *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value is queue number
    }
    
    // *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value is queue number
    while(b->disk==1) {
    /*
    *******************************
    *program dead here            *
    *******************************
    
    */        
        
        continue;
    //     TODO: sleep
    //     sleep(b, &disk.vdisk_lock);
    }
    if(get_tid()==-1) {
        if(int_on) {
            intr_off();
        }
    }

    // int cnt = 0;
    // int max_cnt = 400;
    // while(disk.info[idx[0]].status != 0 && cnt++<max_cnt)
    //     for(int i=1;i<=10000;i++) ;
    // printf("virtio_rw: irq%d\n",plic_claim());
    // printf("virtio_disk_rw: request%d status is %d\n", disk.avail->idx - 1, disk.info[idx[0]].status);
    acquire_spinlock(&disk.lock);
    if(disk.info[idx[0]].status != 0){
        panic("virtio_disk_rw: request failed");
    }
    // wakeup(b);

    disk.info[idx[0]].b = 0;
    free_chain(idx[0]);

    release_spinlock(&disk.lock);
}

void virtio_disk_intr()
{
    
    acquire_spinlock(&disk.lock);

    // the device won't raise another interrupt until we tell it
    // we've seen this interrupt, which the following line does.
    // this may race with the device writing new entries to
    // the "used" ring, in which case we may process the new
    // completion entries in this interrupt, and have nothing to do
    // in the next interrupt, which is harmless.
    *R(VIRTIO_MMIO_INTERRUPT_ACK) = *R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;

    __sync_synchronize();
    // the device increments disk.used->idx when it
    // adds an entry to the used ring.
    //printf("virtio_disk_intr: disk.used_idx %d, disk.used->idx %d\n", disk.used_idx, disk.used->idx);
    while (disk.used_idx != disk.used->idx)
    {
        __sync_synchronize();
        int id = disk.used->ring[disk.used_idx % NUM].id;

        if (disk.info[id].status != 0)
            panic("virtio_disk_intr: virtio_disk_intr status");
        //printf("virtio_disk_intr: finish %p disk-%d\n", disk.info[id].b,disk.info[id].b->block_id);
        struct buf *b = disk.info[id].b;
        b->disk = 0; // disk is done with buf
        // wakeup(b);

        disk.used_idx += 1;
        
    printf("virtio_disk_intr: go in %p\n", b->wait_queue->left);
        awake_wait_queue(b->wait_queue, 0);
    }
    release_spinlock(&disk.lock);
}
