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

void uartputc_sync(int c)
{
//   push_off();

//   if(panicked){
//     for(;;)
//       ;
//   }

  // wait for Transmit Holding Empty to be set in LSR.
  while((ReadReg(LSR) & LSR_TX_IDLE) == 0)
    ;
  WriteReg(THR, c);

//   pop_off();
}