K=kernel

SRC_C = $(wildcard $K/*.c)

OBJS = $K/entry.o
OBJS += $K/stvec.o \
		$K/mtvec.o

OBJS += $(patsubst $K/%.c, $K/%.o ,$(SRC_C) )



TOOLPREFIX = riscv64-unknown-elf-

CC = $(TOOLPREFIX)gcc
LD = $(TOOLPREFIX)ld
GDB = $(TOOLPREFIX)gdb

CFLAGS = -Wall -Werror -O0 -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -MD
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -I$K/include -I.
CFLAGS += -fno-stack-protector
CFLAGS += -fno-pie -no-pie


LDFLAGS = -z max-page-size=4096

QEMU = qemu-system-riscv64

CPUS := 1

QEMUBIOS = none
# QEMUBIOS = /home/smokey-days/Desktop/Courses/os/DagasOS/sbi-qemu
QEMUOPTS = -machine virt -bios ${QEMUBIOS} -kernel $K/kernel -m 128M -smp $(CPUS) -nographic
QEMUOPTS += -global virtio-mmio.force-legacy=false
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0


$K/kernel: $(OBJS) $K/kernel.ld
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS) 

fs.img:
	dd if=/dev/zero of=fs.img bs=1M count=256

qemu: $K/kernel fs.img
	$(QEMU) $(QEMUOPTS)

.gdbinit :
	echo "\
	set confirm off\n \
	set architecture riscv:rv64\n \
	target remote 127.0.0.1:26000\n \
	symbol-file kernel/kernel\n \
	set disassemble-next-line auto\n \
	set riscv use-compressed-breakpoints yes\n \
	" > .gdbinit

qemu-gdb: $K/kernel fs.img 
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::26000

run-gdb : .gdbinit
	$(GDB) -command=.gdbinit

clean: 
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*/*.o */*.d */*.asm */*.sym \
	$K/kernel fs.img \
	.gdbinit

tags: $(OBJS)
	etags *.S *.c
