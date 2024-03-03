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

## 学习编写链接脚本

### OUTPUT_ARCH( CPU_ARCH ) 

指定CPU架构为CPU_ARCH

### ENTRY( .entry )

指定入口点为.entry

### SECTION {}

指定段的链接规则



### . = (ADDRESS);

设定段起始位置为ADDRESS

### .name :{} (> REGION (ROM or RAM) )

设置输出的name段内容在加载时放入REGION区域

### . = ALIGN();

设定段对齐长度

### *(.name)

设置文件中输入的name段链接在当前输出段

name可以包含*作为通配符

### MEMORY {}

制定加载内存的规则

### REGION_ALIAS (alias, REGION)

为REGION提供别名

### ASSERT (equals, error_log_text)

进行断言 

### PROVIDE (key = value)

对字段赋值

## QEMU下构建步骤

### 编译内核

riscv64-unknown-elf-gcc -c kernel/entry.S -o kernel/entry.o

riscv64-unknown-elf-ld kernel/entry.o -T kernel/kernel.ld -o kernel/kernel

### 将用户文件写入磁盘

先创建一个空的fs.img磁盘映像

dd if=/dev/zero of=fs.img bs=1M count=256

### 运行QEMU

qemu-system-riscv64 -machine virt -bios none -kernel kernel/kernel -m 128M -smp 1 -nographic -global virtio-mmio.force-legacy=false -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

## 至此已经实现了进入entry入口点的部分