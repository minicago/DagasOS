BUILD_DIR = build
K = kernel
U = user

TOOLPREFIX = riscv64-unknown-elf-
OBJDUMP	= $(TOOLPREFIX)objdump

CC = $(TOOLPREFIX)gcc
LD = $(TOOLPREFIX)ld
GDB = $(TOOLPREFIX)gdb

CFLAGS = -Wall -Werror -O0 -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -MD
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -Iinclude
CFLAGS += -fno-stack-protector
CFLAGS += -fno-pie -no-pie

LDFLAGS = -z max-page-size=4096

export BUILD_DIR
export K 
export U
export CC
export LD

export CFLAGS
export LDFLAGS

.PHONY: user sdcard kernel

kernel:  
	make -C $K all

user: 
	make -C $U all

clean :
	rm -rf build/* \
	fs.img \
	.gdbinitg

dst=/mnt/fs
fs.img: sdcard
	@if [ ! -f "fs.img" ]; then \
		echo "making fs image..."; \
		dd if=/dev/zero of=fs.img bs=512 count=4096; \
		mkfs.vfat -F 32 -s 4 fs.img; fi
	-@sudo mkdir $(dst)
	@sudo mount fs.img $(dst)
	-@make sdcard dst=$(dst)
	@sudo umount $(dst)
	@echo "fs image is ready"

# Write sdcard mounted at $(dst)
sdcard: user
	for file in $$( ls $(BUILD_DIR)/$U/_* ); do \
		sudo cp $$file $(dst)/$${file#$(BUILD_DIR)/$U/_}; done
	echo fin;

QEMU = qemu-system-riscv64

CPUS := 1

QEMUBIOS = none
# QEMUBIOS = /home/smokey-days/Desktop/Courses/os/DagasOS/sbi-qemu
QEMUOPTS = -machine virt -bios ${QEMUBIOS} -kernel $(BUILD_DIR)/$K/kernel -m 128M -smp $(CPUS) -nographic
QEMUOPTS += -global virtio-mmio.force-legacy=false
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

qemu: fs.img kernel
	$(QEMU) $(QEMUOPTS)

.gdbinit :
	echo "\
	set confirm off\n \
	set architecture riscv:rv64\n \
	target remote 127.0.0.1:26000\n \
	symbol-file $(BUILD_DIR)/$K/kernel\n \
	set disassemble-next-line auto\n \
	set riscv use-compressed-breakpoints yes\n \
	" > .gdbinit

qemu-gdb: $(BUILD_DIR)/$K/kernel fs.img 
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::26000

run-gdb : .gdbinit
	$(GDB) -command=.gdbinit