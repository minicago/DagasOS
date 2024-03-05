#ifndef __CSR__H__
#define __CSR__H__

# include "types.h"

// CSR const

#define MSTATUS_MPP_MASK 0x1800
#define MSTATUS_MPP_M 0x1800
#define MSTATUS_MPP_S 0x0800
#define MSTATUS_MPP_U 0x0000


// CSR operation

# define W_CSR(csr, variable) \
asm("CSRRW x0, "#csr", %0": :"r"(variable))

# define R_CSR(csr, variable) \
asm("CSRRW %0, "#csr", x0": "=r"(variable):)

# define S_CSR(csr, variable) \
asm("CSRRS x0, "#csr", %0": :"r"(variable))

# define C_CSR(csr, variable) \
asm("CSRRC x0, "#csr", %0": :"r"(variable))

// W_CSR mepc : the address to which mret return

#endif