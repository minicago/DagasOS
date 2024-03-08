# 文件系统

## 创建FAT32的磁盘镜像

### 创建空FAT32磁盘镜像
```bash
# 创建磁盘镜像
dd if=/dev/zero of=fs.img bs=512 count=512
# 格式化为FAT32文件系统
mkfs.vfat -F 32 -s 4 fs.img
```
- mkfs 是“make filesystem”的缩写，而 vfat 是指 FAT32 文件系统的一种变种，支持长文件名。
- -F 32：这个选项指定文件系统的类型为 FAT32。
- -s 4：这个选项指定每个分配单元的扇区数。扇区是磁盘存储的基本单元，大多数情况下，一个扇区的大小是 512 字节。这里 -s 4 表示每个簇（FAT文件系统中的分配单元）包含4个扇区，因此每个簇的大小为 2048 字节（4 * 512）。

### 写入编译好的文件
```bash
mount fs.img \\mnt
cp app \\mnt
umount \\mnt
```

