#include "uart.h"
#include "types.h"
// qemu puts UART registers here in physical memory.

void uartputc_sync(int c)
{
//   push_off();

//   if(panicked){
//     for(;;)
//       ;
//   }

  // wait for Transmit Holding Empty to be set in LSR.
   while((R_UART_REG(LSR) & LSR_TX_IDLE) == 0)
     ;
  W_UART_REG(THR, c);

//   pop_off();
}

void uartinit(void)
{
  // disable interrupts.
  W_UART_REG(IER, 0x00);

  // special mode to set baud rate.
  W_UART_REG(LCR, LCR_BAUD_LATCH);

  // LSB for baud rate of 38.4K.
  W_UART_REG(0, 0x03);

  // MSB for baud rate of 38.4K.
  W_UART_REG(1, 0x00);

  // leave set-baud mode,
  // and set word length to 8 bits, no parity.
  W_UART_REG(LCR, LCR_EIGHT_BITS);

  // reset and enable FIFOs.
  W_UART_REG(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);

  // enable transmit and receive interrupts.
  //TODO: open this
  //W_UART_REG(IER, IER_TX_ENABLE | IER_RX_ENABLE);

}