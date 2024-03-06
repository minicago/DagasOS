# Uart and Puts

## AVX

AVX (as well as rcore-os) code as follow:

``` c
void consputc(int c) {
  if (c == BACKSPACE) {
    // if the user typed backspace, overwrite with a space.
#ifdef visionfive
    uart8250_putc('\b');
    uart8250_putc(' ');
    uart8250_putc('\b');
#else
    sbi_console_putchar('\b');
    sbi_console_putchar(' ');
    sbi_console_putchar('\b');
#endif
  } else if (c == '\n' || c == '\r') {
#ifdef visionfive
    uart8250_putc('\r');
    uart8250_putc('\n');
#else
    sbi_console_putchar('\n');
#endif
  } else {
#ifdef visionfive
    uart8250_putc(c);
#else
    sbi_console_putchar(c);
#endif
  }
}
```

Which means they call ``uart8250_putc`` function when realmachine instead of ``sbi_console_putchar`` in stimulation.

In which, they use a asm like ecall to output:
``` rust
// from rcore-os
pub fn console_putchar(ch: u8) {
    let ret: usize;
    let arg0: usize = ch as usize;
    let arg1: usize = 0;
    let arg2: usize = 0;
    let which: usize = 1;
    unsafe {
        asm!("ecall"
             : "={x10}" (ret)
             : "{x10}" (arg0), "{x11}" (arg1), "{x12}" (arg2), "{x17}" (which)
             : "memory"
             : "volatile"
        );
    }
}
```

hint: sbi is abbr. of Serial Bus Interface

We focus on stimulation.

As https://www.bookstack.cn/read/rCore_tutorial_doc/8aa6b11e33b20358.md saying, they are using OpenSBI lib to manage hardwires.

So we may consider that they are just calling OpenSBI. In fact, it may be a kind of bios which qemu will start with when we set ``-bios default``

But when trying this, I encountered:
``` shell
qemu-system-riscv64: Unable to load the RISC-V firmware "opensbi-riscv64-virt-fw_jump.bin"
```

I stuck here. Where should be our main direction.


## xv6

for xv6, it code as follow
``` c
void uartputc_sync(int c)
{
  push_off();

  if(panicked){
    for(;;)
      ;
  }

  // wait for Transmit Holding Empty to be set in LSR.
  while((ReadReg(LSR) & LSR_TX_IDLE) == 0)
    ;
  WriteReg(THR, c);

  pop_off();
}
```

hint: its ``consputc`` func is simply a package-up of ``uartputc_sync`` . 

Here they just write into a (serials of) uart control regs.

They use ``WriteReg`` to write it to uart, which is a macro define.

Core defines pasted as follow:

```c
// qemu puts UART registers here in physical memory.
#define UART0 0x10000000L
#define UART0_IRQ 10

#define Reg(reg) ((volatile unsigned char *)(UART0 + reg))

#define THR 0 // transmit holding register (for output bytes)
#define LSR 5 // line status register

#define LSR_RX_READY (1<<0) // input is waiting to be read from RHR
#define LSR_TX_IDLE (1<<5) // THR can accept another character to send


#define ReadReg(reg) (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))

```

So we may consider that if we'd like to write sth, we should just set ``(byte *)(0x10000000L) = c``

But, I haven't found where xv6 specificed it(0x10000000L) as uart0.