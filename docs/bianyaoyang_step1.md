# 初步环境

第一步是至少能跑起来。

学习https://github.com/mit-pdos/xv6-riscv/

## QEMU参数

QEMUOPTS = -machine virt -bios none -kernel $K/kernel -m 128M -smp $(CPUS) -nographic
QEMUOPTS += -global virtio-mmio.force-legacy=false
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

## 必须文件

### virtio_disk.c

提供虚拟磁盘的IO功能

### kernel.ld user.ld

链接脚本

kernel.ld 保证entry.S / _entry 在 0x80000000 * where qemu's -kernel jumps.

### mkfs.c 

创建 fs.img 磁盘镜像

### entry.S

系统启动地址
