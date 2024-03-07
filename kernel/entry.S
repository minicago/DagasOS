.section .text
.global _entry
_entry:
        # do nothing but add a0 by 1
        la sp, stack0 
        lw a0 , stack_size
        add sp, a0, sp
        call start