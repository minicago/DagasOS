#ifndef __CSR__H__
#define __CSR__H__

# include "types.h"

// CSR const

//MPP
#define MSTATUS_MPP_MASK 0x1800
#define MSTATUS_MPP_M 0x1800
#define MSTATUS_MPP_S 0x0800
#define MSTATUS_MPP_U 0x0000

//MIE
#define MSTATUS_MIE 0x8

//SPP
#define SSTATUS_SPP 0x100

//SIE
#define SSTATUS_SIE 0x2

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

// INTR
#define INTR_MASK 0xaaa

#define S_INTR_MASK 0x222
#define S_SOFTWARE_INTR 0x2
#define S_TIMER_INTR 0x20
#define S_EXTERNAL_INTR 0x200

#define M_INTR_MASK 0x888
#define M_SOFTWARE_INTR 0x8
#define M_TIMER_INTR 0x80
#define M_EXTERNAL_INTR 0x800

// EXCEPTION
#define EXC_MASK 0xbbff
#define INSTRUCTION_ADDRESS_MISALIGNED_EXC 0x1
#define INSTRUCTION_ACCESS_FAULT_EXC 0x2
#define ILLEGAL_INTSTRUCTION_EXC 0x4
#define BREAK_POINT_EXC 0x8
#define LOAD_ADDRESS_MISALIGNED_EXC 0x10
#define LOAD_ACCESS_FAULT_EXC 0x20
#define STORE_AMMO_ADDRESS_MISALIGNED_EXC 0x40
#define STORE_AMMO_ACESS_FAULT_EXC 0x80
#define ENVIRONMENT_CALL_FROM_U_MODE_EXC 0x100
#define ENVIRONMENT_CALL_FROM_S_MODE_EXC 0x200
#define ENVIRONMENT_CALL_FROM_M_MODE_EXC 0x800
#define INSTRUCTION_PAGE_FAULT 0x1000
#define LOAD_PAGE_FAULT 0x2000
#define STORE_AMMO_PAGE_FAULT 0x8000

// CSR operation

# define W_CSR(csr, variable) \
asm("CSRW "#csr", %0": :"r"((uint64) variable))

# define R_CSR(csr, variable) \
asm("CSRR %0, "#csr: "=r"(variable):)

# define S_CSR(csr, variable) \
asm("CSRRS x0, "#csr", %0": :"r"((uint64) variable))

# define C_CSR(csr, variable) \
asm("CSRRC x0, "#csr", %0": :"r"((uint64) variable))

// CSR mepc : the address to which mret return

#endif