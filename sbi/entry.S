.section .entry
.global _entry
_entry:
        auipc sp, 1024 * 1024 * 128 / 4096 # + 0x200
        # auipc a0, 0x200
        # jalr ra, a0
        call 0x80200000