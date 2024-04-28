BUILD_DIR = build
K = kernel
U = user
SBI = sbi
UTILS = utils
TEST = riscv-syscalls-testing

CPUS = 2
KMEMORY = 128*1024*1024

TOOLPREFIX = riscv64-unknown-elf-
OBJDUMP	= $(TOOLPREFIX)objdump

CC = $(TOOLPREFIX)gcc
LD = $(TOOLPREFIX)ld
GDB = $(TOOLPREFIX)gdb
INITRD_BLOCK_SIZE = 128
INITRD_BLOCK_NUM = 128

CFLAGS = -Wall -Werror -O0 -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -MD
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -Iinclude
CFLAGS += -fno-stack-protector
CFLAGS += -fno-pie -no-pie
CFLAGS += -DKMEMORY=$(KMEMORY) -DCPUS=$(CPUS) -DINITRDIMG_SIZE=$(INITRD_BLOCK_SIZE)*$(INITRD_BLOCK_NUM)*1024

LDFLAGS = -z max-page-size=4096

ROOT_USER = $(shell id -u)
ROOT_OR_SUDO = sudo
ifeq ($(ROOT_USER),0)
	ROOT_OR_SUDO =
	DDEBUG = -DNO_PRINT
endif

sdcard_dst=/mnt/sdcard
initrd_dst=/mnt/initrd

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

initrd: user
	@for file in $$( ls $(BUILD_DIR)/$U/_* ); do \
		$(ROOT_OR_SUDO) cp $$file $(initrd_dst)/$${file#$(BUILD_DIR)/$U/_}; done
	@echo fin;

utils:
	make -C $(UTILS) all


$(TEST)/$U/riscv64:
	make -C $(TEST)/$U all CHAPTER=7


umount:
	-@$(ROOT_OR_SUDO) umount $(sdcard_dst)
	-@$(ROOT_OR_SUDO) umount $(initrd_dst)

sdcard.img:
	@if [ ! -f "sdcard.img" ]; then \
		echo "making fs image..."; \
		$(ROOT_OR_SUDO) umount $(sdcard_dst) > /dev/null; \
		dd if=/dev/zero of=sdcard.img bs=512k count=128; \
		mkfs.vfat -F 32 sdcard.img; fi
	-@$(ROOT_OR_SUDO) mkdir $(sdcard_dst)
	@$(ROOT_OR_SUDO) mount sdcard.img $(sdcard_dst)
	-@make sdcard sdcard_dst=$(sdcard_dst)
	@$(ROOT_OR_SUDO) umount $(sdcard_dst)
	@echo "sdcard image is ready"


# Write sdcard mounted at $(sdcard_dst)
sdcard: $(TEST)/$U/riscv64
# for file in $$( ls $(BUILD_DIR)/$U/_* ); do \
# 	$(ROOT_OR_SUDO) cp $$file $(sdcard_dst)/$${file#$(BUILD_DIR)/$U/_}; done
# echo fin;
	
# -$(ROOT_OR_SUDO) mkdir $(sdcard_dst)/syscalls_test
	$(ROOT_OR_SUDO) cp -rf $(TEST)/$U/riscv64/* $(sdcard_dst)

QEMU = qemu-system-riscv64

QEMUBIOS = sbi-qemu
QEMUOPTS = -machine virt -bios $(QEMUBIOS) -kernel kernel-qemu -m 128M -smp $(CPUS) -nographic
# QEMUOPTS += -global virtio-mmio.force-legacy=false
QEMUOPTS += -drive file=sdcard.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
QEMUOPTS += -initrd initrd.img 

if_root: 
ifeq ($(ROOT_USER),0)
	@mkdir build
endif

qemu: sdcard.img kernel sbi initrd.img umount
	$(QEMU) $(QEMUOPTS)

all : if_root sdcard.img kernel sbi initrd.img

.gdbinit :
	echo "\
	set confirm off\n \
	set architecture riscv:rv64\n \
	target remote 127.0.0.1:26000\n \
	symbol-file $(symbol-file) \n \
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
	kernel-qemu \
	initrd.img \
	sbi-qemu 

initrd.img : 
	@if [ ! -f "initrd.img" ]; then \
		echo "making fs image..."; \
		$(ROOT_OR_SUDO) umount $(initrd_dst) > /dev/null; \
		dd if=/dev/zero of=initrd.img bs=$(INITRD_BLOCK_SIZE)k count=$(INITRD_BLOCK_NUM) ; \
		mkfs.vfat -F 32 initrd.img; fi
	-@$(ROOT_OR_SUDO) mkdir $(initrd_dst)
	@$(ROOT_OR_SUDO) mount initrd.img $(initrd_dst)
	-@make initrd
	@$(ROOT_OR_SUDO) umount $(initrd_dst)
	@echo "initrd image is ready"


xiji : all
	qemu-system-riscv64 -machine virt -kernel kernel-qemu \
	 -m 128M -nographic -smp 2 -bios sbi-qemu \
	 -drive file=sdcard.img,if=none,format=raw,id=x0 \
	 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 \
	-initrd initrd.img \