OUTPUT_ARCH( "riscv" ) 
ENTRY (_entry)
BASE_ADDRESS = 0x80200000;
SECTIONS {
    . = BASE_ADDRESS; /*QEMU entry is set at 0x80000000*/
    .text : {
        *(.text)
    }
    .bss : {
        *(.bss)
    }
    .data : {
        *(.data)
    }
    .srodata : {
        *(.srodata)
    }
}