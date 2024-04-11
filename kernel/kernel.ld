OUTPUT_ARCH( "riscv" ) 
ENTRY (start)

SECTIONS {
    . = 0x80200000; /*QEMU entry is set at 0x80000000*/
    .text : {
        *start.o(.text)
        *(.text)
        . = ALIGN(0x1000);
        trampoline_base = .;
        *(trampsec)
        . = ALIGN(0x1000);
        ASSERT(. - trampoline_base == 0x1000, "error: trampoline larger than one page");
    }
    . = ALIGN(16);
    .bss : {
        *(.bss)
    }
    .data : {
        *(.data)
    }
    .rodata : {
        *(.srodata)
    }
    . = ALIGN(0x1000);
    PROVIDE(pmem_base = .);
}