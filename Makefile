BUILD_DIR = build
K = kernel
U = user
SBI = sbi
UTILS = utils
TEST = riscv-syscalls-testing

CPUS = 1
KMEMORY = 128*1024*1024

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
CFLAGS += -DKMEMORY=$(KMEMORY) -DCPUS=$(CPUS)

LDFLAGS = -z max-page-size=4096

export BUILD_DIR
export K 
export U
export CC
export LD

export CFLAGS
export LDFLAGS
export CPUS
export MEMORY_MB

.PHONY: user sdcard testsuit test qemu sbi kernel utils

kernel:  
	-rm kernel-qemu
	make -C $K all DDEBUG=$(DDEBUG)
	cp $(BUILD_DIR)/$K/kernel kernel-qemu

sbi: utils
	-rm sbi-qemu
	make -C $(SBI) all
	cp $(BUILD_DIR)/$(SBI)/sbi sbi-qemu

user: $(TEST)/$U/riscv64
	make -C $U all

utils:
	make -C $(UTILS) all


$(TEST)/$U/riscv64:
	make -C $(TEST)/$U all CHAPTER=7

dst=/mnt/sdcard
sdcard.img:
	@if [ ! -f "sdcard.img" ]; then \
		echo "making fs image..."; \
		sudo umount /mnt/sdcard > /dev/null; \
		dd if=/dev/zero of=sdcard.img bs=512k count=128; \
		mkfs.vfat -F 32 -s 4 sdcard.img; fi
	-@sudo mkdir $(dst)
	@sudo mount sdcard.img $(dst)
	-@make sdcard dst=$(dst)
	@sudo umount $(dst)
	@echo "sdcard image is ready"

# Write sdcard mounted at $(dst)
sdcard: user

	for file in $$( ls $(BUILD_DIR)/$U/_* ); do \
		sudo cp $$file $(dst)/$${file#$(BUILD_DIR)/$U/_}; done
	echo fin;
	
	-sudo mkdir $(dst)/syscalls_test
	sudo cp -rf $(TEST)/$U/riscv64/* $(dst)/syscalls_test

QEMU = qemu-system-riscv64

CPUS := 1

QEMUBIOS = sbi-qemu
QEMUOPTS = -machine virt -bios $(QEMUBIOS) -kernel kernel-qemu -m 128M -smp $(CPUS) -nographic
QEMUOPTS += -global virtio-mmio.force-legacy=false
QEMUOPTS += -drive file=sdcard.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

qemu: sdcard.img kernel sbi
	$(QEMU) $(QEMUOPTS)

all : sdcard.img kernel sbi

.gdbinit :
	echo "\
	set confirm off\n \
	set architecture riscv:rv64\n \
	target remote 127.0.0.1:26000\n \
	symbol-file kernel-qemu \n \
	symbol-file build/sbi/sbi-elf \n \
	set disassemble-next-line auto\n \
	set riscv use-compressed-breakpoints yes\n \
	" > .gdbinit

qemu-gdb: kernel sdcard.img sbi
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::26000

run-gdb : .gdbinit
	$(GDB) -command=.gdbinit

clean :
	@make -C $(TEST)/$U clean
	rm -rf build/* \
	sdcard.img \
	.gdbinit \
	kernel-qemu \
	sbi-qemu 