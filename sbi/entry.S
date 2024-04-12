.section .entry
.global _entry
_entry:
        auipc sp, KMEMORY / 4096
        # auipc a0, 0x200
        # addi a0, a0, -4
        # jalr ra, a0
        call start