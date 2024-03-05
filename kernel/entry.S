.section .text
.global _entry
_entry:
        # do nothing but add a0 by 1
        la sp, stack_base_address
        call start