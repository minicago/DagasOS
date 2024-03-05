# 初步环境

## 目标

1) 内核能正常开始运行entry的汇编。

2) 可以使用gdb去调试内核

## 步骤

1) 编写entry.S汇编代码，使用交叉编译工具链编译并链接entry.S，生成内核。

2) 尝试使用shell指令生成空的用户程序磁盘映像。

3) 在shell中运行qemu

4) 为qemu添加gdb所用的参数，并且在gdb中调试内核

5) 为以上内容构建Makefile，方便之后的调试

## 参考资料

https://github.com/mit-pdos/xv6-riscv/

## 必须文件

### kernel.ld user.ld

链接脚本

kernel.ld 保证entry.S / _entry 在 0x80000000 * where qemu's -kernel jumps.

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


## 编写Makefile

为了后续程序的编译测试方便，这里我为该项目编写最初步的Makefile。

### tags

tags 伪对象可以自动追踪函数的实现，可以大幅简化Makefile。

### %

通配符， 可以为每个符合要求的对象生成对应的目标。