#ifndef __CSR__H__
#define __CSR__H__

# include "types.h"

// CSR const

//MPP
#define MSTATUS_MPP_MASK 0x1800
#define MSTATUS_MPP_M 0x1800
#define MSTATUS_MPP_S 0x0800
#define MSTATUS_MPP_U 0x0000

// PMPCFG
#define PMPADDR0_S_TOR 0x3fffffffffffffull
#define PMPCFG_R 0x1
#define PMPCFG_W 0x2
#define PMPCFG_X 0x4

#define PMPCFG_A_MASK 0x18
#define PMPCFG_A_TOR 0x8 //pmpaddr defines the top of range
#define PMPCFG_A_NA4 0x10 //only for 4-bytes region
#define PMPCFG_A_NAPOT 0x18 //low bits of pmpaddr defines size and high bits of pmpaddr defines base (left shifted by 2)

#define PMPCFG_L 0x20 //lock bit


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