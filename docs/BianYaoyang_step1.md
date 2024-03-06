# 初步环境

第一步是至少能跑起来。

实现一个什么也不做的内核，并且在qemu上能运行和调试。

为了确保内核真实运行，我们令寄存器a0加1，然后使用gdb调试

学习https://github.com/mit-pdos/xv6-riscv/

## Tool Chain Installing

gdb
``` shell
# 安装相关依赖
sudo apt-get install libncurses5-dev python2 python2-dev texinfo libreadline-dev
# 从清华大学开源镜像站下载gdb源码(约23MB)
wget https://mirrors.tuna.tsinghua.edu.cn/gnu/gdb/gdb-13.1.tar.xz
# 解压gdb源码压缩包
tar -xvf gdb-13.1.tar.xz
# 进入gdb源码目录
cd gdb-13.1
mkdir build && cd build
# 配置编译选项，这里只编译riscv64-unknown-elf一个目标文件
../configure --prefix=/usr/local --target=riscv64-unknown-elf --enable-tui=yes
# 在上面一行编译配置选项中，很多其他的文章会配置一个python选项
# 但我在尝试中发现配置了python选项后后面的编译过程会报错，不添加python选项则没有问题

# 开始编译，这里编译可能消耗较长时间，具体时长取决于机器性能
make -j$(nproc)
# 编译完成后进行安装
sudo make install
```

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

``` shell
riscv64-unknown-elf-gcc -c kernel/entry.S -o kernel/entry.o -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie

riscv64-unknown-elf-ld kernel/entry.o -T kernel/kernel.ld -o kernel/kernel -z max-page-size=4096
```

### 将用户文件写入磁盘

先创建一个空的fs.img磁盘映像
``` shell
dd if=/dev/zero of=fs.img bs=1M count=256
```


### 运行QEMU

``` shell
qemu-system-riscv64 -machine virt -bios none -kernel kernel/kernel -m 128M -smp 1 -nographic -global virtio-mmio.force-legacy=false -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
```

-m 参数制定内存大小

-smp 参数制定CPU核心数

** 至此已经实现了进入entry入口点的部分 **

## gdb调试

qemu支持gdb调试，原理是通过tcp将内核信息传递给gdb

在qemu参数中添加 -S -gdb tcp::port 来实现

-S 是让cpu不进行start up等待gdb发送信息

``` shell
qemu-system-riscv64 -machine virt -bios none -kernel kernel/kernel -m 128M -smp 1 -nographic -global virtio-mmio.force-legacy=false -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 -S -gdb tcp::26000
```

Noticing that we may use ``Ctrl+A X`` to exit qemu.

在另一个窗口打开riscv64-unknown-elf-gdb 


使用target remote:26000 截获qemu

使用file kernel/kernel 导入符号表

测试结果

``` shell

(gdb) target remote:26000
Remote debugging using :26000
warning: No executable has been specified and target does not support
determining executable automatically.  Try using the "file" command.
0x0000000000001000 in ?? ()
(gdb) file kernel/kernel
A program is being debugged already.
Are you sure you want to change the file? (y or n) y
Reading symbols from kernel/kernel...
(gdb) b end
Breakpoint 1 at 0x80000002: file kernel/entry.S, line 7.
(gdb) i registers 
...
a0             0x0      0
...
(gdb) c
Continuing.

Breakpoint 1, end () at kernel/entry.S:7
7               j end
(gdb) i registers 
...
a0             0x1      1
...

```
可以发现entry函数正常运行


## 编写makefile

为了后续程序的编译测试方便，这里我为该项目编写最初步的Makefile