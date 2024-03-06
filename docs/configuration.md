# Configuration


## Tool Chain Installing

### install qemu

just download code and compile as saying.

### compile ``riscv64-unknown-elf``

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

make -j$(nproc)
# 编译完成后进行安装
sudo make install
```

## start first version

BYY has wrote a fine Makefile, so just ``make qemu-gdb``

Noticing that we may use ``Ctrl+A X`` to exit qemu.