OUTPUT_ARCH( "riscv" ) 
ENTRY (_entry)

SECTIONS {
    . = 0x80000000; /*QEMU entry is set at 0x80000000*/
    .text : {
        *(.text)
    }
    .bss : {
        *(.bss)
    }
    .data : {
        *(.data)
    }
    .rodata : {
        *(.srodata)
    }

    PROVIDE(pmem_base = .);
}