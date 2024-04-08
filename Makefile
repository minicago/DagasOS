BUILD_DIR = build
K = kernel
U = user
TEST = riscv-syscalls-testing

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

.PHONY: user sdcard kernel testsuit test qemu

kernel:  
	make -C $K all

user: 
	make -C $U all

$(TEST)/$U/riscv64:
	make -C $(TEST)/$U all CHAPTER=7

clean :
	@make -C $(TEST)/$U clean
	rm -rf build/* \
	sdcard.img \
	.gdbinitg

dst=/mnt/sdcard
sdcard.img: sdcard
	@if [ ! -f "sdcard.img" ]; then \
		echo "making fs image..."; \
		dd if=/dev/zero of=sdcard.img bs=512 count=4096; \
		mkfs.vfat -F 32 -s 4 sdcard.img; fi
	-@sudo mkdir $(dst)
	@sudo mount sdcard.img $(dst)
	-@make sdcard dst=$(dst)
	@sudo umount $(dst)
	@echo "fs image is ready"

# Write sdcard mounted at $(dst)
sdcard: user $(TEST)/$U/riscv64

	for file in $$( ls $(BUILD_DIR)/$U/_* ); do \
		sudo cp $$file $(dst)/$${file#$(BUILD_DIR)/$U/_}; done
	echo fin;
	
	-sudo mkdir $(dst)/syscalls_test
	sudo cp -rf $(TEST)/$U/riscv64/* $(dst)/syscalls_test

QEMU = qemu-system-riscv64

CPUS := 1

QEMUBIOS = none
# QEMUBIOS = /home/smokey-days/Desktop/Courses/os/DagasOS/sbi-qemu
QEMUOPTS = -machine virt -bios ${QEMUBIOS} -kernel $(BUILD_DIR)/$K/kernel -m 128M -smp $(CPUS) -nographic
QEMUOPTS += -global virtio-mmio.force-legacy=false
QEMUOPTS += -drive file=sdcard.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

qemu: sdcard.img kernel
	$(QEMU) $(QEMUOPTS)

all : sdcard.img kernel

.gdbinit :
	echo "\
	set confirm off\n \
	set architecture riscv:rv64\n \
	target remote 127.0.0.1:26000\n \
	symbol-file $(BUILD_DIR)/$K/kernel\n \
	set disassemble-next-line auto\n \
	set riscv use-compressed-breakpoints yes\n \
	" > .gdbinit

qemu-gdb: $(BUILD_DIR)/$K/kernel sdcard.img 
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::26000

run-gdb : .gdbinit
	$(GDB) -command=.gdbinit